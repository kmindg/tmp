 /***************************************************************************
  * Copyright (C) EMC Corporation 2009-2010
  * All rights reserved.
  * Licensed material -- property of EMC Corporation
  ***************************************************************************/
 
 /*!**************************************************************************
  * @file fbe_provision_drive_executor.c
  ***************************************************************************
  *
  * @brief
  *  This file contains the executer functions for the provision_drive.
  * 
  *  This includes the fbe_provision_drive_io_entry() function which is our entry
  *  point for functional packets.
  * 
  * @ingroup provision_drive_class_files
  * 
  * @version
  *   05/20/2009:  Created. RPF
  *
  ***************************************************************************/
 
 /*************************
  *   INCLUDE FILES
  *************************/
#include "fbe/fbe_provision_drive.h"
#include "fbe_provision_drive_private.h"
#include "fbe_raid_library.h"
#include "fbe_traffic_trace.h"
#include "fbe_service_manager.h"
#include "fbe_transport_memory.h"
#include "fbe/fbe_sector.h"

 /*************************
  *   FUNCTION DEFINITIONS
  *************************/

/* it is the entry point to handle the block transport i/o operation. */
static fbe_status_t fbe_provision_drive_handle_io(fbe_packet_t * packet, fbe_packet_completion_context_t context);
static fbe_status_t fbe_provision_drive_handle_io_check_slf(fbe_packet_t * packet, fbe_packet_completion_context_t context);

/* it determines whether stripe lock is required before handliing i/o operation. */
static fbe_status_t fbe_provision_drive_block_transport_handle_io_get_stripe_lock_if_needed(fbe_provision_drive_t * provision_drive,
                                                                                            fbe_packet_t * packet);

/* handle i/o without stripe lock - zero on demand not set. */
static fbe_status_t fbe_provision_drive_handle_io_without_lock(fbe_packet_t * packet, fbe_packet_completion_context_t context);

/* handle i/o with stripe lock - background zeroing is in progress and so i/o should go through stripe lock. */
fbe_status_t fbe_provision_drive_handle_io_with_lock(fbe_packet_t * packet,
                                                                            fbe_packet_completion_context_t context);

//static fbe_status_t fbe_provision_drive_block_transport_allocate_packet_completion(fbe_memory_request_t * memory_request, 
//																			  fbe_memory_completion_context_t context);

static fbe_status_t fbe_provision_drive_allocate_packet_master_completion(fbe_packet_t * packet,
                                                                          fbe_packet_completion_context_t context);

static fbe_status_t fbe_provision_drive_send_functional_packet_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);

static fbe_status_t fbe_provision_drive_send_functional_packet_for_object_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);
static fbe_status_t fbe_provision_drive_metadata_read_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);

static fbe_status_t fbe_provision_drive_append_lba_stamp(fbe_provision_drive_t * provision_drive,
                                                         fbe_sg_element_t *sg_p,
                                                         fbe_lba_t lba,
                                                         fbe_u32_t blocks);


/*!****************************************************************************
 * fbe_provision_drive_io_entry()
 ******************************************************************************
 * @brief
 *  This function is the entry point for functional transport operations to the
 *  provision_drive object. The FBE infrastructure will call this function for
 *  our object.
 *
 * @param object_handle - The provision_drive handle.
 * @param packet - The io packet that is arriving.
 *
 * @return
 *  fbe_status_t - FBE_STATUS_GENERIC_FAILURE if transport is unknown,
 *                 otherwise we return the status of the transport handler.
 *
 * @author
 *  05/22/09 - Created. RPF
 *
 ******************************************************************************/
fbe_status_t 
fbe_provision_drive_io_entry(fbe_object_handle_t object_handle, fbe_packet_t * packet)
{
    fbe_status_t status;
    fbe_provision_drive_t * provision_drive = NULL;
    fbe_payload_ex_t * payload = NULL;
    fbe_object_id_t my_object_id;

    provision_drive = (fbe_provision_drive_t *)fbe_base_handle_to_pointer(object_handle);
    /* Set pvd_object_id in io entry */
    payload = fbe_transport_get_payload_ex(packet);
    fbe_base_object_get_object_id((fbe_base_object_t *)provision_drive,&my_object_id);
    fbe_payload_ex_set_pvd_object_id(payload, my_object_id);

    status = fbe_base_config_bouncer_entry((fbe_base_config_t * )provision_drive, packet);
    if(status == FBE_STATUS_OK){ /* Small stack */
        fbe_transport_set_completion_function(packet, fbe_provision_drive_handle_io_check_slf, provision_drive);
        status = fbe_transport_complete_packet(packet);		
    }
    return status;
}
/******************************************************************************
 * end fbe_provision_drive_io_entry()
 ******************************************************************************/

/*!****************************************************************************
 * @fn fbe_provision_drive_block_transport_entry()
 ******************************************************************************
 * @brief
 *   This is the entry function that the block transport will
 *   call to pass a packet to the provision drive object.
 *  
 * @param context       - Pointer to the provision drive object. 
 * @param packet      - The packet to process.
 * 
 * @return fbe_status_t
 *
 * @author
 *  4/15/2010 - Created. Dhaval Patel
 *
 ******************************************************************************/
fbe_status_t
fbe_provision_drive_block_transport_entry(fbe_transport_entry_context_t context, fbe_packet_t * packet)
{
    fbe_status_t status;
    fbe_provision_drive_t * provision_drive = (fbe_provision_drive_t *)context;

    /* hand off to the provision drive block transport handler. */
    fbe_transport_set_completion_function(packet, fbe_provision_drive_handle_io_check_slf, provision_drive);
    status = fbe_transport_complete_packet(packet);

    return status;
}

/*!****************************************************************************
 * fbe_provision_drive_block_transport_restart_io()
 ******************************************************************************
 * @brief
 *  Restart the iots after it has been queued, it was queued before acquiring
 *  stripe lock and so it will resume from where it left.
 *
 * @param iots_p - I/O to restart.               
 *
 * @return fbe_raid_state_status_t - FBE_RAID_STATE_STATUS_WAITING
 *
 * @author
 *  07/07/2010 - Created. Dhaval Patel
 *
 ******************************************************************************/
