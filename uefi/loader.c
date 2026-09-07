// StrixOS UEFI loader - BOOTX64.EFI, freestanding, no gnu-efi.
// Compile with -mabi=ms. Loads kernel.bin from ESP to 0x100000,
// identity-maps 0-4GB + GOP framebuffer, installs own GDT + page tables
// (mirrors BIOS stage2_32 layout), passes bootinfo at 0x6000 via RDI.
#include "efi.h"
#include "../kernel/bootinfo.h"

static EFI_SYSTEM_TABLE *ST;
static EFI_BOOT_SERVICES *BS;

// Serial trace (visible with -serial stdio even in nographic mode)
static void outb(UINT16 p, UINT8 v){ __asm__ volatile("outb %0,%1" :: "a"(v), "Nd"(p)); }
static UINT8 inb(UINT16 p){ UINT8 v; __asm__ volatile("inb %1,%0" : "=a"(v) : "Nd"(p)); return v; }
static void serial_init(void){
    outb(0x3F8+1, 0x00); outb(0x3F8+3, 0x80);
    outb(0x3F8+0, 0x03); outb(0x3F8+1, 0x00);
    outb(0x3F8+3, 0x03); outb(0x3F8+2, 0xC7); outb(0x3F8+4, 0x0B);
}
static void trace(const char *s){
    while(*s){ while(!(inb(0x3F8+5) & 0x20)); outb(0x3F8, *s++); }
}

static void print(const char *s){
    CHAR16 buf[128]; int i = 0;
    while(s[i] && i < 127){ buf[i] = (CHAR16)s[i]; i++; }
    buf[i] = 0;
    ST->ConOut->OutputString(ST->ConOut, buf);
}

static int guid_eq(const EFI_GUID *a, const EFI_GUID *b){
    const UINT8 *x = (const UINT8*)a, *y = (const UINT8*)b;
    for(int i = 0; i < 16; i++) if(x[i] != y[i]) return 0;
    return 1;
}
static const EFI_GUID GopGuid = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;
static const EFI_GUID ImgGuid = EFI_LOADED_IMAGE_PROTOCOL_GUID;
static const EFI_GUID FsGuid  = EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID;

// widen: build CHAR16 path from ascii
static void to16(const char *s, CHAR16 *d, int n){
    int i = 0; while(s[i] && i < n-1){ d[i] = (CHAR16)s[i]; i++; }
    d[i] = 0;
}

