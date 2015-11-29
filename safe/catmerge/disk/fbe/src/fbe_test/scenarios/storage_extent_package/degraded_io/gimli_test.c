/***************************************************************************
 * Copyright (C) EMC Corporation 2013
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file gimli_test.c
 ***************************************************************************
 *
 * @brief
 *  This file contains a test of RAID Reject flags for:
 *   - Rejecting degraded writes that are not aligned to full stripes (parity raid groups only)
 *   - Rejecting I/Os on the non-preferred path during SLF.
 *
 * @version
 *  2/21/2013 - Created. Rob Foley
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
#include "fbe/fbe_api_system_bg_service_interface.h"
#include "fbe/fbe_api_panic_sp_interface.h"

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

char * gimli_short_desc = "Test that RAID can reject I/Os based on client reject flags.";
char * gimli_long_desc = "\
This scenario tests that the RAID Group can reject I/Os for the following scenarios:\n"\
"\n\
  - Rejecting degraded writes that are not aligned to full stripes (parity raid groups only)\n\
  - Rejecting I/Os on the non-preferred path during SLF.\n\
\n\
Description last updated: 2/21/2013.\n";

/*!*******************************************************************
 * @def GIMLI_WAIT_IO_COUNT
 *********************************************************************
 * @brief Number of I/Os per thread we need to wait for after starting
 *        I/O.  Ensures that at least some I/O completes.
 *
 *********************************************************************/
#define GIMLI_WAIT_IO_COUNT 1

/*!*******************************************************************
 * @def GIMLI_TEST_MAX_LUNS_PER_RAID_GROUP
 *********************************************************************
 * @brief luns per rg for the extended test. 
 *
 *********************************************************************/
#define GIMLI_TEST_MAX_LUNS_PER_RAID_GROUP 3

/*!*******************************************************************
 * @def GIMLI_TEST_LUNS_PER_RAID_GROUP
 *********************************************************************
 * @brief luns per rg for the extended test. 
 *
 *********************************************************************/
#define GIMLI_TEST_LUNS_PER_RAID_GROUP 3

/*!*******************************************************************
 * @def GIMLI_CHUNKS_PER_LUN
 *********************************************************************
 * @brief Number of chunks each LUN will occupy.
 *
 *********************************************************************/
#define GIMLI_CHUNKS_PER_LUN 3

/*!*******************************************************************
 * @def GIMLI_MAX_IO_SIZE_BLOCKS
 *********************************************************************
 * @brief Max number of blocks we will test with.
 *
 *********************************************************************/
#define GIMLI_MAX_IO_SIZE_BLOCKS (FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1) * FBE_RAID_MAX_BE_XFER_SIZE 

/*!*******************************************************************
 * @def GIMLI_SMALL_IO_SIZE_BLOCKS
 *********************************************************************
 * @brief Max number of blocks we will test with.
 *
 *********************************************************************/
#define GIMLI_SMALL_IO_SIZE_BLOCKS 1024

/*!*******************************************************************
 * @def GIMLI_LIGHT_THREAD_COUNT
 *********************************************************************
 * @brief Thread count for a light load test.
 *
 *********************************************************************/
#define GIMLI_LIGHT_THREAD_COUNT 1

/*!*******************************************************************
 * @def GIMLI_THIRD_POSITION_TO_REMOVE
 *********************************************************************
 * @brief The 3rd array position to remove a drive for, which will
 *        shutdown the rg.
 *
 *********************************************************************/
#define GIMLI_THIRD_POSITION_TO_REMOVE 2

/*!*******************************************************************
 * @def GIMLI_LONG_IO_SEC
 *********************************************************************
 * @brief Min time we will sleep for.
 *********************************************************************/
#define GIMLI_LONG_IO_SEC 5

/*!*******************************************************************
 * @def GIMLI_WAIT_SEC
 *********************************************************************
 * @brief Number of seconds we should wait for state changes.
 *        We set this relatively large so that if we do exceed this
 *        we will be sure something is wrong.
 *
 *********************************************************************/
#define GIMLI_WAIT_SEC 60

/*!*******************************************************************
 * @def GIMLI_WAIT_MSEC
 *********************************************************************
 * @brief Number of milliseconds we should wait for state changes.
 *        We set this relatively large so that if we do exceed this
 *        we will be sure something is wrong.
 *
 *********************************************************************/
#define GIMLI_WAIT_MSEC 1000 * GIMLI_WAIT_SEC

/*!*******************************************************************
 * @def GIMLI_MAX_DRIVES_TO_REMOVE 
 *********************************************************************
 * @brief Max number of drives we will remove to make the raid group degraded.
 *
 *********************************************************************/
#define GIMLI_MAX_DRIVES_TO_REMOVE 2


/*!*******************************************************************
 * @def GIMLI_PARITY_GROUPS 
 *********************************************************************
 * @brief Max number of drives we will remove to make the raid group degraded.
 *
 *********************************************************************/
#define GIMLI_PARITY_GROUPS 3

/*!*******************************************************************
 * @def GIMLI_MIRROR_GROUPS 
 *********************************************************************
 * @brief Max number of drives we will remove to make the raid group degraded.
 *
 *********************************************************************/
#define GIMLI_MIRROR_GROUPS 2

/*!*******************************************************************
 * @var gimli_test_contexts
 *********************************************************************
 * @brief This contains our context objects for running rdgen I/O.
 *
 *********************************************************************/
static fbe_api_rdgen_context_t gimli_test_contexts[GIMLI_TEST_MAX_LUNS_PER_RAID_GROUP * 2];

