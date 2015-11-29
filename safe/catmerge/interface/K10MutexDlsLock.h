//
//  File: K10MutexDlsLock.h
//
//  Copyright (C) 2001 EMC Corporation
//

#ifndef K10MUTEX_DLS_LOCK_H
#define K10MUTEX_DLS_LOCK_H
#include "csx_ext.h" // required for CSX_MOD_IMPORT and CSX_MOD_EXPORT

// This is the actual implementation class.
class K10MutexDLSLockImpl;

// This class provides way to lock both the current SP and the entire array
// for a given name.  A given thread can acquire a new lock with the same
// name multiple times, but other threads on the same SP and processes on
// the peer SP will not be able to obtain a lock with the same name.
//
// If using this class with a multithreaded process, each thread should 
// use a unique K10MutexDLSLock instance for a given name.  Sharing an 
// instance of a K10MutexDLSLock object (e.g. global) will NOT result in the
// expected locking behavior.  Threads in the same process that use a
// shared object will not block each other - once one thread acquires
// the lock, the other threads using the same instance will think they
// also have the lock.

class CSX_CLASS_EXPORT K10MutexDLSLock  
{
public:
    // Creates a new lock with the supplied name.
    // Can throw errors.
    K10MutexDLSLock( char * Name );
    // Frees any allocated memory.
    // Can throw errors.
    ~K10MutexDLSLock();

    // Returns true if the lock has been obtained.
    bool IsLocked();    
    // Locks the SP and the array for the name supplied in the constructor.
    // If bWait is set, then it will wait for the lock to be released.
    // if bDualSP_Lock is not set, then only the current SP is locked.
    // Can throw errors.
    HRESULT Lock( bool bWait, bool bDualSP_Lock );
    // ulTimeout sets an executioner timeout (specified in minutes) on the
    // DLS lock, such that the peer SP will panic if it doesn't release the 
    // lock before the timer expires.
    HRESULT Lock( bool bWait, bool bDualSP_Lock, ULONG ulTimeout );
    // Releases the lock.
    // Can throw errors.
    void Unlock( );

private:
    K10MutexDLSLockImpl * mpLockImpl;
};

#endif // K10MUTEX_DLS_LOCK_H
