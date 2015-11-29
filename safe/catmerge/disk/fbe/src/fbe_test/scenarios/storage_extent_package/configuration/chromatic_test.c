/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file chromatic_test.c
 ***************************************************************************
 *
 * @brief
 *  Send certain packet to database service to test. 
 *
 * @version
 *  10/8/2011 - Created. He Wei
 *  10/12/2011 - Add Test. zphu
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "mut.h"   
#include "sep_tests.h"
#include "pp_utils.h"
#include "fbe/fbe_api_database_interface.h"
#include "fbe/fbe_api_common.h"
#include "fbe_test_common_utils.h"
#include "fbe_test_configurations.h"
#include "fbe/fbe_api_database_interface.h"
#include "fbe_private_space_layout.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_api_utils.h"
#include "fbe/fbe_api_provision_drive_interface.h"
#include "fbe/fbe_api_job_service_interface.h"
#include "fbe/fbe_api_sim_server.h"
#include "fbe/fbe_api_physical_drive_interface.h"
#include "fbe/fbe_api_protocol_error_injection_interface.h"
#include "fbe/fbe_api_raid_group_interface.h"
#include "fbe/fbe_api_trace_interface.h"
#include "sep_rebuild_utils.h"
#include "neit_utils.h"

#define CHROMATIC_TEST_OBJECT_ID_MCR_DATABASE_LUN	FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_MCR_DATABASE
#define CHROMATIC_TEST_WAIT_MSEC					30000
#define CHROMATIC_TEST_MAX_RETRY_COUNT                         800

#define CHROMATIC_TEST_DRIVE_CAPACITY             	TERMINATOR_MINIMUM_SYSTEM_DRIVE_CAPACITY
#define CHROMATIC_TEST_RAW_MIRROR_REBUILD_TIMEOUT       600
#define CHROMATIC_TEST_NUMBER_OF_PHYSICAL_OBJECTS       66


static void chromatic_test_system_reboot(void);
void chromatic_dualsp_setup(void);
void chromatic_dualsp_cleanup(void);
void chromatic_dualsp_test(void);
static void chromatic_load_physical_config(void);
static fbe_status_t chromatic_wait_system_raid_and_pvd_connection(fbe_u32_t  object_count, fbe_object_id_t pvd_id);

/*test to handle raw-mirror read write errors*/
void chromatic_raw_mirror_error_handling_test_phase1(void);
void chromatic_raw_mirror_error_handling_test_phase2(void);
void chromatic_raw_mirror_write_user_entry(fbe_object_id_t object_id, database_user_entry_t *entry);
void chromatic_raw_mirror_read_user_entry(fbe_object_id_t object_id, database_user_entry_t *entry);
static void chromatic_test_inject_error_record(
                                    fbe_u16_t                               position_bitmap,
                                    fbe_lba_t                               lba,
                                    fbe_block_count_t                       num_blocks,
                                    fbe_api_logical_error_injection_type_t  error_type,
                                    fbe_u32_t                               num_objects_enabled,
                                    fbe_u32_t                               num_records,
                                    fbe_u16_t                               err_adj_bitmask);

static fbe_status_t chromatic_inject_errors_to_system_drive(fbe_u32_t fail_drives);
void chromatic_get_raw_mirror_error_report(fbe_database_control_raw_mirror_err_report_t *report);
void chromatic_wait_raw_mirror_rebuild_complete(fbe_u32_t timeout, fbe_u32_t down_disk_mask,
                                                fbe_u32_t degraded_disk_mask);
void chromatic_raw_mirror_io_test(void);

static fbe_status_t chromatic_rebuild_wait_for_rb_start_by_obj_id(
                                    fbe_object_id_t                 in_raid_group_object_id,
                                    fbe_u32_t                       in_position,
                                    fbe_u32_t  limit);
static fbe_status_t chromatic_rebuild_verify_rb_logging_by_object_id(
                                        fbe_object_id_t                 in_raid_group_object_id,
                                        fbe_u32_t                       in_position);
fbe_status_t chromatic_test_wait_system_pvd_state(fbe_u32_t port_number,
                                             fbe_u32_t enclosure_number,
                                             fbe_u32_t slot_number, 
                                             fbe_lifecycle_state_t state);

void chromatic_raw_mirror_bitmask_test(void);

fbe_u32_t chromatic_get_num_of_upstream_objects(fbe_u32_t  port_number,
                                                fbe_u32_t  enclosure_number,
                                                fbe_u32_t  slot_number);

static fbe_status_t chromatic_test_wait_rg_rebuild_complete(fbe_object_id_t raid_group_object_id, fbe_u32_t rb_position);


char * chromatic_test_short_desc = "Test Database System DB and Raw-mirror Error handling";
char * chromatic_test_long_desc ="\
Test Database System DB and Raw-mirror Error handling\n\
\n\
Dependencies:\n\
        ...\n\
\n";

typedef enum chromatic_test_enclosure_num_e
{
    CHROMATIC_TEST_ENCL1_WITH_SAS_DRIVES = 0,
    CHROMATIC_TEST_ENCL2_WITH_SAS_DRIVES,
    CHROMATIC_TEST_ENCL3_WITH_SAS_DRIVES,
    CHROMATIC_TEST_ENCL4_WITH_SAS_DRIVES
} chromatic_test_enclosure_num_t;

typedef enum chromatic_test_drive_type_e
{
    SAS_DRIVE,
    SATA_DRIVE
} chromatic_test_drive_type_t;

/* This is a set of parameters for the Scoobert spare drive selection tests.
 */
typedef struct enclosure_drive_details_s
{
    chromatic_test_drive_type_t drive_type; /* Type of the drives to be inserted in this enclosure */
    fbe_u32_t port_number;                    /* The port on which current configuration exists */
    chromatic_test_enclosure_num_t  encl_number;      /* The unique enclosure number */
    fbe_block_size_t block_size;
} enclosure_drive_details_t;
/* Table containing actual enclosure and drive details to be inserted.
 */
static enclosure_drive_details_t encl_table[] = 
{
    {   SAS_DRIVE,
        0,
        CHROMATIC_TEST_ENCL1_WITH_SAS_DRIVES,
        520
    },
    {   SAS_DRIVE,
        0,
        CHROMATIC_TEST_ENCL2_WITH_SAS_DRIVES,
        520
    },
    {   SAS_DRIVE,
        0,
        CHROMATIC_TEST_ENCL3_WITH_SAS_DRIVES,
        520
    },
    {   SAS_DRIVE,
        0,
        CHROMATIC_TEST_ENCL4_WITH_SAS_DRIVES,
        520
    },
};

#define CHROMATIC_TEST_ENCL_MAX (sizeof(encl_table)/sizeof(encl_table[0]))

static fbe_sim_transport_connection_target_t chromatic_dualsp_pass_config[] =
{
    FBE_SIM_SP_A, FBE_SIM_SP_B,
};

#define CHROMATIC_MAX_PASS_COUNT (sizeof(chromatic_dualsp_pass_config)/sizeof(chromatic_dualsp_pass_config[0]))


/*********************************************************************************
 * BEGIN Test, Setup, Cleanup.
 *********************************************************************************/
/*!**************************************************************
 * chromatic_test()
 ****************************************************************
 * @brief
 *  Run injection test.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  10/8/2011 
 *
 ****************************************************************/
void chromatic_phase1_test(void)
{
    mut_printf(MUT_LOG_TEST_STATUS, "Start chromatic_phase1_test\n");

	/*test database raw-mirror read/write error handling*/
    chromatic_raw_mirror_error_handling_test_phase1();

	mut_printf(MUT_LOG_TEST_STATUS, "End chromatic_phase1 test\n");
}

void chromatic_phase2_test(void)
{
	fbe_status_t status = FBE_STATUS_OK;
	fbe_database_control_system_db_op_t system_db_cmd;

    mut_printf(MUT_LOG_TEST_STATUS, "Start chromatic_phase2_test\n");

    mut_printf(MUT_LOG_TEST_STATUS, "Verify the system db entry persist and boot in backend\n");
	fbe_zero_memory(&system_db_cmd, sizeof (fbe_database_control_system_db_op_t));
    system_db_cmd.cmd = FBE_DATABASE_CONTROL_SYSTEM_DB_VERIFY_BOOT;
    status = fbe_api_database_system_db_send_cmd(&system_db_cmd);
	MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "System db entry persist verify done and passed \n");

	/*test database raw-mirror read/write error handling*/
    chromatic_raw_mirror_error_handling_test_phase2();
    #if 0
    /*test the bitmask in raw mirror*/
	chromatic_raw_mirror_bitmask_test();
    #endif

	mut_printf(MUT_LOG_TEST_STATUS, "End chromatic phase2 test\n");
}

