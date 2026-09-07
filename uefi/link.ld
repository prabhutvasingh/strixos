/* UEFI PE32+ linker script for BOOTX64.EFI */
ENTRY(efi_main)
SECTIONS
{
    . = 0x0;
    .text : { *(.text) *(.text.*) }
    .data : { *(.data) *(.data.*) *(.rodata) *(.rodata.*) }
    .reloc : { *(.reloc) }
    /DISCARD/ : { *(.eh_frame*) *(.comment) *(.note*) }
}
