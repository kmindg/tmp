/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_extent_pool_usurper.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the extent_pool object code for the usurper.
 * 
 * @ingroup extent_pool_class_files
 * 
 * @version
 *   06/06/2014:  Created.
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_object.h"
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_transport.h"
#include "fbe_extent_pool_private.h"
#include "fbe_transport_memory.h"
#include "fbe_service_manager.h"
#include "fbe_spare.h"
#include "fbe/fbe_physical_drive.h"
#include "fbe_database.h"
#include "fbe_raid_group_object.h"
#include "fbe_raid_library.h"
#include "fbe_cmi.h"
#include "fbe_private_space_layout.h"
#include "fbe_notification.h"


/*****************************************
 * LOCAL FUNCTION DEFINITIONS
 *****************************************/
static fbe_status_t
fbe_extent_pool_handle_attach_edge_completion(fbe_packet_t *packet_p, fbe_packet_completion_context_t context);
static fbe_status_t fbe_extent_pool_set_configuration(fbe_extent_pool_t *extent_pool_p, fbe_packet_t * packet_p);

static fbe_status_t fbe_extent_pool_handle_attach_edge(fbe_extent_pool_t * extent_pool_p,   
                                                       fbe_packet_t * packet_p);
static fbe_status_t fbe_extent_pool_usurper_get_control_buffer(fbe_extent_pool_t *extent_pool_p, 
                                           fbe_packet_t * packet_p,
                                           fbe_payload_control_buffer_length_t in_buffer_length,
                                           fbe_payload_control_buffer_t*       out_buffer_pp);

#ifndef NO_EXT_POOL_ALIAS
static fbe_status_t fbe_extent_pool_get_raid_group_info(fbe_extent_pool_t *extent_pool_p, fbe_packet_t * packet_p);
#endif
static fbe_status_t fbe_extent_pool_get_power_saving_parameters(fbe_extent_pool_t* extent_pool_p, fbe_packet_t *packet_p);

/*!***************************************************************
 * fbe_extent_pool_control_entry()
 ****************************************************************
 * @brief
 *  This function is the management packet entry point for
 *  operations to the extent_pool object.
 *
 * @param object_handle - The config handle.
 * @param packet_p - The packet that is arriving.
 *
 * @return fbe_status_t
 *
 ****************************************************************/
fbe_status_t 
fbe_extent_pool_control_entry(fbe_object_handle_t object_handle, fbe_packet_t * packet_p)
{
    fbe_status_t                            status = FBE_STATUS_GENERIC_FAILURE;
    fbe_extent_pool_t                      *extent_pool = NULL;
    fbe_payload_ex_t                       *payload_p = NULL;
    fbe_payload_control_operation_t        *control_p = NULL;
    fbe_payload_control_operation_opcode_t  opcode;

    extent_pool = (fbe_extent_pool_t *)fbe_base_handle_to_pointer(object_handle);
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_p = fbe_payload_ex_get_control_operation(payload_p);
    fbe_payload_control_get_opcode(control_p, &opcode);

    /* If object handle is NULL then use the class control
     * entry.
     */
    if (object_handle == NULL) {
        return fbe_extent_pool_class_control_entry(packet_p);
    }

    switch (opcode)
    {
#if 0
        case FBE_BLOCK_TRANSPORT_CONTROL_CODE_ATTACH_EDGE:
            status = fbe_extent_pool_handle_attach_edge(extent_pool, packet_p);
            break;
#endif
        /* extent_pool specific handling here.
         */
        case FBE_EXTENT_POOL_CONTROL_CODE_SET_CONFIGURATION:
            status = fbe_extent_pool_set_configuration(extent_pool, packet_p);
            break;

        /* extent_pool fake RG handling here.
         */
#ifndef NO_EXT_POOL_ALIAS
        case FBE_RAID_GROUP_CONTROL_CODE_GET_INFO:
            status = fbe_extent_pool_get_raid_group_info(extent_pool, packet_p);
            break;
        case FBE_RAID_GROUP_CONTROL_CODE_GET_POWER_SAVING_PARAMETERS:
            status = fbe_extent_pool_get_power_saving_parameters(extent_pool, packet_p);
            break;
#endif

        default:
            /* Allow the  object to handle all other ioctls.
             */
            status = fbe_base_config_control_entry(object_handle, packet_p);

            /* If traverse flag is set then send the control packet down to 
             * block edge.
             */
            if (status == FBE_STATUS_TRAVERSE)
            {
                status = fbe_block_transport_send_control_packet (
                            ((fbe_base_config_t *)extent_pool)->block_edge_p,
                            packet_p);
            }
            break;
    }
    return status;
}
/* end fbe_extent_pool_control_entry() */

