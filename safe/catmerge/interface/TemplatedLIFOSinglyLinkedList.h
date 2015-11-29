#ifndef __TemplatedLIFOSinglyLinkedList__
#define __TemplatedLIFOSinglyLinkedList__

//***************************************************************************
// Copyright (C) EMC Corporation 2010-2014
// All rights reserved.
// Licensed material -- property of EMC Corporation
//**************************************************************************/

//**************************************************************************
//  Id
//**************************************************************************

//++
// File Name:
//      TemplateLIFOSinglyLinkedList.h
//
// Contents:
//      Template class to creat and manipulate singly linked lists with only a head. These
//      singly linked lists are typed, and type safety is provided by the compiler.
//
//      Performance optimized for add and remove both the list head.   Smallest footprint
//      list head.
//
//      Note: these lists display well in the debugger.
//
// Usage:
//      0) Select the correct type of list - current choices:
//          - LIFOList if items are typically added/removed from the LIFOhead (e.g.
//            freelists)
//
//      1) declare class of element:  class Foo;
//      
//      2) define one type of list:  
//              typedef LIFOList<Foo>  ListOfFoo;
//
//      3) Define element type, inheriting from ListEntry type:
//              class Foo : public ListOFFoo::ListEntry { ... }
//      4) You can define other list types, using arbitary, but unique 2nd parameters
//         typedef LIFOList<Foo, 23> AnotherListOfFooOnWhichFooCanBEOnSimultaneously;
//         class Foo : public ListOFFoo::ListEntry,
//                          public
//                          AnotherListOfFooOnWhichFooCanBEOnSimultaneously::ListEntry {
//      5) If an element can be on either, but not both of two lists, you can use the same
//         list type.
//
//
//   NOTE: Each list is assumed to be appropriately locked by the caller.
//         Simultaneous calls for the same list on different threads/CPUs will cause list
//         corruption.
//
// Revision History:
//  15-Aug-2000   Harvey    Created.
//--
# include "K10Assert.h"

//
// If DBG_LIST_CHECKING_ENABLED is defined then when running in simulation the code
// which check the first x entries in the list looking for a duplicate.  Currently the
// limit is 1000 entries.
//
//#define DBG_LIST_CHECKING_ENABLED 1


// This class is used to generate default link type for LIFOList. 
//
// The second parameter is only used to create unique types to allow multiple inheritance.
// This parameter is unused except to create different list types
template <class elementType, int uniqueID = 0xDEFA>  
class DefaultLIFOListEntry {
    public:
        DefaultLIFOListEntry() : mForwardLink(reinterpret_cast<elementType*> (DebugWasRemoved)) {}


        enum { DebugWasRemoved =1 };
        elementType *    mForwardLink;
    };


// List with only head pointer.  AddHead/RemoveHead are very fast operations, so if the
// need is Last In First Out, this llist type is ideal. Remove, and operations on the tail
// (if supported) would be slow.
//
// The second parameter allows use of an arbitrary class to inherit from, so long as it
// has a field name "mForwardLink" of type "elementTpe *".  This would enable elementType
// to sometimes be on a LIFO list, and other times on an doubly linked list, but using the same
// pair of link  fields.
template <class elementType, class ListEntryType = DefaultLIFOListEntry<elementType> >
class LIFOList 
{

public:
    typedef  ListEntryType ListEntry;

    LIFOList() : mLIFOHead(NULL)  {}                    
    ~LIFOList() { DBG_ASSERT(mLIFOHead == NULL);}                    

    // Add to the front of the list.
    // @param element - the element to add to the list.  Must not be NULL. The caller is
    //                  responible locking the list during this call.
    VOID AddHead(elementType * element);

    // Add to the end of the list.  (Not very efficient)
    // @param element - the element to add to the list.  Must not be NULL. The caller is
    //                  responible locking the list during this call.
    VOID AddTail(elementType * element);

    // Remove an element from the front of the list.  Returns NULL if the list is empty.
    // The caller is responible locking the list during this call.
    elementType * RemoveHead();  

    // Remove an element from the tail of the list.  Returns NULL if the list is empty.
    // The caller is responible locking the list during this call.
    // WARNING: for compatibility across list types.  This can be very expensive.
    elementType * RemoveTail();     

