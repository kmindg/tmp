/***************************************************************************
 * Copyright (C) EMC Corporation 2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file barnyard_dawg_test.c
 ***************************************************************************
 *
 * @brief
 *   This file contains interaction tests with disk zeroing.
 *
 * @version
 *   12/15/2011 - Created. Jason White
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
#include "fbe/fbe_api_power_saving_interface.h"

//-----------------------------------------------------------------------------
//  TEST DESCRIPTION:

char* barnyard_dawg_short_desc = "Disk zeroing interaction test cases";
char* barnyard_dawg_long_desc =
    "\n"
    "\n"
    "This scenario validates background disk zeroing operation with interactions with other services\n"
    "\n"
    "\n"
    "******* Barnyard Dawg **************************************************************\n"
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

static fbe_test_rg_configuration_t barnyard_dawg_rg_configuration[] =
{
    /* width,                                   capacity,                       raid type,             class,         block size, RAID-id, bandwidth.*/
    {3, 0x50000, FBE_RAID_GROUP_TYPE_RAID5, FBE_CLASS_ID_PARITY, 520, 0, 0},
    {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
};

#define BARNYARD_DAWG_TEST_ZERO_WAIT_TIME 200000

#define BARNYARD_DAWG_SLEEP_TIME 120

typedef struct barnyard_dawg_zero_wait_context_s{
	fbe_semaphore_t zero_semaphore;
	fbe_object_id_t pvd_id;
	fbe_object_zero_notification_t expected_zero_status;
}barnyard_dawg_zero_wait_context_t;

/*****************************************
 * LOCAL FUNCTION DEFINITIONS
 *****************************************/

void barnyard_dawg_random_disk_failure_test(fbe_test_rg_configuration_t*    in_rg_config_p,
        							   void* 							context_p);

static void barnyard_dawg_notification_callback_function (update_object_msg_t * update_object_msg, void *context);

/*****************************************
 * LOCAL FUNCTIONS
 *****************************************/

/*!****************************************************************************
 *  barnyard_dawg_dualsp_setup
 ******************************************************************************
 *
 * @brief
 *   This is the setup function for the barnyard_dawg test.
 *
 * @param   None
 *
 * @return  None
 *
 *****************************************************************************/
void barnyard_dawg_dualsp_setup(void)
{
    mut_printf(MUT_LOG_TEST_STATUS, "=== Setup for Foghorn Leghorn test ===\n");

    /* Enable dualsp mode
     */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_TRUE);

    if (fbe_test_util_is_simulation())
	{
		//  Set the target server to SPA
		fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);

		//  Load the physical package and create the physical configuration.
		zeroing_physical_config();

		//  Set the target server to SPB
		fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);

		//  Load the physical package and create the physical configuration.
		zeroing_physical_config();

		//  Load the logical packages on both SPs; setting target server to SPA for consistency during testing
		fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);

		sep_config_load_sep_and_neit_both_sps();
	}

	//  The following utility function does some setup for hardware; noop for simulation
	fbe_test_common_util_test_setup_init();

    /* Disable dualsp mode
     */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);

    return;
}

/***************************************************************
 * end barnyard_dawg_dualsp_setup()
 ***************************************************************/

/*!****************************************************************************
 *  barnyard_dawg_dualsp_test
 ******************************************************************************
 *
 * @brief
 *  This is the main entry point for the Barnyard Dawg test.
 *
 *
 * @param   None
 *
 * @return  None
 *
 * @author
 *   10/11/2011 - Created. Jason White
 *****************************************************************************/
void barnyard_dawg_dualsp_test(void)
{
    fbe_test_rg_configuration_t *rg_config_p = NULL;

    /* Enable dualsp mode */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_TRUE);

    rg_config_p = &barnyard_dawg_rg_configuration[0];

    fbe_test_sep_util_init_rg_configuration_array(rg_config_p);

    fbe_test_run_test_on_rg_config_no_create(rg_config_p, 0, barnyard_dawg_random_disk_failure_test);

    /* Always clear dualsp mode */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);

    return;
}

