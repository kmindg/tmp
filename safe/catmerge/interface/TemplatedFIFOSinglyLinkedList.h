#ifndef __TemplatedFIFOSinglyLinkedList__
#define __TemplatedFIFOSinglyLinkedList__

//***************************************************************************
// Copyright (C) EMC Corporation 2011-2014
// All rights reserved.
// Licensed material -- property of EMC Corporation
//**************************************************************************/

//**************************************************************************
//  Id
//**************************************************************************

//++
// File Name:
//      TemplateFIFOSinglyLinkedList.h
//
// Contents:
//      Template class to create and manipulate singly linked lists with only a head. These
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
//          - DLNKList if items added/removed to/from both head and tail or removed
//            from within the list.
//          - FIFOList it items typically added to the head or tail and removed from
//            the head.      
//
//      1) declare class of element:  class Foo;
//      
//      2) define one type of list:  
//              typedef FIFOList<Foo>  ListOfFoo;
//
//      3) Define element type, inheriting from ListEntry type:
//              class Foo : public ListOFFoo::ListEntry { ... }
//      4) You can define other list types, using arbitary, but unique 2nd parameters
//         typedef FIFOList<Foo, 23> AnotherListOfFooOnWhichFooCanBEOnSimultaneously;
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
//  1-Feb-2011   Winchell    Created.  Based on TemplatedLIFO by Harvey.
//--
# include "K10Assert.h"

//
// If DBG_LIST_CHECKING_ENABLED is defined then when running in simulation the code
// which check the first x entries in the list looking for a duplicate.  Currently the
// limit is 1000 entries.
//
//#define DBG_LIST_CHECKING_ENABLED 1

// This class is used to generate default link type for FIFOList. 
//
// The second parameter is only used to create unique types to allow multiple inheritance.
// This parameter is unused except to create different list types
template <class elementType, int uniqueID = 0xDEFA>  
class DefaultFIFOListEntry {
    public:
        DefaultFIFOListEntry() : mForwardLink(reinterpret_cast<elementType*> (DebugWasRemoved)) {}


        enum { DebugWasRemoved =1 };
        elementType *    mForwardLink;
    };


// List with head and tail pointer and one link.  AddTail/RemoveHead are very fast operations, so if the
// need is First In First Out, this llist type is ideal. Remove on the tail
// is slow.
//
// The second parameter allows use of an arbitrary class to inherit from, so long as it
// has a field name "mForwardLink" of type "elementTpe *".  This would enable elementType
// to sometimes be on a FIFO list, and other times on an doubly linked list, but using the same
// pair of link  fields.
template <class elementType, class ListEntryType = DefaultFIFOListEntry<elementType> >
class FIFOList 
{

public:
    typedef  ListEntryType ListEntry;

    FIFOList() : mFIFOHead(NULL), mFIFOTail(NULL)  {}                    
    ~FIFOList() { DBG_ASSERT(mFIFOHead == NULL);}                    

    // Add to the front of the list.
    // @param element - the element to add to the list.  Must not be NULL. The caller is
    //                  responsible for locking the list during this call.
    VOID AddHead(elementType * element);

    // Add to the end of the list.
    // @param element - the element to add to the list.  Must not be NULL. The caller is
    //                  responsible for locking the list during this call.
    VOID AddTail(elementType * element);

    // Insert an element after the specified listelement.
    // Listelement is already on the list
    // @param listelement: listelement is a node already on the list.
    //                     if NULL, insert at head
    // @param element: element to be inserted
    VOID InsertAfter(elementType * element, elementType * listelement);

    // Remove an element from the front of the list.  Returns NULL if the list is empty.
    // The caller is responsible for locking the list during this call.
    elementType * RemoveHead();  

    // Remove an element from the tail of the list.  Returns NULL if the list is empty.
    // The caller is responsible for locking the list during this call.
    // WARNING: for compatibility across list types.  This can be very expensive.
    // Use Doubly Linked List if RemoveTail speed is important.
    elementType * RemoveTail();     

    // Remove the specified element from the list. The caller is responsible locking the list during this
    // call.
    // Returns: true if the element was removed, FALSE if it was not on the list.
    // This can be very expensive.
    // Use Doubly Linked List if Remove speed is important.
    bool Remove(elementType * element);

    // Iterator to walk the list.  With no arguments, returns the element on the front of
    // the list, otherwise the subsequent element.  Must not be called with an element
    // which is not currently on the list.  If Remove() is called during iteration, the
    // element removed must not be subsequently passed in to this function. The caller is
    // responsible for locking the list during this call. Returns NULL if element is tail of
    // list.
    elementType * GetNext(elementType * element = 0) const;

