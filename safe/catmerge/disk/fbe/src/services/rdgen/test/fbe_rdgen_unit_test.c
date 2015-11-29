/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_rdgen_unit_test.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the rdgen unit testing code.
 *
 * @version
 *   8/20/2009:  Created. Rob Foley
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/

#include "fbe/fbe_api_rdgen_interface.h"
#include "fbe/fbe_transport.h"
#include "mut.h"

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

/*!*******************************************************************
 * @def FBE_RDGEN_TEST_STANDARD_CASE_THREAD_COUNT
 *********************************************************************
 * @brief This is the max number of threads we will test with.
 *        This is an arbitrary number.
 *
 *********************************************************************/
#define FBE_RDGEN_TEST_STANDARD_CASE_THREAD_COUNT 2

/*!*******************************************************************
 * @def FBE_RDGEN_TEST_STANDARD_CASE_IO_COUNT
 *********************************************************************
 * @brief This is the max number of I/Os we will test with.
 *        This is an arbitrary number.
 *
 *********************************************************************/
#define FBE_RDGEN_TEST_STANDARD_CASE_IO_COUNT 2

/*!*******************************************************************
 * @def FBE_RDGEN_MAX_TEST_CONTEXTS
 *********************************************************************
 * @brief This is the number of tests we will execute in parallel.
 *        This is an arbitrary number.
 *
 *********************************************************************/
#define FBE_RDGEN_MAX_TEST_CONTEXTS 10

/*!*******************************************************************
 * @var fbe_rdgen_test_contexts
 *********************************************************************
 * @brief This has all the context objects with packet and
 *        test parameters.
 *
 *********************************************************************/
fbe_api_rdgen_context_t fbe_rdgen_test_contexts[FBE_RDGEN_MAX_TEST_CONTEXTS];

/*!*******************************************************************
 * @var fbe_rdgen_unit_test_object_id
 *********************************************************************
 * @brief This is the global to keep track of our object under test.
 *
 *********************************************************************/
static fbe_object_id_t fbe_rdgen_unit_test_object_id = 8;

/*!*******************************************************************
 * @var fbe_rdgen_unit_test_physical_object_id
 *********************************************************************
 * @brief This is the global to keep track of our physical package
 *        object under test.
 *        1 port + 1 board + 1 enclosure + 5 physical drives
 *        This id is the object id of the first logical drive.
 *
 *********************************************************************/
static fbe_object_id_t fbe_rdgen_unit_test_physical_object_id = 8;

/*!*******************************************************************
 * @var fbe_rdgen_unit_test_class_id
 *********************************************************************
 * @brief This is the global to keep track of our class under test.
 *
 *********************************************************************/
static fbe_class_id_t fbe_rdgen_unit_test_class_id = FBE_CLASS_ID_INVALID;

/*!*******************************************************************
 * @var fbe_rdgen_unit_test_package_id
 *********************************************************************
 * @brief package id to test.
 *
 *********************************************************************/
static fbe_package_id_t fbe_rdgen_unit_test_package_id = FBE_PACKAGE_ID_PHYSICAL;

/*!*******************************************************************
 * @var fbe_rdgen_unit_test_service_package_id
 *********************************************************************
 * @brief package id of rdgen service.
 *
 *********************************************************************/
static fbe_package_id_t fbe_rdgen_unit_test_service_package_id = FBE_PACKAGE_ID_NEIT;

/*!*******************************************************************
 * @struct fbe_rdgen_unit_test_case_t
 *********************************************************************
 * @brief
 *
 *********************************************************************/
typedef struct fbe_rdgen_unit_test_case_s
{
    /* Information on how to choose lbas.
     */
    fbe_lba_t start_lba;
    fbe_lba_t min_lba;
    fbe_lba_t max_lba;

    /* Information on how to choose blocks.
     */
    fbe_u32_t min_blocks;
    fbe_u32_t max_blocks;
}
fbe_rdgen_unit_test_case_t;

/*!*******************************************************************
 * @def FBE_RDGEN_TEST_INVALID_FIELD
 *********************************************************************
 * @brief Marks the end of a test case.
 *
 *********************************************************************/
#define FBE_RDGEN_TEST_INVALID_FIELD 0xffffffff

/************************************************** 
 * IMPORTED FUNCTIONS 
 **************************************************/
void fbe_rdgen_test_setup(void);
void fbe_rdgen_test_teardown(void);

/*!*******************************************************************
 * @var fbe_rdgen_test_normal_cases
 *********************************************************************
 * @brief This is the set of normal non-error cases.
 *
 *********************************************************************/
fbe_rdgen_unit_test_case_t fbe_rdgen_test_normal_cases[] =
{
    /* start_lba  min_lba max_lba    min_blocks max_blocks */

    /* Simple case with lba range matching max blocks.
     */
    {   0,        0,      0xff,    1,         0x100  },
    {   0,        0,      0x1ff,   1,         0x200  },

    /* Change start to something smaller than normal I/O range.
     * Make I/O range start at something other than 0.
     */
    {  0x100,     0x200,  0x4ff,    1,         0x300  },

    /* cases where block range is fixed.
     */
    {   0,        0,      0x4fff,    1,           1 },
    {   0,        0,      0x4fff,    10,          10 },
    {   0,        0,      0x4fff,    0x10,      0x10 },

    /* Block range maxes out at lba range.
     */
    {   0,        0x80,   0x100,      1,         0x80 },
    {   0,        0x10,   0x100,      1,         0xe0 },
    {   0,        0x100,  0x110,     0x10,       0x10 },
    {   0,        0x10,   0x1f,       1,         0x10 },

    /* Fixed block range matching lba range.
     */
    {   0,        0x100,  0x200,     0x100,     0x100 },
    {   0,         0x50,   0xff,     0xB0,      0xB0  },
    {   10,       0x200,  0x2ff,     0x100,     0x100  },
    {   5,        0x20,   0x2f,      0x10,      0x10  },
    {   0,        0x10,   0x110,     0x100,     0x100 },

    /* Block range maxes out at lba range.
     */
    {   1,        0x50,    0xff,     1,         0xB0  },
    {   10,       0x20,    0xff,     1,         0xE0  },
    {   0x100,    0x200,  0x2ff,     1,         0x100  },

    /* Start lba is below normal lba range.  Blocks fixed. 
     */
    {   0x100,    0x200,  0x5000,    1,           1 },
    {   0x10,     0x200,  0x3000,    0x10,      0x10 },
    {   0x200,    0x500,  0x3000,    0x100,     0x100 },

    /* Start lba is less than normal lba range.  Blocks max out at lba range size. 
     */
    {   0,        0x10,   0x10,      1,           1 },
    {   0,        0x100,  0x100,     1,           1 },
    {   0,        0x100,  0x10f,     0x10,      0x10 },
    {   0,        0x10,    0x1f,     0x10,      0x10 },
    {   0,        0x100,  0x200,     0x100,     0x100 },

    /* Start lba is greater than normal lba range.  Blocks max out at lba range size. 
     */
    {   0x50,     0x100,  0x200,     1,         0x100 },
    {   0x50,     0x100,  0x100,     1,           1 },
    {   0x50,     0x100,  0x10f,     0x10,      0x10 },
    {   5,        0x10,   0x1f,      0x10,      0x10 },
    {   0x50,     0x100,  0x200,     0x100,     0x100 },
    {   FBE_RDGEN_TEST_INVALID_FIELD },
};
/**************************************
 * end fbe_rdgen_test_normal_cases
 **************************************/

