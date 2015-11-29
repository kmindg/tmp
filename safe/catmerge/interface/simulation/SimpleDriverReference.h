/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *                               SimpleReference.h
 ***************************************************************************
 *
 * DESCRIPTION: Simple Driver Reference class definition
 *
 * FUNCTIONS:
 *
 * NOTES:
 *
 * HISTORY:
 *    11/22/2009  Martin Buckley Initial Version
 *
 **************************************************************************/
#ifndef __SIMPLEDRIVERREFERENCE__
#define __SIMPLEDRIVERREFERENCE__

#include "k10ntddk.h"
#include "EmcPAL_DriverShell.h"
# include "generic_types.h"
# include "simulation/AbstractDriverReference.h"
# include "simulation/AbstractDriverConfiguration.h"

class SimpleDriverReference: public AbstractDriverReference
{
public:
    SimpleDriverReference(PAbstractDriverConfiguration configuration,
                            DriverEntry pDriverEntry,
                            const char **RequiredDevices = NULL,
                            const char **ExportedDevices = NULL,
                            CmmInit_t MemoryInit = NULL);
    SimpleDriverReference(const char *Name,
                            DriverEntry pDriverEntry,
                            const char **RequiredDevices = NULL,
                            const char **ExportedDevices = NULL,
                            CmmInit_t MemoryInit = NULL);
                            
    ~SimpleDriverReference();

    virtual void LoadDriver(PEMCPAL_NATIVE_DRIVER_OBJECT DriverObject, const PCHAR registryPath);
    virtual void UnloadDriver(PEMCPAL_NATIVE_DRIVER_OBJECT DriverObject);

    /*
     * Returns a pointer to the routine used to initialize the DriverObject
     */
    DriverEntry     getDriverEntry();

    /*
     * These methods are required for interface AbstractDriverConfiguration
     */
    virtual const char              *getDriverName() = 0;
    virtual PDeviceConstructor       getDeviceFactory() = 0;
    virtual PBvdSim_Registry_Data    getRegistryData() = 0;
    virtual PIOTaskId                getDefaultIOTaskId() = 0;


private:
    DriverEntry                     mpDriverEntry;
};
 

#endif // __SIMPLEDRIVERREFERENCE__
