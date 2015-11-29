/************************************************************************
 *
 *  Copyright (c) 2002, 2005-2006 EMC Corporation.
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
 ************************************************************************/

#ifndef __AllocatedMemoryTracker_h__
#define __AllocatedMemoryTracker_h__

//++
// File Name: AllocatedMemoryTracker.h
//    
//
// Contents:
//      Defines the AllocatedMemoryTracker class.
//
// Revision History:
//--

# include "K10AnsiString.h"
# include "BasicVolumeDriver/BasicVolumeDriver.h"
# include "BasicLib/BasicObjectTracer.h"

class AllocatedMemoryTracker : public BasicObjectTracer {
public:

    // Creates a new AllocatedMemoryTracker.
    //
    // @param name - The name of the memory which we are to track.
    // @param size - The maximum size that the allocated memory can achieve (in bytes).
    // @param driverName - Trace name of the driver.
    AllocatedMemoryTracker(const char * name, ULONG size, const char* driverName);

    // Destructs the AllocatedMemoryTracker.
    virtual ~AllocatedMemoryTracker();

    // Add memory to the memory tracker.
    // 
    // @param pointer - the pointer to the memory being added.
    // @param size - the size of the memory being added (in bytes).
    //
    // Returns true if successful, false when pointer is NULL.
    bool AddMemToTracker(PVOID pointer, ULONG size);

    // Remove memory from the memory tracker.
    // 
    // @param pointer - the pointer to the memory being removed.
    // @param size - the size of the memory being removed (in bytes).
    //
    // Returns true if successful, false when pointer is NULL.
    bool RemoveMemFromTracker(PVOID pointer, ULONG size);

    // Acquire the spin lock associated with the allocated memory tracker.
    void AcquireSpinLock() { mLock.Acquire();}

    // Release the spin lock associated with the allocated memory tracker.
    void ReleaseSpinLock() { mLock.Release();}

protected:

    // The size of the allocated memory.
    ULONG           mAllocMemSize;

    // The maximum size that the allocated memory can achieve.
    ULONG           mMaxAllocMemSize;

    // The name of the memory that we are tracking.
    const char    * mAllocMemName;

    // Used to track whether we've exceeded the maximum allocation size.
    bool            mMaxAllocMemSizeExceeded;

    // Protects the allocated memory tracker.
    K10SpinLock     mLock;

private:

    // Internal logging function, could be overridden to change the trace prefix, etc.
    //
    // @param ring - the ktrace ring buffer to log to
    // @param format - the printf format string
    // @param args - the format arguments.
    virtual VOID TraceRing(KTRACE_ring_id_T ring, const char * format, va_list args)  const __attribute__ ((format(__printf_func__, 3, 0)));

    virtual KTRACE_ring_id_T  DetermineTraceBuffer(BasicTraceLevel level) const;

    // Driver's trace name which we used for tracing.
    const char* mDriverTraceName;

};


#endif  // __AllocatedMemoryTracker_h__

