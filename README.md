Evdoublebind, via evdev, provides double bind keys: keys which are overloaded with functional acting as a modifier when held but another key when tapped alone. Although other applications strive for the same core functionality, Evdoublebind is unique in that it...
* Runs at a low level interfacing evdev directly so that it is display server
agnostic even working in a TTY.
* Is very simple and small, written in less than 100 lines of C code which when
  compiled statically with musl and stripped produces a ~5kb binary on linux. 
* Introduces as little latency as possible, allowing regular key events to
  processed by system directly without redirection or a virtual input device.

Evdoublebind has gone through a major refactor to be easier setup the old simpler version can be found on it own [branch](https://github.com/exrok/evdoublebind/tree/tiny-over-optimized-version); 

## Further information
- [Evdoublebind and My Ergonomic Key Bindings](https://i64.dev/evdoublebind-introduction/) June 10, 2019

A step by step guide for setup on wayland and X11 is comming soon. 

## Usage

### evdoublebind-inspector
Outputs keyboards events from `/dev/input` to aid with configuration of 
evdoublebind.
```sh
> sudo evdoublebind-inspector 
Found 2 Possible Keyboards.
---
/dev/input/by-id/usb-Logitech_Logitech_G710_Keyboard-event-kbd
/dev/input/by-id/usb-Logitech_Logitech_G710_Keyboard-if01-event-kbd
---
[/dev/input/by-id/usb-Logitech_Logitech_G710_Keyboard-event-kbd]{
   EVDEV: keycode:28 name:KEY_ENTER;
   XKB: key[36]:<RTRN> keysym:Return;
}
[/dev/input/by-id/usb-Logitech_Logitech_G710_Keyboard-event-kbd]{
   EVDEV: keycode:31 name:KEY_S;
   XKB: key[39]:<AC02> keysym:s;
}
```

### evdoublebind-make-config
Generates input arguments for evdoublebind from a config that specify keys
interms of XKB keyname and keysyms. Can be configured for different layouts
see `evdoublebind-make-config -h`.

Example config,
```conf
unused : <FK19> <FK20> <FK21> <FK22> <FK23> <FK24> <I159> <I160>
kbd : /dev/input/by-id/usb-Logitech_Logitech_G710_Keyboard-event-kbd
kbd : /dev/input/by-id/some-other-keyboard
<CAPS> : Hyper_L   | Escape             #Tap Caps -> Escape
                                        #Hold Caps -> Hyper
<LFSH> : Shift_L   | backslash          #Tap Left Shift -> \
<RTSH> : Shift_R   | ampersand          #Tap Right Shift -> &
<LALT> : Alt_L     | asciicirum         #Tap Left alt -> ^
<SPCE> : Control_L | space              #Hold Space -> control
39     : Super_L   | semicolon colon    #Hold Semicolon -> Super
```

### evdoublebind
The main program, evdoublebind takes two arguments the path of a input dev node,
ex. `/dev/input/by-id/usb-Logitech_Logitech_G710_Keyboard-event-kbd` and a
keycode mapping, ex. `58:189,42:190,54:191`, which is a comma seperated list of
mappings base key on the left and tap key on the right.
```sh
evdoublebind "/dev/input/by-id/usb-Logitech_Logitech_G710_Keyboard-event-kbd" "58:189,42:190,54:191"
```
or
```sh
evdoublebind-make-config basic.conf | while read args; do
  sudo ../build/evdoublebind $args &
done
```

## Installation

  Todo...

## Todo
  
  Todo...
