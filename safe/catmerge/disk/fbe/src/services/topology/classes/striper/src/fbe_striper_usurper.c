/***************************************************************************
 * Copyright (C) EMC Corporation 2007-2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_striper_usurper.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the striper object code for the usurper.
 * 
 * @ingroup striper_class_files
 * 
 * @version
 *   05/20/2009:  Created. RPF
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_object.h"
#include "fbe/fbe_raid_group.h"
#include "fbe/fbe_parity.h"
#include "fbe_raid_group_object.h"
#include "fbe_striper_private.h"
#include "fbe_transport_memory.h"

fbe_status_t fbe_raid_group_get_zero_checkpoint_raid_extent(fbe_raid_group_t * raid_group_p,
                                                                   fbe_packet_t * packet_p);
fbe_status_t fbe_raid_group_get_zero_checkpoint_raid_extent_completion(fbe_packet_t * packet_p,
                                                                              fbe_packet_completion_context_t context);
static fbe_status_t fbe_striper_map_lba(fbe_striper_t * striper_p, fbe_packet_t * packet_p);
static fbe_status_t fbe_striper_map_lba_calculate(fbe_striper_t * striper_p, fbe_packet_t * packet_p);
static fbe_status_t fbe_striper_map_pba(fbe_striper_t * striper_p, fbe_packet_t * packet_p);
static fbe_status_t fbe_striper_map_pba_calculate(fbe_striper_t * striper_p, fbe_packet_t * packet_p);

static fbe_status_t fbe_striper_get_rebuild_status_raid10_extent(fbe_raid_group_t * raid_group_p,
                                               fbe_packet_t * packet_p);
static fbe_status_t fbe_striper_get_rebuild_status_downstream_mirror_memory_allocation_completion(fbe_memory_request_t * memory_request_p,
                                                                              fbe_memory_completion_context_t context);
static fbe_status_t fbe_striper_get_rebuild_status_downstream_mirror_object_subpacket_completion(fbe_packet_t * packet_p,
                                                                              fbe_packet_completion_context_t context);
static fbe_status_t fbe_striper_get_rebuild_status_raid10_extent_completion(fbe_packet_t * packet_p,
                                                        fbe_packet_completion_context_t context);
static fbe_status_t fbe_striper_get_lun_percent_rebuilt_raid10_extent(fbe_raid_group_t * raid_group_p,
                                                                         fbe_packet_t * packet_p);
static fbe_status_t fbe_striper_get_lun_percent_rebuilt_downstream_mirror_memory_allocation_completion(fbe_memory_request_t * memory_request_p,
                                                                              fbe_memory_completion_context_t context);
static fbe_status_t fbe_striper_get_lun_percent_rebuilt_downstream_mirror_object_subpacket_completion(fbe_packet_t * packet_p,
                                                                              fbe_packet_completion_context_t context);
static fbe_status_t fbe_striper_usurper_raid10_get_extended_media_error_handling(fbe_raid_group_t *raid_group_p, fbe_packet_t *packet_p);
static fbe_status_t fbe_striper_usurper_raid10_set_extended_media_error_handling(fbe_raid_group_t *raid_group_p, fbe_packet_t *packet_p);
static fbe_status_t fbe_striper_get_wear_level_allocation_completion(fbe_memory_request_t * memory_request_p,
                                                                     fbe_memory_completion_context_t context);
static fbe_status_t fbe_striper_get_wear_level_subpacket_completion(fbe_packet_t * packet_p,
                                                                    fbe_packet_completion_context_t context);

/*****************************************
 * LOCAL FUNCTION DEFINITIONS
 *****************************************/

/*!***************************************************************
 * fbe_striper_control_entry()
 ****************************************************************
 * @brief
 *  This function is the management packet entry point for
 *  operations to the striper object.
 *
 * @param object_handle - The config handle.
 * @param packet_p - The packet that is arriving.
 *
 * @return fbe_status_t
 *
 ****************************************************************/
fbe_status_t 
fbe_striper_control_entry(fbe_object_handle_t object_handle, 
                         fbe_packet_t * packet_p)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_base_object_t * base_object_p = NULL;
    fbe_payload_ex_t *payload_p = NULL;
    fbe_payload_control_operation_t *control_p = NULL;
    fbe_payload_control_operation_opcode_t opcode;
    fbe_raid_group_t*               raid_group_p = NULL;
    fbe_raid_geometry_t*            raid_geometry_p = NULL;
    fbe_raid_group_type_t           raid_type;

    /* If object handle is NULL then use the raid group class control
     * entry.
     */
    if (object_handle == NULL)
    {
        return fbe_striper_class_control_entry(packet_p);
    }

    base_object_p = (fbe_base_object_t *)fbe_base_handle_to_pointer(object_handle);
    raid_group_p = (fbe_raid_group_t*)base_object_p;

    payload_p = fbe_transport_get_payload_ex(packet_p);

    control_p = fbe_payload_ex_get_control_operation(payload_p);

    fbe_payload_control_get_opcode(control_p, &opcode);

    switch (opcode)
    {
        case FBE_RAID_GROUP_CONTROL_CODE_GET_ZERO_CHECKPOINT_RAID_EXTENT:
            /* If the raid type is RAID 10 the request has to be split
              into subrequests and sent to the mirror objects
             */
            raid_geometry_p = fbe_raid_group_get_raid_geometry(raid_group_p);                  
            fbe_raid_geometry_get_raid_type(raid_geometry_p, &raid_type);                      
            if(raid_type == FBE_RAID_GROUP_TYPE_RAID10)                                        
            {                                                                                  
                status = fbe_striper_get_zero_checkpoint_raid10_extent(raid_group_p, packet_p);
            }                                                                                  
            else                                                                               
            {                                                                                  
                status = fbe_raid_group_control_entry(object_handle, packet_p);                
            }                                                                                  
            break;                                                                             
        case FBE_RAID_GROUP_CONTROL_CODE_MAP_LBA:
            status = fbe_striper_map_lba((fbe_striper_t*)raid_group_p, packet_p);
            break;
        case FBE_RAID_GROUP_CONTROL_CODE_MAP_PBA:
            status = fbe_striper_map_pba((fbe_striper_t*)raid_group_p, packet_p);
            break;
        case FBE_RAID_GROUP_CONTROL_CODE_GET_REBUILD_STATUS:
            /* If the raid type is RAID 10 the request has to be split
              into subrequests and sent to the mirror objects
             */
            raid_geometry_p = fbe_raid_group_get_raid_geometry(raid_group_p);                 
            fbe_raid_geometry_get_raid_type(raid_geometry_p, &raid_type);                     
            if(raid_type == FBE_RAID_GROUP_TYPE_RAID10)                                       
            {                                                                                 
                status = fbe_striper_get_rebuild_status_raid10_extent(raid_group_p, packet_p);
            }                                                                                 
            else                                                                              
            {                                                                                 
                status = fbe_raid_group_control_entry(object_handle, packet_p);               
            }                                                                                 
            break;
        case FBE_RAID_GROUP_CONTROL_CODE_GET_LUN_PERCENT_REBUILT:
            /* If the raid type is RAID 10 the request has to be split
              into subrequests and sent to the mirror objects
             */
            raid_geometry_p = fbe_raid_group_get_raid_geometry(raid_group_p);                 
            fbe_raid_geometry_get_raid_type(raid_geometry_p, &raid_type);                     
            if(raid_type == FBE_RAID_GROUP_TYPE_RAID10)                                       
            {                                                                                 
                status = fbe_striper_get_lun_percent_rebuilt_raid10_extent(raid_group_p, packet_p);
            }                                                                                 
            else                                                                              
            {                                                                                 
                status = fbe_raid_group_control_entry(object_handle, packet_p);               
            }                                                                                 
            break;  
        case FBE_RAID_GROUP_CONTROL_CODE_GET_EXTENDED_MEDIA_ERROR_HANDLING:
            /* If the raid type is RAID 10 repeat this request for each
             * mirror group.
             */
            raid_geometry_p = fbe_raid_group_get_raid_geometry(raid_group_p);                 
            fbe_raid_geometry_get_raid_type(raid_geometry_p, &raid_type);                     
            if(raid_type == FBE_RAID_GROUP_TYPE_RAID10)                                       
            {                                                                                 
                status = fbe_striper_usurper_raid10_get_extended_media_error_handling(raid_group_p, packet_p);
            }                                                                                 
            else                                                                              
            {                                                                                 
                status = fbe_raid_group_control_entry(object_handle, packet_p);               
            }                                                                                 
            break;
        case FBE_RAID_GROUP_CONTROL_CODE_SET_EXTENDED_MEDIA_ERROR_HANDLING:
            /* If the raid type is RAID 10 repeat this request for each
             * mirror group.
             */
            raid_geometry_p = fbe_raid_group_get_raid_geometry(raid_group_p);                 
            fbe_raid_geometry_get_raid_type(raid_geometry_p, &raid_type);                     
            if(raid_type == FBE_RAID_GROUP_TYPE_RAID10)                                       
            {                                                                                 
                status = fbe_striper_usurper_raid10_set_extended_media_error_handling(raid_group_p, packet_p);
            }                                                                                 
            else                                                                              
            {                                                                                 
                status = fbe_raid_group_control_entry(object_handle, packet_p);               
            }                                                                                 
            break;              
        case FBE_RAID_GROUP_CONTROL_CODE_GET_WEAR_LEVEL_DOWNSTREAM:

            /* If the raid type is RAID 10 repeat this request for each
             * mirror group.
             */
            raid_geometry_p = fbe_raid_group_get_raid_geometry(raid_group_p);                 
            fbe_raid_geometry_get_raid_type(raid_geometry_p, &raid_type);                     
            if(raid_type == FBE_RAID_GROUP_TYPE_RAID10)                                       
            {                                                                                 
                status = fbe_striper_usurper_get_wear_level_downstream(raid_group_p, packet_p);
            }                                                                                 
            else                                                                              
            {                                                                                 
                status = fbe_raid_group_control_entry(object_handle, packet_p);               
            }                                                                                 
            break; 

        /* striper specific handling here.
         */
        default:
            /* Allow the  object to handle all other ioctls.
             */
            status = fbe_raid_group_control_entry(object_handle, packet_p);

            /* If Traverse status is returned and Striper is the most derived
             * class then we have no derived class to handle Traverse status.
             * Complete the packet with Error.
             */
            if((FBE_STATUS_TRAVERSE == status) &&
               (FBE_CLASS_ID_STRIPER == fbe_raid_group_get_class_id(raid_group_p)))
            {
                fbe_base_object_trace((fbe_base_object_t*) raid_group_p,
                                      FBE_TRACE_LEVEL_ERROR,
                                      FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                      "%s Can't Traverse for Most derived Striper class. Opcode: 0x%X\n",
                                      __FUNCTION__, opcode);

                fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
                status = fbe_transport_complete_packet(packet_p);
            }

            break;
    }
    return status;
}
/* end fbe_striper_control_entry() */

/*!****************************************************************************
 * fbe_striper_get_zero_checkpoint_raid10_extent()
 ******************************************************************************
 * @brief
 *  This function is used to get the zero checkpoint for the RAID10 extent,
 *  it allocates new control operation to track the checkpoint for all the 
 *  downstream edges and later it calculates minimum checkpoint from all the
 *  edges to report the zeroing status..
 *
 * @param raid_group_p  - raid group object.
 * @param packet_p      - Pointer to the packet.
 *
 * @return status       - status of the operation.
 *
 * @author
 *  27/06/2011 - Created. Sandeep Chaudhari
 *
 ******************************************************************************/
