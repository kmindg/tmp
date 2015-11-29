/***************************************************************************
 * Copyright (C) EMC Corporation 2012
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file starbuck_test.c
 ***************************************************************************
 *
 * @brief
 *  This file contains a test of zeroing with reboot.
 *
 * @version
 *  8/17/2012 - Created. Rob Foley
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "mut.h"   
#include "fbe/fbe_api_rdgen_interface.h"
#include "fbe/fbe_api_lun_interface.h"
#include "sep_tests.h"
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_service.h"
#include "fbe/fbe_transport.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_utils.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_api_provision_drive_interface.h"
#include "fbe/fbe_api_virtual_drive_interface.h"
#include "fbe/fbe_api_database_interface.h"
#include "fbe/fbe_api_sim_server.h"
#include "fbe/fbe_random.h"
#include "fbe_test_package_config.h"
#include "fbe_test_configurations.h"
#include "sep_utils.h"
#include "sep_test_io.h"
#include "pp_utils.h"
#include "fbe/fbe_api_sep_io_interface.h"
#include "fbe/fbe_api_raid_group_interface.h"
#include "fbe/fbe_api_base_config_interface.h"
#include "fbe/fbe_api_common_transport.h"
#include "fbe_test_common_utils.h"
#include "sep_test_region_io.h"
#include "fbe/fbe_api_scheduler_interface.h"
#include "fbe_test.h"
#include "fbe/fbe_api_event_log_interface.h"

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

char * starbuck_short_desc = "Zeroing with reboot test";
char * starbuck_long_desc = "\
The starbuck scenario tests that zeroing will continue after a reboot.\n\
\n\
Note that this test must be in the dual sp test suite since that suite uses\n\
the simulated drive which can persist drive data across reboots.\n"\
                            "\n\
STEP 1: Add a hook so that zeroing does not make progres in the PVD.\n\
STEP 2: Kick off disk zeroing.\n\
STEP 3: Reboot SP and confirm that disk zeroing runs and completes when the SP is rebooted.\n\
\n"\
                            "\n\
Description last updated: 8/21/2012.\n";


/*!*******************************************************************
 * @def STARBUCK_TEST_LUNS_PER_RAID_GROUP
 *********************************************************************
 * @brief luns per rg for the extended test. 
 *
 *********************************************************************/
#define STARBUCK_TEST_LUNS_PER_RAID_GROUP 3

/* Element size of our LUNs.
 */
#define STARBUCK_TEST_ELEMENT_SIZE 128 

/* Capacity of VD is based on a 32 MB sim drive.
 */
#define STARBUCK_TEST_VIRTUAL_DRIVE_CAPACITY_IN_BLOCKS (TERMINATOR_MINIMUM_NON_SYSTEM_DRIVE_CAPACITY)

/*!*******************************************************************
 * @def STARBUCK_MAX_WAIT_SEC
 *********************************************************************
 * @brief Arbitrary wait time that is long enough that if something goes
 *        wrong we will not hang.
 *
 *********************************************************************/
#define STARBUCK_MAX_WAIT_SEC 120

/*!*******************************************************************
 * @def STARBUCK_MAX_WAIT_MSEC
 *********************************************************************
 * @brief Arbitrary wait time that is long enough that if something goes
 *        wrong we will not hang.
 *
 *********************************************************************/
#define STARBUCK_MAX_WAIT_MSEC STARBUCK_MAX_WAIT_SEC * 1000

/*!*******************************************************************
 * @def STARBUCK_CHUNKS_PER_LUN
 *********************************************************************
 * @brief Number of chunks each LUN will occupy.
 *
 *********************************************************************/
#define STARBUCK_CHUNKS_PER_LUN 3

/*!*******************************************************************
 * @var starbuck_raid_group_config
 *********************************************************************
 * @brief This is the array of configurations we will loop over.
 *
 *********************************************************************/
fbe_test_rg_configuration_t starbuck_raid_group_config[] = 
{
    /* width,   capacity    raid type,                  class,                  block size      RAID-id.    bandwidth.*/
    {FBE_TEST_RG_CONFIG_RANDOM_TAG, 0xE000, FBE_TEST_RG_CONFIG_RANDOM_TAG,  FBE_TEST_RG_CONFIG_RANDOM_TAG,  520, 0,   FBE_TEST_RG_CONFIG_RANDOM_TAG},

    {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX,    /* Terminator. */},
};

