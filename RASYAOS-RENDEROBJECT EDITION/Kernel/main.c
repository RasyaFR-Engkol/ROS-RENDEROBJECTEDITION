#include <stdint.h>
#include <pestruct.h>
#include <osbase.h>
#include <OSType.h>
#include <intrin.h>
#include <hal.h>
#include <rki.h>

#define MAX_CPU 8

// global variable
ULONGLONG BootCycles, BootCyclesEnd = 0;
ULONG ProccesCount = 0;
ULONG RKXKernelCurrentProcessor = 0;
RPLOADER_PARAMETER_BLOCK GlobalLoaderBlock = NULL;
KPCR RKIInitialPCR;
PKPRCB RKXProcessorBlock[MAX_CPU] = {0};

// global function
VOID (*DEBUGPRINT)(CONST CHAR* Fmt, ...) = NULL;

DECLSPEC_NORETURN
CODE_SEG("INIT")
VOID
ROSAPI
RKXKernelInitializeAndSetupCPU(
	RPLOADER_PARAMETER_BLOCK LoaderParameterBlock
)
{
	CCHAR Cpu;
	PKTHREAD InitialThread;
	ULONG64 InitialStackTop;
	PKIPCR Pcr;

	// ACC kita udah kash tau bahwa kita bakal
	// pake serial buat debugging, jadi langsung inisialisasi aja
	RKXKernelInitializeSerial(LoaderParameterBlock);
	DEBUGPRINT = RHalSerialPrintf;

	DEBUGPRINT("WELCOME TO RKXKERNELINITIALIZEANDSETUPCPU.\n");

	BootCycles = __rdtsc();

	Cpu = (CCHAR)RKXKernelCurrentProcessor++;

	if (Cpu == 0)
	{
		/*
		 * Save Loader Block to global variable so it can
		 * be accesed again later.
		 */
		GlobalLoaderBlock = LoaderParameterBlock;

		// setelah init GDT dan IDT, oke kita inisialisasi PCR & PRCB
		RKIInitializeP0BootStructures(LoaderParameterBlock);
	}

	Pcr = CONTAINING_RECORD(&RKIInitialPCR.PrcbData,
		KPCR,
		PrcbData
	);

	RKXProcessorBlock[Cpu] = &Pcr->Prcb;

	InitialThread = (PKTHREAD)(LoaderParameterBlock->Thread);

	InitialThread->ApcState.Process = (PVOID)LoaderParameterBlock->Process;
	DEBUGPRINT("INITIALTHREAD READY. AT MEMORY %p.\n", InitialThread);

	RKXInitializeFeature();

	if (Cpu == 0)
	{
		RKIInitializeException();
	}
}

