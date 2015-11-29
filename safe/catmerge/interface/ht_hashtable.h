#ifndef __HT_HASHTABLE__
#define __HT_HASHTABLE__

//***************************************************************************
// Copyright (C) Data General Corporation 1989-1998
// All rights reserved.
// Licensed material -- property of Data General Corporation
//**************************************************************************/

//**************************************************************************
//  $Id$
//**************************************************************************

//++
// File Name:
//      HT_HashTable.h
//
// Description:
//      HT implements a simple, "fixed bucket" hash table. 
//
//      LOCKING
//      HT does not provide locking for the simple reason that 
//      different granularity of locking may be appropriate for 
//      different users. For example, many users might want one
//      lock protecting the entire table, while other users may
//      want one lock per bucket. The first group of users can
//      wrap the HT functions inside of one lock, while the second
//      group may associate an array of locks with the hash table,
//      and the use the HT_XXXBucket() to manipulate the hash table
//      under a per bucket lock.
//
//      MEMORY ALLOCATION
//      HT does no memory allocation or deallocation, to enable it
//      to be used in paging and non-paging environments. Sample
//      allocation code (in user space):
//
//          PHT_HASHTABLE_ANCHOR    pAnchor; 
//          LIST_ENTRY              pBuckets;
//          pAnchor = (PHT_HASHTABLE_ANCHOR) 
//                                  malloc(sizeof(HT_HASHTABLE_ANCHOR));
//          buckets = (PLIST_ENTRY) 
//                                  malloc(HT_SIZEOFBUCKETS(numberOfBuckets));
//          status = HT_HashTableInitialize(pAnchor,
//                                  pBuckets,
//                                  numberOfBuckets, 
//                                  FIELD_OFFSET(ENTRY, hashLink), ...);
//          
//      
//    
//      BASIC INTERFACE
//      The basic interface is borrowed from the DG/UX ht subsystem.
//      HT_Initialize()  - initialize the (user provided) hash table
//                         anchor
//      HT_Lookup()      - find an entry in the table associated with
//                         a particular key
//      HT_Insert()      - add an entry to the hash table, associating
//                         it with a particular key
//      HT_Remove()      - remove an entry from the array, after looking
//                         it up by key
//
//      EXTENDED INTERFACE
//      The interface has been extended with "bucket specific" functions.
//      HT_LookupBucket()- find an entry in a specific bucket
//      HT_InsertBucket()- insert an entry into a specific bucket
//
//      The bucket specific functions are intended to be used largely
//      for performance reasosn to avoid multiple calls to the
//      hash and compare functions. For example:
//
//              hashValue = ComplicatedHashValueComputation( .... );
//              KeAcquireSpinlock(&bucketLocks[hashValue];
//              status    = HT_LookupBucket(...);
//              KeReleaseSpinLock(&bucketLocks[hashValue];
//
//      The interface has also been extended with a remove function that
//      does not require that the entry be looked up in the remove function.
//      This would be used when an entry has aleady been looked up, so
//      the user has a pointer to the entry, and just wants to remove it
//      HT_RemoveEntry() - remove an entry from the hash table without
//                         looking it up, without verifying that it is in
//                         the hash table - the risk is obvious
//
//
// Contents:
//
//    Globals:
//      HT_MRU_OPTIMIZE
//
//    Types:
//      HT_STATUS
//      HT_HASHFUNC
//      HT_EQUALSFUNC
//      HT_ANCHOR
//
//    Macros:
//      HT_SUCCESS
//      HT_SIZEOFBUCKETS
//
//    Fucntions:
//      HT_Initialize()
//      HT_Lookup()
//      HT_LookupBucket()
//      HT_Insert()
//      HT_InsertBucket()
//      HT_Remove()
//      HT_RemoveEntry()
//      
//
// Revision History:
//      18-Nov-98   MWagner    Created.
//
//      22-Dec-98    MWagner   Modified (Code Review)
//
//--

#include "k10ntddk.h"


//++
//Variable:
//      HT_MRU_OPTIMIZE
//
// Description:
//      If this flag is set, then keep the hash buckets in
//      MRU (most recently used) order.
//
// Revision History:
//      15-Dec-98   MWagner     Created.
//
//      22-Dec-98   MWagner     Modified (Code Review)
//
//      11-May-99   ATaylor     Added HT_Walk* macros.  Added HT_IsEmpty().
//
//--

extern CONST LOGICAL  HT_MRU_OPTIMIZE;
//.End                                                                        

