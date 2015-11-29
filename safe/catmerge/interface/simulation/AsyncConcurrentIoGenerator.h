/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2014
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *                               AsyncConcurrentIoGeneration.h
 ***************************************************************************
 *
 * DESCRIPTION:  Declaration of the AsyncConcurrentIoGeneration Class
 *
 * FUNCTIONS: AsyncConcurrentIoGeneration
 *
 * NOTES:
 *
 * HISTORY:
 *    02/18/2009  Martin Buckley Initial Version
 *    02/16/2014  MJC version
 *
 **************************************************************************/
#ifndef __ASYNCCONCURRENTIO__
#define __ASYNCCONCURRENTIO__

# include "generic_types.h"
# include "k10ntddk.h"
# include "EmcPAL_DriverShell.h"
# include "BvdSimIoTestConfig.h"
# include "BvdSimFileIo.h"
# include "DeviceConstructor.h"
# include "k10ntddk.h"
# include "flare_ioctls.h"
# include "simulation/AbstractFlags.h"
# include "simulation/AbstractThreadConfigurator.h"
# include "simulation/AsyncConcurrentIOStreamConfig.h"
# include "simulation/BvdSimIoTypes.h"
# include "simulation/BvdSimIOUtilityBaseClass.h"
# include "simulation/VirtualFlareDevice.h"
# include "Cache/CacheDriverStatistics.h"

enum AsyncConcurrentIO_flags_e {
    AsyncConcurrentIO_Construct_Devices = 0,
    AsyncConcurrentIO_Verbose = 2,
};

// In order to not to starve other threads in the system, we need to control
// the Number of IOs a thread will issue before yielding the cpu.   So what
// we will do is use the function FN(MAX_IO_COUNT_WITHOUT_YIELD - (((totalThd > 100 ? 100 : totalThd)/10) * 12);
// to determine the number of IOs that a thread will do before yielding the CPU.
//
// The above function will produce the following results:
//      ThreadCount               IOs before yield
//          =>100                       12
//          =>90                        12
//          =>80                        24
//          =>70                        36
//          =>60                        48
//          =>50                        60
//          =>40                        72
//          =>30                        84
//          =>20                        96
//          =>10                       108
//           > 0                       120

#define MAX_IO_COUNT_WITHOUT_YIELD 120

typedef struct _AsyncIoThreadBlock {
        BvdSimFile_IoAsyncBlock asyncBlock;
        BvdSimIo_type_t         ioType;
        BvdSimIo_type_t         methodType;
} AsyncIoThreadBlock, *PAsyncIoThreadBlock;

typedef struct _OPERATION_STATISTICS {
    UINT_32 volume;
    UINT_32 raidGroup;
    UINT_64 operationsPerformed;
    UINT_64 seconds;
    UINT_64 IOPS; 
    UINT_64 operationTypes[(UINT_64) ASYNC_MAX];
} Operation_Statistics, *POperation_Statistics;

class AsyncConcurrentIoThread : public BvdSimIOUtilityBaseClass
{
public:
    AsyncConcurrentIoThread(int threadId,
                       PEMCPAL_RENDEZVOUS_EVENT StartEvent,
                       UINT_32 dev,
                       UINT_32 rg,
                       PDeviceConstructor deviceFactory,
                       BvdSimIoTestConfig* config,
                       AbstractThreadConfigurator *threadConfigurator,UINT_32 ioYieldCnt);
    ~AsyncConcurrentIoThread();

    UINT_32 getDeviceIndex();

    UINT_32 getRGIndex();
    
    void wait();
    void run();
    
    void PerformIO();
    
    void GetPerformanceInfo(UINT_32& volume, UINT_32& rg, UINT64& totalOperations, EMCPAL_TIME_100NSECS&  totalTime, UINT_64* operationsArray)
    {
        volume = mVolume;
        rg = mRaidGroup;
        totalOperations = mTotalOperations;
        totalTime = mTotalTime;
        
        UINT_64 (&statisticsArray)[(UINT_64) ASYNC_MAX]= 
            *reinterpret_cast<UINT_64 (*)[(UINT_64) ASYNC_MAX] >(operationsArray);
        
        EmcpalCopyMemory(statisticsArray, mOperationPerformed,sizeof(mOperationPerformed));
    }
    

private:
    static VOID ThreadStart(PVOID StartContext);
    
