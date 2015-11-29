//************************************************************************
//
//  Copyright (c) 2002-2006 EMC Corporation.
//  All rights reserved.
//
//  ALL RIGHTS ARE RESERVED BY EMC CORPORATION.  ACCESS TO THIS
//  SOURCE CODE IS STRICTLY RESTRICTED UNDER CONTRACT. THIS CODE IS TO
//  BE KEPT STRICTLY CONFIDENTIAL.
//
//  UNAUTHORIZED MODIFICATIONS OF THIS FILE WILL VOID YOUR SUPPORT
//  CONTRACT WITH EMC CORPORATION.  IF SUCH MODIFICATIONS ARE FOR
//  THE PURPOSE OF CIRCUMVENTING LICENSING LIMITATIONS, LEGAL ACTION
//  MAY RESULT.
//************************************************************************/

#ifndef __K10HashTable__
#define __K10HashTable__
# include "ktrace.h"

//++
// File Name:
//      K10HashTable.h
//
// Contents:
//      Defines a template class for hash tables.
//
// Revision History:
//--

//
// A K10HashTable provides a collection class which is designed for fast lookup of Elements
// by Key.  
//
// Elements have the following characteristics:
//          They have a 'key' which is used to determine what to hash on. The 'key' has a
//          'KeyType', and the KeyType has a "Hash()"  function that creates a hash of the
//          key into an integer. The KeyType must have an "==" operator and ">"
//          operator.
//
//          Each element must be of the same type, and have a member called LinkField that is used
//          soley by the K10HashTable class. Each element must have a "Key()" method that returns
//          a reference to the element's key. An element's key must be constant from the time it is
//          added to the collection to when it is removed.
//
//
// An element can only be one K10HashTable at a time for each different LinkField.  A K10HashTable can hold an
// arbitrary number of elements.
//
// The NumElements parameter affects only performance. It should be on the order of the
// maximum number of elements expected to be in the hash table to get an average hash
// depth of 1.
//
// This class does not implement any concurrency control.  Besides ensuring that two threads
// are not accessing the hash table concurrently, the users of this class must
// ensure that this collection cannot change while elements are being iterated through.
//
template <class ElementType, class KeyType, const int NumElements, ElementType * ElementType::*LinkField> class K10HashTable {
public:
    inline K10HashTable();

    inline ~K10HashTable();

    // Add an element to the hash table.  It must not already be in this or any other hash
    // table.
    // @param element - the element to add.
    inline void Add(ElementType * element);

    // Search the hash table for an element with an identical key. If there are multiple
    // instances of the same key, then the first is always returned.
    //
    // @param key - the key to search for
    //
    // Return Value:
    //   NULL - No element matching 'key' was found.
    //   other - the element found.
    inline ElementType * Find(const KeyType & key) const;

    // Search the hash table for an element for which the qualifying function returns
    // TRUE.
    //
    // @param qualifier - A function that takes an Element as a parameter and returns TRUE if
    //                    Find() should return that Element, FALSE otherwise.
    //
    // Return Value:
    //   NULL - No element matching 'key' was found.
    //   other - the element found.
    inline ElementType * Find(bool (*qualifier)(const ElementType *)) const;

    // Get the first element in the hash table to start an enumeration. Can be used in
    // conjunction with the two forms of GetNext().
    //
    // @param element - if NULL, indicates that we are starting an enumeration. otherwise
    //                   points to the last element returned.
    //
    // Return Value:
    //   NULL - there are no more elements in the hash table.
    //   other - the element found.
    inline ElementType * GetFirst() const;

    // Enumerate all elements in the hash table.  Assumes the caller is locking to
    // prevent the hash table from changing.
    //
    // @param element - if NULL, indicates that we are starting an enumeration,
    //                    otherwise points to the last element returned.
    //
    // Return Value:
    //   NULL - there are no more elements in the hash table.
    //   other - the element found.
    inline ElementType * GetNext(ElementType * element = NULL) const;

    // Enumerate all elements in the hash table while allowing the hash table to change
    // in-between calls.
    //
    // @param keyOfLastElement - The value of ElementType::Key() of the previous element
    //                      returned by this function or by GetFirst().  This must be
    //                      copied from the previous element before that element could
    //                      have been freed, etc.
    //
    // Return Value:
    //   NULL - there are no more elements in the hash table.
    //   other - the element found.
    inline ElementType * GetNext(const KeyType & keyOfLastElement) const;

    // Get the next element in the hash table that has the same key as the parameter.  The
    // first element is found by calling Find(), and subsequent matching elements are found
    // by calling this function repeatedly.
    //
    // @param element - an element that is in the hash table
    //
    // Return Value:
    //   NULL - there are no more matching elements in the hash table.
    //   other - the element found.
    inline ElementType * GetNextSameKey(ElementType * element) const;

    // Remove the  element from the hash  table.
    //
    // @param element - the element to remove
    //
    // Return Value:
    //   TRUE - element removed.
    //   FALSE - element was not in the hash table.
    inline bool Remove(ElementType * element);

    // Only used in debugger extension....
    //
    // @param numElements - [out] the number of hash buckets
    //
    // Returns a pointer to the array of hash buckets.
    ElementType ** DebugGetHashTable(ULONG & numElements)  { 
        numElements = NumElements;
        return &mHashTable[0];
    }

private:
    // Determine the proper index into mHashTable for the key.
    ULONG_PTR       Hash(const KeyType & key) const;

    // This collection is implemented as an array of list heads, where each list is singly
    // linked with only a head. An element may be on only one list, and the index of that
    // list is determined by a hash of its key value. Each list is sorted by the elements'
    // key value, with no particular ordering for identical keys.
    ElementType * mHashTable[NumElements];

};

