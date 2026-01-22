#include <OSType.h>
#include <osbase.h>
#include <intrin.h>
#include <stdarg.h>

VOID
ROSAPI
RHaliInitSerial(
)
{
	CONST USHORT COM_PORT_BASE = 0x3F8; // Standard COM1 port address
	__outbyte(COM_PORT_BASE + 1, 0x00);    // Disable all interrupts
	__outbyte(COM_PORT_BASE + 3, 0x80);    // Enable DLAB (set baud rate divisor)
	__outbyte(COM_PORT_BASE + 0, 0x03);    // Set divisor to 3 (lo byte) 38400 baud
	__outbyte(COM_PORT_BASE + 1, 0x00);    //                  (hi byte)
	__outbyte(COM_PORT_BASE + 3, 0x03);    // 8 bits, no parity, one stop bit
	__outbyte(COM_PORT_BASE + 2, 0xC7);    // Enable FIFO, clear them, with 14-byte threshold
	__outbyte(COM_PORT_BASE + 4, 0x0B);    // IRQs enabled, RTS/DSR set
}

VOID
ROSAPI
RKXKernelInitializeSerial(
	RPLOADER_PARAMETER_BLOCK LoaderParameterBlock
)
{
	// call internal function
	RHaliInitSerial();
}

VOID
ROSAPI
RHaliPutSerialChar(
	CHAR Ch
)
{
	CONST USHORT COM_PORT_BASE = 0x3F8; // Standard COM1 port address
	// Wait for the transmit buffer to be empty
	while ((__inbyte(COM_PORT_BASE + 5) & 0x20) == 0);
	// Send the character
	__outbyte(COM_PORT_BASE, (UINT8)Ch);
}

VOID
ROSAPI
RHalPutSerial(
	CONST CHAR* Str
)
{
	for (CONST CHAR* Ptr = Str; *Ptr != '\0'; ++Ptr) {
		RHaliPutSerialChar(*Ptr);
	}
}

static
VOID
RHaliPutSerialInt(
	UINT64 Value,
	UINT32 Base,
	INT32 Width,
	CHAR PadChar,
	BOOL Negative
)
{
	CHAR Buffer[32];
	INT32 Idx = 0;

	if (Value == 0) {
		Buffer[Idx++] = '0';
	}
	else {
		while (Value > 0) {
			UINT32 Remainder = Value % Base;
			Buffer[Idx++] = (Remainder < 10) ? ('0' + Remainder) : ('a' + Remainder - 10);
			Value /= Base;
		}
	}

	// Calculate length including sign
	INT32 Len = Idx;
	if (Negative) Len++;

	// If padding with '0' and negative, print sign first
	if (PadChar == '0' && Negative) {
		RHaliPutSerialChar('-');
		Negative = FALSE; // Sign already handled
	}

	// Print padding
	while (Width > Len) {
		RHaliPutSerialChar(PadChar);
		Width--;
	}

	// If not handled above (space padding or no padding), print sign
	if (Negative) {
		RHaliPutSerialChar('-');
	}

	while (Idx > 0) {
		RHaliPutSerialChar(Buffer[--Idx]);
	}
}

VOID
ROSAPI
RHalSerialPrintf(
	CONST CHAR* Format,
	...
)
{
	va_list Args;
	va_start(Args, Format);

	CONST CHAR* Ptr = Format;
	while (*Ptr) {
		if (*Ptr == '%') {
			Ptr++;
			
			// 1. Parse Padding (Zero or Space)
			CHAR PadChar = ' ';
			if (*Ptr == '0') {
				PadChar = '0';
				Ptr++;
			}

			// 2. Parse Width
			INT32 Width = 0;
			while (*Ptr >= '0' && *Ptr <= '9') {
				Width = Width * 10 + (*Ptr - '0');
				Ptr++;
			}

			// 3. Parse Modifiers (ll, l, z)
			BOOL IsLongLong = FALSE;
			BOOL IsSizeT = FALSE;

			if (*Ptr == 'l') {
				Ptr++;
				if (*Ptr == 'l') {
					IsLongLong = TRUE;
					Ptr++;
				}
				// 'l' (long) is treated as standard int width (32-bit) in this simplified implementation
				// or handled same as default for u/d/x unless ll is present.
			}
			else if (*Ptr == 'z') {
				IsSizeT = TRUE;
				Ptr++;
			}

			// 4. Parse Specifier
			switch (*Ptr) {
			case 's': {
				CONST CHAR* Str = va_arg(Args, CONST CHAR*);
				if (!Str) Str = "(null)";
				RHalPutSerial(Str);
				break;
			}
			case 'c': {
				CHAR Ch = (CHAR)va_arg(Args, int);
				RHaliPutSerialChar(Ch);
				break;
			}
			case 'd': {
				INT64 Val;
				if (IsLongLong) {
					Val = va_arg(Args, INT64);
				}
				else if (IsSizeT) {
#ifdef _WIN64
					Val = va_arg(Args, INT64);
#else
					Val = va_arg(Args, INT32);
#endif
				}
				else {
					Val = va_arg(Args, INT32);
				}

				BOOL Negative = (Val < 0);
				if (Negative) Val = -Val;
				RHaliPutSerialInt((UINT64)Val, 10, Width, PadChar, Negative);
				break;
			}
			case 'u': {
				UINT64 Val;
				if (IsLongLong) {
					Val = va_arg(Args, UINT64);
				}
				else if (IsSizeT) {
#ifdef _WIN64
					Val = va_arg(Args, UINT64);
#else
					Val = va_arg(Args, UINT32);
#endif
				}
				else {
					Val = va_arg(Args, UINT32);
				}
				RHaliPutSerialInt(Val, 10, Width, PadChar, FALSE);
				break;
			}
			case 'x': 
			case 'X': // Treat %X same as %x (lowercase) for now, or could pass Uppercase flag
			{
				UINT64 Val;
				if (IsLongLong) {
					Val = va_arg(Args, UINT64);
				}
				else if (IsSizeT) {
#ifdef _WIN64
					Val = va_arg(Args, UINT64);
#else
					Val = va_arg(Args, UINT32);
#endif
				}
				else {
					Val = va_arg(Args, UINT32);
				}
				RHaliPutSerialInt(Val, 16, Width, PadChar, FALSE);
				break;
			}
			case 'p': {
				UINT64 Val = (UINT64)va_arg(Args, PVOID);
				RHalPutSerial("0x");
				RHaliPutSerialInt(Val, 16, Width > 0 ? Width : 0, '0', FALSE);
				break;
			}
			default:
				RHaliPutSerialChar('%');
				if (*Ptr) RHaliPutSerialChar(*Ptr);
				break;
			}
		}
		else {
			RHaliPutSerialChar(*Ptr);
		}
		Ptr++;
	}

	va_end(Args);
}