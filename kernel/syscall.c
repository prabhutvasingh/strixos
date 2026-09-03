#include "syscall.h"
#include "idt.h"
#include "process.h"
#include "heap.h"
#include "io.h"
#include "vfs.h"
#include "fb.h"
#include "keyboard.h"

extern void vga_puts(const char* s);
extern void serial_puts(const char* s);
extern void vga_putchar(char c);

extern struct task* current;
extern void schedule(void);

// syscall handler: r->rax = num, rdi,rsi,rdx = args, return in rax
void syscall_handler(struct regs* r){
    uint64_t num = r->rax;
    uint64_t arg1 = r->rdi;
    uint64_t arg2 = r->rsi;
    uint64_t arg3 = r->rdx;
    long ret = -1;

    switch(num){
        case SYS_WRITE: {
            int fd = (int)arg1;
            const char* buf = (const char*)arg2;
            size_t len = (size_t)arg3;
            if(fd==1 || fd==2){
                for(size_t i=0;i<len;i++){
                    vga_putchar(buf[i]);
                    while((inb(0x3F8+5) & 0x20)==0);
                    outb(0x3F8, buf[i]);
                }
                ret = len;
            } else {
                ret = vfs_write(fd, buf, len);
            }
            break;
        }
        case SYS_READ: {
            int fd = (int)arg1;
            void* buf = (void*)arg2;
            size_t len = (size_t)arg3;
            if(fd==0){
                // accept input from BOTH QEMU window (keyboard) and terminal (serial) when in graphics
                char* b=(char*)buf;
                size_t got=0;
                while(got<len){
                    char c;
                    if(got==0) c=keyboard_getc(); // keyboard_getc already polls both sources
                    else if(!keyboard_try_getc(&c)){
                        if(inb(0x3F8+5)&1) c=inb(0x3F8);
                        else break;
                    }
                    b[got++]=c;
                    if(c=='\n') break;
                    if(got>=len) break;
                    if(got>0 && !keyboard_has_char() && !(inb(0x3F8+5)&1)) break;
                }
                ret = got;
            } else {
                ret = vfs_read(fd, buf, len);
            }
            break;
        }
        case SYS_OPEN: {
            const char* path = (const char*)arg1;
            int flags = (int)arg2;
            ret = vfs_open(path, flags);
            break;
        }
        case SYS_CLOSE: {
            int fd = (int)arg1;
            ret = vfs_close(fd);
            break;
        }
        case SYS_GETPID:
            ret = current ? (long)current->pid : -1;
            break;
        case SYS_YIELD:
            schedule();
            ret = 0;
            break;
        case SYS_EXIT: {
            if(current){
                current->state = 0; // TASK_UNUSED
                schedule();
            }
            ret = 0;
            break;
        }
        case SYS_BRK: {
            // arg1 = new brk, if 0 return current brk
            // For now just use kmalloc break simulation: return heap end
            // Simplify: if arg1==0 return current heap top, else try to extend
            if(arg1==0){
                ret = (long)0x600000; // dummy brk
            } else {
                ret = arg1;
            }
            break;
        }
        default:
            vga_puts("[SYS] unknown "); 
            ret = -1;
            break;
    }
    r->rax = ret;
}

// User wrappers - trigger int 0x80
long sys_write(int fd, const char* buf, size_t len){
    long ret;
    asm volatile("int $0x80" : "=a"(ret) : "a"(SYS_WRITE), "D"((uint64_t)fd), "S"((uint64_t)buf), "d"((uint64_t)len) : "rcx","r11","memory");
    return ret;
}
long sys_getpid(void){
    long ret;
    asm volatile("int $0x80" : "=a"(ret) : "a"(SYS_GETPID) : "rcx","r11","memory");
    return ret;
}
long sys_yield(void){
    long ret;
    asm volatile("int $0x80" : "=a"(ret) : "a"(SYS_YIELD) : "rcx","r11","memory");
    return ret;
}
long sys_exit(int code){
    long ret;
    asm volatile("int $0x80" : "=a"(ret) : "a"(SYS_EXIT), "D"((uint64_t)code) : "rcx","r11","memory");
    return ret;
}
long sys_brk(void* addr){
    long ret;
    asm volatile("int $0x80" : "=a"(ret) : "a"(SYS_BRK), "D"((uint64_t)addr) : "rcx","r11","memory");
    return ret;
}
long sys_read(int fd, void* buf, size_t len){
    long ret;
    asm volatile("int $0x80" : "=a"(ret) : "a"(SYS_READ), "D"((uint64_t)fd), "S"((uint64_t)buf), "d"((uint64_t)len) : "rcx","r11","memory");
    return ret;
}
long sys_open(const char* path, int flags){
    long ret;
    asm volatile("int $0x80" : "=a"(ret) : "a"(SYS_OPEN), "D"((uint64_t)path), "S"((uint64_t)flags) : "rcx","r11","memory");
    return ret;
}
long sys_close(int fd){
    long ret;
    asm volatile("int $0x80" : "=a"(ret) : "a"(SYS_CLOSE), "D"((uint64_t)fd) : "rcx","r11","memory");
    return ret;
}
