/***************************************************************************
 * Copyright (C) EMC Corporation 2007-2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_logical_drive_io_cmd.c
 ***************************************************************************
 *
 * @brief
 *  This file contains functions of the logical drive io command object.
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

/*!***************************************************************
 * @fn fbe_ldo_io_cmd_map_lba_blocks(
 *                      fbe_logical_drive_io_cmd_t * const io_cmd_p,
 *                      fbe_lba_t *const output_lba_p,
 *                      fbe_block_count_t *const output_blocks_p)
 ****************************************************************
 * @brief
 *  This function maps an lba and block count for
 *  an io command object.
 *
 * @param io_cmd_p - The logical drive io cmd object to map for.
 * @param output_lba_p - Pointer to output lba.
 * @param output_blocks_p - Pointer to the output block count.
 *
 * @return
 *  NONE.
 *
 * HISTORY:
 *  11/08/07 - Created. RPF
 *
 ****************************************************************/
void 
fbe_ldo_io_cmd_map_lba_blocks(fbe_logical_drive_io_cmd_t * const io_cmd_p,
                              fbe_lba_t *const output_lba_p,
                              fbe_block_count_t *const output_blocks_p)
{
    fbe_logical_drive_t * logical_drive_p = 
        fbe_ldo_io_cmd_get_logical_drive( io_cmd_p );

    /* Simply map the given parameters from the io command from
     * the exported to the imported block size.
     */
    fbe_ldo_map_lba_blocks(fbe_ldo_io_cmd_get_lba( io_cmd_p ),
                           fbe_ldo_io_cmd_get_blocks( io_cmd_p ),
                           fbe_ldo_io_cmd_get_block_size( io_cmd_p ),
                           fbe_ldo_io_cmd_get_optimum_block_size( io_cmd_p ),
                           fbe_ldo_get_imported_block_size(logical_drive_p),
                           fbe_ldo_io_cmd_get_imported_optimum_block_size(io_cmd_p),
                           output_lba_p,
                           output_blocks_p);
    return;
}
/* end fbe_ldo_io_cmd_map_lba_blocks() */

/*!***************************************************************
 * @fn fbe_ldo_io_cmd_is_aligned(fbe_logical_drive_io_cmd_t * const io_cmd_p)
 ****************************************************************
 * @brief
 *  This function determines if the command is aligned
 *  and does not have edges.
 *
 * @param io_cmd_p - The logical drive io cmd object to check.
 *
 * @return
 *  TRUE if aligned to optimal block.
 *  FALSE if not aligned to optimal block.
 *
 * HISTORY:
 *  11/08/07 - Created. RPF
 *
 ****************************************************************/
fbe_bool_t 
fbe_ldo_io_cmd_is_aligned(fbe_logical_drive_io_cmd_t * const io_cmd_p)
{
    /* Fetch information from the io command about this operation.
     */
    fbe_lba_t lba = fbe_ldo_io_cmd_get_lba( io_cmd_p );
    fbe_block_count_t blocks = fbe_ldo_io_cmd_get_blocks( io_cmd_p );
    fbe_block_size_t opt_blk_size = 
        fbe_ldo_io_cmd_get_optimum_block_size( io_cmd_p );

    /* Use our helper fn to determine if we are aligned.
     */
    return fbe_ldo_io_is_aligned( lba, blocks, opt_blk_size );
}
/* end fbe_ldo_io_cmd_is_aligned() */

/*!***************************************************************
 * @fn fbe_ldo_io_cmd_is_sg_required(fbe_logical_drive_io_cmd_t * const io_cmd_p)
 ****************************************************************
 * @brief
 *  This function determines if the
 *  command requires an sg.  Only padding
 *  cases require an sg.
 *
 * @param io_cmd_p - The logical drive io cmd object to check.
 *
 * @return
 *  TRUE if aligned to optimal block.
 *  FALSE if not aligned to optimal block.
 *
 * HISTORY:
 *  06/16/08 - Created. RPF
 *
 ****************************************************************/
fbe_bool_t 
fbe_ldo_io_cmd_is_sg_required(fbe_logical_drive_io_cmd_t * const io_cmd_p)
{    
    if ( fbe_ldo_io_cmd_is_padding_required(io_cmd_p) )
    {
        /* If padding is needed, then it is not possible to have an I/O that is
         * completely aligned, and thus an sg is always needed. 
         */
        return TRUE;
    }
    else
    {
        /* Use our helper fn to determine if we are aligned.
         * We only need an sg if we are not aligned. 
         */
        return(fbe_ldo_io_cmd_is_aligned(io_cmd_p) == FALSE); 
    }
}
/* end fbe_ldo_io_cmd_is_sg_required() */

