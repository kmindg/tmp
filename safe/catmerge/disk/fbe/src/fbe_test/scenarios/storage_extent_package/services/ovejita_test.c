/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file ovejita_test.c
 ***************************************************************************
 *
 * @brief
 *  This file contains an rdgen service test.
 *
 * @version
 *  6/16/2010 - Created. Rob Foley
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "mut.h"   
#include "fbe/fbe_api_rdgen_interface.h"
#include "sep_tests.h"
#include "sep_test_io.h"
#include "fbe_test_common_utils.h"
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_service.h"
#include "fbe/fbe_transport.h"
#include "fbe/fbe_api_common.h"
#include "fbe_test_package_config.h"
#include "fbe_test_configurations.h"
#include "fbe/fbe_api_provision_drive_interface.h"
#include "fbe/fbe_api_sim_server.h"
#include "pp_utils.h"
#include "fbe/fbe_random.h"
#include "sep_tests.h"

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

char *ovejita_short_desc = "RDGEN service test";
char *ovejita_long_desc =
    "\n"
    "\n"
    "The Ovejita Test exercises the RDGEN service by testing normal and error paths.\n"
    "\n"
    "\n"
    "Starting Config:\n"
    "    [PP] 1 - armada board\n"
    "    [PP] 1 - SAS PMC port\n"
    "    [PP] 3 - viper enclosures\n"
    "    [PP] 12 - SAS drives (PDOs)\n"
    "    [PP] 12 - Logical drives (LDs)\n"
    "\n"
    "STEP 1: Bring up the initial topology\n"
    "    - Create and verify the initial physical package config\n"
    "    - For each of the drives:(PDO1-PDO2-PDO3)\n"
    "        - Verify that LDO and PVD are both in the READY state\n"
    "\n" 
    "Iterate over the ranges of opcodes, threads, block counts that RDGEN supports\n"
    "\n" 
    "STEP 2: Execute Single SP test cases.\n"
    "    - Exercise Invalid class-id. (Single SP only)\n"
    "       - Pass in a invalid class-id for which we have no objects.\n"
    "       - Verify it returns an error.\n"
    "    - Exercise Invalid object-id. (Single SP only)\n"
    "       - Pass in a invalid object-id and verify it returns with an error.\n"
    "\n"
    "STEP 3: Execute Dual SP test cases.\n"
    "    - This covers the operations being executed on Local SP ONLY, Peer SP Only and a combination of both.\n"
    "       - Exercise Verify.\n"
    "       - Exercise READ-ONLY.\n"
    "       - Exercise WRITE-ONLY.\n"
    "       - Exercise WRITE-READ-COMPARE.\n"
    "       - Exercise operations over the range of allowed thread counts.\n"
    "           - Write some data.\n"
    "           - Read the data back and compare if it is the same we wrote.\n"
    "       - Exercise DATA MISCOMPARE.\n"
    "\n"
    "\n"
    "Execute Error test cases\n"
    "\n"
    "STEP 1: Execute LBA block error test cases\n"
    "    - Send invalid RDERR commands where either the lba's or block counts are incorrect.\n"
    "\n"
    "STEP 2: Execute thread count error test cases\n"
    "    - Verifies where the thread count is invalid for the given lba range and min block count.\n"
    "\n"
    "STEP 3: Execute invalid operation test cases\n"
    "    - Send invalid RDERR operations where either the lba's or block counts are incorrect.\n"
    "\n"
    "STEP 4: Execute invalid thread error test cases\n"
    "    - Issue a single request with an invalid operation.\n"
    "    - Set the thread to a high so that it causes an error.\n"
    "    - Verify the thread causes an error.\n"
    "\n"
    "STEP 5: Execute zero thread error test cases\n"
    "    - Issue a single request with an invalid operation with no threads (0).\n"
    "    - Verify the zero thread causes an error.\n"
    "\n"
    "STEP 6: Execute invalid LBA spec error test cases\n"
    "    - Issue a READ-only request with an invalid LBA spec.\n"
    "    - Verify the invalid LBA spec causes an error.\n"
    "\n"
    "STEP 7: Execute invalid block spec error test cases\n"
    "    - Issue a READ-only request with an invalid LBA spec.\n"
    "    - Verify the invalid LBA spec causes an error.\n"
    "\n"
    "STEP 8: Execute invalid pattern error test cases\n"
    "    - Issue a WRITE-only request with an invalid pattern.\n"
    "    - Verify that the invalid pattern causes an error.\n"
    "\n"
    "Description last updated: 10/04/2011.\n"
    "\n"
    "\n";

/*!*******************************************************************
 * @def OVEJITA_MAX_PEER_OPTIONS
 *********************************************************************
 * @brief Max number of peer option cases we will test in qual.
 *
 *********************************************************************/
#define OVEJITA_MAX_PEER_OPTIONS 1

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
 * @var ovejita_contexts
 *********************************************************************
 * @brief This has all the context objects with packet and
 *        test parameters.
 *
 *********************************************************************/
fbe_api_rdgen_context_t ovejita_contexts[FBE_RDGEN_MAX_TEST_CONTEXTS];

/*!*******************************************************************
 * @var fbe_rdgen_unit_test_object_id
 *********************************************************************
 * @brief This is the global to keep track of our object under test.
 *
 *********************************************************************/
static fbe_object_id_t fbe_rdgen_unit_test_object_id = FBE_OBJECT_ID_INVALID;

/*!*******************************************************************
 * @var fbe_rdgen_unit_test_secondary_object_id
 *********************************************************************
 * @brief This is the global to keep track of another
 *        object under test.
 *
 *********************************************************************/
static fbe_object_id_t fbe_rdgen_unit_test_secondary_object_id = FBE_OBJECT_ID_INVALID;

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
static fbe_package_id_t fbe_rdgen_unit_test_package_id = FBE_PACKAGE_ID_SEP_0;

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
void ovejita_setup(void);
void ovejita_teardown(void);
void ovejita_load_config(void);
void ovejita_unload_neit(void);
void ovejita_load_neit(void);

/*!*******************************************************************
 * @var ovejita_normal_cases
 *********************************************************************
 * @brief This is the set of normal non-error cases.
 *
 *********************************************************************/
fbe_rdgen_unit_test_case_t ovejita_normal_cases[] =
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
 * end ovejita_normal_cases
 **************************************/

/*!*******************************************************************
 * @var ovejita_multi_thread_normal_cases
 *********************************************************************
 * @brief This is the set of normal non-error cases.
 *
 *********************************************************************/
fbe_rdgen_unit_test_case_t ovejita_multi_thread_normal_cases[] =
{
    {   0,     0,  0x5000,     0x1,     0x10 },
    {   0,     0,  0x5000,     0x1,     0x100 },
    {   0,     0,  0x5000,     0x1,     0x100 },
    {   FBE_RDGEN_TEST_INVALID_FIELD },
};
/**************************************
 * end ovejita_multi_thread_normal_cases
 **************************************/
/*!*******************************************************************
 * @var ovejita_error_cases
 *********************************************************************
 * @brief These are cases for which we expect to get back errors.
 *
 *********************************************************************/
fbe_rdgen_unit_test_case_t ovejita_error_cases[] =
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
 * end ovejita_error_cases
 **************************************/

/*!**************************************************************
 * ovejita_execute_cases()
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
 * @param peer_options - options for sending I/O through peer.
 *
 * @return None.
 *
 * @author
 *  9/2/2009 - Created. Rob Foley
 *
 ****************************************************************/

void ovejita_execute_cases(fbe_rdgen_operation_t operation,
                           fbe_object_id_t object_id,
                           fbe_class_id_t class_id,
                           fbe_u32_t num_ios,
                           fbe_time_t time,
                           fbe_u32_t threads,
                           fbe_rdgen_unit_test_case_t *test_case_p,
                           fbe_bool_t b_error_expected,
                           fbe_api_rdgen_peer_options_t peer_options)
{
    fbe_status_t status;
    fbe_rdgen_lba_specification_t lba_spec;
    fbe_rdgen_block_specification_t block_spec;
    fbe_api_rdgen_context_t *context_p = &ovejita_contexts[0];

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

            /* Setup the options for sending I/O to the peer.
             */
            status = fbe_api_rdgen_io_specification_set_peer_options(&context_p->start_io.specification,
                                                                     peer_options);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

            /* Execute the actual test.
             * Assert that the test was started OK. 
             */
            status = fbe_api_rdgen_run_tests(&ovejita_contexts[0], 
                                              fbe_rdgen_unit_test_service_package_id, 1);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

            status = fbe_api_rdgen_get_status(&ovejita_contexts[0], 1);

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
 * end ovejita_execute_cases()
 ******************************************/
/*!**************************************************************
 * ovejita_standard_cases()
 ****************************************************************
 * @brief
 *  This tests verify test cases.
 *
 * @param operation - op to specify in the test.
 * @param peer_options - options for sending I/O through peer.                 
 *
 * @return None.
 *
 * @author
 *  9/2/2009 - Created. Rob Foley
 *
 ****************************************************************/

void ovejita_standard_cases(fbe_rdgen_operation_t op,
                            fbe_api_rdgen_peer_options_t peer_options)
{
    /* We know the object ids under test are 0.
     */
    fbe_u32_t threads;
    fbe_rdgen_unit_test_case_t *test_case_p = NULL;

    /* loop over all the test cases.
     */
    for (test_case_p = &ovejita_normal_cases[0]; 
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
                ovejita_execute_cases(op,
                                      fbe_rdgen_unit_test_object_id,
                                      fbe_rdgen_unit_test_class_id,
                                      num_ios,    /* num_ios */
                                      0,    /* time not used */
                                      threads,
                                      test_case_p,
                                      FALSE,    /* no errors expected */
                                      peer_options);
            }
        }
    }    /* end for all test cases*/
    return;
}
/******************************************
 * end ovejita_standard_cases()
 ******************************************/
/*!**************************************************************
 * ovejita_verify()
 ****************************************************************
 * @brief
 *  This tests verify test cases.
 *
 * @param peer_options - options for sending I/O through peer.                   
 *
 * @return None.
 *
 * @author
 *  9/2/2009 - Created. Rob Foley
 *
 ****************************************************************/

void ovejita_verify(fbe_api_rdgen_peer_options_t peer_options)
{
    mut_printf(MUT_LOG_TEST_STATUS, "== %s starting ==", __FUNCTION__);
    ovejita_standard_cases(FBE_RDGEN_OPERATION_VERIFY, peer_options);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s complete ==", __FUNCTION__);
    return;
}
/******************************************
 * end ovejita_verify()
 ******************************************/