/*for dualsp chromatic test*/
void chromatic_dualsp_test(void)
{
	fbe_sim_transport_connection_target_t active_target;
	fbe_u32_t pass;
	fbe_status_t status = FBE_STATUS_OK;
	/* Enable dualsp mode
	 */
	fbe_test_sep_util_set_dualsp_test_mode(FBE_TRUE);

	mut_printf(MUT_LOG_TEST_STATUS, "Start chromatic_dualsp_test\n");
	
	for (pass = 0 ; pass < CHROMATIC_MAX_PASS_COUNT; pass++)
	{
        
            fbe_test_sep_get_active_target_id(&active_target);
            MUT_ASSERT_INT_NOT_EQUAL(FBE_SIM_INVALID_SERVER, active_target);
            mut_printf(MUT_LOG_TEST_STATUS, "=== DUAL SP test pass %d: Active SP is %s ===\n", pass,
                   active_target == FBE_SIM_SP_A ? "SP A" : "SP B");

            fbe_api_sim_transport_set_target_server(chromatic_dualsp_pass_config[pass]);
            mut_printf(MUT_LOG_TEST_STATUS, "=== DUAL SP test pass %d: Run test on %s ===\n", pass,
                            chromatic_dualsp_pass_config[pass] == FBE_SIM_SP_A ? "SP A" : "SP B");
            status = fbe_test_sep_util_wait_for_database_service(CHROMATIC_TEST_WAIT_MSEC);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
            chromatic_raw_mirror_io_test();
            fbe_api_sleep(20000);
            chromatic_test_system_reboot();
    }
    /* Always clear dualsp mode
     */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);
}

static void chromatic_load_physical_config(void)
{

    fbe_status_t                            status;
    fbe_u32_t                               object_handle;
    fbe_u32_t                               port_idx;
    fbe_u32_t                               enclosure_idx;
    fbe_u32_t                               drive_idx;
    fbe_u32_t                               total_objects;
    fbe_class_id_t                          class_id;
    fbe_api_terminator_device_handle_t      port0_handle;
    fbe_api_terminator_device_handle_t      port0_encl_handle[CHROMATIC_TEST_ENCL_MAX];
    fbe_api_terminator_device_handle_t      drive_handle;
    fbe_u32_t                               slot;
    fbe_u32_t                               enclosure;

    /* Initialize local variables */
    status          = FBE_STATUS_GENERIC_FAILURE;
    object_handle   = 0;
    port_idx        = 0;
    enclosure_idx   = 0;
    drive_idx       = 0;
    total_objects   = 0;

    mut_printf(MUT_LOG_TEST_STATUS, "=== Load the Physical Configuration ===\n");

    mut_printf(MUT_LOG_TEST_STATUS, "=== configuring terminator ===\n");
    /* before loading the physical package we initialize the terminator */
    status = fbe_api_terminator_init();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Insert a board
     */
    status = fbe_test_pp_util_insert_armada_board();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Insert a port
     */
    fbe_test_pp_util_insert_sas_pmc_port(   1, /* io port */
                                            2, /* portal */
                                            0, /* backend number */ 
                                            &port0_handle);

    /* Insert enclosures to port 0
     */
    for ( enclosure = 0; enclosure < CHROMATIC_TEST_ENCL_MAX; enclosure++)
    { 
        fbe_test_pp_util_insert_viper_enclosure(0,
                                                enclosure,
                                                port0_handle,
                                                &port0_encl_handle[enclosure]);
    }

    /* Insert drives for enclosures.
     */
    for ( enclosure = 0; enclosure < CHROMATIC_TEST_ENCL_MAX; enclosure++)
    {
        for ( slot = 0; slot < 15; slot++)
        {
            if(SAS_DRIVE == encl_table[enclosure].drive_type)
            {
                fbe_test_pp_util_insert_sas_drive(encl_table[enclosure].port_number,
                                                encl_table[enclosure].encl_number,
                                                slot,
                                                encl_table[enclosure].block_size,
                                                TERMINATOR_MINIMUM_SYSTEM_DRIVE_CAPACITY,
                                                &drive_handle);
            }
            else if(SATA_DRIVE == encl_table[enclosure].drive_type)
            {
                fbe_test_pp_util_insert_sata_drive(encl_table[enclosure].port_number,
                                                encl_table[enclosure].encl_number,
                                                slot,
                                                encl_table[enclosure].block_size,
                                                TERMINATOR_MINIMUM_SYSTEM_DRIVE_CAPACITY,
                                                &drive_handle);
            }
        }
    }

    mut_printf(MUT_LOG_TEST_STATUS, "=== starting physical package===\n");
    status = fbe_api_sim_server_load_package(FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_TEST_STATUS, "=== set physical package entries ===\n");
    status = fbe_api_sim_server_set_package_entries(FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Init fbe api on server */
    mut_printf(MUT_LOG_TEST_STATUS, "=== starting Server FBE_API ===\n");
    status = fbe_api_sim_server_init_fbe_api();
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Wait for the expected number of objects */
    status = fbe_api_wait_for_number_of_objects(CHROMATIC_TEST_NUMBER_OF_PHYSICAL_OBJECTS, 20000, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, "Wait for number of objects failed!");

    mut_printf(MUT_LOG_TEST_STATUS, "=== verifying configuration ===\n");

    /* Inserted a armada board so we check for it;
     * board is always assumed to be object id 0
     */
    status = fbe_api_get_object_class_id (0, &class_id, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_TRUE(class_id == FBE_CLASS_ID_ARMADA_BOARD);
    mut_printf(MUT_LOG_TEST_STATUS, "verifying board type....Passed");

    /* Make sure we have the expected number of objects.
     */
    status = fbe_api_get_total_objects(&total_objects, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_TRUE(total_objects == CHROMATIC_TEST_NUMBER_OF_PHYSICAL_OBJECTS);
    mut_printf(MUT_LOG_TEST_STATUS, "verifying object count...Passed\n");

} /* End chromatic_load_physical_config() */

/*!**************************************************************
 * chromatic_setup()
 ****************************************************************
 * @brief
 *  
 *
 * @param None.               
 *
 * @return None.   
 *
 * @author
 *  10/8/2011 - Created. He Wei
 *
 ****************************************************************/
void chromatic_setup(void)
{
    /* we expect some error reported from raw read, and database should rebuild it */
    fbe_test_sep_util_set_error_injection_test_mode(FBE_TRUE);

    mut_printf(MUT_LOG_TEST_STATUS, "=== Set Target server to SPA ===\n");
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);

    /* Load the physical configuration */
    chromatic_load_physical_config();

    /* Load sep and neit packages */
    sep_config_load_sep_and_neit();
}
/**************************************
 * end chromatic_setup()
 **************************************/
	
/*!**************************************************************
 * chromatic_dualsp_setup()
 ****************************************************************
 * @brief
 *  
 *
 * @param None.               
 *
 * @return None.   
 *
 * @author
 *  1
 *
 ****************************************************************/
void chromatic_dualsp_setup(void)
{
    fbe_sim_transport_connection_target_t   sp;

    /* we expect some error reported from raw read, and database should rebuild it */
    fbe_test_sep_util_set_error_injection_test_mode(FBE_TRUE);


    if (fbe_test_util_is_simulation())
    { 
        mut_printf(MUT_LOG_TEST_STATUS, "=== Setup for chromatic dualsp test ===\n");
        /* Set the target server */
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    
        /* Make sure target set correctly */
        sp = fbe_api_sim_transport_get_target_server();
        MUT_ASSERT_INT_EQUAL(FBE_SIM_SP_B, sp);
    
        mut_printf(MUT_LOG_TEST_STATUS, "=== Load the Physical Configuration on %s ===\n", 
                   sp == FBE_SIM_SP_A ? "SP A" : "SP B");

        /* Load the physical config on the target server */
        //chromatic_create_physical_config(FBE_FALSE);
        chromatic_load_physical_config();
    
        /* Set the target server */
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    
        /* Make sure target set correctly */
        sp = fbe_api_sim_transport_get_target_server();
        MUT_ASSERT_INT_EQUAL(FBE_SIM_SP_A, sp);
    
        mut_printf(MUT_LOG_TEST_STATUS, "=== Load the Physical Configuration on %s ===\n", 
                   sp == FBE_SIM_SP_A ? "SP A" : "SP B");
    
        /* Load the physical config on the target server */    
        //chromatic_create_physical_config(FBE_FALSE);
		chromatic_load_physical_config();
		
        /* We will work primarily with SPA.  The configuration is automatically
         * instantiated on both SPs.  We no longer create the raid groups during 
         * the setup.
         */
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
        sep_config_load_sep_and_neit_both_sps();
    }

    /* Create a test logical configuration */
    //larry_t_lobster_test_create_logical_config();
	
    //  The following utility function does some setup for hardware; noop for simulation
    fbe_test_common_util_test_setup_init();

}
/**************************************
 * end chromatic_dualsp_setup()
 **************************************/

/*!**************************************************************
 * chromatic_cleanup()
 ****************************************************************
 * @brief
 *  Cleanup the chromatic test.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  10/8/2011 - Created. He Wei
 *
 ****************************************************************/

void chromatic_cleanup(void)
{
    /* Restore the sector trace level to it's default*/ 
    fbe_test_sep_util_set_error_injection_test_mode(FBE_FALSE);

    mut_printf(MUT_LOG_TEST_STATUS, "=== Set Target server to SPA ===\n");
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    /* Destroy the test configuration */
    fbe_test_sep_util_destroy_neit_sep_physical();
}
/******************************************
 * end chromatic_cleanup()
 ******************************************/

/*refers to Dora_test.c*/
void chromatic_dualsp_cleanup(void)
{  
    /* Restore the sector trace level to it's default*/ 
    fbe_test_sep_util_set_error_injection_test_mode(FBE_FALSE);

    /* Restore the sector trace level to it's default.
     */ 
    fbe_test_sep_util_restore_sector_trace_level();
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    fbe_test_sep_util_restore_sector_trace_level();
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);

    /* Always clear dualsp mode
     */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);

    if (fbe_test_util_is_simulation())
    {
        /* First execute teardown on SP B then on SP A
		*/
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
        fbe_test_sep_util_destroy_neit_sep_physical();

        /* First execute teardown on SP A
		*/
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
        fbe_test_sep_util_destroy_neit_sep_physical();
    }
} 


