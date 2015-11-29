/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file flexo_test.c
 ***************************************************************************
 *
 * @brief
 *   This file contains tests of R6 rebuilds.
 *
 * @version
 *   08/15/2011 - Created. Jason White
 *
 ***************************************************************************/


//-----------------------------------------------------------------------------
//  INCLUDES OF REQUIRED HEADER FILES:


#include "sep_rebuild_utils.h"                      //  SEP rebuild utils
#include "mut.h"                                    //  MUT unit testing infrastructure
#include "fbe/fbe_types.h"                          //  general types
#include "sep_utils.h"                              //  for fbe_test_raid_group_disk_set_t
#include "sep_tests.h"                              //  for sep_config_load_sep_and_neit()
#include "sep_test_io.h"                            //  for sep I/O tests
#include "fbe_test_configurations.h"                //  for general configuration functions (elmo_, grover_)
#include "fbe/fbe_api_utils.h"                      //  for fbe_api_wait_for_number_of_objects
#include "fbe/fbe_api_discovery_interface.h"        //  for fbe_api_get_total_objects
#include "fbe/fbe_api_raid_group_interface.h"       //  for fbe_api_raid_group_get_info_t
#include "fbe/fbe_api_provision_drive_interface.h"  //  for fbe_api_provision_drive_get_obj_id_by_location
#include "fbe/fbe_api_virtual_drive_interface.h"    //  for fbe_api_control_automatic_hot_spare
#include "fbe/fbe_api_database_interface.h"
#include "pp_utils.h"                               //  for fbe_test_pp_util_pull_drive, _reinsert_drive
#include "fbe/fbe_api_logical_error_injection_interface.h"  // for error injection definitions
#include "fbe/fbe_random.h"                         //  for fbe_random()
#include "fbe_test_common_utils.h"                  //  for discovering config
#include "fbe/fbe_api_scheduler_interface.h"
#include "fbe/fbe_api_base_config_interface.h"
#include "fbe/fbe_api_lun_interface.h"

//-----------------------------------------------------------------------------
//  TEST DESCRIPTION:

char* flexo_test_short_desc = "R6 Rebuild test cases";
char* flexo_test_long_desc =
    "\n"
    "\n"
    "The Flexo Scenario is a test of R6 rebuild test cases.\n"
    "\n"
    "\n"
    "*** Flexo Phase 1 ********************************************************\n"
    "\n"
    "Phase 1 is a simple rebuild test for RAID-6. It tests doing a rebuild on 2 "
    "disks simultaneously."
    "\n"
    "\n"
	"*** Flexo Phase 2 ********************************************************\n"
	"\n"
	"Phase 2 is a simple rebuild test for RAID-6. It tests doing a rebuild on 2 "
	"disks one at a time."
	"\n"
	"\n"
	"*** Flexo Phase 3 ********************************************************\n"
	"\n"
	"Phase 3 is a simple rebuild test for RAID-6. It tests doing a rebuild on a "
	"first disk and then starting the 2nd drive rebuild while the 1st is already "
	"in progress."
	"\n"
	"\n"
	"*** Flexo Phase 4 ********************************************************\n"
	"\n"
	"Phase 4 is a simple rebuild test for RAID-6. It tests that the rebuild "
	"checkpoint is not reset after the RG goes broken. "
	"\n"
    "\n"
	"arguments:   -use_default_drives - Uses the set default drives for the test,"
	"                                   without this option random drives are used.\n"
    "\n"
        ;


//-----------------------------------------------------------------------------
//  TYPEDEFS, ENUMS, DEFINES, CONSTANTS, MACROS, GLOBALS:

#define FLEXO_NUM_DISKS_TO_REMOVE 2

//  Context for rdgen.  This is an array with an entry for every LUN defined by the test.  The context is used
//  to initialize all the LUNs of the test in a single operation.
static fbe_api_rdgen_context_t fbe_flexo_test_context_g[SEP_REBUILD_UTILS_LUNS_PER_RAID_GROUP * SEP_REBUILD_UTILS_NUM_RAID_GROUPS];

extern fbe_u32_t        sep_rebuild_utils_number_physical_objects_g;

#define FLEXO_TEST_CONFIGS 2

