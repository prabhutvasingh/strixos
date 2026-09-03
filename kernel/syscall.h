#ifndef SYSCALL_H
#define SYSCALL_H
#include <stdint.h>
#include <stddef.h>
#include "idt.h"

#define SYS_EXIT    0
#define SYS_WRITE   1
#define SYS_READ    2
#define SYS_OPEN    3
#define SYS_CLOSE   4
#define SYS_GETPID  5
#define SYS_YIELD   6
#define SYS_BRK     7
#define SYS_FORK    8

// Called from ISR handler when int_no == 0x80
void syscall_handler(struct regs* r);

// User wrappers (also usable from kernel tasks)
long sys_write(int fd, const char* buf, size_t len);
long sys_read(int fd, void* buf, size_t len);
long sys_open(const char* path, int flags);
long sys_close(int fd);
long sys_getpid(void);
long sys_yield(void);
long sys_exit(int code);
long sys_brk(void* addr);

#endif
