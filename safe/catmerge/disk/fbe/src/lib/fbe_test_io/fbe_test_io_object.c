/***************************************************************************
 * Copyright (C) EMC Corporation 2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_test_io_object.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the methods of the test io object.  This object
 *  represents an I/O test and helps in the execution of I/O tests.
 *
 * HISTORY
 *   8/8/2008:  Created. RPF
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_transport.h"
#include "fbe_topology.h"
#include "fbe_block_transport.h"
#include "mut.h"
#include "fbe_test_io_api.h"
#include "fbe_test_io_private.h"


/*************************
 *   FUNCTION DEFINITIONS
 *************************/


/*!**************************************************************
 * @fn fbe_test_io_task_object_init(
 *                     fbe_test_io_task_object_t *const io_obj_p,
 *                     fbe_object_id_t object_id,
 *                     fbe_test_io_pattern_t pattern)
 ****************************************************************
 * @brief
 *  This function initializes an io task object.
 *
 * @param io_obj_p - Test object being executed.
 * @param object_id - The object to test against.
 * @param pattern - The pattern to test with.
 * @param opcode - The operation to perform.
 *
 * @return None.   
 *
 * HISTORY:
 *  8/11/2008 - Created. RPF
 *
 ****************************************************************/

void 
fbe_test_io_task_object_init(fbe_test_io_task_object_t *const io_obj_p,
                             fbe_object_id_t object_id,
                             fbe_test_io_pattern_t pattern,
                             fbe_payload_block_operation_opcode_t opcode)
{
    /* Simply initialize the values that we care about
     * including the object id, patterns and pass. 
     */
    MUT_ASSERT_NOT_NULL(io_obj_p);
    io_obj_p->object_id = object_id;
    io_obj_p->background_pattern = FBE_IO_TEST_TASK_PATTERN_FFS;
    io_obj_p->test_pattern = pattern;
    io_obj_p->pass_count = 0;
    io_obj_p->opcode = opcode;
    return;
}
/******************************************
 * end fbe_test_io_task_object_init()
 ******************************************/

/*!**************************************************************
 * @fn fbe_test_io_task_object_init_fns(
 *                    fbe_test_io_task_object_t *const io_obj_p,
 *                    fbe_test_io_task_object_function_t test_setup,
 *                    fbe_test_io_task_object_function_t test_execute,
 *                    fbe_test_io_task_object_function_t io_setup,
 *                    fbe_test_io_task_object_function_t io_execute,
 *                    fbe_test_io_task_object_function_t io_cleanup)
 ****************************************************************
 * @brief
 *  Initialize the io task object.
 *
 * @param io_obj_p - Test object being used.  
 * @param test_setup - Function that sets up the overall test.
 * @param test_execute - Function that executes a full test.
 *                       We assume that somewhere in this function we will call
 *                       fbe_test_io_task_object_execute_io()
 * @param io_setup - This function sets up an individual I/O (e.g. read/write).
 * @param io_execute - This function executes an individual I/O (e.g.
 *                     read/write).
 * @param io_cleanup - This function executes the cleanup for this task.
 *
 * @return None.
 *
 * HISTORY:
 *  8/11/2008 - Created. RPF
 *
 ****************************************************************/

void fbe_test_io_task_object_init_fns(
    fbe_test_io_task_object_t *const io_obj_p,
    fbe_test_io_task_object_function_t test_setup,
    fbe_test_io_task_object_function_t test_execute,
    fbe_test_io_task_object_function_t io_setup,
    fbe_test_io_task_object_function_t io_execute,
    fbe_test_io_task_object_function_t io_cleanup)
{
    /* The execute functions must always be set.
     */
    MUT_ASSERT_NOT_NULL(io_obj_p);
    MUT_ASSERT_NOT_NULL(io_execute);
    MUT_ASSERT_NOT_NULL(test_execute);

    /* Now save the function pointers.
     */
    io_obj_p->functions.io_cleanup = io_cleanup;
    io_obj_p->functions.io_execute = io_execute;
    io_obj_p->functions.io_setup = io_setup;
    io_obj_p->functions.test_setup = test_setup;
    io_obj_p->functions.test_execute = test_execute;
    return;
}
/******************************************
 * end fbe_test_io_task_object_init_fns()
 ******************************************/

