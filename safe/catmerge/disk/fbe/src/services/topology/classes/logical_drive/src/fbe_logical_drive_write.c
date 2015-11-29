/***************************************************************************
 * Copyright (C) EMC Corporation 2007-2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_logical_drive_write.c
 ***************************************************************************
 *
 * @brief
 *  This file contains functions of the logical drive object's
 *  write state machine.
 *
 * HISTORY
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

/*!**************************************************************
 * @fn fbe_ldo_handle_write(fbe_logical_drive_t *logical_drive_p, 
 *                          fbe_packet_t * packet_p)
 ****************************************************************
 * @brief
 *  This function is the entry point for write
 *  operations to the logical drive object.
 *
 * @param logical_drive_p - The logical drive.
 * @param packet_p - The write io packet that is arriving.
 *
 * @return fbe_status_t - FBE_STATUS_OK.
 *
 * HISTORY:
 *  10/9/2008 - Created. RPF
 *
 ****************************************************************/

fbe_status_t
fbe_ldo_handle_write(fbe_logical_drive_t *logical_drive_p, 
                     fbe_packet_t * packet_p)
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

    /* Get the lba, blocks, block sizes and opcode from the payload.
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
                              "logical drive: unexpected write opt block sizes req:0x%x opt:0x%x Geometry:0x%x\n", 
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

    /* The imported optimal block size needs to be derived from the requested 
     * exported sizes and the imported block size. 
     */
    imported_opt_block_size = 
        fbe_ldo_calculate_imported_optimum_block_size(exported_block_size,
                                                      exported_opt_block_size,
                                                      imported_block_size);
    /* Determine the mapped lba and block count for
     * the given io command.
     */
    fbe_ldo_map_lba_blocks(lba, blocks, 
                           exported_block_size, exported_opt_block_size,
                           imported_block_size, imported_opt_block_size,
                           &imported_lba, &imported_blocks);

    if (!fbe_ldo_io_is_aligned( lba, blocks, exported_opt_block_size ))
    {
        fbe_status_t status;
        fbe_payload_pre_read_descriptor_t *pre_read_desc_p = NULL;

        fbe_payload_block_get_pre_read_descriptor(block_operation_p, &pre_read_desc_p);

        /* If we are not aligned, we have a pre-read descriptor.
         * Make sure it is sane. 
         */
        status = fbe_ldo_validate_pre_read_desc(pre_read_desc_p, exported_block_size,
                                                exported_opt_block_size, imported_block_size,
                                                imported_opt_block_size,
                                                lba, blocks, imported_lba, imported_blocks);
        if (status != FBE_STATUS_OK)
        {
            /* The pre-read descriptor is invalid. Simply set the status and
             * transition to the finished state. 
             */
            fbe_base_object_trace((fbe_base_object_t*) logical_drive_p,
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "LDO: Invalid edge descriptor found.  0x%x:%s\n",
                                __LINE__,__FUNCTION__);
            fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
            fbe_payload_block_set_status(block_operation_p,
                                         FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID_REQUEST,
                                         FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_INVALID_PRE_READ_DESCRIPTOR);
            fbe_ldo_complete_packet(packet_p);
            return FBE_STATUS_OK;
        } /* end pre-read descriptor not valid */

        /* Fill in the sgs with pre-read data.
         */
        status = fbe_ldo_write_setup_sgs(logical_drive_p, packet_p);
        if (status != FBE_STATUS_OK)
        {
            /* Issues using the sgs in the packet, just try to allocate our 
             * context and allocate an sg to handle this. 
             */
            fbe_ldo_io_cmd_allocate(packet_p,
                                    (fbe_memory_completion_function_t)fbe_ldo_io_cmd_allocate_memory_callback, /* SAFEBUG - cast to supress warning but must fix because memory completion function shouldn't return status */
                                    logical_drive_p);
            return FBE_STATUS_OK;
        }
    } /* if not aligned and thus we have a pre-read descriptor */

    /* Then initialize our packet for a write.
     * Note that we pass through the same opcode that we received, 
     * this allows us to handle different opcodes as a write. For example, we 
     * want to handle the write verify opcode the same as a write opcode.
     */
    fbe_ldo_build_packet(packet_p,
                         opcode,
                         imported_lba,
                         imported_blocks,
                         fbe_ldo_get_imported_block_size(logical_drive_p),
                         1, /* imported optimal. */
                         NULL, /* Don't touch the sg ptr. */
                         fbe_ldo_packet_completion, /* completion fn */
                         logical_drive_p,    /* completion context */
                         1 /* repeat count */ ); 

    /* Send out the io packet.  Our callback always gets executed, thus
     * there is no status to handle here.
     */
    fbe_ldo_send_io_packet(logical_drive_p, packet_p);
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_ldo_handle_write()
 ******************************************/

/*!***************************************************************
 * @fn fbe_ldo_io_cmd_write_state0(
 *                     fbe_logical_drive_io_cmd_t * const io_cmd_p)
 ****************************************************************
 * @brief
 *  This function handles the first state of the write state machine.
 *
 *  We will first allocate an IoPacket.
 *
 * @param io_cmd_p - The logical drive io cmd object to execute.
 *
 * @return
 *  fbe_ldo_state_status_enum_t
 *
 * HISTORY:
 *  11/12/07 - Created. RPF
 *
 ****************************************************************/
