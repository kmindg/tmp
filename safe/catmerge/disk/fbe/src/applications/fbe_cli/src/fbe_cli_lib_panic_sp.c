/***************************************************************************
 * Copyright (C) EMC Corporation 2010 
 * All rights reserved. 
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_cli_lib_panic_sp.c
 ***************************************************************************
 *
 * @brief
 *  This file contains cli functions to support the panic SP comamnd in fbe cli.
 *
 * @ingroup fbe_cli
 *
 * @revision
 *  
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include <stdio.h>
//#include <ctype.h>
#include "fbe_cli_private.h"
#include "fbe_trace.h"
#include "fbe/fbe_class.h"
#include "fbe_transport_trace.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_common_transport.h"
#include "fbe/fbe_api_base_config_interface.h"
#include "fbe/fbe_api_utils.h"
#include "fbe/fbe_api_panic_sp_interface.h"
#include "fbe_cli_panic_sp.h"

/*!**************************************************************
 * @fbe_cli_cmd_panic_sp()
 ****************************************************************
 * @brief
 *   This function will panic the sp.
 *  
 *  @param    argc - argument count
 *  @param    argv - argument string
 *
 * @return - none.  
 *
 * @author
 *  
 *
 ****************************************************************/

void fbe_cli_cmd_panic_sp(int argc , char ** argv)
{
    fbe_status_t status = FBE_STATUS_INVALID;

    status = fbe_api_panic_sp();
    if (status == FBE_STATUS_GENERIC_FAILURE)
    {
        fbe_cli_error("\n****Panic Fail****. If on Linux platform this is expected. Please wait for the SP to reboot.\n");/*trace if panic fail*/
    }
    return;
}
/******************************************
 * end fbe_cli_cmd_panic_sp()
 ******************************************/

/*************************
 * end file fbe_cli_lib_panic_sp.c
 *************************/