/*!**************************************************************
 * ovejita_write_read_compare()
 ****************************************************************
 * @brief
 *  This tests all write/read/compare cases.
 *
 * @param peer_options - options for sending I/O through peer.               
 *
 * @return None.
 *
 * @author
 *  9/2/2009 - Created. Rob Foley
 *
 ****************************************************************/

void ovejita_write_read_compare(fbe_api_rdgen_peer_options_t peer_options)
{
    mut_printf(MUT_LOG_TEST_STATUS, "== %s starting ==", __FUNCTION__);
    ovejita_standard_cases(FBE_RDGEN_OPERATION_WRITE_READ_CHECK, peer_options);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s complete ==", __FUNCTION__);
    return;
}
/******************************************
 * end ovejita_write_read_compare()
 ******************************************/

/*!**************************************************************
 * ovejita_remove_passive_SP()
 ****************************************************************
 * @brief
 *  This tests removing the passive SP, the SP that is servicing
 *  I/Os for the peer.
 *
 * @param peer_options - options for sending I/O through peer.               
 *
 * @return None.
 *
 * @author
 *  10/8/2010 - Created. Rob Foley
 *
 ****************************************************************/

void ovejita_remove_passive_SP(fbe_api_rdgen_peer_options_t peer_options)
{
    fbe_api_rdgen_context_t *context_p = &ovejita_contexts[0];
    fbe_status_t status;
    #define OVEJITA_REMOVE_PASSIVE_SP_DELAY_MAX_MSECS 5000
    #define OVEJITA_REMOVE_PASSIVE_SP_THREADS 20
    #define OVEJITA_REMOVE_PASSIVE_SP_BLOCKS 1024

    mut_printf(MUT_LOG_TEST_STATUS, "== %s starting ==", __FUNCTION__);

    status = fbe_api_rdgen_test_context_init(context_p,
                                             fbe_rdgen_unit_test_object_id, 
                                             FBE_CLASS_ID_INVALID, 
                                             fbe_rdgen_unit_test_package_id,
                                             FBE_RDGEN_OPERATION_WRITE_READ_CHECK,
                                             FBE_RDGEN_PATTERN_LBA_PASS,
                                             0,    /* passes not used */
                                             0,    /* num ios run forever */
                                             0,    /* time not used */
                                             fbe_test_sep_io_get_threads(OVEJITA_REMOVE_PASSIVE_SP_THREADS),
                                             FBE_RDGEN_LBA_SPEC_RANDOM,
                                             0, /* start lba */
                                             0, /* min lba*/
                                             /* Use less than the full capacity
                                              * to prevent the memory drive from allocating too much.*  
                                              */
                                             0x2000, 
                                             FBE_RDGEN_BLOCK_SPEC_RANDOM,
                                             1,
                                             OVEJITA_REMOVE_PASSIVE_SP_BLOCKS);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Setup the options for sending I/O to the peer.
     */
    status = fbe_api_rdgen_io_specification_set_peer_options(&context_p->start_io.specification,
                                                             peer_options);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Run our I/O test.
     */
    status = fbe_api_rdgen_start_tests(context_p, FBE_PACKAGE_ID_NEIT, 1);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Wait briefly before bringing the peer down.
     */
    fbe_api_sleep(OVEJITA_REMOVE_PASSIVE_SP_DELAY_MAX_MSECS);

    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s remove SP B ==", __FUNCTION__);
    ovejita_unload_neit();

    fbe_api_sleep(OVEJITA_REMOVE_PASSIVE_SP_DELAY_MAX_MSECS);

    /* Stop I/O since the peer is down.
     */
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s Stopping I/O ==", __FUNCTION__);
    status = fbe_api_rdgen_stop_tests(context_p, 1);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* There will likely be I/O errors, since we unloaded sep while I/O was still running.
     * Just make sure we have a status.
     */
    status = fbe_api_rdgen_get_status(&ovejita_contexts[0], 1);
    MUT_ASSERT_INT_NOT_EQUAL(status, FBE_STATUS_INVALID);

    MUT_ASSERT_INT_NOT_EQUAL(context_p->start_io.status.rdgen_status, FBE_RDGEN_OPERATION_STATUS_INVALID);
    if (context_p->start_io.status.status == FBE_STATUS_OK)
    {
        MUT_ASSERT_INT_NOT_EQUAL(context_p->start_io.status.block_status, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID);
    }
    MUT_ASSERT_INT_NOT_EQUAL(context_p->start_io.status.status, FBE_STATUS_INVALID);

    /* Restar the peer.
     */
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s re-load SP B ==", __FUNCTION__);
    ovejita_load_neit();
    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A); 

    mut_printf(MUT_LOG_TEST_STATUS, "== %s complete ==", __FUNCTION__);
    return;
}
/******************************************
 * end ovejita_remove_passive_SP()
 ******************************************/
/*!**************************************************************
 * ovejita_write_only()
 ****************************************************************
 * @brief
 *  This tests all write only cases.
 *
 * @param peer_options - options for sending I/O through peer.               
 *
 * @return None.
 *
 * @author
 *  9/2/2009 - Created. Rob Foley
 *
 ****************************************************************/

void ovejita_write_only(fbe_api_rdgen_peer_options_t peer_options)
{
    mut_printf(MUT_LOG_TEST_STATUS, "== %s starting ==", __FUNCTION__);
    ovejita_standard_cases(FBE_RDGEN_OPERATION_WRITE_ONLY, peer_options);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s complete ==", __FUNCTION__);
    return;
}
/******************************************
 * end ovejita_write_only()
 ******************************************/
/*!**************************************************************
 * ovejita_read_only()
 ****************************************************************
 * @brief
 *  This tests all read only cases.
 *
 * @param peer_options - options for sending I/O through peer.               
 *
 * @return None.
 *
 * @author
 *  9/2/2009 - Created. Rob Foley
 *
 ****************************************************************/

void ovejita_read_only(fbe_api_rdgen_peer_options_t peer_options)
{
    mut_printf(MUT_LOG_TEST_STATUS, "== %s starting ==", __FUNCTION__);
    ovejita_standard_cases(FBE_RDGEN_OPERATION_READ_ONLY, peer_options);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s complete ==", __FUNCTION__);
    return;
}
/******************************************
 * end ovejita_read_only()
 ******************************************/
/*!**************************************************************
 * ovejita_fixed_pattern()
 ****************************************************************
 * @brief
 *  This tests a case where we send an invalid block spec.
 *
 * @param peer_options - options for sending I/O through peer.
 *
 * @return None.
 *
 * @author
 *  9/15/2009 - Created. Rob Foley
 *
 ****************************************************************/

void ovejita_fixed_pattern(fbe_rdgen_lba_specification_t lba_spec,
                           fbe_rdgen_unit_test_case_t *test_case_p,
                           fbe_rdgen_operation_t operation,
                           fbe_u32_t num_ios,
                           fbe_api_rdgen_peer_options_t peer_options)
{
    fbe_status_t status;
    fbe_api_rdgen_context_t *context_p = &ovejita_contexts[0];

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

    /* Setup the options for sending I/O to the peer.
     */
    status = fbe_api_rdgen_io_specification_set_peer_options(&context_p->start_io.specification,
                                                             peer_options);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Execute the actual test.
     * We assert that the test was started OK. 
     */
    status = fbe_api_rdgen_run_tests(&ovejita_contexts[0], 
                                      fbe_rdgen_unit_test_service_package_id, 1);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Get the status from the context structure.
     * Make sure errors were encountered. 
     */
    status = fbe_api_rdgen_get_status(&ovejita_contexts[0], 1);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Cleanup the context.
     */
    status = fbe_api_rdgen_test_context_destroy(context_p);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    return;
}
/******************************************
 * end ovejita_fixed_pattern()
 ******************************************/
/*!**************************************************************
 * ovejita_read_compare()
 ****************************************************************
 * @brief
 *  This tests all read/compare cases.
 *
 * @param peer_options - options for sending I/O through peer.               
 *
 * @return None.
 *
 * @author
 *  9/2/2009 - Created. Rob Foley
 *
 ****************************************************************/

void ovejita_read_compare(fbe_api_rdgen_peer_options_t peer_options)
{
    fbe_rdgen_unit_test_case_t *test_case_p = &ovejita_normal_cases[0];
    
    mut_printf(MUT_LOG_TEST_STATUS, "== %s starting ==", __FUNCTION__);

    /* Write out a given pattern.
     */
    ovejita_fixed_pattern(FBE_RDGEN_LBA_SPEC_SEQUENTIAL_INCREASING,
                          test_case_p,
                          FBE_RDGEN_OPERATION_WRITE_ONLY,
                          100, 
                          peer_options);

    /* Read back the pattern and make sure it matched.
     */
    ovejita_fixed_pattern(FBE_RDGEN_LBA_SPEC_SEQUENTIAL_INCREASING,
                          test_case_p,
                          FBE_RDGEN_OPERATION_READ_CHECK,
                          100, 
                          peer_options);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s complete ==", __FUNCTION__);
    return;
}
/******************************************
 * end ovejita_read_compare()
 ******************************************/

/*!**************************************************************
 * ovejita_lba_block_errors()
 ****************************************************************
 * @brief
 *  This tests cases where we send invalid rderr commands,
 *  where either the lbas or block counts are incorrect.
 *
 * @param peer_options - options for sending I/O through peer.               
 *
 * @return None.
 *
 * @author
 *  9/2/2009 - Created. Rob Foley
 *
 ****************************************************************/

void ovejita_lba_block_errors(fbe_api_rdgen_peer_options_t peer_options)
{
    /* We know the object ids under test are 0.
     */
    fbe_rdgen_unit_test_case_t *test_case_p = NULL;
    mut_printf(MUT_LOG_TEST_STATUS, "== %s starting ==", __FUNCTION__);

    /* loop over all the test cases.
     */
    for (test_case_p = &ovejita_error_cases[0]; 
        test_case_p->start_lba != FBE_RDGEN_TEST_INVALID_FIELD;
        test_case_p++)
    {
        /* Loop over a predetermined number of  io counts.
         */
        ovejita_execute_cases(FBE_RDGEN_OPERATION_READ_ONLY,
                              fbe_rdgen_unit_test_object_id,
                              fbe_rdgen_unit_test_class_id,
                              100,    /* num_ios is arbitrary */
                              0,    /* time not used */
                              1,    /* threads */
                              test_case_p,
                              TRUE,    /* errors expected */
                              peer_options );
    }    /* end for all test cases*/
    mut_printf(MUT_LOG_TEST_STATUS, "== %s complete ==", __FUNCTION__);
    return;
}
/******************************************
 * end ovejita_lba_block_errors()
 ******************************************/

