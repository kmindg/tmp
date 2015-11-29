//***********************************************************************
//
//  Copyright (c) 2011-2015 EMC Corporation.
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
//
//***********************************************************************

#ifndef __BvdSim3LeggedTest_h__
#define __BvdSim3LeggedTest_h__

#include "simulation/BvdSim_admin.h"
#include "simulation/BvdSim_dmc.h"
#include "simulation/BvdSim3LStateTestEventLog.h"
#include "EmcPAL_Misc.h"
#include "core_config_runtime.h"

#define Default3LeggedTimeout 5*60*1000    // 5 minutes in milliseconds
#define Default3LeggedPollFrequence 1000   // milliseconds
#define DefaultSPHaltTimeout 30*1000            // 30 seconds in milliseconds

// Cache has debounce time of 61 seconds plus 12 second bundle timeout attempting
// to notify the peer SP. After marking any volume as offline, we will wait for this
// much time and then start polling to verify whether volume goes offline or not. Under 
// heavy load in simulation enviroment, timer DPC takes time to schedule and it may
// take more time then expected so polling would be the better choice. We poll for
// sometime and still after that volume doesn't go offline then we declare it as error.
static const int faultedVolumeDebounceWaitTime = 80 * 1000; // 80 seconds.
static const int faultedVolumePollingIteration = 30;
static const int faultedVolumePollingInterval = 2 * 1000; // poll at every 2 seconds.

// The intended use case for this class is to define a set of standard Task names that
// a control leg can use to signal their test legs as to what task to run.   The values
// associated with these names correspond to bit locations in a 32 bit field that is used
// to signal a Test Leg (TL).   When a Control Leg calls the RunTaskOnSPx function this results in
// the bit being set in a shared memory location(s) that both test legs monitor and the task
// associated with the bit being run.
//
// Inheritors are free to use more user friendly names but must ensure that they inherit
// from the following values.    Some values/names are reserved for use by the MUT framework.
class BvdSim3LeggedTestInterface {
public:

    // Control/status bit definitions.
    // Up to 32 bits per SP may be defined.
    // The definitions below are common to both SPs.


    // The following task numbers can be overloaded by users.

    // Task IDs                                 Set by:     Cleared by:
    static const int TASK_1         = 1;    //  Control     SP
    static const int TASK_2         = 2;    //  Control     SP
    static const int TASK_3         = 3;    //  Control     SP
    static const int TASK_4         = 4;    //  Control     SP
    static const int TASK_5         = 5;    //  Control     SP
    static const int TASK_6         = 6;    //  Control     SP
    static const int TASK_7         = 7;    //  Control     SP
    static const int TASK_8         = 8;    //  Control     SP
    static const int TASK_9         = 9;    //  Control     SP
    static const int TASK_10        = 10;   //  Control     SP
    static const int TASK_11        = 11;   //  Control     SP
    static const int TASK_12        = 12;   //  Control     SP
    static const int TASK_13        = 13;   //  Control     SP
    static const int TASK_14        = 14;   //  Control     SP
    static const int TASK_15        = 15;   //  Control     SP
    static const int TASK_16        = 16;   //  Control     SP
    static const int TASK_17        = 17;   //  Control     SP
    static const int TASK_18        = 18;   //  Control     SP
    static const int TASK_19        = 19;   //  Control     SP
    static const int TASK_20        = 20;   //  Control     SP
    static const int TASK_21        = 21;   //  Control     SP
    static const int TASK_22        = 22;   //  Control     SP
    static const int TASK_23        = 23;   //  Control     SP
    static const int TASK_24        = 24;   //  Control     SP
    static const int TASK_25        = 25;   //  Control     SP
    static const int TASK_26        = 26;   //  Control     SP

    // The following task numbers are reserved for simulation.

    // Signal an SP to start a task
    static const int SIGNAL_SP      = 27;   //  Control     SP 
    // SP status bits 
    static const int SP_RUNNING     = 28;   //  SP          Control 
    static const int SP_SHUTDOWN    = 29;   //  Control     SP  
    // Task status bits
    static const int TASK_RUNNING   = 30;   //  SP          Control
    static const int TASK_COMPLETED = 31;   //  SP          Control

};

class BvdSim3LeggedTestControl : public BvdSim3LeggedTestInterface,  public BvdSimDistributedBaseClass {
public:

