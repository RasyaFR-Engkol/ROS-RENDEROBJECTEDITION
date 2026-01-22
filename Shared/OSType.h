#pragma once

// nttypes.h

// Basic integer types (keep layout compatible with Windows/ReactOS style)
typedef signed char     INT8;
typedef unsigned char   UINT8;
typedef signed short    INT16;
typedef unsigned short  UINT16;
typedef signed int      INT32;
typedef unsigned int    UINT32;
typedef signed __int64  INT64;
typedef unsigned __int64 UINT64;

typedef long long           LONG64;
typedef unsigned long long  ULONG64;

// Common pointer-sized types
typedef void*           PVOID;
typedef void const*     PCVOID;
typedef char*           PCHAR;
typedef const char*     PCCHAR;
typedef unsigned char*  PUCHAR;
typedef const unsigned char* PCUCHAR;

typedef short*          PSHORT;
typedef unsigned short* PUSHORT;
typedef int*            PINT;
typedef unsigned int*   PUINT;
typedef long*           PLONG;
typedef unsigned long*  PULONG;
typedef __int64*        PLONGLONG;
typedef unsigned __int64* PULONGLONG;

// Pointer & Handle
#ifdef VOID
#undef VOID
#endif
typedef void            VOID;
typedef VOID*           LPVOID;
typedef PVOID           HANDLE;
typedef HANDLE*         PHANDLE;
typedef HANDLE          HMODULE;
typedef HANDLE          HINSTANCE;

// Integer 8-bit
typedef char            CCHAR;
typedef char            CHAR;
typedef unsigned char   UCHAR;
typedef UCHAR           BYTE;
typedef CHAR*           PSTR;
typedef const CHAR*     PCSTR;

// Integer 16-bit
typedef short           CSHORT;
typedef short           SHORT;
typedef unsigned short  USHORT;
typedef USHORT          WORD;
typedef USHORT          WCHAR;    // Buat Unicode (UTF-16)
typedef WCHAR* PWSTR;
typedef const WCHAR*    PCWSTR;
typedef PWSTR           LPWSTR;
typedef PCWSTR          LPCWSTR;

// Integer 32-bit (Hati-hati: Di Windows x64, LONG tetap 32-bit!)
typedef long            CLONG;
typedef int             LONG;
typedef unsigned int    ULONG;
typedef ULONG           DWORD;
typedef int             ROSSTATUS; // Return code utama NT
typedef ROSSTATUS       NTSTATUS;
typedef NTSTATUS*       PNTSTATUS;
typedef LONG            HRESULT;

// Integer 64-bit
typedef __int64          LONGLONG;
typedef unsigned __int64 ULONGLONG;
typedef ULONGLONG       DWORD64;

// Alamat Memori (Otomatis menyesuaikan 32/64 bit)
typedef unsigned __int64 uintptr_t; // keep local definition (avoid stdint mismatch)
typedef uintptr_t       ULONG_PTR;  // NT Style buat alamat
typedef ULONG_PTR       SIZE_T;
typedef long            LONG_PTR;
typedef unsigned long   DWORD_PTR;
typedef unsigned long   UINT_PTR;

// Boolean
typedef unsigned char   BOOLEAN;
#ifndef TRUE
#define TRUE  1
#endif
#ifndef FALSE
#define FALSE 0
#endif

typedef BOOLEAN*        PBOOLEAN;

// Win32-style BOOL
typedef int             BOOL;
typedef BOOL*           PBOOL;

// Struktur Doubly Linked List

// Struktur String yang dipakai NT
typedef struct _UNICODE_STRING {
    USHORT Length;
    USHORT MaximumLength;
    PWSTR  Buffer;
} UNICODE_STRING, * PUNICODE_STRING;

typedef struct _ANSI_STRING {
    USHORT Length;
    USHORT MaximumLength;
    PCHAR  Buffer;
} ANSI_STRING, *PANSI_STRING;