static fbe_status_t
fbe_striper_get_zero_checkpoint_raid10_extent(fbe_raid_group_t * raid_group_p,
                                               fbe_packet_t * packet_p)
{
    fbe_status_t                                                status;
    fbe_raid_group_get_zero_checkpoint_raid_extent_t *          zero_checkpoint_raid_extent_p = NULL;
    fbe_raid_group_get_zero_checkpoint_downstream_object_t *    zero_checkpoint_downstream_object_p = NULL;
    fbe_payload_ex_t *                                          sep_payload_p = NULL;
    fbe_payload_control_operation_t *                           control_operation_p = NULL;

    /* Get the sep payload and control operation/ */
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(sep_payload_p);

    /* Get the control buffer pointer from the packet payload. */
    status = fbe_raid_group_usurper_get_control_buffer(raid_group_p,
                                                       packet_p, 
                                                       sizeof(fbe_raid_group_get_zero_checkpoint_raid_extent_t),
                                                       (fbe_payload_control_buffer_t)&zero_checkpoint_raid_extent_p);
    if (status != FBE_STATUS_OK) 
    { 
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return status; 
    }

    /* Initialize the raid extent zero checkpoint as zero. */
    zero_checkpoint_raid_extent_p->zero_checkpoint = 0;

    /* Allocate the buffer to send the raid group zero percent raid extent usurpur. */
    zero_checkpoint_downstream_object_p = 
        (fbe_raid_group_get_zero_checkpoint_downstream_object_t *) fbe_transport_allocate_buffer();
    if (zero_checkpoint_downstream_object_p == NULL)
    {
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return status;
    }

    /* Allocate the new control operation to build. */
    control_operation_p = fbe_payload_ex_allocate_control_operation(sep_payload_p);
    fbe_payload_control_build_operation(control_operation_p,
                                        FBE_RAID_GROUP_CONTROL_CODE_GET_ZERO_CHECKPOINT_FOR_DOWNSTREAM_MIRROR_OBJECT,
                                        zero_checkpoint_downstream_object_p,
                                        sizeof(fbe_raid_group_get_zero_checkpoint_downstream_object_t));

    fbe_payload_ex_increment_control_operation_level(sep_payload_p);

    fbe_transport_set_completion_function(packet_p,
                                            fbe_raid_group_get_zero_checkpoint_raid_extent_completion,
                                            raid_group_p);

    /* Call the function to get the zero percentage for raid extent. */
    fbe_striper_get_zero_checkpoint_for_downstream_mirror_objects(raid_group_p, packet_p);
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/******************************************************************************
 * end fbe_striper_get_zero_checkpoint_raid10_extent()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_striper_get_zero_checkpoint_for_downstream_mirror_objects()
 ******************************************************************************
 * @brief
 *  This function is used to issue the request to all the downstream edges to
 *  gather the zero status.
 *
 * @param raid_group_p  - raid group object.
 * @param packet_p      - Pointer to the packet.
 *
 * @return status       - status of the operation.
 *
 * @author
 *  06/27/2011 - Created. Sandeep Chaudhari
 *
 ******************************************************************************/
static fbe_status_t
fbe_striper_get_zero_checkpoint_for_downstream_mirror_objects(fbe_raid_group_t * raid_group_p,
                                                          fbe_packet_t * packet_p)
{
    fbe_status_t                                                status = FBE_STATUS_OK;
    fbe_raid_group_get_zero_checkpoint_downstream_object_t *    zero_checkpoint_downstream_object_p = NULL;
    fbe_u32_t                                                   width = 0;
    fbe_payload_ex_t *                                         sep_payload_p = NULL;
    fbe_payload_control_operation_t *                           control_operation_p = NULL;
    fbe_u32_t                                                   index = 0;
    fbe_base_config_memory_allocation_chunks_t                  mem_alloc_chunks = {0};

    /* Get the sep payload and control operation/ */
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(sep_payload_p);

    /* Get the control buffer pointer from the packet payload. */
    status = fbe_raid_group_usurper_get_control_buffer(raid_group_p,
                                                       packet_p, 
                                                       sizeof(fbe_raid_group_get_zero_checkpoint_downstream_object_t),
                                                       (fbe_payload_control_buffer_t)&zero_checkpoint_downstream_object_p);
    if (status != FBE_STATUS_OK) 
    { 
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return status; 
    }

    /* Initialize the checkpoint to zero before sending the request down. */
    for(index = 0; index < FBE_RAID_MAX_DISK_ARRAY_WIDTH; index++)
    {
        zero_checkpoint_downstream_object_p->zero_checkpoint[index] = 0;
    }

    /* Get the width of the raid group object. */
    fbe_base_config_get_width((fbe_base_config_t *) raid_group_p, &width);

    /* Set the various counts and buffer size */
    mem_alloc_chunks.buffer_count = width;
    mem_alloc_chunks.number_of_packets = width;
    mem_alloc_chunks.sg_element_count = 0;

    mem_alloc_chunks.buffer_size = sizeof(fbe_raid_group_get_zero_checkpoint_raid_extent_t) + sizeof(fbe_edge_index_t);
    mem_alloc_chunks.alloc_pre_read_desc = FALSE;
    
    /* Allocate the subpackets to collect the zero checkpoint for all the edges. */
    status = fbe_base_config_memory_allocate_chunks((fbe_base_config_t *) raid_group_p,
                                                    packet_p,
                                                    mem_alloc_chunks,
                                                    (fbe_memory_completion_function_t)fbe_striper_get_zero_checkpoint_downstream_mirror_object_allocation_completion); /* SAFEBUG - cast to supress warning but must fix because memory completion function shouldn't return status */ 
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s memory allocation failed, status:0x%x\n", __FUNCTION__, status);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_complete_packet(packet_p);
        return status; 
    }
    return status;
}
/******************************************************************************
 * end fbe_striper_get_zero_checkpoint_for_downstream_mirror_objects()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_striper_get_zero_checkpoint_downstream_mirror_object_allocation_completion()
 ******************************************************************************
 * @brief
 *  This function is used to get the zero checkpoint for all the downstream
 *  mirror objects.
 *
 * @param memory_request_p  - memory request.
 * @param context           - context passed to acquire the memory.
 *
 * @return status           - status of the operation.
 *
 * @author
 *  06/27/2011 - Created. Sandeep Chaudhari
 *
 ******************************************************************************/
static fbe_status_t
fbe_striper_get_zero_checkpoint_downstream_mirror_object_allocation_completion(fbe_memory_request_t * memory_request_p,
                                                                           fbe_memory_completion_context_t context)
{
    fbe_packet_t *                                    packet_p = NULL;
    fbe_packet_t **                                   new_packet_p = NULL;
    fbe_u8_t **                                       buffer_p = NULL;
    fbe_payload_ex_t *                                sep_payload_p = NULL;
    fbe_payload_ex_t *                                new_sep_payload_p = NULL;
    fbe_payload_control_operation_t *                 control_operation_p = NULL;
    fbe_payload_control_operation_t *                 prev_control_operation_p = NULL;
    fbe_payload_control_operation_t *                 new_control_operation_p = NULL;
    fbe_raid_group_get_zero_checkpoint_raid_extent_t* zero_checkpoint_mirror_p = NULL;
    fbe_status_t                                      status = FBE_STATUS_OK;
    fbe_raid_group_t *                                raid_group_p = NULL;
    fbe_u32_t                                         width = 0;
    fbe_edge_index_t *                                edge_index_p = NULL;
    fbe_u32_t                                         index = 0;
    fbe_block_edge_t *                                block_edge_p = NULL;
    fbe_base_config_memory_alloc_chunks_address_t     mem_alloc_chunks_addr = {0};
    fbe_sg_element_t                                 *pre_read_sg_list_p = NULL;
    fbe_payload_pre_read_descriptor_t                *pre_read_desc_p = NULL;
    fbe_packet_attr_t                                 packet_attr;
    fbe_raid_group_get_zero_checkpoint_raid_extent_t *zero_checkpoint_striper_extent_p = NULL;
    fbe_lba_t                                         edge_offset = 0;

    /* Get the pointer to original packet. */
    packet_p = fbe_transport_memory_request_to_packet(memory_request_p);

    /* Get the pointer to raid group object. */
    raid_group_p = (fbe_raid_group_t *) context;

    /* Get the payload and control operation. */
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p =  fbe_payload_ex_get_control_operation(sep_payload_p);

    /* Check allocation status */
    if (fbe_memory_request_is_allocation_complete(memory_request_p) == FBE_FALSE)
    {
        /* Currently this callback doesn't expect any aborted requests */
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s: memory request: 0x%p failed. request state: %d \n",
                              __FUNCTION__, memory_request_p, memory_request_p->request_state);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_FAILED;
    }

    /* Get the width of the base config object. */
    fbe_base_config_get_width((fbe_base_config_t *) raid_group_p, &width);

    /* Subpacket control buffer size. */
    mem_alloc_chunks_addr.buffer_size = sizeof(fbe_raid_group_get_zero_checkpoint_raid_extent_t) + sizeof(fbe_edge_index_t);
    mem_alloc_chunks_addr.number_of_packets = width;
    mem_alloc_chunks_addr.buffer_count = width;
    mem_alloc_chunks_addr.sg_element_count = 0;
    mem_alloc_chunks_addr.sg_list_p = NULL;
    mem_alloc_chunks_addr.assign_pre_read_desc = FALSE;
    mem_alloc_chunks_addr.pre_read_sg_list_p = &pre_read_sg_list_p;
    mem_alloc_chunks_addr.pre_read_desc_p = &pre_read_desc_p;
    
    /* Get the zod sgl pointer and and subpacket pointer from the allocated memory. */
    status = fbe_base_config_memory_assign_address_from_allocated_chunks((fbe_base_config_t *) raid_group_p,
                                                                         memory_request_p,
                                                                         &mem_alloc_chunks_addr);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s memory distribution failed, status:0x%x, width:0x%x, buffer_size:0x%x\n",
                              __FUNCTION__, status, width, mem_alloc_chunks_addr.buffer_size);
        //fbe_memory_request_get_entry_mark_free(memory_request_p, &memory_ptr);
        //fbe_memory_free_entry(memory_ptr);
		fbe_memory_free_request_entry(memory_request_p);

        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_INSUFFICIENT_RESOURCES);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return status;
    }

    new_packet_p = mem_alloc_chunks_addr.packet_array_p;
    buffer_p = mem_alloc_chunks_addr.buffer_array_p;

    /* Get the previous control operation which has data sent from lun. */
    prev_control_operation_p =  fbe_payload_ex_get_prev_control_operation(sep_payload_p);
        
    /* Get the control buffer pointer from the packet payload which was sent by lun. */
    fbe_payload_control_get_buffer(prev_control_operation_p, &zero_checkpoint_striper_extent_p);
     
    while(index < width)
    {
        /* Initialize the sub packets. */
        fbe_transport_initialize_sep_packet(new_packet_p[index]);
        new_sep_payload_p = fbe_transport_get_payload_ex(new_packet_p[index]);

        fbe_base_config_get_block_edge((fbe_base_config_t *) raid_group_p, &block_edge_p, index);
        edge_offset = fbe_block_transport_edge_get_offset(block_edge_p);        

        /* Initialize the control buffer. */
        zero_checkpoint_mirror_p = (fbe_raid_group_get_zero_checkpoint_raid_extent_t *) buffer_p[index];
        zero_checkpoint_mirror_p->zero_checkpoint = 0;

        /* Add the striper RG offset to lun offset to normalize lun at striper level */
        zero_checkpoint_mirror_p->lun_offset = zero_checkpoint_striper_extent_p->lun_offset + edge_offset;

        /* Based on striper width, lun capacity is divided in mirror RG objects */         
        zero_checkpoint_mirror_p->lun_capacity = zero_checkpoint_striper_extent_p->lun_capacity/width;
        
        /* Initialize the edge index as a context data. */
        edge_index_p = (fbe_edge_index_t *) (buffer_p[index] + sizeof(fbe_raid_group_get_zero_checkpoint_raid_extent_t));
        *edge_index_p = index;

        /* Allocate and initialize the control operation. */
        new_control_operation_p = fbe_payload_ex_allocate_control_operation(new_sep_payload_p);
        fbe_payload_control_build_operation(new_control_operation_p,
                                            FBE_RAID_GROUP_CONTROL_CODE_GET_ZERO_CHECKPOINT_RAID_EXTENT,
                                            zero_checkpoint_mirror_p,
                                            sizeof(fbe_raid_group_get_zero_checkpoint_raid_extent_t));

        /* Initialize the new packet with FBE_PACKET_FLAG_EXTERNAL same as the master packet */
        fbe_transport_get_packet_attr(packet_p, &packet_attr);
        if (packet_attr & FBE_PACKET_FLAG_EXTERNAL) 
        {
            fbe_transport_set_packet_attr(new_packet_p[index], FBE_PACKET_FLAG_EXTERNAL);            
        }

        /* Set completion functon. */
        fbe_transport_set_completion_function(new_packet_p[index],
                                              fbe_striper_get_zero_checkpoint_downstream_mirror_object_subpacket_completion,
                                              raid_group_p);

        /* Add the subpacket to master packet queue. */
        fbe_transport_add_subpacket(packet_p, new_packet_p[index]);
        index++;
    }

    /* Send all the subpackets together. */
    for(index = 0; index < width; index++)
    {
        fbe_base_config_get_block_edge((fbe_base_config_t *)raid_group_p, &block_edge_p, index);
        fbe_transport_set_packet_attr(new_packet_p[index], FBE_PACKET_FLAG_TRAVERSE);
        fbe_block_transport_send_control_packet(block_edge_p, new_packet_p[index]);
    }

    /* Send all the subpackets. */
     return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/******************************************************************************
 * end fbe_striper_get_zero_checkpoint_downstream_mirror_object_allocation_completion()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_striper_get_zero_checkpoint_downstream_mirror_object_subpacket_completion()
 ******************************************************************************
 * @brief
 *  This function is used to handle the subpacket completion to get the zero
 *  checkpoint for all the downstream objects.
 *
 * @param memory_request_p  - memory request.
 * @param context           - context passed to acquire the memory.
 *
 * @return status           - status of the operation.
 *
 * @author
 *  06/27/2011 - Created. Sandeep Chaudhari
 *
 ******************************************************************************/
