/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_extent_pool_lun_class.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the lun class code .
 * 
 * @ingroup lun_class_files
 * 
 * @version
 *   12/23/2009:  Created. Amit Dhaduk
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe_ext_pool_lun_private.h"
#include "fbe/fbe_lun.h"
#include "fbe_private_space_layout.h"
#include "fbe_transport_memory.h"

/*****************************************
 * LOCAL FUNCTION DEFINITIONS
 *****************************************/
static fbe_status_t fbe_ext_pool_lun_calculate_imported_capacity(fbe_packet_t * packet);
static fbe_status_t fbe_ext_pool_lun_calculate_exported_capacity(fbe_packet_t * packet);
static fbe_status_t fbe_ext_pool_lun_class_prepare_for_power_shutdown(fbe_packet_t * packet);
static fbe_status_t fbe_ext_pool_lun_prefstats_set_enabled(fbe_packet_t * packet);
static fbe_status_t fbe_ext_pool_lun_prefstats_send_packet_to_service(fbe_payload_control_operation_opcode_t control_code,
                                                 fbe_payload_control_buffer_t buffer,
                                                 fbe_payload_control_buffer_length_t buffer_length,
                                                 fbe_service_id_t service_id,
                                                 fbe_sg_element_t *sg_list,
                                                 fbe_u32_t sg_list_count,
                                                 fbe_packet_attr_t attr,
                                                 fbe_package_id_t package_id);
static fbe_status_t fbe_ext_pool_lun_prefstats_forward_packet_to_object(fbe_packet_t *packet,
												   fbe_payload_control_operation_opcode_t control_code,
                                                fbe_payload_control_buffer_t buffer,
                                                fbe_payload_control_buffer_length_t buffer_length,
                                                fbe_object_id_t object_id,
                                                fbe_sg_element_t *sg_list,
                                                fbe_u32_t sg_list_count,
                                                fbe_packet_attr_t attr,
                                                fbe_package_id_t package_id);


static fbe_status_t fbe_ext_pool_lun_prefstats_enumerate_class(fbe_class_id_t class_id,  fbe_package_id_t package_id, fbe_object_id_t ** obj_array_p, fbe_u32_t *num_objects_p);
static fbe_status_t fbe_ext_pool_lun_prefstats_get_total_objects_of_class(fbe_class_id_t class_id, fbe_package_id_t package_id, fbe_u32_t *total_objects_p);
static fbe_status_t fbe_ext_pool_lun_prefstats_enable_peformance_stats(fbe_object_id_t in_lun_object_id, fbe_packet_t *packet);
static fbe_status_t fbe_ext_pool_lun_perfstats_forward_packet_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);

static fbe_status_t fbe_ext_pool_lun_class_calculate_cache_zero_bit_map_size_to_remove(fbe_packet_t *packet_p);

/* Disable performance stats*/
static fbe_status_t fbe_ext_pool_lun_prefstats_set_disabled(fbe_packet_t * packet);
static fbe_status_t fbe_ext_pool_lun_prefstats_disable_peformance_stats(fbe_object_id_t in_lun_object_id, fbe_packet_t *packet);


/*!***************************************************************
 *      fbe_ext_pool_lun_class_control_entry()
 ****************************************************************
 * @brief
 *  This function is the management packet entry point for
 *  operations to the lun class.
 *
 * @param packet - The packet that is arriving.
 *
 * @return fbe_status_t
 *
 ****************************************************************/
fbe_status_t 
fbe_ext_pool_lun_class_control_entry(fbe_packet_t * packet)
{
    fbe_status_t                            status;
    fbe_payload_ex_t *                     sep_payload = NULL;
    fbe_payload_control_operation_t *       control_operation = NULL;
    fbe_payload_control_operation_opcode_t  opcode;

    sep_payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(sep_payload);
    fbe_payload_control_get_opcode(control_operation, &opcode);

    switch (opcode) {
        case FBE_LUN_CONTROL_CODE_CLASS_CALCULATE_IMPORTED_CAPACITY:
            status = fbe_ext_pool_lun_calculate_imported_capacity(packet);
            break;
        case FBE_LUN_CONTROL_CODE_CLASS_CALCULATE_EXPORTED_CAPACITY:
            status = fbe_ext_pool_lun_calculate_exported_capacity(packet);
            break;
        case FBE_LUN_CONTROL_CODE_CLASS_PREPARE_FOR_POWER_SHUTDOWN:
            status = fbe_ext_pool_lun_class_prepare_for_power_shutdown(packet);
            break;
        case  FBE_LUN_CONTROL_CODE_CLASS_PREFSTATS_SET_ENABLED:
            status = fbe_ext_pool_lun_prefstats_set_enabled(packet);
            break;
        case  FBE_LUN_CONTROL_CODE_CLASS_PREFSTATS_SET_DISABLED:
            status = fbe_ext_pool_lun_prefstats_set_disabled(packet);
            break;
        case FBE_LUN_CONTROL_CODE_CLASS_CALCULATE_CACHE_ZERO_BIT_MAP_SIZE_TO_REMOVE:
            status = fbe_ext_pool_lun_class_calculate_cache_zero_bit_map_size_to_remove(packet);
            break;
        default:
            fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
            status = fbe_transport_complete_packet(packet);
            break;
    }
    return status;
}
/* end fbe_ext_pool_lun_class_control_entry() */

/*!***************************************************************
 *      fbe_ext_pool_lun_class_subpacket_completion()
 ****************************************************************
 * @brief
 * Completion function for subpackets created to handle
 * FBE_LUN_CONTROL_CODE_CLASS_PREPARE_FOR_POWER_SHUTDOWN
 * related processing
 * 
 * @param
 * fbe_packet_t, fbe_packet_completion_context_t
 *
 * @return
 * fbe_status_t
 *
 ****************************************************************/
fbe_status_t fbe_ext_pool_lun_class_subpacket_completion(fbe_packet_t * sub_packet_p,
                                        fbe_packet_completion_context_t context)
{
    fbe_packet_t *                          master_packet_p             = NULL;
    fbe_payload_ex_t *                      master_payload_p            = NULL;
    fbe_payload_control_operation_t *       master_control_operation_p  = NULL;
    fbe_payload_ex_t *                      sub_payload_p               = NULL;
    fbe_payload_control_operation_t *       sub_control_operation_p     = NULL;

    fbe_bool_t                              is_empty;
    fbe_queue_head_t                        tmp_queue;

    fbe_topology_class_trace(FBE_CLASS_ID_LUN,
                             FBE_TRACE_LEVEL_INFO,
                             FBE_TRACE_MESSAGE_ID_INFO,
                             "%s Entry packet 0x%p\n", __FUNCTION__, sub_packet_p);

    master_packet_p     = fbe_transport_get_master_packet(sub_packet_p);
    master_payload_p    = fbe_transport_get_payload_ex(master_packet_p);
    master_control_operation_p = fbe_payload_ex_get_control_operation(master_payload_p);

    sub_payload_p       = fbe_transport_get_payload_ex(sub_packet_p);


    /* release the control operation. */
    sub_control_operation_p = fbe_payload_ex_get_control_operation(sub_payload_p);
    fbe_payload_ex_release_control_operation(sub_payload_p, sub_control_operation_p);

    fbe_transport_remove_subpacket_is_queue_empty(sub_packet_p, &is_empty);

    /* when the queue is empty, all subpackets have completed. */
    if(is_empty)
    {
        fbe_topology_class_trace(FBE_CLASS_ID_LUN,
                                 FBE_TRACE_LEVEL_INFO,
                                 FBE_TRACE_MESSAGE_ID_INFO,
                                 "%s All subpackets completed\n", __FUNCTION__);

        fbe_transport_destroy_subpackets(master_packet_p);

        /* release the allocated memory . */
        fbe_memory_free_request_entry(&master_packet_p->memory_request);

        /* Always complete the master packet with a successful status */
        fbe_payload_control_set_status(master_control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_OK);
        fbe_transport_set_status(master_packet_p, FBE_STATUS_OK, 0);

        /* initialize the queue. */
        fbe_queue_init(&tmp_queue);

        /* enqueue this packet temporary queue before we enqueue it to run queue. */
        fbe_queue_push(&tmp_queue, &master_packet_p->queue_element);
        fbe_transport_run_queue_push(&tmp_queue,  NULL,  NULL);
    }

    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}


