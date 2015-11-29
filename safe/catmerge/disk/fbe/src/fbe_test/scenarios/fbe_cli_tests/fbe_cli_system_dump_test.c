/***************************************************************************
* Copyright (C) EMC Corporation 2010
* All rights reserved.
* Licensed material -- property of EMC Corporation
***************************************************************************/

/***************************************************************************
* fbe_cli_systemdump_test.c
***************************************************************************
 *
 * @brief
 *  This file contains test functions for testing luninfo cli commands
 * 
 * @ingroup fbe_cli_test
 *
 * @date
 *  7/9/2012 - Created - Zhang Yang.
 *
***************************************************************************/
/*************************
 *   INCLUDE FILES
 *************************/

#include "fbe_cli_tests.h"
#include "fbe/fbe_api_sim_transport.h"
#include "sep_utils.h"
#include "sep_tests.h"
#include "fbe_test_configurations.h"
#include "fbe_database.h"
#include "fbe/fbe_api_trace_interface.h"
#include "fbe/fbe_api_utils.h"
#include "fbe/fbe_api_common.h"
#include "fbe_private_space_layout.h"
#include "fbe/fbe_api_database_interface.h"
#include "fbe/fbe_api_provision_drive_interface.h"


char * systemdump_test_short_desc = "Test system_backup_restore cli commands";
char * systemdump_test_long_desc ="\
Test Description: Test system_backup_restore command having following syntax\n\
system_backup_restore -backup -file\n\
system_backup_restore -backup \n\
system_backup_restore -restore -file -system_db_header\n\
system_backup_restore -restore -system_db_header\n\
\n\
Starting Config:\n\
        [PP] armada board\n\
        [PP] SAS PMC port\n\
        [PP] 3 viper enclosure\n\
        [PP] 3 SAS drives/enclosure\n\
        [PP] 3 logical drives/enclosure\n\
\n\
STEP 1: Bring up the initial topology.\n\
        - Create and verify the initial physical package config.\n\
STEP 2: Start fbecli application for each command\n\
STEP 3: Check expected output and unexpected output\n\
" ;

/*!*******************************************************************
 * @def FBE_CLI_CMDS_LUNINFO
 *********************************************************************
 * @brief Number of system_backup_restore commands to test
 *
 *********************************************************************/
#define FBE_CLI_CMDS_SYSTEMDUMP_USAGE           2
#define FBE_CLI_CMDS_SYSTEMDUMP_BACKUP           1
#define FBE_CLI_CMDS_SYSTEMDUMP_RESTORE         6
#define FBE_CLI_CMDS_SYSTEMDUMP_CORRUPT           3

/* fbe cli system_backup_restore commands and keyword array. */
/*!*******************************************************************
 * @def cli_cmd_and_keywords
 *********************************************************************
 * @brief SYSTEMDUMP commands with keywords
 *
 *********************************************************************/
#if 0
static fbe_cli_cmd_t cli_cmd_and_keywords_usage[FBE_CLI_CMDS_SYSTEMDUMP_USAGE] =
{
    {
        "system_backup_restore",
        {"system_backup_restore or sysbr -  operations about system dump file"}, 
        {"EEROR: invalidate args"} 
    },
    {
        "system_backup_restore -help",
        {"system_backup_restore or sysbr -  operations about system dump file"}, 
        {"EEROR: invalidate args"} 
    },
};
#endif
static fbe_cli_cmd_t cli_cmd_and_keywords_bakcup[FBE_CLI_CMDS_SYSTEMDUMP_BACKUP] =
{

    {
        "sysbr -backup",
        {"backup success!"}, 
        {"Failed to write section into backup file"} 
    },
};
static fbe_cli_cmd_t cli_cmd_and_keywords_restore[FBE_CLI_CMDS_SYSTEMDUMP_RESTORE] =
{
    {
        "sysbr -restore -SysDBHeader",
		{"System db header restore successfully!"},
        {"Failed to persist system db header to disk"} 
    },
    {
        "sysbr -restore -SysDB",
        {"System db restore successfully!"}, 
        {"Failed to persist system db to disk"} 
    },
	{
		"sysbr -restore -User",
		{"User config restore successfully!"}, 
		{"Failed to restore user config "} 
	},

    {
        "sysbr -restore -SysNP",
        {"System NP restore successfully!"}, 
        {"Failed to restore system NP "} 
    },
    
    {
        "sysbr -restore -UserNP",
        {"User NP restore successfully!"}, 
        {"Failed to restore user NP "} 
    },
	{
        "sysbr -restore -all",
        {
         "System db header restore successfully!",
		 "System db restore successfully!",
		 "User config restore successfully!",
		 "System NP restore successfully!",
		 "User NP restore successfully!"
		}, 
        {
         "Failed to persist system db header to disk",
         "Failed to persist system db to disk",
         "Failed to restore user config ",
         "Failed to restore system NP ",
         "Failed to restore user NP "
		} 
    },
};
static fbe_cli_cmd_t cli_cmd_and_keywords_corrupt[FBE_CLI_CMDS_SYSTEMDUMP_CORRUPT] =
{

    {
        "sysbr -Systemtest",
        {
         "Corrupt the system db header",
		 "Corrupt the system db lun"
		}, 
        {
         "Failed to corrupt the system db header",
		 "Failed to corrupt the system db lun"
		} 
    },
    {
        "sysbr -Usertest",
        {
		 "Corrupt the db lun"
		}, 
        {
		 "Failed to corrupt the db lun"
		} 
    },
    {
        "sysbr -Systemtest -Sysdb",
        {
		 "Corrupt the system db lun"
		}, 
        {
		 "Failed to corrupt the system db lun"
		} 
    },
};