    BvdSim3LeggedTestControl(BvdSimDistributedTestControlBaseClass *control,
        const char *uniqueServiceName, const char *processName, UINT_32 sharedMemSize = 0)
        : mControl(control), BvdSimDistributedBaseClass(uniqueServiceName, processName, sharedMemSize)
        , mManualPmpOperations(false)
    {
        mSPControl = new SPControl();
        mDistrNvRamMem = new PersistentNvRamMemory();
        mSPBootedBefore[SPA] = mSPBootedBefore[SPB] = false;
        mSPShutdownManually[SPA] = mSPShutdownManually[SPB] = false;

    }

    ~BvdSim3LeggedTestControl()
    {
        delete mSPControl;
        delete mDistrNvRamMem;
    }

    void Initialize (BvdSimDistributedTestControlBaseClass *control)
    {
        mControl = control;
    }
    
    void BootSP(SPIdentity_t sp, CACHE_HDW_STATUS status=CACHE_HDW_OK, bool wait=true)
    {
        BvdSim_Trace(MUT_LOG_HIGH, "ControlRunner: Boot SP%c.", sp ? 'B' : 'A');
        if (sp == 0) {
            set_SPA_flag(SP_SHUTDOWN, FALSE);
            mSPControl->Reset(SPA);
            SetSpsStatus(SPA,status);
            mControl->Boot_SP(SPA);
            if (wait) {
                wait_for_SPA_status(SP_RUNNING, TRUE);
            }
        }
        else {
            set_SPB_flag(SP_SHUTDOWN, FALSE);
            mSPControl->Reset(SPB);
            SetSpsStatus(SPB,status);
            mControl->Boot_SP(SPB);
            if (wait) {
                wait_for_SPB_status(SP_RUNNING, TRUE);
            }
        }
    }

    // Serially boot SPA then SPB.
    void BootSpaSpb(CACHE_HDW_STATUS status=CACHE_HDW_OK)
    {
        BootSP(SPA, status);
        BootSP(SPB, status);
    }

    // Simultaneously boot both SPs. There is no guarantee of which SP will start first.
    void BootBothSPs(CACHE_HDW_STATUS status=CACHE_HDW_OK, ULONG freq=1000, ULONG timeout=Default3LeggedTimeout)
    {
        BvdSim_Trace(MUT_LOG_HIGH, "ControlRunner: Boot both SPs.");
        set_SPA_flag(SP_SHUTDOWN, FALSE);
        set_SPB_flag(SP_SHUTDOWN, FALSE);
        mSPControl->Reset(SPA);
        mSPControl->Reset(SPB);
        SetSpsStatus(SPA,status);
        SetSpsStatus(SPB,status);
        mControl->Boot_SP(SPA);
        mControl->Boot_SP(SPB);
        wait_for_SPA_status(SP_RUNNING, TRUE, freq, timeout);
        wait_for_SPB_status(SP_RUNNING, TRUE, freq, timeout);
    }

    // This essentially stops the SP without doing a shutdown, i.e it
    // causes the test leg to exit.
    void HaltSP(SPIdentity_t sp, ULONG timeout = (30 * 1000))
    {
        if( timeout < EMCPAL_MINIMUM_TIMEOUT_MSECS) {
            timeout = EMCPAL_MINIMUM_TIMEOUT_MSECS;
        }
        mControl->Halt_SP(sp);
        if (sp == 0) {
            set_SPA_flag(SP_RUNNING, FALSE);
        }
        else {
            set_SPB_flag(SP_RUNNING, FALSE);
        }

        // Wait a little bit.
        if (timeout >= EMCPAL_MINIMUM_TIMEOUT_MSECS) {
            EmcpalThreadSleep(timeout);
        }
    }

    // This sends a shutdown IOCTL to the drivers on the stack.   The Test Leg
    // does not exit, you must issue a Halt after the shutdown to get the Test Leg
    // to exit.
    void ShutDownSP(SPIdentity_t sp)
    {
        BvdSim_Trace(MUT_LOG_HIGH, "ControlRunner: Shutdown SP%c.", sp ? 'B' : 'A');
        if (sp == 0) {
            set_SPA_flag(SP_SHUTDOWN, TRUE);
            wait_for_SPA_status(SP_SHUTDOWN, FALSE);
            set_SPA_flag(SP_RUNNING, FALSE);
        }
        else {
            set_SPB_flag(SP_SHUTDOWN, TRUE);
            wait_for_SPB_status(SP_SHUTDOWN, FALSE);
            set_SPB_flag(SP_RUNNING, FALSE);
        }
        mControl->Shutdown_SP(sp);
    }

