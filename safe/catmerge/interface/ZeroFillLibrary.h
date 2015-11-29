
//***************************************************************************
// Copyright (C) EMC Corporation 2009
// All rights reserved.
// Licensed material -- property of EMC Corporation
//**************************************************************************/

#ifndef _ZERO_FILL_LIB_H_
#define _ZERO_FILL_LIB_H_

//++
// File Name:
//      ZeroFillLibrary.h
//
// Contents:
//      prototypes for Zero Fill library interface routines
//
// Exported:
//
// Revision History:
//      10/24/2008    Prashanth Adurthi   Created
//
//--

// Include Files
//--

#include "k10ntddk.h"
#include "EmcPAL_DriverShell.h"
#include "k10defs.h"
#include "core_config.h"
#include "flare_ioctls.h"
#include "K10Debug.h"
#include "klogservice.h"
#include "flare_export_ioctls.h"

//
//  This defines the maximum length of the
//  Client Id  that the client should provide
//  the Zero fill library while calling the initialization
//  routine UtilInitializeZeroFillLibrary
//
#define ZF_CLIENT_ID_LENGTH    3

//
//  This new type defines the prototype for the callback routine
//  that the client has to supply to the Zero Fill library
//                                                                                             
typedef EMCPAL_STATUS (*PZERO_FILL_CALL_BACK) 
        ( IN PEMCPAL_DEVICE_OBJECT DeviceObject, IN PEMCPAL_IRP Irp, IN PVOID Context);



//++
// Type:
//      ZERO_FILL_CONTEXT
//
// Description:
//      This structure is managed internally by the Zero Fill library
//      to track a Zero Fill IOCTL via multiple iterations. Zero Fill
//      library internally uses this structure as the context variable
//      while submitting Zero Fill IRPs. Note that each Zero Fill IOCTL 
//      issued is associated with one instance of this structure.
//
// Members:
//
//      Signature   -   This variable is used to determine whether the 
//                      IRP initialization routine is called or not.
//
//      ListEntry   -   List entry to hang this context off of the work
//                      queue that the library uses while re-issuing 
//                      Zero Fills.
//
//      StartLba    -   This variable will always contains the original
//                      StartLba value  that the client passed while
//                      submitting Zero Fill to the library
//
//      BlocksToZero-   This variable will always contains the orignal
//                      value of total number of blocks that have to be
//                      Zeroed.
//
//      PrevStartLba-   This variable is used to remember the startLba 
//                      used in previous (just completed) iteration.
//
//      PrevBlocksToZero-   This variable is used to remember the number
//                      of blocks that we intended to Zero in previous(just completed)
//                      iteration.
//
//      ClientCookie-   This is the context variable that the client 
//                      passed to the Zero Fill library while submitting
//                      the Zero Fill.
//
//      ZeroFillLock-   Zero Fill library uses this lock to synchronize
//                      the access between IoReuseIrp and IoCancelIrp. 
//                      This lock is only  used required in case of clients
//                      which created/initiated the Zero Fill IRP.
//
//      pZeroFillIrp-   The actual Zero Fill IRP that the client
//                      submitted.
//
//      pDevObj     -   The device object to which the Zero Fill has to
//                      be issued.
//
//      pClientDevObj   - The device object of the client.
//
//      IsIrpClientCreated  -   This BOOLEAN is set if the Client is the
//                              creator/initiator of the Zero Fill IRP.
//
//      ClientCancelled -   This BOOLEAN is set if the client uses
//                          UtilCancelZeroFill to cancel the IRP.
//                          Note that the clients which create/initiate
//                          the Zero Fill IRP should use UtilCancelZeroFill
//                          to cancel the IRP. They should not use IoCancelIrp.
//
//      ClientCallBack  -   This the call back routine that the client
//                          submits to the ZeroFill library while
//                          initializing the library.
//
// Revision History:
//      10/24/08   Prashanth Adurthi    Created.
//
//--
typedef struct _ZERO_FILL_CONTEXT
{
    ULONG           Signature;
    EMCPAL_LIST_ENTRY      ListEntry;
    ULONGLONG       StartLba;             
    ULONGLONG       BlocksToZero;               
    ULONGLONG       PrevStartLba;             
    ULONGLONG       PrevBlocksToZero;               
    PVOID           ClientCookie;                      
    EMCPAL_SPINLOCK      ZeroFillLock;
    PEMCPAL_IRP            pZeroFillIrp;
    PEMCPAL_DEVICE_OBJECT  pDevObj;  
    PEMCPAL_DEVICE_OBJECT  pClientDevObj;
    BOOLEAN         IsIrpClientCreated;
    BOOLEAN         ClientCancelled; 
    PZERO_FILL_CALL_BACK    ClientCallBack;
    volatile LONG           ZeroFillLockReferenceCount;
}ZERO_FILL_CONTEXT, *PZERO_FILL_CONTEXT;
// .End ZERO_FILL_CONTEXT


//
//  This macro defines the memory that the client needs to 
//  provide to the library while issuing the Zero Fill IRP
//  
//  The same value can be obtained by calling UtilGetZeroFillIssueMemorySize
//  routine defined below.
//
#define UTIL_ZERO_FILL_ISSUE_MEMORY_SIZE sizeof(ZERO_FILL_CONTEXT)



//
//  Zero Fill library routines
//  See ZeroFillLibrary.c for a description of these routines
//

#ifdef __cplusplus
extern "C"
#endif
SIZE_T UtilGetZeroFillInitMemorySize (
    VOID
    );

#ifdef __cplusplus
extern "C"
#endif
SIZE_T UtilGetZeroFillIssueMemorySize (
    VOID
    );

#ifdef __cplusplus
extern "C"
#endif
EMCPAL_STATUS
UtilInitializeZeroFillLibrary (
    IN  PVOID   pZFData,
    IN  PCHAR   pClientId
    );

#ifdef __cplusplus
extern "C"
#endif
EMCPAL_STATUS
UtilInitClientCreatedZeroFillIrp (
    IN  PEMCPAL_IRP    pZFIrp,
    IN  PFLARE_ZERO_FILL_INFO   pFlareZeroFillInfo,
    IN  ULONGLONG   StartLba,
    IN  ULONGLONG   BlocksToZero,
    IN  PVOID   clientCookie,
    IN  PZERO_FILL_CALL_BACK ClientCallBack ,
    IN  PVOID   pZeroFillMemory
    );

#ifdef __cplusplus
extern "C"
#endif
EMCPAL_STATUS
UtilInitNonClientCreatedZeroFillIrp (
    IN  PEMCPAL_IRP    pZFIrp,
    IN  PVOID   clientCookie,
    IN  PZERO_FILL_CALL_BACK ClientCallBack,
    IN  PVOID   pZeroFillMemory
    );

#ifdef __cplusplus
extern "C"
#endif
EMCPAL_STATUS
UtilIssueZeroFillIrp (
    IN  PEMCPAL_DEVICE_OBJECT pConsumedDevObj,
    IN  PVOID pZeroFillMemory
    );

#ifdef __cplusplus
extern "C"
#endif
EMCPAL_STATUS
UtilCancelZeroFillIrp (
    IN  PVOID pZeroFillMemory
    );

#ifdef __cplusplus
extern "C"
#endif
VOID
UtilUnloadZeroFillLibrary(void
    );

#endif  //_ZERO_FILL_LIB_H_

// End ZeroFillLibrary.h



