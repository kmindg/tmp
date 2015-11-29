/***************************************************************************
 * Copyright (C) EMC Corporation 2012
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *                               SpId_option.h
 ***************************************************************************
 *
 * DESCRIPTION: Declaration of the SpId_option class
 *
 * FUNCTIONS:
 *
 * NOTES:
 *
 * HISTORY:
 *    05/12/2011  Martin Buckley Initial Version
 *
 **************************************************************************/

#ifndef __SPID_OPTION__
#define __SPID_OPTION__

# include "simulation/Program_Option.h"

class SpId_option: public Int_option {
public:
    SpId_option(UINT_32 SpIndex = 1, bool record = false);

    void _set(int v);

    static SpId_option *getCliSingleton(UINT_32 SpIndex =1);
};
#endif
