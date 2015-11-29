/***************************************************************************
 * Copyright (C) EMC Corporation 2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_test_io_api.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the api functions for testing I/O.
 * 
 *  The interface for this I/O test library is located in fbe_test_io_api.h
 *
 * @author
 *   8/8/2008:  Created. RPF
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_transport.h"
#include "fbe_topology.h"
#include "mut.h"
#include "fbe_test_io_api.h"
#include "fbe/fbe_api_rdgen_interface.h"

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

static fbe_api_rdgen_context_t fbe_test_io_rdgen_context;
struct fbe_test_io_params_s;
typedef fbe_status_t (*fbe_test_io_test_fn_t)(fbe_test_io_case_t *const cases_p,
                                              struct fbe_test_io_params_s *params_p);
/*!*******************************************************************
 * @struct fbe_test_io_params_t
 *********************************************************************
 * @brief
 *
 *********************************************************************/
typedef struct fbe_test_io_params_s
{
    fbe_object_id_t object_id;
    fbe_package_id_t package_id;
    fbe_rdgen_operation_t rdgen_operation;
    fbe_rdgen_pattern_t background_pattern;
    fbe_test_io_test_fn_t setup;
    fbe_test_io_test_fn_t test;
    fbe_lba_t current_lba;
    fbe_block_count_t current_blocks;
}
fbe_test_io_params_t;

/*!***************************************************************
 * fbe_test_io_fill_area
 ****************************************************************
 * @brief
 *  This function fills an area with a known pattern.
 *
 * @param io_case_p - io case to run for.
 * @param params_p - The parameters to use in test.
 *
 * @return
 *  The status of the test.
 *  Anything other than FBE_STATUS_OK indicates failure.
 *
 * @author
 *  05/20/08 - Created. RPF
 *
 ****************************************************************/

