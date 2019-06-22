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

struct ModifierMapping {
    const char * sym;
    const char * map_name;
};

struct Settings {
    struct xkb_rule_names xkb_names = {
    .rules = NULL,
    .model = NULL,
    .layout = NULL,
    .variant = NULL,
    .options = NULL,
    };
    bool only_arguments = false;
    const char * input_conf = NULL;
    const char * xkb_conf_output = NULL;
    const char * evdoublebind_conf_output = NULL;
};
ModifierMapping ModifierTable[] = {
{"Meta_L", "Mod1"},
{"Meta_R", "Mod1"},
{"Alt_L", "Mod1"},
{"Alt_R", "Mod1"},
{"Shift_L", "Shift"},
{"Shift_R", "Shift"},
{"Super_L", "Mod4"},
{"Super_R", "Mod4"},
{"Caps_Lock", "Lock"},
{"Control_L", "Control"},
{"Control_R", "Control"},
{"Hyper_L", "Mod3"},
{"Hyper_R", "Mod3"},
//{"ISO_Level3_Shift", "Mod5"},
};

template<class T>
struct Array {
    T *buf;
    int cap = 8;
    int size = 0;
    Array() {
        buf = (T *)malloc(cap * sizeof(T));
    }
    ~Array() {
        free(buf);
    }
    void ensure_free_space() {
        if (size >= cap) {
            cap *= 2;
            buf = (T *)realloc(buf,cap * sizeof(T));
        }
    }
    T *top_new() {
        ensure_free_space();
        return buf + size++;
    }
    bool push(T value) {
        ensure_free_space();
        buf[size++] = value;
        return true;
    }
    T* begin() {
        return buf;
    }
    T* end() {
        return buf+size;
    }
};

struct KeyMapping {
    int keycode;
    char * hold_map[4];
    char * tap_map[4];
};

int parse_symlevel(char *sym[4], char * sym_str) {
    int count = 0;
    bool hasvalue = false;
    if (sym_str != NULL) {
        char *curr = sym_str;
        while (*curr) {
            if (*curr == ' ' || *curr == '\t') {
                *curr = '\0';
                if (hasvalue) {
                    sym[count++] = sym_str;
                    hasvalue = false;
                    if (count == 4) return 4;
                }
            } else {
                if (!hasvalue) {
                    sym_str = curr;
                    hasvalue = true;
                }
            }
            curr++;
        }
    }
    if (hasvalue) {
        sym[count++] = sym_str;
        hasvalue = false;
        if (count == 4) return 4;
    }
    for (int i = count; i < 4; i++ ) sym[i] =NULL;
    return count;
}

int parse_keycode(xkb_keymap *keymap, char * keycode) {
    char * start = strchr(keycode, '<');
    if (start == NULL) {
        return atoi(keycode);
    } else {
        start += 1;
        char * end = strchr(start, '>');
        if (end != NULL) {
            *end = '\0';
            int x = xkb_keymap_key_by_name(keymap,start);
            if (x > 7) {
                *end = '>';
                return x - 8;
            } else {
                fprintf(stderr,"CONFIG ERROR: unknown key: <%s>\n", start);
                exit(1);
            }
        }
    }
    return -1;
}

int match_keyword(const char * keycode) {
    while (*keycode) {
        if (*keycode != ' ' || *keycode != '\t') break;
        keycode++;
    }
    if (strncmp(keycode, "unused", 6) == 0) return 1;
    if (strncmp(keycode, "kbd", 3) == 0) return 2;
    return 0;
}

struct Config {
    Array<KeyMapping> mappings;
    Array<char *> keyboards;
    Array<xkb_keycode_t> unused_keys;
};

char * parse_unused_keys(xkb_keymap *keymap, Config &config, char *buf) {
    char * last = NULL;
    while (*buf) {
        switch (*buf) {
            case '\n': return buf;
            case '<': last = buf + 1; break;
            case '>':
                *buf = '\0';
                int x = xkb_keymap_key_by_name(keymap,last);
                if (x > 7) {
                    config.unused_keys.push(x);
                } else {
                    fprintf(stderr,"CONFIG ERROR: unknown unused key: <%s>\n", last);
                    exit(1);
                }
                break;
        }
        buf++;
    }
    return buf;
}

void generate_modifier_mapping(FILE * file, const char *keyname, char *syms[4]) {
    for(int i = 0; i < 4; i++) {
        if (syms[i] == NULL) return;
        for(auto &mod : ModifierTable) {
            if (strcmp(mod.sym, syms[i]) == 0) {
                fprintf(file, "   modifier_map %s { <%s> };\n", mod.map_name, keyname);
            }
        }
    }
}

