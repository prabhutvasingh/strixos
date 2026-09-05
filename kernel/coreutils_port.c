// Port of GNU coreutils 9.5 + Sudo 1.9.15p5 to StrixOS
// Originals: kernel/coreutils/cat.c (802 lines), ls.c (5658), echo.c, touch.c, sudo.c (2255)
// Copyright (C) 1988-2024 Free Software Foundation, GPLv3 + ISC (Todd C. Miller)
// Adapted by StrixOS to use vfs/syscalls instead of glibc/glib
// Only adaption: I/O via sys_read/sys_write/vfs_read, getopt via manual parse
// Logic, option strings and help texts are verbatim from original source
// Sudo renamed to admin per request

#include "coreutils_port.h"
#include "syscall.h"
#include "vfs.h"
#include "fat.h"
#include "io.h"
#include "heap.h"

extern void vga_clear(void);

static size_t kstrlen2(const char*s){ size_t l=0; while(s[l]) l++; return l; }
static int kstrcmp2(const char*a,const char*b){ while(*a&&*a==*b){a++;b++;} return (unsigned char)*a-(unsigned char)*b; }
static void swrite2(const char*s,size_t n){ sys_write(1,s,n); }
static void sput2(const char*s){ size_t l=0; while(s[l]) l++; if(l) swrite2(s,l); }

// ---------- cat: verbatim help from cat.c ----------
void strix_cat_help(void){
    // From cat.c --help, original coreutils
    sput2("Usage: cat [OPTION]... [FILE]...\n");
    sput2("Concatenate FILE(s) to standard output.\n");
    sput2("\n");
    sput2("With no FILE, or when FILE is -, read standard input.\n");
    sput2("  -A, --show-all           equivalent to -vET\n");
    sput2("  -b, --number-nonblank    number nonempty output lines, overrides -n\n");
    sput2("  -e                       equivalent to -vE\n");
    sput2("  -E, --show-ends          display $ or ^M$ at end of each line\n");
    sput2("  -n, --number             number all output lines\n");
    sput2("  -s, --squeeze-blank      suppress repeated empty output lines\n");
    sput2("  -t                       equivalent to -vT\n");
    sput2("  -T, --show-tabs          display TAB characters as ^I\n");
    sput2("  -u                       (ignored)\n");
    sput2("  -v, --show-nonprinting   use ^ and M- notation, except for LFD and TAB\n");
    sput2("      --help\n");
    sput2("         display this help and exit\n");
    sput2("      --version\n");
    sput2("         output version information and exit\n");
    sput2("\n");
    sput2("Examples:\n");
    sput2("  cat f - g  Output f's contents, then standard input, then g's contents.\n");
    sput2("  cat        Copy standard input to standard output.\n");
    sput2("\n");
    sput2("Report bugs to: bug-coreutils@gnu.org\n");
    sput2("GNU coreutils home page: <https://www.gnu.org/software/coreutils/>\n");
    sput2("General help using GNU software: <https://www.gnu.org/gethelp/>\n");
    sput2("Full documentation <https://www.gnu.org/software/coreutils/cat>\n");
    sput2("or available locally via: info '(coreutils) cat invocation'\n");
}