/*!***************************************************************
 * @fn fbe_ldo_io_cmd_build_sub_packet(
 *                     fbe_logical_drive_io_cmd_t * const io_cmd_p,
 *                     fbe_payload_block_operation_opcode_t opcode,
 *                     fbe_lba_t imported_lba,
 *                     fbe_block_count_t imported_blocks,
 *                     fbe_block_size_t imported_optimal_block_size)
 ****************************************************************
 * @brief
 *  This function builds a sub packet for this io cmd.
 *
 * @param io_cmd_p - The logical drive io cmd object to check.
 * @param opcode - The opcode to set.
 * @param imported_lba - The logical block address to set.
 * @param imported_blocks - The block count to set.
 * @param imported_optimal_block_size - The imported optimal size to set.
 *
 * @return
 *  NONE
 *
 * HISTORY:
 *  11/05/07 - Created. RPF
 *
 ****************************************************************/
void fbe_ldo_io_cmd_build_sub_packet(fbe_logical_drive_io_cmd_t * const io_cmd_p,
                                     fbe_payload_block_operation_opcode_t opcode,
                                     fbe_lba_t imported_lba,
                                     fbe_block_count_t imported_blocks,
                                     fbe_block_size_t imported_optimal_block_size)
{
    fbe_packet_t *packet_p = NULL;
    fbe_logical_drive_t * logical_drive_p = NULL; 
    fbe_u32_t sg_entries;
    fbe_sg_element_t *sg_p = NULL;

    packet_p = fbe_ldo_io_cmd_get_packet(io_cmd_p);
    logical_drive_p = fbe_ldo_io_cmd_get_logical_drive( io_cmd_p );

    sg_entries = fbe_ldo_io_cmd_get_sg_entries(io_cmd_p);

    if (sg_entries == 0 || opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_VERIFY)
    {
        /* If the command is completely aligned, then we did not allocate an sg and
         * instead should use the one in the client's sg ptr.
         */
        sg_p = fbe_ldo_io_cmd_get_client_sg_ptr(io_cmd_p);
    }
    else
    {
        sg_p = fbe_ldo_io_cmd_get_sg_ptr(io_cmd_p);
    }
    /* Fill out the sub io packet for this command.
     */
    fbe_ldo_build_packet( packet_p,
                          opcode,
                          imported_lba,
                          imported_blocks,
                          fbe_ldo_get_imported_block_size(logical_drive_p),
                          imported_optimal_block_size,
                          sg_p,
                          /* completion function */
                          fbe_ldo_io_cmd_packet_completion,
                          /* Completion context is io command. */
                          (fbe_packet_completion_context_t) io_cmd_p,
                          1 /* repeat_count */ );
    return;
}
/* end fbe_ldo_io_cmd_build_sub_packet() */

/*!***************************************************************
 * @fn fbe_ldo_io_cmd_validate_pre_read_desc(
 *                     fbe_logical_drive_io_cmd_t * const io_cmd_p)
 ****************************************************************
 * @brief
 *  This function determines if an edge descriptor is valid
 *  for the given io cmd.
 *
 * @param io_cmd_p - The logical drive io cmd object to validate for.
 *
 * @return
 *  FBE_STATUS_OK on success.  Other values imply error.
 *
 * HISTORY:
 *  11/21/07 - Created. RPF
 *
 ****************************************************************/
fbe_status_t 
fbe_ldo_io_cmd_validate_pre_read_desc( fbe_logical_drive_io_cmd_t * const io_cmd_p )
{ 
    fbe_status_t status = FBE_STATUS_OK;
    fbe_logical_drive_t * logical_drive_p = 
        fbe_ldo_io_cmd_get_logical_drive( io_cmd_p );

    /* If the start and end are aligned, then the edge descriptor is ignored.
     */
    if (fbe_ldo_io_cmd_is_aligned(io_cmd_p))
    {
        status = FBE_STATUS_OK;
    }
    else
    {
        /* Determine the mapped lba and block count for the given io command.
         */
        fbe_lba_t imported_lba;
        fbe_block_count_t imported_blocks;
        fbe_ldo_io_cmd_map_lba_blocks( io_cmd_p, &imported_lba, &imported_blocks );

        /* Call our edge descriptor function to validate.
         */
        status = fbe_ldo_validate_pre_read_desc(fbe_ldo_io_cmd_get_pre_read_desc_ptr(io_cmd_p),
                                                fbe_ldo_io_cmd_get_block_size( io_cmd_p ),
                                                fbe_ldo_io_cmd_get_optimum_block_size( io_cmd_p ),
                                                fbe_ldo_get_imported_block_size( logical_drive_p ),
                                                fbe_ldo_io_cmd_get_imported_optimum_block_size(io_cmd_p),
                                                fbe_ldo_io_cmd_get_lba(io_cmd_p),
                                                fbe_ldo_io_cmd_get_blocks(io_cmd_p),
                                                imported_lba,
                                                imported_blocks);
    }
    return status;
}
/* end fbe_ldo_io_cmd_validate_pre_read_desc() */