fbe_raid_state_status_t
fbe_provision_drive_block_transport_restart_io(fbe_raid_iots_t * iots_p)
{
    fbe_packet_t *              packet = NULL; 
    fbe_provision_drive_t *     provision_drive = NULL; 
    fbe_payload_ex_t *         sep_payload_p = NULL;

    packet = fbe_raid_iots_get_packet(iots_p);
    provision_drive = (fbe_provision_drive_t *)iots_p->callback_context;

    sep_payload_p = fbe_transport_get_payload_ex(packet);
    fbe_payload_ex_get_iots(sep_payload_p, &iots_p);
    fbe_raid_iots_mark_unquiesced(iots_p);

    fbe_base_object_trace((fbe_base_object_t *)provision_drive,
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s: restarting iots %p\n", __FUNCTION__, iots_p);

    /* Before we restart the packet, remove the packet from terminator queue, it is required
     * to avoid multiple entry of same element in terminator queue. 
     */
    fbe_base_object_remove_from_terminator_queue((fbe_base_object_t *) provision_drive, packet);

    /* set the iots status as not used before we restart the iots. */
    fbe_raid_iots_set_as_not_used(iots_p);

    /* If SLF complete the packet.
     */
    if (fbe_provision_drive_is_local_state_set(provision_drive, FBE_PVD_LOCAL_STATE_SLF))
    {
        fbe_transport_set_packet_attr(packet, FBE_PACKET_FLAG_SLF_REQUIRED);
        fbe_transport_set_status(packet, FBE_STATUS_EDGE_NOT_ENABLED, 0);
        fbe_transport_complete_packet(packet);
        return FBE_RAID_STATE_STATUS_WAITING;
    }

    /* Clear the flag as we are re-entering fbe_provision_drive_handle_io()
     * where resource priority of the packet will be adjusted again.
     */
    packet->resource_priority_boost &= ~FBE_TRANSPORT_RESOURCE_PRIORITY_BOOST_FLAG_PVD;
    if (sep_payload_p->current_operation->payload_opcode == FBE_PAYLOAD_OPCODE_STRIPE_LOCK_OPERATION)
    {
        /* If we already have the stripe lock don't get it again.
         */
        fbe_base_object_trace((fbe_base_object_t *)provision_drive,
                              FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s payload opcode is %d\n", __FUNCTION__, sep_payload_p->current_operation->payload_opcode);
        fbe_transport_set_completion_function(packet, fbe_provision_drive_handle_io_without_lock, provision_drive);
    }
    else
    {
        /* Otherwise go down the path where we get the stripe lock.
         */
        fbe_transport_set_completion_function(packet, fbe_provision_drive_handle_io, provision_drive);
    }
    fbe_transport_complete_packet(packet);

    return FBE_RAID_STATE_STATUS_WAITING;
}
/******************************************************************************
 * end fbe_provision_drive_block_transport_restart_io()
 ******************************************************************************/

/*!****************************************************************************
 * @fn fbe_provision_drive_io_allocate_packet_completion()
 ******************************************************************************
 * @brief
 *  This function is used to allocate the packet to be used when a Zero On Demand
 *  request might be required.
 * 
 *  @note
 *  This code is added temporary until we do transition from sep payload
 *  to payload on block transport edge between logical package and physical
 *  package.
 *
 * @param memory_request - Pointer to the memory request.
 * @param context        - Pointer to the provision drive object.
 *
 * @return fbe_status_t.
 *
 ******************************************************************************/
static fbe_status_t fbe_provision_drive_io_allocate_packet_completion(fbe_memory_request_t * memory_request_p, 
                                                                      fbe_memory_completion_context_t context)
{
    fbe_provision_drive_t                  *provision_drive = NULL;
    fbe_packet_t                           *new_packet_p = NULL;
    fbe_packet_t                           *packet = NULL;
    fbe_payload_ex_t                       *payload_p = NULL;
    fbe_payload_ex_t                       *sep_payload_p = NULL;
    fbe_payload_block_operation_t          *block_operation_p = NULL;
    fbe_payload_block_operation_t          *new_block_operation_p = NULL;
    fbe_sg_element_t                       *sg_list = NULL;
    fbe_status_t                            status;
    fbe_optimum_block_size_t                optimum_block_size;
    fbe_block_edge_t                       *block_edge = NULL;
    fbe_lba_t                               start_lba;
    fbe_block_count_t                       block_count;
    fbe_payload_block_operation_opcode_t    opcode;
    fbe_lba_t                               paged_metadata_start_lba;
    fbe_scheduler_hook_status_t             hook_status;


    provision_drive = (fbe_provision_drive_t *)context;
    packet = fbe_transport_memory_request_to_packet(memory_request_p);

    /* Get the current block operation.*/
    sep_payload_p = fbe_transport_get_payload_ex(packet);
    block_operation_p = fbe_payload_ex_get_present_block_operation(sep_payload_p);

    /* Check allocation status */
    if (fbe_memory_request_is_allocation_complete(memory_request_p) == FBE_FALSE) {
        /* Currently this callback doesn't expect any aborted requests */
        fbe_provision_drive_utils_trace(provision_drive, 
                                        FBE_TRACE_LEVEL_ERROR,
                                        FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                        FBE_PROVISION_DRIVE_DEBUG_FLAG_GENERAL_TRACING,
                                        "GNRL: memory request: 0x%p state: %d allocation failed \n",
                                        memory_request_p, memory_request_p->request_state);
        fbe_payload_block_set_status(block_operation_p,
                                     FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED,
                                     FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_UNEXPECTED_ERROR);

        fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }

    /* Get the start lba of the block operation. */
    fbe_payload_block_get_lba(block_operation_p, &start_lba);
    fbe_payload_block_get_block_count(block_operation_p, &block_count);
    fbe_payload_block_get_opcode(block_operation_p, &opcode);

    /* get the paged metadata start lba of the pvd object. */
    fbe_base_config_metadata_get_paged_record_start_lba((fbe_base_config_t *) provision_drive,
                                                        &paged_metadata_start_lba);
    fbe_payload_ex_get_sg_list(sep_payload_p, &sg_list, NULL);

    /* Check if this is a Metadata Operation for the PVD since we have to generate LBA stamp for the
     * writes going down. Also setup a completion function to validate the data on read
     */
    if(start_lba >= paged_metadata_start_lba) 
    {
        if(opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_VERIFY)
        {
            fbe_provision_drive_check_hook(provision_drive,
                                           SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_BLOCK_OPERATION,
                                           FBE_PROVISION_DRIVE_SUBSTATE_BLOCK_OPERATION_CHECK,
                                           opcode, &hook_status);
        }     
    }

    /* get the pointer to subpacket from the preallocated memory. */
    status = fbe_provision_drive_utils_get_packet_and_sg_list(provision_drive,
                                                              memory_request_p,
                                                              &new_packet_p,
                                                              1, /* number of subpackets. */
                                                              NULL,
                                                              0);
    if(status != FBE_STATUS_OK)
    {
        fbe_provision_drive_utils_trace(provision_drive,
                                        FBE_TRACE_LEVEL_ERROR,
                                        FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                        FBE_PROVISION_DRIVE_DEBUG_FLAG_GENERAL_TRACING,
                                        "%s: get packet and sg list failed, status:0x%x\n",
                                        __FUNCTION__, status);
        fbe_transport_destroy_packet(new_packet_p);
        //fbe_memory_request_get_entry_mark_free(memory_request_p, &memory_ptr);
        //fbe_memory_free_entry(memory_ptr);
        fbe_memory_free_request_entry(memory_request_p);
        fbe_payload_block_set_status(block_operation_p,
                                     FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID_REQUEST,
                                     FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_UNEXPECTED_ERROR);
        fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }

    /* initialize sub packet. */
    fbe_transport_initialize_packet(new_packet_p);

    /* Before we handle the request, initialize the iots with default values.
     * We need to use the iots in the packet for quiesce, shutdown etc.
     */
    //status = fbe_provision_drive_utils_initialize_iots(provision_drive, new_packet_p);
    if (status != FBE_STATUS_OK) 
    {
        fbe_provision_drive_utils_trace(provision_drive,
                                        FBE_TRACE_LEVEL_ERROR,
                                        FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                        FBE_PROVISION_DRIVE_DEBUG_FLAG_GENERAL_TRACING,
                                        "%s:initialize iots failed, status:0x%x, line:%d\n", 
                                        __FUNCTION__, status, __LINE__);
        fbe_transport_destroy_packet(new_packet_p);
        //fbe_memory_request_get_entry_mark_free(memory_request_p, &memory_ptr);
        //fbe_memory_free_entry(memory_ptr);
        fbe_memory_free_request_entry(memory_request_p);
        fbe_payload_block_set_status(block_operation_p,
                                     FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID_REQUEST,
                                     FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_UNEXPECTED_ERROR);

        fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
        //fbe_transport_complete_packet(packet);    
        return status;
    }

    /* set block operation same as master packet's prev block operation. */
    payload_p = fbe_transport_get_payload_ex(new_packet_p);
    new_block_operation_p = fbe_payload_ex_allocate_block_operation(payload_p);

    /* set sg list with new packet. */
    fbe_payload_ex_set_sg_list(payload_p, sg_list, 1);

    fbe_block_transport_edge_get_optimum_block_size(&provision_drive->block_edge, &optimum_block_size);

    fbe_payload_block_build_operation(new_block_operation_p,
                                      block_operation_p->block_opcode,
                                      block_operation_p->lba,
                                      block_operation_p->block_count,
                                      block_operation_p->block_size,
                                      optimum_block_size,
                                      block_operation_p->pre_read_descriptor);

    new_block_operation_p->block_flags = block_operation_p->block_flags;
    new_block_operation_p->payload_sg_descriptor = block_operation_p->payload_sg_descriptor;
    fbe_base_config_get_block_edge((fbe_base_config_t *)provision_drive, &block_edge, 0);
    new_block_operation_p->block_edge_p = block_edge;

    /* initialize the block operation with default status. */
    fbe_payload_block_set_status(block_operation_p,
                                 FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID,
                                 FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_INVALID);

    /* activate this block operation*/
    fbe_payload_ex_increment_block_operation_level(payload_p);

    /* we need to assosiate newly allocated packet with original one */
    status = fbe_transport_add_subpacket(packet, new_packet_p);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)provision_drive, 
                              FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s packet is cancelled packet: %p\n", __FUNCTION__, packet);
        fbe_transport_destroy_packet(new_packet_p);
        /* release the preallocated memory for the zero on demand request. */
        //fbe_memory_request_get_entry_mark_free(&packet->memory_request, &memory_ptr);
        //fbe_memory_free_entry(memory_ptr);
        fbe_memory_free_request_entry(&packet->memory_request);

        fbe_payload_block_set_status(block_operation_p,
                                     FBE_PAYLOAD_BLOCK_OPERATION_STATUS_REQUEST_ABORTED,
                                     FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_CLIENT_ABORTED);

        fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }

    /* Set the sub-packet completion to the standard provision drive completion.
     * When the sub-packet completes it will complete the master packet.
     */
    fbe_transport_set_completion_function(new_packet_p,
                                          fbe_provision_drive_allocate_packet_master_completion,
                                          provision_drive);
    fbe_transport_set_resource_priority(new_packet_p, packet->resource_priority + 1);
    /* Populate the required fields from the master packet.
     */
    fbe_transport_propagate_expiration_time(new_packet_p, packet);
    fbe_transport_propagate_physical_drive_service_time(new_packet_p, packet);

    /* Propagate the io_stamp */
    fbe_transport_set_io_stamp_from_master(packet, new_packet_p);

    /* Before we handle the request, initialize the iots with default values.
     * We need to use the iots in the packet for quiesce, shutdown etc.
     */
    status = fbe_provision_drive_utils_initialize_iots(provision_drive, new_packet_p);
    if (status != FBE_STATUS_OK) 
    {
        fbe_provision_drive_utils_trace(provision_drive,
                                        FBE_TRACE_LEVEL_ERROR,
                                        FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                        FBE_PROVISION_DRIVE_DEBUG_FLAG_GENERAL_TRACING,
                                        "%s:initialize iots failed, status:0x%x, line:%d\n", 
                                        __FUNCTION__, status, __LINE__);
        fbe_transport_destroy_packet(new_packet_p);
        //fbe_memory_request_get_entry_mark_free(memory_request_p, &memory_ptr);
        //fbe_memory_free_entry(memory_ptr);
        fbe_memory_free_request_entry(memory_request_p);
        fbe_payload_block_set_status(block_operation_p,
                                     FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID_REQUEST,
                                     FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_UNEXPECTED_ERROR);

        fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }

    /* Put the master packet on the termination queue while we wait for the 
     * sub-packet to complete.  When the sub-packet completes we expect the
     * master packet to be on the termination queue.
     */
    fbe_transport_set_cancel_function(packet, fbe_base_object_packet_cancel_function, (fbe_base_object_t *)provision_drive);
    fbe_base_object_add_to_terminator_queue((fbe_base_object_t *)provision_drive, packet);

    /*! @note Acquire stripe lock if needed.  Since we are now processing the
     *        sub-packet put it on the block transport and start processing it.
     */
    fbe_provision_drive_block_transport_handle_io_get_stripe_lock_if_needed(provision_drive, new_packet_p);
    fbe_transport_complete_packet(new_packet_p);

    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/******************************************************************************
 * end fbe_provision_drive_io_allocate_packet_completion()
 ******************************************************************************/

/*!****************************************************************************
 * @fn fbe_provision_drive_io_allocate_packet()
 ******************************************************************************
 * @brief
 *  This function is used to allocate the packet before we send a I/O packet
 *  to physical package.
 * 
 *  @note This code is added temporary until we do transition from sep payload
 *  to payload on block transport edge between logical package and physical
 *  package.
 *
 * @param provision_drive - Pointer to the provision drive object.
 * @param packet          - Pointer the the I/O packet.
 *
 * @return fbe_status_t.
 *
 ******************************************************************************/
static fbe_status_t fbe_provision_drive_io_allocate_packet(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_status_t                     status = FBE_STATUS_OK;
    fbe_memory_request_t *           memory_request_p = NULL;
    fbe_memory_request_priority_t    memory_request_priority = 0;
    fbe_payload_ex_t *              sep_payload_p = NULL;
    fbe_payload_block_operation_t *  block_operation_p = NULL;
    fbe_provision_drive_t * provision_drive = (fbe_provision_drive_t *)context;
    fbe_u32_t packet_count = 1;
    fbe_u32_t chunk_size = FBE_MEMORY_CHUNK_SIZE_FOR_PACKET;

    /* get the payload and prev block operation. */
    sep_payload_p = fbe_transport_get_payload_ex(packet);
    block_operation_p =  fbe_payload_ex_get_present_block_operation(sep_payload_p);

    if (block_operation_p->block_opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ZERO)
    {
        /* We might need up to 3 packets that we will use later.
         */
        packet_count = 3;
    }
    if (fbe_payload_block_operation_opcode_is_media_modify(block_operation_p->block_opcode) ||
        fbe_payload_block_operation_opcode_is_media_read(block_operation_p->block_opcode) ||
        (block_operation_p->block_opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_CHECK_ZEROED) ||
        (block_operation_p->block_opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_MARK_DISK_ZERO) ||
        (block_operation_p->block_opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_BACKGROUND_ZERO) ||
        (block_operation_p->block_opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_VERIFY_INVALIDATE))
    {
        /* We might need up to 4 packets that we will use later.
         */
        //packet_count = 1;
        chunk_size = FBE_MEMORY_CHUNK_SIZE_FOR_64_BLOCKS_IO;
    }
    /* allocate the subpacket to process zero on demand I/O */
    memory_request_p = fbe_transport_get_memory_request(packet);
    memory_request_priority = fbe_transport_get_resource_priority(packet);
    memory_request_priority += FBE_TRANSPORT_RESOURCE_PRIORITY_ADJUSTMENT_FIRST;

    /* build the memory request for allocation.  We add (1) to the memory
     * request priority of the packet. */
    fbe_memory_build_chunk_request_sync(memory_request_p,
                                   chunk_size,
                                   packet_count,
                                   memory_request_priority,
                                   fbe_transport_get_io_stamp(packet),
                                   (fbe_memory_completion_function_t)fbe_provision_drive_io_allocate_packet_completion, /* SAFEBUG - cast to supress warning but must fix because memory completion function shouldn't return status */
                                   (fbe_memory_completion_context_t)provision_drive);

    
    fbe_transport_memory_request_set_io_master(memory_request_p,  packet);

    /* issue the memory request to memory service. */
    status = fbe_memory_request_entry(memory_request_p);
    if (status == FBE_STATUS_OK)
    { 
        /* We do have memory right away */
        status = fbe_provision_drive_io_allocate_packet_completion(memory_request_p, (fbe_memory_completion_context_t)provision_drive);
        return status;
    }
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/******************************************************************************
 * end fbe_provision_drive_io_allocate_packet()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_provision_drive_start_direct_metadata_io()
 ******************************************************************************
 * @brief
 *  Start a metadata I/O without allocating a sub packet.
 *
 * @param provision_drive - Pointer to the provision drive object.
 * @param packet          - Pointer the the I/O packet.
 *
 * @return fbe_status_t.
 *
 * @author
 *  9/13/2012 - Created. Rob Foley
 *
 ******************************************************************************/
fbe_status_t
fbe_provision_drive_start_direct_metadata_io(fbe_provision_drive_t *provision_drive, fbe_packet_t *packet)
{
    fbe_payload_ex_t *sep_payload_p = NULL;
    fbe_payload_block_operation_t *block_operation_p = NULL;
    fbe_sg_element_t *sg_p = NULL;
    fbe_status_t status;
    fbe_lba_t start_lba;
    fbe_block_count_t block_count;
    fbe_payload_block_operation_opcode_t opcode;
    fbe_lba_t paged_metadata_start_lba;
    fbe_scheduler_hook_status_t hook_status;

    sep_payload_p = fbe_transport_get_payload_ex(packet);
    block_operation_p = fbe_payload_ex_get_present_block_operation(sep_payload_p);

    fbe_payload_block_get_lba(block_operation_p, &start_lba);
    fbe_payload_block_get_block_count(block_operation_p, &block_count);
    fbe_payload_block_get_opcode(block_operation_p, &opcode);

    fbe_base_config_metadata_get_paged_record_start_lba((fbe_base_config_t *) provision_drive,
                                                        &paged_metadata_start_lba);
    fbe_payload_ex_get_sg_list(sep_payload_p, &sg_p, NULL);

    /* Check if this is a Metadata Operation for the PVD since we have to generate LBA stamp for the
     * writes going down. Also setup a completion function to validate the data on read
     */
    if(start_lba >= paged_metadata_start_lba) 
    {
        if(opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_VERIFY)
        {
            fbe_provision_drive_check_hook(provision_drive,
                                           SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_BLOCK_OPERATION,
                                           FBE_PROVISION_DRIVE_SUBSTATE_BLOCK_OPERATION_CHECK,
                                           opcode, &hook_status);
        }

        /* if it is a write operation, the LBA stamps needs to be appended */
        if(fbe_payload_block_operation_opcode_is_media_modify(opcode))
        {
            fbe_provision_drive_append_lba_stamp(provision_drive, sg_p, start_lba, (fbe_u32_t) block_count);
        }
        /* If it is read operation, need to validate the data on the return path and so setup 
         * a completion function
         */
        else if(fbe_payload_block_operation_opcode_is_media_read(opcode)) 
        {
            fbe_transport_set_completion_function(packet,
                                                  fbe_provision_drive_metadata_read_completion,
                                                  provision_drive);
        }
    }

    status = fbe_base_config_send_functional_packet((fbe_base_config_t *)provision_drive, packet, 0);
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/**************************************
 * end fbe_provision_drive_start_direct_metadata_io
 **************************************/
/*!****************************************************************************
 * @fn fbe_provision_drive_process_io()
 ******************************************************************************
 * @brief
 *  This function processes I/Os.
 *      o Monitor I/Os do not need a sub-packet.
 *      o Database I/Os do not require a sub-packet.
 *      o All other I/Os require a sub-packet.
 *
 * @param provision_drive - Pointer to the provision drive object.
 * @param packet          - Pointer the the I/O packet.
 *
 * @return fbe_status_t.
 *
 * @author
 *  4/15/2009 - Created. Dhaval Patel
 *
 ******************************************************************************/
static fbe_status_t fbe_provision_drive_process_io(fbe_provision_drive_t *provision_drive, fbe_packet_t *packet)
{
    fbe_status_t                    status;
    fbe_payload_ex_t               *sep_payload_p = NULL;
    fbe_payload_block_operation_t  *block_operation_p = NULL;
    fbe_lba_t                       exported_capacity;
    fbe_object_id_t                 pvd_object_id;
    fbe_payload_block_operation_opcode_t    block_opcode;

    /* Get the payload */
    /* Initialize block_operation_p to avoide NULL derefencing in fbe_payload_block_set_status(...) below.  */
    sep_payload_p = fbe_transport_get_payload_ex(packet);
    block_operation_p = fbe_payload_ex_get_present_block_operation(sep_payload_p);
    
    /* Before we handle the request, initialize the iots with default values.
     * We need to use the iots in the packet for quiesce, shutdown etc.
     */
    status = fbe_provision_drive_utils_initialize_iots(provision_drive, packet);
    if (status != FBE_STATUS_OK) 
    {
        fbe_provision_drive_utils_trace(provision_drive,
                                        FBE_TRACE_LEVEL_ERROR,
                                        FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                        FBE_PROVISION_DRIVE_DEBUG_FLAG_GENERAL_TRACING,
                                        "%s:initialize iots failed, status:0x%x, line:%d\n", 
                                        __FUNCTION__, status, __LINE__);
        fbe_payload_block_set_status(block_operation_p,
                                     FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID_REQUEST,
                                     FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_UNEXPECTED_ERROR);

        fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
        //fbe_transport_complete_packet(packet);    
        return status;
    }

    /* Now that packet is in PVD, update its resource priority to PVD's base priority.
     * Setting Priority here as io_entry and block_transport_entry both funnell thru here.
     */
    fbe_provision_drive_class_set_resource_priority(packet);


    /* Monitor requests and metadata request don't need to allocate a sub-packet 
     * but other request do.  If the packet originated from the monitor the monitor
     * attribute will be set.
     */
    fbe_base_config_get_capacity((fbe_base_config_t *)provision_drive, &exported_capacity );
    fbe_base_object_get_object_id((fbe_base_object_t*)provision_drive, &pvd_object_id);

    fbe_payload_block_get_opcode(block_operation_p, &block_opcode);
    if ( (block_operation_p->lba >= exported_capacity) &&
         ((block_opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ) ||
          (block_opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE) ||
          (block_opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_VERIFY)) )
    {
        /* Metadata requests should not need a stripe lock.
         */
        status = fbe_provision_drive_start_direct_metadata_io(provision_drive, packet);
        return status;
    }
    else if ((fbe_transport_is_monitor_packet(packet, pvd_object_id)) &&
             (block_opcode != FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_BACKGROUND_ZERO) &&
             (block_opcode != FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_VERIFY_INVALIDATE))
    {
        /* acquire stripe lock if needed.
         * Monitor ops with the exception of BGZ do not get an initial subpacket. 
         */
        status = fbe_provision_drive_block_transport_handle_io_get_stripe_lock_if_needed(provision_drive, packet);
        return status;
    }

    /* If this is not a monitor or metadata request, we need to allocate a packet.
     */
    status = fbe_provision_drive_io_allocate_packet(packet, (fbe_packet_completion_context_t)provision_drive);
    return status;
}
/*********************************************
 * end fbe_provision_drive_process_io()
 *********************************************/
/*!****************************************************************************
 * @fn fbe_provision_drive_packet_completion_check_slf()
 ******************************************************************************
 * @brief
 *  This function checks packet status for drive dead errors.
 *
 * @param packet_p - Pointer the the I/O packet.
 * @param context - Context of completion function.
 *
 * @return fbe_status_t.
 *
 * @author
 *  05/26/2012 - Created. Lili Chen
 *
 ******************************************************************************/
static fbe_status_t 
fbe_provision_drive_packet_completion_check_slf(fbe_packet_t * packet_p, fbe_packet_completion_context_t context)
{
    fbe_provision_drive_t * provision_drive_p = (fbe_provision_drive_t *)context;
    fbe_status_t status;
    fbe_payload_ex_t * payload_p = NULL;
    fbe_raid_iots_t * iots_p = NULL;
    fbe_packet_attr_t               packet_attr;
    fbe_payload_block_operation_t * block_operation_p = NULL;
    fbe_object_id_t object_id;
    fbe_lifecycle_state_t local_state;

    payload_p = fbe_transport_get_payload_ex(packet_p);
    block_operation_p = fbe_payload_ex_get_present_block_operation(payload_p);

    /* We do not expect one of these encryption related errors at run time. 
     * The only recovery action we know of is to panic since it might clear the state.
     */
    if (block_operation_p->status == FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED){

        fbe_base_object_get_object_id((fbe_base_object_t*)provision_drive_p, &object_id);
        if ( ((block_operation_p->status_qualifier == FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_KEY_WRAP_ERROR) ||
              (block_operation_p->status_qualifier == FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_BAD_KEY_HANDLE) ||
              (block_operation_p->status_qualifier == FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_ENCRYPTION_NOT_ENABLED) ) &&
             !fbe_transport_is_monitor_packet(packet_p, object_id) ) {
            fbe_scheduler_hook_status_t             hook_status;
            
            fbe_base_object_trace((fbe_base_object_t *)provision_drive_p, FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                  "provision_drive: encryption error lba: 0x%llx blocks: 0x%llx opcode: 0x%x\n", 
                                  block_operation_p->lba, block_operation_p->block_count, block_operation_p->block_opcode);
    
            fbe_base_object_trace((fbe_base_object_t *)provision_drive_p, FBE_TRACE_LEVEL_CRITICAL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                  "provision_drive: unexpected encryption error block_status: 0x%x block_qualifier: 0x%x\n", 
                                  block_operation_p->status, block_operation_p->status_qualifier);
    
            /* In simulation we disable critical errors, so we can arrive here. */
            fbe_provision_drive_check_hook(provision_drive_p,
                                           SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_ENCRYPTION_IO, 
                                           fbe_provision_drive_get_encryption_qual_substate(block_operation_p->status_qualifier), 
                                           0,  &hook_status);
        }
    }

    status = fbe_transport_get_status_code(packet_p);
    if (status == FBE_STATUS_OK)
    {
        return FBE_STATUS_OK;
    }

    /* We got bad status */
    switch (status)
    {
        case FBE_STATUS_DEAD:
        case FBE_STATUS_FAILED:
        case FBE_STATUS_EDGE_NOT_ENABLED:
        case FBE_STATUS_NO_DEVICE:
        case FBE_STATUS_NO_OBJECT:
            break;
        default:
            return FBE_STATUS_OK;
    }

    if (!fbe_provision_drive_is_slf_enabled() ||
        !fbe_base_config_is_peer_present((fbe_base_config_t *)provision_drive_p))
    {
        return FBE_STATUS_OK;
    }

    /* Object may fail during IO. */
    fbe_base_object_get_lifecycle_state((fbe_base_object_t *)provision_drive_p, &local_state);
    if (local_state != FBE_LIFECYCLE_STATE_READY)
    {
        return FBE_STATUS_OK;
    }
    fbe_payload_ex_get_iots(payload_p, &iots_p);
    fbe_transport_get_packet_attr(packet_p, &packet_attr);
    fbe_base_object_get_object_id((fbe_base_object_t*)provision_drive_p, &object_id);

    if (!fbe_provision_drive_is_local_state_set(provision_drive_p, FBE_PVD_LOCAL_STATE_SLF) &&
        !(packet_attr & FBE_PACKET_FLAG_DO_NOT_QUIESCE) &&
        !(fbe_transport_is_monitor_packet(packet_p, object_id)) &&
        ((packet_attr & FBE_PACKET_FLAG_REDIRECTED) != FBE_PACKET_FLAG_REDIRECTED) &&
        !fbe_payload_block_is_flag_set(block_operation_p, FBE_PAYLOAD_BLOCK_OPERATION_FLAGS_METADATA_OP) )
    {
        /* Initialize iots before queueing. */
        fbe_provision_drive_utils_initialize_iots(provision_drive_p, packet_p);

        /* Mark it quiesced so the monitor will restart it. */
        fbe_raid_iots_transition_quiesced(iots_p, fbe_provision_drive_block_transport_restart_io, provision_drive_p);

        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_set_cancel_function(packet_p, fbe_base_object_packet_cancel_function, (fbe_base_object_t *) provision_drive_p);
        fbe_transport_set_completion_function(packet_p, fbe_provision_drive_packet_completion_check_slf, provision_drive_p);
        status = fbe_base_object_add_to_terminator_queue((fbe_base_object_t *)provision_drive_p, packet_p);

        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p, 
                              FBE_TRACE_LEVEL_INFO, 
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s:quiesce queued packet: %p\n", __FUNCTION__, packet_p);

        /* This condition should already be set. */
        status = fbe_lifecycle_set_cond(&fbe_provision_drive_lifecycle_const,
                                        (fbe_base_object_t*)provision_drive_p,
                                        FBE_PROVISION_DRIVE_LIFECYCLE_COND_EVAL_SLF);

        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }

    if ((packet_attr & FBE_PACKET_FLAG_SLF_REQUIRED) && 
        fbe_provision_drive_slf_need_redirect(provision_drive_p))
    {
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p, 
                              FBE_TRACE_LEVEL_INFO, 
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s:redirect packet: %p\n", __FUNCTION__, packet_p);
        fbe_transport_clear_packet_attr(packet_p, FBE_PACKET_FLAG_SLF_REQUIRED);
        fbe_provision_drive_slf_send_packet_to_peer(provision_drive_p, packet_p);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }

    return FBE_STATUS_OK;
}
/*********************************************
 * end fbe_provision_drive_packet_completion_check_slf()
 *********************************************/

/*!****************************************************************************
 * @fn fbe_provision_drive_send_to_cmi_completion()
 ******************************************************************************
 * @brief
 *  This function checks packet status after it is returned from fbe CMI.
 *
 * @param packet_p - Pointer the the I/O packet.
 * @param context - Context of completion function.
 *
 * @return fbe_status_t.
 *
 * @author
 *  06/29/2012 - Created. Lili Chen
 *
 ******************************************************************************/
fbe_status_t 
fbe_provision_drive_send_to_cmi_completion(fbe_packet_t * packet_p, fbe_packet_completion_context_t context)
{
    fbe_provision_drive_t * provision_drive_p = (fbe_provision_drive_t *)context;
    fbe_status_t status;
    fbe_payload_ex_t * payload_p = NULL;
    fbe_payload_block_operation_t * block_operation_p = NULL;
    fbe_raid_iots_t * iots_p = NULL;
    fbe_lifecycle_state_t   local_state;

    status = fbe_transport_get_status_code(packet_p);
    if (status == FBE_STATUS_OK)
    {
        return FBE_STATUS_OK;
    }

    /* We got bad status */
    payload_p = fbe_transport_get_payload_ex(packet_p);
    block_operation_p = fbe_payload_ex_get_block_operation(payload_p);
    switch (status)
    {
        case FBE_STATUS_NO_OBJECT:
            fbe_transport_set_status(packet_p, FBE_STATUS_FAILED, 0);
            fbe_payload_block_set_status(block_operation_p, 
                                         FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED,
                                         FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_NOT_POSSIBLE);
            /* Fall through. */
        case FBE_STATUS_DEAD:
        case FBE_STATUS_FAILED:
        case FBE_STATUS_EDGE_NOT_ENABLED:
        case FBE_STATUS_NO_DEVICE:
            fbe_base_object_trace((fbe_base_object_t *)provision_drive_p, 
                                  FBE_TRACE_LEVEL_INFO, 
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s: packet: %p returned from CMI, status %d\n", __FUNCTION__, packet_p, status);
            /* Go to disabled immediately, since we lost the PVD on other SP.
             * We do not know if the peer is in activate or fail, so start with disabled, and 
             * ds health disabled with transition to fail if needed. 
             */
            status = fbe_lifecycle_set_cond(&fbe_provision_drive_lifecycle_const,
                                            (fbe_base_object_t*)provision_drive_p,
                                            FBE_BASE_CONFIG_LIFECYCLE_COND_DOWNSTREAM_HEALTH_DISABLED);
            return FBE_STATUS_OK;
    }

    /* Object may fail during IO. */
    fbe_base_object_get_lifecycle_state((fbe_base_object_t *)provision_drive_p, &local_state);
    if (local_state != FBE_LIFECYCLE_STATE_READY)
    {
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p, 
                              FBE_TRACE_LEVEL_INFO, 
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s: packet: %p returned from CMI, local_state %d\n", __FUNCTION__, packet_p, local_state);
        /* PVD must be in pending state. */
        fbe_transport_set_status(packet_p, FBE_STATUS_FAILED, 0);
        fbe_payload_block_set_status(block_operation_p, 
                                     FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED,
                                     FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_NOT_POSSIBLE);
        return FBE_STATUS_OK;
    }

    if (status == FBE_STATUS_QUIESCED)
    {
        if (!fbe_base_config_is_any_clustered_flag_set((fbe_base_config_t *) provision_drive_p, 
            (FBE_BASE_CONFIG_CLUSTERED_FLAG_QUIESCING|FBE_BASE_CONFIG_CLUSTERED_FLAG_QUIESCED)))
        {
            /* We may just unquiesced. Send again. */
            fbe_base_object_trace((fbe_base_object_t *)provision_drive_p, 
                                  FBE_TRACE_LEVEL_INFO, 
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s: send packet: %p returned from CMI, retries in 500ms\n", __FUNCTION__, packet_p);
            /* Clear the flag since peer may also enter SLF, the packet would re-enter fbe_provision_drive_handle_io() */
            packet_p->resource_priority_boost &= ~FBE_TRANSPORT_RESOURCE_PRIORITY_BOOST_FLAG_PVD;
            fbe_transport_set_timer(packet_p, 500, fbe_provision_drive_handle_io_check_slf, context);
                                     
            return FBE_STATUS_MORE_PROCESSING_REQUIRED;
        }

        //payload_p = fbe_transport_get_payload_ex(packet_p);
        fbe_payload_ex_get_iots(payload_p, &iots_p);

        /* Initialize iots before queueing. */
        fbe_provision_drive_utils_initialize_iots(provision_drive_p, packet_p);

        /* Mark it quiesced so the monitor will restart it. */
        fbe_raid_iots_transition_quiesced(iots_p, fbe_provision_drive_block_transport_restart_io, provision_drive_p);

        /* Terminates the packet until we transition to zero on demand mode, after transition it will restart
         * the zeroing operation.
         */
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_set_cancel_function(packet_p, fbe_base_object_packet_cancel_function, (fbe_base_object_t *) provision_drive_p);
        fbe_transport_set_completion_function(packet_p, fbe_provision_drive_packet_completion_check_slf, provision_drive_p);
        status = fbe_base_object_add_to_terminator_queue((fbe_base_object_t *)provision_drive_p, packet_p);

        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p, 
                              FBE_TRACE_LEVEL_INFO, 
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s:quiesce queued packet: %p returned from CMI\n", __FUNCTION__, packet_p);

        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }

    return FBE_STATUS_OK;
}
/*********************************************
 * end fbe_provision_drive_send_to_cmi_completion()
 *********************************************/

/*!****************************************************************************
 * @fn fbe_provision_drive_handle_io_check_slf()
 ******************************************************************************
 * @brief
 *  This function is the entry point for block transport operations to the
 *  provision drive object, it is called only from the provision drive block
 *  transport entry. It checks whether the packet needs to be redirected. If
 *  not, it calls fbe_provision_drive_handle_io to handle the packet.
 *
 * @param provision_drive - Pointer to the provision drive object.
 * @param packet          - Pointer the the I/O packet.
 *
 * @return fbe_status_t.
 *
 * @author
 *  09/04/2012 - Created. Lili Chen
 *
 ******************************************************************************/
static fbe_status_t fbe_provision_drive_handle_io_check_slf(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_status_t                            status;
    fbe_provision_drive_t                  *provision_drive = NULL;
    fbe_payload_block_operation_opcode_t    block_opcode;
    fbe_payload_ex_t				       *payload = NULL;
    fbe_payload_block_operation_t          *block_operation_p = NULL;

    provision_drive = (fbe_provision_drive_t *)context;
    payload = fbe_transport_get_payload_ex(packet);
    block_operation_p = fbe_payload_ex_get_block_operation(payload); /* It was get_present here. Why? */
    fbe_payload_block_get_opcode(block_operation_p, &block_opcode);

    /* Check for SLF */
    if (fbe_provision_drive_slf_need_redirect(provision_drive))
    {
        fbe_provision_drive_slf_send_packet_to_peer(provision_drive, packet);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }

    /* For normal path */
    fbe_transport_set_completion_function(packet, fbe_provision_drive_packet_completion_check_slf, provision_drive);
    status = fbe_provision_drive_handle_io(packet, provision_drive);
    return status;
}
/******************************************************************************
 * end fbe_provision_drive_handle_io_check_slf()
 ******************************************************************************/

/*!****************************************************************************
 * @fn fbe_provision_drive_handle_io()
 ******************************************************************************
 * @brief
 *  This function is the entry point for block transport operations to the
 *  provision drive object, it is called only from the provision drive block
 *  transport entry.
 *
 * @param provision_drive - Pointer to the provision drive object.
 * @param packet          - Pointer the the I/O packet.
 *
 * @return fbe_status_t.
 *
 * @author
 *  4/15/2009 - Created. Dhaval Patel
 *
 ******************************************************************************/
static fbe_status_t fbe_provision_drive_handle_io(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_status_t                            status;
    fbe_provision_drive_t                  *provision_drive = NULL;
    fbe_lba_t                               zero_checkpoint;
    fbe_lba_t                               exported_capacity;
    fbe_payload_block_operation_opcode_t    block_opcode;
    fbe_payload_ex_t				       *payload = NULL;
    fbe_payload_block_operation_t          *block_operation_p = NULL;
    fbe_packet_attr_t                       packet_attr;
    fbe_lba_t								lba;
    fbe_block_count_t						block_count;
    fbe_provision_drive_config_type_t       config_type;

    provision_drive = (fbe_provision_drive_t *)context;
    payload = fbe_transport_get_payload_ex(packet);
    block_operation_p = fbe_payload_ex_get_block_operation(payload); /* It was get_present here. Why? */
    fbe_payload_block_get_opcode(block_operation_p, &block_opcode);

    fbe_provision_drive_set_key_handle(provision_drive, packet);

    fbe_provision_drive_get_config_type(provision_drive, &config_type);
    if (config_type == FBE_PROVISION_DRIVE_CONFIG_TYPE_EXT_POOL) {
        /* This I/O can be send directly to PDO */
        fbe_transport_set_completion_function(packet, fbe_provision_drive_send_functional_packet_for_object_completion, provision_drive);
        return FBE_STATUS_CONTINUE;
    }
    
    /* If system object needs to do a nonpage access, it can't be stopped, and not stripe lock needed */
    /* Unless FBE_PAYLOAD_BLOCK_OPERATION_FLAGS_FORCED_WRITE is true, in which case we have to update PVD metadata
     *   and can't bypass the PVD */
    fbe_transport_get_packet_attr(packet, &packet_attr);
    if((packet_attr & FBE_PACKET_FLAG_DO_NOT_STRIPE_LOCK) &&
       !fbe_payload_block_is_flag_set(block_operation_p, FBE_PAYLOAD_BLOCK_OPERATION_FLAGS_FORCED_WRITE) &&
       ((block_opcode != FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_UNMARK_ZERO) &&
        (block_opcode != FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_CHECK_ZEROED) &&
        (block_opcode != FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ZERO))){
        /* This I/O can be send directly to PDO */
        fbe_transport_set_completion_function(packet, fbe_provision_drive_send_functional_packet_for_object_completion, provision_drive);
        return FBE_STATUS_CONTINUE;			
    }

    fbe_provision_drive_metadata_get_background_zero_checkpoint(provision_drive, &zero_checkpoint);
    fbe_base_config_get_capacity((fbe_base_config_t *)provision_drive, &exported_capacity );
    fbe_payload_block_get_lba(block_operation_p, &lba);
    fbe_payload_block_get_block_count(block_operation_p, &block_count);

#if 0
    if((lba >= exported_capacity) && (block_opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_VERIFY)) {
        fbe_scheduler_hook_status_t             hook_status;
        fbe_provision_drive_check_hook(provision_drive,
                                       SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_BLOCK_OPERATION,
                                       FBE_PROVISION_DRIVE_SUBSTATE_BLOCK_OPERATION_CHECK,
                                       block_opcode, &hook_status);
    }

#endif

    /* special case for remap ios - there is no need to go through zod for unmapped drives */
    if (zero_checkpoint > lba + block_count &&  
        fbe_provision_drive_is_unmap_supported(provision_drive) &&
        block_opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_VERIFY &&
        (packet_attr & FBE_PACKET_FLAG_BACKGROUND_SNIFF) &&
        !fbe_payload_block_is_flag_set(block_operation_p, FBE_PAYLOAD_BLOCK_OPERATION_FLAGS_FORCED_WRITE) && /* A forced write will need to go through the PVD in order to update the consumed bits. */
        lba < exported_capacity)
    {
        fbe_transport_set_completion_function(packet, fbe_provision_drive_send_functional_packet_for_object_completion, provision_drive);
        return FBE_STATUS_CONTINUE; 
    }

    /* If zero checkpoint reached metadata start LBA we do not need to take a lock anymore */
    if((zero_checkpoint > lba + block_count) && fbe_provision_drive_unmap_bitmap_is_lba_range_mapped(provision_drive, lba, block_count)){
        //requires_sg = fbe_payload_block_operation_opcode_requires_sg(block_opcode);        
        if((block_opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE || 
            block_opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_NONCACHED ||
            block_opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_VERIFY ||
            block_opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_VERIFY_WRITE ||
            block_opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ) && 
            !fbe_payload_block_is_flag_set(block_operation_p, FBE_PAYLOAD_BLOCK_OPERATION_FLAGS_FORCED_WRITE) && /* A forced write will need to go through the PVD in order to update the consumed bits. */
            lba < exported_capacity){
            /* This I/O can be send directly to LDO */
            fbe_transport_set_completion_function(packet, fbe_provision_drive_send_functional_packet_for_object_completion, provision_drive);
            return FBE_STATUS_CONTINUE;			
        }
    }

    /* Need to determine the type of request and possibly allocate a sub-packet. */
    status = fbe_provision_drive_process_io(provision_drive, packet);
    return status;
}
/******************************************************************************
 * end fbe_provision_drive_handle_io()
 ******************************************************************************/

/*!**************************************************************
 * fbe_provision_drive_rba_trace()
 ****************************************************************
 * @brief
 *  Trace out rba information for the pvd.
 *
 * @param provision_drive
 * @param block_operation_p
 *
 * @return None.   
 *
 * @author
 *  9/19/2012 - Created. Rob Foley
 *
 ****************************************************************/
void fbe_provision_drive_rba_trace(fbe_provision_drive_t * provision_drive,
                                   fbe_payload_block_operation_t *block_operation_p)
{
    if (fbe_traffic_trace_is_enabled(KTRC_TPVD))
    {
        fbe_provision_drive_nonpaged_metadata_drive_info_t nonpaged_drive_info;
        fbe_u64_t fru;
        fbe_u32_t op;

        /* get port, enclosure and slot information and encode:
         * fru = 2 byte port | 2 byte enclosure | 2 byte slot 
         */
        fbe_provision_drive_metadata_get_nonpaged_metadata_drive_info(provision_drive, &nonpaged_drive_info);
        fru = (nonpaged_drive_info.port_number & 0xFFFF);
        fru <<= 16;
        fru |= (nonpaged_drive_info.enclosure_number & 0xFFFF);
        fru <<= 16;
        fru |= (nonpaged_drive_info.slot_number & 0xFFFF);

        switch(block_operation_p->block_opcode) 
        {
            case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ:
                op = KT_TRAFFIC_READ_START;
                break;
            case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE:
                op = KT_TRAFFIC_WRITE_START;
                break;
            case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ZERO:
                op = KT_TRAFFIC_ZERO_START;
                break;
            default:
                op = KT_TRAFFIC_NO_OP;
        }

        if (op != KT_TRAFFIC_NO_OP)
        {
            KTRACEX(TRC_K_TRAFFIC,
                    KT_FBE_PVD_TRAFFIC,
                    (fbe_u64_t)block_operation_p->lba,
                    (fbe_u64_t)0,
                    (fbe_u64_t)block_operation_p->block_count,
                    (fru << 16) | op);
        }
    }
}
/******************************************
 * end fbe_provision_drive_rba_trace()
 ******************************************/
/*!****************************************************************************
 * @fn fbe_provision_drive_block_transport_handle_io_get_stripe_lock_if_needed()
 ******************************************************************************
 * @brief
 *  This function is called to determine whether we need stripe lock at pvd
 *  object level before we process the i/o operation.
 *
 * @param provision_drive - Pointer to the provision drive object.
 * @param packet          - Pointer the the I/O packet.
 *
 * @return fbe_status_t.
 *
 * @author
 *  6/21/2010 - Created. Dhaval Patel
 *
 ******************************************************************************/
static fbe_status_t 
fbe_provision_drive_block_transport_handle_io_get_stripe_lock_if_needed(fbe_provision_drive_t * provision_drive, 
                                                                        fbe_packet_t * packet)
{
    fbe_payload_ex_t *                     sep_payload_p = NULL;
    fbe_payload_block_operation_t *         block_operation_p = NULL;
    fbe_bool_t                              is_zero_on_demand_set_b = FBE_FALSE;
    fbe_bool_t                              is_stripe_lock_needed_b = FBE_FALSE;
    fbe_raid_iots_t *                       iots_p = NULL;
    fbe_payload_block_operation_opcode_t    block_opcode;	

    /* Get the payload */
    sep_payload_p = fbe_transport_get_payload_ex(packet);
    block_operation_p = fbe_payload_ex_get_present_block_operation(sep_payload_p);
    fbe_payload_block_get_opcode(block_operation_p, &block_opcode);
    fbe_payload_ex_get_iots(sep_payload_p, &iots_p);

    /* if it is not quiesce then set the iots status as not used. */
    fbe_raid_iots_set_as_not_used(iots_p);
    
    /* Determine if we need stripe lock. */
    fbe_provision_drive_is_stripe_lock_needed(provision_drive, block_operation_p, &is_stripe_lock_needed_b);
    
    /* Determine whether zero on demand flag is already set. */
    /*!@todo we might need to acquire distributed lock (ideally quiesce should take care of it). */
    //fbe_provision_drive_metadata_get_zero_on_demand_flag(provision_drive, &is_zero_on_demand_set_b);

    /* If the `forced write' flag is set, we need to execute a zero on demand.
     */
    if (fbe_payload_block_is_flag_set(block_operation_p, FBE_PAYLOAD_BLOCK_OPERATION_FLAGS_FORCED_WRITE))
    {
        is_zero_on_demand_set_b = FBE_TRUE;
    }

    /* trace to RBA buffer */
    fbe_provision_drive_rba_trace(provision_drive, block_operation_p);

    /* If packet attribute is FBE_PACKET_FLAG_CONSUMED we do not need stripe lock and paged access */
    if(packet->packet_attr & FBE_PACKET_FLAG_CONSUMED){ /* Client guarantee that this area is already consumed (written to) */
        is_zero_on_demand_set_b = FBE_FALSE;
        is_stripe_lock_needed_b = FBE_FALSE;
    }

    /*!@todo zero on demand flag is not persistent currenty with metadata service and so on reboot 
     * zero on demand flag will be cleared and so to avoid any issue with current infrastructure we
     * will always acquire the stripe lock.
     */
    if(is_stripe_lock_needed_b) {
        /* If zero on demand flag is set then all the incoming i/o request goes through acquiring stripe lock. */
        fbe_transport_set_completion_function(packet, fbe_provision_drive_handle_io_with_lock, provision_drive);
        fbe_transport_set_completion_function(packet, fbe_provision_drive_acquire_stripe_lock, provision_drive);		
    } else {
        fbe_transport_set_completion_function(packet, fbe_provision_drive_handle_io_without_lock, provision_drive);		
    }
    
    return FBE_STATUS_CONTINUE;
}


/******************************************************************************
 * end fbe_provision_drive_block_transport_handle_io_get_stripe_lock_if_needed()
 ******************************************************************************/

/*!****************************************************************************
 * @fn fbe_provision_drive_handle_io_with_lock()
 ******************************************************************************
 * @brief
 *  This function is used to handle the completion of acquiring stripe lock
 *  operation, after acquiring stripe lock it sends i/o request to different
 *  code path based on the block operation code.
 *
 * @param packet                      - Pointer to the packet.
 * @param packet_completion_context     - Packet completion context.
 *
 * @return fbe_status_t                 - status of the operation.
 *
 * @author
 *  21/06/2010 - Created. Dhaval Patel
 *
 ******************************************************************************/
fbe_status_t
fbe_provision_drive_handle_io_with_lock(fbe_packet_t * packet,
                                                        fbe_packet_completion_context_t context)
{
    fbe_status_t                            status;
    fbe_provision_drive_t *                 provision_drive = (fbe_provision_drive_t *) context;
    fbe_payload_ex_t *                     sep_payload_p = NULL;
    fbe_payload_block_operation_status_t    block_status;
    fbe_payload_block_operation_qualifier_t    block_qual;
    fbe_payload_block_operation_t *         block_operation_p = NULL;

   /* get the sep payload and block operation. */
    sep_payload_p = fbe_transport_get_payload_ex(packet);
    block_operation_p = fbe_payload_ex_get_present_block_operation(sep_payload_p);

    status = fbe_transport_get_status_code(packet);
    fbe_payload_block_get_status(block_operation_p, &block_status);
    fbe_payload_block_get_qualifier(block_operation_p, &block_qual);
    if((status != FBE_STATUS_OK) || (block_status != FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS)) {
        fbe_provision_drive_utils_trace(provision_drive,
                                        FBE_TRACE_LEVEL_WARNING,
                                        FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                        FBE_PROVISION_DRIVE_DEBUG_FLAG_GENERAL_TRACING,
                                        "pvd_handle_io_with_lock: SL failed stat=0x%x block_stat=0x%x bl_qual: 0x%x\n",
                                        status, block_status, block_qual);
        return status;
    }

    /* set the completion function to release the stripe lock before start i/o. */
    fbe_transport_set_completion_function(packet,
                                          fbe_provision_drive_block_transport_release_stripe_lock,
                                          provision_drive);

    /* send the request to process the i/o operation after acquiring stripe lock. */
    //status = fbe_provision_drive_handle_io_without_lock(packet, provision_drive);
    //return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    fbe_transport_set_completion_function(packet, fbe_provision_drive_handle_io_without_lock, provision_drive);	
    return FBE_STATUS_CONTINUE;
}
/******************************************************************************
 * end fbe_provision_drive_handle_io_with_lock()
 ******************************************************************************/

/*!****************************************************************************
 * @fn fbe_provision_drive_block_transport_release_stripe_lock()
 ******************************************************************************
 * @brief
 *  This function is used to release the stripe lock, it will be called only if
 *  we have acquired stripe lock before starting i/o operation.
 *
 *  It will be called after the compleiton of the i/o operation.
 *   
 * @param provision_drive     - Pointer to the provision drive object.
 * @param packet              - Pointer to the packet.
 *
 * @return fbe_status_t         - status of the operation.
 *
 * @author
 *  21/06/2010 - Created. Dhaval Patel
 *
 ******************************************************************************/
fbe_status_t
fbe_provision_drive_block_transport_release_stripe_lock(fbe_packet_t * packet,
                                                        fbe_packet_completion_context_t context)
{
    fbe_provision_drive_t * provision_drive = (fbe_provision_drive_t *) context;
    fbe_status_t            status;

    /* release the stripe lock before we complete the block operation. */
    status = fbe_provision_drive_release_stripe_lock(provision_drive, packet);
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/******************************************************************************
 * end fbe_provision_drive_block_transport_release_stripe_lock()
 ******************************************************************************/

/*!****************************************************************************
 * @fn fbe_provision_drive_handle_io_without_lock()
 ******************************************************************************
 * @brief
 *  This function is used to handle the block tranport operation without lock,
 *  if caller does not require stripe lock then it calls directly this function
 *  to process i/o except it goes through acquiring stripe lock and then it
 *  calls this routine to process the i/o operation.

 *
 * @param packet                      - Pointer to the packet.
 * @param packet_completion_context     - Packet completion context.
 *
 * @return fbe_status_t                 - status of the operation.
 *
 * @author
 *  21/06/2010 - Created. Dhaval Patel
 *
 ******************************************************************************/
static fbe_status_t
fbe_provision_drive_handle_io_without_lock(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_payload_ex_t *                     sep_payload_p = NULL;
    fbe_payload_block_operation_t *         block_operation_p = NULL;
    fbe_payload_block_operation_opcode_t    block_operation_opcode = FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INVALID;
    fbe_lba_t                               lba = 0;
    fbe_bool_t								is_stripe_lock_needed;
    fbe_provision_drive_t * provision_drive = (fbe_provision_drive_t *)context;
   /* Get the sep payload and block operation. */
    sep_payload_p = fbe_transport_get_payload_ex(packet);
    block_operation_p = fbe_payload_ex_get_present_block_operation(sep_payload_p);

    fbe_payload_block_get_lba(block_operation_p, &lba);

    /* First, get the opcodes of the block operation. */
    fbe_payload_block_get_opcode(block_operation_p, &block_operation_opcode);

    /* Determine if we need stripe lock. */
    fbe_provision_drive_is_stripe_lock_needed(provision_drive, block_operation_p, &is_stripe_lock_needed);

    switch (block_operation_opcode)
    {
            case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ZERO:
            case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_MARK_DISK_ZERO:
            /* Zero opcode will be handled by the provision drive object, it updates the metadata 
             * for the chunk aligned zero request and for the edges which is not aligned to chunk
             * it sends write same request to perform zeroing operation.
             */
            status = fbe_provision_drive_user_zero_handle_request(provision_drive, packet);
            status = FBE_STATUS_MORE_PROCESSING_REQUIRED;
            break;

            case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_MARK_CONSUMED_ZERO:
            status = fbe_provision_drive_consumed_area_zero_handle_request(provision_drive, packet);
            status = FBE_STATUS_MORE_PROCESSING_REQUIRED;
            break;
            
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_BACKGROUND_ZERO:
            /* Background zero opcode will look at the paged metadata and perform the zeroing operation
             * if required, it does this under stripe lock and while updating nonpaged metadata it
             * will acquire the distributed lock.
             */
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_VERIFY_INVALIDATE:
            /* Verify invalidate opcode will look at the paged metadata and invalidate the user region
             * if required, it does this under stripe lock and while updating nonpaged metadata it
             * will acquire the distributed lock. Piggy back on the background zero because the process
             * is the same instead of doing zeroing this will invalidate the region as needed
             */
            status = fbe_provision_drive_background_zero_process_background_zero_request(provision_drive, packet);
            status = FBE_STATUS_MORE_PROCESSING_REQUIRED;
            break;

        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_VERIFY:
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_NEGOTIATE_BLOCK_SIZE:
            fbe_transport_set_completion_function(packet, fbe_provision_drive_send_packet_completion, provision_drive);
            status = FBE_STATUS_CONTINUE;
            break;
            
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_CHECK_ZEROED:
            status = fbe_provision_drive_zero_on_demand_process_io_request(provision_drive, packet);			
            break;

        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_MARK_CONSUMED:
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_UNMARK_ZERO:
            /* This block operation will mark provision drive's paged metadata as consumed.
             */
            status = fbe_provision_drive_metadata_mark_consumed(provision_drive, packet);
            status = FBE_STATUS_MORE_PROCESSING_REQUIRED;
            break;            

        default:
            /* If packet attribute is FBE_PACKET_FLAG_CONSUMED we do not need stripe lock and paged access */
            if(packet->packet_attr & FBE_PACKET_FLAG_CONSUMED){ /* Client guarantee that this area is already consumed (written to) */
                is_stripe_lock_needed = FBE_FALSE;
            }

            if(fbe_payload_block_operation_opcode_is_media_modify(block_operation_opcode) ||
               fbe_payload_block_operation_opcode_is_media_read(block_operation_opcode))
            {
                /* process the incoming read/write i/o request. */
                if(is_stripe_lock_needed){
                    status = fbe_provision_drive_zero_on_demand_process_io_request(provision_drive, packet);
                } else {
                    fbe_transport_set_completion_function(packet, fbe_provision_drive_send_packet_completion, provision_drive);
                    status = FBE_STATUS_CONTINUE;
                }
                
            }
            else
            {
                fbe_provision_drive_utils_trace(provision_drive,
                                                FBE_TRACE_LEVEL_ERROR,
                                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                                FBE_PROVISION_DRIVE_DEBUG_FLAG_GENERAL_TRACING,
                                                "%s: unsupported block opcode, opcode:0x%x\n",
                                                __FUNCTION__, block_operation_opcode);
                fbe_payload_block_set_status(block_operation_p,
                                             FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID_REQUEST,
                                             FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_UNSUPPORTED_OPCODE);
                
                fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
                
                status = FBE_STATUS_OK;
            }			
            break;
    }

    return status;
}
/******************************************************************************
 * end fbe_provision_drive_handle_io_without_lock()
 ******************************************************************************/

fbe_status_t 
fbe_provision_drive_send_functional_packet_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_packet_t * new_packet = (fbe_packet_t *)context;
    fbe_payload_ex_t *                 payload = NULL;
    fbe_payload_block_operation_t * block_operation = NULL;
    fbe_block_edge_t *	block_edge = NULL;

    payload = fbe_transport_get_payload_ex(new_packet);
    fbe_payload_ex_increment_block_operation_level(payload);
    block_operation = fbe_payload_ex_get_block_operation(payload);
    block_edge = block_operation->block_edge_p;

    fbe_block_transport_send_functional_packet(block_edge, new_packet);
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/*!**************************************************************
 * fbe_provision_drive_send_functional_packet_for_object_completion()
 ****************************************************************
 * @brief
 *  Simply send the input packet to the block edge.
 *
 * @param packet - Packet to send to the edge.
 * @param context - Provision drive.
 *
 * @return fbe_status_t   
 *
 * @author
 *  1/10/2012 - Created. Rob Foley
 *
 ****************************************************************/
static fbe_status_t 
fbe_provision_drive_send_functional_packet_for_object_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_provision_drive_t * provision_drive = (fbe_provision_drive_t *)context;
    fbe_block_edge_t *	block_edge = NULL;
    fbe_base_config_get_block_edge((fbe_base_config_t*)provision_drive, &block_edge, 0);

    fbe_block_transport_send_functional_packet(block_edge, packet);
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/******************************************
 * end fbe_provision_drive_send_functional_packet_for_object_completion()
 ******************************************/

/*!****************************************************************************
 * fbe_provision_drive_allocate_packet_master_completion()
 ******************************************************************************
 * @brief
 *  This function is used to handle the completion for the subpacket for the
 *  the provisiion drive object.
 * 
 * @note
 *  Provision drive object should use this function whenever it creates
 *  subpacket and send it to the downstream edge, it is to make sure that error
 *  precedence take into account while updating master packet status.
 *
 * @param   packet        - pointer to disk zeroing IO packet
 * @param   context         - context, which is a pointer to the pvd object.
 *
 * @return fbe_status_t.    - status of the operation.
 *
 * @author
 *   07/14/2010 - Created. Dhaval Patel
 *
 ******************************************************************************/
static fbe_status_t 
fbe_provision_drive_allocate_packet_master_completion(fbe_packet_t * packet,
                                                      fbe_packet_completion_context_t context)
{
    fbe_provision_drive_t *                 provision_drive = (fbe_provision_drive_t *)context;
    fbe_packet_t *                          master_packet_p = NULL;
    fbe_payload_ex_t *                      new_payload_p = NULL;
    fbe_payload_ex_t *                      sep_payload_p = NULL;
    fbe_payload_block_operation_t *         master_block_operation_p = NULL;
    fbe_payload_block_operation_t *         block_operation_p = NULL;
    //fbe_queue_head_t                    provision_drive_io_tmp_queue;
    //fbe_status_t                    status;
    fbe_bool_t								is_empty;

    /* get the master packet. */
    master_packet_p = fbe_transport_get_master_packet(packet);

    if (master_packet_p == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t *)provision_drive, 
                              FBE_TRACE_LEVEL_CRITICAL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s master packet is null packet: %p\n", __FUNCTION__, packet);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }
    sep_payload_p = fbe_transport_get_payload_ex(master_packet_p);
    new_payload_p = fbe_transport_get_payload_ex(packet);
    /* get the subpacket and master packet's block operation. */
    master_block_operation_p = fbe_payload_ex_get_present_block_operation(sep_payload_p);
    block_operation_p = fbe_payload_ex_get_block_operation(new_payload_p);

    /* update the master packet status only if it is required. */
    fbe_provision_drive_utils_process_block_status(provision_drive, master_packet_p, packet);

    /* remove the subpacket from master packet. */
    //fbe_transport_remove_subpacket(packet);
    fbe_transport_remove_subpacket_is_queue_empty(packet, &is_empty);

    /* release the block operation. */
    fbe_payload_ex_release_block_operation(new_payload_p, block_operation_p);

    /* destroy the packet. */
    //fbe_transport_destroy_packet(packet);

    /* when the queue is empty, all subpackets have completed. */
    if(is_empty)
    {
        fbe_transport_destroy_subpackets(master_packet_p);
        /* release the preallocated memory for the zero on demand request. */
        //fbe_memory_request_get_entry_mark_free(&master_packet_p->memory_request, &memory_ptr);
        //fbe_memory_free_entry(memory_ptr);
        fbe_memory_free_request_entry(&master_packet_p->memory_request);

        /* remove the master packet from terminator queue and complete it. */
        fbe_base_object_remove_from_terminator_queue((fbe_base_object_t *) provision_drive, master_packet_p);
        /* After removing the master packet, unmark it as in progress */

#if 0
        /* initialize the queue. */
        fbe_queue_init(&provision_drive_io_tmp_queue);

        /* enqueue this packet temporary queue before we enqueue it to run queue. */
        status = fbe_transport_enqueue_packet(master_packet_p, &provision_drive_io_tmp_queue);

        /*!@note Que      ue this request to run queue to break the i/o context to avoid the stack
        * overflow, later this needs to be queued with the thread running in same core.
        */
        fbe_transport_run_queue_push(&provision_drive_io_tmp_queue,  NULL,  NULL);
#else

        fbe_provision_drive_rba_trace(provision_drive, block_operation_p);

        fbe_transport_complete_packet(master_packet_p);
#endif


    }
    else
    {
        /* not done with processing sub packets.
         */
    }
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/******************************************************************************
 * end fbe_provision_drive_allocate_packet_master_completion()
 ******************************************************************************/
/*!**************************************************************
 * fbe_provision_drive_metadata_read_completion()
 ****************************************************************
 * @brief
 *  This is the callback function after metadata read is completed and
 * all post validation is done here
 *
 * @param   packet        - pointer to disk zeroing IO packet
 * @param   context         - context, which is a pointer to the pvd object.
 * 
 * @return fbe_status_t 
 *
 * @author
 *  2/07/2012 - Created. Ashok Tamilarasan
 *
 ****************************************************************/
static fbe_status_t 
fbe_provision_drive_metadata_read_completion(fbe_packet_t * packet,
                                             fbe_packet_completion_context_t context)
{
    fbe_provision_drive_t *                 provision_drive = (fbe_provision_drive_t *)context;
    fbe_payload_ex_t *             sep_payload_p = NULL;
    fbe_payload_block_operation_t * block_operation_p = NULL;
    fbe_lba_t                               start_lba;
    fbe_block_count_t                       block_count;
    fbe_sg_element_t *                  sg_list = NULL;
    fbe_status_t                        status;
    fbe_payload_block_operation_status_t block_status;
    sep_payload_p = fbe_transport_get_payload_ex(packet);
    block_operation_p = fbe_payload_ex_get_present_block_operation(sep_payload_p);

    fbe_payload_block_get_status(block_operation_p, &block_status);
    if(block_status == FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS) 
    {
        /* Get the start lba of the block operation. */
        fbe_payload_block_get_lba(block_operation_p, &start_lba);
        fbe_payload_block_get_block_count(block_operation_p, &block_count);
    
        /* get sg list with new packet. */
        fbe_payload_ex_get_sg_list(sep_payload_p, &sg_list, NULL);
    
        status = fbe_provision_drive_validate_data(provision_drive,
                                                   sg_list, 
                                                   start_lba, 
                                                   (fbe_u32_t)block_count,
                                                   0);
        if(status != FBE_STATUS_OK) 
        {
            fbe_payload_block_set_status(block_operation_p,
                                         FBE_PAYLOAD_BLOCK_OPERATION_STATUS_MEDIA_ERROR,
                                         FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_MEDIA_ERROR_DATA_LOST);
        }
    }
    return FBE_STATUS_OK;
}

/******************************************************************************
 * end fbe_provision_drive_metadata_read_completion()
 ******************************************************************************/
/*!**************************************************************
 * fbe_provision_drive_validate_data()
 ****************************************************************
 * @brief
 *  The function validates the checksum and lba stamp on the data that was read.
 *
 * @param provision_drive - Pointer to the provision drive object
 * @param sg_p - Scatter gather list having the data.
 * @param lba - starting lba of the read.
 * @param blkcnt - Number of blocks to check.
 * @param offset - Offset in the start lba
 * 
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE 
 *                        for checksum and LBA stamp errors
 *
 * @author
 *  02/07/2012 - Created. Ashok Tamilarasan
 *  05/31/2012 - Ashok Tamilarasan - Modified to use execute stamp function
 *
 ****************************************************************/
fbe_status_t fbe_provision_drive_validate_data(fbe_provision_drive_t * provision_drive,
                                               fbe_sg_element_t *sg_p,
                                               fbe_lba_t lba,
                                               fbe_u32_t blocks,
                                               fbe_u32_t offset)
                                 
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_xor_execute_stamps_t execute_stamps;
    /* Check both lba stamp and checksum.
     */
    fbe_xor_lib_execute_stamps_init(&execute_stamps);
    execute_stamps.fru[0].sg = sg_p;
    execute_stamps.fru[0].seed = lba;
    execute_stamps.fru[0].count = blocks;
    execute_stamps.fru[0].offset = offset;
    execute_stamps.fru[0].position_mask = 0;

    /* Setup the drive count and eboard pointer.
     */
    execute_stamps.data_disks = 1;
    execute_stamps.array_width = 1;
    execute_stamps.eboard_p = NULL;
    execute_stamps.eregions_p = NULL;
    execute_stamps.option = FBE_XOR_OPTION_CHK_CRC | FBE_XOR_OPTION_CHK_LBA_STAMPS;
    status = fbe_xor_lib_execute_stamps(&execute_stamps); 

    /* Check the execute stamp status for errors */
    if(execute_stamps.status != FBE_XOR_STATUS_NO_ERROR)
    {
        status = FBE_STATUS_GENERIC_FAILURE;
    }

    if(status != FBE_STATUS_OK)
    {
        fbe_provision_drive_utils_trace(provision_drive,
                                        FBE_TRACE_LEVEL_WARNING,
                                        FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                        FBE_PROVISION_DRIVE_DEBUG_FLAG_GENERAL_TRACING,
                                        "%s: checksum or LBA stamp error in data. LBA:0x%llx, blocks:%d\n",
                                        __FUNCTION__, lba + offset, blocks);
    }
    
    return status;
}
/******************************************
 * end fbe_provision_drive_validate_data()
 ******************************************/
/*!**************************************************************
 * fbe_provision_drive_validate_checksum()
 ****************************************************************
 * @brief
 *  Check checksum on the data provided.
 *
 * @param provision_drive - Pointer to the provision drive object
 * @param sg_p - Scatter gather list having the data.
 * @param lba - starting lba of the read.
 * @param blkcnt - Number of blocks to check.
 * @param offset - Offset in the start lba
 * 
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE 
 *                        for checksum and LBA stamp errors
 *
 * @author
 *  2/13/2014 - Created from fbe_provision_drive_validate_data. Rob Foley 
 *
 ****************************************************************/
fbe_status_t fbe_provision_drive_validate_checksum(fbe_provision_drive_t * provision_drive,
                                                   fbe_sg_element_t *sg_p,
                                                   fbe_lba_t lba,
                                                   fbe_u32_t blocks,
                                                   fbe_u32_t offset)
                                 
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_xor_execute_stamps_t execute_stamps;
    /* Check both lba stamp and checksum.
     */
    fbe_xor_lib_execute_stamps_init(&execute_stamps);
    execute_stamps.fru[0].sg = sg_p;
    execute_stamps.fru[0].seed = lba;
    execute_stamps.fru[0].count = blocks;
    execute_stamps.fru[0].offset = offset;
    execute_stamps.fru[0].position_mask = 0;

    /* Setup the drive count and eboard pointer.
     */
    execute_stamps.data_disks = 1;
    execute_stamps.array_width = 1;
    execute_stamps.eboard_p = NULL;
    execute_stamps.eregions_p = NULL;
    execute_stamps.option = FBE_XOR_OPTION_CHK_CRC;
    status = fbe_xor_lib_execute_stamps(&execute_stamps); 

    /* Check the execute stamp status for errors */
    if(execute_stamps.status != FBE_XOR_STATUS_NO_ERROR)
    {
        status = FBE_STATUS_GENERIC_FAILURE;
    }

    if(status != FBE_STATUS_OK)
    {
        fbe_provision_drive_utils_trace(provision_drive,
                                        FBE_TRACE_LEVEL_WARNING,
                                        FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                        FBE_PROVISION_DRIVE_DEBUG_FLAG_GENERAL_TRACING,
                                        "%s: checksum error in data. LBA:0x%llx, blocks:%d\n",
                                        __FUNCTION__, lba + offset, blocks);
    }
    
    return status;
}
/******************************************
 * end fbe_provision_drive_validate_checksum()
 ******************************************/

/*!**************************************************************
 * fbe_provision_drive_append_lba_stamp()
 ****************************************************************
 * @brief
 *  The function appends the lba stamp to the sector
 *
 * @param provision_drive - Pointer to the provision drive object
 * @param sg_p - Scatter gather list having the data.
 * @param lba - starting lba of the read.
 * @param blkcnt - Number of blocks to check.
 * 
 * @return fbe_status_t - status
 *
 * @author
 *  05/31/2012 - Created. Ashok Tamilarasan
 *
 ****************************************************************/
static fbe_status_t fbe_provision_drive_append_lba_stamp(fbe_provision_drive_t * provision_drive,
                                                         fbe_sg_element_t *sg_p,
                                                         fbe_lba_t lba,
                                                         fbe_u32_t blocks)
                                 
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_xor_execute_stamps_t execute_stamps;

    /* Check both lba stamp and checksum.
     */
    fbe_xor_lib_execute_stamps_init(&execute_stamps);
    execute_stamps.fru[0].sg = sg_p;
    execute_stamps.fru[0].seed = lba;
    execute_stamps.fru[0].count = blocks;
    execute_stamps.fru[0].offset = 0;
    execute_stamps.fru[0].position_mask = 0;

    /* Setup the drive count and eboard pointer.
     */
    execute_stamps.data_disks = 1;
    execute_stamps.array_width = 1;
    execute_stamps.eboard_p = NULL;
    execute_stamps.eregions_p = NULL;
    execute_stamps.option = FBE_XOR_OPTION_GEN_LBA_STAMPS;
    status = fbe_xor_lib_execute_stamps(&execute_stamps); 

    return status;
}
/******************************************
 * end fbe_provision_drive_append_lba_stamp()
 ******************************************/


/*!****************************************************************************
 * fbe_provision_drive_send_monitor_packet
 ******************************************************************************
 *
 * @brief
 *  This function builds a block operation in the supplied monitor packet
 *  and sends the packet to the provision drive's executor. It is assumed
 *  that any completion function and associated function parameters  have
 *  already been established before calling this function.
 *
 * @param   in_provision_drive_p  -  pointer to provision drive
 * @param   in_packet_p           -  pointer to packet requesting block i/o
 * @param   in_opcode             -  i/o operation code
 * @param   in_start_lba          -  i/o start lba
 * @param   in_block_count        -  i/o count
 *
 * @return  fbe_status_t          -  status of this block i/o request
 *
 * @version
 *    02/12/2010 - Created. Randy Black
 *
 ******************************************************************************/

fbe_status_t
fbe_provision_drive_send_monitor_packet(fbe_provision_drive_t*               provision_drive,
                                        fbe_packet_t*                        in_packet_p,
                                        fbe_payload_block_operation_opcode_t in_opcode,
                                        fbe_lba_t                            in_start_lba,
                                        fbe_block_count_t                    in_block_count)
{
    fbe_payload_block_operation_t*  block_operation_p;  // block operation
    fbe_optimum_block_size_t        optimum_block_size; // optimium block size
    fbe_payload_ex_t*              sep_payload_p;      // sep payload
    fbe_status_t                    status;             // fbe status

    // get optimum block size for this i/o request
    fbe_block_transport_edge_get_optimum_block_size( &provision_drive->block_edge, &optimum_block_size );

    // get the sep payload
    sep_payload_p = fbe_transport_get_payload_ex( in_packet_p );

    // set-up block operation in sep payload
    block_operation_p = fbe_payload_ex_allocate_block_operation( sep_payload_p );

    // next, build block operation in sep payload
    fbe_payload_block_build_operation( block_operation_p,       // pointer to block operation in sep payload
                                       in_opcode,               // block i/o operation code
                                       in_start_lba,            // block i/o starting lba
                                       in_block_count,          // block i/o count
                                       FBE_BE_BYTES_PER_BLOCK,  // block size
                                       optimum_block_size,      // optimum block size
                                       NULL                     // no pre-read descriptor
                                     );

    // now, activate this block operation 
    fbe_payload_ex_increment_block_operation_level( sep_payload_p );

    /* If it is Background Zeroing or Sniff Verify, set priority to low. */
    if ((in_opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_BACKGROUND_ZERO) ||
        (in_opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_VERIFY_INVALIDATE))
    {
        fbe_transport_set_packet_priority(in_packet_p, FBE_PACKET_PRIORITY_LOW);
    }

    // invoke bouncer to forward i/o request to downstream object
    status = fbe_base_config_bouncer_entry( (fbe_base_config_t *)provision_drive, in_packet_p );
    if(status == FBE_STATUS_OK){
        status = fbe_provision_drive_block_transport_entry(provision_drive, in_packet_p);
    }

    return status;
}
/******************************************************************************
 * end fbe_provision_drive_send_monitor_packet()
 ******************************************************************************/

fbe_status_t 
fbe_provision_drive_set_key_handle(fbe_provision_drive_t * provision_drive, fbe_packet_t * packet)
{
    fbe_status_t						status;
    fbe_base_config_encryption_mode_t	encryption_mode;
    fbe_payload_ex_t					* sep_payload_p;
    fbe_payload_block_operation_t       * block_operation_p;
    fbe_lba_t                           exported_capacity;
    fbe_lba_t							lba;
    fbe_edge_index_t					server_index = 0;
    fbe_provision_drive_key_info_t      * key_info;
    fbe_object_id_t						object_id;
    fbe_bool_t							is_system_drive;
    fbe_bool_t                          is_monitor_packet;

    /* Retrieve payload and block operation */
    sep_payload_p = fbe_transport_get_payload_ex(packet);
    block_operation_p = fbe_payload_ex_get_block_operation(sep_payload_p);

    fbe_base_config_get_encryption_mode((fbe_base_config_t *) provision_drive, &encryption_mode);

    if ((encryption_mode != FBE_BASE_CONFIG_ENCRYPTION_MODE_ENCRYPTED) &&
        (encryption_mode != FBE_BASE_CONFIG_ENCRYPTION_MODE_ENCRYPTION_IN_PROGRESS) &&
        (encryption_mode != FBE_BASE_CONFIG_ENCRYPTION_MODE_REKEY_IN_PROGRESS)) {
        fbe_payload_ex_set_key_handle(sep_payload_p, FBE_INVALID_KEY_HANDLE);
        return FBE_STATUS_OK; /* There is nothing to do - we are not in encryption mode */
    }

    /* Encryption for system drives is temporary disabled */
    fbe_base_object_get_object_id((fbe_base_object_t *)provision_drive, &object_id);
    is_system_drive = fbe_database_is_object_system_pvd(object_id);
    is_monitor_packet = fbe_transport_is_monitor_packet(packet, object_id);

    fbe_payload_block_get_lba(block_operation_p, &lba);
    fbe_base_config_get_capacity((fbe_base_config_t *)provision_drive, &exported_capacity);
    /* Check if it is user I/O */
    if(lba < exported_capacity){ /* Code for success. It is user I/O */
        /* Monitor I/Os do not have a server index (it is 0). 
         * For system drives, determine the appropriate server index. 
         */
        if (is_monitor_packet) {
            if (is_system_drive) {
                /* If we have an edge that consumes this, then use that server index.
                 */
                fbe_block_transport_server_get_server_index_for_lba(&provision_drive->base_config.block_transport_server,
                                                                    lba, &server_index);
                if (server_index == FBE_EDGE_INDEX_INVALID) {
                    server_index = 0;
                }
            } else {
                /* Non system drives just use server index 0 for monitor ops.
                 */
                server_index = 0;
            }
        }
        else {
            if (is_system_drive && (packet->packet_attr & FBE_PACKET_FLAG_REDIRECTED)) {
                /* If we were redirected, we do not have an edge to get the server index from, so derive the server now.
                 */
                fbe_block_transport_server_get_server_index_for_lba(&provision_drive->base_config.block_transport_server,
                                                                    lba, &server_index);
                if (server_index == FBE_EDGE_INDEX_INVALID) {
                    server_index = 0;
                }
            } else {
                fbe_transport_get_server_index(packet, &server_index);
            }
        }

        status = fbe_provision_drive_get_key_info(provision_drive, server_index, &key_info);
        if(status != FBE_STATUS_OK){
            return status; /* Something is VERY WRONG. */
        }
        /* If we are in rekey and RAID told us to use the new key, then use key #2.
         * Otherwise use key number one. 
         */ 
        if (fbe_payload_block_is_flag_set(block_operation_p, FBE_PAYLOAD_BLOCK_OPERATION_FLAGS_USE_NEW_KEY)){
            fbe_payload_ex_set_key_handle(sep_payload_p, key_info->mp_key_handle_2);
        } else {
            fbe_payload_ex_set_key_handle(sep_payload_p, key_info->mp_key_handle_1);
        }
    } else { /* It is metadata I/O */
        /* We always use client 0 for the paged.
         */
        status = fbe_provision_drive_get_key_info(provision_drive, 0, &key_info);
        if(status != FBE_STATUS_OK){
            return status; /* Something is VERY WRONG. */
        }
        /* If encryption mode is in progress we need to pick the key based on 
         * if the paged was already encrypted or not. 
         * Paged is not encrypted on system drives. 
         */
        if (!is_system_drive &&
            (encryption_mode == FBE_BASE_CONFIG_ENCRYPTION_MODE_ENCRYPTION_IN_PROGRESS) ||
            (encryption_mode == FBE_BASE_CONFIG_ENCRYPTION_MODE_REKEY_IN_PROGRESS)){

            if (fbe_provision_drive_metadata_is_np_flag_set(provision_drive, 
                                                            FBE_PROVISION_DRIVE_NP_FLAG_PAGED_VALID) ||
                fbe_provision_drive_metadata_is_np_flag_set(provision_drive, 
                                                            FBE_PROVISION_DRIVE_NP_FLAG_PAGED_NEEDS_CONSUMED)||
                fbe_provision_drive_metadata_is_np_flag_set(provision_drive, 
                                                            FBE_PROVISION_DRIVE_NP_FLAG_PAGED_NEEDS_ZERO)){
                /* Paged already encrypted with the new key. */
                fbe_payload_ex_set_key_handle(sep_payload_p, key_info->mp_key_handle_2);
            } else {
                /* Paged not encrypted with new key yet. */
                fbe_payload_ex_set_key_handle(sep_payload_p, key_info->mp_key_handle_1);
            }
        } else {
            /* Just encrypted.  Use the only key.
             */
            fbe_payload_ex_set_key_handle(sep_payload_p, key_info->mp_key_handle_1);
        }
    }

    fbe_provision_drive_utils_trace(provision_drive, FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_INFO,
                                    FBE_PROVISION_DRIVE_DEBUG_FLAG_ENCRYPTION,
                                    "PVD: use key handle 0x%llx op: 0x%x lba: 0x%llx bl: 0x%llx i:%d m:%d\n",
                                    sep_payload_p->key_handle, block_operation_p->block_opcode,
                                    block_operation_p->lba, block_operation_p->block_count,
                                    server_index, is_monitor_packet);
    return FBE_STATUS_OK;
}

/*******************************************
 * end file fbe_provision_drive_executor.c
 *******************************************/
 
 
 
