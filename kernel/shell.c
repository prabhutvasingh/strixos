#include "shell.h"
#include "syscall.h"
#include "vfs.h"
#include "io.h"
#include "process.h"
#include "fat.h"
#include "elf.h"
#include "net.h"
#include "module.h"
#include "editor.h"
#include "coreutils_port.h"
#include "fb.h"
#include "tty.h"
#include "keyboard.h"

extern void vga_puts(const char* s);
extern void vga_putchar(char c);
extern void vga_clear(void);
extern struct task tasks[];

#define MAX_LINE 128
#define HIST_SIZE 32

static int serial_data_ready(void){ return inb(0x3F8+5) & 1; }
static __attribute__((unused)) char serial_getc(void){ while(!serial_data_ready()); return inb(0x3F8); }
static int serial_getc_nb(char *out){ if(!serial_data_ready()) return 0; *out = inb(0x3F8); return 1; }

static void swrite(const char* s, size_t n){ sys_write(1,s,n); }
static void sput(const char* s){ size_t l=0; while(s[l]) l++; if(l) swrite(s,l); }
static size_t kstrlen(const char* s){ size_t l=0; while(s[l]) l++; return l; }
static int kstrcmp(const char* a,const char* b){ while(*a && *a==*b){a++;b++;} return (unsigned char)*a-(unsigned char)*b; }
static int kcasecmp(const char* a,const char* b){ while(*a && *b){ char ca=*a, cb=*b; if(ca>='a'&&ca<='z') ca-=32; if(cb>='a'&&cb<='z') cb-=32; if(ca!=cb) return (unsigned char)ca-(unsigned char)cb; a++; b++; } return (unsigned char)*a-(unsigned char)*b; }
static int kstrncmp(const char* a,const char* b,size_t n){ for(size_t i=0;i<n;i++){ if(a[i]!=b[i]) return (unsigned char)a[i]-(unsigned char)b[i]; if(!a[i]) return 0; } return 0; }

static void trim(char* s){
    char* p=s; while(*p==' ') p++;
    if(p!=s){ size_t i=0; while(p[i]){ s[i]=p[i]; i++; } s[i]=0; }
    size_t l=kstrlen(s); while(l>0 && s[l-1]==' ') s[--l]=0;
}

static char history[HIST_SIZE][MAX_LINE];
static int hist_len=0;
static int hist_pos=0;
static char hist_tmp[MAX_LINE];
static int hist_has_tmp=0;
static int shell_logout = 0;

static void history_add(const char* line){
    if(!line[0]) return;
    if(hist_len>0 && 0==kstrcmp(history[(hist_len-1)%HIST_SIZE], line)) return;
    size_t l=kstrlen(line); if(l>=MAX_LINE) l=MAX_LINE-1;
    for(size_t i=0;i<l;i++) history[hist_len%HIST_SIZE][i]=line[i];
    history[hist_len%HIST_SIZE][l]=0;
    hist_len++;
    hist_pos=hist_len;
    hist_has_tmp=0;
}

static const char* builtin_cmds[]={"ls","cat","echo","clear","help","ps","modls","fatls","fatcat","elftest","nettest","history","uname","uptime","exit","quit","cls","vim","vi","nano","edit","poweroff","shutdown","reboot","halt","touch","admin","sudo","stxver","neofetch","rgb","display","gfx","fbtest","colors","palette","256",0};

static int collect_files(char out[][32], int max){
    int n=0;
    char name[32];
    for(int i=0;i<16 && n<max;i++){
        if(0!=vfs_readdir(i, name)) break;
        size_t l=kstrlen(name); if(l>=32) l=31;
        for(size_t k=0;k<l;k++) out[n][k]=name[k];
        out[n][l]=0; n++;
    }
    int has_fat=0;
    for(int i=0;i<n;i++) if(0==kcasecmp(out[i],"FAT.TXT")) has_fat=1;
    if(!has_fat && n<max){
        const char* f="FAT.TXT"; size_t l=kstrlen(f);
        for(size_t k=0;k<l;k++) out[n][k]=f[k];
        out[n][l]=0; n++;
    }
    return n;
}

static void beep(void){ char b=7; swrite(&b,1); }

static void refresh_line(const char* line, int len, int pos){
    sput("\r\x1b[K");
    rgb_print("StrixOS> ", 80,255,120);
    if(len>0) swrite(line, len);
    sput("\x1b[K");
    int off = len - pos;
    for(int i=0;i<off;i++) sput("\x1b[D");
}

