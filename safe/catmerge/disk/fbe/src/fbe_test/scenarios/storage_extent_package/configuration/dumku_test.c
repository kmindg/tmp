/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file dumku_test.c
 ***************************************************************************
 *
 * @brief
 *  This file contains a PVD Pool-id test with various scenarios.
 *
 * @version
 *   6/15/2011 - Created. Arun S
 *   5/7/2012   - MOdified. Zhangy add system pvd pool id test
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "mut.h"   
#include "fbe_test_package_config.h"
#include "fbe/fbe_terminator_api.h"
#include "fbe/fbe_api_provision_drive_interface.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_utils.h"
#include "fbe/fbe_api_base_config_interface.h"
#include "fbe/fbe_api_sim_server.h"
#include "fbe/fbe_api_terminator_interface.h"
#include "fbe/fbe_api_database_interface.h"
#include "fbe_test_configurations.h"
#include "pp_utils.h"
#include "sep_utils.h"
#include "sep_tests.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_api_common_transport.h"
#include "fbe/fbe_api_virtual_drive_interface.h"
#include "fbe/fbe_api_lun_interface.h"
#include "fbe/fbe_api_raid_group_interface.h"
#include "fbe/fbe_random.h"



char * dumku_short_desc = "PVD Storage Pool-id test";
char * dumku_long_desc ="\n"
    "The Dumku scenario tests that the PVD Pool-id persists even after reboots.\n"
    "\n"
    "\n"
    "Starting Config:\n"
    "    [PP] 1 - armada board\n"
    "    [PP] 2 - SAS PMC port\n"
    "    [PP] 1 - viper enclosures\n"
    "    [PP] 10 - SAS drives (PDOs)\n"
    "    [PP] 10 - Logical drives (LDs)\n"
    "\n"
    "\n"
    "STEP 1: Bring up the initial topology\n"
    "    - Create and verify the initial physical package config\n"
    "\n"
    "STEP 2: Verify the basic PVD Pool-id scenario.\n"
    "    - Pick a PVD and set the Pool-id for that PVD.\n"
    "    - Wait for 1 monitor cycle.\n"
    "    - Get the Pool-id and verify it against the SET pool-id.\n"
    "\n"
    "STEP 3: Verify the PVD Pool-id persists across reboot.\n" 
    "    - Set the Pool-id for a specified PVD.\n"
    "    - Wait for 1 monitor cycle.\n"
    "    - Destroy and recreate the topology (Reboot).\n"
    "    - Get the Pool-id and verify it against the SET pool-id (Persisted before the reboot).\n"
    "\n"
    "STEP 4: Verify the PVD Pool-id persists even if the disk is removed and reinserted in a different slot.\n" 
    "    - Pull a drive from 'x' slot.\n"
    "    - Insert the drive (pulled from 'x' slot) in 'y' slot.\n"
    "    - Wait for 1 monitor cycle.\n"
    "    - Get the Pool-id and verify if we have the same Pool-id for the drive after re-inserting it in a different slot.\n"
    "\n"
    "Description last updated: 9/22/2011.\n";
   

/*!*******************************************************************
 * @def DUMKU_TEST_DRIVE_CAPACITY
 *********************************************************************
 * @brief Number of blocks in the physical drive.
 *
 *********************************************************************/
#define DUMKU_TEST_DRIVE_CAPACITY (TERMINATOR_MINIMUM_NON_SYSTEM_DRIVE_CAPACITY)

/*!*******************************************************************
 * @def DUMKU_TEST_NUMBER_OF_PHYSICAL_OBJECTS
 *********************************************************************
 * @brief Number of objects we will create.
 *        (1 board + 1 port + 1 encl + 9 pdo ) 
 *********************************************************************/
#define DUMKU_TEST_NUMBER_OF_PHYSICAL_OBJECTS 12

/*!*******************************************************************
 * @def DUMKU_WAIT_MSEC
 *********************************************************************
 * @brief Number of seconds we should wait for state changes.
 *        We set this relatively large so that if we do exceed this
 *        we will be sure something is wrong.
 *
 *********************************************************************/
/* Maximum number of hot spares. */
#define DUMKU_TEST_HS_COUNT   2
#define DUMKU_WAIT_MSEC 30000
#define DUMKU_TEST_LUNS_PER_RAID_GROUP 1
#define DUMKU_TEST_RAID_GROUP_COUNT 1
#define DUMKU_JOB_SERVICE_CREATION_TIMEOUT 150000 /* in ms*/

/*!*******************************************************************
 * @def DUMKU_POLLING_INTERVAL_MSEC
 *********************************************************************
 * @brief Number of msec we should wait before checking again 
 *        if the pvd reconnected
 *
 *********************************************************************/
#define DUMKU_POLLING_INTERVAL_MSEC   200

