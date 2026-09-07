# =============================================================================
# MyOS Build System - Split Stage2
# =============================================================================
NASM = nasm
CC   = gcc
LD   = ld
QEMU = qemu-system-x86_64

CFLAGS  = -ffreestanding -mno-red-zone -mcmodel=kernel \
          -mno-sse -mno-sse2 -mno-mmx \
          -fno-stack-protector -fno-pie -no-pie \
          -Wall -Wextra -Werror -I kernel/ -fno-builtin -g

LDFLAGS = -nostdlib -z max-page-size=0x1000 -T kernel/linker.ld
NFLAGS  = -f elf64

BUILD_DIR = build

BOOT_SRC    = bootloader/boot.asm
S16_SRC     = bootloader/stage2_16.asm
S32_SRC     = bootloader/stage2_32.asm
S64_SRC     = bootloader/stage2_64.asm
KENTRY      = kernel/entry.asm
KSRC        = kernel/main.c
KIDT        = kernel/idt.c
KPIC        = kernel/pic.c
KISR        = kernel/isr.asm
KPMM        = kernel/pmm.c
KVMM        = kernel/vmm.c
KHEAP       = kernel/heap.c
KPROC       = kernel/process.c
KTIMER      = kernel/timer.c
KCTX        = kernel/context.asm
KSYSC       = kernel/syscall.c
KVFS        = kernel/vfs.c
KINITRD     = kernel/initrd.c
KSHELL      = kernel/shell.c
KMODULE     = kernel/module.c
KFAT        = kernel/fat.c
KELF        = kernel/elf.c
KNET        = kernel/net.c
KSTRING     = kernel/string.c
KEDITOR     = kernel/editor.c
 KCOREPORT   = kernel/coreutils_port.c
KFB         = kernel/fb.c
KTTY        = kernel/tty.c
KKBD        = kernel/keyboard.c

BOOT_BIN = $(BUILD_DIR)/boot.bin
S16_BIN  = $(BUILD_DIR)/stage2_16.bin
S32_BIN  = $(BUILD_DIR)/stage2_32.bin
S64_BIN  = $(BUILD_DIR)/stage2_64.bin
STAGE2_BIN = $(BUILD_DIR)/stage2.bin
KENTRY_OBJ = $(BUILD_DIR)/entry.o
KSRC_OBJ   = $(BUILD_DIR)/main.o
KIDT_OBJ   = $(BUILD_DIR)/idt.o
KPIC_OBJ   = $(BUILD_DIR)/pic.o
KISR_OBJ   = $(BUILD_DIR)/isr.o
KPMM_OBJ   = $(BUILD_DIR)/pmm.o
KVMM_OBJ   = $(BUILD_DIR)/vmm.o
KHEAP_OBJ  = $(BUILD_DIR)/heap.o
KPROC_OBJ  = $(BUILD_DIR)/process.o
KTIMER_OBJ = $(BUILD_DIR)/timer.o
KCTX_OBJ   = $(BUILD_DIR)/context.o
KSYSC_OBJ  = $(BUILD_DIR)/syscall.o
KVFS_OBJ   = $(BUILD_DIR)/vfs.o
KINITRD_OBJ= $(BUILD_DIR)/initrd.o
KSHELL_OBJ = $(BUILD_DIR)/shell.o
KMODULE_OBJ= $(BUILD_DIR)/module.o
KFAT_OBJ   = $(BUILD_DIR)/fat.o
KELF_OBJ   = $(BUILD_DIR)/elf.o
KNET_OBJ   = $(BUILD_DIR)/net.o
KSTRING_OBJ = $(BUILD_DIR)/string.o
KEDITOR_OBJ = $(BUILD_DIR)/editor.o
 KCOREPORT_OBJ= $(BUILD_DIR)/coreutils_port.o
KFB_OBJ      = $(BUILD_DIR)/fb.o
KTTY_OBJ     = $(BUILD_DIR)/tty.o
KKBD_OBJ     = $(BUILD_DIR)/keyboard.o
KERNEL_ELF = $(BUILD_DIR)/kernel.elf
KERNEL_BIN = $(BUILD_DIR)/kernel.bin

