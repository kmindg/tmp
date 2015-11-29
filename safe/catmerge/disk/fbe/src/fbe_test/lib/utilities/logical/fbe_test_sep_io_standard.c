/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_test_sep_io_standard.c
 ***************************************************************************
 *
 * @brief
 *  This file contains utility functions for use with the Storage Extent 
 *  Package (SEP).
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
/* Fixed operation globals
 */
static fbe_bool_t           fbe_test_sep_io_fixed_io_enabled = FBE_FALSE;
static fbe_lba_t            fbe_test_sep_io_fixed_io_lba = FBE_LBA_INVALID;
static fbe_block_count_t    fbe_test_sep_io_fixed_io_blocks = 0;
static fbe_rdgen_operation_t fbe_test_sep_io_fixed_io_operation = FBE_RDGEN_OPERATION_WRITE_ONLY;

/*************************
 *   FUNCTIONS
 *************************/

/*!***************************************************************************
 * fbe_test_sep_io_setup_standard_rdgen_test_context()
 *****************************************************************************
 * @brief
 *  Setup the context structure to run a random I/O test.
 *
 * @param context_p - Context of the operation.
 * @param object_id - object id to run io against.
 * @param class_id - class id to test.
 * @param rdgen_operation - operation to start.
 * @param capacity - capacity in blocks.
 * @param max_passes - Number of passes to execute.
 * @param io_size_blocks - io size to fill with in blocks.
 * @param b_inject_random_aborts - FBE_TRUE - Inject random aborts using the
 *          configured random abort msecs value
 * @param b_dualsp_io - Run dualsp I/O using the previously configured peer
 *          options
 * 
 * @return fbe_status_t
 *
 * @author
 *  9/3/2009 - Created. Rob Foley
 *
 *****************************************************************************/
fbe_status_t fbe_test_sep_io_setup_standard_rdgen_test_context(fbe_api_rdgen_context_t *context_p,
                                                fbe_object_id_t object_id,
                                                fbe_class_id_t class_id,
                                                fbe_rdgen_operation_t rdgen_operation,
                                                fbe_lba_t capacity,
                                                fbe_u32_t max_passes,
                                                fbe_u32_t threads,
                                                fbe_u64_t io_size_blocks,
                                                fbe_bool_t b_inject_random_aborts,
                                                fbe_bool_t b_dualsp_io)
{
    fbe_status_t    status = FBE_STATUS_OK;
    fbe_u32_t       io_seconds;
    fbe_time_t      io_time;

    /* First determine if a run time is enabled or not.  If it is, it overrides
     * the maximum number of passes.
     */
    io_seconds = fbe_test_sep_io_get_io_seconds();
    io_time = io_seconds * FBE_TIME_MILLISECONDS_PER_SECOND;
    if (io_seconds > 0)
    {
        mut_printf(MUT_LOG_TEST_STATUS, "%s using I/O seconds of %d", __FUNCTION__, io_seconds);
        max_passes = 0;
    }

    /*! Now configure the standard rdgen test context values
     *
     *  @todo Add abort and peer options to the context setup
     */
    status = fbe_api_rdgen_test_context_init(context_p, 
                                             object_id, 
                                             class_id,
                                             FBE_PACKAGE_ID_SEP_0,
                                             rdgen_operation,
                                             FBE_RDGEN_PATTERN_LBA_PASS,    /* Standard rdgen data pattern */
                                             max_passes,
                                             0,                             /* io count not used */
                                             io_time,
                                             threads,
                                             FBE_RDGEN_LBA_SPEC_RANDOM,     /* Standard I/O mode is random */
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
        status = fbe_api_rdgen_io_specification_set_peer_options(&context_p->start_io.specification,
                                                                 rdgen_peer_options);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    }

    return FBE_STATUS_OK;
}
/*********************************************************
 * end fbe_test_sep_io_setup_standard_rdgen_test_context()
 *********************************************************/

/*!**************************************************************
 * fbe_test_sep_io_setup_fill_rdgen_test_context()
 ****************************************************************
 * @brief
 *  Setup the context for a fill operation.  This operation
 *  will sweep over an area with a given capacity using
 *  i/os of the same size.
 *
 * @param context_p - Context structure to setup.  
 * @param object_id - object id to run io against.
 * @param class_id - class id to test.
 * @param rdgen_operation - operation to start.
 * @param max_lba - largest lba to test with in blocks.
 * @param io_size_blocks - io size to fill with in blocks.             
 *
 * @return None.
 *
 * @author
 *  9/3/2009 - Created. Rob Foley
 *
 ****************************************************************/

