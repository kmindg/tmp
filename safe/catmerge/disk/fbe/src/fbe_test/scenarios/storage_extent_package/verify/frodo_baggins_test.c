/***************************************************************************
 * Copyright (C) EMC Corporation 2012
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file frodo_baggins_test.c
 ***************************************************************************
 *
 * @brief
 *  This file contains a test of initial verify and no-inital verify.
 *
 * @version
 *  3/28/2012 - Created. Rob Foley
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
#include "fbe/fbe_random.h"

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

char * frodo_baggins_short_desc = "Initial background verify test.";
char * frodo_baggins_long_desc = "\
The frodo_baggins scenario tests the inital background verify functionality.\n\
\n\
Note that this test must be in the dual sp test suite since that suite uses\n\
the simulated drive which can persist drive data across reboots.\n"\
"\n\
STEP 1: Bind a LUN with initial background verify enabled by default.\n\
        - Bind the LUN.\n\
        - Wait for LUN zeroing to finish.\n\
        - Confirm that the initial background verify started and completed on each LUN.\n\
        - Confirm that the raid group verify finished.\n\
        - Confirm that the raid group does not have chunks marked for verify.\n"\
"\n\
STEP 2: Bind a LUN with no initial background verify.\n\
        - Set a raid group debug hook to hold background verify.\n\
        - Bind the LUN with the no initial background verify option set.\n\
        - Confirm that the initial background verify was not started.\n\
          - look at verify report and raid group checkpoints to confirm.\n\
          - Also confirm that the raid group has no chunks marked.\n"\
"\n\
STEP 3: Reboot SP and confirm that initial background verify is not started on either LUN.\n\
        - Prior two steps bound LUNs both with and without initial background verify set.\n\
        - Reboot the SP.\n\
        - Validate that no verifies have run by looking at the LUN verify report and the rg verify checkpoints.\n\
        - Validate that no chunks on the raid group are marked for verify.\n\
        - Verify LUNs remembered if they needed to do or actually previously ran initial verifies.\n\
\n"\
"\n\
Description last updated: 3/28/2011.\n";


/*!*******************************************************************
 * @def FRODO_BAGGINS_TEST_LUNS_PER_RAID_GROUP
 *********************************************************************
 * @brief luns per rg for the extended test. 
 *
 *********************************************************************/
#define FRODO_BAGGINS_TEST_LUNS_PER_RAID_GROUP 3

/* Element size of our LUNs.
 */
#define FRODO_BAGGINS_TEST_ELEMENT_SIZE 128 

/* Capacity of VD is based on a 32 MB sim drive.
 */
#define FRODO_BAGGINS_TEST_VIRTUAL_DRIVE_CAPACITY_IN_BLOCKS (TERMINATOR_MINIMUM_NON_SYSTEM_DRIVE_CAPACITY)

/*!*******************************************************************
 * @def FRODO_BAGGINS_MAX_WAIT_SEC
 *********************************************************************
 * @brief Arbitrary wait time that is long enough that if something goes
 *        wrong we will not hang.
 *
 *********************************************************************/
#define FRODO_BAGGINS_MAX_WAIT_SEC 120

/*!*******************************************************************
 * @def FRODO_BAGGINS_MAX_WAIT_MSEC
 *********************************************************************
 * @brief Arbitrary wait time that is long enough that if something goes
 *        wrong we will not hang.
 *
 *********************************************************************/
#define FRODO_BAGGINS_MAX_WAIT_MSEC FRODO_BAGGINS_MAX_WAIT_SEC * 1000

/*!*******************************************************************
 * @def FRODO_BAGGINS_CHUNKS_PER_LUN
 *********************************************************************
 * @brief Number of chunks each LUN will occupy.
 *
 *********************************************************************/
#define FRODO_BAGGINS_CHUNKS_PER_LUN 3

/*!*******************************************************************
 * @var frodo_baggins_raid_group_config
 *********************************************************************
 * @brief This is the array of configurations we will loop over.
 *
 *********************************************************************/
