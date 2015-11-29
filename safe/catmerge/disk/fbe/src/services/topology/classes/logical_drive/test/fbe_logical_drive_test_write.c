/***************************************************************************
 * Copyright (C) EMC Corporation 2007-2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 * @file fbe_logical_drive_test_write.c
 ***************************************************************************
 *
 * @brief
 *  This file contains testing functions for the logical drive object's
 *  write state machine.
 * 
 * @ingroup logical_drive_class_tests
 * 
 * @version
 *   12/11/2007:  Created. RPF
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_types.h"
#include "fbe_trace.h"
#include "fbe/fbe_logical_drive.h"
#include "fbe/fbe_transport.h"
#include "fbe_logical_drive_private.h"
#include "fbe_logical_drive_io_command.h"
#include "fbe_logical_drive_test_private.h"
#include "fbe_logical_drive_test_prototypes.h"
#include "fbe_logical_drive_test_state_object.h"
#include "fbe_logical_drive_test_alloc_mem_mock_object.h"
#include "fbe_logical_drive_test_send_io_packet_mock_object.h"
#include "fbe_logical_drive_test_complete_packet_mock_object.h"
#include "fbe_logical_drive_test_alloc_io_cmd_mock_object.h"
#include "mut.h"

/*!***************************************************************
 * @fn fbe_ldo_test_state_setup_write(
 *                        fbe_ldo_test_state_t * const test_state_p,   
 *                        fbe_ldo_test_state_machine_task_t *const task_p) 
 ****************************************************************
 * @brief
 *  This function tests read state machine state0.
 *
 * @param test_state_p - The state object, which includes the io cmd and the
 *                       packet.
 * @param task_p - The task object, including parameters
 *                 of this device like block sizes.
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - no error.
 *                 Other values imply there was an error.
 *
 * HISTORY:
 *  11/20/07 - Created. RPF
 *
 ****************************************************************/
fbe_status_t fbe_ldo_test_state_setup_write(fbe_ldo_test_state_t * const test_state_p,
                                            fbe_ldo_test_state_machine_task_t *const task_p)

{
    fbe_logical_drive_io_cmd_t *io_cmd_p;
    fbe_packet_t *io_packet_p;
    fbe_logical_drive_t *logical_drive_p;
    fbe_lba_t pre_read_lba = task_p->lba;
    fbe_block_count_t pre_read_blocks = task_p->blocks;

    /* Determine the range of the pre-read.
     */
    fbe_logical_drive_get_pre_read_lba_blocks_for_write(task_p->exported_block_size,
                                              task_p->exported_opt_block_size,
                                              task_p->imported_block_size,
                                              &pre_read_lba,
                                              &pre_read_blocks);

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
    fbe_ldo_setup_block_sizes( logical_drive_p, task_p->imported_block_size );
    
    /* Setup io packet.
     */
    fbe_ldo_build_sub_packet( io_packet_p,
                              FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,
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

    if ( !fbe_ldo_io_cmd_is_aligned(io_cmd_p) )
    {
        /* When we are not aligned, we need a pre-read descriptor.
         */
        fbe_payload_pre_read_descriptor_t *const pre_read_desc_p = 
            fbe_ldo_test_get_pre_read_descriptor(test_state_p);

        /* Setup the pre-read_descriptor values.
         */
        fbe_payload_pre_read_descriptor_set_lba(pre_read_desc_p, pre_read_lba); 
        fbe_payload_pre_read_descriptor_set_block_count(pre_read_desc_p, pre_read_blocks);
        fbe_payload_pre_read_descriptor_set_sg_list(pre_read_desc_p, 
                                                    fbe_ldo_test_state_get_pre_read_desc_sg(test_state_p));
        
        /* Setup the io block's pre-read descriptor.
         */
        ldo_packet_set_block_cmd_pre_read_desc_ptr(io_packet_p, pre_read_desc_p);

        /* Setup pre-read descriptor sg.
         */
        fbe_ldo_test_setup_sg_list( fbe_ldo_test_state_get_pre_read_desc_sg(test_state_p),
                                    fbe_ldo_test_state_get_pre_read_desc_sg_memory(test_state_p),
                                    (fbe_u32_t)(pre_read_blocks * task_p->exported_block_size),    /* Bytes */
                                    /* Max bytes per sg entries */
                                    (fbe_u32_t)fbe_ldo_test_get_max_blks_per_sg(test_state_p) );

    }/* end if not aligned pre-read descriptor needed */
    else
    {
        /* No pre-read descriptor needed, validate this is true.
         */
        MUT_ASSERT_INT_EQUAL_MSG((fbe_u32_t)(task_p->blocks), (fbe_u32_t)(pre_read_blocks),
                                 "If no pre-read is needed, then blocks and pre_read_blocks should be equal");
        
        MUT_ASSERT_TRUE_MSG((task_p->lba == pre_read_lba), 
                             "If no pre-read is needed, then lba and pre_read_lba should be equal");
    }

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

    MUT_ASSERT_TRUE_MSG(fbe_ldo_io_cmd_get_state(io_cmd_p) == fbe_ldo_io_cmd_write_state0,
                        "LDO:  io cmd state not read state0");

    /* Make sure the check words are sane.
     */
    MUT_ASSERT_TRUE(fbe_ldo_test_state_validate_check_words(test_state_p) == FBE_STATUS_OK);
    return FBE_STATUS_OK;  
}
/* end fbe_ldo_test_state_setup_write() */

/*!***************************************************************
 * @fn fbe_ldo_test_handle_write(
 *                      fbe_ldo_test_state_t * const test_state_p)
 ****************************************************************
 * @brief
 *  This function tests fbe_ldo_handle_write.
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
fbe_status_t fbe_ldo_test_handle_write(fbe_ldo_test_state_t * const test_state_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_logical_drive_io_cmd_t *io_cmd_p = NULL;
    fbe_packet_t *io_packet_p = NULL;
    fbe_logical_drive_t *logical_drive_p = NULL;
    fbe_payload_ex_t *payload_p = NULL;
    fbe_ldo_test_send_io_packet_mock_t *mock_p = NULL;
    fbe_payload_block_operation_opcode_t opcode;
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
    fbe_ldo_send_io_packet_mock_init(mock_p, io_cmd_p, FBE_STATUS_OK, 0);

    MUT_ASSERT_FALSE_MSG(fbe_ldo_io_cmd_is_sg_required(io_cmd_p),
                         "Only aligned cmds should call this function.");

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

    /* Call handle write with our io packet.
     */
    status = fbe_ldo_handle_write(logical_drive_p, io_packet_p);

    /* Make sure the status was set as expected.
     */
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* We should have only called send packet once.
     */
    MUT_ASSERT_INT_EQUAL_MSG(mock_p->packet_sent_count, 1,
                             "LDO: handle_write send packet mock not called");

    /* Make sure we have the right number of bytes in this sg.
     */
    MUT_ASSERT_INT_EQUAL_MSG(mock_p->sg_list_bytes, mock_p->io_packet_bytes,
                             "LDO: handle_write sg list bytes incorrect");
    
    /* If there are not sufficient bytes in the sg list, we will get back a 
     * zero for sg entries.
     */
    MUT_ASSERT_INT_NOT_EQUAL_MSG(mock_p->sg_entries, 0, "io packet sg is incorrect");

    /* Make sure the sub io packet has a write same command.
     */
    MUT_ASSERT_INT_EQUAL_MSG(ldo_packet_get_block_cmd_opcode(io_packet_p), 
                             opcode, 
                             "handle write opcode not set as expected");

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
    return status;  
}
/* end fbe_ldo_test_handle_write() */

