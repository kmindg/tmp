/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *                               Callable.h
 ***************************************************************************
 *
 * DESCRIPTION:  Definition of the Callable Interface, which contains the
 *                  method void * call()
 *
 * FUNCTIONS:
 *
 * NOTES:
 *
 * HISTORY:
 *    12/10/2009  Martin Buckley Initial Version
 *
 **************************************************************************/


class Callable 
{
public:
    virtual char *getName() = 0;
    virtual void *call() = 0;
};