/*!**************************************************************
 * fbe_test_setup_load_time_debug_hook()
 ****************************************************************
 * @brief
 *  Setup a debug hook that can be used at load time.
 * 
 * @param hook_p
 * @param object_id
 * @param monitor_state
 * @param monitor_substate
 * @param val1
 * @param val2
 * @param check_type
 * @param action
 * 
 * @return fbe_status_t             
 * 
 * @author
 *  8/21/2012 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_test_setup_load_time_debug_hook(fbe_scheduler_debug_hook_t *hook_p,
                                                 fbe_object_id_t object_id,
                                                 fbe_u32_t monitor_state,
                                                 fbe_u32_t monitor_substate,
                                                 fbe_u64_t val1,
                                                 fbe_u64_t val2,
                                                 fbe_u32_t check_type,
                                                 fbe_u32_t action)
{
    hook_p->object_id = object_id;
    hook_p->monitor_state = monitor_state;
    hook_p->monitor_substate = monitor_substate;
    hook_p->val1 = val1;
    hook_p->val2 = val2;
    hook_p->check_type = check_type;
    hook_p->action = action;
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_test_setup_load_time_debug_hook()
 ******************************************/
/*!**************************************************************
 * starbuck_reboot_sp()
 ****************************************************************
 * @brief
 *  - We created some LUNs that did initial verify.
 *  - We created other LUNs that did not do initial verify.
 *  - Reboot the SP.
 *
 * @param rg_config_p - Config array to create.  This is null terminated.
 *
 * @return None.
 *
 * @author
 *  8/21/2012 - Created. Rob Foley
 *
 ****************************************************************/
void starbuck_reboot_sp(fbe_test_rg_configuration_t *rg_config_p, fbe_object_id_t pvd_object_id)
{
    fbe_sim_transport_connection_target_t   target_sp;
    fbe_sim_transport_connection_target_t   peer_sp;
    fbe_test_rg_configuration_t temp_config[FBE_TEST_RG_CONFIG_ARRAY_MAX_TYPE];
    fbe_sep_package_load_params_t sep_params; 
    fbe_neit_package_load_params_t neit_params; 
    fbe_u32_t raid_group_count = fbe_test_get_rg_array_length(rg_config_p);

    mut_printf(MUT_LOG_LOW, "Crash both SPs");
    /* Get the local SP ID
     */
    target_sp = fbe_api_sim_transport_get_target_server();
    MUT_ASSERT_INT_NOT_EQUAL(FBE_SIM_INVALID_SERVER, target_sp);

    /* set the passive SP
     */
    peer_sp = (target_sp == FBE_SIM_SP_A ? FBE_SIM_SP_B : FBE_SIM_SP_A);

    /* Destroy the SPs and its current session
     */
    fbe_test_base_suite_destroy_sp();

    /* Start the SPs with a new session
     */
    fbe_test_base_suite_startup_post_powercycle();

    fbe_test_sep_duplicate_config(rg_config_p, temp_config);

    /* load the physical config
     */
    elmo_create_physical_config_for_rg(&temp_config[0], raid_group_count);

    /* Bring up a single SP.
     */
    sep_config_fill_load_params(&sep_params);
    neit_config_fill_load_params(&neit_params);

    mut_printf(MUT_LOG_TEST_STATUS, "%s Starting", __FUNCTION__);
    fbe_test_setup_load_time_debug_hook(&sep_params.scheduler_hooks[0],
                                        pvd_object_id,
                                        SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_ZERO,
                                        FBE_PROVISION_DRIVE_SUBSTATE_UNCONSUMED_ZERO_SEND,
                                        0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE);
    sep_config_load_sep_and_neit_params(&sep_params, &neit_params);
}
/**************************************
 * end starbuck_reboot_sp()
 **************************************/