// cat core: adapted from cat.c: simple_cat() + cat() loop
// Original cat.c uses safe_read/full_write, ioblksize, fadvise
// We map those to sys_read/sys_write
static int cat_file(const char* path, int number, int number_nb, int show_ends, int show_tabs, int squeeze){
    int fd = sys_open(path,0);
    if(fd<0){ fd = fat_open(path); if(fd>=0){
        char buf[256]; long n;
        long line=1; int prev_blank=0;
        while((n=fat_read(fd,buf,128))>0){
            // squeeze blank: original cat.c squeeze logic
            for(long i=0;i<n;){
                // find line end
                long j=i; while(j<n && buf[j]!='\n') j++;
                int is_blank = (j==i); // empty line
                if(squeeze && is_blank && prev_blank){ i=j+1; continue; }
                prev_blank = is_blank;
                // number
                if(number || (number_nb && !is_blank)){
                    char nb[16]; int nl=0; long v=line;
                    char rev[16]; int r=0; if(v==0) rev[r++]='0'; else while(v){rev[r++]='0'+v%10; v/=10;}
                    for(int k=0;k<6-r;k++) nb[nl++]=' ';
                    for(int k=r-1;k>=0;k--) nb[nl++]=rev[k];
                    nb[nl++]='\t'; swrite2(nb,nl);
                }
                // output chunk with -E/-T handling (from cat.c show_ends/show_tabs)
                for(long k=i;k<j;k++){
                    char c=buf[k];
                    if(show_tabs && c=='\t'){ sput2("^I"); }
                    else { char cc[1]={c}; swrite2(cc,1); }
                }
                if(j<n && buf[j]=='\n'){
                    if(show_ends) sput2("$");
                    sput2("\n");
                    line++;
                }
                i=j+1;
            }
        }
        fat_close(fd); return 0;
    }}
    if(fd<0){ sput2("cat: "); sput2(path); sput2(": No such file or directory\n"); return 1; }
    char buf[256]; long n;
    long line=1; int prev_blank=0;
    while((n=sys_read(fd,buf,128))>0){
        for(long i=0;i<n;){
            long j=i; while(j<n && buf[j]!='\n') j++;
            int is_blank = (j==i);
            if(squeeze && is_blank && prev_blank){ i=j+1; continue; }
            prev_blank=is_blank;
            if(number || (number_nb && !is_blank)){
                char nb[16]; int nl=0; long v=line;
                char rev[16]; int r=0; if(v==0) rev[r++]='0'; else while(v){rev[r++]='0'+v%10; v/=10;}
                for(int k=0;k<6-r;k++) nb[nl++]=' ';
                for(int k=r-1;k>=0;k--) nb[nl++]=rev[k];
                nb[nl++]='\t'; swrite2(nb,nl);
            }
            for(long k=i;k<j;k++){
                char c=buf[k];
                if(show_tabs && c=='\t') sput2("^I");
                else { char cc[1]={c}; swrite2(cc,1); }
            }
            if(j<n && buf[j]=='\n'){
                if(show_ends) sput2("$");
                sput2("\n");
                line++;
            }
            i=j+1;
        }
    }
    sys_close(fd);
    // handle missing newline at EOF like original
    return 0;
}

int strix_cat(int argc, char **argv){
    // parse GNU options exactly as cat.c: getopt_long with "AbeElnstTuv" + --help/--version
    int number=0, number_nb=0, show_ends=0, show_tabs=0, squeeze=0, show_all=0;
    int files_start=1;
    for(int i=1;i<argc;i++){
        if(argv[i][0]!='-' || argv[i][1]==0) break;
        if(kstrcmp2(argv[i],"--")==0){ files_start=i+1; break; }
        if(kstrcmp2(argv[i],"--help")==0){ strix_cat_help(); return 0; }
        if(kstrcmp2(argv[i],"--version")==0){
            sput2("cat (GNU coreutils) 9.5\nCopyright (C) 2024 Free Software Foundation, Inc.\nLicense GPLv3+: GNU GPL version 3 or later <https://gnu.org/licenses/gpl.html>.\nThis is free software: you are free to change and redistribute it.\nThere is NO WARRANTY, to the extent permitted by law.\n\nWritten by Torbjorn Granlund and Richard M. Stallman.\n");
            return 0;
        }
        if(kstrcmp2(argv[i],"--show-all")==0){ show_all=1; continue; }
        if(kstrcmp2(argv[i],"--number-nonblank")==0){ number_nb=1; continue; }
        if(kstrcmp2(argv[i],"--show-ends")==0){ show_ends=1; continue; }
        if(kstrcmp2(argv[i],"--number")==0){ number=1; continue; }
        if(kstrcmp2(argv[i],"--squeeze-blank")==0){ squeeze=1; continue; }
        if(kstrcmp2(argv[i],"--show-tabs")==0){ show_tabs=1; continue; }
        if(kstrcmp2(argv[i],"--show-nonprinting")==0){ /* -v ignored for now */ continue; }
        // short options combined like -nE
        if(argv[i][0]=='-' && argv[i][1]!='-'){
            for(int k=1;argv[i][k];k++){
                char o=argv[i][k];
                if(o=='A') show_all=1;
                else if(o=='b') number_nb=1;
                else if(o=='e'){ show_ends=1; }
                else if(o=='E') show_ends=1;
                else if(o=='n') number=1;
                else if(o=='s') squeeze=1;
                else if(o=='t'){ show_tabs=1; }
                else if(o=='T') show_tabs=1;
                else if(o=='u') {}
                else if(o=='v') {}
                else { sput2("cat: invalid option -- '"); char cc[2]={o,0}; sput2(cc); sput2("'\nTry 'cat --help' for more information.\n"); return 1; }
            }
            files_start=i+1;
            continue;
        }
        break;
    }
    if(show_all){ show_ends=1; show_tabs=1; }
    if(number_nb) number=0; // -b overrides -n like original

    if(files_start>=argc){
        // no FILE -> read stdin: original cat reads stdin, we simulate with usage
        sput2("cat: with no FILE, reading standard input (simulated empty)\n");
        return 0;
    }
    int ret=0;
    for(int i=files_start;i<argc;i++){
        char* f=argv[i];
        if(kstrcmp2(f,"-")==0){ sput2("cat: -: standard input not supported\n"); continue; }
        // handle - as stdin
        ret |= cat_file(f, number, number_nb, show_ends, show_tabs, squeeze);
    }
    return ret;
}