//++
// Enum:
//      HT_STATUS
//
// Description:
//      Status codes for the HashTable utility
//
// Members:
//      HT_STATUS_INVALID = -1                 : dummy, should never be
//                                               returned
//      HT_STATUS_OK                           : the operation succeeded
//      HT_STATUS_ERROR_DUPLICATE_KEY          : an entry with this key
//                                               already exists          
//      HT_STATUS_RROR_HASHVALUE_OUT_OF_RANGE  : the computed (or passed in)
//                                               hash value is out of bounds
//                                             
//      HT_STATUS_ERROR_KEY_NOT_FOUND          : the key was not found in 
//                                               the hash table
//      HT_STATUS_MAX                          : dummy, should never be
//                                               returned
//
// Revision History:
//      22-Dec-98   MWagner  Created.
//
//      22-Dec-98    MWagner   Modified (Code Review)
//
//--

typedef enum _HT_STATUS
{
    HT_STATUS_INVALID = -1,
    HT_STATUS_OK, 
    HT_STATUS_ERROR_DUPLICATE_KEY,          
    HT_STATUS_ERROR_HASHVALUE_OUT_OF_RANGE,
    HT_STATUS_ERROR_KEY_NOT_FOUND, 
    HT_STATUS_MAX
} HT_STATUS, *PHT_STATUS;
//.End  

//++
// Exported basic data types
//                               
// Type:
//      HT_HASHFUNC
//
// Description:
//      The hash function type.  The result must be
//      between 0 and (numberOfBuckets - 1). The general
//      idea is to compute some value, and return the
//      modulus of that value by the numberOfBuckets
//
// Arguments:
//      key             - the key to be hashed to a bucket
//      numberOfBuckets - the number of buckets to be hashed
//                        into
//
// Return:
//      XXX             0 <= XXX < numberOfBuckets
//
// Revision History:
//      15-Dec-98   MWagner    Created.
//
//      22-Dec-98    MWagner   Modified (Code Review)
//
//      04-Jan-99    MWagner   made key a PVOID
//--

typedef ULONG (*HT_HASHFUNC)(PVOID key, ULONG numberOfBuckets);
//.End                                                                        

//++
// Type:
//      HT_EQUALSFUNC
//
// Description:
//      The hash table compare function type.  This function 
//      must return TRUE if this key is associated with this
//      entry, FALSE otherwise.
//
// Arguments:
//      key   - the key 
//      entry - the  entry to be checked
//
// Revision History:
//      15-Dec-98   MWagner    Created.
//
//      22-Dec-98    MWagner   Modified (Code Review)
//
//      04-Jan-99    MWagner   made key a PVOID
//
//
//--

typedef BOOLEAN (*HT_EQUALSFUNC)(PVOID key, PVOID entry);
//.End                                                                        

//++
// Type:
//      HT_ANCHOR
//
// Description:
//      The "hash table"- the buckets and the associated fields.
//      The bucket array (HHA_Buckets) must also be allocated. 
//      This is an array of LIST_ENTRY. See sample code in this
//      header file - HT does no memory allocation/de-allocation.
//
// Members:
//      HHA_Buckets         : the buckets of the hash table
//      HHA_NumberOfBuckets : the number of buckets in the hash table
//      HHA_NumberOfEntries : the number of entries in the hash table
//      HHA_LinkOffset      : the size, in bytes from the beginning of
//                            the structure to the hash link to be used
//                            for this table.  For example, the value
//                            returned by FIELD_OFFSET(EntryType, hashlink).
//                            The hash link should be a PLIST_ENTRY, or some
//                            other struct with with a Flink and Blink
//      HHA_HashFunc        : the function used to determine the proper
//                            bucket for the entry, the result will be
//                            greater than zero and less than 
//                            HHA_NumberOfBuckets
//      HHA_EqualsFunc      : the function used to compare hash entries
//                            for equality
//      HHA_Flags           : flags for controlling hash table behavior;
//                            the only current flag is
//                            HT_HASHTABLE_MRU_OPTIMIZE
//--

typedef struct _HT_ANCHOR
{
    PEMCPAL_LIST_ENTRY                     HHA_Buckets;
    ULONG                           HHA_NumberOfBuckets;
    ULONG                           HHA_NumberOfEntries;
    ULONG                           HHA_LinkOffset;
    HT_HASHFUNC                     HHA_HashFunction;
    HT_EQUALSFUNC                   HHA_EqualsFunction;
    LOGICAL                         HHA_Flags;
} HT_ANCHOR, *PHT_ANCHOR;
//.End                                                                        

