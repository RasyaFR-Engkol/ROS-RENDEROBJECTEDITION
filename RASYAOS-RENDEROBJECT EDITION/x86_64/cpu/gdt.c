#include <OSType.h>
#include <osbase.h>
#include <hal.h>

VOID
ROSAPI
RKISetupGdtMemory(
    IN PKGDTENTRY64 Entry,   // Pointer ke slot GDT
    IN UINT64 Base,          // Base Address (Biasanya 0 di x64 flat model)
    IN UINT64 Limit,         // Limit (Biasanya 0 di x64 Code/Data)
    IN UINT8 AccessByte,     // Flags1 (Type, S, DPL, P) -> cth: 0x9A
    IN UINT8 Flags           // Flags2 (AVL, L, D/B, G)  -> cth: 0x20
)
{
    // 1. Set Limit Low (Bit 0-15)
    Entry->LimitLow = (UINT16)(Limit & 0xFFFF);

    // 2. Set Base Low (Bit 0-15)
    Entry->BaseLow = (UINT16)(Base & 0xFFFF);

    // 3. Set Base Middle (Bit 16-23)
    Entry->Bytes.BaseMiddle = (UINT8)((Base >> 16) & 0xFF);

    // 4. Set Access Byte (Flags1)
    // Isinya: Present, DPL, System/CodeData bit, Type
    Entry->Bytes.Flags1 = AccessByte;

    // 5. Set Granularity/Flags (Flags2) & Limit High (Bit 16-19)
    // Flags yang kamu pass (misal 0x20) ada di High Nibble (4 bit atas).
    // Limit High ada di Low Nibble (4 bit bawah).
    Entry->Bytes.Flags2 = (Flags & 0xF0) | ((UINT8)(Limit >> 16) & 0x0F);

    // 6. Set Base High (Bit 24-31)
    Entry->Bytes.BaseHigh = (UINT8)((Base >> 24) & 0xFF);
}

VOID
ROSAPI
RKISetupTssDescriptor(
    IN PKGDTENTRY64 Entry,
    IN PKTSS64 TssBase
)
{
    PKGDTENTRY64_LONG SystemEntry = (PKGDTENTRY64_LONG)Entry;
    UINT64 Base = (UINT64)(uintptr_t)TssBase;
    UINT64 Limit = sizeof(KTSS64) - 1;

    // Pakai logika yang sama buat bagian bawah (8 byte pertama)
    // Type 0x9 = 64-bit TSS (Available)
    // Access: Present(1), DPL(0), S(0-System), Type(9) -> 0x89
    RKISetupGdtMemory(Entry, Base, Limit, 0x89, 0x00);

    // TAPI, kita timpa bagian atasnya (karena TSS itu System Segment 16-byte)
    SystemEntry->BaseUpper = (UINT32)(Base >> 32);
    SystemEntry->Reserved = 0;
}