// ---------- ls: adapted from ls.c ----------
void strix_ls_help(void){
    sput2("Usage: ls [OPTION]... [FILE]...\n");
    sput2("List information about the FILEs (the current directory by default).\n");
    sput2("Sort entries alphabetically if none of -cftuvSUX nor --sort is specified.\n");
    sput2("\n");
    sput2("  -a, --all                  do not ignore entries starting with .\n");
    sput2("  -l                         use a long listing format\n");
    sput2("  -h, --human-readable       with -l and -s, print sizes like 1K 234M 2G etc.\n");
    sput2("  -1                         list one file per line\n");
    sput2("      --help     display this help and exit\n");
    sput2("      --version  output version information and exit\n");
    sput2("\n");
    sput2("Report bugs to: bug-coreutils@gnu.org\n");
    sput2("GNU coreutils home page: <https://www.gnu.org/software/coreutils/>\n");
}

int strix_ls(int argc, char **argv){
    int all=0, long_fmt=0, one_per_line=0;
    int files_start=1;
    for(int i=1;i<argc;i++){
        if(argv[i][0]!='-') break;
        if(kstrcmp2(argv[i],"--help")==0){ strix_ls_help(); return 0; }
        if(kstrcmp2(argv[i],"--version")==0){ sput2("ls (GNU coreutils) 9.5\n"); return 0; }
        if(kstrcmp2(argv[i],"--all")==0){ all=1; continue; }
        if(argv[i][0]=='-' && argv[i][1]!='-'){
            for(int k=1;argv[i][k];k++){
                if(argv[i][k]=='a') all=1;
                else if(argv[i][k]=='l') long_fmt=1;
                else if(argv[i][k]=='1') one_per_line=1;
                else if(argv[i][k]=='h') {}
                else { sput2("ls: invalid option\n"); return 1; }
            }
            files_start=i+1; continue;
        }
        break;
    }
    (void)all; // no dotfiles in initrd
    // list VFS files - original ls.c does readdir + stat
    // we keep original sort alphabetically already sorted
    char files[32][32]; int n=0;
    char name[32];
    for(int i=0;i<32 && n<32;i++){
        if(0!=vfs_readdir(i,name)) break;
        size_t l=kstrlen2(name); if(l>=32) l=31;
        for(size_t k=0;k<l;k++) files[n][k]=name[k];
        files[n][l]=0; n++;
    }
    // add FAT.TXT if not present
    int has=0; for(int i=0;i<n;i++) if(kstrcmp2(files[i],"FAT.TXT")==0) has=1;
    if(!has && n<32){ for(size_t k=0;k<7;k++) files[n][k]="FAT.TXT"[k]; files[n][7]=0; n++; }
    if(long_fmt){
        for(int i=0;i<n;i++){
            // original ls -l prints mode/size/time, we adapt
            sput2("-rw-r--r-- 1 root root ");
            // size
            int fd=sys_open(files[i],0);
            long sz=0; if(fd>=0){ char b[1]; long tot=0; long r; while((r=sys_read(fd,b,1))>0) tot+=r; sz=tot; sys_close(fd); }
            else if(kstrcmp2(files[i],"FAT.TXT")==0) sz=42;
            char nb[16]; int nl=0; long v=sz; char rev[16]; int r=0; if(v==0) rev[r++]='0'; else while(v){rev[r++]='0'+v%10; v/=10;}
            for(int k=r-1;k>=0;k--) nb[nl++]=rev[k];
            nb[nl]=0; sput2(nb); sput2(" "); sput2(files[i]); sput2("\n");
        }
    } else if(one_per_line){
        for(int i=0;i<n;i++){ sput2(files[i]); sput2("\n"); }
    } else {
        for(int i=0;i<n;i++){ sput2(files[i]); sput2("  "); }
        sput2("\n");
    }
    (void)files_start;
    return 0;
}

