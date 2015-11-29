/***************************************************************************
 * Copyright (C) EMC Corporation 2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_logical_drive_test_state_machine.c
 ***************************************************************************
 *
 * @brief
 *  This file contains logical drive test functions for testing state machines.
 *  This includes code for testing helper functions of state machines such
 *  as sg tests.
 *
 * HISTORY
 *   9/10/2008:  Created. RPF
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_types.h"
#include "fbe_trace.h"
#include "fbe/fbe_logical_drive.h"
#include "fbe_logical_drive_private.h"
#include "fbe_logical_drive_io_command.h"
#include "fbe_logical_drive_test_private.h"
#include "fbe_logical_drive_test_state_object.h"
#include "fbe_logical_drive_test_alloc_mem_mock_object.h"
#include "fbe_logical_drive_test_send_io_packet_mock_object.h"
#include "fbe_logical_drive_test_prototypes.h"
#include "fbe_test_progress_object.h"
#include "mut.h"

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

/* This structure contains the test cases for testing a 
 * range of I/Os, each with a specific block size configuration. 
 *  
 * This table is expected to be used by integration style I/O 
 * tests which are trying to test the logical drive. 
 */
fbe_test_io_task_t fbe_ldo_test_io_tasks[] = 
{
    /* Relatively short run tests.
     * These cases attempt to test a reasonable sub-set of the 
     * possible test cases. 
     * First we always test with the exported block size of 520 
     * since this is what the system currently uses. 
     *  
     * We also choose a reasonable set of physical block sizes 
     * such as 520, 512 and 4k variants. 
     *  
     * We also choose a reasonable set of start lba and block 
     * counts to cover.  When choosing lbas and block counts we 
     * try to cover a full optimal block for each of the tests. 
     */
    /*
     * start end     start  end     exp  exp  imp  imp  
     * lba   lba     blocks blocks  blk  opt  blk  opt
     *                                   blk  size blk
     */
#if 0
    /* These are lossy cases for 512 imported. 
     * We cover several full optimal blocks for each of these cases.
     */
    {0,      100,    0x1,   100,    520, 2,   512,  3, 1},
    {0,      100,    0x1,   100,    520, 2,   512,  5, 1},
    {0,      100,    0x1,   100,    520, 2,   512,  7, 1},
#endif
    /* 512 imported non-lossy cases.
     */
    {0,      0x41,   0x1,   0x41,   520, 64,  512,  65, 1},
    {0x3f,   0x81,   0x180, 0x181,  520, 64,  512,  65, 1},
    {0x7f,   0x101,  0x300, 0x341,  520, 64,  512,  65, 1},
    {0x7f,   0x101,  0x800, 0x841,  520, 64,  512,  65, 1},
    {0x7f,   0x101,  0x500, 0x541,  520, 64,  512,  65, 1},

    /* 4096 imported.  Cover a full optimal block.
     * This takes a long time to run since it is so large to cover, 
     * so we only do one optimal block.
     */
    {0,      513,    1,     513,    520, 512, 4096, 65, 1},

    /* 4160, also cover several full optimal blocks.
     */
    {0,      128,   1,     128,    520, 8,   4160, 1, 1},
    {0,      32,    0x200, 0x250,  520, 8,   4160, 1, 1},
    /* Long running tests.
     * 
     * {0, (4096 * 5),        1, 0x400, 520, 64,  512, 1, 1},
     * {0, (4096 * 5), 1, 0x400, 520, 512, 4096, 1, 1},
     * {0, (4160 * 5), 1, 0x400, 520, 8,   4160, 1, 1},
     */
    /* Insert any new test cases above this point.
     * FBE_LDO_TEST_INVALID_FIELD, means end
     */  
    {FBE_LDO_TEST_INVALID_FIELD, 0, 0, 0, 0, 0, 0, 0} 
};

/* This structure contains the test cases for testing a 
 * range of I/Os, each with a specific block size configuration. 
 *  
 * This is used for the cases where we test scatter gather list 
 * generation. 
 *  
 * Since this code is only used when we have a block conversion to 
 * perform, we only test the cases where the exported optimum block size 
 * is not 1. 
 *  
 * We choose a reasonable sub set of cases where the lba and block ranges are 
 * spread across the optimal block. 
 */
