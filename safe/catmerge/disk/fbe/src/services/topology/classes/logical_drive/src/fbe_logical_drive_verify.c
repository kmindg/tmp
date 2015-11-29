/***************************************************************************
 * Copyright (C) EMC Corporation  2007-2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_logical_drive_verify.c
 ***************************************************************************
 *
 * @brief
 *  This file contains functions of the logical drive object's
 *  verify state machine.
 * 
 *  Verifies get handled by simply filling out the next payload and sending the
 *  packet down.
 * 
 *  Completion is also straightforward, we simply copy the block status to the
 *  client's payload and return.
 * 
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


/*!**************************************************************
 * @fn fbe_ldo_handle_verify(fbe_logical_drive_t *logical_drive_p, 
 *                           fbe_packet_t * packet_p)
 ****************************************************************
 * @brief
 *  This function is the entry point for verify
 *  operations to the logical drive object.
 *
 * @param logical_drive_p - The logical drive.
 * @param packet_p - The read io packet that is arriving.
 *
 * @return fbe_status_t - FBE_STATUS_OK.
 *
 * @version
 *  10/9/2008 - Created. RPF
 *
 ****************************************************************/

fbe_status_t
fbe_ldo_handle_verify(fbe_logical_drive_t *logical_drive_p, 
                      fbe_packet_t * packet_p)
{
    fbe_lba_t lba;
    fbe_block_count_t blocks;
    fbe_lba_t imported_lba;
    fbe_block_count_t imported_blocks;

    fbe_block_size_t exported_block_size;
    fbe_block_size_t exported_opt_block_size;
    fbe_block_size_t imported_block_size;
    fbe_block_size_t imported_opt_block_size;
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
    if (exported_opt_block_size == 0)
    {
        /* Something is wrong if the optimum block size is zero.
         */
        fbe_base_object_trace((fbe_base_object_t*)logical_drive_p, FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                              "logical drive: unexpected vr opt block sizes req:0x%x opt:0x%x Geometry:0x%x\n", 
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

    /* Determine the mapped lba and block count for
     * the given io command.
     */
    fbe_ldo_map_lba_blocks(lba, blocks, 
                           exported_block_size, exported_opt_block_size,
                           imported_block_size, imported_opt_block_size,
                           &imported_lba, &imported_blocks);

    /* Then initialize our packet for a read.
     */
    fbe_ldo_build_packet(packet_p,
                         FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_VERIFY,
                         imported_lba,
                         imported_blocks,
                         fbe_ldo_get_imported_block_size(logical_drive_p),
                         /* Imported optimal.
                          * This is always 1 since the physical drive 
                          * cannot perform block conversion. 
                          */
                         1,
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
 * end fbe_ldo_handle_verify()
 ******************************************/

/*******************************
 * fbe_logical_drive_verify.c
 ******************************/