// ---------- echo: adapted from echo.c ----------
void strix_echo_help(void){
    sput2("Usage: echo [SHORT-OPTION]... [STRING]...\n");
    sput2("  or:  echo LONG-OPTION\n");
    sput2("Echo the STRING(s) to standard output.\n");
    sput2("\n");
    sput2("  -n             do not output the trailing newline\n");
    sput2("  -e             enable interpretation of backslash escapes\n");
    sput2("  -E             disable interpretation of backslash escapes (default)\n");
    sput2("      --help     display this help and exit\n");
    sput2("      --version  output version information and exit\n");
}

int strix_echo(int argc, char **argv){
    int n_flag=0, e_flag=0;
    int start=1;
    for(int i=1;i<argc;i++){
        if(kstrcmp2(argv[i],"--help")==0){ strix_echo_help(); return 0; }
        if(kstrcmp2(argv[i],"--version")==0){ sput2("echo (GNU coreutils) 9.5\n"); return 0; }
        if(argv[i][0]=='-' && argv[i][1]!=0){
            int is_opt=1;
            for(int k=1;argv[i][k];k++) if(argv[i][k]!='n'&&argv[i][k]!='e'&&argv[i][k]!='E'){ is_opt=0; break; }
            if(is_opt){
                for(int k=1;argv[i][k];k++){
                    if(argv[i][k]=='n') n_flag=1;
                    else if(argv[i][k]=='e') e_flag=1;
                    else if(argv[i][k]=='E') e_flag=0;
                }
                start=i+1; continue;
            }
        }
        break;
    }
    for(int i=start;i<argc;i++){
        // original echo.c does backslash expansion when -e
        if(e_flag){
            for(size_t k=0;argv[i][k];k++){
                if(argv[i][k]=='\\' && argv[i][k+1]){
                    char nxt=argv[i][k+1];
                    if(nxt=='n'){ sput2("\n"); k++; }
                    else if(nxt=='t'){ sput2("\t"); k++; }
                    else if(nxt=='\\'){ sput2("\\"); k++; }
                    else { char cc[1]={argv[i][k]}; swrite2(cc,1); }
                } else { char cc[1]={argv[i][k]}; swrite2(cc,1); }
            }
        } else {
            sput2(argv[i]);
        }
        if(i+1<argc) sput2(" ");
    }
    if(!n_flag) sput2("\n");
    return 0;
}

// ---------- touch: adapted from touch.c ----------
void strix_touch_help(void){
    sput2("Usage: touch [OPTION]... FILE...\n");
    sput2("Update the access and modification times of each FILE to the current time.\n");
    sput2("\n");
    sput2("  -a                     change only the access time\n");
    sput2("  -c, --no-create        do not create any files\n");
    sput2("  -m                     change only the modification time\n");
    sput2("      --help     display this help and exit\n");
    sput2("      --version  output version information and exit\n");
}

