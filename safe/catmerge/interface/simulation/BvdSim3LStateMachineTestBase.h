//***********************************************************************
//
//  Copyright (c) 2015 EMC Corporation.
//  All rights reserved.
//
//  ALL RIGHTS ARE RESERVED BY EMC CORPORATION.  ACCESS TO THIS
//  SOURCE CODE IS STRICTLY RESTRICTED UNDER CONTRACT. THIS CODE IS TO
//  BE KEPT STRICTLY CONFIDENTIAL.
//
//  UNAUTHORIZED MODIFICATIONS OF THIS FILE WILL VOID YOUR SUPPORT
//  CONTRACT WITH EMC CORPORATION.  IF SUCH MODIFICATIONS ARE FOR THE PURPOSE OF
//  CIRCUMVENTING LICENSING LIMITATIONS, LEGAL ACTION MAY RESULT.
//
//***********************************************************************

#ifndef __BvdSim3LStateMachineTestBase_h__
#define __BvdSim3LStateMachineTestBase_h__


#include "k10ntddk.h"
#include "EmcPAL.h"
#include "EmcPAL_DriverShell.h"
#include "simulation/BvdSimulationData.h"
#include "simulation/BvdSim3LMutBaseClass.h"
#include "simulation/BvdSimMutBaseClass.h"
#include "simulation/BvdSimSP.h"
#include "simulation/BvdSim3LStateTestEventLog.h"


// Milisecs per second
#define MSEC_PER_SEC            1000

// Milisecs per tick
#define MSEC_PER_TICK           100


//++
// File Name:
//  BvdSim3LStateMachineTestBase.h
//
// Contents:
//
//  This file contains the BvdSim State Machine test base class.
//
// Revision History:
//  09-17-2010 -- Created. MEV
//--


// Hardware status enumerator
enum HardwareStatus
{
    Ok,   // Hardware is operating correctly, power okay
    Bad,  // Hardware encountered a powerfail
    SI,   // Shutdown Imminent, Battery Exhaustion
};



// BvdSimStateMachineTestBase - This class instantiates a test base class that implements a
//                             set of test methods for driving the state machine and
//                             similtaed Sp device.
class BvdSimStateMachineTestBase {
public:

    // Constructor
    BvdSimStateMachineTestBase() : mSPControl(new SPControl()),
    mSPDriverReference(get_BvdSimSP_DriverReference()) {};

    ~BvdSimStateMachineTestBase() {
        delete mSPControl;
    }

    void StartUp()
    {
        // start the SP Driver
        mSPDriverReference->Start_Driver();

        GenerateHardwareEvent(Ok);
    }

    void TearDown() {
        mSPDriverReference->Stop_Driver();
    }

    // Get the background I/O quiesce state from the simulated SP device.
    //
    // Returns true if background I/O is quiesced.
    BOOL IsBackgroundServicesIoQuiesced() { return mSPControl->IsBackgroundServicesIoQuiesced(); }

    // Get the battery connection state from the simulated SP device.
    //
    // Returns true if the battery is connected.
    BOOL IsPoweredOn() { return mSPControl->IsPoweredOn(); }


    // Reset hardware state back to inital state
    void ResetHardwareState() 
    {
        GenerateHardwareEvent(Ok);
    }

    // Generate a SP hardware event.
    //
    // @param status    - the hardware event to generate
    void GenerateHardwareEvent(HardwareStatus status)
    { 
        switch (status)
        {   
            case Ok:
                mSPControl->SetPowerOk();
                break;
            case Bad:
                mSPControl->SetPowerFail();
                break;
            case SI:
                mSPControl->SetBatteryExhausted();
                break;
        }
    }

protected:


    // Simulated SP device
    SPControl *mSPControl;
    AbstractDriverReference *mSPDriverReference;

};  // class BvdSimStateMachineTestBase


// BvdSimStateMachineSimpleTestBase - This class instantiates a virtual SP and provides a
//                                   set of test methods for interfacing with the state
//                                   machine in a simple single Sp test environment.
class BvdSimStateMachineSimpleTestBase : public BvdSim3LIntegrationTestBaseClass,
                                        public BvdSimStateMachineTestBase {
public:
    // Constructor
    BvdSimStateMachineSimpleTestBase ()
    : BvdSim3LIntegrationTestBaseClass() {}

    virtual void StartUp()
    {
        BvdSim3LIntegrationTestBaseClass::StartUp();
    };

    virtual void TearDown() {
        BvdSim3LIntegrationTestBaseClass::TearDown();
    }


};  // class BvdStateMachineSimpleTestBase


class BvdSimStateMachineMasterSPTestBase : public BvdSimSystemTestMasterSPBaseClass,
                                          public BvdSimStateMachineTestBase {
public:

    // Constructor
    BvdSimStateMachineMasterSPTestBase() {};

    void StartUp()
    {
        BvdSimSystemTestMasterSPBaseClass::StartUp();
    };

    void TearDown() {
        BvdSimSystemTestMasterSPBaseClass::TearDown();
    }

};  // class BvdSimStateMachineMasterSPTestBase


class BvdSimStateMachinePeerSPTestBase : public BvdSimSystemTestPeerSPBaseClass,
                                        public BvdSimStateMachineTestBase {
public:

    // Constructor
    BvdSimStateMachinePeerSPTestBase() {};

    void StartUp()
    {
        BvdSimSystemTestPeerSPBaseClass ::StartUp();
    };

    void TearDown() {
        BvdSimSystemTestPeerSPBaseClass::TearDown();
    }

};  // class BvdSimStateMachinePeerSPTestBase


#endif // __BvdSim3LStateMachineTestBase_h__
