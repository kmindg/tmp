/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *                               GenericExceptionHandler.h
 ***************************************************************************
 *
 * DESCRIPTION: Definition of a class that invoke a Runnable and catches
 *              expcetions
 * 
 *
 * FUNCTIONS:
 *
 * NOTES:
 *
 * HISTORY:
 *    02/27/2010  Martin Buckley Initial Version
 *
 **************************************************************************/

#ifndef _MICROSOFTEXCEPTIONHANDLER_
#define _MICROSOFTEXCEPTIONHANDLER_

#include "AbstractExceptionHandler.h"


class GenericExceptionHandler: public AbstractExceptionHandler {
public:
    GenericExceptionHandler();
    GenericExceptionHandler(UINT_32 flags);

    void execute(Runnable *runnable);

    BOOL getExceptionEncountered();
    void setExceptionEncountered(BOOL b);

    UINT_32 getFlags();
    void setFlags(UINT_32 flags);

    void setMessageOnException(char *msg);
    void setMessageOnMissedException(char *msg);

    char *getMessageOnException();
    char *getMessageOnMissedException();
    
private:
    BOOL    mExceptionEncountered;
    UINT_32 mFlags;
    char    *mMessageOnException;
    char    *mMessageOnMissedException;
};

#endif // _MICROSOFTEXCEPTIONHANDLER_
