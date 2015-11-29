#ifndef __K10TemplatedLockedList_h__
#define __K10TemplatedLockedList_h__

//***************************************************************************
// Copyright (C) EMC Corporation 2009-2011
// Licensed material -- property of EMC Corporation
//**************************************************************************/

//++
// File Name:
//      K10TemplatedLockedList
//
// Contents: This file contains the template class definition and code for 
// K10TemplatedLockedList class. This class can be used to impliment a single line
// of a hash table with a lock per line, using one pointer to each list head.
//  bit 0 set means line locked.
//  bit 1 set when locked means IRQL was 2, cleared means restore PASSIVE_LEVEL.
//
// Revision History: 
//      04-14-2009 -- Created. GS
//      05-07-2009 -- Updated to be a templated class and added comments. HAS
//      04-08-2010 -- IRQL support. GS
//      04-12-2010 -- Changed method embedding link field, created new type.
//      06-20-2011 -- APC support. GS
//
//--

#include "EmcKeInline.h"
#include "InlineInterlocked.h"
#include "K10Assert.h"

#define LOCK_IRQL 0x7
#define IRQL_MASK 0x6

#define DBG_ASSERT_DISPATCH_LEVEL()  DBG_ASSERT(EMCPAL_LEVEL_IS_DISPATCH())

// A K10TemplatedLockedList provides a locked hash line which is made up of elements.
// The lock represented by LeastSignificantBit of mHead pointer.
// 
// Elements have the following characteristics:
//          To declare a list head type, only a forward reference (e.g. class foo;) is required to the ElementType,
//          allowing the element to have a pointer to the list head.
//
//          Elements inherit from K10TemplatedLockedList<..>::LockedListEntry, which is where
//          their link field for this list is stored. An element may be on two locked lists simultaneously,
//          if it inherits from two different types of LockedListEntry.  
//
//          Elements have a 'key' which is used to determine which element on the list
//          is specified. The 'key' is of a 'KeyType' and must have an "==" operator and 
//          "Compare()" method. The "Compare()" method is used to insert the 'key' 
//          in a "sorted" order. 
//
//          Each element must have a "Key()" method which An element's 
//          key must be constant from the time it is added to the collection to when it is removed.
//
//
// Creates a list with only head pointer.  AddHead/RemoveHead are very fast operations, so if the
// need is Last In First Out, this list type is ideal. Remove, and operations on the tail
// (if supported) would be slower.
//
// The third parameter (uniqueLinkType) is only used to create unique types to allow multiple inheritance.
// This parameter is unused except to create different list types
template <class ElementType, class KeyType, int uniqueLinkType = 0xDEFA> class K10TemplatedLockedList 
{
 public:

    // Defines a type that ElementType should inherit from. 
    class LockedListEntry {
    public:
        LockedListEntry() : mLockedListNext(NULL) {}
    private:
        friend class K10TemplatedLockedList;

        // Next Pointer
        ElementType             * mLockedListNext;
    };

    // Default Constructor and destructor
    K10TemplatedLockedList() : mHead(0) {};
    ~K10TemplatedLockedList() {};

    // Acquire the lock for this line.
    void AcquireLock();  

    // Acquire the lock if no one has held it.
    // Returns: true if acquired, false if held elsewhere.
    bool TryLock();

    // Release the lock for this line.
    void ReleaseLock();

    // Claims that the caller has the lock.  The actual implemententation
    // is weaker, only checking that the lock is owned.
    void AssertLockHeld() const;

    void DebugAssertLockHeld() const { 
#if DBG
        AssertLockHeld(); 
#endif
    }

    // Find the element key which matches the specified key.
    // The lock is held upon return.
    // @param key - the element key desired.
    //
    // Return Value:
    //   NULL - there are no matching keys in the hash line.
    //   other - the matching element.
    ElementType * FindLockHeld(const KeyType & key);

    // Find the next element key which matches the specified key.
    // The lock is held upon return.
    // @param key - the element key desired.
    //
    // Return Value:
    //   NULL - there are no matching keys in the hash line.
    //   other - the matching element.
    ElementType * FindNextLockHeld(const KeyType & key, LockedListEntry * elem);

    // Add a element to the list of elements in this line. Assumes lock held.
    // The lock is still held upon return.
    // @param elem - Element to be added.
    // @param sort - Insert sorted or unsorted (default).
    // Returns false if second of deferred pair true otherwise (only valid with sort) 
    bool AddLockHeld(ElementType *elem, bool sort=FALSE);

    // Remove a element from the list of elements in this line. Assumes lock held.
    // The lock is still held upon return.
    // @param elem - Element to be removed.
    void RemoveLockHeld(ElementType *elem);


    // Optimize common cases: empty or single unlocked element.
    // element = FindAcquireLock(key); // element link locked, or NULL with no lock
    // hit:   ...; ReleaseLock();
    // evict: ...; RemoveReleaseLock();
    // miss:  allocate element; AddUnlocked(element)
    //
    // Find and lock element link if found otherwise, return NULL without the lock. 
    // @param desiredKey - desired element key
    //
    // Return Value:
    //   NULL - there are no matching keys in the hash line and line unlocked.
    //   other - the matching element with the lock held.
    ElementType * FindAcquireLock(const KeyType & desiredKey);

    // Add a element after missing. If line is locked by another client, lock will 
    // be acquired and then released.
    // 
    // @param elem - element to be added to the line.
    // @param sort - Insert sorted or unsorted (default).
    void AddUnlocked(ElementType *elem, bool sort=FALSE);

    // Remove element and release the lock.
    // Lock is released upon return.
    //
    // @param elem - the element to be removed.
    void RemoveReleaseLock(ElementType *elem);

    // Returns:  An element from the list, NULL if the list is empty.  Does not remove.
    ElementType * GetHead() const;

    // WINDBG extension visibility
    // void DebugDisplay(); // friend void ::DebugDisplay();
#ifndef __WINDBG_SUPPORT__
 private:
#endif

    volatile ElementType *mHead;

    // Raise IRQL if necessary, return lock and irql bits
    EMCPAL_KIRQL RaiseLockIrql();
    VOID LowerLockIrql(EMCPAL_KIRQL);

    // ExchangePVOIDLockHeld does not strictly require InterlockedExchange
    void ExchangePVOIDLockHeld(PVOID *mem, PVOID exc);
};