ISO = $(BUILD_DIR)/strixos.iso
ISO_DIR = $(BUILD_DIR)/iso

.PHONY: all iso clean run run-gui run-iso debug uefi run-uefi run-uefi-nographic iso-uefi run-iso-uefi

all: $(BUILD_DIR)/os-image.bin $(ISO)

iso: $(ISO)

$(BOOT_BIN): $(BOOT_SRC)
	@mkdir -p $(BUILD_DIR)
	$(NASM) -f bin $< -o $@

$(S16_BIN): $(S16_SRC) bootloader/boot_config.inc
	@mkdir -p $(BUILD_DIR)
	$(NASM) -f bin -I . -I bootloader $< -o $@

$(S32_BIN): $(S32_SRC) bootloader/boot_config.inc
	$(NASM) -f bin -I . -I bootloader $< -o $@

$(S64_BIN): $(S64_SRC) bootloader/boot_config.inc
	$(NASM) -f bin -I . -I bootloader $< -o $@

$(STAGE2_BIN): $(S16_BIN) $(S32_BIN) $(S64_BIN)
	cat $(S16_BIN) $(S32_BIN) $(S64_BIN) > $@

$(KENTRY_OBJ): $(KENTRY)
	$(NASM) $(NFLAGS) $< -o $@

$(KISR_OBJ): $(KISR)
	$(NASM) $(NFLAGS) $< -o $@

$(KSRC_OBJ): $(KSRC) kernel/io.h kernel/idt.h kernel/pmm.h kernel/vmm.h kernel/heap.h
	$(CC) $(CFLAGS) -c $< -o $@

$(KIDT_OBJ): $(KIDT) kernel/idt.h kernel/pic.h kernel/io.h
	$(CC) $(CFLAGS) -c $< -o $@

$(KPIC_OBJ): $(KPIC) kernel/pic.h kernel/io.h
	$(CC) $(CFLAGS) -c $< -o $@

$(KPMM_OBJ): $(KPMM) kernel/pmm.h kernel/io.h
	$(CC) $(CFLAGS) -c $< -o $@

$(KVMM_OBJ): $(KVMM) kernel/vmm.h kernel/pmm.h kernel/io.h
	$(CC) $(CFLAGS) -c $< -o $@

$(KHEAP_OBJ): $(KHEAP) kernel/heap.h kernel/pmm.h
	$(CC) $(CFLAGS) -c $< -o $@

$(KPROC_OBJ): $(KPROC) kernel/process.h kernel/pmm.h kernel/heap.h
	$(CC) $(CFLAGS) -c $< -o $@

$(KTIMER_OBJ): $(KTIMER) kernel/timer.h kernel/pic.h kernel/process.h
	$(CC) $(CFLAGS) -c $< -o $@

$(KCTX_OBJ): $(KCTX)
	$(NASM) $(NFLAGS) $< -o $@

$(KSYSC_OBJ): $(KSYSC) kernel/syscall.h kernel/idt.h kernel/process.h kernel/heap.h kernel/vfs.h
	$(CC) $(CFLAGS) -c $< -o $@

$(KVFS_OBJ): $(KVFS) kernel/vfs.h kernel/initrd.h kernel/heap.h
	$(CC) $(CFLAGS) -c $< -o $@

$(KINITRD_OBJ): $(KINITRD) kernel/initrd.h
	$(CC) $(CFLAGS) -c $< -o $@

$(KSHELL_OBJ): $(KSHELL) kernel/shell.h kernel/syscall.h kernel/vfs.h kernel/process.h
	$(CC) $(CFLAGS) -c $< -o $@

$(KMODULE_OBJ): $(KMODULE) kernel/module.h
	$(CC) $(CFLAGS) -c $< -o $@

$(KFAT_OBJ): $(KFAT) kernel/fat.h
	$(CC) $(CFLAGS) -c $< -o $@

$(KELF_OBJ): $(KELF) kernel/elf.h
	$(CC) $(CFLAGS) -c $< -o $@

