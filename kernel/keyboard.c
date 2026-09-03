#include "keyboard.h"
#include "io.h"
#include "fb.h"

static int shift = 0;
static int caps = 0;

// scancode set 1 -> ascii without shift
static const char sc_ascii[] = {
 0, 27, '1','2','3','4','5','6','7','8','9','0','-','=', '\b',
 '\t','q','w','e','r','t','y','u','i','o','p','[',']','\n', 0,
 'a','s','d','f','g','h','j','k','l',';','\'','`', 0,'\\',
 'z','x','c','v','b','n','m',',','.','/', 0, '*', 0, ' ',
 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
};
static const char sc_ascii_shift[] = {
 0, 27, '!','@','#','$','%','^','&','*','(',')','_','+', '\b',
 '\t','Q','W','E','R','T','Y','U','I','O','P','{','}','\n', 0,
 'A','S','D','F','G','H','J','K','L',':','"','~', 0,'|',
 'Z','X','C','V','B','N','M','<','>','?', 0, '*', 0, ' ',
 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
};

#define KBD_BUF 128
static char kbuf[KBD_BUF];
static int khead=0, ktail=0;

static void kpush(char c){
    int n=(khead+1)%KBD_BUF;
    if(n!=ktail){ kbuf[khead]=c; khead=n; }
}
static void handle_scancode(uint8_t sc);
void keyboard_isr_push(uint8_t sc){ handle_scancode(sc); }

void keyboard_init(void){
    khead=ktail=0; shift=0; caps=0;
    // enable keyboard IRQ1 via PIC
    uint8_t m = inb(0x21);
    outb(0x21, m & ~0x02); // unmask IRQ1
    // flush
    while(inb(0x64)&1) inb(0x60);
}

// called from ISR or poll
static void handle_scancode(uint8_t sc){
    if(sc==0x2A || sc==0x36){ shift=1; return; }
    if(sc==0xAA || sc==0xB6){ shift=0; return; }
    if(sc==0x3A){ caps^=1; return; }
    if(sc & 0x80) return; // release
    char c = shift ? sc_ascii_shift[sc] : sc_ascii[sc];
    if(caps && c>='a' && c<='z') c-=32;
    else if(caps && c>='A' && c<='Z') c+=32;
    if(c) kpush(c);
}

// poll PS/2 controller
static void keyboard_poll(void){
    while(inb(0x64) & 1){
        uint8_t sc = inb(0x60);
        handle_scancode(sc);
    }
}

int keyboard_has_char(void){
    keyboard_poll();
    return khead!=ktail;
}

int keyboard_try_getc(char *out){
    keyboard_poll();
    if(khead==ktail) return 0;
    *out = kbuf[ktail];
    ktail=(ktail+1)%KBD_BUF;
    return 1;
}

char keyboard_getc(void){
    char c;
    while(!keyboard_try_getc(&c)){
        asm volatile("pause");
        // allow typing via terminal (serial) OR display keyboard when in graphics
        if(inb(0x3F8+5)&1){ c=inb(0x3F8); return c; }
    }
    return c;
}
