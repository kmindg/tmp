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

#ifndef __RequiredMemoryCalculator_h__
#define __RequiredMemoryCalculator_h__

//++
// File Name: RequiredMemoryCalculator.h
//    
//
// Contents:
//      Defines the RequiredMemoryCalculator class.
//
// Revision History:
//--



// defines

// FIX: We shouldn't have to hard code these values. CMM should provide us with an
// interface to get the right size.

#define CMM_MEMORY_DESCRIPTOR_SIZE 128

#define CMM_ALLOCATION_UNIT_SIZE 64
#define CMM_SMALL_ALLOC_DESCRIPTOR_SIZE 64
#define OS_SMALL_ALLOC_HEADER_SIZE 32 // The OS header is the in the 8 bytes before the pointer you get from ExAllocatePool().
#define SMALL_ALLOC_HEADER_SIZE (CMM_SMALL_ALLOC_DESCRIPTOR_SIZE + OS_SMALL_ALLOC_HEADER_SIZE)

class RequiredMemoryCalculator {
public:

    // Creates a new RequiredMemoryCalculator.
    //
    // @param name - The name of the memory whose size we are calculating.
    // @param CmmMemSize - The preliminary size of Cmm memory already accounted for.
    // @param WdmMemSize - The preliminary size of Wdm memory already accounted for.
    RequiredMemoryCalculator(const char * name, ULONGLONG CmmMemSize, ULONGLONG WdmMemSize);

    // Destructs the RequiredMemoryCalculator.
    ~RequiredMemoryCalculator();

    // Adds the passed in size of the memory and the count to the total.
    //
    // @param numberOfAllocations - The number of allocations.
    // @param numberOfBytesPerAllocation - The number of bytes per allocation.
    //
    // Returns the true size of numberOfBytesPerAllocation if Cmm were to allocate it.
    ULONG AddMemToTotal(ULONG numberOfAllocations, ULONG numberOfBytesPerAllocation);

    // Returns the size of the Cmm memory we have calculated.
    ULONGLONG GetCmmMemTotal() { return mTotalCmmMem; }

    // Returns the size of the Wdm memory we have calculated.
    ULONGLONG GetWdmMemTotal() { return mTotalWdmMem; }

    // Adds an arbitrary amount of memory into our total for Wdm memory.
    //
    // Note: Should only be used when an allocation will bypass Cmm.
    VOID AddToWdmMem(ULONGLONG size) { mTotalWdmMem += size; }

protected:

    // The name of the memory whose size we are calculating.
    const char    * mMemoryName;

    // The total amount of Cmm memory that has been summed together from all of the calls
    // to AddToTotal().
    ULONGLONG       mTotalCmmMem;

    // The total amount of Wdm memory that has been summed together from all of the calls
    // to AddToTotal().
    ULONGLONG       mTotalWdmMem;
};


#endif  // __RequiredMemoryCalculator_h__