template <class ElementType, class KeyType, const int NumElements, ElementType * ElementType::*LinkField>
inline ULONG_PTR K10HashTable<ElementType,KeyType,NumElements,LinkField>::Hash(const KeyType & key) const  
{
    return key.Hash() % NumElements;
}

template <class ElementType, class KeyType, const int NumElements, ElementType * ElementType::*LinkField>
inline K10HashTable<ElementType,KeyType,NumElements,LinkField>::K10HashTable() 
{
    for (ULONG i = 0; i < NumElements; i ++) {
        mHashTable[i] = 0;
    }
}

template <class ElementType, class KeyType, const int NumElements, ElementType * ElementType::*LinkField>
inline K10HashTable<ElementType,KeyType,NumElements,LinkField>::~K10HashTable() 
{
    for (ULONG i = 0; i < NumElements; i ++) {
        DBG_ASSERT(!mHashTable[i]);
    }
}

template <class ElementType, class KeyType, const int NumElements, ElementType * ElementType::*LinkField>
inline void K10HashTable<ElementType,KeyType,NumElements,LinkField>::Add(ElementType * element)
{
    ULONG_PTR index = Hash(element->Key());

    ElementType * prev = mHashTable[index];

    // We sort the hash table entries so that the iterators will be able to search for the
    // next key.
    if (prev == NULL || prev->Key() > element->Key()) {
        // Insert the element at the head.
        element->*LinkField = prev;
        mHashTable[index] = element;
    }
    else {
        // Insert the element somewhere after the head.
        while(prev->*LinkField && element->Key() > (prev->*LinkField)->Key() ) {
            prev = prev->*LinkField;
        }
        DBG_ASSERT(prev != element);
        element->*LinkField = prev->*LinkField;
        prev->*LinkField = element;
    }

}

template <class ElementType, class KeyType, const int NumElements, ElementType * ElementType::*LinkField>
inline 
ElementType * K10HashTable<ElementType,KeyType,NumElements,LinkField>::Find(const KeyType & key) const
{
    // Find by key...
    ElementType * element = mHashTable[Hash(key)];

    while (element && !(key == element->Key())) {
        element = element->*LinkField;
    }
    return element;
}

