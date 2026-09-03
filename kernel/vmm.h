#ifndef VMM_H
#define VMM_H
#include <stdint.h>
#include <stddef.h>

#define PTE_PRESENT  (1ULL<<0)
#define PTE_WRITABLE (1ULL<<1)
#define PTE_USER     (1ULL<<2)
#define PTE_HUGE     (1ULL<<7)
#define PTE_NX       (1ULL<<63)

void vmm_init(void);
int  vmm_map_page(uint64_t va, uint64_t pa, uint64_t flags);
int  vmm_unmap_page(uint64_t va);
uint64_t vmm_get_phys(uint64_t va);
void vmm_map_range(uint64_t va, uint64_t pa, size_t pages, uint64_t flags);
void vmm_test(void);

extern uint64_t vmm_kernel_pml4;
#endif
