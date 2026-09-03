; =============================================================================
; stage2_64.asm - 64-bit long mode portion
; Loaded at 0x9E00 (STAGE64_ENTRY), 4096 bytes padded
; - Setup 64-bit segments/stack, clear VGA, jump to kernel 0x100000
; =============================================================================
%include "bootloader/boot_config.inc"
[bits 64]
[org STAGE64_ENTRY]

stage2_64_entry:
    mov ax, GDT_DATA
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov rsp, STACK_TOP

    ; Clear VGA 80x25
    mov rdi, 0xB8000
    mov rax, 0x0F200F200F200F20
    mov rcx, 500  ; 4000/8
    rep stosq

    mov rdi, 0xB8000
    mov rsi, msg_lm
    call vga_print

    mov rax, KERNEL_OFFSET
    jmp rax

.halt: cli
    hlt
    jmp .halt

vga_print:
    push rax
    push rcx
    mov rcx, rdi
.lp: lodsb
    test al, al
    jz .dn
    mov ah, 0x0F
    mov [rcx], ax
    add rcx, 2
    jmp .lp
.dn: pop rcx
    pop rax
    ret

msg_lm: db "[64] Long mode active -> kernel", 0

times STAGE64_SIZE - ($ - $$) db 0