/*!*******************************************************************
 * @def DUMKU_PVD_RECONNECT_TIME
 *********************************************************************
 * @brief Number of milli-seconds we should wait for pvd to reconnect.
 * @todo  Change DUMKU_PVD_RECONNECT_TIME to 2000*3  after fix in 
 *        terminator      
 *
 *********************************************************************/
#define DUMKU_PVD_RECONNECT_TIME 2000*30 

#define DUMKU_RG_PVDS_POOL_ID 44

static fbe_object_id_t dumku_pvd_id = FBE_OBJECT_ID_INVALID;
static fbe_object_id_t dumku_system_pvd_id = FBE_OBJECT_ID_INVALID;

static fbe_u32_t expected_pool_id = FBE_POOL_ID_INVALID;

static fbe_test_rg_configuration_t dumku_rg_config[DUMKU_TEST_RAID_GROUP_COUNT] = 
{
 /* width, capacity,        raid type,                        class,        block size, RAID-id(let MCR pick),    bandwidth.*/
    3,     FBE_LBA_INVALID, FBE_RAID_GROUP_TYPE_RAID5, FBE_CLASS_ID_PARITY, 512,        FBE_RAID_ID_INVALID,          0,
    /* rg_disk_set */
    {{0,0,4}, {0,0,5}, {0,0,6}}
};

fbe_test_hs_configuration_t dumku_hs_config[DUMKU_TEST_HS_COUNT] =
{
    /* disk set     block size           object-id.         */
    {{0, 0, 7},     520,                 FBE_OBJECT_ID_INVALID},
    {{0, 0, 8},     520,                 FBE_OBJECT_ID_INVALID},
};

/*****************************************
 * LOCAL FUNCTION DEFINITIONS
 *****************************************/
static void dumku_physical_config(void);
static void dumku_verify_pvd_pool_id(void);
static void dumku_reboot_and_verify_pvd_pool_id(fbe_u32_t expected_pool_id);
static void dumku_swap_and_verify_pvd_pool_id(fbe_u32_t expected_pool_id);
static void dumku_test_create_rg(void);
static void dumku_verify_system_pvd_pool_id(void);
static void dumku_reboot_and_verify_system_pvd_pool_id(fbe_u32_t expected_pool_id);
static fbe_status_t dumku_unbind_all_luns(fbe_test_rg_configuration_t *rg_config_p);

/*!****************************************************************************
 *  dumku_setup
 ******************************************************************************
 *
 * @brief
 *   This is the setup function for the dumku test.  
 *
 * @param   None
 *
 * @return  None 
 *
 *****************************************************************************/
