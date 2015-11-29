/*******************************************************************************
 * Copyright (C) EMC Corporation, 1998-2008,2010,2013
 * All rights reserved.
 * Licensed material - Property of EMC Corporation
 ******************************************************************************/

/*******************************************************************************
 * EmcKeInline.h
 *
 * This file redefines several Ke/HAL functions for X86 platforms.  This allows
 * for more accuracy when doing performance measurements, so that time for
 * basic primitives get chalked up to the .sys file that uses them. There
 * also might be some slight raw performance improvement, at least in some cases.
 *
 *
 * These are the standard renames to use:
 *    InterlockedIncrement => InlineInterlockedIncrement
 *    InterlockedDecrement => InlineInterlockedDecrement
 *    InterlockedExchange =>  InlineInterlockedExchange
 *    KeInitializeSpinLock => EmcKeInitializeSpinLock
 *    KeAcquireSpinLockAtDpcLevel  => EmcKeAcquireSpinLockAtDpcLevel
 *    KeReleaseSpinLockFromDpcLevel => EmcKeReleaseSpinLockFromDpcLevel
 *    KeGetCurrentIrql => EmcKeGetCurrentIrql
 *    KeAcquireSpinLock => EmcKeAcquireSpinLock
 *    KeReleaseSpinLock => EmcKeReleaseSpinLock
 *    KeWaitForSingleObject => EmcKeWaitForSingleObject
 *    KeWaitForMutextObject => EmcKeWaitForMutexObject
 *    KeDelayExecutionThread => EmcKeDelayExecutionThread
 *    KeLowerIrql => EmcKeLowerIrql
 *    KeRaiseIrql => EmcKeRaiseIrql
 *
 *
 * Notes:
 *
 * History:
 *
 *	05/98	Dave Harvey created.
 *  07/06   GSchaffer. Inline assembly is not allowed by Win64 studio.
 *          Either use compiler intrinsic, or extern EmcKe\Emc64.asm function.
 ******************************************************************************/

# ifndef __EmcKeInline_h
# define __EmcKeInline_h 1

#include "EmcPAL_Configuration.h"

/* if we have inlined spinlocks and we're running in c4 user space with user preemption disable supported then
 * as an optimization we don't need to do irql manipulation
 */
#if defined(EMCPAL_CASE_WK) || defined(CSX_BV_LOCATION_KERNEL_SPACE) || !defined(CSX_BO_USER_PREEMPTION_DISABLE_SUPPORTED) || defined(SIMMODE_ENV)
#define EMCKE_LOCKS_USE_IRQL_MANIPULATION
#endif

#ifdef EMCPAL_CASE_WK

#include <ntddk.h>
#include "InlineInterlocked.h"