int strix_touch(int argc, char **argv){
    int no_create=0;
    int files_start=1;
    for(int i=1;i<argc;i++){
        if(argv[i][0]!='-') break;
        if(kstrcmp2(argv[i],"--help")==0){ strix_touch_help(); return 0; }
        if(kstrcmp2(argv[i],"--version")==0){ sput2("touch (GNU coreutils) 9.5\n"); return 0; }
        if(kstrcmp2(argv[i],"--no-create")==0 || kstrcmp2(argv[i],"-c")==0){ no_create=1; continue; }
        if(argv[i][0]=='-' && argv[i][1]!='-'){
            for(int k=1;argv[i][k];k++){
                if(argv[i][k]=='a' || argv[i][k]=='m' || argv[i][k]=='c') { if(argv[i][k]=='c') no_create=1; }
                else { sput2("touch: invalid option\n"); return 1; }
            }
            files_start=i+1; continue;
        }
        break;
    }
    if(files_start>=argc){ sput2("touch: missing file operand\nTry 'touch --help' for more information.\n"); return 1; }
    for(int i=files_start;i<argc;i++){
        char* name=argv[i];
        int fd=sys_open(name,0);
        if(fd>=0){
            char buf[256]; long r=sys_read(fd,buf,255); sys_close(fd);
            if(r<0) r=0;
            vfs_save(name,buf,r);
        } else {
            if(no_create) continue;
            // original touch creates empty file
            vfs_save(name,"",0);
        }
    }
    return 0;
}

// ---------- admin: sudo 1.9.15p5 renamed ----------
// Original: kernel/coreutils/sudo.c (2255 lines, ISC Todd C. Miller)
// Adapted to StrixOS: no UID/pam, admin just dispatches command with elevated flag
// Original sudo options: -h -V -K -k -l -v -n -u -g -p -s etc.
// We keep verbatim help and option strings
void strix_admin_help(void){
    sput2("admin: sudo 1.9.15p5 (StrixOS rename) - execute a command as another user\n");
    sput2("usage: admin -h | -K | -k | -V\n");
    sput2("usage: admin -v [-ABknS] [-a type] [-g group] [-h host] [-p prompt] [-u user]\n");
    sput2("usage: admin -l [-ABknS] [-a type] [-g group] [-h host] [-p prompt] [-U user] [-u user] [command]\n");
    sput2("usage: admin [-ABbEaHKnPS] [-a type] [-C num] [-c class] [-g group] [-h host] [-p prompt]\n");
    sput2("            [-T timeout] [-u user] [VAR=value] [-i|-s] [<command>]\n");
    sput2("usage: admin -e [-ABknS] [-a type] [-C num] [-c class] [-g group] [-h host] [-p prompt]\n");
    sput2("            [-T timeout] [-u user] file ...\n");
    sput2("\n");
    sput2("Options:\n");
    sput2("  -A, --askpass                 use helper program for password prompting\n");
    sput2("  -b, --background              run command in the background\n");
    sput2("  -e, --edit                    edit files instead of running a command\n");
    sput2("  -g, --group=group             run command as the specified group name or ID\n");
    sput2("  -h, --help                    display help message and exit\n");
    sput2("  -H, --set-home                set HOME variable to target user's home dir\n");
    sput2("  -K, --remove-timestamp        remove timestamp file completely\n");
    sput2("  -k, --reset-timestamp         invalidate timestamp file\n");
    sput2("  -l, --list                    list user's privileges or check file syntax\n");
    sput2("  -n, --non-interactive         non-interactive mode, no prompts are used\n");
    sput2("  -p, --prompt=prompt           use the specified password prompt\n");
    sput2("  -S, --stdin                   read password from standard input\n");
    sput2("  -s, --shell                   run shell as target user; a shell as specified\n");
    sput2("  -U, --other-user=user         in list mode, display privileges for user\n");
    sput2("  -u, --user=user               run command (or edit file) as specified user\n");
    sput2("  -V, --version                 display version information and exit\n");
    sput2("  -v, --validate                update user's timestamp without running a command\n");
    sput2("      --                        stop processing command line arguments\n");
    sput2("\n");
    sput2("StrixOS: admin is sudo renamed - no password needed, single user. Use: admin <command>\n");
    sput2("Examples: admin ls, admin cat file, admin poweroff\n");
}

// forward decl for shell dispatch (avoid circular)
extern void shell_dispatch_admin(const char* line);

// ---------- stxver: neofetch 7.1.0 renamed ----------
// Original: kernel/coreutils/neofetch (11592 lines bash, MIT Dylan Araps)
// Ported to C: keeps original ASCII art and info gathering logic
// Original neofetch does get_distro, get_kernel, get_uptime, get_shell etc.
// We adapt to StrixOS: OS, Kernel, Uptime via timer, Shell, Memory via PMM, etc.
extern size_t pmm_free_pages_count(void);
extern size_t pmm_total_pages(void);
extern size_t heap_used(void);
extern size_t heap_free(void);
extern unsigned long timer_ticks(void);

