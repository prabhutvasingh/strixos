#ifndef PROCESS_H
#define PROCESS_H
#include <stdint.h>
#include <stddef.h>

#define MAX_TASKS 16
#define STACK_PAGES 4   // 16KB stack
#define TASK_NAME_LEN 16

enum task_state { TASK_UNUSED, TASK_READY, TASK_RUNNING, TASK_BLOCKED };

struct task {
    uint64_t pid;
    enum task_state state;
    uint64_t rsp;          // saved stack pointer
    uint64_t* stack;       // base of stack allocation
    void (*entry)(void);
    char name[TASK_NAME_LEN];
    struct task* next;
};

void process_init(void);
struct task* task_create(void (*entry)(void), const char* name);
void task_yield(void);
void task_exit(void);
struct task* current_task(void);
void schedule(void);
void timer_tick(void);

extern struct task* task_list;
extern struct task* current;

#endif
