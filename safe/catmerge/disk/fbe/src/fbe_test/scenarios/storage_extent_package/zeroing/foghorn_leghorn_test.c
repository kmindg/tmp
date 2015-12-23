/***************************************************************************
 * Copyright (C) EMC Corporation 2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file foghorn_leghorn_test.c
 ***************************************************************************
 *
 * @brief
 *   This file contains tests of non configured drive zeroing with failures.
 *
 * @version
 *   10/10/2011 - Created. Jason White
 *
 ***************************************************************************/


/*************************
 *   INCLUDE FILES
 *************************/
#include "mut.h"
#include "fbe_test_package_config.h"
#include "fbe/fbe_terminator_api.h"
#include "fbe/fbe_api_provision_drive_interface.h"
#include "fbe/fbe_api_raid_group_interface.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_utils.h"
#include "fbe/fbe_api_base_config_interface.h"
#include "fbe/fbe_api_sim_server.h"

#include "fbe_test_common_utils.h"
#include "fbe_test_configurations.h"
#include "pp_utils.h"
#include "sep_utils.h"
#include "sep_rebuild_utils.h"
#include "sep_zeroing_utils.h"
#include "sep_tests.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_api_common_transport.h"
#include "fbe/fbe_api_scheduler_interface.h"
#include "fbe/fbe_api_database_interface.h"

//-----------------------------------------------------------------------------
//  TEST DESCRIPTION:

char* foghorn_leghorn_short_desc = "New drive zeroing (on unbound drive) - Error cases";
char* foghorn_leghorn_long_desc =
    "\n"
    "\n"
    "This scenario validates background disk zeroing operation for unbound drive in\n"
    "single SP with different failures.\n"
    "\n"
    "\n"
    "******* Foghorn Leghorn **************************************************************\n"
    "\n"
    "\n"
    "Dependencies:\n"
    "    - LD object should support Disk zero IO.\n"
    "\n"
    "Starting Config:\n"
    "    [PP] armada board\n"
    "    [PP] SAS PMC port\n"
    "    [PP] viper enclosure\n"
    "    [PP] 1 SAS drives (PDOs)\n"
    "    [PP] 1 logical drives (LDs)\n"
    "    [SEP] 1 provision drives (PVDs)\n"
    "\n"
    "STEP A: Setup - Bring up the initial topology for local SP\n"
    "    - Insert a new enclosure having one non configured drives\n"
    "    - Create a PVD object\n"
    "\n"
    "TEST 1: Single SP test - Verify Disk zeroing operation with disk failure \n"
    "\n"
    "STEP 1: Disk zero validation\n"
    "        (Disk zero preset condition already set to start disk zero process.)\n"
    "    - Read the non paged metadata for PVD1 using config service\n"
    "    - Check that the checkpoint value should not be 0xFFFFFFFF\n"
    "    - Check that there is no upstream edge connected to PVDs.\n"
    "      This PVD should not be part of any Raid Group\n"
    "    - Check that PVD object should be in READY state\n"
    "\n"
    "STEP 2: Start Disk zeroing \n"
    "    - Read the paged metadata for PVD1 from disk using config service\n"
    "    - Find the first chunk from bitmap data to perform disk zero IO\n"
    "    - Sends the disk zero IO from PVD to LD object\n"
    "\n"
	"STEP 3: Remove a drive and verify Disk zeroing stopped\n"
	"\n"
	"STEP 4: Reinsert drive and verify Disk zeroing started again\n"
	"\n"
    "STEP 5: Verify the disk zeroing operation\n"
    "    - Verify the progress of disk zero operation with updating the checkpoint. After completion\n"
    "      of each disk zero IO, update the paged metadata first and then update the non paged metadata.\n"
    "    - Send disk zero IO for next chunk to perform disk zeroing\n"
    "\n"
    "STEP 6: Disk zeroing complete\n"
    "    - When PVD found that all chunks of disk have zeroing completed, it removes the disk zero preset\n"
    "      condition and update the paged and non paged metadata with disk zeroing completed.\n"
	"\n"
	"TEST 2: Single SP test - Verify Disk zeroing operation with RG creation \n"
	"\n"
	"STEP 1: Disk zero validation\n"
	"        (Disk zero preset condition already set to start disk zero process.)\n"
	"    - Read the non paged metadata for PVD1 using config service\n"
	"    - Check that the checkpoint value should not be 0xFFFFFFFF\n"
	"    - Check that there is no upstream edge connected to PVDs.\n"
	"      This PVD should not be part of any Raid Group\n"
	"    - Check that PVD object should be in READY state\n"
	"\n"
	"STEP 2: Start Disk zeroing \n"
	"    - Read the paged metadata for PVD1 from disk using config service\n"
	"    - Find the first chunk from bitmap data to perform disk zero IO\n"
	"    - Sends the disk zero IO from PVD to LD object\n"
	"\n"
	"STEP 3: Create RG\n"
	"\n"
	"STEP 4: Ensure that Zeroing checkpoint moves back to the PBA of the RG paged metadata\n"
	"\n"
	"STEP 5: Verify the disk zeroing operation\n"
	"    - Verify the progress of disk zero operation with updating the checkpoint. After completion\n"
	"      of each disk zero IO, update the paged metadata first and then update the non paged metadata.\n"
	"    - Send disk zero IO for next chunk to perform disk zeroing\n"
	"\n"
	"STEP 6: Disk zeroing complete\n"
	"    - When PVD found that all chunks of disk have zeroing completed, it removes the disk zero preset\n"
	"      condition and update the paged and non paged metadata with disk zeroing completed.\n"
	"\n"
	"TEST 3: Single SP test - Verify Disk zeroing operation with LUN creation \n"
	"\n"
	"STEP 1: Disk zero validation\n"
	"        (Disk zero preset condition already set to start disk zero process.)\n"
	"    - Read the non paged metadata for PVD1 using config service\n"
	"    - Check that the checkpoint value should not be 0xFFFFFFFF\n"
	"    - Check that there is no upstream edge connected to PVDs.\n"
	"      This PVD should not be part of any Raid Group\n"
	"    - Check that PVD object should be in READY state\n"
	"\n"
	"STEP 2: Start Disk zeroing \n"
	"    - Read the paged metadata for PVD1 from disk using config service\n"
	"    - Find the first chunk from bitmap data to perform disk zero IO\n"
	"    - Sends the disk zero IO from PVD to LD object\n"
	"\n"
	"STEP 3: Create LUN\n"
	"\n"
	"STEP 4: Ensure that Zeroing checkpoint moves back to the PBA of the LUN offset\n"
	"\n"
	"STEP 5: Verify the disk zeroing operation\n"
	"    - Verify the progress of disk zero operation with updating the checkpoint. After completion\n"
	"      of each disk zero IO, update the paged metadata first and then update the non paged metadata.\n"
	"    - Send disk zero IO for next chunk to perform disk zeroing\n"
	"\n"
	"STEP 6: Disk zeroing complete\n"
	"    - When PVD found that all chunks of disk have zeroing completed, it removes the disk zero preset\n"
	"      condition and update the paged and non paged metadata with disk zeroing completed.\n"
    "\n";

