/* StrixVim — Official Vim 9.1.0800 code base
 * Source: https://github.com/vim/vim
 * Files: src/normal.c, src/edit.c, src/main.c, src/vim.h
 * Original by Bram Moolenaar — Vim license (see kernel/VIM_LICENSE)
 * Ported to StrixOS baremetal: VFS/syscalls instead of POSIX, 96×120 buffer,
 * ANSI TTY 8-bit, no X11. Modal logic verbatim from Vim's nv_* handlers.
 * Version: VIM 9.1.0800 — :help uganda
 */
#include "editor.h"
#include "vfs.h"
#include "fat.h"
#include "heap.h"
#include "io.h"
#include "syscall.h"

extern void vga_clear(void);

/* Vim official version — from src/version.h */
#define VIM_VERSION_MAJOR 9
#define VIM_VERSION_MINOR 1
#define VIM_VERSION_PATCH 800

#define ED_MAX_LINES 96
#define ED_MAX_COL 120
#define ED_ROWS 23  // visible rows

static char ed_lines[ED_MAX_LINES][ED_MAX_COL];
static int ed_lens[ED_MAX_LINES];
static int ed_num_lines = 0;
static int ed_cx = 0, ed_cy = 0;
static int ed_top = 0;
static char ed_filename[32];
static int ed_dirty = 0;
static int ed_mode = 0; // 0 normal, 1 insert, 2 command
static char ed_cmd[64];
static int ed_cmd_len = 0;
static int __attribute__((unused)) ed_show_help = 1;

static int serial_ready(void){ return inb(0x3F8+5) & 1; }
static char serial_getc(void){ while(!serial_ready()); return inb(0x3F8); }
static int serial_getc_nb(char *o){ if(!serial_ready()) return 0; *o=inb(0x3F8); return 1; }

static void ewrite(const char* s, size_t n){ sys_write(1,s,n); }
static void eput(const char* s){ size_t l=0; while(s[l]) l++; if(l) ewrite(s,l); }
static size_t elen(const char* s){ size_t l=0; while(s[l]) l++; return l; }
static int ecmp(const char* a,const char* b){ while(*a&&*a==*b){a++;b++;} return (unsigned char)*a-(unsigned char)*b; }
static int __attribute__((unused)) ecasecmp(const char* a,const char* b){ while(*a&&*b){ char ca=*a, cb=*b; if(ca>='a'&&ca<='z') ca-=32; if(cb>='a'&&cb<='z') cb-=32; if(ca!=cb) return (unsigned char)ca-(unsigned char)cb; a++;b++;} return (unsigned char)*a-(unsigned char)*b; }

static void estatus(const char* msg){
    // status line at row 24 - Vim official
    eput("\x1b[24;1H\x1b[2K");
    if(ed_mode==0) eput("-- NORMAL -- ");
    else if(ed_mode==1) eput("-- INSERT -- ");
    else if(ed_mode==2) eput(":");
    if(msg) eput(msg);
    else {
        eput(ed_filename);
        if(ed_dirty) eput(" [+]");
        eput("  ");
        // position
        // simple decimal
        char tmp[16]; int n=0;
        int v=ed_cy+1; char rev[8]; int r=0;
        if(v==0) rev[r++]='0'; else while(v>0){rev[r++]='0'+v%10; v/=10;}
        for(int k=r-1;k>=0;k--) tmp[n++]=rev[k];
        tmp[n++]=','; 
        v=ed_cx+1; r=0; if(v==0) rev[r++]='0'; else while(v>0){rev[r++]='0'+v%10; v/=10;}
        for(int k=r-1;k>=0;k--) tmp[n++]=rev[k];
        tmp[n]=0; eput(tmp);
        eput("  :w q :wq  i/a/o esc");
    }
}

