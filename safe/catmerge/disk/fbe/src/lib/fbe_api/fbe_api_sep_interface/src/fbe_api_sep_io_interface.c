/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_api_sep_io_interface.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the io interface for the storage extent package.
 *
 * @ingroup fbe_api_storage_extent_package_interface_class_files
 * @ingroup fbe_api_sep_io_interface
 *
 * @version
 *   5/22/2009:  Created. rfoley
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

#include "fbe/fbe_winddk.h"
#include "fbe/fbe_types.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_payload_block_operation.h"
#include "fbe/fbe_api_common_transport.h"
#include "fbe_block_transport.h"
#include "fbe/fbe_api_sep_io_interface.h"

/**************************************
                Local variables
**************************************/

/*******************************************
                Local functions
********************************************/
static fbe_status_t fbe_api_sep_send_block_payload_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);
static fbe_status_t fbe_api_sep_get_block_size_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);


/*!***************************************************************
 * @fn fbe_api_sep_io_interface_send_block_payload(
 *        fbe_object_id_t object_id,
 *        fbe_payload_block_operation_opcode_t opcode,
 *        fbe_lba_t lba,
 *        fbe_block_count_t blocks,
 *        fbe_block_size_t block_size,
 *        fbe_block_size_t opt_block_size,
 *        fbe_sg_element_t * const sg_p,
 *        fbe_payload_pre_read_descriptor_t * const pre_read_desc_p,
 *        fbe_packet_attr_t * const transport_qualifier_p,
 *        fbe_api_sep_block_status_values_t * const block_status_p)
 ****************************************************************
 * @brief
 *  This function sends a sep block command for the given params.
 *
 * @param object_id - Object being sent to.
 * @param opcode - Operation to use for this command.
 * @param lba - Start logical block address of command.
 * @param blocks - Total blocks for transfer.
 * @param block_size - Block size to use in command.
 * @param opt_block_size - Optimal block size to use in command.
 * @param sg_p - Scatter gather pointer.
 * @param pre_read_desc_p - The edge descriptor pointer for this cmd.
 * @param transport_qualifier_p - The qualifier from the transport.
 * @param block_status_p - The status of the block payload.
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  05/22/09 - Created. RPF
 *
 ****************************************************************/
