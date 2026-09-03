#ifndef PMM_H
#define PMM_H
#include <stdint.h>
#include <stddef.h>

#define PAGE_SIZE 4096
#define PAGE_ALIGN_UP(x)   (((x) + PAGE_SIZE-1) & ~(PAGE_SIZE-1))
#define PAGE_ALIGN_DOWN(x) ((x) & ~(PAGE_SIZE-1))

void pmm_init(void);
void* pmm_alloc_page(void);
void  pmm_free_page(void* p);
void* pmm_alloc_pages(size_t n);
void  pmm_free_pages(void* p, size_t n);
size_t pmm_total_pages(void);
size_t pmm_free_pages_count(void);
size_t pmm_used_pages_count(void);
void  pmm_dump(void);
#endif
