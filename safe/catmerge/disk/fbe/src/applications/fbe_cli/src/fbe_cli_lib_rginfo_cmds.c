/**************************************************************************
 * Copyright (C) EMC Corporation 2010 
 * All rights reserved. 
 * Licensed material -- property of EMC Corporation
 *************************************************************************/

/*!************************************************************************
* @file fbe_cli_lib_rginfo_cmds.c
 **************************************************************************
 *
 * @brief
 *  This file contains cli functions for the RGINFO related features in FBE CLI
 *  This new command has following syntax
 *  rginfo -d -rg <raidgroup number|all> -object_id <object id | all> 
 *  -debug_flags <value> -library_flags <value> -trace_level <value> 
 * 
 * @ingroup fbe_cli
 *
 * @date
 *  6/11/2010 - Created. Swati Fursule
 *
 **************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include <stdio.h>
#include <math.h>
#include "fbe_cli_private.h"
#include "fbe_trace.h"
#include "fbe/fbe_class.h"
#include "fbe/fbe_api_physical_drive_interface.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_api_common_transport.h"
#include "fbe/fbe_api_trace.h"
#include "fbe/fbe_api_utils.h"
#include "fbe_api_raid_group_interface.h"
#include "fbe_cli_rginfo.h"
#include "fbe/fbe_api_database_interface.h"
#include "fbe/fbe_api_block_transport_interface.h"
#include "fbe_cli_lib_paged_utils.h"
#include "fbe/fbe_api_encryption_interface.h"

/*!*************************************************
 * @typedef fbe_cli_rginfo_fn
 ***************************************************
 * @brief
 *  This is the function pointer we will use
 *  to execute the command for this cli.
 ***************************************************/
struct fbe_cli_rginfo_cmd_s;
typedef void(*fbe_cli_rginfo_fn)(struct fbe_cli_rginfo_cmd_s *cmd_p);
/*!*******************************************************************
 * @struct fbe_cli_rginfo_cmd_t
 *********************************************************************
 * @brief Struct to track our command.
 *
 *********************************************************************/
typedef struct fbe_cli_rginfo_cmd_s
{
    fbe_cli_rginfo_fn                cmd_fn; /*< Fn to perform this cmd. */
    fbe_u32_t                        raid_group_num;
    fbe_object_id_t                  rg_object_id;
    fbe_raid_group_debug_flags_t     set_rg_debug_flags;
    fbe_raid_library_debug_flags_t   set_lib_debug_flags;
    fbe_trace_level_t                set_trace_levels;
    fbe_bool_t                       b_allow_invalid;  /* Allow rg num and id to be invalid */
    fbe_bool_t                       b_user_only;
    fbe_block_transport_set_throttle_info_t set_throttle;
    fbe_raid_group_set_default_bts_params_t set_bts_params;
    fbe_bool_t b_verbose_display;
    fbe_cli_paged_change_t           change;
    fbe_u32_t                        wear_level_timer_interval;
    fbe_raid_group_get_wear_level_t  wear_leveling_info;
}
fbe_cli_rginfo_cmd_t;

void fbe_cli_rginfo_parse_cmds(int argc, char** argv);
void fbe_cli_rginfo_get_rg_class_info(fbe_raid_group_type_t raid_type,
                                      fbe_class_id_t class_id,
                                      fbe_u32_t width,
                                      fbe_lba_t capacity);
void fbe_cli_rginfo_get_rg_object_info(fbe_cli_rginfo_cmd_t *cmd_p);
void fbe_cli_rginfo_set_rg_debug_flag(fbe_cli_rginfo_cmd_t *cmd_p);
void fbe_cli_rginfo_set_lib_debug_flags(fbe_cli_rginfo_cmd_t *cmd_p);
void fbe_cli_rginfo_get_rg_debug_flag(fbe_cli_rginfo_cmd_t *cmd_p);
void fbe_cli_rginfo_get_lib_debug_flags(fbe_cli_rginfo_cmd_t *cmd_p);
void fbe_cli_rginfo_set_rg_tracelevel(fbe_cli_rginfo_cmd_t *cmd_p);
void fbe_cli_rginfo_get_rg_tracelevel(fbe_cli_rginfo_cmd_t *cmd_p);
void fbe_cli_rginfo_display_metadata_bits(fbe_u32_t raid_group_num,
                                          fbe_object_id_t rg_object_id,
                                          fbe_chunk_index_t chunk_offset,
                                          fbe_chunk_index_t chunk_counts,
                                          fbe_bool_t b_force_unit_access);
void fbe_cli_rginfo_display_object_metadata_bits(fbe_object_id_t rg_object_id,
                                                 fbe_chunk_index_t chunk_offset,
                                                 fbe_chunk_index_t chunk_counts,
                                                 fbe_bool_t b_force_unit_access);
void fbe_cli_rginfo_display_rggetinfo(
                              fbe_api_raid_group_get_info_t *getrg_info,
                              fbe_cli_lurg_rg_details_t *rg_details,
                              fbe_bool_t b_verbose);
void fbe_cli_rginfo_display_tracelevelinfo(
                            fbe_api_trace_level_control_t tracelevel_info);
void fbe_cli_rginfo_display_rg_debug_flag_info(
                       fbe_raid_group_debug_flags_t rg_debug_flag_info);
void fbe_cli_rginfo_display_lib_debug_flag_info(
                       fbe_raid_library_debug_flags_t lib_debug_flag_info);
void fbe_cli_rginfo_display_raid_group_flag(
                       fbe_raid_group_flags_t raid_group_flags);
void fbe_cli_rginfo_display_raid_type(
                       fbe_raid_group_type_t raid_type);
void fbe_cli_rginfo_get_class_id(
                              fbe_raid_group_type_t raid_type);
void fbe_cli_rginfo_display_raid_memory_stats(
                   fbe_api_raid_memory_stats_t raid_memory_stats_info);
static fbe_status_t fbe_cli_rginfo_speclz_object(fbe_object_id_t object_id,
                                                         fbe_bool_t *speclz_state);
void fbe_cli_rginfo_map_lba(fbe_u32_t raid_group_num,
                            fbe_object_id_t raid_group_object_id,
                            fbe_lba_t lba);

void fbe_cli_rginfo_map_pba(fbe_u32_t raid_group_num,
                            fbe_object_id_t raid_group_object_id,
                            fbe_lba_t pba, 
                            fbe_raid_position_t position);
void fbe_cli_rginfo_paged_summary(fbe_u32_t raid_group_num,
                                  fbe_object_id_t raid_group_object_id,
                                  fbe_bool_t b_force_unit_access);
void fbe_cli_rginfo_dispatch_cmd(fbe_cli_rginfo_cmd_t *cmd_p);
void fbe_cli_rginfo_display_statistics(fbe_cli_rginfo_cmd_t *cmd_p);
static fbe_status_t fbe_cli_rginfo_parse_paged_switches(const char *name,
                                                        int argc, char **argv,
                                                        fbe_u32_t *args_processed,
                                                        fbe_cli_rginfo_cmd_t *cmd_p);
static void fbe_cli_rginfo_paged_switch(fbe_cli_rginfo_cmd_t *cmd);
static void fbe_cli_rginfo_start_encryption(fbe_cli_rginfo_cmd_t *cmd_p);
static void fbe_cli_rginfo_set_wear_level_timer_interval(fbe_cli_rginfo_cmd_t *cmd_p);
static void fbe_cli_rginfo_get_wear_leveling_info(fbe_cli_rginfo_cmd_t *cmd_p);

/*!*******************************************************************
 * @var fbe_cli_rginfo
 *********************************************************************
 * @brief Function to implement rginfo commands.
 *
 *  @param    argc - argument count
 *  @param    argv - argument string
 *
 * @return - none.  
 *
 * @author
 *  07/01/2010 - Created. Swati Fursule
 *********************************************************************/
void fbe_cli_rginfo(int argc, char** argv)
{
    fbe_cli_printf("%s", "\n");
    fbe_cli_rginfo_parse_cmds(argc, argv);

    return;
}
/******************************************
 * end fbe_cli_rderr()
 ******************************************/

/*!**************************************************************
 * fbe_cli_rginfo_get_throttle_info()
 ****************************************************************
 * @brief
 *  Get and display the throttle information.
 *
 * @param cmd_p - Input command to execute.
 *
 * @return None
 *
 * @author
 *  3/27/2013 - Created. Rob Foley
 *
 ****************************************************************/
void fbe_cli_rginfo_get_throttle_info(fbe_cli_rginfo_cmd_t *cmd_p)
{
    fbe_status_t status;
    fbe_block_transport_get_throttle_info_t get_throttle;
    fbe_u32_t index;

    status = fbe_api_block_transport_get_throttle_info(cmd_p->rg_object_id, FBE_PACKAGE_ID_SEP_0, &get_throttle);
    if (status == FBE_STATUS_OK)
    {
        fbe_cli_printf("RAID Group Object id:  0x%x\n", cmd_p->rg_object_id);
        fbe_cli_printf("io_throttle_count:     %lld\n", get_throttle.io_throttle_count);
        fbe_cli_printf("io_throttle_max:       %lld\n", get_throttle.io_throttle_max);
        fbe_cli_printf("outstanding_io_count:  %lld\n", get_throttle.outstanding_io_count);
        fbe_cli_printf("outstanding_io_max:    %d\n", get_throttle.outstanding_io_max);
        fbe_cli_printf("outstanding_io_credits: %d\n", get_throttle.outstanding_io_credits);
        fbe_cli_printf("io_credits_max:         %d\n", get_throttle.io_credits_max);
        for (index = 0; index < FBE_PACKET_PRIORITY_LAST - 1; index++)
        {
            fbe_cli_printf("%d) %d\n", index, get_throttle.queue_length[index]);
        }
    }
    else
    {
        fbe_cli_printf("rginfo: ERROR: status of %d from getting throttle info\n", status);
    }
}
/******************************************
 * end fbe_cli_rginfo_get_throttle_info()
 ******************************************/

/*!**************************************************************
 * fbe_cli_rginfo_parse_set_throttle_info()
 ****************************************************************
 * @brief
 *  Set the throttles of the raid group.
 *
 * @param argc
 * @param argv
 * @param cmd_p - Input command to execute.
 *
 * @return None
 *
 * @author
 *  3/27/2013 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t fbe_cli_rginfo_parse_set_throttle_info(int argc, char** argv,
                                                    fbe_cli_rginfo_cmd_t *cmd_p)
{
    if (argc >= 2)
    {
        cmd_p->set_throttle.outstanding_io_max = (fbe_u32_t)strtoul(*argv, 0, 0);
        argv++;
        cmd_p->set_throttle.io_credits_max = (fbe_u32_t)strtoul(*argv, 0, 0);
        argv++;
        cmd_p->set_throttle.io_throttle_max = (fbe_u32_t)strtoul(*argv, 0, 0);
    }
    else
    {
        fbe_cli_printf("rginfo: ERROR: -set_throttle [io max][max_credits][bw throttle]\n");
        return FBE_STATUS_GENERIC_FAILURE;
    }
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_cli_rginfo_parse_set_throttle_info()
 ******************************************/
/*!**************************************************************
 * fbe_cli_rginfo_init_cmd()
 ****************************************************************
 * @brief
 *  Initialize the command object.
 *
 * @param  cmd_p - ptr to object to init.               
 *
 * @return None.
 *
 * @author
 *  4/22/2013 - Created. Rob Foley
 *
 ****************************************************************/

void fbe_cli_rginfo_init_cmd(fbe_cli_rginfo_cmd_t *cmd_p)
{
    cmd_p->cmd_fn = fbe_cli_rginfo_get_rg_object_info; /* Default to just display all rgs. */
    cmd_p->raid_group_num = FBE_RAID_ID_INVALID;
    cmd_p->rg_object_id = FBE_OBJECT_ID_INVALID;
    cmd_p->set_rg_debug_flags = 0;
    cmd_p->set_lib_debug_flags = 0;
    cmd_p->set_trace_levels = 0;
    cmd_p->b_allow_invalid = FBE_FALSE;
    cmd_p->b_verbose_display = FBE_FALSE;
    cmd_p->b_user_only = FBE_FALSE;
    cmd_p->wear_level_timer_interval = 0;
    
    memset(&cmd_p->change, 0, sizeof(cmd_p->change));
    memset(&cmd_p->wear_leveling_info, 0, sizeof(cmd_p->wear_leveling_info));
}
/******************************************
 * end fbe_cli_rginfo_init_cmd()
 ******************************************/

/*!**************************************************************
 * fbe_cli_rginfo_set_throttle_cmd()
 ****************************************************************
 * @brief
 *  Initialize the command object.
 *
 * @param  cmd_p - ptr to object to init.               
 *
 * @return None.
 *
 * @author
 *  4/22/2013 - Created. Rob Foley
 *
 ****************************************************************/
void fbe_cli_rginfo_set_throttle_cmd(fbe_cli_rginfo_cmd_t *cmd_p)
{
    fbe_status_t status;
    status = fbe_api_block_transport_set_throttle_info(cmd_p->rg_object_id, &cmd_p->set_throttle);
    if (status == FBE_STATUS_OK)
    {
        fbe_cli_printf("rginfo: RG Object id: 0x%x successfully set throttle info\n", cmd_p->rg_object_id);
    }
    else
    {
        fbe_cli_printf("rginfo: ERROR: status of %d from getting throttle info rg: 0x%x\n", status, cmd_p->rg_object_id);
    }
}
/**************************************
 * end fbe_cli_rginfo_set_throttle_cmd()
 **************************************/
/*!**************************************************************
 * fbe_cli_rginfo_parse_set_bts_params()
 ****************************************************************
 * @brief
 *  Set the throttles of the raid group.
 *
 * @param argc
 * @param argv
 * @param args_processed_p - Ptr to number of args we dealt with
 * @param cmd_p - Ptr to current rg command.
 *
 * @return None
 *
 * @author
 *  4/29/2013 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t fbe_cli_rginfo_parse_set_bts_params(int argc, char** argv,
                                                 fbe_u32_t *args_processed_p,
                                                 fbe_cli_rginfo_cmd_t *cmd_p)
{
    *args_processed_p = 0;
    if (argc >= 2)
    {
        cmd_p->set_bts_params.b_throttle_enabled = (fbe_u32_t)strtoul(*argv, 0, 0);
        argv++;
        cmd_p->set_bts_params.user_queue_depth = (fbe_u32_t)strtoul(*argv, 0, 0);
        argv++;
        cmd_p->set_bts_params.system_queue_depth = (fbe_u32_t)strtoul(*argv, 0, 0);
        *args_processed_p = 2;
    }
    else
    {
        fbe_cli_printf("rginfo: ERROR: -set_bts_params [enable (1 or 0)][user queue depth][system queue depth]\n");
        return FBE_STATUS_GENERIC_FAILURE;
    }
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_cli_rginfo_parse_set_bts_params()
 ******************************************/
/*!**************************************************************
 * fbe_cli_rginfo_set_bts_params()
 ****************************************************************
 * @brief
 *  Set the throttle params for the raid group.
 *  The params were setup when we parsed the command string.
 *
 * @param  cmd_p - ptr to cmd object.               
 *
 * @return None.
 *
 * @author
 *  4/26/2013 - Created. Rob Foley
 *  
 ****************************************************************/
void fbe_cli_rginfo_set_bts_params(fbe_cli_rginfo_cmd_t *cmd_p)
{
    fbe_status_t status;
    status = fbe_api_raid_group_set_bts_params(&cmd_p->set_bts_params);
    if (status == FBE_STATUS_OK)
    {
        fbe_cli_printf("rginfo: successfully set bts params info\n");
    }
    else
    {
        fbe_cli_printf("rginfo: ERROR: status of %d from setting bts params\n", status);
    }
}
/**************************************
 * end fbe_cli_rginfo_set_bts_params()
 **************************************/

/*!**************************************************************
 * fbe_cli_rginfo_get_bts_params()
 ****************************************************************
 * @brief
 *  Fetch/display the Raid Group throttle params.
 *
 * @param  cmd_p - ptr to cmd object.
 *
 * @return None.
 *
 * @author
 *  4/29/2013 - Created. Rob Foley
 *  
 ****************************************************************/
void fbe_cli_rginfo_get_bts_params(fbe_cli_rginfo_cmd_t *cmd_p)
{
    fbe_status_t status;
    fbe_raid_group_get_default_bts_params_t params;

    status = fbe_api_raid_group_get_bts_params(&params);

    if (status == FBE_STATUS_OK)
    {
        fbe_cli_printf("rginfo: Default BTS params\n");
        fbe_cli_printf("Throttle Enabled:   %s\n", params.b_throttle_enabled ? "YES" : "NO");
        fbe_cli_printf("System Queue Depth: %d\n", params.system_queue_depth);
        fbe_cli_printf("User Queue Depth:   %d\n", params.user_queue_depth);
    }
    else
    {
        fbe_cli_printf("rginfo: ERROR: status of %d from getting BTS params\n", status);
    }
}
/**************************************
 * end fbe_cli_rginfo_get_bts_params()
 **************************************/
/*!**************************************************************
 * fbe_cli_rginfo_set_cmd_state()
 ****************************************************************
 * @brief
 *  set the state of the cmd object
 *
 * @param  cmd_p - ptr to object.         
 * @param cmd_fn - Function to set into the cmd.     
 *
 * @return None.
 *
 * @author
 *  4/22/2013 - Created. Rob Foley
 *
 ****************************************************************/