/*!***************************************************************
 *      fbe_ext_pool_lun_class_prepare_for_power_shutdown_alloc_completion()
 ****************************************************************
 * @brief
 * Completion function for the memory allocation needed to create
 * subpackets to handle
 * FBE_LUN_CONTROL_CODE_CLASS_PREPARE_FOR_POWER_SHUTDOWN.
 * Initializes and sends the subpackets down to individual LUN
 * objects.
 *
 * @param
 * fbe_memory_request_t, fbe_memory_completion_context_t
 *
 * @return
 * void
 *
 ****************************************************************/
void fbe_ext_pool_lun_class_prepare_for_power_shutdown_alloc_completion(fbe_memory_request_t * memory_request_p, 
                                                               fbe_memory_completion_context_t context)
{
    fbe_status_t                        status                  = FBE_STATUS_INVALID;
    fbe_u32_t                           lun_index               = 0;
    fbe_private_space_layout_lun_info_t lun_info;
    fbe_packet_t *                      packet_p                = NULL;
    fbe_packet_t *                      new_packet_p            = NULL;
    fbe_payload_control_operation_t *   new_control_operation_p = NULL;
    fbe_payload_ex_t *                  new_payload_p           = NULL;
    fbe_memory_header_t *               current_memory_header   = NULL;
    fbe_memory_header_t *               next_memory_header      = NULL;
    fbe_queue_head_t                    tmp_queue;
    fbe_queue_element_t *               queue_element           = NULL;

    fbe_topology_class_trace(FBE_CLASS_ID_LUN,
                             FBE_TRACE_LEVEL_INFO,
                             FBE_TRACE_MESSAGE_ID_INFO,
                             "%s Entry \n", __FUNCTION__);

    packet_p = fbe_transport_memory_request_to_packet(memory_request_p);

    /* Check allocation status */
    if (fbe_memory_request_is_allocation_complete(memory_request_p) == FBE_FALSE)
    {
        /* Currently this callback doesn't expect any aborted requests */
        fbe_topology_class_trace(FBE_CLASS_ID_LUN,
                                 FBE_TRACE_LEVEL_ERROR,
                                 FBE_TRACE_MESSAGE_ID_INFO,
                                 "%s mem req 0x%p failed\n",
                                 __FUNCTION__, memory_request_p);

        fbe_transport_set_status(packet_p, FBE_STATUS_INSUFFICIENT_RESOURCES, 0);
        fbe_transport_complete_packet(packet_p);
        return;
    }

    current_memory_header = fbe_memory_get_first_memory_header(memory_request_p);

    fbe_queue_init(&tmp_queue);

    /* Create a subpacket for each system LUN */
    for(lun_index = 0; lun_index < FBE_PRIVATE_SPACE_LAYOUT_MAX_PRIVATE_LUNS; lun_index++)
    {
        status = fbe_private_space_layout_get_lun_by_index(lun_index, &lun_info);
        if(status != FBE_STATUS_OK) {
            break;
        }
        if(lun_info.lun_number == FBE_PRIVATE_SPACE_LAYOUT_LUN_ID_INVALID)
        {
            break;
        }

        fbe_topology_class_trace(FBE_CLASS_ID_LUN,
                                 FBE_TRACE_LEVEL_DEBUG_HIGH,
                                 FBE_TRACE_MESSAGE_ID_INFO,
                                 "%s LUN %d \n", __FUNCTION__, lun_info.lun_number);

        new_packet_p = (fbe_packet_t *)fbe_memory_header_to_data(current_memory_header);
        fbe_memory_get_next_memory_header(current_memory_header, &next_memory_header);
        current_memory_header = next_memory_header;

        fbe_transport_initialize_packet(new_packet_p);

        fbe_transport_set_address(new_packet_p,
                                  FBE_PACKAGE_ID_SEP_0,
                                  FBE_SERVICE_ID_TOPOLOGY,
                                  FBE_CLASS_ID_INVALID,
                                  lun_info.object_id);

        new_payload_p           = fbe_transport_get_payload_ex(new_packet_p);

        new_control_operation_p = fbe_payload_ex_allocate_control_operation(new_payload_p);

        fbe_payload_ex_increment_control_operation_level(new_payload_p);

        fbe_payload_control_build_operation(new_control_operation_p,
                                            FBE_LUN_CONTROL_CODE_PREPARE_FOR_POWER_SHUTDOWN,
                                            NULL,
                                            0);

        fbe_transport_set_completion_function(new_packet_p,
                                              fbe_ext_pool_lun_class_subpacket_completion,
                                              context);

        status = fbe_transport_add_subpacket(packet_p, new_packet_p);

        fbe_queue_push(&tmp_queue, &new_packet_p->queue_element);
    }

    /* Send the subpackets */
    while(queue_element = fbe_queue_pop(&tmp_queue))
    {
        new_packet_p    = fbe_transport_queue_element_to_packet(queue_element);
        status          = fbe_service_manager_send_control_packet(new_packet_p);
    }
}

/*!***************************************************************
 *      fbe_ext_pool_lun_class_prepare_for_power_shutdown()
 ****************************************************************
 * @brief
 * Initial processing of
 * FBE_LUN_CONTROL_CODE_CLASS_PREPARE_FOR_POWER_SHUTDOWN.
 * This function just sends off a memory request, and processing
 * is continued in the completion function for the memory req.
 *
 * @param
 * fbe_packet_t
 *
 * @return
 * fbe_status_t
 *
 ****************************************************************/
