/***************************************************************************
 * Copyright (C) EMC Corporation 2007-2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_logical_drive_io.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the logical drive object code for handling io packets.
 * 
 * @ingroup logical_drive_class_files
 * 
 * HISTORY
 *   10/30/2007:  Created. RPF
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_logical_drive.h"
#include "fbe_logical_drive_private.h"
#include "fbe_logical_drive_io_command.h"

/*****************************************
 * LOCAL FUNCTION DEFINITIONS
 *****************************************/

void fbe_ldo_io_state_machine(fbe_logical_drive_io_cmd_t *const io_cmd_p);

/*!**************************************************************
 * @fn fbe_ldo_resources_required(
 *                           fbe_logical_drive_t *logical_drive_p, 
 *                           fbe_packet_t * packet_p)
 ****************************************************************
 * @brief
 *  This function returns TRUE if we will need to allocate
 *  additional resources (like an sg list) in order to handle
 *  this request.  Normally, the packet contains enough resources
 *  to process a request, however there are some lossy cases where
 *  we cannot avoid allocating a resource.
 *
 * @param logical_drive_p - The logical drive.
 * @param packet_p - The read io packet that is arriving.
 *
 * @return fbe_bool_t - FBE_TRUE if resources required.
 *                      FBE_FALSE if no additional resources required.
 *
 * HISTORY:
 *  10/9/2008 - Created. RPF
 *
 ****************************************************************/

fbe_status_t
fbe_ldo_resources_required(fbe_logical_drive_t *logical_drive_p, 
                           fbe_packet_t * packet_p)
{
    fbe_bool_t b_padding_required;
    fbe_lba_t lba;
    fbe_block_count_t blocks;
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

    block_edge = (fbe_block_edge_t *)fbe_transport_get_edge(packet_p);
    if (block_edge == NULL)
    {
        fbe_payload_block_get_block_size(block_operation_p, &exported_block_size);
        fbe_payload_block_get_optimum_block_size(block_operation_p, &exported_opt_block_size);
        fbe_block_transport_edge_get_exported_block_size(&logical_drive_p->block_edge, &imported_block_size);
    }
    else
    {
        fbe_block_transport_edge_get_exported_block_size(block_edge, &exported_block_size);
        fbe_block_transport_edge_get_optimum_block_size(block_edge, &exported_opt_block_size);
        imported_block_size = fbe_ldo_get_imported_block_size(logical_drive_p);
    }

    imported_opt_block_size = 
        fbe_ldo_calculate_imported_optimum_block_size(exported_block_size,
                                                      exported_opt_block_size,
                                                      imported_block_size);

    b_padding_required = fbe_ldo_is_padding_required(exported_block_size,
                                                     exported_opt_block_size,
                                                     imported_block_size,
                                                     imported_opt_block_size);

    /*! If padding is required, then it means that we will need to allocate an 
     * sg list. 
     */
    if (b_padding_required)
    {
       /*! When padding is required, we always need to allocate an sg. 
        * Transition to the state for allocating an sg. 
        */
       return FBE_TRUE;
    }
    /* No resources are required.
     */
    return FBE_FALSE;
}
/******************************************
 * end fbe_ldo_resources_required()
 ******************************************/

/*!***************************************************************
 * @fn fbe_ldo_io_cmd_invalid_opcode(fbe_logical_drive_io_cmd_t *const io_cmd_p)
 ****************************************************************
 * @brief
 *  This function handles the state where an invalid operation
 *  was detected.
 *
 * @param io_cmd_p - The logical drive io cmd object to execute.
 *
 * @return
 *  FBE_LDO_STATE_STATUS_DONE
 *
 * HISTORY:
 *  10/22/07 - Created. RPF
 *
 ****************************************************************/
fbe_ldo_state_status_enum_t 
fbe_ldo_io_cmd_invalid_opcode(fbe_logical_drive_io_cmd_t *const io_cmd_p)
{
    /* The transport status is OK since this was delivered OK.
     */
    fbe_transport_set_status(fbe_ldo_io_cmd_get_packet(io_cmd_p), 
                              FBE_STATUS_OK, 0);

    /* The payload status is bad since this operation is not supported.
     */
    fbe_ldo_set_payload_status(fbe_ldo_io_cmd_get_packet(io_cmd_p),
                               FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID_REQUEST,
                               FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_UNSUPPORTED_OPCODE);
    fbe_ldo_complete_packet(fbe_ldo_io_cmd_get_packet(io_cmd_p));
    
    return FBE_LDO_STATE_STATUS_DONE;
}
/* end fbe_ldo_io_cmd_invalid_opcode() */

