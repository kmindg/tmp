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

#ifndef __HashTable__
#define __HashTable__
# include "generic_types.h"
# include "ktrace.h"
# include "K10Assert.h"

//++
// File Name:
//      HashTable.h
//
// Contents:
//      Defines a template class for hash tables.
//
// Revision History:
//--

//
// A HashTable provides a collection class which is designed for fast lookup of Elements
// by Key.  
//
// Elements have the following characteristics:
//          They have a 'key' which is used to determine what to hash on. The 'key' has a
//          'KeyType', and the KeyType has a "Hash()"  function that creates a hash of the
//          key into an integer. The KeyType must have an "==" operator and ">"
//          operator.
//
//          Each element must be of the same type, and have a member called LinkField that is used
//          soley by the HashTable class. Each element must have a "Key()" method that returns
//          a reference to the element's key. An element's key must be constant from the time it is
//          added to the collection to when it is removed.
//
//
// An element can only be one HashTable at a time for each different LinkField.  A HashTable can hold an
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
template <class ElementType, typename KeyType, const int NumElements, ElementType * ElementType::*LinkField> class HashTable {
public:
    HashTable();

    ~HashTable();

    // Add an element to the hash table.  It must not already be in this or any other hash
    // table.
    // @param element - the element to add.
    void Add(ElementType * element);

    // Search the hash table for an element with an identical key. If there are multiple
    // instances of the same key, then the first is always returned.
    //
    // @param key - the key to search for
    //
    // Return Value:
    //   NULL - No element matching 'key' was found.
    //   other - the element found.
    ElementType * Find(const KeyType key) const;

    // Search the hash table for an element for which the qualifying function returns
    // TRUE.
    //
    // @param qualifier - A function that takes an Element as a parameter and returns TRUE if
    //                    Find() should return that Element, FALSE otherwise.
    //
    // Return Value:
    //   NULL - No element matching 'key' was found.
    //   other - the element found.
    ElementType * Find(bool (*qualifier)(const ElementType *)) const;

    // Get the first element in the hash table to start an enumeration. Can be used in
    // conjunction with the two forms of GetNext().
    //
    // @param element - if NULL, indicates that we are starting an enumeration. otherwise
    //                   points to the last element returned.
    //
    // Return Value:
    //   NULL - there are no more elements in the hash table.
    //   other - the element found.
    ElementType * GetFirst() const;

    // Enumerate all elements in the hash table.  Assumes the caller is locking to
    // prevent the hash table from changing.
    //
    // @param element - if NULL, indicates that we are starting an enumeration,
    //                    otherwise points to the last element returned.
    //
    // Return Value:
    //   NULL - there are no more elements in the hash table.
    //   other - the element found.
    ElementType * GetNext(ElementType * element = NULL) const;

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
    ElementType * GetNext(const KeyType keyOfLastElement) const;

    // Get the next element in the hash table that has the same key as the parameter.  The
    // first element is found by calling Find(), and subsequent matching elements are found
    // by calling this function repeatedly.
    //
    // @param element - an element that is in the hash table
    //
    // Return Value:
    //   NULL - there are no more matching elements in the hash table.
    //   other - the element found.
    ElementType * GetNextSameKey(ElementType * element) const;

    // Remove the  element from the hash  table.
    //
    // @param element - the element to remove
    //
    // Return Value:
    //   TRUE - element removed.
    //   FALSE - element was not in the hash table.
    bool Remove(ElementType * element);

    // Return the hash  table to its original empty state.
    // Note none of the element next pointers are reset to NULL
    // 
    void Clear();

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
    KeyType     Hash(KeyType key) const;

    // This collection is implemented as an array of list heads, where each list is singly
    // linked with only a head. An element may be on only one list, and the index of that
    // list is determined by a hash of its key value. Each list is sorted by the elements'
    // key value, with no particular ordering for identical keys.
    ElementType * mHashTable[NumElements];

};

template <class ElementType, typename KeyType, const int NumElements, ElementType * ElementType::*LinkField>
inline KeyType HashTable<ElementType,KeyType,NumElements,LinkField>::Hash(KeyType key) const  
{
    return key % NumElements;
}

template <class ElementType, typename KeyType, const int NumElements, ElementType * ElementType::*LinkField>
inline HashTable<ElementType,KeyType,NumElements,LinkField>::HashTable() 
{
    for (ULONG i = 0; i < NumElements; i ++) {
        mHashTable[i] = 0;
    }
}