/*!**************************************************************
 * ovejita_thread_count_errors()
 ****************************************************************
 * @brief
 *  This tests cases where the thread count is invalid for
 *  the given lba range and min block count.
 *
 * @param peer_options - options for sending I/O through peer.        
 *
 * @return None.
 *
 * @author
 *  9/2/2009 - Created. Rob Foley
 *
 ****************************************************************/

void ovejita_thread_count_errors(fbe_api_rdgen_peer_options_t peer_options)
{
    /* We know the object ids under test are 0.
     */
    fbe_rdgen_unit_test_case_t *test_case_p = NULL;
    mut_printf(MUT_LOG_TEST_STATUS, "== %s starting ==", __FUNCTION__);

    /* loop over all the test cases.
     */
    for (test_case_p = &ovejita_normal_cases[0]; 
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
        ovejita_execute_cases(FBE_RDGEN_OPERATION_READ_ONLY,
                              fbe_rdgen_unit_test_object_id,
                              fbe_rdgen_unit_test_class_id,
                              100,    /* num_ios is arbitrary */
                              0,    /* time not used */
                              max_threads + 1,    /* threads */
                              test_case_p,
                              TRUE,    /* errors expected */ 
                              peer_options);
    }    /* end for all test cases*/
    mut_printf(MUT_LOG_TEST_STATUS, "== %s complete ==", __FUNCTION__);
    return;
}
/******************************************
 * end ovejita_thread_count_errors()
 ******************************************/

/*!**************************************************************
 * ovejita_invalid_operation()
 ****************************************************************
 * @brief
 *  This tests cases where we send invalid operation to cause 
 *  an error.
 *
 * @param peer_options - options for sending I/O through peer.               
 *
 * @return None.
 *
 * @author
 *  9/2/2009 - Created. Rob Foley
 *
 ****************************************************************/

void ovejita_invalid_operation(fbe_api_rdgen_peer_options_t peer_options)
{
    fbe_status_t status;
    fbe_rdgen_unit_test_case_t *test_case_p = &ovejita_normal_cases[0];
    fbe_api_rdgen_context_t *context_p = &ovejita_contexts[0];
    mut_printf(MUT_LOG_TEST_STATUS, "== %s starting ==", __FUNCTION__);

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

    /* Setup the options for sending I/O to the peer.
     */
    status = fbe_api_rdgen_io_specification_set_peer_options(&context_p->start_io.specification,
                                                             peer_options);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Execute the actual test.
     * We assert that the test was started OK. 
     */
    status = fbe_api_rdgen_run_tests(&ovejita_contexts[0], 
                                      fbe_rdgen_unit_test_service_package_id, 1);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Get the status from the context structure.
     * Make sure errors were encountered. 
     */
    status = fbe_api_rdgen_get_status(&ovejita_contexts[0], 1);
    MUT_ASSERT_INT_NOT_EQUAL(status, FBE_STATUS_OK);

    /* Cleanup the context.
     */
    status = fbe_api_rdgen_test_context_destroy(context_p);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s complete ==", __FUNCTION__);
    return;
}
/******************************************
 * end ovejita_invalid_operation()
 ******************************************/

/*!**************************************************************
 * ovejita_disk_ops_test()
 ****************************************************************
 * @brief
 *  Test sending disk op related commands.
 *
 * @param peer_options - options for sending I/O through peer.               
 *
 * @return None.
 *
 * @author
 *  2/27/2013 - Created. Rob Foley
 *
 ****************************************************************/

void ovejita_disk_ops_test(fbe_api_rdgen_peer_options_t peer_options)
{
    fbe_status_t status;
    fbe_rdgen_unit_test_case_t *test_case_p = &ovejita_normal_cases[0];
    fbe_api_rdgen_context_t *context_p = &ovejita_contexts[0];
    mut_printf(MUT_LOG_TEST_STATUS, "== %s starting ==", __FUNCTION__);

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
                                             1,  /* threads */
                                             FBE_RDGEN_LBA_SPEC_RANDOM,
                                             test_case_p->start_lba,
                                             test_case_p->min_lba,
                                             test_case_p->max_lba,
                                             FBE_RDGEN_BLOCK_SPEC_RANDOM,
                                             test_case_p->min_blocks,
                                             test_case_p->max_blocks);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    status = fbe_api_rdgen_filter_init_for_driver(&context_p->start_io.filter, FBE_RDGEN_FILTER_TYPE_DISK_INTERFACE,
                                                  "CLARiiONdisk0",  0x5);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    /* Setup the options for sending I/O to the peer.
     */
    status = fbe_api_rdgen_io_specification_set_peer_options(&context_p->start_io.specification,
                                                             peer_options);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Execute the actual test.
     * We assert that the test was started OK. 
     */
    status = fbe_api_rdgen_run_tests(&ovejita_contexts[0], 
                                      fbe_rdgen_unit_test_service_package_id, 1);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Get the status from the context structure.
     * Make sure errors were encountered. 
     */
    status = fbe_api_rdgen_get_status(&ovejita_contexts[0], 1);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Cleanup the context.
     */
    status = fbe_api_rdgen_test_context_destroy(context_p);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s complete ==", __FUNCTION__);
    return;
}
/******************************************
 * end ovejita_disk_ops_test()
 ******************************************/
/*!**************************************************************
 * ovejita_thread_count_cases()
 ****************************************************************
 * @brief
 *  This tests kicks off tests over the range of allowed
 *  thread counts for all operations.
 *
 * @param object_id - the object to test.
 * @param class_id - the class to test.     
 * @param package_id - the package to test.             
 * @param num_ios - number of I/Os to test.
 * @param peer_options - options for sending I/O through peer.  
 *
 * @return None.
 *
 * @author
 *  9/25/2009 - Created. Rob Foley
 *
 ****************************************************************/

