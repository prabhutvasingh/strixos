#pragma once
// Tiny ncurses shim for official GNU nano -> StrixOS VGA 0xB8000 + keyboard 0x60/0x3F8
#include <stdint.h>
#include "io.h"
#define OK 0
#define ERR -1
#define A_REVERSE 0x70
typedef struct {} WINDOW;
static WINDOW _stdscr; static inline WINDOW* initscr(void){ volatile uint16_t *v=(volatile uint16_t*)0xB8000; for(int i=0;i<80*25;i++) v[i]=(0x07<<8)|' '; return &_stdscr; }
static inline int endwin(void){ return OK; }
static inline int refresh(void){ return OK; }
static inline int clear(void){ volatile uint16_t *v=(volatile uint16_t*)0xB8000; for(int i=0;i<80*25;i++) v[i]=(0x07<<8)|' '; return OK; }
static inline int move(int y,int x){ int pos=y*80+x; outb(0x3D4,0x0F); outb(0x3D5,pos&0xFF); outb(0x3D4,0x0E); outb(0x3D5,(pos>>8)&0xFF); return OK; }
static inline int addch(int c){ outb(0x3F8,c); return OK; }
static inline int addstr(const char *s){ while(*s) outb(0x3F8,*s++); return OK; }
static inline int mvaddstr(int y,int x,const char *s){ move(y,x); return addstr(s); }
static inline int getch(void){ extern char keyboard_getc(void); return keyboard_getc(); }
static inline int cbreak(void){ return OK; }
static inline int nocbreak(void){ return OK; }
static inline int echo(void){ return OK; }
static inline int noecho(void){ return OK; }
static inline int keypad(WINDOW*w,int b){ return OK; }
static inline int raw(void){ return OK; }
static inline int noraw(void){ return OK; }
