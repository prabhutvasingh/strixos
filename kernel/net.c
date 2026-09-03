#include "net.h"
#include "heap.h"
extern void vga_puts(const char* s);
extern void serial_puts(const char* s);

#define LO_BUF 2048
static uint8_t lo_buf[LO_BUF];
static int lo_len=0;
static int lo_ready=0;

void net_init(void){
    vga_puts("[NET] loopback ready\n");
    serial_puts("[NET] lo up\n");
}
int net_loopback_send(const void* data, int len){
    if(len>LO_BUF) len=LO_BUF;
    for(int i=0;i<len;i++) lo_buf[i]=((uint8_t*)data)[i];
    lo_len=len; lo_ready=1;
    return len;
}
int net_loopback_recv(void* buf, int max){
    if(!lo_ready) return 0;
    int n = lo_len < max ? lo_len : max;
    for(int i=0;i<n;i++) ((uint8_t*)buf)[i]=lo_buf[i];
    lo_ready=0;
    return n;
}
void net_dump(void){
    vga_puts("[NET] lo "); vga_puts(lo_ready?"ready ":"idle "); vga_puts("\n");
}