void dumku_setup(void)
{    
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t           slot;
    fbe_u32_t           port;
    fbe_u32_t           enclosure;

    mut_printf(MUT_LOG_LOW, "=== dumku setup ===\n");       

    /* load the physical package */
    dumku_physical_config();

    /* load the logical package */
    sep_config_load_sep_and_neit();

	dumku_test_create_rg();

    // Lookup the ids of the pvds
    port = 0;
    enclosure = 0;
    slot = 7;/*choosing drive 7 leaves drive as the only drive eligible to be swapped in as a spare and we'll verify it later*/
    status = fbe_api_provision_drive_get_obj_id_by_location(port, enclosure, slot, &dumku_pvd_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
	/*choosing 0-0-1 to test system pvd pool id*/
    status = fbe_api_provision_drive_get_obj_id_by_location(0, 0, 1, &dumku_system_pvd_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);   

    return;
}
/***************************************************************
 * end dumku_setup()
 ***************************************************************/


/*!**************************************************************
 * dumku_physical_config()
 ****************************************************************
 * @brief
 *  Configure the yeti test's physical configuration.
 *
 * @param None.              
 *
 * @return None.
 *
 *
 ****************************************************************/

static void dumku_physical_config(void)
{
    fbe_status_t                        status  = FBE_STATUS_GENERIC_FAILURE;

    fbe_u32_t                           total_objects = 0;
    fbe_class_id_t                      class_id;

    fbe_terminator_api_device_handle_t  port0_handle;
    fbe_terminator_api_device_handle_t  encl0_0_handle;
    fbe_terminator_api_device_handle_t  drive_handle;
    fbe_u32_t slot;
    fbe_object_id_t object_id;

    mut_printf(MUT_LOG_TEST_STATUS, "%s: using new creation API and Terminator Class Management\n", __FUNCTION__);
    mut_printf(MUT_LOG_LOW, "=== configuring terminator ===\n");
    /*before loading the physical package we initialize the terminator*/
    status = fbe_api_terminator_init();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    
    /* insert a board
     */
    status = fbe_test_pp_util_insert_armada_board();
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* insert a port
     */
    fbe_test_pp_util_insert_sas_pmc_port(1, /* io port */
                                         2, /* portal */
                                         0, /* backend number */ 
                                         &port0_handle);

   /* insert an enclosures to port 0
     */
    fbe_test_pp_util_insert_viper_enclosure(0, 0, port0_handle, &encl0_0_handle);

    /* Insert drives for enclosures.
     */
    for ( slot = 0; slot < 9; slot++)
    {
        fbe_test_pp_util_insert_sas_drive(0, 0, slot, 520, DUMKU_TEST_DRIVE_CAPACITY, &drive_handle);
    }

    /* Init fbe api on server */
    mut_printf(MUT_LOG_LOW, "=== starting Server FBE_API ===\n");
    status = fbe_api_sim_server_init_fbe_api();
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_LOW, "=== starting physical package===\n");
    status = fbe_api_sim_server_load_package(FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_LOW, "=== set Physical Package entries ===\n");
    status = fbe_api_sim_server_set_package_entries(FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_LOW, "=== starting system discovery ===\n");
    /* wait for the expected number of objects */
    status = fbe_api_wait_for_number_of_objects(DUMKU_TEST_NUMBER_OF_PHYSICAL_OBJECTS, 20000, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_TRUE_MSG(status == FBE_STATUS_OK, "Wait for number of objects failed!");

    mut_printf(MUT_LOG_LOW, "=== verifying configuration ===\n");

    /* We inserted a armada board so we check for it.
     * board is always assumed to be object id 0.
     */
    status = fbe_api_get_object_class_id (0, &class_id, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_TRUE(class_id == FBE_CLASS_ID_ARMADA_BOARD);
    mut_printf(MUT_LOG_LOW, "Verifying board type....Passed");

    /* Make sure we have the expected number of objects.
     */
    status = fbe_api_get_total_objects(&total_objects, FBE_PACKAGE_ID_PHYSICAL);
    MUT_ASSERT_TRUE(total_objects == DUMKU_TEST_NUMBER_OF_PHYSICAL_OBJECTS);
    mut_printf(MUT_LOG_LOW, "Verifying object count...Passed");

    for (object_id = 0 ; object_id < DUMKU_TEST_NUMBER_OF_PHYSICAL_OBJECTS; object_id++)
    {
        status = fbe_api_wait_for_object_lifecycle_state(object_id, FBE_LIFECYCLE_STATE_READY, 20000, FBE_PACKAGE_ID_PHYSICAL);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    }
    return;
}
/******************************************
 * end dumku_physical_config()
 ******************************************/

/*!****************************************************************************
 *  dumku_test
 ******************************************************************************
 *
 * @brief
 *   This is the main entry point for the dumku test.  
 *
 *
 * @param   None
 *
 * @return  None 
 *
 *****************************************************************************/
void dumku_test(void)
{


    mut_printf(MUT_LOG_LOW, "=== Dumku test started ===\n");   
	
    /* Verify the Pool-id for the PVD */
    dumku_verify_pvd_pool_id();

    /* Reboot and verify the PVD pool-id */
    dumku_reboot_and_verify_pvd_pool_id(expected_pool_id);

    /* Move the drive to a different slot and verify the PVD Pool-id */
    dumku_swap_and_verify_pvd_pool_id(expected_pool_id);

    mut_printf(MUT_LOG_LOW, "=== Dumku test completed ===\n");  

    mut_printf(MUT_LOG_LOW, "=== Dumku system pvd test started ===\n");    

    /* Verify the Pool-id for the PVD */
    dumku_verify_system_pvd_pool_id();

    /* Reboot and verify the PVD pool-id */
    dumku_reboot_and_verify_system_pvd_pool_id(expected_pool_id);

    /* Unbind all luns. */
    dumku_unbind_all_luns(&dumku_rg_config[0]);

    mut_printf(MUT_LOG_LOW, "=== Dumku system test completed ===\n");  

    return;

}
/***************************************************************
 * end dumku_test()
 ***************************************************************/

/*!****************************************************************************
 *  dumku_verify_pvd_pool_id
 ******************************************************************************
 *
 * @brief
 *   This function verifies the PVD pool-id.
 *
 *
 * @param   None
 *
 * @return  None 
 *
 *****************************************************************************/
void dumku_verify_pvd_pool_id(void)
{
    fbe_u32_t                                   actual_pool_id = 0;
    fbe_status_t                                status;

    mut_printf(MUT_LOG_TEST_STATUS, "%s: Verifying the GET/SET pool-id API for this PVD.\n", __FUNCTION__);

    actual_pool_id = 5;
   
    /* Persist the pool-id on this provision drive */
    mut_printf(MUT_LOG_TEST_STATUS, "%s: Persist the PVD pool-id in DB.\n", __FUNCTION__);
    fbe_test_sep_util_provision_drive_pool_id(dumku_pvd_id, actual_pool_id);

	/* Wait for 1 monitor cycle to complete */
    fbe_api_sleep(3500);

    status = fbe_api_provision_drive_get_pool_id(dumku_pvd_id, &expected_pool_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);   
	MUT_ASSERT_INTEGER_EQUAL_MSG(actual_pool_id, expected_pool_id, "Unexpected Pool id");

    mut_printf(MUT_LOG_TEST_STATUS, "%s: New Pool id for the PVD(0x%x) in DB is: %d\n", 
               __FUNCTION__, dumku_pvd_id, actual_pool_id);

    return;
}

/*!****************************************************************************
 *  dumku_reboot_and_verify_pvd_pool_id
 ******************************************************************************
 *
 * @brief
 *   This function verifies the PVD pool-id after the reboot.
 *
 *
 * @param   None
 *
 * @return  None 
 *
 *****************************************************************************/
void dumku_reboot_and_verify_pvd_pool_id(fbe_u32_t expected_pool_id)
{
    fbe_provision_drive_control_pool_id_t       pvd_pool_id;
    fbe_status_t                                status;

    mut_printf(MUT_LOG_LOW, "=== Rebooting the SP ===\n");
    /*reboot sep*/
    fbe_test_sep_util_destroy_neit_sep();
    sep_config_load_sep_and_neit();
    status = fbe_test_sep_util_wait_for_database_service(DUMKU_WAIT_MSEC);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Wait for 1 monitor cycle to complete */
    fbe_api_sleep(3500);

    /* Now get the pool-id for the PVD. It is Persisted and so should contain the new id. */
    status = fbe_api_provision_drive_get_pool_id(dumku_pvd_id, &pvd_pool_id.pool_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);   
    mut_printf(MUT_LOG_TEST_STATUS, "%s: New Pool id for the PVD(0x%x) in DB is: %d\n", 
               __FUNCTION__, dumku_pvd_id, pvd_pool_id.pool_id);
    MUT_ASSERT_INTEGER_EQUAL_MSG(expected_pool_id, pvd_pool_id.pool_id, "Unexpected Pool id");
}

/*!****************************************************************************
 *  dumku_swap_and_verify_pvd_pool_id
 ******************************************************************************
 *
 * @brief
 *   This function verifies the PVD pool-id after the drive is swapped to a
 *   different slot.
 *
 *
 * @param   None
 *
 * @return  None 
 *
 *****************************************************************************/
void dumku_swap_and_verify_pvd_pool_id(fbe_u32_t expected_pool_id)
{
    fbe_provision_drive_control_pool_id_t       	pvd_pool_id;
    fbe_status_t                                	status;
    fbe_api_terminator_device_handle_t          	drive_handle;
    fbe_u32_t                                   	wait_time_ms=0;
	fbe_object_id_t									vd_object_id, rg_object_id, pvd_id, lun_id;
	fbe_edge_index_t                				edge_index;	
	fbe_api_base_config_downstream_object_list_t	pvd_list;
	fbe_api_provision_drive_info_t					pvd_info;
	fbe_api_lun_create_t							create_lun;
	fbe_lun_number_t								temp_number;
	fbe_object_id_t									temp_pvd_id;
	fbe_provision_drive_copy_to_status_t			copy_status;
	fbe_api_base_config_downstream_object_list_t	edge_list;
    fbe_job_service_error_type_t                    job_error_type;
    fbe_u32_t                                       position_to_remove = 2;

    fbe_zero_memory(&create_lun, sizeof(fbe_api_lun_create_t));
	mut_printf(MUT_LOG_LOW, "=== Testing Automatic LU # assignment ===");
	/*this next sub test does not exactly belong here but there are no other dedicated tests for this small thing.
	Since dumku test is dealing with user related setting this makes some sense*/
	create_lun.capacity = 0x1000;
	create_lun.lun_number = FBE_LUN_ID_INVALID;
	create_lun.ndb_b = FBE_FALSE;
	create_lun.raid_group_id = dumku_rg_config[0].raid_group_id;
	create_lun.raid_type = FBE_RAID_GROUP_TYPE_RAID5;
    create_lun.placement = FBE_BLOCK_TRANSPORT_BEST_FIT;
    create_lun.noinitialverify_b = FBE_FALSE;
    create_lun.addroffset = FBE_LBA_INVALID; /* set to a valid offset for NDB */
    create_lun.world_wide_name.bytes[0] = (fbe_u8_t)fbe_random() & 0xf;
    create_lun.world_wide_name.bytes[1] = (fbe_u8_t)fbe_random() & 0xf;

	status = fbe_api_create_lun(&create_lun, FBE_TRUE, 20000, &lun_id, &job_error_type);
	MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK); 
	MUT_ASSERT_INT_NOT_EQUAL(create_lun.lun_number, FBE_LUN_ID_INVALID); 
	temp_number = create_lun.lun_number;/*remember hte lun number we got here*/

    /* We need to change the wwn of the lun */
	create_lun.lun_number = FBE_LUN_ID_INVALID;
    create_lun.world_wide_name.bytes[0] = (fbe_u8_t)fbe_random() & 0xf;
    create_lun.world_wide_name.bytes[1] = (fbe_u8_t)fbe_random() & 0xf;
        
	status = fbe_api_create_lun(&create_lun, FBE_TRUE, 20000, &lun_id, &job_error_type);
	MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK); 
	MUT_ASSERT_INT_NOT_EQUAL(create_lun.lun_number, FBE_LUN_ID_INVALID);
	MUT_ASSERT_TRUE(temp_number != create_lun.lun_number);

    /* Bind all the luns on this raid group.
     */
    fbe_test_sep_util_fill_lun_configurations_rounded(dumku_rg_config, DUMKU_TEST_RAID_GROUP_COUNT, 
                                                      1, DUMKU_TEST_LUNS_PER_RAID_GROUP);

    /* Create a set of LUNs for this RAID group. */
    status = fbe_test_sep_util_create_logical_unit_configuration(dumku_rg_config->logical_unit_configuration_list,
                                                                 dumku_rg_config->number_of_luns_per_rg);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Creation of Logical units failed.");
    mut_printf(MUT_LOG_TEST_STATUS, "%s: END\n", __FUNCTION__);

    fbe_test_sep_util_update_permanent_spare_trigger_timer(1); /* 1 second hotspare timeout */

	status = fbe_api_database_lookup_raid_group_by_number(dumku_rg_config[0].raid_group_id, &rg_object_id);

    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, rg_object_id);

	fbe_test_sep_util_get_virtual_drive_object_id_by_position(rg_object_id, position_to_remove, &vd_object_id);

    mut_printf(MUT_LOG_LOW, "=== Pulling the Drive from 0_0_6 to make RG swap a spare===");

	status = fbe_api_provision_drive_get_obj_id_by_location(0, 0, 6, &pvd_id);
	MUT_ASSERT_INT_EQUAL (status, FBE_STATUS_OK);

    status = fbe_test_pp_util_pull_drive(0, 0, 6, &drive_handle); 
    MUT_ASSERT_INT_EQUAL (status, FBE_STATUS_OK);

	/* Verify the SEP objects is in expected lifecycle State  */
    status = fbe_api_wait_for_object_lifecycle_state(pvd_id,
                                                     FBE_LIFECYCLE_STATE_FAIL,
                                                     15000,
                                                     FBE_PACKAGE_ID_SEP_0);
	MUT_ASSERT_INT_EQUAL (status, FBE_STATUS_OK);

   /* Get the edge index where the permanent spare swaps in. */
    fbe_test_sep_drive_get_permanent_spare_edge_index(vd_object_id, &edge_index);

    /* Wait for spare to swap-in. we'll check which drive was swapped leter*/
    status = fbe_api_wait_for_block_edge_path_state(vd_object_id,
                                                    edge_index,
                                                    FBE_PATH_STATE_ENABLED,
                                                    FBE_PACKAGE_ID_SEP_0,
                                                    60000);
    MUT_ASSERT_INT_EQUAL (status, FBE_STATUS_OK);

    /* Now get the pool-id for the PVD. It is Persisted and so should contain the new id. */
    status = fbe_api_provision_drive_get_pool_id(dumku_pvd_id, &pvd_pool_id.pool_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);   
    mut_printf(MUT_LOG_TEST_STATUS, "%s: New Pool id for the PVD(0x%x) in DB is: %d\n", 
               __FUNCTION__, dumku_pvd_id, pvd_pool_id.pool_id);
    MUT_ASSERT_INTEGER_EQUAL_MSG(expected_pool_id, pvd_pool_id.pool_id, "Unexpected Pool id");

    /*let's make sure the RG swapped in drive 8 and not 7 which was marked with a pool ID*/
	status = fbe_api_base_config_get_downstream_object_list(vd_object_id, &pvd_list);
	MUT_ASSERT_INT_EQUAL (status, FBE_STATUS_OK);
	MUT_ASSERT_INT_EQUAL(pvd_list.number_of_downstream_objects, 1);/*better have one disk*/

	status = fbe_api_provision_drive_get_info(pvd_list.downstream_object_list[0],&pvd_info);
	MUT_ASSERT_INT_EQUAL (status, FBE_STATUS_OK);
	MUT_ASSERT_INT_EQUAL(pvd_info.slot_number , 8);

    /* Wait for the job to complete */
    status = fbe_test_sep_util_wait_for_swap_request_to_complete(vd_object_id);
	MUT_ASSERT_INT_EQUAL (status, FBE_STATUS_OK);

	mut_printf(MUT_LOG_LOW, "=== checking new swapped in drive got same pool ID as swapped out drive ===");
	status = fbe_api_provision_drive_get_pool_id(pvd_list.downstream_object_list[0], &pvd_pool_id.pool_id);
	MUT_ASSERT_INT_EQUAL (status, FBE_STATUS_OK);
	MUT_ASSERT_INT_EQUAL(pvd_pool_id.pool_id , DUMKU_RG_PVDS_POOL_ID);

	mut_printf(MUT_LOG_LOW, "=== checking old swapped out drive got an invalid pool ID ===");
	/*make sure the swapped out PVD (which is still there in memory) has an invalid pool on it*/
	status = fbe_api_provision_drive_get_pool_id(pvd_id, &pvd_pool_id.pool_id);
	MUT_ASSERT_INT_EQUAL (status, FBE_STATUS_OK);
	MUT_ASSERT_INT_EQUAL(pvd_pool_id.pool_id , FBE_POOL_ID_INVALID);

	mut_printf(MUT_LOG_LOW, "=== Verified RG swapped in a drive which is not in a pool ===");

	/*now let's make sure that as move move a drive with a pool id around, it keeps it's pool id*/
	mut_printf(MUT_LOG_LOW, "=== Verifying the Pool-id of the same drive from a different location ===");
	status = fbe_api_provision_drive_get_obj_id_by_location(0, 0, 7, &dumku_pvd_id);
	MUT_ASSERT_INT_EQUAL (status, FBE_STATUS_OK);
	temp_pvd_id = dumku_pvd_id;
	mut_printf(MUT_LOG_LOW, "=== Pulling the Drive and re-inserting it on a different slot ===\n");
	status = fbe_test_pp_util_pull_drive(0, 0, 7, &drive_handle); 
	MUT_ASSERT_INT_EQUAL (status, FBE_STATUS_OK);

	fbe_api_sleep(2000);/*let the simualtion system process the move*/
	
	mut_printf(MUT_LOG_LOW, "=== Re-inserting the Drive from 0_0_7 to 0_0_10 ===");
	status = fbe_test_pp_util_reinsert_drive(0, 0, 10, drive_handle);
	MUT_ASSERT_INT_EQUAL (status, FBE_STATUS_OK);

	while (wait_time_ms < DUMKU_PVD_RECONNECT_TIME)
	{
		status = fbe_api_provision_drive_get_obj_id_by_location(0, 0, 10, &dumku_pvd_id);
		if (status == FBE_STATUS_OK )
			break;
		wait_time_ms+=DUMKU_POLLING_INTERVAL_MSEC;
		fbe_api_sleep(DUMKU_POLLING_INTERVAL_MSEC);
	}

	MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK); 
	MUT_ASSERT_INT_EQUAL(temp_pvd_id, dumku_pvd_id); 
	status = fbe_api_provision_drive_get_pool_id(dumku_pvd_id, &pvd_pool_id.pool_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);   
	MUT_ASSERT_INT_EQUAL(pvd_pool_id.pool_id, 5);   

    /* Wait for raid group to become optimal otherwise the copy will fail.*/
    mut_printf(MUT_LOG_TEST_STATUS, "waiting for rebuild rg_id: %d obj: 0x%x position: %d",
               dumku_rg_config[0].raid_group_id, rg_object_id, position_to_remove);
    status = fbe_test_sep_util_wait_rg_pos_rebuild(rg_object_id, position_to_remove,
                                                   30000/* Time to wait */);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

	/*now, let's copy stuff from the hot spare to another drive and make sure that as the copy is over
	the pool ID of the drive we copied from moved as well to the new drive*/
	mut_printf(MUT_LOG_LOW, "=== copying data from 0_0_8 on a RG to a free drive on 0_0_10 ===");
	status = fbe_api_provision_drive_get_obj_id_by_location(0, 0, 8, &pvd_id);
	MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK); 
	status = fbe_api_copy_to_replacement_disk(pvd_id, dumku_pvd_id, &copy_status);
	MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
	MUT_ASSERT_INT_EQUAL(copy_status, FBE_PROVISION_DRIVE_COPY_TO_STATUS_NO_ERROR);

	mut_printf(MUT_LOG_LOW, "=== copying worked, checking the pool moved ===");
	/*wait for the VD to have only one edge which means the copy is over*/
	wait_time_ms = 0;
	do{
		status = fbe_api_base_config_get_downstream_object_list(vd_object_id, &edge_list);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
		wait_time_ms+=DUMKU_POLLING_INTERVAL_MSEC;
		fbe_api_sleep(DUMKU_POLLING_INTERVAL_MSEC);
	}while ((edge_list.number_of_downstream_objects != 1) && (wait_time_ms < DUMKU_PVD_RECONNECT_TIME));

    /* Wait for the job to complete */
    status = fbe_test_sep_util_wait_for_swap_request_to_complete(vd_object_id);
	MUT_ASSERT_INT_EQUAL (status, FBE_STATUS_OK);
    
	MUT_ASSERT_INT_NOT_EQUAL(wait_time_ms, DUMKU_POLLING_INTERVAL_MSEC);
	status = fbe_api_provision_drive_get_pool_id(dumku_pvd_id, &pvd_pool_id.pool_id);
	MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
	MUT_ASSERT_INT_EQUAL(pvd_pool_id.pool_id, DUMKU_RG_PVDS_POOL_ID);   

	/*the drive we copied from shoul now have invalid pool ID*/
	status = fbe_api_provision_drive_get_pool_id(pvd_id, &pvd_pool_id.pool_id);
	MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
	MUT_ASSERT_INT_EQUAL(pvd_pool_id.pool_id, FBE_POOL_ID_INVALID);   
	mut_printf(MUT_LOG_LOW, "=== Veryfied pool ID moved with a copy opertion ===");

}