static fbe_status_t
fbe_ext_pool_lun_class_prepare_for_power_shutdown(fbe_packet_t * packet_p)
{
    fbe_status_t                    status = FBE_STATUS_INVALID;
    fbe_memory_request_t *          memory_request_p = NULL;
    fbe_payload_ex_t *              payload_p = NULL;

    fbe_topology_class_trace(FBE_CLASS_ID_LUN,
                             FBE_TRACE_LEVEL_INFO,
                             FBE_TRACE_MESSAGE_ID_INFO,
                             "%s Entry \n", __FUNCTION__);

    payload_p = fbe_transport_get_payload_ex(packet_p);

    memory_request_p = fbe_transport_get_memory_request(packet_p);

    fbe_memory_build_chunk_request(memory_request_p,
                                   FBE_MEMORY_CHUNK_SIZE_FOR_PACKET,
                                   FBE_PRIVATE_SPACE_LAYOUT_MAX_PRIVATE_LUNS,
                                   fbe_transport_get_resource_priority(packet_p),
                                   fbe_transport_get_io_stamp(packet_p),
                                   fbe_ext_pool_lun_class_prepare_for_power_shutdown_alloc_completion,
                                   NULL);

    fbe_transport_memory_request_set_io_master(memory_request_p, packet_p);
    
    status = fbe_memory_request_entry(memory_request_p);
    if((status != FBE_STATUS_OK) && (status != FBE_STATUS_PENDING))
    {
        fbe_topology_class_trace(FBE_CLASS_ID_LUN,
                                 FBE_TRACE_LEVEL_ERROR,
                                 FBE_TRACE_MESSAGE_ID_INFO,
                                 "%s alloc failed, status:0x%x, line:%d\n", __FUNCTION__, status, __LINE__);

        fbe_transport_set_status(packet_p, FBE_STATUS_INSUFFICIENT_RESOURCES, 0);
        fbe_transport_complete_packet(packet_p);

        return status;
    }

    return FBE_STATUS_PENDING;
}


/*!****************************************************************************
 *      fbe_ext_pool_lun_calculate_imported_capacity()
 ******************************************************************************
 * @brief
 *  This function calculates imported capacity of lun which is required at the
 *  time of lun creation. Imported capacity takes lun meta data and exported capacity
 *  into account.
 *
 * @param packet - The packet that is arriving.
 *
 * @return fbe_status_t
 * 
 * @version
 *  10/31/2011 - Modified. Vera Wang
 ******************************************************************************/
