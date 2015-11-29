/***************************************************************************
* Copyright (C) EMC Corporation 2011 
* All rights reserved. 
* Licensed material -- property of EMC Corporation
***************************************************************************/

/*!*************************************************************************
*                 @file fbe_cli_lib_cms.c
****************************************************************************
*
* @brief
*  This file contains cli functions for the persistent memory related features in
*  FBE CLI.
*
* @ingroup fbe_cli
*
*
***************************************************************************/

/*************************
*   INCLUDE FILES
*************************/
#include "fbe_cli_private.h"
#include "fbe_cli_cms.h"
#include "fbe/fbe_api_cms_interface.h"


/**************************
*   LOCAL FUNCTIONS
**************************/
static void fbe_cli_cms_get_info(void);

/*!**********************************************************************
*                      fbe_cli_cms ()                 
*************************************************************************
*
*  @brief
*    fbe_cli_cms - Get persistent memory service information
	
*  @param    argc - argument count
*  @param    argv - argument string
*
*  @return
*            None
************************************************************************/
void fbe_cli_cms(fbe_s32_t argc, fbe_s8_t ** argv)
{
    
    if (argc < 1){
        /*If there are no arguments show usage*/
        fbe_cli_printf("%s", FBE_CLI_CMS_USAGE);
        return;
    }
    
    if ((strncmp(*argv, "-get_info", 10) == 0)){
        fbe_cli_cms_get_info();
        return;
    }
    
	/*If there are no arguments show usage*/
	fbe_cli_error("%s", FBE_CLI_CMS_USAGE);
	return;
    
}


static void fbe_cli_cms_get_info(void)
{
	fbe_status_t						status;
	fbe_cms_get_info_t 	get_info;

	status = fbe_api_cms_get_info(&get_info);
	if (status != FBE_STATUS_OK) {
		fbe_cli_error("%s - failed to get info\n", __FUNCTION__);
		return;
	}

	fbe_cli_printf("\nPersistent Memory Information\n======================\n\n");
	fbe_cli_printf("Some info:0x%llX\n", (unsigned long long)get_info.some_info);

}


/*************************
* end fbe_cli_cms.c
*************************/