    void ShutdownSpaSpb()
    {
        ShutDownSP(SPA);
        ShutDownSP(SPB);
    }

    void HaltBothSPs(ULONG timeout = DefaultSPHaltTimeout)
    {
        BvdSim_Trace(MUT_LOG_HIGH, "ControlRunner: Halt both SPs.");
        mControl->Halt_SP(SPA);
        mControl->Halt_SP(SPB);
        set_SPBoth_flag(SP_RUNNING, FALSE);

        // Wait a little bit.
        if (timeout >= EMCPAL_MINIMUM_TIMEOUT_MSECS) {
            EmcpalThreadSleep(timeout);
        }
    }


    void runTask(SPIdentity_t sp, int taskId, ULONG freq=1000, ULONG timeout=Default3LeggedTimeout)
    {
        if (sp == SPA) {
            runTaskOnSPA(taskId, freq, timeout);
        }
        else {
            runTaskOnSPB(taskId, freq, timeout);
        }
    }

    void runTaskOnSPA(int taskId, ULONG freq=1000, ULONG timeout=Default3LeggedTimeout, bool wait=true)
    {
        BvdSim_Trace(MUT_LOG_HIGH, "ControlRunner: Run Task ID %d on SPA.", taskId);
        set_SPA_flag(TASK_RUNNING, FALSE);
        set_SPA_flag(TASK_COMPLETED, FALSE);
        set_SPA_flag(taskId, TRUE);
        set_SPA_flag(SIGNAL_SP, TRUE);
        if(wait) {
            waitTaskOnSPA(taskId,freq,timeout);
        }
    }

    void runTaskOnSPB(int taskId, ULONG freq=1000, ULONG timeout=Default3LeggedTimeout, bool wait=true)
    {
        BvdSim_Trace(MUT_LOG_HIGH, "ControlRunner: Run Task ID %d on SPB.", taskId);
        set_SPB_flag(TASK_RUNNING, FALSE);
        set_SPB_flag(TASK_COMPLETED, FALSE);
        set_SPB_flag(taskId, TRUE);
        set_SPB_flag(SIGNAL_SP, TRUE);
        if(wait) {
            waitTaskOnSPB(taskId,freq,timeout);
        }
    }

    void waitTaskOnSPA(int taskId, ULONG freq=1000, ULONG timeout=Default3LeggedTimeout) {
        wait_for_SPA_status(TASK_RUNNING, TRUE);
        BvdSim_Trace(MUT_LOG_HIGH, "ControlRunner: Task ID %d running on SPA.", taskId);
        wait_for_SPA_status(TASK_COMPLETED, TRUE, freq, timeout);
        BvdSim_Trace(MUT_LOG_HIGH, "ControlRunner: Task ID %d completed on SPA.", taskId);
    }

    void waitTaskOnSPB(int taskId, ULONG freq=1000, ULONG timeout=Default3LeggedTimeout) {
        wait_for_SPB_status(TASK_RUNNING, TRUE);
        BvdSim_Trace(MUT_LOG_HIGH, "ControlRunner: Task ID %d running on SPB.", taskId);
        wait_for_SPB_status(TASK_COMPLETED, TRUE, freq, timeout);
        BvdSim_Trace(MUT_LOG_HIGH, "ControlRunner: Task ID %d completed on SPB.", taskId);
    }

    void runTaskOnBothSPs(int taskId, ULONG freq=1000, ULONG timeout=Default3LeggedTimeout)
    {
        BvdSim_Trace(MUT_LOG_HIGH, "ControlRunner: Run Task ID %d on both SPs.", taskId);
        set_SPBoth_flag(TASK_RUNNING, FALSE);
        set_SPBoth_flag(TASK_COMPLETED, FALSE);
        set_SPBoth_flag(taskId, TRUE);
        set_SPBoth_flag(SIGNAL_SP, TRUE);
        wait_for_SPBoth_status(TASK_RUNNING, TRUE);
        BvdSim_Trace(MUT_LOG_HIGH, "ControlRunner: Task ID %d running on both SPs.", taskId);
        wait_for_SPBoth_status(TASK_COMPLETED, TRUE, freq, timeout);
        BvdSim_Trace(MUT_LOG_HIGH, "ControlRunner: Task ID %d completed on both SPs.", taskId);
    }