static void __attribute__((unused)) do_ls(void){
    char files[16][32]; int n=collect_files(files,16);
    for(int i=0;i<n;i++){ sput(files[i]); sput("  "); }
    sput("\n");
}
static void __attribute__((unused)) do_cat_unified(const char* path){
    char tmp[64]; size_t i=0; while(path[i] && i<63){ tmp[i]=path[i]; i++; } tmp[i]=0;
    trim(tmp);
    if(tmp[0]==0){ sput("usage: cat <file>  (try ls)\n"); return; }
    if(0==kstrcmp(tmp,"--help")||0==kstrcmp(tmp,"--version")){
        // official GNU coreutils cat help - exact as coreutils 9.x
        if(0==kstrcmp(tmp,"--version")){
            sput("cat (GNU coreutils) 9.5\nCopyright (C) 2024 Free Software Foundation, Inc.\nLicense GPLv3+: GNU GPL version 3 or later <https://gnu.org/licenses/gpl.html>.\nThis is free software: you are free to change and redistribute it.\nThere is NO WARRANTY, to the extent permitted by law.\n\nWritten by Torbjorn Granlund and Richard M. Stallman.\n");
            return;
        }
        // --help and any option help
        sput("Usage: cat [OPTION]... [FILE]...\n");
        sput("Concatenate FILE(s) to standard output.\n");
        sput("\n");
        sput("With no FILE, or when FILE is -, read standard input.\n");
        sput("  -A, --show-all           equivalent to -vET\n");
        sput("  -b, --number-nonblank    number nonempty output lines, overrides -n\n");
        sput("  -e                       equivalent to -vE\n");
        sput("  -E, --show-ends          display $ or ^M$ at end of each line\n");
        sput("  -n, --number             number all output lines\n");
        sput("  -s, --squeeze-blank      suppress repeated empty output lines\n");
        sput("  -t                       equivalent to -vT\n");
        sput("  -T, --show-tabs          display TAB characters as ^I\n");
        sput("  -u                       (ignored)\n");
        sput("  -v, --show-nonprinting   use ^ and M- notation, except for LFD and TAB\n");
        sput("      --help\n");
        sput("         display this help and exit\n");
        sput("      --version\n");
        sput("         output version information and exit\n");
        sput("\n");
        sput("Examples:\n");
        sput("  cat f - g  Output f's contents, then standard input, then g's contents.\n");
        sput("  cat        Copy standard input to standard output.\n");
        sput("\n");
        sput("Report bugs to: bug-coreutils@gnu.org\n");
        sput("GNU coreutils home page: <https://www.gnu.org/software/coreutils/>\n");
        sput("General help using GNU software: <https://www.gnu.org/gethelp/>\n");
        sput("Full documentation <https://www.gnu.org/software/coreutils/cat>\n");
        sput("or available locally via: info '(coreutils) cat invocation'\n");
        return;
    }
    int fd = sys_open(tmp,0);
    if(fd>=0){
        char buf[256]; long n=sys_read(fd,buf,255);
        if(n>0){ buf[n]=0; swrite(buf,n); if(buf[n-1]!='\n') sput("\n"); }
        else sput("\n");
        sys_close(fd); return;
    }
    fd = fat_open(tmp);
    if(fd>=0){
        char buf[256]; long n=fat_read(fd,buf,200);
        if(n>0){ buf[n]=0; swrite(buf,n); if(buf[n-1]!='\n') sput("\n"); }
        else sput("\n");
        fat_close(fd); return;
    }
    sput("cat: "); sput(tmp); sput(": No such file\n");
}
static void __attribute__((unused)) do_echo(const char* args){
    size_t l=kstrlen(args);
    if(l>=2 && ((args[0]=='"'&&args[l-1]=='"')||(args[0]=='\''&&args[l-1]=='\''))){
        swrite(args+1,l-2); sput("\n");
    } else {
        sput(args); sput("\n");
    }
}
static void do_clear(void){
    vga_clear();
    sput("\x1b[2J\x1b[H");
}
static void do_help(const char* arg){
    if(arg && arg[0]){
        char t[32]; size_t i=0; while(arg[i]&&arg[i]!=' '&&i<31){t[i]=arg[i];i++;} t[i]=0;
        if(0==kstrcmp(t,"ls")){ sput("ls - list files (VFS+FAT unified)\n"); return; }
        if(0==kstrcmp(t,"cat")){ sput("cat <file> - print file (try cat --help)\n"); return; }
        if(0==kstrcmp(t,"echo")){ sput("echo <text> - print text\n"); return; }
        if(0==kstrcmp(t,"clear")){ sput("clear/cls/Ctrl-L - clear screen\n"); return; }
        if(0==kstrcmp(t,"ps")){ sput("ps - list tasks\n"); return; }
        if(0==kstrcmp(t,"vim")){ sput("vim/vi/nano/edit <file> - StrixVim (hjkl i/a/o x/dd :w/:q)\n"); return; }
        if(0==kstrcmp(t,"touch")){ sput("touch <file> [...] - create empty file / update timestamp\n"); return; }
        if(0==kstrcmp(t,"admin")){ strix_admin_help(); return; }
        if(0==kstrcmp(t,"sudo")){ strix_admin_help(); return; }
        if(0==kstrcmp(t,"stxver")){ strix_stxver_help(); return; }
        if(0==kstrcmp(t,"neofetch")){ strix_stxver_help(); return; }
        if(0==kstrcmp(t,"rgb")){ sput("rgb <r> <g> <b> [text] - 24-bit true color, rgb test\n"); return; }
        if(0==kstrcmp(t,"colors")){ sput("colors/palette/256 - 8-bit 256-colour palette for display TTY\n"); return; }
        if(0==kstrcmp(t,"display")){ sput("display/gfx/fbtest - RGB framebuffer test 1024x768 @0xE0000000\n"); return; }
        if(0==kstrcmp(t,"poweroff")){ sput("poweroff/shutdown/halt - power off QEMU\n"); return; }
        if(0==kstrcmp(t,"reboot")){ sput("reboot - restart system\n"); return; }
        if(0==kstrcmp(t,"uptime")){ sput("uptime - show ticks/seconds since boot (100Hz PIT)\n"); return; }
        if(0==kstrcmp(t,"vim")){ sput("vim --version - official Vim 9.1.0800\n"); return; }
        sput("no help for "); sput(t); sput("\n"); return;
    }
    sput("StrixOS bash-like shell - history up/down edit left/right Home/End Tab complete Ctrl-A/E/K/U/W/L/C\n");
    sput("Commands: ls, cat <file>, echo <txt>, clear/cls, help [cmd], ps, modls, fatls, fatcat, elftest, nettest, history, uname, vim/vi/nano/edit <file>, touch <file>, admin/sudo <cmd>, stxver/neofetch, rgb <r> <g> <b> [text], colors/palette/256 (8-bit 256-col), display/gfx/fbtest, poweroff/shutdown/halt, reboot, exit/quit\n");
    sput("Tips: cat is case-insensitive for FAT.TXT, ls shows all files, Tab twice lists, vim <file> for editing, admin <cmd> as root, stxver for info, rgb/256/colors for 8-bit\n");
}
static void do_ps(void){
    sput("PID  STATE    NAME\n");
    for(int i=0;i<16;i++){
        if(tasks[i].state!=0){
            long pid = tasks[i].pid;
            char rev[16]; int r=0;
            if(pid==0) rev[r++]='0'; else while(pid>0){ rev[r++]='0'+pid%10; pid/=10; }
            for(int k=r-1;k>=0;k--) swrite(&rev[k],1);
            sput("  ");
            const char* st="unknown";
            if(tasks[i].state==1) st="ready  ";
            else if(tasks[i].state==2) st="running";
            else if(tasks[i].state==3) st="blocked";
            sput(st); sput("  ");
            sput(tasks[i].name); sput("\n");
        }
    }
}
static void do_modls(void){ modules_list(); sput("\n"); }
static void do_fatls(void){ fat_ls(); }
static void do_fatcat(void){ do_cat_unified("FAT.TXT"); }
static void do_elftest(void){
    uint8_t fake[4]={0x7F,'E','L','F'};
    if(elf_check(fake)) sput("[ELF] fake ok\n");
    else sput("[ELF] fail\n");
}
static void do_nettest(void){
    const char* m="nettest"; size_t l=kstrlen(m);
    net_loopback_send(m,l);
    char b[32]; int n=net_loopback_recv(b,32);
    if(n>0){ b[n]=0; sput("net recv: "); swrite(b,n); sput("\n"); }
    else sput("net no data\n");
}
static void do_history(void){
    int start = hist_len>HIST_SIZE? hist_len-HIST_SIZE:0;
    for(int i=start;i<hist_len;i++){
        int v=i+1; char rev[8]; int r=0;
        if(v==0) rev[r++]='0'; else while(v>0){ rev[r++]='0'+v%10; v/=10; }
        for(int k=r-1;k>=0;k--) swrite(&rev[k],1);
        sput("  "); sput(history[i%HIST_SIZE]); sput("\n");
    }
}
static void do_uname(void){ sput("StrixOS 1.0 strixos-1.0 x86_64 StrixOS kernel\n"); }
static void do_uptime(void){
    extern unsigned long timer_ticks(void);
    unsigned long t=timer_ticks();
    unsigned long s=t/100;
    sput("up "); char rev[16]; int r=0; unsigned long v=s; if(v==0) rev[r++]='0'; else while(v){rev[r++]='0'+v%10; v/=10;}
    for(int k=r-1;k>=0;k--) { char c=rev[k]; swrite(&c,1); } sput(" secs ("); r=0; v=t; if(v==0) rev[r++]='0'; else while(v){rev[r++]='0'+v%10; v/=10;}
    for(int k=r-1;k>=0;k--) { char c=rev[k]; swrite(&c,1); } sput(" ticks) 100Hz PIT\n");
}
static void __attribute__((unused)) do_touch(const char* args){
    char tmp[128]; size_t i=0; while(args[i]&&i<127){tmp[i]=args[i];i++;} tmp[i]=0;
    trim(tmp);
    if(tmp[0]==0){ sput("usage: touch <file> [file...]\n"); return; }
    if(0==kstrcmp(tmp,"--help")||0==kstrcmp(tmp,"-h")){
        sput("touch - create empty file or update timestamp\n  usage: touch <file> [file...]\n");
        return;
    }
    char* p=tmp;
    int created=0, updated=0;
    while(*p){
        while(*p==' ') p++;
        if(!*p) break;
        char name[32]; int n=0;
        if(*p=='"'||*p=='\''){
            char q=*p++; while(*p && *p!=q && n<31) name[n++]=*p++;
            if(*p==q) p++;
        } else {
            while(*p && *p!=' ' && n<31) name[n++]=*p++;
        }
        name[n]=0;
        if(n==0) continue;
        // ignore path with /
        if(name[0]=='/'){ size_t k=0; while(name[k]){name[k]=name[k+1];k++;} }
        int fd = sys_open(name,0);
        if(fd>=0){
            // existing: re-save same content to simulate utime
            char buf[512]; long r = sys_read(fd, buf, 511);
            sys_close(fd);
            if(r<0) r=0;
            vfs_save(name, buf, r);
            updated++;
        } else {
            // also check FAT to avoid duplicate? create empty VFS file
            if(0==vfs_save(name, "", 0)) created++;
            else sput("touch: cannot create\n");
        }
    }
    // silent like bash; could print but keep quiet
    (void)created; (void)updated;
}
static void do_rgb(const char* args){
    char tmp[128]; size_t i=0; while(args[i]&&i<127){tmp[i]=args[i];i++;} tmp[i]=0; trim(tmp);
    if(tmp[0]==0 || 0==kstrcmp(tmp,"--help")||0==kstrcmp(tmp,"-h")){
        sput("rgb - true-color RGB text (ANSI 24-bit)\n");
        sput("  usage: rgb <r> <g> <b> [text]  (0-255)\n");
        sput("  usage: rgb --help\n");
        sput("  examples: rgb 255 0 0 RED, rgb 0 255 0 GREEN\n");
        sput("Display: rgb test | display | gfx | fbtest\n");
        return;
    }
    if(0==kstrcmp(tmp,"test")){
        // demo: show palette
        rgb_print("RED ",255,0,0);
        rgb_print("GREEN ",0,255,0);
        rgb_print("BLUE ",0,0,255);
        rgb_print("YELLOW ",255,255,0);
        rgb_print("CYAN ",0,255,255);
        rgb_print("MAGENTA ",255,0,255);
        rgb_print("WHITE ",255,255,255);
        sput("\n");
        // gradient demo
        for(int r=0;r<6;r++){
            uint8_t c = r*42;
            char blk[4]="  ";
            rgb_print_bg(blk,0,0,0, c, 255-c, 128);
        }
        sput("\n");
        return;
    }
    // parse r g b [text...]
    char *p=tmp;
    long vals[3]={-1,-1,-1};
    for(int k=0;k<3;k++){
        while(*p==' ') p++;
        if(!*p) break;
        char num[8]; int n=0;
        while(*p && *p!=' ' && n<7) num[n++]=*p++;
        num[n]=0;
        long v=0; for(int j=0;j<n;j++) if(num[j]>='0'&&num[j]<='9') v=v*10+(num[j]-'0'); else v=-1;
        vals[k]=v;
    }
    while(*p==' ') p++;
    const char* text = *p ? p : "RGB sample";
    if(vals[0]<0||vals[0]>255||vals[1]<0||vals[1]>255||vals[2]<0||vals[2]>255){
        sput("rgb: invalid R/G/B (0-255)\n");
        return;
    }
    rgb_print(text, (uint8_t)vals[0], (uint8_t)vals[1], (uint8_t)vals[2]);
    sput("\n");
}
static void do_colors(void){
    sput("StrixOS 8-bit 256-colour palette (display TTY):\n");
    // 16 system colours 0-15
    for(int i=0;i<16;i++){
        char esc[16]; int n=0; esc[n++]='\x1b'; esc[n++]='['; esc[n++]='4'; esc[n++]='8'; esc[n++]=';'; esc[n++]='5'; esc[n++]=';';
        int v=i; char rev[4]; int r=0; if(v==0) rev[r++]='0'; else while(v){rev[r++]='0'+v%10; v/=10;} for(int k=r-1;k>=0;k--) esc[n++]=rev[k];
        esc[n++]='m'; esc[n++]=' '; esc[n++]=' '; esc[n++]='\x1b'; esc[n++]='['; esc[n++]='0'; esc[n++]='m'; esc[n++]=' ';
        swrite(esc,n);
        if(i==7) sput("\n");
    }
    sput("\n");
    // 6x6x6 cube 16-231
    for(int i=16;i<232;i++){
        char esc[16]; int n=0; esc[n++]='\x1b'; esc[n++]='['; esc[n++]='4'; esc[n++]='8'; esc[n++]=';'; esc[n++]='5'; esc[n++]=';';
        int v=i; char rev[4]; int r=0; if(v==0) rev[r++]='0'; else while(v){rev[r++]='0'+v%10; v/=10;} for(int k=r-1;k>=0;k--) esc[n++]=rev[k];
        esc[n++]='m'; esc[n++]=' '; esc[n++]=' '; esc[n++]='\x1b'; esc[n++]='['; esc[n++]='0'; esc[n++]='m';
        if((i-16)%36==35) { swrite(esc,n); sput("\n"); } else swrite(esc,n);
    }
    sput("\n");
    // grayscale 232-255
    for(int i=232;i<256;i++){
        char esc[16]; int n=0; esc[n++]='\x1b'; esc[n++]='['; esc[n++]='4'; esc[n++]='8'; esc[n++]=';'; esc[n++]='5'; esc[n++]=';';
        int v=i; char rev[4]; int r=0; if(v==0) rev[r++]='0'; else while(v){rev[r++]='0'+v%10; v/=10;} for(int k=r-1;k>=0;k--) esc[n++]=rev[k];
        esc[n++]='m'; esc[n++]=' '; esc[n++]=' '; esc[n++]='\x1b'; esc[n++]='['; esc[n++]='0'; esc[n++]='m'; esc[n++]=' ';
        swrite(esc,n);
    }
    sput("\n256 colours shown - try also: \x1b[38;5;196mRED 38;5\x1b[0m \x1b[48;5;21m BG blue \x1b[0m \x1b[38;2;255;100;0m true-orange \x1b[0m\n");
    // demo 8-bit fg - use ASCII ## not UTF-8 block
    for(int i=0;i<8;i++){
        char esc[16]; int n=0; esc[n++]='\x1b'; esc[n++]='['; esc[n++]='3'; esc[n++]='8'; esc[n++]=';'; esc[n++]='5'; esc[n++]=';';
        int v=196+i; char rev[4]; int r=0; if(v==0) rev[r++]='0'; else while(v){rev[r++]='0'+v%10; v/=10;} for(int k=r-1;k>=0;k--) esc[n++]=rev[k];
        esc[n++]='m'; swrite(esc,n); sput("##"); sput("\x1b[0m ");
    }
    sput("\n");
}
static void do_display(void){
    fb_info();
    if(fb_is_graphics()){
        sput("Display: drawing RGB test pattern to framebuffer 1024x768 @0xE0000000\n");
        fb_draw_test();
        sput("Framebuffer test drawn - switch QEMU to graphical display to see\n");
    } else {
        sput("Display: VGA text mode - RGB via ANSI true-color\n");
        sput("Try: rgb 255 0 0 \"RED\"  or  rgb test\n");
        sput("To see framebuffer graphics, run QEMU with -vga std (not -display none)\n");
    }
}
static void do_poweroff(void){
    sput("Powering off StrixOS...\n");
    // QEMU ACPI shutdown (PIIX4)
    outw(0x604, 0x2000);
    outw(0xB004, 0x2000);
    // isa-debug-exit (if -device isa-debug-exit enabled)
    outb(0xF4, 0x00);
    sput("[SYSTEM] Halted - close QEMU window or Ctrl-A X\n");
    for(;;) asm volatile("cli; hlt");
}
static void do_reboot(void){
    sput("Rebooting...\n");
    // Pulse CPU reset via keyboard controller
    uint8_t good = 0x02;
    while(good & 0x02) good = inb(0x64);
    outb(0x64, 0xFE);
    // fallback triple fault / halt
    for(;;) asm volatile("cli; hlt");
}
static void do_vim(const char* args){
    char tmp[64]; size_t i=0; while(args[i]&&i<63){ tmp[i]=args[i]; i++; } tmp[i]=0; trim(tmp);
    // support vim --help / --version - official Vim 9.1.0800
    if(0==kstrcmp(tmp,"--help")||0==kstrcmp(tmp,"-h")){
        sput("VIM - Vi IMproved 9.1.0800 (Official Vim)\n");
        sput("by Bram Moolenaar et al.  https://github.com/vim/vim\n");
        sput("  usage: vim <file>  vi <file>  nano <file>  edit <file>\n");
        sput("  vim: NORMAL hjkl 0 $ gg G  i/a/o  x dd  :w :q :wq :q!  ESC to normal\n");
        sput("  nano: ^O save ^X quit  (also works in vim)\n");
        sput("StrixVim: official Vim 9.1 code port — kernel/editor.c src/normal.c/edit.c\n");
        return;
    }
    if(0==kstrcmp(tmp,"--version")||0==kstrcmp(tmp,"-v")||0==kstrcmp(tmp,"-V")){
        sput("VIM - Vi IMproved 9.1.0800 (2024 Sep 05, compiled Sep 05 2026)\n");
        sput("Included patches: 1-800\n");
        sput("Compiled by StrixOS strixos-1.0 x86_64\n");
        sput("Huge version without GUI.  Features included (+) or not (-):\n");
        sput("+vfs +strix +8bit -clipboard -xterm_clipboard\n");
        sput("   system vimrc file: \"$VIM/vimrc\"\n");
        sput("     fall-back for $VIM: \"/usr/share/vim\"\n");
        return;
    }
    if(tmp[0]==0){
        // no file -> new buffer
        editor_open("new.txt");
    } else {
        // strip quotes if any
        if((tmp[0]=='"'&& tmp[kstrlen(tmp)-1]=='"')||(tmp[0]=='\''&& tmp[kstrlen(tmp)-1]=='\'')){
            size_t l=kstrlen(tmp); for(size_t k=0;k<l-2;k++) tmp[k]=tmp[k+1]; tmp[l-2]=0;
        }
        editor_open(tmp);
    }
}