fbe_ldo_state_status_enum_t 
fbe_ldo_io_cmd_write_state0(fbe_logical_drive_io_cmd_t * const io_cmd_p)
{
    /* Validate the edge descriptor for this io command.
     */
    if (fbe_ldo_io_cmd_validate_pre_read_desc(io_cmd_p) != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*) fbe_ldo_io_cmd_get_logical_drive(io_cmd_p),
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "LDO: Invalid edge descriptor found. 0x%x:%s\n",
                              __LINE__, __FUNCTION__);
        /* The pre-read descriptor is invalid. Simply set the status and
         * transition to the finished state. 
         */
        fbe_transport_set_status(fbe_ldo_io_cmd_get_packet(io_cmd_p),
                                  FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_ldo_set_payload_status(fbe_ldo_io_cmd_get_packet(io_cmd_p),
                                   FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID_REQUEST,
                                   FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_INVALID_PRE_READ_DESCRIPTOR);
        fbe_ldo_io_cmd_set_state(io_cmd_p, fbe_ldo_io_cmd_finished);
    }
    else
    {
        /* Transition to the next state before trying to get the buffer.
         * The callback might get executed which could cause our 
         * next state to get triggered.
         */
        fbe_ldo_io_cmd_set_state(io_cmd_p, fbe_ldo_io_cmd_write_state10);
    }
    return FBE_LDO_STATE_STATUS_EXECUTING;
}
/* end fbe_ldo_io_cmd_write_state0() */

/*!***************************************************************
 * @fn fbe_ldo_io_cmd_write_state10(
 *                     fbe_logical_drive_io_cmd_t * const io_cmd_p)
 ****************************************************************
 * @brief
 *  This function will coordinate allocation of sgs for this
 *  request.
 *
 * @param io_cmd_p - The logical drive io cmd object to execute.
 *
 * @return
 *  fbe_ldo_state_status_enum_t
 *
 * HISTORY:
 *  10/22/07 - Created. RPF
 *
 ****************************************************************/
fbe_ldo_state_status_enum_t 
fbe_ldo_io_cmd_write_state10(fbe_logical_drive_io_cmd_t * const io_cmd_p)
{
    fbe_bool_t b_padding_required = fbe_ldo_io_cmd_is_padding_required(io_cmd_p);

    if (b_padding_required)
    {
       /* When padding is required, we always need to alloc an sg.
        * Transition to the state for allocating an sg.
        */
        fbe_ldo_io_cmd_set_state(io_cmd_p, fbe_ldo_io_cmd_write_state11);
    }
    else if (fbe_ldo_io_cmd_is_aligned(io_cmd_p) == FALSE)
    {
        /* Transition to the state for allocating an sg.
         */
        fbe_ldo_io_cmd_set_state(io_cmd_p, fbe_ldo_io_cmd_write_state11);
    }
    else
    {
        /* We are aligned and thus we do not need to allocate an sg.
         * Instead we will use the sg passed to us in the io packet.
         */
        fbe_ldo_io_cmd_set_state(io_cmd_p, fbe_ldo_io_cmd_write_state20);
    }
    return FBE_LDO_STATE_STATUS_EXECUTING;
}
/* end fbe_ldo_io_cmd_write_state10() */

/*!***************************************************************
 * @fn fbe_ldo_io_cmd_write_state11(
 *                     fbe_logical_drive_io_cmd_t * const io_cmd_p)
 ****************************************************************
 * @brief
 *  This function will coordinate allocation of sgs for this
 *  request.
 *
 * @param io_cmd_p - The logical drive io cmd object to execute.
 *
 * @return
 *  fbe_ldo_state_status_enum_t
 *
 * HISTORY:
 *  10/22/07 - Created. RPF
 *
 ****************************************************************/
