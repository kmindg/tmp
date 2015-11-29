/************************************************************************
 *
 *  Copyright (c) 2011 EMC Corporation.
 *  All rights reserved.
 *
 *  ALL RIGHTS ARE RESERVED BY EMC CORPORATION.  ACCESS TO THIS
 *  SOURCE CODE IS STRICTLY RESTRICTED UNDER CONTRACT. THIS CODE IS TO
 *  BE KEPT STRICTLY CONFIDENTIAL.
 *
 *  UNAUTHORIZED MODIFICATIONS OF THIS FILE WILL VOID YOUR SUPPORT
 *  CONTRACT WITH EMC CORPORATION.  IF SUCH MODIFICATIONS ARE FOR
 *  THE PURPOSE OF CIRCUMVENTING LICENSING LIMITATIONS, LEGAL ACTION
 *  MAY RESULT.
 *
 *
 ************************************************************************/

#ifndef __TestControlImplDefs__h__
#define __TestControlImplDefs__h__


//++
// File Name:
//      TestControlImplDefs.h
//
// Contents:
//      This file provides the definitions needed to implement Test Control
//      operations in kernel space, that is, to handle specific
//      IOCTL_TEST_CONTROL requests sent from user space, to query current
//      requests for their possible influence on kernel behavior, and
//      generally to manage the mechanism for a driver.
//
//      See services/TestControl/TestControlOverview.txt for an overview of
//      the Test Control mechanism.
//
//      As many definitions herein as possible are usable from both C and C++.
//--


#include "TestControlUserDefs.h"

#include "BasicLib/BasicIoRequest.h"
    // BasicIoRequest
#include "EmcPAL_Irp.h"
    // EMCPAL_IRP_TRANSFER_LENGTH, EMCPAL_STATUS
#include "K10SpinLock.h"
    // K10SpinLock
//#include "ntdef.h"
    // LIST_ENTRY


// forward declaration
#ifdef __cplusplus
struct TctlLocus;
#else
struct _TctlLocus;
typedef struct _TctlLocus TctlLocus;
#endif