/*!**************************************************************
 * fbe_extent_pool_handle_attach_edge()
 ****************************************************************
 * @brief
 *  This function kicks off the attach of an edge.
 *
 * @param extent_pool_p - Pointer to extent pool object.
 * @param packet_p - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  6/23/2014 - Created. Rob Foley
 *
 ****************************************************************/
static fbe_status_t
fbe_extent_pool_handle_attach_edge(fbe_extent_pool_t * extent_pool_p,   
                                   fbe_packet_t * packet_p)
{   
    fbe_transport_set_completion_function(packet_p, fbe_extent_pool_handle_attach_edge_completion, extent_pool_p);
    return fbe_base_config_attach_upstream_block_edge((fbe_base_config_t*)extent_pool_p, packet_p);
}
/**************************************
 * end fbe_extent_pool_handle_attach_edge()
 **************************************/

/*!**************************************************************
 * fbe_extent_pool_handle_attach_edge_completion()
 ****************************************************************
 * @brief
 *  If the edge was attached successfully,
 *  create the map table.
 *
 * @param packet_p - Completing packet.
 * @param context - extent pool.
 * 
 * @return fbe_status_t
 *
 * @author
 *  6/23/2014 - Created. Rob Foley
 *
 ****************************************************************/
static fbe_status_t
fbe_extent_pool_handle_attach_edge_completion(fbe_packet_t *packet_p, fbe_packet_completion_context_t context)
{
    fbe_extent_pool_t                         *extent_pool_p = (fbe_extent_pool_t*)context;
    fbe_status_t                               status;
    fbe_block_transport_control_attach_edge_t *attach_edge_p = NULL;
    fbe_payload_ex_t                          *payload_p = NULL;
    fbe_payload_control_operation_t           *control_operation_p = NULL;

    status = fbe_transport_get_status_code(packet_p);
    if (status != FBE_STATUS_OK) { 
        return status; 
    } 

    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);
    fbe_payload_control_get_buffer(control_operation_p, &attach_edge_p);

    if (attach_edge_p == NULL){
        fbe_base_object_trace(  (fbe_base_object_t *)extent_pool_p, 
                                FBE_TRACE_LEVEL_ERROR, 
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                                "%s no buffer\n", __FUNCTION__);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    /*! Create a fully mapped extent pool. 
     */
    status = fbe_extent_pool_fully_map_lun(extent_pool_p,
                                           attach_edge_p->block_edge->base_edge.server_index, /* LUN */ 
                                           attach_edge_p->block_edge->capacity,
                                           attach_edge_p->block_edge->offset);

    if (status != FBE_STATUS_OK) { 
        return status;
    }

    /* Populate the user slice with the disk map
     */
    fbe_extent_pool_construct_lun_slices(extent_pool_p,
                                         attach_edge_p->block_edge->base_edge.server_index /* LUN */);

    /* Regardless of what status we recieve, we want to send back the packet, so return OK.
     */
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_extent_pool_handle_attach_edge_completion()
 ******************************************/
/*!****************************************************************************
 * fbe_extent_pool_set_configuration()
 ******************************************************************************
 * @brief
 *  This function sets up the basic configuration info for this provision 
 *  drive object.
 *
 * @param extent_pool_p - Pointer to provision drive object
 * @param packet_p - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  06/20/2014 - Created. Lili Chen
 *
 ******************************************************************************/
fbe_status_t
fbe_extent_pool_set_configuration(fbe_extent_pool_t *extent_pool_p, fbe_packet_t * packet_p)
{
    fbe_extent_pool_configuration_t *           config_p = NULL;    /* INPUT */
    fbe_status_t                                status = FBE_STATUS_OK;
    fbe_payload_ex_t *                          payload_p = NULL;
    fbe_payload_control_operation_t *           control_operation_p = NULL; 
    fbe_u32_t                                   length = 0;  

    /* Get the payload to set the configuration information.
     */
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);  

    fbe_payload_control_get_buffer(control_operation_p, &config_p);
    if (config_p == NULL)
    {
        status = FBE_STATUS_GENERIC_FAILURE;
        fbe_base_object_trace((fbe_base_object_t *)extent_pool_p,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s payload buffer NULL.\n",
                                __FUNCTION__);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return status;
    }

    /* If length of the buffer does not match with set configuration request 
     * structure lenght then return error.
     */
    fbe_payload_control_get_buffer_length (control_operation_p, &length);
    if(length != sizeof(fbe_extent_pool_configuration_t))
    {
        status = FBE_STATUS_GENERIC_FAILURE;
        fbe_base_object_trace((fbe_base_object_t *)extent_pool_p,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s payload buffer lenght mismatch.\n",
                                __FUNCTION__);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return status;
    }

    fbe_extent_pool_lock(extent_pool_p);

    /* Set the configured capacity. */
    fbe_base_config_set_width((fbe_base_config_t *)extent_pool_p, config_p->width);

    /* Set the generation number. */
    fbe_base_config_set_generation_num((fbe_base_config_t*)extent_pool_p, config_p->generation_number);

    fbe_extent_pool_unlock(extent_pool_p);

    fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet_p, status, 0);
    fbe_transport_complete_packet(packet_p);
    return status;
}
/******************************************************************************
 * end fbe_extent_pool_set_configuration()
 ******************************************************************************/

