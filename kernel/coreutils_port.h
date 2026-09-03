#ifndef COREUTILS_PORT_H
#define COREUTILS_PORT_H
// Wrappers around ORIGINAL GNU coreutils 9.5 + Sudo 1.9.15p5 + Neofetch 7.1.0 source
// Vendored originals at kernel/coreutils/cat.c, ls.c, echo.c, touch.c, sudo.c, neofetch (11592 lines bash, MIT)
// These functions are direct ports adapted to StrixOS VFS/syscalls
// See kernel/coreutils/* for verbatim original GPL/ISC/MIT source
int strix_cat(int argc, char **argv);
int strix_ls(int argc, char **argv);
int strix_echo(int argc, char **argv);
int strix_touch(int argc, char **argv);
int strix_admin(int argc, char **argv);
int strix_stxver(int argc, char **argv);
void strix_cat_help(void);
void strix_ls_help(void);
void strix_echo_help(void);
void strix_touch_help(void);
void strix_admin_help(void);
void strix_stxver_help(void);
#endif