/*!**************************************************************
 * chromatic_test_system_reboot()
 ****************************************************************
 * @brief
 *  Reboot sep and neit package.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  10/12/2011 - Created. zphu
 ****************************************************************/

void chromatic_test_system_reboot()
{
    fbe_status_t status;
	
    mut_printf(MUT_LOG_TEST_STATUS, "Now reboot the system ......\n");
    fbe_test_sep_util_destroy_neit_sep();
    sep_config_load_sep_and_neit();
    status = fbe_test_sep_util_wait_for_database_service(CHROMATIC_TEST_WAIT_MSEC);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
}
/******************************************
 * end chromatic_test_system_reboot()
 ******************************************/



static fbe_status_t chromatic_wait_system_raid_and_pvd_connection(fbe_u32_t  object_count, fbe_object_id_t pvd_id)
{
    fbe_status_t    status;
    fbe_api_base_config_upstream_object_list_t    upstream_object_arr;
    fbe_u32_t   max_try_times = CHROMATIC_TEST_MAX_RETRY_COUNT;

    while(--max_try_times)
    {
        status = fbe_api_base_config_get_upstream_object_list(pvd_id, &upstream_object_arr);
        if(FBE_STATUS_OK != status)
        {
            fbe_api_sleep(1000);
            continue;
        }

        if(object_count != upstream_object_arr.number_of_upstream_objects)
        {
            fbe_api_sleep(1000);
            continue;
        }

        break;
    }

    if(0 == max_try_times)
        return FBE_STATUS_TIMEOUT;
   
    return FBE_STATUS_OK;
}