    // Remove the specified element from the list. The caller is responsible locking the list during this
    // call.
    // Returns: true if the element was removed, FALSE if it was not on the list.
    bool Remove(elementType * element);

    // Remove the specified element from the list without iterating whole list. The caller is responsible for
    // locking the list during this call and ensuring that previous is still on the list. Caller has to provide 
    // previous element. It may validate the chain between element to be removed and previous element. 
    // If it find any inconsistency then it panics.
    void RemoveUsingPrevious(elementType * element, elementType * prevElement);

    // Iterator to walk the list.  With no arguments, returns the element on the front of
    // the list, otherwise the subsequent element.  Must not be called with an element
    // which is not currently on the list.  If Remove() is called during iteration, the
    // element removed must not be subsequently passed in to this function. The caller is
    // responible locking the list during this call. Returns NULL if element is tail of
    // list.
    elementType * GetNext(elementType * element = 0) const;

    // Returns true if there are no elements on the list, false otherwise. This function
    // does not require locking.
    bool IsEmpty() const;  

    // Returns true if there are at least 2 items on the list. The caller is responible
    // locking the list during this call.
    bool HasMultipleItems()   const; 

    // The caller is responsible locking the list during this call.
    bool IsOnList(elementType * element)   const;

    // Moves all the items on the from list to this list, preserving items already on this
    // list. The caller is responsible locking both lists during this call.
    VOID MoveList(LIFOList &  from);

protected:
    // Value to store in Link field when item is not on list.  For debug only.  No code should test this. 
    enum { DebugWasRemoved =1 };

    VOID DEBUG_ClearLink(elementType * e);
    VOID DEBUG_AssertNotOnList(elementType * e);     
    // We want this to be the full type for the debugger...
    elementType * mLIFOHead; 

    // Non-sensical operations since they would leave items on two lists, but used for persistent memory recovery.
    LIFOList(const LIFOList & other) : mLIFOHead(other.mLIFOHead) {}

    LIFOList & operator = (const LIFOList & other); 


   
};

template <class elementType, class ListEntryType>
inline void LIFOList<elementType,ListEntryType>::AddHead(elementType * element)               
{  
#if defined(DBG_LIST_CHECKING_ENABLED) && defined(SIMMODE_ENV)
    // For simulation, search entire list to check whether the element is already on list 
    // or not.
    DEBUG_AssertNotOnList(element);             
#else
    // For hardware, just compare the element with the head.
    // NOTE: On hardware search entire list takes significant time.
    DBG_ASSERT(mLIFOHead != element);
#endif

    element->ListEntryType::mForwardLink = mLIFOHead;                        
    mLIFOHead = element;    
}                                                                  

template <class elementType, class ListEntryType>
inline elementType *  LIFOList<elementType,ListEntryType>::RemoveHead()                        
{                                                                  
    elementType * result;  

    result = mLIFOHead;
    if(result) {
        mLIFOHead = result->ListEntryType::mForwardLink;

        FF_ASSERT(mLIFOHead != reinterpret_cast<elementType*> (DebugWasRemoved));
        DEBUG_ClearLink(result) ;
    }
    return (result);
}

template <class elementType, class ListEntryType>
inline elementType *  LIFOList<elementType,ListEntryType>::RemoveTail()                        
{                                                                  
    elementType * prev;  

    prev = mLIFOHead;
    if(prev) {
        elementType * result = prev->ListEntryType::mForwardLink;  
        
        if (result == NULL) {
            mLIFOHead = NULL; // now empty
            return prev;
        }

        while (result->ListEntryType::mForwardLink) {
            prev = result;
            result = prev->ListEntryType::mForwardLink;
        }
        
        prev->ListEntryType::mForwardLink = NULL;
        DEBUG_ClearLink(result) ;  
        return result;

        mLIFOHead = result->ListEntryType::mForwardLink;

    }
    return NULL;
}

template <class elementType, class ListEntryType>
inline bool LIFOList<elementType,ListEntryType>::Remove(elementType * element)                
{                                                                  

    if((element) == mLIFOHead) {                        
        mLIFOHead = (element)->ListEntryType::mForwardLink; 
        DEBUG_ClearLink(element);
        return true; 
    }                                                   
    else {                                              
        elementType * next = mLIFOHead;                         
        while (next) {                                     
            if (next->ListEntryType::mForwardLink == element) {                 
                next->ListEntryType::mForwardLink = element->ListEntryType::mForwardLink;   
                DEBUG_ClearLink(element);
                return true;                                   
            }                                            
            next = next->ListEntryType::mForwardLink;                               
        }                                                
    }                                                   
    return false;
}   