/*!*******************************************************************
 * @var fbe_rdgen_test_multi_thread_normal_cases
 *********************************************************************
 * @brief This is the set of normal non-error cases.
 *
 *********************************************************************/
fbe_rdgen_unit_test_case_t fbe_rdgen_test_multi_thread_normal_cases[] =
{
    {   0,     0,  0x5000,     0x1,     0x10 },
    {   0,     0,  0x5000,     0x1,     0x100 },
    {   0,     0,  0x5000,     0x1,     0x100 },
    {   FBE_RDGEN_TEST_INVALID_FIELD },
};
/**************************************
 * end fbe_rdgen_test_multi_thread_normal_cases
 **************************************/
/*!*******************************************************************
 * @var fbe_rdgen_test_error_cases
 *********************************************************************
 * @brief These are cases for which we expect to get back errors.
 *
 *********************************************************************/
fbe_rdgen_unit_test_case_t fbe_rdgen_test_error_cases[] =
{
    /* start_lba  min_lba max_lba    min_blocks max_blocks */

    /* Min lba larger than max lba.
     */
    {  0,         500,    200,       1,         50  },
    {  0,         2,      1,         1,         50  },
    {  0,         1,      0,         1,         50  },
    {  0,         5000,   0,         1,         50  },

    /* Zero starting block range.
     */
    {   0,        0x100,  0x100,     0,         1 },
    {   10,       0x200,  0x5000,    0,         0x500  },
    {   0x5000,   0,      0x500,     0,         0x300 },

    /* Zero ending  block range.
     */
    {   0,        0x100,  0x100,     1,         0 },
    {   10,       0x0,    0x4fff,    0x5000,    0  },
    {   0x5000,   0,      0x4ff,     0x500,     0 },

    /* Ending block range smaller than starting block range.
     */
    {   0,        0x100,  0x10f,     0x10,      0xf },
    {   0,        0x100,  0x10f,     0x10,      0x5 },
    {   10,       0,      0x4fff,    0x5000,    0x4fff },
    {   10,       0,      0x4fff,    0x5000,    0x200 },
    {   0x5000,   0,      0x2ff,     0x300,     0x1 },
    {   0x5000,   0,      0x2ff,     0x300,     0x2ff },

    /* Block range is larger than lba range.
     */
    {   0,        0x2000, 0x5000,    1,         0x5000  },
    {   0,        0x500,  0x5000,    1,         0x5000  },
    {   10,       0x200,  0x5000,    1,         0x5000  },
    {   0,        0x2000, 0x3000,    1,         0x5000  },
    {   0,        0x2000, 0x2fff,    1,         0x1001  },
    {   0,        0x2000, 0x2fff,    0x1001,    0x1001  },

    {   0,        0x100,  0x100,     0x200,     0x200 },
    {   0,        0x100,  0x110,     100,       100 },
    {   0,        0x100,  0x1100,    0x1100,    0x1100 },
    {   0,        0x100,  0x1ff,     0x101,     0x101 },

    /* lba range is smaller than the min blocks.
     */
    {   0,        0x700,  0x7ff,     0x101,     0x101  },
    {   0,        0x500,  0x6ff,     0x201,     0x201  },
    {   0,        0x200,  0x4ff,     0x301,     0x301  },
    {   0,        0x2000, 0x2fff,    0x1002,    0x1002  },
    {   0,        0x2000, 0x2fff,    0x1001,    0x1001  },
    {   0,        0x2000, 0x2fff,    0x1001,    0x1001  },

    {   0,        0x100,  0x100,     0x2,       0x2 },
    {   0,        0x100,  0x110,     0x100,     0x100 },
    {   0,        0x100,  0x1100,    0x1100,    0x1100 },
    {   0,        0x100,  0x1ff,     0x101,     0x101 },

    /* start lba range (start lba to max lba) is smaller than the min blocks.
     */
    {   0x701,    0x700,  0x7ff,     0x100,     0x100  },
    {   0x580,    0x500,  0x6ff,     0x200,     0x200  },
    {   0x201,    0x200,  0x4ff,     0x300,     0x300  },
    {   0x2001,   0x2000, 0x2fff,    0x1000,    0x1000  },
    {   0x2fff,   0x2000, 0x2fff,    0x1000,    0x1000  },
    {   0x2080,   0x2000, 0x2fff,    0x1000,    0x1000  },

    {   0x101,    0x100,  0x101,     0x2,       0x2 },
    {   0x101,    0x100,  0x10f,     0x10,      0x10 },
    {   0x200,    0x100,  0x10ff,    0x1000,    0x1000 },
    {   0x101,    0x100,  0x1ff,     0x100,     0x100 },

    /* start lba is larger than max lba.
     */
    {   0x800,    0x700,  0x7ff,     0x100,     0x100  },
    {   0x680,    0x500,  0x6ff,     0x100,     0x100  },
    {   0x501,    0x200,  0x4ff,     0x300,     0x300  },
    {   0x3000,   0x2000, 0x2fff,    0x1000,    0x1000  },
    {   0x3080,   0x2000, 0x2fff,    0x1000,    0x1000  },
    {   0x5000,   0x2000, 0x2fff,    0x1000,    0x1000  },

    {   0x102,    0x100,  0x101,     0x2,       0x2 },
    {   0x110,    0x100,  0x10f,     0x10,      0x10 },
    {   0x111,    0x100,  0x10f,     0x10,      0x10 },
    {   0x1101,   0x100,  0x10ff,    0x1000,    0x1000 },
    {   0x200,    0x100,  0x1ff,     0x100,     0x100 },
    {   FBE_RDGEN_TEST_INVALID_FIELD },
};
/**************************************
 * end fbe_rdgen_test_error_cases
 **************************************/

