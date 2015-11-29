//******************************************************************************
//      COMPANY CONFIDENTIAL:
//      Copyright (C) EMC Corporation, 2010
//      All rights reserved.
//      Licensed material - Property of EMC Corporation.
//******************************************************************************

//*****************************************************************************
//  FILE NAME:
//      RangeLock.h
//
//  DESCRIPTION:
//      Declare Range Locking Enhancement Interface.
//
//******************************************************************************

#ifndef _RANGE_LOCK_H_
#define _RANGE_LOCK_H_

#if defined(__cplusplus)
extern "C" {
#endif

// The unit can be compiled under either user mode or kernel mode
#if defined(UMODE_ENV)
#include "windef.h"
#else
#include "k10ntddk.h"
#endif

#include "csx_ext.h" 

typedef enum 
{
    // Indicate operation is completed successfully
    RWRL_SUCCESS            = 0,

    // Function is called with invalid parameter
    RWRL_INVALID_PARAMETER  = 1,

    // Operation requires the request not to be locked, but it is locked now
    RWRL_ALREADY_LOCKED     = 2,

    // Operation cannot be completed
    RWRL_UNAVAILABLE        = 3,
} rwrlStatus;  

typedef enum 
{
    // The lock request is initialized
    RWRL_REQUEST_IDLE                       = 0,

    // The lock request currently conflicts with an existing granted range 
    // request and this request has registered a callback to be invoked
    RWRL_REQUEST_CONFLICT                   = 1,

    // The lock request currently is granted
    RWRL_REQUEST_LOCKED                     = 2,

    // The lock request currently is granted, notification callback to be 
    // invoked
    RWRL_REQUEST_LOCKED_CB_OUTSTANDING      = 3,
} rwrlRequestState;


struct _K10_MISC_UTIL_RW_RANGE_LOCK;
struct _K10_MISC_UTIL_RW_RANGE_LOCK_REQUEST;
typedef void (*K10_MISC_UTIL_RW_RANGE_LOCK_REQUEST_CALLBACK)
(
    struct _K10_MISC_UTIL_RW_RANGE_LOCK            *pLock,
    struct _K10_MISC_UTIL_RW_RANGE_LOCK_REQUEST    *pRequest,
    void                                           *pContext,
    BOOLEAN                                         bCancelled
); 

typedef struct  _K10_MISC_UTIL_RW_RANGE_LOCK_TREE_NODE
{
    struct _K10_MISC_UTIL_RW_RANGE_LOCK_TREE_NODE  *parent;
    struct _K10_MISC_UTIL_RW_RANGE_LOCK_TREE_NODE  *llink;
    struct _K10_MISC_UTIL_RW_RANGE_LOCK_TREE_NODE  *rlink;
} 
K10_MISC_UTIL_RW_RANGE_LOCK_TREE_NODE, *PK10_MISC_UTIL_RW_RANGE_LOCK_TREE_NODE;

// 
// End LBA of shared range group. Recorded in the leader of the shared group
// An optimization to speed up check against the range group
//
typedef struct  _K10_MISC_UTIL_RW_RANGE_LOCK_SHARED_GROUP_INFO
{
    ULONGLONG                                       group_lba_end;
} 
K10_MISC_UTIL_RW_RANGE_LOCK_SHARED_GROUP_INFO;

typedef struct _K10_MISC_UTIL_RW_RANGE_LOCK_REQUEST 
{
    //
    // Information from user request
    //
    ULONGLONG                                       lba_start;
    ULONGLONG                                       lba_end;
    BOOLEAN                                         is_exclusive;

    //
    // RWRL_REQUEST_IDLE:
    //      When initialized, or range released
    // 
    // RWRL_REQUEST_CONFLICT:
    //      Pending lock whose callback will be invoked
    // 
    // RWRL_REQUEST_LOCKED:
    //      Range granted, check with is_exclusive
    //
    // RWRL_REQUEST_LOCKED_CB_OUTSTANDING:
    //      Range granted, notification callback to be invoked
    rwrlRequestState                                state;
    
    //
    // Valid only when state is RWRL_REQUEST_CONFLICT
    //
    K10_MISC_UTIL_RW_RANGE_LOCK_REQUEST_CALLBACK    callback;
    void                                           *context;

    //
    // If state is RWRL_REQUEST_CONFLICT
    // The pointer to master conflict request
    //
    struct _K10_MISC_UTIL_RW_RANGE_LOCK_REQUEST    *master_conflict_reqeust;

    //
    // If state is RWRL_REQUEST_LOCKED, start of list of conflict requests.
    // Conflict requests are chained through this field
    //
    struct _K10_MISC_UTIL_RW_RANGE_LOCK_REQUEST    *next_conflict_request;

    //
    // Back pointer to lock object
    // When state is NOT RWRL_REQUEST_IDLE
    //
    struct _K10_MISC_UTIL_RW_RANGE_LOCK            *master_lock;

    //
    // Tree traverse link 
    //
    K10_MISC_UTIL_RW_RANGE_LOCK_TREE_NODE           tree_node;

    //
    // Contain the information of a node in shared_tree
    // This field is valid in the first request in a shared group: group leader
    //
    K10_MISC_UTIL_RW_RANGE_LOCK_SHARED_GROUP_INFO   shared_group_info;

    // 
    // The next request in a shared group
    //
    struct _K10_MISC_UTIL_RW_RANGE_LOCK_REQUEST    *next_shared_request_in_group;

    //
    // The pointer to the leader of the shared group
    //
    struct _K10_MISC_UTIL_RW_RANGE_LOCK_REQUEST    *group_leader;

    //
    // Use this field to link the request from conflict_list in lock object
    //
    csx_dlist_entry_t                               conflict_node;
    

} K10_MISC_UTIL_RW_RANGE_LOCK_REQUEST, 
 *PK10_MISC_UTIL_RW_RANGE_LOCK_REQUEST;

typedef 
void
(*K10_MISC_UTIL_RW_RANGE_LOCK_ACQUIRE_MUTEX_CALLBACK)(
    struct _K10_MISC_UTIL_RW_RANGE_LOCK            *pLock,
    PVOID                                           pMutex);

typedef 
void
(*K10_MISC_UTIL_RW_RANGE_LOCK_RELEASE_MUTEX_CALLBACK)(
    struct _K10_MISC_UTIL_RW_RANGE_LOCK            *pLock,
    PVOID                                           pMutex);

typedef 
void
(*K10_MISC_UTIL_RW_RANGE_LOCK_ASSERT_FAIL_CALLBACK)(
    struct _K10_MISC_UTIL_RW_RANGE_LOCK            *pLock,
    char*                                           pExpression,
    char*                                           pFileName,
    int                                             iLineNum);

typedef struct _K10_MISC_UTIL_RW_RANGE_LOCK 
{
    //
    // The abstraction of mutex, it will be passed back to 
    // acquire_mutex / release_mutex.
    // Initialized by K10MiscUtilRWRangeLockInitialize
    //
    PVOID                                               mutex;

    //
    // Pointer to the root of exclusive tree, NULL if tree is empty
    //
    PK10_MISC_UTIL_RW_RANGE_LOCK_TREE_NODE              exclusive_tree;

    //
    // Pointer to the root of shared tree, NULL if tree is empty
    //
    PK10_MISC_UTIL_RW_RANGE_LOCK_TREE_NODE              shared_tree;

    //
    // Anchor to link requests with field conflict_node
    //
    csx_dlist_head_t                                    conflict_list;

    //
    // User provided function.  Will be called to acquire mutex
    //
    K10_MISC_UTIL_RW_RANGE_LOCK_ACQUIRE_MUTEX_CALLBACK  acquire_mutex;

    //
    // User provided function.  Will be called to release mutex
    //
    K10_MISC_UTIL_RW_RANGE_LOCK_RELEASE_MUTEX_CALLBACK  release_mutex;

    // 
    // User provided function.  Will be called to notify user if an 
    // internal assertion is violated
    //
    K10_MISC_UTIL_RW_RANGE_LOCK_ASSERT_FAIL_CALLBACK    assert_fail;
} 
K10_MISC_UTIL_RW_RANGE_LOCK, *PK10_MISC_UTIL_RW_RANGE_LOCK; 

//
// Interface Functions
//

//
// Range Lock functions
//

//++
// Name:
//      K10MiscUtilRWRangeLockInitialize
//
// Description:
//      Call this routine to initialize range lock object
//
// Arguments:
//      pLock
//          The address of K10_MISC_UTIL_RW_RANGE_LOCK structure to be 
//          initialized.
//
//      pMutex
//          The pointer will be passed back when user provided 
//          pAcquireMutexCallback / pReleaseMutexCallback is called.  
//          It is not necessarily a pointer to a Mutex type object. 
//          User is free to choose any value which will be passed back via 
//          pAcquireMutexCallback / pReleaseMutexCallback
//
//      pAcquireMutexCallback
//          User provided function. Will be called when range lock object needs 
//          to acquire exclusive access to the object
//      
//      pReleaseMutexCallback
//          User provided function. Will be called when range lock object 
//          needs to release exclusive access to the object
//
//      pAssertFail
//          User provided function. Will be called when range lock object 
//          detects an internal violation and needs to notify the user
//
// Returns:
//      RWRL_SUCCESS
//          If range lock object is successfully initialized 
//
//      RWRL_INVALID_PARAMETER 
//          If the initialization of range lock object fails 
//
//--
rwrlStatus 
K10MiscUtilRWRangeLockInitialize(
    PK10_MISC_UTIL_RW_RANGE_LOCK                        pLock,
    PVOID                                               pMutex,
    K10_MISC_UTIL_RW_RANGE_LOCK_ACQUIRE_MUTEX_CALLBACK  pAcquireMutexCallback,
    K10_MISC_UTIL_RW_RANGE_LOCK_RELEASE_MUTEX_CALLBACK  pReleaseMutexCallback,
    K10_MISC_UTIL_RW_RANGE_LOCK_ASSERT_FAIL_CALLBACK    pAssertFail);


//++
// Name:
//      K10MiscUtilRWRangeLockUninitialize
//
// Description:
//      Call this routine to uninitialize range lock object
//
// Arguments:
//      pLock
//          The address of K10_MISC_UTIL_RW_RANGE_LOCK structure to be 
//          uninitialized.
//
// Returns:
//      RWRL_SUCCESS
//          If there are no granted range request
// 
//      RWRL_INVALID_PARAMETER
//          Otherwise
//
//--
rwrlStatus 
K10MiscUtilRWRangeLockUninitialize(
    PK10_MISC_UTIL_RW_RANGE_LOCK                    pLock );


//
// Range Lock Request functions
//

//++
// Name:
//      K10MiscUtilRWRangeLockInitializeRequest
//
// Description:
//      Call this routine to initialize range lock request object
//
// Arguments:
//      pRequest
//          The address of K10_MISC_UTIL_RW_RANGE_LOCK_REQUEST to be 
//          initialized.
//
// Returns:
//      None
//
//--
void 
K10MiscUtilRWRangeLockInitializeRequest(
    PK10_MISC_UTIL_RW_RANGE_LOCK_REQUEST            pRequest);


//++
// Name:
//      K10MiscUtilRWRangeLockAcquireLock
//
// Description:
//      Call this routine to try to acquire a range
//
// Arguments:
//      pLock
//          The address of K10_MISC_UTIL_RW_RANGE_LOCK object which is 
//          responsible to arbitrate lock request to gain access to LBA range
//
//      pRequest 
//          The address of K10_MISC_UTIL_RW_RANGE_LOCK_REQUEST which has been 
//          initialized.  Whether the access is exclusive or shared is 
//          based on the flag bExclusive.
//
//      StartLBA
//          The starting logical block address of the request
//  
//      EndLBA
//          The ending logical block address of the request. 
//          Can be calculated by EndLBA = StartLBA + LengthOfRequest - 1
//          if StartLBA and LengthOfRequest are known
//  
//      bExclusive
//          Whether access to the range should be exclusive or shared.
//          TRUE if access is intended to be exclusive.
//          FALSE if access is intended to be shared.
//
// Returns:
//      RWRL_SUCCESS
//          If there is no overlap and the lock is granted
//          No callback will be invoked
//
//      RWRL_INVALID_PARAMETER
//          If EndLBA is less than StartLBA,
//          OR this request belongs to another range lock object,
//          OR this request does not belong to any range lock object but its 
//              internal state is invalid,
//          OR the request belongs this range lock object but its not granted
//          No callback will be invoked
//      
//      RWRL_ALREADY_LOCKED
//          If this request is already granted by this range lock object
//          No callback will be invoked
//
//      RWRL_UNAVAILABLE 
//          If there is an overlap with another lock request and lock is denied.  
//          Will call Callback when set
//
//--
rwrlStatus 
K10MiscUtilRWRangeLockAcquireLock(
    PK10_MISC_UTIL_RW_RANGE_LOCK                    pLock,
    PK10_MISC_UTIL_RW_RANGE_LOCK_REQUEST            pRequest,
    ULONGLONG                                       StartLBA,
    ULONGLONG                                       EndLBA, 
    BOOLEAN                                         bExclusive,
    K10_MISC_UTIL_RW_RANGE_LOCK_REQUEST_CALLBACK    Callback,
    void                                           *pContext);


//++
// Name:
//      K10MiscUtilRWRangeLockReleaseLock
//
// Description:
//      Call this routine to release a range lock
//
// Arguments:
//      pLock
//          The address of K10_MISC_UTIL_RW_RANGE_LOCK which manages the request.
//
//      pRequest 
//          The address of K10_MISC_UTIL_RW_RANGE_LOCK_REQUEST to be released
//
// Returns:
//      RWRL_SUCCESS 
//          If lock is successfully released.
//
//      RWRL_INVALID_PARAMETER
//          If any of
//              This lock request does not belong to this range lock object
//              This lock request is not already locked
//--
rwrlStatus 
K10MiscUtilRWRangeLockReleaseLock(
    PK10_MISC_UTIL_RW_RANGE_LOCK                    pLock,
    PK10_MISC_UTIL_RW_RANGE_LOCK_REQUEST            pRequest);


//++
// Name:
//      K10MiscUtilRWRangeLockCancelLock
//
// Description:
//      Call this routine to cancel a range lock
//
// Arguments:
//      pLock
//          The address of K10_MISC_UTIL_RW_RANGE_LOCK which manages the request.
//
//      pRequest 
//          The address of K10_MISC_UTIL_RW_RANGE_LOCK_REQUEST to be cancelled
//
// Returns:
//      RWRL_SUCCESS
//          If range lock request is notified to be cancelled.
//
//      RWRL_UNAVAILABLE
//          If this range lock does not conflicts with an existing request 
//          object
// 
//--
rwrlStatus 
K10MiscUtilRWRangeLockCancelLock(
    PK10_MISC_UTIL_RW_RANGE_LOCK                    pLock,
    PK10_MISC_UTIL_RW_RANGE_LOCK_REQUEST            pRequest);


//++
// Name:
//      K10MiscUtilRWRangeLockConvertExclusiveLockToShared
//
// Description:
//      Call this routine to convert an exclusive range lock to be shared
//
// Arguments:
//      pLock
//          The address of K10_MISC_UTIL_RW_RANGE_LOCK which manages the request.
//
//      pRequest 
//          The address of K10_MISC_UTIL_RW_RANGE_LOCK_REQUEST to be changed 
//          from exclusive lock to shared lock
//
// Returns:
//      RWRL_SUCCESS
//          If lock is successfully converted from exclusive access to shared 
//          access.
//      
//      RWRL_NOT_LOCKED
//          If the lock request given is not actually holding a range lock.
//
//      RWRL_INVALID_PARAMETER 
//          If any of
//              This lock request does not belong to this range lock object
//              This lock request is not already locked
//              This lock request is granted as shared
// 
//--
rwrlStatus 
K10MiscUtilRWRangeLockConvertExclusiveLockToShared(
    PK10_MISC_UTIL_RW_RANGE_LOCK                    pLock,
    PK10_MISC_UTIL_RW_RANGE_LOCK_REQUEST            pRequest);


//++
// Name:
//      K10MiscUtilRWRangeLockConvertSharedLockToExclusive
//
// Description:
//      Call this routine to convert a shared range lock to be exclusive
//
// Arguments:
//      pLock
//          The address of K10_MISC_UTIL_RW_RANGE_LOCK which manages the request.
//
//      pRequest 
//          The address of the range request to be acquired exclusively. 
//
// Returns:
//      RWRL_SUCCESS 
//          If range request is granted.
//
//      RWRL_UNAVAILABLE
//          If range request cannot be granted due to LBA conflict.
//
//--
rwrlStatus 
K10MiscUtilRWRangeLockConvertSharedLockToExclusive(
    PK10_MISC_UTIL_RW_RANGE_LOCK                    pLock,
    PK10_MISC_UTIL_RW_RANGE_LOCK_REQUEST            pRequest);


#if defined(__cplusplus)
}
#endif

#endif // _RANGE_LOCK_H_

// End RangeLock.h
