/***************************************************************************
 * Copyright (C) EMC Corporation 2007-2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_logical_drive_test_read.c
 ***************************************************************************
 *
 * @brief
 *  This file contains testing functions for the logical drive object's
 *  read state machine.
 * 
 * @ingroup logical_drive_class_tests
 *
 * @version
 *   11/20/2007:  Created. RPF
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
#include "fbe_logical_drive_test_alloc_io_cmd_mock_object.h"
#include "fbe_logical_drive_test_send_io_packet_mock_object.h"
#include "fbe_logical_drive_test_prototypes.h"
#include "mut.h"


/*************************
 *   FUNCTION DEFINITIONS
 *************************/
fbe_status_t fbe_logical_drive_read_state_machine_test_case(
    fbe_ldo_test_state_machine_task_t *const task_p);

/*************************
 *   FUNCTIONS
 *************************/

/*!***************************************************************
 * @fn fbe_ldo_read_state_machine_tests(void)
 ****************************************************************
 * @brief
 *  This function executes tests of the read state machine.
 *
 * @param  - None
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
fbe_ldo_read_state_machine_tests(void)
{
    fbe_status_t status = FBE_STATUS_OK;

    /* Test all the logical drive read cases for these test tasks.
     */
    status = fbe_logical_drive_state_machine_tests(fbe_ldo_test_io_tasks,
                                                   fbe_logical_drive_read_state_machine_test_case);
    
    return status;
}
/* end fbe_ldo_read_state_machine_tests() */

/*!***************************************************************
 * @fn fbe_ldo_test_state_setup_read(
 *                   fbe_ldo_test_state_t * const test_state_p,
 *                   fbe_ldo_test_state_machine_task_t *const task_p)
 ****************************************************************
 * @brief
 *  This function sets up for a read state machine test.
 *
 * @param test_state_p - The test state object to use in the test.
 * @param task_p - The test task to use.
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - no error.
 *                 Other values imply there was an error.
 *
 * HISTORY:
 *  11/20/07 - Created. RPF
 *
 ****************************************************************/
fbe_status_t fbe_ldo_test_state_setup_read(fbe_ldo_test_state_t * const test_state_p,
                                           fbe_ldo_test_state_machine_task_t *const task_p)
{
    fbe_logical_drive_io_cmd_t *io_cmd_p;
    fbe_packet_t *io_packet_p;
    fbe_logical_drive_t *logical_drive_p;

    /* Init the state object.
     */
    fbe_ldo_test_state_init(test_state_p);

    /* Setup the number of sgs we need per sg entry max.
     */
    fbe_ldo_test_set_max_blks_per_sg(test_state_p, task_p->max_bytes_per_sg_entry);

    logical_drive_p = fbe_ldo_test_state_get_logical_drive(test_state_p);
    io_packet_p = fbe_ldo_test_state_get_io_packet(test_state_p);
    io_cmd_p = fbe_ldo_test_state_get_io_cmd(test_state_p);

    MUT_ASSERT_TRUE_MSG( logical_drive_p != NULL, "Logical drive is NULL");
    MUT_ASSERT_TRUE_MSG( io_packet_p != NULL, "io packet is NULL");
    MUT_ASSERT_TRUE_MSG( io_cmd_p != NULL, "io cmd is NULL");

    /* Initialize our logical drive block size and optimum block size.
     */
    fbe_ldo_setup_block_sizes( logical_drive_p, task_p->imported_block_size);
    
    /* Setup io packet.
     */
    fbe_ldo_build_sub_packet( io_packet_p,
                              FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,
                              task_p->lba,    /* lba */
                              task_p->blocks,    /* blocks */
                              task_p->exported_block_size,    /* exported block size */
                              task_p->exported_opt_block_size,
                              fbe_ldo_test_state_get_io_packet_sg(test_state_p),
                              /* Context is the io cmd. */
                              io_cmd_p );

    /* We need to increment the stack level manually. 
     * Normally the send packet function increments the stack level, 
     * but since we are not really sending the packet we need to increment it 
     * manually. 
     */
    fbe_ldo_test_increment_stack_level(io_packet_p);

    /* Setup io packet sg.
     */
    fbe_ldo_test_setup_sg_list( fbe_ldo_test_state_get_io_packet_sg(test_state_p),
                                fbe_ldo_test_state_get_io_packet_sg_memory(test_state_p),
                                (fbe_u32_t)(task_p->blocks * task_p->exported_block_size), /* Bytes */
                                /* Max bytes per sg entries */
                                (fbe_u32_t)fbe_ldo_test_get_max_blks_per_sg(test_state_p) );

    /* Call generate to init the remainder of the fields.
     */
    MUT_ASSERT_TRUE_MSG(fbe_ldo_io_cmd_generate(io_cmd_p) == FBE_LDO_STATE_STATUS_EXECUTING,
                        "LDO:  io cmd generate did not return executing");

    MUT_ASSERT_TRUE_MSG(fbe_ldo_io_cmd_get_state(io_cmd_p) == fbe_ldo_io_cmd_read_state0,
                        "LDO:  io cmd state not read state0");

    /* Make sure the check words are sane.
     */
    MUT_ASSERT_TRUE(fbe_ldo_test_state_validate_check_words(test_state_p) == FBE_STATUS_OK);
    return FBE_STATUS_OK;  
}
/* end fbe_ldo_test_state_setup_read() */

/*!**************************************************************
 * @fn fbe_ldo_test_get_payload_bytes(fbe_payload_ex_t *payload_p)
 ****************************************************************
 * @brief
 *  Count the total number of bytes in all the sgs of the payload.
 *  
 * @param payload_p - The current payload to check bytes of.
 * 
 * @return fbe_u32_t - Total bytes in the payload sgs.
 *
 * HISTORY:
 *  11/5/2008 - Created. RPF
 *
 ****************************************************************/

fbe_u32_t fbe_ldo_test_get_payload_bytes(fbe_payload_ex_t *const payload_p)
{
    fbe_u32_t total_bytes = 0;
    fbe_sg_element_t *pre_sg_p;
    fbe_sg_element_t *sg_p;
    fbe_sg_element_t *post_sg_p;
    fbe_payload_ex_get_pre_sg_list(payload_p, &pre_sg_p);
    fbe_payload_ex_get_post_sg_list(payload_p, &post_sg_p);
    fbe_payload_ex_get_sg_list(payload_p, &sg_p, NULL);

    /* The pre and the post should both not be too long.
     */
    MUT_ASSERT_TRUE(fbe_sg_element_count_entries(pre_sg_p) < FBE_PAYLOAD_SG_ARRAY_LENGTH);
    MUT_ASSERT_TRUE(fbe_sg_element_count_entries(post_sg_p) < FBE_PAYLOAD_SG_ARRAY_LENGTH);

    /* Sum the total bytes in all the sg lists in the packet.
     */
    total_bytes += fbe_sg_element_count_list_bytes(pre_sg_p);
    total_bytes += fbe_sg_element_count_list_bytes(sg_p);
    total_bytes += fbe_sg_element_count_list_bytes(post_sg_p);

    return total_bytes;
}
/**************************************
 * end fbe_ldo_test_get_payload_bytes()
 **************************************/

/*!***************************************************************
 * @fn fbe_ldo_test_handle_read(
 *                      fbe_ldo_test_state_t * const test_state_p)
 ****************************************************************
 * @brief
 *  This function tests fbe_ldo_handle_read.
 *
 * @param test_state_p - The test state with info about the configuration.
 *
 * @return fbe_status_t - FBE_STATUS_OK - no error.
 *                         Other values imply there was an error.
 *
 * HISTORY:
 *  10/17/08 - Created. RPF
 *
 ****************************************************************/
