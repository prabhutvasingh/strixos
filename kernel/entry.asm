; =============================================================================
; Kernel Entry Point - 64-bit Long Mode
; This is jumped to by the bootloader at 0x100000
; =============================================================================

[bits 64]
section .text

; External C main function
extern kmain

global _start
global boot_info_ptr
section .data
boot_info_ptr: dq 0
section .text
_start:
    ; Set up stack (16KB at 0x90000, growing downward)
    mov rsp, 0x90000

    ; Save bootloader-provided bootinfo pointer (RDI = 0x6000 on UEFI)
    mov [rel boot_info_ptr], rdi

    ; Enable SSE for user binaries (nano uses SSE)
    mov rax, cr0
    and ax, 0xFFFB
    or ax, 0x22
    mov cr0, rax
    mov rax, cr4
    or ax, 0x600
    mov cr4, rax
    fninit

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
