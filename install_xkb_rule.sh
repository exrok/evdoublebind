#!/bin/sh
mkdir -p $HOME/.xkb/rules
cp /usr/share/X11/xkb/rules/evdev $HOME/.xkb/rules/evdev-doublebind
cp /usr/share/X11/xkb/rules/evdev.xml $HOME/.xkb/rules/evdev-doublebind.xml
cp /usr/share/X11/xkb/rules/evdev.lst $HOME/.xkb/rules/evdev-doublebind.lst
sed -i 's/!\s*option\s*=\s*symbols/! option	=	symbols\n  evdoublebind:mapping = +evdoublebind(mapping)/g' $HOME/.xkb/rules/evdev-doublebind