/*!***************************************************************
 * @fn fbe_ldo_test_handle_write_pre_read_desc_null(
 *                      fbe_ldo_test_state_t * const test_state_p)
 ****************************************************************
 * @brief
 *  This function tests fbe_ldo_handle_write for the case where
 *  the pre-read descriptor is not setup in the payload.
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
fbe_status_t fbe_ldo_test_handle_write_pre_read_desc_null(fbe_ldo_test_state_t * const test_state_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_logical_drive_io_cmd_t *io_cmd_p;
    fbe_packet_t *io_packet_p;
    fbe_logical_drive_t *logical_drive_p;
    fbe_ldo_test_send_io_packet_mock_t *send_packet_mock_p;
    fbe_ldo_test_complete_packet_mock_t *complete_packet_mock_p;
    fbe_payload_ex_t *payload_p = NULL;
    fbe_payload_block_operation_t *block_operation_p = NULL;
    fbe_payload_block_operation_status_t block_status;
    fbe_payload_block_operation_qualifier_t block_qualifier;

    logical_drive_p = fbe_ldo_test_state_get_logical_drive(test_state_p);
    io_packet_p = fbe_ldo_test_state_get_io_packet(test_state_p);
    io_cmd_p = fbe_ldo_test_state_get_io_cmd(test_state_p);

    payload_p = fbe_transport_get_payload_ex(io_packet_p);
    MUT_ASSERT_NOT_NULL(payload_p);

    block_operation_p = fbe_payload_ex_get_block_operation(payload_p);
    MUT_ASSERT_NOT_NULL(block_operation_p);

    MUT_ASSERT_TRUE_MSG(fbe_ldo_io_cmd_is_sg_required(io_cmd_p),
                         "Only unaligned cmds should call this function.");

    /* Set the pre-read descriptor to NULL to force an error.
     */
    fbe_payload_block_set_pre_read_descriptor(block_operation_p, NULL);
    
    /* Setup our mock io packet send function.
     */
    fbe_ldo_set_send_io_packet_fn(fbe_ldo_send_io_packet_mock);
    send_packet_mock_p = fbe_ldo_test_get_send_packet_mock();
    fbe_ldo_send_io_packet_mock_init(send_packet_mock_p, NULL, /* no io cmd */
                                     FBE_STATUS_OK, 0);
    
    /* Setup our mock complete packet function.
     */
    fbe_ldo_set_complete_io_packet_fn(fbe_ldo_complete_packet_mock);
    complete_packet_mock_p = fbe_ldo_test_get_complete_packet_mock();
    fbe_ldo_complete_packet_mock_init(complete_packet_mock_p);

    MUT_ASSERT_TRUE_MSG( logical_drive_p != NULL, "Logical drive is NULL");
    MUT_ASSERT_TRUE_MSG( io_packet_p != NULL, "io packet is NULL");
    MUT_ASSERT_TRUE_MSG( io_cmd_p != NULL, "io cmd is NULL");

    /* Call handle write with our io packet.
     */
    status = fbe_ldo_handle_write(logical_drive_p, io_packet_p);

    /* Make sure the status was set as expected.
     */
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);

    /* We should not have set any packets.
     */
    MUT_ASSERT_INT_EQUAL_MSG(send_packet_mock_p->packet_sent_count, 0,
                             "LDO: handle_write send packet mock not called");

    /* Make sure the complete mock was called as expected with the correct
     * status. 
     */
    MUT_ASSERT_INT_EQUAL_MSG(complete_packet_mock_p->packet_completed_count, 1,
                             "LDO: handle_write complete packet mock not called");
    MUT_ASSERT_INT_EQUAL_MSG(complete_packet_mock_p->packet_status, 
                             FBE_STATUS_GENERIC_FAILURE,
                             "LDO: handle_write packet status not set as expected");
    MUT_ASSERT_INT_EQUAL_MSG(complete_packet_mock_p->packet_status_qualifier, 
                             0,
                             "LDO: handle_write packet qualifier not set as expected");
    /* Make sure the payload error was as we expected.
     */
    fbe_payload_block_get_status(block_operation_p, &block_status);
    fbe_payload_block_get_qualifier(block_operation_p, &block_qualifier);

    MUT_ASSERT_INT_EQUAL(block_status, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID_REQUEST);
    MUT_ASSERT_INT_EQUAL(block_qualifier,
                         FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_INVALID_PRE_READ_DESCRIPTOR);
    return status;  
}
/* end fbe_ldo_test_handle_write_pre_read_desc_null() */

/*!***************************************************************
 * @fn fbe_ldo_test_handle_write_remainder_case(
 *                      fbe_ldo_test_state_t * const test_state_p)
 ****************************************************************
 * @brief
 *  This function tests fbe_ldo_handle_write case where an sg
 *  is passed down that has extra data in it.
 *
 * @param test_state_p - The test state with info about the configuration.
 *
 * @return fbe_status_t - FBE_STATUS_OK - no error.
 *                         Other values imply there was an error.
 *
 * HISTORY:
 *  11/10/08 - Created. RPF
 *
 ****************************************************************/
fbe_status_t fbe_ldo_test_handle_write_remainder_case(fbe_ldo_test_state_t * const test_state_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_logical_drive_io_cmd_t *io_cmd_p = NULL;
    fbe_packet_t *io_packet_p = NULL;
    fbe_logical_drive_t *logical_drive_p = NULL;
    fbe_ldo_test_send_io_packet_mock_t *send_mock_p = NULL;
    fbe_ldo_test_alloc_io_cmd_mock_t *alloc_mock_p = NULL;
    fbe_payload_block_operation_opcode_t opcode;
    fbe_payload_ex_t *payload_p = NULL;
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

    /* Call handle write with our io packet.
     */
    status = fbe_ldo_handle_write(logical_drive_p, io_packet_p);

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
                                 "LDO: handle_write send packet mock not called");

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
                                 "LDO: handle_write send packet mock not called");

        /* Make sure that the allocate io cmd
         * mock was called. 
         */
        fbe_ldo_alloc_io_cmd_mock_validate(alloc_mock_p);
    }
    return status;  
}
/* end fbe_ldo_test_handle_write_remainder_case() */

/*!***************************************************************
 * @fn fbe_ldo_test_write_state0(fbe_ldo_test_state_t * const test_state_p)
 ****************************************************************
 * @brief
 *  This function tests read state machine state0.
 *
 * @param test_state_p - The state object, which includes the io cmd and the
 *                       packet.
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - no error.
 *                 Other values imply there was an error.
 *
 * HISTORY:
 *  05/29/08 - Created. RPF
 *
 ****************************************************************/
