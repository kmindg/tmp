/***************************************************************************
 * Copyright (C) EMC Corporation 2014
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *                               AsyncConcurrentIOStreamConfig.h
 ***************************************************************************
 *
 * DESCRIPTION: Defines the AsyncConcurrentIOStreamConfig class    
 *       
 *       
 *
 * FUNCTIONS:
 *
 * NOTES:
 *     All I/Os are done asynchronously, the user should be aware that 
 *     that validating data can be problematic.   We cannot guarantee
 *     that some other thread is not write the same sector that is being
 *     verified via another thread.  
 *
 * HISTORY:
 *    02/16/2014 MJC Initial Version
 *
 **************************************************************************/
 #ifndef __ASYNCCONCURRENTIOSTREAMCONFIG__
 #define __ASYNCCONCURRENTIOSTREAMCONFIG__

 #include "generic_types.h"
 #include "AbstractStreamConfig.h"
 #include "ConcurrentIoOperation.h"
 #include "SinglyLinkedList.h"
 #include "simulation/BvdSimFileIo.h"
 #include "simulation/IOTaskId.h"
 #include "simulation/ConcurrentIoGenerator.h"
 #include "simulation/shmem.h"
 #include "simulation/VirtualFlareDevice.h"

#define DEFAULT_OUTSTANDING_IO_COUNT 100
#define DEFAULT_VFD_THROTTLING_ENABLED FALSE
#define DEFAULT_VALIDATING_READ_DATA TRUE

typedef enum  _AsyncIoSequence {
    ASYNC_WRITE = 0,
    ASYNC_READ = 1,
    ASYNC_ZEROFILL = 2,
    ASYNC_MAX = 3,
} AsyncIoSequence, *PAsyncIoSequence;



class AsyncConcurrentIoStreamConfig: public AbstractStreamConfig {
public:
    AsyncConcurrentIoStreamConfig(UINT_32 startBlock,
                         UINT_32 endBlock,
                         UINT_32 blocksToTransfer,
                         UINT_32 iterations,
                         BvdSimIo_stream_type_t streamType,
                         BvdSimIo_type_t readType,
                         BvdSimIo_type_t writeType,
                         BvdSimIo_type_t zeroFillType = SIMIO_METHOD_NONE,
                         UINT_32 outstandingIOs = DEFAULT_OUTSTANDING_IO_COUNT,
                         BOOL vfdThrottlingEnabled = DEFAULT_VFD_THROTTLING_ENABLED,
                         BOOL validateReadData = DEFAULT_VALIDATING_READ_DATA,
                         PIOTaskId taskID = NULL,
                         UINT_32 postCreateSleep = 1,
                         UINT_32 strideInBlocks=0,
                         bool performBurstIo = false,
                         bool refAlignIo = false);
    AsyncConcurrentIoStreamConfig();

    ~AsyncConcurrentIoStreamConfig(); 

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

    UINT_32 getStrideInBlocks() const CSX_CPP_OVERRIDE { return mStrideInBlocks; }
    void setStrideInBlocks(UINT_32 strideInBlocks) { mStrideInBlocks = strideInBlocks; }

    UINT_32 getOutstandingIOs() { return mOutstandingIOs; }  
    void setOutstandingIOs(UINT_32 ios) { mOutstandingIOs = ios; }  
    
    BOOL getVFDThrottlingEnabled() { return mVFDThrottlingEnabled; }
    void setVFDThrottlingEnabled(BOOL enabled = TRUE) { mVFDThrottlingEnabled = enabled; }
    
    BOOL getValidateReadData() { return mValidateReadData; }
    void SetValidateReadData(BOOL enabled = TRUE) { mValidateReadData = enabled; }

    IoStyle_t GetIoStyle() { return BVD_SIM_IO; }
    
    BOOL getMixedIoEnabled() { return mMixedIo; }
    AsyncIoSequence getMixedIoMax() { return mMixedIoMax; }
    
    BOOL OverriddingDeviceIoResponses() { return mOverrideDeviceIOResponses;}
    
    bool IsBurstModeEnabled() { return mBurstModeEnabled; }

    bool IsSectorSizeRandom(); 
    
    bool IsRefAlignedIo() { return mRefAlignedIo; }

    ConcurrentIoIterator* GetIoIterator(PDeviceConstructor deviceFactory, UINT_32 deviceIndex, UINT_32 RGIndex, UINT_32 thdId, UINT_32 totalThd) { return NULL; };
    
    static void OverrideDeviceIOResponse(DeviceIOResponses ssdResponse,
                    DeviceIOResponses fibreResponse,
                    DeviceIOResponses sasResponse,
                    DeviceIOResponses ataResponse);
    
    static void GetOveriddenDeviceIOResponse(DeviceIOResponses* responses);

protected:
    UINT_32 mStartBlock;
    UINT_32 mEndBlock;
    UINT_32 mBlocksToTransfer;
    UINT_32 mIterations;
    UINT_32 mOutstandingIOs;
    UINT_32 mStrideInBlocks;

    UINT_32 mCurrentStartBlock;
    UINT_32 mCurrentDeviceIndex;
    bool mFirstDeviceCreation; 
    bool mBurstModeEnabled;
    BOOL mVFDThrottlingEnabled;
    BOOL mValidateReadData;
    BOOL mMixedIo;
    bool mRefAlignedIo;
    AsyncIoSequence mMixedIoMax;
    
    BvdSimIo_stream_type_t  mStreamType;
    BvdSimIo_type_t         mReadType;
    BvdSimIo_type_t         mWriteType;
    BvdSimIo_type_t         mZeroFillType;
    PIOTaskId               mTaskID;
    UINT_32                 mPostCreateSleep;
    
    static DeviceIOResponses mDeviceIOResponses[4];
    static bool mOverrideDeviceIOResponses;
};

 #endif
