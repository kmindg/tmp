#ifndef __FBE_CLI_SERVICES_H__
#define __FBE_CLI_SERVICES_H__
/***************************************************************************
 * Copyright (C) EMC Corporation 2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_cli_services.h
 ***************************************************************************
 *
 * @brief
 *  This file contains definitions of commands of FBE Cli related to Services
 *
 * @version
 *   11/09/2010:  Created. Vinay Patki
 *
 ***************************************************************************/

#include "global_catmerge_rg_limits.h"

// ---------------------------NEW STARTS------------------------

#define SERVICES_SETVERIFY_USAGE "\
setverify [switches]...\n\
General Switches:\n\
Usage: v -h\n\
\n\
    v -h                      - get help on usage\n\
\n\
    v <drive>              - Show Sniff Verify report for the drive\n\
    v <drive> [options - sniff verify]\n\
\n\
    v <lun>                 - Show Background Verify report for the drive\n\
    v <lun> [options - background verify]\n\
\n\
    v [options - general]\n\
\n\
General:\n\
    -rg\n\
    -rg # <options>**\n\
    -all\n\
    -all <options>**\n\
\n\
Options:\n\
    For Sniff Verify\n\
        -sne <1/0> enable/disable sniff verify\n\
        -cr  <1/0> clear/no all verify reports for this drive\n\
\n\
    For Background Verify\n\
        -bve       start background verify\n\
        -ro        start read only background verify (e.g. -bve -ro )\n\
        -system    start system verify\n\
        -incomplete_wr start incomplete write verify\n\
        -error     start error verify\n\
        Optional arguments for the above arguments\n\
            -lba        specify the start lba\n\
            -blocks     specify the block count\n\
        -cr  <1/0> clear/no all verify reports for this LU\n\
\n\
E.g\n\
v 0_0_6          - Getting report of SV of a drive\n\
v 0_1_2 -sne 0   - Set SV on a drive\n\
v -rg 3 -sne 1   - Set SV on all drives of a RG\n\
v 23             - Get report of BV of a lun\n\
v 23 -bve        - Set BV on a lun\n\
v 23 -bve -lba 0x100 -blocks 0x1000 - Set BV on a lun for 0x1000 blocks from lba 0x100\n\
v -all -bve      - Set BV on all the luns in system\n\
\n\
Note:\n\
  Clearing report on all drives/luns at once is under development.\n\
\n"

#define SERVICES_CONFIG_INVALIDATE_DB_USAGE "\
config_invalidate_db\n\
General Switches:\n\
  None\n\
\n\
E.g\n\
cidb\n\
config_invalidate_db\n\
\n"

#define SERVICES_CONFIG_EMV_USAGE "\
config_emv [switches]...\n\
General Switches:\n\
  -get\n\
  -set shared_emv_value( in unit of GB)  ctm(in unit of GB)\n\
\n\
E.g\n\
config_emv -get\n\
config_emv -set 32 0\n\
or\n\
cgemv -get\n\
cgemv -set 32 0\n\
\n"


#define SERVICES_BGO_ENABLE_USAGE "\
bgo_enable -disk <b_e_d> or -rg <rg num> -operation <code>\n\
or  \n\
bgo_enable -all <To enable all background operations>\n\
\n\
Usage:This command is used to enable background opeartion \n\
      on a specified drive or raid object\n\
\n\
General Operation code:\n\
    For -disk option:\n\
         PVD_DZ\n\
         PVD_SNIFF\n\
         PVD_ALL\n\
\n\
    For -rg option:\n\
         RG_MET_REB\n\
         RG_REB\n\
         RG_ERR_VER\n\
         RG_RW_VER\n\
         RG_RO_VER\n\
         RG_ENCRYPTION\n\
         RG_ALL\n\
\n"

#define SERVICES_BGO_DISABLE_USAGE "\
bgo_disable -disk <b_e_d> or -rg <rg num> -operation <code>\n\
or  \n\
bgo_disable -all <To disable all background operations>\n\
\n\
Usage:This command is used to disable backgroun opeartion \n\
      on a specified drive or raid object\n\
\n\
General Operation code:\n\
    For -disk option:\n\
         PVD_DZ\n\
         PVD_SNIFF\n\
         PVD_ALL\n\