/*!****************************************************************************
 *  barnyard_dawg_random_disk_failure_test
 ******************************************************************************
 *
 * @brief
 *   This function is used to perform disk zeroing and randomly pull the drive.
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
void barnyard_dawg_random_disk_failure_test(fbe_test_rg_configuration_t*    in_rg_config_p,
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
    fbe_api_provision_drive_info_t      pvd_info;
    fbe_bool_t                          zeroing_in_progress = FBE_TRUE;
	fbe_notification_registration_id_t  reg_id;
	barnyard_dawg_zero_wait_context_t	zero_wait_context;

    /*
     *  Test 1: Start disk zeroing and remove and reinsert drive then make sure that
     *  the disk zeroing completed.
     */
    mut_printf(MUT_LOG_LOW, "=== Test 1 - Disk zeroing test with random drive removal ===\n");

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

	
    mut_printf(MUT_LOG_LOW, "=== starting disk zeroing for PVD obj id 0x%x(be patient, it will take some time)\n", object_id);

	/*regsiter so we can get the notification for zeroing start*/
	fbe_semaphore_init(&zero_wait_context.zero_semaphore, 0, 1);
	zero_wait_context.pvd_id = object_id;
	zero_wait_context.expected_zero_status = FBE_OBJECT_ZERO_STARTED;

	status = fbe_api_notification_interface_register_notification(FBE_NOTIFICATION_TYPE_ZEROING,
																  FBE_PACKAGE_NOTIFICATION_ID_SEP_0,
																  FBE_TOPOLOGY_OBJECT_TYPE_PROVISIONED_DRIVE,
																  barnyard_dawg_notification_callback_function,
																  &zero_wait_context,
																  &reg_id);

	MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* start disk zeroing */
    status = fbe_api_provision_drive_initiate_disk_zeroing(object_id);
	MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

	/*wait for the zero start status to trip the semaphore*/
	status = fbe_semaphore_wait_ms(&zero_wait_context.zero_semaphore, BARNYARD_DAWG_TEST_ZERO_WAIT_TIME);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

	/*next time we wanted to make sure it stopped so let's set it up here before we do anything else*/
	zero_wait_context.expected_zero_status = FBE_OBJECT_ZERO_ENDED;

    while (zeroing_in_progress)
    {
        status = fbe_api_get_total_objects(&number_physical_objects, FBE_PACKAGE_ID_PHYSICAL);
		MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

		number_physical_objects-=1;

		mut_printf(MUT_LOG_LOW, "=== Removing the disk - %d_%d_%d, PVD: 0x%x\n", port, enclosure, slot, object_id);
		/* need to remove a drive here */
		sep_rebuild_utils_remove_drive_no_check(port, enclosure, slot, number_physical_objects, &drive);
        if (fbe_test_sep_util_get_dualsp_test_mode())
        {
            fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
		    sep_rebuild_utils_remove_drive_no_check(port, enclosure, slot, number_physical_objects, &drive);
            fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
        }
        /* Verify the SEP objects is in expected lifecycle State  */
		status = fbe_api_wait_for_object_lifecycle_state(object_id,
														 FBE_LIFECYCLE_STATE_FAIL,
														 60000,
														 FBE_PACKAGE_ID_SEP_0);
		MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

		mut_printf(MUT_LOG_LOW, "=== Checking to ensure that the zeroing stopped\n");

		/* wait for disk zeroing to complete */
		status = sep_zeroing_utils_check_disk_zeroing_stopped(object_id);
		MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);


		mut_printf(MUT_LOG_LOW, "=== Reinserting the disk - %d_%d_%d, PVD: 0x%x\n", port, enclosure, slot, object_id);
		sep_rebuild_utils_reinsert_removed_drive_no_check(port, enclosure, slot, &drive);
		

		/* Verify the SEP objects is in expected lifecycle State  */
		status = fbe_api_wait_for_object_lifecycle_state(object_id,
														 FBE_LIFECYCLE_STATE_READY,
														 60000,
														 FBE_PACKAGE_ID_SEP_0);
		MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

		fbe_api_sleep(BARNYARD_DAWG_SLEEP_TIME);

		status = sep_zeroing_utils_is_zeroing_in_progress(object_id, &zeroing_in_progress);
		MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    }

	/*wait for the zero end status to trip the semaphore*/
	status = fbe_semaphore_wait_ms(&zero_wait_context.zero_semaphore, BARNYARD_DAWG_TEST_ZERO_WAIT_TIME);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* also verify the checkpoint (we leave this here to exercize all the APIs)*/
    status = sep_zeroing_utils_check_disk_zeroing_status(object_id, BARNYARD_DAWG_TEST_ZERO_WAIT_TIME, FBE_LBA_INVALID);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

	fbe_api_notification_interface_unregister_notification(barnyard_dawg_notification_callback_function, reg_id);
	fbe_semaphore_destroy(&zero_wait_context.zero_semaphore);

    /* disk zeroing completed */
    mut_printf(MUT_LOG_LOW, "=== disk zeroing completed for PVD obj id 0x%x\n", object_id);

    return;
}
/***************************************************************
 * end barnyard_dawg_random_disk_failure_test()
 ***************************************************************/

/*!**************************************************************
 * barnyard_dawg_dualsp_cleanup()
 ****************************************************************
 * @brief
 *  Cleanup the barnyard_dawg test.
 *
 * @param None.
 *
 * @return None.
 *
 * @author
 *  10/10/2011 - Created. Jason White
 *
 ****************************************************************/

void barnyard_dawg_dualsp_cleanup(void)
{
    mut_printf(MUT_LOG_LOW, "=== %s starts ===\n", __FUNCTION__);

    if (fbe_test_util_is_simulation())
    {
		fbe_test_sep_util_destroy_neit_sep_physical_both_sps();
    }

    mut_printf(MUT_LOG_LOW, "=== %s completed ===\n", __FUNCTION__);
    return;
}
/******************************************
 * end barnyard_dawg_dualsp_cleanup()
 ******************************************/

static void barnyard_dawg_notification_callback_function (update_object_msg_t * update_object_msg, void *context)
{
	barnyard_dawg_zero_wait_context_t *wait_context = (barnyard_dawg_zero_wait_context_t *)context;

	if (wait_context->pvd_id == update_object_msg->object_id &&
		update_object_msg->notification_info.notification_type == FBE_NOTIFICATION_TYPE_ZEROING &&
		update_object_msg->notification_info.notification_data.object_zero_notification.zero_operation_status == wait_context->expected_zero_status) {
	
		fbe_semaphore_release(&wait_context->zero_semaphore, 0, 1, FALSE);
	}

}

/*************************
 * end file barnyard_dawg_test.c
 *************************/
