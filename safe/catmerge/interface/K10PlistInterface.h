//**********************************************************************
//      Copyright (C) EMC Corporation, 2007
//      All rights reserved.
//      Licensed material - Property of EMC Corporation.
//**********************************************************************

//**********************************************************************
//.++
// FILE NAME:
//      MluPlistInterface.h
//
// DESCRIPTION:
//
//**********************************************************************

#ifndef _K10_PLIST_INTERFACE_H_
#define _K10_PLIST_INTERFACE_H_

#include "MemMgr.h"
#include "csx_ext.h"
#include "EmcPAL_List.h"


typedef PVOID K10_PLIST_HANDLE;

typedef PVOID K10_WAREHOUSE_ID;

typedef PVOID K10_PLIST_HASH_TABLE_ANCHOR;

typedef ULONG (*K10_PLIST_HASHFUNC)(IN PVOID        key,
                                    IN ULONG        NumberOfBuckets);


typedef BOOLEAN (*K10_PLIST_COMPFUNC)(IN PVOID      Key,
                                      IN PVOID      DataPtr);


#define K10PListInitializeListEntry       EmcpalInitializeListHead

//++
//  Type:
//      K10_PLIST_ENTRY
//
//  Description:
//      
//
//--


typedef struct _K10_PLIST_ENTRY
{
    EMCPAL_LIST_ENTRY       Link;
    PVOID            DataPtr;
} K10_PLIST_ENTRY, *PK10_PLIST_ENTRY;



//++
//  Type:
//      K10_PLIST_HEAD
//
// ListHead:            Head of list.
//
// WareHouseId:         Warehouse that provides list entry structures.
//
// pWarehouseLock:       Pointer to lock that protects warehouse.  Optional.  May be NULL.
//
// NumberOfEntries:     Total number of entries in the P-List
// 
//  Description:
//      The K10_PLIST_HEAD structure keeps track of the P-List and the warehouse used
//      to provide list entry structures.  If a WareHouseLock is provided when the P-List
//      is created, the "interlocked" warehouse interfaces are used when allocating or
//      freeing list entries.  This locking is required when no other external locking is
//      used around the P-List insert/remove functions.
//
//      There is no locking provided for the P-List itself.  The component using the P-List
//      must provide that, as needed.
//
//--

typedef struct _K10_PLIST_HEAD
{

   // Points to the list head
   EMCPAL_LIST_ENTRY           ListHead;

   // Warehouse identifier
   K10_WAREHOUSE_ID     WareHouseId;

   // Warehouse lock pointer.  Optional.  May be NULL.
   PEMCPAL_SPINLOCK     pWarehouseLock;

   // Number of entries on this Plist
   ULONG                NumberOfEntries;  

} K10_PLIST_HEAD, *PK10_PLIST_HEAD;

// 
// 
//  Name:
//
//      K10_PLIST_HASH_TABLE
//
//  Description:
//
//      This structure defines the binding data structure for a hash
//      table.
//
//

typedef struct _K10_PLIST_HASH_TABLE
{

    // The current number of chains.
    ULONG                               NumberOfBuckets;

    // The current number of entries in all the lists
    ULONG                               NumberOfEntries;

    // Hash Function...to get the bucket ID
    K10_PLIST_HASHFUNC                  HashFunction;

    // Hash Compare Function, to compare the entry with provided key
    K10_PLIST_COMPFUNC                  CompareFunction;

    // The array of Buckes
    K10_PLIST_HANDLE                    PListBucket[1];

}K10_PLIST_HASH_TABLE, *PK10_PLIST_HASH_TABLE;


// 
// 
//  Name:
//
//      K10_PLIST_ITERATOR
//
//  Description:
//
//      This structure is used to iterate the Plist
//
//

typedef struct _K10_PLIST_ITERATOR
{

    PEMCPAL_LIST_ENTRY            pListEntry;   
    PK10_PLIST_HEAD        pPListHead;

}K10_PLIST_ITERATOR, *PK10_PLIST_ITERATOR;


//
//  Use this macro to adjust the current entry pointer while removing the 
//  entries from the plist.
// 

#define K10_REWIND_PLIST_ENTRY(_m_EntryOffset) \
        ((_m_EntryOffset) = ((PEMCPAL_LIST_ENTRY)_m_EntryOffset)->Blink)

//
// Macro used to convert plist entry to data while walking plist
// 

 
#define K10_PLIST_ENTRY_TO_DATA(_m_EntryOffset) \
        ((PK10_PLIST_ENTRY)CSX_CONTAINING_STRUCTURE(_m_EntryOffset , K10_PLIST_ENTRY, Link))->DataPtr


#define HANDLE_TO_PLIST_HEAD(h) ((PK10_PLIST_HEAD)(h))