fbe_status_t fbe_ldo_test_handle_read(fbe_ldo_test_state_t * const test_state_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_logical_drive_io_cmd_t *io_cmd_p = NULL;
    fbe_packet_t *io_packet_p;
    fbe_logical_drive_t *logical_drive_p;
    fbe_ldo_test_send_io_packet_mock_t *mock_p = NULL;
    fbe_payload_block_operation_opcode_t opcode;
    fbe_payload_ex_t *payload_p = NULL;
    fbe_u32_t bytes_in_payload_sg;
    fbe_u32_t bytes_in_block_cmd;
    fbe_bool_t b_aligned_start;
    fbe_bool_t b_aligned_end;
    fbe_sg_element_t *sg_p = NULL;

    logical_drive_p = fbe_ldo_test_state_get_logical_drive(test_state_p);
    io_packet_p = fbe_ldo_test_state_get_io_packet(test_state_p);
    io_cmd_p = fbe_ldo_test_state_get_io_cmd(test_state_p);
    payload_p = fbe_transport_get_payload_ex(io_packet_p);

    /* Get the opcode from the packet, we'll make sure below that the 
     * payload we send has the same opcode as the payload we received. 
     */
    opcode = ldo_packet_get_block_cmd_opcode(io_packet_p);
    
    /* Setup our mock io packet send function.
     */
    fbe_ldo_set_send_io_packet_fn(fbe_ldo_send_io_packet_mock);
    mock_p = fbe_ldo_test_get_send_packet_mock();
    fbe_ldo_send_io_packet_mock_init(mock_p, NULL, /* no io cmd */ 
                                     FBE_STATUS_OK, 0);

    MUT_ASSERT_FALSE_MSG(fbe_ldo_io_cmd_is_padding_required(io_cmd_p),
                         "Only Cmds that do not need padding should call this function.");

    MUT_ASSERT_TRUE_MSG( logical_drive_p != NULL, "Logical drive is NULL");
    MUT_ASSERT_TRUE_MSG( io_packet_p != NULL, "io packet is NULL");
    MUT_ASSERT_TRUE_MSG( io_cmd_p != NULL, "io cmd is NULL");

    /* Save info about alignment.
     */
    if (ldo_packet_get_block_cmd_blocks(io_packet_p) %
        ldo_packet_get_block_cmd_optimum_block_size(io_packet_p))
    {
        /* Start is not aligned.
         */
        b_aligned_start = FALSE;
    }
    else
    {
        /* Start is aligned.
         */
        b_aligned_start = TRUE;
    }
     /* Save info about alignment.
     */
    if ( (ldo_packet_get_block_cmd_lba(io_packet_p) + 
          ldo_packet_get_block_cmd_blocks(io_packet_p)) %
         ldo_packet_get_block_cmd_optimum_block_size(io_packet_p))
    {
        /* End is not aligned.
         */
        b_aligned_end = FALSE;
    }
    else
    {
        /* End is aligned.
         */
        b_aligned_end = TRUE;
    }

    /* Call handle read with our io packet.
     * fbe_ldo_handle_read will eventually get called. 
     */
    status = fbe_logical_drive_block_transport_handle_io(logical_drive_p, io_packet_p);

    /* Make sure the status was set as expected.
     */
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_PENDING);

    /* We should have only called send packet once.
     */
    MUT_ASSERT_INT_EQUAL_MSG(mock_p->packet_sent_count, 1,
                             "LDO: handle_read send packet mock not called");

    /* Make sure we have the right number of bytes in this sg.
     */
    MUT_ASSERT_INT_EQUAL_MSG(mock_p->sg_list_bytes, mock_p->io_packet_bytes,
                             "LDO: handle_read sg list bytes incorrect");
    
    /* If there are not sufficient bytes in the sg list, we will get back a 
     * zero for sg entries.
     */
    MUT_ASSERT_INT_NOT_EQUAL_MSG(mock_p->sg_entries, 0, "io packet sg is incorrect");

    /* Make sure the sub io packet has a write same command.
     */
    MUT_ASSERT_INT_EQUAL_MSG(ldo_packet_get_block_cmd_opcode(io_packet_p), 
                             opcode, 
                             "handle read opcode not set as expected");

    /* Validate the number of bytes in the payload matches the number of bytes
     * in the payload sgs.  
     */
    bytes_in_payload_sg = fbe_ldo_test_get_payload_bytes(payload_p);
    bytes_in_block_cmd = ((fbe_u32_t)(ldo_packet_get_block_cmd_blocks(io_packet_p)) * 
                          ldo_packet_get_block_cmd_block_size(io_packet_p));
    MUT_ASSERT_INT_EQUAL(bytes_in_block_cmd, bytes_in_payload_sg);

    fbe_payload_ex_get_pre_sg_list(payload_p, &sg_p);
    if (!b_aligned_start)
    {
        /* If the start is not aligned then the pre-sg should be filled.
         */
        MUT_ASSERT_INT_NOT_EQUAL(fbe_sg_element_count_list_bytes(sg_p), 0); 
    }
    else
    {
        /* If the start is aligned, then the pre-sg should be empty.
         */
        MUT_ASSERT_INT_EQUAL(fbe_sg_element_count_list_bytes(sg_p), 0);
    }

    fbe_payload_ex_get_post_sg_list(payload_p, &sg_p);
    if (!b_aligned_end)
    { 
        /* If the end is not aligned then the post-sg should be filled.
         */ 
        MUT_ASSERT_INT_NOT_EQUAL(fbe_sg_element_count_list_bytes(sg_p), 0); 
    }
    else
    {
        /* If the end is aligned, then the post-sg should be empty.
         */
        MUT_ASSERT_INT_EQUAL(fbe_sg_element_count_list_bytes(sg_p), 0);
    }

    return FBE_STATUS_OK;  
}
/* end fbe_ldo_test_handle_read() */

/*!***************************************************************
 * @fn fbe_ldo_test_handle_read_remainder_case(
 *                      fbe_ldo_test_state_t * const test_state_p)
 ****************************************************************
 * @brief
 *  This function tests fbe_ldo_handle_read case where an sg
 *  is passed down that has extra data in it.
 *
 * @param test_state_p - The test state with info about the configuration.
 *
 * @return fbe_status_t - FBE_STATUS_OK - no error.
 *                         Other values imply there was an error.
 *
 * HISTORY:
 *  10/17/08 - Created. RPF
 *
 ****************************************************************/
fbe_status_t fbe_ldo_test_handle_read_remainder_case(fbe_ldo_test_state_t * const test_state_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_logical_drive_io_cmd_t *io_cmd_p;
    fbe_packet_t *io_packet_p;
    fbe_logical_drive_t *logical_drive_p;
    fbe_ldo_test_send_io_packet_mock_t *send_mock_p;
    fbe_ldo_test_alloc_io_cmd_mock_t *alloc_mock_p;
    fbe_payload_block_operation_opcode_t opcode;
    fbe_payload_ex_t *payload_p;
    fbe_bool_t b_aligned_end;
    fbe_bool_t b_aligned_start;
    fbe_bool_t b_padding_required;
    fbe_sg_element_t *sg_p;
    fbe_u32_t old_sg_count;

    logical_drive_p = fbe_ldo_test_state_get_logical_drive(test_state_p);
    io_packet_p = fbe_ldo_test_state_get_io_packet(test_state_p);
    io_cmd_p = fbe_ldo_test_state_get_io_cmd(test_state_p);
    payload_p = fbe_transport_get_payload_ex(io_packet_p);
    fbe_payload_ex_get_sg_list(payload_p, &sg_p, NULL);

    /* Save the sg count so we can restore it later.
     */
    old_sg_count = fbe_sg_element_count(sg_p);

    /* Set count to a larger value so it appears there is more data.
     */
    sg_p->count = old_sg_count + 1;

    /* Get the opcode from the packet, we'll make sure below that the 
     * payload we send has the same opcode as the payload we received. 
     */
    opcode = ldo_packet_get_block_cmd_opcode(io_packet_p);
    b_padding_required = fbe_ldo_io_cmd_is_padding_required(io_cmd_p);
    
    /* Setup our mock io packet send function.
     */
    fbe_ldo_set_send_io_packet_fn(fbe_ldo_send_io_packet_mock);
    send_mock_p = fbe_ldo_test_get_send_packet_mock();
    fbe_ldo_send_io_packet_mock_init(send_mock_p, NULL, /* no io cmd */ 
                                     FBE_STATUS_OK, 0);
    
    /* Setup our mock allocate io cmd function.
     */
    fbe_ldo_set_allocate_io_cmd_fn(fbe_ldo_test_alloc_io_cmd_mock);
    alloc_mock_p = fbe_ldo_test_get_alloc_io_cmd_mock();
    fbe_ldo_alloc_io_cmd_mock_init(alloc_mock_p);

    MUT_ASSERT_FALSE_MSG(fbe_ldo_io_cmd_is_aligned(io_cmd_p),
                         "Only unaligned should call this function.");

    MUT_ASSERT_TRUE_MSG( logical_drive_p != NULL, "Logical drive is NULL");
    MUT_ASSERT_TRUE_MSG( io_packet_p != NULL, "io packet is NULL");
    MUT_ASSERT_TRUE_MSG( io_cmd_p != NULL, "io cmd is NULL");

    /* Save info about alignment.
     */
    if (ldo_packet_get_block_cmd_blocks(io_packet_p) %
        ldo_packet_get_block_cmd_optimum_block_size(io_packet_p))
    {
        /* Start is not aligned.
         */
        b_aligned_start = FALSE;
    }
    else
    {
        /* Start is aligned.
         */
        b_aligned_start = TRUE;
    }
     /* Save info about alignment.
     */
    if ( (ldo_packet_get_block_cmd_lba(io_packet_p) + 
          ldo_packet_get_block_cmd_blocks(io_packet_p)) %
         ldo_packet_get_block_cmd_optimum_block_size(io_packet_p))
    {
        /* End is not aligned.
         */
        b_aligned_end = FALSE;
    }
    else
    {
        /* End is aligned.
         */
        b_aligned_end = TRUE;
    }
    MUT_ASSERT_TRUE( !b_aligned_start || !b_aligned_end );

    /* Call handle read with our io packet.
     */
    status = fbe_ldo_handle_read(logical_drive_p, io_packet_p);

    /* Make sure the status was set as expected.
     */
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* If the end is aligned, then we will not expect an error since it is OK 
     * to have extra data in the sg. 
     * It is only not OK to have extra data in the SG in the case where we need 
     * to insert a "post sg". 
     * If padding is needed we also don't expect to have extra data in the sg 
     * since a post sg is needed here also. 
     */
    if (b_aligned_end && !b_padding_required)
    {
        /* We should have only called send packet once.
         */
        MUT_ASSERT_INT_EQUAL_MSG(send_mock_p->packet_sent_count, 1,
                                 "LDO: handle_read send packet mock not called");

        /* Make sure the io cmd mock was not called.
         */
        fbe_ldo_alloc_io_cmd_mock_validate_not_called(alloc_mock_p);
    }
    else
    {
        /* The end is not aligned, thus we expect to get an error and a packet
         * should not be set. 
         */
        MUT_ASSERT_INT_EQUAL_MSG(send_mock_p->packet_sent_count, 0,
                                 "LDO: handle_read send packet mock not called");

        /* Make sure that the allocate io cmd
         * mock was called. 
         */
        fbe_ldo_alloc_io_cmd_mock_validate(alloc_mock_p);
    }
    return status;  
}
/* end fbe_ldo_test_handle_read_remainder_case() */

