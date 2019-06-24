Evdoublebind, via evdev, provides double bind keys: keys which are overloaded with functionality acting as a modifier when held but another key when tapped alone. Although other applications strive for the same core functionality, Evdoublebind is unique in that it...
* Runs at a low level interfacing evdev directly so that it is display server
agnostic even working in a TTY.
* The main program is very simple and small, written in less than 200 lines of C code which when
  compiled statically with musl and stripped produces a ~5kb binary on linux. 
* Introduces as little latency as possible, allowing regular key events to
  processed by system directly without redirection or a virtual input device.

Evdoublebind has gone through a major refactor to be easier setup the old simpler version can be found on it own [branch](https://github.com/exrok/evdoublebind/tree/tiny-over-optimized-version); 

## Further information
- [Making Low Level Keyboard Hacks Easy to Use](https://i64.dev/low-level-keyboards-hacks-easy-to-use/) June 24, 2019
- [Evdoublebind and My Ergonomic Key Bindings](https://i64.dev/evdoublebind-introduction/) June 10, 2019

## Installation
### Compiling from Source

Install dependencies:

* libxkbcommon (for evdoublebind-inspect/-make-config)
* linux-headers (for <linux/input.h>)
* musl (optional: for smaller/faster binary)

Run these commands:

```sh
make
#or make musl-static

sudo make install
```

To run install the XKB rule-set with the `evdoublebind:mapping` into the user xkb 
folder,`~/.xkb`, option run

```sh
~/install_xkb_rule.sh
```

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
### Simple Example Script
The following the following script will setup capslock as control
where tapping capslock insert escape. The script works on X11 envoirments where
setxkbmap is respected, I know it works on i3wm and sway.
```sh
#!/bin/sh
#First install the XKB rule set if havn't yet
#./install_xkb_rule.sh

 echo 'unused : <FK19> <FK20> <FK21> <FK22> <FK23> <FK24>' > basic.conf

# find keyboards, THIS MAY FIND TO MANY KEYBOARDS!!! for real setup please
# run `evdoublebind-inspector` to identify your actual keyboards
echo "sudo evdoublebind-inspector -k : to get keyboards"
sudo evdoublebind-inspector -k | awk '{print "kbd:" $1;}' >> basic.conf
echo '<CAPS> : Control_L   | Escape' >> basic.conf

# Generate XKB_option will go in `~/.xkb/symbols/evdoublebind` and `evdb.in`.
evdoublebind-make-config -c evdb.args basic.conf || exit #abort on failure

#make sure no other instances are running
killall evdoublebind

#Start evdouble-bind
cat evdb.args | while read args; do
    evdoublebind $args &
done

# Alternatively you could generate the arguments from the config
# and pass the directly
# evdoublebind-make-config basic.conf | while read args; do
#     sudo ../build/evdoublebind $args &
# done

#SET XKB ON X11
setxkbmap -I$HOME/.xkb -rules 'evdev-doublebind' -option evdoublebind:mapping\
 -print | xkbcomp -w 2 -I$HOME/.xkb - $DISPLAY
echo "it is 'normal' for xkbcomp to output some warnings."
# SET XKB ON SWAY, you'll probably want to specify your keyboard directly
# for actual use instead of the '*'.
#swaymsg input '*' xkb_rules evdev-doublebind 
#swaymsg input '*' xkb_options evdoublebind:mapping
```