fbe_test_io_task_t fbe_ldo_test_sg_tasks[] = 
{
    /*
     * start end     start  end     exp  exp  imp  imp  
     * lba   lba     blocks blocks  blk  opt  blk  opt
     *                                   blk  size blk
     */
    /* We cover various block counts within a single optimal block.
     */
    {0,    65,       1,     65,     520, 64,  512,  65,  1},

    /* Now try other optimal blocks with various block counts.
     */
    {0x3f,   0x81,   0x180, 0x181,  520, 64,  512,  65, 1},
    {0x7f,   0x101,  0x300, 0x341,  520, 64,  512,  65, 1},
    {0x7f,   0x101,  0x500, 0x541,  520, 64,  512,  65, 1},

    /* now cover an entire 4k optimal block.
     */
    {0,    513,      1,     513,    520, 512, 4096, 65,  1},

    /* Cover various optimal block ranges for 4160.
     */
    {0,    20,       1,     128,    520, 8,   4160, 1,  1},
    {21,   30,       0x200, 0x250,  520, 8,   4160, 1,  1},
    {31,   40,       0x380, 0x480,  520, 8,   4160, 1,  1},

    /* Insert any new test cases above this point.
     * FBE_LDO_TEST_INVALID_FIELD, means end
     */  
    {FBE_LDO_TEST_INVALID_FIELD, 0, 0, 0, 0, 0, 0, 0} 
};

/*! @struct fbe_ldo_test_sg_lossy_tasks 
 *  @brief This structure contains the tasks for testing sg list
 *         functions where the exported optimal size and imported
 *         optimal size are not equal.
 */
fbe_test_io_task_t fbe_ldo_test_sg_lossy_tasks[] = 
{
    /*
     * start end     start  end     exp  exp  imp  imp  
     * lba   lba     blocks blocks  blk  opt  blk  opt
     *                                   blk  size blk
     */
    /* First cover some lossy cases for 512 imported.
     */
    {0,    100,      1,     100,    520, 2,   512,  3,   1},
    {0,    100,      1,     100,    520, 4,   512,  5,   1},
    {0,    100,      1,     100,    520, 6,   512,  7,   1},

    /* Also cover some lossy cases for 4k.
     */
    {0,    100,      1,     100,    520, 7,   4096, 1,   1},
    {0,    100,      1,     100,    520, 15,  4096, 2,   1},

    /* Insert any new test cases above this point.
     * FBE_LDO_TEST_INVALID_FIELD, means end
     */  
    {FBE_LDO_TEST_INVALID_FIELD, 0, 0, 0, 0, 0, 0, 0} 
};

/*!**************************************************************
 * @fn fbe_logical_drive_state_machine_tests(
 *           fbe_test_io_task_t *const table_p,
 *           fbe_ldo_test_sm_test_case_fn const test_function_p)
 ****************************************************************
 * @brief
 *  This function executes tests of the read state machine.
 *
 * @param table_p - The ptr to the table of test tasks to execute
 *                  for this given test function.
 * @param test_function_p - The function to execute for the given test tasks.
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - no error.
 *                 Other values imply there was an error.
 *
 * HISTORY:
 *  11/20/07 - Created. RPF
 *
 ****************************************************************/
