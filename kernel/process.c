#include "process.h"
#include "pmm.h"
#include "heap.h"
#include "io.h"

extern void vga_puts(const char* s);
extern void serial_puts(const char* s);
extern void context_switch(uint64_t* old_rsp, uint64_t new_rsp);
extern void task_trampoline(void);

struct task tasks[MAX_TASKS];
struct task* task_list = 0;
struct task* current = 0;
static uint64_t next_pid = 1;

void process_init(void){
    for(int i=0;i<MAX_TASKS;i++) tasks[i].state = TASK_UNUSED;
    struct task* main = &tasks[0];
    main->pid = next_pid++;
    main->state = TASK_RUNNING;
    main->entry = 0;
    main->stack = 0;
    main->rsp = 0;
    int i; for(i=0;i<TASK_NAME_LEN-1 && "main"[i];i++) main->name[i]="main"[i];
    main->name[i]=0;
    current = main;
    task_list = main;
    main->next = 0;
    vga_puts("[PROC] main task pid 1\n");
    serial_puts("[PROC] main pid1\n");
}

struct task* task_create(void (*entry)(void), const char* name){
    for(int i=0;i<MAX_TASKS;i++) if(tasks[i].state==TASK_UNUSED){
        struct task* t=&tasks[i];
        t->pid = next_pid++;
        t->state = TASK_READY;
        t->entry = entry;
        t->stack = (uint64_t*)pmm_alloc_pages(STACK_PAGES);
        if(!t->stack){ t->state=TASK_UNUSED; return 0; }
        uint64_t* top = (uint64_t*)((uint8_t*)t->stack + STACK_PAGES*4096);
        top = (uint64_t*)((uint64_t)top & ~0xF);
        *--top = (uint64_t)entry;
        *--top = (uint64_t)task_trampoline;
        *--top = 0; *--top = 0; *--top = 0; *--top = 0; *--top = 0; *--top = 0;
        *--top = 0x202;
        t->rsp = (uint64_t)top;
        int j; for(j=0;j<TASK_NAME_LEN-1 && name[j];j++) t->name[j]=name[j];
        t->name[j]=0;
        t->next = 0;
        struct task* cur=task_list;
        while(cur->next) cur=cur->next;
        cur->next = t;
        vga_puts("[PROC] created "); vga_puts(name);
        vga_puts(" pid "); int n=t->pid,idx=0; char rev[16];
        if(n==0) rev[idx++]='0'; else while(n){rev[idx++]='0'+n%10; n/=10;}
        for(int k=idx-1;k>=0;k--){ char c[2]={rev[k],0}; vga_puts(c); }
        vga_puts("\n");
        serial_puts("[PROC] created "); serial_puts(name); serial_puts("\n");
        return t;
    }
    return 0;
}

struct task* current_task(void){ return current; }

void schedule(void){
    if(!current) return;
    struct task* prev = current;
    struct task* next = prev->next ? prev->next : task_list;
    struct task* start = next;
    do{
        if(next->state == TASK_READY) break;
        next = next->next ? next->next : task_list;
    } while(next != start);
    if(next->state != TASK_READY) return;
    if(next == prev) return;
    prev->state = (prev->state==TASK_RUNNING) ? TASK_READY : prev->state;
    next->state = TASK_RUNNING;
    struct task* old = current;
    current = next;
    context_switch(&old->rsp, next->rsp);
}

void task_yield(void){
    asm volatile("cli");
    schedule();
    asm volatile("sti");
}

void task_exit(void){
    vga_puts("[PROC] "); vga_puts(current->name); vga_puts(" exit\n");
    current->state = TASK_UNUSED;
    schedule();
    for(;;) asm volatile("hlt");
}