void fbe_cli_rginfo_set_cmd_state(fbe_cli_rginfo_cmd_t *cmd_p,
                                  fbe_cli_rginfo_fn cmd_fn)
{
    cmd_p->cmd_fn = cmd_fn;
}
/**************************************
 * end fbe_cli_rginfo_set_cmd_state()
 **************************************/
/*!**************************************************************
 *          fbe_cli_rginfo_parse_cmds()
 ****************************************************************
 *
 * @brief   Parse rginfo commands
 * rginfo -d -rg <raidgroup number|all> |-object_id <object id | all> \n\
 * -default_debug_flags|ddf <value> -default_library_flags|dlf <value> \n\
 * -debug_flags|df <value> -library_flags|lf <value> -trace_level|tl <value> \n\
 * -display_raid_memory_stats|disp_rms\n\
 * @param   argument count
 * @param arguments list
 *
 * @return  None.
 *
 * @author
 *  07/01/2010 - Created. Swati Fursule
 *
 ****************************************************************/
void fbe_cli_rginfo_parse_cmds(int argc, char** argv)
{
    fbe_cli_rginfo_cmd_t    cmd;
    fbe_status_t            status = FBE_STATUS_OK;
    fbe_u32_t               args_processed = 0;
    fbe_bool_t              b_force_unit_access = FBE_FALSE; /* By default allow read from cache */
    fbe_u32_t               fua = 0;
    fbe_char_t             *tmp_ptr = NULL;

    fbe_cli_rginfo_init_cmd(&cmd);
    /* 
     * Parse the rginfo commands and call particular functions
     * rginfo -d -rg <raidgroup number|all> |-object_id <object id | all> \n\
     * -default_debug_flags|ddf <value> -default_library_flags|dlf <value> \n\
     * -debug_flags|df <value> -library_flags|lf <value> -trace_level|tl <value> \n\
     * -display_raid_memory_stats|disp_rms\n\
     */
    if (argc == 0)
    {
        fbe_cli_printf("rginfo: ERROR: Too few args.\n");
        fbe_cli_printf("%s", RGINFO_CMD_USAGE);
        return;
    }
    if ((strcmp(*argv, "-help") == 0) ||
        (strcmp(*argv, "-h") == 0) )
    {
        /* If they are asking for help, just display help and exit.
        */
        fbe_cli_printf("%s", RGINFO_CMD_USAGE);
        return;
    }
    /* Parse args
     */
    while (argc && **argv == '-')
    {
        argc--;
        /* Display the raid group information. 
         */
        if (!strcmp(*argv, "-d"))
        {
            fbe_cli_rginfo_set_cmd_state(&cmd, fbe_cli_rginfo_get_rg_object_info);
        }
        else if (!strcmp(*argv, "-v"))
        {
            cmd.b_verbose_display = FBE_TRUE;
            fbe_cli_rginfo_set_cmd_state(&cmd, fbe_cli_rginfo_get_rg_object_info);
        }
        else if (!strcmp(*argv, "-rg") )
        {
            /* Get the RG number from command line */
            argc--;
            argv++;

            /* argc should not be less than 0 */
            if (argc < 0)
            {
                fbe_cli_printf("rginfo: ERROR: Raid group number is expected \n");
                fbe_cli_printf("%s", RGINFO_CMD_USAGE);
                return;
            }

            cmd.rg_object_id = FBE_OBJECT_ID_INVALID;
            /* Get the raid group number for which to get/set the information. 
             */
            if (!strcmp(*argv, "all"))
            {
                cmd.raid_group_num = FBE_RAID_ID_INVALID;
            }
            else if (!strcmp(*argv, "user")) 
            {
                cmd.raid_group_num = FBE_RAID_ID_INVALID;
                cmd.b_user_only = FBE_TRUE;
            }
            else
            {
                /* get the value*/
                tmp_ptr = *argv;
                if ((*tmp_ptr == '0') && 
                    (*(tmp_ptr + 1) == 'x' || *(tmp_ptr + 1) == 'X'))
                {
                    fbe_cli_printf("rginfo: ERROR: Invalid argument (Raid group number should be in decimal).\n");
                    fbe_cli_printf("%s", RGINFO_CMD_USAGE);
                    return;
                }
                cmd.raid_group_num = atoi(tmp_ptr);
                if (cmd.raid_group_num < 0) 
                {
                    fbe_cli_printf("rginfo: ERROR: Invalid argument (Raid group number).\n");
                    fbe_cli_printf("%s", RGINFO_CMD_USAGE);
                    return;
                }
            }
        }
        else if (!strcmp(*argv, "-object_id") )
        {
            /* Get the RG number from command line */
            argc--;
            argv++;

            /* argc should not be less than 0 */
            if (argc < 0)
            {
                fbe_cli_printf("rginfo: ERROR: Object Id argument is expected \n");
                fbe_cli_printf("%s", RGINFO_CMD_USAGE);
                return;
            }
            
            cmd.raid_group_num = FBE_RAID_ID_INVALID;
            /* Get the raid group number for which to get/set the information. 
             */
            if (!strcmp(*argv, "all"))
            {
                cmd.rg_object_id = FBE_OBJECT_ID_INVALID;
            } 
            else if (!strcmp(*argv, "user")) 
            {
                cmd.rg_object_id = FBE_OBJECT_ID_INVALID;
                cmd.b_user_only = FBE_TRUE;
            }
            else
            {
                /* get the value*/
                cmd.rg_object_id = fbe_atoh(*argv);
            }
        }
        else if (!strcmp(*argv, "-get_throttle"))
        {
            fbe_cli_rginfo_set_cmd_state(&cmd, fbe_cli_rginfo_get_throttle_info);
        }
        else if (!strcmp(*argv, "-set_throttle"))
        {
            fbe_cli_rginfo_set_cmd_state(&cmd, fbe_cli_rginfo_set_throttle_cmd);
            argc--;
            argv++;
            status = fbe_cli_rginfo_parse_set_throttle_info(argc, argv, &cmd);
            if (status != FBE_STATUS_OK)
            {
                return;
            }
            if (argc >= 2) {
                argc -= 2;
                argv++;
                argv++;
            }
            else {
                argc = 0;
            }
        }
        else if (!strcmp(*argv, "-get_bts_params"))
        {
            cmd.b_allow_invalid = FBE_TRUE;
            fbe_cli_rginfo_set_cmd_state(&cmd, fbe_cli_rginfo_get_bts_params);
        }
        else if (!strcmp(*argv, "-reload_bts_params"))
        {
            cmd.b_allow_invalid = FBE_TRUE;
            fbe_cli_rginfo_set_cmd_state(&cmd, fbe_cli_rginfo_set_bts_params);
            cmd.set_bts_params.b_persist = FBE_FALSE;
            cmd.set_bts_params.b_reload = FBE_TRUE;
        }
        else if (!strcmp(*argv, "-set_bts_params") ||
                 !strcmp(*argv, "-persist_bts_params"))
        {
            cmd.b_allow_invalid = FBE_TRUE;
            cmd.set_bts_params.b_persist = FBE_FALSE;
            cmd.set_bts_params.b_reload = FBE_FALSE;
            if (argc > 0 && !strcmp(*argv, "-persist_bts_params"))
            {
                cmd.set_bts_params.b_persist = FBE_TRUE;
            }
            fbe_cli_rginfo_set_cmd_state(&cmd, fbe_cli_rginfo_set_bts_params);
            argc--;
            argv++;
            status = fbe_cli_rginfo_parse_set_bts_params(argc, argv, &args_processed, &cmd);
            if (status != FBE_STATUS_OK)
            {
                return;
            }
            argc -= args_processed;
            argv += args_processed;
        }
        else if ( (!strcmp(*argv, "-get_lib_debug_flags") ) ||
                  (!strcmp(*argv, "-gldf") ) )
        {
            fbe_cli_rginfo_set_cmd_state(&cmd, fbe_cli_rginfo_get_lib_debug_flags);
            cmd.b_allow_invalid = FBE_TRUE;
        }
        else if ( (!strcmp(*argv, "-lf")) ||
                  (!strcmp(*argv, "-dlf")) ||
                  (!strcmp(*argv, "-library_flags")) ||
                  (!strcmp(*argv, "-default_library_flags") ) )
        {
            if ( (!strcmp(*argv, "-default_library_flags")) ||
                  (!strcmp(*argv, "-dlf")) )
            {
                /* default library flag is the flag set for all RAID groups along with newly created RG
                 * fbe_api_raid_group_send_set_debug_to_all_raid_group_classes is being called 
                 * we need to set object id as invalid, 
                 * we also need to set raid_group_num to invalid, since inside fbe_cli_rginfo_set_lib_debug_flags,
                 * we calculate the object id if raid group number is valid.
                */
                cmd.raid_group_num = FBE_RAID_ID_INVALID;
                cmd.rg_object_id = FBE_OBJECT_ID_INVALID;
                cmd.b_allow_invalid = FBE_TRUE;
            }

            {
                if ( argc == 0)
                {
                    fbe_cli_printf("rginfo: ERROR: Too few args.\n");
                    fbe_cli_printf("%s", RGINFO_CMD_USAGE);
                    return;
                }
                argc--;
                argv++;
                /* Set the default raid library debug flag.
                 */
                /* get the value*/
                cmd.set_lib_debug_flags = fbe_atoh(*argv);
                /*!todo : accept multiple flags value*/
                if ((cmd.set_lib_debug_flags < 0) || 
                    (cmd.set_lib_debug_flags > FBE_RAID_LIBRARY_DEBUG_FLAG_LAST))
                {
                    fbe_cli_printf("rginfo: ERROR: Invalid argument (library debug flags is less than 0 or greater than FBE_RAID_LIBRARY_DEBUG_FLAG_LAST 0x%x).\n", FBE_RAID_LIBRARY_DEBUG_FLAG_LAST);
                    fbe_cli_printf("%s", RGINFO_CMD_USAGE);
                    return;
                }
                fbe_cli_rginfo_set_cmd_state(&cmd, fbe_cli_rginfo_set_lib_debug_flags);
            }
        }
        else if ( (!strcmp(*argv, "-get_rg_debug_flags") ) ||
                  (!strcmp(*argv, "-grdf") ) )
        {
            fbe_cli_rginfo_set_cmd_state(&cmd, fbe_cli_rginfo_get_rg_debug_flag);
            cmd.b_allow_invalid = FBE_TRUE;
        }
        else if ( (!strcmp(*argv, "-df")) ||
                  (!strcmp(*argv, "-ddf")) ||
                  (!strcmp(*argv, "-debug_flags")) ||
                  (!strcmp(*argv, "-default_debug_flags")) )
        {
            if ( (!strcmp(*argv, "-default_debug_flags")) ||
                 (!strcmp(*argv, "-ddf")))
            {
                /* default debug flag is the flag set for all RAID groups along with newly created RG
                 * fbe_api_raid_group_send_set_debug_to_all_raid_group_classes is being called 
                 * we need to set object id as invalid, 
                 * we also need to set raid_group_num to invalid, since inside fbe_cli_rginfo_set_lib_debug_flags,
                 * we calculate the object id if raid group number is valid.
                */
                cmd.raid_group_num = FBE_RAID_ID_INVALID;
                cmd.rg_object_id = FBE_OBJECT_ID_INVALID;
                cmd.b_allow_invalid = FBE_TRUE;
            }
           
            {
                if ( argc == 0)
                {
                    fbe_cli_printf("rginfo: ERROR: Too few args.\n");
                    fbe_cli_printf("%s", RGINFO_CMD_USAGE);
                    return;
                }
                argc--;
                argv++;
                /* Set the default raid library debug flag.
                 */
                /* get the value*/
                cmd.set_rg_debug_flags = fbe_atoh(*argv);
                /*!todo : accept multiple flags value*/
                if ((cmd.set_rg_debug_flags == 0) || 
                    ((cmd.set_rg_debug_flags & FBE_RAID_GROUP_DEBUG_FLAG_INVALID_MASK) != 0))
                {
                    fbe_cli_printf("rginfo: ERROR: Invalid argument: 0x%x (raid group debug flags is either less than 0 or not valid: FBE_RAID_GROUP_DEBUG_FLAG_VALID_MASK 0x%x ).\n", 
                                   cmd.set_rg_debug_flags, FBE_RAID_GROUP_DEBUG_FLAG_VALID_MASK);
                    fbe_cli_printf("%s", RGINFO_CMD_USAGE);
                    return;
                }
                fbe_cli_rginfo_set_cmd_state(&cmd, fbe_cli_rginfo_set_rg_debug_flag);
            }
        }
        else if ( (!strcmp(*argv, "-get_trace_level") ) ||
                  (!strcmp(*argv, "-gtl") ) )
        {
            fbe_cli_rginfo_set_cmd_state(&cmd, fbe_cli_rginfo_get_rg_tracelevel);
            cmd.b_allow_invalid = FBE_TRUE;
        }
        else if ( (!strcmp(*argv, "-display_stats") ) ||
                  (!strcmp(*argv, "-ds") ) )
        {
            fbe_cli_rginfo_set_cmd_state(&cmd, fbe_cli_rginfo_display_statistics);
        }
        else if ( (!strcmp(*argv, "-trace_level") ) ||
                  (!strcmp(*argv, "-tl") ) )
        {
            
            {
                if ( argc == 0)
                {
                    fbe_cli_printf("rginfo: ERROR: Too few args.\n");
                    fbe_cli_printf("%s", RGINFO_CMD_USAGE);
                    return;
                }
                argc--;
                argv++;
                /* Set the default raid trace level flag.
                 */
                /* get the value*/
                cmd.set_trace_levels = fbe_atoh(*argv);
                /*!todo : accept multiple flags value*/
                if ((cmd.set_trace_levels < 0) || 
                    (cmd.set_trace_levels > FBE_TRACE_LEVEL_LAST))
                {
                    fbe_cli_printf("rginfo: ERROR: Invalid argument (trace levels).\n");
                    fbe_cli_printf("%s", RGINFO_CMD_USAGE);
                    return;
                }
                fbe_cli_rginfo_set_cmd_state(&cmd, fbe_cli_rginfo_set_rg_tracelevel);
                cmd.b_allow_invalid = FBE_TRUE;
            }
            if (status != FBE_STATUS_OK)
            {
                fbe_cli_printf("rginfo: ERROR: Not able to get/set trace level info \n");
                return;
            }
        }
        else if ( (!strcmp(*argv, "-display_raid_memory_stats") ) ||
                  (!strcmp(*argv, "-disp_rms") ) )
        {
            fbe_api_raid_memory_stats_t memory_statistics;
            status = fbe_api_raid_group_get_raid_memory_stats(&memory_statistics);
            if (status != FBE_STATUS_OK)
            {
                fbe_cli_printf("rginfo: ERROR: Not able to get raid memory statistics\n");
                return;
            }
            fbe_cli_rginfo_display_raid_memory_stats(memory_statistics);
            return;
        }
        else if ( (!strcmp(*argv, "-reset_raid_memory_stats") ) ||
                  (!strcmp(*argv, "-reset_rms") ) )
        {
            status = fbe_api_raid_group_reset_raid_memory_stats();
            if (status != FBE_STATUS_OK)
            {
                fbe_cli_printf("rginfo: ERROR: Not able to reset raid memory statistics\n");
                return;
            }
            return;
        }
        else if (!strcmp(*argv, "-map_lba") )
        {
            fbe_lba_t lba;

            argc--;
            argv++;

            /* argc should not be less than 0 */
            if (argc < 0)
            {
                fbe_cli_printf("rginfo: ERROR: -map_lba lba expected\n");
                fbe_cli_printf("%s", RGINFO_CMD_USAGE);
                return;
            }
            
            /* get the offset*/
            status = fbe_atoh64(*argv, &lba);
            if (status != FBE_STATUS_OK)
            {
                fbe_cli_error("rginfo error processing lba for map lba to pba\n");
                return;
            }
            fbe_cli_rginfo_map_lba(cmd.raid_group_num, cmd.rg_object_id, lba);
            return;
        }
        else if (!strcmp(*argv, "-map_pba") )
        {
            fbe_lba_t pba;
            fbe_raid_position_t position;

            argc--;
            argv++;

            /* argc should not be less than 0 */
            if (argc < 0)
            {
                fbe_cli_printf("rginfo: ERROR: -map_pba pba expected\n");
                fbe_cli_printf("%s", RGINFO_CMD_USAGE);
                return;
            }
            
            /* get the pba */
            status = fbe_atoh64(*argv, &pba);
            if (status != FBE_STATUS_OK)
            {
                fbe_cli_error("rginfo error processing pba for map pba to lba\n");
                return;
            }
            argc--;
            argv++;
            /* argc should not be less than 0 */
            if (argc < 0)
            {
                fbe_cli_printf("rginfo: ERROR: -map_pba position expected\n");
                fbe_cli_printf("%s", RGINFO_CMD_USAGE);
                return;
            }
            position = fbe_atoh(*argv);

            fbe_cli_rginfo_map_pba(cmd.raid_group_num, cmd.rg_object_id, pba, position);
            return;
        }
        else if (!strcmp(*argv, "-chunk_info") )
        {
            fbe_chunk_index_t  chunk_offset;
            fbe_chunk_index_t  chunk_counts;  

            argc--;
            argv++;

            /* argc should not be less than 0 */
            if (argc < 1)
            {
                fbe_cli_printf("rginfo: ERROR: chunk offset / counts arguments are expected \n");
                fbe_cli_printf("%s", RGINFO_CMD_USAGE);
                return;
            }
            
            /* get the offset*/
            chunk_offset = fbe_atoh(*argv);
            
            argc--;
            argv++;

            /* get the count*/
            chunk_counts = fbe_atoh(*argv);

            /* Determine if FUA was requested */
            if (argc > 0)
            {
                argc--;
                argv++;
                if(!strcmp(*argv, "-fua") )
                {
                    /* argc should be greater than 0 here */
                    if (argc <= 0)
                    {
                        fbe_cli_error("Please provide value for -fua switch. \n\n");
                        return;
                    }
                    argc--;
                    argv++;
                    fua = fbe_atoh(*argv);
                    b_force_unit_access = (fua == 0) ? FBE_FALSE : FBE_TRUE;
                }
            }
            fbe_cli_rginfo_display_metadata_bits(cmd.raid_group_num, 
                                                 cmd.rg_object_id, 
                                                 chunk_offset,
                                                 chunk_counts,
                                                 b_force_unit_access);
            return;
        }
        else if (!strcmp(*argv, "-paged_summary") )
        {
            /* By default the summary gets the data from disk */
            b_force_unit_access = FBE_TRUE;

            /* Determine if FUA was requested */
            if (argc > 0)
            {
                argc--;
                argv++;
                if(!strcmp(*argv, "-fua") )
                {
                    /* argc should be greater than 0 here */
                    if (argc <= 0)
                    {
                        fbe_cli_error("Please provide value for -fua switch. \n\n");
                        return;
                    }
                    argc--;
                    argv++;
                    fua = fbe_atoh(*argv);
                    b_force_unit_access = (fua == 0) ? FBE_FALSE : FBE_TRUE;
                }
            }
            fbe_cli_rginfo_paged_summary(cmd.raid_group_num, cmd.rg_object_id, b_force_unit_access);
            return;
        }
        else if ( (!strcmp(*argv, "-sdf")) ||
                  (!strcmp(*argv, "-udf")) ||
                  (!strcmp(*argv, "-user_default_debug_flags")) ||
                  (!strcmp(*argv, "-system_default_debug_flags")) )
        {
            fbe_bool_t b_system = FBE_FALSE;
            fbe_bool_t b_user = FBE_FALSE;
            fbe_raid_group_debug_flags_t set_user_rg_debug_flags = FBE_RAID_GROUP_DEBUG_FLAG_RESERVED_IGNORE_SET;
            fbe_raid_group_debug_flags_t set_system_rg_debug_flags = FBE_RAID_GROUP_DEBUG_FLAG_RESERVED_IGNORE_SET;
            fbe_raid_group_debug_flags_t set_rg_debug_flags;
            if ( (!strcmp(*argv, "-sdf")) ||
                 (!strcmp(*argv, "-system_default_debug_flags")) )
            {
                b_system = FBE_TRUE;
            }
            if ( (!strcmp(*argv, "-udf")) ||
                 (!strcmp(*argv, "-user_default_debug_flags")) )
            {
                b_user = FBE_TRUE;
            }
                
            if ( argc == 0)
            {
                fbe_cli_printf("rginfo: ERROR: Too few args.\n");
                fbe_cli_printf("%s", RGINFO_CMD_USAGE);
                return;
            }
            argc--;
            argv++;
            /* Get the value to set.
             */ 
            set_rg_debug_flags = fbe_atoh(*argv);

            if ((set_rg_debug_flags & FBE_RAID_GROUP_DEBUG_FLAG_INVALID_MASK) != 0)
            {
                fbe_cli_printf("rginfo: ERROR: Invalid argument flag 0x%x not valid: FBE_RAID_GROUP_DEBUG_FLAG_VALID_MASK 0x%x\n", 
                               set_rg_debug_flags, FBE_RAID_GROUP_DEBUG_FLAG_VALID_MASK);
                fbe_cli_printf("%s", RGINFO_CMD_USAGE);
                return;
            }

            if (b_system == FBE_TRUE)
            {
                set_system_rg_debug_flags = set_rg_debug_flags;
                fbe_cli_printf("setting system default debug flags: 0x%x\n", set_system_rg_debug_flags);
            }
            if (b_user == FBE_TRUE)
            {
                set_user_rg_debug_flags = set_rg_debug_flags;
                fbe_cli_printf("setting user default debug flags: 0x%x\n", set_user_rg_debug_flags);
            }
            status = fbe_api_raid_group_set_default_debug_flags(FBE_CLASS_ID_PARITY,
                                                                set_user_rg_debug_flags,
                                                                set_system_rg_debug_flags);
            if (status != FBE_STATUS_OK)
            {
                fbe_cli_error("failed setting rg debug flags status: 0x%x\n", status);
            }
            else
            {
                fbe_cli_printf("Successfully set rg debug flags\n");
            }
            return;
        }
        else if ( (!strcmp(*argv, "-slf")) ||
                  (!strcmp(*argv, "-ulf")) ||
                  (!strcmp(*argv, "-user_default_library_flags")) ||
                  (!strcmp(*argv, "-system_default_library_flags")) )
        {
            fbe_bool_t b_system = FBE_FALSE;
            fbe_bool_t b_user = FBE_FALSE;
            fbe_raid_group_debug_flags_t set_user_library_debug_flags = FBE_RAID_LIBRARY_DEBUG_FLAG_LAST;
            fbe_raid_group_debug_flags_t set_system_library_debug_flags = FBE_RAID_LIBRARY_DEBUG_FLAG_LAST;
            fbe_raid_group_debug_flags_t set_lib_debug_flags;

            if ( (!strcmp(*argv, "-slf")) ||
                 (!strcmp(*argv, "-system_default_library_flags")))
            {
                b_system = FBE_TRUE;
            }
            if ( (!strcmp(*argv, "-ulf")) ||
                 (!strcmp(*argv, "-user_default_library_flags")) )
            {
                b_user = FBE_TRUE;
            }
                
            if ( argc == 0)
            {
                fbe_cli_printf("rginfo: ERROR: Too few args.\n");
                fbe_cli_printf("%s", RGINFO_CMD_USAGE);
                return;
            }
            argc--;
            argv++;

            /* Get the value to set.
             */ 
            set_lib_debug_flags = fbe_atoh(*argv);
            if (set_lib_debug_flags >= FBE_RAID_LIBRARY_DEBUG_FLAG_LAST)
            {
                fbe_cli_printf("rginfo: ERROR: Invalid argument.  Flag 0x%x not recognized.\n", set_lib_debug_flags);
                fbe_cli_printf("%s", RGINFO_CMD_USAGE);
                return;
            }

            if (b_system == FBE_TRUE)
            {
                set_system_library_debug_flags = set_lib_debug_flags;
                fbe_cli_printf("setting system default library flags: 0x%x\n", set_system_library_debug_flags);
            }
            if (b_user == FBE_TRUE)
            {
                set_user_library_debug_flags = set_lib_debug_flags;
                fbe_cli_printf("setting user default library flags: 0x%x\n", set_user_library_debug_flags);
            }
            status = fbe_api_raid_group_set_default_library_flags(FBE_CLASS_ID_PARITY,
                                                                  set_user_library_debug_flags,
                                                                  set_system_library_debug_flags);
            if (status != FBE_STATUS_OK)
            {
                fbe_cli_error("failed setting default library flags status: 0x%x\n", status);
            }
            else
            {
                fbe_cli_printf("Successfully set default library flags\n");
            }
            return;
        }
        else if (!strcmp(*argv, "-paged_set_bits")   ||
                 !strcmp(*argv, "-paged_clear_bits") ||
                 !strcmp(*argv, "-paged_write")      ||
                 !strcmp(*argv, "-clear_cache")         )
        {
            char *name = *argv;
            fbe_cli_paged_change_t *change = &cmd.change;

            fbe_cli_rginfo_set_cmd_state(&cmd, fbe_cli_rginfo_paged_switch);
            if (!strcmp(*argv, "-paged_set_bits")) {
                change->operation = FBE_CLI_PAGED_OP_SET_BITS;
            } else if (!strcmp(*argv, "-paged_clear_bits")) {
                change->operation = FBE_CLI_PAGED_OP_CLEAR_BITS;
            } else if (!strcmp(*argv, "-paged_write")) {
                change->operation = FBE_CLI_PAGED_OP_WRITE;
            } else if (strcmp(argv[0], "-clear_cache") == 0) {
                change->operation = FBE_CLI_PAGED_OP_CLEAR_CACHE;
            } else {
                fbe_cli_error("%s:%u: Internal error, invalid command: %s\n",
                              __FUNCTION__, __LINE__, *argv);
                return;
            }

            argv++;
            status = fbe_cli_rginfo_parse_paged_switches(name, argc, argv, &args_processed, &cmd);
            if (status != FBE_STATUS_OK) {
                return;
            }
            argv += args_processed;
            argc -= args_processed;
        }
        else if (!strcmp(*argv, "-start_enc"))
        {
            fbe_cli_rginfo_set_cmd_state(&cmd, fbe_cli_rginfo_start_encryption);
        }
        else if (!strcmp(*argv, "-wear_level_timer"))
        {
            argv++;
            argc--;
            cmd.wear_level_timer_interval = fbe_atoh(*argv);
            fbe_cli_rginfo_set_cmd_state(&cmd, fbe_cli_rginfo_set_wear_level_timer_interval);
            cmd.b_allow_invalid = FBE_TRUE;
        }
        else if (!strcmp(*argv, "-wear_level"))
        {
            fbe_cli_rginfo_set_cmd_state(&cmd, fbe_cli_rginfo_get_wear_leveling_info);
            cmd.b_allow_invalid = FBE_FALSE;
        }
        else
        {
            fbe_cli_printf("rginfo: ERROR: Invalid switch.\n");
            fbe_cli_printf("%s", RGINFO_CMD_USAGE);
            return;
        }
        argv++;
    }
    fbe_cli_rginfo_dispatch_cmd(&cmd);
    return;
}
/******************************************
 * end fbe_cli_rginfo_parse_cmds()
 ******************************************/
