/***************************************************************************
 * Copyright (C) EMC Corporation 2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *                               ListOfIoOperations.h
 ***************************************************************************
 *
 * DESCRIPTION:  The ListOfIoOperations class maintains a list of ConcurrentIoOperations
 *
 * FUNCTIONS:
 *
 * NOTES:
 *
 * HISTORY:
 *    03/15/2011  Komal Padia Initial Version
 *
 **************************************************************************/
 #ifndef __LISTOFIOOPERATIONS__
 #define __LISTOFIOOPERATIONS__

 #include "generic_types.h"
 #include "SinglyLinkedList.h"
 #include "ConcurrentIoOperation.h"

 #define MAX_IO_OPERATIONS_COUNT 1000

 SLListDefineListType(ConcurrentIoOperation, ListOfIoOps); 
 SLListDefineInlineCppListTypeFunctions(ConcurrentIoOperation, ListOfIoOps, mNext);
 
 class ListOfIoOperations {
 public:
    ListOfIoOperations(AbstractStreamConfig *config, UINT_32 deviceIndex); 
    ~ListOfIoOperations(); 
    ConcurrentIoOperation* GetNextIOOperation(); 

    UINT_32 getDeviceIndex() { return mDeviceIndex; }

    ListOfIoOperations *mNext; // For use in a singly-linked list    
private:
    void GenerateNewListLockHeld(); 
    void TryRefillListOfIoOpsLockHeld(); 

    ListOfIoOps mListOfIoOps; 
    ListOfIoOps mListOfIoOpsInProgress; 
    ListOfIoOps mFreeListOfIoOps; 
    
    UINT_32 mNumOfElements; 
    EMCPAL_MUTEX mMutex;
    int mNoOfTimesListShouldBeRegenerated; 

    BvdSimIo_stream_type_t mStreamType; 
    UINT_32 mStartBlock; 
    UINT_32 mEndBlock; 
    UINT_32 mLength; 

    // The lun this list is associated with
    UINT_32 mDeviceIndex; 
 };

 #endif
