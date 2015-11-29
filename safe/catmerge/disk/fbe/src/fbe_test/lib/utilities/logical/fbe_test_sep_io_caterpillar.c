/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_test_sep_io_caterpillar.c
 ***************************************************************************
 *
 * @brief
 *  This file contains SEP Test for Caterpillar I/O.
 * 
 *
 ***************************************************************************/


/*************************
 *   INCLUDE FILES
 *************************/
#include "sep_tests.h"
#include "fbe_test_common_utils.h"
#include "sep_utils.h"
#include "sep_test_io.h"
#include "sep_test_io_private.h"

/*****************************************
 * DEFINITIONS
 *****************************************/


/*************************
 *   GLOBALS
 *************************/

/*!*******************************************************************
 * @var fbe_test_sep_io_caterpillar_pass_count
 *********************************************************************
 * @brief Default number of caterpillar passes we will perform
 *
 *********************************************************************/
static fbe_u32_t fbe_test_sep_io_caterpillar_pass_count = 1;

/*!*******************************************************************
 * @def FBE_TEST_SEP_IOS_MAX_CATERPILLAR_TIME_MS
 *********************************************************************
 * @brief Limit each caterpillar run to this number of milliseconds.
 *
 *********************************************************************/
#define FBE_TEST_SEP_IOS_MAX_CATERPILLAR_TIME_MS 5000

/*!*******************************************************************
 * @def FBE_TEST_SEP_IOS_MAX_CATERPILLAR_TIME_EXTENDED_MS
 *********************************************************************
 * @brief Limit each caterpillar run to this number of milliseconds at
 *        raid test level 1.
 *
 *********************************************************************/
#define FBE_TEST_SEP_IOS_MAX_CATERPILLAR_TIME_EXTENDED_MS 10000

/*************************
 *   FUNCTIONS
 *************************/

/*!***************************************************************************
 * fbe_test_sep_io_setup_caterpillar_rdgen_test_context()
 *****************************************************************************
 * @brief
 *  Setup the context structure to run a caterpillar I/O test against a single
 *  object.
 *
 * @param context_p - Context of the operation.
 * @param object_id - object id to run io against.
 * @param rdgen_operation - operation to start.
 * @param capacity - capacity in blocks.
 * @param max_passes - Number of passes to execute.
 * @param io_size_blocks - io size to fill with in blocks.
 * @param b_random - Must be FBE_FALSE
 * @param b_increasing - FBE_TRUE - Increasing sequential requests
 *                       FBE_FALSE - Decreasing sequential requests
 * @param b_inject_random_aborts - FBE_TRUE - Inject random aborts using the
 *          configured random abort msecs value
 * @param b_dualsp_io - Run dualsp I/O using the previously configured peer
 *          options
 * 
 * @return fbe_status_t
 *
 * @note    Caterpillar to all objects of a class is not supported.
 *
 * @author
 *  9/3/2009 - Created. Rob Foley
 *
 *****************************************************************************/
