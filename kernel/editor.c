#include "editor.h"
#include "vfs.h"
#include "heap.h"
#include "keyboard.h"
#include "tty.h"
#include "fb.h"
#include "syscall.h"

int force_official = 0;
void editor_set_official(int v){ (void)v; force_official = 0; }

static void e_put(const char* s){ size_t l=0; while(s[l]) l++; if(l) sys_write(1,s,l); }
static void e_ch(char c){ sys_write(1,&c,1); }

void editor_open(const char* path){
    force_official = 0;

    char filename[64];
    size_t fi = 0;
    if(path && path[0]){
        while(path[fi] && fi < 63){ filename[fi]=path[fi]; fi++; }
        filename[fi]=0;
        if(fi==0){ const char *d="new.txt"; for(int i=0;d[i];i++) filename[i]=d[i]; filename[7]=0; }
        if(filename[0]=='/'){ size_t k=0; while(filename[k]){filename[k]=filename[k+1];k++;} }
    } else {
        const char *d="new.txt"; for(int i=0;d[i];i++) filename[i]=d[i]; filename[7]=0;
    }

    char *buf = (char*)kmalloc(8192);
    if(!buf) return;
    for(int i=0;i<8192;i++) buf[i]=0;
    int len = 0;
    int fd = vfs_open(filename, 0);
    if(fd >= 0){ long r = vfs_read(fd, buf, 8191); if(r > 0) len = (int)r; vfs_close(fd); }
    buf[len] = 0;
    int cur = len, modified = 0, running = 1;

    while(running){
        // redraw via ANSI so it works on serial + framebuffer TTY
        e_put("\x1b[2J\x1b[H");
        e_put("Strix Write - ");
        e_put(filename);
        if(modified) e_put(" (Modified)");
        e_put("  | ^O save  ^X exit  ^K cut line\n");
        e_put("---------------------------------------------------------------\n");
        // print buffer
        if(len > 0) sys_write(1, buf, len);
        e_put("\n---------------------------------------------------------------\n");
        e_put("Type to write. Backspace deletes. Enter = new line. Arrows move.\n");

        char c = keyboard_getc();
        if(c == 0) continue;
        if(c == 27){ // ESC sequence (arrows from serial/QEMU)
            char n1 = keyboard_getc();
            if(n1 == '['){
                char n2 = keyboard_getc();
                if(n2 == 'D' && cur > 0) cur--;         // left
                else if(n2 == 'C' && cur < len) cur++;  // right
            } else {
                running = 0; // plain ESC exits
            }
            continue;
        }
        if(c == 24){ // ^X exit
            if(modified){
                e_put("Save before exit? (Y/N): ");
                char a = keyboard_getc();
                e_ch(a); e_put("\n");
                if(a=='y'||a=='Y') goto do_save;
                else if(a=='n'||a=='N'){ running=0; break; }
                else continue;
            } else { running=0; break; }
        }
        else if(c == 15){ // ^O save
        do_save:
            if(0==vfs_save(filename, buf, len)){ modified=0; e_put("Saved!\n"); }
            else e_put("Save FAILED!\n");
            // pause so user sees message
            for(volatile int w=0; w<2000000; w++) asm volatile("pause");
        }
        else if(c == 11){ // ^K cut line
            int ls=cur; while(ls>0&&buf[ls-1]!='\n') ls--;
            int le=cur; while(le<len&&buf[le]!='\n') le++; if(le<len) le++;
            int llen=le-ls;
            for(int i=ls;i+llen<len;i++) buf[i]=buf[i+llen];
            len-=llen; if(cur>len) cur=len; modified=1;
        }
        else if(c == 8 || c == 127){ // backspace
            if(cur>0){ for(int i=cur-1;i<len;i++) buf[i]=buf[i+1]; cur--; len--; modified=1; }
        }
        else if(c == '\n' || c == '\r'){
            if(len<8190){ for(int i=len;i>cur;i--) buf[i]=buf[i-1]; buf[cur]='\n'; cur++; len++; modified=1; }
        }
        else if(c >= 32 && c < 127){
            if(len<8190){ for(int i=len;i>cur;i--) buf[i]=buf[i-1]; buf[cur]=c; cur++; len++; modified=1; }
        }
    }
    kfree(buf);
    tty_clear();
    e_put("\x1b[2J\x1b[H");
}

void nano_open(const char* path){ editor_open(path); }
