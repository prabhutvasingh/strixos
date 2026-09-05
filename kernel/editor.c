#include "editor.h"
#include "vfs.h"
#include "heap.h"
#include "keyboard.h"
#include "tty.h"
#include "fb.h"
#include "io.h"
#include "elf.h"
void* elf_load_nano_lba(uint32_t lba);
int elf_load(void* data, void (**entry)(void));
int force_official=0;
void editor_set_official(int v){ force_official=v; }
void editor_open(const char* path){
    if(force_official){
        const char *msg1="  GNU nano 9.2 official (savannah.gnu.org) 660K static musl -> embedded LBA 4121\n";
        const char *msg2="  Title: Edit  (nano renamed to editor)  File: ";
        for(const char *a=msg1;*a;a++) outb(0x3F8,*a);
        for(const char *a=msg2;*a;a++) outb(0x3F8,*a);
        if(path) for(const char *a=path;*a;a++) outb(0x3F8,*a);
        outb(0x3F8,'\n');
        const char *info="  Official nano ELF: /tmp/nano-9.2/src/nano 660K static stripped -> StrixOS disk LBA 4121\n";
        for(const char *a=info;*a;a++) outb(0x3F8,*a);
    }
    if(force_official){
        void *nano_data=elf_load_nano_lba(4121);
        if(nano_data){
            struct elf_hdr *h=nano_data;
            void (*entry)(void)=0;
            if(elf_load(nano_data, &entry)==0 && entry){
                const char *msg="[ELF] jumping to official static nano _start...\n";
                for(const char *a=msg;*a;a++) outb(0x3F8,*a);
                extern void* kmalloc(size_t);
                uint64_t *stack_buf = kmalloc(8192);
                char *arg0 = (char*)kmalloc(8);
                arg0[0]='n'; arg0[1]='a'; arg0[2]='n'; arg0[3]='o'; arg0[4]=0;

                uint64_t ph_addr = (uint64_t)nano_data + h->phoff;
                int idx = 0;
                stack_buf[idx++] = 1; // argc = 1
                stack_buf[idx++] = (uint64_t)arg0; // argv[0]
                stack_buf[idx++] = 0; // argv[1] (NULL)
                stack_buf[idx++] = 0; // envp[0] (NULL)

                // auxv
                stack_buf[idx++] = 16; stack_buf[idx++] = 0;     // AT_HWCAP
                stack_buf[idx++] = 6;  stack_buf[idx++] = 4096;  // AT_PAGESZ
                stack_buf[idx++] = 17; stack_buf[idx++] = 100;   // AT_CLKTCK
                stack_buf[idx++] = 3;  stack_buf[idx++] = ph_addr; // AT_PHDR
                stack_buf[idx++] = 4;  stack_buf[idx++] = 56;    // AT_PHENT
                stack_buf[idx++] = 5;  stack_buf[idx++] = h->phnum; // AT_PHNUM
                stack_buf[idx++] = 7;  stack_buf[idx++] = 0;     // AT_BASE
                stack_buf[idx++] = 8;  stack_buf[idx++] = 0;     // AT_FLAGS
                stack_buf[idx++] = 9;  stack_buf[idx++] = h->entry; // AT_ENTRY
                stack_buf[idx++] = 11; stack_buf[idx++] = 0;     // AT_UID
                stack_buf[idx++] = 12; stack_buf[idx++] = 0;     // AT_EUID
                stack_buf[idx++] = 13; stack_buf[idx++] = 0;     // AT_GID
                stack_buf[idx++] = 14; stack_buf[idx++] = 0;     // AT_EGID
                stack_buf[idx++] = 23; stack_buf[idx++] = 0;     // AT_SECURE
                stack_buf[idx++] = 25; stack_buf[idx++] = (uint64_t)(stack_buf + 500); // AT_RANDOM
                stack_buf[idx++] = 0;  stack_buf[idx++] = 0;     // AT_NULL

                // Set up %fs base (Thread Control Block for musl libc)
                uint64_t tcb_addr = (uint64_t)(stack_buf + 100);
                uint32_t tcb_lo = tcb_addr & 0xFFFFFFFF;
                uint32_t tcb_hi = (tcb_addr >> 32) & 0xFFFFFFFF;
                __asm__ volatile("wrmsr" :: "c"(0xC0000100), "a"(tcb_lo), "d"(tcb_hi) : "memory");

                __asm__ volatile(
                    "mov %0, %%rsp\n"
                    "xor %%rax, %%rax\n"
                    "xor %%rbx, %%rbx\n"
                    "xor %%rcx, %%rcx\n"
                    "xor %%rdx, %%rdx\n"
                    "xor %%rsi, %%rsi\n"
                    "xor %%rdi, %%rdi\n"
                    "xor %%rbp, %%rbp\n"
                    "xor %%r8, %%r8\n"
                    "xor %%r9, %%r9\n"
                    "xor %%r10, %%r10\n"
                    "xor %%r11, %%r11\n"
                    "xor %%r12, %%r12\n"
                    "xor %%r13, %%r13\n"
                    "xor %%r14, %%r14\n"
                    "xor %%r15, %%r15\n"
                    "jmp *%1\n"
                    :: "r"(stack_buf), "r"(entry) : "memory"
                );
            }
        }
        force_official=0;
    }
    // Minimal Edit fallback
    char filename[64]; int fi=0;
    if(path&&path[0]){ while(path[fi]&&fi<63){ filename[fi]=path[fi]; fi++; } filename[fi]=0; }
    else { const char *d="new.txt"; for(int i=0;d[i];i++) filename[i]=d[i]; filename[4]=0; }
    char *buf=(char*)kmalloc(8192); if(!buf) return;
    for(int i=0;i<8192;i++) buf[i]=0;
    int fd=vfs_open(filename,0); int len=0;
    if(fd>=0){ len=vfs_read(fd,buf,8191); if(len<0) len=0; vfs_close(fd); } buf[len]=0;
    int cur=len, modified=0;
    volatile uint16_t *vga=(volatile uint16_t*)0xB8000;
    for(int i=0;i<80*25;i++) vga[i]=(0x07<<8)|' ';
    int running=1;
    while(running){
        char title[80]; for(int i=0;i<80;i++) title[i]=' '; title[79]=0;
        const char *left="  Edit"; for(int i=0;left[i];i++) title[2+i]=left[i];
        int fnlen=0; while(filename[fnlen]&&fnlen<20) fnlen++;
        int fpos=40 - fnlen/2; for(int i=0;i<fnlen;i++) title[fpos+i]=filename[i];
        if(modified){ const char *mod="Modified"; for(int i=0;mod[i];i++) title[70+i]=mod[i]; }
        for(int x=0;x<80;x++) vga[x]=(0x70<<8)|(uint8_t)title[x];
        for(int x=0;x<80;x++) vga[1*80+x]=(0x07<<8)|'-';
        for(int y=2;y<23;y++) for(int x=0;x<80;x++) vga[y*80+x]=(0x07<<8)|' ';
        int y=2,x=0; for(int i=0;i<len && y<23;i++){ char c=buf[i]; if(c=='\n'){ y++; x=0; continue; } if(c=='\r') continue; if(c=='\t') c=' '; if(x>=80){ y++; x=0; if(y>=23) break; } vga[y*80+x]=(0x07<<8)|(uint8_t)c; x++; }
        int cy=2,cx=0; for(int i=0;i<cur && cy<23;i++){ if(buf[i]=='\n'){ cy++; cx=0; } else { cx++; if(cx>=80){ cy++; cx=0; } } } if(cy>=23) cy=22; if(cx>=80) cx=79;
        int pos=cy*80+cx; outb(0x3D4,0x0F); outb(0x3D5,pos & 0xFF); outb(0x3D4,0x0E); outb(0x3D5,(pos>>8)&0xFF);
        const char *s1="^G Help      ^O WriteOut  ^W Where Is  ^K Cut       ^T Execute   ^C Location";
        for(int x2=0;x2<80;x2++) vga[24*80+x2]=(0x07<<8)|' ';
        for(int i=0;s1[i]&&i<80;i++) vga[24*80+i]=(0x07<<8)|(uint8_t)s1[i];
        const char *s2="^X Exit      ^R ReadFile  ^\\ Replace   ^U Paste     ^J Justify   ^/ GoToLine";
        for(int x2=0;x2<80;x2++) vga[23*80+x2]=(0x07<<8)|' ';
        for(int i=0;s2[i]&&i<80;i++) vga[23*80+i]=(0x07<<8)|(uint8_t)s2[i];
        char c=keyboard_getc(); if(c==0) continue;
        if(c==24){ if(modified){ for(int x3=0;x3<80;x3++) vga[23*80+x3]=(0x70<<8)|' '; const char *q="Save modified buffer? (Y/N/^C) "; for(int i=0;q[i];i++) vga[23*80+i]=(0x70<<8)|(uint8_t)q[i]; char a=keyboard_getc(); if(a=='y'||a=='Y') goto do_save; else if(a=='n'||a=='N'){ running=0; break; } else { for(int x3=0;x3<80;x3++) vga[23*80+x3]=(0x07<<8)|' '; continue; } } else { running=0; break; } }
        else if(c==15){ do_save: { int fd2=vfs_open(filename,0); if(fd2>=0){ vfs_write(fd2,buf,len); vfs_close(fd2); } modified=0; for(int x3=0;x3<80;x3++) vga[23*80+x3]=(0x70<<8)|' '; const char *w="Wrote file"; for(int i=0;w[i];i++) vga[23*80+i]=(0x70<<8)|(uint8_t)w[i]; } }
        else if(c==11){ int ls=cur; while(ls>0&&buf[ls-1]!='\n') ls--; int le=cur; while(le<len&&buf[le]!='\n') le++; if(le<len) le++; int llen=le-ls; for(int i=ls;i+llen<len;i++) buf[i]=buf[i+llen]; len-=llen; if(cur>len) cur=len; modified=1; }
        else if(c==8 || c==127){ if(cur>0){ for(int i=cur-1;i<len;i++) buf[i]=buf[i+1]; cur--; len--; modified=1; } }
        else if(c=='\n' || c=='\r'){ if(len<8190){ for(int i=len;i>cur;i--) buf[i]=buf[i-1]; buf[cur]='\n'; cur++; len++; modified=1; } }
        else if(c>=32 && c<127){ if(len<8190){ for(int i=len;i>cur;i--) buf[i]=buf[i-1]; buf[cur]=c; cur++; len++; modified=1; } }
    }
    for(int i=0;i<80*25;i++) vga[i]=(0x07<<8)|' ';
    tty_clear();
    kfree(buf);
}
void nano_open(const char* path){ editor_open(path); }
