/***************************************************************************
 * Copyright (C) EMC Corporation 2010-2012
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *                               DVLMutBaseClass.h
 ***************************************************************************
 *
 * DESCRIPTION:  Implementation of the DVL base testing classes
 *
 * FUNCTIONS:
 *
 * NOTES:
 *
 * HISTORY:
 *    04/29/2014   Andy Feld Initial Version
 *
 **************************************************************************/
#ifndef _DVLMUTBASECLASS_
#define _DVLMUTBASECLASS_

# include "simulation/simulation_dynamic_scope.h"
# include "simulation/AbstractDriverReference.h"
# include "simulation/BvdSim_cmm.h"


class DVL_DriverReference: public AbstractDriverReference {

public:


public:
    SYMBOL_SCOPE DVL_DriverReference();

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
    static EMCPAL_STATUS DVLDriverUnload(PEMCPAL_DRIVER pPalDriverObject);
};

SYMBOL_SCOPE DVL_DriverReference *get_Dvl_DriverReference();

#endif  // _DVLMUTBASECLASS_
