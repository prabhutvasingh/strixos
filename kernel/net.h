#ifndef NET_H
#define NET_H
#include <stdint.h>
void net_init(void);
int  net_loopback_send(const void* data, int len);
int  net_loopback_recv(void* buf, int max);
void net_dump(void);
#endif