template <class ElementType, class KeyType, int uniqueLinkType> 
inline EMCPAL_KIRQL K10TemplatedLockedList<ElementType, KeyType, uniqueLinkType>::RaiseLockIrql()
{
#ifdef EMCKE_LOCKS_USE_IRQL_MANIPULATION
    EMCPAL_KIRQL irql; EmcpalLevelRaiseToDispatch(&irql);
    if (EMCPAL_THIS_LEVEL_IS_PASSIVE(irql)) {
        return (EMCPAL_KIRQL)0x1; // LOCK_PASSIVE /* SAFEBUG */
    } else if (EMCPAL_THIS_LEVEL_IS_APC(irql)) {
        return (EMCPAL_KIRQL)0x3; // LOCK_APC /* SAFEBUG */
    } else {
        DBG_ASSERT(EMCPAL_THIS_LEVEL_IS_DISPATCH(irql));
        return (EMCPAL_KIRQL)0x5; // LOCK_DISPATCH /* SAFEBUG */
    }
#else
    csx_p_user_preempt_disable();
    return (EMCPAL_KIRQL)0x1;
#endif
}

template <class ElementType, class KeyType, int uniqueLinkType> 
inline VOID K10TemplatedLockedList<ElementType, KeyType, uniqueLinkType>::LowerLockIrql(EMCPAL_KIRQL irql)
{
#ifdef EMCKE_LOCKS_USE_IRQL_MANIPULATION
    EmcpalLevelLower(irql); /* SAFEBUG */
#else
    CSX_UNREFERENCED_PARAMETER(irql);
    csx_p_user_preempt_enable();
#endif
}

// ExchangePVOIDLockHeld - LockHeld so InterlockedExchange is not strictly necessary.
template <class ElementType, class KeyType, int uniqueLinkType> 
inline void K10TemplatedLockedList<ElementType, KeyType, uniqueLinkType>::ExchangePVOIDLockHeld(PVOID *mem, PVOID exc)
{
#if 0 // OPTIMIZE_LOCK_HELD
    *mem = exc;
#else
    EmcpalInterlockedExchangePointer(mem, exc);
#endif
}


template <class ElementType, class KeyType, int uniqueLinkType> 
inline void K10TemplatedLockedList<ElementType, KeyType, uniqueLinkType>::AcquireLock()
{
    EMCPAL_KIRQL lock_irql = RaiseLockIrql();
    ULONG_PTR h = ((ULONG_PTR) mHead) & ~LOCK_IRQL;
    if (InterlockedCompareExchangePVOID((PVOID *)&mHead, (PVOID) (h | lock_irql), (PVOID)h) == (PVOID)h) {
        return; // AcquireLock
    }

    EmcKeSpinOnTemplatedLockedList((PVOID *)&mHead, (EMCPAL_KIRQL)((lock_irql & IRQL_MASK)>>1)); /* SAFEBUG */
}

