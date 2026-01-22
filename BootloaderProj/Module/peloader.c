#include <Uefi.h>
#include <Library/UefiBootServicesTableLib.h>
#include <intrin.h>
#include "../InInclude/pestruct.h"

// Assembly helper to write CR3 (implemented in asmhelpers.asm)
VOID REFIXAsmWriteCr3(UINT64 Value);

void EFIAPI REFIXHandoverToKernel(
	IN UINT64 EntryPoint,
	IN UINT64 Pml4Physical,
	IN UINT64 KernelStackInfo,
	IN VOID* LoaderBlock
);


STATIC VOID PeZeroMem(VOID* Buffer, UINTN Size)
{
	UINT8* Ptr = (UINT8*)Buffer;
	for (UINTN i = 0; i < Size; ++i) {
		Ptr[i] = 0;
	}
}

STATIC VOID PeCopyMem(VOID* Dest, CONST VOID* Src, UINTN Size)
{
	UINT8* D = (UINT8*)Dest;
	const UINT8* S = (const UINT8*)Src;
	for (UINTN i = 0; i < Size; ++i) {
		D[i] = S[i];
	}
}

#define PAGE_PRESENT 0x001ULL
#define PAGE_RW      0x002ULL
#define PAGE_PS      0x080ULL // 2MB large page
#define PAGE_SIZE    4096ULL

STATIC VOID* PeAllocPage(EFI_SYSTEM_TABLE* SysTable)
{
	EFI_PHYSICAL_ADDRESS Addr = 0;
	EFI_STATUS Status = SysTable->BootServices->AllocatePages(AllocateAnyPages, EfiLoaderData, 1, &Addr);
	if (EFI_ERROR(Status)) {
		return NULL;
	}
	PeZeroMem((VOID*)(UINTN)Addr, PAGE_SIZE);
	return (VOID*)(UINTN)Addr;
}

STATIC EFI_STATUS PeMapRange(
    IN EFI_SYSTEM_TABLE* SysTable,
    IN UINT64* Pml4,
    IN UINT64 VirtStart,
    IN UINT64 PhysStart,
    IN UINT64 Size
)
{
    UINT64 virtPageStart = VirtStart & ~((UINT64)0xFFF);
    UINT64 physPageStart = PhysStart & ~((UINT64)0xFFF);
    UINT64 pageOffset = VirtStart - virtPageStart;
    UINT64 total = Size + pageOffset;
    UINTN pageCount = (UINTN)((total + 0xFFF) / 0x1000);

    for (UINTN page = 0; page < pageCount; ++page) {
        UINT64 vaddr = virtPageStart + ((UINT64)page * 0x1000);
        UINT64 paddr = physPageStart + ((UINT64)page * 0x1000);

        UINTN pml4Index = (UINTN)((vaddr >> 39) & 0x1FF);
        UINTN pdptIndex = (UINTN)((vaddr >> 30) & 0x1FF);
        UINTN pdIndex   = (UINTN)((vaddr >> 21) & 0x1FF);
        UINTN ptIndex   = (UINTN)((vaddr >> 12) & 0x1FF);

        UINT64* Pdpt;
        if ((Pml4[pml4Index] & PAGE_PRESENT) == 0) {
            Pdpt = (UINT64*)PeAllocPage(SysTable);
            if (!Pdpt) return EFI_OUT_OF_RESOURCES;
            Pml4[pml4Index] = (UINT64)(UINTN)Pdpt | PAGE_PRESENT | PAGE_RW;
        } else {
            Pdpt = (UINT64*)(UINTN)(Pml4[pml4Index] & ~((UINT64)0xFFF));
        }

        UINT64* Pd;
        if ((Pdpt[pdptIndex] & PAGE_PRESENT) == 0) {
            Pd = (UINT64*)PeAllocPage(SysTable);
            if (!Pd) return EFI_OUT_OF_RESOURCES;
            Pdpt[pdptIndex] = (UINT64)(UINTN)Pd | PAGE_PRESENT | PAGE_RW;
        } else {
            Pd = (UINT64*)(UINTN)(Pdpt[pdptIndex] & ~((UINT64)0xFFF));
        }

        UINT64* Pt;
        if ((Pd[pdIndex] & PAGE_PRESENT) == 0) {
            Pt = (UINT64*)PeAllocPage(SysTable);
            if (!Pt) return EFI_OUT_OF_RESOURCES;
            Pd[pdIndex] = (UINT64)(UINTN)Pt | PAGE_PRESENT | PAGE_RW;
        } else {
            Pt = (UINT64*)(UINTN)(Pd[pdIndex] & ~((UINT64)0xFFF));
        }

        Pt[ptIndex] = paddr | PAGE_PRESENT | PAGE_RW;
    }
    return EFI_SUCCESS;
}

