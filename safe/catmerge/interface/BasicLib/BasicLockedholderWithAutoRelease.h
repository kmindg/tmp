#ifndef __BasicLockedHolderWithAutoRelease_h__
#define __BasicLockedHolderWithAutoRelease_h__

//***************************************************************************
// Copyright (C) EMC Corporation 2002
// Licensed material -- property of EMC Corporation
//**************************************************************************/

//**************************************************************************
//  $Id$
//**************************************************************************

//++
// File Name:
//      BasicLockedHolderWithAutoRelease.h
//
// Contents:
//      Defines the BasicLockedHolderWithAutoRelease class. 
//
// Revision History:
//--

// This class allows meant to be used to create automatic variables on the stack 
// which acquire the object lock for a BasicLockedObject on contruction, and
// which automatically release the lock when the scope is left.
#include "BasicLib/BasicLockedObject.h"
class BasicLockedObjectHolderWithAutoRelease {
public:

    // The default constructor 
    BasicLockedObjectHolderWithAutoRelease(): mLock(NULL) {}

    BasicLockedObjectHolderWithAutoRelease(BasicLockedObject * lock)
    { 
        mLock = lock;
        mLock->AcquireSpinLock(); 
     }

    BasicLockedObjectHolderWithAutoRelease(BasicLockedObjectHolderWithAutoRelease & holder)
    {
        mLock = holder.mLock;
        holder.mLock = NULL;
    }
    
    ~BasicLockedObjectHolderWithAutoRelease() 
    {
        BasicLockedObject * lock = mLock;
        if(lock) {
            mLock = NULL;
            lock->ReleaseSpinLock();
        }
    } 

    void TakeControl(BasicLockedObject * lock) 
    {
        FF_ASSERT(mLock == NULL);
        mLock = lock;
    }

    void RelinquishControl() 
    {
        BasicLockedObject * CSX_MAYBE_UNUSED lock = mLock;
        FF_ASSERT(lock);
        mLock = NULL;
    }

    bool IsHeld() const { return mLock != NULL; }

    void Acquire(BasicLockedObject * lock) 
    { 
        FF_ASSERT(!mLock);

        lock->AcquireSpinLock();
        mLock = lock;
    }

    void Release() 
    {
        BasicLockedObject * lock = mLock;
        FF_ASSERT(lock);
        
        mLock = NULL;
        lock->ReleaseSpinLock();
    }

private:
    BasicLockedObject * mLock;
};


#endif  // __BasicLockedHolderWithAutoRelease_h__