void fbe_test_sep_io_setup_fill_rdgen_test_context(fbe_api_rdgen_context_t *context_p,
                                             fbe_object_id_t object_id,
                                             fbe_class_id_t class_id,
                                             fbe_rdgen_operation_t rdgen_operation,
                                             fbe_lba_t max_lba,
                                             fbe_u64_t io_size_blocks)
{
    fbe_status_t    status = FBE_STATUS_OK;

    /* If fixed I/O is enabled (for debug purposes) send the single fixed I/O
     */
    if (fbe_test_sep_io_fixed_io_enabled == FBE_TRUE)
    {
        status = fbe_api_rdgen_test_context_init(context_p, 
                                             object_id,
                                             class_id, 
                                             FBE_PACKAGE_ID_SEP_0,
                                             fbe_test_sep_io_fixed_io_operation,
                                             FBE_RDGEN_PATTERN_LBA_PASS,
                                             0xff,    /* We do one full sequential pass. */
                                             0,    /* num ios not used */
                                             0,    /* time not used*/
                                             1,    /* threads */
                                             FBE_RDGEN_LBA_SPEC_FIXED,
                                             fbe_test_sep_io_fixed_io_lba,    /* Start lba*/
                                             fbe_test_sep_io_fixed_io_lba,    /* Min lba */
                                             fbe_test_sep_io_fixed_io_lba + fbe_test_sep_io_fixed_io_blocks - 1,    /* max lba */
                                             FBE_RDGEN_BLOCK_SPEC_CONSTANT,
                                             fbe_test_sep_io_fixed_io_blocks,    /* Min blocks */
                                             fbe_test_sep_io_fixed_io_blocks    /* Max blocks */ );
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    }
    else
    {
        /* Else setup the request I/O context
         */
        status = fbe_api_rdgen_test_context_init(context_p, 
                                             object_id,
                                             class_id, 
                                             FBE_PACKAGE_ID_SEP_0,
                                             rdgen_operation,
                                             FBE_RDGEN_PATTERN_LBA_PASS,
                                             1,    /* We do one full sequential pass. */
                                             0,    /* num ios not used */
                                             0,    /* time not used*/
                                             1,    /* threads */
                                             FBE_RDGEN_LBA_SPEC_SEQUENTIAL_INCREASING,
                                             0,    /* Start lba*/
                                             0,    /* Min lba */
                                             max_lba,    /* max lba */
                                             FBE_RDGEN_BLOCK_SPEC_STRIPE_SIZE,
                                             16,    /* Number of stripes to write. */
                                             io_size_blocks    /* Max blocks */ );
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    }
    return;
}
/****************************************************
 * end fbe_test_sep_io_setup_fill_rdgen_test_context()
 ****************************************************/

/*!***************************************************************************
 * fbe_test_sep_io_run_write_pattern()
 *****************************************************************************
 * @brief
 *  Fill every block of every non-private LUN with a pre-fill pattern and then
 *  verify that the pattern was written successfully.
 *
 * @param sequence_config_p - Pointer to test sequence configuration
 * @param context_p - context to execute.
 * 
 * @return None.   
 *
 * @author
 *  12/18/2009 - Created. Rob Foley
 *
 *****************************************************************************/
