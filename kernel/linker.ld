/* =============================================================================
 * Kernel Linker Script for x86_64
 * ============================================================================= */

ENTRY(_start)

KERNEL_OFFSET = 0x100000;

SECTIONS
{
    . = KERNEL_OFFSET;

    .text : AT(KERNEL_OFFSET) {
        *(.multiboot)
        *(.text)
    }

    .rodata : {
        *(.rodata)
    }

    .data : {
        *(.data)
    }

    .bss : {
        __bss_start = .;
        *(.bss)
        *(COMMON)
        __bss_end = .;
    }

    /DISCARD/ : {
        *(.comment)
        *(.note*)
        *(.eh_frame*)
    }
}
