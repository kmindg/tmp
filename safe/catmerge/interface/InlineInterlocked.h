/*******************************************************************************
 * Copyright (C) EMC Corporation, 1998-2008
 * All rights reserved.
 * Licensed material - Property of EMC Corporation
 ******************************************************************************/

/*******************************************************************************
 * InlineInterlocked.h
 *
 * This file redefines several HAL functions for X86 platforms.  This allows
 * for more accuracy when doing performance measurements.
 *
 * Notes:
 *
 * History:
 *
 *	05/98	Dave Harvey created.
 *  07/06   GSchaffer extend for 64-bit pointers.
 *          InterlockedCompareExchangePVOID, InterlockedCompareExchangePVOIDVOID
 *          argument order to match compiler intrinsics.
 ******************************************************************************/

# ifndef __InlineInterlocked_h
# define __InlineInterlocked_h 1

#include "k10csx.h"

#ifdef __cplusplus
extern "C" {
#endif

#if defined(_MM_HINT_NTA) || defined(CSX_AD_TOOLCHAIN_STYLE_GNU)
#define InlineCachePrefetchPVOID(addr)    csx_p_io_prefetch((const char *)(addr), CSX_P_IO_PREFETCH_HINT_T0)
#define asm_pause()				csx_p_atomic_crude_pause()
#else
#define InlineCachePrefetchPVOID(addr)
#define asm_pause()
#endif


// Interlocked instructions on natural alignment or may hang memory bus.
#if (!defined(I_AM_DEBUG_EXTENSION) && !defined(__DBG_EXT_CSX_H__)) &&  !defined(__DBG_EXT_H__)
#define EMCKE_PTR_ALIGN  DBG  /* 0, or 1 do-not-ship, or DBG */
#endif

#if EMCKE_PTR_ALIGN
extern ULONG EmcKePtrNotAligned(void);
extern ULONG EmcKeListNotAligned(void);
#define EMCKE_PTR_ALIGNED(p)  (((ULONG_PTR)(p) & (sizeof(ULONG_PTR)-1)) ? EmcKePtrNotAligned() : 0)
#define EMCKE_LIST_ALIGNED(p) (((ULONG_PTR)(p) & (sizeof(PVOIDPVOID)-1)) ? EmcKeListNotAligned() : 0)
#else
#define EMCKE_PTR_ALIGNED(p)  (0)
#define EMCKE_LIST_ALIGNED(p) (0)
#endif

#define PVOIDPVOID csx_ptrpair_t
 
#define InterlockedCompareExchangePVOID(mem, exc, cmp) csx_p_alt_atomic_cas_ptr((csx_pvoid_t *)mem, (csx_pvoid_t)exc, (csx_pvoid_t)cmp)
 
#define InterlockedCompareExchangePVOIDPVOID(mem,exc,cmp) csx_p_alt_atomic_cas_ptrpair((csx_ptrpair_t *)mem, (csx_ptrpair_t *)exc, (csx_ptrpair_t *)cmp)
 
#define InlineInterlockedIncrement(a) csx_p_atomic_inc_return((csx_atomic_t *)(a))
#define InlineInterlockedDecrement(a) csx_p_atomic_dec_return((csx_atomic_t *)(a))
#define InlineInterlockedExchange(a,b) csx_p_atomic_swap_nsint(a,b)
#define InlineInterlockedExchangeULONG(a,b) csx_p_atomic_swap_nuint(a,b)
#define InlineInterlockedExchangePVOID(a,b) csx_p_atomic_swap_ptr((PVOID *)a,b)
 
 // The inline 32 bit compare and exchange operations defined in another block
 // below have reversed the Comparand and Exchange arguments from what the 
 // Microsoft calls provide.  We must reverse the "b" and "c" arguments in the
 // following macro definitions
#define InlineInterlockedCompareAndExchange(a,b,c) (ULONG)csx_p_atomic_cas_32((csx_s32_t *)a,(LONG)b,(LONG)c)
#define InlineInterlockedCompareAndExchangePVOID(a,b,c) csx_p_atomic_cas_ptr((PVOID *)a,(PVOID)b,(PVOID)c)
#define InlineInterlockedCompareAndExchangeVolatileULONGLONG(a,b,c) (UINT_64E)csx_p_atomic_cas_64((INT_64E *)a, (INT_64E)b, (INT_64E)c)
#define InlineInterlockedCompareAndExchangeULONGLONG(a,b,c) InlineInterlockedCompareAndExchangeVolatileULONGLONG(a,b,c)

# ifdef __cplusplus
}
# endif
# endif
