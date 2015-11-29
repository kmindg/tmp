//++
// Copyright (C) EMC Corporation, 2003
// All rights reserved.
// Licensed material -- property of EMC Corporation
//--

//++
// File:            K10IoMarshal.h
//
// Description:     This file contains data structure definitions that are used
//                  in implementing K10IoMarshal library.
//
// History:
//                  21-Mar-2003       V Nagapraveen     Added code review changes
//                  11-Mar-2003       V Nagapraveen     Created
//--

#ifndef _K10_IOMARSHAL_H_
#define _K10_IOMARSHAL_H_

#include "k10ntddk.h"
#include "EmcPAL_DriverShell.h"
#include "EmcPAL_List.h"
#include "K10IoMarshalMessages.h"


//
//  K10 I/O Marshal provides flexible I/O queuing interface to the clients.
//  The term I/O is used to describe read or writes. It is up to the client
//  to determine what it needs to queue.
//
//  Below is a functional flow diagram that illustrates the usage of
//  K10 I/O Marshal by a client.
//

//
//
//                  Host                        Canceled I/O
//                          ------------------>
//                Initiated                     Callback
//                                           ( for each queued I/O )
//                                                  |
//                                                  |
//                                                  |-----------------------
//                                                  |                      |
//                                                  |                      |
//                                                  |                      |
//  Client                                          v                      v
//  Initiated-------->Inspect I/O----------->I/O Processing---------->I/O Complete
//                                    |             ^^
//                                    |              |
//                                    |                |
//                                    |                  |        I/O Callback
//                                    v                    |
//                               I/O Queued                  |  ( for each queued I/O )
//                                                             |
//                                                               |
//                                                                 |
//  Client                                                           |
//  Initiated--------->Arrest I/O---------->Arrest Processing----->Release I/O
//
//


//
//  Type:
//
//      K10_IO_MARSHAL_CALLBACK_EVENT
//
//  Description:
//
//      This enumeration lists the possible events for which the callback
//      routine is executed.
//
//  Members:
//
//      K10IoMarshalPlayArrestQueue
//
//          The above event indicates that the I/O is from the arrest queue.
//
//      K10IoMarshalInvalid
//
//          Terminator for the enumeration
//
//
typedef enum _K10_IO_MARSHAL_CALLBACK_EVENT
{
    K10IoMarshalPlayArrestQueue,

    K10IoMarshalInvalid

} K10_IO_MARSHAL_CALLBACK_EVENT, *PK10_IO_MARSHAL_CALLBACK_EVENT;


//
//  Type:
//
//      K10_IO_MARSHAL_TYPE
//
//  Description:
//
//      This enumeration lists the possible types of marshaling that is
//      currently supported.
//
//  Members:
//
//      K10IoMarshalTypeNoNewIo
//
//          The marshal will not allow new I/O to continue after an arrest.
//
//      K10IoMarshalTypeNoNewIoAndDrain
//
//          The marshal will not allow new I/O to continue after an arrest.
//          Additionally, all existing I/O will be drained off before an
//          arrest operation is allowed to complete.
//
//      K10IoMarshalTypeNoNewIoAndDeferredDrain
//
//          The marshal will not allow new I/O to continue after an arrest.  In
//          addition, the caller may or may not call "wait for drain" at some
//          point in the future.  This can be before or after the subsequent
//          release call.
//
//      K10IoMarshalTypeInvalid
//
//          Terminator for the enumeration
//
//
typedef enum _K10_IO_MARSHAL_TYPE
{
    K10IoMarshalTypeNoNewIo,

    K10IoMarshalTypeNoNewIoAndDrain,

    K10IoMarshalTypeNoNewIoAndDeferredDrain,

    K10IoMarshalTypeInvalid

} K10_IO_MARSHAL_TYPE, *PK10_IO_MARSHAL_TYPE;



//
//  Function:
//
//      K10_IO_MARSHAL_CALLBACK_ROUTINE
//
//  Description:
//
//      A routine that the K10 I/O Marshall will execute to replay I/Os
//      after being released.
//
//      The caller must assume that this routine will be called at
//      DISPATCH_LEVEL.
//
//  Parameters:
//
//      pIrp
//
//          The IRP that requires continuing.
//
//      Context
//
//          A caller supplied context to identify the device intended for this
//          IRP.  Most likely this is a device extension.
//
//
//      Event
//
//          An event id that specifies the reason for K10 I/O Marshal
//          executing the callback routine.
//
//  Returns:
//
//      None
//
typedef
VOID
(* K10_IO_MARSHAL_CALLBACK_ROUTINE)(

    IN PEMCPAL_IRP                             pIrp,
    IN PVOID                            Context,
    IN K10_IO_MARSHAL_CALLBACK_EVENT    Event
    );


