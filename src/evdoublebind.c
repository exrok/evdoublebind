#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>

#include <linux/input.h>

// KEY_TIMEOUT_CONFIG: Number of microseconds before a keypress will not insert
//   a tap even if press aloned. Usefull when used in graphical applications
//   with a mouse, setting it too 0 disables it.
#define KEY_TIMEOUT_CONFIG 200000


typedef struct QuitKeyState {
    __u16 key;
    __u16 down;
} QuitKeyState;

typedef struct KeyBind {
    __u16 key;
    __u16 tap_key;
} KeyBind;

static QuitKeyState QUITMAP[8] = {};
static int QUITMAP_LEN = 0;
void update_quitmap_down(__u16 key) {
    int all_down = 1;
    for(int i = 0; i <QUITMAP_LEN; i++) {
        if (QUITMAP[i].key == key) QUITMAP[i].down = 1;
        if (!QUITMAP[i].down) all_down = 0;
    }
    if (all_down) {
        exit(0);
    }
}

void update_quitmap_up(__u16 key) {
    int all_down = 1;
    for(int i = 0; i <QUITMAP_LEN; i++) {
        if (QUITMAP[i].key == key) QUITMAP[i].down = 1;
        if (!QUITMAP[i].down) all_down = 0;
    }
    if (all_down) {
        exit(0);
    }
}

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
char * file_contents( const char *path) {
    int fd = open(path, O_RDONLY);
    if (fd == -1) return NULL;
    int offset = lseek(fd, 0, SEEK_END);
    if (offset < 1) {
        close(fd);
        return NULL;
    }
    lseek(fd, 0, SEEK_SET);
    char * buffer = (char*) malloc(offset + 1);
    read(fd, buffer, offset);
    buffer[offset] = '\0';
    close(fd);
    return buffer;
}

int parse_config(char* conf) {
    char * config = file_contents(conf); //we leek this never cleaning it up
    if (!config){
        perror("Failed to access config\n");
        exit(1);
    }
    char *c = config;
    while (*c && *c != '\n') c++;
    if (*c == '\0') {
        exit(1);
//        printf("NO FILES FOUND IN CONFIG");
    }
    *c = '\0';
    int found = parse_keymap(config);
    //   printf("%d Mappings Found\n", found);
    int done = 0;
    while (!done) {
        config = c + 1;
        c++;
        while (*c && *c != '\n') c++;
        if (*c == '\0') done = 1;
        *c = '\0';
        if (access(config, R_OK|W_OK) != -1){
            //       printf("Acessable %s\n", config);
            if (fork() == 0) {
                int fd = open(config, O_RDWR);
                //   printf("child_opening: %s\n", config);
//                free(config);
                // printf("hellp\n");
                return fd;
            }
        }
    }
    return -1;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        perror("Error: Missing command line argument: config\n");
        perror("Usage Example: \n  ");
        perror(argv[0]);
        perror(" evdoublebind.in\n");
        exit(1);
    };
    int fd = parse_config(argv[1]);
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