static fbe_status_t 
fbe_striper_get_zero_checkpoint_downstream_mirror_object_subpacket_completion(fbe_packet_t * packet_p,
                                                                          fbe_packet_completion_context_t context)
{
    fbe_raid_group_t *                                          raid_group_p = NULL;
    fbe_packet_t *                                              master_packet_p = NULL;
    fbe_payload_ex_t *                                         master_sep_payload_p = NULL;
    fbe_payload_control_operation_t *                           master_control_operation_p = NULL;
    fbe_payload_ex_t *                                         sep_payload_p = NULL;
    fbe_payload_control_operation_t *                           control_operation_p = NULL;
    fbe_payload_control_status_t                                control_status;
    fbe_payload_control_status_qualifier_t                      control_qualifier;
    fbe_status_t                                                status;
    fbe_raid_group_get_zero_checkpoint_downstream_object_t *    zero_checkpoint_downstream_object_p = NULL;
    fbe_raid_group_get_zero_checkpoint_raid_extent_t*           raid_zero_checkpoint_p = NULL;
    fbe_edge_index_t *                                          edge_index_p = NULL;
    fbe_bool_t is_empty;

    /* Cast the context to base config object pointer. */
    raid_group_p = (fbe_raid_group_t *) context;

    /* Get the master packet, its payload and associated control operation. */
    master_packet_p = fbe_transport_get_master_packet(packet_p);
    master_sep_payload_p = fbe_transport_get_payload_ex(master_packet_p);
    master_control_operation_p = fbe_payload_ex_get_control_operation(master_sep_payload_p);

    /* Get the control buffer of the master packet. */
    fbe_payload_control_get_buffer(master_control_operation_p, &zero_checkpoint_downstream_object_p);

    /* Get the control operation of the subpacket. */
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(sep_payload_p);

    /* Get the status of the subpacket operation. */
    status = fbe_transport_get_status_code(packet_p);
    fbe_payload_control_get_status(control_operation_p, &control_status);
    fbe_payload_control_get_status_qualifier(control_operation_p, &control_qualifier);

    /* If any of the subpacket failed with bad status then update the master status. */
    if(status != FBE_STATUS_OK)
    {
        fbe_transport_set_status(master_packet_p, status, 0);
    }

    /* If any of the subpacket failed with failure status then update the master status. */
    /*!@todo we can use error predence here if required. */
    if(control_status != FBE_PAYLOAD_CONTROL_STATUS_OK)
    {
        fbe_payload_control_set_status(master_control_operation_p, control_status);
        fbe_payload_control_set_status_qualifier(master_control_operation_p, control_qualifier);
    }

    if((status == FBE_STATUS_OK) &&
       (control_status == FBE_PAYLOAD_CONTROL_STATUS_OK))
    {
        /* Get the control buffer of the master packet. */
        fbe_payload_control_get_buffer(control_operation_p, &raid_zero_checkpoint_p);

        /* Get the edge index of the subpacket. */
        edge_index_p = (fbe_edge_index_t *) (((fbe_u8_t *) raid_zero_checkpoint_p) + sizeof(fbe_raid_group_get_zero_checkpoint_raid_extent_t));

        /* Update the master packet with zero checkpoint. */
        zero_checkpoint_downstream_object_p->zero_checkpoint[*edge_index_p] = raid_zero_checkpoint_p->zero_checkpoint;
    }

    /* Remove the sub packet from master queue. */
    //fbe_transport_remove_subpacket(packet_p);
    fbe_transport_remove_subpacket_is_queue_empty(packet_p, &is_empty);

    /* Destroy the subpacket. */
    //fbe_transport_destroy_packet(packet_p);

    /* When the queue is empty, all subpackets have completed. */

    if (is_empty)
    {
        fbe_transport_destroy_subpackets(master_packet_p);
        //fbe_memory_request_get_entry_mark_free(&master_packet_p->memory_request, &memory_ptr);
        //fbe_memory_free_entry(memory_ptr);
		fbe_memory_free_request_entry(&master_packet_p->memory_request);

        fbe_transport_set_status(master_packet_p, FBE_STATUS_OK, 0);
        fbe_payload_control_set_status (master_control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_OK);
        fbe_transport_complete_packet(master_packet_p);
    }
    else
    {
        /* Not done with processing sub packets.
         */
    }

    return FBE_STATUS_MORE_PROCESSING_REQUIRED; 
}
/******************************************************************************
 * end fbe_striper_get_zero_checkpoint_downstream_mirror_object_subpacket_completion()
 ******************************************************************************/

fbe_status_t fbe_striper_map_lba_completion(fbe_packet_t * packet_p,
                                            fbe_packet_completion_context_t context)
{
    fbe_striper_t *striper_p = (fbe_striper_t*)context;

    fbe_striper_map_lba_calculate(striper_p, packet_p);
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}

/*!**************************************************************
 * fbe_striper_map_lba()
 ****************************************************************
 * @brief
 *  This function maps a lba to pba.
 *
 * @param striper_p - The raid group.
 * @param packet_p - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  5/31/2012 - Created. Rob Foley
 *
 ****************************************************************/
static fbe_status_t
fbe_striper_map_lba(fbe_striper_t * striper_p, fbe_packet_t * packet_p)
{
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_group_get_raid_geometry((fbe_raid_group_t*)striper_p);
    fbe_block_edge_t    *block_edge_p = NULL;
    fbe_status_t             status;
    fbe_payload_ex_t *payload_p = NULL;
    fbe_payload_control_operation_t *control_operation_p = NULL;
    fbe_raid_group_map_info_t *map_info_p = NULL;
    fbe_raid_siots_geometry_t geo;
    fbe_lba_t pba;
    fbe_chunk_size_t chunk_size;

    chunk_size = fbe_raid_group_get_chunk_size((fbe_raid_group_t*)striper_p);

    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);

    status = fbe_raid_group_usurper_get_control_buffer((fbe_raid_group_t*)striper_p, packet_p, 
                                                       sizeof(fbe_raid_group_map_info_t),
                                                       (fbe_payload_control_buffer_t *)&map_info_p);
    if (status != FBE_STATUS_OK)
    {
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return status;
    }
    map_info_p->original_lba = map_info_p->lba;

    status = fbe_striper_get_geometry(raid_geometry_p, map_info_p->lba, &geo);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)striper_p,
                              FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s error from get parity lun geometry status: 0x%x\n", __FUNCTION__, status);
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return status;
    }

    if (fbe_raid_geometry_is_raid10(raid_geometry_p))
    {
        /* Calculate the lba and pass it downstream.
         */
        pba = geo.logical_parity_start + geo.start_offset_rel_parity_stripe;
        map_info_p->lba = pba;
        /* We send the R10 downstream so it can pick up the offset, and lba-> pba from the mirror.
         */
        fbe_raid_geometry_get_block_edge(raid_geometry_p, &block_edge_p, geo.position[geo.start_index]);
        fbe_transport_set_completion_function(packet_p, fbe_striper_map_lba_completion, striper_p);
        fbe_block_transport_send_control_packet(block_edge_p, packet_p);
    }
    else
    {
        fbe_raid_geometry_get_block_edge(raid_geometry_p, &block_edge_p, 0);
        map_info_p->offset = fbe_block_transport_edge_get_offset(block_edge_p);
        map_info_p->data_pos = geo.position[geo.start_index];
        map_info_p->parity_pos = 0;

        /* We always use logical parity start + start offset relative to the parity stripe to get 
         * our offset on each drive. 
         * Then we add on the physical offset to get the pba. 
         */
        pba = geo.logical_parity_start + geo.start_offset_rel_parity_stripe;
        map_info_p->pba = pba + map_info_p->offset;

        /* Take the raid group pba and get the chunk index.
         */
        map_info_p->chunk_index = (fbe_chunk_count_t)(pba / chunk_size);
        fbe_raid_group_get_metadata_lba((fbe_raid_group_t*)striper_p,
                                        map_info_p->chunk_index,
                                        &map_info_p->metadata_lba);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
    }
    return FBE_STATUS_PENDING;
}
/**************************************
 * end fbe_striper_map_lba()
 **************************************/

/*!**************************************************************
 * fbe_striper_map_lba_calculate()
 ****************************************************************
 * @brief
 *  Calculate the pba from an lba.
 *
 * @param striper_p - The raid group.
 * @param packet_p - The packet requesting this operation.
 *
 * @return fbe_status_t   
 *
 * @author
 *  5/31/2012 - Created. Rob Foley
 *
 ****************************************************************/

static fbe_status_t
fbe_striper_map_lba_calculate(fbe_striper_t * striper_p, fbe_packet_t * packet_p)
{
    fbe_status_t             status;
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_group_get_raid_geometry((fbe_raid_group_t*)striper_p);
    fbe_payload_ex_t *payload_p = NULL;
    fbe_payload_control_operation_t *control_operation_p = NULL;
    fbe_raid_group_map_info_t *map_info_p = NULL;
    fbe_raid_siots_geometry_t geo;

    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);

    status = fbe_raid_group_usurper_get_control_buffer((fbe_raid_group_t*)striper_p, packet_p, 
                                                       sizeof(fbe_raid_group_map_info_t),
                                                       (fbe_payload_control_buffer_t *)&map_info_p);
    if(status != FBE_STATUS_OK)
    {
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return status;
    }

    /* We changed the lba when we sent it down originally, change it back.
     */
    map_info_p->lba = map_info_p->original_lba;

    status = fbe_striper_get_geometry(raid_geometry_p, map_info_p->lba, &geo);
    if(status != FBE_STATUS_OK)
    {
         fbe_base_object_trace((fbe_base_object_t *)striper_p,
                               FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                               "%s error from get parity lun geometry status: 0x%x\n", __FUNCTION__, status);
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return status;
    }
    /* Raid 10 only sees the mirrors below it.
     * To get the actual positions, we need to multiply by the mirror width (2). 
     */
    map_info_p->data_pos = geo.position[geo.start_index] * 2;
    map_info_p->parity_pos = map_info_p->data_pos + 1;

    /* Raid 10 already has the rest of the fields, since the mirror calculated this.
     * Raid 10 also already has the mirror's chunk index and metadata lba. 
     */

    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);
    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_striper_map_lba_calculate()
 **************************************/

/*!**************************************************************
 * fbe_striper_map_pba_completion()
 ****************************************************************
 * @brief
 *  Call the function to calculate the lba
 *
 * @param striper_p - The raid group.
 * @param packet_p - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  11/1/2012 - Created. Deanna Heng
 *
 ****************************************************************/
fbe_status_t fbe_striper_map_pba_completion(fbe_packet_t * packet_p,
                                            fbe_packet_completion_context_t context)
{
    fbe_striper_t *striper_p = (fbe_striper_t*)context;

    fbe_striper_map_pba_calculate(striper_p, packet_p);
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/**************************************
 * end fbe_striper_map_pba_completion()
 **************************************/

/*!**************************************************************
 * fbe_striper_map_pba()
 ****************************************************************
 * @brief
 *  This function maps a pba to lba.
 *
 * @param striper_p - The raid group.
 * @param packet_p - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  11/1/2012 - Created. Deanna Heng
 *
 ****************************************************************/
static fbe_status_t
fbe_striper_map_pba(fbe_striper_t * striper_p, fbe_packet_t * packet_p)
{
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_group_get_raid_geometry((fbe_raid_group_t*)striper_p);
    fbe_block_edge_t    *block_edge_p = NULL;
    fbe_status_t             status;
    fbe_payload_ex_t *payload_p = NULL;
    fbe_payload_control_operation_t *control_operation_p = NULL;
    fbe_raid_group_map_info_t *map_info_p = NULL;
    fbe_lba_t pba;
    fbe_lba_t lba;
    fbe_chunk_size_t chunk_size;
    fbe_element_size_t  sectors_per_element;
    fbe_block_count_t blocks_per_stripe;
    fbe_u16_t             width;

    chunk_size = fbe_raid_group_get_chunk_size((fbe_raid_group_t*)striper_p);

    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);

    status = fbe_raid_group_usurper_get_control_buffer((fbe_raid_group_t*)striper_p, packet_p, 
                                                       sizeof(fbe_raid_group_map_info_t),
                                                       (fbe_payload_control_buffer_t *)&map_info_p);
    if (status != FBE_STATUS_OK)
    {
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return status;
    }


    if (fbe_raid_geometry_is_raid10(raid_geometry_p))
    {
        /* Let the mirror do its thing and the striper completion function 
         * will do the calculations afterward
         */
        map_info_p->data_pos = map_info_p->position / 2;
        fbe_raid_geometry_get_block_edge(raid_geometry_p, &block_edge_p, map_info_p->data_pos);
        fbe_transport_set_completion_function(packet_p, fbe_striper_map_pba_completion, striper_p);
        fbe_block_transport_send_control_packet(block_edge_p, packet_p);
    }
    else
    {
        fbe_raid_geometry_get_block_edge(raid_geometry_p, &block_edge_p, map_info_p->position);
        map_info_p->offset = fbe_block_transport_edge_get_offset(block_edge_p);
        map_info_p->data_pos = map_info_p->position;
        map_info_p->parity_pos = 0;

        /* Get the logical pba for the disk
         */
        pba = map_info_p->pba - map_info_p->offset;

        /* Calculate the lba from the pba
         */
        fbe_raid_geometry_get_element_size(raid_geometry_p, &sectors_per_element);
        fbe_raid_geometry_get_data_disks(raid_geometry_p, &width);
        blocks_per_stripe = sectors_per_element * width;
        lba = (pba / sectors_per_element) * blocks_per_stripe
               + (pba % sectors_per_element);
        lba += map_info_p->position * sectors_per_element;
        map_info_p->lba = lba;

        /* Take the raid group pba and get the chunk index.
         */
        map_info_p->chunk_index = (fbe_chunk_count_t)(pba / chunk_size);
        fbe_raid_group_get_metadata_lba((fbe_raid_group_t*)striper_p,
                                        map_info_p->chunk_index,
                                        &map_info_p->metadata_lba);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
    }
    return FBE_STATUS_PENDING;
}
/**************************************
 * end fbe_striper_map_pba()
 **************************************/

/*!**************************************************************
 * fbe_striper_map_pba_calculate()
 ****************************************************************
 * @brief
 *  Calculate the pba from an lba.
 *
 * @param striper_p - The raid group.
 * @param packet_p - The packet requesting this operation.
 *
 * @return fbe_status_t   
 *
 * @author
 *  11/2/2012 - Created. Deanna Heng
 *
 ****************************************************************/

static fbe_status_t
fbe_striper_map_pba_calculate(fbe_striper_t * striper_p, fbe_packet_t * packet_p)
{
    fbe_status_t             status;
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_group_get_raid_geometry((fbe_raid_group_t*)striper_p);
    fbe_payload_ex_t *payload_p = NULL;
    fbe_payload_control_operation_t *control_operation_p = NULL;
    fbe_raid_group_map_info_t *map_info_p = NULL;
    fbe_block_count_t blocks_per_stripe;
    fbe_u16_t             width;
    fbe_lba_t             pba;
    fbe_lba_t             lba;
    fbe_chunk_size_t chunk_size;
    fbe_element_size_t  sectors_per_element;

    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);

    status = fbe_raid_group_usurper_get_control_buffer((fbe_raid_group_t*)striper_p, packet_p, 
                                                       sizeof(fbe_raid_group_map_info_t),
                                                       (fbe_payload_control_buffer_t *)&map_info_p);
    if(status != FBE_STATUS_OK)
    {
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return status;
    }

    /* Raid 10 only sees the mirrors below it.
     * To get the actual positions, we need to multiply by the mirror width (2). 
     */
    map_info_p->data_pos = (map_info_p->position / 2) * 2;
    map_info_p->parity_pos = map_info_p->data_pos + 1;
    // get the logical pba on the disk
    pba = map_info_p->pba - map_info_p->offset;

    // Calculate the lba from the pba
    fbe_raid_geometry_get_data_disks(raid_geometry_p, &width);
    fbe_raid_geometry_get_element_size(raid_geometry_p, &sectors_per_element);
    blocks_per_stripe = sectors_per_element * width;
    lba = (pba / sectors_per_element) * blocks_per_stripe
           + (pba % sectors_per_element);
    lba += (map_info_p->position / 2) * sectors_per_element;
    map_info_p->lba = lba;

    /* Take the raid group pba and get the chunk index.
     */
    chunk_size = fbe_raid_group_get_chunk_size((fbe_raid_group_t*)striper_p);
    map_info_p->chunk_index = (fbe_chunk_count_t)(pba / chunk_size);
    fbe_raid_group_get_metadata_lba((fbe_raid_group_t*)striper_p,
                                    map_info_p->chunk_index,
                                    &map_info_p->metadata_lba);

    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);
    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_striper_map_pba_calculate()
 **************************************/

