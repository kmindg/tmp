#ifndef __TemplatedDoublyLinkedList__
#define __TemplatedDoublyLinkedList__

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
//      TemplateDoublyLinkedList.h
//
// Contents:
//      Template class to create and manipulate doubly linked lists. These
//      doubly linked lists are typed, and type safety is provided by the compiler.
//
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
//              typedef DLNKList<Foo>  ListOfFoo;
//
//      3) Define element type, inheriting from ListEntry type:
//              class Foo : public ListOFFoo::ListEntry { ... }
//      4) You can define other list types, using arbitary, but unique 2nd parameters
//         typedef DLNKList<Foo, 23> AnotherListOfFooOnWhichFooCanBEOnSimultaneously;
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


// This class is used to generate default link type for DLNKList. 
//
// The second parameter is only used to create unique types to allow multiple inheritance.
// This parameter is unused except to create different list types
template <class elementType, int uniqueID = 0xDEFA>  
class DefaultDLNKListEntry {
    public:
        DefaultDLNKListEntry() : mForwardLink(reinterpret_cast<elementType*> (DebugWasRemoved)), mBackwardLink(NULL) {}


        enum { DebugWasRemoved =1 };
        elementType *    mForwardLink;
        elementType *    mBackwardLink;
    };


// List with head and tail pointer and two link fields.
//
// The second parameter allows use of an arbitrary class to inherit from, so long as it
// has a field name "mForwardLink, mBackwardLink" of type "elementTpe *".  This would enable elementType
// to sometimes be on a LIFO list, and other times on an doubly linked list, but using the same
// pair of link  fields.
template <class elementType, class ListEntryType = DefaultDLNKListEntry<elementType> >
class DLNKList 
{

public:
    typedef  ListEntryType ListEntry;

    DLNKList() : mDLNKHead(NULL), mDLNKTail(NULL) {} 
    ~DLNKList() { DBG_ASSERT(mDLNKHead == NULL);}                     

    // Add to the front of the list.
    // @param element - the element to add to the list.  Must not be NULL. The caller is
    //                  responsible for locking the list during this call.
    VOID AddHead(elementType * element);

    // Add to the end of the list.
    // @param element - the element to add to the list.  Must not be NULL. The caller is
    //                  responsible for locking the list during this call.
    VOID AddTail(elementType * element);

    // Remove an element from the front of the list.  Returns NULL if the list is empty.
    // The caller is responsible for locking the list during this call.
    elementType * RemoveHead();  

    // Remove an element from the tail of the list.  Returns NULL if the list is empty.
    // The caller is responsible for locking the list during this call.
    // WARNING: for compatibility across list types.  This can be very expensive.
    elementType * RemoveTail();     

    // Remove the specified element from the list. The caller is responsible locking the list during this
    // call.
    // Returns: true if the element was removed, FALSE if it was not on the list.
    bool Remove(elementType * element);

    // Move the specified element to tail of the list. The caller is responsible locking the list during this
    // call.
    // Returns: true if the element was moved to, FALSE if it was not on the list.
    bool MoveToTail(elementType * element);

    // Iterator to walk the list.  With no arguments, returns the element on the front of
    // the list, otherwise the subsequent element.  Must not be called with an element
    // which is not currently on the list.  If Remove() is called during iteration, the
    // element removed must not be subsequently passed in to this function. The caller is
    // responsible for locking the list during this call. Returns NULL if element is tail of
    // list.
    elementType * GetNext(elementType * element = 0) const;

    // Iterator to walk the list from tail. With no arguments, returns the element on the tail of
    // the list, otherwise the previous element.  Must not be called with an element
    // which is not currently on the list.  If Remove() is called during iteration, the
    // element removed must not be subsequently passed in to this function. The caller is
    // responsible for locking the list during this call. Returns NULL if element is head of
    // list.
    elementType * GetPrevious(elementType * element = 0) const;

    
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
    VOID MoveList(DLNKList &  from);

    // Insert an element before the specified listelement.
    // Listelement is already on the list
    // @param newelement: element to be inserted
    // @param listelement: listelement is a node already on the list.
    //                     if NULL, insert at head
    VOID InsertBefore(elementType * newelement, elementType * listelement = NULL);

protected:
    // Value to store in Link field when item is not on list.  For debug only.  No code should test this. 
    enum { DebugWasRemoved =1 };

    VOID DEBUG_ClearLink(elementType * e);
    VOID DEBUG_AssertNotOnList(elementType * e);     
    // We want this to be the full type for the debugger...
    elementType * mDLNKHead; 
    elementType * mDLNKTail; 

    // Non-sensical operations since they would leave items on two lists, but used for persistent memory recovery.
    DLNKList(const DLNKList & other) : mDLNKHead(other.mDLNKHead), mDLNKTail(other.mDLNKTail) {}

    DLNKList & operator = (const DLNKList & other); 


   
};

