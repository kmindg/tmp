/***************************************************************************
 * Copyright (C) EMC Corporation 2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_cli_lib_get_wwn_cmds.c
 ***************************************************************************
 *
 * @brief
 *  This file contains FBE cli command for get current World Wide Name Seed.
 *
 * @ingroup fbe_cli
 *
 * @revision
 *  25-July-2011:  Harshal Wanjari - created
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include <stdio.h>
#include "fbe_cli_lib_get_wwn_cmds.h"
#include "fbe_cli_lib_resume_prom_cmds.h"
#include "fbe_base_environment_debug.h"

/*************************************************************************
 *                            @fn fbe_cli_cmd_get_wwn ()                                                           *
 *************************************************************************
 *
 *  @brief
 *   fbe_cli_cmd_get_wwn performs the execution of the GETWWN command
 *   Display wwn seed.
 *
 * @param :
 *   argc     (INPUT) NUMBER OF ARGUMENTS
 *   argv     (INPUT) ARGUMENT LIST
 *
 * @return:
 *   None
 *
 * @author
 *   25-July-2011:  Harshal Wanjari - created
 *
 *************************************************************************/
void fbe_cli_cmd_get_wwn(fbe_s32_t argc,char ** argv)
{
    /*****************
    **    BEGIN    **
    *****************/
    if ((argc == 1 )&&((strcmp(*argv, "-help") == 0) ||
        (strcmp(*argv, "-h") == 0)))
    {
        /* If they are asking for help, just display help and exit.
        */
        fbe_cli_printf("\n");
        fbe_cli_printf("%s", GET_WWN_USAGE);
        return;
    }
   
    if (argc >= 1)
    {
        fbe_cli_printf("Invalid arguments .Please check the command \n");
        fbe_cli_print_usage();
        return;
    }

    fbe_cli_get_wwn_seed();

    return;

}
/************************************************************************
 **                       End of fbe_cli_cmd_get_wwn ()                **
 ************************************************************************/

/*!**************************************************************
 * fbe_cli_get_wwn_seed()
 ****************************************************************
 * @brief
 *      Prints current World Wide Name Seed.
 *      Called by FBE cli getwwn command
 *
 *  @param :
 *      Nothing.
 *
 *  @return:
 *      Nothing.
 *
 * @author
 *   25-July-2011:  Harshal Wanjari - created
 *
 ****************************************************************/
static void fbe_cli_get_wwn_seed(void)
{
    fbe_status_t                                status = FBE_STATUS_OK;
    fbe_esp_base_env_get_resume_prom_info_cmd_t GetResumePromInfo = {0};
    fbe_esp_board_mgmt_board_info_t   boardInfo = {0};

    status = fbe_api_esp_board_mgmt_getBoardInfo(&boardInfo);

    if (status != FBE_STATUS_OK)
    {
        fbe_cli_error ( "%s:Failed to get BoardInfo, status %s.\n", 
            __FUNCTION__, get_trace_status(status));
        return;
    }

    GetResumePromInfo.deviceType = FBE_DEVICE_TYPE_ENCLOSURE;

    if(boardInfo.platform_info.enclosureType == XPE_ENCLOSURE_TYPE)
    {
        GetResumePromInfo.deviceLocation.bus = FBE_XPE_PSEUDO_BUS_NUM;
        GetResumePromInfo.deviceLocation.enclosure = FBE_XPE_PSEUDO_ENCL_NUM;
    }
    else
    {
        GetResumePromInfo.deviceLocation.bus = 0;
        GetResumePromInfo.deviceLocation.enclosure = 0;
    }

    /*get resume prom information*/
    status = fbe_api_esp_common_get_resume_prom_info(&GetResumePromInfo);
    if (status != FBE_STATUS_OK)
    {
        fbe_cli_printf("Failed to get midplane resume prom info, bus %d encl %d status %s\n",
            GetResumePromInfo.deviceLocation.bus, 
            GetResumePromInfo.deviceLocation.enclosure, 
            get_trace_status(status));
        return;
    }

    if(GetResumePromInfo.op_status == FBE_RESUME_PROM_STATUS_READ_SUCCESS)
    {
        fbe_cli_printf("Resume PROM WWN Seed: 0x%X.\n",GetResumePromInfo.resume_prom_data.wwn_seed);
    }
    else
    {
        fbe_cli_printf("Resume PROM Read Status Error %s.\n", 
                       fbe_base_env_decode_resume_prom_status(GetResumePromInfo.op_status));
    }

    return;
}
/******************************************************
 * end fbe_cli_get_wwn_seed() 
 ******************************************************/
 
/******************************************************
 * end fbe_cli_lib_get_wwn_cmds() 
 ******************************************************/