fbe_status_t 
fbe_extent_pool_usurper_get_control_buffer(fbe_extent_pool_t *extent_pool_p, 
                                           fbe_packet_t * packet_p,
                                           fbe_payload_control_buffer_length_t in_buffer_length,
                                           fbe_payload_control_buffer_t*       out_buffer_pp)
{
    fbe_payload_ex_t*                  payload_p;
    fbe_payload_control_operation_t*    control_operation_p;  
    fbe_payload_control_buffer_length_t buffer_length;


    // Get the control op buffer data pointer from the packet payload.
    payload_p = fbe_transport_get_payload_ex(packet_p);
    if (payload_p == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t *)extent_pool_p,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s payload is NULL\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);
    if (control_operation_p == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t *)extent_pool_p,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s control operation is NULL\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer(control_operation_p, out_buffer_pp);
    if (*out_buffer_pp == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t *)extent_pool_p,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s buffer pointer is NULL\n", __FUNCTION__);

        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        return FBE_STATUS_GENERIC_FAILURE;
    };

    fbe_payload_control_get_buffer_length(control_operation_p, &buffer_length);
    if (buffer_length != in_buffer_length)
    {
        fbe_base_object_trace((fbe_base_object_t *)extent_pool_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s expected len: %d actual len: %d\n", 
                              __FUNCTION__, in_buffer_length, buffer_length);

        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return FBE_STATUS_OK;

}


#ifndef NO_EXT_POOL_ALIAS

/*!**************************************************************
 * fbe_extent_pool_get_raid_group_info()
 ****************************************************************
 * @brief
 *  This function returns raid group information about ext pool.
 *
 * @param extent_pool_p - The extent pool.
 * @param packet_p - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  07/12/2014 - Created. Lili Chen
 *
 ****************************************************************/
