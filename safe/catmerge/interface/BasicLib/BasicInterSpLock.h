//***************************************************************************
// Copyright (C) EMC Corporation 2009
// Licensed material -- property of EMC Corporation
//**************************************************************************/

//**************************************************************************
//  $Id$
//**************************************************************************

//++
// File Name:
//      BasicInterSpLock.h
//
// Contents:
//      Defines the BasicInterSpLock class. This class is used for distributed
//      locks between SP's. 
//  
//      Current behavior of this gets the exclusive access to the DLS lock. It
//      will not be shared across the threads on the same SP. This is what 
//      used for SPCache. If this requirement changes then this class needs
//      to be updated.
//
// Revision History:
//     4-Dec-2009     Created.   -Richard Chandler
//     31-Aug-2011   Modified - Bhavesh Patel
//
//--

#if !defined(_intersplock_h)
#define _intersplock_h

#include <string.h>
#include "BasicBinarySemaphore.h"

extern "C" {
#include "DlsTypes.h"
#include "DluSyncLock.h"
}

class BasicInterSpLock {

public:

    BasicInterSpLock() : mCallback(BasicInterSpLockCallback) {}

    // Initialize the lock with the given DLS domain name and monicker.
    //
    // @param domainName - The underlying Dlu lock's domain name.
    // @param moniker - The underlying Dlu lock's monicker.
    void Initialize(const char *domainName, const char *moniker);

    // Acquire the lock in exclusive mode.
    // Note: shared inter-SP locking is not supported.
    void AcquireLock();

    // Release the lock.
    void ReleaseLock();

    // Close the lock.
    // Called when a client is done with it.
    void CloseLock();

private:

    DLS_LOCK_NAME mLockName;         // Unique name for this lock

    DLS_LOCK_CALLBACKFUNC mCallback; // Callback function receives notifica-
                                     // tions if there are lock conflicts.

    DLU_SYNC_LOCK mLock;             // The underlying distributed lock.

    // Binary semaphore to locally synchronize the access to the inter sp lock.
    BasicBinarySemaphore mBinarySemaphore;

    // Function called if there are lock conflicts.
    // Because the underlying implementation uses a DLU_SYNC_LOCK, there is
    // no need to complete asynchronous operations.
    //
    // @param event - The event that triggered the callback.
    // @param data - The "this" pointer to the inter-SP lock.
    // Returns:
    //   STATUS_SUCCESS
    static EMCPAL_STATUS BasicInterSpLockCallback(DLS_EVENT event, PVOID data);

};

#endif