\n\
    For -rg option:\n\
         RG_MET_REB\n\
         RG_REB\n\
         RG_ERR_VER\n\
         RG_RW_VER\n\
         RG_RO_VER\n\
         RG_ENCRYPTION\n\
         RG_ALL\n\
\n"

#define SERVICES_BGO_STATUS_USAGE "\
bgo_status -disk <b_e_d> or -rg <rg num> -operation <code> -get_bgo_flags\n\
\n\
Usage: This command is used to show the status of backgroun opeartions \n\
      on a specified drive or raid object\n\
\n\
General Operation code:\n\
    For -disk option:\n\
         PVD_DZ\n\
         PVD_SNIFF\n\
         PVD_ALL\n\
\n\
    For -rg option:\n\
         RG_MET_REB\n\
         RG_REB\n\
         RG_ERR_VER\n\
         RG_RW_VER\n\
         RG_RO_VER\n\
         RG_ENCRYPTION\n\
         RG_ALL\n\
\n"

#define SET_BG_OP_SPEED_USAGE "\n\
set_bg_op_speed,  it sets or read Background operation speed.\n\n\
*** Only change operation speed for test purpose. e.g. test setup etc and reset ***\n\
*** the speed to its default value before using the system for normal use. ***\n\
To set Background operation speed:\n\
set_bg_op_speed -operation <backgrond op> -speed <backgound op speed(in decimal)>\n\
To get Background operation speed:\n\
set_bg_op_speed -status\n\n\
Valid background operations:\n\
PVD_DZ (Disk zeroing)\n\
PVD_SNIFF (Disk Sniffing)\n\
RG_VER (Raid group Verify)\n\
RG_REB (Raid group Rebuild)\n\n\
Example:\n\
set_bg_op_speed -operation PVD_DZ -speed 100 \n\
set_bg_op_speed -status \n\n\
Default speed \n\
PVD_DZ    - 120 \n\
PVD_SNIFF -  10 \n\
RG_REB    - 120 \n\
RG_VER    -  10 \n\n\
\n"

#define SERVICES_CLUSTER_LOCK_USAGE "\
dlslock [-read|-release|-write]\n\
or  \n\
dls [-read|-release|-write]\n\
\n\
Usage: This command is used to exercise cluster lock. \n\
\n\
General Operation code:\n\
    -read     acquire read lock\n\
    -release  release lock\n\
    -write    acquire write lock\n\
\n"


/* Assuming maximum of 16 drives in R6 RG. */
#define MAX_NUM_DRIVES_IN_RG 16

/* This is defined in fbe_lun_private.h and should be changed 
 * when the value for this in that files changes 
 */
#define FBE_LUN_CHUNK_SIZE 0x800 /* in blocks ( 2048 blocks ) */

#define fbe_cli_lustat_line_size 512

#define FBE_RG_USER_COUNT 256
#define RAID_GROUP_COUNT 256
#define MAX_RAID_GROUP_ID GLOBAL_CATMERGE_RAID_GROUP_ID_MAX

#define MAX_LUN_ID 10240 /* assuming 10K LUN limit */

/*
 * This is to indicate whether bgs_halt command is to
 * be processed or bgs_unhalt command.
 */
typedef enum fbe_bgo_enable_disable_cmd_s
{
    FBE_BGO_ENABLE_CMD = 0,
    FBE_BGO_DISABLE_CMD,
    FBE_BGO_STATUS_CMD
}
fbe_bgo_enable_disable_cmd_t;

/*-------------------------
 * LUN background VERIFY LIMITS
 *
 * These are for testing. Must be removed before check-in.
 *-------------------------*/
#define FBE_LU_VERIFY_PRIORITY_ASAP     0
#define FBE_LU_VERIFY_PRIORITY_HIGH     6
#define FBE_LU_VERIFY_PRIORITY_MED      12
#define FBE_LU_VERIFY_PRIORITY_LOW      18
#define FBE_LU_VERIFY_PRIORITY_MAX      254
#define FBE_LU_MIN_VERIFY_PRIORITY      (FBE_LU_VERIFY_PRIORITY_ASAP)
#define FBE_LU_MAX_VERIFY_PRIORITY      (FBE_LU_VERIFY_PRIORITY_MAX)
#define FBE_LU_DEFAULT_VERIFY_PRIORITY  (FBE_LU_VERIFY_PRIORITY_MED)