void ovejita_thread_count_cases(fbe_object_id_t object_id,
                                fbe_class_id_t class_id,
                                fbe_package_id_t package_id,
                                fbe_u32_t num_ios,
                                fbe_api_rdgen_peer_options_t peer_options,
                                fbe_u32_t max_threads)
{
    fbe_status_t status;
    fbe_rdgen_unit_test_case_t *test_case_p = &ovejita_multi_thread_normal_cases[0];
    fbe_api_rdgen_context_t *context_p = &ovejita_contexts[0];
    fbe_rdgen_operation_t operation;
    fbe_u32_t threads;

    /* Iterate over the power of two thread counts.
     */
    for ( threads = 1; threads <= max_threads; threads *= 2)
    {
        /* Iterate over all operation types.
         */
        for ( operation = FBE_RDGEN_OPERATION_READ_ONLY; operation <= FBE_RDGEN_OPERATION_WRITE_READ_CHECK; operation++)
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

            /* Setup the options for sending I/O to the peer.
             */
            status = fbe_api_rdgen_io_specification_set_peer_options(&context_p->start_io.specification,
                                                                     peer_options);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

            /* Execute the actual test.
             * We assert that the test was started OK. 
             */
            status = fbe_api_rdgen_run_tests(&ovejita_contexts[0], 
                                              fbe_rdgen_unit_test_service_package_id, 1);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

            /* Get the status from the context structure.
             * Make sure errors were encountered. 
             */
            status = fbe_api_rdgen_get_status(&ovejita_contexts[0], 1);
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
 * end ovejita_thread_count_cases()
 ******************************************/
/*!**************************************************************
 * ovejita_thread_counts()
 ****************************************************************
 * @brief
 *  This tests kicks off tests over the range of allowed
 *  thread counts for all operations.
 *
 * @param peer_options - options for sending I/O through peer. 
 * @param max_threads - max thread count to use (power of 2).              
 *
 * @return None.
 *
 * @author
 *  9/2/2009 - Created. Rob Foley
 *
 ****************************************************************/

void ovejita_thread_counts(fbe_api_rdgen_peer_options_t peer_options, fbe_u32_t max_threads)
{
    MUT_ASSERT_CONDITION(max_threads, <=, FBE_RDGEN_MAX_THREADS_PER_IO_SPECIFICATION);

    mut_printf(MUT_LOG_TEST_STATUS, "== %s starting ==", __FUNCTION__);
    /* First run I/O to a given object.
     */
    ovejita_thread_count_cases(fbe_rdgen_unit_test_object_id, 
                               FBE_CLASS_ID_INVALID,
                               fbe_rdgen_unit_test_package_id,
                               1, 
                               peer_options,
                               max_threads);
    ovejita_thread_count_cases(fbe_rdgen_unit_test_object_id, 
                               FBE_CLASS_ID_INVALID,
                               fbe_rdgen_unit_test_package_id,
                               20, 
                               peer_options,
                               max_threads);

    if (fbe_rdgen_unit_test_secondary_object_id != FBE_OBJECT_ID_INVALID)
    {
        /* Test against another object.
         */
        ovejita_thread_count_cases(fbe_rdgen_unit_test_secondary_object_id, 
                                   FBE_CLASS_ID_INVALID,
                                   fbe_rdgen_unit_test_package_id,
                                   1, 
                                   peer_options,
                                   max_threads);
        ovejita_thread_count_cases(fbe_rdgen_unit_test_secondary_object_id, 
                                   FBE_CLASS_ID_INVALID,
                                   fbe_rdgen_unit_test_package_id,
                                   20, 
                                   peer_options,
                                   max_threads);
    }
    mut_printf(MUT_LOG_TEST_STATUS, "== %s complete ==", __FUNCTION__);
    return;
}
/******************************************
 * end ovejita_thread_counts()
 ******************************************/
/*!**************************************************************
 * ovejita_invalid_threads()
 ****************************************************************
 * @brief
 *  This tests cases where we issue a invalid operation and send 
 *  more than the max threads to cause an error.
 *
 * @param peer_options - options for sending I/O through peer.               
 *
 * @return None.
 *
 * @author
 *  9/2/2009 - Created. Rob Foley
 *
 ****************************************************************/

void ovejita_invalid_threads(fbe_api_rdgen_peer_options_t peer_options)
{
    fbe_status_t status;
    fbe_rdgen_unit_test_case_t *test_case_p = &ovejita_normal_cases[0];
    fbe_api_rdgen_context_t *context_p = &ovejita_contexts[0];
    mut_printf(MUT_LOG_TEST_STATUS, "== %s starting ==", __FUNCTION__);

    /* Issue a single request with an invalid operation.
     * Threads is set too high to cause an error. 
     */
    status = fbe_api_rdgen_test_context_init(context_p,
                                             fbe_rdgen_unit_test_object_id, 
                                             fbe_rdgen_unit_test_class_id, 
                                             fbe_rdgen_unit_test_package_id,
                                             FBE_RDGEN_OPERATION_READ_ONLY,
                                             FBE_RDGEN_PATTERN_LBA_PASS,
                                             0,    /* passes not used */
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

    /* Setup the options for sending I/O to the peer.
     */
    status = fbe_api_rdgen_io_specification_set_peer_options(&context_p->start_io.specification,
                                                             peer_options);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Execute the actual test.
     * We assert that the test was started OK. 
     */
    status = fbe_api_rdgen_run_tests(&ovejita_contexts[0], 
                                      fbe_rdgen_unit_test_service_package_id, 1);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Get the status from the context structure.
     * Make sure errors were encountered. 
     */
    status = fbe_api_rdgen_get_status(&ovejita_contexts[0], 1);
    MUT_ASSERT_INT_NOT_EQUAL(status, FBE_STATUS_OK);

    /* Cleanup the context.
     */
    status = fbe_api_rdgen_test_context_destroy(context_p);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s complete ==", __FUNCTION__);
    return;
}
/******************************************
 * end ovejita_invalid_threads()
 ******************************************/

/*!**************************************************************
 * ovejita_invalid_class()
 ****************************************************************
 * @brief
 *  This tests cases where we send a class id for which
 *  we have no objects.
 *
 * @param peer_options - options for sending I/O through peer.               
 *
 * @return None.
 *
 * @author
 *  10/5/2010 - Created. Rob Foley
 *
 ****************************************************************/

void ovejita_invalid_class(fbe_api_rdgen_peer_options_t peer_options)
{
    fbe_status_t status;
    fbe_rdgen_unit_test_case_t *test_case_p = &ovejita_normal_cases[0];
    fbe_api_rdgen_context_t *context_p = &ovejita_contexts[0];
    mut_printf(MUT_LOG_TEST_STATUS, "== %s starting ==", __FUNCTION__);

    /* Issue a single request with an invalid class id (a class id where there are no objects in sep).
     * This should cause an error. 
     */
    status = fbe_api_rdgen_test_context_init(context_p,
                                             FBE_OBJECT_ID_INVALID, 
                                             FBE_CLASS_ID_BASE_BOARD,  
                                             fbe_rdgen_unit_test_package_id,
                                             FBE_RDGEN_OPERATION_READ_ONLY,
                                             FBE_RDGEN_PATTERN_LBA_PASS,
                                             0,    /* passes not used */
                                             100,    /* num ios */
                                             0,    /* time not used */
                                             FBE_RDGEN_MAX_THREADS_PER_IO_SPECIFICATION,
                                             FBE_RDGEN_LBA_SPEC_RANDOM,
                                             test_case_p->start_lba,
                                             test_case_p->min_lba,
                                             test_case_p->max_lba,
                                             FBE_RDGEN_BLOCK_SPEC_RANDOM,
                                             test_case_p->min_blocks,
                                             test_case_p->max_blocks);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Setup the options for sending I/O to the peer.
     */
    status = fbe_api_rdgen_io_specification_set_peer_options(&context_p->start_io.specification,
                                                             peer_options);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Execute the actual test.
     * We assert that the test was started OK. 
     */
    status = fbe_api_rdgen_run_tests(&ovejita_contexts[0], 
                                     fbe_rdgen_unit_test_service_package_id, 1);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Get the status from the context structure.
     * Make sure errors were encountered. 
     */
    status = fbe_api_rdgen_get_status(&ovejita_contexts[0], 1);
    MUT_ASSERT_INT_NOT_EQUAL(status, FBE_STATUS_OK);

    /* Validate the status is as expected.
     */
    MUT_ASSERT_INT_EQUAL(context_p->start_io.status.rdgen_status, FBE_RDGEN_OPERATION_STATUS_NO_OBJECTS_OF_CLASS);

    /* Cleanup the context.
     */
    status = fbe_api_rdgen_test_context_destroy(context_p);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s complete ==", __FUNCTION__);
    return;
}
/******************************************
 * end ovejita_invalid_class()
 ******************************************/

/*!**************************************************************
 * ovejita_invalid_object()
 ****************************************************************
 * @brief
 *  This tests cases where we send a object id for which
 *  we have no objects.
 *
 * @param peer_options - options for sending I/O through peer.               
 *
 * @return None.
 *
 * @author
 *  10/12/2010 - Created. Rob Foley
 *
 ****************************************************************/

void ovejita_invalid_object(fbe_api_rdgen_peer_options_t peer_options)
{
    fbe_status_t status;
    fbe_rdgen_unit_test_case_t *test_case_p = &ovejita_normal_cases[0];
    fbe_api_rdgen_context_t *context_p = &ovejita_contexts[0];
    mut_printf(MUT_LOG_TEST_STATUS, "== %s starting ==", __FUNCTION__);

    /* Issue a single request with an invalid object id.
     * This should cause an error.
     */
    status = fbe_api_rdgen_test_context_init(context_p,
                                             /* Not FBE_OBJECT_ID_INVALID, 
                                              * but an object id that is invalid.
                                              */
                                             FBE_OBJECT_ID_INVALID - 1, 
                                             FBE_CLASS_ID_INVALID,  
                                             fbe_rdgen_unit_test_package_id,
                                             FBE_RDGEN_OPERATION_READ_ONLY,
                                             FBE_RDGEN_PATTERN_LBA_PASS,
                                             0,    /* passes not used */
                                             100,    /* num ios */
                                             0,    /* time not used */
                                             FBE_RDGEN_MAX_THREADS_PER_IO_SPECIFICATION,
                                             FBE_RDGEN_LBA_SPEC_RANDOM,
                                             test_case_p->start_lba,
                                             test_case_p->min_lba,
                                             test_case_p->max_lba,
                                             FBE_RDGEN_BLOCK_SPEC_RANDOM,
                                             test_case_p->min_blocks,
                                             test_case_p->max_blocks);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Setup the options for sending I/O to the peer.
     */
    status = fbe_api_rdgen_io_specification_set_peer_options(&context_p->start_io.specification,
                                                             peer_options);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Execute the actual test.
     * We assert that the test was started OK. 
     */
    status = fbe_api_rdgen_run_tests(&ovejita_contexts[0], 
                                     fbe_rdgen_unit_test_service_package_id, 1);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Get the status from the context structure.
     * Make sure errors were encountered. 
     */
    status = fbe_api_rdgen_get_status(&ovejita_contexts[0], 1);
    MUT_ASSERT_INT_NOT_EQUAL(status, FBE_STATUS_OK);

    /* Make sure we got the correct status.
     */
    MUT_ASSERT_INT_EQUAL(context_p->start_io.status.rdgen_status, FBE_RDGEN_OPERATION_STATUS_OBJECT_DOES_NOT_EXIST);

    /* Cleanup the context.
     */
    status = fbe_api_rdgen_test_context_destroy(context_p);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s complete ==", __FUNCTION__);
    return;
}
/******************************************
 * end ovejita_invalid_object()
 ******************************************/
/*!**************************************************************
 * ovejita_zero_threads()
 ****************************************************************
 * @brief
 *  This tests cases where we send an invalid operation with
 *  no threads to cause an error.
 *
 * @param peer_options - options for sending I/O through peer.               
 *
 * @return None.
 *
 * @author
 *  9/2/2009 - Created. Rob Foley
 *
 ****************************************************************/

void ovejita_zero_threads(fbe_api_rdgen_peer_options_t peer_options)
{
    fbe_status_t status;
    fbe_rdgen_unit_test_case_t *test_case_p = &ovejita_normal_cases[0];
    fbe_api_rdgen_context_t *context_p = &ovejita_contexts[0];
    mut_printf(MUT_LOG_TEST_STATUS, "== %s starting ==", __FUNCTION__);

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

    /* Setup the options for sending I/O to the peer.
     */
    status = fbe_api_rdgen_io_specification_set_peer_options(&context_p->start_io.specification,
                                                             peer_options);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Execute the actual test.
     * We assert that the test was started OK. 
     */
    status = fbe_api_rdgen_run_tests(&ovejita_contexts[0], 
                                      fbe_rdgen_unit_test_service_package_id, 1);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Get the status from the context structure.
     * Make sure errors were encountered. 
     */
    status = fbe_api_rdgen_get_status(&ovejita_contexts[0], 1);
    MUT_ASSERT_INT_NOT_EQUAL(status, FBE_STATUS_OK);

    /* Cleanup the context.
     */
    status = fbe_api_rdgen_test_context_destroy(context_p);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s complete ==", __FUNCTION__);
    return;
}
/******************************************
 * end ovejita_zero_threads()
 ******************************************/

/*!**************************************************************
 * ovejita_invalid_lba_spec()
 ****************************************************************
 * @brief
 *  This tests a case where we send an invalid lba spec.
 *
 * @param peer_options - options for sending I/O through peer.               
 *
 * @return None.
 *
 * @author
 *  9/2/2009 - Created. Rob Foley
 *
 ****************************************************************/

void ovejita_invalid_lba_spec(fbe_api_rdgen_peer_options_t peer_options)
{
    fbe_status_t status;
    fbe_rdgen_unit_test_case_t *test_case_p = &ovejita_normal_cases[0];
    fbe_api_rdgen_context_t *context_p = &ovejita_contexts[0];

    mut_printf(MUT_LOG_TEST_STATUS, "== %s starting ==", __FUNCTION__);
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

    /* Setup the options for sending I/O to the peer.
     */
    status = fbe_api_rdgen_io_specification_set_peer_options(&context_p->start_io.specification,
                                                             peer_options);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Execute the actual test.
     * We assert that the test was started OK. 
     */
    status = fbe_api_rdgen_run_tests(&ovejita_contexts[0], 
                                      fbe_rdgen_unit_test_service_package_id, 1);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Get the status from the context structure.
     * Make sure errors were encountered. 
     */
    status = fbe_api_rdgen_get_status(&ovejita_contexts[0], 1);
    MUT_ASSERT_INT_NOT_EQUAL(status, FBE_STATUS_OK);

    /* Cleanup the context.
     */
    status = fbe_api_rdgen_test_context_destroy(context_p);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s complete ==", __FUNCTION__);
    return;
}
/******************************************
 * end ovejita_invalid_lba_spec()
 ******************************************/

/*!**************************************************************
 * ovejita_invalid_block_spec()
 ****************************************************************
 * @brief
 *  This tests a case where we send an invalid block spec.
 *
 * @param peer_options - options for sending I/O through peer.               
 *
 * @return None.
 *
 * @author
 *  9/2/2009 - Created. Rob Foley
 *
 ****************************************************************/

void ovejita_invalid_block_spec(fbe_api_rdgen_peer_options_t peer_options)
{
    fbe_status_t status;
    fbe_rdgen_unit_test_case_t *test_case_p = &ovejita_normal_cases[0];
    fbe_api_rdgen_context_t *context_p = &ovejita_contexts[0];

    mut_printf(MUT_LOG_TEST_STATUS, "== %s starting ==", __FUNCTION__);
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

    /* Setup the options for sending I/O to the peer.
     */
    status = fbe_api_rdgen_io_specification_set_peer_options(&context_p->start_io.specification,
                                                             peer_options);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Execute the actual test.
     * We assert that the test was started OK. 
     */
    status = fbe_api_rdgen_run_tests(&ovejita_contexts[0], 
                                      fbe_rdgen_unit_test_service_package_id, 1);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Get the status from the context structure.
     * Make sure errors were encountered. 
     */
    status = fbe_api_rdgen_get_status(&ovejita_contexts[0], 1);
    MUT_ASSERT_INT_NOT_EQUAL(status, FBE_STATUS_OK);

    /* Cleanup the context.
     */
    status = fbe_api_rdgen_test_context_destroy(context_p);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s complete ==", __FUNCTION__);
    return;
}
/******************************************
 * end ovejita_invalid_block_spec()
 ******************************************/

/*!**************************************************************
 * ovejita_invalid_pattern()
 ****************************************************************
 * @brief
 *  This tests a case where we send an invalid pattern.
 *
 * @param peer_options - options for sending I/O through peer.               
 *
 * @return None.
 *
 * @author
 *  9/2/2009 - Created. Rob Foley
 *
 ****************************************************************/

void ovejita_invalid_pattern(fbe_api_rdgen_peer_options_t peer_options)
{
    fbe_status_t status;
    fbe_rdgen_unit_test_case_t *test_case_p = &ovejita_normal_cases[0];
    fbe_api_rdgen_context_t *context_p = &ovejita_contexts[0];

    mut_printf(MUT_LOG_TEST_STATUS, "== %s starting ==", __FUNCTION__);
    /* Setup the request with an invalid pattern.
     */
    status = fbe_api_rdgen_test_context_init(context_p,
                                             fbe_rdgen_unit_test_object_id, 
                                             fbe_rdgen_unit_test_class_id, 
                                             fbe_rdgen_unit_test_package_id,
                                             FBE_RDGEN_OPERATION_WRITE_ONLY,
                                             FBE_RDGEN_PATTERN_INVALID,
                                             0,    /* passes not used */
                                             100,    /* num ios */
                                             0,    /* time not used */
                                             1,    /* threads. */
                                             FBE_RDGEN_LBA_SPEC_RANDOM,
                                             test_case_p->start_lba,
                                             test_case_p->min_lba,
                                             test_case_p->max_lba,
                                             FBE_RDGEN_BLOCK_SPEC_RANDOM,
                                             test_case_p->min_blocks,
                                             test_case_p->max_blocks);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Setup the options for sending I/O to the peer.
     */
    status = fbe_api_rdgen_io_specification_set_peer_options(&context_p->start_io.specification,
                                                             peer_options);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Execute the actual test.
     * We assert that the test was started OK. 
     */
    status = fbe_api_rdgen_run_tests(&ovejita_contexts[0], 
                                      fbe_rdgen_unit_test_service_package_id, 1);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Get the status from the context structure.
     * Make sure errors were encountered. 
     */
    status = fbe_api_rdgen_get_status(&ovejita_contexts[0], 1);
    MUT_ASSERT_INT_NOT_EQUAL(status, FBE_STATUS_OK);

    /* Cleanup the context.
     */
    status = fbe_api_rdgen_test_context_destroy(context_p);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    mut_printf(MUT_LOG_TEST_STATUS, "== %s complete ==", __FUNCTION__);
    return;
}
/******************************************
 * end ovejita_invalid_pattern()
 ******************************************/

/*!**************************************************************
 * ovejita_miscompare_test()
 ****************************************************************
 * @brief
 *  This tests a case where we check that rdgen can detect
 *  miscompares.
 *
 * @param peer_options - determines if we should send I/O using the peer.               
 *
 * @return None.
 *
 * @author
 *  8/10/2010 - Created. Rob Foley
 *
 ****************************************************************/

void ovejita_miscompare_test(fbe_api_rdgen_peer_options_t peer_options)
{
    fbe_status_t status;
    fbe_rdgen_unit_test_case_t *test_case_p = &ovejita_normal_cases[0];
    fbe_api_rdgen_context_t *context_p = &ovejita_contexts[0];

    mut_printf(MUT_LOG_TEST_STATUS, "== %s starting ==", __FUNCTION__);
    /* Setup the request.
     */
    status = fbe_api_rdgen_test_context_init(context_p,
                                             fbe_rdgen_unit_test_object_id, 
                                             fbe_rdgen_unit_test_class_id, 
                                             fbe_rdgen_unit_test_package_id,
                                             FBE_RDGEN_OPERATION_WRITE_READ_CHECK,
                                             FBE_RDGEN_PATTERN_ZEROS,
                                             0,    /* passes not used */
                                             1,    /* num ios */
                                             0,    /* time not used */
                                             1,    /* threads. */
                                             FBE_RDGEN_LBA_SPEC_RANDOM,
                                             test_case_p->start_lba,
                                             test_case_p->min_lba,
                                             test_case_p->max_lba,
                                             FBE_RDGEN_BLOCK_SPEC_RANDOM,
                                             test_case_p->min_blocks,
                                             test_case_p->max_blocks);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Setup the options for sending I/O to the peer.
     */
    status = fbe_api_rdgen_io_specification_set_peer_options(&context_p->start_io.specification,
                                                             peer_options);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Execute the actual test.
     * We assert that the test was started OK. 
     */
    status = fbe_api_rdgen_run_tests(&ovejita_contexts[0], 
                                     fbe_rdgen_unit_test_service_package_id, 1);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Get the status from the context structure.
     * Make sure errors were encountered. 
     */
    status = fbe_api_rdgen_get_status(&ovejita_contexts[0], 1);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL(context_p->start_io.status.status, FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL(context_p->start_io.status.block_status, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS);
    MUT_ASSERT_INT_EQUAL(context_p->start_io.status.block_qualifier, FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE);
    MUT_ASSERT_INT_EQUAL(context_p->start_io.status.rdgen_status, FBE_RDGEN_OPERATION_STATUS_SUCCESS);

    /* Cleanup the context.
     */
    status = fbe_api_rdgen_test_context_destroy(context_p);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Now read back and see if we detect a miscompare. 
     */ 
    status = fbe_api_rdgen_test_context_init(context_p,
                                             fbe_rdgen_unit_test_object_id, 
                                             fbe_rdgen_unit_test_class_id, 
                                             fbe_rdgen_unit_test_package_id,
                                             FBE_RDGEN_OPERATION_READ_CHECK,
                                             FBE_RDGEN_PATTERN_LBA_PASS,
                                             0,    /* passes not used */
                                             1,    /* num ios */
                                             0,    /* time not used */
                                             1,    /* threads. */
                                             FBE_RDGEN_LBA_SPEC_RANDOM,
                                             test_case_p->start_lba,
                                             test_case_p->min_lba,
                                             test_case_p->max_lba,
                                             FBE_RDGEN_BLOCK_SPEC_RANDOM,
                                             test_case_p->min_blocks,
                                             test_case_p->max_blocks);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    /* Setup the option to not panic on this miscompare.
     */
    status = fbe_api_rdgen_io_specification_set_options(&context_p->start_io.specification,
                                                        FBE_RDGEN_OPTIONS_DO_NOT_PANIC_ON_DATA_MISCOMPARE);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Setup the options for sending I/O to the peer.
     */
    status = fbe_api_rdgen_io_specification_set_peer_options(&context_p->start_io.specification,
                                                             peer_options);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Execute the actual test.
     * We assert that the test was started OK. 
     */
    status = fbe_api_rdgen_run_tests(&ovejita_contexts[0], 
                                     fbe_rdgen_unit_test_service_package_id, 1);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Get the status from the context structure.
     * Make sure errors were encountered. 
     */
    status = fbe_api_rdgen_get_status(&ovejita_contexts[0], 1);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_GENERIC_FAILURE);
    MUT_ASSERT_INT_EQUAL(context_p->start_io.status.status, FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL(context_p->start_io.status.block_status, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED);
    MUT_ASSERT_INT_EQUAL(context_p->start_io.status.block_qualifier, FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_NOT_POSSIBLE);
    MUT_ASSERT_INT_EQUAL(context_p->start_io.status.rdgen_status, FBE_RDGEN_OPERATION_STATUS_MISCOMPARE);

    /* Cleanup the context.
     */
    status = fbe_api_rdgen_test_context_destroy(context_p);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    mut_printf(MUT_LOG_TEST_STATUS, "== %s completed ==", __FUNCTION__);
    return;
}
/******************************************
 * end ovejita_miscompare_test()
 ******************************************/
/*!**************************************************************
 * ovejita_memory_alloc_free_test()
 ****************************************************************
 * @brief
 *  This tests a case where we send an invalid lba spec.
 *
 * @param peer_options - options for sending I/O through peer.               
 *
 * @return None.
 *
 * @author
 *  5/24/2012 - Created. Rob Foley
 *
 ****************************************************************/

void ovejita_memory_alloc_free_test(void)
{
    fbe_status_t status;
    fbe_api_rdgen_context_t *context_p = &ovejita_contexts[0];
    fbe_api_rdgen_get_stats_t stats;
    fbe_rdgen_filter_t filter;
    mut_printf(MUT_LOG_TEST_STATUS, "== %s starting ==", __FUNCTION__);

    #define OVEJITA_FIRST_MEMORY_SIZE 20
    #define OVEJITA_SECOND_MEMORY_SIZE 40
    status = fbe_api_rdgen_get_stats(&stats, &filter);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* We expect memory to have been allocated by test startup.
     */
    if (stats.memory_size_mb != FBE_API_RDGEN_DEFAULT_MEM_SIZE_MB)
    {
        MUT_FAIL_MSG("No memory allocated")
    }

    status = fbe_api_rdgen_release_dps_memory();
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Releasing memory again should fail.
     */
    status = fbe_api_rdgen_release_dps_memory();
    MUT_ASSERT_INT_NOT_EQUAL(FBE_STATUS_OK, status);

    /* Make sure memory is 0.
     */
    status = fbe_api_rdgen_get_stats(&stats, &filter);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    if (stats.memory_size_mb != 0)
    {
        MUT_FAIL_MSG("memory still allocated")
    }

    /* Issue a single request.  Make sure it fails back with an error since we don't have memory.
     */
    status = fbe_api_rdgen_send_one_io(context_p,
                                       fbe_rdgen_unit_test_object_id, 
                                       fbe_rdgen_unit_test_class_id, 
                                       fbe_rdgen_unit_test_package_id,
                                       FBE_RDGEN_OPERATION_READ_ONLY,
                                       FBE_RDGEN_PATTERN_LBA_PASS,
                                       0, 1, /* lba, blocks */
                                       FBE_RDGEN_OPTIONS_INVALID,
                                       0, 0, /* no expiration or abort time */
                                       FBE_API_RDGEN_PEER_OPTIONS_INVALID);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_STATUS_OK, status);

    /* Alloc dps memory.
     */
    status = fbe_api_rdgen_init_dps_memory(OVEJITA_FIRST_MEMORY_SIZE, FBE_RDGEN_MEMORY_TYPE_CACHE);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Initting memory again should fail.
     */
    status = fbe_api_rdgen_init_dps_memory(OVEJITA_SECOND_MEMORY_SIZE, FBE_RDGEN_MEMORY_TYPE_CMM);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_STATUS_OK, status);

    status = fbe_api_rdgen_get_stats(&stats, &filter);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    if (stats.memory_size_mb < OVEJITA_FIRST_MEMORY_SIZE)
    {
        mut_printf(MUT_LOG_TEST_STATUS, "memory in MB: %d memory_type: %d",
                   stats.memory_size_mb, stats.memory_type);
        MUT_FAIL_MSG("memory not allocated")
    }
    if (stats.memory_type != FBE_RDGEN_MEMORY_TYPE_CACHE)
    {
        mut_printf(MUT_LOG_TEST_STATUS, "memory in MB: %d memory_type: %d",
                   stats.memory_size_mb, stats.memory_type);
        MUT_FAIL_MSG("memory type unexpected")
    }

    /* Issue a single request.  Make sure it succeeds since we should have memory.
     */
    status = fbe_api_rdgen_send_one_io(context_p,
                                       fbe_rdgen_unit_test_object_id, 
                                       fbe_rdgen_unit_test_class_id, 
                                       fbe_rdgen_unit_test_package_id,
                                       FBE_RDGEN_OPERATION_READ_ONLY,
                                       FBE_RDGEN_PATTERN_LBA_PASS,
                                       0, 1, /* lba, blocks */
                                       FBE_RDGEN_OPTIONS_INVALID,
                                       0, 0, /* no expiration or abort time */
                                       FBE_API_RDGEN_PEER_OPTIONS_INVALID);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    status = fbe_api_rdgen_release_dps_memory();
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Make sure memory is 0.
     */
    status = fbe_api_rdgen_get_stats(&stats, &filter);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    if (stats.memory_size_mb != 0)
    {
        MUT_FAIL_MSG("memory still allocated")
    }

    /* Alloc dps memory via cmm.
     */
    status = fbe_api_rdgen_init_dps_memory(OVEJITA_SECOND_MEMORY_SIZE, FBE_RDGEN_MEMORY_TYPE_CMM);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    status = fbe_api_rdgen_get_stats(&stats, &filter);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    if (stats.memory_size_mb < OVEJITA_SECOND_MEMORY_SIZE)
    {
        mut_printf(MUT_LOG_TEST_STATUS, "memory in MB: %d memory_type: %d",
                   stats.memory_size_mb, stats.memory_type);
        MUT_FAIL_MSG("memory not allocated")
    }
    if (stats.memory_type != FBE_RDGEN_MEMORY_TYPE_CMM)
    {
        mut_printf(MUT_LOG_TEST_STATUS, "memory in MB: %d memory_type: %d",
                   stats.memory_size_mb, stats.memory_type);
        MUT_FAIL_MSG("memory type unexpected")
    }
    /* Issue a single request.  Make sure it succeeds since we should have memory.
     */
    status = fbe_api_rdgen_send_one_io(context_p,
                                       fbe_rdgen_unit_test_object_id, 
                                       fbe_rdgen_unit_test_class_id, 
                                       fbe_rdgen_unit_test_package_id,
                                       FBE_RDGEN_OPERATION_READ_ONLY,
                                       FBE_RDGEN_PATTERN_LBA_PASS,
                                       0, 1, /* lba, blocks */
                                       FBE_RDGEN_OPTIONS_INVALID,
                                       0, 0, /* no expiration or abort time */
                                       FBE_API_RDGEN_PEER_OPTIONS_INVALID);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    status = fbe_api_rdgen_release_dps_memory();
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Make sure memory is 0.
     */
    status = fbe_api_rdgen_get_stats(&stats, &filter);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    if (stats.memory_size_mb != 0)
    {
        MUT_FAIL_MSG("memory still allocated")
    }

    /* If the user does not pick a memory size we should default to one for them instead of rejecting the command.
     */
    status = fbe_api_rdgen_init_dps_memory(FBE_API_RDGEN_DEFAULT_MEM_SIZE_MB, FBE_RDGEN_MEMORY_TYPE_INVALID);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    mut_printf(MUT_LOG_TEST_STATUS, "== %s complete ==", __FUNCTION__);
    return;
}
/******************************************
 * end ovejita_memory_alloc_free_test()
 ******************************************/
/*!**************************************************************
 * ovejita_write_background_pattern()
 ****************************************************************
 * @brief
 *  Write a background pattern to the object in order to
 *  clear out the test area.
 *
 * @param object_id - Object id under test.               
 *
 * @return None.
 *
 * @author
 *  5/13/2011 - Created. Rob Foley
 *
 ****************************************************************/

void ovejita_write_background_pattern(fbe_object_id_t object_id)
{
    fbe_api_rdgen_context_t *context_p = &ovejita_contexts[0];
    fbe_status_t status;

    /* Write a background pattern to the LUNs.
     */
    fbe_test_sep_io_setup_fill_rdgen_test_context(context_p,
                                                  object_id,
                                                  FBE_CLASS_ID_INVALID,
                                                  FBE_RDGEN_OPERATION_WRITE_ONLY,
                                                  0x6000, /* use max capacity */
                                                  1024);
    status = fbe_api_rdgen_test_context_run_all_tests(context_p, FBE_PACKAGE_ID_NEIT, 1);
    
    /* Make sure there were no errors.
     */
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.error_count, 0);
    return;
}
/******************************************
 * end ovejita_write_background_pattern()
 ******************************************/

/*!**************************************************************
 * ovejita_overlapping_threads()
 ****************************************************************
 * @brief
 *  This tests where rdgen handles threads that consume
 *  large areas and thus need to wait for each other.
 *
 * @param peer_options - options for sending I/O through peer.  
 * @param rdgen_operation - Operation type to start.
 * @param passes - Number of passes to kick off.
 * @param max_threads - Number of threads to start.
 * @param min_lba - starting/min lba for rdgen.
 * @param max_lba - End of lba range for rdgen threads.
 * @param min_blocks - Smallest block count to use.
 * @param max_blocks - Largest block count to use.
 *
 * @return None.
 *
 * @author
 *  10/10/2011 - Created. Rob Foley
 *
 ****************************************************************/
void ovejita_overlapping_threads(fbe_api_rdgen_peer_options_t peer_options,
                                 fbe_rdgen_operation_t rdgen_operation,
                                 fbe_u32_t passes,
                                 fbe_u32_t max_threads,
                                 fbe_lba_t min_lba,
                                 fbe_lba_t max_lba,
                                 fbe_block_count_t min_blocks,
                                 fbe_block_count_t max_blocks)
{
    fbe_status_t status;
    fbe_api_rdgen_context_t *context_p = &ovejita_contexts[0];
    fbe_u32_t index;

    mut_printf(MUT_LOG_TEST_STATUS, "== %s starting ==", __FUNCTION__);
    for (index = 0; index < max_threads; index++)
    {
        context_p = &ovejita_contexts[index];
        /* Setup the request to be multiple fixed sizes that are overlapping.
         */
        status = fbe_api_rdgen_test_context_init(context_p,
                                                 fbe_rdgen_unit_test_object_id, 
                                                 fbe_rdgen_unit_test_class_id, 
                                                 fbe_rdgen_unit_test_package_id,
                                                 rdgen_operation,
                                                 FBE_RDGEN_PATTERN_LBA_PASS,
                                                 passes,    /* passes */
                                                 0,    /* num ios not used */
                                                 0,    /* time not used */
                                                 1,    /* threads. */
                                                 FBE_RDGEN_LBA_SPEC_RANDOM,
                                                 0, /* start lba */
                                                 0, /* min lba */
                                                 1, /* max lba*/
                                                 FBE_RDGEN_BLOCK_SPEC_RANDOM,
                                                 1, /* min blocks */
                                                 1 /* max blocks */ );
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    
        /* Setup the options for sending I/O to the peer.
         */
        status = fbe_api_rdgen_io_specification_set_peer_options(&context_p->start_io.specification,
                                                                 peer_options);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    }

    /* Execute the actual test.
     * We assert that the test was started OK. 
     */
    status = fbe_api_rdgen_run_tests(&ovejita_contexts[0], fbe_rdgen_unit_test_service_package_id, max_threads);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    for (index = 0; index < max_threads; index++)
    {
        context_p = &ovejita_contexts[index];
        /* Get the status from the context structure.
         * Make sure errors were encountered. 
         */
        status = fbe_api_rdgen_get_status(context_p, 1);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        MUT_ASSERT_INT_EQUAL(context_p->start_io.status.status, FBE_STATUS_OK);
        MUT_ASSERT_INT_EQUAL(context_p->start_io.status.block_status, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS);
        MUT_ASSERT_INT_EQUAL(context_p->start_io.status.block_qualifier, FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE);
        MUT_ASSERT_INT_EQUAL(context_p->start_io.status.rdgen_status, FBE_RDGEN_OPERATION_STATUS_SUCCESS);
        MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.error_count, 0);

        /* We should have exactly as many passes as we asked for.
         */
        MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.pass_count, passes);

        if (rdgen_operation == FBE_RDGEN_OPERATION_WRITE_READ_CHECK)
        {
            /* We issue two I/Os for every pass.  Write/read
             */
            MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.io_count, passes * 2);
        }
        else
        {
            MUT_ASSERT_INT_EQUAL(context_p->start_io.statistics.io_count, passes);
        }
        mut_printf(MUT_LOG_MEDIUM, "== %s thread %d) io_count: 0x%x pass_count: 0x%x lock_conflicts: 0x%x ==", 
                   __FUNCTION__, index, context_p->start_io.statistics.io_count, context_p->start_io.statistics.pass_count,
                   context_p->start_io.statistics.lock_conflicts);
        /* Cleanup the context.
         */
        status = fbe_api_rdgen_test_context_destroy(context_p);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    }

    mut_printf(MUT_LOG_TEST_STATUS, "== %s completed ==", __FUNCTION__);
    return;
}
/******************************************
 * end ovejita_overlapping_threads()
 ******************************************/
