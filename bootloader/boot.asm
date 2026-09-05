; =============================================================================
; Stage 1 Bootloader - MBR (512 bytes)
; Loaded by BIOS at 0x7C00 in 16-bit real mode
; Loads Stage 2 from disk sectors 1-32
; =============================================================================

[bits 16]
[org 0x7C00]

STAGE2_OFFSET equ 0x7E00          ; Where Stage 2 is loaded in memory
STAGE2_SECTORS equ 24             ; 3*4096 /512 = 24 sectors for split Stage2
KERNEL_OFFSET equ 0x100000        ; Where kernel will be loaded (1MB mark)

start:
    ; Set up segments and stack
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00                 ; Stack grows down from 0x7C00
    sti

    ; Save boot drive number (BIOS passes it in DL)
    mov [boot_drive], dl

    ; Print boot message
    mov si, msg_boot
    call print_string

    ; ============================================================
    ; Load Stage 2 from disk (sectors 1-24) to STAGE2_OFFSET
    ; ============================================================
    mov bx, STAGE2_OFFSET          ; ES:BX = destination buffer
    mov al, STAGE2_SECTORS         ; Number of sectors to read
    mov ch, 0                      ; Cylinder 0
    mov cl, 2                      ; Start from sector 2 (sector 1 = MBR)
    mov dh, 0                      ; Head 0
    mov dl, [boot_drive]           ; Drive number
    mov ah, 0x02                   ; BIOS function: read sectors
    int 0x13                       ; Call BIOS disk interrupt
    jc disk_error                  ; Jump if carry flag set (error)

    ; Verify we loaded the right amount
    cmp al, STAGE2_SECTORS
    jne disk_error

    print_string_loaded:
    mov si, msg_stage2
    call print_string

    ; Jump to Stage 2
    jmp 0x0000:STAGE2_OFFSET

; =============================================================================
; Subroutines
; =============================================================================

print_string:
    pusha
.loop:
    lodsb
    or al, al
    jz .done
    mov ah, 0x0E                   ; BIOS teletype function
    mov bh, 0
    int 0x10
    jmp .loop
.done:
    popa
    ret

disk_error:
    push ax
    mov si, msg_disk_err
    call print_string
    pop ax
    push ax
    mov al, ah
    shr al, 4
    call print_hex_nibble
    pop ax
    mov al, ah
    and al, 0x0F
    call print_hex_nibble
    mov si, msg_newline
    call print_string
.halt:
    cli
    hlt
    jmp .halt

print_hex_nibble:
    and al, 0x0F
    add al, '0'
    cmp al, '9'
    jle .store
    add al, 7
.store:
    mov ah, 0x0E
    mov bh, 0
    int 0x10
    ret

; =============================================================================
; Data
; =============================================================================

boot_drive: db 0
msg_boot:     db "[Stage1] Loading Stage 2...", 13, 10, 0
msg_stage2:   db "[Stage2] Loaded, jumping...", 13, 10, 0
msg_disk_err: db "[ERROR] Disk read failed! AH=", 0
msg_newline:  db 13, 10, 0

align 4
disk_packet:
    db 0x10          ; Size of packet (16 bytes)
    db 0             ; Reserved (0)
    dw STAGE2_SECTORS; Number of sectors to read
    dw STAGE2_OFFSET ; Destination offset (0x7E00)
    dw 0x0000        ; Destination segment (0x0000)
    dq 1             ; Starting LBA sector (1)

; =============================================================================
; Pad to 510 bytes and add boot signature
; =============================================================================

times 510 - ($ - $$) db 0
dw 0xAA55                        ; Boot signature
