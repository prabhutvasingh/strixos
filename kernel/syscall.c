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
        case 1:
        case 5001: { // write
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
        case 0:
        case 5002: { // read
            int fd = (int)arg1;
            void* buf = (void*)arg2;
            size_t len = (size_t)arg3;
            if(fd==0){
                char* b=(char*)buf;
                size_t got=0;
                while(got<len){
                    char c;
                    if(got==0) c=keyboard_getc();
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
        case 2:
        case 5003: { // open
            const char* path = (const char*)arg1;
            int flags = (int)arg2;
            ret = vfs_open(path, flags);
            break;
        }
        case 3:
        case 5004: { // close
            int fd = (int)arg1;
            ret = vfs_close(fd);
            break;
        }
        case 39:
        case 5005: { // getpid
            ret = current ? (long)current->pid : -1;
            break;
        }
        case 1000:
        case 5006: { // yield
            schedule();
            ret = 0;
            break;
        }
        case 60:
        case 231:
        case 5000: { // exit / exit_group
            if(current){
                current->state = 0;
                schedule();
            }
            ret = 0;
            break;
        }
        case 16: { // Linux ioctl - termios for nano
            int fd=(int)arg1; unsigned long req=(unsigned long)arg2; void *argp=(void*)arg3; (void)fd;
            if(req==0x5401){ // TCGETS
                if(argp) for(int i=0;i<64;i++) ((char*)argp)[i]=0;
                ret=0;
            } else if(req==0x5402){ // TCSETS
                ret=0;
            } else if(req==0x5413){ // TIOCGWINSZ
                if(argp){ struct { unsigned short ws_row,ws_col,ws_xpixel,ws_ypixel; } *w=argp; w->ws_row=25; w->ws_col=80; w->ws_xpixel=640; w->ws_ypixel=400; }
                ret=0;
            } else if(req==0x541B){
                ret=0;
            } else {
                ret=-1;
            }
            break;
        }
        case 9: { // Linux mmap
            extern void* kmalloc(size_t);
            size_t sz = arg2 ? arg2 : 4096;
            sz = (sz + 4095) & ~4095;
            void* ptr=kmalloc(sz + 4096);
            if(ptr){
                uintptr_t p = (uintptr_t)ptr;
                p = (p + 4095) & ~4095;
                ret = (long)p;
            } else {
                ret = -1;
            }
            break;
        }
        case 10: // mprotect
        case 11: // munmap
        case 28: // madvise
            ret=0;
            break;
        case 96: { // gettimeofday
            struct { int64_t tv_sec, tv_usec; } *tv = (void*)arg1;
            if(tv){ tv->tv_sec = 0; tv->tv_usec = 0; }
            ret=0;
            break;
        }
        case 318: { // getrandom
            char *buf = (char*)arg1;
            size_t len = (size_t)arg2;
            if(buf){
                for(size_t i=0; i<len; i++) buf[i] = (char)(i * 31 + 7);
            }
            ret = len;
            break;
        }
        case 4: // stat
        case 5: { // Linux fstat
            if(arg2) for(int i=0;i<144;i++) ((char*)arg2)[i]=0;
            // for stat, return -1 ENOENT to force fallback, but also zero buf
            if(num==4){
                // check path for terminfo: if contains "terminfo", return 0 to pretend exists
                const char *p=(const char*)arg1;
                if(p && p[0]){
                    // look for "terminfo" substring
                    int has=0; for(const char *s=p; *s; s++) if(s[0]=='t' && s[1]=='e' && s[2]=='r' && s[3]=='m') has=1;
                    if(has) ret=0; else ret=-2;
                } else ret=-2;
            } else ret=0;
            break;
        }
        case 21: { // access
            const char *p=(const char*)arg1;
            if(p && p[0]){
                int has=0; for(const char *s=p; *s; s++) if(s[0]=='t' && s[1]=='e' && s[2]=='r' && s[3]=='m') has=1;
                ret = has ? 0 : -2;
            } else ret=-2;
            break;
        }
        case 8: { // Linux lseek
            ret=0;
            break;
        }
        case 7: { // Linux poll
            ret=1;
            break;
        }
        case 23: { // select
            ret=1;
            break;
        }
        case 13: // rt_sigaction
        case 14: // rt_sigprocmask
        case 131: // sigaltstack
        case 102: // getuid
        case 104: // getgid
        case 107: // geteuid
        case 108: // getegid
        case 186: // gettid
        case 302: // prlimit64
            ret=0;
            break;
        case 160: { // uname
            struct utsname {
                char sysname[65];
                char nodename[65];
                char release[65];
                char version[65];
                char machine[65];
                char domainname[65];
            } *u = (void*)arg1;
            if(u){
                const char* s = "StrixOS"; for(int i=0;s[i];i++) u->sysname[i]=s[i]; u->sysname[7]=0;
                const char* n = "strix"; for(int i=0;n[i];i++) u->nodename[i]=n[i]; u->nodename[5]=0;
                const char* r = "1.0"; for(int i=0;r[i];i++) u->release[i]=r[i]; u->release[3]=0;
                const char* v = "StrixOS 1.0"; for(int i=0;v[i];i++) u->version[i]=v[i]; u->version[11]=0;
                const char* m = "x86_64"; for(int i=0;m[i];i++) u->machine[i]=m[i]; u->machine[6]=0;
                const char* d = "local"; for(int i=0;d[i];i++) u->domainname[i]=d[i]; u->domainname[5]=0;
            }
            ret=0;
            break;
        }
        case 72: { // fcntl
            ret=0;
            break;
        }
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

        case 257: { // openat
            const char* path=(const char*)arg2;
            int flags=(int)arg3;
            if(path) ret=vfs_open(path, flags);
            else ret=-1;
            if(ret<0 && path && path[0]=='/') ret=vfs_open(path+1, flags);
            break;
        }
        case 17: { // pread64
            ret=-1;
            break;
        }
        case 19: { // readv
            // arg1 fd, arg2 iov, arg3 iovcnt
            // Simplify: if fd 0, do read for first iov
            if(arg1==0 && arg2){
                struct iovec { void *base; size_t len; } *iov=(void*)arg2;
                // try to read into first iov
                char *b=iov[0].base;
                size_t len=iov[0].len;
                if(b && len){
                    // non-blocking check
                    if(!keyboard_has_char() && !(inb(0x3F8+5)&1)){
                        ret=-11; // EAGAIN
                    } else {
                        char c=keyboard_getc();
                        b[0]=c;
                        ret=1;
                    }
                } else ret=0;
            } else ret=-1;
            break;
        }
        case 20: { // writev
            int fd=(int)arg1;
            struct iovec { void *base; size_t len; } *iov=(void*)arg2;
            size_t cnt=(size_t)arg3;
            long total=0;
            for(size_t i=0;i<cnt;i++){
                const char *b=iov[i].base;
                size_t l=iov[i].len;
                if(fd==1||fd==2){
                    for(size_t k=0;k<l;k++){ vga_putchar(b[k]); while((inb(0x3F8+5)&0x20)==0); outb(0x3F8,b[k]); }
                    total+=l;
                } else {
                    long r=vfs_write(fd,b,l);
                    if(r>0) total+=r;
                }
            }
            ret=total;
            break;
        }

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
        case 5007: // SYS_BRK
        {
            static uint64_t cur_brk=0x600000;
            if(arg1==0){
                ret = (long)cur_brk;
            } else {
                if(arg1 > cur_brk && arg1 < 0x40000000){
                    cur_brk = arg1;
                } else if(arg1 < cur_brk){
                    cur_brk = arg1;
                }
                ret = cur_brk;
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