fbe_test_rg_configuration_t frodo_baggins_raid_group_config[] = 
{
    /* width,   capacity    raid type,                  class,                  block size      RAID-id.    bandwidth.*/
    {5,       0xE000,     FBE_RAID_GROUP_TYPE_RAID5,  FBE_CLASS_ID_PARITY,   520,            0,          0},
    {16,      0xE000,     FBE_RAID_GROUP_TYPE_RAID5,  FBE_CLASS_ID_PARITY,   520,            1,          0},

    {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
};
/*!****************************************************************************
 *  frodo_baggins_check_event_log_msg
 ******************************************************************************
 *
 * @brief
 *  This is the test function to check event messages for LUN zeroing operation.
 *
 * @param   pvd_object_id       - provision drive object id
 * @param   lun_number          - LUN number
 * @param   is_lun_verify_start  - 
 *                  FBE_TRUE  - check message for lun verifying start
 *                  FBE_FALSE - check message for lun verifying complete
 *
 * @author
 *  3/29/2012 - Created. Rob Foley 
 *
 *****************************************************************************/
static fbe_bool_t frodo_baggins_check_event_log_msg(fbe_object_id_t lun_object_id,
                                                    fbe_lun_number_t lun_number, 
                                                    fbe_bool_t  is_lun_verify_start)
{

    fbe_status_t                        status;               
    fbe_bool_t                          is_msg_present = FBE_FALSE;    
    fbe_u32_t                           message_code;               


    if (is_lun_verify_start == FBE_TRUE)
    {
        message_code = SEP_INFO_RAID_OBJECT_LU_AUTOMATIC_VERIFY_STARTED;
        /* check that given event message is logged correctly */
        status = fbe_api_check_event_log_msg(FBE_PACKAGE_ID_SEP_0, 
                                             &is_msg_present, 
                                             message_code, lun_number, lun_object_id);
    }
    else
    {
        message_code = SEP_INFO_RAID_OBJECT_LU_AUTOMATIC_VERIFY_COMPLETE;
        status = fbe_api_check_event_log_msg(FBE_PACKAGE_ID_SEP_0, 
                                             &is_msg_present, 
                                             message_code, lun_number, lun_object_id);
    }

    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    return is_msg_present; 
}
/*******************************************************
 * end frodo_baggins_check_event_log_msg()
 *******************************************************/
/*!**************************************************************
 * frodo_baggins_reboot_test()
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
 *  3/28/2012 - Created. Rob Foley
 *
 ****************************************************************/

void frodo_baggins_reboot_test(fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_status_t status;
    fbe_u32_t raid_group_count = fbe_test_get_rg_array_length(rg_config_p);
    fbe_test_rg_configuration_t *current_rg_config_p = NULL;
    fbe_u32_t rg_index;
    fbe_u32_t lun_index;
    fbe_object_id_t rg_object_id;
    fbe_object_id_t lun_object_id;
    fbe_api_lun_get_lun_info_t lun_info;
    fbe_lun_verify_report_t verify_report;
    fbe_api_raid_group_get_info_t rg_info;
    fbe_u32_t lun_number;
    fbe_sim_transport_connection_target_t   target_sp;

    mut_printf(MUT_LOG_TEST_STATUS, "== %s starting ==", __FUNCTION__);

    /* Get the local SP ID
     */
    target_sp = fbe_api_sim_transport_get_target_server();
    MUT_ASSERT_INT_NOT_EQUAL(FBE_SIM_INVALID_SERVER, target_sp);

    /* Restart this SP.
     */
    fbe_test_reboot_sp(rg_config_p, target_sp);

    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p))
        {
            current_rg_config_p++;
            continue;
        }
        fbe_test_sep_wait_for_rg_objects_ready_both_sps(rg_config_p);

        for (lun_index = 0; lun_index < current_rg_config_p->number_of_luns_per_rg; lun_index++)
        {
            lun_number = current_rg_config_p->logical_unit_configuration_list[lun_index].lun_number;
            status = fbe_api_database_lookup_lun_by_number(lun_number, &lun_object_id);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            status = fbe_test_sep_util_lun_get_verify_report(lun_object_id, &verify_report);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

            /* Make sure the LUN did not perform the initial verify on reboot. (from verify report).
             */
            MUT_ASSERT_INT_EQUAL(verify_report.pass_count, 0);
            status = fbe_api_lun_get_lun_info(lun_object_id, &lun_info);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

            if (lun_index < FRODO_BAGGINS_TEST_LUNS_PER_RAID_GROUP)
            {
                /* These LUNs were initial verify.  Make sure they still are and they still ran the verify already.
                 */
                MUT_ASSERT_INT_EQUAL(lun_info.b_initial_verify_already_run, 1);
                MUT_ASSERT_INT_EQUAL(lun_info.b_initial_verify_requested, 1);
            }
            else
            {
                /* Next make sure that the LUN was *not* requested to do initial verify and that it did not perform initial
                 * verify. 
                 */
                MUT_ASSERT_INT_EQUAL(lun_info.b_initial_verify_already_run, 0);
                MUT_ASSERT_INT_EQUAL(lun_info.b_initial_verify_requested, 0);
            }

            status = fbe_api_database_lookup_raid_group_by_number(current_rg_config_p->raid_group_id, &rg_object_id);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

            /* Get the raid group info.  Make sure no verifies in progress since the bind should not have kicked off the
             * verify. 
             */
            status = fbe_api_raid_group_get_info(rg_object_id, &rg_info, FBE_PACKET_FLAG_NO_ATTRIB);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

            if ((rg_info.ro_verify_checkpoint != FBE_LBA_INVALID) ||
                (rg_info.rw_verify_checkpoint != FBE_LBA_INVALID) ||
                (rg_info.error_verify_checkpoint != FBE_LBA_INVALID) ||
                (rg_info.system_verify_checkpoint != FBE_LBA_INVALID))
            {
                /* We do not expect a verify to be in progress.
                 * We intentionally do not look for error verifies since these are always kicked off 
                 * automatically when we do the paged at the raid group level, and when the paged is done 
                 * we always set the checkpoint to zero and do a scan once we get to ready.
                 */
                mut_printf(MUT_LOG_TEST_STATUS, "verify chkpts: ro: 0x%llx rw: 0x%llx iw: 0x%llx sys: 0x%llx ev: 0x%llx\n",
                           (unsigned long long)rg_info.ro_verify_checkpoint, (unsigned long long)rg_info.rw_verify_checkpoint, 
                           (unsigned long long)rg_info.incomplete_write_verify_checkpoint, (unsigned long long)rg_info.system_verify_checkpoint,
                           (unsigned long long)rg_info.error_verify_checkpoint);
                MUT_FAIL_MSG("verify in progress unexpectedly");
            }
        }
        current_rg_config_p++;
    }

    fbe_test_sep_util_validate_no_rg_verifies(rg_config_p, raid_group_count);

    mut_printf(MUT_LOG_TEST_STATUS, "== %s complete ==", __FUNCTION__);
    return;
}
/**************************************
 * end frodo_baggins_reboot_test()
 **************************************/