fbe_status_t fbe_test_sep_io_run_write_pattern(fbe_test_sep_io_sequence_config_t *sequence_config_p,
                                               fbe_api_rdgen_context_t *context_p)
{
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_u64_t                           default_io_size = FBE_RAID_SECTORS_PER_ELEMENT;
    fbe_test_sep_io_sequence_state_t    sequence_io_state;

    /* First validate that the test sequence has been configured
     */
    sequence_io_state = fbe_test_sep_io_get_sequence_io_state(sequence_config_p);
    MUT_ASSERT_TRUE(sequence_io_state >= FBE_TEST_SEP_IO_SEQUENCE_STATE_IO_READY);

    /* Now populate the rdgen context
     */
    fbe_test_sep_io_setup_fill_rdgen_test_context(context_p,
                                            FBE_OBJECT_ID_INVALID,
                                            FBE_CLASS_ID_LUN,
                                            FBE_RDGEN_OPERATION_WRITE_ONLY,
                                            FBE_LBA_INVALID, /* use max capacity */
                                            default_io_size);
    /* Write a background pattern to the LUNs.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "%s Writing background pattern to all LUNs. Sequence type: %d", 
               __FUNCTION__, sequence_config_p->sequence_type);
    status = fbe_test_sep_io_run_rdgen_tests(context_p, FBE_PACKAGE_ID_NEIT, 1, sequence_config_p->rg_config_p);

    /* Make sure there were no errors.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "%s Writing background pattern to all LUNs...Finished", __FUNCTION__);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.error_count, 0);

    return FBE_STATUS_OK;
}
/*******************************************
 * end fbe_test_sep_io_run_write_pattern()
 ******************************************/

/*!***************************************************************************
 * fbe_test_sep_io_run_read_pattern()
 *****************************************************************************
 * @brief
 *  Fill every block of every non-private LUN with a pre-fill pattern and then
 *  verify that the pattern was written successfully.
 *
 * @param sequence_config_p - Pointer to test sequence configuration
 * @param context_p - context to execute.
 * 
 * @return None.   
 *
 * @author
 *  12/18/2009 - Created. Rob Foley
 *
 *****************************************************************************/
fbe_status_t fbe_test_sep_io_run_read_pattern(fbe_test_sep_io_sequence_config_t *sequence_config_p,
                                              fbe_api_rdgen_context_t *context_p)
{
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_u64_t                           default_io_size = FBE_RAID_SECTORS_PER_ELEMENT;
    fbe_test_sep_io_sequence_state_t    sequence_io_state;

    /* First validate that the test sequence has been configured
     */
    sequence_io_state = fbe_test_sep_io_get_sequence_io_state(sequence_config_p);
    MUT_ASSERT_TRUE(sequence_io_state >= FBE_TEST_SEP_IO_SEQUENCE_STATE_IO_READY);

    /* Read the expected pattern
     */
    fbe_test_sep_io_setup_fill_rdgen_test_context(context_p,
                                            FBE_OBJECT_ID_INVALID,
                                            FBE_CLASS_ID_LUN,
                                            FBE_RDGEN_OPERATION_READ_CHECK,
                                            FBE_LBA_INVALID, /* use max capacity */
                                            default_io_size);
    /* Write a background pattern to the LUNs.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "%s Reading background pattern from all LUNs. Sequence type: %d", 
               __FUNCTION__, sequence_config_p->sequence_type);
    status = fbe_test_sep_io_run_rdgen_tests(context_p, FBE_PACKAGE_ID_NEIT, 1, sequence_config_p->rg_config_p);

    /* Make sure there were no errors.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "%s Reading background pattern from all LUNs...Finished", __FUNCTION__);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.error_count, 0);

    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_test_sep_io_run_read_pattern()
 ******************************************/

/*!***************************************************************************
 * fbe_test_sep_io_run_verify()
 *****************************************************************************
 * @brief
 *  Fill every block of every non-private LUN with a pre-fill pattern and then
 *  verify that the pattern was written successfully.
 *
 * @param sequence_config_p - Pointer to test sequence configuration
 * 
 * @return None.   
 *
 * @author
 *  12/18/2009 - Created. Rob Foley
 *
 *****************************************************************************/
fbe_status_t fbe_test_sep_io_run_verify(fbe_test_sep_io_sequence_config_t *sequence_config_p)
{
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_test_sep_io_sequence_state_t    sequence_io_state;

    /* First validate that the test sequence has been configured
     */
    sequence_io_state = fbe_test_sep_io_get_sequence_io_state(sequence_config_p);
    MUT_ASSERT_TRUE(sequence_io_state >= FBE_TEST_SEP_IO_SEQUENCE_STATE_IO_READY);

    /* Verify no errors
     */
    status = fbe_test_sep_io_run_verify_on_all_luns(sequence_config_p->rg_config_p,
                                                    FBE_TRUE, /* Verify the entire lun */
                                                    0,
                                                    FBE_LBA_INVALID);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_test_sep_io_run_verify()
 ******************************************/

/*!***************************************************************************
 * fbe_test_sep_io_run_prefill()
 *****************************************************************************
 * @brief
 *  Fill every block of every non-private LUN with a pre-fill pattern and then
 *  verify that the pattern was written successfully.
 *
 * @param sequence_config_p - Pointer to test sequence configuration
 * @param context_p - context to execute.
 * 
 * @return None.   
 *
 * @author
 *  12/18/2009 - Created. Rob Foley
 *
 *****************************************************************************/

fbe_status_t fbe_test_sep_io_run_prefill(fbe_test_sep_io_sequence_config_t *sequence_config_p,
                                         fbe_api_rdgen_context_t *context_p)
{
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_u64_t                           default_io_size = FBE_RAID_SECTORS_PER_ELEMENT;
    fbe_test_sep_io_sequence_state_t    sequence_io_state;

    /* First validate that the test sequence has been configured
     */
    sequence_io_state = fbe_test_sep_io_get_sequence_io_state(sequence_config_p);
    MUT_ASSERT_TRUE(sequence_io_state >= FBE_TEST_SEP_IO_SEQUENCE_STATE_IO_READY);

    /* Now populate the rdgen context
     */
    fbe_test_sep_io_setup_fill_rdgen_test_context(context_p,
                                            FBE_OBJECT_ID_INVALID,
                                            FBE_CLASS_ID_LUN,
                                            FBE_RDGEN_OPERATION_WRITE_ONLY,
                                            FBE_LBA_INVALID, /* use max capacity */
                                            default_io_size);
    /* Write a background pattern to the LUNs.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "%s Writing background pattern to all LUNs. Sequence type: %d", 
               __FUNCTION__, sequence_config_p->sequence_type);
    status = fbe_test_sep_io_run_rdgen_tests(context_p, FBE_PACKAGE_ID_NEIT, 1, sequence_config_p->rg_config_p);

    /* Make sure there were no errors.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "%s Writing background pattern to all LUNs...Finished", __FUNCTION__);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.error_count, 0);

    fbe_test_sep_io_setup_fill_rdgen_test_context(context_p,
                                            FBE_OBJECT_ID_INVALID,
                                            FBE_CLASS_ID_LUN,
                                            FBE_RDGEN_OPERATION_READ_CHECK,
                                            FBE_LBA_INVALID, /* use max capacity */
                                            default_io_size);
    /* Write a background pattern to the LUNs.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "%s Reading background pattern from all LUNs. Sequence type: %d", 
               __FUNCTION__, sequence_config_p->sequence_type);
    status = fbe_test_sep_io_run_rdgen_tests(context_p, FBE_PACKAGE_ID_NEIT, 1, sequence_config_p->rg_config_p);

    /* Make sure there were no errors.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "%s Reading background pattern from all LUNs...Finished", __FUNCTION__);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.error_count, 0);

    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_test_sep_io_run_prefill()
 ******************************************/

/*!***************************************************************************
 *          fbe_test_sep_io_run_standard_sequence()
 *****************************************************************************
 *
 * @brief   Run the standard I/O sequence against all the non-private LUNs
 *
 * @param   sequence_config_p - Pointer to configuration for this test sequence
 * @param   context_p - Pointer to rdgen context to use for I/O groups
 *
 * @return  status - Determine if test passed or not
 *
 * @author
 *  12/18/2009 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t fbe_test_sep_io_run_standard_sequence(fbe_test_sep_io_sequence_config_t *sequence_config_p,
                                                   fbe_api_rdgen_context_t *context_p)
                                                          
{
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_test_sep_io_sequence_state_t    sequence_io_state;

    /* First validate that the test sequence has been configured
     */
    sequence_io_state = fbe_test_sep_io_get_sequence_io_state(sequence_config_p);
    MUT_ASSERT_TRUE(sequence_io_state >= FBE_TEST_SEP_IO_SEQUENCE_STATE_IO_READY);

    /* Step 1)  Execute a write/read/compare I/O for small I/O
     *
     */
    mut_printf(MUT_LOG_TEST_STATUS, "%s Executing write/read/compare for all LUNs. ", 
               __FUNCTION__);
    status = fbe_test_sep_io_setup_standard_rdgen_test_context(context_p,
                                                FBE_OBJECT_ID_INVALID,
                                                FBE_CLASS_ID_LUN,
                                                FBE_RDGEN_OPERATION_WRITE_READ_CHECK,
                                                FBE_LBA_INVALID, /* use capacity */
                                                sequence_config_p->small_io_count, /* passes */
                                                sequence_config_p->io_thread_count, /* threads */
                                                sequence_config_p->max_blocks_for_small_io,
                                                sequence_config_p->abort_enabled,
                                                sequence_config_p->dualsp_enabled);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Run our I/O test.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "%s Running write/read/compare small I/Os to all LUNs. max blocks: %llu  passes: %d", 
               __FUNCTION__,
	       (unsigned long long)sequence_config_p->max_blocks_for_small_io,
	       sequence_config_p->small_io_count );
    status = fbe_test_sep_io_run_rdgen_tests(context_p, FBE_PACKAGE_ID_NEIT, 1, sequence_config_p->rg_config_p);

    /* Make sure there were no errors.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "%s Running write/read/compare small I/Os to all LUNs...Finished", __FUNCTION__);
    fbe_test_sep_io_validate_status(status, context_p, sequence_config_p->abort_enabled);

    /* Step 2) Verify/Write/Read/Compare for small I/O
     */
    status = fbe_test_sep_io_setup_standard_rdgen_test_context(context_p,
                                                FBE_OBJECT_ID_INVALID,
                                                FBE_CLASS_ID_LUN,
                                                FBE_RDGEN_OPERATION_VERIFY_WRITE_READ_CHECK,
                                                FBE_LBA_INVALID, /* use capacity */
                                                1, /* Num passes. */
                                                2, /* threads */
                                                sequence_config_p->max_blocks_for_small_io,
                                                sequence_config_p->abort_enabled,
                                                sequence_config_p->dualsp_enabled);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Run our I/O test.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "%s Running verify-write/read/compare small I/Os to all LUNs. max blocks: %llu  passes: %d", 
               __FUNCTION__,
	       (unsigned long long)sequence_config_p->max_blocks_for_small_io, 1);
    status = fbe_test_sep_io_run_rdgen_tests(context_p, FBE_PACKAGE_ID_NEIT, 1, sequence_config_p->rg_config_p);

    /* Make sure there were no errors.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "%s Running verify-write/read/compare small I/Os to all LUNs...Finished", __FUNCTION__);
    fbe_test_sep_io_validate_status(status, context_p, sequence_config_p->abort_enabled);

    /* Step 3) Verify/Write/Read/Compare for large I/O
     */
    status = fbe_test_sep_io_setup_standard_rdgen_test_context(context_p,
                                                FBE_OBJECT_ID_INVALID,
                                                FBE_CLASS_ID_LUN,
                                                FBE_RDGEN_OPERATION_VERIFY_WRITE_READ_CHECK,
                                                FBE_LBA_INVALID, /* use capacity */
                                                1, /* I/O passes. */
                                                2, /* threads */
                                                sequence_config_p->max_blocks_for_large_io,
                                                sequence_config_p->abort_enabled,
                                                sequence_config_p->dualsp_enabled);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Run our I/O test.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "%s Running verify-write/read/compare large I/Os to all LUNs. max blocks: %llu  passes: %d", 
               __FUNCTION__,
	       (unsigned long long)sequence_config_p->max_blocks_for_large_io, 1);
    status = fbe_test_sep_io_run_rdgen_tests(context_p, FBE_PACKAGE_ID_NEIT, 1, sequence_config_p->rg_config_p);

    /* Make sure there were no errors.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "%s Running verify-write/read/compare large I/Os to all LUNs...Finished", __FUNCTION__);
    fbe_test_sep_io_validate_status(status, context_p, sequence_config_p->abort_enabled);

    /* Step 4)  Execute a write/read/compare I/O for large I/O
     */
    status = fbe_test_sep_io_setup_standard_rdgen_test_context(context_p,
                                                FBE_OBJECT_ID_INVALID,
                                                FBE_CLASS_ID_LUN,
                                                FBE_RDGEN_OPERATION_WRITE_READ_CHECK,
                                                FBE_LBA_INVALID, /* use capacity */
                                                sequence_config_p->large_io_count, /* passes */
                                                sequence_config_p->io_thread_count, /* threads */
                                                sequence_config_p->max_blocks_for_large_io,
                                                sequence_config_p->abort_enabled,
                                                sequence_config_p->dualsp_enabled);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Run our I/O test.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "%s Running write/read/compare large I/Os to all LUNs. max blocks: %llu  passes: %d", 
               __FUNCTION__,
	       (unsigned long long)sequence_config_p->max_blocks_for_large_io,
	       sequence_config_p->large_io_count);
    status = fbe_test_sep_io_run_rdgen_tests(context_p, FBE_PACKAGE_ID_NEIT, 1, sequence_config_p->rg_config_p);

    /* Make sure there were no errors.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "%s Running write/read/compare large I/Os to all LUNs...Finished", __FUNCTION__);
    fbe_test_sep_io_validate_status(status, context_p, sequence_config_p->abort_enabled);

    return FBE_STATUS_OK;
}
/**********************************************
 * end fbe_test_sep_io_run_standard_sequence()
 **********************************************/

