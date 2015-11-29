#ifndef __FBE_CLI_LIB_PROVISION_DRIVE_CMDS_H__
#define __FBE_CLI_LIB_PROVISION_DRIVE_CMDS_H__
/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_cli_lib_provision_drive_cmds.h
 ***************************************************************************
 *
 * @brief
 *  This file contains provision drive command cli declaration.
 *
 * @version
 *   1/13/2011:  Created. Sandeep Chaudhari
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/

/*************************
 *   FUNCTION DECLARATION
 *************************/
void fbe_cli_provision_drive_set_debug_flag(fbe_s32_t argc, fbe_s8_t ** argv);
void fbe_cli_provision_drive_info(fbe_s32_t argc, fbe_s8_t ** argv);
void fbe_cli_display_all_pvd_objects(fbe_bool_t list);
void fbe_cli_display_all_pvd_objects_block_size(void);
void fbe_cli_get_all_pvd_objects_with_block_size(fbe_object_id_t * pvd_object_id_list_p, 
                                                 fbe_u32_t *num_objects_with_blcksz, fbe_u32_t block_size);
void fbe_cli_display_pvd_info(fbe_object_id_t pvd_object_id, fbe_bool_t list);
fbe_u32_t fbe_cli_get_pvd_info_block_size(fbe_object_id_t pvd_object_id);
void fbe_cli_display_pvd_info_block_size(fbe_object_id_t pvd_object_id);
void fbe_cli_display_pvd_sniff_verify_info(fbe_provision_drive_verify_report_t  *pvd_verify_report_p,
                                           fbe_provision_drive_get_verify_status_t  *verify_status_p);
void fbe_cli_display_pvd_debug_flags_info(fbe_provision_drive_debug_flags_t debug_flags);
void fbe_cli_display_pvd_paged_metadata(fbe_object_id_t pvd_object_id,
                                        fbe_chunk_index_t  chunk_index,
                                        fbe_chunk_count_t chunk_count,
                                        fbe_bool_t b_force_unit_access);
void fbe_cli_display_pvd_zeroing_percentage(fbe_object_id_t pvd_object_id);
void fbe_cli_remove_zero_checkpoint(fbe_s32_t argc, fbe_s8_t ** argv);
void fbe_cli_cmd_clear_pvd_drive_states(fbe_s32_t argc, fbe_s8_t ** argv);
void fbe_cli_pvd_time_threshold(fbe_s32_t argc, fbe_s8_t ** argv);
void fbe_cli_sleep_at_prompt_for_twenty_seconds(fbe_s32_t argc, fbe_s8_t ** argv); //don't check in
void fbe_cli_provision_drive_wear_level_timer(fbe_s32_t argc, fbe_s8_t ** argv);
#define PVD_SET_DEBUG_FLAG_USAGE "\
spddf -h | -help \n\
spddf -object_id <object id | all> -debug_flags <debug_flag value (in hex)> \n\n\
spddf -bes <b_e_s>                 -debug_flags <debug_flag value (in hex)> \n\n\
Switches:\n\
  -object_id    - for Object Id [ Object Id | all ] \n\
  -bes <b_e_s>   - bus_enclosure_slot format %d_%d_%d. Alternative to -object_id\n\
  -debug_flags  - [debug_flag value]\n\n\
Debug Flag Value & Description Lookup: \n\n\
0x0 \t  None - To clear debug flag value\n\
0x1 \t  General Tracing \n\
0x2 \t  Background Zeroing Tracing \n\
0x4 \t  User Zero Tracing \n\
0x8 \t  Zeroing On Demand Tracing \n\
0x10\t  Verify Tracing  \n\
0x20\t  Metadata Tracing  \n\
0x40\t  Lock Tracing \n\n\
e.g. \n\
Following example set General Tracing and User Zero Tracing debug flag at class level.\n\
spddf -object_id all -debug_flags 0x5 \n\n\
Following example set Background Zeroing Tracing and Zeroing On Demand Tracing debug flag at object level.\n\
spddf -object_id 0x109 -debug_flags 0xA \n\n\
Following example set User Zero Tracing and Verify Tracing debug flag at object level.\n\
spddf -object_id 0x109 -debug_flags 0x14 \n\n\
spddf -b_e_s 0_0_10 -debug_flags 0x14 \n\n\
\n"

#define PVD_GET_INFO_USAGE "\
pvdinfo -h  -help \n\
pvdinfo -Display info for all pvd objects  \n\
pvdinfo -global_sniff_verify <enable or disable or status> Enable or Disable or Get gsv status. \n\
pvdinfo -single_loop_failure <enable or disable or null> Enable or Disable or Get slf_enabled. \n\
pvdinfo { -object_id <id> | -bes <b_e_s> }  Display info for particular object \n\
pvdinfo { -object_id <id> | -bes <b_e_s> } [-chunk_index <chunk index>] [-chunk_count <chunk count>] [-fua <0/1>] - Display metadata info and all other info. \n\
pvdinfo { -object_id <id> | -bes <b_e_s> } -paged_summary [-fua <0/1>] -  Display counts of chunks in various states. \n\
pvdinfo { -object_id <id> | -bes <b_e_s> } -paged_set_bits <chunk_index> <chunk_count> -  set metadata bits. \n\
pvdinfo { -object_id <id> | -bes <b_e_s> } -paged_clear_bits <chunk_index> <chunk_count> -  clear metadata bits. \n\
pvdinfo { -object_id <id> | -bes <b_e_s> } -paged_write <chunk_index> <chunk_count> -  write metadata bits. \n\
pvdinfo { -object_id <id> | -bes <b_e_s> } -clear_cache - Clear cached paged metadata. \n\
    paged_set_bits/clear_bits/write switches: \n\
        -valid -nz -uz -cons -reserved\n\