/*!**************************************************************
 * @fn fbe_test_io_task_object_set_test_case(
 *                    fbe_test_io_task_object_t *const io_test_object_p,
 *                    fbe_test_io_case_t *const case_p)
 ****************************************************************
 * @brief
 *  Set the test case into the io task object.
 *
 * @param  io_test_object_p - Test object being used.  
 * @param  case_p - Test case to set into the test object.
 *
 * @return None.
 *
 * HISTORY:
 *  8/11/2008 - Created. RPF
 *
 ****************************************************************/
void 
fbe_test_io_task_object_set_test_case(fbe_test_io_task_object_t *const io_test_object_p,
                                      fbe_test_io_case_t *const case_p)
{
    MUT_ASSERT_NOT_NULL(io_test_object_p);
    MUT_ASSERT_NOT_NULL(case_p);
    io_test_object_p->io_case = *case_p;
    return;
}
/******************************************
 * end fbe_test_io_task_object_set_test_case()
 ******************************************/

/*!**************************************************************
 * @fn fbe_test_io_task_object_execute_io(
 *                  fbe_test_io_task_object_t *const io_test_object_p)
 ****************************************************************
 * @brief
 *  Execute a single I/O using the setup, execute and cleanup
 *  methods of the io object.
 *
 * @param io_test_object_p - Test object being executed.               
 *
 * @return fbe_status_t status   
 *
 * HISTORY:
 *  8/11/2008 - Created. RPF
 *
 ****************************************************************/

fbe_status_t 
fbe_test_io_task_object_execute_io(fbe_test_io_task_object_t *const io_test_object_p)
{
    fbe_status_t status = FBE_STATUS_OK;

    /* Now execute the I/O functions.
     * Note that setup is optional, so we only execute cleanup if 
     * it is set. 
     */
    if ( io_test_object_p->functions.io_setup != NULL )
    {
        status = io_test_object_p->functions.io_setup(io_test_object_p);
    }

    /* As long as the setup went OK, issue the execute.
     */
    if (status == FBE_STATUS_OK)
    {
        /* The execute must always be set.
         */
        MUT_ASSERT_NOT_NULL(io_test_object_p->functions.io_execute);
        
        status = io_test_object_p->functions.io_execute(io_test_object_p);
    }

    /* As long as the execute went OK, issue the cleanup.
     * Note that cleanup is optional, so we only execute cleanup if 
     * it is set. 
     */
    if (status == FBE_STATUS_OK &&
        io_test_object_p->functions.io_cleanup != NULL)
    {
        status = io_test_object_p->functions.io_cleanup(io_test_object_p);
    }
    return status;
}
/******************************************
 * end fbe_test_io_task_object_execute_io()
 ******************************************/

/*!**************************************************************
 * @fn fbe_test_io_task_object_execute(
 *                  fbe_test_io_task_object_t *const io_test_object_p)
 ****************************************************************
 * @brief
 *  Execute the overall test.
 *
 *  Note that it is assumed that the test_execute function will at some point call 
 *  fbe_test_io_task_object_execute_io() to execute the individual I/Os.
 *
 * @param io_test_object_p - Test object being executed.
 *
 * @return fbe_status_t status
 *
 * HISTORY:
 *  8/11/2008 - Created. RPF
 *
 ****************************************************************/
fbe_status_t 
fbe_test_io_task_object_execute(fbe_test_io_task_object_t *const io_test_object_p)
{
    fbe_status_t status = FBE_STATUS_OK;

    /* Now execute the I/O functions.
     */
    if (io_test_object_p->functions.test_setup != NULL)
    {
        status = io_test_object_p->functions.test_setup(io_test_object_p);
    }

    /* As long as the setup went OK, issue the execute.
     */
    if (status == FBE_STATUS_OK)
    {
        MUT_ASSERT_NOT_NULL(io_test_object_p->functions.test_execute);
        status = io_test_object_p->functions.test_execute(io_test_object_p);
    }
    return status;
}
/******************************************
 * end fbe_test_io_task_object_execute()
 ******************************************/