#ifdef __cplusplus
extern "C" {
#endif



////////
// The following routines permit driver code (including operation handlers) to
// operate on the active test control IOCTLs in a locus.
////////


// The following routines provide mutually exclusive access to a locus.
// All other locus operations may be performed only when the lock is held.

// Acquires the locus lock.
void TctlAcquireLock (TctlLocus* locus);

// Releases the locus lock.
void TctlReleaseLock (TctlLocus* locus);


// The following routines provide enumeration of the list of active test control
// IOCTLs and allow for adding and removing test control IOCTLs to/from the list.
//
// The valid sequences of calls of the following routines are described by:
//
//  [ AcquireLock [ InitEnum [ TakeNext | GiveOver ]* TermEnum ]* ReleaseLock ]*
//
// Note in particular that any sequence of TakeNext and GiveOver is permissible:
//  * Test control IOCTLs can be given to the locus at any time during an
//    enumeration (but do not become part of that enumeration).
//  * Once existing IOCTLs have been enumerated, TakeNext will return NULL as
//    many times as it's called (until the current enumeration is terminated).

// Initiates an enumeration of current test control IOCTLs.
// An enumeration must not be active.
// The locus lock must already be held and will remain held on return.
//
void TctlInitEnum (TctlLocus* locus);

// Terminates an enumeration of current test control IOCTLs.
// An enumeration must be active.
// The locus lock must already be held and will remain held on return.
//
void TctlTermEnum (TctlLocus* locus);

// Takes the next test control IOCTL from the locus and returns its buffer.
// Returns NULL when no more test control IOCTLs are left to enumerate.
//
// The sequence of TakeNext calls between an InitEnum and TermEnum enumerates the
// list of test control IOCTLs that were held by the locus at the time of the
// InitEnum.  In particular, calls to GiveOver do not affect the enumeration.
//
// An enumeration must be active.  (Hence, the locus lock must already be held
// and will remain held on return.)
//
TctlBuffer* TctlTakeNext (TctlLocus* locus);

// Gives the test control IOCTL owning the specified buffer to the locus.
//
// Note that this call does not enter its argument into the current enumeration.
//
// An enumeration must be active.  (Hence, the locus lock must already be held
// and will remain held on return.)
//
void TctlGiveOver (TctlLocus* locus, TctlBuffer* buffer);


// The following routines provide access to the locus context.

// Gets the locus context.
void* TctlGetContext (TctlLocus* locus);

// Sets the locus context.
void TctlSetContext (TctlLocus* locus, void* context);



////////
// The following routines provide operations on test control IOCTL buffers.
////////


// Returns whether a test control IOCTL buffer is (yet) to be treated as active.
// To prepare for future enhancements that require that certain test control
// requests be treated as inactive despite their presence on the list, a handler
// should not treat as active any TctlBuffer returned by TctlTakeNext that does
// not satisfy TctlIsActive.
//
bool TctlIsActive (TctlBuffer* buffer);


// A test control operation handler may establish a "cancel logic routine" for
// a test control IOCTL buffer to perform cleanup processing on cancellation.
// If the buffer's Irp is canceled while being held, the cancel logic routine
// will be invoked, with the buffer removed from the list of current buffers
// and the locus lock no longer held.  If the cancel logic routine returns
// true, it must have handled the cancellation (that is, completed the Irp with
// STATUS_CANCELLED, assuming that's appropriate).  If the cancel logic routine
// returns false (similarly to when no cancel logic routine is defined for the
// buffer), the Irp will be completed with STATUS_CANCELLED.
//
typedef bool (*TctlCancelLogic) (TctlBuffer* buffer);

// Sets the cancel logic routine for a test control IOCTL buffer.
//
// When a test control IOCTL buffer is first presented to its operation handler,
// no cancel logic routine is defined.  (That is, the value is NULL.)  The value
// is subsequently changed only by this routine.  In particular, the value
// persists no matter how often the IOCTL buffer is removed from and added back
// to the list of current buffers.
//
void TctlSetCancelLogic (TctlBuffer* buffer, TctlCancelLogic cancelLogic);


// Returns the length of a test control IOCTL buffer.
EMCPAL_IRP_TRANSFER_LENGTH TctlBufferLength (TctlBuffer* buffer);


// Sets the result status for a test control IOCTL.
void TctlSetStatus (TctlBuffer* buffer, EMCPAL_STATUS status);

// Sets the result information for a test control IOCTL.
void TctlSetInformation (TctlBuffer* buffer, ULONG_PTR information);


// Gets the result status for a test control IOCTL.
EMCPAL_STATUS TctlGetStatus (TctlBuffer* buffer);

// Gets the result information for a test control IOCTL.
ULONG_PTR TctlGetInformation (TctlBuffer* buffer);


// Completes a test control IOCTL with the current result status and information.
void TctlCompleteRequest(TctlBuffer* buffer);

// Completes a test control IOCTL with the given result status and information.
void TctlCompleteRequestWith
    (TctlBuffer* buffer, EMCPAL_STATUS status, ULONG_PTR information);



////////
// The following routines provide general management of the test control
// mechanism.
////////


// The behavior of a test control operation is implemented in an operation handler.
// The following typedef describes the signature of an operation handler.
//
// When an operation handler is called:
//  * The locus lock is not held.  (This facilitates a handler executing production
//    (i.e., non-testing) logic, which may acquire and release the locus lock.)
//
// An operation handler:
//  * May acquire and release the locus lock any number of times.
//  * May perform any number of test control enumerations (while holding the lock).
//  * Should treat the locus opaquely (not refer directly to contents of TctlLocus).
//  * May complete the test control IOCTL or not.
//
// When an operation handler returns:
//  * The locus lock must not be held.
//
// The handler returns whether it completed the operation.  If false, the test control
// IOCTL will be marked pending and placed at the end of the locus's queue to influence
// future behavior.
//
typedef bool (*TctlHandler) (
    void*               context,
    TctlLocus*          locus,
    TctlBuffer*         buffer);


// A device forwarding routine forwards a test control request to the next
// lower device when the locus's driver is not the targeted driver.
//
typedef void (*TctlDeviceForwardingRoutine) (void* context, BasicIoRequest* irp);


// A WWID lookup routine is used to identify a related test control locus
// associated with the WWID provided in the test control request.  The
// routine returns NULL to indicate no match was found.
//
typedef TctlLocus* (*TctlWwidLookupRoutine) (void* context, TctlWWID* wwid);


// Sets the driver identifier for the locus.
// `driverId == 0` declines to identify the driver.
// The driver ID may not be set to nonzero when it's already nonzero.
// Initially the driver identifier is zero.
void TctlSetDriverId (TctlLocus* locus,
        TctlDriverId driverId);

// Sets the device forwarding routine for the locus.
// `routine == NULL` removes any existing device forwarding routine.
// The device forwarder may not be set to non-`NULL` when it's already non-`NULL`.
// Initially the device forwarding routine is `NULL`.
void TctlSetDeviceForwardingRoutine (TctlLocus* locus,
        TctlDeviceForwardingRoutine routine, void* context);


// Use at most one of the following two routines (and only once) for each locus.

// Associates the locus with a driver.
// Provides a routine to look up a volume locus corresponding to a WWID.
void TctlSetAsDriverLocus (TctlLocus* locus,
        TctlWwidLookupRoutine wwidLookupRoutine, void* wwidLookupContext);

// Associates the locus with a volume.
// Provides the volume WWID and the locus of the driver the volume belongs to.
void TctlSetAsVolumeLocus (TctlLocus* locus,
        TctlWWID* volumeWwid, TctlLocus* driverLocus);

// Provided one of the preceding two routines has been used for a locus, this
// routine returns the driver locus associated with the locus.
TctlLocus* TctlGetDriverLocus (TctlLocus* locus);


// Sets the next locus to search for a handler.
// `link == NULL` removes any existing locus.
// The handling link may not be set to non-`NULL` when it's already non-`NULL`.
// Initially the handling link is `NULL`.
void TctlSetHandlingLink (TctlLocus* locus,
        TctlLocus* link);


// Handles an IOCTL_TEST_CONTROL Irp by dispatching to an appropriate operation handler.
void TctlHandleTestControlIoctl (TctlLocus* locus,
        BasicIoRequest* irp);


// Establishes (or removes, when `handler == NULL`) the association between an
// operation ID (represented by a feature ID and an opcode ID) and a handler
// (represented by a routine and an opaque context).
// An association may not be made for an operation ID if one already exists.
// The handler previously in effect (or `NULL` if none) is returned.
TctlHandler TctlDefineOperation (TctlLocus* locus,
        TctlFeatureId featureId, TctlOpcodeId opcodeId, TctlHandler handler, void* context);

// Defines the built-in operations: those associated with the "Tctl" feature.
void TctlDefineBuiltinOperations (TctlLocus* locus);


#ifdef __cplusplus
}
#endif