/*!***************************************************************
 * @fn fbe_ldo_test_read_state0_aligned(
 *                      fbe_ldo_test_state_t * const test_state_p)
 ****************************************************************
 * @brief
 *  This function tests read state machine state0 when the command is aligned.
 *
 * @param test_state_p - The test state with info about the configuration.
 *
 * @return fbe_status_t - FBE_STATUS_OK - no error.
 *                         Other values imply there was an error.
 *
 * HISTORY:
 *  11/20/07 - Created. RPF
 *
 ****************************************************************/
fbe_status_t fbe_ldo_test_read_state0_aligned(fbe_ldo_test_state_t * const test_state_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_ldo_state_status_enum_t state_status;
    fbe_logical_drive_io_cmd_t *io_cmd_p;
    fbe_packet_t *io_packet_p;
    fbe_logical_drive_t *logical_drive_p;

    logical_drive_p = fbe_ldo_test_state_get_logical_drive(test_state_p);
    io_packet_p = fbe_ldo_test_state_get_io_packet(test_state_p);
    io_cmd_p = fbe_ldo_test_state_get_io_cmd(test_state_p);

    MUT_ASSERT_TRUE_MSG(fbe_ldo_io_cmd_is_aligned(io_cmd_p),
                         "Only aligned cmds should call this function.");
    MUT_ASSERT_FALSE_MSG(fbe_ldo_io_cmd_is_padding_required(io_cmd_p),
                         "We should only call this function when padding is not needed.");

    MUT_ASSERT_TRUE_MSG( logical_drive_p != NULL, "Logical drive is NULL");
    MUT_ASSERT_TRUE_MSG( io_packet_p != NULL, "io packet is NULL");
    MUT_ASSERT_TRUE_MSG( io_cmd_p != NULL, "io cmd is NULL");

    /* Call state 10 with our io command.
     */
    state_status = fbe_ldo_io_cmd_read_state0(io_cmd_p);

    /* Make sure the status and state were set as expected.
     */
    MUT_ASSERT_TRUE_MSG(state_status == FBE_LDO_STATE_STATUS_EXECUTING,
                        "LDO: return status not executing");
    MUT_ASSERT_TRUE_MSG(fbe_ldo_io_cmd_get_state(io_cmd_p) == fbe_ldo_io_cmd_read_state20,
                        "LDO: io command state unexpected");

    /* The sg entries should not be modified.
     */
    MUT_ASSERT_TRUE_MSG(fbe_ldo_io_cmd_get_sg_entries(io_cmd_p) == 0,
                        "LDO: io command sg entries changed");
    return status;  
}
/* end fbe_ldo_test_read_state0_aligned() */

/*!***************************************************************
 * @fn fbe_ldo_test_read_state0_unaligned(
 *                      fbe_ldo_test_state_t * const test_state_p)
 ****************************************************************
 * @brief
 *  This function tests read state machine state0.
 *
 * @param test_state_p - The test state with info about the configuration.
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - no error.
 *                 Other values imply there was an error.
 *
 * HISTORY:
 *  08/05/08 - Created. RPF
 *
 ****************************************************************/
fbe_status_t fbe_ldo_test_read_state0_unaligned(fbe_ldo_test_state_t * const test_state_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_ldo_state_status_enum_t state_status;
    fbe_logical_drive_io_cmd_t *io_cmd_p;
    fbe_packet_t *io_packet_p;
    fbe_logical_drive_t *logical_drive_p;

    logical_drive_p = fbe_ldo_test_state_get_logical_drive(test_state_p);
    io_packet_p = fbe_ldo_test_state_get_io_packet(test_state_p);
    io_cmd_p = fbe_ldo_test_state_get_io_cmd(test_state_p);

    MUT_ASSERT_TRUE_MSG(fbe_ldo_io_cmd_is_sg_required(io_cmd_p) ||
                        !fbe_ldo_io_cmd_is_aligned(io_cmd_p),
                         "Cmds that need an sg should call this function.");

    MUT_ASSERT_TRUE_MSG( logical_drive_p != NULL, "Logical drive is NULL");
    MUT_ASSERT_TRUE_MSG( io_packet_p != NULL, "io packet is NULL");
    MUT_ASSERT_TRUE_MSG( io_cmd_p != NULL, "io cmd is NULL");
    
    /* Call state 10 with our io command.
     */
    state_status = fbe_ldo_io_cmd_read_state0(io_cmd_p);

    /* Make sure the status and state were set as expected.
     */
    MUT_ASSERT_TRUE_MSG(state_status == FBE_LDO_STATE_STATUS_EXECUTING,
                        "LDO: return status not waiting");

    /* We always goto state 10.
     */
    MUT_ASSERT_TRUE_MSG(fbe_ldo_io_cmd_get_state(io_cmd_p) == fbe_ldo_io_cmd_read_state10,
                        "LDO: io command state unexpected.  Expected read state 10");

    return status;  
}
/* end fbe_ldo_test_read_state0_unaligned() */

/*!***************************************************************
 * @fn fbe_ldo_test_read_state0_unaligned_error(
 *                      fbe_ldo_test_state_t * const test_state_p)
 ****************************************************************
 * @brief
 *  This function tests read state machine state0.
 *
 * @param test_state_p - The test state with info about the configuration.
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - no error.
 *                 Other values imply there was an error.
 *
 * HISTORY:
 *  08/05/08 - Created. RPF
 *
 ****************************************************************/
fbe_status_t fbe_ldo_test_read_state0_unaligned_error(fbe_ldo_test_state_t * const test_state_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_ldo_state_status_enum_t state_status;
    fbe_logical_drive_io_cmd_t *io_cmd_p;
    fbe_packet_t *io_packet_p;
    fbe_logical_drive_t *logical_drive_p;

    logical_drive_p = fbe_ldo_test_state_get_logical_drive(test_state_p);
    io_packet_p = fbe_ldo_test_state_get_io_packet(test_state_p);
    io_cmd_p = fbe_ldo_test_state_get_io_cmd(test_state_p);

    MUT_ASSERT_TRUE_MSG(fbe_ldo_io_cmd_is_sg_required(io_cmd_p) ||
                        !fbe_ldo_io_cmd_is_aligned(io_cmd_p),
                         "Cmds that need an sg should call this function.");

    MUT_ASSERT_TRUE_MSG( logical_drive_p != NULL, "Logical drive is NULL");
    MUT_ASSERT_TRUE_MSG( io_packet_p != NULL, "io packet is NULL");
    MUT_ASSERT_TRUE_MSG( io_cmd_p != NULL, "io cmd is NULL");
    
    /* Call state 10 with our io command.
     */
    state_status = fbe_ldo_io_cmd_read_state0(io_cmd_p);

    /* We expect an error, which causes us to transition to state 11
     * immediately. 
     */
    MUT_ASSERT_TRUE_MSG(state_status == FBE_LDO_STATE_STATUS_EXECUTING,
                        "LDO: return status not waiting");
    MUT_ASSERT_TRUE_MSG(fbe_ldo_io_cmd_get_state(io_cmd_p) == fbe_ldo_io_cmd_read_state10,
                        "LDO: io command state unexpected");
    return status;  
}
/* end fbe_ldo_test_read_state0_unaligned_error() */

/*!***************************************************************
 * @fn fbe_ldo_test_read_state10_unaligned(
 *                      fbe_ldo_test_state_t * const test_state_p)
 ****************************************************************
 * @brief
 *  This function tests read state machine state10.
 *
 * @param test_state_p - The test state with info about the configuration.
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - no error.
 *                 Other values imply there was an error.
 *
 * HISTORY:
 *  11/20/07 - Created. RPF
 *
 ****************************************************************/