fbe_status_t fbe_ldo_test_write_state0(fbe_ldo_test_state_t * const test_state_p)
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

    MUT_ASSERT_TRUE_MSG( logical_drive_p != NULL, "Logical drive is NULL");
    MUT_ASSERT_TRUE_MSG( io_packet_p != NULL, "io packet is NULL");
    MUT_ASSERT_TRUE_MSG( io_cmd_p != NULL, "io cmd is NULL");

    /* Free up the io packet's sg ptr, since this state does not need the sg set.
     */
    payload_p = fbe_transport_get_payload_ex(fbe_ldo_test_state_get_io_packet(test_state_p));
    fbe_payload_ex_set_sg_list(payload_p, NULL, 0);
    
    /* Call state 0 with our io command.
     */
    state_status = fbe_ldo_io_cmd_write_state0(io_cmd_p);

    /* Make sure the status and state were set as expected.
     */
    MUT_ASSERT_INT_EQUAL_MSG(state_status, FBE_LDO_STATE_STATUS_EXECUTING,
                             "LDO: read state 0 return status unexpected");
    MUT_ASSERT_TRUE_MSG(fbe_ldo_io_cmd_get_state(io_cmd_p) == fbe_ldo_io_cmd_write_state10,
                        "LDO: read io command state unexpected");
    
    return status;  
}
/* end fbe_ldo_test_write_state0() */

/****************************************************************
 * fbe_ldo_test_write_state0_with_error(
 *                        fbe_ldo_test_state_t * const test_state_p)
 ****************************************************************
 * @brief
 *  This function tests read state machine state0.
 *
 * @param test_state_p - The state object, which includes the io cmd and the
 *                       packet.
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - no error.
 *                 Other values imply there was an error.
 *
 * HISTORY:
 *  05/30/08 - Created. RPF
 *
 ****************************************************************/
fbe_status_t fbe_ldo_test_write_state0_with_error(fbe_ldo_test_state_t * const test_state_p)
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

    MUT_ASSERT_TRUE_MSG( logical_drive_p != NULL, "Logical drive is NULL");
    MUT_ASSERT_TRUE_MSG( io_packet_p != NULL, "io packet is NULL");
    MUT_ASSERT_TRUE_MSG( io_cmd_p != NULL, "io cmd is NULL");

    /* Free up the io packet's sg ptr, since this state does not need the sg set.
     */
    payload_p = fbe_transport_get_payload_ex(fbe_ldo_test_state_get_io_packet(test_state_p));
    fbe_payload_ex_set_sg_list(payload_p, NULL, 0);

    /* Since this request is not aligned, it needs a pre-read descriptor.
     * This descriptor will be validated in state0. 
     * Set the descriptor to NULL in the io packet to intentionally cause an error. 
     */
    ldo_packet_set_block_cmd_pre_read_desc_ptr(io_packet_p, NULL);
    
    /* Call state 0 with our io command.
     */
    state_status = fbe_ldo_io_cmd_write_state0(io_cmd_p);

    /* Make sure the status and state were set as expected.
     * We should have detected the error anad transitioned to state 40. 
     */
    MUT_ASSERT_INT_EQUAL_MSG(state_status, FBE_LDO_STATE_STATUS_EXECUTING,
                             "LDO: write state 0 return status unexpected");
    MUT_ASSERT_TRUE_MSG(fbe_ldo_io_cmd_get_state(io_cmd_p) == fbe_ldo_io_cmd_finished,
                        "LDO: write io command state unexpected");
    
    return status;  
}
/* end fbe_ldo_test_write_state0_with_error() */

/****************************************************************
 * fbe_ldo_test_write_state10_aligned(
 *                        fbe_ldo_test_state_t * const test_state_p)
 ****************************************************************
 * @brief
 *  This function tests read state machine state10.
 *
 * @param test_state_p - The state object, which includes the io cmd and the
 *                       packet.
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - no error.
 *                 Other values imply there was an error.
 *
 * HISTORY:
 *  05/30/08 - Created. RPF
 *
 ****************************************************************/
fbe_status_t fbe_ldo_test_write_state10_aligned(fbe_ldo_test_state_t * const test_state_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_ldo_state_status_enum_t state_status;
    fbe_logical_drive_io_cmd_t *io_cmd_p;
    fbe_packet_t *io_packet_p;
    fbe_logical_drive_t *logical_drive_p;

    logical_drive_p = fbe_ldo_test_state_get_logical_drive(test_state_p);
    io_packet_p = fbe_ldo_test_state_get_io_packet(test_state_p);
    io_cmd_p = fbe_ldo_test_state_get_io_cmd(test_state_p);

    MUT_ASSERT_TRUE_MSG( logical_drive_p != NULL, "Logical drive is NULL");
    MUT_ASSERT_TRUE_MSG( io_packet_p != NULL, "io packet is NULL");
    MUT_ASSERT_TRUE_MSG( io_cmd_p != NULL, "io cmd is NULL");

    /* Call state 10 with our io command.
     */
    state_status = fbe_ldo_io_cmd_write_state10(io_cmd_p);

    /* Make sure the status and state were set as expected.
     */
    MUT_ASSERT_TRUE_MSG(state_status == FBE_LDO_STATE_STATUS_EXECUTING,
                        "LDO: return status not executing");
    MUT_ASSERT_TRUE_MSG(fbe_ldo_io_cmd_get_state(io_cmd_p) == fbe_ldo_io_cmd_write_state20,
                        "LDO: io command state unexpected");

    /* The sg entries should not be modified.
     */
    MUT_ASSERT_TRUE_MSG(fbe_ldo_io_cmd_get_sg_entries(io_cmd_p) == 0,
                        "LDO: io command sg entries changed");
    return status;  
}
/* end fbe_ldo_test_write_state10_aligned() */

/*!***************************************************************
 * @fn fbe_ldo_test_write_state10_unaligned(
 *                      fbe_ldo_test_state_t * const test_state_p)
 ****************************************************************
 * @brief
 *  This function tests write state machine state10.
 *
 * @param test_state_p - The state object, which includes the io cmd and the
 *                       packet.
 * @return
 *  fbe_status_t - FBE_STATUS_OK - no error.
 *                 Other values imply there was an error.
 *
 * HISTORY:
 *  08/05/08 - Created. RPF
 *
 ****************************************************************/
fbe_status_t fbe_ldo_test_write_state10_unaligned(fbe_ldo_test_state_t * const test_state_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_ldo_state_status_enum_t state_status;
    fbe_logical_drive_io_cmd_t *io_cmd_p;
    fbe_packet_t *io_packet_p;
    fbe_logical_drive_t *logical_drive_p;

    logical_drive_p = fbe_ldo_test_state_get_logical_drive(test_state_p);
    io_packet_p = fbe_ldo_test_state_get_io_packet(test_state_p);
    io_cmd_p = fbe_ldo_test_state_get_io_cmd(test_state_p);

    MUT_ASSERT_TRUE_MSG(fbe_ldo_io_cmd_is_sg_required(io_cmd_p),
                         "Only unaligned cmds should call this function.");

    MUT_ASSERT_TRUE_MSG( logical_drive_p != NULL, "Logical drive is NULL");
    MUT_ASSERT_TRUE_MSG( io_packet_p != NULL, "io packet is NULL");
    MUT_ASSERT_TRUE_MSG( io_cmd_p != NULL, "io cmd is NULL");
    
    /* Call state 10 with our io command.
     */
    state_status = fbe_ldo_io_cmd_write_state10(io_cmd_p);

    /* Make sure the status and state were set as expected.
     */
    MUT_ASSERT_TRUE_MSG(state_status == FBE_LDO_STATE_STATUS_EXECUTING,
                        "LDO: return status not waiting");

    /* We always expect a transition to state11.
     */
    MUT_ASSERT_TRUE_MSG(fbe_ldo_io_cmd_get_state(io_cmd_p) == fbe_ldo_io_cmd_write_state11,
                        "LDO: io command state unexpected.  Expected write 11");

    return status;  
}
/* end fbe_ldo_test_write_state10_unaligned() */

