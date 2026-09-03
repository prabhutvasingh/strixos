#include "timer.h"
#include "io.h"
#include "pic.h"
#include "process.h"

#define PIT_CMD  0x43
#define PIT_CH0  0x40
#define PIT_FREQ 1193180

static volatile uint64_t ticks = 0;

void timer_init(uint32_t hz){
    uint32_t div = PIT_FREQ / hz;
    outb(PIT_CMD, 0x36);
    outb(PIT_CH0, div & 0xFF);
    outb(PIT_CH0, (div >> 8) & 0xFF);
    pic_clear_mask(0); // unmask timer IRQ0
}

uint64_t timer_ticks(void){ return ticks; }
void timer_wait(uint64_t t){ uint64_t cur=ticks; while(ticks - cur < t) asm volatile("hlt"); }

void timer_tick(void){
    ticks++;
    // preemptive schedule every 10 ticks (100ms at 100Hz)
    if(ticks % 10 == 0){
        schedule();
    }
}