#ifdef __cplusplus
extern "C" {
#endif

#if defined(SIMMODE_ENV) && !defined(EMCPAL_CASE_NTDDK_EXPOSE)
#error "use case no longer supported!"
#endif
#if defined(UMODE_ENV) && !defined(EMCPAL_CASE_NTDDK_EXPOSE)
#error "use case no longer supported!"
#endif

extern void EmcKePANIC(const char * why);

#define _InterlockedExchangeLONG_PTR(p,v) csx_p_atomic_swap_ptr((PVOID *)p, CSX_CAST_PTRHLD_TO_PTR(v))

#define InlineCachePrefetchntPVOID(addr) csx_p_io_prefetch((const char *)addr, CSX_P_IO_PREFETCH_HINT_NTA)

#define EmcKeGetCurrentThread() KeGetCurrentThread()

#define EMCKeInlinedX86 1 //PZPZ

// To enable SPINON_TRACING in CSX and CBFS, also add this define in header file csx_rt_sked_llp_kw_if.h.
//#define SPINON_TRACING  DBG  /* 0, or 1 do-not-ship, or DBG */
#if SPINON_TRACING
// Services\EmcKe\EmcKe.c - trace function name on spinlock contention.
#define SPINON_FDECL    ,const char *__function__
#define SPINON_FUNCTION ,__FUNCTION__
#define SPINON_function ,__function__
#else
#define SPINON_FUNCTION
#define SPINON_FDECL
#define SPINON_function
#endif

// Local Function:  EmcKeSpinOnLockAtDpcLevel
//
// We break out the lock spinning time into a separate function
// so that vtune can measure it on a per-file basis.  We
// want all of the spinning captured in this function, which
// is why the retry of the lock get is done here and not
// in the caller.  The successful get here is measured as
// spin time, but there must have been an unsucessful get in 
// the caller which is not counted as spin time.  These 
// errors cancel.
extern void EmcKeSpinOnLockAtDpcLevel(IN volatile PKSPIN_LOCK SpinLock  SPINON_FDECL);

// This function sets bit 0 of a lock and returns the original value 
// of bit 0 of the lock.  The LOCK# signal is asserted during the bts 
// instruction to ensure the instruction is atomic.
static __forceinline ULONG_PTR InlineInterlockedBTS0(PULONG_PTR pLock) 
{
#ifdef _AMD64_
    return (ULONG_PTR) _interlockedbittestandset64((PLONG_PTR)pLock, 0);
#else
#ifndef ALAMOSA_WINDOWS_ENV
    return (ULONG_PTR) _interlockedbittestandset((PLONG_PTR)pLock, 0);
#else
    __asm {
        xor eax, eax
        push ecx
        mov ecx, pLock
        lock bts DWORD PTR [ecx], 0
        setc al
        pop ecx
    }
#endif /* ALAMOSA_WINDOWS_ENV - STDPORT */
#endif
}

static __forceinline VOID EmcKeInitializeSpinLock(IN volatile PKSPIN_LOCK SpinLock) 
{
    EMCKE_PTR_ALIGNED(SpinLock);
    KeInitializeSpinLock(SpinLock);
}

// Set the current IRQL to DISPATCH_LEVEL
static __forceinline VOID EmcKeSetIrqlToDispatchLevel(VOID)
{
    EMCPAL_KIRQL kirql;
    KeRaiseIrql(DISPATCH_LEVEL, & kirql);
}

// Set the current IRQL to APC_LEVEL
static __forceinline VOID EmcKeSetIrqlToApcLevel(VOID)
{
    KeLowerIrql(APC_LEVEL);
}

// Set the current IRQL to PASSIVE_LEVEL
static __forceinline VOID EmcKeSetIrqlToPassiveLevel(VOID)
{
    KeLowerIrql(PASSIVE_LEVEL);
}

// Local Function:  EmcKeSpinOnLock
//
// We break out the lock spinning time into a separate function
// so that vtune can measure it on a per-file basis.  We
// want all of the spinning captured in this function, which
// is why the retry of the lock get is done here and not
// in the caller.  The successful get here is measured as
// spin time, but there must have been an unsucessful get in 
// the caller which is not counted as spin time.  These 
// errors cancel.
extern void EmcKeSpinOnLock(IN volatile PKSPIN_LOCK SpinLock, EMCPAL_KIRQL oldIrql  SPINON_FDECL);

extern void EmcKeSpinOnTemplatedLockedList(IN volatile PVOID *ListLock, EMCPAL_KIRQL oldIrql);

// Kernel function:  EmcKeGetCurrentIrql 
//          replaces KeGetCurrentIrql
static __forceinline  EMCPAL_KIRQL 
EmcKeGetCurrentIrql(VOID) 
{
    EMCPAL_KIRQL irql;
    irql =   KeGetCurrentIrql();
    return irql;
}

// Kernel function:  EmcKeAcquireSpinLockAtDpcLevel 
//          replaces KeAcquireSpinLockAtDpcLevel
#define EmcKeAcquireSpinLockAtDpcLevel(SpinLock)  EmcKeIAcquireSpinLockAtDpcLevel ((SpinLock)  SPINON_FUNCTION)
static __forceinline VOID EmcKeIAcquireSpinLockAtDpcLevel (
    IN  PKSPIN_LOCK SpinLock
    SPINON_FDECL
    )
{   
#   if DBG
    if (EmcKeGetCurrentIrql() != DISPATCH_LEVEL) {
        EmcKePANIC("Wrong IRQL");
    }
#   endif
    EMCKE_PTR_ALIGNED(SpinLock);
    
    if (InlineInterlockedBTS0((PULONG_PTR)SpinLock)) {
        EmcKeSpinOnLockAtDpcLevel(SpinLock  SPINON_function);
    }
}

// Kernel function:  EmcKeReleaseSpinLockFromDpcLevel 
//          replaces KeReleaseSpinLockAtDpcLevel
static __forceinline VOID EmcKeReleaseSpinLockFromDpcLevel (
    IN volatile PKSPIN_LOCK SpinLock
    )
{
#   if DBG
    if (EmcKeGetCurrentIrql() != DISPATCH_LEVEL) {
        EmcKePANIC("Wrong IRQL");
    }
    
#   endif

#if OPTIMIZE_RELEASE
    if (!*SpinLock) {   // Releasing lock that is not held?
        EmcKePANIC("Release of lock not held");
    }
    *Spinlock = 0; // volatile - speculative reads are OK on release.
#else
    if (!_InterlockedExchangeLONG_PTR((PLONG_PTR)SpinLock, 0)) {
        EmcKePANIC("Release of lock not held");
    }
#endif
}

#define EmcKeRaiseIrql(a,b) *(b) = EmcKfRaiseIrql(a)

#define EmcKeLowerIrql(a) EmcKfLowerIrql(a)

static __forceinline VOID EmcKfLowerIrql (IN EMCPAL_KIRQL NewIrql)
{
# if DBG
    if (EmcKeGetCurrentIrql() < NewIrql) {
        EmcKePANIC("EmcKfLowerIrql Raising IRLQ");
    }
# endif

    KeLowerIrql(NewIrql);
}

static __forceinline EMCPAL_KIRQL EmcKfRaiseIrql ( IN EMCPAL_KIRQL NewIrql)
{
# if DBG
    if (NewIrql > HIGH_LEVEL) {
        EmcKePANIC("EmcKfLowerIrql IRLQ out of range");
    }
    if (EmcKeGetCurrentIrql() > NewIrql) {
        EmcKePANIC("EmcKfRaiseIrql Lowering IRLQ");
    }
# endif

    return KfRaiseIrql(NewIrql);
}


// Kernel function:  EmcKeTryAcquireSpinLock         
static __forceinline BOOL
EmcKeTryAcquireSpinLock(
                    IN volatile PKSPIN_LOCK SpinLock, 
                    EMCPAL_KIRQL* oldIrql)                 
                    
{   
    EMCPAL_KIRQL  Res;
    
    Res = EmcKeGetCurrentIrql();
    
    // avoid the IQRL raise which stalls pipe-line if it
    // would be a no-op.
    if (Res != DISPATCH_LEVEL) {
# if DBG
        if (Res != PASSIVE_LEVEL  && Res != APC_LEVEL) {
            EmcKePANIC("Wrong IRQL");
        }
# endif
        
        EmcKeSetIrqlToDispatchLevel();
    }
    
    EMCKE_PTR_ALIGNED(SpinLock);
    if (InlineInterlockedBTS0((PULONG_PTR)SpinLock)) {
        EmcKeLowerIrql(Res);
        return FALSE;
    }
    *oldIrql = Res;
    return TRUE;
}

// Kernel function:  EmcKeAcquireSpinLock 
//          replaces KeAcquireSpinLock
#define EmcKeAcquireSpinLock(a,b) *(b) = EmcKeIAcquireSpinLock((a)  SPINON_FUNCTION)
static __forceinline EMCPAL_KIRQL
EmcKeIAcquireSpinLock(
                    IN volatile PKSPIN_LOCK SpinLock
                    SPINON_FDECL
                    )
{   
    EMCPAL_KIRQL  Res;
    
    Res = EmcKeGetCurrentIrql();
    
    // avoid the IQRL raise which stalls pipe-line if it
    // would be a no-op.
    if (Res != DISPATCH_LEVEL) {
# if DBG
        if (Res != PASSIVE_LEVEL  && Res != APC_LEVEL) {
            EmcKePANIC("Wrong IRQL");
        }
# endif
        
        EmcKeSetIrqlToDispatchLevel();
    }
    
    EMCKE_PTR_ALIGNED(SpinLock);
    if (InlineInterlockedBTS0((PULONG_PTR)SpinLock)) {
        EmcKeSpinOnLock(SpinLock, Res  SPINON_function);
    }
    return Res;
}


// Kernel function:  EmcKeReleaseSpinLock 
//          replaces KeReleaseSpinLock
static __forceinline VOID
EmcKeReleaseSpinLock(
                   IN volatile PKSPIN_LOCK SpinLock,
                   IN EMCPAL_KIRQL newIRQL
                   
                   )
{
#   if DBG
    if (EmcKeGetCurrentIrql() != DISPATCH_LEVEL) {
        EmcKePANIC("Wrong IRQL");
    }
    
#   endif

#if OPTIMIZE_RELEASE
    if (!*SpinLock) {   // Releasing lock that is not held?
        EmcKePANIC("Release of lock not held");
    }
    *Spinlock = 0; // volatile - speculative reads are OK on release.
#else
    if (!_InterlockedExchangeLONG_PTR((PLONG_PTR)SpinLock, 0)) {
        EmcKePANIC("Release of lock not held");
    }
#endif

    // If we should stay at dispatch level, do nothing. 
    if (newIRQL != DISPATCH_LEVEL) {
        // Otherwise, we should only be changing the level down.
        EmcKeLowerIrql(newIRQL);
    }
}

// Functions replaced for improved debugging.

// Kernel function:  EmcKeWaitForMutexObject 
//          replaces KeWaitForMutexObject
#define EmcKeWaitForMutexObject EmcKeWaitForSingleObject


// prefast generates a warning stating that the argument 'WaitMode' should exactly 
// match the type 'enum _MODE'.  'WaitMode' should be typed as KPROCESSOR_MODE as it 
// is in the function below.  Microsoft has acknowledged this PREfast bug.  We need 
// to disable PREfast warning 8139 until a PREfast fix has been released.
// disable 4068 warning so that compiler does not generate unknown pragma warning and 
// disable 8139 warning so that prefast does not flag type mismatch during this 
// inline function; restore warnings notification to original state after function
#pragma warning(disable: 4068)
#pragma prefast(disable: 8139)
// Kernel function:  EmcKeWaitForSingleObject 
//          replaces KeWaitForSingleObject
static __forceinline NTSTATUS 
EmcKeWaitForSingleObject (
    IN PVOID Object,
    IN KWAIT_REASON WaitReason,
    IN KPROCESSOR_MODE WaitMode,
    IN BOOLEAN Alertable,
    IN PLARGE_INTEGER Timeout OPTIONAL
    )
{
    if (EmcKeGetCurrentIrql() > APC_LEVEL) {
        if (!Timeout || Timeout->QuadPart) {
            EmcKePANIC("wait at wrong IRQL");
        }
    }

    EMCKE_PTR_ALIGNED(Object);
    return KeWaitForSingleObject(Object, WaitReason, WaitMode, Alertable, Timeout);
}
#pragma prefast(default: 8139)
#pragma warning(default: 4068)

   
// Kernel function:  EmcKeDelayExecutionThread 
//          replaces KeDelayExecutionThread
extern EMCPAL_STATUS 
EmcKeDelayExecutionThread (
    IN KPROCESSOR_MODE WaitMode,                    
    IN BOOLEAN Alertable,                           
    IN PLARGE_INTEGER Interval                      
    );

#ifdef __cplusplus
}
#endif

#else // EMCPAL_CASE_WK

#ifdef __cplusplus
extern "C" {
#endif

//PZPZ MAKE THIS CLEANER
extern void EmcKePANIC(const char * why);
extern void EmcKeSpinOnTemplatedLockedList(IN volatile PVOID *ListLock, EMCPAL_KIRQL oldIrql);
#define InlineCachePrefetchntPVOID(addr) csx_p_io_prefetch((const char *)addr, CSX_P_IO_PREFETCH_HINT_NTA)

#ifdef __cplusplus
}
#endif

#endif // EMCPAL_CASE_WK

# endif
