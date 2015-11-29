/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_provision_drive_metadata.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the provision drive object's metadata related 
 *  functionality.
 * 
 * @ingroup provision_drive_class_files
 * 
 * @version
 *   10/30/2009:  Created. Dhaval Patel
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/


#include "fbe_provision_drive_private.h"
#include "fbe_transport_memory.h"
#include "fbe/fbe_enclosure.h"
#include "fbe_raid_library.h"
#include "fbe/fbe_physical_drive.h"
#include "fbe/fbe_event_log_utils.h"               /*  for message codes */

/*************************
 *   FORWARD DECLARATIONS
 *************************/

static fbe_status_t fbe_provision_drive_metadata_write_default_paged_metadata_completion(fbe_packet_t * packet_p,
                                                                                         fbe_packet_completion_context_t context);
static fbe_status_t fbe_provision_drive_write_default_paged_metadata_allocate_completion(fbe_memory_request_t * memory_request_p, 
                                                                                         fbe_memory_completion_context_t context);
static fbe_raid_state_status_t fbe_provision_drive_iots_resend_write_default_paged_meta(fbe_raid_iots_t *iots_p);


static fbe_status_t fbe_provision_drive_metadata_get_physical_drive_info_completion(fbe_packet_t * packet_p,
                                                                                    fbe_packet_completion_context_t context);


static fbe_status_t fbe_provision_drive_metadata_update_physical_drive_info_completion(fbe_packet_t * packet_p,
                                                                                       fbe_packet_completion_context_t context);

static fbe_status_t
fbe_provision_drive_zero_paged_metadata_allocate_completion(fbe_memory_request_t * memory_request_p, 
                                                                                  fbe_memory_completion_context_t context);

static fbe_status_t 
fbe_provision_drive_zero_paged_metadata(fbe_provision_drive_t * provision_drive_p, fbe_packet_t *packet_p);

static fbe_status_t fbe_provision_drive_zero_paged_metadata_send_subpacket(fbe_provision_drive_t *provision_drive_p,
                                                                           fbe_packet_t *new_packet_p,
                                                                           fbe_packet_t *original_packet_p,
                                                                           fbe_lba_t start_lba,
                                                                           fbe_block_count_t block_count);

static fbe_status_t fbe_provision_drive_zero_paged_metadata_send_subpacket_completion(fbe_packet_t *subpacket,
                                                                                      fbe_packet_completion_context_t packet_completion_context);

static fbe_status_t fbe_provision_drive_metadata_zero_nonpaged_metadata_completion(fbe_packet_t *packet_p, fbe_packet_completion_context_t context);

static fbe_status_t fbe_provision_drive_metadata_mark_consumed_completion(fbe_packet_t * packet_p,
                                                               fbe_packet_completion_context_t context);  
static fbe_status_t fbe_provision_drive_metadata_unmark_zero_completion(fbe_packet_t * packet_p, fbe_packet_completion_context_t context);
static fbe_status_t
fbe_provision_drive_metadata_write_paged_metadata_chunk_completion(fbe_packet_t * packet_p,
                                                                   fbe_packet_completion_context_t context);
static fbe_status_t
fbe_provision_drive_metadata_unmark_zero(fbe_provision_drive_t * provision_drive_p,
                                         fbe_packet_t * packet_p,
                                         fbe_packet_completion_function_t completion_function,
                                         fbe_bool_t use_write_verify);
static fbe_status_t
fbe_provision_drive_mark_consumed_update_paged_metadata(fbe_provision_drive_t * provision_drive_p,
                                                        fbe_packet_t * packet_p,
                                                        fbe_packet_completion_function_t completion_function,
                                                        fbe_bool_t use_write_verify);
static fbe_status_t                                                                                                  
fbe_provision_drive_metadata_mark_consumed_write_verify_completion(fbe_packet_t * packet_p,
                                                                   fbe_packet_completion_context_t context);   
static fbe_status_t                                                                                                  
fbe_provision_drive_metadata_unmark_zero_write_verify_completion(fbe_packet_t * packet_p,
                                                                 fbe_packet_completion_context_t context);                                          
static fbe_status_t
fbe_provision_drive_write_default_system_paged_alloc_completion(fbe_memory_request_t * memory_request_p, 
                                                                fbe_memory_completion_context_t context);
static fbe_status_t
fbe_provision_drive_write_default_system_paged_completion(fbe_packet_t * packet_p,
                                                          fbe_packet_completion_context_t context);
static fbe_status_t fbe_provision_drive_verify_paged_completion(fbe_packet_t * packet_p,
                                                                fbe_packet_completion_context_t context);
static fbe_status_t fbe_provision_drive_verify_paged_allocate_completion(fbe_memory_request_t * memory_request_p, 
                                                                         fbe_memory_completion_context_t context);

static fbe_status_t fbe_provision_drive_init_validation_area_allocate_completion(fbe_memory_request_t * memory_request_p, 
                                                                         fbe_memory_completion_context_t context);
static fbe_status_t fbe_provision_drive_init_validation_area_completion(fbe_packet_t * packet_p,
                                                                        fbe_packet_completion_context_t context);
static fbe_status_t fbe_provision_drive_validation_set_completion(fbe_packet_t * packet_p,
                                                                  fbe_packet_completion_context_t context);
static fbe_status_t fbe_provision_drive_set_read_validate_key(fbe_provision_drive_t * provision_drive_p,
                                                              fbe_packet_t *packet_p,
                                                              fbe_edge_index_t client_index);
static fbe_status_t fbe_provision_drive_set_write_validation_key(fbe_provision_drive_t * provision_drive_p,
                                                                 fbe_packet_t *packet_p,
                                                                 fbe_edge_index_t client_index);
static fbe_status_t fbe_provision_drive_metadata_write_default_paged_for_ext_pool_alloc_completion(fbe_memory_request_t * memory_request_p, 
                                                                                                   fbe_memory_completion_context_t context);
static fbe_status_t fbe_provision_drive_metadata_write_default_paged_for_ext_pool_completion(fbe_packet_t * packet_p,
                                                                                             fbe_packet_completion_context_t context);

/*!****************************************************************************
 * fbe_provision_drive_get_metadata_positions()
 ******************************************************************************
 * @brief
 *  This function is used to get the metadata position from the
 *  object.
 *
 * @param provision_drive_p                  - Provision drive object.  
 * @param provision_drive_metadata_positions - Pointer to the metadata position.
 *
 * @return fbe_status_t
 *  The status of the operation typically FBE_STATUS_OK.
 *
 * @author
 *  10/14/2009 - Created. Dhaval Patel
 *
 ******************************************************************************/
fbe_status_t  
fbe_provision_drive_get_metadata_positions(fbe_provision_drive_t *  provision_drive_p,
                                           fbe_provision_drive_metadata_positions_t * provision_drive_metadata_positions)
{
    fbe_status_t                status;
    /* fbe_block_edge_geometry_t block_edge_geometry; */
    fbe_optimum_block_size_t    optimum_block_size;
    fbe_block_size_t            block_size;
    fbe_lba_t                   imported_capacity = FBE_LBA_INVALID;
    
    /* fbe_block_transport_edge_get_geometry(&provision_drive_p->block_edge, &block_edge_geometry); */
    fbe_block_transport_edge_get_optimum_block_size(&provision_drive_p->block_edge, &optimum_block_size);
    fbe_block_transport_edge_get_exported_block_size(&provision_drive_p->block_edge, &block_size);

    /*!@todo This needs to use the exported capacity going forward, since if drive is not present 
     * it will see invalid inported capacity.
     */
    imported_capacity = provision_drive_p->block_edge.capacity;

    /* Round the imported capacity to down side, it is needed to make sure that
     * exported capacity and metadata is always aligned with chunk size. 
     */ 
    if(imported_capacity % FBE_PROVISION_DRIVE_CHUNK_SIZE)
    {
        imported_capacity = provision_drive_p->block_edge.capacity - (provision_drive_p->block_edge.capacity % FBE_PROVISION_DRIVE_CHUNK_SIZE);
    }

    status = fbe_class_provision_drive_get_metadata_positions(imported_capacity,
                                                              provision_drive_metadata_positions);
    
    return status;
}
/******************************************************************************
 * end fbe_provision_drive_get_metadata_positions()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_provision_drive_metadata_initialize_metadata_element()
 ******************************************************************************
 * @brief
 *  This function is used to initialize the metadata element of the provision
 *  drive object.
 *
 * @param provision_drive_p         - pointer to the provision drive
 * @param packet_p             - pointer to a monitor packet.
 *
 * @return  fbe_status_t  
 *
 * @author
 *   12/16/2009 - Created. Dhaval Patel
 *
 ******************************************************************************/
fbe_status_t
fbe_provision_drive_metadata_initialize_metadata_element(fbe_provision_drive_t * provision_drive_p,
                                                         fbe_packet_t * packet_p)
{
    fbe_provision_drive_metadata_positions_t    metadata_positions;
    fbe_status_t                                status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_ex_t *                             payload_p = NULL;
    fbe_payload_control_operation_t *           control_operation_p = NULL;
    fbe_lba_t                                   exported_capacity = FBE_LBA_INVALID;

    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);


    /* Get the metadata position for the provision drive object. */
    //fbe_base_config_get_capacity((fbe_base_config_t *)provision_drive_p, &exported_capacity);
    fbe_provision_drive_get_user_capacity(provision_drive_p, &exported_capacity);
    status = fbe_class_provision_drive_get_metadata_positions_from_exported_capacity(exported_capacity, &metadata_positions);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s metadata position calculation failed, status:0x%x.\n",
                              __FUNCTION__, status);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, status, 0);
        return status;
    }

    /* debug trace to track the paged metadata and signature data positions. */
    fbe_provision_drive_utils_trace( provision_drive_p,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_INFO, FBE_PROVISION_DRIVE_DEBUG_FLAG_METADATA_TRACING,
                          "%s metadata position, paged_lba:0x%llx, mir_off:0x%llx\n",
                          __FUNCTION__,
                          (unsigned long long)metadata_positions.paged_metadata_lba,
                          (unsigned long long)metadata_positions.paged_mirror_metadata_offset);

    /* initialize metadata element with metadata position information. */
    fbe_base_config_metadata_set_paged_record_start_lba((fbe_base_config_t *) provision_drive_p,
                                                        metadata_positions.paged_metadata_lba);

    fbe_base_config_metadata_set_paged_mirror_metadata_offset((fbe_base_config_t *) provision_drive_p,
                                                              metadata_positions.paged_mirror_metadata_offset);

    fbe_base_config_metadata_set_paged_metadata_capacity((fbe_base_config_t *) provision_drive_p,
                                                        metadata_positions.paged_metadata_capacity);

    fbe_base_config_metadata_set_number_of_stripes((fbe_base_config_t *) provision_drive_p,
                                                         metadata_positions.number_of_stripes);

    fbe_base_config_metadata_set_number_of_private_stripes((fbe_base_config_t *) provision_drive_p,
                                                         metadata_positions.number_of_private_stripes);

    /* Use local cache for paged metadata. */
    if (fbe_provision_drive_metadata_cache_is_enabled(provision_drive_p)) {
        fbe_metadata_element_set_paged_object_cache_flag(&((fbe_base_config_t *)provision_drive_p)->metadata_element);
        fbe_metadata_element_set_paged_object_cache_function(&((fbe_base_config_t *)provision_drive_p)->metadata_element,
                                                             fbe_provision_drive_metadata_cache_callback_entry);
    }

    /* complete the packet with good status here. */
    fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);
    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_provision_drive_metadata_initialize_metadata_element()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_provision_drive_nonpaged_metadata_init()
 ******************************************************************************
 * @brief
 *  This function is used to initialize the nonpaged metadata memory of the
 *  provision drive object.
 *
 * @param provision_drive_p         - pointer to the provision drive
 * @param packet_p                  - pointer to a monitor packet.
 *
 * @return  fbe_status_t  
 *
 * @author
 *   03/01/2010 - Created. Dhaval Patel
 *
 ******************************************************************************/
fbe_status_t 
fbe_provision_drive_nonpaged_metadata_init(fbe_provision_drive_t * provision_drive_p,
                                           fbe_packet_t *packet_p)
{
    fbe_status_t status;

    /* call the base config nonpaged metadata init to initialize the metadata. */
    status = fbe_base_config_metadata_nonpaged_init((fbe_base_config_t *) provision_drive_p,
                                                    sizeof(fbe_provision_drive_nonpaged_metadata_t),
                                                    packet_p);
    return status;
}
/******************************************************************************
 * end fbe_provision_drive_nonpaged_metadata_init()
 ******************************************************************************/

/*this function is called to start the process of zeroing the paged metadata area before we write into it*/
static fbe_status_t 
fbe_provision_drive_zero_paged_metadata(fbe_provision_drive_t * provision_drive_p,
                                        fbe_packet_t *packet_p)
{
    fbe_payload_block_operation_t *         block_operation_p = NULL;
    fbe_payload_ex_t *                     sep_payload_p = NULL;
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_block_count_t                       block_count;
    fbe_memory_request_t *                  memory_request_p = NULL;
    
    /* Get the payload and block operation. */
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    block_operation_p = fbe_payload_ex_get_block_operation(sep_payload_p);
    fbe_payload_block_get_block_count(block_operation_p, &block_count);

    /* Allocate the subpacket to handle the zero */
    memory_request_p = fbe_transport_get_memory_request(packet_p);

    /* build the memory request for allocation. */
    fbe_memory_build_chunk_request(memory_request_p,
                                   FBE_MEMORY_CHUNK_SIZE_FOR_PACKET,
                                   1,/*we will use one packet which we'll just send up and down until the zero complets*/
                                   fbe_transport_get_resource_priority(packet_p),
                                   fbe_transport_get_io_stamp(packet_p),
                                   (fbe_memory_completion_function_t)fbe_provision_drive_zero_paged_metadata_allocate_completion, /* SAFEBUG - cast to supress warning but must fix because memory completion function shouldn't return status */
                                   provision_drive_p);

    fbe_transport_memory_request_set_io_master(memory_request_p, packet_p);

    /* Issue the memory request to memory service. */
    status = fbe_memory_request_entry(memory_request_p);
    if((status != FBE_STATUS_OK) && (status != FBE_STATUS_PENDING))
    {
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s memory allocation failed",  __FUNCTION__);

        fbe_payload_block_set_status(block_operation_p,
                                     FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID_REQUEST,
                                     FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_INSUFFICIENT_RESOURCES);
        fbe_provision_drive_utils_complete_packet(packet_p, FBE_STATUS_OK, 0);
        return status;
    }
  
    return FBE_STATUS_PENDING;

}

/*!****************************************************************************
 * fbe_provision_drive_metadata_write_default_paged_metadata()
 ******************************************************************************
 * @brief
 *   This function is used to initialize the metadata with default values for 
 *   the non paged and paged metadata. Before we initialize it we have to
 *   zero out this area.
 *
 * @param provision_drive_p         - pointer to the provision drive
 * @param packet_p                  - pointer to a monitor packet.
 *
 * @return  fbe_status_t  
 *
 * @author
 *   03/01/2010 - Created. Dhaval Patel
 *
 ******************************************************************************/
fbe_status_t 
fbe_provision_drive_metadata_write_default_paged_metadata(fbe_provision_drive_t * provision_drive_p,
                                                          fbe_packet_t *packet_p)
{
    fbe_status_t                                    status = FBE_STATUS_GENERIC_FAILURE; 
    fbe_payload_ex_t *                              sep_payload_p = NULL;
    fbe_payload_control_operation_t *               control_operation_p = NULL;
    fbe_memory_request_t *                          memory_request_p = NULL;

    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(sep_payload_p);

    /* allocate memory for client_blob */
    memory_request_p = fbe_transport_get_memory_request(packet_p);
    fbe_memory_build_chunk_request(memory_request_p,
                                   FBE_MEMORY_CHUNK_SIZE_FOR_64_BLOCKS_IO,
                                   2, /* number of chunks */
                                   fbe_transport_get_resource_priority(packet_p),
                                   fbe_transport_get_io_stamp(packet_p),
                                   (fbe_memory_completion_function_t)fbe_provision_drive_write_default_paged_metadata_allocate_completion, /* SAFEBUG - cast to supress warning but must fix because memory completion function shouldn't return status */
                                   provision_drive_p);
    fbe_transport_memory_request_set_io_master(memory_request_p, packet_p);

    /* Issue the memory request to memory service. */
    status = fbe_memory_request_entry(memory_request_p);
    if((status != FBE_STATUS_OK) && (status != FBE_STATUS_PENDING))
    {
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s memory allocation failed",  __FUNCTION__);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return status;
    }

    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/******************************************************************************
 * end fbe_provision_drive_metadata_write_default_paged_metadata()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_provision_drive_write_default_paged_metadata_allocate_completion()
 ******************************************************************************
 * @brief
 *  This is the completion for memory allocation for paged blob.
 *
 * @param memory_request_p         - pointer to the memory request
 * @param context                  - context.
 *
 * @return  fbe_status_t  
 *
 * @author
 *   02/01/2013 - Created. Lili Chen
 *
 ******************************************************************************/
static fbe_status_t
fbe_provision_drive_write_default_paged_metadata_allocate_completion(fbe_memory_request_t * memory_request_p, 
                                                                     fbe_memory_completion_context_t context)
{
    fbe_status_t                                    status = FBE_STATUS_GENERIC_FAILURE; 
    fbe_u64_t                                       repeat_count = 0;
    fbe_provision_drive_paged_metadata_t            paged_set_bits;
    fbe_payload_ex_t *                              sep_payload_p = NULL;
    fbe_payload_metadata_operation_t *              metadata_operation_p = NULL;
    fbe_payload_control_operation_t *               control_operation_p = NULL;
    fbe_lba_t                                       paged_metadata_capacity = 0;
    fbe_packet_t *                                  packet_p;
    fbe_provision_drive_t *                         provision_drive_p = (fbe_provision_drive_t *) context;

    /* get the payload and control operation. */
    packet_p = fbe_transport_memory_request_to_packet(memory_request_p);
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(sep_payload_p);

    /* Check allocation status */
    if (fbe_memory_request_is_allocation_complete(memory_request_p) == FBE_FALSE)
    {
        /* Currently this callback doesn't expect any aborted requests */
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p, 
                                        FBE_TRACE_LEVEL_ERROR,
                                        FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                        "%s memory allocation failed, state:%d\n",
                                        __FUNCTION__, memory_request_p->request_state);

        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*let's Initialize the paged bith with default values. */
    fbe_zero_memory(&paged_set_bits, sizeof(fbe_provision_drive_paged_metadata_t));
    paged_set_bits.need_zero_bit = 1; 
    paged_set_bits.consumed_user_data_bit = 0;
    paged_set_bits.user_zero_bit = 0;
    paged_set_bits.valid_bit = 1; 
    paged_set_bits.unused_bit = 0;

        /* Set the repeat count as number of records in paged metadata capacity.
         * This is the number of chunks of user data + paddings for alignment.
         */
    fbe_base_config_metadata_get_paged_metadata_capacity((fbe_base_config_t *)provision_drive_p,
                                                       &paged_metadata_capacity);
        repeat_count = (paged_metadata_capacity*FBE_METADATA_BLOCK_DATA_SIZE)/sizeof(fbe_provision_drive_paged_metadata_t);

    /* Allocate the metadata operation. */
    metadata_operation_p =  fbe_payload_ex_allocate_metadata_operation(sep_payload_p);

    /* Build the paged metadata write request. */
    status = fbe_payload_metadata_build_paged_write(metadata_operation_p, 
                                                    &(((fbe_base_config_t *) provision_drive_p)->metadata_element),
                                                    0, /* Paged metadata offset is zero for the default paged metadata write. 
                                                          paint the whole paged metadata with the default value */
                                                    (fbe_u8_t *) &paged_set_bits,
                                                    sizeof(fbe_provision_drive_paged_metadata_t),
                                                    repeat_count);
    if(status != FBE_STATUS_OK)
    {
        fbe_memory_free_request_entry(memory_request_p);
        fbe_payload_ex_release_metadata_operation(sep_payload_p, metadata_operation_p);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, status, 0);
        return status;
    }

    /* Initialize cient_blob */
    status = fbe_metadata_paged_init_client_blob(memory_request_p, metadata_operation_p, 0, FBE_FALSE);

    /* set the completion function for the default paged metadata write. */
    fbe_transport_set_completion_function(packet_p, fbe_provision_drive_metadata_write_default_paged_metadata_completion, provision_drive_p);

    fbe_payload_ex_increment_metadata_operation_level(sep_payload_p);

    /* Issue the metadata operation. */
    fbe_metadata_operation_entry(packet_p);
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;

    
}
/******************************************************************************
 * end fbe_provision_drive_write_default_paged_metadata_allocate_completion()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_provision_drive_metadata_write_default_paged_metadata_completion()
 ******************************************************************************
 * @brief
 *  This function is used to handle the comletion of the default paged metadata
 *  initialization.
 *
 * @param provision_drive_p         - pointer to the provision drive
 * @param packet_p                  - pointer to a monitor packet.
 *
 * @return  fbe_status_t  
 *
 * @author
 *   03/01/2010 - Created. Dhaval Patel
 *
 ******************************************************************************/
static fbe_status_t
fbe_provision_drive_metadata_write_default_paged_metadata_completion(fbe_packet_t * packet_p,
                                                                     fbe_packet_completion_context_t context)
{
    fbe_provision_drive_t *                 provision_drive_p = NULL;
    fbe_payload_metadata_operation_t *      metadata_operation_p = NULL;
    fbe_payload_control_operation_t *       control_operation_p = NULL;
    fbe_payload_ex_t *                     sep_payload_p = NULL;
    fbe_status_t                            status;
    fbe_payload_metadata_status_t           metadata_status;
    fbe_payload_metadata_operation_opcode_t metadata_opcode;
    fbe_memory_request_t *                  memory_request = NULL;
    fbe_scheduler_hook_status_t             hook_status;
    
    provision_drive_p = (fbe_provision_drive_t *) context;

    /* get the payload and metadata operation. */
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    metadata_operation_p =  fbe_payload_ex_get_metadata_operation(sep_payload_p);
    control_operation_p = fbe_payload_ex_get_prev_control_operation(sep_payload_p);
    
    /* get the status of the paged metadata operation. */
    fbe_payload_metadata_get_status(metadata_operation_p, &metadata_status);
    status = fbe_transport_get_status_code(packet_p);

    fbe_payload_metadata_get_opcode(metadata_operation_p, &metadata_opcode);

    /* release the metadata operation. */
    fbe_payload_ex_release_metadata_operation(sep_payload_p, metadata_operation_p);

    /* release client_blob */
    memory_request = fbe_transport_get_memory_request(packet_p);
    fbe_memory_free_request_entry(memory_request);

    if((status != FBE_STATUS_OK) ||
       (metadata_status != FBE_PAYLOAD_METADATA_STATUS_OK))
    {
        if (metadata_status == FBE_PAYLOAD_METADATA_STATUS_BAD_KEY_HANDLE){
            /* We decided to critical on this error since it is a software bug.
             */
            fbe_base_object_trace((fbe_base_object_t*)provision_drive_p, FBE_TRACE_LEVEL_CRITICAL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                  "provision drive write default paged: Key not found error status:0x%x, metadata_status:0x%x.\n",
                                  status, metadata_status);

            fbe_provision_drive_check_hook(provision_drive_p,
                                           SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_ACTIVATE, 
                                           FBE_PROVISION_DRIVE_SUBSTATE_ENCRYPTION_BAD_KEY_HANDLE, 
                                           0,  &hook_status);
        } else if (metadata_status == FBE_PAYLOAD_METADATA_STATUS_ENCRYPTION_NOT_ENABLED) {
            /* For these cases we will tolerate the error, but put the PVD into a state where we will not 
             * retry unless a new key is pushed. 
             */
            fbe_provision_drive_set_encryption_state(provision_drive_p,
                                                     0, /* Client 0 for paged. */
                                                     FBE_BASE_CONFIG_ENCRYPTION_STATE_LOCKED_KEY_ERROR);
            fbe_provision_drive_check_hook(provision_drive_p,
                                           SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_ACTIVATE, 
                                           FBE_PROVISION_DRIVE_SUBSTATE_ENCRYPTION_NOT_ENABLED, 
                                           0,  &hook_status);
        } else if (metadata_status == FBE_PAYLOAD_METADATA_STATUS_KEY_WRAP_ERROR) {
            /* For these cases we will tolerate the error, but put the PVD into a state where we will not 
             * retry unless a new key is pushed. 
             */
            fbe_provision_drive_set_encryption_state(provision_drive_p,
                                                     0, /* Client 0 for paged. */
                                                     FBE_BASE_CONFIG_ENCRYPTION_STATE_LOCKED_KEY_ERROR);
            fbe_provision_drive_check_hook(provision_drive_p,
                                           SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_ACTIVATE, 
                                           FBE_PROVISION_DRIVE_SUBSTATE_ENCRYPTION_KEY_WRAP_ERROR, 
                                           0,  &hook_status);
        }


        fbe_base_object_trace((fbe_base_object_t*)provision_drive_p,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "provision drive write default paged: metadata failed, status:0x%x, metadata_status:0x%x.\n",
                              status, metadata_status);
        status = (status != FBE_STATUS_OK) ? status : FBE_STATUS_GENERIC_FAILURE;
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, status, 0);
    }
    else
    {
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_OK);
    }

    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_provision_drive_metadata_write_default_paged_metadata_completion()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_provision_drive_write_default_system_paged()
 ******************************************************************************
 * @brief
 *   This function is used to initialize the metadata with default values for 
 *   the non paged and paged metadata. Before we initialize it we have to
 *   zero out this area.
 *
 * @param provision_drive_p         - pointer to the provision drive
 * @param packet_p                  - pointer to a monitor packet.
 *
 * @return  fbe_status_t  
 *
 * @author
 *  2/12/2014 - Created. Rob Foley
 *
 ******************************************************************************/
fbe_status_t 
fbe_provision_drive_write_default_system_paged(fbe_provision_drive_t * provision_drive_p,
                                               fbe_packet_t *packet_p)
{
    fbe_status_t                                    status = FBE_STATUS_GENERIC_FAILURE; 
    fbe_payload_ex_t *                              sep_payload_p = NULL;
    fbe_payload_control_operation_t *               control_operation_p = NULL;
    fbe_memory_request_t *                          memory_request_p = NULL;

    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(sep_payload_p);

    /* allocate memory for client_blob */
    memory_request_p = fbe_transport_get_memory_request(packet_p);
    fbe_memory_build_chunk_request(memory_request_p,
                                   FBE_MEMORY_CHUNK_SIZE_FOR_64_BLOCKS_IO,
                                   2, /* number of chunks */
                                   fbe_transport_get_resource_priority(packet_p),
                                   fbe_transport_get_io_stamp(packet_p),
                                   (fbe_memory_completion_function_t)fbe_provision_drive_write_default_system_paged_alloc_completion, 
                                   provision_drive_p);
    fbe_transport_memory_request_set_io_master(memory_request_p, packet_p);

    /* Issue the memory request to memory service. */
    status = fbe_memory_request_entry(memory_request_p);
    if((status != FBE_STATUS_OK) && (status != FBE_STATUS_PENDING))
    {
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s memory allocation failed",  __FUNCTION__);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return status;
    }

    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/******************************************************************************
 * end fbe_provision_drive_write_default_system_paged()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_provision_drive_init_paged_metadata_callback()
 ******************************************************************************
 * @brief
 *   This function is the callback from the metadata service when it has
 *   finished reading a piece of paged.
 *   We will simply update the paged to look freshly initialized.
 * 
 * @param packet_p           - pointer to the packet 
 * @param raid_group_p       - pointer to a raid group object
 *
 * @return  fbe_status_t  
 *
 * @author
 *  2/10/2014 - Created. Rob Foley
 *
 ******************************************************************************/
static fbe_status_t 
fbe_provision_drive_init_paged_metadata_callback(fbe_packet_t * packet, fbe_metadata_callback_context_t context)
{
    fbe_provision_drive_t                   *provision_drive_p;
    fbe_payload_ex_t                   *sep_payload = fbe_transport_get_payload_ex(packet);
    fbe_payload_metadata_operation_t   *mdo = NULL;
    fbe_provision_drive_paged_metadata_t    *paged_metadata_p;
    fbe_sg_element_t                   *sg_list = NULL;
    fbe_sg_element_t                   *sg_ptr = NULL;
    fbe_lba_t                           lba_offset;
    fbe_u64_t                           slot_offset;

    /* Get the metadata operation
     */
    if(sep_payload->current_operation->payload_opcode == FBE_PAYLOAD_OPCODE_METADATA_OPERATION){
        mdo = fbe_payload_ex_get_metadata_operation(sep_payload);
    } else if(sep_payload->current_operation->payload_opcode == FBE_PAYLOAD_OPCODE_STRIPE_LOCK_OPERATION){
        mdo = fbe_payload_ex_get_any_metadata_operation(sep_payload);
    }
    if ((mdo == NULL) || (mdo->metadata_element == NULL)){
        /* The metadata code will complete a packet in error.
         */
        fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR,
                                 FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                 "%s NULL metadata operation\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Get the object from the metadata element.
     */
    provision_drive_p = (fbe_provision_drive_t *)fbe_base_config_metadata_element_to_base_config_object(mdo->metadata_element);

    lba_offset = fbe_metadata_paged_get_lba_offset(mdo);
    slot_offset = fbe_metadata_paged_get_slot_offset(mdo);
    sg_list = mdo->u.metadata_callback.sg_list;
    sg_ptr = &sg_list[lba_offset - mdo->u.metadata_callback.start_lba];
    paged_metadata_p = (fbe_provision_drive_paged_metadata_t *)(sg_ptr->address + slot_offset);

    /* For each chunk make it look initialized.
     */
    while ((sg_ptr->address != NULL) && 
           (mdo->u.metadata_callback.current_count_private < mdo->u.metadata_callback.repeat_count)) {

        /* Set default values.
         */
        paged_metadata_p->need_zero_bit = 1; 
        paged_metadata_p->consumed_user_data_bit = 0;
        paged_metadata_p->user_zero_bit = 0;
        paged_metadata_p->valid_bit = 1; 
        paged_metadata_p->unused_bit = 0;

        /* Perform required book-keeping
         */
        mdo->u.metadata_callback.current_count_private++;
        paged_metadata_p++;
        if (paged_metadata_p == (fbe_provision_drive_paged_metadata_t *)(sg_ptr->address + FBE_METADATA_BLOCK_DATA_SIZE)) {
            sg_ptr++;
            paged_metadata_p = (fbe_provision_drive_paged_metadata_t *)sg_ptr->address;
        }
    }

    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/********************************************************************
 * end fbe_provision_drive_init_paged_metadata_callback()
 ********************************************************************/
/*!****************************************************************************
 * fbe_provision_drive_write_default_system_paged_alloc_completion()
 ******************************************************************************
 * @brief
 *  This is the completion for memory allocation for paged blob.
 *
 * @param memory_request_p         - pointer to the memory request
 * @param context                  - context.
 *
 * @return  fbe_status_t  
 *
 * @author
 *  2/10/2014 - Created. Rob Foley
 *
 ******************************************************************************/
static fbe_status_t
fbe_provision_drive_write_default_system_paged_alloc_completion(fbe_memory_request_t * memory_request_p, 
                                                                     fbe_memory_completion_context_t context)
{
    fbe_status_t                                    status = FBE_STATUS_GENERIC_FAILURE; 
    fbe_u64_t                                       repeat_count = 0;
    fbe_payload_ex_t *                              sep_payload_p = NULL;
    fbe_payload_metadata_operation_t *              metadata_operation_p = NULL;
    fbe_payload_control_operation_t *               control_operation_p = NULL;
    fbe_lba_t                                       paged_metadata_capacity = 0;
    fbe_packet_t *                                  packet_p;
    fbe_provision_drive_t *                         provision_drive_p = (fbe_provision_drive_t *) context;
    fbe_lba_t                                       metadata_offset;
    fbe_lba_t                                       default_offset;
    fbe_lba_t                                       capacity;
    fbe_chunk_index_t                               chunk_index;
    fbe_chunk_count_t                               chunk_count;
    fbe_lba_t                                       metadata_start_lba;
    fbe_block_count_t                               metadata_block_count;

    /* get the payload and control operation. */
    packet_p = fbe_transport_memory_request_to_packet(memory_request_p);
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(sep_payload_p);

    /* Check allocation status */
    if (fbe_memory_request_is_allocation_complete(memory_request_p) == FBE_FALSE){
        /* Currently this callback doesn't expect any aborted requests */
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p, 
                                        FBE_TRACE_LEVEL_ERROR,
                                        FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                        "%s memory allocation failed, state:%d\n",
                                        __FUNCTION__, memory_request_p->request_state);

        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        return FBE_STATUS_GENERIC_FAILURE;
    }
   

    fbe_base_config_metadata_get_paged_metadata_capacity((fbe_base_config_t *)provision_drive_p,
                                                         &paged_metadata_capacity);
    fbe_base_config_get_capacity((fbe_base_config_t *)provision_drive_p,
                                 &capacity);
    
    /* On a system drive we can only zero out the area above the default offset.
     */
    fbe_base_config_get_default_offset((fbe_base_config_t *)provision_drive_p, &default_offset);
    fbe_provision_drive_utils_calculate_chunk_range_without_edges(default_offset,
                                                                  capacity - default_offset,
                                                                  FBE_PROVISION_DRIVE_CHUNK_SIZE,
                                                                  &chunk_index,
                                                                  &chunk_count);
    metadata_offset = chunk_index * sizeof(fbe_provision_drive_paged_metadata_t);
    repeat_count = chunk_count * sizeof(fbe_provision_drive_paged_metadata_t);

    /* Allocate the metadata operation. */
    metadata_operation_p =  fbe_payload_ex_allocate_metadata_operation(sep_payload_p);

    status = fbe_payload_metadata_build_paged_update(metadata_operation_p, 
                                                    &(((fbe_base_config_t *) provision_drive_p)->metadata_element),
                                                     metadata_offset, 
                                                     sizeof(fbe_provision_drive_paged_metadata_t),
                                                     chunk_count,
                                                     fbe_provision_drive_init_paged_metadata_callback,
                                                     (void *)metadata_operation_p);
    if(status != FBE_STATUS_OK) {
        fbe_memory_free_request_entry(memory_request_p);
        fbe_payload_ex_release_metadata_operation(sep_payload_p, metadata_operation_p);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, status, 0);
        return status;
    }

    /* Initialize cient_blob */
    status = fbe_metadata_paged_init_client_blob(memory_request_p, metadata_operation_p, 0, FBE_FALSE);

    /* set the completion function for the default paged metadata write. */
    fbe_transport_set_completion_function(packet_p, fbe_provision_drive_write_default_system_paged_completion, provision_drive_p);

    fbe_payload_ex_increment_metadata_operation_level(sep_payload_p);

    fbe_metadata_paged_get_lock_range(metadata_operation_p, &metadata_start_lba, &metadata_block_count);

    fbe_payload_metadata_set_metadata_stripe_offset(metadata_operation_p, metadata_start_lba);
    fbe_payload_metadata_set_metadata_stripe_count(metadata_operation_p, metadata_block_count);

    /* Issue the metadata operation. */
    fbe_metadata_operation_entry(packet_p);
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;

    
}
/******************************************************************************
 * end fbe_provision_drive_write_default_system_paged_alloc_completion()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_provision_drive_write_default_system_paged_completion()
 ******************************************************************************
 * @brief
 *  This function is used to handle the comletion of the default paged metadata
 *  initialization.
 *
 * @param provision_drive_p         - pointer to the provision drive
 * @param packet_p                  - pointer to a monitor packet.
 *
 * @return  fbe_status_t  
 *
 * @author
 *   2/12/2014 - Created. Rob Foley
 *
 ******************************************************************************/
static fbe_status_t
fbe_provision_drive_write_default_system_paged_completion(fbe_packet_t * packet_p,
                                                          fbe_packet_completion_context_t context)
{
    fbe_provision_drive_t *                 provision_drive_p = NULL;
    fbe_payload_metadata_operation_t *      metadata_operation_p = NULL;
    fbe_payload_control_operation_t *       control_operation_p = NULL;
    fbe_payload_ex_t *                     sep_payload_p = NULL;
    fbe_status_t                            status;
    fbe_payload_metadata_status_t           metadata_status;
    fbe_payload_metadata_operation_opcode_t metadata_opcode;
    fbe_memory_request_t *                  memory_request = NULL;
    fbe_scheduler_hook_status_t             hook_status;
    
    provision_drive_p = (fbe_provision_drive_t *) context;

    /* get the payload and metadata operation. */
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    metadata_operation_p =  fbe_payload_ex_get_metadata_operation(sep_payload_p);
    control_operation_p = fbe_payload_ex_get_prev_control_operation(sep_payload_p);
    
    /* get the status of the paged metadata operation. */
    fbe_payload_metadata_get_status(metadata_operation_p, &metadata_status);
    status = fbe_transport_get_status_code(packet_p);

    fbe_payload_metadata_get_opcode(metadata_operation_p, &metadata_opcode);

    /* release the metadata operation. */
    fbe_payload_ex_release_metadata_operation(sep_payload_p, metadata_operation_p);

    /* release client_blob */
    memory_request = fbe_transport_get_memory_request(packet_p);
    fbe_memory_free_request_entry(memory_request);

    if((status != FBE_STATUS_OK) ||
       (metadata_status != FBE_PAYLOAD_METADATA_STATUS_OK))
    {
        if (metadata_status == FBE_PAYLOAD_METADATA_STATUS_BAD_KEY_HANDLE){
            /* We decided to critical on this error since it is a software bug.
             */
            fbe_base_object_trace((fbe_base_object_t*)provision_drive_p, FBE_TRACE_LEVEL_CRITICAL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                  "provision drive write default system paged: Key not found error status:0x%x, metadata_status:0x%x.\n",
                                  status, metadata_status);

            fbe_provision_drive_check_hook(provision_drive_p,
                                           SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_ACTIVATE, 
                                           FBE_PROVISION_DRIVE_SUBSTATE_ENCRYPTION_BAD_KEY_HANDLE, 
                                           0,  &hook_status);
        } else if (metadata_status == FBE_PAYLOAD_METADATA_STATUS_ENCRYPTION_NOT_ENABLED) {
            /* For these cases we will tolerate the error, but put the PVD into a state where we will not 
             * retry unless a new key is pushed. 
             */
            fbe_provision_drive_set_encryption_state(provision_drive_p,
                                                     0, /* Client 0 for paged. */
                                                     FBE_BASE_CONFIG_ENCRYPTION_STATE_LOCKED_KEY_ERROR);
            fbe_provision_drive_check_hook(provision_drive_p,
                                           SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_ACTIVATE, 
                                           FBE_PROVISION_DRIVE_SUBSTATE_ENCRYPTION_NOT_ENABLED, 
                                           0,  &hook_status);
        } else if (metadata_status == FBE_PAYLOAD_METADATA_STATUS_KEY_WRAP_ERROR) {
            /* For these cases we will tolerate the error, but put the PVD into a state where we will not 
             * retry unless a new key is pushed. 
             */
            fbe_provision_drive_set_encryption_state(provision_drive_p,
                                                     0, /* Client 0 for paged. */
                                                     FBE_BASE_CONFIG_ENCRYPTION_STATE_LOCKED_KEY_ERROR);
            fbe_provision_drive_check_hook(provision_drive_p,
                                           SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_ACTIVATE, 
                                           FBE_PROVISION_DRIVE_SUBSTATE_ENCRYPTION_KEY_WRAP_ERROR, 
                                           0,  &hook_status);
        }
        fbe_base_object_trace((fbe_base_object_t*)provision_drive_p,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "provision drive write default system paged: failed, status:0x%x, metadata_status:0x%x.\n",
                              status, metadata_status);
        status = (status != FBE_STATUS_OK) ? status : FBE_STATUS_GENERIC_FAILURE;
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, status, 0);
    }
    else
    {
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_OK);
    }

    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_provision_drive_write_default_system_paged_completion()
 ******************************************************************************/

