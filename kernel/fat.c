#include "fat.h"
#include "heap.h"
extern void vga_puts(const char* s);
extern void serial_puts(const char* s);

static uint8_t* fat_img;
static size_t fat_sz;
static int fat_inited=0;

// Fake FAT12 for demo: single file "FAT.TXT" with content
static const char* fake_name = "FAT.TXT";
static const char* fake_data = "FAT12 demo file - StrixOS scalable FS\nSector 0: boot\n";
static int fake_fd = -1;
static size_t fake_off=0;

void fat_init(void* img, size_t sz){
    fat_img = img; fat_sz = sz; fat_inited=1;
    vga_puts("[FAT] in-mem 1 file FAT.TXT\n");
    serial_puts("[FAT] ready\n");
}
int fat_open(const char* path){
    if(!fat_inited) return -1;
    if(path[0]=='/') path++;
    // case-insensitive compare for demo
    const char* a=path; const char* b=fake_name;
    while(*a && *b){ char ca=*a, cb=*b; if(ca>='a'&&ca<='z') ca-=32; if(cb>='a'&&cb<='z') cb-=32; if(ca!=cb) break; a++; b++; }
    if(*a==0 && *b==0){ fake_off=0; fake_fd=3; return 3; }
    return -1;
}
long fat_read(int fd, void* buf, size_t len){
    if(fd!=3) return -1;
    size_t avail = 0; while(fake_data[avail]) avail++;
    if(fake_off>=avail) return 0;
    size_t to=len; if(fake_off+to>avail) to=avail-fake_off;
    for(size_t i=0;i<to;i++) ((uint8_t*)buf)[i]=fake_data[fake_off+i];
    fake_off+=to;
    return to;
}
int fat_close(int fd){ if(fd==3){ fake_off=0; return 0; } return -1; }
void fat_ls(void){
    vga_puts("  FAT.TXT\n");
    serial_puts("FAT.TXT\n");
}