/*!****************************************************************************
 * fbe_striper_get_rebuild_status_raid10_extent()
 ******************************************************************************
 * @brief
 *  This function is used to get the rebuild status for the RAID10 extent. It 
 *  allocates memory for the downstream raid group objects to get rg rebuild status.
 *
 * @param raid_group_p  - raid group object.
 * @param packet_p      - Pointer to the packet.
 *
 * @return status       - status of the operation.
 *
 * @author
 *  06/18/2012 - Created. Vera Wang
 *
 ******************************************************************************/
static fbe_status_t
fbe_striper_get_rebuild_status_raid10_extent(fbe_raid_group_t * raid_group_p,
                                             fbe_packet_t * packet_p)
{
    fbe_status_t                                        status;
    fbe_raid_group_rebuild_status_t *                   rebuild_status_p = NULL;
    fbe_payload_ex_t *                                  sep_payload_p = NULL;
    fbe_payload_control_operation_t *                   control_operation_p = NULL;
    fbe_u32_t                                           width = 0;
    fbe_base_config_memory_allocation_chunks_t          mem_alloc_chunks = {0};

    /* get the rebuild percentage buffer from the packet. */
    status = fbe_raid_group_usurper_get_control_buffer(raid_group_p, packet_p, 
                                                       sizeof(fbe_raid_group_rebuild_status_t),
                                                       (fbe_payload_control_buffer_t *)&rebuild_status_p);
    if (status != FBE_STATUS_OK)
    {
        fbe_transport_set_status(packet_p, status, 0);
        status = fbe_transport_complete_packet(packet_p);
        return status;
    }

    /* get the sep payload and control operation/ */
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(sep_payload_p);
   
    /* Get the width of the raid group object. */
    fbe_base_config_get_width((fbe_base_config_t *) raid_group_p, &width);

    /* Set the various counts and buffer size */
    mem_alloc_chunks.buffer_count = width;
    mem_alloc_chunks.number_of_packets = width;
    mem_alloc_chunks.sg_element_count = 0;

    mem_alloc_chunks.buffer_size = sizeof(fbe_raid_group_rebuild_status_t) + sizeof(fbe_edge_index_t);
    mem_alloc_chunks.alloc_pre_read_desc = FALSE;
    
    /* Allocate the subpackets to collect the rebuild status for all the edges. */
    status = fbe_base_config_memory_allocate_chunks((fbe_base_config_t *) raid_group_p,
                                                    packet_p,
                                                    mem_alloc_chunks,
                                                    (fbe_memory_completion_function_t)fbe_striper_get_rebuild_status_downstream_mirror_memory_allocation_completion); /* SAFEBUG - cast to supress warning but must fix because memory completion function shouldn't return status */
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s memory allocation failed, status:0x%x\n", __FUNCTION__, status);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_complete_packet(packet_p);
        return status; 
    }

    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_striper_get_rebuild_status_raid10_extent()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_striper_get_rebuild_status_downstream_mirror_memory_allocation_completion()
 ******************************************************************************
 * @brief
 *  This function is used to get the rebuild status for all the downstream
 *  mirror objects.
 *
 * @param memory_request_p  - memory request.
 * @param context           - context passed to acquire the memory.
 *
 * @return status           - status of the operation.
 *
 * @author
 *  06/27/2011 - Created. Vera Wang
 *
 ******************************************************************************/
static fbe_status_t
fbe_striper_get_rebuild_status_downstream_mirror_memory_allocation_completion(fbe_memory_request_t * memory_request_p,
                                                                              fbe_memory_completion_context_t context)
{
    fbe_packet_t *                                    packet_p = NULL;
    fbe_packet_t **                                   new_packet_p = NULL;
    fbe_u8_t **                                       buffer_p = NULL;
    fbe_payload_ex_t *                                sep_payload_p = NULL;
    fbe_payload_ex_t *                                new_sep_payload_p = NULL;
    fbe_payload_control_operation_t *                 control_operation_p = NULL;
    fbe_payload_control_operation_t *                 new_control_operation_p = NULL;
    fbe_status_t                                      status = FBE_STATUS_OK;
    fbe_raid_group_rebuild_status_t*                  rebuild_status_p = NULL;
    fbe_raid_group_t *                                raid_group_p = NULL;
    fbe_u32_t                                         width = 0;
    fbe_edge_index_t *                                edge_index_p = NULL;
    fbe_u32_t                                         index = 0;
    fbe_block_edge_t *                                block_edge_p = NULL;
    fbe_base_config_memory_alloc_chunks_address_t     mem_alloc_chunks_addr = {0};
    fbe_sg_element_t                                 *pre_read_sg_list_p = NULL;
    fbe_payload_pre_read_descriptor_t                *pre_read_desc_p = NULL;
    fbe_packet_attr_t                                 packet_attr;

    /* Get the pointer to original packet. */
    packet_p = fbe_transport_memory_request_to_packet(memory_request_p);

    /* Get the pointer to raid group object. */
    raid_group_p = (fbe_raid_group_t *) context;

    /* Get the payload and control operation. */
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p =  fbe_payload_ex_get_control_operation(sep_payload_p);

    /* Check allocation status */
    if (fbe_memory_request_is_allocation_complete(memory_request_p) == FBE_FALSE)
    {
        /* Currently this callback doesn't expect any aborted requests */
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s: memory request: 0x%p failed. request state: %d \n",
                              __FUNCTION__, memory_request_p, memory_request_p->request_state);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_FAILED;
    }

    /* Get the width of the base config object. */
    fbe_base_config_get_width((fbe_base_config_t *) raid_group_p, &width);

    /* Subpacket control buffer size. */
    mem_alloc_chunks_addr.buffer_size = sizeof(fbe_raid_group_rebuild_status_t) + sizeof(fbe_edge_index_t);
    mem_alloc_chunks_addr.number_of_packets = width;
    mem_alloc_chunks_addr.buffer_count = width;
    mem_alloc_chunks_addr.sg_element_count = 0;
    mem_alloc_chunks_addr.sg_list_p = NULL;
    mem_alloc_chunks_addr.assign_pre_read_desc = FALSE;
    mem_alloc_chunks_addr.pre_read_sg_list_p = &pre_read_sg_list_p;
    mem_alloc_chunks_addr.pre_read_desc_p = &pre_read_desc_p;
    
    status = fbe_base_config_memory_assign_address_from_allocated_chunks((fbe_base_config_t *) raid_group_p,
                                                                         memory_request_p,
                                                                         &mem_alloc_chunks_addr);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s memory distribution failed, status:0x%x, width:0x%x, buffer_size:0x%x\n",
                              __FUNCTION__, status, width, mem_alloc_chunks_addr.buffer_size);
        //fbe_memory_request_get_entry_mark_free(memory_request_p, &memory_ptr);
        //fbe_memory_free_entry(memory_ptr);
		fbe_memory_free_request_entry(memory_request_p);

        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_INSUFFICIENT_RESOURCES);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return status;
    }

    new_packet_p = mem_alloc_chunks_addr.packet_array_p;
    buffer_p = mem_alloc_chunks_addr.buffer_array_p;
     
    while(index < width)
    {
        /* Initialize the sub packets. */
        fbe_transport_initialize_sep_packet(new_packet_p[index]);
        new_sep_payload_p = fbe_transport_get_payload_ex(new_packet_p[index]);

        /* Initialize the control buffer. */
        rebuild_status_p = (fbe_raid_group_rebuild_status_t *) buffer_p[index];

        /* Initialize the edge index as a context data. */
        edge_index_p = (fbe_edge_index_t *) (buffer_p[index] + sizeof(fbe_raid_group_rebuild_status_t));
        *edge_index_p = index;

        /* Allocate and initialize the control operation. */
        new_control_operation_p = fbe_payload_ex_allocate_control_operation(new_sep_payload_p);
        fbe_payload_control_build_operation(new_control_operation_p,
                                            FBE_RAID_GROUP_CONTROL_CODE_GET_REBUILD_STATUS,
                                            rebuild_status_p,
                                            sizeof(fbe_raid_group_rebuild_status_t));

        /* Initialize the new packet with FBE_PACKET_FLAG_EXTERNAL same as the master packet */
        fbe_transport_get_packet_attr(packet_p, &packet_attr);
        if (packet_attr & FBE_PACKET_FLAG_EXTERNAL) 
        {
            fbe_transport_set_packet_attr(new_packet_p[index], FBE_PACKET_FLAG_EXTERNAL);            
        }

        /* Set completion functon. */
        fbe_transport_set_completion_function(new_packet_p[index],
                                              fbe_striper_get_rebuild_status_downstream_mirror_object_subpacket_completion,
                                              raid_group_p);

        /* Add the subpacket to master packet queue. */
        fbe_transport_add_subpacket(packet_p, new_packet_p[index]);
        index++;
    }

    /* Send all the subpackets together. */
    for(index = 0; index < width; index++)
    {
        fbe_base_config_get_block_edge((fbe_base_config_t *)raid_group_p, &block_edge_p, index);
        fbe_transport_set_packet_attr(new_packet_p[index], FBE_PACKET_FLAG_TRAVERSE);
        fbe_block_transport_send_control_packet(block_edge_p, new_packet_p[index]);
    }

    /* Send all the subpackets. */
     return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/******************************************************************************
 * end fbe_striper_get_rebuild_status_downstream_mirror_memory_allocation_completion()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_striper_get_rebuild_status_downstream_mirror_object_subpacket_completion()
 ******************************************************************************
 * @brief
 *  This function is used to handle the subpacket completion to get the rebuild
 *  status for all the downstream objects.
 *
 * @param memory_request_p  - memory request.
 * @param context           - context passed to acquire the memory.
 *
 * @return status           - status of the operation.
 *
 * @author
 *  06/18/2012 - Created. Vera Wang
 *
 ******************************************************************************/
static fbe_status_t 
fbe_striper_get_rebuild_status_downstream_mirror_object_subpacket_completion(fbe_packet_t * packet_p,
                                                                              fbe_packet_completion_context_t context)
{
    fbe_raid_group_t *                                          raid_group_p = NULL;
    fbe_packet_t *                                              master_packet_p = NULL;
    fbe_payload_ex_t *                                          master_sep_payload_p = NULL;
    fbe_payload_control_operation_t *                           master_control_operation_p = NULL;
    fbe_payload_ex_t *                                          sep_payload_p = NULL;
    fbe_payload_control_operation_t *                           control_operation_p = NULL;
    fbe_payload_control_status_t                                control_status;
    fbe_payload_control_status_qualifier_t                      control_qualifier;
    fbe_status_t                                                status;
    fbe_raid_group_rebuild_status_t *                           rebuild_status_p = NULL;
    fbe_raid_group_rebuild_status_t*                            master_rebuild_status_p = NULL;
    fbe_base_config_memory_alloc_chunks_address_t               mem_alloc_chunks_addr = {0};
    fbe_sg_element_t                                           *pre_read_sg_list_p = NULL;
    fbe_payload_pre_read_descriptor_t                          *pre_read_desc_p = NULL;
    fbe_u8_t **                                                 buffer_p = NULL;
    fbe_u32_t                                                   index= 0;
    fbe_u32_t                                                   width;
    fbe_bool_t                                                  is_empty;

    /* Cast the context to base config object pointer. */
    raid_group_p = (fbe_raid_group_t *) context;
    fbe_base_config_get_width((fbe_base_config_t *) raid_group_p, &width);

    /* Get the master packet, its payload and associated control operation. */ 
    master_packet_p = fbe_transport_get_master_packet(packet_p);
    master_sep_payload_p = fbe_transport_get_payload_ex(master_packet_p);
    master_control_operation_p = fbe_payload_ex_get_control_operation(master_sep_payload_p);

    /* Get the control buffer of the master packet. */
    fbe_payload_control_get_buffer(master_control_operation_p, &master_rebuild_status_p);

    /* Get the control operation of the subpacket. */
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(sep_payload_p);
    /* Get the control buffer of the packet. */
    fbe_payload_control_get_buffer(control_operation_p, &rebuild_status_p);

    /* Get the status of the subpacket operation. */
    status = fbe_transport_get_status_code(packet_p);
    fbe_payload_control_get_status(control_operation_p, &control_status);
    fbe_payload_control_get_status_qualifier(control_operation_p, &control_qualifier);

    /* If any of the subpacket failed with bad status then update the master status. */
    if(status != FBE_STATUS_OK)
    {
        fbe_transport_set_status(master_packet_p, status, 0);
    }

    /* If any of the subpacket failed with failure status then update the master status. */
    /*!@todo we can use error predence here if required. */
    if(control_status != FBE_PAYLOAD_CONTROL_STATUS_OK)
    {
        fbe_payload_control_set_status(master_control_operation_p, control_status);
        fbe_payload_control_set_status_qualifier(master_control_operation_p, control_qualifier);
    }

    /* Check if we have more subpacket to process from master queue. */
    fbe_transport_remove_subpacket_is_queue_empty(packet_p, &is_empty);

    /* When the queue is empty, all subpackets have completed. */

    if (is_empty)
    {     
        /* once all subpacket completes, we have to update the master_rebuild_status using the
           value got from subpacket which are stored on the memory_request in master packet. */
        mem_alloc_chunks_addr.buffer_size = sizeof(fbe_raid_group_rebuild_status_t) + sizeof(fbe_edge_index_t);
        mem_alloc_chunks_addr.number_of_packets = width;
        mem_alloc_chunks_addr.buffer_count = width;
        mem_alloc_chunks_addr.sg_element_count = 0;
        mem_alloc_chunks_addr.sg_list_p = NULL;
        mem_alloc_chunks_addr.assign_pre_read_desc = FALSE;
        mem_alloc_chunks_addr.pre_read_sg_list_p = &pre_read_sg_list_p;
        mem_alloc_chunks_addr.pre_read_desc_p = &pre_read_desc_p;
        
        status = fbe_base_config_memory_assign_address_from_allocated_chunks((fbe_base_config_t *) raid_group_p,
                                                                             &master_packet_p->memory_request,
                                                                             &mem_alloc_chunks_addr);
        if(status != FBE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                                  FBE_TRACE_LEVEL_WARNING,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s memory distribution failed, status:0x%x\n", 
                                  __FUNCTION__, status);
            fbe_memory_free_request_entry(&master_packet_p->memory_request);

            fbe_payload_control_set_status(master_control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_INSUFFICIENT_RESOURCES);
            fbe_transport_set_status(master_packet_p, FBE_STATUS_OK, 0);
            fbe_transport_complete_packet(master_packet_p);
            return status;
        }

        buffer_p = mem_alloc_chunks_addr.buffer_array_p;
        rebuild_status_p = (fbe_raid_group_rebuild_status_t *)buffer_p[0];
        master_rebuild_status_p->rebuild_checkpoint = rebuild_status_p->rebuild_checkpoint;
        master_rebuild_status_p->rebuild_percentage = rebuild_status_p->rebuild_percentage;
        while(index < width)
        {
            /* Initialize the control buffer. */
            rebuild_status_p = (fbe_raid_group_rebuild_status_t *) buffer_p[index];
            if (master_rebuild_status_p->rebuild_checkpoint > rebuild_status_p->rebuild_checkpoint) 
            {
                master_rebuild_status_p->rebuild_checkpoint = rebuild_status_p->rebuild_checkpoint;
                master_rebuild_status_p->rebuild_percentage = rebuild_status_p->rebuild_percentage;
            }
            index++;
        }

        /* Destroy the subpacket. */
        fbe_transport_destroy_subpackets(master_packet_p); 
        fbe_memory_free_request_entry(&master_packet_p->memory_request);
        fbe_transport_set_status(master_packet_p, FBE_STATUS_OK, 0);
        fbe_payload_control_set_status (master_control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_OK);
        fbe_transport_complete_packet(master_packet_p);         
    }
    else
    {
        /* Not done with processing sub packets.
         */
    }

    return FBE_STATUS_MORE_PROCESSING_REQUIRED; 
}
/******************************************************************************
 * end fbe_striper_get_rebuild_status_downstream_mirror_object_subpacket_completion()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_striper_get_lun_percent_rebuilt_raid10_extent()
 ******************************************************************************
 * @brief
 *  This function is used to get the rebuild status for the RAID10 extent. It 
 *  allocates memory for the downstream raid group objects to get rg rebuild status.
 *
 * @param raid_group_p  - raid group object.
 * @param packet_p      - Pointer to the packet.
 *
 * @return status       - status of the operation.
 *
 * @author
 *  06/18/2012 - Created. Vera Wang
 *
 ******************************************************************************/