#define FBE_LU_ASAP_ZERO_THROTTLE_RATE  0    //The defaulted background zeroing rate 0GB/Hr
#define FBE_LU_MIN_ZERO_THROTTLE_RATE   4    //The defaulted background zeroing rate 4GB/Hr
#define FBE_LU_MED_ZERO_THROTTLE_RATE   6    //The defaulted background zeroing rate 6GB/Hr
#define FBE_LU_MAX_ZERO_THROTTLE_RATE   12   //The defaulted background zeroing rate 12GB/Hr

#define FBE_LU_DEFAULT_ZERO_THROTTLE_RATE FBE_LU_MIN_ZERO_THROTTLE_RATE

#define FBE_JOB_SERVICE_WAIT_TIMEOUT       8000  /*ms*/


/*************************
 *   FUNCTION DEFINITIONS
 *************************/

void fbe_cli_bgo_enable(fbe_s32_t argc, fbe_s8_t ** argv);
void fbe_cli_bgo_disable(fbe_s32_t argc, fbe_s8_t ** argv);
void fbe_cli_bgo_status(fbe_s32_t argc, fbe_s8_t ** argv);
void fbe_cli_bgo_enable_disable_common(fbe_s32_t argc, fbe_s8_t ** argv,
                                       fbe_bgo_enable_disable_cmd_t process_halt_unhalt);
fbe_u8_t* fbe_cli_lib_bgo_get_command_usages(fbe_bgo_enable_disable_cmd_t command);
void fbe_cli_lib_bgo_enable_disable(fbe_bgo_enable_disable_cmd_t command,
                                    fbe_object_id_t obj_id,fbe_u32_t  operation_code,
                                    fbe_u8_t* operation_code_string);
fbe_u32_t fbe_cli_lib_bgo_get_operation_code(fbe_s8_t * argv);
void fbe_cli_lib_bgo_enable_disable_all(fbe_bgo_enable_disable_cmd_t command,
                                        fbe_u8_t* usages_string);
fbe_bool_t fbe_cli_lib_validate_operation(fbe_u8_t* operation_code_string,fbe_u32_t rg);
void fbe_cli_set_bg_op_speed(fbe_s32_t argc, fbe_s8_t ** argv);

void fbe_cli_setverify(fbe_s32_t argc , fbe_s8_t** argv);
void fbe_cli_dispatch_verify(fbe_object_id_t verify_obj_id,
                          fbe_bool_t is_group,
                          fbe_bool_t get_report,
                          FBE_TRI_STATE sniffs_enabled,
                          fbe_bool_t is_sv,
                          fbe_lun_verify_type_t do_verify,
                          fbe_lba_t lba,
                          fbe_block_count_t blocks,
                          fbe_bool_t is_bv,
                          fbe_bool_t clear_reports,
                          FBE_TRI_STATE user_sniff_control_enabled);
void fbe_cli_send_group_verify_cmd(fbe_u32_t group,
                          FBE_TRI_STATE sniffs_enabled,
                                   fbe_lun_verify_type_t do_verify,
                                   fbe_bool_t clear_reports,
                                   FBE_TRI_STATE user_sniff_control_enabled);
void fbe_cli_print_group_verify_report(fbe_object_id_t rg_id);
void fbe_cli_print_sniff_verify_report(fbe_object_id_t verify_obj_id);
void fbe_cli_print_bg_verify_report(fbe_lun_number_t lun_num,fbe_lun_verify_report_t *lun_verify_report_p);
void fbe_cli_print_bg_verify_counts(fbe_lun_verify_counts_t *lun_verify_counts_p);
void fbe_cli_set_sniff_verify_service(fbe_object_id_t verify_obj_id, FBE_TRI_STATE sniffs_enabled);
void fbe_cli_print_bgo_flags(void);

/* for config service */
void fbe_cli_config_invalidate_db(fbe_s32_t argc, fbe_s8_t ** argv);
void fbe_cli_config_emv(fbe_s32_t argc, fbe_s8_t ** argv);

void fbe_cli_cluster_lock(fbe_s32_t argc , fbe_s8_t** argv);



#endif /* end __FBE_CLI_SERVICES_H__ */
/*************************
 * end file fbe_cli_services.h
 *************************/