/*!****************************************************************************
 *  dumku_verify_system_pvd_pool_id
 ******************************************************************************
 *
 * @brief
 *   This function verifies the system PVD pool-id.
 *
 *
 * @param   None
 *
 * @return  None 
 *
 *****************************************************************************/
static void dumku_verify_system_pvd_pool_id(void)
{
    fbe_u32_t                                   actual_pool_id = FBE_POOL_ID_INVALID;
    fbe_status_t                                status;
    mut_printf(MUT_LOG_TEST_STATUS, "%s: Verifying the GET/SET System pool-id API for this PVD.\n", __FUNCTION__);

    expected_pool_id = 0;
	/*check the system pvd's pool id is initialized as FBE_POOL_ID_INVALID*/
	
    status = fbe_api_provision_drive_get_pool_id(dumku_system_pvd_id, &expected_pool_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);   
	MUT_ASSERT_INTEGER_EQUAL_MSG(actual_pool_id, expected_pool_id, "Unexpected Pool id");
    mut_printf(MUT_LOG_TEST_STATUS, "%s: Initial Pool id for the PVD(0x%x) in DB is: %X\n", 
               __FUNCTION__, dumku_system_pvd_id, expected_pool_id);

    actual_pool_id = 5;
	/* Persist the pool-id on this provision drive */
    mut_printf(MUT_LOG_TEST_STATUS, "%s: Persist the PVD pool-id in DB.\n", __FUNCTION__);
    fbe_test_sep_util_provision_drive_pool_id(dumku_system_pvd_id, actual_pool_id);

	/* Wait for 1 monitor cycle to complete */
    fbe_api_sleep(3500);

    status = fbe_api_provision_drive_get_pool_id(dumku_system_pvd_id, &expected_pool_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);   
	MUT_ASSERT_INTEGER_EQUAL_MSG(actual_pool_id, expected_pool_id, "Unexpected Pool id");

    mut_printf(MUT_LOG_TEST_STATUS, "%s: New Pool id for the PVD(0x%x) in DB is: %d\n", 
               __FUNCTION__, dumku_system_pvd_id, actual_pool_id);

    return;
}

