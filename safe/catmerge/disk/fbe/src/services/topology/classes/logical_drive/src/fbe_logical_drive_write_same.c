/***************************************************************************
 * Copyright (C) EMC Corporation 2007-2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_logical_drive_write_same.c
 ***************************************************************************
 *
 * @brief
 *  This file contains functions of the logical drive object's
 *  write same state machine.
 * 
 * @ingroup logical_drive_class_files
 *
 * @version
 *   10/29/2007:  Created. RPF
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/

#include "fbe/fbe_winddk.h"
#include "fbe/fbe_logical_drive.h"
#include "fbe_logical_drive_private.h"
#include "fbe_logical_drive_io_command.h"


/****************************************
 * LOCALLY DEFINED FUNCTIONS
 ****************************************/

static void 
fbe_ldo_io_cmd_ws_setup_sub_packet(fbe_logical_drive_io_cmd_t * const io_cmd_p);



/*!**************************************************************
 * @fn fbe_ldo_handle_write_same(fbe_logical_drive_t *const logical_drive_p, 
 *                               fbe_packet_t *const packet_p)
 ****************************************************************
 * @brief
 *  This function is the entry point for write same
 *  operations to the logical drive object.
 *
 * @param logical_drive_p - The logical drive.
 * @param packet_p - The write same io packet that is arriving.
 *
 * @return fbe_status_t - FBE_STATUS_OK if no error.
 *                        FBE_STATUS_GENERIC_FAILURE on error.
 *
 * @author
 *  10/9/2008 - Created. RPF
 *
 ****************************************************************/

