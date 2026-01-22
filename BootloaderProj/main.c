#include <Uefi.h>
#include <Library/UefiLib.h>
#include "InInclude/efibase.h"
#include <osbase.h>
#include "InInclude/pestruct.h"
#include <OSType.h>

EFI_STATUS
EFIAPI
KernelLoader(
	IN EFI_HANDLE ImageHandle,
	IN EFI_SYSTEM_TABLE* SysTable,
	IN CONST CHAR16* Path,
	IN uintptr_t MemoryRSPBaseAddress,
	IN uintptr_t KernelMemoryAddress,
	IN OUT RPLOADER_PARAMETER_BLOCK LoaderBlock,
	OUT UINT64* KernelEntryPoint
);

STATIC CONST CHAR16* gKernelPath = L"\\EFI\\BOOT\\roskrnl.exe";

// 2. Direct Mapping (HHDM) - Tempat kamu akses RAM fisik langsung
// Di Windows ini mirip area 'Non-Paged Pool'
#define HIGHER_HALF_KERNEL_MEMORY       0xFFFF800000000000

// 3. Kernel Image Base (.text, .data) 
// Ini standar Windows x64 (Kernel Half-High)
#define HIGHER_HALF_KERNEL_TEXT_MEMORY  0xFFFFF80000000000

// 4. Kernel Stack (NT hobi naruh stack di area yang sangat tinggi)
#define KERNEL_STACK_BASE               0xFFFFB00000000000

