#ifndef __BasicLockedObject_h__
#define __BasicLockedObject_h__

//***************************************************************************
// Copyright (C) EMC Corporation 2002-2013
// Licensed material -- property of EMC Corporation
//**************************************************************************/

//**************************************************************************
//  $Id$
//**************************************************************************

//++
// File Name:
//      BasicLockedObject.h
//
// Contents:
//      Defines the BasicLockedObject class. 
//
// Revision History:
//--
# include "K10SpinLock.h"

// This base class provides a paradigm for object level locking.
// Public methods on the sub-classes are expected to get the lock on entrance,
// and release it on exit.
//
// BasicLockedObjectHolderWithAutoRelease is meant to work in conjuction with this
// base class. Each public function of a BasicLockedObject sub-class could have the first line be:
//      BasicLockedObjectHolderWithAutoRelease   lockHolder(this);
//
// If nothing else is done, then this lock would be released on return, when lockHolder goes out of scope.
//
// 
class BasicLockedObject {
public:
    void AcquireSpinLock() { mSpinlock.Acquire(); }
    void ReleaseSpinLock() { mSpinlock.Release(); }
    void AssertLockHeld() { mSpinlock.AssertLockHeld(); }
    BOOL TryAcquireSpinLock() {return mSpinlock.TryAcquire(); }

    BasicLockedObject() {}
    
    ~BasicLockedObject() {};

private:
    // Make the copy constructor private, because the semantics of copy construction are unclear.
    BasicLockedObject(BasicLockedObject &);

    K10SpinLock   mSpinlock;
};

#endif  // __BasicLockedObject_h__