typedef ANSI_STRING STRING;
typedef PANSI_STRING PSTRING;

// Struktur Alamat Fisik
typedef union _LARGE_INTEGER {
    struct {
        ULONG LowPart;
        LONG HighPart;
    } DUMMYSTRUCTNAME;
    LONGLONG QuadPart;
} LARGE_INTEGER, * PLARGE_INTEGER;

typedef union _ULARGE_INTEGER {
    struct {
        ULONG LowPart;
        ULONG HighPart;
    } DUMMYSTRUCTNAME;
    ULONGLONG QuadPart;
} ULARGE_INTEGER, *PULARGE_INTEGER;

#ifdef PHYSICAL_ADDRESS
#undef PHYSICAL_ADDRESS
typedef LARGE_INTEGER PHYSICAL_ADDRESS, * PPHYSICAL_ADDRESS;
#endif

// Calling convention / linkage helpers
#ifndef NTAPI
#  define NTAPI __stdcall
#endif

#ifndef WINAPI
#  define WINAPI __stdcall
#endif

#ifndef STDCALL
#  define STDCALL __stdcall
#endif

#ifndef CDECL
#  define CDECL __cdecl
#endif

#ifndef DECLSPEC_NORETURN
#  if defined(_MSC_VER)
#    define DECLSPEC_NORETURN __declspec(noreturn)
#  else
#    define DECLSPEC_NORETURN __attribute__((noreturn))
#  endif
#endif

#ifndef DECLSPEC_ALIGN
#  if defined(_MSC_VER)
#    define DECLSPEC_ALIGN(x) __declspec(align(x))
#  else
#    define DECLSPEC_ALIGN(x) __attribute__((aligned(x)))
#  endif
#endif

#ifndef UNREFERENCED_PARAMETER
#  define UNREFERENCED_PARAMETER(P) (void)(P)
#endif

// Supaya kode kamu portable dan MSVC gak ngambek
#if defined(_MSC_VER)
#define FORCEINLINE __forceinline
#else
#define FORCEINLINE inline __attribute__((always_inline))
#endif

// Struktur dasar Doubly Linked List
#if defined(__BASE_H__)
typedef LIST_ENTRY* PLIST_ENTRY;
// Base.h already defines LIST_ENTRY with ForwardLink/BackLink
#ifndef Flink
#define Flink ForwardLink
#endif
#ifndef Blink
#define Blink BackLink
#endif
#else
typedef struct _LIST_ENTRY {
    struct _LIST_ENTRY* Flink; // Forward Link (Nunjuk ke depan)
    struct _LIST_ENTRY* Blink; // Backward Link (Nunjuk ke belakang)
} LIST_ENTRY, * PLIST_ENTRY;
#endif

#define ROSAPI __stdcall

// Common constants
#ifndef NULL
#  define NULL ((void*)0)
#endif

#ifndef STATUS_SUCCESS
#  define STATUS_SUCCESS                   ((NTSTATUS)0x00000000L)
#  define STATUS_UNSUCCESSFUL              ((NTSTATUS)0xC0000001L)
#  define STATUS_NOT_IMPLEMENTED           ((NTSTATUS)0xC0000002L)
#  define STATUS_INVALID_INFO_CLASS        ((NTSTATUS)0xC0000003L)
#  define STATUS_INFO_LENGTH_MISMATCH      ((NTSTATUS)0xC0000004L)
#  define STATUS_ACCESS_VIOLATION          ((NTSTATUS)0xC0000005L)
#  define STATUS_IN_PAGE_ERROR             ((NTSTATUS)0xC0000006L)
#  define STATUS_INVALID_HANDLE            ((NTSTATUS)0xC0000008L)
#  define STATUS_INVALID_PARAMETER         ((NTSTATUS)0xC000000DL)
#  define STATUS_NO_MEMORY                 ((NTSTATUS)0xC0000017L)
#  define STATUS_BUFFER_TOO_SMALL          ((NTSTATUS)0xC0000023L)
#  define STATUS_END_OF_FILE               ((NTSTATUS)0xC0000011L)
#  define STATUS_PENDING                   ((NTSTATUS)0x00000103L)
#endif