fbe_status_t fbe_ldo_test_read_state10_unaligned(fbe_ldo_test_state_t * const test_state_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_ldo_state_status_enum_t state_status;
    fbe_logical_drive_io_cmd_t *io_cmd_p;
    fbe_packet_t *io_packet_p;
    fbe_logical_drive_t *logical_drive_p;

    logical_drive_p = fbe_ldo_test_state_get_logical_drive(test_state_p);
    io_packet_p = fbe_ldo_test_state_get_io_packet(test_state_p);
    io_cmd_p = fbe_ldo_test_state_get_io_cmd(test_state_p);

    MUT_ASSERT_TRUE_MSG(fbe_ldo_io_cmd_is_sg_required(io_cmd_p) ||
                        !fbe_ldo_io_cmd_is_aligned(io_cmd_p),
                         "Cmds that need an sg should call this function.");

    MUT_ASSERT_TRUE_MSG( logical_drive_p != NULL, "Logical drive is NULL");
    MUT_ASSERT_TRUE_MSG( io_packet_p != NULL, "io packet is NULL");
    MUT_ASSERT_TRUE_MSG( io_cmd_p != NULL, "io cmd is NULL");
    
    /* Call state 10 with our io command.
     */
    state_status = fbe_ldo_io_cmd_read_state10(io_cmd_p);

    /* Make sure the status and state were set as expected.
     */
    MUT_ASSERT_TRUE_MSG(state_status == FBE_LDO_STATE_STATUS_EXECUTING,
                        "LDO: return status not waiting");
    MUT_ASSERT_TRUE_MSG(fbe_ldo_io_cmd_get_state(io_cmd_p) == fbe_ldo_io_cmd_read_state20,
                        "LDO: io command state unexpected");

    /* The sg entries should be modified in this path.
     * We assume the read is unaligned and has at least one edge.
     * We should have at least 3 sgs, one for the edge, one for the 
     * client data and one for the terminator.
     */
    MUT_ASSERT_TRUE_MSG(fbe_ldo_io_cmd_get_sg_entries(io_cmd_p) > 1,
                        "LDO: io command sg entries not set");

    return status;  
}
/* end fbe_ldo_test_read_state10_unaligned() */

/*!***************************************************************
 * @fn fbe_ldo_test_read_state10_unaligned_sg_error(
 *                      fbe_ldo_test_state_t * const test_state_p)
 ****************************************************************
 * @brief
 *  This function tests read state machine state10.
 *
 * @param test_state_p - The test state with info about the configuration.
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - no error.
 *                 Other values imply there was an error.
 *
 * HISTORY:
 *  11/20/07 - Created. RPF
 *
 ****************************************************************/
fbe_status_t fbe_ldo_test_read_state10_unaligned_sg_error(fbe_ldo_test_state_t * const test_state_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_ldo_state_status_enum_t state_status;
    fbe_logical_drive_io_cmd_t *io_cmd_p;
    fbe_packet_t *io_packet_p;
    fbe_logical_drive_t *logical_drive_p;
    fbe_payload_ex_t *payload_p;

    logical_drive_p = fbe_ldo_test_state_get_logical_drive(test_state_p);
    io_packet_p = fbe_ldo_test_state_get_io_packet(test_state_p);
    io_cmd_p = fbe_ldo_test_state_get_io_cmd(test_state_p);

    MUT_ASSERT_TRUE_MSG(fbe_ldo_io_cmd_is_sg_required(io_cmd_p) ||
                        !fbe_ldo_io_cmd_is_aligned(io_cmd_p),
                         "Cmds that need an sg should call this function.");

    MUT_ASSERT_TRUE_MSG( logical_drive_p != NULL, "Logical drive is NULL");
    MUT_ASSERT_TRUE_MSG( io_packet_p != NULL, "io packet is NULL");
    MUT_ASSERT_TRUE_MSG( io_cmd_p != NULL, "io cmd is NULL");

    /* Free up the io packet's sg ptr, since this state does not need the sg set.
     */
    payload_p = fbe_transport_get_payload_ex(fbe_ldo_test_state_get_io_packet(test_state_p));
    fbe_payload_ex_set_sg_list(payload_p, NULL, 0);

    /* Make sure the client packet status is bad.
     */
    fbe_transport_set_status(fbe_ldo_io_cmd_get_packet(io_cmd_p), FBE_STATUS_INVALID, 0);
    MUT_ASSERT_TRUE(fbe_transport_get_status_code(fbe_ldo_io_cmd_get_packet(io_cmd_p)) == FBE_STATUS_INVALID);
    MUT_ASSERT_TRUE(fbe_transport_get_status_qualifier(fbe_ldo_io_cmd_get_packet(io_cmd_p)) == 0);
    fbe_ldo_set_payload_status(fbe_ldo_io_cmd_get_packet(io_cmd_p), 
                               FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID,
                               FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_INVALID);
    
    /* Call state 10 with our io command.
     */
    state_status = fbe_ldo_io_cmd_read_state10(io_cmd_p);

    /* Make sure the status and state were set as expected.
     */
    MUT_ASSERT_TRUE_MSG(state_status == FBE_LDO_STATE_STATUS_EXECUTING,
                        "LDO: return status not executing");
    MUT_ASSERT_TRUE_MSG(fbe_ldo_io_cmd_get_state(io_cmd_p) == fbe_ldo_io_cmd_finished,
                        "LDO: io command state unexpected");

    /* Make sure the client packet status and qualifier is set to to indicate
     * the error we encountered here.
     */
    MUT_ASSERT_INT_EQUAL_MSG(fbe_transport_get_status_code(fbe_ldo_io_cmd_get_packet(io_cmd_p)),
                             FBE_STATUS_GENERIC_FAILURE,
                             "LDO: Client packet's status not set from sub packet");
    MUT_ASSERT_INT_EQUAL_MSG(fbe_transport_get_status_qualifier(fbe_ldo_io_cmd_get_packet(io_cmd_p)),
                             0,
                             "LDO: Client packet's qualifier not set from sub packet");
    MUT_ASSERT_INT_EQUAL_MSG(fbe_ldo_get_payload_status(fbe_ldo_io_cmd_get_packet(io_cmd_p)),
                             FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID_REQUEST,
                             "LDO: Client packet's payload status not set from sub packet");
    MUT_ASSERT_INT_EQUAL_MSG(fbe_ldo_get_payload_qualifier(fbe_ldo_io_cmd_get_packet(io_cmd_p)),
                             FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_INVALID_SG,
                             "LDO: Client packet's qualifier not set from sub packet");

    /* The sg entries should not be modified in this path.
     */
    MUT_ASSERT_TRUE_MSG(fbe_ldo_io_cmd_get_sg_entries(io_cmd_p) == 0,
                        "LDO: io command sg entries changed");
    
    return status;  
}
/* end fbe_ldo_test_read_state10_unaligned_sg_error() */

/*!***************************************************************
 * @fn fbe_ldo_test_read_state20(fbe_ldo_test_state_t * const test_state_p)
 ****************************************************************
 * @brief
 *  This function tests read state machine state20.
 *
 * @param test_state_p - The test state with info about the configuration.
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - no error.
 *                 Other values imply there was an error.
 *
 * HISTORY:
 *  11/20/07 - Created. RPF
 *
 ****************************************************************/
fbe_status_t fbe_ldo_test_read_state20( fbe_ldo_test_state_t * const test_state_p )
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_ldo_state_status_enum_t state_status;
    fbe_logical_drive_io_cmd_t *io_cmd_p;
    fbe_packet_t *io_packet_p;
    fbe_logical_drive_t *logical_drive_p;
    fbe_ldo_test_send_io_packet_mock_t *mock_p;
    fbe_u32_t expected_sg_count;

    logical_drive_p = fbe_ldo_test_state_get_logical_drive(test_state_p);
    io_packet_p = fbe_ldo_test_state_get_io_packet(test_state_p);
    io_cmd_p = fbe_ldo_test_state_get_io_cmd(test_state_p);

    MUT_ASSERT_TRUE_MSG( logical_drive_p != NULL, "Logical drive is NULL" );
    MUT_ASSERT_TRUE_MSG( io_packet_p != NULL, "io packet is NULL" );
    MUT_ASSERT_TRUE_MSG( io_cmd_p != NULL, "io cmd is NULL" );
    
    /* Setup our mock io packet send function.
     */
    fbe_ldo_set_send_io_packet_fn(fbe_ldo_send_io_packet_mock);
    mock_p = fbe_ldo_test_get_send_packet_mock();
    fbe_ldo_send_io_packet_mock_init(mock_p, io_cmd_p, FBE_STATUS_OK, 0);

    /* If we are aligned we need to setup the io cmd differently.
     */
    if (fbe_ldo_io_cmd_is_sg_required(io_cmd_p))
    {
        /* Set the sg entries as if we allocated them already.
         */
        fbe_ldo_io_cmd_set_sg_entries(io_cmd_p, 
                                      fbe_ldo_io_cmd_rd_count_sg_entries(io_cmd_p));

        MUT_ASSERT_TRUE_MSG(fbe_ldo_io_cmd_get_sg_entries(io_cmd_p) > 0,
                            "LDO: read state20 sg entries not set");
    }
    /* Get the expected number of sgs. We will compare to this later.
     */
    expected_sg_count = fbe_ldo_io_cmd_rd_count_sg_entries(io_cmd_p);

    /* Execute state 20.
     */
    state_status = fbe_ldo_io_cmd_read_state20(io_cmd_p);

    /* Make sure the status and state were set as expected.
     */
    MUT_ASSERT_INT_EQUAL_MSG(state_status, FBE_LDO_STATE_STATUS_WAITING,
                             "LDO: read state20 return status unexpected");
    MUT_ASSERT_TRUE_MSG(fbe_ldo_io_cmd_get_state(io_cmd_p) == fbe_ldo_io_cmd_read_state30,
                        "read state20 io command state unexpected");

    /* We should have only called send packet once.
     */
    MUT_ASSERT_INT_EQUAL_MSG(mock_p->packet_sent_count, 1,
                             "LDO: read state20 send packet mock not called");
    /* Make sure we have the right number of bytes in this sg.
     */
    MUT_ASSERT_INT_EQUAL_MSG(mock_p->sg_list_bytes, mock_p->io_packet_bytes,
                             "LDO: read state20 sg list bytes incorrect");
    
    /* If there are not sufficient bytes in the sg list, we will get back a 
     * zero for sg entries.
     */
    MUT_ASSERT_INT_NOT_EQUAL_MSG(mock_p->sg_entries, 0, "io packet sg is incorrect");

    /* Make sure we have the right number of sg entries in this sg.  We add one for the null terminator.
     * We can validate this using fbe_ldo_rd_count_sg_entries() since it is used
     * to calculate how many sgs to allocate.
     */
    MUT_ASSERT_INT_EQUAL_MSG(mock_p->sg_entries + 1, expected_sg_count,
                             "LDO: read state20 sg entries incorrect");

    /* Make sure the state was set to state 30 before we sent out the io packet.
     */
    MUT_ASSERT_TRUE_MSG(mock_p->io_cmd_state == fbe_ldo_io_cmd_read_state30,
                        "read state20 did not transition to next state before sending io packet");

    /* Make sure the sub io packet has a write same command.
     */
    MUT_ASSERT_INT_EQUAL_MSG(ldo_packet_get_block_cmd_opcode(io_packet_p), 
                             FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ, 
                             "read state 20 opcode not read");
    return status;
}
/* end fbe_ldo_test_read_state20() */

