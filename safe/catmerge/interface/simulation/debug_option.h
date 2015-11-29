/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2013
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *                               debug_option.h
 ***************************************************************************
 *
 * DESCRIPTION: Declaration of the debug_option class
 *
 * FUNCTIONS:
 *
 * NOTES:
 *
 * HISTORY:
 *    15/05/2012  Wason Wang Initial Version
 *
 **************************************************************************/
     
#ifndef __DEBUG_OPTION_H_
#define __DEBUG_OPTION_H_
#include "csx_ext.h"     
#include "simulation/Program_Option.h"

#if defined(SHMEM_DLL_EXPORT)
#define SHMEMDLLPUBLIC CSX_MOD_EXPORT 
#else
#define SHMEMDLLPUBLIC CSX_MOD_IMPORT 
#endif

//option 1: choose debugger
class debugger_option: public String_option {
    public:
        SHMEMDLLPUBLIC debugger_option(char* debugger, bool record = false);   
		SHMEMDLLPUBLIC void set(char* debugger);
        SHMEMDLLPUBLIC static debugger_option *getCliSingleton(); 
};

//option 2: choose sp to debug
//0 SPA/  1 SPB/  2 both/ 3 none
class sp_todebug_option: public Int_option {
    public:
        SHMEMDLLPUBLIC sp_todebug_option();
        SHMEMDLLPUBLIC ~sp_todebug_option();
		SHMEMDLLPUBLIC void set(char* spchoice);
		SHMEMDLLPUBLIC bool isSet(int spid);
        SHMEMDLLPUBLIC static sp_todebug_option *getCliSingleton(); 
};

#endif