/*!***************************************************************
 * @fn fbe_ldo_test_write_state10_unaligned_error(
 *                      fbe_ldo_test_state_t * const test_state_p)
 ****************************************************************
 * @brief
 *  This function tests write state machine state10.
 *
 * @param test_state_p - The state object, which includes the io cmd and the
 *                       packet.
 * 
 * @return
 *  fbe_status_t - FBE_STATUS_OK - no error.
 *                 Other values imply there was an error.
 *
 * HISTORY:
 *  08/07/08 - Created. RPF
 *
 ****************************************************************/
fbe_status_t fbe_ldo_test_write_state10_unaligned_error(fbe_ldo_test_state_t * const test_state_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_ldo_state_status_enum_t state_status;
    fbe_logical_drive_io_cmd_t *io_cmd_p;
    fbe_packet_t *io_packet_p;
    fbe_logical_drive_t *logical_drive_p;

    logical_drive_p = fbe_ldo_test_state_get_logical_drive(test_state_p);
    io_packet_p = fbe_ldo_test_state_get_io_packet(test_state_p);
    io_cmd_p = fbe_ldo_test_state_get_io_cmd(test_state_p);

    MUT_ASSERT_TRUE_MSG(fbe_ldo_io_cmd_is_sg_required(io_cmd_p),
                         "Only unaligned cmds should call this function.");

    MUT_ASSERT_TRUE_MSG( logical_drive_p != NULL, "Logical drive is NULL");
    MUT_ASSERT_TRUE_MSG( io_packet_p != NULL, "io packet is NULL");
    MUT_ASSERT_TRUE_MSG( io_cmd_p != NULL, "io cmd is NULL");
    
    /* Call state 10 with our io command.
     */
    state_status = fbe_ldo_io_cmd_write_state10(io_cmd_p);

    /* We expect an error, which causes us to transition to state 11
     * immediately. 
     */
    MUT_ASSERT_TRUE_MSG(state_status == FBE_LDO_STATE_STATUS_EXECUTING,
                        "LDO: return status not waiting");
    MUT_ASSERT_TRUE_MSG(fbe_ldo_io_cmd_get_state(io_cmd_p) == fbe_ldo_io_cmd_write_state11,
                        "LDO: io command state unexpected");
    return status;  
}
/* end fbe_ldo_test_write_state10_unaligned_error() */
/*!***************************************************************
 * @fn fbe_ldo_test_write_state11_unaligned(
 *                      fbe_ldo_test_state_t * const test_state_p)
 ****************************************************************
 * @brief
 *  This function tests write state machine state10.
 *
 * @param test_state_p - The state object, which includes the io cmd and the
 *                       packet.
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - no error.
 *                 Other values imply there was an error.
 *
 * HISTORY:
 *  08/07/08 - Created. RPF
 *
 ****************************************************************/
fbe_status_t fbe_ldo_test_write_state11_unaligned(fbe_ldo_test_state_t * const test_state_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_ldo_state_status_enum_t state_status;
    fbe_logical_drive_io_cmd_t *io_cmd_p;
    fbe_packet_t *io_packet_p;
    fbe_logical_drive_t *logical_drive_p;

    logical_drive_p = fbe_ldo_test_state_get_logical_drive(test_state_p);
    io_packet_p = fbe_ldo_test_state_get_io_packet(test_state_p);
    io_cmd_p = fbe_ldo_test_state_get_io_cmd(test_state_p);

    MUT_ASSERT_TRUE_MSG(fbe_ldo_io_cmd_is_sg_required(io_cmd_p),
                         "Only unaligned cmds should call this function.");

    MUT_ASSERT_TRUE_MSG( logical_drive_p != NULL, "Logical drive is NULL");
    MUT_ASSERT_TRUE_MSG( io_packet_p != NULL, "io packet is NULL");
    MUT_ASSERT_TRUE_MSG( io_cmd_p != NULL, "io cmd is NULL");
    
    /* Call state 10 with our io command.
     */
    state_status = fbe_ldo_io_cmd_write_state11(io_cmd_p);

    /* Make sure the status and state were set as expected.
     */
    MUT_ASSERT_TRUE_MSG(state_status == FBE_LDO_STATE_STATUS_EXECUTING,
                        "LDO: return status not waiting");
    MUT_ASSERT_TRUE_MSG(fbe_ldo_io_cmd_get_state(io_cmd_p) == fbe_ldo_io_cmd_write_state20,
                        "LDO: io command state unexpected");

    /* The sg entries should be modified in this path.
     * We assume the write is unaligned and has at least one edge.
     * We should have at least 3 sgs, one for the edge, one for the 
     * client data and one for the terminator.
     */
    MUT_ASSERT_TRUE_MSG(fbe_ldo_io_cmd_get_sg_entries(io_cmd_p) > 1,
                        "LDO: io command sg entries not set");

    return status;  
}
/* end fbe_ldo_test_write_state11_unaligned() */

/*!***************************************************************
 * @fn fbe_ldo_test_write_state11_unaligned_sg_error(
 *                      fbe_ldo_test_state_t * const test_state_p)
 ****************************************************************
 * @brief
 *  This function tests write state machine state10.
 *
 * @param test_state_p - The state object, which includes the io cmd and the
 *                       packet.
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - no error.
 *                 Other values imply there was an error.
 *
 * HISTORY:
 *  08/07/08 - Created. RPF
 *
 ****************************************************************/
fbe_status_t fbe_ldo_test_write_state11_unaligned_sg_error(fbe_ldo_test_state_t * const test_state_p)
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

    MUT_ASSERT_TRUE_MSG(fbe_ldo_io_cmd_is_sg_required(io_cmd_p),
                        "Only unaligned cmds should call this function.");

    MUT_ASSERT_TRUE_MSG( logical_drive_p != NULL, "Logical drive is NULL");
    MUT_ASSERT_TRUE_MSG( io_packet_p != NULL, "io packet is NULL");
    MUT_ASSERT_TRUE_MSG( io_cmd_p != NULL, "io cmd is NULL");

    /* Free up the io packet's sg ptr, since this state does not need the sg set.
     */
    payload_p = fbe_transport_get_payload_ex(fbe_ldo_test_state_get_io_packet(test_state_p));
    fbe_payload_ex_set_sg_list(payload_p, NULL, 0);
    
    /* Call state 10 with our io command.
     */
    state_status = fbe_ldo_io_cmd_write_state11(io_cmd_p);

    /* Make sure the status and state were set as expected.
     */
    MUT_ASSERT_TRUE_MSG(state_status == FBE_LDO_STATE_STATUS_EXECUTING,
                        "LDO: return status not executing");
    MUT_ASSERT_TRUE_MSG(fbe_ldo_io_cmd_get_state(io_cmd_p) == fbe_ldo_io_cmd_finished,
                        "LDO: io command state unexpected");

    /* The sg entries should not be modified in this path.
     */
    MUT_ASSERT_TRUE_MSG(fbe_ldo_io_cmd_get_sg_entries(io_cmd_p) == 0,
                        "LDO: io command sg entries changed");
    
    return status;  
}
/* end fbe_ldo_test_write_state11_unaligned_sg_error() */

