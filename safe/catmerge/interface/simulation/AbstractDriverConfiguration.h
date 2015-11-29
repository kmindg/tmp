/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *                               AbstractDriverConfig.h
 ***************************************************************************
 *
 * DESCRIPTION: Definition of Interface AbstractDriverConfig
 *
 * FUNCTIONS:
 *
 * NOTES:
 *
 * HISTORY:
 *    11/22/2009  Martin Buckley Initial Version
 *
 **************************************************************************/
#ifndef __ABSTRACTDRIVERCONFIG__
#define __ABSTRACTDRIVERCONFIG__

# include "simulation/DeviceConstructor.h"
# include "simulation/IOTaskId.h"
# include "simulation/BvdSim_Registry.h"


class BvdSimMutBaseClass;

class AbstractDriverConfiguration {
public:
    virtual ~AbstractDriverConfiguration() {}
    virtual const char              *getDriverName() = 0;
    virtual PDeviceConstructor       getDeviceFactory() = 0;
    virtual PBvdSim_Registry_Data    getRegistryData() = 0;
    virtual const PCHAR              getDriverRegistryPath() = 0;
    virtual PIOTaskId                getDefaultIOTaskId() = 0;
    virtual void                     setCurrentTest(BvdSimMutBaseClass *test) = 0;
};

typedef AbstractDriverConfiguration *PAbstractDriverConfiguration;

#endif // __ABSTRACTDRIVERCONFIG__
