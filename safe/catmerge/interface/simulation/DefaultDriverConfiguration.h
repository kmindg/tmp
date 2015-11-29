/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *                               DefaultDriverConfiguration.h
 ***************************************************************************
 *
 * DESCRIPTION: Definition of class DefaultDriverConfiguration
 *
 * FUNCTIONS: This class provides a standard implementation of
 *            interface AbstractDriverconfiguration
 *
 * NOTES:
 *
 * HISTORY:
 *    11/22/2009  Martin Buckley Initial Version
 *
 **************************************************************************/
#ifndef __BVDSIMDEFAULTDRIVERCONFIG__
#define __BVDSIMDEFAULTDRIVERCONFIG__

 
# include "simulation/AbstractDriverConfiguration.h"


class DefaultDriverConfiguration: public AbstractDriverConfiguration {
public:
    DefaultDriverConfiguration(const char *DriverName = "Unknown");
    virtual const char               *getDriverName();
    virtual PDeviceConstructor       getDeviceFactory();
    virtual PBvdSim_Registry_Data    getRegistryData();
    virtual PIOTaskId                getDefaultIOTaskId();

    void                             setCurrentTest(BvdSimMutBaseClass *test);
    BvdSimMutBaseClass               *getCurrentTest();

private:
    const char *mDriverName;
    PIOTaskId   mDefaultIOTask;
    BvdSimMutBaseClass *mTest;
};


#endif //__BVDSIMDEFAULTDRIVERCONFIG__


