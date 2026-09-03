<div align="center">

```diff
@@  ____  _        _       ___  ____    v1.0 BETA @@
@@ / ___|| |_ _ __(_)_  __/ _ \/ ___|           @@
@@ \___ \| __| '__| \ \/ / | | \___ \    2026   @@
@@  ___) | |_| |  | |>  <| |_| |___) |  BAREBONES @@
@@ |____/ \__|_|  |_/_/\_\\___/|____/   x86_64   @@
```

# **StrixOS** — `Version 1.0 Beta`

### *Main Functions Completed — Barebones OS — Now Booting on Real Hardware & QEMU*

<p>
  <img src="https://img.shields.io/badge/Version-1.0%20Beta-00FF88?style=for-the-badge&labelColor=0a0a0a" />
  <img src="https://img.shields.io/badge/Arch-x86__64%20Long%20Mode-00BFFF?style=for-the-badge&labelColor=0a0a0a" />
  <img src="https://img.shields.io/badge/Boot-3--Stage%20%7C%20M2%20%7C%20VBE-FF6B6B?style=for-the-badge&labelColor=0a0a0a" />
  <img src="https://img.shields.io/badge/Display-8--bit%20256--colour-FF00FF?style=for-the-badge&labelColor=0a0a0a" />
</p>

<p>
  <img src="https://img.shields.io/badge/C-99%25-00599C?style=flat-square&logo=c&logoColor=white" />
  <img src="https://img.shields.io/badge/Assembly-Intel%20NASM-6E4C13?style=flat-square&logo=data:image/svg+xml;base64,PHN2Zz4=" />
  <img src="https://img.shields.io/badge/Unix-like-POSIX%20Syscalls-333333?style=flat-square&logo=linux&logoColor=white" />
  <img src="https://img.shields.io/badge/License-GPL--3.0-00C853?style=flat-square" />
  <img src="https://img.shields.io/badge/QEMU-9.x-800080?style=flat-square" />
</p>

---

### `>_` *Bare metal. No Linux. No GRUB. Just Strix.*

**StrixOS** is a from-scratch `x86_64` Unix-like OS — 3-stage bootloader → Long Mode → preemptive kernel → `VFS` + `TTY` + `StrixShell`.  
Built in `C` + `x86 Assembly` on `Arch Linux` · Boots in `QEMU` `—` `720p / 1080p / 1024×768` · `VGA 80×25` `8-bit` fallback.

</div>

<br>

<div align="center">

| <sub><b>⚡ LANGUAGE</b></sub> | <sub><b>🧱 CORE</b></sub> | <sub><b>🎨 DISPLAY</b></sub> |
|:-:|:-:|:-:|
| <b><code>C</code></b> `99%` <br> <sub>kernel, drivers, VFS, heap</sub> | <b><code>Unix-like</code></b> <br> <sub><code>int 0x80</code> `8` syscalls</sub> | <b><code>8-bit 256-colour</code></b> <br> <sub>`38;5` `48;5` `38;2` `VGA nearest` + `FB RGB`</sub> |
| <b><code>Assembly</code></b> `NASM` <br> <sub>boot, IDT, ISR, context</sub> | <b><code>Preemptive</code></b> <br> <sub>PIT 100Hz · `MAX_TASKS 16`</sub> | <b><code>VBE / VGA</code></b> <br> <sub>`1024×768×32` `0xE0000000` `80×25`</sub> |

</div>

---

## <samp>◆ PHASES — 10/10 COMPLETE <sub>v0.11</sub></samp>

<div align="center">

| Phase | Subsystem | Status | Details |
|:---:|---|:---:|---|
| **01** | `Bootloader` | `█ 100%` | `boot.asm` `512B` + `stage2_16/32/64` `12KB` → `0x7E00→0x100000` `LBA25` |
| **02** | `Long Mode` | `█ 100%` | `GDT` `PAE` `PML4 0x1000` `P→0-64MB` `2MB` `EFER.LME` |
| **03** | `IDT / ISR / PIC` | `█ 100%` | `IDT` `0x20/0x28` `int3` `div0` `PF` remapped |
| **04** | `PMM / VMM / Heap` | `█ 100%` | `PMM 0x150000 62MB` `VMM 4K` `Heap 1MB` `kmalloc` |
| **05** | `Scheduler` | `█ 100%` | `MAX_TASKS 16` `4` `STACK_PAGES` `PIT 100Hz` `yield` |
| **06** | `Syscalls` | `█ 100%` | `int 0x80` `SYS_WRITE/READ/OPEN/CLOSE/GETPID/YIELD/EXIT/BRK` |
| **07** | `VFS + InitRD` | `█ 100%` | `VFS_MAX_FD 16` `INITRD 5` `→` `32` `writable` |
| **08** | `StrixShell` | `█ 100%` | `bash-like` `32` `history` `Tab` `Ctrl-A/E/K/U/W/L` |
| **09** | `StrixVim` | `█ 100%` | `96×120` `hjkl` `i/a/o` `x/dd` `:w/:q` |
| **10** | `Scalable Mods` | `█ 100%` | `module_register()` `FAT` `ELF` `Net loopback` |

</div>

---

## <samp>◆ DISPLAY — 8-bit 256-colour TTY <sub>this release</sub></samp>

```ansi
[38;5;196m██ [38;5;202m██ [38;5;226m██ [38;5;46m██ [38;5;21m██ [38;5;201m██[0m  256-colour  [48;5;196m  [48;5;46m  [48;5;21m [0m
[38;2;220;220;180m      _..._     [0m  StrixOS Moon — true-colour 38;2 + 256 fallback
[90m# [91m# [92m# [93m# [94m# [95m# [96m# [97m#[0m   VGA 16 → 256 nearest 0x000000…0xFFFFFF
```

