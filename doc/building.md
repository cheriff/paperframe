# Building

Building _should_ be a matter of fetching the gity repo and typing make. The included Makefile is intended to fetch SDKs and utilities required to build. For the moment, I've only tested on MacOS.

One item which is not automatic is the compiler toolchain itself, as well as CMake. The best way to obtain this would be to follow manufacturer's reccomendation: [getting-started-with-pico.pdf](https://datasheets.raspberrypi.org/pico/getting-started-with-pico.pdf)

```
$ git clone https://github.com/cheriff/paperframe.git
Cloning into 'paperframe'...
remote: Enumerating objects: 3, done.
remote: Counting objects: 100% (3/3), done.
remote: Total 3 (delta 0), reused 0 (delta 0), pack-reused 0
Receiving objects: 100% (3/3), done.

$ cd paperframe/rpi-pico

$ make
mkdir build
( cd build && cmake ../ )
PICO_SDK_PATH is /Users/davidm/code/paperframe/rpi-pico/pico-sdk
Defaulting PICO_PLATFORM to rp2040 since not specified.
Defaulting PICO platform compiler to pico_arm_gcc since not specified.
<etc>

$ ls build/paperframe*
$ ls build/paperframe.*
build/paperframe.bin     build/paperframe.dis     build/paperframe.elf 
build/paperframe.elf.map build/paperframe.hex     build/paperframe.uf2
```

These files can then be loaded onto a running board. Some folks like to copy the `uf2` file to the Mass Storage device presented by the firware, but I prefer to use picotool, which is also wrapped by the makefile:

```
$ make run
picotool/picotool load -x build/paperframe.elf
Loading into Flash: [==============================]  100%
```
The board will reset and begin running.
