/***************************************************************************
 * Copyright (C) EMC Corporation 2012
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file pippin_test.c
 ***************************************************************************
 *
 * @brief
 *  This file has a test for injection of unexpected errors.
 *
 * @version
 *  6/26/2012 - Created. Rob Foley
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
#include "sep_hook.h"
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
#include "fbe/fbe_api_trace_interface.h"

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

char * pippin_short_desc = "Unexpected error injection test.";
char * pippin_long_desc = "\
The pippin scenario tests injection of unexpected errors.\n\
\n\
Errors over time test\n\
STEP 1) Run I/O with error injection for some number of I/Os X\n\
        which is guaranteed to cause the LUN to fail.\n\
STEP 2) Wait for the LUN to fail.  Clear the errors, and wait for the \n\
        LUN to come back.\n\
STEP 3) Run some number of I/Os X/2 with error injection which is not\n\
        enough to cause the LUNs to FAIL.  Ensure the LUNs do not fail.\n\
STEP 4) Run enough I/Os to cancel out this X/2 number of errors.\n\
        LUN to come back.\n\
STEP 5) Run some number of I/Os X/2 with error injection which is not\n\
        enough to cause the LUNs to FAIL.  Ensure the LUNs do not fail.\n\
\n\
Fail LUNs with Unexpected Errors Test.\n\
  The below steps are done with reads, writes and non-cached writes.\n\
\n\
STEP 1) Unexpected errors are injected on a raid group.\n\
STEP 2) I/O is sent to the LUNs on this raid group for some number of iterations.\n\
STEP 3) We validate that the LUN eventually goes failed.\n\
        We validate that the appropriate event log message was logged. \n\
STEP 4) We issue the usurper to get the LUN to come out of failed.\n\
STEP 5) Validate the LUN is now ready.\n\
\n\
Unexpected error handling disabled test.\n\
STEP 1) Disable unexpected error handling so the LUN will not FAIL when\n\
        errors are injected.\n\
STEP 2) Run I/O with errors injected.\n\
STEP 3) Make sure the LUNs do not FAIL.\n\
\n\
Description last updated: 6/26/2012.\n";

enum {
    PIPPIN_TEST_LUNS_PER_RAID_GROUP = 3,
    PIPPIN_MAX_WAIT_SEC = 120,
    PIPPIN_MAX_WAIT_MSEC = (PIPPIN_MAX_WAIT_SEC * 1000),
    PIPPIN_CHUNKS_PER_LUN = 4,
    PIPPIN_MAX_IO_BLOCKS = 128,
    PIPPIN_RAID_GROUP_COUNT = 6,
    PIPPIN_RDGEN_CONTEXT_COUNT = PIPPIN_RAID_GROUP_COUNT * 2, 
};

static fbe_api_rdgen_context_t pippin_rdgen_contexts[PIPPIN_RDGEN_CONTEXT_COUNT];

/*!*******************************************************************
 * @var pippin_raid_group_config
 *********************************************************************
 * @brief This is the array of configurations we will loop over.
 * 
 *        Plus one on rg count for the terminator.
 *
 *********************************************************************/
fbe_test_rg_configuration_t pippin_raid_group_config[PIPPIN_RAID_GROUP_COUNT + 1] = 
{
    /* width,                       capacity    raid type,                  class,       block size RAID-id. bandwidth. */
    { 6 /*FBE_TEST_RG_CONFIG_RANDOM_TAG*/, 0xE000, FBE_RAID_GROUP_TYPE_RAID10,  FBE_CLASS_ID_STRIPER, 520, 0, FBE_TEST_RG_CONFIG_RANDOM_TAG},
    {12 /*FBE_TEST_RG_CONFIG_RANDOM_TAG*/, 0xE000, FBE_RAID_GROUP_TYPE_RAID6,   FBE_CLASS_ID_PARITY,  520, 1, FBE_TEST_RG_CONFIG_RANDOM_TAG},
    {6 /*FBE_TEST_RG_CONFIG_RANDOM_TAG*/, 0xE000, FBE_RAID_GROUP_TYPE_RAID5,   FBE_CLASS_ID_PARITY,  520, 2, FBE_TEST_RG_CONFIG_RANDOM_TAG},
    {4 /*FBE_TEST_RG_CONFIG_RANDOM_TAG*/, 0xE000, FBE_RAID_GROUP_TYPE_RAID3,   FBE_CLASS_ID_PARITY,  520, 3, FBE_TEST_RG_CONFIG_RANDOM_TAG},
    {4 /*FBE_TEST_RG_CONFIG_RANDOM_TAG*/, 0xE000, FBE_RAID_GROUP_TYPE_RAID0,   FBE_CLASS_ID_STRIPER, 520, 5, FBE_TEST_RG_CONFIG_RANDOM_TAG},
    {3 /* FBE_TEST_RG_CONFIG_RANDOM_TAG*/, 0xE000, FBE_RAID_GROUP_TYPE_RAID1,   FBE_CLASS_ID_MIRROR,  520, 4, 0},
    {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
};

/*!****************************************************************************
 *  pippin_rdgen_context_index()
 ******************************************************************************
 * @brief 
 *  In some tests we use this function to determine which test context
 *  to use for which LUNs.
 *
 * @param lun_object_id - lun object id
 *
 * @author
 *  10/3/2012 - Created. Rob Foley
 *
 *****************************************************************************/
fbe_u32_t pippin_rdgen_context_index(fbe_u32_t raid_group, fbe_u32_t lun_index)
{
    return ((raid_group * 2) + lun_index);
}
/**************************************
 * end pippin_rdgen_context_index()
 **************************************/

/*!****************************************************************************
 *  pippin_check_event_log_msg_present
 ******************************************************************************
 *
 * @brief
 *  This is the test function to check event messages for LUN going failed due
 *  to too many software errors.
 *
 * @param lun_object_id - lun object id
 *
 * @author
 *  6/28/2012 - Created. Rob Foley
 *
 *****************************************************************************/
static void pippin_check_event_log_msg_present(fbe_object_id_t lun_object_id,
                                               fbe_lun_number_t lun_number)
{
    fbe_status_t                        status;               
    fbe_bool_t                          is_msg_present = FBE_FALSE;  

    /* check that given event message is logged correctly */
    status = fbe_api_check_event_log_msg(FBE_PACKAGE_ID_SEP_0, 
                                         &is_msg_present, 
                                         SEP_ERROR_CODE_LUN_UNEXPECTED_ERRORS, 
                                         lun_number, lun_object_id);

    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    if (!is_msg_present)
    {
        MUT_FAIL_MSG("SEP_ERROR_CODE_LUN_UNEXPECTED_ERRORS not found as expected");
    }
}
/*******************************************************
 * end pippin_check_event_log_msg_present()
 *******************************************************/
/*!****************************************************************************
 *  pippin_check_event_log_msg_not_present
 ******************************************************************************
 * @brief
 *  This is the test function to check event messages are not present
 *  for LUN going failed due to too many software errors.
 *
 * @param lun_object_id - lun object id
 *
 * @author
 *  6/28/2012 - Created. Rob Foley
 *
 *****************************************************************************/
static void pippin_check_event_log_msg_not_present(fbe_object_id_t lun_object_id,
                                          fbe_lun_number_t lun_number)
{
    fbe_status_t                        status;               
    fbe_bool_t                          is_msg_present = FBE_FALSE;  

    /* check that given event message is logged correctly */
    status = fbe_api_check_event_log_msg(FBE_PACKAGE_ID_SEP_0, 
                                         &is_msg_present, 
                                         SEP_ERROR_CODE_LUN_UNEXPECTED_ERRORS, 
                                         lun_number, lun_object_id);

    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    if (is_msg_present)
    {
        MUT_FAIL_MSG("SEP_ERROR_CODE_LUN_UNEXPECTED_ERRORS not expected");
    }
}
/*******************************************************
 * end pippin_check_event_log_msg_not_present()
 *******************************************************/

/*!**************************************************************
 * pippin_setup_rdgen_test_context()
 ****************************************************************
 * @brief
 *  Setup the context structure to run a random I/O test.
 *
 * @param context_p       - context of the operation.
 * @param object_id       - object id to run io against.
 * @param class_id        - class id to test.
 * @param rdgen_operation - operation to start.
 * @param capacity        - capacity in blocks.
 * @param max_passes      - number of passes to execute.
 * @param io_size_blocks  - I/O size to fill with in blocks.
 * 
 * @return fbe_status_t
 *
 * @author
 *  6/26/2012 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t pippin_setup_rdgen_test_context(fbe_api_rdgen_context_t *context_p,
                                                 fbe_object_id_t object_id,
                                                 fbe_class_id_t class_id,
                                                 fbe_rdgen_operation_t rdgen_operation,
                                                 fbe_lba_t capacity,
                                                 fbe_u32_t max_passes,
                                                 fbe_u32_t threads,
                                                 fbe_u32_t io_size_blocks)
{
    fbe_status_t status;
    status = fbe_api_rdgen_test_context_init(context_p, 
                                             object_id, 
                                             class_id,
                                             FBE_PACKAGE_ID_SEP_0,
                                             rdgen_operation,
                                             FBE_RDGEN_PATTERN_LBA_PASS,
                                             max_passes,
                                             0, /* io count not used */
                                             0, /* time not used*/
                                             threads,
                                             FBE_RDGEN_LBA_SPEC_RANDOM,
                                             0,    /* Start lba*/
                                             0,    /* Min lba */
                                             capacity, /* max lba */
                                             FBE_RDGEN_BLOCK_SPEC_RANDOM,
                                             1,    /* Min blocks */
                                             io_size_blocks  /* Max blocks */ );
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    return FBE_STATUS_OK;
}
/******************************************
 * end pippin_setup_rdgen_test_context()
 ******************************************/
/*!**************************************************************
 * pippin_unexpected_errors_disabled_test()
 ****************************************************************
 * @brief
 *  - Disable handling of unexpected errors.
 *  - Run I/Os with unexpected error injection.
 *  - Ensure that the LUNs do not fail.
 *
 * @param rg_config_p - Config array to create.  This is null terminated.
 *
 * @return None.
 *
 * @author
 *  6/28/2012 - Created. Rob Foley
 *
 ****************************************************************/