/*!*******************************************************************
 * @var gimli_raid_group_config_extended
 *********************************************************************
 * @brief This is the array of configurations we will loop over.
 *
 *********************************************************************/
fbe_test_rg_configuration_t gimli_raid_group_config_extended[] = 
{
    /* width,   capacity    raid type,                  class,               block size     RAID-id.    bandwidth.*/
    {FBE_TEST_RG_CONFIG_RANDOM_TAG, 0xE000, FBE_RAID_GROUP_TYPE_RAID6,   FBE_CLASS_ID_PARITY,  520, 1, FBE_TEST_RG_CONFIG_RANDOM_TAG},
    {FBE_TEST_RG_CONFIG_RANDOM_TAG, 0xE000, FBE_RAID_GROUP_TYPE_RAID5,   FBE_CLASS_ID_PARITY,  520, 2, FBE_TEST_RG_CONFIG_RANDOM_TAG},
    {FBE_TEST_RG_CONFIG_RANDOM_TAG, 0xE000, FBE_RAID_GROUP_TYPE_RAID3,   FBE_CLASS_ID_PARITY,  520, 3, FBE_TEST_RG_CONFIG_RANDOM_TAG},
    {FBE_TEST_RG_CONFIG_RANDOM_TAG, 0xE000, FBE_RAID_GROUP_TYPE_RAID10,  FBE_CLASS_ID_STRIPER, 520, 0, FBE_TEST_RG_CONFIG_RANDOM_TAG},
    {FBE_TEST_RG_CONFIG_RANDOM_TAG, 0xE000, FBE_RAID_GROUP_TYPE_RAID1,   FBE_CLASS_ID_MIRROR,  520, 4, 0},
    {FBE_TEST_RG_CONFIG_RANDOM_TAG, 0xE000, FBE_RAID_GROUP_TYPE_RAID0,   FBE_CLASS_ID_STRIPER, 520, 5, FBE_TEST_RG_CONFIG_RANDOM_TAG},

    {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
};

/*!*******************************************************************
 * @var gimli_raid_group_config_qual
 *********************************************************************
 * @brief This is the array of configurations we will loop over.
 *
 *********************************************************************/
fbe_test_rg_configuration_t gimli_raid_group_config_qual[] = 
{
    /* width,   capacity    raid type,                  class,                  block size      RAID-id.    bandwidth.*/
    {5,       0xE000,     FBE_RAID_GROUP_TYPE_RAID5,  FBE_CLASS_ID_PARITY,   520,            4,          0},

    {FBE_U32_MAX, FBE_U32_MAX, FBE_U32_MAX, /* Terminator. */},
};

/*!*******************************************************************
 * @var gimli_io_msec_short
 *********************************************************************
 * @brief The default I/O time we will run for short durations.
 * 
 *********************************************************************/
static fbe_u32_t gimli_io_msec_short = EMCPAL_MINIMUM_TIMEOUT_MSECS;


extern fbe_status_t big_bird_setup_lun_test_contexts(fbe_test_rg_configuration_t *rg_config_p,
                                                     fbe_api_rdgen_context_t *context_p,
                                                     fbe_u32_t num_contexts,
                                                     fbe_rdgen_operation_t rdgen_operation,
                                                     fbe_u32_t threads,
                                                     fbe_u32_t io_size_blocks,
                                                     fbe_test_random_abort_msec_t ran_abort_msecs,
                                                     fbe_api_rdgen_peer_options_t peer_options,
                                                     fbe_bool_t b_stripe_align);

extern void fbe_test_io_wait_for_io_count(fbe_api_rdgen_context_t *context_p,
                                          fbe_u32_t num_tests,
                                          fbe_u32_t io_count,
                                          fbe_u32_t timeout_msecs);
extern void big_bird_wait_for_rgs_rb_logging(fbe_test_rg_configuration_t * const rg_config_p,
                                             fbe_u32_t raid_group_count,
                                             fbe_u32_t num_rebuilds);

/*!**************************************************************
 * fbe_test_io_wait_for_thread_count()
 ****************************************************************
 * @brief
 *  Wait for threads to all stop.
 *
 * @param thread_count - Number of rdgen threads to wait for.
 * @param timeout_msecs - Number of milliseconds to timeout after.
 * 
 * @return fbe_status_t
 * 
 * @return FBE_STATUS_OK if no errors were encountered.
 *
 * @author
 *  2/20/2013 - Created. Rob Foley
 *
 ****************************************************************/
void fbe_test_io_wait_for_thread_count(fbe_u32_t thread_count,
                                       fbe_u32_t timeout_msecs)
{
    fbe_status_t status;
    fbe_api_rdgen_get_stats_t stats;
    fbe_rdgen_filter_t filter;
    fbe_u32_t msecs = 0;

    fbe_api_rdgen_filter_init(&filter, 
                              FBE_RDGEN_FILTER_TYPE_INVALID, 
                              FBE_OBJECT_ID_INVALID, 
                              FBE_CLASS_ID_INVALID, 
                              FBE_PACKAGE_ID_INVALID,
                              0    /* edge index */);

    status = fbe_api_rdgen_get_stats(&stats, &filter);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Wait for all threads to stop.
     */
    while (stats.threads != 0)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_INFO, "waiting for I/Os to fail threads still running: %u\n",
                      stats.threads);
        fbe_api_sleep(500);
        /* We always increment and never reset this to zero so that our 
         * total wait time is not longer than timeout_msecs. 
         */
        msecs += 500;
        if (msecs > timeout_msecs)
        {
            fbe_api_trace(FBE_TRACE_LEVEL_INFO, "threads still running: %u ios: %u\n",
                          stats.threads, stats.io_count);
            MUT_FAIL_MSG("exceeded wait time");
        }
        status = fbe_api_rdgen_get_stats(&stats, &filter);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }
    return;
}
/**************************************
 * end fbe_test_io_wait_for_thread_count()
 **************************************/