fbe_status_t fbe_test_io_fill_area(fbe_test_io_case_t *io_case_p,
                                   fbe_test_io_params_t *params_p)
{
    fbe_status_t status;
    fbe_lba_t lba = io_case_p->start_lba;
    fbe_block_count_t max_lba;
    fbe_api_rdgen_context_t *context_p = &fbe_test_io_rdgen_context;
    fbe_block_count_t max_blocks;
    /* Start out with writing the total number of blocks we will be reading.
     *  
     */
    max_lba = (fbe_block_count_t)(io_case_p->end_lba - lba + io_case_p->end_blocks);

    /* Then round the max_lba up to the optimal block size boundary if
     * needed. 
     */
    if (max_lba % io_case_p->exported_optimal_block_size)
    {
        max_lba += (io_case_p->exported_optimal_block_size - 
                            (max_lba % io_case_p->exported_optimal_block_size));
    }
    /* Try for a large block count 0x800 is a reasonable count.  Make sure it is 
     * no larger than the max lba. 
     */
    max_blocks = FBE_MIN(0x800, max_lba);
    status = fbe_api_rdgen_test_context_init(context_p, 
                                             params_p->object_id,
                                             FBE_CLASS_ID_INVALID, 
                                             params_p->package_id,
                                             params_p->rdgen_operation,
                                             params_p->background_pattern,
                                             1,    /* We do one full sequential pass. */
                                             0,    /* num ios not used */
                                             0,    /* time not used*/
                                             1,    /* threads */
                                             FBE_RDGEN_LBA_SPEC_SEQUENTIAL_INCREASING,
                                             0,    /* Start lba*/
                                             0,    /* Min lba */
                                             max_lba,    /* max lba */
                                             FBE_RDGEN_BLOCK_SPEC_CONSTANT,
                                             max_blocks,    /* Min blocks */
                                             max_blocks    /* Max blocks */ );
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_api_rdgen_test_context_run_all_tests(context_p, FBE_PACKAGE_ID_NEIT, 1);

    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_EQUAL(context_p->start_io.status.status, FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL(context_p->start_io.status.block_status, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS);
    // Remove this check, because sometime the return has remap qualifier. MUT_ASSERT_INT_EQUAL(context_p->start_io.status.block_qualifier, FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE);
    
    return status;
}
/* end fbe_test_io_fill_area() */

/*!***************************************************************
 * fbe_test_io_write_read_compare
 ****************************************************************
 * @brief
 *  This function fills an area with a known pattern.
 *
 * @param io_case_p - io case to run for.
 * @param params_p - The parameters to use in test.
 *
 * @return
 *  The status of the test.
 *  Anything other than FBE_STATUS_OK indicates failure.
 *
 * @author
 *  05/20/08 - Created. RPF
 *
 ****************************************************************/

fbe_status_t fbe_test_io_write_read_compare(fbe_test_io_case_t *io_case_p,
                                            fbe_test_io_params_t *params_p)
{
    fbe_status_t status;
    fbe_api_rdgen_context_t *context_p = &fbe_test_io_rdgen_context;

    status = fbe_api_rdgen_test_context_init(context_p, 
                                             params_p->object_id, 
                                             FBE_CLASS_ID_INVALID,
                                             params_p->package_id,
                                             params_p->rdgen_operation,
                                             FBE_RDGEN_PATTERN_LBA_PASS,
                                             1,    /* One pass */
                                             0,    /* io count not used */
                                             0,    /* time not used*/
                                             1,
                                             FBE_RDGEN_LBA_SPEC_SEQUENTIAL_INCREASING,
                                             io_case_p->start_lba,    /* Start lba*/
                                             io_case_p->start_lba,    /* Min lba */
                                             io_case_p->end_lba, /* max lba */
                                             FBE_RDGEN_BLOCK_SPEC_INCREASING,
                                             io_case_p->start_blocks,    /* Min blocks */
                                             io_case_p->end_blocks    /* Max blocks */ );
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Set up the test to that we will iterate over both lba and blocks.
     */
    fbe_api_rdgen_io_specification_set_inc_lba_blocks(&context_p->start_io.specification,
                                                      io_case_p->increment_blocks,
                                                      0);

    /* Run the test.
     */
    status = fbe_api_rdgen_test_context_run_all_tests(context_p, FBE_PACKAGE_ID_NEIT, 1);

    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_EQUAL(context_p->start_io.status.status, FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL(context_p->start_io.status.block_status, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS);
    MUT_ASSERT_INT_EQUAL(context_p->start_io.status.block_qualifier, FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE);
    return status;
}
/**************************************
 * end fbe_test_io_write_read_compare()
 **************************************/
/*!***************************************************************
 * fbe_test_io_loop_over_cases()
 ****************************************************************
 * @brief
 *  This function performs an io test.
 * 
 * @param io_test_object_p - Test object to run cases for.
 * @param cases_p - These are the test cases we are executing.
 * @param max_case - This is the maximum case index we will execute.
 *
 * @return
 *  The status of the test.
 *  Anything other than FBE_STATUS_OK indicates failure.
 *
 * @author
 *  5/3/2010 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t fbe_test_io_loop_over_cases(fbe_test_io_case_t *const cases_p,
                                         fbe_test_io_params_t *params_p,
                                         fbe_u32_t max_case)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t index = 0;

    /* Loop over all of our test cases.
     * Stop when either we hit the max case or 
     * when we hit the end of this test array (FBE_TEST_IO_INVALID_FIELD). 
     */
    while (cases_p[index].exported_block_size != FBE_TEST_IO_INVALID_FIELD &&
           index < max_case &&
           status == FBE_STATUS_OK)
    {
        mut_printf(MUT_LOG_MEDIUM, "fbe test io case slba: 0x%llx elba: 0x%llx "
                   "sbl: 0x%llx ebl: 0x%llx exp: 0x%x imp_bl: 0x%x Status: %d", 
                   (unsigned long long)cases_p[index].start_lba,
		   (unsigned long long)cases_p[index].end_lba, 
                   (unsigned long long)cases_p[index].start_blocks,
		   (unsigned long long)cases_p[index].end_blocks,
                   cases_p[index].exported_block_size,
                   cases_p[index].imported_block_size,
                   status);
        status = params_p->setup(&cases_p[index], params_p);
        if (status != FBE_STATUS_OK)
        {
            return status;
        }
        status = params_p->test(&cases_p[index], params_p);
        
        if (status != FBE_STATUS_OK)
        {
            return status;
        }
        index++;
    } /* End while cases_p[index].exported_block_size != 0 */

    return status;
}
/* end fbe_test_io_loop_over_cases() */
/*!***************************************************************
 *  fbe_test_io_run_write_read_check_test()
 ****************************************************************
 * @brief
 *  This function performs a write/read/check test.
 *
 * @param cases_p - the io case to test with.
 * @param object_id - object id to test.
 * @param max_case_index - maximum case number to run.
 *
 * @return
 *  The status of the test.
 *  Anything other than FBE_STATUS_OK indicates failure.
 *
 * @author
 *  02/12/08 - Created. RPF
 *
 ****************************************************************/
fbe_status_t 
fbe_test_io_run_write_read_check_test(fbe_test_io_case_t *const cases_p,
                                      fbe_object_id_t object_id,
                                      fbe_u32_t max_case_index)
{
    fbe_test_io_params_t test_params;
    test_params.object_id = object_id;
    test_params.package_id = FBE_PACKAGE_ID_PHYSICAL;
    test_params.rdgen_operation = FBE_RDGEN_OPERATION_WRITE_READ_CHECK;
    test_params.background_pattern = FBE_RDGEN_PATTERN_ZEROS;
    test_params.setup = fbe_test_io_fill_area;
    test_params.test = fbe_test_io_write_read_compare;
    /* Initialize the functions for this test.
     * This will loop over the range of test cases, 
     * issue writes, read the data back, and validate it is an lba seeded 
     * pattern. 
     */
    return fbe_test_io_loop_over_cases(cases_p, &test_params, max_case_index);
}
/* end fbe_test_io_run_write_read_check_test() */

/*!***************************************************************
 * @fn fbe_test_io_run_write_verify_read_check_test(
 *                         fbe_test_io_case_t *const cases_p,
 *                         fbe_object_id_t object_id,
 *                         fbe_u32_t max_case_index)
 ****************************************************************
 * @brief
 *  This function performs a write verify/read/check test.
 *
 * @param cases_p - the io case to test with.
 * @param object_id - object id to test.
 * @param max_case_index - maximum case number to run.
 *
 * @return
 *  The status of the test.
 *  Anything other than FBE_STATUS_OK indicates failure.
 *
 * @author
 *  12/09/08 - Created. RPF
 *
 ****************************************************************/
fbe_status_t 
fbe_test_io_run_write_verify_read_check_test(fbe_test_io_case_t *const cases_p,
                                             fbe_object_id_t object_id,
                                             fbe_u32_t max_case_index)
{
    fbe_test_io_params_t test_params;
    test_params.object_id = object_id;
    test_params.package_id = FBE_PACKAGE_ID_PHYSICAL;
    test_params.rdgen_operation = FBE_RDGEN_OPERATION_WRITE_VERIFY_READ_CHECK;
    test_params.background_pattern = FBE_RDGEN_PATTERN_ZEROS;
    test_params.setup = fbe_test_io_fill_area;
    test_params.test = fbe_test_io_write_read_compare;
    /* Initialize the functions for this test.
     * This will loop over the range of test cases, 
     * issue writes, read the data back, and validate it is an lba seeded 
     * pattern. 
     */
    return fbe_test_io_loop_over_cases(cases_p, &test_params, max_case_index);
}
/* end fbe_test_io_run_write_verify_read_check_test() */

/*!***************************************************************
 * @fn fbe_test_io_run_write_same_test(
 *                         fbe_test_io_case_t *const cases_p,
 *                         fbe_object_id_t object_id,
 *                         fbe_u32_t max_case_index)
 ****************************************************************
 * @brief
 *  This function performs a write same io test.
 *
 * @param cases_p - the io case to test with.
 * @param object_id - object id to test.
 * @param max_case_index - The highest case index to test.
 *
 * @return
 *  The status of the test.
 *  Anything other than FBE_STATUS_OK indicates failure.
 *
 * @author
 *  02/12/08 - Created. RPF
 *
 ****************************************************************/
fbe_status_t 
fbe_test_io_run_write_same_test(fbe_test_io_case_t *const cases_p,
                                fbe_object_id_t object_id,
                                fbe_u32_t max_case_index)
{
    fbe_test_io_params_t test_params;
    test_params.object_id = object_id;
    test_params.package_id = FBE_PACKAGE_ID_PHYSICAL;
    test_params.rdgen_operation = FBE_RDGEN_OPERATION_WRITE_SAME_READ_CHECK;
    test_params.background_pattern = FBE_RDGEN_PATTERN_ZEROS;
    test_params.setup = fbe_test_io_fill_area;
    test_params.test = fbe_test_io_write_read_compare;

    /* Initialize the functions for this test.
     * This will loop over the range of test cases, 
     * issue writes, read the data back, and validate it is an lba seeded 
     * pattern. 
     */
    return fbe_test_io_loop_over_cases(cases_p, &test_params, max_case_index);
}
/* end fbe_test_io_run_write_same_test() */

/*!***************************************************************
 * @fn fbe_test_io_run_write_only_test(
 *                         fbe_test_io_case_t *const cases_p,
 *                         fbe_object_id_t object_id,
 *                         fbe_u32_t max_case_index)
 ****************************************************************
 * @brief
 *  This function performs a write only test.
 *
 * @param cases_p - the io case to test with.
 * @param object_id - object id to test.
 * @param max_case_index - The highest case index to test.
 *
 * RETURNS:
 *  The status of the test.
 *  Anything other than FBE_STATUS_OK indicates failure.
 *
 * @author
 *  02/12/08 - Created. RPF
 *
 ****************************************************************/
fbe_status_t 
fbe_test_io_run_write_only_test(fbe_test_io_case_t *const cases_p,
                                fbe_object_id_t object_id,
                                fbe_u32_t max_case_index)
{
    fbe_test_io_params_t test_params;
    test_params.object_id = object_id;
    test_params.package_id = FBE_PACKAGE_ID_PHYSICAL;
    test_params.rdgen_operation = FBE_RDGEN_OPERATION_WRITE_ONLY;
    test_params.background_pattern = FBE_RDGEN_PATTERN_ZEROS;
    test_params.setup = fbe_test_io_fill_area;
    test_params.test = fbe_test_io_write_read_compare;

    /* Initialize the functions for this test.
     * This will loop over the range of test cases, 
     * issue writes, read the data back, and validate it is an lba seeded 
     * pattern. 
     */
    return fbe_test_io_loop_over_cases(cases_p, &test_params, max_case_index);
}
/* end fbe_test_io_run_write_only_test() */

/*!***************************************************************
 * @fn fbe_test_io_run_read_only_test(
 *                         fbe_test_io_case_t *const cases_p,
 *                         fbe_object_id_t object_id,
 *                         fbe_u32_t max_case_index)
 ****************************************************************
 * @brief
 *  This function performs a read only test.
 *
 * @param cases_p - the io case to test with.
 * @param object_id - object id to test.
 * @param max_case_index - The highest case index to test.
 *
 * @return
 *  The status of the test.
 *  Anything other than FBE_STATUS_OK indicates failure.
 *
 * @author
 *  02/12/08 - Created. RPF
 *
 ****************************************************************/
fbe_status_t 
fbe_test_io_run_read_only_test(fbe_test_io_case_t *const cases_p,
                               fbe_object_id_t object_id,
                               fbe_u32_t max_case_index)
{
    fbe_test_io_params_t test_params;
    test_params.object_id = object_id;
    test_params.package_id = FBE_PACKAGE_ID_PHYSICAL;
    test_params.rdgen_operation = FBE_RDGEN_OPERATION_READ_ONLY;
    test_params.background_pattern = FBE_RDGEN_PATTERN_ZEROS;
    test_params.setup = fbe_test_io_fill_area;
    test_params.test = fbe_test_io_write_read_compare;

    /* Initialize the functions for this test.
     * This will loop over the range of test cases, 
     * issue writes, read the data back, and validate it is an lba seeded 
     * pattern. 
     */
    return fbe_test_io_loop_over_cases(cases_p, &test_params, max_case_index);
}
/* end fbe_test_io_run_read_only_test() */

/*!***************************************************************
 * @fn fbe_test_io_run_verify_test(
 *                         fbe_test_io_case_t *const cases_p,
 *                         fbe_object_id_t object_id,
 *                         fbe_u32_t max_case_index)
 ****************************************************************
 * @brief
 *  This function performs a verify test.
 *
 * @param cases_p - the io case to test with.
 * @param object_id - object id to test.
 * @param max_case_index - The highest case index to test.
 *
 * @return
 *  The status of the test.
 *  Anything other than FBE_STATUS_OK indicates failure.
 *
 * @author
 *  02/12/08 - Created. RPF
 *
 ****************************************************************/
fbe_status_t 
fbe_test_io_run_verify_test(fbe_test_io_case_t *const cases_p,
                            fbe_object_id_t object_id,
                            fbe_u32_t max_case_index)
{

    fbe_test_io_params_t test_params;
    test_params.object_id = object_id;
    test_params.package_id = FBE_PACKAGE_ID_PHYSICAL;
    test_params.rdgen_operation = FBE_RDGEN_OPERATION_VERIFY;
    test_params.background_pattern = FBE_RDGEN_PATTERN_ZEROS;
    test_params.setup = fbe_test_io_fill_area;
    test_params.test = fbe_test_io_write_read_compare;

    /* Initialize the functions for this test.
     * This will loop over the range of test cases, 
     * issue writes, read the data back, and validate it is an lba seeded 
     * pattern. 
     */
    return fbe_test_io_loop_over_cases(cases_p, &test_params, max_case_index);
}
/* end fbe_test_io_run_verify_test() */

/*************************
 * end file fbe_test_io.c
 *************************/