fbe_status_t 
fbe_logical_drive_state_machine_tests(fbe_test_io_task_t *const table_p,
                                      fbe_ldo_test_sm_test_case_fn const test_function_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t index = 0;
    fbe_lba_t lba;
    fbe_block_count_t blocks;
    fbe_ldo_test_state_machine_task_t state_machine_task_object;
    fbe_test_progress_object_t progress_object;

    fbe_test_progress_object_init(&progress_object, FBE_LDO_TEST_SECONDS_TO_TRACE);
    
    /* First loop over all the tasks.
     */
    while (table_p[index].start_lba != FBE_LDO_TEST_INVALID_FIELD &&
           status == FBE_STATUS_OK)
    {
        mut_printf(MUT_LOG_MEDIUM, "ldo test sm: lba: 0x%llx sblocks: 0x%4llx eblocks: "
                                   "0x%4llx exported: 0x%4x imported: 0x%4x",
                   (unsigned long long)table_p[index].start_lba,
		   (unsigned long long)table_p[index].start_blocks, 
                   (unsigned long long)table_p[index].end_blocks,
                   table_p[index].exported_block_size, 
                   table_p[index].imported_block_size);

        /* Next loop over the lba range specified.  Break if status is bad.
         */
        for (lba = table_p[index].start_lba;
             lba <= table_p[index].end_lba && status == FBE_STATUS_OK;
             lba += table_p[index].increment_blocks)
        {
            /* Next loop over the block range specified.  Break if status is bad.
             */
            for (blocks = table_p[index].start_blocks;
                 blocks <= table_p[index].end_blocks && status == FBE_STATUS_OK;
                 blocks += table_p[index].increment_blocks)
            {
                /* Every N seconds display something so the user will
                 * see that progress is being made. 
                 */ 
                if (fbe_test_progress_object_is_time_to_display(&progress_object))
                {
                    mut_printf(MUT_LOG_HIGH, 
                               "ldo test sm: seconds:%d lba:0x%llx blocks:0x%llx imported block size:0x%x",
                               fbe_test_progress_object_get_seconds_elapsed(&progress_object),
                               (unsigned long long)lba,
			       (unsigned long long)blocks,
			       table_p[index].imported_block_size);
                }
                /* Init our task object.
                 */
                fbe_ldo_test_state_machine_task_init(&state_machine_task_object,
                                                     lba,
                                                     blocks, 
                                                     table_p[index].exported_block_size,
                                                     table_p[index].exported_opt_block_size,
                                                     table_p[index].imported_block_size,
                                                     table_p[index].imported_opt_block_size,
                                                     FBE_U32_MAX,
                                                     0, 0 /* sg start and end length not used.*/ );

                /* Call the test function.
                 */
                status = (test_function_p)(&state_machine_task_object);

                MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

            } /* End for block range. */
            
        } /* End for lba range. */

        index++;
    } /* End for all test cases. */
    
    return status;
}
/* end fbe_logical_drive_state_machine_tests() */

/*!***************************************************************
 * @fn fbe_ldo_test_execute_state_machine(
 *          fbe_ldo_test_state_t * const test_state_p,
 *          fbe_ldo_test_state_setup_fn_t state_setup_fn_p,
 *          fbe_ldo_test_state_execute_fn_t state_fn_p,
 *          fbe_ldo_test_state_machine_task_t *const task_p)
 ****************************************************************
 * @brief
 *  This function executes tests of the read state machine.
 *
 * @param test_state_p - The state object to use in test.
 * @param state_setup_fn_p - The function to use when setting up
 *                           this test.
 * @param state_fn_p - The state function to test.
 * @param task_p - The given task to execute for this state.
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - no error.
 *                 Other values imply there was an error.
 *
 * HISTORY:
 *  05/19/08 - Created. RPF
 *
 ****************************************************************/
fbe_status_t fbe_ldo_test_execute_state_machine(
    fbe_ldo_test_state_t * const test_state_p,
    fbe_ldo_test_state_setup_fn_t state_setup_fn_p,
    fbe_ldo_test_state_execute_fn_t state_fn_p,
    fbe_ldo_test_state_machine_task_t *const task_p)
{
    fbe_status_t status;

    /* Execute the setup function for the given test state.
     */
    status = state_setup_fn_p(test_state_p, task_p);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Execute the given state machine test.
     */
    status = state_fn_p(test_state_p);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* Destroy the test state.
     */
    MUT_ASSERT_INT_EQUAL(fbe_ldo_test_state_destroy(test_state_p),
                         FBE_STATUS_OK);

    return FBE_STATUS_OK;
}
/* end fbe_ldo_test_execute_state_machine */

/*!***************************************************************
 * @fn fbe_ldo_test_sg_cases(fbe_test_io_task_t *const task_p,
 *                           fbe_ldo_test_count_setup_sgs_fn test_fn)
 ****************************************************************
 * @brief
 *  This function executes sg tests.
 *
 * @param task_p - The test task to execute.
 * @param test_fn - The sg test function to execute.
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - no error.
 *                 Other values imply there was an error.
 *
 * HISTORY:
 *  11/20/07 - Created. RPF
 *
 ****************************************************************/