template <class ElementType, typename KeyType, const int NumElements, ElementType * ElementType::*LinkField>
inline HashTable<ElementType,KeyType,NumElements,LinkField>::~HashTable() 
{
    for (ULONG i = 0; i < NumElements; i ++) {
        DBG_ASSERT(!mHashTable[i]);
    }
}

template <class ElementType, typename KeyType, const int NumElements, ElementType * ElementType::*LinkField>
inline void HashTable<ElementType,KeyType,NumElements,LinkField>::Add(ElementType * element)
{
    KeyType index = Hash(element->Key());

    ElementType * prev = mHashTable[index];
    ElementType * next;
    //KvPrint("add(%p) index %d,  mHashTable[index] %p", element, index, prev);

    // We sort the hash table entries so that the iterators will be able to search for the
    // next key.
    if (prev == NULL || prev->Key() > element->Key()) {
        // Insert the element at the head.
        //KvPrint("Adding %p to head of slot %d\n",element, index);
        element->*LinkField = prev;
        mHashTable[index] = element;
    }
    else {
        // Insert the element somewhere after the head.
        //KvPrint("Inserting %p into slot %d linked list, head %p\n", element, index, mHashTable[index]);
        while(prev->*LinkField && element->Key() > (prev->*LinkField)->Key() ) {
            next = prev->*LinkField;

            //KvPrint("prev %p next %p\n", prev, next);
            prev = next;
        }
        DBG_ASSERT(prev != element);
        element->*LinkField = prev->*LinkField;
        prev->*LinkField = element;
    }

}

template <class ElementType, typename KeyType, const int NumElements, ElementType * ElementType::*LinkField>
inline 
ElementType * HashTable<ElementType,KeyType,NumElements,LinkField>::Find(KeyType key) const
{
    // Find by key...
    ElementType * element = mHashTable[Hash(key)];

    while (element && !(key == element->Key())) {
        element = element->*LinkField;
    }
    return element;
}

template <class ElementType, typename KeyType, const int NumElements, ElementType * ElementType::*LinkField>
inline 
ElementType * HashTable<ElementType,KeyType,NumElements,LinkField>::Find(bool (*qualifier)(const ElementType *)) const
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

template <class ElementType, typename KeyType, const int NumElements, ElementType * ElementType::*LinkField>
inline 
ElementType * HashTable<ElementType,KeyType,NumElements,LinkField>::GetFirst() const
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

template <class ElementType, typename KeyType, const int NumElements, ElementType * ElementType::*LinkField>
inline 
ElementType * HashTable<ElementType,KeyType,NumElements,LinkField>::GetNext(ElementType * element) const
{
    KeyType  index = 0;

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

template <class ElementType, typename KeyType, const int NumElements, ElementType * ElementType::*LinkField>
inline 
ElementType * HashTable<ElementType,KeyType,NumElements,LinkField>::GetNext(KeyType key) const
{
    KeyType  index = Hash(key);

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

template <class ElementType, typename KeyType, const int NumElements, ElementType * ElementType::*LinkField>
inline 
ElementType * HashTable<ElementType,KeyType,NumElements,LinkField>::GetNextSameKey(ElementType * element) const
{
    ElementType * next = element->LinkField;

    // The hash chain is sorted by key, so if the key of the next element matches, return it.
    if (next && next->Key() == element->Key()) {
        return next;
    }
    // No match.
    return NULL;
}

template <class ElementType, typename KeyType, const int NumElements, ElementType * ElementType::*LinkField>
inline 
bool HashTable<ElementType,KeyType,NumElements,LinkField>::Remove(ElementType * element)
{
    KeyType index = Hash(element->Key());

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

template <class ElementType, typename KeyType, const int NumElements, ElementType * ElementType::*LinkField>
inline 
void HashTable<ElementType,KeyType,NumElements,LinkField>::Clear()
{
    for (ULONG i = 0; i < NumElements; i ++) {
        mHashTable[i] = 0;
    }
}

class AddressHash {
public:
    AddressHash()              { reference = 0; }
    AddressHash(UINT_64 value) { reference = value; }
    AddressHash(void *value)   { reference = (UINT_64)value; }
    /*
     * handle/pointer reference
     */
    UINT_64  reference;

    /*
     * K10HashTable Template requires
     *   specification of a public field (used to link items withn a bucket)
     *   existance of method Key() that returns a field that can be hashed.
     */
    AddressHash *mNextAddress;
    UINT_64  Key() { return reference; }
};

const int AddressHashTableSize = 2048;
typedef HashTable<AddressHash, UINT_64, AddressHashTableSize, &AddressHash::mNextAddress> HashOfAddresses;

#endif  // HashTable__