template <class elementType, class ListEntryType>
inline void LIFOList<elementType,ListEntryType>::RemoveUsingPrevious(elementType * element, elementType * prevElement)                
{                                                                  

    if((element) == mLIFOHead) {                        
        mLIFOHead = (element)->ListEntryType::mForwardLink; 
        DEBUG_ClearLink(element);
    }                                                   
    else {                                              
        DBG_ASSERT(prevElement);
        FF_ASSERT(prevElement->ListEntryType::mForwardLink == element);
        prevElement->ListEntryType::mForwardLink = element->ListEntryType::mForwardLink;
        DEBUG_ClearLink(element);
    }                                                   
}  

template <class elementType, class ListEntryType>
inline void LIFOList<elementType,ListEntryType>::AddTail(elementType * element)                
{ 
    DBG_ASSERT(element);  // Pointer followed below, so this is not needed
    elementType * current = mLIFOHead;

    if(current == NULL) {
        AddHead(element);
    }
    else {
                                            
        for(;;) {
            FF_ASSERT(current != element);  // already on list?

            elementType * next = current->ListEntryType::mForwardLink;
            if (next == NULL) break;
            current = next;
        }
        element->ListEntryType::mForwardLink = NULL;
        current->ListEntryType::mForwardLink = element;
    }                                                   
}


template <class elementType, class ListEntryType>
inline bool LIFOList<elementType,ListEntryType>::IsEmpty() const                              
{                                                                  
    return mLIFOHead == NULL;                                 
}

template <class elementType, class ListEntryType>
inline bool LIFOList<elementType,ListEntryType>::HasMultipleItems()   const                   
{                                                                  
    return (mLIFOHead!=NULL) && (mLIFOHead->ListEntryType::mForwardLink!= NULL);             
}             

template <class elementType, class ListEntryType>
inline void LIFOList<elementType,ListEntryType>::MoveList(LIFOList<elementType,ListEntryType> & from) 
{ 
    // Take action only if there is something on the from list.
    if (from.mLIFOHead) {

        // Is there something on each list? Guess that from is shorter (e.g., we are accumulating
        // items in this list).
        if (mLIFOHead) {
            elementType * next = from.mLIFOHead;

            // Find tail
            while(next->ListEntryType::mForwardLink) {
                next = next->ListEntryType::mForwardLink;
            }

            // Append existing list to from
            next->ListEntryType::mForwardLink = mLIFOHead;
        }
        mLIFOHead = from.mLIFOHead;                         
        from.mLIFOHead = NULL; 
    }
}

template <class elementType, class ListEntryType>
inline elementType *  LIFOList<elementType,ListEntryType>::GetNext(elementType * element) const
{ 
    if (element== NULL) return mLIFOHead;
    else return element->ListEntryType::mForwardLink;
}

template <class elementType, class ListEntryType>
inline    bool LIFOList<elementType,ListEntryType>::IsOnList(elementType * element)   const {
    if (element->ListEntryType::mForwardLink == reinterpret_cast<elementType*> (DebugWasRemoved)) {
        return false;
    }
    elementType * next = mLIFOHead;
    while(next) {
        if (next == element) { return TRUE; }
        next = next->ListEntryType::mForwardLink;
    }
    return false;
}



template <class elementType, class ListEntryType>
inline    VOID LIFOList<elementType,ListEntryType>::DEBUG_ClearLink(elementType * element) {

    // NOTE: this is done for debuggability, but no assumptions should be made about
    // the state of the link pointers for items not on lists.
    element->ListEntryType::mForwardLink = reinterpret_cast<elementType*> (DebugWasRemoved);

}
template <class elementType, class ListEntryType>
inline VOID LIFOList<elementType,ListEntryType>::DEBUG_AssertNotOnList(elementType * element)      
{   
    // FIFO lists with tail pointers can be much faster at verification
    DBG_ASSERT(!IsOnList(element));

}

# endif