/*!*******************************************************************
 * @struct ovejita_overlapped_cases_info_t
 *********************************************************************
 * @brief Struct used to define a test case for overlapped I/Os.
 *        Each rdgen thread we start has these characteristics.
 *
 *********************************************************************/
typedef struct ovejita_overlapped_cases_info_s
{
    fbe_lba_t min_lba;
    fbe_lba_t max_lba;
    fbe_block_count_t min_blocks;
    fbe_block_count_t max_blocks;
}
ovejita_overlapped_cases_info_t;

/*!*******************************************************************
 * @var ovejita_overlapped_cases
 *********************************************************************
 * @brief Array of cases for testing overlapped threads.
 *
 *********************************************************************/
static ovejita_overlapped_cases_info_t ovejita_overlapped_cases[] =
{   /* min lba    max lba    min blocks    max blocks */
    {0,           1,         1,            1}, 
    {0,           4,         2,            4},
    {0,           400,       400,          400},
    {0, 0, 0, 0}, /* terminator */
};
/*!**************************************************************
 * ovejita_overlapping_thread_test()
 ****************************************************************
 * @brief
 *  This tests where rdgen handles threads that consume
 *  large areas and thus need to wait for each other.
 *
 * @param peer_options - options for sending I/O through peer.               
 *
 * @return None.
 *
 * @author
 *  10/10/2011 - Created. Rob Foley
 *
 ****************************************************************/