void print_keysyms(FILE * file, char *syms[4]) {
    char *last = NULL;
    if (syms[0] == NULL) return;
    for(int i = 0; i < 4; i++) {
        if (syms[i] != NULL) last = syms[i];
        if (i != 0) {
            fprintf(file, ", ");
        }
        fprintf(file,"%s", last);
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
char * skip_ws(char * c) {
    while (*c && (*c == '\t' || *c == ' ')) c++;
    return c;
}

// Very simple parser for config in with syntax "key | value # comment"
// modifying by config text, inserting null terminations, returns
// key value pairs from config in declaration order. Trims white space
// around key and value.
struct ConfigParser {
    char * c;
    char * key = NULL;   //Current key
    char * value = NULL; //Current value
    int line = -1;       //Line of current key/value pair
    bool end = false;

    ConfigParser(char * c): c{c}{}
    bool next() {
        key = value = NULL;
        while (!end) {
            char *last; //last character in ident
            char *ident; //current token either value or key depending on state
            int state = 0;
            ident = last = c = skip_ws(c);
            while (1) {
                switch (*c) {
                    case ':':
                        if (state == 0) {
                            *(last+1) = '\0';
                            state = 1;
                            key = ident;
                            ident = c = skip_ws(c+1);
                            continue;
                        } else {
                            fprintf(stderr,"ERROR: unexpected ':' on line %d\n'", line + 1);
                            exit(1);
                        }
                        break;
                    case ' ': //Skip White Space
                    case '\t':
                        last = c-1;
                        c = skip_ws(c + 1);
                        continue;
                    case '#': //Skip Comment
                        c++;
                        while (*c && *c != '\n' ) c++; //Fallthougth
                    case '\n':
                    case '\0':
                        goto end_line;
                }
                last = c++;
            }
        end_line:
            line++;
            end = (*c == '\0');
            if (state == 1 && last != ident) {
                *(last+1) = '\0';
                value = ident;
                c++;
                return true;
            }
            c++;
        }
        return false;
    }
};

void generate_xkb_file(xkb_keymap *keymap, Config &config, Settings &settings) {
    FILE * xkbfile;
    if (settings.xkb_conf_output == NULL) {
        char buf[1024];
        system("mkdir -p $HOME/.xkb/symbols/");
        int len = snprintf(buf,1024, "%s/.xkb/symbols/evdoublebind", getenv("HOME"));
        if (len >= 1023) {
            fprintf(stderr,"WOW, your home path is really long, CRITICAL ERROR");
            fprintf(stderr,"You should specify the xkb conf output manually");
            exit(1);
        }
        fprintf(stderr,"Outputing XKB option to: %s\n", buf);
        xkbfile = fopen(buf, "w");
    } else {
        fprintf(stderr,"Outputing XKB option to: %s\n", settings.xkb_conf_output);
        xkbfile = fopen(settings.xkb_conf_output, "w");
    }
    if (xkbfile == NULL) {
        perror("Failed Creating xkb config file: ");
        exit(1);
    }

    auto unused = config.unused_keys.begin();

     fprintf(xkbfile,"partial modifier_keys\n");
     fprintf(xkbfile,"xkb_symbols \"mapping\" {\n");
     for (auto & mapping: config.mappings) {
         const char * keyname = xkb_keymap_key_get_name(keymap, mapping.keycode + 8);
         fprintf(xkbfile,"   replace key <%s> { [ ",  keyname);
         print_keysyms(xkbfile,mapping.hold_map);
         fprintf(xkbfile," ] };\n");
         if (mapping.tap_map[0] != NULL) {
             if (unused == config.unused_keys.end()) {
                 fprintf(stderr,"ERROR: not enough unused keys specified for config\n");
                 exit(1);
             }
             const char * tapkeyname = xkb_keymap_key_get_name(keymap, *unused);
             fprintf(xkbfile,"   replace key <%s> { [ ",  tapkeyname);
             print_keysyms(xkbfile,mapping.tap_map);
             fprintf(xkbfile," ] };\n");
             unused++;
         }
         generate_modifier_mapping(xkbfile, keyname, mapping.hold_map);
         fprintf(xkbfile,"\n");
     }
     fprintf(xkbfile,"};\n");
     fclose(xkbfile);
}

void generate_hfile(Config &config, Settings &settings) {
     FILE * hfile;
     if (settings.evdoublebind_conf_output == NULL) {
         hfile = stdout;
     } else {
         hfile = fopen(settings.evdoublebind_conf_output, "w");
     }
     if (hfile == NULL) {
         perror("Failed Creating xkb config file: ");
     }
     for (auto &kbd : config.keyboards){
         fprintf(hfile,"%s ", kbd);
         auto unused = config.unused_keys.begin();
         for (auto & mapping: config.mappings) {
             if (mapping.tap_map[0] != NULL) {
                 if (unused == config.unused_keys.end()) {
                     fprintf(stderr,"ERROR: not enough unused keys specified for config\n");
                     exit(1);
                 }
                 if (unused != config.unused_keys.begin()) {
                     fprintf(hfile,",");
                 }
                 fprintf(hfile,"%d:%d", mapping.keycode, *unused - 8);
                 unused++;
             }
         }
         fprintf(hfile,"\n");
     }
     if (settings.evdoublebind_conf_output != NULL) {
         fclose(hfile); //don't close stdout
     } else {
         fflush(stdout);
     }
}


int gen(Settings &settings) {
    char * buffer = file_contents(settings.input_conf);
    if (!buffer) {
        fprintf(stderr,"Error Reading Config File [%s]: ", settings.input_conf);
        perror("");
        exit(1);
    }
    Config config;

    struct xkb_keymap *keymap;
    struct xkb_context *ctx;

    ctx = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
    if (!ctx) {
        exit(1);
    }
    keymap = xkb_keymap_new_from_names(ctx, &settings.xkb_names,
                                       XKB_KEYMAP_COMPILE_NO_FLAGS);
    if (!keymap) {
        exit(1);
    }
    struct xkb_state *state;
    state = xkb_state_new(keymap);

    if (!state) {
        exit(1);
    }

    auto entry = ConfigParser(buffer);
    while (entry.next()) {
        if (strcmp(entry.key, "unused") == 0)  {
            parse_unused_keys(keymap, config, entry.value);
        } else if (strcmp(entry.key, "kbd") == 0) {
            config.keyboards.push(entry.value);
        } else {
            char *tap = strchr(entry.value, '|');
            if (tap != NULL){
                *tap = '\0';
                tap++;
                auto map = config.mappings.top_new();
                map->keycode = parse_keycode(keymap, entry.key);

                parse_symlevel(map->hold_map, entry.value);
                parse_symlevel(map->tap_map, tap);
            }
        }
    }
    generate_hfile(config,settings);
    if (!settings.only_arguments) {
        generate_xkb_file(keymap,config, settings);
    }
    free(buffer);

    xkb_state_unref(state);
    xkb_keymap_unref(keymap);
    xkb_context_unref(ctx);
    return 0;
}


void print_usage(){
    static const char * DOCS_STRING =
        "evdoublebind-make-config " "0.1"
R"XXX(

Usage:
   evdoublebind-make-config [options] <input_config_path>

<input_config_path>: Path to base config, example below
---
unused : <FK19> <FK20> <FK21> <FK22> <FK23> <FK24> <I159> <I160>
kbd : /dev/input/by-id/usb-Logitech_Logitech_G710_Keyboard-event-kbd
<CAPS> : Hyper_L   | Escape             #Tap Caps -> Escape
                                        #Hold Caps -> Hyper
<LFSH> : Shift_L   | backslash          #Tap Left Shift -> \
<RTSH> : Shift_R   | ampersand          #Tap Right Shift -> &
<LALT> : Alt_L     | asciicirum         #Tap Left alt -> ^
<SPCE> : Control_L | space              #Hold Space -> control
39     : Super_L   | semicolon colon    #Hold Semicolon -> Super
---

Options:
   -a                                    Only Generate Argument File
   (default: STDOUT)
   -c <evdoublebind_argument_file>       File path
   (default: STDOUT)
   -x <xkb_conf_output>
   (default: '~/.xkb/symbols/evdoublebind')
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
    while ((c = getopt (argc, argv, "ahr:m:l:v:o:x:c:")) != -1)
        switch(c){
            case 'a':
                setting.only_arguments = true;
                continue;
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
            case 'x':
                setting.xkb_conf_output = optarg;
                continue;
            case 'c':
                setting.evdoublebind_conf_output = optarg;
                continue;
        }
    if (argc - optind < 1) {
        fprintf(stderr, "No config specfied\n");
        print_usage();
        exit(1);
    }
    setting.input_conf = argv[optind];
    return setting;
}

int main(int argc, char *argv[]) {
    Settings settings = parse_arguments(Settings(), argc, argv);
    gen(settings);
    return 0;
}