//++
// Exported Macros
//
// Macro:
//      HT_SUCCESS
//
// Description:
//      checks an HT_STATUS for SUCCESS
//
// Arguments:
//      HT_STATUS  status:  
//
// Return Value
//      TRUE         :   the status was OK
//      FALSE        :   the status was not OK
//
// Revision History:
//      22-Dec-98   MWagner  Created.
//
//      22-Dec-98    MWagner   Modified (Code Review)
//
//--

#define HT_SUCCESS(status)  (status == HT_STATUS_OK)
// .End 

//++
// Macro:
//      HT_SIZEOFBUCKETS
//
// Description:
//      A hash table is a variable sized array of hash table links.
//      This macro determines the amount of space (in bytes) to allocate
//      for a hash table.  The parameter num_links, is the same
//      as the parameter of the same name to ht_initialize.                     
//
// Arguments:
//      ULONG number_of_buckets    : the nuumber of buckets in the 
//                                    hash table
//--

#define HT_SIZEOFBUCKETS(number_of_buckets)                             (\
        (ULONG) (number_of_buckets * sizeof(EMCPAL_LIST_ENTRY))                 \
                                                                        )
//.End                                                                        

//++
// Exported Functions
//
// Function:
//      HT_Initialize()
//
// Description:
//      Initializes a hash table- the caller must have
//      already allocated the memory for the hash table
//      and the buckets.
//
// Arguments:
//      PHT_ANCHOR                  pAnchor         : the hash table anchor
//      PLIST_ENTRY                 buckets         : the array of buckets
//      ULONG                       numberOfBuckets : number of buckets in
//                                                    the bucket array
//      ULONG                       linkOffset      : offset of hash link 
//                                                    in the entry
//      HT_HASHFUNC                 hashFunction    : the hash function, 
//                                                    given a key, it
//                                                    returns (zero index)
//                                                    bucket number
//      HT_EQUALSFUNC               equalsFunction  : compares a key and
//                                                    an entry to see if
//                                                    the key matches the
//                                                    entry
//      LOGICAL                     flags           : 0 or set some bits
//                                                    HT_MRU_OPTIMIZE
//
// Return Values:
//      HT_SUCCESS                  the table was initialized
//
// Revision History:
//      15-Dec-98   MWagner    Created.
//
//      22-Dec-98    MWagner   Modified (Code Review)
//
//--

HT_STATUS
HT_Initialize(
            PHT_ANCHOR              pAnchor,
            PEMCPAL_LIST_ENTRY             buckets,
            ULONG                   numberOfBuckets,
            ULONG                   linkOffset,
            HT_HASHFUNC             hashFunction,
            HT_EQUALSFUNC           equalsFunction,
            LOGICAL                 flags
             );
//.End                                                                        

//++
// Function:
//      HT_IsEmpty()
//
// Description:
//      Enumerates the hash table and deterimines if the table is empty
//      (returns TRUE) or has at least one entry (returns FALSE).
//
// Arguments:
//      PHT_ANCHOR                      pAnchor     : the hash table anchor
//
// Return Values:
//      TRUE iff the hash table contains no entries.
//      FALSE iff the hash table contains at least one entry.
//
// Revision History:
//      11-May-99   ATaylor    Created.
//--

BOOLEAN
HT_IsEmpty(
            PHT_ANCHOR              pAnchor
         );
//.End

//++
// Function:
//      HT_Lookup()
//
// Description:
//      searches the hash table for an entry associated with a key
//
// Arguments:
//      PHT_ANCHOR                      pAnchor     : the hash table anchor
//      PVOID*                          pEntry      : the entry 
//      PVOID                           key         : the key 
//
// Return Values:
//      HT_SUCCESS                     pEntry points to the entry
//      HT_STATUS_ERROR_HASHVALUE_OUT_OF_RANGE
//                                      the key hashed to an out of range
//                                      value
//      HT_STATUS_ERROR_KEY_NOT_FOUND
//                                      no entry was found, pEntry is not
//                                      set
//
// Revision History:
//      15-Dec-98   MWagner    Created.
//
//      22-Dec-98    MWagner   Modified (Code Review)
//
//      04-Jan-99    MWagner   made key a PVOID
//--

HT_STATUS
HT_Lookup(
            PHT_ANCHOR              pAnchor,
            PVOID*                  pEntry,
            PVOID                   key
         );
//.End                                                                        

