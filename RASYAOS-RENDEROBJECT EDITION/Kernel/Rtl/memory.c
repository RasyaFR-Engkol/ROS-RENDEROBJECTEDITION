#include <rtl.h>
#include <stddef.h> // for size_t

#pragma function(memset)
#pragma function(memcpy)
#pragma function(memmove)

extern VOID* CDECL RtlASMMemcpy(
    IN PVOID Destination,
    IN CONST VOID* Source,
    IN SIZE_T Length
);

extern VOID* CDECL RtlASMMemset(
    IN PVOID Destination,
    IN UCHAR Fill,
    IN SIZE_T Length
);

VOID
NTAPI
RtlFillMemory(
    IN PVOID Destination,
    IN SIZE_T Length,
    IN UCHAR Fill
    )
{
	RtlASMMemset(Destination, Fill, Length);
}

VOID
NTAPI
RtlZeroMemory(
    IN PVOID Destination,
    IN SIZE_T Length
    )
{
    RtlASMMemset(Destination, 0, Length);
}

VOID
NTAPI
RtlCopyMemory(
    IN PVOID Destination,
    IN CONST VOID* Source,
    IN SIZE_T Length
    )
{
	RtlASMMemcpy(Destination, Source, Length);
}

VOID
NTAPI
RtlMoveMemory(
    IN PVOID Destination,
    IN CONST VOID* Source,
    IN SIZE_T Length
    )
{
    PUCHAR D = (PUCHAR)Destination;
    PCUCHAR S = (PCUCHAR)Source;
    
    if (D < S) {
        // Source is higher, or non-overlapping (D..D+L < S)
        // Copy forward
        while (Length--) {
            *D++ = *S++;
        }
    } else if (D > S) {
        // Source is lower (overlap possible if S..S+L > D)
        // Copy backward
        D += Length;
        S += Length;
        while (Length--) {
            *--D = *--S;
        }
    }
    // else D == S, nothing to do
}

VOID
NTAPI
RtlInitUnicodeString(
    OUT PUNICODE_STRING DestinationString,
    IN PCWSTR SourceString OPTIONAL
)
{
    DestinationString->Buffer = (PWSTR)SourceString;
    if (SourceString) {
        USHORT Length = 0;
        while (SourceString[Length]) Length++; // wcslen manual
        DestinationString->Length = Length * sizeof(WCHAR);
        DestinationString->MaximumLength = DestinationString->Length + sizeof(WCHAR);
    }
    else {
        DestinationString->Length = 0;
        DestinationString->MaximumLength = 0;
    }
}

//
// CRT wrappers (Compiler often emits calls to these)
//

void* __cdecl memset(void* dest, int c, size_t count)
{
    RtlFillMemory(dest, count, (UCHAR)c);
    return dest;
}

void* __cdecl memcpy(void* dest, const void* src, size_t count)
{
    RtlCopyMemory(dest, src, count);
    return dest;
}

void* __cdecl memmove(void* dest, const void* src, size_t count)
{
    RtlMoveMemory(dest, src, count);
    return dest;
}