$(KNET_OBJ): $(KNET) kernel/net.h
	$(CC) $(CFLAGS) -c $< -o $@

$(KSTRING_OBJ): $(KSTRING)
	$(CC) $(CFLAGS) -c $< -o $@

$(KEDITOR_OBJ): $(KEDITOR) kernel/editor.h kernel/vfs.h kernel/heap.h
	$(CC) $(CFLAGS) -c $< -o $@


$(KCOREPORT_OBJ): $(KCOREPORT) kernel/coreutils_port.h kernel/vfs.h kernel/heap.h
	$(CC) $(CFLAGS) -c $< -o $@

$(KFB_OBJ): $(KFB) kernel/fb.h
	$(CC) $(CFLAGS) -c $< -o $@

$(KTTY_OBJ): $(KTTY) kernel/tty.h kernel/fb.h kernel/keyboard.h
	$(CC) $(CFLAGS) -c $< -o $@

$(KKBD_OBJ): $(KKBD) kernel/keyboard.h kernel/fb.h
	$(CC) $(CFLAGS) -c $< -o $@


KOBJS = $(KENTRY_OBJ) $(KISR_OBJ) $(KCTX_OBJ) $(KSRC_OBJ) $(KIDT_OBJ) $(KPIC_OBJ) $(KPMM_OBJ) $(KVMM_OBJ) $(KHEAP_OBJ) $(KPROC_OBJ) $(KTIMER_OBJ) $(KSYSC_OBJ) $(KVFS_OBJ) $(KINITRD_OBJ) $(KSHELL_OBJ) $(KMODULE_OBJ) $(KFAT_OBJ) $(KELF_OBJ) $(KNET_OBJ) $(KSTRING_OBJ) $(KEDITOR_OBJ) $(KCOREPORT_OBJ) $(KFB_OBJ) $(KTTY_OBJ) $(KKBD_OBJ)

$(KERNEL_ELF): $(KOBJS)
	$(LD) $(LDFLAGS) -o $@ $^
	@echo "Kernel ELF: $$(wc -c < $@) bytes"

$(KERNEL_BIN): $(KERNEL_ELF)
	objcopy -O binary $< $@
	@echo "Kernel BIN: $$(wc -c < $@) bytes"

# Disk: 4M for StrixOS (sector 0 boot, 1-24 stage2, 25+ kernel)
# Optional nano blob: only embedded if present, never breaks build
NANO_BIN = $(BUILD_DIR)/nano.bin
NANO_SRC = /tmp/nano_official_bin
$(NANO_BIN):
	@mkdir -p $(BUILD_DIR)
	@if [ -f $(NANO_SRC) ]; then cp $(NANO_SRC) $@ && echo "Nano official: $$(wc -c < $@) bytes"; else echo "(no nano blob - skipping, native Strix editor used)"; touch $@; fi

# Disk: 4M for StrixOS
$(BUILD_DIR)/os-image.bin: $(BOOT_BIN) $(STAGE2_BIN) $(KERNEL_BIN)
	@echo "=== Building OS image ==="
	dd if=/dev/zero of=$@ bs=1M count=4 2>/dev/null
	dd if=$(BOOT_BIN) of=$@ bs=512 count=1 conv=notrunc 2>/dev/null
	dd if=$(STAGE2_BIN) of=$@ bs=512 seek=1 conv=notrunc 2>/dev/null
	dd if=$(KERNEL_BIN) of=$@ bs=512 seek=25 conv=notrunc 2>/dev/null
	@if [ -f $(NANO_SRC) ]; then dd if=$(NANO_SRC) of=$@ bs=512 seek=4121 conv=notrunc 2>/dev/null && echo "Nano embedded"; fi
	@echo "Boot  : $$(wc -c < $(BOOT_BIN)) bytes"
	@echo "Stage2: $$(wc -c < $(STAGE2_BIN)) bytes (16:$$(wc -c < $(S16_BIN)) 32:$$(wc -c < $(S32_BIN)) 64:$$(wc -c < $(S64_BIN)))"
	@echo "Kernel: $$(wc -c < $(KERNEL_BIN)) bytes"
	@echo "=== $@ built ==="