/*!***************************************************************
 * @fn fbe_ldo_test_read_state20_sg_error(
 *                        fbe_ldo_test_state_t * const test_state_p)
 ****************************************************************
 * @brief
 *  This function tests the error case of read state machine state20.
 *
 * @param test_state_p - The test state with info about the configuration.
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - no error.
 *                 Other values imply there was an error.
 *
 * HISTORY:
 *  11/20/07 - Created. RPF
 *
 ****************************************************************/
fbe_status_t fbe_ldo_test_read_state20_sg_error( fbe_ldo_test_state_t * const test_state_p )
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_ldo_state_status_enum_t state_status;
    fbe_logical_drive_io_cmd_t *io_cmd_p;
    fbe_packet_t *io_packet_p;
    fbe_logical_drive_t *logical_drive_p;
    fbe_ldo_test_send_io_packet_mock_t *mock_p;

    logical_drive_p = fbe_ldo_test_state_get_logical_drive(test_state_p);
    io_packet_p = fbe_ldo_test_state_get_io_packet(test_state_p);
    io_cmd_p = fbe_ldo_test_state_get_io_cmd(test_state_p);

    MUT_ASSERT_TRUE_MSG(fbe_ldo_io_cmd_is_sg_required(io_cmd_p),
                        "Only unaligned cmds should call this function.");

    MUT_ASSERT_TRUE_MSG( logical_drive_p != NULL, "Logical drive is NULL" );
    MUT_ASSERT_TRUE_MSG( io_packet_p != NULL, "io packet is NULL" );
    MUT_ASSERT_TRUE_MSG( io_cmd_p != NULL, "io cmd is NULL" );

    /* Setup our packet status to known values.
     */
    fbe_transport_set_status(fbe_ldo_io_cmd_get_packet(io_cmd_p), FBE_STATUS_INVALID, 0);
    MUT_ASSERT_TRUE(fbe_transport_get_status_code(fbe_ldo_io_cmd_get_packet(io_cmd_p)) == FBE_STATUS_INVALID);
    MUT_ASSERT_TRUE(fbe_transport_get_status_qualifier(fbe_ldo_io_cmd_get_packet(io_cmd_p)) == 0);
    
    /* Setup our mock io packet send function.
     */
    fbe_ldo_set_send_io_packet_fn(fbe_ldo_send_io_packet_mock);
    mock_p = fbe_ldo_test_get_send_packet_mock();
    fbe_ldo_send_io_packet_mock_init(mock_p, io_cmd_p, FBE_STATUS_OK, 0);

    /* Set the sg entries as if we allocated them already.
     */
    fbe_ldo_io_cmd_set_sg_entries(io_cmd_p, 
                                  fbe_ldo_io_cmd_rd_count_sg_entries(io_cmd_p));

    MUT_ASSERT_TRUE_MSG(fbe_ldo_io_cmd_get_sg_entries(io_cmd_p) > 0,
                        "LDO: read state20 sg entries not set");

    /* Change the number of sg entries in the io cmd to force an error 
     * in setting up the sgs. 
     * In this case we decrement it by one. 
     */
    fbe_ldo_io_cmd_set_sg_entries(io_cmd_p,
                                  fbe_ldo_io_cmd_get_sg_entries(io_cmd_p) - 1);

    /* Execute state 20.
     */
    state_status = fbe_ldo_io_cmd_read_state20(io_cmd_p);

    /* Make sure the status and state were set as expected on error.
     */
    MUT_ASSERT_INT_EQUAL_MSG(state_status, FBE_LDO_STATE_STATUS_EXECUTING,
                             "LDO: read state20 return status unexpected");
    MUT_ASSERT_TRUE_MSG(fbe_ldo_io_cmd_get_state(io_cmd_p) == fbe_ldo_io_cmd_finished,
                        "read state20 io command state unexpected on error");

    /* Make sure the client packet status and qualifier are set to indicate the
     * correct error. 
     */
    MUT_ASSERT_INT_EQUAL_MSG(fbe_transport_get_status_code(fbe_ldo_io_cmd_get_packet(io_cmd_p)),
                             FBE_STATUS_GENERIC_FAILURE,
                             "LDO: Client packet's status unexpected");
    MUT_ASSERT_INT_EQUAL_MSG(fbe_transport_get_status_qualifier(fbe_ldo_io_cmd_get_packet(io_cmd_p)),
                             0,
                             "LDO: Client packet's qualifier unexpected");
    MUT_ASSERT_INT_EQUAL_MSG(fbe_ldo_get_payload_status(fbe_ldo_io_cmd_get_packet(io_cmd_p)),
                             FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID_REQUEST,
                             "LDO: Client packet's payload status unexpected");
    MUT_ASSERT_INT_EQUAL_MSG(fbe_ldo_get_payload_qualifier(fbe_ldo_io_cmd_get_packet(io_cmd_p)),
                             FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_UNEXPECTED_ERROR,
                             "LDO: Client packet's qualifier value unexpected");

    /* We should not have called send packet.
     */
    MUT_ASSERT_INT_EQUAL_MSG(mock_p->packet_sent_count, 0,
                             "LDO: read state20 send packet mock called unexpectedly");

    return status;
}
/* end fbe_ldo_test_read_state20_sg_error() */

/*!***************************************************************
 * @fn fbe_ldo_test_read_state30(
 *                      fbe_ldo_test_state_t * const test_state_p)
 ****************************************************************
 * @brief
 *  This function tests read state machine state30.
 *
 * @param test_state_p - The test state with info about the configuration.
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - no error.
 *                 Other values imply there was an error.
 *
 * HISTORY:
 *  08/07/08 - Created. RPF
 *
 ****************************************************************/