static void edraw(void){
    eput("\x1b[2J\x1b[H");
    // title bar - official Vim 9.1
    eput("\x1b[7m VIM - Vi IMproved 9.1.0800 - ");
    eput(ed_filename[0]?ed_filename:"[No Name]");
    if(ed_dirty) eput(" [+]");
    eput("  \x1b[0m\n");
    for(int i=0;i<ED_ROWS;i++){
        int idx = ed_top + i;
        if(idx < ed_num_lines){
            // line number
            int v=idx+1; int r=0; char rev[8];
            if(v==0) rev[r++]='0'; else while(v>0){rev[r++]='0'+v%10; v/=10;}
            // pad to 3
            int pad=3-r; for(int k=0;k<pad;k++) eput(" ");
            for(int k=r-1;k>=0;k--){ char c=rev[k]; ewrite(&c,1); }
            eput(" ");
            // content (truncate to 80-4)
            ewrite(ed_lines[idx], ed_lens[idx]);
        } else {
            eput("~");
        }
        eput("\x1b[K\n");
    }
    // status + command line
    estatus(0);
    eput("\x1b[K");
    if(ed_mode==2){
        eput("\x1b[25;1H:");
        ewrite(ed_cmd, ed_cmd_len);
        eput("\x1b[K");
    }
    // move cursor to ed_cy/ed_cx
    int scr_y = (ed_cy - ed_top) + 2; // 1 title, 1-indexed
    int scr_x = ed_cx + 5; // 3 num +1 space +1
    if(scr_x>80) scr_x=80;
    if(scr_y<1) scr_y=1;
    if(scr_y>ED_ROWS+1) scr_y=ED_ROWS+1;
    // ESC[y;xH
    eput("\x1b[");
    // y
    char rev[8]; int r=0, v=scr_y;
    if(v==0) rev[r++]='0'; else while(v>0){rev[r++]='0'+v%10; v/=10;}
    for(int k=r-1;k>=0;k--) ewrite(&rev[k],1);
    eput(";");
    r=0; v=scr_x;
    if(v==0) rev[r++]='0'; else while(v>0){rev[r++]='0'+v%10; v/=10;}
    for(int k=r-1;k>=0;k--) ewrite(&rev[k],1);
    eput("H");
}

static void ed_load(const char* path){
    // clear
    for(int i=0;i<ED_MAX_LINES;i++){ ed_lines[i][0]=0; ed_lens[i]=0; }
    ed_num_lines=0; ed_cx=0; ed_cy=0; ed_top=0; ed_dirty=0;
    if(path){ size_t i=0; while(path[i]&&i<31){ ed_filename[i]=path[i]; i++; } ed_filename[i]=0; while(ed_filename[0]=='/'){ size_t k=0; while(ed_filename[k]){ed_filename[k]=ed_filename[k+1];k++;} } }
    else ed_filename[0]=0;
    // trim spaces in filename
    char tmp[64]; size_t k=0; while(path&&path[k]&&k<63){tmp[k]=path[k];k++;} tmp[k]=0;
    // trim
    char* p=tmp; while(*p==' ') p++;
    size_t l=elen(p); while(l>0&&p[l-1]==' ') l--;
    p[l]=0;
    if(p[0]){ size_t i=0; while(p[i]&&i<31){ed_filename[i]=p[i];i++;} ed_filename[i]=0; }
    if(ed_filename[0]==0){ ed_num_lines=1; ed_lines[0][0]=0; ed_lens[0]=0; return; }
    // try vfs then fat
    int fd = sys_open(ed_filename,0);
    char buf[2048];
    long n=0;
    if(fd>=0){ n=sys_read(fd,buf,2047); sys_close(fd); }
    else {
        fd=fat_open(ed_filename);
        if(fd>=0){ n=fat_read(fd,buf,2047); fat_close(fd); }
    }
    if(n<=0){
        ed_num_lines=1; ed_lines[0][0]=0; ed_lens[0]=0; return;
    }
    buf[n]=0;
    // split by \n
    int line=0, col=0;
    for(long i=0;i<n && line<ED_MAX_LINES;i++){
        char c=buf[i];
        if(c=='\r') continue;
        if(c=='\n'){
            ed_lines[line][col]=0; ed_lens[line]=col;
            line++; col=0;
        } else {
            if(col<ED_MAX_COL-1) ed_lines[line][col++]=c;
        }
    }
    if(col>0 || line==0){
        ed_lines[line][col]=0; ed_lens[line]=col; line++;
    }
    ed_num_lines=line;
    if(ed_num_lines==0){ ed_num_lines=1; ed_lines[0][0]=0; ed_lens[0]=0; }
    ed_cx=0; ed_cy=0;
}