#ifndef NT_SUCCESS
#  define NT_SUCCESS(Status) (((NTSTATUS)(Status)) >= 0)
#endif

#define IN
#define OUT
#define OPTIONAL

#ifndef CONST
#define CONST const
#endif

//
// Object Attributes Structure
//
typedef struct _OBJECT_ATTRIBUTES {
    ULONG Length;
    HANDLE RootDirectory;
    PUNICODE_STRING ObjectName;
    ULONG Attributes;
    PVOID SecurityDescriptor;        // PSECURITY_DESCRIPTOR
    PVOID SecurityQualityOfService;  // PSECURITY_QUALITY_OF_SERVICE
} OBJECT_ATTRIBUTES, * POBJECT_ATTRIBUTES;

typedef CONST OBJECT_ATTRIBUTES* PCOBJECT_ATTRIBUTES;

// Object Attribute Flags
#define OBJ_INHERIT             0x00000002L
#define OBJ_PERMANENT           0x00000010L
#define OBJ_EXCLUSIVE           0x00000020L
#define OBJ_CASE_INSENSITIVE    0x00000040L
#define OBJ_OPENIF              0x00000080L
#define OBJ_OPENLINK            0x00000100L
#define OBJ_KERNEL_HANDLE       0x00000200L
#define OBJ_FORCE_ACCESS_CHECK  0x00000400L
#define OBJ_VALID_ATTRIBUTES    0x000007F2L

// Helpher Macro to Initialize Object Attributes
#define InitializeObjectAttributes( p, n, a, r, s ) { \
    (p)->Length = sizeof( OBJECT_ATTRIBUTES );          \
    (p)->RootDirectory = r;                             \
    (p)->Attributes = a;                                \
    (p)->ObjectName = n;                                \
    (p)->SecurityDescriptor = s;                        \
    (p)->SecurityQualityOfService = NULL;               \
    }

//
// Access Mask
//
typedef ULONG ACCESS_MASK;
typedef ACCESS_MASK* PACCESS_MASK;

#define DELETE                           (0x00010000L)
#define READ_CONTROL                     (0x00020000L)
#define WRITE_DAC                        (0x00040000L)
#define WRITE_OWNER                      (0x00080000L)
#define SYNCHRONIZE                      (0x00100000L)

#define STANDARD_RIGHTS_REQUIRED         (0x000F0000L)
#define STANDARD_RIGHTS_READ             (READ_CONTROL)
#define STANDARD_RIGHTS_WRITE            (READ_CONTROL)
#define STANDARD_RIGHTS_EXECUTE          (READ_CONTROL)
#define STANDARD_RIGHTS_ALL              (0x001F0000L)
#define SPECIFIC_RIGHTS_ALL              (0x0000FFFFL)

#define GENERIC_READ                     (0x80000000L)
#define GENERIC_WRITE                    (0x40000000L)
#define GENERIC_EXECUTE                  (0x20000000L)
#define GENERIC_ALL                      (0x10000000L)

//
// Client ID
//
typedef struct _CLIENT_ID {
    HANDLE UniqueProcess;
    HANDLE UniqueThread;
} CLIENT_ID, * PCLIENT_ID;

//
// Processor Modes
//
typedef CCHAR KPROCESSOR_MODE;

typedef enum _MODE {
    KernelMode,
    UserMode,
    MaximumMode
} MODE;

//
// IRQL (Interrupt Request Level)
//
typedef UCHAR KIRQL;
typedef KIRQL* PKIRQL;

#define PASSIVE_LEVEL 0
#define APC_LEVEL 1
#define DISPATCH_LEVEL 2
#define HIGH_LEVEL 31

