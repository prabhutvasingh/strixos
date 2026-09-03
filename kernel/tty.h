#ifndef TTY_H
#define TTY_H
// TTY that runs on any display (720p/1080p/1024x768 via VBE or VGA text)
// Login: "User Login:   " -> shell

void tty_init(void);
void tty_login(int tty_id);
int tty_current(void);
void tty_switch(int id);
void tty_clear(void);

#endif
