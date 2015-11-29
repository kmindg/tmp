#ifndef __FBE_RAID_GROUP_TEST_H__
#define __FBE_RAID_GROUP_TEST_H__
/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_raid_library_test.h
 ***************************************************************************
 *
 * @brief
 *  This file contains definitions for the raid_group unit test.
 *
 * @version
 *   7/30/2009:  Created. Rob Foley
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "mut.h"

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

/*! @def FBE_RAID_GROUP_STATE_MAX_BLKS_PER_SG  
 *  
 *  @brief This is the max number of blocks that we will allow per
 *         sg entry in our testing.
 */
#define FBE_RAID_GROUP_STATE_MAX_BLKS_PER_SG 0xFFFFFFFF 

/*!********************************************************************** 
 * @def FBE_RAID_GROUP_TEST_MAX_SG_LENGTH
 *  
 * @brief 
 *  This is the maximum sg list length we will be allowed. This value is
 *  determined by limits in the backend drivers. Eventually we will allow this
 *  to be 2049, once we are allowed to have a pool of memory, but for now we are
 *  limited by the max chunk size.
 ************************************************************************/
#define FBE_RAID_GROUP_TEST_MAX_SG_LENGTH 2049

/* Simple check word to make sure we do not overrun.
 */
enum { FBE_RAID_GROUP_TEST_STATE_CHECK_WORD = 0x12345678 };

/* This is an invalid marker we use for marking the end of test case stuctures.
 */
#define FBE_RAID_GROUP_TEST_INVALID_FIELD FBE_U32_MAX

/* This structure represents a test case for a range of
 * I/Os.
 */
typedef struct fbe_raid_group_test_io_range_s
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

    /* Geometry info for this range.
     */
    fbe_u32_t start_width;
    fbe_u32_t end_width;
    fbe_u32_t start_element_size;
    fbe_u32_t end_element_size;
}
fbe_raid_group_test_io_range_t;

/* This structure represents a test case for a state machine test.
 */
typedef struct fbe_raid_group_test_state_machine_task_s
{
    fbe_lba_t lba;
    fbe_block_count_t blocks;
    fbe_block_size_t exported_block_size;
    fbe_block_size_t exported_opt_block_size;
    fbe_block_size_t imported_block_size;
    fbe_block_size_t imported_opt_block_size;
    fbe_u32_t        max_bytes_per_sg_entry;
    fbe_u32_t        width;
    fbe_lba_t        capacity;
    fbe_element_size_t element_size;
    fbe_elements_per_parity_t elements_per_parity_stripe;
}
fbe_raid_group_test_state_machine_task_t;

static __inline void fbe_raid_group_test_state_machine_task_init(fbe_raid_group_test_state_machine_task_t *task_p,
                                                          fbe_lba_t lba,
                                                          fbe_block_count_t blocks,
                                                          fbe_block_size_t exported_block_size,
                                                          fbe_block_size_t exported_opt_block_size,
                                                          fbe_block_size_t imported_block_size,
                                                          fbe_block_size_t imported_opt_block_size,
                                                          fbe_u32_t max_bytes_per_sg_entry,
                                                          fbe_element_size_t element_size,
                                                          fbe_elements_per_parity_t  elements_per_parity_stripe,
                                                          fbe_u32_t        width,
                                                          fbe_lba_t        capacity)
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
    task_p->element_size = element_size;
    task_p->elements_per_parity_stripe = elements_per_parity_stripe;
    task_p->width = width;
    task_p->capacity = capacity;
    return;
}


/* This is a test case function prototype for testing state machines.
 */
typedef fbe_status_t (*fbe_raid_group_test_sm_test_case_fn)(fbe_raid_group_test_state_machine_task_t * const task_p);
typedef fbe_status_t (*fbe_raid_group_test_io_case_fn)(fbe_raid_group_test_io_range_t *const cases_p,
                                                       fbe_object_id_t object_id,
                                                       fbe_u32_t max_case_index);

/*************************
 * end file fbe_raid_library_test.h
 *************************/

#endif /* end __FBE_RAID_GROUP_TEST_H__ */