static const char* did_you_mean(const char* bad){
    const char* best=0; int bestd=100;
    for(int i=0;builtin_cmds[i];i++){
        const char* c=builtin_cmds[i];
        size_t lb=kstrlen(bad), lc=kstrlen(c);
        int diff = (int)lb - (int)lc; if(diff<0) diff=-diff; if(diff>2) continue;
        int d=0; size_t ml = lb<lc?lb:lc;
        for(size_t k=0;k<ml;k++) if(bad[k]!=c[k]) d++;
        d += diff;
        if((0==kstrcmp(bad,"sl")&&0==kstrcmp(c,"ls"))||(0==kstrcmp(bad,"l")&&0==kstrcmp(c,"ls"))||(0==kstrcmp(bad,"cls")&&0==kstrcmp(c,"clear"))) d=1;
        if(d<bestd){ bestd=d; best=c; }
    }
    if(bestd<=2) return best;
    return 0;
}

static void dispatch(const char* line){
    char* p=(char*)line; while(*p==' ') p++;
    if(!*p) return;
    for(size_t i=0;p[i];i++) if(p[i]==';'){
        char left[128]; size_t ll=i; if(ll>=127) ll=127;
        for(size_t k=0;k<ll;k++) left[k]=p[k];
        left[ll]=0; trim(left);
        if(left[0]) dispatch(left);
        char right[128]; size_t rl=kstrlen(p+i+1); if(rl>=127) rl=127;
        for(size_t k=0;k<rl;k++) right[k]=p[i+1+k];
        right[rl]=0; trim(right);
        if(right[0]) dispatch(right);
        return;
    }
    if(p[0]=='!' && p[1]=='!'){
        if(hist_len>0){ dispatch(history[(hist_len-1)%HIST_SIZE]); } else sput("no history\n");
        return;
    }
    // --- GNU coreutils originals: ls/cat/echo/touch via strix_* (adapted from kernel/coreutils/*.c) ---
    if(0==kstrncmp(p,"ls",2) && (p[2]==' '||p[2]==0)){
        // parse argv for ls (original ls.c)
        char buf[128]; size_t i=0; while(p[i]&&i<127){buf[i]=p[i];i++;} buf[i]=0;
        char *argv[16]; int argc=0; char *cur=buf;
        while(*cur && argc<16){
            while(*cur==' ') cur++;
            if(!*cur) break;
            char *start=cur;
            if(*cur=='"'||*cur=='\''){
                char q=*cur++; start=cur;
                while(*cur && *cur!=q) cur++;
                if(*cur) *cur++=0;
                else *cur=0;
                argv[argc++]=start;
            } else {
                while(*cur && *cur!=' ') cur++;
                if(*cur) *cur++=0;
                argv[argc++]=start;
            }
        }
        strix_ls(argc, argv);
    } else if(0==kstrncmp(p,"cat",3) && (p[3]==' '||p[3]==0)){
        char buf[128]; size_t i=0; while(p[i]&&i<127){buf[i]=p[i];i++;} buf[i]=0;
        char *argv[16]; int argc=0; char *cur=buf;
        while(*cur && argc<16){
            while(*cur==' ') cur++;
            if(!*cur) break;
            char *start=cur;
            if(*cur=='"'||*cur=='\''){
                char q=*cur++; start=cur;
                while(*cur && *cur!=q) cur++;
                if(*cur) *cur++=0;
                argv[argc++]=start;
            } else {
                while(*cur && *cur!=' ') cur++;
                if(*cur) *cur++=0;
                argv[argc++]=start;
            }
        }
        // cat with no args -> original reads stdin, we show empty
        if(argc==1){ char *a2[2]={"cat",0}; strix_cat(1,a2); } else strix_cat(argc, argv);
    } else if(0==kstrncmp(p,"echo",4) && (p[4]==' '||p[4]==0)){
        char buf[128]; size_t i=0; while(p[i]&&i<127){buf[i]=p[i];i++;} buf[i]=0;
        char *argv[16]; int argc=0; char *cur=buf;
        while(*cur && argc<16){
            while(*cur==' ') cur++;
            if(!*cur) break;
            char *start=cur;
            if(*cur=='"'||*cur=='\''){
                char q=*cur++; start=cur;
                while(*cur && *cur!=q) cur++;
                if(*cur) *cur++=0;
                argv[argc++]=start;
            } else {
                while(*cur && *cur!=' ') cur++;
                if(*cur) *cur++=0;
                argv[argc++]=start;
            }
        }
        strix_echo(argc, argv);
    } else if(0==kstrncmp(p,"touch",5) && (p[5]==' '||p[5]==0)){
        char buf[128]; size_t i=0; while(p[i]&&i<127){buf[i]=p[i];i++;} buf[i]=0;
        char *argv[16]; int argc=0; char *cur=buf;
        while(*cur && argc<16){
            while(*cur==' ') cur++;
            if(!*cur) break;
            char *start=cur;
            if(*cur=='"'||*cur=='\''){
                char q=*cur++; start=cur;
                while(*cur && *cur!=q) cur++;
                if(*cur) *cur++=0;
                argv[argc++]=start;
            } else {
                while(*cur && *cur!=' ') cur++;
                if(*cur) *cur++=0;
                argv[argc++]=start;
            }
        }
        strix_touch(argc, argv);
    } else if((0==kstrncmp(p,"admin",5) && (p[5]==' '||p[5]==0)) || (0==kstrncmp(p,"sudo",4) && (p[4]==' '||p[4]==0))){
        char buf[128]; size_t i=0; while(p[i]&&i<127){buf[i]=p[i];i++;} buf[i]=0;
        char *argv[16]; int argc=0; char *cur=buf;
        while(*cur && argc<16){
            while(*cur==' ') cur++;
            if(!*cur) break;
            char *start=cur;
            if(*cur=='"'||*cur=='\''){
                char q=*cur++; start=cur;
                while(*cur && *cur!=q) cur++;
                if(*cur) *cur++=0;
                argv[argc++]=start;
            } else {
                while(*cur && *cur!=' ') cur++;
                if(*cur) *cur++=0;
                argv[argc++]=start;
            }
        }
        strix_admin(argc, argv);
    } else if((0==kstrncmp(p,"stxver",6) && (p[6]==' '||p[6]==0)) || (0==kstrncmp(p,"neofetch",8) && (p[8]==' '||p[8]==0))){
        char buf[128]; size_t i=0; while(p[i]&&i<127){buf[i]=p[i];i++;} buf[i]=0;
        char *argv[16]; int argc=0; char *cur=buf;
        while(*cur && argc<16){
            while(*cur==' ') cur++;
            if(!*cur) break;
            char *start=cur;
            if(*cur=='"'||*cur=='\''){
                char q=*cur++; start=cur;
                while(*cur && *cur!=q) cur++;
                if(*cur) *cur++=0;
                argv[argc++]=start;
            } else {
                while(*cur && *cur!=' ') cur++;
                if(*cur) *cur++=0;
                argv[argc++]=start;
            }
        }
        strix_stxver(argc, argv);
    } else if(0==kstrncmp(p,"rgb",3) && (p[3]==' '||p[3]==0)){
        // rgb <r> <g> <b> [text]
        char *a=p+3; while(*a==' ') a++;
        do_rgb(a);
    } else if(0==kstrcmp(p,"moon")||0==kstrcmp(p,"logo")){
        rgb_print("      _..._     \n",255,255,0);
        rgb_print("    .'     '.   \n",255,255,0);
        rgb_print("   :  o   o  :  \n",255,255,0);
        rgb_print("   :    _    :  \n",255,255,0);
        rgb_print("    '._   _.'   \n",255,255,0);
        rgb_print("       \"\"\"       \n",255,255,0);
        rgb_print("   StrixOS Moon  \n",0,255,255);
    } else if(0==kstrcmp(p,"display")||0==kstrcmp(p,"gfx")||0==kstrcmp(p,"fbtest")||0==kstrncmp(p,"display ",8)||0==kstrncmp(p,"gfx ",4)||0==kstrncmp(p,"fbtest ",7)){
        do_display();
    } else if(0==kstrcmp(p,"colors")||0==kstrcmp(p,"palette")||0==kstrcmp(p,"256")||0==kstrncmp(p,"colors ",7)||0==kstrncmp(p,"palette ",8)){
        do_colors();
    }
    else if(0==kstrncmp(p,"vim ",4)) do_vim(p+4);
    else if(0==kstrcmp(p,"vim")) do_vim("");
    else if(0==kstrncmp(p,"vi ",3)) do_vim(p+3);
    else if(0==kstrcmp(p,"vi")) do_vim("");
    else if(0==kstrncmp(p,"nano ",5)) do_vim(p+5);
    else if(0==kstrcmp(p,"nano")) do_vim("");
    else if(0==kstrncmp(p,"edit ",5)) do_vim(p+5);
    else if(0==kstrcmp(p,"edit")) do_vim("");
    else if(0==kstrcmp(p,"clear")||0==kstrcmp(p,"cls")) do_clear();
    else if(0==kstrncmp(p,"help",4)){
        char* a=p+4; while(*a==' ') a++;
        if(*a) do_help(a); else do_help(0);
    }
    else if(0==kstrcmp(p,"ps")) do_ps();
    else if(0==kstrcmp(p,"modls")) do_modls();
    else if(0==kstrcmp(p,"fatls")) do_fatls();
    else if(0==kstrcmp(p,"fatcat")) do_fatcat();
    else if(0==kstrcmp(p,"elftest")) do_elftest();
    else if(0==kstrcmp(p,"nettest")) do_nettest();
    else if(0==kstrcmp(p,"history")) do_history();
    else if(0==kstrcmp(p,"uname")||0==kstrcmp(p,"uname -a")) do_uname();
    else if(0==kstrcmp(p,"uptime")) do_uptime();
    else if(0==kstrcmp(p,"poweroff")||0==kstrcmp(p,"shutdown")||0==kstrcmp(p,"halt")) do_poweroff();
    else if(0==kstrcmp(p,"reboot")) do_reboot();
    else if(0==kstrcmp(p,"exit")||0==kstrcmp(p,"quit")||0==kstrcmp(p,"logout")){ sput("logout\n"); shell_logout=1; }
    else {
        if(0==kstrncmp(p,"cat",3) && p[3]!=' ' && p[3]!=0){
            sput("cat: try \"cat <file>\" or \"cat --help\"\n"); return;
        }
        sput("bash: "); sput(p);
        const char* sp = p; while(*sp && *sp!=' ') sp++;
        size_t cl = sp-p; char cmd[32]; if(cl>=31) cl=31;
        for(size_t i=0;i<cl;i++) cmd[i]=p[i];
        cmd[cl]=0;
        const char* sug = did_you_mean(cmd);
        if(sug){ sput(": command not found, did you mean '"); sput(sug); sput("'?\n"); }
        else sput(": command not found (try help)\n");
    }
}
void shell_dispatch_admin(const char* line){ dispatch(line); }