/*!**************************************************************
 * fbe_cli_rginfo_dispatch_cmd()
 ****************************************************************
 * @brief
 *  Run the input command.
 *  We will either run it against one raid group or
 *  if no raid group is specified we will enumerate all
 *  raid groups and run it against several.
 *           
 * @param cmd_p - Input command to execute.
 *
 * @return -   
 *
 * @author
 *  4/19/2013 - Created. Rob Foley
 *
 ****************************************************************/
void fbe_cli_rginfo_dispatch_cmd(fbe_cli_rginfo_cmd_t *cmd_p)
{
    fbe_status_t status;
    fbe_u32_t num_objects = 0;
    fbe_object_id_t *obj_list_p = NULL;
    fbe_status_t nest_status;
    fbe_u32_t index;
    fbe_u32_t class_index;
    fbe_cli_rginfo_cmd_t current_cmd;
    
    if (cmd_p->raid_group_num != FBE_RAID_ID_INVALID){
        /* Set the debug flags for raid groups that we are interested in.
         * Currently is the is the the 3-way non-optimal raid group.
         */
        status = fbe_api_database_lookup_raid_group_by_number(cmd_p->raid_group_num, 
                                                              &cmd_p->rg_object_id);
        if (status != FBE_STATUS_OK)
        {
            fbe_cli_printf("rginfo: ERROR: Raid group number %d does not exist\n", 
                           cmd_p->raid_group_num);
            return;
        }
    }

    /* If there is an object id or we're allowed to pass through an invalid object id, 
     * then go ahead and call the cmd.  
     */
    if ((cmd_p->rg_object_id != FBE_OBJECT_ID_INVALID) ||
        (cmd_p->b_allow_invalid == FBE_TRUE)){
        cmd_p->cmd_fn(cmd_p);
        return;
    }

    fbe_cli_printf("\nEnumerate ALL RAID GROUPS avaiable in the system\n");
    /* First get all members of this class.
     */
    for (class_index = FBE_CLASS_ID_MIRROR;class_index <= FBE_CLASS_ID_PARITY; class_index++)
    {
        nest_status = 
        fbe_api_enumerate_class(class_index, 
                                FBE_PACKAGE_ID_SEP_0, 
                                &obj_list_p, 
                                &num_objects);
        if (nest_status != FBE_STATUS_OK){
            fbe_cli_error("Failed to enumerate class\n");
            return;
        }

        /* Iterate over each of the objects we found.
         */
        for ( index = 0; index < num_objects; index++)
        {
            /* if user only flag was specified, only perform the operation
             * on the user raid groups
             */
            if (!cmd_p->b_user_only || 
                (cmd_p->b_user_only && obj_list_p[index] > FBE_RESERVED_OBJECT_IDS)) 
            {
                current_cmd = *cmd_p;
                current_cmd.rg_object_id = obj_list_p[index];
                current_cmd.cmd_fn(&current_cmd);
            }
        }
        /*To avoid csx NULL pointer soft assert*/
        if (obj_list_p != NULL){
            fbe_api_free_memory(obj_list_p);
        }
    }
}
/******************************************
 * end fbe_cli_rginfo_dispatch_cmd()
 ******************************************/

/*!**************************************************************
 *          fbe_cli_rginfo_get_rg_class_info()
 ****************************************************************
 *
 * @brief   Get raid class information
 *
 * @param   raid_type : raid type for given operation
 * @param   class_id : class ID
 * @param   width : width of RG
 * @param   capacity : capacity for RG
 *
 * @return  None.
 *
 * @author
 *  07/01/2010 - Created. Swati Fursule
 *
 ****************************************************************/
void fbe_cli_rginfo_get_rg_class_info(fbe_raid_group_type_t raid_type,
                                      fbe_class_id_t class_id,
                                      fbe_u32_t width,
                                      fbe_lba_t capacity)
{
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_u16_t                       *data_disks_p = NULL;
    fbe_lba_t                       *capacity_p = NULL;
    fbe_api_raid_group_class_get_info_t get_info;
    get_info.raid_type = raid_type;
    get_info.width = width;
    get_info.b_bandwidth = FBE_FALSE;
    get_info.exported_capacity = capacity;

    status = fbe_api_raid_group_class_get_info(&get_info, class_id);
    if (status != FBE_STATUS_OK)
    {
        fbe_cli_printf("rginfo: ERROR: Not able to get info for class %d\n", class_id);
        return;
    }
    *data_disks_p = get_info.data_disks;
    *capacity_p = get_info.imported_capacity;
     fbe_cli_printf("Getting get_class info to 0x%p for "
                    "class_id: 0x%x == \n", 
                    &get_info, class_id);
     /* !TODO: Print actual information here
      * fbe_cli_printf("== %s == \n", );
      */

    return;
}
/******************************************
 * end fbe_cli_rginfo_get_rg_class_info()
 ******************************************/


/*!**************************************************************
 *          fbe_cli_rginfo_get_rg_object_info()
 ****************************************************************
 *
 * @brief   Get raid group information
 *
 * @param cmd_p - Input command to execute.
 *
 * @return  None.
 *
 * @authoras
 *  07/01/2010 - Created. Swati Fursule
 *
 ****************************************************************/
void fbe_cli_rginfo_get_rg_object_info(fbe_cli_rginfo_cmd_t *cmd_p)
{
    fbe_u32_t raid_group_num = cmd_p->raid_group_num;
    fbe_object_id_t rg_object_id = cmd_p->rg_object_id;
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_api_raid_group_get_info_t   raid_group_info;
    fbe_raid_group_number_t         raid_group_id = FBE_RAID_ID_INVALID;
    fbe_bool_t                      is_system_rg = FBE_FALSE;
    fbe_cli_lurg_rg_details_t       rg_details;
    fbe_bool_t                      speclz_state;

    /*Can't get infor when object is Specialize, coz topology won't send the packet*/
    status = fbe_cli_rginfo_speclz_object(rg_object_id, &speclz_state);
    if ((status != FBE_STATUS_OK) || (speclz_state == FBE_TRUE))
    {
        fbe_cli_printf("rginfo: ERROR: status: 0x%x  b_is_speclz: %d RG object id 0x%x\n", 
                       status, speclz_state, rg_object_id);
        return;
    }
    
    /* Now that we know the RG exists get the information for the RG */
    status = fbe_api_raid_group_get_info(rg_object_id, &raid_group_info, 
                                         FBE_PACKET_FLAG_NO_ATTRIB);
    if (status != FBE_STATUS_OK)
    {
        fbe_cli_printf("rginfo: ERROR: Not able to get the information for RG object id 0x%x\n", 
                       rg_object_id);
        return;
    }

    /* We only want downstream objects for this RG indicate 
    * that by setting corresponding variable in structure*/
    rg_details.direction_to_fetch_object = DIRECTION_BOTH;

    /* Get the details of RG associated with this LUN */
    status = fbe_cli_get_rg_details(rg_object_id,&rg_details);
    if (status != FBE_STATUS_OK)
    {
        fbe_cli_error("Failed to get RG detail associated with this LUN of an Object id:0x%x, Status:0x%x",rg_object_id,status);
        return ;
    }

    /*Set the status to invalid*/
    status = FBE_STATUS_INVALID;	

    /* Get the RG number associated with this RG */
    raid_group_id = FBE_RAID_ID_INVALID;
    status = fbe_api_database_lookup_raid_group_by_object_id(rg_object_id, &raid_group_id);
    
    /*
     * Check whether it is a system Raid group object, if so, print that information
     */
    status = fbe_api_database_is_system_object(rg_object_id, &is_system_rg);
    if (status != FBE_STATUS_OK)
    {
        fbe_cli_printf("rginfo: ERROR: Not able to get the information if RG is system RG %d\n", 
                       raid_group_num);
        return;
    }
    /* Print Current Raid Group Number then print remaining information*/
    if (is_system_rg)
    {
        fbe_cli_printf("\nSYSTEM ");
    }
    else
    {
        fbe_cli_printf("\nUSER ");
    }
    fbe_cli_printf("RAID Group number: %d (0x%x) object_id: %d (0x%x)\n", 
                raid_group_id, raid_group_id,
                rg_object_id, rg_object_id);

    fbe_cli_rginfo_display_rggetinfo(&raid_group_info, &rg_details, cmd_p->b_verbose_display);
    return;
}
/******************************************
 * end fbe_cli_rginfo_get_rg_object_info()
 ******************************************/

