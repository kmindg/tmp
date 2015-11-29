#include "EmcPAL.h"
#include "InlineInterlocked.h"
#include "swap_exports.h"
#include "legacy_driver_proto.h"

#ifndef DBG_PAGED_CODE
#ifdef EMCPAL_CASE_WK
#ifdef DBG
#include "ktrace.h"
#define DBG_PAGED_CODE() \
	if (KeGetCurrentIrql() > APC_LEVEL) { \
		KvPrint("DBG_PAGED_CODE called at IRQL %d\n", KeGetCurrentIrql() ); \
		KvPrint("at %s:%d ", __FILE__, __LINE__); \
		EmcpalDebugBreakPoint(); \
	}
#else
#define DBG_PAGED_CODE()
#endif
#else
#define DBG_PAGED_CODE()
#endif
#endif

//
//	This literal is a temporary placeholder to allow quick conversion of calls
//	to "stdio"-style string functions (sprintf(), strcat(), strlen(), etc.) that
//	do no string buffer bounds checking into calls to "string-safe" RtlString...()
//	functions.  The string-safe functions require a buffer length parameter, and
//	this value (INT32_MAX) can be used without further analysis.  As time permits,
//	uses of this literal can be replaced with the actual size of the string buffers
//	in question.
//
#ifndef K10_UNKNOWN_CHAR_BUF_LENGTH
#define K10_UNKNOWN_CHAR_BUF_LENGTH		(2147483647)
#endif

#ifndef I_AM_K10_DDK
#define I_AM_K10_DDK
#endif

#ifndef SAFE_AVOID_PULLING_IN_C_RUNTIMES
#include <string.h>
#include <wchar.h>
#endif
