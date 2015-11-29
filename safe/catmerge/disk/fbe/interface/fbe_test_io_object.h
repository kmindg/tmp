#ifndef FBE_TEST_IO_OBJECT_H
#define FBE_TEST_IO_OBJECT_H
/***************************************************************************
 * Copyright (C) EMC Corporation 2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_test_io_object.h
 ***************************************************************************
 *
 * @brief
 *  This file contains the definition of the test io object.  This object aids
 *  in the definition and execution of io tests.
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
#include "fbe/fbe_api_logical_drive_interface.h"

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

/* This is an invalid case count. 
 * This tells us that we want to execute all cases.
 */
#define FBE_TEST_IO_INVALID_CASE_COUNT -1

/* Number of iterations to trace after when testing.
 */
#define FBE_TEST_IO_ITERATIONS_TO_TRACE 1000

/* This structure represents a test case for a range of
 * I/Os.
 */
typedef struct fbe_test_io_task_s
{
    /* This is the start and end lba for the range to test over.
     * This controls the outer loop of the test cases. 
     */
    fbe_lba_t start_lba;
    fbe_lba_t end_lba;
    /* This is the start block count and end block count to test.
     * This controls the next inner loop for the test cases. 
     */
    fbe_block_count_t start_blocks;
    fbe_block_count_t end_blocks;
    /* This is the set of test parameters and allows us to 
     * choose the particular drive and block sizes to test with. 
     */
    fbe_block_size_t exported_block_size;
    fbe_block_size_t exported_opt_block_size;
    fbe_block_size_t imported_block_size;
    fbe_block_size_t imported_opt_block_size;

    /* Blocks to inc by when looping over a range.  This controls the block
     * count portion of the loop. 
     */
    fbe_block_count_t increment_blocks; 
}
fbe_test_io_task_t;

/*************************************************
 * fbe_test_io_task_type_t
 *  Enumeration of possible I/O tests. 
 *************************************************/
typedef enum fbe_test_io_task_type_e
{
    FBE_IO_TEST_TASK_TYPE_READ_ONLY,
    FBE_IO_TEST_TASK_TYPE_WRITE_ONLY,
    FBE_IO_TEST_TASK_TYPE_WRITE_SAME_ONLY,
    FBE_IO_TEST_TASK_TYPE_WRITE_READ_COMPARE,
    FBE_IO_TEST_TASK_TYPE_WRITE_SAME_READ_COMPARE,
}
fbe_test_io_task_type_t;

/*************************************************
 * fbe_test_io_pattern_t 
 * Enumeration of possible patterns. 
 *************************************************/
typedef enum fbe_test_io_pattern_e
{
    FBE_IO_TEST_TASK_PATTERN_NONE,
    FBE_IO_TEST_TASK_PATTERN_ADDRESS_PASS_COUNT,
    FBE_IO_TEST_TASK_PATTERN_ZEROS,
    FBE_IO_TEST_TASK_PATTERN_FFS
}
fbe_test_io_pattern_t;

/*************************************************
 * fbe_test_io_case_t 
 *  
 *   Structure for specifying I/O cases to test.
 *  
 *************************************************/
typedef struct fbe_test_io_case_s
{
    /* Specifies the block sizes to test with.
     */
    fbe_block_size_t exported_block_size;
    fbe_block_size_t imported_block_size;

    /* The below control the lba ranges and block counts that we test. 
     * Start/end lba control the outer loop, and start/end blocks controls the 
     * inner loop. 
     * The increment block also is used to increment the blocks for the inner 
     * loop. 
     */
    fbe_lba_t start_lba;
    fbe_lba_t end_lba;
    fbe_block_count_t start_blocks;
    fbe_block_count_t end_blocks;
    fbe_block_count_t increment_blocks;
    /* Specifies the optimal block sizes to test with.
     */
    fbe_block_size_t exported_optimal_block_size;
    fbe_block_size_t imported_optimal_block_size;
}
fbe_test_io_case_t;

/* Forward definition.
 */
struct fbe_test_io_task_object_s;

/* Definition of function pointers for testing with io task objects.
 */
struct fbe_test_io_task_object_s;
typedef fbe_status_t (*fbe_test_io_task_object_function_t)(struct fbe_test_io_task_object_s *const test_obj_p);

/* Set of functions used for an io task.
 */
typedef struct fbe_test_io_task_object_fns_s
{
    /* Function to run prior to starting the test.
     */
    fbe_test_io_task_object_function_t test_setup;

    /* Function which helps drive the test. This function will call the below io
     * functions. 
     */
    fbe_test_io_task_object_function_t test_execute;

    /* Function to run prior to issuing the I/O.
     */
    fbe_test_io_task_object_function_t io_setup;

    /* Function to run to perform I/O.
     */
    fbe_test_io_task_object_function_t io_execute;

    /* Function to run after the I/O completed.
     */
    fbe_test_io_task_object_function_t io_cleanup;

}
fbe_test_io_task_object_fns_t;

/* This is an io task object.
 * This object identifies a test and its current state.
 */
typedef struct fbe_test_io_task_object_s
{
    fbe_test_io_task_type_t e_test_type;

    /* Describes the block sizes and lba ranges to test.
     */
    fbe_test_io_case_t io_case;

    /* Current state of the test including our lba, block count and pass.
     */
    fbe_lba_t current_lba;
    fbe_block_count_t current_blocks;
    fbe_u32_t pass_count;

    /* More parameters of the test.
     */
    fbe_test_io_pattern_t background_pattern;
    fbe_test_io_pattern_t test_pattern;
    fbe_object_id_t object_id;
    fbe_payload_block_operation_opcode_t opcode;

    /* Set of functions used for this test. 
     * Helps isolate the execution of the test from the parameters used to drive 
     * the test. 
     */
    fbe_test_io_task_object_fns_t functions;
}
fbe_test_io_task_object_t;