$(ISO): $(BUILD_DIR)/os-image.bin
	@echo "=== Building ISO (convenience) ==="
	@mkdir -p $(ISO_DIR)
	@cp $(BUILD_DIR)/os-image.bin $(ISO_DIR)/strixos.img
	@cp $(KERNEL_BIN) $(ISO_DIR)/kernel.bin 2>/dev/null || true
	@cp $(BOOT_BIN) $(ISO_DIR)/boot.bin 2>/dev/null || true
	@echo "StrixOS 1.0 Beta - https://github.com/prabhutvasingh/strixos" > $(ISO_DIR)/README.txt
	@echo "Boot raw: dd if=strixos.img of=/dev/sdX bs=512" >> $(ISO_DIR)/README.txt
	@echo "QEMU raw: qemu-system-x86_64 -drive file=strixos.img,format=raw -m 256" >> $(ISO_DIR)/README.txt
	@echo "QEMU iso: qemu-system-x86_64 -cdrom $(ISO) -m 256 -boot d" >> $(ISO_DIR)/README.txt
	@xorriso -as mkisofs -o $@ -V STRIXOS_1_0 -b strixos.img -hard-disk-boot -boot-load-size 8192 -J -R -iso-level 2 -v $(ISO_DIR) 2>&1 | tail -n 20
	@echo "ISO  : $$(wc -c < $@) bytes  -> $@"
	@echo "IMG  : $$(wc -c < $(BUILD_DIR)/os-image.bin) bytes (raw bootable)"
	@echo "Both built for convenience: use .iso for drag/drop, .img/.bin for dd"

run: $(BUILD_DIR)/os-image.bin
	$(QEMU) -drive file=$<,format=raw,index=0,media=disk -m 256 -serial stdio -display none -vga std

run-gui: $(BUILD_DIR)/os-image.bin
	$(QEMU) -drive file=$<,format=raw,index=0,media=disk -m 256 -serial stdio -vga std -display gtk,zoom-to-fit=off

run-iso: $(ISO)
	$(QEMU) -cdrom $< -m 256 -serial stdio -vga std -display gtk,zoom-to-fit=off -boot d

debug: $(BUILD_DIR)/os-image.bin
	$(QEMU) -drive file=$<,format=raw,index=0,media=disk -m 256 -serial stdio -display none -vga std -S -gdb tcp::1234 &

# --- UEFI (dual-boot: BIOS path above untouched) ---
UEFI_SRC = uefi/loader.c
UEFI_OBJ = $(BUILD_DIR)/uefi-loader.o
UEFI_EFI = $(BUILD_DIR)/BOOTX64.EFI
UEFI_IMG = $(BUILD_DIR)/strix-uefi.img
OVMF_CODE = /usr/share/edk2-ovmf/x64/OVMF_CODE.4m.fd

$(UEFI_OBJ): $(UEFI_SRC) uefi/efi.h kernel/bootinfo.h
	@mkdir -p $(BUILD_DIR)
	$(CC) -ffreestanding -mabi=ms -mgeneral-regs-only -mcmodel=large -fno-stack-protector -fno-pie -no-pie \
	  -Wall -Wextra -I uefi/ -I kernel/ -fno-builtin -g -c $< -o $@

$(UEFI_EFI): $(UEFI_OBJ) uefi/link.ld
	$(LD) -mi386pep --subsystem 10 --entry efi_main --strip-debug --discard-all -o $@ $<
	objcopy -R .debug_info -R .debug_abbrev -R .debug_aranges -R .debug_line -R .debug_str -R .comment -R .note* $@
	@echo "UEFI loader: $$(wc -c < $@) bytes -> $@"

