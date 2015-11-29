/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2012
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *                               AbstractSystemConfigurator.h
 ***************************************************************************
 *
 * DESCRIPTION: Definition of Interface AbstractSystemConfigurator
 *
 * FUNCTIONS:
 *
 * NOTES:
 *
 * HISTORY:
 *    11/22/2009  Martin Buckley Initial Version
 *
 **************************************************************************/
#ifndef __ABSTRACTSYSTEMCONFIGURATOR__
#define __ABSTRACTSYSTEMCONFIGURATOR__
# include "simulation/AbstractThreadConfigurator.h"
# include "simulation/Program_Option.h"


class AbstractSystemConfigurator {
public:
    virtual ~AbstractSystemConfigurator() {}

    virtual void configureSystemDrivers() = 0;

    virtual AbstractThreadConfigurator *getThreadConfigurator() = 0;

    /*
     * Provide a general function for any driver that needs to maintain 
     * this expectation.  If a driver does not overload then no
     * action will be taken even if the method is called.
     */
    virtual void ExpectPeerDeath(bool setting) {}

};

typedef AbstractSystemConfigurator *PAbstractSystemConfigurator;

#endif //  __ABSTRACTSYSTEMCONFIGURATOR__