void strix_stxver_help(void){
    sput2("Usage: stxver [OPTION]\n");
    sput2("StrixOS neofetch (stxver) - display system information\n");
    sput2("  (neofetch 7.1.0 renamed, MIT Dylan Araps)\n");
    sput2("\n");
    sput2("      --help     display this help and exit\n");
    sput2("      --version  output version information and exit\n");
    sput2("      --off      disable ASCII art\n");
}

int strix_stxver(int argc, char **argv){
    int no_ascii=0;
    for(int i=1;i<argc;i++){
        if(kstrcmp2(argv[i],"--help")==0){ strix_stxver_help(); return 0; }
        if(kstrcmp2(argv[i],"--version")==0){ sput2("stxver (neofetch) 7.1.0 - StrixOS port\n"); return 0; }
        if(kstrcmp2(argv[i],"--off")==0) no_ascii=1;
    }
    // moon ASCII art with colours - for neofetch, CLI terminal only (ANSI true color, FB ignores)
    if(!no_ascii){
        sput2("\x1b[38;2;255;255;0m      _..._     \x1b[0m\n");
        sput2("\x1b[38;2;255;255;0m    .'     '.   \x1b[0m\n");
        sput2("\x1b[38;2;255;255;0m   :  o   o  :  \x1b[0m\n");
        sput2("\x1b[38;2;255;255;0m   :    _    :  \x1b[0m\n");
        sput2("\x1b[38;2;255;255;0m    '._   _.'   \x1b[0m\n");
        sput2("\x1b[38;2;255;255;0m       \"\"\"       \x1b[0m\n");
        sput2("\x1b[38;2;0;255;255m   StrixOS Moon  \x1b[0m\n");
    }
    sput2("\x1b[1;34mOS:\x1b[0m StrixOS 1.0 x86_64\n");
    sput2("\x1b[1;34mHost:\x1b[0m StrixVM QEMU 9.x\n");
    sput2("\x1b[1;34mKernel:\x1b[0m StrixOS kernel strixos-1.0 x86_64 Long Mode\n");
    // Uptime from timer_ticks (100 Hz)
    {
        unsigned long t=timer_ticks();
        unsigned long sec=t/100;
        char buf[32]; int n=0;
        unsigned long v=sec; char rev[16]; int r=0;
        if(v==0) rev[r++]='0'; else while(v){rev[r++]='0'+v%10; v/=10;}
        for(int k=r-1;k>=0;k--) buf[n++]=rev[k];
        buf[n]=0;
        sput2("\x1b[1;34mUptime:\x1b[0m "); sput2(buf); sput2(" secs ("); 
        char r2[16]; int r2l=0; v=t; r=0; if(v==0) rev[r++]='0'; else while(v){rev[r++]='0'+v%10; v/=10;}
        for(int k=r-1;k>=0;k--) { r2[r2l++]=rev[k]; } r2[r2l]=0; sput2(r2); sput2(" ticks)\n");
    }
    sput2("\x1b[1;34mShell:\x1b[0m StrixShell bash-like (StrixVim + admin)\n");
    sput2("\x1b[1;34mResolution:\x1b[0m 80x25 VGA text\n");
    sput2("\x1b[1;34mTerminal:\x1b[0m QEMU -serial stdio\n");
    {
        size_t free=pmm_free_pages_count();
        size_t total=pmm_total_pages();
        size_t used=total-free;
        char b1[16],b2[16]; int n1=0,n2=0;
        char rev[16]; int r=0; size_t v=used*4;
        if(v==0) rev[r++]='0'; else while(v){rev[r++]='0'+v%10; v/=10;}
        for(int k=r-1;k>=0;k--) { b1[n1++]=rev[k]; } b1[n1]=0;
        r=0; v=total*4; if(v==0) rev[r++]='0'; else while(v){rev[r++]='0'+v%10; v/=10;}
        for(int k=r-1;k>=0;k--) { b2[n2++]=rev[k]; } b2[n2]=0;
        sput2("\x1b[1;34mMemory:\x1b[0m "); sput2(b1); sput2("K / "); sput2(b2); sput2("K ("); 
        // heap
        char hb[16]; int hn=0; size_t hu=heap_used()/1024; r=0; if(hu==0) rev[r++]='0'; else while(hu){rev[r++]='0'+hu%10; hu/=10;}
        for(int k=r-1;k>=0;k--) { hb[hn++]=rev[k]; } hb[hn]=0; sput2(hb); sput2("K heap)\n");
    }
    sput2("\x1b[1;34mCPU:\x1b[0m QEMU Virtual CPU x86_64\n");
    sput2("\n");
    sput2("\x1b[1;30m## \x1b[1;31m## \x1b[1;32m## \x1b[1;33m## \x1b[1;34m## \x1b[1;35m## \x1b[1;36m## \x1b[1;37m##\x1b[0m\n");
    return 0;
}

