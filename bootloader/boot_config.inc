; =============================================================================
; Bootloader shared constants
; =============================================================================

; --- Memory layout ---
STAGE2_BASE     equ 0x7E00       ; Stage1 loads Stage2 here
STACK_TOP       equ 0x90000      ; Temporary stack

STAGE16_SIZE    equ 4096         ; 8 sectors
STAGE32_SIZE    equ 4096
STAGE64_SIZE    equ 4096
STAGE2_SIZE     equ STAGE16_SIZE + STAGE32_SIZE + STAGE64_SIZE  ; 12288 = 24 sectors

STAGE16_ENTRY   equ STAGE2_BASE                                   ; 0x7E00
STAGE32_ENTRY   equ STAGE2_BASE + STAGE16_SIZE                    ; 0x8E00
STAGE64_ENTRY   equ STAGE2_BASE + STAGE16_SIZE + STAGE32_SIZE     ; 0x9E00

; --- Disk layout (1-indexed sectors) ---
; Sector 1:        Stage1 boot.asm (512 bytes)
; Sectors 2-9:     stage2_16 (4096 bytes)
; Sectors 10-17:   stage2_32 (4096 bytes)
; Sectors 18-25:   stage2_64 (4096 bytes)
; Sectors 26+:     kernel.bin
BOOT_SECTORS    equ 1
STAGE16_SECTORS equ STAGE16_SIZE / 512   ; 8
STAGE32_SECTORS equ STAGE32_SIZE / 512   ; 8
STAGE64_SECTORS equ STAGE64_SIZE / 512   ; 8
STAGE2_SECTORS  equ STAGE2_SIZE / 512    ; 24
KERNEL_LBA_START equ 1 + STAGE2_SECTORS  ; LBA 25
KERNEL_SECTOR_START equ KERNEL_LBA_START + 1  ; CHS sector number 26 (1-indexed)
KERNEL_SECTORS  equ 4096                 ; 2M kernel for official Vim tiny 1.5M (was 220)

KERNEL_OFFSET   equ 0x100000     ; Final kernel location (1MB)
KERNEL_TMP      equ 0x10000      ; Temp bounce buffer for BIOS reads

; --- GDT selectors ---
GDT_CODE        equ 0x08
GDT_DATA        equ 0x10