/*!***************************************************************
 * @fn fbe_test_io_negotiate_and_validate_block_size(
 *                  fbe_object_id_t object_id,
 *                  fbe_block_size_t exported_block_size,
 *                  fbe_block_size_t exported_optimal_block_size,
 *                  fbe_block_size_t expected_imported_block_size,
 *                  fbe_block_size_t expected_imported_optimal_block_size)
 ****************************************************************
 * @brief
 *  This function negotiates the block size and validates the returned
 *  information.
 *
 * @param object_id - The object_id to test.
 * @param exported_block_size - The bytes per block in the exported block size.
 * @param exported_optimal_block_size - Number of blocks in exported block size.
 * @param expected_imported_block_size - This is what we expect to be returned
 *                                       for the imported block size.
 * @param expected_imported_optimal_block_size - This is what we expect to be
 *                                           returned for the imported optimal
 *                                           block size.
 * @return 
 *  The status of the test.
 *  Anything other than FBE_STATUS_OK indicates failure.
 *
 * HISTORY:
 *  11/12/07 - Created. RPF
 *
 ****************************************************************/

fbe_status_t 
fbe_test_io_negotiate_and_validate_block_size(fbe_object_id_t object_id,
                                              fbe_block_size_t exported_block_size,
                                              fbe_block_size_t exported_optimal_block_size,
                                              fbe_block_size_t expected_imported_block_size,
                                              fbe_block_size_t expected_imported_optimal_block_size)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_block_transport_negotiate_t capacity;
    fbe_payload_block_operation_status_t block_status;
    fbe_payload_block_operation_qualifier_t block_qualifier;

    /* Read the capacity in so that we refresh the correct info to be used below..
     */
    capacity.requested_block_size = exported_block_size;

    /* If the optimal block size is zero, the client does not care what it
     * is.  We'll find out what is supported and fill it in. 
     */
    if (exported_optimal_block_size == 0)
    {
        capacity.requested_optimum_block_size = exported_block_size;
    }
    else
    {
        capacity.requested_optimum_block_size = exported_optimal_block_size;
    }

    status = fbe_test_io_get_block_size( object_id, &capacity, 
                                         &block_status, &block_qualifier );
    MUT_ASSERT_INT_EQUAL_MSG(status, 
                             FBE_STATUS_OK, 
                             "negotiate block size failed unexpectedly.");
    MUT_ASSERT_INT_EQUAL(block_status, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS);
    MUT_ASSERT_INT_EQUAL(block_qualifier, FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE);

    /* Make sure the exported block size and exported optimal block size is as
     * we negotiated. 
     */
    MUT_ASSERT_INT_EQUAL(exported_block_size, capacity.block_size);
    MUT_ASSERT_INT_EQUAL(exported_optimal_block_size, capacity.optimum_block_size);

    /* Make sure the physical are as expected.
     */
    MUT_ASSERT_INT_EQUAL(expected_imported_block_size, capacity.physical_block_size);
    MUT_ASSERT_INT_EQUAL(expected_imported_optimal_block_size, capacity.physical_optimum_block_size);
    return status;
}
/* end fbe_test_io_negotiate_and_validate_block_size() */

/*!***************************************************************
 * @fn fbe_test_io_task_object_negotiate_block_sizes(
 *                  fbe_test_io_task_object_t *const io_test_object_p)
 ****************************************************************
 * @brief
 *  This function performs a test for negotiate block size.
 *
 * @param io_test_object_p - Test object to use.  
 *
 * @return
 *  The status of the test.
 *  Anything other than FBE_STATUS_OK indicates failure.
 *
 * HISTORY:
 *  11/12/07 - Created. RPF
 *
 ****************************************************************/

