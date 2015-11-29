/***************************************************************************
* Copyright (C) EMC Corporation 2015
* All rights reserved. 
* Licensed material -- property of EMC Corporation
***************************************************************************/

/*!*************************************************************************
*                 @file fbe_cli_lib_emeh_cmds.c
****************************************************************************
*
* @brief
*  This file contains cli functions for the Extended Media Error Handling
*  (EMEH) in FBE CLI.
*
* @ingroup fbe_cli
*
* @date
*   03/11/2015  Ron Proulx - Created.
*
***************************************************************************/

/*************************
*   INCLUDE FILES
*************************/
#include "fbe_cli_private.h"
#include "fbe_cli_emeh_cmds.h"
#include "fbe/fbe_api_raid_group_interface.h"
#include "fbe/fbe_raid_group.h"

/**************************
*   LOCAL FUNCTIONS
**************************/
static void fbe_cli_lib_get_class_emeh_values(void);
static void fbe_cli_lib_get_rg_emeh_values(fbe_object_id_t rg_object_id);
static void fbe_cli_lib_set_class_emeh_values(fbe_u32_t set_control, fbe_bool_t b_set_threshold, fbe_u32_t new_increase_threshold, fbe_bool_t b_persist);
static void fbe_cli_lib_set_rg_emeh_values(fbe_object_id_t rg_object_id, fbe_u32_t set_control, fbe_bool_t b_set_threshold, fbe_u32_t new_increase_threshold);

