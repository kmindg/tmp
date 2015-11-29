/***************************************************************************
 * Copyright (C) EMC Corporation 2007-2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_lun_zero.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the functionality related to LUN zeroing, all the 
 *  functions related to LUN zeroing will be added here.
 * 
 * @ingroup lun_class_files
 * 
 * @version
 *   04/15/2009:  Created. Dhavla Patel
 *
 ***************************************************************************/

 /*************************
 *   INCLUDE FILES
 *************************/
#include "fbe_lun_private.h"
#include "fbe_transport_memory.h"

/*****************************************
 * LOCAL FUNCTION PROTOTYPES
 *****************************************/

/* Completion function for the LUN zeroing. */
static fbe_status_t  fbe_lun_zero_start_lun_zeroing_completion(fbe_packet_t * packet_p,
                                                               fbe_packet_completion_context_t context);

/* completion function to get the raid extent zero checkpoint. */
static fbe_status_t fbe_lun_zero_get_raid_extent_zero_checkpoint_completion(fbe_packet_t * packet_p,
                                                                            fbe_packet_completion_context_t context);

/* completion function for lun zeroing packet allocation */
static fbe_status_t
fbe_lun_zero_start_lun_zeroing_packet_allocation_completion(fbe_memory_request_t * memory_request, 
                                                              fbe_memory_completion_context_t context);


/* completion function for lun zeroing packet allocation */
static fbe_status_t
fbe_lun_zero_start_lun_hard_zeroing_packet_allocation_completion(fbe_memory_request_t * memory_request, 
                                                                 fbe_memory_completion_context_t context);

/* build a packet to hard zero a couple of chunks */
static fbe_status_t 
fbe_lun_zero_build_hard_zero_packet(fbe_packet_t * new_packet_p, 
                                    fbe_lun_hard_zero_progress_t * lun_hard_zero_progess_p);

/* Completion function for the LUN hard zeroing. */
static fbe_status_t  fbe_lun_zero_lun_hard_zero_io_completion(fbe_packet_t * packet_p,
                                                              fbe_packet_completion_context_t context);
/* the completion function to send the zero write IO. */
static fbe_status_t  fbe_lun_zero_lun_hard_zero_to_send_io_completion(fbe_packet_t * packet_p,
                                                                   fbe_packet_completion_context_t context);

/* the completion function of the mater packet for hard zero */
static fbe_status_t  fbe_lun_zero_lun_hard_zero_completion(fbe_packet_t * packet_p,
                                                           fbe_packet_completion_context_t context);
/***************
 * FUNCTIONS
 ***************/

/*!****************************************************************************
 * fbe_lun_zero_start_lun_zeroing_packet_allocation()
 ******************************************************************************
 * @brief
 *   This function allocates a new sub-packet for zeroing.
 *   It is called from the LUN zeroing condition
 *   and condition will not be cleared until we get the response.
 *
 * @param lun_p         - pointer to the LUN object.
 * @param packet_p      - pointer to a monitor packet.
 *
 * @return  fbe_status_t  
 *
 ******************************************************************************/
fbe_status_t
fbe_lun_zero_start_lun_zeroing_packet_allocation(fbe_lun_t * lun_p, fbe_packet_t * packet_p)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_memory_request_t                    *memory_request_p = NULL;
    fbe_payload_ex_t                       *sep_payload_p = NULL;
    fbe_payload_control_operation_t *       control_operation_p = NULL;

    /* get the payload and control operation. */
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(sep_payload_p);

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
                                   (fbe_memory_completion_function_t)fbe_lun_zero_start_lun_zeroing_packet_allocation_completion,
                                   lun_p);

	fbe_transport_memory_request_set_io_master(memory_request_p,  packet_p);

    /* issue the memory request to memory service. */
    status = fbe_memory_request_entry(memory_request_p);
    if((status != FBE_STATUS_OK) && (status != FBE_STATUS_PENDING))
    {
        fbe_base_object_trace((fbe_base_object_t*)lun_p, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s: memory request failed with status: 0x%x\n",
                              __FUNCTION__, status);
        fbe_payload_control_set_status(control_operation_p, 
                                       FBE_PAYLOAD_CONTROL_STATUS_FAILURE);        
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return status;
    }

    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_lun_zero_start_lun_zeroing_packet_allocation()
 ******************************************************************************/


/*!****************************************************************************
 * fbe_lun_zero_start_lun_zeroing_packet_allocation_completion()
 ******************************************************************************
 * @brief
 *  This function is used as the completion function for the memory allocation
 *  of the new packet.
 *  It is used to issue the LUN zeroing block operation and then
 *  it waits for its completion. 
 *
 * @param memory_request    - Memory request for the allocation of the packet.
 * @param context           - Memory request completion context.
 *
 * @return  fbe_status_t  
 *
 * @author
 *   4/15/2009 - Created. Dhaval Patel
 *
 * @notes
 *   4/27/2011 - updated by Maya D. , to work with the newly allocated sub-packet
 ******************************************************************************/
