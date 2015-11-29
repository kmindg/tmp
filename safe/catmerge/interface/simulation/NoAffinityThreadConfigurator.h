/***************************************************************************
 * Copyright (C) EMC Corporation 2012
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *                               NoAffinityThreadConfigurator.h
 ***************************************************************************
 *
 * DESCRIPTION: Implementation of Class NoAffinityThreadConfigurator
 *
 * FUNCTIONS:
 *
 * NOTES:
 *
 * HISTORY:
 *    03/12/2012  Martin Buckley Initial Version
 *
 **************************************************************************/

#ifndef __NOAFFINITYTHREADCONFIGURATOR__
#define __NOAFFINITYTHREADCONFIGURATOR__
# include "generic_types.h"
# include "simulation/AbstractThreadConfigurator.h"

class NoAffinityThreadConfigurator: public AbstractThreadConfigurator {
public:
    NoAffinityThreadConfigurator();
    ~NoAffinityThreadConfigurator();

    virtual UINT_64 getHostThreadAffinity(UINT_32 LunIndex, UINT_32 RGIndex);

    virtual UINT_64 getBEThreadAffinity(UINT_32 LunIndex, UINT_32 RGIndex);

private:
    UINT_64 mMask;
};


#endif // __NOAFFINITYTHREADCONFIGURATOR__