static fbe_status_t fbe_striper_get_lun_percent_rebuilt_raid10_extent(fbe_raid_group_t * raid_group_p,
                                                                         fbe_packet_t * packet_p)
{
    fbe_status_t                                status = FBE_STATUS_OK;
    fbe_raid_group_get_lun_percent_rebuilt_t   *lun_percent_rebuilt_p = NULL;
    fbe_payload_ex_t                           *sep_payload_p = NULL;
    fbe_payload_control_operation_t            *control_operation_p = NULL;
    fbe_u32_t                                   width = 0;
    fbe_base_config_memory_allocation_chunks_t  mem_alloc_chunks = {0};

    /* get the rebuild percentage buffer from the packet. */
    status = fbe_raid_group_usurper_get_control_buffer(raid_group_p, packet_p, 
                                                       sizeof(fbe_raid_group_get_lun_percent_rebuilt_t),
                                                       (fbe_payload_control_buffer_t *)&lun_percent_rebuilt_p);
    if (status != FBE_STATUS_OK)
    {
        fbe_transport_set_status(packet_p, status, 0);
        status = fbe_transport_complete_packet(packet_p);
        return status;
    }

    /* get the sep payload and control operation/ */
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(sep_payload_p);
   
    /* Get the width of the raid group object. */
    fbe_base_config_get_width((fbe_base_config_t *) raid_group_p, &width);

    /* Set the various counts and buffer size */
    mem_alloc_chunks.buffer_count = width;
    mem_alloc_chunks.number_of_packets = width;
    mem_alloc_chunks.sg_element_count = 0;

    mem_alloc_chunks.buffer_size = sizeof(fbe_raid_group_get_lun_percent_rebuilt_t) + sizeof(fbe_edge_index_t);
    mem_alloc_chunks.alloc_pre_read_desc = FALSE;
    
    /* Allocate the subpackets to collect the rebuild status for all the edges. */
    status = fbe_base_config_memory_allocate_chunks((fbe_base_config_t *) raid_group_p,
                                                    packet_p,
                                                    mem_alloc_chunks,
                                                    (fbe_memory_completion_function_t)fbe_striper_get_lun_percent_rebuilt_downstream_mirror_memory_allocation_completion); /* SAFEBUG - cast to supress warning but must fix because memory completion function shouldn't return status */
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s memory allocation failed, status:0x%x\n", __FUNCTION__, status);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_complete_packet(packet_p);
        return status; 
    }

    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_striper_get_lun_percent_rebuilt_raid10_extent()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_striper_get_lun_percent_rebuilt_downstream_mirror_memory_allocation_completion()
 ******************************************************************************
 * @brief
 *  This function is used to get the rebuild status for all the downstream
 *  mirror objects.
 *
 * @param memory_request_p  - memory request.
 * @param context           - context passed to acquire the memory.
 *
 * @return status           - status of the operation.
 *
 * @author
 *  06/27/2011 - Created. Vera Wang
 *
 ******************************************************************************/
static fbe_status_t
fbe_striper_get_lun_percent_rebuilt_downstream_mirror_memory_allocation_completion(fbe_memory_request_t * memory_request_p,
                                                                              fbe_memory_completion_context_t context)
{
    fbe_status_t                                    status = FBE_STATUS_OK;
    fbe_packet_t                                   *packet_p = NULL;
    fbe_packet_t                                  **new_packet_p = NULL;
    fbe_u8_t                                      **buffer_p = NULL;
    fbe_payload_ex_t                               *sep_payload_p = NULL;
    fbe_payload_ex_t                               *new_sep_payload_p = NULL;
    fbe_payload_control_operation_t                *control_operation_p = NULL;
    fbe_payload_control_operation_t                *new_control_operation_p = NULL;
    fbe_raid_group_get_lun_percent_rebuilt_t       *master_lun_percent_rebuilt_p = NULL;
    fbe_raid_group_get_lun_percent_rebuilt_t       *lun_percent_rebuilt_p = NULL;
    fbe_raid_group_t                               *raid_group_p = NULL;
    fbe_u32_t                                       width = 0;
    fbe_edge_index_t                               *edge_index_p = NULL;
    fbe_u32_t                                       index = 0;
    fbe_block_edge_t                               *block_edge_p = NULL;
    fbe_base_config_memory_alloc_chunks_address_t   mem_alloc_chunks_addr = {0};
    fbe_sg_element_t                               *pre_read_sg_list_p = NULL;
    fbe_payload_pre_read_descriptor_t              *pre_read_desc_p = NULL;
    fbe_packet_attr_t                               packet_attr;

    /* Get the pointer to original packet. */
    packet_p = fbe_transport_memory_request_to_packet(memory_request_p);

    /* Get the pointer to raid group object. */
    raid_group_p = (fbe_raid_group_t *) context;

    /* Get the payload and control operation. */
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p =  fbe_payload_ex_get_control_operation(sep_payload_p);

    /* get the rebuild percentage buffer from the packet. 
     */
    status = fbe_raid_group_usurper_get_control_buffer(raid_group_p, packet_p, 
                                                       sizeof(*master_lun_percent_rebuilt_p),
                                                       (fbe_payload_control_buffer_t *)&master_lun_percent_rebuilt_p);
    if (status != FBE_STATUS_OK)
    {
        fbe_transport_set_status(packet_p, status, 0);
        status = fbe_transport_complete_packet(packet_p);
        return status;
    }

    /* Check allocation status */
    if (fbe_memory_request_is_allocation_complete(memory_request_p) == FBE_FALSE)
    {
        /* Currently this callback doesn't expect any aborted requests */
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s: memory request: 0x%p failed. request state: %d \n",
                              __FUNCTION__, memory_request_p, memory_request_p->request_state);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_FAILED;
    }

    /* Get the width of the base config object. */
    fbe_base_config_get_width((fbe_base_config_t *) raid_group_p, &width);

    /* Subpacket control buffer size. */
    mem_alloc_chunks_addr.buffer_size = sizeof(fbe_raid_group_get_lun_percent_rebuilt_t) + sizeof(fbe_edge_index_t);
    mem_alloc_chunks_addr.number_of_packets = width;
    mem_alloc_chunks_addr.buffer_count = width;
    mem_alloc_chunks_addr.sg_element_count = 0;
    mem_alloc_chunks_addr.sg_list_p = NULL;
    mem_alloc_chunks_addr.assign_pre_read_desc = FALSE;
    mem_alloc_chunks_addr.pre_read_sg_list_p = &pre_read_sg_list_p;
    mem_alloc_chunks_addr.pre_read_desc_p = &pre_read_desc_p;
    
    status = fbe_base_config_memory_assign_address_from_allocated_chunks((fbe_base_config_t *) raid_group_p,
                                                                         memory_request_p,
                                                                         &mem_alloc_chunks_addr);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s memory distribution failed, status:0x%x, width:0x%x, buffer_size:0x%x\n",
                              __FUNCTION__, status, width, mem_alloc_chunks_addr.buffer_size);
        //fbe_memory_request_get_entry_mark_free(memory_request_p, &memory_ptr);
        //fbe_memory_free_entry(memory_ptr);
		fbe_memory_free_request_entry(memory_request_p);

        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_INSUFFICIENT_RESOURCES);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return status;
    }

    new_packet_p = mem_alloc_chunks_addr.packet_array_p;
    buffer_p = mem_alloc_chunks_addr.buffer_array_p;
     
    while(index < width)
    {
        /* Initialize the sub packets. */
        fbe_transport_initialize_sep_packet(new_packet_p[index]);
        new_sep_payload_p = fbe_transport_get_payload_ex(new_packet_p[index]);

        /* Initialize the control buffer. */
        lun_percent_rebuilt_p = (fbe_raid_group_get_lun_percent_rebuilt_t *)buffer_p[index];

        /* Populate the fields from the master buffer.
         */
        *lun_percent_rebuilt_p = *master_lun_percent_rebuilt_p;

        /* Initialize the edge index as a context data. */
        edge_index_p = (fbe_edge_index_t *) (buffer_p[index] + sizeof(fbe_raid_group_get_lun_percent_rebuilt_t));
        *edge_index_p = index;

        /* Allocate and initialize the control operation. */
        new_control_operation_p = fbe_payload_ex_allocate_control_operation(new_sep_payload_p);
        fbe_payload_control_build_operation(new_control_operation_p,
                                            FBE_RAID_GROUP_CONTROL_CODE_GET_LUN_PERCENT_REBUILT,
                                            lun_percent_rebuilt_p,
                                            sizeof(*lun_percent_rebuilt_p));

        /* Initialize the new packet with FBE_PACKET_FLAG_EXTERNAL same as the master packet */
        fbe_transport_get_packet_attr(packet_p, &packet_attr);
        if (packet_attr & FBE_PACKET_FLAG_EXTERNAL) 
        {
            fbe_transport_set_packet_attr(new_packet_p[index], FBE_PACKET_FLAG_EXTERNAL);            
        }

        /* Set completion functon. */
        fbe_transport_set_completion_function(new_packet_p[index],
                                              fbe_striper_get_lun_percent_rebuilt_downstream_mirror_object_subpacket_completion,
                                              raid_group_p);

        /* Add the subpacket to master packet queue. */
        fbe_transport_add_subpacket(packet_p, new_packet_p[index]);
        index++;
    }

    /* Send all the subpackets together. */
    for(index = 0; index < width; index++)
    {
        fbe_base_config_get_block_edge((fbe_base_config_t *)raid_group_p, &block_edge_p, index);
        fbe_transport_set_packet_attr(new_packet_p[index], FBE_PACKET_FLAG_TRAVERSE);
        fbe_block_transport_send_control_packet(block_edge_p, new_packet_p[index]);
    }

    /* Send all the subpackets. */
     return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/******************************************************************************
 * end fbe_striper_get_lun_percent_rebuilt_downstream_mirror_memory_allocation_completion()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_striper_get_lun_percent_rebuilt_downstream_mirror_object_subpacket_completion()
 ******************************************************************************
 * @brief
 *  This function is used to handle the subpacket completion to get the rebuild
 *  status for all the downstream objects.
 *
 * @param memory_request_p  - memory request.
 * @param context           - context passed to acquire the memory.
 *
 * @return status           - status of the operation.
 *
 * @author
 *  06/18/2012 - Created. Vera Wang
 *
 ******************************************************************************/
