/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *                               McrSPLogOption.h
 ***************************************************************************
 *
 * DESCRIPTION: Declaration of the sp_log_option class
 *
 * FUNCTIONS:
 *
 * NOTES:
 *
 * HISTORY:
 *    07/02/2012  Wason Wang Initial Version
 *
 **************************************************************************/
     
#ifndef __SP_LOG_OPTION_H_
#define __SP_LOG_OPTION_H_   

#include "simulation/Program_Option.h"
#include "McrTypes.h"

class sp_log_option: public Int_option {
    public:
        sp_log_option(UINT_32 logMode = 0, bool record = false);   
        void set(char* logModestr);
        static sp_log_option *getCliSingleton(); 
};
#endif