//  RG configurations
fbe_test_rg_configuration_t flexo_rg_config_g[] =
{
    //  Width, Capacity, Raid type, Class, Block size, RG number, Bandwidth
    {SEP_REBUILD_UTILS_TRIPLE_MIRROR_RAID_GROUP_WIDTH, SEP_REBUILD_UTILS_RG_CAPACITY, FBE_RAID_GROUP_TYPE_RAID1, FBE_CLASS_ID_MIRROR, 520, 2, 0},
    {SEP_REBUILD_UTILS_R6_RAID_GROUP_WIDTH, SEP_REBUILD_UTILS_RG_CAPACITY, FBE_RAID_GROUP_TYPE_RAID6, FBE_CLASS_ID_PARITY, 520, 4, 0},
    {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
};

//-----------------------------------------------------------------------------
//  FORWARD DECLARATIONS:
static void flexo_test_dual_rebuilds(
                                        fbe_test_rg_configuration_t*    in_rg_config_p,
                                        void*                           context_p);

static void flexo_test_dual_rebuilds_one_at_a_time(
                                        fbe_test_rg_configuration_t*    in_rg_config_p,
                                        void* 							context_p);

static void flexo_test_dual_rebuilds_second_during_first(
                                        fbe_test_rg_configuration_t*    in_rg_config_p,
                                        void* 							context_p);

static void flexo_test_dual_rebuilds_first_during_second(
                                        fbe_test_rg_configuration_t*    in_rg_config_p,
                                        void* 							context_p);

static void flexo_test_dual_rebuilds_second_during_first_hs(
                                        fbe_test_rg_configuration_t*    in_rg_config_p,
                                        void* 							context_p);

static void flexo_test_run_tests(
                                        fbe_test_rg_configuration_t*    in_rg_config_p, void * context_p);

/*!****************************************************************************
 *  flexo_test
 ******************************************************************************
 *
 * @brief
 *   This is the main entry point for the Flexo test.  The test does the
 *   following:
 *
 *   - Creates raid groups and Hot Spares as needed
 *   - For each raid group, tests rebuild of one or more drives
 *
 * @param   None
 *
 * @return  None
 *
 *****************************************************************************/
void flexo_test(void)
{
    fbe_test_rg_configuration_t *rg_config_p = NULL;
	fbe_lun_number_t        next_lun_id;        //  sets/gets number of the next lun (0, 1, 2)


    //  Initialize configuration setup parameters - start with 0
    next_lun_id = 0;

    //--------------------------------------------------------------------------
    //  Test RAID-6 - Differential Rebuild dual drive failure
    //

    rg_config_p = &flexo_rg_config_g[0];
    //  Set up and perform tests on the raid groups
    fbe_test_run_test_on_rg_config(rg_config_p, NULL,flexo_test_run_tests ,
    		SEP_REBUILD_UTILS_LUNS_PER_RAID_GROUP,
       								SEP_REBUILD_UTILS_CHUNKS_PER_LUN);
    //  Return
    return;

} //  End flexo_test()


/*!****************************************************************************
 *  flexo_test_setup
 ******************************************************************************
 *
 * @brief
 *   This is the common setup function for the Flexo test.  It creates the
 *   physical configuration and loads the packages.
 *
 * @param   None
 *
 * @return  None
 *
 *****************************************************************************/
void flexo_test_setup(void)
{
	fbe_scheduler_debug_hook_t hook;
	sep_rebuild_utils_setup();
	fbe_api_scheduler_clear_all_debug_hooks(&hook);

} //  End flexo_test_setup()

/*!****************************************************************************
 *  flexo_test_cleanup
 ******************************************************************************
 *
 * @brief
 *   This is the cleanup function for the Flexo test.
 *
 * @param   None
 *
 * @return  None
 *
 *****************************************************************************/
void flexo_test_cleanup(void)
{
    if (fbe_test_util_is_simulation())
    {
         fbe_test_sep_util_destroy_neit_sep_physical();
    }
    return;

}

static void flexo_test_run_tests(
                                    fbe_test_rg_configuration_t*    in_rg_config_p,
                                    void* 							context_p
                                 )
{
	flexo_test_dual_rebuilds(in_rg_config_p, context_p);
	flexo_test_dual_rebuilds_one_at_a_time(in_rg_config_p, context_p);
	flexo_test_dual_rebuilds_second_during_first(in_rg_config_p, context_p);
	flexo_test_dual_rebuilds_first_during_second(in_rg_config_p, context_p);
	flexo_test_dual_rebuilds_second_during_first_hs(in_rg_config_p, context_p);
}

/*!****************************************************************************
 *  flexo_test_dual_rebuilds
 ******************************************************************************
 *
 * @brief
 *   This function tests differential rebuilds (both at the same time) for a
 *   RAID-6 RAID group.  It does the following:
 *
 *   - Writes a seed pattern
 *   - Removes the drive in random position (or 0 if fixed) in the RG
 *   - Removes the drive in random position (or 1 if fixed) in the RG
 *   - Writes a new seed pattern
 *   - Reinserts the first drive
 *   - Waits for rebuild to start on that drive
 *   - Reinserts the 2nd drive
 *   - Waits for rebuild to start on that drive
 *   - inserts scheduler hook to pause the rebuild
 *   - Ensure that both rebuilds are in progress
 *   - Remove the scheduler hook
 *   - Waits for the rebuilds of both drives to finish
 *   - Verifies that the data is correct
 *
 *   At the end of this test, the RG is enabled.
 *
 * @param in_rg_config_p     - pointer to the RG configuration info
 * @param context_p          - pointer to the context
 *
 * @return  None
 *
 *****************************************************************************/
static void flexo_test_dual_rebuilds(
                                        fbe_test_rg_configuration_t*    in_rg_config_p,
                                        void* 							context_p)
{

    fbe_api_terminator_device_handle_t  drive_info[FLEXO_TEST_CONFIGS][FLEXO_NUM_DISKS_TO_REMOVE];                   //  drive info needed for reinsertion
    fbe_status_t                        status;
    fbe_u32_t 							i;
    fbe_u32_t 							rg_index;
    fbe_u32_t                           drive_slots_to_be_removed[FLEXO_TEST_CONFIGS][FLEXO_NUM_DISKS_TO_REMOVE];
    fbe_object_id_t                     raid_group_object_id[FLEXO_TEST_CONFIGS];
    fbe_scheduler_debug_hook_t          hook;
    fbe_lba_t                           target_checkpoint[FLEXO_TEST_CONFIGS];
    fbe_test_rg_configuration_t*        rg_config_p;

    //  Print message as to where the test is at
    mut_printf(MUT_LOG_TEST_STATUS, "%s", __FUNCTION__);

    if(fbe_test_rg_config_is_enabled(in_rg_config_p))
    {
        //  Get the number of physical objects in existence at this point in time.  This number is
        //  used when checking if a drive has been removed or has been reinserted.
        status = fbe_api_get_total_objects(&sep_rebuild_utils_number_physical_objects_g, FBE_PACKAGE_ID_PHYSICAL);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

        /* go through all the RG for which user has provided configuration data. */
       	for (rg_index = 0; rg_index < FLEXO_TEST_CONFIGS; rg_index++)
       	{
       		//  Write a seed pattern to the RG
       		sep_rebuild_utils_write_bg_pattern(&fbe_flexo_test_context_g[0], SEP_REBUILD_UTILS_ELEMENT_SIZE);
       	}

       	rg_config_p = in_rg_config_p;
		for (rg_index = 0; rg_index < FLEXO_TEST_CONFIGS; rg_index++)
		{
			if (fbe_test_sep_util_get_cmd_option("-use_default_drives"))
			{
				mut_printf(MUT_LOG_TEST_STATUS, "Using the Default drives...");
				drive_slots_to_be_removed[rg_index][0] = SEP_REBUILD_UTILS_POSITION_1;
				drive_slots_to_be_removed[rg_index][1] = SEP_REBUILD_UTILS_POSITION_2;
			}
			else
			{

				mut_printf(MUT_LOG_TEST_STATUS, "Using random drives...");
				sep_rebuild_utils_get_random_drives(rg_config_p, FLEXO_NUM_DISKS_TO_REMOVE, drive_slots_to_be_removed[rg_index]);
			}
			rg_config_p++;
		}

		rg_config_p = in_rg_config_p;
		for (rg_index = 0; rg_index < FLEXO_TEST_CONFIGS; rg_index++)
		{
			for(i=0;i<FLEXO_NUM_DISKS_TO_REMOVE;i++)
			{

				/* go through all the RG for which user has provided configuration data. */

				sep_rebuild_utils_number_physical_objects_g -= 1;

				//  Remove a single drive in the RG.  Check the object states change correctly and that rb logging
				//  is marked.
				sep_rebuild_utils_remove_drive_and_verify(rg_config_p, drive_slots_to_be_removed[rg_index][i], sep_rebuild_utils_number_physical_objects_g, &drive_info[rg_index][i]);
			}
			rg_config_p++;
		}

		mut_printf(MUT_LOG_TEST_STATUS, "Reading BG Pattern...");
		sep_rebuild_utils_read_bg_pattern(&fbe_flexo_test_context_g[0], SEP_REBUILD_UTILS_ELEMENT_SIZE);

		rg_config_p = in_rg_config_p;
		for (rg_index = 0; rg_index < FLEXO_TEST_CONFIGS; rg_index++)
		{
			mut_printf(MUT_LOG_TEST_STATUS, "Writing BG Pattern...");
			sep_rebuild_utils_write_new_data(rg_config_p, &fbe_flexo_test_context_g[0], 0x200);
			rg_config_p++;
		}


		rg_config_p = in_rg_config_p;
		for (rg_index = 0; rg_index < FLEXO_TEST_CONFIGS; rg_index++)
		{
			//  Get the RG's object id.
			fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &raid_group_object_id[rg_index]);

			mut_printf(MUT_LOG_TEST_STATUS, "Adding Debug Hook...");

			status = fbe_api_scheduler_add_debug_hook(raid_group_object_id[rg_index],
									SCHEDULER_MONITOR_STATE_RAID_GROUP_METADATA_REBUILD,
									FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY,
									NULL,
									NULL,
									SCHEDULER_CHECK_STATES,
									SCHEDULER_DEBUG_ACTION_PAUSE);

			MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
			rg_config_p++;
		}

		mut_printf(MUT_LOG_TEST_STATUS, "Reinserting drives...");

		rg_config_p = in_rg_config_p;
		for (rg_index = 0; rg_index < FLEXO_TEST_CONFIGS; rg_index++)
		{
			for(i=0;i<FLEXO_NUM_DISKS_TO_REMOVE;i++)
			{
				sep_rebuild_utils_number_physical_objects_g += 1;

				// Reinsert the drive and wait for the rebuild to start
				sep_rebuild_utils_reinsert_drive_and_verify(rg_config_p, drive_slots_to_be_removed[rg_index][i], sep_rebuild_utils_number_physical_objects_g, &drive_info[rg_index][i]);
			}

			rg_config_p++;
		}

		rg_config_p = in_rg_config_p;
		for (rg_index = 0; rg_index < FLEXO_TEST_CONFIGS; rg_index++)
		{
			//  Get the RG's object id.
			fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &raid_group_object_id[rg_index]);

			target_checkpoint[rg_index] = 0x800;

			mut_printf(MUT_LOG_TEST_STATUS, "Adding Debug Hook...");

			status = fbe_api_scheduler_add_debug_hook(raid_group_object_id[rg_index],
									SCHEDULER_MONITOR_STATE_RAID_GROUP_REBUILD,
									FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY,
									target_checkpoint[rg_index],
									NULL,
									SCHEDULER_CHECK_VALS_LT,
									SCHEDULER_DEBUG_ACTION_PAUSE);

			MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
			rg_config_p++;
		}

		rg_config_p = in_rg_config_p;
		for (rg_index = 0; rg_index < FLEXO_TEST_CONFIGS; rg_index++)
		{
			mut_printf(MUT_LOG_TEST_STATUS, "Getting Debug Hook...");

			hook.object_id = raid_group_object_id[rg_index];
			hook.action = SCHEDULER_DEBUG_ACTION_PAUSE;
			hook.check_type = SCHEDULER_CHECK_STATES;
			hook.monitor_state = SCHEDULER_MONITOR_STATE_RAID_GROUP_METADATA_REBUILD;
			hook.monitor_substate = FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY;
			hook.val1 = NULL;
			hook.val2 = NULL;
			hook.counter = 0;

			status = fbe_api_scheduler_get_debug_hook(&hook);

			MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

			mut_printf(MUT_LOG_TEST_STATUS,
				   "The hook was hit %llu times",
				   (unsigned long long)hook.counter);

			mut_printf(MUT_LOG_TEST_STATUS, "Deleting Debug Hook...");

			status = fbe_api_scheduler_del_debug_hook(raid_group_object_id[rg_index],
									SCHEDULER_MONITOR_STATE_RAID_GROUP_METADATA_REBUILD,
									FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY,
									NULL,
									NULL,
									SCHEDULER_CHECK_STATES,
									SCHEDULER_DEBUG_ACTION_PAUSE);

			MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
			rg_config_p++;
		}

		rg_config_p = in_rg_config_p;
		for (rg_index = 0; rg_index < FLEXO_TEST_CONFIGS; rg_index++)
		{
			for(i=0;i<FLEXO_NUM_DISKS_TO_REMOVE;i++)
			{
				sep_rebuild_utils_wait_for_rb_to_start(rg_config_p, drive_slots_to_be_removed[rg_index][i]);
				sep_rebuild_utils_check_for_reb_in_progress(rg_config_p, drive_slots_to_be_removed[rg_index][i]);
			}
			rg_config_p++;
		}

		rg_config_p = in_rg_config_p;
		for (rg_index = 0; rg_index < FLEXO_TEST_CONFIGS; rg_index++)
		{
			mut_printf(MUT_LOG_TEST_STATUS, "Getting Debug Hook...");

			hook.object_id = raid_group_object_id[rg_index];
			hook.action = SCHEDULER_DEBUG_ACTION_PAUSE;
			hook.check_type = SCHEDULER_CHECK_VALS_LT;
			hook.monitor_state = SCHEDULER_MONITOR_STATE_RAID_GROUP_REBUILD;
			hook.monitor_substate = FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY;
			hook.val1 = target_checkpoint[rg_index];
			hook.val2 = NULL;
			hook.counter = 0;

			status = fbe_api_scheduler_get_debug_hook(&hook);

			MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

			mut_printf(MUT_LOG_TEST_STATUS,
				   "The hook was hit %llu times",
				   (unsigned long long)hook.counter);

			mut_printf(MUT_LOG_TEST_STATUS, "Deleting Debug Hook...");

			status = fbe_api_scheduler_del_debug_hook(raid_group_object_id[rg_index],
									SCHEDULER_MONITOR_STATE_RAID_GROUP_REBUILD,
									FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY,
									target_checkpoint[rg_index],
									NULL,
									SCHEDULER_CHECK_VALS_LT,
									SCHEDULER_DEBUG_ACTION_PAUSE);

			MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
			rg_config_p++;
		}

		rg_config_p = in_rg_config_p;
		for (rg_index = 0; rg_index < FLEXO_TEST_CONFIGS; rg_index++)
		{
			for(i=0;i<FLEXO_NUM_DISKS_TO_REMOVE;i++)
			{
				//  Wait until the rebuilds finish
				sep_rebuild_utils_wait_for_rb_comp(rg_config_p, drive_slots_to_be_removed[rg_index][i]);
			}

			sep_rebuild_utils_check_bits(raid_group_object_id[rg_index]);

			/* Verify the SEP objects is in expected lifecycle State  */
			status = fbe_api_wait_for_object_lifecycle_state(raid_group_object_id[rg_index],
															 FBE_LIFECYCLE_STATE_READY,
															 60000,
															 FBE_PACKAGE_ID_SEP_0);

			//  Read the data and make sure it matches the new seed pattern
			sep_rebuild_utils_verify_new_data(rg_config_p, &fbe_flexo_test_context_g[0], 0x200);
			rg_config_p++;
		}
		sep_rebuild_utils_validate_event_log_errors();
    }

    //  Return
    return;

} //  End flexo_test_dual_rebuilds()

