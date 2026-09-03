; =============================================================================
; Stage 2 Bootloader - Flat Binary
; Loaded at 0x7E00 by Stage 1
;
; Flow:
;   1. 16-bit real mode: Enable A20, load kernel from disk
;   2. Enter 32-bit protected mode
;   3. Set up page tables for long mode
;   4. Enter 64-bit long mode
;   5. Jump to kernel at 0x100000
; =============================================================================

; =============================================================================
; 16-BIT REAL MODE
; =============================================================================
[bits 16]

kernel_offset   equ 0x100000      ; Load kernel at 1MB
kernel_sectors  equ 100           ; Max kernel size: 50KB
stack_top       equ 0x90000       ; Temporary stack

stage2_entry:
    ; Save boot drive from Stage 1
    mov [drive_num], dl

    ; --- Enable A20 line (fast method) ---
    in al, 0x92
    or al, 2
    out 0x92, al

    ; --- Print message (16-bit BIOS) ---
    mov si, .msg_start
    call .print16

    ; --- Load kernel from disk ---
    ; Read kernel_sectors sectors starting at sector 33
    ; into memory at 0x10000 (0x1000:0x0000)
    mov ax, 0x1000
    mov es, ax
    mov bx, 0x0000                 ; ES:BX = 0x1000:0x0000 = 0x10000

    mov al, kernel_sectors
    mov ch, 0                      ; Cylinder 0
    mov cl, 33                     ; Start sector (after Stage 1+2)
    mov dh, 0                      ; Head 0
    mov dl, [drive_num]
    mov ah, 0x02                   ; BIOS: read sectors
    int 0x13
    jc .disk_err

    mov si, .msg_loaded
    call .print16

    ; --- Set up stack ---
    xor ax, ax
    mov ss, ax
    mov sp, stack_top

    ; --- Enter protected mode ---
    cli
    lgdt [gdt_desc]
    mov eax, cr0
    or eax, 1
    mov cr0, eax

    ; Far jump to 32-bit code segment
    jmp 0x08:pm32_entry

.disk_err:
    mov si, .msg_err
    call .print16
.halt16:
    cli
    hlt
    jmp .halt16

; --- 16-bit print function ---
.print16:
    pusha
.lp16:
    lodsb
    test al, al
    jz .dn16
    mov ah, 0x0E
    xor bh, bh
    int 0x10
    jmp .lp16
.dn16:
    popa
    ret

; --- 16-bit Data ---
drive_num:  db 0
.msg_start: db "[Stage2] A20 enabled, loading kernel...", 13, 10, 0
.msg_loaded:db "[Stage2] Kernel loaded, entering protected mode...", 13, 10, 0
.msg_err:   db "[ERROR] Disk read failed!", 13, 10, 0

; =============================================================================
; GDT (used by both 16->32 and 32->64 transitions)
; =============================================================================
align 4
gdt_start:
    ; Null descriptor
    dq 0x0000000000000000

    ; Code segment 0x08: Base=0, Limit=4GB, 64-bit capable
    dw 0xFFFF       ; Limit 0-15
    dw 0x0000       ; Base 0-15
    db 0x00         ; Base 16-23
    db 10011010b    ; Access: P=1, DPL=0, Type=Code, Readable
    db 10101111b    ; Flags: L=1 (64-bit), G=1 (4KB), Limit 16-19
    db 0x00         ; Base 24-31

    ; Data segment 0x10: Base=0, Limit=4GB, 64-bit capable
    dw 0xFFFF
    dw 0x0000
    db 0x00
    db 10010010b    ; Access: P=1, DPL=0, Type=Data, Writable
    db 11001111b    ; Flags: L=1 (64-bit), G=1 (4KB), Limit 16-19
    db 0x00
gdt_end:

gdt_desc:
    dw gdt_end - gdt_start - 1
    dd gdt_start

; =============================================================================
; 32-BIT PROTECTED MODE
; =============================================================================
[bits 32]

