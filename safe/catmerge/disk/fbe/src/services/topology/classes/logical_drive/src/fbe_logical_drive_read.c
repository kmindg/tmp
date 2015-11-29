/***************************************************************************
 * Copyright (C) EMC Corporation 2007-2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_logical_drive_read.c
 ***************************************************************************
 *
 * @brief
 *  This file contains functions of the logical drive object's
 *  read state machine.
 * 
 * @ingroup logical_drive_class_files
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
 * @fn fbe_ldo_handle_read(fbe_logical_drive_t *logical_drive_p, 
 *                         fbe_packet_t * packet_p)
 ****************************************************************
 * @brief
 *  This function is the entry point for read
 *  operations to the logical drive object.
 *
 * @param logical_drive_p - The logical drive.
 * @param packet_p - The read io packet that is arriving.
 *
 * @return fbe_status_t - FBE_STATUS_OK.
 *
 * HISTORY:
 *  10/9/2008 - Created. RPF
 *
 ****************************************************************/

fbe_status_t
fbe_ldo_handle_read(fbe_logical_drive_t *logical_drive_p, 
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
    fbe_u32_t edge0_bytes = 0;
    fbe_u32_t edge1_bytes = 0;
	fbe_block_edge_t * block_edge = NULL;

    fbe_payload_ex_t *payload_p = fbe_transport_get_payload_ex(packet_p);
    fbe_payload_block_operation_t *block_operation_p = fbe_payload_ex_get_block_operation(payload_p);

    /* Get the block sizes, lba and blocks from the payload. Also calculate the
     * imported optimum size. 
     */
    imported_block_size = fbe_ldo_get_imported_block_size(logical_drive_p);
    fbe_payload_block_get_lba(block_operation_p, &lba);
    fbe_payload_block_get_block_count(block_operation_p, &blocks);
    fbe_payload_block_get_block_size(block_operation_p, &exported_block_size);

#if 0 /* Block edge geometry */
	fbe_payload_block_get_optimum_block_size(block_operation_p, &exported_opt_block_size);
#endif
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
    if (exported_opt_block_size == 0)
    {
        /* Something is wrong if the optimum block size is zero.
         */
        fbe_base_object_trace((fbe_base_object_t*)logical_drive_p, FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                              "logical drive: unexpected read opt block sizes req:0x%x opt:0x%x Geometry:0x%x\n", 
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

    fbe_payload_block_get_opcode(block_operation_p, &opcode);

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

    /* If we are not aligned, then get the edge sizes and
     * fill out the pre/post sgs. 
     */
    if (!fbe_ldo_io_is_aligned(lba, blocks, exported_opt_block_size))
    {
        fbe_status_t status;

        fbe_ldo_get_edge_sizes( lba, blocks, exported_block_size, exported_opt_block_size,
                                imported_block_size, imported_opt_block_size,
                                &edge0_bytes,
                                &edge1_bytes);
        status = fbe_ldo_read_setup_sgs(logical_drive_p, payload_p,
                                        edge0_bytes, edge1_bytes);
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
    }
    /* Then initialize our packet for a read.
     *  
     * Note that we pass through the same opcode that we received, 
     * this allows us to handle different opcodes as a read. For example, we 
     * want to handle the read & remap opcode the same as a read.
     */
    fbe_ldo_build_packet(packet_p,
                         opcode,
                         imported_lba,
                         imported_blocks,
                         fbe_ldo_get_imported_block_size(logical_drive_p),
                         1,    /* imported optimal. */
                         NULL,    /* Don't touch the sg ptr. */
                         fbe_ldo_packet_completion,    /* completion fn */
                         logical_drive_p,    /* completion context */
                         1    /* repeat count */ ); 

    /* Send out the io packet.  Our callback always gets executed, thus
     * there is no status to handle here.
     */
    fbe_ldo_send_io_packet(logical_drive_p, packet_p);
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_ldo_handle_read()
 ******************************************/

/*!***************************************************************
 * @fn fbe_ldo_io_cmd_read_state0(
 *                    fbe_logical_drive_io_cmd_t * const io_cmd_p)
 ****************************************************************
 * @brief
 *  This function handles the first state of the read state machine.
 *
 *  We will first allocate an IoPacket.
 *
 * @param io_cmd_p - The logical drive io cmd object to execute.
 *
 * @return FBE_LDO_STATE_STATUS_EXECUTING
 *
 * HISTORY:
 *  10/22/07 - Created. RPF
 *
 ****************************************************************/
fbe_ldo_state_status_enum_t 
fbe_ldo_io_cmd_read_state0(fbe_logical_drive_io_cmd_t * const io_cmd_p)
{
    fbe_bool_t b_padding_required = fbe_ldo_io_cmd_is_padding_required(io_cmd_p);

    if (b_padding_required)
    {
       /* When padding is required, we always need to alloc an sg.
        * Transition to the state for allocating an sg.
        */
        fbe_ldo_io_cmd_set_state(io_cmd_p, fbe_ldo_io_cmd_read_state10);
    }
    else if (fbe_ldo_io_cmd_is_aligned(io_cmd_p) == FALSE)
    {
        /* Transition to the state for allocating an sg.
         */
        fbe_ldo_io_cmd_set_state(io_cmd_p, fbe_ldo_io_cmd_read_state10);
    }
    else
    {
        /* We are aligned and thus we do not need to allocate an sg.
         * Instead we will use the sg passed to us in the io packet. 
         * Transition to the state for sending out I/O.
         */
        fbe_ldo_io_cmd_set_state(io_cmd_p, fbe_ldo_io_cmd_read_state20);
    }
    return FBE_LDO_STATE_STATUS_EXECUTING;
}
/* end fbe_ldo_io_cmd_read_state0() */

/*!***************************************************************
 * @fn fbe_ldo_io_cmd_read_state10(
 *                    fbe_logical_drive_io_cmd_t * const io_cmd_p)
 ****************************************************************
 * @brief
 *  This function handles the count sgs state of the read sm.
 *
 * @param io_cmd_p - The logical drive io cmd object to execute.
 *
 * @return
 *  FBE_LDO_STATE_STATUS_EXECUTING - Always execute the next state.
 *
 * HISTORY:
 *  08/04/08 - Created. RPF
 *
 ****************************************************************/
fbe_ldo_state_status_enum_t 
fbe_ldo_io_cmd_read_state10(fbe_logical_drive_io_cmd_t * const io_cmd_p)
{
    fbe_u32_t sg_entries;

    /* If we are not aligned, then we will need to allocate an sg.
     */
    sg_entries = fbe_ldo_io_cmd_rd_count_sg_entries(io_cmd_p);

    if (sg_entries == 0)
    {
        /* Something is wrong with the input sg.
         * Set the error in the packet and transition to finished.
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
        /* Transition to the next state before trying to get the sg.
         * The callback might get executed which could cause our 
         * next state to get triggered.
         */
        fbe_ldo_io_cmd_set_state(io_cmd_p, fbe_ldo_io_cmd_read_state20);

        /* Save this value in the io cmd for later use.
         */
        fbe_ldo_io_cmd_set_sg_entries(io_cmd_p, sg_entries);
    }
    return FBE_LDO_STATE_STATUS_EXECUTING;
}
/* end fbe_ldo_io_cmd_read_state10() */

/*!***************************************************************
 * @fn fbe_ldo_io_cmd_read_state20(
 *                    fbe_logical_drive_io_cmd_t * const io_cmd_p)
 ****************************************************************
 * @brief
 *  This function initializes our sg, and io packet and sends out
 *  the sub io packet for the read.
 *
 * @param io_cmd_p - The logical drive io cmd object to execute.
 *
 * @return
 *  FBE_LDO_STATE_STATUS_EXECUTING - If an error was encountered
 *                                   and we should immediately
 *                                   execute the next state.
 *  FBE_LDO_STATE_STATUS_WAITING - We always wait for the next
 *                                 state to get executed when
 *                                 we have sent out the request.
 * HISTORY:
 *  11/07/07 - Created. RPF
 *
 ****************************************************************/
fbe_ldo_state_status_enum_t 
fbe_ldo_io_cmd_read_state20(fbe_logical_drive_io_cmd_t * const io_cmd_p)
{
    fbe_lba_t imported_lba;
    fbe_block_count_t imported_blocks;

    /* Determine the mapped lba and block count for
     * the given io command.
     */
    fbe_ldo_io_cmd_map_lba_blocks( io_cmd_p, &imported_lba, &imported_blocks );

    /* First, if we needed to allocate an sg list, then
     * initialize our newly allocated sg list.
     */
    if (fbe_ldo_io_cmd_get_sg_entries(io_cmd_p) != 0)
    {
        fbe_status_t status = fbe_ldo_io_cmd_read_setup_sg(io_cmd_p);

        if (status != FBE_STATUS_OK)
        {
            /* Packet status is OK since there was no transport error. 
             * Payload status is failed since something went wrong with setting up the sg.
             */
            fbe_transport_set_status(fbe_ldo_io_cmd_get_packet(io_cmd_p),
                                      FBE_STATUS_GENERIC_FAILURE, 0);
            fbe_ldo_set_payload_status(fbe_ldo_io_cmd_get_packet(io_cmd_p),
                                       FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID_REQUEST,
                                       FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_UNEXPECTED_ERROR);
            fbe_ldo_io_cmd_set_state(io_cmd_p, fbe_ldo_io_cmd_finished);
            return FBE_LDO_STATE_STATUS_EXECUTING;
        }
    }
    
    /* Transition to the next state before trying to send out the io.
     * Upon sending the io packet, the callback might get executed 
     * which could cause our next state to get triggered.
     */
    fbe_ldo_io_cmd_set_state(io_cmd_p, fbe_ldo_io_cmd_read_state30);

    /* Then initialize our io packet.
     */
    fbe_ldo_io_cmd_build_sub_packet(io_cmd_p, 
                                    FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,
                                    imported_lba,
                                    imported_blocks,
                                    1 /* imported optimal. */);

    /* Send out the io packet.  Our callback always gets executed, thus
     * there is no status to handle here.
     */
    fbe_ldo_io_cmd_send_io(io_cmd_p);

    return FBE_LDO_STATE_STATUS_WAITING;
}
/* end fbe_ldo_io_cmd_read_state20() */

/*!***************************************************************
 * @fn fbe_ldo_io_cmd_read_state30(
 *                    fbe_logical_drive_io_cmd_t * const io_cmd_p)
 ****************************************************************
 * @brief
 *  This function handle the first state of the read state machine.
 *
 * @param io_cmd_p - The logical drive io cmd object to execute.
 *
 * @return
 *  FBE_LDO_STATE_STATUS_EXECUTING - Always execute the next state.
 *
 * HISTORY:
 *  11/07/07 - Created. RPF
 *
 ****************************************************************/
fbe_ldo_state_status_enum_t 
fbe_ldo_io_cmd_read_state30(fbe_logical_drive_io_cmd_t * const io_cmd_p)
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
/* end fbe_ldo_io_cmd_read_state30() */

/*!***************************************************************
 * @fn fbe_ldo_count_sgs_for_optimal_block_size(
 *                  fbe_sg_element_t *const sg_element_p,
 *                  fbe_u32_t offset_to_first_optimal_block,
 *                  fbe_u32_t optimal_block_size,
 *                  fbe_u32_t pad_sgs,
 *                  fbe_u32_t total_bytes)
 ****************************************************************
 * @brief
 *  This function determines how many sg entries are required to
 *  represent the "total_bytes" worth of client data for the
 *  given parameters.
 * 
 *  Suppose we have a 3 520 byte transfer.
 *  Optimal block size is 7 x 520 = 3640
 *       ___________________
 *       | Total Bytes is  |
 *       | 3 x 520 = 1560  |
 *      \|/               \|/ 
 * |-----|-----|-----|-----|-----|-----|-----|       Exported 520
 * |-----------------------------------------------| Imported 4096
 *      /|\  Offset to optimal block is     /|\
 *       |   From the start of the transfer  |
 *       | to the end of this optimal block  |
 *       | 5 x 7 = 3640 - 520 = 3120         |
 *       -------------------------------------
 *
 * @param sg_element_p - The sg list of client data
 * @param offset_to_first_optimal_block - Number of bytes until we
 *                           reach the first optimal block boundary.
 * @param optimal_block_size - Size in bytes of the exported optimal block.
 * @param pad_sgs - The total number sgs to add whenever we need to add padding.
 * @param total_bytes - Bytes to count at the most.
 *
 * @return
 *  The number of sg entries required to represent the
 *  input number of bytes in this sg list.
 *  Note that if there is an error then 0 is returned.
 *
 * HISTORY:
 *  10/22/07 - Created. RPF
 *
 ****************************************************************/
static fbe_u32_t 
fbe_ldo_count_sgs_for_optimal_block_size(fbe_sg_element_t *const sg_element_p,
                                         fbe_u32_t offset_to_first_optimal_block,
                                         fbe_u32_t optimal_block_size,
                                         fbe_u32_t pad_sgs,
                                         fbe_u32_t total_bytes)
{
    fbe_u32_t sg_entries = 0;
    fbe_u32_t bytes_left_in_opt_blk = FBE_MIN(offset_to_first_optimal_block,
                                              total_bytes);
    fbe_sg_element_t *sg_p = sg_element_p;
    fbe_u32_t bytes_remaining_in_sg;
    fbe_u32_t total_bytes_remaining = total_bytes;

    /* If the sg list passed in is NULL, then we simply return
     * 0, which indicates error.
     */
    if (sg_p == NULL)
    {
        return sg_entries;
    }
    else if ( total_bytes_remaining <= bytes_left_in_opt_blk )
    {
        /* We have less than a full optimal block total, so 
         * simply count how many entries there are for what is left in the 
         * optimal block. 
         *  
         * |-----|-----|-----|-----|-----|-----|-----|       Exported 520
         * |-----------------------------------------------| Imported 4096
         */
        sg_entries = 
            fbe_sg_element_count_entries_for_bytes(sg_element_p, total_bytes);
        return sg_entries;
    }

    /* Once we have validated the sg, it is OK to use it.
     */
    bytes_remaining_in_sg = fbe_sg_element_count(sg_p);

    /* Simply iterate over the sg list, counting the number of entries.
     * We stop counting if we hit a zero count or NULL address.
     * We also stop counting sgs when we have exhausted the byte count.
     */
    while (fbe_sg_element_count(sg_p) != 0 && 
           fbe_sg_element_address(sg_p) != NULL &&
           total_bytes_remaining > 0)
    {
        /* Determine the number of sg entries needed to fill client data (and padding)
         * for the next optimal block.
         */
        total_bytes_remaining -= bytes_left_in_opt_blk;

        /* Loop over all the sgs needed to fill out this optimal block with client data.
         */
        while ( bytes_left_in_opt_blk > 0 )
        {
            if ( bytes_remaining_in_sg > bytes_left_in_opt_blk )
            {
                /* This entry has more than enough, just set bytes remaining
                 * in our optimal block to zero.
                 */
                bytes_remaining_in_sg -= bytes_left_in_opt_blk;
                bytes_left_in_opt_blk = 0;
            }
            else
            {
                bytes_left_in_opt_blk -= bytes_remaining_in_sg;
                fbe_sg_element_increment(&sg_p);
                bytes_remaining_in_sg = fbe_sg_element_count(sg_p);
            }
            sg_entries++;
        }    /* end while bytes_left_in_opt_blk > 0 */

        /* Only add padding if we have more blocks to do. 
         * Any padding at the end is considered an edge and not padding. 
         */
        if (total_bytes_remaining > 0)
        {
            /* We've just transitioned past an optimal block, just add on the 
             * number of sgs needed for padding. 
             */
            sg_entries += pad_sgs;
        }
        /* Also, since we are starting a new optimal block,
         * reset the number of bytes in the optimal block size.
         */
        bytes_left_in_opt_blk = FBE_MIN(optimal_block_size, total_bytes_remaining);

    } /* end while blocks remaining */

    /* If we have bytes remaining, then the sg list is too short.
     * Set sg_entries to zero to indicate error.
     */
    if (total_bytes_remaining > 0)
    {
        sg_entries = 0;
    }
    
    return sg_entries;
}
/* end fbe_ldo_count_sgs_for_optimal_block_size() */

/*!***************************************************************
 * @fn fbe_ldo_io_cmd_count_sgs_for_client_data(
 *                     fbe_logical_drive_io_cmd_t * const io_cmd_p)
 ****************************************************************
 * @brief
 *  Count number of sgs needed for client data.
 *
 * @param io_cmd_p - The logical drive io cmd object to execute.
 *
 * @return
 *  Count of sgs needed for client sgs.         
 *
 * HISTORY:
 *  5/21/2008 - Created. RPF
 *
 ****************************************************************/

fbe_u32_t 
fbe_ldo_io_cmd_count_sgs_for_client_data(fbe_logical_drive_io_cmd_t * const io_cmd_p)
{
    fbe_sg_element_t * const sg_element_p = fbe_ldo_io_cmd_get_client_sg_ptr(io_cmd_p);
    fbe_block_count_t block_count = fbe_ldo_io_cmd_get_blocks(io_cmd_p);
    fbe_block_size_t block_size = fbe_ldo_io_cmd_get_block_size(io_cmd_p);
    fbe_u32_t transfer_bytes = (fbe_u32_t)(block_count * block_size);
    fbe_bool_t b_pad_needed = fbe_ldo_io_cmd_is_padding_required(io_cmd_p);
    fbe_u32_t sg_entries = 0;

    if (b_pad_needed == FBE_TRUE)
    {
        /* Padding is needed in cases where the imported optimal block size does 
         * not equal the exported optimal block size. 
         *  
         * For example if we map 2 520 byte blocks onto 3 512 byte blocks, we 
         * end up with the below. 
         *  
         *         Padding needed for these areas.  
         *                     |------|                   |------| 
         *                    \|/    \|/                 \|/    \|/
         * |---------|---------| pad  |---------|---------| pad  | Exported(520)
         *  
         * |--------|--------|--------|--------|--------|--------| Imported(512)
         */
        fbe_block_size_t exported_opt_blk_size = fbe_ldo_io_cmd_get_optimum_block_size(io_cmd_p);
        fbe_lba_t lba = fbe_ldo_io_cmd_get_lba(io_cmd_p);

        /* Calculate the offset to the end of the exported optimum block.
         * In this illustrated example, we have: 
         *  
         * exported block size = 520 
         * imported block size = 4096 
         * exported optimal block size = 7 
         * imported optimal block size = 1 
         *       _________________________
         *       |                       |
         *      \|/ 3 blocks in xfer    \|/
         * |-----|-----|-----|-----|-----|-----|-----|       Exported 520
         * |-----------------------------------------------| Imported 4096
         *      /|\  Offset to end of optimum block /|\
         *       |is from start of transfer (lba)    |
         *       |  until the end of the EXPORTED    |
         *       |  optimal block size.              |
         *       -------------------------------------
         */
        fbe_u32_t offset_to_opt_blk = 
            (fbe_u32_t)(exported_opt_blk_size - (lba % exported_opt_blk_size));

        /* Lossy algorithms where the number of bytes imported is larger than the number
         * of bytes exported, have different imported and exported optimal block sizes. We
         * need to traverse the sg list and count exactly how many sgs are needed to 
         * account for splitting sgs and adding fill. 
         */
        sg_entries = fbe_ldo_count_sgs_for_optimal_block_size(sg_element_p,
                                                              (offset_to_opt_blk * block_size),
                                                              /* opt blk size in bytes */
                                                              (exported_opt_blk_size * block_size),
                                                              /* Currently we only need one pad sg
                                                               * since the pad always fits in the write bitbucket.
                                                               */
                                                              1,
                                                              transfer_bytes);
    }
    else
    {
        /* Optimize the case where the algorithm is not lossy,
         * just count the entries in the sg.
         */
        sg_entries = 
            fbe_sg_element_count_entries_for_bytes(sg_element_p, transfer_bytes);
    }

    return sg_entries;
}
/******************************************
 * end fbe_ldo_rd_count_sgs_for_client_data()
 ******************************************/

/*!***************************************************************
 * @fn fbe_ldo_io_cmd_rd_count_sg_entries(
 *                      fbe_logical_drive_io_cmd_t * const io_cmd_p)
 ****************************************************************
 * @brief
 *  This function counts the number of needed sg entries
 *  for this read operation.
 *
 * @param io_cmd_p - The logical drive io cmd object to execute.
 *
 * @return
 *  Number of sg entries needed for this read operation.
 *  0 means that some error was encountered.
 *
 * HISTORY:
 *  05/21/08 - Created. RPF
 *
 ****************************************************************/

fbe_u32_t 
fbe_ldo_io_cmd_rd_count_sg_entries(fbe_logical_drive_io_cmd_t * const io_cmd_p)
{
    fbe_u32_t sg_entries = 0;
    fbe_logical_drive_t * logical_drive_p =  NULL;
    fbe_u32_t edge0_bytes = 0;
    fbe_u32_t edge1_bytes = 0;
    fbe_lba_t lba;
    fbe_block_count_t block_count = 0;

    logical_drive_p = fbe_ldo_io_cmd_get_logical_drive( io_cmd_p );

    lba = fbe_ldo_io_cmd_get_lba(io_cmd_p);
    block_count = fbe_ldo_io_cmd_get_blocks(io_cmd_p);
    
    /* The sg list we are creating will look like this.
     * Each |------| represents an fbe_sg_element_t 
     * 
     * |----------|  /|\ <-- Start of first edge sgs.
     * |----------|   | 0 or more entries for first edge.
     * |----------|   |
     * |----------|  \|/ <-- End   of first edge sgs.
     * |----------|  /|\ <-- Start of client sgs
     * |----------|   | 
     * |----------|   |   1 or more client sgs.
     * |----------|   |
     * |----------|  \|/ <-- End of client sgs
     * |----------|  /|\ <-- Start of last edge sgs.
     * |----------|   | 0 or more entries for last  edge.
     * |----------|   |
     * |----------|  \|/ <-- End   of last edge sgs. 
     * |----------|  Terminator. 
     */

    /* First start out with how many sg entries are needed for the original
     * sg list from the client.
     * Only count the number of sgs required for the total number of bytes in 
     * the transfer. 
     */
    sg_entries = fbe_ldo_io_cmd_count_sgs_for_client_data(io_cmd_p);
    
    if (sg_entries == 0)
    {
        /* Something is wrong, we did not find any sgs.
         */
        return sg_entries;
    }
    
    /* Next add on the number of sgs for the first and last edge.
     */
    fbe_ldo_get_edge_sizes( lba, block_count,
                            fbe_ldo_io_cmd_get_block_size(io_cmd_p),
                            fbe_ldo_io_cmd_get_optimum_block_size(io_cmd_p),
                            fbe_ldo_get_imported_block_size(logical_drive_p),
                            fbe_ldo_io_cmd_get_imported_optimum_block_size(io_cmd_p),
                            &edge0_bytes,
                            &edge1_bytes );

    /* Add on the full sg entries needed for the first edge.
     */
    sg_entries += fbe_ldo_bitbucket_get_sg_entries(edge0_bytes);

    /* Add on the full sg entries needed for the second edge.
     */
    sg_entries += fbe_ldo_bitbucket_get_sg_entries(edge1_bytes);

    /* Add one for the null terminator.
     */
    sg_entries++;
    
    return sg_entries;
}
/* end fbe_ldo_io_cmd_rd_count_sg_entries() */

/*!***************************************************************
 * @fn fbe_ldo_setup_sgs_for_optimal_block_size(
 *                      fbe_sg_element_t *const source_sg_p,
 *                      fbe_sg_element_t *const dest_sg_p,
 *                      fbe_u32_t offset_to_first_optimal_block,
 *                      fbe_u32_t optimal_block_size,
 *                      fbe_u32_t pad_bytes,
 *                      fbe_u32_t total_bytes,
 *                      fbe_ldo_bitbucket_t *const bitbucket_p)
 ****************************************************************
 * @brief
 *  This function counts the number of sg entries in the input
 *  sg list that represent the input number of bytes.
 *
 * @param source_sg_p - The sg list to read from.
 * @param dest_sg_p - The sg list to setup.
 * @param offset_to_first_optimal_block - offset to the end
 *                         of the first optimal block.
 * @param optimal_block_size - Size of the exported optimal block in blocks.
 * @param pad_bytes - number of bytes we need to add padding
 *                    for at the end of the optimal block.
 * @param total_bytes - Bytes to count at the most.
 * @param bitbucket_p - The bitbucket to use when adding padding.
 *
 * @return
 *  The pointer to the next unused entry in the passed in
 *  dest_sg_p.  If we hit an error, then NULL will be returned.
 *
 * HISTORY:
 *  10/22/07 - Created. RPF
 *
 ****************************************************************/
fbe_sg_element_t * 
fbe_ldo_setup_sgs_for_optimal_block_size(fbe_sg_element_t *const source_sg_p,
                                         fbe_sg_element_t *const dest_sg_p,
                                         fbe_u32_t offset_to_first_optimal_block,
                                         fbe_u32_t optimal_block_size,
                                         fbe_u32_t pad_bytes,
                                         fbe_u32_t total_bytes,
                                         fbe_ldo_bitbucket_t *const bitbucket_p)
{
    fbe_sg_element_t *current_dest_sg_p = dest_sg_p;
    fbe_sg_element_t *current_src_sg_p = source_sg_p;
    fbe_u32_t bytes_left_in_opt_blk = FBE_MIN(offset_to_first_optimal_block,
                                              total_bytes);
    fbe_sg_element_t *sg_p = source_sg_p;
    fbe_u32_t bytes_remaining_in_sg = fbe_sg_element_count(sg_p);
    fbe_u32_t sg_offset = 0;
    fbe_u32_t total_bytes_remaining = total_bytes;

    /* If the sg list passed in is NULL, then we simply return
     * 0, which indicates error.
     */
    if (source_sg_p == NULL || dest_sg_p == NULL || total_bytes == 0)
    {
        return sg_p;
    }
    else if ( total_bytes_remaining <= bytes_left_in_opt_blk )
    {
        /* We have less than a full optimal block total, so simply setup the sg entries.
         * We assume padding only is at end of optimal block. 
         * In future we might want to allow padding at front of optimal block if offset not zero. 
         */
        current_dest_sg_p = fbe_sg_element_copy_list(current_src_sg_p,    /* source. */
                                                     current_dest_sg_p,    /* destination */
                                                     total_bytes_remaining );
        return current_dest_sg_p;
    }

    /* Simply iterate over the sg list, counting the number of entries.
     * We stop counting if we hit a zero count or NULL address.
     * We also stop counting sgs when we have exhausted the byte count.
     */
    while (fbe_sg_element_count(sg_p) != 0 && 
           fbe_sg_element_address(sg_p) != NULL &&
           total_bytes_remaining > 0)
    {
        /* Copy client data (and padding) for the next optimal block.
         */
        total_bytes_remaining -= bytes_left_in_opt_blk;

        /* Loop over all the sgs needed to fill out this optimal block with client data.
         */
        while ( bytes_left_in_opt_blk > 0 )
        {
            if ( bytes_remaining_in_sg >= bytes_left_in_opt_blk )
            {
                /* This entry has more than enough. Copy the bytes remaining
                 * in the optimal block, and set bytes remaining in our sg to what is left.
                 */
                current_dest_sg_p = 
                fbe_sg_element_copy_list_with_offset(current_src_sg_p,    /* source. */
                                                     current_dest_sg_p,    /* destination */
                                                     bytes_left_in_opt_blk,
                                                     sg_offset);
                bytes_remaining_in_sg -= bytes_left_in_opt_blk;
                sg_offset += bytes_left_in_opt_blk;
                bytes_left_in_opt_blk = 0;
            }
            else
            {
                /* This sg does not have enough to fill out the optimal block, 
                 * simply copy what is left in this sg. 
                 */
                current_dest_sg_p = 
                fbe_sg_element_copy_list_with_offset(current_src_sg_p,    /* source. */
                                                     current_dest_sg_p,    /* destination */
                                                     bytes_remaining_in_sg,
                                                     sg_offset);
                bytes_left_in_opt_blk -= bytes_remaining_in_sg;
                fbe_sg_element_increment(&current_src_sg_p);
                bytes_remaining_in_sg = fbe_sg_element_count(current_src_sg_p);
                sg_offset = 0;
            }
        }    /* end while bytes_left_in_opt_blk > 0 */

        /* Only add padding if we have more blocks to do.
         * Any padding at the end of the overall transfer is considered an edge 
         * and not padding. 
         * Since we are only concerned about setting up client data, we do not 
         * worry about the edge. 
         */
        if (total_bytes_remaining > 0)
        {
            /* We've just transitioned past an optimal block, just copy
             * in the sgs needed for padding.
             */
            current_dest_sg_p = 
                fbe_ldo_fill_sg_with_bitbucket(current_dest_sg_p, 
                                               pad_bytes,
                                               bitbucket_p);
        }

        /* Also, since we are starting a new optimal block,
         * reset the number of bytes in the optimal block size.
         */
        bytes_left_in_opt_blk = FBE_MIN(optimal_block_size, total_bytes_remaining);

    }    /* end while blocks remaining */

    if (total_bytes_remaining > 0)
    {
        /* Something is wrong since we should never have remaining bytes unless 
         * we hit the end of an sg list unexpectedly. 
         * We return NULL to indicate an error. 
         */
        return NULL;
    }
    return current_dest_sg_p;
}
/* end fbe_ldo_setup_sgs_for_optimal_block_size() */

/*!***************************************************************
 * @fn fbe_ldo_io_cmd_setup_sgs_for_client_data(
 *             fbe_logical_drive_io_cmd_t * const io_cmd_p,
 *             fbe_sg_element_t * const dest_sg_p,
 *             fbe_ldo_bitbucket_t *const bitbucket_p)
 ****************************************************************
 * @brief
 *  Setup the sgs needed for client data.
 *
 * @param io_cmd_p - The logical drive io cmd object to execute.
 * @param dest_sg_p - The sg list to setup.
 * @param bitbucket_p - The bitbucket to use when adding padding.
 *
 * @return fbe_sg_element_t - The pointer to the next entry that
 *    is free in this sg list.  Another way to say this is that we
 *    increment the destination sg ptr as we add the client data to
 *    the sg, and we return the ptr to the end of the sg list.
 * 
 *    If we hit some unexpected error, then we will return NULL.
 *
 * HISTORY:
 *  5/21/2008 - Created. RPF
 *
 ****************************************************************/

fbe_sg_element_t * 
fbe_ldo_io_cmd_setup_sgs_for_client_data(fbe_logical_drive_io_cmd_t * const io_cmd_p,
                                         fbe_sg_element_t * const dest_sg_p,
                                         fbe_ldo_bitbucket_t *const bitbucket_p)
{
    fbe_block_count_t block_count = fbe_ldo_io_cmd_get_blocks(io_cmd_p);
    fbe_block_size_t exported_block_size = fbe_ldo_io_cmd_get_block_size(io_cmd_p);
    fbe_u32_t bytes_to_copy = (fbe_u32_t)(block_count * exported_block_size);
    fbe_bool_t b_pad_needed = fbe_ldo_io_cmd_is_padding_required(io_cmd_p);
    fbe_sg_element_t *sg_p;
    fbe_sg_element_t * const client_sg_p = fbe_ldo_io_cmd_get_client_sg_ptr(io_cmd_p);

    /* Mappings that require padding have different imported and exported optimal block
     * sizes. We need to traverse the sg list and count exactly how many sgs are needed to
     * account for splitting sgs and adding fill. 
     */
    if (b_pad_needed == FBE_TRUE)
    {
        fbe_block_size_t exported_opt_blk_size = 
            fbe_ldo_io_cmd_get_optimum_block_size(io_cmd_p);
        fbe_lba_t lba = fbe_ldo_io_cmd_get_lba(io_cmd_p);

        /* The offset to the optimal block is the offset to the end of the first
         * exported block. 
         */
        fbe_u32_t offset_to_opt_blk = 
            (fbe_u32_t)(exported_opt_blk_size - (lba % exported_opt_blk_size));
        fbe_u32_t pad_bytes = fbe_ldo_calculate_pad_bytes(io_cmd_p);

        sg_p = fbe_ldo_setup_sgs_for_optimal_block_size(client_sg_p,/* source */
                                                        dest_sg_p, /* destination */
                                                        (offset_to_opt_blk * exported_block_size),
                                                        /* opt blk size in bytes */
                                                        (exported_opt_blk_size * exported_block_size),
                                                        pad_bytes,
                                                        bytes_to_copy,
                                                        bitbucket_p);
    }
    else
    {
        /* Optimize the case where the algorithm is not lossy,
         * Just copy the source to destination. 
         */
        sg_p = fbe_sg_element_copy_list(client_sg_p,    /* source. */
                                        dest_sg_p,    /* destination */
                                        bytes_to_copy );
    }

    return sg_p;
}
/******************************************
 * end fbe_ldo_io_cmd_setup_sgs_for_client_data() 
 ******************************************/

/*!***************************************************************
 * @fn fbe_ldo_io_cmd_read_setup_sg(fbe_logical_drive_io_cmd_t * const io_cmd_p)
 ****************************************************************
 * @brief
 *  This function sets up the sg list for a read operation.
 *
 * @param io_cmd_p - The logical drive io cmd object to execute.
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - no error.
 *                 Other values imply there was an error.
 *
 * HISTORY:
 *  10/22/07 - Created. RPF
 *
 ****************************************************************/
fbe_status_t 
fbe_ldo_io_cmd_read_setup_sg(fbe_logical_drive_io_cmd_t * const io_cmd_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_logical_drive_t * logical_drive_p = NULL;
    fbe_u32_t bytes_to_copy;
    fbe_sg_element_t * sg_p = fbe_ldo_io_cmd_get_sg_ptr(io_cmd_p);
    fbe_u32_t edge0_bytes = 0;
    fbe_u32_t edge1_bytes = 0;
    fbe_lba_t lba;
    fbe_block_count_t block_count = 0;
    fbe_block_size_t block_size = 0;

    logical_drive_p = fbe_ldo_io_cmd_get_logical_drive( io_cmd_p );

    lba = fbe_ldo_io_cmd_get_lba(io_cmd_p);
    block_count = fbe_ldo_io_cmd_get_blocks(io_cmd_p);
    block_size = fbe_ldo_io_cmd_get_block_size(io_cmd_p);
    
    /* The sg list we are creating will look like this.
     * Each |------| represents an fbe_sg_element_t 
     * 
     * |----------|  /|\ <-- Start of first edge sgs.
     * |----------|   | 0 or more entries for first edge.
     * |----------|   |
     * |----------|  \|/ <-- End   of first edge sgs.
     * |----------|  /|\ <-- Start of client sgs
     * |----------|   | 
     * |----------|   |   1 or more client sgs.
     * |----------|   |
     * |----------|  \|/ <-- End of client sgs
     * |----------|  /|\ <-- Start of last edge sgs.
     * |----------|   | 0 or more entries for last  edge.
     * |----------|   |
     * |----------|  \|/ <-- End   of last edge sgs. 
     * |----------|  Terminator. 
     */

    /* First get the edge sizes for the first and last edge.
     */
    fbe_ldo_get_edge_sizes( lba, block_count,
                            block_size,
                            fbe_ldo_io_cmd_get_optimum_block_size(io_cmd_p),
                            fbe_ldo_get_imported_block_size(logical_drive_p),
                            fbe_ldo_io_cmd_get_imported_optimum_block_size(io_cmd_p),
                            &edge0_bytes,
                            &edge1_bytes);
    
    /* Fill in the edge 0 bytes with the bitbucket if necessary.
     */
    if (edge0_bytes)
    {
        sg_p = fbe_ldo_fill_sg_with_bitbucket(sg_p, edge0_bytes,
                                              fbe_ldo_get_read_bitbucket_ptr());
    }

    /* Fill in the client portion of the sg list.
     */ 
    bytes_to_copy = (fbe_u32_t)(block_count * block_size);

    sg_p = fbe_ldo_io_cmd_setup_sgs_for_client_data(io_cmd_p,
                                                    sg_p /* destination */,
                                                    fbe_ldo_get_read_bitbucket_ptr());
    if ( sg_p == NULL )
    {    
        /* Something went wrong when setting up client data.
         */
        fbe_base_object_trace((fbe_base_object_t*)
                              fbe_ldo_io_cmd_get_logical_drive(io_cmd_p), 
                              FBE_TRACE_LEVEL_ERROR, 
                              FBE_TRACE_MESSAGE_ID_CREATE_OBJECT,
                              "LDO: Error setting up client data %s \n", __FUNCTION__);
        return(FBE_STATUS_GENERIC_FAILURE);
    }

    /* Fill in the edge 1 bytes with the bitbucket if necessary.
     */
    if (edge1_bytes)
    {
        sg_p = fbe_ldo_fill_sg_with_bitbucket(sg_p, edge1_bytes,
                                              fbe_ldo_get_read_bitbucket_ptr());
    }

    /* Terminate the sg and increment past the terminator since the following
     * checks expect the sg_p to be one past the end of the sg list. 
     */
    fbe_sg_element_terminate(sg_p);
    sg_p ++;
    
    if (((fbe_u32_t)(sg_p - fbe_ldo_io_cmd_get_sg_ptr(io_cmd_p))) >
        fbe_ldo_io_cmd_get_sg_entries(io_cmd_p))
    {
        /* If we did not allocate enough sgs return an error.
         */
        fbe_base_object_trace((fbe_base_object_t*)
                              fbe_ldo_io_cmd_get_logical_drive(io_cmd_p), 
                              FBE_TRACE_LEVEL_ERROR, 
                              FBE_TRACE_MESSAGE_ID_CREATE_OBJECT,
                              "LDO: Not enough sgs allocated.  %s \n", __FUNCTION__);
        status = FBE_STATUS_GENERIC_FAILURE;
    }
    
    if (((fbe_u32_t)(sg_p - fbe_ldo_io_cmd_get_sg_ptr(io_cmd_p))) <
        fbe_ldo_io_cmd_get_sg_entries(io_cmd_p))
    {
        /* If we allocated too many sgs return an error.
         */
        fbe_base_object_trace((fbe_base_object_t*)
                              fbe_ldo_io_cmd_get_logical_drive(io_cmd_p), 
                              FBE_TRACE_LEVEL_WARNING, 
                              FBE_TRACE_MESSAGE_ID_CREATE_OBJECT,
                              "LDO: Too many sgs allocated.  %s \n", __FUNCTION__);
        status = FBE_STATUS_GENERIC_FAILURE;
    }
    return status;
}
/* end fbe_ldo_io_cmd_read_setup_sg() */

/*!***************************************************************
 * @fn fbe_ldo_read_setup_sgs(   
 *                 fbe_logical_drive_t * const logical_drive_p, 
 *                 fbe_payload_ex_t * const payload_p,
 *                  fbe_u32_t edge0_bytes,
 *                  fbe_u32_t edge1_bytes)
 ****************************************************************
 * @brief
 *  This function sets up the sg list for a read operation where
 *  we are simply filling in the packet's pre and post ptrs.
 * 
 * @param logical_drive_p - The logical drive object.
 * @param payload_p - The ptr to the payload we are executing.
 * @param edge0_bytes - the number of bytes in the first edge.
 * @param edge1_bytes - the number of bytes in the last edge.
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - no error.
 *                 FBE_STATUS_INSUFFICIENT_RESOURCES - if we hit
 *                 an error where either the input sg was not
 *                 setup as expected or the sg list embedded in the
 *                 payload is not big enough.  In either case we
 *                 will fall back to going through the state machine
 *                 to allocate resources and handle the read.
 *
 * HISTORY:
 *  11/05/08 - Created. RPF
 *
 ****************************************************************/
fbe_status_t 
fbe_ldo_read_setup_sgs(fbe_logical_drive_t * const logical_drive_p,
                       fbe_payload_ex_t * const payload_p,
                       fbe_u32_t edge0_bytes,
                       fbe_u32_t edge1_bytes)
{
    fbe_u32_t client_bytes;
    fbe_sg_element_t *client_sg_p = NULL;
    fbe_payload_block_operation_t *block_operation_p = NULL;
    fbe_block_count_t blocks;
    fbe_block_size_t block_size;

    fbe_payload_ex_get_sg_list(payload_p, &client_sg_p, NULL);
    block_operation_p = fbe_payload_ex_get_block_operation(payload_p);

    /* Determine how many blocks are in the read.
     */
    fbe_payload_block_get_block_count(block_operation_p, &blocks);
    fbe_payload_block_get_block_size(block_operation_p, &block_size);
    client_bytes = (fbe_u32_t)(blocks * block_size);

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
    }

    /* There are three sg lists in the payload.
     * If we have a first edge or a last edge, we will simply fill in
     * the pre and post sgs lists with the first or last edge sg.
     *
     *  Payload
     *   pre_sg_list ----> First edge 
     *   sg_list --------> Client data 
     *   post_sg_list ---> Last edge
     *
     * Fill in the edge 0 bytes with the bitbucket if necessary.
     */
    if (edge0_bytes)
    {
        fbe_u32_t edge0_sgs;
        fbe_sg_element_t *pre_sg_p;
        edge0_sgs = fbe_ldo_bitbucket_get_sg_entries(edge0_bytes);

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

        fbe_payload_ex_get_pre_sg_list(payload_p, &pre_sg_p);

        /* Fill in the pre sg list with the pre-read data.
         */
        pre_sg_p = fbe_ldo_fill_sg_with_bitbucket(pre_sg_p, edge0_bytes,
                                                  fbe_ldo_get_read_bitbucket_ptr());

        /* Terminate the sg.
         */
        fbe_sg_element_terminate(pre_sg_p);
    }
    
    /* Fill in the edge 1 bytes with the bitbucket if necessary.
     */
    if (edge1_bytes)
    {
        fbe_u32_t edge1_sgs;
        fbe_sg_element_t *post_sg_p;
        edge1_sgs = fbe_ldo_bitbucket_get_sg_entries(edge1_bytes);

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

        /* Fill in the post sg list with the bitbucket memory.
         */
        post_sg_p = fbe_ldo_fill_sg_with_bitbucket(post_sg_p, edge1_bytes,
                                                   fbe_ldo_get_read_bitbucket_ptr());

        /* Terminate the sg.
         */
        fbe_sg_element_terminate(post_sg_p);
    }
    
    return FBE_STATUS_OK;
}
/* end fbe_ldo_read_setup_sgs() */

/*******************************
 * end fbe_logical_drive_read.c
 ******************************/