/*test to handle raw-mirror read write errors*/
/*
 Test Case 1:
    no disk failed and no error, performs normal IO and pass the test;
 
 Test Case 2:
    1) pull out disk 0_0_1,  performs IO and modify the database raw-mirrors;
    2) re-insert disk 0_0_1, waits for the rebuild or flush to complete and
       check if it can complete rebuilding
    3) pull out disk 0_0_0 and disk 0_0_2, read raw-mirror, check that the read
       content consistent with the content write in 1);
 
 Test Case 3:
    boot system with disk 0_0_1 down, test the system can boot successfully and
    do database raw-mirror IO
 
 Test Case 4:
    1) system boot with 3 disks ok;
    2) pull disk 0_0_1 out, do database raw-mirror IO;
    3) pull disk 0_0_2 out, do database raw-mirror IO; write a specific block X;
    4) re-insert disk 0_0_1, wait for the rebuild complete;
    5) pull disk 0_0_0 out, re-insert disk 0_0_2, read block X and check it consistent
       with the content write in step 3);
    6) pull disk 0_0_1,  read block X and check it consistent with the content write
       in step 3);
 
 Test Case 5:
    1) system boot with disk 0_0_1 down;
    2) after boot, do database raw-mirror IO; especially write block X;
    3) insert an empty disk 0_0_1, wait for the rebuild to complete;
    4) pull out disk 0_0_0 and disk 0_0_2, read block X and check that the content
       consistent with content wrote in step 2);
 
 Test case 6:
    1) system boot with 3 disk ok;
    2) inject magic number error for disk 0_0_1; check database raw-mirror read/write
       IO consistent; especially write block X;
    3) pull out disk 0_0_0 and disk 0_0_2, check that block X consistent with content
       wrote in step 2)
 

 Test case 7: 
    1) configure a hot spare 0_0_7; 
    2) pull the drive 0_0_1 and check the 0_0_7 is swapped in and rebuild starts; 
    3) Test if the raw-mirror IO can be done correctly; 
    4) re-insert drive 0_0_1 and test the raw-mirror can be rebuilt successfully; 
    5) cleanup and restart the system 

*/
void chromatic_raw_mirror_error_handling_test_phase1(void)
{
    fbe_database_control_system_db_op_t system_db_cmd;
    fbe_status_t status = FBE_STATUS_OK;
   fbe_database_control_raw_mirror_err_report_t err_report;
    database_user_entry_t  user_entry;
#if 0
    fbe_u64_t wait_count = 0;
    fbe_u32_t i = 0;
#endif
    //fbe_object_id_t vd_object_id;
    //fbe_notification_registration_id_t  registration_id;
    //chromatic_test_spare_context_t    spare_context;
    fbe_u32_t                         upstream_object_count = 0;

   //fbe_api_terminator_device_handle_t  drive_handle;
   /*fbe_api_terminator_device_handle_t  drive_handle2;*/

    mut_printf(MUT_LOG_TEST_STATUS, "Start Test database raw-mirror error handling\n");
    fbe_zero_memory(&system_db_cmd, sizeof (fbe_database_control_system_db_op_t));

    #if 0
/*
      Test case 1: 
      1) configure a hot spare 0_0_7; 
      2) pull the drive 0_0_1 and check the 0_0_7 is swapped in and rebuild starts; 
      3) Test if the raw-mirror IO can be done correctly; 
      4) re-insert drive 0_0_1 and test the raw-mirror can be rebuilt successfully; 
      5) cleanup and restart the system 
*/
      mut_printf(MUT_LOG_TEST_STATUS, "Chromatic Phase 1 Test Case 1 Started");
      /* Speed up VD hot spare */
      status = fbe_test_sep_util_update_permanent_spare_trigger_timer(1); /* 1 second hotspare timeout */
      MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    
      /* Register notification service to the vd object connecting the removed drive*/
      fbe_test_sep_util_get_virtual_drive_object_id_by_position(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_TRIPLE_MIRROR,
                                                      1, &vd_object_id);
      
      /*register the spare notification*/
      spare_context.object_id = vd_object_id;
      fbe_semaphore_init(&(spare_context.sem), 0, 1);
    
      status = fbe_api_notification_interface_register_notification(FBE_NOTIFICATION_TYPE_JOB_ACTION_STATE_CHANGED, 
          FBE_PACKAGE_NOTIFICATION_ID_SEP_0, FBE_TOPOLOGY_OBJECT_TYPE_VIRTUAL_DRIVE, 
          chromatic_test_spare_callback_function, &spare_context, &registration_id);
      
      MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
            
      mut_printf(MUT_LOG_TEST_STATUS, "%s: pull drive 0-0-1", 
                 __FUNCTION__);
    
      /* Remove the specified system drive*/
      status = fbe_test_pp_util_pull_drive_hw(0, 0, 1);
      MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    
    
      /* Verify the RG state after removal of the system drive*/
      mut_printf(MUT_LOG_TEST_STATUS, "%s: vefity triple_mirror system RG lifecycle.",__FUNCTION__);
      status = fbe_api_wait_for_object_lifecycle_state(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_TRIPLE_MIRROR,
                                                      FBE_LIFECYCLE_STATE_READY, 30000, FBE_PACKAGE_ID_SEP_0);
      MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
      
      /* Wait for the notification about the hot spare action */
      mut_printf(MUT_LOG_TEST_STATUS, "%s: Wait for the notification about the hot spare action",__FUNCTION__);
      status = fbe_semaphore_wait_ms(&(spare_context.sem), 80000);
      MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);    
      MUT_ASSERT_INT_EQUAL(FBE_JOB_SERVICE_ERROR_NO_ERROR,spare_context.job_srv_info.error_code);

      /*wait for rebuild starting*/
          
      mut_printf(MUT_LOG_TEST_STATUS, "%s: wait for DB raidgroup start rebuilding",__FUNCTION__);
      status = chromatic_rebuild_wait_for_rb_start_by_obj_id(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_TRIPLE_MIRROR, 1, 0x80);
      MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    
      mut_printf(MUT_LOG_TEST_STATUS, "%s: rebuild started and do IO test.",__FUNCTION__);
    
      /*check if the raw-mirror IO can be done correctly*/
      chromatic_raw_mirror_io_test();
      chromatic_raw_mirror_read_user_entry(CHROMATIC_TEST_OBJECT_ID_MCR_DATABASE_LUN, 
                                               &user_entry);
    
      memcpy(user_entry.user_data_union.lu_user_data.world_wide_name.bytes, "crazy7", 6);
      chromatic_raw_mirror_write_user_entry(CHROMATIC_TEST_OBJECT_ID_MCR_DATABASE_LUN, 
                                               &user_entry);
    
      status = fbe_test_pp_util_reinsert_drive_hw(0, 0, 1);
      MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    
      /*wait for the rebuild to complete*/
      chromatic_wait_raw_mirror_rebuild_complete(CHROMATIC_TEST_RAW_MIRROR_REBUILD_TIMEOUT, 0, 0);

      status = chromatic_wait_system_vd_and_pvd_connection(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_VD_1,
                                                                                            FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_1);
      MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "fail to wait system vd and pvd connection");
    
      status = fbe_test_pp_util_pull_drive_hw(0, 0, 0);
      MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
      status = fbe_test_pp_util_pull_drive_hw(0, 0, 2);
      MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
      chromatic_raw_mirror_read_user_entry(CHROMATIC_TEST_OBJECT_ID_MCR_DATABASE_LUN, 
                                           &user_entry);
      // cannot 'print' a wwn, it's not an ascii string
      //mut_printf(MUT_LOG_TEST_STATUS, "lun wwn name: %s \n", 
      //           user_entry.user_data_union.lu_user_data.world_wide_name.bytes);

      if (memcmp(user_entry.user_data_union.lu_user_data.world_wide_name.bytes, "crazy7", 6) != 0)
      {
           mut_printf(MUT_LOG_TEST_STATUS, "Raw-mirror Rebuild when System Drive Sparing failed \n");
           MUT_ASSERT_INT_EQUAL(0, 1);
      } 
      else
           mut_printf(MUT_LOG_TEST_STATUS, "Raw-mirror Rebuild when System Drive Sparing successful \n");
    
      mut_printf(MUT_LOG_TEST_STATUS, "Chromatic Phase 1 Test Case 1 Passed");
      status = fbe_test_pp_util_reinsert_drive_hw(0, 0, 0);
      MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
      status = fbe_test_pp_util_reinsert_drive_hw(0, 0, 2);
      MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
      chromatic_wait_raw_mirror_rebuild_complete(CHROMATIC_TEST_RAW_MIRROR_REBUILD_TIMEOUT, 0, 0);

      #endif

    
    /* 
       Test Case 2:
         no disk failed and no error, performs database raw-mirror IO and
         pass the test; 
    */ 
    mut_printf(MUT_LOG_TEST_STATUS, "Verify the system db entry persist and boot in backend\n");

    chromatic_raw_mirror_io_test();

    /*
     Test Case 3:
    1) pull out disk 0_0_1,  performs IO and modify the database raw-mirrors;
    2) re-insert disk 0_0_1, waits for the rebuild or flush to complete and
       check if it can complete rebuilding
    3) pull out disk 0_0_0 and disk 0_0_2, read raw-mirror, check that the read
       content consistent with the content write in 1);
    */

    mut_printf(MUT_LOG_TEST_STATUS, "Chromatic Phase 1 Test Case 3\n");
    mut_printf(MUT_LOG_TEST_STATUS, "Pull out the drive 0_0_1\n");

    status = fbe_test_pp_util_pull_drive_hw(0, 0, 1);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    /* Wait for the notification about the hot spare action */
    /*mut_printf(MUT_LOG_TEST_STATUS, "%s: Wait for the notification about the hot spare action",__FUNCTION__);
    status = fbe_semaphore_wait_ms(&(spare_context.sem), 80000);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);    
    MUT_ASSERT_INT_EQUAL(FBE_JOB_SERVICE_ERROR_NO_ERROR,spare_context.job_srv_info.error_code); */
    
    mut_printf(MUT_LOG_TEST_STATUS, "Do IO on raw-3way-mirror after one drive is pulled out\n");
    chromatic_raw_mirror_io_test();

    chromatic_raw_mirror_read_user_entry(CHROMATIC_TEST_OBJECT_ID_MCR_DATABASE_LUN, 
                                         &user_entry);
    memcpy(user_entry.user_data_union.lu_user_data.world_wide_name.bytes, "crm0", 4);
    chromatic_raw_mirror_write_user_entry(CHROMATIC_TEST_OBJECT_ID_MCR_DATABASE_LUN, 
                                         &user_entry);

    chromatic_get_raw_mirror_error_report(&err_report);
    mut_printf(MUT_LOG_TEST_STATUS, "raw mirror err report err: %llu, flushed: %llu, failed: %llu,"
                                     "down disk bitmap: %x, degradation bitmap: %x, nr disk %x", 
               (unsigned long long)err_report.error_count,
	       (unsigned long long)err_report.flush_error_count, 
               (unsigned long long)err_report.failed_flush_count,
	       err_report.down_disk_bitmap,
               err_report.degraded_disk_bitmap, err_report.nr_disk_bitmap);

    MUT_ASSERT_INT_EQUAL(err_report.degraded_disk_bitmap, 0x0);
  

    status = fbe_test_pp_util_reinsert_drive_hw(0, 0, 1); 
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    upstream_object_count = chromatic_get_num_of_upstream_objects(0,0,1);
    status = chromatic_wait_system_raid_and_pvd_connection(upstream_object_count, 
                                                         FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_1);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "fail to wait system vd and pvd connection");

    /*wait for the rebuild to complete*/
    chromatic_wait_raw_mirror_rebuild_complete(CHROMATIC_TEST_RAW_MIRROR_REBUILD_TIMEOUT, 0, 0);
    chromatic_test_wait_rg_rebuild_complete(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_TRIPLE_MIRROR,1);

    status = fbe_test_pp_util_pull_drive_hw(0, 0, 0);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    status = fbe_test_pp_util_pull_drive_hw(0, 0, 2);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    chromatic_raw_mirror_read_user_entry(CHROMATIC_TEST_OBJECT_ID_MCR_DATABASE_LUN, 
                                         &user_entry);
    // cannot 'print' a wwn, it's not an ascii string

    //mut_printf(MUT_LOG_TEST_STATUS, "lun wwn name: %s \n", 
    //           user_entry.user_data_union.lu_user_data.world_wide_name.bytes);
    if (memcmp(user_entry.user_data_union.lu_user_data.world_wide_name.bytes,
               "crm0", 4) != 0)
    {
        mut_printf(MUT_LOG_TEST_STATUS, "raw_mirror rebuild failure \n");
        MUT_ASSERT_INT_EQUAL(0, 1);
    } 
    else
        mut_printf(MUT_LOG_TEST_STATUS, "raw_mirror rebuild check successful \n");
   
    status = fbe_test_pp_util_reinsert_drive_hw(0, 0, 0);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    status = fbe_test_pp_util_reinsert_drive_hw(0, 0, 2);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    chromatic_wait_raw_mirror_rebuild_complete(CHROMATIC_TEST_RAW_MIRROR_REBUILD_TIMEOUT, 0, 0);
    chromatic_test_wait_rg_rebuild_complete(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_TRIPLE_MIRROR,0);
    chromatic_test_wait_rg_rebuild_complete(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_TRIPLE_MIRROR,2);
  
    mut_printf(MUT_LOG_TEST_STATUS, "Chromatic Phase 1 Test Case 3 Passed\n");

    #if 0
    /*
     Test Case 4:
     1) boot system with disk 0_0_1 down, test the system can boot successfully and
     2) do database raw-mirror IO;
     3) re-insert 0_0_1 and check if it can be rebuilt
    */

    mut_printf(MUT_LOG_TEST_STATUS, "Chromatic Phase 1 Test Case 4 started \n");
    fbe_test_pp_util_pull_drive(0, 0, 1, &drive_handle);
    /* Wait for the notification about the hot spare action */
    /*mut_printf(MUT_LOG_TEST_STATUS, "%s: Wait for the notification about the hot spare action",__FUNCTION__);
    status = fbe_semaphore_wait_ms(&(spare_context.sem), 80000);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);    
    MUT_ASSERT_INT_EQUAL(FBE_JOB_SERVICE_ERROR_NO_ERROR,spare_context.job_srv_info.error_code);*/

    chromatic_test_system_reboot();
    
    /* Speed up VD hot spare */
    status = fbe_test_sep_util_update_permanent_spare_trigger_timer(1); /* 1 second hotspare timeout */
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    mut_printf(MUT_LOG_TEST_STATUS, "Verify the system db entry persist with one drive down\n");

    chromatic_raw_mirror_io_test();

    chromatic_raw_mirror_read_user_entry(CHROMATIC_TEST_OBJECT_ID_MCR_DATABASE_LUN, 
                                         &user_entry);
    memcpy(user_entry.user_data_union.lu_user_data.world_wide_name.bytes,
            "rit47", 5);

    chromatic_raw_mirror_write_user_entry(CHROMATIC_TEST_OBJECT_ID_MCR_DATABASE_LUN, 
                                         &user_entry);


    mut_printf(MUT_LOG_TEST_STATUS, "re-insert the drive 0_0_1 and rebuild\n");
    fbe_test_pp_util_reinsert_drive(0, 0, 1, drive_handle);
    upstream_object_count = chromatic_get_num_of_upstream_objects(0,0,1);
    status = chromatic_wait_system_raid_and_pvd_connection(upstream_object_count, FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_1);
                                                                                           
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "fail to wait system raid and pvd connection");
    
    /*wait for the rebuild to complete*/
    chromatic_wait_raw_mirror_rebuild_complete(CHROMATIC_TEST_RAW_MIRROR_REBUILD_TIMEOUT, 0, 0);
    mut_printf(MUT_LOG_TEST_STATUS, "drive 0_0_1 rebuild successful===\n");

    status = fbe_test_pp_util_pull_drive_hw(0, 0, 0);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    status = fbe_test_pp_util_pull_drive_hw(0, 0, 2);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    chromatic_raw_mirror_read_user_entry(CHROMATIC_TEST_OBJECT_ID_MCR_DATABASE_LUN, 
                                         &user_entry);
    // cannot 'print' a wwn, it's not an ascii string
    //mut_printf(MUT_LOG_TEST_STATUS, "lun wwn name: %s \n", 
    //           user_entry.user_data_union.lu_user_data.world_wide_name.bytes);
    if (memcmp(user_entry.user_data_union.lu_user_data.world_wide_name.bytes, "rit47", 5) != 0)
    {
        mut_printf(MUT_LOG_TEST_STATUS, "raw_mirror rebuild failure \n");
        MUT_ASSERT_INT_EQUAL(0, 1);
    }
    else
        mut_printf(MUT_LOG_TEST_STATUS, "raw_mirror rebuild check successful \n");
    
    status = fbe_test_pp_util_reinsert_drive_hw(0, 0, 0);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    status = fbe_test_pp_util_reinsert_drive_hw(0, 0, 2);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    chromatic_wait_raw_mirror_rebuild_complete(CHROMATIC_TEST_RAW_MIRROR_REBUILD_TIMEOUT, 0, 0);
    mut_printf(MUT_LOG_TEST_STATUS, "Chromatic Phase 1 Test Case 4 passed \n");
 
    /*status = fbe_api_notification_interface_unregister_notification(chromatic_test_spare_callback_function ,
         registration_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    fbe_semaphore_destroy(&(spare_context.sem));*/
    #endif
}