void pippin_unexpected_errors_disabled_test(fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_api_rdgen_context_t * context_p = &pippin_rdgen_contexts[0];
    fbe_status_t status;
    fbe_rdgen_operation_t rdgen_opcode = FBE_RDGEN_OPERATION_READ_ONLY;
    fbe_u32_t raid_group_count = fbe_test_get_rg_array_length(rg_config_p);
    fbe_test_rg_configuration_t *current_rg_config_p = NULL;
    fbe_object_id_t rg_object_id;
    fbe_object_id_t lun_object_id;
    fbe_u32_t rg_index;
    fbe_u32_t lun_index;
    fbe_lifecycle_state_t current_state;

    mut_printf(MUT_LOG_TEST_STATUS, "== %s starting ==", __FUNCTION__);

    status = fbe_api_clear_event_log(FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    fbe_test_sep_util_reduce_sector_trace_level();

	fbe_test_sep_util_disable_trace_limits();

    /* Setup the rdgen context with more than enough errors to fail the lun.
     */
    status = pippin_setup_rdgen_test_context(context_p,
                                             FBE_OBJECT_ID_INVALID,
                                             FBE_CLASS_ID_LUN,
                                             rdgen_opcode,
                                             FBE_LBA_INVALID,    /* use capacity */
                                             500,  /* limit the passes */
                                             5,   /* threads */
                                             PIPPIN_MAX_IO_BLOCKS);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    /* We need this option since the LUN will occasionally 
     * transition its edge as it gets unexpected errors. 
     */
    status = fbe_api_rdgen_io_specification_set_options(&context_p->start_io.specification, FBE_RDGEN_OPTIONS_CONTINUE_ON_ALL_ERRORS);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Disable fail on error for all LUNs.
     */
    current_rg_config_p = rg_config_p;
    for (rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        for ( lun_index = 0; lun_index < PIPPIN_TEST_LUNS_PER_RAID_GROUP; lun_index++)
        {
            status = fbe_api_database_lookup_lun_by_number(current_rg_config_p->logical_unit_configuration_list[lun_index].lun_number,
                                                           &lun_object_id);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

            status = fbe_api_get_object_lifecycle_state(lun_object_id, &current_state, FBE_PACKAGE_ID_SEP_0);
            MUT_ASSERT_INT_EQUAL(current_state, FBE_LIFECYCLE_STATE_READY);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status); 

            status = fbe_api_lun_disable_unexpected_errors(lun_object_id);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        }

        status = fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        status = fbe_api_get_object_lifecycle_state(rg_object_id, &current_state, FBE_PACKAGE_ID_SEP_0);
        MUT_ASSERT_INT_EQUAL(current_state, FBE_LIFECYCLE_STATE_READY);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        current_rg_config_p++;
    }

    /* Enable random error-testing path which will consistently give us back errors. 
     */
    fbe_api_raid_group_enable_raid_lib_random_errors(100, FBE_TRUE /* user RGs only */);

    /* Run rdgen operation(s).
     */
    status = fbe_api_rdgen_test_context_run_all_tests(context_p, FBE_PACKAGE_ID_NEIT, 1);
    MUT_ASSERT_INT_NOT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_NOT_EQUAL(context_p->start_io.statistics.invalid_request_err_count, 0);
    MUT_ASSERT_INT_EQUAL(context_p->start_io.status.block_status, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID_REQUEST);
    MUT_ASSERT_INT_EQUAL(context_p->start_io.status.block_qualifier, FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_UNEXPECTED_ERROR);

    /* Reset error-testing stats which will clean all logs as well as 
     * disable error-testing.
     */
    fbe_api_raid_group_reset_raid_lib_error_testing_stats();
    fbe_test_sep_util_restore_sector_trace_level();

    /* Make sure none of the LUNs failed.
     */
    current_rg_config_p = rg_config_p;
    for (rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        for ( lun_index = 0; lun_index < PIPPIN_TEST_LUNS_PER_RAID_GROUP; lun_index++)
        {
            status = fbe_api_database_lookup_lun_by_number(current_rg_config_p->logical_unit_configuration_list[lun_index].lun_number,
                                                           &lun_object_id);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            status = fbe_api_get_object_lifecycle_state(lun_object_id, &current_state, FBE_PACKAGE_ID_SEP_0);
            MUT_ASSERT_INT_EQUAL(current_state, FBE_LIFECYCLE_STATE_READY);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

            /* Should be no messages of this type.
             */
            pippin_check_event_log_msg_not_present(lun_object_id, current_rg_config_p->logical_unit_configuration_list[lun_index].lun_number);

            /* Reset the unexpected error handling to enabled.
             */
            status = fbe_api_lun_enable_unexpected_errors(lun_object_id);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        }

        status = fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        status = fbe_api_get_object_lifecycle_state(rg_object_id, &current_state, FBE_PACKAGE_ID_SEP_0);
        MUT_ASSERT_INT_EQUAL(current_state, FBE_LIFECYCLE_STATE_READY);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        current_rg_config_p++;
    }
    mut_printf(MUT_LOG_TEST_STATUS, "== %s complete ==", __FUNCTION__);
    return;
}
/**************************************
 * end pippin_unexpected_errors_disabled_test()
 **************************************/
/*!**************************************************************
 * pippin_fail_luns_with_unexpected_errors_test()
 ****************************************************************
 * @brief
 *  - Issue I/O with errors injected until LUN fails.
 *  - Clear the condition on every LUN and make sure they enable.
 *
 * @param rg_config_p - Config array to create.  This is null terminated.
 *
 * @return None.
 *
 * @author
 *  6/28/2012 - Created. Rob Foley
 *
 ****************************************************************/