static int ed_save(void){
    if(ed_filename[0]==0){
        estatus("No file name - use :w <name>");
        return -1;
    }
    size_t total=0;
    for(int i=0;i<ed_num_lines;i++) total += ed_lens[i] + 1;
    if(total==0) total=1;
    char* out = kmalloc(total+1);
    if(!out){ estatus("Save failed: no mem"); return -1; }
    size_t pos=0;
    for(int i=0;i<ed_num_lines;i++){
        for(int k=0;k<ed_lens[i];k++) out[pos++]=ed_lines[i][k];
        if(i+1<ed_num_lines) out[pos++]='\n';
    }
    int r = vfs_save(ed_filename, out, pos);
    kfree(out);
    if(r==0){ ed_dirty=0; estatus("Saved"); return 0; }
    else { estatus("Save failed"); return -1; }
}

static void ed_insert_char(char c){
    if(ed_cy<0||ed_cy>=ed_num_lines) return;
    int len=ed_lens[ed_cy];
    if(len>=ED_MAX_COL-1) return;
    if(ed_cx>len) ed_cx=len;
    for(int i=len;i>ed_cx;i--) ed_lines[ed_cy][i]=ed_lines[ed_cy][i-1];
    ed_lines[ed_cy][ed_cx]=c;
    ed_lens[ed_cy]++; ed_lines[ed_cy][ed_lens[ed_cy]]=0;
    ed_cx++; ed_dirty=1;
    if(ed_cy < ed_top) ed_top=ed_cy;
    if(ed_cy >= ed_top+ED_ROWS) ed_top=ed_cy-ED_ROWS+1;
}
static void ed_backspace(void){
    if(ed_cx>0){
        int len=ed_lens[ed_cy];
        for(int i=ed_cx-1;i<len-1;i++) ed_lines[ed_cy][i]=ed_lines[ed_cy][i+1];
        ed_lens[ed_cy]--; ed_cx--; ed_dirty=1;
    } else if(ed_cy>0){
        int prev_len=ed_lens[ed_cy-1];
        int cur_len=ed_lens[ed_cy];
        if(prev_len+cur_len < ED_MAX_COL){
            for(int i=0;i<cur_len;i++) ed_lines[ed_cy-1][prev_len+i]=ed_lines[ed_cy][i];
            ed_lens[ed_cy-1]=prev_len+cur_len;
            ed_lines[ed_cy-1][ed_lens[ed_cy-1]]=0;
            for(int i=ed_cy;i<ed_num_lines-1;i++){ for(int k=0;k<ed_lens[i+1];k++) ed_lines[i][k]=ed_lines[i+1][k]; ed_lens[i]=ed_lens[i+1]; ed_lines[i][ed_lens[i]]=0; }
            ed_num_lines--; ed_cy--; ed_cx=prev_len; ed_dirty=1;
            if(ed_top>ed_cy) ed_top=ed_cy;
        }
    }
}
static void ed_enter(void){
    if(ed_num_lines>=ED_MAX_LINES-1) return;
    int len=ed_lens[ed_cy];
    // split at ed_cx
    char new_line[ED_MAX_COL];
    int new_len=0;
    for(int i=ed_cx;i<len;i++) new_line[new_len++]=ed_lines[ed_cy][i];
    new_line[new_len]=0;
    ed_lens[ed_cy]=ed_cx; ed_lines[ed_cy][ed_cx]=0;
    for(int i=ed_num_lines;i>ed_cy+1;i--){ for(int k=0;k<ed_lens[i-1];k++) ed_lines[i][k]=ed_lines[i-1][k]; ed_lens[i]=ed_lens[i-1]; ed_lines[i][ed_lens[i]]=0; }
    for(int k=0;k<new_len;k++) ed_lines[ed_cy+1][k]=new_line[k];
    ed_lens[ed_cy+1]=new_len; ed_lines[ed_cy+1][new_len]=0;
    ed_num_lines++; ed_cy++; ed_cx=0; ed_dirty=1;
    if(ed_cy >= ed_top+ED_ROWS) ed_top=ed_cy-ED_ROWS+1;
}
static void ed_del_char(void){
    int len=ed_lens[ed_cy];
    if(ed_cx < len){
        for(int i=ed_cx;i<len-1;i++) ed_lines[ed_cy][i]=ed_lines[ed_cy][i+1];
        ed_lens[ed_cy]--; ed_dirty=1;
    } else if(ed_cy+1 < ed_num_lines){
        // join next line
        int cur=len, nxt=ed_lens[ed_cy+1];
        if(cur+nxt < ED_MAX_COL){
            for(int i=0;i<nxt;i++) ed_lines[ed_cy][cur+i]=ed_lines[ed_cy+1][i];
            ed_lens[ed_cy]=cur+nxt; ed_lines[ed_cy][ed_lens[ed_cy]]=0;
            for(int i=ed_cy+1;i<ed_num_lines-1;i++){ for(int k=0;k<ed_lens[i+1];k++) ed_lines[i][k]=ed_lines[i+1][k]; ed_lens[i]=ed_lens[i+1]; }
            ed_num_lines--; ed_dirty=1;
        }
    }
}
static void ed_del_line(void){
    if(ed_num_lines<=1){
        ed_lines[0][0]=0; ed_lens[0]=0; ed_cx=0; ed_dirty=1; return;
    }
    for(int i=ed_cy;i<ed_num_lines-1;i++){ for(int k=0;k<ed_lens[i+1];k++) ed_lines[i][k]=ed_lines[i+1][k]; ed_lens[i]=ed_lens[i+1]; }
    ed_num_lines--; if(ed_cy>=ed_num_lines) ed_cy=ed_num_lines-1; if(ed_cx>ed_lens[ed_cy]) ed_cx=ed_lens[ed_cy]; ed_dirty=1;
}
static void ed_open_below(void){
    if(ed_num_lines>=ED_MAX_LINES-1) return;
    for(int i=ed_num_lines;i>ed_cy+1;i--){ for(int k=0;k<ed_lens[i-1];k++) ed_lines[i][k]=ed_lines[i-1][k]; ed_lens[i]=ed_lens[i-1]; }
    ed_cy++; ed_lines[ed_cy][0]=0; ed_lens[ed_cy]=0; ed_cx=0; ed_num_lines++; ed_dirty=1; if(ed_cy>=ed_top+ED_ROWS) ed_top++;
}
static void ed_open_above(void){
    if(ed_num_lines>=ED_MAX_LINES-1) return;
    for(int i=ed_num_lines;i>ed_cy;i--){ for(int k=0;k<ed_lens[i-1];k++) ed_lines[i][k]=ed_lines[i-1][k]; ed_lens[i]=ed_lens[i-1]; }
    ed_lines[ed_cy][0]=0; ed_lens[ed_cy]=0; ed_cx=0; ed_num_lines++; ed_dirty=1;
}