//
// Macro to walk plist
// 


#define K10WalkPList( _m_plist_handle , _m_entry ) \
    for ( (_m_entry) =  (((PK10_PLIST_HEAD)(_m_plist_handle))->ListHead.Flink);\
          (_m_entry) != &(((PK10_PLIST_HEAD)(_m_plist_handle))->ListHead);\
          (_m_entry) =  (((PEMCPAL_LIST_ENTRY)_m_entry)->Flink) )
//
// Macro to walk Phash table

#define K10WalkPHashTable(_m_phash_anchor , _m_current_bucket , _m_bucketcnt )  \
    for ( (_m_bucketcnt) = 0, (_m_current_bucket) = ((PK10_PLIST_HASH_TABLE)(_m_phash_anchor))->PListBucket[0];\
          (_m_bucketcnt) < ((PK10_PLIST_HASH_TABLE)(_m_phash_anchor))->NumberOfBuckets;\
          ((_m_current_bucket) = ((PK10_PLIST_HASH_TABLE)(_m_phash_anchor))->PListBucket[++(_m_bucketcnt)] ) )   
          

EMCPAL_STATUS
K10InitializeWarehouse(IN ULONG                 NumberOfEntries,
                       OUT K10_WAREHOUSE_ID     *WareHouseId);

EMCPAL_STATUS
K10UninitializeWarehouse(IN K10_WAREHOUSE_ID    WareHouseId);


EMCPAL_STATUS
K10CreatePList(IN  K10_WAREHOUSE_ID     WareHouseId,
               IN  PEMCPAL_SPINLOCK     pWarehouseLock,
               OUT K10_PLIST_HANDLE    *pPListHandle);


BOOLEAN
K10IsPListEmpty(IN K10_PLIST_HANDLE     pPListHandle);


EMCPAL_STATUS
K10PListInsertAtHead(IN K10_PLIST_HANDLE        pPListHandle,
                      IN PVOID                  DataPtr);


EMCPAL_STATUS
K10PListInsertAtTail(IN K10_PLIST_HANDLE        pPListHandle,
                      IN PVOID                  DataPtr);


EMCPAL_STATUS
K10PListInsert(IN K10_PLIST_HANDLE      pPListHandle,
                IN PVOID                DataPtr);



PVOID
K10PListRemoveFromHead (IN K10_PLIST_HANDLE     pPListHandle);


PVOID
K10PListRemoveFromTail(IN K10_PLIST_HANDLE      pPListHandle);


VOID
K10PListRemoveEntry(IN K10_PLIST_HANDLE     pPListHandle,
                    IN PVOID               *EntryPtr );


EMCPAL_STATUS
K10PListFindAndRemove(IN K10_PLIST_HANDLE       pPListHandle,
                      IN PVOID                  DataPtr);


EMCPAL_STATUS
K10PListStartIterator (IN  K10_PLIST_HANDLE           pPListHandle,
                       OUT PK10_PLIST_ITERATOR        pIterator);

EMCPAL_STATUS
K10PListGetNextEntry (IN OUT PK10_PLIST_ITERATOR         pIterator,
                      OUT    PVOID                      *DataPtr);


VOID
K10DestroyPList(IN K10_PLIST_HANDLE     pPListHandle);


ULONG
K10PListGetEntryCount(IN K10_PLIST_HANDLE       pPListHandle);


EMCPAL_STATUS
K10CreatePHashTable (IN K10_WAREHOUSE_ID                WareHouseId,
                     IN ULONG                           NumberOfBuckets,
                     IN K10_PLIST_HASHFUNC              HashFunction,
                     IN K10_PLIST_COMPFUNC              CompareFunction,
                     OUT K10_PLIST_HASH_TABLE_ANCHOR    *pAnchor);



EMCPAL_STATUS
K10PHashLookup (IN K10_PLIST_HASH_TABLE_ANCHOR           pAnchor,
                IN PVOID                                 Key,
                OUT PVOID                                *DataPtr);


EMCPAL_STATUS
K10PHashInsertEntry (IN K10_PLIST_HASH_TABLE_ANCHOR pAnchor,
                     IN PVOID                           DataPtr,
                     IN PVOID                           Key);


// Remove from the current entry that is tracked by walk list

EMCPAL_STATUS
K10PHashRemoveEntry(IN K10_PLIST_HASH_TABLE_ANCHOR      pAnchor,
                    IN PVOID                            Key,
                    IN PVOID                            DataPtr );


VOID
K10PHashDestroy (IN K10_PLIST_HASH_TABLE_ANCHOR pAnchor);


ULONG
K10PHashGetEntryCount (IN K10_PLIST_HASH_TABLE_ANCHOR   pAnchor);


#endif // _K10_PLIST_INTERFACE_H_
