 /***************************************************************************
  * Copyright (C) EMC Corporation 2008
  * All rights reserved.
  * Licensed material -- property of EMC Corporation
  ***************************************************************************/
 
 /*!**************************************************************************
  * @file fbe_virtual_drive_executor.c
  ***************************************************************************
  *
  * @brief
  *  This file contains the executer functions for the virtual_drive.
  * 
  *  This includes the fbe_virtual_drive_io_entry() function which is our entry
  *  point for functional packets.
  * 
  * @ingroup virtual_drive_class_files
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
#include "fbe/fbe_virtual_drive.h"
#include "fbe_virtual_drive_private.h"
#include "fbe/fbe_winddk.h"
#include "fbe_traffic_trace.h"
#include "fbe_service_manager.h"
#include "fbe_transport_memory.h"
#include "fbe_raid_library.h"

 /*************************
  *   FUNCTION DEFINITIONS
  *************************/

static fbe_status_t fbe_virtual_drive_block_transport_entry_allocate_packet(fbe_virtual_drive_t * virtual_drive_p,
                                                                            fbe_packet_t * packet_p);

static fbe_status_t fbe_virtual_drive_block_transport_entry_allocation_completion(fbe_memory_request_t * memory_request, 
                                                                                  fbe_memory_completion_context_t context);

static fbe_status_t fbe_virtual_drive_block_transport_entry_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);


/*!****************************************************************************
 *          fbe_virtual_drive_io_entry()
 ******************************************************************************
 * 
 *  @brief  This function is the entry point for functional transport 
 *          operations (including metadata operations) to the virtual_drive 
 *          object.  There are (3) possible execution paths:
 *
 * @param   object_handle   - The virtual_drive handle.
 * @param   packet_p        - The io packet that is arriving.
 *
 * @return
 *  fbe_status_t - Return status of the operation.
 *
 * @author
 *  05/22/09 - Created. Rob Foley.
 *
 ******************************************************************************/
fbe_status_t 
fbe_virtual_drive_io_entry(fbe_object_handle_t object_handle, fbe_packet_t *packet_p)
{
    fbe_status_t status;
    fbe_virtual_drive_t * virtual_drive_p = NULL;

    virtual_drive_p = (fbe_virtual_drive_t *)fbe_base_handle_to_pointer(object_handle);
    status = fbe_base_config_bouncer_entry((fbe_base_config_t * ) virtual_drive_p, packet_p);
    return status;
}
/******************************************************************************
 * end fbe_virtual_drive_io_entry()
 ******************************************************************************/

/*!****************************************************************************
 * @fn fbe_virtual_drive_block_transport_entry
 ******************************************************************************
 * @brief
 *  This is the entry function that the block transport will call to pass a 
 *  packet to the virtual drive object.
 *  
 * @param context       - Pointer to the provision drive object. 
 * @param packet_p      - The packet to process.
 * 
 * @note
 *  Memory request allocation for new packet is added temporary until packet
 *  size allows us to allocate more payload in I/O stack, once packet size 
 *  allows us to have more payloads then we will directly forward request
 *  down without allocating new packet.
 *
 * @return fbe_status_t
 *
 * @author
 *  4/28/2010 - Created. Dhaval Patel
 *
 ******************************************************************************/
