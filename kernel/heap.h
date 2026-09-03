#ifndef HEAP_H
#define HEAP_H
#include <stddef.h>
#include <stdint.h>

void  heap_init(void);
void* kmalloc(size_t size);
void  kfree(void* ptr);
void* krealloc(void* ptr, size_t size);
void* kcalloc(size_t n, size_t size);
void  heap_dump(void);
size_t heap_used(void);
size_t heap_free(void);
#endif