template <class ElementType, class KeyType, int uniqueLinkType> 
inline bool K10TemplatedLockedList<ElementType, KeyType, uniqueLinkType>::TryLock()
{
    EMCPAL_KIRQL lock_irql = RaiseLockIrql();
    ULONG_PTR h = ((ULONG_PTR) mHead) & ~LOCK_IRQL;
    if (InterlockedCompareExchangePVOID((PVOID *)&mHead, (PVOID) (h | lock_irql), (PVOID)h) == (PVOID)h) {
        return true; // Acquired Lock
    }
    LowerLockIrql((EMCPAL_KIRQL)(((lock_irql & IRQL_MASK)>>1))); /* SAFEBUG */
    return false;
}

template <class ElementType, class KeyType, int uniqueLinkType> 
inline void K10TemplatedLockedList<ElementType, KeyType, uniqueLinkType>::ReleaseLock()
{
    DBG_ASSERT_DISPATCH_LEVEL();
    FF_ASSERT(((ULONG_PTR) mHead) & 0x1); // already locked
    ULONG_PTR h = (ULONG_PTR) mHead;
    // clear lock,irql bits.
    ExchangePVOIDLockHeld((PVOID *)&mHead, (PVOID)(h & ~LOCK_IRQL));
    LowerLockIrql((EMCPAL_KIRQL)(((h & IRQL_MASK)>>1))); /* SAFEBUG */
}

// FindLockHeld - find element or NULL with lock still held.
template <class ElementType, class KeyType, int uniqueLinkType> 
inline ElementType *K10TemplatedLockedList<ElementType, KeyType, uniqueLinkType>::FindLockHeld(const KeyType & requestedKey)
{
    ULONG_PTR h = (ULONG_PTR) mHead;
    FF_ASSERT(h & 0x1); // already locked
    DBG_ASSERT_DISPATCH_LEVEL();

    ElementType *elem;
    for(elem = (ElementType *)(h & ~LOCK_IRQL);
        // FIX:  with qualified name, you can't put the link field in a subclass.  I don't know why.  Could be compiler bug. 
        elem; elem = elem->K10TemplatedLockedList<ElementType,KeyType,uniqueLinkType>::LockedListEntry::mLockedListNext) {
        if (elem->Key() == requestedKey) {
            return elem; // Found elem
        }
    }
    return NULL; // Not found, still locked
}

// FindNextLockHeld - find next element or NULL with lock still held.
template <class ElementType, class KeyType, int uniqueLinkType> 
inline ElementType *K10TemplatedLockedList<ElementType, KeyType, uniqueLinkType>::FindNextLockHeld(const KeyType & requestedKey, LockedListEntry * startElem)
{
    ULONG_PTR CSX_MAYBE_UNUSED h = (ULONG_PTR) mHead;
    FF_ASSERT(h & 0x1); // already locked
    DBG_ASSERT_DISPATCH_LEVEL();

    ElementType *elem;
    for(elem = startElem->K10TemplatedLockedList<ElementType,KeyType,uniqueLinkType>::LockedListEntry::mLockedListNext;
        elem; elem = elem->K10TemplatedLockedList<ElementType,KeyType,uniqueLinkType>::LockedListEntry::mLockedListNext) {
        if (elem->Key() == requestedKey) {
            return elem; // Found elem
        }
    }
    return NULL; // Not found, still locked
}

// AddLockHeld - Add element to head of Line with lock still held.
// Returns false if second of deferred pair true otherwise (only valid with sort) 
template <class ElementType, class KeyType, int uniqueLinkType> 
inline bool K10TemplatedLockedList<ElementType, KeyType, uniqueLinkType>::AddLockHeld(ElementType *elem, bool sort)
{
    DBG_ASSERT((reinterpret_cast<ULONG_PTR>(elem) & LOCK_IRQL) == 0); // LOCK_IRQL clear in address
    ULONG_PTR h = (ULONG_PTR) mHead;
    FF_ASSERT(h & 0x1); // already locked
    DBG_ASSERT_DISPATCH_LEVEL();
    ElementType * prev = (ElementType *)(h & ~LOCK_IRQL);
    ElementType * next;
    bool first;

    if (!sort || (prev == NULL) || !(prev->Key()).Compare(elem->Key()))
    {
        ULONG_PTR helem = (ULONG_PTR) elem | ((ULONG_PTR)h & LOCK_IRQL); // Locked elem
        // Either not sorting or the list is empty so add to 
        // the head of the list.
        elem->K10TemplatedLockedList<ElementType,KeyType,uniqueLinkType>::LockedListEntry::mLockedListNext = (ElementType *)(h & ~LOCK_IRQL);
        ExchangePVOIDLockHeld((PVOID *)&mHead, (PVOID)helem);
        return true;
    }
    else
    {
        // sorted list desired so walk the list to determine
        // where the current element belongs.
        next = prev->K10TemplatedLockedList<ElementType,KeyType,uniqueLinkType>::LockedListEntry::mLockedListNext;
        while (next && (next->Key()).Compare((elem)->Key()) )
        {
            prev = next;
            next = prev->K10TemplatedLockedList<ElementType,KeyType,uniqueLinkType>::LockedListEntry::mLockedListNext;
        }
        elem->K10TemplatedLockedList<ElementType,KeyType,uniqueLinkType>::LockedListEntry::mLockedListNext = prev->K10TemplatedLockedList<ElementType,KeyType,uniqueLinkType>::LockedListEntry::mLockedListNext;
        prev->K10TemplatedLockedList<ElementType,KeyType,uniqueLinkType>::LockedListEntry::mLockedListNext = elem;
        
        first = !(prev->Key().Equal(elem->Key()));
    }
    return first;
}