void chromatic_raw_mirror_error_handling_test_phase2(void)
{
    fbe_database_control_system_db_op_t system_db_cmd;
    fbe_status_t status = FBE_STATUS_OK;
   fbe_database_control_raw_mirror_err_report_t err_report;
    database_user_entry_t  user_entry;
#if 0
    fbe_u64_t wait_count = 0;
    fbe_u32_t i = 0;
#endif
    fbe_u32_t     upstream_object_count = 0;

   //fbe_api_terminator_device_handle_t  drive_handle;
   //fbe_api_terminator_device_handle_t  drive_handle2;

    mut_printf(MUT_LOG_TEST_STATUS, "Start Test database raw-mirror error handling\n");
    fbe_zero_memory(&system_db_cmd, sizeof (fbe_database_control_system_db_op_t));

    /* Speed up VD hot spare */
    status = fbe_test_sep_util_update_permanent_spare_trigger_timer(1); /* 1 second hotspare timeout */
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
 
    /*
     Test Case 1:
    1) system boot with 3 disks ok;
    2) pull disk 0_0_1 out, do database raw-mirror IO;
    3) pull disk 0_0_2 out, do database raw-mirror IO; write a specific block X;
    4) re-insert disk 0_0_1, wait for the rebuild complete;
    5) pull disk 0_0_0 out, re-insert disk 0_0_2, read block X and check it consistent
       with the content write in step 3);
    6) pull disk 0_0_1,  read block X and check it consistent with the content write
       in step 3);
    */
    mut_printf(MUT_LOG_TEST_STATUS, "Chromatic Phase 2 Test Case 1 started \n");
    mut_printf(MUT_LOG_TEST_STATUS, "Pull out the drive 0_0_1\n");
    fbe_test_pp_util_pull_drive_hw(0, 0, 1);

    mut_printf(MUT_LOG_TEST_STATUS, "Do IO on database raw-mirror\n");
    chromatic_raw_mirror_io_test();

    mut_printf(MUT_LOG_TEST_STATUS, "Pull out the drive 0_0_2\n");
    fbe_test_pp_util_pull_drive_hw(0, 0, 2);
    chromatic_raw_mirror_read_user_entry(CHROMATIC_TEST_OBJECT_ID_MCR_DATABASE_LUN, 
                                         &user_entry);
    memcpy(user_entry.user_data_union.lu_user_data.world_wide_name.bytes,
            "0rcm", 4);
    chromatic_raw_mirror_write_user_entry(CHROMATIC_TEST_OBJECT_ID_MCR_DATABASE_LUN, 
                                         &user_entry);
    fbe_test_pp_util_reinsert_drive_hw(0, 0, 1); 
    upstream_object_count = chromatic_get_num_of_upstream_objects(0,0,1);
    status = chromatic_wait_system_raid_and_pvd_connection(upstream_object_count, 
                                                    FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_1);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "fail to wait system vd and pvd connection");

    /*wait for the rebuild to complete*/
    chromatic_wait_raw_mirror_rebuild_complete(CHROMATIC_TEST_RAW_MIRROR_REBUILD_TIMEOUT, 0x4, 0x4);
    chromatic_test_wait_rg_rebuild_complete(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_TRIPLE_MIRROR,1);

    fbe_test_pp_util_pull_drive_hw(0, 0, 0);
    fbe_test_pp_util_reinsert_drive_hw(0, 0, 2);
    upstream_object_count = chromatic_get_num_of_upstream_objects(0,0,2);
    status = chromatic_wait_system_raid_and_pvd_connection(upstream_object_count, 
                                                    FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_2);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "fail to wait system vd and pvd connection");
    chromatic_raw_mirror_read_user_entry(CHROMATIC_TEST_OBJECT_ID_MCR_DATABASE_LUN, 
                                         &user_entry);
    // cannot 'print' a wwn, it's not an ascii string
    //mut_printf(MUT_LOG_TEST_STATUS, "lun wwn name: %s \n", 
    //           user_entry.user_data_union.lu_user_data.world_wide_name.bytes);
   if (memcmp(user_entry.user_data_union.lu_user_data.world_wide_name.bytes, "0rcm", 4) != 0)
    {
        mut_printf(MUT_LOG_TEST_STATUS, "raw_mirror rebuild failure \n");
        MUT_ASSERT_INT_EQUAL(0, 1);
    } else
        mut_printf(MUT_LOG_TEST_STATUS, "raw_mirror rebuild check successful \n");

    chromatic_wait_raw_mirror_rebuild_complete(CHROMATIC_TEST_RAW_MIRROR_REBUILD_TIMEOUT, 0x1, 0x1);
    chromatic_test_wait_rg_rebuild_complete(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_TRIPLE_MIRROR,2);

    fbe_test_pp_util_pull_drive_hw(0, 0, 1);
    chromatic_raw_mirror_read_user_entry(CHROMATIC_TEST_OBJECT_ID_MCR_DATABASE_LUN, 
                                         &user_entry);
    // cannot 'print' a wwn, it's not an ascii string

    //mut_printf(MUT_LOG_TEST_STATUS, "lun wwn name: %s \n", 
    //           user_entry.user_data_union.lu_user_data.world_wide_name.bytes);
   if (memcmp(user_entry.user_data_union.lu_user_data.world_wide_name.bytes,
               "0rcm", 4) != 0)
    {
        mut_printf(MUT_LOG_TEST_STATUS, "raw_mirror rebuild failure \n");
        MUT_ASSERT_INT_EQUAL(0, 1);
    } else
        mut_printf(MUT_LOG_TEST_STATUS, "raw_mirror rebuild check successful \n");


    fbe_test_pp_util_reinsert_drive_hw(0, 0, 0);
    fbe_test_pp_util_reinsert_drive_hw(0, 0, 1);

    chromatic_wait_raw_mirror_rebuild_complete(CHROMATIC_TEST_RAW_MIRROR_REBUILD_TIMEOUT, 0x0, 0x0);
    chromatic_test_wait_rg_rebuild_complete(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_TRIPLE_MIRROR,0);
    chromatic_test_wait_rg_rebuild_complete(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_TRIPLE_MIRROR,1);
    mut_printf(MUT_LOG_TEST_STATUS, "Chromatic Phase 2 Test Case 1 passed \n");

    #if 0