/*!****************************************************************************
 *  flexo_test_dual_rebuilds_one_at_a_time
 ******************************************************************************
 *
 * @brief
 *   This function tests differential rebuilds (one after another) for a RAID-6
 *   RAID group.  It does the following:
 *
 *   - Writes a seed pattern
 *   - Removes the drive in random position (or 0 if fixed) in the RG
 *   - Removes the drive in random position (or 1 if fixed) in the RG
 *   - Writes a new seed pattern
 *   - Reinserts the first drive
 *   - Waits for rebuild to finish on that drive
 *   - Reinserts the 2nd drive
 *   - Waits for rebuild to finish on that drive
 *   - Verifies that the data is correct
 *
 *   At the end of this test, the RG is enabled.
 *
 * @param in_rg_config_p     - pointer to the RG configuration info
 * @param context_p          - pointer to the context
 *
 * @return  None
 *
 *****************************************************************************/
static void flexo_test_dual_rebuilds_one_at_a_time(
                                        fbe_test_rg_configuration_t*    in_rg_config_p,
                                        void* 							context_p)
{

	fbe_api_terminator_device_handle_t  drive_info[FLEXO_TEST_CONFIGS][FLEXO_NUM_DISKS_TO_REMOVE];                   //  drive info needed for reinsertion
	fbe_status_t                        status;
	fbe_u32_t 							i;
	fbe_u32_t 							rg_index;
	fbe_u32_t                           drive_slots_to_be_removed[FLEXO_TEST_CONFIGS][FLEXO_NUM_DISKS_TO_REMOVE];
	fbe_test_rg_configuration_t*        rg_config_p;
	fbe_api_lun_get_status_t            lun_rebuild_status;
    fbe_api_rg_get_status_t             rg_rebuild_status;
    fbe_object_id_t                     raid_group_object_id[FLEXO_TEST_CONFIGS];
    fbe_object_id_t                     lun_object_id[FLEXO_TEST_CONFIGS];
    fbe_api_base_config_upstream_object_list_t    upstream_object_list;

    rg_config_p = in_rg_config_p;
    for (rg_index = 0; rg_index < FLEXO_TEST_CONFIGS; rg_index++)
    {
		/* To test rebuild status API for lun */
		status = fbe_api_database_lookup_raid_group_by_number(in_rg_config_p->raid_group_id, &raid_group_object_id[rg_index]);
		 MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
		/*Find the LUN belonging to RG i.e. find upstream obj of RG*/
		status = fbe_api_base_config_get_upstream_object_list(raid_group_object_id[rg_index], &upstream_object_list);
		MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
		lun_object_id[rg_index] = upstream_object_list.upstream_object_list[1];
		rg_config_p++;
    }

	//  Print message as to where the test is at
	mut_printf(MUT_LOG_TEST_STATUS, "%s", __FUNCTION__);

	if(fbe_test_rg_config_is_enabled(in_rg_config_p))
	{
		//  Get the number of physical objects in existence at this point in time.  This number is
		//  used when checking if a drive has been removed or has been reinserted.
		status = fbe_api_get_total_objects(&sep_rebuild_utils_number_physical_objects_g, FBE_PACKAGE_ID_PHYSICAL);
		MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

		/* go through all the RG for which user has provided configuration data. */
		for (rg_index = 0; rg_index < FLEXO_TEST_CONFIGS; rg_index++)
		{
			//  Write a seed pattern to the RG
			sep_rebuild_utils_write_bg_pattern(&fbe_flexo_test_context_g[0], SEP_REBUILD_UTILS_ELEMENT_SIZE);
		}

		rg_config_p = in_rg_config_p;
		for (rg_index = 0; rg_index < FLEXO_TEST_CONFIGS; rg_index++)
		{
			if (fbe_test_sep_util_get_cmd_option("-use_default_drives"))
			{
				mut_printf(MUT_LOG_TEST_STATUS, "Using the Default drives...");
				drive_slots_to_be_removed[rg_index][0] = SEP_REBUILD_UTILS_POSITION_1;
				drive_slots_to_be_removed[rg_index][1] = SEP_REBUILD_UTILS_POSITION_2;
			}
			else
			{

				mut_printf(MUT_LOG_TEST_STATUS, "Using random drives...");
				sep_rebuild_utils_get_random_drives(rg_config_p, FLEXO_NUM_DISKS_TO_REMOVE, drive_slots_to_be_removed[rg_index]);
			}
			rg_config_p++;
		}

		rg_config_p = in_rg_config_p;
		for (rg_index = 0; rg_index < FLEXO_TEST_CONFIGS; rg_index++)
		{
			for(i=0;i<FLEXO_NUM_DISKS_TO_REMOVE;i++)
			{

				/* go through all the RG for which user has provided configuration data. */

				sep_rebuild_utils_number_physical_objects_g -= 1;

				//  Remove a single drive in the RG.  Check the object states change correctly and that rb logging
				//  is marked.
				sep_rebuild_utils_remove_drive_and_verify(rg_config_p, drive_slots_to_be_removed[rg_index][i], sep_rebuild_utils_number_physical_objects_g, &drive_info[rg_index][i]);
			}
			rg_config_p++;
		}

		rg_config_p = in_rg_config_p;
		for (rg_index = 0; rg_index < FLEXO_TEST_CONFIGS; rg_index++)
		{
			 /*start - Rebuild checkpoint @ this point will be 0*/
			status = fbe_api_raid_group_get_rebuild_status(raid_group_object_id[rg_index],&rg_rebuild_status);
			MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
			MUT_ASSERT_INT_EQUAL(0, rg_rebuild_status.percentage_completed);

			status = fbe_api_lun_get_rebuild_status(lun_object_id[rg_index],&lun_rebuild_status);
			MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
			MUT_ASSERT_INT_EQUAL(0, lun_rebuild_status.percentage_completed);
		   /*end*/
			rg_config_p++;
		}

		mut_printf(MUT_LOG_TEST_STATUS, "Reading BG Pattern...");
		sep_rebuild_utils_read_bg_pattern(&fbe_flexo_test_context_g[0], SEP_REBUILD_UTILS_ELEMENT_SIZE);

		rg_config_p = in_rg_config_p;
		for (rg_index = 0; rg_index < FLEXO_TEST_CONFIGS; rg_index++)
		{
			mut_printf(MUT_LOG_TEST_STATUS, "Writing BG Pattern...");
			sep_rebuild_utils_write_new_data(rg_config_p, &fbe_flexo_test_context_g[0], 0x200);
			rg_config_p++;
		}

		rg_config_p = in_rg_config_p;
		for (rg_index = 0; rg_index < FLEXO_TEST_CONFIGS; rg_index++)
		{
			for(i=0;i<FLEXO_NUM_DISKS_TO_REMOVE;i++)
			{
				sep_rebuild_utils_number_physical_objects_g += 1;

				// Reinsert the drive and wait for the rebuild to start
				sep_rebuild_utils_reinsert_drive_and_verify(rg_config_p, drive_slots_to_be_removed[rg_index][i], sep_rebuild_utils_number_physical_objects_g, &drive_info[rg_index][i]);
				sep_rebuild_utils_wait_for_rb_to_start(rg_config_p, drive_slots_to_be_removed[rg_index][i]);				

				//  Wait until the rebuilds finish
				sep_rebuild_utils_wait_for_rb_comp(rg_config_p, drive_slots_to_be_removed[rg_index][i]);				

				//  Read the data and make sure it matches the new seed pattern
				sep_rebuild_utils_verify_new_data(rg_config_p, &fbe_flexo_test_context_g[0], 0x200);
			}
			sep_rebuild_utils_check_bits(raid_group_object_id[rg_index]);
			rg_config_p++;
		}

		rg_config_p = in_rg_config_p;
		for (rg_index = 0; rg_index < FLEXO_TEST_CONFIGS; rg_index++)
		{
			/*start - Rebuild percentage @ this point will be 100*/
			status = fbe_api_raid_group_get_rebuild_status(raid_group_object_id[rg_index],&rg_rebuild_status);
			MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
			MUT_ASSERT_INT_EQUAL(100,rg_rebuild_status.percentage_completed);

			status = fbe_api_lun_get_rebuild_status(lun_object_id[rg_index],&lun_rebuild_status);
			MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
			MUT_ASSERT_INT_EQUAL(100, lun_rebuild_status.percentage_completed);
			/*end*/
			rg_config_p++;
		}

        sep_rebuild_utils_validate_event_log_errors();
	}
	return;
} //  End flexo_test_dual_rebuilds_one_at_a_time()