/*!***************************************************************
 * @fn fbe_ldo_io_cmd_get_pre_read_desc_offsets(
 *               fbe_logical_drive_io_cmd_t * const io_cmd_p,
 *               fbe_u32_t *first_edge_offset_p,
 *               fbe_u32_t *last_edge_offset_p,
 *               fbe_u32_t first_edge_size)
 ****************************************************************
 * @brief
 *  This function returns the offsets into the edge descriptor
 *  sg list for the first and last edge.
 *
 * @param io_cmd_p - The logical drive io cmd object.
 * @param first_edge_offset_p - Offset in bytes from start of
 *                             sg list to the first edge.
 * @param last_edge_offset_p - Offset in bytes from start of 
 *                            sg list to the last edge.
 * @param first_edge_size - The size of the first edge.
 *
 * @return
 *  FBE_STATUS_OK on success.  Other values imply error.
 *
 * HISTORY:
 *  11/21/07 - Created. RPF
 *
 ****************************************************************/
fbe_status_t 
fbe_ldo_io_cmd_get_pre_read_desc_offsets( fbe_logical_drive_io_cmd_t * const io_cmd_p,
                                          fbe_u32_t *first_edge_offset_p,
                                          fbe_u32_t *last_edge_offset_p,
                                          fbe_u32_t first_edge_size )
{
    fbe_status_t status;
    fbe_logical_drive_t * logical_drive_p = 
    fbe_ldo_io_cmd_get_logical_drive( io_cmd_p );

    /* Just call the mapping function which calculates the edge sizes.
     */
    status = fbe_ldo_map_get_pre_read_desc_offsets(
        fbe_ldo_io_cmd_get_lba( io_cmd_p ),
        fbe_ldo_io_cmd_get_blocks( io_cmd_p ),
        fbe_ldo_io_cmd_get_pre_read_desc_lba(io_cmd_p),
        fbe_ldo_io_cmd_get_block_size(io_cmd_p),
        fbe_ldo_io_cmd_get_optimum_block_size( io_cmd_p ),
        fbe_ldo_get_imported_block_size(logical_drive_p),
        first_edge_size,
        first_edge_offset_p,
        last_edge_offset_p);
    return status;
}
/* end fbe_ldo_io_cmd_get_pre_read_desc_offsets() */

/*!***************************************************************
 * @fn fbe_ldo_io_cmd_finished(fbe_logical_drive_io_cmd_t * const io_cmd_p)
 ****************************************************************
 * @brief
 *  This function handle the terminal state of the io cmd object.
 *
 *  This state will free resources and complete the io packet.
 *
 * @param io_cmd_p - The logical drive io cmd object to execute. 
 *
 * @return
 *  FBE_LDO_STATE_STATUS_DONE
 *
 * HISTORY:
 *  11/07/07 - Created. RPF
 *
 ****************************************************************/
fbe_ldo_state_status_enum_t 
fbe_ldo_io_cmd_finished(fbe_logical_drive_io_cmd_t * const io_cmd_p)
{
    fbe_packet_t * packet_p = fbe_ldo_io_cmd_get_packet(io_cmd_p);

    /* Free up the resources.
     */
    fbe_ldo_io_cmd_destroy(io_cmd_p);

    /* Complete our io packet.
     */
    fbe_ldo_complete_packet(packet_p);

    /* Return done so that we stop executing this command.
     */
    return FBE_LDO_STATE_STATUS_DONE;
}
/* end fbe_ldo_io_cmd_finished() */

/*!***************************************************************
 * @fn fbe_ldo_io_cmd_send_io(fbe_logical_drive_io_cmd_t * const io_cmd_p)
 ****************************************************************
 * @brief
 *  This function sends an io to the next object.
 *
 * @param io_cmd_p - The logical drive io cmd object to send for.
 *
 * @return
 *  None.
 *
 * HISTORY:
 *  11/20/07 - Created. RPF
 *
 ****************************************************************/
