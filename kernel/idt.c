#include "idt.h"
#include "pic.h"
#include "io.h"
#include "timer.h"
#include "syscall.h"

extern void* isr_stub_table[];

static struct idt_entry idt[IDT_ENTRIES];
static struct idt_ptr idtp;

static const char* exc_msg[] = {
    "Division By Zero","Debug","NMI","Breakpoint","Overflow","Bound Range",
    "Invalid Opcode","Device Not Avail","Double Fault","Coprocessor Seg",
    "Invalid TSS","Seg Not Present","Stack Seg Fault","General Protection",
    "Page Fault","Reserved","x87 FPU","Alignment Check","Machine Check",
    "SIMD","Virtualization","Control Prot","Reserved","Reserved","Reserved",
    "Reserved","Reserved","Reserved","Hypervisor","VMM Comm","Security","Reserved"
};

void idt_set_gate(int n, uint64_t handler, uint16_t sel, uint8_t flags) {
    idt[n].offset_low  = handler & 0xFFFF;
    idt[n].selector    = sel;
    idt[n].ist         = 0;
    idt[n].type_attr   = flags;
    idt[n].offset_mid  = (handler >> 16) & 0xFFFF;
    idt[n].offset_high = (handler >> 32) & 0xFFFFFFFF;
    idt[n].zero        = 0;
}

void idt_init(void) {
    idtp.limit = sizeof(idt) - 1;
    idtp.base  = (uint64_t)&idt;

    for (int i = 0; i < IDT_ENTRIES; i++) {
        idt_set_gate(i, (uint64_t)isr_stub_table[i], 0x08, 0x8E);
    }

    // Remap PIC to 32-47 (0x20-0x2F)
    pic_remap(0x20, 0x28);
    // Mask all IRQs except we will unmask timer/keyboard later
    outb(0x21, 0xFF);
    outb(0xA1, 0xFF);

    asm volatile("lidt %0" :: "m"(idtp));
    isr_install();
}

// Forward decl for VGA/serial helpers (from main.c)
void vga_puts(const char* s);
void serial_puts(const char* s);
void vga_putchar(char c);

void isr_handler(struct regs* r) {
    if (r->int_no < 32) {
        serial_puts("[EXC] ");
        if (r->int_no < 32) {
            const char* m = exc_msg[r->int_no];
            serial_puts(m);
            vga_puts("[EXC] ");
            vga_puts(m);
            vga_puts(" err=");
        }
        // print err and rip
        char buf[32];
        // simple hex
        const char* hex="0123456789ABCDEF";
        buf[0]='0'; buf[1]='x';
        for(int i=0;i<16;i++) buf[2+i]= hex[(r->err_code >> (60-i*4)) & 0xF];
        buf[18]=0; serial_puts(buf); vga_puts(buf);
        vga_puts(" rip=");
        for(int i=0;i<16;i++) buf[2+i]= hex[(r->rip >> (60-i*4)) & 0xF];
        serial_puts(" rip="); serial_puts(buf);
        vga_puts(" RIP="); vga_puts(buf);
        vga_puts("\n"); serial_puts("\n");
        if (r->int_no == 14) {
            uint64_t cr2; asm volatile("mov %%cr2, %0" : "=r"(cr2));
            serial_puts(" cr2="); vga_puts(" cr2=");
            for(int i=0;i<16;i++) buf[2+i]= hex[(cr2 >> (60-i*4)) & 0xF];
            serial_puts(buf); vga_puts(buf);
            serial_puts("\n"); vga_puts("\n");
        }
        // Don't halt on breakpoint (int3)
        if (r->int_no == 3) return;
        serial_puts("System halted.\n");
        vga_puts("System halted - exception.\n");
        for(;;) asm volatile("cli; hlt");
    } else if (r->int_no == 0x80) {
        syscall_handler(r);
    } else if (r->int_no >= 32 && r->int_no < 48) {
        // IRQ
        uint8_t irq = r->int_no - 32;
        if (irq == 0) {
            timer_tick();
        } else if (irq == 1) {
            uint8_t sc = inb(0x60);
            // push to keyboard buffer (handled via polling too)
            extern void keyboard_isr_push(uint8_t);
            keyboard_isr_push(sc);
        }
        pic_send_eoi(irq);
    }
}

void isr_install(void) {
    asm volatile("sti");
}