/*
    Test Case 2:
    1) system boot with 3 disk good;
    2) after boot, pull drive 0_0_1 physically
    3) do database raw-mirror IO; especially write block X;
    4) insert an empty disk 0_0_1, wait for the rebuild to complete;
    5) pull out disk 0_0_0 and disk 0_0_2, read block X and check that the content
       consistent with content wrote in step 3);
*/
    mut_printf(MUT_LOG_TEST_STATUS, "Chromatic Phase 2 Test Case 2 Started\n");
    mut_printf(MUT_LOG_TEST_STATUS, "Pull out the drive 0_0_1\n");
    //fbe_test_pp_util_pull_drive_hw(0, 0, 1);
    fbe_test_pp_util_pull_drive(0, 0, 1, &drive_handle);

    chromatic_raw_mirror_io_test();

    chromatic_raw_mirror_read_user_entry(CHROMATIC_TEST_OBJECT_ID_MCR_DATABASE_LUN, 
                                         &user_entry);
    // cannot 'print' a wwn, it's not an ascii string
	
    //mut_printf(MUT_LOG_TEST_STATUS, "lun wwn name: %s \n", 
    //           user_entry.user_data_union.lu_user_data.world_wide_name.bytes);
    memcpy(user_entry.user_data_union.lu_user_data.world_wide_name.bytes, "r1mc", 4);
    chromatic_raw_mirror_write_user_entry(CHROMATIC_TEST_OBJECT_ID_MCR_DATABASE_LUN, 
                                         &user_entry);

    mut_printf(MUT_LOG_TEST_STATUS, "Insert a new empty drive 0_0_1\n");
    status = fbe_test_pp_util_insert_unique_sas_drive(0, 0, 1, 520, TERMINATOR_MINIMUM_SYSTEM_DRIVE_CAPACITY,
                                                &drive_handle2);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    upstream_object_count = chromatic_get_num_of_upstream_objects(0,0,1);
    status = chromatic_wait_system_raid_and_pvd_connection(upstream_object_count, 
                                                    FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_1);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "fail to wait system vd and pvd connection");
    chromatic_wait_raw_mirror_rebuild_complete(CHROMATIC_TEST_RAW_MIRROR_REBUILD_TIMEOUT, 0, 0);

    status = fbe_test_pp_util_pull_drive_hw(0, 0, 0);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);				 
	
	/*	  Print message as to where the test is at */
	mut_printf(MUT_LOG_TEST_STATUS, "Removed Drive: %d_%d_%d", 
			   0,
			   0,
			   0);
    fbe_test_pp_util_pull_drive_hw(0, 0, 2);
	
	MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);				 
	
	/*	  Print message as to where the test is at */
	mut_printf(MUT_LOG_TEST_STATUS, "Removed Drive: %d_%d_%d", 
			   0,
			   0,
			   2);
    mut_printf(MUT_LOG_TEST_STATUS, "try to read data from database raw-mirror \n");
    chromatic_raw_mirror_read_user_entry(CHROMATIC_TEST_OBJECT_ID_MCR_DATABASE_LUN, 
                                         &user_entry);
    // cannot 'print' a wwn, it's not an ascii string

    //mut_printf(MUT_LOG_TEST_STATUS, "lun wwn name: %s \n", 
    //           user_entry.user_data_union.lu_user_data.world_wide_name.bytes);
   if (memcmp(user_entry.user_data_union.lu_user_data.world_wide_name.bytes,
               "r1mc", 4) != 0)
    {
        mut_printf(MUT_LOG_TEST_STATUS, "raw_mirror rebuild failure \n");
        MUT_ASSERT_INT_EQUAL(0, 1);
    } else
        mut_printf(MUT_LOG_TEST_STATUS, "raw_mirror rebuild check successful \n");

    fbe_test_pp_util_reinsert_drive_hw(0, 0, 0);
    fbe_test_pp_util_reinsert_drive_hw(0, 0, 2);
    upstream_object_count = chromatic_get_num_of_upstream_objects(0,0,2);
    status = chromatic_wait_system_raid_and_pvd_connection(upstream_object_count, 
                                                    FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_2);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "fail to wait system vd and pvd connection");
    chromatic_wait_raw_mirror_rebuild_complete(CHROMATIC_TEST_RAW_MIRROR_REBUILD_TIMEOUT, 0, 0);
    mut_printf(MUT_LOG_TEST_STATUS, "Chromatic Phase 2 Test Case 2 passed \n");

    #endif

    /*
     Test Case 3: 
     inject write errors to physical drive 0_0_0 and test if it can
     handle it approapriately 
    */

    mut_printf(MUT_LOG_TEST_STATUS, "Starting Chromatic Phase 2 Test Case 3");
    status = fbe_api_logical_error_injection_disable_records(0, FBE_LOGICAL_ERROR_INJECTION_MAX_RECORDS); 
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /*  Enable the error injection service
     */
    status = fbe_api_logical_error_injection_enable();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    mut_printf(MUT_LOG_TEST_STATUS, "inject errors to disk 0_0_0 and do raw-mirror IO");
    chromatic_inject_errors_to_system_drive(0x1);

    chromatic_raw_mirror_io_test();

    chromatic_raw_mirror_read_user_entry(CHROMATIC_TEST_OBJECT_ID_MCR_DATABASE_LUN, 
                                             &user_entry);

    memcpy(user_entry.user_data_union.lu_user_data.world_wide_name.bytes, "gaoj", 4);
    chromatic_raw_mirror_write_user_entry(CHROMATIC_TEST_OBJECT_ID_MCR_DATABASE_LUN, 
                                             &user_entry);
    chromatic_get_raw_mirror_error_report(&err_report);

    status = fbe_api_protocol_error_injection_stop();
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    status = fbe_api_logical_error_injection_disable();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /*pull and re-insert the drive to fix the injected errors*/
    fbe_test_pp_util_pull_drive_hw(0, 0, 0);
    fbe_api_sleep(8000);
    fbe_test_pp_util_reinsert_drive_hw(0, 0, 0);
    /* Clear drive fault */
    status = fbe_api_provision_drive_clear_drive_fault(0x1);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /*wait for the rebuild complete*/
    chromatic_wait_raw_mirror_rebuild_complete(CHROMATIC_TEST_RAW_MIRROR_REBUILD_TIMEOUT, 0, 0);
    chromatic_test_wait_rg_rebuild_complete(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_TRIPLE_MIRROR,0);
    chromatic_test_wait_rg_rebuild_complete(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_TRIPLE_MIRROR,1);

    fbe_test_pp_util_pull_drive_hw(0, 0, 1);
    fbe_test_pp_util_pull_drive_hw(0, 0, 2);
    chromatic_raw_mirror_read_user_entry(CHROMATIC_TEST_OBJECT_ID_MCR_DATABASE_LUN, 
                                         &user_entry);
    // cannot 'print' a wwn, it's not an ascii string

    //mut_printf(MUT_LOG_TEST_STATUS, "lun wwn name: %s \n", 
    //           user_entry.user_data_union.lu_user_data.world_wide_name.bytes);
    if (memcmp(user_entry.user_data_union.lu_user_data.world_wide_name.bytes, "gaoj", 4) != 0)
    {
         mut_printf(MUT_LOG_TEST_STATUS, "raw_mirror write error rebuild failed \n");
         MUT_ASSERT_INT_EQUAL(0, 1);
     } else
         mut_printf(MUT_LOG_TEST_STATUS, "raw_mirror write error rebuild successful \n");

    fbe_test_pp_util_reinsert_drive_hw(0, 0, 1);
    fbe_test_pp_util_reinsert_drive_hw(0, 0, 2);
    chromatic_wait_raw_mirror_rebuild_complete(CHROMATIC_TEST_RAW_MIRROR_REBUILD_TIMEOUT, 0, 0);
    chromatic_test_wait_rg_rebuild_complete(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_TRIPLE_MIRROR,1);
    chromatic_test_wait_rg_rebuild_complete(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_TRIPLE_MIRROR,2);
    mut_printf(MUT_LOG_TEST_STATUS, "TChromatic Phase 2 Test Case 3 Passed\n");
    mut_printf(MUT_LOG_TEST_STATUS, "Chromatic Phase 2 Test Passed \n");
}


fbe_status_t chromatic_test_wait_system_pvd_state(fbe_u32_t port_number,
                                             fbe_u32_t enclosure_number,
                                             fbe_u32_t slot_number, 
                                             fbe_lifecycle_state_t state)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_object_id_t         pvd_object_id;
    
    /*only handle system pvd in chromatic test*/
    pvd_object_id = slot_number + 1;
    status = fbe_api_wait_for_object_lifecycle_state(pvd_object_id, state,
                                                     60000, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    return status;
}


