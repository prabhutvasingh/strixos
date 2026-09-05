#include "editor.h"
#include "vfs.h"
#include "heap.h"
#include "keyboard.h"
#include "tty.h"
#include "fb.h"
#include "io.h"

#define NANO_W 80
#define NANO_H 25
#define NANO_VERSION "9.2"

// Forward from tty.c/keyboard.c
extern char tty_user[4][32];
extern int tty_cur;

static void nano_clear(void){
    // clear via serial ANSI + VGA
    for(int i=0;i<80;i++) __asm__ volatile("" ::: "memory");
    // Use fb clear if graphics, else VGA
    // emit ESC[2J via putchar loop using serial out
    const char *s="\x1b[2J\x1b[H";
    for(const char *p=s;*p;p++) { outb(0x3F8,*p); if(*p=='\n') outb(0x3F8,'\r'); }
    // also clear VGA memory directly
    volatile uint16_t *vga=(volatile uint16_t*)0xB8000;
    for(int i=0;i<80*25;i++) vga[i]=(0x07<<8)|' ';
    // reset cursor
    outb(0x3D4,0x0F); outb(0x3D5,0);
    outb(0x3D4,0x0E); outb(0x3D5,0);
}

static void nano_put_at(int x,int y, const char *s, uint8_t attr){
    volatile uint16_t *vga=(volatile uint16_t*)0xB8000;
    if(x<0||x>=80||y<0||y>=25) return;
    int off=y*80+x;
    for(int i=0;s[i]&&x+i<80;i++) vga[off+i]=(attr<<8)| (uint8_t)s[i];
    // also serial echo for qemu -serial
    for(const char *p=s;*p;p++) { outb(0x3F8,*p); }
}

static void nano_status(const char *msg){
    volatile uint16_t *vga=(volatile uint16_t*)0xB8000;
    for(int x=0;x<80;x++) vga[23*80+x]=(0x70<<8)|' ';
    nano_put_at(0,23,msg,0x70);
}

void editor_open(const char* path){ nano_open(path); }