    void SetSpsStatus(SPIdentity_t sp, CACHE_HDW_STATUS status)
    {
        switch (status)
        {
            case CACHE_HDW_OK:
                mSPControl->SetPowerOk(sp);
                break;
    
            case CACHE_HDW_FAULT:
                mSPControl->SetPowerFail(sp);
                break;
    
            case CACHE_HDW_SHUTDOWN_IMMINENT:
                mSPControl->SetBatteryExhausted(sp);
                mSPShutdownManually[sp] = true;
                BvdSim_Trace(MUT_LOG_HIGH, "ControlRunner: CACHE_HDW_SHUTDOWN_IMMINENT SP%c.", sp ? 'B' : 'A');
                break;
        }
        // Wait for the status to change.
        MUT_ASSERT_TRUE (WaitForSpsHardwareStatusChange(sp, status));
    }

    void SetSpsStatusBothSPs(CACHE_HDW_STATUS status)
    {
        SetSpsStatus(SPA, status);
        SetSpsStatus(SPB, status);
    }

    // Verify that the SP device is quiesced and that it requested
    // to be powered off.
    void VerifySPQuiescedAndRequestedPowerDown(SPIdentity_t sp)
    {
        VerifySPRequestedPowerDown(sp);
        VerifySPQuiesced(sp);
    }

    // Verify that the SP device requested to be powered off.
    void VerifySPRequestedPowerDown(SPIdentity_t sp)
    {
        MUT_ASSERT_TRUE (WaitForPowerOff(sp));
    }

    // Verify that both SP devices are quiesced and have requested to
    // be powered off.
    void VerifyBothSPsQuiescedAndRequestedPowerDown()
    {
        VerifySPQuiescedAndRequestedPowerDown(SPA);
        VerifySPQuiescedAndRequestedPowerDown(SPB);
    }

    // Verify that the SP device is powered on.
    bool VerifySPPoweredOn(SPIdentity_t sp)
    {
        return (mSPControl->IsPoweredOn(sp));
    }

    // Verify that both SP devices are powered on.
    bool VerifyBothSPsPoweredOn()
    {
        if (mSPControl->IsPoweredOn(SPA) &&
            mSPControl->IsPoweredOn(SPB))
        {
            return true;
        }
        return false;
    }

    // Verify that both SP devices are quiesced.
    void VerifyBothSPsQuiesced()
    {
        VerifySPQuiesced(SPA);
        VerifySPQuiesced(SPB);
    }

    // Verify that the SP device is quiesced.
    void VerifySPQuiesced(SPIdentity_t sp)
    {
        MUT_ASSERT_TRUE(WaitForSPQuiesced(sp, true));
    }

    // Verify that the SP device is unquiesced.
    void VerifySPUnquiesced(SPIdentity_t sp)
    {
        MUT_ASSERT_TRUE(WaitForSPQuiesced(sp, false));
    }

    // Verify that both SP devices are unquiesced.
    void VerifyBothSPsUnquiesced()
    {
        VerifySPUnquiesced(SPA);
        VerifySPUnquiesced(SPB);
    }

    // Set the persistent memory status.
    void SetPersistentMemoryStatus(SPIdentity_t sp, UINT_32E status)
    {
        PersistentMemoryId_e id;
        if (sp == SPA) {
            id = SPA_PersistentMemory;
        }
        else {
            id = SPB_PersistentMemory;
        }

        mDistrNvRamMem->SetPersistentStatus(id, status);
    }

    // Initialize the random number generator
    void InitializeRandomInteger32()
    {
        csx_rt_ux_rand32_initialize(&mPRNGContext32,true);
    }