/*!*******************************************************************
 * @def FBECLI_SYSTEM_DUMP_LUNS_PER_RAID_GROUP
 *********************************************************************
 * @brief Max number of LUNs for each raid group.
 *
 *********************************************************************/
#define FBECLI_SYSTEM_DUMP_LUNS_PER_RAID_GROUP 1

/*!*******************************************************************
 * @def FBECLI_SYSTEM_DUMP_CHUNKS_PER_LUN
 *********************************************************************
 * @brief Number of chunks each LUN will occupy.
 *
 *********************************************************************/
#define FBECLI_SYSTEM_DUMP_CHUNKS_PER_LUN 3

/*!*******************************************************************
 * @def FBECLI_SYSTEM_DUMP_RAID_TYPE_COUNT
 *********************************************************************
 * @brief Number of separate configs we will setup.
 *
 *********************************************************************/
#define FBECLI_SYSTEM_DUMP_RAID_TYPE_COUNT 2

/*!*******************************************************************
 * @var fbecli_luninfo_raid_group_config_qual
 *********************************************************************
 * @brief This is the array of configurations we will loop over.
 *
 *********************************************************************/
#ifdef ALAMOSA_WINDOWS_ENV
fbe_test_rg_configuration_array_t fbecli_systemdump_raid_group_config_qual[FBE_TEST_RG_CONFIG_ARRAY_MAX_TYPE] = 
#else
fbe_test_rg_configuration_array_t fbecli_systemdump_raid_group_config_qual[] = 
#endif /* ALAMOSA_WINDOWS_ENV - ODDCASE - shrink table/executable size */
{
    /* Raid 1 configurations.
     */
    {
        /* width, capacity     raid type,                  class,                  block size      RAID-id.    bandwidth.*/
        {2,       0xE000,      FBE_RAID_GROUP_TYPE_RAID1,  FBE_CLASS_ID_MIRROR,    512,            0,         0},
        {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
    },
    /* Raid 5 configurations.
     */
    {
        /* width, capacity     raid type,                  class,                  block size      RAID-id.    bandwidth.*/
        {3,       0xE000,      FBE_RAID_GROUP_TYPE_RAID5,  FBE_CLASS_ID_PARITY,    520,            1,         0},
        {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
    },
    {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
};


/**************************************
 * end fbecli_luninfo_raid_group_config_qual()
 **************************************/
static void fbe_cli_destroy_system_db_header_and_db_lun(void);
static void fbe_cli_restore_system_db_header_and_db_lun(void);
static void fbe_cli_systemdump_create_rg(void);
static void fbe_cli_back_up(void);
static void fbe_cli_destroy_user_config(void);
static void fbe_cli_restore_user_config(void);
static void fbe_cli_restore_system_NP_and_user_NP(void);
static void fbe_cli_destroy_system_db_lun(void);
/*!**************************************************************
 * fbe_test_load_systemdump_config()
 ****************************************************************
 * @brief
 *  Setup for a system dump test.
 *
 * @param array_p.               
 *
 * @return None.   
 *
 * @revision
 *
 ****************************************************************/
void fbe_test_load_systemdump_config(fbe_test_rg_configuration_array_t* array_p)
{
    fbe_u32_t raid_type_count = fbe_test_sep_util_rg_config_array_count(array_p);
    fbe_u32_t test_index;

    for (test_index = 0; test_index < raid_type_count; test_index++)
    {
        fbe_test_sep_util_init_rg_configuration_array(&array_p[test_index][0]);
    }
    fbe_test_sep_util_rg_config_array_load_physical_config(array_p);
    elmo_load_sep_and_neit();
    return;
}

/******************************************
 * end fbe_test_load_systemdump_config()
 ******************************************/


/*!**************************************************************
 * fbe_cli_systemdump_setup()
 ****************************************************************
 * @brief
 *  Setup function for system_backup_restore command execution.
 *
 * @param None.               
 *
 * @return None.   
 *
 * @revision
 *
 ****************************************************************/
void fbe_cli_systemdump_setup(void)
{
    fbe_test_rg_configuration_array_t *array_p = NULL;
    /* Qual.
     */
    array_p = &fbecli_systemdump_raid_group_config_qual[0];
    /* Config test to be done under SPA*/
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);

    /* Set up fbecli to run in interactive mode with option "s a"*/
    init_test_control(default_cli_executable_name,default_cli_script_file_name,CLI_DRIVE_TYPE_SIMULATOR, CLI_SP_TYPE_SPA,CLI_MODE_TYPE_INTERACTIVE);

    /* Load "rginfo" specific test config*/
    fbe_test_load_systemdump_config(array_p);
    return;
}
/******************************************
 * end fbe_cli_systemdump_setup()
 ******************************************/

/*!**************************************************************
 * fbe_cli_systemdump_test()
 ****************************************************************
 * @brief
 *  Test function for system_backup_restore command execution.
 *
 * @param None.               
 *
 * @return None.   
 *
 * @revision
 *
 ****************************************************************/
void fbe_cli_systemdump_test(void)
{
	fbe_cli_systemdump_create_rg();
	fbe_cli_back_up();
	fbe_cli_restore_system_db_header_and_db_lun();
	//fbe_cli_restore_user_config();
	//fbe_cli_restore_system_NP_and_user_NP();
    return;
}
/******************************************
 * end fbe_cli_systemdump_test()
 ******************************************/
static void fbe_cli_systemdump_create_rg(void)
{
    fbe_status_t                 status;
    fbe_u32_t                   raid_type_index = 1;
	fbe_job_service_error_type_t job_error_code;
	fbe_status_t  job_status;
	fbe_object_id_t             rg_object_id = FBE_OBJECT_ID_INVALID;

	for (raid_type_index = 0; raid_type_index < FBECLI_SYSTEM_DUMP_RAID_TYPE_COUNT; raid_type_index++)
	{
		status = fbe_test_sep_util_create_raid_group(&fbecli_systemdump_raid_group_config_qual[raid_type_index][0]);
		MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "RG creation failed");
	
		// wait for notification from job service. 
		status = fbe_api_common_wait_for_job(fbecli_systemdump_raid_group_config_qual[raid_type_index][0].job_number,
											30000,
											&job_error_code,
											&job_status,
											NULL);
	
		MUT_ASSERT_TRUE_MSG((status != FBE_STATUS_TIMEOUT), "Timed out waiting to created RG");
		MUT_ASSERT_TRUE_MSG((job_status == FBE_STATUS_OK), "RG creation failed");
	
		// Verify the object id of the raid group 
		status = fbe_api_database_lookup_raid_group_by_number(fbecli_systemdump_raid_group_config_qual[raid_type_index][0].raid_group_id, &rg_object_id);
	
		MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
		MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, rg_object_id);
	
		// Verify the raid group get to ready state in reasonable time 
		status = fbe_api_wait_for_object_lifecycle_state(rg_object_id, 
														FBE_LIFECYCLE_STATE_READY, 30000,
														FBE_PACKAGE_ID_SEP_0);
	
		MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
		mut_printf(MUT_LOG_TEST_STATUS, "Created Raid Group object %d\n", rg_object_id);
	}
    return;
}
static void fbe_cli_back_up(void)
{
    fbe_bool_t                   b_result;
	mut_printf(MUT_LOG_TEST_STATUS, "Start system back up\n");
	/*Test backup CLI*/
	b_result = fbe_cli_base_test(cli_cmd_and_keywords_bakcup[0]);
	MUT_ASSERT_TRUE(b_result);

}
static void fbe_cli_restore_system_db_header_and_db_lun(void)
{
    fbe_status_t                 status;
    fbe_bool_t                   b_result;
	fbe_database_state_t         out_db_state;

	mut_printf(MUT_LOG_TEST_STATUS, "force sep goes into service mode.....");

    /*destroy system db header and system db*/
	//fbe_cli_destroy_system_db_header_and_db_lun();
	fbe_cli_destroy_system_db_lun();
    //reboot
    mut_printf(MUT_LOG_TEST_STATUS, "Now reboot the system ......\n");
	
	fbe_api_trace_reset_error_counters(FBE_PACKAGE_ID_SEP_0);
    fbe_test_sep_util_destroy_neit_sep();
    sep_config_load_sep_and_neit();

	/*make sure the db is ready after we restored the system db header*/
	status = fbe_api_database_get_state(&out_db_state);
	MUT_ASSERT_INT_EQUAL(out_db_state, FBE_DATABASE_STATE_SERVICE_MODE);
	
		
	mut_printf(MUT_LOG_TEST_STATUS, "CLI restore test start.....");
	/*Test restore CLI*/
	b_result = fbe_cli_base_test(cli_cmd_and_keywords_restore[0]);
	MUT_ASSERT_TRUE(b_result);
	
	//b_result = fbe_cli_base_test(cli_cmd_and_keywords_restore[1]);
	MUT_ASSERT_TRUE(b_result);
	mut_printf(MUT_LOG_TEST_STATUS, "Reboot system.....");
	/*reboot the system*/
	/*we need to clear the error trace account to let 
	   sep boot successfully*/
	fbe_api_trace_reset_error_counters(FBE_PACKAGE_ID_SEP_0);
	fbe_test_sep_util_destroy_neit_sep();
	sep_config_load_sep_and_neit();
	status = fbe_test_sep_util_wait_for_database_service(30000);
	MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
	/*make sure the db is ready after we restored the system db header*/
	status = fbe_api_database_get_state(&out_db_state);
	MUT_ASSERT_INT_EQUAL(out_db_state, FBE_DATABASE_STATE_READY);
	mut_printf(MUT_LOG_TEST_STATUS, "Restore System config done \n");
    return;

}
static void fbe_cli_restore_user_config(void)
{
    fbe_status_t                 status;
    fbe_bool_t                   b_result;
	fbe_object_id_t             rg_object_id = FBE_OBJECT_ID_INVALID;

	mut_printf(MUT_LOG_TEST_STATUS, "Clean up database lun.....");

    /*destroy  db lun*/
	fbe_cli_destroy_user_config();
	
    //reboot
    mut_printf(MUT_LOG_TEST_STATUS, "Now reboot the system ......\n");
    fbe_test_sep_util_destroy_neit_sep();
    sep_config_load_sep_and_neit();
	status = fbe_test_sep_util_wait_for_database_service(30000);
	MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

	// Verify the object id of the raid group  is invalidate
	status = fbe_api_database_lookup_raid_group_by_number(fbecli_systemdump_raid_group_config_qual[0][0].raid_group_id, &rg_object_id);
	
	MUT_ASSERT_INT_NOT_EQUAL(FBE_STATUS_OK, status);
	MUT_ASSERT_INT_EQUAL(FBE_OBJECT_ID_INVALID, rg_object_id);
	
	status = fbe_api_database_lookup_raid_group_by_number(fbecli_systemdump_raid_group_config_qual[1][0].raid_group_id, &rg_object_id);
	
	MUT_ASSERT_INT_NOT_EQUAL(FBE_STATUS_OK, status);
	MUT_ASSERT_INT_EQUAL(FBE_OBJECT_ID_INVALID, rg_object_id);
    mut_printf(MUT_LOG_TEST_STATUS, "Raid Group lost, now cleanup and re-init to fix it\n");
	
	/*Test restore CLI*/
	b_result = fbe_cli_base_test(cli_cmd_and_keywords_restore[2]);
	MUT_ASSERT_TRUE(b_result);
	
	mut_printf(MUT_LOG_TEST_STATUS, "Reboot system.....");
	/*reboot the system*/
	fbe_test_sep_util_destroy_neit_sep();
	sep_config_load_sep_and_neit();
	status = fbe_test_sep_util_wait_for_database_service(30000);
	MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
	
	// Verify the object id of the raid group 
	status = fbe_api_database_lookup_raid_group_by_number(fbecli_systemdump_raid_group_config_qual[0][0].raid_group_id, &rg_object_id);
	
	MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
	MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, rg_object_id);
	
	// Verify the raid group get to ready state in reasonable time 
	status = fbe_api_wait_for_object_lifecycle_state(rg_object_id, 
													FBE_LIFECYCLE_STATE_READY, 30000,
													FBE_PACKAGE_ID_SEP_0);
	
	MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
	
	// Verify the object id of the raid group 
	status = fbe_api_database_lookup_raid_group_by_number(fbecli_systemdump_raid_group_config_qual[1][0].raid_group_id, &rg_object_id);
	
	MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
	MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, rg_object_id);
	
	// Verify the raid group get to ready state in reasonable time 
	status = fbe_api_wait_for_object_lifecycle_state(rg_object_id, 
													FBE_LIFECYCLE_STATE_READY, 30000,
													FBE_PACKAGE_ID_SEP_0);
	
	MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
	mut_printf(MUT_LOG_TEST_STATUS, "Restore User config done \n");
    return;

}
static void fbe_cli_restore_system_NP_and_user_NP(void)
{
    fbe_status_t                 status;
    fbe_bool_t                   b_result;
	fbe_object_id_t              rg_object_id = FBE_OBJECT_ID_INVALID;
	fbe_object_id_t              system_pvd_id = FBE_OBJECT_ID_INVALID;
	
	mut_printf(MUT_LOG_TEST_STATUS, "Restore system NP and User NP test start.....");
	/*Test restore CLI*/
	b_result = fbe_cli_base_test(cli_cmd_and_keywords_restore[3]);
	MUT_ASSERT_TRUE(b_result);
	
	b_result = fbe_cli_base_test(cli_cmd_and_keywords_restore[4]);
	MUT_ASSERT_TRUE(b_result);
	mut_printf(MUT_LOG_TEST_STATUS, "Reboot system.....");
	/*reboot the system*/
	fbe_test_sep_util_destroy_neit_sep();
	sep_config_load_sep_and_neit();
	status = fbe_test_sep_util_wait_for_database_service(30000);
	MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
	
	// Verify the object id of the raid group 
	status = fbe_api_database_lookup_raid_group_by_number(fbecli_systemdump_raid_group_config_qual[0][0].raid_group_id, &rg_object_id);
	
	MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
	MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, rg_object_id);
	
	// Verify the raid group get to ready state in reasonable time 
	status = fbe_api_wait_for_object_lifecycle_state(rg_object_id, 
													FBE_LIFECYCLE_STATE_READY, 30000,
													FBE_PACKAGE_ID_SEP_0);
	
	MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
	
	// Verify the object id of the raid group 
	status = fbe_api_database_lookup_raid_group_by_number(fbecli_systemdump_raid_group_config_qual[1][0].raid_group_id, &rg_object_id);
	
	MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
	MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, rg_object_id);
	
	// Verify the raid group get to ready state in reasonable time 
	status = fbe_api_wait_for_object_lifecycle_state(rg_object_id, 
													FBE_LIFECYCLE_STATE_READY, 30000,
													FBE_PACKAGE_ID_SEP_0);
	
	MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
	
	// Verify the object id of system pvd 
    status = fbe_api_provision_drive_get_obj_id_by_location(0,
                                                            0,
                                                            1,
                                                            &system_pvd_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
	MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, system_pvd_id);
	
	// Verify the pvd get to ready state in reasonable time 
	status = fbe_api_wait_for_object_lifecycle_state(system_pvd_id, 
													FBE_LIFECYCLE_STATE_READY, 30000,
													FBE_PACKAGE_ID_SEP_0);
	
	MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
	mut_printf(MUT_LOG_TEST_STATUS, "Restore system NP and user NP done \n");
    return;

}

