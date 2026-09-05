#ifndef SYSCALL_H
#define SYSCALL_H
#include <stdint.h>
#include <stddef.h>
#include "idt.h"

#define SYS_EXIT    100
#define SYS_WRITE   101
#define SYS_READ    102
#define SYS_OPEN    103
#define SYS_CLOSE   104
#define SYS_GETPID  105
#define SYS_YIELD   106
#define SYS_BRK     107
#define SYS_FORK    108

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
