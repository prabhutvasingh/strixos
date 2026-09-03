; =============================================================================
; stage2_16.asm - 16-bit real mode portion
; Loaded at 0x7E00 by Stage1 (8 sectors = 4096 bytes, padded)
; - Enable A20
; - Load kernel from disk (sectors 26+) to bounce buffer 0x10000
; - Setup GDT, enter 32-bit protected mode, far jump to stage2_32
; =============================================================================
%include "bootloader/boot_config.inc"
[bits 16]
[org STAGE2_BASE]

stage2_16_entry:
    mov [drive_num], dl

    ; --- Enable A20 (fast method port 0x92) ---
    in al, 0x92
    or al, 2
    out 0x92, al

    mov si, msg_start
    call print16

    ; --- Load kernel from disk to KERNEL_TMP (0x10000) via LBA extended read (ah=0x42) ---
    ; Supports >127 sectors by looping 64 at a time
    ; setup DAP
    mov word [dap_seg], 0x1000
    mov word [dap_off], 0
    mov dword [dap_lba], KERNEL_LBA_START
    mov dword [dap_lba+4], 0
    mov word [dap_remain], KERNEL_SECTORS

.read_loop:
    mov ax, [dap_remain]
    test ax, ax
    jz .done
    cmp ax, 64
    jbe .use_ax
    mov ax, 64
.use_ax:
    mov [dap_cnt], ax
    mov ah, 0x42
    mov dl, [drive_num]
    mov si, dap
    int 0x13
    jc disk_err
    ; advance
    movzx eax, word [dap_cnt]
    sub [dap_remain], ax
    add dword [dap_lba], eax
    adc dword [dap_lba+4], 0
    ; advance buffer: offset += ax*512, handle segment overflow
    movzx eax, word [dap_cnt]
    shl eax, 9 ; *512
    add [dap_off], ax
    jnc .no_carry
    ; carry: add 0x1000 to segment (64KB)
    mov ax, [dap_seg]
    add ax, 0x1000
    mov [dap_seg], ax
.no_carry:
    ; also need to handle offset overflow >0xFFFF? already via carry, but offset is 16-bit, add may overflow 0x10000
    ; if offset >= 0x8000*? we already handled carry, but need to normalize if offset >= 0xF000
    jmp .read_loop
.done:

    mov si, msg_loaded
    call print16

    ; --- Force VGA text 80x25 for stable display (VBE 8-bit ready but FB PF via paging - enable via .try_1024 if fixed)
    mov byte [0x9000], 0
    mov byte [0x9001], 0
    jmp .no_vbe
.try_1024:
    mov ax, 0x4F02
    mov bx, 0x4118 ; 1024x768 preferred
    int 0x10
    cmp ax, 0x004F
    je .vbe_ok_1024
.try_720:
    mov ax, 0x4F02
    mov bx, 0x4127 ; 1280x720
    int 0x10
    cmp ax, 0x004F
    je .vbe_ok_720
.try_1080:
    mov ax, 0x4F02
    mov bx, 0x4120 ; 1920x1080 fallback
    int 0x10
    cmp ax, 0x004F
    je .vbe_ok_1080
.try_480:
    mov ax, 0x4F02
    mov bx, 0x4112 ; 640x480
    int 0x10
    cmp ax, 0x004F
    jne .no_vbe
.vbe_ok_480:
    mov byte [0x9000], 0xFB
    mov byte [0x9001], 0x01
    mov word [0x9002], 640
    mov word [0x9004], 480
    jmp .store_bpp
.vbe_ok_1024:
    mov byte [0x9000], 0xFB
    mov byte [0x9001], 0x01
    mov word [0x9002], 1024
    mov word [0x9004], 768
    jmp .store_bpp
.vbe_ok_720:
    mov byte [0x9000], 0xFB
    mov byte [0x9001], 0x01
    mov word [0x9002], 1280
    mov word [0x9004], 720
    jmp .store_bpp
.vbe_ok_1080:
    mov byte [0x9000], 0xFB
    mov byte [0x9001], 0x01
    mov word [0x9002], 1920
    mov word [0x9004], 1080
.store_bpp:
    mov byte [0x9006], 32
    mov word [0x900C], 0
    mov dword [0x9008], 0xE0000000
    ; store VBE PhysBasePtr - use 0x6000 buffer (0x8000 overlaps stage2 at 0x7E00-0xADFF -> reboot loop)
    pusha
    xor ax, ax
    mov es, ax
    mov di, 0x6000
    mov ax, 0x4F01
    mov cx, bx
    int 0x10
    cmp ax, 0x004F
    jne .no_phys
    mov eax, [es:di+40] ; PhysBasePtr
    mov [0x9008], eax
    jmp .phys_done
.no_phys:
    nop
.phys_done:
    popa
    mov si, msg_vbe
    call print16
    jmp .vbe_done
.no_vbe:
    mov dword [0x9008], 0
    mov word [0x900C], 0
    mov si, msg_vga
    call print16
.vbe_done:

    ; --- Setup stack (real mode, below bootloader) ---
    xor ax, ax
    mov ss, ax
    mov sp, 0x7C00

    ; --- Enter protected mode ---
    cli
    lgdt [gdt_desc]
    mov eax, cr0
    or eax, 1
    mov cr0, eax
    jmp GDT_CODE:STAGE32_ENTRY

disk_err:
    mov si, msg_err
    call print16
.halt:
    cli
    hlt
    jmp .halt

print16:
    pusha
.lp: lodsb
    test al, al
    jz .dn
    mov ah, 0x0E
    xor bh, bh
    int 0x10
    jmp .lp
.dn: popa
    ret

; --- Data ---
drive_num: db 0
dap_remain: dw 0
dap:        db 0x10, 0
dap_cnt:    dw 0
dap_off:    dw 0
dap_seg:    dw 0
dap_lba:    dd 0, 0
msg_start:  db "[16] A20 OK, loading kernel...", 13, 10, 0
msg_loaded: db "[16] Kernel at 0x10000, entering PM...", 13, 10, 0
msg_vbe:    db "[16] VBE RGB enabled (480p/1024x768)", 13, 10, 0
msg_vga:    db "[16] VGA text 80x25", 13, 10, 0
msg_err:    db "[16] Disk error!", 13, 10, 0

; --- GDT for 16->32 transition (identity, 4GB) ---
align 4
gdt_start:
    dq 0x0000000000000000                ; null
    dq 0x00CF9A000000FFFF                ; code: base 0, limit 4GB, exec/read, 32-bit, gran 4KB
    dq 0x00CF92000000FFFF                ; data: base 0, limit 4GB, read/write, 32-bit, gran 4KB
gdt_end:
gdt_desc:
    dw gdt_end - gdt_start - 1
    dd gdt_start

; --- Pad to exactly STAGE16_SIZE ---
times STAGE16_SIZE - ($ - $$) db 0