/*!***************************************************************
 * @fn fbe_ldo_test_write_state20(
 *                      fbe_ldo_test_state_t * const test_state_p)
 ****************************************************************
 * @brief
 *  This function tests write state machine state20.
 *
 * @param test_state_p - The state object, which includes the io cmd and the
 *                       packet.
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - no error.
 *                 Other values imply there was an error.
 *
 * HISTORY:
 *  05/30/08 - Created. RPF
 *
 ****************************************************************/
fbe_status_t fbe_ldo_test_write_state20( fbe_ldo_test_state_t * const test_state_p )
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
                            "LDO: write state20 sg entries not set");
    }
    /* Get the expected number of sgs. We will compare to this later.
     */
    expected_sg_count = fbe_ldo_io_cmd_wr_count_sg_entries(io_cmd_p);

    /* Execute state 20.
     */
    state_status = fbe_ldo_io_cmd_write_state20(io_cmd_p);

    /* Make sure the status and state were set as expected.
     */
    MUT_ASSERT_INT_EQUAL_MSG(state_status, FBE_LDO_STATE_STATUS_WAITING,
                             "LDO: write state20 return status unexpected");
    MUT_ASSERT_TRUE_MSG(fbe_ldo_io_cmd_get_state(io_cmd_p) == fbe_ldo_io_cmd_write_state30,
                        "write state20 io command state unexpected");

    /* We should have only called send packet once.
     */
    MUT_ASSERT_INT_EQUAL_MSG(mock_p->packet_sent_count, 1,
                             "LDO: write state20 send packet mock not called");
    /* Make sure we have the right number of bytes in this sg.
     */
    MUT_ASSERT_INT_EQUAL_MSG(mock_p->sg_list_bytes, mock_p->io_packet_bytes,
                             "LDO: write state20 sg list bytes incorrect");
    
    /* If there are not sufficient bytes in the sg list, we will get back a 
     * zero for sg entries.
     */
    MUT_ASSERT_INT_NOT_EQUAL_MSG(mock_p->sg_entries, 0, "io packet sg is incorrect");

    /* Make sure the sub io packet has a write command.
     */
    MUT_ASSERT_INT_EQUAL_MSG(ldo_packet_get_block_cmd_opcode(io_packet_p), 
                             FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE, 
                             "write state 20 opcode not write");

    /* Make sure we have the right number of sg entries in this sg.  We add one for the null terminator.
     * We can validate this using fbe_ldo_rd_count_sg_entries() since it is used
     * to calculate how many sgs to allocate.
     */
    MUT_ASSERT_INT_EQUAL_MSG(mock_p->sg_entries + 1, expected_sg_count,
                             "LDO: write state20 sg entries incorrect");

    /* Make sure the state was set to state 30 before we sent out the io packet.
     */
    MUT_ASSERT_TRUE_MSG(mock_p->io_cmd_state == fbe_ldo_io_cmd_write_state30,
                        "write state20 did not transition to next state before sending io packet");

    return status;
}
/* end fbe_ldo_test_write_state20() */

/****************************************************************
 * fbe_ldo_test_write_state20_sg_error()
 ****************************************************************
 * @brief
 *  This function tests the error case of write state machine state20.
 *
 * @param test_state_p - The state object, which includes the io cmd and the
 *                       packet.
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - no error.
 *                 Other values imply there was an error.
 *
 * HISTORY:
 *  11/20/07 - Created. RPF
 *
 ****************************************************************/
fbe_status_t fbe_ldo_test_write_state20_sg_error( fbe_ldo_test_state_t * const test_state_p )
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
                        "LDO: write state20 sg entries not set");

    /* Change the number of sg entries in the io cmd to force an error 
     * in setting up the sgs. 
     * In this case we decrement it by one. 
     */
    fbe_ldo_io_cmd_set_sg_entries(io_cmd_p,
                                  fbe_ldo_io_cmd_get_sg_entries(io_cmd_p) - 1);

    /* Execute state 20.
     */
    state_status = fbe_ldo_io_cmd_write_state20(io_cmd_p);

    /* Make sure the status and state were set as expected on error.
     */
    MUT_ASSERT_INT_EQUAL_MSG(state_status, FBE_LDO_STATE_STATUS_EXECUTING,
                             "LDO: write state20 return status unexpected");
    MUT_ASSERT_TRUE_MSG(fbe_ldo_io_cmd_get_state(io_cmd_p) == fbe_ldo_io_cmd_finished,
                        "write state20 io command state unexpected on error");

    /* We should not have called send packet.
     */
    MUT_ASSERT_INT_EQUAL_MSG(mock_p->packet_sent_count, 0,
                             "LDO: write state20 send packet mock called unexpectedly");

    /* Change the sg entries to cause an error in setting up the sg. 
     * In this case we increment it by two to cause it to be too large. 
     */
    fbe_ldo_io_cmd_set_sg_entries(io_cmd_p,
                                  fbe_ldo_io_cmd_get_sg_entries(io_cmd_p) + 2);
    /* Execute state 20.
     */
    state_status = fbe_ldo_io_cmd_write_state20(io_cmd_p);

    /* Make sure the status and state were set as expected on error.
     */
    MUT_ASSERT_INT_EQUAL_MSG(state_status, FBE_LDO_STATE_STATUS_EXECUTING,
                             "LDO: write state20 return status unexpected");
    MUT_ASSERT_TRUE_MSG(fbe_ldo_io_cmd_get_state(io_cmd_p) == fbe_ldo_io_cmd_finished,
                        "write state20 io command state unexpected on error");

    /* We should not have called send packet.
     */
    MUT_ASSERT_INT_EQUAL_MSG(mock_p->packet_sent_count, 0,
                             "LDO: write state20 send packet mock called unexpectedly");

    return status;
}
/* end fbe_ldo_test_write_state20_sg_error() */

/*!***************************************************************
 * @fn fbe_ldo_test_write_state30(
 *                      fbe_ldo_test_state_t * const test_state_p)
 ****************************************************************
 * @brief
 *  This function tests write state machine state30.
 *
 * @param test_state_p - The state object, which includes the io cmd and the
 *                       packet.
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - no error.
 *                 Other values imply there was an error.
 *
 * HISTORY:
 *  08/07/08 - Created. RPF
 *
 ****************************************************************/
fbe_status_t fbe_ldo_test_write_state30( fbe_ldo_test_state_t * const test_state_p )
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
    state_status = fbe_ldo_io_cmd_write_state30(io_cmd_p);

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

    /* Make sure the client packet status and qualifier is set to the sub packet
     * status and qualifier
     */
    MUT_ASSERT_INT_EQUAL_MSG(fbe_transport_get_status_code(io_packet_p),
                             FBE_STATUS_OK,
                             "LDO: Client packet's status not set from sub packet");
    MUT_ASSERT_INT_EQUAL_MSG(fbe_transport_get_status_qualifier(io_packet_p),
                             0x12345678,
                             "LDO: Client packet's qualifier not set from sub packet");

    /* Make sure the status and state were set as expected.
     */
    MUT_ASSERT_INT_EQUAL_MSG(state_status, FBE_LDO_STATE_STATUS_EXECUTING,
                             "LDO: write state30 return status unexpected");
    MUT_ASSERT_TRUE_MSG(fbe_ldo_io_cmd_get_state(io_cmd_p) == fbe_ldo_io_cmd_finished,
                        "write state30 io command state unexpected");

    return status;
}
/* end fbe_ldo_test_write_state30() */