fbe_status_t
fbe_ldo_handle_write_same(fbe_logical_drive_t *const logical_drive_p, 
                          fbe_packet_t *const packet_p)
{
    fbe_lba_t lba;
    fbe_block_count_t blocks;
    fbe_block_size_t exported_block_size;
    fbe_block_size_t exported_opt_block_size;
    fbe_block_size_t imported_block_size;
    fbe_block_size_t imported_opt_block_size;
    fbe_lba_t imported_lba;
    fbe_block_count_t imported_blocks;
    fbe_payload_block_operation_opcode_t opcode;
	fbe_block_edge_t * block_edge = NULL;

    fbe_payload_ex_t *payload_p = fbe_transport_get_payload_ex(packet_p);
    fbe_payload_block_operation_t *block_operation_p = fbe_payload_ex_get_block_operation(payload_p);

    /* Get the block sizes, lba and blocks from the payload. Also calculate the
     * imported optimum size. 
     */
    fbe_payload_block_get_lba(block_operation_p, &lba);
    fbe_payload_block_get_block_count(block_operation_p, &blocks);
    fbe_payload_block_get_block_size(block_operation_p, &exported_block_size);

	block_edge = (fbe_block_edge_t *)fbe_transport_get_edge(packet_p);

    if (block_edge == NULL)
    {
        fbe_payload_block_get_optimum_block_size(block_operation_p, &exported_opt_block_size);
        fbe_block_transport_edge_get_exported_block_size(&logical_drive_p->block_edge, &imported_block_size);
    }
    else
    {
	fbe_block_transport_edge_get_optimum_block_size(block_edge, &exported_opt_block_size);
        imported_block_size = fbe_ldo_get_imported_block_size(logical_drive_p);
    }

    fbe_payload_block_get_opcode(block_operation_p, &opcode);

    if (exported_opt_block_size == 0)
    {
        /* Something is wrong if the optimum block size is zero.
         */
        fbe_base_object_trace((fbe_base_object_t*)logical_drive_p, FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                              "logical drive: unexpected ws opt block sizes req:0x%x opt:0x%x Geometry:0x%x\n", 
                              exported_block_size, exported_opt_block_size,
                              logical_drive_p->block_edge.block_edge_geometry);
        /* Transport status on the packet is OK since it was received OK. 
         * The payload status is set to bad since this command is not formed 
         * correctly. 
         */
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_ldo_set_payload_status(packet_p, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID_REQUEST,
                                   FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_INVALID_BLOCK_SIZE);
        fbe_ldo_complete_packet(packet_p);
        return FBE_STATUS_PENDING;
    }

    imported_opt_block_size = 
        fbe_ldo_calculate_imported_optimum_block_size(exported_block_size,
                                                      exported_opt_block_size,
                                                      imported_block_size);

    /* We only support aligned write same commands.
     * Unaligned write same commands would require pre-reads, and
     * should be performed by the caller as a write.
     */
    if (fbe_ldo_io_is_aligned( lba, blocks, exported_opt_block_size ) == FBE_FALSE)
    {
        fbe_base_object_trace((fbe_base_object_t*)logical_drive_p, 
                              FBE_TRACE_LEVEL_WARNING, 
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "LDO: Write same command unaligned. Lba: 0x%llx Blocks: 0x%llx Block Size: 0x%x\n",
                              (unsigned long long)lba,
			      (unsigned long long)blocks,
			      exported_opt_block_size );

        /* The write same operation is invalid.
         * Simply set status and transition to finished.
         */
        fbe_transport_set_status(packet_p,
                                  FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_ldo_set_payload_status(packet_p,
                                   FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID_REQUEST,
                                   FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_INVALID);
        fbe_ldo_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Determine the mapped lba and block count for
     * the given io command.
     */
    fbe_ldo_map_lba_blocks(lba, blocks, 
                           exported_block_size, exported_opt_block_size,
                           imported_block_size, imported_opt_block_size,
                           &imported_lba, &imported_blocks);

    /* Then initialize our packet for a write same.
     * Note that we pass through the same opcode that we received, 
     * this allows us to handle different opcodes as a write same. 
     *  
     * Note that the repeat count is set to the optimum block size. 
     * This is because the minimum we can write same is the optimum block size 
     * and the data we are sending down is a single optimum block. 
     */
    fbe_ldo_build_packet(packet_p,
                         opcode,
                         imported_lba,
                         imported_blocks,
                         fbe_ldo_get_imported_block_size(logical_drive_p),
                         imported_opt_block_size, /* imported optimal. */
                         NULL, /* Don't touch the sg ptr. */
                         fbe_ldo_packet_completion, /* completion fn */
                         logical_drive_p,    /* completion context */
                          /* repeat count */ 
                         exported_opt_block_size);

    /* Send out the io packet.  Our callback always gets executed, thus
     * there is no status to handle here.
     */
    fbe_ldo_send_io_packet(logical_drive_p, packet_p);
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_ldo_handle_write_same()
 ******************************************/

/*!***************************************************************
 * @fn fbe_ldo_io_cmd_write_same_state0(fbe_logical_drive_io_cmd_t * const io_cmd_p)
 ****************************************************************
 * @brief
 *  This function handle the first state of the write 
 *  same state machine.
 * 
 *  We come through this state only when we are performing
 *  a "lossy" style of write that requires an sg list.
 *
 *  @param io_cmd_p - The logical drive io cmd object to execute.
 *
 * @return
 *  FBE_LDO_STATE_STATUS_EXECUTING - If the command is not valid.
 *  FBE_LDO_STATE_STATUS_WAITING - If we're waiting for the io packet
 *                                 to be allocated.
 *
 * @author
 *  10/22/07 - Created. RPF
 *
 ****************************************************************/
fbe_ldo_state_status_enum_t 
fbe_ldo_io_cmd_write_same_state0(fbe_logical_drive_io_cmd_t * const io_cmd_p)
{
    /* Validate the io packet for this write same command.
     */
    if (fbe_ldo_io_cmd_validate_write_same(io_cmd_p) != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*)
                              fbe_ldo_io_cmd_get_logical_drive(io_cmd_p), 
                              FBE_TRACE_LEVEL_INFO, 
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "LDO: Invalid write same operation.  %s \n", 
                              __FUNCTION__);

        /* The write same operation is invalid.
         * Simply set status and transition to finished.
         */
        fbe_transport_set_status(fbe_ldo_io_cmd_get_packet(io_cmd_p),
                                  FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_ldo_set_payload_status(fbe_ldo_io_cmd_get_packet(io_cmd_p),
                                   FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID_REQUEST,
                                   FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_INVALID_SG);
        fbe_ldo_io_cmd_set_state(io_cmd_p, fbe_ldo_io_cmd_finished);
    }
    else
    {
        /* Transition to the next state to fill out the packet.
         */
        fbe_ldo_io_cmd_set_state(io_cmd_p, fbe_ldo_io_cmd_write_same_state10);
    }
    return FBE_LDO_STATE_STATUS_EXECUTING;  
}
/* end fbe_ldo_io_cmd_write_same_state0() */

/*!***************************************************************
 * @fn fbe_ldo_io_cmd_write_same_state10(fbe_logical_drive_io_cmd_t * const io_cmd_p)
 ****************************************************************
 * @brief
 *  This function will coordinate allocation of sgs for this
 *  request.
 *
 *  @param io_cmd_p - The logical drive io cmd object to execute.
 *
 * @return
 *  fbe_ldo_state_status_enum_t
 *
 * @author
 *  01/29/08 - Created. RPF
 *
 ****************************************************************/
fbe_ldo_state_status_enum_t 
fbe_ldo_io_cmd_write_same_state10(fbe_logical_drive_io_cmd_t * const io_cmd_p)
{
    /* The callback might get executed which could cause our 
     * next state to get triggered.
     */
    fbe_ldo_io_cmd_set_state(io_cmd_p, fbe_ldo_io_cmd_write_same_state20);

    /* Fill out the sg list for this command.
     */
    fbe_ldo_io_cmd_write_same_setup_sg(io_cmd_p);

    /* Initialize the io packet for this write same command.
     */
    fbe_ldo_io_cmd_ws_setup_sub_packet(io_cmd_p);

    /* Send out the io packet.  Our callback always gets executed, thus
     * there is no status to handle here.
     */
    fbe_ldo_io_cmd_send_io(io_cmd_p);

    return FBE_LDO_STATE_STATUS_WAITING;
}
/* end fbe_ldo_io_cmd_write_same_state10() */

/*!***************************************************************
 * @fn fbe_ldo_io_cmd_write_same_state20(fbe_logical_drive_io_cmd_t * const io_cmd_p)
 ****************************************************************
 * @brief
 *  This function initializes our sg, and io packet and sends out
 *  the sub io packet for the write same.
 *
 *  @param io_cmd_p - The logical drive io cmd object to execute.
 *
 * @return
 *  FBE_LDO_STATE_STATUS_EXECUTING.
 *
 * @author
 *  01/29/08 - Created. RPF
 *
 ****************************************************************/
fbe_ldo_state_status_enum_t 
fbe_ldo_io_cmd_write_same_state20(fbe_logical_drive_io_cmd_t * const io_cmd_p)
{
    /* Transition to the finished state.
     */
    fbe_ldo_io_cmd_set_state(io_cmd_p, fbe_ldo_io_cmd_finished);

    /* The operation has completed, set the status in the master io packet
     * from the status in the sub io packet.
     */
    fbe_ldo_io_cmd_set_status_from_sub_io_packet(io_cmd_p);

    /* Return executing, so we can execute the finished state.
     */
    return FBE_LDO_STATE_STATUS_EXECUTING;
}
/* end fbe_ldo_io_cmd_write_same_state20() */

/*!***************************************************************
 * @fn fbe_ldo_io_cmd_write_same_state30(fbe_logical_drive_io_cmd_t * const io_cmd_p)
 ****************************************************************
 * @brief
 *  This function handles returning an error on this io cmd.
 *
 *  @param io_cmd_p - The logical drive io cmd object to execute.
 *
 * @return
 *  Always returns FBE_LDO_STATE_STATUS_EXECUTING.
 *
 * @author
 *  01/29/08 - Created. RPF
 *
 ****************************************************************/
fbe_ldo_state_status_enum_t 
fbe_ldo_io_cmd_write_same_state30(fbe_logical_drive_io_cmd_t * const io_cmd_p)
{
    /* Transition to the finished state.
     */
    fbe_ldo_io_cmd_set_state(io_cmd_p, fbe_ldo_io_cmd_finished);

    /* We hit an error on this command, set error now.
     */
    fbe_ldo_io_cmd_set_status(io_cmd_p, FBE_STATUS_GENERIC_FAILURE, 0);

    /* Return executing, so we can execute the finished state.
     */
    return FBE_LDO_STATE_STATUS_EXECUTING;
}
/* end fbe_ldo_io_cmd_write_same_state30() */

/*!***************************************************************
 * fbe_ldo_io_cmd_validate_write_same(fbe_logical_drive_io_cmd_t * const io_cmd_p)
 ****************************************************************
 * @brief
 *  This function handles validation of the write same command.
 *
 *  We need to validate that the sg list supplied is valid.
 *  It must contain at least one exported block worth of data.
 *
 *  We need to validate the write same since we only allow
 *  write same commands that are aligned to the
 *  optimal block size.
 *
 * @param io_cmd_p - The logical drive io cmd object to execute.
 *
 * @return
 *  FBE_STATUS_OK - If command is valid.
 *  FBE_STATUS_GENERIC_FAILURE - If command not valid.
 *
 * @author
 *  01/29/08 - Created. RPF
 *
 ****************************************************************/

fbe_status_t
fbe_ldo_io_cmd_validate_write_same(fbe_logical_drive_io_cmd_t * const io_cmd_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_bool_t b_valid = FBE_TRUE;
    fbe_block_size_t exported_block_size;
    exported_block_size = fbe_ldo_io_cmd_get_block_size(io_cmd_p);

    /* We only support aligned write same commands.
     * Unaligned write same commands would require pre-reads, and
     * should be performed by the caller as a write.
     */
    if (fbe_ldo_io_cmd_is_aligned(io_cmd_p) == FBE_FALSE)
    {
        fbe_base_object_trace((fbe_base_object_t*)
                              fbe_ldo_io_cmd_get_logical_drive(io_cmd_p), 
                              FBE_TRACE_LEVEL_INFO, 
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "LDO: Write same command unaligned. Lba: 0x%llx Blocks: 0x%llx Size: 0x%llx\n",
                              (unsigned long long)fbe_ldo_io_cmd_get_lba(io_cmd_p),
                              (unsigned long long)fbe_ldo_io_cmd_get_blocks(io_cmd_p),
                              (unsigned long long)(fbe_ldo_io_cmd_get_blocks(io_cmd_p) * exported_block_size) );
        b_valid = FBE_FALSE;
    }

    /* Validate the sg list supplied.
     */
    if (fbe_ldo_io_cmd_get_client_sg_ptr(io_cmd_p) == NULL)
    {
        /* Sg ptr not valid.  We can't do a write same if the
         * client does not supply an sg ptr.
         */
        fbe_base_object_trace((fbe_base_object_t*)
                              fbe_ldo_io_cmd_get_logical_drive(io_cmd_p), 
                              FBE_TRACE_LEVEL_INFO, 
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "LDO: Write same sg list NULL.\n");
        b_valid = FBE_FALSE;
    }
    else
    {
        fbe_u32_t sg_list_bytes;

        /* Also make sure the caller sent in one and only one exported block.
         */
        sg_list_bytes = 
            fbe_sg_element_count_list_bytes(fbe_ldo_io_cmd_get_client_sg_ptr(io_cmd_p));

        if (exported_block_size != sg_list_bytes)
        {
            /* Client sent in something that is not a multiple
             * of an exported block.
             */
            b_valid = FBE_FALSE;
            fbe_base_object_trace((fbe_base_object_t*)
                                  fbe_ldo_io_cmd_get_logical_drive(io_cmd_p), 
                                  FBE_TRACE_LEVEL_INFO, 
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "LDO: Write same sg list length invalid. "
                                  "Length: 0x%x Expected: 0x%x\n",
                                  sg_list_bytes, exported_block_size);
        }
    } /* end if sg ptr not NULL */

    if (b_valid)
    {
        /* Command checked out OK.
         */
        status = FBE_STATUS_OK;
    }
    else
    {
        /* Something invalid about command.
         */
        status = FBE_STATUS_GENERIC_FAILURE;
    }

    return status;
}
/* end fbe_ldo_io_cmd_validate_write_same() */

/*!***************************************************************
 * @fn fbe_ldo_io_cmd_ws_setup_sub_packet(fbe_logical_drive_io_cmd_t * const io_cmd_p)
 ****************************************************************
 * @brief
 *  This function initializes our sg, and io packet and sends out
 *  the sub io packet for the write same.
 *
 * @param io_cmd_p - The logical drive io cmd object to execute.
 *
 * @return
 *  None.
 *
 * @author
 *  02/07/08 - Created. RPF
 *
 ****************************************************************/
static void 
fbe_ldo_io_cmd_ws_setup_sub_packet(fbe_logical_drive_io_cmd_t * const io_cmd_p)
{
    fbe_logical_drive_t * logical_drive_p = fbe_ldo_io_cmd_get_logical_drive( io_cmd_p );
    fbe_lba_t current_lba;
    fbe_block_count_t current_blocks;
    fbe_payload_ex_t *payload_p = NULL;
    fbe_payload_block_operation_t *block_operation_p = NULL;
    fbe_packet_t *packet_p = fbe_ldo_io_cmd_get_packet(io_cmd_p);
    fbe_payload_block_operation_opcode_t opcode;

    /* Simply map the given parameters from the io command from
     * the exported to the imported block size.
     */
    fbe_ldo_map_lba_blocks(fbe_ldo_io_cmd_get_lba(io_cmd_p),
                           fbe_ldo_io_cmd_get_blocks (io_cmd_p),
                           fbe_ldo_io_cmd_get_block_size(io_cmd_p),
                           fbe_ldo_io_cmd_get_optimum_block_size(io_cmd_p),
                           fbe_ldo_get_imported_block_size(logical_drive_p),
                           fbe_ldo_io_cmd_get_imported_optimum_block_size(io_cmd_p),
                           &current_lba,
                           &current_blocks);

    /* Determine the opcode so we can use this below to setup the next payload.
     */
    payload_p = fbe_transport_get_payload_ex(packet_p);
    block_operation_p = fbe_payload_ex_get_block_operation(payload_p);
    fbe_payload_block_get_opcode(block_operation_p, &opcode);

    fbe_payload_ex_set_sg_list(payload_p, 
                            fbe_ldo_io_cmd_get_sg_ptr(io_cmd_p),
                            0);
    /* Then initialize our packet for a write same.
     * Note that we pass through the same opcode that we received, 
     * this allows us to handle different opcodes as a write same. 
     *  
     * Note that the repeat count is set to the optimum block size. 
     * This is because the minimum we can write same is the optimum block size 
     * and the data we are sending down is a single optimum block. 
     */
    fbe_ldo_build_packet(packet_p,
                         opcode,
                         current_lba,
                         current_blocks,
                         fbe_ldo_get_imported_block_size(logical_drive_p),
                         fbe_ldo_io_cmd_get_imported_optimum_block_size(io_cmd_p),
                         NULL, /* Don't touch the sg ptr. */
                         fbe_ldo_io_cmd_packet_completion, /* completion fn */
                         io_cmd_p,    /* completion context */
                         1 /* repeat count */ );
    return;
}
/* end fbe_ldo_io_cmd_ws_setup_sub_packet() */

/*!**************************************************************
 * @fn fbe_ldo_io_cmd_write_same_setup_sg(
 *                    fbe_logical_drive_io_cmd_t * const io_cmd_p)
 ****************************************************************
 * @brief
 *  Initialize the sg list for a write same command.
 *  
 * @param io_cmd_p - The io command tracking this operation.
 *
 * @return - None.
 *
 * @author
 *  10/31/2008 - Created. RPF
 *
 ****************************************************************/

void fbe_ldo_io_cmd_write_same_setup_sg(fbe_logical_drive_io_cmd_t * const io_cmd_p)
{
    fbe_sg_element_t *client_sg_p = fbe_ldo_io_cmd_get_client_sg_ptr(io_cmd_p);
    fbe_sg_element_t *sg_p = fbe_ldo_io_cmd_get_sg_ptr(io_cmd_p);
    fbe_u32_t bytes_to_copy;

    /* If padding is needed, then we need to create an sg list with an 
     * entire optimal block's worth of data. 
     */
    fbe_u32_t pad_bytes = fbe_ldo_calculate_pad_bytes(io_cmd_p);
    fbe_u32_t current_block;
    fbe_block_size_t exported_opt_blk_size = fbe_ldo_io_cmd_get_optimum_block_size(io_cmd_p);

    /* The number of bytes to copy for each entry is one block.
     */ 
    bytes_to_copy = (fbe_u32_t) fbe_ldo_io_cmd_get_block_size(io_cmd_p);

    /* The sg has only one block in it.
     * Repeat this block for all the blocks in the optimum block.
     */
    for (current_block = 0; 
        current_block < exported_opt_blk_size; 
        current_block ++)
    {
        sg_p = fbe_sg_element_copy_list(client_sg_p,    /* source. */
                                        sg_p,    /* destination */
                                        bytes_to_copy    /* bytes to copy */);
    }    /* end for all blocks in optimal block */

    if (pad_bytes > 0)
    {
        /* Next add the pad after the user data.
         */
        sg_p = fbe_ldo_fill_sg_with_bitbucket(sg_p, pad_bytes,
                                              fbe_ldo_get_write_bitbucket_ptr());
    }

    /* Terminate the sg list.
     */
    fbe_sg_element_terminate(sg_p);

    return;
}
/**************************************
 * end fbe_ldo_io_cmd_write_same_setup_sg()
 **************************************/

/************************************************************
 * end file fbe_logical_drive_write_same.c
 ***********************************************************/
