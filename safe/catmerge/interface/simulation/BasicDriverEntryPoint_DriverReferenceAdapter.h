/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *                 BasicDriverEntryPoint_DriverReferenceAdapter.h
 ***************************************************************************
 *
 * DESCRIPTION: An Adapter that provides a BasidDriverEntryPoint with
 *                  an Abstract Driver Reference Interface
 *                  
 *
 * FUNCTIONS:
 *
 * NOTES:
 *
 * HISTORY:
 *    01/07/2010  Martin Buckley Initial Version
 *
 **************************************************************************/
#ifndef __BASICDRIVERENTRYPOINT_DRIVERREFERENCEADAPTER
#define __BASICDRIVERENTRYPOINT_DRIVERREFERENCEADAPTER 

#include "k10ntddk.h"
#include "EmcPAL_DriverShell.h"
# include "BasicLib/BasicDriver.h"
# include "simulation/AbstractDriverReference.h"
# include "simulation/AbstractDriverConfiguration.h"

class BasicDriverEntryPoint_DriverReferenceAdapter: public AbstractDriverReference
{
public:
    BasicDriverEntryPoint_DriverReferenceAdapter(const char *name,
                                                 const char **RequiredDevices = NULL,
                                                 const char **ExportedDevices = NULL);
    BasicDriverEntryPoint_DriverReferenceAdapter(BasicDriverEntryPoint  *DriverEntryPoint,
                                                 const char **RequiredDevices = NULL,
                                                 const char **ExportedDevices = NULL);
    

    ~BasicDriverEntryPoint_DriverReferenceAdapter();

    virtual void InitRegistry(PBvdSim_Registry_Data registry_data = NULL);

    virtual void                     PreLoadDriverSetup();
    virtual void LoadDriver(PEMCPAL_NATIVE_DRIVER_OBJECT DriverObject, const PCHAR registerRegistry);
    virtual void UnloadDriver(PEMCPAL_NATIVE_DRIVER_OBJECT DriverObject);

    /*
     * These methods are required for interface AbstractDriverConfiguration
     */
    virtual const char               *getDriverName();
    virtual PDeviceConstructor       getDeviceFactory() = 0;
    virtual PBvdSim_Registry_Data    getRegistryData() = 0;
    virtual PIOTaskId                getDefaultIOTaskId() = 0;

private:
    BasicDriverEntryPoint            *mBasicDriverEntryPoint;
    PAbstractDriverConfiguration     mConfiguration;
};


#endif