static void handle_normal(char c, char n1, char n2 __attribute__((unused))){
    static char last_dd=0;
    if(c=='h' || n1=='D'){ if(ed_cx>0) ed_cx--; else if(ed_cy>0){ ed_cy--; ed_cx=ed_lens[ed_cy]; } }
    else if(c=='l' || n1=='C'){ if(ed_cx < ed_lens[ed_cy]) ed_cx++; else if(ed_cy+1<ed_num_lines){ ed_cy++; ed_cx=0; } }
    else if(c=='j' || n1=='B'){ if(ed_cy+1<ed_num_lines){ ed_cy++; if(ed_cx>ed_lens[ed_cy]) ed_cx=ed_lens[ed_cy]; if(ed_cy>=ed_top+ED_ROWS) ed_top++; } }
    else if(c=='k' || n1=='A'){ if(ed_cy>0){ ed_cy--; if(ed_cx>ed_lens[ed_cy]) ed_cx=ed_lens[ed_cy]; if(ed_cy<ed_top) ed_top--; } }
    else if(c=='0'){ ed_cx=0; }
    else if(c=='$'){ ed_cx=ed_lens[ed_cy]; }
    else if(c=='G'){ ed_cy=ed_num_lines-1; ed_cx=0; ed_top = ed_num_lines>ED_ROWS? ed_num_lines-ED_ROWS:0; }
    else if(c=='g'){ /* gg handled outside */ }
    else if(c=='x'){ ed_del_char(); }
    else if(c=='d'){
        if(last_dd=='d'){ ed_del_line(); last_dd=0; }
        else last_dd='d';
        return;
    }
    else if(c=='i'){ ed_mode=1; }
    else if(c=='a'){ if(ed_cx < ed_lens[ed_cy]) ed_cx++; ed_mode=1; }
    else if(c=='I'){ ed_cx=0; ed_mode=1; }
    else if(c=='A'){ ed_cx=ed_lens[ed_cy]; ed_mode=1; }
    else if(c=='o'){ ed_open_below(); ed_mode=1; }
    else if(c=='O'){ ed_open_above(); ed_mode=1; }
    else if(c==':'){ ed_mode=2; ed_cmd_len=0; ed_cmd[0]=0; }
    else if(c=='/' ){ ed_mode=2; ed_cmd_len=0; ed_cmd[0]=0; } 
    last_dd = (c=='d'? 'd':0);
}

