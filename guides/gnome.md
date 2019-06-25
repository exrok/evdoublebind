# Step By Step Guide For Advanced Usage of Evdoublebind for Gnome( Wayland or X11) [WIP]

Since gnome is more difficult to configure to due lack of support for custom
keybindings, the setup process is a bit more involved. System files need to
modified to allow the use of custom XKB keyboard options. As such, configuration
in the package via hooks are required to insure the modifications persist
across updates. This guide will focus on using Arch Linux and it's package
managements systems.

### Step 1: Install Evdoublebind

You can either install from cloning the git repo and building it, see more
details in the readme or install the AUR package, `evdoublebind`.

### Step 2: Identify Keyboard

Run `sudo evdoublebind-inspector` and type on the keyboard you wish to remap you
will see key events that look like this.
```
[/dev/input/by-path/platform-i8042-serio-0-event-kbd]{
   EVDEV: keycode:46 name:KEY_C;
   XKB: key[54]:<AB03> keysym:C;
}
```
The keyboard identify for the source of this event is `/dev/input/by-path/platform-i8042-serio-0-event-kbd`.

You should key `evdoublebind-inspector` open in a separate window during
configuration as it will continue to be helpful for identifying keys.

### Step 3: Start Writing Your Configuration

Create a configuration file for evdoublebind in your favourite text editor, for
this guide we will put the configuration file in `~/.config/evdoublebind.conf`.

Let's start off the config specifying our keyboard
```conf
kbd : insert-your-keyboard-path-here
```

### Step 4: Adding a Basic Direct Binding
Let's add a basic direct binding.

We are going to make the left shift key insert a back slash when tapped.

When `evdoublebind-inspector` open in a terminal, we press the left shift key
once and the back slash key once. We can see the events below,
```
[/dev/input/by-path/platform-i8042-serio-0-event-kbd]{
   EVDEV: keycode:42 name:KEY_LEFTSHIFT;
   XKB MODS: SHIFT;
   XKB: key[50]:<LFSH> keysym:Shift_L;
}
[/dev/input/by-path/platform-i8042-serio-0-event-kbd]{
   EVDEV: keycode:43 name:KEY_BACKSLASH;
   XKB: key[51]:<BKSL> keysym:backslash;
}
```
from the XKB line we identify the key names associated with the keys, left shift
is `<LFSH>`  and backslash is `<BKSL>`. Let's add it to our config,

```conf
kbd : insert-your-keyboard-path-here
<LFSH> : @ <BKSL>
```
on each line the part before the `:` is either a keyboard or keyname for base
key for a mapping. The "@" specifies that this is a direct binding, that is when
evdoublebind inserts the tap key it will insert the keycode for `<BKSL>`
directly, we will see some different options when we get to advanced bindings. 

### Step 5: Adding another Basic Direct Binding with a Tap Modifier
This time we are going to make the right shift key insert `Control+w` on tap.
Again using `evdoublebind-inspector` we will identify the key names by pressing the
relevant keys and reading the events. The important lines from the output this
time are,
```
...  XKB: key[62]:<RTSH> keysym:Shift_R; ...
...  XKB: key[37]:<LCTL> keysym:Control_L; ...
...  XKB: key[25]:<AD02> keysym:w; ...
```
You may notice the key name of `w` is `<AD02>` which isn't very readable, this
is one of the downsides of direct bindings.

Now lets add this to our setup.
```conf
kbd : /dev/input/by-path/platform-i8042-serio-0-event-kbd
<LFSH> : @ <BKSL>
<RTSH> : @ <AD02> <LCTL>
```

When using direct bindings you can specify two keys in the tap slot. If a
second key is specified it will be used as a modifier to tap the first. That is
evdoublebind will press the second key down, tap first key and then release the
second key. 

This example also illustrates another downside of directly bound keys: if you
are holding left control and tap the right shift key, the tap inserted by
evdoublebind will release left control even though it is still pressed by you.

### Step 6: Let's Try it Out

Since we are only using basic, directly bound keys, XKB settings do not need to
be modified yet.

