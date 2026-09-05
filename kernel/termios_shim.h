#pragma once
// Minimal termios/ioctl shim for official GNU nano 9.2 on StrixOS
// Maps Linux termios to StrixOS keyboard/VGA
#include <stdint.h>
#define TCGETS 0x5401
#define TCSETS 0x5402
#define TIOCGWINSZ 0x5413
#define NCCS 32
typedef unsigned char cc_t;
typedef unsigned int tcflag_t;
typedef unsigned int speed_t;
struct termios { tcflag_t c_iflag,c_oflag,c_cflag,c_lflag; cc_t c_cc[NCCS]; speed_t c_ispeed,c_ospeed; };
struct winsize { unsigned short ws_row,ws_col,ws_xpixel,ws_ypixel; };
static inline int tcgetattr(int fd, struct termios *t){ if(t){ for(int i=0;i<NCCS;i++) t->c_cc[i]=0; t->c_lflag=0; } return 0; }
static inline int tcsetattr(int fd,int a, const struct termios *t){ return 0; }
static inline int ioctl(int fd, unsigned long req, void *arg){ if(req==TIOCGWINSZ && arg){ struct winsize *w=(struct winsize*)arg; w->ws_row=25; w->ws_col=80; w->ws_xpixel=640; w->ws_ypixel=400; return 0; } return 0; }
