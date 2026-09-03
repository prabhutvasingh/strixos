#include "vmm.h"
#include "pmm.h"
#include "io.h"

extern void vga_puts(const char* s);
extern void serial_puts(const char* s);

uint64_t vmm_kernel_pml4;

static inline uint64_t* pml4_ptr(void){ return (uint64_t*)(vmm_kernel_pml4 & ~0xFFFULL); }

void vmm_init(void){
    uint64_t cr3; asm volatile("mov %%cr3, %0": "=r"(cr3));
    vmm_kernel_pml4 = cr3;
    serial_puts("[VMM] PML4 at ");
    vga_puts("[VMM] PML4 at 0x");
    char hex[]="0123456789ABCDEF";
    char buf[20]; buf[0]='0'; buf[1]='x';
    for(int i=0;i<16;i++) buf[2+i]=hex[(cr3>> (60-i*4)) &0xF];
    buf[18]=0; vga_puts(buf+2); serial_puts(buf); vga_puts("\n"); serial_puts("\n");
}

int vmm_map_page(uint64_t va, uint64_t pa, uint64_t flags){
    uint64_t pml4_idx = (va >> 39) & 0x1FF;
    uint64_t pdpt_idx = (va >> 30) & 0x1FF;
    uint64_t pd_idx   = (va >> 21) & 0x1FF;
    uint64_t pt_idx   = (va >> 12) & 0x1FF;

    uint64_t* pml4 = (uint64_t*)(vmm_kernel_pml4 & ~0xFFFULL);
    if(!(pml4[pml4_idx] & PTE_PRESENT)){
        uint64_t* pdpt = (uint64_t*)pmm_alloc_page();
        if(!pdpt) return -1;
        pml4[pml4_idx] = ((uint64_t)pdpt) | PTE_PRESENT | PTE_WRITABLE;
    }
    uint64_t* pdpt = (uint64_t*)(pml4[pml4_idx] & ~0xFFFULL);
    // If PDPT entry is huge (1GB) we can't map 4K inside, but our setup uses 2MB
    if(pdpt[pdpt_idx] & PTE_HUGE) return -1;
    if(!(pdpt[pdpt_idx] & PTE_PRESENT)){
        uint64_t* pd = (uint64_t*)pmm_alloc_page();
        if(!pd) return -1;
        pdpt[pdpt_idx] = ((uint64_t)pd) | PTE_PRESENT | PTE_WRITABLE;
    }
    uint64_t* pd = (uint64_t*)(pdpt[pdpt_idx] & ~0xFFFULL);
    if(pd[pd_idx] & PTE_HUGE){
        // 2MB page already mapped, need to split if 4K requested
        return -1;
    }
    if(!(pd[pd_idx] & PTE_PRESENT)){
        uint64_t* pt = (uint64_t*)pmm_alloc_page();
        if(!pt) return -1;
        pd[pd_idx] = ((uint64_t)pt) | PTE_PRESENT | PTE_WRITABLE;
    }
    uint64_t* pt = (uint64_t*)(pd[pd_idx] & ~0xFFFULL);
    if(pt[pt_idx] & PTE_PRESENT) return -1; // already mapped
    pt[pt_idx] = (pa & ~0xFFFULL) | (flags & 0xFFF) | PTE_PRESENT;
    if(flags & PTE_NX) pt[pt_idx] |= PTE_NX;
    asm volatile("invlpg (%0)" :: "r"(va) : "memory");
    return 0;
}

int vmm_unmap_page(uint64_t va){
    uint64_t pml4_idx = (va >> 39) & 0x1FF;
    uint64_t pdpt_idx = (va >> 30) & 0x1FF;
    uint64_t pd_idx   = (va >> 21) & 0x1FF;
    uint64_t pt_idx   = (va >> 12) & 0x1FF;
    uint64_t* pml4 = (uint64_t*)(vmm_kernel_pml4 & ~0xFFFULL);
    if(!(pml4[pml4_idx] & PTE_PRESENT)) return -1;
    uint64_t* pdpt = (uint64_t*)(pml4[pml4_idx] & ~0xFFFULL);
    if(!(pdpt[pdpt_idx] & PTE_PRESENT)) return -1;
    uint64_t* pd = (uint64_t*)(pdpt[pdpt_idx] & ~0xFFFULL);
    if(!(pd[pd_idx] & PTE_PRESENT)) return -1;
    uint64_t* pt = (uint64_t*)(pd[pd_idx] & ~0xFFFULL);
    pt[pt_idx]=0;
    asm volatile("invlpg (%0)" :: "r"(va) : "memory");
    return 0;
}

uint64_t vmm_get_phys(uint64_t va){
    uint64_t pml4_idx = (va >> 39) & 0x1FF;
    uint64_t pdpt_idx = (va >> 30) & 0x1FF;
    uint64_t pd_idx   = (va >> 21) & 0x1FF;
    uint64_t pt_idx   = (va >> 12) & 0x1FF;
    uint64_t* pml4 = (uint64_t*)(vmm_kernel_pml4 & ~0xFFFULL);
    if(!(pml4[pml4_idx] & PTE_PRESENT)) return 0;
    uint64_t* pdpt = (uint64_t*)(pml4[pml4_idx] & ~0xFFFULL);
    if(!(pdpt[pdpt_idx] & PTE_PRESENT)) return 0;
    if(pdpt[pdpt_idx] & PTE_HUGE) return (pdpt[pdpt_idx] & ~0x3FFFFFFFULL) + (va & 0x3FFFFFFF);
    uint64_t* pd = (uint64_t*)(pdpt[pdpt_idx] & ~0xFFFULL);
    if(!(pd[pd_idx] & PTE_PRESENT)) return 0;
    if(pd[pd_idx] & PTE_HUGE) return (pd[pd_idx] & ~0x1FFFFFULL) + (va & 0x1FFFFF);
    uint64_t* pt = (uint64_t*)(pd[pd_idx] & ~0xFFFULL);
    if(!(pt[pt_idx] & PTE_PRESENT)) return 0;
    return (pt[pt_idx] & ~0xFFFULL) + (va & 0xFFF);
}

void vmm_map_range(uint64_t va, uint64_t pa, size_t pages, uint64_t flags){
    for(size_t i=0;i<pages;i++) vmm_map_page(va+i*4096, pa+i*4096, flags);
}

void vmm_test(void){
    // Map a test page at 0x8000000 (128MB, outside 0-64MB identity) to test 4K mapping
    void* phys = pmm_alloc_page();
    if(!phys){ vga_puts("[VMM] alloc fail\n"); return; }
    uint64_t va = 0x8000000;
    if(vmm_map_page(va, (uint64_t)phys, PTE_WRITABLE)!=0){
        vga_puts("[VMM] map fail\n"); serial_puts("[VMM] map fail\n");
        return;
    }
    volatile uint64_t* p = (uint64_t*)va;
    *p = 0xDEADBEEFCAFEBABEULL;
    if(*p == 0xDEADBEEFCAFEBABEULL){
        vga_puts("[VMM] test map 0x8000000 -> OK (0xDEADBEEF)\n");
        serial_puts("[VMM] test OK\n");
    } else {
        vga_puts("[VMM] test FAIL\n");
    }
    vmm_unmap_page(va);
    pmm_free_page(phys);

    // Test higher-half mapping already exists at 0xFFFFFFFF80000000 (mapped to 0)
    // Our bootloader mapped PD[510] -> 2MB at 0, so high half should be accessible
    // But we haven't set up higher-half stack, just test read
    // Skip if not needed
}
