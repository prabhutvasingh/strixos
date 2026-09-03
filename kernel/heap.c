#include "heap.h"
#include "pmm.h"

extern void vga_puts(const char* s);
extern void serial_puts(const char* s);

#define HEAP_PAGES 256
#define HEAP_SIZE  (HEAP_PAGES * 4096)
#define HEAP_MAGIC 0xDEADBEEF

struct block {
    size_t size;
    int free;
    uint32_t magic;
    struct block* next;
};

static uint8_t* heap_base;
static struct block* head;

void heap_init(void){
    // Allocate heap backing from PMM (identity mapped 0-64MB)
    heap_base = (uint8_t*)pmm_alloc_pages(HEAP_PAGES);
    if(!heap_base){
        vga_puts("[HEAP] alloc fail\n");
        serial_puts("[HEAP] alloc fail\n");
        return;
    }
    head = (struct block*)heap_base;
    head->size = HEAP_SIZE - sizeof(struct block);
    head->free = 1;
    head->magic = HEAP_MAGIC;
    head->next = 0;
    vga_puts("[HEAP] 1MB heap ready\n");
    serial_puts("[HEAP] 1MB ready\n");
}

static struct block* find_free(size_t size){
    struct block* cur=head;
    while(cur){
        if(cur->free && cur->size >= size) return cur;
        cur=cur->next;
    }
    return 0;
}

void* kmalloc(size_t size){
    if(size==0) return 0;
    size = (size + 15) & ~15;
    struct block* b = find_free(size);
    if(!b) return 0;
    if(b->size >= size + sizeof(struct block) + 16){
        struct block* n = (struct block*)((uint8_t*)b + sizeof(struct block) + size);
        n->size = b->size - size - sizeof(struct block);
        n->free = 1;
        n->magic = HEAP_MAGIC;
        n->next = b->next;
        b->size = size;
        b->next = n;
    }
    b->free = 0;
    return (uint8_t*)b + sizeof(struct block);
}

void kfree(void* ptr){
    if(!ptr) return;
    struct block* b = (struct block*)((uint8_t*)ptr - sizeof(struct block));
    if(b->magic != HEAP_MAGIC) return;
    b->free = 1;
    if(b->next && b->next->free){
        b->size += sizeof(struct block) + b->next->size;
        b->next = b->next->next;
    }
    struct block* cur=head;
    while(cur && cur->next != b) cur=cur->next;
    if(cur && cur->free){
        cur->size += sizeof(struct block) + b->size;
        cur->next = b->next;
    }
}

void* kcalloc(size_t n, size_t size){
    size_t tot = n*size;
    void* p = kmalloc(tot);
    if(p) for(size_t i=0;i<tot;i++) ((uint8_t*)p)[i]=0;
    return p;
}
void* krealloc(void* ptr, size_t size){
    if(!ptr) return kmalloc(size);
    if(size==0){ kfree(ptr); return 0; }
    struct block* b = (struct block*)((uint8_t*)ptr - sizeof(struct block));
    size_t old = b->size;
    if(old >= size) return ptr;
    void* np = kmalloc(size);
    if(!np) return 0;
    for(size_t i=0;i<old;i++) ((uint8_t*)np)[i]=((uint8_t*)ptr)[i];
    kfree(ptr);
    return np;
}

size_t heap_used(void){
    size_t used=0; struct block* cur=head;
    while(cur){ if(!cur->free) used+=cur->size; cur=cur->next; }
    return used;
}
size_t heap_free(void){
    size_t f=0; struct block* cur=head;
    while(cur){ if(cur->free) f+=cur->size; cur=cur->next; }
    return f;
}
void heap_dump(void){
    vga_puts("[HEAP] used="); serial_puts("[HEAP] used=");
    size_t u=heap_used();
    char rev[16]; int idx=0; if(u==0) rev[idx++]='0'; else while(u){rev[idx++]='0'+u%10; u/=10;}
    for(int i=idx-1;i>=0;i--){ char c[2]={rev[i],0}; vga_puts(c); serial_puts(c); }
    vga_puts(" free="); serial_puts(" free=");
    size_t f=heap_free(); idx=0; if(f==0) rev[idx++]='0'; else while(f){rev[idx++]='0'+f%10; f/=10;}
    for(int i=idx-1;i>=0;i--){ char c[2]={rev[i],0}; vga_puts(c); serial_puts(c); }
    vga_puts("\n"); serial_puts("\n");
}