fbe_status_t 
fbe_test_io_task_object_negotiate_block_sizes(fbe_test_io_task_object_t *const io_test_object_p)
{
    fbe_status_t status = FBE_STATUS_OK;

    /* Simply use our helper function to negotiate block size and validate 
     * returned information. 
     */
    status = fbe_test_io_negotiate_and_validate_block_size(
        io_test_object_p->object_id,
        io_test_object_p->io_case.exported_block_size,
        io_test_object_p->io_case.exported_optimal_block_size,
        io_test_object_p->io_case.imported_block_size,
        io_test_object_p->io_case.imported_optimal_block_size);

    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    return status;
}
/* end fbe_test_io_task_object_negotiate_block_sizes() */

/*!***************************************************************
 * @fn fbe_test_io_task_object_range_setup(
 *                  fbe_test_io_task_object_t *const io_test_object_p)
 ****************************************************************
 * @brief
 *  This function performs a setup of the range under test.
 *
 * @param io_test_object_p - Test object to use.
 *
 * @return
 *  The status of the test.
 *  Anything other than FBE_STATUS_OK indicates failure.
 *
 * HISTORY:
 *  05/20/08 - Created. RPF
 *
 ****************************************************************/

fbe_status_t 
fbe_test_io_task_object_range_setup(fbe_test_io_task_object_t *const io_test_object_p)
{
    fbe_status_t status;

    /* First setup is to set the block sizes.
     */
    status = fbe_test_io_task_object_negotiate_block_sizes(io_test_object_p);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Next setup is to set the background pattern for the range to test.
     */
    status = fbe_test_io_task_object_fill_area(io_test_object_p);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    return FBE_STATUS_OK;
}
/* end fbe_test_io_task_object_range_setup() */


/*!***************************************************************
 * @fn fbe_test_io_task_object_fill_area(
 *            fbe_test_io_task_object_t *const io_test_object_p)
 ****************************************************************
 * @brief
 *  This function fills an area with a known pattern.
 *
 * @param io_test_object_p - io object to use for test.
 *
 * @return
 *  The status of the test.
 *  Anything other than FBE_STATUS_OK indicates failure.
 *
 * HISTORY:
 *  05/20/08 - Created. RPF
 *
 ****************************************************************/

fbe_status_t 
fbe_test_io_task_object_fill_area(fbe_test_io_task_object_t *const io_test_object_p)
{
    return FBE_STATUS_OK;
}
/* end fbe_test_io_task_object_fill_area() */

/*!***************************************************************
 * @fn fbe_test_io_task_object_loop_over_range(
 *            fbe_test_io_task_object_t *const io_test_object_p)
 ****************************************************************
 * @brief
 *  This function performs an io test over an io range.
 *
 * @param io_test_object_p - io object to use for test.
 *
 * @return
 *  The status of the test.
 *  Anything other than FBE_STATUS_OK indicates failure.
 *
 * HISTORY:
 *  11/12/07 - Created. RPF
 *
 ****************************************************************/