/*!**************************************************************
 * fbe_rdgen_execute_cases()
 ****************************************************************
 * @brief
 *  Test over a set of test cases.
 * 
 * @param operation - the operation to kick off.
 * @param object_id - the object id to test.
 * @param class_id - the class id to test.
 * @param num_ios - number of I/Os to send.
 * @param time - Time to execute.
 * @param threads - number of threads to kick off.
 * @param test_case_p - the test case to run.
 * @param b_error_expected - true if we expect an error.
 *
 * @return None.
 *
 * @author
 *  9/2/2009 - Created. Rob Foley
 *
 ****************************************************************/

void fbe_rdgen_execute_cases(fbe_rdgen_operation_t operation,
                             fbe_object_id_t object_id,
                             fbe_class_id_t class_id,
                             fbe_u32_t num_ios,
                             fbe_time_t time,
                             fbe_u32_t threads,
                             fbe_rdgen_unit_test_case_t *test_case_p,
                             fbe_bool_t b_error_expected)
{
    fbe_status_t status;
    fbe_rdgen_lba_specification_t lba_spec;
    fbe_rdgen_block_specification_t block_spec;
    fbe_api_rdgen_context_t *context_p = &fbe_rdgen_test_contexts[0];

    /* Loop over all possible lba and block specs.
     */
    for (lba_spec = FBE_RDGEN_LBA_SPEC_RANDOM; lba_spec < FBE_RDGEN_LBA_SPEC_LAST; lba_spec++)
    {
        /* If the threads are guaranteed to overlap then do not issue them.
         */
        if (!b_error_expected && 
            (threads > 1) && 
            ((lba_spec == FBE_RDGEN_LBA_SPEC_FIXED) ||
             ((test_case_p->start_lba == test_case_p->min_lba) &&
              (test_case_p->max_lba == test_case_p->min_lba))))
        {
            continue;
        }
        for (block_spec = FBE_RDGEN_BLOCK_SPEC_RANDOM; block_spec <= FBE_RDGEN_BLOCK_SPEC_STRIPE_SIZE; block_spec++)
        {
            /* Initialize the context.
             */
            status = fbe_api_rdgen_test_context_init(context_p,
                                                 object_id, 
                                                 class_id,
                                                 fbe_rdgen_unit_test_package_id,
                                                 operation,
                                                 FBE_RDGEN_PATTERN_LBA_PASS,
                                                 0, /* passes not used */
                                                 num_ios,
                                                 time,
                                                 threads,
                                                 lba_spec,
                                                 test_case_p->start_lba,
                                                 test_case_p->min_lba,
                                                 test_case_p->max_lba,
                                                 block_spec,
                                                 test_case_p->min_blocks,
                                                 test_case_p->max_blocks);

            /* Only assert if we did not expect error.
             * If we expect error, this may or may not fail. 
             */
            if (!b_error_expected)
            {
                MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
            }

            /* Execute the actual test.
             * Assert that the test was started OK. 
             */
            status = fbe_api_rdgen_run_tests(&fbe_rdgen_test_contexts[0], 
                                              fbe_rdgen_unit_test_service_package_id, 1);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

            status = fbe_api_rdgen_get_status(&fbe_rdgen_test_contexts[0], 1);

            /* Assert differently if errors were expected.
             * If an error was expected this should fail. 
             */
            if (b_error_expected)
            {
                MUT_ASSERT_INT_NOT_EQUAL(status, FBE_STATUS_OK);
            }
            else
            {
                MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
            }

            /* Cleanup the context.
             */
            status = fbe_api_rdgen_test_context_destroy(context_p);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        }    /* end for all block specs */
    }    /* end for all lba specs */
    return;
}
/******************************************
 * end fbe_rdgen_execute_cases()
 ******************************************/
/*!**************************************************************
 * fbe_rdgen_test_standard_cases()
 ****************************************************************
 * @brief
 *  This tests verify test cases.
 *
 * @param operation - op to specify in the test.               
 *
 * @return None.
 *
 * @author
 *  9/2/2009 - Created. Rob Foley
 *
 ****************************************************************/

