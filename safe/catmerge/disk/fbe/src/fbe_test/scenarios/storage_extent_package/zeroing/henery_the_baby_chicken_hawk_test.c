/***************************************************************************
 * Copyright (C) EMC Corporation 2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file henery_the_baby_chicken_hawk_test.c
 ***************************************************************************
 *
 * @brief
 *   This file contains tests of drive zeroing testing out PVD persistence.
 *
 * @version
 *   11/01/2011 - Created. Jason White
 *
 ***************************************************************************/


/*************************
 *   INCLUDE FILES
 *************************/
#include "mut.h"
#include "fbe_test_package_config.h"
#include "fbe_test.h"
#include "fbe/fbe_terminator_api.h"
#include "fbe/fbe_api_provision_drive_interface.h"
#include "fbe/fbe_api_raid_group_interface.h"
#include "fbe/fbe_api_physical_drive_interface.h"
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
#include "fbe/fbe_random.h"


//-----------------------------------------------------------------------------
//  TEST DESCRIPTION:

char* henery_the_baby_chicken_hawk_short_desc = "New drive zeroing (on unbound drive) - Error cases";
char* henery_the_baby_chicken_hawk_long_desc =
    "\n"
    "\n"
    "This scenario validates persistence in the PVD object.\n"
    "\n"
    "\n"
    "******* Henery The Baby Chicken Hawk **************************************************************\n"
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
    "TEST 1: Dual SP test - Verify PVD persistence from Active Side \n"
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
    "STEP 3: Verify the disk zeroing operation\n"
    "    - Verify the progress of disk zero operation with updating the checkpoint. After completion\n"
    "      of each disk zero IO, update the paged metadata first and then update the non paged metadata.\n"
    "    - Send disk zero IO for next chunk to perform disk zeroing\n"
    "\n"
    "STEP 4: Disk zeroing complete\n"
    "    - When PVD found that all chunks of disk have zeroing completed, it removes the disk zero preset\n"
    "      condition and update the paged and non paged metadata with disk zeroing completed.\n"
	"\n"
	"STEP 5: Reboot the active SP\n"
	"\n"
	"STEP 6: Ensure the ZOD is not set and the checkpoint is set to FBE_LBA_INVALID\n"
	"\n"
	"TEST 2: Dual SP test - Verify PVD persistence from Passive Side \n"
	"\n"
	"STEP 1: Disk zero validation\n"
	"        (Disk zero preset condition already set to start disk zero process.)\n"
	"    - Read the non paged metadata for PVD1 using config service\n"
	"    - Check that the checkpoint value should not be 0xFFFFFFFF\n"
	"    - Check that there is no upstream edge connected to PVDs.\n"
	"      This PVD should not be part of any Raid Group\n"
	"    - Check that PVD object should be in READY state\n"
	"STEP 2: Start Disk zeroing \n"
	"    - Read the paged metadata for PVD1 from disk using config service\n"
	"    - Find the first chunk from bitmap data to perform disk zero IO\n"
	"    - Sends the disk zero IO from PVD to LD object\n"
	"\n"
	"STEP 3: Verify the disk zeroing operation\n"
	"    - Verify the progress of disk zero operation with updating the checkpoint. After completion\n"
	"      of each disk zero IO, update the paged metadata first and then update the non paged metadata.\n"
	"    - Send disk zero IO for next chunk to perform disk zeroing\n"
	"\n"
	"STEP 4: Disk zeroing complete\n"
	"    - When PVD found that all chunks of disk have zeroing completed, it removes the disk zero preset\n"
	"      condition and update the paged and non paged metadata with disk zeroing completed.\n"
	"\n"
	"STEP 5: Set EOL on the passive SP \n"
	"\n"
	"STEP 6: Reboot both SPs \n"
	"\n"
	"STEP 7: Ensure that EOL is still set \n"
	"\n"
	"TEST 3: Dual SP test - Verify PVD checkpoint persistence \n"
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
	"STEP 3: Verify the disk zeroing operation\n"
	"    - Verify the progress of disk zero operation with updating the checkpoint. After completion\n"
	"      of each disk zero IO, update the paged metadata first and then update the non paged metadata.\n"
	"    - Send disk zero IO for next chunk to perform disk zeroing\n"
	"\n"
	"STEP 4: Disk Zero Completion Function \n"
	"    - Hold the completion function and move the checkpoint back."
	"\n"
	"STEP 5: Let completetion functions continue. \n"
	"    - Check that the checkpoint is not set to FBE_LBA_INVALID. \n"
	"\n"
	"STEP 6: Disk zeroing complete\n"
	"    - When PVD found that all chunks of disk have zeroing completed, it removes the disk zero preset\n"
	"      condition and update the paged and non paged metadata with disk zeroing completed.\n"
	"\n"
	"TEST 4: Dual SP test - Background Zeroing progression after a single or dual SP failure. \n"
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
	"STEP 3: Verify the disk zeroing operation\n"
	"    - Verify the progress of disk zero operation with updating the checkpoint. After completion\n"
	"      of each disk zero IO, update the paged metadata first and then update the non paged metadata.\n"
	"    - Send disk zero IO for next chunk to perform disk zeroing\n"
	"\n"
	"STEP 4: Hold the PVD in Zeroing state (either in entry checkpoint 0x10800 or in completion)\n"
    "\n"
	"STEP 5: Perform SP failure - Passive SP crash, Active SP crash, or Dual SP crash\n"
    "\n"
    "STEP 6: Remove the debug hooks and allow the zeroing to complete\n"
	"\n"
	"STEP 7: Verify that after the crash the following is checked:\n"
    "    - Passive SP crash in ZERO_ENTRY: Active SP continues with zeroing\n"
    "           - a higher checkpoint is hit.\n"
    "           - a lower checkpoint is not hit.\n"
    "    - Passive SP crash in ZERO_COMPLETION: Active SP finishes zeroing\n"
    "    - Active SP crash in ZERO_ENTRY: Passive SP continues with zeroing\n"
    "           - a higher checkpoint is hit.\n"
    "           - a lower checkpoint is not hit.\n"
    "    - Active SP crash in ZERO_COMPLETION: Active SP finishes zeroing\n"
    "    - Dual SP crash in ZERO_ENTRY: SPs will start zeroing from the default offset.\n"
    "    - Dual SP crash in ZERO_COMPLETION: SPs will start zeroing from the default offset\n"
    "\n";

