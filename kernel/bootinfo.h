#ifndef STRIX_BOOTINFO_H
#define STRIX_BOOTINFO_H
#include <stdint.h>

// Shared BIOS/UEFI boot info block at 0x6000, filled by bootloader.
// BIOS path never writes it -> magic invalid -> kernel uses BIOS defaults.
#define STRIX_BOOTINFO_ADDR  0x6000ULL
#define STRIX_BOOTINFO_MAGIC 0x5354524958424F4FULL // "STRIXBOO"

struct strix_bootinfo {
    uint64_t magic;       // STRIX_BOOTINFO_MAGIC if valid
    uint64_t boot_mode;   // 1 = BIOS, 2 = UEFI
    uint64_t fb_base;     // framebuffer phys addr (GOP or VBE)
    uint32_t fb_width;
    uint32_t fb_height;
    uint32_t fb_pitch;    // pixels per scanline
    uint32_t fb_bpp;      // bits per pixel
};

#endif
