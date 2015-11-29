#ifndef BGSL_WINDDK_H
#define BGSL_WINDDK_H

#include "EmcPAL.h"
#include "EmcPAL_Memory.h"
#include "EmcPAL_Misc.h"

#include "fbe/fbe_platform.h"

#include "bgsl_types.h"

#if !defined(UMODE_ENV) && !defined(SIMMODE_ENV)
#undef  printf
#endif

#include <stdio.h>
#include <stdarg.h>

#if !defined(UMODE_ENV) && !defined(SIMMODE_ENV)
#undef  printf
#define printf EmcpalDbgPrint
#endif

#include "ktrace.h"

/* Memory manipulators */

__forceinline static void bgsl_zero_memory(void * dst,  bgsl_u32_t length)
{
    EmcpalZeroMemory(dst,length);
}

__forceinline static void bgsl_copy_memory(void * dst, const void * src, bgsl_u32_t length)
{
    EmcpalCopyMemory(dst,src,length);
}

__forceinline static void bgsl_move_memory(void * dst, const void * src, bgsl_u32_t length)
{
    EmcpalMoveMemory(dst,src,length);
}

__forceinline static void bgsl_set_memory(void * dst, unsigned char fill, bgsl_u32_t length)
{
    EmcpalFillMemory(dst,length, fill);
}

__forceinline static bgsl_bool_t bgsl_equal_memory(const void * src1, const void * src2, bgsl_u32_t length)
{
    return(EmcpalEqualMemory(src1,src2,length));
}

#endif /* BGSL_WINDDK_H */