fbe_status_t 
fbe_api_sep_io_interface_send_block_payload(fbe_object_id_t object_id,
                                            fbe_payload_block_operation_opcode_t opcode,
                                            fbe_lba_t lba,
                                            fbe_block_count_t blocks,
                                            fbe_block_size_t block_size,
                                            fbe_block_size_t opt_block_size,
                                            fbe_sg_element_t * const sg_p,
                                            fbe_payload_pre_read_descriptor_t * const pre_read_desc_p,
                                            fbe_packet_attr_t * const transport_qualifier_p,
                                            fbe_api_sep_block_status_values_t * const block_status_p)
{
    fbe_status_t status;
    fbe_packet_t *packet_p;
    fbe_block_edge_t edge;
    fbe_payload_ex_t * payload_p = NULL;
    fbe_payload_block_operation_t *block_operation_p;
    fbe_semaphore_t sem;

    fbe_semaphore_init(&sem, 0, 1);


    /* Initialize the status values to invalid values.
     */
    block_status_p->status = FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID;
    block_status_p->qualifier = FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_INVALID;
    block_status_p->media_error_lba = FBE_LBA_INVALID;
    block_status_p->pdo_object_id = FBE_OBJECT_ID_INVALID;

    fbe_semaphore_init(&sem, 0, 1);

    /* We fill in an edge, so that the packet will have an associated edge.
     */
    fbe_base_transport_set_transport_id((fbe_base_edge_t*)&edge, FBE_TRANSPORT_ID_BLOCK);
    edge.offset = 0;
    /* Allocate packet.  If the allocation fails, return an error.
     */
    packet_p = fbe_api_get_contiguous_packet();
    if (packet_p == NULL) 
    {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, 
                      "%s fbe_transport_allocate_packet fail", __FUNCTION__);   
        return FBE_STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Initialize and fill out the packet for this block command.
     */
    packet_p->base_edge = NULL;

    payload_p = fbe_transport_get_payload_ex(packet_p);
    block_operation_p = fbe_payload_ex_allocate_block_operation(payload_p);

    /* First build the block operation.
     * We also need to manually increment the payload level 
     * since we will not be sending this on an edge. 
     */
    status = fbe_payload_block_build_operation(block_operation_p,
                                               opcode,
                                               lba,
                                               blocks,
                                               block_size,
                                               opt_block_size,
                                               pre_read_desc_p);

    fbe_payload_ex_increment_block_operation_level(payload_p);

    /* Set the sg ptr into the packet.
     */
    status = fbe_payload_ex_set_sg_list(payload_p, sg_p, 0);

    fbe_transport_set_completion_function(packet_p, 
                                          fbe_api_sep_send_block_payload_completion, 
                                          &sem);

    fbe_transport_set_address(packet_p,
                              FBE_PACKAGE_ID_SEP_0,
                              FBE_SERVICE_ID_TOPOLOGY,
                              FBE_CLASS_ID_INVALID,
                              object_id); 

    /* the send will be implemented differently in user and kernel space
     */
    status = fbe_api_common_send_io_packet_to_driver(packet_p);

    if (status != FBE_STATUS_PENDING && status != FBE_STATUS_OK) {
        fbe_semaphore_destroy(&sem);
         /* Stash the block status and qualifier before returning.*/
        fbe_payload_block_get_status(block_operation_p, &block_status_p->status);
        fbe_payload_block_get_qualifier(block_operation_p, &block_status_p->qualifier);

        /* Free up the packet memory.*/
        fbe_api_return_contiguous_packet(packet_p);
        return status;
    }

    /*we block here and wait for the callback function to take the semaphore down*/
    fbe_semaphore_wait(&sem, NULL);
    fbe_semaphore_destroy(&sem);

    /* Stash the block status and qualifier before returning.
     */
    fbe_payload_block_get_status(block_operation_p, &block_status_p->status);
    fbe_payload_block_get_qualifier(block_operation_p, &block_status_p->qualifier);
    fbe_payload_ex_get_media_error_lba(payload_p, &block_status_p->media_error_lba);
    //fbe_payload_ex_get_pdo_object_id(payload_p, &block_status_p->pdo_object_id);
    /* Free up the packet memory.
     */
    fbe_api_return_contiguous_packet(packet_p); 
    
    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_api_sep_interface_send_block_payload()
 **************************************/

/*!***************************************************************
 * @fn fbe_api_sep_send_block_payload_completion(
 *        fbe_packet_t * packet, 
 *        fbe_packet_completion_context_t context)
 *****************************************************************
 * @brief
 *  completion function for fbe_api_logical_drive_interface_send_block_payload
 *
 * @param packet - completing packet.
 * @param context - completion context with relevant completion information (semaphore to take down)
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  10/17/08: sharel  created
 *
 ****************************************************************/
static fbe_status_t fbe_api_sep_send_block_payload_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_semaphore_t * sem = (fbe_semaphore_t * )context;
    fbe_semaphore_release(sem, 0, 1, FALSE);
    return(FBE_STATUS_OK);
}