fbe_status_t 
fbe_test_io_task_object_loop_over_range(fbe_test_io_task_object_t *const io_test_object_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_lba_t lba = io_test_object_p->io_case.start_lba;
    fbe_block_count_t blocks;
    fbe_u32_t iteration_count = 0;

    /* Continue loop over the entire lba range as long as there are no errors.
     */
    while (lba < io_test_object_p->io_case.end_lba && status == FBE_STATUS_OK)
    {
        blocks = io_test_object_p->io_case.start_blocks;

        /* Loop over the range of block sizes to issue.
         * Continue as long as there are no errors.
         */
        while (blocks < io_test_object_p->io_case.end_blocks &&
               (lba + blocks < io_test_object_p->io_case.end_lba) && 
               status == FBE_STATUS_OK)
        {
            /* Every N iterations display something so the user will see that 
             * progress is being made. 
             */
            iteration_count++;
            if (iteration_count % FBE_TEST_IO_ITERATIONS_TO_TRACE == 0)
            {
                mut_printf(MUT_LOG_HIGH, 
                           "fbe_test_io: lba:0x%llx blocks:0x%llx imported block size:0x%x",
                           (unsigned long long)lba, (unsigned long long)blocks,
			   io_test_object_p->io_case.imported_block_size);
            }

            /* Setup the io object with the current lba and blocks.
             */
            io_test_object_p->current_lba = lba;
            io_test_object_p->current_blocks = blocks;

            /* Execute the I/O functions.
             */
            status = fbe_test_io_task_object_execute_io(io_test_object_p);
                
            blocks += io_test_object_p->io_case.increment_blocks;
        }/* end while blocks < end_blocks. */
            
        lba += io_test_object_p->io_case.increment_blocks;
    }/* end while lba < end_lba. */
    
    return status;
}
/* end fbe_test_io_task_object_loop_over_range() */

/*!***************************************************************
 * @fn fbe_test_io_task_object_issue_write(
 *            fbe_test_io_task_object_t *const io_test_object_p)
 ****************************************************************
 * @brief
 *  This function issues a single write IO.
 *
 * @param io_test_object_p - io object to use for test.
 *
 * @return
 *  The status of the test.
 *  Anything other than FBE_STATUS_OK indicates failure.
 *
 * HISTORY:
 *  05/20/08 - Created. RPF
 *
 ****************************************************************/
fbe_status_t 
fbe_test_io_task_object_issue_write(fbe_test_io_task_object_t *const io_test_object_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_api_block_status_values_t block_status;
    fbe_u32_t qualifier;

    /* Determine the size to fill.  We will fill up to the
     * entire area being modified..
     */
    fbe_lba_t edge_lba = io_test_object_p->current_lba;
    fbe_block_count_t edge_blocks = io_test_object_p->current_blocks;

    fbe_logical_drive_get_pre_read_lba_blocks_for_write( 
        io_test_object_p->io_case.exported_block_size,
        io_test_object_p->io_case.exported_optimal_block_size,
        io_test_object_p->io_case.imported_block_size,
        &edge_lba,
        &edge_blocks );

    if ( edge_lba != 0)
    {
        /* Extend the range written with pattern by 1 at beginning and end.
         * This will allow us to come back after the write and validate that the 
         * pre-read blocks are still intact. 
         */
        edge_lba --;
        edge_blocks += 2;
    }

    /* Issue background pattern write for this range if we are
     * performing a pre-read. 
     */
    if (edge_lba != io_test_object_p->current_lba ||
        edge_blocks != io_test_object_p->current_blocks)
    {
        status = fbe_test_io_write_pattern( io_test_object_p->object_id,
                                            io_test_object_p->background_pattern,
                                            0xAA,
                                            FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,
                                            edge_lba,
                                            edge_blocks,
                                            io_test_object_p->io_case.exported_block_size,
                                            io_test_object_p->io_case.exported_optimal_block_size,
                                            io_test_object_p->io_case.imported_block_size,
                                            &block_status,
                                            &qualifier);
        
        MUT_ASSERT_INT_EQUAL_MSG(FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, block_status.status, 
                                 "Expected background pattern write to be successful.");
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }

    /* Issue the seeded write for this range.
     */
    status = fbe_test_io_write_pattern( io_test_object_p->object_id,
                                        FBE_IO_TEST_TASK_PATTERN_ADDRESS_PASS_COUNT,
                                        io_test_object_p->pass_count,
                                        io_test_object_p->opcode,
                                        io_test_object_p->current_lba,
                                        io_test_object_p->current_blocks,
                                        io_test_object_p->io_case.exported_block_size,
                                        io_test_object_p->io_case.exported_optimal_block_size,
                                        io_test_object_p->io_case.imported_block_size,
                                        &block_status,
                                        &qualifier);

    MUT_ASSERT_INT_EQUAL_MSG(FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, block_status.status, 
                             "Expected background pattern write to be successful.");
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    return status;
}
/* end fbe_test_io_task_object_issue_write() */