/*!****************************************************************************
 *  flexo_test_dual_rebuilds_second_during_first
 ******************************************************************************
 *
 * @brief
 *   This function tests differential rebuilds (starting one after the other
 *   is in progress) for a RAID-6 RAID group.  It does the following:
 *
 *   - Writes a seed pattern
 *   - Removes the drive in random position (or 0 if fixed) in the RG
 *   - Removes the drive in random position (or 1 if fixed) in the RG
 *   - Writes a new seed pattern
 *   - Reinserts the first drive
 *   - Waits for rebuild to start on that drive
 *   - insert scheduler hook to pause rebuild
 *   - Reinserts the 2nd drive
 *   - removes hook
 *   - Waits for the rebuilds of both drives to finish
 *   - Verifies that the data is correct
 *
 *   At the end of this test, the RG is enabled.
 *
 * @param in_rg_config_p     - pointer to the RG configuration info
 * @param context_p          - pointer to the context
 *
 * @return  None
 *
 *****************************************************************************/
static void flexo_test_dual_rebuilds_second_during_first(
                                        fbe_test_rg_configuration_t*    in_rg_config_p,
                                        void* 							context_p)
{
	fbe_api_terminator_device_handle_t  drive_info[FLEXO_TEST_CONFIGS][FLEXO_NUM_DISKS_TO_REMOVE];                   //  drive info needed for reinsertion
	fbe_status_t                        status;
	fbe_u32_t 							i;
	fbe_u32_t 							rg_index;
	fbe_u32_t                           drive_slots_to_be_removed[FLEXO_TEST_CONFIGS][FLEXO_NUM_DISKS_TO_REMOVE];
	fbe_object_id_t                     raid_group_object_id[FLEXO_TEST_CONFIGS];
	fbe_scheduler_debug_hook_t          hook;
	fbe_lba_t                           target_checkpoint[FLEXO_TEST_CONFIGS][FLEXO_NUM_DISKS_TO_REMOVE];
	fbe_lba_t                           checkpoint;
	fbe_test_rg_configuration_t*        rg_config_p;

    //  Print message as to where the test is at
    mut_printf(MUT_LOG_TEST_STATUS, "%s", __FUNCTION__);

    if(fbe_test_rg_config_is_enabled(in_rg_config_p))
    {
    	//  Get the number of physical objects in existence at this point in time.  This number is
		//  used when checking if a drive has been removed or has been reinserted.
		status = fbe_api_get_total_objects(&sep_rebuild_utils_number_physical_objects_g, FBE_PACKAGE_ID_PHYSICAL);
		MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

		/* go through all the RG for which user has provided configuration data. */
		for (rg_index = 0; rg_index < FLEXO_TEST_CONFIGS; rg_index++)
		{
			//  Write a seed pattern to the RG
			sep_rebuild_utils_write_bg_pattern(&fbe_flexo_test_context_g[0], SEP_REBUILD_UTILS_ELEMENT_SIZE);
		}

		rg_config_p = in_rg_config_p;
		for (rg_index = 0; rg_index < FLEXO_TEST_CONFIGS; rg_index++)
		{
			if (fbe_test_sep_util_get_cmd_option("-use_default_drives"))
			{
				mut_printf(MUT_LOG_TEST_STATUS, "Using the Default drives...");
				drive_slots_to_be_removed[rg_index][0] = SEP_REBUILD_UTILS_POSITION_1;
				drive_slots_to_be_removed[rg_index][1] = SEP_REBUILD_UTILS_POSITION_2;
			}
			else
			{

				mut_printf(MUT_LOG_TEST_STATUS, "Using random drives...");
				sep_rebuild_utils_get_random_drives(rg_config_p, FLEXO_NUM_DISKS_TO_REMOVE, drive_slots_to_be_removed[rg_index]);
			}
			rg_config_p++;
		}

		rg_config_p = in_rg_config_p;
		for (rg_index = 0; rg_index < FLEXO_TEST_CONFIGS; rg_index++)
		{
			for(i=0;i<FLEXO_NUM_DISKS_TO_REMOVE;i++)
			{

				/* go through all the RG for which user has provided configuration data. */

				sep_rebuild_utils_number_physical_objects_g -= 1;

				//  Remove a single drive in the RG.  Check the object states change correctly and that rb logging
				//  is marked.
				sep_rebuild_utils_remove_drive_and_verify(rg_config_p, drive_slots_to_be_removed[rg_index][i], sep_rebuild_utils_number_physical_objects_g, &drive_info[rg_index][i]);
			}
			rg_config_p++;
		}

		mut_printf(MUT_LOG_TEST_STATUS, "Reading BG Pattern...");
		sep_rebuild_utils_read_bg_pattern(&fbe_flexo_test_context_g[0], SEP_REBUILD_UTILS_ELEMENT_SIZE);

		rg_config_p = in_rg_config_p;
		for (rg_index = 0; rg_index < FLEXO_TEST_CONFIGS; rg_index++)
		{
			mut_printf(MUT_LOG_TEST_STATUS, "Writing BG Pattern...");
			sep_rebuild_utils_write_new_data(rg_config_p, &fbe_flexo_test_context_g[0], 0x200);
			rg_config_p++;
		}


		rg_config_p = in_rg_config_p;
		for (rg_index = 0; rg_index < FLEXO_TEST_CONFIGS; rg_index++)
		{
			//  Get the RG's object id.
			fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &raid_group_object_id[rg_index]);

			target_checkpoint[rg_index][0] = 0x1800;

			mut_printf(MUT_LOG_TEST_STATUS, "Adding Debug Hook...");

			status = fbe_api_scheduler_add_debug_hook(raid_group_object_id[rg_index],
									SCHEDULER_MONITOR_STATE_RAID_GROUP_REBUILD,
									FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY,
									target_checkpoint[rg_index][0],
									NULL,
									SCHEDULER_CHECK_VALS_LT,
									SCHEDULER_DEBUG_ACTION_PAUSE);

			MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

			target_checkpoint[rg_index][1] = 0x2800;

			mut_printf(MUT_LOG_TEST_STATUS, "Adding 2nd Debug Hook...");

			status = fbe_api_scheduler_add_debug_hook(raid_group_object_id[rg_index],
									SCHEDULER_MONITOR_STATE_RAID_GROUP_REBUILD,
									FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY,
									target_checkpoint[rg_index][1],
									NULL,
									SCHEDULER_CHECK_VALS_LT,
									SCHEDULER_DEBUG_ACTION_PAUSE);

			MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

			rg_config_p++;
		}

		rg_config_p = in_rg_config_p;
		for (rg_index = 0; rg_index < FLEXO_TEST_CONFIGS; rg_index++)
		{
			sep_rebuild_utils_number_physical_objects_g += 1;

			// Reinsert the first drive and wait for the rebuild to start
			sep_rebuild_utils_reinsert_drive_and_verify(rg_config_p, drive_slots_to_be_removed[rg_index][0], sep_rebuild_utils_number_physical_objects_g, &drive_info[rg_index][0]);
			sep_rebuild_utils_wait_for_rb_to_start(rg_config_p, drive_slots_to_be_removed[rg_index][0]);
			rg_config_p++;
		}

		rg_config_p = in_rg_config_p;
		for (rg_index = 0; rg_index < FLEXO_TEST_CONFIGS; rg_index++)
		{
			sep_rebuild_utils_number_physical_objects_g += 1;

			// Reinsert the second drive
			sep_rebuild_utils_reinsert_drive_and_verify(rg_config_p, drive_slots_to_be_removed[rg_index][1], sep_rebuild_utils_number_physical_objects_g, &drive_info[rg_index][1]);
			rg_config_p++;
		}

		rg_config_p = in_rg_config_p;
		for (rg_index = 0; rg_index < FLEXO_TEST_CONFIGS; rg_index++)
		{
			mut_printf(MUT_LOG_TEST_STATUS, "Getting Debug Hook...");

			hook.object_id = raid_group_object_id[rg_index];
			hook.action = SCHEDULER_DEBUG_ACTION_PAUSE;
			hook.check_type = SCHEDULER_CHECK_VALS_LT;
			hook.monitor_state = SCHEDULER_MONITOR_STATE_RAID_GROUP_REBUILD;
			hook.monitor_substate = FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY;
			hook.val1 = target_checkpoint[rg_index][0];
			hook.val2 = NULL;
			hook.counter = 0;

			status = fbe_api_scheduler_get_debug_hook(&hook);

			MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

			mut_printf(MUT_LOG_TEST_STATUS,
				   "The hook was hit %llu times",
				   (unsigned long long)hook.counter);

			mut_printf(MUT_LOG_TEST_STATUS, "Deleting Debug Hook...");

			status = fbe_api_scheduler_del_debug_hook(raid_group_object_id[rg_index],
									SCHEDULER_MONITOR_STATE_RAID_GROUP_REBUILD,
									FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY,
									target_checkpoint[rg_index][0],
									NULL,
									SCHEDULER_CHECK_VALS_LT,
									SCHEDULER_DEBUG_ACTION_PAUSE);

			MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
			rg_config_p++;
		}

		rg_config_p = in_rg_config_p;
		for (rg_index = 0; rg_index < FLEXO_TEST_CONFIGS; rg_index++)
		{
			sep_rebuild_utils_get_reb_checkpoint(rg_config_p, drive_slots_to_be_removed[rg_index][1], &checkpoint);

			MUT_ASSERT_UINT64_EQUAL(checkpoint, 0);

			mut_printf(MUT_LOG_TEST_STATUS, "Getting 2nd Debug Hook...");

			hook.object_id = raid_group_object_id[rg_index];
			hook.action = SCHEDULER_DEBUG_ACTION_PAUSE;
			hook.check_type = SCHEDULER_CHECK_VALS_LT;
			hook.monitor_state = SCHEDULER_MONITOR_STATE_RAID_GROUP_REBUILD;
			hook.monitor_substate = FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY;
			hook.val1 = target_checkpoint[rg_index][1];
			hook.val2 = NULL;
			hook.counter = 0;

			status = fbe_api_scheduler_get_debug_hook(&hook);

			MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

			mut_printf(MUT_LOG_TEST_STATUS,
				   "The hook was hit %llu times",
				   (unsigned long long)hook.counter);

			mut_printf(MUT_LOG_TEST_STATUS, "Deleting 2nd Debug Hook...");

			status = fbe_api_scheduler_del_debug_hook(raid_group_object_id[rg_index],
									SCHEDULER_MONITOR_STATE_RAID_GROUP_REBUILD,
									FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY,
									target_checkpoint[rg_index][1],
									NULL,
									SCHEDULER_CHECK_VALS_LT,
									SCHEDULER_DEBUG_ACTION_PAUSE);

			MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
			rg_config_p++;
		}

		rg_config_p = in_rg_config_p;
		for (rg_index = 0; rg_index < FLEXO_TEST_CONFIGS; rg_index++)
		{
			for(i=0;i<FLEXO_NUM_DISKS_TO_REMOVE;i++)
			{
				//  Wait until the rebuilds finish
				sep_rebuild_utils_wait_for_rb_comp(rg_config_p, drive_slots_to_be_removed[rg_index][i]);
			}

			sep_rebuild_utils_check_bits(raid_group_object_id[rg_index]);

			/* Verify the SEP objects is in expected lifecycle State  */
			status = fbe_api_wait_for_object_lifecycle_state(raid_group_object_id[rg_index],
															 FBE_LIFECYCLE_STATE_READY,
															 60000,
															 FBE_PACKAGE_ID_SEP_0);

			//  Read the data and make sure it matches the new seed pattern
			sep_rebuild_utils_verify_new_data(rg_config_p, &fbe_flexo_test_context_g[0], 0x200);
			rg_config_p++;
		}
		sep_rebuild_utils_validate_event_log_errors();

    }

    //  Return
    return;

} //  End flexo_test_dual_rebuilds_second_during_first()