/*!****************************************************************************
 *  dumku_reboot_and_verify_system_pvd_pool_id
 ******************************************************************************
 *
 * @brief
 *   This function verifies the system PVD pool-id after the reboot.
 *
 *
 * @param   None
 *
 * @return  None 
 *
 *****************************************************************************/
static void dumku_reboot_and_verify_system_pvd_pool_id(fbe_u32_t expected_pool_id)
{
    fbe_provision_drive_control_pool_id_t       pvd_pool_id;
    fbe_status_t                                status;

    mut_printf(MUT_LOG_LOW, "=== Rebooting the SP ===\n");
    /*reboot sep*/
    fbe_test_sep_util_destroy_neit_sep();
    sep_config_load_sep_and_neit();
    status = fbe_test_sep_util_wait_for_database_service(DUMKU_WAIT_MSEC);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Wait for 1 monitor cycle to complete */
    fbe_api_sleep(3500);

    /* Now get the pool-id for the PVD. It is Persisted and so should contain the new id. */
    status = fbe_api_provision_drive_get_pool_id(dumku_system_pvd_id, &pvd_pool_id.pool_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);   
    mut_printf(MUT_LOG_TEST_STATUS, "%s: New Pool id for the PVD(0x%x) in DB is: %d\n", 
               __FUNCTION__, dumku_system_pvd_id, pvd_pool_id.pool_id);
    MUT_ASSERT_INTEGER_EQUAL_MSG(expected_pool_id, pvd_pool_id.pool_id, "Unexpected Pool id");
}
/*!***************************************************************************
 *      dumku_unbind_all_luns()
 *****************************************************************************
 *
 * @brief   Unbind all the luns on the raid group specified.
 *
 * @param   rg_config_p - Pointer to raid group to unbind all luns on
 *
 * @return  status - Typically FBE_STATUS_OK
 *
 * @author
 *  05/23/2012  Ron Proulx  - Created.
 *****************************************************************************/
