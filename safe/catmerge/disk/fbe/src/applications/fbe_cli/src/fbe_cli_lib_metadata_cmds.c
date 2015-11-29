
/***************************************************************************
* Copyright (C) EMC Corporation 2012 
* All rights reserved. 
* Licensed material -- property of EMC Corporation
***************************************************************************/

/*!*************************************************************************
*                 @file fbe_cli_lib_metadata_cmds.c
****************************************************************************
*
* @brief
*  This file contains cli functions for the metadata related features in
*  FBE CLI.
*
* @ingroup fbe_cli
*
* @date
*  05/14/2012 - Created. Jingcheng Zhang
*
***************************************************************************/

/*************************
*   INCLUDE FILES
*************************/
#include "fbe_cli_private.h"
#include "fbe_cli_metadata.h"
#include "fbe/fbe_api_metadata_interface.h"
#include "fbe_system_limits.h"
#include "fbe/fbe_api_base_config_interface.h"


/**************************
*   LOCAL FUNCTIONS
**************************/

static void fbe_cli_print_nonpaged_metadata_get_version(fbe_object_id_t object_id);


/*!**********************************************************************
*                      fbe_cli_metadata ()                 
*************************************************************************
*
*  @brief
*    fbe_cli_metadata - Get metadata information 
	
*  @param    argc - argument count
*  @param    argv - argument string
*
*  @return
*            None
************************************************************************/
void fbe_cli_metadata(fbe_s32_t argc, fbe_s8_t ** argv)
{
    fbe_object_id_t   object_id;
    fbe_u32_t max_objects;


    fbe_system_limits_get_max_objects (FBE_PACKAGE_ID_SEP_0, &max_objects);

    if (argc < 2)
    {
        /*If there are no arguments show usage*/
        fbe_cli_printf("%s", FBE_CLI_METADATA_USAGE);
        return;
    }
    
    if ((strncmp(*argv, "-getNonPagedVersion", 19) == 0))
    {
        argv ++;
        if (*argv != NULL)
        {
            object_id = fbe_atoh(*argv);
            fbe_cli_print_nonpaged_metadata_get_version(object_id);
        } else
            fbe_cli_error("%s", FBE_CLI_METADATA_USAGE);

        return;
    }
    else
    {
        /*If there are no arguments show usage*/
        fbe_cli_error("%s", FBE_CLI_METADATA_USAGE);
        return;
    }
}

/*!**************************************************************
 * fbe_cli_print_nonpaged_metadata_get_version()
 ****************************************************************
 * @brief
 *  This function prints out the version info of an object's non-paged metadata.
 *
 * @param 
 *        object_id
 * 
 * @return
 *        None
 ***************************************************************/
static void fbe_cli_print_nonpaged_metadata_get_version(fbe_object_id_t object_id)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_nonpaged_metadata_version_info_t ver_info;

    fbe_zero_memory(&ver_info, sizeof (fbe_api_nonpaged_metadata_version_info_t));
    status = fbe_api_metadata_nonpaged_get_version_info(object_id, &ver_info);

    if ((status == FBE_STATUS_OK))
    {
        if (ver_info.metadata_initialized) {
            fbe_cli_printf("\n Version info for nonpaged metadata of object %x is : \n", object_id);    
            fbe_cli_printf("\n       on-disk nonpaged metadata size: %d\n", ver_info.ondisk_ver_header.size);          
            fbe_cli_printf("\n       nonpaged metadata data structure size: %d\n", ver_info.cur_structure_size);
            fbe_cli_printf("\n       committed nonpaged metadata size : %d\n", ver_info.commit_nonpaged_size);    
        } else {
            fbe_cli_printf("\n The nonpaged metadata of object %x is not initialized \n", object_id);    
        }
    }
    else
    {
        fbe_cli_printf("\n Failed to get nonpaged metadata version info ... Error: %d\n", status);            
    }

    return;
}

void fbe_cli_cmd_set_default_nonpaged_metadata(fbe_s32_t argc,char ** argv)
{
    /*****************
    **    BEGIN    **
    *****************/
    fbe_u32_t object_id;
	fbe_char_t * tmp_ptr;

	
    if (argc < 1)
    {
        /*If there are no arguments show usage*/
        fbe_cli_printf("%s", SET_NP_USAGE);
        return;
    }
    if ((argc == 1 )&&((strcmp(*argv, "-help") == 0) ||
        (strcmp(*argv, "-h") == 0)))
    {
        /* If they are asking for help, just display help and exit.
        */
        fbe_cli_printf("\n");
        fbe_cli_printf("%s", SET_NP_USAGE);
        return;
    }
   
    if (argc > 1)
    {
        fbe_cli_printf("Invalid arguments .Please check the command \n");
        fbe_cli_print_usage();
        return;
    }
	tmp_ptr = *argv;
	object_id = atoi(tmp_ptr);
    fbe_cli_set_default_nonpaged_metadata(object_id);

    return;

}

static void fbe_cli_set_default_nonpaged_metadata(fbe_u32_t object_id)
{
    fbe_status_t                                status = FBE_STATUS_OK;

    status = fbe_api_base_config_metadata_set_default_nonpaged_metadata((fbe_object_id_t)object_id);

    if (status != FBE_STATUS_OK)
    {
        fbe_cli_error ( "%s:Failed to set object nonpaged metadata, status %d.\n", 
            __FUNCTION__, status);
        return;
    }
	
	fbe_cli_printf("set object NP successfully.\n");


    return;
}


/*************************
* end file fbe_cli_lib_metadata_cmds.c
*************************/