//++
// Function:
//      HT_LookupBucket()
//  
//
// Description:
//      This function searches for an element with a particular key
//      in a particular bucket.  The bucketNumber should be the number
//      returned by
//      (*pAnchor->HHA_HashFunction)(key, pAnchor->HHA_NumberOfBuckets)
//
//      Most callers should use HT_Lookup(), unless they
//      will gain a lot of efficiency by calling the hash function
//      once externally- I don't like exposing the structure of the
//      hash table to this extent (or allowing for people to mistakenly
//      look for an entry in the wrong bucket, but Jeff really, really
//      wanted this).
//
// Arguments:
//      PHT_ANCHOR              pAnchor          : the hash table anchor
//      PVOID*                  pEntry          : the entry 
//      PVOID                   key             : the key 
//      ULONG                   bucketNumber    : the hash bucket
//                                                in which to look
// Return Values:
//      HT_SUCCESS                     pEntry points to the entry
//      HT_STATUS_ERROR_HASHVALUE_OUT_OF_RANGE
//                                      bucketNumber is out of range
//      HT_STATUS_ERROR_KEY_NOT_FOUND
//                                      no entry was found, pEntry is not
//                                      set
//
// Revision History:
//      15-Dec-98   MWagner    Created.
//
//      22-Dec-98    MWagner   Modified (Code Review)
//
//      04-Jan-99    MWagner   made key a PVOID
//
//--

HT_STATUS
HT_LookupBucket(
            PHT_ANCHOR              pAnchor,
            PVOID*                  pEntry,
            PVOID                   key,
            ULONG                   bucketNumber
               );
//.End                                                                        

//++
// Function:
//      HT_Insert()
//
// Description:
//      insert an entry in to the hash table
//
// Arguments:
//      PHT_ANCHOR              pAnchor          : the hash table anchor
//      PVOID                   entry           : the entry to be
//                                                inserted
//      PVOID                   key             : the key to be 
//                                                associated with 
//                                                this entry
// Return Values:
//      HT_SUCCESS                     the entry was inserted in the table
//      HT_STATUS_ERROR_HASHVALUE_OUT_OF_RANGE
//                                      the key hashed to an out of range
//                                      value
//      HT_STATUS_ERROR_DUPLICATE_KEY
//                                      an entry is already associated with
//                                      this key in the table
//
// Revision History:
//      15-Dec-98   MWagner    Created.
//
//      22-Dec-98    MWagner   Modified (Code Review)
//
//      04-Jan-99    MWagner   made key a PVOID
//--

HT_STATUS
HT_Insert(
            PHT_ANCHOR              pAnchor,
            PVOID                   entry,
            PVOID                   key
         );
//.End

//++
// Function:
//      HT_InsertBucket()
//
// Description:
//      This function inserts an element into the hash table in 
//      a certain bucket. This should be used carefully, as an
//      element inserted into an incorrect bucket may be findable.
//      Callers are responsible for ensuring that bucketNumber is
//      the same number that will be returned by the call to
//      (*pAnchor->HHA_HashFunction)(key, pAnchor->HHA_NumberOfBuckets)
//      in HT_Lookup()
//
//      Most callers should use HT_Insert(), unless they
//      will gain a lot of efficiency by calling the hash function
//      once externally- I don't like exposing the structure of the
//      hash table to this extent (or allowing for people to mistakenly
//      insert an entry in the wrong bucket, but Jeff really, really
//      wanted this)
//
// Arguments:
//      PHT_ANCHOR              pAnchor          : the hash table anchor
//      PVOID                   entry           : the entry to be
//                                                inserted
//      PVOID                   key             : the key to be 
//                                                associated with 
//                                                this entry
//      ULONG                   bucketNumber    : the hash bucket 
//                                                into which to 
//                                                insert this element
//
// Return Values:
//      HT_STATUS_OK                the entry was inserted in the table
//      HT_STATUS_ERROR_HASHVALUE_OUT_OF_RANGE
//                                  bucketNumber is out of range
//      HT_STATUS_ERROR_DUPLICATE_KEY
//                                  an entry is already associated with
//                                  this key in the table
// Revision History:
//      15-Dec-98   MWagner    Created.
//
//      22-Dec-98    MWagner   Modified (Code Review)
//
//      04-Jan-99    MWagner   made key a PVOID
//--

HT_STATUS
HT_InsertBucket(
            PHT_ANCHOR              pAnchor,
            PVOID                   entry,
            PVOID                   key,
            ULONG                   bucketNumber
               );
//.End