void ovejita_overlapping_thread_test(fbe_api_rdgen_peer_options_t peer_options)
{
    #define OVEJITA_OVERLAP_TEST_INVALID_FIELD 0xFFFFFFFF
    #define OVEJITA_OVERLAP_TEST_MIN_PASSES 5
    #define OVEJITA_OVERLAP_TEST_MAX_PASSES 50
    #define OVEJITA_OVERLAP_TEST_MIN_THREADS 1
    #define OVEJITA_OVERLAP_TEST_MAX_THREADS 16
    fbe_u32_t threads_index = 0;
    fbe_rdgen_operation_t overlapped_operations[] = {
        FBE_RDGEN_OPERATION_WRITE_READ_CHECK, FBE_RDGEN_OPERATION_WRITE_ONLY, FBE_RDGEN_OPERATION_INVALID, /* terminator */};

    fbe_u32_t threads[] = { OVEJITA_OVERLAP_TEST_MIN_THREADS, 2, OVEJITA_OVERLAP_TEST_INVALID_FIELD, 0 };
    fbe_u32_t passes[] = { OVEJITA_OVERLAP_TEST_MIN_PASSES, OVEJITA_OVERLAP_TEST_INVALID_FIELD, 0 };

    mut_printf(MUT_LOG_TEST_STATUS, "== %s starting ==", __FUNCTION__);
    /* Loop until terminator.
     */
    while (threads[threads_index] != 0)
    {
        fbe_u32_t cases_index = 0;
        fbe_u32_t current_threads = threads[threads_index];

        /* Invalid field means insert random value.
         */
        if (current_threads == OVEJITA_OVERLAP_TEST_INVALID_FIELD)
        {
            current_threads = OVEJITA_OVERLAP_TEST_MIN_THREADS + (fbe_random() % OVEJITA_OVERLAP_TEST_MAX_THREADS);
        }
        /* Loop until terminator.
         */
        while (ovejita_overlapped_cases[cases_index].max_blocks != 0)
        {
            fbe_u32_t passes_index = 0;
            while (passes[passes_index] != 0)
            {
                fbe_u32_t operations_index = 0;
                fbe_u32_t current_passes = passes[passes_index];
                /* Invalid field means insert random value.
                 */
                if (current_passes == OVEJITA_OVERLAP_TEST_INVALID_FIELD)
                {
                    current_passes = OVEJITA_OVERLAP_TEST_MIN_PASSES + (fbe_random() % OVEJITA_OVERLAP_TEST_MAX_PASSES);
                }
                while (overlapped_operations[operations_index] != FBE_RDGEN_OPERATION_INVALID)
                {
                    ovejita_overlapping_threads(peer_options, overlapped_operations[operations_index],
                                                current_passes, current_threads, 
                                                ovejita_overlapped_cases[cases_index].min_lba,
                                                ovejita_overlapped_cases[cases_index].max_lba,
                                                ovejita_overlapped_cases[cases_index].min_blocks,
                                                ovejita_overlapped_cases[cases_index].max_blocks);
                    operations_index++;
                }
                passes_index++;
            }
            cases_index++;
        }
        threads_index++;
    }

    mut_printf(MUT_LOG_TEST_STATUS, "== %s completed ==", __FUNCTION__);
    return;
}
/******************************************
 * end ovejita_overlapping_thread_test()
 ******************************************/