static fbe_status_t 
fbe_striper_get_lun_percent_rebuilt_downstream_mirror_object_subpacket_completion(fbe_packet_t * packet_p,
                                                                              fbe_packet_completion_context_t context)
{
    fbe_status_t                                    status = FBE_STATUS_OK;
    fbe_raid_group_t                               *raid_group_p = NULL;
    fbe_packet_t                                   *master_packet_p = NULL;
    fbe_payload_ex_t                               *master_sep_payload_p = NULL;
    fbe_payload_control_operation_t                *master_control_operation_p = NULL;
    fbe_payload_ex_t                               *sep_payload_p = NULL;
    fbe_payload_control_operation_t                *control_operation_p = NULL;
    fbe_payload_control_status_t                    control_status;
    fbe_payload_control_status_qualifier_t          control_qualifier;
    fbe_raid_group_get_lun_percent_rebuilt_t       *lun_percent_rebuilt_p = NULL;
    fbe_raid_group_get_lun_percent_rebuilt_t       *master_lun_percent_rebuilt_p = NULL;
    fbe_base_config_memory_alloc_chunks_address_t   mem_alloc_chunks_addr = {0};
    fbe_sg_element_t                               *pre_read_sg_list_p = NULL;
    fbe_payload_pre_read_descriptor_t              *pre_read_desc_p = NULL;
    fbe_u8_t                                      **buffer_p = NULL;
    fbe_u32_t                                       index= 0;
    fbe_u32_t                                       width;
    fbe_bool_t                                      is_empty;

    /* Cast the context to base config object pointer. */
    raid_group_p = (fbe_raid_group_t *) context;
    fbe_base_config_get_width((fbe_base_config_t *) raid_group_p, &width);

    /* Get the master packet, its payload and associated control operation. */ 
    master_packet_p = fbe_transport_get_master_packet(packet_p);
    master_sep_payload_p = fbe_transport_get_payload_ex(master_packet_p);
    master_control_operation_p = fbe_payload_ex_get_control_operation(master_sep_payload_p);

    /* Get the control buffer of the master packet. */
    fbe_payload_control_get_buffer(master_control_operation_p, &master_lun_percent_rebuilt_p);

    /* Get the control operation of the subpacket. */
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(sep_payload_p);
    /* Get the control buffer of the packet. */
    fbe_payload_control_get_buffer(control_operation_p, &lun_percent_rebuilt_p);

    /* Get the status of the subpacket operation. */
    status = fbe_transport_get_status_code(packet_p);
    fbe_payload_control_get_status(control_operation_p, &control_status);
    fbe_payload_control_get_status_qualifier(control_operation_p, &control_qualifier);

    /* If any of the subpacket failed with bad status then update the master status. */
    if(status != FBE_STATUS_OK)
    {
        fbe_transport_set_status(master_packet_p, status, 0);
    }

    /* If any of the subpacket failed with failure status then update the master status. */
    /*!@todo we can use error predence here if required. */
    if(control_status != FBE_PAYLOAD_CONTROL_STATUS_OK)
    {
        fbe_payload_control_set_status(master_control_operation_p, control_status);
        fbe_payload_control_set_status_qualifier(master_control_operation_p, control_qualifier);
    }

    /* Check if we have more subpacket to process from master queue. */
    fbe_transport_remove_subpacket_is_queue_empty(packet_p, &is_empty);

    /* When the queue is empty, all subpackets have completed. */
    if (is_empty)
    {     
        /* once all subpacket completes, we have to update the master_rebuild_status using the
           value got from subpacket which are stored on the memory_request in master packet. */
        mem_alloc_chunks_addr.buffer_size = sizeof(fbe_raid_group_get_lun_percent_rebuilt_t) + sizeof(fbe_edge_index_t);
        mem_alloc_chunks_addr.number_of_packets = width;
        mem_alloc_chunks_addr.buffer_count = width;
        mem_alloc_chunks_addr.sg_element_count = 0;
        mem_alloc_chunks_addr.sg_list_p = NULL;
        mem_alloc_chunks_addr.assign_pre_read_desc = FALSE;
        mem_alloc_chunks_addr.pre_read_sg_list_p = &pre_read_sg_list_p;
        mem_alloc_chunks_addr.pre_read_desc_p = &pre_read_desc_p;
        
        status = fbe_base_config_memory_assign_address_from_allocated_chunks((fbe_base_config_t *) raid_group_p,
                                                                             &master_packet_p->memory_request,
                                                                             &mem_alloc_chunks_addr);
        if(status != FBE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                                  FBE_TRACE_LEVEL_WARNING,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s memory distribution failed, status:0x%x\n", 
                                  __FUNCTION__, status);
            fbe_memory_free_request_entry(&master_packet_p->memory_request);

            fbe_payload_control_set_status(master_control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_INSUFFICIENT_RESOURCES);
            fbe_transport_set_status(master_packet_p, FBE_STATUS_OK, 0);
            fbe_transport_complete_packet(master_packet_p);
            return status;
        }

        /* Start with the first mirror raid group and get the lowest lun rebuild
         * checkpoint.
         */
        buffer_p = mem_alloc_chunks_addr.buffer_array_p;
        lun_percent_rebuilt_p = (fbe_raid_group_get_lun_percent_rebuilt_t *)buffer_p[0];
        master_lun_percent_rebuilt_p->lun_rebuild_checkpoint = lun_percent_rebuilt_p->lun_rebuild_checkpoint;
        master_lun_percent_rebuilt_p->lun_percent_rebuilt = lun_percent_rebuilt_p->lun_percent_rebuilt;
        master_lun_percent_rebuilt_p->debug_flags = lun_percent_rebuilt_p->debug_flags;
        master_lun_percent_rebuilt_p->is_rebuild_in_progress = lun_percent_rebuilt_p->is_rebuild_in_progress;
        
        for (index = 0; index < width; index++)
        {
            /* Starting with the first mirror raid group, get the lowest checkpoint.
             */
            lun_percent_rebuilt_p = (fbe_raid_group_get_lun_percent_rebuilt_t *)buffer_p[index];
            if (lun_percent_rebuilt_p->lun_rebuild_checkpoint < master_lun_percent_rebuilt_p->lun_rebuild_checkpoint)
            {
                master_lun_percent_rebuilt_p->lun_rebuild_checkpoint = lun_percent_rebuilt_p->lun_rebuild_checkpoint;
                master_lun_percent_rebuilt_p->lun_percent_rebuilt = lun_percent_rebuilt_p->lun_percent_rebuilt;
            }
        }

        /* Destroy the subpacket. */
        fbe_transport_destroy_subpackets(master_packet_p); 
        fbe_memory_free_request_entry(&master_packet_p->memory_request);
        fbe_transport_set_status(master_packet_p, FBE_STATUS_OK, 0);
        fbe_payload_control_set_status (master_control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_OK);
        fbe_transport_complete_packet(master_packet_p);         
    }
    else
    {
        /* Not done with processing sub packets.
         */
    }

    return FBE_STATUS_MORE_PROCESSING_REQUIRED; 
}
/******************************************************************************
 * end fbe_striper_get_lun_percent_rebuilt_downstream_mirror_object_subpacket_completion()
 ******************************************************************************/

/*!****************************************************************************
 *          fbe_striper_usurper_get_raid10_extended_media_error_handling()
 ****************************************************************************** 
 * 
 * @brief   This function returns the current values for the Extended Media
 *          Error Handling (EMEH) for the raid group specified.
 * 
 * @param   raid_group_p - pointer to raid group object to get thresholds for
 * @param   packet_p - pointer to the packet
 *
 * @return status 
 *
 * @author 
 *  05/14/2015  Ron Proulx - Created.
 * 
 ******************************************************************************/
static fbe_status_t fbe_striper_usurper_raid10_get_extended_media_error_handling(fbe_raid_group_t *raid_group_p, fbe_packet_t *packet_p)
{
    fbe_status_t                                        status = FBE_STATUS_OK;
    fbe_payload_control_status_t			            control_status;
    fbe_lifecycle_state_t                               lifecycle_state;
    fbe_u32_t                                           width;
    fbe_u32_t                                           physical_width;
    fbe_bool_t                                          b_downstream_retrieved = FBE_FALSE;
    fbe_u32_t                                           position;
    fbe_u32_t                                           physical_position;
    fbe_raid_position_bitmask_t                         rl_bitmask;
    fbe_raid_group_get_extended_media_error_handling_t *get_emeh_info_p = NULL;
    fbe_payload_control_operation_t                    *control_operation_p = NULL;
    fbe_payload_ex_t                                   *sep_payload_p = NULL;
    fbe_raid_group_get_extended_media_error_handling_t *downstream_emeh_info_p = NULL;
    fbe_payload_control_operation_t                    *downstream_control_operation_p = NULL;
    fbe_payload_ex_t                                   *downstream_payload_p = NULL; 
    fbe_block_edge_t                                   *block_edge_p = NULL;

    FBE_RAID_GROUP_TRACE_FUNC_ENTRY(raid_group_p);

    /* get the control operation of the subpacket. */
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(sep_payload_p);
    status = fbe_raid_group_usurper_get_control_buffer(raid_group_p, packet_p, 
                                                       sizeof(fbe_raid_group_get_extended_media_error_handling_t),
                                                       (fbe_payload_control_buffer_t)&get_emeh_info_p);
    if (status != FBE_STATUS_OK){ 
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED; 
    }

    /* Can only send this is raid group is ready.
     */
    fbe_base_object_get_lifecycle_state((fbe_base_object_t *)raid_group_p, &lifecycle_state);
    if (lifecycle_state != FBE_LIFECYCLE_STATE_READY)
    {
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s - lifecycle state: %d is not READY\n",
                              __FUNCTION__, lifecycle_state);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_OK;
    }

    /*! @note Technically RAID-10 doesn't support EMEH (the downstream mirror
     *        raid groups do).  But we coalesce the information from the
     *        downstream raid groups.
     *
     *        Invoke the method that will update the raid group values to the 
     *        class values as neccessary.
     */
    fbe_raid_group_is_extended_media_error_handling_capable(raid_group_p);

    /*        Simply send the request to all positions.  First populate the
     *        RAID-10 information.
     */
    fbe_raid_group_get_width(raid_group_p, &width);
    physical_width = (width * 2);
    fbe_raid_group_lock(raid_group_p);
    get_emeh_info_p->width = physical_width;
    fbe_raid_group_get_emeh_enabled_mode(raid_group_p, &get_emeh_info_p->enabled_mode);
    fbe_raid_group_get_emeh_current_mode(raid_group_p, &get_emeh_info_p->current_mode);
    fbe_raid_group_emeh_get_class_default_increase(&get_emeh_info_p->default_threshold_increase);
    fbe_raid_group_get_emeh_reliability(raid_group_p, &get_emeh_info_p->reliability);
    fbe_raid_group_get_emeh_request(raid_group_p, &get_emeh_info_p->active_command);
    fbe_raid_group_get_emeh_paco_position(raid_group_p, &get_emeh_info_p->paco_position);
    fbe_raid_group_get_rb_logging_bitmask(raid_group_p, &rl_bitmask);
    fbe_raid_group_unlock(raid_group_p);

    /* Allocate a buffer for each mirror request.
     */
    downstream_emeh_info_p = (fbe_raid_group_get_extended_media_error_handling_t *)fbe_transport_allocate_buffer();
    if (downstream_emeh_info_p == NULL)
    {
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return status;
    }

    /* Send to each mirror raid group.
     */
    for (position = 0; position < width; position++)
    {
        /* Initialize both physical positions.
        */
        physical_position = position * 2;
        get_emeh_info_p->dieh_reliability[physical_position] = FBE_U32_MAX;
        get_emeh_info_p->dieh_mode[physical_position] = FBE_U32_MAX;
        get_emeh_info_p->media_weight_adjust[physical_position] = FBE_U32_MAX;
        get_emeh_info_p->vd_paco_position[physical_position] = FBE_EDGE_INDEX_INVALID;
        get_emeh_info_p->dieh_reliability[physical_position + 1] = FBE_U32_MAX;
        get_emeh_info_p->dieh_mode[physical_position + 1] = FBE_U32_MAX;
        get_emeh_info_p->media_weight_adjust[physical_position + 1] = FBE_U32_MAX;
        get_emeh_info_p->vd_paco_position[physical_position + 1] = FBE_EDGE_INDEX_INVALID;

        /* Don't send to degraded position.
         */
        if (((1 << position) & rl_bitmask) == 0)
        {
            /* Allocate the control payload
             */
            downstream_payload_p = fbe_transport_get_payload_ex(packet_p);
            downstream_control_operation_p = fbe_payload_ex_allocate_control_operation(downstream_payload_p);
            fbe_payload_control_build_operation(downstream_control_operation_p,
                                                FBE_RAID_GROUP_CONTROL_CODE_GET_EXTENDED_MEDIA_ERROR_HANDLING,
                                                downstream_emeh_info_p,
                                                sizeof(fbe_raid_group_get_extended_media_error_handling_t));

            /* Send to non-degraded*/
            fbe_base_config_get_block_edge((fbe_base_config_t*)raid_group_p, &block_edge_p, position);
            fbe_transport_set_packet_attr(packet_p, FBE_PACKET_FLAG_TRAVERSE);
            fbe_transport_set_sync_completion_type(packet_p, FBE_TRANSPORT_COMPLETION_TYPE_MORE_PROCESSING_REQUIRED);

            /* Send the packet and wait for its completion. */
            fbe_block_transport_send_control_packet(block_edge_p, packet_p);
            fbe_transport_wait_completion(packet_p);

            /* Get the contorl and packet status.  If it failed then continue */
            fbe_payload_control_get_status(downstream_control_operation_p, &control_status);
            fbe_payload_ex_release_control_operation(downstream_payload_p, downstream_control_operation_p);
            status = fbe_transport_get_status_code(packet_p);
            if ((status == FBE_STATUS_OK)                         &&
                (control_status == FBE_PAYLOAD_CONTROL_STATUS_OK)    )
            {
                b_downstream_retrieved = FBE_TRUE;

                /* First populate the per-position information.
                 */
                get_emeh_info_p->dieh_reliability[physical_position] = downstream_emeh_info_p->dieh_reliability[0];
                get_emeh_info_p->dieh_mode[physical_position] = downstream_emeh_info_p->dieh_mode[0];
                get_emeh_info_p->media_weight_adjust[physical_position] = downstream_emeh_info_p->media_weight_adjust[0];
                get_emeh_info_p->vd_paco_position[physical_position] = downstream_emeh_info_p->vd_paco_position[0];
                get_emeh_info_p->dieh_reliability[physical_position + 1] = downstream_emeh_info_p->dieh_reliability[1];
                get_emeh_info_p->dieh_mode[physical_position + 1] = downstream_emeh_info_p->dieh_mode[1];
                get_emeh_info_p->media_weight_adjust[physical_position + 1] = downstream_emeh_info_p->media_weight_adjust[1];
                get_emeh_info_p->vd_paco_position[physical_position + 1] = downstream_emeh_info_p->vd_paco_position[1];
                
                /* If enabled trace some information.
                 */
                fbe_raid_group_trace(raid_group_p,
                                     FBE_TRACE_LEVEL_INFO,
                                     FBE_RAID_GROUP_DEBUG_FLAG_EXTENDED_MEDIA_ERROR_HANDLING,
                                     "raid10 get emeh for mir pos: %d 0: rel %d mode: %d media: %d 1: rel %d mode: %d media: %d\n", 
                                     position,
                                     get_emeh_info_p->dieh_reliability[physical_position],
                                     get_emeh_info_p->dieh_mode[physical_position],
                                     get_emeh_info_p->media_weight_adjust[physical_position],
                                     get_emeh_info_p->dieh_reliability[physical_position + 1],
                                     get_emeh_info_p->dieh_mode[physical_position + 1],
                                     get_emeh_info_p->media_weight_adjust[physical_position + 1]);

                /* If proactive copy is running on one of the positions populate.
                 */
                if (get_emeh_info_p->vd_paco_position[physical_position] != FBE_EDGE_INDEX_INVALID)
                {
                    get_emeh_info_p->paco_position = get_emeh_info_p->vd_paco_position[physical_position];
                    fbe_raid_group_trace(raid_group_p,
                                         FBE_TRACE_LEVEL_INFO,
                                         FBE_RAID_GROUP_DEBUG_FLAG_EXTENDED_MEDIA_ERROR_HANDLING,
                                         "raid10 get emeh for mir pos: %d paco on mir pos: %d physical position: %d\n",
                                         position, get_emeh_info_p->paco_position,
                                         physical_position);
                }
                else if (get_emeh_info_p->vd_paco_position[physical_position + 1] != FBE_EDGE_INDEX_INVALID)
                {
                    get_emeh_info_p->paco_position = get_emeh_info_p->vd_paco_position[physical_position + 1];
                    fbe_raid_group_trace(raid_group_p,
                                         FBE_TRACE_LEVEL_INFO,
                                         FBE_RAID_GROUP_DEBUG_FLAG_EXTENDED_MEDIA_ERROR_HANDLING,
                                         "raid10 get emeh for mir pos: %d paco on mir pos: %d physical position: %d\n",
                                         position, get_emeh_info_p->paco_position,
                                         (physical_position + 1));
                }

                /* If the current reliability is unknown and the new one isn't then set it.
                 */
                if ((get_emeh_info_p->reliability == FBE_RAID_EMEH_RELIABILITY_UNKNOWN)     &&
                    (downstream_emeh_info_p->reliability != FBE_RAID_EMEH_RELIABILITY_UNKNOWN)    )
                {
                    fbe_raid_group_trace(raid_group_p,
                                         FBE_TRACE_LEVEL_INFO,
                                         FBE_RAID_GROUP_DEBUG_FLAG_EXTENDED_MEDIA_ERROR_HANDLING,
                                         "raid10 get emeh for mir pos: %d change reliability from: %d to: %d\n",
                                         position, get_emeh_info_p->reliability, downstream_emeh_info_p->reliability); 
                    get_emeh_info_p->reliability = downstream_emeh_info_p->reliability;
                }
                else if ((get_emeh_info_p->reliability == FBE_RAID_EMEH_RELIABILITY_VERY_HIGH)     &&
                         (downstream_emeh_info_p->reliability == FBE_RAID_EMEH_RELIABILITY_LOW)    )
                {
                    fbe_raid_group_trace(raid_group_p,
                                         FBE_TRACE_LEVEL_INFO,
                                         FBE_RAID_GROUP_DEBUG_FLAG_EXTENDED_MEDIA_ERROR_HANDLING,
                                         "raid10 get emeh for mir pos: %d change reliability from: %d to: %d\n",
                                         position, get_emeh_info_p->reliability, downstream_emeh_info_p->reliability); 
                    get_emeh_info_p->reliability = downstream_emeh_info_p->reliability;
                }

            } /* end if status of dieh ok */
            else
            {
                fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                                      FBE_TRACE_LEVEL_WARNING,
                                      FBE_TRACE_MESSAGE_ID_INFO,
                                      "raid10 get emeh for pos: %d failed - control status: %d status: %d\n", 
                                      position, control_status, status);
            }

        } /* end if the position isn't degraded*/
    } /* end for all positions*/

    /* If we never got a response trace an error and set status
     */
    fbe_transport_release_buffer(downstream_emeh_info_p);
    if (b_downstream_retrieved == FBE_FALSE)
    {
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s - failed to send dieh to any position\n",
                              __FUNCTION__);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
    }
    else
    {
        /* Trace an information message.
         */
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "raid10 get emeh physical width: %d current mode: %d reliability: %d paco position: %d\n",
                              physical_width, get_emeh_info_p->current_mode,
                              get_emeh_info_p->reliability, get_emeh_info_p->paco_position);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_OK);
    }

    /* Complete the request*/
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);
    return FBE_STATUS_OK;
}
/***********************************************************************
 * end fbe_striper_usurper_raid10_get_extended_media_error_handling()
 ***********************************************************************/