fbe_status_t fbe_ldo_test_sg_cases(fbe_test_io_task_t *const task_p,
                                   fbe_ldo_test_count_setup_sgs_fn test_fn)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t index = 0;
    fbe_lba_t lba;
    fbe_block_count_t blocks;
    fbe_ldo_test_state_machine_task_t state_machine_task_object;

    /* Get the state object. 
     */
    fbe_ldo_test_state_t * const test_state_p = fbe_ldo_test_get_state_object();
    fbe_test_progress_object_t progress_object;

    fbe_test_progress_object_init(&progress_object, FBE_LDO_TEST_SECONDS_TO_TRACE);

    /* First loop over all the tasks.  Exit if status is bad.
     */
    while (task_p[index].start_lba != FBE_LDO_TEST_INVALID_FIELD &&
           status == FBE_STATUS_OK)
    {
        mut_printf(MUT_LOG_MEDIUM, "ldo test sm: lba: 0x%llx sblocks: 0x%4llx eblocks: 0x%4llx exported: 0x%4x imported: 0x%4x",
                (unsigned long long)task_p[index].start_lba,
		(unsigned long long)task_p[index].start_blocks, 
                (unsigned long long)task_p[index].end_blocks,
                task_p[index].exported_block_size, 
                task_p[index].imported_block_size);

        /* Next loop over the lba range specified.  Break if status is bad.
         */
        for (lba = task_p[index].start_lba;
             lba <= task_p[index].end_lba && status == FBE_STATUS_OK;
             lba++)
        {
            /* Next loop over the block range specified.  Break if status is bad.
             */
            for (blocks = task_p[index].start_blocks;
                 blocks <= task_p[index].end_blocks && status == FBE_STATUS_OK;
                 blocks++)
            {
                /* Every N seconds display something so the user will
                 * see that progress is being made. 
                 */ 
                if (fbe_test_progress_object_is_time_to_display(&progress_object))
                {
                    mut_printf(MUT_LOG_HIGH, 
                               "ldo test sg: seconds:%d lba:0x%llx blocks:0x%llx imported block size:0x%x",
                               fbe_test_progress_object_get_seconds_elapsed(&progress_object),
                               (unsigned long long)lba,
			       (unsigned long long)blocks,
			       task_p[index].imported_block_size);
                }
                /* Always test with at least one sg entry.
                 * Init our task object.
                 */
                fbe_ldo_test_state_machine_task_init(&state_machine_task_object,
                                                     lba,
                                                     blocks, 
                                                     task_p[index].exported_block_size,
                                                     task_p[index].exported_opt_block_size,
                                                     task_p[index].imported_block_size,
                                                     task_p[index].imported_opt_block_size,
                                                     /* Max bytes per sg entry.  */
                                                     (fbe_u32_t)(blocks * task_p[index].exported_block_size),
                                                     0, 0 /* sg start and end length not used.*/ );

                /* Call the test function.
                 */
                status = (test_fn)(test_state_p, &state_machine_task_object);
                MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

                /* If the block count is larger than one, then test with the max sg entries allowed.
                 */
                if (blocks > 1)
                {
                    fbe_ldo_test_state_machine_task_init(&state_machine_task_object,
                                                         lba,
                                                         blocks, 
                                                         task_p[index].exported_block_size,
                                                         task_p[index].exported_opt_block_size,
                                                         task_p[index].imported_block_size,
                                                         task_p[index].imported_opt_block_size,
                                                         /* Max bytes per sg entry. */
                                                         (fbe_u32_t)((blocks / 2) * task_p[index].exported_block_size),
                                                         0, 0 /* sg start and end length not used.*/ );
                    /* Call the test function.
                     */ 
                    status = (test_fn)(test_state_p, &state_machine_task_object);
                    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
                }
                
            } /* End for block range. */
            
        } /* End for lba range. */

        index++;
    } /* End for all test cases. */
    
    return status;
}
/* end fbe_ldo_test_sg_cases() */

/*!***************************************************************
 * @fn fbe_ldo_test_setup_sg_list(fbe_sg_element_t *const sg_p,
 *                                fbe_u8_t *const memory_p,
 *                                fbe_u32_t bytes,
 *                                fbe_u32_t requested_max_bytes_per_sg)
 ****************************************************************
 * @brief
 *  This function sets up an sg list based on the input parameters.
 *
 * @param sg_p - Sg list to setup.
 * @param memory_p - Pointer to memory to use in sg.
 * @param bytes - Bytes to setup in sg.
 * @param max_bytes_per_sg - Maximum number of bytes per sg entry.
 *                           Allows us to setup longer sg lists.
 *
 * @return
 *  None.
 *
 * HISTORY:
 *  11/29/07 - Created. RPF
 *
 ****************************************************************/