fbe_status_t fbe_ldo_test_read_state30( fbe_ldo_test_state_t * const test_state_p )
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_ldo_state_status_enum_t state_status;
    fbe_logical_drive_io_cmd_t *io_cmd_p;
    fbe_packet_t *io_packet_p;
    fbe_logical_drive_t *logical_drive_p;

    logical_drive_p = fbe_ldo_test_state_get_logical_drive(test_state_p);
    io_packet_p = fbe_ldo_test_state_get_io_packet(test_state_p);
    io_cmd_p = fbe_ldo_test_state_get_io_cmd(test_state_p);

    MUT_ASSERT_TRUE_MSG( logical_drive_p != NULL, "Logical drive is NULL" );
    MUT_ASSERT_TRUE_MSG( io_packet_p != NULL, "io packet is NULL" );
    MUT_ASSERT_TRUE_MSG( io_cmd_p != NULL, "io cmd is NULL" );

    /* Make sure the client packet status is bad.
     */
    fbe_transport_set_status(io_packet_p, FBE_STATUS_INVALID, 0);
    MUT_ASSERT_TRUE(fbe_transport_get_status_code(io_packet_p) == FBE_STATUS_INVALID);
    MUT_ASSERT_TRUE(fbe_transport_get_status_qualifier(io_packet_p) == 0);
    fbe_ldo_set_payload_status(io_packet_p, 
                               FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID,
                               FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_INVALID);

    /* Increment stack level since when a packet has been sent this was already
     * incremented. 
     */
    fbe_ldo_test_allocate_and_increment_stack_level(io_packet_p);
    fbe_transport_set_status(io_packet_p, FBE_STATUS_OK, 0x12345678);
    fbe_ldo_set_payload_status(io_packet_p, 
                               FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS,
                               FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE);

    /* Execute state 30.
     */
    state_status = fbe_ldo_io_cmd_read_state30(io_cmd_p);

    /* Make sure the client packet status and qualifier is set to the sub packet
     * status and qualifier.
     */
    MUT_ASSERT_INT_EQUAL_MSG(fbe_transport_get_status_code(io_packet_p),
                             FBE_STATUS_OK,
                             "LDO: Client packet's status not set from sub packet");
    MUT_ASSERT_INT_EQUAL_MSG(fbe_transport_get_status_qualifier(io_packet_p),
                             0x12345678,
                             "LDO: Client packet's qualifier not set from sub packet");
    MUT_ASSERT_INT_EQUAL_MSG(fbe_ldo_get_payload_status(io_packet_p),
                             FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS,
                             "LDO: Client packet's payload status not set from sub packet");
    MUT_ASSERT_INT_EQUAL_MSG(fbe_ldo_get_payload_qualifier(io_packet_p),
                             FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE,
                             "LDO: Client packet's qualifier not set from sub packet");

    /* Make sure the status and state were set as expected.
     */
    MUT_ASSERT_INT_EQUAL_MSG(state_status, FBE_LDO_STATE_STATUS_EXECUTING,
                             "LDO: read state30 return status unexpected");
    MUT_ASSERT_TRUE_MSG(fbe_ldo_io_cmd_get_state(io_cmd_p) == fbe_ldo_io_cmd_finished,
                        "read state30 io command state unexpected");

    return status;
}
/* end fbe_ldo_test_read_state30() */

/*!***************************************************************
 * @fn fbe_ldo_test_read_state30_payload_error(
 *                      fbe_ldo_test_state_t * const test_state_p)
 ****************************************************************
 * @brief
 *  This function tests read state machine state30 case
 *  where an error in the payload has been encountered.
 *
 * @param test_state_p - The test state with info about the configuration.
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - no error.
 *                 Other values imply there was an error.
 *
 * HISTORY:
 *  08/07/08 - Created. RPF
 *
 ****************************************************************/
fbe_status_t 
fbe_ldo_test_read_state30_payload_error( fbe_ldo_test_state_t * const test_state_p )
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_ldo_state_status_enum_t state_status;
    fbe_logical_drive_io_cmd_t *io_cmd_p;
    fbe_packet_t *io_packet_p;
    fbe_logical_drive_t *logical_drive_p;

    logical_drive_p = fbe_ldo_test_state_get_logical_drive(test_state_p);
    io_packet_p = fbe_ldo_test_state_get_io_packet(test_state_p);
    io_cmd_p = fbe_ldo_test_state_get_io_cmd(test_state_p);

    MUT_ASSERT_TRUE_MSG( logical_drive_p != NULL, "Logical drive is NULL" );
    MUT_ASSERT_TRUE_MSG( io_packet_p != NULL, "io packet is NULL" );
    MUT_ASSERT_TRUE_MSG( io_cmd_p != NULL, "io cmd is NULL" );

    /* Make sure the client packet status is bad.
     */
    fbe_transport_set_status(io_packet_p, FBE_STATUS_INVALID, 0);
    MUT_ASSERT_TRUE(fbe_transport_get_status_code(io_packet_p) == FBE_STATUS_INVALID);
    MUT_ASSERT_TRUE(fbe_transport_get_status_qualifier(io_packet_p) == 0);
    fbe_ldo_set_payload_status(io_packet_p, 
                               FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID,
                               FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_INVALID);

    /* Increment stack level since when a packet has been sent this was already
     * incremented. 
     */
    fbe_ldo_test_allocate_and_increment_stack_level(io_packet_p);
    fbe_transport_set_status(io_packet_p, FBE_STATUS_OK, 0);
    fbe_ldo_set_payload_status(io_packet_p, 
                               FBE_PAYLOAD_BLOCK_OPERATION_STATUS_MEDIA_ERROR,
                               FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_MEDIA_ERROR_DATA_LOST);

    /* Execute state 30.
     */
    state_status = fbe_ldo_io_cmd_read_state30(io_cmd_p);

    /* Make sure the client packet status and qualifier is set to the sub packet
     * status and qualifier. 
     * In this case since an error was injected in the payload, we expect the 
     * transport status to be OK and the payload status to be bad. 
     */
    MUT_ASSERT_INT_EQUAL_MSG(fbe_transport_get_status_code(io_packet_p),
                             FBE_STATUS_OK,
                             "LDO: Client packet's status not set from sub packet");
    MUT_ASSERT_INT_EQUAL_MSG(fbe_transport_get_status_qualifier(io_packet_p),
                             0,
                             "LDO: Client packet's qualifier not set from sub packet");
    MUT_ASSERT_INT_EQUAL_MSG(fbe_ldo_get_payload_status(io_packet_p),
                             FBE_PAYLOAD_BLOCK_OPERATION_STATUS_MEDIA_ERROR,
                             "LDO: Client packet's payload status not set from sub packet");
    MUT_ASSERT_INT_EQUAL_MSG(fbe_ldo_get_payload_qualifier(io_packet_p),
                             FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_MEDIA_ERROR_DATA_LOST,
                             "LDO: Client packet's qualifier not set from sub packet");

    /* Make sure the status and state were set as expected.
     */
    MUT_ASSERT_INT_EQUAL_MSG(state_status, FBE_LDO_STATE_STATUS_EXECUTING,
                             "LDO: read state30 return status unexpected");
    MUT_ASSERT_TRUE_MSG(fbe_ldo_io_cmd_get_state(io_cmd_p) == fbe_ldo_io_cmd_finished,
                        "read state30 io command state unexpected");

    return status;
}
/* end fbe_ldo_test_read_state30_payload_error() */

/*!***************************************************************
 * @fn fbe_ldo_test_read_state30_transport_error(
 *                      fbe_ldo_test_state_t * const test_state_p)
 ****************************************************************
 * @brief
 *  This function tests read state machine state30 case
 *  where an error in the transport has been encountered.
 *
 * @param test_state_p - The test state with info about the configuration.
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - no error.
 *                 Other values imply there was an error.
 *
 * HISTORY:
 *  08/07/08 - Created. RPF
 *
 ****************************************************************/
fbe_status_t 
fbe_ldo_test_read_state30_transport_error( fbe_ldo_test_state_t * const test_state_p )
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_ldo_state_status_enum_t state_status;
    fbe_logical_drive_io_cmd_t *io_cmd_p;
    fbe_packet_t *io_packet_p;
    fbe_logical_drive_t *logical_drive_p;

    logical_drive_p = fbe_ldo_test_state_get_logical_drive(test_state_p);
    io_packet_p = fbe_ldo_test_state_get_io_packet(test_state_p);
    io_cmd_p = fbe_ldo_test_state_get_io_cmd(test_state_p);

    MUT_ASSERT_TRUE_MSG( logical_drive_p != NULL, "Logical drive is NULL" );
    MUT_ASSERT_TRUE_MSG( io_packet_p != NULL, "io packet is NULL" );
    MUT_ASSERT_TRUE_MSG( io_cmd_p != NULL, "io cmd is NULL" );

    /* Make sure the client packet status is bad.
     */
    fbe_transport_set_status(io_packet_p, FBE_STATUS_INVALID, 0);
    MUT_ASSERT_TRUE(fbe_transport_get_status_code(io_packet_p) == FBE_STATUS_INVALID);
    MUT_ASSERT_TRUE(fbe_transport_get_status_qualifier(io_packet_p) == 0);
    fbe_ldo_set_payload_status(io_packet_p, 
                               FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID,
                               FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_INVALID);

    /* Increment stack level since when a packet has been sent this was already
     * incremented. 
     */
    fbe_ldo_test_allocate_and_increment_stack_level(io_packet_p);
    fbe_transport_set_status(io_packet_p, FBE_STATUS_INSUFFICIENT_RESOURCES, 0x12345678);
    fbe_ldo_set_payload_status(io_packet_p, 
                               FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS,
                               FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE);

    /* Execute state 30.
     */
    state_status = fbe_ldo_io_cmd_read_state30(io_cmd_p);

    /* Make sure the client packet status and qualifier is set to the sub packet
     * status and qualifier
     */
    MUT_ASSERT_INT_EQUAL_MSG(fbe_transport_get_status_code(io_packet_p),
                             FBE_STATUS_INSUFFICIENT_RESOURCES,
                             "LDO: Client packet's status not set from sub packet");
    MUT_ASSERT_INT_EQUAL_MSG(fbe_transport_get_status_qualifier(io_packet_p),
                             0x12345678,
                             "LDO: Client packet's qualifier not set from sub packet");
    MUT_ASSERT_INT_EQUAL_MSG(fbe_ldo_get_payload_status(io_packet_p),
                             FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS,
                             "LDO: Client packet's payload status not set from sub packet");
    MUT_ASSERT_INT_EQUAL_MSG(fbe_ldo_get_payload_qualifier(io_packet_p),
                             FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE,
                             "LDO: Client packet's qualifier not set from sub packet");

    /* Make sure the status and state were set as expected.
     */
    MUT_ASSERT_INT_EQUAL_MSG(state_status, FBE_LDO_STATE_STATUS_EXECUTING,
                             "LDO: read state30 return status unexpected");
    MUT_ASSERT_TRUE_MSG(fbe_ldo_io_cmd_get_state(io_cmd_p) == fbe_ldo_io_cmd_finished,
                        "read state30 io command state unexpected");

    return status;
}
/* end fbe_ldo_test_read_state30_transport_error() */

