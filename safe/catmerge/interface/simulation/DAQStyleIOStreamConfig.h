/***************************************************************************
 * Copyright (C) EMC Corporation 2013
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *                               DAQStyleIOStreamConfig.h
 ***************************************************************************
 *
 * DESCRIPTION:  BvdSim File IO DAQ-like IO Test characterization class
 *                         DAQ-Style IO mimics the pattern generation by DAQ-test on the array
 *
 * FUNCTIONS:
 *
 * NOTES:
 *
 * HISTORY:
 *    03/04/2011  Komal Padia Initial Version
 *
 **************************************************************************/
#ifndef __DAQSTYLEIOSTREAMCONFIG__
#define __DAQSTYLEIOSTREAMCONFIG__

#include "generic_types.h"
#include "ConcurrentIoOperation.h"
#include "AbstractStreamConfig.h"
#include "SinglyLinkedList.h"
#include "ListOfIoOperations.h"

SLListDefineListType(ListOfIoOperations, LunWiseIoOpsList); 
SLListDefineInlineCppListTypeFunctions(ListOfIoOperations, LunWiseIoOpsList, mNext);

class DAQStyleIoStreamConfigIterator: public ConcurrentIoIterator {
public:
    DAQStyleIoStreamConfigIterator(AbstractStreamConfig *config, PDeviceConstructor deviceFactory, UINT_32 deviceIndex, UINT_32 RGIndex, UINT_32 thdId);

    ~DAQStyleIoStreamConfigIterator(); 

    PBvdSimFileIo_File getFile();
    
    AbstractStreamConfig* getStreamConfig(); 

    PIOTaskId getTaskID() { return mTaskID; }

    UINT_32 getDeviceIndex() { return mDeviceIndex; }

    UINT_32 getRGIndex() { return mRGIndex; }

    ConcurrentIoOperation* next(); 

    DAQStyleIoStreamConfigIterator* mNextIterator; // For use with singly linked list

private:
    void setDeviceIndex(PDeviceConstructor deviceFactory, UINT_32 index);

    char                    mDeviceName[K10_DEVICE_ID_LEN];
    UINT_32                 mDeviceIndex;
    UINT_32                 mRGIndex;
    BvdSimFileIo_File       *mFile;
    PIOTaskId               mTaskID;

    // DAQ-style I/O requires a list of IO operations per lun
    static LunWiseIoOpsList mLunWiseList; 
    static UINT_32 mTotalNumOfDevices; 
    
    ListOfIoOperations *mpListOfIoOps;

    // Stream config to which this iterator belongs
    AbstractStreamConfig *mConfig; 
};


SLListDefineListType(DAQStyleIoStreamConfigIterator, ListOfDAQStyleIoIterators); 
SLListDefineInlineCppListTypeFunctions(DAQStyleIoStreamConfigIterator, ListOfDAQStyleIoIterators, mNextIterator);

class DAQStyleIoStreamConfig: public AbstractStreamConfig {
public:
    DAQStyleIoStreamConfig(UINT_32 startBlock,
                         UINT_32 endBlock,
                         UINT_32 blocksToTransfer,
                         UINT_32 iterations,
                         BvdSimIo_stream_type_t streamType,
                         BvdSimIo_type_t readType,
                         BvdSimIo_type_t writeType,
                         BvdSimIo_type_t zeroFillType = SIMIO_METHOD_NONE,
                         PIOTaskId taskID = NULL,
                         UINT_32 postCreateSleep = 1)
                         :  mStartBlock(startBlock), mEndBlock(endBlock), mBlocksToTransfer(blocksToTransfer), 
                            mIterations(iterations), mStreamType(streamType), mReadType(readType), 
                            mWriteType(writeType),mZeroFillType(zeroFillType), mTaskID(taskID), 
                            mPostCreateSleep(postCreateSleep), mNumOfIoIterators(0) {}; 

    ~DAQStyleIoStreamConfig(); 

    UINT_32 getStartBlock() { return mStartBlock; }
    void setStartBlock(UINT_32 startBlock) { mStartBlock = startBlock; }

    UINT_32 getEndBlock() { return mEndBlock; }
    void setEndBlock(UINT_32 endBlock) { mEndBlock = endBlock; }

    UINT_32 getBlocksToTransfer()  { return mBlocksToTransfer; }
    void setBlocksToTransfer(UINT_32 blocksToTransfer) { mBlocksToTransfer = blocksToTransfer; }

    UINT_32 getIterations() { return mIterations; }  
    void setIterations(UINT_32 iterations) { mIterations = iterations; }  

    BvdSimIo_stream_type_t getStreamType() { return mStreamType; }
    void setStreamType(BvdSimIo_stream_type_t type) { mStreamType = type; }

    BvdSimIo_type_t getReadType() { return mReadType; }
    void setReadType(BvdSimIo_type_t type) { mReadType = type; }

    BvdSimIo_type_t getWriteType() { return mWriteType; }
    void setWriteType(BvdSimIo_type_t type) { mWriteType = type; }

    BvdSimIo_type_t getZeroFillType() { return mZeroFillType; }
    void setZeroFillType(BvdSimIo_type_t type) { mZeroFillType = type; }
    
    PIOTaskId getTaskID() { return mTaskID; }
    void setTaskID(PIOTaskId taskID) { mTaskID = taskID; }

    UINT_32 getPostCreateSleep() { return mPostCreateSleep; }
    void setPostCreateSleep(UINT_32 PostCreateSleep) { mPostCreateSleep = PostCreateSleep; }

    IoStyle_t GetIoStyle() { return DAQ_STYLE_IO; } 

    bool IsSectorSizeRandom() { return true; }

    ConcurrentIoIterator* GetIoIterator(PDeviceConstructor deviceFactory, UINT_32 deviceIndex, UINT_32 RGIndex, UINT_32 thdId, UINT_32 thdTotal);
    
private:
    UINT_32 mStartBlock;
    UINT_32 mEndBlock;
    UINT_32 mBlocksToTransfer;
    UINT_32 mIterations;
    BvdSimIo_stream_type_t mStreamType;
    BvdSimIo_type_t mReadType;
    BvdSimIo_type_t mWriteType;
    BvdSimIo_type_t         mZeroFillType;
    PIOTaskId               mTaskID;
    UINT_32                   mPostCreateSleep;

    ListOfDAQStyleIoIterators mIteratorList; 
    UINT_32 mNumOfIoIterators; 
};

#endif