/*!****************************************************************************
 *  flexo_test_dual_rebuilds_second_during_first_hs
 ******************************************************************************
 *
 * @brief
 *   This function tests permanent spare rebuilds (starting one after the other
 *   is in progress) for a RAID-6 RAID group.  It does the following:
 *
 *   - Writes a seed pattern
 *   - Removes the drive in random position (or 0 if fixed) in the RG
 *   - Removes the drive in random position (or 1 if fixed) in the RG
 *   - Writes a new seed pattern
 *   - inserts HS
 *   - Waits for rebuild to start on that drive
 *   - insert scheduler hook to pause rebuild
 *   - inserts another HS
 *   - removes hook
 *   - Waits for the rebuilds of both drives to finish
 *   - Verifies that the data is correct
 *
 *   At the end of this test, the RG is enabled.
 *
 * @param in_rg_config_p     - pointer to the RG configuration info
 * @param context_p          - pointer to the context
 *
 * @return  None
 *
 *****************************************************************************/
static void flexo_test_dual_rebuilds_second_during_first_hs(
                                        fbe_test_rg_configuration_t*    in_rg_config_p,
                                        void* 							context_p)
{
	fbe_api_terminator_device_handle_t  drive_info[FLEXO_TEST_CONFIGS][FLEXO_NUM_DISKS_TO_REMOVE];                   //  drive info needed for reinsertion
	fbe_status_t                        status;
	fbe_test_raid_group_disk_set_t      hs_config[FLEXO_TEST_CONFIGS][FLEXO_NUM_DISKS_TO_REMOVE];
	fbe_u32_t                           hs_count = 0;
	fbe_u32_t                           i;
	fbe_u32_t                           rg_index;
	fbe_u32_t                           drive_slots_to_be_removed[FLEXO_TEST_CONFIGS][FLEXO_NUM_DISKS_TO_REMOVE];
	fbe_object_id_t                     raid_group_object_id[FLEXO_TEST_CONFIGS];
	fbe_scheduler_debug_hook_t          hook;
	fbe_lba_t                           target_checkpoint[FLEXO_TEST_CONFIGS][FLEXO_NUM_DISKS_TO_REMOVE];
	fbe_lba_t                           checkpoint;
	fbe_test_rg_configuration_t*        rg_config_p;
    fbe_raid_position_t rebuilding_position;
    fbe_raid_position_t second_rebuild_position[FLEXO_TEST_CONFIGS];
    fbe_raid_position_t first_rebuild_position[FLEXO_TEST_CONFIGS];

    //  Print message as to where the test is at
    mut_printf(MUT_LOG_TEST_STATUS, "%s", __FUNCTION__);

    if(fbe_test_rg_config_is_enabled(in_rg_config_p))
    {
        // Turn off automatic sparing.
        fbe_api_control_automatic_hot_spare(FBE_FALSE);

    	//  Get the number of physical objects in existence at this point in time.  This number is
		//  used when checking if a drive has been removed or has been reinserted.
		status = fbe_api_get_total_objects(&sep_rebuild_utils_number_physical_objects_g, FBE_PACKAGE_ID_PHYSICAL);
		MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

		/* go through all the RG for which user has provided configuration data. */
		for (rg_index = 0; rg_index < FLEXO_TEST_CONFIGS; rg_index++)
		{
			//  Write a seed pattern to the RG
			sep_rebuild_utils_write_bg_pattern(&fbe_flexo_test_context_g[0], SEP_REBUILD_UTILS_ELEMENT_SIZE);
		}

		rg_config_p = in_rg_config_p;
		for (rg_index = 0; rg_index < FLEXO_TEST_CONFIGS; rg_index++)
		{
			if (fbe_test_sep_util_get_cmd_option("-use_default_drives"))
			{
				mut_printf(MUT_LOG_TEST_STATUS, "Using the Default drives...");
				drive_slots_to_be_removed[rg_index][0] = SEP_REBUILD_UTILS_POSITION_1;
				drive_slots_to_be_removed[rg_index][1] = SEP_REBUILD_UTILS_POSITION_2;
			}
			else
			{

				mut_printf(MUT_LOG_TEST_STATUS, "Using random drives...");
				sep_rebuild_utils_get_random_drives(rg_config_p, FLEXO_NUM_DISKS_TO_REMOVE, drive_slots_to_be_removed[rg_index]);
			}
			rg_config_p++;
		}

		rg_config_p = in_rg_config_p;
		for (rg_index = 0; rg_index < FLEXO_TEST_CONFIGS; rg_index++)
		{
			//  Get the RG's object id.
			fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &raid_group_object_id[rg_index]);

			target_checkpoint[rg_index][0] = 0x800;

			mut_printf(MUT_LOG_TEST_STATUS, "Adding Debug Hook...");

			status = fbe_api_scheduler_add_debug_hook(raid_group_object_id[rg_index],
									SCHEDULER_MONITOR_STATE_RAID_GROUP_REBUILD,
									FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY,
									target_checkpoint[rg_index][0],
									NULL,
									SCHEDULER_CHECK_VALS_LT,
									SCHEDULER_DEBUG_ACTION_PAUSE);

			MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

			target_checkpoint[rg_index][1] = 0x2800;

			mut_printf(MUT_LOG_TEST_STATUS, "Adding 2nd Debug Hook...");

			status = fbe_api_scheduler_add_debug_hook(raid_group_object_id[rg_index],
									SCHEDULER_MONITOR_STATE_RAID_GROUP_REBUILD,
									FBE_RAID_GROUP_SUBSTATE_REBUILD_SINGLY_DEGRADED,
									0,
									NULL,
									SCHEDULER_CHECK_STATES,
									SCHEDULER_DEBUG_ACTION_PAUSE);

			MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
			rg_config_p++;
		}

		rg_config_p = in_rg_config_p;
		for (rg_index = 0; rg_index < FLEXO_TEST_CONFIGS; rg_index++)
		{
			for(i=0;i<FLEXO_NUM_DISKS_TO_REMOVE;i++)
			{

				/* go through all the RG for which user has provided configuration data. */

				sep_rebuild_utils_number_physical_objects_g -= 1;

				//  Remove a single drive in the RG.  Check the object states change correctly and that rb logging
				//  is marked.
				sep_rebuild_utils_remove_drive_and_verify(rg_config_p, drive_slots_to_be_removed[rg_index][i], sep_rebuild_utils_number_physical_objects_g, &drive_info[rg_index][i]);
			}

			mut_printf(MUT_LOG_TEST_STATUS, "Writing BG Pattern...");
			sep_rebuild_utils_write_new_data(rg_config_p, &fbe_flexo_test_context_g[0], 0x200);

			status = sep_rebuild_utils_find_free_drive(&hs_config[rg_index][0], rg_config_p->unused_disk_set, hs_count);

			if(status != FBE_STATUS_OK)
			{
				mut_printf(MUT_LOG_TEST_STATUS, "**** Failed to find a free drive to configure as HS . ****");
				return;
			}
			sep_rebuild_utils_setup_hot_spare(hs_config[rg_index][0].bus, hs_config[rg_index][0].enclosure, hs_config[rg_index][0].slot);
			hs_count++;

			status = sep_rebuild_utils_wait_for_any_rb_to_start(rg_config_p, &rebuilding_position);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

            if ((rebuilding_position != drive_slots_to_be_removed[rg_index][0]) &&
                (rebuilding_position != drive_slots_to_be_removed[rg_index][1]))
            {
                mut_printf(MUT_LOG_TEST_STATUS, "**** %s Rebuilding position %d not %d or %d ****\n", __FUNCTION__,
                           rebuilding_position, 
                           drive_slots_to_be_removed[rg_index][0],
                           drive_slots_to_be_removed[rg_index][1]);
                MUT_FAIL();
            }
            else
            {
                if (rebuilding_position == drive_slots_to_be_removed[rg_index][0])
                {
                    first_rebuild_position[rg_index] = drive_slots_to_be_removed[rg_index][0];
                    second_rebuild_position[rg_index] = drive_slots_to_be_removed[rg_index][1];
                }
                else
                {
                    second_rebuild_position[rg_index] = drive_slots_to_be_removed[rg_index][0];
                    first_rebuild_position[rg_index] = drive_slots_to_be_removed[rg_index][1];
                }
            }

			status = sep_rebuild_utils_find_free_drive(&hs_config[rg_index][1], rg_config_p->unused_disk_set, hs_count);

			if(status != FBE_STATUS_OK)
			{
				mut_printf(MUT_LOG_TEST_STATUS, "**** Failed to find a free drive to configure as HS . ****");
				return;
			}
			sep_rebuild_utils_setup_hot_spare(hs_config[rg_index][1].bus, hs_config[rg_index][1].enclosure, hs_config[rg_index][1].slot);

            /* Wait for the permanent spare to swap in.  This is needed since it is possible for this spare to
             * swap into the next raid group we are going to degrade.  If we wait for rebuild logging to get cleared, 
             * we are assured that the spares we added swapped into this raid group. 
             */
            sep_rebuild_utils_wait_for_rb_logging_clear(rg_config_p);
			hs_count++;
			rg_config_p++;
		}

        /* Now remove the first debug hook, this allows the first rebuild to finish.
         */
		rg_config_p = in_rg_config_p;
		for (rg_index = 0; rg_index < FLEXO_TEST_CONFIGS; rg_index++)
		{
			mut_printf(MUT_LOG_TEST_STATUS, "Getting Debug Hook...");

			hook.object_id = raid_group_object_id[rg_index];
			hook.action = SCHEDULER_DEBUG_ACTION_PAUSE;
			hook.check_type = SCHEDULER_CHECK_VALS_LT;
			hook.monitor_state = SCHEDULER_MONITOR_STATE_RAID_GROUP_REBUILD;
			hook.monitor_substate = FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY;
			hook.val1 = target_checkpoint[rg_index][0];
			hook.val2 = NULL;
			hook.counter = 0;

			status = fbe_api_scheduler_get_debug_hook(&hook);

			MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

			mut_printf(MUT_LOG_TEST_STATUS,
				   "The hook was hit %llu times",
				   (unsigned long long)hook.counter);

			mut_printf(MUT_LOG_TEST_STATUS, "Deleting Debug Hook...");

			status = fbe_api_scheduler_del_debug_hook(raid_group_object_id[rg_index],
									SCHEDULER_MONITOR_STATE_RAID_GROUP_REBUILD,
									FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY,
									target_checkpoint[rg_index][0],
									NULL,
									SCHEDULER_CHECK_VALS_LT,
									SCHEDULER_DEBUG_ACTION_PAUSE);

			MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
			rg_config_p++;
		}

		rg_config_p = in_rg_config_p;
		for (rg_index = 0; rg_index < FLEXO_TEST_CONFIGS; rg_index++)
		{
            /* Wait for first rebuild to finish.
             */
            sep_rebuild_utils_wait_for_rb_comp(rg_config_p, first_rebuild_position[rg_index]);
            /* Because of the second debug hook the second rebuild cannot start.
             * Validate the checkpoint is still 0. 
             */
			sep_rebuild_utils_get_reb_checkpoint(rg_config_p, second_rebuild_position[rg_index], &checkpoint);

			MUT_ASSERT_UINT64_EQUAL(checkpoint, 0);

			mut_printf(MUT_LOG_TEST_STATUS, "Getting 2nd Debug Hook...");

			hook.object_id = raid_group_object_id[rg_index];
			hook.action = SCHEDULER_DEBUG_ACTION_PAUSE;
			hook.check_type = SCHEDULER_CHECK_STATES;
			hook.monitor_state = SCHEDULER_MONITOR_STATE_RAID_GROUP_REBUILD;
			hook.monitor_substate = FBE_RAID_GROUP_SUBSTATE_REBUILD_SINGLY_DEGRADED,
			hook.val1 = 0;
			hook.val2 = NULL;
			hook.counter = 0;

			status = fbe_api_scheduler_get_debug_hook(&hook);

			MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

			mut_printf(MUT_LOG_TEST_STATUS,
				   "The hook was hit %llu times",
				   (unsigned long long)hook.counter);

            /* Removing the second debug hook allows the second rebuild to finish.
             */
			mut_printf(MUT_LOG_TEST_STATUS, "Deleting 2nd Debug Hook...");

			status = fbe_api_scheduler_del_debug_hook(raid_group_object_id[rg_index],
									SCHEDULER_MONITOR_STATE_RAID_GROUP_REBUILD,
									FBE_RAID_GROUP_SUBSTATE_REBUILD_SINGLY_DEGRADED,
									0,
									NULL,
									SCHEDULER_CHECK_STATES,
									SCHEDULER_DEBUG_ACTION_PAUSE);

			MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
			rg_config_p++;
		}

		rg_config_p = in_rg_config_p;
		for (rg_index = 0; rg_index < FLEXO_TEST_CONFIGS; rg_index++)
		{
            /* Wait for the 2nd rebuild to complete.
             */
            sep_rebuild_utils_wait_for_rb_comp(rg_config_p, second_rebuild_position[rg_index]);

			sep_rebuild_utils_check_bits(raid_group_object_id[rg_index]);

			/* Verify the SEP objects is in expected lifecycle State  */
			status = fbe_api_wait_for_object_lifecycle_state(raid_group_object_id[rg_index],
															 FBE_LIFECYCLE_STATE_READY,
															 60000,
															 FBE_PACKAGE_ID_SEP_0);

			//  Read the data and make sure it matches the new seed pattern
			sep_rebuild_utils_verify_new_data(rg_config_p, &fbe_flexo_test_context_g[0], 0x200);
			rg_config_p++;
		}
		sep_rebuild_utils_validate_event_log_errors();

        // Turn on automatic sparing.
        fbe_api_control_automatic_hot_spare(FBE_TRUE);

    }
    //  Return
    return;

} //  End flexo_test_dual_rebuilds_second_during_first_hs()