static void fbe_cli_destroy_system_db_header_and_db_lun(void)
{
    fbe_bool_t  b_result = FBE_FALSE;
	/*destroy the system db header*/
	b_result = fbe_cli_base_test(cli_cmd_and_keywords_corrupt[0]);
	MUT_ASSERT_TRUE(b_result);
	
    return;
}
static void fbe_cli_destroy_system_db_lun(void)
{
    fbe_bool_t  b_result = FBE_FALSE;
	/*destroy the system db header*/
	b_result = fbe_cli_base_test(cli_cmd_and_keywords_corrupt[2]);
	MUT_ASSERT_TRUE(b_result);
	
    return;
}

static void fbe_cli_destroy_user_config(void)
{
    fbe_bool_t  b_result = FBE_FALSE;
	/*destroy the system db header*/
	b_result = fbe_cli_base_test(cli_cmd_and_keywords_corrupt[1]);
	MUT_ASSERT_TRUE(b_result);
	
    return;
}

/*!**************************************************************
 * fbe_cli_systemdump_teardown()
 ****************************************************************
 * @brief
 *  Teardown function for system_backup_restore command execution.
 *
 * @param None.               
 *
 * @return None.   
 *
 * @revision
 *
 ****************************************************************/
void fbe_cli_systemdump_teardown(void)
{
    fbe_test_sep_util_disable_trace_limits();
    fbe_test_sep_util_destroy_neit_sep_physical();
    
    /* Reset test to be done under SPA */
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);

    return;
}

/******************************************
 * end fbe_cli_systemdump_teardown()
 ******************************************/
/*************************
 * end file fbe_cli_luninfo_test.c
 *************************/

