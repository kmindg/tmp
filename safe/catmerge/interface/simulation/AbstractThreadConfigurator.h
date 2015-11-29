/***************************************************************************
 * Copyright (C) EMC Corporation 2012
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *                               AbstractThreadConfigurator.h
 ***************************************************************************
 *
 * DESCRIPTION: Definition of Interface AbstractThreadConfigurator
 *
 * FUNCTIONS:
 *
 * NOTES:
 *
 * HISTORY:
 *    03/12/2012  Martin Buckley Initial Version
 *
 **************************************************************************/

#ifndef __ABSTRACTTHREADCONFIGURATOR__
#define __ABSTRACTTHREADCONFIGURATOR__
# include "generic_types.h"

class AbstractThreadConfigurator {
public:
    AbstractThreadConfigurator() : mShuttingDown(false) {}
    virtual ~AbstractThreadConfigurator() {}

    virtual UINT_64 getHostThreadAffinity(UINT_32 LunIndex, UINT_32 RGIndex) = 0;
    virtual UINT_64 getBEThreadAffinity(UINT_32 LunIndex, UINT_32 RGIndex) = 0;

    virtual bool ShuttingDown() const { return mShuttingDown; };
    virtual void ShutDown()  { mShuttingDown = true; };
private:
    bool mShuttingDown;
};


#endif // __ABSTRACTTHREADCONFIGURATOR__