    // Returns a pseudo-random number in the range [0,max-1].
    // This uses the CSX implementation of Bob Jenkins's Isaac PRNG, which
    // saves state in a context object local to an object of this class.
    // Therefore, there is no side-effect on global state.
    ULONG GetRandomInteger32(ULONG max) {
        ULONG r = csx_rt_ux_rand32(&mPRNGContext32);

        // Isaac mixes all bits equally, so the results should be evenly spread
        // integers on the interval [0, 0xffffffff].  Scale using a double to
        // take advantage of all bits (rather than taking a modulus).
        double d =  r / (0xffffffff + 1.0);

        return static_cast<ULONG>(d * max);
    }

protected:

    // Wait for the SPS hardware status to change to the specified value.
    // Returns: true when status changed; false if it timed out waiting.
    bool WaitForSpsHardwareStatusChange(SPIdentity_t sp, CACHE_HDW_STATUS status)
    {
        // Wait up to one minute for the SPS status to change.
        int ticks = ((60*1000)/100);
        for (; ticks > 0; ticks--) {
            EmcpalThreadSleep(100);
            if (mSPControl->GetHardwareStatus(sp) == status) {
                // The SP device has seen the state change, but it may not
                // have propagated to the CacheDriver yet. Delay here to allow
                // it to reach the CacheDriver.
                EmcpalThreadSleep(2000);
                return true;
            }
        }
        return false;
    }

    // Wait for the SP to be powered off.
    // Returns: true when power has been removed; false if it timed out waiting.
    bool WaitForPowerOff(SPIdentity_t sp)
    {
        if (!mSPControl->IsPoweredOn(sp)) {
            return true;
        }

        // Wait up to one minute for the SPS to power down.
        int ticks = ((60*1000)/100);
        for (; ticks > 0; ticks--) {
            EmcpalThreadSleep(100);
            if (!mSPControl->IsPoweredOn(sp)) {
                return true;
            }
        }
        return false;
    }

    // Wait for the SP's quiesce state to change to the desired state.
    // Returns: true if the new state is reached; false if it timed out waiting. 
    bool WaitForSPQuiesced(SPIdentity_t sp, bool state)
    {
        // Wait up to one minute for the SP quiesce state to change.
        int ticks = ((60*1000)/100);
        for (; ticks > 0; ticks--) {
            EmcpalThreadSleep(100);
            if (mSPControl->IsBackgroundServicesIoQuiesced(sp) == state) {
                return true;
            }
        }
        return false;
    }

protected:
    BvdSimDistributedTestControlBaseClass *mControl;
    SPControl * mSPControl;
    PersistentNvRamMemory *mDistrNvRamMem;
    csx_rt_ux_rand32_context_t mPRNGContext32;

    // The following data structures are used by components concerned
    // with the PMP state of the SP.   Currently used by MCx
    bool mSPBootedBefore[2];
    bool mSPShutdownManually[2];
    bool mManualPmpOperations;
};