//
//  Function:
//
//      K10_IO_MARSHAL_IO_CANCEL_ROUTINE
//
//  Description:
//
//      A routine that the K10 I/O Marshall will execute to replay I/Os
//      after I/O that is being arrested is cancelled
//
//
//  Parameters:
//
//      pDeviceObject
//
//          A pointer to the device object that issued the IRP.
//
//      pIrp
//
//          The IRP that has been cancelled.
//
//
//  Returns:
//
//      None
//


typedef
VOID
(* K10_IO_MARSHAL_IO_CANCEL_ROUTINE)(
    IN PEMCPAL_DEVICE_OBJECT   pDeviceObject,
    IN PEMCPAL_IRP             pIrp
    );



//
//  Name:
//
//      K10_IO_MARSHAL
//
//  Description:
//
//      The following defines the queue head for an I/O Marshal.  This should
//      be included in some "per-device" data structure.  Most likely this
//      will be embedded in a device extension.
//
//      This data structure should be treated as opaque by the K10 I/O Marshal
//      clients.
//
//  Members:
//
//      K10IoMarshalListEntry
//
//          List entry to group I/O marshal queues.
//
//      K10IoMarshalArrestQueueHead
//
//          The linked list for the K10 I/O Marshal arrest queue.
//
//      K10IoMarshalDeferredQueueHead
//
//          The linked list for the deferred requests between an inspect
//          and extricate operation.  This is only used when the "ByLink"
//          versions are called.
//
//      K10IoMarshalOutstandingQueueHead
//
//          The linked list for the outstanding requests between an inspect
//          and extricate operation.  This is only used when the "ByLink"
//          versions are called.
//
//      K10IoMarshalArrestQueueLock
//
//          The spin lock that protects the K10 I/O Marshal arrest queue.
//
//      Context
//
//          A caller supplied parameter that points to the "per-device"
//          data structure.  Most likely a device extension.
//
//      ArrestAllIo
//
//          This boolean indicates to the K10 I/O Marshal whether or not
//          all the I/O should be arrested.
//
//      Arrested
//
//          This boolean indicates whether or not we have been arrested.  It
//          is cleared on release.
//
//      ArrestQueueDepth
//
//          This field contains the number of entries in the Arrest Queue.
//
//      IRPPlaybackCallbackRoutine
//
//          A caller supplied callback routine used to replay I/O after
//          release
//
//      IRPCancelRoutine
//
//          A caller supplied callback routine to replay I/O after it
//          has been cancelled
//
//      K10IoMarshalType
//
//          The type of marshaling that the client desires (no new I/O or
//          drain).
//
//      K10IoMarshalQueueDepth
//
//          The number of I/Os that are currently outstanding.
//
//      K10IoMarshalQueueMaximumDepth
//
//          The maximum number of I/Os that were ever outstanding.
//
//      K10IoMarshalDrainEvent
//
//          The event that is signaled when there are no outstanding I/Os.
//
//      K10IoMarshalLastArrestTimestamp
//
//          The timestamp for the last point in time when the I/O Marshal
//          arrested this queue.
//

typedef struct _K10_IO_MARSHAL
{
    EMCPAL_LIST_ENTRY                              K10IoMarshalListEntry;

    EMCPAL_LIST_ENTRY                              K10IoMarshalArrestQueueHead;

    EMCPAL_LIST_ENTRY                              K10IoMarshalDeferredQueueHead;

    EMCPAL_LIST_ENTRY                              K10IoMarshalOutstandingQueueHead;

    EMCPAL_SPINLOCK                              K10IoMarshalArrestQueueLock;

    PVOID                                   Context;

    BOOLEAN                                 ArrestAllIo;

    BOOLEAN                                 Arrested;

    ULONG                                   ArrestQueueDepth;

    K10_IO_MARSHAL_CALLBACK_ROUTINE         IRPPlaybackRoutine;

    K10_IO_MARSHAL_IO_CANCEL_ROUTINE        IRPCancelRoutine;

    K10_IO_MARSHAL_TYPE                     K10IoMarshalType;

    ULONG                                   K10IoMarshalQueueDepth;

    ULONG                                   K10IoMarshalQueueMaximumDepth;

    EMCPAL_RENDEZVOUS_EVENT                                  K10IoMarshalDrainEvent;

    EMCPAL_LARGE_INTEGER                           K10IoMarshalLastArrestTimestamp;

    EMCPAL_SEMAPHORE                              ArrestQueueSemaphore;

} K10_IO_MARSHAL, *PK10_IO_MARSHAL;