fbe_ldo_state_status_enum_t 
fbe_ldo_io_cmd_write_state11(fbe_logical_drive_io_cmd_t * const io_cmd_p)
{
    fbe_u32_t sg_entries;

    /* Transition to the next state before trying to get the sg.
     * The callback might get executed which could cause our 
     * next state to get triggered.
     */
    fbe_ldo_io_cmd_set_state(io_cmd_p, fbe_ldo_io_cmd_write_state20);

    /* If we are not aligned, then we will need to allocate an sg.
     */
    sg_entries = fbe_ldo_io_cmd_wr_count_sg_entries(io_cmd_p);

    if (sg_entries == 0)
    {
        /* Something is wrong with the input sg.
         * Transition to the final state.
         */
        fbe_transport_set_status(fbe_ldo_io_cmd_get_packet(io_cmd_p),
                                  FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_ldo_io_cmd_set_state(io_cmd_p, fbe_ldo_io_cmd_finished);
    }
    else if (sg_entries > FBE_LDO_MAX_SG_LENGTH)
    {
        /* The request will not fit into our sg list.
         * The max sg list size needs to account for 
         * our max size of request. 
         */
        fbe_transport_set_status(fbe_ldo_io_cmd_get_packet(io_cmd_p),
                                  FBE_STATUS_GENERIC_FAILURE, __LINE__);
        fbe_ldo_io_cmd_set_state(io_cmd_p, fbe_ldo_io_cmd_finished);
        fbe_base_object_trace((fbe_base_object_t*)
                              fbe_ldo_io_cmd_get_logical_drive(io_cmd_p), 
                              FBE_TRACE_LEVEL_ERROR, 
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "LDO: sg required for this write is too large.  %s entries:%d\n", 
                              __FUNCTION__,  sg_entries);
    }
    else
    {
        /* Save this value in the io cmd for later use.
         */
        fbe_ldo_io_cmd_set_sg_entries(io_cmd_p, sg_entries);
    }
    return FBE_LDO_STATE_STATUS_EXECUTING;
}
/* end fbe_ldo_io_cmd_write_state11() */

/*!***************************************************************
 * @fn fbe_ldo_io_cmd_write_state20(
 *                     fbe_logical_drive_io_cmd_t * const io_cmd_p)
 ****************************************************************
 * @brief
 *  This function initializes our sg, and io packet and sends out
 *  the sub io packet for the write.
 *
 * @param io_cmd_p - The logical drive io cmd object to execute.
 *
 * @return
 *  FBE_LDO_STATE_STATUS_WAITING - 
 *  We always wait for the next state to get executed.
 *
 * HISTORY:
 *  11/07/07 - Created. RPF
 *
 ****************************************************************/
fbe_ldo_state_status_enum_t 
fbe_ldo_io_cmd_write_state20(fbe_logical_drive_io_cmd_t * const io_cmd_p)
{
    fbe_lba_t imported_lba;
    fbe_block_count_t imported_blocks;
    /* Determine the mapped lba and block count for
     * the given io command.
     */
    fbe_ldo_io_cmd_map_lba_blocks( io_cmd_p, &imported_lba, &imported_blocks );
    
    /* Transition to the next state before trying to send out the io.
     * Upon sending the io packet, the callback might get executed 
     * which could cause our next state to get triggered.
     */
    fbe_ldo_io_cmd_set_state(io_cmd_p, fbe_ldo_io_cmd_write_state30);

    /* First, if we needed to allocate an sg list, then
     * initialize our newly allocated sg list.
     */
    if (fbe_ldo_io_cmd_get_sg_entries(io_cmd_p) != 0)
    {
        fbe_status_t status = fbe_ldo_io_cmd_wr_setup_sg(io_cmd_p);

        if (status != FBE_STATUS_OK)
        {
            /* Something went wrong with setting up the sg.
             * Return error now.
             */
            fbe_transport_set_status(fbe_ldo_io_cmd_get_packet(io_cmd_p),
                                      FBE_STATUS_GENERIC_FAILURE, 0);
            fbe_ldo_io_cmd_set_state(io_cmd_p, fbe_ldo_io_cmd_finished);
            return FBE_LDO_STATE_STATUS_EXECUTING;
        }
    }
    
    /* Then initialize our io packet.
     */
    fbe_ldo_io_cmd_build_sub_packet(io_cmd_p,
                                    FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,
                                    imported_lba,
                                    imported_blocks,
                                    1 /* imported optimal. */);

    /* Send out the io packet.  Our callback always gets executed, thus
     * there is no status to handle here.
     */
    fbe_ldo_io_cmd_send_io(io_cmd_p);

    return FBE_LDO_STATE_STATUS_WAITING;
}
/* end fbe_ldo_io_cmd_write_state20() */

/*!***************************************************************
 * @fn fbe_ldo_io_cmd_write_state30(
 *                     fbe_logical_drive_io_cmd_t * const io_cmd_p)
 ****************************************************************
 * @brief
 *  This function handles the completion of the io packet.
 *  We simply set status in the master io packet
 *  from the sub io packet.
 *
 * @param io_cmd_p - The logical drive io cmd object to execute.
 *
 * @return
 *  FBE_LDO_STATE_STATUS_EXECUTING
 *
 * HISTORY:
 *  11/07/07 - Created. RPF
 *
 ****************************************************************/
fbe_ldo_state_status_enum_t 
fbe_ldo_io_cmd_write_state30(fbe_logical_drive_io_cmd_t * const io_cmd_p)
{
    /* Transition to the finished state.
     */
    fbe_ldo_io_cmd_set_state(io_cmd_p, fbe_ldo_io_cmd_finished);

    /* The operation has completed, set the status in the master io packet
     * from the status in the sub io packet.
     */
    fbe_ldo_io_cmd_set_status_from_sub_io_packet(io_cmd_p);
    
    /* Return done so that we stop executing this command.
     */
    return FBE_LDO_STATE_STATUS_EXECUTING;
}
/* end fbe_ldo_io_cmd_write_state30() */

/*!**************************************************************
 * @fn fbe_ldo_io_cmd_wr_get_edge1_fill_sizes(
 *                     fbe_logical_drive_io_cmd_t * const io_cmd_p,
 *                     fbe_u32_t edge1_bytes,
 *                     fbe_u32_t *const client_edge_bytes,
 *                     fbe_u32_t *const pad_bytes)
 ****************************************************************
 * @brief
 *  Determine the sizes of the client edge bytes and the pad required
 *  for a write operation.
 * 
 * @param io_cmd_p - The logical drive's io cmd for this operation.
 * @param edge1_bytes - The first edge byte count.
 * @param client_edge_bytes - The bytes in the edge that need to be
 *                            copied from client pre-read data.
 * @param pad_bytes - The number of bytes in this edge that are pad.
 *
 * @return - None.
 *
 * HISTORY:
 *  9/15/2008 - Created. RPF
 *
 ****************************************************************/

void fbe_ldo_io_cmd_wr_get_edge1_fill_sizes(fbe_logical_drive_io_cmd_t * const io_cmd_p,
                                            fbe_u32_t edge1_bytes,
                                            fbe_u32_t *const client_edge_bytes,
                                            fbe_u32_t *const pad_bytes)
{
    fbe_lba_t lba = fbe_ldo_io_cmd_get_lba( io_cmd_p );
    fbe_block_count_t blocks = fbe_ldo_io_cmd_get_blocks( io_cmd_p );
    fbe_block_size_t exported_block_size = fbe_ldo_io_cmd_get_block_size(io_cmd_p);
    fbe_block_size_t exported_opt_block_size = fbe_ldo_io_cmd_get_optimum_block_size(io_cmd_p);

    /* Assuming the end of the I/O is not aligned to the
     * optimal block boundary, then the edge bytes that we
     * get from client pre-read is imported_block_size.
     *  
     * Example where exported block size > imported block size 
     *  
     *                              Optimal block boundary
     *   Start of host I/O           |
     *  \|/                         \|/
     *   |ioioioioioioio|------------| <--- Exported
     *   |--------|----------|----------| <--- Imported
     *                 /|\          /|\    
     *   2nd edge start |            | pad start
     *  
     * Example where exported block size < imported block size
     * In this case the edge contains both host and client data
     *                              Optimal block boundary
     *   Start of host I/O           |
     *  \|/                         \|/
     *   |ioioioioioioio|------------| <--- Exported
     *   |---------------------------------------| <--- Imported
     *                 /|\   /|\    /|\    
     *   2nd edge start |     |      | pad start
     *                        |
     *                     Host data copied for this region.
     */
    if ((lba + blocks) % exported_opt_block_size)
    {
        fbe_u32_t bytes_to_opt_block_boundary = 
            (fbe_u32_t)((exported_opt_block_size - ((lba + blocks) % exported_opt_block_size)) *
                        exported_block_size);

        /* We are not aligned to the optimal block size, 
         * thus we have some client data to copy also. 
         */
        *client_edge_bytes = FBE_MIN(bytes_to_opt_block_boundary, edge1_bytes);
        *pad_bytes = edge1_bytes - *client_edge_bytes;
    }
    else
    {
        /* Aligned to the optimal block sizes, so there is no client pre-read 
         * data to copy, we only have pad bytes to copy. 
         */
        *client_edge_bytes = 0;
        *pad_bytes = edge1_bytes;
    }
    return;
}
/******************************************
 * end fbe_ldo_io_cmd_wr_get_edge1_fill_sizes()
 ******************************************/

/*!***************************************************************
 * @fn fbe_ldo_io_cmd_wr_count_sg_entries(
 *                     fbe_logical_drive_io_cmd_t * const io_cmd_p)
 ****************************************************************
 * @brief
 *  This function counts the number of needed sg entries
 *  for this write operation.
 *
 * @param io_cmd_p - The logical drive io cmd object to execute.
 *
 * @return
 *  Number of sg entries needed for this write operation.
 *  0 means that some error was encountered.
 *
 * HISTORY:
 *  10/22/07 - Created. RPF
 *
 ****************************************************************/
fbe_u32_t 
fbe_ldo_io_cmd_wr_count_sg_entries(fbe_logical_drive_io_cmd_t * const io_cmd_p)
{
    fbe_status_t status;
    fbe_u32_t sg_entries = 0;
    fbe_logical_drive_t * logical_drive_p = 
        fbe_ldo_io_cmd_get_logical_drive( io_cmd_p );
    fbe_u32_t edge0_bytes, edge1_bytes;
    
    /* Offset in bytes into edge descriptor's sg for first and last edges.
     */
    fbe_u32_t first_edge_offset, last_edge_offset; 

    /*! @remarks The sg list we are creating will look like this.
     *
     * @verbatim
     * |----------|  /|\ <-- Start of first client edge sgs.
     * |----------|   | 0 or more entries for first edge.
     * |----------|   |
     * |----------|  \|/ <-- End of first edge sgs.
     * |----------|  /|\ <-- Start of client sgs
     * |----------|   | 
     * |----------|   |   1 or more client sgs.
     * |----------|   |
     * |----------|  \|/ <-- End of client sgs
     * |----------|  /|\ <-- Start of last client edge sgs.
     * |----------|   | 0 or more entries for last edge.
     * |----------|   |
     * |----------|  \|/ <-- End of last client edge sgs.
     * 
     * @endverbatim
     */

    /* First start out with how many bytes are needed for the original
     * sg list from the client.
     * Only count the number of sgs required
     * for the total number of bytes in the transfer.
     */ 
    sg_entries = fbe_ldo_io_cmd_count_sgs_for_client_data(io_cmd_p);
    
    if (sg_entries == 0)
    {
        /* Something is wrong, we did not find any sgs.
         */
        return sg_entries;
    }
    
    /* Get the sizes of the edges.
     */
    fbe_ldo_get_edge_sizes( fbe_ldo_io_cmd_get_lba( io_cmd_p ),
                            fbe_ldo_io_cmd_get_blocks( io_cmd_p ),
                            fbe_ldo_io_cmd_get_block_size( io_cmd_p ),
                            fbe_ldo_io_cmd_get_optimum_block_size( io_cmd_p ),
                            fbe_ldo_get_imported_block_size(logical_drive_p),
                            fbe_ldo_io_cmd_get_imported_optimum_block_size(io_cmd_p),
                            &edge0_bytes,
                            &edge1_bytes );

    /* Only unaligned writes have pre-read descriptors.
     */
    if (fbe_ldo_io_cmd_is_aligned(io_cmd_p) == FALSE)
    {
        status = fbe_ldo_io_cmd_get_pre_read_desc_offsets( io_cmd_p,
                                                       &first_edge_offset,
                                                       &last_edge_offset,
                                                       edge0_bytes );
        if (status != FBE_STATUS_OK)
        {
            /* Error with getting the offsets, return 0 to signify error.
             */
            fbe_ldo_set_payload_status(fbe_ldo_io_cmd_get_packet(io_cmd_p),
                                       FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID_REQUEST,
                                       FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_INVALID_PRE_READ_DESCRIPTOR);
            return 0;
        }
    }
    else
    {
        first_edge_offset = 0;
        last_edge_offset = 0;
    }

    /* If we have a first edge, determine how many sgs to add.
     */
    if (edge0_bytes > 0)
    {
        fbe_u32_t edge0_sg_entries;
        fbe_sg_element_t *sg_p = fbe_ldo_io_cmd_get_pre_read_desc_sg_ptr(io_cmd_p);

        /* Get the sg entries for the first edge.
         */
        edge0_sg_entries = 
            fbe_sg_element_count_entries_with_offset(sg_p,
                                                     /* Bytes to count. */
                                                     edge0_bytes,
                                                     /* offset into sg. */
                                                     first_edge_offset );
        if (edge0_sg_entries == 0)
        {
            /* Some error occurred with fbe_sg_element_count_entries_with_offset.
             */
            fbe_ldo_set_payload_status(
                fbe_ldo_io_cmd_get_packet(io_cmd_p),
                FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID_REQUEST,
                FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_INVALID_PRE_READ_DESCRIPTOR_SG);
            return 0;
        }
        /* Add on the sg entries required for first edge.
         */
        sg_entries += edge0_sg_entries;

    }/* end if we have a first edge. */

    /* If we have a first edge, determine how many sgs to add.
     */
    if (edge1_bytes > 0)
    {
        fbe_u32_t edge1_sg_entries = 0;
        fbe_u32_t client_edge_bytes = edge1_bytes;
        fbe_u32_t pad_bytes = 0;
        const fbe_bool_t b_pad_needed = fbe_ldo_io_cmd_is_padding_required(io_cmd_p);
        const fbe_u32_t pad_sgs = 1;

        if (b_pad_needed == FALSE)
        {
            fbe_sg_element_t *sg_p;
            sg_p = fbe_ldo_io_cmd_get_pre_read_desc_sg_ptr(io_cmd_p);

           /* Get the sg   entries for the first edge.
            */
            edge1_sg_entries = 
                fbe_sg_element_count_entries_with_offset(sg_p,
                                                         /* Bytes to count. */
                                                         edge1_bytes,
                                                         /* offset into sg. */
                                                         last_edge_offset );
        }
        else
        {
            /* No padding is required, just copy the pre-read data into the 
             * edge. 
             */
            fbe_ldo_io_cmd_wr_get_edge1_fill_sizes(io_cmd_p, 
                                             edge1_bytes,
                                             &client_edge_bytes,
                                             &pad_bytes);
            if (client_edge_bytes > 0)
            {
                fbe_sg_element_t *sg_p;
                sg_p = fbe_ldo_io_cmd_get_pre_read_desc_sg_ptr(io_cmd_p);
                /* Get the sg   entries for the first edge.
                */
                edge1_sg_entries = 
                    fbe_sg_element_count_entries_with_offset(sg_p,
                                                             /* Bytes to count. */
                                                             client_edge_bytes,
                                                             /* offset into sg. */
                                                             last_edge_offset );
            }
            /* Next, insert the pad.
             */
            if (pad_bytes > 0)
            {
                edge1_sg_entries += pad_sgs;
            }
        } /* end insert padding */
        if (edge1_sg_entries == 0)
        {
            /* Some error occurred with fbe_sg_element_count_entries_with_offset.
             */
            fbe_ldo_set_payload_status(
                fbe_ldo_io_cmd_get_packet(io_cmd_p),
                FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID_REQUEST,
                FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_INVALID_PRE_READ_DESCRIPTOR_SG);
            return 0;
        }
        /* Add on the sg entries required for last edge.
         */
        sg_entries += edge1_sg_entries;

    }/* end if we have a last edge. */

    /* Add one for the null terminator.
     */
    sg_entries++;
    
    return sg_entries;
}
/* end fbe_ldo_io_cmd_wr_count_sg_entries() */

/*!***************************************************************
 * @fn fbe_ldo_io_cmd_wr_setup_sg(
 *                     fbe_logical_drive_io_cmd_t * const io_cmd_p)
 ****************************************************************
 * @brief
 *  This function sets up the sg list for a write operation.
 *
 * @param io_cmd_p - The logical drive io cmd object to execute.
 *
 * @return
 *  Status of the setup operation.  FBE_STATUS_OK indicates
 *  success and any other status indicates failure.
 *
 * HISTORY:
 *  10/22/07 - Created. RPF
 *
 ****************************************************************/
fbe_status_t 
fbe_ldo_io_cmd_wr_setup_sg(fbe_logical_drive_io_cmd_t * const io_cmd_p)
{
    fbe_status_t status;
    fbe_sg_element_t *sg_p = fbe_ldo_io_cmd_get_sg_ptr(io_cmd_p);
    fbe_logical_drive_t * logical_drive_p = 
        fbe_ldo_io_cmd_get_logical_drive( io_cmd_p );
    fbe_u32_t edge0_bytes, edge1_bytes;
    
    /* Offset in bytes into edge descriptor's sg for first and last edges.
     */
    fbe_u32_t first_edge_offset, last_edge_offset; 
    
    /* First get the edge sizes for the first and last edge.
     */
    fbe_ldo_get_edge_sizes( fbe_ldo_io_cmd_get_lba( io_cmd_p ),
                            fbe_ldo_io_cmd_get_blocks( io_cmd_p ),
                            fbe_ldo_io_cmd_get_block_size( io_cmd_p ),
                            fbe_ldo_io_cmd_get_optimum_block_size( io_cmd_p ),
                            fbe_ldo_get_imported_block_size(logical_drive_p),
                            fbe_ldo_io_cmd_get_imported_optimum_block_size(io_cmd_p),
                            &edge0_bytes,
                            &edge1_bytes);

    /* Only unaligned writes have pre-read descriptors.
     */
    if (fbe_ldo_io_cmd_is_aligned(io_cmd_p) == FALSE)
    {
        status = fbe_ldo_io_cmd_get_pre_read_desc_offsets( io_cmd_p,
                                                       &first_edge_offset,
                                                       &last_edge_offset,
                                                       edge0_bytes );
        if (status != FBE_STATUS_OK)
        {
            /* Error with getting the offsets, set payload status and return
             * error. 
             */
            fbe_base_object_trace((fbe_base_object_t*)
                              fbe_ldo_io_cmd_get_logical_drive(io_cmd_p), 
                              FBE_TRACE_LEVEL_ERROR, 
                              FBE_TRACE_MESSAGE_ID_CREATE_OBJECT,
                              "LDO: Error getting pre-read desc offsets %s \n", __FUNCTION__);
            fbe_ldo_set_payload_status(fbe_ldo_io_cmd_get_packet(io_cmd_p),
                                       FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID_REQUEST,
                                       FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_INVALID_PRE_READ_DESCRIPTOR);
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }
    else
    {
        first_edge_offset = 0;
        last_edge_offset = 0;
        status = FBE_STATUS_OK;
    }

    /* Fill in the edge 0 bytes with the client supplied edges.
     */
    if (edge0_bytes)
    {
        fbe_sg_element_t *pre_read_desc_sg_p = fbe_ldo_io_cmd_get_pre_read_desc_sg_ptr(io_cmd_p);
        sg_p = fbe_sg_element_copy_list_with_offset(pre_read_desc_sg_p,
                                                    sg_p,
                                                    edge0_bytes,
                                                    first_edge_offset);
    }

    /* Fill in the client portion of the sg list.
     */ 
    sg_p = fbe_ldo_io_cmd_setup_sgs_for_client_data(io_cmd_p,
                                                    sg_p /* destination */,
                                                    fbe_ldo_get_write_bitbucket_ptr());
    if ( sg_p == NULL )
    {    
        /* Something went wrong when setting up client data.
         */
        fbe_base_object_trace((fbe_base_object_t*)
                              fbe_ldo_io_cmd_get_logical_drive(io_cmd_p), 
                              FBE_TRACE_LEVEL_ERROR, 
                              FBE_TRACE_MESSAGE_ID_CREATE_OBJECT,
                              "LDO: Error setting up client data %s \n", __FUNCTION__);
        fbe_ldo_set_payload_status(fbe_ldo_io_cmd_get_packet(io_cmd_p),
                                  FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID_REQUEST,
                                  FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_INVALID_PRE_READ_DESCRIPTOR_SG);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Fill in the edge 1 bytes with the client supplied edges.
     */
    if (edge1_bytes)
    {
        const fbe_bool_t b_pad_needed = fbe_ldo_io_cmd_is_padding_required(io_cmd_p);

        if (b_pad_needed == FALSE)
        {
            fbe_sg_element_t *pre_read_desc_sg_p = fbe_ldo_io_cmd_get_pre_read_desc_sg_ptr(io_cmd_p);

            /* No padding is required, just copy the pre-read data into the 
             * edge. 
             */
            sg_p = fbe_sg_element_copy_list_with_offset(pre_read_desc_sg_p,
                                                        sg_p,
                                                        edge1_bytes,
                                                        last_edge_offset);
        }
        else 
        {
            /* We need padding, determine how much padding and how much user 
             * supplied edge data to copy. 
             */
            fbe_u32_t client_edge_bytes = edge1_bytes;
            fbe_u32_t pad_bytes = 0;

            fbe_ldo_io_cmd_wr_get_edge1_fill_sizes(io_cmd_p, edge1_bytes, &client_edge_bytes, &pad_bytes);
            /* Copy the client supplied edge data first.
             */
            if (client_edge_bytes > 0)
            {
                fbe_sg_element_t *pre_read_desc_sg_p = fbe_ldo_io_cmd_get_pre_read_desc_sg_ptr(io_cmd_p);
                sg_p = fbe_sg_element_copy_list_with_offset(pre_read_desc_sg_p,
                                                            sg_p,
                                                            client_edge_bytes,
                                                            last_edge_offset);
            }
            /* Next, insert the pad.
             */
            if (pad_bytes > 0)
            {
                sg_p = fbe_ldo_fill_sg_with_bitbucket(sg_p, pad_bytes,
                                                      fbe_ldo_get_write_bitbucket_ptr());
            }
        }/* end else padding needed */
    } /* end fill edge 1 */

    fbe_sg_element_terminate(sg_p);
    sg_p ++;
    
    if (((fbe_u32_t)(sg_p - fbe_ldo_io_cmd_get_sg_ptr(io_cmd_p))) >
        fbe_ldo_io_cmd_get_sg_entries(io_cmd_p))
    {
        /* If we did not allocate enough sgs return an error.
         * This is never expected since by the time we reach here everything 
         * should be setup properly. 
         */
        fbe_base_object_trace((fbe_base_object_t*) fbe_ldo_io_cmd_get_logical_drive(io_cmd_p),
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "LDO: Not enough sgs allocated.  0x%x:%s", __LINE__, __FUNCTION__);
        status = FBE_STATUS_GENERIC_FAILURE;

        fbe_ldo_set_payload_status(fbe_ldo_io_cmd_get_packet(io_cmd_p),
                                  FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID_REQUEST,
                                  FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_UNEXPECTED_ERROR);
    }
    
    if (((fbe_u32_t)(sg_p - fbe_ldo_io_cmd_get_sg_ptr(io_cmd_p))) <
        fbe_ldo_io_cmd_get_sg_entries(io_cmd_p))
    {
        /* If we allocated too many sgs return an error.
         * This is never expected since by the time we reach here everything 
         * should be setup properly. 
         */
        fbe_base_object_trace((fbe_base_object_t*) fbe_ldo_io_cmd_get_logical_drive(io_cmd_p),
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "LDO: Too many sgs allocated. 0x%x:%s", __LINE__, __FUNCTION__);
        status = FBE_STATUS_GENERIC_FAILURE;
        fbe_ldo_set_payload_status(fbe_ldo_io_cmd_get_packet(io_cmd_p),
                                  FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID_REQUEST,
                                  FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_UNEXPECTED_ERROR);
    }
    return status;
}
/* end fbe_ldo_io_cmd_wr_setup_sg() */

/*!***************************************************************
 * @fn fbe_ldo_write_setup_sgs(   
 *                 fbe_logical_drive_t * const logical_drive_p, 
 *                 fbe_payload_ex_t * const payload_p)
 ****************************************************************
 * @brief
 *  This function sets up the sg list for a write operation where
 *  we are simply filling in the packet's pre and post ptrs.
 * 
 *  In this case we fill out the pre and post sgs with the pre-read
 *  data that the client passed to us.
 * 
 * @param logical_drive_p - The logical drive object.
 * @param payload_p - The ptr to the payload we are executing.
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - no error.
 *                 Other values imply there was an error.
 *
 * HISTORY:
 *  11/10/08 - Created. RPF
 *
 ****************************************************************/
fbe_status_t 
fbe_ldo_write_setup_sgs(fbe_logical_drive_t * logical_drive_p, fbe_packet_t * packet_p)
{
    fbe_status_t status;
    fbe_u32_t client_bytes;
    fbe_sg_element_t *client_sg_p = NULL;
    fbe_payload_block_operation_t *block_operation_p = NULL;
    fbe_payload_pre_read_descriptor_t *pre_read_desc_p = NULL;
    fbe_sg_element_t *pre_read_sg_p = NULL;
    fbe_lba_t lba;
    fbe_lba_t pre_read_lba;
    fbe_block_count_t blocks;
    fbe_block_size_t exported_block_size;
    fbe_block_size_t exported_opt_block_size;
    fbe_block_size_t imported_block_size;
    fbe_block_size_t imported_opt_block_size;
    fbe_u32_t first_edge_offset, last_edge_offset;
    fbe_u32_t edge0_bytes, edge1_bytes;
	fbe_block_edge_t * block_edge = NULL;
	fbe_payload_ex_t * payload_p = NULL;

	payload_p = fbe_transport_get_payload_ex(packet_p);
    fbe_payload_ex_get_sg_list(payload_p, &client_sg_p, NULL);
    block_operation_p = fbe_payload_ex_get_block_operation(payload_p);

    /* Get the pointer to the pre-read descriptor.
     */
    fbe_payload_block_get_pre_read_descriptor(block_operation_p, &pre_read_desc_p);
    fbe_payload_pre_read_descriptor_get_sg_list(pre_read_desc_p, &pre_read_sg_p);

    /* Determine how many blocks are in the write.
     */
    fbe_payload_block_get_lba(block_operation_p, &lba);
    fbe_payload_block_get_block_count(block_operation_p, &blocks);
    fbe_payload_block_get_block_size(block_operation_p, &exported_block_size);
    client_bytes = (fbe_u32_t)(blocks * exported_block_size);

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

    /* The imported optimal block size needs to be derived from the requested 
     * exported sizes and the imported block size. 
     */
    imported_opt_block_size = 
        fbe_ldo_calculate_imported_optimum_block_size(exported_block_size,
                                                      exported_opt_block_size,
                                                      imported_block_size);
    /* Get the edge sizes and fill out the pre/post sgs.
     */
    fbe_ldo_get_edge_sizes( lba, blocks, exported_block_size, exported_opt_block_size,
                            imported_block_size, imported_opt_block_size,
                            &edge0_bytes, &edge1_bytes );

    /* Only unaligned writes have pre-read descriptors.
     * Determine the offset from the beginning of the pre-read data 
     * to the first and last edges. 
     */
    fbe_payload_pre_read_descriptor_get_lba(pre_read_desc_p, &pre_read_lba);
    status = fbe_ldo_map_get_pre_read_desc_offsets(lba, blocks,
                                                   pre_read_lba,
                                                   exported_block_size, exported_opt_block_size,
                                                   imported_block_size, edge0_bytes, /* First edge size. */
                                                   &first_edge_offset, &last_edge_offset);

    if (status != FBE_STATUS_OK)
    {
        /* Error with getting the offsets.
         */
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* If we have a second edge, then validate that there are exactly 
     * the right number of bytes in the client SG List. 
     * We need to validate this since we will try to fill out the 
     * post-sg in the packet with the second edge bytes. 
     * If the client sg has too many bytes, then the post-sg will not appear 
     * immediately after the data the client is trying to read. 
     */
    if (edge1_bytes)
    {
        fbe_bool_t b_remainder;
        fbe_u32_t client_data_sg_count = 0;
        /* Check to see if we have a remainder.
         */
        client_data_sg_count = 
            fbe_sg_element_count_entries_for_bytes_with_remainder(client_sg_p,
                                                                  client_bytes,
                                                                  &b_remainder);
        if (b_remainder)
        {
            /* If there is a remainder, then it means that the number of bytes 
             * ended within an sg entry.
             * If there is a trailing edge, then it will not be possible to use
             * the post-sg in the packet for this trailing edge, since
             * the sg list with the host data is too long. This prevents us
             * from inserting the trailing edge just after the host data,
             * since there is additional data in the client sg list just
             * after the host data they wish to read.
             *
             * Return failure so that we will allocate an sg list 
             */
            return FBE_STATUS_INSUFFICIENT_RESOURCES;
        }
    } /* End if edge1_bytes sanity check. */

    /* There are three sg lists in the payload.
     * If we have a first edge or a last edge, we will simply fill in
     * the pre and post sgs lists with the first or last edge sg.
     *
     *  Payload
     *   pre_sg_list ----> First edge 
     *   sg_list --------> Client data 
     *   post_sg_list ---> Last edge
     *
     * Fill in the edge 0 bytes with the pre-read data.
     */
    if (edge0_bytes)
    {
        fbe_u32_t edge0_sgs;
        fbe_sg_element_t *pre_sg_p;

        /* Get the sg entries for the first edge.
         */
        edge0_sgs = fbe_sg_element_count_entries_with_offset(pre_read_sg_p,
                                                             /* Bytes to count. */
                                                             edge0_bytes,
                                                             /* offset into sg. */
                                                             first_edge_offset );

        /* There are only FBE_PAYLOAD_SG_ARRAY_LENGTH entries 
         * reserved in the pre payload sg list, and the final entry 
         * needs to be reserved for the terminator. 
         */
        if (edge0_sgs > FBE_PAYLOAD_SG_ARRAY_LENGTH - 1)
        {
            /* The sgs supplied in the packet is not long enough.
             * Return an error so we will allocate an sg list. 
             */
            return FBE_STATUS_INSUFFICIENT_RESOURCES;
        }
        /* Fetch the ptr to the pre-sg list.
         */
        fbe_payload_ex_get_pre_sg_list(payload_p, &pre_sg_p);

        /* Copy the pre-read data into the sg.
         */
        pre_sg_p = fbe_sg_element_copy_list_with_offset(pre_read_sg_p,
                                                        pre_sg_p,
                                                        edge0_bytes,
                                                        first_edge_offset);
        /* Terminate the pre-read sg.
         */
        fbe_sg_element_terminate(pre_sg_p);
    } /* End if edge0_bytes */
    
    /* Fill in the edge 1 bytes with the pre-read data.
     */
    if (edge1_bytes)
    {
        fbe_u32_t edge1_sgs;
        fbe_sg_element_t *post_sg_p;

        /* Determine how many sg entries are needed to represent 
         * the pre-read data.  We perform this check so we can 
         * sanity check the number of sgs needed. 
         */
        edge1_sgs = fbe_sg_element_count_entries_with_offset(pre_read_sg_p,
                                                             /* Bytes to count. */
                                                             edge1_bytes,
                                                             /* offset into sg. */
                                                             last_edge_offset );
        if (edge1_sgs == 0)
        {
            /* Some error occurred with fbe_sg_element_count_entries_with_offset.
             */
            return FBE_STATUS_GENERIC_FAILURE;
        }

        /* There are only FBE_PAYLOAD_SG_ARRAY_LENGTH entries 
         * reserved in the pre payload sg list, and the final entry 
         * needs to be reserved for the terminator. 
         */
        if (edge1_sgs > FBE_PAYLOAD_SG_ARRAY_LENGTH - 1)
        {
            /* The sgs supplied in the packet is not long enough.
             * Return an error so we will allocate an sg list. 
             */
            return FBE_STATUS_INSUFFICIENT_RESOURCES;
        }

        fbe_payload_ex_get_post_sg_list(payload_p, &post_sg_p);

        /* Just copy the pre-read data into the post edge sg.
         */
        post_sg_p = fbe_sg_element_copy_list_with_offset(pre_read_sg_p,
                                                         post_sg_p,
                                                         edge1_bytes,
                                                         last_edge_offset);
        /* Terminate the sg.
         */
        fbe_sg_element_terminate(post_sg_p);
    } /* End if edge1_bytes */
    
    return FBE_STATUS_OK;
}
/* end fbe_ldo_write_setup_sgs() */

/*******************************
 * end fbe_logical_drive_write.c
 ******************************/
