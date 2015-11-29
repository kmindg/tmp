#ifndef PPFD_PANIC_H
#define PPFD_PANIC_H

/***************************************************************************
 * Copyright (C) EMC Corporation 2008-2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation * All rights reserved
 ***************************************************************************/

/**************************************************************
 *  ppfd_panic.h
 **************************************************************
 *
 *  Defines and function prototypes for a panic facility for the ppfd driver.
 *  Taken from asidc_panic.h
 *
 * Revision History:
 *
 *
 **************************************************************/

#include "k10ntddk.h" 
#include "ktrace.h"
/*
 * When we panic, we specify WHO and WHY.  
 * The WHO is comprised of two parts:
 *  The upper 16 bits are a facility code, defined below.
 *  The lower 16 bits are defined in a header file for that facility.
 * The WHY is extra information (pointer, variable, etc.) that is
 *  dependant on WHO calls panic.
 */

/* Facility codes for WHO */
/* The Flare panic.h file has panic bases up to 0x04xxx00000,
 *  so we start our offsets at 0x05010000 so our values are distinct.
 * (We use our own panic facility so this is not critical)
 *
 * TBD - Reconcile our panic (error) codes with what is published for field service.
 */
#define PPFD_BASE_MASK    (0xFFFF0000)
#define PPFD_ENTRY_BASE   (0x06010000)
#define PPFD_IO_BASE      (0x06020000)    /* I/O code path */
#define PPFD_IOCTL_BASE   (0x06030000)    /* IRP_MJ_DEVICE_CONTROL code path */
#define PPFD_MISC_BASE    (0x06040000)
#define PPFD_LOG_BASE     (0x06050000)    /* event logging functions */
#define PPFD_REG_BASE     (0x06060000)    /* registry access functions */
#define PPFD_IRQL_BASE    (0x06070000)
/* special panic code for use by ppfdAssert() */
#define PPFD_ASSERT_PANIC (0x06080000)


// Panic Codes
#define PPFD_FAILED_PHYSICALPACKAGE_INIT (PPFD_ENTRY_BASE + 0x0000)
#define PPFD_FAILED_ADD_DEVICE           (PPFD_ENTRY_BASE + 0x0001)
#define PPFD_FAILED_CREATE_THREAD        (PPFD_ENTRY_BASE + 0x0002)

#define PPFD_ASYNC_EVENT_QUEUE_FULL      (PPFD_MISC_BASE + 0x0001)
#define PPFD_ASYNC_EVENT_INVALID         (PPFD_MISC_BASE + 0x0002)
#define PPFD_ASYNC_EVENT_INFO_INVALID    (PPFD_MISC_BASE + 0x0003)
#define PPFD_ASYNC_THREAD_CONTEXT_DATA_INVALID    (PPFD_MISC_BASE + 0x0004)
#define PPFD_ASYNC_EVENT_INSUFFICIENT_MEMORY      (PPFD_MISC_BASE + 0x0005)

#define PPFD_IO_INVALID_PARAMETER_IN_COMPLETION    (PPFD_IO_BASE + 0x0000)
#define PPFD_IO_INVALID_PAYLOAD_IN_COMPLETION      (PPFD_IO_BASE + 0x0001)
#define PPFD_IO_INVALID_BLOCK_OP_IN_COMPLETION     (PPFD_IO_BASE + 0x0002)
#define PPFD_IO_INVALID_PARAMETER                  (PPFD_IO_BASE + 0x0003)
#define PPFD_IO_INVALID_STATE                      (PPFD_IO_BASE + 0x0004)
/*
 * Prototypes from ppfdPanic.c
 */
extern BOOLEAN panicDbgBreakpoint;
/* inform compiler that ppfdPanic() does not return; prefast fix */
VOID ppfdPanic(UINT_PTR who, UINT_PTR why);
VOID ppfdAssert(UCHAR *filename, ULONG lineNumber, ULONG condition);


/*
 * Macros
 */

/* The panic macro will print two lines of DebugPrint() info, 
 *  will cause a WinDbg breakpoint (checked builds only, eventually),
 *  then calls ppfdPanic() in ppfd_panic.c which essentially
 *  calls KeBugCheckEx().
 */
#ifndef ALAMOSA_WINDOWS_ENV
SCSIPORT_API VOID ScsiDebugPrint (IN ULONG DebugPrintLevel, IN PCCHAR DebugMessage, ...);
#define DebugPrint(x) ScsiDebugPrint x
#endif /* ALAMOSA_WINDOWS_ENV - STDPORT - odd DDK interaction */

#if DBG
#define panic(who, why) (  \
    DebugPrint((1,"*** PPFD PANIC %s,%s\n", #who,#why)),    \
    DebugPrint((1,"%s:%d\n", __FILE__,__LINE__)),     \
    ((panicDbgBreakpoint) ? EmcpalDebugBreakPoint() : (VOID)FALSE), \
    ppfdPanic(who, why))
#else /* in free builds we just want to call ppfdPanic() */
#define panic(who, why) ppfdPanic(who, why)
#endif

#if DBG

/* A set of panic codes for IRQL violations.
 */
#define IRQL_GREATER_THAN_PASSIVE_LEVEL   (PPFD_IRQL_BASE + 0x00000000)
#define IRQL_GREATER_THAN_APC_LEVEL       (PPFD_IRQL_BASE + 0x00000001)
#define IRQL_GREATER_THAN_DISPATCH_LEVEL  (PPFD_IRQL_BASE + 0x00000002)

/* Test for IRQL passive level.
 */
/*#define IRQL_PASSIVE() \
    if (EmcKeGetCurrentIrql() > PASSIVE_LEVEL) \
    { \
        DebugPrint((1,"IRQL %d > passive level\n", EmcKeGetCurrentIrql() )); \
        panic(IRQL_GREATER_THAN_PASSIVE_LEVEL, EmcKeGetCurrentIrql()); \
    }*/

#define IRQL_PASSIVE() 
        //DebugPrint((1,"IRQL %d > passive level\n", EmcKeGetCurrentIrql() )); \


/* Test for IRQL APC level.
 */
#define IRQL_APC() \
    if (EMCPAL_LEVEL_IS_OR_ABOVE_DISPATCH()) \
    { \
        DebugPrint((1,"IRQL %d > APC level\n", EmcpalGetCurrentLevel() )); \
        panic(IRQL_GREATER_THAN_APC_LEVEL, EmcpalGetCurrentLevel()); \
    }

/* Test for IRQL dispatch level.
 */
#define IRQL_DISPATCH() \
    if (EMCPAL_LEVEL_ABOVE_DISPATCH()) \
    { \
        DebugPrint((1,"IRQL %d > dispatch level\n", EmcpalGetCurrentLevel() )); \
        panic(IRQL_GREATER_THAN_DISPATCH_LEVEL, EmcpalGetCurrentLevel()); \
    }
#else
#define IRQL_PASSIVE()
#define IRQL_APC()
#define IRQL_DISPATCH()
#endif


#endif /* PPFD_PANIC_H */
