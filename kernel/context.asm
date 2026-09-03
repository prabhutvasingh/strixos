; context.asm - task switch for x86_64
[bits 64]

global context_switch
; void context_switch(uint64_t* old_rsp, uint64_t new_rsp)
; old_rsp: pointer to store current rsp
; new_rsp: value to load
context_switch:
    push rbp
    push rbx
    push r12
    push r13
    push r14
    push r15
    pushfq

    mov [rdi], rsp      ; *old_rsp = rsp
    mov rsp, rsi        ; rsp = new_rsp

    popfq
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    pop rbp
    ret

; helper for initial task entry
global task_trampoline
extern task_exit
task_trampoline:
    pop rdi
    call rdi
    call task_exit
.halt: hlt
    jmp .halt
