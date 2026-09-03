#include "elf.h"
extern void vga_puts(const char* s);

int elf_check(void* data){
    struct elf_hdr* h=data;
    return h->magic==ELF_MAGIC;
}
int elf_load(void* data, void (**entry)(void)){
    struct elf_hdr* h=data;
    if(!elf_check(data)) return -1;
    *entry=(void(*)(void))(uintptr_t)h->entry;
    vga_puts("[ELF] entry 0x");
    // simple hex
    char hex[]="0123456789ABCDEF";
    for(int i=60;i>=0;i-=4) vga_puts((char[]){hex[(h->entry>>i)&0xF],0});
    vga_puts("\n");
    return 0;
}