EFI_STATUS
PeBuildTemporaryPml4(
	IN EFI_SYSTEM_TABLE* SysTable,
	IN UINT64 HhdmOffset,
	IN UINT64 KernelPhysBase,
	IN UINT64 KernelSize,
	IN UINT64 KernelVirtBase,
    IN UINT64 StackPhysBase,
    IN UINT64 StackVirtTop,
    IN UINT64 StackSize,
	OUT UINT64* Pml4Physical
)
{
	if (!SysTable || !Pml4Physical) return EFI_INVALID_PARAMETER;

	UINT64* Pml4 = (UINT64*)PeAllocPage(SysTable);
	if (!Pml4) return EFI_OUT_OF_RESOURCES;

	UINT64* Pdpt = (UINT64*)PeAllocPage(SysTable);
	if (!Pdpt) return EFI_OUT_OF_RESOURCES;

	// Link PML4 entries: identity map at index 0, HHDM map at calculated index.
	Pml4[0] = (UINT64)(UINTN)Pdpt | PAGE_PRESENT | PAGE_RW;
	UINTN HhdmIndex = (UINTN)((HhdmOffset >> 39) & 0x1FF);
	if (HhdmIndex < 512) {
		Pml4[HhdmIndex] = (UINT64)(UINTN)Pdpt | PAGE_PRESENT | PAGE_RW;
	}

	// For simplicity, map first 4GB with 2MB pages (4 PD tables)
	for (UINTN pdptEntry = 0; pdptEntry < 4; ++pdptEntry) {
		UINT64* Pd = (UINT64*)PeAllocPage(SysTable);
		if (!Pd) return EFI_OUT_OF_RESOURCES;

		Pdpt[pdptEntry] = (UINT64)(UINTN)Pd | PAGE_PRESENT | PAGE_RW;

		for (UINTN pdIndex = 0; pdIndex < 512; ++pdIndex) {
			UINT64 phys = (pdptEntry * 0x40000000ULL) + (pdIndex * 0x200000ULL); // 1GB chunks * 2MB pages
			Pd[pdIndex] = phys | PAGE_PRESENT | PAGE_RW | PAGE_PS;
		}
	}

	// Map kernel image
	if (KernelSize > 0) {
        EFI_STATUS Status = PeMapRange(SysTable, Pml4, KernelVirtBase, KernelPhysBase, KernelSize);
        if (EFI_ERROR(Status)) return Status;
	}

    // Map kernel stack
    if (StackSize > 0) {
        // Stack grows down, so PhysBase maps to VirtTop - Size
        EFI_STATUS Status = PeMapRange(SysTable, Pml4, StackVirtTop - StackSize, StackPhysBase, StackSize);
        if (EFI_ERROR(Status)) return Status;
    }

	*Pml4Physical = (UINT64)(UINTN)Pml4;
	return EFI_SUCCESS;
}