/*!**************************************************************
 * starbuck_reboot_test()
 ****************************************************************
 * @brief
 *  - We created some LUNs that did initial verify.
 *  - We created other LUNs that did not do initial verify.
 *  - Reboot the SP.
 *
 * @param rg_config_p - Config array to create.  This is null terminated.
 *
 * @return None.
 *
 * @author
 *  8/21/2012 - Created. Rob Foley
 *
 ****************************************************************/

void starbuck_reboot_test(fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_status_t status;
    fbe_u32_t raid_group_count = fbe_test_get_rg_array_length(rg_config_p);
    fbe_object_id_t pvd_object_id;
	fbe_lba_t zero_checkpoint;

    mut_printf(MUT_LOG_TEST_STATUS, "== %s starting ==", __FUNCTION__);

    fbe_test_sep_util_raid_group_refresh_extra_disk_info(rg_config_p, raid_group_count);

    status = fbe_api_provision_drive_get_obj_id_by_location(rg_config_p->extra_disk_set[0].bus,
                                                            rg_config_p->extra_disk_set[0].enclosure,
                                                            rg_config_p->extra_disk_set[0].slot,
                                                            &pvd_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    status = fbe_api_wait_for_object_lifecycle_state(pvd_object_id, FBE_LIFECYCLE_STATE_READY, FBE_TEST_WAIT_TIMEOUT_MS, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* First install a hook so that zeroing does not run on this PVD.
     */
    status = fbe_api_scheduler_add_debug_hook(pvd_object_id,
                                              SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_ZERO,
                                              FBE_PROVISION_DRIVE_SUBSTATE_UNCONSUMED_ZERO_SEND,
                                              0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_TEST_STATUS, "== Start zeroing on PVD.");
    status = fbe_api_provision_drive_initiate_disk_zeroing(pvd_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_TEST_STATUS, "== Make sure zeroing is in progress.");
    status = fbe_test_zero_wait_for_zero_on_demand_state(pvd_object_id, FBE_TEST_HOOK_WAIT_MSEC, FBE_TRUE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

	status = fbe_api_provision_drive_get_zero_checkpoint(pvd_object_id, &zero_checkpoint);
	MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "zero_checkpoint = 0x%llx.", zero_checkpoint);

    /* Restart this SP.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== Reboot SP ==");
    starbuck_reboot_sp(rg_config_p, pvd_object_id);
    mut_printf(MUT_LOG_TEST_STATUS, "== Reboot SP Complete ==");
    status = fbe_api_wait_for_object_lifecycle_state(pvd_object_id, FBE_LIFECYCLE_STATE_READY, FBE_TEST_WAIT_TIMEOUT_MS, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_TEST_STATUS, "Make sure we have ZOD set and PVD checkpoint is not invalid");

	status = fbe_api_provision_drive_get_zero_checkpoint(pvd_object_id, &zero_checkpoint);
	MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "zero_checkpoint = 0x%llx.", zero_checkpoint);

    mut_printf(MUT_LOG_TEST_STATUS, "== Make sure we are still marked as zeroing.");
    status = fbe_test_zero_wait_for_zero_on_demand_state(pvd_object_id, FBE_TEST_HOOK_WAIT_MSEC, FBE_TRUE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Remove debug hook and let zeroing finish.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== remove debug hook and let zeroing finish ");
    status = fbe_api_scheduler_del_debug_hook(pvd_object_id,
                                              SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_ZERO,
                                              FBE_PROVISION_DRIVE_SUBSTATE_UNCONSUMED_ZERO_SEND,
                                              0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_TEST_STATUS, "== wait for zero to complete on pvd: 0x%x==",  pvd_object_id);
    fbe_test_zero_wait_for_disk_zeroing_complete(pvd_object_id, FBE_TEST_HOOK_WAIT_MSEC);

    status = fbe_test_sep_drive_check_for_disk_zeroing_event(pvd_object_id, 
                                                             SEP_INFO_PROVISION_DRIVE_OBJECT_DISK_ZEROING_COMPLETED,
                                                             rg_config_p->extra_disk_set[0].bus,
                                                             rg_config_p->extra_disk_set[0].enclosure,
                                                             rg_config_p->extra_disk_set[0].slot); 
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK); 

    /* we may have zero_on_demand cleared right away, because of the np write
     */
    status = fbe_test_zero_wait_for_zero_on_demand_state(pvd_object_id, FBE_TEST_HOOK_WAIT_MSEC, FBE_FALSE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_TEST_STATUS, "== %s complete ==", __FUNCTION__);
    return;
}
/**************************************
 * end starbuck_reboot_test()
 **************************************/
/*!**************************************************************
 * starbuck_run_tests()
 ****************************************************************
 * @brief
 *  Run the starbuck test.
 *
 * @param rg_config_p - Config array to create.  This is null terminated.
 * @param context_p - Not used.
 *
 * @return None.
 *
 * @author
 *  8/21/2012 - Created. Rob Foley
 *
 ****************************************************************/

void starbuck_run_tests(fbe_test_rg_configuration_t *rg_config_p, void * context_p)
{
    starbuck_reboot_test(rg_config_p);
    return;
}
/**************************************
 * end starbuck_run_tests()
 **************************************/
/*!**************************************************************
 * starbuck_test()
 ****************************************************************
 * @brief
 *  Run a verify test.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  8/21/2012 - Created. Rob Foley
 *
 ****************************************************************/

void starbuck_test(void)
{
    fbe_test_rg_configuration_t *rg_config_p = NULL;
    fbe_u32_t CSX_MAYBE_UNUSED test_level = fbe_sep_test_util_get_raid_testing_extended_level();

    /* Based on the test level determine which configuration to use.
     */
    rg_config_p = &starbuck_raid_group_config[0];
    fbe_test_run_test_on_rg_config(rg_config_p, NULL, starbuck_run_tests,
                                   STARBUCK_TEST_LUNS_PER_RAID_GROUP,
                                   STARBUCK_CHUNKS_PER_LUN);
    return;
}
/******************************************
 * end starbuck_test()
 ******************************************/
/*!**************************************************************
 * starbuck_setup()
 ****************************************************************
 * @brief
 *  Setup for a verify test.
 *
 * @param None.               
 *
 * @return None.   
 *
 * @author
 *  8/21/2012 - Created. Rob Foley
 *
 ****************************************************************/
void starbuck_setup(void)
{
    /* Only load the physical config in simulation.
     */
    if (fbe_test_util_is_simulation())
    {
        fbe_test_rg_configuration_t *rg_config_p = NULL;
        fbe_u32_t CSX_MAYBE_UNUSED test_level = fbe_sep_test_util_get_raid_testing_extended_level();
        fbe_u32_t num_raid_groups;

        /* Based on the test level determine which configuration to use.
        */
        rg_config_p = &starbuck_raid_group_config[0];
        fbe_test_sep_util_init_rg_configuration_array(rg_config_p);

        fbe_test_sep_util_randomize_rg_configuration_array(rg_config_p);

        num_raid_groups = fbe_test_get_rg_array_length(rg_config_p);
        fbe_test_sep_util_update_extra_chunk_size(rg_config_p, STARBUCK_CHUNKS_PER_LUN * 2);

        /* Always have an extra drive so we can use it to do disk zeroing.
         */
        fbe_test_rg_populate_extra_drives(rg_config_p, 1);

        elmo_create_physical_config_for_rg(rg_config_p, num_raid_groups);
        elmo_load_sep_and_neit();
    }

    /* Initialize any required fields and perform cleanup if required
     */
    fbe_test_common_util_test_setup_init();

    return;
}
/**************************************
 * end starbuck_setup()
 **************************************/

/*!**************************************************************
 * starbuck_cleanup()
 ****************************************************************
 * @brief
 *  Cleanup the starbuck test.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  8/21/2012 - Created. Rob Foley
 *
 ****************************************************************/

void starbuck_cleanup(void)
{
    mut_printf(MUT_LOG_HIGH, "%s entry", __FUNCTION__);
    if (fbe_test_util_is_simulation())
    {
        fbe_test_sep_util_destroy_neit_sep_physical();
    }
    return;
}
/******************************************
 * end starbuck_cleanup()
 ******************************************/
/*************************
 * end file starbuck_test.c
 *************************/