template <class elementType, class ListEntryType>
inline void DLNKList<elementType,ListEntryType>::AddHead(elementType * element)               
{  
#if defined(UMODE_ENV) || defined(SIMMODE_ENV)
    // For simulation, search entire list to check whether the element is already on list 
    // or not.
    DEBUG_AssertNotOnList(element);             
#else
    // For hardware, just compare the element with the head.
    // NOTE: On hardware search entire list takes significant time.
    DBG_ASSERT(mDLNKHead != element);
#endif

    element->ListEntryType::mForwardLink = mDLNKHead;
    element->ListEntryType::mBackwardLink = NULL;
    if(mDLNKHead == NULL) {
        mDLNKTail = element;
    }
    else {
        mDLNKHead->ListEntryType::mBackwardLink = element;
    }
    mDLNKHead = element;    
}                                                                  

template <class elementType, class ListEntryType>
inline void DLNKList<elementType,ListEntryType>::AddTail(elementType * element)                
{                                                                  
#if defined(UMODE_ENV) || defined(SIMMODE_ENV)
    // For simulation, search entire list to check whether the element is already on list 
    // or not.
    DEBUG_AssertNotOnList(element);             
#else
    // For hardware, just compare the element with the head.
    // NOTE: On hardware search entire list takes significant time.
    DBG_ASSERT(mDLNKHead != element);
#endif

    element->ListEntryType::mForwardLink =  NULL;
    if (mDLNKHead == NULL) {
        mDLNKHead = element;
        element->ListEntryType::mBackwardLink = NULL;
    }
    else {
        element->ListEntryType::mBackwardLink = mDLNKTail;
        mDLNKTail->ListEntryType::mForwardLink = element;
    }
    mDLNKTail = element;
}

template <class elementType, class ListEntryType>
inline elementType *  DLNKList<elementType,ListEntryType>::RemoveHead()                        
{                                                                  
    elementType * result;  

    result = mDLNKHead;
    if(result) {
        FF_ASSERT(mDLNKHead->ListEntryType::mBackwardLink == NULL);
        mDLNKHead = result->ListEntryType::mForwardLink;
        if(mDLNKHead) {
            FF_ASSERT(mDLNKHead->ListEntryType::mBackwardLink == result);
            mDLNKHead->ListEntryType::mBackwardLink = NULL;
        }
        else {
            mDLNKTail = NULL;
        }
        FF_ASSERT(mDLNKHead != reinterpret_cast<elementType*> (DebugWasRemoved));
        DEBUG_ClearLink(result) ;
    }
    return (result);
}

template <class elementType, class ListEntryType>
inline elementType *  DLNKList<elementType,ListEntryType>::RemoveTail()                        
{                                                                  
    elementType * result;

    result = mDLNKTail;
    if(result) {
        FF_ASSERT(result->ListEntryType::mForwardLink == NULL);
        if(result->ListEntryType::mBackwardLink) {
            FF_ASSERT(result->ListEntryType::mBackwardLink->ListEntryType::mForwardLink == result);
            result->ListEntryType::mBackwardLink->ListEntryType::mForwardLink = NULL;
            mDLNKTail = result->ListEntryType::mBackwardLink;
        }
        else {
            FF_ASSERT(mDLNKHead == mDLNKTail);
            mDLNKHead = NULL;
            mDLNKTail = NULL;
        }
        DEBUG_ClearLink(result);
    }
    else {
        FF_ASSERT(mDLNKHead == NULL);
    }
    return (result);
}

template <class elementType, class ListEntryType>
inline bool DLNKList<elementType,ListEntryType>::Remove(elementType * element)                
{ 
    FF_ASSERT(element->ListEntryType::mForwardLink != reinterpret_cast<elementType*> (DebugWasRemoved));
    FF_ASSERT(element->ListEntryType::mBackwardLink != reinterpret_cast<elementType*> (DebugWasRemoved));
    if(element->ListEntryType::mForwardLink) { /* Not tail */
        FF_ASSERT(element->ListEntryType::mForwardLink->ListEntryType::mBackwardLink == element);
        if(element->ListEntryType::mBackwardLink) {	/* middle of list */ 
            FF_ASSERT(element->ListEntryType::mBackwardLink->ListEntryType::mForwardLink == element);
            element->ListEntryType::mBackwardLink->ListEntryType::mForwardLink = element->ListEntryType::mForwardLink;
            element->ListEntryType::mForwardLink->ListEntryType::mBackwardLink = element->ListEntryType::mBackwardLink;
        }
        else { /* At Head */
            FF_ASSERT(mDLNKHead == element);
            mDLNKHead = element->ListEntryType::mForwardLink;
            mDLNKHead->ListEntryType::mBackwardLink = NULL;
        }
    }
    else { /* At Tail */
        FF_ASSERT(mDLNKTail == element);
        if(element->ListEntryType::mBackwardLink) {
            FF_ASSERT(element->ListEntryType::mBackwardLink->ListEntryType::mForwardLink == element);
            element->ListEntryType::mBackwardLink->ListEntryType::mForwardLink = NULL;
            mDLNKTail = element->ListEntryType::mBackwardLink;
        }
        else {   /* Only element */   
            FF_ASSERT(mDLNKHead == mDLNKTail);
            mDLNKHead = NULL;
            mDLNKTail = NULL;
        }
    }
    DEBUG_ClearLink(element);
    return true;
}   