/*!***************************************************************
 * fbe_ldo_io_cmd_get_first_state(
 *                        fbe_payload_block_operation_t *block_command_p)
 ****************************************************************
 * @brief
 *  This function determines the first state for this
 *  request based upon the input opcode.
 *
 * @param block_command_p - The io block ptr.
 *
 * @return fbe_ldo_io_cmd_state_t
 *  the first state to execute for this given opcode.
 *
 * HISTORY:
 *  10/22/07 - Created. RPF
 *
 ****************************************************************/
fbe_ldo_io_cmd_state_t
fbe_ldo_io_cmd_get_first_state(fbe_payload_block_operation_t *block_command_p)
{
    fbe_payload_block_operation_opcode_t opcode;
    fbe_ldo_io_cmd_state_t new_state;

    /* Get opcode from the payload.
     */
    fbe_payload_block_get_opcode(block_command_p, &opcode);
    
    /* Set the new_state based on the opcode.
     */
    switch (opcode)
    {
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ:
            new_state = fbe_ldo_io_cmd_read_state0;
            break;
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE:
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_VERIFY:
            new_state = fbe_ldo_io_cmd_write_state0;
            break;
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_SAME:
            new_state = fbe_ldo_io_cmd_write_same_state0;
            break;
        default:
            /* This opcode is unexpected, so we will transition
             * to the state where we return invalid status.
             */
            new_state = fbe_ldo_io_cmd_invalid_opcode;
            break;
    };
    
    return new_state;
}
/* end fbe_ldo_io_cmd_get_first_state() */

/*!***************************************************************
 * @fn fbe_ldo_io_cmd_generate(fbe_logical_drive_io_cmd_t *const io_cmd_p)
 ****************************************************************
 * @brief
 *  This function generates an I/O command.
 *  It determines the algorithm and sets the next state.
 *  It also performs a validation of the command.
 *
 * @param io_cmd_p - The logical drive io cmd object to execute.
 *
 * @return
 *  FBE_LDO_STATE_STATUS_EXECUTING
 *
 * HISTORY:
 *  10/22/07 - Created. RPF
 *
 ****************************************************************/
fbe_ldo_state_status_enum_t 
fbe_ldo_io_cmd_generate(fbe_logical_drive_io_cmd_t *const io_cmd_p)
{
    fbe_ldo_io_cmd_state_t next_state;
    fbe_packet_t *packet_p = fbe_ldo_io_cmd_get_packet(io_cmd_p);
    fbe_payload_ex_t *payload_p = fbe_transport_get_payload_ex(packet_p);
    fbe_payload_block_operation_t *block_operation_p = fbe_payload_ex_get_block_operation(payload_p);

    /* Next, calculate the optimal block size and save it in the io command.
     */
    fbe_block_size_t imported_optimum_block_size;
    imported_optimum_block_size = 
        fbe_ldo_calculate_imported_optimum_block_size(
            fbe_ldo_io_cmd_get_block_size(io_cmd_p),
            fbe_ldo_io_cmd_get_optimum_block_size(io_cmd_p),
            fbe_ldo_get_imported_block_size(fbe_ldo_io_cmd_get_logical_drive(io_cmd_p)));
    fbe_ldo_io_cmd_set_imported_optimum_block_size(io_cmd_p,
                                                   imported_optimum_block_size);

    /* Init our state based on the operation.
     */
    next_state = fbe_ldo_io_cmd_get_first_state(block_operation_p);
    fbe_ldo_io_cmd_set_state(io_cmd_p, next_state);
    
    return FBE_LDO_STATE_STATUS_EXECUTING;
}
/* end fbe_ldo_io_cmd_generate() */

/*!***************************************************************
 * @fn fbe_ldo_io_cmd_init(fbe_logical_drive_io_cmd_t *const io_cmd_p,
 *                         fbe_object_handle_t logical_drive_p,
 *                         fbe_packet_t * packet_p)
 ****************************************************************
 * @brief
 *  This function initializes an io command object.
 *  It associates an I/O command with the logical drive object
 *  and initializes the fields in the io cmd.
 *
 * @param io_cmd_p - Io command to initialize.
 * @param logical_drive_p - Logical drive to init on. 
 * @param packet_p - The IO packet to init for. 
 *
 * @return
 *  None.
 *
 * HISTORY:
 *  10/22/07 - Created. RPF
 *
 ****************************************************************/
