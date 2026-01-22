#pragma once
#include <OSType.h>

#ifdef __cplusplus
extern "C" {
#endif

//
// Memory Management
//

VOID
NTAPI
RtlFillMemory(
    IN PVOID Destination,
    IN SIZE_T Length,
    IN UCHAR Fill
    );

VOID
NTAPI
RtlZeroMemory(
    IN PVOID Destination,
    IN SIZE_T Length
    );

VOID
NTAPI
RtlCopyMemory(
    IN PVOID Destination,
    IN CONST VOID* Source,
    IN SIZE_T Length
    );

VOID
NTAPI
RtlMoveMemory(
    IN PVOID Destination,
    IN CONST VOID* Source,
    IN SIZE_T Length
    );

//
// Common Macros
//
#define RtlSetMemory(Destination, Length, Fill) RtlFillMemory(Destination, Length, Fill)

#ifdef __cplusplus
}
#endif