EFI_STATUS EFIAPI EfiMain(
    IN EFI_HANDLE        ImageHandle,      // <--- Parameter 1: ID Aplikasi Lu
    IN EFI_SYSTEM_TABLE* SystemTable      // <--- Parameter 2: Pointer ke "Alat-alat" UEFI
)
{
    EFI_STATUS Status;
    EFI_INPUT_KEY Key;
    UINTN EventIndex;

	SystemTable->ConIn->Reset(SystemTable->ConIn, FALSE);
	SystemTable->ConOut->ClearScreen(SystemTable->ConOut);

    SystemTable->ConOut->OutputString(SystemTable->ConOut, L"ROS::RENDEROBJECTS OPERATING SYSTEM LOADER v0.1\r\n");
	SystemTable->ConOut->OutputString(SystemTable->ConOut, L"1. Load ROS Kernel Normal\r\n");
	SystemTable->ConOut->OutputString(SystemTable->ConOut, L"2. Masuk ke Debug Mode\r\n");
	SystemTable->ConOut->OutputString(SystemTable->ConOut, L"3. Reboot\r\n");

	SystemTable->BootServices->WaitForEvent(1, &SystemTable->ConIn->WaitForKey, &EventIndex);
	Status = SystemTable->ConIn->ReadKeyStroke(SystemTable->ConIn, &Key);
    
    if (!EFI_ERROR(Status)) {
        if (Key.UnicodeChar == L'1') {
			SystemTable->ConOut->OutputString(SystemTable->ConOut, L"Loading ROS Kernel...\r\n");

			EFI_PHYSICAL_ADDRESS KernelPhysBase = 0x2000000; // Alokasikan di 32MB
            UINTN KernelPages = 4096;
            Status = SystemTable->BootServices->AllocatePages(AllocateAnyPages, EfiLoaderData, KernelPages, &KernelPhysBase);
            if (EFI_ERROR(Status)) return Status;

			// Siapkan Loader Parameter Block
			RPLOADER_PARAMETER_BLOCK LoaderBlock = NULL;
			Status = SystemTable->BootServices->AllocatePool(EfiLoaderData, sizeof(RLOADER_PARAMETER_BLOCK), (VOID**)&LoaderBlock);
			if (EFI_ERROR(Status)) return Status;
			SystemTable->BootServices->SetMem(LoaderBlock, sizeof(RLOADER_PARAMETER_BLOCK), 0);

			// Init list head untuk module list
			LoaderBlock->LoadOrderModuleList.InLoadOrderLinks.Flink = &LoaderBlock->LoadOrderModuleList.InLoadOrderLinks;
			LoaderBlock->LoadOrderModuleList.InLoadOrderLinks.Blink = &LoaderBlock->LoadOrderModuleList.InLoadOrderLinks;

			// HHDM offset
			LoaderBlock->HhdmOffset = HIGHER_HALF_KERNEL_MEMORY;

			// Stack info
			UINTN StackSize = 0x8000; // 32KB
			EFI_PHYSICAL_ADDRESS StackPhys = 0;
			Status = SystemTable->BootServices->AllocatePages(AllocateAnyPages, EfiLoaderData, EFI_SIZE_TO_PAGES(StackSize), &StackPhys);
			if (EFI_ERROR(Status)) return Status;
			SystemTable->BootServices->SetMem((VOID*)(UINTN)StackPhys, StackSize, 0);

			LoaderBlock->KernelStackBase = KERNEL_STACK_BASE;
			LoaderBlock->KernelStackLimit = KERNEL_STACK_BASE - StackSize;

			UINT64 KernelEntryPoint = 0;


			// Load kernel image ke memory (physical base)
			Status = KernelLoader(
				ImageHandle,
				SystemTable,
				gKernelPath,
				LoaderBlock->KernelStackBase,
				(uintptr_t)(UINTN)KernelPhysBase,
			LoaderBlock,
			&KernelEntryPoint
			);
			if (EFI_ERROR(Status)) {
				SystemTable->ConOut->OutputString(SystemTable->ConOut, L"Kernel load failed\r\n");
				return Status;
			}

			// Build temporary PML4 for HHDM jump + map kernel high-half
			UINT64 Pml4Phys = 0;
			Status = PeBuildTemporaryPml4(
				SystemTable,
				LoaderBlock->HhdmOffset,
				(UINT64)(UINTN)KernelPhysBase,
				LoaderBlock->KernelSize,
				HIGHER_HALF_KERNEL_TEXT_MEMORY,
				(UINT64)(UINTN)StackPhys,
				LoaderBlock->KernelStackBase,
				StackSize,
				&Pml4Phys);
			if (EFI_ERROR(Status)) return Status;
			LoaderBlock->Pml4PhysicalAddress = Pml4Phys;
			LoaderBlock->KernelBase = HIGHER_HALF_KERNEL_TEXT_MEMORY;
			
			// initialize idle thread and idle process here
			EFI_PHYSICAL_ADDRESS PcrPhys = 0;
			Status = SystemTable->BootServices->AllocatePages(AllocateAnyPages, EfiLoaderData, EFI_SIZE_TO_PAGES(sizeof(KIPCR)), &PcrPhys);
			if (EFI_ERROR(Status)) {
				SystemTable->ConOut->OutputString(SystemTable->ConOut, L"Failed to allocate PCR\r\n");
				return Status;
			}
			PKIPCR Pcr = (PKIPCR)(UINTN)PcrPhys;
			SystemTable->BootServices->SetMem(Pcr, sizeof(KIPCR), 0);

            UINT64 PcrVa = (UINT64)PcrPhys + HIGHER_HALF_KERNEL_MEMORY;
			Pcr->Self = (PKIPCR)PcrVa;
			Pcr->PrcbData.Number = 0;
			Pcr->PrcbData.SetMember = 1 << 0; // CPU 0

			PKPRCB Prcb = &Pcr->PrcbData;
            UINT64 PrcbVa = PcrVa + ((UINT64)Prcb - (UINT64)Pcr);

			EFI_PHYSICAL_ADDRESS ProcessPhys = 0;
			Status = SystemTable->BootServices->AllocatePages(AllocateAnyPages, EfiLoaderData, EFI_SIZE_TO_PAGES(sizeof(EPROCESS)), &ProcessPhys);
			if (EFI_ERROR(Status)) {
				SystemTable->ConOut->OutputString(SystemTable->ConOut, L"Failed to allocate Process\r\n");
				return Status;
			}
			PEPROCESS IdleProcess = (PEPROCESS)(UINTN)ProcessPhys;
			SystemTable->BootServices->SetMem(IdleProcess, sizeof(EPROCESS), 0);
            UINT64 ProcessVa = (UINT64)ProcessPhys + HIGHER_HALF_KERNEL_MEMORY;
			
			IdleProcess->Pcb.DirectoryTableBase = Pml4Phys;

			EFI_PHYSICAL_ADDRESS ThreadPhys = 0;
			Status = SystemTable->BootServices->AllocatePages(AllocateAnyPages, EfiLoaderData, EFI_SIZE_TO_PAGES(sizeof(KTHREAD)), &ThreadPhys);
			if (EFI_ERROR(Status)) {
				SystemTable->ConOut->OutputString(SystemTable->ConOut, L"Failed to allocate Thread\r\n");
				return Status;
			}
			PKTHREAD Thread = (PKTHREAD)(UINTN)ThreadPhys;
			SystemTable->BootServices->SetMem(Thread, sizeof(KTHREAD), 0);
            UINT64 ThreadVa = (UINT64)ThreadPhys + HIGHER_HALF_KERNEL_MEMORY;

			// LINKING: THE CIRCLE OF LIFE
			// Sambungkan semua pointer sesuai standar NT
	
			// PRCB -> Thread (CPU lagi jalanin thread apa?)
			Prcb->CurrentThread = (PKTHREAD)ThreadVa;
			Prcb->NextThread = NULL;
			Prcb->IdleThread = (PKTHREAD)ThreadVa;

			// Thread -> Process (Thread ini anaknya siapa?)
			Thread->ApcState.Process = (PEPROCESS)ProcessVa;

			// Thread -> Stack (Thread ini stack-nya dimana?)
			// Kita pakai stack yang baru saja kita alokasikan di atas
			Thread->InitialStack = (PVOID)(UINTN)LoaderBlock->KernelStackBase;
			Thread->StackLimit = (PVOID)(UINTN)LoaderBlock->KernelStackLimit;
			Thread->KernelStack = (PVOID)(UINTN)LoaderBlock->KernelStackBase;

			// Simpan ke LoaderBlock biar kernel bisa akses
			LoaderBlock->Prcb = (PKPRCB)PrcbVa;
			LoaderBlock->Process = (PEPROCESS)ProcessVa;
			LoaderBlock->Thread = (PKTHREAD)ThreadVa;

			// Adjust entrypoint to mapped high-half address
			UINT64 KernelEntryVirtual = (KernelEntryPoint - (UINT64)(UINTN)KernelPhysBase) + HIGHER_HALF_KERNEL_TEXT_MEMORY;

			SystemTable->ConOut->OutputString(SystemTable->ConOut, L"Kernel loaded. Ready to jump.\r\n");

			Status = REFIXJumpToKernel(ImageHandle, SystemTable, KernelEntryVirtual, Pml4Phys, LoaderBlock);
			return Status;


        }
        else if (Key.UnicodeChar == L'2') {
            // Masuk ke Debug Mode
        }
        else if (Key.UnicodeChar == L'3') {
            // Reboot
        }
    }

    return EFI_SUCCESS;
}