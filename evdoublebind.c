#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>

#include <linux/input.h>

typedef struct KeyBind {
    __u16 key;
    __u16 tap_key;
    __u16 tap_modifier;
} KeyBind;

const KeyBind KEYMAP[] = {
    {.key = KEY_LEFTSHIFT, .tap_key = KEY_BACKSLASH},
    {.key = KEY_RIGHTSHIFT,.tap_key = KEY_7, .tap_modifier = KEY_LEFTSHIFT},
    {.key = KEY_SPACE,     .tap_key = KEY_F13},
    {.key = KEY_CAPSLOCK,  .tap_key = KEY_ESC},
    {.key = KEY_SEMICOLON, .tap_key = KEY_F14},
    {.key = KEY_LEFTALT,   .tap_key = KEY_6, .tap_modifier = KEY_LEFTSHIFT}
};

void perror(const char *str){
    int i = 0;
    for (const char *q = str; *q != '\0'; q++) i++;
    write(STDERR_FILENO, str, i);
}

#define KEYMAP_LEN  sizeof(KEYMAP)/sizeof(KeyBind)

#define INPUT_BUF_LEN 16
#define V_KEY_UP 0
#define V_KEY_DOWN 1
#define V_KEY_REPEAT 2

int tap_mod_key(__u16 key_code, __u16 mod_code, int fd){
    struct input_event buf[] = {{{0,0}, .type = EV_KEY, .code = mod_code, .value = 1},
                         {{0,0}, .type = EV_KEY, .code = key_code, .value = 1},
                         {{0,0}, .type = EV_KEY, .code = key_code, .value = 0},
                         {{0,0}, .type = EV_KEY, .code = mod_code, .value = 0},
                         {{0,0}, .type = EV_SYN, .code = SYN_REPORT, .value = 0}};
    return write(fd, buf, sizeof(buf));
}

int tap_key(__u16 key_code, int fd){
    struct input_event buf[] = {{{0,0}, .type = EV_KEY, .code = key_code, .value = 1},
                         {{0,0}, .type = EV_SYN, .code = SYN_REPORT, .value = 0},
                         {{0,0}, .type = EV_KEY, .code = key_code, .value = 0},
                         {{0,0}, .type = EV_SYN, .code = SYN_REPORT, .value = 0}};
    write(fd, buf, sizeof(buf));
    return 0;
}

const KeyBind *find_keybind(int keycode){
    for(int i = 0; i < KEYMAP_LEN; i++)
        if(KEYMAP[i].key == keycode) return &KEYMAP[i];
    return NULL;
}

void map_events(int fd){
    struct input_event buf[INPUT_BUF_LEN];
    u_int32_t previous_scancode = 0, current_scancode = 0;
    while(1) {
        int rd = read(fd, buf, sizeof(buf));
        if (rd == -1) {
            perror("Failed Reading Input");
            exit(1);
        }
        struct input_event *end = buf + (rd / sizeof(struct input_event));
        int after_scancode = 0;
        for(const struct input_event *curr = buf; curr < end; curr++){
            if (curr->type == EV_MSC && curr->code == MSC_SCAN) {
                current_scancode = curr->value;
                after_scancode = 1;
                continue;
            }

            if (curr->type != EV_KEY || !after_scancode || curr->value == V_KEY_REPEAT) continue;
            after_scancode = 0;

            if (curr->value == V_KEY_DOWN) {
                previous_scancode = current_scancode;
            } else if (current_scancode == previous_scancode) {
                const KeyBind *index = find_keybind(curr->code);
                if (!index) continue;
                if (index->tap_modifier)
                    tap_mod_key(index->tap_key,index->tap_modifier, fd);
                else
                    tap_key(index->tap_key, fd);

            }
        }
    }
}

int main(int argc, char *argv[]) {
    int fd = open(argv[1], O_RDWR);
    unsigned int key_repeat[2] = {2048,2048};
    ioctl(fd, EVIOCSREP, &key_repeat);
    map_events(fd);
    return 0;
}