static fbe_status_t
fbe_extent_pool_get_raid_group_info(fbe_extent_pool_t *extent_pool_p, fbe_packet_t * packet_p)
{
    fbe_raid_group_get_info_t * get_info_p = NULL;
    fbe_lba_t capacity;
    fbe_u32_t width;
    fbe_element_size_t element_size;
    fbe_raid_geometry_t *raid_geometry_p = extent_pool_p->geometry_p;
    fbe_u32_t           position_index;         // index for a specific disk position 
    fbe_base_config_flags_t base_config_flags;
    fbe_base_config_clustered_flags_t base_config_clustered_flags;
    fbe_base_config_clustered_flags_t base_config_peer_clustered_flags;
    fbe_status_t status;
    fbe_object_id_t my_object_id;
    fbe_block_edge_t *block_edge_p = NULL;
    fbe_lba_t     journal_log_start_lba;

    status = fbe_extent_pool_usurper_get_control_buffer(extent_pool_p,
                                            packet_p, 
                                            sizeof(fbe_raid_group_get_info_t),
                                            (fbe_payload_control_buffer_t)&get_info_p);
    if (status != FBE_STATUS_OK)
    {
        status = FBE_STATUS_GENERIC_FAILURE;
        fbe_base_object_trace((fbe_base_object_t *)extent_pool_p,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s payload buffer NULL.\n",
                                __FUNCTION__);
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return status;
    }

    //exported_disk_capacity = fbe_raid_group_get_exported_disk_capacity(extent_pool_p);

    get_info_p->background_op_seconds = 0;

    /* Validation checks are complete. 
     * Collect the raid group information. 
     */
    fbe_extent_pool_lock(extent_pool_p);

    fbe_base_config_get_capacity((fbe_base_config_t*)extent_pool_p, &capacity);
    get_info_p->capacity = capacity;

    fbe_base_config_get_width((fbe_base_config_t*)extent_pool_p, &width);
    get_info_p->width = raid_geometry_p->width;
    get_info_p->physical_width = get_info_p->width;
 
    get_info_p->paged_metadata_start_lba = capacity;
    get_info_p->paged_metadata_capacity = 0x2000;
    fbe_raid_geometry_get_data_disks(raid_geometry_p, &get_info_p->num_data_disk);
    get_info_p->write_log_start_pba = (get_info_p->paged_metadata_start_lba + get_info_p->paged_metadata_capacity) / get_info_p->num_data_disk;
    get_info_p->write_log_physical_capacity = 0x10000;
    get_info_p->raid_capacity = capacity + get_info_p->paged_metadata_capacity;
    get_info_p->imported_blocks_per_disk = capacity * 2 / width;

    fbe_raid_geometry_get_block_edge(raid_geometry_p, &block_edge_p, 0);
    get_info_p->physical_offset = fbe_block_transport_edge_get_offset(block_edge_p);

    get_info_p->debug_flags = 0;
    fbe_raid_geometry_get_debug_flags(raid_geometry_p, &get_info_p->library_debug_flags);
    fbe_base_config_get_flags((fbe_base_config_t *) extent_pool_p, &base_config_flags);

    get_info_p->flags = 0;
    get_info_p->base_config_flags = base_config_flags;

    fbe_base_config_get_clustered_flags((fbe_base_config_t *) extent_pool_p, &base_config_clustered_flags);
    get_info_p->base_config_clustered_flags = base_config_clustered_flags;
    fbe_base_config_get_peer_clustered_flags((fbe_base_config_t *) extent_pool_p, &base_config_peer_clustered_flags);
    get_info_p->base_config_peer_clustered_flags = base_config_peer_clustered_flags;
    get_info_p->metadata_element_state = extent_pool_p->base_config.metadata_element.metadata_element_state;

    fbe_raid_geometry_get_element_size(raid_geometry_p, &element_size);
    get_info_p->element_size = element_size;

    fbe_raid_geometry_get_raid_type(raid_geometry_p, &get_info_p->raid_type);
    fbe_raid_geometry_get_exported_size(raid_geometry_p, &get_info_p->exported_block_size);
    fbe_raid_geometry_get_imported_size(raid_geometry_p, &get_info_p->imported_block_size);
    fbe_raid_geometry_get_optimal_size(raid_geometry_p, &get_info_p->optimal_block_size);

    fbe_raid_geometry_get_data_disks(raid_geometry_p, &get_info_p->num_data_disk);
    get_info_p->sectors_per_stripe = element_size * get_info_p->num_data_disk;

    get_info_p->stripe_count = (fbe_u32_t)(capacity / (get_info_p->sectors_per_stripe));
    fbe_raid_geometry_get_4k_bitmask(raid_geometry_p, &get_info_p->bitmask_4k);

    // Get elements per parity
    fbe_raid_geometry_get_elements_per_parity(raid_geometry_p, &get_info_p->elements_per_parity_stripe);
    fbe_raid_geometry_get_journal_log_start_lba(raid_geometry_p, &journal_log_start_lba);

    //  Get the rebuild logging setting and rebuild checkpoint for each disk position
    // For RAID 10, we'll have to get it from the objects below.
    for (position_index = 0; position_index < width; position_index++)
    {
        get_info_p->rebuild_checkpoint[position_index] = FBE_LBA_INVALID;                    
    }

    get_info_p->ro_verify_checkpoint = 
    get_info_p->rw_verify_checkpoint = 
    get_info_p->error_verify_checkpoint = 
    get_info_p->journal_verify_checkpoint =  
    get_info_p->system_verify_checkpoint =              
    get_info_p->incomplete_write_verify_checkpoint = FBE_LBA_INVALID;
    
    get_info_p->raid_group_np_md_flags = 0; 
    get_info_p->raid_group_np_md_extended_flags = 0;
            
    //fbe_raid_group_get_lun_align_size(extent_pool_p, &get_info_p->lun_align_size);
    get_info_p->lun_align_size = FBE_RAID_DEFAULT_CHUNK_SIZE * get_info_p->num_data_disk;

    get_info_p->local_state = 0;
    get_info_p->peer_state = extent_pool_p->extent_pool_metadata_memory_peer.base_config_metadata_memory.lifecycle_state;

    get_info_p->paged_metadata_verify_pass_count = 0;

    get_info_p->total_chunks = (fbe_chunk_count_t)(get_info_p->capacity / get_info_p->num_data_disk) / FBE_RAID_DEFAULT_CHUNK_SIZE;
    fbe_base_object_get_object_id((fbe_base_object_t*)extent_pool_p, &my_object_id);
    get_info_p->user_private = FBE_FALSE;

    /* Retrieve if anything is in the raid group objects event q.
     * We already have the rg config lock, so don't get it again.
     */
    get_info_p->b_is_event_q_empty = fbe_base_config_event_queue_is_empty_no_lock((fbe_base_config_t*)extent_pool_p);
    
    get_info_p->rekey_checkpoint = FBE_LBA_INVALID;

    fbe_base_config_get_encryption_mode((fbe_base_config_t*)extent_pool_p, &get_info_p->encryption_mode);

    fbe_extent_pool_unlock(extent_pool_p);

    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);
    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_extent_pool_get_raid_group_info()
 **************************************/