void
fbe_ldo_io_cmd_init( fbe_logical_drive_io_cmd_t *const io_cmd_p,
                     fbe_object_handle_t logical_drive_p, 
                     fbe_packet_t * packet_p )
{
 /* First we init the parts of the io cmd that are dependent
 * on the IoPacket interface.
 */
    /* Initialize the io packet we will be performing work for.
     */
    fbe_ldo_io_cmd_set_packet(io_cmd_p, packet_p);

/* Next we init the parts of the io cmd that are independent of 
 * the interface.
 */
    /* Initialize our object handle to associate this
     * io cmd with a given logical drive.
     */
    fbe_ldo_io_cmd_set_object_handle(io_cmd_p, logical_drive_p);

    /* Init our state to start generation.
     */
    fbe_ldo_io_cmd_set_state(io_cmd_p, fbe_ldo_io_cmd_generate);
    
    /* Clear out other values.
     */
    fbe_ldo_io_cmd_set_sg_entries(io_cmd_p, 0);

    return;
}
/* end fbe_ldo_io_cmd_init() */

/*!***************************************************************
 * @fn fbe_ldo_io_cmd_destroy(fbe_logical_drive_io_cmd_t *const io_cmd_p)
 ****************************************************************
 * @brief
 *  This function destroys an io command object.
 *
 * @param io_cmd_p - Io command to destroy.
 *
 * @return
 *  None.
 *
 * HISTORY:
 *  10/22/07 - Created. RPF
 *
 ****************************************************************/
void
fbe_ldo_io_cmd_destroy( fbe_logical_drive_io_cmd_t *const io_cmd_p )
{
    /* Free up any resources associated with the io command
     * including the io command itself. 
     */
    fbe_ldo_io_cmd_free_resources(io_cmd_p);
    fbe_ldo_io_cmd_free(io_cmd_p);
    return;
}
/* end fbe_ldo_io_cmd_destroy() */

/*!***************************************************************
 * @fn fbe_ldo_io_state_machine(fbe_logical_drive_io_cmd_t *const io_cmd_p)
 ****************************************************************
 * @brief
 *  This function executes the state machine for the
 *  logical drive io object.
 *
 * @param io_cmd_p - The logical drive io cmd object to execute.
 *
 * @return
 *  NONE.
 *
 * HISTORY:
 *  10/22/07 - Created. RPF
 *
 ****************************************************************/
void
fbe_ldo_io_state_machine(fbe_logical_drive_io_cmd_t *const io_cmd_p)
{
    fbe_ldo_io_cmd_state_t state;
    fbe_ldo_state_status_enum_t state_status;

    /* Execute the state of this object until it does not return
     * the executing state.
     */
    do
    {
        /* Get the state for the next pass of the loop.
         */
        state = fbe_ldo_io_cmd_get_state(io_cmd_p);

        /* Execute the state, which returns the status we will check below.
         */
        state_status = state(io_cmd_p);
    }
    while ( FBE_LDO_STATE_STATUS_EXECUTING == state_status );
    
    return;
}
/* end fbe_ldo_io_state_machine() */

/*!***************************************************************
 * @fn fbe_ldo_packet_completion(
 *                       fbe_packet_t * packet_p,
 *                       fbe_packet_completion_context_t context)
 ****************************************************************
 * @brief
 *  This function is the completion function for io packets
 *  sent out by the logical drive.
 * 
 *  All we need to do is copy status and unwind the payload.
 *
 * @param packet_p - The packet being completed.
 * @param context - The logical drive object.
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK.
 *
 * HISTORY:
 *  10/26/07 - Created. RPF
 *
 ****************************************************************/