#ifdef __cplusplus

// Machinery to allow use of K10HashTable

#include "K10HashTable.h"
    // K10HashTable

struct TestControlHandlerHashKey {
    TctlOperationId OpId;
    TctlOperationId Hash() const { return OpId; }
};

inline bool operator== (TestControlHandlerHashKey a, TestControlHandlerHashKey b)
{
    return a.OpId == b.OpId;
}

inline bool operator> (TestControlHandlerHashKey a, TestControlHandlerHashKey b)
{
    return a.OpId > b.OpId;
}

struct TestControlHandlerHashElement {
    TestControlHandlerHashKey        mKey;
    TctlHandler                      mHandler;
    void*                            mContext;
    TestControlHandlerHashElement*   mLink;
    TestControlHandlerHashKey & Key () { return mKey; }
};

#define HandlerHashTableSize 16  // Value is arbitrary.
typedef K10HashTable<TestControlHandlerHashElement,
                     TestControlHandlerHashKey,
                     HandlerHashTableSize,
                     &TestControlHandlerHashElement::mLink>
    TctlHandlerHashTable;

#endif


// The following structure defines the contents of a test control locus.
// The structure contents should generally be treated as opaque.
//
#ifdef __cplusplus
struct TctlLocus {
#else
struct _TctlLocus {
#endif

    // This spin lock protects locus data.
    K10SpinLock                 Lock;

    // Active test control IOCTL requests are kept on this queue.
    EMCPAL_LIST_ENTRY                  Queue;

    // During an enumeration of active test control requests, `Cursor` locates
    // the next active request to be enumerated.
    // When it locates the `Queue` member itself, no active requests remain to
    // be enumerated.
    // When it is `NULL`, no enumeration is active.
    EMCPAL_LIST_ENTRY*                 Cursor;

    // This member allows the handlers for a locus to associate arbitrary data
    // with the locus.
    void*                       Context;

    // This member indicates whether there might be any active test control
    // requests on the locus's queue.  It is intended to provide an efficient
    // way to detect the normal case where no test control requests are active.
    // To check the value, read the member atomically; if the value is zero, no
    // test control requests are active for the locus.
    UINT32                      MightBeActive;

