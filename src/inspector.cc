#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <dirent.h>
#include <string.h>

#include <sys/epoll.h>
#include <linux/input.h>
#include <xkbcommon/xkbcommon.h>
#include "input_event_names.h"

struct Settings {
    struct xkb_rule_names xkb_names = {
    .rules = NULL,
    .model = NULL,
    .layout = NULL,
    .variant = NULL,
    .options = NULL,
    };
    bool only_print = false;
};
struct Keyboard {
    int event_number;
    int fd;
    char * name;
};

int is_keyboard(int fd) {
    errno = 0;
    u_int64_t evbitset = 0;
    u_int64_t keybits[4] = {0};
    ioctl(fd, EVIOCGBIT(0, sizeof(evbitset)), &evbitset);
    if (errno) return 0;
    if (!(evbitset & (1ul << EV_KEY))) {
        return 0;
    }

    errno = 0;
    ioctl(fd, EVIOCGBIT(EV_KEY, sizeof(keybits)), keybits);
    if (errno)
        return 0;

    if (keybits[0] | keybits[1] | keybits[2] | keybits[3]) {
        return 1;
    }
    return 0;
}

int kbd_suffix(char * name){
    int len = strlen(name);
    return len > 3 && strcmp(name + (len - 3), "kbd") == 0;
}

struct KeyboardList {
    struct Keyboard keyboards[32]; //12 Slots
    int len = 0;
    bool contains_event_number(int event_number) {
        for(int i = 0; i < len; i++) {
            if (keyboards[i].event_number == event_number)
                return true;
        }
        return false;
    }
    void append(int event_number, int fd, char*name) {
        if (len >= 32) return;
        keyboards[len].event_number = event_number;
        keyboards[len].fd = fd;
        keyboards[len].name = name;
        len++;
    }

    void print() {
        for(int i = 0; i < len; i++) {
            printf("%s\n", keyboards[i].name);
        }
    }
};

void find_keyboard_in(KeyboardList &keyboards, const char * path) {
    DIR *d;
    struct dirent *dir;
    struct Keyboard inputs[32] = {0}; //12 Slots
    int inputs_len = 0;
    char buf[512]; //12 Slots
    char input[256]; //12 Slots
    d = opendir(path);
    if (d) {
        while ((dir = readdir(d)) != NULL) {
            if (kbd_suffix(dir->d_name)) {
                sprintf(buf,"%s%s",path, dir->d_name);
                if (realpath(buf, input) == NULL) continue;
                if (input[11] == 'e') {
                    int event_number = atoi(input + 16);
                    if (!keyboards.contains_event_number(event_number)) {
                        int fd = open(input, O_RDONLY);
                        if (!is_keyboard(fd)){
                            close(fd);
                        } else {
                            unsigned int key_repeat[2] = {2048,2048};
                            ioctl(fd, EVIOCSREP, &key_repeat); //Disable Local Simulated Key repeat
                            keyboards.append(event_number, fd, strdup(buf));
                        }
                    }
                }
            }
        }
        closedir(d);
    }
}

void print_mods(xkb_state * state) {
    struct {const char *name; const char *label;} mods[] = {
    {XKB_MOD_NAME_LOGO, "SUPER"},
    {XKB_MOD_NAME_ALT, "ALT"},
    {XKB_MOD_NAME_SHIFT, "SHIFT"},
    {XKB_MOD_NAME_CTRL, "CONTROL"},
    };
    int found = 0;
    for (auto & mod: mods) {
        if (xkb_state_mod_name_is_active(state, mod.name,
                                         XKB_STATE_MODS_EFFECTIVE) > 0) {
            if (found == 0) {
                printf("   XKB MODS: ");
            } else {
                printf(" ");
            }
            found++;
            printf("%s", mod.label);
        }
    }
    if (found > 0) {
        printf(";\n");
    }
}

#define V_KEY_UP 0
#define V_KEY_DOWN 1
#define V_KEY_REPEAT 2
void read_events(xkb_keymap *keymap,xkb_state *state, Keyboard *input) {
    struct input_event buf[16];
    errno = 0;
    int rd = read(input->fd, buf, sizeof(buf));
    struct input_event *end = buf + (rd / sizeof(struct input_event));
    int after_scancode = 0;
    for(const struct input_event *curr = buf; curr < end; curr++){
        if (curr->type != EV_KEY || curr->value == V_KEY_REPEAT) continue;
        xkb_keycode_t keycode;
        xkb_keysym_t keysym;
        keycode = curr->code + 8;
        keysym = xkb_state_key_get_one_sym(state, keycode);
        char keysym_name[64];
        xkb_keysym_get_name(keysym, keysym_name, sizeof(keysym_name));
        const char * keyname = xkb_keymap_key_get_name(keymap, keycode);
        printf("[%s]{\n",input->name);
        printf("   EVDEV: keycode:%d name:%s;\n", curr->code, key_code_name(curr->code));
        print_mods(state);
        printf("   XKB: key[%d]:<%s> keysym:%s;\n",
               keycode, keyname,keysym_name);
        if (curr->value == V_KEY_DOWN)
            xkb_state_update_key(state, keycode, XKB_KEY_DOWN);
        else if (curr->value == V_KEY_UP)
            xkb_state_update_key(state, keycode, XKB_KEY_UP);
        printf("}\n");
    }
}