    // Returns the last element in the list. Returns NULL if the list is empty.
    elementType * GetTail() const;

    // Returns true if there are no elements on the list, false otherwise. This function
    // does not require locking.
    bool IsEmpty() const;  

    // Returns true if there are at least 2 items on the list. The caller is responsible for
    // locking the list during this call.
    bool HasMultipleItems()   const; 

    // The caller is responsible locking the list during this call.
    bool IsOnList(elementType * element)   const;

    // Moves all the items on the from list to this list, preserving items already on this
    // list. The caller is responsible locking both lists during this call.
    VOID MoveList(FIFOList &  from);

protected:
    // Value to store in Link field when item is not on list.  For debug only.  No code should test this. 
    enum { DebugWasRemoved =1 };

    VOID DEBUG_ClearLink(elementType * e);
    VOID DEBUG_AssertNotOnList(elementType * e);     
    // We want this to be the full type for the debugger...
    elementType * mFIFOHead; 
    elementType * mFIFOTail; 

    // Non-sensical operations since they would leave items on two lists, but used for persistent memory recovery.
    FIFOList(const FIFOList & other) : mFIFOHead(other.mFIFOHead), mFIFOTail(other.mFIFOTail) {}

    FIFOList & operator = (const FIFOList & other); 


   
};

template <class elementType, class ListEntryType>
inline void FIFOList<elementType,ListEntryType>::AddHead(elementType * element)               
{  
#if defined(DBG_LIST_CHECKING_ENABLED) && defined(SIMMODE_ENV)
    // For simulation, search entire list to check whether the element is already on list 
    // or not.
    DEBUG_AssertNotOnList(element);             
#else
    // For hardware, just compare the element with the head.
    // NOTE: On hardware search entire list takes significant time.
    DBG_ASSERT(mFIFOHead != element);
#endif

    element->ListEntryType::mForwardLink = mFIFOHead;
    if(mFIFOHead == NULL) {
        mFIFOTail = element;
    } 
    mFIFOHead = element;    
}
                                                                  
template <class elementType, class ListEntryType>
inline void FIFOList<elementType,ListEntryType>::AddTail(elementType * element)                
{ 
#if defined(DBG_LIST_CHECKING_ENABLED) && defined(SIMMODE_ENV)
    // For simulation, search entire list to check whether the element is already on list 
    // or not.
    DEBUG_AssertNotOnList(element);             
#else
    // For hardware, just compare the element with the head.
    // NOTE: On hardware search entire list takes significant time.
    DBG_ASSERT(mFIFOHead != element);
#endif

    element->ListEntryType::mForwardLink =  NULL;
    if (mFIFOHead == NULL) {
        mFIFOHead = element;
    }
    else {
        mFIFOTail->ListEntryType::mForwardLink = element;
    }
    mFIFOTail = element;
}

template <class elementType, class ListEntryType>
inline VOID FIFOList<elementType,ListEntryType>::InsertAfter(elementType * element, elementType * listelement)                
{
#if defined(DBG_LIST_CHECKING_ENABLED) && defined(SIMMODE_ENV)
    // For simulation, search entire list to check whether the element is already on list
    // or not.
    DEBUG_AssertNotOnList(element);
#else
    // For hardware, just compare the element with the head.
    // NOTE: On hardware search entire list takes significant time.
    DBG_ASSERT(mFIFOHead != element);
#endif

    (element)->ListEntryType::mForwardLink = NULL;
    if(!mFIFOHead)
    {                        
        // Empty list
        mFIFOHead = mFIFOTail = element;
    }                                                   
    else if((listelement) == NULL)
    {
        // Add to head of list
        (element)->ListEntryType::mForwardLink = mFIFOHead;
        mFIFOHead = element;
    }
    else if((listelement) == mFIFOTail)
    {
        // Append to end of list
        mFIFOTail->ListEntryType::mForwardLink = element;
        mFIFOTail = element;
    }
    else
    {
        // Insert in middle of list
        (element)->ListEntryType::mForwardLink = (listelement)->ListEntryType::mForwardLink;
        (listelement)->ListEntryType::mForwardLink = element;
    }
}


template <class elementType, class ListEntryType>
inline elementType *  FIFOList<elementType,ListEntryType>::RemoveHead()                        
{                                                                  
    elementType * result;  

    result = mFIFOHead;
    if(result) {
        elementType * next = result->ListEntryType::mForwardLink;
        FF_ASSERT(next != reinterpret_cast<elementType*> (DebugWasRemoved));
        mFIFOHead = next;
        if(mFIFOHead == NULL) {
            mFIFOTail = NULL;
        }
        DEBUG_ClearLink(result) ;
    }
    return (result);
}

