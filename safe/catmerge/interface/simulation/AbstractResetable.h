/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *                               AbstractResetable.h
 ***************************************************************************
 *
 * DESCRIPTION:  Abstract Class definitions that define Testable Systems
 *
 * FUNCTIONS:
 *
 * NOTES:
 *
 * HISTORY:
 *    01/31/2010  Martin Buckley Initial Version
 *
 **************************************************************************/
#ifndef _ABSTRACTRESETABLE_
#define _ABSTRACTRESETABLE_
# include "generic_types.h"

typedef struct ResetInfo_s
{
    char *name;
    void (*reset)();
} ResetInfo_t;

#ifdef __cplusplus
extern "C" {
#endif

 
class AbstractResetable
{
public:
    // For use in SinglyLinkedList.h
    AbstractResetable *mLink;

    AbstractResetable(): mLink(NULL) {}
    virtual ~AbstractResetable() {}

    virtual const char *getName() { return "Unknown"; }
    virtual void    reset() { }
};


#ifdef __cplusplus
};
#endif


#endif
