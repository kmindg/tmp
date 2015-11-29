/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved. Licensed material -- property of EMC
 * Corporation
 ***************************************************************************/
/*!**************************************************************************
 * @file fbe_cli_lib_board_cmds.c
 ***************************************************************************
 *
 * @brief
 *  This file contain fbe_cli function used to board info.
 *
 * @version
 *  11-Aug-2010: Created  Vaibhav Gaonkar
 *
 ***************************************************************************/
/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_board.h"
#include "fbe/fbe_api_board_interface.h"
#include "fbe_cli.h"
#include "generic_utils_lib.h"
#include "fbe_cli_lib_board_cmd.h"
#include "fbe_api_raid_group_interface.h"
#include "fbe/fbe_api_common_transport.h"
#include "fbe/fbe_api_esp_board_mgmt_interface.h"

/****************************** 
 *    LOCAL FUNCTIONS
 ******************************/

 static fbe_semaphore_t sem;
 static fbe_bool_t sem_created = FBE_FALSE;

/*!***************************************************************
 * fbe_cli_display_board_info(int argc, char** argv)
 ****************************************************************
 * @brief
 *  This function is used display board info
 *
 * @param
 *
 * @return
 *
 * @author
 *  11-Aug-2010: Created  Vaibhav Gaonkar
 *
 ****************************************************************/
void fbe_cli_display_board_info(int argc, char** argv)
{
    fbe_status_t status;
    fbe_object_id_t board_object_id;
    fbe_base_board_get_base_board_info_t base_board_info;

    /* Get the board object id*/
    status = fbe_api_get_board_object_id(&board_object_id);
    if(status != FBE_STATUS_OK)
    {
        fbe_cli_error("Error in retrieving board object ID, Status %d", status);
        return;
    }

	if(argc > 0){
        if ((strcmp(*argv, "-help") == 0) || (strcmp(*argv, "-h") == 0)) {
            /* If they are asking for help, just display help and exit.
            */
            fbe_cli_printf("%s", BOARDPRVDATA_USAGE);
            return;
        }
        if (strcmp(*argv, "-set_async_io") == 0) {
			fbe_api_board_set_async_io(board_object_id, FBE_TRUE);
            return;
        }
        if (strcmp(*argv, "-set_sync_io") == 0) {
			fbe_api_board_set_async_io(board_object_id, FBE_FALSE);
            return;
        }
        if (strcmp(*argv, "-enable_dmrb_zeroing") == 0) {
			fbe_api_board_set_dmrb_zeroing(board_object_id, FBE_TRUE);
            return;
        }
        if (strcmp(*argv, "-disable_dmrb_zeroing") == 0) {
			fbe_api_board_set_dmrb_zeroing(board_object_id, FBE_FALSE);
            return;
        }

        fbe_cli_printf("%s", BOARDPRVDATA_USAGE);
        return;
	}

    /* Get the base board info */
    status = fbe_api_board_get_base_board_info(board_object_id, &base_board_info);
    if(status != FBE_STATUS_OK)
    {
        fbe_cli_error("Error in retrieving base board info, Status %d", status);
        return;
    }

    fbe_cli_printf("Number of IO ports: %d\n", base_board_info.number_of_io_ports);
    fbe_cli_printf("Edal Block Handle: 0x%x\n", (unsigned int)base_board_info.edal_block_handle);
    fbe_cli_printf("SPID: %s\n", decodeSpId(base_board_info.spid));
    fbe_cli_printf("Local IO Module Count: %d\n", base_board_info.localIoModuleCount);
    /* Print the Platform Info */
    fbe_cli_printf("Platform Info:\n");
    fbe_cli_printf("\tFamily Type: %s \n", decodeFamilyType(base_board_info.platformInfo.familyType));
    fbe_cli_printf("\tPlatform Type: %s\n", decodeSpidHwType(base_board_info.platformInfo.platformType));
    fbe_cli_printf("\tHardwareName: %s\n", base_board_info.platformInfo.hardwareName);
    fbe_cli_printf("\tUnique Type: %s\n", decodeFamilyFruID(base_board_info.platformInfo.uniqueType));
    fbe_cli_printf("\tCPUMoudle: %s\n", decodeCPUModule(base_board_info.platformInfo.cpuModule));
    fbe_cli_printf("\tEnclosure Type: %s\n", decodeEnclosureType(base_board_info.platformInfo.enclosureType));
    fbe_cli_printf("\tMidplane Type: %s\n", decodeMidplaneType(base_board_info.platformInfo.midplaneType));

    return;
}
/*************************************************
*   end of fbe_cli_display_board_info()
*************************************************/

/*!***************************************************************
 * fbe_cli_reboot_sp(int argc, char** argv)
 ****************************************************************
 * @brief
 *  This function is used to reboot thisSP or the PeerSP.
 *
 *  @param    argc - argument count
 *  @param    argv - argument string
 *
 * @author
 *  12/1/2010: Created  Musaddique Ahmed
 *
 ****************************************************************/