Note: All paged operation (set/clear bits, write) values are in Hexadecimal. \n\n\
Switches:\n\
  -global_sniff_verify or -gsv enable or disable or status\n\
  -object_id     - for Object Id [ This value should be in Hex]\n\
  -bes <b_e_s>   - bus_enclosure_slot format %d_%d_%d. Alternative to -object_id\n\
  -chunk_index   - for meta data chunk index [ This value should be in Decimal]\n\
  -chunk_count   - number of chunks to display.\n\
  -fua           - Force Unit Access (Default is 1 which gets data from disk. 0 - Allow data from cache)\n\
  -list          - display just object_id's and locations \n\
  -list_block_size - display configured physical block size\n\
  -map_lba [lba] - Map a PVD's lba to a chunk index\n\
  -single_loop_faiure or -slf enable or disable or null\n\
Example:\n\
pvdinfo\n\
pvdinfo -gsv enable \n\
pvdinfo -slf enable \n\
pvdinfo -bes 0_0_10 \n\
pvdinfo -object_id 0x109  \n\
pvdinfo -object_id 0x109  -chunk_index 2 \n\n\
pvdinfo -object_id 0x109  -chunk_index 2 -chunk_count 5\n\n\
pvdinfo -list \n\n\
pvdinfo -object_id 0x109 -paged_set_bits 2 4 -vaild 1 -nz 1 -uz 1 -cons 1 -reserved 0\n\n\
pvdinfo -object_id 0x109 -paged_clear_bits 2 4 -vaild 1 -nz 1 -uz 1 -cons 1 -reserved 0xff\n\n\
pvdinfo -object_id 0x109 -paged_write 2 4 -vaild 1 -nz 1 -uz 1 -cons 1 -reserved 0\n\n\
pvdinfo -object_id 0x109 -clear_cache \n\n\
\n"

#define REMOVE_ZERO_CHECKPOINT_USAGE "\
rmzerochkpt <pvd object id> Invalidate zero checkpoint ID for this PVD \n\
Switches:\n\
	-all - for all PVD's \n"
   
#define CLEAR_DRIVE_FAULT_USAGE \
"\npvdsrvc -h | -help\n"\
"pvdsrvc { -object_id <id> | -bes <b_e_s> } -mark_swap_pending <set/clear/status>\n"\
"pvdsrvc { -object_id <id> | -bes <b_e_s> } -drive_fault <clear/status>\n"\
"pvdsrvc { -object_id <id> | -bes <b_e_s> } -eol <set/clear/status>\n"\
"pvdsrvc { -object_id <id> | -bes <b_e_s> } -all_states <clear/status>\n"\
"pvdsrvc -object_id <all>                   -all_states <clear>\n"\
"pvdsrvc { -object_id <id> | -bes <b_e_s> } -set_block_size <value>\n"\
"pvdsrvc -object_id <all>                   -set_block_size <value>\n"\
"\n"\
"Switches:\n"\
"  -help                                    (display the usage of the PVD service functionality.)\n"\
"  -object_id <id | all>  (PVD object ID in hex.) See Notes below for the 'all' option\n"\
"  -bes <b_e_s>                             (bus_enclosure_slot format %d_%d_%d. Alternative to -object_id)\n"\
"  -mark_swap_pending <set/clear/status>    (set/clear the 'swap_pending' PVD state or \n"\
"                                            display current 'swap_pending' state.)\n"\
"  -drive_fault <clear/status>              (clear PVD drive fault state or\n"\
"                                            display current drive fault state.)\n"\
"  -eol <set/clear/status>                  (set/clear PVD end of life state or \n"\
"                                            display current end of life state.)\n"\
"  -all_states <clear/status>               (clear all offline faults: swap pending, drive fault and eol or \n"\
"                                            display current state of: swap pending, drive fault and eol)\n"\
"  -set_block_size <value>                  (sets PVD's physical block size. Used for testing ONLY\n"\
"Note: \n"\
"  '-object_id all' can only be used with the '-all_states clear' or 'set_block_size' option.\n"\
"\n"\
"Examples:\n"\
"pvdsrvc -object_id 0x104 -swap_pending set\n"\
"pvdsrvc -object_id 0x104 -drive_fault clear\n"\
"pvdsrvc -object_id 0x104 -eol clear\n"\
"pvdsrvc -object_id 0x104 -eol set\n"\
"pvdsrvc -object_id 0x104 -all_states clear\n"\
"pvdsrvc -object_id 0x104 -drive_fault status\n"\
"pvdsrvc -object_id all -all_states clear\n\n"\
"pvdsrvc -bes 0_0_10 -set_block_size 4160\n\n"\
"pvdsrvc -object_id all -set_block_size 4160\n\n"\

#define PVD_DESTROY_TIME_THRESHOLD_USAGE "\
	pvddeltime -h -help \n\
	pvddeltime -get  Get the time threshold for deleting PVD\n\
	pvddeltime -set <timeinminutes>  Set the time threshold for deleting PVD\n\
\n"

#define PVD_WEAR_LEVELING_TIMER_USAGE "\
pvdweartimer -h | -help \n\
pvdweartimer -set <time in seconds> \n\
\n"

/*************************
 * end file fbe_cli_lib_provision_drive_cmds.h
 *************************/

#endif /* end __FBE_CLI_LIB_PROVISION_DRIVE_CMDS_H__ */