pm32_entry:
    ; Load data segment registers
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov esp, stack_top

    ; --- Print to VGA (0xB8000) ---
    mov edi, 0xB8000
    mov esi, .msg_pm
    call .print32

    ; ========================================
    ; Set up page tables for long mode
    ; Identity map first 2MB + higher-half at 0xFFFFFFFF80000000
    ; ========================================

    ; Zero out 4 pages at 0x1000 (PML4, PDPT, PD, spare)
    mov edi, 0x1000
    xor eax, eax
    mov ecx, 4096                  ; 4 pages * 1024 dwords = 16384 bytes
    rep stosd

    ; PML4[0] -> PDPT at 0x2000 (identity map)
    mov dword [0x1000], 0x2003     ; Present | Read/Write
    ; PML4[511] -> PDPT at 0x2000 (higher-half: PML4 index 511 = bits 48-57 of virtual addr)
    mov dword [0x1FF8], 0x2003     ; Present | Read/Write

    ; PDPT[0] -> PD at 0x3000 (identity map)
    mov dword [0x2000], 0x3003     ; Present | Read/Write
    ; PDPT[510] -> PD at 0x3000 (higher-half: PDPT index 510 = bits 39-47)
    mov dword [0x1FF0], 0x3003     ; Present | Read/Write

    ; PD[0] -> 2MB page at physical 0 (identity map)
    mov dword [0x3000], 0x0083     ; Present | Read/Write | PageSize (2MB)
    ; PD[510] -> 2MB page at physical 0 (higher-half: PD index 510 = bits 30-38)
    ; Virtual: 0xFFFFFFFF80000000 -> Physical: 0x0
    mov dword [0x3000 + 510*4], 0x0083

    ; Load PML4 into CR3
    mov eax, 0x1000
    mov cr3, eax

    ; Enable PAE (CR4 bit 5)
    mov eax, cr4
    or eax, (1 << 5)
    mov cr4, eax

    ; Enable long mode (IA32_EFER MSR bit 8)
    mov ecx, 0xC0000080
    rdmsr
    or eax, (1 << 8)
    wrmsr

    ; Enable paging (CR0 bit 31) - also enables protected mode
    mov eax, cr0
    or eax, (1 << 31)
    mov cr0, eax

    ; We are now in compatibility mode. Jump to 64-bit code.
    jmp 0x08:lm64_entry

; --- 32-bit print function ---
.print32:
    pusha
.lp32:
    lodsb
    test al, al
    jz .dn32
    mov ah, 0x0F                   ; White on black
    mov [edi], ax
    add edi, 2
    jmp .lp32
.dn32:
    popa
    ret

; --- 32-bit Data ---
.msg_pm: db "[Stage2] 32-bit protected mode active", 0

; =============================================================================
; 64-BIT LONG MODE
; =============================================================================
[bits 64]

lm64_entry:
    ; Load 64-bit data segment
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov rsp, stack_top

    ; --- Clear screen ---
    mov rdi, 0xB8000
    mov rax, 0x0F200F200F200F20   ; 4 spaces, white on black
    mov rcx, 520                   ; 80*25*2/8 = 520 qwords = 4160 bytes... but 80*25=2000 entries * 2 = 4000 bytes
    rep stosq

    ; --- Print banner to VGA ---
    mov rdi, 0xB8000
    mov rsi, .msg_banner
    call .vga_print64

    ; --- Jump to kernel at 0x100000 ---
    mov rax, kernel_offset
    jmp rax

.halt64:
    cli
    hlt
    jmp .halt64

; --- 64-bit VGA print ---
.vga_print64:
    push rax
    push rcx
    mov rcx, rdi
.lp64:
    lodsb
    test al, al
    jz .dn64
    mov ah, 0x0F
    mov [rcx], ax
    add rcx, 2
    jmp .lp64
.dn64:
    pop rcx
    pop rax
    ret

; --- 64-bit Data ---
.msg_banner: db "[Stage2] 64-bit long mode active - jumping to kernel...", 0

; =============================================================================
; Pad to exactly 32 sectors (16384 bytes)
; =============================================================================
times 16384 - ($ - $$) db 0
