/***************************************************************************
 * Copyright (C) EMC Corporation 2010 
 * All rights reserved. 
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
* @file fbe_cli_lib_dps_stats_cmds.c
 ***************************************************************************
 *
 * @brief
 *  This file contains cli functions for the performance statistics related 
 *  features in FBE CLI.
 *
 * @ingroup fbe_cli
 *
 * @version
 *  03/30/2011 - Created. Swati Fursule
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include <stdlib.h> /*for atoh*/
#include <stdio.h>
#include "fbe_cli_private.h"
#include "fbe_trace.h"
#include "fbe/fbe_class.h"
#include "fbe_cli_dps_stats.h"
#include "fbe/fbe_api_dps_memory_interface.h"

void fbe_cli_cmd_dps_stats_parse_cmds(int argc, char** argv);

/*!*******************************************************************
 *  fbe_cli_cmd_dps_stats
 *********************************************************************
 * @brief Function to implement dps memory statistics commands execution.
 *    fbe_cli_cmd_dps_stats executes a SEP/PP Tester operation.  This command
 *               is for simulation/lab debug only.
 *
 *  @param    argc - argument count
 *  @param    argv - argument string
 *
 * @return - none.  
 *
 * @revision:
 *  03/30/2011 - Created. Swati Fursule
 *
 *********************************************************************/
void fbe_cli_cmd_dps_stats(int argc, char** argv)
{
    fbe_cli_printf("%s", "\n");
    fbe_cli_cmd_dps_stats_parse_cmds(argc, argv);
    return;
}
/******************************************
 * end fbe_cli_cmd_dps_stats()
 ******************************************/

/*!**************************************************************
 *          fbe_cli_cmd_dps_stats_parse_cmds()
 ****************************************************************
 *
 * @brief   Parse dps_stats commands
 *
 *  @param    argc - argument count
 *  @param    argv - argument string
 *
 * @return  None.
 *
 * @revision:
 *  03/30/2011 - Created. Swati Fursule
 *
 ****************************************************************/
void fbe_cli_cmd_dps_stats_parse_cmds(int argc, char** argv)
{
    fbe_status_t       status;
    fbe_bool_t         verbose = FBE_FALSE;
    fbe_api_dps_display_category_t          stats_by = FBE_API_DPS_DISPLAY_DEFAULT;
    fbe_package_id_t package_id = FBE_PACKAGE_ID_SEP_0;

    if (argc == 0)
    {
        status = fbe_api_dps_memory_display_statistics(verbose, stats_by, FBE_PACKAGE_ID_SEP_0);
        if (status == FBE_STATUS_OK)
        {
            fbe_cli_printf("dps: SUCCESS: DPS memory statistics recieved successfully\n ");
        }
        else
        {
            fbe_cli_printf("dps: ERROR: DPS memory statistics could not get with status 0x%x\n ",
                           status);
        }
        fbe_cli_printf("\n");
        return;
    }
    if ((strcmp(*argv, "-help") == 0) ||
        (strcmp(*argv, "-h") == 0) )
    {
        /* If they are asking for help, just display help and exit.
        */
        fbe_cli_printf("%s", DPS_STATS_CMD_USAGE);
        return;
    } 

	if(argc > 0){
		if(strcmp(*argv, "-diff") == 0) {
			status = fbe_api_dps_memory_display_statistics(verbose, FBE_API_DPS_DISPLAY_FAST_POOL_ONLY_DIFF, FBE_PACKAGE_ID_SEP_0);
			return;
		 }
	}
	
	status = fbe_api_dps_memory_display_statistics(verbose, FBE_API_DPS_DISPLAY_FAST_POOL_ONLY, FBE_PACKAGE_ID_SEP_0);

	fbe_cli_printf("\n");
	return;


    if (strcmp(*argv, "-neit") == 0) {
        fbe_cli_printf("dps: displaying NEIT package DPS information\n");
        package_id = FBE_PACKAGE_ID_NEIT;
    }

    while (argc && **argv == '-')
    {
        if (!strcmp(*argv, "-v"))
        {
            verbose = FBE_TRUE;
        }
        if (!strcmp(*argv, "-pool"))
        {
            stats_by = FBE_API_DPS_DISPLAY_BY_POOL;
        }
        else if (!strcmp(*argv, "-priority"))
        {                                
            stats_by = FBE_API_DPS_DISPLAY_BY_PRIORITY;
        }
        else if (!strcmp(*argv, "-both"))
        {                                
            stats_by = FBE_API_DPS_DISPLAY_BY_POOL_PRIORITY;
        }
        else
        {
            stats_by = FBE_API_DPS_DISPLAY_DEFAULT;
        }
        argc--;
        argv++;
    }
    status = fbe_api_dps_memory_display_statistics(verbose, stats_by, package_id);
    if (status == FBE_STATUS_OK)
    {
        fbe_cli_printf("dps: SUCCESS: DPS memory statistics recieved successfully\n ");
    }
    else
    {
        fbe_cli_printf("dps: ERROR: DPS memory statistics could not get with status 0x%x\n ",
                       status);
    }
    fbe_cli_printf("\n");
    return;

}
/******************************************
 * end fbe_cli_cmd_dps_stats_parse_cmds()
 ******************************************/

