// Minimal UEFI definitions for StrixOS loader - no gnu-efi needed.
// Compile loader.c with -mabi=ms (UEFI uses MS x64 calling convention).
#ifndef STRIX_EFI_H
#define STRIX_EFI_H

typedef unsigned char      UINT8;
typedef unsigned short     UINT16;
typedef unsigned int       UINT32;
typedef unsigned long long UINT64;
typedef UINT64             UINTN;
typedef long long          INTN;
typedef void               VOID;
typedef UINTN              EFI_STATUS;
typedef VOID*              EFI_HANDLE;
typedef UINT16             CHAR16;

#define EFI_SUCCESS 0
#define EFI_ERROR_MASK 0x8000000000000000ULL
#define EFI_ERROR(x) ((x) & EFI_ERROR_MASK)

typedef struct { UINT32 Data1; UINT16 Data2; UINT16 Data3; UINT8 Data4[8]; } EFI_GUID;

#define EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL_GUID \
    {0x387477c2,0x69c7,0x11d2,{0x8e,0x39,0x00,0xa0,0xc9,0x69,0x72,0x3b}}
#define EFI_LOADED_IMAGE_PROTOCOL_GUID \
    {0x5b1b31a1,0x9562,0x11d2,{0x8e,0x3f,0x00,0xa0,0xc9,0x69,0x72,0x3b}}
#define EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID \
    {0x0964e5b22,0x6459,0x11d2,{0x8e,0x39,0x00,0xa0,0xc9,0x69,0x72,0x3b}}
#define EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID \
    {0x9042a9de,0x23dc,0x4a38,{0x96,0xfb,0x7a,0xde,0xd0,0x80,0x51,0x6a}}

typedef struct EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL;
struct EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL {
    UINTN _reset;
    EFI_STATUS (*OutputString)(EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL*, CHAR16*);
    UINTN _test;
    UINTN _query;
    UINTN _setmode;
    UINTN _setattr;
    EFI_STATUS (*ClearScreen)(EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL*);
};

typedef struct {
    UINT32 RedMask; UINT32 GreenMask; UINT32 BlueMask; UINT32 ReservedMask;
} EFI_PIXEL_BITMASK;
typedef enum { PixelRedGreenBlueReserved8BitPerColor, PixelBlueGreenRedReserved8BitPerColor,
               PixelBitMask, PixelBltOnly, PixelFormatMax } EFI_GRAPHICS_PIXEL_FORMAT;
typedef struct {
    UINT32 Version; UINT32 HorizontalResolution; UINT32 VerticalResolution;
    EFI_GRAPHICS_PIXEL_FORMAT PixelFormat; EFI_PIXEL_BITMASK PixelInformation;
    UINT32 PixelsPerScanLine;
} EFI_GRAPHICS_OUTPUT_MODE_INFORMATION;
typedef struct {
    UINT32 MaxMode; UINT32 Mode; EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *Info;
    UINTN SizeOfInfo; UINT64 FrameBufferBase; UINTN FrameBufferSize;
} EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE;
typedef struct EFI_GRAPHICS_OUTPUT_PROTOCOL EFI_GRAPHICS_OUTPUT_PROTOCOL;
struct EFI_GRAPHICS_OUTPUT_PROTOCOL {
    EFI_STATUS (*QueryMode)(EFI_GRAPHICS_OUTPUT_PROTOCOL*, UINT32, UINTN*, EFI_GRAPHICS_OUTPUT_MODE_INFORMATION**);
    EFI_STATUS (*SetMode)(EFI_GRAPHICS_OUTPUT_PROTOCOL*, UINT32);
    UINTN _blt;
    EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE *Mode;
};

typedef struct {
    UINT32 Revision; EFI_HANDLE ParentHandle; VOID *SystemTable;
    EFI_HANDLE DeviceHandle; VOID *FilePath; VOID *Reserved;
    UINT32 LoadOptionsSize; VOID *LoadOptions; VOID *ImageBase;
    UINT64 ImageSize; UINT32 ImageCodeType; UINT32 ImageDataType;
} EFI_LOADED_IMAGE_PROTOCOL;

typedef struct EFI_FILE_PROTOCOL EFI_FILE_PROTOCOL;
typedef UINT64 EFI_FILE_OPEN_MODE;
struct EFI_FILE_PROTOCOL {
    UINT64 Revision;
    EFI_STATUS (*Open)(EFI_FILE_PROTOCOL*, EFI_FILE_PROTOCOL**, CHAR16*, UINT64, UINT64);
    EFI_STATUS (*Close)(EFI_FILE_PROTOCOL*);
    EFI_STATUS (*Delete)(EFI_FILE_PROTOCOL*);
    EFI_STATUS (*Read)(EFI_FILE_PROTOCOL*, UINTN*, VOID*);
    EFI_STATUS (*Write)(EFI_FILE_PROTOCOL*, UINTN*, VOID*);
    EFI_STATUS (*GetPosition)(EFI_FILE_PROTOCOL*, UINT64*);
    EFI_STATUS (*SetPosition)(EFI_FILE_PROTOCOL*, UINT64);
    EFI_STATUS (*GetInfo)(EFI_FILE_PROTOCOL*, EFI_GUID*, UINTN*, VOID*);
    EFI_STATUS (*SetInfo)(EFI_FILE_PROTOCOL*, EFI_GUID*, UINTN*, VOID*);
    EFI_STATUS (*Flush)(EFI_FILE_PROTOCOL*);
};
#define EFI_FILE_MODE_READ 0x0000000000000001ULL
typedef struct EFI_SIMPLE_FILE_SYSTEM_PROTOCOL EFI_SIMPLE_FILE_SYSTEM_PROTOCOL;
struct EFI_SIMPLE_FILE_SYSTEM_PROTOCOL {
    UINT64 Revision;
    EFI_STATUS (*OpenVolume)(EFI_SIMPLE_FILE_SYSTEM_PROTOCOL*, EFI_FILE_PROTOCOL**);
};