static fbe_status_t
fbe_lun_zero_start_lun_zeroing_packet_allocation_completion(fbe_memory_request_t * memory_request, 
                                                              fbe_memory_completion_context_t context)
{
    fbe_lun_t *                             lun_p = NULL;
    fbe_packet_t *                          new_packet_p = NULL;
    fbe_packet_t *                          packet_p = NULL;
    fbe_payload_ex_t *                     sep_payload_p = NULL;
    fbe_payload_ex_t *                     new_sep_payload_p = NULL;
    fbe_payload_block_operation_t *         new_block_operation_p = NULL;
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_memory_header_t *                   master_memory_header = NULL;
    fbe_packet_resource_priority_t          new_packet_resource_priority = 0;
    fbe_optimum_block_size_t                optimum_block_size; // optimium block size
    fbe_lba_t                               lun_imported_capacity = FBE_LBA_INVALID;
    fbe_lba_t                               start_lba = 0;
    fbe_block_edge_t *                      block_edge_p;
    fbe_payload_control_operation_t *       control_operation_p = NULL;
    fbe_payload_block_operation_opcode_t    block_opcode;

    lun_p = (fbe_lun_t *)context;

    /* Get the originating packet and validate that the memory request is complete
     */
    packet_p = fbe_transport_memory_request_to_packet(memory_request);
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(sep_payload_p);

    /* Check allocation status */
    if (fbe_memory_request_is_allocation_complete(memory_request) == FBE_FALSE)
    {
        /* Currently this callback doesn't expect any aborted requests */
        fbe_base_object_trace((fbe_base_object_t*)lun_p, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "lunzero_startzro_allocpkt_complete: memory req: 0x%p failed\n",
                              memory_request);
        fbe_payload_control_set_status(control_operation_p, 
                                     FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
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

    
    /* Set block operation */
    new_block_operation_p = fbe_payload_ex_allocate_block_operation(new_sep_payload_p);
    block_opcode = ((lun_p->clear_need_zero_b == FBE_TRUE) ? 
                    FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_UNMARK_ZERO: FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ZERO);

    /* LUN zeroing request start LBA will be zero and block count will be
     * imported capacity of the LUN.
     */
    start_lba = 0;
    fbe_lun_get_imported_capacity(lun_p, &lun_imported_capacity);
    fbe_lun_get_block_edge(lun_p, &block_edge_p);

    fbe_base_object_trace((fbe_base_object_t*) lun_p, 
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "lunzero_startzro_allocpkt_complete: area: lba:0x%llx, blk_count:0x%llx, pkt:0x%p\n",
                          (unsigned long long)(block_edge_p->offset + start_lba),
			  (unsigned long long)lun_imported_capacity, packet_p);

    /* get optimum block size for this i/o request */
    fbe_block_transport_edge_get_optimum_block_size(&lun_p->block_edge, &optimum_block_size);

     /* next, build block operation in sep payload */
    fbe_payload_block_build_operation(new_block_operation_p,
                                      block_opcode,
                                      start_lba,
                                      (fbe_block_count_t) lun_imported_capacity,
                                      FBE_BE_BYTES_PER_BLOCK,
                                      optimum_block_size,
                                      NULL);

    /* Propagate the current expiration time */
    fbe_transport_propagate_expiration_time(new_packet_p, packet_p);
    
    /* Set completion function */
    fbe_transport_set_completion_function(new_packet_p, fbe_lun_zero_start_lun_zeroing_completion, lun_p);

    /* We need to assosiate newly allocated packet with original one */
    fbe_transport_add_subpacket(packet_p, new_packet_p);

    /* Bump the resource priority for any downstream allocations */
    new_packet_resource_priority = fbe_transport_get_resource_priority(packet_p);
    new_packet_resource_priority += FBE_TRANSPORT_RESOURCE_PRIORITY_ADJUSTMENT_FIRST;
    fbe_transport_set_resource_priority(new_packet_p, new_packet_resource_priority); 

    /* Put packet on the termination queue while we wait for the subpacket to complete. */
    fbe_transport_set_cancel_function(packet_p, fbe_base_object_packet_cancel_function, (fbe_base_object_t *)lun_p);
    fbe_base_object_add_to_terminator_queue((fbe_base_object_t *)lun_p, packet_p);


     /* now, activate this block operation */
    fbe_payload_ex_increment_block_operation_level(new_sep_payload_p);

    /* invoke bouncer to forward i/o request to downstream object */
    status = fbe_base_config_bouncer_entry((fbe_base_config_t *)lun_p, new_packet_p);
	if(status == FBE_STATUS_OK){ /* Small stack */
		status = fbe_lun_send_io_packet(lun_p, new_packet_p);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
	}

    return status;
}


/*!****************************************************************************
 * fbe_lun_zero_start_lun_zeroing_completion()
 ******************************************************************************
 * @brief
 *  This function is used as a completion function for the LUN zeroing block
 *  operation.
 *  It release the newly allocated packet after copying status and then
 *  it completes master packet.
 *
 * @param packet_p - Pointer to the packet.
 * @param context  - Packet completion context.
 *
 * @return fbe_status_t
 *
 * @author
 *  4/15/2010 - Created. Dhaval Patel
 *
 ******************************************************************************/
static fbe_status_t 
fbe_lun_zero_start_lun_zeroing_completion(fbe_packet_t * packet_p, fbe_packet_completion_context_t context)
{
    fbe_lun_t *                          lun_p = NULL;
    fbe_packet_t *                       master_packet = NULL;
    fbe_payload_ex_t *                  payload = NULL;
    fbe_payload_ex_t *                  sep_payload = NULL;
    fbe_payload_control_operation_t *    master_control_operation;
    fbe_payload_block_operation_t *      block_operation;
    fbe_status_t                         status;
    fbe_payload_block_operation_status_t block_operation_status;
    fbe_payload_control_status_t         master_control_status = FBE_PAYLOAD_CONTROL_STATUS_OK;

    lun_p = (fbe_lun_t *)context;
    master_packet = fbe_transport_get_master_packet(packet_p);

    /* Remove master packet from the termination queue. */
    fbe_base_object_remove_from_terminator_queue((fbe_base_object_t *)lun_p, master_packet);

    sep_payload = fbe_transport_get_payload_ex(master_packet);
    payload = fbe_transport_get_payload_ex(packet_p);

    master_control_operation = fbe_payload_ex_get_control_operation(sep_payload);
    block_operation = fbe_payload_ex_get_block_operation(payload);
   

    /* Get block status */
    fbe_payload_block_get_status(block_operation, &block_operation_status);
    status = fbe_transport_get_status_code(packet_p); 

    fbe_base_object_trace((fbe_base_object_t*) lun_p, 
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "lun zero request complete with: status:0x%x, block_status:0x%x\n",
                          status, block_operation_status);

    /* Copy packet status */
    if ((status != FBE_STATUS_OK)  || 
        (block_operation_status != FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS))
    {
        fbe_base_object_trace((fbe_base_object_t*) lun_p, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "lun zero request failed: status:0x%x, block_status:0x%x\n",
                              status, block_operation_status);
        master_control_status = FBE_PAYLOAD_CONTROL_STATUS_FAILURE;
        status = (status != FBE_STATUS_OK) ? status : FBE_STATUS_GENERIC_FAILURE;

    }
    else if (lun_p->clear_need_zero_b == FBE_FALSE)
    {
        master_control_status = FBE_PAYLOAD_CONTROL_STATUS_OK;

        /* log an event message that lun zeroing started */
        fbe_lun_event_log_lun_zero_start_or_complete(lun_p, FBE_TRUE);
    }

    fbe_transport_set_status(master_packet, status, 0);
    fbe_payload_control_set_status(master_control_operation, master_control_status);

    /* We need to remove packet from master queue */
    fbe_transport_remove_subpacket(packet_p);

    fbe_payload_ex_release_block_operation(payload, block_operation);

    /* destroy the packet. */
    fbe_transport_destroy_packet(packet_p);

    /* release the memory for the packet allocated by the lun */
    //fbe_memory_request_get_entry_mark_free(&master_packet->memory_request, &memory_entry_ptr);
    //fbe_memory_free_entry(memory_entry_ptr);
	fbe_memory_free_request_entry(&master_packet->memory_request);

    /* Complete master packet */
    fbe_transport_complete_packet(master_packet);
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}


/*!****************************************************************************
 * fbe_lun_zero_calculate_zero_checkpoint_for_lu_extent()
 ******************************************************************************
 * @brief
 *   This function is used to calculate the checkpoint for the lun extend from
 *   raid based checkpint.
 *
 * @param lun_p                 - pointer to the LUN object.
 * @param zero_checkpoint       - Lun extent zero checkpoint calculated by raid group.
 *                                This checkpoint is already normalize at RG level 
 *                                for lun offset.
 * @param lu_zero_checkpoint_p  - pointer to the lun zero checkpoint.
 *
 * @return  fbe_status_t  
 *
 * @author
 *  10/15/2009 - Created. Dhaval Patel
 *
 ******************************************************************************/
fbe_status_t
fbe_lun_zero_calculate_zero_checkpoint_for_lu_extent(fbe_lun_t * lun_p,
                                                     fbe_lba_t   zero_checkpoint,
                                                     fbe_lba_t * lu_zero_checkpoint_p)
{
    fbe_lba_t       imported_capacity;

    /* initialze the lu zero percentage as zero. */
    *lu_zero_checkpoint_p = 0;

    /* get the capacity of lun object. */
    fbe_lun_get_imported_capacity(lun_p, &imported_capacity);

    if(zero_checkpoint != FBE_LBA_INVALID)
    {
        /* calculate the zero checkpoint for the lu extent. */
        if(zero_checkpoint >= imported_capacity)
        {
            /* update the lun zero checkpoint as lba invalid. */
            *lu_zero_checkpoint_p = FBE_LBA_INVALID;
        }
        else
        {
            /* if it is in lun range then return lu zero checkpoint. */
            *lu_zero_checkpoint_p  = zero_checkpoint;
        }
    }
    else if(zero_checkpoint == FBE_LBA_INVALID)
    {
        /* if raid zero checkpoint is lba invalid the update the lu zero checkpoint same
         * as capacity.
         */
        *lu_zero_checkpoint_p = FBE_LBA_INVALID;
    }

    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_lun_zero_calculate_zero_checkpoint_for_lu_extent()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_lun_zero_calculate_zero_percent_for_lu_extent()
 ******************************************************************************
 * @brief
 *  This function is used to calculate the percentage for the lun zeroing based
 *  on its checkpoint.
 *
 * @param lun_p                 - pointer to the LUN object.
 * @param lu_zero_checkpoint    - lun zero checkpoint.
 * @param lu_zero_percentage_p  - lun zeroing progress in percentage.
 *
 * @return  fbe_status_t  
 *
 * @author
 *  10/15/2009 - Created. Dhaval Patel
 *
 ******************************************************************************/
fbe_status_t
fbe_lun_zero_calculate_zero_percent_for_lu_extent(fbe_lun_t * lun_p,
                                                  fbe_lba_t   lu_zero_checkpoint,
                                                  fbe_u16_t * lu_zero_percentage_p)
{
    fbe_lba_t       imported_capacity;

    /* get the imported capacity of lun object which includes user data and metadata. */
    fbe_lun_get_imported_capacity(lun_p, &imported_capacity);

    /* if lu zero checkpoint is greater than capacity then return 100% zero. */
    if(lu_zero_checkpoint >= imported_capacity)
    {
        *lu_zero_percentage_p = 100;
    }
    else
    {
        /* calculate the zero percentage. */
        *lu_zero_percentage_p = (fbe_u16_t) ((lu_zero_checkpoint * 100) / imported_capacity);
    }

    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_lun_zero_calculate_zero_percent_for_lu_extent()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_lun_zero_adjust_zeroing_udpate_rate()
 ******************************************************************************
 * @brief
 *  Adjusts the lun zero update checkpoint rate based on zeroing velocity.
 *
 * @param lun_p                 - pointer to the LUN object.
 * @param zero_checkpoint       - Zeroing checkpoint based on current update.
 *
 * @return  none
 *
 * @author
 *  07/02/2012 - Created. Prahlad Purohit
 *
 ******************************************************************************/
static void
fbe_lun_zero_adjust_zeroing_udpate_rate(fbe_lun_t *lun_p, 
                                        fbe_lba_t zero_checkpoint)

{
    fbe_lba_t   current_zero_checkpoint = FBE_LBA_INVALID;
    fbe_lba_t   exported_capacity = 0;
    fbe_u32_t   percentage_zeroed = 0;

    fbe_lun_metadata_get_zero_checkpoint(lun_p, &current_zero_checkpoint);
    fbe_base_config_get_capacity((fbe_base_config_t *)lun_p, &exported_capacity);

    percentage_zeroed = (fbe_u32_t) (((current_zero_checkpoint - zero_checkpoint) * 100) / exported_capacity);

    if(zero_checkpoint != 0)
    {
        /* If percentage zeroed since last update is less than 1 decrease the rate of update.
         * If percentage zeroed since last update is more than 1 increate the rate of update.*/
        if((0 == percentage_zeroed) && (lun_p->zero_update_interval_count < FBE_LUN_MAX_ZEROING_UPDATE_INTERVAL))
        {
            lun_p->zero_update_interval_count++;
        }
        else if((percentage_zeroed > 1) && (lun_p->zero_update_interval_count > FBE_LUN_MIN_ZEROING_UPDATE_INTERVAL))
        {
            lun_p->zero_update_interval_count--;
        }
    }

    return;
}
/******************************************************************************
 * end fbe_lun_zero_adjust_zeroing_udpate_rate()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_lun_zero_get_raid_extent_zero_checkpoint()
 ******************************************************************************
 * @brief
 *  This function is used to get the zero checkpoint for the raid group extent.
 *
 * @param lun_p                 - pointer to the LUN object.
 * @param packet_p              - pointer to the packet.
 *
 * @return  fbe_status_t  
 *
 * @author
 *  10/15/2009 - Created. Dhaval Patel
 *
 ******************************************************************************/
fbe_status_t
fbe_lun_zero_get_raid_extent_zero_checkpoint(fbe_lun_t * lun_p,
                                             fbe_packet_t * packet_p)
{

    fbe_raid_group_get_zero_checkpoint_raid_extent_t *  zero_checkpoint_raid_extent_p = NULL;
    fbe_payload_ex_t *                                 sep_payload_p = NULL;
    fbe_payload_control_operation_t *                   control_operation_p = NULL;
    fbe_payload_control_operation_t *                   new_control_operation_p = NULL;
    fbe_block_edge_t *                                  block_edge_p = NULL;

    /* get the sep payload and control operation/ */
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(sep_payload_p);

    /* allocate the new control operation to buil. */
    new_control_operation_p = fbe_payload_ex_allocate_control_operation(sep_payload_p);
    if (new_control_operation_p == NULL)
    {
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* allocate the buffer to send the raid group zero percent raid extent usurpur. */
    zero_checkpoint_raid_extent_p = (fbe_raid_group_get_zero_checkpoint_raid_extent_t *) fbe_transport_allocate_buffer();
    if(zero_checkpoint_raid_extent_p == NULL)
    {
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* initialize the raid extent zero checkpoint as zero.
       Pass the lun capacity and lun offset into the request so RG can
       normalize zero checkpoint at lun level */
    zero_checkpoint_raid_extent_p->zero_checkpoint = 0;
    fbe_lun_get_offset(lun_p, &zero_checkpoint_raid_extent_p->lun_offset);   
    fbe_lun_get_imported_capacity(lun_p, &zero_checkpoint_raid_extent_p->lun_capacity);

    fbe_payload_control_build_operation(new_control_operation_p,
                                        FBE_RAID_GROUP_CONTROL_CODE_GET_ZERO_CHECKPOINT_RAID_EXTENT,
                                        zero_checkpoint_raid_extent_p,
                                        sizeof(fbe_raid_group_get_zero_checkpoint_raid_extent_t));
    fbe_payload_ex_increment_control_operation_level(sep_payload_p);

    /* set the completion function. */
    fbe_transport_set_completion_function(packet_p, fbe_lun_zero_get_raid_extent_zero_checkpoint_completion, lun_p);

    /* send the packet to raid object to get the checkpoint for raid extent. */
    fbe_lun_get_block_edge(lun_p, &block_edge_p);
    fbe_block_transport_send_control_packet(block_edge_p, packet_p);
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/******************************************************************************
 * end fbe_lun_zero_get_raid_extent_zero_checkpoint()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_lun_zero_get_raid_extent_zero_checkpoint_completion()
 ******************************************************************************
 * @brief
 *  This function is used as completion function for collecting zero checkpoint
 *  for raid extent.
 *
 * @param packet_p             - pointer to the packet.
 * @param context              - completion context.
 *
 * @return  fbe_status_t  
 *
 * @author
 *  10/15/2009 - Created. Dhaval Patel
 *  07/02/2012 - Modified Prahlad Purohit
 *
 ******************************************************************************/
static fbe_status_t
fbe_lun_zero_get_raid_extent_zero_checkpoint_completion(fbe_packet_t * packet_p,
                                                        fbe_packet_completion_context_t context)
{
    fbe_lun_t *                                             lun_p = NULL;
    fbe_payload_ex_t *                                      sep_payload_p = NULL;
    fbe_payload_control_operation_t *                       control_operation_p = NULL;
    fbe_payload_control_operation_t *                       prev_control_operation_p = NULL;
    fbe_payload_control_status_t                            control_status;
    fbe_status_t                                            status;
    fbe_raid_group_get_zero_checkpoint_raid_extent_t *      zero_checkpoint_raid_extent_p = NULL;
    fbe_lba_t                                               zero_checkpoint = FBE_LBA_INVALID;
    fbe_bool_t                                              persist_checkpoint = FALSE;    


    /* Cast the context to base config object pointer. */
    lun_p = (fbe_lun_t *) context;

    /* get the control operation of the subpacket. */
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(sep_payload_p);
    prev_control_operation_p = fbe_payload_ex_get_prev_control_operation(sep_payload_p);
    fbe_payload_control_get_buffer(control_operation_p, &zero_checkpoint_raid_extent_p);

    /* get the status of the subpacket operation. */
    status = fbe_transport_get_status_code(packet_p);
    fbe_payload_control_get_status(control_operation_p, &control_status);

    /* if any of the subpacket failed with bad status then update the master status. */
    if((status != FBE_STATUS_OK) ||
       (control_status != FBE_PAYLOAD_CONTROL_STATUS_OK))
    {
        /* release the allocated buffer and current control operation. */
        fbe_transport_release_buffer(zero_checkpoint_raid_extent_p);
        fbe_payload_ex_release_control_operation(sep_payload_p, control_operation_p);
        fbe_payload_control_set_status(prev_control_operation_p, control_status);
        fbe_transport_set_status(packet_p, status, 0);
        return status;
    }

    /* calculate the zero checkpoint for rg extent. */
    fbe_lun_zero_calculate_zero_checkpoint_for_lu_extent(lun_p,
                                                         zero_checkpoint_raid_extent_p->zero_checkpoint,
                                                         &zero_checkpoint);   

    /* release the allocated buffer and control operation. */
    fbe_transport_release_buffer(zero_checkpoint_raid_extent_p);
    fbe_payload_ex_release_control_operation(sep_payload_p, control_operation_p);

    /* check if LUN zeroing completed */
    if(zero_checkpoint == FBE_LBA_INVALID)
    {

        if(lun_p->ndb_b)
        {
            fbe_u32_t                           lun_number;                 /* Navi LUN number/ALU */
            fbe_object_id_t                     object_id;                  /* object id of the LUN object */

            /* get lun related information to trace it */
            fbe_base_object_get_object_id((fbe_base_object_t*) lun_p, &object_id);
            fbe_database_get_lun_number(object_id, &lun_number);               

            /* if lun bound with NDB flag set, print some traces so user know that we are logging
               LUN Zeroing completed message just for informative that LUN is completely initialized.
               There could be case if someone unbind a lun accidently when zeroing is in progress for LUN and
               it again rebind with NDB flag set before disk completes zeroing on LUN bound area, this message will
               logs when actual zeroing complete. This message does not mean LUN bind with NDB has initiated zeroing */
               
            fbe_base_object_trace((fbe_base_object_t*) lun_p, 
                                  FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                  "LUN bound with NDB flag, so logging informative msg for lun %d(obj 0x%x) that zeroing is done\n",
                                   lun_number, object_id);
        }
    
        /* log a message that lun zeroing completed */
        fbe_lun_event_log_lun_zero_start_or_complete(lun_p, FBE_FALSE);

        /* Since we are done zeroing, it might be time for the initial verify
         * to get started if the user requested it and we have not run it yet. 
         */

        if (fbe_lun_metadata_is_initial_verify_needed(lun_p))
        {
            fbe_base_object_trace((fbe_base_object_t*) lun_p, 
                                  FBE_TRACE_LEVEL_DEBUG_LOW, FBE_TRACE_MESSAGE_ID_INFO,
                                  "zero is complete start initial verify noinitialverify: 0x%x verify_ran:0x%x\n",
                                  fbe_lun_get_noinitialverify_b(lun_p),
                                  fbe_lun_metadata_was_initial_verify_run(lun_p));
            fbe_lifecycle_set_cond(&fbe_lun_lifecycle_const, (fbe_base_object_t *)lun_p, FBE_LUN_LIFECYCLE_COND_VERIFY_NEW_LUN);
        }
        else
        {
            fbe_base_object_trace((fbe_base_object_t*) lun_p, 
                                  FBE_TRACE_LEVEL_DEBUG_LOW, FBE_TRACE_MESSAGE_ID_INFO,
                                  "zero is complete do not run initial verify noinitialverify: 0x%x verify_ran:0x%x\n",
                                  fbe_lun_get_noinitialverify_b(lun_p),
                                  !fbe_lun_metadata_was_initial_verify_run(lun_p));
        }

        /* if lun zeroing completeted persist the checkpoint otherwise just set. */
        persist_checkpoint = TRUE;        
    }

    /* adjust zeroing update rate based on zeroing velocity. */
    fbe_lun_zero_adjust_zeroing_udpate_rate(lun_p, zero_checkpoint);

    /* update the zero checkpoint with lun nonpaged metadata. */
    status = fbe_lun_metadata_set_zero_checkpoint(lun_p, packet_p, zero_checkpoint, persist_checkpoint);

    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/******************************************************************************
 * end fbe_lun_zero_get_raid_extent_zero_checkpoint_completion()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_lun_zero_start_lun_hard_zeroing_packet_allocation()
 ******************************************************************************
 * @brief
 *  This function is to initialte the hard zero on a LUN
 *  for raid extent.
 *
 * @param lun_p                 - pointer to the LUN object.
 * @param in_packet_p           - pointer to the packet.
 *
 * @return  fbe_status_t  
 *
 * @author
 *  27/12/2012 - Created. Geng Han
 *
 ******************************************************************************/
fbe_status_t  fbe_lun_zero_start_lun_hard_zeroing_packet_allocation(fbe_lun_t *lun_p,
                                                                    fbe_packet_t *packet_p,
                                                                    fbe_lba_t lba,
                                                                    fbe_block_count_t blocks,
                                                                    fbe_u64_t threads)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_memory_request_t                    *memory_request_p = NULL;
    fbe_payload_ex_t                       *sep_payload_p = NULL;
    fbe_payload_control_operation_t *       control_operation_p = NULL;

    /* get the payload and control operation. */
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(sep_payload_p);

    /* We need to allocate new packet due to the limited number of block
     * payloads.  The memory request priority comes from the original packet.
     */
     memory_request_p = fbe_transport_get_memory_request(packet_p);

    /* build the memory request for allocation. */
    fbe_memory_build_chunk_request(memory_request_p,
                                   FBE_MEMORY_CHUNK_SIZE_FOR_PACKET,
                                   (fbe_memory_number_of_objects_t)(threads + 1),
                                   fbe_transport_get_resource_priority(packet_p),
                                   fbe_transport_get_io_stamp(packet_p),
                                   (fbe_memory_completion_function_t)fbe_lun_zero_start_lun_hard_zeroing_packet_allocation_completion,
                                   lun_p);

	fbe_transport_memory_request_set_io_master(memory_request_p,  packet_p);

    /* issue the memory request to memory service. */
    status = fbe_memory_request_entry(memory_request_p);
    if((status != FBE_STATUS_OK) && (status != FBE_STATUS_PENDING))
    {
        fbe_base_object_trace((fbe_base_object_t*)lun_p, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s: memory request failed with status: 0x%x\n",
                              __FUNCTION__, status);
        fbe_payload_control_set_status(control_operation_p, 
                                       FBE_PAYLOAD_CONTROL_STATUS_FAILURE);        
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return status;
    }

    return FBE_STATUS_OK;
}

/*!****************************************************************************
 * fbe_lun_zero_start_lun_hard_zeroing_packet_allocation_completion()
 ******************************************************************************
 * @brief
 *  This function is used as the completion function for the memory allocation
 *  of the new packet.
 *  It is used to issue the LUN write zero block operation and then
 *  it waits for its completion. 
 *
 * @param memory_request    - Memory request for the allocation of the packet.
 * @param context           - Memory request completion context.
 *
 * @return  fbe_status_t  
 *
 * @author
 *   27/12/2014 - Created. Geng Han
 *
 * @notes
 ******************************************************************************/
static fbe_status_t
fbe_lun_zero_start_lun_hard_zeroing_packet_allocation_completion(fbe_memory_request_t * memory_request, 
                                                                 fbe_memory_completion_context_t context)
{
    fbe_lun_t *                             lun_p = NULL;
    fbe_packet_t *                          new_packet_p = NULL;
    fbe_packet_t *                          packet_p = NULL;
    fbe_payload_ex_t *                      sep_payload_p = NULL;
    fbe_lun_hard_zero_t *                   init_lun_hard_zero_p = NULL;
    fbe_lun_hard_zero_progress_t *          lun_hard_zero_progess_p = NULL;
    fbe_u32_t                               index;
    fbe_memory_header_t *                   cur_memory_header = NULL;
    fbe_memory_header_t *                   next_memory_header = NULL;
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_payload_control_operation_t *       control_operation_p = NULL;
    fbe_queue_element_t *                   queue_element;

    lun_p = (fbe_lun_t *)context;

    /* Get the originating packet and validate that the memory request is complete
     */
    packet_p = fbe_transport_memory_request_to_packet(memory_request);
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(sep_payload_p);
    fbe_payload_control_get_buffer(control_operation_p, &init_lun_hard_zero_p);

    /* Check allocation status */
    if (fbe_memory_request_is_allocation_complete(memory_request) == FBE_FALSE)
    {
        /* Currently this callback doesn't expect any aborted requests */
        fbe_base_object_trace((fbe_base_object_t*)lun_p, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "lunzero_start_hard_zero_allocpkt_complete: memory req: 0x%p failed\n",
                              memory_request);
        fbe_payload_control_set_status(control_operation_p, 
                                     FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Get the pointer to the allocated memory from the memory request. */
    cur_memory_header = fbe_memory_get_first_memory_header(memory_request);
    lun_hard_zero_progess_p = (fbe_lun_hard_zero_progress_t *)fbe_memory_header_to_data(cur_memory_header);

    fbe_spinlock_init(&lun_hard_zero_progess_p->io_progress_lock);
    lun_hard_zero_progess_p->usurper_packet_p = packet_p;
    lun_hard_zero_progess_p->lun_p = lun_p;
    lun_hard_zero_progess_p->current_lba = init_lun_hard_zero_p->lba;
    lun_hard_zero_progess_p->blocks_remaining = init_lun_hard_zero_p->blocks;
    lun_hard_zero_progess_p->outstanding_io_count = 0;

    /* Initialize sub packets. */
    for (index = 0; index < init_lun_hard_zero_p->threads; index++)
    {
        fbe_memory_get_next_memory_header(cur_memory_header, &next_memory_header);
        new_packet_p = (fbe_packet_t *)fbe_memory_header_to_data(next_memory_header);
        fbe_transport_initialize_sep_packet(new_packet_p);
        status = fbe_lun_zero_build_hard_zero_packet(new_packet_p, lun_hard_zero_progess_p);
        if (status == FBE_STATUS_OK)
        {
            fbe_transport_add_subpacket(packet_p, new_packet_p);
        }
        else
        {
            break;
        }
        cur_memory_header = next_memory_header;
    }

    /* Put packet on the termination queue while we wait for the subpacket to complete. */
    fbe_transport_set_cancel_function(packet_p, fbe_base_object_packet_cancel_function, (fbe_base_object_t *)lun_p);
    // fbe_base_object_add_to_terminator_queue((fbe_base_object_t *)lun_p, packet_p);

    // fbe_transport_set_completion_function(packet_p, fbe_lun_zero_lun_hard_zero_completion, lun_p);
    
    /* send the subpackets */
    fbe_spinlock_lock(&lun_hard_zero_progess_p->io_progress_lock);

    queue_element = fbe_queue_front(&packet_p->subpacket_queue_head);
    while (queue_element != NULL)
    {
        new_packet_p = fbe_transport_subpacket_queue_element_to_packet(queue_element);

        fbe_transport_set_completion_function(new_packet_p, fbe_lun_zero_lun_hard_zero_to_send_io_completion, lun_p);
        fbe_transport_run_queue_push_packet(new_packet_p, FBE_TRANSPORT_RQ_METHOD_SAME_CORE);
        
        queue_element = fbe_queue_next(&packet_p->subpacket_queue_head, queue_element);
    }

    fbe_spinlock_unlock(&lun_hard_zero_progess_p->io_progress_lock);

    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}

/* build a packet to hard zero a couple of chunks */
/*!****************************************************************************
 * fbe_lun_zero_start_lun_hard_zeroing_packet_allocation_completion()
 ******************************************************************************
 * @brief
 *  This function is used to build a packet to hard zero a couple of chunks
 *
 * @param new_packet_p              - the packet pointer
 * @param lun_hard_zero_progess_p   - the pointer to the zero progress structure
 *
 * @return  fbe_status_t  
 *
 * @author
 *   27/12/2014 - Created. Geng Han
 *
 * @notes
 ******************************************************************************/
fbe_status_t fbe_lun_zero_build_hard_zero_packet(fbe_packet_t * new_packet_p, 
                                                 fbe_lun_hard_zero_progress_t * lun_hard_zero_progess_p)
{
    fbe_packet_t *                          packet_p = lun_hard_zero_progess_p->usurper_packet_p;
    fbe_lun_t *                             lun_p = lun_hard_zero_progess_p->lun_p;
    fbe_payload_ex_t *                      new_sep_payload_p = NULL;
    fbe_payload_block_operation_t *         new_block_operation_p = NULL;
    fbe_packet_resource_priority_t          new_packet_resource_priority = 0;
    fbe_optimum_block_size_t                optimum_block_size; // optimium block size
    fbe_lba_t                               start_lba = 0;
    fbe_block_count_t                       blocks = 0;                          
    fbe_block_count_t                       max_blocks = FBE_LUN_CHUNK_SIZE*10;
    fbe_payload_block_operation_opcode_t    block_opcode;

    if (lun_hard_zero_progess_p->blocks_remaining == 0)
    {
        fbe_base_object_trace((fbe_base_object_t*)lun_p, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s: no action needed\n",
                              __FUNCTION__);
        return FBE_STATUS_NO_ACTION;
    }
    start_lba = lun_hard_zero_progess_p->current_lba;
    blocks = lun_hard_zero_progess_p->blocks_remaining > max_blocks ? max_blocks : lun_hard_zero_progess_p->blocks_remaining;
    lun_hard_zero_progess_p->blocks_remaining -= blocks;
    lun_hard_zero_progess_p->current_lba += blocks;
    lun_hard_zero_progess_p->outstanding_io_count++;

#if 0
    fbe_base_object_trace((fbe_base_object_t*)lun_p, 
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s: hard zero lba 0x%llx, blocks: 0x%llx\n",
                          __FUNCTION__, start_lba, blocks);
#endif
    

    new_sep_payload_p = fbe_transport_get_payload_ex(new_packet_p);

    /* Set block operation */
    new_block_operation_p = fbe_payload_ex_allocate_block_operation(new_sep_payload_p);
    block_opcode = FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_ZEROS;

    
    /* get optimum block size for this i/o request */
    fbe_block_transport_edge_get_optimum_block_size(&lun_p->block_edge, &optimum_block_size);

     /* next, build block operation in sep payload */
    fbe_payload_block_build_operation(new_block_operation_p,
                                      block_opcode,
                                      start_lba,
                                      blocks,
                                      FBE_BE_BYTES_PER_BLOCK,
                                      optimum_block_size,
                                      NULL);
    

    /* Propagate the current expiration time */
    fbe_transport_propagate_expiration_time(new_packet_p, packet_p);

    /* Set completion function */
    fbe_transport_set_completion_function(new_packet_p, fbe_lun_zero_lun_hard_zero_io_completion, lun_hard_zero_progess_p);
    
    /* Bump the resource priority for any downstream allocations */
    new_packet_resource_priority = fbe_transport_get_resource_priority(packet_p);
    new_packet_resource_priority += FBE_TRANSPORT_RESOURCE_PRIORITY_ADJUSTMENT_FIRST;
    fbe_transport_set_resource_priority(new_packet_p, new_packet_resource_priority); 
    
     /* now, activate this block operation */
    fbe_payload_ex_increment_block_operation_level(new_sep_payload_p);
    
    return FBE_STATUS_OK;
}


/*!****************************************************************************
 * fbe_lun_zero_lun_hard_zero_io_completion()
 ******************************************************************************
 * @brief
 *  This function is used as a completion function for the LUN zero write block
 *  operation.
 *  It release the newly allocated packet after copying status and then
 *  it completes master packet.
 *
 * @param packet_p - Pointer to the packet.
 * @param context  - Packet completion context.
 *
 * @return fbe_status_t
 *
 * @author
 *  28/12/2014 - Created. Geng Han
 *
 ******************************************************************************/
static fbe_status_t 
fbe_lun_zero_lun_hard_zero_io_completion(fbe_packet_t * packet_p, fbe_packet_completion_context_t context)
{
    fbe_lun_hard_zero_progress_t *       lun_hard_zero_progess_p = (fbe_lun_hard_zero_progress_t*)context;
    fbe_lun_t *                          lun_p = lun_hard_zero_progess_p->lun_p;
    fbe_packet_t *                       master_packet = NULL;
    fbe_payload_ex_t *                   payload = NULL;
    fbe_payload_ex_t *                   sep_payload = NULL;
    fbe_payload_control_operation_t *    master_control_operation;
    fbe_payload_block_operation_t *      block_operation;
    fbe_status_t                         status;
    fbe_payload_block_operation_status_t block_operation_status;
    fbe_payload_control_status_t         master_control_new_status = FBE_PAYLOAD_CONTROL_STATUS_OK;
    fbe_payload_control_status_t         master_control_old_status;
    fbe_status_t                         master_packet_old_status;
    fbe_status_t                         master_packet_new_status = FBE_STATUS_OK;
    fbe_bool_t                           is_empty = FBE_FALSE;
        

    master_packet = fbe_transport_get_master_packet(packet_p);

    sep_payload = fbe_transport_get_payload_ex(master_packet);
    payload = fbe_transport_get_payload_ex(packet_p);

    master_control_operation = fbe_payload_ex_get_control_operation(sep_payload);
    block_operation = fbe_payload_ex_get_block_operation(payload);
   

    /* Get block status */
    fbe_payload_block_get_status(block_operation, &block_operation_status);
    status = fbe_transport_get_status_code(packet_p); 

    fbe_payload_ex_release_block_operation(payload, block_operation);

#if 0
    fbe_base_object_trace((fbe_base_object_t*) lun_p, 
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "lun hard zero request complete with: status:0x%x, block_status:0x%x\n",
                          status, block_operation_status);
#endif

    /* Copy packet status */
    if ((status != FBE_STATUS_OK)  || 
        (block_operation_status != FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS))
    {
        fbe_base_object_trace((fbe_base_object_t*) lun_p, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "lun hard zero request failed: status:0x%x, block_status:0x%x\n",
                              status, block_operation_status);
        if (block_operation_status != FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS)
        {
            master_control_new_status = FBE_PAYLOAD_CONTROL_STATUS_FAILURE;
        }
        master_packet_new_status = status;
    }
    
    fbe_spinlock_lock(&lun_hard_zero_progess_p->io_progress_lock);
    lun_hard_zero_progess_p->outstanding_io_count--;

    /* update the control status */
    fbe_payload_control_get_status(master_control_operation, &master_control_old_status);
    master_packet_old_status = fbe_transport_get_status_code(master_packet);
    if (master_control_old_status == FBE_PAYLOAD_CONTROL_STATUS_OK)
    {
        /* we will not overwrite the failure status */
        fbe_payload_control_set_status(master_control_operation, master_control_new_status);
    }
    if (master_packet_old_status == FBE_STATUS_OK || master_packet_old_status == FBE_STATUS_INVALID)
    {
        fbe_transport_set_status(master_packet, master_packet_new_status, 0);
    }


    /* reuse this packet */
    //fbe_transport_reuse_packet(packet_p);

    /* We are resending this block op and need to reuse the resource priority. */
    packet_p->resource_priority_boost = 0;
    
    status = fbe_lun_zero_build_hard_zero_packet(packet_p, lun_hard_zero_progess_p);
    
    if (status == FBE_STATUS_OK)
    {
        fbe_transport_set_completion_function(packet_p, fbe_lun_zero_lun_hard_zero_to_send_io_completion, lun_p);
        fbe_transport_run_queue_push_packet(packet_p, FBE_TRANSPORT_RQ_METHOD_SAME_CORE);
        fbe_spinlock_unlock(&lun_hard_zero_progess_p->io_progress_lock);

        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }
    else
    {
        fbe_transport_remove_subpacket_is_queue_empty(packet_p, &is_empty);
    }        

    /* all the blocks are zeroed, complete the master packet if necessary */
    if (lun_hard_zero_progess_p->blocks_remaining == 0 && lun_hard_zero_progess_p->outstanding_io_count == 0 && is_empty)
    {
        fbe_spinlock_unlock(&lun_hard_zero_progess_p->io_progress_lock);

        /* remove the master packet from terminator queue and complete it. */
        //fbe_base_object_remove_from_terminator_queue((fbe_base_object_t *)lun_p, master_packet);
        /* destroy the subpackets */
        fbe_transport_destroy_subpackets(master_packet);
        /* destroy the spin lock */
        fbe_spinlock_destroy(&lun_hard_zero_progess_p->io_progress_lock);

        /* release the preallocated memory for the hard zero request. */
        fbe_memory_free_request_entry(&master_packet->memory_request);

        fbe_transport_complete_packet(master_packet);
    }
    else
    {
        fbe_spinlock_unlock(&lun_hard_zero_progess_p->io_progress_lock);
    }

    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}

/*!****************************************************************************
 * fbe_lun_zero_lun_hard_zero_to_send_io_completion()
 ******************************************************************************
 * @brief
 *  This function is used to send write zero IO
 *
 * @param packet_p - Pointer to the packet.
 * @param context  - Packet completion context.
 *
 * @return fbe_status_t
 *
 * @author
 *  28/12/2014 - Created. Geng Han
 *
 ******************************************************************************/
static fbe_status_t  fbe_lun_zero_lun_hard_zero_to_send_io_completion(fbe_packet_t * packet_p,
                                                                   fbe_packet_completion_context_t context)
{
    fbe_status_t status;
    fbe_lun_t *lun_p = (fbe_lun_t*)context;

    /* invoke bouncer to forward i/o request to downstream object */
    status = fbe_base_config_bouncer_entry((fbe_base_config_t *)lun_p, packet_p);
    if(status == FBE_STATUS_OK)
    { 
        /* Small stack */
        status = fbe_lun_send_io_packet(lun_p, packet_p);
    }

    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}

/*!****************************************************************************
 * fbe_lun_zero_lun_hard_zero_completion()
 ******************************************************************************
 * @brief
 *  the completion function of the mater packet for hard zero
 *
 * @param packet_p - Pointer to the packet.
 * @param context  - Packet completion context.
 *
 * @return fbe_status_t
 *
 * @author
 *  28/12/2014 - Created. Geng Han
 *
 ******************************************************************************/
fbe_status_t  fbe_lun_zero_lun_hard_zero_completion(fbe_packet_t * packet_p,
                                                           fbe_packet_completion_context_t context)
{
    //fbe_lun_t *lun_p = (fbe_lun_t*)context;
    
    /* remove the master packet from terminator queue and complete it. */
    // fbe_base_object_remove_from_terminator_queue((fbe_base_object_t *)lun_p, packet_p);
    /* destroy the subpackets */
    fbe_transport_destroy_subpackets(packet_p);
    /* release the preallocated memory for the hard zero request. */
    fbe_memory_free_request_entry(&packet_p->memory_request);

    return FBE_STATUS_OK;
}


/*******************************
 * end fbe_lun_zero.c
 *******************************/
