#ifndef __TemplatedCountedList__
#define __TemplatedCountedList__

//***************************************************************************
// Copyright (C) EMC Corporation 2010-2013
// All rights reserved.
// Licensed material -- property of EMC Corporation
//**************************************************************************/

//**************************************************************************
//  Id
//**************************************************************************

//++
// File Name:
//      CountedList.h
//
// Contents:
//      Wraps a list type completely with one with an identical interface
//      which also keeps a counter.
//
// Example:      
//      typedfef CountedList<foo, LIFOList<Foo>> CountedLIFOListOfFoo;
//

//
// Revision History:
//  1-Jul-2010   Harvey    Created.
//--

// Extends listType to reliably keep track of the number of items on the list.
//
// Assumes the list type is not locked, and that the caller is responsible for 
// concurency control.
template <class elementType, class listType>
class CountedList : private listType 
{

public:

    CountedList() : mCount(0)  {} 

    ULONG GetNumElements() const { return mCount; }

    // Add to the front of the list.
    // @param element - the element to add to the list.  Must not be NULL. The caller is
    //                  responible locking the list during this call.
    VOID AddHead(elementType * element) { mCount++; listType::AddHead(element); }

    // Add to the end of the list.  (Not very efficient)
    // @param element - the element to add to the list.  Must not be NULL. The caller is
    //                  responible locking the list during this call.
    VOID AddTail(elementType * element) { mCount++; listType::AddTail(element); }

    // Remove an element from the front of the list.  Returns NULL if the list is empty.
    // The caller is responible locking the list during this call.
    elementType * RemoveHead()
    { 
        elementType * elem = listType::RemoveHead();
        if (elem)  { FF_ASSERT(mCount); mCount --; }
        return elem;
    }

    // Remove an element from the tail of the list.  Returns NULL if the list is empty.
    // The caller is responible locking the list during this call.
    // WARNING: for compatibility across list types.  This can be very expensive.
    elementType * RemoveTail()
    { 
        elementType * elem = listType::RemoveTail();
        if (elem)  { FF_ASSERT(mCount); mCount --; }
        return elem;
    }     

    // Remove the specified element from the list. The caller is responsible locking the list during this
    // call.
    // Returns: true if the element was removed, FALSE if it was not on the list.
    bool Remove(elementType * element)
    { 
        bool result =  listType::Remove(element);
        if (result)  { FF_ASSERT(mCount); mCount --; }
        return result;
    }

    // Remove the specified element from the list. The caller is responsible locking the list during this
    // call.
    // Returns: true if the element was removed, FALSE if it was not on the list.
    bool MoveToTail(elementType * element)
    { 
        bool result =  listType::MoveToTail(element);
        return result;
    }
    

    // Iterator to walk the list.  With no arguments, returns the element on the front of
    // the list, otherwise the subsequent element.  Must not be called with an element
    // which is not currently on the list.  If Remove() is called during iteration, the
    // element removed must not be subsequently passed in to this function. The caller is
    // responible locking the list during this call. Returns NULL if element is tail of
    // list.
    elementType * GetNext(elementType * element = 0) const
    {
        return listType::GetNext(element);
    }

     // Remove the specified element from the list without iterating whole list. The caller is responsible for
    // locking the list during this call and ensuring that previous is still on the list. Caller has to provide 
    // previous element. It may validate the chain between element to be removed and previous element. 
    // If it find any inconsistency then it panics.
    void RemoveUsingPrevious(elementType * element, elementType * prevElement) {
        FF_ASSERT(mCount); 
        mCount --;  
        listType::RemoveUsingPrevious(element, prevElement);
    }

  
    // Returns true if there are no elements on the list, false otherwise. This function
    // does not require locking.
    bool IsEmpty() const { return listType::IsEmpty(); } 

    // Returns true if there are at least 2 items on the list. The caller is responible
    // locking the list during this call.
    bool HasMultipleItems()   const { return mCount > 1; }

    // The caller is responsible locking the list during this call.
    bool IsOnList(elementType * element)   const { return listType::IsOnList(); }

    // Moves all the items on the from list to this list, preserving items already on this
    // list. The caller is responsible locking both lists during this call.
    VOID MoveList(CountedList &  from);

    // Moves all the items on the from list to this list, preserving items already on this
    // list. The caller is responsible locking both lists during this call.
    VOID MoveList(listType &  from);

    // Moves all elements from this list to the given uncounted list and resets count to 
    // zero. The caller is responsible locking both lists during this call.
    VOID MoveToUncountedList(listType & toList);

protected:
    // The number of items on the list.
    ULONG    mCount;

    void EventListMoved() { mCount = 0; }
};


template <class elementType, class listType>
inline void CountedList<elementType,listType>::MoveList(listType & from) 
{ 
    for(;;) {
        elementType * elem = from.RemoveHead();
        if (!elem) break;
        AddTail(elem);
    }
   
}

template <class elementType, class listType>
inline void CountedList<elementType,listType>::MoveList(CountedList<elementType,listType> & from) 
{ 
    mCount += from.GetNumElements();    
    listType::MoveList(from);
    from.EventListMoved();
}

template <class elementType, class listType>
inline void CountedList<elementType,listType>::MoveToUncountedList(listType & to) 
{ 
    to.listType::MoveList(*this);
    EventListMoved();
}

# endif