/*!****************************************************************************
 *          fbe_striper_usurper_raid10_set_extended_media_error_handling()
 ****************************************************************************** 
 * 
 * @brief   This function set the current values for the Extended Media
 *          Error Handling (EMEH) for the raid group specified.
 * 
 * @param   raid_group_p - pointer to raid group object to get thresholds for
 * @param   packet_p - pointer to the packet
 *
 * @return status 
 *
 * @author 
 *  05/14/2015  Ron Proulx - Created.
 * 
 ******************************************************************************/
static fbe_status_t fbe_striper_usurper_raid10_set_extended_media_error_handling(fbe_raid_group_t *raid_group_p, fbe_packet_t *packet_p)
{
    fbe_status_t                                        status = FBE_STATUS_OK;
    fbe_payload_control_status_t			            control_status;
    fbe_lifecycle_state_t                               lifecycle_state;
    fbe_u32_t                                           width;
    fbe_u32_t                                           position;
    fbe_raid_group_set_extended_media_error_handling_t *set_emeh_p = NULL;
    fbe_payload_control_operation_t                    *control_operation_p = NULL;
    fbe_payload_ex_t                                   *sep_payload_p = NULL;
    fbe_raid_group_set_extended_media_error_handling_t  downstream_set_emeh;
    fbe_payload_control_operation_t                    *downstream_control_operation_p = NULL;
    fbe_payload_ex_t                                   *downstream_payload_p = NULL; 
    fbe_block_edge_t                                   *block_edge_p = NULL;    

    FBE_RAID_GROUP_TRACE_FUNC_ENTRY(raid_group_p);

    /* get the control operation of the subpacket. */
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(sep_payload_p);
    status = fbe_raid_group_usurper_get_control_buffer(raid_group_p, packet_p, 
                                                       sizeof(fbe_raid_group_set_extended_media_error_handling_t),
                                                       (fbe_payload_control_buffer_t)&set_emeh_p);
    if (status != FBE_STATUS_OK){ 
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED; 
    }
   
    /* Can only send this is raid group is ready.
     */
    fbe_base_object_get_lifecycle_state((fbe_base_object_t *)raid_group_p, &lifecycle_state);
    if (lifecycle_state != FBE_LIFECYCLE_STATE_READY)
    {
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s - lifecycle state: %d is not READY\n",
                              __FUNCTION__, lifecycle_state);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_OK;
    }

    /* Simply send the request to all positions.  First populate the
     * raid group class information.
     */
    fbe_raid_group_get_width(raid_group_p, &width);
    for (position = 0; position < width; position++)
    {
        /* Send to ALL positions. Allocate the control payload
         */
        downstream_payload_p = fbe_transport_get_payload_ex(packet_p);
        downstream_control_operation_p = fbe_payload_ex_allocate_control_operation(downstream_payload_p);
        fbe_payload_control_build_operation(downstream_control_operation_p,
                                            FBE_RAID_GROUP_CONTROL_CODE_SET_EXTENDED_MEDIA_ERROR_HANDLING,
                                            &downstream_set_emeh,
                                            sizeof(fbe_raid_group_set_extended_media_error_handling_t));

        /* Send to non-degraded. Copy original request.
         */
        downstream_set_emeh = *set_emeh_p;
        fbe_base_config_get_block_edge((fbe_base_config_t*)raid_group_p, &block_edge_p, position);
        fbe_transport_set_packet_attr(packet_p, FBE_PACKET_FLAG_TRAVERSE);
        fbe_transport_set_sync_completion_type(packet_p, FBE_TRANSPORT_COMPLETION_TYPE_MORE_PROCESSING_REQUIRED);

        /* Send the packet and wait for its completion. */
        fbe_block_transport_send_control_packet(block_edge_p, packet_p);
        fbe_transport_wait_completion(packet_p);

        /* Get the contorl and packet status.  If it failed then continue */
        fbe_payload_control_get_status(downstream_control_operation_p, &control_status);
        fbe_payload_ex_release_control_operation(downstream_payload_p, downstream_control_operation_p);
        status = fbe_transport_get_status_code(packet_p);
        if ((status != FBE_STATUS_OK)                         ||
            (control_status != FBE_PAYLOAD_CONTROL_STATUS_OK)    )
        {
            fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                                  FBE_TRACE_LEVEL_WARNING,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "raid10 set emeh for pos: %d failed - control status: %d status: %d\n", 
                                  position, control_status, status);
        }

    } /* end for all positions*/


    /*! @todo Change to debug */
    fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "raid10 set emeh: set control: %d is paco: %d change threshold: %d percent increase: %d\n", 
                          set_emeh_p->set_control, set_emeh_p->b_is_paco,
                          set_emeh_p->b_change_threshold, set_emeh_p->percent_threshold_increase);  

    /* Complete the request*/
    fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);
    return FBE_STATUS_OK;
}
/***********************************************************************
 * end fbe_striper_usurper_raid10_set_extended_media_error_handling()
 ***********************************************************************/

/*!****************************************************************************
 *          fbe_striper_usurper_get_wear_level_downstream()
 ****************************************************************************** 
 * 
 * @brief   This function gets the max wear level collected from the drives
 *          in the raid group. Special for r10
 * 
 * @param   raid_group_p - pointer to raid group object to query
 *              for
 * @param   packet_p - pointer to the packet
 *
 * @return status 
 *
 * @author 
 *  06/25/2015  Deanna Heng - Created.
 * 
 ******************************************************************************/
fbe_status_t fbe_striper_usurper_get_wear_level_downstream(fbe_raid_group_t *raid_group_p, fbe_packet_t *packet_p)
{
    fbe_status_t                                status;
    fbe_u32_t                                   width;
    fbe_u32_t                                   index;
    fbe_base_config_memory_allocation_chunks_t  mem_alloc_chunks;
    fbe_raid_group_get_wear_level_downstream_t  *wear_level_downstream_p = NULL;
    fbe_payload_ex_t                            *sep_payload_p = NULL;
    fbe_payload_control_operation_t             *control_operation_p = NULL;


    /* get the sep payload and control operation/ */
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(sep_payload_p);

    /* get the control buffer pointer from the packet payload. */
    status = fbe_raid_group_usurper_get_control_buffer(raid_group_p,
                                                       packet_p, 
                                                       sizeof(fbe_raid_group_get_wear_level_downstream_t),
                                                       (fbe_payload_control_buffer_t)&wear_level_downstream_p);
    if (status != FBE_STATUS_OK) 
    { 
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED; 
    }

    /* initialize data zero before sending the request down. */
    wear_level_downstream_p->max_wear_level_percent = 0;
    wear_level_downstream_p->edge_index = 0;
    wear_level_downstream_p->max_erase_count = 0;
    wear_level_downstream_p->eol_PE_cycles = 0;
    wear_level_downstream_p->power_on_hours = 0;
    for(index = 0; index < FBE_RAID_MAX_DISK_ARRAY_WIDTH; index++)
    {
        wear_level_downstream_p->drive_wear_level[index].current_pe_cycle = 0;
        wear_level_downstream_p->drive_wear_level[index].max_pe_cycle = 0;
        wear_level_downstream_p->drive_wear_level[index].power_on_hours = 0;
    }


    /* get the width of the raid group object. */
    fbe_base_config_get_width((fbe_base_config_t *) raid_group_p, &width);

    /* We need to send the request down to all the edges and so get the width and allocate
     * packet for every edge */
    /* Set the various counts and buffer size */
    mem_alloc_chunks.buffer_count = width;
    mem_alloc_chunks.number_of_packets = width;
    mem_alloc_chunks.sg_element_count = 0;
    mem_alloc_chunks.buffer_size = sizeof(fbe_raid_group_get_wear_level_downstream_t) + sizeof(fbe_edge_index_t);
    mem_alloc_chunks.alloc_pre_read_desc = FALSE;
    
    /* allocate the subpackets to collect the zero checkpoint for all the edges. */
    status = fbe_base_config_memory_allocate_chunks((fbe_base_config_t *) raid_group_p,
                                                    packet_p,
                                                    mem_alloc_chunks,
                                                    (fbe_memory_completion_function_t)fbe_striper_get_wear_level_allocation_completion); /* SAFEBUG - cast to supress warning but must fix because memory completion function shouldn't return status */
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s memory allocation failed, status:0x%x\n", __FUNCTION__, status);
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED; 
    }

    return FBE_STATUS_MORE_PROCESSING_REQUIRED; 

}
/**************************************************************
 * end fbe_striper_usurper_get_wear_level_downstream()
 **************************************************************/