/*!**************************************************************
 *          fbe_cli_rginfo_set_rg_debug_flag()
 ****************************************************************
 *
 * @brief   Set debug flags for the raid group
 *
 * @param cmd_p - Input command to execute.
 *
 * @return  None.
 *
 * @author
 *  07/01/2010 - Created. Swati Fursule
 *
 ****************************************************************/
void fbe_cli_rginfo_set_rg_debug_flag(fbe_cli_rginfo_cmd_t *cmd_p)
{
    fbe_status_t                    status = FBE_STATUS_OK;

    if ((cmd_p->rg_object_id == FBE_OBJECT_ID_INVALID) &&
        (cmd_p->raid_group_num == FBE_RAID_ID_INVALID))
    {
        fbe_cli_printf("Setting raid group debug flag at default level\n");
    }
     /* Set the raid group debug flags
      */
     status = fbe_api_raid_group_set_group_debug_flags(cmd_p->rg_object_id, 
                                                       cmd_p->set_rg_debug_flags);
    if (status != FBE_STATUS_OK)
    {
        fbe_cli_printf("rginfo: ERROR: Not able to set debug flags information "
                       "for Raid group %d\n", cmd_p->raid_group_num);
        return;
    }

    if ( ( cmd_p->rg_object_id == FBE_OBJECT_ID_INVALID) && 
         ( cmd_p->raid_group_num == FBE_RAID_ID_INVALID))
    {
        fbe_cli_printf("Set default RAID GROUP DEBUG FLAG to 0x%08x \n",
                        cmd_p->set_rg_debug_flags);
    }
    else
    {
        fbe_cli_printf("Set RAID GROUP DEBUG FLAG to 0x%08x "
                    "for rg_object_id: 0x%x successfully \n", 
                    cmd_p->set_rg_debug_flags, cmd_p->rg_object_id);
    }
    return;
}
/******************************************
 * end fbe_cli_rginfo_set_rg_debug_flag()
 ******************************************/

/*!**************************************************************
 *          fbe_cli_rginfo_set_lib_debug_flags()
 ****************************************************************
 *
 * @brief   Set debug flags for the raid group
 *
 * @param cmd_p - Input command to execute.
 *
 * @return  None.
 *
 * @author
 *  07/01/2010 - Created. Swati Fursule
 *
 ****************************************************************/
void fbe_cli_rginfo_set_lib_debug_flags(fbe_cli_rginfo_cmd_t *cmd_p)
{
    fbe_status_t                    status = FBE_STATUS_OK;

     /* Set the raid library debug flags
      */
     status = fbe_api_raid_group_set_library_debug_flags(cmd_p->rg_object_id, 
                                                         cmd_p->set_lib_debug_flags);
    if (status != FBE_STATUS_OK)
    {
        fbe_cli_printf("rginfo: ERROR: Not able to set library debug flags information"
                       "for Raid group %d\n", cmd_p->raid_group_num);
        return;
    }

    if ( ( cmd_p->rg_object_id == FBE_OBJECT_ID_INVALID) && 
         ( cmd_p->raid_group_num == FBE_RAID_ID_INVALID))
    {
        fbe_cli_printf("Set default RAID LIBRARY DEBUG FLAG to 0x%08x \n",
                        cmd_p->set_lib_debug_flags);
    }
    else
    {
        fbe_cli_printf("Set RAID LIBRARY FLAG to 0x%08x "
                       "for rg_object_id: 0x%x successfully \n", 
                       cmd_p->set_lib_debug_flags, cmd_p->rg_object_id);
    }
    return;
}
/******************************************
 * end fbe_cli_rginfo_set_lib_debug_flags()
 ******************************************/

/*!**************************************************************
 *          fbe_cli_rginfo_get_rg_debug_flag()
 ****************************************************************
 *
 * @brief   Get debug flags for the raid group
 *
 * @param cmd_p - Input command to execute.
 *
 * @return  None.
 *
 * @author
 *  07/01/2010 - Created. Swati Fursule
 *
 ****************************************************************/
void fbe_cli_rginfo_get_rg_debug_flag(fbe_cli_rginfo_cmd_t *cmd_p)
{
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_raid_group_debug_flags_t    raid_group_debug_flags;
    fbe_raid_group_default_debug_flag_payload_t default_debug_flags;

    if ((cmd_p->rg_object_id == FBE_OBJECT_ID_INVALID) &&
        (cmd_p->raid_group_num == FBE_RAID_ID_INVALID))
    {
        fbe_cli_printf("Getting raid group debug flag at default level\n");
        status = fbe_api_raid_group_get_default_debug_flags(FBE_CLASS_ID_PARITY, &default_debug_flags);
        if (status != FBE_STATUS_OK)
        {
            fbe_cli_printf("rginfo: error getting default debug flags %d\n", status);
            return;
        }
        fbe_cli_printf("default user debug flags:   0x%x\n", default_debug_flags.user_debug_flags);
        fbe_cli_printf("default system debug flags: 0x%x\n", default_debug_flags.system_debug_flags);
    }

    /* Get the current value of the raid group debug flags.
     */
    status = fbe_api_raid_group_get_group_debug_flags(cmd_p->rg_object_id,
                                                      &raid_group_debug_flags);
    if (status != FBE_STATUS_OK)
    {
        fbe_cli_printf("rginfo: ERROR: Not able to get debug flags information"
                       "for Raid group %d\n", cmd_p->raid_group_num);
        return;
    }
    fbe_cli_printf("\n");
    fbe_cli_rginfo_display_rg_debug_flag_info(raid_group_debug_flags);
    return;
}
/******************************************
 * end fbe_cli_rginfo_get_rg_debug_flag()
 ******************************************/

/*!**************************************************************
 *          fbe_cli_rginfo_get_lib_debug_flags()
 ****************************************************************
 *
 * @brief   Get debug flags for the raid group
 *
 * @param cmd_p - Input command to execute.
 *
 * @return  None.
 *
 * @author
 *  07/01/2010 - Created. Swati Fursule
 *
 ****************************************************************/
void fbe_cli_rginfo_get_lib_debug_flags(fbe_cli_rginfo_cmd_t *cmd_p)
{
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_raid_library_debug_flags_t  raid_lib_debug_flags;
    fbe_raid_group_default_library_flag_payload_t default_debug_flags;

     
    if ((cmd_p->rg_object_id == FBE_OBJECT_ID_INVALID) &&
        (cmd_p->raid_group_num == FBE_RAID_ID_INVALID))
    {
        fbe_cli_printf("Getting library debug flag at default level\n");

        status = fbe_api_raid_group_get_default_library_flags(FBE_CLASS_ID_PARITY, &default_debug_flags);
        if (status != FBE_STATUS_OK)
        {
            fbe_cli_printf("rginfo: error getting default debug flags %d\n", status);
            return;
        }
        fbe_cli_printf("default user library flags:   0x%x\n", default_debug_flags.user_debug_flags);
        fbe_cli_printf("default system library flags: 0x%x\n", default_debug_flags.system_debug_flags);
    }

    /* Get the current value of the raid library debug flags.
     */
    status = fbe_api_raid_group_get_library_debug_flags(cmd_p->rg_object_id, &raid_lib_debug_flags);
    if (status != FBE_STATUS_OK)
    {
        fbe_cli_printf("rginfo: ERROR: Not able to get library debug flags information"
                      "for Raid group %d\n", cmd_p->raid_group_num);
        return;
    }
    fbe_cli_printf("\n");
    fbe_cli_rginfo_display_lib_debug_flag_info(raid_lib_debug_flags);
    return;
}
/******************************************
 * end fbe_cli_rginfo_get_lib_debug_flags()
 ******************************************/


/*!**************************************************************
 *          fbe_cli_rginfo_set_rg_tracelevel()
 ****************************************************************
 *
 * @brief   Set trace level for the raid group
 *
 * @param cmd_p - Input command to execute.
 *
 * @return  None.
 *
 * @author
 *  07/01/2010 - Created. Swati Fursule
 *
 ****************************************************************/
void fbe_cli_rginfo_set_rg_tracelevel(fbe_cli_rginfo_cmd_t *cmd_p)
{
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_api_trace_level_control_t   level_control;
    
    if ((cmd_p->rg_object_id == FBE_OBJECT_ID_INVALID) &&
        (cmd_p->raid_group_num == FBE_RAID_ID_INVALID))
    {
        fbe_cli_printf("Setting trace level at default level\n");
        level_control.trace_type = FBE_TRACE_TYPE_DEFAULT;
    }
    else
    {
        level_control.trace_type = FBE_TRACE_TYPE_OBJECT;
    }
    level_control.fbe_id = cmd_p->rg_object_id;
    level_control.trace_level = cmd_p->set_trace_levels;
    status = fbe_api_trace_set_level(&level_control, FBE_PACKAGE_ID_SEP_0);
    if (status != FBE_STATUS_OK)
    {
        fbe_cli_printf("rginfo: ERROR: Not able to set trace level for the Raid group"
                       " of number %d\n", cmd_p->raid_group_num);
        return;
    }

     fbe_cli_printf("%s: Set trace levels to  %d\n", 
                __FUNCTION__, level_control.trace_level);
    return;
}

/*!**************************************************************
 *          fbe_cli_rginfo_get_rg_tracelevel()
 ****************************************************************
 *
 * @brief   Get trace level for the raid group
 *
 * @param cmd_p - Input command to execute.
 *
 * @return  None.
 *
 * @author
 *  07/01/2010 - Created. Swati Fursule
 *
 ****************************************************************/
void fbe_cli_rginfo_get_rg_tracelevel(fbe_cli_rginfo_cmd_t *cmd_p)
{
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_api_trace_level_control_t   level_control;
    
    if (cmd_p->rg_object_id == FBE_OBJECT_ID_INVALID)
    {
        fbe_cli_printf("Getting trace level at default level\n");
    }
    
     /* Get the state trace level for this object (if enabled)
      */
    level_control.trace_type = FBE_TRACE_TYPE_DEFAULT;
    level_control.fbe_id = cmd_p->rg_object_id;
    level_control.trace_level = FBE_TRACE_LEVEL_INVALID;
     status = fbe_api_trace_get_level(&level_control, FBE_PACKAGE_ID_SEP_0);
    if (status != FBE_STATUS_OK)
    {
        fbe_cli_printf("rginfo: ERROR: Not able to get trace level for the Raid group "
                       "of number %d\n", cmd_p->rg_object_id);
        return;
    }
    fbe_cli_printf("\n");
    fbe_cli_rginfo_display_tracelevelinfo(level_control);
    return;
}

/******************************************
 * end fbe_cli_rginfo_get_rg_tracelevel()
 ******************************************/
/*!**************************************************************
 * fbe_cli_rginfo_display_verify_rate()
 ****************************************************************
 * @brief
 *  Display the rate of verify.
 *
 * @param  rg_info - raid group information for rg to get rate on.               
 *
 * @return -   
 *
 * @author
 *  4/29/2013 - Created. Rob Foley
 *
 ****************************************************************/
void fbe_cli_rginfo_display_verify_rate(fbe_api_raid_group_get_info_t *rg_info)
{
    fbe_char_t *vr_type = "ukn";
    fbe_lba_t max_vr_checkpoint = 0;

    if ((rg_info->incomplete_write_verify_checkpoint != FBE_LBA_INVALID) &&
        (rg_info->incomplete_write_verify_checkpoint > max_vr_checkpoint))
    {
        max_vr_checkpoint = rg_info->incomplete_write_verify_checkpoint;
        vr_type = "iw";
    }
    else if ((rg_info->error_verify_checkpoint != FBE_LBA_INVALID) &&
             (rg_info->error_verify_checkpoint > max_vr_checkpoint))
    {
        max_vr_checkpoint = rg_info->error_verify_checkpoint;
        vr_type = "error";
    }
    else if ((rg_info->system_verify_checkpoint != FBE_LBA_INVALID) &&
             (rg_info->system_verify_checkpoint > max_vr_checkpoint))
    {
        max_vr_checkpoint = rg_info->system_verify_checkpoint;
        vr_type = "system";
    }
    else if ((rg_info->rw_verify_checkpoint != FBE_LBA_INVALID) &&
             (rg_info->rw_verify_checkpoint > max_vr_checkpoint))
    {
        max_vr_checkpoint = rg_info->rw_verify_checkpoint;
        vr_type = "rw";
    }
    else if ((rg_info->ro_verify_checkpoint != FBE_LBA_INVALID) &&
             (rg_info->ro_verify_checkpoint > max_vr_checkpoint))
    {
        max_vr_checkpoint = rg_info->ro_verify_checkpoint;
        vr_type = "ro";
    }
    if ((max_vr_checkpoint > 0) && (rg_info->background_op_seconds > 0))
    {
        fbe_u32_t remaining_mb;
        fbe_u32_t remaining_seconds;
        fbe_u32_t remaining_minutes;
        fbe_u32_t remaining_hours;
        fbe_u32_t total_mb = (fbe_u32_t)(max_vr_checkpoint / 0x800);
        double rate;
        double percentage;
        rate = (double)total_mb / (double)rg_info->background_op_seconds;
        percentage = (double)(100 * max_vr_checkpoint) /(double)rg_info->imported_blocks_per_disk;

        remaining_mb = (fbe_u32_t)((rg_info->imported_blocks_per_disk - max_vr_checkpoint) / 0x800);
        if (rate == 0)
        {
            remaining_seconds = remaining_mb;
        }
        else
        {
            remaining_seconds = remaining_mb / (fbe_u32_t)rate;
        }
        remaining_hours = remaining_seconds / 3600;
        remaining_seconds -= (3600 * remaining_hours);
        remaining_minutes = remaining_seconds / 60;
        remaining_seconds -= (60 * remaining_minutes);

        fbe_cli_printf("%s verify rate : %2.2f (%d mb in %d seconds) Percent: %2.5f Time Remaining: %0u:%02u:%02u\n",
                       vr_type, rate, total_mb, rg_info->background_op_seconds,
                       percentage,
                       remaining_hours, remaining_minutes, remaining_seconds);
    }
}
/**************************************
 * end fbe_cli_rginfo_display_verify_rate
 **************************************/

/*!**************************************************************
 * fbe_cli_rginfo_display_rebuild_rate()
 ****************************************************************
 * @brief
 *  Display the rate of verify.
 *
 * @param  rg_info - raid group information for rg to get rate on.               
 *
 * @return -   
 *
 * @author
 *  4/29/2013 - Created. Rob Foley
 *
 ****************************************************************/
void fbe_cli_rginfo_display_rebuild_rate(fbe_api_raid_group_get_info_t *rg_info)
{
    fbe_u32_t index;
    fbe_u32_t rg_width = rg_info->width;
    fbe_lba_t max_rb_checkpoint = 0;
    for (index = 0; index < rg_width; index++)
    {
        if ((rg_info->rebuild_checkpoint[index] != FBE_LBA_INVALID) &&
            (rg_info->rebuild_checkpoint[index] > max_rb_checkpoint))
        {
            max_rb_checkpoint = rg_info->rebuild_checkpoint[index];
        }
    }
    if (max_rb_checkpoint > 0 && rg_info->background_op_seconds > 0)
    {
        fbe_u32_t remaining_mb;
        fbe_u32_t remaining_seconds;
        fbe_u32_t remaining_minutes;
        fbe_u32_t remaining_hours;
        double rate;
        double percentage;
        fbe_u32_t total_mb = (fbe_u32_t)(max_rb_checkpoint / 0x800);
        rate = (double)total_mb / (double)rg_info->background_op_seconds;
        percentage = (double)(100 * max_rb_checkpoint) /(double)rg_info->imported_blocks_per_disk;
        remaining_mb = (fbe_u32_t)((rg_info->imported_blocks_per_disk - max_rb_checkpoint) / 0x800);
        if (rate == 0)
        {
            remaining_seconds = remaining_mb;
        }
        else
        {
            remaining_seconds = remaining_mb / (fbe_u32_t)rate;
        }
        remaining_hours = remaining_seconds / 3600;
        remaining_seconds -= (3600 * remaining_hours);
        remaining_minutes = remaining_seconds / 60;
        remaining_seconds -= (60 * remaining_minutes);

        fbe_cli_printf("rb_rate: %2.2f (%d mb in %d seconds) Percent: %2.5f Time Remaining: %0u:%02u:%02u\n",
                       rate, total_mb, rg_info->background_op_seconds, percentage,
                       remaining_hours, remaining_minutes, remaining_seconds);
    }
}
/**************************************
 * end fbe_cli_rginfo_display_rebuild_rate
 **************************************/

