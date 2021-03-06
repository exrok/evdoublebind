#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>

#include <linux/input.h>

// KEY_TIMEOUT_CONFIG: Number of microseconds before a keypress will not insert
//   a tap even if press aloned. Usefull when used in graphical applications
//   with a mouse, setting it too 0 disables it.
#define KEY_TIMEOUT_CONFIG 220000


typedef struct KeyBind {
    __u16 key;
    __u16 tap_key;
    __u16 tap_modifier;
} KeyBind;

static KeyBind KEYMAP[64];
static int  KEYMAP_LEN = 0;
// Evdoublebind Code.
// Parses something like '42:189,54:190|32,56:191,57:192,39:193,58:194'
// null terminated.
int parse_keymap(const char * mapstr) {
    const char *c = mapstr;
    int val = 0;
    while (1) {
        switch (*c) {
            case '\0': //fallthrough
            case ',': //fallthrough
                if (KEYMAP[KEYMAP_LEN].tap_key != 0)
                    KEYMAP[KEYMAP_LEN].tap_modifier = val;
                else
                    KEYMAP[KEYMAP_LEN].tap_key = val;
                ++KEYMAP_LEN;
                val = 0;
                if (*c == '\0') {
                    return KEYMAP_LEN;
                }
                break;
            case ':': //fallthrough
                KEYMAP[KEYMAP_LEN].key = val;
                val = 0;
                break;
            case '|': //fallthrough
                KEYMAP[KEYMAP_LEN].tap_key = val;
                val = 0;
                break;
            default:
                if (*c > '9' || *c < '0') {
                    exit(1);
                }
                val = val*10 + (*c - '0');
        }
        c++;
    }
}
void perror(const char *str){
    int i = 0;
    for (const char *q = str; *q != '\0'; q++) i++;
    write(STDERR_FILENO, str, i);
}


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
                         {{0,0}, .type = EV_KEY, .code = key_code, .value = 0},
                         {{0,0}, .type = EV_SYN, .code = SYN_REPORT, .value = 0}};
    return write(fd, buf, sizeof(buf));
}

const KeyBind *find_keybind(int keycode){
    for(int i = 0; i < KEYMAP_LEN; i++)
        if(KEYMAP[i].key == keycode) return &KEYMAP[i];
    return NULL;
}
int open_input(char * path) {
    int fd = open(path, O_RDWR);
    if (fd < 0) {
        perror("Error: Opening Keyboard");
        exit(1);
    }
    unsigned int key_repeat[2] = {2048,2048};
    ioctl(fd, EVIOCSREP, &key_repeat);
    return fd;
}

void map_events(char *input_path){
    struct input_event buf[INPUT_BUF_LEN];
    u_int32_t previous_scancode = 0, current_scancode = 0;
    int fd = open_input(input_path);

#if KEY_TIMEOUT_CONFIG != 0
    time_t last_sec = 0;
    suseconds_t last_usec = 0;
#endif
    while(1) {
        int rd = read(fd, buf, sizeof(buf));
        if (rd == -1) {
            close(fd);
            //Wait for device initialization
            for (int i = 0; i < 20; i++) { //Try acessing the path for 2s
                if (access(input_path, F_OK) != -1) break;
                usleep(100000);
            }
            int fd = open_input(input_path); //Try again maybe device reset.
            rd = read(fd, buf, sizeof(buf));
            if (rd != -1) {
                perror("Error: Opening Keyboard");
                exit(1);
            }
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
#if KEY_TIMEOUT_CONFIG != 0
                last_sec = curr->time.tv_sec;
                last_usec = curr->time.tv_usec;
#endif
                previous_scancode = current_scancode;
            } else if (current_scancode == previous_scancode) {
#if KEY_TIMEOUT_CONFIG != 0
                long sec_diff = curr->time.tv_sec - last_sec;
                if (sec_diff > 1) continue;
                if (curr->time.tv_usec - last_usec + (sec_diff * 1000000)  > KEY_TIMEOUT_CONFIG)
                    continue;
#endif
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
    if (argc < 3) {
        perror("\
Error: Missing Arguments\n\
Consult `evdoublebind-make-config -h` for config details\n\
Usage: \n\
  evdoublebind <Keyboard> <Mapping>");
        exit(1);
    };
    nice(-20);
    int found = parse_keymap(argv[2]);
    map_events(argv[1]);
}
