; =============================================================================
; Kernel Entry Point - 64-bit Long Mode
; This is jumped to by the bootloader at 0x100000
; =============================================================================

[bits 64]
section .text

; External C main function
extern kmain

global _start
_start:
    ; Set up stack (16KB at 0x90000, growing downward)
    mov rsp, 0x90000

    ; Save bootloader-provided info (if any)
    ; RDI, RSI are available from bootloader

    ; Clear BSS section
    extern __bss_start
    extern __bss_end
    mov rdi, __bss_start
    mov rcx, __bss_end
    sub rcx, rdi
    xor al, al
    rep stosb

    ; Jump to kernel main
    call kmain

    ; If kmain returns, halt
.halt:
    cli
    hlt
    jmp .halt