/*!**************************************************************
 * fbe_cli_rginfo_display_rekey_rate()
 ****************************************************************
 * @brief
 *  Display the rate of rekey.
 *
 * @param  rg_info - raid group information for rg to get rate on.               
 *
 * @return -   
 *
 * @author
 *  12/5/2013 - Created. Rob Foley
 *
 ****************************************************************/
void fbe_cli_rginfo_display_rekey_rate(fbe_api_raid_group_get_info_t *rg_info)
{
    fbe_lba_t rekey_checkpoint = rg_info->rekey_checkpoint;
    if ((rekey_checkpoint != FBE_LBA_INVALID) &&
        (rekey_checkpoint > 0) && (rg_info->background_op_seconds > 0))
    {
        fbe_u32_t remaining_mb;
        fbe_u32_t remaining_seconds;
        fbe_u32_t remaining_minutes;
        fbe_u32_t remaining_hours;
        fbe_u32_t total_mb = (fbe_u32_t)(rekey_checkpoint / 0x800);
        double rate;
        double percentage;
        rate = (double)total_mb / (double)rg_info->background_op_seconds;
        percentage = (double)(100 * rekey_checkpoint) /(double)rg_info->imported_blocks_per_disk;

        remaining_mb = (fbe_u32_t)((rg_info->imported_blocks_per_disk - rekey_checkpoint) / 0x800);
        if (rate == 0)
        {
            remaining_seconds = remaining_mb;
        }
        else
        {
            remaining_seconds = remaining_mb / (fbe_u32_t)rate;
        }
        remaining_hours = remaining_seconds / 3600;
        remaining_seconds -= (3600 * remaining_hours);
        remaining_minutes = remaining_seconds / 60;
        remaining_seconds -= (60 * remaining_minutes);

        fbe_cli_printf("rekey rate : %2.2f (%d mb in %d seconds) Percent: %2.5f Time Remaining: %0u:%02u:%02u\n",
                       rate, total_mb, rg_info->background_op_seconds,
                       percentage,
                       remaining_hours, remaining_minutes, remaining_seconds);
    }
}
/**************************************
 * end fbe_cli_rginfo_display_rekey_rate
 **************************************/
/*!**************************************************************
 * fbe_cli_display_rg_background_rate()
 ****************************************************************
 * @brief
 *  Display basic info about our rate.
 *
 * @param  rg_info - raid group information for rg to get rate on.               
 *
 * @return -   
 *
 * @author
 *  4/29/2013 - Created. Rob Foley
 *
 ****************************************************************/

void fbe_cli_display_rg_background_rate(fbe_api_raid_group_get_info_t *rg_info)
{
    if (rg_info->background_op_seconds)
    {
        fbe_cli_rginfo_display_rebuild_rate(rg_info);
        fbe_cli_rginfo_display_verify_rate(rg_info);
        fbe_cli_rginfo_display_rekey_rate(rg_info);
    }
}
/******************************************
 * end fbe_cli_display_rg_background_rate()
 ******************************************/
/*!**************************************************************
 *          fbe_cli_rginfo_display_rggetinfo()
 ****************************************************************
 *
 * @brief   Get trace level for the raid group
 *
 * @param   getrg_info : get raid group info structure
 * @param rg_details:
 * @param b_verbose : TRUE for more info in display.
 *
 * @return  None.
 *
 * @author
 *  07/01/2010 - Created. Swati Fursule
 *  06/08/2011 - Modified Sandeep Chaudhari
 ****************************************************************/
void fbe_cli_rginfo_display_rggetinfo(
                              fbe_api_raid_group_get_info_t *getrg_info,
                              fbe_cli_lurg_rg_details_t *rg_details,
                              fbe_bool_t b_verbose)
{
    fbe_status_t status;
    fbe_u32_t index;
    fbe_u32_t rg_width;
    fbe_lun_number_t      lun_number;
    fbe_cli_printf("Raid group Information :\n");

    rg_width = getrg_info->physical_width;

    fbe_cli_printf("Number of drives in the raid group: %d\n", rg_width);
    fbe_cli_printf("Drives associated with the raid group:");
    for (index = 0; index < rg_width; index++)
    {
        fbe_cli_printf(" %d_%d_%d",rg_details->drive_list[index].port_num,
            rg_details->drive_list[index].encl_num,
            rg_details->drive_list[index].slot_num);
    }
    fbe_cli_printf("\n");

    fbe_cli_rginfo_display_raid_type(getrg_info->raid_type);
    fbe_cli_printf("\nuser Capacity:            0x%llx\n", (unsigned long long)getrg_info->capacity);
    fbe_cli_printf(  "user + metadata capacity: 0x%llx\n", (unsigned long long)getrg_info->raid_capacity);
    fbe_cli_printf(  "metadata start lba:       0x%llx\n", (unsigned long long)getrg_info->paged_metadata_start_lba);
    fbe_cli_printf(  "metadata capacity:        0x%llx\n", (unsigned long long)getrg_info->paged_metadata_capacity);
    fbe_cli_printf(  "total metadata chunks:    0x%x\n", getrg_info->total_chunks);
    fbe_cli_printf(  "journal start pba:        0x%llx\n", (unsigned long long)getrg_info->write_log_start_pba);
    fbe_cli_printf(  "journal capacity:         0x%llx\n", (unsigned long long)getrg_info->write_log_physical_capacity);
    fbe_cli_printf(  "physical offset:          0x%llx\n", (unsigned long long)getrg_info->physical_offset);
    fbe_cli_printf(  "Element Size:             0x%x\n", getrg_info->element_size);
    fbe_cli_printf(  "Exported block size:      0x%x\n", getrg_info->exported_block_size);
    fbe_cli_printf(  "Imported block size:      0x%x\n", getrg_info->imported_block_size);
    fbe_cli_printf(  "Optimal block size:       0x%x\n", getrg_info->optimal_block_size);
    fbe_cli_printf(  "Stripe count:             %d\n", (int)getrg_info->stripe_count);
    fbe_cli_printf(  "Sectors per stripe:       0x%x\n", (unsigned int)getrg_info->sectors_per_stripe);    
    fbe_cli_printf(  "Lun Align size:           0x%x\n", (unsigned int)getrg_info->lun_align_size);
    fbe_cli_printf(  "Elements Per Parity Stripe: 0x%x\n", getrg_info->elements_per_parity_stripe);

    fbe_cli_printf("\nUser Private Flag: %s\n",(getrg_info->user_private==FBE_TRUE)?"TRUE":"FALSE");
    fbe_cli_rginfo_display_rg_debug_flag_info(getrg_info->debug_flags);
    fbe_cli_printf("Flags related to I/O processing: 0x%x\n", 
        getrg_info->flags);
    fbe_cli_rginfo_display_raid_group_flag(getrg_info->flags);
    fbe_cli_rginfo_display_lib_debug_flag_info(getrg_info->library_debug_flags);

    /* Note: The rginfo displays the information for the object specified. The object 
     * specified in case of RAID10 is striper object, for striper object number of edges
     * are equal to half of the number of disks used to create rg, as specified by the 
     * width. Therefore we are displaying only this 'width' number of array 
     * entries for NR on Demand and Rebuild Checkpoint.
     */
    fbe_cli_printf("\n");
    for (index = 0; index < rg_width; index++)
    {
        fbe_cli_printf("Information for disk at index %d:\n", index);
        fbe_cli_printf("\trb logging[%d].: %s\n", 
            index, getrg_info->b_rb_logging[index]?"TRUE": "FALSE");
        fbe_cli_printf("\tRebuild checkpoint[%d].: 0x%llx\n", 
            index, (unsigned long long)getrg_info->rebuild_checkpoint[index]);
    }
    fbe_cli_printf("\nread only verify checkpoint: 0x%llx\n", (unsigned long long)getrg_info->ro_verify_checkpoint);
    fbe_cli_printf(  "user verify checkpoint:      0x%llx\n", (unsigned long long)getrg_info->rw_verify_checkpoint);
    fbe_cli_printf(  "error verify checkpoint:     0x%llx\n", (unsigned long long)getrg_info->error_verify_checkpoint);
    fbe_cli_printf(  "system verify checkpoint:    0x%llx\n", (unsigned long long)getrg_info->system_verify_checkpoint);
    fbe_cli_printf(  "incomplete write vr chkpt:   0x%llx\n", (unsigned long long)getrg_info->incomplete_write_verify_checkpoint);
    fbe_cli_printf(  "journal verify checkpoint:   0x%llx\n", (unsigned long long)getrg_info->journal_verify_checkpoint);
    fbe_cli_printf(  "rekey checkpoint:            0x%llx\n", (unsigned long long)getrg_info->rekey_checkpoint);

    if (b_verbose)
    {
        fbe_cli_display_rg_background_rate(getrg_info);
    }

    fbe_cli_printf("NP_metadata_flags: 0x%x\n", getrg_info->raid_group_np_md_flags);
    fbe_cli_printf("NP_metadata_extended_flags: 0x%llx\n", getrg_info->raid_group_np_md_extended_flags);
    fbe_cli_printf("Metadata element state on this SP: %s\n",
                   fbe_cli_convert_metadata_element_state_to_string(getrg_info->metadata_element_state));
    
    fbe_cli_printf("Lun(s) in this RG:");

    for (index =0; index<rg_details->upstream_object_list.number_of_upstream_objects;index++)
    {
        status = fbe_api_database_lookup_lun_by_object_id(rg_details->upstream_object_list.upstream_object_list[index],&lun_number);
        if (status != FBE_STATUS_OK)
        {
            fbe_cli_error("Failed to get lun number for lun Object id:0x%x, Status:0x%x",rg_details->upstream_object_list.upstream_object_list[index],status);
            return ;
        }
        fbe_cli_printf(" %d",lun_number);
    }
    fbe_cli_printf("\n");
}
/******************************************
 * end fbe_cli_rginfo_display_rggetinfo()
 ******************************************/

/*!**************************************************************
 *          fbe_cli_rginfo_display_raid_type()
 ****************************************************************
 *
 * @brief   Display raid group debug flag information
 *
 * @param   rg_debug_flag_info : library debug flag info 
 *
 * @return  None.
 *
 * @author
 *  07/01/2010 - Created. Swati Fursule
 *
 ****************************************************************/
void fbe_cli_rginfo_display_raid_type(fbe_raid_group_type_t raid_type)
{
    fbe_u8_t *raid_type_str = NULL;
    //fbe_cli_printf("Raid Type: %d\n", raid_type);
    switch(raid_type)
    {
    case FBE_RAID_GROUP_TYPE_UNKNOWN:
        raid_type_str = "FBE_RAID_GROUP_TYPE_UNKNOWN";
        break;
    case FBE_RAID_GROUP_TYPE_RAID1:
        raid_type_str = "FBE_RAID_GROUP_TYPE_RAID1";
        break;
    case FBE_RAID_GROUP_TYPE_RAW_MIRROR:
        raid_type_str = "FBE_RAID_GROUP_TYPE_RAW_MIRROR";
        break;
    case FBE_RAID_GROUP_TYPE_RAID10:
        raid_type_str = "FBE_RAID_GROUP_TYPE_RAID10";
        break;
    case FBE_RAID_GROUP_TYPE_RAID3:
        raid_type_str = "FBE_RAID_GROUP_TYPE_RAID3";
        break;
    case FBE_RAID_GROUP_TYPE_RAID0:
        raid_type_str = "FBE_RAID_GROUP_TYPE_RAID0";
        break;
    case FBE_RAID_GROUP_TYPE_RAID5:
        raid_type_str = "FBE_RAID_GROUP_TYPE_RAID5";
        break;
    case FBE_RAID_GROUP_TYPE_RAID6:
        raid_type_str = "FBE_RAID_GROUP_TYPE_RAID6";
        break;
    case FBE_RAID_GROUP_TYPE_SPARE:
        raid_type_str = "FBE_RAID_GROUP_TYPE_SPARE";
        break;
    case FBE_RAID_GROUP_TYPE_INTERNAL_MIRROR_UNDER_STRIPER:
        raid_type_str = 
            "FBE_RAID_GROUP_TYPE_INTERNAL_MIRROR_UNDER_STRIPER";
        break;
    case FBE_RAID_GROUP_TYPE_INTERNAL_METADATA_MIRROR:
        raid_type_str 
            = "FBE_RAID_GROUP_TYPE_INTERNAL_METADATA_MIRROR";
        break;
    default:
        break;
    }
    fbe_cli_printf("Raid Type: %s\n", raid_type_str);
    return;
}
/******************************************
 * end fbe_cli_rginfo_display_raid_type()
 ******************************************/

/*!*******************************************************************
 * @var fbe_cli_rginfo_rg_debug_flag_strings
 *********************************************************************
 * @brief We use this to do a lookup on the string that represents
 *        this flag.
 *
 *********************************************************************/
const fbe_u8_t *fbe_cli_rginfo_rg_debug_flag_strings[] = {FBE_RAID_GROUP_DEBUG_FLAG_STRINGS};

/*!**************************************************************
 *          fbe_cli_rginfo_display_lib_debug_flag_info()
 ****************************************************************
 *
 * @brief   Display raid group debug flag information
 *
 * @param   rg_debug_flags - The current value of the raid group
 *              debug flags
 *
 * @return  None.
 *
 * @author
 *  07/01/2010 - Created. Swati Fursule
 *
 ****************************************************************/
void fbe_cli_rginfo_display_rg_debug_flag_info(fbe_raid_group_debug_flags_t rg_debug_flags)
{
    fbe_u32_t       shiftBit, index;
    const fbe_u8_t *rg_debug_flag_str = NULL;
    fbe_u32_t       shift_bit_num = 0;
    fbe_u32_t       valid_mask = (fbe_u32_t)FBE_RAID_GROUP_DEBUG_FLAG_VALID_MASK;
    
    fbe_cli_printf("Raid Group Debug Flag: 0x%x\n", rg_debug_flags);
    if (rg_debug_flags == 0)
    {
        fbe_cli_printf("\t %s \n", "FBE_RAID_GROUP_DEBUG_FLAG_NONE");
        return;
    }
    
     while(valid_mask > 1)
     {
         valid_mask /= 2;
         shift_bit_num++;
     }
        
     for (index = 0; index < shift_bit_num; index++)
     {
          shiftBit = (1 <<  index);
          if (rg_debug_flags & shiftBit)
          {
              rg_debug_flag_str = fbe_cli_rginfo_rg_debug_flag_strings[index];
              fbe_cli_printf("\t %s \n", rg_debug_flag_str);
          }   // end of if
      }   // end of for loop

    return;
}
/******************************************
 * end fbe_cli_rginfo_display_lib_debug_flag_info()
 ******************************************/

/*!*******************************************************************
 * @var fbe_cli_rginfo_rg_flag_strings
 *********************************************************************
 * @brief We use this to do a lookup on the string that represents
 *        this flag.
 *
 *********************************************************************/
const fbe_u8_t *fbe_cli_rginfo_rg_flag_strings[] = {FBE_RAID_GROUP_FLAG_STRINGS};

/*!**************************************************************
 *          fbe_cli_rginfo_display_raid_group_flag()
 ****************************************************************
 *
 * @brief   Display raid group debug flag information
 *
 * @param   rg_debug_flag_info : library debug flag info 
 *
 * @return  None.
 *
 * @author
 *  07/01/2010 - Created. Swati Fursule
 *
 ****************************************************************/
void fbe_cli_rginfo_display_raid_group_flag(fbe_raid_group_flags_t raid_group_flags)
{
    fbe_u32_t flag = raid_group_flags;
    fbe_u32_t shiftBit, index;
    const fbe_u8_t *rg_flag_str = NULL;
    fbe_u32_t shift_bit_num = 0;
    fbe_s32_t fbe_raid_group_flag_data = (fbe_s32_t)FBE_RAID_GROUP_FLAG_LAST;
    
    fbe_cli_printf("Raid Group Flag: 0x%x\n", flag);
    if(flag == 0)
    {
        fbe_cli_printf("\t %s ", "FBE_RAID_GROUP_FLAG_INVALID");
    }
    

     while(fbe_raid_group_flag_data > 1)
     {
        fbe_raid_group_flag_data /= 2;
        shift_bit_num++;
     }

     if(FBE_RAID_GROUP_FLAG_LAST != pow(2,shift_bit_num))
     {
         return;
     }

     for (index = 0; index < shift_bit_num; index++)
     {
          shiftBit = (1 <<  index);
          if (flag & shiftBit)
          {
              rg_flag_str = fbe_cli_rginfo_rg_flag_strings[index];
              fbe_cli_printf("\t %s \n", rg_flag_str);
          }   // end of if
      }   // end of for loop
             /*fbe_cli_printf("\n");*/
    return;
}
/******************************************
 * end fbe_cli_rginfo_display_raid_group_flag()
 ******************************************/

