#include <rki.h>
#include <OSType.h>
#include <rtl.h>
#include <intrin.h>
#include <hal.h>

extern KPCR RKIInitialPCR;
extern VOID RKILoadTr(UINT16 TrSelector);
extern VOID RKIReloadSegments(UINT16 CodeSelector, UINT16 DataSelector);

// ===============================================
// DEFINE
// ===============================================

#define MSR_GS_BASE     0xC0000101
#define KGDT64_NULL     0x00
#define KGDT64_R0_CODE  0x10 // index 2
#define KGDT64_R0_DATA  0x18 // index 3
#define KGDT64_R3_CODE  0x28 // index 5
#define KGDT64_R3_DATA  0x20 // index 4
#define KGDT64_TSS      0x30 // index 6

#define CR0_MP              (1 << 1)   // Monitor Coprocessor
#define CR0_EM              (1 << 2)   // Emulation (HARUS 0 buat x64)
#define CR4_OSFXSR          (1 << 9)   // OS supports FXSAVE/FXRSTOR
#define CR4_OSXMMEXCPT      (1 << 10)  // OS supports Unmasked SIMD Exceptions
#define CR4_UMIP            (1 << 11)  // User-Mode Instruction Prevention (Security)
#define CR4_FSGSBASE        (1 << 16)  // Enable RDFSBASE/RDGSBASE instructions

#define EFER_NXE            (1 << 11)  // No-Execute Enable
#define EFER_SCE            (1 << 0)   // System Call Extensions (SYSCALL/SYSRET)

#define MSR_EFER            0xC0000080

VOID
ROSAPI
RKISetupTssDescriptor(
	IN PKGDTENTRY64 Entry,
	IN PKTSS64 TssBase
);

VOID
ROSAPI
RKISetupGdtMemory(
	IN PKGDTENTRY64 Entry,   // Pointer ke slot GDT
	IN UINT64 Base,          // Base Address (Biasanya 0 di x64 flat model)
	IN UINT64 Limit,         // Limit (Biasanya 0 di x64 Code/Data)
	IN UINT8 AccessByte,     // Flags1 (Type, S, DPL, P) -> cth: 0x9A
	IN UINT8 Flags           // Flags2 (AVL, L, D/B, G)  -> cth: 0x20
);

VOID
ROSAPI
RKIInitializeP0BootStructures(
	RPLOADER_PARAMETER_BLOCK LoaderParameterBlock
)
{
	PKPCR Pcr = &RKIInitialPCR;
	PKPRCB Prcb = &Pcr->PrcbData;

	// 1. Setup Basic Pointers
	Pcr->Self = Pcr;
	Pcr->Prcb = Prcb;

	// 2. Setup GDT
	RtlZeroMemory(&Prcb->Gdt[0], sizeof(Prcb->Gdt));

	// index 0: null descriptor (already zeroed)
	// index 2: Kernel Code (selector 0x10)
	RKISetupGdtMemory(&Prcb->Gdt[2], 0, 0, 0x9A, 0x20);
	// index 3: Kernel Data (selector 0x18)
	RKISetupGdtMemory(&Prcb->Gdt[3], 0, 0, 0x92, 0x00);

	// Entry 4: User Data (Ring 3) selector 0x20
	// Access Byte: 0xF2 -> Present(1), DPL(3), S(1), Type(Writeable Data)
	// Flags2: 0x00 (no Long Mode bit needed for data)
	RKISetupGdtMemory(&Prcb->Gdt[4], 0, 0, 0xF2, 0x00);

	// Entry 5: User Code (Ring 3) selector 0x28
	// Access Byte: 0xFA -> Present(1), DPL(3), S(1), Type(Readable Code)
	// Flags2: 0x20 -> Long Mode (L=1), Default(D=0)
	RKISetupGdtMemory(&Prcb->Gdt[5], 0, 0, 0xFA, 0x20);

	Prcb->Tss.Rsp0 = LoaderParameterBlock->KernelStackBase;
	Prcb->Tss.IoMapBase = sizeof(KTSS64);

	RKISetupTssDescriptor(&Prcb->Gdt[6], &Prcb->Tss);

#pragma pack(push, 1)
	struct {
		UINT16 Limit;
		UINT64 Base;
	} GdtPtr;
#pragma pack(pop)

	GdtPtr.Limit = ((sizeof(KGDTENTRY64) * 16) - 1);
	GdtPtr.Base = (UINT64)(uintptr_t)&Prcb->Gdt[0];

	_lgdt(&GdtPtr);
	
	__segmentlimit(KGDT64_TSS); // Dummy read biar aman
	// Load TSS
	RKILoadTr(KGDT64_TSS);
	RKIReloadSegments(KGDT64_R0_CODE, KGDT64_R0_DATA);

	// Set GS Base to PCR
	__writemsr(MSR_GS_BASE, (UINT64)(uintptr_t)Pcr);

	// Setup IDT (Interrupt Descriptor Table)
    // Biasanya IDT dialokasikan terpisah atau di dalam PRCB juga bisa
    // RKIInitializeIdt(Prcb); // Implementasi terpisah
	DEBUGPRINT("PCR and GDT initialized successfully. GDT: %p, TSS: %p\n", GdtPtr.Base, Prcb->Tss.Rsp0);
}

VOID
ROSAPI
RKXInitializeFeature
(VOID)
{ 
	INT32 CPUInfo[4];
	UINT64 Cr0, Cr4, Efer;

	Cr0 = __readcr0();
	Cr4 = __readcr4();
	// konfigurasi CR0
	Cr0 |= CR0_MP;  // Enable Monitor Coprocessor
	Cr0 &= ~CR0_EM; // Disable Emulation

	// CR4 yang kita konfigurasi adalah:
	// OSFXSR : Enable FXSAVE/FXRSTOR instructions
	// OSXMMEXCPT : Enable Unmasked SIMD Exceptions
	Cr4 |= CR4_OSFXSR | CR4_OSXMMEXCPT;

	__writecr0(Cr0);
	__writecr4(Cr4);
	DEBUGPRINT("CR0 and CR4 configured. CR0: 0x%llx, CR4: 0x%llx\n", Cr0, Cr4);
	RHalSerialPrintf("CR0 and CR4 configured. CR0: 0x%llx, CR4: 0x%llx\n", Cr0, Cr4);

	__cpuid(CPUInfo, 0x80000001);

	if (CPUInfo[3] & (1 << 20))
	{
		// bearti NXE (No-Execute Enable)
		Efer = __readmsr(MSR_EFER);
		Efer |= EFER_NXE;
		__writemsr(MSR_EFER, Efer);
		DEBUGPRINT("NXE (No-Execute Enable) diaktifkan.\n");
	}

	__cpuid(CPUInfo, 1);
	if (!(CPUInfo[2] & (1 << 13))) 
	{ 
		// RKXBugCheck(0x0000006A, UNSUPPORTED_PROCESSOR_TYPE);
	}
}

VOID
ROSAPI
RKIInitializeException(VOID)
{

}