static fbe_status_t dumku_unbind_all_luns(fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_status_t    status = FBE_STATUS_OK;
    fbe_u32_t       lu_index;
    fbe_test_logical_unit_configuration_t *logical_unit_configuration_list_p = NULL;

    /* Only unbind the luns if they exist.*/
    if (rg_config_p->number_of_luns_per_rg == 0)
    {
        return FBE_STATUS_OK;
    }

    /* Unbind all the luns on this raid group.
     */
    status = fbe_test_sep_util_destroy_logical_unit_configuration(rg_config_p->logical_unit_configuration_list,
                                                                  rg_config_p->number_of_luns_per_rg);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Destroy of Logical units failed.");

    logical_unit_configuration_list_p = &rg_config_p->logical_unit_configuration_list[0];
    for (lu_index = 0; lu_index < rg_config_p->number_of_luns_per_rg; lu_index++)
    {
        fbe_zero_memory(logical_unit_configuration_list_p, sizeof(*logical_unit_configuration_list_p));
        logical_unit_configuration_list_p++;
    }
    rg_config_p->number_of_luns_per_rg = 0;

    return status;
}
/**************************************
 * end dumku_unbind_all_luns()
 **************************************/

/*!**************************************************************
 * dumku_test_create_rg()
 ****************************************************************
 * @brief
 *  Create the RG for the dumku test to test the spare swap-in.
 *
 * @param None.              
 *
 * @return None.
 *
 ****************************************************************/

