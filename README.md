Evdoublebind, via evdev, provides double bind keys: keys which are overloaded with functional acting as a modifier when held but another key when tapped alone. Although other applications strive for the same core functionality, Evdoublebind is unique in that it...
* Runs at a low level interfacing evdev directly so that it is display server
agnostic even working in a TTY.
* Introduces as little latency as possible, allowing regular key events to
  processed by system directly without redirection or a virtual input device.

Evdoublebind has gone through a major refactor to be easier setup the old simpler version can be found on it own [branch](https://github.com/exrok/evdoublebind/tree/tiny-over-optimized-version); 

Currently more information can found on a [blog post](https://i64.dev/evdoublebind-introduction/) introducing the program.

A step by step guide for setup on wayland and X11 is comming soon. 
