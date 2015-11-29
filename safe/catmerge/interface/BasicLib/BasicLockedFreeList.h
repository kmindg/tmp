#ifndef __BasicLockedFreeList_h__
#define __BasicLockedFreeList_h__

//***************************************************************************
// Copyright (C) EMC Corporation 2010-2014
// Licensed material -- property of EMC Corporation
//**************************************************************************/

//**************************************************************************
//  $Id$
//**************************************************************************

//++
// File Name:
//      BasicLockedFreeList.h
//
// Contents:
//      Defines the BasicLockedFreeList class. 
//
// Revision History:
//--

# include "BasicLib/BasicLockedObject.h"

// Provide a way to identify that should we remove particular element from the list or not.
template<class Type>
class ListEvaluator {
public:
    virtual bool ShouldRemove(Type* element) = 0;
};

// Free list with own lock, and no provision for resource wait.  The lock is
// exposed, allowing the caller to acquire the lock, and create larger
// atomic operations.
template <class Type, class ListOfType>
class BasicLockedFreeList : public BasicLockedObject  {
public:
    BasicLockedFreeList() : mCount(0) {}

    // Returns the number of items currently free.
    //   Data is stale before returning.
    ULONGLONG GetNumFree() const { return mCount; }
    // Frees an item
    void AddHead(Type * element) {
        AcquireSpinLock();
        AddHeadLockHeld(element);
        ReleaseSpinLock();
    }

    // Frees an item
    void AddHeadLockHeld(Type * element) {
        mFreeList.AddHead(element);
        mCount++;
    }

    // Frees multiple items
    // @param list - the list of items to free.  Returned empty.
    // Returns: Number of items added.
    ULONG AddHead(ListOfType & list) {
        AcquireSpinLock();
        ULONG result = AddHeadLockHeld(list);
        ReleaseSpinLock();
        return result;
    }
    // Frees multiple items
    // @param list - the list of items to free.  Returned empty.
    // Returns: Number of items added.
    ULONG AddHeadLockHeld(ListOfType & list) {
        ULONG result = 0;
        for (;;) {
            Type * element = list.RemoveHead();
            if (!element) break;
            result++;

            mFreeList.AddHead(element);
        }
        mCount += result;
        return result;
    }

    // Allocates one item.  Returns NULL of none are free.
    Type * RemoveHead() {
        AcquireSpinLock();
        Type * element = RemoveHeadLockHeld();
        ReleaseSpinLock();
        return element;
    }

    // Allocates one item.  Returns NULL of none are free.
    Type * RemoveHeadLockHeld() {
        Type * element = mFreeList.RemoveHead();
        if (element)  {
            DBG_ASSERT(mCount > 0);
            mCount--;
        }
        else {
            DBG_ASSERT(mCount == 0);
        }
        return element;
    }

    // Allocates the number of items requested
    // @param list - allocated items are placed onto this list.  The list need not be empty.
    // @param numElements - the number of items requested.
    // Returns: the number of items added to the list.  If < numElements, then there were
    // no available items.
    ULONG RemoveHead(ListOfType & list, ULONG numElements =1) {
        AcquireSpinLock();
        ULONG i = RemoveHeadLockHeld(list, numElements);
        ReleaseSpinLock();   
        return i;
    }

    // Allocates the number of items requested
    // @param list - allocated items are placed onto this list.  The list need not be empty.
    // @param numElements - the number of items requested.
    // Returns: the number of items added to the list.  If < numElements, then there were
    // no available items.
    ULONG RemoveHeadLockHeld(ListOfType & list, ULONG numElements =1) {
        ULONG i = 0;
        for(; i < numElements; i++) {
            Type * element = mFreeList.RemoveHead();
            if (element == NULL) {
                DBG_ASSERT(mCount == 0);
                break;
            }
            list.AddHead(element);
            DBG_ASSERT(mCount > 0);
            mCount--;
        }

        return i;
    }

    // Removes given element from the list if it is on the list.
    // Returns: 
    //      true: Successfully removed.
    //      false: Didn't find element on list.
    bool Remove(Type* elementToBeRemoved) {
        bool result = false;
        AcquireSpinLock();
        result = mFreeList.Remove(elementToBeRemoved);
        if(result) {
            DBG_ASSERT(mCount > 0);
            mCount--;
        }
        ReleaseSpinLock();
        return result;
    }
    
    // Iterate through each element of the list and identify whether we need to remove it from the list or not 
    // by using ListEvaluator object. Removed elements will be added to the provided list.
    void RemoveIfMatch(ListEvaluator<Type> * eval, ListOfType & removedElements) {
        Type *element;
        Type *prevElement = NULL;
        AcquireSpinLock();
        element = GetNextLockHeld(NULL);
        while(element) {
            if(eval->ShouldRemove(element)) {
                mFreeList.RemoveUsingPrevious(element, prevElement);
                mCount--;
                removedElements.AddHead(element);
                element = prevElement;
            }
            prevElement = element;
            element = GetNextLockHeld(prevElement);
        }
        
        ReleaseSpinLock();
    }

    // Removes given element from the list without iterating whole list by
    // using previous element.
    void RemoveUsingPreviousLockHeld(Type* elementToBeRemoved, Type* prevElement) {
        mFreeList.RemoveUsingPrevious(elementToBeRemoved, prevElement);
        mCount--;
    }

    // Returns next element of the given element. Returns head if given element is NULL.
    Type* GetNextLockHeld(Type* currentElement) {
        Type* element = mFreeList.GetNext(currentElement);
        return element;
    }
    
    ULONGLONG QueueLength()
    {
        return mCount;
    }

private:
    ListOfType mFreeList; 
    ULONGLONG  mCount;
};



#endif  // __BasicLockedFreeList_h__
