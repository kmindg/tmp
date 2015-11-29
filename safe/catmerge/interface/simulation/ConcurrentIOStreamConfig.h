/***************************************************************************
 * Copyright (C) EMC Corporation 2009,2013
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *                               ConcurrentIOStreamConfig.h
 ***************************************************************************
 *
 * DESCRIPTION: Defines the ConcurrentIOStreamConfig class    
 *       
 *       
 *
 * FUNCTIONS:
 *
 * NOTES:
 *
 * HISTORY:
 *    03/10/2011 Komal Padia Initial Version
 *
 **************************************************************************/
 #ifndef __CONCURRENTIOSTREAMCONFIG__
 #define __CONCURRENTIOSTREAMCONFIG__

 #include "generic_types.h"
 #include "AbstractStreamConfig.h"
 #include "ConcurrentIoOperation.h"
 #include "SinglyLinkedList.h"
 #include "simulation/BvdSimFileIo.h"
 #include "simulation/IOTaskId.h"
 #include "simulation/ConcurrentIoGenerator.h"
# include "simulation/shmem.h"

// Used to communicate Merge parameters to SPs from Control Leg.
typedef struct _MERGEIO_CONTROL_MEMORY {
    ULONG numOfMerges;
    ULONGLONG maxDiskSizeInSectors;
} MERGEIO_CONTROL_MEMORY, *PMERGEIO_CONTROL_MEMORY;

// Shared memory class that can be used to share memory between components of a
// 3 legged test.   You should strive to keep the name that you are going to use
// for your shared memory unique so that it does not conflict with some other test
// that may be running at the same time.
class ConcurrentIoSharedMemory
{
public:
    ConcurrentIoSharedMemory() : mMemoryAddress(0), mMemorySize(0) {}
    ~ConcurrentIoSharedMemory() {}

    // Create of block of shared memory if it does not already exist.  If
    // it already exists return a pointer to the existing block.
    // @param name of the shared memory region to create
    // @param memSize - size of the shared memory block created in bytes.
    BOOL Create(char* const memName,UINT_32 memSize)
    {
        if(!memName || !memSize) {
            return false;
        }

        // See if we have already mapped memory with this class.
        if(mMemoryAddress)
        {
            if(memSize != mMemorySize) {
                return false;
            }
            return true;
        }

        // Try to open up a block of shared memory that already exists
        shmem_segment_desc* pSegDesc =
                shmem_get_segment_using_id(shmem_format_segment_id(memName,memSize));

        // If the segment does not already exist then we will try to create it.  If we can't we fail.
        if(!pSegDesc) {
            // Try to create the shared memory segment because it does not exist.
            pSegDesc = shmem_segment_create(memName, memSize);
            if(!pSegDesc) {
                return false;
            }
        }

        // Get the base address of where the shared memory starts
        mMemoryAddress = shmem_segment_get_base(pSegDesc);

        if(mMemoryAddress) {
            mMemorySize = memSize;
            return true;
        }

        return false;
    }

    // Attempt to get an address to an already existing block of shared memory
    // @param name of the shared memory region to create
    // @param memSize - size of the shared memory block created in bytes.
    BOOL OpenExisting(char* const memName, UINT_32 memSize)
    {
        if(!memName || !memSize) {
            return false;
        }

        // Make sure that we have not already mapped memory with this class
        if(mMemoryAddress) {
            if(memSize != mMemorySize) {
                return false;
            }
            return true;
        }

        // Try to open up a block of shared memory that already exists
        shmem_segment_desc* pSegDesc =
            shmem_get_segment_using_id(shmem_format_segment_id(memName, memSize));

        // Get the base address of where the shared memory starts
        mMemoryAddress = shmem_segment_get_base(pSegDesc);

        if(mMemoryAddress) {
            mMemorySize = memSize;
            return true;
        }

        return false;
    }

    // Return the address of the shared memory block that was allocated.
    void* GetMemoryAddress()
    {
        return mMemoryAddress;
    }

private:
    void*   mMemoryAddress;
    UINT_32 mMemorySize;
};


class ConcurrentIoStreamConfigIterator: public ConcurrentIoIterator {
public:
    ConcurrentIoStreamConfigIterator(AbstractStreamConfig *config, UINT_32 startBlock, UINT_32 ioRange, PDeviceConstructor deviceFactory, 
            UINT_32 index, UINT_32 RGIndex, PMERGEIO_CONTROL_MEMORY mergeControlMemory = NULL, ULONG* mergeIoMemory = NULL, 
            ULONG mergeIoIndex = 0);

    PBvdSimFileIo_File getFile();
    
    AbstractStreamConfig* getStreamConfig() CSX_CPP_OVERRIDE;

    PIOTaskId getTaskID() { return mTaskID; }

    UINT_32 getDeviceIndex() { return mDeviceIndex; }

    UINT_32 getRGIndex() { return mRGIndex; }

    ConcurrentIoOperation* next() CSX_CPP_OVERRIDE;

    ConcurrentIoStreamConfigIterator* mNextIterator; // For use with singly linked list
    
    void* getControlMemoryAddress() const CSX_CPP_OVERRIDE { return mMergeControlMemoryAddress; }