// RemoveLockHeld - Remove elem from Line with lock still held.
template <class ElementType, class KeyType, int uniqueLinkType> 
inline void K10TemplatedLockedList<ElementType, KeyType, uniqueLinkType>::RemoveLockHeld(ElementType *elem)
{
    ULONG_PTR h = (ULONG_PTR) mHead;
    FF_ASSERT(h & 0x1); // already locked
    DBG_ASSERT_DISPATCH_LEVEL();
    ElementType *p = (ElementType *)(h & ~LOCK_IRQL);
    if (p == elem) {
        ULONG_PTR next = (ULONG_PTR)(elem->K10TemplatedLockedList<ElementType,KeyType,uniqueLinkType>::LockedListEntry::mLockedListNext);
        FF_ASSERT(!(next & LOCK_IRQL));
        next |= (h & LOCK_IRQL); // copy lock,irql bits
        mHead = (ElementType *)next;
        return; // Remove from head, lock still held.
    } else {
        // Validate that elem is on Line list.
        while((p != NULL) && (p->K10TemplatedLockedList<ElementType,KeyType,uniqueLinkType>::LockedListEntry::mLockedListNext != elem)) {
            p = p->K10TemplatedLockedList<ElementType,KeyType,uniqueLinkType>::LockedListEntry::mLockedListNext;
        }
        FF_ASSERT((p->K10TemplatedLockedList<ElementType,KeyType,uniqueLinkType>::LockedListEntry::mLockedListNext == elem));
        // Remove elem from Line (not mHead)
        p->K10TemplatedLockedList<ElementType,KeyType,uniqueLinkType>::LockedListEntry::mLockedListNext = elem->K10TemplatedLockedList<ElementType,KeyType,uniqueLinkType>::LockedListEntry::mLockedListNext;
    }
    return;
}

template <class ElementType, class KeyType, int uniqueLinkType> 
inline ElementType * K10TemplatedLockedList<ElementType, KeyType, uniqueLinkType>::GetHead() const
{
    ULONG_PTR h = (ULONG_PTR) mHead;
    return (ElementType *)(h & ~LOCK_IRQL);
}

template <class ElementType, class KeyType, int uniqueLinkType> 
inline VOID K10TemplatedLockedList<ElementType, KeyType, uniqueLinkType>::AssertLockHeld() const
{
    ULONG_PTR CSX_MAYBE_UNUSED h = (ULONG_PTR) mHead;
    FF_ASSERT(h & 1);
}


// Optimize common cases: empty or single unlocked element in line.
// elem = FindAcquireLock(Key); // elem link locked, or NULL with no lock

