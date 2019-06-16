#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>

#include <linux/input.h>

typedef struct KeyBind {
    __u16 key;
    __u16 tap_key;
    __u16 tap_modifier;
} KeyBind;

// KEY_TIMEOUT_CONFIG: Number of microseconds before a keypress will not insert
//   a tap even if press aloned. Usefull when used in graphical applications
//   with a mouse, setting it too 0 disables it.
#define KEY_TIMEOUT_CONFIG 200000

////  My personal setup; requires external configuration to work in XKBLayout to remap
////  normal keys to modifier keys. https://i64.dev/evdoublebind-introduction/
//const KeyBind KEYMAP[] = {
//   {.key = KEY_LEFTSHIFT, .tap_key = KEY_BACKSLASH},
//   {.key = KEY_RIGHTSHIFT,.tap_key = KEY_7, .tap_modifier = KEY_LEFTSHIFT},
//   {.key = KEY_SPACE,     .tap_key = KEY_F13}, // External Remapped SPACE->Hyper, KEY_F13->SPACE
//   {.key = KEY_CAPSLOCK,  .tap_key = KEY_ESC},
//   {.key = KEY_SEMICOLON, .tap_key = KEY_F14}, // External Remapped SPACE->Hyper, KEY_F14->Semicolon
//   {.key = KEY_LEFTALT,   .tap_key = KEY_6, .tap_modifier = KEY_LEFTSHIFT}
//};

// Basic setup makes capslock insert ESC when tapped you still need to
// remap CAPSLOCK to a modifier separately
// const KeyBind KEYMAP[] = {
//      {.key = KEY_CAPSLOCK,  .tap_key = KEY_ESC},
// };

// No external remapping required, basic config.
const KeyBind KEYMAP[] = {
     {.key = KEY_LEFTSHIFT, .tap_key = KEY_BACKSLASH},
};

// Evdoublebind Code.

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
#if KEY_TIMEOUT_CONFIG != 0
    time_t last_sec = 0;
    suseconds_t last_usec = 0;
#endif
    while(1) {
        int rd = read(fd, buf, sizeof(buf));
        if (rd == -1) {
            perror("Reading Keyboard Input");
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
    if (argc < 2) {
        perror("Error: Missing command line argument: keyboard dev node\n");
        perror("Usage Example: \n  ");
        perror(argv[0]);
        perror(" /dev/input/by-path/platform-i8042-serio-0-event-kbd\n");
        exit(1);
    };
    int fd = open(argv[1], O_RDWR);
    if (fd < 0) {
        perror("Could not open file do you have the correct permission.");
        exit(1);
    }
    unsigned int key_repeat[2] = {2048,2048};
    if (ioctl(fd, EVIOCSREP, &key_repeat) == -1)
        perror("Warning: Key repeat could not be disabled");
    map_events(fd);
    return 0;
}
