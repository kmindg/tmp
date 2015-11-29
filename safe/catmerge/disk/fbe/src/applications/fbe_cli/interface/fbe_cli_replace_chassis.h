#ifndef __FBE_CLI_REPLACECHASSIS_H__
#define __FBE_CLI_REPLACECHASSIS_H__
/***************************************************************************
 * Copyright (C) EMC Corporation 2015
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/
 
/*!*************************************************************************
 * @file fbe_cli_replace_chassis.h
 ***************************************************************************
 *
 * @brief
 *  This file contains REPLACE CHASSIS(rc) command cli definitions.
 *
 * @version
 * @ user guide: 
If wwnseed is consistent between system drives and not same with Chassis wwnseed, the system will go to service mode. 
In this situation, you can use svc_change_hw_config to restore the array.  In fact, svc_change_hw_config will use FBECLI>rc command to restore the array. 
Restore array steps as following:
(1) rc -update_wwn_seed  
    If wwnseed is consistent between system drives and user disks , this command will automatically update the chassis wwn_seed to system drive wwnseed successfully.  
(2) rc -update_wwn_seed   -force
    If one system drive is NEW DISK or NO DISK and wwnseed is consistent between the other two system drives and user disks, we still allow update the chassis wwn_seed successfully .
    step (1) can not be executed successfully for this case. 
(3) rc -get_system_drive_status
    If you meet any error in step(1) or step(2), you can check the original chassis wwn_seed and system drives status through this command. 
(4) rc -force_wwn_seed  <wwn_seed_value>
    If you meet any error in step(1) or step(2), It means it is not safe to update Chassis wwn_seed now.
    While, If you are very sure that you want to update chassis wwn_seed, you can update chassis wwn_seed to specific value using (4) command after checking system status with (3). 

 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/

#include "fbe/fbe_cli.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_job_service_interface.h"
#include "fbe_fru_descriptor_structure.h"

#define FBE_CLI_RC_USER_DISK_SAMPLE_COUNT 10
#define FBE_CLI_FAILED_IO_RETRIES 5
#define FBE_CLI_FAILED_IO_RETRY_INTERVAL 1000

typedef enum fbe_system_disk_type_e{
    FBE_HW_DISK_TYPE_INVALID = 0x0,
    FBE_HW_NO_DISK	= 0x1,
    FBE_HW_NEW_DISK = 0x2,
    FBE_HW_DISK_TYPE_UNKNOWN	= 0xFF,
}fbe_system_disk_type_t;
	
typedef struct{
    fbe_homewrecker_fru_descriptor_t  fru_descriptor;
    fbe_bool_t is_fru_descriptor_valid;
    fbe_fru_signature_t 	fru_sig;
    fbe_bool_t is_fru_signature_valid ;   
    fbe_system_disk_type_t disk_type;
    /*The (bus encl and slot) records where this system drive really loacated.*/
    fbe_u32_t actual_bus;
    fbe_u32_t actual_encl;
    fbe_u32_t actual_slot;
}homewrecker_system_disk_info_t;
	

/*************************
 *   FUNCTION DEFINITIONS
 *************************/


void fbe_cli_replace_chassis(int argc, char** argv);
static fbe_status_t fbe_cli_get_wwn_seed_from_userdisk(fbe_u64_t * wwn_user, fbe_u32_t *sample_count);
static void fbe_cli_display_std_info(homewrecker_system_disk_info_t* system_db_drive_table);
static fbe_status_t fbe_cli_get_system_drive_status(homewrecker_system_disk_info_t* system_db_drive_table);
static fbe_status_t fbe_cli_replace_chassis_process_cmds(fbe_bool_t isForce);
static fbe_status_t fbe_cli_set_wwn_seed_from_prom_resume(fbe_u32_t * wwn_seed);
//fbe_status_t fbe_cli_init_psl(void);
static fbe_status_t fbe_cli_get_fru_descriptor_lba(fbe_lba_t*  lba);
static fbe_status_t fbe_cli_get_fru_signature_lba(fbe_lba_t *lba);

#define RC_CMD_USAGE "\
Usage: rc -h\
       rc <operation>\n\
Operations:\n\
     -get_system_drive_status            - get Current Chassis wwn_seed and system drive info. \n\
     -update_wwn_seed                    - automatically update chassis wwn_seed according to system info.  \n\
     -update_wwn_seed -force             - automatically update chassis wwn_seed according to system info(allow one new drive in system slot). \n\
     -force_wwn_seed <wwn_seed_value>    - Forcely update chassis wwn_seed to specific wwn_seed_value.\n\
e.g.\n\
     rc -get_system_drive_status \n\
     rc -update_wwn_seed \n\
     rc -update_wwn_seed -force \n\
     rc -force_wwn_seed 0x47e0010d   \n" 

/*************************
 * end file fbe_cli_replace_chassis.h
 *************************/

#endif 
