#ifndef FBE_LOGICAL_DRIVE_TEST_PRIVATE_H
#define FBE_LOGICAL_DRIVE_TEST_PRIVATE_H
/***************************************************************************
 * Copyright (C) EMC Corporation 2007-2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 * fbe_logical_drive_test_private.h
 ***************************************************************************
 *
 * DESCRIPTION
 *  This file contains testing functions for the logical drive object.
 *
 * HISTORY
 *   10/29/2007:  Created. RPF
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_transport.h"
#include "fbe_topology.h"
#include "fbe_logical_drive_private.h"
#include "fbe_logical_drive_io_command.h"
#include "fbe_test_io_api.h"
#include "fbe/fbe_api_rdgen_interface.h"
#include "mut.h"

/*************************
 * ENUMERATIONS
 *************************/

/* Simple check word to make sure we do not overrun.
 */
enum { FBE_LDO_TEST_STATE_CHECK_WORD = 0x12345678 };

/* This is an invalid marker we use for marking the end of test case stuctures.
 */
#define FBE_LDO_TEST_INVALID_FIELD FBE_U32_MAX

/* FBE_LDO_TEST_SECONDS_TO_TRACE
 * This is the number of seconds in between tracing.
 */
enum { FBE_LDO_TEST_SECONDS_TO_TRACE = 3 };

/* This is the max size of sg we will assume
 * when we are testing.
 */
enum { FBE_LDO_TEST_MAX_SG_LENGTH = 128 };

/* This is the number of block sizes we will support.
 * We no longer support 512 byte/sector block sizes
 * 520, 4096 and 4160 are the block sizes. 
 */

#ifdef BLOCKSIZE_512_SUPPORTED
enum { FBE_LDO_TEST_SUPPORTED_BLOCK_SIZE_MAX = 4 };
#else
enum { FBE_LDO_TEST_SUPPORTED_BLOCK_SIZE_MAX = 3 };
#endif

/* This is the standard file size we use for the logical drive tests.
 * We need this to test our get attributes functionality. 
 */
enum { FBE_LDO_DEFAULT_FILE_SIZE_IN_BLOCKS = 0x10b5c730 };

/* This is the number of objects we test with 
 * for logical drive integration tests. 
 * 1 board, 1 port, 1 enclosure, and FBE_LDO_TEST_SUPPORTED_BLOCK_SIZE_MAX 
 * drives each for logical and physical drives. 
 */
enum { FBE_LDO_TEST_NUMBER_OF_OBJECTS = (3 + (2 * FBE_LDO_TEST_SUPPORTED_BLOCK_SIZE_MAX)) };
/*  
 * Object ID we use for testing.
 */
extern fbe_object_id_t fbe_ldo_test_logical_drive_object_id_array[];

#define MUT_ASSERT_FBE_STATUS_OK(m_status)\
MUT_ASSERT_INT_EQUAL(m_status, FBE_STATUS_OK)

#define MUT_ASSERT_FBE_STATUS_FAILED(m_status)\
MUT_ASSERT_INT_EQUAL(m_status, FBE_STATUS_GENERIC_FAILURE)


/* This structure represents a test case for a state machine test.
 */
typedef struct fbe_ldo_test_state_machine_task_s
{
    fbe_lba_t lba;
    fbe_block_count_t blocks;
    fbe_block_size_t exported_block_size;
    fbe_block_size_t exported_opt_block_size;
    fbe_block_size_t imported_block_size;
    fbe_block_size_t imported_opt_block_size;
    fbe_u32_t max_bytes_per_sg_entry;
    fbe_u32_t        sg_start_length;
    fbe_u32_t        sg_end_length;
}
fbe_ldo_test_state_machine_task_t;

