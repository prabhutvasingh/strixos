<div align="center">

<pre>
 .--..--..--..--..--..--..--..--..--..--..--..--..--..--.
 / .. \.. \.. \.. \.. \.. \.. \.. \.. \.. \.. \.. \.. \.. \
 \ \/\ `'\ `'\ `'\ `'\ `'\ `'\ `'\ `'\ `'\ `'\ `'\ `'\ \/ /
  \/ /`--'`--'`--'`--'`--'`--'`--'`--'`--'`--'`--'`--'\/ /
  / /\                                                / /\
 / /\ \     ____ _____ ____  _____  _____  ____      / /\ \
 \ \/ /    / ___|_   _|  _ \|_ _\ \/ / _ \/ ___|     \ \/ /
  \/ /     \___ \ | | | |_) || | \  / | | \___ \      \/ /
  / /\      ___) || | |  _ < | | /  \ |_| |___) |     / /\
 / /\ \    |____/ |_| |_| \_\___/_/\_\___/|____/     / /\ \
 \ \/ /                                              \ \/ /
  \/ /                                                \/ /
  / /\.--..--..--..--..--..--..--..--..--..--..--..--./ /\
 / /\ \.. \.. \.. \.. \.. \.. \.. \.. \.. \.. \.. \.. \/\ \
 \ `'\ `'\ `'\ `'\ `'\ `'\ `'\ `'\ `'\ `'\ `'\ `'\ `'\ `' /
  `--'`--'`--'`--'`--'`--'`--'`--'`--'`--'`--'`--'`--'`--'
</pre>

# **StrixOS** — `Version 1.0 Beta`

### *Made from scratch by **Avi, age 12** — Custom Bootloader + Custom Strix Kernel*

<p>
  <img src="https://img.shields.io/badge/Made_by-Avi_age_12-FF6B9D?style=for-the-badge&labelColor=0a0a0a" />
  <img src="https://img.shields.io/badge/100%25-Independent-00FF88?style=for-the-badge&labelColor=0a0a0a" />
  <img src="https://img.shields.io/badge/No_Linux-No_Unix-No_GRUB-00BFFF?style=for-the-badge&labelColor=0a0a0a" />
</p>

<p>
  <img src="https://img.shields.io/badge/C-99%25-00599C?style=flat-square&logo=c&logoColor=white" />
  <img src="https://img.shields.io/badge/Assembly-Intel_NASM-6E4C13?style=flat-square" />
  <img src="https://img.shields.io/badge/Arch-x86__64_Long_Mode-800080?style=flat-square" />
  <img src="https://img.shields.io/badge/License-GPL--3.0-00C853?style=flat-square" />
  <img src="https://img.shields.io/badge/QEMU-9.x-FF00FF?style=flat-square" />
</p>

> ### `>_` *Bare metal. No Linux. No Unix. No GRUB. Just Strix.*

**StrixOS** is a fully independent OS — 3-stage bootloader → Long Mode → Strix Kernel → `VFS` + `TTY` + friendly `Strix Shell` + `Strix Write` editor.
Built in `C` + `x86 Assembly` · Boots in `QEMU` + real hardware · `720p / 1080p / 1024×768` · `256-colour` TTY.

</div>

<br>

<div align="center">

| <sub><b>⚡ LANGUAGE</b></sub> | <sub><b>🧱 CORE</b></sub> | <sub><b>🎨 DISPLAY</b></sub> |
|:-:|:-:|:-:|
| <b><code>C</code></b> `99%` <br> <sub>kernel, drivers, VFS, heap</sub> | <b><code>Strix Kernel</code></b> <br> <sub>own syscalls, scheduler, VFS</sub> | <b><code>8-bit 256-colour</code></b> <br> <sub>`38;5` `48;5` `38;2` + `FB RGB`</sub> |
| <b><code>Assembly</code></b> `NASM` <br> <sub>boot, IDT, ISR, context</sub> | <b><code>Preemptive</code></b> <br> <sub>PIT 100Hz · tasks · TTY logins</sub> | <b><code>VBE / VGA</code></b> <br> <sub>`1024×768×32` `0xE0000000` `80×25`</sub> |

</div>

---

## <samp>◆ WHY STRIXOS IS SPECIAL <sub>proudly independent</sub></samp>

<div align="center">

