/***************************************************************************
 * Copyright (C) EMC Corporation 2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *                               ConcurrentIoOperation.h
 ***************************************************************************
 *
 * DESCRIPTION: The ConcurrentIoOperation class defines the IO Operation to be performed
 *                        by a BvdSimIOTest
 * 
 * FUNCTIONS:
 *
 * NOTES:
 *
 * HISTORY:
 *    03/10/2011  Komal Padia Initial Version
 *
 **************************************************************************/
#ifndef __CONCURRENTIOIOPERATION__
#define __CONCURRENTIOIOPERATION__

#include "generic_types.h"
#include "BvdSimIoTypes.h"
#include "BvdSimFileIo.h"
#include "AbstractStreamConfig.h"
#include "mut.h"
#include "AbstractSystemConfigurator.h"
typedef enum operation_e {
    WRITE,
    READ, 
    WRITE_READ,
    DONE
}operation_t;

class ConcurrentIoOperation {
public:
    ConcurrentIoOperation(UINT_32 startLBA = 0, UINT_32 sectors = 0, operation_t op = DONE)
        : mStartLBA(startLBA), mSectors(sectors), mOp(op), mCompleted(false) {}; 
    UINT_32 GetStartLBA() const { return mStartLBA; }  
    UINT_32 GetSectors() const { return mSectors; }
    operation_t GetOperation() const { return mOp; }
    bool IsComplete() const { return mCompleted; }
        
    void SetOperation(UINT_32 startLBA, UINT_32 sectors, operation_t operation) {
        mStartLBA = startLBA;
        mSectors = sectors;
        mOp = operation;
    }
    
    void SetOperation(operation_t op) { mOp = op; } 
    void MarkIncomplete() { mCompleted = false; }

    void PerformIO(BvdSimFileIo_File* file, AbstractThreadConfigurator  * threadConfigurator,
                   AbstractStreamConfig *config,
                   UINT_32 threadId,
                   PIOTaskId taskID);

    ConcurrentIoOperation *mNext; // For use in a singly linked list

private:
    UINT_32 mStartLBA; 
    UINT_32 mSectors; 
    operation_t mOp; 
    bool mCompleted; 
};

#endif
