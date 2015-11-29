/***************************************************************************
 * Copyright (C) EMC Corporation 2001-2014
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

//
// ktrace_NT.h
//	ktrace driver NT related definitions.
//
#if !defined(KTRACE_NT_H)
#define KTRACE_NT_H

#include "k10ntddk.h"
#include "EmcPAL_DriverShell.h"
#include "bool.h"
#include "ktrace.h"
//#include "ktraceintf.h"

#if 0
typedef long int EMCPAL_STATUS;

#if !defined(IN)
#define IN 
#endif
#if !defined(OUT)
#define OUT
#endif
#endif /*0*/
EMCPAL_STATUS KtraceCreateDevice(IN PEMCPAL_NATIVE_DRIVER_OBJECT pDriverObject) ;

EMCPAL_STATUS KtraceDispatchClose(
    IN PEMCPAL_DEVICE_OBJECT device_object_ptr,
    IN PEMCPAL_IRP irp_ptr);

EMCPAL_STATUS KtraceDispatchDeviceIoControl(
    IN PEMCPAL_DEVICE_OBJECT device_object_ptr,
    IN PEMCPAL_IRP irp_ptr);

EMCPAL_STATUS KtraceDispatchLoad(
	IN PEMCPAL_NATIVE_DRIVER_OBJECT pDriverObject);

EMCPAL_STATUS KtraceDispatchOpen(
    IN PEMCPAL_DEVICE_OBJECT device_object_ptr,
    IN PEMCPAL_IRP irp_ptr);

EMCPAL_STATUS KtraceDispatchUnload(
	IN PEMCPAL_NATIVE_DRIVER_OBJECT pDriverObject);


EMCPAL_STATUS KtraceUnsupportedCallback(
    IN PEMCPAL_DEVICE_OBJECT device_object_ptr,
    IN PEMCPAL_IRP irp_ptr
    );

#if KTRACE_MARKER
void KtraceMarkerStart(long intervalA, int ringA);
void KtraceMarkerStop (void);
#endif

// Being an NT/W2K dude I really want an extension. Call it a 
// security blanket. It can be small because currently most of the
// data we'd store here is in a static instance of a structure defined
// above (KTRC_file_head). We could, in the future, stuff the data that
// is in that header into the device extension

typedef struct _KTRACE_DEVICE_EXTENSION {
    PEMCPAL_DEVICE_OBJECT   DeviceObject;
    ULONG                   debugMask;
    ULONG                   eventLog;
    ULONG                   exitPrintThread;
    ULONG                   HighPriorityTrigger;
#if !defined(SIMMODE_ENV)
    EMCPAL_SPINLOCK	        ISRSpinLock;
	EMCPAL_SPINLOCK         printSpinLock;
#endif // !defined(SIMMODE_ENV)
	EMCPAL_SEMAPHORE        printSemaphore ;
    PEMCPAL_IRP             curPrintIRP;
#if !defined(SIMMODE_ENV)
    PEMCPAL_RENDEZVOUS_EVENT    InterruptPrintEvent ;
#else // !defined(SIMMODE_ENV)
    EMCPAL_SYNC_EVENT           InterruptPrintEvent ;
#endif // !defined(SIMMODE_ENV)
	const KTRACE_ring_id_T  ringA;
    EMCPAL_THREAD PrintThreadObjectPointer;  // worker thread 
    BOOLEAN PrintThreadObjectCreated;
} KTRACE_DEVICE_EXTENSION;

typedef KTRACE_DEVICE_EXTENSION *PKTRACE_DEVICE_EXTENSION;

#define KTRACE_DEVICE_EXTENSION_SIZE sizeof(KTRACE_DEVICE_EXTENSION)

#endif /* !defined(KTRACE_NT_H) */
/* end of file ktrace_NT.h */

