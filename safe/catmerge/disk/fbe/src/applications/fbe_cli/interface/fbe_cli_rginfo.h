#ifndef __FBE_CLI_RGINFO_H__
#define __FBE_CLI_RGINFO_H__
/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_cli_rginfo.h
 ***************************************************************************
 *
 * @brief
 *  This file contains raid group info command cli definitions.
 *
 * @version
 *  07/01/2010 - Created. Swati Fursule
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

#include "fbe/fbe_cli.h"
#include "fbe/fbe_lun.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_base_config_interface.h"
#include "fbe/fbe_api_job_service_interface.h"


void fbe_cli_rginfo(int argc , char ** argv);
#define RGINFO_CMD_USAGE "\
rginfo -help|h\n\
rginfo -d -rg <raidgroup number|all|user> |-object_id <object id | all> \n\
-default_debug_flags|ddf <value> -default_library_flags|dlf <value> \n\
-debug_flags|df <value> -library_flags|lf <value> -trace_level|tl <value> \n\
-display_raid_memory_stats|disp_rms\n\
-chunk_info <offset> <count> [-fua <0/1>]\n\
-paged_set_bits <offset> <count> [-valid <value>] [-verify <value>] [-nr <value>] [-reserved <value>] \n\
-paged_clear_bits <offset> <count> [-valid <value>] [-verify <value>] [-nr <value>] [-reserved <value>] \n\
-paged_write <offset> <count> [-valid <value>] [-verify <value>] [-nr <value>] [-reserved <value>] \n\
-clear_cache |-wear_level_timer <seconds> |-wear_level\n\
Note that Object ids, debug flags, trace level,  paged operation (set/clear bits, write) values are in Hexadecimal \n\
and Raid group number is in decimal \n\
Switches:\n\
    -d                        - Display mode (else set mode) In display mode, any value for the options provided is ignored\n\
    -rg                       - for Raid group [RG number | all | user]\n\
    -object_id                - for Object Id [Object Id | all ]\n\
    -get_lib_debug_flags|gldf - Get the library debug flags]\n\
    -get_rg_debug_flags|grdf - Get the raid group debug flags]\n\
    -debug_flags|df [flag] - Set the raid group debug flags.\n\
    -library_flags|lf [flag]   - Set the library debug flags ]\n\
    -default_debug_flags|-ddf   - [default DEBUG_flag value, this will be populated for newly created raid group ]\n\
    -default_library_flags|-dlf - [default library_flag value, this will be populated for newly created raid group ]\n\
    -user_default_debug_flags|-udf [Set user rg default rg flags]\n\
    -system_default_debug_flags|-sdf [Set system rg default rg flags]\n\
    -user_default_library_flags|-ulf [Set user rg default library flags]\n\
    -system_default_library_flags|-slf [Set system rg default library flags]\n\
    -get_trace_level|gtl      - Get the raid group trace level\n\
    -trace_level|tl           - [trace_level value ]\n\
    -chunk_info               - [chunk offset / count] this will display metadata bits for the specified range\n\
    -fua                      - Force Unit Access (Default is 1 which gets data from disk. 0 - Allow data from cache)\n\
    -map_lba [lba]            - Map the lba to a pba.\n\
    -map_pba [pba] [position] - Map the pba to an lba.\n\
    -display_raid_memory_stats|disp_rms  - Display raid memory statistics\n\
    -reset_raid_memory_stats|reset_rms  - Reset raid memory statistics\n\
    -paged_summary            - Display counts of bits set in paged metadata.\n\
    -display_stats            - Display counts of in progress I/Os.\n\
    -get_bts_params           - Get the default throttling parameters.\n\
    -set_bts_params [throttle enable 0 or 1][user queue depth][system queue depth] - Set our default throttle parameters.\n\
    -persist_bts_params [throttle enable 0 or 1][user queue depth][system queue depth] - Set our default throttle parameters and persist it.\n\
    -reload_bts_params        - Reload the persisted copy of throttle parameters.\n\
    -get_throttle             - Get throttle information on a specific raid group.\n\
    -set_throttle [io max][max_credits][bw throttle] - Set our throttle params for this raid group.\n\
    -paged_set_bits           - set paged metadata bits.\n\
    -paged_clear_bits         - clear paged metadata bits.\n\
    -paged_write              - write paged metadata bits.\n\
    -clear_cache              - clear the paged metadata cache.\n\
    -wear_level_timer         - set the wear level lifecycle timer condition interval in seconds.\n\
    -wear_level               - get the wear leveling information\n"

void fbe_cli_rginfo_display_raid_type(fbe_raid_group_type_t raid_type);
/*************************
 * end file fbe_cli_rginfo.h
 *************************/

#endif /* end __FBE_CLI_RGINFO_H__ */