fbe_status_t fbe_test_sep_io_setup_caterpillar_rdgen_test_context(fbe_api_rdgen_context_t *context_p,
                                                                  fbe_object_id_t object_id,
                                                                  fbe_rdgen_operation_t rdgen_operation,
                                                                  fbe_lba_t capacity,
                                                                  fbe_u32_t max_passes,
                                                                  fbe_u32_t threads,
                                                                  fbe_u64_t io_size_blocks,
                                                                  fbe_bool_t b_random,
                                                                  fbe_bool_t b_increasing,
                                                                  fbe_bool_t b_inject_random_aborts,
                                                                  fbe_bool_t b_dualsp_io)
{
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_u32_t                       io_seconds;
    fbe_time_t                      io_time;
    fbe_rdgen_lba_specification_t   lba_specification = FBE_RDGEN_LBA_SPEC_CATERPILLAR_INCREASING; 
    fbe_u32_t raid_test_level = fbe_sep_test_util_get_raid_testing_extended_level();

    /* Only sequential is supported with caterpillar
     */
    if (b_random == FBE_TRUE)
    {
        mut_printf(MUT_LOG_TEST_STATUS, 
                   "%s b_random: %d is TRUE.  Caterpillar only supports sequential I/O", 
                   __FUNCTION__, b_random);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* First determine if a run time is enabled or not.  If it is, it overrides
     * the maximum number of passes.
     */
    io_seconds = fbe_test_sep_io_get_io_seconds();
    io_time = io_seconds * FBE_TIME_MILLISECONDS_PER_SECOND;
    if (io_seconds > 0)
    {
        mut_printf(MUT_LOG_TEST_STATUS, 
                   "%s using I/O seconds of %d", 
                   __FUNCTION__, io_seconds);
        max_passes = 0;
    }
    else
    {
        /* Caterpillar I/O takes a long time.  Limit this time in test level 0.
         * If the user specified an io seconds, we will just use that. 
         */ 
        if (raid_test_level == 0)
        {
            io_time = FBE_TEST_SEP_IOS_MAX_CATERPILLAR_TIME_MS;
        }
        else
        {
            io_time = FBE_TEST_SEP_IOS_MAX_CATERPILLAR_TIME_EXTENDED_MS;
        }
    }

    /* Default is sequential increasing, change to decreasing if requested
     */
    if (b_increasing == FBE_FALSE)
    {
        lba_specification = FBE_RDGEN_LBA_SPEC_CATERPILLAR_DECREASING;
    }

    
    /*! Now configure the caterpillar rdgen test context values
     *
     *  @todo Add abort and peer options to the context setup
     */
    status = fbe_api_rdgen_test_context_init(context_p, 
                                             object_id, 
                                             FBE_CLASS_ID_INVALID,
                                             FBE_PACKAGE_ID_SEP_0,
                                             rdgen_operation,
                                             FBE_RDGEN_PATTERN_LBA_PASS,    /* Standard rdgen data pattern */
                                             max_passes,
                                             0,                             /* io count not used */
                                             io_time,
                                             threads,
                                             lba_specification,             /* Caterpillar I/O mode is sequential */
                                             0,                             /* Start lba*/
                                             0,                             /* Min lba */
                                             capacity,                      /* max lba */
                                             FBE_RDGEN_BLOCK_SPEC_RANDOM,   /* Standard I/O transfer size is random */
                                             1,                             /* Min blocks per request */
                                             io_size_blocks                 /* Max blocks per request */ );
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* If the injection of random aborts is enabled update the context.
     */
    if (b_inject_random_aborts == FBE_TRUE)
    {
        fbe_test_random_abort_msec_t    rdgen_abort_msecs;

        /*! @note Assumes that rdgen abort msecs has been configured
         */
        rdgen_abort_msecs = fbe_test_sep_io_get_configured_rdgen_abort_msecs();
        if (rdgen_abort_msecs != FBE_TEST_RANDOM_ABORT_TIME_INVALID)
        {
            status = fbe_api_rdgen_set_random_aborts(&context_p->start_io.specification,
                                                     rdgen_abort_msecs);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
            status = fbe_api_rdgen_io_specification_set_options(&context_p->start_io.specification, 
                                                                FBE_RDGEN_OPTIONS_CONTINUE_ON_ERROR);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        }
        else
        {
            /* Else if random aborts are enabled they should have been configured
             */
            status = FBE_STATUS_GENERIC_FAILURE;
            mut_printf(MUT_LOG_TEST_STATUS, "%s Need to configure abort mode to enable aborts", __FUNCTION__);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        }
    }

    /* If peer I/O is enabled update the context
     */
    if (b_dualsp_io == FBE_TRUE)
    {
        fbe_api_rdgen_peer_options_t    rdgen_peer_options;

        /*! @note Assumes that rdgen peer options has been configured
         */
        rdgen_peer_options = fbe_test_sep_io_get_configured_rdgen_peer_options();
        if (rdgen_peer_options != FBE_API_RDGEN_PEER_OPTIONS_INVALID)
        {
            status = fbe_api_rdgen_io_specification_set_peer_options(&context_p->start_io.specification,
                                                                     rdgen_peer_options);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        }
        else
        {
            /* If peer I/O is enabled, it should have been previously configured
             */
            status = FBE_STATUS_GENERIC_FAILURE;
            mut_printf(MUT_LOG_TEST_STATUS, "%s Need to configure peer options to enable peer I/O", __FUNCTION__);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        }
    }

    return FBE_STATUS_OK;
}
/************************************************************
 * end fbe_test_sep_io_setup_caterpillar_rdgen_test_context()
 ************************************************************/

/*!**************************************************************
 * fbe_test_sep_io_caterpillar_get_small_io_count()
 ****************************************************************
 * @brief
 *  Return the number of I/Os to perform during this test.
 *
 * @param - None          
 *
 * @return - Number of I/Os that we should perform in this test.
 *
 ****************************************************************/

fbe_u32_t fbe_test_sep_io_caterpillar_get_small_io_count(void)
{
    /* Start with the default above.
     */
    fbe_u32_t io_count;

    io_count = fbe_test_sep_io_get_small_io_count(fbe_test_sep_io_caterpillar_pass_count);

    return io_count;
}
/******************************************************
 * end fbe_test_sep_io_caterpillar_get_small_io_count()
 ******************************************************/

/*!**************************************************************
 * fbe_test_sep_io_caterpillar_get_large_io_count()
 ****************************************************************
 * @brief
 *  Return the number of large I/Os to perform during this test.
 *
 * @param - default_io_count - default io count to use if parameter
 *                             is not set.              
 *
 * @return - Number of large I/Os that we should perform in this test.
 *
 ****************************************************************/

fbe_u32_t fbe_test_sep_io_caterpillar_get_large_io_count(void)
{
    /* Start with the default above.
     */
    fbe_u32_t io_count;

    io_count = fbe_test_sep_io_get_large_io_count(fbe_test_sep_io_caterpillar_pass_count);
    return io_count;
}
/******************************************************
 * end fbe_test_sep_io_caterpillar_get_large_io_count()
 ******************************************************/

/*!***************************************************************************
 *          fbe_test_sep_io_run_caterpillar_sequence()
 *****************************************************************************
 *
 * @brief   Run the caterpillar I/O (a.k.a overlapping I/O) sequence against 
 *          all the non-private LUNs
 *
 * @param   sequence_config_p - Pointer to configuration for this test sequence
 *
 * @return  status - Determine if test passed or not
 *
 * @author
 *  03/31/2011  Ron Proulx  - Created
 *
 ****************************************************************/
fbe_status_t fbe_test_sep_io_run_caterpillar_sequence(fbe_test_sep_io_sequence_config_t *sequence_config_p)
{
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_test_sep_io_sequence_state_t    sequence_io_state;
    fbe_api_rdgen_context_t            *context_p = NULL;
    fbe_u32_t                           number_of_luns = 0;

    /* First validate that the test sequence has been configured
     */
    sequence_io_state = fbe_test_sep_io_get_sequence_io_state(sequence_config_p);
    MUT_ASSERT_TRUE(sequence_io_state >= FBE_TEST_SEP_IO_SEQUENCE_STATE_IO_READY);

    status = fbe_test_sep_io_get_num_luns(sequence_config_p->rg_config_p, &number_of_luns);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* First allocate the required rdgen test contexts
     */
    status = fbe_test_sep_io_allocate_rdgen_test_context_for_all_luns(&context_p, number_of_luns);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Step 1)  Execute a write/read/compare I/O for small I/O
     *
     */
    mut_printf(MUT_LOG_TEST_STATUS, "%s Executing write/read/compare for all LUNs. ", 
               __FUNCTION__);
    status = fbe_test_sep_io_setup_rdgen_test_context_for_all_luns(sequence_config_p->rg_config_p,
                                              context_p,
                                              number_of_luns,
                                              FBE_RDGEN_OPERATION_WRITE_READ_CHECK,
                                              sequence_config_p->small_io_count, /* passes */
                                              sequence_config_p->io_thread_count, /* threads */
                                              sequence_config_p->max_blocks_for_small_io,
                                              FBE_FALSE, /* Must be FALSE for caterpillar */
                                              FBE_TRUE, /* Run I/O increasing */
                                              sequence_config_p->abort_enabled,
                                              sequence_config_p->dualsp_enabled,
                                              fbe_test_sep_io_setup_caterpillar_rdgen_test_context);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Run our I/O test.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "%s Running write/read/compare small I/Os to all LUNs(%d) max blocks: %llu  passes: %d", 
               __FUNCTION__, number_of_luns,
	       (unsigned long long)sequence_config_p->max_blocks_for_small_io,
	       sequence_config_p->small_io_count );
    status = fbe_test_sep_io_run_rdgen_tests(context_p, FBE_PACKAGE_ID_NEIT, number_of_luns, sequence_config_p->rg_config_p);

    /* Make sure there were no errors.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "%s Running write/read/compare small I/Os to all LUNs...Finished", __FUNCTION__);
    fbe_test_sep_io_validate_status(status, context_p, sequence_config_p->abort_enabled);

    /* Step 2) Verify/Write/Read/Compare for small I/O
     */
    status = fbe_test_sep_io_setup_rdgen_test_context_for_all_luns(sequence_config_p->rg_config_p,
                                              context_p,
                                              number_of_luns,
                                              FBE_RDGEN_OPERATION_VERIFY_WRITE_READ_CHECK,
                                              1, /* Num passes. */
                                              2, /* threads */
                                              sequence_config_p->max_blocks_for_small_io,
                                              FBE_FALSE, /* Must be FALSE for caterpillar */
                                              FBE_FALSE, /* Run I/O decreasing */
                                              sequence_config_p->abort_enabled,
                                              sequence_config_p->dualsp_enabled,
                                              fbe_test_sep_io_setup_caterpillar_rdgen_test_context);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Run our I/O test.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "%s Running verify-write/read/compare small I/Os to all LUNs(%d). max blocks: %llu  passes: %d", 
               __FUNCTION__, number_of_luns,
	       (unsigned long long)sequence_config_p->max_blocks_for_small_io, 1);
    status = fbe_test_sep_io_run_rdgen_tests(context_p, FBE_PACKAGE_ID_NEIT, number_of_luns, sequence_config_p->rg_config_p);

    /* Make sure there were no errors.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "%s Running verify-write/read/compare small I/Os to all LUNs...Finished", __FUNCTION__);
    fbe_test_sep_io_validate_status(status, context_p, sequence_config_p->abort_enabled);

    /* Step 3) Verify/Write/Read/Compare for large I/O
     */
    status = fbe_test_sep_io_setup_rdgen_test_context_for_all_luns(sequence_config_p->rg_config_p,
                                              context_p,
                                              number_of_luns,
                                              FBE_RDGEN_OPERATION_VERIFY_WRITE_READ_CHECK,
                                              1, /* I/O passes. */
                                              2, /* threads */
                                              sequence_config_p->max_blocks_for_large_io,
                                              FBE_FALSE, /* Must be FALSE for caterpillar */
                                              FBE_TRUE, /* Run I/O increasing */
                                              sequence_config_p->abort_enabled,
                                              sequence_config_p->dualsp_enabled,
                                              fbe_test_sep_io_setup_caterpillar_rdgen_test_context);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Run our I/O test.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "%s Running verify-write/read/compare large I/Os to all LUNs(%d). max blocks: %llu  passes: %d", 
               __FUNCTION__, number_of_luns,
	       (unsigned long long)sequence_config_p->max_blocks_for_large_io, 1);
    status = fbe_test_sep_io_run_rdgen_tests(context_p, FBE_PACKAGE_ID_NEIT, number_of_luns, sequence_config_p->rg_config_p);

    /* Make sure there were no errors.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "%s Running verify-write/read/compare large I/Os to all LUNs...Finished", __FUNCTION__);
    fbe_test_sep_io_validate_status(status, context_p, sequence_config_p->abort_enabled);

    /* Step 4)  Execute a write/read/compare I/O for large I/O
     */
    status = fbe_test_sep_io_setup_rdgen_test_context_for_all_luns(sequence_config_p->rg_config_p,
                                              context_p,
                                              number_of_luns,
                                              FBE_RDGEN_OPERATION_WRITE_READ_CHECK,
                                              sequence_config_p->large_io_count, /* passes */
                                              sequence_config_p->io_thread_count, /* threads */
                                              sequence_config_p->max_blocks_for_large_io,
                                              FBE_FALSE, /* Must be FALSE for caterpillar */
                                              FBE_FALSE, /* Run I/O decreasing */
                                              sequence_config_p->abort_enabled,
                                              sequence_config_p->dualsp_enabled,
                                              fbe_test_sep_io_setup_caterpillar_rdgen_test_context);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Run our I/O test.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "%s Running write/read/compare large I/Os to all LUNs(%d). max blocks: %llu  passes: %d", 
               __FUNCTION__, number_of_luns,
	       (unsigned long long)sequence_config_p->max_blocks_for_large_io,
	       sequence_config_p->large_io_count);
    status = fbe_test_sep_io_run_rdgen_tests(context_p, FBE_PACKAGE_ID_NEIT, number_of_luns, sequence_config_p->rg_config_p);

    /* Make sure there were no errors.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "%s Running write/read/compare large I/Os to all LUNs...Finished", __FUNCTION__);
    fbe_test_sep_io_validate_status(status, context_p, sequence_config_p->abort_enabled);

    /* Now free the previously allocated rdgen test contexts
     */
    status = fbe_test_sep_io_free_rdgen_test_context_for_all_luns(&context_p, number_of_luns);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    return FBE_STATUS_OK;
}
/***********************************************
 * end fbe_test_sep_io_run_caterpillar_sequence()
 ***********************************************/


/*******************************************
 * end of file fbe_test_sep_io_caterpillar.c
 *******************************************/