fbe_status_t
fbe_virtual_drive_block_transport_entry(fbe_transport_entry_context_t context, fbe_packet_t * packet_p)
{
    fbe_virtual_drive_t *   virtual_drive_p = NULL;
    fbe_status_t            status;
    fbe_virtual_drive_configuration_mode_t  configuration_mode = FBE_VIRTUAL_DRIVE_CONFIG_MODE_UNKNOWN;
    fbe_block_edge_t * edge = NULL;
    fbe_payload_ex_t *                     sep_payload_p;
    fbe_payload_block_operation_t *        block_operation_p;

    /* Get the payload and block operation. */
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    block_operation_p = fbe_payload_ex_get_block_operation(sep_payload_p);

    virtual_drive_p = (fbe_virtual_drive_t *)context;

    fbe_base_object_trace((fbe_base_object_t*)virtual_drive_p, 
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s: Entry\n", __FUNCTION__);

#if 0
    /* trace to RBA buffer */
    if (fbe_traffic_trace_is_enabled (KTRC_TVD))
    {
        fbe_traffic_trace_block_operation(FBE_CLASS_ID_VIRTUAL_DRIVE,
                                      block_operation_p,
                                      ((fbe_base_object_t *)((fbe_base_config_t *)virtual_drive_p))->object_id,
                                      0x0, /*extra information*/
                                      FBE_TRUE /*RBA trace start*/);
    }
#endif

    /* Call the virtual drive block transport handle i/o routine to take appropriate action. */
    //status = fbe_virtual_drive_block_transport_handle_io(virtual_drive_p, packet_p);

    /* If the virtual drive is in pass-thru mode simply forward the packet to
     * the downstream object else go through the raid group I/O entry to perform 
     * mirror operation.
     */
    fbe_virtual_drive_get_configuration_mode(virtual_drive_p, &configuration_mode);

    if(configuration_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_FIRST_EDGE){
        fbe_base_config_get_block_edge((fbe_base_config_t *)virtual_drive_p, &edge, 0);
    }else if(configuration_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_SECOND_EDGE){
        fbe_base_config_get_block_edge((fbe_base_config_t *)virtual_drive_p, &edge, 1);
    }

    if((configuration_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_FIRST_EDGE) ||
       (configuration_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_SECOND_EDGE))
    {
        /* Forward the packet to the first downstream edge. */
        //status = fbe_virtual_drive_block_transport_entry_allocate_packet(virtual_drive_p, packet_p);

        if (fbe_base_config_is_rekey_mode((fbe_base_config_t *)virtual_drive_p))
        {
            fbe_lba_t lba, paged_start_lba;
            fbe_payload_block_get_lba(block_operation_p, &lba);
            fbe_base_config_metadata_get_paged_record_start_lba((fbe_base_config_t *)virtual_drive_p, &paged_start_lba);

            if ((lba >= paged_start_lba) &&
                (fbe_raid_group_is_encryption_state_set((fbe_raid_group_t *)virtual_drive_p, FBE_RAID_GROUP_ENCRYPTION_FLAG_PAGED_ENCRYPTED) ||
                 fbe_raid_group_is_encryption_state_set((fbe_raid_group_t *)virtual_drive_p, FBE_RAID_GROUP_ENCRYPTION_FLAG_PAGED_NEEDS_RECONSTRUCT))) 
            {
                fbe_payload_block_set_flag(block_operation_p, FBE_PAYLOAD_BLOCK_OPERATION_FLAGS_USE_NEW_KEY);
            }
        }
        /* Clear cancel function */
        fbe_transport_set_cancel_function(packet_p, NULL, NULL);
        status = fbe_block_transport_send_functional_packet(edge, packet_p);
    }
    else if(fbe_virtual_drive_is_mirror_configuration_mode_set(virtual_drive_p))
    {
        /* Now that packet is in VD, update its resource priority to VD's base priority */
        fbe_virtual_drive_class_set_resource_priority(packet_p);

        /* call the raid group block transport to handle mirror i/o. */
        status = fbe_raid_group_block_transport_entry((fbe_transport_entry_context_t)virtual_drive_p, packet_p);
    }
    else
    {
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        status = FBE_STATUS_GENERIC_FAILURE;
    }

    /* Return the I/O start status. */
    return status;
}
/******************************************************************************
 * end fbe_virtual_drive_block_transport_entry()
 ******************************************************************************/

/*!****************************************************************************
 * @fn fbe_virtual_drive_block_transport_entry_allocate_packet()
 ******************************************************************************
 * @brief
 *  This function is used to allocate the packet in pass through mode.
 *  
 * @param virtual_drive_p   - Pointer to the virtual drive object.
 * @param packet_p          - Pointer to the packet.
 *
 * @return fbe_status_t.
 *
 * @note
 *  This function is is added temporary until packet size allows us to allocate 
 *  more payload in I/O stack, once packet size allows us to have more payloads
 *  then we will directly forward request down without allocating new packet.
 *
 * @todo
 *  Remove this allocation when the additional block payload is either no longer
 *  required or added to the packet.
 *
 * @author
 *  4/28/2009 - Created. Dhaval Patel
 *
 ******************************************************************************/
static fbe_status_t
fbe_virtual_drive_block_transport_entry_allocate_packet(fbe_virtual_drive_t * virtual_drive_p,
                                                        fbe_packet_t * packet_p)
{
    fbe_status_t                    status = FBE_STATUS_OK;
	fbe_memory_request_t           *memory_request_p = NULL;
    fbe_payload_ex_t              *sep_payload_p = NULL;
    fbe_payload_block_operation_t  *block_operation_p = NULL;

    /* get the payload and prev block operation. */
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    block_operation_p =  fbe_payload_ex_get_present_block_operation(sep_payload_p);

    /* We need to allocate new packet due to the limited number of block
     * payloads.  The memory request priority comes from the original packet.
     */
    memory_request_p = fbe_transport_get_memory_request(packet_p);

    /* build the memory request for allocation. */
    fbe_memory_build_chunk_request(memory_request_p,
                                   FBE_MEMORY_CHUNK_SIZE_FOR_PACKET,
                                   1,
                                   fbe_transport_get_resource_priority(packet_p),
                                   fbe_transport_get_io_stamp(packet_p),
                                   (fbe_memory_completion_function_t)fbe_virtual_drive_block_transport_entry_allocation_completion,
                                   virtual_drive_p);

	fbe_transport_memory_request_set_io_master(memory_request_p, packet_p);

    /* issue the memory request to memory service. */
    status = fbe_memory_request_entry(memory_request_p);
    if((status != FBE_STATUS_OK) && (status != FBE_STATUS_PENDING))
    {
        fbe_base_object_trace((fbe_base_object_t*)virtual_drive_p, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s: memory request failed with status: 0x%x\n",
                              __FUNCTION__, status);
        fbe_payload_block_set_status(block_operation_p,
                                     FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID_REQUEST,
                                     FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_UNEXPECTED_ERROR);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return status;
    }

    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_virtual_drive_block_transport_entry_allocate_packet()
 ******************************************************************************/

/*!****************************************************************************
 * @fn fbe_virtual_drive_block_transport_entry_allocation_completion()
 ******************************************************************************
 * @brief
 *  This function is used as the completion function for the memory allocation
 *  of the new packet.
 *  
 * @note
 *  This function is is added temporary until packet size allows us to allocate 
 *  more payload in I/O stack, once packet size allows us to have more payloads
 *  then we will directly forward request down without allocating new packet.
 *
 * @param memory_request    - Memory request for the allocation of the packet.
 * @param context           - Memory request completion context.
 *
 * @return fbe_status_t.
 *
 * @author
 *  4/28/2009 - Created. Dhaval Patel
 *
 ******************************************************************************/
static fbe_status_t
fbe_virtual_drive_block_transport_entry_allocation_completion(fbe_memory_request_t * memory_request, 
                                                              fbe_memory_completion_context_t context)
{
    fbe_virtual_drive_t *                   virtual_drive_p = NULL;
    fbe_packet_t *                          new_packet_p = NULL;
    fbe_packet_t *                          packet_p = NULL;
    fbe_payload_ex_t *                     sep_payload_p = NULL;
    fbe_payload_ex_t *                     new_sep_payload_p = NULL;
    fbe_payload_block_operation_t *         block_operation_p = NULL;
    fbe_payload_block_operation_t *         new_block_operation_p = NULL;
    fbe_sg_element_t *                      sg_list_p = NULL;
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_raid_iots_t *                       raid_iots_p = NULL;
    fbe_memory_header_t *                   master_memory_header = NULL;
    fbe_virtual_drive_configuration_mode_t  configuration_mode = FBE_VIRTUAL_DRIVE_CONFIG_MODE_UNKNOWN;
    fbe_packet_resource_priority_t          new_packet_resource_priority = 0;

    virtual_drive_p = (fbe_virtual_drive_t *)context;

    /* Get the originating packet and validate that the memory request is complete
     */
    packet_p = fbe_transport_memory_request_to_packet(memory_request);
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    block_operation_p = fbe_payload_ex_get_present_block_operation(sep_payload_p);

    /* Check allocation status */
    if (fbe_memory_request_is_allocation_complete(memory_request) == FBE_FALSE)
    {
        /* Currently this callback doesn't expect any aborted requests */
        fbe_base_object_trace((fbe_base_object_t*)virtual_drive_p, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s: memory request: 0x%p failed\n",
                              __FUNCTION__, memory_request);
        fbe_payload_block_set_status(block_operation_p,
                                     FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED,
                                     FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_UNEXPECTED_ERROR);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Get the pointer to the allocated memory from the memory request. */
    master_memory_header = fbe_memory_get_first_memory_header(memory_request);
    new_packet_p = (fbe_packet_t *)fbe_memory_header_to_data(master_memory_header);

    /* Initialize sub packet. */
    fbe_transport_initialize_sep_packet(new_packet_p);
    new_sep_payload_p = fbe_transport_get_payload_ex(new_packet_p);

    /* Set sg list */
    fbe_payload_ex_get_sg_list(sep_payload_p, &sg_list_p, NULL);
    fbe_payload_ex_set_sg_list(new_sep_payload_p, sg_list_p, 1);

    /* Set block operation */
    block_operation_p = fbe_payload_ex_get_block_operation(sep_payload_p);
    new_block_operation_p = fbe_payload_ex_allocate_block_operation(new_sep_payload_p);

    fbe_payload_block_build_operation(new_block_operation_p,
                                      block_operation_p->block_opcode,
                                      block_operation_p->lba,
                                      block_operation_p->block_count,
                                      block_operation_p->block_size,
                                      block_operation_p->optimum_block_size,
                                      block_operation_p->pre_read_descriptor);

    new_block_operation_p->block_flags = block_operation_p->block_flags;
    new_block_operation_p->payload_sg_descriptor = block_operation_p->payload_sg_descriptor;
    new_block_operation_p->block_edge_p = block_operation_p->block_edge_p;

    /* Propagate the current expiration time */
    fbe_transport_propagate_expiration_time(new_packet_p, packet_p);
    fbe_transport_propagate_physical_drive_service_time(new_packet_p, packet_p);

    /* Set completion function */
    fbe_transport_set_completion_function(new_packet_p, fbe_virtual_drive_block_transport_entry_completion, virtual_drive_p);

    /* We need to assosiate newly allocated packet with original one */
    fbe_transport_add_subpacket(packet_p, new_packet_p);

    /* Bump the resource priority for any downstream allocations */
    new_packet_resource_priority = fbe_transport_get_resource_priority(packet_p);
    new_packet_resource_priority += FBE_TRANSPORT_RESOURCE_PRIORITY_ADJUSTMENT_FIRST;
    fbe_transport_set_resource_priority(new_packet_p, new_packet_resource_priority); 

    /* Put packet on the termination queue while we wait for the subpacket to complete. */
    fbe_payload_ex_get_iots(sep_payload_p, &raid_iots_p);
    fbe_raid_iots_set_as_not_used(raid_iots_p);
    fbe_transport_set_cancel_function(packet_p, fbe_base_object_packet_cancel_function, (fbe_base_object_t *)virtual_drive_p);
    fbe_base_object_add_to_terminator_queue((fbe_base_object_t *)virtual_drive_p, packet_p);

    /* get the configuration mode of the virtual drive object. */
    fbe_virtual_drive_get_configuration_mode(virtual_drive_p, &configuration_mode);

    if(configuration_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_FIRST_EDGE)
    {
        /* Forward the packet to the first downstream edge. */
        status = fbe_base_config_send_functional_packet((fbe_base_config_t *) virtual_drive_p, new_packet_p, 0);
    }
    else if(configuration_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_SECOND_EDGE)
    {
        /* Forward the packet to the second downstream edge. */
        status = fbe_base_config_send_functional_packet((fbe_base_config_t *) virtual_drive_p, new_packet_p, 1);
    }
    else
    {
        /* It does not expect configuration mode to setup mirror when it is here. */
        fbe_base_object_trace((fbe_base_object_t*)virtual_drive_p, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s invalid configuration mode, config_mode:0x%x\n",
                              __FUNCTION__, configuration_mode);
        fbe_payload_block_set_status(new_block_operation_p,
                                     FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID_REQUEST,
                                     FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_UNEXPECTED_ERROR);
        fbe_transport_set_status(new_packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(new_packet_p);
    }

    /* handle this new packet based on configuration mode. */
    return status;
}
/******************************************************************************
 * end virtual_drive_block_transport_entry_allocation_completion()
 ******************************************************************************/

/*!****************************************************************************
 * @fn fbe_virtual_drive_block_transport_entry_completion()
 ******************************************************************************
 * @brief
 *  This function is used as the completion for the newly created packet, it
 *  release the newly allocated packet after copying appropriate data and then
 *  it completes master packet.
 *
 * @note
 *  This function is is added temporary until packet size allows us to allocate 
 *  more payload in I/O stack, once packet size allows us to have more payloads
 *  then we will directly forward request down without allocating new packet.
 *
 * @param packet_p          - Pointer the the I/O packet.
 * @param context           - Packet completion context.
 *
 * @return fbe_status_t.
 *
 * @author
 *  4/28/2009 - Created. Dhaval Patel
 *
 ******************************************************************************/
static fbe_status_t 
fbe_virtual_drive_block_transport_entry_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_virtual_drive_t * virtual_drive_p = NULL;
    fbe_packet_t * master_packet = NULL;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_ex_t * master_payload = NULL;
    fbe_payload_block_operation_t * master_block_operation;
    fbe_payload_block_operation_t * block_operation;
    fbe_status_t status;
    fbe_u32_t status_qualifier;
    fbe_payload_block_operation_status_t block_operation_status;
    fbe_payload_block_operation_qualifier_t block_operation_qualifier;
    fbe_time_t block_operation_retry_wait_msecs;
    fbe_lba_t block_operation_media_error_lba;
    fbe_object_id_t pvd_object_id;	

    virtual_drive_p = (fbe_virtual_drive_t *)context;
    master_packet = fbe_transport_get_master_packet(packet);

    /* Remove master packet from the termination queue. */
    fbe_base_object_remove_from_terminator_queue((fbe_base_object_t *)virtual_drive_p, master_packet);

    master_payload = fbe_transport_get_payload_ex(master_packet);
    payload = fbe_transport_get_payload_ex(packet);

    master_block_operation = fbe_payload_ex_get_block_operation(master_payload);
    block_operation = fbe_payload_ex_get_block_operation(payload);

#if 0
    /* trace to RBA buffer */
    if (fbe_traffic_trace_is_enabled (KTRC_TVD))
    {
        fbe_traffic_trace_block_operation(FBE_CLASS_ID_VIRTUAL_DRIVE,
                                      master_block_operation,
                                      ((fbe_base_object_t *)((fbe_base_config_t *)virtual_drive_p))->object_id,
                                      0x0, /*extra information*/
                                      FBE_FALSE /*RBA trace end*/);
    }
#endif

    /* Copy block status */
    fbe_payload_block_get_status(block_operation, &block_operation_status);
    fbe_payload_block_get_qualifier(block_operation, &block_operation_qualifier);
    fbe_payload_ex_get_media_error_lba(payload, &block_operation_media_error_lba);
    fbe_payload_ex_get_retry_wait_msecs(payload, &block_operation_retry_wait_msecs);
    fbe_payload_ex_get_pvd_object_id(payload, &pvd_object_id);
    fbe_transport_propagate_physical_drive_service_time(master_packet, packet);

    fbe_payload_block_set_status(master_block_operation, block_operation_status, block_operation_qualifier);
    fbe_payload_ex_set_media_error_lba(master_payload, block_operation_media_error_lba);
    fbe_payload_ex_set_retry_wait_msecs(master_payload, block_operation_retry_wait_msecs);
    fbe_payload_ex_set_pvd_object_id(master_payload, pvd_object_id);

    /* Copy packet status */
    status = fbe_transport_get_status_code(packet); 
    status_qualifier = fbe_transport_get_status_qualifier(packet);

    fbe_transport_set_status(master_packet, status, status_qualifier);

    /* We need to remove packet from master queue */
    fbe_transport_remove_subpacket(packet);

    fbe_payload_ex_release_block_operation(payload, block_operation);

    /* destroy the packet. */
    fbe_transport_destroy_packet(packet);

    /* release the memory for the packet allocated by the virtual drive */
    //fbe_memory_request_get_entry_mark_free(&master_packet->memory_request, &memory_entry_ptr);
    //fbe_memory_free_entry(memory_entry_ptr);
	fbe_memory_free_request_entry(&master_packet->memory_request);

    /* Complete master packet */
    fbe_transport_complete_packet(master_packet);
    return FBE_STATUS_OK;
}
/******************************************************************************
 * end virtual_drive_block_transport_entry_completion()
 ******************************************************************************/


/*************************
 * end file fbe_base_config_executer.c
 *************************/
 
 
