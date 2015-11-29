/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *                               McrCLIOption.h
 ***************************************************************************
 *
 * DESCRIPTION: Declaration of the McrCLIOption class
 *
 * FUNCTIONS:
 *
 * NOTES:
 *
 * HISTORY:
 *    17/05/2012  Wason Wang Initial Version
 *
 **************************************************************************/
     
#ifndef __MCR_CLI_OPTION_H_
#define __MCR_CLI_OPTION_H_     
#include "simulation/Program_Option.h"
#include "McrTypes.h"

class McrDriveTypeOption: public Int_option {
    public:
        McrDriveTypeOption(UINT_32 drive_type = DRIVE_TYPE_LOCAL_MEMORY, bool record = false);
        static McrDriveTypeOption *getCliSingleton(); 
};
#endif//end __MCR_CLI_OPTION_H_




