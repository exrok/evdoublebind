#!/bin/sh

#install XKB rule set: evdev-doublebind
# ./install_xkb_rule.sh
#

echo 'unused : <FK19> <FK20> <FK21> <FK22> <FK23> <FK24>' > basic.conf

# find keyboards, THIS MAY FIND TO MANY KEYBOARDS!!! for real setup please
# run `evdoublebind-inspector` to identify your actual keyboards
echo "sudo evdoublebind-inspector -k : to get keyboards"
sudo evdoublebind-inspector -k | awk '{print "kbd:" $1;}' >> basic.conf
echo '<CAPS> : Control_L   | Escape' >> basic.conf

# Generate XKB_option will go in `~/.xkb/symbols/evdoublebind` and `evdb.in`.
evdoublebind-make-config -c evdb.in basic.conf || exit #abort on failure

#start evdouble-bind
killall evdoublebind

#Start evdouble-bind
evdoublebind evdb.in || exit

#xkb to use the generated options
setxkbmap -I$HOME/.xkb -rules 'evdev-doublebind' -option evdoublebind:mapping -print \
    | xkbcomp -w 2 -I$HOME/.xkb - $DISPLAY
echo "it is 'normal' for xkbcomp to output some errors."
