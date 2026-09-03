; =============================================================================
; ISR stubs - 256 entries for x86_64 IDT
; Each stub pushes int_no and err_code then jumps to common handler
; =============================================================================
[bits 64]

extern isr_handler

; Common ISR handler: saves regs, calls C, restores, iretq
global isr_common
isr_common:
    ; Save general registers
    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push rbp
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    mov rdi, rsp        ; first arg = struct regs*
    call isr_handler

    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rbp
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax

    ; Remove int_no and err_code pushed by stub
    add rsp, 16
    iretq

; --- Macro for ISR without error code (push dummy 0) ---
%macro ISR_NOERR 1
global isr%1
isr%1:
    push qword 0        ; dummy err_code
    push qword %1       ; int_no
    jmp isr_common
%endmacro

; --- Macro for ISR with error code (CPU pushes err) ---
%macro ISR_ERR 1
global isr%1
isr%1:
    ; err_code already pushed by CPU
    push qword %1
    jmp isr_common
%endmacro

; CPU exceptions 0-31
ISR_NOERR 0
ISR_NOERR 1
ISR_NOERR 2
ISR_NOERR 3
ISR_NOERR 4
ISR_NOERR 5
ISR_NOERR 6
ISR_NOERR 7
ISR_ERR   8
ISR_NOERR 9
ISR_ERR   10
ISR_ERR   11
ISR_ERR   12
ISR_ERR   13
ISR_ERR   14
ISR_NOERR 15
ISR_NOERR 16
ISR_ERR   17
ISR_NOERR 18
ISR_NOERR 19
ISR_NOERR 20
ISR_NOERR 21
ISR_NOERR 22
ISR_NOERR 23
ISR_NOERR 24
ISR_NOERR 25
ISR_NOERR 26
ISR_NOERR 27
ISR_NOERR 28
ISR_NOERR 29
ISR_ERR   30
ISR_NOERR 31

; IRQs 32-47 (remapped PIC) - no error
%assign i 32
%rep 16
ISR_NOERR i
%assign i i+1
%endrep

; Remaining 48-255
%assign i 48
%rep 208
ISR_NOERR i
%assign i i+1
%endrep

; Export table of ISR addresses
global isr_stub_table
isr_stub_table:
%assign i 0
%rep 256
    dq isr %+ i
%assign i i+1
%endrep