//
// Spinlock
//
typedef ULONG_PTR KSPIN_LOCK;
typedef KSPIN_LOCK* PKSPIN_LOCK;

//
// Pool Types
//
typedef enum _POOL_TYPE {
    NonPagedPool,
    NonPagedPoolExecute = NonPagedPool,
    PagedPool,
    NonPagedPoolMustSucceed,
    DontUseThisType,
    NonPagedPoolCacheAligned,
    PagedPoolCacheAligned,
    NonPagedPoolCacheAlignedMustS,
    MaxPoolType,
    NonPagedPoolBase = 0,
    NonPagedPoolBaseMustSucceed = NonPagedPoolBase + 2,
    NonPagedPoolBaseCacheAligned = NonPagedPoolBase + 4,
    NonPagedPoolBaseCacheAlignedMustS = NonPagedPoolBase + 6,
    NonPagedPoolSession = 32,
    PagedPoolSession = NonPagedPoolSession + 1,
    NonPagedPoolMustSucceedSession = PagedPoolSession + 1,
    DontUseThisTypeSession = NonPagedPoolMustSucceedSession + 1,
    NonPagedPoolCacheAlignedSession = DontUseThisTypeSession + 1,
    PagedPoolCacheAlignedSession = NonPagedPoolCacheAlignedSession + 1,
    NonPagedPoolCacheAlignedMustSSession = PagedPoolCacheAlignedSession + 1,
    NonPagedPoolNx = 512,
    NonPagedPoolNxCacheAligned = NonPagedPoolNx + 4,
    NonPagedPoolSessionNx = NonPagedPoolNx + 32,
} POOL_TYPE;

//
// Priority
//
typedef LONG KPRIORITY;

//
// Opaque Handles (Forward Declarations)
//
typedef struct _EPROCESS *PEPROCESS;
typedef struct _ETHREAD *PETHREAD;
typedef struct _KPROCESS *PKPROCESS;
typedef struct _KTHREAD *PKTHREAD;
typedef struct _DRIVER_OBJECT *PDRIVER_OBJECT;
typedef struct _DEVICE_OBJECT *PDEVICE_OBJECT;
typedef struct _MDL *PMDL;

//
// MDL (Memory Descriptor List)
//
typedef struct _MDL {
    struct _MDL* Next;
    CSHORT Size;
    CSHORT MdlFlags;
    struct _EPROCESS* Process;
    PVOID MappedSystemVa;
    PVOID StartVa;
    ULONG ByteCount;
    ULONG ByteOffset;
} MDL;

//
// Context & Exception Record (Simplified for now)
//
typedef struct _EXCEPTION_RECORD {
    NTSTATUS ExceptionCode;
    ULONG ExceptionFlags;
    struct _EXCEPTION_RECORD* ExceptionRecord;
    PVOID ExceptionAddress;
    ULONG NumberParameters;
    ULONG_PTR ExceptionInformation[15];
} EXCEPTION_RECORD, * PEXCEPTION_RECORD;

typedef ULONG_PTR KAFFINITY;
typedef KAFFINITY* PKAFFINITY;

//
// NT_TIB (Thread Information Block)
//
typedef struct _NT_TIB {
    struct _EXCEPTION_REGISTRATION_RECORD* ExceptionList;
    PVOID StackBase;
    PVOID StackLimit;
    PVOID SubSystemTib;
    union {
        PVOID FiberData;
        ULONG Version;
    };
    PVOID ArbitraryUserPointer;
    struct _NT_TIB* Self;
} NT_TIB;
typedef NT_TIB* PNT_TIB;