typedef struct {
    UINT8 Type; UINT8 SubType; UINT8 Length[2];
} EFI_DEVICE_PATH_PROTOCOL;

typedef struct {
    char Hdr[24];
    VOID *FirmwareVendor;
    UINT32 FirmwareRevision;
    UINT32 _pad0;
    EFI_HANDLE ConsoleInHandle;
    VOID *ConIn;
    EFI_HANDLE ConsoleOutHandle;
    EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *ConOut; // offset 64
    EFI_HANDLE StandardErrorHandle;
    VOID *StdErr;
    VOID *RuntimeServices;
    struct EFI_BOOT_SERVICES *BootServices;  // offset 96
} EFI_SYSTEM_TABLE;

typedef enum { AllocateAnyPages, AllocateMaxAddress, AllocateAddress, MaxAllocateType } EFI_ALLOCATE_TYPE;
typedef enum { EfiReservedMemoryType, EfiLoaderCode, EfiLoaderData, EfiBootServicesCode,
               EfiBootServicesData, EfiRuntimeServicesCode, EfiRuntimeServicesData,
               EfiConventionalMemory, EfiUnusableMemory, EfiACPIReclaimMemory,
               EfiACPIMemoryNVS, EfiMemoryMappedIO, EfiMemoryMappedIOPortSpace,
               EfiPalCode, EfiPersistentMemory, MaxMemoryType } EFI_MEMORY_TYPE;
typedef struct { UINT32 Type; UINT64 PhysicalStart; UINT64 VirtualStart;
                 UINT64 NumberOfPages; UINT64 Attribute; } EFI_MEMORY_DESCRIPTOR;

typedef enum { ByProtocol, GetProtocol, TestProtocol } EFI_LOCATE_SEARCH_TYPE;
typedef struct EFI_BOOT_SERVICES EFI_BOOT_SERVICES;
struct EFI_BOOT_SERVICES {
    char _hdr[24];                                            // 0  Hdr
    UINTN _raise_tpl; UINTN _restore_tpl;                     // 1-2
    EFI_STATUS (*AllocatePages)(EFI_ALLOCATE_TYPE, EFI_MEMORY_TYPE, UINTN, UINT64*); // 3
    EFI_STATUS (*FreePages)(UINT64, UINTN);                   // 4
    EFI_STATUS (*GetMemoryMap)(UINTN*, EFI_MEMORY_DESCRIPTOR*, UINTN*, UINTN*, UINT32*); // 5
    EFI_STATUS (*AllocatePool)(EFI_MEMORY_TYPE, UINTN, VOID**); // 6
    EFI_STATUS (*FreePool)(VOID*);                            // 7
    EFI_STATUS (*CreateEvent)(UINT32, UINTN, VOID*, VOID*, VOID*); // 8
    EFI_STATUS (*SetTimer)(VOID*, INTN, UINT64);              // 9
    EFI_STATUS (*WaitForEvent)(UINTN, VOID**, UINTN*);        // 10
    UINTN _signal; UINTN _close; UINTN _check;                // 11-13
    EFI_STATUS (*InstallProtocolInterface)(EFI_HANDLE*, EFI_GUID*, INTN, VOID*); // 14
    EFI_STATUS (*ReinstallProtocolInterface)(EFI_HANDLE, EFI_GUID*, VOID*, VOID*); // 15
    EFI_STATUS (*UninstallProtocolInterface)(EFI_HANDLE, EFI_GUID*, VOID*); // 16
    EFI_STATUS (*HandleProtocol)(EFI_HANDLE, EFI_GUID*, VOID**); // 17
    UINTN _reserved;                                          // 18
    UINTN _register;                                          // 19 RegisterProtocolNotify
    EFI_STATUS (*LocateHandle)(EFI_LOCATE_SEARCH_TYPE, EFI_GUID*, VOID*, UINTN*, EFI_HANDLE*); // 20
    UINTN _locate_device;                                     // 21 LocateDevicePath
    EFI_STATUS (*InstallConfigurationTable)(EFI_GUID*, VOID*); // 22
    EFI_STATUS (*ImageLoad)(UINT8, EFI_HANDLE, EFI_DEVICE_PATH_PROTOCOL*, VOID*, UINTN, EFI_HANDLE*); // 23 LoadImage
    EFI_STATUS (*ImageStart)(EFI_HANDLE, UINTN*, CHAR16**);   // 24 StartImage
    EFI_STATUS (*Exit)(EFI_HANDLE, EFI_STATUS, UINTN, CHAR16*); // 25
    EFI_STATUS (*ImageUnload)(EFI_HANDLE);                    // 26 UnloadImage
    EFI_STATUS (*ExitBootServices)(EFI_HANDLE, UINTN);        // 27
    UINTN _get_tick;                                          // 28 GetNextMonotonicCount
    EFI_STATUS (*Stall)(UINTN);                               // 29
    EFI_STATUS (*SetWatchdogTimer)(UINTN, UINT64, UINTN, CHAR16*); // 30
    UINTN _connect; UINTN _disconnect; UINTN _open; UINTN _close2; UINTN _open_info; // 31-35
    UINTN _per_handle;                                        // 36 ProtocolsPerHandle
    EFI_STATUS (*LocateHandleBuffer)(EFI_LOCATE_SEARCH_TYPE, EFI_GUID*, VOID*, UINTN*, EFI_HANDLE**); // 37
    EFI_STATUS (*LocateProtocol)(EFI_GUID*, VOID*, VOID**);   // 38
};

#endif
