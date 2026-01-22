#pragma once

#include <stdint.h>
#include <OSType.h>

#ifndef CHAR16
typedef WCHAR CHAR16;
#endif


// Provide CODE_SEG("NAME") similar to Windows headers.
// For MSVC we use __pragma(code_seg(push, "NAME")) / __pragma(code_seg(pop)).
// For GCC/Clang we map to section attribute.
#if defined(_MSC_VER)
#  define CODE_SEG(seg) __pragma(code_seg(push, seg))
#  define END_CODE_SEG() __pragma(code_seg(pop))
#elif defined(__GNUC__)
#  define CODE_SEG(seg) __attribute__((section(seg)))
#  define END_CODE_SEG()
#else
#  define CODE_SEG(seg)
#  define END_CODE_SEG()
#endif

typedef struct _LDR_DATA_TABLE_ENTRY {
    LIST_ENTRY InLoadOrderLinks;      // Link ke modul berikutnya (Flink/Blink)
    PVOID DllBase;                    // Alamat virtual modul di memori (High-Half)
    PVOID EntryPoint;                 // Alamat fungsi inisialisasi modul
    ULONG SizeOfImage;                // Ukuran total modul di memori
    UNICODE_STRING FullDllName;       // Path lengkap (misal: \System32\Drivers\roskrnl.sys)
    UNICODE_STRING BaseDllName;       // Nama filenya aja (misal: roskrnl.sys)
    ULONG Flags;                      // Status (Loaded, Unloaded, dll)
    USHORT LoadCount;                 // Reference count
} LDR_DATA_TABLE_ENTRY, * PLDR_DATA_TABLE_ENTRY;

typedef struct _ROS_LOADER_PARAMETER_BLOCK {
    LDR_DATA_TABLE_ENTRY LoadOrderModuleList; // Linked List modul yang di-load
    // 1. Memory Management (The most important!)
    uintptr_t MemoryMapPhysicalAddress; // Alamat fisik EFI_MEMORY_DESCRIPTOR
    uint32_t MemoryMapDescriptorCount;
    uint32_t MemoryMapDescriptorSize;
    uintptr_t HhdmOffset;               // Alamat virtual basis HHDM (misal 0xFFFF8000...)

    // 2. Stack & Kernel Info
    uintptr_t KernelStackBase;          // RSP Base Address
    uintptr_t KernelStackLimit;         // Batas atas stack (buat nyegah overflow)
    uintptr_t KernelBase;               // Alamat virtual kernel (High-Half)
    uint32_t  KernelSize;

    // 3. Graphics (Biar kernel bisa langsung gambar OC kamu)
    struct {
        uint32_t* FrameBufferBase;      // Alamat virtual (HHDM) ke layar
        uint32_t  HorizontalResolution;
        uint32_t  VerticalResolution;
        uint32_t  PixelsPerScanLine;
    } GraphicsInfo;

    // 4. Arch Specific (Sangat NT-style)
    uintptr_t Pml4PhysicalAddress;      // CR3 yang diset sama Bootloader
    uintptr_t ConfigurationTable;       // Pointer ke ACPI Table (buat deteksi Core Ryzen-mu)

    struct _EPROCESS* Process;           // Pointer ke struktur EPROCESS dari kernel    
    struct _KTHREAD* Thread;             // Pointer ke struktur KTHREAD dari kernel 
    struct _KPRCB* Prcb;               // Pointer ke struktur KPRCB dari kernel
} RLOADER_PARAMETER_BLOCK, * RPLOADER_PARAMETER_BLOCK;

// Macro buat dapetin alamat awal struct dari alamat member-nya
#define CONTAINING_RECORD(address, type, field) \
    ((type *)((PCHAR)(address) - (uintptr_t)(&((type *)0)->field)))

// Minimal reimplementation untuk operasi list dari BaseLib
FORCEINLINE LIST_ENTRY*
ROSAPI
InsertTailList(
	IN OUT LIST_ENTRY* ListHead,
	IN OUT LIST_ENTRY* Entry
)
{
	LIST_ENTRY* Blink = ListHead->Blink;
	Entry->Flink = ListHead;
	Entry->Blink = Blink;
	Blink->Flink = Entry;
	ListHead->Blink = Entry;
	return ListHead;
}

FORCEINLINE ULONGLONG
ROSAPI
StrLen(
	IN CONST CHAR16* Str
)
{
	UINT64 Len = 0;
	while (Str[Len]) Len++;
	return Len;
}

// Bikin fungsi StrLen sendiri biar gak butuh library eksternal
// (gunakan StrLen di atas)

/* UNUSED 
ROSSTATUS
ROSAPI
KernelLoader(
	IN EFI_HANDLE ImageHandle,
	IN EFI_SYSTEM_TABLE* SysTable,
	IN CONST CHAR16* Path,
	IN uintptr_t MemoryRSPBaseAddress,
	IN uintptr_t KernelMemoryAddress,
	IN OUT RPLOADER_PARAMETER_BLOCK LoaderBlock,
	OUT UINT64* KernelEntryPoint
);

*/