/*!***************************************************************
 * @fn fbe_test_io_task_object_issue_write_same(
 *            fbe_test_io_task_object_t *const io_test_object_p)
 ****************************************************************
 * @brief
 *  This function issues a single write same IO.
 *
 * @param io_test_object_p - io object to use for test.
 *
 * @return
 *  The status of the test.
 *  Anything other than FBE_STATUS_OK indicates failure.
 *
 * HISTORY:
 *  05/20/08 - Created. RPF
 *
 ****************************************************************/
fbe_status_t 
fbe_test_io_task_object_issue_write_same(fbe_test_io_task_object_t *const io_test_object_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t qualifier;

    /* Issue background pattern write for this range.
     */
    status = fbe_test_io_write_same_pattern( io_test_object_p->object_id,
                                               FBE_IO_TEST_TASK_PATTERN_FFS,
                                               0xFF,
                                               FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_SAME,
                                               io_test_object_p->current_lba,
                                               io_test_object_p->current_blocks,
                                               io_test_object_p->io_case.exported_block_size,    
                                               io_test_object_p->io_case.imported_block_size,
                                               io_test_object_p->io_case.exported_optimal_block_size,
                                               &qualifier);
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, 
                             "Expected write same background pattern write to be successful.");

    /* Issue the seeded write for this range.
     */
    status = fbe_test_io_write_same_pattern( io_test_object_p->object_id,
                                               FBE_IO_TEST_TASK_PATTERN_ZEROS,
                                               io_test_object_p->pass_count,
                                               FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_SAME,
                                               io_test_object_p->current_lba,
                                               io_test_object_p->current_blocks,
                                               io_test_object_p->io_case.exported_block_size,
                                               io_test_object_p->io_case.imported_block_size,
                                               io_test_object_p->io_case.exported_optimal_block_size,
                                               &qualifier );
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, 
                             "Expected write same to be successful.");
    return(status);
}
/* end fbe_test_io_task_object_issue_write_same() */

/*!***************************************************************
 * @fn fbe_test_io_task_object_issue_read_compare(
 *            fbe_test_io_task_object_t *const io_test_object_p)
 ****************************************************************
 * @brief
 *  This function issues a single read with a check for a pattern.
 *
 * @param io_test_object_p - io object to use for test.
 *
 * @return
 *  The status of the test.
 *  Anything other than FBE_STATUS_OK indicates failure.
 *
 * HISTORY:
 *  05/20/08 - Created. RPF
 *
 ****************************************************************/
fbe_status_t 
fbe_test_io_task_object_issue_read_compare(fbe_test_io_task_object_t *const io_test_object_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_payload_block_operation_status_t block_status;
    fbe_payload_block_operation_qualifier_t block_qualifier;
    fbe_u32_t qualifier;

    /* Issue a read and check the pattern in the data that was returned.
     */
    status = fbe_test_io_read_check_pattern( io_test_object_p->object_id,
                                             io_test_object_p->test_pattern,
                                             io_test_object_p->pass_count,
                                             io_test_object_p->current_lba,
                                             io_test_object_p->current_blocks,
                                             io_test_object_p->io_case.exported_block_size,
                                             io_test_object_p->io_case.exported_optimal_block_size,
                                             io_test_object_p->io_case.imported_block_size,
                                             &block_status,
                                             &block_qualifier,
                                             &qualifier );
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, 
                             "Expected read/check pattern to be successful.");
    MUT_ASSERT_INT_EQUAL(block_status, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS);
    MUT_ASSERT_INT_EQUAL(block_qualifier, FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE);
    return(status);
}
/* end fbe_test_io_task_object_issue_read_compare() */