fbe_status_t
fbe_ldo_io_cmd_send_io(fbe_logical_drive_io_cmd_t * const io_cmd_p)
{
    fbe_logical_drive_t * logical_drive_p = 
        fbe_ldo_io_cmd_get_logical_drive( io_cmd_p );
    fbe_status_t status;
    fbe_packet_t *packet_p;
    packet_p = fbe_ldo_io_cmd_get_packet(io_cmd_p);

    status = fbe_ldo_send_io_packet(logical_drive_p, packet_p);
    return status;
}
/* end fbe_ldo_io_cmd_send_io() */

/*!***************************************************************
 * @fn fbe_ldo_io_cmd_set_status(fbe_logical_drive_io_cmd_t * const io_cmd_p,
 *                               fbe_status_t status,
 *                               fbe_u32_t qualifier)
 ****************************************************************
 * @brief
 *  This function sets the packet status.
 *
 * @param io_cmd_p - The logical drive io cmd object to set status.
 * @param status - The status to set.
 * @param qualifier - The qualifier to set.
 *
 * @return
 *  None.
 *
 * HISTORY:
 *  11/20/07 - Created. RPF
 *
 ****************************************************************/
void 
fbe_ldo_io_cmd_set_status(fbe_logical_drive_io_cmd_t * const io_cmd_p,
                          fbe_status_t status,
                          fbe_u32_t qualifier)
{
    /* We hit an error on this command, set error now.
     */
    fbe_transport_set_status(fbe_ldo_io_cmd_get_packet(io_cmd_p), 
                              status, 
                              qualifier);
    
    return;
}
/* end fbe_ldo_io_cmd_set_status() */

/*!***************************************************************
 * @fn fbe_ldo_io_cmd_set_payload_status(
 *                    fbe_logical_drive_io_cmd_t * const io_cmd_p,
 *                    fbe_status_t status,
 *                    fbe_u32_t qualifier)
 ****************************************************************
 * @brief
 *  This function sets the payload status.
 *
 * @param io_cmd_p - The logical drive io cmd object to set status.
 * @param status - The status to set.
 * @param qualifier - The qualifier to set.
 *
 * @return
 *  None.
 *
 * HISTORY:
 *  11/20/07 - Created. RPF
 *
 ****************************************************************/
void 
fbe_ldo_io_cmd_set_payload_status(fbe_logical_drive_io_cmd_t * const io_cmd_p,
                                  fbe_status_t status,
                                  fbe_u32_t qualifier)
{
    /* We hit an error on this command, set error now.
     */
    fbe_ldo_set_payload_status(fbe_ldo_io_cmd_get_packet(io_cmd_p), 
                               status, 
                               qualifier);
    return;
}
/* end fbe_ldo_io_cmd_set_payload_status() */

/*!***************************************************************
 * @fn fbe_ldo_io_cmd_set_status_from_sub_io_packet(
 *                    fbe_logical_drive_io_cmd_t * const io_cmd_p)
 ****************************************************************
 * @brief
 *  This function sends an io to the next object.
 *
 * @param io_cmd_p - The logical drive io cmd object to set status.
 *
 * @return
 *  None.
 *
 * HISTORY:
 *  11/20/07 - Created. RPF
 *
 ****************************************************************/
void 
fbe_ldo_io_cmd_set_status_from_sub_io_packet(fbe_logical_drive_io_cmd_t * const io_cmd_p)
{
    fbe_payload_ex_t *payload_p = NULL;
    fbe_payload_block_operation_t *block_operation_p = NULL;
    fbe_packet_t *packet_p = fbe_ldo_io_cmd_get_packet(io_cmd_p);
    fbe_payload_block_operation_status_t block_status;
    fbe_payload_block_operation_qualifier_t block_qualifier;

    payload_p = fbe_transport_get_payload_ex(packet_p);
    block_operation_p = fbe_payload_ex_get_block_operation(payload_p);

    /* Also set the master payload status from the sub packet's payload status.
     */
    fbe_payload_block_get_status(block_operation_p, &block_status);
    fbe_payload_block_get_qualifier(block_operation_p, &block_qualifier);

    /* Now unwind the payload.
     */
    fbe_payload_ex_release_block_operation(payload_p, block_operation_p);
    block_operation_p = fbe_payload_ex_get_block_operation(payload_p);

    /* Set status in client's payload.
     */
    fbe_payload_block_set_status(block_operation_p, block_status, block_qualifier);
    return;
}
/* end fbe_ldo_io_cmd_set_status_from_sub_io_packet() */

/*******************************
 * end fbe_logical_drive_io_cmd.c
 ******************************/