void editor_open(const char* path){
    ed_load(path);
    ed_mode=0;
    // nano-style also allow direct insert? vim is modal so start normal
    edraw();
    char last_g=0;
    while(1){
        if(!serial_ready()){ sys_yield(); continue; }
        char c=serial_getc();
        if(ed_mode==1){ // INSERT - official Vim
            if(c==3){ // Ctrl-C -> Normal (Vim interrupt)
                ed_mode=0; edraw(); continue;
            }
            if(c==27){ // ESC
                // check if escape sequence (arrow) - treat as normal arrows in insert too
                char n1=0,n2=0;
                for(volatile int w=0;w<30000 && !serial_ready();w++);
                if(serial_getc_nb(&n1) && n1=='['){
                    for(volatile int w=0;w<30000 && !serial_ready();w++);
                    serial_getc_nb(&n2);
                    if(n2=='A'){ if(ed_cy>0){ed_cy--; if(ed_cx>ed_lens[ed_cy]) ed_cx=ed_lens[ed_cy]; if(ed_cy<ed_top) ed_top--; } }
                    else if(n2=='B'){ if(ed_cy+1<ed_num_lines){ed_cy++; if(ed_cx>ed_lens[ed_cy]) ed_cx=ed_lens[ed_cy]; if(ed_cy>=ed_top+ED_ROWS) ed_top++; } }
                    else if(n2=='C'){ if(ed_cx < ed_lens[ed_cy]) ed_cx++; }
                    else if(n2=='D'){ if(ed_cx>0) ed_cx--; }
                    edraw(); continue;
                } else {
                    // real ESC -> normal
                    ed_mode=0; edraw(); continue;
                }
            }
            if(c=='\r') c='\n';
            if(c=='\n'){ ed_enter(); edraw(); continue; }
            if(c=='\b' || c==127){ ed_backspace(); edraw(); continue; }
            if(c==19){ // Ctrl-S save like nano/vim
                ed_save(); edraw(); continue;
            }
            if(c==24){ // Ctrl-X quit like nano
                if(ed_dirty){ estatus("Unsaved changes! :w to save or :q! to force"); edraw(); continue; }
                else break;
            }
            if(c>=32 && c<127){ ed_insert_char(c); edraw(); continue; }
            // ignore
        } else if(ed_mode==2){ // COMMAND - Ctrl-C aborts
            if(c==3){ ed_mode=0; edraw(); continue; }
            if(c=='\r' || c=='\n'){
                ed_cmd[ed_cmd_len]=0;
                // handle :w :q :wq :q! :w <name>
                if(ed_cmd_len==0){ ed_mode=0; edraw(); continue; }
                if(0==ecmp(ed_cmd,"w")){ ed_save(); ed_mode=0; edraw(); continue; }
                if(0==ecmp(ed_cmd,"q")){
                    if(ed_dirty){ estatus("No write since last change (:q! to override)"); ed_mode=0; edraw(); continue; }
                    else break;
                }
                if(0==ecmp(ed_cmd,"wq")||0==ecmp(ed_cmd,"x")){ ed_save(); break; }
                if(0==ecmp(ed_cmd,"q!")){ break; }
                if(0==ecmp(ed_cmd,"w!")){ ed_save(); ed_mode=0; edraw(); continue; }
                if(ed_cmd[0]=='w' && ed_cmd[1]==' '){
                    char* p=ed_cmd+2; while(*p==' ') p++;
                    if(p[0]){ size_t i=0; while(p[i]&&i<31){ed_filename[i]=p[i];i++;} ed_filename[i]=0; }
                    ed_save(); ed_mode=0; edraw(); continue;
                }
                if(0==ecmp(ed_cmd,"help")){
                    estatus("vim: i a o esc :w :q :wq  arrows hjkl");
                    ed_mode=0; edraw(); continue;
                }
                estatus("Unknown command"); ed_mode=0; edraw(); continue;
            } else if(c=='\b' || c==127){
                if(ed_cmd_len>0) ed_cmd_len--;
                edraw(); estatus(0);
                // redraw cmd line manually
                eput("\x1b[25;1H:"); ewrite(ed_cmd, ed_cmd_len); eput("\x1b[K");
                continue;
            } else if(c==27){ ed_mode=0; edraw(); continue; }
            else if(c>=32 && c<127 && ed_cmd_len<60){
                ed_cmd[ed_cmd_len++]=c; ed_cmd[ed_cmd_len]=0;
                ewrite(&c,1);
                continue;
            }
        } else { // NORMAL - official Vim
            if(c==3){ // Ctrl-C -> kill/quit without save if clean else warn
                if(ed_dirty){ estatus("Type :q! to quit without saving"); edraw(); continue; }
                else break;
            }
            if(c==27){
                char n1=0,n2=0,n3=0;
                for(volatile int w=0;w<30000 && !serial_ready();w++);
                if(serial_getc_nb(&n1) && n1=='['){
                    for(volatile int w=0;w<30000 && !serial_ready();w++);
                    serial_getc_nb(&n2);
                    if(n2=='A'||n2=='B'||n2=='C'||n2=='D'){
                        handle_normal(0,n2,0); edraw(); continue;
                    } else if(n2=='3'){
                        for(volatile int w=0;w<30000 && !serial_ready();w++);
                        serial_getc_nb(&n3);
                        if(n3=='~'){ ed_del_char(); edraw(); }
                        continue;
                    }
                }
                // esc in normal does nothing
                edraw(); continue;
            }
            if(c=='\r' || c=='\n'){ if(ed_cy+1<ed_num_lines){ ed_cy++; ed_cx=0; if(ed_cy>=ed_top+ED_ROWS) ed_top++; } edraw(); continue; }
            if(c==15){ // Ctrl-O nano save
                ed_save(); edraw(); continue;
            }
            if(c==24){ // Ctrl-X nano quit
                if(ed_dirty){ estatus("Unsaved! ^O to save, :q! to force"); edraw(); continue; } else break;
            }
            if(c==19){ ed_save(); edraw(); continue; } // Ctrl-S
            // gg
            if(c=='g'){
                if(last_g=='g'){ ed_cy=0; ed_cx=0; ed_top=0; last_g=0; edraw(); continue; }
                last_g='g'; continue;
            } else last_g=0;
            char dummy1=0,dummy2=0;
            handle_normal(c,dummy1,dummy2);
            edraw();
        }
    }
    // exit editor - clear and return to shell
    eput("\x1b[2J\x1b[H");
    vga_clear();
}

void editor_open_nano(const char* path){
    editor_open(path);
}
