#pragma once

#include <OSType.h>
#include <osbase.h>

VOID
ROSAPI
RKIInitializeP0BootStructures(
	RPLOADER_PARAMETER_BLOCK LoaderParameterBlock
);

VOID
ROSAPI
RKXInitializeFeature
(VOID);

extern VOID(*DEBUGPRINT)(CONST CHAR* Fmt, ...);