//
// Dispatcher Header (used by KTHREAD, KEVENT, etc.)
//
typedef struct _DISPATCHER_HEADER {
    union {
        struct {
            UCHAR Type;
            union {
                UCHAR Abandoned;
                UCHAR Absolute;
                UCHAR NpxIrql;
                BOOLEAN Signalling;
            };
            union {
                UCHAR Size;
                UCHAR Hand;
            };
            union {
                UCHAR Inserted;
                BOOLEAN DebugActive;
                BOOLEAN DpcActive;
            };
        };
        LONG Lock;
    };
    LONG SignalState;
    LIST_ENTRY WaitListHead;
} DISPATCHER_HEADER;

typedef struct _KAPC_STATE {
    LIST_ENTRY ApcListHead[2];
    struct _KPROCESS* Process;
    BOOLEAN KernelApcInProgress;
    BOOLEAN KernelApcPending;
    BOOLEAN UserApcPending;
} KAPC_STATE, *PKAPC_STATE, *PRKAPC_STATE;

//
// KTHREAD (Kernel Thread)
//
typedef struct _KTHREAD {
    DISPATCHER_HEADER Header;
    LIST_ENTRY MutationListHead;
    PVOID InitialStack;
    PVOID StackLimit;
    PVOID KernelStack;          // Current stack pointer (RSP)
    KSPIN_LOCK ThreadLock;
    union {
        KAPC_STATE ApcState;
        UCHAR ApcStateFill[23]; // Padding placeholder
    };
    BOOLEAN ApcQueueable;
    BOOLEAN NextProcessor;
    BOOLEAN DeferredProcessor;
    UCHAR AdjustReason;
    CHAR AdjustIncrement;
    KPRIORITY Priority;
    volatile UINT32 ContextSwitches;
    volatile UCHAR State;       // Thread state (Ready, Running, Waiting...)
    CHAR NpxState;
    KIRQL WaitIrql;
    CHAR WaitMode;
    PVOID Teb;                  // Thread Environment Block
} KTHREAD, * PKTHREAD, * PRKTHREAD;

//
// KPROCESS (Kernel Process)
//
typedef struct _KPROCESS {
    DISPATCHER_HEADER Header;
    LIST_ENTRY ProfileListHead;
    ULONG_PTR DirectoryTableBase;
    LIST_ENTRY ThreadListHead;
    KSPIN_LOCK ProcessLock; // defined in OSType.h
    KAFFINITY Affinity;

    // Time Accounting
    ULONGLONG KernelTime;
    ULONGLONG UserTime;
    
    LIST_ENTRY ReadyListHead;
    LIST_ENTRY SwapListEntry;

    // Thread & stack info
    volatile KAFFINITY ActiveProcessors;
    LONG AutoAlignment;
    LONG DisableBoost;
    LONG DisableQuantum;

    // Resident kernel stack count
    LONG StackCount;
    KSPIN_LOCK StackCountLock;

    UCHAR State;
    CHAR BasePriority;
    UCHAR ThreadQuantum;
    UCHAR PowerState;

} KPROCESS, *PKPROCESS, *PRKPROCESS;

//
// Helpers for EPROCESS
//
typedef struct _HANDLE_TABLE* PHANDLE_TABLE;

typedef struct _EX_PUSH_LOCK {
    ULONG_PTR Value;
} EX_PUSH_LOCK, *PEX_PUSH_LOCK;

typedef struct _EX_RUNDOWN_REF {
    ULONG_PTR Value;
} EX_RUNDOWN_REF, *PEX_RUNDOWN_REF;