/*!**************************************************************
 * ovejita_run_unit_tests()
 ****************************************************************
 * @brief
 *  Run all the rdgen tests.
 * 
 * @param pvd_520_id - Object id to test for 520.
 * @param pvd_512_id - Object id to test for 512.
 *
 * @return None
 *
 * @author
 *  6/16/2010 - Created. Rob Foley
 *
 ****************************************************************/

void ovejita_run_unit_tests(fbe_object_id_t pvd_520_id, fbe_object_id_t pvd_512_id)
{
    fbe_status_t status;
    fbe_bool_t is_dual_sp = FALSE;
    fbe_u32_t index = 0;
    fbe_api_rdgen_peer_options_t peer_options;
    fbe_u32_t test_level = fbe_sep_test_util_get_raid_testing_extended_level();
    fbe_api_provision_drive_info_t drive_info;

    fbe_api_rdgen_peer_options_t peer_options_array[] = 
    {
        FBE_API_RDGEN_PEER_OPTIONS_INVALID, /* Run locally on SP A. */
        FBE_API_RDGEN_PEER_OPTIONS_SEND_THRU_PEER, /* Run through SP B. */
        FBE_API_RDGEN_PEER_OPTIONS_LOAD_BALANCE, /* Load Balance across both. */
        //FBE_API_RDGEN_PEER_OPTIONS_RANDOM, /* Randomize between SPs. */
        FBE_U32_MAX,
    };

    fbe_rdgen_unit_test_object_id = pvd_520_id;
    fbe_rdgen_unit_test_secondary_object_id = pvd_512_id;

    if(pvd_520_id != FBE_OBJECT_ID_INVALID)
    {
        status = fbe_api_provision_drive_get_info(pvd_520_id, &drive_info);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        status = fbe_test_sep_util_provision_drive_disable_zeroing(pvd_520_id);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK); 
		mut_printf(MUT_LOG_TEST_STATUS, "zero_object_capacity 0x%x default_offset 0x%llx\n", pvd_520_id, drive_info.default_offset);
        fbe_test_sep_util_zero_object_capacity(pvd_520_id, drive_info.default_offset); 
		mut_printf(MUT_LOG_TEST_STATUS, "write_background_pattern 0x%x\n", pvd_520_id);
        ovejita_write_background_pattern(pvd_520_id);
    }

    if(pvd_512_id != FBE_OBJECT_ID_INVALID)
    {
        status = fbe_api_provision_drive_get_info(pvd_512_id, &drive_info);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        status = fbe_test_sep_util_provision_drive_disable_zeroing(pvd_512_id);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        fbe_test_sep_util_zero_object_capacity(pvd_512_id, drive_info.default_offset);
        ovejita_write_background_pattern(pvd_512_id);
    }


    mut_printf(MUT_LOG_TEST_STATUS, "Starting rdgen unit tests\n");

    ovejita_invalid_class(FBE_API_RDGEN_PEER_OPTIONS_INVALID);
    ovejita_invalid_object(FBE_API_RDGEN_PEER_OPTIONS_INVALID);

    is_dual_sp = fbe_test_sep_util_get_dualsp_test_mode();

    /*! @todo we would need to do work to get this to work on hardware.
     */
    if (fbe_test_util_is_simulation())
    {
        if(is_dual_sp)
        {
            fbe_u32_t loop_count = test_level;
            while (loop_count--)
            {
                ovejita_remove_passive_SP(FBE_API_RDGEN_PEER_OPTIONS_SEND_THRU_PEER);
            }
        }
    }
    /* Loop over all the test cases for peer options.
     */
    while (peer_options_array[index] != FBE_U32_MAX)
    {
        peer_options = peer_options_array[index];

        /* In qual or rtl 1 we only test with a limited set of options to limit the test time.
         * Test level 2 and beyond test all options.
         */
        if ((test_level < 2) && (index > OVEJITA_MAX_PEER_OPTIONS))
        {
            break;
        }

        /* We do not want to do the dual sp tests in single sp test.  
         * In the dualsp test we want to do all peer options. 
         */
        if ((is_dual_sp == FBE_FALSE) && (peer_options != FBE_API_RDGEN_PEER_OPTIONS_INVALID))
        {
            break;
        }

        mut_printf(MUT_LOG_TEST_STATUS, "== running with peer options set to %d ==\n", 
                   peer_options);

        ovejita_verify(peer_options);
        ovejita_read_only(peer_options);
        ovejita_write_only(peer_options);
        ovejita_write_read_compare(peer_options);
        ovejita_read_compare(peer_options);

        if (peer_options == FBE_API_RDGEN_PEER_OPTIONS_INVALID)
        {
            ovejita_thread_counts(peer_options, FBE_RDGEN_MAX_THREADS_PER_IO_SPECIFICATION);
        }
        else
        {
            /* Limit our test time by limiting the number of threads when we're going through the peer.
             */
            ovejita_thread_counts(peer_options, 4);
        }
        index++;
    }
    mut_printf(MUT_LOG_TEST_STATUS, "== running error tests ==");

    /* Only need to run these in the single SP test.
     */
    if(!is_dual_sp)
    {
        ovejita_disk_ops_test(FBE_API_RDGEN_PEER_OPTIONS_INVALID);
        ovejita_overlapping_thread_test(FBE_API_RDGEN_PEER_OPTIONS_INVALID);
        ovejita_lba_block_errors(FBE_API_RDGEN_PEER_OPTIONS_INVALID);
        ovejita_thread_count_errors(FBE_API_RDGEN_PEER_OPTIONS_INVALID);
        ovejita_invalid_operation(FBE_API_RDGEN_PEER_OPTIONS_INVALID);
        ovejita_invalid_threads(FBE_API_RDGEN_PEER_OPTIONS_INVALID);
        ovejita_zero_threads(FBE_API_RDGEN_PEER_OPTIONS_INVALID);
        ovejita_invalid_lba_spec(FBE_API_RDGEN_PEER_OPTIONS_INVALID);
        ovejita_invalid_block_spec(FBE_API_RDGEN_PEER_OPTIONS_INVALID);
        ovejita_invalid_pattern(FBE_API_RDGEN_PEER_OPTIONS_INVALID);
        ovejita_memory_alloc_free_test();
    }
    mut_printf(MUT_LOG_TEST_STATUS, "Starting rdgen unit tests complete\n");
    return;
}
/******************************************
 * end ovejita_run_unit_tests()
 ******************************************/