/*!***************************************************************
 * @fn fbe_logical_drive_write_state_machine_test_case(
 *                fbe_ldo_test_state_machine_task_t *const task_p)
 ****************************************************************
 * @brief
 *  This function executes tests of the read state machine.
 *
 * @param task_p - The task object to use for this case, which includes
 *                 information like the block sizes of the device.
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - no error.
 *                 Other values imply there was an error.
 *
 * HISTORY:
 *  05/19/08 - Created. RPF
 *
 ****************************************************************/
fbe_status_t fbe_logical_drive_write_state_machine_test_case(fbe_ldo_test_state_machine_task_t *const task_p)
{
    fbe_status_t status = FBE_STATUS_OK;

    /* Get the read state object. 
     */
    fbe_ldo_test_state_t * const test_state_p = fbe_ldo_test_get_state_object();
    fbe_logical_drive_io_cmd_t * io_cmd_p;
    fbe_bool_t b_is_sg_required;
    fbe_bool_t b_is_aligned;
    fbe_bool_t b_is_pad_required;

    MUT_ASSERT_TRUE(test_state_p != NULL);

    /* Make sure we can at least setup a write and destroy the state.
     */
    status = fbe_ldo_test_state_setup_write(test_state_p, task_p);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    io_cmd_p = fbe_ldo_test_state_get_io_cmd(test_state_p);
    MUT_ASSERT_TRUE(io_cmd_p != NULL);
    MUT_ASSERT_TRUE(fbe_ldo_test_state_destroy(test_state_p) == FBE_STATUS_OK);

    /* Get info about alignment, pad and sg required.
     */ 
    b_is_sg_required = fbe_ldo_io_cmd_is_sg_required(io_cmd_p);
    b_is_aligned = fbe_ldo_io_cmd_is_aligned(io_cmd_p);
    b_is_pad_required = fbe_ldo_io_cmd_is_padding_required(io_cmd_p);

    /************************************************** 
     *  State 0 tests.
     **************************************************/
    fbe_ldo_test_execute_state_machine(test_state_p,
                                       fbe_ldo_test_state_setup_write,
                                       fbe_ldo_test_write_state0,
                                       task_p);

    /* Only misaligned requests need a pre-read descriptor and thus can 
     * only get an error in write state0. 
     */
    if (b_is_aligned == FALSE)
    {
        /************************************************** 
         *  State 0 pre-read descriptor error tests.
         **************************************************/
        fbe_ldo_test_execute_state_machine(test_state_p,
                                           fbe_ldo_test_state_setup_write,
                                           fbe_ldo_test_write_state0_with_error,
                                           task_p);
    }

    if (b_is_sg_required == FALSE)
    {
        /************************************************** 
         *  State 10 aligned tests.
         **************************************************/
        fbe_ldo_test_execute_state_machine(test_state_p,
                                           fbe_ldo_test_state_setup_write,
                                           fbe_ldo_test_write_state10_aligned,
                                           task_p);
    }
    else
    { 
        /************************************************** 
         *  Perform the unaligned state 10 and 11 tests.
         **************************************************/
        fbe_ldo_test_execute_state_machine(test_state_p,
                                           fbe_ldo_test_state_setup_write,
                                           fbe_ldo_test_write_state10_unaligned,
                                           task_p);


        fbe_ldo_test_execute_state_machine(test_state_p,
                                           fbe_ldo_test_state_setup_write,
                                           fbe_ldo_test_write_state10_unaligned_error,
                                           task_p);

        fbe_ldo_test_execute_state_machine(test_state_p,
                                           fbe_ldo_test_state_setup_write,
                                           fbe_ldo_test_write_state11_unaligned,
                                           task_p);

        fbe_ldo_test_execute_state_machine(test_state_p,
                                           fbe_ldo_test_state_setup_write,
                                           fbe_ldo_test_write_state11_unaligned_sg_error,
                                           task_p);

        /* call state 20 state that requires an sg.
         */
        fbe_ldo_test_execute_state_machine(test_state_p,
                                           fbe_ldo_test_state_setup_write,
                                           fbe_ldo_test_write_state20_sg_error,
                                           task_p);
    }/* end else b_is_sg_required == TRUE */

    /************************************************** 
     *  State 20 tests.
     **************************************************/
    fbe_ldo_test_execute_state_machine(test_state_p,
                                       fbe_ldo_test_state_setup_write,
                                       fbe_ldo_test_write_state20,
                                       task_p);

    /************************************************** 
     *  State 30 tests.
     **************************************************/
    fbe_ldo_test_execute_state_machine(test_state_p,
                                       fbe_ldo_test_state_setup_write,
                                       fbe_ldo_test_write_state30,
                                       task_p);

    /************************************************** 
     * Handle inline cases. Inline is only for 
     * cases where we do not need padding. 
     **************************************************/
    if (b_is_pad_required == FBE_FALSE)
    {
        if (b_is_aligned)
        {
            /* Until we get an sg in the packet, we will only 
             * use this code for aligned.  
             */
            fbe_ldo_test_execute_state_machine(test_state_p,
                                               fbe_ldo_test_state_setup_write,
                                               fbe_ldo_test_handle_write,
                                               task_p);
        }
        if (b_is_aligned == FBE_FALSE)
        {
            /* For unaligned cases an sg descriptor is used.
             * Test the error case where the pre-read descriptor is not setup.
             */
            fbe_ldo_test_execute_state_machine(test_state_p,
                                               fbe_ldo_test_state_setup_write,
                                               fbe_ldo_test_handle_write_pre_read_desc_null,
                                               task_p);
            /* Test the case where there is extra data in the sg list and we need 
             * to allocate a resource to handle this.
             */
            fbe_ldo_test_execute_state_machine(test_state_p,
                                               fbe_ldo_test_state_setup_write,
                                               fbe_ldo_test_handle_write_remainder_case,
                                               task_p);
        }
    } /* end if no pad required */
    return status;
}
/* end fbe_logical_drive_write_state_machine_test_case() */

/*!***************************************************************
 * @fn fbe_ldo_test_write_count_setup_sgs(
 *                    fbe_ldo_test_state_t * const test_state_p,
 *                    fbe_ldo_test_state_machine_task_t *const task_p)
 ****************************************************************
 * @brief
 *  This function executes sg tests of the write state machine.
 *
 * @param test_state_p - The state object, which includes the io cmd and the
 *                       packet.
 * @param task_p - The task object, including parameters
 *                 of this device like block sizes.
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - no error.
 *                 Other values imply there was an error.
 *
 * HISTORY:
 *  05/30/08 - Created. RPF
 *
 ****************************************************************/