if you run `evdoublebind-make-config -a ~/.config/evdoublebind.conf` now, it
should output something like,

```
/dev/input/by-path/platform-i8042-serio-0-event-kbd 42:43,54:17|29
```

These are command line arguments for `evdoublebind` that when passed will perform
the mapping.

Thus to start `evdoublebind` you run in the shell,
```sh
> evdoublebind $(evdoublebind-make-config -a ~/.config/evdoublebind.conf)
```

Now when tapping left shift you should get a backslash and when tapping right
shift get `Control+w`. The inserted events will also show up in
`evdoublebind-inspector` so you can see exactly whats being inserted. 

### Interlude: Advanced Use

If you want to more than just basic direct mapping, like transforming normal
keys into new modifiers and outputting keys not on your keyboard, you are going
to need a custom a XKB layout. But, there is good
news,`evdoublebind-make-config` 
can generate the XKB settings for you. Normally, once `evdoublebind-make-config`
generates the XKB option all you have to do is enable it, however gnomes doesn't
make it that simple, as mentioned in the introduction. Thus before we more
advanced bindings we are going to need to setup things so we can use generated options.

### Step 6: Setup XKB Rules Hook

Since we are going to be modifying system files to get Gnome to use the a
custom rule, we are going to add a hook so when pacman updates the changes
will be maintained.

First we will create a script file in
`/usr/share/libalpm/scripts/update-xkb-rule` with contents below.
```sh
#!/bin/sh
sed -i 's/!\s*option\s*=\s*symbols/! option	=	symbols\n  evdoublebind:mapping = +evdoublebind(mapping)/g' /usr/share/X11/xkb/rules/evdev
```
Make sure the script is executable `sudo chmod +x
/usr/share/libalpm/scripts/update-xkb-rule`. The script does a simple find and
then insertion to add the option `evdoublebind:mapping` to evdev rules file in XKB.

Next we need add the hook that will run that script when necessary by creating
the file `/usr/share/libalpm/hooks/evdoublebind.hook` with the contents below. 
```conf
[Trigger]
Type = File
Operation = Install
Operation = Upgrade
Target = usr/share/X11/xkb/rules/evdev

[Action]
Description = Adding Evdoublebind Rule to XKB
When = PostTransaction
Exec = /usr/share/libalpm/scripts/update-xkb-rule
NeedsTargets
```

### Step 7: Reinstall xkeyboard-config to trigger hook
On Arch the XKB directory is owned by the package `xkeyboard-config` so
reinstalling it will trigger our hook.

```sh
> sudo pacman -S xkeyboard-config
```

After installing you see the following output indicating the hook was run. 
```
:: Running post-transaction hooks...
(1/2) Adding Evdoublebind Rule to XKB
```

You can also run `cat /usr/share/X11/xkb/rules/evdev | grep evdoublebind` to
check if it was successful.

### Step 8: Advanced Direct Binding: Modifying Capslock 

A popular use for Evdoublebind is to make full use of the easy to reach Capslock
which by default is nearly useless. Usually by map Capslock to control but
tapping Capslock maps to escape which is useful in modal text editors.

Like before lets look at the output of `evdoublebind-inspector` when pressing
the relevant keys.
```
[/dev/input/by-id/usb-Razer_Razer_BlackWidow_Ultimate_2016-event-kbd]{
   EVDEV: keycode:29 name:KEY_LEFTCTRL;
   XKB: key[37]:<LCTL> keysym:Control_L;
}
[/dev/input/by-id/usb-Razer_Razer_BlackWidow_Ultimate_2016-event-kbd]{
   EVDEV: keycode:58 name:KEY_CAPSLOCK;
   XKB: key[66]:<CAPS> keysym:Caps_Lock;
}
[/dev/input/by-id/usb-Razer_Razer_BlackWidow_Ultimate_2016-event-kbd]{
   EVDEV: keycode:1 name:KEY_ESC;
   XKB: key[9]:<ESC> keysym:Escape;
}
```
The important parts above are the key names for escape and capslock and the
keysym for control. When working with XKB the keysym is important since that is
end representation of key after translation to your layout and language. 