#endif

/*!**************************************************************
 * fbe_extent_pool_get_power_saving_parameters()
 ****************************************************************
 * @brief
 *  returns the power saving related parameters the user configured in RG
 *
 * @param extent_pool_t   - Pointer to the raid group.
 * @param packet_p       - Pointer to packet
 
 * @return status - The status of the operation. 
 *
 * @author
 *  05/03/2010 - Created. sharel
 *
 ****************************************************************/
static fbe_status_t 
fbe_extent_pool_get_power_saving_parameters(fbe_extent_pool_t * extent_pool_p, fbe_packet_t *packet_p)
{
    fbe_raid_group_get_power_saving_info_t *    get_power_save_info;
    fbe_status_t                                status;

    /* Get the control buffer pointer from the packet payload. */
    status = fbe_extent_pool_usurper_get_control_buffer(extent_pool_p, packet_p, 
                                            sizeof(fbe_raid_group_get_power_saving_info_t),
                                            (fbe_payload_control_buffer_t)&get_power_save_info);
    if (status != FBE_STATUS_OK) 
    { 
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return status; 
    }

    /* Populate the information the user asked for*/
    get_power_save_info->idle_time_in_sec = 60;
    get_power_save_info->max_raid_latency_time_is_sec = 120;
    get_power_save_info->power_saving_enabled = FBE_FALSE;
    
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);

    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_extent_pool_get_power_saving_parameters()
 **************************************/


/************************************
 * end fbe_extent_pool_usurper.c
 ************************************/