/*!***************************************************************
 * @fn fbe_logical_drive_read_state_machine_test_case(
 *   fbe_ldo_test_state_machine_task_t *const task_p)
 ****************************************************************
 * @brief
 *  This function executes tests of the read state machine.
 *
 * @param task_p - The test task to execute for the read state machine.
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
fbe_logical_drive_read_state_machine_test_case(fbe_ldo_test_state_machine_task_t *const task_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_bool_t b_is_sg_required;
    fbe_bool_t b_is_aligned;

    /* Get the read state object. 
     */
    fbe_ldo_test_state_t * const test_state_p = fbe_ldo_test_get_state_object();
    fbe_logical_drive_io_cmd_t *const io_cmd_p = fbe_ldo_test_state_get_io_cmd(test_state_p);

    /* Make sure we can setup a read and destroy it.
     */
    status = fbe_ldo_test_state_setup_read(test_state_p, task_p);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    b_is_sg_required = fbe_ldo_io_cmd_is_sg_required(io_cmd_p);
    b_is_aligned = fbe_ldo_io_cmd_is_aligned(io_cmd_p);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(fbe_ldo_test_state_destroy(test_state_p) == FBE_STATUS_OK);

    if (fbe_ldo_io_cmd_is_aligned(io_cmd_p) &&
        fbe_ldo_io_cmd_is_padding_required(io_cmd_p) == FALSE)
    {
        /* Perform the aligned (no sg tests).
         */
        fbe_ldo_test_execute_state_machine(test_state_p,
                                           fbe_ldo_test_state_setup_read,
                                           fbe_ldo_test_read_state0_aligned,
                                           task_p);
    }
    else if (fbe_ldo_io_cmd_is_aligned(io_cmd_p) == FALSE)
    { 
        /* Perform the unaligned state 0 cases.
         */
        fbe_ldo_test_execute_state_machine(test_state_p,
                                           fbe_ldo_test_state_setup_read,
                                           fbe_ldo_test_read_state0_unaligned,
                                           task_p);
        fbe_ldo_test_execute_state_machine(test_state_p,
                                           fbe_ldo_test_state_setup_read,
                                           fbe_ldo_test_read_state0_unaligned_error,
                                           task_p);

        /* Perform the unaligned state 10 cases.
         */
        fbe_ldo_test_execute_state_machine(test_state_p,
                                           fbe_ldo_test_state_setup_read,
                                           fbe_ldo_test_read_state10_unaligned,
                                           task_p);
        fbe_ldo_test_execute_state_machine(test_state_p,
                                           fbe_ldo_test_state_setup_read,
                                           fbe_ldo_test_read_state10_unaligned_sg_error,
                                           task_p);
    } /* End not aligned */

    /************************************************** 
     *  State 20 test, normal case, no error.
     **************************************************/
    fbe_ldo_test_execute_state_machine(test_state_p,
                                       fbe_ldo_test_state_setup_read,
                                       fbe_ldo_test_read_state20,
                                       task_p);

    /************************************************** 
     *  State 20 test, setup sg error case
     *  This case only exists when we have an unaligned
     *  read and thus need to setup the sg.
     **************************************************/
    if (b_is_sg_required)
    {
        fbe_ldo_test_execute_state_machine(test_state_p,
                                           fbe_ldo_test_state_setup_read,
                                           fbe_ldo_test_read_state20_sg_error,
                                           task_p);
    }

    /************************************************** 
     *  State 30 tests.
     **************************************************/
    fbe_ldo_test_execute_state_machine(test_state_p,
                                       fbe_ldo_test_state_setup_read,
                                       fbe_ldo_test_read_state30,
                                       task_p);
    fbe_ldo_test_execute_state_machine(test_state_p,
                                       fbe_ldo_test_state_setup_read,
                                       fbe_ldo_test_read_state30_payload_error,
                                       task_p);
    fbe_ldo_test_execute_state_machine(test_state_p,
                                       fbe_ldo_test_state_setup_read,
                                       fbe_ldo_test_read_state30_transport_error,
                                       task_p);

    /************************************************** 
     * Handle inline cases. 
     *  
     * Only use this for cases where a resource is not needed.
     **************************************************/
    if (b_is_sg_required == FALSE )
    {
        fbe_ldo_test_execute_state_machine(test_state_p,
                                           fbe_ldo_test_state_setup_read,
                                           fbe_ldo_test_handle_read,
                                           task_p);
    }
    if ( b_is_aligned == FALSE )
    {
        fbe_ldo_test_execute_state_machine(test_state_p,
                                           fbe_ldo_test_state_setup_read,
                                           fbe_ldo_test_handle_read_remainder_case,
                                           task_p);
    }
    return status;
}
/* end fbe_logical_drive_read_state_machine_test_case() */

/*!***************************************************************
 * @fn fbe_ldo_test_read_count_setup_sgs(
 *                   fbe_ldo_test_state_t * const test_state_p,
 *                   fbe_ldo_test_state_machine_task_t *const task_p)
 ****************************************************************
 * @brief
 *  This function executes tests of the read state machine.
 *
 * @param test_state_p - The test state with info about the configuration.
 * @param task_p - The test task to use for testing the read sg fns.
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
fbe_ldo_test_read_count_setup_sgs( fbe_ldo_test_state_t * const test_state_p,
                                   fbe_ldo_test_state_machine_task_t *const task_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_logical_drive_io_cmd_t *io_cmd_p;
    fbe_u32_t sg_entries;
    
    status = fbe_ldo_test_state_setup_read(test_state_p, task_p);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    io_cmd_p = fbe_ldo_test_state_get_io_cmd(test_state_p);

    /* If we are not aligned, validate the count/setup sgs.
     */
    if (fbe_ldo_io_cmd_is_sg_required(io_cmd_p))
    {
        /* Determine how many sg entries are needed.
         */
        sg_entries = fbe_ldo_io_cmd_rd_count_sg_entries(io_cmd_p);
    
        MUT_ASSERT_TRUE(sg_entries > 0);

        /* Save this value in the io cmd for later use.
         */
        fbe_ldo_io_cmd_set_sg_entries(io_cmd_p, sg_entries);

        /* Setup the sg list.
         * If something goes wrong it will return an error.
         */
        status = fbe_ldo_io_cmd_read_setup_sg(io_cmd_p);

        /* Check that the setup succeeded.  Also validate our check words to make sure
         * we did not exceed an sg list.
         */
        MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
        MUT_ASSERT_TRUE(fbe_ldo_test_state_validate_check_words(test_state_p) == FBE_STATUS_OK);
        
        /* Check the number of bytes in the sg list.
         */
        MUT_ASSERT_TRUE( fbe_ldo_test_io_cmd_sg(io_cmd_p) == FBE_STATUS_OK );
        
    } /* End if not aligned. */

    /* Destroy the state object and assert that all goes well.
     */
    MUT_ASSERT_TRUE(fbe_ldo_test_state_destroy(test_state_p) == FBE_STATUS_OK);

    return status;
}
/* end fbe_ldo_test_read_count_setup_sgs() */

/*!***************************************************************
 * @fn fbe_ldo_test_read_count_setup_sg_cases(void)
 ****************************************************************
 * @brief
 *  This function executes tests of the read state machine.
 *
 * @param  - none
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - no error.
 *                 Other values imply there was an error.
 *
 * HISTORY:
 *  11/20/07 - Created. RPF
 *
 ****************************************************************/
fbe_status_t fbe_ldo_test_read_count_setup_sg_cases(void)
{
    fbe_status_t status = FBE_STATUS_OK;

    /* Simply call the test function with our task and
     * the read setup sgs function. 
     */
    fbe_ldo_test_sg_cases(fbe_ldo_test_sg_tasks,
                          fbe_ldo_test_read_count_setup_sgs);
    fbe_ldo_test_sg_cases(fbe_ldo_test_sg_lossy_tasks,
                          fbe_ldo_test_read_count_setup_sgs);

    return status;
}
/* end fbe_ldo_test_read_count_setup_sg_cases() */

/*!***************************************************************
 * @fn fbe_ldo_test_io_cmd_sg(fbe_logical_drive_io_cmd_t * const io_cmd_p)
 ****************************************************************
 * @brief
 *  This function validates the sg list of an io cmd.
 *
 * @param io_cmd_p - Io cmd to test the sg of for validity.
 *
 * @return fbe_status_t FBE_STATUS_OK.
 *
 * HISTORY:
 *  11/20/07 - Created. RPF
 *
 ****************************************************************/
