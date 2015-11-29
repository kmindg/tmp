/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *                               AbstractMutex.h
 ***************************************************************************
 *
 * DESCRIPTION:  Definition of interface AbstractMutex 
 *
 * FUNCTIONS:
 *
 * NOTES:
 *
 * HISTORY:
 *    07/27/2011  Martin Buckley Initial Version
 *
 **************************************************************************/
#ifndef __ABSTRACTMUTEX__
#define __ABSTRACTMUTEX__

# include "generic_types.h"

class AbstractMutex {
public:
    virtual ~AbstractMutex() {}
    virtual BOOL lock(UINT_64 millisecondTimeout) = 0;
    virtual void lock() = 0;
    virtual void unlock() = 0;
};

#endif  // __ABSTRACTMUTEX__