/*!***************************************************************************
 *  gimli_set_reject_flags
 *****************************************************************************
 *
 * @brief   Run the rdgen test setup method passed against all luns for all
 *          raid groups specified.
 *
 * @param rg_config_p - The raid group configuration array under test.
 * @param context_p - Array of rdgen test contexts, one per LUN.
 * @param num_contexts - Total number of contexts in array.
 * @param client_flags
 * @param arrival flags
 * 
 * @return fbe_status_t
 *
 * @author
 *  2/19/2013 - Created. Rob Foley
 *
 *****************************************************************************/

fbe_status_t gimli_set_reject_flags(fbe_test_rg_configuration_t *rg_config_p,
                                       fbe_api_rdgen_context_t *context_p,
                                       fbe_u32_t num_contexts,
                                       fbe_u8_t client_flags,
                                       fbe_u8_t arrival_flags)
{
    fbe_status_t                status = FBE_STATUS_OK;
    fbe_test_rg_configuration_t *current_rg_config_p = NULL;
    fbe_api_rdgen_context_t     *current_context_p = NULL;
    fbe_u32_t                   rg_count = 0;
    fbe_u32_t                   rg_index = 0;
    fbe_u32_t                   lun_index = 0;
    fbe_u32_t                   total_luns_processed = 0;

    /* Get the number of raid groups to run I/O against
     */
    rg_count = fbe_test_get_rg_array_length(rg_config_p);

    /* Iterate over all the raid groups
     */
    current_rg_config_p = rg_config_p;
    current_context_p = context_p;
    for (rg_index = 0; 
         (rg_index < rg_count) && (status == FBE_STATUS_OK); 
         rg_index++)
    {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p))
        {
            current_rg_config_p++;
            continue;
        }
        /* For this raid group iterate over all the luns
         */
        for (lun_index = 0; lun_index < current_rg_config_p->number_of_luns_per_rg; lun_index++)
        {
            /* Validate that we haven't exceeded the number of context available.
             */
            if (total_luns_processed >= num_contexts) 
            {
                mut_printf(MUT_LOG_TEST_STATUS, 
                           "setup context for all luns: rg_config_p: 0x%p luns processed: %d exceeds contexts: %d",
                           rg_config_p, total_luns_processed, num_contexts); 
                return FBE_STATUS_GENERIC_FAILURE;
            }
            status = fbe_api_rdgen_io_specification_set_reject_flags(&current_context_p->start_io.specification,
                                                                     client_flags, arrival_flags);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

            /* Increment the number of luns processed and the test context pointer
             */
            total_luns_processed++;
            current_context_p++;

        } /* end for all luns in a raid group */

        /* Goto next raid group
         */
        current_rg_config_p++;

    } /* end for all raid groups configured */

    /* Else return success
     */
    return FBE_STATUS_OK;
}
/************************************************************
 * end gimli_set_reject_flags()
 ************************************************************/

/*!**************************************************************
 * big_bird_start_io_with_reject_flags()
 ****************************************************************
 * @brief
 *  Run I/O.
 *  Degrade the raid group.
 *  Allow I/O to continue degraded.
 *  Bring drive back and allow it to rebuild.
 *  Stop I/O.
 *
 * 
 * @param rg_config_p
 * @param context_p
 * @param threads - number of threads to pass to rdgen.
 * @param io_size_blocks - Max I/O size in blocks for rdgen.
 * @param rdgen_operation - opcode to pass to rdgen.
 * @param rdgen_operation
 * @param ran_abort_msecs
 * @param peer_options - To send this to peer or not.
 * @param client_flags
 * @param arrival_flags
 *
 * @return None.
 *
 * @author
 *  2/19/2013 - Created. Rob Foley
 *
 ****************************************************************/
void big_bird_start_io_with_reject_flags(fbe_test_rg_configuration_t * const rg_config_p,
                                         fbe_api_rdgen_context_t **context_p,
                                         fbe_u32_t threads,
                                         fbe_u32_t io_size_blocks,
                                         fbe_rdgen_operation_t rdgen_operation,
                                         fbe_test_random_abort_msec_t ran_abort_msecs,
                                         fbe_api_rdgen_peer_options_t peer_options,
                                         fbe_u8_t client_flags,
                                         fbe_u8_t arrival_flags,
                                         fbe_bool_t b_stripe_align)
{
    fbe_status_t status;
    fbe_u32_t num_luns;

    status = fbe_test_sep_io_get_num_luns(rg_config_p, &num_luns);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    status = fbe_test_sep_io_allocate_rdgen_test_context_for_all_luns(context_p, num_luns);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    status = big_bird_setup_lun_test_contexts(rg_config_p,
                                              *context_p,
                                              num_luns,
                                              rdgen_operation,
                                              threads,
                                              io_size_blocks, ran_abort_msecs, peer_options,
                                              b_stripe_align);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    gimli_set_reject_flags(rg_config_p,
                              *context_p,
                              num_luns,
                              client_flags,
                              arrival_flags);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    /* Run our I/O test.
     */
    status = fbe_api_rdgen_start_tests(*context_p, FBE_PACKAGE_ID_NEIT, num_luns);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    return;
}
/******************************************
 * end big_bird_start_io_with_reject_flags()
 ******************************************/