fbe_status_t 
fbe_ldo_packet_completion(fbe_packet_t * packet_p, fbe_packet_completion_context_t context)
{
    fbe_logical_drive_t *logical_drive_p = (fbe_logical_drive_t * ) context;
    fbe_payload_ex_t * payload_p = NULL;
    fbe_payload_block_operation_t *block_operation_p = NULL;
    fbe_payload_block_operation_t *sub_block_operation_p = NULL;
    fbe_payload_block_operation_status_t block_status;
    fbe_payload_block_operation_qualifier_t block_qualifier;
    fbe_time_t retry_wait_msecs;
    fbe_lba_t media_error_lba;
    //fbe_object_id_t pdo_object_id;
    fbe_block_edge_t * block_edge = NULL;

    payload_p = fbe_transport_get_payload_ex(packet_p);
    sub_block_operation_p = fbe_payload_ex_get_block_operation(payload_p);

    /* Get the status from the lower payload. */
    fbe_payload_block_get_status(sub_block_operation_p, &block_status);
    fbe_payload_block_get_qualifier(sub_block_operation_p, &block_qualifier);
    fbe_payload_ex_get_media_error_lba(payload_p, &media_error_lba);
    fbe_payload_ex_get_retry_wait_msecs(payload_p, &retry_wait_msecs);

    //fbe_payload_block_get_pdo_object_id(sub_block_operation_p, &pdo_object_id);
    /* Unwind the playload stack so we can get to the client's payload.
     */
    fbe_payload_ex_release_block_operation(payload_p, sub_block_operation_p);

    /* Set the status in the client's payload.
     */
    block_operation_p = fbe_payload_ex_get_block_operation(payload_p);
    fbe_payload_block_set_status(block_operation_p, block_status, block_qualifier);
    //fbe_payload_block_set_retry_wait_msecs(block_operation_p, retry_wait_msecs);
    //fbe_payload_block_set_pdo_object_id(block_operation_p, pdo_object_id);

    /* If we received a media error, then translate the media error lba from the imported 
     * block size back to the client's block size. 
     */
    if (block_status == FBE_PAYLOAD_BLOCK_OPERATION_STATUS_MEDIA_ERROR)
    {
        fbe_block_size_t exported_block_size;
        fbe_block_size_t exported_opt_block_size;
        fbe_block_size_t imported_block_size;
        fbe_block_size_t imported_opt_block_size;
        fbe_lba_t exported_lba;

        block_edge = &logical_drive_p->block_edge;
        fbe_block_transport_edge_get_exported_block_size(block_edge, &exported_block_size);
        fbe_block_transport_edge_get_optimum_block_size(block_edge, &exported_opt_block_size);

        imported_block_size = fbe_ldo_get_imported_block_size(logical_drive_p);
        imported_opt_block_size = 
            fbe_ldo_calculate_imported_optimum_block_size(exported_block_size,
                                                          exported_opt_block_size,
                                                          imported_block_size);

        fbe_payload_block_get_lba(block_operation_p, &exported_lba);

        /* If we encountered a media error, then map the media error lba 
         * from the imported block to the exported block.
         */
        media_error_lba = fbe_ldo_get_media_error_lba(media_error_lba, exported_lba,
                                                      exported_block_size, 
                                                      exported_opt_block_size, 
                                                      imported_block_size, 
                                                      imported_opt_block_size); 
        /* Set this into the block payload.
         */
        fbe_payload_ex_set_media_error_lba(payload_p, media_error_lba);
    }
    return FBE_STATUS_OK;
}
/* end fbe_ldo_packet_completion() */

/*!***************************************************************
 * @fn fbe_ldo_io_cmd_packet_completion(
 *                       fbe_packet_t * packet_p,
 *                       fbe_packet_completion_context_t context)
 ****************************************************************
 * @brief
 *  This function is the completion function for io packets
 *  sent out by the logical drive.
 * 
 *  All we need to do is kick off the state machine for
 *  the io command that is completing.
 *
 * @param packet_p - The packet being completed.
 * @param context - The ldo io cmd operation.
 *
 * @return
 *  fbe_status_t - FBE_STATUS_MORE_PROCESSING_REQUIRED.
 *
 * HISTORY:
 *  10/26/07 - Created. RPF
 *
 ****************************************************************/