static fbe_test_rg_configuration_t foghorn_leghorn_rg_configuration[] =
{
    /* width,                                   capacity,                       raid type,             class,         block size, RAID-id, bandwidth.*/
    {3, 0x16000, FBE_RAID_GROUP_TYPE_RAID5, FBE_CLASS_ID_PARITY, 520, 0, 0},
    {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
};

#define FOGHORN_LEGHORN_TEST_ZERO_WAIT_TIME 200000
#define FOGHORN_LEGHORN_CHUNK_SIZE                  0x800

/*****************************************
 * LOCAL FUNCTION DEFINITIONS
 *****************************************/

void foghorn_leghorn_rg_create_test(fbe_test_rg_configuration_t*    in_rg_config_p,
        						    void* 							context_p);

void foghorn_leghorn_disk_failure_test(fbe_test_rg_configuration_t*    in_rg_config_p,
        							   void* 							context_p);

void foghorn_leghorn_lun_create_test(fbe_test_rg_configuration_t*    in_rg_config_p,
        						    void* 							context_p);

/*****************************************
 * LOCAL FUNCTIONS
 *****************************************/

/*!****************************************************************************
 *  foghorn_leghorn_setup
 ******************************************************************************
 *
 * @brief
 *   This is the setup function for the foghorn_leghorn test.
 *
 * @param   None
 *
 * @return  None
 *
 *****************************************************************************/
void foghorn_leghorn_setup(void)
{
    mut_printf(MUT_LOG_TEST_STATUS, "=== Setup for Foghorn Leghorn test ===\n");

    if (fbe_test_util_is_simulation())
    {
       /* load the physical package */
        zeroing_physical_config();
        sep_config_load_sep();
    }

    return;
}

/***************************************************************
 * end foghorn_leghorn_setup()
 ***************************************************************/

/*!****************************************************************************
 *  foghorn_leghorn_test
 ******************************************************************************
 *
 * @brief
 *  This is the main entry point for the Foghorn Leghorn test.
 *
 *
 * @param   None
 *
 * @return  None
 *
 * @author
 *   10/11/2011 - Created. Jason White
 *****************************************************************************/
void foghorn_leghorn_test(void)
{
    fbe_test_rg_configuration_t *rg_config_p = NULL;

    rg_config_p = &foghorn_leghorn_rg_configuration[0];

    fbe_test_sep_util_init_rg_configuration_array(rg_config_p);

    fbe_test_run_test_on_rg_config_no_create(rg_config_p, 0, foghorn_leghorn_disk_failure_test);
    fbe_test_run_test_on_rg_config_no_create(rg_config_p, 0, foghorn_leghorn_rg_create_test);
    fbe_test_run_test_on_rg_config_no_create(rg_config_p, 0, foghorn_leghorn_lun_create_test);


    return;
}

/*!****************************************************************************
 *  foghorn_leghorn_disk_failure_test
 ******************************************************************************
 *
 * @brief
 *   This function is used to perform disk zeroing in single SP configuration.
 *   It initiates disk zeroing then removes the drive.  It checks that disk zeroing
 *   stopped and then reinserts the drive and verify that disk zeroing continues
 *   and then completed.
 *   successfully.
 *
 *
 * @param   in_rg_config_p - pointer to the RG configuration
 * @param	context_p - pointer to the context
 *
 * @return  None
 *
 *****************************************************************************/
void foghorn_leghorn_disk_failure_test(fbe_test_rg_configuration_t*    in_rg_config_p,
        							   void* 							context_p)
{
    fbe_u32_t           				port;
    fbe_u32_t           				enclosure;
    fbe_u32_t           				slot;
    fbe_object_id_t     				object_id;
    fbe_status_t        				status;
    fbe_u32_t           				timeout_ms = 30000;
    fbe_u32_t           				number_physical_objects;
    fbe_api_terminator_device_handle_t  drive;
    fbe_scheduler_debug_hook_t          hook;
    fbe_lba_t                           chkpt;
    fbe_api_provision_drive_info_t      pvd_info;

    /*
     *  Test 1: Start disk zeroing and remove and reinsert drive then make sure that
     *  the disk zeroing completed in single SP configuration.
     */
    mut_printf(MUT_LOG_LOW, "=== Test 1  single SP - Disk zeroing test with drive removal ===\n");

    /* Disable dualsp mode */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);

    // @todo need to add support for running on hardware

    /* Lookup the disk location */
    port = in_rg_config_p->rg_disk_set[0].bus;
    enclosure = in_rg_config_p->rg_disk_set[0].enclosure;
    slot = in_rg_config_p->rg_disk_set[0].slot;

    /* Wait for the pvd to come online */
    status = fbe_test_sep_drive_verify_pvd_state(port, enclosure, slot,
                                                 FBE_LIFECYCLE_STATE_READY,
                                                 timeout_ms);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* get the PVD object ID */
    status = fbe_api_provision_drive_get_obj_id_by_location(port, enclosure, slot, &object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    fbe_api_provision_drive_get_info(object_id, &pvd_info);

    mut_printf(MUT_LOG_TEST_STATUS, "Adding Debug Hook...");

    status = fbe_api_scheduler_add_debug_hook(object_id,
    									SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_ZERO,
    									FBE_PROVISION_DRIVE_SUBSTATE_ZERO_ENTRY,
    									pvd_info.default_offset+0x800,
    									NULL,
    									SCHEDULER_CHECK_VALS_LT,
    									SCHEDULER_DEBUG_ACTION_PAUSE);

    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    mut_printf(MUT_LOG_LOW, "=== starting disk zeroing for PVD obj id 0x%x(be patient, it will take some time)\n", object_id);

    /* start disk zeroing */
    status = fbe_api_provision_drive_initiate_disk_zeroing(object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    sep_zeroing_utils_wait_for_disk_zeroing_to_start(object_id, FOGHORN_LEGHORN_TEST_ZERO_WAIT_TIME);

    /* wait for disk zeroing to get to the hook */
    status = sep_zeroing_utils_check_disk_zeroing_status(object_id, FOGHORN_LEGHORN_TEST_ZERO_WAIT_TIME, pvd_info.default_offset+0x800);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_api_get_total_objects(&number_physical_objects, FBE_PACKAGE_ID_PHYSICAL);
    number_physical_objects -= 1;
    

    mut_printf(MUT_LOG_LOW, "=== Removing the disk\n");
    /* need to remove a drive here */
    sep_rebuild_utils_remove_drive_no_check(port, enclosure, slot, number_physical_objects, &drive);

    mut_printf(MUT_LOG_TEST_STATUS, "Getting Debug Hook...");

	hook.object_id = object_id;
	hook.action = SCHEDULER_DEBUG_ACTION_PAUSE;
	hook.check_type = SCHEDULER_CHECK_VALS_LT;
	hook.monitor_state = SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_ZERO;
	hook.monitor_substate = FBE_PROVISION_DRIVE_SUBSTATE_ZERO_ENTRY;
	hook.val1 = pvd_info.default_offset+0x800;
	hook.val2 = NULL;
	hook.counter = 0;

	status = fbe_api_scheduler_get_debug_hook(&hook);

	MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

	mut_printf(MUT_LOG_TEST_STATUS, "The hook was hit %llu times",
		   (unsigned long long)hook.counter);

	mut_printf(MUT_LOG_TEST_STATUS, "Deleting Debug Hook...");

	status = fbe_api_scheduler_del_debug_hook(object_id,
											SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_ZERO,
											FBE_PROVISION_DRIVE_SUBSTATE_ZERO_ENTRY,
											pvd_info.default_offset+0x800,
											NULL,
											SCHEDULER_CHECK_VALS_LT,
											SCHEDULER_DEBUG_ACTION_PAUSE);

	MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    mut_printf(MUT_LOG_LOW, "=== Checking to ensure that the zeroing stopped\n");
    /* wait for disk zeroing to complete */
    status = sep_zeroing_utils_check_disk_zeroing_stopped(object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* get the current disk zeroing checkpoint */
    status = fbe_api_provision_drive_get_zero_checkpoint(object_id, &chkpt);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    MUT_ASSERT_UINT64_EQUAL(pvd_info.default_offset+0x800, chkpt);

    mut_printf(MUT_LOG_LOW, "=== Reinserting the disk\n");
    sep_rebuild_utils_reinsert_removed_drive_no_check(port, enclosure, slot, &drive);

    /* wait for disk zeroing to complete */
    status = sep_zeroing_utils_check_disk_zeroing_status(object_id, FOGHORN_LEGHORN_TEST_ZERO_WAIT_TIME, FBE_LBA_INVALID);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* disk zeroing completed */
    mut_printf(MUT_LOG_LOW, "=== disk zeroing completed for PVD obj id 0x%x\n", object_id);

    return;
}
/***************************************************************
 * end foghorn_leghorn_disk_failure_test()
 ***************************************************************/

/*!****************************************************************************
 *  foghorn_leghorn_rg_create_test
 ******************************************************************************
 *
 * @brief
 *   This function is used to perform disk zeroing in single SP configuration.
 *   It initiates disk zeroing then creates a RG with the drive.  It checks that
 *   disk zeroing restarts from the previous checkpoint if it is less than the
 *   checkpoint of the new RG needing to be zeroed and verify that
 *   disk zeroing continues and then completed successfully.
 *
 * @param   in_rg_config_p - pointer to the RG configuration
 * @param	context_p - pointer to the context
 *
 * @return  None
 *
 *****************************************************************************/
void foghorn_leghorn_rg_create_test(fbe_test_rg_configuration_t*    in_rg_config_p,
        						    void* 							context_p)
{
    fbe_u32_t           				port;
    fbe_u32_t           				enclosure;
    fbe_u32_t           				slot;
    fbe_object_id_t     				object_id;
    fbe_status_t        				status;
    fbe_u32_t           				timeout_ms = 30000;
    fbe_u32_t           				number_physical_objects;
    fbe_scheduler_debug_hook_t          hook;
    fbe_lba_t                           chkpt;
    fbe_lba_t                           pre_chkpt;
    fbe_api_raid_group_get_info_t       rg_info;
    fbe_object_id_t						rg_object_id;
    fbe_u64_t                           pba = NULL;
	fbe_api_provision_drive_info_t      pvd_info;
    fbe_provision_drive_paged_metadata_t         *pvd_metadata;
    fbe_api_base_config_metadata_paged_get_bits_t paged_get_bits;
    fbe_database_system_scrub_progress_t  scrub_progress;

    /*
     *  Test 1: Start disk zeroing and remove and reinsert drive then make sure that
     *  the disk zeroing completed in single SP configuration.
     */
    mut_printf(MUT_LOG_LOW, "=== Test 1  single SP - Disk zeroing test with RG Creation ===\n");

    /* Disable dualsp mode */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);

    // @todo need to add support for running on hardware

    /* Lookup the disk location */
    port = in_rg_config_p->rg_disk_set[1].bus;
    enclosure = in_rg_config_p->rg_disk_set[1].enclosure;
    slot = in_rg_config_p->rg_disk_set[1].slot;

    /* Wait for the pvd to come online */
    status = fbe_test_sep_drive_verify_pvd_state(port, enclosure, slot,
                                                 FBE_LIFECYCLE_STATE_READY,
                                                 timeout_ms);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* get the PVD object ID */
    status = fbe_api_provision_drive_get_obj_id_by_location(port, enclosure, slot, &object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

	fbe_api_provision_drive_get_info(object_id, &pvd_info);

    mut_printf(MUT_LOG_TEST_STATUS, "Adding Debug Hook...");
    status = fbe_api_scheduler_add_debug_hook(object_id,
    									SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_ZERO,
    									FBE_PROVISION_DRIVE_SUBSTATE_ZERO_ENTRY,
    									0x29001,
    									NULL,
    									SCHEDULER_CHECK_VALS_LT,
    									SCHEDULER_DEBUG_ACTION_PAUSE);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

	/* start disk zeroing */
    status = fbe_api_provision_drive_initiate_disk_zeroing(object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_LOW, "=== starting disk zeroing for PVD obj id 0x%x(be patient, it will take some time)\n", object_id);

    sep_zeroing_utils_wait_for_disk_zeroing_to_start(object_id, FOGHORN_LEGHORN_TEST_ZERO_WAIT_TIME);

    /* wait for disk zeroing to get to the hook */
    status = sep_zeroing_utils_check_disk_zeroing_status(object_id, FOGHORN_LEGHORN_TEST_ZERO_WAIT_TIME, 0x29001);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Add a 2nd hook to ensure that the zeroing checkpoint moves back to cover the paged metadata */
	mut_printf(MUT_LOG_TEST_STATUS, "Adding 2nd Debug Hook...");

	status = fbe_api_scheduler_add_debug_hook(object_id,
										SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_ZERO,
										FBE_PROVISION_DRIVE_SUBSTATE_ZERO_ENTRY,
										NULL,
										NULL,
										SCHEDULER_CHECK_STATES,
										SCHEDULER_DEBUG_ACTION_PAUSE);
	MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

	/* get the current disk zeroing checkpoint */
	status = fbe_api_provision_drive_get_zero_checkpoint(object_id, &pre_chkpt);
	MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

	mut_printf(MUT_LOG_LOW, "Zero Chkpt pre RG creation: 0x%llX\n",
		   (unsigned long long)pre_chkpt);

    status = fbe_api_get_total_objects(&number_physical_objects, FBE_PACKAGE_ID_PHYSICAL);

    mut_printf(MUT_LOG_LOW, "=== Creating an RG on the disk\n");
    if(fbe_test_rg_config_is_enabled(in_rg_config_p))
    {
        /* Create RG */
        sep_zeroing_utils_create_rg(in_rg_config_p);
    }

    status = fbe_api_database_lookup_raid_group_by_number(in_rg_config_p->raid_group_id, &rg_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_api_raid_group_get_info(rg_object_id, &rg_info,
                                                 FBE_PACKET_FLAG_NO_ATTRIB);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    mut_printf(MUT_LOG_LOW, "RG paged metadata LBA: 0x%llX\n",
	       (unsigned long long)rg_info.paged_metadata_start_lba);

    status = fbe_api_raid_group_get_physical_from_logical_lba(rg_object_id, rg_info.paged_metadata_start_lba, &pba);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    mut_printf(MUT_LOG_LOW, "RG paged metadata PBA: 0x%llX\n",
	       (unsigned long long)pba);

    /* get the current disk zeroing checkpoint */
    status = fbe_api_provision_drive_get_zero_checkpoint(object_id, &chkpt);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    mut_printf(MUT_LOG_LOW, "Zero Chkpt: 0x%llX\n", (unsigned long long)chkpt);
    MUT_ASSERT_TRUE(chkpt < pre_chkpt);
    //MUT_ASSERT_UINT64_EQUAL(pba, chkpt);
    mut_printf(MUT_LOG_TEST_STATUS, "Getting Debug Hook...");

	hook.object_id = object_id;
	hook.action = SCHEDULER_DEBUG_ACTION_PAUSE;
	hook.check_type = SCHEDULER_CHECK_VALS_LT;
	hook.monitor_state = SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_ZERO;
	hook.monitor_substate = FBE_PROVISION_DRIVE_SUBSTATE_ZERO_ENTRY;
	hook.val1 = 0x29001;
	hook.val2 = NULL;
	hook.counter = 0;

	status = fbe_api_scheduler_get_debug_hook(&hook);
	MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

	mut_printf(MUT_LOG_TEST_STATUS, "The hook was hit %llu times",
		   (unsigned long long)hook.counter);
	mut_printf(MUT_LOG_TEST_STATUS, "Deleting Debug Hook...");

	status = fbe_api_scheduler_del_debug_hook(object_id,
											SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_ZERO,
											FBE_PROVISION_DRIVE_SUBSTATE_ZERO_ENTRY,
											0x29001,
											NULL,
											SCHEDULER_CHECK_VALS_LT,
											SCHEDULER_DEBUG_ACTION_PAUSE);
	MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

	mut_printf(MUT_LOG_TEST_STATUS, "Deleting 2nd Debug Hook...");
	status = fbe_api_scheduler_del_debug_hook(object_id,
										SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_ZERO,
										FBE_PROVISION_DRIVE_SUBSTATE_ZERO_ENTRY,
										NULL,
										NULL,
										SCHEDULER_CHECK_STATES,
										SCHEDULER_DEBUG_ACTION_PAUSE);
	MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* wait for disk zeroing to complete */
    status = sep_zeroing_utils_check_disk_zeroing_status(object_id, FOGHORN_LEGHORN_TEST_ZERO_WAIT_TIME, FBE_LBA_INVALID);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* disk zeroing completed */
    mut_printf(MUT_LOG_LOW, "=== disk zeroing completed for PVD obj id 0x%x\n", object_id);

    status = fbe_test_sep_util_destroy_raid_group(in_rg_config_p->raid_group_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* disable background service */
    status = fbe_api_base_config_disable_background_operation(object_id, FBE_PROVISION_DRIVE_BACKGROUND_OP_DISK_ZEROING);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Read metadata */        
    mut_printf(MUT_LOG_LOW, "=== verify PVD obj id 0x%x paged metadata, chunck 0x%x, 0x%llxx\n", 
               object_id, (0x10000/FOGHORN_LEGHORN_CHUNK_SIZE), (pba/FOGHORN_LEGHORN_CHUNK_SIZE));
    paged_get_bits.metadata_offset = (fbe_u32_t )(0x10000/FOGHORN_LEGHORN_CHUNK_SIZE) * sizeof(fbe_provision_drive_paged_metadata_t);;
    paged_get_bits.metadata_record_data_size = sizeof(fbe_provision_drive_paged_metadata_t);
    * (fbe_u32_t *)paged_get_bits.metadata_record_data = 0;
    paged_get_bits.get_bits_flags = 0;
    status = fbe_api_base_config_metadata_paged_get_bits(object_id, &paged_get_bits);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    pvd_metadata = (fbe_provision_drive_paged_metadata_t *)&paged_get_bits.metadata_record_data[0];
    MUT_ASSERT_UINT64_EQUAL(pvd_metadata[0].need_zero_bit, 0);
    MUT_ASSERT_UINT64_EQUAL(pvd_metadata[0].consumed_user_data_bit, 0);

    paged_get_bits.metadata_offset = (fbe_u32_t )(pba/FOGHORN_LEGHORN_CHUNK_SIZE) * sizeof(fbe_provision_drive_paged_metadata_t);
    paged_get_bits.metadata_record_data_size = sizeof(fbe_provision_drive_paged_metadata_t);
    * (fbe_u32_t *)paged_get_bits.metadata_record_data = 0;
    paged_get_bits.get_bits_flags = 0;
    status = fbe_api_base_config_metadata_paged_get_bits(object_id, &paged_get_bits);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    pvd_metadata = (fbe_provision_drive_paged_metadata_t *)&paged_get_bits.metadata_record_data[0];
    MUT_ASSERT_UINT64_EQUAL(pvd_metadata[0].need_zero_bit, 0);
    MUT_ASSERT_UINT64_EQUAL(pvd_metadata[0].consumed_user_data_bit, 1);

	/* start disk zeroing */
    status = fbe_api_job_service_scrub_old_user_data();
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    status = fbe_test_sep_util_wait_for_pvd_np_flags(object_id, FBE_PROVISION_DRIVE_NP_FLAG_PAGED_SCRUB_NEEDED, 6000);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Read metadata again*/        
    paged_get_bits.metadata_offset = (fbe_u32_t )(0x10000/FOGHORN_LEGHORN_CHUNK_SIZE) * sizeof(fbe_provision_drive_paged_metadata_t);
    paged_get_bits.metadata_record_data_size = sizeof(fbe_provision_drive_paged_metadata_t);
    * (fbe_u32_t *)paged_get_bits.metadata_record_data = 0;
    paged_get_bits.get_bits_flags = 0;
    status = fbe_api_base_config_metadata_paged_get_bits(object_id, &paged_get_bits);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    pvd_metadata = (fbe_provision_drive_paged_metadata_t *)&paged_get_bits.metadata_record_data[0];
    MUT_ASSERT_UINT64_EQUAL(pvd_metadata[0].need_zero_bit, 0);
    MUT_ASSERT_UINT64_EQUAL(pvd_metadata[0].consumed_user_data_bit, 0);

    paged_get_bits.metadata_offset = (fbe_u32_t )(pba/FOGHORN_LEGHORN_CHUNK_SIZE) * sizeof(fbe_provision_drive_paged_metadata_t);
    paged_get_bits.metadata_record_data_size = sizeof(fbe_provision_drive_paged_metadata_t);
    * (fbe_u32_t *)paged_get_bits.metadata_record_data = 0;
    paged_get_bits.get_bits_flags = 0;
    status = fbe_api_base_config_metadata_paged_get_bits(object_id, &paged_get_bits);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    pvd_metadata = (fbe_provision_drive_paged_metadata_t *)&paged_get_bits.metadata_record_data[0];
    MUT_ASSERT_UINT64_EQUAL(pvd_metadata[0].need_zero_bit, 1);
    MUT_ASSERT_UINT64_EQUAL(pvd_metadata[0].consumed_user_data_bit, 0);

    fbe_api_provision_drive_get_info(object_id, &pvd_info);
    MUT_ASSERT_INT_EQUAL(pvd_info.scrubbing_in_progress, FBE_TRUE);

    /* make sure scrubbing is in progress */
    status = fbe_api_database_get_system_scrub_progress(&scrub_progress);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_UINT64_NOT_EQUAL(scrub_progress.blocks_already_scrubbed, scrub_progress.total_capacity_in_blocks);

    mut_printf(MUT_LOG_LOW, "=== starting consumed area zeroing for PVD obj id 0x%x(be patient, it will take some time)\n", object_id);

    status = fbe_api_base_config_enable_background_operation(object_id, FBE_PROVISION_DRIVE_BACKGROUND_OP_DISK_ZEROING);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* wait for disk zeroing to complete */
    status = sep_zeroing_utils_check_disk_zeroing_status(object_id, FOGHORN_LEGHORN_TEST_ZERO_WAIT_TIME, FBE_LBA_INVALID);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* disk zeroing completed */
    mut_printf(MUT_LOG_LOW, "=== consumed area zeroing completed for PVD obj id 0x%x\n", object_id);

    return;
}
/***************************************************************
 * end foghorn_leghorn_rg_create_test()
 ***************************************************************/

/*!****************************************************************************
 *  foghorn_leghorn_lun_create_test
 ******************************************************************************
 *
 * @brief
 *   This function is used to perform disk zeroing in single SP configuration.
 *   It initiates disk zeroing then creates a LUN with the drive.  It checks that
 *   disk zeroing restarts from the LBA of the LUN offset and verify that
 *   disk zeroing continues and then completed successfully.
 *
 * @param   in_rg_config_p - pointer to the RG configuration
 * @param	context_p - pointer to the context
 *
 * @return  None
 *
 *****************************************************************************/
void foghorn_leghorn_lun_create_test(fbe_test_rg_configuration_t*    in_rg_config_p,
        						    void* 							context_p)
{
    fbe_u32_t           						port;
    fbe_u32_t           						enclosure;
    fbe_u32_t           						slot;
    fbe_object_id_t     						object_id;
    fbe_status_t        						status;
    fbe_u32_t           						timeout_ms = 30000;
    fbe_u32_t           						number_physical_objects;
    fbe_scheduler_debug_hook_t          		hook;
    fbe_lba_t                           		chkpt;
    fbe_lba_t                           		pre_chkpt;
    zeroing_test_lun_t                  		lun;
    fbe_api_lun_get_lun_info_t          		lun_info;
    fbe_object_id_t                     		lun_obj_id;
    fbe_object_id_t                     		raid_group_object_id;
    fbe_api_base_config_upstream_object_list_t  upstream_object_list;
    fbe_u64_t                                   pba;
	fbe_api_provision_drive_info_t              pvd_info;

    /*
     *  Test 1: Start disk zeroing and remove and reinsert drive then make sure that
     *  the disk zeroing completed in single SP configuration.
     */
    mut_printf(MUT_LOG_LOW, "=== Test 1  single SP - Disk zeroing test with RG Creation ===\n");

    /* Disable dualsp mode */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);

    // @todo need to add support for running on hardware

    /* Lookup the disk location */
    port = in_rg_config_p->rg_disk_set[2].bus;
    enclosure = in_rg_config_p->rg_disk_set[2].enclosure;
    slot = in_rg_config_p->rg_disk_set[2].slot;

    /* Wait for the pvd to come online */
    status = fbe_test_sep_drive_verify_pvd_state(port, enclosure, slot,
                                                 FBE_LIFECYCLE_STATE_READY,
                                                 timeout_ms);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* get the PVD object ID */
    status = fbe_api_provision_drive_get_obj_id_by_location(port, enclosure, slot, &object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

	fbe_api_provision_drive_get_info(object_id, &pvd_info);

    mut_printf(MUT_LOG_TEST_STATUS, "Adding Debug Hook...");
    status = fbe_api_scheduler_add_debug_hook(object_id,
    									SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_ZERO,
    									FBE_PROVISION_DRIVE_SUBSTATE_ZERO_ENTRY,
    									0x10001,
    									NULL,
    									SCHEDULER_CHECK_VALS_LT,
    									SCHEDULER_DEBUG_ACTION_PAUSE);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* start disk zeroing */
    status = fbe_api_provision_drive_initiate_disk_zeroing(object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_LOW, "=== starting disk zeroing for PVD obj id 0x%x(be patient, it will take some time)\n", object_id);

    sep_zeroing_utils_wait_for_disk_zeroing_to_start(object_id, FOGHORN_LEGHORN_TEST_ZERO_WAIT_TIME);

    /* wait for disk zeroing to get to the hook */
    status = sep_zeroing_utils_check_disk_zeroing_status(object_id, FOGHORN_LEGHORN_TEST_ZERO_WAIT_TIME, 0x10001);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* get the current disk zeroing checkpoint */
    fbe_api_provision_drive_get_zero_checkpoint(object_id, &pre_chkpt);
    mut_printf(MUT_LOG_LOW, "Zero Chkpt pre LUN creation: 0x%llX\n",
	       (unsigned long long)pre_chkpt);

    /* Add a 2nd hook to ensure that the zeroing checkpoint doesnt get reset to 0 */
    mut_printf(MUT_LOG_TEST_STATUS, "Adding 2nd Debug Hook...");
	status = fbe_api_scheduler_add_debug_hook(object_id,
										SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_ZERO,
										FBE_PROVISION_DRIVE_SUBSTATE_ZERO_ENTRY,
										NULL,
										NULL,
										SCHEDULER_CHECK_STATES,
										SCHEDULER_DEBUG_ACTION_PAUSE);
	MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_api_get_total_objects(&number_physical_objects, FBE_PACKAGE_ID_PHYSICAL);

    /* get the current disk zeroing checkpoint */
	fbe_api_provision_drive_get_zero_checkpoint(object_id, &chkpt);

	mut_printf(MUT_LOG_TEST_STATUS, "Pre lun creation checkpoint is 0x%llX",
		   (unsigned long long)chkpt);

    mut_printf(MUT_LOG_LOW, "=== Creating an RG on the disk\n");
    if(fbe_test_rg_config_is_enabled(in_rg_config_p))
    {
        /* Create RG */
        sep_zeroing_utils_create_rg(in_rg_config_p);
    }

    sep_zeroing_utils_create_lun(&lun, 0, in_rg_config_p->raid_group_id);

    status = fbe_api_database_lookup_raid_group_by_number(in_rg_config_p->raid_group_id, &raid_group_object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    /*Find the LUN belonging to RG i.e. find upstream obj of RG*/
    status = fbe_api_base_config_get_upstream_object_list(raid_group_object_id, &upstream_object_list);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    lun_obj_id = upstream_object_list.upstream_object_list[0];

    status = fbe_api_lun_get_lun_info(lun_obj_id, &lun_info);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    mut_printf(MUT_LOG_LOW, "LUN Offset LBA: 0x%llX\n",
	       (unsigned long long)lun_info.offset);

    status = fbe_api_raid_group_get_physical_from_logical_lba(raid_group_object_id, lun_info.offset, &pba);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    mut_printf(MUT_LOG_LOW, "LUN Offset PBA: 0x%llX\n",
	       (unsigned long long)pba);

    /* get the current disk zeroing checkpoint */
    fbe_api_provision_drive_get_zero_checkpoint(object_id, &chkpt);
    mut_printf(MUT_LOG_LOW, "Zero Chkpt post LUN creation: 0x%llX\n",
	       (unsigned long long)chkpt);

    MUT_ASSERT_TRUE(pre_chkpt > chkpt);
    MUT_ASSERT_UINT64_EQUAL(pba, chkpt);

    mut_printf(MUT_LOG_TEST_STATUS, "Post lun creation checkpoint is 0x%llX",
	       (unsigned long long)chkpt);

    mut_printf(MUT_LOG_TEST_STATUS, "Getting Debug Hook...");
	hook.object_id = object_id;
	hook.action = SCHEDULER_DEBUG_ACTION_PAUSE;
	hook.check_type = SCHEDULER_CHECK_VALS_LT;
	hook.monitor_state = SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_ZERO;
	hook.monitor_substate = FBE_PROVISION_DRIVE_SUBSTATE_ZERO_ENTRY;
	hook.val1 = 0x10001;
	hook.val2 = NULL;
	hook.counter = 0;
	status = fbe_api_scheduler_get_debug_hook(&hook);

	MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

	mut_printf(MUT_LOG_TEST_STATUS, "The hook was hit %llu times",
		   (unsigned long long)hook.counter);
	mut_printf(MUT_LOG_TEST_STATUS, "Deleting Debug Hook...");
	status = fbe_api_scheduler_del_debug_hook(object_id,
											SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_ZERO,
											FBE_PROVISION_DRIVE_SUBSTATE_ZERO_ENTRY,
											0x10001,
											NULL,
											SCHEDULER_CHECK_VALS_LT,
											SCHEDULER_DEBUG_ACTION_PAUSE);
	MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    mut_printf(MUT_LOG_TEST_STATUS, "Deleting 2nd Debug Hook...");
	status = fbe_api_scheduler_del_debug_hook(object_id,
										SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_ZERO,
										FBE_PROVISION_DRIVE_SUBSTATE_ZERO_ENTRY,
										NULL,
										NULL,
										SCHEDULER_CHECK_STATES,
										SCHEDULER_DEBUG_ACTION_PAUSE);
	MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* wait for disk zeroing to complete */
    status = sep_zeroing_utils_check_disk_zeroing_status(object_id, FOGHORN_LEGHORN_TEST_ZERO_WAIT_TIME, FBE_LBA_INVALID);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* disk zeroing completed */
    mut_printf(MUT_LOG_LOW, "=== disk zeroing completed for PVD obj id 0x%x\n", object_id);

    return;
}
/***************************************************************
 * end foghorn_leghorn_lun_create_test()
 ***************************************************************/

/*!**************************************************************
 * foghorn_leghorn_cleanup()
 ****************************************************************
 * @brief
 *  Cleanup the foghorn_leghorn test.
 *
 * @param None.
 *
 * @return None.
 *
 * @author
 *  10/10/2011 - Created. Jason White
 *
 ****************************************************************/

void foghorn_leghorn_cleanup(void)
{
    mut_printf(MUT_LOG_LOW, "=== %s starts ===\n", __FUNCTION__);

    if (fbe_test_util_is_simulation())
    {
        fbe_test_sep_util_destroy_sep_physical();
    }

    mut_printf(MUT_LOG_LOW, "=== %s completed ===\n", __FUNCTION__);
    return;
}
/******************************************
 * end foghorn_leghorn_cleanup()
 ******************************************/

/*************************
 * end file foghorn_leghorn_test.c
 *************************/