void pippin_fail_luns_with_unexpected_errors_test(fbe_test_rg_configuration_t *rg_config_p,
                                                  fbe_rdgen_operation_t rdgen_opcode,
                                                  fbe_bool_t b_non_cached)
{
    fbe_api_rdgen_context_t * context_p = &pippin_rdgen_contexts[0];
    fbe_status_t status;
    fbe_u32_t raid_group_count = fbe_test_get_rg_array_length(rg_config_p);
    fbe_test_rg_configuration_t *current_rg_config_p = NULL;
    fbe_object_id_t rg_object_id;
    fbe_object_id_t lun_object_id;
    fbe_u32_t rg_index;
    fbe_u32_t lun_index;
    fbe_rdgen_options_t options = FBE_RDGEN_OPTIONS_CONTINUE_ON_ALL_ERRORS;

    mut_printf(MUT_LOG_TEST_STATUS, "== %s starting opcode: %d %s==", __FUNCTION__, rdgen_opcode, b_non_cached ? "non-cached" : "cached");

    status = fbe_api_clear_event_log(FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    fbe_test_sep_util_reduce_sector_trace_level();

	fbe_test_sep_util_disable_trace_limits();

    /* Enable random error-testing path which will consistently give us back errors. 
     */
    fbe_api_raid_group_enable_raid_lib_random_errors(100, FBE_TRUE /* user RGs only */);

    /* Setup the rdgen context with more than enough errors to fail the lun.
     */
    status = pippin_setup_rdgen_test_context(context_p,
                                             FBE_OBJECT_ID_INVALID,
                                             FBE_CLASS_ID_LUN,
                                             rdgen_opcode,
                                             FBE_LBA_INVALID,    /* use capacity */
                                             0,   /* run forever */
                                             5,   /* threads */
                                             PIPPIN_MAX_IO_BLOCKS);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    if (b_non_cached)
    {
        options |= FBE_RDGEN_OPTIONS_NON_CACHED;
    }
    /* We need this option since the LUN will occasionally 
     * transition its edge as it gets unexpected errors. 
     */
    status = fbe_api_rdgen_io_specification_set_options(&context_p->start_io.specification, options);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Start rdgen operation(s).
     */
    status = fbe_api_rdgen_start_tests(context_p, FBE_PACKAGE_ID_NEIT, 1);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Wait for each LUN to fail.  Also clear the errors so that the LUN goes ready.
     */
    current_rg_config_p = rg_config_p;
    for (rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        for ( lun_index = 0; lun_index < PIPPIN_TEST_LUNS_PER_RAID_GROUP; lun_index++)
        {
            status = fbe_api_database_lookup_lun_by_number(current_rg_config_p->logical_unit_configuration_list[lun_index].lun_number,
                                                           &lun_object_id);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            status = fbe_api_wait_for_object_lifecycle_state(lun_object_id, FBE_LIFECYCLE_STATE_FAIL,
                                                             FBE_TEST_WAIT_TIMEOUT_MS, FBE_PACKAGE_ID_SEP_0);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        }

        status = fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        status = fbe_api_wait_for_object_lifecycle_state(rg_object_id, FBE_LIFECYCLE_STATE_READY,
                                                         FBE_TEST_WAIT_TIMEOUT_MS, FBE_PACKAGE_ID_SEP_0);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        current_rg_config_p++;
    }
    status = fbe_api_rdgen_stop_tests(context_p, 1);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_NOT_EQUAL(context_p->start_io.statistics.invalid_request_err_count, 0);
    MUT_ASSERT_INT_EQUAL(context_p->start_io.status.block_status, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID_REQUEST);
    MUT_ASSERT_INT_EQUAL(context_p->start_io.status.block_qualifier, FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_UNEXPECTED_ERROR);

    /* Reset error-testing stats which will clean all logs as well as 
     * disable error-testing.
     */
    fbe_api_raid_group_reset_raid_lib_error_testing_stats();
    fbe_test_sep_util_restore_sector_trace_level();

    /* Wait for each LUN to fail.  Also clear the errors so that the LUN goes ready.
     */
    current_rg_config_p = rg_config_p;
    for (rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        for ( lun_index = 0; lun_index < PIPPIN_TEST_LUNS_PER_RAID_GROUP; lun_index++)
        {
            status = fbe_api_database_lookup_lun_by_number(current_rg_config_p->logical_unit_configuration_list[lun_index].lun_number,
                                                           &lun_object_id);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            status = fbe_api_wait_for_object_lifecycle_state(lun_object_id, FBE_LIFECYCLE_STATE_FAIL,
                                                             FBE_TEST_WAIT_TIMEOUT_MS, FBE_PACKAGE_ID_SEP_0);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

            pippin_check_event_log_msg_present(lun_object_id, current_rg_config_p->logical_unit_configuration_list[lun_index].lun_number);

            /* For some LUNs we will clear the errors.  For other LUNs we will disable unexpected error handling. 
             * Both of these should definately allow the LUN to come out of FAIL. 
             */
            if (lun_index % 2)
            {
                status = fbe_api_lun_clear_unexpected_errors(lun_object_id);
            }
            else
            {
                status = fbe_api_lun_disable_unexpected_errors(lun_object_id);
            }
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        }

        status = fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        status = fbe_api_wait_for_object_lifecycle_state(rg_object_id, FBE_LIFECYCLE_STATE_READY,
                                                         FBE_TEST_WAIT_TIMEOUT_MS, FBE_PACKAGE_ID_SEP_0);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        current_rg_config_p++;
    }
    /* Wait for LUN to go ready.
     */
    current_rg_config_p = rg_config_p;
    for (rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        for ( lun_index = 0; lun_index < PIPPIN_TEST_LUNS_PER_RAID_GROUP; lun_index++)
        {
            status = fbe_api_database_lookup_lun_by_number(current_rg_config_p->logical_unit_configuration_list[lun_index].lun_number,
                                                           &lun_object_id);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            status = fbe_api_wait_for_object_lifecycle_state(lun_object_id, FBE_LIFECYCLE_STATE_READY,
                                                             FBE_TEST_WAIT_TIMEOUT_MS, FBE_PACKAGE_ID_SEP_0);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

            status = fbe_api_lun_clear_unexpected_errors(lun_object_id);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            /* Set this back to default, enabled.
             */
            status = fbe_api_lun_enable_unexpected_errors(lun_object_id);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        }
        current_rg_config_p++;
    }
    abby_cadabby_wait_for_verifies(rg_config_p, raid_group_count);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s complete ==", __FUNCTION__);
    return;
}
/**************************************
 * end pippin_fail_luns_with_unexpected_errors_test()
 **************************************/


void pippin_enable_injection(void)
{
    fbe_status_t status;

    /* We need to control the SP's behavior here so just trace when the critical error hits.
     */
    status = fbe_api_trace_set_error_limit(FBE_TRACE_LEVEL_CRITICAL_ERROR,
                                           FBE_API_TRACE_ERROR_LIMIT_ACTION_TRACE,
                                           1,    /* Num errors. */
                                           FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = fbe_api_clear_event_log(FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    fbe_test_sep_util_reduce_sector_trace_level();

	fbe_test_sep_util_disable_trace_limits();

    /* Enable random error-testing path which will consistently give us back errors. 
     */
    fbe_api_raid_group_enable_raid_lib_random_errors(100, FBE_TRUE /* user RGs only */);

}
void pippin_start_io(fbe_api_rdgen_context_t * context_p,
                     fbe_rdgen_operation_t rdgen_opcode,
                     fbe_bool_t b_non_cached)
{
    fbe_status_t status;
    fbe_rdgen_options_t options = FBE_RDGEN_OPTIONS_CONTINUE_ON_ALL_ERRORS;

    /* Setup the rdgen context with more than enough errors to fail the lun.
     */
    status = pippin_setup_rdgen_test_context(context_p,
                                             FBE_OBJECT_ID_INVALID,
                                             FBE_CLASS_ID_LUN,
                                             rdgen_opcode,
                                             FBE_LBA_INVALID,    /* use capacity */
                                             0,   /* run forever */
                                             5,   /* threads */
                                             PIPPIN_MAX_IO_BLOCKS);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    if (b_non_cached)
    {
        options |= FBE_RDGEN_OPTIONS_NON_CACHED;
    }
    /* We need this option since the LUN will occasionally 
     * transition its edge as it gets unexpected errors. 
     */
    status = fbe_api_rdgen_io_specification_set_options(&context_p->start_io.specification, options);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Start rdgen operation(s).
     */
    status = fbe_api_rdgen_start_tests(context_p, FBE_PACKAGE_ID_NEIT, 1);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
}
void pippin_start_object_io(fbe_api_rdgen_context_t * context_p,
                            fbe_object_id_t object_id,
                            fbe_rdgen_operation_t rdgen_opcode,
                            fbe_bool_t b_non_cached)
{
    fbe_status_t status;
    fbe_rdgen_options_t options = FBE_RDGEN_OPTIONS_CONTINUE_ON_ALL_ERRORS;

    /* Setup the rdgen context with more than enough errors to fail the lun.
     */
    status = pippin_setup_rdgen_test_context(context_p,
                                             object_id,
                                             FBE_CLASS_ID_INVALID,
                                             rdgen_opcode,
                                             FBE_LBA_INVALID,    /* use capacity */
                                             0,   /* run forever */
                                             5,   /* threads */
                                             PIPPIN_MAX_IO_BLOCKS);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    if (b_non_cached)
    {
        options |= FBE_RDGEN_OPTIONS_NON_CACHED;
    }
    /* We need this option since the LUN will occasionally 
     * transition its edge as it gets unexpected errors. 
     */
    status = fbe_api_rdgen_io_specification_set_options(&context_p->start_io.specification, options);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Start rdgen operation(s).
     */
    status = fbe_api_rdgen_start_tests(context_p, FBE_PACKAGE_ID_NEIT, 1);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
}

/*!**************************************************************
 * pippin_unexpected_errs_both_sps()
 ****************************************************************
 * @brief
 *  This test will confirm the case where both SPs get software errors
 *  at the same time on the same LUNs.
 *  - In this case we expect that the two LUN objects negotiate and the active SP
 *  is the one that actually panics.
 *  - The passive SPs LUNs will transition to FAIL once the other SP goes away.
 *  - Once the panicked SP reboots, the SP comes up and causes the LUNs
 *    on the other SP to come out of fail and clear their errors.
 *  - The LUNs come back Ready on both SPs.
 * 
 *  The test will:
 *    - Run I/Os with error injection on both SPs.
 *    - Use a scheduler hook inside the object to cause both LUNs to
 *      have errors at the point where they negotiate on who will panic.
 *    - This results in the active SP panicking since it has errors.
 *    - The test waits for the active SP to have critical errors.
 *    - Then we crash the active SP to simulate the crash.
 *    - Next, we make sure the passive SP's LUNs have failed.
 *    - Next we boot up the panicked SP.
 *    - We then wait until all the LUNs have come ready on both SPs.
 *
 * @param rg_config_p - Config array to create.  This is null terminated.
 *
 * @return None.
 *
 * @author
 *  9/26/2012 - Created. Rob Foley
 *
 ****************************************************************/

void pippin_unexpected_errs_both_sps(fbe_test_rg_configuration_t *rg_config_p,
                                                  fbe_rdgen_operation_t rdgen_opcode,
                                                  fbe_bool_t b_non_cached)
{
    fbe_api_rdgen_context_t * context_p = &pippin_rdgen_contexts[0];
    fbe_api_rdgen_context_t * context1_p = &pippin_rdgen_contexts[1];
    fbe_status_t status;
    fbe_u32_t raid_group_count = fbe_test_get_rg_array_length(rg_config_p);
    fbe_test_rg_configuration_t *current_rg_config_p = NULL;
    fbe_object_id_t lun_object_id;
    fbe_u32_t rg_index;
    fbe_u32_t lun_index;
    fbe_sim_transport_connection_target_t peer_sp;
    fbe_sim_transport_connection_target_t first_io_sp = FBE_SIM_SP_A;
    fbe_sim_transport_connection_target_t second_io_sp = FBE_SIM_SP_B;
    fbe_sim_transport_connection_target_t active_sp;
    fbe_api_trace_error_counters_t error_counters;

    fbe_test_sep_get_active_target_id(&active_sp);

    peer_sp = fbe_test_sep_get_peer_target_id(active_sp);

    mut_printf(MUT_LOG_TEST_STATUS, "== %s starting opcode: %d %s==", 
               __FUNCTION__, rdgen_opcode, b_non_cached ? "non-cached" : "cached");

    /* Decide which SP to start I/O on first.
     */
    if (fbe_random() % 2)
    {
        first_io_sp = FBE_SIM_SP_B;
        second_io_sp = FBE_SIM_SP_A;
    }
    /* Set debug hook so that the LUNs will see that the threshold 
     * was reached for both SPs. 
     */
    current_rg_config_p = rg_config_p;
    for (rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        for ( lun_index = 0; lun_index < PIPPIN_TEST_LUNS_PER_RAID_GROUP; lun_index++)
        {
            status = fbe_api_database_lookup_lun_by_number(current_rg_config_p->logical_unit_configuration_list[lun_index].lun_number,
                                                           &lun_object_id);
            fbe_api_sim_transport_set_target_server(active_sp);
            status = fbe_api_scheduler_add_debug_hook(lun_object_id,
                                                      SCHEDULER_MONITOR_STATE_LUN_EVAL_ERR,
                                                      FBE_LUN_SUBSTATE_EVAL_ERR_ENTRY,
                                                      0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            fbe_api_sim_transport_set_target_server(peer_sp);
            status = fbe_api_scheduler_add_debug_hook(lun_object_id,
                                                      SCHEDULER_MONITOR_STATE_LUN_EVAL_ERR,
                                                      FBE_LUN_SUBSTATE_EVAL_ERR_ENTRY,
                                                      0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        }
        current_rg_config_p++;
    }

    fbe_api_sim_transport_set_target_server(first_io_sp);
    pippin_enable_injection();
    pippin_start_io(context_p, rdgen_opcode, b_non_cached);
    fbe_api_sim_transport_set_target_server(second_io_sp);
    pippin_enable_injection();
    pippin_start_io(context1_p, rdgen_opcode, b_non_cached);

    /* Validate that we hit the hook on both SPs.
     */
    current_rg_config_p = rg_config_p;
    for (rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        for ( lun_index = 0; lun_index < PIPPIN_TEST_LUNS_PER_RAID_GROUP; lun_index++)
        {
            status = fbe_api_database_lookup_lun_by_number(current_rg_config_p->logical_unit_configuration_list[lun_index].lun_number,
                                                           &lun_object_id);
            fbe_api_sim_transport_set_target_server(active_sp);
            status = fbe_test_wait_for_debug_hook(lun_object_id,
                                                 SCHEDULER_MONITOR_STATE_LUN_EVAL_ERR,
                                                 FBE_LUN_SUBSTATE_EVAL_ERR_ENTRY,
                                                 SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE, 0,0);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

            fbe_api_sim_transport_set_target_server(peer_sp);
            status = fbe_test_wait_for_debug_hook(lun_object_id,
                                                 SCHEDULER_MONITOR_STATE_LUN_EVAL_ERR,
                                                 FBE_LUN_SUBSTATE_EVAL_ERR_ENTRY,
                                                 SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE, 0,0);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        }
        current_rg_config_p++;
    }

    /* We hit the errors, stop I/Os and error injection.
     */
    fbe_api_sim_transport_set_target_server(first_io_sp);
    status = fbe_api_rdgen_stop_tests(context_p, 1);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_NOT_EQUAL(context_p->start_io.statistics.invalid_request_err_count, 0);
    MUT_ASSERT_INT_EQUAL(context_p->start_io.status.block_status, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID_REQUEST);
    MUT_ASSERT_INT_EQUAL(context_p->start_io.status.block_qualifier, FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_UNEXPECTED_ERROR);
    fbe_api_raid_group_reset_raid_lib_error_testing_stats();
    fbe_test_sep_util_restore_sector_trace_level();

    fbe_api_sim_transport_set_target_server(second_io_sp);
    status = fbe_api_rdgen_stop_tests(context1_p, 1);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_NOT_EQUAL(context1_p->start_io.statistics.invalid_request_err_count, 0);
    MUT_ASSERT_INT_EQUAL(context1_p->start_io.status.block_status, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID_REQUEST);
    MUT_ASSERT_INT_EQUAL(context1_p->start_io.status.block_qualifier, FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_UNEXPECTED_ERROR);
    fbe_api_raid_group_reset_raid_lib_error_testing_stats();
    fbe_test_sep_util_restore_sector_trace_level();

    /* Remove hook to allow both LUNs to proceed.
     */
    current_rg_config_p = rg_config_p;
    for (rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        for ( lun_index = 0; lun_index < PIPPIN_TEST_LUNS_PER_RAID_GROUP; lun_index++)
        {
            status = fbe_api_database_lookup_lun_by_number(current_rg_config_p->logical_unit_configuration_list[lun_index].lun_number,
                                                           &lun_object_id);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            status = fbe_api_sim_transport_set_target_server(active_sp);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            status = fbe_api_scheduler_del_debug_hook(lun_object_id,
                                                      SCHEDULER_MONITOR_STATE_LUN_EVAL_ERR,
                                                      FBE_LUN_SUBSTATE_EVAL_ERR_ENTRY,
                                                      0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
            status = fbe_api_sim_transport_set_target_server(peer_sp);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            status = fbe_api_scheduler_del_debug_hook(lun_object_id,
                                                      SCHEDULER_MONITOR_STATE_LUN_EVAL_ERR,
                                                      FBE_LUN_SUBSTATE_EVAL_ERR_ENTRY,
                                                      0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        }
        current_rg_config_p++;
    }


    /* Validate that the peer SP LUNs have not failed yet. They should wait for peer to die.
     */
    current_rg_config_p = rg_config_p;
    for (rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        for ( lun_index = 0; lun_index < PIPPIN_TEST_LUNS_PER_RAID_GROUP; lun_index++)
        {
            status = fbe_api_database_lookup_lun_by_number(current_rg_config_p->logical_unit_configuration_list[lun_index].lun_number,
                                                           &lun_object_id);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            status = fbe_api_wait_for_object_lifecycle_state(lun_object_id, FBE_LIFECYCLE_STATE_READY,
                                                             FBE_TEST_WAIT_TIMEOUT_MS, FBE_PACKAGE_ID_SEP_0);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        }
        current_rg_config_p++;
    }

    /* Since the LUNs will proceed the active SP only will hit the critical error
     * since both SPs saw the software errors and thus only the active SP will 
     * critical error and the passive SP will fail. 
     */
    fbe_api_sim_transport_set_target_server(active_sp);
    fbe_test_wait_for_critical_error();
    status = fbe_api_trace_get_error_counter(&error_counters, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "failed to get trace error counters from Package!");
    MUT_ASSERT_INT_NOT_EQUAL(error_counters.trace_error_counter, 0);

    status = fbe_api_sim_transport_set_target_server(peer_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = fbe_api_trace_get_error_counter(&error_counters, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "failed to get trace error counters from Package!");
    MUT_ASSERT_INT_EQUAL(error_counters.trace_critical_error_counter, 0);
    MUT_ASSERT_INT_NOT_EQUAL(error_counters.trace_error_counter, 0);

    /* Crash the active SP manually.
     */
    mut_printf(MUT_LOG_LOW, " == Shutdown target SP (%s) == ", active_sp == FBE_SIM_SP_A ? "SP A" : "SP B");
    fbe_api_sim_transport_destroy_client(active_sp);
    fbe_test_sp_sim_stop_single_sp(active_sp == FBE_SIM_SP_A ? FBE_TEST_SPA : FBE_TEST_SPB);

    /* Validate that the peer SP LUNs have failed since it too saw errors.
     */
    status = fbe_api_sim_transport_set_target_server(peer_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    current_rg_config_p = rg_config_p;
    for (rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        for ( lun_index = 0; lun_index < PIPPIN_TEST_LUNS_PER_RAID_GROUP; lun_index++)
        {
            status = fbe_api_database_lookup_lun_by_number(current_rg_config_p->logical_unit_configuration_list[lun_index].lun_number,
                                                           &lun_object_id);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            status = fbe_api_wait_for_object_lifecycle_state(lun_object_id, FBE_LIFECYCLE_STATE_FAIL,
                                                             FBE_TEST_WAIT_TIMEOUT_MS, FBE_PACKAGE_ID_SEP_0);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        }
        current_rg_config_p++;
    }

    /* Now we will bring up the original SP and make sure all the LUNs come ready on both SPs.
     */
    status = fbe_api_sim_transport_set_target_server(active_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_LOW, " == Startup target SP (%s) == ", active_sp == FBE_SIM_SP_A ? "SP A" : "SP B");
    fbe_test_boot_sp(rg_config_p, active_sp);

    /* Disable backtrace to avoid filling the logs.
     */
    status = fbe_api_trace_disable_backtrace(FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = fbe_api_trace_disable_backtrace(FBE_PACKAGE_ID_NEIT);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    current_rg_config_p = rg_config_p;
    for (rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        for ( lun_index = 0; lun_index < PIPPIN_TEST_LUNS_PER_RAID_GROUP; lun_index++)
        {
            status = fbe_api_database_lookup_lun_by_number(current_rg_config_p->logical_unit_configuration_list[lun_index].lun_number,
                                                           &lun_object_id);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

            status = fbe_api_sim_transport_set_target_server(peer_sp);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            status = fbe_api_wait_for_object_lifecycle_state(lun_object_id, FBE_LIFECYCLE_STATE_READY,
                                                             FBE_TEST_WAIT_TIMEOUT_MS, FBE_PACKAGE_ID_SEP_0);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

            status = fbe_api_sim_transport_set_target_server(active_sp);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            status = fbe_api_wait_for_object_lifecycle_state(lun_object_id, FBE_LIFECYCLE_STATE_READY,
                                                             FBE_TEST_WAIT_TIMEOUT_MS, FBE_PACKAGE_ID_SEP_0);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        }
        current_rg_config_p++;
    }

    status = fbe_api_sim_transport_set_target_server(active_sp);
    abby_cadabby_wait_for_verifies(rg_config_p, raid_group_count);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s complete ==", __FUNCTION__);
    status = fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    return;
}
/**************************************
 * end pippin_unexpected_errs_both_sps()
 **************************************/

/*!**************************************************************
 * pippin_unexpected_errs_alternate_sps()
 ****************************************************************
 * @brief
 *  This test will confirm the case where both SPs get software errors
 *  at the same time on different LUNs.
 *  - In this case we expect that the active SP will grant only one
 *    SP's request to reboot.
 *  - The SP that gets permission will critical error.
 *  - We then reboot that SP.
 *  - The LUN objects on the peer that had software errors will transition
 *  to FAIL once the other SP goes away.
 *  - Once the panicked SP reboots, the SP comes up and causes the LUNs
 *  on the other SP to come out of fail and clear their errors.
 *  - The LUNs come back Ready on both SPs.
 * 
 *  The test will:
 *    - Run I/Os with error injection on alternate LUNs.
 *    - A given LUN will only get software errors on one of the SPs
 *    - Use a scheduler hook inside the object to cause both LUNs to wait
 *    at the point where they negotiate on who will panic.
 *    - We clear the debug hook on one LUN either on the active or passive side
 *      that had errors.  We expect that only this SP gets critical errors.
 *    - Then we crash the SP that we allowed to get critical errors.
 *    - Next, we make sure the passive SP's LUNs with errors have failed.
 *    - Next we boot up the panicked SP.
 *    - We then wait until all the LUNs have come ready on both SPs.
 *
 * @param rg_config_p - Config array to create.  This is null terminated.
 *
 * @return None.
 *
 * @author
 *  9/26/2012 - Created. Rob Foley
 *
 ****************************************************************/

void pippin_unexpected_errs_alternate_sps(fbe_test_rg_configuration_t *rg_config_p,
                                          fbe_rdgen_operation_t rdgen_opcode,
                                          fbe_bool_t b_non_cached,
                                          fbe_bool_t b_active_panics)
{
    fbe_api_rdgen_context_t * context_p = &pippin_rdgen_contexts[0];
    fbe_u32_t context_index;
    fbe_status_t status;
    fbe_u32_t raid_group_count = fbe_test_get_rg_array_length(rg_config_p);
    fbe_test_rg_configuration_t *current_rg_config_p = NULL;
    fbe_object_id_t lun_object_id;
    fbe_u32_t rg_index;
    fbe_u32_t lun_index;
    fbe_sim_transport_connection_target_t first_io_sp;
    fbe_sim_transport_connection_target_t second_io_sp = FBE_SIM_SP_B;
    fbe_api_trace_error_counters_t error_counters;
    fbe_sim_transport_connection_target_t peer_sp;
    fbe_sim_transport_connection_target_t active_sp;
    fbe_u32_t first_substate, second_substate;

    fbe_test_sep_get_active_target_id(&active_sp);
    peer_sp = fbe_test_sep_get_peer_target_id(active_sp);

    mut_printf(MUT_LOG_TEST_STATUS, "== %s starting opcode: %d %s==", 
               __FUNCTION__, rdgen_opcode, b_non_cached ? "non-cached" : "cached");

    if (b_active_panics)
    {
        first_io_sp = active_sp;
        second_io_sp = peer_sp;
        first_substate = FBE_LUN_SUBSTATE_EVAL_ERR_ACTIVE_GRANTED;
        second_substate = FBE_LUN_SUBSTATE_EVAL_ERR_PEER_DENIED;
    }
    else
    {
        first_io_sp = peer_sp;
        second_io_sp = active_sp;

        first_substate = FBE_LUN_SUBSTATE_EVAL_ERR_PEER_GRANTED;
        second_substate = FBE_LUN_SUBSTATE_EVAL_ERR_ACTIVE_DENIED;
    }

    /* Set debug hook so that the LUNs will see that the threshold 
     * was reached for both SPs. 
     */
    current_rg_config_p = rg_config_p;
    for (rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        status = fbe_api_database_lookup_lun_by_number(current_rg_config_p->logical_unit_configuration_list[0].lun_number,
                                                       &lun_object_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        fbe_api_sim_transport_set_target_server(first_io_sp);
        mut_printf(MUT_LOG_TEST_STATUS, "== add hook for lun: %d obj: 0x%x on %s",
                   current_rg_config_p->logical_unit_configuration_list[0].lun_number,
                   lun_object_id,
                   first_io_sp == FBE_SIM_SP_A ? "SP A" : "SP B");
        status = fbe_api_scheduler_add_debug_hook(lun_object_id,
                                                  SCHEDULER_MONITOR_STATE_LUN_EVAL_ERR,
                                                  FBE_LUN_SUBSTATE_EVAL_ERR_ENTRY,
                                                  0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        fbe_api_sim_transport_set_target_server(second_io_sp);
        status = fbe_api_database_lookup_lun_by_number(current_rg_config_p->logical_unit_configuration_list[1].lun_number,
                                                       &lun_object_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        mut_printf(MUT_LOG_TEST_STATUS, "== add hook for lun: %d obj: 0x%x on %s",
                   current_rg_config_p->logical_unit_configuration_list[1].lun_number,
                   lun_object_id,
                   second_io_sp == FBE_SIM_SP_A ? "SP A" : "SP B");
        status = fbe_api_scheduler_add_debug_hook(lun_object_id,
                                                  SCHEDULER_MONITOR_STATE_LUN_EVAL_ERR,
                                                  FBE_LUN_SUBSTATE_EVAL_ERR_ENTRY,
                                                  0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        current_rg_config_p++;
    }

    /* On the active side, we add hooks for luns that will be granted and LUNs that will be denied.
     */
    status = fbe_api_sim_transport_set_target_server(active_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    current_rg_config_p = rg_config_p;
    for (rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        status = fbe_api_database_lookup_lun_by_number(current_rg_config_p->logical_unit_configuration_list[0].lun_number,
                                                       &lun_object_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        mut_printf(MUT_LOG_TEST_STATUS, "== add hook for lun: %d obj: 0x%x on %s",
                   current_rg_config_p->logical_unit_configuration_list[0].lun_number,
                   lun_object_id,
                   first_io_sp == FBE_SIM_SP_A ? "SP A" : "SP B");
        status = fbe_api_scheduler_add_debug_hook(lun_object_id,
                                                  SCHEDULER_MONITOR_STATE_LUN_EVAL_ERR,
                                                  first_substate,
                                                  0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_LOG);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        status = fbe_api_database_lookup_lun_by_number(current_rg_config_p->logical_unit_configuration_list[1].lun_number,
                                                       &lun_object_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        mut_printf(MUT_LOG_TEST_STATUS, "== add hook for lun: %d obj: 0x%x on %s",
                   current_rg_config_p->logical_unit_configuration_list[1].lun_number,
                   lun_object_id,
                   second_io_sp == FBE_SIM_SP_A ? "SP A" : "SP B");
        status = fbe_api_scheduler_add_debug_hook(lun_object_id,
                                                  SCHEDULER_MONITOR_STATE_LUN_EVAL_ERR, second_substate,
                                                  0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_LOG);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        current_rg_config_p++;
    }
    fbe_api_sim_transport_set_target_server(first_io_sp);
    pippin_enable_injection();
    
    fbe_api_sim_transport_set_target_server(second_io_sp);
    pippin_enable_injection();
    
    /* Start I/O on all LUNs.
     */
    current_rg_config_p = rg_config_p;
    for (rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        status = fbe_api_database_lookup_lun_by_number(current_rg_config_p->logical_unit_configuration_list[0].lun_number,
                                                       &lun_object_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        context_index = pippin_rdgen_context_index(rg_index, 0);
        fbe_api_sim_transport_set_target_server(first_io_sp);
        pippin_start_object_io(&context_p[context_index], lun_object_id, rdgen_opcode, b_non_cached);
        mut_printf(MUT_LOG_TEST_STATUS, "== start I/O on for lun: %d obj: 0x%x on %s",
                   current_rg_config_p->logical_unit_configuration_list[0].lun_number,
                   lun_object_id, first_io_sp == FBE_SIM_SP_A ? "SP A" : "SP B");

        fbe_api_sim_transport_set_target_server(second_io_sp);
        status = fbe_api_database_lookup_lun_by_number(current_rg_config_p->logical_unit_configuration_list[1].lun_number,
                                                       &lun_object_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        context_index = pippin_rdgen_context_index(rg_index, 1);
        pippin_start_object_io(&context_p[context_index], lun_object_id, rdgen_opcode, b_non_cached);

        mut_printf(MUT_LOG_TEST_STATUS, "== start I/O on for lun: %d obj: 0x%x on %s",
                   current_rg_config_p->logical_unit_configuration_list[1].lun_number,
                   lun_object_id, second_io_sp == FBE_SIM_SP_A ? "SP A" : "SP B");
        current_rg_config_p++;
    }
    /* Validate that we hit the hook on both SPs.
     */
    current_rg_config_p = rg_config_p;
    for (rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        status = fbe_api_database_lookup_lun_by_number(current_rg_config_p->logical_unit_configuration_list[0].lun_number,
                                                       &lun_object_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        mut_printf(MUT_LOG_TEST_STATUS, "== wait for hook for lun: %d obj: 0x%x on %s",
                   current_rg_config_p->logical_unit_configuration_list[0].lun_number,
                   lun_object_id, first_io_sp == FBE_SIM_SP_A ? "SP A" : "SP B");
        fbe_api_sim_transport_set_target_server(first_io_sp);
        status = fbe_test_wait_for_debug_hook(lun_object_id,
                                             SCHEDULER_MONITOR_STATE_LUN_EVAL_ERR,
                                             FBE_LUN_SUBSTATE_EVAL_ERR_ENTRY,
                                             SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE, 0,0);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        context_index = pippin_rdgen_context_index(rg_index, 0);
        status = fbe_api_rdgen_stop_tests(&context_p[context_index], 1);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        MUT_ASSERT_INT_NOT_EQUAL(context_p[context_index].start_io.statistics.invalid_request_err_count, 0);
        MUT_ASSERT_INT_EQUAL(context_p[context_index].start_io.status.block_status, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID_REQUEST);
        MUT_ASSERT_INT_EQUAL(context_p[context_index].start_io.status.block_qualifier, FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_UNEXPECTED_ERROR);

        status = fbe_api_database_lookup_lun_by_number(current_rg_config_p->logical_unit_configuration_list[1].lun_number,
                                                       &lun_object_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        mut_printf(MUT_LOG_TEST_STATUS, "== wait for hook for lun: %d obj: 0x%x on %s",
                   current_rg_config_p->logical_unit_configuration_list[1].lun_number,
                   lun_object_id, second_io_sp == FBE_SIM_SP_A ? "SP A" : "SP B");
        fbe_api_sim_transport_set_target_server(second_io_sp);
        status = fbe_test_wait_for_debug_hook(lun_object_id,
                                             SCHEDULER_MONITOR_STATE_LUN_EVAL_ERR,
                                             FBE_LUN_SUBSTATE_EVAL_ERR_ENTRY,
                                             SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE, 0,0);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        context_index = pippin_rdgen_context_index(rg_index, 1);
        status = fbe_api_rdgen_stop_tests(&context_p[context_index], 1);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        MUT_ASSERT_INT_NOT_EQUAL(context_p[context_index].start_io.statistics.invalid_request_err_count, 0);
        MUT_ASSERT_INT_EQUAL(context_p[context_index].start_io.status.block_status, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID_REQUEST);
        MUT_ASSERT_INT_EQUAL(context_p[context_index].start_io.status.block_qualifier, FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_UNEXPECTED_ERROR);

        current_rg_config_p++;
    }

    fbe_api_sim_transport_set_target_server(second_io_sp);
    
    pippin_start_object_io(&context_p[0], FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PSM, 
                           FBE_RDGEN_OPERATION_WRITE_ONLY, FBE_FALSE);
    fbe_api_sleep(2000);
    status = fbe_api_rdgen_stop_tests(&context_p[0], 1);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* We hit the errors, stop error injection.
     */
    fbe_api_sim_transport_set_target_server(first_io_sp);
    fbe_api_raid_group_reset_raid_lib_error_testing_stats();
    fbe_test_sep_util_restore_sector_trace_level();

    fbe_api_sim_transport_set_target_server(second_io_sp);
    fbe_api_raid_group_reset_raid_lib_error_testing_stats();
    fbe_test_sep_util_restore_sector_trace_level();

    /* Remove hook first on one SP.
     * These LUNs will be allowed to hit a critical error. 
     */
    status = fbe_api_sim_transport_set_target_server(first_io_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_TEST_STATUS, "== remove debug hooks for LUNs on %s",
               first_io_sp == FBE_SIM_SP_A ? "SP A" : "SP B");
    current_rg_config_p = rg_config_p;
    for (rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        status = fbe_api_database_lookup_lun_by_number(current_rg_config_p->logical_unit_configuration_list[0].lun_number,
                                                       &lun_object_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        status = fbe_api_scheduler_del_debug_hook(lun_object_id,
                                                  SCHEDULER_MONITOR_STATE_LUN_EVAL_ERR,
                                                  FBE_LUN_SUBSTATE_EVAL_ERR_ENTRY,
                                                  0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        current_rg_config_p++;
    }

    /* Make sure that on the active SP we hit the hooks for the LUNs that caused critical error.
     */
    status = fbe_api_sim_transport_set_target_server(active_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    
    current_rg_config_p = rg_config_p;
    for (rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        status = fbe_api_database_lookup_lun_by_number(current_rg_config_p->logical_unit_configuration_list[0].lun_number,
                                                       &lun_object_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        status = fbe_test_wait_for_debug_hook(lun_object_id,
                                             SCHEDULER_MONITOR_STATE_LUN_EVAL_ERR,
                                             first_substate,
                                             SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_LOG, 0,0);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        status = fbe_api_scheduler_del_debug_hook(lun_object_id,
                                                  SCHEDULER_MONITOR_STATE_LUN_EVAL_ERR,
                                                  first_substate,
                                                  0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_LOG);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        current_rg_config_p++;
    }

    /* Check that we got critical errors on this SP.
     * We allowed the LUNs we got software errors to on this SP to
     * proceed first so they should have critical errored first.
     */
    fbe_api_sim_transport_set_target_server(first_io_sp);
    fbe_test_wait_for_critical_error();
    status = fbe_api_trace_get_error_counter(&error_counters, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "failed to get trace error counters from Package!");
    MUT_ASSERT_INT_NOT_EQUAL(error_counters.trace_error_counter, 0);
    mut_printf(MUT_LOG_TEST_STATUS, "== validate error counters on %s cerr: 0x%x err: 0x%x",
               first_io_sp == FBE_SIM_SP_A ? "SP A" : "SP B", error_counters.trace_critical_error_counter,
               error_counters.trace_error_counter);

    /* Remove debug hook on the peer.  It should not be allowed to get a critical error.
     */
    status = fbe_api_sim_transport_set_target_server(second_io_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_TEST_STATUS, "== remove debug hooks for LUNs on %s",
               first_io_sp == FBE_SIM_SP_A ? "SP A" : "SP B");
    current_rg_config_p = rg_config_p;
    for (rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        status = fbe_api_database_lookup_lun_by_number(current_rg_config_p->logical_unit_configuration_list[1].lun_number,
                                                       &lun_object_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        status = fbe_api_scheduler_del_debug_hook(lun_object_id,
                                                  SCHEDULER_MONITOR_STATE_LUN_EVAL_ERR,
                                                  FBE_LUN_SUBSTATE_EVAL_ERR_ENTRY,
                                                  0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_PAUSE);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        current_rg_config_p++;
    }

    /* Make sure that on the active SP we hit the hooks for the LUNs that were denied.
     */
    status = fbe_api_sim_transport_set_target_server(active_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    current_rg_config_p = rg_config_p;
    for (rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        status = fbe_api_database_lookup_lun_by_number(current_rg_config_p->logical_unit_configuration_list[1].lun_number,
                                                       &lun_object_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        status = fbe_test_wait_for_debug_hook(lun_object_id,
                                             SCHEDULER_MONITOR_STATE_LUN_EVAL_ERR,
                                             second_substate,
                                             SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_LOG, 0,0);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        status = fbe_api_scheduler_del_debug_hook(lun_object_id,
                                                  SCHEDULER_MONITOR_STATE_LUN_EVAL_ERR,
                                                  second_substate,
                                                  0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_LOG);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        current_rg_config_p++;
    }

    status = fbe_api_sim_transport_set_target_server(second_io_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    /* Validate that the peer SP LUNs have not failed yet. They should wait for peer to die.
     */
    current_rg_config_p = rg_config_p;
    for (rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        status = fbe_api_database_lookup_lun_by_number(current_rg_config_p->logical_unit_configuration_list[1].lun_number,
                                                       &lun_object_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        status = fbe_api_wait_for_object_lifecycle_state(lun_object_id, FBE_LIFECYCLE_STATE_READY,
                                                         FBE_TEST_WAIT_TIMEOUT_MS, FBE_PACKAGE_ID_SEP_0);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        current_rg_config_p++;
    }

    /* There should be no critical errors on the second sp.  It was not allowed to proceed
     * with the negotiate a the LUN level so it should have been denied the request to panic. 
     * Thus, no critical errors should be present. 
     */
    status = fbe_api_sim_transport_set_target_server(second_io_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    status = fbe_api_trace_get_error_counter(&error_counters, FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, "failed to get trace error counters from Package!");
    MUT_ASSERT_INT_EQUAL(error_counters.trace_critical_error_counter, 0);
    MUT_ASSERT_INT_NOT_EQUAL(error_counters.trace_error_counter, 0);
    mut_printf(MUT_LOG_TEST_STATUS, "== validate error counters on %s cerr: 0x%x err: 0x%x",
               second_io_sp == FBE_SIM_SP_A ? "SP A" : "SP B", error_counters.trace_critical_error_counter,
               error_counters.trace_error_counter);

    /* Crash the active SP manually.
     */
    mut_printf(MUT_LOG_LOW, " == Shutdown target SP (%s) == ", first_io_sp == FBE_SIM_SP_A ? "SP A" : "SP B");
    fbe_api_sim_transport_destroy_client(first_io_sp);
    fbe_test_sp_sim_stop_single_sp(first_io_sp == FBE_SIM_SP_A ? FBE_TEST_SPA : FBE_TEST_SPB);

    /* Validate that the peer SP LUNs have failed since it too saw errors.
     */
    status = fbe_api_sim_transport_set_target_server(second_io_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_TEST_STATUS, "== validate LUNs have failed on %s",
               second_io_sp == FBE_SIM_SP_A ? "SP A" : "SP B");
    current_rg_config_p = rg_config_p;
    for (rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        status = fbe_api_database_lookup_lun_by_number(current_rg_config_p->logical_unit_configuration_list[1].lun_number,
                                                       &lun_object_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        status = fbe_api_wait_for_object_lifecycle_state(lun_object_id, FBE_LIFECYCLE_STATE_FAIL,
                                                         FBE_TEST_WAIT_TIMEOUT_MS, FBE_PACKAGE_ID_SEP_0);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        current_rg_config_p++;
    }

    /* Now we will bring up the original SP and make sure all the LUNs come ready on both SPs.
     */
    status = fbe_api_sim_transport_set_target_server(first_io_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_LOW, " == Startup target SP (%s) == ", first_io_sp == FBE_SIM_SP_A ? "SP A" : "SP B");
    fbe_test_boot_sp(rg_config_p, first_io_sp);

    /* Disable backtrace to avoid filling the logs.
     */
    status = fbe_api_trace_disable_backtrace(FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = fbe_api_trace_disable_backtrace(FBE_PACKAGE_ID_NEIT);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_TEST_STATUS, "== Validate all LUNs come ready on both SPs");
    current_rg_config_p = rg_config_p;
    for (rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        for ( lun_index = 0; lun_index < PIPPIN_TEST_LUNS_PER_RAID_GROUP; lun_index++)
        {
            status = fbe_api_database_lookup_lun_by_number(current_rg_config_p->logical_unit_configuration_list[lun_index].lun_number,
                                                           &lun_object_id);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

            status = fbe_api_sim_transport_set_target_server(second_io_sp);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            status = fbe_api_wait_for_object_lifecycle_state(lun_object_id, FBE_LIFECYCLE_STATE_READY,
                                                             FBE_TEST_WAIT_TIMEOUT_MS, FBE_PACKAGE_ID_SEP_0);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

            status = fbe_api_sim_transport_set_target_server(first_io_sp);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            status = fbe_api_wait_for_object_lifecycle_state(lun_object_id, FBE_LIFECYCLE_STATE_READY,
                                                             FBE_TEST_WAIT_TIMEOUT_MS, FBE_PACKAGE_ID_SEP_0);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        }
        current_rg_config_p++;
    }

    status = fbe_api_sim_transport_set_target_server(first_io_sp);
    abby_cadabby_wait_for_verifies(rg_config_p, raid_group_count);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s complete ==", __FUNCTION__);
    status = fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    return;
}
/**************************************
 * end pippin_unexpected_errs_alternate_sps()
 **************************************/

/*!**************************************************************
 * pippin_unexpected_errs_one_sp()
 ****************************************************************
 * @brief
 *  - Issue I/O with errors injected on oen SP until LUN fails.
 *  - Ensure that we hit a critical error.
 *  - Note that we disable the panic on critical error for this test so
 *    that we can control the hitting of the critical error.
 *  
 *
 * @param rg_config_p - Config array to create.  This is null terminated.
 *
 * @return None.
 *
 * @author
 *  9/26/2012 - Created. Rob Foley
 *
 ****************************************************************/

void pippin_unexpected_errs_one_sp(fbe_test_rg_configuration_t *rg_config_p,
                                                  fbe_rdgen_operation_t rdgen_opcode,
                                                  fbe_bool_t b_non_cached)
{
    fbe_api_rdgen_context_t * context_p = &pippin_rdgen_contexts[0];
    fbe_status_t status;
    fbe_u32_t raid_group_count = fbe_test_get_rg_array_length(rg_config_p);
    fbe_test_rg_configuration_t *current_rg_config_p = NULL;
    fbe_object_id_t lun_object_id;
    fbe_u32_t rg_index;
    fbe_u32_t lun_index;
    fbe_sim_transport_connection_target_t target_sp = FBE_SIM_SP_A;
    fbe_sim_transport_connection_target_t peer_sp = FBE_SIM_SP_B;

    if (fbe_random() % 2)
    {
        target_sp = FBE_SIM_SP_B;
        peer_sp = FBE_SIM_SP_A;
    }

    mut_printf(MUT_LOG_TEST_STATUS, "== %s starting opcode: %d %s==", __FUNCTION__, rdgen_opcode, b_non_cached ? "non-cached" : "cached");

    /* We need to setup the injection and kick off I/O.
     */
    fbe_api_sim_transport_set_target_server(target_sp);
    pippin_enable_injection();
    pippin_start_io(&pippin_rdgen_contexts[0], rdgen_opcode, b_non_cached);

    /* The LUN will decide to critical error.
     */
    fbe_test_wait_for_critical_error();

    status = fbe_api_rdgen_stop_tests(context_p, 1);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_NOT_EQUAL(context_p->start_io.statistics.invalid_request_err_count, 0);
    MUT_ASSERT_INT_EQUAL(context_p->start_io.status.block_status, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID_REQUEST);
    MUT_ASSERT_INT_EQUAL(context_p->start_io.status.block_qualifier, FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_UNEXPECTED_ERROR);

    /* Reset error-testing stats which will clean all logs as well as 
     * disable error-testing.
     */
    fbe_api_raid_group_reset_raid_lib_error_testing_stats();
    fbe_test_sep_util_restore_sector_trace_level();

    /* Crash the SP manually.
     */
    mut_printf(MUT_LOG_LOW, " == Shutdown target SP (%s) == ", target_sp == FBE_SIM_SP_A ? "SP A" : "SP B");
    fbe_api_sim_transport_destroy_client(target_sp);
    fbe_test_sp_sim_stop_single_sp(target_sp == FBE_SIM_SP_A ? FBE_TEST_SPA : FBE_TEST_SPB);

    status = fbe_api_sim_transport_set_target_server(peer_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Validate that the peer SP has the LUNs in ready still.
     */
    current_rg_config_p = rg_config_p;
    for (rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        for ( lun_index = 0; lun_index < PIPPIN_TEST_LUNS_PER_RAID_GROUP; lun_index++)
        {
            status = fbe_api_database_lookup_lun_by_number(current_rg_config_p->logical_unit_configuration_list[lun_index].lun_number,
                                                           &lun_object_id);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            status = fbe_api_wait_for_object_lifecycle_state(lun_object_id, FBE_LIFECYCLE_STATE_READY,
                                                             FBE_TEST_WAIT_TIMEOUT_MS, FBE_PACKAGE_ID_SEP_0);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        }
        current_rg_config_p++;
    }
    
    status = fbe_api_sim_transport_set_target_server(target_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_LOW, " == Startup target SP (%s) == ", target_sp == FBE_SIM_SP_A ? "SP A" : "SP B");
    fbe_test_boot_sp(rg_config_p, target_sp);

    /* Disable backtrace to avoid filling the logs.
     */
    status = fbe_api_trace_disable_backtrace(FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = fbe_api_trace_disable_backtrace(FBE_PACKAGE_ID_NEIT);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Make sure all the objects come back on both sides.
     */
    current_rg_config_p = rg_config_p;
    for (rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        for ( lun_index = 0; lun_index < PIPPIN_TEST_LUNS_PER_RAID_GROUP; lun_index++)
        {
            status = fbe_api_database_lookup_lun_by_number(current_rg_config_p->logical_unit_configuration_list[lun_index].lun_number,
                                                           &lun_object_id);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            status = fbe_api_sim_transport_set_target_server(target_sp);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            status = fbe_api_wait_for_object_lifecycle_state(lun_object_id, FBE_LIFECYCLE_STATE_READY,
                                                             FBE_TEST_WAIT_TIMEOUT_MS, FBE_PACKAGE_ID_SEP_0);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

            status = fbe_api_sim_transport_set_target_server(peer_sp);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            status = fbe_api_wait_for_object_lifecycle_state(lun_object_id, FBE_LIFECYCLE_STATE_READY,
                                                             FBE_TEST_WAIT_TIMEOUT_MS, FBE_PACKAGE_ID_SEP_0);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        }
        current_rg_config_p++;
    }

    status = fbe_api_sim_transport_set_target_server(target_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    abby_cadabby_wait_for_verifies(rg_config_p, raid_group_count);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s complete ==", __FUNCTION__);
    status = fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    return;
}
/**************************************
 * end pippin_unexpected_errs_one_sp()
 **************************************/

/*!**************************************************************
 * pippin_reset_unexpected_errors()
 ****************************************************************
 * @brief
 *  Reset the number of unexpected errors.
 *
 * @param rg_config_p - Config array to create.  This is null terminated.
 *
 * @return None.
 *
 * @author
 *  7/9/2012 - Created. Rob Foley
 *
 ****************************************************************/

void pippin_reset_unexpected_errors(fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_status_t status;
    fbe_object_id_t lun_object_id;
    fbe_u32_t raid_group_count = fbe_test_get_rg_array_length(rg_config_p);
    fbe_test_rg_configuration_t *current_rg_config_p = NULL;
    fbe_u32_t rg_index;
    fbe_u32_t lun_index;

    current_rg_config_p = rg_config_p;
    for (rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        for ( lun_index = 0; lun_index < PIPPIN_TEST_LUNS_PER_RAID_GROUP; lun_index++)
        {
            status = fbe_api_database_lookup_lun_by_number(current_rg_config_p->logical_unit_configuration_list[lun_index].lun_number,
                                                           &lun_object_id);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            /* clear the errors.
             */
            status = fbe_api_lun_clear_unexpected_errors(lun_object_id);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        }
        current_rg_config_p++;
    }
    return;
}
/**************************************
 * end pippin_reset_unexpected_errors()
 **************************************/

/*!**************************************************************
 * pippin_wait_for_initial_verify()
 ****************************************************************
 * @brief
 *  Wait until the initial verify finishes, since this
 *  will interfere with our test.
 *  If a verify gets unexpected errors on bitmap I/Os, it will cause
 *  the raid group to goto activate.  This slows down the test
 *  significantly, causing it to timeout.
 *
 * @param rg_config_p - Config array to create.  This is null terminated.
 *
 * @return None.
 *
 * @author
 *  7/9/2012 - Created. Rob Foley
 *
 ****************************************************************/

void pippin_wait_for_initial_verify(fbe_test_rg_configuration_t *rg_config_p)
{
    fbe_status_t status;
    fbe_object_id_t lun_object_id;
    fbe_u32_t raid_group_count = fbe_test_get_rg_array_length(rg_config_p);
    fbe_test_rg_configuration_t *current_rg_config_p = NULL;
    fbe_u32_t rg_index;
    fbe_u32_t lun_index;
    fbe_api_lun_get_lun_info_t lun_info;
    fbe_lun_verify_report_t verify_report;

    current_rg_config_p = rg_config_p;
    for (rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        for ( lun_index = 0; lun_index < PIPPIN_TEST_LUNS_PER_RAID_GROUP; lun_index++)
        {
            status = fbe_api_database_lookup_lun_by_number(current_rg_config_p->logical_unit_configuration_list[lun_index].lun_number,
                                                           &lun_object_id);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

            /* First make sure we finished the zeroing.
             */
            fbe_test_sep_util_wait_for_lun_zeroing_complete(current_rg_config_p->logical_unit_configuration_list[lun_index].lun_number, 
                                                            FBE_TEST_WAIT_TIMEOUT_MS, FBE_PACKAGE_ID_SEP_0);

            /* Then make sure we finished the verify.  This waits for pass 1 to complete.
             */
            fbe_test_sep_util_wait_for_lun_verify_completion(lun_object_id, &verify_report, 1);

            /* Next make sure that the LUN was requested to do initial verify and that it performed it already.
             */
            status = fbe_api_lun_get_lun_info(lun_object_id, &lun_info);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            MUT_ASSERT_INT_EQUAL(lun_info.b_initial_verify_already_run, 1);
            MUT_ASSERT_INT_EQUAL(lun_info.b_initial_verify_requested, 1);
        }
        current_rg_config_p++;
    }
    return;
}
/**************************************
 * end pippin_wait_for_initial_verify()
 **************************************/
/*!**************************************************************
 * pippin_unexpected_error_io()
 ****************************************************************
 * @brief
 *  Issue I/O while injecting unexpected errors.
 *  We do not expect the LUN to fail since we do not issue
 *  enough I/Os to go over the threshold.
 *
 * @param rg_config_p - Config array to create.  This is null terminated.
 * @param b_failure_expected - FBE_TRUE if the lun is expected to fail.
 * @param num_passes - Number of passes to run.
 *
 * @return None.
 *
 * @author
 *  6/29/2012 - Created. Rob Foley
 *
 ****************************************************************/

void pippin_unexpected_error_io(fbe_test_rg_configuration_t *rg_config_p,
                                fbe_bool_t b_failure_expected,
                                fbe_u32_t num_passes)
{
    fbe_api_rdgen_context_t * context_p = &pippin_rdgen_contexts[0];
    fbe_status_t status;
    fbe_rdgen_operation_t rdgen_opcode = FBE_RDGEN_OPERATION_READ_ONLY;
    fbe_u32_t raid_group_count = fbe_test_get_rg_array_length(rg_config_p);
    fbe_test_rg_configuration_t *current_rg_config_p = NULL;
    fbe_object_id_t lun_object_id;
    fbe_u32_t rg_index;
    fbe_u32_t lun_index;
    fbe_lifecycle_state_t current_state;

    mut_printf(MUT_LOG_TEST_STATUS, "== %s starting ==", __FUNCTION__);

    status = fbe_api_clear_event_log(FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    fbe_test_sep_util_reduce_sector_trace_level();

	fbe_test_sep_util_disable_trace_limits();

    /* Enable random error-testing path which will consistently give us back errors. 
     */
    fbe_api_raid_group_enable_raid_lib_random_errors(100, FBE_TRUE /* user RGs only */);

    /* Setup the rdgen context with more than enough errors to fail the lun.
     */
    status = pippin_setup_rdgen_test_context(context_p,
                                             FBE_OBJECT_ID_INVALID,
                                             FBE_CLASS_ID_LUN,
                                             rdgen_opcode,
                                             FBE_LBA_INVALID,    /* use capacity */
                                             num_passes,  /* limit the passes */
                                             1,   /* threads */
                                             PIPPIN_MAX_IO_BLOCKS);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    /* We need this option since the LUN will occasionally 
     * transition its edge as it gets unexpected errors. 
     */
    status = fbe_api_rdgen_io_specification_set_options(&context_p->start_io.specification, FBE_RDGEN_OPTIONS_CONTINUE_ON_ALL_ERRORS);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = fbe_api_rdgen_io_specification_set_io_complete_flags(&context_p->start_io.specification, 
                                                                  FBE_RDGEN_IO_COMPLETE_FLAGS_UNEXPECTED_ERRORS);
    /* Run rdgen operation(s).
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== run I/O with error injections ==");
    status = fbe_api_rdgen_test_context_run_all_tests(context_p, FBE_PACKAGE_ID_NEIT, 1);
    MUT_ASSERT_INT_NOT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_NOT_EQUAL(context_p->start_io.statistics.invalid_request_err_count, 0);
    MUT_ASSERT_INT_EQUAL(context_p->start_io.status.block_status, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID_REQUEST);
    MUT_ASSERT_INT_EQUAL(context_p->start_io.status.block_qualifier, FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_UNEXPECTED_ERROR);
    mut_printf(MUT_LOG_TEST_STATUS, "== run I/O with error injections Done.==");

    /* Reset error-testing stats which will clean all logs as well as 
     * disable error-testing.
     */
    fbe_api_raid_group_reset_raid_lib_error_testing_stats();
    fbe_test_sep_util_restore_sector_trace_level();

    current_rg_config_p = rg_config_p;
    for (rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        for ( lun_index = 0; lun_index < PIPPIN_TEST_LUNS_PER_RAID_GROUP; lun_index++)
        {
            status = fbe_api_database_lookup_lun_by_number(current_rg_config_p->logical_unit_configuration_list[lun_index].lun_number,
                                                           &lun_object_id);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

            if (b_failure_expected)
            {
                /* Wait for the LUN to fail, then clear errors so the LUN can start to transition ready.
                 */
                mut_printf(MUT_LOG_TEST_STATUS, "wait for lun %d (obj 0x%x) to fail",
                           current_rg_config_p->logical_unit_configuration_list[lun_index].lun_number,
                           lun_object_id);
                status = fbe_api_wait_for_object_lifecycle_state(lun_object_id, FBE_LIFECYCLE_STATE_FAIL,
                                                                 FBE_TEST_WAIT_TIMEOUT_MS, FBE_PACKAGE_ID_SEP_0);
                MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
                pippin_check_event_log_msg_present(lun_object_id, 
                                                   current_rg_config_p->logical_unit_configuration_list[lun_index].lun_number);

                /* clear the errors.
                 */
                status = fbe_api_lun_clear_unexpected_errors(lun_object_id);
                MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            }
            else
            {
                status = fbe_api_get_object_lifecycle_state(lun_object_id, &current_state, FBE_PACKAGE_ID_SEP_0);
                MUT_ASSERT_INT_EQUAL(current_state, FBE_LIFECYCLE_STATE_READY);
                MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
                /* Should be no messages of this type.
                 */
                pippin_check_event_log_msg_not_present(lun_object_id, current_rg_config_p->logical_unit_configuration_list[lun_index].lun_number);
            }
        }
        current_rg_config_p++;
    }

    if (b_failure_expected)
    {
        /* Just ensure all LUNs become ready.
         */
        current_rg_config_p = rg_config_p;
        for (rg_index = 0; rg_index < raid_group_count; rg_index++)
        {
            for ( lun_index = 0; lun_index < PIPPIN_TEST_LUNS_PER_RAID_GROUP; lun_index++)
            {
                status = fbe_api_database_lookup_lun_by_number(current_rg_config_p->logical_unit_configuration_list[lun_index].lun_number,
                                                               &lun_object_id);
                MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

                mut_printf(MUT_LOG_TEST_STATUS, "wait for lun %d (obj 0x%x) to become ready",
                           current_rg_config_p->logical_unit_configuration_list[lun_index].lun_number,
                           lun_object_id);
                status = fbe_api_wait_for_object_lifecycle_state(lun_object_id, FBE_LIFECYCLE_STATE_READY,
                                                                 FBE_TEST_WAIT_TIMEOUT_MS, FBE_PACKAGE_ID_SEP_0);
                MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            }
            current_rg_config_p++;
        }
        abby_cadabby_wait_for_verifies(rg_config_p, raid_group_count);
    }
    return;
}
/**************************************
 * end pippin_unexpected_error_io()
 **************************************/

/*!**************************************************************
 * pippin_normal_io()
 ****************************************************************
 * @brief
 *  Just run normal I/O to the LUNs.
 *
 * @param rg_config_p - Config array to create.  This is null terminated.
 * @param num_ios - Number of I/Os.
 *
 * @return None.
 *
 * @author
 *  6/29/2012 - Created. Rob Foley
 *
 ****************************************************************/

void pippin_normal_io(fbe_test_rg_configuration_t *rg_config_p, fbe_u32_t num_ios)
{
    fbe_api_rdgen_context_t * context_p = &pippin_rdgen_contexts[0];
    fbe_status_t status;
    fbe_rdgen_operation_t rdgen_opcode = FBE_RDGEN_OPERATION_READ_ONLY;
    fbe_u32_t raid_group_count = fbe_test_get_rg_array_length(rg_config_p);
    fbe_test_rg_configuration_t *current_rg_config_p = NULL;
    fbe_object_id_t lun_object_id;
    fbe_u32_t rg_index;
    fbe_u32_t lun_index;
    fbe_lifecycle_state_t current_state;

    mut_printf(MUT_LOG_TEST_STATUS, "== %s starting ==", __FUNCTION__);

    status = fbe_api_clear_event_log(FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    fbe_test_sep_util_reduce_sector_trace_level();

	fbe_test_sep_util_disable_trace_limits();

    /* Setup the rdgen context with with enough I/O to wipe out the bad I/Os.
     */
    status = pippin_setup_rdgen_test_context(context_p,
                                             FBE_OBJECT_ID_INVALID,
                                             FBE_CLASS_ID_LUN,
                                             rdgen_opcode,
                                             FBE_LBA_INVALID,    /* use capacity */
                                             num_ios,  /* limit the passes */
                                             1,   /* threads */
                                             PIPPIN_MAX_IO_BLOCKS);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    /* Run rdgen operation(s).  No errors are expected.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== run Normal I/O ==");
    status = fbe_api_rdgen_test_context_run_all_tests(context_p, FBE_PACKAGE_ID_NEIT, 1);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.invalid_request_err_count, 0);
    MUT_ASSERT_INT_EQUAL(context_p->start_io.status.block_status, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS);
    MUT_ASSERT_INT_EQUAL(context_p->start_io.status.block_qualifier, FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE);
    mut_printf(MUT_LOG_TEST_STATUS, "== run Normal I/O Done==");

    current_rg_config_p = rg_config_p;
    for (rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        for ( lun_index = 0; lun_index < PIPPIN_TEST_LUNS_PER_RAID_GROUP; lun_index++)
        {
            status = fbe_api_database_lookup_lun_by_number(current_rg_config_p->logical_unit_configuration_list[lun_index].lun_number,
                                                           &lun_object_id);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            status = fbe_api_get_object_lifecycle_state(lun_object_id, &current_state, FBE_PACKAGE_ID_SEP_0);
            MUT_ASSERT_INT_EQUAL(current_state, FBE_LIFECYCLE_STATE_READY);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status); 
            /* Should be no messages of this type.
             */
            pippin_check_event_log_msg_not_present(lun_object_id, current_rg_config_p->logical_unit_configuration_list[lun_index].lun_number);
        }
        current_rg_config_p++;
    }
    return;
}
/**************************************
 * end pippin_normal_io()
 **************************************/

/*!**************************************************************
 * pippin_run_tests()
 ****************************************************************
 * @brief
 *  Run the pippin test
 *
 * @param rg_config_p - Config array to create.  This is null terminated.
 * @param context_p - Not used.
 *
 * @return None.
 *
 * @author
 *  6/26/2012 - Created. Rob Foley
 *
 ****************************************************************/

void pippin_run_tests(fbe_test_rg_configuration_t *rg_config_p, void * context_p)
{
    char *test_cases_option_p = mut_get_user_option_value("-start_index");
    fbe_u32_t index = 0;

    /* Wait for initial verify so verifies do not get kicked off in the middle of our test. 
     * Failed md I/Os result in raid group breaking, which makes the test very slow to run. 
     */
    fbe_test_sep_util_wait_for_initial_verify(rg_config_p);

    if (fbe_test_is_test_case_enabled(index++, test_cases_option_p))
    {
        pippin_fail_luns_with_unexpected_errors_test(rg_config_p, FBE_RDGEN_OPERATION_READ_ONLY, FBE_FALSE);
    }
    if (fbe_test_is_test_case_enabled(index++, test_cases_option_p))
    {
        pippin_fail_luns_with_unexpected_errors_test(rg_config_p, FBE_RDGEN_OPERATION_WRITE_ONLY, FBE_TRUE /* non-cached */);
    }
    if (fbe_test_is_test_case_enabled(index++, test_cases_option_p))
    {
        pippin_fail_luns_with_unexpected_errors_test(rg_config_p, FBE_RDGEN_OPERATION_WRITE_ONLY, 
                                                     FBE_FALSE /* not non-cached */ );
    }
    if (fbe_test_is_test_case_enabled(index++, test_cases_option_p))
    {
        pippin_unexpected_errors_disabled_test(rg_config_p);
    }
    return;
}
/**************************************
 * end pippin_run_tests()
 **************************************/
/*!**************************************************************
 * pippin_dualsp_run_tests()
 ****************************************************************
 * @brief
 *  Run the dualsp pippin test
 *
 * @param rg_config_p - Config array to create.  This is null terminated.
 * @param context_p - Not used.
 *
 * @return None.
 *
 * @author
 *  9/26/2012 - Created. Rob Foley
 *
 ****************************************************************/

void pippin_dualsp_run_tests(fbe_test_rg_configuration_t *rg_config_p, void * context_p)
{
    char *test_cases_option_p = mut_get_user_option_value("-start_index");
    fbe_u32_t index = 0;

    /* Wait for initial verify so verifies do not get kicked off in the middle of our test. 
     * Failed md I/Os result in raid group breaking, which makes the test very slow to run. 
     */
    fbe_test_sep_util_wait_for_initial_verify(rg_config_p);
    if (fbe_test_is_test_case_enabled(index++, test_cases_option_p))
    {
        pippin_unexpected_errs_one_sp(rg_config_p, FBE_RDGEN_OPERATION_READ_ONLY, FBE_FALSE);
    }
    if (fbe_test_is_test_case_enabled(index++, test_cases_option_p))
    {
        pippin_unexpected_errs_both_sps(rg_config_p, FBE_RDGEN_OPERATION_READ_ONLY, FBE_FALSE);
    }
    if (fbe_test_is_test_case_enabled(index++, test_cases_option_p))
    {
        pippin_unexpected_errs_alternate_sps(rg_config_p, FBE_RDGEN_OPERATION_READ_ONLY, FBE_FALSE, FBE_TRUE);
        pippin_unexpected_errs_alternate_sps(rg_config_p, FBE_RDGEN_OPERATION_READ_ONLY, FBE_FALSE, FBE_FALSE);
    }
    
    return;
}
/**************************************
 * end pippin_dualsp_run_tests()
 **************************************/
/*!**************************************************************
 * pippin_test()
 ****************************************************************
 * @brief
 *  Run pipipin test.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  6/26/2012 - Created. Rob Foley
 *
 ****************************************************************/

void pippin_test(void)
{
    fbe_test_rg_configuration_t *rg_config_p = NULL;
    fbe_u32_t CSX_MAYBE_UNUSED test_level = fbe_sep_test_util_get_raid_testing_extended_level();

    /* Based on the test level determine which configuration to use.
     */
    rg_config_p = &pippin_raid_group_config[0];
    fbe_test_run_test_on_rg_config(rg_config_p, NULL, pippin_run_tests,
                                   PIPPIN_TEST_LUNS_PER_RAID_GROUP,
                                   PIPPIN_CHUNKS_PER_LUN);
    return;
}
/******************************************
 * end pippin_test()
 ******************************************/
/*!**************************************************************
 * pippin_setup()
 ****************************************************************
 * @brief
 *  Setup for a pippin test.
 *
 * @param None.               
 *
 * @return None.   
 *
 * @author
 *  6/26/2012 - Created. Rob Foley
 *
 ****************************************************************/
void pippin_setup(void)
{
    fbe_status_t status;
    /* Only load the physical config in simulation.
    */
    if (fbe_test_util_is_simulation())
    {
        fbe_test_rg_configuration_t *rg_config_p = NULL;
        fbe_u32_t CSX_MAYBE_UNUSED test_level = fbe_sep_test_util_get_raid_testing_extended_level();
        fbe_u32_t num_raid_groups;

        /* Based on the test level determine which configuration to use.
        */
        rg_config_p = &pippin_raid_group_config[0];
        fbe_test_sep_util_init_rg_configuration_array(rg_config_p);
        fbe_test_sep_util_randomize_rg_configuration_array(rg_config_p);
        num_raid_groups = fbe_test_get_rg_array_length(rg_config_p);
        fbe_test_sep_util_update_extra_chunk_size(rg_config_p, PIPPIN_CHUNKS_PER_LUN * 2);

        /* initialize the number of extra drive required by each rg 
          */
        fbe_test_sep_util_populate_rg_num_extra_drives(rg_config_p);

        elmo_create_physical_config_for_rg(rg_config_p, num_raid_groups);
        elmo_load_sep_and_neit();
    }
    /* Disable backtrace to avoid filling the logs.
     */
    status = fbe_api_trace_disable_backtrace(FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = fbe_api_trace_disable_backtrace(FBE_PACKAGE_ID_NEIT);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Initialize any required fields and perform cleanup if required
     */
    fbe_test_common_util_test_setup_init();
    fbe_test_disable_background_ops_system_drives();

    return;
}
/**************************************
 * end pippin_setup()
 **************************************/

/*!**************************************************************
 * pippin_cleanup()
 ****************************************************************
 * @brief
 *  Cleanup the pippin test.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  6/26/2012 - Created. Rob Foley
 *
 ****************************************************************/

void pippin_cleanup(void)
{
    fbe_status_t status;

    mut_printf(MUT_LOG_HIGH, "%s entry", __FUNCTION__);

    status = fbe_api_trace_enable_backtrace(FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = fbe_api_trace_enable_backtrace(FBE_PACKAGE_ID_NEIT);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    if (fbe_test_util_is_simulation())
    {
        fbe_sep_util_destroy_neit_sep_phy_expect_errs();
    }
    return;
}
/******************************************
 * end pippin_cleanup()
 ******************************************/

/*!**************************************************************
 * pippin_dualsp_test()
 ****************************************************************
 * @brief
 *  Run pipipin test.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  9/7/2012 - Created. Rob Foley
 *
 ****************************************************************/

void pippin_dualsp_test(void)
{
    fbe_test_rg_configuration_t *rg_config_p = NULL;
    fbe_u32_t CSX_MAYBE_UNUSED test_level = fbe_sep_test_util_get_raid_testing_extended_level();

    /* Based on the test level determine which configuration to use.
     */
    rg_config_p = &pippin_raid_group_config[0];
    fbe_test_run_test_on_rg_config(rg_config_p, NULL, pippin_dualsp_run_tests,
                                   2,
                                   PIPPIN_CHUNKS_PER_LUN);
    return;
}
/******************************************
 * end pippin_dualsp_test()
 ******************************************/
/*!**************************************************************
 * pippin_dualsp_setup()
 ****************************************************************
 * @brief
 *  Setup for a pippin test.
 *
 * @param None.               
 *
 * @return None.   
 *
 * @author
 *  9/7/2012 - Created. Rob Foley
 *
 ****************************************************************/
void pippin_dualsp_setup(void)
{
    fbe_status_t status;
    /* Only load the physical config in simulation.
    */
    if (fbe_test_util_is_simulation())
    {
        fbe_test_rg_configuration_t *rg_config_p = NULL;
        fbe_u32_t CSX_MAYBE_UNUSED test_level = fbe_sep_test_util_get_raid_testing_extended_level();
        fbe_u32_t num_raid_groups;

        /* Based on the test level determine which configuration to use.
        */
        rg_config_p = &pippin_raid_group_config[0];
        fbe_test_sep_util_init_rg_configuration_array(rg_config_p);
        fbe_test_sep_util_randomize_rg_configuration_array(rg_config_p);
        num_raid_groups = fbe_test_get_rg_array_length(rg_config_p);
        fbe_test_sep_util_update_extra_chunk_size(rg_config_p, PIPPIN_CHUNKS_PER_LUN * 2);

        /* initialize the number of extra drive required by each rg 
          */
        fbe_test_sep_util_populate_rg_num_extra_drives(rg_config_p);
        
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
        elmo_create_physical_config_for_rg(rg_config_p, num_raid_groups);

        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
        elmo_create_physical_config_for_rg(rg_config_p, num_raid_groups);

        sep_config_load_sep_and_neit_load_balance_both_sps();
    }

    /* Disable backtrace to avoid filling the logs.
     */
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    status = fbe_api_trace_disable_backtrace(FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = fbe_api_trace_disable_backtrace(FBE_PACKAGE_ID_NEIT);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    status = fbe_api_trace_disable_backtrace(FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = fbe_api_trace_disable_backtrace(FBE_PACKAGE_ID_NEIT);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);

    /* Initialize any required fields and perform cleanup if required
     */
    fbe_test_common_util_test_setup_init();
    fbe_test_sep_util_set_dualsp_test_mode(FBE_TRUE);
    fbe_test_disable_background_ops_system_drives();

    return;
}
/**************************************
 * end pippin_dualsp_setup()
 **************************************/

/*!**************************************************************
 * pippin_dualsp_cleanup()
 ****************************************************************
 * @brief
 *  Cleanup the pippin test.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  9/7/2012 - Created. Rob Foley
 *
 ****************************************************************/

void pippin_dualsp_cleanup(void)
{
    fbe_status_t status;
    /* Always clear dualsp mode
     */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);

    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    status = fbe_api_trace_enable_backtrace(FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = fbe_api_trace_enable_backtrace(FBE_PACKAGE_ID_NEIT);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    status = fbe_api_trace_enable_backtrace(FBE_PACKAGE_ID_SEP_0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = fbe_api_trace_enable_backtrace(FBE_PACKAGE_ID_NEIT);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_HIGH, "%s entry", __FUNCTION__);
    if (fbe_test_util_is_simulation())
    {
        /* First execute teardown on SP B then on SP A
        */
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
        fbe_sep_util_destroy_neit_sep_phy_expect_errs();

        /* First execute teardown on SP A
         */
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
        fbe_sep_util_destroy_neit_sep_phy_expect_errs();
    }

    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);
    return;
}
/******************************************
 * end pippin_dualsp_cleanup()
 ******************************************/
/*************************
 * end file pippin_test.c
 *************************/


