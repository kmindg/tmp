/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_raid_group_test_state_machine.c
 ***************************************************************************
 *
 * @brief
 *  This file contains functions for testing raid state machines.
 *
 * @version
 *   7/30/2009:  Created. Rob Foley
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/

#include "fbe/fbe_winddk.h"
#include "fbe/fbe_transport.h"
#include "fbe_raid_library_private.h"
#include "fbe_raid_library_test_proto.h"
#include "fbe_raid_library_test.h"
#include "fbe_raid_library_test_state_object.h"
#include "fbe_raid_library_proto.h"

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

/*!**************************************************************
 *  fbe_raid_group_state_machine_tests()
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
 * @author
 *  07/30/09 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t 
fbe_raid_group_state_machine_tests(fbe_raid_group_test_io_range_t *const table_p,
                                   fbe_raid_group_test_sm_test_case_fn const test_function_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t index = 0;
    fbe_lba_t lba;
    fbe_block_count_t blocks;
    fbe_raid_group_test_state_machine_task_t state_machine_task_object;
    fbe_element_size_t element_size;
    fbe_u32_t width;
    //fbe_test_progress_object_t progress_object;

    //fbe_test_progress_object_init(&progress_object, FBE_LDO_TEST_SECONDS_TO_TRACE);
    
    /* First loop over all the tasks.
     */
    while (table_p[index].start_lba != FBE_RAID_GROUP_TEST_INVALID_FIELD &&
           status == FBE_STATUS_OK)
    {
        mut_printf(MUT_LOG_MEDIUM, "pdo test sm: lba: 0x%llx sblocks: 0x%4llx eblocks: "
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

                /* loop over all element sizes.
                 */
                for (element_size = table_p[index].start_element_size;
                    element_size <= table_p[index].end_element_size && status == FBE_STATUS_OK;
                    element_size += 1)
                {
                    /* Loop over all widths.
                     */
                    for (width = table_p[index].start_width;
                        width <= table_p[index].end_width && status == FBE_STATUS_OK;
                        width += 1)
                    {

                        /* Every N seconds display something so the user will
                         * see that progress is being made. 
                         */
#if 0
                        if (fbe_test_progress_object_is_time_to_display(&progress_object))
                        {
                            mut_printf(MUT_LOG_HIGH, 
                                       "pdo test sm: seconds:%d lba:0x%llx blocks:0x%x imported block size:0x%x",
                                       fbe_test_progress_object_get_seconds_elapsed(&progress_object),
                                       lba, blocks, table_p[index].imported_block_size);
                        }
#endif
                        /* Init our task object.
                         */
                        fbe_raid_group_test_state_machine_task_init(&state_machine_task_object,
                                                                    lba,
                                                                    blocks, 
                                                                    table_p[index].exported_block_size,
                                                                    table_p[index].exported_opt_block_size,
                                                                    table_p[index].imported_block_size,
                                                                    table_p[index].imported_opt_block_size,
                                                                    FBE_U32_MAX,
                                                                    element_size, 
                                                                    1, /* elsz per parity stripe */
                                                                    width,
                                                                    -1);
                    }    /* end for all widths */
                }    /* end for all element sizes */
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
/* end fbe_raid_group_state_machine_tests() */

/*!***************************************************************
 * fbe_raid_group_test_execute_state_machine()
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
 * @author
 *  05/19/08 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t fbe_raid_group_test_execute_state_machine(
    fbe_raid_group_test_state_t * const test_state_p,
    fbe_raid_group_test_state_setup_fn_t state_setup_fn_p,
    fbe_raid_group_test_state_execute_fn_t state_fn_p,
    fbe_raid_group_test_state_destroy_fn_t destroy_fn_p,
    fbe_raid_group_test_state_machine_task_t *const task_p)
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
    status = destroy_fn_p(test_state_p);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    return FBE_STATUS_OK;
}
/* end fbe_raid_group_test_execute_state_machine */

/*!***************************************************************
 * fbe_raid_test_state_setup
 ****************************************************************
 * @brief
 *  This function sets up a test state.
 *
 * @param test_state_p - The test state object to use in the test.
 * @param task_p - The test task to use.
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - no error.
 *                 Other values imply there was an error.
 *
 * @author:
 *  11/2/2009 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t fbe_raid_test_state_setup(fbe_raid_group_test_state_t * const test_state_p,
                                       fbe_raid_group_test_state_machine_task_t *const task_p,
                                       fbe_payload_block_operation_opcode_t opcode)
{       
    fbe_status_t status = FBE_STATUS_OK;
    fbe_raid_state_status_t raid_state_status = FBE_RAID_STATE_STATUS_INVALID;
    fbe_raid_iots_t *iots_p = NULL;
    fbe_raid_siots_t *siots_p = NULL;
    fbe_packet_t *packet_p = NULL;
    fbe_payload_ex_t *payload_p = NULL;
    fbe_payload_block_operation_t *block_operation_p = NULL;
    fbe_raid_geometry_t *raid_geometry_p = NULL;
    raid_geometry_p = fbe_raid_group_test_state_get_raid_geometry(test_state_p);

    /* Init the state object.
     */
    fbe_raid_group_test_state_init(test_state_p);
    
    /* Setup the number of sgs we need per sg entry max.
     */
    fbe_raid_group_test_set_max_blks_per_sg(test_state_p, task_p->max_bytes_per_sg_entry);
    siots_p = fbe_raid_group_test_state_get_siots(test_state_p);

    packet_p = fbe_raid_group_test_state_get_io_packet(test_state_p);
    iots_p = fbe_raid_group_test_state_get_iots(test_state_p);
    siots_p = fbe_raid_group_test_state_get_siots(test_state_p);

    MUT_ASSERT_NOT_NULL_MSG( packet_p, "packet is NULL");
    MUT_ASSERT_NOT_NULL_MSG( iots_p, "iots is NULL");

    /*! @note We could initialize our logical drive block size and optimum block size.
     */
    //fbe_ldo_setup_block_sizes( logical_drive_p, task_p->imported_block_size);
    
    /* Setup packet.
     */
    fbe_raid_group_test_build_packet( packet_p,
                                      opcode,
                                      fbe_raid_group_test_state_get_block_edge(test_state_p),
                                      task_p->lba,    /* lba */
                                      task_p->blocks,    /* blocks */
                                      task_p->exported_block_size,    /* exported block size */
                                      task_p->exported_opt_block_size,
                                      fbe_raid_group_test_state_get_io_packet_sg(test_state_p),
                                      /* Context is the io cmd. */
                                      iots_p );

    /* We need to increment the stack level manually. 
     * Normally the send packet function increments the stack level, 
     * but since we are not really sending the packet we need to increment it 
     * manually. 
     */
    fbe_raid_group_test_increment_stack_level(packet_p);

    /* Setup io packet sg.
     */
    fbe_raid_group_test_setup_sg_list( fbe_raid_group_test_state_get_io_packet_sg(test_state_p),
                                       fbe_raid_group_test_state_get_io_packet_sg_memory(test_state_p),
                                       (fbe_u32_t)(task_p->blocks * task_p->exported_block_size), /* Bytes */
                                       /* Max bytes per sg entries */
                                       (fbe_u32_t)fbe_raid_group_test_get_max_blks_per_sg(test_state_p) );

    /* Make sure the check words are sane.
     */
    MUT_ASSERT_TRUE(fbe_raid_group_test_state_validate_check_words(test_state_p) == FBE_STATUS_OK);

    /* Get the block operation.
     */
    payload_p = fbe_transport_get_payload_ex(packet_p);
    block_operation_p = fbe_payload_ex_get_block_operation(payload_p);

    /* Initialize the iots from the io packet.
     */
    raid_state_status = fbe_raid_iots_init(iots_p, packet_p, block_operation_p, 
                                           raid_geometry_p, task_p->lba, task_p->blocks);

    /* Call generate to init the remainder of the fields.
     */
    MUT_ASSERT_INT_EQUAL_MSG(raid_state_status, FBE_STATUS_OK, 
                             "striper: bad status from fbe_raid_iots_init");

    MUT_ASSERT_TRUE_MSG(fbe_raid_iots_get_state(iots_p) == fbe_raid_iots_generate_siots,
                        "striper: generate start state unexpected");

    status = fbe_raid_siots_embedded_init(siots_p, iots_p);
    MUT_ASSERT_INT_EQUAL_MSG(status, FBE_STATUS_OK,
                             "raid_group: bad status from fbe_raid_siots_embedded_init");
    return status;
}
/* end fbe_raid_test_state_setup() */

/*************************
 * end file fbe_raid_group_test_state_machine.c
 *************************/