Adding this to our config we get,

```conf
kbd : /dev/input/by-path/platform-i8042-serio-0-event-kbd
<LFSH> : @ <BKSL>
<RTSH> : @ <AD02> <LCTL>
<CAPS> : Control_L @ <ESC>
```

In the config, `<KEY_NAME> : Key_Syms ..` defines a XKB mapping for the
generated XKB file. Let's generate a XKB option file now.

Due to limitations in gnome the place of file is restricted and it must be
install into `/usr/share/X11/xkb/symbols` directory directly. You can do this by running,
```
> sudo evdoublebind-make-config -x /usr/share/X11/xkb/symbols/evdoublebind ~/.config/evdoublebind.conf 
```

Although you shouldn't need to modify the file yourself, lets have a look at
what it generates.
```
partial modifier_keys
xkb_symbols "mapping" {
   replace key <CAPS> { [ Control_L, Control_L, Control_L, Control_L ] };
   modifier_map Control { <CAPS> };
};
```
In this case, the generated file is pretty simple we see. 

### Step 9: Some Helper Scripts

When messing with your keybindings you may accidentally make bindings that make
you compute difficult to use. It is also important to make sure that
evdoublebind is used at the same time as the generated XKB option. So before we
run lets create two simple scripts to synchronize activation and deactivation. 

We will create an activation script, '~/scripts/evdb-start.sh' with the contents below, 
```sh
#!/bin/sh
#make sure no other instances are running
killall evdoublebind

#Build Config and Exit on failure
sudo evdoublebind-make-config -x /usr/share/X11/xkb/symbols/evdoublebind \
 ~/.config/evdoublebind.conf || exit 1

#Tell gnome to use our custom xkb options
#Change the 'us' to the proper layout
gsettings set org.gnome.desktop.input-sources sources "[('xkb', 'us')]" || exit 1
gsettings set org.gnome.desktop.input-sources xkb-options "['evdoublebind:mapping']"

evdoublebind $(evdoublebind-make-config -a ~/.config/evdoublebind.conf)

# Optionally fork if you want
# evdoublebind $(evdoublebind-make-config -a ~/.config/evdoublebind.conf) &
```

We will create an deactivation script, '~/scripts/evdb-stop.sh' with the contents below, 
```sh
#!/bin/sh
#kill any running instances 
killall evdoublebind

gsettings reset org.gnome.desktop.input-sources xkb-options 
```

### Step 10: Let's try It Out: This Time with XKB 

Now all you should have to do is run the start script `~/scripts/evdb-start.sh`. 

To stop evdoublebind run `~/scripts/evdb-stop.sh`

### Step 11: Keysym Tap Binding: Modifying Capslock 

We can avoid the problems with direct binding by using symbol binding where
instead of binding the tap key directly we use a XKB keysym. 

Doing this with escape look almost like this,

```conf
kbd : /dev/input/by-path/platform-i8042-serio-0-event-kbd
<LFSH> : @ <BKSL>
<RTSH> : @ <AD02> <LCTL>
#<CAPS> : Control_L @ <ESC>
<CAPS> : Control_L | Escape # THIS CONFIG IS MISSING SOMETHING
```
Notice how instead of the `@` symbol we used a pipe, `|`, this allows us to use
a Keysym. However, in order to use keysym bindings we need to specify unused keys for
evdoublebind to use as intermediate representations between XKB translation.
Which looks like, 
```conf
unused : <FK19> <FK20> <FK21> <FK22> <FK23> <FK24> 
kbd : /dev/input/by-path/platform-i8042-serio-0-event-kbd
<LFSH> : @ <BKSL>
<RTSH> : @ <AD02> <LCTL>
<CAPS> : Control_L | Escape 
```
Here I here the function keys passed 18 since my keyboards does not use them.
You can usually find more by looking in `cat /usr/share/X11/xkb/keycodes/evdev`.
You need to specify enough a unused keys, one for each keysym tap binding you use.

When using keysym tap binding you can specify the shift levels for you key, this
would look like,
```conf
<CAPS> : Control_L | Escape Ampersand
```

... WIP ...