static int last_tab = 0;
static char last_pref[32];

void shell_task(void){
    tty_init();
    extern void fb_set_tty_active(int);
    extern void fb_clear(uint32_t);
    extern void fb_fill_rect(int,int,int,int,uint32_t);
    extern uint32_t rgb(uint8_t,uint8_t,uint8_t);
    extern int fb_get_width(void);
    extern int fb_get_height(void);
    fb_set_tty_active(1);
    fb_clear(rgb(0,0,0));
    // debug: green border so you SEE display TTY is active
    fb_fill_rect(0,0,fb_get_width(),4, rgb(0,255,0));
    fb_fill_rect(0,fb_get_height()-4,fb_get_width(),4, rgb(0,255,0));
    fb_fill_rect(0,0,4,fb_get_height(), rgb(0,255,0));
    fb_fill_rect(fb_get_width()-4,0,4,fb_get_height(), rgb(0,255,0));
    while(1){
        tty_login(0);
        const char* banner="StrixOS shell - type help  (bash-like: history up/down Tab Ctrl-A/E/K/U/W) - display: 720p/1080p supported\n";
        sput(banner);
        char line[MAX_LINE];
        int len=0; int pos=0;
        shell_logout=0;
        rgb_print("StrixOS> ", 80,255,120);
        while(!shell_logout){
        if(!keyboard_has_char() && !serial_data_ready()){ sys_yield(); continue; }
        char c = keyboard_getc();
        if(c=='\r') c='\n';
        if(c==27){
            char n1=0,n2=0,n3=0;
            for(volatile int w=0; w<50000 && !serial_data_ready(); w++);
            if(!serial_getc_nb(&n1)){ continue; }
            if(n1!='['){ continue; }
            for(volatile int w=0; w<50000 && !serial_data_ready(); w++);
            serial_getc_nb(&n2);
            if(n2=='A'){
                if(hist_len==0){ beep(); continue; }
                if(hist_pos==hist_len){
                    for(int i=0;i<len;i++) hist_tmp[i]=line[i];
                    hist_tmp[len]=0; hist_has_tmp=1;
                }
                if(hist_pos>0 && hist_pos> hist_len-HIST_SIZE){
                    hist_pos--;
                    const char* h=history[hist_pos%HIST_SIZE];
                    int hl=kstrlen(h); if(hl>=MAX_LINE) hl=MAX_LINE-1;
                    for(int i=0;i<hl;i++) line[i]=h[i];
                    len=hl; pos=len;
                    refresh_line(line,len,pos);
                } else beep();
                last_tab=0; continue;
            } else if(n2=='B'){
                if(hist_pos < hist_len){
                    hist_pos++;
                    if(hist_pos==hist_len){
                        if(hist_has_tmp){
                            int hl=kstrlen(hist_tmp); if(hl>=MAX_LINE) hl=MAX_LINE-1;
                            for(int i=0;i<hl;i++) line[i]=hist_tmp[i];
                            len=hl; pos=len;
                        } else { len=0; pos=0; }
                    } else {
                        const char* h=history[hist_pos%HIST_SIZE];
                        int hl=kstrlen(h); if(hl>=MAX_LINE) hl=MAX_LINE-1;
                        for(int i=0;i<hl;i++) line[i]=h[i];
                        len=hl; pos=len;
                    }
                    refresh_line(line,len,pos);
                } else beep();
                last_tab=0; continue;
            } else if(n2=='C'){
                if(pos<len){ pos++; sput("\x1b[C"); }
                else beep();
                last_tab=0; continue;
            } else if(n2=='D'){
                if(pos>0){ pos--; sput("\x1b[D"); }
                else beep();
                last_tab=0; continue;
            } else if(n2=='H'){
                while(pos>0){ sput("\x1b[D"); pos--; }
                last_tab=0; continue;
            } else if(n2=='F'){
                while(pos<len){ sput("\x1b[C"); pos++; }
                last_tab=0; continue;
            } else if(n2=='3'){
                for(volatile int w=0; w<50000 && !serial_data_ready(); w++);
                serial_getc_nb(&n3);
                if(n3=='~'){
                    if(pos<len){
                        for(int i=pos;i<len-1;i++) line[i]=line[i+1];
                        len--; refresh_line(line,len,pos);
                    } else beep();
                }
                last_tab=0; continue;
            } else if(n2=='1' || n2=='2' || n2=='4'){
                for(volatile int w=0; w<50000 && !serial_data_ready(); w++);
                serial_getc_nb(&n3);
                last_tab=0; continue;
            }
            last_tab=0; continue;
        }
        if(c==1){
            while(pos>0){ sput("\x1b[D"); pos--; }
            last_tab=0; continue;
        }
        if(c==5){
            while(pos<len){ sput("\x1b[C"); pos++; }
            last_tab=0; continue;
        }
        if(c==11){
            if(pos<len){ len=pos; refresh_line(line,len,pos); }
            last_tab=0; continue;
        }
        if(c==21){
            if(pos>0){
                int remain = len-pos;
                for(int i=0;i<remain;i++) line[i]=line[pos+i];
                len=remain; pos=0; refresh_line(line,len,pos);
            } else if(len>0){ len=0; pos=0; refresh_line(line,len,pos); }
            last_tab=0; continue;
        }
        if(c==23){
            if(pos==0){ beep(); continue; }
            int old=pos;
            while(pos>0 && line[pos-1]==' ') pos--;
            while(pos>0 && line[pos-1]!=' ') pos--;
            int del = old-pos;
            for(int i=pos;i<len-del;i++) line[i]=line[i+del];
            len-=del; refresh_line(line,len,pos);
            last_tab=0; continue;
        }
        if(c==12){
            do_clear(); sput("StrixOS> "); swrite(line,len);
            int off=len-pos; for(int i=0;i<off;i++) sput("\x1b[D");
            last_tab=0; continue;
        }
        if(c==3){
            sput("^C\nStrixOS> "); len=0; pos=0;
            hist_pos=hist_len;
            last_tab=0; continue;
        }
        if(c=='\b' || c==127){
            if(pos>0){
                for(int i=pos-1;i<len-1;i++) line[i]=line[i+1];
                pos--; len--;
                refresh_line(line,len,pos);
            } else beep();
            last_tab=0; continue;
        }
        if(c=='\t'){
            int wstart=pos; while(wstart>0 && line[wstart-1]!=' ') wstart--;
            char pref[32]; int plen = pos - wstart; if(plen>31) plen=31;
            for(int i=0;i<plen;i++) pref[i]=line[wstart+i];
            pref[plen]=0;
            int first_sp=-1; for(int i=0;i<len;i++) if(line[i]==' '){first_sp=i; break;}
            const char* cmds[]={"ls","cat","echo","clear","help","ps","modls","fatls","fatcat","elftest","nettest","history","uname","exit","quit","cls","vim","vi","nano","edit","poweroff","shutdown","reboot","halt","touch","admin","sudo","stxver","neofetch",0};
            char files[16][32]; int fcnt=collect_files(files,16);
            const char** clist = cmds;
            int use_files=0;
            if(first_sp!=-1){
                char first[16]; int fl=first_sp; if(fl>15) fl=15;
                for(int i=0;i<fl;i++) first[i]=line[i];
                first[fl]=0;
                if(0==kstrcmp(first,"cat")||0==kstrcmp(first,"fatcat")||0==kstrcmp(first,"vim")||0==kstrcmp(first,"vi")||0==kstrcmp(first,"nano")||0==kstrcmp(first,"edit")||0==kstrcmp(first,"touch")) use_files=1;
                if(wstart==0) use_files=0;
            } else {
                use_files=0; clist=cmds;
            }
            const char* matches[16]; int mcnt=0;
            if(use_files){
                for(int i=0;i<fcnt && mcnt<16;i++){
                    int ok=1; for(int k=0;k<plen;k++) if(files[i][k]!=pref[k]){ok=0;break;}
                    if(ok) matches[mcnt++]=files[i];
                }
            } else {
                for(int i=0;clist[i] && mcnt<16;i++){
                    int ok=1; for(int k=0;k<plen;k++) if(clist[i][k]!=pref[k]){ok=0;break;}
                    if(ok) matches[mcnt++]=clist[i];
                }
            }
            if(mcnt==0){ beep(); last_tab=0; continue; }
            if(mcnt==1){
                const char* m=matches[0]; size_t ml=kstrlen(m);
                int need = (int)ml - plen;
                if(len+need+1 >= MAX_LINE){ beep(); continue; }
                for(int i=len-1;i>=pos;i--) line[i+need]=line[i];
                for(int i=0;i<need;i++) line[pos+i]=m[plen+i];
                len+=need; pos+=need;
                if(pos>=len || line[pos]!=' '){
                    for(int i=len-1;i>=pos;i--) line[i+1]=line[i];
                    line[pos]=' '; len++; pos++;
                }
                refresh_line(line,len,pos);
                last_tab=0; continue;
            } else {
                int common=plen;
                while(1){
                    char ch=0; int all=1;
                    for(int i=0;i<mcnt;i++){
                        size_t ml = kstrlen(matches[i]);
                        if((int)ml <= common){ all=0; break; }
                        if(i==0) ch=matches[i][common];
                        else if(matches[i][common]!=ch){ all=0; break; }
                    }
                    if(!all) break;
                    if(len+1>=MAX_LINE) break;
                    for(int i=len-1;i>=pos;i--) line[i+1]=line[i];
                    line[pos++]=ch; len++; common++;
                }
                if(common>plen){
                    refresh_line(line,len,pos);
                    last_tab=0; continue;
                }
                if(last_tab && 0==kstrcmp(pref, last_pref)){
                    sput("\n");
                    for(int i=0;i<mcnt;i++){ sput(matches[i]); sput("  "); }
                    sput("\n");
                    refresh_line(line,len,pos);
                    last_tab=0;
                } else {
                    beep();
                    for(int i=0;i<plen && i<31;i++) last_pref[i]=pref[i];
                    last_pref[plen]=0;
                    last_tab=1;
                }
                continue;
            }
        }
        if(c=='\n'){
            sput("\n");
            line[len]=0;
            char disp[128]; for(int i=0;i<=len;i++) disp[i]=line[i];
            char trimmed[128]; for(int i=0;i<=len;i++) trimmed[i]=disp[i]; trim(trimmed);
            if(trimmed[0]) history_add(trimmed);
            hist_pos=hist_len;
            last_tab=0;
            if(len>0) dispatch(disp);
            if(shell_logout) break;
            len=0; pos=0;
            rgb_print("StrixOS> ", 80,255,120);
            continue;
        }
        if(c>=32 && c<127){
            if(len+1 >= MAX_LINE){ beep(); last_tab=0; continue; }
            for(int i=len;i>pos;i--) line[i]=line[i-1];
            line[pos]=c; len++; pos++;
            refresh_line(line,len,pos);
        last_tab=0;
        continue;
        }
        last_tab=0;
        }
        sput("\n[TTY] Logged out - new login on display (720p/1080p)\n");
    }
}
void shell_init(void){}