void fbe_rdgen_test_standard_cases(fbe_rdgen_operation_t op)
{
    /* We know the object ids under test are 0.
     */
    fbe_u32_t threads;
    fbe_rdgen_unit_test_case_t *test_case_p = NULL;

    /* loop over all the test cases.
     */
    for (test_case_p = &fbe_rdgen_test_normal_cases[0]; 
        test_case_p->start_lba != FBE_RDGEN_TEST_INVALID_FIELD;
        test_case_p++)
    {
        /* Loop over a predetermined number of thread counts.
         */
        for ( threads = 1; threads <= FBE_RDGEN_TEST_STANDARD_CASE_THREAD_COUNT; threads += 1)
        {
            fbe_u32_t num_ios;
            fbe_u32_t max_threads = (fbe_u32_t)((test_case_p->max_lba - test_case_p->min_lba + 1) / test_case_p->min_blocks);

            /* Make sure we do not test with an invalid thread count.
             */
            if (max_threads < threads)
            {
                break;
            }
            /* Loop over a predetermined number of IO counts.  
             */ 
            for ( num_ios = 1; num_ios <= FBE_RDGEN_TEST_STANDARD_CASE_IO_COUNT; num_ios++)
            {
                fbe_rdgen_execute_cases(op,
                                        fbe_rdgen_unit_test_object_id,
                                        fbe_rdgen_unit_test_class_id,
                                        num_ios,    /* num_ios */
                                        0,          /* time not used */
                                        threads,
                                        test_case_p,
                                        FALSE    /* no errors expected */);
            }
        }
    }    /* end for all test cases*/
    return;
}
/******************************************
 * end fbe_rdgen_test_standard_cases()
 ******************************************/
/*!**************************************************************
 * fbe_rdgen_test_verify()
 ****************************************************************
 * @brief
 *  This tests verify test cases.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  9/2/2009 - Created. Rob Foley
 *
 ****************************************************************/

void fbe_rdgen_test_verify(void)
{
    fbe_rdgen_test_standard_cases(FBE_RDGEN_OPERATION_VERIFY);
    return;
}
/******************************************
 * end fbe_rdgen_test_verify()
 ******************************************/
/*!**************************************************************
 * fbe_rdgen_test_write_read_compare()
 ****************************************************************
 * @brief
 *  This tests all write/read/compare cases.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  9/2/2009 - Created. Rob Foley
 *
 ****************************************************************/

void fbe_rdgen_test_write_read_compare(void)
{
    fbe_rdgen_test_standard_cases(FBE_RDGEN_OPERATION_WRITE_READ_CHECK);
    return;
}
/******************************************
 * end fbe_rdgen_test_write_read_compare()
 ******************************************/
/*!**************************************************************
 * fbe_rdgen_test_write_only()
 ****************************************************************
 * @brief
 *  This tests all write only cases.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  9/2/2009 - Created. Rob Foley
 *
 ****************************************************************/

void fbe_rdgen_test_write_only(void)
{
    fbe_rdgen_test_standard_cases(FBE_RDGEN_OPERATION_WRITE_ONLY);
    return;
}
/******************************************
 * end fbe_rdgen_test_write_only()
 ******************************************/
/*!**************************************************************
 * fbe_rdgen_test_read_only()
 ****************************************************************
 * @brief
 *  This tests all read only cases.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  9/2/2009 - Created. Rob Foley
 *
 ****************************************************************/

void fbe_rdgen_test_read_only(void)
{
    fbe_rdgen_test_standard_cases(FBE_RDGEN_OPERATION_READ_ONLY);
    return;
}
/******************************************
 * end fbe_rdgen_test_read_only()
 ******************************************/
/*!**************************************************************
 * fbe_rdgen_test_fixed_pattern()
 ****************************************************************
 * @brief
 *  This tests a case where we send an invalid block spec.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  9/15/2009 - Created. Rob Foley
 *
 ****************************************************************/

void fbe_rdgen_test_fixed_pattern(fbe_rdgen_lba_specification_t lba_spec,
                                  fbe_rdgen_unit_test_case_t *test_case_p,
                                  fbe_rdgen_operation_t operation,
                                  fbe_u32_t num_ios)
{
    fbe_status_t status;
    fbe_api_rdgen_context_t *context_p = &fbe_rdgen_test_contexts[0];

    /* Issue a single request with an invalid operation.
     * Block spec is set to an invalid value.
     */
    status = fbe_api_rdgen_test_context_init(context_p,
                                         fbe_rdgen_unit_test_object_id,
                                         fbe_rdgen_unit_test_class_id,
                                         fbe_rdgen_unit_test_package_id,
                                         operation,
                                         FBE_RDGEN_PATTERN_LBA_PASS,
                                         0, /* passes not used */
                                         num_ios,    /* num ios */
                                         0,    /* time not used */
                                         1,  /* threads. */
                                         lba_spec,
                                         test_case_p->start_lba,
                                         test_case_p->min_lba,
                                         test_case_p->max_lba,
                                         FBE_RDGEN_BLOCK_SPEC_CONSTANT,
                                         test_case_p->min_blocks,
                                         test_case_p->max_blocks);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Execute the actual test.
     * We assert that the test was started OK. 
     */
    status = fbe_api_rdgen_run_tests(&fbe_rdgen_test_contexts[0], 
                                      fbe_rdgen_unit_test_service_package_id, 1);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Get the status from the context structure.
     * Make sure errors were encountered. 
     */
    status = fbe_api_rdgen_get_status(&fbe_rdgen_test_contexts[0], 1);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Cleanup the context.
     */
    status = fbe_api_rdgen_test_context_destroy(context_p);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    return;
}
/******************************************
 * end fbe_rdgen_test_fixed_pattern()
 ******************************************/
/*!**************************************************************
 * fbe_rdgen_test_read_compare()
 ****************************************************************
 * @brief
 *  This tests all read/compare cases.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  9/2/2009 - Created. Rob Foley
 *
 ****************************************************************/

void fbe_rdgen_test_read_compare(void)
{
    fbe_rdgen_unit_test_case_t *test_case_p = &fbe_rdgen_test_normal_cases[0];

    /* Write out a given pattern.
     */
    fbe_rdgen_test_fixed_pattern(FBE_RDGEN_LBA_SPEC_SEQUENTIAL_INCREASING,
                                 test_case_p,
                                 FBE_RDGEN_OPERATION_WRITE_ONLY,
                                 100);

    /* Read back the pattern and make sure it matched.
     */
    fbe_rdgen_test_fixed_pattern(FBE_RDGEN_LBA_SPEC_SEQUENTIAL_INCREASING,
                                 test_case_p,
                                 FBE_RDGEN_OPERATION_READ_CHECK,
                                 100);
    return;
}
/******************************************
 * end fbe_rdgen_test_read_compare()
 ******************************************/

