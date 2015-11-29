/***************************************************************************
 * Copyright (C) EMC Corporation 2009 
 * All rights reserved. 
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_cli_src_virtual_drive_cmds.c
 ***************************************************************************
 *
 * @brief
 *  This file contains cli functions for the virtual drive object.
 *
 * @ingroup fbe_cli
 *
 * @revision
 *   09/01/2010:  Prafull Parab - created
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include <stdio.h>
#include "fbe_cli_private.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_api_provision_drive_interface.h"

/*!**************************************************************
 * fbe_cli_cmd_proactive_spare()
 ****************************************************************
 * @brief
 *   This function will start proactive copy for desired disk.
 *  
 *  @param    argc - argument count
 *  @param    argv - argument string
 *
 * @return - none.  
 *
 * @author
 *  4/29/2009 - Created. Prafull Parab
 *
 ****************************************************************/

void fbe_cli_cmd_proactive_spare (int argc , char ** argv)
{
    fbe_u32_t port = 8;
    fbe_u32_t enclosure = 8;
    fbe_u32_t slot = 15;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_object_id_t provision_drive_object_id = FBE_OBJECT_ID_INVALID;
    
    /*
     * Parse the command line.
     */
    while (argc > 0)
    {
        if ((strcmp(*argv, "-help") == 0) ||
            (strcmp(*argv, "-h") == 0))
        {
            /* If they are asking for help, just display help and exit.
             */
            fbe_cli_printf("%s", PROACTIVE_SPARE_DRIVE_USAGE);
            return;
        }
        else if (strcmp(*argv, "-p") == 0)
        {
            /* Get Port argument.
             */
            argc--;
            argv++;
            if(argc == 0)
            {
                fbe_cli_error("-p, expected port, too few arguments \n");
                return;
            }
            port = (fbe_u32_t)strtoul(*argv, 0, 0);
        }
        else if (strcmp(*argv, "-e") == 0)
        {
            /* Get Enclosure argument.
             */
            argc--;
            argv++;
            if(argc == 0)
            {
                fbe_cli_error("-e, expected enclosure, too few arguments \n");
                return;
            }
            enclosure = (fbe_u32_t)strtoul(*argv, 0, 0);
        }
        else if (strcmp(*argv, "-s") == 0)
        {
            /* Get Slot argument.
             */
            argc--;
            argv++;
            if (argc == 0)
            {
                fbe_cli_error("-s, expected slot, too few arguments \n");
                return;
            }
            slot = (fbe_u32_t)strtoul(*argv, 0, 0);
        }
        argc--;
        argv++;

    }   /* end of while */

    status = fbe_api_provision_drive_get_obj_id_by_location(port, enclosure, slot, &provision_drive_object_id);
    
    if((status != FBE_STATUS_OK) || (provision_drive_object_id == FBE_OBJECT_ID_INVALID))
    {
        fbe_cli_error("Invalid provision_drive_object at position: port-%d Encl-%d Slot-%d",port,enclosure,slot);
        return;
    }
    
    /* Here we are triggering proactive sparing by setting end of life state for the given drive*/
    status = fbe_api_provision_drive_set_eol_state(provision_drive_object_id,FBE_PACKET_FLAG_NO_ATTRIB);
    
    if (status != FBE_STATUS_OK)
    {
        fbe_cli_printf("\n Proactive spare swap-in request is not completed status: 0x%x\n",
                        status);
    }
    return;
}
/******************************************
 * end fbe_cli_cmd_proactive_spare()
 ******************************************/

void fbe_cli_auto_hot_spare(int argc , char ** argv)
{
	fbe_status_t status;
	fbe_bool_t test_mode_enabled = FBE_FALSE;

	if (*argv == NULL) {
		fbe_cli_printf("%s", AUTO_HOT_SPARE_USAGE);
		return;
	}

	if ((strcmp(*argv, "-help") == 0) || (strcmp(*argv, "-h") == 0)){
		/* If they are asking for help, just display help and exit.
		 */
		fbe_cli_printf("%s", AUTO_HOT_SPARE_USAGE);
		return;
    }

	if (strcmp(*argv, "-on") == 0){
		status = fbe_api_control_automatic_hot_spare(FBE_TRUE);
		if (status != FBE_STATUS_OK) {
			fbe_cli_error("Failed to turn auto hot spare on\n");
		}
		return;
	}

	if (strcmp(*argv, "-off") == 0){
		status = fbe_api_control_automatic_hot_spare(FBE_FALSE);
		if (status != FBE_STATUS_OK) {
			fbe_cli_error("Failed to turn auto hot spare off\n");
		}
		return;
	}

	if (strcmp(*argv, "-state") == 0){
		status = fbe_api_provision_drive_get_config_hs_flag(&test_mode_enabled);
		if (status != FBE_STATUS_OK) {
			fbe_cli_error("Failed to turn auto hot spare off\n");
			return;
		}

        if (FBE_IS_FALSE(test_mode_enabled)) {
			fbe_cli_printf("Automatic HS is enabled - System will Automatically pick up a suitable hot spare as long as there are available ones\n");
		}else{
			fbe_cli_printf("Automatic HS is DISABLED ! - System will not pick up any hot spare and RGs will stay degraded\n");
		}
		return;
	}

	fbe_cli_printf("%s", AUTO_HOT_SPARE_USAGE);
	return;
		

}
    
/*************************
 * end file fbe_cli_src_logical_drive_cmds.c
 *************************/