void fbe_ldo_test_setup_sg_list(fbe_sg_element_t *const sg_p,
                                fbe_u8_t *const memory_p,
                                fbe_u32_t bytes,
                                fbe_u32_t max_bytes_per_sg)
{
    fbe_u32_t current_bytes;
    fbe_u32_t bytes_remaining = bytes;
    fbe_sg_element_t *current_sg_p = sg_p;
    
    if (max_bytes_per_sg == 0)
    {
        /* If the caller did not specify the max bytes per sg,
         * then assume we can put as much as we want in each sg entry.
         */
        max_bytes_per_sg = FBE_U32_MAX;
    }

    /* Loop over all the bytes that we are adding to this sg list.
     */
    while (bytes_remaining != 0)
    {
        if (max_bytes_per_sg > bytes_remaining)
        {
            /* Don't allow current_bytes to be negative.
             */
            current_bytes = bytes_remaining;
        }
        else
        {
            current_bytes = max_bytes_per_sg;
        }

        /* Setup this sg element.
         */
        fbe_sg_element_init(current_sg_p, current_bytes, memory_p);
        fbe_sg_element_increment(&current_sg_p);
        bytes_remaining -= current_bytes;

    } /* end while bytes remaining != 0 */

    /* Terminate the list.
     */
    fbe_sg_element_terminate(current_sg_p);
    return;
}
/* end fbe_ldo_test_setup_sg_list */

/*!**************************************************************
 * @fn fbe_ldo_test_increment_stack_level(fbe_packet_t *const packet_p)
 ****************************************************************
 * @brief
 *  A test function helper that increments the stack level for us.
 *
 * @param  packet_p - The packet to increment the stack level for.              
 *
 * @return None.   
 *
 * HISTORY:
 *  10/2/2008 - Created. RPF
 *
 ****************************************************************/

void fbe_ldo_test_increment_stack_level(fbe_packet_t *const packet_p)
{
    /* We need to increment the stack level manually. 
     * Normally the send packet function increments the stack level, 
     * but since we are not really sending the packet we need to increment it 
     * manually. 
     */
    fbe_status_t status;
    fbe_payload_ex_t *payload_p;
    payload_p = fbe_transport_get_payload_ex(packet_p);

    status = fbe_payload_ex_increment_block_operation_level(payload_p);
    MUT_ASSERT_FBE_STATUS_OK(status);

    return;
}
/******************************************
 * end fbe_ldo_test_increment_stack_level()
 ******************************************/

/*!**************************************************************
 * @fn fbe_ldo_test_allocate_and_increment_stack_level(fbe_packet_t *const packet_p)
 ****************************************************************
 * @brief
 *  A test function helper that increments the stack level for us.
 *
 * @param  packet_p - The packet to increment the stack level for.              
 *
 * @return None.   
 *
 * HISTORY:
 *  10/2/2008 - Created. RPF
 *
 ****************************************************************/

void fbe_ldo_test_allocate_and_increment_stack_level(fbe_packet_t *const packet_p)
{
    /* First allocate the stack level.
     *  
     * Then increment the stack level manually. Normally the send packet 
     * function increments the stack level, but since we are not really sending 
     * the packet we need to increment it manually. 
     */
    fbe_status_t status;
    fbe_payload_ex_t *payload_p;

    payload_p = fbe_transport_get_payload_ex(packet_p);
    /* We need to allocate the block operation and increment the stack level 
     * to point to the block operation. 
     */
    fbe_payload_ex_allocate_block_operation(payload_p);

    status = fbe_payload_ex_increment_block_operation_level(payload_p);
    MUT_ASSERT_FBE_STATUS_OK(status);

    return;
}
/******************************************
 * end fbe_ldo_test_allocate_and_increment_stack_level()
 ******************************************/
/*************************
 * end file fbe_logical_drive_test_state_machine.c
 *************************/


