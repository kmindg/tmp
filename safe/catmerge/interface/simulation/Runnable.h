/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *                               Runnable.h
 ***************************************************************************
 *
 * DESCRIPTION:  Definition of Interface Runnable, which contains the
 *                  single method void  run()
 *
 * FUNCTIONS:
 *
 * NOTES:
 *
 * HISTORY:
 *    12/10/2009  Martin Buckley Initial Version
 *
 **************************************************************************/
#ifndef _RUNNABLE_
#define _RUNNABLE_

class Runnable 
{
public:
    virtual ~Runnable() {};
    virtual const char *getName() = 0;
    virtual void run() = 0;
};

#endif //_RUNNABLE_