class BvdSim3LeggedSPServiceRunnerBase: public BvdSim3LeggedTestInterface, public BvdSimDistributedBaseClass /*,
                                       public CacheModifyDriverParameters*/  {
public:
#ifdef C4_INTEGRATED
    const static UINT_32 DEFAULT_THREAD_COUNT = 2;
    const static UINT_32 DEFAULT_VOLUME_COUNT = 4;
#else //C4_INTEGRATED
    const static UINT_32 DEFAULT_THREAD_COUNT = 5;
    const static UINT_32 DEFAULT_VOLUME_COUNT = 4;
#endif //C4_INTEGRATED

    BvdSim3LeggedSPServiceRunnerBase(const char *uniqueServiceName, const char *processName,
        BvdSimDistributedPeerSPControlBaseClass *config, UINT_32 sharedMemSize = 0) : mTask(0), mThreadCount(DEFAULT_THREAD_COUNT),
                mVolumeCount(DEFAULT_VOLUME_COUNT),
        mSP(BvdSim_Registry::getSpId() ? 'B' : 'A'), mCreateVolumes(true), mIoIterationCount(2500),
        mControl(config),
        BvdSimDistributedBaseClass(uniqueServiceName, processName,sharedMemSize) {
        mControl->getDeviceFactory();
        RBA_option* rba_option = RBA_option::getCliSingleton();
        if(BvdSim_Registry::getSpId() == 0) {
            SetSP(SPA);
            if(rba_option->get()) {
                KtraceStartTracing(TRC_K_TRAFFIC, "ktraceSPA.ktr");
            }
        }
        else {
            SetSP(SPB);
            if(rba_option->get()) {
                KtraceStartTracing(TRC_K_TRAFFIC, "ktraceSPB.ktr");
            }
        }
    }

    ~BvdSim3LeggedSPServiceRunnerBase() {
        RBA_option* rba_option = RBA_option::getCliSingleton();
        if(BvdSim_Registry::getSpId() == 0) {
            SetSP(SPA);
            if(rba_option->get()) {
                KtraceUninit();
            }
        }
        else {
            SetSP(SPB);
            if(rba_option->get()) {
                KtraceUninit();
            }
        }
    }

    void SetSP(SPIdentity_t sp) 
    {
        mSP = sp ? 'B':'A';
    }

    // Set the number of I/O threads.
    void SetThreadCount(UINT_32 threads) {
        mThreadCount = threads;
    }

    // Set the number of I/O volumes.
    void SetVolumeCount(UINT_32 volumes) {
        mVolumeCount = volumes;
    }

    // Set the number of I/O iterations.
    void SetIterationCount(UINT_32 iter) {
        mIoIterationCount = iter;
    }

    // Disable I/O Generator volume creation before starting I/O.
    void DisableIoGeneratorVolumeCreation() {
        mCreateVolumes = false;
    }

    // Perform state machine setup operations.
    // What happens here would be up to the inheritor
    virtual void SetupStateMachine() {
    };

    // Perform state machine teardown operations.
    // What happens here would be up to the inheritor
    virtual void TeardownStateMachine() {
    };

    // Setup a task to run.
    void SetupTask(const int taskFlag)
    {
        mTask = taskFlag;
        set_SP_flag(TASK_RUNNING, TRUE);
        set_SP_flag(taskFlag, FALSE);
        BvdSim_Trace(MUT_LOG_HIGH, "SP%c ServiceRunner task ID %d started.", mSP, mTask);
    }

    // Tear down a task after it has completed.
    void TeardownTask()
    {
        BvdSim_Trace(MUT_LOG_HIGH, "SP%c ServiceRunner task ID %d completed.", mSP, mTask);
        set_SP_flag(TASK_COMPLETED, TRUE);
    }

    // The service runner state machine
    virtual void run()
    {
        // Signal that this SP has started.
        set_SP_flag(SP_RUNNING, TRUE);                
        BvdSim_Trace(MUT_LOG_HIGH, "SP%c ServiceRunner started.", mSP);

        SetupStateMachine();

        // Initialize test leg
        Initialize();

        // Wait for the contol process to wake up this SP.
        wait_for_SP_status(SIGNAL_SP, TRUE);
        set_SP_flag(SIGNAL_SP, FALSE);

        while (!get_SP_flag(SP_SHUTDOWN))
        {
            if (get_SP_flag(TASK_1) == TRUE) {
                task_1(TASK_1);
            }
            else if (get_SP_flag(TASK_2) == TRUE) {
                task_2(TASK_2);
            }
            else if (get_SP_flag(TASK_3) == TRUE) {
                task_3(TASK_3);
            }
            else if (get_SP_flag(TASK_4) == TRUE) {
                task_4(TASK_4);
            }
            else if (get_SP_flag(TASK_5) == TRUE) {
                task_5(TASK_5);
            }
            else if (get_SP_flag(TASK_6) == TRUE) {
                task_6(TASK_6);
            }
            else if (get_SP_flag(TASK_7) == TRUE) {
                task_7(TASK_7);
            }
            else if (get_SP_flag(TASK_8) == TRUE) {
                task_8(TASK_8);
            }
            else if (get_SP_flag(TASK_9) == TRUE) {
                task_9(TASK_9);
            }
            else if (get_SP_flag(TASK_10) == TRUE) {
                task_10(TASK_10);
            }
            else if (get_SP_flag(TASK_11) == TRUE) {
                task_11(TASK_11);
            }
            else if (get_SP_flag(TASK_12) == TRUE) {
                task_12(TASK_12);
            }
            else if (get_SP_flag(TASK_13) == TRUE) {
                task_13(TASK_13);
            }
            else if (get_SP_flag(TASK_14) == TRUE) {
                task_14(TASK_14);
            }
            else if (get_SP_flag(TASK_15) == TRUE) {
                task_15(TASK_15);
            }
            else if (get_SP_flag(TASK_16) == TRUE) {
                task_16(TASK_16);
            }
            else if (get_SP_flag(TASK_17) == TRUE) {
                task_17(TASK_17);
            }
            else if (get_SP_flag(TASK_18) == TRUE) {
                task_18(TASK_18);
            }
            else if (get_SP_flag(TASK_19) == TRUE) {
                task_19(TASK_19);
            }
            else if (get_SP_flag(TASK_20) == TRUE) {
                task_20(TASK_20);
            }
            else if (get_SP_flag(TASK_21) == TRUE) {
                task_21(TASK_21);
            }
            else if (get_SP_flag(TASK_22) == TRUE) {
                task_22(TASK_22);
            }
            else if (get_SP_flag(TASK_23) == TRUE) {
                task_23(TASK_23);
            }
            else if (get_SP_flag(TASK_24) == TRUE) {
                task_24(TASK_24);
            }
            else if (get_SP_flag(TASK_25) == TRUE) {
                task_25(TASK_25);
            }
            else if (get_SP_flag(TASK_26) == TRUE) {
                task_26(TASK_26);
            }
            else {
                EmcpalThreadSleep(500);
            }
        }

        TeardownStateMachine();
        BvdSim_Trace(MUT_LOG_HIGH, "SP%c ServiceRunner done.", mSP);
        set_SP_flag(SP_SHUTDOWN, FALSE);
    }

protected:
    virtual void task_1(const int taskFlag) { SetupTask(taskFlag); TeardownTask(); };
    virtual void task_2(const int taskFlag) { SetupTask(taskFlag); TeardownTask(); };
    virtual void task_3(const int taskFlag) { SetupTask(taskFlag); TeardownTask(); };
    virtual void task_4(const int taskFlag) { SetupTask(taskFlag); TeardownTask(); };
    virtual void task_5(const int taskFlag) { SetupTask(taskFlag); TeardownTask(); };
    virtual void task_6(const int taskFlag) { SetupTask(taskFlag); TeardownTask(); };
    virtual void task_7(const int taskFlag) { SetupTask(taskFlag); TeardownTask(); };
    virtual void task_8(const int taskFlag) { SetupTask(taskFlag); TeardownTask(); };
    virtual void task_9(const int taskFlag) { SetupTask(taskFlag); TeardownTask(); };
    virtual void task_10(const int taskFlag) { SetupTask(taskFlag); TeardownTask(); };
    virtual void task_11(const int taskFlag) { SetupTask(taskFlag); TeardownTask(); };
    virtual void task_12(const int taskFlag) { SetupTask(taskFlag); TeardownTask(); };
    virtual void task_13(const int taskFlag) { SetupTask(taskFlag); TeardownTask(); };
    virtual void task_14(const int taskFlag) { SetupTask(taskFlag); TeardownTask(); };
    virtual void task_15(const int taskFlag) { SetupTask(taskFlag); TeardownTask(); };
    virtual void task_16(const int taskFlag) { SetupTask(taskFlag); TeardownTask(); };
    virtual void task_17(const int taskFlag) { SetupTask(taskFlag); TeardownTask(); };
    virtual void task_18(const int taskFlag) { SetupTask(taskFlag); TeardownTask(); };
    virtual void task_19(const int taskFlag) { SetupTask(taskFlag); TeardownTask(); };
    virtual void task_20(const int taskFlag) { SetupTask(taskFlag); TeardownTask(); };
    virtual void task_21(const int taskFlag) { SetupTask(taskFlag); TeardownTask(); };
    virtual void task_22(const int taskFlag) { SetupTask(taskFlag); TeardownTask(); };
    virtual void task_23(const int taskFlag) { SetupTask(taskFlag); TeardownTask(); };
    virtual void task_24(const int taskFlag) { SetupTask(taskFlag); TeardownTask(); };
    virtual void task_25(const int taskFlag) { SetupTask(taskFlag); TeardownTask(); };
    virtual void task_26(const int taskFlag) { SetupTask(taskFlag); TeardownTask(); };

    void Initialize()
    {
    }

protected:

    BvdSimDistributedPeerSPControlBaseClass *mControl;
    int mTask;
    char mSP;
    UINT_32 mThreadCount;
    UINT_32 mVolumeCount;
    UINT_32 mIoIterationCount;
    bool mCreateVolumes;
};

#endif //__BvdSim3LeggedTest_h__

