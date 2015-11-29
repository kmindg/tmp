//***************************************************************************
// Copyright (C) EMC Corporation 2012
//
// Licensed material -- property of EMC Corporation
//**************************************************************************/

#ifndef __BasicMemoryAllocator_h__
#define __BasicMemoryAllocator_h__

//**************************************************************************
//  $Id$
//**************************************************************************

//++
// File Name:
//      BasicMemoryAllocator.h
//
// Contents:
//      Defines the BasicMemoryAllocator class. 
//
// Revision History:
//      04/02/2012 Initial Revision - Bhavesh Patel
//--


// It contains the preallocated memory. It allocates the memory to 
// other objects upon request.
class BasicMemoryAllocator {
public:
    
    // Allocates numBytes memory from the pre-allocated memory.
    //
    // @param numBytes - Number of bytes to allocate.
    //
    // Returns:
    //      pointer to the memory if it suceeded otherwise NULL.
    //
    // Note: This routine is not implemented as each module has
    //      its own requirement for the memory alignment so sub
    //      class has to implement this routine according to its
    //      memory alignment requirement.
    virtual PCHAR getMemory(ULONGLONG numBytes) = 0;

    // Returns remaining number of bytes available memory.
    virtual ULONGLONG availableMemory() const {
        return (mNumBytes - mCursor);
    }

    PCHAR getCurrentMemPtr() const {
        return (mMemory + mCursor);
    }

    virtual ~BasicMemoryAllocator() {}

protected:
    // Constructor
    //
    // @param memory - Pointer to the pre-allocated memory.
    // @param numBytes - Number of bytes contiguous memoy
    //                  pointed by the given memory pointer.
    BasicMemoryAllocator(PCHAR memory, ULONGLONG numBytes) :
        mMemory(memory), mNumBytes(numBytes), mCursor(0) {    
            
    }

    // Pointer to preallocated memory.
    const PCHAR mMemory;

    // Number of bytes contiguous memory pointed by mMemory pointer.
    ULONGLONG mNumBytes;

    // Points to the current available memory location within the memory
    // contains by mMemory pointer.
    ULONGLONG mCursor;

private:
    // Make the default constructor private so it can't be instantiated
    // without specifying proper arguments.
    BasicMemoryAllocator() : mMemory(NULL) {
    }
};


#endif  // __BasicMemoryAllocator_h__

