#include "tty.h"
#include "io.h"
#include "syscall.h"
#include "fb.h"
#include "keyboard.h"

extern void vga_clear(void);

#define MAX_TTY 4

static int current_tty = 0;
static char tty_user[MAX_TTY][32];

void tty_init(void){
    for(int i=0;i<MAX_TTY;i++) tty_user[i][0]=0;
    current_tty=0;
}

void tty_clear(void){
    if(fb_is_graphics()){
        fb_clear(0);
    } else vga_clear();
    // don't send ANSI to FB - it renders as ←[2J garbage; serial already cleared via vga_clear/fb_clear
    if(!fb_is_graphics()) sys_write(1, "\x1b[2J\x1b[H",7);
}

int tty_current(void){ return current_tty; }
void tty_switch(int id){
    if(id<0||id>=MAX_TTY) return;
    current_tty=id;
    tty_clear();
}

static __attribute__((unused)) int serial_ready(void){ return inb(0x3F8+5)&1; }
static __attribute__((unused)) char serial_getc(void){ while(!serial_ready()); return inb(0x3F8); }

static int is_admin_user(const char* u){
    if(!u) return 0;
    // avi and root are admin
    const char* a="avi"; int i=0; while(a[i]&&u[i]&&a[i]==u[i]) i++; if(a[i]==0 && u[i]==0) return 1;
    const char* r="root"; i=0; while(r[i]&&u[i]&&r[i]==u[i]) i++; return r[i]==0 && u[i]==0;
}
int tty_is_admin(void){ return is_admin_user(tty_user[current_tty]); }
void tty_login(int tty_id){
    if(tty_id<0||tty_id>=MAX_TTY) tty_id=0;
    current_tty=tty_id;
    char user[32];
    char pwd[32];
    static int first_login=1;
    while(1){
        if(first_login){
            tty_clear();
            first_login=0;
        } else {
            // don't clear every retry - just newline to avoid flicker/resize loop
            sys_write(1,"\n",1);
        }
        const char* banner="StrixOS TTY";
        sys_write(1, banner, 11);
        char idc[4]; idc[0]='0'+tty_id; idc[1]=0;
        sys_write(1, " ",1); sys_write(1,idc,1);
        if(fb_is_graphics()){
            sys_write(1, " - ",3);
            char wh[32]; int n=0;
            int w=fb_get_width(), h=fb_get_height();
            char rev[8]; int r=0;
            int v=w; if(v==0) rev[r++]='0'; else while(v){rev[r++]='0'+v%10; v/=10;}
            for(int k=r-1;k>=0;k--) wh[n++]=rev[k];
            wh[n++]='x';
            r=0; v=h; if(v==0) rev[r++]='0'; else while(v){rev[r++]='0'+v%10; v/=10;}
            for(int k=r-1;k>=0;k--) wh[n++]=rev[k];
            wh[n]=0; sys_write(1, wh, n);
            sys_write(1, " RGB",4);
        }
        sys_write(1,"\n",1);
        // User Login: single space (was "User Login:   " 3 spaces → gap reported)
        const char* prompt="User Login: ";
        sys_write(1,prompt,12);
        // read user: allow both QEMU window (PS/2) and serial
        int pos=0; user[0]=0;
        while(1){
            char c = keyboard_getc(); // polls PS/2 + serial
            if(c=='\r') c='\n';
            if(c=='\n'){
                sys_write(1,"\n",1);
                user[pos]=0;
                break;
            }
            if(c=='\b' || c==127){
                if(pos>0){ pos--; sys_write(1,"\b \b",3); }
                continue;
            }
            if(c>=32 && c<127 && pos<31){
                user[pos++]=c;
                sys_write(1,&c,1);
            }
        }
        // trim spaces
        char* p=user; while(*p==' ') p++;
        size_t l=0; while(p[l]) l++;
        while(l>0 && p[l-1]==' ') l--;
        p[l]=0;
        if(p[0]==0){
            sys_write(1,"Login failed - empty user\n",26);
            continue;
        }
        // only avi and root exist; others -> invalid
        {
            int is_avi=1; const char* a="avi"; int i=0; while(a[i]&&p[i]&&a[i]==p[i]) i++; if(a[i]!=0||p[i]!=0) is_avi=0;
            int is_root=1; const char* r="root"; i=0; while(r[i]&&p[i]&&r[i]==p[i]) i++; if(r[i]!=0||p[i]!=0) is_root=0;
            if(!is_avi && !is_root){
                sys_write(1,"Login incorrect: invalid credentials\n",37);
                continue;
            }
        }
        // password: root = none, avi = power
        {
            int is_root=1; const char* r="root"; int i=0; while(r[i]&&p[i]&&r[i]==p[i]) i++; if(r[i]!=0||p[i]!=0) is_root=0;
            if(is_root){
                // no password for root
            } else if(is_admin_user(p)){
            sys_write(1,"Password: ",10);
            int pp=0; pwd[0]=0;
            while(1){
                char c=keyboard_getc();
                if(c=='\r') c='\n';
                if(c=='\n'){ sys_write(1,"\n",1); pwd[pp]=0; break; }
                if(c=='\b'||c==127){ if(pp>0){ pp--; sys_write(1,"\b \b",3); } continue; }
                if(c>=32&&c<127&&pp<31){ pwd[pp++]=c; sys_write(1,"*",1); }
            }
            // check power
            const char* pw="power"; int ok=1; int k=0; while(pw[k]&&pwd[k]&&pw[k]==pwd[k]) k++; if(pw[k]!=0||pwd[k]!=0) ok=0;
            if(!ok){ sys_write(1,"Login incorrect\n",16); continue; }
            }
        }
        // store
        size_t i=0; while(p[i]&&i<31){tty_user[tty_id][i]=p[i];i++;} tty_user[tty_id][i]=0;
        const char* welcome="Welcome, ";
        sys_write(1,welcome,9); sys_write(1,tty_user[tty_id], l);
        if(is_admin_user(p)) sys_write(1," (admin)",8);
        sys_write(1,"!\n",2);
        // small delay
        for(volatile int d=0;d<2000000;d++);
        break;
    }
}