fbe_status_t fbe_ldo_test_write_count_setup_sgs(
    fbe_ldo_test_state_t * const test_state_p,
    fbe_ldo_test_state_machine_task_t *const task_p )
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_logical_drive_io_cmd_t *io_cmd_p;
    fbe_u32_t sg_entries;
    
    status = fbe_ldo_test_state_setup_write(test_state_p, task_p);

    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(fbe_ldo_test_state_validate_check_words(test_state_p) == FBE_STATUS_OK);

    io_cmd_p = fbe_ldo_test_state_get_io_cmd(test_state_p);

    /* If we are not aligned, validate the count/setup sgs.
     */
    if (fbe_ldo_io_cmd_is_sg_required(io_cmd_p))
    {
        /* Determine how many sg entries are needed.
         */
        sg_entries = fbe_ldo_io_cmd_wr_count_sg_entries(io_cmd_p);
    
        MUT_ASSERT_TRUE(sg_entries > 0);

        /* Save this value in the io cmd for later use.
         */
        fbe_ldo_io_cmd_set_sg_entries(io_cmd_p, sg_entries);

        /* Setup the sg list.
         * If something goes wrong it will return an error.
         */
        status = fbe_ldo_io_cmd_wr_setup_sg(io_cmd_p);

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
/* end fbe_ldo_test_write_count_setup_sgs() */

/*!***************************************************************
 * @fn fbe_ldo_test_write_count_setup_sg_cases(void)
 ****************************************************************
 * @brief
 *  This function executes tests of the read state machine.
 *
 * @param - None.
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - no error.
 *                 Other values imply there was an error.
 *
 * HISTORY:
 *  11/20/07 - Created. RPF
 *
 ****************************************************************/
fbe_status_t fbe_ldo_test_write_count_setup_sg_cases(void)
{
    fbe_status_t status = FBE_STATUS_OK;

    /* Simply call the test function with our task and
     * the read setup sgs function. 
     */
    fbe_ldo_test_sg_cases(fbe_ldo_test_sg_tasks,
                          fbe_ldo_test_write_count_setup_sgs);
    fbe_ldo_test_sg_cases(fbe_ldo_test_sg_lossy_tasks,
                          fbe_ldo_test_write_count_setup_sgs);

    return status;
}
/* end fbe_ldo_test_write_count_setup_sg_cases() */

/*!***************************************************************
 * @fn fbe_ldo_write_state_machine_tests(void)
 ****************************************************************
 * @brief
 *  This function executes tests of the write state machine.
 * 
 * @param - None.
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - no error.
 *                 Other values imply there was an error.
 *
 * HISTORY:
 *  12/11/07 - Created. RPF
 *
 ****************************************************************/
fbe_status_t fbe_ldo_write_state_machine_tests(void)
{
    fbe_status_t status;

    /* Test all the logical drive write cases for these test tasks.
     */
    status = fbe_logical_drive_state_machine_tests(fbe_ldo_test_io_tasks,
                                                   fbe_logical_drive_write_state_machine_test_case);

    return status;
}
/* end fbe_ldo_write_state_machine_tests() */

/*!**************************************************************
 * @fn fbe_ldo_test_write_setup_sg(
 *                   fbe_ldo_test_state_t * const test_state_p,
 *                   fbe_ldo_test_state_machine_task_t *const task_p)
 ****************************************************************
 * @brief
 *  Test error cases for fbe_ldo_io_cmd_write_setup_host_sg
 *
 * @param test_state_p - The test state with info about the configuration.
 * @param task_p - The test task to use for testing the write sg fns.
 *
 * @return -   
 *  fbe_status_t - FBE_STATUS_OK - no error.
 *                 Other values imply there was an error.
 *
 * HISTORY:
 *  11/07/2008 - Created. RPF
 *
 ****************************************************************/
fbe_status_t fbe_ldo_test_write_setup_sg(fbe_ldo_test_state_t * const test_state_p,
                                         fbe_ldo_test_state_machine_task_t *const task_p )
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_logical_drive_io_cmd_t *io_cmd_p = NULL;
    fbe_u32_t edge0_bytes = 0;
    fbe_u32_t edge1_bytes = 0;
    fbe_payload_ex_t * payload_p = NULL;
    fbe_packet_t *packet_p = NULL;
    fbe_logical_drive_t *logical_drive_p = NULL;
    fbe_status_t e_status;
    fbe_payload_block_operation_t *block_operation_p = NULL;
    fbe_payload_pre_read_descriptor_t *pre_read_desc_p = NULL;
    fbe_sg_element_t *pre_read_sg_p = NULL;
    fbe_lba_t pre_read_lba;
    fbe_block_count_t pre_read_blocks;
    
    status = fbe_ldo_test_state_setup_write(test_state_p, task_p);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    logical_drive_p = fbe_ldo_test_state_get_logical_drive(test_state_p);
    packet_p = fbe_ldo_test_state_get_io_packet(test_state_p);
    io_cmd_p = fbe_ldo_test_state_get_io_cmd(test_state_p);

    MUT_ASSERT_TRUE_MSG( logical_drive_p != NULL, "Logical drive is NULL");
    MUT_ASSERT_TRUE_MSG( packet_p != NULL, "packet is NULL");
    MUT_ASSERT_TRUE_MSG( io_cmd_p != NULL, "io cmd is NULL");

    payload_p = fbe_transport_get_payload_ex(packet_p);
    MUT_ASSERT_NOT_NULL(payload_p);
    block_operation_p = fbe_payload_ex_get_block_operation(payload_p);
    MUT_ASSERT_NOT_NULL(block_operation_p);

    /* First get the edge sizes for the first and last edge.
     */
    fbe_ldo_get_edge_sizes( task_p->lba, task_p->blocks,
                            task_p->exported_block_size,
                            task_p->exported_opt_block_size,
                            task_p->imported_block_size,
                            task_p->imported_opt_block_size,
                            &edge0_bytes,
                            &edge1_bytes);

    io_cmd_p = fbe_ldo_test_state_get_io_cmd(test_state_p);

    /* Get the pointer to the pre-read descriptor.
     */
    fbe_payload_block_get_pre_read_descriptor(block_operation_p, &pre_read_desc_p);

    if ( !fbe_ldo_io_cmd_is_aligned(io_cmd_p) )
    {
        /* If there are edges, then this is not aligned, and there should be a pre-read 
         * desc. 
         */
        MUT_ASSERT_NOT_NULL(pre_read_desc_p);
        fbe_payload_pre_read_descriptor_get_sg_list(pre_read_desc_p, &pre_read_sg_p);
        fbe_payload_pre_read_descriptor_get_lba(pre_read_desc_p, &pre_read_lba);
        fbe_payload_pre_read_descriptor_get_block_count(pre_read_desc_p, &pre_read_blocks);
    
        /* Change the pre-read descriptor to cause an error.
         */
        fbe_payload_pre_read_descriptor_set_lba(pre_read_desc_p, pre_read_lba + pre_read_blocks);
    
        /* This should get an error since the pre-read is not big enough.
         */
        e_status = fbe_ldo_write_setup_sgs(logical_drive_p, packet_p);
        MUT_ASSERT_TRUE(e_status == FBE_STATUS_GENERIC_FAILURE);
        MUT_ASSERT_TRUE(fbe_ldo_test_state_validate_check_words(test_state_p) == FBE_STATUS_OK);
    
        /* Restore the pre-read descriptor.
         */
        fbe_payload_pre_read_descriptor_set_lba(pre_read_desc_p, pre_read_lba);
    }
    else
    {
        /* There should not be a pre-read descriptor if there are no edges (aligned).
         * Nothing else to do for the aligned case. 
         */
        MUT_ASSERT_NULL(pre_read_desc_p);
        return FBE_STATUS_OK;
    }

    if (edge0_bytes)
    {
        /* Setup pre-read descriptor sg so the first block takes 520 sg elements.
         */
        fbe_ldo_test_setup_sg_list( fbe_ldo_test_state_get_pre_read_desc_sg(test_state_p),
                                    fbe_ldo_test_state_get_pre_read_desc_sg_memory(test_state_p),
                                    (fbe_u32_t)(task_p->exported_block_size),    /* Bytes */
                                    /* Max bytes per sg entries */
                                    (fbe_u32_t) 1 );
        fbe_ldo_test_setup_sg_list( fbe_ldo_test_state_get_pre_read_desc_sg(test_state_p) + 520,
                                    fbe_ldo_test_state_get_pre_read_desc_sg_memory(test_state_p),
                                    (fbe_u32_t)((pre_read_blocks - 1) * task_p->exported_block_size),    /* Bytes */
                                    /* Max bytes per sg entries */
                                    (fbe_u32_t)(pre_read_blocks * task_p->exported_block_size) );
    
        /* The edge0_bytes should take more than we can fit in the sg.
         */
        e_status = fbe_ldo_write_setup_sgs(logical_drive_p, packet_p);
        MUT_ASSERT_TRUE(e_status == FBE_STATUS_INSUFFICIENT_RESOURCES);
        MUT_ASSERT_TRUE(fbe_ldo_test_state_validate_check_words(test_state_p) == FBE_STATUS_OK);

        /* Restore the pre-read descriptor sg.
         */
        fbe_ldo_test_setup_sg_list( fbe_ldo_test_state_get_pre_read_desc_sg(test_state_p),
                                    fbe_ldo_test_state_get_pre_read_desc_sg_memory(test_state_p),
                                    (fbe_u32_t)(pre_read_blocks * task_p->exported_block_size),    /* Bytes */
                                    /* Max bytes per sg entries */
                                    (fbe_u32_t)(pre_read_blocks * task_p->exported_block_size) );
    }

    if (edge1_bytes)
    {
        /* There is a case where if we have edge1_bytes, but not edge0_bytes, and 
         * there is something wrong with the pre-read descriptor. 
         */
        if (edge0_bytes == 0)
        {
            /* Set pre-read descriptor sg ptr to NULL to force an error.
             */
            fbe_payload_pre_read_descriptor_set_sg_list(pre_read_desc_p, NULL);
    
            e_status = fbe_ldo_write_setup_sgs(logical_drive_p, packet_p);
            MUT_ASSERT_TRUE(e_status == FBE_STATUS_GENERIC_FAILURE);
            MUT_ASSERT_TRUE(fbe_ldo_test_state_validate_check_words(test_state_p) == FBE_STATUS_OK);
    
            /* Restore pre-read descriptor sg ptr.
             */
            fbe_payload_pre_read_descriptor_set_sg_list(pre_read_desc_p, pre_read_sg_p);
        }

        /* If there are first edge bytes,then the last edge is at the end of the pre-read 
         * sg. 
         */ 
        /* Setup pre-read descriptor sg so the last block takes 520 sg elements.
         */
        fbe_ldo_test_setup_sg_list( fbe_ldo_test_state_get_pre_read_desc_sg(test_state_p),
                                    fbe_ldo_test_state_get_pre_read_desc_sg_memory(test_state_p),
                                    (fbe_u32_t)(pre_read_blocks * task_p->exported_block_size) - 1,    /* Bytes */
                                    /* Max bytes per sg entries */
                                    (fbe_u32_t) ((pre_read_blocks - 1) * task_p->exported_block_size) );
        fbe_ldo_test_setup_sg_list( fbe_ldo_test_state_get_pre_read_desc_sg(test_state_p) + 1,
                                    fbe_ldo_test_state_get_pre_read_desc_sg_memory(test_state_p),
                                    (fbe_u32_t)(task_p->exported_block_size),    /* Bytes */
                                    /* Max bytes per sg entries */
                                    (fbe_u32_t) 1 );
        /* The edge1_bytes should take more than we can fit in the sg.
         */
        e_status = fbe_ldo_write_setup_sgs(logical_drive_p, packet_p);
        MUT_ASSERT_TRUE(e_status == FBE_STATUS_INSUFFICIENT_RESOURCES);
        MUT_ASSERT_TRUE(fbe_ldo_test_state_validate_check_words(test_state_p) == FBE_STATUS_OK);

        /* Restore the pre-read descriptor sg.
         */
        fbe_ldo_test_setup_sg_list( fbe_ldo_test_state_get_pre_read_desc_sg(test_state_p),
                                    fbe_ldo_test_state_get_pre_read_desc_sg_memory(test_state_p),
                                    (fbe_u32_t)(pre_read_blocks * task_p->exported_block_size),    /* Bytes */
                                    /* Max bytes per sg entries */
                                    (fbe_u32_t)(pre_read_blocks * task_p->exported_block_size) );
    }

    /* Call the setup function for the normal case where we expect success.
     */
    e_status = fbe_ldo_write_setup_sgs(logical_drive_p, packet_p);
    MUT_ASSERT_TRUE(e_status == FBE_STATUS_OK);
    MUT_ASSERT_TRUE(fbe_ldo_test_state_validate_check_words(test_state_p) == FBE_STATUS_OK);

    /* Destroy the state object and assert that all goes well.
     */
    MUT_ASSERT_TRUE(fbe_ldo_test_state_destroy(test_state_p) == FBE_STATUS_OK);

    return status;
}
/******************************************
 * end fbe_ldo_test_write_setup_sg()
 ******************************************/