/*!**************************************************************
 * frodo_baggins_no_initial_verify_test()
 ****************************************************************
 * @brief
 *  - Bind a LUN with no initial verify set.
 *  - Wait for zeroing to complete.
 *  - Make sure that no verifies have occurred.
 *
 * @param rg_config_p - Config array to create.  This is null terminated.
 *
 * @return None.
 *
 * @author
 *  3/28/2012 - Created. Rob Foley
 *
 ****************************************************************/

void frodo_baggins_no_initial_verify_test(fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_status_t status;
    fbe_u32_t raid_group_count = fbe_test_get_rg_array_length(rg_config_p);
    fbe_test_rg_configuration_t *current_rg_config_p = NULL;
    fbe_u32_t rg_index;
    fbe_object_id_t rg_object_id;
    fbe_object_id_t lun_object_id;
    fbe_api_lun_create_t create_lun;
    fbe_u32_t lun_number = FRODO_BAGGINS_TEST_LUNS_PER_RAID_GROUP * raid_group_count;
    fbe_lun_verify_report_t verify_report;
    fbe_api_lun_get_lun_info_t lun_info;
    fbe_api_raid_group_get_info_t rg_info;
    fbe_job_service_error_type_t job_error_type;

    mut_printf(MUT_LOG_TEST_STATUS, "== %s starting ==", __FUNCTION__);

    fbe_zero_memory(&create_lun, sizeof(fbe_api_lun_create_t));
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p))
        {
            current_rg_config_p++;
            continue;
        }
        status = fbe_api_database_lookup_raid_group_by_number(current_rg_config_p->raid_group_id, &rg_object_id);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        
        fbe_test_sep_wait_for_rg_objects_ready_both_sps(current_rg_config_p);

        status = fbe_api_scheduler_add_debug_hook(rg_object_id,
                                                  SCHEDULER_MONITOR_STATE_RAID_GROUP_VERIFY,
                                                  FBE_RAID_GROUP_SUBSTATE_VERIFY_CHECKPOINT,
                                                  0,0,
                                                  SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        create_lun.raid_type        = current_rg_config_p->raid_type;
        create_lun.raid_group_id    = current_rg_config_p->raid_group_id;
        create_lun.lun_number       = FBE_LUN_ID_INVALID;
        create_lun.capacity         = current_rg_config_p->logical_unit_configuration_list[0].capacity;
        create_lun.placement        = FBE_BLOCK_TRANSPORT_BEST_FIT;
        create_lun.ndb_b            = FBE_FALSE;

        /* We want to explicitly disable the initial verify.
         */
        create_lun.noinitialverify_b = FBE_TRUE;
        create_lun.addroffset       = FBE_LBA_INVALID;

        create_lun.world_wide_name.bytes[0] = fbe_random() & 0xf;
        create_lun.world_wide_name.bytes[1] = fbe_random() & 0xf;

        status = fbe_api_create_lun(&create_lun, FBE_TRUE, FRODO_BAGGINS_MAX_WAIT_MSEC, &lun_object_id, &job_error_type);
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);   
        MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, lun_object_id);
        current_rg_config_p++;
        lun_number++;
    }

    lun_number = FRODO_BAGGINS_TEST_LUNS_PER_RAID_GROUP * raid_group_count;
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p))
        {
            current_rg_config_p++;
            continue;
        }
        current_rg_config_p->logical_unit_configuration_list[FRODO_BAGGINS_TEST_LUNS_PER_RAID_GROUP].lun_number = lun_number;
        status = fbe_api_database_lookup_lun_by_number(lun_number, &lun_object_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        status = fbe_test_sep_util_lun_get_verify_report(lun_object_id, &verify_report);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Make sure the LUN did not perform the initial verify. (from verify report).
         */
        MUT_ASSERT_INT_EQUAL(verify_report.pass_count, 0);

        /* Next make sure that the LUN was *not* requested to do initial verify and that it did not perform initial
         * verify. 
         */
        status = fbe_api_lun_get_lun_info(lun_object_id, &lun_info);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        MUT_ASSERT_INT_EQUAL(lun_info.b_initial_verify_already_run, 0);
        MUT_ASSERT_INT_EQUAL(lun_info.b_initial_verify_requested, 0);

        status = fbe_api_database_lookup_raid_group_by_number(current_rg_config_p->raid_group_id, &rg_object_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Get the raid group info.  Make sure no verifies in progress since the bind should not have kicked off the
         * verify. 
         */
        status = fbe_api_raid_group_get_info(rg_object_id, &rg_info, FBE_PACKET_FLAG_NO_ATTRIB);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        if ((rg_info.ro_verify_checkpoint != FBE_LBA_INVALID) ||
            (rg_info.rw_verify_checkpoint != FBE_LBA_INVALID) ||
            (rg_info.system_verify_checkpoint != FBE_LBA_INVALID) ||            
            (rg_info.incomplete_write_verify_checkpoint != FBE_LBA_INVALID) ||
            (rg_info.error_verify_checkpoint != FBE_LBA_INVALID))
        {
            /* We do not expect a verify to be in progress.
             */
            mut_printf(MUT_LOG_TEST_STATUS, "verify chkpts: ro: 0x%llx rw: 0x%llx iw: 0x%llx ld: 0x%llx ev: 0x%llx\n",
                       (unsigned long long)rg_info.ro_verify_checkpoint, (unsigned long long)rg_info.rw_verify_checkpoint, 
                       (unsigned long long)rg_info.incomplete_write_verify_checkpoint, (unsigned long long)rg_info.system_verify_checkpoint,
                       rg_info.error_verify_checkpoint);
            MUT_FAIL_MSG("verify in progress unexpectedly");
        }

        status = fbe_api_scheduler_del_debug_hook(rg_object_id,
                                                  SCHEDULER_MONITOR_STATE_RAID_GROUP_VERIFY,
                                                  FBE_RAID_GROUP_SUBSTATE_VERIFY_CHECKPOINT,
                                                  0,0,
                                                  SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);


        current_rg_config_p++;
        lun_number++;
    }

    fbe_test_sep_util_validate_no_rg_verifies(rg_config_p, raid_group_count);

    mut_printf(MUT_LOG_TEST_STATUS, "== %s complete ==", __FUNCTION__);
    return;
}
/**************************************
 * end frodo_baggins_no_initial_verify_test()
 **************************************/
