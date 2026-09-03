#include "pmm.h"
#include "io.h"

extern void vga_puts(const char* s);
extern void serial_puts(const char* s);

// --- Hardcoded memory layout for QEMU -m 256 ---
// We manage 2MB .. 64MB (62MB = 15872 pages) for demo.
// Kernel at 0x100000 (9152B), page tables at 0x1000, stack at 0x90000.
// Bitmap placed at 0x150000 (8KB for 64MB).
#define MEM_START   0x200000ULL   // 2MB
#define MEM_END     0x4000000ULL  // 64MB
#define BITMAP_ADDR 0x150000ULL

static uint8_t* bitmap;
static size_t total_pages;
static size_t free_pages;
static size_t used_pages;

static inline void bmp_set(size_t i)   { bitmap[i/8] |=  (1 << (i%8)); }
static inline void bmp_clear(size_t i) { bitmap[i/8] &= ~(1 << (i%8)); }
static inline int  bmp_test(size_t i)  { return bitmap[i/8] & (1 << (i%8)); }

static inline uint64_t page_to_addr(size_t i) { return MEM_START + i * PAGE_SIZE; }
static inline size_t addr_to_page(uint64_t a) { return (a - MEM_START) / PAGE_SIZE; }

void pmm_init(void) {
    total_pages = (MEM_END - MEM_START) / PAGE_SIZE;
    bitmap = (uint8_t*)BITMAP_ADDR;
    // Clear bitmap
    for (size_t i = 0; i < (total_pages+7)/8; i++) bitmap[i]=0;
    free_pages = total_pages;
    used_pages = 0;

    // Mark bitmap's own page as used (0x150000)
    size_t bmp_page = addr_to_page(BITMAP_ADDR);
    if (bmp_page < total_pages) {
        bmp_set(bmp_page);
        free_pages--; used_pages++;
    }

    serial_puts("[PMM] bitmap at 0x150000, ");
    vga_puts("[PMM] bitmap at 0x150000, total=");
    // simple dec
    size_t n=total_pages;
    int idx=0; char rev[16];
    if(n==0) rev[idx++]='0'; else while(n){rev[idx++]='0'+n%10; n/=10;}
    for(int i=idx-1;i>=0;i--){ char c[2]={rev[i],0}; vga_puts(c); serial_puts(c); }
    vga_puts(" pages ("); serial_puts(" pages (");
    n=(MEM_END-MEM_START)/1024/1024;
    idx=0; if(n==0) rev[idx++]='0'; else while(n){rev[idx++]='0'+n%10; n/=10;}
    for(int i=idx-1;i>=0;i--){ char c[2]={rev[i],0}; vga_puts(c); serial_puts(c); }
    vga_puts(" MB)\n"); serial_puts(" MB)\n");
}

void* pmm_alloc_page(void) {
    for (size_t i=0;i<total_pages;i++) if(!bmp_test(i)) {
        bmp_set(i); free_pages--; used_pages++;
        uint64_t addr = page_to_addr(i);
        // zero
        for(size_t k=0;k<PAGE_SIZE/8;k++) ((uint64_t*)addr)[k]=0;
        return (void*)addr;
    }
    return 0;
}
void pmm_free_page(void* p) {
    uint64_t a=(uint64_t)p;
    if(a < MEM_START || a >= MEM_END) return;
    if(a % PAGE_SIZE) return;
    size_t i=addr_to_page(a);
    if(!bmp_test(i)) return;
    bmp_clear(i); free_pages++; used_pages--;
}
void* pmm_alloc_pages(size_t n) {
    if(n==0) return 0;
    if(n==1) return pmm_alloc_page();
    for(size_t i=0;i+ n <= total_pages; ) {
        int ok=1;
        for(size_t j=0;j<n;j++) if(bmp_test(i+j)){ ok=0; i+=j+1; break; }
        if(ok){ for(size_t j=0;j<n;j++) bmp_set(i+j); free_pages-=n; used_pages+=n;
                uint64_t addr=page_to_addr(i);
                for(size_t k=0;k<n*PAGE_SIZE/8;k++) ((uint64_t*)addr)[k]=0;
                return (void*)addr; }
    }
    return 0;
}
void pmm_free_pages(void* p, size_t n){ for(size_t i=0;i<n;i++) pmm_free_page((uint8_t*)p+i*PAGE_SIZE); }
size_t pmm_total_pages(void){ return total_pages; }
size_t pmm_free_pages_count(void){ return free_pages; }
size_t pmm_used_pages_count(void){ return used_pages; }

void pmm_dump(void){
    vga_puts("[PMM] free="); serial_puts("[PMM] free=");
    size_t n=free_pages; char rev[16]; int idx=0;
    if(n==0) rev[idx++]='0'; else while(n){rev[idx++]='0'+n%10; n/=10;}
    for(int i=idx-1;i>=0;i--){ char c[2]={rev[i],0}; vga_puts(c); serial_puts(c); }
    vga_puts(" used="); serial_puts(" used=");
    n=used_pages; idx=0; if(n==0) rev[idx++]='0'; else while(n){rev[idx++]='0'+n%10; n/=10;}
    for(int i=idx-1;i>=0;i--){ char c[2]={rev[i],0}; vga_puts(c); serial_puts(c); }
    vga_puts("\n"); serial_puts("\n");
}