/*!***************************************************************
 * @fn fbe_ldo_test_write_setup_sg_cases(void)
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
fbe_status_t fbe_ldo_test_write_setup_sg_cases(void)
{
    fbe_status_t status = FBE_STATUS_OK;

    /* Simply call the test function with our task and
     * the write setup sgs function. 
     */
    fbe_ldo_test_sg_cases(fbe_ldo_test_sg_tasks,
                          fbe_ldo_test_write_setup_sg );

    return status;
}
/* end fbe_ldo_test_write_setup_sg_cases() */

/*!**************************************************************
 * @fn fbe_ldo_test_add_write_unit_tests(
 *                               mut_testsuite_t * const suite_p)
 ****************************************************************
 * @brief
 *  Add the write unit tests to the input suite.
 *
 * @param suite_p - suite to add tests to.               
 *
 * @return none.
 *
 * HISTORY:
 *  11/07/2008 - Created. RPF
 *
 ****************************************************************/

void fbe_ldo_test_add_write_unit_tests(mut_testsuite_t * const suite_p)
{
    /* Just add the tests we want to run to the suite.
     */
    MUT_ADD_TEST(suite_p, 
                 fbe_ldo_test_write_count_setup_sg_cases, 
                 fbe_logical_drive_unit_tests_setup, 
                 fbe_logical_drive_unit_tests_teardown);
    MUT_ADD_TEST(suite_p, 
                 fbe_ldo_write_state_machine_tests, 
                 fbe_logical_drive_unit_tests_setup, 
                 fbe_logical_drive_unit_tests_teardown);
    MUT_ADD_TEST(suite_p, 
                 fbe_ldo_test_write_setup_sg_cases, 
                 fbe_logical_drive_unit_tests_setup, 
                 fbe_logical_drive_unit_tests_teardown);
    return;
}
/******************************************
 * end fbe_ldo_test_add_write_unit_tests() 
 ******************************************/

/*****************************************
 * end fbe_logical_drive_test_write.c
 *****************************************/