/*!**************************************************************
 * fbe_rdgen_test_lba_block_errors()
 ****************************************************************
 * @brief
 *  This tests cases where we send invalid rderr commands,
 *  where either the lbas or block counts are incorrect.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  9/2/2009 - Created. Rob Foley
 *
 ****************************************************************/

void fbe_rdgen_test_lba_block_errors(void)
{
    /* We know the object ids under test are 0.
     */
    fbe_rdgen_unit_test_case_t *test_case_p = NULL;

    /* loop over all the test cases.
     */
    for (test_case_p = &fbe_rdgen_test_error_cases[0]; 
        test_case_p->start_lba != FBE_RDGEN_TEST_INVALID_FIELD;
        test_case_p++)
    {
        /* Loop over a predetermined number of  io counts.
         */
        fbe_rdgen_execute_cases(FBE_RDGEN_OPERATION_READ_ONLY,
                                fbe_rdgen_unit_test_object_id,
                                fbe_rdgen_unit_test_class_id,
                                100,    /* num_ios is arbitrary */
                                0,    /* time not used */
                                1,    /* threads */
                                test_case_p,
                                TRUE    /* errors expected */ );
    }    /* end for all test cases*/
    return;
}
/******************************************
 * end fbe_rdgen_test_lba_block_errors()
 ******************************************/

/*!**************************************************************
 * fbe_rdgen_test_thread_count_errors()
 ****************************************************************
 * @brief
 *  This tests cases where the thread count is invalid for
 *  the given lba range and min block count.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  9/2/2009 - Created. Rob Foley
 *
 ****************************************************************/

void fbe_rdgen_test_thread_count_errors(void)
{
    /* We know the object ids under test are 0.
     */
    fbe_rdgen_unit_test_case_t *test_case_p = NULL;

    /* loop over all the test cases.
     */
    for (test_case_p = &fbe_rdgen_test_normal_cases[0]; 
        test_case_p->start_lba != FBE_RDGEN_TEST_INVALID_FIELD;
        test_case_p++)
    {
        /* Calculate the max possible threads we could run to this lba range with the
         * specified lba count.
         */
        fbe_u32_t max_threads = 
            (fbe_u32_t)((test_case_p->max_lba - test_case_p->min_lba + 1) / test_case_p->min_blocks); 

        /* Loop over a predetermined number of io counts.
         */
        fbe_rdgen_execute_cases(FBE_RDGEN_OPERATION_READ_ONLY,
                                fbe_rdgen_unit_test_object_id,
                                fbe_rdgen_unit_test_class_id,
                                100,    /* num_ios is arbitrary */
                                0,    /* time not used */
                                max_threads + 1,    /* threads */
                                test_case_p,
                                TRUE    /* errors expected */ );
    }    /* end for all test cases*/
    return;
}
/******************************************
 * end fbe_rdgen_test_thread_count_errors()
 ******************************************/

/*!**************************************************************
 * fbe_rdgen_test_invalid_operation()
 ****************************************************************
 * @brief
 *  This tests cases where we send invalid rderr commands,
 *  where either the lbas or block counts are incorrect.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  9/2/2009 - Created. Rob Foley
 *
 ****************************************************************/

void fbe_rdgen_test_invalid_operation(void)
{
    fbe_status_t status;
    fbe_rdgen_unit_test_case_t *test_case_p = &fbe_rdgen_test_normal_cases[0];
    fbe_api_rdgen_context_t *context_p = &fbe_rdgen_test_contexts[0];

    /* Issue a single request with an invalid operation
     */
    status = fbe_api_rdgen_test_context_init(context_p,
                                         fbe_rdgen_unit_test_object_id, 
                                         fbe_rdgen_unit_test_class_id, 
                                         fbe_rdgen_unit_test_package_id,
                                         FBE_RDGEN_OPERATION_INVALID,
                                         FBE_RDGEN_PATTERN_LBA_PASS,
                                         0, /* passes not used */
                                         100,    /* num ios */
                                         0,    /* time not used */
                                         1,  /* threads */
                                         FBE_RDGEN_LBA_SPEC_RANDOM,
                                         test_case_p->start_lba,
                                         test_case_p->min_lba,
                                         test_case_p->max_lba,
                                         FBE_RDGEN_BLOCK_SPEC_RANDOM,
                                         test_case_p->min_blocks,
                                         test_case_p->max_blocks);
    MUT_ASSERT_INT_NOT_EQUAL(status, FBE_STATUS_OK);

    /* Execute the actual test.
     * We assert that the test was started OK. 
     */
    status = fbe_api_rdgen_run_tests(&fbe_rdgen_test_contexts[0], 
                                      fbe_rdgen_unit_test_service_package_id, 1);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Get the status from the context structure.
     * Make sure errors were encountered. 
     */
    status = fbe_api_rdgen_get_status(&fbe_rdgen_test_contexts[0], 1);
    MUT_ASSERT_INT_NOT_EQUAL(status, FBE_STATUS_OK);

    /* Cleanup the context.
     */
    status = fbe_api_rdgen_test_context_destroy(context_p);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    return;
}
/******************************************
 * end fbe_rdgen_test_invalid_operation()
 ******************************************/
/*!**************************************************************
 * fbe_rdgen_test_thread_count_cases()
 ****************************************************************
 * @brief
 *  This tests kicks off tests over the range of allowed
 *  thread counts for all operations.
 *
 * @param object_id - the object to test.
 * @param class_id - the class to test.     
 * @param package_id - the package to test.             
 * @param num_ios - number of I/Os to test.
 *
 * @return None.
 *
 * @author
 *  9/25/2009 - Created. Rob Foley
 *
 ****************************************************************/

