#ifndef __BasicPartitionedFreeList_h__
#define __BasicPartitionedFreeList_h__

//***************************************************************************
// Copyright (C) EMC Corporation 2010-2014
// Licensed material -- property of EMC Corporation
//**************************************************************************/

//**************************************************************************
//  $Id$
//**************************************************************************

//++
// File Name:
//      BasicPartitionedFreeList.h
//
// Contents:
//      Defines the BasicPartitionedFreeList class. 
//
// Revision History:
//--

# include "BasicLib/BasicLockedFreeList.h"
# include "csx_ext_p_core.h"

// Provides a free list mechanism that is distributed over multiple cores.  
// Each CPU # has affinity to one of the internal free lists, and adds
// all items to that list.  It will try that list first on allocation, but on failure it will allocate from other lists.
// @param Type - the type of the item in the free list
// @param ListOfType - must provide single element AddHead/RemoveHead methods.
template <class Type, class ListOfType>
class BasicPartitionedFreeList {
public:

    BasicPartitionedFreeList() {
        csx_p_sys_info_t  sys_info;
        csx_p_get_sys_info(&sys_info);
        mNumCpus = sys_info.os_cpu_count;
        p = new Freelist[mNumCpus];
        FF_ASSERT(p);
    }

    // Returns the number of bytes that this class dynamically allocates
    // when it is instantiated.
    static ULONG GetAllocationSizeBytes() {
        csx_p_sys_info_t  sys_info;
        csx_p_get_sys_info(&sys_info);
        csx_nuint_t cpuCnt = sys_info.os_cpu_count;
        return (sizeof(Freelist) * cpuCnt);
    }

    // Returns the number of items currently free.
    // Expensive because of cache misses if other cores are using this.  Data is stale before returning.
    ULONGLONG GetNumFree() const { 
        ULONGLONG sum = 0;

        for (ULONG i=0; i < mNumCpus; i++) {

            sum += p[i].mFreeList.GetNumFree();
        }
        return sum;
    }

        // Returns the number of items currently free.
    // Expensive because of cache misses if other cores are using this.  Data is stale before returning.
    ULONGLONG GetNumFree(ULONG partition) const { return p[partition].mFreeList.GetNumFree(); }

    // Frees an item
    void AddHead(Type * element) {
        p[Hash()].mFreeList.AddHead(element);
    }

    // Frees multiple items
    // @param list - the list of items to free.  Returned empty.
    // Returns: Number of items added.
    ULONG  AddHead(ListOfType & list) {
        return p[Hash()].mFreeList.AddHead(list);
    }

    // Allocates one item.  Returns NULL of none are free.
    Type * RemoveHead() {
        ULONG ptn = Hash();
        ULONG first = ptn;

        do {
            Type * element = p[ptn].mFreeList.RemoveHead();
            if (element) return element;
            ptn = (ptn + 1) % mNumCpus;

        } while (ptn != first);

        return NULL;
    }


    // Allocates the number of items requested
    // @param list - allocated items are placed onto this list.  The list need not be empty.
    // @param numElements - the number of items requested.
    // Returns: the number of items added to the list.  If < numElements, then there were
    // no available items.
    ULONG RemoveHead(ListOfType & list, ULONG numElements =1) {
        ULONG ptn = Hash();
        ULONG first = ptn;
        ULONG sum = 0;
        
        do {
            sum += p[ptn].mFreeList.RemoveHead(list, numElements-sum);
            if (sum == numElements) {
                return sum;
            }
            ptn = (ptn + 1) % mNumCpus;

        } while (ptn != first);
        return sum;
    }

    // Removes given element from the list.
    // Returns:
    //      true: Successfully removed the element.
    //      false: Didn't find the element on list.
    bool Remove(Type* elementToBeRemoved) {
        bool result = false;
        for (ULONG i=0; i < mNumCpus && !result; i++) {

            result = p[i].mFreeList.Remove(elementToBeRemoved);
        }
        return result;
    }

    // Iterate through each element of the list and identify whether we need to remove it from the list or not 
    // by using ListEvaluator object. Removed elements will be added to the provided list.
    void RemoveIfMatch(ListEvaluator<Type> * eval, ListOfType & removedElements) {
        for(ULONG numPart = 0; numPart < mNumCpus; numPart++) {
            p[numPart].mFreeList.AcquireSpinLock();
            
            Type* element = NULL;
            Type* prevElement = NULL;
            element = GetNext(element, numPart);
            while(element) {
                if(eval->ShouldRemove(element)) {
                    p[numPart].mFreeList.RemoveUsingPreviousLockHeld(element, prevElement);
                    removedElements.AddHead(element);
                    element = prevElement;
                }
                prevElement = element;
                element = GetNext(prevElement, numPart);
            }
            
            p[numPart].mFreeList.ReleaseSpinLock();
        }
    }

    ULONG NumPartition() const {
        return mNumCpus;
    }
    
private:
    // Returns next element of provided current element for the given partition.
    Type* GetNext(Type* currentElement, ULONG partition) {
        return p[partition].mFreeList.GetNextLockHeld(currentElement);
    }

    // Returns a partition number within the range allowed,
    // based on the CPU that execution occurs on.
    ULONG Hash() const { 
        // FIX use OS independent implementation.
        return csx_p_get_processor_id() % mNumCpus;
    }

    // An array of freelists, with padding to avoid "false sharing".
    struct CSX_ALIGN_N(CSX_AD_L1_CACHE_LINE_SIZE_BYTES) Freelist {

        BasicLockedFreeList<Type, ListOfType> mFreeList;

    };
    
    int      mNumCpus;
    Freelist *p;   // An Array of mNumCpus elements
};



#endif  // __BasicPartitionedFreeList_h__