/*!****************************************************************************
 * @fn fbe_provision_drive_metadata_write_verify_paged_metadata_callback()
 ******************************************************************************
 * @brief
 *
 *  This is the callback function to clear bits for disk zeroing. 
 * 
 * @param packet            - Pointer to packet.
 * @param context           - Pointer to context.
 *
 * @return fbe_status_t.
 *  05/06/2014 - Created. Lili Chen
 *
 ******************************************************************************/
fbe_status_t 
fbe_provision_drive_metadata_write_verify_paged_metadata_callback(fbe_packet_t * packet, fbe_metadata_callback_context_t context)
{
#if 0
    fbe_payload_ex_t * sep_payload = fbe_transport_get_payload_ex(packet);
    fbe_payload_metadata_operation_t * mdo = NULL;
    fbe_provision_drive_paged_metadata_t * paged_metadata_p;
    fbe_sg_element_t * sg_list = NULL;
    fbe_sg_element_t * sg_ptr = NULL;
    fbe_lba_t lba_offset;
    fbe_u64_t slot_offset;
    fbe_provision_drive_paged_metadata_t  paged_set_bits;

    if(sep_payload->current_operation->payload_opcode == FBE_PAYLOAD_OPCODE_METADATA_OPERATION){
        mdo = fbe_payload_ex_get_metadata_operation(sep_payload);
    } else if(sep_payload->current_operation->payload_opcode == FBE_PAYLOAD_OPCODE_STRIPE_LOCK_OPERATION){
        mdo = fbe_payload_ex_get_any_metadata_operation(sep_payload);
    } else {
        fbe_topology_class_trace(FBE_CLASS_ID_PROVISION_DRIVE, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                      "%s: Invalid payload opcode:%d \n", __FUNCTION__, sep_payload->current_operation->payload_opcode);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    lba_offset = fbe_metadata_paged_get_lba_offset(mdo);
    slot_offset = fbe_metadata_paged_get_slot_offset(mdo);
    sg_list = mdo->u.metadata_callback.sg_list;
    sg_ptr = &sg_list[lba_offset - mdo->u.metadata_callback.start_lba];
    paged_metadata_p = (fbe_provision_drive_paged_metadata_t *)(sg_ptr->address + slot_offset);

    fbe_zero_memory(&paged_set_bits, sizeof(fbe_provision_drive_paged_metadata_t));
    paged_set_bits.valid_bit = 1;
    paged_set_bits.consumed_user_data_bit = 1;

    while ((sg_ptr->address != NULL) && (mdo->u.metadata_callback.current_count_private < mdo->u.metadata_callback.repeat_count))
    {
        ((fbe_u8_t *)paged_metadata_p)[0] = ((fbe_u8_t *)&paged_set_bits)[0];

        mdo->u.metadata_callback.current_count_private++;
        paged_metadata_p++;
        if (paged_metadata_p == (fbe_provision_drive_paged_metadata_t *)(sg_ptr->address + FBE_METADATA_BLOCK_DATA_SIZE))
        {
            sg_ptr++;
            paged_metadata_p = (fbe_provision_drive_paged_metadata_t *)sg_ptr->address;
        }
    }

    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
#endif
    fbe_provision_drive_paged_metadata_t paged_metadata;

    fbe_zero_memory(&paged_metadata, sizeof(fbe_provision_drive_paged_metadata_t));
    paged_metadata.valid_bit = 1;
    paged_metadata.consumed_user_data_bit = 1;

    return (fbe_provision_drive_metadata_paged_callback_write(packet, &paged_metadata));
}
/******************************************************************************
 * end fbe_provision_drive_metadata_write_verify_paged_metadata_callback()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_provision_drive_metadata_write_paged_metadata_chunk()
 ******************************************************************************
 * @brief
 *   This function is used to write the specified data to the entire paged metadata 
 *   chunk 
 *
 * @param provision_drive_p         - pointer to the provision drive
 * @param packet_p                  - pointer to a monitor packet.
 * @param paged_set_bits            - Buffer that contains data to be written
 * @param chunk_index               - Chunk index that needs to be written
 * @param verify_required           - Indicates if we need to do a write verify
 *
 * @return  fbe_status_t  
 *
 * @author
 *   03/01/2012 - Created. Ashok Tamilarasan
 *
 ******************************************************************************/
fbe_status_t 
fbe_provision_drive_metadata_write_paged_metadata_chunk(fbe_provision_drive_t * provision_drive_p,
                                                        fbe_packet_t *packet_p,
                                                        fbe_provision_drive_paged_metadata_t *paged_set_bits,
                                                        fbe_chunk_index_t chunk_index,
                                                        fbe_bool_t verify_required)
{
    fbe_status_t                                    status = FBE_STATUS_GENERIC_FAILURE; 
    fbe_u64_t                                       repeat_count = 0;
    fbe_payload_metadata_operation_t *              metadata_operation_p = NULL;
    fbe_u64_t                                       metadata_offset;
    fbe_payload_ex_t *                              sep_payload_p = NULL;
    fbe_lba_t                                       metadata_start_lba;
    fbe_block_count_t                               metadata_block_count;
    

    /* get the payload and block operation. */
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);

    /* We want to write out the entire chunk */
    repeat_count = (FBE_PROVISION_DRIVE_CHUNK_SIZE * FBE_METADATA_BLOCK_DATA_SIZE)/sizeof(fbe_provision_drive_paged_metadata_t);

    metadata_offset = repeat_count * chunk_index;

    /* Allocate the metadata operation. */
    metadata_operation_p =  fbe_payload_ex_allocate_metadata_operation(sep_payload_p);

    /* Build the paged metadata write request. */
#if 0
    if(verify_required)
    {
        status = fbe_payload_metadata_build_paged_write_verify(metadata_operation_p, 
                                                               &(((fbe_base_config_t *) provision_drive_p)->metadata_element),
                                                               metadata_offset,
                                                               (fbe_u8_t *) paged_set_bits,
                                                               sizeof(fbe_provision_drive_paged_metadata_t),
                                                               repeat_count);
    }
    else
    {
           status = fbe_payload_metadata_build_paged_write(metadata_operation_p, 
                                                        &(((fbe_base_config_t *) provision_drive_p)->metadata_element),
                                                        metadata_offset,
                                                        (fbe_u8_t *) paged_set_bits,
                                                        sizeof(fbe_provision_drive_paged_metadata_t),
                                                        repeat_count);
    }
#else
    status = fbe_payload_metadata_build_paged_update(metadata_operation_p,
                                                     &(((fbe_base_config_t *) provision_drive_p)->metadata_element),
                                                     metadata_offset,
                                                     sizeof(fbe_provision_drive_paged_metadata_t),
                                                     repeat_count,
                                                     fbe_provision_drive_metadata_write_verify_paged_metadata_callback,
                                                     (void *)metadata_operation_p);
    if(verify_required)
    {
        fbe_payload_metadata_set_metadata_operation_flags(metadata_operation_p, FBE_PAYLOAD_METADATA_OPERATION_PAGED_WRITE_VERIFY);
    }
    else
    {
        fbe_payload_metadata_set_metadata_operation_flags(metadata_operation_p, FBE_PAYLOAD_METADATA_OPERATION_PAGED_WRITE);
    }
    /* Initialize cient_blob, skip the subpacket */
    //fbe_provision_drive_metadata_paged_init_client_blob(packet_p, metadata_operation_p);
#endif

    if(status != FBE_STATUS_OK)
    {
        fbe_payload_ex_release_metadata_operation(sep_payload_p, metadata_operation_p);
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return status;
    }

    /* Get the metadata stripe lock*/
   fbe_metadata_paged_get_lock_range(metadata_operation_p, &metadata_start_lba, &metadata_block_count);

   fbe_payload_metadata_set_metadata_stripe_offset(metadata_operation_p, metadata_start_lba);
   fbe_payload_metadata_set_metadata_stripe_count(metadata_operation_p, metadata_block_count);

    /* set the completion function for the default paged metadata write. */
    fbe_transport_set_completion_function(packet_p, fbe_provision_drive_metadata_write_paged_metadata_chunk_completion, provision_drive_p);

    fbe_payload_ex_increment_metadata_operation_level(sep_payload_p);

    /* Issue the metadata operation. */
    fbe_metadata_operation_entry(packet_p);
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;

    
}
/******************************************************************************
 * end fbe_provision_drive_metadata_write_paged_metadata_chunk()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_provision_drive_metadata_write_paged_metadata_chunk_completion()
 ******************************************************************************
 * @brief
 *  This function is used to handle the comletion of the default paged metadata
 *  initialization.
 *
 * @param provision_drive_p         - pointer to the provision drive
 * @param packet_p                  - pointer to a monitor packet.
 *
 * @return  fbe_status_t  
 *
 * @author
 *   03/01/2012 - Created. Ashok Tamilarasan
 *
 ******************************************************************************/
static fbe_status_t
fbe_provision_drive_metadata_write_paged_metadata_chunk_completion(fbe_packet_t * packet_p,
                                                                     fbe_packet_completion_context_t context)
{
    fbe_provision_drive_t *                 provision_drive_p = NULL;
    fbe_payload_metadata_operation_t *      metadata_operation_p = NULL;
    fbe_payload_ex_t *                     sep_payload_p = NULL;
    fbe_status_t                            status;
    fbe_payload_metadata_status_t           metadata_status;
    fbe_payload_metadata_operation_opcode_t metadata_opcode;
    
    provision_drive_p = (fbe_provision_drive_t *) context;

    /* get the payload and metadata operation. */
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    metadata_operation_p =  fbe_payload_ex_get_metadata_operation(sep_payload_p);
    
    /* get the status of the paged metadata operation. */
    fbe_payload_metadata_get_status(metadata_operation_p, &metadata_status);
    status = fbe_transport_get_status_code(packet_p);

    fbe_payload_metadata_get_opcode(metadata_operation_p, &metadata_opcode);

    /* release the metadata operation. */
    fbe_payload_ex_release_metadata_operation(sep_payload_p, metadata_operation_p);

    if((status != FBE_STATUS_OK) ||
       (metadata_status != FBE_PAYLOAD_METADATA_STATUS_OK))
    {
        fbe_base_object_trace((fbe_base_object_t*)provision_drive_p,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s Write failed, status:0x%x, MD status:0x%x.\n",
                              __FUNCTION__, status, metadata_status);
        status = (status != FBE_STATUS_OK) ? status : FBE_STATUS_GENERIC_FAILURE;
        fbe_transport_set_status(packet_p, status, 0);
    }
    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_provision_drive_metadata_write_paged_metadata_chunk_completion()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_provision_drive_metadata_write_default_nonpaged_metadata()
 ******************************************************************************
 * @brief
 *   This function is used to initialize the metadata with default values for 
 *   the non paged and paged metadata.
 *
 * @param provision_drive_p         - pointer to the provision drive
 * @param packet_p                  - pointer to a monitor packet.
 *
 * @return  fbe_status_t  
 *
 * @author
 *   03/01/2010 - Created. Dhaval Patel
 *
 ******************************************************************************/
fbe_status_t 
fbe_provision_drive_metadata_write_default_nonpaged_metadata(fbe_provision_drive_t *provision_drive_p,
                                                             fbe_packet_t *packet_p)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_bool_t                              b_is_active;
    fbe_provision_drive_nonpaged_metadata_t provision_drive_nonpaged_metadata;
    fbe_lba_t                               default_offset;
    fbe_object_id_t                         object_id;
    static fbe_bool_t   b_enable_background_zeroing = FBE_TRUE;
    static fbe_bool_t   b_enable_system_drive_background_zeroing = FBE_TRUE;
    
    /*! @note We should not be here if this is not the active SP.
     */
    b_is_active = fbe_base_config_is_active((fbe_base_config_t *)provision_drive_p);
    if (b_is_active == FBE_FALSE)
    {
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p,
                              FBE_TRACE_LEVEL_ERROR, 
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s Unexpected passive set.\n",
                              __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* We must zero the local structure since it will be used to set all the
     * default metadata values.
     */
    fbe_zero_memory(&provision_drive_nonpaged_metadata, sizeof(fbe_provision_drive_nonpaged_metadata_t));

    /* Now set the default non-paged metadata.
     */
    status = fbe_base_config_set_default_nonpaged_metadata((fbe_base_config_t *)provision_drive_p,
                                                           (fbe_base_config_nonpaged_metadata_t *)&provision_drive_nonpaged_metadata);
    if (status != FBE_STATUS_OK)
    {
        /* Trace an error and return.
         */
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p,
                              FBE_TRACE_LEVEL_ERROR, 
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s Set default base config nonpaged failed status: 0x%x\n",
                              __FUNCTION__, status);

        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);

        return status;
    }	

    /* Find where the drive really starts.
     */
    fbe_base_config_get_default_offset((fbe_base_config_t *)provision_drive_p, &default_offset);

    /* Get pvd object id. 
     */
    fbe_base_object_get_object_id((fbe_base_object_t *) provision_drive_p, &object_id);

    /* Initialize the nonpaged metadata with default values. 
     */
    provision_drive_nonpaged_metadata.sniff_verify_checkpoint = 0;
    provision_drive_nonpaged_metadata.pass_count = 0;
    provision_drive_nonpaged_metadata.zero_on_demand = FBE_TRUE;
    provision_drive_nonpaged_metadata.end_of_life_state = FBE_FALSE;
    provision_drive_nonpaged_metadata.drive_fault_state = FBE_FALSE;
    provision_drive_nonpaged_metadata.media_error_lba = FBE_LBA_INVALID;
    provision_drive_nonpaged_metadata.verify_invalidate_checkpoint = FBE_LBA_INVALID;
    provision_drive_nonpaged_metadata.nonpaged_drive_info.port_number = FBE_PORT_NUMBER_INVALID;
    provision_drive_nonpaged_metadata.nonpaged_drive_info.enclosure_number = FBE_ENCLOSURE_NUMBER_INVALID;
    provision_drive_nonpaged_metadata.nonpaged_drive_info.slot_number = FBE_SLOT_NUMBER_INVALID;
    provision_drive_nonpaged_metadata.nonpaged_drive_info.drive_type = FBE_DRIVE_TYPE_INVALID;
    /* default time stamp is zero */
    fbe_zero_memory(&provision_drive_nonpaged_metadata.remove_timestamp,sizeof(fbe_system_time_t));
    /* set default zero checkpoint */
    if(fbe_database_is_object_system_pvd(object_id))    
    {
        /* for system drive pvd, set the default zero checkpoint to 0th LBA so that background zeroing operation can
         * zero the system luns and system raid groups area.
         */
        provision_drive_nonpaged_metadata.zero_checkpoint = 0;
    }
    else
    {
        /* for non system drive pvd, set the default zero checkpoint to the default offset because background zeroing
         * does not need to touch the private space below the default offset          
         */
        provision_drive_nonpaged_metadata.zero_checkpoint = default_offset;
    }

    /* Set all background operations by default as enabled in provision drive */
    provision_drive_nonpaged_metadata.base_config_nonpaged_metadata.operation_bitmask = 0;
    provision_drive_nonpaged_metadata.validate_area_bitmap = 0;

    /*!@todo Need to remove following code once we enable background zeroing by default
     */
    if (b_enable_background_zeroing == FBE_FALSE)
    {
        /*! @todo Disable background zeroing for all drives.
         *
         *  @note Setting the bit actually disables the background operation!!
         */
        provision_drive_nonpaged_metadata.base_config_nonpaged_metadata.operation_bitmask |= FBE_PROVISION_DRIVE_BACKGROUND_OP_DISK_ZEROING;
    }
    else if (b_enable_system_drive_background_zeroing == FBE_FALSE)
    {
        /* disable background zeroing for system drives */
        if(fbe_database_is_object_system_pvd(object_id))    
        {
            provision_drive_nonpaged_metadata.base_config_nonpaged_metadata.operation_bitmask |= FBE_PROVISION_DRIVE_BACKGROUND_OP_DISK_ZEROING;
            fbe_base_object_trace((fbe_base_object_t*)provision_drive_p,
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "pvd: background zeroing disabled for system obj: 0x%x default offset: 0x%llx\n",
                                  object_id, (unsigned long long)default_offset);
        }
    } 

    /* Send the nonpaged metadata write request to metadata service. */
    status = fbe_base_config_metadata_nonpaged_write((fbe_base_config_t *) provision_drive_p,
                                                     packet_p,
                                                     0,
                                                     (fbe_u8_t *)&provision_drive_nonpaged_metadata, /* non paged metadata memory. */
                                                     sizeof(fbe_provision_drive_nonpaged_metadata_t)); /* size of the non paged metadata. */
    return status;
}
/******************************************************************************
 * end fbe_provision_drive_metadata_write_default_nonpaged_metadata()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_provision_drive_metadata_get_background_zero_checkpoint()
 ******************************************************************************
 * @brief
 *   This function is used to get the background zeroing checkpoint from the
 *   non paged metadata.
 *
 * @param provision_drive_p         - pointer to the provision drive
 * @param packet_p                  - pointer to a monitor packet.
 * @param zero_checkpoint_p         - pointer to the zero checkpoint.
 *
 * @return  fbe_status_t  
 *
 * @author
 *   12/16/2009 - Created. Peter Puhov
 *
 ******************************************************************************/