static fbe_status_t chromatic_rebuild_verify_rb_logging_by_object_id(
                                        fbe_object_id_t                 in_raid_group_object_id,
                                        fbe_u32_t                       in_position)
{
    fbe_status_t                        status;                         //  fbe status
    fbe_u32_t                           count;                          //  loop count
    fbe_u32_t                           max_retries;                    //  maximum number of retries
    fbe_api_raid_group_get_info_t       raid_group_info;                //  rg's information from "get info" command


    /*  Set the max retries in a local variable. (This allows it to be changed within the debugger if needed.)*/
    max_retries = 2400;

    /*  Loop until the drive has been marked rebuild logging and the rebuild checkpoint is 0 */
    for (count = 0; count < max_retries; count ++)
    {
        /*  Get the raid group information*/
        status = fbe_api_raid_group_get_info(in_raid_group_object_id, &raid_group_info, FBE_PACKET_FLAG_NO_ATTRIB);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        if ((raid_group_info.b_rb_logging[in_position] == FBE_TRUE) )
            return FBE_STATUS_OK;

        fbe_api_sleep(5000);
    }

    return FBE_STATUS_TIMEOUT;
}


static fbe_status_t chromatic_rebuild_wait_for_rb_start_by_obj_id(
                                    fbe_object_id_t                 in_raid_group_object_id,
                                    fbe_u32_t                       in_position,
                                    fbe_u32_t  limit)
{
    fbe_status_t                    status;
    fbe_u32_t                       count;
    fbe_u32_t                       max_retries;
    fbe_api_raid_group_get_info_t   raid_group_info;
    fbe_lba_t last_checkpoint = 0;
    fbe_lba_t cur_checkpoint = 0;
    max_retries = 1200;

    for (count = 0; count < max_retries; count++)
    {
        //  Get the raid group information
        status = fbe_api_raid_group_get_info(in_raid_group_object_id, &raid_group_info, FBE_PACKET_FLAG_NO_ATTRIB);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        mut_printf(MUT_LOG_LOW, "== rg object id: 0x%x pos: %d rebuild checkpoint is at 0x%x ==", 
            in_raid_group_object_id, in_position, (unsigned int)raid_group_info.rebuild_checkpoint[in_position]);
                                
        if (count == 0) {
            cur_checkpoint = last_checkpoint = raid_group_info.rebuild_checkpoint[in_position];
        } else {
            last_checkpoint = cur_checkpoint;
            cur_checkpoint = raid_group_info.rebuild_checkpoint[in_position];
            if (last_checkpoint != cur_checkpoint && cur_checkpoint >= limit)
                return FBE_STATUS_OK;
        }

        fbe_api_sleep(1000);
    }

    return FBE_STATUS_TIMEOUT;
}


void chromatic_raw_mirror_write_user_entry(fbe_object_id_t object_id, database_user_entry_t *entry)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_database_control_system_db_op_t system_db_cmd;

    /*read system db object directly to verify if the transaction actually persist the objects*/
    fbe_zero_memory(&system_db_cmd, sizeof (fbe_database_control_system_db_op_t));


    system_db_cmd.cmd = FBE_DATABASE_CONTROL_SYSTEM_DB_WRITE_USER;
    system_db_cmd.object_id = object_id;
    fbe_copy_memory(system_db_cmd.cmd_data, entry, sizeof (database_user_entry_t));
    status = fbe_api_database_system_db_send_cmd(&system_db_cmd);
	MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

}

void chromatic_raw_mirror_read_user_entry(fbe_object_id_t object_id, database_user_entry_t *entry)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_database_control_system_db_op_t system_db_cmd;

    /*read system db object directly to verify if the transaction actually persist the objects*/
    fbe_zero_memory(&system_db_cmd, sizeof (fbe_database_control_system_db_op_t));


    system_db_cmd.cmd = FBE_DATABASE_CONTROL_SYSTEM_DB_READ_USER;
    system_db_cmd.object_id = object_id;
    status = fbe_api_database_system_db_send_cmd(&system_db_cmd);
	MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    fbe_copy_memory(entry, system_db_cmd.cmd_data, sizeof(database_user_entry_t));

}

void chromatic_get_raw_mirror_error_report(fbe_database_control_raw_mirror_err_report_t *report)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_database_control_system_db_op_t system_db_cmd;

    MUT_ASSERT_POINTER_NOT_EQUAL(report, NULL);

    fbe_zero_memory(&system_db_cmd, sizeof (fbe_database_control_system_db_op_t));

    system_db_cmd.cmd = FBE_DATABASE_CONTROL_SYSTEM_DB_GET_RAW_MIRROR_ERR_REPORT;
    status = fbe_api_database_system_db_send_cmd(&system_db_cmd);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    fbe_copy_memory(report, system_db_cmd.cmd_data, sizeof (fbe_database_control_raw_mirror_err_report_t));

}

void chromatic_raw_mirror_io_test(void)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_database_control_system_db_op_t system_db_cmd;

    fbe_zero_memory(&system_db_cmd, sizeof (fbe_database_control_system_db_op_t));
    system_db_cmd.cmd = FBE_DATABASE_CONTROL_SYSTEM_DB_VERIFY_BOOT;
    status = fbe_api_database_system_db_send_cmd(&system_db_cmd);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
}


void chromatic_wait_raw_mirror_rebuild_complete(fbe_u32_t timeout, fbe_u32_t down_disk_mask,
                                                fbe_u32_t nr_disk_mask)
{
    fbe_status_t status;
    mut_printf(MUT_LOG_TEST_STATUS, "wait for raw mirror degraded bitmask to be: 0x%x", nr_disk_mask);
    status = fbe_test_sep_util_wait_for_degraded_bitmask(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_RAW_MIRROR_RG, 
                                                         nr_disk_mask, FBE_TEST_WAIT_TIMEOUT_MS);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
#if 0
    fbe_database_control_raw_mirror_err_report_t err_report;
    fbe_u32_t wait_count = 0;
    fbe_u64_t database_raw_mirror_capacity = 0;
    fbe_private_space_layout_region_t region_info;
    fbe_private_space_layout_get_region_by_region_id(FBE_PRIVATE_SPACE_LAYOUT_REGION_ID_SEP_SYSTEM_DB, &region_info);
    database_raw_mirror_capacity = region_info.size_in_blocks;
    /*wait for the rebuild to complete*/
    wait_count = 0;
    while (wait_count < timeout) {
        chromatic_get_raw_mirror_error_report(&err_report);
        mut_printf(MUT_LOG_TEST_STATUS, "raw mirror err report err: %d, flushed: %d, failed: %d,"
                                         "down disk bitmap: %x, degradation bitmap: %x, nr disk: 0x%x,"
                                         "rebuild checkpint: 0x%x", 
                   err_report.error_count, err_report.flush_error_count, 
                   err_report.failed_flush_count, err_report.down_disk_bitmap,
                   err_report.degraded_disk_bitmap, err_report.nr_disk_bitmap,
                   err_report.rebuild_checkpoint);
        if (err_report.rebuild_checkpoint >= database_raw_mirror_capacity && 
            err_report.nr_disk_bitmap == nr_disk_mask && 
            err_report.down_disk_bitmap == down_disk_mask) {
            break;
        }
        wait_count += 10;
        fbe_api_sleep(10000);
    }

    if (err_report.rebuild_checkpoint < database_raw_mirror_capacity
        || err_report.nr_disk_bitmap != nr_disk_mask ||
        err_report.down_disk_bitmap != down_disk_mask) 
    {
        mut_printf(MUT_LOG_TEST_STATUS, "database raw_mirror rebuild timeout \n");
        MUT_ASSERT_INT_EQUAL(0, 1);
    }
#endif
}