static fbe_test_rg_configuration_t henery_the_baby_chicken_hawk_rg_configuration[] =
{
    /* width,                                   capacity,                       raid type,             class,         block size, RAID-id, bandwidth.*/
    {3, 0x32000, FBE_RAID_GROUP_TYPE_RAID5, FBE_CLASS_ID_PARITY, 520, 0, 0},
    {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
};

#define HENERY_THE_BABY_CHICKEN_HAWK_TEST_MAX_CONFIGS 2
#define HENERY_THE_BABY_CHICKEN_HAWK_TEST_ZERO_WAIT_TIME 200000

/*!*******************************************************************
 * @enum henery_the_baby_chicken_hawk_crash_flag_e
 *********************************************************************
 * @brief This is a simple enum for crashing one or both SPs
 *
 *********************************************************************/
typedef enum henery_the_baby_chicken_hawk_crash_flag_e
{
    CRASH_PASSIVE_SP = 0,
    CRASH_ACTIVE_SP = 1,
    CRASH_BOTH_SPS = 2
}
henery_the_baby_chicken_hawk_crash_flag_t;

/*****************************************
 * LOCAL FUNCTION DEFINITIONS
 *****************************************/

void henery_the_baby_chicken_hawk_pvd_active_side_persist_test(fbe_test_rg_configuration_t*    in_rg_config_p,
        						    void* 							context_p);

void henery_the_baby_chicken_hawk_pvd_passive_side_persist_test(fbe_test_rg_configuration_t*    in_rg_config_p,
        							void* 							context_p);

void henery_the_baby_chicken_hawk_pvd_chkpt_persist_test(fbe_test_rg_configuration_t*    in_rg_config_p,
        							   void* 							context_p);

void henery_the_baby_chicken_hawk_sp_failure_test(fbe_test_rg_configuration_t * rg_config_p,
												  void* context_p);
void henery_the_baby_chicken_hawk_select_pvd_in_rg(fbe_test_rg_configuration_t * rg_config_p,
												   fbe_object_id_t * object_id,
												   fbe_api_provision_drive_info_t * pvd_info);
void henery_the_baby_check_hawk_setup_hooks(fbe_object_id_t	pvd_object_id,
                                            fbe_api_provision_drive_info_t * pvd_info,
                                            fbe_scheduler_debug_hook_t * hook,
                                            fbe_scheduler_debug_hook_t * low_hook,
                                            fbe_scheduler_debug_hook_t * high_hook);
void henery_the_baby_chicken_hawk_add_hook(fbe_scheduler_debug_hook_t * hook);
void henery_the_baby_chicken_hawk_verify_zeroing_complete(fbe_object_id_t pvd_object_id);
void henery_the_baby_chicken_hawk_crash_passive_sp_test(fbe_scheduler_debug_hook_t * hook,
                                                        fbe_scheduler_debug_hook_t * low_hook,
                                                        fbe_scheduler_debug_hook_t * high_hook,
                                                        fbe_api_provision_drive_info_t * pvd_info);
void henery_the_baby_chicken_hawk_crash_active_sp_test(fbe_scheduler_debug_hook_t * hook,
                                                       fbe_scheduler_debug_hook_t * low_hook,
                                                       fbe_scheduler_debug_hook_t * high_hook,
                                                       fbe_api_provision_drive_info_t * pvd_info);
void henery_the_baby_chicken_hawk_crash_both_sps_test(fbe_scheduler_debug_hook_t * hook,
                                                      fbe_api_provision_drive_info_t * pvd_info);


static void henery_the_baby_chicken_hawk_crash_dual_sp(void);

static fbe_status_t henery_the_baby_chicken_hawk_wait_for_eol_set(fbe_object_id_t object_id);


/*****************************************
 * LOCAL FUNCTIONS
 *****************************************/
/*!****************************************************************************
 *  henery_the_baby_chicken_hawk_setup
 ******************************************************************************
 *
 * @brief
 *   This is the setup function for the henery_the_baby_chicken_hawk test.
 *
 * @param   None
 *
 * @return  None
 *
 *****************************************************************************/
void henery_the_baby_chicken_hawk_setup(void)
{
	fbe_scheduler_debug_hook_t hook;

    mut_printf(MUT_LOG_TEST_STATUS, "=== Setup for Henery the Baby Chicken Hawk test ===\n");

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

	fbe_api_scheduler_clear_all_debug_hooks(&hook);
    return;
}
/***************************************************************
 * end henery_the_baby_chicken_hawk_setup()
 ***************************************************************/
/*!****************************************************************************
 *  henery_the_baby_chicken_hawk_dualsp_setup
 ******************************************************************************
 *
 * @brief
 *   This is the setup function for the henery_the_baby_chicken_hawk test.
 *
 * @param   None
 *
 * @return  None
 *
 *****************************************************************************/
void henery_the_baby_chicken_hawk_dualsp_setup(void)
{
    henery_the_baby_chicken_hawk_setup();

	//  The following utility function does some setup for hardware; noop for simulation
	fbe_test_common_util_test_setup_init();

    return;
}
/***************************************************************
 * end henery_the_baby_chicken_hawk_dualsp_setup()
 ***************************************************************/

/*!****************************************************************************
 *  henery_the_baby_chicken_hawk_dualsp_test
 ******************************************************************************
 *
 * @brief
 *  This is the main entry point for the henery_the_baby_chicken_hawk test.
 *
 *
 * @param   None
 *
 * @return  None
 *
 * @author
 *   10/11/2011 - Created. Jason White
 *****************************************************************************/
void henery_the_baby_chicken_hawk_dualsp_test(void)
{
    fbe_test_rg_configuration_t *rg_config_p = NULL;
    fbe_u32_t                    rg_index;

    /* Enable dualsp mode
     */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_TRUE);
    for (rg_index = 0; rg_index < HENERY_THE_BABY_CHICKEN_HAWK_TEST_MAX_CONFIGS; rg_index++)
    {
        rg_config_p = &henery_the_baby_chicken_hawk_rg_configuration[rg_index];

        fbe_test_sep_util_init_rg_configuration_array(rg_config_p);

        fbe_test_run_test_on_rg_config_no_create(rg_config_p, 0, henery_the_baby_chicken_hawk_pvd_active_side_persist_test);
        fbe_test_run_test_on_rg_config_no_create(rg_config_p, 0, henery_the_baby_chicken_hawk_pvd_chkpt_persist_test);
        fbe_test_run_test_on_rg_config_no_create(rg_config_p, 0, henery_the_baby_chicken_hawk_pvd_passive_side_persist_test);
        fbe_test_run_test_on_rg_config_no_create(rg_config_p, 0, henery_the_baby_chicken_hawk_sp_failure_test);
    }

    /* Disable dualsp mode
     */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);

    return;
}

/*!****************************************************************************
 *  henery_the_baby_chicken_hawk_pvd_active_side_persist_test
 ******************************************************************************
 *
 * @brief
 *   This function is used to perform disk zeroing in dual SP configuration.
 *   It initiates disk zeroing, waits for completion, and then crashes the SPs.
 *   It checks that ZOD is not set and that the zero checkpoint it set to FBE_LBA_INVALID.
 *
 *
 * @param   in_rg_config_p - pointer to the RG configuration
 * @param	context_p - pointer to the context
 *
 * @return  None
 *
 *****************************************************************************/