fbe_status_t 
fbe_provision_drive_metadata_get_background_zero_checkpoint(fbe_provision_drive_t * provision_drive,
                                                            fbe_lba_t * zero_checkpoint_p)
{
    fbe_provision_drive_nonpaged_metadata_t * provision_drive_nonpaged_metadata = NULL;

    fbe_base_config_get_nonpaged_metadata_ptr((fbe_base_config_t *) provision_drive, (void **)&provision_drive_nonpaged_metadata);

    if (provision_drive_nonpaged_metadata == NULL) 
    {
        fbe_base_object_trace((fbe_base_object_t *)provision_drive,
                              FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                              "pvd_md_get_bgz_checkpoint: non-paged MetaData is NULL for PVD Object.\n");

        return FBE_STATUS_GENERIC_FAILURE;
    }

    *zero_checkpoint_p = provision_drive_nonpaged_metadata->zero_checkpoint;

    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_provision_drive_metadata_get_background_zero_checkpoint()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_provision_drive_metadata_set_background_zero_checkpoint()
 ******************************************************************************
 * @brief
 *   This function is used to set the background zeroing checkpoint in 
 *   non paged metadata.
 *
 * @param provision_drive_p         - pointer to the provision drive
 * @param packet_p                  - pointer to a monitor packet.
 * @param zero_checkpoint           - zero checkpoint.
 *
 * @return  fbe_status_t  
 *
 * @author
 *   12/16/2009 - Created. Peter Puhov
 *
 ******************************************************************************/
fbe_status_t 
fbe_provision_drive_metadata_set_background_zero_checkpoint(fbe_provision_drive_t * provision_drive_p, 
                                                            fbe_packet_t * packet_p,
                                                            fbe_lba_t zero_checkpoint)
{
    fbe_status_t    status;
    fbe_lba_t       exported_capacity = FBE_LBA_INVALID;

    /* get the paged metadatas start lba. */
    fbe_base_config_get_capacity((fbe_base_config_t *) provision_drive_p, &exported_capacity);

    /* check if disk zeroing completed */
    if((zero_checkpoint != FBE_LBA_INVALID) &&
       (zero_checkpoint  > exported_capacity))
    {
        /* disk zeroing completed */
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s:checkpoint cannot be updated beyond capacity, zero_chk:0x%llx, export_cap:0x%llx\n",
                              __FUNCTION__,
                  (unsigned long long)zero_checkpoint,
                  (unsigned long long)exported_capacity);
        zero_checkpoint = FBE_LBA_INVALID;
    }


    status = fbe_base_config_metadata_nonpaged_write_persist((fbe_base_config_t *) provision_drive_p,
                                                             packet_p,
                                                             (fbe_u64_t)(&((fbe_provision_drive_nonpaged_metadata_t*)0)->zero_checkpoint),
                                                             (fbe_u8_t *)&zero_checkpoint,
                                                             sizeof(fbe_lba_t));

    /* Inform metadata cache for the checkpoint changes */
    if (((fbe_base_config_t *) provision_drive_p)->metadata_element.cache_callback) {
        (((fbe_base_config_t *) provision_drive_p)->metadata_element.cache_callback)(
                FBE_METADATA_PAGED_CACHE_ACTION_NONPAGE_CHANGED, &((fbe_base_config_t *) provision_drive_p)->metadata_element, NULL, 0, 0);
    }

    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/******************************************************************************
 * end fbe_provision_drive_metadata_set_background_zero_checkpoint()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_provision_drive_metadata_get_verify_invalidate_checkpoint()
 ******************************************************************************
 * @brief
 *   This function is used to get the metadata verify checkpoint from the
 *   non paged metadata.
 *
 * @param provision_drive_p         - pointer to the provision drive
 * @param packet_p                  - pointer to a monitor packet.
 * @param verify_invalidate_checkpoint_p         - pointer to the metadata verify checkpoint.
 *
 * @return  fbe_status_t  
 *
 * @author
 *   03/09/2012 - Created. Ashok Tamilarasan
 *
 ******************************************************************************/
fbe_status_t 
fbe_provision_drive_metadata_get_verify_invalidate_checkpoint(fbe_provision_drive_t * provision_drive,
                                                            fbe_lba_t * verify_invalidate_checkpoint_p)
{
    fbe_provision_drive_nonpaged_metadata_t * provision_drive_nonpaged_metadata = NULL;

    fbe_base_config_get_nonpaged_metadata_ptr((fbe_base_config_t *) provision_drive, (void **)&provision_drive_nonpaged_metadata);

    if (provision_drive_nonpaged_metadata == NULL) 
    {
        fbe_base_object_trace((fbe_base_object_t *)provision_drive,
                              FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s, non-paged MetaData is NULL.\n", __FUNCTION__);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    *verify_invalidate_checkpoint_p = provision_drive_nonpaged_metadata->verify_invalidate_checkpoint;

    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_provision_drive_metadata_get_verify_invalidate_checkpoint()
 ******************************************************************************/
/*!****************************************************************************
 * fbe_provision_drive_metadata_set_verify_invalidate_checkpoint()
 ******************************************************************************
 * @brief
 *   This function is used to set the metadata verify checkpoint in 
 *   non paged metadata.
 *
 * @param provision_drive_p         - pointer to the provision drive
 * @param packet_p                  - pointer to a monitor packet.
 * @param verify_checkpoint_p       - metadata verify checkpoint.
 *
 * @return  fbe_status_t  
 *
 * @author
 *   03/09/2012 - Created. Ashok Tamilarasan
 *
 ******************************************************************************/
fbe_status_t 
fbe_provision_drive_metadata_set_verify_invalidate_checkpoint(fbe_provision_drive_t * provision_drive_p, 
                                                            fbe_packet_t * packet_p,
                                                            fbe_lba_t verify_checkpoint_p)
{
    fbe_status_t    status;
    fbe_lba_t       exported_capacity = FBE_LBA_INVALID;

    /* get the paged metadatas start lba. */
    fbe_base_config_get_capacity((fbe_base_config_t *) provision_drive_p, &exported_capacity);

    /* check if disk zeroing completed */
    if((verify_checkpoint_p != FBE_LBA_INVALID) &&
       (verify_checkpoint_p  > exported_capacity))
    {
        /* disk zeroing completed */
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s:checkpoint cannot be updated beyond capacity, chk:0x%llx, export_cap:0x%llx\n",
                              __FUNCTION__, (unsigned long long)verify_checkpoint_p, (unsigned long long)exported_capacity);
        verify_checkpoint_p = FBE_LBA_INVALID;
    }

  
    status = fbe_base_config_metadata_nonpaged_write_persist((fbe_base_config_t *) provision_drive_p,
                                                             packet_p,
                                                             (fbe_u64_t)(&((fbe_provision_drive_nonpaged_metadata_t*)0)->verify_invalidate_checkpoint),
                                                             (fbe_u8_t *)&verify_checkpoint_p,
                                                             sizeof(fbe_lba_t));
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/******************************************************************************
 * end fbe_provision_drive_metadata_set_verify_invalidate_checkpoint()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_provision_drive_metadata_set_bz_chkpt_without_persist()
 ******************************************************************************
 * @brief
 *   This function is used to set the background zeroing checkpoint in 
 *   non paged metadata.
 *
 * @param provision_drive_p         - pointer to the provision drive
 * @param packet_p                  - pointer to a monitor packet.
 * @param zero_checkpoint           - zero checkpoint.
 *
 * @return  fbe_status_t  
 *
 * @author
 *   10/19/2011 - Created. Ashwin Tamilarasan
 *
 ******************************************************************************/
fbe_status_t 
fbe_provision_drive_metadata_set_bz_chkpt_without_persist(fbe_provision_drive_t * provision_drive_p, 
                                                            fbe_packet_t * packet_p,
                                                            fbe_lba_t zero_checkpoint)
{
    fbe_status_t    status;
    
    /*! @todo Need to handle failure case.
     */
    status = fbe_base_config_metadata_nonpaged_write((fbe_base_config_t *) provision_drive_p,
                                                     packet_p,
                                                     (fbe_u64_t)(&((fbe_provision_drive_nonpaged_metadata_t*)0)->zero_checkpoint),
                                                     (fbe_u8_t *)&zero_checkpoint,
                                                     sizeof(fbe_lba_t));
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/******************************************************************************
 * end fbe_provision_drive_metadata_set_bz_chkpt_without_persist()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_provision_drive_metadata_update_background_zero_checkpoint()
 ******************************************************************************
 * @brief
 *   This function is used to set the background zeroing checkpoint in 
 *   non paged metadata and persist it
 *
 * @param provision_drive_p         - pointer to the provision drive
 * @param packet_p                  - pointer to a monitor packet.
 * @param zero_checkpoint           - zero checkpoint.
 *
 * @return  fbe_status_t  
 *
 * @author
 *   10/19/2011 - Created. Ashwin Tamilarasan
 *
 ******************************************************************************/
fbe_status_t 
fbe_provision_drive_metadata_update_background_zero_checkpoint(fbe_provision_drive_t * provision_drive_p, 
                                                            fbe_packet_t * packet_p,
                                                            fbe_lba_t zero_checkpoint)
{
    fbe_status_t    status;
    fbe_lba_t current_chkpt;
    fbe_provision_drive_metadata_get_background_zero_checkpoint(provision_drive_p, &current_chkpt);

    /* disk zeroing completed */
    fbe_base_object_trace((fbe_base_object_t *)provision_drive_p,
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "pvd_md_update_bz_chkpt Setting the BZ chkpt. current:0x%llx new:0x%llx\n",
                          (unsigned long long)current_chkpt, (unsigned long long)zero_checkpoint);
           
    status = fbe_base_config_metadata_nonpaged_set_checkpoint_persist((fbe_base_config_t *) provision_drive_p,
                                                                      packet_p,
                                                                    (fbe_u64_t)(&((fbe_provision_drive_nonpaged_metadata_t*)0)->zero_checkpoint),
                                                                      0, /* There is only a single zero checkpoint. */
                                                                    zero_checkpoint);
                                                                       
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/******************************************************************************
 * end fbe_provision_drive_metadata_update_background_zero_checkpoint()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_provision_drive_metadata_incr_background_zero_checkpoint()
 ******************************************************************************
 * @brief
 *   This function is used to increment the background zeroing checkpoint in 
 *   non paged metadata.
 *
 * @param provision_drive_p         - pointer to the provision drive
 * @param packet_p                  - pointer to a monitor packet.
 * @param zero_checkpoint           - zero checkpoint.
 * @param block_count               - block count to increment the checkpoint by
 *
 * @return  fbe_status_t  
 *
 * @author
 *   10/18/2011 - Created. Ashwin Tamilarasan
 *
 ******************************************************************************/
fbe_status_t 
fbe_provision_drive_metadata_incr_background_zero_checkpoint(fbe_provision_drive_t * provision_drive_p, 
                                                            fbe_packet_t * packet_p,
                                                            fbe_lba_t zero_checkpoint,
                                                            fbe_block_count_t  block_count)
{
    fbe_status_t    status;
    fbe_lba_t       exported_capacity = FBE_LBA_INVALID;
    fbe_u32_t ms_since_checkpoint;
    fbe_time_t last_checkpoint_time;

    /* get the paged metadatas start lba. */
    fbe_base_config_get_capacity((fbe_base_config_t *) provision_drive_p, &exported_capacity);

    /* Sanity check. We can get into this situation when we support multiple chunks background
       operation 
     */
    if((zero_checkpoint + block_count) > exported_capacity )
    {
        block_count = (zero_checkpoint + block_count) - exported_capacity;
        /* disk zeroing completed */
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "zero chkpt beyond capacity, zero_chk:0x%llx, export_cap:0x%llx,blk_cnt:0x%llx\n",
                              (unsigned long long)zero_checkpoint,
                  (unsigned long long)exported_capacity,
                  (unsigned long long)block_count);        
    }

    fbe_provision_drive_get_last_checkpoint_time(provision_drive_p, &last_checkpoint_time);
    ms_since_checkpoint = fbe_get_elapsed_milliseconds(last_checkpoint_time);


    if ((ms_since_checkpoint > FBE_PROVISION_DRIVE_PEER_CHECKPOINT_UPDATE_MS) && 
        /* Update peer only when we actually wrote to paged MD */
        !fbe_provision_drive_metadata_cache_is_flush_valid(provision_drive_p))
    {
        /* Periodically we will set the checkpoint to the peer. 
         * A set is needed since the peer might be out of date with us and an increment 
         * will not suffice. 
         */
        status = fbe_base_config_metadata_nonpaged_force_set_checkpoint((fbe_base_config_t *) provision_drive_p,
                                                                        packet_p,
                                                                        (fbe_u64_t)(&((fbe_provision_drive_nonpaged_metadata_t*)0)->zero_checkpoint),
                                                                        0,    /* Second metadata offset is not used for pvd*/
                                                                        zero_checkpoint + block_count);
        fbe_provision_drive_update_last_checkpoint_time(provision_drive_p);
        /* Also send notification upstream about our checkpoint.  Admin needs this in order to see the checkpoint
         * change. 
         */
        fbe_provision_drive_send_checkpoint_notification(provision_drive_p);
    }
    else
    {
        /* Otherwise update the checkpoint locally only and do not send it to the peer to avoid 
         * thrashing the CMI. 
         */
        status = fbe_base_config_metadata_nonpaged_incr_checkpoint_no_peer((fbe_base_config_t *) provision_drive_p,
                                                                   packet_p,
                                                                   (fbe_u64_t)(&((fbe_provision_drive_nonpaged_metadata_t*)0)->zero_checkpoint),
                                                                   0,    /* Second metadata offset is not used for pvd*/
                                                                   zero_checkpoint,
                                                                   block_count);
    }
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/******************************************************************************
 * end fbe_provision_drive_metadata_incr_background_zero_checkpoint()
 ******************************************************************************/
/*!****************************************************************************
 * fbe_provision_drive_metadata_incr_metadata_invalidate_checkpoint()
 ******************************************************************************
 * @brief
 *   This function is used to increment the metadata invalidate checkpoint in 
 *   non paged metadata.
 *
 * @param provision_drive_p         - pointer to the provision drive
 * @param packet_p                  - pointer to a monitor packet.
 * @param invalidate_checkpoint     - invalidate checkpoint.
 * @param block_count               - block count to increment the checkpoint by
 *
 * @return  fbe_status_t  
 *
 * @author
 *   03/09/2012 - Created. Ashok Tamilarasan
 *
 ******************************************************************************/
fbe_status_t 
fbe_provision_drive_metadata_incr_verify_invalidate_checkpoint(fbe_provision_drive_t * provision_drive_p, 
                                                               fbe_packet_t * packet_p,
                                                               fbe_lba_t invalidate_checkpoint,
                                                               fbe_block_count_t  block_count)
{
    fbe_status_t    status;
    fbe_lba_t       exported_capacity = FBE_LBA_INVALID;

    /* get the paged metadatas start lba. */
    fbe_base_config_get_capacity((fbe_base_config_t *) provision_drive_p, &exported_capacity);

    /* Sanity check. We can get into this situation when we support multiple chunks background
       operation 
     */
    if((invalidate_checkpoint + block_count) > exported_capacity )
    {
        block_count = (invalidate_checkpoint + block_count) - exported_capacity;
        /* disk zeroing completed */
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "zero chkpt beyond capacity, zero_chk:0x%llx, export_cap:0x%llx,blk_cnt:0x%llx\n",
                              (unsigned long long)invalidate_checkpoint, (unsigned long long)exported_capacity, (unsigned long long)block_count);        
    }

    
    /* The called function will take care of error and complete the packet */
    status = fbe_base_config_metadata_nonpaged_incr_checkpoint((fbe_base_config_t *) provision_drive_p,
                                                                packet_p,
                                                               (fbe_u64_t)(&((fbe_provision_drive_nonpaged_metadata_t*)0)->verify_invalidate_checkpoint),
                                                                0, /* Second metadata offset is not used for pvd*/
                                                               invalidate_checkpoint,
                                                               block_count);
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/******************************************************************************
 * end fbe_provision_drive_metadata_incr_verify_invalidate_checkpoint()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_provision_drive_metadata_get_zero_on_demand_flag()
 ******************************************************************************
 * @brief
 *  This function is used to get the zero on demand flag fron the nonpaged
 *  metadata memory.
 *
 * @param provision_drive_p         - pointer to the provision drive
 * @param zero_on_demand_p          - pointer to the zero on demand flag.
 *
 * @return  fbe_status_t  
 *
 * @author
 *   06/21/2010 - Created. Dhaval Patel
 *
 ******************************************************************************/
fbe_status_t 
fbe_provision_drive_metadata_get_zero_on_demand_flag(fbe_provision_drive_t * provision_drive_p,
                                                     fbe_bool_t * zero_on_demand_p)
{

    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_provision_drive_nonpaged_metadata_t * provision_drive_nonpaged_metadata_p = NULL;

    status = fbe_base_config_get_nonpaged_metadata_ptr((fbe_base_config_t *) provision_drive_p, (void **)&provision_drive_nonpaged_metadata_p);

   if((status != FBE_STATUS_OK) ||
       (provision_drive_nonpaged_metadata_p == NULL))
    {
        return status;
    }

    *zero_on_demand_p = provision_drive_nonpaged_metadata_p->zero_on_demand;
    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_provision_drive_get_zero_on_demand_flag()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_provision_drive_metadata_set_zero_on_demand_flag()
 ******************************************************************************
 * @brief
 *  This function is used to set the zero on demand flag in non paged metadata.
 *
 * @param provision_drive_p         - pointer to the provision drive
 * @param packet_p                  - pointer to a monitor packet.
 * @param zero_on_demand_b          - zero on demand flag.
 *
 * @return  fbe_status_t            - return status of the operation.
 *
 * @author
 *   06/21/2010 - Created. Dhaval Patel
 *
 ******************************************************************************/
fbe_status_t 
fbe_provision_drive_metadata_set_zero_on_demand_flag(fbe_provision_drive_t * provision_drive_p, 
                                                     fbe_packet_t * packet_p,
                                                     fbe_bool_t zero_on_demand_b)
{
    fbe_status_t status;

    fbe_bool_t zero_on_demand_flag = zero_on_demand_b;

    /* set the zero on demand flag using base config nonpaged write. */
    status = fbe_base_config_metadata_nonpaged_write_persist((fbe_base_config_t *) provision_drive_p,
                                                             packet_p,
                                                             (fbe_u64_t)(&((fbe_provision_drive_nonpaged_metadata_t*)0)->zero_on_demand),
                                                             (fbe_u8_t *)&zero_on_demand_flag,
                                                             sizeof(fbe_bool_t));
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/******************************************************************************
 * end fbe_provision_drive_metadata_set_zero_on_demand_flag()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_provision_drive_metadata_get_end_of_life_state()
 ******************************************************************************
 * @brief
 *  This function is used to get the end of life state for the provision drive
 *  object.
 *
 * @param provision_drive_p         - pointer to the provision drive
 * @param end_of_life_state         - pointer to the end of life state.
 *
 * @return  fbe_status_t  
 *
 * @author
 *  08/20/2010 - Created. Dhaval Patel
 *
 ******************************************************************************/
fbe_status_t 
fbe_provision_drive_metadata_get_end_of_life_state(fbe_provision_drive_t * provision_drive,
                                                   fbe_bool_t * end_of_life_state)
{
    fbe_provision_drive_nonpaged_metadata_t * provision_drive_nonpaged_metadata = NULL;
    fbe_status_t status;

    /* get end of life state of the pvd object. */
    status = fbe_base_config_get_nonpaged_metadata_ptr((fbe_base_config_t *) provision_drive, (void **)&provision_drive_nonpaged_metadata);

   if((status != FBE_STATUS_OK) ||
       (provision_drive_nonpaged_metadata == NULL))
    {
        return status;
    }
    *end_of_life_state = provision_drive_nonpaged_metadata->end_of_life_state;
    return FBE_STATUS_OK;
}

/******************************************************************************
 * end fbe_provision_drive_metadata_get_end_of_life_state()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_provision_drive_metadata_set_end_of_life_state()
 ******************************************************************************
 * @brief
 *  This function is used to to set the end of life state for the provision
 *  drive object.
 *
 * @param provision_drive_p         - pointer to the provision drive
 * @param packet_p                  - pointer to a monitor packet.
 * @param end_of_life_state         - end of life state.
 *
 * @return  fbe_status_t  
 *
 * @author
 *   20/08/2010 - Created. Dhaval Patel
 *
 ******************************************************************************/
fbe_status_t 
fbe_provision_drive_metadata_set_end_of_life_state(fbe_provision_drive_t * provision_drive_p,
                                                   fbe_packet_t * packet_p,
                                                   fbe_bool_t end_of_life_state)
{
    fbe_status_t    status;

    /*!@todo we might need to handle the nonpaged metadata write explicitly to
     * pass the error message to the caller.
     */
    status = fbe_base_config_metadata_nonpaged_write_persist((fbe_base_config_t *) provision_drive_p,
                                                             packet_p,
                                                             (fbe_u64_t)(&((fbe_provision_drive_nonpaged_metadata_t*)0)->end_of_life_state),
                                                             (fbe_u8_t *)&end_of_life_state,
                                                             sizeof(fbe_bool_t));
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/******************************************************************************
 * end fbe_provision_drive_metadata_set_end_of_life_state()
 ******************************************************************************/

fbe_status_t
fbe_provision_drive_metadata_get_paged_metadata_capacity(fbe_provision_drive_t * provision_drive_p,
                                                         fbe_lba_t * paged_metadata_capacity_p)
{
    fbe_lba_t   paged_mirror_metadata_offset;

    fbe_base_config_metadata_get_paged_mirror_metadata_offset((fbe_base_config_t *) provision_drive_p, &paged_mirror_metadata_offset);
    *paged_metadata_capacity_p = paged_mirror_metadata_offset * FBE_PROVISION_DRIVE_NUMBER_OF_METADATA_COPIES;
    return FBE_STATUS_OK;
}

/*!****************************************************************************
 * fbe_provision_drive_metadata_set_sniff_verify_checkpoint()
 ******************************************************************************
 * @brief
 *   This function sends the request to metadata service to set the sniff verify checkpoint in the 
 *   non-paged metadata for the specified provision drive.
 *
 * @param provision_drive_p           - pointer to the provision drive
 * @param packet_p                     - pointer to a monitor packet.
 * @param sniff_verify_checkpoint   - sniff verify checkpoint
 *
 * @return  fbe_status_t  
 *
 * @author
 *   07/16/2010 - Created. Amit Dhaduk
 *
 ******************************************************************************/
fbe_status_t 
fbe_provision_drive_metadata_set_sniff_verify_checkpoint(fbe_provision_drive_t * provision_drive_p, 
                                                         fbe_packet_t * packet_p,
                                                         fbe_lba_t sniff_verify_checkpoint)
{

    fbe_status_t  status;

    fbe_lba_t sniff_checkpoint = sniff_verify_checkpoint;

    // sending request to metadata service to set verify checkpoint
    status = fbe_base_config_metadata_nonpaged_write((fbe_base_config_t *) provision_drive_p,
                                                    packet_p,
                                                    (fbe_u64_t)(&((fbe_provision_drive_nonpaged_metadata_t*)0)->sniff_verify_checkpoint),
                                                    (fbe_u8_t *)&sniff_checkpoint,
                                                    sizeof(fbe_lba_t));
     
                                                
    return status;
    
}
/******************************************************************************
 * end fbe_provision_drive_metadata_set_sniff_verify_checkpoint()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_provision_drive_metadata_get_sniff_verify_checkpoint()
 ******************************************************************************
 *
 * @brief
 *    This function gets the sniff verify checkpoint in the non-paged metadata
 *    for the specified provision drive.
 *
 * @param   in_provision_drive_p     -  pointer to provision drive
 * @param   out_verify_checkpoint_p  -  verify checkpoint output parameter
 *
 * @return  fbe_status_t             - return status of the operation. 
 *
 * @version
 *    02/12/2010 - Created. Randy Black
 *
 ******************************************************************************/
fbe_status_t 
fbe_provision_drive_metadata_get_sniff_verify_checkpoint(fbe_provision_drive_t * in_provision_drive,
                                                         fbe_lba_t * out_verify_checkpoint_p)
{
    fbe_provision_drive_nonpaged_metadata_t * provision_drive_nonpaged_metadata = NULL;

    fbe_base_config_get_nonpaged_metadata_ptr((fbe_base_config_t *) in_provision_drive, (void **)&provision_drive_nonpaged_metadata);

    // get verify checkpoint from provision drive's non-paged metadata
    * out_verify_checkpoint_p = provision_drive_nonpaged_metadata->sniff_verify_checkpoint;

    return FBE_STATUS_OK;
} 
/******************************************************************************
 * end  fbe_provision_drive_metadata_get_sniff_verify_checkpoint()
 ******************************************************************************/
/*!****************************************************************************
 * fbe_provision_drive_metadata_get_nonpaged_metadata_drive_info()
 ******************************************************************************
 * @brief
 *   This function is used to get the drive location from the nonpaged metadata.
 *   non-paged metadata for the specified provision drive.
 *
 * @param provision_drive_p           - pointer to the provision drive
 * @param nonpaged_drive_info_p       - pointer to the nonpaged drive info.
 *
 * @return  fbe_status_t  
 *
 * @author
 *   07/16/2010 - Created. Dhaval Patel
 *
 ******************************************************************************/
fbe_status_t
fbe_provision_drive_metadata_get_nonpaged_metadata_drive_info(fbe_provision_drive_t * in_provision_drive,
                                                              fbe_provision_drive_nonpaged_metadata_drive_info_t * nonpaged_drive_info_p)
{
    fbe_provision_drive_nonpaged_metadata_t *   provision_drive_nonpaged_metadata_p = NULL;
    fbe_status_t                                status;
    
    /* get nonpaged metadata pointer, if it is null or status is not good then return error. */
    status = fbe_base_config_get_nonpaged_metadata_ptr((fbe_base_config_t *) in_provision_drive, (void **)&provision_drive_nonpaged_metadata_p);
    if((status != FBE_STATUS_OK) ||
       (provision_drive_nonpaged_metadata_p == NULL))
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    // get drive location from nonpaged metadata
    nonpaged_drive_info_p->port_number = provision_drive_nonpaged_metadata_p->nonpaged_drive_info.port_number;
    nonpaged_drive_info_p->enclosure_number = provision_drive_nonpaged_metadata_p->nonpaged_drive_info.enclosure_number;
    nonpaged_drive_info_p->slot_number = provision_drive_nonpaged_metadata_p->nonpaged_drive_info.slot_number;
    nonpaged_drive_info_p->drive_type = provision_drive_nonpaged_metadata_p->nonpaged_drive_info.drive_type;

    return FBE_STATUS_OK;
}
/******************************************************************************
 * end  fbe_provision_drive_metadata_get_nonpaged_metadata_drive_info()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_provision_drive_metadata_get_physical_drive_info()
 ******************************************************************************
 *
 * @brief
 *  This function issues usurpur command to get the physical drive location
 *  and once it gets information it updates it into nonpaged metadata.
 *
 * @param   provision_drive_p       -  pointer to provision drive
 * @param   packet_p                -  pointer to the packet
 *
 * @return  void
 *
 * @version
 *    12/10/2010 - Created. Dhaval Patel
 *
 ******************************************************************************/
fbe_status_t
fbe_provision_drive_metadata_get_physical_drive_info(fbe_provision_drive_t * provision_drive_p, fbe_packet_t * packet_p)
{
    fbe_payload_ex_t *                                  sep_payload_p = NULL;
    fbe_payload_control_operation_t *                   control_operation_p = NULL;
    fbe_payload_control_operation_t *                   new_control_operation_p = NULL;
    fbe_physical_drive_mgmt_get_drive_information_t *   get_physical_drive_info_p = NULL;
    fbe_status_t                                        status;

    /* get the current control operation for the port information. */
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(sep_payload_p);

    /* allocate the new control operation. */
    new_control_operation_p = fbe_payload_ex_allocate_control_operation(sep_payload_p);

    /* allocate the memory to get the drive location. */
    get_physical_drive_info_p = (fbe_physical_drive_mgmt_get_drive_information_t *) fbe_transport_allocate_buffer();
    if(get_physical_drive_info_p  == NULL)
    {
        fbe_payload_ex_release_control_operation(sep_payload_p, new_control_operation_p);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* initialize nonpaged drive information as invalid. */
    get_physical_drive_info_p->get_drive_info.port_number = FBE_PORT_NUMBER_INVALID;
    get_physical_drive_info_p->get_drive_info.enclosure_number = FBE_ENCLOSURE_NUMBER_INVALID;
    get_physical_drive_info_p->get_drive_info.slot_number = FBE_SLOT_NUMBER_INVALID;
    get_physical_drive_info_p->get_drive_info.drive_type = FBE_DRIVE_TYPE_INVALID;
    get_physical_drive_info_p->get_drive_info.spin_down_qualified = FBE_FALSE;
    get_physical_drive_info_p->get_drive_info.drive_parameter_flags = 0;
    get_physical_drive_info_p->get_drive_info.drive_type = FBE_DRIVE_TYPE_INVALID;

    /* Build the control packet to get the physical drive inforamtion. */
    fbe_payload_control_build_operation(new_control_operation_p,
                                        FBE_PHYSICAL_DRIVE_CONTROL_CODE_GET_DRIVE_INFORMATION,
                                        get_physical_drive_info_p,
                                        sizeof(fbe_physical_drive_mgmt_get_drive_information_t));

    /* increment the control operation and call the direct function to get encl/drive/slot info */
    fbe_payload_ex_increment_control_operation_level(sep_payload_p);

    /* Set the completion function. */
    fbe_transport_set_completion_function(packet_p,
                                          fbe_provision_drive_metadata_get_physical_drive_info_completion,
                                          provision_drive_p);

    /* Set the traverse attribute which allows traverse to this packet
     * till port to get the required information.
     */
    fbe_transport_set_packet_attr(packet_p, FBE_PACKET_FLAG_TRAVERSE);
    status = fbe_block_transport_send_control_packet(((fbe_base_config_t *)provision_drive_p)->block_edge_p, packet_p);
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/******************************************************************************
 * end  fbe_provision_drive_metadata_get_physical_drive_info()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_provision_drive_metadata_get_physical_drive_info_completion()
 ******************************************************************************
 *
 * @brief
 *    This function handles the completion for the get physical drive info.
 *
 * @param   in_provision_drive_p     -  pointer to provision drive
 * @param   out_verify_checkpoint_p  -  verify checkpoint output parameter
 *
 * @return  void
 *
 * @version
 *    02/12/2010 - Created. Dhaval Patel
 *
 ******************************************************************************/
static fbe_status_t
fbe_provision_drive_metadata_get_physical_drive_info_completion(fbe_packet_t * packet_p,
                                                                fbe_packet_completion_context_t context)
{
    fbe_provision_drive_t *                                 provision_drive_p = NULL;
    fbe_payload_ex_t *                                      sep_payload_p = NULL;
    fbe_payload_control_operation_t *                       control_operation_p = NULL;
    fbe_payload_control_operation_t *                       prev_control_operation_p = NULL;
    fbe_physical_drive_mgmt_get_drive_information_t *       get_physical_drive_info_p= NULL;
    fbe_physical_drive_mgmt_get_drive_information_t         pvd_physical_drive_info;
    fbe_status_t                                            status;
    fbe_payload_control_status_t                            control_status;
    fbe_bool_t                                              is_unmap_supported = FBE_FALSE;
    fbe_pdo_drive_parameter_flags_t                         drive_param_flags;

    provision_drive_p = (fbe_provision_drive_t *)context;

    /* get the current control operation for the port information. */
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(sep_payload_p);

    /* get the status of the control operation. */
    fbe_payload_control_get_status(control_operation_p, &control_status);
    status = fbe_transport_get_status_code(packet_p);

    /* get the buffer of the current control operation. */
    fbe_payload_control_get_buffer(control_operation_p, &get_physical_drive_info_p);

    /* get the previous control operation and associated buffer. */
    prev_control_operation_p = fbe_payload_ex_get_prev_control_operation(sep_payload_p);

    /* If status is not good then complete the packet with error. */
    if((status != FBE_STATUS_OK) ||
       (control_status != FBE_PAYLOAD_CONTROL_STATUS_OK) ||
       ((get_physical_drive_info_p->get_drive_info.port_number == FBE_PORT_NUMBER_INVALID) ||
        (get_physical_drive_info_p->get_drive_info.enclosure_number == FBE_ENCLOSURE_NUMBER_INVALID) ||
        (get_physical_drive_info_p->get_drive_info.slot_number == FBE_SLOT_NUMBER_INVALID) ||
        (get_physical_drive_info_p->get_drive_info.drive_type == FBE_DRIVE_TYPE_INVALID)))
    {
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s: get drive location failed, ctl_stat:0x%x, stat:0x%x, \n",
                               __FUNCTION__, control_status, status);
        status = (status != FBE_STATUS_OK) ? status : FBE_STATUS_GENERIC_FAILURE;
        control_status = (control_status != FBE_PAYLOAD_CONTROL_STATUS_OK) ? control_status : FBE_PAYLOAD_CONTROL_STATUS_FAILURE;
        fbe_payload_ex_release_control_operation(sep_payload_p, control_operation_p);
        fbe_transport_release_buffer(get_physical_drive_info_p);
        fbe_transport_set_status(packet_p, status, 0);
        fbe_payload_control_set_status(prev_control_operation_p, control_status);
        return status;
    }

    /* if status is good then copy the drive location and drive type. */
    fbe_copy_memory(&pvd_physical_drive_info, get_physical_drive_info_p, sizeof(fbe_physical_drive_mgmt_get_drive_information_t));

    /* set the spin_down_flag for the PVD using physical drive info. */
    fbe_provision_drive_set_spin_down_qualified(provision_drive_p, get_physical_drive_info_p->get_drive_info.spin_down_qualified);

    /* set the unmap_flag for the PVD using physical drive info. 
     * set to true if either unmap or unmap write same is supported
     */
    drive_param_flags = get_physical_drive_info_p->get_drive_info.drive_parameter_flags;
    is_unmap_supported = ((drive_param_flags & (FBE_PDO_FLAG_SUPPORTS_UNMAP | FBE_PDO_FLAG_SUPPORTS_WS_UNMAP)) != 0);
    fbe_provision_drive_set_unmap_supported(provision_drive_p, is_unmap_supported);
    fbe_base_object_trace((fbe_base_object_t *)provision_drive_p,
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "UNMAP fbe_pvd_md_get_physical_drive_info: is_unmap_supported:%d flags: 0x%x 0x%x\n",
                          is_unmap_supported, drive_param_flags,
                          get_physical_drive_info_p->get_drive_info.drive_type);

    /* Only send wear level warnings for le and ri ssd drives */
    if (get_physical_drive_info_p->get_drive_info.drive_type == FBE_DRIVE_TYPE_SAS_FLASH_LE ||
        get_physical_drive_info_p->get_drive_info.drive_type == FBE_DRIVE_TYPE_SAS_FLASH_RI)
    {
        fbe_provision_drive_set_flag(provision_drive_p, FBE_PROVISION_DRIVE_FLAG_REPORT_WPD_WARNING); 
    }
    fbe_provision_drive_set_wear_leveling_query_time(provision_drive_p, fbe_get_time());

    /* set drive location in metadata memory. */
    fbe_provision_drive_set_drive_location(provision_drive_p, 
                                           get_physical_drive_info_p->get_drive_info.port_number,
                                           get_physical_drive_info_p->get_drive_info.enclosure_number,
                                           get_physical_drive_info_p->get_drive_info.slot_number);

    /* releaes the buffer and control operation. */
    fbe_payload_ex_release_control_operation(sep_payload_p, control_operation_p);
    fbe_transport_release_buffer(get_physical_drive_info_p);

    /* set the good status in prev control operation */
    fbe_payload_control_set_status(prev_control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_OK);

    /* for passive side we don't need to update the nonpaged metadata */
    if (!fbe_base_config_is_active((fbe_base_config_t*)provision_drive_p))
    {
        return FBE_STATUS_OK;
    }

    /* set the comletion function to update the nonpaged metadata */

    fbe_transport_set_completion_function(packet_p,
                                          fbe_provision_drive_metadata_update_physical_drive_info_completion,
                                          provision_drive_p);

    /* get drive location passsed successfully and so update the nonpaged metadata. */
    status = fbe_provision_drive_metadata_update_physical_drive_info(provision_drive_p, packet_p, &pvd_physical_drive_info);
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/******************************************************************************
 * end  fbe_provision_drive_metadata_get_physical_drive_info_completion()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_provision_drive_metadata_update_physical_drive_info()
 ******************************************************************************
 * @brief
 *  This function sends the request to metadata service to update the enclosure
 *  port and slot information in nonpaged metadata.
 *
 * @param provision_drive_p             - pointer to the provision drive
 * @param packet_p                      - pointer to a monitor packet.
 * @param drive_location                - drive location information.
 *
 * @return  fbe_status_t
 *
 * @author
 *   12/10/2010 - Created. Dhaval Patel
 *
 ******************************************************************************/
fbe_status_t 
fbe_provision_drive_metadata_update_physical_drive_info(fbe_provision_drive_t * provision_drive_p, 
                                                        fbe_packet_t * packet_p,
                                                        fbe_physical_drive_mgmt_get_drive_information_t *physical_drive_info)
{

    fbe_status_t                                        status;
    fbe_provision_drive_nonpaged_metadata_drive_info_t  np_metadata_drive_info;
    fbe_provision_drive_nonpaged_metadata_drive_info_t  metadata_drive_info;

    /* first get the drive location which is stored in nonpaged metadata */
    status = fbe_provision_drive_metadata_get_nonpaged_metadata_drive_info(provision_drive_p, &np_metadata_drive_info);
    if(status != FBE_STATUS_OK)
    {
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return status;
    }

    /* if drive information in nonpaged metadata is same as drive information retrieved from the pdo
     * object then complete the packet without updating nonpaged data.
     */
    if((np_metadata_drive_info.port_number == physical_drive_info->get_drive_info.port_number) &&
       (np_metadata_drive_info.enclosure_number == physical_drive_info->get_drive_info.enclosure_number) &&
       (np_metadata_drive_info.slot_number == physical_drive_info->get_drive_info.slot_number) &&
       (np_metadata_drive_info.drive_type == physical_drive_info->get_drive_info.drive_type))
    {
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_OK;
    }

    /* copy metadata drive information */
    metadata_drive_info.port_number = physical_drive_info->get_drive_info.port_number;
    metadata_drive_info.enclosure_number = physical_drive_info->get_drive_info.enclosure_number;
    metadata_drive_info.slot_number = physical_drive_info->get_drive_info.slot_number;
    metadata_drive_info.drive_type = physical_drive_info->get_drive_info.drive_type;

    // sending request to metadata service to update the drive information
    status = fbe_base_config_metadata_nonpaged_write_persist((fbe_base_config_t *) provision_drive_p,
                                                     packet_p,
                                                     (fbe_u64_t)(&((fbe_provision_drive_nonpaged_metadata_t *)0)->nonpaged_drive_info),
                                                     (fbe_u8_t *)&metadata_drive_info,
                                                     sizeof(fbe_provision_drive_nonpaged_metadata_drive_info_t));
    return status;
    
}
/******************************************************************************
 * end fbe_provision_drive_metadata_update_physical_drive_info()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_provision_drive_metadata_update_physical_drive_info_completion()
 ******************************************************************************
 * @brief
 *  This function is used as completion function to update the drive location.
 *
 * @param provision_drive_p             - pointer to the provision drive
 * @param packet_p                      - pointer to a monitor packet.
 * @param drive_location                - drive location information.
 *
 * @return  fbe_status_t
 *
 * @author
 *   12/10/2010 - Created. Dhaval Patel
 *
 ******************************************************************************/
static fbe_status_t
fbe_provision_drive_metadata_update_physical_drive_info_completion(fbe_packet_t * packet_p,
                                                                   fbe_packet_completion_context_t context)
{
    fbe_payload_ex_t *                 sep_payload_p = NULL;
    fbe_payload_control_operation_t *   control_operation_p = NULL;
    fbe_provision_drive_t *             provision_drive_p = NULL;
    fbe_status_t                        status;

    provision_drive_p = (fbe_provision_drive_t *)context;

    /* Get the packet status. */
    status = fbe_transport_get_status_code(packet_p);

    /* Get control operation */
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(sep_payload_p);

    /* Clear the set logical marker end condition if status is ok. */
    if(status != FBE_STATUS_OK)
    {    
        fbe_provision_drive_utils_trace(provision_drive_p, 
                                        FBE_TRACE_LEVEL_DEBUG_LOW,
                                        FBE_TRACE_MESSAGE_ID_INFO,
                                        FBE_PROVISION_DRIVE_DEBUG_FLAG_BGZ_TRACING,
                                        "%s update drive location failed, stat:%d\n",
                                        __FUNCTION__, status);
        return status;
    }
    else
    {
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_OK);
        return FBE_STATUS_OK;
    }

    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_provision_drive_metadata_update_physical_drive_info_completion()
 ******************************************************************************/


/*!****************************************************************************
 * fbe_provision_drive_metadata_set_sniff_media_error_lba()
 ******************************************************************************
 * @brief
 *   This function sends the request to metadata service to set the media error lba in the 
 *   non-paged metadata for the specified provision drive.
 *
 * @param provision_drive_p           - pointer to the provision drive
 * @param packet_p                    - pointer to a monitor packet.
 * @param media error lba             - verify media error lba 
 *
 * @return  fbe_status_t  
 *
 * @author
 *   11/29/2010 - Created. Maya Dagon
 *
 ******************************************************************************/
fbe_status_t 
fbe_provision_drive_metadata_set_sniff_media_error_lba(fbe_provision_drive_t * provision_drive_p, 
                                                   fbe_packet_t * packet_p,
                                                   fbe_lba_t media_error_lba)
{
    fbe_status_t  status;
    fbe_lba_t current_media_error_lba;

    fbe_provision_drive_metadata_get_sniff_media_error_lba(provision_drive_p, &current_media_error_lba);

    if(current_media_error_lba == media_error_lba){
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_OK;
    }

    // sending request to metadata service to set verify checkpoint
    status = fbe_base_config_metadata_nonpaged_write((fbe_base_config_t *) provision_drive_p,
                                                    packet_p,
                                                    (fbe_u64_t)(&((fbe_provision_drive_nonpaged_metadata_t*)0)->media_error_lba),
                                                    (fbe_u8_t *)&media_error_lba,
                                                    sizeof(fbe_lba_t));
     
                                                
    return status;
    
} 
/******************************************************************************
 * end fbe_provision_drive_metadata_set_sniff_media_error_lba()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_provision_drive_metadata_get_sniff_media_error_lba
 ******************************************************************************
 *
 * @brief
 *    This function gets the media error lba in the non-paged metadata
 *    for the specified provision drive.
 *
 * @param   in_provision_drive_p     -  pointer to provision drive
 * @param   out_verify_checkpoint_p  -  sniff media error lba output parameter
 *
 * @return  void
 *
 * @version
 *    11/29/2010 - Created. Maya Dagon
 *
 ******************************************************************************/
fbe_status_t 
fbe_provision_drive_metadata_get_sniff_media_error_lba(fbe_provision_drive_t * in_provision_drive,
                                                   fbe_lba_t * out_media_error_lba_p)
{

     fbe_status_t  status;
     fbe_provision_drive_nonpaged_metadata_t * provision_drive_nonpaged_metadata = NULL;

    status = fbe_base_config_get_nonpaged_metadata_ptr((fbe_base_config_t *) in_provision_drive, (void **)&provision_drive_nonpaged_metadata);

    if((status != FBE_STATUS_OK) ||
       (provision_drive_nonpaged_metadata == NULL))
    {
        return status;
    }
    
    // get verify checkpoint from provision drive's non-paged metadata
    * out_media_error_lba_p = provision_drive_nonpaged_metadata->media_error_lba;

    return FBE_STATUS_OK;
} 
/******************************************************************************
 * end  fbe_provision_drive_metadata_get_sniff_media_error_lba()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_provision_drive_metadata_get_sniff_pass_count
 ******************************************************************************
 *
 * @brief
 *    This function gets the total sniff pass count in the non-paged metadata
 *    for the specified provision drive.
 *
 * @param   in_provision_drive_p     -  pointer to the provision drive
 * @param   out_pass_count_p         -  pointer to sniff pass count
 *
 * @return fbe_status_t
 *  The status of the operation typically FBE_STATUS_OK.
 *
 * @version
 *    06/25/2014 - Created. SaranyaDevi Ganesan
 *
 ******************************************************************************/
fbe_status_t 
fbe_provision_drive_metadata_get_sniff_pass_count(fbe_provision_drive_t *in_provision_drive_p,
                                                   fbe_u32_t *out_pass_count_p)
{
    fbe_status_t status;
    fbe_provision_drive_nonpaged_metadata_t *provision_drive_nonpaged_metadata = NULL;

    status = fbe_base_config_get_nonpaged_metadata_ptr((fbe_base_config_t *) in_provision_drive_p, (void **)&provision_drive_nonpaged_metadata);

    if((status != FBE_STATUS_OK) ||
       (provision_drive_nonpaged_metadata == NULL))
    {
        return status;
    }
    
    // Get pass count from provision drive's non-paged metadata
    *out_pass_count_p = provision_drive_nonpaged_metadata->pass_count;

    return FBE_STATUS_OK;
} 
/******************************************************************************
 * end fbe_provision_drive_metadata_get_sniff_pass_count()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_provision_drive_metadata_set_sniff_pass_count()
 ******************************************************************************
 * @brief
 *  This function sends the request to metadata service to set the sniff 
 *  pass count in the non-paged metadata for the specified provision drive.
 *
 * @param in_provision_drive_p          - pointer to the provision drive
 * @param packet_p                      - pointer to a monitor packet
 * @param pass_count                    - sniff pass count
 *
 * @return fbe_status_t
 *  The status of the operation typically FBE_STATUS_OK.
 *
 * @version
 *    06/25/2014 - Created. SaranyaDevi Ganesan
 *
 ******************************************************************************/
fbe_status_t 
fbe_provision_drive_metadata_set_sniff_pass_count(fbe_provision_drive_t *in_provision_drive_p,
                                                   fbe_packet_t *packet_p,
                                                   fbe_u32_t pass_count)
{
    fbe_status_t status;

    status = fbe_base_config_metadata_nonpaged_write_persist((fbe_base_config_t *) in_provision_drive_p,
                                                             packet_p,
                                                             (fbe_u64_t)(&((fbe_provision_drive_nonpaged_metadata_t*)0)->pass_count),
                                                             (fbe_u8_t *)&pass_count,
                                                             sizeof(fbe_u32_t));
    return status;
}
/******************************************************************************
 * end fbe_provision_drive_metadata_set_sniff_pass_count()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_provision_drive_metadata_clear_sniff_pass_count()
 ******************************************************************************
 * @brief
 *  This function is used to clear the sniff pass count in the NP and persist it.
 *
 * @param packet_p      - pointer to a control packet from the scheduler.
 * @param context       - context, which is a pointer to the base object.
 * 
 * @return fbe_status_t    - Always FBE_STATUS_MORE_PROCESSING_REQUIRED
 *
 * @author
 *  07/01/2014 - Created. SaranyaDevi Ganesan
 *
 ******************************************************************************/
fbe_status_t 
fbe_provision_drive_metadata_clear_sniff_pass_count(fbe_packet_t * packet_p,
                                    fbe_packet_completion_context_t context)
{
    fbe_provision_drive_t *provision_drive_p = NULL;    
    fbe_status_t status;
    fbe_u32_t pass_count = 0;
    
    provision_drive_p = (fbe_provision_drive_t*)context;

    /* If the status is not ok, that means we didn't get the 
       lock. Just return, we are already in the completion routine
    */
    status = fbe_transport_get_status_code (packet_p);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*)provision_drive_p, 
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s can't get the NP lock, status: 0x%X", __FUNCTION__, status); 
        return FBE_STATUS_OK;
    }

    pass_count = 0;

    fbe_transport_set_completion_function(packet_p, fbe_provision_drive_release_NP_lock, provision_drive_p);
    fbe_provision_drive_metadata_set_sniff_pass_count(provision_drive_p, packet_p, pass_count);

    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/******************************************************************************
 * end fbe_provision_drive_metadata_clear_sniff_pass_count()
 ******************************************************************************/

/*this function is called when we are done allocating the the packet we will use to do write_same and zero the paged metadata area*/
static fbe_status_t
fbe_provision_drive_zero_paged_metadata_allocate_completion(fbe_memory_request_t * memory_request_p, 
                                                                                  fbe_memory_completion_context_t context)
{
    fbe_provision_drive_t *                 provision_drive_p = (fbe_provision_drive_t *) context;
    fbe_packet_t *                          packet_p = NULL;
    fbe_payload_ex_t *                      sep_payload_p = NULL;
    fbe_payload_block_operation_t *         block_operation_p = NULL;
    fbe_status_t                            status;
    fbe_lba_t                               start_lba;
    fbe_block_count_t                       block_count;
    fbe_sg_element_t *                      sg_list_p = NULL;
    fbe_packet_t *                          new_packet_p;
    fbe_payload_ex_t *                      new_payload_p;

    /* get the pointer to original packet. */
    packet_p = fbe_transport_memory_request_to_packet(memory_request_p);

    /* Get the payload and metadata operation. */
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    
    /* get the block operation. */
    block_operation_p =  fbe_payload_ex_get_present_block_operation(sep_payload_p);

    /* Check allocation status */
    if (fbe_memory_request_is_allocation_complete(memory_request_p) == FBE_FALSE)
    {
        /* Currently this callback doesn't expect any aborted requests */
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p, 
                                        FBE_TRACE_LEVEL_ERROR,
                                        FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                        "%s memory allocation failed, state:%d\n",
                                        __FUNCTION__, memory_request_p->request_state);

        fbe_payload_block_set_status(block_operation_p,
                                     FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED,
                                     FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_UNEXPECTED_ERROR);
        fbe_provision_drive_utils_complete_packet(packet_p, FBE_STATUS_OK, 0);

        return FBE_STATUS_INSUFFICIENT_RESOURCES;
    }

    /* get the subpacket pointers from the allocated memory. */
    status = fbe_provision_drive_utils_get_packet_and_sg_list(provision_drive_p,
                                                              memory_request_p,
                                                              &new_packet_p,
                                                              1,
                                                              NULL,
                                                              0);

    /*initialize it*/
    fbe_transport_initialize_packet(new_packet_p);

    /*and get all the information we need to send it down*/
    fbe_payload_block_get_block_count(block_operation_p, &block_count);

    /*we zero in chunk sizes and if we have too much we just limit it*/
    if (block_count >= FBE_PROVISION_DRIVE_CHUNK_SIZE) {
        block_count = FBE_PROVISION_DRIVE_CHUNK_SIZE;
    }

    /* get the optimum block size, start lba and block count. */
    fbe_payload_block_get_lba(block_operation_p, &start_lba);
    
    
    /* get the zeroed initialized sg list for the write same operation. */
    /* CBE WRITE SAME */
    if(fbe_provision_drive_is_write_same_enabled(provision_drive_p, start_lba)){ /* Write Same IS enabled */
        sg_list_p = fbe_zero_get_bit_bucket_sgl();
    } else { /* Write Same NOT enabled */
        fbe_sg_element_t * pre_sg_list;
        sg_list_p = fbe_zero_get_bit_bucket_sgl_1MB();

        fbe_payload_ex_get_pre_sg_list(sep_payload_p, &pre_sg_list);
        pre_sg_list[0].address = sg_list_p[0].address;
        pre_sg_list[0].count = (fbe_u32_t)(block_operation_p->block_size * block_operation_p->block_count);
        pre_sg_list[1].address = NULL;
        pre_sg_list[1].count = 0;

        sg_list_p = NULL;
    }


    /* if either status is not good or sg list pointer is null then complete
     * the packet with error.
     */
    if((status != FBE_STATUS_OK) || (sg_list_p == NULL))
    {
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s:get packet and sg list failed, status:0x%x\n", __FUNCTION__, status);

        fbe_memory_free_request_entry(memory_request_p);

        fbe_payload_block_set_status(block_operation_p,
                                     FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID_REQUEST,
                                     FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_UNEXPECTED_ERROR);
        fbe_provision_drive_utils_complete_packet(packet_p, FBE_STATUS_OK, 0);
        return FBE_STATUS_INSUFFICIENT_RESOURCES;
    }

    new_payload_p = fbe_transport_get_payload_ex(new_packet_p);

    fbe_payload_ex_set_sg_list(new_payload_p, sg_list_p, 1);

    fbe_provision_drive_zero_paged_metadata_send_subpacket(provision_drive_p,
                                                        new_packet_p,
                                                        packet_p,
                                                        start_lba,
                                                        block_count);
    
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}

/*this function is called either the first time or from completion as a way to send the write same packet to the drive*/
static fbe_status_t fbe_provision_drive_zero_paged_metadata_send_subpacket(fbe_provision_drive_t *provision_drive_p,
                                                                           fbe_packet_t *new_packet_p,
                                                                           fbe_packet_t *original_packet_p,
                                                                           fbe_lba_t start_lba,
                                                                           fbe_block_count_t block_count)
{
    fbe_payload_ex_t *                         new_payload_p = NULL;
    fbe_payload_block_operation_t *         new_block_operation_p = NULL;
    fbe_optimum_block_size_t                optimum_block_size;
    
    new_payload_p = fbe_transport_get_payload_ex(new_packet_p);

    /* allocate and initialize the block operation. */
    fbe_block_transport_edge_get_optimum_block_size(&provision_drive_p->block_edge, &optimum_block_size);
    new_block_operation_p = fbe_payload_ex_allocate_block_operation(new_payload_p);

    /* CBE WRITE SAME */
    if(fbe_provision_drive_is_write_same_enabled(provision_drive_p, start_lba)){
        fbe_payload_block_build_operation(new_block_operation_p,
                                          FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_SAME,
                                          start_lba,
                                          block_count,
                                          FBE_BE_BYTES_PER_BLOCK,
                                          optimum_block_size,
                                          NULL);
    } else {
        /* Check that pre_sg or sg has correct count */
        if(new_payload_p->sg_list != NULL){
            if(new_payload_p->sg_list[0].count != (fbe_u32_t)(block_count * FBE_BE_BYTES_PER_BLOCK)){
                fbe_provision_drive_utils_trace(provision_drive_p,
                                                FBE_TRACE_LEVEL_CRITICAL_ERROR,
                                                FBE_TRACE_MESSAGE_ID_INFO,
                                                FBE_PROVISION_DRIVE_DEBUG_FLAG_ZOD_TRACING,
                                                "SGL count Invalid %d\n", new_payload_p->sg_list[0].count);


            }
        } else {
            if(new_payload_p->pre_sg_array[0].count != (fbe_u32_t)(block_count * FBE_BE_BYTES_PER_BLOCK)){
                fbe_provision_drive_utils_trace(provision_drive_p,
                                                FBE_TRACE_LEVEL_CRITICAL_ERROR,
                                                FBE_TRACE_MESSAGE_ID_INFO,
                                                FBE_PROVISION_DRIVE_DEBUG_FLAG_ZOD_TRACING,
                                                "pre SGL count Invalid %d\n", new_payload_p->pre_sg_array[0].count);


            }
        }

        fbe_payload_block_build_operation(new_block_operation_p,
                                          FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,
                                          start_lba,
                                          block_count,
                                          FBE_BE_BYTES_PER_BLOCK,
                                          optimum_block_size,
                                          NULL);
    }

    /* set completion functon to handle the write same request completion. */
    fbe_transport_set_completion_function(new_packet_p,
                                          fbe_provision_drive_zero_paged_metadata_send_subpacket_completion,
                                          provision_drive_p);

    fbe_transport_propagate_expiration_time(new_packet_p, original_packet_p);
    fbe_transport_add_subpacket(original_packet_p, new_packet_p);
    
    /* put packet on the termination queue while we wait for the subpackets to complete. */
    fbe_transport_set_cancel_function(original_packet_p, fbe_base_object_packet_cancel_function, (fbe_base_object_t *) provision_drive_p);

    /* send packet to the block edge */
    fbe_base_config_send_functional_packet((fbe_base_config_t *)provision_drive_p, new_packet_p, 0);

    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}

/*this function is called when the write_same operation is done. it will check if we need to continue and zero more or just 
complete the master packet which will continue with writing the data*/
static fbe_status_t fbe_provision_drive_zero_paged_metadata_send_subpacket_completion(fbe_packet_t *subpacket,
                                                                                      fbe_packet_completion_context_t packet_completion_context)
{
    fbe_packet_t *							master_packet = NULL;
    fbe_payload_ex_t *                     	payload_p = NULL;
    fbe_payload_ex_t *                      master_payload_p = NULL;
    fbe_payload_block_operation_t *         master_block_operation_p = NULL;
    fbe_payload_block_operation_status_t	block_status;
    fbe_provision_drive_t *					provision_drive_p = (fbe_provision_drive_t *)packet_completion_context;
    fbe_block_count_t						block_count;
    fbe_sg_element_t *						sg_list = NULL;
    fbe_u32_t								sg_count;
    fbe_payload_block_operation_t *			block_operation_p = NULL;
    fbe_payload_ex_t *                      new_payload_p = NULL;

    /*read status*/
    payload_p = fbe_transport_get_payload_ex(subpacket);
    block_operation_p =  fbe_payload_ex_get_block_operation(payload_p);
    fbe_payload_block_get_status(block_operation_p, &block_status);

    master_packet = fbe_transport_get_master_packet(subpacket);
    master_payload_p = fbe_transport_get_payload_ex(master_packet);
    master_block_operation_p = fbe_payload_ex_get_block_operation(master_payload_p);


    /* Remove subpacket from master queue. */
    fbe_transport_remove_subpacket(subpacket);

    /*did the zero work ?*/
    if (block_status != FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS) {

         fbe_base_object_trace((fbe_base_object_t *)provision_drive_p,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s failed to zero lba:%llX count:%llX, status:%d\n",
                               __FUNCTION__,
                   (unsigned long long)block_operation_p->lba,
                   (unsigned long long)block_operation_p->block_count,
                   block_status);

        fbe_payload_block_set_status(master_block_operation_p,
                                     block_status,
                                     0);

        fbe_payload_ex_release_block_operation(payload_p, block_operation_p);

        /*and destroy*/
        fbe_transport_destroy_packet(subpacket);
    
        /* release the preallocated memory for the request. */
        fbe_memory_free_request_entry(&master_packet->memory_request);

        fbe_provision_drive_utils_complete_packet(master_packet, FBE_STATUS_OK, 0);
        return FBE_STATUS_OK;
    }

    fbe_payload_ex_release_block_operation(payload_p, block_operation_p);

    /*the zero work, let's do another one if needed*/
    if (master_block_operation_p->block_count >= FBE_PROVISION_DRIVE_CHUNK_SIZE) {
        master_block_operation_p->block_count -= FBE_PROVISION_DRIVE_CHUNK_SIZE;
    }else{
        master_block_operation_p->block_count -= master_block_operation_p->block_count;
    }

    /*are we done ?, if not, let's see how many blocks we need to do*/
    if (master_block_operation_p->block_count != 0) {
        master_block_operation_p->lba += FBE_PROVISION_DRIVE_CHUNK_SIZE;/*move to next one*/

        if (master_block_operation_p->block_count >= FBE_PROVISION_DRIVE_CHUNK_SIZE) {
            block_count = FBE_PROVISION_DRIVE_CHUNK_SIZE;
        }else{
            block_count = master_block_operation_p->block_count;
        }

        /* Preserve SGL's */
        fbe_payload_ex_get_sg_list(payload_p, &sg_list, &sg_count);

        /*and let's call the zero function again*/
        fbe_transport_reuse_packet(subpacket);

        new_payload_p = fbe_transport_get_payload_ex(subpacket);
        fbe_payload_ex_set_sg_list(new_payload_p, sg_list, 1);

        /* CBE WRITE SAME */
        if(fbe_provision_drive_is_write_same_enabled(provision_drive_p, master_block_operation_p->lba)){ /* Write Same IS enabled */
            /* Do nothing */
        } else { /* Write Same NOT enabled */
            fbe_sg_element_t * pre_sg_list;
            fbe_sg_element_t * sg_list_p;
            sg_list_p = fbe_zero_get_bit_bucket_sgl_1MB();

            fbe_payload_ex_get_pre_sg_list(new_payload_p, &pre_sg_list);
            pre_sg_list[0].address = sg_list_p[0].address;
            pre_sg_list[0].count = (fbe_u32_t)(master_block_operation_p->block_size * block_count);
            pre_sg_list[1].address = NULL;
            pre_sg_list[1].count = 0;
        }

        fbe_provision_drive_zero_paged_metadata_send_subpacket(provision_drive_p,
                                                                subpacket, /* new_packet */
                                                                master_packet, /* original packet */
                                                                master_block_operation_p->lba,
                                                                block_count);

         return FBE_STATUS_MORE_PROCESSING_REQUIRED;/*need this to continue sending the packet down for another zero*/
        
    }else{
        /*we are done, let's clean everything and return the master packet*/
        fbe_payload_block_set_status(master_block_operation_p,
                                     FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS,
                                     0);

        /*and destroy*/
        fbe_transport_destroy_packet(subpacket);
    
        /* release the preallocated memory for the request. */
        //fbe_memory_request_get_entry_mark_free(&master_packet->memory_request, &memory_ptr);
        //fbe_memory_free_entry(memory_ptr);
        fbe_memory_free_request_entry(&master_packet->memory_request);

        fbe_provision_drive_utils_complete_packet(master_packet, FBE_STATUS_OK, 0);
        return FBE_STATUS_OK;
    }

}

/*this function zeroes out the non paged metadata area before we write into it the default bits*/
fbe_status_t fbe_provision_drive_metadata_zero_nonpaged_metadata(fbe_provision_drive_t * provision_drive_p,
                                                                          fbe_packet_t *packet_p)
{
    fbe_status_t                                    status = FBE_STATUS_GENERIC_FAILURE; 
    fbe_payload_ex_t *                             sep_payload_p = NULL;
    fbe_payload_control_operation_t *               control_operation_p = NULL;
    fbe_payload_block_operation_t *					block_operation_p = NULL;
    fbe_block_size_t 								block_size = 0;

    /* get the payload operation. */
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(sep_payload_p);

    /*we'll allocate a block payload on this packet so we can pass it as a context to the memory allocation completion
    it will contain the start and end LBA we'll use to generate the sub packets for doing the actual zero*/
    block_operation_p = fbe_payload_ex_allocate_block_operation(sep_payload_p);
    if (block_operation_p == NULL) {
        fbe_base_object_trace((fbe_base_object_t*)provision_drive_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s can't allocate block operation\n",
                              __FUNCTION__);
        
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return status;

    }

    fbe_base_config_metadata_get_paged_record_start_lba((fbe_base_config_t *) provision_drive_p, &block_operation_p->lba);
    block_operation_p->block_count = provision_drive_p->block_edge.capacity - block_operation_p->lba - 1;

    /*some sanity check before we do anything important. The code below will assume 520 block size and will
    not do preread, this should work for now but might break in the future so we want to make sure we know about it*/
    fbe_block_transport_edge_get_exported_block_size(&provision_drive_p->block_edge, &block_size);
    if (block_size != 520) {
        fbe_base_object_trace((fbe_base_object_t*)provision_drive_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s block size not equal 520\n",
                              __FUNCTION__);

        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return status;
    }
    
    fbe_transport_set_completion_function(packet_p,
                                          fbe_provision_drive_metadata_zero_nonpaged_metadata_completion,
                                          provision_drive_p);

    fbe_payload_ex_increment_block_operation_level(sep_payload_p);
    
    return fbe_provision_drive_zero_paged_metadata(provision_drive_p, packet_p);

}

static fbe_status_t fbe_provision_drive_metadata_zero_nonpaged_metadata_completion(fbe_packet_t *packet_p, fbe_packet_completion_context_t context)
{
    fbe_status_t                                    status = FBE_STATUS_GENERIC_FAILURE; 
    fbe_payload_ex_t *                              sep_payload_p = NULL;
    fbe_payload_control_operation_t *               control_operation_p = NULL;
    fbe_payload_block_operation_t *					block_operation_p = NULL;
    fbe_payload_block_operation_status_t			operation_status;
    fbe_provision_drive_t *							provision_drive_p = (fbe_provision_drive_t *)context;

    /* get the payload and block operation. */
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    block_operation_p = fbe_payload_ex_get_block_operation(sep_payload_p);
    fbe_payload_block_get_status(block_operation_p, &operation_status);

    fbe_payload_ex_release_block_operation(sep_payload_p, block_operation_p);/*we won't use it anymore*/

    control_operation_p = fbe_payload_ex_get_control_operation(sep_payload_p);

    /*check how the zero went*/
    if (operation_status != FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS) {
        fbe_base_object_trace((fbe_base_object_t*)provision_drive_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s failed to pre-zero paged metadata area\n",__FUNCTION__);

        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return status;
    }

    fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);
    return FBE_STATUS_OK;


}

/*!****************************************************************************
 * @fn fbe_provision_drive_metadata_mark_consumed()
 ******************************************************************************
 * @brief
 *  This function is used to mark pvd's paged map as consumed. It will send 
 *  set_bit request to metadata service to set the "consumed bit" for the 
 *  given LBA range.
 *
 * @param provision_drive_p     - Pointer to the provision drive object.
 * @param packet_p              - Pointer to the packet.
 *
 * @return fbe_status_t         - status of the operation.
 *
 * @author
 *  10/21/2011 - Created. Amit Dhaduk
 *
 ******************************************************************************/
fbe_status_t
fbe_provision_drive_metadata_mark_consumed(fbe_provision_drive_t * provision_drive_p,
                                                    fbe_packet_t * packet_p)
{
    fbe_status_t                            status;
    fbe_payload_ex_t *                     sep_payload_p = NULL;
    fbe_payload_block_operation_t *         block_operation_p = NULL;
    fbe_lba_t                               start_lba;
    fbe_block_count_t                       block_count;
    fbe_bool_t                          is_block_request_aligned = FBE_FALSE;
    fbe_optimum_block_size_t                optimum_block_size;
    fbe_lba_t                               exported_capacity;
 

   /* Get the sep payload and block operation. */
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    block_operation_p = fbe_payload_ex_get_present_block_operation(sep_payload_p);
    
    fbe_payload_block_get_lba(block_operation_p, &start_lba);
    fbe_payload_block_get_block_count(block_operation_p, &block_count);

    /* get the optimum block size. */
    fbe_block_transport_edge_get_optimum_block_size(&provision_drive_p->block_edge, &optimum_block_size);

    /* get the exported capacity of pvd object. */
    fbe_base_config_get_capacity((fbe_base_config_t *) provision_drive_p, &exported_capacity);


    /* check that mark consumed request is aligned. */
    status = fbe_provision_drive_utils_is_io_request_aligned(start_lba,
                                                             block_count,
                                                             FBE_PROVISION_DRIVE_CHUNK_SIZE,
                                                             &is_block_request_aligned);
    if((status != FBE_STATUS_OK) || (!is_block_request_aligned))
    {
        fbe_provision_drive_utils_trace(provision_drive_p, 
                                        FBE_TRACE_LEVEL_ERROR,
                                        FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                        FBE_PROVISION_DRIVE_DEBUG_FLAG_USER_ZERO_TRACING,
                                        "PVD mark consumed:unaligned request, status:0x%x, lba:0x%llx, blks:0x%llx, packet:%p\n",
                                        status, (unsigned long long)start_lba,
                        (unsigned long long)block_count,
                        packet_p);
        fbe_payload_block_set_status(block_operation_p,
                                     FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID_REQUEST,
                                     FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_UNALIGNED_ZERO_REQUEST);
        fbe_provision_drive_utils_complete_packet(packet_p, FBE_STATUS_OK, 0);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* we do not expect to get the mark consume request beyond the exported capacity. */
    if((start_lba + block_count) > exported_capacity)
    {
        fbe_provision_drive_utils_trace(provision_drive_p, 
                                        FBE_TRACE_LEVEL_ERROR,
                                        FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                        FBE_PROVISION_DRIVE_DEBUG_FLAG_USER_ZERO_TRACING,
                                        "PVD mark consumed, request beyond capacity, lba:0x%llx, blks:0x%llx, packet:%p\n",
                                        (unsigned long long)start_lba,
                        (unsigned long long)block_count,
                        packet_p);
        fbe_payload_block_set_status(block_operation_p,
                                     FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID_REQUEST,
                                     FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_CAPACITY_EXCEEDED);
        fbe_provision_drive_utils_complete_packet(packet_p, FBE_STATUS_OK, 0);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_provision_drive_mark_consumed_update_paged_metadata(provision_drive_p, packet_p,
                                                                     fbe_provision_drive_metadata_mark_consumed_completion, 
                                                                     FBE_FALSE);
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}

/*!****************************************************************************
 * @fn fbe_provision_drive_mark_consumed_update_paged_metadata_callback()
 ******************************************************************************
 * @brief
 *
 *  This is the callback function to clear bits for disk zeroing. 
 * 
 * @param packet            - Pointer to packet.
 * @param context           - Pointer to context.
 *
 * @return fbe_status_t.
 *  05/06/2014 - Created. Lili Chen
 *
 ******************************************************************************/
fbe_status_t 
fbe_provision_drive_mark_consumed_update_paged_metadata_callback(fbe_packet_t * packet, fbe_metadata_callback_context_t context)
{
#if 0
    fbe_payload_ex_t * sep_payload = fbe_transport_get_payload_ex(packet);
    fbe_payload_metadata_operation_t * mdo = NULL;
    fbe_provision_drive_paged_metadata_t * paged_metadata_p;
    fbe_sg_element_t * sg_list = NULL;
    fbe_sg_element_t * sg_ptr = NULL;
    fbe_lba_t lba_offset;
    fbe_u64_t slot_offset;
    fbe_bool_t is_write_requiered = FBE_FALSE;

    if(sep_payload->current_operation->payload_opcode == FBE_PAYLOAD_OPCODE_METADATA_OPERATION){
        mdo = fbe_payload_ex_get_metadata_operation(sep_payload);
    } else if(sep_payload->current_operation->payload_opcode == FBE_PAYLOAD_OPCODE_STRIPE_LOCK_OPERATION){
        mdo = fbe_payload_ex_get_any_metadata_operation(sep_payload);
    } else {
        fbe_topology_class_trace(FBE_CLASS_ID_PROVISION_DRIVE, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                      "%s: Invalid payload opcode:%d \n", __FUNCTION__, sep_payload->current_operation->payload_opcode);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    lba_offset = fbe_metadata_paged_get_lba_offset(mdo);
    slot_offset = fbe_metadata_paged_get_slot_offset(mdo);
    sg_list = mdo->u.metadata_callback.sg_list;
    sg_ptr = &sg_list[lba_offset - mdo->u.metadata_callback.start_lba];
    paged_metadata_p = (fbe_provision_drive_paged_metadata_t *)(sg_ptr->address + slot_offset);

    while ((sg_ptr->address != NULL) && (mdo->u.metadata_callback.current_count_private < mdo->u.metadata_callback.repeat_count))
    {
        if (paged_metadata_p->consumed_user_data_bit == 0) {
            is_write_requiered = FBE_TRUE;
            /* set the consumed bit if it is not set. */
            paged_metadata_p->consumed_user_data_bit = 1;
        }

        mdo->u.metadata_callback.current_count_private++;
        paged_metadata_p++;
        if (paged_metadata_p == (fbe_provision_drive_paged_metadata_t *)(sg_ptr->address + FBE_METADATA_BLOCK_DATA_SIZE))
        {
            sg_ptr++;
            paged_metadata_p = (fbe_provision_drive_paged_metadata_t *)sg_ptr->address;
        }
    }

    if (is_write_requiered) {
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    } else {
        return FBE_STATUS_OK;
    }
#endif

    fbe_provision_drive_paged_metadata_t paged_metadata;

    /* Set consumed bit */
    fbe_zero_memory(&paged_metadata, sizeof(fbe_provision_drive_paged_metadata_t));
    paged_metadata.consumed_user_data_bit = 1;

    return (fbe_provision_drive_metadata_paged_callback_set_bits(packet, &paged_metadata));
}
/******************************************************************************
 * end fbe_provision_drive_mark_consumed_update_paged_metadata_callback()
 ******************************************************************************/

/*!****************************************************************************
 * @fn fbe_provision_drive_mark_consumed_update_paged_metadata()
 ******************************************************************************
 * @brief
 *  This function is used to start zeroing request by updating the paged 
 *  metadata for the chunks which correponds to zero request.
 *
 * @param provision_drive_p     - Pointer to the provision drive object.
 * @param packet_p              - Pointer to the packet.
 *
 * @return fbe_status_t         - status of the operation.
 *
 * @author
 *  21/06/2010 - Created. Dhaval Patel
 *
 ******************************************************************************/
static fbe_status_t
fbe_provision_drive_mark_consumed_update_paged_metadata(fbe_provision_drive_t * provision_drive_p,
                                                        fbe_packet_t * packet_p,
                                                        fbe_packet_completion_function_t completion_function,
                                                        fbe_bool_t use_write_verify)
{
    fbe_status_t                            status;
    fbe_payload_ex_t *                     sep_payload_p = NULL;
    fbe_payload_block_operation_t *         block_operation_p = NULL;
    fbe_payload_metadata_operation_t *      metadata_operation_p = NULL;
    fbe_lba_t                               start_lba;
    fbe_block_count_t                       block_count;
    fbe_chunk_index_t                       start_chunk_index;
    fbe_chunk_count_t                       chunk_count= 0;
    fbe_lba_t                               metadata_offset;
    fbe_provision_drive_paged_metadata_t    paged_set_bits;
    fbe_lba_t                               metadata_start_lba;
    fbe_block_count_t                       metadata_block_count;

    fbe_payload_block_operation_opcode_t    block_opcode = FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INVALID;

   /* Get the sep payload and block operation. */
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    block_operation_p = fbe_payload_ex_get_present_block_operation(sep_payload_p);

    fbe_payload_block_get_lba(block_operation_p, &start_lba);
    fbe_payload_block_get_block_count(block_operation_p, &block_count);

    /* Next task is to first determine the chunk range which requires to update the metadata */
    fbe_provision_drive_utils_calculate_chunk_range(start_lba,
                                                                  block_count,
                                                                  FBE_PROVISION_DRIVE_CHUNK_SIZE,
                                                                  &start_chunk_index,
                                                                  &chunk_count);

    /* get the metadata offset for the chunk index. */
    metadata_offset = start_chunk_index * sizeof(fbe_provision_drive_paged_metadata_t);
    fbe_payload_block_get_opcode(block_operation_p, &block_opcode);

    /* debug trace to track the paged metadata and signature data positions. */
    if (block_opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_UNMARK_ZERO)
    {
        fbe_provision_drive_utils_trace( provision_drive_p,
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO, FBE_PROVISION_DRIVE_DEBUG_FLAG_METADATA_TRACING,
                          "PVD unmark zero start, slba 0x%llx, bcnt 0x%llx, s_chk_ind 0x%llx, chk_count 0x%x, packet:0x%p\n",
                          (unsigned long long)start_lba,
              (unsigned long long)block_count,
              (unsigned long long)start_chunk_index,
              chunk_count, packet_p) ;
    }
    else
    {
        fbe_provision_drive_utils_trace( provision_drive_p,
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO, FBE_PROVISION_DRIVE_DEBUG_FLAG_METADATA_TRACING,
                          "PVD mark consumed start, slba 0x%llx, bcnt 0x%llx, s_chk_ind 0x%llx, chk_count 0x%x, packet:0x%p\n",
                          (unsigned long long)start_lba,
              (unsigned long long)block_count,
              (unsigned long long)start_chunk_index,
              chunk_count, packet_p) ;
    }

    /*  initialize the paged set bits. */
    fbe_zero_memory(&paged_set_bits, sizeof(fbe_provision_drive_paged_metadata_t));

    /* allocate metadata operation for SET paged bit data */
    metadata_operation_p =  fbe_payload_ex_allocate_metadata_operation(sep_payload_p); 

    /* set the consume bit as TRUE */
    paged_set_bits.consumed_user_data_bit = FBE_TRUE;

    /* Set completion functon to handle the write same edge request completion. */
    fbe_transport_set_completion_function(packet_p,
                                          completion_function,
                                          provision_drive_p);

    if(use_write_verify)
    {
        /* set the consume bit as TRUE */
        paged_set_bits.valid_bit = FBE_TRUE;

#if 0
        /* Build the paged metadata set bit request. */
        fbe_payload_metadata_build_paged_write_verify(metadata_operation_p,
                                                      &(((fbe_base_config_t *) provision_drive_p)->metadata_element),
                                                      metadata_offset,
                                                      (fbe_u8_t *) &paged_set_bits,
                                                      sizeof(fbe_provision_drive_paged_metadata_t),
                                                      chunk_count);
#else
        fbe_payload_metadata_build_paged_update(metadata_operation_p,
                                              &(((fbe_base_config_t *) provision_drive_p)->metadata_element),
                                              metadata_offset,
                                              sizeof(fbe_provision_drive_paged_metadata_t),
                                              chunk_count,
                                              fbe_provision_drive_metadata_write_verify_paged_metadata_callback,
                                              (void *)metadata_operation_p);
        fbe_payload_metadata_set_metadata_operation_flags(metadata_operation_p, FBE_PAYLOAD_METADATA_OPERATION_PAGED_WRITE_VERIFY);
        /* Initialize cient_blob, skip the subpacket */
        fbe_provision_drive_metadata_paged_init_client_blob(packet_p, metadata_operation_p);
#endif
    }
    else
    {
#if 0
        /* Build the paged metadata set bit request. */
        fbe_payload_metadata_build_paged_set_bits(metadata_operation_p,
                                                 &(((fbe_base_config_t *) provision_drive_p)->metadata_element),
                                                 metadata_offset,
                                                 (fbe_u8_t *) &paged_set_bits,
                                                 sizeof(fbe_provision_drive_paged_metadata_t),
                                                 chunk_count);
    
#else
        fbe_payload_metadata_build_paged_update(metadata_operation_p,
                                              &(((fbe_base_config_t *) provision_drive_p)->metadata_element),
                                              metadata_offset,
                                              sizeof(fbe_provision_drive_paged_metadata_t),
                                              chunk_count,
                                              fbe_provision_drive_mark_consumed_update_paged_metadata_callback,
                                              (void *)metadata_operation_p);
        /* Initialize cient_blob, skip the subpacket */
        fbe_provision_drive_metadata_paged_init_client_blob(packet_p, metadata_operation_p);
#endif
    }
    /* Get the metadata stripe lock*/
    fbe_metadata_paged_get_lock_range(metadata_operation_p, &metadata_start_lba, &metadata_block_count);

    fbe_payload_metadata_set_metadata_stripe_offset(metadata_operation_p, metadata_start_lba);
    fbe_payload_metadata_set_metadata_stripe_count(metadata_operation_p, metadata_block_count);

    fbe_payload_ex_increment_metadata_operation_level(sep_payload_p);

    /* send metadata request to mark consumed pvd's paged metadata */
    status =  fbe_metadata_operation_entry(packet_p);
    return status;
}
/******************************************************************************
 * end fbe_provision_drive_metadata_mark_consumed()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_provision_drive_metadata_mark_consumed_completion()
 ******************************************************************************
 * @brief
 *   This function handles the completion of mark consume block request.
 *
 * @param packet_p          - pointer to a block operation packet 
 * @param in_context        - context, which is a pointer to the provision drive
 *                                      object
 *
 *
 * @return  fbe_status_t  
 *
 * @author
 *   10/21/2011 - Created. Amit Dhaduk
 *
 ******************************************************************************/
static fbe_status_t                                                                                                  
fbe_provision_drive_metadata_mark_consumed_completion(fbe_packet_t * packet_p,
                                                               fbe_packet_completion_context_t context)                                          
{   
    fbe_provision_drive_t *                 provision_drive_p = NULL;
    fbe_payload_metadata_operation_t *      metadata_operation_p = NULL;
    fbe_payload_ex_t *                     sep_payload_p = NULL;
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_payload_metadata_status_t           metadata_status;
    fbe_payload_block_operation_t *         block_operation_p = NULL;
    fbe_payload_block_operation_opcode_t    block_opcode = FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INVALID;
    
 
    provision_drive_p = (fbe_provision_drive_t *) context;

    /* Get the payload and control operation. */
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    metadata_operation_p =  fbe_payload_ex_get_metadata_operation(sep_payload_p);
    
    /* Get the status of the paged metadata operation. */
    fbe_payload_metadata_get_status(metadata_operation_p, &metadata_status);
    status = fbe_transport_get_status_code(packet_p);

    /* Get the block operation. */
    block_operation_p = fbe_payload_ex_get_present_block_operation(sep_payload_p);
    fbe_payload_block_get_opcode(block_operation_p, &block_opcode);


    /* Verify the metadata status and set the appropriate block status. */
    if((status != FBE_STATUS_OK) ||
       (metadata_status != FBE_PAYLOAD_METADATA_STATUS_OK))
    {
        /* Release the metadata operation. */
        fbe_payload_ex_release_metadata_operation(sep_payload_p, metadata_operation_p);
        fbe_provision_drive_utils_trace(provision_drive_p,
                                        FBE_TRACE_LEVEL_WARNING,
                                        FBE_TRACE_MESSAGE_ID_INFO,
                                        FBE_PROVISION_DRIVE_DEBUG_FLAG_GENERAL_TRACING,
                                        "PVD mark consumed failed, status 0x%x, metadata status 0x%x, packet:0x%p\n",
                                        status , metadata_status , packet_p);

        if(metadata_status == FBE_PAYLOAD_METADATA_STATUS_IO_UNCORRECTABLE)
        {
            fbe_provision_drive_mark_consumed_update_paged_metadata(provision_drive_p,packet_p,
                                                                    fbe_provision_drive_metadata_mark_consumed_write_verify_completion,
                                                                    FBE_TRUE);
            /* We might setting other region in the MD as invalid and so kick off a verify invalidate to fix those region
             */
            fbe_lifecycle_set_cond(&fbe_provision_drive_lifecycle_const, (fbe_base_object_t *) provision_drive_p,
                                   FBE_PROVISION_DRIVE_LIFECYCLE_COND_METADATA_VERIFY_INVALIDATE);

            return FBE_STATUS_MORE_PROCESSING_REQUIRED;
        }
        else
        {
            
            fbe_payload_block_set_status(block_operation_p,
                                         FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED,
                                         FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_POSSIBLE);
    
            fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
            return status;
        }

    }


    /* If it is a unmark zero request clear all the bits except CD */
    if(block_opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_UNMARK_ZERO)
    {
        status = fbe_provision_drive_metadata_unmark_zero(provision_drive_p, packet_p,
                                                          fbe_provision_drive_metadata_unmark_zero_completion,
                                                          FBE_FALSE);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }

    /* debug trace to track the metadata status. */
    fbe_provision_drive_utils_trace( provision_drive_p,
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO, FBE_PROVISION_DRIVE_DEBUG_FLAG_METADATA_TRACING,
                          "PVD mark consumed complete, status 0x%x, metadata status 0x%x, packet:0x%p\n",
                           status , metadata_status , packet_p);

    /* Release the metadata operation. */
    fbe_payload_ex_release_metadata_operation(sep_payload_p, metadata_operation_p);

    fbe_payload_block_set_status(block_operation_p,
                         FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS,
                         FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE);
    
    return status;                                                                                                        

}   
/******************************************************************************
 * end fbe_provision_drive_metadata_mark_consumed_completion()
 ******************************************************************************/
/*!****************************************************************************
 * fbe_provision_drive_metadata_mark_consumed_write_verify_completion()
 ******************************************************************************
 * @brief
 *   This function handles the completion of mark consume block request.
 *
 * @param packet_p          - pointer to a block operation packet 
 * @param in_context        - context, which is a pointer to the provision drive
 *                                      object
 *
 *
 * @return  fbe_status_t  
 *
 * @author
 *   10/21/2011 - Created. Amit Dhaduk
 *
 ******************************************************************************/
static fbe_status_t                                                                                                  
fbe_provision_drive_metadata_mark_consumed_write_verify_completion(fbe_packet_t * packet_p,
                                                                   fbe_packet_completion_context_t context)                                          
{   
    fbe_provision_drive_t *                 provision_drive_p = NULL;
    fbe_payload_metadata_operation_t *      metadata_operation_p = NULL;
    fbe_payload_ex_t *                     sep_payload_p = NULL;
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_payload_metadata_status_t           metadata_status;
    fbe_payload_block_operation_t *         block_operation_p = NULL;
    fbe_payload_block_operation_opcode_t    block_opcode = FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INVALID;
    
 
    provision_drive_p = (fbe_provision_drive_t *) context;

    /* Get the payload and control operation. */
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    metadata_operation_p =  fbe_payload_ex_get_metadata_operation(sep_payload_p);
    
    /* Get the status of the paged metadata operation. */
    fbe_payload_metadata_get_status(metadata_operation_p, &metadata_status);
    status = fbe_transport_get_status_code(packet_p);

    /* Get the block operation. */
    block_operation_p = fbe_payload_ex_get_present_block_operation(sep_payload_p);
    fbe_payload_block_get_opcode(block_operation_p, &block_opcode);


    /* Verify the metadata status and set the appropriate block status. */
    if((status != FBE_STATUS_OK) ||
       (metadata_status != FBE_PAYLOAD_METADATA_STATUS_OK))
    {
        /* Release the metadata operation. */
        fbe_payload_ex_release_metadata_operation(sep_payload_p, metadata_operation_p);
        fbe_provision_drive_utils_trace(provision_drive_p,
                                        FBE_TRACE_LEVEL_WARNING,
                                        FBE_TRACE_MESSAGE_ID_INFO,
                                        FBE_PROVISION_DRIVE_DEBUG_FLAG_GENERAL_TRACING,
                                        "PVD write verify mark consumed failed, status 0x%x, metadata status 0x%x, packet:0x%x\n",
                                        status , metadata_status , (unsigned int)packet_p);

        fbe_payload_block_set_status(block_operation_p,
                                     FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED,
                                     FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_POSSIBLE);
    
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        return status;
    }


    /* If it is a unmark zero request clear all the bits except CD */
    if(block_opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_UNMARK_ZERO)
    {
        status = fbe_provision_drive_metadata_unmark_zero(provision_drive_p, packet_p,
                                                          fbe_provision_drive_metadata_unmark_zero_completion,
                                                          FBE_FALSE);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }

    /* debug trace to track the metadata status. */
    fbe_provision_drive_utils_trace( provision_drive_p,
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO, FBE_PROVISION_DRIVE_DEBUG_FLAG_METADATA_TRACING,
                          "PVD mark consumed complete, status 0x%x, metadata status 0x%x, packet:0x%x\n",
                           status , metadata_status , (unsigned int)packet_p);

    /* Release the metadata operation. */
    fbe_payload_ex_release_metadata_operation(sep_payload_p, metadata_operation_p);

    fbe_payload_block_set_status(block_operation_p,
                         FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS,
                         FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE);
    
    return status;                                                                                                        

}   
/******************************************************************************
 * end fbe_provision_drive_metadata_mark_consumed_write_verify_completion()
 ******************************************************************************/

/*!****************************************************************************
 * @fn fbe_provision_drive_metadata_unmark_zero_callback()
 ******************************************************************************
 * @brief
 *
 *  This is the callback function to clear bits for unmark zero. 
 * 
 * @param packet            - Pointer to packet.
 * @param context           - Pointer to context.
 *
 * @return fbe_status_t.
 *  05/06/2014 - Created. Lili Chen
 *
 ******************************************************************************/
fbe_status_t 
fbe_provision_drive_metadata_unmark_zero_callback(fbe_packet_t * packet, fbe_metadata_callback_context_t context)
{
#if 0
    fbe_payload_ex_t * sep_payload = fbe_transport_get_payload_ex(packet);
    fbe_payload_metadata_operation_t * mdo = NULL;
    fbe_provision_drive_paged_metadata_t * paged_metadata_p;
    fbe_sg_element_t * sg_list = NULL;
    fbe_sg_element_t * sg_ptr = NULL;
    fbe_lba_t lba_offset;
    fbe_u64_t slot_offset;
    fbe_bool_t is_write_requiered = FBE_FALSE;

    if(sep_payload->current_operation->payload_opcode == FBE_PAYLOAD_OPCODE_METADATA_OPERATION){
        mdo = fbe_payload_ex_get_metadata_operation(sep_payload);
    } else if(sep_payload->current_operation->payload_opcode == FBE_PAYLOAD_OPCODE_STRIPE_LOCK_OPERATION){
        mdo = fbe_payload_ex_get_any_metadata_operation(sep_payload);
    } else {
        fbe_topology_class_trace(FBE_CLASS_ID_PROVISION_DRIVE, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                      "%s: Invalid payload opcode:%d \n", __FUNCTION__, sep_payload->current_operation->payload_opcode);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    lba_offset = fbe_metadata_paged_get_lba_offset(mdo);
    slot_offset = fbe_metadata_paged_get_slot_offset(mdo);
    sg_list = mdo->u.metadata_callback.sg_list;
    sg_ptr = &sg_list[lba_offset - mdo->u.metadata_callback.start_lba];
    paged_metadata_p = (fbe_provision_drive_paged_metadata_t *)(sg_ptr->address + slot_offset);

    while ((sg_ptr->address != NULL) && (mdo->u.metadata_callback.current_count_private < mdo->u.metadata_callback.repeat_count))
    {
        if (paged_metadata_p->need_zero_bit || paged_metadata_p->user_zero_bit) {
            is_write_requiered = FBE_TRUE;
            /* clear the need zero bit if it is set. */
            paged_metadata_p->need_zero_bit = 0;
            /* clear the user zero bit if it is set. */
            paged_metadata_p->user_zero_bit = 0;
        }

        mdo->u.metadata_callback.current_count_private++;
        paged_metadata_p++;
        if (paged_metadata_p == (fbe_provision_drive_paged_metadata_t *)(sg_ptr->address + FBE_METADATA_BLOCK_DATA_SIZE))
        {
            sg_ptr++;
            paged_metadata_p = (fbe_provision_drive_paged_metadata_t *)sg_ptr->address;
        }
    }

    if (is_write_requiered) {
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    } else {
        return FBE_STATUS_OK;
    }
#endif

    fbe_provision_drive_paged_metadata_t paged_metadata;

    /* Clear NZ bit and UZ bit */
    fbe_zero_memory(&paged_metadata, sizeof(fbe_provision_drive_paged_metadata_t));
    paged_metadata.need_zero_bit = 1; 
    paged_metadata.user_zero_bit = 1;

    return (fbe_provision_drive_metadata_paged_callback_clear_bits(packet, &paged_metadata));
}
/******************************************************************************
 * end fbe_provision_drive_metadata_unmark_zero_callback()
 ******************************************************************************/

/*!****************************************************************************
 * @fn fbe_provision_drive_metadata_unmark_zero()
 ******************************************************************************
 * @brief
 *  This function is used to start zeroing request by updating the paged 
 *  metadata for the chunks which correponds to zero request.
 *
 * @param provision_drive_p     - Pointer to the provision drive object.
 * @param packet_p              - Pointer to the packet.
 *
 * @return fbe_status_t         - status of the operation.
 *
 * @author
 *  21/06/2010 - Created. Dhaval Patel
 *
 ******************************************************************************/
static fbe_status_t
fbe_provision_drive_metadata_unmark_zero(fbe_provision_drive_t * provision_drive_p,
                                         fbe_packet_t * packet_p,
                                         fbe_packet_completion_function_t completion_function,
                                         fbe_bool_t use_write_verify)
{
    fbe_status_t                            status;
    fbe_payload_ex_t *                     sep_payload_p = NULL;
    fbe_payload_block_operation_t *         block_operation_p = NULL;
    fbe_payload_metadata_operation_t *      metadata_operation_p = NULL;
    fbe_lba_t                               start_lba;
    fbe_block_count_t                       block_count;
    fbe_chunk_index_t                       start_chunk_index;
    fbe_chunk_count_t                       chunk_count= 0;
    fbe_lba_t                               metadata_offset;
    fbe_lba_t                               metadata_start_lba;
    fbe_block_count_t                       metadata_block_count;
    fbe_provision_drive_paged_metadata_t    paged_clear_bits;


    /* Get the payload and control operation. */
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    metadata_operation_p =  fbe_payload_ex_get_metadata_operation(sep_payload_p);

    block_operation_p = fbe_payload_ex_get_present_block_operation(sep_payload_p);

    fbe_payload_block_get_lba(block_operation_p, &start_lba);
    fbe_payload_block_get_block_count(block_operation_p, &block_count);
    
    /* Next task is to first determine the chunk range which requires to update the metadata */
    fbe_provision_drive_utils_calculate_chunk_range(start_lba,
                                                    block_count,
                                                    FBE_PROVISION_DRIVE_CHUNK_SIZE,
                                                    &start_chunk_index,
                                                    &chunk_count);

    /* get the metadata offset for the chuk index. */
    metadata_offset = start_chunk_index * sizeof(fbe_provision_drive_paged_metadata_t);

    /*  initialize the paged clear bits with user zero bit and consumed bit as true. */
    fbe_zero_memory(&paged_clear_bits, sizeof(fbe_provision_drive_paged_metadata_t));

    /* Set completion functon to handle the write same edge request completion. */
    fbe_transport_set_completion_function(packet_p,
                                          completion_function,
                                          provision_drive_p);

    if (use_write_verify)
    {
#if 0
        paged_clear_bits.valid_bit = FBE_TRUE;
        paged_clear_bits.consumed_user_data_bit = FBE_TRUE;
        /* Build the paged metadata set bit request. */
        fbe_payload_metadata_build_paged_write_verify(metadata_operation_p,
                                                      &(((fbe_base_config_t *) provision_drive_p)->metadata_element),
                                                      metadata_offset,
                                                      (fbe_u8_t *) &paged_clear_bits,
                                                      sizeof(fbe_provision_drive_paged_metadata_t),
                                                      chunk_count);
#else
        fbe_payload_metadata_build_paged_update(metadata_operation_p,
                                              &(((fbe_base_config_t *) provision_drive_p)->metadata_element),
                                              metadata_offset,
                                              sizeof(fbe_provision_drive_paged_metadata_t),
                                              chunk_count,
                                              fbe_provision_drive_metadata_write_verify_paged_metadata_callback,
                                              (void *)metadata_operation_p);
        fbe_payload_metadata_set_metadata_operation_flags(metadata_operation_p, FBE_PAYLOAD_METADATA_OPERATION_PAGED_WRITE_VERIFY);
        /* Initialize cient_blob, skip the subpacket */
        fbe_provision_drive_metadata_paged_init_client_blob(packet_p, metadata_operation_p);
#endif
    } 
    else
    {
#if 0
        paged_clear_bits.need_zero_bit = FBE_TRUE;
        paged_clear_bits.user_zero_bit = FBE_TRUE;
        /* Build the paged metadata set bit request. */
        fbe_payload_metadata_build_paged_clear_bits(metadata_operation_p,
                                                    &(((fbe_base_config_t *) provision_drive_p)->metadata_element),
                                                    metadata_offset,
                                                    (fbe_u8_t *) &paged_clear_bits,
                                                    sizeof(fbe_provision_drive_paged_metadata_t),
                                                    chunk_count);
#else
        fbe_payload_metadata_build_paged_update(metadata_operation_p,
                                              &(((fbe_base_config_t *) provision_drive_p)->metadata_element),
                                              metadata_offset,
                                              sizeof(fbe_provision_drive_paged_metadata_t),
                                              chunk_count,
                                              fbe_provision_drive_metadata_unmark_zero_callback,
                                              (void *)metadata_operation_p);
        /* Initialize cient_blob, skip the subpacket */
        fbe_provision_drive_metadata_paged_init_client_blob(packet_p, metadata_operation_p);
#endif
    }
    /* Get the metadata stripe lock*/
    fbe_metadata_paged_get_lock_range(metadata_operation_p, &metadata_start_lba, &metadata_block_count);

    fbe_payload_metadata_set_metadata_stripe_offset(metadata_operation_p, metadata_start_lba);
    fbe_payload_metadata_set_metadata_stripe_count(metadata_operation_p, metadata_block_count);

    fbe_payload_ex_increment_metadata_operation_level(sep_payload_p);
    status =  fbe_metadata_operation_entry(packet_p);
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}

/*!****************************************************************************
 * fbe_provision_drive_metadata_unmark_zero_completion()
 ******************************************************************************
 * @brief
 *   This function handles the completion of unmark zero block request.
 *
 * @param packet_p          - pointer to a block operation packet 
 * @param in_context        - context, which is a pointer to the provision drive
 *                                      object
 *
 *
 * @return  fbe_status_t  
 *
 * @author
 *   01/20/2012 - Created. Chenl6
 *
 ******************************************************************************/
static fbe_status_t                                                                                                  
fbe_provision_drive_metadata_unmark_zero_completion(fbe_packet_t * packet_p,
                                                    fbe_packet_completion_context_t context)                                          
{   
    fbe_provision_drive_t *                 provision_drive_p = NULL;
    fbe_payload_metadata_operation_t *      metadata_operation_p = NULL;
    fbe_payload_ex_t *                     sep_payload_p = NULL;
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_payload_metadata_status_t           metadata_status;
    fbe_payload_block_operation_t *         block_operation_p = NULL;

 
    provision_drive_p = (fbe_provision_drive_t *) context;

    /* Get the payload and control operation. */
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    metadata_operation_p =  fbe_payload_ex_get_metadata_operation(sep_payload_p);
    
    /* Get the status of the paged metadata operation. */
    fbe_payload_metadata_get_status(metadata_operation_p, &metadata_status);
    status = fbe_transport_get_status_code(packet_p);

    /* Get the block operation. */
    block_operation_p = fbe_payload_ex_get_present_block_operation(sep_payload_p);


    /* Verify the metadata status and set the appropriate block status. */
    if((status != FBE_STATUS_OK) || (metadata_status != FBE_PAYLOAD_METADATA_STATUS_OK))
    {
        fbe_provision_drive_utils_trace(provision_drive_p,
                                        FBE_TRACE_LEVEL_WARNING,
                                        FBE_TRACE_MESSAGE_ID_INFO,
                                        FBE_PROVISION_DRIVE_DEBUG_FLAG_GENERAL_TRACING,
                                        "PVD unmark zero failed, status 0x%x, metadata status 0x%x, packet:0x%p\n",
                                        status , metadata_status , packet_p);

        if(metadata_status == FBE_PAYLOAD_METADATA_STATUS_IO_UNCORRECTABLE)
        {
            fbe_provision_drive_metadata_unmark_zero(provision_drive_p,packet_p,
                                                     fbe_provision_drive_metadata_unmark_zero_write_verify_completion,
                                                     FBE_TRUE);
            /* We might setting other region in the MD as invalid and so kick off a verify invalidate to fix those region
             */
            fbe_lifecycle_set_cond(&fbe_provision_drive_lifecycle_const, (fbe_base_object_t *) provision_drive_p,
                                   FBE_PROVISION_DRIVE_LIFECYCLE_COND_METADATA_VERIFY_INVALIDATE);

            return FBE_STATUS_MORE_PROCESSING_REQUIRED;
        }
        else
        {
            /* Release the metadata operation. */
            fbe_payload_ex_release_metadata_operation(sep_payload_p, metadata_operation_p);

            fbe_payload_block_set_status(block_operation_p,
                                         FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED,
                                         FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_POSSIBLE);

            fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
            return status;
        }

    }

    /* Release the metadata operation. */
    fbe_payload_ex_release_metadata_operation(sep_payload_p, metadata_operation_p);
    /* debug trace to track the metadata status. */
    fbe_provision_drive_utils_trace( provision_drive_p,
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO, FBE_PROVISION_DRIVE_DEBUG_FLAG_METADATA_TRACING,
                          "PVD unmark zero complete, status 0x%x, metadata status 0x%x, packet:0x%p\n",
                           status , metadata_status , packet_p);

    fbe_payload_block_set_status(block_operation_p,
                         FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS,
                         FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE);
    
    return status;                                                                                                        

}   
/******************************************************************************
 * end fbe_provision_drive_metadata_unmark_zero_completion()
 ******************************************************************************/
/*!****************************************************************************
 * fbe_provision_drive_metadata_unmark_zero_write_verify_completion()
 ******************************************************************************
 * @brief
 *   This function handles the completion of unmark zero block request.
 *
 * @param packet_p          - pointer to a block operation packet 
 * @param in_context        - context, which is a pointer to the provision drive
 *                                      object
 *
 *
 * @return  fbe_status_t  
 *
 * @author
 *   01/20/2012 - Created. Chenl6
 *
 ******************************************************************************/
static fbe_status_t                                                                                                  
fbe_provision_drive_metadata_unmark_zero_write_verify_completion(fbe_packet_t * packet_p,
                                                                 fbe_packet_completion_context_t context)                                          
{   
    fbe_provision_drive_t *                 provision_drive_p = NULL;
    fbe_payload_metadata_operation_t *      metadata_operation_p = NULL;
    fbe_payload_ex_t *                     sep_payload_p = NULL;
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_payload_metadata_status_t           metadata_status;
    fbe_payload_block_operation_t *         block_operation_p = NULL;

 
    provision_drive_p = (fbe_provision_drive_t *) context;

    /* Get the payload and control operation. */
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    metadata_operation_p =  fbe_payload_ex_get_metadata_operation(sep_payload_p);
    
    /* Get the status of the paged metadata operation. */
    fbe_payload_metadata_get_status(metadata_operation_p, &metadata_status);
    status = fbe_transport_get_status_code(packet_p);

    /* Get the block operation. */
    block_operation_p = fbe_payload_ex_get_present_block_operation(sep_payload_p);

        /* Release the metadata operation. */
    fbe_payload_ex_release_metadata_operation(sep_payload_p, metadata_operation_p);

    /* Verify the metadata status and set the appropriate block status. */
    if((status != FBE_STATUS_OK) || (metadata_status != FBE_PAYLOAD_METADATA_STATUS_OK))
    {
        fbe_provision_drive_utils_trace(provision_drive_p,
                                        FBE_TRACE_LEVEL_WARNING,
                                        FBE_TRACE_MESSAGE_ID_INFO,
                                        FBE_PROVISION_DRIVE_DEBUG_FLAG_GENERAL_TRACING,
                                        "PVD unmark zero write verify failed, status 0x%x, metadata status 0x%x, packet:0x%x\n",
                                        status , metadata_status , (unsigned int)packet_p);

        fbe_payload_ex_release_metadata_operation(sep_payload_p, metadata_operation_p);

        fbe_payload_block_set_status(block_operation_p,
                                     FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED,
                                     FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_POSSIBLE);

        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        return status;
   }

    /* debug trace to track the metadata status. */
    fbe_provision_drive_utils_trace( provision_drive_p,
                                     FBE_TRACE_LEVEL_INFO,
                                     FBE_TRACE_MESSAGE_ID_INFO, FBE_PROVISION_DRIVE_DEBUG_FLAG_METADATA_TRACING,
                                     "PVD unmark zero write verify complete, status 0x%x, metadata status 0x%x, packet:0x%x\n",
                                     status , metadata_status , (unsigned int)packet_p);

    fbe_payload_block_set_status(block_operation_p,
                         FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS,
                         FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE);
    
    return status;                                                                                                        

}   
/******************************************************************************
 * end fbe_provision_drive_metadata_unmark_zero_write_verify_completion()
 ******************************************************************************/


/*!****************************************************************************
 * fbe_provision_drive_metadata_get_time_stamp()
 ******************************************************************************
 *
 * @brief
 *    This function gets the time stamp in the non-paged metadata
 *    for the specified provision drive.
 *
 * @param   in_provision_drive_p     -  pointer to provision drive
 * @param   out_time_stamp  -  time stamp output parameter
 *
 * @return  fbe_status_t             - return status of the operation. 
 *
 * @version
 *    01/09/2012 - Created. zhangy
 *
 ******************************************************************************/
fbe_status_t 
fbe_provision_drive_metadata_get_time_stamp(fbe_provision_drive_t * in_provision_drive,
                                                         fbe_system_time_t *out_time_stamp_p)
{
    fbe_provision_drive_nonpaged_metadata_t * provision_drive_nonpaged_metadata = NULL;
    if(out_time_stamp_p == NULL)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_base_config_get_nonpaged_metadata_ptr((fbe_base_config_t *) in_provision_drive, (void **)&provision_drive_nonpaged_metadata);

    /* get time stamp from provision drive's non-paged metadata */
    
    fbe_copy_memory(out_time_stamp_p,&provision_drive_nonpaged_metadata->remove_timestamp,sizeof(fbe_system_time_t));
    return FBE_STATUS_OK;
} 

/*!****************************************************************************
 * fbe_provision_drive_metadata_set_time_stamp()
 ******************************************************************************
 *
 * @brief
 *    This function sets the time stamp in the non-paged metadata
 *    for the specified provision drive.
 *
 * @param   in_provision_drive_p     -  pointer to provision drive
 * @param   out_time_stamp  -  time stamp output parameter
 *
 * @return  fbe_status_t             - return status of the operation. 
 *
 * @version
 *    01/09/2012 - Created. zhangy
 *
 ******************************************************************************/
fbe_status_t fbe_provision_drive_metadata_set_time_stamp(fbe_provision_drive_t * provision_drive_p,
                                                                      fbe_packet_t * packet_p,
                                                                      fbe_bool_t is_clear)
{
    fbe_status_t  status;
    fbe_system_time_t time_stamp;	
    if(is_clear == FBE_TRUE) {
        
        fbe_zero_memory(&time_stamp,sizeof(fbe_system_time_t));
            
        fbe_provision_drive_utils_trace(provision_drive_p,
                                        FBE_TRACE_LEVEL_INFO,
                                        FBE_TRACE_MESSAGE_ID_INFO,
                                        FBE_PROVISION_DRIVE_DEBUG_FLAG_GENERAL_TRACING,
                                        "clear PVD removedtime stamp: object ID: %X\n",
                                        provision_drive_p->base_config.base_object.object_id);

    }else{
        fbe_get_system_time(&time_stamp);
		fbe_provision_drive_utils_trace(provision_drive_p,
										FBE_TRACE_LEVEL_INFO,
										FBE_TRACE_MESSAGE_ID_INFO,
										FBE_PROVISION_DRIVE_DEBUG_FLAG_GENERAL_TRACING,
										"xxx set PVD removedtime stamp: object ID: %X\n",
										provision_drive_p->base_config.base_object.object_id);
    }

    /* sending request to metadata service to set time stamp */
   status = fbe_base_config_metadata_nonpaged_write_persist((fbe_base_config_t *) provision_drive_p,
                                                    packet_p,
                                                    (fbe_u64_t)(&((fbe_provision_drive_nonpaged_metadata_t*)0)->remove_timestamp),
                                                    (fbe_u8_t *)&time_stamp,
                                                    sizeof(fbe_system_time_t));
     
                                                
    return status;
    

}
/******************************************************************************
 * end  fbe_provision_drive_metadata_set_time_stamp()
 ******************************************************************************/
fbe_status_t fbe_provision_drive_metadata_ex_set_time_stamp(fbe_provision_drive_t * provision_drive_p,
                                                                          fbe_packet_t * packet_p,
                                                                          fbe_system_time_t time_stamp,
                                                                          fbe_bool_t is_persist)
{
    fbe_status_t  status;
    
    if(is_persist){
        
        /* sending request to metadata service to set time stamp */
     status = fbe_base_config_metadata_nonpaged_write_persist((fbe_base_config_t *) provision_drive_p,
                                                        packet_p,
                                                        (fbe_u64_t)(&((fbe_provision_drive_nonpaged_metadata_t*)0)->remove_timestamp),
                                                        (fbe_u8_t *)&time_stamp,
                                                        sizeof(fbe_system_time_t));
    }else{
     
     status = fbe_base_config_metadata_nonpaged_write((fbe_base_config_t *) provision_drive_p,
                                                        packet_p,
                                                        (fbe_u64_t)(&((fbe_provision_drive_nonpaged_metadata_t*)0)->remove_timestamp),
                                                        (fbe_u8_t *)&time_stamp,
                                                        sizeof(fbe_system_time_t));
    }
                                                    
    return status;
        
    
}

/*!****************************************************************************
 * fbe_provision_drive_metadata_get_md_chunk_index_for_lba()
 ******************************************************************************
 *
 * @brief
 *    This function gets the MD Chunk Index for the specified lba 
 *
 * @param   in_provision_drive_p     -  pointer to provision drive
 * @param   in_media_error_lba       -  The LBA that was taking errors
 * @param   chunk_index              - Buffer to store the chunk index value
 *                                   FBE_CHUNK_INDEX_INVALID if this lba is not found
 *
 * @return  fbe_status_t             - return status of the operation. 
 *
 * @version
 *    02/28/2012 - Created. Ashok Tamilarasan
 *
 ******************************************************************************/
fbe_status_t fbe_provision_drive_metadata_get_md_chunk_index_for_lba(fbe_provision_drive_t * in_provision_drive_p,
                                                                     fbe_lba_t in_media_error_lba,
                                                                     fbe_chunk_index_t *chunk_index)
{
    fbe_lba_t start_lba;
    fbe_lba_t exported_capacity;
    fbe_lba_t total_capacity;
    fbe_lba_t paged_metadata_capacity;

    /* get the paged metadata start lba of the pvd object. */
    fbe_base_config_metadata_get_paged_record_start_lba((fbe_base_config_t *) in_provision_drive_p,
                                                        &start_lba);
    // get exported capacity for this provision drive
    //fbe_base_config_get_capacity((fbe_base_config_t *) in_provision_drive_p, &exported_capacity);
    fbe_provision_drive_get_user_capacity(in_provision_drive_p, &exported_capacity);
    fbe_base_config_metadata_get_paged_metadata_capacity((fbe_base_config_t *)in_provision_drive_p,
                                                         &paged_metadata_capacity);
    total_capacity = exported_capacity + paged_metadata_capacity;

    *chunk_index = FBE_CHUNK_INDEX_INVALID;

    if(in_media_error_lba < total_capacity)
    {
        *chunk_index = (in_media_error_lba - start_lba) / FBE_PROVISION_DRIVE_CHUNK_SIZE;
    }
    else
    {
        fbe_provision_drive_utils_trace( in_provision_drive_p,
                                         FBE_TRACE_LEVEL_WARNING,
                                         FBE_TRACE_MESSAGE_ID_INFO, 
                                         FBE_PROVISION_DRIVE_DEBUG_FLAG_GENERAL_TRACING,
                                         "%s: LBA:0x%llx is > that cap:0x%llx\n", 
                                         __FUNCTION__, in_media_error_lba, total_capacity);
    }

    return FBE_STATUS_OK;
}
/******************************************************************************
 * end  fbe_provision_drive_metadata_get_md_chunk_index_for_lba()
 ******************************************************************************/
/*!****************************************************************************
 * fbe_provision_drive_metadata_is_paged_metadata_valid()
 ******************************************************************************
 * @brief
 *   This function checks if the paged record is valid or not by looking at the
 *   valid bits
 *
 * @param provision_drive_p         - pointer to the provision drive
 * @param paged_data_bits_p         - Buffer that has the paged record read
 * @param is_paged_data_valid       - Buffer to return if record is valid or not
 *
 * @return  fbe_status_t  
 *
 * @author
 *   03/06/2012 - Created. Ashok Tamilarasan
 *
 ******************************************************************************/
fbe_status_t fbe_provision_drive_metadata_is_paged_metadata_valid(fbe_lba_t host_start_lba,
                                                                  fbe_block_count_t host_block_count,
                                                                  fbe_provision_drive_paged_metadata_t * paged_data_bits_p, 
                                                                  fbe_bool_t * is_paged_data_valid)
{
    fbe_chunk_index_t   start_chunk_index;
    fbe_chunk_count_t   chunk_count;
    fbe_chunk_index_t   next_invalid_index_in_paged_bits = FBE_CHUNK_INDEX_INVALID;
    fbe_chunk_size_t    chunk_size = FBE_PROVISION_DRIVE_CHUNK_SIZE;

    /* initialize the media read needed as false. */
    *is_paged_data_valid = FBE_TRUE;

    /* get the start chunk index and chunk count from the start lba and end lba of the range. */
    fbe_provision_drive_utils_calculate_chunk_range(host_start_lba,
                                                    host_block_count,
                                                    chunk_size,
                                                    &start_chunk_index,
                                                    &chunk_count);

    /* get the next zeroed chunk in paged data bits. */
    fbe_provision_drive_bitmap_get_next_invalid_chunk(paged_data_bits_p,
                                                     0,
                                                     chunk_count,
                                                     &next_invalid_index_in_paged_bits);

    if(next_invalid_index_in_paged_bits == FBE_CHUNK_INDEX_INVALID)
    {
        /* we do not have any chunk which is invalid */
        return FBE_STATUS_OK;
    }

    /* set the media read needed as true. */
    *is_paged_data_valid = FBE_FALSE;
    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_provision_drive_metadata_is_paged_metadata_valid()
 **************************************/
/*!**************************************************************
 * fbe_provision_drive_metadata_verify_invalidate_is_need_to_run()
 ****************************************************************
 * @brief
 *  This function checks the metadata verify invalidate checkpoint
 * to determine if we need to invalidate any user region.
 *
 * @param provision_drive_p           - pointer to the provision drive
 *
 * @return bool status - returns FBE_TRUE if the operation needs to run
 *                       otherwise returns FBE_FALSE
 *
 * @author
 *  03/09/2012 - Created. Ashok Tamilarasan
 *
 ****************************************************************/
fbe_bool_t
fbe_provision_drive_metadata_verify_invalidate_is_need_to_run(fbe_provision_drive_t * provision_drive_p)
{
    fbe_lba_t verify_invalidate_checkpoint;
    fbe_status_t    status;
    fbe_provision_drive_config_type_t     config_type;

    fbe_provision_drive_get_config_type(provision_drive_p, &config_type);
    if (config_type == FBE_PROVISION_DRIVE_CONFIG_TYPE_EXT_POOL) {
        return FBE_FALSE;
    }

    /* If fail to get metadata verify checkpoint, then return false. */
    status = fbe_provision_drive_metadata_get_verify_invalidate_checkpoint(provision_drive_p, &verify_invalidate_checkpoint);
    if (status != FBE_STATUS_OK) 
    {
        fbe_provision_drive_utils_trace( provision_drive_p,
                     FBE_TRACE_LEVEL_WARNING,
                     FBE_TRACE_MESSAGE_ID_INFO, 
                     FBE_PROVISION_DRIVE_DEBUG_FLAG_GENERAL_TRACING,
                     "pvd_md_verify_inv_is_need_to_run: Failed to get checkpoint. Status: 0x%X\n", status);

        return FBE_FALSE;
    }

    /* CBE Unbound encrypted drive should not run Metadata operations for now */
    if(fbe_base_config_is_encrypted_mode((fbe_base_config_t *)provision_drive_p)) {
        fbe_provision_drive_get_config_type(provision_drive_p, &config_type);

        /* Currently "unbound" PVD will not have keys  and will not run BGZ*/
        if(config_type != FBE_PROVISION_DRIVE_CONFIG_TYPE_RAID){
            return FBE_FALSE;
        }
    }

    if(verify_invalidate_checkpoint != FBE_LBA_INVALID)
    {
        fbe_provision_drive_utils_trace( provision_drive_p,
                                         FBE_TRACE_LEVEL_DEBUG_LOW,
                                         FBE_TRACE_MESSAGE_ID_INFO, 
                                         FBE_PROVISION_DRIVE_DEBUG_FLAG_GENERAL_TRACING,
                                         "%s, chkpoint:0x%llx\n", __FUNCTION__, verify_invalidate_checkpoint);
        return FBE_TRUE;
    }

    return FBE_FALSE;
}
/******************************************************************************
 * end fbe_provision_drive_metadata_verify_invalidate_is_need_to_run()
 ******************************************************************************/
/*!****************************************************************************
 * fbe_provision_drive_metadata_set_md_verify_invalid_chkpt_without_persist()
 ******************************************************************************
 * @brief
 *   This function is used to set the metadata verify invalidate checkpoint in 
 *   non paged metadata.
 *
 * @param provision_drive_p         - pointer to the provision drive
 * @param packet_p                  - pointer to a monitor packet.
 * @param verify_invalidate_checkpoint- verify invalidate checkpoint.
 *
 * @return  fbe_status_t  
 *
 * @author
 *   03/09/2012 - Created. Ashok Tamilarasan
 *
 ******************************************************************************/
fbe_status_t 
fbe_provision_drive_metadata_set_verify_invalidate_chkpt_without_persist(fbe_provision_drive_t * provision_drive_p, 
                                                                         fbe_packet_t * packet_p,
                                                                         fbe_lba_t verify_invalidate_checkpoint)
{
    fbe_status_t    status;
    
    /*! @todo Need to handle failure case.
     */
    status = fbe_base_config_metadata_nonpaged_write((fbe_base_config_t *) provision_drive_p,
                                                     packet_p,
                                                     (fbe_u64_t)(&((fbe_provision_drive_nonpaged_metadata_t*)0)->verify_invalidate_checkpoint),
                                                     (fbe_u8_t *)&verify_invalidate_checkpoint,
                                                     sizeof(fbe_lba_t));
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/******************************************************************************
 * end fbe_provision_drive_metadata_set_verify_invalidate_chkpt_without_persist()
 ******************************************************************************/
/*!****************************************************************************
* fbe_provision_drive_metadata_set_default_nonpaged_metadata()
******************************************************************************
* @brief
*	 This function is used to set and persist the metadata with default values for 
*	 the non paged metadata.
*
* @param provision_drive_p 		- pointer to the provision drive
* @param packet_p				- pointer to a monitor packet.
*
* @return	fbe_status_t  
*
* @author
*	 5/15/2012 - Created. zhangy26
*
******************************************************************************/
fbe_status_t 
fbe_provision_drive_metadata_set_default_nonpaged_metadata(fbe_provision_drive_t *provision_drive_p,
                                                                               fbe_packet_t *packet_p)
{
    fbe_status_t							status = FBE_STATUS_OK;
    fbe_provision_drive_nonpaged_metadata_t provision_drive_nonpaged_metadata;
    fbe_lba_t								default_offset;
    fbe_object_id_t 						object_id;
    /*TODO: this two varioubles is temperaly unsed in case we enable background zeroing by default*/
    fbe_bool_t   b_enable_background_zeroing = FBE_TRUE;
    fbe_bool_t   b_enable_system_drive_background_zeroing = FBE_TRUE;	
      
    /* We must zero the local structure since it will be used to set all the
    * default metadata values.
    */
    fbe_zero_memory(&provision_drive_nonpaged_metadata, sizeof(fbe_provision_drive_nonpaged_metadata_t));
    
    /* Now set the default non-paged metadata.
    */
    status = fbe_base_config_set_default_nonpaged_metadata((fbe_base_config_t *)provision_drive_p,
                                                               (fbe_base_config_nonpaged_metadata_t *)&provision_drive_nonpaged_metadata);

    fbe_base_config_nonpaged_metadata_set_np_state((fbe_base_config_nonpaged_metadata_t *)&provision_drive_nonpaged_metadata, 
                                                   FBE_NONPAGED_METADATA_INITIALIZED);
    if (status != FBE_STATUS_OK)
    {
        /* Trace an error and return.
        */
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p,
                                 FBE_TRACE_LEVEL_ERROR, 
                                 FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                 "%s Set default base config nonpaged failed status: 0x%x\n",
                                 __FUNCTION__, status);

        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);

        return status;
    }	
    
    /* Find where the drive really starts.
    */
    fbe_base_config_get_default_offset((fbe_base_config_t *)provision_drive_p, &default_offset);
    
    /* Get pvd object id. 
    */
    fbe_base_object_get_object_id((fbe_base_object_t *) provision_drive_p, &object_id);
    
    /* Initialize the nonpaged metadata with default values. 
    */
    provision_drive_nonpaged_metadata.sniff_verify_checkpoint = 0;
    provision_drive_nonpaged_metadata.pass_count = 0;
    provision_drive_nonpaged_metadata.zero_on_demand = FBE_TRUE;
    provision_drive_nonpaged_metadata.end_of_life_state = FBE_FALSE;
    //provision_drive_nonpaged_metadata.drive_fault_state = FBE_FALSE;
    provision_drive_nonpaged_metadata.media_error_lba = FBE_LBA_INVALID;
    provision_drive_nonpaged_metadata.verify_invalidate_checkpoint = FBE_LBA_INVALID;
    provision_drive_nonpaged_metadata.nonpaged_drive_info.port_number = FBE_PORT_NUMBER_INVALID;
    provision_drive_nonpaged_metadata.nonpaged_drive_info.enclosure_number = FBE_ENCLOSURE_NUMBER_INVALID;
    provision_drive_nonpaged_metadata.nonpaged_drive_info.slot_number = FBE_SLOT_NUMBER_INVALID;
    provision_drive_nonpaged_metadata.nonpaged_drive_info.drive_type = FBE_DRIVE_TYPE_INVALID;
    /* default time stamp is zero */
    fbe_zero_memory(&provision_drive_nonpaged_metadata.remove_timestamp,sizeof(fbe_system_time_t));
    /* set default zero checkpoint */
    if(fbe_database_is_object_system_pvd(object_id))	
    {
        /* for system drive pvd, set the default zero checkpoint to 0th LBA so that background zeroing operation can
        * zero the system luns and system raid groups area.
        */
        provision_drive_nonpaged_metadata.zero_checkpoint = 0;
    }
    else
    {
        /* for non system drive pvd, set the default zero checkpoint to the default offset because background zeroing
            * does not need to touch the private space below the default offset		  
        */
        provision_drive_nonpaged_metadata.zero_checkpoint = default_offset;
    }
    
    /* Set all background operations by default as enabled in provision drive */
    provision_drive_nonpaged_metadata.base_config_nonpaged_metadata.operation_bitmask = 0;
    
    
    /*!@todo Need to remove following code once we enable background zeroing by default
    */
    if (b_enable_background_zeroing == FBE_FALSE)
    {
        /*! @todo Disable background zeroing for all drives.
        *
        *	@note Setting the bit actually disables the background operation!!
        */
        provision_drive_nonpaged_metadata.base_config_nonpaged_metadata.operation_bitmask |= FBE_PROVISION_DRIVE_BACKGROUND_OP_DISK_ZEROING;
    }
    else if (b_enable_system_drive_background_zeroing == FBE_FALSE)
    {
        /* disable background zeroing for system drives */
        if(fbe_database_is_object_system_pvd(object_id))	
        {
            provision_drive_nonpaged_metadata.base_config_nonpaged_metadata.operation_bitmask |= FBE_PROVISION_DRIVE_BACKGROUND_OP_DISK_ZEROING;
            fbe_base_object_trace((fbe_base_object_t*)provision_drive_p,
                                    FBE_TRACE_LEVEL_INFO,
                                    FBE_TRACE_MESSAGE_ID_INFO,
                                    "pvd: background zeroing disabled for system obj: 0x%x default offset: 0x%llx\n",
                                    object_id, (unsigned long long)default_offset);
        }
    } 
    
    /* Send the nonpaged metadata write request to metadata service. */
    status = fbe_base_config_metadata_nonpaged_write_persist((fbe_base_config_t *) provision_drive_p,
                                                              packet_p,
                                                              0,
                                                              (fbe_u8_t *)&provision_drive_nonpaged_metadata, /* non paged metadata memory. */
                                                              sizeof(fbe_provision_drive_nonpaged_metadata_t)); /* size of the non paged metadata. */
    return status;
}
/******************************************************************************
* end fbe_provision_drive_metadata_write_default_nonpaged_metadata()
******************************************************************************/

/*!****************************************************************************
 * fbe_provision_drive_metadata_get_drive_fault_state()
 ******************************************************************************
 * @brief
 *  This function is used to get the drive fault state for the provision drive
 *  object.
 *
 * @param provision_drive_p         - pointer to the provision drive
 * @param drive_fault_state         - pointer to the end of life state.
 *
 * @return  fbe_status_t  
 *
 * @author
 *  07/16/2010 - Created. Lili Chen
 *
 ******************************************************************************/
fbe_status_t 
fbe_provision_drive_metadata_get_drive_fault_state(fbe_provision_drive_t * provision_drive,
                                                   fbe_bool_t * drive_fault_state)
{
    fbe_provision_drive_nonpaged_metadata_t * provision_drive_nonpaged_metadata = NULL;
    fbe_status_t status;

    /* get end of life state of the pvd object. */
    status = fbe_base_config_get_nonpaged_metadata_ptr((fbe_base_config_t *) provision_drive, (void **)&provision_drive_nonpaged_metadata);

   if((status != FBE_STATUS_OK) ||
       (provision_drive_nonpaged_metadata == NULL))
    {
        return status;
    }
    *drive_fault_state = provision_drive_nonpaged_metadata->drive_fault_state;
    return FBE_STATUS_OK;
}

/******************************************************************************
 * end fbe_provision_drive_metadata_get_drive_fault_state()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_provision_drive_metadata_set_drive_fault_state()
 ******************************************************************************
 * @brief
 *  This function is used to to set the drive fault state for the provision
 *  drive object.
 *
 * @param provision_drive_p         - pointer to the provision drive
 * @param packet_p                  - pointer to a monitor packet.
 * @param drive_fault_state         - drive fault state.
 *
 * @return  fbe_status_t  
 *
 * @author
 *  07/16/2010 - Created. Lili Chen
 *
 ******************************************************************************/
fbe_status_t 
fbe_provision_drive_metadata_set_drive_fault_state(fbe_provision_drive_t * provision_drive_p,
                                                   fbe_packet_t * packet_p,
                                                   fbe_bool_t drive_fault_state)
{
    fbe_status_t    status;

    status = fbe_base_config_metadata_nonpaged_write_persist((fbe_base_config_t *) provision_drive_p,
                                                             packet_p,
                                                             (fbe_u64_t)(&((fbe_provision_drive_nonpaged_metadata_t*)0)->drive_fault_state),
                                                             (fbe_u8_t *)&drive_fault_state,
                                                             sizeof(fbe_bool_t));
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/******************************************************************************
 * end fbe_provision_drive_metadata_set_drive_fault_state()
 ******************************************************************************/


/*!****************************************************************************
 * fbe_provision_drive_metadata_need_reinitialize()
 ******************************************************************************
 * @brief
 *  This function is used to determine whether the provision drive needs to be reinitialized. If yes,
 *  the caller should transit the provision drive to SPECIALIZE state, where it can do the reinit, including
 *  reinit metadata element, write default nonpaged and paged MD, etc.
 *
 * @param provision_drive_p         - pointer to the provision drive
 *
 * @return  fbe_bool_t  
 *
 * @author
 *  06/23/2013 - Created. Zhipeng Hu
 *
 ******************************************************************************/
fbe_bool_t 
fbe_provision_drive_metadata_need_reinitialize(fbe_provision_drive_t * provision_drive_p)
{

    return fbe_metadata_element_is_element_reinited(&((fbe_base_config_t*)provision_drive_p)->metadata_element);
}
/******************************************************************************
 * end fbe_provision_drive_metadata_set_drive_fault_state()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_provision_drive_metadata_is_np_flag_set()
 ******************************************************************************
 * @brief
 *  This function is used to fetch the flags.
 *
 * @param provision_drive_p         - pointer to the provision drive
 * @param drive_fault_state         - pointer to the end of life state.
 *
 * @return  fbe_status_t  
 *
 * @author
 *  11/7/2013 - Created. Rob Foley
 *
 ******************************************************************************/
fbe_bool_t    
fbe_provision_drive_metadata_is_np_flag_set(fbe_provision_drive_t * provision_drive_p,
                                            fbe_provision_drive_np_flags_t flag) 
{
    fbe_provision_drive_nonpaged_metadata_t * provision_drive_nonpaged_metadata = NULL;
    fbe_status_t status;

    /* get end of life state of the pvd object. */
    status = fbe_base_config_get_nonpaged_metadata_ptr((fbe_base_config_t *) provision_drive_p, (void **)&provision_drive_nonpaged_metadata);

   if((status != FBE_STATUS_OK) ||
       (provision_drive_nonpaged_metadata == NULL)){
        return FBE_FALSE;
    }
   if ((provision_drive_nonpaged_metadata->flags & flag) == flag) {
       return FBE_TRUE;
   }
   return FBE_FALSE;
}

/******************************************************************************
 * end fbe_provision_drive_metadata_is_np_flag_set()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_provision_drive_metadata_is_any_np_flag_set()
 ******************************************************************************
 * @brief
 *  This function is used to to check if one or more NP flag(s) are set
 *
 * @param provision_drive_p         - pointer to the provision drive
 * @param drive_fault_state         - pointer to the end of life state.
 *
 * @return  fbe_status_t  
 *
 ******************************************************************************/
fbe_bool_t    
fbe_provision_drive_metadata_is_any_np_flag_set(fbe_provision_drive_t * provision_drive_p,
                                                fbe_provision_drive_np_flags_t flags) 
{
    fbe_provision_drive_nonpaged_metadata_t * provision_drive_nonpaged_metadata = NULL;
    fbe_status_t status;

    /* get end of life state of the pvd object. */
    status = fbe_base_config_get_nonpaged_metadata_ptr((fbe_base_config_t *) provision_drive_p, (void **)&provision_drive_nonpaged_metadata);

   if((status != FBE_STATUS_OK) ||
       (provision_drive_nonpaged_metadata == NULL)){
        return FBE_FALSE;
    }
   if ((provision_drive_nonpaged_metadata->flags & flags) != 0) {
       return FBE_TRUE;
   }
   return FBE_FALSE;
}

/******************************************************************************
 * end fbe_provision_drive_metadata_is_any_np_flag_set()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_provision_drive_metadata_np_flag_set()
 ******************************************************************************
 * @brief
 *  This function is used to to set the drive fault state for the provision
 *  drive object.
 *
 * @param provision_drive_p         - pointer to the provision drive
 * @param packet_p                  - pointer to a monitor packet.
 * @param drive_fault_state         - drive fault state.
 *
 * @return  fbe_status_t  
 *
 * @author
 *  11/7/2013 - Created. Rob Foley
 *
 ******************************************************************************/
fbe_status_t 
fbe_provision_drive_metadata_np_flag_set(fbe_provision_drive_t * provision_drive_p,
                                         fbe_packet_t * packet_p,
                                         fbe_provision_drive_np_flags_t flags)
{
    fbe_status_t    status;

    status = fbe_base_config_metadata_nonpaged_write_persist((fbe_base_config_t *) provision_drive_p,
                                                             packet_p,
                                                             (fbe_u64_t)(&((fbe_provision_drive_nonpaged_metadata_t*)0)->flags),
                                                             (fbe_u8_t *)&flags,
                                                             sizeof(fbe_provision_drive_np_flags_t));
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/******************************************************************************
 * end fbe_provision_drive_metadata_np_flag_set()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_provision_drive_reinit_paged_metadata_completion()
 ******************************************************************************
 * @brief
 *  This function is used to handle the completion after re-initializing the
 *  paged metadata (BOTH copies).
 *
 * @param provision_drive_p         - pointer to the provision drive
 * @param packet_p                  - pointer to a monitor packet.
 *
 * @return  fbe_status_t  
 *
 * @author
 *  01/09/2014  Ron Proulx  - Created.
 *
 ******************************************************************************/
static fbe_status_t fbe_provision_drive_reinit_paged_metadata_completion(fbe_packet_t * packet_p,
                                                                     fbe_packet_completion_context_t context)
{
    fbe_status_t                        status;
    fbe_provision_drive_t              *provision_drive_p = NULL;
    fbe_payload_metadata_operation_t   *metadata_operation_p = NULL;
    fbe_payload_ex_t                   *sep_payload_p = NULL;
    fbe_payload_metadata_status_t       metadata_status;
    fbe_payload_metadata_operation_opcode_t metadata_opcode;
    fbe_memory_request_t               *memory_request = NULL;
    
    provision_drive_p = (fbe_provision_drive_t *) context;

    /* get the payload and metadata operation. */
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    metadata_operation_p =  fbe_payload_ex_get_metadata_operation(sep_payload_p);
    
    /* get the status of the paged metadata operation. */
    fbe_payload_metadata_get_status(metadata_operation_p, &metadata_status);
    status = fbe_transport_get_status_code(packet_p);

    fbe_payload_metadata_get_opcode(metadata_operation_p, &metadata_opcode);

    /* release the metadata operation. */
    fbe_payload_ex_release_metadata_operation(sep_payload_p, metadata_operation_p);

    /* release client_blob */
    memory_request = fbe_transport_get_memory_request(packet_p);
    fbe_memory_free_request_entry(memory_request);

    if((status != FBE_STATUS_OK) ||
       (metadata_status != FBE_PAYLOAD_METADATA_STATUS_OK))
    {
        fbe_base_object_trace((fbe_base_object_t*)provision_drive_p,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s reinit of paged metadata failed, status:0x%x, metadata_status:0x%x.\n",
                              __FUNCTION__, status, metadata_status);
        status = (status != FBE_STATUS_OK) ? status : FBE_STATUS_GENERIC_FAILURE;
        fbe_transport_set_status(packet_p, status, 0);
    }

    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_provision_drive_reinit_paged_metadata_completion()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_provision_drive_reinit_paged_metadata_allocate_completion()
 ******************************************************************************
 * @brief
 *  This is the completion for memory allocation for paged blob.
 *
 * @param memory_request_p         - pointer to the memory request
 * @param context                  - context.
 *
 * @return  fbe_status_t  
 *
 * @author
 *  01/09/2014  Ron Proulx  - Created.
 *
 ******************************************************************************/
static fbe_status_t fbe_provision_drive_reinit_paged_metadata_allocate_completion(fbe_memory_request_t * memory_request_p, 
                                                                     fbe_memory_completion_context_t context)
{
    fbe_status_t                            status = FBE_STATUS_GENERIC_FAILURE; 
    fbe_u64_t                               repeat_count = 0;
    fbe_provision_drive_paged_metadata_t    paged_set_bits;
    fbe_payload_ex_t                       *sep_payload_p = NULL;
    fbe_payload_metadata_operation_t       *metadata_operation_p = NULL;
    fbe_lba_t                               paged_metadata_capacity = 0;
    fbe_packet_t                           *packet_p = NULL;
    fbe_provision_drive_t                  *provision_drive_p = (fbe_provision_drive_t *)context;

    /* get the payload and control operation. */
    packet_p = fbe_transport_memory_request_to_packet(memory_request_p);
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);

    /* Check allocation status */
    if (fbe_memory_request_is_allocation_complete(memory_request_p) == FBE_FALSE)
    {
        /* Currently this callback doesn't expect any aborted requests */
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s memory allocation failed, state:%d\n",
                              __FUNCTION__, memory_request_p->request_state);
        fbe_transport_set_status(packet_p, FBE_STATUS_INSUFFICIENT_RESOURCES, 0);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Re-initialize the paged metadata with sane values.
     */
    fbe_zero_memory(&paged_set_bits, sizeof(fbe_provision_drive_paged_metadata_t));
    
    paged_set_bits.need_zero_bit = 0; 
    paged_set_bits.valid_bit = 1; 
    paged_set_bits.consumed_user_data_bit = 1;
    paged_set_bits.user_zero_bit = 0;
    paged_set_bits.unused_bit = 0;

    /* Set the repeat count as number of records in paged metadata capacity.
     * This capacity not only includes the paged metadata but also the second
     * copy.
     */
    fbe_base_config_metadata_get_paged_metadata_capacity((fbe_base_config_t *)provision_drive_p,
                                                       &paged_metadata_capacity);
    repeat_count = (paged_metadata_capacity*FBE_METADATA_BLOCK_DATA_SIZE)/sizeof(fbe_provision_drive_paged_metadata_t);

    /* Allocate the metadata operation. */
    metadata_operation_p =  fbe_payload_ex_allocate_metadata_operation(sep_payload_p);

    /* Build the paged metadata write request. */
    status = fbe_payload_metadata_build_paged_write(metadata_operation_p, 
                                                    &(((fbe_base_config_t *) provision_drive_p)->metadata_element),
                                                    0, /* Paged metadata offset is zero for the default paged metadata write. 
                                                          paint the whole paged metadata with the reinit value */
                                                    (fbe_u8_t *) &paged_set_bits,
                                                    sizeof(fbe_provision_drive_paged_metadata_t),
                                                    repeat_count);
    if(status != FBE_STATUS_OK)
    {
        fbe_memory_free_request_entry(memory_request_p);
        fbe_payload_ex_release_metadata_operation(sep_payload_p, metadata_operation_p);
        fbe_transport_set_status(packet_p, status, 0);
        return status;
    }

    /*! @note We currently don't take ANY paged lock.  The NP lock should
     *        protect us.
     */
    status = fbe_metadata_paged_init_client_blob(memory_request_p, metadata_operation_p, 0, FBE_FALSE);

    /* set the completion function for the default paged metadata write. */
    fbe_transport_set_completion_function(packet_p, fbe_provision_drive_reinit_paged_metadata_completion, provision_drive_p);

    fbe_payload_ex_increment_metadata_operation_level(sep_payload_p);

    /* Issue the metadata operation. */
    fbe_metadata_operation_entry(packet_p);
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/******************************************************************************
 * end fbe_provision_drive_reinit_paged_metadata_allocate_completion()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_provision_drive_reinit_paged_metadata()
 ******************************************************************************
 * @brief
 *   This function is used to re-initialize the metadata with default values for 
 *   the paged metadata (BOTH copies of the paged metadata).
 *
 * @param provision_drive_p         - pointer to the provision drive
 * @param packet_p                  - pointer to a monitor packet.
 *
 * @return  fbe_status_t  
 *
 * @author
 *  01/09/2014  Ron Proulx  - Created.
 *
 ******************************************************************************/
fbe_status_t fbe_provision_drive_reinit_paged_metadata(fbe_provision_drive_t * provision_drive_p,
                                                          fbe_packet_t *packet_p)
{
    fbe_status_t            status = FBE_STATUS_GENERIC_FAILURE; 
    fbe_payload_ex_t       *sep_payload_p = NULL;
    fbe_memory_request_t   *memory_request_p = NULL;

    sep_payload_p = fbe_transport_get_payload_ex(packet_p);

    /* allocate memory for client_blob */
    memory_request_p = fbe_transport_get_memory_request(packet_p);
    fbe_memory_build_chunk_request(memory_request_p,
                                   FBE_MEMORY_CHUNK_SIZE_FOR_64_BLOCKS_IO,
                                   2, /* number of chunks */
                                   fbe_transport_get_resource_priority(packet_p),
                                   fbe_transport_get_io_stamp(packet_p),
                                   (fbe_memory_completion_function_t)fbe_provision_drive_reinit_paged_metadata_allocate_completion, /* SAFEBUG - cast to supress warning but must fix because memory completion function shouldn't return status */
                                   provision_drive_p);
    fbe_transport_memory_request_set_io_master(memory_request_p, packet_p);

    /* Issue the memory request to memory service. */
    status = fbe_memory_request_entry(memory_request_p);
    if((status != FBE_STATUS_OK) && (status != FBE_STATUS_PENDING))
    {
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s memory allocation failed",  __FUNCTION__);
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return status;
    }

    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/******************************************************************************
 * end fbe_provision_drive_reinit_paged_metadata()
 ******************************************************************************/


/*!****************************************************************************
 * fbe_provision_drive_mark_paged_encrypted()
 ******************************************************************************
 * @brief
 *  Completion function for writing the paged metadata.
 *  Set the bit indicating paged is encrypted.
 *
 * @param packet_p     - Packet requesting operation.
 * @param context      - Packet completion context (provision drive object).
 *
 * @return status - The status of the operation.
 *
 * @author
 *  10/30/2013 - Created. Rob Foley
 *
 ******************************************************************************/
static fbe_status_t 
fbe_provision_drive_mark_paged_encrypted(fbe_packet_t * packet_p,
                                         fbe_packet_completion_context_t context)
{
    fbe_provision_drive_t *                                 provision_drive_p = NULL;
    fbe_status_t                                            status;
    fbe_provision_drive_np_flags_t np_flags = FBE_PROVISION_DRIVE_NP_FLAG_PAGED_VALID;
    fbe_provision_drive_np_flags_t existing_np_flags = FBE_PROVISION_DRIVE_NP_FLAG_PAGED_VALID;

    provision_drive_p = (fbe_provision_drive_t *)context;
    status = fbe_transport_get_status_code(packet_p);

    /* If status is not good then complete the packet with error. */
    if (status != FBE_STATUS_OK){
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,    
                               "%s: reinit paged failed stat:0x%x, \n",
                               __FUNCTION__, status);
        fbe_transport_set_status(packet_p, status, 0);
        return status;
    }

    status = fbe_provision_drive_metadata_get_np_flag(provision_drive_p, &existing_np_flags);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,    
                               "%s: could not get np lock stat:0x%x, \n",
                               __FUNCTION__, status);
        fbe_transport_set_status(packet_p, status, 0);
        return status;
    }
    np_flags |= (existing_np_flags & FBE_PROVISION_DRIVE_NP_FLAG_SCRUB_BITS);

    /* set the zero on demand flag using base config nonpaged write. */
    status = fbe_base_config_metadata_nonpaged_write_persist((fbe_base_config_t *) provision_drive_p,
                                                             packet_p,
                                                             (fbe_u64_t)(&((fbe_provision_drive_nonpaged_metadata_t*)0)->flags),
                                                             (fbe_u8_t *)&np_flags,
                                                             sizeof(np_flags));
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/********************************************************* 
* end fbe_provision_drive_mark_paged_encrypted()
**********************************************************/
/*!****************************************************************************
 * fbe_provision_drive_start_paged_reinit()
 ******************************************************************************
 * @brief
 *  We just updated the NP to indicate paged needs reconstruct.
 *  Start writing out the new paged.
 *
 * @param packet_p     - Packet requesting operation.
 * @param context      - Packet completion context (provision drive object).
 *
 * @return status - The status of the operation.
 *
 * @author
 *  10/30/2013 - Created. Rob Foley
 *
 ******************************************************************************/
static fbe_status_t 
fbe_provision_drive_start_paged_reinit(fbe_packet_t * packet_p,
                                       fbe_packet_completion_context_t context)
{
    fbe_status_t                            status;
    fbe_provision_drive_t                  *provision_drive_p = NULL;

    provision_drive_p = (fbe_provision_drive_t *)context;
    status = fbe_transport_get_status_code(packet_p);

    /* If status is not good then complete the packet with error. */
    if (status != FBE_STATUS_OK){
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,    
                               "%s: mark of NP encryption start failed stat:0x%x, \n",
                               __FUNCTION__, status);
        fbe_transport_set_status(packet_p, status, 0);
        return status;
    }
    /* Issue the re-initialize paged.
     */
    status = fbe_provision_drive_reinit_paged_metadata(provision_drive_p, packet_p);
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/********************************************************* 
* end fbe_provision_drive_start_paged_reinit()
**********************************************************/

/*!****************************************************************************
 * fbe_provision_drive_mark_paged_write_start()
 ******************************************************************************
 * @brief
 *  We have the NP lock.  Mark paged as invalid.  If we go down when writing it
 *  with the new key we will have to reconstruct it.
 *
 * @param packet_p     - Packet requesting operation.
 * @param context      - Packet completion context (provision drive object).
 *
 * @return status - The status of the operation.
 *
 * @author
 *  10/30/2013 - Created. Rob Foley
 *
 ******************************************************************************/
fbe_status_t fbe_provision_drive_mark_paged_write_start(fbe_packet_t * packet_p,
                                           fbe_packet_completion_context_t context)
{
    fbe_status_t                            status;
    fbe_provision_drive_t                  *provision_drive_p = NULL;
    fbe_provision_drive_np_flags_t          np_flags = FBE_PROVISION_DRIVE_NP_FLAG_PAGED_NEEDS_CONSUMED;
    fbe_provision_drive_nonpaged_metadata_t *nonpaged_metadata_p = NULL;
    fbe_base_config_encryption_mode_t	    encryption_mode;
    fbe_provision_drive_np_flags_t existing_np_flags;

    provision_drive_p = (fbe_provision_drive_t *)context;
    status = fbe_transport_get_status_code(packet_p);


    /* If status is not good then we failed to get the NP lock.
     * Return the failure to call completion.
     */
    if (status != FBE_STATUS_OK){
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,    
                               "%s: could not get np lock stat:0x%x, \n",
                               __FUNCTION__, status);
        fbe_transport_set_status(packet_p, status, 0);
        return status;
    }

    status = fbe_provision_drive_metadata_get_np_flag(provision_drive_p, &existing_np_flags);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,    
                               "%s: could not get np lock stat:0x%x, \n",
                               __FUNCTION__, status);
        fbe_transport_set_status(packet_p, status, 0);
        return status;
    }
    np_flags |= (existing_np_flags & FBE_PROVISION_DRIVE_NP_FLAG_SCRUB_BITS);

    /* We own the NP. We must always release it.
     */
    fbe_transport_set_completion_function(packet_p, fbe_provision_drive_release_NP_lock, provision_drive_p);

    /*! @note Validate that there are no background operations that are required. 
     * If background operations are in progress we cannot initialize the paged. 
     */
    fbe_base_config_get_nonpaged_metadata_ptr((fbe_base_config_t *) provision_drive_p, (void **)&nonpaged_metadata_p);
    if ((nonpaged_metadata_p->zero_checkpoint != FBE_LBA_INVALID)              ||
        (nonpaged_metadata_p->verify_invalidate_checkpoint != FBE_LBA_INVALID)    ) {
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p,
                              FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,    
                              "PVD zero chkpt: %llx or verifyinv: %llx not at end do not init paged\n",
                              nonpaged_metadata_p->zero_checkpoint,nonpaged_metadata_p->verify_invalidate_checkpoint);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        return FBE_STATUS_CONTINUE;
    }

    /* Get nonpaged metadata pointer, if it is null or status is not good then
     * return error.
     */
    fbe_base_config_get_encryption_mode((fbe_base_config_t *)provision_drive_p, &encryption_mode);
  
    /* Trace some information.
     */
    fbe_base_object_trace((fbe_base_object_t *)provision_drive_p,
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,    
                          "%s: NP flags: 0x%x encryption mode: 0x%x\n",
                          __FUNCTION__, existing_np_flags,
                          encryption_mode);

    /* We own the NP.  Start re-writting the paged to encrypt it.
     */
    fbe_transport_set_completion_function(packet_p, fbe_provision_drive_mark_paged_encrypted, provision_drive_p);
    fbe_transport_set_completion_function(packet_p, fbe_provision_drive_start_paged_reinit, provision_drive_p);

    /* set the zero on demand flag using base config nonpaged write. */
    status = fbe_base_config_metadata_nonpaged_write_persist((fbe_base_config_t *) provision_drive_p,
                                                             packet_p,
                                                             (fbe_u64_t)(&((fbe_provision_drive_nonpaged_metadata_t*)0)->flags),
                                                             (fbe_u8_t *)&np_flags,
                                                             sizeof(np_flags));
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/********************************************************* 
* end fbe_provision_drive_mark_paged_write_start()
**********************************************************/

/*!****************************************************************************
 * fbe_provision_drive_metadata_get_np_flag()
 ******************************************************************************
 * @brief
 *  This function gets the NP flags.
 *
 * @param provision_drive_p     - Packet requesting operation.
 * @param flag                  - Point to the flag.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  01/17/2014 - Created. Lili Chen
 *
 ******************************************************************************/
fbe_status_t    
fbe_provision_drive_metadata_get_np_flag(fbe_provision_drive_t * provision_drive_p,
                                         fbe_provision_drive_np_flags_t * flag) 
{
    fbe_provision_drive_nonpaged_metadata_t * provision_drive_nonpaged_metadata = NULL;
    fbe_status_t status;

    /* Get NP metadata of the pvd object. */
    status = fbe_base_config_get_nonpaged_metadata_ptr((fbe_base_config_t *) provision_drive_p, (void **)&provision_drive_nonpaged_metadata);

    if((status != FBE_STATUS_OK) ||
       (provision_drive_nonpaged_metadata == NULL)){
        *flag = 0;
        return FBE_STATUS_GENERIC_FAILURE;
    }

    *flag = provision_drive_nonpaged_metadata->flags;
   return FBE_STATUS_OK;
}
/********************************************************* 
* end fbe_provision_drive_metadata_get_np_flag()
**********************************************************/

/*!****************************************************************************
 * fbe_provision_drive_metadata_set_np_flag_need_zero_paged()
 ******************************************************************************
 * @brief
 *  This function sets NP NEEDS_ZERO flag with NP lock.
 *
 * @param packet_p     - Packet requesting operation.
 * @param context      - Packet completion context (provision drive object).
 *
 * @return status - The status of the operation.
 *
 * @author
 *  01/17/2014 - Created. Lili Chen
 *
 ******************************************************************************/
fbe_status_t 
fbe_provision_drive_metadata_set_np_flag_need_zero_paged(fbe_packet_t * packet_p,
                                                         fbe_packet_completion_context_t context)
{
    fbe_status_t                            status;
    fbe_provision_drive_t                  *provision_drive_p = NULL;
    fbe_provision_drive_np_flags_t          np_flags;

    provision_drive_p = (fbe_provision_drive_t *)context;
    status = fbe_transport_get_status_code(packet_p);


    /* If status is not good then we failed to get the NP lock.
     * Return the failure to call completion.
     */
    if (status != FBE_STATUS_OK){
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,    
                               "%s: could not get np lock stat:0x%x, \n",
                               __FUNCTION__, status);
        fbe_transport_set_status(packet_p, status, 0);
        return status;
    }

    /* We own the NP. We must always release it.
     */
    fbe_transport_set_completion_function(packet_p, fbe_provision_drive_release_NP_lock, provision_drive_p);

    status = fbe_provision_drive_metadata_get_np_flag(provision_drive_p, &np_flags);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,    
                               "%s: could not get np lock stat:0x%x, \n",
                               __FUNCTION__, status);
        fbe_transport_set_status(packet_p, status, 0);
        return status;
    }

    /* Trace some information.
     */
    fbe_base_object_trace((fbe_base_object_t *)provision_drive_p,
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,    
                          "%s: NP flags: 0x%x \n",
                          __FUNCTION__, np_flags);

    /* Change the bits.
     */
    np_flags |= FBE_PROVISION_DRIVE_NP_FLAG_PAGED_NEEDS_ZERO;
    np_flags &= ~FBE_PROVISION_DRIVE_NP_FLAG_PAGED_VALID;

    /* We own the NP.  Start re-writting the paged to encrypt it.
     */
    status = fbe_base_config_metadata_nonpaged_write_persist((fbe_base_config_t *) provision_drive_p,
                                                             packet_p,
                                                             (fbe_u64_t)(&((fbe_provision_drive_nonpaged_metadata_t*)0)->flags),
                                                             (fbe_u8_t *)&np_flags,
                                                             sizeof(np_flags));
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/********************************************************* 
* end fbe_provision_drive_metadata_set_np_flag_need_zero_paged()
**********************************************************/

/*!****************************************************************************
 * fbe_provision_drive_metadata_clear_np_flag_set_zero_checkpoint()
 ******************************************************************************
 * @brief
 *  This function sets zero checkpoint after clearing NEEDS_ZERO.
 *
 * @param packet_p     - Packet requesting operation.
 * @param context      - Packet completion context (provision drive object).
 *
 * @return status - The status of the operation.
 *
 * @author
 *  01/17/2014 - Created. Lili Chen
 *
 ******************************************************************************/
fbe_status_t 
fbe_provision_drive_metadata_clear_np_flag_set_zero_checkpoint(fbe_packet_t * packet_p,
                                                               fbe_packet_completion_context_t context)
{
    fbe_status_t                            status;
    fbe_provision_drive_t                  *provision_drive_p = NULL;
    fbe_lba_t                               default_offset;
    fbe_object_id_t							object_id;
    fbe_bool_t								is_system_drive;

    provision_drive_p = (fbe_provision_drive_t *)context;
    status = fbe_transport_get_status_code(packet_p);

    /* If status is not good then we failed to clear np need zero.
     * Return the failure to call completion.
     */
    if (status != FBE_STATUS_OK){
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,    
                               "%s: could not get np lock stat:0x%x, \n",
                               __FUNCTION__, status);
        fbe_transport_set_status(packet_p, status, 0);
        return status;
    }

    fbe_base_config_get_default_offset((fbe_base_config_t *)provision_drive_p, &default_offset);
    fbe_base_object_trace((fbe_base_object_t *)provision_drive_p,
                           FBE_TRACE_LEVEL_WARNING,
                           FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,    
                           "%s: moving checkpoint to: 0x%llx, \n",
                           __FUNCTION__, (unsigned long long)default_offset);

    fbe_base_object_get_object_id((fbe_base_object_t *)provision_drive_p, &object_id);
    is_system_drive = fbe_database_is_object_system_pvd(object_id);
    if (is_system_drive == FBE_TRUE)
    {
        fbe_provision_drive_metadata_set_background_zero_checkpoint(provision_drive_p, packet_p, 0);
    } else {
        fbe_provision_drive_metadata_set_background_zero_checkpoint(provision_drive_p, packet_p, default_offset);
    }

    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/********************************************************* 
* end fbe_provision_drive_metadata_clear_np_flag_set_zero_checkpoint()
**********************************************************/

/*!****************************************************************************
 * fbe_provision_drive_metadata_clear_np_flag_need_zero_paged()
 ******************************************************************************
 * @brief
 *  This function clears NP NEEDS_ZERO flag with NP lock.
 *
 * @param packet_p     - Packet requesting operation.
 * @param context      - Packet completion context (provision drive object).
 *
 * @return status - The status of the operation.
 *
 * @author
 *  01/17/2014 - Created. Lili Chen
 *
 ******************************************************************************/
fbe_status_t 
fbe_provision_drive_metadata_clear_np_flag_need_zero_paged(fbe_packet_t * packet_p,
                                                           fbe_packet_completion_context_t context)
{
    fbe_status_t                            status;
    fbe_provision_drive_t                  *provision_drive_p = NULL;
    fbe_provision_drive_np_flags_t          np_flags;

    provision_drive_p = (fbe_provision_drive_t *)context;
    status = fbe_transport_get_status_code(packet_p);


    /* If status is not good then we failed to get the NP lock.
     * Return the failure to call completion.
     */
    if (status != FBE_STATUS_OK){
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,    
                               "%s: could not get np lock stat:0x%x, \n",
                               __FUNCTION__, status);
        fbe_transport_set_status(packet_p, status, 0);
        return status;
    }

    /* We own the NP. We must always release it.
     */
    fbe_transport_set_completion_function(packet_p, fbe_provision_drive_release_NP_lock, provision_drive_p);

    status = fbe_provision_drive_metadata_get_np_flag(provision_drive_p, &np_flags);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,    
                               "%s: could not get np lock stat:0x%x, \n",
                               __FUNCTION__, status);
        fbe_transport_set_status(packet_p, status, 0);
        return status;
    }

    /* Trace some information.
     */
    fbe_base_object_trace((fbe_base_object_t *)provision_drive_p,
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,    
                          "%s: NP flags: 0x%x \n",
                          __FUNCTION__, np_flags);

    /* Change the bits.
     */
    np_flags &= ~FBE_PROVISION_DRIVE_NP_FLAG_PAGED_NEEDS_ZERO;
    np_flags |= FBE_PROVISION_DRIVE_NP_FLAG_PAGED_VALID;

    /* We own the NP.  Start re-writting the paged to encrypt it.
     */
    fbe_transport_set_completion_function(packet_p, fbe_provision_drive_metadata_clear_np_flag_set_zero_checkpoint, provision_drive_p);
    status = fbe_base_config_metadata_nonpaged_write_persist((fbe_base_config_t *) provision_drive_p,
                                                             packet_p,
                                                             (fbe_u64_t)(&((fbe_provision_drive_nonpaged_metadata_t*)0)->flags),
                                                             (fbe_u8_t *)&np_flags,
                                                             sizeof(fbe_provision_drive_np_flags_t));
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/********************************************************* 
* end fbe_provision_drive_metadata_clear_np_flag_need_zero_paged()
**********************************************************/
/*!****************************************************************************
 * fbe_provision_drive_verify_paged()
 ******************************************************************************
 * @brief
 *   Start the allocate of memory to perform a verify of either paged
 *  (for user PVDs) or validation area for system PVDs.
 *  This verify is needed to validate that the keys are correct.
 *
 * @param provision_drive_p         - pointer to the provision drive
 * @param packet_p                  - pointer to a monitor packet.
 *
 * @return  fbe_status_t  
 *
 * @author
 *  2/4/2014 - Created. Rob Foley
 *
 ******************************************************************************/
fbe_status_t fbe_provision_drive_verify_paged(fbe_provision_drive_t * provision_drive_p,
                                              fbe_packet_t *packet_p)
{
    fbe_status_t            status = FBE_STATUS_GENERIC_FAILURE; 
    fbe_payload_ex_t       *sep_payload_p = NULL;
    fbe_memory_request_t   *memory_request_p = NULL;

    sep_payload_p = fbe_transport_get_payload_ex(packet_p);

    /* allocate memory for sg list and buffer. */
    memory_request_p = fbe_transport_get_memory_request(packet_p);
    fbe_memory_build_chunk_request(memory_request_p,
                                   FBE_MEMORY_CHUNK_SIZE_FOR_64_BLOCKS_IO,
                                   2, /* number of chunks */
                                   fbe_transport_get_resource_priority(packet_p),
                                   fbe_transport_get_io_stamp(packet_p),
                                   (fbe_memory_completion_function_t)fbe_provision_drive_verify_paged_allocate_completion,
                                   provision_drive_p);
    fbe_transport_memory_request_set_io_master(memory_request_p, packet_p);

    /* Issue the memory request to memory service. */
    status = fbe_memory_request_entry(memory_request_p);
    if((status != FBE_STATUS_OK) && (status != FBE_STATUS_PENDING))
    {
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s memory allocation failed",  __FUNCTION__);
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return status;
    }

    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/******************************************************************************
 * end fbe_provision_drive_verify_paged()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_provision_drive_verify_paged_allocate_completion()
 ******************************************************************************
 * @brief
 *  This is the completion for memory allocation for verify of either paged
 *  (for user PVDs) or validation area for system PVDs.
 *
 * @param memory_request_p         - pointer to the memory request
 * @param context                  - context.
 *
 * @return  fbe_status_t  
 *
 * @author
 *  2/4/2014 - Created. Rob Foley
 *
 ******************************************************************************/
static fbe_status_t fbe_provision_drive_verify_paged_allocate_completion(fbe_memory_request_t * memory_request_p, 
                                                                         fbe_memory_completion_context_t context)
{
    fbe_payload_ex_t                       *payload_p = NULL;
    fbe_lba_t                               paged_metadata_capacity = 0;
    fbe_lba_t                               paged_metadata_lba = 0;
    fbe_block_count_t                       block_count;
    fbe_packet_t                           *packet_p = NULL;
    fbe_provision_drive_t                  *provision_drive_p = (fbe_provision_drive_t *)context;
    fbe_memory_header_t                    *memory_header_p = NULL;
    fbe_memory_header_t                    *next_memory_header_p = NULL;
    fbe_u8_t                               *buffer_p = NULL;
    fbe_sg_element_t                       *sg_p = NULL;
    fbe_payload_block_operation_t *block_operation_p = NULL;
    fbe_object_id_t pvd_object_id;
    fbe_cpu_id_t cpu_id;
    fbe_packet_t *new_packet_p = NULL;
    fbe_payload_ex_t                       *new_payload_p = NULL;
    fbe_payload_control_operation_t *control_operation_p = NULL;
    fbe_base_port_mgmt_register_keys_t *                    port_register_keys = NULL;
    fbe_provision_drive_register_keys_context_t * context_p = NULL;
    fbe_object_id_t object_id;
    fbe_bool_t is_system_drive;
    fbe_lba_t paged_metadata_offset;

    fbe_base_object_get_object_id((fbe_base_object_t *)provision_drive_p, &object_id);
    is_system_drive = fbe_database_is_object_system_pvd(object_id);

    /* get the payload and control operation. */
    packet_p = fbe_transport_memory_request_to_packet(memory_request_p);
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);

    fbe_payload_control_get_buffer(control_operation_p, &buffer_p);
    port_register_keys = (fbe_base_port_mgmt_register_keys_t *) buffer_p;
    context_p = (fbe_provision_drive_register_keys_context_t*) (port_register_keys + 1);

    /* Check allocation status */
    if (fbe_memory_request_is_allocation_complete(memory_request_p) == FBE_FALSE) {
        /* Currently this callback doesn't expect any aborted requests */
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s memory allocation failed, state:%d\n",
                              __FUNCTION__, memory_request_p->request_state);
        fbe_transport_set_status(packet_p, FBE_STATUS_INSUFFICIENT_RESOURCES, 0);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    memory_header_p = fbe_memory_get_first_memory_header(memory_request_p);
    buffer_p = (fbe_u8_t *)fbe_memory_header_to_data(memory_header_p);

    fbe_memory_get_next_memory_header(memory_header_p, &next_memory_header_p);
    new_packet_p = (fbe_packet_t*)fbe_memory_header_to_data(next_memory_header_p);
    sg_p = (fbe_sg_element_t*)(new_packet_p + 1);
    sg_p->address = buffer_p;
    sg_p->count = FBE_MEMORY_CHUNK_SIZE_BLOCK_COUNT_64 * FBE_BE_BYTES_PER_BLOCK;
    fbe_sg_element_terminate(&sg_p[1]);

    fbe_transport_initialize_packet(new_packet_p);
    fbe_base_object_get_object_id((fbe_base_object_t *) provision_drive_p, &pvd_object_id);
    fbe_transport_set_address(new_packet_p,
                              FBE_PACKAGE_ID_SEP_0,
                              FBE_SERVICE_ID_TOPOLOGY,
                              FBE_CLASS_ID_INVALID,
                              pvd_object_id);
    new_payload_p = fbe_transport_get_payload_ex(new_packet_p);

    cpu_id = (memory_request_p->io_stamp & FBE_PACKET_IO_STAMP_MASK) >> FBE_PACKET_IO_STAMP_SHIFT;
    fbe_transport_set_cpu_id(new_packet_p, cpu_id);

    /* Set the repeat count as number of records in paged metadata capacity.
     * This capacity not only includes the paged metadata but also the second
     * copy.
     */
    fbe_base_config_metadata_get_paged_metadata_capacity((fbe_base_config_t *)provision_drive_p,
                                                       &paged_metadata_capacity);
    fbe_base_config_metadata_get_paged_record_start_lba((fbe_base_config_t *)provision_drive_p,
                                                        &paged_metadata_lba);
    fbe_base_config_metadata_get_paged_mirror_metadata_offset((fbe_base_config_t *)provision_drive_p,
                                                              &paged_metadata_offset);
    block_count = FBE_MIN(paged_metadata_capacity, FBE_MEMORY_CHUNK_SIZE_BLOCK_COUNT_64);

    block_operation_p = fbe_payload_ex_allocate_block_operation(new_payload_p);

    /* System drives use a special validation area per client.
     */
    if (is_system_drive) {
        paged_metadata_lba += paged_metadata_offset; 
        paged_metadata_lba += context_p->client_index * FBE_MEMORY_CHUNK_SIZE_BLOCK_COUNT_64;
    }
    /* We will either use 64 blocks or the capacity of the paged metadata, whichever is smaller.
     */
    fbe_payload_block_build_operation(block_operation_p, FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,
                                      paged_metadata_lba, block_count, FBE_BE_BYTES_PER_BLOCK, 1,
                                      NULL);
    fbe_payload_ex_set_sg_list(new_payload_p, sg_p, 0);

    fbe_transport_add_subpacket(packet_p, new_packet_p);
    /* set the completion function for the default paged metadata write. */
    fbe_transport_set_completion_function(new_packet_p, fbe_provision_drive_verify_paged_completion, provision_drive_p);

    /* Need to do this before set key handle so it sees our block operation. */
    fbe_payload_ex_increment_block_operation_level(new_payload_p);
    if (is_system_drive) {
        fbe_provision_drive_set_read_validate_key(provision_drive_p, new_packet_p, context_p->client_index);
    }
    else {
        fbe_provision_drive_set_key_handle(provision_drive_p, new_packet_p);
    }

    fbe_base_config_send_functional_packet((fbe_base_config_t *)provision_drive_p, new_packet_p, 0);
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/******************************************************************************
 * end fbe_provision_drive_verify_paged_allocate_completion()
 ******************************************************************************/
/*!**************************************************************
 * fbe_provision_drive_set_encryption_state()
 ****************************************************************
 * @brief
 *  Set the current encryption state of a given client.

 * @param provision_drive_p
 * @param encryption_state
 * @param client_index
 * 
 * @return fbe_status_t
 *
 * @author
 *  3/10/2014 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t fbe_provision_drive_set_encryption_state(fbe_provision_drive_t *provision_drive_p,
                                                      fbe_edge_index_t client_index,
                                                      fbe_base_config_encryption_state_t encryption_state)
{
    fbe_object_id_t object_id;
    fbe_bool_t is_system_drive;

    fbe_base_object_get_object_id((fbe_base_object_t *)provision_drive_p, &object_id);
    is_system_drive = fbe_database_is_object_system_pvd(object_id);

    if (is_system_drive) {
        provision_drive_p->key_info_table[client_index].encryption_state = encryption_state;
    } else {
        fbe_base_config_set_encryption_state((fbe_base_config_t*)provision_drive_p,
                                             encryption_state);
    }
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_provision_drive_set_encryption_state()
 ******************************************/

/*!**************************************************************
 * fbe_provision_drive_get_encryption_state()
 ****************************************************************
 * @brief
 *  get the current encryption state of a given client.
 *
 * @param provision_drive_p
 * @param encryption_state
 * @param client_index
 * 
 * @return fbe_status_t
 *
 * @author
 *  3/10/2014 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t fbe_provision_drive_get_encryption_state(fbe_provision_drive_t *provision_drive_p,
                                                      fbe_edge_index_t client_index,
                                                      fbe_base_config_encryption_state_t *encryption_state_p)
{
    fbe_object_id_t object_id;
    fbe_bool_t is_system_drive;

    fbe_base_object_get_object_id((fbe_base_object_t *)provision_drive_p, &object_id);
    is_system_drive = fbe_database_is_object_system_pvd(object_id);

    if (is_system_drive) {
        *encryption_state_p = provision_drive_p->key_info_table[client_index].encryption_state;
    } else {
        fbe_base_config_get_encryption_state((fbe_base_config_t*)provision_drive_p,
                                             encryption_state_p);
    }
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_provision_drive_get_encryption_state()
 ******************************************/

/*!****************************************************************************
 * fbe_provision_drive_verify_paged_completion()
 ******************************************************************************
 * @brief
 *  This is the completion for verify of either paged (for user PVDs) or
 *  validation area for system PVDs. The read completed, now check the checksums of the data.
 *
 * @param memory_request_p         - pointer to the memory request
 * @param context                  - context.
 *
 * @return  fbe_status_t  
 *
 * @author
 *  2/21/2014 - Created. Rob Foley
 *
 ******************************************************************************/
static fbe_status_t fbe_provision_drive_verify_paged_completion(fbe_packet_t * packet_p,
                                                                fbe_packet_completion_context_t context)
{
    fbe_status_t                            status;
    fbe_provision_drive_t                  *provision_drive_p = NULL;
    fbe_payload_block_operation_t *block_operation_p = NULL;
    fbe_payload_ex_t                       *payload_p = NULL;
    fbe_payload_ex_t                       *master_payload_p = NULL;
    fbe_sector_t                           *sector_p = NULL;
    fbe_u16_t                               checksum;
    fbe_sg_element_t                       *sg_p = NULL;
    fbe_u32_t                               crc_err_count = 0;
    fbe_u32_t                               sector_index;
    fbe_memory_request_t                   *memory_request_p = NULL;
    fbe_packet_t                           *master_packet_p = NULL;
    fbe_scheduler_hook_status_t             hook_status;
    fbe_base_config_encryption_state_t      encryption_state;
    fbe_payload_control_operation_t        *control_operation_p = NULL;
    fbe_u8_t                               *buffer_p = NULL;
    fbe_base_port_mgmt_register_keys_t     *port_register_keys = NULL;
    fbe_provision_drive_register_keys_context_t *context_p = NULL;
    fbe_object_id_t object_id;
    fbe_bool_t is_system_drive;

    provision_drive_p = (fbe_provision_drive_t *)context;
    payload_p = fbe_transport_get_payload_ex(packet_p);
    block_operation_p = fbe_payload_ex_get_block_operation(payload_p);
    fbe_payload_ex_get_sg_list(payload_p, &sg_p, NULL);
    master_packet_p = fbe_transport_get_master_packet(packet_p);
    memory_request_p = fbe_transport_get_memory_request(master_packet_p);
    fbe_transport_remove_subpacket(packet_p);

    fbe_base_object_get_object_id((fbe_base_object_t *)provision_drive_p, &object_id);
    is_system_drive = fbe_database_is_object_system_pvd(object_id);

    status = fbe_transport_get_status_code(packet_p);

    fbe_payload_ex_release_block_operation(payload_p, block_operation_p);

    master_payload_p = fbe_transport_get_payload_ex(master_packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(master_payload_p);
    fbe_payload_control_get_buffer(control_operation_p, &buffer_p);
    port_register_keys = (fbe_base_port_mgmt_register_keys_t *) buffer_p;
    context_p = (fbe_provision_drive_register_keys_context_t*) (port_register_keys + 1);

    if ((status != FBE_STATUS_OK) || (block_operation_p->status != FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS)) {
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p,
                              FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                              "provision_drive: validate paged st: 0x%x bl_s: 0x%x bl_q: 0x%x \n",
                              status, block_operation_p->status, block_operation_p->status_qualifier);
        fbe_transport_set_status(master_packet_p, FBE_STATUS_GENERIC_FAILURE, 0);

        /* For certain errors we would like to set the encryption state so we do not retry forever. 
         * A new key push will clear this encryption state. 
         */
        if ( (block_operation_p->status == FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED) &&
             ((block_operation_p->status_qualifier == FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_KEY_WRAP_ERROR) ||
              (block_operation_p->status_qualifier == FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_ENCRYPTION_NOT_ENABLED) ||
              (block_operation_p->status_qualifier == FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_BAD_KEY_HANDLE)) ) {
            
            if (block_operation_p->status_qualifier == FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_BAD_KEY_HANDLE) {
                /* We have chosen to critical error since there is no recovery for this and we will critical error
                 * eventually anyway. 
                 */
                fbe_base_object_trace((fbe_base_object_t *)provision_drive_p, FBE_TRACE_LEVEL_CRITICAL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                      "provision_drive: unexpected key not found error client: %d\n", context_p->client_index);
            }
            fbe_base_config_get_encryption_state((fbe_base_config_t*)provision_drive_p, &encryption_state);
            fbe_base_object_trace((fbe_base_object_t *)provision_drive_p, FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                  "provision_drive: encryption state %d -> LOCKED_KEY_ERROR client: %d\n", 
                                  encryption_state, context_p->client_index);
            fbe_provision_drive_set_encryption_state(provision_drive_p, context_p->client_index, FBE_BASE_CONFIG_ENCRYPTION_STATE_LOCKED_KEY_ERROR);
            fbe_provision_drive_check_hook(provision_drive_p,
                                           SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_VALIDATE_KEYS,
                                           fbe_provision_drive_get_encryption_qual_substate(block_operation_p->status_qualifier),
                                           0,  &hook_status);

            /* Unregister since there is something wrong with the key.
             */
            if (block_operation_p->status_qualifier == FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_KEY_WRAP_ERROR){
                fbe_transport_destroy_packet(packet_p);
                fbe_memory_free_request_entry(memory_request_p);
                /* Unregister keys so that when we get another push we will know to re-register the keys.
                 */
                fbe_provision_drive_port_unregister_keys(provision_drive_p, master_packet_p, context_p->client_index);
                return FBE_STATUS_MORE_PROCESSING_REQUIRED;
            }
        } 
    } else {
        /* Status is good since we successfully did the read.
         */
        fbe_transport_set_status(master_packet_p, FBE_STATUS_OK, 0);
        /* Check the checksum of the blocks we just read.
         */
        sector_p = (fbe_sector_t*) sg_p->address;
        for (sector_index = 0; sector_index < block_operation_p->block_count; sector_index++) {
            checksum = fbe_xor_lib_calculate_checksum((fbe_u32_t*)sector_p);
            if (checksum != sector_p->crc) {
                crc_err_count++;
            }
            sector_p++;
        }
        if (crc_err_count >= FBE_PROVISION_DRIVE_MAX_CRC_ERR_FOR_KEY_CHECK) {
            fbe_provision_drive_set_encryption_state(provision_drive_p, context_p->client_index, 
                                                     FBE_BASE_CONFIG_ENCRYPTION_STATE_LOCKED_KEYS_INCORRECT);
            fbe_provision_drive_check_hook(provision_drive_p,
                                       SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_ENCRYPTION, 
                                       FBE_PROVISION_DRIVE_SUBSTATE_ENCRYPTION_INCORRECT_KEYS, 
                                       0,  &hook_status);
            fbe_base_object_trace((fbe_base_object_t *)provision_drive_p,
                                  FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                  "provision_drive: encryption validation found %d of 64 blocks with crc errors client: %d\n",
                                  crc_err_count, context_p->client_index);
            /* Given that the drive is not able to be used without the correct keys, we log an event.
             */
            fbe_provision_drive_write_event_log(provision_drive_p, SEP_ERROR_CODE_PROVISION_DRIVE_INCORRECT_KEY);

            fbe_transport_destroy_packet(packet_p);
            fbe_memory_free_request_entry(memory_request_p);

            /* Unregister keys so that when we get another push we will know to re-register the keys.
             */
            fbe_provision_drive_port_unregister_keys(provision_drive_p, master_packet_p, context_p->client_index);
            return FBE_STATUS_MORE_PROCESSING_REQUIRED;
        } else {
            fbe_base_object_trace((fbe_base_object_t *)provision_drive_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                  "provision_drive: encryption validation successful. client: %d\n", context_p->client_index);
            fbe_lifecycle_reschedule(&fbe_provision_drive_lifecycle_const,
                                     (fbe_base_object_t*)provision_drive_p,
                                     (fbe_lifecycle_timer_msec_t)0);

            if (is_system_drive) {
                /* Now allow the edge to go ready for system drives.
                 */
                fbe_provision_drive_mark_edge_state(provision_drive_p, context_p->client_index);
            }
        }
    }

    fbe_transport_destroy_packet(packet_p);
    fbe_memory_free_request_entry(memory_request_p);

    fbe_transport_complete_packet(master_packet_p);
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/******************************************************************************
 * end fbe_provision_drive_verify_paged_completion()
 ******************************************************************************/
/*!**************************************************************
 * fbe_provision_drive_get_substate_for_qualifier()
 ****************************************************************
 * @brief
 *  Fetch the substate associated with this encryption qualifier.
 *
 * @param qualifier
 * 
 * @return fbe_u32_t
 *
 * @author
 *  3/6/2014 - Created. Rob Foley
 *
 ****************************************************************/

fbe_u32_t fbe_provision_drive_get_encryption_qual_substate(fbe_payload_block_operation_qualifier_t qualifier)
{
    fbe_u32_t substate;
    switch (qualifier) {
        case FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_BAD_KEY_HANDLE:
            substate = FBE_PROVISION_DRIVE_SUBSTATE_ENCRYPTION_BAD_KEY_HANDLE;
            break;
        case FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_KEY_WRAP_ERROR:
            substate = FBE_PROVISION_DRIVE_SUBSTATE_ENCRYPTION_KEY_WRAP_ERROR;
            break;
        case FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_ENCRYPTION_NOT_ENABLED:
            substate = FBE_PROVISION_DRIVE_SUBSTATE_ENCRYPTION_NOT_ENABLED;
            break;
        default:
            substate = 0;
            break;
    }
    return substate;
}
/******************************************
 * end fbe_provision_drive_get_substate_for_qualifier()
 ******************************************/
/*!**************************************************************
 * fbe_provision_drive_set_read_validate_key()
 ****************************************************************
 * @brief
 *  Determine which key to use for reading validation area.
 *
 * @param provision_drive_p
 * @param packet_p
 * @param client_index
 * 
 * @return fbe_status_t
 *
 * @author
 *  3/11/2014 - Created. Rob Foley
 *
 ****************************************************************/
static fbe_status_t fbe_provision_drive_set_read_validate_key(fbe_provision_drive_t * provision_drive_p,
                                                           fbe_packet_t *packet_p,
                                                           fbe_edge_index_t client_index)
{
    fbe_encryption_key_mask_t           mask0, mask1;
    fbe_status_t                        status;
    fbe_payload_ex_t                   *sep_payload_p = NULL;
    fbe_payload_block_operation_t      *block_operation_p = NULL;
    fbe_lba_t                           lba;
    fbe_provision_drive_key_info_t     *key_info_p = NULL;
    fbe_u32_t                           validation_area_bitmap;

    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    block_operation_p = fbe_payload_ex_get_block_operation(sep_payload_p);

    fbe_payload_block_get_lba(block_operation_p, &lba);

    status = fbe_provision_drive_get_key_info(provision_drive_p, client_index, &key_info_p);
    fbe_encryption_key_mask_get(&key_info_p->key_handle->key_mask, 0, &mask0);
    fbe_encryption_key_mask_get(&key_info_p->key_handle->key_mask, 1, &mask1);
    fbe_provision_drive_get_validation_area_bitmap(provision_drive_p, &validation_area_bitmap);

    if (validation_area_bitmap & (FBE_PROVISION_DRIVE_KEY1_FLAG << (client_index * FBE_PROVISION_DRIVE_VALIDATION_BITS))) {
        /* We have a second key, so use it.
         */
        fbe_payload_ex_set_key_handle(sep_payload_p, key_info_p->mp_key_handle_2);
    } else {

        /* Use the only key.
         */
        fbe_payload_ex_set_key_handle(sep_payload_p, key_info_p->mp_key_handle_1);
    }
    fbe_provision_drive_utils_trace(provision_drive_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                    FBE_PROVISION_DRIVE_DEBUG_FLAG_ENCRYPTION,
                                    "PVD: rd validate use key handle 0x%llx op: 0x%x lba: 0x%llx bl: 0x%llx i:%d bm:0x%x\n",
                                    sep_payload_p->key_handle, block_operation_p->block_opcode,
                                    block_operation_p->lba, block_operation_p->block_count,
                                    client_index, validation_area_bitmap);
    return FBE_STATUS_OK;
} 
/******************************************
 * end fbe_provision_drive_set_read_validate_key()
 ******************************************/

/*!**************************************************************
 * fbe_provision_drive_set_write_validation_key()
 ****************************************************************
 * @brief
 *  Determine which key to use for validation area when writing it.
 *
 * @param provision_drive_p
 * @param packet_p
 * @param client_index
 * 
 * @return fbe_status_t
 *
 * @author
 *  3/11/2014 - Created. Rob Foley
 *
 ****************************************************************/
static fbe_status_t fbe_provision_drive_set_write_validation_key(fbe_provision_drive_t * provision_drive_p,
                                                           fbe_packet_t *packet_p,
                                                           fbe_edge_index_t client_index)
{
    fbe_encryption_key_mask_t           mask0, mask1;
    fbe_status_t                        status;
    fbe_payload_ex_t                   *sep_payload_p = NULL;
    fbe_payload_block_operation_t      *block_operation_p = NULL;
    fbe_lba_t                           lba;
    fbe_provision_drive_key_info_t     *key_info_p = NULL;
    fbe_u32_t                           validation_area_bitmap;

    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    block_operation_p = fbe_payload_ex_get_block_operation(sep_payload_p);

    fbe_payload_block_get_lba(block_operation_p, &lba);

    status = fbe_provision_drive_get_key_info(provision_drive_p, client_index, &key_info_p);
    fbe_encryption_key_mask_get(&key_info_p->key_handle->key_mask, 0, &mask0);
    fbe_encryption_key_mask_get(&key_info_p->key_handle->key_mask, 1, &mask1);
    fbe_provision_drive_get_validation_area_bitmap(provision_drive_p, &validation_area_bitmap);

    if (mask1 & FBE_ENCRYPTION_KEY_MASK_VALID) {
        /* We have a second key, so use it to write the new validation area.
         */
        fbe_payload_ex_set_key_handle(sep_payload_p, key_info_p->mp_key_handle_2);
    } else {

        /* Use the only key.
         */
        fbe_payload_ex_set_key_handle(sep_payload_p, key_info_p->mp_key_handle_1);
    }
    fbe_provision_drive_utils_trace(provision_drive_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                    FBE_PROVISION_DRIVE_DEBUG_FLAG_ENCRYPTION,
                                    "PVD: wr validate use key handle 0x%llx op: 0x%x lba: 0x%llx bl: 0x%llx i:%d bm:0x%x\n",
                                    sep_payload_p->key_handle, block_operation_p->block_opcode,
                                    block_operation_p->lba, block_operation_p->block_count,
                                    client_index, validation_area_bitmap);
    return FBE_STATUS_OK;
} 
/******************************************
 * end fbe_provision_drive_set_write_validation_key()
 ******************************************/
/*!****************************************************************************
 * fbe_provision_drive_init_validation_area()
 ******************************************************************************
 * @brief
 *   Start the allocate of memory to perform an init of the validation area,
 *   which is used on system drives to validate the keys.
 *   This verify is needed to validate that the keys are correct.
 *
 * @param provision_drive_p         - pointer to the provision drive
 * @param packet_p                  - pointer to a monitor packet.
 *
 * @return  fbe_status_t  
 *
 * @author
 *  3/4/2014 - Created. Rob Foley
 *
 ******************************************************************************/
fbe_status_t fbe_provision_drive_init_validation_area(fbe_packet_t * packet_p,
                                                      fbe_packet_completion_context_t context)
{
    fbe_status_t                                 status = FBE_STATUS_GENERIC_FAILURE;
    fbe_provision_drive_t                       *provision_drive_p = NULL; 
    fbe_payload_ex_t                            *payload_p = NULL;
    fbe_payload_control_operation_t             *control_operation_p = NULL;
    fbe_base_port_mgmt_register_keys_t          *port_register_keys = NULL;
    fbe_u8_t                                    *buffer_p = NULL;
    fbe_provision_drive_register_keys_context_t *context_p = NULL;
    fbe_memory_request_t                        *memory_request_p = NULL;

    provision_drive_p = (fbe_provision_drive_t *)context;
    payload_p = fbe_transport_get_payload_ex(packet_p);
    status = fbe_transport_get_status_code(packet_p);

    control_operation_p = fbe_payload_ex_get_prev_control_operation(payload_p);
    fbe_payload_control_get_buffer(control_operation_p, &buffer_p);
    port_register_keys = (fbe_base_port_mgmt_register_keys_t *) buffer_p;
    context_p = (fbe_provision_drive_register_keys_context_t*) (port_register_keys + 1);

    /* If status is not good then we failed to get the NP lock.
     * Return the failure to call completion.
     */
    if (status != FBE_STATUS_OK){
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,    
                               "%s: could not get np lock stat:0x%x, \n",
                               __FUNCTION__, status);
        fbe_transport_set_status(packet_p, status, 0);
        return status;
    }
    
    /* We own the NP now. We must always release it.
     */
    fbe_transport_set_completion_function(packet_p, fbe_provision_drive_release_NP_lock, provision_drive_p);

    /* Now that we have the NP lock, make sure we really need to do this.  The peer might have initialized it first.
     */
    if (!fbe_provision_drive_validation_area_needs_init(provision_drive_p, context_p->client_index)){
        fbe_u32_t bitmap;
        fbe_provision_drive_get_validation_area_bitmap(provision_drive_p, &bitmap);
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p,
                              FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "provision_drive: validate already initted for client: %d bitmap: 0x%x\n", 
                              context_p->client_index, bitmap);

        fbe_provision_drive_mark_edge_state(provision_drive_p, context_p->client_index);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }
    /* allocate memory for sg list and buffer. */
    memory_request_p = fbe_transport_get_memory_request(packet_p);
    fbe_memory_build_chunk_request(memory_request_p,
                                   FBE_MEMORY_CHUNK_SIZE_FOR_64_BLOCKS_IO,
                                   1, /* number of chunks */
                                   fbe_transport_get_resource_priority(packet_p),
                                   fbe_transport_get_io_stamp(packet_p),
                                   (fbe_memory_completion_function_t)fbe_provision_drive_init_validation_area_allocate_completion,
                                   provision_drive_p);
    fbe_transport_memory_request_set_io_master(memory_request_p, packet_p);

    /* Issue the memory request to memory service. */
    status = fbe_memory_request_entry(memory_request_p);
    if((status != FBE_STATUS_OK) && (status != FBE_STATUS_PENDING))
    {
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s memory allocation failed",  __FUNCTION__);
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return status;
    }

    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/******************************************************************************
 * end fbe_provision_drive_init_validation_area()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_provision_drive_init_validation_area_allocate_completion()
 ******************************************************************************
 * @brief
 *  This is the completion for memory allocation for init of validation area.
 *
 * @param memory_request_p         - pointer to the memory request
 * @param context                  - context.
 *
 * @return  fbe_status_t  
 *
 * @author
 *  3/4/2014 - Created. Rob Foley
 *
 ******************************************************************************/
static fbe_status_t fbe_provision_drive_init_validation_area_allocate_completion(fbe_memory_request_t * memory_request_p, 
                                                                         fbe_memory_completion_context_t context)
{
    fbe_payload_ex_t                       *payload_p = NULL;
    fbe_lba_t                               paged_metadata_capacity = 0;
    fbe_lba_t                               paged_metadata_lba = 0;
    fbe_block_count_t                       block_count;
    fbe_packet_t                           *packet_p = NULL;
    fbe_provision_drive_t                  *provision_drive_p = (fbe_provision_drive_t *)context;
    fbe_memory_header_t                    *memory_header_p = NULL;
    fbe_sg_element_t                       *sg_p = NULL;
    fbe_payload_block_operation_t          *block_operation_p = NULL;
    fbe_object_id_t                         pvd_object_id;
    fbe_cpu_id_t                            cpu_id;
    fbe_packet_t                           *new_packet_p = NULL;
    fbe_payload_ex_t                       *new_payload_p = NULL;
    fbe_payload_control_operation_t        *control_operation_p = NULL;
    fbe_base_port_mgmt_register_keys_t     *port_register_keys = NULL;
    fbe_u8_t                               *buffer_p = NULL;
    fbe_provision_drive_register_keys_context_t * context_p = NULL;
    fbe_lba_t       paged_metadata_offset;
    fbe_u32_t       zero_bit_bucket_size_in_blocks;
    fbe_u8_t *      zero_bit_bucket_p = NULL;
    fbe_status_t    status;

    /* get the payload and control operation. */
    packet_p = fbe_transport_memory_request_to_packet(memory_request_p);
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_prev_control_operation(payload_p);

    fbe_payload_control_get_buffer(control_operation_p, &buffer_p);
    port_register_keys = (fbe_base_port_mgmt_register_keys_t *) buffer_p;
    context_p = (fbe_provision_drive_register_keys_context_t*) (port_register_keys + 1);

    /* Check allocation status */
    if (fbe_memory_request_is_allocation_complete(memory_request_p) == FBE_FALSE) {

        /* Currently this callback doesn't expect any aborted requests */
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s memory allocation failed, state:%d\n",
                              __FUNCTION__, memory_request_p->request_state);
        fbe_transport_set_status(packet_p, FBE_STATUS_INSUFFICIENT_RESOURCES, 0);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    memory_header_p = fbe_memory_get_first_memory_header(memory_request_p);
    new_packet_p = (fbe_packet_t *)fbe_memory_header_to_data(memory_header_p);

    /* get the zero bit bucket address and its size before we plant the sgl */
    status = fbe_memory_get_zero_bit_bucket(&zero_bit_bucket_p, &zero_bit_bucket_size_in_blocks);
    if((status != FBE_STATUS_OK) || 
       (zero_bit_bucket_p == NULL) ||
       (zero_bit_bucket_size_in_blocks < FBE_MEMORY_CHUNK_SIZE_BLOCK_COUNT_64)) {

        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s zero bitbucket too small st: %d size: %d \n",
                              __FUNCTION__, status, zero_bit_bucket_size_in_blocks);
        fbe_transport_set_status(packet_p, FBE_STATUS_INSUFFICIENT_RESOURCES, 0);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    sg_p = (fbe_sg_element_t*)(new_packet_p + 1);
    sg_p->address = zero_bit_bucket_p;
    sg_p->count = FBE_MEMORY_CHUNK_SIZE_BLOCK_COUNT_64 * FBE_BE_BYTES_PER_BLOCK;
    fbe_sg_element_terminate(&sg_p[1]);

    fbe_transport_initialize_packet(new_packet_p);
    fbe_base_object_get_object_id((fbe_base_object_t *) provision_drive_p, &pvd_object_id);
    fbe_transport_set_address(new_packet_p,
                              FBE_PACKAGE_ID_SEP_0,
                              FBE_SERVICE_ID_TOPOLOGY,
                              FBE_CLASS_ID_INVALID,
                              pvd_object_id);
    new_payload_p = fbe_transport_get_payload_ex(new_packet_p);

    cpu_id = (memory_request_p->io_stamp & FBE_PACKET_IO_STAMP_MASK) >> FBE_PACKET_IO_STAMP_SHIFT;
    fbe_transport_set_cpu_id(new_packet_p, cpu_id);

    /* Set the repeat count as number of records in paged metadata capacity.
     * This capacity not only includes the paged metadata but also the second
     * copy.
     */
    fbe_base_config_metadata_get_paged_metadata_capacity((fbe_base_config_t *)provision_drive_p,
                                                       &paged_metadata_capacity);
    fbe_base_config_metadata_get_paged_record_start_lba((fbe_base_config_t *)provision_drive_p,
                                                        &paged_metadata_lba);
    fbe_base_config_metadata_get_paged_mirror_metadata_offset((fbe_base_config_t *)provision_drive_p,
                                                              &paged_metadata_offset);
    block_count = FBE_MIN(paged_metadata_capacity, FBE_MEMORY_CHUNK_SIZE_BLOCK_COUNT_64);

    block_operation_p = fbe_payload_ex_allocate_block_operation(new_payload_p);

    /* System drives use a special validation area per client.
     */
    paged_metadata_lba += paged_metadata_offset; 
    paged_metadata_lba += context_p->client_index * FBE_MEMORY_CHUNK_SIZE_BLOCK_COUNT_64;

    fbe_payload_block_build_operation(block_operation_p, FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,
                                      paged_metadata_lba, block_count, FBE_BE_BYTES_PER_BLOCK, 1,
                                      NULL);
    fbe_payload_ex_set_sg_list(new_payload_p, sg_p, 0);

    fbe_transport_add_subpacket(packet_p, new_packet_p);
    /* set the completion function for the default paged metadata write. */
    fbe_transport_set_completion_function(new_packet_p, fbe_provision_drive_init_validation_area_completion, provision_drive_p);

    /* Need to do this before set key handle so it sees our block operation. */
    fbe_payload_ex_increment_block_operation_level(new_payload_p);
    fbe_provision_drive_set_write_validation_key(provision_drive_p, new_packet_p, context_p->client_index);

    fbe_provision_drive_utils_trace(provision_drive_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                    FBE_PROVISION_DRIVE_DEBUG_FLAG_ENCRYPTION,
                                    "provision_drive: writing validation area for client: %d lba: 0x%llx\n",
                                    context_p->client_index, block_operation_p->lba);
    fbe_base_config_send_functional_packet((fbe_base_config_t *)provision_drive_p, new_packet_p, 0);
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/******************************************************************************
 * end fbe_provision_drive_init_validation_area_allocate_completion()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_provision_drive_init_validation_area_completion()
 ******************************************************************************
 * @brief
 *  This is the completion for init of validation area.
 *
 * @param memory_request_p         - pointer to the memory request
 * @param context                  - context.
 *
 * @return  fbe_status_t  
 *
 * @author
 *  3/4/2014 - Created. Rob Foley
 *
 ******************************************************************************/
static fbe_status_t fbe_provision_drive_init_validation_area_completion(fbe_packet_t * packet_p,
                                                                        fbe_packet_completion_context_t context)
{
    fbe_status_t                            status;
    fbe_provision_drive_t                  *provision_drive_p = NULL;
    fbe_payload_block_operation_t *block_operation_p = NULL;
    fbe_payload_control_operation_t *control_operation_p = NULL;
    fbe_payload_ex_t                       *payload_p = NULL;
    fbe_payload_ex_t                       *master_payload_p = NULL;
    fbe_sg_element_t                       *sg_p = NULL;
    fbe_memory_request_t                   *memory_request_p = NULL;
    fbe_packet_t                           *master_packet_p = NULL;
    fbe_u8_t                                    *buffer_p = NULL;
    fbe_base_port_mgmt_register_keys_t          *port_register_keys = NULL;
    fbe_provision_drive_register_keys_context_t *context_p = NULL;
    fbe_base_config_encryption_state_t encryption_state;
    fbe_scheduler_hook_status_t        hook_status;

    provision_drive_p = (fbe_provision_drive_t *)context;
    payload_p = fbe_transport_get_payload_ex(packet_p);
    block_operation_p = fbe_payload_ex_get_block_operation(payload_p);
    fbe_payload_ex_get_sg_list(payload_p, &sg_p, NULL);
    master_packet_p = fbe_transport_get_master_packet(packet_p);
    memory_request_p = fbe_transport_get_memory_request(master_packet_p);
    fbe_transport_remove_subpacket(packet_p);

    master_payload_p = fbe_transport_get_payload_ex(master_packet_p);
    control_operation_p = fbe_payload_ex_get_prev_control_operation(master_payload_p);

    block_operation_p = fbe_payload_ex_get_block_operation(payload_p);

    status = fbe_transport_get_status_code(packet_p);

    fbe_payload_ex_release_block_operation(payload_p, block_operation_p);

    fbe_payload_control_get_buffer(control_operation_p, &buffer_p);
    port_register_keys = (fbe_base_port_mgmt_register_keys_t *) buffer_p;
    context_p = (fbe_provision_drive_register_keys_context_t*) (port_register_keys + 1);

    if ((status != FBE_STATUS_OK) || (block_operation_p->status != FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS)) {
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p,
                              FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s: status: 0x%x block_status: 0x%x\n",__FUNCTION__, status, block_operation_p->status);
        fbe_transport_set_status(master_packet_p, FBE_STATUS_GENERIC_FAILURE, 0);

        if ( (block_operation_p->status == FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED) &&
             ((block_operation_p->status_qualifier == FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_KEY_WRAP_ERROR) ||
              (block_operation_p->status_qualifier == FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_ENCRYPTION_NOT_ENABLED) ||
              (block_operation_p->status_qualifier == FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_BAD_KEY_HANDLE)) ) {
            
            if (block_operation_p->status_qualifier == FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_BAD_KEY_HANDLE) {
                /* We have chosen to critical error since there is no recovery for this and we will critical error
                 * eventually anyway. 
                 */
                fbe_base_object_trace((fbe_base_object_t *)provision_drive_p, FBE_TRACE_LEVEL_CRITICAL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                      "provision_drive: validation area key not found error client: %d\n", context_p->client_index);
            }
            fbe_base_config_get_encryption_state((fbe_base_config_t*)provision_drive_p, &encryption_state);
            fbe_base_object_trace((fbe_base_object_t *)provision_drive_p, FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                  "provision_drive: validation encryption state %d -> LOCKED_KEY_ERROR client: %d\n", 
                                  encryption_state, context_p->client_index);
            fbe_provision_drive_set_encryption_state(provision_drive_p, context_p->client_index, FBE_BASE_CONFIG_ENCRYPTION_STATE_LOCKED_KEY_ERROR);
            fbe_provision_drive_check_hook(provision_drive_p,
                                           SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_INIT_VALIDATION_AREA,
                                           fbe_provision_drive_get_encryption_qual_substate(block_operation_p->status_qualifier),
                                           0,  &hook_status);

            /* Unregister since there is something wrong with the key.
             */
            if (block_operation_p->status_qualifier == FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_KEY_WRAP_ERROR){
                fbe_transport_destroy_packet(packet_p);
                fbe_memory_free_request_entry(memory_request_p);
                /* Unregister keys so that when we get another push we will know to re-register the keys.
                 */
                fbe_provision_drive_port_unregister_keys(provision_drive_p, master_packet_p, context_p->client_index);
                return FBE_STATUS_MORE_PROCESSING_REQUIRED;
            }
        }
        fbe_transport_destroy_packet(packet_p);
        fbe_memory_free_request_entry(memory_request_p);
        fbe_transport_complete_packet(master_packet_p);
    } else {

        fbe_provision_drive_utils_trace(provision_drive_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                        FBE_PROVISION_DRIVE_DEBUG_FLAG_ENCRYPTION,
                                        "provision_drive: successfully updated validation area for client: %d lba: 0x%llx\n",
                                        context_p->client_index, block_operation_p->lba);

        fbe_transport_destroy_packet(packet_p);
        fbe_memory_free_request_entry(memory_request_p);
        fbe_provision_drive_encryption_validation_set(master_packet_p, provision_drive_p);
    }
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/******************************************************************************
 * end fbe_provision_drive_init_validation_area_completion()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_provision_drive_encryption_validation_set()
 ******************************************************************************
 * @brief
 *  Update the encryption validation bitmap.
 *
 * @param packet_p     - Packet requesting operation.
 * @param context      - Packet completion context (provision drive object).
 *
 * @return status - The status of the operation.
 * 
 * @notes We are assumed to have the NP lock already.
 * 
 * @author
 *  3/4/2014 - Created. Rob Foley
 *
 ******************************************************************************/
fbe_status_t fbe_provision_drive_encryption_validation_set(fbe_packet_t * packet_p,
                                                           fbe_packet_completion_context_t context)
{
    fbe_status_t                            status;
    fbe_provision_drive_t                  *provision_drive_p = NULL;
    fbe_payload_control_operation_t *control_operation_p = NULL;
    fbe_base_port_mgmt_register_keys_t *                    port_register_keys = NULL;
    fbe_u8_t *                                              buffer_p = NULL;
    fbe_provision_drive_register_keys_context_t *context_p = NULL;
    fbe_payload_ex_t                            *payload_p = NULL;
    fbe_u32_t bitmap;
    fbe_u32_t current_bitmap;
    fbe_encryption_key_mask_t           mask0, mask1;
    fbe_provision_drive_key_info_t     *key_info_p = NULL;
    
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_prev_control_operation(payload_p);

    fbe_payload_control_get_buffer(control_operation_p, &buffer_p);
    port_register_keys = (fbe_base_port_mgmt_register_keys_t *) buffer_p;
    context_p = (fbe_provision_drive_register_keys_context_t*) (port_register_keys + 1);

    provision_drive_p = (fbe_provision_drive_t *)context;
    status = fbe_transport_get_status_code(packet_p);

    /* If status is not good then we failed to get the NP lock.
     * Return the failure to call completion.
     */
    if (status != FBE_STATUS_OK){
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,    
                               "%s: could not get np lock stat:0x%x, \n",
                               __FUNCTION__, status);
        fbe_transport_set_status(packet_p, status, 0);
        return status;
    }
    
    /* Get nonpaged metadata pointer, if it is null or status is not good then
     * return error.
     */
    fbe_provision_drive_get_validation_area_bitmap(provision_drive_p, &current_bitmap);
  
    /* We own the NP.  Write out the new bitmap.
     */
    fbe_transport_set_completion_function(packet_p, fbe_provision_drive_validation_set_completion, provision_drive_p);

    status = fbe_provision_drive_get_key_info(provision_drive_p, context_p->client_index, &key_info_p);
    fbe_encryption_key_mask_get(&key_info_p->key_handle->key_mask, 0, &mask0);
    fbe_encryption_key_mask_get(&key_info_p->key_handle->key_mask, 1, &mask1);

    /* If we are encrypting, then indicate the second key is used to validate paged. 
     * Otherwise we must be done encrypting, in which case just the first key is used to validate paged.
     */
    if (mask1 & FBE_ENCRYPTION_KEY_MASK_VALID) {
        bitmap = (current_bitmap & ~(FBE_PROVISION_DRIVE_KEYS_MASK << (context_p->client_index * FBE_PROVISION_DRIVE_VALIDATION_BITS))) |
                 (FBE_PROVISION_DRIVE_KEY1_FLAG << (context_p->client_index * FBE_PROVISION_DRIVE_VALIDATION_BITS));
    } else {
        bitmap = (current_bitmap & ~(FBE_PROVISION_DRIVE_KEYS_MASK << (context_p->client_index * FBE_PROVISION_DRIVE_VALIDATION_BITS))) |
                 (FBE_PROVISION_DRIVE_KEY0_FLAG << (context_p->client_index * FBE_PROVISION_DRIVE_VALIDATION_BITS));
    }

    fbe_base_object_trace((fbe_base_object_t *)provision_drive_p,
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,    
                          "provision_drive: encr valid area set current_bm: 0x%x set client %d new_bm: 0x%x\n",
                          current_bitmap, context_p->client_index, bitmap);

    /* set the zero on demand flag using base config nonpaged write. */
    status = fbe_base_config_metadata_nonpaged_write_persist((fbe_base_config_t *) provision_drive_p,
                                                             packet_p,
                     (fbe_u64_t)(&((fbe_provision_drive_nonpaged_metadata_t*)0)->validate_area_bitmap),
                                                             (fbe_u8_t *)&bitmap,
                                                             sizeof(bitmap));
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/********************************************************* 
* end fbe_provision_drive_encryption_validation_set()
**********************************************************/

/*!****************************************************************************
 * fbe_provision_drive_validation_set_completion()
 ******************************************************************************
 * @brief
 *  We just finished persisting the set of the client bit.
 *
 * @param packet_p     - Packet requesting operation.
 * @param context      - Packet completion context (provision drive object).
 *
 * @return status - The status of the operation.
 *
 * @author
 *  3/4/2014 - Created. Rob Foley
 *
 ******************************************************************************/
static fbe_status_t 
fbe_provision_drive_validation_set_completion(fbe_packet_t * packet_p,
                                              fbe_packet_completion_context_t context)
{
    fbe_status_t                            status;
    fbe_provision_drive_t                  *provision_drive_p = NULL;
    fbe_payload_ex_t *payload_p = NULL;
    fbe_payload_control_operation_t                      *control_operation_p = NULL;
    fbe_base_port_mgmt_register_keys_t                   *port_register_keys = NULL;
    fbe_u8_t *                                            buffer_p = NULL;
    fbe_provision_drive_register_keys_context_t          *context_p = NULL;

    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_prev_control_operation(payload_p);

    fbe_payload_control_get_buffer(control_operation_p, &buffer_p);
    port_register_keys = (fbe_base_port_mgmt_register_keys_t *) buffer_p;
    context_p = (fbe_provision_drive_register_keys_context_t*) (port_register_keys + 1);

    provision_drive_p = (fbe_provision_drive_t *)context;
    status = fbe_transport_get_status_code(packet_p);

    /* If status is not good then complete the packet with error. */
    if (status != FBE_STATUS_OK){
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,    
                               "%s: mark of client validation failed stat:0x%x, \n",
                               __FUNCTION__, status);
        fbe_transport_set_status(packet_p, status, 0);
        return status;
    }
    /* Now allow the edge to go ready.
     */
    fbe_provision_drive_mark_edge_state(provision_drive_p, context_p->client_index);

    return FBE_STATUS_OK;
}
/********************************************************* 
* end fbe_provision_drive_validation_set_completion()
**********************************************************/
/*!****************************************************************************
 * fbe_provision_drive_validation_area_needs_init()
 ******************************************************************************
 * @brief
 *  Determine if client has initted this area yet.
 *
 * @param provision_drive_p - pointer to the provision drive
 * @param client_index       - pointer to the end of life state.
 *
 * @return  fbe_bool_t FBE_TRUE - We are already initialized, no need to init.
 *                    FBE_FALSE - We need to write out the validation area.
 *
 * @author
 *  3/4/2014 - Created. Rob Foley
 *
 ******************************************************************************/
fbe_bool_t    
fbe_provision_drive_validation_area_needs_init(fbe_provision_drive_t * provision_drive_p,
                                               fbe_u32_t client_index) 
{
    fbe_provision_drive_nonpaged_metadata_t * provision_drive_nonpaged_metadata = NULL;
    fbe_status_t status;
    fbe_u32_t key0_flag = (FBE_PROVISION_DRIVE_KEY0_FLAG << (client_index * FBE_PROVISION_DRIVE_VALIDATION_BITS));
    fbe_u32_t key1_flag = (FBE_PROVISION_DRIVE_KEY1_FLAG << (client_index * FBE_PROVISION_DRIVE_VALIDATION_BITS));
    fbe_encryption_key_mask_t           mask1;
    fbe_provision_drive_key_info_t     *key_info_p = NULL;

    /* get end of life state of the pvd object. */
    status = fbe_base_config_get_nonpaged_metadata_ptr((fbe_base_config_t *) provision_drive_p, (void **)&provision_drive_nonpaged_metadata);

   if((status != FBE_STATUS_OK) ||
       (provision_drive_nonpaged_metadata == NULL)){
        return FBE_TRUE;
    }
   /* If either key is in use it means that the validation area is initialized.
    */
   if ( ((provision_drive_nonpaged_metadata->validate_area_bitmap & key0_flag) == key0_flag) ||
         ((provision_drive_nonpaged_metadata->validate_area_bitmap & key1_flag) == key1_flag) ) {

       status = fbe_provision_drive_get_key_info(provision_drive_p, client_index, &key_info_p);
       fbe_encryption_key_mask_get(&key_info_p->key_handle->key_mask, 1, &mask1);

       if ( (mask1 & FBE_ENCRYPTION_KEY_MASK_VALID) && 
            ((provision_drive_nonpaged_metadata->validate_area_bitmap & key1_flag) != key1_flag) ){
           /* If we have a valid second key, but our key1 validation area is not valid, return 
            * false, since we need to init using the second key.
            */
           return FBE_TRUE;
       }
       return FBE_FALSE;
   }
   return FBE_TRUE;
}
/******************************************************************************
 * end fbe_provision_drive_validation_area_needs_init()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_provision_drive_get_validation_area_bitmap()
 ******************************************************************************
 * @brief
 *  fetch the bitmap of clients that initted their validation area.
 *
 * @param provision_drive_p - pointer to the provision drive
 * @param bitmap_p       - pointer to the return value.
 *
 * @return  fbe_status_t
 *
 * @author
 *  3/4/2014 - Created. Rob Foley
 *
 ******************************************************************************/
fbe_status_t    
fbe_provision_drive_get_validation_area_bitmap(fbe_provision_drive_t * provision_drive_p,
                                               fbe_u32_t *bitmap_p) 
{
    fbe_provision_drive_nonpaged_metadata_t * provision_drive_nonpaged_metadata = NULL;
    fbe_status_t status;

    /* get end of life state of the pvd object. */
    status = fbe_base_config_get_nonpaged_metadata_ptr((fbe_base_config_t *) provision_drive_p, (void **)&provision_drive_nonpaged_metadata);

    *bitmap_p = provision_drive_nonpaged_metadata->validate_area_bitmap;

    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_provision_drive_get_validation_area_bitmap()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_provision_drive_mark_edge_state()
 ******************************************************************************
 * @brief
 *  Transition the edge state according to the lifecycle state.
 *
 * @param provision_drive_p - Current provision_drive.
 * @param client_index - current client to mark.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  3/4/2014 - Created. Rob Foley
 *
 ******************************************************************************/
fbe_status_t fbe_provision_drive_mark_edge_state(fbe_provision_drive_t *provision_drive_p,
                                                 fbe_edge_index_t client_index)
{
    fbe_status_t status;
    fbe_path_state_t path_state;
    fbe_lifecycle_state_t lifecycle_state;

    status = fbe_lifecycle_get_state(&fbe_provision_drive_lifecycle_const,(fbe_base_object_t*)provision_drive_p, &lifecycle_state);
    status = fbe_base_transport_server_lifecycle_to_path_state(lifecycle_state, &path_state);
    
    fbe_block_transport_server_set_edge_path_state(&provision_drive_p->base_config.block_transport_server, 
                                                   client_index,
                                                   path_state);
    
    fbe_provision_drive_utils_trace(provision_drive_p, FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                    FBE_PROVISION_DRIVE_DEBUG_FLAG_ENCRYPTION,
                                    "provision_drive: mark edge state edge %d to path_state %d\n", 
                                    client_index, path_state);
    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_provision_drive_mark_edge_state()
 **************************************/

/*!****************************************************************************
 * fbe_provision_drive_encrypt_validate_update_all()
 ******************************************************************************
 * @brief
 *  Update the encryption validation bitmap for all clients to match the
 *  current state.
 *  This is used when we have finished encryption or rekey and need to
 *  set the validation bits to indicate which area is in use.
 *
 * @param packet_p     - Packet requesting operation.
 * @param context      - Packet completion context (provision drive object).
 *
 * @return status - The status of the operation.
 *
 * @author
 *  3/4/2014 - Created. Rob Foley
 *
 ******************************************************************************/
fbe_status_t fbe_provision_drive_encrypt_validate_update_all(fbe_packet_t * packet_p,
                                                             fbe_packet_completion_context_t context)
{
    fbe_status_t                      status;
    fbe_provision_drive_t            *provision_drive_p = NULL;
    fbe_u32_t                         bitmap;
    fbe_u32_t                         current_bitmap;
    fbe_encryption_key_mask_t         mask0, mask1;
    fbe_provision_drive_key_info_t   *key_info_p = NULL;
    fbe_u32_t                         number_of_clients;
    fbe_u32_t                         index;

    provision_drive_p = (fbe_provision_drive_t *)context;
    
    status = fbe_transport_get_status_code(packet_p);

    /* If status is not good then we failed to get the NP lock.
     * Return the failure to call completion.
     */
    if (status != FBE_STATUS_OK){
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,    
                               "%s: could not get np lock stat:0x%x, \n",
                               __FUNCTION__, status);
        fbe_transport_set_status(packet_p, status, 0);
        return status;
    }
    
    /* We own the NP. We must always release it.
     */
    fbe_transport_set_completion_function(packet_p, fbe_provision_drive_release_NP_lock, provision_drive_p);

    fbe_block_transport_server_get_number_of_clients(&((fbe_base_config_t *)provision_drive_p)->block_transport_server,
                                                     &number_of_clients);
    
    fbe_provision_drive_get_validation_area_bitmap(provision_drive_p, &current_bitmap);

    /* Our purpose is to switch from the key1 flag to the key0 flag whenever we become 
     * encrypted (from encryption in progress) 
     * Also, whenever we become unconsumed we would like to clear both bits. 
     */
    bitmap = 0;
    for (index = 0; index < number_of_clients; index++) {

        status = fbe_provision_drive_get_key_info(provision_drive_p, index, &key_info_p);
        if (key_info_p->key_handle != NULL) {
            fbe_encryption_key_mask_get(&key_info_p->key_handle->key_mask, 0, &mask0);
            fbe_encryption_key_mask_get(&key_info_p->key_handle->key_mask, 1, &mask1);

            /* Only set a bit if we already had one set. 
             * We might be in the case where we have not initialized this area yet. 
             */
            if ( current_bitmap & (FBE_PROVISION_DRIVE_KEYS_MASK << (index * FBE_PROVISION_DRIVE_VALIDATION_BITS)) ){
                /* If we are encrypting, then indicate the second key is used to validate paged. 
                 * Otherwise we must be done encrypting, in which case just the first key is used to validate paged.
                 */
                if ((mask1 & FBE_ENCRYPTION_KEY_MASK_VALID) && fbe_base_config_is_rekey_mode((fbe_base_config_t*)provision_drive_p)) {
                    bitmap |= (FBE_PROVISION_DRIVE_KEY1_FLAG << (index * FBE_PROVISION_DRIVE_VALIDATION_BITS));
                } else {
                    bitmap |= (FBE_PROVISION_DRIVE_KEY0_FLAG << (index * FBE_PROVISION_DRIVE_VALIDATION_BITS));
                }
            }
        }
    }
    fbe_base_object_trace((fbe_base_object_t *)provision_drive_p,
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,    
                          "provision_drive: encr validate update all set current_bm: 0x%x new_bm: 0x%x\n",
                          current_bitmap, bitmap);

    /* We own the NP.  Write out the new bitmap.
     */
    if (bitmap != current_bitmap) {

        /* set the zero on demand flag using base config nonpaged write. */
        status = fbe_base_config_metadata_nonpaged_write_persist((fbe_base_config_t *) provision_drive_p,
                                                                 packet_p,
                                                                 (fbe_u64_t)(&((fbe_provision_drive_nonpaged_metadata_t*)0)->validate_area_bitmap),
                                                                 (fbe_u8_t *)&bitmap,
                                                                 sizeof(bitmap));
    } else {
        fbe_transport_complete_packet(packet_p);
    }
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/********************************************************* 
* end fbe_provision_drive_encrypt_validate_update_all()
**********************************************************/


/*!****************************************************************************
 * @fn fbe_provision_drive_metadata_paged_init_client_blob()
 ******************************************************************************
 * @brief
 *
 *  This is the function to setup client blob for provision drives. 
 * 
 * @param packet            - Pointer to packet.
 * @param context           - Pointer to context.
 *
 * @return fbe_status_t.
 *  05/06/2014 - Created. Lili Chen
 *
 ******************************************************************************/
fbe_status_t 
fbe_provision_drive_metadata_paged_init_client_blob(fbe_packet_t * packet_p, fbe_payload_metadata_operation_t * metadata_operation_p)
{
    fbe_status_t                            status;
    fbe_memory_request_t *                  memory_request_p = NULL;
    fbe_packet_t *                          master_packet_p = NULL;

    master_packet_p = fbe_transport_get_master_packet(packet_p);
    if (master_packet_p == NULL) {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    memory_request_p = fbe_transport_get_memory_request(master_packet_p);
    status = fbe_metadata_paged_init_client_blob(memory_request_p, metadata_operation_p, FBE_MEMORY_CHUNK_SIZE_FOR_PACKET, FBE_FALSE);

    return status;
}
/******************************************************************************
 * end fbe_provision_drive_metadata_paged_init_client_blob()
 ******************************************************************************/

/*!****************************************************************************
 * @fn fbe_provision_drive_metadata_paged_callback_get_info()
 ******************************************************************************
 * @brief
 *
 *  This is the function to get information for paged metadata callback functions. 
 * 
 * @param packet               - Pointer to packet.
 * @param lba_offset           - Out lba offset.
 * @param slot_offset          - Out slot offset.
 * @param sg_ptr               - Out pointer to sg list.
 *
 * @return fbe_status_t.
 *  05/06/2014 - Created. Lili Chen
 *
 ******************************************************************************/
fbe_status_t 
fbe_provision_drive_metadata_paged_callback_get_info(fbe_packet_t *packet, 
                                                     fbe_payload_metadata_operation_t ** out_mdo,
                                                     fbe_lba_t * lba_offset,
                                                     fbe_u64_t * slot_offset,
                                                     fbe_sg_element_t ** sg_ptr)
{
    fbe_payload_ex_t * sep_payload = fbe_transport_get_payload_ex(packet);
    fbe_payload_metadata_operation_t * mdo = NULL;
    fbe_sg_element_t * sg_list = NULL;

    if(sep_payload->current_operation->payload_opcode == FBE_PAYLOAD_OPCODE_METADATA_OPERATION){
        mdo = fbe_payload_ex_get_metadata_operation(sep_payload);
    } else if(sep_payload->current_operation->payload_opcode == FBE_PAYLOAD_OPCODE_STRIPE_LOCK_OPERATION){
        mdo = fbe_payload_ex_get_any_metadata_operation(sep_payload);
    } else {
        fbe_topology_class_trace(FBE_CLASS_ID_PROVISION_DRIVE, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s: Invalid payload opcode:%d \n", __FUNCTION__, sep_payload->current_operation->payload_opcode);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    *out_mdo = mdo;
    *lba_offset = fbe_metadata_paged_get_lba_offset(mdo);
    *slot_offset = fbe_metadata_paged_get_slot_offset(mdo);
    sg_list = mdo->u.metadata_callback.sg_list;
    *sg_ptr = &sg_list[*lba_offset - mdo->u.metadata_callback.start_lba];

    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_provision_drive_metadata_paged_callback_get_info()
 ******************************************************************************/

/*!****************************************************************************
 * @fn fbe_provision_drive_metadata_paged_callback_read()
 ******************************************************************************
 * @brief
 *
 *  This is the function to set bits for paged metadata callback functions. 
 * 
 * @param packet               - Pointer to packet.
 * @param context              - Pointer to context.
 *
 * @return fbe_status_t.
 *  05/06/2014 - Created. Lili Chen
 *
 ******************************************************************************/
fbe_status_t 
fbe_provision_drive_metadata_paged_callback_read(fbe_packet_t * packet, fbe_metadata_callback_context_t context)
{
    fbe_payload_metadata_operation_t * mdo = NULL;
    fbe_sg_element_t * sg_ptr = NULL;
    fbe_lba_t lba_offset;
    fbe_u64_t slot_offset;
    fbe_u32_t dst_size;
    fbe_u8_t * dst_ptr;
    fbe_u8_t * data_ptr;

    if (fbe_provision_drive_metadata_paged_callback_get_info(packet, &mdo, &lba_offset, &slot_offset, &sg_ptr) != FBE_STATUS_OK){
        return FBE_STATUS_GENERIC_FAILURE;
    }

    data_ptr = sg_ptr->address + slot_offset;
    dst_size = (fbe_u32_t)(mdo->u.metadata_callback.record_data_size * mdo->u.metadata_callback.repeat_count);
    dst_ptr = mdo->u.metadata_callback.record_data;

    if(slot_offset + dst_size <= FBE_METADATA_BLOCK_DATA_SIZE){
        fbe_copy_memory(dst_ptr, data_ptr, dst_size);
    } else {
        fbe_copy_memory(dst_ptr, data_ptr, FBE_METADATA_BLOCK_DATA_SIZE - (fbe_u32_t)slot_offset);
        dst_ptr += FBE_METADATA_BLOCK_DATA_SIZE - slot_offset;
        sg_ptr++;
        data_ptr = sg_ptr->address;
        fbe_copy_memory(dst_ptr, data_ptr, dst_size - (FBE_METADATA_BLOCK_DATA_SIZE - (fbe_u32_t)slot_offset));
    }

    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_provision_drive_metadata_paged_callback_read()
 ******************************************************************************/

/*!****************************************************************************
 * @fn fbe_provision_drive_metadata_paged_callback_set_bits()
 ******************************************************************************
 * @brief
 *
 *  This is the function to set bits for paged metadata callback functions. 
 * 
 * @param packet               - Pointer to packet.
 * @param set_bits             - Pointer to paged metadata.
 *
 * @return fbe_status_t.
 *  05/06/2014 - Created. Lili Chen
 *
 ******************************************************************************/
fbe_status_t 
fbe_provision_drive_metadata_paged_callback_set_bits(fbe_packet_t * packet, 
                                                     fbe_provision_drive_paged_metadata_t * set_bits)
{
    fbe_payload_metadata_operation_t * mdo = NULL;
    fbe_provision_drive_paged_metadata_t * paged_metadata_p;
    fbe_sg_element_t * sg_ptr = NULL;
    fbe_lba_t lba_offset;
    fbe_u64_t slot_offset;
    fbe_bool_t is_write_requiered = FBE_FALSE;
    fbe_u8_t * paged_set_bits = (fbe_u8_t *)set_bits;

    if (fbe_provision_drive_metadata_paged_callback_get_info(packet, &mdo, &lba_offset, &slot_offset, &sg_ptr) != FBE_STATUS_OK){
        return FBE_STATUS_GENERIC_FAILURE;
    }

    paged_metadata_p = (fbe_provision_drive_paged_metadata_t *)(sg_ptr->address + slot_offset);
    while ((sg_ptr->address != NULL) && (mdo->u.metadata_callback.current_count_private < mdo->u.metadata_callback.repeat_count))
    {
        if ((((fbe_u8_t *)paged_metadata_p)[0] & paged_set_bits[0]) != paged_set_bits[0]) {
            is_write_requiered = FBE_TRUE;
            ((fbe_u8_t *)paged_metadata_p)[0] |= paged_set_bits[0];
        }

        mdo->u.metadata_callback.current_count_private++;
        paged_metadata_p++;
        if (paged_metadata_p == (fbe_provision_drive_paged_metadata_t *)(sg_ptr->address + FBE_METADATA_BLOCK_DATA_SIZE))
        {
            sg_ptr++;
            paged_metadata_p = (fbe_provision_drive_paged_metadata_t *)sg_ptr->address;
        }
    }

    if (is_write_requiered) {
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    } else {
        return FBE_STATUS_OK;
    }
}
/******************************************************************************
 * end fbe_provision_drive_metadata_paged_callback_set_bits()
 ******************************************************************************/

/*!****************************************************************************
 * @fn fbe_provision_drive_metadata_paged_callback_clear_bits()
 ******************************************************************************
 * @brief
 *
 *  This is the function to clear bits for paged metadata callback functions. 
 * 
 * @param packet               - Pointer to packet.
 * @param set_bits             - Pointer to paged metadata.
 *
 * @return fbe_status_t.
 *  05/06/2014 - Created. Lili Chen
 *
 ******************************************************************************/
fbe_status_t 
fbe_provision_drive_metadata_paged_callback_clear_bits(fbe_packet_t * packet, 
                                                       fbe_provision_drive_paged_metadata_t * clear_bits)
{
    fbe_payload_metadata_operation_t * mdo = NULL;
    fbe_provision_drive_paged_metadata_t * paged_metadata_p;
    fbe_sg_element_t * sg_ptr = NULL;
    fbe_lba_t lba_offset;
    fbe_u64_t slot_offset;
    fbe_bool_t is_write_requiered = FBE_FALSE;
    fbe_u8_t * paged_clear_bits = (fbe_u8_t *)clear_bits;

    if (fbe_provision_drive_metadata_paged_callback_get_info(packet, &mdo, &lba_offset, &slot_offset, &sg_ptr) != FBE_STATUS_OK){
        return FBE_STATUS_GENERIC_FAILURE;
    }

    paged_metadata_p = (fbe_provision_drive_paged_metadata_t *)(sg_ptr->address + slot_offset);
    while ((sg_ptr->address != NULL) && (mdo->u.metadata_callback.current_count_private < mdo->u.metadata_callback.repeat_count))
    {
        if ((((fbe_u8_t *)paged_metadata_p)[0] & paged_clear_bits[0]) != 0) {
            is_write_requiered = FBE_TRUE;
            ((fbe_u8_t *)paged_metadata_p)[0] &= ~paged_clear_bits[0];
        }

        mdo->u.metadata_callback.current_count_private++;
        paged_metadata_p++;
        if (paged_metadata_p == (fbe_provision_drive_paged_metadata_t *)(sg_ptr->address + FBE_METADATA_BLOCK_DATA_SIZE))
        {
            sg_ptr++;
            paged_metadata_p = (fbe_provision_drive_paged_metadata_t *)sg_ptr->address;
        }
    }

    if (is_write_requiered) {
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    } else {
        return FBE_STATUS_OK;
    }
}
/******************************************************************************
 * end fbe_provision_drive_metadata_paged_callback_clear_bits()
 ******************************************************************************/

/*!****************************************************************************
 * @fn fbe_provision_drive_metadata_paged_callback_write()
 ******************************************************************************
 * @brief
 *
 *  This is the function to write/write_verify for paged metadata callback functions. 
 * 
 * @param packet               - Pointer to packet.
 * @param set_bits             - Pointer to paged metadata.
 *
 * @return fbe_status_t.
 *  05/06/2014 - Created. Lili Chen
 *
 ******************************************************************************/
fbe_status_t 
fbe_provision_drive_metadata_paged_callback_write(fbe_packet_t * packet, 
                                                  fbe_provision_drive_paged_metadata_t * set_bits)
{
    fbe_payload_metadata_operation_t * mdo = NULL;
    fbe_provision_drive_paged_metadata_t * paged_metadata_p;
    fbe_sg_element_t * sg_ptr = NULL;
    fbe_lba_t lba_offset;
    fbe_u64_t slot_offset;

    if (fbe_provision_drive_metadata_paged_callback_get_info(packet, &mdo, &lba_offset, &slot_offset, &sg_ptr) != FBE_STATUS_OK){
        return FBE_STATUS_GENERIC_FAILURE;
    }

    paged_metadata_p = (fbe_provision_drive_paged_metadata_t *)(sg_ptr->address + slot_offset);
    while ((sg_ptr->address != NULL) && (mdo->u.metadata_callback.current_count_private < mdo->u.metadata_callback.repeat_count))
    {
        fbe_copy_memory(paged_metadata_p, set_bits, sizeof(fbe_provision_drive_paged_metadata_t));

        mdo->u.metadata_callback.current_count_private++;
        paged_metadata_p++;
        if (paged_metadata_p == (fbe_provision_drive_paged_metadata_t *)(sg_ptr->address + FBE_METADATA_BLOCK_DATA_SIZE))
        {
            sg_ptr++;
            paged_metadata_p = (fbe_provision_drive_paged_metadata_t *)sg_ptr->address;
        }
    }

    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/******************************************************************************
 * end fbe_provision_drive_metadata_paged_callback_write()
 ******************************************************************************/


/*!****************************************************************************
 *          fbe_provision_drive_set_swap_pending()
 ****************************************************************************** 
 * 
 * @brief   Use the drive offline `reason' to determine which provision drive
 *          offline flag to set and then set it.
 *
 * @param   packet_p     - Packet requesting operation.
 * @param   context      - Packet completion context (provision drive object).
 *
 * @return  status - The status of the operation.
 *
 * @author
 *  06/05/2014  Ron Proulx  - Created.
 *
 ******************************************************************************/
fbe_status_t fbe_provision_drive_set_swap_pending(fbe_packet_t *packet_p,
                                                        fbe_packet_completion_context_t context)
{
    fbe_status_t                            status;
    fbe_provision_drive_t                  *provision_drive_p = NULL;
    fbe_provision_drive_np_flags_t          np_flags_to_set = 0;
    fbe_base_config_encryption_mode_t       encryption_mode;
    fbe_provision_drive_np_flags_t          existing_np_flags;
    fbe_provision_drive_set_swap_pending_t *set_swap_pending_p = NULL;

    /* Get the provision drive object and the encryption mode for debug.
     */
    provision_drive_p = (fbe_provision_drive_t *)context;
    fbe_base_config_get_encryption_mode((fbe_base_config_t *)provision_drive_p, &encryption_mode);

    /* If status is not good then we failed to get the NP lock.
     * Return the failure to call completion.
     */
    status = fbe_transport_get_status_code(packet_p);
    if (status != FBE_STATUS_OK){
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,    
                               "%s: could not get np lock stat:0x%x, \n",
                               __FUNCTION__, status);
        fbe_transport_set_status(packet_p, status, 0);
        return status;
    }

    /* We own the NP. We must always release it.
     */
    fbe_transport_set_completion_function(packet_p, fbe_provision_drive_release_NP_lock, provision_drive_p);

    /* Get the buffer from the usurper to determine which NP flag to set.
     */
    status = fbe_provision_drive_get_control_buffer(provision_drive_p, packet_p,
                                                    sizeof(fbe_provision_drive_set_swap_pending_t),
                                                    (fbe_payload_control_buffer_t)&set_swap_pending_p);
    if (status != FBE_STATUS_OK) {
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s fbe_payload_control_get_buffer failed\n",
                                __FUNCTION__);
        fbe_transport_set_status( packet_p, status, 0 );
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }

    /* Validate the reason.
     */
    if ((set_swap_pending_p->set_swap_pending_reason <= FBE_PROVISION_DRIVE_DRIVE_SWAP_PENDING_REASON_NONE) ||
        (set_swap_pending_p->set_swap_pending_reason >= FBE_PROVISION_DRIVE_DRIVE_SWAP_PENDING_REASON_LAST)    ) {
        /* Unsupported reason.
         */
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s mark offline reason: %d not supported\n",
                                __FUNCTION__, set_swap_pending_p->set_swap_pending_reason);
        fbe_transport_set_status( packet_p, FBE_STATUS_GENERIC_FAILURE, 0 );
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }
    switch (set_swap_pending_p->set_swap_pending_reason) {
        case FBE_PROVISION_DRIVE_DRIVE_SWAP_PENDING_REASON_PROACTIVE_COPY:
            np_flags_to_set = FBE_PROVISION_DRIVE_NP_FLAG_SWAP_PENDING_PROACTIVE_COPY;
            break;
        case FBE_PROVISION_DRIVE_DRIVE_SWAP_PENDING_REASON_USER_COPY:
            np_flags_to_set = FBE_PROVISION_DRIVE_NP_FLAG_SWAP_PENDING_USER_COPY;
            break;
        default:
            fbe_base_object_trace((fbe_base_object_t *)provision_drive_p,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s swap pending reason: %d not supported\n",
                                __FUNCTION__, set_swap_pending_p->set_swap_pending_reason);
            fbe_transport_set_status( packet_p, FBE_STATUS_GENERIC_FAILURE, 0 );
            fbe_transport_complete_packet(packet_p);
            return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }

    /* Get the existing NP flags.
     */
    status = fbe_provision_drive_metadata_get_np_flag(provision_drive_p, &existing_np_flags);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,    
                               "%s: could not get np lock stat:0x%x, \n",
                               __FUNCTION__, status);
        fbe_transport_set_status( packet_p, status, 0 );
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }

    /* Trace some information.
     */
    np_flags_to_set ^= existing_np_flags;
    fbe_base_object_trace((fbe_base_object_t *)provision_drive_p,
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,    
                          "%s: swap pending flag: 0x%x current NP flags: 0x%x encryption mode: 0x%x\n",
                          __FUNCTION__, np_flags_to_set, existing_np_flags, encryption_mode);

    /* If the flags is already set there is no need to change them.
     */
    if (np_flags_to_set == 0) {
        fbe_transport_complete_packet(packet_p);
    } else {
        /* Set the `swap pending' flags in the non-paged.
         */
        existing_np_flags |= np_flags_to_set;
        status = fbe_base_config_metadata_nonpaged_write_persist((fbe_base_config_t *) provision_drive_p,
                                                                 packet_p,
                                                                 (fbe_u64_t)(&((fbe_provision_drive_nonpaged_metadata_t*)0)->flags),
                                                                 (fbe_u8_t *)&existing_np_flags,
                                                                 sizeof(existing_np_flags));
    }
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/********************************************************* 
* end fbe_provision_drive_set_swap_pending()
**********************************************************/

/*!****************************************************************************
 *          fbe_provision_drive_clear_swap_pending()
 ****************************************************************************** 
 * 
 * @brief   Clear the `mark drive offline' NP lfag.
 *
 * @param   packet_p     - Packet requesting operation.
 * @param   context      - Packet completion context (provision drive object).
 *
 * @return  status - The status of the operation.
 *
 * @author
 *  06/05/2014  Ron Proulx  - Created.
 *
 ******************************************************************************/
fbe_status_t fbe_provision_drive_clear_swap_pending(fbe_packet_t *packet_p,
                                                          fbe_packet_completion_context_t context)
{
    fbe_status_t                            status;
    fbe_provision_drive_t                  *provision_drive_p = NULL;
    fbe_provision_drive_np_flags_t          np_flags_to_clear = FBE_PROVISION_DRIVE_NP_FLAG_SWAP_PENDING_BITS;
    fbe_base_config_encryption_mode_t       encryption_mode;
    fbe_provision_drive_np_flags_t          existing_np_flags;

    /* Get the provision drive object and the encryption mode for debug.
     */
    provision_drive_p = (fbe_provision_drive_t *)context;
    fbe_base_config_get_encryption_mode((fbe_base_config_t *)provision_drive_p, &encryption_mode);

    /* If status is not good then we failed to get the NP lock.
     * Return the failure to call completion.
     */
    status = fbe_transport_get_status_code(packet_p);
    if (status != FBE_STATUS_OK){
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,    
                               "%s: could not get np lock stat:0x%x, \n",
                               __FUNCTION__, status);
        fbe_transport_set_status(packet_p, status, 0);
        return status;
    }

    /* We own the NP. We must always release it.
     */
    fbe_transport_set_completion_function(packet_p, fbe_provision_drive_release_NP_lock, provision_drive_p);

    /* Get the existing NP flags.
     */
    status = fbe_provision_drive_metadata_get_np_flag(provision_drive_p, &existing_np_flags);
    if (status != FBE_STATUS_OK) {
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,    
                               "%s: could not get np flags stat:0x%x, \n",
                               __FUNCTION__, status);
        fbe_transport_set_status( packet_p, status, 0 );
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }

    /* Trace some information.
     */
    np_flags_to_clear &= existing_np_flags;
    fbe_base_object_trace((fbe_base_object_t *)provision_drive_p,
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,    
                          "%s: swap pending flag: 0x%x current NP flags: 0x%x encryption mode: 0x%x\n",
                          __FUNCTION__, np_flags_to_clear, existing_np_flags, encryption_mode);

    /* We own the NP lock.  If the bits are not set there is no need to clear them.
     * Complete the packet and return pending.
     */
    if (np_flags_to_clear == 0) {
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    } else {
        /* Else clear the `mark offline' NP flags.
         */
        existing_np_flags &= ~np_flags_to_clear;
        status = fbe_base_config_metadata_nonpaged_write_persist((fbe_base_config_t *) provision_drive_p,
                                                                 packet_p,
                                                                 (fbe_u64_t)(&((fbe_provision_drive_nonpaged_metadata_t*)0)->flags),
                                                                 (fbe_u8_t *)&existing_np_flags,
                                                                 sizeof(existing_np_flags));
    }
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/********************************************************* 
* end fbe_provision_drive_clear_swap_pending()
**********************************************************/


/*!****************************************************************************
 *          fbe_provision_drive_set_eas_start()
 ****************************************************************************** 
 * 
 * @brief   This function sets EAS started NP flag for the provision drive.
 *          It also sets NZ NP flag.
 *
 * @param   packet_p     - Packet requesting operation.
 * @param   context      - Packet completion context (provision drive object).
 *
 * @return  status - The status of the operation.
 *
 * @author
 *  07/22/2014  Lili Chen  - Created.
 *
 ******************************************************************************/
fbe_status_t fbe_provision_drive_set_eas_start(fbe_packet_t *packet_p,
                                               fbe_packet_completion_context_t context)
{
    fbe_status_t                            status;
    fbe_provision_drive_t                  *provision_drive_p = NULL;
    fbe_provision_drive_np_flags_t          np_flags = 0;

    /* Get the provision drive object and the encryption mode for debug.
     */
    provision_drive_p = (fbe_provision_drive_t *)context;

    /* If status is not good then we failed to get the NP lock.
     * Return the failure to call completion.
     */
    status = fbe_transport_get_status_code(packet_p);
    if (status != FBE_STATUS_OK){
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,    
                               "%s: could not get np lock stat:0x%x, \n",
                               __FUNCTION__, status);
        fbe_transport_set_status(packet_p, status, 0);
        return status;
    }

    /* We own the NP. We must always release it.
     */
    fbe_transport_set_completion_function(packet_p, fbe_provision_drive_release_NP_lock, provision_drive_p);

    /* Get the existing NP flags.
     */
    status = fbe_provision_drive_metadata_get_np_flag(provision_drive_p, &np_flags);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,    
                               "%s: could not get np flag stat:0x%x, \n",
                               __FUNCTION__, status);
        fbe_transport_set_status( packet_p, status, 0 );
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }

    np_flags &= ~FBE_PROVISION_DRIVE_NP_FLAG_EAS_COMPLETE;
    np_flags |= FBE_PROVISION_DRIVE_NP_FLAG_EAS_STARTED;

    /* Trace some information.
     */
    fbe_base_object_trace((fbe_base_object_t *)provision_drive_p,
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,    
                          "%s: NP flags: 0x%x \n",
                          __FUNCTION__, np_flags);

    status = fbe_base_config_metadata_nonpaged_write_persist((fbe_base_config_t *) provision_drive_p,
                                                             packet_p,
                                                             (fbe_u64_t)(&((fbe_provision_drive_nonpaged_metadata_t*)0)->flags),
                                                             (fbe_u8_t *)&np_flags,
                                                             sizeof(fbe_provision_drive_np_flags_t));
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/********************************************************* 
* end fbe_provision_drive_set_eas_start()
**********************************************************/

/*!****************************************************************************
 *          fbe_provision_drive_set_eas_complete()
 ****************************************************************************** 
 * 
 * @brief   This function sets EAS complete NP flag for the provision drive.
 *          It also sets NZ NP flag.
 *
 * @param   packet_p     - Packet requesting operation.
 * @param   context      - Packet completion context (provision drive object).
 *
 * @return  status - The status of the operation.
 *
 * @author
 *  07/22/2014  Lili Chen  - Created.
 *
 ******************************************************************************/
fbe_status_t fbe_provision_drive_set_eas_complete(fbe_packet_t *packet_p,
                                               fbe_packet_completion_context_t context)
{
    fbe_status_t                            status;
    fbe_provision_drive_t                  *provision_drive_p = NULL;
    fbe_provision_drive_np_flags_t          np_flags = 0;

    /* Get the provision drive object and the encryption mode for debug.
     */
    provision_drive_p = (fbe_provision_drive_t *)context;

    /* If status is not good then we failed to get the NP lock.
     * Return the failure to call completion.
     */
    status = fbe_transport_get_status_code(packet_p);
    if (status != FBE_STATUS_OK){
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,    
                               "%s: could not get np lock stat:0x%x, \n",
                               __FUNCTION__, status);
        fbe_transport_set_status(packet_p, status, 0);
        return status;
    }

    /* We own the NP. We must always release it.
     */
    fbe_transport_set_completion_function(packet_p, fbe_provision_drive_release_NP_lock, provision_drive_p);

    /* Get the existing NP flags.
     */
    status = fbe_provision_drive_metadata_get_np_flag(provision_drive_p, &np_flags);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,    
                               "%s: could not get np flag stat:0x%x, \n",
                               __FUNCTION__, status);
        fbe_transport_set_status( packet_p, status, 0 );
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }

    np_flags &= ~FBE_PROVISION_DRIVE_NP_FLAG_EAS_STARTED;
    np_flags |= FBE_PROVISION_DRIVE_NP_FLAG_EAS_COMPLETE;
    /* Set NZ flag so that when it goes out of fail, paged MD will be initialized */
    np_flags |= FBE_PROVISION_DRIVE_NP_FLAG_PAGED_NEEDS_ZERO;
    np_flags &= ~FBE_PROVISION_DRIVE_NP_FLAG_PAGED_VALID;

    /* Trace some information.
     */
    fbe_base_object_trace((fbe_base_object_t *)provision_drive_p,
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,    
                          "%s: NP flags: 0x%x \n",
                          __FUNCTION__, np_flags);

    status = fbe_base_config_metadata_nonpaged_write_persist((fbe_base_config_t *) provision_drive_p,
                                                             packet_p,
                                                             (fbe_u64_t)(&((fbe_provision_drive_nonpaged_metadata_t*)0)->flags),
                                                             (fbe_u8_t *)&np_flags,
                                                             sizeof(fbe_provision_drive_np_flags_t));
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/********************************************************* 
* end fbe_provision_drive_set_eas_complete()
**********************************************************/




/*!****************************************************************************
 * fbe_provision_drive_metadata_write_default_paged_for_ext_pool()
 ******************************************************************************
 * @brief
 *   This function is used to initialize the metadata with default values for 
 *   the non paged and paged metadata. Before we initialize it we have to
 *   zero out this area.
 *
 * @param provision_drive_p         - pointer to the provision drive
 * @param packet_p                  - pointer to a monitor packet.
 *
 * @return  fbe_status_t  
 *
 * @author
 *   06/09/2014 - Created. Lili Chen
 *
 ******************************************************************************/
fbe_status_t 
fbe_provision_drive_metadata_write_default_paged_for_ext_pool(fbe_provision_drive_t * provision_drive_p,
                                                              fbe_packet_t *packet_p)
{
    fbe_status_t                                    status = FBE_STATUS_GENERIC_FAILURE; 
    fbe_payload_ex_t *                              sep_payload_p = NULL;
    fbe_payload_control_operation_t *               control_operation_p = NULL;
    fbe_memory_request_t *                          memory_request_p = NULL;

    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(sep_payload_p);

    /* allocate memory for client_blob */
    memory_request_p = fbe_transport_get_memory_request(packet_p);
    fbe_memory_build_chunk_request(memory_request_p,
                                   FBE_MEMORY_CHUNK_SIZE_FOR_64_BLOCKS_IO,
                                   2, /* number of chunks */
                                   fbe_transport_get_resource_priority(packet_p),
                                   fbe_transport_get_io_stamp(packet_p),
                                   (fbe_memory_completion_function_t)fbe_provision_drive_metadata_write_default_paged_for_ext_pool_alloc_completion,
                                   provision_drive_p);
    fbe_transport_memory_request_set_io_master(memory_request_p, packet_p);

    /* Issue the memory request to memory service. */
    status = fbe_memory_request_entry(memory_request_p);
    if((status != FBE_STATUS_OK) && (status != FBE_STATUS_PENDING))
    {
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s memory allocation failed",  __FUNCTION__);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return status;
    }

    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/******************************************************************************
 * end fbe_provision_drive_metadata_write_default_paged_for_ext_pool()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_provision_drive_metadata_write_default_paged_for_ext_pool_alloc_completion()
 ******************************************************************************
 * @brief
 *  This is the completion for memory allocation for paged blob.
 *
 * @param memory_request_p         - pointer to the memory request
 * @param context                  - context.
 *
 * @return  fbe_status_t  
 *
 * @author
 *   06/09/2014 - Created. Lili Chen
 *
 ******************************************************************************/
static fbe_status_t
fbe_provision_drive_metadata_write_default_paged_for_ext_pool_alloc_completion(fbe_memory_request_t * memory_request_p, 
                                                                               fbe_memory_completion_context_t context)
{
    fbe_status_t                                    status = FBE_STATUS_GENERIC_FAILURE; 
    fbe_u64_t                                       repeat_count = 0;
    fbe_provision_drive_paged_metadata_for_pool_t   paged_data;
    fbe_payload_ex_t *                              sep_payload_p = NULL;
    fbe_payload_metadata_operation_t *              metadata_operation_p = NULL;
    fbe_payload_control_operation_t *               control_operation_p = NULL;
    fbe_lba_t                                       paged_metadata_capacity = 0;
    fbe_packet_t *                                  packet_p;
    fbe_provision_drive_t *                         provision_drive_p = (fbe_provision_drive_t *) context;

    /* get the payload and control operation. */
    packet_p = fbe_transport_memory_request_to_packet(memory_request_p);
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(sep_payload_p);

    /* Check allocation status */
    if (fbe_memory_request_is_allocation_complete(memory_request_p) == FBE_FALSE)
    {
        /* Currently this callback doesn't expect any aborted requests */
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p, 
                                        FBE_TRACE_LEVEL_ERROR,
                                        FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                        "%s memory allocation failed, state:%d\n",
                                        __FUNCTION__, memory_request_p->request_state);

        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*let's Initialize the paged bith with default values. */
    fbe_zero_memory(&paged_data, sizeof(fbe_provision_drive_paged_metadata_for_pool_t));

    /* Set the repeat count as number of records in paged metadata capacity.
     * This is the number of chunks of user data + paddings for alignment.
     */
    fbe_base_config_metadata_get_paged_metadata_capacity((fbe_base_config_t *)provision_drive_p,
                                                       &paged_metadata_capacity);
    repeat_count = (paged_metadata_capacity*FBE_METADATA_BLOCK_DATA_SIZE)/sizeof(fbe_provision_drive_paged_metadata_for_pool_t);

    /* Allocate the metadata operation. */
    metadata_operation_p =  fbe_payload_ex_allocate_metadata_operation(sep_payload_p);

    /* Build the paged metadata write request. */
    status = fbe_payload_metadata_build_paged_write(metadata_operation_p, 
                                                    &(((fbe_base_config_t *) provision_drive_p)->metadata_element),
                                                    0, /* Paged metadata offset is zero for the default paged metadata write. 
                                                          paint the whole paged metadata with the default value */
                                                    (fbe_u8_t *) &paged_data,
                                                    sizeof(fbe_provision_drive_paged_metadata_for_pool_t),
                                                    repeat_count);
    if(status != FBE_STATUS_OK)
    {
        fbe_memory_free_request_entry(memory_request_p);
        fbe_payload_ex_release_metadata_operation(sep_payload_p, metadata_operation_p);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, status, 0);
        return status;
    }

    /* Initialize cient_blob */
    status = fbe_metadata_paged_init_client_blob(memory_request_p, metadata_operation_p, 0, FBE_FALSE);

    /* set the completion function for the default paged metadata write. */
    fbe_transport_set_completion_function(packet_p, fbe_provision_drive_metadata_write_default_paged_for_ext_pool_completion, provision_drive_p);

    fbe_payload_ex_increment_metadata_operation_level(sep_payload_p);

    /* Issue the metadata operation. */
    fbe_metadata_operation_entry(packet_p);
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;

    
}
/******************************************************************************
 * end fbe_provision_drive_metadata_write_default_paged_for_ext_pool_alloc_completion()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_provision_drive_metadata_write_default_paged_for_ext_pool_completion()
 ******************************************************************************
 * @brief
 *  This function is used to handle the comletion of the default paged metadata
 *  initialization.
 *
 * @param provision_drive_p         - pointer to the provision drive
 * @param packet_p                  - pointer to a monitor packet.
 *
 * @return  fbe_status_t  
 *
 * @author
 *   06/09/2014 - Created. Lili Chen
 *
 ******************************************************************************/
static fbe_status_t
fbe_provision_drive_metadata_write_default_paged_for_ext_pool_completion(fbe_packet_t * packet_p,
                                                                         fbe_packet_completion_context_t context)
{
    fbe_provision_drive_t *                 provision_drive_p = NULL;
    fbe_payload_metadata_operation_t *      metadata_operation_p = NULL;
    fbe_payload_control_operation_t *       control_operation_p = NULL;
    fbe_payload_ex_t *                      sep_payload_p = NULL;
    fbe_status_t                            status;
    fbe_payload_metadata_status_t           metadata_status;
    fbe_memory_request_t *                  memory_request = NULL;
    
    provision_drive_p = (fbe_provision_drive_t *) context;

    /* get the payload and metadata operation. */
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    metadata_operation_p =  fbe_payload_ex_get_metadata_operation(sep_payload_p);
    control_operation_p = fbe_payload_ex_get_prev_control_operation(sep_payload_p);
    
    /* get the status of the paged metadata operation. */
    fbe_payload_metadata_get_status(metadata_operation_p, &metadata_status);
    status = fbe_transport_get_status_code(packet_p);

    /* release the metadata operation. */
    fbe_payload_ex_release_metadata_operation(sep_payload_p, metadata_operation_p);

    /* release client_blob */
    memory_request = fbe_transport_get_memory_request(packet_p);
    fbe_memory_free_request_entry(memory_request);

    if((status != FBE_STATUS_OK) ||
       (metadata_status != FBE_PAYLOAD_METADATA_STATUS_OK))
    {

        fbe_base_object_trace((fbe_base_object_t*)provision_drive_p,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s write default paged failed, status:0x%x, metadata_status:0x%x.\n",
                              __FUNCTION__, status, metadata_status);
        status = (status != FBE_STATUS_OK) ? status : FBE_STATUS_GENERIC_FAILURE;
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, status, 0);
    }
    else
    {
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_OK);
    }

    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_provision_drive_metadata_write_default_paged_for_ext_pool_completion()
 ******************************************************************************/

/*!***************************************************************************
 *          fbe_provision_drive_write_default_paged_for_ext_pool_clear_np_flag()
 *****************************************************************************
 *
 * @brief 
 *  This is the completion for writing default paged metadata.
 *
 * @param packet_p     - Packet requesting operation.
 * @param context      - Packet completion context (provision drive object).
 *
 * @return status - The status of the operation.
 *
 * @author
 *   06/09/2014 - Created. Lili Chen
 *
 ******************************************************************************/
fbe_status_t 
fbe_provision_drive_metadata_write_default_paged_for_ext_pool_clear_np_flag(fbe_packet_t * packet_p,
                                                                            fbe_packet_completion_context_t context)
{
    fbe_provision_drive_t *             provision_drive_p = NULL;
    fbe_payload_ex_t *                     payload = NULL;
    fbe_payload_control_operation_t *   control_operation = NULL;
    fbe_payload_control_status_t        control_status;
    fbe_status_t                        status = FBE_STATUS_GENERIC_FAILURE;

    provision_drive_p = (fbe_provision_drive_t*)context;

    /* get the packet status. */
    status = fbe_transport_get_status_code (packet_p);

    /* get control status */
    payload = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload);
    fbe_payload_control_get_status(control_operation, &control_status);

    /* verify the status and take appropriate action. */
    if((control_status == FBE_PAYLOAD_CONTROL_STATUS_OK) && (status == FBE_STATUS_OK))
    {
        /* Clear need zero if status is good. */
        fbe_provision_drive_get_NP_lock(provision_drive_p, packet_p, fbe_provision_drive_metadata_clear_np_flag_need_zero_paged);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }

    return FBE_STATUS_OK;
}
/*******************************************************************
 * end fbe_provision_drive_write_default_paged_for_ext_pool_clear_np_flag()
 *******************************************************************/


/*************************************
 * end fbe_provision_drive_metadata.c
 ************************************/

