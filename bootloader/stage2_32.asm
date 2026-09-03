; =============================================================================
; stage2_32.asm - 32-bit protected mode portion
; Loaded at 0x8E00 (STAGE32_ENTRY), 4096 bytes padded
; - Copy kernel from bounce buffer 0x10000 to 0x100000
; - Setup page tables, enable PAE + long mode
; - Far jump to stage2_64
; =============================================================================
%include "bootloader/boot_config.inc"
[bits 32]
[org STAGE32_ENTRY]

stage2_32_entry:
    ; Load data segments
    mov ax, GDT_DATA
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov esp, STACK_TOP

    ; --- Copy kernel 0x10000 -> 0x100000 ---
    ; KERNEL_SECTORS * 512 bytes
    mov esi, KERNEL_TMP
    mov edi, KERNEL_OFFSET
    mov ecx, (KERNEL_SECTORS * 512) / 4
    rep movsd

    ; VGA msg
    mov esi, msg_pm
    mov edi, 0xB8000
    call print32

    ; --- Setup page tables (4 pages at 0x1000) ---
    ; Clear 16KB at 0x1000
    mov edi, 0x1000
    xor eax, eax
    mov ecx, 4096
    rep stosd

    ; PML4[0] -> PDPT 0x2000 ; PML4[511] -> PDPT 0x2000 (higher-half)
    mov dword [0x1000 + 0*8], 0x2003
    mov dword [0x1000 + 511*8], 0x2003

    ; PDPT[0] -> PD 0x3000 ; PDPT[510] -> PD 0x3000
    mov dword [0x2000 + 0*8], 0x3003
    mov dword [0x2000 + 510*8], 0x3003

    ; Map 0-64MB identity via 2MB pages (32 entries)
    mov ecx, 0
.map_loop:
    mov eax, ecx
    shl eax, 21                 ; 2MB * ecx
    or eax, 0x83                ; P | RW | PS
    mov dword [0x3000 + ecx*8], eax
    mov dword [0x3000 + ecx*8 +4], 0
    inc ecx
    cmp ecx, 32
    jb .map_loop

    ; Higher-half: map 0xFFFFFFFF80000000 -> 0 (first 2MB) for kernel higher-half
    mov dword [0x3000 + 510*8], 0x83
    mov dword [0x3000 + 510*8 +4], 0

    ; --- Map VBE framebuffer 0xE0000000 (up to 10MB for 1080p) for RGB any display ---
    ; PML4[0] already -> PDPT 0x2000, add PDPT[3] -> PD 0x4000 for 3GB+
    mov dword [0x2000 + 3*8], 0x4003
    mov dword [0x2000 + 3*8 +4], 0
    ; PD 0x4000: 256=0xE0000000, 257=0xE0200000, 258=0xE0400000, 259=0xE0600000, 260=0xE0800000 (10MB)
    mov dword [0x4000 + 256*8], 0xE0000083
    mov dword [0x4000 + 256*8 +4], 0
    mov dword [0x4000 + 257*8], 0xE0200083
    mov dword [0x4000 + 257*8 +4], 0
    mov dword [0x4000 + 258*8], 0xE0400083
    mov dword [0x4000 + 258*8 +4], 0
    mov dword [0x4000 + 259*8], 0xE0600083
    mov dword [0x4000 + 259*8 +4], 0
    mov dword [0x4000 + 260*8], 0xE0800083
    mov dword [0x4000 + 260*8 +4], 0

    mov eax, 0x1000
    mov cr3, eax

    ; Enable PAE
    mov eax, cr4
    or eax, (1 << 5)
    mov cr4, eax

    ; Enable LME (EFER bit 8)
    mov ecx, 0xC0000080
    rdmsr
    or eax, (1 << 8)
    wrmsr

    ; Enable paging
    mov eax, cr0
    or eax, (1 << 31)
    mov cr0, eax

    ; Jump to 64-bit (needs 64-bit GDT, reuse same GDT with L bit set)
    ; Reload GDT with 64-bit descriptors
    lgdt [gdt64_desc]
    jmp GDT_CODE:STAGE64_ENTRY

print32:
    pusha
.lp: lodsb
    test al, al
    jz .dn
    mov ah, 0x0F
    mov [edi], ax
    add edi, 2
    jmp .lp
.dn: popa
    ret

msg_pm: db "[32] Protected mode, paging setup -> long mode", 0

align 4
gdt64_start:
    dq 0x0000000000000000
    dq 0x00209A0000000000    ; code 64-bit: L=1, D=0
    dq 0x0000920000000000    ; data 64-bit
gdt64_end:
gdt64_desc:
    dw gdt64_end - gdt64_start - 1
    dd gdt64_start

times STAGE32_SIZE - ($ - $$) db 0
