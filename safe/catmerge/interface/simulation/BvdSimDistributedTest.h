//***********************************************************************
//
//  Copyright (c) 2011-2014 EMC Corporation.
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

#ifndef __BvdSimDistributeTest_h__
#define __BvdSimDistributeTest_h__

#include "k10ntddk.h"
#include "simulation/DistributedService.h"
#include "simulation/BvdSimMutBaseClass.h"
#include "simulation/BvdSim3LStateMachineTestBase.h"

class BvdSimDistributedTestControlBaseClass: public BvdSim3LControlRemoteSystemTestBaseClass {
public:

    BvdSimDistributedTestControlBaseClass(PAbstractDriverReference config = NULL)
    : BvdSim3LControlRemoteSystemTestBaseClass(config) {}
};


class BvdSimDistributedMasterSPControlBaseClass: public BvdSimStateMachineMasterSPTestBase  {
public:
    SPIdentity_t getMasterSPID() { return SPA; }
    SPIdentity_t getMySPID() { return SPA; }
    char *getPeerSPProgram() { return ""; }

    void TearDown()
    {
        ArrayDirector::MonitorSystem();
        closeDeviceFactory();
        //BvdSimMutSystemTestBaseClass::TearDown();
    }
};

class BvdSimDistributedPeerSPControlBaseClass: public BvdSimStateMachinePeerSPTestBase {
public:
    SPIdentity_t getMasterSPID() { return SPA; }
    SPIdentity_t getMySPID() { return SPB; }
    char *getPeerSPProgram() { return ""; }

    void TearDown()
    {
        ArrayDirector::MonitorSystem();
        closeDeviceFactory();
        //BvdSimMutSystemTestBaseClass::TearDown();
    }
};

class BvdSimDistributedBaseClass: public DistributedService {
public:
    BvdSimDistributedBaseClass(const char *uniqueServiceName, const char *processName, UINT_32 sharedMemSize = 0)
    : DistributedService(uniqueServiceName, processName, sharedMemSize) {}

    // Include dummy run, so that Test Control can create an instance of the service for
    // use in communicating with the SP test processes.
    virtual void run() {}
};


#endif //__BvdSimDistributeTest_h__

