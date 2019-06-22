#ifndef __INPUT_EVENT_NAMES_H_
#define __INPUT_EVENT_NAMES_H_
#include <linux/input.h>
#include <linux/uinput.h>
#include <stdio.h>

const char * LOOKUP_EV_NAME[] = {"EV_SYN",
                                 "EV_KEY",
                                 "EV_REL",
                                 "EV_ABS",
                                 "EV_MSC",
                                 "EV_SW"};

const char * LOOKUP_SYN_NAME[] = {"SYN_REPORT",
                                 "SYN_CONFIG",
                                 "SYN_MT_REPORT",
                                 "SYN_DROPPED",
                                 };


const char * UNKNOWN_EV = "EV_UNKNOWN";
const char * UNKNOWN_KEY = "KEY_UNKNOWN";

const char * LOOKUP_KEY_NAME[] = {"KEY_RESERVED",
"KEY_ESC",
"KEY_1",
"KEY_2",
"KEY_3",
"KEY_4",
"KEY_5",
"KEY_6",
"KEY_7",
"KEY_8",
"KEY_9",
"KEY_0",
"KEY_MINUS",
"KEY_EQUAL",
"KEY_BACKSPACE",
"KEY_TAB",
"KEY_Q",
"KEY_W",
"KEY_E",
"KEY_R",
"KEY_T",
"KEY_Y",
"KEY_U",
"KEY_I",
"KEY_O",
"KEY_P",
"KEY_LEFTBRACE",
"KEY_RIGHTBRACE",
"KEY_ENTER",
"KEY_LEFTCTRL",
"KEY_A",
"KEY_S",
"KEY_D",
"KEY_F",
"KEY_G",
"KEY_H",
"KEY_J",
"KEY_K",
"KEY_L",
"KEY_SEMICOLON",
"KEY_APOSTROPHE",
"KEY_GRAVE",
"KEY_LEFTSHIFT",
"KEY_BACKSLASH",
"KEY_Z",
"KEY_X",
"KEY_C",
"KEY_V",
"KEY_B",
"KEY_N",
"KEY_M",
"KEY_COMMA",
"KEY_DOT",
"KEY_SLASH",
"KEY_RIGHTSHIFT",
"KEY_KPASTERISK",
"KEY_LEFTALT",
"KEY_SPACE",
"KEY_CAPSLOCK",
"KEY_F1",
"KEY_F2",
"KEY_F3",
"KEY_F4",
"KEY_F5",
"KEY_F6",
"KEY_F7",
"KEY_F8",
"KEY_F9",
"KEY_F10",
"KEY_NUMLOCK",
"KEY_SCROLLLOCK",
"KEY_KP7",
"KEY_KP8",
"KEY_KP9",
"KEY_KPMINUS",
"KEY_KP4",
"KEY_KP5",
"KEY_KP6",
"KEY_KPPLUS",
"KEY_KP1",
"KEY_KP2",
"KEY_KP3",
"KEY_KP0",
"KEY_KPDOT",
"KEY_NULL84",
"KEY_ZENKAKUHANKAKU",
"KEY_102ND",
"KEY_F11",
"KEY_F12",
"KEY_RO",
"KEY_KATAKANA",
"KEY_HIRAGANA",
"KEY_HENKAN",
"KEY_KATAKANAHIRAGANA",
"KEY_MUHENKAN",
"KEY_KPJPCOMMA",
"KEY_KPENTER",
"KEY_RIGHTCTRL",
"KEY_KPSLASH",
"KEY_SYSRQ",
"KEY_RIGHTALT",
"KEY_LINEFEED",
"KEY_HOME",
"KEY_UP",
"KEY_PAGEUP",
"KEY_LEFT",
"KEY_RIGHT",
"KEY_END",
"KEY_DOWN",
"KEY_PAGEDOWN",
"KEY_INSERT",
"KEY_DELETE",
"KEY_MACRO",
"KEY_MUTE",
"KEY_VOLUMEDOWN",
"KEY_VOLUMEUP",
"KEY_POWER",
"KEY_KPEQUAL",
"KEY_KPPLUSMINUS",
"KEY_PAUSE",
"KEY_SCALE",
"KEY_KPCOMMA",
"KEY_HANGEUL",
"KEY_HANJA",
"KEY_YEN",
"KEY_LEFTMETA",
"KEY_RIGHTMETA",
"KEY_COMPOSE",
"KEY_STOP",
"KEY_AGAIN",
"KEY_PROPS",
"KEY_UNDO",
"KEY_FRONT",
"KEY_COPY",
"KEY_OPEN",
"KEY_PASTE",
"KEY_FIND",
"KEY_CUT",
"KEY_HELP",
"KEY_MENU",
"KEY_CALC",
"KEY_SETUP",
"KEY_SLEEP",
"KEY_WAKEUP",
"KEY_FILE",
"KEY_SENDFILE",
"KEY_DELETEFILE",
"KEY_XFER",
"KEY_PROG1",
"KEY_PROG2",
"KEY_WWW",
"KEY_MSDOS",
"KEY_COFFEE",
"KEY_ROTATE_DISPLAY",
"KEY_CYCLEWINDOWS",
"KEY_MAIL",
"KEY_BOOKMARKS",
"KEY_COMPUTER",
"KEY_BACK",
"KEY_FORWARD",
"KEY_CLOSECD",
"KEY_EJECTCD",
"KEY_EJECTCLOSECD",
"KEY_NEXTSONG",
"KEY_PLAYPAUSE",
"KEY_PREVIOUSSONG",
"KEY_STOPCD",
"KEY_RECORD",
"KEY_REWIND",
"KEY_PHONE",
"KEY_ISO",
"KEY_CONFIG",
"KEY_HOMEPAGE",
"KEY_REFRESH",
"KEY_EXIT",
"KEY_MOVE",
"KEY_EDIT",
"KEY_SCROLLUP",
"KEY_SCROLLDOWN",
"KEY_KPLEFTPAREN",
"KEY_KPRIGHTPAREN",
"KEY_NEW",
"KEY_REDO",
"KEY_F13",
"KEY_F14",
"KEY_F15",
"KEY_F16",
"KEY_F17",
"KEY_F18",
"KEY_F19",
"KEY_F20",
"KEY_F21",
"KEY_F22",
"KEY_F23",
"KEY_F24",
"KEY_NULL196",
"KEY_NULL197",
"KEY_NULL198",
"KEY_NULL199",
"KEY_PLAYCD",
"KEY_PAUSECD",
"KEY_PROG3",
"KEY_PROG4",
"KEY_DASHBOARD",
"KEY_SUSPEND",
"KEY_CLOSE",
"KEY_PLAY",
"KEY_FASTFORWARD",
"KEY_BASSBOOST",
"KEY_PRINT",
"KEY_HP",
"KEY_CAMERA",
"KEY_SOUND",
"KEY_QUESTION",
"KEY_EMAIL",
"KEY_CHAT",
"KEY_SEARCH",
"KEY_CONNECT",
"KEY_FINANCE",
"KEY_SPORT",
"KEY_SHOP",
"KEY_ALTERASE",
"KEY_CANCEL",
"KEY_BRIGHTNESSDOWN",
"KEY_BRIGHTNESSUP",
"KEY_MEDIA",
"KEY_SWITCHVIDEOMODE",
"KEY_KBDILLUMTOGGLE",
"KEY_KBDILLUMDOWN",
"KEY_KBDILLUMUP",
"KEY_SEND",
"KEY_REPLY",
"KEY_FORWARDMAIL",
"KEY_SAVE",
"KEY_DOCUMENTS",
"KEY_BATTERY",
"KEY_BLUETOOTH",
"KEY_WLAN",
"KEY_UWB",
"KEY_UNKNOWN",
"KEY_VIDEO_NEXT",
"KEY_VIDEO_PREV",
"KEY_BRIGHTNESS_CYCLE",
"KEY_BRIGHTNESS_AUTO",
"KEY_DISPLAY_OFF",
"KEY_WWAN",
"KEY_RFKILL",
"KEY_MICMUTE"};

