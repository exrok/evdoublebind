#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>

#include <linux/input.h>

// KEY_TIMEOUT_CONFIG: Number of microseconds before a keypress will not insert
//   a tap even if press aloned. Usefull when used in graphical applications
//   with a mouse, setting it too 0 disables it.
#define KEY_TIMEOUT_CONFIG 200000


typedef struct KeyBind {
    __u16 key;
    __u16 tap_key;
} KeyBind;

static KeyBind KEYMAP[64] = {};
static int  KEYMAP_LEN = 0;
// Evdoublebind Code.

// Parses something like '42:189,54:190,56:191,57:192,39:193,58:194'
// null terminated.
int parse_keymap(const char * mapstr) {
    const char *c = mapstr;
    int len = 0;
    int val = 0;
    while (1) {
        switch (*c) {
            case '\0': //fallthrough
            case ',': //fallthrough
            case ':':
                if (len & 1) KEYMAP[len >> 1].tap_key = val;
                else         KEYMAP[len >> 1].key = val;
                val = 0;
                len++;
                if (*c == '\0') {
                    KEYMAP_LEN = len >> 1;
                    return KEYMAP_LEN;
                }
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
                tap_key(index->tap_key, fd);

            }
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        perror("Error: Missing command line argument: config\n");
        perror("Usage Example: \n  ");
        perror(argv[0]);
        perror(" evdoublebind.in\n");
        exit(1);
    };
    int found = parse_keymap(argv[2]);
    int fd = open(argv[1],O_RDWR);
    if (fd < 0) {
        exit(0);
    }
    nice(-20);
    unsigned int key_repeat[2] = {2048,2048};
    if (ioctl(fd, EVIOCSREP, &key_repeat) == -1)
        perror("Warning: Key repeat could not be disabled");
    map_events(fd);
    return 0;
}