EFI_STATUS
REFIXJumpToKernel(
    IN EFI_HANDLE ImageHandle,
    IN EFI_SYSTEM_TABLE* SysTable,
	IN UINT64 EntryPoint,
	IN UINT64 Pml4Physical,
	IN RPLOADER_PARAMETER_BLOCK LoaderBlock
)
{
	if (ImageHandle == NULL || SysTable == NULL || EntryPoint == 0 || Pml4Physical == 0 || LoaderBlock == NULL) {
		return EFI_INVALID_PARAMETER;
	}

	EFI_STATUS Status;
	EFI_MEMORY_DESCRIPTOR* MemoryMap = NULL;
	UINTN MemoryMapSize = 0;
	UINTN MapKey = 0;
	UINTN DescriptorSize = 0;
	UINT32 DescriptorVersion = 0;

	Status = SysTable->BootServices->GetMemoryMap(&MemoryMapSize, MemoryMap, &MapKey, &DescriptorSize, &DescriptorVersion);
	if (Status != EFI_BUFFER_TOO_SMALL) {
		return Status;
	}

	MemoryMapSize += DescriptorSize * 2; // sedikit headroom
	Status = SysTable->BootServices->AllocatePool(EfiLoaderData, MemoryMapSize, (VOID**)&MemoryMap);
	if (EFI_ERROR(Status)) {
		return Status;
	}

	Status = SysTable->BootServices->GetMemoryMap(&MemoryMapSize, MemoryMap, &MapKey, &DescriptorSize, &DescriptorVersion);
	if (EFI_ERROR(Status)) {
		return Status;
	}

	LoaderBlock->MemoryMapPhysicalAddress = (uintptr_t)(UINTN)MemoryMap;
	LoaderBlock->MemoryMapDescriptorCount = (uint32_t)(MemoryMapSize / DescriptorSize);
	LoaderBlock->MemoryMapDescriptorSize = (uint32_t)DescriptorSize;
	LoaderBlock->ConfigurationTable = (uintptr_t)(UINTN)SysTable->ConfigurationTable;

	Status = SysTable->BootServices->ExitBootServices(ImageHandle, MapKey);
	if (Status == EFI_INVALID_PARAMETER) {
		Status = SysTable->BootServices->GetMemoryMap(&MemoryMapSize, MemoryMap, &MapKey, &DescriptorSize, &DescriptorVersion);
		if (EFI_ERROR(Status)) {
			return Status;
		}

		LoaderBlock->MemoryMapPhysicalAddress = (uintptr_t)(UINTN)MemoryMap;
		LoaderBlock->MemoryMapDescriptorCount = (uint32_t)(MemoryMapSize / DescriptorSize);
		LoaderBlock->MemoryMapDescriptorSize = (uint32_t)DescriptorSize;

		Status = SysTable->BootServices->ExitBootServices(ImageHandle, MapKey);
	}
	if (EFI_ERROR(Status)) {
		return Status;
	}

	// Call kernel entry point with loader block
	typedef void (*KERNEL_ENTRY)(RPLOADER_PARAMETER_BLOCK);
	KERNEL_ENTRY Kernel = (KERNEL_ENTRY)(UINTN)EntryPoint;
	REFIXHandoverToKernel(EntryPoint, Pml4Physical, LoaderBlock->KernelStackBase, (VOID*)LoaderBlock);

	return EFI_SUCCESS; // Should not return jika kernel takeover
}