static fbe_status_t 
fbe_ext_pool_lun_calculate_imported_capacity(fbe_packet_t * packet)
{
    fbe_lun_control_class_calculate_capacity_t *            calculate_capacity_p = NULL;    
    fbe_payload_ex_t *                                      sep_payload = NULL;
    fbe_payload_control_operation_t *                       control_operation = NULL; 
    fbe_u32_t                                               length = 0;  	
    fbe_block_count_t                                       metadata_size_blocks;
    fbe_block_count_t                                       userdata_size_blocks;
    fbe_block_count_t                                       cache_zero_bit_map_size;

	/* get the payload from packet */
    sep_payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(sep_payload);

    /* validation of packet */
    fbe_payload_control_get_buffer(control_operation, &calculate_capacity_p);
    if (calculate_capacity_p == NULL) {
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length (control_operation, &length);
    if(length != sizeof(fbe_lun_control_class_calculate_capacity_t)) {
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* If user exported capacity is invalid then return error. */
    if(calculate_capacity_p->exported_capacity == FBE_LBA_INVALID) 
    {
        fbe_topology_class_trace(FBE_CLASS_ID_LUN,
                                 FBE_TRACE_LEVEL_WARNING,
                                 FBE_TRACE_MESSAGE_ID_INFO,
                                 "%s user exported capacity is invalid.\n", __FUNCTION__);
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_topology_class_trace(FBE_CLASS_ID_LUN,
                             FBE_TRACE_LEVEL_DEBUG_HIGH,
                             FBE_TRACE_MESSAGE_ID_INFO,
                             "%s Exported size blocks = 0x%llX chunks=0x%llX remainder=0x%llX.\n",
                             __FUNCTION__,
                             (unsigned long long)calculate_capacity_p->exported_capacity,
                             (unsigned long long)(calculate_capacity_p->exported_capacity / FBE_LUN_CHUNK_SIZE),
                             (unsigned long long)(calculate_capacity_p->exported_capacity % FBE_LUN_CHUNK_SIZE)
                             );

    userdata_size_blocks = calculate_capacity_p->exported_capacity;

    /*before we start to use the capacity requested by the user, we have to add to it a bit extra capacity
	This is done so cache can store at the end of the LUN the zero bitmaps*/
	fbe_ext_pool_lun_calculate_cache_zero_bit_map_size(calculate_capacity_p->exported_capacity, &cache_zero_bit_map_size);
	userdata_size_blocks = calculate_capacity_p->exported_capacity + cache_zero_bit_map_size;

    fbe_topology_class_trace(FBE_CLASS_ID_LUN,
                             FBE_TRACE_LEVEL_DEBUG_HIGH,
                             FBE_TRACE_MESSAGE_ID_INFO,
                             "%s Userdata size blocks = 0x%llX chunks=0x%llX remainder=0x%llX.\n",
                             __FUNCTION__,
                             (unsigned long long)userdata_size_blocks,
                             (unsigned long long)(userdata_size_blocks / FBE_LUN_CHUNK_SIZE),
                             (unsigned long long)(userdata_size_blocks % FBE_LUN_CHUNK_SIZE)
                             );

    if((userdata_size_blocks % FBE_LUN_CHUNK_SIZE) != 0)
    {
        userdata_size_blocks += (FBE_LUN_CHUNK_SIZE - (userdata_size_blocks % FBE_LUN_CHUNK_SIZE));
    }


    metadata_size_blocks = (((userdata_size_blocks / FBE_LUN_CHUNK_SIZE) * FBE_LUN_METADATA_BITMAP_SIZE) + (FBE_METADATA_BLOCK_DATA_SIZE - 1))
                            / FBE_METADATA_BLOCK_DATA_SIZE;

    metadata_size_blocks += FBE_LUN_DIRTY_FLAGS_SIZE_BLOCKS;

    fbe_topology_class_trace(FBE_CLASS_ID_LUN,
                             FBE_TRACE_LEVEL_DEBUG_HIGH,
                             FBE_TRACE_MESSAGE_ID_INFO,
                             "%s Metadata size blocks = 0x%llX chunks=0x%llX remainder=0x%llX.\n",
                             __FUNCTION__,
                             metadata_size_blocks,
                             metadata_size_blocks / FBE_LUN_CHUNK_SIZE,
                             metadata_size_blocks % FBE_LUN_CHUNK_SIZE
                             );

    if((metadata_size_blocks % FBE_LUN_CHUNK_SIZE) != 0)
    {
        metadata_size_blocks += (FBE_LUN_CHUNK_SIZE - (metadata_size_blocks % FBE_LUN_CHUNK_SIZE));
    }

    calculate_capacity_p->imported_capacity = (metadata_size_blocks + userdata_size_blocks);

    fbe_topology_class_trace(FBE_CLASS_ID_LUN,
                             FBE_TRACE_LEVEL_DEBUG_HIGH,
                             FBE_TRACE_MESSAGE_ID_INFO,
                             "%s Capacity before alignment=0x%llX, align=0x%llX.\n",
                             __FUNCTION__,
                             (unsigned long long)calculate_capacity_p->imported_capacity,
                             (unsigned long long)calculate_capacity_p->lun_align_size
                             );

    /* Round the imported capacity up with align size. */
    if (calculate_capacity_p->imported_capacity % calculate_capacity_p->lun_align_size != 0) 
    {
        if (calculate_capacity_p->imported_capacity > calculate_capacity_p->lun_align_size) 
        {
            calculate_capacity_p->imported_capacity += calculate_capacity_p->lun_align_size - (calculate_capacity_p->imported_capacity % calculate_capacity_p->lun_align_size);  
        } 
        else 
        {
            calculate_capacity_p->imported_capacity = calculate_capacity_p->lun_align_size;
        }   
    } 

    fbe_topology_class_trace(FBE_CLASS_ID_LUN,
                             FBE_TRACE_LEVEL_DEBUG_HIGH,
                             FBE_TRACE_MESSAGE_ID_INFO,
                             "%s Capacity after alignment=0x%llX.\n",
                             __FUNCTION__,
                             (unsigned long long)calculate_capacity_p->imported_capacity
                             );

    /* set the packet status with completion and return it back to source */
    fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;

}
/* end fbe_ext_pool_lun_calculate_imported_capacity() */


/*!****************************************************************************
 *      fbe_ext_pool_lun_calculate_exported_capacity()
 ******************************************************************************
 * @brief
 *  This function calculates exported capacity of lun. 
 *
 *
 * @param packet - The packet that is arriving.
 *
 * @return fbe_status_t
 * 
 * @version
 *  10/31/2011 - Created. Vera Wang
 ******************************************************************************/
static fbe_status_t 
fbe_ext_pool_lun_calculate_exported_capacity(fbe_packet_t * packet)
{
    fbe_lun_control_class_calculate_capacity_t *            calculate_capacity_p = NULL;    
    fbe_payload_ex_t *                                      sep_payload = NULL;
    fbe_payload_control_operation_t *                       control_operation = NULL; 
    fbe_u32_t                                               length = 0;  	
    fbe_u64_t                                               max_chunk_num;
    fbe_u64_t                                               metadata_bytes;
    fbe_u64_t                                               metadata_chunk_num;
    fbe_block_count_t                                       cache_zero_bit_map_size;

    /* get the payload from packet */
    sep_payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(sep_payload);  

    /* validation of packet */
    fbe_payload_control_get_buffer(control_operation, &calculate_capacity_p);
    if (calculate_capacity_p == NULL) {
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length (control_operation, &length);
    if(length != sizeof(fbe_lun_control_class_calculate_capacity_t)) {
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Check if imported capacity is invalid or less than lun_align_size, then return error. */
    if((calculate_capacity_p->imported_capacity == FBE_LBA_INVALID) || 
       (calculate_capacity_p->imported_capacity < calculate_capacity_p->lun_align_size)) 
    {
        fbe_topology_class_trace(FBE_CLASS_ID_LUN,
                                 FBE_TRACE_LEVEL_WARNING,
                                 FBE_TRACE_MESSAGE_ID_INFO,
                                 "%s user imported capacity is invalid.\n", __FUNCTION__);
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Round the imported capacity down with align size. */
    if (calculate_capacity_p->imported_capacity % calculate_capacity_p->lun_align_size != 0) 
    {
        calculate_capacity_p->imported_capacity -= calculate_capacity_p->imported_capacity % calculate_capacity_p->lun_align_size;     
    }

    /* Calculate the export capacity considering metadata*/
    max_chunk_num = calculate_capacity_p->imported_capacity / FBE_LUN_CHUNK_SIZE;
    metadata_bytes = (max_chunk_num * FBE_LUN_METADATA_BITMAP_SIZE); //(2 bytes/per chunk)
    metadata_bytes += FBE_LUN_DIRTY_FLAGS_SIZE_BLOCKS * FBE_METADATA_BLOCK_DATA_SIZE;

    if (metadata_bytes % (FBE_LUN_CHUNK_SIZE * FBE_METADATA_BLOCK_DATA_SIZE) != 0) 
    {
        metadata_chunk_num = (metadata_bytes / (FBE_LUN_CHUNK_SIZE * FBE_METADATA_BLOCK_DATA_SIZE)) + 1;
    }
    else 
    {
        metadata_chunk_num = metadata_bytes / (FBE_LUN_CHUNK_SIZE * FBE_METADATA_BLOCK_DATA_SIZE);
    }

    if(metadata_chunk_num == 0) {
        EmcpalDebugBreakPoint();
    }

    /* The cache needs a small amount of space to store bitmaps, which comes out of the
       exported capacity visible to the user.
     */
	fbe_ext_pool_lun_calculate_cache_zero_bit_map_size(
        ((max_chunk_num - metadata_chunk_num) * FBE_LUN_CHUNK_SIZE), 
        &cache_zero_bit_map_size);

    calculate_capacity_p->exported_capacity = (max_chunk_num * FBE_LUN_CHUNK_SIZE) - (cache_zero_bit_map_size +
                                                (metadata_chunk_num * FBE_LUN_CHUNK_SIZE));
    fbe_topology_class_trace(FBE_CLASS_ID_LUN,
                             FBE_TRACE_LEVEL_DEBUG_HIGH,
                             FBE_TRACE_MESSAGE_ID_INFO,
                             "%s Imported Capacity=0x%llX.\n",
                             __FUNCTION__,
                             calculate_capacity_p->imported_capacity
                             );

    fbe_topology_class_trace(FBE_CLASS_ID_LUN,
                             FBE_TRACE_LEVEL_DEBUG_HIGH,
                             FBE_TRACE_MESSAGE_ID_INFO,
                             "%s Exported Capacity=0x%llX.\n",
                             __FUNCTION__,
                             calculate_capacity_p->exported_capacity
                             );

    fbe_topology_class_trace(FBE_CLASS_ID_LUN,
                             FBE_TRACE_LEVEL_DEBUG_HIGH,
                             FBE_TRACE_MESSAGE_ID_INFO,
                             "%s Metadata Capacity=0x%llX.\n",
                             __FUNCTION__,
                             (metadata_chunk_num * FBE_LUN_CHUNK_SIZE)
                             );

    fbe_topology_class_trace(FBE_CLASS_ID_LUN,
                             FBE_TRACE_LEVEL_DEBUG_HIGH,
                             FBE_TRACE_MESSAGE_ID_INFO,
                             "%s Cache Bitmap Capacity=0x%llX.\n",
                             __FUNCTION__,
                             cache_zero_bit_map_size
                             );

    /* set the packet status with completion and return it back to source */
    fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;

}
/* end fbe_ext_pool_lun_calculate_exported_capacity() */


/*!****************************************************************************
 * fbe_ext_pool_lun_class_set_resource_priority()
 ******************************************************************************
 * @brief
 * This function sets memory allocation priority for the packet.
 *
 * @param packet - The packet that is arriving.
 *
 * @return fbe_status_t
 *
 * @author
 *  6/20/2011 - Created. Vamsi V
 *
 ******************************************************************************/
fbe_status_t  
fbe_ext_pool_lun_class_set_resource_priority(fbe_packet_t * packet_p)
{
    
    if((packet_p->resource_priority_boost & FBE_TRANSPORT_RESOURCE_PRIORITY_BOOST_FLAG_LUN) == 0)
    {
        /* If flag is not set, set it to true */
        packet_p->resource_priority_boost |= FBE_TRANSPORT_RESOURCE_PRIORITY_BOOST_FLAG_LUN;
    }
    else
    {
        /* Packet is being resent/reused without resetting the resource priroity. */ 
        fbe_base_config_class_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                       "lun: resource priority is not reset for packet 0x%p \n",
                                       packet_p);
    }

    if(packet_p->resource_priority >= FBE_MEMORY_DPS_LUN_BASE_PRIORITY)
    {

        /* Generally in IO path packets flow down the stack from LUN to RG to VD to PVD.
         * In this case, resource priority will be less than Objects base priority and so
         * we bump it up. 
         * But in some cases (such as monitor, control, event operations, etc.) packets do not 
         * always move down the stack. In these cases packets's resource priority will already be greater
         * than Object's base priority. But since we should never reduce priority, we dont
         * update it.    
         */
    }
    else
    {
        packet_p->resource_priority = FBE_MEMORY_DPS_LUN_BASE_PRIORITY;
    }
    return FBE_STATUS_OK;
}

/*!****************************************************************************
 * fbe_ext_pool_lun_calculate_cache_zero_bit_map_size()
 ******************************************************************************
 * @brief
 * This function calculates and returns the number of blocks we'll need to
 * add to the end of the lun in order to reserve space for SP cache to persist
 * it's zero bitmap so it can scale with the luns
 *
 * @param lun_capacity - The size of the lun in blocks
 * @param blocks - how many blocks to add to the end
 *
 * @return fbe_status_t
 *
 ******************************************************************************/
fbe_status_t fbe_ext_pool_lun_calculate_cache_zero_bit_map_size(fbe_block_count_t lun_capacity, fbe_block_count_t *blocks)
{
	/*there is always a 64KB for header so this is 128 block. After that we need another 128 blocks for each 512GB*/
	fbe_block_count_t	size_needed = MCC_ZERO_BITMAP_HEADER_SIZE;
	fbe_u64_t			number_of_512_gb_chunks = (fbe_u64_t)(lun_capacity * (fbe_u64_t)FBE_BYTES_PER_BLOCK) / (fbe_u64_t)(MCC_ZERO_BITMAP_GB_PER_CHUNK * (fbe_u64_t)FBE_GIGABYTE);
	fbe_u64_t			number_of_512_gb_chunks_remainder = (fbe_u64_t)(lun_capacity * (fbe_u64_t)FBE_BYTES_PER_BLOCK) % (fbe_u64_t)(MCC_ZERO_BITMAP_GB_PER_CHUNK * (fbe_u64_t)FBE_GIGABYTE);

	if (number_of_512_gb_chunks_remainder != 0) {
		number_of_512_gb_chunks += 1;/*we need to round up*/
	}

	size_needed += (number_of_512_gb_chunks * MCC_ZERO_BITMAP_CHUNKS_SIZE);

	*blocks = size_needed;

	return FBE_STATUS_OK;
}


/*!****************************************************************************
 * fbe_ext_pool_lun_calculate_cache_zero_bit_map_size_to_remove()
 ******************************************************************************
 * @brief
 * This function is the revers of fbe_ext_pool_lun_calculate_cache_zero_bit_map_size
 * it finds out how many blocks to remove from a lun that already had the cache zero bitmap added to it.
 * it should be used for reporting the user capacity of the lun
 *
 * @param lun_capacity - The size of the lun in blocks, this will already include the cache bitmap size we created the lun with
 * @param blocks - how many blocks to remove
 *
 * @return fbe_status_t
 *
 ******************************************************************************/
fbe_status_t fbe_ext_pool_lun_calculate_cache_zero_bit_map_size_to_remove(fbe_block_count_t lun_capacity, fbe_block_count_t *blocks)
{
	/*the problem is that the size of the lun right now already includes the cache bitmap so we will end up
	rounding it up and we'll report the user data as having 128 blocks less.
	to resolve that we'll just divide the lun to 512G chunks first and see how much we have.
	Then, based on that, we'll reduce the cache bitmap size that represents these chunks*/

	/*there is always a 64KB for header so this is 128 block. After that we need another 128 blocks for each 512GB*/
	fbe_block_count_t	size_needed = MCC_ZERO_BITMAP_HEADER_SIZE;
	fbe_u64_t			number_of_512_gb_chunks = (fbe_u64_t)(lun_capacity * (fbe_u64_t)FBE_BYTES_PER_BLOCK) / (fbe_u64_t)(MCC_ZERO_BITMAP_GB_PER_CHUNK * (fbe_u64_t)FBE_GIGABYTE);
	fbe_u64_t			number_of_512_gb_chunks_remainder = (fbe_u64_t)(lun_capacity * (fbe_u64_t)FBE_BYTES_PER_BLOCK) % (fbe_u64_t)(MCC_ZERO_BITMAP_GB_PER_CHUNK * (fbe_u64_t)FBE_GIGABYTE);

	/*if someone bound a 1TB+1 byte lun for example we need to round up but to make sure this is the case we have to delete the extra capacity which is the blocks
	we calculated in fbe_ext_pool_lun_calculate_cache_zero_bit_map_size*/
	if ((number_of_512_gb_chunks_remainder != 0) &&
		 ((number_of_512_gb_chunks_remainder / FBE_BYTES_PER_BLOCK) - (number_of_512_gb_chunks * MCC_ZERO_BITMAP_CHUNKS_SIZE) - MCC_ZERO_BITMAP_HEADER_SIZE)!= 0) {
		number_of_512_gb_chunks += 1;/*we need to round up*/
	}

	size_needed += (number_of_512_gb_chunks * MCC_ZERO_BITMAP_CHUNKS_SIZE);

	*blocks = size_needed;

	return FBE_STATUS_OK;
}

/******************************************************************************
 * end fbe_ext_pool_lun_class_set_resource_priority()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_ext_pool_lun_class_calculate_cache_zero_bit_map_size_to_remove()
 ******************************************************************************
 * @brief
 * This function finds out how many blocks to remove from a lun that already had
 *  the cache zero bitmap added to it. 
 *  It should be used for reporting the user capacity of the lun.
 *
 * @param packet_p - The packet that is arriving.
 *
 * @return fbe_status_t
 *
 * @version
 *  06/03/2013 - Created. Sandeep Chaudhari
 ******************************************************************************/
fbe_status_t fbe_ext_pool_lun_class_calculate_cache_zero_bit_map_size_to_remove(fbe_packet_t *packet_p)
{
    fbe_lun_control_class_calculate_cache_zero_bitmap_blocks_t *        calculate_block_count_p = NULL;
    fbe_payload_ex_t *                                                  sep_payload = NULL;
    fbe_payload_control_operation_t *                                   control_operation = NULL;
    fbe_u32_t                                                           length = 0;
    fbe_block_count_t                                                   lun_capacity = 0;
    fbe_block_count_t                                                   blocks = 0;
    fbe_status_t                                                        status = FBE_STATUS_GENERIC_FAILURE;
    
    /* Get the payload from packet */
    sep_payload = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(sep_payload);

    /* Validation of packet*/
    fbe_payload_control_get_buffer(control_operation, &calculate_block_count_p);
    if(NULL == calculate_block_count_p)
    {
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &length);
    if(length != sizeof(fbe_lun_control_class_calculate_cache_zero_bitmap_blocks_t))
    {
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Check if lun capacity is invalid then return error */
    if(calculate_block_count_p->lun_capacity_in_blocks == FBE_LBA_INVALID)
    {
        fbe_topology_class_trace(FBE_CLASS_ID_LUN,
                                    FBE_TRACE_LEVEL_WARNING,
                                    FBE_TRACE_MESSAGE_ID_INFO,
                                    "%s lun capacity is invalid. \n", __FUNCTION__);
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    lun_capacity = calculate_block_count_p->lun_capacity_in_blocks;
    
    status = fbe_ext_pool_lun_calculate_cache_zero_bit_map_size_to_remove(lun_capacity, &blocks);

    if(FBE_STATUS_OK != status)
    {
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    calculate_block_count_p->cache_zero_bitmap_block_count = blocks;

    /* Set the packet status with completion and return it back to source*/
    fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);
    return FBE_STATUS_OK;
}

/******************************************************************************
 * end fbe_ext_pool_lun_class_calculate_cache_zero_bit_map_size_to_remove()
 ******************************************************************************/


static fbe_status_t fbe_ext_pool_lun_prefstats_set_enabled(fbe_packet_t *packet)
{
    
    fbe_status_t                            status = FBE_STATUS_GENERIC_FAILURE;
    fbe_u32_t                                   obj_i = 0;
    fbe_object_id_t                             *obj_list;
    fbe_u32_t                                   obj_count = 0;
    fbe_payload_ex_t *                                      sep_payload = NULL;
    fbe_payload_control_operation_t *                       control_operation = NULL; 

      /* get the payload from packet */
    sep_payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(sep_payload);  

    
    status = fbe_ext_pool_lun_prefstats_enumerate_class(FBE_CLASS_ID_LUN, FBE_PACKAGE_ID_SEP_0, &obj_list, &obj_count);
    for(obj_i = 0; obj_i < obj_count; obj_i++) {
            status = fbe_ext_pool_lun_prefstats_enable_peformance_stats(*(obj_list+obj_i), packet);
    }

    /* releasing obj_list. It was allocated in fbe_ext_pool_lun_perfstats_enumerate_class and used in this function*/
    fbe_release_nonpaged_pool_with_tag(obj_list, 'tats');
    
    /* set the packet status with completion and return it back to source */
    fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;   
}

static fbe_status_t fbe_ext_pool_lun_prefstats_get_total_objects_of_class(fbe_class_id_t class_id, fbe_package_id_t package_id, fbe_u32_t *total_objects_p)
{

   fbe_status_t                        status = FBE_STATUS_GENERIC_FAILURE;
   fbe_topology_control_get_total_objects_of_class_t total_objects = {0};
    total_objects.total_objects = 0;
    total_objects.class_id = class_id;

    status = fbe_ext_pool_lun_prefstats_send_packet_to_service(FBE_TOPOLOGY_CONTROL_CODE_GET_TOTAL_OBJECTS_OF_CLASS,
                                                 &total_objects,
                                                 sizeof(fbe_topology_control_get_total_objects_of_class_t),
                                                 FBE_SERVICE_ID_TOPOLOGY,
                                                 NULL,  /* no sg list*/
                                                 0,  /* no sg list*/
                                                 FBE_PACKET_FLAG_NO_ATTRIB,  /* no sg list*/
                                                 package_id
                                                 );

    if (status != FBE_STATUS_OK) {
        fbe_topology_class_trace(FBE_CLASS_ID_LUN,
                                 FBE_TRACE_LEVEL_ERROR,
                                 FBE_TRACE_MESSAGE_ID_INFO,
                                 "%s:packet error:%d\n",
                                 __FUNCTION__,status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    *total_objects_p = total_objects.total_objects;

    return status;

}

static fbe_status_t fbe_ext_pool_lun_prefstats_enumerate_class(fbe_class_id_t class_id,  fbe_package_id_t package_id, fbe_object_id_t ** obj_array_p, fbe_u32_t *num_objects_p)
{
   fbe_status_t                        status = FBE_STATUS_GENERIC_FAILURE;
   fbe_u32_t                               alloc_size = {0};
   fbe_topology_control_enumerate_class_t  enumerate_class = {0};/*structure is small enough to be on the stack*/

   /* one for the entry and one for the NULL termination.
     */
    fbe_sg_element_t *                      sg_list = NULL;
    fbe_sg_element_t *                      temp_sg_list = NULL;

    /* Get total objects of class*/
    status = fbe_ext_pool_lun_prefstats_get_total_objects_of_class(class_id, package_id, num_objects_p);

    if ( *num_objects_p == 0 ){
        *obj_array_p  = NULL;
        return status;
    }
    
    sg_list = fbe_allocate_nonpaged_pool_with_tag(sizeof(fbe_sg_element_t) * FBE_TOPLOGY_CONTROL_ENUMERATE_CLASS_SG_COUNT, 'tats' );
    if (sg_list == NULL) {
        fbe_topology_class_trace(FBE_CLASS_ID_LUN,
                                 FBE_TRACE_LEVEL_ERROR,
                                 FBE_TRACE_MESSAGE_ID_INFO,
                                 "%s out of memory\n",
                                 __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    temp_sg_list = sg_list;

     /* Allocate memory for the objects */
    alloc_size = sizeof (**obj_array_p) * (*num_objects_p);
    *obj_array_p = fbe_allocate_nonpaged_pool_with_tag(alloc_size, 'tats');
    if (*obj_array_p == NULL) {
        fbe_topology_class_trace(FBE_CLASS_ID_LUN,
                                 FBE_TRACE_LEVEL_ERROR,
                                 FBE_TRACE_MESSAGE_ID_INFO,
                                 "%s out of memory\n",
                                 __FUNCTION__);
        fbe_release_nonpaged_pool_with_tag(sg_list, 'stat');
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*set up the sg list to point to the list of objects the user wants to get*/
    temp_sg_list->address = (fbe_u8_t *)*obj_array_p;
    temp_sg_list->count = alloc_size;
    temp_sg_list ++;
    temp_sg_list->address = NULL;
    temp_sg_list->count = 0;

    enumerate_class.number_of_objects = *num_objects_p;
    enumerate_class.class_id = class_id;
    enumerate_class.number_of_objects_copied = 0;

    status = fbe_ext_pool_lun_prefstats_send_packet_to_service(FBE_TOPOLOGY_CONTROL_CODE_ENUMERATE_CLASS,
                                                                        &enumerate_class,
                                                                        sizeof (fbe_topology_control_enumerate_class_t),
                                                                        FBE_SERVICE_ID_TOPOLOGY,
                                                                        sg_list,
                                                                        FBE_TOPLOGY_CONTROL_ENUMERATE_CLASS_SG_COUNT, /* sg entries*/
                                                                        FBE_PACKET_FLAG_NO_CONTIGUOS_ALLOCATION_NEEDED,
                                                                        package_id);
    if (status != FBE_STATUS_OK) {
        fbe_topology_class_trace(FBE_CLASS_ID_LUN,
                                 FBE_TRACE_LEVEL_ERROR,
                                 FBE_TRACE_MESSAGE_ID_INFO,
                                 "%s:packet error:%d\n",
                                 __FUNCTION__,status);
        fbe_release_nonpaged_pool_with_tag(sg_list, 'tats');
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if ( enumerate_class.number_of_objects_copied < *num_objects_p )
    {
        *num_objects_p = enumerate_class.number_of_objects_copied;
    }

    fbe_release_nonpaged_pool_with_tag(sg_list, 'tats');

    return FBE_STATUS_OK;
}

static fbe_status_t fbe_ext_pool_lun_prefstats_enable_peformance_stats(fbe_object_id_t in_lun_object_id, fbe_packet_t *packet)
{
    fbe_status_t                                status;

   status = fbe_ext_pool_lun_prefstats_forward_packet_to_object (packet, FBE_LUN_CONTROL_CODE_ENABLE_PERFORMANCE_STATS,
                                                         NULL,
                                                         0,
                                                         in_lun_object_id,
                                                         NULL,
                                                         0,
                                                         FBE_PACKET_FLAG_NO_ATTRIB,
                                                         FBE_PACKAGE_ID_SEP_0);

   if (status != FBE_STATUS_OK) 
    {
      fbe_topology_class_trace(FBE_CLASS_ID_LUN,
                                 FBE_TRACE_LEVEL_ERROR,
                                 FBE_TRACE_MESSAGE_ID_INFO,
                                 "%s:packet error:%d\n",
                                 __FUNCTION__,status);
      return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;

}
   
static fbe_status_t fbe_ext_pool_lun_prefstats_send_packet_to_service(fbe_payload_control_operation_opcode_t control_code,
                                                 fbe_payload_control_buffer_t buffer,
                                                 fbe_payload_control_buffer_length_t buffer_length,
                                                 fbe_service_id_t service_id,
                                                 fbe_sg_element_t *sg_list,
                                                 fbe_u32_t sg_list_count,
                                                 fbe_packet_attr_t attr,
                                                 fbe_package_id_t package_id)
{
    fbe_status_t                        status = FBE_STATUS_GENERIC_FAILURE;
    fbe_packet_t *                      packet = NULL;
    fbe_payload_ex_t *                  payload = NULL;
    fbe_payload_control_operation_t *   control_operation = NULL;
    fbe_payload_control_status_t        control_status;
    fbe_payload_control_status_qualifier_t status_qualifier;
	fbe_cpu_id_t cpu_id;

    /* Allocate packet.*/
    packet = fbe_transport_allocate_packet();
    if (packet == NULL) {
        fbe_topology_class_trace (FBE_CLASS_ID_LUN,
                                  FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                        "%s: unable to allocate packet\n", 
                        __FUNCTION__); 
        return FBE_STATUS_GENERIC_FAILURE;
    }
 
    fbe_transport_initialize_packet(packet);
    payload = fbe_transport_get_payload_ex (packet);
	fbe_payload_ex_allocate_memory_operation(payload);

    control_operation = fbe_payload_ex_allocate_control_operation(payload);
    if (sg_list != NULL) {    
		fbe_payload_ex_set_sg_list (payload, sg_list, sg_list_count);
    }

   	cpu_id = fbe_get_cpu_id();
	fbe_transport_set_cpu_id(packet, cpu_id);

    fbe_payload_control_build_operation (control_operation,
                                         control_code,
                                         buffer,
                                         buffer_length);
    /* Set packet address.*/
    fbe_transport_set_address(packet,
                              package_id,
                              service_id,
                              FBE_CLASS_ID_INVALID,
                              FBE_OBJECT_ID_INVALID); 
    
    /* Mark packet with the attribute, either traverse or not */
    fbe_transport_set_packet_attr(packet, attr);

    fbe_transport_set_sync_completion_type(packet, FBE_TRANSPORT_COMPLETION_TYPE_MORE_PROCESSING_REQUIRED);

    status = fbe_service_manager_send_control_packet(packet);
    if (status != FBE_STATUS_OK) {
        fbe_topology_class_trace (FBE_CLASS_ID_LUN,
                                  FBE_TRACE_LEVEL_WARNING, 
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: error in sending 0x%x to srv 0x%x, status:0x%x\n",
                       __FUNCTION__, control_code, service_id, status);  
    }

    fbe_transport_wait_completion(packet);

    /* Save the status for returning.*/
    status = fbe_transport_get_status_code(packet);
    if (status != FBE_STATUS_OK ) {
        fbe_topology_class_trace (FBE_CLASS_ID_LUN,
                                  FBE_TRACE_LEVEL_WARNING,
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: 0x%x to srv 0x%x, return status:0x%x\n",
                       __FUNCTION__, control_code, service_id, status);  
    }
    else
    {
        fbe_payload_control_get_status(control_operation, &control_status);
        if (control_status != FBE_PAYLOAD_CONTROL_STATUS_OK ) {
            fbe_topology_class_trace (FBE_CLASS_ID_LUN,
                                      FBE_TRACE_LEVEL_WARNING,
                           FBE_TRACE_MESSAGE_ID_INFO,
                           "%s: 0x%x to srv 0x%x, return payload status:0x%x\n",
                           __FUNCTION__, control_code, service_id, control_status);
    
            switch (control_status){
            case FBE_PAYLOAD_CONTROL_STATUS_BUSY:
                status = FBE_STATUS_BUSY;
                break;
            case FBE_PAYLOAD_CONTROL_STATUS_INSUFFICIENT_RESOURCES:
                status = FBE_STATUS_INSUFFICIENT_RESOURCES;
                break;
            case FBE_PAYLOAD_CONTROL_STATUS_FAILURE:
                fbe_payload_control_get_status_qualifier(control_operation, &status_qualifier);
                status = (status_qualifier == FBE_OBJECT_ID_INVALID) ? FBE_STATUS_NO_OBJECT: FBE_STATUS_GENERIC_FAILURE;
                break;
            default:
                status = FBE_STATUS_GENERIC_FAILURE;
                break;
            }
            fbe_topology_class_trace (FBE_CLASS_ID_LUN,
                                      FBE_TRACE_LEVEL_INFO,
                           FBE_TRACE_MESSAGE_ID_INFO,
                           "%s: Set return status to 0x%x\n",
                           __FUNCTION__, status);  
        }
    }

    /* free the memory*/
    fbe_payload_ex_release_control_operation(payload, control_operation);
	fbe_payload_ex_release_memory_operation(payload, payload->payload_memory_operation);
    fbe_transport_release_packet(packet);

    return status;
}   

static fbe_status_t fbe_ext_pool_lun_prefstats_forward_packet_to_object(fbe_packet_t *packet,
												   fbe_payload_control_operation_opcode_t control_code,
                                                fbe_payload_control_buffer_t buffer,
                                                fbe_payload_control_buffer_length_t buffer_length,
                                                fbe_object_id_t object_id,
                                                fbe_sg_element_t *sg_list,
                                                fbe_u32_t sg_list_count,
                                                fbe_packet_attr_t attr,
                                                fbe_package_id_t package_id)
{
    fbe_status_t                        status = FBE_STATUS_GENERIC_FAILURE, send_status;
    fbe_payload_ex_t *                  payload = NULL;
    fbe_payload_control_operation_t *   control_operation = NULL;
    fbe_payload_control_status_t        control_status;
	fbe_cpu_id_t  						cpu_id;		
	fbe_semaphore_t						sem;/* we use our own and our own completion function in case the packet is already using it's own semaphore*/
	fbe_packet_attr_t					original_attr;/*preserve the sender attribute*/

	fbe_semaphore_init(&sem, 0, 1);
    
    cpu_id = fbe_get_cpu_id();
	fbe_transport_set_cpu_id(packet, cpu_id);


	payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_allocate_control_operation(payload); 

    fbe_payload_control_build_operation (control_operation,
                                         control_code,
                                         buffer,
                                         buffer_length);
    /* Set packet address.*/
    fbe_transport_set_address(packet,
                              package_id,
                              FBE_SERVICE_ID_TOPOLOGY,
                              FBE_CLASS_ID_INVALID,
                              object_id); 
    
    /* Mark packet with the attribute, either traverse or not */
	fbe_transport_get_packet_attr(packet, &original_attr);
    fbe_transport_clear_packet_attr(packet, original_attr);/*remove the old one, we'll restore it later*/
	fbe_transport_set_packet_attr(packet, attr);

	fbe_transport_set_completion_function(packet, fbe_ext_pool_lun_perfstats_forward_packet_completion, (fbe_packet_completion_context_t) &sem);

    send_status = fbe_service_manager_send_control_packet(packet);
    if ((send_status != FBE_STATUS_OK)&&(send_status != FBE_STATUS_NO_OBJECT) && (send_status != FBE_STATUS_PENDING) && (send_status != FBE_STATUS_MORE_PROCESSING_REQUIRED)) {
        fbe_topology_class_trace(FBE_CLASS_ID_LUN,
                                 FBE_TRACE_LEVEL_WARNING, 
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: error in sending 0x%x to obj 0x%x, status:0x%x\n",
                       __FUNCTION__, control_code, object_id, send_status);  
    }

    fbe_semaphore_wait(&sem, NULL);

    /* Save the status for returning.*/
    status = fbe_transport_get_status_code(packet);
    fbe_topology_class_trace(FBE_CLASS_ID_LUN,
                             FBE_TRACE_LEVEL_DEBUG_HIGH,
                   FBE_TRACE_MESSAGE_ID_INFO,
                   "%s: 0x%x to obj 0x%x, packet status:0x%x\n",
                   __FUNCTION__, control_code, object_id, status);  

    fbe_payload_control_get_status(control_operation, &control_status);
    if (control_status != FBE_PAYLOAD_CONTROL_STATUS_OK ) {
        fbe_topology_class_trace(FBE_CLASS_ID_LUN,
                                 FBE_TRACE_LEVEL_DEBUG_HIGH,
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: 0x%x to obj 0x%x, return payload status:0x%x\n",
                       __FUNCTION__, control_code, object_id, control_status);

        switch (control_status){
        case FBE_PAYLOAD_CONTROL_STATUS_BUSY:
            status = FBE_STATUS_BUSY;
            break;
        case FBE_PAYLOAD_CONTROL_STATUS_INSUFFICIENT_RESOURCES:
            status = FBE_STATUS_INSUFFICIENT_RESOURCES;
            break;
        case FBE_PAYLOAD_CONTROL_STATUS_FAILURE:
        default:
            status = FBE_STATUS_GENERIC_FAILURE;
            break;
        }
        fbe_topology_class_trace(FBE_CLASS_ID_LUN,
                                 FBE_TRACE_LEVEL_INFO,
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: Set return status to 0x%x\n",
                       __FUNCTION__, status);  
    }

    fbe_transport_set_packet_attr(packet, original_attr);
	fbe_semaphore_destroy(&sem);
    fbe_payload_ex_release_control_operation(payload, control_operation);
    return status;
}

static fbe_status_t fbe_ext_pool_lun_perfstats_forward_packet_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
	fbe_semaphore_t * sem = (fbe_semaphore_t *)context;
	fbe_semaphore_release(sem, 0, 1, FBE_FALSE);
	return FBE_STATUS_MORE_PROCESSING_REQUIRED;/*important, otherwise we'll complete the original one*/
}


static fbe_status_t fbe_ext_pool_lun_prefstats_set_disabled(fbe_packet_t *packet)
{
    
    fbe_status_t                            status = FBE_STATUS_GENERIC_FAILURE;
    fbe_u32_t                                   obj_i = 0;
    fbe_object_id_t                             *obj_list;
    fbe_u32_t                                   obj_count = 0;
    fbe_payload_ex_t *                                      sep_payload = NULL;
    fbe_payload_control_operation_t *                       control_operation = NULL; 

      /* get the payload from packet */
    sep_payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(sep_payload);  

    
    status = fbe_ext_pool_lun_prefstats_enumerate_class(FBE_CLASS_ID_LUN, FBE_PACKAGE_ID_SEP_0, &obj_list, &obj_count);
    for(obj_i = 0; obj_i < obj_count; obj_i++) {
            status = fbe_ext_pool_lun_prefstats_disable_peformance_stats(*(obj_list+obj_i), packet);
    }

    /* releasing obj_list. It was allocated in fbe_ext_pool_lun_perfstats_enumerate_class and used in this function*/
    fbe_release_nonpaged_pool_with_tag(obj_list, 'tats');
    
    /* set the packet status with completion and return it back to source */
    fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;   
}

static fbe_status_t fbe_ext_pool_lun_prefstats_disable_peformance_stats(fbe_object_id_t in_lun_object_id, fbe_packet_t *packet)
{
    fbe_status_t                                status;

   status = fbe_ext_pool_lun_prefstats_forward_packet_to_object (packet, FBE_LUN_CONTROL_CODE_DISABLE_PERFORMANCE_STATS,
                                                         NULL,
                                                         0,
                                                         in_lun_object_id,
                                                         NULL,
                                                         0,
                                                         FBE_PACKET_FLAG_NO_ATTRIB,
                                                         FBE_PACKAGE_ID_SEP_0);

   if (status != FBE_STATUS_OK) 
    {
      fbe_topology_class_trace(FBE_CLASS_ID_LUN,
                                 FBE_TRACE_LEVEL_ERROR,
                                 FBE_TRACE_MESSAGE_ID_INFO,
                                 "%s:packet error:%d\n",
                                 __FUNCTION__,status);
      return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;

}