/*!*******************************************************************
 * @var fbe_cli_rginfo_raid_lib_debug_flag_strings
 *********************************************************************
 * @brief We use this to do a lookup on the string that represents
 *        this flag.
 *
 *********************************************************************/
const fbe_u8_t *fbe_cli_rginfo_raid_lib_debug_flag_strings[] = {FBE_RAID_LIBRARY_DEBUG_FLAG_STRINGS};

/*!**************************************************************
 *          fbe_cli_rginfo_display_lib_debug_flag_info()
 ****************************************************************
 *
 * @brief   Display library degug flag information
 *
 * @param   lib_debug_flag_info : library debug flag info 
 *
 * @return  None.
 *
 * @author
 *  07/01/2010 - Created. Swati Fursule
 *
 ****************************************************************/
void fbe_cli_rginfo_display_lib_debug_flag_info(fbe_raid_library_debug_flags_t lib_debug_flag_info)
{
    fbe_u32_t flag = lib_debug_flag_info;
    fbe_u32_t shiftBit, index;
    fbe_u32_t shift_bit_num = 0;
    fbe_s32_t fbe_raid_library_debug_flag_data = (fbe_s32_t)FBE_RAID_LIBRARY_DEBUG_FLAG_LAST;

    const fbe_u8_t *lib_debug_flag_str = NULL;
    fbe_cli_printf("Library Debug Flag: 0x%x\n", lib_debug_flag_info);
    if(flag == 0)
    {
        fbe_cli_printf("\t %s ", "FBE_RAID_LIBRARY_DEBUG_FLAG_NONE");
    }
       
    while(fbe_raid_library_debug_flag_data > 1)
    {
        fbe_raid_library_debug_flag_data /= 2;
        shift_bit_num++;
    }
    
    if(FBE_RAID_LIBRARY_DEBUG_FLAG_LAST != pow(2,shift_bit_num))
    {
         return;
    }
    
    for (index = 0; index < shift_bit_num; index++)
    {
          shiftBit = (1 <<  index);
          if (flag & shiftBit)
          {
                lib_debug_flag_str = fbe_cli_rginfo_raid_lib_debug_flag_strings[index];
                fbe_cli_printf("\t %s \n", lib_debug_flag_str);
          }   // end of if
     }   // end of for loop
     fbe_cli_printf("\n");

     return;
}
/******************************************
 * end fbe_cli_rginfo_display_lib_debug_flag_info()
 ******************************************/

/*!**************************************************************
 *          fbe_cli_rginfo_display_tracelevelinfo()
 ****************************************************************
 *
 * @brief   Get trace level for the raid group
 *
 * @param   tracelevel_info : get trace level info structure
 *
 * @return  None.
 *
 * @author
 *  07/01/2010 - Created. Swati Fursule
 *
 ****************************************************************/
void fbe_cli_rginfo_display_tracelevelinfo(
                   fbe_api_trace_level_control_t tracelevel_info)
{
    fbe_u8_t *trace_level_type_str = NULL;
    fbe_u8_t *trace_level_str = NULL;
    //fbe_u8_t *disp_data;
    //fbe_cli_printf("Trace Level Type: %d\n", tracelevel_info.trace_type);
    switch(tracelevel_info.trace_type)
    {
    case FBE_TRACE_TYPE_INVALID:
        trace_level_type_str = "FBE_TRACE_TYPE_INVALID";
        break;
    case FBE_TRACE_TYPE_DEFAULT:
        trace_level_type_str = "FBE_TRACE_TYPE_DEFAULT";
        break;
    case FBE_TRACE_TYPE_OBJECT:
        trace_level_type_str = "FBE_TRACE_TYPE_OBJECT";
        break;
    case FBE_TRACE_TYPE_SERVICE:
        trace_level_type_str = "FBE_TRACE_TYPE_SERVICE";
        break;
    case FBE_TRACE_TYPE_LIB:
        trace_level_type_str = "FBE_TRACE_TYPE_LIB";
        break;
    default:
        break;
    }
    //fbe_cli_printf("Trace Level : %d\n", tracelevel_info.trace_level);
    switch(tracelevel_info.trace_level)
    {
    case FBE_TRACE_LEVEL_INVALID:
        trace_level_str = "FBE_TRACE_LEVEL_INVALID";
        break;
    case FBE_TRACE_LEVEL_CRITICAL_ERROR:
        trace_level_str = "FBE_TRACE_LEVEL_CRITICAL_ERROR";
        break;
    case FBE_TRACE_LEVEL_ERROR:
        trace_level_str = "FBE_TRACE_LEVEL_ERROR";
        break;
    case FBE_TRACE_LEVEL_WARNING:
        trace_level_str = "FBE_TRACE_LEVEL_WARNING";
        break;
    case FBE_TRACE_LEVEL_INFO:
        trace_level_str = "FBE_TRACE_LEVEL_INFO";
        break;
    case FBE_TRACE_LEVEL_DEBUG_LOW:
        trace_level_str = "FBE_TRACE_LEVEL_DEBUG_LOW";
        break;
    case FBE_TRACE_LEVEL_DEBUG_MEDIUM:
        trace_level_str = "FBE_TRACE_LEVEL_DEBUG_MEDIUM";
        break;
    case FBE_TRACE_LEVEL_DEBUG_HIGH:
        trace_level_str = "FBE_TRACE_LEVEL_DEBUG_HIGH";
        break;
    default:
        break;
    }
    fbe_cli_printf("Trace level Information :\n");
    fbe_cli_printf("Trace Level Type: %s\n", trace_level_type_str);
    fbe_cli_printf("Trace Level ID.: %d\n", tracelevel_info.fbe_id);
    fbe_cli_printf("Trace Level : %s\n", trace_level_str);
    fbe_cli_printf("Trace Level Flags.: %d\n", tracelevel_info.trace_flag);
    return;
}
/******************************************
 * end fbe_cli_rginfo_display_tracelevelinfo()
 ******************************************/

/*!**************************************************************
 *          fbe_cli_rginfo_get_class_id()
 ****************************************************************
 *
 * @brief   Get trace level for the raid group
 *
 * @param   tracelevel_info : get trace level info structure
 *
 * @return  None.
 *
 * @author
 *  07/01/2010 - Created. Swati Fursule
 *
 ****************************************************************/
void fbe_cli_rginfo_get_class_id(
                              fbe_raid_group_type_t raid_type)
{
    fbe_class_id_t class_id = FBE_CLASS_ID_PARITY;

    /* Set the correct class id */
    switch (raid_type)
    {
        case FBE_RAID_GROUP_TYPE_RAW_MIRROR:
        case FBE_RAID_GROUP_TYPE_RAID1:
            class_id = FBE_CLASS_ID_MIRROR;
            break;

        case FBE_RAID_GROUP_TYPE_RAID0:
        case FBE_RAID_GROUP_TYPE_RAID10:
            class_id = FBE_CLASS_ID_STRIPER;
            break;

        default:
            class_id = FBE_CLASS_ID_PARITY;
            break;
    }
}
/******************************************
 * end fbe_cli_rginfo_get_class_id()
 ******************************************/


/*!**************************************************************
 *          fbe_cli_rginfo_display_raid_memory_stats()
 ****************************************************************
 *
 * @brief   Display raid memory statistics
 *
 * @param   raid_memory_stats_info : raid memory statistics info
 *
 * @return  None.
 *
 * @author
 *  07/01/2010 - Created. Swati Fursule
 *
 ****************************************************************/
void fbe_cli_rginfo_display_raid_memory_stats(
                   fbe_api_raid_memory_stats_t raid_memory_stats_info)
{
    fbe_cli_printf("RAID MEMORY STATISTICS :\n");
    fbe_cli_printf("Number of allocations: %lld\n",
		   (long long)raid_memory_stats_info.allocations);
    fbe_cli_printf("Number of frees: %lld\n",
		   (long long)raid_memory_stats_info.frees);
    fbe_cli_printf("Number of allocated_bytes: %lld\n",
		   (long long)raid_memory_stats_info.allocated_bytes);
    fbe_cli_printf("Number of freed_bytes: %lld\n",
		   (long long)raid_memory_stats_info.freed_bytes);
    fbe_cli_printf("Number of deferred_allocations: %lld\n",
		   (long long)raid_memory_stats_info.deferred_allocations);
    fbe_cli_printf("Number of pending_allocations: %lld\n",
		   (long long)raid_memory_stats_info.pending_allocations);
    fbe_cli_printf("Number of aborted_allocations: %lld\n",
		   (long long)raid_memory_stats_info.aborted_allocations);
    fbe_cli_printf("Number of allocation_errors: %lld\n",
		   (long long)raid_memory_stats_info.allocation_errors);
    fbe_cli_printf("Number of free_errors: %lld\n",
		   (long long)raid_memory_stats_info.free_errors);
    fbe_cli_printf("\n");
    return;
}
/******************************************
 * end fbe_cli_rginfo_display_raid_memory_stats()
 ******************************************/

/*!**************************************************************
 *          fbe_cli_rginfo_speclz_object()
 ****************************************************************
 *
 * @brief   Check whether the object is SPECIALIZE or not
 *
 * @param   object_id : input object id
 * @param   speclz_state : output for know if it is SPECIALZE or not
 *
 * @return  None.
 *
 * @author
 *  03/31/2012 - Created. He Wei
 *
 ****************************************************************/