| | |
|---|---|
| 🧒 | **Second 12-year-old in the world to build a real OS from zero** — every line of bootloader + kernel written from scratch |
| 🛠️ | **Custom everything** — bootloader, GDT, paging, IDT, heap, VFS, shell, editor |
| 💛 | **Beginner friendly** — `list`, `read`, `say`, `make`, `write`, `goto`, `whereami`, `about` |
| 🖥️ | **Boots for real** — QEMU + real hardware, VBE graphics + VGA fallback |

</div>

---

## <samp>◆ QUICK START</samp>

<table><tr><td>

```bash
# 1 — dependencies (Arch)
sudo pacman -S nasm qemu qemu-system-x86 gcc binutils

# 2 — build
make -C StrixOS all

# 3 — run
make -C StrixOS run-gui    # graphical
make -C StrixOS run        # serial
```

</td><td>

```bash
# QEMU window → TTY0
User Login: avi
Password: power
Welcome to StrixOS! Made from scratch by Avi (12).
StrixOS> help
StrixOS> about
StrixOS> list
StrixOS> write hello.txt
StrixOS> bye
```

</td></tr></table>

---

## <samp>◆ SHELL — Strix Shell (friendly first)</samp>

<div align="center">

`list` · `read` · `say` · `make` · `write` · `goto` · `whereami` · `whoami` · `wipe` · `admin` · `about` · `colors` · `moon` · `bye`

<sub>old unix names (`ls` `cat` `echo` `touch` `cd` `pwd` `sudo` `clear` `vim` `nano`) still work</sub>

</div>

```bash
StrixOS> say hello strix
StrixOS> make note.txt
StrixOS> write note.txt    # Strix Write: type, ^O save, ^X exit
StrixOS> read note.txt
StrixOS> goto /home/strix
StrixOS> whereami
StrixOS> admin list        # be the boss
StrixOS> colors            # 256 colours
StrixOS> moon              # StrixOS Moon
```

---

## <samp>◆ PHASES — 10/10 COMPLETE <sub>v1.0 Beta</sub></samp>

<div align="center">

| Phase | Subsystem | Status | Details |
|:---:|---|:---:|---|
| **01** | `Bootloader` | `█ 100%` | `boot.asm` `512B` + `stage2_16/32/64` `12KB` → Long Mode |
| **02** | `Long Mode` | `█ 100%` | `GDT` `PAE` `PML4` `2MB pages` `EFER.LME` |
| **03** | `IDT / ISR / PIC` | `█ 100%` | keyboard, timer, faults, remapped PIC |
| **04** | `PMM / VMM / Heap` | `█ 100%` | `PMM 62MB` `VMM 4K` `Heap 1MB` `kmalloc` |
| **05** | `Scheduler` | `█ 100%` | preemptive `PIT 100Hz`, tasks, yield |
| **06** | `Syscalls` | `█ 100%` | Strix syscalls `WRITE/READ/OPEN/CLOSE/YIELD/EXIT/BRK` |
| **07** | `VFS + InitRD` | `█ 100%` | writable in-memory FS, `save` really saves |
| **08** | `Strix Shell` | `█ 100%` | friendly + history + Tab + Ctrl keys |
| **09** | `Strix Write` | `█ 100%` | ANSI editor, `^O` save `^X` exit `^K` cut |
| **10** | `Display` | `█ 100%` | VBE framebuffer + VGA + 256-colour TTY |

</div>

---

## <samp>◆ TREE</samp>

```
StrixOS/
├─ bootloader/  boot.asm  stage2_16.asm  stage2_32.asm  stage2_64.asm
├─ kernel/      main.c  shell.c  editor.c  vfs.c  tty.c  fb.c  keyboard.c …
├─ user/        sh.c
├─ build/       boot.bin  stage2.bin  kernel.bin  os-image.bin  strixos.iso
└─ Makefile     run / run-gui / run-iso
```

---

<div align="center">

### <samp>Lowtier Studios — Avi — Lucknow</samp>
<sub>Built from zero. No GRUB. No Linux. Just registers & faith.</sub>

<br>

```
  _..._      StrixOS Moon
.'     '.    Barebones → Booted
:  o o  :    x86_64 ~= ready
 '._ _.'     1.0 Beta — made by Avi, 12
    """
```

**`git clone https://github.com/prabhutvasingh/strixos && make -C strixos run-gui`**

<p>
  <img src="https://img.shields.io/badge/StrixOS-1.0_BETA-00FF88?style=for-the-badge" />
  <img src="https://img.shields.io/badge/MADE_BY_AVI-AGE_12-FF6B9D?style=for-the-badge" />
  <img src="https://img.shields.io/badge/BOOTING-✓-00BFFF?style=for-the-badge" />
</p>

</div>