static __inline void fbe_ldo_test_state_machine_task_init(fbe_ldo_test_state_machine_task_t *task_p,
                                                   fbe_lba_t lba,
                                                   fbe_block_count_t blocks,
                                                   fbe_block_size_t exported_block_size,
                                                   fbe_block_size_t exported_opt_block_size,
                                                   fbe_block_size_t imported_block_size,
                                                   fbe_block_size_t imported_opt_block_size,
                                                   fbe_u32_t max_bytes_per_sg_entry,
                                                   fbe_u32_t        sg_start_length,
                                                   fbe_u32_t        sg_end_length)
{
    /* Simply init the members from those passed in. 
     */
    task_p->lba = lba;
    task_p->blocks = blocks;
    task_p->exported_block_size = exported_block_size;
    task_p->exported_opt_block_size = exported_opt_block_size;
    task_p->imported_block_size = imported_block_size;
    task_p->imported_opt_block_size = imported_opt_block_size;
    task_p->max_bytes_per_sg_entry = max_bytes_per_sg_entry;
    task_p->sg_end_length = sg_end_length;
    task_p->sg_start_length = sg_start_length;
    return;
}
/* This structure represents a test case for mapping
 * lba and block counts. 
 */
typedef struct fbe_ldo_test_lba_map_task_s
{
    fbe_lba_t exported_lba;
    fbe_block_count_t exported_blocks;
    fbe_block_size_t exported_block_size;
    fbe_block_size_t exported_opt_block_size;
    fbe_block_size_t imported_block_size;
    fbe_block_size_t imported_opt_block_size;
    fbe_lba_t imported_lba;
    fbe_block_count_t imported_blocks;
}
fbe_ldo_test_lba_map_task_t;


/*******************************************************************************
 * fbe_ldo_test_negotiate_block_size_case_t
 *******************************************************************************
 * This structure defines the test cases for testing negotiate block size.
 *
 *******************************************************************************/

typedef struct fbe_ldo_test_negotiate_block_size_case_s
{
    /* Capacity of the device in terms of physical blocks.
     */
    fbe_lba_t imported_capacity;
    /* Physical block size of the device we are testing.
     */
    fbe_block_size_t imported_block_size;

    /* Requested_block_size and optimum block size.
     */
    fbe_block_size_t requested_block_size;
    fbe_block_size_t requested_opt_block_size;

    /* Expected status on the negotiate block size.
     */
    fbe_bool_t b_failure_expected;

    /* Expected negotiate block size data back from LDO.
     */
    fbe_block_transport_negotiate_t negotiate_data_expected;
}
fbe_ldo_test_negotiate_block_size_case_t;

/* This structure represents a test case for a pre-read descriptor.
 */
typedef struct fbe_ldo_test_pre_read_desc_task_s
{
    fbe_lba_t pre_read_desc_lba;
    fbe_block_count_t pre_read_desc_blocks;
    fbe_block_size_t exported_block_size;
    fbe_block_size_t exported_opt_block_size;
    fbe_block_size_t imported_block_size;
    fbe_block_size_t imported_opt_block_size;
    fbe_lba_t imported_lba;
    fbe_block_count_t imported_blocks;
    fbe_lba_t exported_lba;
    fbe_block_count_t exported_blocks;
}
fbe_ldo_test_pre_read_desc_task_t;

/* Define the test cases for state machines.
 */
extern fbe_test_io_task_t fbe_ldo_test_io_tasks[];
extern fbe_test_io_task_t fbe_ldo_test_sg_tasks[];
extern fbe_test_io_task_t fbe_ldo_test_sg_lossy_tasks[];
extern fbe_api_rdgen_test_io_case_t fbe_ldo_test_read_write_io_case_array[];

/* This is a test case function prototype for testing state machines.
 */
typedef fbe_status_t (*fbe_ldo_test_sm_test_case_fn)(fbe_ldo_test_state_machine_task_t * const task_p);
typedef fbe_status_t (*fbe_ldo_test_io_case_fn)(fbe_api_rdgen_test_io_case_t *const cases_p,
                                                fbe_object_id_t object_id,
                                                fbe_u32_t max_case_index);
#endif /* FBE_LOGICAL_DRIVE_TEST_PRIVATE_H */
/****************************************
 * end fbe_logical_drive_test_private.h
 ****************************************/