/*!***************************************************************************
 *          fbe_test_sep_io_run_standard_abort_sequence()
 *****************************************************************************
 *
 * @brief   Run the standard I/O sequence with aborts against all the 
 *          non-private LUNs.
 *
 * @param   sequence_config_p - Pointer to test sequence configuration
 * @param   context_p - Pointer to rdgen context to use for I/O groups
 * @param   max_blocks_for_small_io - Maximum blocks per small I/O request
 * @param   max_blocks_for_large_io - Maximum blocks per large I/O request
 * @param   b_inject_random_aborts - FBE_TRUE - Use random_abort_time_msecs
 *              to determine and abort random requests
 *                                   FBE_FALSE - Don't inject aborts
 * @param   random_abort_time_msecs - Random time to abort in milliseconds 
 * @param   b_run_dual_sp - FBE_TRUE - Run non-prefill I/O in dual SP mode
 *                          FBE_FALSE - Run all I/O on this SP (single SP mode)
 * @param   dual_sp_mode - Dual SP mode (only used with b_run_dual_sp is true)
 * 
 * @return  status - Determine if test passed or not
 *
 *
 * @author
 *  5/8/2010 - Created. Mahesh Agarkar
 *
 ****************************************************************/
fbe_status_t fbe_test_sep_io_run_standard_abort_sequence(fbe_test_sep_io_sequence_config_t *sequence_config_p,
                                                         fbe_api_rdgen_context_t *context_p)
{
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_u32_t                           test_level = fbe_sep_test_util_get_raid_testing_extended_level();
    fbe_test_sep_io_sequence_state_t    sequence_io_state;

    /* First validate that the test sequence has been configured
     */
    sequence_io_state = fbe_test_sep_io_get_sequence_io_state(sequence_config_p);
    MUT_ASSERT_TRUE(sequence_io_state >= FBE_TEST_SEP_IO_SEQUENCE_STATE_IO_READY);

    /*! In test level 0 we will always do some amount of abort testing.  
     *  This ensures that our abort testing does not regress. 
     *  Option(1): run I/Os with random abort value set to '0' msecs
     *  
     *  @note We ignore the configured abort mode!!
     */

    mut_printf(MUT_LOG_TEST_STATUS, "%s Started I/Os with random Aborts: ZERO MSECS. ", 
               __FUNCTION__);
    status = fbe_test_sep_io_configure_abort_mode(sequence_config_p->abort_enabled, 
                                                  FBE_TEST_RANDOM_ABORT_TIME_ZERO_MSEC);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    status = fbe_test_sep_io_run_standard_sequence(sequence_config_p, context_p);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "%s Completed I/Os with random Aborts: ZERO MSECS. ", 
               __FUNCTION__);

    /* For test_level greater than ZERO we will rerun the test with random abort
     * We will I/Os with random_abort time caculated using '100' msecs abort time.
     */
    if (test_level > 0)
    {
        mut_printf(MUT_LOG_TEST_STATUS, "%s Started I/Os with random Aborts: 100 MSECS. ", 
               __FUNCTION__);
        /*! Option(2): run I/Os with random abort value set to '100' msecs
         *
         *  @note We ignore the configured abort mode!!
         */
        status = fbe_test_sep_io_configure_abort_mode(sequence_config_p->abort_enabled, 
                                                      FBE_TEST_RANDOM_ABORT_TIME_ONE_TENTH_MSEC);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        status = fbe_test_sep_io_run_standard_sequence(sequence_config_p, context_p);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        mut_printf(MUT_LOG_TEST_STATUS, "%s Completed I/Os with random Aborts: 100 MSECS ", 
               __FUNCTION__);
    }

    return FBE_STATUS_OK;
}
/***************************************************
 * end fbe_test_sep_io_run_standard_abort_sequence()
 ***************************************************/


/****************************************
 * end of file fbe_test_sep_io_standard.c
 ****************************************/