EFI_STATUS efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable){
    ST = SystemTable;
    BS = SystemTable->BootServices;
    (void)ImageHandle;

    serial_init();
    trace("[UEFI] StrixOS loader start\n");
    ST->ConOut->ClearScreen(ST->ConOut);
    print("StrixOS UEFI loader - by Avi (12)\r\n");

    // Watchdog off (avoid 5-min reset)
    BS->SetWatchdogTimer(0, 0, 0, 0);

    // --- GOP: pick 1024x768 or best effort ---
    trace("[UEFI] locating GOP...\n");
    EFI_GRAPHICS_OUTPUT_PROTOCOL *gop = 0;
    EFI_GUID g = GopGuid;
    EFI_STATUS gs = BS->LocateProtocol(&g, 0, (VOID**)&gop);
    if(EFI_ERROR(gs) || !gop){
        trace("[UEFI] FATAL: no GOP, status=0x");
        { char h[] = "0123456789ABCDEF"; for(int i = 15; i >= 0; i--){ char c[2] = { h[(gs >> (i*4)) & 0xF], 0 }; trace(c); } trace("\n"); }
        print("No GOP found, halt\r\n");
        for(;;) __asm__ volatile("hlt");
    }
    trace("[UEFI] GOP found, picking mode...\n");
    UINT32 best = gop->Mode->Mode, bw = 0, bh = 0;
    for(UINT32 i = 0; i < gop->Mode->MaxMode; i++){
        UINTN sz = 0; EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *mi = 0;
        if(EFI_ERROR(gop->QueryMode(gop, i, &sz, &mi)) || !mi) continue;
        if(mi->PixelFormat != PixelBlueGreenRedReserved8BitPerColor &&
           mi->PixelFormat != PixelRedGreenBlueReserved8BitPerColor) continue;
        if(mi->HorizontalResolution == 1024 && mi->VerticalResolution == 768){
            best = i; bw = 1024; bh = 768; break;
        }
        if(mi->HorizontalResolution > bw){ best = i; bw = mi->HorizontalResolution; bh = mi->VerticalResolution; }
    }
    trace("[UEFI] setting GOP mode...\n");
    gop->SetMode(gop, best);
    trace("[UEFI] GOP mode set\n");
    UINT64 fb_base = gop->Mode->FrameBufferBase;
    UINTN  fb_size = gop->Mode->FrameBufferSize;
    UINT32 fb_w = gop->Mode->Info->HorizontalResolution;
    UINT32 fb_h = gop->Mode->Info->VerticalResolution;
    UINT32 fb_p = gop->Mode->Info->PixelsPerScanLine;
    print("GOP framebuffer ready\r\n");

    // --- Open kernel.bin from same ESP volume ---
    trace("[UEFI] opening ESP volume...\n");
    EFI_LOADED_IMAGE_PROTOCOL *img = 0;
    EFI_GUID ig = ImgGuid;
    BS->HandleProtocol(ImageHandle, &ig, (VOID**)&img);
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *fs = 0;
    EFI_GUID fg = FsGuid;
    BS->HandleProtocol(img->DeviceHandle, &fg, (VOID**)&fs);
    EFI_FILE_PROTOCOL *root = 0, *kf = 0;
    fs->OpenVolume(fs, &root);
    trace("[UEFI] opening kernel.bin...\n");
    CHAR16 kpath[32];
    to16("kernel.bin", kpath, 32);
    if(EFI_ERROR(root->Open(root, &kf, kpath, EFI_FILE_MODE_READ, 0)) || !kf){
        trace("[UEFI] FATAL: kernel.bin not found\n");
        print("kernel.bin not found on ESP\r\n");
        for(;;) __asm__ volatile("hlt");
    }
    // file size
    UINT64 fsize = 0;
    { UINTN eof = 0xFFFFFFFFFFFFFFFFULL; kf->SetPosition(kf, eof); kf->GetPosition(kf, &fsize); kf->SetPosition(kf, 0); }
    UINTN pages = (UINTN)((fsize + 0xFFF) / 0x1000) + 1;
    UINT64 kaddr = 0x100000ULL;
    UINT64 staging = 0; // fallback buffer if 0x100000 is occupied
    trace("[UEFI] loading kernel.bin...\n");
    if(EFI_ERROR(BS->AllocatePages(AllocateAddress, EfiLoaderData, pages, &kaddr)) || kaddr != 0x100000ULL){
        trace("[UEFI] 0x100000 busy, using staging buffer\n");
        print("0x100000 busy, staging...\r\n");
        staging = 0;
        if(EFI_ERROR(BS->AllocatePages(AllocateAnyPages, EfiLoaderData, pages, &staging))){
            trace("[UEFI] FATAL: no memory for kernel\n");
            print("No memory for kernel\r\n");
            for(;;) __asm__ volatile("hlt");
        }
        kaddr = staging;
    }
    UINTN toread = (UINTN)fsize;
    if(EFI_ERROR(kf->Read(kf, &toread, (VOID*)(UINTN)kaddr))){
        trace("[UEFI] FATAL: kernel read failed\n");
        print("kernel read failed\r\n");
        for(;;) __asm__ volatile("hlt");
    }
    kf->Close(kf);
    trace("[UEFI] kernel.bin loaded\n");
    print("kernel.bin loaded at 0x100000\r\n");

    // --- bootinfo at 0x6000 ---
    struct strix_bootinfo *bi = (struct strix_bootinfo*)STRIX_BOOTINFO_ADDR;
    bi->magic = STRIX_BOOTINFO_MAGIC;
    bi->boot_mode = 2;
    bi->fb_base = fb_base;
    bi->fb_width = fb_w;
    bi->fb_height = fb_h;
    bi->fb_pitch = fb_p;
    bi->fb_bpp = 32;

    // --- Reserve low pages for our tables (0x1000-0x8FFF) BEFORE exiting ---
    trace("[UEFI] reserving low page-table memory...\n");
    { UINT64 low = 0x1000ULL;
      if(EFI_ERROR(BS->AllocatePages(AllocateAddress, EfiLoaderData, 8, &low)) || low != 0x1000ULL){
          trace("[UEFI] FATAL: cannot reserve 0x1000-0x8FFF\n");
          print("Low memory reserve failed\r\n");
          for(;;) __asm__ volatile("hlt");
      } }
    trace("[UEFI] low memory reserved\n");

    // --- Exit boot services ---
    UINTN msize = 0, mkey = 0, dsize = 0; UINT32 dver = 0;
    BS->GetMemoryMap(&msize, 0, &mkey, &dsize, &dver);
    msize += 2 * dsize + 4096;
    VOID *mbuf = 0;
    BS->AllocatePool(EfiLoaderData, msize, &mbuf);
    EFI_STATUS st = BS->GetMemoryMap(&msize, mbuf, &mkey, &dsize, &dver);
    if(EFI_ERROR(st)){ print("GetMemoryMap failed\r\n"); for(;;) __asm__ volatile("hlt"); }
    st = BS->ExitBootServices(ImageHandle, mkey);
    if(EFI_ERROR(st)){ /* retry once */
        msize = 0; BS->GetMemoryMap(&msize, 0, &mkey, &dsize, &dver);
        // buffer already big enough
        BS->GetMemoryMap(&msize, mbuf, &mkey, &dsize, &dver);
        st = BS->ExitBootServices(ImageHandle, mkey);
        if(EFI_ERROR(st)){ trace("[UEFI] FATAL: ExitBootServices failed\n"); for(;;) __asm__ volatile("hlt"); }
    }
    __asm__ volatile("cli"); // no interrupts from here on (UEFI IDT is going away)
    trace("[UEFI] boot services off\n");

    // Staged load? copy to final 0x100000 now that memory is ours
    if(staging){
        UINT8 *d = (UINT8*)0x100000ULL, *s = (UINT8*)(UINTN)staging;
        for(UINTN i = 0; i < (UINTN)fsize; i++) d[i] = s[i];
        trace("[UEFI] kernel copied to 0x100000\n");
    }

    // --- Page tables mirroring BIOS layout: 0-64MB + GOP ---
    // PML4 0x1000, PDPT 0x2000, PD 0x3000 (0-1GB low), extra PDs for GOP.
    // PD pages: 0x3000 = low 0-1GB, 0x4000 = 1-2GB, 0x5000 = 2-3GB, 0x8000 = 3-4GB
    UINT64 *pml4 = (UINT64*)0x1000, *pdpt = (UINT64*)0x2000, *pd0 = (UINT64*)0x3000;
    UINT64 *pd1 = (UINT64*)0x4000, *pd2 = (UINT64*)0x5000, *pd3 = (UINT64*)0x8000;
    for(int i = 0; i < 512; i++){ pml4[i] = 0; pdpt[i] = 0; pd0[i] = 0; pd1[i] = 0; pd2[i] = 0; pd3[i] = 0; }
    pml4[0] = 0x2003; pml4[511] = 0x2003;
    // Full 0-4GB identity map: loader runs from high-loaded image pages,
    // so EVERYTHING below 4GB must stay mapped across the CR3 switch.
    pdpt[0] = 0x3003; pdpt[1] = 0x4003; pdpt[2] = 0x5003; pdpt[3] = 0x8003;
    for(int i = 0; i < 512; i++){
        pd0[i] = ((UINT64)i << 21) | 0x87;                    // 0-1GB
        pd1[i] = ((UINT64)i << 21) | (1ULL << 30) | 0x87;     // 1-2GB
        pd2[i] = ((UINT64)i << 21) | (2ULL << 30) | 0x87;     // 2-3GB
        pd3[i] = ((UINT64)i << 21) | (3ULL << 30) | 0x87;     // 3-4GB
    }
    pd0[510] = 0x83; // higher-half compat
    // GOP: map framebuffer window (aligned to 2MB)
    UINT64 gbase = fb_base & ~0x1FFFFFULL;
    UINT64 gend = fb_base + fb_size;
    for(UINT64 a = gbase; a < gend; a += 0x200000ULL){
        UINT64 idx = a >> 30;          // PDPT index (1GB each)
        UINT64 *pd = 0; UINT64 pd_addr = 0;
        if(idx == 0){ pd = pd0; }
        else if(idx == 1){ pd = pd1; pd_addr = 0x4003; }
        else if(idx == 2){ pd = pd2; pd_addr = 0x5003; }
        else if(idx == 3){ pd = pd3; pd_addr = 0x8003; }
        else continue;
        if(pd_addr && !(pdpt[idx] & 1)) pdpt[idx] = pd_addr;
        pd[(a >> 21) & 0x1FF] = a | 0x83;
    }
    // keep BIOS VBE window mapping too (harmless under UEFI)
    if(!(pdpt[3] & 1)) pdpt[3] = 0x8003;
    if(!(pd3[256] & 1)) pd3[256] = 0xE0000083;
    if(!(pd3[257] & 1)) pd3[257] = 0xE0200083;

    __asm__ volatile("mov %0, %%cr3" :: "r"((UINT64)0x1000) : "memory");

    // --- Own GDT at 0x7000 (null, code, data) ---
    UINT64 *gdt = (UINT64*)0x7000;
    gdt[0] = 0;
    gdt[1] = 0x00209A0000000000ULL;
    gdt[2] = 0x0000920000000000ULL;
    struct { UINT16 lim; UINT64 base; } __attribute__((packed)) gdtr = { 23, 0x7000 };
    __asm__ volatile("lgdt %0" :: "m"(gdtr) : "memory");
    __asm__ volatile(
        "mov $0x10, %%ax\n"
        "mov %%ax, %%ds\n mov %%ax, %%es\n mov %%ax, %%fs\n"
        "mov %%ax, %%gs\n mov %%ax, %%ss\n"
        ::: "rax", "memory");

    trace("[UEFI] paging+GDT ready, jumping to kernel\n");
    // stack + bootinfo pointer, jump to kernel _start (0x100000)
    // (interrupts already off since ExitBootServices)
    __asm__ volatile(
        "mov $0x90000, %%rsp\n"
        "mov $0x6000, %%rdi\n"
        "mov $0x100000, %%rax\n"
        "push $0x08\n"
        "lea 1f(%%rip), %%rcx\n"
        "push %%rcx\n"
        "lretq\n"
        "1:\n"
        "jmp *%%rax\n"
        ::: "rax", "rcx", "memory");
    for(;;) __asm__ volatile("hlt");
    (void)guid_eq; (void)bw; (void)bh;
}
