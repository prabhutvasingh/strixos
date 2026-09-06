# StrixOS — Made by Avi, age 12

> **100% independent OS. Custom bootloader. Custom Strix Kernel. No Linux. No Unix. No GRUB. Just Strix.**

Second 12-year-old in the world to build a real OS from zero — boots on QEMU + real hardware, with its own shell, editor, filesystem, colors.

## Why StrixOS is special
- Built from scratch in C + Assembly (x86_64)
- Own 3-stage bootloader → Long Mode → Strix Kernel
- Friendly Strix Shell — made for beginners, not unix wizards

## Super-friendly commands
```
help      show friendly help
list      see your files
read hi.txt   read a file
say hello     print something
make note.txt create a file
write note.txt edit a file
goto /home/strix go places (whereami to see where you are)
whoami    who are you?
admin ... run as boss (sudo still works)
about     who made this?
colors, moon, rgb test  have fun
bye / poweroff, restart / reboot
```
Old unix names (ls/cat/echo/touch/cd/pwd/sudo/clear/vim) still work too.

## Quick start
```bash
make -C StrixOS all
make -C StrixOS run-gui
# Login: avi / power
StrixOS> help
StrixOS> about
StrixOS> list
```

## Proudly independent
Not a Linux distro. Not Unix-like. Strix Kernel, Strix Shell, Strix Editor — all custom.

— Avi, Lowtier Studios, Lucknow. Built from zero. Registers & faith.