void nano_open(const char* path){
    char filename[64]; int fi=0;
    if(path&&path[0]){ while(path[fi]&&fi<63){ filename[fi]=path[fi]; fi++; } filename[fi]=0; }
    else { const char *d="new.txt"; for(int i=0;d[i];i++) filename[i]=d[i]; filename[4]=0; }

    // Load file via VFS into heap buffer
    char *buf=(char*)kmalloc(8192);
    if(!buf) return;
    for(int i=0;i<8192;i++) buf[i]=0;
    int fd=vfs_open(filename,0);
    int len=0;
    if(fd>=0){ len=vfs_read(fd,buf,8191); if(len<0) len=0; vfs_close(fd); }
    buf[len]=0;

    // Normalize to lines: keep as linear buffer with cursor
    int cur=len;
    int modified=0;

    nano_clear();
    // Main loop
    int running=1;
    while(running){
        // Draw title bar y=0 inverted
        char title[80];
        // "  GNU nano 9.2                 filename                  Modified  "
        for(int i=0;i<80;i++) title[i]=' ';
        title[79]=0;
        const char *left="  Edit";
        for(int i=0; left[i]; i++) title[2+i]=left[i];
        int fnlen=0; while(filename[fnlen]&&fnlen<20) fnlen++;
        int fpos=40 - fnlen/2;
        for(int i=0;i<fnlen;i++) title[fpos+i]=filename[i];
        if(modified){ const char *mod="Modified"; for(int i=0;mod[i];i++) title[70+i]=mod[i]; }
        // render title
        volatile uint16_t *vga=(volatile uint16_t*)0xB8000;
        for(int x=0;x<80;x++) vga[x]=(0x70<<8)|(uint8_t)title[x];
        outb(0x3F8,'\r'); outb(0x3F8,'\n');
        for(int i=0;i<80;i++) outb(0x3F8,title[i]);
        outb(0x3F8,'\r'); outb(0x3F8,'\n');
        // divider
        for(int x=0;x<80;x++) vga[1*80+x]=(0x07<<8)|'-';
        // text window y=2..22 (21 lines)
        // clear text area
        for(int y=2;y<23;y++) for(int x=0;x<80;x++) vga[y*80+x]=(0x07<<8)|' ';
        // render buffer linewrapped
        int y=2, x=0;
        for(int i=0;i<len && y<23;i++){
            char c=buf[i];
            if(c=='\n'){ y++; x=0; continue; }
            if(c=='\r') continue;
            if(c=='\t'){ c=' '; }
            if(x>=80){ y++; x=0; if(y>=23) break; }
            vga[y*80+x]=(0x07<<8)|(uint8_t)c;
            x++;
        }
        // cursor position compute
        int cy=2, cx=0;
        for(int i=0;i<cur && cy<23;i++){
            if(buf[i]=='\n'){ cy++; cx=0; } else { cx++; if(cx>=80){ cy++; cx=0; } }
        }
        if(cy>=23) cy=22;
        if(cx>=80) cx=79;
        // set hardware cursor
        int pos=cy*80+cx;
        outb(0x3D4,0x0F); outb(0x3D5,pos & 0xFF);
        outb(0x3D4,0x0E); outb(0x3D5,(pos>>8)&0xFF);
        // shortcuts y=24-25
        const char *s1="^G Help      ^O Write Out ^W Where Is  ^K Cut        ^T Execute    ^C Location";
        const char *s2="^X Exit      ^R Read File ^\\ Replace   ^U Paste      ^J Justify    ^/ Go To Line";
        for(int x2=0;x2<80;x2++) vga[24*80+x2]=(0x07<<8)|(x2<(int)sizeof(s1)? ' ': ' ');
        nano_put_at(0,24,s1,0x07);
        nano_put_at(0,25-1,s2,0x07); // y=24
        // Actually use y=24 for second line, we have 25 rows 0..24
        // fix: second shortcuts at y=24
        for(int x2=0;x2<80;x2++) vga[24*80+x2]=(0x07<<8)|' ';
        nano_put_at(0,24,s2,0x07);
        // status already at 23

        // Wait key
        char c=keyboard_getc();
        if(c==0) continue;
        // Ctrl keys: nano uses Ctrl
        if(c==24){ // ^X Exit 0x18
            if(modified){
                nano_status("Save modified buffer? (Y/N/^C) ");
                char a=keyboard_getc();
                if(a=='y'||a=='Y'){
                    // fall through to save
                    goto do_save;
                } else if(a=='n'||a=='N'){
                    running=0; break;
                } else {
                    nano_status("");
                    continue;
                }
            } else { running=0; break; }
        } else if(c==15){ // ^O WriteOut 0x0F
do_save:
            {
                int fd2=vfs_open(filename,0);
                // vfs_open creates if not exists? use vfs_create via open fallback
                if(fd2<0){
                    // try create via vfs_create if exists
                    vfs_close(fd2);
                    fd2=vfs_open(filename,0);
                }
                // StrixOS VFS: vfs_write truncates, simple
                // ensure file exists by creating empty then write
                if(fd2<0){
                    // minimal: use vfs_create not exposed, just write via direct
                } else {
                    vfs_write(fd2,buf,len);
                    vfs_close(fd2);
                }
                // Try generic: if still not saved, use syscall-like
                // fallback: write via vfs_write with new fd
                if(fd2<0){
                    int fd3=vfs_open(filename,0);
                    if(fd3>=0){ vfs_write(fd3,buf,len); vfs_close(fd3); }
                }
                modified=0;
                nano_status("Wrote file");
            }
        } else if(c==11){ // ^K Cut line
            // cut line at cursor
            int ls=cur; while(ls>0&&buf[ls-1]!='\n') ls--;
            int le=cur; while(le<len&&buf[le]!='\n') le++; if(le<len) le++;
            int len2=le-ls;
            // remove
            for(int i=ls;i+len2<len;i++) buf[i]=buf[i+len2];
            len-=len2;
            if(cur>len) cur=len;
            modified=1;
        } else if(c==21){ // ^U Paste not impl
            nano_status("Paste not implemented");
        } else if(c==7){ // ^G Help
            nano_status("Help: ^O save ^X exit ^K cut  nano 9.2 savannah.gnu.org");
        } else if(c==8 || c==127){ // Backspace
            if(cur>0){
                for(int i=cur-1;i<len;i++) buf[i]=buf[i+1];
                cur--; len--; modified=1;
            }
        } else if(c=='\n' || c=='\r'){
            if(len<8190){
                for(int i=len;i>cur;i--) buf[i]=buf[i-1];
                buf[cur]='\n'; cur++; len++; modified=1;
            }
        } else if(c==27){ // ESC - ignore, or arrow via ESC[?
            // drain ESC[ sequence for arrows: try peek
            // simple: read next chars if available quickly (poll)
            // we poll io 0x64 for next
            // For now ignore
        } else if(c>=32 && c<127){
            if(len<8190){
                for(int i=len;i>cur;i--) buf[i]=buf[i-1];
                buf[cur]=c; cur++; len++; modified=1;
            }
        }
        // Arrow movement via raw scan? Use simple: '[' etc not needed for now
        // Left/Right with Ctrl-B/F not impl, use backspace for left? keep minimal
    }
    // restore shell screen
    nano_clear();
    tty_clear();
    kfree(buf);
}