    // A locus may identify its driver.
    // Zero means no driver has been identified.
    TctlDriverId                DriverId;

    // A locus may specify a device forwarding routine along with its context.
    // The first member is `NULL` if no routine has been specified.
    TctlDeviceForwardingRoutine DeviceForwardingRoutine;
    void*                       DeviceForwardingContext;

    // This member specifies the WWID associated with the locus.
    // Nonzero indicates the volume with that WWID.  Zero indicates the driver.
    TctlWWID                    Wwid;

    // A locus may specify a routine to look up a WWID along with its context.
    // The first member is `NULL` if no routine has been given.
    TctlWwidLookupRoutine       WwidLookupRoutine;
    void*                       WwidLookupContext;

    // A volume locus may specify its driver's locus.
    // A value of `NULL` means the locus is associated with a driver.
    TctlLocus*                  DriverLocus;

    // This member specifies the next locus to search for a handler.
    // `NULL` means there is no next locus.
    TctlLocus*                  HandlingLink;

    // This member indicates the next instance identifier value to use.
    ULONG64                     NextInstanceId;

#ifdef __cplusplus

    // This member maintains the set of handlers for the locus.
    TctlHandlerHashTable        Handlers;

public:

    TctlLocus () :
        Lock(),
        Cursor(NULL),
        Context(NULL),                  // to be tidy
        MightBeActive(false),
        DriverId(0),
        DeviceForwardingRoutine(NULL),
        DeviceForwardingContext(NULL),  // to be tidy
        Wwid(0,0),
        WwidLookupRoutine(NULL),
        WwidLookupContext(NULL),        // to be tidy
        DriverLocus(NULL),
        HandlingLink(NULL),
        NextInstanceId(1),
        Handlers()
    {
        EmcpalInitializeListHead(&Queue);
    }

    ~TctlLocus() {
        AcquireLock();
        TestControlHandlerHashElement *p;
        while ((p = Handlers.GetFirst()) != NULL) {
            Handlers.Remove(p);
        }
        ReleaseLock();
    }

    // Provide convenience methods for operating on a locus.

    void        AcquireLock (                  ) {        TctlAcquireLock (this         ); }
    void        ReleaseLock (                  ) {        TctlReleaseLock (this         ); }
    void        InitEnum    (                  ) {        TctlInitEnum    (this         ); }
    void        TermEnum    (                  ) {        TctlTermEnum    (this         ); }
    TctlBuffer* TakeNext    (                  ) { return TctlTakeNext    (this         ); }
    void        GiveOver    (TctlBuffer* buffer) {        TctlGiveOver    (this, buffer ); }
    void*       GetContext  (                  ) { return TctlGetContext  (this         ); }
    void        SetContext  (void* context     ) {        TctlSetContext  (this, context); }

    void        SetDriverId (TctlDriverId driverId)
    {       TctlSetDriverId (this,        driverId); }

    void        SetDeviceForwardingRoutine (TctlDeviceForwardingRoutine routine, void* context)
    {       TctlSetDeviceForwardingRoutine (this, routine, context); }

    void        SetAsDriverLocus (TctlWwidLookupRoutine wwidLookupRoutine, void* wwidLookupContext)
    {       TctlSetAsDriverLocus (this, wwidLookupRoutine, wwidLookupContext); }

    void        SetAsVolumeLocus (TctlWWID* volumeWwid, TctlLocus* driverLocus)
    {       TctlSetAsVolumeLocus (this, volumeWwid, driverLocus); }

    TctlLocus*  GetDriverLocus ()
    {return TctlGetDriverLocus (this); }

    void        SetHandlingLink (TctlLocus* locus)
    {       TctlSetHandlingLink (this, locus); }

    void        HandleTestControlIoctl (BasicIoRequest* irp)
    {       TctlHandleTestControlIoctl (this, irp); }

    TctlHandler DefineOperation
            (TctlFeatureId featureId, TctlOpcodeId opcodeId, TctlHandler handler, void* context)
    {return TctlDefineOperation (this, featureId, opcodeId, handler, context); }

    void        DefineBuiltinOperations ()
    {       TctlDefineBuiltinOperations (this); }


private:  // Prevent copying of the structure.
    TctlLocus (const TctlLocus&);
    const TctlLocus& operator= (const TctlLocus&);

#endif

};


#endif // __TestControlImplDefs__h__