void fbe_rdgen_test_thread_count_cases(fbe_object_id_t object_id,
                                       fbe_class_id_t class_id,
                                       fbe_package_id_t package_id,
                                       fbe_u32_t num_ios)
{
    fbe_status_t status;
    fbe_rdgen_unit_test_case_t *test_case_p = &fbe_rdgen_test_multi_thread_normal_cases[0];
    fbe_api_rdgen_context_t *context_p = &fbe_rdgen_test_contexts[0];
    fbe_rdgen_operation_t operation;
    fbe_u32_t threads;

    /* Iterate over the power of two thread counts.
     */
    for ( threads = 1; threads <= FBE_RDGEN_MAX_THREADS_PER_IO_SPECIFICATION; threads *= 2)
    {
        /* Iterate over all operation types.
         */
        for ( operation = FBE_RDGEN_OPERATION_READ_ONLY; operation <= FBE_RDGEN_OPERATION_VERIFY; operation++)
        {
            /* Issue a single request with an invalid operation
             */
            status = fbe_api_rdgen_test_context_init(context_p,
                                                 object_id,
                                                 class_id,
                                                 package_id,
                                                 operation,
                                                 FBE_RDGEN_PATTERN_LBA_PASS,
                                                 0, /* passes not used */
                                                 num_ios,   /* num ios */
                                                 0,    /* time not used */
                                                 threads,
                                                 FBE_RDGEN_LBA_SPEC_RANDOM,
                                                 test_case_p->start_lba,
                                                 test_case_p->min_lba,
                                                 test_case_p->max_lba,
                                                 FBE_RDGEN_BLOCK_SPEC_RANDOM,
                                                 test_case_p->min_blocks,
                                                 test_case_p->max_blocks);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

            /* Execute the actual test.
             * We assert that the test was started OK. 
             */
            status = fbe_api_rdgen_run_tests(&fbe_rdgen_test_contexts[0], 
                                              fbe_rdgen_unit_test_service_package_id, 1);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

            /* Get the status from the context structure.
             * Make sure errors were encountered. 
             */
            status = fbe_api_rdgen_get_status(&fbe_rdgen_test_contexts[0], 1);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

            /* Cleanup the context.
             */
            status = fbe_api_rdgen_test_context_destroy(context_p);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        }    /* end for all operations */
    }    /* end for all thread counts */
    return;
}
/******************************************
 * end fbe_rdgen_test_thread_count_cases()
 ******************************************/
/*!**************************************************************
 * fbe_rdgen_test_thread_counts()
 ****************************************************************
 * @brief
 *  This tests kicks off tests over the range of allowed
 *  thread counts for all operations.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  9/2/2009 - Created. Rob Foley
 *
 ****************************************************************/

void fbe_rdgen_test_thread_counts(void)
{
    /* First run I/O to a given object.
     */
    fbe_rdgen_test_thread_count_cases(fbe_rdgen_unit_test_object_id, 
                                      FBE_CLASS_ID_INVALID,
                                      fbe_rdgen_unit_test_package_id,
                                      1);
    fbe_rdgen_test_thread_count_cases(fbe_rdgen_unit_test_object_id, 
                                      FBE_CLASS_ID_INVALID,
                                      fbe_rdgen_unit_test_package_id,
                                      20);

    /* Test against the physical package.
     * First drive I/O from sep to physical package objects. 
     */
    fbe_rdgen_test_thread_count_cases(fbe_rdgen_unit_test_physical_object_id, 
                                      FBE_CLASS_ID_INVALID,
                                      FBE_PACKAGE_ID_PHYSICAL,
                                      1);
    fbe_rdgen_test_thread_count_cases(fbe_rdgen_unit_test_physical_object_id, 
                                      FBE_CLASS_ID_INVALID,
                                      FBE_PACKAGE_ID_PHYSICAL,
                                      20);
    return;
}
/******************************************
 * end fbe_rdgen_test_thread_counts()
 ******************************************/
/*!**************************************************************
 * fbe_rdgen_test_invalid_threads()
 ****************************************************************
 * @brief
 *  This tests cases where we send invalid rderr commands,
 *  where either the lbas or block counts are incorrect.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  9/2/2009 - Created. Rob Foley
 *
 ****************************************************************/

void fbe_rdgen_test_invalid_threads(void)
{
    fbe_status_t status;
    fbe_rdgen_unit_test_case_t *test_case_p = &fbe_rdgen_test_normal_cases[0];
    fbe_api_rdgen_context_t *context_p = &fbe_rdgen_test_contexts[0];

    /* Issue a single request with an invalid operation.
     * Threads is set too high to cause an error. 
     */
    status = fbe_api_rdgen_test_context_init(context_p,
                                         fbe_rdgen_unit_test_object_id, 
                                         fbe_rdgen_unit_test_class_id, 
                                         fbe_rdgen_unit_test_package_id,
                                         FBE_RDGEN_OPERATION_READ_ONLY,
                                         FBE_RDGEN_PATTERN_LBA_PASS,
                                         0, /* passes not used */
                                         100,    /* num ios */
                                         0,    /* time not used */
                                         FBE_RDGEN_MAX_THREADS_PER_IO_SPECIFICATION + 1,
                                         FBE_RDGEN_LBA_SPEC_RANDOM,
                                         test_case_p->start_lba,
                                         test_case_p->min_lba,
                                         test_case_p->max_lba,
                                         FBE_RDGEN_BLOCK_SPEC_RANDOM,
                                         test_case_p->min_blocks,
                                         test_case_p->max_blocks);
    MUT_ASSERT_INT_NOT_EQUAL(status, FBE_STATUS_OK);

    /* Execute the actual test.
     * We assert that the test was started OK. 
     */
    status = fbe_api_rdgen_run_tests(&fbe_rdgen_test_contexts[0], 
                                      fbe_rdgen_unit_test_service_package_id, 1);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Get the status from the context structure.
     * Make sure errors were encountered. 
     */
    status = fbe_api_rdgen_get_status(&fbe_rdgen_test_contexts[0], 1);
    MUT_ASSERT_INT_NOT_EQUAL(status, FBE_STATUS_OK);

    /* Cleanup the context.
     */
    status = fbe_api_rdgen_test_context_destroy(context_p);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    return;
}
/******************************************
 * end fbe_rdgen_test_invalid_threads()
 ******************************************/
