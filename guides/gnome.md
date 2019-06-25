# Step By Step Guide For Advanced Usage of Evdoublebind for Gnome( Wayland or X11) [WIP]

... GUIDE IS WIP ...

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

Lets start off the config specifying our keyboard
```conf
kbd : insert-your-keyboard-path-here
```

### Step 4: Adding a Basic Direct Binding
Lets add a basic direct binding.

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
is `<LFSH>`  and backslash is `<BKSL>`. Lets add it to our config,

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

### Step 6: Lets Try it Out

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
> sudo evdoublebind $(evdoublebind-make-config -a ~/.config/evdoublebind.conf)
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
make it that simple, as mentioned in the introduction.

... GUIDE IS WIP ...

For advanced use we also need to specify some on used key of our keyboard so
evdoublebind can use them as intermediate representations before being finally
mapped to the desired key. 

### Step 6: Setup XKB Rules Hook

/usr/share/libalpm/hooks/evdoublebind.hook
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

/usr/share/libalpm/scripts/update-xkb-rule
```sh
#!/bin/sh
sed -i 's/!\s*option\s*=\s*symbols/! option	=	symbols\n  evdoublebind:mapping = +evdoublebind(mapping)/g' /usr/share/X11/xkb/rules/evdev
```

### Step 7: Reinstall xkeyboard-config to trigger hook

```sh
> sudo pacman -S xkeyboard-config
```