/*!**************************************************************
 * gimli_full_stripe_reject_flags_test()
 ****************************************************************
 * @brief
 *  Run a rebuild logging test where we exercise the code that
 *  allows raid to reject degraded writes that are not full stripes
 *  on parity raid groups.
 *
 * @param element_size - size in blocks of element.             
 *
 * @return None.
 *
 * @author
 *  2/19/2013 - Created. Rob Foley
 *
 ****************************************************************/

void gimli_full_stripe_reject_flags_test(fbe_test_rg_configuration_t * const rg_config_p,
                                            fbe_u32_t raid_group_count,
                                            fbe_u32_t luns_per_rg,
                                            fbe_element_size_t element_size,
                                            fbe_u32_t drives_to_remove,
                                            fbe_test_random_abort_msec_t ran_abort_msecs,
                                            fbe_api_rdgen_peer_options_t peer_options)
{
    fbe_api_rdgen_context_t *context_p = &gimli_test_contexts[0];
    fbe_status_t status;
    fbe_u32_t num_contexts = 1;

    status = fbe_test_sep_io_get_num_luns(rg_config_p, &num_contexts);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_TEST_STATUS, "== %s starting ==", __FUNCTION__);
    /* Run I/O degraded.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Starting heavy I/O. ==", __FUNCTION__);

    big_bird_start_io_with_reject_flags(rg_config_p, &context_p, FBE_U32_MAX, GIMLI_MAX_IO_SIZE_BLOCKS,
                                        FBE_RDGEN_OPERATION_WRITE_READ_CHECK, 
                                        ran_abort_msecs, peer_options, FBE_MEMORY_REJECT_ALLOWED_FLAG_PREFER_FULL_STRIPE,
                                        FBE_MEMORY_REJECT_ALLOWED_FLAG_PREFER_FULL_STRIPE,
                                        FBE_FALSE);

    /* Allow I/O to continue for a random time up to a second.
     */
    fbe_api_sleep((fbe_random() % gimli_io_msec_short) + EMCPAL_MINIMUM_TIMEOUT_MSECS);
    fbe_test_io_wait_for_io_count(context_p, num_contexts, GIMLI_WAIT_IO_COUNT, GIMLI_WAIT_MSEC);

    /* Pull drives.
     */
    big_bird_remove_all_drives(rg_config_p, raid_group_count, drives_to_remove,
                               FBE_TRUE, /* Yes we are pulling and reinserting the same drive*/
                               0, /* do not wait between removals */
                               FBE_DRIVE_REMOVAL_MODE_RANDOM);

    /* Sleep a random time from 1msec to 2 sec.
     * Half of the time we will not do a sleep here. 
     */
    if ((fbe_random() % 2) == 1)
    {
        fbe_api_sleep((fbe_random() % 2000) + 1);
    }
    /* This code tests that the rg get info code does the right things to return 
     * the rebuild logging status for these drive(s). 
     */
    big_bird_wait_for_rgs_rb_logging(rg_config_p, raid_group_count, drives_to_remove);

    /* Only parity fails for full stripe writes 
     */ 
    if ((rg_config_p->raid_type != FBE_RAID_GROUP_TYPE_RAID1) &&
        (rg_config_p->raid_type != FBE_RAID_GROUP_TYPE_RAID10)) 
    {
        /* Wait for all threads to fail.
        */
        fbe_test_io_wait_for_thread_count(0, GIMLI_WAIT_MSEC);
    }

    /* Stop I/O.
    */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s stopping I/O. ==", __FUNCTION__);
    status = fbe_api_rdgen_stop_tests(context_p, num_contexts);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s stopping I/O successful ==", __FUNCTION__);

    /* Make sure there were errors.
     * We expect every thread to get errors. 
     */
    if ((rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID1) ||
        (rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID10)) 
    {
        status = fbe_api_rdgen_context_check_io_status(context_p, num_contexts, FBE_STATUS_OK, 0, /* err count*/
                                                   FBE_FALSE,    /* Check io count */ FBE_RDGEN_OPERATION_STATUS_SUCCESS,
                                                   FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, 
                                                   FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }
    else
    {
        status = fbe_api_rdgen_context_check_io_status(context_p, num_contexts, FBE_STATUS_OK, 
                                                       context_p->start_io.specification.threads, /* err count */
                                                       FBE_FALSE,    /* Check io count */ FBE_RDGEN_OPERATION_STATUS_IO_ERROR,
                                                       FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED, 
                                                       FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NOT_FULL_STRIPE);
        /*! Rdgen has an issue where we will simply return io success for a failed operation on the peer. 
         * Given that we do not use this production code today, commenting out this check is not a big issue. 
         */
        //MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }

    /* Start I/O with one set of flags that should not get rejected.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Starting Flags 1 I/O. ==", __FUNCTION__);

    big_bird_start_io_with_reject_flags(rg_config_p, &context_p, FBE_U32_MAX, GIMLI_MAX_IO_SIZE_BLOCKS,
                                        FBE_RDGEN_OPERATION_WRITE_READ_CHECK, 
                                        ran_abort_msecs, peer_options, FBE_MEMORY_REJECT_ALLOWED_FLAG_PREFER_FULL_STRIPE,
                                        0, /* 0 to indicate SEP did not notify yet when this arrived. */
                                        FBE_FALSE /* don't stripe align */);
    fbe_test_io_wait_for_io_count(context_p, num_contexts, GIMLI_WAIT_IO_COUNT, GIMLI_WAIT_MSEC);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s stopping I/O. ==", __FUNCTION__);
    status = fbe_api_rdgen_stop_tests(context_p, num_contexts);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s stopping I/O successful ==", __FUNCTION__);

    /* Make sure there were no errors.
     */
    status = fbe_api_rdgen_context_check_io_status(context_p, num_contexts, FBE_STATUS_OK, 0, /* err count*/
                                                   FBE_FALSE,    /* Check io count */ FBE_RDGEN_OPERATION_STATUS_SUCCESS,
                                                   FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, 
                                                   FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        /* Start I/O with one set of flags that should not get rejected.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Starting Flags 2 I/O. ==", __FUNCTION__);

    big_bird_start_io_with_reject_flags(rg_config_p, &context_p, FBE_U32_MAX, GIMLI_MAX_IO_SIZE_BLOCKS,
                                        FBE_RDGEN_OPERATION_WRITE_READ_CHECK, 
                                        ran_abort_msecs, peer_options, 
                                        0, /* 0 to indicate the client does not allow rejection. */
                                        FBE_MEMORY_REJECT_ALLOWED_FLAG_PREFER_FULL_STRIPE,
                                        FBE_FALSE /* don't stripe align */);
    fbe_test_io_wait_for_io_count(context_p, num_contexts, GIMLI_WAIT_IO_COUNT, GIMLI_WAIT_MSEC);

    mut_printf(MUT_LOG_TEST_STATUS, "== %s stopping I/O. ==", __FUNCTION__);
    status = fbe_api_rdgen_stop_tests(context_p, num_contexts);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s stopping I/O successful ==", __FUNCTION__);

    /* Make sure there were no errors.
     */
    status = fbe_api_rdgen_context_check_io_status(context_p, num_contexts, FBE_STATUS_OK, 0, /* err count*/
                                                   FBE_FALSE,    /* Check io count */ FBE_RDGEN_OPERATION_STATUS_SUCCESS,
                                                   FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, 
                                                   FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Start full stripe I/Os with flags that say we could reject them. 
     * But since they are full stripe writes, they should not get rejected. 
     */
    big_bird_start_io_with_reject_flags(rg_config_p, &context_p, GIMLI_LIGHT_THREAD_COUNT, GIMLI_MAX_IO_SIZE_BLOCKS,
                                        FBE_RDGEN_OPERATION_WRITE_READ_CHECK, 
                                        ran_abort_msecs, peer_options, 
                                        FBE_MEMORY_REJECT_ALLOWED_FLAG_PREFER_FULL_STRIPE,
                                        FBE_MEMORY_REJECT_ALLOWED_FLAG_PREFER_FULL_STRIPE,
                                        FBE_TRUE /* stripe align */);
    /* Put the drives back in.
     */
    big_bird_insert_all_drives(rg_config_p, raid_group_count, drives_to_remove,
                               FBE_TRUE /* Yes we are pulling and reinserting the same drive*/);

    /* Make sure all objects are ready.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for all objects to become ready. ==", __FUNCTION__);
    fbe_test_sep_wait_for_rg_objects_ready_both_sps(rg_config_p);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for all objects to become ready. (successful)==", __FUNCTION__);

    /* Wait for all rebuilds to finish.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for rebuilds to finish. ==", __FUNCTION__);
    big_bird_wait_for_rebuilds(rg_config_p, raid_group_count, drives_to_remove);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for rebuilds to finish. (successful)==", __FUNCTION__);

    /* Stop I/O.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s stopping I/O. ==", __FUNCTION__);
    status = fbe_api_rdgen_stop_tests(context_p, num_contexts);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s stopping I/O successful ==", __FUNCTION__);

    /* Validate the status of I/Os.
     */
    fbe_test_sep_io_check_status(context_p, num_contexts, ran_abort_msecs);

    mut_printf(MUT_LOG_TEST_STATUS, "== %s complete ==", __FUNCTION__);

    status = fbe_test_sep_io_free_rdgen_test_context_for_all_luns(&context_p, num_contexts);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
}
/******************************************
 * end gimli_full_stripe_reject_flags_test()
 ******************************************/
/*!**************************************************************
 * gimli_not_preferred_reject_flags_test()
 ****************************************************************
 * @brief
 *  Run an SLF test where we check that I/Os are rejected
 *  when we enter SLF if the client supports this rejection.
 *
 * @param element_size - size in blocks of element.             
 *
 * @return None.
 *
 * @author
 *  2/21/2013 - Created. Rob Foley
 *
 ****************************************************************/

void gimli_not_preferred_reject_flags_test(fbe_test_rg_configuration_t * const rg_config_p,
                                              fbe_u32_t raid_group_count,
                                              fbe_u32_t luns_per_rg,
                                              fbe_element_size_t element_size,
                                              fbe_u32_t drives_to_remove,
                                              fbe_test_random_abort_msec_t ran_abort_msecs)
{
    fbe_api_rdgen_context_t *context_p = &gimli_test_contexts[0];
    fbe_status_t status;
    fbe_u32_t num_contexts = 1;
    fbe_api_rdgen_peer_options_t peer_options = FBE_API_RDGEN_PEER_OPTIONS_INVALID;

    status = fbe_test_sep_io_get_num_luns(rg_config_p, &num_contexts);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_TEST_STATUS, "== %s starting ==", __FUNCTION__);
    /* Start I/O before we start SLF.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Starting heavy I/O. ==", __FUNCTION__);

    big_bird_start_io_with_reject_flags(rg_config_p, &context_p, FBE_U32_MAX, GIMLI_MAX_IO_SIZE_BLOCKS,
                                        FBE_RDGEN_OPERATION_WRITE_READ_CHECK, 
                                        ran_abort_msecs, peer_options, FBE_MEMORY_REJECT_ALLOWED_FLAG_NOT_PREFERRED,
                                        FBE_MEMORY_REJECT_ALLOWED_FLAG_NOT_PREFERRED,
                                        FBE_FALSE);

    /* Allow I/O to continue for a random time up to a second.
     */
    fbe_api_sleep((fbe_random() % gimli_io_msec_short) + EMCPAL_MINIMUM_TIMEOUT_MSECS);
    fbe_test_io_wait_for_io_count(context_p, num_contexts, GIMLI_WAIT_IO_COUNT, GIMLI_WAIT_MSEC);

    /* Pull drives locally only.
     */
    fbe_test_sep_util_set_remove_on_both_sps_mode(FBE_FALSE);
    big_bird_remove_all_drives(rg_config_p, raid_group_count, drives_to_remove,
                               FBE_TRUE, /* Yes we are pulling and reinserting the same drive*/
                               0, /* do not wait between removals */
                               FBE_DRIVE_REMOVAL_MODE_RANDOM);

    /* Wait for all threads to fail.
     */
    fbe_test_io_wait_for_thread_count(0, GIMLI_WAIT_MSEC);

    /* Stop I/O.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s stopping I/O. ==", __FUNCTION__);
    status = fbe_api_rdgen_stop_tests(context_p, num_contexts);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s stopping I/O successful ==", __FUNCTION__);

    /* Make sure there were errors.
     * We expect every thread to get errors. 
     */
    status = fbe_api_rdgen_context_check_io_status(context_p, num_contexts, FBE_STATUS_OK, 
                                                   context_p->start_io.specification.threads,    /* err count */
                                                   FBE_FALSE,    /* Check io count */ FBE_RDGEN_OPERATION_STATUS_IO_ERROR,
                                                   FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED, 
                                                   FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NOT_PREFERRED);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Start I/O with one set of flags that should not get rejected.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Starting Flags 1 I/O. ==", __FUNCTION__);

    big_bird_start_io_with_reject_flags(rg_config_p, &context_p, FBE_U32_MAX, GIMLI_MAX_IO_SIZE_BLOCKS,
                                        FBE_RDGEN_OPERATION_WRITE_READ_CHECK, 
                                        ran_abort_msecs, peer_options, FBE_MEMORY_REJECT_ALLOWED_FLAG_NOT_PREFERRED,
                                        0, /* 0 to indicate SEP did not notify yet when this arrived. */
                                        FBE_FALSE /* don't stripe align */);
    fbe_test_io_wait_for_io_count(context_p, num_contexts, GIMLI_WAIT_IO_COUNT, GIMLI_WAIT_MSEC);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s stopping I/O. ==", __FUNCTION__);
    status = fbe_api_rdgen_stop_tests(context_p, num_contexts);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s stopping I/O successful ==", __FUNCTION__);

    /* Make sure there were no errors.
     */
    status = fbe_api_rdgen_context_check_io_status(context_p, num_contexts, FBE_STATUS_OK, 0, /* err count*/
                                                   FBE_FALSE,    /* Check io count */ FBE_RDGEN_OPERATION_STATUS_SUCCESS,
                                                   FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, 
                                                   FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    /* Start I/O with one set of flags that should not get rejected.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Starting Flags 2 I/O. ==", __FUNCTION__);

    big_bird_start_io_with_reject_flags(rg_config_p, &context_p, FBE_U32_MAX, GIMLI_MAX_IO_SIZE_BLOCKS,
                                        FBE_RDGEN_OPERATION_WRITE_READ_CHECK, 
                                        ran_abort_msecs, peer_options, 
                                        0, /* 0 to indicate the client does not allow rejection. */
                                        FBE_MEMORY_REJECT_ALLOWED_FLAG_NOT_PREFERRED,
                                        FBE_FALSE /* don't stripe align */);
    fbe_test_io_wait_for_io_count(context_p, num_contexts, GIMLI_WAIT_IO_COUNT, GIMLI_WAIT_MSEC);

    mut_printf(MUT_LOG_TEST_STATUS, "== %s stopping I/O. ==", __FUNCTION__);
    status = fbe_api_rdgen_stop_tests(context_p, num_contexts);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s stopping I/O successful ==", __FUNCTION__);

    /* Make sure there were no errors.
     */
    status = fbe_api_rdgen_context_check_io_status(context_p, num_contexts, FBE_STATUS_OK, 0, /* err count*/
                                                   FBE_FALSE,    /* Check io count */ FBE_RDGEN_OPERATION_STATUS_SUCCESS,
                                                   FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, 
                                                   FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Put the drives back in.
     */
    big_bird_insert_all_drives(rg_config_p, raid_group_count, drives_to_remove,
                               FBE_TRUE /* Yes we are pulling and reinserting the same drive*/);

    /* Make sure all objects are ready.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for all objects to become ready. ==", __FUNCTION__);
    fbe_test_sep_wait_for_rg_objects_ready_both_sps(rg_config_p);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Waiting for all objects to become ready. (successful)==", __FUNCTION__);

    /* This was just SLF, so there are no rebuilds to wait for.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "== %s complete ==", __FUNCTION__);

    status = fbe_test_sep_io_free_rdgen_test_context_for_all_luns(&context_p, num_contexts);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    fbe_test_sep_util_set_remove_on_both_sps_mode(FBE_TRUE);
}
/******************************************
 * end gimli_not_preferred_reject_flags_test()
 ******************************************/

/*!**************************************************************
 * gimli_run_tests()
 ****************************************************************
 * @brief
 *  Run the set of tests that make up the big bird test.
 *
 * @param rg_config_p - Config to create.
 * @param drive_to_remove_p - Num drives to remove. 
 *
 * @return none
 *
 * @author
 *  2/21/2013 - Created. Rob Foley
 *
 ****************************************************************/
void gimli_run_tests(fbe_test_rg_configuration_t *rg_config_p, void * drive_to_remove_p)
{
    fbe_u32_t drives_to_remove = (fbe_u32_t)drive_to_remove_p;
    fbe_element_size_t element_size;
    fbe_api_rdgen_peer_options_t peer_options = FBE_API_RDGEN_PEER_OPTIONS_INVALID;
    fbe_u32_t luns_per_rg = GIMLI_TEST_LUNS_PER_RAID_GROUP;
    fbe_u32_t raid_group_count =  fbe_test_get_rg_array_length(rg_config_p);
    fbe_u32_t saved_width;
    fbe_u32_t test_level = fbe_sep_test_util_get_raid_testing_extended_level();

    /* We need to wait for initial verify complete so that raid group will not go into
     * quiesced mode to update verify checkpoint.
     */
    fbe_test_sep_util_wait_for_initial_verify(rg_config_p);

    if (rg_config_p->b_bandwidth)
    {
        element_size = FBE_RAID_SECTORS_PER_ELEMENT_BANDWIDTH;
    }
    else
    {
        element_size = FBE_RAID_SECTORS_PER_ELEMENT;
    }

    if (fbe_test_sep_util_get_dualsp_test_mode())
    {
        fbe_u32_t random_number = fbe_random() % 2;
        if (random_number == 0)
        {
            peer_options = FBE_API_RDGEN_PEER_OPTIONS_SEND_THRU_PEER;
        }
        else
        {
            peer_options = FBE_API_RDGEN_PEER_OPTIONS_LOAD_BALANCE;
        }
        mut_printf(MUT_LOG_TEST_STATUS, "%s testing peer_options %d", __FUNCTION__, peer_options);
    }

    /* Write the background pattern to seed this element size.
     */
    big_bird_write_background_pattern();

    /* Test just the parity groups.  The parity has different behavior from mirrors.
     */
    saved_width = rg_config_p[GIMLI_PARITY_GROUPS].width;
    rg_config_p[GIMLI_PARITY_GROUPS].width = FBE_U32_MAX;
    gimli_full_stripe_reject_flags_test(rg_config_p, GIMLI_PARITY_GROUPS, luns_per_rg,
                                        element_size, drives_to_remove, 
                                        FBE_TEST_RANDOM_ABORT_TIME_INVALID,
                                        peer_options);
    rg_config_p[GIMLI_PARITY_GROUPS].width = saved_width;

    /* Only when test level > 0, we test the mirror rg */
    if (test_level > 0) {
        /* Test just the mirror groups.  The mirror groups have different test code from the 
         * parity groups. 
         */
        saved_width = rg_config_p[GIMLI_PARITY_GROUPS + GIMLI_MIRROR_GROUPS].width;
        rg_config_p[GIMLI_PARITY_GROUPS + GIMLI_MIRROR_GROUPS].width = FBE_U32_MAX;
        gimli_full_stripe_reject_flags_test(&rg_config_p[GIMLI_PARITY_GROUPS], GIMLI_MIRROR_GROUPS, luns_per_rg,
                                            element_size, drives_to_remove, 
                                            FBE_TEST_RANDOM_ABORT_TIME_INVALID,
                                            peer_options);
        rg_config_p[GIMLI_PARITY_GROUPS + GIMLI_MIRROR_GROUPS].width = saved_width;
    }


    /* Run a test where we enter SLF and make sure we properly reject the I/Os.
     * Since SLF requires two SPs, it only makes sense with a dual sp test. 
     * This intentionally tests all raid types including R0. 
     */
    if (fbe_test_sep_util_get_dualsp_test_mode())
    {
        gimli_not_preferred_reject_flags_test(rg_config_p, raid_group_count, luns_per_rg,
                                              element_size, drives_to_remove, 
                                              FBE_TEST_RANDOM_ABORT_TIME_INVALID);
    }
}   
/******************************************
 * end gimli_run_tests()
 ******************************************/

/*!**************************************************************
 * gimli_test()
 ****************************************************************
 * @brief
 *  Run the tests that exercise the code that allows raid to
 *  reject I/Os if the client allows it.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  2/21/2013 - Created. Rob Foley
 *
 ****************************************************************/

void gimli_test(void)
{
    fbe_test_rg_configuration_t *rg_config_p = NULL;
    fbe_u32_t test_level = fbe_sep_test_util_get_raid_testing_extended_level();

    /* Based on the test level determine which configuration to use.
     */
    if (test_level > 0)
    {

        /* Run test for normal element size.
         */
        mut_printf(MUT_LOG_TEST_STATUS, "%s testing standard element size", __FUNCTION__);
        rg_config_p = &gimli_raid_group_config_extended[0];
    }
    else
    {
        rg_config_p = &gimli_raid_group_config_qual[0];

    }

    fbe_test_run_test_on_rg_config_with_extra_disk(rg_config_p, (void*)1, /* drives to remove */
                                                   gimli_run_tests,
                                                   GIMLI_TEST_LUNS_PER_RAID_GROUP,
                                                   GIMLI_CHUNKS_PER_LUN);
    return;
}
/******************************************
 * end gimli_test()
 ******************************************/
/*!**************************************************************
 * gimli_setup()
 ****************************************************************
 * @brief
 *  Setup for a raid 5 I/O test.
 *
 * @param None.               
 *
 * @return None.   
 *
 * @author
 *  2/21/2013 - Created. Rob Foley
 *
 ****************************************************************/
void gimli_setup(void)
{
    /* Only load the physical config in simulation.
    */
    if (fbe_test_util_is_simulation())
    {
        fbe_test_rg_configuration_t *rg_config_p = NULL;
        fbe_u32_t test_level = fbe_sep_test_util_get_raid_testing_extended_level();
        fbe_u32_t num_raid_groups;

        /* Based on the test level determine which configuration to use.
        */
        if (test_level > 0)
        {
            /* Run test for normal element size.
            */
            rg_config_p = &gimli_raid_group_config_extended[0];
        }
        else
        {
            rg_config_p = &gimli_raid_group_config_qual[0];
        }
        fbe_test_sep_util_init_rg_configuration_array(rg_config_p);
        num_raid_groups = fbe_test_get_rg_array_length(rg_config_p);
        fbe_test_sep_util_randomize_rg_configuration_array(rg_config_p);

        /* initialize the number of extra drive required by each rg 
          */
        fbe_test_sep_util_populate_rg_num_extra_drives(rg_config_p);

        elmo_create_physical_config_for_rg(rg_config_p, num_raid_groups);
        elmo_load_sep_and_neit();
    }

    /* This test will pull a drive out and insert a new drive with configure extra drive as spare
     * Set the spare trigger timer 1 second to swap in immediately.
     */
    fbe_test_sep_util_update_permanent_spare_trigger_timer(1/* 1 second */);

    /* Initialize any required fields and perform cleanup if required
     */
    fbe_test_common_util_test_setup_init();
    fbe_test_sep_util_disable_system_drive_zeroing();
    return;
}
/**************************************
 * end gimli_setup()
 **************************************/

/*!**************************************************************
 * gimli_cleanup()
 ****************************************************************
 * @brief
 *  Cleanup the test.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  2/21/2013 - Created. Rob Foley
 *
 ****************************************************************/

void gimli_cleanup(void)
{
    mut_printf(MUT_LOG_HIGH, "%s entry", __FUNCTION__);
    if (fbe_test_util_is_simulation())
    {
        fbe_test_sep_util_destroy_neit_sep_physical();
    }
    return;
}
/******************************************
 * end gimli_cleanup()
 ******************************************/

void gimli_dualsp_test(void)
{
    gimli_test();
}
/*!**************************************************************
 * gimli_dualsp_setup()
 ****************************************************************
 * @brief
 *  Setup for a raid 5 degraded I/O test.
 *
 * @param   None.               
 *
 * @return  None.   
 *
 * @note    Must run in dual-sp mode
 *
 * @author
 *  2/21/2013 - Created. Rob Foley
 *
 ****************************************************************/
void gimli_dualsp_setup(void)
{
    fbe_u32_t CSX_MAYBE_UNUSED test_level = fbe_sep_test_util_get_raid_testing_extended_level();

    if (fbe_test_util_is_simulation())
    { 
        fbe_test_rg_configuration_t *rg_config_p = NULL;
        fbe_u32_t test_level = fbe_sep_test_util_get_raid_testing_extended_level();
        fbe_u32_t num_raid_groups;

        /* Based on the test level determine which configuration to use.
         */
        if (test_level > 0)
        {
            /* Run test for normal element size.
             */
            rg_config_p = &gimli_raid_group_config_extended[0];
        }
        else
        {
            rg_config_p = &gimli_raid_group_config_qual[0];
        }
        fbe_test_sep_util_init_rg_configuration_array(rg_config_p);
        num_raid_groups = fbe_test_get_rg_array_length(rg_config_p);
        fbe_test_sep_util_randomize_rg_configuration_array(rg_config_p);

        /* initialize the number of extra drive required by each rg which will be use by
            simulation test and hardware test */
        fbe_test_sep_util_populate_rg_num_extra_drives(rg_config_p);

        /* Instantiate the drives on SP A
         */
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
        elmo_create_physical_config_for_rg(rg_config_p, num_raid_groups);
       
        /*! Currently the terminator doesn't automatically instantiate the drives
         *  on both SPs.  Therefore instantiate the drives on SP B also
         */
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
        elmo_create_physical_config_for_rg(rg_config_p, num_raid_groups);

        sep_config_load_sep_and_neit_load_balance_both_sps();
    }

    /* This test will pull a drive out and insert a new drive with configure extra drive as spare
     * Set the spare trigger timer 1 second to swap in immediately.
     */
    fbe_test_sep_util_update_permanent_spare_trigger_timer(1/* 1 second */);

    /* Initialize any required fields and perform cleanup if required
     */
    fbe_test_common_util_test_setup_init();
    fbe_test_disable_background_ops_system_drives();
    
    fbe_test_sep_util_set_dualsp_test_mode(FBE_TRUE);
    return;
}
/******************************************
 * end gimli_dualsp_setup()
 ******************************************/

/*!**************************************************************
 * gimli_dualsp_cleanup()
 ****************************************************************
 * @brief
 *  Cleanup the test.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  2/21/2013 - Created. Rob Foley
 *
 ****************************************************************/

void gimli_dualsp_cleanup(void)
{
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

    return;
}
/******************************************
 * end gimli_dualsp_cleanup()
 ******************************************/
/*************************
 * end file gimli_test.c
 *************************/