/*!**************************************************************
 * ovejita_unload_neit()
 ****************************************************************
 * @brief
 *  Unload neit package.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  5/13/2011 - Created. Rob Foley
 *
 ****************************************************************/

void ovejita_unload_neit(void)
{
    fbe_test_package_list_t package_list;
    fbe_zero_memory(&package_list, sizeof(package_list)); 
	
    mut_printf(MUT_LOG_LOW, "%s: unload neit package\n", __FUNCTION__);
    
    /* Only unload and reload neit.
     * We are just trying to make sure that rdgen handles peer errors.
     */
    package_list.number_of_packages = 1;
    package_list.package_list[0] = FBE_PACKAGE_ID_NEIT;

    fbe_test_common_util_package_destroy(&package_list);

    fbe_test_common_util_verify_package_destroy_status(&package_list);
    fbe_test_common_util_verify_package_trace_error(&package_list);
    return;
}
/******************************************
 * end ovejita_unload_neit()
 ******************************************/

/*!**************************************************************
 * ovejita_load_neit()
 ****************************************************************
 * @brief
 *  Load the neit package.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  5/13/2011 - Created. Rob Foley
 *
 ****************************************************************/

void ovejita_load_neit(void)
{
    fbe_status_t status;
    mut_printf(MUT_LOG_LOW, "%s: load neit package", __FUNCTION__);
    status = fbe_api_sim_server_load_package(FBE_PACKAGE_ID_NEIT);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    status = fbe_api_sim_server_set_package_entries(FBE_PACKAGE_ID_NEIT);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    fbe_test_pp_util_enable_trace_limits();
    return;
}
/******************************************
 * end ovejita_load_neit()
 ******************************************/
/*!**************************************************************
 * ovejita_load_config()
 ****************************************************************
 * @brief
 *  Setup for an rdgen service test.
 *
 * @param None.               
 *
 * @return None.   
 *
 * @author
 *  9/27/2010 - Created. Rob Foley
 *
 ****************************************************************/
void ovejita_load_config(void)
{
    /* Load the packages and get our object ids to test. 
     */
    fbe_test_pp_util_create_physical_config_for_disk_counts(1, /* Num 520 */
                                                            0, /* Num 4160 */ 
                                                            TERMINATOR_MINIMUM_NON_SYSTEM_DRIVE_CAPACITY);

    return;
}

/**************************************
 * end ovejita_load_config()
 **************************************/
/*!**************************************************************
 * ovejita_test()
 ****************************************************************
 * @brief
 *  Run an rdgen service test.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  6/16/2010 - Created. Rob Foley
 *
 ****************************************************************/

void ovejita_test(void)
{
    fbe_status_t status;
    fbe_test_discovered_drive_locations_t drive_locations;
    fbe_object_id_t pvd_520_object_id;
    fbe_object_id_t pvd_512_object_id;
    fbe_test_raid_group_disk_set_t disk_set;

    fbe_test_sep_util_discover_all_drive_information(&drive_locations);

    /* Find the first 520 drive. */
    if (fbe_test_get_520_drive_location(&drive_locations, &disk_set) == FBE_STATUS_OK)
    {
        mut_printf(MUT_LOG_TEST_STATUS, "Using 520 drive %d_%d_%d",
                   disk_set.bus, 
                   disk_set.enclosure, 
                   disk_set.slot);
        status = fbe_api_provision_drive_get_obj_id_by_location(
                                                               disk_set.bus, 
                                                               disk_set.enclosure, 
                                                               disk_set.slot, 
                                                               &pvd_520_object_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }
    else
    {
        mut_printf(MUT_LOG_TEST_STATUS, "Ovejita cannot run since there are no 520 byte block drives");
        return;
    }

    /* Find the first 4160 drive.
     */
    if (fbe_test_get_4160_drive_location(&drive_locations, &disk_set) == FBE_STATUS_OK)
    {
        mut_printf(MUT_LOG_TEST_STATUS, "Using 4160 drive %d_%d_%d",
                   disk_set.bus, 
                   disk_set.enclosure, 
                   disk_set.slot);
        status = fbe_api_provision_drive_get_obj_id_by_location(
                                                               disk_set.bus, 
                                                               disk_set.enclosure, 
                                                               disk_set.slot, 
                                                               &pvd_512_object_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }
    else
    {
        pvd_512_object_id = FBE_OBJECT_ID_INVALID;
    }

    ovejita_run_unit_tests(pvd_520_object_id, pvd_512_object_id);

    return;
}

/*!**************************************************************
 * ovejita_singlesp_test()
 ****************************************************************
 * @brief
 *  Run an rdgen service test.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  6/16/2010 - Created. Rob Foley
 *
 ****************************************************************/

void ovejita_singlesp_test(void)
{
    /* Disable dualsp mode */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);

    /* Run the test */
    ovejita_test();

    return;
}

/******************************************
 * end ovejita_test()
 ******************************************/

/*!**************************************************************
 * ovejita_dualsp_test()
 ****************************************************************
 * @brief
 *  Run an rdgen service test.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  7/15/2011 - Created. Arun S
 *
 ****************************************************************/

void ovejita_dualsp_test(void)
{
    /* Enable dualsp mode */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_TRUE);

    /* Run the test */
    ovejita_test();

    /* Disable dualsp mode */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);

    return;
}
/******************************************
 * end ovejita_dualsp_test()
 ******************************************/

/*!**************************************************************
 * ovejita_setup()
 ****************************************************************
 * @brief
 *  Setup for an rdgen service test.
 *
 * @param None.               
 *
 * @return None.   
 *
 * @author
 *  6/16/2010 - Created. Rob Foley
 *
 ****************************************************************/
void ovejita_setup(void)
{
    /* We do this so that the fbe api does not trace info messages. 
     * Since we start so many different rdgen operations this gets chatty as 
     * the fbe api for rdgen traces each operation completion. 
     */
    fbe_trace_set_default_trace_level(FBE_TRACE_LEVEL_WARNING);

    if (fbe_test_util_is_simulation())
    {
        ovejita_load_config();
        /* Load the SEP and NEIT */
        elmo_load_sep_and_neit();
    }
    
    /* Initialize any required fields */
    fbe_test_common_util_test_setup_init();
    return;
}

/**************************************
 * end ovejita_setup()
 **************************************/
/*!**************************************************************
 * ovejita_dualsp_setup()
 ****************************************************************
 * @brief
 *  Setup for an rdgen service dual-sp test.
 *
 * @param None.               
 *
 * @return None.   
 *
 * @author
 *  7/14/2011 - Created. Arun S
 *
 ****************************************************************/
void ovejita_dualsp_setup(void)
{
    /* We do this so that the fbe api does not trace info messages. 
     * Since we start so many different rdgen operations this gets chatty as 
     * the fbe api for rdgen traces each operation completion. 
     */
    fbe_trace_set_default_trace_level(FBE_TRACE_LEVEL_WARNING);

    if (fbe_test_util_is_simulation())
    {
        /* Load the config on both SPs. */
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
        ovejita_load_config();

        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
        ovejita_load_config();

        /* We will work primarily with SPA.  The configuration is automatically
         * instantiated on both SPs.  We no longer create the raid groups during 
         * the setup.
         */
        fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
        sep_config_load_sep_and_neit_both_sps();
    }
    return;
}
/**************************************
 * end ovejita_setup()
 **************************************/

/*!**************************************************************
 * ovejita_cleanup()
 ****************************************************************
 * @brief
 *  Cleanup the ovejita test.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  6/16/2010 - Created. Rob Foley
 *
 ****************************************************************/

void ovejita_cleanup(void)
{
    mut_printf(MUT_LOG_LOW, "=== %s starts ===\n", __FUNCTION__);

    if (fbe_test_util_is_simulation())
    {
        fbe_test_sep_util_destroy_neit_sep_physical();
    }
    mut_printf(MUT_LOG_LOW, "=== %s completed ===\n", __FUNCTION__);
    return;
}
/******************************************
 * end ovejita_cleanup()
 ******************************************/

/*!**************************************************************
 * ovejita_dualsp_cleanup()
 ****************************************************************
 * @brief
 *  Cleanup the ovejita Dual SP test.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  7/14/2011 - Created. Arun S
 *
 ****************************************************************/

void ovejita_dualsp_cleanup(void)
{
    mut_printf(MUT_LOG_HIGH, "%s entry", __FUNCTION__);

    /* Always clear dualsp mode */
    fbe_test_sep_util_set_dualsp_test_mode(FBE_FALSE);

    if (fbe_test_util_is_simulation())
    {
        fbe_test_sep_util_destroy_neit_sep_physical_both_sps();
    }

    return;
}
/******************************************
 * end ovejita_dualsp_cleanup()
 ******************************************/

/*************************
 * end file ovejita_test.c
 *************************/