/*!****************************************************************************
 *  flexo_test_dual_rebuilds_first_during_second
 ******************************************************************************
 *
 * @brief
 *   This function tests differential rebuilds (starting one after the other
 *   is in progress) for a RAID-6 RAID group.  It does the following:
 *
 *   - Writes a seed pattern
 *   - Removes the drive in random position (or 0 if fixed) in the RG
 *   - Removes the drive in random position (or 1 if fixed) in the RG
 *   - Writes a new seed pattern
 *   - Reinserts the second drive
 *   - Waits for rebuild to start on that drive
 *   - insert scheduler hook to pause rebuild
 *   - Reinserts the 1st drive
 *   - removes hook
 *   - Waits for the rebuilds of both drives to finish
 *   - Verifies that the data is correct
 *
 *   At the end of this test, the RG is enabled.
 *
 * @param in_rg_config_p     - pointer to the RG configuration info
 * @param context_p          - pointer to the context
 *
 * @return  None
 *
 *****************************************************************************/
static void flexo_test_dual_rebuilds_first_during_second(
                                        fbe_test_rg_configuration_t*    in_rg_config_p,
                                        void* 							context_p)
{

	fbe_api_terminator_device_handle_t  drive_info[FLEXO_TEST_CONFIGS][FLEXO_NUM_DISKS_TO_REMOVE];                   //  drive info needed for reinsertion
	fbe_status_t                        status;
	fbe_u32_t 							i;
	fbe_u32_t 							rg_index;
	fbe_u32_t                           drive_slots_to_be_removed[FLEXO_TEST_CONFIGS][FLEXO_NUM_DISKS_TO_REMOVE];
	fbe_object_id_t                     raid_group_object_id[FLEXO_TEST_CONFIGS];
	fbe_scheduler_debug_hook_t          hook;
	fbe_lba_t                           target_checkpoint[FLEXO_TEST_CONFIGS][FLEXO_NUM_DISKS_TO_REMOVE];
	fbe_lba_t                           checkpoint;
	fbe_test_rg_configuration_t*        rg_config_p;

    //  Print message as to where the test is at
    mut_printf(MUT_LOG_TEST_STATUS, "%s", __FUNCTION__);

    if(fbe_test_rg_config_is_enabled(in_rg_config_p))
    {
    	//  Get the number of physical objects in existence at this point in time.  This number is
		//  used when checking if a drive has been removed or has been reinserted.
		status = fbe_api_get_total_objects(&sep_rebuild_utils_number_physical_objects_g, FBE_PACKAGE_ID_PHYSICAL);
		MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

		/* go through all the RG for which user has provided configuration data. */
		for (rg_index = 0; rg_index < FLEXO_TEST_CONFIGS; rg_index++)
		{
			//  Write a seed pattern to the RG
			sep_rebuild_utils_write_bg_pattern(&fbe_flexo_test_context_g[0], SEP_REBUILD_UTILS_ELEMENT_SIZE);
		}

		rg_config_p = in_rg_config_p;
		for (rg_index = 0; rg_index < FLEXO_TEST_CONFIGS; rg_index++)
		{
			if (fbe_test_sep_util_get_cmd_option("-use_default_drives"))
			{
				mut_printf(MUT_LOG_TEST_STATUS, "Using the Default drives...");
				drive_slots_to_be_removed[rg_index][0] = SEP_REBUILD_UTILS_POSITION_1;
				drive_slots_to_be_removed[rg_index][1] = SEP_REBUILD_UTILS_POSITION_2;
			}
			else
			{

				mut_printf(MUT_LOG_TEST_STATUS, "Using random drives...");
				sep_rebuild_utils_get_random_drives(rg_config_p, FLEXO_NUM_DISKS_TO_REMOVE, drive_slots_to_be_removed[rg_index]);
			}
			rg_config_p++;
		}

		rg_config_p = in_rg_config_p;
		for (rg_index = 0; rg_index < FLEXO_TEST_CONFIGS; rg_index++)
		{
			for(i=0;i<FLEXO_NUM_DISKS_TO_REMOVE;i++)
			{

				/* go through all the RG for which user has provided configuration data. */

				sep_rebuild_utils_number_physical_objects_g -= 1;

				//  Remove a single drive in the RG.  Check the object states change correctly and that rb logging
				//  is marked.
				sep_rebuild_utils_remove_drive_and_verify(rg_config_p, drive_slots_to_be_removed[rg_index][i], sep_rebuild_utils_number_physical_objects_g, &drive_info[rg_index][i]);
			}
			rg_config_p++;
		}

		mut_printf(MUT_LOG_TEST_STATUS, "Reading BG Pattern...");
		sep_rebuild_utils_read_bg_pattern(&fbe_flexo_test_context_g[0], SEP_REBUILD_UTILS_ELEMENT_SIZE);

		rg_config_p = in_rg_config_p;
		mut_printf(MUT_LOG_TEST_STATUS, "Writing BG Pattern...");
        fbe_test_sep_rebuild_generate_distributed_io(rg_config_p, FLEXO_TEST_CONFIGS, &fbe_flexo_test_context_g[0],
                                                      FBE_RDGEN_OPERATION_WRITE_ONLY,
                                                      FBE_RDGEN_PATTERN_LBA_PASS,
                                                      1);


		rg_config_p = in_rg_config_p;
		for (rg_index = 0; rg_index < FLEXO_TEST_CONFIGS; rg_index++)
		{
			//  Get the RG's object id.
			fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &raid_group_object_id[rg_index]);

			target_checkpoint[rg_index][0] = 0x800;

			mut_printf(MUT_LOG_TEST_STATUS, "Adding Debug Hook...");

			status = fbe_api_scheduler_add_debug_hook(raid_group_object_id[rg_index],
									SCHEDULER_MONITOR_STATE_RAID_GROUP_REBUILD,
									FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY,
									target_checkpoint[rg_index][0],
									NULL,
									SCHEDULER_CHECK_VALS_LT,
									SCHEDULER_DEBUG_ACTION_PAUSE);

			MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
			rg_config_p++;
		}

		rg_config_p = in_rg_config_p;
		for (rg_index = 0; rg_index < FLEXO_TEST_CONFIGS; rg_index++)
		{
			sep_rebuild_utils_number_physical_objects_g += 1;

			// Reinsert the first drive and wait for the rebuild to start
			sep_rebuild_utils_reinsert_drive_and_verify(rg_config_p, drive_slots_to_be_removed[rg_index][1], sep_rebuild_utils_number_physical_objects_g, &drive_info[rg_index][1]);
			sep_rebuild_utils_wait_for_rb_to_start(rg_config_p, drive_slots_to_be_removed[rg_index][1]);
			rg_config_p++;
		}

		rg_config_p = in_rg_config_p;
		for (rg_index = 0; rg_index < FLEXO_TEST_CONFIGS; rg_index++)
		{
			target_checkpoint[rg_index][1] = 0x1800;

			mut_printf(MUT_LOG_TEST_STATUS, "Adding 2nd Debug Hook...");

			status = fbe_api_scheduler_add_debug_hook(raid_group_object_id[rg_index],
									SCHEDULER_MONITOR_STATE_RAID_GROUP_REBUILD,
									FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY,
									target_checkpoint[rg_index][1],
									NULL,
									SCHEDULER_CHECK_VALS_LT,
									SCHEDULER_DEBUG_ACTION_PAUSE);

			MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
			rg_config_p++;
		}

		rg_config_p = in_rg_config_p;
		for (rg_index = 0; rg_index < FLEXO_TEST_CONFIGS; rg_index++)
		{
			sep_rebuild_utils_number_physical_objects_g += 1;

			// Reinsert the second drive
			sep_rebuild_utils_reinsert_drive_and_verify(rg_config_p, drive_slots_to_be_removed[rg_index][0], sep_rebuild_utils_number_physical_objects_g, &drive_info[rg_index][0]);
			rg_config_p++;
		}

		rg_config_p = in_rg_config_p;
		for (rg_index = 0; rg_index < FLEXO_TEST_CONFIGS; rg_index++)
		{
			mut_printf(MUT_LOG_TEST_STATUS, "Getting Debug Hook...");

			hook.object_id = raid_group_object_id[rg_index];
			hook.action = SCHEDULER_DEBUG_ACTION_PAUSE;
			hook.check_type = SCHEDULER_CHECK_VALS_LT;
			hook.monitor_state = SCHEDULER_MONITOR_STATE_RAID_GROUP_REBUILD;
			hook.monitor_substate = FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY;
			hook.val1 = target_checkpoint[rg_index][0];
			hook.val2 = NULL;
			hook.counter = 0;

			status = fbe_api_scheduler_get_debug_hook(&hook);

			MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

			mut_printf(MUT_LOG_TEST_STATUS,
				   "The hook was hit %llu times",
				   (unsigned long long)hook.counter);

			mut_printf(MUT_LOG_TEST_STATUS, "Deleting Debug Hook...");

			status = fbe_api_scheduler_del_debug_hook(raid_group_object_id[rg_index],
									SCHEDULER_MONITOR_STATE_RAID_GROUP_REBUILD,
									FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY,
									target_checkpoint[rg_index][0],
									NULL,
									SCHEDULER_CHECK_VALS_LT,
									SCHEDULER_DEBUG_ACTION_PAUSE);

			MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
			rg_config_p++;
		}

		rg_config_p = in_rg_config_p;
		for (rg_index = 0; rg_index < FLEXO_TEST_CONFIGS; rg_index++)
		{
			sep_rebuild_utils_get_reb_checkpoint(rg_config_p, drive_slots_to_be_removed[rg_index][0], &checkpoint);

            // fbe_raid_group_rebuild_find_disk_needing_action() is coded such way that if this position is after the earlier position
            // the checkpoint stays at the end of metadata, not reset to 0.
			//MUT_ASSERT_UINT64_EQUAL(checkpoint, 0);

			mut_printf(MUT_LOG_TEST_STATUS, "Getting 2nd Debug Hook...");

			hook.object_id = raid_group_object_id[rg_index];
			hook.action = SCHEDULER_DEBUG_ACTION_PAUSE;
			hook.check_type = SCHEDULER_CHECK_VALS_LT;
			hook.monitor_state = SCHEDULER_MONITOR_STATE_RAID_GROUP_REBUILD;
			hook.monitor_substate = FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY;
			hook.val1 = target_checkpoint[rg_index][1];
			hook.val2 = NULL;
			hook.counter = 0;

			status = fbe_api_scheduler_get_debug_hook(&hook);

			MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

			mut_printf(MUT_LOG_TEST_STATUS,
				   "The hook was hit %llu times",
				   (unsigned long long)hook.counter);

			mut_printf(MUT_LOG_TEST_STATUS, "Deleting 2nd Debug Hook...");

			status = fbe_api_scheduler_del_debug_hook(raid_group_object_id[rg_index],
									SCHEDULER_MONITOR_STATE_RAID_GROUP_REBUILD,
									FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY,
									target_checkpoint[rg_index][1],
									NULL,
									SCHEDULER_CHECK_VALS_LT,
									SCHEDULER_DEBUG_ACTION_PAUSE);

			MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
			rg_config_p++;
		}

		rg_config_p = in_rg_config_p;
		for (rg_index = 0; rg_index < FLEXO_TEST_CONFIGS; rg_index++)
		{
			for(i=0;i<FLEXO_NUM_DISKS_TO_REMOVE;i++)
			{
				//  Wait until the rebuilds finish
				sep_rebuild_utils_wait_for_rb_comp(rg_config_p, drive_slots_to_be_removed[rg_index][i]);
			}

			sep_rebuild_utils_check_bits(raid_group_object_id[rg_index]);

			/* Verify the SEP objects is in expected lifecycle State  */
			status = fbe_api_wait_for_object_lifecycle_state(raid_group_object_id[rg_index],
															 FBE_LIFECYCLE_STATE_READY,
															 60000,
															 FBE_PACKAGE_ID_SEP_0);

			rg_config_p++;
		}
		rg_config_p = in_rg_config_p;
		mut_printf(MUT_LOG_TEST_STATUS, "Verify BG Pattern...");
        fbe_test_sep_rebuild_generate_distributed_io(rg_config_p, FLEXO_TEST_CONFIGS, &fbe_flexo_test_context_g[0],
                                                      FBE_RDGEN_OPERATION_READ_CHECK,
                                                      FBE_RDGEN_PATTERN_LBA_PASS,
                                                      1);
		sep_rebuild_utils_validate_event_log_errors();

    }

    //  Return
    return;

} //  End flexo_test_dual_rebuilds_first_during_second()