    void* getDataMemoryAddress() const CSX_CPP_OVERRIDE { return mMergeDataMemoryAddress; }

protected:
    void setDeviceIndex(PDeviceConstructor deviceFactory, UINT_32 index);

    char                    mDeviceName[K10_DEVICE_ID_LEN];
    UINT_32                 mDeviceIndex;
    UINT_32                 mRGIndex;
    BvdSimFileIo_File       *mFile;
    PIOTaskId               mTaskID;

    ConcurrentIoOperation mIoOperation; 

    // We use the following members differently from that in the generic stream config    
    UINT_32 mIterations; 
    UINT_32 mStartBlock; 
    UINT_32 mEndBlock; 
    UINT_32 mIoRange;
    
    // Stream config to which this iterator belongs
    AbstractStreamConfig *mConfig;

    PMERGEIO_CONTROL_MEMORY     mMergeControlMemoryAddress;
    ULONG*                      mMergeDataMemoryAddress;
    ULONG                       mMergeIoIndex;

    void InitializeRandomInteger32();
    UINT_32 GetRandomInteger32(UINT_32 max);

    csx_rt_ux_rand32_context_t mPRNGContext32;
    };


#define MERGE_IO_CONTROL_MEMORY_NAME "MergeIoMemoryControl"
#define MERGE_IO_DATA_MEMORY_NAME "MergeIoMemory"


 SLListDefineListType(ConcurrentIoStreamConfigIterator, ListOfConcurrentIoIterators); 
 SLListDefineInlineCppListTypeFunctions(ConcurrentIoStreamConfigIterator, ListOfConcurrentIoIterators, mNextIterator);

class ConcurrentIoStreamConfig: public AbstractStreamConfig {
public:
    ConcurrentIoStreamConfig(UINT_32 startBlock,
                         UINT_32 endBlock,
                         UINT_32 blocksToTransfer,
                         UINT_32 iterations,
                         BvdSimIo_stream_type_t streamType,
                         BvdSimIo_type_t readType,
                         BvdSimIo_type_t writeType,
                         BvdSimIo_type_t zeroFillType = SIMIO_METHOD_NONE,
                         PIOTaskId taskID = NULL,
                         UINT_32 postCreateSleep = 1,
                         UINT_32 strideInBlocks=0);
    ConcurrentIoStreamConfig();

    ~ConcurrentIoStreamConfig(); 

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

    IoStyle_t GetIoStyle() { return BVD_SIM_IO; }

    bool IsSectorSizeRandom(); 

    ConcurrentIoIterator* GetIoIterator(PDeviceConstructor deviceFactory, UINT_32 deviceIndex, UINT_32 RGIndex, UINT_32 thdId, UINT_32 totalThd);

    // Configure memory to be shared between MergeIo concurrent Io Threads.
    // @param numOfMerges - number of merges to perform
    // @param diskSizeInSectors - size of disk in sectors
    virtual bool ConfigureMergeMemory(ULONG numOfMerges, ULONGLONG diskSizeInSectors = DEFAULT_DISK_SIZE_IN_SECTORS) {

        if(!mMergeConfiguration) {
            return false;
        }

        csx_ic_id_t masterIc = mut_get_master_ic_id();

        csx_size_t strSize  = csx_p_strsize(MERGE_IO_CONTROL_MEMORY_NAME) + sizeof(masterIc)*2;
        char* str = (char*) malloc((UINT_32) strSize);
        if(!str) {
            return false;
        }

        csx_p_memzero(str,strSize);
        csx_rt_print_sprintf(str,"%s%x",MERGE_IO_CONTROL_MEMORY_NAME,masterIc);

        if(!mMergeIoMemoryControl.Create(str,sizeof(MERGEIO_CONTROL_MEMORY))) {
            free(str);
            return false;
        }

        mPMergeIoControlMemory = static_cast<PMERGEIO_CONTROL_MEMORY>(mMergeIoMemoryControl.GetMemoryAddress());
        mPMergeIoControlMemory->numOfMerges = numOfMerges;
        mPMergeIoControlMemory->maxDiskSizeInSectors = diskSizeInSectors;

        csx_p_memzero(str,strSize);
        csx_rt_print_sprintf(str,"%s%x",MERGE_IO_DATA_MEMORY_NAME,masterIc);

        if(!mMergeIoMemory.Create(str,numOfMerges*sizeof(ULONG))) {
            free(str);
            return false;
        }

        mPMergeIoMemory = static_cast<ULONG*>(mMergeIoMemory.GetMemoryAddress());

        free(str);

        return true;
    }

    virtual ULONG* GetMergeIoMemory() {
        return mPMergeIoMemory;
    }

protected:
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
    UINT_32 mStrideInBlocks;

    UINT_32 mCurrentStartBlock;
    UINT_32 mCurrentDeviceIndex;
    bool mFirstDeviceCreation; 
    
    ListOfConcurrentIoIterators mIteratorList; 
    UINT_32 mNumOfIoIterators;

    ConcurrentIoSharedMemory     mMergeIoMemory;
    ConcurrentIoSharedMemory     mMergeIoMemoryControl;
    PMERGEIO_CONTROL_MEMORY      mPMergeIoControlMemory;
    ULONG*                       mPMergeIoMemory;
    bool                         mMergeConfiguration;
};

 #endif