UEFI_PART_OFF = 1048576
$(UEFI_IMG): $(UEFI_EFI) $(KERNEL_BIN)
	@echo "=== Building UEFI ESP image (MBR + FAT ESP) ==="
	dd if=/dev/zero of=$@ bs=1M count=64 2>/dev/null
	printf 'label: dos\nstart=2048, type=ef\n' | sfdisk $@ >/dev/null 2>&1
	mkdir -p $(BUILD_DIR)/esp/EFI/BOOT
	cp $(UEFI_EFI) $(BUILD_DIR)/esp/EFI/BOOT/BOOTX64.EFI
	cp $(KERNEL_BIN) $(BUILD_DIR)/esp/kernel.bin
	mformat -i $@@@$(UEFI_PART_OFF) -v STRIXESP ::
	mmd -i $@@@$(UEFI_PART_OFF) ::/EFI ::/EFI/BOOT
	mcopy -i $@@@$(UEFI_PART_OFF) $(BUILD_DIR)/esp/EFI/BOOT/BOOTX64.EFI ::/EFI/BOOT/
	mcopy -i $@@@$(UEFI_PART_OFF) $(BUILD_DIR)/esp/kernel.bin ::
	@echo "=== $@ built (MBR ESP: BOOTX64.EFI + kernel.bin) ==="

# --- Bootable ISO (UEFI El Torito; plain FAT EFI image, no MBR) ---
UEFI_FAT = $(BUILD_DIR)/esp-fat.img
UEFI_ISO = $(BUILD_DIR)/strixos-uefi.iso
$(UEFI_FAT): $(UEFI_EFI) $(KERNEL_BIN)
	@echo "=== Building El Torito FAT image ==="
	dd if=/dev/zero of=$@ bs=1M count=12 2>/dev/null
	mformat -i $@ -v STRIXEFI ::
	mmd -i $@ ::/EFI ::/EFI/BOOT
	mkdir -p $(BUILD_DIR)/esp/EFI/BOOT
	cp $(UEFI_EFI) $(BUILD_DIR)/esp/EFI/BOOT/BOOTX64.EFI
	cp $(KERNEL_BIN) $(BUILD_DIR)/esp/kernel.bin
	mcopy -i $@ $(BUILD_DIR)/esp/EFI/BOOT/BOOTX64.EFI ::/EFI/BOOT/
	mcopy -i $@ $(BUILD_DIR)/esp/kernel.bin ::
	@echo "=== $@ built ==="

$(UEFI_ISO): $(UEFI_FAT)
	@echo "=== Building bootable UEFI ISO ==="
	@mkdir -p $(BUILD_DIR)/iso-uefi
	@cp $(UEFI_FAT) $(BUILD_DIR)/iso-uefi/esp-fat.img
	@echo "StrixOS 1.0 Beta UEFI - by Avi (12)" > $(BUILD_DIR)/iso-uefi/README.txt
	@echo "Boot: burn to USB (dd) or boot ISO in UEFI mode" >> $(BUILD_DIR)/iso-uefi/README.txt
	@xorriso -as mkisofs -o $@ -V STRIXOS_UEFI -e esp-fat.img -no-emul-boot -R -J $(BUILD_DIR)/iso-uefi 2>&1 | tail -n 4
	@echo "ISO  : $$(wc -c < $@) bytes -> $@"

iso-uefi: $(UEFI_ISO)

run-iso-uefi: $(UEFI_ISO)
	$(QEMU) -cdrom $< -m 256 -serial stdio -vga std -display gtk,zoom-to-fit=off -boot d \
	  -drive if=pflash,format=raw,readonly=on,file=$(OVMF_CODE)

uefi: $(UEFI_IMG)

run-uefi: $(UEFI_IMG)
	$(QEMU) -drive file=$<,format=raw,index=0,media=disk -m 256 -serial stdio -vga std \
	  -display gtk,zoom-to-fit=off -drive if=pflash,format=raw,readonly=on,file=$(OVMF_CODE)

run-uefi-nographic: $(UEFI_IMG)
	$(QEMU) -drive file=$<,format=raw,index=0,media=disk -m 256 -serial stdio -display none -vga std \
	  -drive if=pflash,format=raw,readonly=on,file=$(OVMF_CODE)

clean:
	rm -rf $(BUILD_DIR)