/*!**********************************************************************
*                      fbe_cli_emeh()        
*************************************************************************
*
*  @brief
*            fbe_cli_emeh - Extended Media Error Handling (EMEH) CLI
	
*  @param    argc - argument count
*  @param    argv - argument string
*
*  @return
*            None
*  @author 
*   03/11/2015  Ron Proulx  - Created 
************************************************************************/
void fbe_cli_emeh(fbe_s32_t argc, fbe_s8_t ** argv)
{
    fbe_u32_t       set_control = 0;
    fbe_u32_t       set_mode = 0;
    fbe_bool_t      b_persist = FBE_FALSE;
    fbe_bool_t      b_set_threshold = FBE_FALSE;
    fbe_u32_t       new_increase_threshold = 0;
    fbe_object_id_t rg_object_id = FBE_OBJECT_ID_INVALID;

    if (argc < 1) {
        fbe_cli_printf("%s", FBE_CLI_EMEH_USAGE);
        return;
    }

    if ((strcmp(*argv, "-help") == 0) || (strcmp(*argv, "-h") == 0)) {
        /* If they are asking for help, just display help and exit.*/
        fbe_cli_printf("%s", FBE_CLI_EMEH_USAGE);
        return;
    }

    /* Process the command line. */
    if (strncmp(*argv, "-get", 4) == 0) {
        /* Get the parameters from the command line*/
        argc--;
        argv++;

        /* Must specify either `-class' or `-rg_obj'
         */
        if (argc > 0) {
            if (strncmp(*argv, "-rg_obj", 7) == 0) {
                argc--;
                argv++;
                if (argc < 1) {
                    fbe_cli_error("emeh: -get -rg_obj requires a value \n");
                    return;
                } else if (argc > 1) {
                    argc--;
                    argv++;
                    fbe_cli_error("emeh: -get Invalid 3rd param: %s\n", *argv);
                    return;
                }
                rg_object_id = fbe_atoh(*argv);

                /* Call the method to get the emeh values for the raid group.
                 */
                fbe_cli_lib_get_rg_emeh_values(rg_object_id);
            } else if (strncmp(*argv, "-class", 6) == 0) {
                argc--;
                argv++;
                if (argc > 0) {
                    fbe_cli_error("emeh: -get -class doesn't take a parameter\n");
                    return;
                }

                /* Call the method to get the EMEH parameters for the raid group
                 * class (i.e. this SP).
                 */
                fbe_cli_lib_get_class_emeh_values();
            } else {
                fbe_cli_error("emeh: -get first parameter must be either `-class' or `-rg_obj'\n");
                return;
            }
        } else {
            fbe_cli_error("emeh: -get first parameter must be either `-class' or `-rg_obj'\n");
            return;
        }
    } else if (strncmp(*argv, "-set", 4) == 0) {
        /* Get the flags from the command line*/
        argc--;
        argv++;

        /* Must specify either `-class' or `-rg_obj'
         */
        if (argc > 0) {
            if (strncmp(*argv, "-rg_obj", 7) == 0) {
                argc--;
                argv++;
                if (argc < 1) {
                    fbe_cli_error("emeh: -set -rg_obj requires a value \n");
                    return;
                }
                rg_object_id = fbe_atoh(*argv);
                argc--;
                argv++;

                /* The set control is required
                 */
                if (argc < 1) {
                    fbe_cli_error("emeh: -set -rg_obj <obj_id> <emeh_control> - Is required\n");
                    return;
                }
                set_control = atoi(*argv);
                argc--;
                argv++;

                /* Now look for optional parameters.
                 */
                if (argc > 0) {
                    if (strncmp(*argv, "-inc_prct", 9) == 0) {
                        argc--;
                        argv++;
                        b_set_threshold = FBE_TRUE;
                        if (argc < 1) {
                            fbe_cli_error("emeh: -set Increase percent requires a value. \n");
                            return;
                        }
                        new_increase_threshold = atoi(*argv);
                    } else {
                        fbe_cli_error("emeh: -set Invalid 3rd param: %s\n", *argv);
                        return;
                    }
                }

                /* Send the set EMEH for raid group object request.
                 */
                fbe_cli_lib_set_rg_emeh_values(rg_object_id, set_control, b_set_threshold, new_increase_threshold);

            } else if (strncmp(*argv, "-class", 6) == 0) {
                argc--;
                argv++;
                if (argc < 1) {
                    fbe_cli_error("emeh: -set -class requires a emeh_mode\n");
                    return;
                }
                set_mode = atoi(*argv);
                argc--;
                argv++;

                /* Now look for optional parameters.
                 */
                if (argc > 0) {
                    if (strncmp(*argv, "-inc_prct", 9) == 0) {
                        argc--;
                        argv++;
                        b_set_threshold = FBE_TRUE;
                        if (argc < 1) {
                            fbe_cli_error("emeh: -set Increase percent requires a value. \n");
                            return;
                        }
                        new_increase_threshold = atoi(*argv);
                        argc--;
                        argv++;
                    } else if (strncmp(*argv, "-persist", 8) == 0) {
                        argc--;
                        argv++;
                        b_persist = FBE_TRUE;
                    } else {
                        fbe_cli_error("emeh: -set Invalid 3rd param: %s\n", *argv);
                        return;
                    }

                    /* Now look for persist.
                     */
                    if (argc > 0) {
                        if (strncmp(*argv, "-persist", 8) == 0) {
                            argc--;
                            argv++;
                            b_persist = FBE_TRUE;
                        } else {
                            fbe_cli_error("emeh: -set parameter other than -persist found: %s\n",*argv);
                            return;
                        }
                    }
                } /* end look for optional parameters */

                /* Send the request.
                 */
                fbe_cli_lib_set_class_emeh_values(set_mode, b_set_threshold, new_increase_threshold, b_persist);
            } /* end if -set -class */
        } else  {
            fbe_cli_error("emeh: Invalid param: %s\n", *argv);
            return;
        }
    } else {
        fbe_cli_error("emeh: Invalid param: %s\n", *argv);
        return;
    }

    fbe_cli_printf("\n");		
    return;
    
};
/********************* 
 * end fbe_cli_emeh()
 *********************/

static fbe_status_t fbe_cli_lib_emeh_mode_to_string(fbe_raid_emeh_mode_t emeh_mode, fbe_u8_t **mode_string_pp)
{
    fbe_status_t    status = FBE_STATUS_OK;

    switch(emeh_mode) {
        case FBE_RAID_EMEH_MODE_ENABLED_NORMAL:
            *mode_string_pp = "normal-emeh enabled";
            break;
        case FBE_RAID_EMEH_MODE_DEGRADED_HA:
            *mode_string_pp = "Degraded HA-thresholds disabled";
            break;
        case FBE_RAID_EMEH_MODE_PACO_HA:
            *mode_string_pp = "PACO HA-thresholds disabled";
            break;
        case FBE_RAID_EMEH_MODE_THRESHOLDS_INCREASED:
            *mode_string_pp = "thesholds increased";
            break;
        case FBE_RAID_EMEH_MODE_DISABLED:
            *mode_string_pp = "emeh disabled";
            break;
        case FBE_RAID_EMEH_MODE_INVALID:
        default:
            *mode_string_pp = "UNEXPECTED MODE VALUE";
            status = FBE_STATUS_GENERIC_FAILURE;
            break;
    }
    return status;
}

