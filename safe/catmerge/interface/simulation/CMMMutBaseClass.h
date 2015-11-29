/***************************************************************************
 * Copyright (C) EMC Corporation 2010-2012
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *                               CMMMutBaseClass.h
 ***************************************************************************
 *
 * DESCRIPTION:  Implementation of the CMM base testing classes
 *
 * FUNCTIONS:
 *
 * NOTES:
 *
 * HISTORY:
 *    04/09/2012   Martin Buckley Initial Version
 *
 **************************************************************************/
#ifndef _CMMMUTBASECLASS_
#define _CMMMUTBASECLASS_

# include "simulation/simulation_dynamic_scope.h"
# include "simulation/AbstractDriverReference.h"
# include "simulation/BvdSim_cmm.h"


class CMM_DriverReference: public AbstractDriverReference {

public:
    CMM_CLIENT_ID  cmmCtlDesc;
    CMM_CLIENT_ID  cmmDataDesc;
    CMM_POOL_ID    cmmCtlPoolId;
    CMM_POOL_ID    cmmDataPoolId;

public:
    SYMBOL_SCOPE CMM_DriverReference();

    /*
     *********************************************************************
     * These methods are required for interface AbstractDriverConfiguration
     *********************************************************************
     */
    SYMBOL_SCOPE const char              *getDriverName();

    SYMBOL_SCOPE PDeviceConstructor       getDeviceFactory();

    SYMBOL_SCOPE PBvdSim_Registry_Data    getRegistryData();

    SYMBOL_SCOPE PIOTaskId                getDefaultIOTaskId();

    SYMBOL_SCOPE void                     PreLoadDriverSetup();

    /*
     *********************************************************************
     * These methods are required for interface AbstractDriverReference
     *********************************************************************
     */
    SYMBOL_SCOPE void                     LoadDriver(PEMCPAL_NATIVE_DRIVER_OBJECT DriverObject, const PCHAR registryPath);

    SYMBOL_SCOPE void                     UnloadDriver(PEMCPAL_NATIVE_DRIVER_OBJECT DriverObject);

    SYMBOL_SCOPE void configureSystemDrivers();

private:
    static EMCPAL_STATUS CMMDriverUnload(PEMCPAL_DRIVER pPalDriverObject);
};

SYMBOL_SCOPE CMM_DriverReference *get_CMM_DriverReference();

#endif  // _CMMMUTBASECLASS_