/*!**************************************************************
 * fbe_rdgen_test_zero_threads()
 ****************************************************************
 * @brief
 *  This tests cases where we send invalid rderr commands,
 *  where either the lbas or block counts are incorrect.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  9/2/2009 - Created. Rob Foley
 *
 ****************************************************************/

void fbe_rdgen_test_zero_threads(void)
{
    fbe_status_t status;
    fbe_rdgen_unit_test_case_t *test_case_p = &fbe_rdgen_test_normal_cases[0];
    fbe_api_rdgen_context_t *context_p = &fbe_rdgen_test_contexts[0];

    /* Issue a single request with an invalid operation
     */
    status = fbe_api_rdgen_test_context_init(context_p,
                                         fbe_rdgen_unit_test_object_id, 
                                         fbe_rdgen_unit_test_class_id, 
                                         fbe_rdgen_unit_test_package_id,
                                         FBE_RDGEN_OPERATION_READ_ONLY,
                                         FBE_RDGEN_PATTERN_LBA_PASS,
                                         0, /* passes not used */
                                         100,    /* num ios */
                                         0,    /* time not used */
                                         0,  /* threads set to zero to cause error. */
                                         FBE_RDGEN_LBA_SPEC_RANDOM,
                                         test_case_p->start_lba,
                                         test_case_p->min_lba,
                                         test_case_p->max_lba,
                                         FBE_RDGEN_BLOCK_SPEC_RANDOM,
                                         test_case_p->min_blocks,
                                         test_case_p->max_blocks);
    MUT_ASSERT_INT_NOT_EQUAL(status, FBE_STATUS_OK);

    /* Execute the actual test.
     * We assert that the test was started OK. 
     */
    status = fbe_api_rdgen_run_tests(&fbe_rdgen_test_contexts[0], 
                                      fbe_rdgen_unit_test_service_package_id, 1);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Get the status from the context structure.
     * Make sure errors were encountered. 
     */
    status = fbe_api_rdgen_get_status(&fbe_rdgen_test_contexts[0], 1);
    MUT_ASSERT_INT_NOT_EQUAL(status, FBE_STATUS_OK);

    /* Cleanup the context.
     */
    status = fbe_api_rdgen_test_context_destroy(context_p);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    return;
}
/******************************************
 * end fbe_rdgen_test_zero_threads()
 ******************************************/

/*!**************************************************************
 * fbe_rdgen_test_invalid_lba_spec()
 ****************************************************************
 * @brief
 *  This tests a case where we send an invalid lba spec.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  9/2/2009 - Created. Rob Foley
 *
 ****************************************************************/

void fbe_rdgen_test_invalid_lba_spec(void)
{
    fbe_status_t status;
    fbe_rdgen_unit_test_case_t *test_case_p = &fbe_rdgen_test_normal_cases[0];
    fbe_api_rdgen_context_t *context_p = &fbe_rdgen_test_contexts[0];

    /* Issue a single request with an invalid operation.
     * lba spec is set to an invalid value.
     */
    status = fbe_api_rdgen_test_context_init(context_p,
                                         fbe_rdgen_unit_test_object_id, 
                                         fbe_rdgen_unit_test_class_id, 
                                         fbe_rdgen_unit_test_package_id,
                                         FBE_RDGEN_OPERATION_READ_ONLY,
                                         FBE_RDGEN_PATTERN_LBA_PASS,
                                         0, /* passes not used */
                                         100,    /* num ios */
                                         0,    /* time not used */
                                         1,  /* threads. */
                                         FBE_RDGEN_LBA_SPEC_INVALID,
                                         test_case_p->start_lba,
                                         test_case_p->min_lba,
                                         test_case_p->max_lba,
                                         FBE_RDGEN_BLOCK_SPEC_RANDOM,
                                         test_case_p->min_blocks,
                                         test_case_p->max_blocks);
    MUT_ASSERT_INT_NOT_EQUAL(status, FBE_STATUS_OK);

    /* Execute the actual test.
     * We assert that the test was started OK. 
     */
    status = fbe_api_rdgen_run_tests(&fbe_rdgen_test_contexts[0], 
                                      fbe_rdgen_unit_test_service_package_id, 1);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Get the status from the context structure.
     * Make sure errors were encountered. 
     */
    status = fbe_api_rdgen_get_status(&fbe_rdgen_test_contexts[0], 1);
    MUT_ASSERT_INT_NOT_EQUAL(status, FBE_STATUS_OK);

    /* Cleanup the context.
     */
    status = fbe_api_rdgen_test_context_destroy(context_p);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    return;
}
/******************************************
 * end fbe_rdgen_test_invalid_lba_spec()
 ******************************************/

/*!**************************************************************
 * fbe_rdgen_test_invalid_block_spec()
 ****************************************************************
 * @brief
 *  This tests a case where we send an invalid block spec.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  9/2/2009 - Created. Rob Foley
 *
 ****************************************************************/

void fbe_rdgen_test_invalid_block_spec(void)
{
    fbe_status_t status;
    fbe_rdgen_unit_test_case_t *test_case_p = &fbe_rdgen_test_normal_cases[0];
    fbe_api_rdgen_context_t *context_p = &fbe_rdgen_test_contexts[0];

    /* Issue a single request with an invalid operation.
     * Block spec is set to an invalid value.
     */
    status = fbe_api_rdgen_test_context_init(context_p,
                                         fbe_rdgen_unit_test_object_id, 
                                         fbe_rdgen_unit_test_class_id, 
                                         fbe_rdgen_unit_test_package_id,
                                         FBE_RDGEN_OPERATION_READ_ONLY,
                                         FBE_RDGEN_PATTERN_LBA_PASS,
                                         0, /* passes not used */
                                         100,    /* num ios */
                                         0,    /* time not used */
                                         1,  /* threads. */
                                         FBE_RDGEN_LBA_SPEC_RANDOM,
                                         test_case_p->start_lba,
                                         test_case_p->min_lba,
                                         test_case_p->max_lba,
                                         FBE_RDGEN_BLOCK_SPEC_INVALID,
                                         test_case_p->min_blocks,
                                         test_case_p->max_blocks);
    MUT_ASSERT_INT_NOT_EQUAL(status, FBE_STATUS_OK);

    /* Execute the actual test.
     * We assert that the test was started OK. 
     */
    status = fbe_api_rdgen_run_tests(&fbe_rdgen_test_contexts[0], 
                                      fbe_rdgen_unit_test_service_package_id, 1);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Get the status from the context structure.
     * Make sure errors were encountered. 
     */
    status = fbe_api_rdgen_get_status(&fbe_rdgen_test_contexts[0], 1);
    MUT_ASSERT_INT_NOT_EQUAL(status, FBE_STATUS_OK);

    /* Cleanup the context.
     */
    status = fbe_api_rdgen_test_context_destroy(context_p);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    return;
}
/******************************************
 * end fbe_rdgen_test_invalid_block_spec()
 ******************************************/