void fbe_cli_reboot_sp(int argc, char** argv)
{
    fbe_status_t status;
    fbe_object_id_t board_object_id;
    SP_ID       tmp_sp_id = SP_INVALID;
    fbe_bool_t  tmp_holdInPost = FALSE;
    fbe_bool_t  tmp_holdInReset = FALSE;
    fbe_bool_t  tmp_rebootBlade = TRUE; // Default is to simply reboot
    fbe_bool_t  tmp_ShutdownSP = FALSE;
    fbe_u8_t    hold_options = REBOOT_SP_OPTIONS_NONE; // Default is to simply reboot
    
    while (argc > 0)
    {
        if ((strcmp(*argv, "-help") == 0) ||
            (strcmp(*argv, "-h") == 0))
        {
            /* If they are asking for help, just display help and exit.
            */
            fbe_cli_printf("%s", REBOOTSP_USAGE);
            return;
        }
        else if ((strcmp(*argv, "-sp") == 0))
        {
            /* Get the SP Type from command line */
            argc--;
            argv++;
            /* argc should be greater than 0 here */
            if (argc <= 0)
            {
                fbe_cli_error("SP ID is expected\n");
                return;
            }

            if(strcmp(*argv, "a") == 0)
            {
                tmp_sp_id = SP_A;
            }
            else if(strcmp(*argv, "b") == 0)
            {
                tmp_sp_id = SP_B;
            }
            else
            {
                fbe_cli_error("Invalid SP ID.\n");
                return;
            }

            argc--;
            argv++;
        }
        else if ((strcmp(*argv, "-hold") == 0))
        {
            argc--;
            argv++;

            /* argc should be greater than 0 here */
            if (argc <= 0)
            {
                fbe_cli_error("hold argument is expected\n");
                return;
            }

            hold_options = atoi(*argv);

            switch(hold_options)
            {
                case REBOOT_SP_OPTIONS_NONE:
                    tmp_holdInPost = FALSE;
                    tmp_holdInReset = FALSE;
                    tmp_rebootBlade = TRUE;
                break;
                case REBOOT_SP_OPTIONS_HOLD_IN_POST:
                    tmp_holdInPost = TRUE;
                    tmp_holdInReset = FALSE;
                    tmp_rebootBlade = TRUE;
                break;
                case REBOOT_SP_OPTIONS_HOLD_IN_RESET:
                    tmp_holdInPost = FALSE;
                    tmp_holdInReset = TRUE;
                    tmp_rebootBlade = TRUE;
                break;
                case REBOOT_SP_OPTIONS_HOLD_IN_POWER_OFF:
                    tmp_ShutdownSP = TRUE;
                    tmp_rebootBlade = FALSE;
                break;

                default:
                    fbe_cli_error("Invalid option.\n");
                    fbe_cli_printf("%s", REBOOTSP_USAGE);
                    return;
                break;
            }

            argc--;
            argv++;
        }
        else
        {
            /* The command line parameters should be properly entered */
            fbe_cli_error("Invalid argument.\n");
            fbe_cli_printf("%s", REBOOTSP_USAGE);
            return;
        }
    }


    if(tmp_sp_id == SP_INVALID)
    {
        fbe_cli_error("SP ID is not specified.\n");
        fbe_cli_printf("%s", REBOOTSP_USAGE);
        return;
    }

    /* Get the board object id*/
    status = fbe_api_get_board_object_id(&board_object_id);
    if(status != FBE_STATUS_OK)
    {
        fbe_cli_error("Error in retrieving board object ID, Status %d\n", status);
        return;
    }

    if (sem_created == FBE_FALSE)
    {
        fbe_semaphore_init(&sem, 0, 1);
        sem_created = FBE_TRUE;
    }

    if(!tmp_ShutdownSP)
    {
        fbe_board_mgmt_set_PostAndOrReset_t command;
        
        command.sp_id = tmp_sp_id;
        command.holdInPost = tmp_holdInPost;
        command.holdInReset = tmp_holdInReset;
        command.rebootBlade = tmp_rebootBlade;
        command.retryStatusCount = POST_AND_OR_RESET_STATUS_RETRY_COUNT;
        command.rebootLocalSp = FBE_FALSE;
        command.flushBeforeReboot = FBE_FALSE;
        
        fbe_cli_printf("Rebooting %s\n",(tmp_sp_id == SP_A)?"SPA":"SPB");

        if(tmp_holdInPost)
        {
            fbe_cli_printf("Reboot with Hold in Post\n");
        }
        else if(tmp_holdInReset)
        {
            fbe_cli_printf("Reboot with Hold in Reset\n");
        }
        
        /* Set/Clear Hold in Post or Reset. */
        status = fbe_api_esp_board_mgmt_reboot_sp(&command);

        if(status != FBE_STATUS_OK)
        {
            fbe_cli_error("Error in Setting Post&|Reset command write failed with status %d\n", status);
            return;
        }
        
    }
    return;
}