static fbe_status_t fbe_cli_lib_emeh_command_to_string(fbe_raid_emeh_command_t emeh_command, fbe_u8_t **command_string_pp)
{
    fbe_status_t    status = FBE_STATUS_OK;

    switch(emeh_command) {
        case FBE_RAID_EMEH_COMMAND_INVALID:
            *command_string_pp = "invalid";
            break;
        case FBE_RAID_EMEH_COMMAND_RAID_GROUP_DEGRADED:
            *command_string_pp = "raid group degraded";
            break;
        case FBE_RAID_EMEH_COMMAND_PACO_STARTED:
            *command_string_pp = "paco started";
            break;
        case FBE_RAID_EMEH_COMMAND_RESTORE_NORMAL_MODE:
            *command_string_pp = "set enabled-normal";
            break;
        case FBE_RAID_EMEH_COMMAND_INCREASE_THRESHOLDS:
            *command_string_pp = "increase thresholds";
            break;
        case FBE_RAID_EMEH_COMMAND_DISABLE_EMEH:
            *command_string_pp = "disable EMEH";
            break;
        case FBE_RAID_EMEH_COMMAND_ENABLE_EMEH:
            *command_string_pp = "enable EMEH";
            break;
        default:
            *command_string_pp = "UNEXPECTED COMMAND VALUE";
            status = FBE_STATUS_GENERIC_FAILURE;
            break;
    }
    return status;
}

static void fbe_cli_lib_get_class_emeh_values(void)
{
    fbe_status_t    status = FBE_STATUS_OK;
	fbe_raid_group_class_get_extended_media_error_handling_t get_emeh;
    fbe_u8_t       *mode_string_p = "UNINITIALIZED";        

    status = fbe_api_raid_group_class_get_extended_media_error_handling(&get_emeh);
    if (status != FBE_STATUS_OK) {
        fbe_cli_error("emeh: Failed to get emeh values - status: 0x%x \n", status);
        return;
    }

    fbe_cli_lib_emeh_mode_to_string(get_emeh.current_mode, &mode_string_p);
    fbe_cli_printf("Extended Media Error Handling: Mode: %d (%s)\n", 
                   get_emeh.current_mode, mode_string_p);
    fbe_cli_lib_emeh_mode_to_string(get_emeh.default_mode, &mode_string_p);
    fbe_cli_printf("                       Default Mode: %d (%s)\n", 
                   get_emeh.default_mode, mode_string_p);
    fbe_cli_printf("    Default Increase Threshold Percent: %d \n", 
                   get_emeh.default_threshold_increase);
    fbe_cli_printf("    Current Increase Threshold Percent: %d \n", 
                   get_emeh.current_threshold_increase);
    return;
}

