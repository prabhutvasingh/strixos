#include "elf.h"
#include "io.h"
#include "pmm.h"
#include "vmm.h"
#include "heap.h"
extern void vga_puts(const char* s);
extern void serial_puts(const char* s);

int elf_check(void* data){
    struct elf_hdr* h=data;
    return h->magic==ELF_MAGIC;
}
// ATA PIO read LBA28
static void ata_read_lba(uint32_t lba, uint8_t sectors, void *buf){
    // wait BSY clear
    while(inb(0x1F7) & 0x80);
    outb(0x1F6, 0xE0 | ((lba>>24)&0x0F));
    outb(0x1F1, 0x00);
    outb(0x1F2, sectors);
    outb(0x1F3, lba & 0xFF);
    outb(0x1F4, (lba>>8)&0xFF);
    outb(0x1F5, (lba>>16)&0xFF);
    outb(0x1F7, 0x20);
    uint16_t *p=(uint16_t*)buf;
    for(int i=0;i<sectors;i++){
        while(inb(0x1F7) & 0x80);
        while(!(inb(0x1F7) & 0x08));
        for(int j=0;j<256;j++) p[i*256+j]=inw(0x1F0);
    }
}
static inline void *load_nano_lba(uint32_t lba, size_t bytes){
    size_t sectors=(bytes+511)/512;
    void *buf=kmalloc(sectors*512+0x1000);
    if(!buf) return 0;
    // read in chunks of 255 (max)
    uint8_t *ptr=buf;
    uint32_t cur=lba;
    size_t left=sectors;
    while(left){
        uint8_t n= left>255?255:left;
        ata_read_lba(cur,n,ptr);
        ptr+=n*512;
        cur+=n;
        left-=n;
    }
    return buf;
}
int elf_load(void* data, void (**entry)(void)){
    struct elf_hdr* h=data;
    if(!elf_check(data)) return -1;
    struct elf_phdr *ph=(struct elf_phdr*)((uint8_t*)data + h->phoff);
    for(int i=0;i<h->phnum;i++){
        if(ph[i].type==1){ // PT_LOAD
            uint64_t vaddr=ph[i].vaddr;
            uint64_t filesz=ph[i].filesz;
            uint64_t memsz=ph[i].memsz;
            uint64_t off=ph[i].off;
            // For static PIE, vaddr is offset, we load at data + off -> vaddr relative to 0
            // Simplest: if vaddr < 0x100000, allocate at vaddr else use kmalloc and copy then jump via entry offset
            // For musl static, vaddr ~0x400000, we need to map
            // Use identity: copy to vaddr if vaddr > 0x100000 and pmm available, else to heap and adjust entry
            // For now, copy to vaddr directly (identity mapped 0-64M)
            if(vaddr < 0x40000000){
                // ensure pages present (0-64M already mapped)
                uint8_t *dst=(uint8_t*)(uintptr_t)vaddr;
                uint8_t *src=(uint8_t*)data + off;
                for(uint64_t k=0;k<filesz;k++) dst[k]=src[k];
                for(uint64_t k=filesz;k<memsz;k++) dst[k]=0;
            }
        }
    }
    *entry=(void(*)(void))(uintptr_t)h->entry;
    vga_puts("[ELF] nano static loaded entry 0x");
    char hex[]="0123456789ABCDEF";
    for(int i=60;i>=0;i-=4){ char c[2]={hex[(h->entry>>i)&0xF],0}; vga_puts(c); }
    vga_puts("\n");
    return 0;
}
void* elf_load_nano_lba(uint32_t lba){
    // read enough to get header first
    void *tmp=load_nano_lba(lba, 4096);
    if(!tmp) return 0;
    struct elf_hdr *h=tmp;
    // find max filesz+off
    struct elf_phdr *ph=(struct elf_phdr*)((uint8_t*)tmp + h->phoff);
    size_t max_end=0;
    for(int i=0;i<h->phnum;i++) if(ph[i].type==1){
        size_t end=ph[i].off+ph[i].filesz;
        if(end>max_end) max_end=end;
    }
    kfree(tmp);
    void *full=load_nano_lba(lba, max_end);
    return full;
}
