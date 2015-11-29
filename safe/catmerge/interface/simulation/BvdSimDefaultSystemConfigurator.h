/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *                               BvdSimDefaultSystemConfigurator.h
 ***************************************************************************
 *
 * DESCRIPTION: Definition of class BvdSimDefaultSystemConfigurator
 *
 * FUNCTIONS:
 *
 * NOTES:
 *
 * HISTORY:
 *    11/22/2009  Martin Buckley Initial Version
 *
 **************************************************************************/
#ifndef __BVDSIMDEFAULTSYSTEMCONFIGURATOR__
#define __BVDSIMDEFAULTSYSTEMCONFIGURATOR__

# include "simulation/AbstractSystemConfigurator.h"
class BvdSimMutSystemTestBaseClass;

class BvdSimDefaultSystemConfigurator: public AbstractSystemConfigurator {
public:
    BvdSimDefaultSystemConfigurator(BvdSimMutSystemTestBaseClass *system);

    BvdSimMutSystemTestBaseClass *getSystem();

    virtual void configureSystemDrivers() ;

private:
    BvdSimMutSystemTestBaseClass    *mSystem;
};



#endif // __BVDSIMDEFAULTSYSTEMCONFIGURATOR__