static void fbe_cli_lib_get_rg_emeh_values(fbe_object_id_t rg_object_id)
{
    fbe_status_t    status = FBE_STATUS_OK;
	fbe_raid_group_get_extended_media_error_handling_t get_emeh;
    fbe_u8_t       *mode_string_p = "UNINITIALIZED"; 
    fbe_u32_t       position;      

    if ((rg_object_id == 0)                     ||
        (rg_object_id == FBE_OBJECT_ID_INVALID)    )
    {
        fbe_cli_error("emeh: -get Invalid rg_object_id: 0x%x \n", rg_object_id);
        return;
    }
    status = fbe_api_raid_group_get_extended_media_error_handling(rg_object_id, &get_emeh);
    if (status != FBE_STATUS_OK) 
    {
        fbe_cli_error("emeh: Failed to get emeh values - status: 0x%x \n", status);
        return;
    }

    fbe_cli_lib_emeh_mode_to_string(get_emeh.current_mode, &mode_string_p);
    fbe_cli_printf("Extended Media Error Handling: rg obj: 0x%x Mode: %d (%s)\n", 
                   rg_object_id, get_emeh.current_mode, mode_string_p);
    fbe_cli_lib_emeh_mode_to_string(get_emeh.enabled_mode, &mode_string_p);
    fbe_cli_printf("                       Enabled Mode: %d (%s)\n", 
                   get_emeh.enabled_mode, mode_string_p);
    fbe_cli_printf("    Active EMEH Command (0 if none): %d \n", 
                   get_emeh.active_command);
    fbe_cli_printf("    Proactive Copy VD Position (65535 if none): %d  \n", 
                   get_emeh.paco_position);
    fbe_cli_printf("    Default Increase Threshold Percent: %d \n", 
                   get_emeh.default_threshold_increase);
    fbe_cli_printf("\t\tPer position drive info.\n");
    fbe_cli_printf(" Pos   dieh reliability   dieh mode   media adjust   paco position \n"); 
    for (position = 0; position < get_emeh.width; position++)
    {
    fbe_cli_printf(" [%2d]       %2d             %2d           %2d           %2d\n", 
                   position, get_emeh.dieh_reliability[position],
                   get_emeh.dieh_mode[position], get_emeh.media_weight_adjust[position],
                   get_emeh.vd_paco_position[position]);
    }
    return;
}

static void fbe_cli_lib_set_class_emeh_values(fbe_u32_t set_mode, fbe_bool_t b_set_threshold, fbe_u32_t new_increase_threshold, fbe_bool_t b_persist)
{
    fbe_status_t    status = FBE_STATUS_OK;
    fbe_u8_t       *mode_string_p = "UNINITIALIZED";    

    fbe_cli_printf("emeh class: set values mode: %d persist: %d set threshold: %d increase threshold: %d\n", 
                   set_mode, b_persist, b_set_threshold, new_increase_threshold);
    status = fbe_cli_lib_emeh_mode_to_string(set_mode, &mode_string_p);
    if (status != FBE_STATUS_OK) {
        fbe_cli_error("emeh class: Set mode to: %d (%s) failed - status: 0x%x \n", 
                      set_mode, mode_string_p, status);
        return;
    }

    status = fbe_api_raid_group_class_set_extended_media_error_handling(set_mode, b_set_threshold, new_increase_threshold, b_persist);
    if (status != FBE_STATUS_OK) {
        fbe_cli_error("emeh class: Failed to set emeh values - status: 0x%x \n", status);
        return;
    }
    return;
}

static void fbe_cli_lib_set_rg_emeh_values(fbe_object_id_t rg_object_id, fbe_u32_t set_control, fbe_bool_t b_set_threshold, fbe_u32_t new_increase_threshold)
{
    fbe_status_t    status = FBE_STATUS_OK;
    fbe_u8_t       *control_string_p = "UNINITIALIZED";    

    fbe_cli_printf("emeh rg: set values rg obj: 0x%x control: %d set threshold: %d increase threshold: %d\n", 
                   rg_object_id, set_control, b_set_threshold, new_increase_threshold);
    if ((rg_object_id == 0)                     ||
        (rg_object_id == FBE_OBJECT_ID_INVALID)    )
    {
        fbe_cli_error("emeh: -set Invalid rg_object_id: 0x%x \n", rg_object_id);
        return;
    }
    status = fbe_cli_lib_emeh_command_to_string(set_control, &control_string_p);
    if (status != FBE_STATUS_OK) 
    {
        fbe_cli_error("emeh rg: Set control to: %d (%s) failed - status: 0x%x \n", 
                      set_control, control_string_p, status);
        return;
    }

    status = fbe_api_raid_group_set_extended_media_error_handling(rg_object_id, set_control, b_set_threshold, new_increase_threshold);
    if (status != FBE_STATUS_OK) 
    {
        fbe_cli_error("emeh rg: Failed to set emeh values - status: 0x%x \n", status);
        return;
    }
    return;
}

/**********************************
* end file fbe_cli_lib_emeh_cmds.c
***********************************/
