#pragma once

#include <Uefi/UefiBaseType.h>

static EFI_GUID gEfiLoadedImageProtocolGuid = { 0x5B1B31A1, 0x9562, 0x11D2, { 0x8E, 0x3F, 0x00, 0xA0, 0xC9, 0x69, 0x72, 0x3B } };
static EFI_GUID gEfiSimpleFileSystemProtocolGuid = { 0x0964E5B22, 0x6459, 0x11D2, { 0x8E, 0x39, 0x00, 0xA0, 0xC9, 0x69, 0x72, 0x3B } };
static EFI_GUID gEfiFileInfoGuid = { 0x09576E92, 0x6D3F, 0x11D2, { 0x8E, 0x39, 0x00, 0xA0, 0xC9, 0x69, 0x72, 0x3B } };

typedef struct {
    UINT32            Revision;
    EFI_HANDLE        ParentHandle;
    EFI_SYSTEM_TABLE* SystemTable;
    // Source location image (Disk tempat lu berada)
    EFI_HANDLE        DeviceHandle;
    void* FilePath;     // Aslinya EFI_DEVICE_PATH_PROTOCOL*, kita void* dulu biar simpel
    void* Reserved;
    // Image load options
    UINT32            LoadOptionsSize;
    void* LoadOptions;
    // Location in memory
    void* ImageBase;    // <--- INI PENTING NANTI
    UINT64            ImageSize;
    int               ImageCodeType;
    int               ImageDataType;
    void* Unload;
} EFI_LOADED_IMAGE_PROTOCOL;

#define EFI_FILE_INFO_ID \
  { 0x09576e92, 0x6d3f, 0x11d2, { 0x8e, 0x39, 0x00, 0xa0, 0xc9, 0x69, 0x72, 0x3b } }

typedef struct {
    UINT64    Size;               // Ukuran seluruh struktur ini (termasuk nama file)
    UINT64    FileSize;           // Ukuran file SEBENARNYA dalam bytes (Ini yang lo cari!)
    UINT64    PhysicalSize;       // Ukuran file di dalam disk (termasuk slack space)
    EFI_TIME  CreateTime;         // Waktu dibuat
    EFI_TIME  LastAccessTime;     // Terakhir dibuka
    EFI_TIME  ModificationTime;   // Terakhir diedit
    UINT64    Attribute;          // Read-only, Hidden, System, Directory, dll.
    CHAR16    FileName[1];        // Nama file (Null-terminated string)
} EFI_FILE_INFO;