/***************************************************************************
 * Copyright (C) EMC Corporation 2006
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/
/***************************************************************************
 * FILE: CmiPeerMem.h
 *
 * DESCRIPTION: 
 *  This file contains definitions for Local Only memory ranges
 *  on various platforms.
 *
 * Macros:
 *  cmiIsPeerPhysMemAccessible()
 *  cmiIsPeerVirtMemAccessible()
 *  
 * NOTES:
 *  Due to hardware errata on hammer family arrays, certain memory
 *  ranges are not accessible on the peer SP via CMI. Those memory
 *  ranges are defined here, with macros that will return if a
 *  physical memory address falls within those ranges.
 *
 *  These ranges are also defined as "Local Only" pools in CMM by 
 *  CMM_SGL cmmLocalOnlyMemory[] in \catmerge\services\cmm\cmm_init.c
 *
 * HISTORY:
 *   19-Dec-06: Tom Rivera -- Created
 **************************************************************/
# if !defined(__CmiPeerMem_h__)
#define __CmiPeerMem_h__ 1


/******************
 *  Includes
 ******************/
 
#include "cmm.h"

/******************
 *  Definitions
 ******************/
 

/******************
 *  Macros
 ******************/
// Platform specific macros to determine whether a fixed data
// transfer to the peer SP is possible to a particular address.


// Determine if the addresss range has any special DMA restrictions.
//
// @param vaddr - the starting system virtual address of the range
// @param byteCount - the number of bytes in the range.
//
// Returns: true if this SP will allow the peer CMI to DMA into  address range specifies.
__inline BOOLEAN cmiIsPeerVirtMemAccessibleForFixedDataTransfer(PVOID vaddr, ULONG byteCount);

// Same on all current platforms: must be word aligned.
#define cmiIsPeerPhysMemAlignedForFixedDataTransfer(_vaddr, _byteCount) ((((ULONG_PTR)_vaddr | _byteCount) & 3) == 0)

#define cmiIsPeerPhysMemAccessibleForFixedDataTransfer(_addr) TRUE

__inline BOOLEAN cmiIsPeerVirtMemAccessibleForFixedDataTransfer(PVOID vaddr, ULONG byteCount)
{
    // Make sure that if its in the right pool, it is properly aligned for DMA.
    if (!cmiIsPeerPhysMemAlignedForFixedDataTransfer(vaddr, byteCount) ) {
        return false;
    }
    return true;
}


# endif