static fbe_status_t fbe_cli_rginfo_speclz_object(fbe_object_id_t object_id,
                                                         fbe_bool_t *speclz_state)
{
    fbe_status_t lifecycle_status;
    fbe_lifecycle_state_t lifecycle_state;
    *speclz_state = FBE_FALSE;
    
    lifecycle_status = fbe_api_get_object_lifecycle_state(object_id, 
                                                &lifecycle_state, FBE_PACKAGE_ID_SEP_0);
    if(lifecycle_status != FBE_STATUS_OK)
    {
        fbe_cli_printf("%s Get lifecycle failed: obj id: 0x%x status: 0x%x \n",
                       __FUNCTION__, object_id, lifecycle_status);
        return lifecycle_status;
    }
    else if (lifecycle_state == FBE_LIFECYCLE_STATE_SPECIALIZE)
    {
        fbe_cli_printf("Object ID 0x%x is in SPECIALIZE\n\n", object_id);
        *speclz_state = FBE_TRUE;
        return FBE_STATUS_OK;
    }
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_cli_rginfo_speclz_object()
 ******************************************/


/*!**************************************************************
 *          fbe_cli_rginfo_display_object_metadata_bits()
 ****************************************************************
 *
 * @brief   Display raid metadata bits 
 *
 * @param   rg_obj_id   :    raid group object id
 *          chunk_offset:    beginning of the chunk
 *          chunk_counts:    number of the chunks
 * @param b_force_unit_access - FBE_TRUE - Get the paged data from disk (not from cache)
 *                              FBE_FALSE - Allow the data to come from cache     
 *
 * @return  None.
 *
 * @author
 *  05/10/2011 - Created. NChiu
 *
 ****************************************************************/
void fbe_cli_rginfo_display_object_metadata_bits(fbe_object_id_t rg_object_id,
                                                 fbe_chunk_index_t chunk_offset,
                                                 fbe_chunk_index_t chunk_counts,
                                                 fbe_bool_t b_force_unit_access)
{
    fbe_api_raid_group_get_info_t rg_info;
    fbe_status_t  status = FBE_STATUS_OK;
    fbe_u32_t index;
    fbe_chunk_count_t total_chunks;
    fbe_chunk_index_t chunks_remaining;
    fbe_chunk_index_t current_chunks;
    fbe_u32_t chunks_per_request;
    fbe_raid_group_paged_metadata_t *current_paged_p = NULL;
    fbe_raid_group_paged_metadata_t *paged_p = NULL;

    /* Get the information about the capacity so we can calculate number of chunks.
     */
    status = fbe_api_raid_group_get_info(rg_object_id, &rg_info, FBE_PACKET_FLAG_NO_ATTRIB);

    if (status != FBE_STATUS_OK) 
    { 
        fbe_cli_printf("%s: rg get info failed, obj 0x%x, status: 0x%x\n", 
                        __FUNCTION__, rg_object_id, status);
        return ; 
    }

    total_chunks = rg_info.total_chunks;

    if (chunk_offset > total_chunks)
    {
        fbe_cli_printf("%s: input chuck offset 0x%llx is greater than total chunks 0x%x\n", 
                        __FUNCTION__, (unsigned long long)chunk_offset, total_chunks);
        return; 
    }
    status = fbe_api_raid_group_max_get_paged_chunks(&chunks_per_request);
    if (status != FBE_STATUS_OK) 
    { 
        fbe_api_trace(FBE_TRACE_LEVEL_WARNING, "%s: get max paged failed, obj 0x%x, status: 0x%x\n", 
                        __FUNCTION__, rg_object_id, status);
        return; 
    }
    paged_p = fbe_api_allocate_memory(chunks_per_request * sizeof(fbe_raid_group_paged_metadata_t));

    chunks_remaining = FBE_MIN(chunk_counts, (total_chunks - chunk_offset));

    /* Loop over all the chunks, and sum up the bits across all chunks. 
     * We will fetch as many chunks as we can per request (chunks_per_request).
     */
    while (chunks_remaining > 0)
    {
        current_chunks = FBE_MIN(chunks_per_request, chunks_remaining);
        if (current_chunks > FBE_U32_MAX)
        {
            fbe_api_free_memory(paged_p);
            fbe_cli_printf("%s: current_chunks: %d > FBE_U32_MAX\n", __FUNCTION__, (int)current_chunks);
            return ;
        }
        status = fbe_api_raid_group_get_paged_metadata(rg_object_id,
                                                       chunk_offset, current_chunks, paged_p,
                                                       b_force_unit_access);
        if (status != FBE_STATUS_OK) 
        { 
            fbe_api_trace(FBE_TRACE_LEVEL_WARNING, "%s: get paged failed, obj 0x%x, status: 0x%x\n", 
                          __FUNCTION__, rg_object_id, status);
            return; 
        }

        /* For each chunk in this request, do display.
         */
        fbe_cli_printf("0x%-8llx | VLD | SYS I_W ERR URO URW |  NR  | RK | RV\n",  chunk_offset);
        
        for ( index = 0; index < current_chunks; index++)
        {
            current_paged_p = &paged_p[index];

            fbe_cli_printf("  %8x |  %d  |  %d   %d   %d   %d   %d  | %04x | %x  | %02x\n", 
                           index, 
                           current_paged_p->valid_bit,
                           ((current_paged_p->verify_bits & FBE_RAID_BITMAP_VERIFY_SYSTEM)>>4),
                           ((current_paged_p->verify_bits & FBE_RAID_BITMAP_VERIFY_INCOMPLETE_WRITE)>>3),
                           ((current_paged_p->verify_bits & FBE_RAID_BITMAP_VERIFY_ERROR)>>2),
                           ((current_paged_p->verify_bits & FBE_RAID_BITMAP_VERIFY_USER_READ_ONLY)>>1),
                           (current_paged_p->verify_bits & FBE_RAID_BITMAP_VERIFY_USER_READ_WRITE),
                           current_paged_p->needs_rebuild_bits,
                           current_paged_p->rekey,
                           current_paged_p->reserved_bits);
        }
        chunks_remaining -= current_chunks;
        chunk_offset += current_chunks;
    }

    fbe_api_free_memory(paged_p);
    return;
}
/******************************************
 * end fbe_cli_rginfo_display_object_metadata_bits()
 ******************************************/

/*!**************************************************************
 *          fbe_cli_rginfo_display_metadata_bits()
 ****************************************************************
 *
 * @brief   Display raid metadata bits 
 *
 * @param   rg_num      :    raid group number
 *          rg_obj_id   :    raid group object id
 *          chunk_offset:    beginning of the chunk
 *          chunk_counts:    number of the chunks
 * @param b_force_unit_access - FBE_TRUE - Get the paged data from disk (not from cache)
 *                              FBE_FALSE - Allow the data to come from cache    
 *
 * @return  None.
 *
 * @author
 *  05/10/2011 - Created. NChiu
 *
 ****************************************************************/
void fbe_cli_rginfo_display_metadata_bits(fbe_u32_t raid_group_num,
                                          fbe_object_id_t rg_object_id,
                                          fbe_chunk_index_t chunk_offset,
                                          fbe_chunk_index_t chunk_counts,
                                          fbe_bool_t b_force_unit_access)
{
    fbe_object_id_t raid_group_object_id = FBE_OBJECT_ID_INVALID;
    fbe_status_t    status = FBE_STATUS_OK;
   
    if ( rg_object_id != FBE_OBJECT_ID_INVALID)
    {
        raid_group_object_id = rg_object_id;
    }
    if(raid_group_num != FBE_RAID_ID_INVALID)
    {
        /* Set the debug flags for raid groups that we are interested in.
         * Currently is the is the the 3-way non-optimal raid group.
         */
        status = fbe_api_database_lookup_raid_group_by_number(raid_group_num, 
                                                          &raid_group_object_id);
        if (status != FBE_STATUS_OK)
        {
            fbe_cli_printf("rginfo: ERROR: Not able to get Raid group of number %d\n", 
                           raid_group_num);
            return;
        }
    }
    if ( raid_group_object_id != FBE_OBJECT_ID_INVALID)
    {
        fbe_cli_rginfo_display_object_metadata_bits(raid_group_object_id,
                                                    chunk_offset,
                                                    chunk_counts,
                                                    b_force_unit_access);
    }
}
/******************************************
 * end fbe_cli_rginfo_display_metadata_bits()
 ******************************************/

/*!**************************************************************
 *  fbe_cli_rginfo_map_lba()
 ****************************************************************
 *
 * @brief   map this lba and display the results.
 *
 * @param   raid_group_num
 * @param   raid_group_object_id
 * @param   lba - Lba to map
 *
 * @return  None.
 *
 * @author
 *  5/29/2012 - Created. Rob Foley
 *
 ****************************************************************/
void fbe_cli_rginfo_map_lba(fbe_u32_t raid_group_num,
                            fbe_object_id_t raid_group_object_id,
                            fbe_lba_t lba)
{
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_object_id_t                 rg_object_id = FBE_OBJECT_ID_INVALID;
    fbe_raid_group_map_info_t map_info;
    fbe_api_raid_group_get_info_t rg_info;

    if ( raid_group_object_id != FBE_OBJECT_ID_INVALID)
    {
        rg_object_id = raid_group_object_id;
        status = fbe_api_database_lookup_raid_group_by_object_id(rg_object_id, &raid_group_num);
        if (status != FBE_STATUS_OK)
        {
            fbe_cli_printf("rginfo: ERROR: Not able to get Raid group of number %d\n", 
                           raid_group_num);
            return;
        }
    }
    else if(raid_group_num != FBE_RAID_ID_INVALID)
    {
        /* Clear the debug flags for raid groups that we are interested in.
         * Currently is the is the the 3-way non-optimal raid group.
         */
        status = fbe_api_database_lookup_raid_group_by_number(raid_group_num, 
                                                          &rg_object_id);
        if (status != FBE_STATUS_OK)
        {
            fbe_cli_printf("rginfo: ERROR: Not able to get Raid group of number %d\n", 
                           raid_group_num);
            return;
        }
    }

    /* Get the current value of the raid group debug flags.
     */
    map_info.lba = lba;
    status = fbe_api_raid_group_map_lba(rg_object_id, &map_info);
    if (status != FBE_STATUS_OK)
    {
        fbe_cli_printf("rginfo: ERROR: error doing map"
                       "for Raid group %d lba: 0x%llx\n", raid_group_num, (unsigned long long)lba);
        return;
    }

    status = fbe_api_raid_group_get_info(rg_object_id, &rg_info, FBE_PACKET_FLAG_NO_ATTRIB);
    if (status != FBE_STATUS_OK)
    {
        fbe_cli_printf("rginfo: ERROR: error getting rg info"
                       "for Raid group %d lba: 0x%llx\n", raid_group_num, (unsigned long long)lba);
        return;
    }
    fbe_cli_printf("\n");
    fbe_cli_printf("Raid Group Number: 0x%x (%d) object id: 0x%x (%d)\n", 
                   raid_group_num, raid_group_num, rg_object_id, rg_object_id);
    fbe_cli_printf("lba: 0x%llx\n", (unsigned long long)map_info.lba);
    fbe_cli_printf("pba: 0x%llx\n", (unsigned long long)map_info.pba);
    fbe_cli_printf("data/primary pos:      %d\n", map_info.data_pos);
    fbe_cli_printf("parity/secondary pos:  %d\n", map_info.parity_pos);
    fbe_cli_printf("chunk_index:  %d\n", (int)map_info.chunk_index);
    fbe_cli_printf("metadata lba: 0x%llx\n", (unsigned long long)map_info.metadata_lba);
    fbe_cli_printf("\n");
    fbe_cli_rginfo_display_raid_type(rg_info.raid_type);
    fbe_cli_printf("width:               %d\n", rg_info.width);
    fbe_cli_printf("element size:        %d\n", rg_info.element_size);
    fbe_cli_printf("elements per parity: %d\n", rg_info.elements_per_parity_stripe);
    fbe_cli_printf("offset:              0x%llx\n", (unsigned long long)map_info.offset);

    return;
}
/******************************************
 * end fbe_cli_rginfo_map_lba()
 ******************************************/

/*!**************************************************************
 *  fbe_cli_rginfo_map_pba()
 ****************************************************************
 *
 * @brief   map this lba and display the results.
 *
 * @param   raid_group_num
 * @param   raid_group_object Id
 * @param   pba - pba to map
 * @param   position - disk position in the raid group
 *
 * @return  None.
 *
 * @author
 *  11/2/2012 - Created. Deanna Heng
 *
 ****************************************************************/
void fbe_cli_rginfo_map_pba(fbe_u32_t raid_group_num,
                            fbe_object_id_t raid_group_object_id,
                            fbe_lba_t pba, 
                            fbe_raid_position_t position)
{
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_object_id_t                 rg_object_id = FBE_OBJECT_ID_INVALID;
    fbe_raid_group_map_info_t map_info;
    fbe_api_raid_group_get_info_t rg_info;

    if ( raid_group_object_id != FBE_OBJECT_ID_INVALID)
    {
        rg_object_id = raid_group_object_id;
        status = fbe_api_database_lookup_raid_group_by_object_id(rg_object_id, &raid_group_num);
        if (status != FBE_STATUS_OK)
        {
            fbe_cli_printf("rginfo: ERROR: Not able to get Raid group of number %d\n", 
                           raid_group_num);
            return;
        }
    }
    else if(raid_group_num != FBE_RAID_ID_INVALID)
    {
        /* Clear the debug flags for raid groups that we are interested in.
         * Currently is the is the the 3-way non-optimal raid group.
         */
        status = fbe_api_database_lookup_raid_group_by_number(raid_group_num, 
                                                              &rg_object_id);
        if (status != FBE_STATUS_OK)
        {
            fbe_cli_printf("rginfo: ERROR: Not able to get Raid group of number %d\n", 
                           raid_group_num);
            return;
        }
    }

    /* Get the current value of the raid group debug flags.
     */
    map_info.pba = pba;
    map_info.position = position;
    status = fbe_api_raid_group_map_pba(rg_object_id, &map_info);
    if (status != FBE_STATUS_OK)
    {
        fbe_cli_printf("rginfo: ERROR: error doing map"
                       "for Raid group %d pba: 0x%llx position %d\n", raid_group_num, (unsigned long long)pba, position);
        return;
    }

    status = fbe_api_raid_group_get_info(rg_object_id, &rg_info, FBE_PACKET_FLAG_NO_ATTRIB);
    if (status != FBE_STATUS_OK)
    {
        fbe_cli_printf("rginfo: ERROR: error getting rg info"
                       "for Raid group %d pba: 0x%llx position %d\n", raid_group_num, (unsigned long long)pba, position);
        return;
    }

    fbe_cli_printf("\n");
    fbe_cli_printf("Raid Group Number: 0x%x (%d) object id: 0x%x (%d)\n", 
                   raid_group_num, raid_group_num, rg_object_id, rg_object_id);
    fbe_cli_printf("lba: 0x%llx\n", (unsigned long long)map_info.lba);
    fbe_cli_printf("pba: 0x%llx\n", (unsigned long long)map_info.pba);
    fbe_cli_printf("data/primary pos:      %d\n", map_info.data_pos);
    fbe_cli_printf("parity/secondary pos:  %d\n", map_info.parity_pos);
    fbe_cli_printf("chunk_index:  %d\n", (int)map_info.chunk_index);
    fbe_cli_printf("metadata lba: 0x%llx\n", (unsigned long long)map_info.metadata_lba);
    fbe_cli_printf("\n");
    fbe_cli_rginfo_display_raid_type(rg_info.raid_type);
    fbe_cli_printf("width:               %d\n", rg_info.width);
    fbe_cli_printf("element size:        %d\n", rg_info.element_size);
    fbe_cli_printf("elements per parity: %d\n", rg_info.elements_per_parity_stripe);
    fbe_cli_printf("offset:              0x%llx\n", (unsigned long long)map_info.offset);
    fbe_cli_printf("position:            %d\n", map_info.position);

    return;
}
/******************************************
 * end fbe_cli_rginfo_map_pba()
 ******************************************/

/*!**************************************************************
 *  fbe_cli_rginfo_paged_summary()
 ****************************************************************
 *
 * @brief   Display the paged metadata.
 *
 * @param   raid_group_num
 * @param   raid_group_object Id
 * @param   b_force_unit_access - FBE_TRUE - Get metadata from disk (not cache)
 *
 * @return  None.
 *
 * @author
 *  4/9/2013 - Created. Rob Foley
 *
 ****************************************************************/
void fbe_cli_rginfo_paged_summary(fbe_u32_t raid_group_num,
                                  fbe_object_id_t raid_group_object_id,
                                  fbe_bool_t b_force_unit_access)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_api_raid_group_get_paged_info_t paged_info;
    fbe_u32_t index;
    fbe_object_id_t rg_object_id = FBE_OBJECT_ID_INVALID;

    if ( raid_group_object_id != FBE_OBJECT_ID_INVALID)
    {
        rg_object_id = raid_group_object_id;
        status = fbe_api_database_lookup_raid_group_by_object_id(rg_object_id, &raid_group_num);
        if (status != FBE_STATUS_OK)
        {
            fbe_cli_printf("rginfo: ERROR: Not able to get Raid group of number %d\n", 
                           raid_group_num);
            return;
        }
    }
    else if (raid_group_num != FBE_RAID_ID_INVALID)
    {
        /* Clear the debug flags for raid groups that we are interested in.
         * Currently is the is the the 3-way non-optimal raid group.
         */
        status = fbe_api_database_lookup_raid_group_by_number(raid_group_num, &rg_object_id);
        if (status != FBE_STATUS_OK)
        {
            fbe_cli_printf("rginfo: ERROR: Not able to get Raid group of number %d\n", 
                           raid_group_num);
            return;
        }
    }

    /* Clear this out since we will be adding to it.
     */
    fbe_zero_memory(&paged_info, sizeof(fbe_api_raid_group_get_paged_info_t)); 
    status = fbe_api_raid_group_get_paged_bits(rg_object_id, &paged_info, b_force_unit_access);
    if (status != FBE_STATUS_OK)
    {
        fbe_cli_error("rginfo error %d on get paged bits\n", status);
        return;
    } 
    fbe_cli_printf("chunk_count:                     0x%llx\n", (unsigned long long)paged_info.chunk_count);
    fbe_cli_printf("num_nr_chunks:                   0x%llx\n", paged_info.num_nr_chunks);
    fbe_cli_printf("needs_rebuild_bitmask:           0x%x\n", paged_info.needs_rebuild_bitmask);
    fbe_cli_printf("num_user_vr_chunks:              0x%llx\n", paged_info.num_user_vr_chunks);
    fbe_cli_printf("num_error_vr_chunks:             0x%llx\n", paged_info.num_error_vr_chunks);
    fbe_cli_printf("num_read_only_vr_chunks:         0x%llx\n", paged_info.num_read_only_vr_chunks);
    fbe_cli_printf("num_incomplete_write_vr_chunks:  0x%llx\n", paged_info.num_incomplete_write_vr_chunks);
    fbe_cli_printf("num_system_vr_chunks:            0x%llx\n", paged_info.num_system_vr_chunks);
    fbe_cli_printf("num_rekey_chunks:                0x%llx\n", paged_info.num_rekey_chunks);

    for (index = 0; index < FBE_RAID_MAX_DISK_ARRAY_WIDTH; index++)
    {
        fbe_cli_printf("pos_nr_chunks[%d]:           0x%llx\n", index, paged_info.pos_nr_chunks[index]);
    }
    
    return;
}
/******************************************
 * end fbe_cli_rginfo_paged_summary()
 ******************************************/
/*!**************************************************************
 * fbe_cli_rginfo_display_fru_stats()
 ****************************************************************
 * @brief
 *  Display the formatted per fru stats.
 *
 * @param stats_p - FRU stats data to display.
 * @param width - Number of entries to display.               
 *
 * @return -   
 *
 * @author
 *  4/30/2013 - Created. Rob Foley
 *
 ****************************************************************/

void fbe_cli_rginfo_display_fru_stats(fbe_raid_fru_statistics_t *stats_p,
                                      fbe_u32_t width)
{
    fbe_u32_t index;
    for (index = 0; index < width; index++)
    {
        fbe_cli_printf("(%2d) %8d %8d %8d %8d %8d %8d (0x%x)\n", 
                       index,
                       stats_p[index].read_count, 
                       stats_p[index].write_count,
                       stats_p[index].write_verify_count,
                       stats_p[index].zero_count,
                       stats_p[index].parity_count,
                       stats_p[index].unknown_count,
                       stats_p[index].unknown_opcode);
    }
}
/******************************************
 * end fbe_cli_rginfo_display_fru_stats()
 ******************************************/
/*!**************************************************************
 * fbe_cli_rginfo_display_statistics()
 ****************************************************************
 *
 * @brief   Set debug flags for the raid group
 *
 * @param cmd_p - Input command to execute.
 *
 * @return  None.
 *
 * @author
 *  07/01/2010 - Created. Swati Fursule
 *
 ****************************************************************/
void fbe_cli_rginfo_display_statistics(fbe_cli_rginfo_cmd_t *cmd_p)
{
    fbe_status_t status;
    fbe_raid_group_get_statistics_t stats;
    fbe_api_raid_group_get_info_t rg_info;
    fbe_raid_library_statistics_t *rl_stats_p = &stats.raid_library_stats;
    fbe_u32_t index;

    status = fbe_api_raid_group_get_info(cmd_p->rg_object_id, &rg_info, FBE_PACKET_FLAG_NO_ATTRIB);
    if (status != FBE_STATUS_OK)
    {
        fbe_cli_error("status %d from get info\n", status);
        return;
    }
    status = fbe_api_raid_group_get_stats(cmd_p->rg_object_id, &stats);
    if (status != FBE_STATUS_OK)
    {
        fbe_cli_error("status %d from get stats\n", status);
        return;
    }
    fbe_cli_printf("Position   Read     Write     Write-Vr Zero    Parity  Unknown\n");
    fbe_cli_printf("Totals: \n");
    fbe_cli_rginfo_display_fru_stats(&rl_stats_p->total_fru_stats[0], rg_info.width);

    fbe_cli_printf("Metadata: \n");
    fbe_cli_rginfo_display_fru_stats(&rl_stats_p->metadata_fru_stats[0], rg_info.width);

    fbe_cli_printf("Journal: \n");
    fbe_cli_rginfo_display_fru_stats(&rl_stats_p->journal_fru_stats[0], rg_info.width);

    fbe_cli_printf("User: \n");
    fbe_cli_rginfo_display_fru_stats(&rl_stats_p->user_fru_stats[0], rg_info.width);
    fbe_cli_printf("--------------------------------------------------\n");
    fbe_cli_printf("Reads:          %8d\n", rl_stats_p->read_count);
    fbe_cli_printf("Writes:         %8d\n", rl_stats_p->write_count);
    fbe_cli_printf("Verify-Write:   %8d\n", rl_stats_p->verify_write_count);
    fbe_cli_printf("Zero count:     %8d\n", rl_stats_p->zero_count);
    fbe_cli_printf("Unknown Count:  %8d\n", rl_stats_p->unknown_count);
    fbe_cli_printf("Unknown Opcode: %8d\n", rl_stats_p->unknown_opcode);
    fbe_cli_printf("IOTS:           %8d\n", rl_stats_p->total_iots);
    fbe_cli_printf("SIOTS:          %8d\n", rl_stats_p->total_siots);
    fbe_cli_printf("Nest SIOTS:     %8d\n", rl_stats_p->total_nest_siots);

    fbe_cli_printf("Outstanding io count:     %llu\n", stats.outstanding_io_count);
    fbe_cli_printf("Outstanding io credits:   %u\n", stats.outstanding_io_credits);

    for (index = 0; index < FBE_PACKET_PRIORITY_LAST - 1; index++)
    {
        fbe_cli_printf("%d) %d\n", index, stats.queue_length[index]);
    }
    
}
/**************************************
 * end fbe_cli_rginfo_display_statistics
 **************************************/
static void fbe_cli_rginfo_paged_usage(const char *name)
{
    if (strcmp(name, "-clear_cache") == 0) {
        fbe_cli_printf("Usage: %s \n",
                       name);
    } else {
        fbe_cli_printf("Usage: %s <chunk_offset> <chunk_count>"
                       " -valid <value> -verify <value> -nr <value> -rekey <value> -reserved <value>\n",
                       name);
    }
}