static void dumku_test_create_rg(void)
{
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_object_id_t                     rg_object_id = FBE_OBJECT_ID_INVALID;
    fbe_api_raid_group_create_t			create_rg;
	fbe_u32_t 							disk;
	fbe_object_id_t 					pvd_id;
    fbe_job_service_error_type_t        job_error_type;

    /* Create a RG */
    mut_printf(MUT_LOG_TEST_STATUS, "=== Create a RAID Group ===\n");
    fbe_zero_memory(&create_rg, sizeof(fbe_api_raid_group_create_t));
	create_rg.b_bandwidth = dumku_rg_config->b_bandwidth;
	create_rg.capacity = dumku_rg_config->capacity;
	create_rg.drive_count = dumku_rg_config->width;
	//create_rg.explicit_removal = FBE_FALSE;
	create_rg.is_private = FBE_FALSE;
	create_rg.max_raid_latency_time_is_sec = 120;
	create_rg.power_saving_idle_time_in_seconds = 0xFFFFFFFFFF;
	create_rg.raid_group_id = FBE_RAID_ID_INVALID;
	create_rg.raid_type = FBE_RAID_GROUP_TYPE_RAID5;
	fbe_copy_memory(create_rg.disk_array , dumku_rg_config->rg_disk_set, dumku_rg_config->width * sizeof(fbe_test_raid_group_disk_set_t));
	

    status = fbe_api_create_rg(&create_rg, FBE_TRUE, 20000, &rg_object_id, &job_error_type);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "RG creation failed");

   /* Verify the object id of the raid group */
	dumku_rg_config[0].raid_group_id = create_rg.raid_group_id;
    status = fbe_api_database_lookup_raid_group_by_number(dumku_rg_config[0].raid_group_id, &rg_object_id);

    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, rg_object_id);

    mut_printf(MUT_LOG_TEST_STATUS, "Created Raid Group object %d\n", rg_object_id);

	/*set the pool ID of the PVDs in this RG, we'll test later that as we swap hot spares, the pool is being passed with it*/
    for (disk = 0 ; disk <dumku_rg_config->width ; disk++) {
		fbe_api_provision_drive_get_obj_id_by_location(dumku_rg_config->rg_disk_set[disk].bus,
													   dumku_rg_config->rg_disk_set[disk].enclosure,
													   dumku_rg_config->rg_disk_set[disk].slot, &pvd_id);

		fbe_test_sep_util_provision_drive_pool_id(pvd_id, DUMKU_RG_PVDS_POOL_ID);
	}
	
    /* Bind all the luns on this raid group.
     */
    fbe_test_sep_util_fill_lun_configurations_rounded(dumku_rg_config, DUMKU_TEST_RAID_GROUP_COUNT, 
                                                      1, DUMKU_TEST_LUNS_PER_RAID_GROUP);

    /* Create a set of LUNs for this RAID group. */
    status = fbe_test_sep_util_create_logical_unit_configuration(dumku_rg_config->logical_unit_configuration_list,
                                                                 dumku_rg_config->number_of_luns_per_rg);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "Creation of Logical units failed.");
    mut_printf(MUT_LOG_TEST_STATUS, "%s: END\n", __FUNCTION__);

    return;

} /* End dumku_test_create_rg() */

/*!****************************************************************************
 *  dumku_destroy
 ******************************************************************************
 *
 * @brief
 *   This function unloads the logical and physical packages and destroy the configuration.
 *
 * @param   None
 *
 * @return  None 
 *
 *****************************************************************************/
void dumku_destroy(void)
{
    mut_printf(MUT_LOG_LOW, "=== dumku destroy starts ===\n");
    fbe_test_sep_util_destroy_neit_sep_physical();
    mut_printf(MUT_LOG_LOW, "=== dumku destroy completes ===\n");

    return;
}
/***************************************************************
 * end dumku_destroy()
 ***************************************************************/