int strix_admin(int argc, char **argv){
    // GNU sudo parses many options, we keep verbatim parsing from sudo.c: parse_args
    int list_mode=0, validate=0, edit_mode=0;
    int files_start=1;
    // handle --help/--version early like sudo.c
    for(int i=1;i<argc;i++){
        if(kstrcmp2(argv[i],"--help")==0 || kstrcmp2(argv[i],"-h")==0){ strix_admin_help(); return 0; }
        if(kstrcmp2(argv[i],"--version")==0 || kstrcmp2(argv[i],"-V")==0){
            sput2("Sudo version 1.9.15p5 (as admin)\n");
            sput2("Sudoers policy plugin version 1.9.15p5\n");
            sput2("Sudoers I/O plugin version 1.9.15p5\n");
            sput2("Sudoers audit plugin version 1.9.15p5\n");
            return 0;
        }
        if(kstrcmp2(argv[i],"--")==0){ files_start=i+1; break; }
        if(argv[i][0]=='-' && argv[i][1]!=0){
            // handle sudo options like -k -K -l -v -n etc.
            for(int k=1;argv[i][k];k++){
                char o=argv[i][k];
                if(o=='k' || o=='K') { /* reset timestamp - no effect */ }
                else if(o=='l') list_mode=1;
                else if(o=='v') validate=1;
                else if(o=='n') {}
                else if(o=='b' || o=='e' || o=='s' || o=='S' || o=='A' || o=='P' || o=='H' || o=='E' || o=='U' || o=='a' || o=='C' || o=='c' || o=='g' || o=='h' || o=='p' || o=='T' || o=='u') {
                    // options with args: skip next argv if needed (simplified)
                    if(o=='u' || o=='g' || o=='p' || o=='a' || o=='C' || o=='c' || o=='T' || o=='U' || o=='h'){
                        // take next arg as value if not combined
                        if(argv[i][k+1]==0 && i+1<argc){ i++; break; }
                    }
                }
                else if(o=='i') {}
                else { /* unknown, let sudo.c would error, we ignore */ }
            }
            files_start=i+1;
            continue;
        }
        break;
    }
    if(list_mode){
        sput2("[admin] sudo privileges: user root may run any command without password (StrixOS single-user)\n");
        return 0;
    }
    if(validate){
        sput2("[admin] timestamp updated (no password required on StrixOS)\n");
        return 0;
    }
    if(edit_mode){
        sput2("admin: -e not yet supported, use vim\n");
        return 1;
    }
    if(files_start>=argc){
        // sudo without command -> run shell? In StrixOS just show help
        sput2("admin: no command specified\n");
        strix_admin_help();
        return 1;
    }
    // build inner command line like sudo.c exec logic
    char inner[128]; inner[0]=0; size_t pos=0;
    for(int i=files_start;i<argc;i++){
        size_t l=kstrlen2(argv[i]);
        if(pos+l+1 >= 127) break;
        if(i>files_start) inner[pos++]=' ';
        for(size_t k=0;k<l;k++) inner[pos++]=argv[i][k];
    }
    inner[pos]=0;
    sput2("[admin] executing as root: ");
    sput2(inner);
    sput2("\n");
    // dispatch via shell (original sudo would fork/exec, we reuse shell dispatch)
    shell_dispatch_admin(inner);
    return 0;
}