/*! @struct fbe_test_io_thread_task_t  
 *  @brief This describes the static information about
 *         a given test task.
 *         This describes what kind of test we will run
 *         and the limits and parameters of this test.
 */
typedef struct fbe_test_io_thread_task_s
{
    /*! This is the kind of test, read, write, write/read/check,  
     *  write same/read/check
     */
    fbe_test_io_task_type_t test_type;

    /*! Max blocks to issue with each request.
     */
    fbe_block_count_t blocks;

    /*! Number of packets that will be outstanding 
     *  for every thread that is created. 
     */
    fbe_u32_t packets_per_thread;

    /*! Block size in bytes exported to us by the device.
     */
    fbe_block_size_t exported_block_size;

    /*! Optimum block size requested by us.
     */
    fbe_block_size_t exported_opt_block_size;

    /*! Block size in bytes imported by the device.
     */
    fbe_block_size_t imported_block_size;

    /*! Imported optimal block size used by us.
     */
    fbe_block_size_t imported_opt_block_size;

    /*! Number of seconds to test for. 
     */
    fbe_u32_t seconds_to_test;
}
fbe_test_io_thread_task_t;

/* API for this library.
 */
void fbe_test_io_task_object_init_fns(fbe_test_io_task_object_t *const io_obj_p,
                                      fbe_test_io_task_object_function_t test_setup,
                                      fbe_test_io_task_object_function_t test_execute,
                                      fbe_test_io_task_object_function_t io_setup,
                                      fbe_test_io_task_object_function_t io_execute,
                                      fbe_test_io_task_object_function_t io_cleanup);
void fbe_test_io_task_object_init(fbe_test_io_task_object_t *const io_obj_p,
                                  fbe_object_id_t object_id,
                                  fbe_test_io_pattern_t pattern,
                                  fbe_payload_block_operation_opcode_t opcode);
void fbe_test_io_task_object_set_test_case(fbe_test_io_task_object_t *const io_test_object_p,
                                           fbe_test_io_case_t *const case_p);

fbe_status_t fbe_test_io_task_object_execute_io(fbe_test_io_task_object_t *const io_test_object_p);
fbe_status_t fbe_test_io_task_object_execute(fbe_test_io_task_object_t *const io_test_object_p);

fbe_status_t fbe_test_io_task_object_set_block_sizes(fbe_test_io_task_object_t *const io_test_object_p);
fbe_status_t fbe_test_io_task_object_range_setup(fbe_test_io_task_object_t *const io_test_object_p);
fbe_status_t fbe_test_io_task_object_fill_area(fbe_test_io_task_object_t *const io_test_object_p);
fbe_status_t fbe_test_io_task_object_loop_over_range(fbe_test_io_task_object_t *const io_test_object_p);
fbe_status_t fbe_test_io_task_object_run_all_io_cases(fbe_test_io_task_object_t *const io_test_object_p,
                                                      fbe_test_io_case_t *const cases_p,
                                                      fbe_u32_t max_case);
fbe_status_t fbe_test_io_task_object_issue_verify(fbe_test_io_task_object_t *const io_test_object_p);
fbe_status_t fbe_test_io_task_object_issue_read(fbe_test_io_task_object_t *const io_test_object_p);
fbe_status_t fbe_test_io_task_object_issue_compare_for_write(fbe_test_io_task_object_t *const io_test_object_p);
fbe_status_t fbe_test_io_task_object_issue_read_compare(fbe_test_io_task_object_t *const io_test_object_p);
fbe_status_t fbe_test_io_task_object_issue_write_same(fbe_test_io_task_object_t *const io_test_object_p);
fbe_status_t fbe_test_io_task_object_issue_write(fbe_test_io_task_object_t *const io_test_object_p);

/* APIs for issuing individual I/Os.
 */
fbe_status_t fbe_test_io_read_check_pattern( fbe_object_id_t object_id,
                                             fbe_test_io_pattern_t pattern,
                                             fbe_u32_t pass_count,
                                             fbe_lba_t lba,
                                             const fbe_block_count_t blocks,
                                             fbe_block_size_t exported_block_size,
                                             fbe_block_size_t exported_opt_block_size,
                                             fbe_block_size_t imported_block_size,
                                             fbe_payload_block_operation_status_t * const block_status_p,
                                             fbe_payload_block_operation_qualifier_t * const block_qualifier_p,
                                             fbe_u32_t * const qualifier_p);
fbe_status_t fbe_test_io_write_pattern( fbe_object_id_t object_id,
                                        fbe_test_io_pattern_t pattern,
                                        fbe_u32_t pass_count,
                                        fbe_payload_block_operation_opcode_t opcode,
                                        fbe_lba_t lba,
                                        fbe_block_count_t blocks,
                                        fbe_u32_t exported_block_size,
                                        fbe_block_size_t exported_opt_block_size,
                                        fbe_block_size_t imported_block_size,
                                        fbe_api_block_status_values_t * const block_status_p,
                                        fbe_u32_t * const qualifier_p);
fbe_status_t fbe_test_io_issue_read( fbe_object_id_t object_id,
                                     fbe_payload_block_operation_opcode_t opcode,
                                     fbe_u32_t pass_count,
                                     fbe_lba_t lba,
                                     fbe_block_count_t blocks,
                                     fbe_block_size_t exported_block_size,
                                     fbe_block_size_t exported_opt_block_size,
                                     fbe_block_size_t imported_block_size,
                                     fbe_api_block_status_values_t *const block_status_p,
                                     fbe_u32_t * const qualifier_p );
#endif /* FBE_TEST_IO_OBJECT_H */
/*************************
 * end file fbe_test_io_object.h
 *************************/