// Inefficient. Use doubly linked list instead.

template <class elementType, class ListEntryType>
inline elementType *  FIFOList<elementType,ListEntryType>::RemoveTail()                        
{                                                                  
    elementType * prev;  

    prev = mFIFOHead;
    if(prev) {
        elementType * result = prev->ListEntryType::mForwardLink;  
        
        if (result == NULL) {
            mFIFOHead = NULL; // now empty
            mFIFOTail = NULL;
            DEBUG_ClearLink(prev);
            return prev;
        }

        while (result->ListEntryType::mForwardLink) {
            prev = result;
            result = prev->ListEntryType::mForwardLink;
        }
        
        prev->ListEntryType::mForwardLink = NULL;
        FF_ASSERT(mFIFOTail == result);
        mFIFOTail = prev;
        DEBUG_ClearLink(result) ;  
        return result;

    }
    FF_ASSERT(mFIFOTail == NULL);
    return NULL;
}

// Inefficient. Use doubly linked list instead.

template <class elementType, class ListEntryType>
inline bool FIFOList<elementType,ListEntryType>::Remove(elementType * element)                
{                                                                  

    if((element) == mFIFOHead) {                        
        mFIFOHead = (element)->ListEntryType::mForwardLink;
        if(mFIFOHead == NULL) {
            mFIFOTail = NULL;
        }
        DEBUG_ClearLink(element);
        return true; 
    }                                                   
    else {                                              
        elementType * next = mFIFOHead;                         
        while (next) {                                     
            if (next->ListEntryType::mForwardLink == element) {                 
                next->ListEntryType::mForwardLink = element->ListEntryType::mForwardLink;
                if(next->ListEntryType::mForwardLink == NULL) {
                    FF_ASSERT(mFIFOTail == element);
                    mFIFOTail = next;
                }
                DEBUG_ClearLink(element);
                return true;                                   
            }                                            
            next = next->ListEntryType::mForwardLink;                               
        }                                                
    }                                                   
    return false;
}   



template <class elementType, class ListEntryType>
inline bool FIFOList<elementType,ListEntryType>::IsEmpty() const                              
{                                                                  
    return mFIFOHead == NULL;                                 
}

template <class elementType, class ListEntryType>
inline bool FIFOList<elementType,ListEntryType>::HasMultipleItems()   const                   
{                                                                  
    return (mFIFOHead!=NULL) && (mFIFOHead->ListEntryType::mForwardLink!= NULL);             
}             

template <class elementType, class ListEntryType>
inline void FIFOList<elementType,ListEntryType>::MoveList(FIFOList<elementType,ListEntryType> & from) 
{ 
    if(from.mFIFOHead) {
        if (mFIFOHead) {
            mFIFOTail->ListEntryType::mForwardLink = from.mFIFOHead;
            mFIFOTail = from.mFIFOTail; 
        }
        else {
            mFIFOHead = from.mFIFOHead;
            mFIFOTail = from.mFIFOTail;
        }
        from.mFIFOHead = NULL;
        from.mFIFOTail = NULL;
    }
}

template <class elementType, class ListEntryType>
inline elementType *  FIFOList<elementType,ListEntryType>::GetNext(elementType * element) const
{ 
    if (element== NULL) {
        return mFIFOHead;
    }
    else {
        return element->ListEntryType::mForwardLink;
    }
}

template <class elementType, class ListEntryType>
inline elementType *  FIFOList<elementType,ListEntryType>::GetTail() const
{ 
    return mFIFOTail;
}

template <class elementType, class ListEntryType>
inline    bool FIFOList<elementType,ListEntryType>::IsOnList(elementType * element)   const {
    if (element->ListEntryType::mForwardLink == reinterpret_cast<elementType*> (DebugWasRemoved)) {
        return false;
    }
    elementType * next = mFIFOHead;
    int i = 0;
    while(next) {
        if(i++ > 1000) { break; }
        if (next == element) { return TRUE; }
        next = next->ListEntryType::mForwardLink;
    }
    return false;
}



template <class elementType, class ListEntryType>
inline    VOID FIFOList<elementType,ListEntryType>::DEBUG_ClearLink(elementType * element) {

    // NOTE: this is done for debuggability, but no assumptions should be made about
    // the state of the link pointers for items not on lists.
    element->ListEntryType::mForwardLink = reinterpret_cast<elementType*> (DebugWasRemoved);

}
template <class elementType, class ListEntryType>
inline VOID FIFOList<elementType,ListEntryType>::DEBUG_AssertNotOnList(elementType * element)      
{   
    // FIFO lists with tail pointers can be much faster at verification
    DBG_ASSERT(!IsOnList(element));

}

# endif