//++
// Function:
//      HT_Remove()
//
// Description:
//      Removes the element associated with key from the hash table.
//
// Arguments:
//      PHT_ANCHOR              pAnchor          : the hash table anchor
//      PVOID                   key             : the key to be 
//                                                associated with 
//                                                this entry
// Return Values:
//      HT_STATUS_OK                    the entry was removed
//      HT_STATUS_ERROR_KEY_NOT_FOUND
//                                      no entry was found for the key
// Revision History:
//      15-Dec-98   MWagner    Created.
//
//      22-Dec-98    MWagner   Modified (Code Review)
//
//      04-Jan-99    MWagner   made key a PVOID
//--

HT_STATUS
HT_Remove(
            PHT_ANCHOR              pAnchor,
            PVOID                   key
         );
//.End

//++
// Function:
//      HT_RemoveEntry()
//
// Description:
//      Removes the entry from the hash table, without looking it
//      up first. If the entry is not in the hashtable, random 
//      memory may get trashed.
//
// Arguments:
//      PHT_ANCHOR              pAnchor          : the hash table anchor
//      PVOID                   entry           : the entry to be
//                                                removed
// Return Values:
//      HT_SUCCESS                     the entry was removed
//
// Revision History:
//      15-Dec-98   MWagner    Created.
//
//      22-Dec-98    MWagner   Modified (Code Review)
//--

HT_STATUS
HT_RemoveEntry(
            PHT_ANCHOR              pAnchor,
            PVOID                   entry
              );
//.End

//++
// Macro:
//      HT_WalkHashTable()
//
// Description:
//      This macro enumerates each bucket in a hash table.  Some applications
//      may find it useful to perform operations on each bucket.  This macro
//      is most often used in conjunction with the HT_WalkHashBucket() macro
//      to enumerate each entry in the hash table.
//
//      PHT_ANCHOR          pAnchror            = NULL;
//      PLIST_ENTRY         pCurrentBucket      = NULL;
//      PLIST_ENTRY         pCurrentEntry       = NULL;
//      ULONG               hashCount           = 0;
//
//      //
//      // Setup the hash table anchor 'pAnchor'
//      //
//
//      HT_WalkHashTable( pCurrentBucket, pAnchor, hashCount )
//      {
//          HT_WalkHashBucket( pCurrentEntry, pCurrentBucket )
//          {
//              //
//              // Application specific processing.  Use CONTAINING_RECORD()
//              // to obtain the entry in the hash table.
//              //
//          }
//      }
//
// Arguments:
//      PLIST_ENTRY             m_pCurrentBucket    : A pointer to the current
//                                                    bucket that is being
//                                                    processed in the hash
//                                                    table.
//      PHT_ANCHOR              m_pAnchor           : The hash table anchor.
//      ULONG                   m_hashCount         : A private variable used
//                                                    by the macro to terminate
//                                                    the loop.
//
// Return Values:
//      NONE
//
// Revision History:
//      11-May-99   ATaylor     Created.
//--

#define HT_WalkHashTable( m_pCurrentBucket, m_pAnchor, m_hashCount ) \
    for ( (m_pCurrentBucket) = (m_pAnchor)->HHA_Buckets, (m_hashCount) = 0; \
          (m_hashCount) < (m_pAnchor)->HHA_NumberOfBuckets; \
          (m_pCurrentBucket)++, (m_hashCount)++ )

//.End

//++
// Macro:
//      HT_WalkHashBucket()
//
// Description:
//      This macro enumerates each entry in a hash table bucket.  Some
//      applications may find it useful to perform operations on each entry
//      in a bucket.  This macro is most often used in conjunction with the
//      HT_WalkHashTable() macro to enumerate each entry in the hash table.
//
//      See code sample in HT_WalkHashTable() macro description.
//
// Arguments:
//      PLIST_ENTRY             m_pCurrentEntry     : A pointer to the current
//                                                    entry that is being
//                                                    processed in the hash
//                                                    table.
//      PLIST_ENTRY             m_pCurrentBucket    : A pointer to the bucket
//                                                    to enumerate the entries.
//
// Return Values:
//      NONE
//
// Revision History:
//      11-May-99   ATaylor     Created.
//--

#define HT_WalkHashBucket( m_pCurrentEntry, m_pCurrentBucket ) \
    for ( (m_pCurrentEntry)  = (m_pCurrentBucket)->Flink; \
          (m_pCurrentEntry) != (m_pCurrentBucket); \
          (m_pCurrentEntry)  = (m_pCurrentEntry)->Flink )
          
//.End

#endif 