static fbe_status_t fbe_cli_rginfo_paged_check_valid(fbe_object_id_t rg_object_id,
                                                     fbe_cli_paged_change_t *change)
{
    fbe_api_raid_group_get_info_t rg_info;
    fbe_status_t  status = FBE_STATUS_OK;
    fbe_chunk_count_t total_chunks;
    fbe_chunk_index_t chunk_offset;
    fbe_chunk_count_t chunk_count;

    /* Get the information about the capacity so we can calculate number of chunks.
     */
    status = fbe_api_raid_group_get_info(rg_object_id, &rg_info, FBE_PACKET_FLAG_NO_ATTRIB);

    if (status != FBE_STATUS_OK) {
        fbe_cli_printf("%s: rg get info failed, obj 0x%x, status: 0x%x\n",
                        __FUNCTION__, rg_object_id, status);
        return status;
    }

    /* If clear cache return success
     */
    if (change->operation == FBE_CLI_PAGED_OP_CLEAR_CACHE) {
        return FBE_STATUS_OK;
    }

    total_chunks = rg_info.total_chunks;
    chunk_offset = change->chunk_index;
    chunk_count = change->chunk_count;

    if (chunk_offset >= total_chunks) {
        fbe_cli_printf("%s: input chuck offset 0x%llx is greater than total chunks 0x%x\n",
                        __FUNCTION__, (unsigned long long)chunk_offset, total_chunks);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    if (chunk_offset + chunk_count > total_chunks) {
        change->chunk_count = (fbe_chunk_count_t) (total_chunks - chunk_offset);
        fbe_cli_printf("%s: shrink chunk count to %u\n",
                       __FUNCTION__, (fbe_u32_t)change->chunk_count);
    }
    return FBE_STATUS_OK;
}
/*!**************************************************************
 * fbe_cli_rginfo_parse_paged_switches
 ****************************************************************
 *
 * @brief   parse paged metadata command switches
 *
 * @param argc
 * @param argv
 * @param args_processed_p - Ptr to number of args we dealt with
 * @param cmd_p - Ptr to current rg command.
 *
 * @return FBE_STATUS_OK for success, else error.
 *
 * @author
 *  05/03/2013 - Created. Jamin Kang
 *
 ****************************************************************/
static fbe_status_t fbe_cli_rginfo_parse_paged_switches(const char *cmd_name,
                                                        int argc, char **argv,
                                                        fbe_u32_t *args_processed,
                                                        fbe_cli_rginfo_cmd_t *cmd_p)
{
    fbe_status_t status;
    int orig_argc = argc;
    fbe_u64_t v;
    fbe_chunk_index_t chunk_index;
    fbe_chunk_count_t chunk_count;
    fbe_cli_paged_change_t *change = &cmd_p->change;
    fbe_raid_group_paged_metadata_t *bitmap = (fbe_raid_group_paged_metadata_t *)&change->bitmap[0];
    fbe_raid_group_paged_metadata_t *data = (fbe_raid_group_paged_metadata_t *)&change->data[0];

    if (cmd_p->change.operation == FBE_CLI_PAGED_OP_CLEAR_CACHE) {
        if (argc != 0) {
            fbe_cli_rginfo_paged_usage(cmd_name);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        return FBE_STATUS_OK;
    }
    if (argc <= 2) {
        fbe_cli_rginfo_paged_usage(cmd_name);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    status = fbe_atoh64(argv[0], &chunk_index);
    if (status != FBE_STATUS_OK) {
        fbe_cli_error("Invalid chunk index '%s'\n", argv[0]);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    status = fbe_atoh64(argv[1], &v);
    if (status != FBE_STATUS_OK) {
        fbe_cli_error("Invalid chunk count '%s'\n", argv[1]);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    chunk_count = (fbe_u32_t)v;
    argc -= 2;
    argv += 2;

    change->metadata_size = sizeof(fbe_raid_group_paged_metadata_t);
    change->chunk_index = chunk_index;
    change->chunk_count = chunk_count;

    while (argc) {
        if (argc < 2 || argv[0][0] != '-') {
            fbe_cli_rginfo_paged_usage(cmd_name);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        status = fbe_atoh64(argv[1], &v);
        if (status != FBE_STATUS_OK) {
            fbe_cli_error("Invalid value '%s'\n", argv[1]);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        if (!strcmp(*argv, "-valid")) {
            data->valid_bit = (fbe_u8_t)v;
            bitmap->valid_bit = 0x1;
        } else if (!strcmp(*argv, "-verify")) {
            data->verify_bits = (fbe_u8_t)v;
            bitmap->verify_bits = 0x7f;
        } else if (!strcmp(*argv, "-nr")) {
            data->needs_rebuild_bits = (fbe_u16_t)v;
            bitmap->needs_rebuild_bits = 0xffff;
        } else if (!strcmp(*argv, "-rekey")) {
            data->rekey = (fbe_u8_t)v;
            bitmap->rekey = 0x1;
        } else if (!strcmp(*argv, "-reserved")) {
            data->reserved_bits = (fbe_u8_t)v;
            bitmap->reserved_bits = 0x7f;
        } else {
            fbe_cli_error("Invalid switch '%s'\n", *argv);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        argc -= 2;
        argv += 2;
    }
    *args_processed = orig_argc - argc;

    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_cli_rginfo_parse_paged_switches
 **************************************/
/*!**************************************************************
 * fbe_cli_rginfo_paged_switch
 ****************************************************************
 *
 * @brief   do paged metadata operation
 *
 * @param cmd - Ptr to current rg command.
 *
 * @return None
 *
 * @author
 *  05/03/2013 - Created. Jamin Kang
 *
 ****************************************************************/
static void fbe_cli_rginfo_paged_switch(fbe_cli_rginfo_cmd_t *cmd)
{
    fbe_status_t status;
    fbe_cli_paged_change_t *change = &cmd->change;

    status = fbe_cli_rginfo_paged_check_valid(cmd->rg_object_id, change);
    if (status != FBE_STATUS_OK)
        return ;

    status = fbe_cli_paged_switch(cmd->rg_object_id, change);
    if (status != FBE_STATUS_OK) {
        fbe_cli_printf("%s failed: %u\n",
                       fbe_cli_paged_get_operation_name(change->operation),
                       (fbe_u32_t)status);
    }
}
/**************************************
 * end fbe_cli_rginfo_paged_switch
 **************************************/
fbe_status_t fbe_cli_encryption_generate_encryption_key(fbe_object_id_t object_id,
                                                       fbe_database_control_setup_encryption_key_t *key_info)
{
    fbe_status_t status;
    fbe_config_generation_t generation_number;
    fbe_api_base_config_get_width_t get_width;
    fbe_u8_t            keys[FBE_ENCRYPTION_KEY_SIZE];
    fbe_u32_t           index;

    status = fbe_api_base_config_get_generation_number(object_id, &generation_number);
    if (status != FBE_STATUS_OK) {
        fbe_cli_printf("cannot get generation number object: 0x%x status: 0x%x\n", object_id, status);
        return status;
    }

    status = fbe_api_base_config_get_width(object_id, &get_width);
    if (status != FBE_STATUS_OK) {
        fbe_cli_printf("cannot get width object: 0x%x status: 0x%x\n", object_id, status);
        return status;
    }

    fbe_zero_memory(keys, FBE_ENCRYPTION_KEY_SIZE);

    key_info->key_setup.num_of_entries = 1;
    key_info->key_setup.key_table_entry[0].object_id = object_id;
    key_info->key_setup.key_table_entry[0].num_of_keys = get_width.width;
    key_info->key_setup.key_table_entry[0].generation_number = generation_number;

    for(index = 0; index < key_info->key_setup.key_table_entry[0].num_of_keys; index++)
    {
        _snprintf(&keys[0], FBE_ENCRYPTION_KEY_SIZE, "FBEFBE%d%d%d\n",index, object_id, 0);
        fbe_copy_memory(&key_info->key_setup.key_table_entry[0].encryption_keys[index].key1, keys, FBE_ENCRYPTION_KEY_SIZE);
        key_info->key_setup.key_table_entry[0].encryption_keys[index].key_mask = 0;
    }
    return FBE_STATUS_OK;
}

fbe_status_t fbe_cli_encryption_invalidate_key(fbe_object_id_t object_id,
                                              fbe_database_control_setup_encryption_key_t *key_info,
                                              fbe_u32_t key_index,
                                              fbe_encryption_key_mask_t mask)
{
    fbe_u8_t keys[FBE_ENCRYPTION_KEY_SIZE];
    fbe_u32_t index;

    fbe_zero_memory(keys, FBE_ENCRYPTION_KEY_SIZE);

    key_info->key_setup.num_of_entries = 1;

    for(index = 0; index < key_info->key_setup.key_table_entry[0].num_of_keys; index++)
    {
        if (key_index == 0) {
            fbe_copy_memory(&key_info->key_setup.key_table_entry[0].encryption_keys[index].key1, keys, FBE_ENCRYPTION_KEY_SIZE);
        } else {
            fbe_copy_memory(&key_info->key_setup.key_table_entry[0].encryption_keys[index].key2, keys, FBE_ENCRYPTION_KEY_SIZE);
        } 
        fbe_encryption_key_mask_set(&key_info->key_setup.key_table_entry[0].encryption_keys[index].key_mask,
                                    key_index,
                                    mask);
    }
    return FBE_STATUS_OK;
}
fbe_status_t fbe_cli_encryption_generate_next_key(fbe_object_id_t object_id,
                                                 fbe_database_control_setup_encryption_key_t *key_info,
                                                 fbe_u32_t key_index,
                                                 fbe_u32_t generation,
                                                 fbe_encryption_key_mask_t mask)
{
    fbe_u8_t  keys[FBE_ENCRYPTION_KEY_SIZE];
    fbe_u32_t index;

    key_info->key_setup.num_of_entries = 1;
    for(index = 0; index < key_info->key_setup.key_table_entry[0].num_of_keys; index++) {
        _snprintf(&keys[0], FBE_ENCRYPTION_KEY_SIZE, "FBEFBE%d%d%d\n",index, object_id, generation);
        if (key_index == 0) {
            fbe_copy_memory(&key_info->key_setup.key_table_entry[0].encryption_keys[index].key1, keys, FBE_ENCRYPTION_KEY_SIZE);
        } else {
            fbe_copy_memory(&key_info->key_setup.key_table_entry[0].encryption_keys[index].key2, keys, FBE_ENCRYPTION_KEY_SIZE);
        }

        fbe_encryption_key_mask_set(&key_info->key_setup.key_table_entry[0].encryption_keys[index].key_mask,
                                    key_index,
                                    mask);
    }
    return FBE_STATUS_OK;
}

fbe_status_t fbe_cli_wait_rg_encryption_state(fbe_object_id_t rg_object_id,
                                              fbe_base_config_encryption_state_t expected_state,
                                              fbe_u32_t wait_ms)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_base_config_encryption_state_t encryption_state;
    fbe_api_raid_group_get_info_t   rg_info = {0};
    fbe_u32_t elapsed_msec = 0;
    fbe_u32_t target_wait_msec = wait_ms;

    /* We arbitrarily decide to wait 2 seconds in between displaying.
     */
    #define REKEY_WAIT_MSEC 2000

    /* Keep looping until we hit the target wait milliseconds.
     */
    while (elapsed_msec < target_wait_msec) {
        status = fbe_api_base_config_get_encryption_state(rg_object_id, &encryption_state);
        if (status != FBE_STATUS_OK){
            return status;
        }
        status = fbe_api_raid_group_get_info(rg_object_id, &rg_info, FBE_PACKET_FLAG_NO_ATTRIB);
        if (status != FBE_STATUS_OK){
            return status;
        }
        if (encryption_state == expected_state){
            /* Only display complete if we waited below.
             */
            if (elapsed_msec > 0) {
                fbe_cli_printf( "=== %s rekey complete rg: 0x%x", 
                           __FUNCTION__, rg_object_id);
            }
            return status;
        }
        /* Display after 2 seconds.
         */
        if ((elapsed_msec % REKEY_WAIT_MSEC) == 0) {
            fbe_cli_printf( "=== %s waiting for rekey rg: 0x%x exp_state: 0x%x encr_state: 0x%x, chkpt: 0x%llx", 
                            __FUNCTION__, rg_object_id, expected_state, encryption_state, 
                            (unsigned long long)rg_info.rekey_checkpoint);
        }
        EmcutilSleep(500);
        elapsed_msec += 500;
    }

    /* If we waited longer than expected, return a failure. 
     */
    if (elapsed_msec >= wait_ms) {
        status = FBE_STATUS_TIMEOUT;
    }
    return status;
}
static void fbe_cli_rginfo_start_encryption(fbe_cli_rginfo_cmd_t *cmd_p)
{       
    fbe_database_control_setup_encryption_key_t encryption_info;
    fbe_status_t        status;
    fbe_object_id_t rg_object_id = cmd_p->rg_object_id; 
    if (rg_object_id == FBE_OBJECT_ID_INVALID) {
        return;
    }
    fbe_zero_memory(&encryption_info, sizeof(fbe_database_control_setup_encryption_key_t));

    status = fbe_cli_encryption_generate_encryption_key(rg_object_id, &encryption_info);
    if (status != FBE_STATUS_OK) {
        fbe_cli_printf("generate keys failed object: 0x%x status: 0x%x\n", rg_object_id, status);
        return;
    }

    /* Encryption start or end.  Keep key 0 invalid, add key 1 as valid.
     */
    fbe_cli_encryption_invalidate_key(rg_object_id, &encryption_info, 0, FBE_ENCRYPTION_KEY_MASK_UPDATE);
    
        /* When we start we are adding a key and it needs to get updated.
         */
        fbe_cli_encryption_generate_next_key(rg_object_id, &encryption_info, 1, 1, 
                                            (FBE_ENCRYPTION_KEY_MASK_UPDATE | FBE_ENCRYPTION_KEY_MASK_VALID));
    /* Push it down */
    status = fbe_api_database_setup_encryption_rekey(&encryption_info);
    if (status != FBE_STATUS_OK) {
        fbe_cli_printf("push keys failed object: 0x%x status: 0x%x\n", rg_object_id, status);
        return;
    }

    status = fbe_cli_wait_rg_encryption_state(rg_object_id, FBE_BASE_CONFIG_ENCRYPTION_STATE_KEYS_PROVIDED, 
                                              60000);
    if (status == FBE_STATUS_OK) {
        status = fbe_api_change_rg_encryption_mode(rg_object_id, FBE_BASE_CONFIG_ENCRYPTION_MODE_ENCRYPTION_IN_PROGRESS);

        if (status != FBE_STATUS_OK) {
            fbe_cli_printf("start rekey failed object: 0x%x status: 0x%x\n", rg_object_id, status);
            return;
        }
    }
}

/*!**************************************************************
 * fbe_cli_rginfo_set_wear_level_timer_interval
 ****************************************************************
 *
 * @brief Set the wear leveling lifecycle condition timer interval
 *        When the timer expires the wear leveling will be fetched
 *        downstream from the drives
 *
 * @param cmd_p - command details
 *
 * @return None
 *
 * @author
 *  07/23/2015 - Created. Deanna Heng
 *
 ****************************************************************/
static void fbe_cli_rginfo_set_wear_level_timer_interval(fbe_cli_rginfo_cmd_t *cmd_p)
{
    fbe_status_t        status;
    fbe_lifecycle_timer_info_t timer_info = {0};
    

    if (cmd_p->wear_level_timer_interval <= 0) {
        fbe_cli_printf("Invalid interval object: 0x%x interval: 0x%x\n", cmd_p->rg_object_id, cmd_p->wear_level_timer_interval);
        return;
    }

    /* lifecycle timer is by 100ths of a second. interval is in seconds */
    timer_info.interval = 100 * cmd_p->wear_level_timer_interval;
    timer_info.lifecycle_condition = FBE_RAID_GROUP_SET_LIFECYCLE_TIMER_COND_WEAR_LEVEL;
    status = fbe_api_raid_group_set_lifecycle_timer_interval(cmd_p->rg_object_id, 
                                                             &timer_info);

    if (status != FBE_STATUS_OK) {
        fbe_cli_printf("Set wear level timer interval failed. object: 0x%x status: 0x%x\n", cmd_p->rg_object_id, status);
        return;
    }

    fbe_cli_printf("Set wear level timer interval successful. object: 0x%x interval: 0x%x\n", cmd_p->rg_object_id, cmd_p->wear_level_timer_interval);

    return;
}

/*!**************************************************************
 * fbe_cli_rginfo_set_wear_level_timer_interval
 ****************************************************************
 *
 * @brief Get the wear level info from the raid group. This is based on the max
 *        wear leveling percent of a drive in the raid gorup
 *
 * @param cmd_p - command details
 *
 * @return None
 *
 * @author
 *  07/23/2015 - Created. Deanna Heng
 *
 ****************************************************************/
static void fbe_cli_rginfo_get_wear_leveling_info(fbe_cli_rginfo_cmd_t *cmd_p)
{
    fbe_status_t        status;

    status = fbe_api_raid_group_get_wear_level(cmd_p->rg_object_id, 
                                               &cmd_p->wear_leveling_info);

    if (status != FBE_STATUS_OK) {
        fbe_cli_printf("Set wear level timer interval failed. object: 0x%x status: 0x%x\n", cmd_p->rg_object_id, status);
        return;
    }

    fbe_cli_printf("Raid group (0x%x) Wear Leveling Information :\n", cmd_p->rg_object_id);
    fbe_cli_printf("Current PE Cycle Count: 0x%llx\n", cmd_p->wear_leveling_info.current_pe_cycle);
    fbe_cli_printf("EOL PE Cycle Count: 0x%llx\n", cmd_p->wear_leveling_info.max_pe_cycle);
    fbe_cli_printf("Power on hours: 0x%llx\n", cmd_p->wear_leveling_info.power_on_hours);

    return;
}

/*************************
 * end file fbe_cli_lib_rginfo_cmds.c
 *************************/
