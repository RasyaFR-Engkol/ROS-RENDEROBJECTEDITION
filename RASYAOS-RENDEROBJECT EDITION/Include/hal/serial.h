#pragma once

#include <OSType.h>
#include <osbase.h>

VOID
ROSAPI
RKXKernelInitializeSerial(
	RPLOADER_PARAMETER_BLOCK LoaderParameterBlock
);

VOID
ROSAPI
RHalPutSerial(
	CONST CHAR* Str
);

VOID
ROSAPI
RHalSerialPrintf(
    CONST CHAR* Format,
    ...
);