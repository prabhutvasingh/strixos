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
        case 0: // Linux read
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
        case 2: // Linux open
        case SYS_OPEN: {
            const char* path = (const char*)arg1;
            int flags = (int)arg2;
            ret = vfs_open(path, flags);
            break;
        }
        case 3: // Linux close
        case SYS_CLOSE: {
            int fd = (int)arg1;
            ret = vfs_close(fd);
            break;
        }
        case 39: // Linux getpid
        case SYS_GETPID:
            ret = current ? (long)current->pid : -1;
            break;
        case SYS_YIELD:
            schedule();
            ret = 0;
            break;
        case 60: // Linux exit
        case SYS_EXIT: {
            if(current){
                current->state = 0; // TASK_UNUSED
                schedule();
            }
            ret = 0;
            break;
        }
        case 16: { // Linux ioctl - termios for nano
            int fd=(int)arg1; unsigned long req=(unsigned long)arg2; void *argp=(void*)arg3; (void)fd;
            if(req==0x5401){ // TCGETS
                // fill termios with zeros
                if(argp) for(int i=0;i<64;i++) ((char*)argp)[i]=0;
                ret=0;
            } else if(req==0x5402){ // TCSETS
                ret=0;
            } else if(req==0x5413){ // TIOCGWINSZ
                if(argp){ struct { unsigned short ws_row,ws_col,ws_xpixel,ws_ypixel; } *w=argp; w->ws_row=25; w->ws_col=80; w->ws_xpixel=640; w->ws_ypixel=400; }
                ret=0;
            } else if(req==0x541B){ // FIONREAD?
                ret=0;
            } else {
                ret=0;
            }
            break;
        }
        case 9: { // Linux mmap
            // arg1 addr, arg2 len, arg3 prot, arg4 flags from r10, arg5 fd from r8, arg6 offset from r9 - but we only have 3 args in int80, so use brk-like
            // Simplify: return heap alloc
            // Use kmalloc for len
            extern void* kmalloc(size_t);
            void* ptr=kmalloc(arg2);
            ret=ptr? (long)ptr : -1;
            break;
        }
        case 11: { // Linux munmap
            ret=0;
            break;
        }
        case 5: { // Linux fstat
            // arg1 fd, arg2 statbuf
            if(arg2) for(int i=0;i<144;i++) ((char*)arg2)[i]=0;
            ret=0;
            break;
        }
        case 8: { // Linux lseek
            ret=0;
            break;
        }
        case 7: { // Linux poll
            // arg1 fds, arg2 nfds, arg3 timeout
            // Simulate input ready
            ret=1;
            break;
        }
        case 23: { // select
            ret=1;
            break;
        }
        case 13: // rt_sigaction
        case 14: // rt_sigprocmask
            ret=0;
            break;
        case 72: // fcntl
            ret=0;
            break;
        case 158: { // arch_prctl
            int code=(int)arg1;
            void *addr=(void*)arg2;
            // log
            vga_puts("[SYS] arch_prctl "); char hex[]="0123456789ABCDEF"; for(int i=28;i>=0;i-=4){ char c[2]={hex[((uint64_t)code>>i)&0xF],0}; vga_puts(c);} vga_puts(" "); for(int i=60;i>=0;i-=4){ char c[2]={hex[((uint64_t)addr>>i)&0xF],0}; vga_puts(c);} vga_puts("\n");
            for(const char *a="[SYS] arch_prctl ";*a;a++) outb(0x3F8,*a);
            for(int i=28;i>=0;i-=4) outb(0x3F8,hex[((uint64_t)code>>i)&0xF]);
            outb(0x3F8,' ');
            for(int i=60;i>=0;i-=4) outb(0x3F8,hex[((uint64_t)addr>>i)&0xF]);
            outb(0x3F8,'\n');
            if(code==0x1002){ // ARCH_SET_FS
                uint64_t base=(uint64_t)addr;
                // wrmsr 0xC0000100
                uint32_t lo=base & 0xFFFFFFFF;
                uint32_t hi=(base>>32) & 0xFFFFFFFF;
                __asm__ volatile("wrmsr" :: "c"(0xC0000100), "a"(lo), "d"(hi) : "memory");
                ret=0;
            } else if(code==0x1003){ // ARCH_GET_FS
                uint32_t lo,hi;
                __asm__ volatile("rdmsr" : "=a"(lo), "=d"(hi) : "c"(0xC0000100) : "memory");
                uint64_t base=((uint64_t)hi<<32)|lo;
                if(arg2) *(uint64_t*)arg2=base;
                ret=0;
            } else {
                ret=0;
            }
            break;
        }
        case 218: // set_tid_address
            ret=current?current->pid:1;
            break;
        case 270: // pselect6
        case 271: { // ppoll
            ret=1;
            break;
        }
        case 273: // set_robust_list
        case 334: // rseq
            ret=0;
            break;
        case 231: // exit_group
            if(current){ current->state=0; schedule(); }
            ret=0;
            break;
        case 257: { // openat
            // arg1 dirfd, arg2 path, arg3 flags
            const char* path=(const char*)arg2;
            int flags=(int)arg3;
            // ignore dirfd, handle absolute or relative
            if(path) ret=vfs_open(path, flags);
            else ret=-1;
            // if vfs_open fails try without leading /
            if(ret<0 && path && path[0]=='/') ret=vfs_open(path+1, flags);
            break;
        }
        case 17: { // pread64
            ret=-1;
            break;
        }
        case 19: // readv
        case 20: { // writev
            ret=-1;
            break;
        }
        case 10: // mprotect
        case 28: // madvise
            ret=0;
            break;
        case 35: // nanosleep
            ret=0;
            break;
        case 63: { // uname
            if(arg1) for(int i=0;i<390;i++) ((char*)arg1)[i]=0;
            ret=0;
            break;
        }
        case 202: // futex
            ret=0;
            break;
        case 228: { // clock_gettime
            if(arg2){
                // struct timespec { long tv_sec, tv_nsec; }
                *(long*)arg2=0;
                *((long*)arg2+1)=0;
            }
            ret=0;
            break;
        }
        case 12: // Linux brk
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
        default: {
            vga_puts("[SYS] unknown "); 
            char hex[]="0123456789ABCDEF";
            for(int i=28;i>=0;i-=4){ char c[2]={hex[(num>>i)&0xF],0}; vga_puts(c); }
            vga_puts(" ");
            // also serial
            const char *m="[SYS] unknown "; for(const char *a=m;*a;a++) outb(0x3F8,*a);
            for(int i=28;i>=0;i-=4){ char c=hex[(num>>i)&0xF]; outb(0x3F8,c); }
            outb(0x3F8,'\n');
            ret = -1;
            break;
        }
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