    void ExecuteAsyncOperations();
    void CalculateIoLimits();
    void createFile();
    void InitializeRandomInteger64();
    AsyncIoSequence getNextIoSequence();
    UINT_64 GetRandomInteger64(UINT_64 max);
    void getNextOperation();
    bool CanAnotherRequestBeIssued();

    EMCPAL_THREAD               mThreadData;
    PEMCPAL_RENDEZVOUS_EVENT    mpStartEvent;

    char                        mDeviceName[K10_DEVICE_ID_LEN];
    UINT_32                     mThreadId;
    BvdSimFileIo_File           *mFile;
    BvdSimFileIo_File           *mFileSync;
    PDeviceConstructor          mDeviceFactory;
    AbstractThreadConfigurator  *mThreadConfigurator;
    BvdSimIoTestConfig          *mIoConfig;
    AsyncConcurrentIoStreamConfig* mAsyncConfig;
    UINT_32                     mIoYieldCount;
    csx_rt_ux_rand64_context_t  mPRNGContext64;

    UINT_32 mVolume;
    UINT_32 mRaidGroup;
    UINT_64 mEndBlock;
    UINT_64 mIoRange;
    UINT_64 mStartBlock;
    UINT_32 mOutstandingIoCount;
    UINT_32 mOperationsToPerform;
    AsyncIoSequence mCurrentIoOperation;
    BOOL    mFirstPass;
    UINT_32 mIterations;
    UINT_64 mSectors;
    UINT_64 mStartLBA;
    UINT_64 mPeriod;
    
    BOOL    mRandomLBA;
    BOOL    mRandomLength;
    
    bool mEnteredBurstMode;
    bool mUnlimitedOperations;

    VirtualFlareVolume * FlareVolume(BvdSimFileIo_File* bfdFile) const;
    AsyncIoThreadBlock*  mAsyncBlockArray; // [MAX_NUMBER_OF_OUTSTANDING_IOS];
    BOOL* mAsyncFreeArray; // [MAX_NUMBER_OF_OUTSTANDING_IOS];

    void WaitForAsyncIoToComplete(BOOL waitForAll = FALSE);
    UINT_32 FindFreeAsyncBlock();
    
    UINT_64               mTotalOperations;
    EMCPAL_TIME_100NSECS  mTotalTime;
    EMCPAL_TIME_100NSECS  mShortestTime;
    EMCPAL_TIME_100NSECS  mLongestTime;
    
    UINT_64                mOperationPerformed[(UINT_32) ASYNC_MAX];
};

class  AsyncConcurrentIoGenerator: public SimpleFlags {
public:
    AsyncConcurrentIoGenerator(PDeviceConstructor DeviceFactory,
                          AbstractThreadConfigurator *threadConfigurator,
                          BvdSimIoTestConfig *IoConfig,
                          UINT_64 flags = (1<<AsyncConcurrentIO_Construct_Devices));
    ~AsyncConcurrentIoGenerator();

    // Perform IO with parallel threads.  This operation
    // will be asynchronous.  Use these if you would like to configure
    // a test that is running multiple types of concurrent I/O simultaneously.
    // e.g 520 and 512 format testing.
    bool InitializeParallelIOAsync(bool ignoreYield = false);
    bool StartParallelIoAsync();
    bool WaitForParallelIoAsyncCompletion(POperation_Statistics statistics = NULL);

    // Calculate the number of IOs a thread can generate before it
    // must perform a yield.  This helps prevent thread cpu starvation.
    virtual UINT_32 CalculateIoYieldCount(UINT_32 totalThd);

protected:
    static VOID ThreadStart(PVOID StartContext);

    EMCPAL_RENDEZVOUS_EVENT      mStartEvent;
    PDeviceConstructor           mDeviceFactory;
    BvdSimIoTestConfig          *mIoConfig;
    AbstractThreadConfigurator  *mThreadConfigurator;
    AsyncConcurrentIoThread **       mTasks;
    UINT_32                     mTotalThds;

};

#endif //__ASYNCCONCURRENTIO__