fbe_status_t fbe_ldo_test_io_cmd_sg(fbe_logical_drive_io_cmd_t * const io_cmd_p)
{    
    fbe_lba_t unused_lba;
    fbe_block_count_t blocks;
    fbe_u32_t transfer_bytes;
    fbe_u32_t sg_entries;
    fbe_u32_t list_bytes;
    fbe_u32_t expected_sg_entries;
    fbe_logical_drive_t * const logical_drive_p = 
        fbe_ldo_io_cmd_get_logical_drive( io_cmd_p );
    fbe_block_size_t block_size = fbe_ldo_get_imported_block_size(logical_drive_p);

    /* Determine the mapped block count for the given io command.
     */
    fbe_ldo_io_cmd_map_lba_blocks( io_cmd_p, &unused_lba, &blocks );

    transfer_bytes = (fbe_u32_t) (blocks * block_size);
    
    sg_entries = fbe_sg_element_count_entries(fbe_ldo_io_cmd_get_sg_ptr(io_cmd_p));
    
    list_bytes = fbe_sg_element_count_list_bytes(
        fbe_ldo_io_cmd_get_sg_ptr(io_cmd_p));

    /* Make sure we have the right number of bytes in this sg for this transfer.
     */
    MUT_ASSERT_TRUE_MSG(list_bytes == transfer_bytes,
                        "LDO: sg list bytes != transfer_bytes");

    /* Make sure we have the right number of sg entries in this sg.  We add one for the null terminator.
     * We can validate this using fbe_ldo_rd_count_sg_entries() since it is used
     * to calculate how many sgs to allocate.
     */
    expected_sg_entries = fbe_ldo_io_cmd_get_sg_entries(io_cmd_p);
    MUT_ASSERT_TRUE_MSG(sg_entries + 1 == expected_sg_entries,
                        "LDO: io cmd sg list bytes incorrect");

    return FBE_STATUS_OK;
}
/* end fbe_ldo_test_io_cmd_sg() */

/*!**************************************************************
 * @fn fbe_ldo_test_read_setup_sg(
 *                   fbe_ldo_test_state_t * const test_state_p,
 *                   fbe_ldo_test_state_machine_task_t *const task_p)
 ****************************************************************
 * @brief
 *  Test error cases for fbe_ldo_io_cmd_read_setup_host_sg
 *
 * @param test_state_p - The test state with info about the configuration.
 * @param task_p - The test task to use for testing the read sg fns.
 *
 * @return -   
 *  fbe_status_t - FBE_STATUS_OK - no error.
 *                 Other values imply there was an error.
 *
 * HISTORY:
 *  11/07/2008 - Created. RPF
 *
 ****************************************************************/
fbe_status_t fbe_ldo_test_read_setup_sg(fbe_ldo_test_state_t * const test_state_p,
                                        fbe_ldo_test_state_machine_task_t *const task_p )
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_logical_drive_io_cmd_t *io_cmd_p = NULL;
    fbe_u32_t edge0_bytes = 0;
    fbe_u32_t edge1_bytes = 0;
    fbe_payload_ex_t * payload_p = NULL;
    fbe_packet_t *packet_p = NULL;
    fbe_logical_drive_t *logical_drive_p = NULL;
    fbe_u32_t max_edge_bytes;
    fbe_status_t e_status;
    
    status = fbe_ldo_test_state_setup_read(test_state_p, task_p);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    logical_drive_p = fbe_ldo_test_state_get_logical_drive(test_state_p);
    packet_p = fbe_ldo_test_state_get_io_packet(test_state_p);
    io_cmd_p = fbe_ldo_test_state_get_io_cmd(test_state_p);

    MUT_ASSERT_TRUE_MSG( logical_drive_p != NULL, "Logical drive is NULL");
    MUT_ASSERT_TRUE_MSG( packet_p != NULL, "packet is NULL");
    MUT_ASSERT_TRUE_MSG( io_cmd_p != NULL, "io cmd is NULL");

    payload_p = fbe_transport_get_payload_ex(packet_p);
    MUT_ASSERT_NOT_NULL(payload_p);

    /* First get the edge sizes for the first and last edge.
     */
    fbe_ldo_get_edge_sizes( task_p->lba, task_p->blocks,
                            task_p->exported_block_size,
                            task_p->exported_opt_block_size,
                            task_p->imported_block_size,
                            task_p->imported_opt_block_size,
                            &edge0_bytes,
                            &edge1_bytes);

    /* Setup the edge bytes so that they will not fit into 
     * the sg list that is located in the payload. 
     * This causes an error. 
     */
    max_edge_bytes = fbe_ldo_get_bitbucket_bytes() * FBE_PAYLOAD_SG_ARRAY_LENGTH;

    /* Make the edge0 bytes larger than we can fit into an sg list to
     * force an error. 
     */
    e_status = fbe_ldo_read_setup_sgs(logical_drive_p, payload_p,
                                      max_edge_bytes, edge1_bytes);
    MUT_ASSERT_TRUE(e_status == FBE_STATUS_INSUFFICIENT_RESOURCES);
    MUT_ASSERT_TRUE(fbe_ldo_test_state_validate_check_words(test_state_p) == FBE_STATUS_OK);

    /* Make the edge1 bytes larger than we can fit into an sg list to
     * force an error. 
     */
    e_status = fbe_ldo_read_setup_sgs(logical_drive_p, payload_p,
                                      edge0_bytes, max_edge_bytes);
    MUT_ASSERT_TRUE(e_status == FBE_STATUS_INSUFFICIENT_RESOURCES);
    MUT_ASSERT_TRUE(fbe_ldo_test_state_validate_check_words(test_state_p) == FBE_STATUS_OK);

    /* Make both edge0 and edge 1 bytes larger than we can fit into an
     * sg list to force an error. 
     */
    e_status = fbe_ldo_read_setup_sgs(logical_drive_p, payload_p,
                                      max_edge_bytes, max_edge_bytes);
    MUT_ASSERT_TRUE(e_status == FBE_STATUS_INSUFFICIENT_RESOURCES);
    MUT_ASSERT_TRUE(fbe_ldo_test_state_validate_check_words(test_state_p) == FBE_STATUS_OK);

    /* Call the setup function for the normal case where we expect success.
     */
    e_status = fbe_ldo_read_setup_sgs(logical_drive_p, payload_p,
                                      edge0_bytes, edge1_bytes);
    MUT_ASSERT_TRUE(e_status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(fbe_ldo_test_state_validate_check_words(test_state_p) == FBE_STATUS_OK);

    /* Destroy the state object and assert that all goes well.
     */
    MUT_ASSERT_TRUE(fbe_ldo_test_state_destroy(test_state_p) == FBE_STATUS_OK);

    return status;
}
/******************************************
 * end fbe_ldo_test_read_setup_sg()
 ******************************************/

/*!***************************************************************
 * @fn fbe_ldo_test_read_setup_sg_cases(void)
 ****************************************************************
 * @brief
 *  This function tests the setup of an sg for the
 *  path where resources are not allocated.
 *
 * @param  - None.
 *
 * @return fbe_status_t - FBE_STATUS_OK - no error.
 *                        Other values imply there was an error.
 *
 * HISTORY:
 *  11/07/08 - Created. RPF
 *
 ****************************************************************/
fbe_status_t fbe_ldo_test_read_setup_sg_cases(void)
{
    fbe_status_t status = FBE_STATUS_OK;

    /* Simply call the test function with our task and
     * the read setup sgs function. 
     */
    fbe_ldo_test_sg_cases(fbe_ldo_test_sg_tasks,
                          fbe_ldo_test_read_setup_sg );

    return status;
}
/* end fbe_ldo_test_read_setup_sg_cases() */

/*!**************************************************************
 * @fn fbe_ldo_test_add_read_unit_tests(
 *                               mut_testsuite_t * const suite_p)
 ****************************************************************
 * @brief
 *  Add the read unit tests to the input suite.
 *
 * @param suite_p - suite to add tests to.               
 *
 * @return none.
 *
 * HISTORY:
 *  11/07/2008 - Created. RPF
 *
 ****************************************************************/

void fbe_ldo_test_add_read_unit_tests(mut_testsuite_t * const suite_p)
{
    /* Just add the tests we want to run to the suite.
     */
    MUT_ADD_TEST(suite_p, 
                 fbe_ldo_test_read_count_setup_sg_cases, 
                 fbe_logical_drive_unit_tests_setup, 
                 fbe_logical_drive_unit_tests_teardown);

    MUT_ADD_TEST(suite_p, 
                 fbe_ldo_read_state_machine_tests, 
                 fbe_logical_drive_unit_tests_setup, 
                 fbe_logical_drive_unit_tests_teardown);

    MUT_ADD_TEST(suite_p, 
                 fbe_ldo_test_read_setup_sg_cases, 
                 fbe_logical_drive_unit_tests_setup, 
                 fbe_logical_drive_unit_tests_teardown);
    return;
}
/******************************************
 * end fbe_ldo_test_add_read_unit_tests() 
 ******************************************/

/*****************************************
 * end fbe_logical_drive_test_read.c
 *****************************************/
