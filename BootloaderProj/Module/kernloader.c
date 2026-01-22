#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <stdint.h>
#include "../InInclude/efibase.h"
#include <Library/BaseLib.h>
#include "../InInclude/pestruct.h"

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
)
{
	EFI_STATUS Status;
	EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* SimpleFSProtocol;
	EFI_LOADED_IMAGE_PROTOCOL* Protocol;
	EFI_FILE_HANDLE Root, KernelFile;

	Status = SysTable->BootServices->HandleProtocol(ImageHandle, &gEfiLoadedImageProtocolGuid, (VOID**)&Protocol);
	if (EFI_ERROR(Status)) return Status;

	Status = SysTable->BootServices->HandleProtocol(Protocol->DeviceHandle, &gEfiSimpleFileSystemProtocolGuid, (VOID**)&SimpleFSProtocol);
	if (EFI_ERROR(Status)) return Status;

	Status = SimpleFSProtocol->OpenVolume(SimpleFSProtocol, &Root);
	if (EFI_ERROR(Status)) return Status;

	Status = Root->Open(Root, &KernelFile, (CHAR16*)Path, EFI_FILE_MODE_READ, 0);
	if (EFI_ERROR(Status)) return Status;

	// LOAD KERNEL TO MEMORY
	UINTN FileSize = 0;
	EFI_FILE_INFO* FileInfo = 0;
	UINTN InfoSize = 0;

	Status = KernelFile->GetInfo(KernelFile, &gEfiFileInfoGuid, &InfoSize, NULL);
	if (Status == EFI_BUFFER_TOO_SMALL) {
		Status = SysTable->BootServices->AllocatePool(EfiLoaderData, InfoSize, (VOID**)&FileInfo);
		Status = KernelFile->GetInfo(KernelFile, &gEfiFileInfoGuid, &InfoSize, FileInfo);
		FileSize = FileInfo->FileSize;
	}
	if (EFI_ERROR(Status)) return Status;

	VOID* RawBuffer = NULL;
	Status = SysTable->BootServices->AllocatePool(EfiLoaderData, FileSize, (VOID**)&RawBuffer);
	if (EFI_ERROR(Status)) return Status;

	UINTN BytesToRead = FileSize;
	Status = KernelFile->Read(KernelFile, &BytesToRead, RawBuffer);
	if (EFI_ERROR(Status) || BytesToRead != FileSize) return EFI_LOAD_ERROR;

	PE_LOAD_RESULT PeLoadResult = { 0 };
	Status = PeLoadImageToMemory(RawBuffer, FileSize, (VOID*)KernelMemoryAddress, &PeLoadResult);
	if (EFI_ERROR(Status)) return Status;

	SysTable->BootServices->FreePool(RawBuffer);
	if (FileInfo) SysTable->BootServices->FreePool(FileInfo);

	UINT64 ActualEntryPoint = PeLoadResult.EntryPoint;
	if (KernelEntryPoint) {
		*KernelEntryPoint = ActualEntryPoint;
	}

	PLDR_DATA_TABLE_ENTRY KernelEntry;
	Status = SysTable->BootServices->AllocatePool(EfiLoaderData, sizeof(LDR_DATA_TABLE_ENTRY), (VOID**)&KernelEntry);
	if (!EFI_ERROR(Status)) {
		SysTable->BootServices->SetMem(KernelEntry, sizeof(LDR_DATA_TABLE_ENTRY), 0);

		KernelEntry->DllBase = (PVOID)KernelMemoryAddress;
		KernelEntry->SizeOfImage = PeLoadResult.SizeOfImage;

		KernelEntry->BaseDllName.Buffer = (PWSTR)Path; // Sementara pake path uefi
		KernelEntry->BaseDllName.Length = (USHORT)(StrLen(Path) * sizeof(CHAR16));
		KernelEntry->BaseDllName.MaximumLength = KernelEntry->BaseDllName.Length;

		InsertTailList(
			(LIST_ENTRY*)&LoaderBlock->LoadOrderModuleList,
			(LIST_ENTRY*)&KernelEntry->InLoadOrderLinks
		);
	}
	
	LoaderBlock->KernelBase = KernelMemoryAddress;
	LoaderBlock->KernelSize = PeLoadResult.SizeOfImage;
	LoaderBlock->KernelStackBase = MemoryRSPBaseAddress;

	return EFI_SUCCESS;
}