/*!***************************************************************
 * @fn fbe_test_io_task_object_issue_compare_for_write(
 *            fbe_test_io_task_object_t *const io_test_object_p)
 ****************************************************************
 * @brief
 *  This function issues a set of reads to check patterns for the write.
 *
 * @param io_test_object_p - io object to use for test.
 *
 * @return
 *  The status of the test.
 *  Anything other than FBE_STATUS_OK indicates failure.
 *
 * HISTORY:
 *  05/20/08 - Created. RPF
 *
 ****************************************************************/
fbe_status_t 
fbe_test_io_task_object_issue_compare_for_write(fbe_test_io_task_object_t *const io_test_object_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_payload_block_operation_status_t block_status;
    fbe_payload_block_operation_qualifier_t block_qualifier;
    fbe_u32_t qualifier;

    /* Determine the size to fill.  We will fill up to the
     * entire area being modified..
     */
    fbe_lba_t edge_lba = io_test_object_p->current_lba;
    fbe_block_count_t edge_blocks = io_test_object_p->current_blocks;

    fbe_logical_drive_get_pre_read_lba_blocks_for_write( io_test_object_p->io_case.exported_block_size,
                                                         io_test_object_p->io_case.exported_optimal_block_size,
                                                         io_test_object_p->io_case.imported_block_size,
                                                         &edge_lba,
                                                         &edge_blocks );

    if ( edge_lba != 0)
    {
        /* Extend the range written with pattern by 1 to check for any issues.
         */
        edge_lba --;
        edge_blocks += 2;
    }

    /* Issue a read and check the pattern in the data that was returned.
     */
    status = fbe_test_io_read_check_pattern( io_test_object_p->object_id,
                                             io_test_object_p->test_pattern,
                                             io_test_object_p->pass_count,
                                             io_test_object_p->current_lba,    /* lba */
                                             io_test_object_p->current_blocks,    /* blocks */
                                             io_test_object_p->io_case.exported_block_size,    /* requested block size */
                                             io_test_object_p->io_case.exported_optimal_block_size,
                                             io_test_object_p->io_case.imported_block_size,
                                             &block_status,
                                             &block_qualifier,
                                             &qualifier );
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, 
                             "Expected read/check pattern to be successful.");
    MUT_ASSERT_INT_EQUAL(block_status, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS);
    MUT_ASSERT_INT_EQUAL(block_qualifier, FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE);

    /* If there was a pre-read at the beginning, make sure the correct pattern
     * is still there.  The write would have put down a pattern first. 
     */
    if (edge_lba != io_test_object_p->current_lba &&
        edge_lba < io_test_object_p->current_lba)
    {
        fbe_block_count_t blocks;
        MUT_ASSERT_TRUE(edge_lba < io_test_object_p->current_lba);
        blocks = (fbe_block_count_t)(io_test_object_p->current_lba - edge_lba);

        /* Read the area where a pre-read was issued.
         */
        status = fbe_test_io_read_check_pattern( io_test_object_p->object_id,
                                                 io_test_object_p->background_pattern,
                                                 0xAA,
                                                 edge_lba,    /* lba */
                                                 blocks,    /* blocks */
                                                 io_test_object_p->io_case.exported_block_size,    /* requested block size */
                                                 io_test_object_p->io_case.exported_optimal_block_size,
                                                 io_test_object_p->io_case.imported_block_size,
                                                 &block_status,
                                                 &block_qualifier,
                                                 &qualifier );
        MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, 
                                 "Expected end edge read/check pattern to be successful.");
        MUT_ASSERT_INT_EQUAL(block_status, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS);
        MUT_ASSERT_INT_EQUAL(block_qualifier, FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE);
    }

    /* If there was a pre-read at the end, make sure the correct pattern
     * is still there.  The write would have put down a pattern first. 
     */
    if ((edge_lba + edge_blocks) >
        (io_test_object_p->current_lba + io_test_object_p->current_blocks))
    {
        fbe_block_count_t blocks;
        fbe_lba_t lba;
        blocks = (fbe_block_count_t)((edge_lba + edge_blocks) - 
                                     (io_test_object_p->current_lba + io_test_object_p->current_blocks));
        lba = (io_test_object_p->current_lba + io_test_object_p->current_blocks);

        /* Read the end area where the pre-read was issued.
         */
        status = fbe_test_io_read_check_pattern( io_test_object_p->object_id,
                                                 io_test_object_p->background_pattern,
                                                 0xAA,
                                                 lba,    /* lba */
                                                 blocks,    /* blocks */
                                                 io_test_object_p->io_case.exported_block_size,    /* requested block size */
                                                 io_test_object_p->io_case.exported_optimal_block_size,
                                                 io_test_object_p->io_case.imported_block_size,
                                                 &block_status,
                                                 &block_qualifier,
                                                 &qualifier );
        MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, 
                                 "Expected start edge read/check pattern to be successful.");
        MUT_ASSERT_INT_EQUAL(block_status, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS);
        MUT_ASSERT_INT_EQUAL(block_qualifier, FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE);
    }
    return(status);
}
/* end fbe_test_io_task_object_issue_compare_for_write() */

