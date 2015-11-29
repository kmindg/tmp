/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2013
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *                               ConcurrentIoGeneration.h
 ***************************************************************************
 *
 * DESCRIPTION:  Declaration of the ConcurrentIoGeneration Class
 *
 * FUNCTIONS: ConcurrentIoGeneration
 *
 * NOTES:
 *
 * HISTORY:
 *    12/18/2009  Martin Buckley Initial Version
 *
 **************************************************************************/
#ifndef __CONCURRENTIO__
#define __CONCURRENTIO__
# include "generic_types.h"
# include "k10ntddk.h"
# include "EmcPAL_DriverShell.h"
# include "BvdSimIoTestConfig.h"
# include "ConcurrentIoOperation.h"
# include "BvdSimFileIo.h"
# include "DeviceConstructor.h"
# include "simulation/AbstractFlags.h"
# include "simulation/AbstractThreadConfigurator.h"

enum ConcurrentIO_flags_e {
    ConcurrentIO_Construct_Devices = 0,
    ConcurrentIO_Sequential_threads = 1,
    ConcurrentIO_Verbose = 2,
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

class AbstractConcurrentIoThread
{
public:
    virtual void run() = 0;
};

class ConcurrentIoThread
{
public:
    ConcurrentIoThread(int threadId,
                       PEMCPAL_RENDEZVOUS_EVENT StartEvent,
                       ConcurrentIoIterator *iterator,
                       AbstractThreadConfigurator *threadConfigurator,UINT_32 ioYieldCnt);
    ~ConcurrentIoThread();

    UINT_32 getDeviceIndex();

    UINT_32 getRGIndex();
    
    void wait();
    void run();
    
    void PerformIO();

private:
    static VOID ThreadStart(PVOID StartContext);

    EMCPAL_THREAD               mThreadData;
    PEMCPAL_RENDEZVOUS_EVENT    mpStartEvent;

    UINT_32                     mThreadId;
    BvdSimFileIo_File           *mFile;
    ConcurrentIoIterator        *mIterator;
    PDeviceConstructor          mDeviceFactory;
    AbstractThreadConfigurator  *mThreadConfigurator;
    UINT_32                     mIoYieldCount;
};


class  ConcurrentIoGenerator: public SimpleFlags {
public:
    ConcurrentIoGenerator(PDeviceConstructor DeviceFactory,
                          AbstractThreadConfigurator *threadConfigurator,
                          BvdSimIoTestConfig *IoConfig,
                          UINT_64 flags = (1<<ConcurrentIO_Construct_Devices));
    ~ConcurrentIoGenerator();

    // Perform IO with either sequential or parallel threads.  This operation
    // will be synchronous, i.e. it will exit when all the threads have completed.
    void PerformIO(bool ignoreYield = false);
    
    // Perform IO with parallel threads.  This operation
    // will be asynchronous.  Use these if you would like to configure
    // a test that is running multiple types of concurrent I/O simultaneously.
    // e.g 520 and 512 format testing.
    bool InitializeParallelIOAsync(bool ignoreYield = false);
    bool StartParallelIoAsync();
    bool WaitForParallelIoAsyncCompletion();

    // Calculate the number of IOs a thread can generate before it
    // must perform a yield.  This helps prevent thread cpu starvation.
    virtual UINT_32 CalculateIoYieldCount(UINT_32 totalThd);

protected:
    static VOID ThreadStart(PVOID StartContext);

    EMCPAL_RENDEZVOUS_EVENT      mStartEvent;
    PDeviceConstructor           mDeviceFactory;
    BvdSimIoTestConfig          *mIoConfig;
    AbstractThreadConfigurator  *mThreadConfigurator;
    bool                        mConfiguredForAsync;
    ConcurrentIoIterator **     mIterators;
    ConcurrentIoThread **       mTasks;
    UINT_32                     mTotalThds;

};

#endif //__CONCURRENTIO__