template <class ElementType, class KeyType, int uniqueLinkType> 
inline ElementType *K10TemplatedLockedList<ElementType, KeyType, uniqueLinkType>::FindAcquireLock(const KeyType & desiredKey)
{
    ULONG_PTR h = (ULONG_PTR) mHead;
    if (h == 0) {
        return NULL; // empty Line, unlocked
    }

    EMCPAL_KIRQL lock_irql = RaiseLockIrql();

    ElementType *elem;


    if (!(h & 0x1)) {
        elem = (ElementType *)(h & ~LOCK_IRQL);
        if ((elem->Key() == desiredKey) &&
            InterlockedCompareExchangePVOID((PVOID *)&mHead, (PVOID)(h | lock_irql), (PVOID)h) == (PVOID)h) {
            // Re-check the key in case the ref at the head was re-used with a new key.
            // Reload to make sure elem->Key() wasn't cached.
            elem = (ElementType *)((ULONG_PTR) mHead & ~LOCK_IRQL);
            if(elem->Key() == desiredKey) {
                return elem; // Head matched, now locked.
            }
            // Lock held, but Head not matched
        } else {
            // Not desired ref, AcquireLock
            if (!(InterlockedCompareExchangePVOID((PVOID *)&mHead, (PVOID) (h | lock_irql), (PVOID)h) == (PVOID)h)) {
                EmcKeSpinOnTemplatedLockedList((PVOID *)&mHead, ((EMCPAL_KIRQL)(lock_irql & IRQL_MASK)>>1)); /* SAFEBUG */
            }
        }
    } else {
        EmcKeSpinOnTemplatedLockedList((PVOID *)&mHead, ((EMCPAL_KIRQL)(lock_irql & IRQL_MASK)>>1)); /* SAFEBUG */
    }

    for(elem = (ElementType *)((ULONG_PTR) mHead & ~LOCK_IRQL);
        elem; elem = elem->K10TemplatedLockedList<ElementType,KeyType,uniqueLinkType>::LockedListEntry::mLockedListNext) {
        if (elem->Key() == desiredKey) {
            return elem; // Found elem, line already Locked.
        }
    }
    // Not found - ReleaseLock() inline
    h = (ULONG_PTR) mHead;
    ExchangePVOIDLockHeld((PVOID *)&mHead, (PVOID)(h & ~LOCK_IRQL));

    LowerLockIrql((EMCPAL_KIRQL)(((h & IRQL_MASK)>>1))); /* SAFEBUG */

    return NULL; // Not found, unlocked
}

// AddUnlocked - add element to head of Line without lock.
template <class ElementType, class KeyType, int uniqueLinkType> 
void K10TemplatedLockedList<ElementType, KeyType, uniqueLinkType>::AddUnlocked(ElementType *elem, bool sort)
{
    DBG_ASSERT((reinterpret_cast<ULONG_PTR>(elem) & LOCK_IRQL) == 0); // LOCK_IRQL clear in address
    ULONG_PTR h = (ULONG_PTR) mHead;
    DBG_ASSERT(EMCPAL_LEVEL_IS_OR_BELOW_DISPATCH());
    if ((h == 0) && (InterlockedCompareExchangePVOID((PVOID *)&mHead, (PVOID)elem, (PVOID)h) == (PVOID)h)) {
        return; // Elem added to empty Line, unlocked
    }

    AcquireLock();
    AddLockHeld(elem, sort);
    ReleaseLock();
}

// RemoveReleaseLock - Remove element from Line and unlock.
template <class ElementType, class KeyType, int uniqueLinkType> 
void K10TemplatedLockedList<ElementType, KeyType, uniqueLinkType>::RemoveReleaseLock(ElementType *elem)
{
    ULONG_PTR h = (ULONG_PTR) mHead;
    FF_ASSERT(h & 0x1); // already locked
    DBG_ASSERT_DISPATCH_LEVEL();
    ElementType *p = (ElementType *)(h & ~LOCK_IRQL);
    if (p == elem) {
        ULONG_PTR next = (ULONG_PTR) (elem->K10TemplatedLockedList<ElementType,KeyType,uniqueLinkType>::LockedListEntry::mLockedListNext);
        FF_ASSERT(!(next & LOCK_IRQL));
        // set next and clear lock,irql bits
        ExchangePVOIDLockHeld((PVOID *)&mHead, (PVOID)next);
        LowerLockIrql((EMCPAL_KIRQL)(((h & IRQL_MASK)>>1))); /* SAFEBUG */
        return; // ReleaseLock from head.
    } else {
        // Validate that elem is on Line list.
        while((p != NULL) && (p->K10TemplatedLockedList<ElementType,KeyType,uniqueLinkType>::LockedListEntry::mLockedListNext != elem)) {
            p = p->K10TemplatedLockedList<ElementType,KeyType,uniqueLinkType>::LockedListEntry::mLockedListNext;
        }

        if(p != NULL) {
            FF_ASSERT((p->K10TemplatedLockedList<ElementType,KeyType,uniqueLinkType>::LockedListEntry::mLockedListNext == elem));

            // Remove elem from Line (not mHead)
            p->K10TemplatedLockedList<ElementType,KeyType,uniqueLinkType>::LockedListEntry::mLockedListNext = elem->K10TemplatedLockedList<ElementType,KeyType,uniqueLinkType>::LockedListEntry::mLockedListNext;
        }
        ReleaseLock();
        return;
    }
}


#endif  // __K10TemplatedLockedList_h__