/*!**************************************************************
 * frodo_baggins_initial_verify_test()
 ****************************************************************
 * @brief
 *  - Wait for zeroing to complete on LUNs.
 *  - Wait for initial verify pass to complete.
 *
 * @param rg_config_p - Config array to create.  This is null terminated.
 *
 * @return None.
 *
 * @author
 *  3/28/2012 - Created. Rob Foley
 *
 ****************************************************************/

void frodo_baggins_initial_verify_test(fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_status_t status;
    fbe_u32_t raid_group_count = fbe_test_get_rg_array_length(rg_config_p);
    fbe_test_rg_configuration_t *current_rg_config_p = NULL;
    fbe_u32_t rg_index;
    fbe_u32_t lun_index;
    fbe_object_id_t rg_object_id;
    fbe_object_id_t lun_object_id;
    fbe_api_lun_get_lun_info_t lun_info;

    mut_printf(MUT_LOG_TEST_STATUS, "== %s starting ==", __FUNCTION__);

    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p))
        {
            current_rg_config_p++;
            continue;
        }
        status = fbe_api_database_lookup_raid_group_by_number(current_rg_config_p->raid_group_id, &rg_object_id);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        
        fbe_test_sep_wait_for_rg_objects_ready_both_sps(rg_config_p);

        for (lun_index = 0; lun_index < current_rg_config_p->number_of_luns_per_rg; lun_index++)
        {
            fbe_lun_verify_report_t verify_report;
            fbe_lun_number_t lun_number =  current_rg_config_p->logical_unit_configuration_list[lun_index].lun_number;

            MUT_ASSERT_INT_EQUAL(current_rg_config_p->logical_unit_configuration_list[lun_index].raid_group_id, current_rg_config_p->raid_group_id);
            status = fbe_api_database_lookup_lun_by_number(lun_number, &lun_object_id);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

            /* First make sure we finished the zeroing.
             */
            fbe_test_sep_util_wait_for_lun_zeroing_complete(lun_number, 60000, FBE_PACKAGE_ID_SEP_0);

            /* Then make sure we finished the verify.  This waits for pass 1 to complete.
             */
            fbe_test_sep_util_wait_for_lun_verify_completion(lun_object_id, &verify_report, 1);

            /* Next make sure that the LUN was requested to do initial verify and that it performed it already.
             */
            status = fbe_api_lun_get_lun_info(lun_object_id, &lun_info);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            MUT_ASSERT_INT_EQUAL(lun_info.b_initial_verify_already_run, 1);
            MUT_ASSERT_INT_EQUAL(lun_info.b_initial_verify_requested, 1);

            /* Make sure the correct event log messages are there.
             */
            frodo_baggins_check_event_log_msg(lun_object_id, lun_number, FBE_TRUE);
            frodo_baggins_check_event_log_msg(lun_object_id, lun_number, FBE_FALSE);
        }

        /* Now that the LUN verifies are done, wait for the rg verify to complete.
         * This is needed so the next test does not stumble on verifies running.
         */
        abby_cadabby_wait_for_raid_group_verify(rg_object_id, FRODO_BAGGINS_MAX_WAIT_SEC);

        current_rg_config_p++;
    }
    /* Make sure nothing is set.
     */
    fbe_test_sep_util_validate_no_rg_verifies(rg_config_p, raid_group_count);

    mut_printf(MUT_LOG_TEST_STATUS, "== %s complete ==", __FUNCTION__);
    return;
}
/**************************************
 * end frodo_baggins_initial_verify_test()
 **************************************/