//
// EPROCESS (Executive Process)
//
typedef struct _EPROCESS {
    KPROCESS Pcb;

    EX_PUSH_LOCK ProcessLock;
    LARGE_INTEGER CreateTime;
    LARGE_INTEGER ExitTime;
    EX_RUNDOWN_REF RundownProtect;
    HANDLE UniqueProcessId;
    LIST_ENTRY ActiveProcessLinks;

    // Quota usage
    SIZE_T QuotaUsage[3];   // PagedPool, NonPagedPool, PageFile
    SIZE_T QuotaPeak[3];
    SIZE_T CommitCharge;
    SIZE_T PeakCommitCharge;
    SIZE_T VirtualSize;
    SIZE_T PeakVirtualSize;

    LIST_ENTRY SessionProcessLinks;

    PVOID DebugPort;
    PVOID ExceptionPort;
    PHANDLE_TABLE ObjectTable;

    // Security
    PVOID Token; // (EX_FAST_REF)

    // MM / Cache
    PVOID ImageFilePointer; // PFILE_OBJECT
    UCHAR ImageFileName[16];

    PVOID VadRoot;
    PVOID VadHint;
    PVOID CloneRoot;
    
    // Thread enumeration
    LIST_ENTRY ThreadListHead;

    PVOID SectionObject;
    PVOID SectionBaseAddress;

    PEPROCESS ParentProcess;
    ULONG_PTR ExitStatus;

    // Flags (Simplified)
    union {
        ULONG Flags;
        struct {
            ULONG CreateReported : 1;
            ULONG NoDebugInherit : 1;
            ULONG ProcessExiting : 1;
            ULONG ProcessDelete : 1;
            ULONG Wow64SplitPages : 1;
            ULONG VmDeleted : 1;
            ULONG OutswapEnabled : 1;
            ULONG Outswapped : 1;
            ULONG ForkFailed : 1;
            ULONG Wow64VaSpace4Gb : 1;
            ULONG AddressSpaceInitialized : 2;
            ULONG SetTimerResolution : 1;
            ULONG BreakOnTermination : 1;
            ULONG DeprioritizeViews : 1;
            ULONG WriteWatch : 1;
            ULONG ProcessInSession : 1;
            ULONG OverrideAddressSpace : 1;
            ULONG HasAddressSpace : 1;
            ULONG LaunchPrefetched : 1;
            ULONG Background : 1;
            ULONG VmTopDown : 1;
            ULONG ImageNotifyDone : 1;
            ULONG PdeUpdateNeeded : 1;
            ULONG VdmAllowed : 1;
            ULONG CrossSessionCreate : 1;
            ULONG ProcessInserted : 1;
            ULONG DefaultIoPriority : 3;
            ULONG ProcessSelfDelete : 1;
            ULONG SetTimerResolutionLink : 1;
        };
    };

    ULONG ActiveThreads;
    
} EPROCESS, *PEPROCESS;

// ---------------------------------------------------------
// Struktur x64 Hardware Standard
// ---------------------------------------------------------

#pragma pack(push, 1)

typedef struct _KGDTENTRY64 {
    USHORT LimitLow;
    USHORT BaseLow;
    union {
        struct {
            UCHAR BaseMiddle;
            UCHAR Flags1;     // P, DPL, S, Type
            UCHAR Flags2;     // G, D/B, L, AVL, LimitHigh
            UCHAR BaseHigh;
        } Bytes;
        struct {
            ULONG BaseMiddle : 8;
            ULONG Type : 5;
            ULONG Dpl : 2;
            ULONG Present : 1;
            ULONG LimitHigh : 4;
            ULONG System : 1;
            ULONG LongMode : 1;
            ULONG DefaultBig : 1;
            ULONG Granularity : 1;
            ULONG BaseHigh : 8;
        } Bits;
    };
} KGDTENTRY64, *PKGDTENTRY64;

typedef struct _KGDTENTRY64_LONG {
    KGDTENTRY64 Low;
    ULONG BaseUpper;
    ULONG Reserved;
} KGDTENTRY64_LONG, *PKGDTENTRY64_LONG;

typedef struct _KTSS64 {
    ULONG Reserved0;
    UINT64 Rsp0;      // Stack kernel saat interrupt (PENTING!)
    UINT64 Rsp1;
    UINT64 Rsp2;
    UINT64 Ist[8];    // Interrupt Stack Table
    UINT64 Reserved1;
    USHORT Reserved2;
    USHORT IoMapBase;
} KTSS64, *PKTSS64;