<div align="center">

| <b>Mode</b> | <b>Resolution</b> | <b>Colours</b> | <b>Mechanism</b> |
|---|:-:|---|---|
| `VBE` | `1920×1080` `1280×720` `1024×768` `640×480` | `32bpp` `true` | `0xE0000000` `10MB` `fb_put32` `8×8` `font8x8_basic` |
| `VGA fallback` | `80×25` `0xB8000` | `16` `→` `256` `nearest` | `vga_nearest()` `pal256()` `48;5` `&0x7` `blink-safe` |

<small>`ANSI` `38;5;N` `48;5;N` `38;2;R;G;B` `48;2` `30-37` `40-47` `90-97` `100-107` `0` `1` bold</small>

</div>

---

## <samp>◆ QUICK START</samp>

<table><tr><td>

```bash
# 1 — dependencies (Arch)
echo "power" | sudo -S pacman -S nasm qemu qemu-system-x86 gcc binutils

# 2 — build  (≈100KB kernel, 220 sectors)
make -C StrixOS all
# Boot 512B | Stage2 12KB | Kernel 103KB → build/os-image.bin 1M

# 3 — run
make -C StrixOS run-gui    # QEMU -vga std -display gtk,zoom-to-fit=off
make -C StrixOS run        # -display none -serial stdio -vga std
```

</td><td>

```bash
# QEMU window → TTY0
User Login: avi
Password: power      # ******  admin
Welcome, avi (admin)!
StrixOS> ls
README  hello.txt  test.bin  file1.txt  file2.txt  FAT.TXT
StrixOS> stxver      # moon + 8 ## palette
StrixOS> colors      # 256 colours: 0-15  16-231 6×6×6  232-255 gray
StrixOS> rgb test    # RED GREEN BLUE + gradient
StrixOS> rgb 255 100 0 true-orange
StrixOS> vim hello.txt  # StrixVim
StrixOS> poweroff
```

</td></tr></table>

<div align="center">

| <b>Login</b> | `avi` | <b>Password</b> | `power` | <i>only user</i> `bob` → <b>invalid credentials</b> |
|:-:|:-:|:-:|:-:|---|
| <sub>prompt</sub> | `rgb_print("StrixOS> ",80,255,120)` `0x50FF78` persists after `ls`/`cat` |

</div>

---

## <samp>◆ SHELL — bash-like</samp>

<div align="center">

`ls` · `cat` · `echo` · `touch` · `vim`/`vi`/`nano`/`edit` · `admin`/`sudo` · `stxver`/`neofetch` · `rgb` · `colors`/`palette`/`256` · `display`/`gfx`/`fbtest` · `ps` · `modls` · `fatls` · `history` · `uname` · `poweroff`/`reboot`

<sub>`Tab` `complete` `↑↓` `32` `history` `←→` `Home/End` `Ctrl-A/E/K/U/W/L/C` `beep` `did_you_mean`</sub>

</div>

```bash
StrixOS> cat --help      # GNU coreutils 9.5 verbatim help
StrixOS> admin ls        # sudo 1.9.15p5 → [admin] executing as root
StrixOS> touch new.txt && ls
```

---

## <samp>◆ TREE</samp>

```
StrixOS/
├─ bootloader/  boot.asm  stage2_16.asm  stage2_32.asm  stage2_64.asm  boot_config.inc
├─ kernel/      main.c  fb.c  tty.c  keyboard.c  shell.c  coreutils_port.c
│              idt.c  pmm.c  vmm.c  heap.c  process.c  timer.c  syscall.c  vfs.c …
├─ user/        sh.c
├─ build/       boot.bin  stage2.bin  kernel.bin  os-image.bin
└─ Makefile     KERNEL_SECTORS=220  run/run-gui
```

---

## <samp>◆ SPECS</samp>

<div align="center">

| <b>Item</b> | <b>Value</b> |
|---|---|
| `ISA` | `x86_64` `Long Mode` `PAE` |
| `Freq` | `PIT 100Hz` `10 ticks preempt` |
| `Mem` | `PMM 62MB` `15872 pages` `Heap 1MB` `0x150000 bitmap` |
| `FB` | `VBE 0x4F02 0x4118/0x4127/0x4120/0x4112` `PhysBase 0x9008` `pitch 0x900C` |
| `GCC` | `16.2.1` `ffreestanding -mno-red-zone -mcmodel=kernel` |
| `QEMU` | `11.1.0` `-m 256 -serial stdio -vga std -display gtk,zoom-to-fit=off` |

</div>

---

## <samp>◆ ROADMAP — beyond Beta</samp>

- [ ] `FAT32` `write` + `ELF` `loader` `exec`
- [ ] `Net` `e1000` + `TCP`
- [ ] `SMP` + `APIC`
- [ ] `VFS` `ext2`
- [ ] `Framebuffer` `enable` when `PD` covers `phys` `0xF0000000` + `PAT`

---

<div align="center">

### <samp>Lowtier Studios — Prabhutva Singh — Lucknow</samp>
<sub>Built from zero. No GRUB. No Linux. Just registers & faith.</sub>

<br>

```
  _..._      StrixOS Moon
.'     '.    Barebones → Booted
:  o o  :    x86_64 ~= ready
 '._ _.'     1.0 Beta — main functions completed
   """
```

**`git clone https://github.com/prabhutvasingh/strixos && make -C strixos run-gui`**

<p>
  <img src="https://img.shields.io/badge/StrixOS-1.0%20BETA-00FF88?style=for-the-badge" />
  <img src="https://img.shields.io/badge/BOOTING-✓-00BFFF?style=for-the-badge" />
</p>

</div>