STATIC
EFI_STATUS
PeValidateHeaders(
	IN CONST UINT8* ImageBase,
	IN UINTN ImageSize,
	OUT CONST IMAGE_DOS_HEADER** Dos,
	OUT CONST IMAGE_NT_HEADERS64** Nt
)
{
	if (ImageBase == NULL || ImageSize < sizeof(IMAGE_DOS_HEADER)) {
		return EFI_INVALID_PARAMETER;
	}

	const IMAGE_DOS_HEADER* DosHdr = (CONST IMAGE_DOS_HEADER*)ImageBase;
	if (DosHdr->e_magic != IMAGE_DOS_SIGNATURE || DosHdr->e_lfanew <= 0) {
		return EFI_LOAD_ERROR;
	}

	if ((UINTN)DosHdr->e_lfanew + sizeof(IMAGE_NT_HEADERS64) > ImageSize) {
		return EFI_LOAD_ERROR;
	}

	const IMAGE_NT_HEADERS64* NtHdr = (CONST IMAGE_NT_HEADERS64*)(ImageBase + DosHdr->e_lfanew);
	if (NtHdr->Signature != IMAGE_NT_SIGNATURE || NtHdr->OptionalHeader.Magic != 0x20B) {
		return EFI_LOAD_ERROR;
	}

	*Dos = DosHdr;
	*Nt = NtHdr;
	return EFI_SUCCESS;
}

EFI_STATUS
PeLoadImageToMemory(
	IN CONST VOID* FileBuffer,
	IN UINTN FileSize,
	IN VOID* LoadBase,
	OUT PE_LOAD_RESULT* Result
)
{
	const UINT8* ImageBytes = (const UINT8*)FileBuffer;
	const IMAGE_DOS_HEADER* DosHdr;
	const IMAGE_NT_HEADERS64* NtHdr;

	EFI_STATUS Status = PeValidateHeaders(ImageBytes, FileSize, &DosHdr, &NtHdr);
	if (EFI_ERROR(Status)) {
		return Status;
	}

	if (LoadBase == NULL || Result == NULL) {
		return EFI_INVALID_PARAMETER;
	}

	UINT8* Target = (UINT8*)LoadBase;
	UINT32 ImageSize = NtHdr->OptionalHeader.SizeOfImage;
	UINT32 HeaderSize = NtHdr->OptionalHeader.SizeOfHeaders;

	if (HeaderSize > FileSize || ImageSize < HeaderSize) {
		return EFI_LOAD_ERROR;
	}

	// Clear destination image region
	PeZeroMem(Target, ImageSize);

	// Copy headers
	PeCopyMem(Target, ImageBytes, HeaderSize);

	// Copy each section respecting VirtualAddress
	const IMAGE_SECTION_HEADER* Section = (const IMAGE_SECTION_HEADER*)((const UINT8*)&NtHdr->OptionalHeader + NtHdr->FileHeader.SizeOfOptionalHeader);
	for (UINT16 i = 0; i < NtHdr->FileHeader.NumberOfSections; ++i, ++Section) {
		UINT32 RawSize = Section->SizeOfRawData;
		UINT32 RawOffset = Section->PointerToRawData;
		UINT32 VirtualSize = Section->Misc.VirtualSize;

		// Avoid overruns on source buffer
		if (RawOffset > FileSize) {
			return EFI_LOAD_ERROR;
		}

		UINT32 CopySize = RawSize;
		if (RawOffset + CopySize > FileSize) {
			if (RawOffset >= FileSize) {
				CopySize = 0;
			} else {
				CopySize = (UINT32)(FileSize - RawOffset);
			}
		}

		UINT8* Dest = Target + Section->VirtualAddress;
		if ((UINTN)(Section->VirtualAddress + RawSize) > ImageSize) {
			return EFI_LOAD_ERROR;
		}

		if (CopySize > 0) {
			PeCopyMem(Dest, ImageBytes + RawOffset, CopySize);
		}

		// Ensure BSS within VirtualSize is zeroed (already zeroed by ZeroMem)
		if (VirtualSize > RawSize && (UINTN)(Section->VirtualAddress + VirtualSize) > ImageSize) {
			return EFI_LOAD_ERROR;
		}
	}

	Result->EntryPoint = (UINT64)(UINTN)(Target + NtHdr->OptionalHeader.AddressOfEntryPoint);
	Result->ImageBase = (UINT64)(UINTN)Target;
	Result->SizeOfImage = ImageSize;

	return EFI_SUCCESS;
}