typedef struct _KIDTENTRY64 {
    USHORT OffsetLow;
    USHORT Selector;
    USHORT IstIndex : 3;
    USHORT Reserved0 : 5;
    USHORT Type : 5;
    USHORT Dpl : 2;
    USHORT Present : 1;
    USHORT OffsetMiddle;
    ULONG OffsetHigh;
    ULONG Reserved1;
} KIDTENTRY64, *PKIDTENTRY64;

#pragma pack(pop)

// ---------------------------------------------------------
// NT Style Processor Control Region (Simplified)
// ---------------------------------------------------------

typedef struct _KPRCB {
	UINT64 Number; // Processor Number
	KAFFINITY SetMember; // CPU Mask
	ULONG64 CurrentPrcbLoad; // Load saat ini
	ULONG64 ExpectedPrcbLoad; // Load yang diharapkan
	ULONG64 PrcbPad0[3]; // Padding biar align 16 byte
    // Info Thread yang sedang jalan
    struct _KTHREAD* CurrentThread;
    struct _KTHREAD* NextThread;
    struct _KTHREAD* IdleThread;
    
    // GDT, IDT, TSS biasanya ditempel di sini biar locality bagus
    KGDTENTRY64 Gdt[16]; // Cukup buat Null, KernelCode, KernelData, UserCode, UserData, TSS
    KTSS64      Tss;
    // IDT butuh 256 entry, kadang ditaruh terpisah tapi dipoint dari sini
    struct _KIDTENTRY64* IdtBase; 
} KPRCB, *PKPRCB;

//
// KPCR (Kernel Processor Control Region)
//
typedef struct _KIPCR {

union {
    NT_TIB NtTib;           // Thread Information Block for the processor ?? (Normally start of fs/gs)
    struct {
        struct _KEXCEPTION_REGISTRATION_RECORD* Used_ExceptionList;
        PVOID Used_StackBase;
        PVOID Used_StackLimit;
        PVOID SubSystemTib;
        union {
            PVOID FiberData;
            ULONG Version;
        };
        PVOID ArbitraryUserPointer;
        struct _KIPCR* Self; // Linear address of this PCR
    };
    struct {
            UINT64 GdtBase; // GS:[0x00] ? (Tergantung implementasi ROS kamu)
            UINT64 TssBase;
            UINT64 UserRsp; // GS:[0x10] -> Disimpan pas SYSCALL
            struct _KIPCR* SelfUnused; // Pointer ke diri sendiri
            struct _KPRCB* PrcbUnused;
    };
};
    struct _KIPCR* SelfPcr;       // Pointer to this PCR
    struct _KPRCB* Prcb;          // Pointer to Processor Control Block (Data per core)
    KIRQL Irql;                   // Current IRQL
    ULONG IRR;                    // Interrupt Request Register (simulation)
    ULONG IrrActive;              // Active IRRs
    ULONG IDR;                    // Interrupt Delivery Register
    PVOID IdtBase;                // IDT Address
    PVOID PcrGdtBase;                // GDT Address
    PVOID PcrTssBase;                // TSS Address
    USHORT MajorVersion;
    USHORT MinorVersion;
    KAFFINITY SetMember;          // CPU mask
    ULONG StallScaleFactor;
    UCHAR DebugActive;
    
    KPRCB PrcbData; // Data PRCB ditempel langsung
} KIPCR, * PKIPCR;

typedef KIPCR KPCR;
typedef PKIPCR PKPCR;

#pragma pack(push, 4)
typedef struct _KFLOATING_SAVE {
    ULONG   ControlWord;
    ULONG   StatusWord;
    ULONG   ErrorOffset;
    ULONG   ErrorSelector;
    ULONG   DataOffset;
    ULONG   DataSelector;
    ULONG   Cr0NpxState;
    ULONG   Spare1;
} KFLOATING_SAVE, *PKFLOATING_SAVE;
#pragma pack(pop)

//
// End of definitions
//