void fbe_cli_cmd_dps_add_mem(int argc , char ** argv)
{

    fbe_status_t status;
    if ((strcmp(*argv, "-help") == 0) ||
        (strcmp(*argv, "-h") == 0) )
    {
        /* If they are asking for help, just display help and exit.
        */
        fbe_cli_printf("%s", DPS_ADD_MEM_CMD_USAGE);
        return;
    }

    status = fbe_api_dps_memory_add_more_memory(FBE_PACKAGE_ID_NEIT);
    if (status == FBE_STATUS_OK)
    {
        fbe_cli_printf("dps: added memory to NEIT successfully\n ");
    }
    else
    {
        fbe_cli_printf("dps: ERROR: DPS could not add more memory, status 0x%x\n ",
                       status);
    }
}

/*!**************************************************************
 *          fbe_cli_cmd_display_env_limits()
 ****************************************************************
 *
 * @brief   Parse dps_stats commands
 *
 *  @param    argc - argument count
 *  @param    argv - argument string
 *
 * @return  None.
 *
 * @revision:
 *  06/28/2012 - Vera Wang. Created
 *
 ****************************************************************/
void fbe_cli_cmd_display_env_limits(int argc , char ** argv)
{
    fbe_status_t status;
    fbe_environment_limits_t env_limits;

    if (argc == 0) 
    {
        status = fbe_api_memory_get_env_limits(&env_limits);
        if (status != FBE_STATUS_OK)
        {
           fbe_cli_printf(" ERROR: Memory couldn't get environment limits, status 0x%x\n ", status);
        }
        else
        {
            fbe_cli_printf("\nEnvironment Limits: \n");
            fbe_cli_printf("  Max Number of frus supported on this platform: %d \n", env_limits.platform_fru_count);            
            fbe_cli_printf("  Max number of RG supported on this platform: %d \n", env_limits.platform_max_rg);
            fbe_cli_printf("  Max number of user visible LUNs: %d \n", env_limits.platform_max_user_lun);
            fbe_cli_printf("  Max number of LUNs per RG: %d \n", env_limits.platform_max_lun_per_rg); 
            fbe_cli_printf("  Max number of back end buses supported on this platform: %d \n", env_limits.platform_max_be_count);
            fbe_cli_printf("  Max number of enclosures per bus: %d \n", env_limits.platform_max_encl_count_per_bus);
        }
        fbe_cli_printf("\n");
        return;
    }
    else
    {
        /* If they are asking for help, just display help and exit.
        */
        fbe_cli_printf("%s", DISPLAY_ENV_LIMITS_CMD_USAGE);
        fbe_cli_printf("\n");
        return;
    }
}

/*************************
 * end file fbe_cli_lib_dps_stats_cmds.c
 *************************/