/*!****************************************************************************
 * fbe_striper_get_wear_level_allocation_completion()
 ******************************************************************************
 * @brief
 *  This function gets called after memory has been allocated and sends the 
 *  request down to the the block edge
 *
 * @param memory_request_p  - memory request.
 * @param context           - context passed to acquire the memory.
 *
 * @return status           - status of the operation.
 *
 * @author
 *  06/25/2015 - Created. Deanna Heng
 *
 ******************************************************************************/
static fbe_status_t
fbe_striper_get_wear_level_allocation_completion(fbe_memory_request_t * memory_request_p,
                                                 fbe_memory_completion_context_t context)
{
    fbe_packet_t                                   *packet_p = NULL;
    fbe_packet_t                                  **new_packet_p = NULL;
    fbe_u8_t                                      **buffer_p = NULL;
    fbe_payload_ex_t                               *new_sep_payload_p = NULL;
    fbe_payload_control_operation_t                *new_control_operation_p = NULL;
    fbe_status_t                                    status = FBE_STATUS_OK;
    fbe_raid_group_t                               *raid_group_p = NULL;
    fbe_u32_t                                       width = 0;
    fbe_u32_t                                       index = 0;
    fbe_block_edge_t                               *block_edge_p = NULL;
    fbe_base_config_memory_alloc_chunks_address_t   mem_alloc_chunks_addr = {0};
    fbe_sg_element_t                               *pre_read_sg_list_p = NULL;
    fbe_payload_pre_read_descriptor_t              *pre_read_desc_p = NULL;
    fbe_packet_attr_t                               packet_attr;
    fbe_raid_group_get_wear_level_downstream_t     *mirror_ssd_stats_p;
    fbe_edge_index_t                               *edge_index_p = NULL;
    
    /* get the pointer to original packet. */
    packet_p = fbe_transport_memory_request_to_packet(memory_request_p);

    /* get the pointer to raid group object. */
    raid_group_p = (fbe_raid_group_t *) context;

    /* Check allocation status */
    if (fbe_memory_request_is_allocation_complete(memory_request_p) == FBE_FALSE)
    {
        /* Currently this callback doesn't expect any aborted requests */
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s: memory request: 0x%p failed. request state: %d \n",
                              __FUNCTION__, memory_request_p, memory_request_p->request_state);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED; 
    }

    /* get the width of the base config object. */
    fbe_base_config_get_width((fbe_base_config_t *) raid_group_p, &width);

    /* subpacket control buffer size. */
    mem_alloc_chunks_addr.buffer_size = sizeof(fbe_raid_group_get_wear_level_downstream_t) + sizeof(fbe_edge_index_t);
    mem_alloc_chunks_addr.number_of_packets = width;
    mem_alloc_chunks_addr.buffer_count = width;
    mem_alloc_chunks_addr.sg_element_count = 0;
    mem_alloc_chunks_addr.sg_list_p = NULL;
    mem_alloc_chunks_addr.assign_pre_read_desc = FALSE;
    mem_alloc_chunks_addr.pre_read_sg_list_p = &pre_read_sg_list_p;
    mem_alloc_chunks_addr.pre_read_desc_p = &pre_read_desc_p;
    
    /* get subpacket pointer from the allocated memory. */
    status = fbe_base_config_memory_assign_address_from_allocated_chunks((fbe_base_config_t *) raid_group_p,
                                                                         memory_request_p,
                                                                         &mem_alloc_chunks_addr);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s memory distribution failed, status:0x%x, width:0x%x, buffer_size:0x%x\n",
                              __FUNCTION__, status, width, mem_alloc_chunks_addr.buffer_size);
        fbe_memory_free_request_entry(memory_request_p);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED; 
    }

    new_packet_p = mem_alloc_chunks_addr.packet_array_p;
    buffer_p = mem_alloc_chunks_addr.buffer_array_p;

    while(index < width)
    {
        /* Initialize the sub packets. */
        fbe_transport_initialize_sep_packet(new_packet_p[index]);
        new_sep_payload_p = fbe_transport_get_payload_ex(new_packet_p[index]);

        /* initialize the control buffer. */
        mirror_ssd_stats_p = (fbe_raid_group_get_wear_level_downstream_t *) buffer_p[index];

        /* initialize the edge index as a context data. */
        edge_index_p = (fbe_edge_index_t *) (buffer_p[index] + sizeof(fbe_raid_group_get_wear_level_downstream_t));
        *edge_index_p = index;

        /* allocate and initialize the control operation. */
        new_control_operation_p = fbe_payload_ex_allocate_control_operation(new_sep_payload_p);
        fbe_payload_control_build_operation(new_control_operation_p,
                                            FBE_RAID_GROUP_CONTROL_CODE_GET_WEAR_LEVEL_DOWNSTREAM,
                                            mirror_ssd_stats_p,
                                            sizeof(fbe_raid_group_get_wear_level_downstream_t));

        /* Initialize the new packet with FBE_PACKET_FLAG_EXTERNAL same as the master packet */
        fbe_transport_get_packet_attr(packet_p, &packet_attr);
        if (packet_attr & FBE_PACKET_FLAG_EXTERNAL) 
        {
            fbe_transport_set_packet_attr(new_packet_p[index], FBE_PACKET_FLAG_EXTERNAL);            
        }

        /* set completion functon. */
        fbe_transport_set_completion_function(new_packet_p[index],
                                              fbe_striper_get_wear_level_subpacket_completion,
                                              raid_group_p);

        /* add the subpacket to master packet queue. */
        fbe_transport_add_subpacket(packet_p, new_packet_p[index]);
        index++;
    }

    /* set the parent packet status to good before sending subpackets */
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);

    fbe_raid_group_trace(raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_RAID_GROUP_DEBUG_FLAG_DRIVE_TIER_TRACING,
                         "%s: striper: Send all mirror subpackets for drive wear level query\n",
                         __FUNCTION__);

    /* send all the subpackets together. */
    for(index = 0; index < width; index++)
    {
        fbe_base_config_get_block_edge((fbe_base_config_t *)raid_group_p, &block_edge_p, index);
        fbe_transport_set_packet_attr(new_packet_p[index], FBE_PACKET_FLAG_TRAVERSE);
        fbe_block_transport_send_control_packet(block_edge_p, new_packet_p[index]);
    }

    /* send all the subpackets. */
     return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/******************************************************************************
 * end fbe_striper_get_wear_level_allocation_completion()
 ******************************************************************************/
/*!****************************************************************************
 * fbe_striper_get_wear_level_subpacket_completion()
 ******************************************************************************
 * @brief
 *  Handle the completion of a subpacket that was sent downstream to query for the 
 *  physical drive information. Update the drive information from to the subpacket to 
 *  the master packet.
 *
 * @param packet_p          - packet.
 * @param context           - context passed to request.
 *
 * @return status           - status of the operation.
 *
 * @author
 *  2/06/2014 - Created. Deanna Heng
 *
 ******************************************************************************/
static fbe_status_t 
fbe_striper_get_wear_level_subpacket_completion(fbe_packet_t * packet_p,
                                                fbe_packet_completion_context_t context)
{
    fbe_raid_group_t *                              raid_group_p = NULL;
    fbe_packet_t *                                  master_packet_p = NULL;
    fbe_payload_ex_t *                              master_sep_payload_p = NULL;
    fbe_payload_control_operation_t *               master_control_operation_p = NULL;
    fbe_payload_ex_t *                              sep_payload_p = NULL;
    fbe_payload_control_operation_t *               control_operation_p = NULL;
    fbe_payload_control_status_t                    control_status;
    fbe_payload_control_status_qualifier_t          control_qualifier;
    fbe_bool_t                                      is_empty;
    fbe_status_t                                    status;
    fbe_raid_group_get_wear_level_downstream_t *    wear_level_downstream_p = NULL;
    fbe_raid_group_get_wear_level_downstream_t *    mirror_ssd_stats_p = NULL;
    fbe_edge_index_t *                              edge_index_p = NULL;
    fbe_u32_t                                       max_wear_level_percent = 0;
	

    /* Cast the context to base config object pointer. */
    raid_group_p = (fbe_raid_group_t *) context;

    /* Get the master packet, its payload and associated control operation. */
    master_packet_p = fbe_transport_get_master_packet(packet_p);
    master_sep_payload_p = fbe_transport_get_payload_ex(master_packet_p);
    master_control_operation_p = fbe_payload_ex_get_control_operation(master_sep_payload_p);

    /* get the control buffer of the master packet. */
    fbe_payload_control_get_buffer(master_control_operation_p, &wear_level_downstream_p);

     /* get the status of the subpacket operation. */
    status = fbe_transport_get_status_code(packet_p);

    /* Get the control operation of the subpacket. */
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(sep_payload_p);
    fbe_payload_control_get_status(control_operation_p, &control_status);
    fbe_payload_control_get_status_qualifier(control_operation_p, &control_qualifier);

    /* if any of the subpacket failed with bad status then update the master status. */
    if(status != FBE_STATUS_OK)
    {
        fbe_transport_set_status(master_packet_p, status, 0);
        /* only control operation status is processed later */
        fbe_payload_control_set_status(master_control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
    }

    fbe_raid_group_trace(raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_RAID_GROUP_DEBUG_FLAG_DRIVE_TIER_TRACING,
                         "%s drive wear level control status: 0x%x qualifier 0x%x status 0x%x\n", 
                         __FUNCTION__, control_status, control_qualifier, status);



    /* if any of the subpacket failed with failure status then update the master status. */
    /*!@todo we can use error predence here if required. */
    if(control_status != FBE_PAYLOAD_CONTROL_STATUS_OK)
    {
        fbe_transport_set_status(master_packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_payload_control_set_status(master_control_operation_p, control_status);
        fbe_payload_control_set_status_qualifier(master_control_operation_p, control_qualifier);
    }

    /* The subpacket was successful. Update the rg nonpaged with the subpacket's information */
	if((status == FBE_STATUS_OK) &&
       (control_status == FBE_PAYLOAD_CONTROL_STATUS_OK))
    {
        /* get the control buffer of the subpacket. */
        fbe_payload_control_get_buffer(control_operation_p, &mirror_ssd_stats_p);
	
        /* get the edge index of the subpacket. */
        edge_index_p = (fbe_edge_index_t *) (((fbe_u8_t *) mirror_ssd_stats_p) + sizeof(fbe_raid_group_get_wear_level_downstream_t));

        fbe_raid_group_trace(raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_RAID_GROUP_DEBUG_FLAG_DRIVE_TIER_TRACING,
                             "mirror wear index: 0x%x max percent: 0x%x poh:0x%llx\n", 
                             *edge_index_p, mirror_ssd_stats_p->max_wear_level_percent, mirror_ssd_stats_p->power_on_hours);
        fbe_raid_group_trace(raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_RAID_GROUP_DEBUG_FLAG_DRIVE_TIER_TRACING,
                             "mirror wear index: 0x%x erase_count: 0x%llx eol pe cycles:0x%llx\n", 
                             *edge_index_p, mirror_ssd_stats_p->max_erase_count, mirror_ssd_stats_p->eol_PE_cycles);

        
        wear_level_downstream_p->drive_wear_level[*edge_index_p].power_on_hours = mirror_ssd_stats_p->power_on_hours;
        wear_level_downstream_p->drive_wear_level[*edge_index_p].current_pe_cycle = mirror_ssd_stats_p->max_erase_count;
        wear_level_downstream_p->drive_wear_level[*edge_index_p].max_pe_cycle = mirror_ssd_stats_p->eol_PE_cycles;
        max_wear_level_percent = mirror_ssd_stats_p->max_wear_level_percent;

        if (max_wear_level_percent > wear_level_downstream_p->max_wear_level_percent)
        {
            fbe_raid_group_trace(raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_RAID_GROUP_DEBUG_FLAG_DRIVE_TIER_TRACING,
                                 "max wear level changed 0x%x -> 0x%x. mec: 0x%llx pec: 0x%llx poh: 0x%llx\n", 
                                 wear_level_downstream_p->max_wear_level_percent, max_wear_level_percent,
                                 wear_level_downstream_p->max_erase_count, wear_level_downstream_p->eol_PE_cycles,
                                 wear_level_downstream_p->power_on_hours);
            wear_level_downstream_p->max_wear_level_percent = max_wear_level_percent;
            wear_level_downstream_p->edge_index = *edge_index_p;
            wear_level_downstream_p->max_erase_count = mirror_ssd_stats_p->max_erase_count;
            wear_level_downstream_p->eol_PE_cycles = mirror_ssd_stats_p->eol_PE_cycles;
            wear_level_downstream_p->power_on_hours = mirror_ssd_stats_p->power_on_hours;

        }
        
    }

    /* remove the sub packet from master queue. */ 
    fbe_transport_remove_subpacket_is_queue_empty(packet_p, &is_empty);

    /* when the queue is empty, all subpackets have completed. */
    if(is_empty) {
        fbe_transport_destroy_subpackets(master_packet_p);
        /* remove the master packet from terminator queue and complete it. */
        fbe_raid_group_trace(raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_RAID_GROUP_DEBUG_FLAG_DRIVE_TIER_TRACING,
                             "wear level subpacket queue is empty\n");

        fbe_memory_free_request_entry(&master_packet_p->memory_request);
        fbe_transport_complete_packet(master_packet_p);
    }
    else
    {
        /* Not done with processing sub packets.
         */
    }

    return FBE_STATUS_MORE_PROCESSING_REQUIRED; 
}

/******************************************************************************
 * end fbe_striper_get_wear_level_subpacket_completion()
 ******************************************************************************/



/******************************
 * end fbe_striper_usurper.c
 ******************************/