int poll_keyboard_events(Settings &settings, KeyboardList &keyboards) {

    struct xkb_context *ctx = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
    struct xkb_rule_names names = settings.xkb_names;
    struct xkb_keymap *keymap = xkb_keymap_new_from_names(ctx, &names,
                                       XKB_KEYMAP_COMPILE_NO_FLAGS);
    struct xkb_state *state = xkb_state_new(keymap);
    int epfd, ret;
    struct epoll_event evs[16];


    epfd = epoll_create1(0);
    if (epfd < 0) {
        fprintf(stderr, "Couldn't create epoll instance: %s\n",
                strerror(errno));
        return -errno;
    }

    for (int i =0; i < keyboards.len; i++) {
        struct epoll_event ev;
        memset(&ev, 0, sizeof(ev));
        ev.events = EPOLLIN;
        ev.data.ptr = keyboards.keyboards + i;
        ret = epoll_ctl(epfd, EPOLL_CTL_ADD, keyboards.keyboards[i].fd, &ev);
        if (ret) {
            ret = -errno;
            fprintf(stderr, "Couldn't add %s to epoll: %s\n",
                    keyboards.keyboards[i].name, strerror(errno));
            goto err_epoll;
        }
    }

    while (1) {
        ret = epoll_wait(epfd, evs, 16, -1);
        if (ret < 0) {
            if (errno == EINTR) continue;
            ret = -errno;
            fprintf(stderr, "Poll Error for events: %s\n",
                    strerror(errno));
            goto err_epoll;
        }
        for (int i = 0; i < ret; i++) {
            read_events(keymap, state, (Keyboard *)evs[i].data.ptr);
            // kbd = evs[i].data.ptr;
            // ret = read_keyboard(kbd);
            // if (ret) {
            //     goto err_epoll;
            // }
        }
    }

    close(epfd);
    return 0;

err_epoll:
    close(epfd);
    return ret;
}

void print_usage(){
    static const char * DOCS_STRING =
        "evdoublebind-inspector " "0.1"
R"XXX(

Usage:
   evdoublebind-inspector [options]

Options:
   -k                                    Only print keyboards
   -l <xkb_layout>                       Specifies XKB layout
   (default: XKB System Default)
   -r <xkb_rules>                        Specifies XKB rules
   (default: XKB System Default)
   -m <xkb_model>                        Specifies XKB model
   (default: XKB System Default)
   -o <xkb_options>                      Specifies XKB options
   (default: XKB System Default)
   -v <xkb_variant>                      Specifies XKB variant
   (default: XKB System Default)
)XXX";
    puts(DOCS_STRING);
}
Settings parse_arguments(Settings &&setting, int argc, char **argv){
    opterr = 0;
    optind = 0;
    char *cvalue = NULL;
    int c;
    while ((c = getopt (argc, argv, "hr:m:l:v:o:k")) != -1)
        switch(c){
            case 'h':
                print_usage();
                exit(0);
            case 'l':
                setting.xkb_names.layout = optarg;
                continue;
            case 'm':
                setting.xkb_names.model = optarg;
                continue;
            case 'r':
                setting.xkb_names.rules = optarg;
                continue;
            case 'o':
                setting.xkb_names.options = optarg;
                continue;
            case 'v':
                setting.xkb_names.variant = optarg;
                continue;
            case 'k':
                setting.only_print = true;
                continue;
        }
    return setting;
}
int main(int argc, char *argv[]) {
    Settings settings = parse_arguments(Settings(), argc, argv);
    KeyboardList keyboards;
    find_keyboard_in(keyboards, "/dev/input/by-id/");
    find_keyboard_in(keyboards, "/dev/input/by-path/");
    if(settings.only_print) {
        keyboards.print();
        exit(1);
    }
    printf("Found %d Possible Keyboards.\n---\n", keyboards.len);
    keyboards.print();
    printf("---\n");
    if (poll_keyboard_events(settings, keyboards)) {
        perror("ERROR");
    }
    return 0;
}