template <class ElementType, class KeyType, const int NumElements, ElementType * ElementType::*LinkField>
inline 
ElementType * K10HashTable<ElementType,KeyType,NumElements,LinkField>::Find(bool (*qualifier)(const ElementType *)) const
{
    // Find by caller supplied criteria.
    ElementType * element;
    for(ULONG index = 0;index < NumElements; index ++) {
        ElementType * element = mHashTable[index];

        while (element) {
            if ((*qualifier)(element)) {
                return element;
            }
            element = element->*LinkField;
        }
    }
    return NULL;
}

template <class ElementType, class KeyType, const int NumElements, ElementType * ElementType::*LinkField>
inline 
ElementType * K10HashTable<ElementType,KeyType,NumElements,LinkField>::GetFirst() const
{
    ULONG  index = 0;

    // Returns the first element in the collection.
    for(;index < NumElements; index ++) {
        if (mHashTable[index]) {
            return mHashTable[index];
        }
    }
    return NULL;
}

template <class ElementType, class KeyType, const int NumElements, ElementType * ElementType::*LinkField>
inline 
ElementType * K10HashTable<ElementType,KeyType,NumElements,LinkField>::GetNext(ElementType * element) const
{
    ULONG_PTR  index = 0;

    // Find the element that is "after" this element in the collection. NULL indicates that
    // we start from the beginning of the table. We assume that element is still part of the
    // table.
    if (element) {
        if (element->*LinkField) {
            return element->*LinkField;
        }

        index = Hash(element->Key()) + 1;
    }
    for(;index < NumElements; index ++) {
        if (mHashTable[index]) {
            return mHashTable[index];
        }
    }
    return NULL;
}

template <class ElementType, class KeyType, const int NumElements, ElementType * ElementType::*LinkField>
inline 
ElementType * K10HashTable<ElementType,KeyType,NumElements,LinkField>::GetNext(const KeyType & key) const
{
    ULONG_PTR  index = Hash(key);

    ElementType * element = mHashTable[index];

    // Each hash chain is sorted by Key().  Find the first entry that has a greater key.
    while (element && key >= element->Key()) {
        element = element->*LinkField;
    }

    if (element) {
        return element;
    }

    // If we exhausted that hash chain, then return the first element of the next hash chain.
    index ++;

    // This is an iterator that is covering all of the values, but there is no guarantee to
    // the caller as to order.  We actually return keys in order on the same hash chain, but
    // across hash chains it is ordered by the hash value.
    for(;index < NumElements; index ++) {
        if (mHashTable[index]) {
            return mHashTable[index];
        }
    }
    return NULL;
}

template <class ElementType, class KeyType, const int NumElements, ElementType * ElementType::*LinkField>
inline 
ElementType * K10HashTable<ElementType,KeyType,NumElements,LinkField>::GetNextSameKey(ElementType * element) const
{
    ElementType * next = element->LinkField;

    // The hash chain is sorted by key, so if the key of the next element matches, return it.
    if (next && next->Key() == element->Key()) {
        return next;
    }
    // No match.
    return NULL;
}

template <class ElementType, class KeyType, const int NumElements, ElementType * ElementType::*LinkField>
inline 
bool K10HashTable<ElementType,KeyType,NumElements,LinkField>::Remove(ElementType * element)
{
    ULONG_PTR index = Hash(element->Key());

    // Is it at the head of the list?
    if (element == mHashTable[index]) {
        mHashTable[index] = element->*LinkField;
        return true;
    }
    else {
        ElementType * el = mHashTable[index];

        while(el) {
            if (el->*LinkField == element) {
                el->*LinkField = element->*LinkField;
                return true;
            }
            el = el->*LinkField;
        }
        return false;
    }
}




#endif  // __K10HashTable__

