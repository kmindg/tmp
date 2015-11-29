/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *                               AbstractExceptionHandler.h
 ***************************************************************************
 *
 * DESCRIPTION: Abstract class that invokes a Runable and catches
 *              expcetions thrown
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

#ifndef _ABSTRACTEXCEPTIONHANDLER_
#define _ABSTRACTEXCEPTIONHANDLER_

# include "generic_types.h"
# include "Runnable.h"

enum ExceptionHandlingFlags_e {
    ReportException             = 1,
    ReportMissedException       = 2,
    ContinueExceptionProcessing = 4
};

class AbstractExceptionHandler {
public:
    virtual ~AbstractExceptionHandler() {}
    virtual void execute(Runnable *runnable) = 0;

    virtual BOOL getExceptionEncountered() = 0;
    virtual void setExceptionEncountered(BOOL b) = 0;

    virtual UINT_32 getFlags() = 0;
    virtual void setFlags(UINT_32 flags) = 0;

    virtual void setMessageOnException(char *msg) = 0;
    virtual void setMessageOnMissedException(char *msg) = 0;

    virtual char *getMessageOnException() = 0;
    virtual char *getMessageOnMissedException() = 0;
};

#endif // _ABSTRACTEXCEPTIONHANDLER_