fbe_status_t 
fbe_ldo_io_cmd_packet_completion(fbe_packet_t * packet_p, 
                                 fbe_packet_completion_context_t context)
{
    fbe_logical_drive_io_cmd_t *io_cmd_p = (fbe_logical_drive_io_cmd_t * ) context;

    /* Simply start the operation into the state machine.
     */
    fbe_ldo_io_state_machine(io_cmd_p);
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/* end fbe_ldo_io_cmd_packet_completion() */

/*!***************************************************************
 * @fn fbe_ldo_build_sub_packet(fbe_packet_t * const packet_p,
 *                              fbe_payload_block_operation_opcode_t opcode,
 *                              fbe_lba_t lba,
 *                              fbe_block_count_t blocks,
 *                              fbe_block_size_t block_size,
 *                              fbe_block_size_t optimal_block_size,
 *                              fbe_sg_element_t * const sg_p,
 *                              fbe_packet_completion_context_t context)
 ****************************************************************
 * @brief
 *  This function builds a sub packet with the given parameters.
 * 
 * @param packet_p - Packet to fill in.
 * @param opcode - The opcode to set in the packet.
 * @param lba - The logical block address to put in the packet.
 * @param blocks - The block count.
 * @param block_size - The block size in bytes.
 * @param optimal_block_size - The optimal block size in blocks.
 * @param sg_p - Pointer to the sg list.
 * @param context - The context for the callback.
 *
 * @return
 *  NONE
 *
 * HISTORY:
 *  11/05/07 - Created. RPF
 *
 ****************************************************************/
void fbe_ldo_build_sub_packet(fbe_packet_t * const packet_p,
                              fbe_payload_block_operation_opcode_t opcode,
                              fbe_lba_t lba,
                              fbe_block_count_t blocks,
                              fbe_block_size_t block_size,
                              fbe_block_size_t optimal_block_size,
                              fbe_sg_element_t * const sg_p,
                              fbe_packet_completion_context_t context)
{
    fbe_payload_ex_t *payload_p = NULL;
    fbe_payload_block_operation_t *block_operation_p = NULL;
    fbe_payload_sg_descriptor_t *sg_desc_p = NULL;
    /* Initialize the packet and set the completion function.
     */
    fbe_transport_set_completion_function(packet_p,
                                           fbe_ldo_io_cmd_packet_completion,
                                           context);

    payload_p = fbe_transport_get_payload_ex(packet_p);
    block_operation_p = fbe_payload_ex_allocate_block_operation(payload_p);

    fbe_payload_ex_set_sg_list(payload_p, sg_p, 0);

    /* First build the block operation.
     */
    fbe_payload_block_build_operation(block_operation_p,
                                      opcode,
                                      lba,
                                      blocks,
                                      block_size,
                                      optimal_block_size,
                                      NULL    /* no pre-read descriptor. */);

    /* Default our repeat count to 1.
     */
    fbe_payload_block_get_sg_descriptor(block_operation_p, &sg_desc_p);
    sg_desc_p->repeat_count = 1;
    return;
}
/* end fbe_ldo_build_sub_packet() */

/*!***************************************************************
 * @fn fbe_ldo_build_packet(fbe_packet_t * const packet_p,
 *                          fbe_payload_block_operation_opcode_t opcode,
 *                          fbe_lba_t lba,
 *                          fbe_block_count_t blocks,
 *                          fbe_block_size_t block_size,
 *                          fbe_block_size_t optimal_block_size,
 *                          fbe_sg_element_t * const sg_p,
 *                     fbe_packet_completion_function_t completion_function,   
 *                          fbe_packet_completion_context_t context,
 *                          fbe_u32_t repeat_count)
 ****************************************************************
 * @brief
 *  This function builds a packet with the given parameters.
 * 
 * @param packet_p - Packet to fill in.
 * @param opcode - The opcode to set in the packet.
 * @param lba - The logical block address to put in the packet.
 * @param blocks - The block count.
 * @param block_size - The block size in bytes.
 * @param optimal_block_size - The optimal block size in blocks.
 * @param sg_p - Pointer to the sg list.
 * @param completion_function - The callback function.
 * @param context - The context for the callback.
 * @param repeat_count - The repeat count for the sg descriptor.
 *
 * @return
 *  NONE
 *
 * HISTORY:
 *  10/09/08 - Created. RPF
 *
 ****************************************************************/
void fbe_ldo_build_packet(fbe_packet_t * const packet_p,
                          fbe_payload_block_operation_opcode_t opcode,
                          fbe_lba_t lba,
                          fbe_block_count_t blocks,
                          fbe_block_size_t block_size,
                          fbe_block_size_t optimal_block_size,
                          fbe_sg_element_t * const sg_p,
                          fbe_packet_completion_function_t completion_function,
                          fbe_packet_completion_context_t context,
                          fbe_u32_t repeat_count)
{
    fbe_payload_ex_t *payload_p = NULL;
    fbe_payload_block_operation_t *block_operation_p = NULL;
    fbe_payload_sg_descriptor_t *sg_desc_p = NULL;

    /* Initialize the packet and set the completion function.
     */
    fbe_transport_set_completion_function(packet_p,
                                           completion_function,
                                           context);

    payload_p = fbe_transport_get_payload_ex(packet_p);
    block_operation_p = fbe_payload_ex_allocate_block_operation(payload_p);

    if (sg_p != NULL)
    {
        /* Optionally set the sg_p.
         */
        fbe_payload_ex_set_sg_list(payload_p, sg_p, 0);
    }

    /* First build the block operation.
     */
    fbe_payload_block_build_operation(block_operation_p,
                                      opcode,
                                      lba,
                                      blocks,
                                      block_size,
                                      optimal_block_size,
                                      NULL    /* no pre-read descriptor. */);
    fbe_payload_block_get_sg_descriptor(block_operation_p, &sg_desc_p);

    /* Sets the repeat count to the passed in value.
     */
    sg_desc_p->repeat_count = repeat_count;
    return;
}
/* end fbe_ldo_build_packet() */
/******************************
 * end fbe_logical_drive_io.c
 ******************************/