/*!**************************************************************
 * frodo_baggins_run_tests()
 ****************************************************************
 * @brief
 *  Run the initial verify test.
 *
 * @param rg_config_p - Config array to create.  This is null terminated.
 * @param context_p - Not used.
 *
 * @return None.
 *
 * @author
 *  3/28/2012 - Created. Rob Foley
 *
 ****************************************************************/

void frodo_baggins_run_tests(fbe_test_rg_configuration_t *rg_config_p, void * context_p)
{

    frodo_baggins_initial_verify_test(rg_config_p);
    frodo_baggins_no_initial_verify_test(rg_config_p);
    frodo_baggins_reboot_test(rg_config_p);
    return;
}
/**************************************
 * end frodo_baggins_run_tests()
 **************************************/
/*!**************************************************************
 * frodo_baggins_test()
 ****************************************************************
 * @brief
 *  Run a verify test.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  3/29/2012 - Created. Rob Foley
 *
 ****************************************************************/

void frodo_baggins_test(void)
{
    fbe_test_rg_configuration_t *rg_config_p = NULL;
    fbe_u32_t CSX_MAYBE_UNUSED test_level = fbe_sep_test_util_get_raid_testing_extended_level();

    /* Based on the test level determine which configuration to use.
     */
    rg_config_p = &frodo_baggins_raid_group_config[0];
    fbe_test_run_test_on_rg_config(rg_config_p, NULL, frodo_baggins_run_tests,
                                   FRODO_BAGGINS_TEST_LUNS_PER_RAID_GROUP,
                                   FRODO_BAGGINS_CHUNKS_PER_LUN);
    return;
}
/******************************************
 * end frodo_baggins_test()
 ******************************************/
