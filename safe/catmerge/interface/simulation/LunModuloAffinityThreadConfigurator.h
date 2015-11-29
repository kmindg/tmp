/***************************************************************************
 * Copyright (C) EMC Corporation 2012
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *                    LunModuloAffinityThreadConfigurator.h
 ***************************************************************************
 *
 * DESCRIPTION: Definition of Class LunModuloAffinityThreadConfigurator
 *
 * FUNCTIONS:
 *
 * NOTES:
 *
 * HISTORY:
 *    03/12/2012  Martin Buckley Initial Version
 *
 **************************************************************************/

#ifndef __LUNMODULOAFFINITYTHREADCONFIGURATOR__
#define __LUNMODULOAFFINITYTHREADCONFIGURATOR__
# include "generic_types.h"
# include "simulation/AbstractThreadConfigurator.h"

class LunModuloAffinityThreadConfigurator: public AbstractThreadConfigurator {
public:
    LunModuloAffinityThreadConfigurator();
    ~LunModuloAffinityThreadConfigurator();

    UINT_64 getHostThreadAffinity(UINT_32 LunIndex, UINT_32 RGIndex);

    UINT_64 getBEThreadAffinity(UINT_32 LunIndex, UINT_32 RGIndex);

private:
    UINT_32 mNoCores;
    csx_processor_mask_t *mAffinityMasks;
};


#endif // __LUNMODULOAFFINITYTHREADCONFIGURATOR__