/*!***************************************************************
 * @fn fbe_test_io_task_object_issue_read(
 *            fbe_test_io_task_object_t *const io_test_object_p)
 ****************************************************************
 * @brief
 *  This function issues a single read IO.
 *
 * @param io_test_object_p - io object to use for test.
 *
 * @return
 *  The status of the test.
 *  Anything other than FBE_STATUS_OK indicates failure.
 *
 * HISTORY:
 *  05/20/08 - Created. RPF
 *
 ****************************************************************/
fbe_status_t 
fbe_test_io_task_object_issue_read(fbe_test_io_task_object_t *const io_test_object_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t qualifier;
    fbe_api_block_status_values_t block_status;

    /* Issue a read.
     */
    status = fbe_test_io_issue_read( io_test_object_p->object_id,
                                     FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,
                                     io_test_object_p->pass_count,
                                     io_test_object_p->current_lba,    /* lba */
                                     io_test_object_p->current_blocks,    /* blocks */
                                     io_test_object_p->io_case.exported_block_size,    /* requested block size */
                                     io_test_object_p->io_case.exported_optimal_block_size,
                                     io_test_object_p->io_case.imported_block_size,
                                     &block_status,
                                     &qualifier );
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, 
                             "Expected read to be successful (packet status).");
    MUT_ASSERT_INT_EQUAL_MSG(FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, block_status.status, 
                             "Expected read to be successful (block payload status).");
    MUT_ASSERT_INT_EQUAL_MSG(FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE, block_status.qualifier, 
                             "Expected block operation qualifier to be 0.");

    return(status);
}
/* end fbe_test_io_task_object_issue_read() */

/*!***************************************************************
 * @fn fbe_test_io_task_object_issue_verify(
 *            fbe_test_io_task_object_t *const io_test_object_p)
 ****************************************************************
 * @brief
 *  This function issues a single verify IO.
 *
 * @param io_test_object_p - io object to use for test.
 *
 * @return
 *  The status of the test.
 *  Anything other than FBE_STATUS_OK indicates failure.
 *
 * HISTORY:
 *  05/20/08 - Created. RPF
 *
 ****************************************************************/

fbe_status_t 
fbe_test_io_task_object_issue_verify(fbe_test_io_task_object_t *const io_test_object_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t qualifier;

    /* Issue a verify.
     */
    status = fbe_test_io_issue_verify( io_test_object_p->object_id,
                                  io_test_object_p->pass_count,
                                  io_test_object_p->current_lba,    /* lba */
                                  io_test_object_p->current_blocks,    /* blocks */
                                  io_test_object_p->io_case.exported_block_size,    /* requested block size */
                                  io_test_object_p->io_case.exported_optimal_block_size,
                                  &qualifier );
    MUT_ASSERT_INT_EQUAL_MSG(FBE_STATUS_OK, status, 
                             "Expected verify to be successful.");
    return(status);
}
/* end fbe_test_io_task_object_issue_verify() */

/*************************
 * end file fbe_test_io_object.c
 *************************/