const char * LOOKUP_MSC_NAME[]= {
"MSC_SERIAL"	,
"MSC_PULSELED"	,
"MSC_GESTURE"	,
"MSC_RAW"		,

"MSC_SCAN","MSC_TIMESTAMP","MSC_MAX"};
const char * key_code_name(u_int16_t code){
    if (code < sizeof(LOOKUP_KEY_NAME)/sizeof(char *)){
        return LOOKUP_KEY_NAME[code];
    }else{
        return UNKNOWN_KEY;
    }
}

const char * key_state_name(int state){
    if (state == 0) return "UP";
    if (state == 1) return "DOWN";
    if (state == 2) return "REPEAT";
    return "UNKOWN";
}

const char * msc_code_name(u_int16_t code){
    if (code < sizeof(LOOKUP_MSC_NAME)/sizeof(char *)){
        return LOOKUP_MSC_NAME[code];
    }else{
        return "UNKNOWN_MSC";
    }
}
const char * ev_code_name(u_int16_t code){
    if (code < sizeof(LOOKUP_EV_NAME)/sizeof(char *)){
        return LOOKUP_EV_NAME[code];
    }else{
        return UNKNOWN_EV;
    }
}

const char * syn_code_name(u_int16_t code){
    if (code < sizeof(LOOKUP_SYN_NAME)/sizeof(char *)){
        return LOOKUP_SYN_NAME[code];
    }else{
        return "UNKNOWN_SYN";
    }
}
void evout(const input_event &e){
   if (e.type == EV_KEY){
       fprintf(stdout,"EV_KEY(%s:%d, %s:%d)\n",
              key_code_name(e.code), e.code,
              key_state_name(e.value), e.value);
   }else if (e.type == EV_MSC){
       fprintf(stdout,"EV_MSC(%s:%d, %d)\n",
              msc_code_name(e.code), e.code,
              e.value);
   }else if (e.type == EV_SYN){
       fprintf(stdout,"EV_SYN~%s:%d\n",
              syn_code_name(e.code),e.value);
   }
   else{
       fprintf(stderr,"%s\n", ev_code_name(e.type));
   }
}

#endif // __INPUT_EVENT_NAMES_H_