/*!**************************************************************
 * frodo_baggins_setup()
 ****************************************************************
 * @brief
 *  Setup for a verify test.
 *
 * @param None.               
 *
 * @return None.   
 *
 * @author
 *  3/29/2012 - Created. Rob Foley
 *
 ****************************************************************/
void frodo_baggins_setup(void)
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
        rg_config_p = &frodo_baggins_raid_group_config[0];
        fbe_test_sep_util_init_rg_configuration_array(rg_config_p);
        num_raid_groups = fbe_test_get_rg_array_length(rg_config_p);
        fbe_test_sep_util_update_extra_chunk_size(rg_config_p, FRODO_BAGGINS_CHUNKS_PER_LUN * 2);

        /* initialize the number of extra drive required by each rg 
          */
        fbe_test_sep_util_populate_rg_num_extra_drives(rg_config_p);

        elmo_create_physical_config_for_rg(rg_config_p, num_raid_groups);
        elmo_load_sep_and_neit();
    }

    /* Initialize any required fields and perform cleanup if required
     */
    fbe_test_common_util_test_setup_init();
    fbe_test_sep_util_set_chunks_per_rebuild(1);

    return;
}
/**************************************
 * end frodo_baggins_setup()
 **************************************/

/*!**************************************************************
 * frodo_baggins_cleanup()
 ****************************************************************
 * @brief
 *  Cleanup the frodo_baggins test.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  3/29/2012 - Created. Rob Foley
 *
 ****************************************************************/

void frodo_baggins_cleanup(void)
{
    mut_printf(MUT_LOG_HIGH, "%s entry", __FUNCTION__);
    if (fbe_test_util_is_simulation())
    {
        fbe_test_sep_util_destroy_neit_sep_physical();
    }
    return;
}
/******************************************
 * end frodo_baggins_cleanup()
 ******************************************/
/*************************
 * end file frodo_baggins_test.c
 *************************/


