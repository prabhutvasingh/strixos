#ifndef SYSCALL_H
#define SYSCALL_H
#include <stdint.h>
#include <stddef.h>
#include "idt.h"

#define SYS_EXIT    5000
#define SYS_WRITE   5001
#define SYS_READ    5002
#define SYS_OPEN    5003
#define SYS_CLOSE   5004
#define SYS_GETPID  5005
#define SYS_YIELD   5006
#define SYS_BRK     5007
#define SYS_FORK    5008

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