/*!**************************************************************
 * fbe_rdgen_test_invalid_pattern()
 ****************************************************************
 * @brief
 *  This tests a case where we send an invalid pattern.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  9/2/2009 - Created. Rob Foley
 *
 ****************************************************************/

void fbe_rdgen_test_invalid_pattern(void)
{
    fbe_status_t status;
    fbe_rdgen_unit_test_case_t *test_case_p = &fbe_rdgen_test_normal_cases[0];
    fbe_api_rdgen_context_t *context_p = &fbe_rdgen_test_contexts[0];

    /* Setup the request.
     */
    status = fbe_api_rdgen_test_context_init(context_p,
                                         fbe_rdgen_unit_test_object_id, 
                                         fbe_rdgen_unit_test_class_id, 
                                         fbe_rdgen_unit_test_package_id,
                                         FBE_RDGEN_OPERATION_READ_ONLY,
                                         FBE_RDGEN_PATTERN_LBA_PASS,
                                         0, /* passes not used */
                                         100,    /* num ios */
                                         0,    /* time not used */
                                         1,  /* threads. */
                                         FBE_RDGEN_LBA_SPEC_RANDOM,
                                         test_case_p->start_lba,
                                         test_case_p->min_lba,
                                         test_case_p->max_lba,
                                         FBE_RDGEN_BLOCK_SPEC_RANDOM,
                                         test_case_p->min_blocks,
                                         test_case_p->max_blocks);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Set the pattern to invalid.
     */
    context_p->start_io.specification.pattern = FBE_RDGEN_PATTERN_INVALID;

    /* Execute the actual test.
     * We assert that the test was started OK. 
     */
    status = fbe_api_rdgen_run_tests(&fbe_rdgen_test_contexts[0], 
                                      fbe_rdgen_unit_test_service_package_id, 1);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Get the status from the context structure.
     * Make sure errors were encountered. 
     */
    status = fbe_api_rdgen_get_status(&fbe_rdgen_test_contexts[0], 1);
    MUT_ASSERT_INT_NOT_EQUAL(status, FBE_STATUS_OK);

    /* Cleanup the context.
     */
    status = fbe_api_rdgen_test_context_destroy(context_p);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    return;
}
/******************************************
 * end fbe_rdgen_test_invalid_pattern()
 ******************************************/

/*!**************************************************************
 * fbe_rdgen_test_add_unit_tests()
 ****************************************************************
 * @brief
 *  Add the rdgen unit tests to the unit test suite.
 *
 * @param  suite_p - the suite to populate with tests.               
 *
 * @return None
 *
 * @author
 *  9/2/2009 - Created. Rob Foley
 *
 ****************************************************************/

void fbe_rdgen_test_add_unit_tests(mut_testsuite_t *suite_p)
{
    MUT_ADD_TEST(suite_p, fbe_rdgen_test_verify, fbe_rdgen_test_setup, fbe_rdgen_test_teardown);

    MUT_ADD_TEST(suite_p, fbe_rdgen_test_read_only, fbe_rdgen_test_setup, fbe_rdgen_test_teardown);
    MUT_ADD_TEST(suite_p, fbe_rdgen_test_write_only, fbe_rdgen_test_setup, fbe_rdgen_test_teardown);
    MUT_ADD_TEST(suite_p, fbe_rdgen_test_write_read_compare, fbe_rdgen_test_setup, fbe_rdgen_test_teardown);
    MUT_ADD_TEST(suite_p, fbe_rdgen_test_read_compare, fbe_rdgen_test_setup, fbe_rdgen_test_teardown);
    MUT_ADD_TEST(suite_p, fbe_rdgen_test_thread_counts, fbe_rdgen_test_setup, fbe_rdgen_test_teardown);
    
    MUT_ADD_TEST(suite_p, fbe_rdgen_test_lba_block_errors, fbe_rdgen_test_setup, fbe_rdgen_test_teardown);
    MUT_ADD_TEST(suite_p, fbe_rdgen_test_thread_count_errors, fbe_rdgen_test_setup, fbe_rdgen_test_teardown);
    MUT_ADD_TEST(suite_p, fbe_rdgen_test_invalid_operation, fbe_rdgen_test_setup, fbe_rdgen_test_teardown);
    MUT_ADD_TEST(suite_p, fbe_rdgen_test_invalid_threads, fbe_rdgen_test_setup, fbe_rdgen_test_teardown);
    MUT_ADD_TEST(suite_p, fbe_rdgen_test_zero_threads, fbe_rdgen_test_setup, fbe_rdgen_test_teardown);
    MUT_ADD_TEST(suite_p, fbe_rdgen_test_invalid_lba_spec, fbe_rdgen_test_setup, fbe_rdgen_test_teardown);
    MUT_ADD_TEST(suite_p, fbe_rdgen_test_invalid_block_spec, fbe_rdgen_test_setup, fbe_rdgen_test_teardown);
    MUT_ADD_TEST(suite_p, fbe_rdgen_test_invalid_pattern, fbe_rdgen_test_setup, fbe_rdgen_test_teardown);

    return;
}
/******************************************
 * end fbe_rdgen_test_add_unit_tests()
 ******************************************/

/*************************
 * end file fbe_rdgen_unit_test.c
 *************************/
