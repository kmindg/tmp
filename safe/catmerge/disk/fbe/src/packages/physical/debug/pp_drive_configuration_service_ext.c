/******************************************************************************
 * Copyright (C) EMC Corporation 2012
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 *****************************************************************************/

/*!****************************************************************************
 * @file    pp_drive_configuration_service_ext.c
 *
 * @brief   This module implements the drive configuration service debugger
 *          extensions
 *
 * @author
 *          11/15/2012 Wayne Garrett -- Created.
 *
 *****************************************************************************/
#include "pp_dbgext.h"
#include "fbe/fbe_topology_interface.h"
#include "fbe_drive_configuration_service_private.h" 
#include "fbe_topology_debug.h"
#include "fbe_drive_configuration_service_debug.h" 



#pragma data_seg ("EXT_HELP$4dcs")
static char usageMsg_dcs[] =
"!dcs\n"
"  Show drive configuration service information.\n\n"
"  Usage:dcs \n\
          -h/elp       - For this help information.\n\
          -p/arameters - Display tunable parameters.\n\
";
#pragma data_seg ()
                                  

 /*!****************************************************************************
 * @fn  dcs
 *
 * @brief
 *      Entry of drive configuration service macro.
 *
 * @author
 *          11/15/2012 Wayne Garrett -- Created.
 *
 *****************************************************************************/
CSX_DBG_EXT_DEFINE_CMD(dcs, "dcs")
{
    fbe_char_t*             str      = NULL;
    fbe_bool_t              display_params = FBE_FALSE;
    fbe_dbgext_ptr          dcs_ptr = 0;
    
    str = strtok((char*)args, " \t");

    if (NULL == str)
    {
        /* display all for default */
        display_params = FBE_TRUE;
    }

    while (str != NULL)
    {
        if((strncmp(str, "-help", 5) == 0 || (strncmp(str, "-h", 2) == 0)))
        {
            csx_dbg_ext_print("%s\n",usageMsg_dcs);
            return;
        }
        if((strncmp(str, "-parameters", 10) == 0 || (strncmp(str, "-p", 2) == 0)))
        {
            display_params = FBE_TRUE;
        }
        else
		{
			csx_dbg_ext_print("%s Invalid input. Please try again.\n\n", __FUNCTION__);            
            csx_dbg_ext_print("%s\n",usageMsg_dcs);            
			return;
		}
        str = strtok (NULL, " \t");
    } //while (str != NULL)


    fbe_debug_trace_func(NULL, "use -help to see more options\n\n"); 

    /* Get the terminator_registry pointer information. */
    FBE_GET_EXPRESSION(pp_ext_physical_package_name, drive_configuration_service, &dcs_ptr);
    fbe_debug_trace_func(NULL, "drive_configuration_service ptr: 0x%I64x\n", dcs_ptr);    

    if (display_params)
    {
        fbe_dcs_parameters_debug_trace(pp_ext_physical_package_name, dcs_ptr, fbe_debug_trace_func, NULL, 2);
    }

    return;
}