template <class elementType, class ListEntryType>
inline bool DLNKList<elementType,ListEntryType>::MoveToTail(elementType * element)      
{
    // already in tail
    if (element == mDLNKTail ) {
        return true;
    }
    else { 
        if (Remove(element)) {
            AddTail(element);
            return true;
        }
    }
    return false;
}


template <class elementType, class ListEntryType>
inline bool DLNKList<elementType,ListEntryType>::IsEmpty() const                              
{                                                                  
    return mDLNKHead == NULL;                                 
}

template <class elementType, class ListEntryType>
inline bool DLNKList<elementType,ListEntryType>::HasMultipleItems()   const                   
{                                                                  
    return (mDLNKHead!=NULL) && (mDLNKHead->ListEntryType::mForwardLink!= NULL);             
}             

template <class elementType, class ListEntryType>
inline void DLNKList<elementType,ListEntryType>::MoveList(DLNKList<elementType,ListEntryType> & from) 
{ 
    if(from.mDLNKHead) {
        if (mDLNKHead) {
            mDLNKTail->ListEntryType::mForwardLink = from.mDLNKHead;
            mDLNKTail->ListEntryType::mForwardLink->ListEntryType::mBackwardLink = mDLNKTail;
            mDLNKTail = from.mDLNKTail;
        }
        else {
            mDLNKHead = from.mDLNKHead;
            mDLNKTail = from.mDLNKTail;
        }
        from.mDLNKHead = NULL; 
        from.mDLNKTail = NULL; 
    }
}

template <class elementType, class ListEntryType>
inline elementType *  DLNKList<elementType,ListEntryType>::GetNext(elementType * element) const
{ 
    if (element == NULL) {
        return mDLNKHead;
    }
    else {
        return element->ListEntryType::mForwardLink;
    }
}

template <class elementType, class ListEntryType>
inline elementType *  DLNKList<elementType,ListEntryType>::GetPrevious(elementType * element) const
{ 
    if (element == NULL) {
        return mDLNKTail;
    }
    else {
        return element->ListEntryType::mBackwardLink;
    }
}


template <class elementType, class ListEntryType>
inline    bool DLNKList<elementType,ListEntryType>::IsOnList(elementType * element)   const {
    if (element->ListEntryType::mForwardLink == reinterpret_cast<elementType*> (DebugWasRemoved)) {
        return false;
    }
    elementType * next = mDLNKHead;
    int i = 0;
    while(next) {
        if(i++ > 1000) { break; }
        if (next == element) { return TRUE; }
        next = next->ListEntryType::mForwardLink;
    }
    return false;
}

template <class elementType, class ListEntryType>
inline VOID DLNKList<elementType,ListEntryType>::InsertBefore(elementType * newelement, elementType * listelement)                
{
    #if defined(UMODE_ENV) || defined(SIMMODE_ENV)
        // For simulation, search entire list to check whether the element is already on list 
        // or not.
        DEBUG_AssertNotOnList(newelement);             
    #else
        // For hardware, just compare the element with the head and tail.
        // NOTE: On hardware search entire list takes significant time.
        DBG_ASSERT(mDLNKHead != newelement);
        DBG_ASSERT(mDLNKTail != newelement);
    #endif

    (newelement)->ListEntryType::mForwardLink = NULL;
    (newelement)->ListEntryType::mBackwardLink = NULL;
    if(mDLNKHead == NULL)
    {                        
        // Empty list
        mDLNKHead = mDLNKTail = newelement;
    }                                                   
    else if((listelement) == mDLNKHead)
    {
        // Add to head of list
        (newelement)->ListEntryType::mForwardLink = mDLNKHead;
        (listelement)->ListEntryType::mBackwardLink = newelement;
        mDLNKHead = newelement;
    }
    else if((listelement) == NULL)
    {
        // Add to tail of list
        (newelement)->ListEntryType::mBackwardLink = mDLNKTail;
        mDLNKTail->ListEntryType::mForwardLink = newelement;
        mDLNKTail = newelement;
    }
    else
    {
        // Insert in middle of list
        (newelement)->ListEntryType::mForwardLink = listelement;
        (newelement)->ListEntryType::mBackwardLink = (listelement)->ListEntryType::mBackwardLink;
        (listelement)->ListEntryType::mBackwardLink->ListEntryType::mForwardLink = newelement;
        (listelement)->ListEntryType::mBackwardLink = newelement;
    }
}

template <class elementType, class ListEntryType>
inline    VOID DLNKList<elementType,ListEntryType>::DEBUG_ClearLink(elementType * element) {

    // NOTE: this is done for debuggability, but no assumptions should be made about
    // the state of the link pointers for items not on lists.
    element->ListEntryType::mForwardLink = reinterpret_cast<elementType*> (DebugWasRemoved);
    element->ListEntryType::mBackwardLink = reinterpret_cast<elementType*> (DebugWasRemoved);

}
template <class elementType, class ListEntryType>
inline VOID DLNKList<elementType,ListEntryType>::DEBUG_AssertNotOnList(elementType * element)      
{   
    DBG_ASSERT(!IsOnList(element));

}

# endif