/*!***************************************************************
 * @fn fbe_api_sep_io_interface_get_block_size(
 *        const fbe_object_id_t object_id,
 *        fbe_block_transport_negotiate_t *const negotiate_p,
 *        fbe_payload_block_operation_status_t *const block_status_p,
 *        fbe_payload_block_operation_qualifier_t *const block_qualifier)
 *****************************************************************
 * @brief
 *    Sends a syncronous mgmt packet to get block size (i.e. negotiate) 
 *    the block size.
 *
 * @param object_id        - The logical drive object id to send request to
 * @param negotiate_p      - pointer to the negotiate block structure
 * @param block_status_p   - Pointer to the block status structure
 * @param block_qualifier  - Pointer to the block qualifier structure
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  10/14/08: sharel  created
 *
 ****************************************************************/
 fbe_status_t fbe_api_sep_io_interface_get_block_size(const fbe_object_id_t object_id,
                                                      fbe_block_transport_negotiate_t *const negotiate_p,
                                                      fbe_payload_block_operation_status_t *const block_status_p,
                                                      fbe_payload_block_operation_qualifier_t *const block_qualifier)
 {
    fbe_packet_t                           *packet_p = NULL;
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_semaphore_t                         sem;
    fbe_payload_ex_t *                     payload_p = NULL;
    fbe_payload_block_operation_t *         block_operation_p = NULL;
    fbe_sg_element_t                       *sg_p = NULL;
    fbe_sg_element_t                        sg[2]; /* We use this sg to send negotitate info to the object. */

    /* Init our pointer to reference the static sg we allocated.
     */
    sg_p = &sg[0];

    /* Validate parameters. The valid range of object ids is greater than
     * FBE_OBJECT_ID_INVALID and less than FBE_MAX_PHYSICAL_OBJECTS. 
     * (we don't validate the negotiate_p, the object should do that.
     */
    if ((object_id == FBE_OBJECT_ID_INVALID) || (object_id > FBE_MAX_SEP_OBJECTS)){
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*we will set a sempahore to block until the packet comes back,
    set up the packet with the correct opcode and then send it*/
    packet_p = fbe_api_get_contiguous_packet();
    if (packet_p == NULL){
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: Unable to allocate memory for packet\n", __FUNCTION__); 
        return FBE_STATUS_GENERIC_FAILURE;
    }
   
    fbe_semaphore_init(&sem, 0, 1);

    payload_p = fbe_transport_get_payload_ex(packet_p);
    block_operation_p = fbe_payload_ex_allocate_block_operation(payload_p);

    /* Setup the sg using the internal packet sgs.
     * This creates an sg list using the negotiate information passed in. 
     */
    fbe_payload_ex_set_sg_list(payload_p, sg_p, 0);
    fbe_sg_element_init(sg_p, sizeof(fbe_block_transport_negotiate_t), negotiate_p);
    fbe_sg_element_increment(&sg_p);
    fbe_sg_element_terminate(sg_p);

    status =  fbe_payload_block_build_negotiate_block_size(block_operation_p);
    fbe_payload_ex_increment_block_operation_level(payload_p);
    fbe_transport_set_completion_function(packet_p, 
                                          fbe_api_sep_get_block_size_completion, 
                                          &sem);

    fbe_transport_set_address(packet_p,
                              FBE_PACKAGE_ID_SEP_0,
                              FBE_SERVICE_ID_TOPOLOGY,
                              FBE_CLASS_ID_INVALID,
                              object_id); 

    /*the send will be implemented differently in user and kernel space*/
    fbe_api_common_send_io_packet_to_driver(packet_p);
    
    /*we block here and wait for the callback function to take the semaphore down*/
    fbe_semaphore_wait(&sem, NULL);
    fbe_semaphore_destroy(&sem);

    /*check the packet status to make sure we have no errors*/
    status = fbe_transport_get_status_code (packet_p);
    if (status != FBE_STATUS_OK){
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: Packet request failed with status %d \n", __FUNCTION__, status); 
    }

    payload_p = fbe_transport_get_payload_ex(packet_p);
    block_operation_p = fbe_payload_ex_get_block_operation(payload_p);
    if (block_status_p != NULL) {
        fbe_payload_block_get_status(block_operation_p, block_status_p);
    }

    if (block_qualifier != NULL) {
        fbe_payload_block_get_qualifier(block_operation_p, block_qualifier);
    }
    fbe_api_return_contiguous_packet(packet_p);
    
    return status;
}

/*!***************************************************************
 * @fn fbe_api_sep_get_block_size_completion(
 *       fbe_packet_t * packet, 
 *       fbe_packet_completion_context_t context)
 *****************************************************************
 * @brief
 *    completion function for fbe_api_sep_io_interface_get_block_size.
 *
 * @param packet     - pointer to the completing packet
 * @param context    - completion context with relevant completion information (semaphore to take down)
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *   6/29/07: sharel   created
 *
 ****************************************************************/
static fbe_status_t fbe_api_sep_get_block_size_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_semaphore_t * sem = (fbe_semaphore_t * )context;
    fbe_semaphore_release(sem, 0, 1, FALSE);
    return(FBE_STATUS_OK);
} 
/*************************
 * end file fbe_api_sep_interface.c
 *************************/