void henery_the_baby_chicken_hawk_pvd_active_side_persist_test(fbe_test_rg_configuration_t*    in_rg_config_p,
        							   void* 							context_p)
{
    fbe_u32_t           				  port;
    fbe_u32_t           				  enclosure;
    fbe_u32_t           				  slot;
    fbe_object_id_t     				  object_id;
    fbe_status_t        				  status;
    fbe_u32_t           				  timeout_ms = 30000;
    fbe_sim_transport_connection_target_t local_sp;

    /*
     *  Test 1: Start disk zeroing and remove and reinsert drive then make sure that
     *  the disk zeroing completed in single SP configuration.
     */
    mut_printf(MUT_LOG_LOW, "=== Test 1  dual SP - PVD Active Side Persist Test ===\n");

    /* Get the local SP ID */
    local_sp = fbe_api_sim_transport_get_target_server();

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

    mut_printf(MUT_LOG_LOW, "=== starting disk zeroing for PVD obj id 0x%x(be patient, it will take some time)\n", object_id);

    /* start disk zeroing */
    status = fbe_api_provision_drive_initiate_disk_zeroing(object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = fbe_api_base_config_enable_background_operation(object_id, FBE_PROVISION_DRIVE_BACKGROUND_OP_DISK_ZEROING);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    sep_zeroing_utils_wait_for_disk_zeroing_to_start(object_id, HENERY_THE_BABY_CHICKEN_HAWK_TEST_ZERO_WAIT_TIME);

    /* wait for disk zeroing to complete */
    status = sep_zeroing_utils_check_disk_zeroing_status(object_id, HENERY_THE_BABY_CHICKEN_HAWK_TEST_ZERO_WAIT_TIME, FBE_LBA_INVALID);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* disk zeroing completed */
    mut_printf(MUT_LOG_LOW, "=== disk zeroing completed for PVD obj id 0x%x\n", object_id);

    /* Need to powercycle the system */
    henery_the_baby_chicken_hawk_crash_dual_sp();

    /* get the current disk zeroing checkpoint */
    status = fbe_test_zero_wait_for_zero_on_demand_state(object_id, FBE_TEST_HOOK_WAIT_MSEC, FBE_FALSE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    return;
}
/***************************************************************
 * end henery_the_baby_chicken_hawk_pvd_active_side_persist_test()
 ***************************************************************/

/*!****************************************************************************
 *  henery_the_baby_chicken_hawk_pvd_passive_side_persist_test
 ******************************************************************************
 *
 * @brief
 *   This function is used to perform disk zeroing in dual SP configuration.
 *   It initiates disk zeroing, waits for completion, sets the EOL flag on the
 *   passive side, and then crashes the SPs. It checks that EOL is still set.
 *
 *
 * @param   in_rg_config_p - pointer to the RG configuration
 * @param	context_p - pointer to the context
 *
 * @return  None
 *
 *****************************************************************************/
void henery_the_baby_chicken_hawk_pvd_passive_side_persist_test(fbe_test_rg_configuration_t*    in_rg_config_p,
        							   void* 							context_p)
{
    fbe_u32_t           				  port;
    fbe_u32_t           				  enclosure;
    fbe_u32_t           				  slot;
    fbe_object_id_t     				  object_id;
    fbe_object_id_t						  pdo_obj_id;
    fbe_status_t        				  status;
    fbe_u32_t           				  timeout_ms = 30000;
    fbe_api_provision_drive_info_t        pvd_info;
    fbe_sim_transport_connection_target_t   local_sp;
    fbe_sim_transport_connection_target_t   peer_sp;

    /*
     *  Test 1: Start disk zeroing and remove and reinsert drive then make sure that
     *  the disk zeroing completed in single SP configuration.
     */
    mut_printf(MUT_LOG_LOW, "=== Test 3  dual SP - PVD Passive Side Persist Test ===\n");

	/* Get the local SP ID */
	local_sp = fbe_api_sim_transport_get_target_server();
	MUT_ASSERT_INT_NOT_EQUAL(FBE_SIM_INVALID_SERVER, local_sp);
	mut_printf(MUT_LOG_LOW, "=== ACTIVE(local) side (%s) ===", local_sp == FBE_SIM_SP_A ? "SP A" : "SP B");

	/* set the passive SP */
	peer_sp = (local_sp == FBE_SIM_SP_A ? FBE_SIM_SP_B : FBE_SIM_SP_A);

	fbe_api_sim_transport_set_target_server(local_sp);

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

    mut_printf(MUT_LOG_LOW, "=== starting disk zeroing for PVD obj id 0x%x(be patient, it will take some time)\n", object_id);

    /* start disk zeroing */
    status = fbe_api_provision_drive_initiate_disk_zeroing(object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = fbe_api_base_config_enable_background_operation(object_id, FBE_PROVISION_DRIVE_BACKGROUND_OP_DISK_ZEROING);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    sep_zeroing_utils_wait_for_disk_zeroing_to_start(object_id, HENERY_THE_BABY_CHICKEN_HAWK_TEST_ZERO_WAIT_TIME);

    /* wait for disk zeroing to complete */
    status = sep_zeroing_utils_check_disk_zeroing_status(object_id, HENERY_THE_BABY_CHICKEN_HAWK_TEST_ZERO_WAIT_TIME, FBE_LBA_INVALID);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* disk zeroing completed */
    mut_printf(MUT_LOG_LOW, "=== disk zeroing completed for PVD obj id 0x%x\n", object_id);

    fbe_api_sim_transport_set_target_server(peer_sp);

    status = fbe_api_get_physical_drive_object_id_by_location(port, enclosure, slot, &pdo_obj_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    mut_printf(MUT_LOG_LOW, "=== Setting EOL for PVD obj id 0x%x through PDO obj id 0x%x\n", object_id, pdo_obj_id);

    status = fbe_api_physical_drive_set_pvd_eol(pdo_obj_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* get the PVD object ID */
    status = fbe_api_provision_drive_get_obj_id_by_location(port, enclosure, slot, &object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = henery_the_baby_chicken_hawk_wait_for_eol_set(object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Need to powercycle the system */
    henery_the_baby_chicken_hawk_crash_dual_sp();

    /* Ensure that EOL is set */
    fbe_api_provision_drive_get_info(object_id, &pvd_info);
    MUT_ASSERT_TRUE(pvd_info.end_of_life_state);

    return;
}
/***************************************************************
 * end henery_the_baby_chicken_hawk_pvd_passive_side_persist_test()
 ***************************************************************/

/*!****************************************************************************
 *  henery_the_baby_chicken_hawk_pvd_chkpt_persist_test
 ******************************************************************************
 *
 * @brief
 *   This function is used to perform disk zeroing in dual SP configuration.
 *   It initiates disk zeroing, waits for completion function, and then moves the
 *   checkpoint back and ensures it is not set to FBE_LBA_INVALID when the
 *   completion function completes.
 *
 *
 * @param   in_rg_config_p - pointer to the RG configuration
 * @param	context_p - pointer to the context
 *
 * @return  None
 *
 *****************************************************************************/
void henery_the_baby_chicken_hawk_pvd_chkpt_persist_test(fbe_test_rg_configuration_t*    in_rg_config_p,
        							   void* 							context_p)
{
    fbe_u32_t           				  port;
    fbe_u32_t           				  enclosure;
    fbe_u32_t           				  slot;
    fbe_object_id_t     				  object_id;
    fbe_status_t        				  status;
    fbe_u32_t           				  timeout_ms = 30000;
    fbe_api_provision_drive_info_t        pvd_info;
    fbe_sim_transport_connection_target_t local_sp;
    fbe_sim_transport_connection_target_t peer_sp;
    fbe_scheduler_debug_hook_t            hook;
    fbe_lba_t                             checkpoint;

    /*
     *  Test 1: Start disk zeroing and remove and reinsert drive then make sure that
     *  the disk zeroing completed in single SP configuration.
     */
    mut_printf(MUT_LOG_LOW, "=== Test 2  dual SP - PVD Checkpoint Persist Test ===\n");

    /* Get the local SP ID */
    local_sp = fbe_api_sim_transport_get_target_server();
    MUT_ASSERT_INT_NOT_EQUAL(FBE_SIM_INVALID_SERVER, local_sp);
    mut_printf(MUT_LOG_LOW, "=== ACTIVE(local) side (%s) ===", local_sp == FBE_SIM_SP_A ? "SP A" : "SP B");

    /* set the passive SP */
    peer_sp = (local_sp == FBE_SIM_SP_A ? FBE_SIM_SP_B : FBE_SIM_SP_A);

    fbe_api_sim_transport_set_target_server(local_sp);

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
    									FBE_PROVISION_DRIVE_SUBSTATE_ZERO_COMPLETION,
    									NULL,
    									NULL,
    									SCHEDULER_CHECK_STATES,
    									SCHEDULER_DEBUG_ACTION_PAUSE);

    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    mut_printf(MUT_LOG_LOW, "=== starting disk zeroing for PVD obj id 0x%x(be patient, it will take some time)\n", object_id);

    /* start disk zeroing */
    status = fbe_api_provision_drive_initiate_disk_zeroing(object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    sep_zeroing_utils_wait_for_disk_zeroing_to_start(object_id, HENERY_THE_BABY_CHICKEN_HAWK_TEST_ZERO_WAIT_TIME);

    hook.object_id = object_id;
    hook.action = SCHEDULER_DEBUG_ACTION_PAUSE;
    hook.check_type = SCHEDULER_CHECK_STATES;
    hook.monitor_state = SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_ZERO;
    hook.monitor_substate = FBE_PROVISION_DRIVE_SUBSTATE_ZERO_COMPLETION;
    hook.val1 = NULL;
    hook.val2 = NULL;
    hook.counter = 0;

    // wait for hook to be hit
    status = sep_zeroing_utils_wait_for_hook(&hook, HENERY_THE_BABY_CHICKEN_HAWK_TEST_ZERO_WAIT_TIME);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_api_provision_drive_get_info(object_id, &pvd_info);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    mut_printf(MUT_LOG_TEST_STATUS, "Adding 2nd Debug Hook...");

    status = fbe_api_scheduler_add_debug_hook(object_id,
    									SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_ZERO,
    									FBE_PROVISION_DRIVE_SUBSTATE_ZERO_ENTRY,
    									pvd_info.default_offset+0x800,
    									NULL,
    									SCHEDULER_CHECK_VALS_LT,
    									SCHEDULER_DEBUG_ACTION_PAUSE);

    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* start disk zeroing */
    status = fbe_api_provision_drive_initiate_disk_zeroing(object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    sep_zeroing_utils_wait_for_disk_zeroing_to_start(object_id, HENERY_THE_BABY_CHICKEN_HAWK_TEST_ZERO_WAIT_TIME);

    // delete first hook
    status = fbe_api_scheduler_del_debug_hook(object_id,
    									SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_ZERO,
    									FBE_PROVISION_DRIVE_SUBSTATE_ZERO_COMPLETION,
    									NULL,
    									NULL,
    									SCHEDULER_CHECK_STATES,
    									SCHEDULER_DEBUG_ACTION_PAUSE);

    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* get the current disk zeroing checkpoint */
    status = fbe_api_provision_drive_initiate_disk_zeroing(object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = fbe_api_base_config_enable_background_operation(object_id, FBE_PROVISION_DRIVE_BACKGROUND_OP_DISK_ZEROING);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = fbe_api_provision_drive_get_zero_checkpoint(object_id, &checkpoint);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    // check that the checkpoint is not invalid
    MUT_ASSERT_UINT64_NOT_EQUAL(checkpoint, FBE_LBA_INVALID);

    // delete 2nd checkpoint
    status = fbe_api_scheduler_del_debug_hook(object_id,
    									SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_ZERO,
    									FBE_PROVISION_DRIVE_SUBSTATE_ZERO_ENTRY,
    									pvd_info.default_offset+0x800,
    									NULL,
    									SCHEDULER_CHECK_VALS_LT,
    									SCHEDULER_DEBUG_ACTION_PAUSE);

    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* wait for disk zeroing to complete */
    status = sep_zeroing_utils_check_disk_zeroing_status(object_id, HENERY_THE_BABY_CHICKEN_HAWK_TEST_ZERO_WAIT_TIME, FBE_LBA_INVALID);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* disk zeroing completed */
    mut_printf(MUT_LOG_LOW, "=== disk zeroing completed for PVD obj id 0x%x\n", object_id);

    return;
}
/***************************************************************
 * end henery_the_baby_chicken_hawk_pvd_chkpt_persist_test()
 ***************************************************************/

/*!****************************************************************************
 *  henery_the_baby_chicken_hawk_sp_failure_test
 ******************************************************************************
 *
 * @brief
 *  Verify Background Zeroing progression after a single or dual 
 *  SP failure.
 *
 *
 * @param   rg_config_p - configuration to run the test against
 *          context_p - TBD
 *
 * @return  None
 *
 * @author
 *   12/08/2011 - Created. Deanna Heng
 *****************************************************************************/
void henery_the_baby_chicken_hawk_sp_failure_test(fbe_test_rg_configuration_t*    rg_config_p,
												  void* context_p) {
    fbe_status_t					status = FBE_STATUS_OK;
    fbe_object_id_t					pvd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_api_provision_drive_info_t	pvd_info;
    fbe_scheduler_debug_hook_t      hook;
    fbe_scheduler_debug_hook_t      low_hook;
    fbe_scheduler_debug_hook_t      high_hook;
    henery_the_baby_chicken_hawk_crash_flag_t	crash_flag;



    /*  Test 4: We will randomly select whether we want to crash during 
     *  the start of zeroing or during the completion of zeroing. We will
     *  then crash one or both SPs and check that zeroing continued as
     *  expected.
     */
    mut_printf(MUT_LOG_LOW, "=== Test 4  dual SP - Background zeroing progression after SP failure ===\n");

    // Select a random disk in the rg to zero
	henery_the_baby_chicken_hawk_select_pvd_in_rg(rg_config_p, &pvd_object_id, &pvd_info);

    // setup the hook that we will use to pause the disk as well as 2 sanity hooks 
    // that we may use to check later
    henery_the_baby_check_hawk_setup_hooks(pvd_object_id, &pvd_info, &hook, &low_hook, &high_hook);
	henery_the_baby_chicken_hawk_add_hook(&hook);

    /* start disk zeroing */
    status = fbe_api_provision_drive_initiate_disk_zeroing(pvd_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
	sep_zeroing_utils_wait_for_disk_zeroing_to_start(pvd_object_id, HENERY_THE_BABY_CHICKEN_HAWK_TEST_ZERO_WAIT_TIME);

	// Wait for the hook to get hit and reach the zeroing checkpoint
	// FBE_PROVISION_DRIVE_SUBSTATE_ZERO_ENTRY
	if (hook.monitor_substate == FBE_PROVISION_DRIVE_SUBSTATE_ZERO_ENTRY) {
		mut_printf(MUT_LOG_LOW, "OBJ 0x%x Wait for FBE_PROVISION_DRIVE_SUBSTATE_ZERO_ENTRY at 0x%llx", 
                   hook.object_id, (unsigned long long)hook.val1);
	} else {
	// FBE_PROVISION_DRIVE_SUBSTATE_ZERO_COMPLETION
		mut_printf(MUT_LOG_LOW, "OBJ 0x%x Wait for FBE_PROVISION_DRIVE_SUBSTATE_ZERO_COMPLETION", hook.object_id);
	} 
    
    status = sep_zeroing_utils_wait_for_hook(&hook, HENERY_THE_BABY_CHICKEN_HAWK_TEST_ZERO_WAIT_TIME);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    // Select how we want to fail our SPs
    crash_flag =  fbe_random() % 3;
    switch(crash_flag)
    {
        case CRASH_PASSIVE_SP:
        henery_the_baby_chicken_hawk_crash_passive_sp_test(&hook, &low_hook, &high_hook, &pvd_info);    
        break;

        case CRASH_ACTIVE_SP:
        henery_the_baby_chicken_hawk_crash_active_sp_test(&hook, &low_hook, &high_hook, &pvd_info);    
        break;

        case CRASH_BOTH_SPS:
        henery_the_baby_chicken_hawk_crash_both_sps_test(&high_hook, &pvd_info);    
        break;

        default:
        break;
    }

	// remove hook on both SPs in order to continue zeroing
	fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
	fbe_api_scheduler_clear_all_debug_hooks(&hook);
	fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
	fbe_api_scheduler_clear_all_debug_hooks(&hook);
	// set the target server back to SPA 
	fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
}
/***************************************************************
 * end henery_the_baby_chicken_hawk_sp_failure_test()
 ***************************************************************/

/*!****************************************************************************
 *  henery_the_baby_chicken_hawk_select_pvd_in_rg
 ******************************************************************************
 *
 * @brief
 *  Select a random pvd in an rg and make sure it is READY
 *
 * @param   rg_config_p - configuration to run the test against
 *          object_id - Pointer to the object id we are selecting
 *          pvd_info - Information about the pvd we are selecting
 *
 * @return  None
 *
 * @author
 *   12/08/2011 - Created. Deanna Heng
 *****************************************************************************/
void henery_the_baby_chicken_hawk_select_pvd_in_rg(fbe_test_rg_configuration_t * rg_config_p,
												   fbe_object_id_t * object_id,
												   fbe_api_provision_drive_info_t * pvd_info) {
	fbe_u32_t       port = 0;
	fbe_u32_t       enclosure = 0;
	fbe_u32_t       slot = 0;
	fbe_u32_t       drive_pos = 0;
    fbe_u32_t       timeout_ms = 30000;
    fbe_status_t    status = FBE_STATUS_OK;

    /* Select a random disk location in the rg*/
	drive_pos = 0;//fbe_random() % rg_config_p->width;

    /* Lookup the disk location */
    port = rg_config_p->rg_disk_set[drive_pos].bus;
    enclosure = rg_config_p->rg_disk_set[drive_pos].enclosure;
    slot = rg_config_p->rg_disk_set[drive_pos].slot;

    /* Wait for the pvd to come online */
    status = fbe_test_sep_drive_verify_pvd_state(port, enclosure, slot,
                                                 FBE_LIFECYCLE_STATE_READY,
                                                 timeout_ms);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* get the PVD object ID */
    status = fbe_api_provision_drive_get_obj_id_by_location(port, enclosure, slot, object_id);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

	status = fbe_api_provision_drive_get_info(*object_id, pvd_info);
	MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
}
/***************************************************************
 * end henery_the_baby_chicken_hawk_select_pvd_in_rg()
 ***************************************************************/

/*!****************************************************************************
 *  henery_the_baby_chicken_hawk_setup_debug_hooks
 ******************************************************************************
 *
 * @brief
 *  Set up the hooks we will use in the test
 *
 * @param   pvd_object_id - provision drive object id we are testing with
 *          pvd_info - information about the pvd - for default offset
 *          hook - the hook we will use to pause during Zeroing
 *          low_hook - contains lower checkpoint than hook - used to check counter
 *          high_hook - contains higher checkpoint than hook - used to check counter
 *
 *
 * @return  None
 *
 * @author
 *   12/08/2011 - Created. Deanna Heng
 *****************************************************************************/
void henery_the_baby_check_hawk_setup_hooks(fbe_object_id_t	pvd_object_id,
                                            fbe_api_provision_drive_info_t * pvd_info,
                                            fbe_scheduler_debug_hook_t * hook,
                                            fbe_scheduler_debug_hook_t * low_hook,
                                            fbe_scheduler_debug_hook_t * high_hook) {


	fbe_scheduler_hook_substate_t	zeroing_state = FBE_SCHEDULER_MONITOR_SUB_STATE_LAST; 
	fbe_lba_t						checkpoint = 0x800;
    fbe_lba_t						low_chkpt = checkpoint/2;
    fbe_lba_t                       high_chkpt = checkpoint*4;

	// randomly select whether we want to stop in start or completion
	zeroing_state = (fbe_random() % 2 ? FBE_PROVISION_DRIVE_SUBSTATE_ZERO_ENTRY : FBE_PROVISION_DRIVE_SUBSTATE_ZERO_COMPLETION);

    // Set up the hook for pausing during disk zeroing
    hook->object_id = pvd_object_id;
	hook->monitor_state = SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_ZERO;
	hook->monitor_substate = zeroing_state;
	hook->val1 = pvd_info->default_offset+checkpoint;
	hook->val2 = NULL;
    hook->action = SCHEDULER_DEBUG_ACTION_PAUSE;
	hook->check_type = SCHEDULER_CHECK_VALS_LT;
	hook->counter = 0;

    if (zeroing_state == FBE_PROVISION_DRIVE_SUBSTATE_ZERO_COMPLETION) {
        hook->val1 = NULL;
        hook->check_type = SCHEDULER_CHECK_STATES;
    }

    // Set up a hook to insert at a lower checkpoint. This will not pause the zeroing
    // This is just used to check whether this hook has been hit (counter)
    low_hook->object_id = hook->object_id;
    low_hook->monitor_state = SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_ZERO;
    low_hook->monitor_substate = FBE_PROVISION_DRIVE_SUBSTATE_ZERO_ENTRY;
    low_hook->val1 = pvd_info->default_offset+low_chkpt;
    low_hook->val2 = NULL;
    low_hook->check_type = SCHEDULER_CHECK_VALS_EQUAL;
    low_hook->action = SCHEDULER_DEBUG_ACTION_LOG;
    low_hook->counter = 0;

    // Set up a hook to insert at a higher checkpoint. This will not pause the zeroing
    // This is just used to check whether this hook has been hit (counter)
    high_hook->object_id = hook->object_id;
    high_hook->monitor_state = SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_ZERO;
    high_hook->monitor_substate = FBE_PROVISION_DRIVE_SUBSTATE_ZERO_ENTRY;
    high_hook->val1 = pvd_info->default_offset+high_chkpt;
    high_hook->val2 = NULL;
    high_hook->check_type = SCHEDULER_CHECK_VALS_EQUAL;
    high_hook->action = SCHEDULER_DEBUG_ACTION_LOG;
    high_hook->counter = 0;
}
/***************************************************************
 * end henery_the_baby_chicken_hawk_setup_hooks()
 ***************************************************************/

/*!****************************************************************************
 *  henery_the_baby_chicken_hawk_add_hook
 ******************************************************************************
 *
 * @brief
 *  Add a hook for the PVD
 *
 * @param   hook - the hook we will add
 *
 * @return  None
 *
 * @author
 *   12/08/2011 - Created. Deanna Heng
 *****************************************************************************/
void henery_the_baby_chicken_hawk_add_hook(fbe_scheduler_debug_hook_t * hook) {
	fbe_status_t					status = FBE_STATUS_OK;

	// FBE_PROVISION_DRIVE_SUBSTATE_ZERO_ENTRY
	if (hook->monitor_substate == FBE_PROVISION_DRIVE_SUBSTATE_ZERO_ENTRY) {
		mut_printf(MUT_LOG_LOW, "OBJ 0x%x Insert a hook for FBE_PROVISION_DRIVE_SUBSTATE_ZERO_ENTRY at 0x%llx", 
                   hook->object_id, (unsigned long long)hook->val1);
	} else {
	// FBE_PROVISION_DRIVE_SUBSTATE_ZERO_COMPLETION
		mut_printf(MUT_LOG_LOW, "OBJ 0x%x Insert a hook for FBE_PROVISION_DRIVE_SUBSTATE_ZERO_COMPLETION", hook->object_id);
	} 

	status = fbe_api_scheduler_add_debug_hook(hook->object_id,
											  hook->monitor_state,
											  hook->monitor_substate,
											  hook->val1,
											  hook->val2,
											  hook->check_type,
											  hook->action);

	MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
}
/***************************************************************
 * end henery_the_baby_chicken_hawk_add_hook()
 ***************************************************************/

/*!****************************************************************************
 *  henery_the_baby_chicken_hawk_verify_zeroing_complete
 ******************************************************************************
 *
 * @brief
 *  Verify that zeroing end marker is set for btoh SPs
 *
 * @param   pvd_object_id - the pvd we will check the zeroing checkpoint on
 *
 * @return  None
 *
 * @author
 *   12/08/2011 - Created. Deanna Heng
 *****************************************************************************/
void henery_the_baby_chicken_hawk_verify_zeroing_complete(fbe_object_id_t pvd_object_id) {
	fbe_status_t						status = FBE_STATUS_OK;

    mut_printf(MUT_LOG_TEST_STATUS, "Verifying that zeroing completed on SPA");
	status =fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
	MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
	status = sep_zeroing_utils_check_disk_zeroing_status(pvd_object_id, HENERY_THE_BABY_CHICKEN_HAWK_TEST_ZERO_WAIT_TIME, FBE_LBA_INVALID);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
	/* Ensure that ZOD is not set */
    status = fbe_test_zero_wait_for_zero_on_demand_state(pvd_object_id, FBE_TEST_HOOK_WAIT_MSEC, FBE_FALSE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_TEST_STATUS, "Verifying that zeroing completed on SPB");
	status =fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
	MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
	status = sep_zeroing_utils_check_disk_zeroing_status(pvd_object_id, HENERY_THE_BABY_CHICKEN_HAWK_TEST_ZERO_WAIT_TIME, FBE_LBA_INVALID);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
	/* Ensure that ZOD is not set */
    status = fbe_test_zero_wait_for_zero_on_demand_state(pvd_object_id, FBE_TEST_HOOK_WAIT_MSEC, FBE_FALSE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    status =fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
	MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
}
/***************************************************************
 * end henery_the_baby_chicken_hawk_verify_zeroing_complete()
 ***************************************************************/

/*!****************************************************************************
 *  henery_the_baby_chicken_hawk_crash_passive_sp_test
 ******************************************************************************
 *
 * @brief
 *  Crash the passive SP and verify that zeroing continues as expected
 *
 * @param   hook - the hook we will use to pause during Zeroing
 *          low_hook - contains lower checkpoint than hook - used to check counter
 *          high_hook - contains higher checkpoint than hook - used to check counter
 *          pvd_info - information about the pvd
 *
 * @return  None
 *
 * @author
 *   12/08/2011 - Created. Deanna Heng
 *****************************************************************************/
void henery_the_baby_chicken_hawk_crash_passive_sp_test(fbe_scheduler_debug_hook_t * hook,
                                                        fbe_scheduler_debug_hook_t * low_hook,
                                                        fbe_scheduler_debug_hook_t * high_hook,
                                                        fbe_api_provision_drive_info_t * pvd_info) {
    fbe_sim_transport_connection_target_t   target_sp;
	fbe_sim_transport_connection_target_t   peer_sp;
	fbe_status_t							status = FBE_STATUS_OK;
    fbe_u32_t                               timeout_ms = 20000;

	/* Get the local SP ID */
    target_sp = fbe_api_sim_transport_get_target_server();
    MUT_ASSERT_INT_NOT_EQUAL(FBE_SIM_INVALID_SERVER, target_sp);

	/* set the passive SP */
	peer_sp = (target_sp == FBE_SIM_SP_A ? FBE_SIM_SP_B : FBE_SIM_SP_A);
    
    // Insert hooks that record the counter at a lower checkpoint and a higher checkpoint
    mut_printf(MUT_LOG_TEST_STATUS, "Adding a sanity hook at checkpoint 0x%llx",
	       (unsigned long long)low_hook->val1);
    henery_the_baby_chicken_hawk_add_hook(low_hook);

    mut_printf(MUT_LOG_TEST_STATUS,
	       "Adding a 2nd sanity hook at checkpoint 0x%llx",
	       (unsigned long long)high_hook->val1);
    henery_the_baby_chicken_hawk_add_hook(high_hook);

    // Reboot the passive SP
    mut_printf(MUT_LOG_LOW, "=== REBOOT THE PASSIVE SP ===\n");
    darkwing_duck_shutdown_peer_sp();
    // Remove hook on Active SP 
    mut_printf(MUT_LOG_TEST_STATUS, "Deleting Origninal Debug Hook in order for zeroing to proceed...");
	status = fbe_api_scheduler_del_debug_hook(hook->object_id,
											  hook->monitor_state,
											  hook->monitor_substate,
											  hook->val1,
											  hook->val2,
											  hook->check_type,
											  hook->action);
	MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    // Startup passive SP
    status = fbe_api_sim_transport_set_target_server(peer_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    //  Load the physical package and create the physical configuration.
    zeroing_physical_config();
	sep_config_load_sep_and_neit();
    // Wait for the PVD to become ready
	status = fbe_test_sep_drive_verify_pvd_state(pvd_info->port_number, pvd_info->enclosure_number, 
                                                 pvd_info->slot_number, FBE_LIFECYCLE_STATE_READY,timeout_ms);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
	status = fbe_api_sim_transport_set_target_server(target_sp);
	MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    // Wait for bgz to finish on both SPs
    henery_the_baby_chicken_hawk_verify_zeroing_complete(hook->object_id);

    // Verify that low checkpoint is not hit
    mut_printf(MUT_LOG_TEST_STATUS, "Verify that checkpoint 0x%llx was not hit",
	       (unsigned long long)low_hook->val1);
    status = fbe_api_scheduler_get_debug_hook(low_hook);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_UINT64_EQUAL(0, low_hook->counter);

    // Verify the expectation from the high checkpoint
    status = fbe_api_scheduler_get_debug_hook(high_hook);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    if (hook->monitor_substate == FBE_PROVISION_DRIVE_SUBSTATE_ZERO_ENTRY) {
        mut_printf(MUT_LOG_TEST_STATUS, "Verify that checkpoint 0x%llx was hit",
		   (unsigned long long)high_hook->val1);
        MUT_ASSERT_UINT64_NOT_EQUAL(0, high_hook->counter);
    } else {
        mut_printf(MUT_LOG_TEST_STATUS,
		   "Verify that checkpoint 0x%llx was not hit",
		   (unsigned long long)high_hook->val1);
        MUT_ASSERT_UINT64_EQUAL(0, high_hook->counter);
    }
}
/******************************************
 * end henery_the_baby_chicken_hawk_crash_active_sp_test()
 ******************************************/


/*!****************************************************************************
 *  henery_the_baby_chicken_hawk_crash_active_sp_test
 ******************************************************************************
 *
 * @brief
 *  Crash the active SP and verify that zeroing continues as expected on the 
 *  new active SP
 *
 * @param   hook - the hook we will use to pause during Zeroing
 *          low_hook - contains lower checkpoint than hook - used to check counter
 *          high_hook - contains higher checkpoint than hook - used to check counter
 *          pvd_info - information about the pvd - for default offset
 *
 * @return  None
 *
 * @author
 *   12/08/2011 - Created. Deanna Heng
 *****************************************************************************/
void henery_the_baby_chicken_hawk_crash_active_sp_test(fbe_scheduler_debug_hook_t * hook,
                                                       fbe_scheduler_debug_hook_t * low_hook,
                                                       fbe_scheduler_debug_hook_t * high_hook,
                                                       fbe_api_provision_drive_info_t * pvd_info) {
    fbe_sim_transport_connection_target_t   target_sp;
	fbe_sim_transport_connection_target_t   peer_sp;
    fbe_u32_t                               timeout_ms = 20000;
    fbe_status_t                            status =  FBE_STATUS_OK;

    /* Get the local SP ID */
    target_sp = fbe_api_sim_transport_get_target_server();
    MUT_ASSERT_INT_NOT_EQUAL(FBE_SIM_INVALID_SERVER, target_sp);

	/* set the passive SP */
	peer_sp = (target_sp == FBE_SIM_SP_A ? FBE_SIM_SP_B : FBE_SIM_SP_A);

    status = fbe_api_sim_transport_set_target_server(peer_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    
    // Insert hooks that record the counter at a lower checkpoint and a higher checkpoint
    mut_printf(MUT_LOG_TEST_STATUS, "Adding a sanity hook at checkpoint 0x%llx",
	       (unsigned long long)low_hook->val1);
    henery_the_baby_chicken_hawk_add_hook(low_hook);

    mut_printf(MUT_LOG_TEST_STATUS,
	       "Adding a 2nd sanity hook at checkpoint 0x%llx",
	       (unsigned long long)high_hook->val1);
    henery_the_baby_chicken_hawk_add_hook(high_hook);

    // Reboot the passive SP
    mut_printf(MUT_LOG_LOW, "=== REBOOT THE ACTIVE SP ===\n");
    darkwing_duck_shutdown_peer_sp();

    // Startup active SP
    status = fbe_api_sim_transport_set_target_server(target_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    //  Load the physical package and create the physical configuration.
    zeroing_physical_config();
	sep_config_load_sep_and_neit();
    // Wait for the PVD to become ready
	status = fbe_test_sep_drive_verify_pvd_state(pvd_info->port_number, pvd_info->enclosure_number, 
                                                 pvd_info->slot_number, FBE_LIFECYCLE_STATE_READY,timeout_ms);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    // Wait for bgz to finish on both SPs
    henery_the_baby_chicken_hawk_verify_zeroing_complete(hook->object_id);

    // check SPB for the hooks...
    status = fbe_api_sim_transport_set_target_server(peer_sp);
	MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    // Verify that low checkpoint is not hit
    status = fbe_api_scheduler_get_debug_hook(low_hook);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "Verify that checkpoint 0x%llx was not hit",
	       (unsigned long long)low_hook->val1);
    MUT_ASSERT_UINT64_EQUAL(0, low_hook->counter);

    // Verify the expectation from the high checkpoint
    status = fbe_api_scheduler_get_debug_hook(high_hook);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    if (hook->monitor_substate == FBE_PROVISION_DRIVE_SUBSTATE_ZERO_ENTRY) {
        mut_printf(MUT_LOG_TEST_STATUS, "Verify that checkpoint 0x%llx was hit",
		   (unsigned long long)high_hook->val1);
        MUT_ASSERT_UINT64_NOT_EQUAL(0, high_hook->counter);
    } else {
        // For FBE_PROVISION_DRIVE_SUBSTATE_ZERO_COMPLETION the high checkpoint 
        // should have been shared across both SPs
        mut_printf(MUT_LOG_TEST_STATUS, "Verify that checkpoint 0x%llx was not hit", high_hook->val1);
        MUT_ASSERT_UINT64_EQUAL(0, high_hook->counter);
    }
}
/******************************************
 * end henery_the_baby_chicken_hawk_crash_active_sp_test()
 ******************************************/


/*!****************************************************************************
 *  henery_the_baby_chicken_hawk_crash_both_sps_test
 ******************************************************************************
 *
 * @brief
 *  Crash both SPs and verify that zeroing runs again after the crash
 *
 * @param   hook - the hook we will use to check that background verify is being run
 *          pvd_info - information about the pvd
 *
 * @return  None
 *
 * @author
 *   12/08/2011 - Created. Deanna Heng
 *****************************************************************************/
void henery_the_baby_chicken_hawk_crash_both_sps_test(fbe_scheduler_debug_hook_t * hook,
                                                      fbe_api_provision_drive_info_t * pvd_info) {
    fbe_sim_transport_connection_target_t   target_sp;
    fbe_sim_transport_connection_target_t   peer_sp;
    fbe_status_t							status = FBE_STATUS_OK;
    fbe_sep_package_load_params_t           sep_params_p;
    fbe_u32_t                               timeout_ms = 30000;

    /* Get the local SP ID */
    target_sp = fbe_api_sim_transport_get_target_server();
    MUT_ASSERT_INT_NOT_EQUAL(FBE_SIM_INVALID_SERVER, target_sp);

    /* set the passive SP */
    peer_sp = (target_sp == FBE_SIM_SP_A ? FBE_SIM_SP_B : FBE_SIM_SP_A);


    /* set passive SP as target  to set the debug hook only*/
    status = fbe_api_sim_transport_set_target_server(peer_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* set the debug hook on peer SP 
     * We need to set the debug on peer SP becuse there is possibility that when active sp 
     * shutdown first, PVD will mark as active on peer SP side for some time just before shutdown
     * and background zeroing gets comleted  if we do not set the debug hook on peer sp.
     */
    henery_the_baby_chicken_hawk_add_hook(hook);

    /*  Set the target sp back to continue the test */
    status = fbe_api_sim_transport_set_target_server(target_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    sep_config_fill_load_params(&sep_params_p);

    // setup the hook that we will insert
	sep_params_p.scheduler_hooks[0].object_id = hook->object_id;
    sep_params_p.scheduler_hooks[0].monitor_state = hook->monitor_state;
    sep_params_p.scheduler_hooks[0].monitor_substate = hook->monitor_substate;
    sep_params_p.scheduler_hooks[0].check_type = hook->check_type;
    sep_params_p.scheduler_hooks[0].action = hook->action;
    sep_params_p.scheduler_hooks[0].val1 = hook->val1;
    sep_params_p.scheduler_hooks[0].val2 = hook->val2;
	/* and this will be our signal to stop */
    sep_params_p.scheduler_hooks[1].object_id = FBE_OBJECT_ID_INVALID;

    mut_printf(MUT_LOG_LOW, "=== CRASH BOTH SPs ===\n");
    // Destroy the SPs and its current session 
    fbe_test_base_suite_destroy_sp();
    // Start the SPs with a new session 
    fbe_test_base_suite_startup_post_powercycle();
    // Create the physical config on both SPS
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    zeroing_physical_config();
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    zeroing_physical_config();
    //  Load the logical packages with sep params on both SPs
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    sep_config_load_sep_and_neit_params(&sep_params_p, NULL);
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    sep_config_load_sep_and_neit_params(&sep_params_p, NULL);
    // set the target server back to A for consistency
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    status = fbe_test_sep_drive_verify_pvd_state(pvd_info->port_number, pvd_info->enclosure_number, 
                                                 pvd_info->slot_number, FBE_LIFECYCLE_STATE_READY,timeout_ms);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    
    /* start disk zeroing */
    status = fbe_api_provision_drive_initiate_disk_zeroing(hook-> object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    sep_zeroing_utils_wait_for_disk_zeroing_to_start(hook->object_id, HENERY_THE_BABY_CHICKEN_HAWK_TEST_ZERO_WAIT_TIME);

    // Wait for bgz to finish on both SPs
    henery_the_baby_chicken_hawk_verify_zeroing_complete(hook->object_id);

    // Verify that low checkpoint is not hit
    status = fbe_api_scheduler_get_debug_hook(hook);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "Verify that checkpoint 0x%llx was hit",
	       (unsigned long long)hook->val1);
    MUT_ASSERT_UINT64_NOT_EQUAL(0, hook->counter);

}
/******************************************
 * end henery_the_baby_chicken_hawk_crash_both_sps_test()
 ******************************************/

/*!**************************************************************
 * henery_the_baby_chicken_hawk_cleanup()
 ****************************************************************
 * @brief
 *  Cleanup the henery_the_baby_chicken_hawk test.
 *
 * @param None.
 *
 * @return None.
 *
 * @author
 *  10/24/2011 - Created. Jason White
 *
 ****************************************************************/

void henery_the_baby_chicken_hawk_dualsp_cleanup(void)
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
 * end henery_the_baby_chicken_hawk_dualsp_cleanup()
 ******************************************/

/*!****************************************************************************
 *  henery_the_baby_chicken_hawk_crash_dual_sp
 ******************************************************************************
 *
 * @brief
 *   This function will do an unexpected SP (dual) crash and kills the session.
 *   It also recreates the session with the same config as it was before the crash.
 *
 *   This is different from reboot. Reboot does a graceful shutdown of all the
 *   systems... but a crash.. no sympathy... just halts and pulls everything
 *   down.
 *
 * @param   None
 *
 * @return  None
 *
 *****************************************************************************/
static void henery_the_baby_chicken_hawk_crash_dual_sp(void)
{
    /* Destroy the SPs and its current session */
    fbe_test_base_suite_destroy_sp();

    /* Start the SPs with a new session */
    fbe_test_base_suite_startup_post_powercycle();

    /* Load the Physical_config of the test.. */
    henery_the_baby_chicken_hawk_setup();

    return;
}

/*!****************************************************************************
 *  henery_the_baby_chicken_hawk_crash_dual_sp
 ******************************************************************************
 *
 * @brief
 *   This function will wait for the EOL bit to get set in the PVD.
 *
 * @param   object_id - the object id of the PVD
 *
 * @return  fbe_status_t
 *
 *****************************************************************************/
static fbe_status_t henery_the_baby_chicken_hawk_wait_for_eol_set(fbe_object_id_t object_id)
{
	fbe_status_t status = FBE_STATUS_OK;
	fbe_u32_t    current_time = 0;
	fbe_api_provision_drive_info_t        pvd_info;

	while (current_time < 200000){

		current_time = current_time + 100;
		fbe_api_sleep (100);

		/* Ensure that EOL is set */
	    fbe_api_provision_drive_get_info(object_id, &pvd_info);
	    if (pvd_info.end_of_life_state)
	    {
	    	return status;
	    }

	}

	return FBE_STATUS_GENERIC_FAILURE;
}

/*************************
 * end file henery_the_baby_chicken_hawk_test.c
 *************************/

