Evdoublebind, via evdev, provides double bind keys: keys which are overloaded with functional acting as a modifier when held but another key when tapped alone. Although other applications strive for the same core functionality, Evdoublebind is unique in that it...
* Runs at a low level interfacing evdev directly so that it is display server
agnostic even working in a TTY. 
* Is very simple and small, written in less than 100 lines of C code which when
  compiled statically with musl and stripped produces a ~6kb binary on linux. 
* Introduces as little latency as possible, allowing regular key events to
  processed by system directly without redirection or a virtual input device.

Currently more information can found on a [blog post](https://i64.dev/evdoublebind-introduction/) introducing the program.

A step by step guide for setup on wayland and X11 is comming soon. 
