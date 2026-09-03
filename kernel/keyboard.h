#ifndef KEYBOARD_H
#define KEYBOARD_H
#include <stdint.h>
void keyboard_init(void);
int keyboard_has_char(void);
char keyboard_getc(void); // blocking poll for display input
int keyboard_try_getc(char *out); // non-blocking, 1 if got
#endif