//
// This macro is used to detect if the I/O Marshal queue is arrested.
//

#define K10_IO_MARSHAL_IS_ARRESTED( _p_io_marshal_ ) \
    (_p_io_marshal_)->Arrested

//
// This routine initializes the K10 I/O Marshal data structure that is
// embedded in a client's "per-device" data structure.  This routine
// may be called at any IRQL at or below DISPATCH_LEVEL.
//

VOID
K10IoMarshalInitialize(
    IN PK10_IO_MARSHAL                      pK10IoMarshalQueue,
    IN K10_IO_MARSHAL_TYPE                  MarshalType,
    IN K10_IO_MARSHAL_IO_CANCEL_ROUTINE     IRPCancelRoutine,
    IN K10_IO_MARSHAL_CALLBACK_ROUTINE      IRPPlaybackRoutine,
    IN PVOID                                Context
    );

//
// This routine cleans up the K10 I/O Marshal data structure that is
// embedded in a client's "per-device" data structure.  This routine
// may be called at any IRQL at or below DISPATCH_LEVEL.
//

VOID
K10IoMarshalCleanup(
    IN PK10_IO_MARSHAL                  pK10IoMarshalQueue
    );

//
// This routine checks if the I/O can proceed normally or if it
// needs to be arrested.
//

EMCPAL_STATUS
K10IoMarshalInspectIo(
    IN PK10_IO_MARSHAL                  pK10IoMarshalQueue,
    IN PEMCPAL_IRP                             IrpPtr
    );

//
// If the marshal type is "K10IoMarshalTypeNoNewIoAndDeferredDrain" this routine
// must be called to check if the I/O can proceed normally or if it
// needs to be arrested.
//

EMCPAL_STATUS
K10IoMarshalInspectIoByLink(
    IN PK10_IO_MARSHAL                  pK10IoMarshalQueue,
    IN PEMCPAL_IRP                             IrpPtr,
    IN PEMCPAL_LIST_ENTRY                      pIoLink
    );

//
// If the marshal type is "K10IoMarshalTypeNoNewIoAndDrain" this routine must
// be called when completing an I/O.
//

EMCPAL_STATUS
K10IoMarshalExtricateIo(
    IN PK10_IO_MARSHAL                  pK10IoMarshalQueue
    );

//
// If the marshal type is "K10IoMarshalTypeNoNewIoAndDeferredDrain" this routine
// must be called when completing an I/O.
//

EMCPAL_STATUS
K10IoMarshalExtricateIoByLink(
    IN PK10_IO_MARSHAL                  pK10IoMarshalQueue,
    IN PEMCPAL_LIST_ENTRY                      pIoLink
    );

//
// This routine Arrest' s I/O by queuing the specified I/O
// to the K10 I/O Marshal arrest queue. The client can also
// arrest all the subsequent I/O by specifying ArrestAllIo
// flag to be true.
//

VOID
K10IoMarshalArrestIo(
    IN PK10_IO_MARSHAL                  pK10IoMarshalQueue,
    IN PEMCPAL_IRP                             IrpPtr,
    IN BOOLEAN                          ArrestAllIo
    );

//
// This routine releases the I/O that is being held in the
// K10 I/O Marshal Arrest Queue.
//

VOID
K10IoMarshalReleaseIo(
    IN PK10_IO_MARSHAL                  pK10IoMarshalQueue
    );

//
// This routine returns the number of I/O 's that have been
// arrested so far.
//

ULONG
K10IoMarshalArrestQueueDepth(
    IN PK10_IO_MARSHAL                  pK10IoMarshalQueue
    );

//
// This routine returns the number of I/O Marshal Arrest Queues
//

ULONG
K10IoMarshalQueueLength(
    VOID
     );

//
// This routine waits for the draining of IOs.
//

EMCPAL_STATUS
K10IoMarshalWaitForDrain(
    IN PK10_IO_MARSHAL                  pK10IoMarshalQueue
    );


#endif // _K10_IOMARSHAL_H_