static fbe_status_t chromatic_inject_errors_to_system_drive(fbe_u32_t fail_drives)
{
    fbe_object_id_t                                     object_id[4];
    fbe_status_t                                        status;
    fbe_u32_t                                           i;
    fbe_protocol_error_injection_error_record_t         record;
    fbe_protocol_error_injection_record_handle_t        out_record_handle_p[4];
    fbe_private_space_layout_region_t region_info;
    fbe_private_space_layout_lun_info_t lun_info;
	fbe_private_space_layout_get_region_by_region_id(FBE_PRIVATE_SPACE_LAYOUT_REGION_ID_SEP_RAW_MIRROR_RG, &region_info);
    fbe_private_space_layout_get_lun_by_lun_number(FBE_PRIVATE_SPACE_LAYOUT_LUN_ID_MCR_RAW_SYSTEM_CONFIG, &lun_info);


    fbe_test_neit_utils_init_error_injection_record(&record);
    record.lba_start                = region_info.starting_block_address + lun_info.raid_group_address_offset;
    record.lba_end                  = region_info.starting_block_address + lun_info.raid_group_address_offset + lun_info.external_capacity;
    record.num_of_times_to_insert   = FBE_U32_MAX;

    record.protocol_error_injection_error_type = FBE_PROTOCOL_ERROR_INJECTION_ERROR_TYPE_SCSI;
    record.protocol_error_injection_error.protocol_error_injection_scsi_error.scsi_command[0] = FBE_SCSI_WRITE;
    record.protocol_error_injection_error.protocol_error_injection_scsi_error.scsi_command[1] = FBE_SCSI_WRITE_6;
    record.protocol_error_injection_error.protocol_error_injection_scsi_error.scsi_command[2] = FBE_SCSI_WRITE_10;
    record.protocol_error_injection_error.protocol_error_injection_scsi_error.scsi_command[3] = FBE_SCSI_WRITE_12;
    record.protocol_error_injection_error.protocol_error_injection_scsi_error.scsi_command[4] = FBE_SCSI_WRITE_16;
    record.protocol_error_injection_error.protocol_error_injection_scsi_error.scsi_sense_key = FBE_SCSI_SENSE_KEY_HARDWARE_ERROR;
    record.protocol_error_injection_error.protocol_error_injection_scsi_error.scsi_additional_sense_code =   FBE_SCSI_ASC_GENERAL_FIRMWARE_ERROR;
    record.protocol_error_injection_error.protocol_error_injection_scsi_error.scsi_additional_sense_code_qualifier = FBE_SCSI_ASCQ_GENERAL_QUALIFIER;
    record.protocol_error_injection_error.protocol_error_injection_scsi_error.port_status = FBE_PORT_REQUEST_STATUS_SUCCESS;
    for (i = 0; i < PROTOCOL_ERROR_INJECTION_MAX_ERRORS ; i++) {
        if (i <= (record.lba_end - record.lba_start)) {
            record.b_active[i] = FBE_TRUE;
        }
    } 
    record.frequency = 0;
    record.b_test_for_media_error = FBE_FALSE;
    record.reassign_count = 0;

    for (i = 0; i < 3; i++) {
        if (fail_drives & (0x1 << i)) {
            status = fbe_api_get_physical_drive_object_id_by_location(0, 0, i, &object_id[i]);
            MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

            record.object_id = object_id[i];
            status = fbe_api_protocol_error_injection_add_record(&record, &out_record_handle_p[i]);
            MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
        }
    }

    status = fbe_api_protocol_error_injection_start();
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    return FBE_STATUS_OK;
}

/*test to use the bitmask in raw mirror correctly*/
/*
The test does the following:
1.	Pull out a system drive.
2.	Destroy the system PVD using the API. This will set the bitmask in raw mirror.
3.	Start raw mirror IO, and check whether the IO can be completed successfully.
4.	Re-insert the system PVD. This will unmask the bit mask in raw mirror.
5.	Start raw mirror IO, and check whether the IO can be completed successfully.
*/
void chromatic_raw_mirror_bitmask_test(void)
{
	fbe_status_t status = FBE_STATUS_OK;
	fbe_database_control_raw_mirror_err_report_t err_report;
	database_user_entry_t  user_entry;
    fbe_object_id_t  pvd_object_id;
	fbe_status_t  job_status;
	fbe_api_terminator_device_handle_t	drive_handle;
	
	mut_printf(MUT_LOG_TEST_STATUS, "chromatic_raw_mirror_bitmask_test\n");
    mut_printf(MUT_LOG_TEST_STATUS, "Pull out the drive 0_0_1\n");
	status = fbe_api_provision_drive_get_obj_id_by_location(0, 0, 1, &pvd_object_id);

	fbe_test_pp_util_pull_drive(0, 0, 1,&drive_handle);
	
	/*	  Print message as to where the test is at */
	mut_printf(MUT_LOG_TEST_STATUS, "Removed Drive: %d_%d_%d, PVD: 0x%X", 0, 0, 1, pvd_object_id);
			  
	/*	Verify that the PVD  are in the FAIL state */
	sep_rebuild_utils_verify_sep_object_state(pvd_object_id, FBE_LIFECYCLE_STATE_FAIL);

	/* destroy the system pvd*/
	status = fbe_api_job_service_destroy_system_pvd(pvd_object_id,&job_status);
	MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
	MUT_ASSERT_TRUE(job_status == FBE_STATUS_OK);

	mut_printf(MUT_LOG_TEST_STATUS, "Do IO on raw-3way-mirror after one drive is pulled out\n");
    chromatic_raw_mirror_io_test();

    chromatic_raw_mirror_read_user_entry(CHROMATIC_TEST_OBJECT_ID_MCR_DATABASE_LUN, 
                                         &user_entry);
    memcpy(user_entry.user_data_union.lu_user_data.world_wide_name.bytes, "mask", 4);
    chromatic_raw_mirror_write_user_entry(CHROMATIC_TEST_OBJECT_ID_MCR_DATABASE_LUN, 
                                         &user_entry);

    chromatic_get_raw_mirror_error_report(&err_report);
    mut_printf(MUT_LOG_TEST_STATUS, "raw mirror err report err: %lld, flushed: %lld, failed: %lld,"
                                     "down disk bitmap: %x, degradation bitmap: %x, nr disk %x", 
               err_report.error_count, err_report.flush_error_count, 
               err_report.failed_flush_count, err_report.down_disk_bitmap,
               err_report.degraded_disk_bitmap, err_report.nr_disk_bitmap);

    MUT_ASSERT_INT_EQUAL(err_report.degraded_disk_bitmap, 0x0);
  
    fbe_test_pp_util_reinsert_drive(0, 0, 1,drive_handle); 
    chromatic_test_wait_system_pvd_state(0, 0, 1, FBE_LIFECYCLE_STATE_READY);
	/*Print message as to where the test is at */
	mut_printf(MUT_LOG_TEST_STATUS, "Re-inserted Drive: %d_%d_%d", 0, 0, 1);

    /*wait for the rebuild to complete*/
    chromatic_wait_raw_mirror_rebuild_complete(CHROMATIC_TEST_RAW_MIRROR_REBUILD_TIMEOUT, 0, 0);

    chromatic_raw_mirror_read_user_entry(CHROMATIC_TEST_OBJECT_ID_MCR_DATABASE_LUN, 
                                         &user_entry);
    // cannot 'print' a wwn, it's not an ascii string

    //mut_printf(MUT_LOG_TEST_STATUS, "lun wwn name: %s \n", 
    //  user_entry.user_data_union.lu_user_data.world_wide_name.bytes);
   if (memcmp(user_entry.user_data_union.lu_user_data.world_wide_name.bytes,
               "mask", 4) != 0)
    {
        mut_printf(MUT_LOG_TEST_STATUS, "raw_mirror rebuild failure \n");
        MUT_ASSERT_INT_EQUAL(0, 1);
    } else
        mut_printf(MUT_LOG_TEST_STATUS, "raw_mirror rebuild check successful \n");
    
}

fbe_u32_t chromatic_get_num_of_upstream_objects(fbe_u32_t  port_number,
                                                fbe_u32_t  enclosure_number,
                                                fbe_u32_t  slot_number)
{
    fbe_u32_t    count =0;

    MUT_ASSERT_INT_EQUAL(0, port_number);
    MUT_ASSERT_INT_EQUAL(0, enclosure_number);

    switch(slot_number)
    {
        case 0: 
#ifndef __SAFE__
            count = fbe_private_space_layout_get_number_of_system_raid_groups();
#else
            count = fbe_private_space_layout_get_number_of_system_raid_groups() - 1; /* c4mirror RG build on the pair of drive 0,2 and 1,3 */
#endif
        break;

        case 1: 
#ifndef __SAFE__
            count = fbe_private_space_layout_get_number_of_system_raid_groups();
#else
            count = fbe_private_space_layout_get_number_of_system_raid_groups() - 1; /* c4mirror RG build on the pair of drive 0,2 and 1,3 */
#endif
        break;

        case 2: 
#ifndef __SAFE__
            count = fbe_private_space_layout_get_number_of_system_raid_groups();
#else
            count = fbe_private_space_layout_get_number_of_system_raid_groups() - 1; /* c4mirror RG build on the pair of drive 0,2 and 1,3 */
#endif
        break;

        case 3: 
#ifndef __SAFE__
            count = 2;
#else
            count = 3;
#endif
        break;

        default:
            MUT_ASSERT_TRUE(0);
    }

   return count;
}


static fbe_status_t chromatic_test_wait_rg_rebuild_complete(fbe_object_id_t raid_group_object_id, fbe_u32_t rb_position)
{
    fbe_status_t    status;
    fbe_lifecycle_state_t   current_state;
    
    /* Validate raid group is ready
     */
    status = fbe_api_get_object_lifecycle_state(raid_group_object_id, &current_state, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_EQUAL(FBE_LIFECYCLE_STATE_READY, current_state);

    status = fbe_test_sep_util_wait_rg_pos_rebuild(raid_group_object_id, rb_position, 
                                                   FBE_TEST_SEP_UTIL_RB_WAIT_SEC);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    return status;
}   


/*************************
 * end file chromatic_test.c
 *************************/


