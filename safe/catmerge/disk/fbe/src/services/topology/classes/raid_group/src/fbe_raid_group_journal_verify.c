/***************************************************************************
 * Copyright (C) EMC Corporation 2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_raid_group_journal_verify.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the raid group code takes care of journal verify
 * 
 * @version
 *   4/09/2012:  Created. Ashwin Tamilarasan
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe_raid_group_bitmap.h"
#include "fbe_raid_group_object.h"
#include "fbe_transport_memory.h"
#include "fbe_service_manager.h"
#include "fbe_raid_geometry.h"
#include "fbe_raid_verify.h"

/*****************************************
 * LOCAL FUNCTION PROTOTYPES
 *****************************************/

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

fbe_status_t fbe_raid_group_init_journal_memory_allocation_completion(fbe_memory_request_t * memory_request_p, 
                                                                      fbe_memory_completion_context_t in_context);

fbe_status_t fbe_raid_group_monitor_init_journal_completion(fbe_packet_t *packet_p,
                                                            fbe_packet_completion_context_t context);

fbe_status_t fbe_raid_group_journal_init_subpacket_completion(fbe_packet_t* packet_p,
                                                              fbe_packet_completion_context_t context);
/*!**************************************************************
 * fbe_raid_group_monitor_run_journal_verify()
 ****************************************************************
 * @brief
 *  This function initiate the journal verify.
 *
 * @param in_object_p - The pointer to the raid group.
 * @param in_packet_p - The control packet sent to us from the scheduler.
 *
 * @return fbe_lifecycle_status_t - FBE_LIFECYCLE_STATUS_PENDING 
 *
 * @author
 *  03/14/2012 - Created Ashwin Tamilarasan
 *
 ****************************************************************/
fbe_lifecycle_status_t fbe_raid_group_monitor_run_journal_verify(fbe_base_object_t*  in_object_p,
                                                                  fbe_packet_t*       packet_p)
{
    fbe_raid_group_t*                    raid_group_p;   
    fbe_bool_t                           can_proceed = FBE_TRUE;
    fbe_raid_geometry_t*                 raid_geometry_p = NULL;
    fbe_memory_request_t*                memory_request_p = NULL;
    fbe_packet_resource_priority_t       memory_request_priority;
    fbe_u32_t                            width;
    fbe_status_t                         status;
    fbe_scheduler_hook_status_t          hook_status = FBE_SCHED_STATUS_OK;
    
    FBE_RAID_GROUP_TRACE_FUNC_ENTRY(in_object_p);


    raid_group_p = (fbe_raid_group_t*)in_object_p;

    /* If we are quiesced/quiescing exit this condition. */
    if (fbe_base_config_is_any_clustered_flag_set((fbe_base_config_t *)raid_group_p, 
                                (FBE_BASE_CONFIG_CLUSTERED_FLAG_QUIESCED|FBE_BASE_CONFIG_CLUSTERED_FLAG_QUIESCING))) {
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s quiesced or quiescing, defer MD verify\n", __FUNCTION__);

        /* Just return done.  Since we haven't cleared the condition we will be re-scheduled. */
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    //  See if we are allowed to do a verify I/O based on the current load.
    fbe_raid_group_verify_check_permission_based_on_load(raid_group_p, &can_proceed);
    if (can_proceed == FBE_FALSE) {
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s The load on downstream edges is higher\n", __FUNCTION__);

        return FBE_LIFECYCLE_STATUS_DONE;
    }

    fbe_raid_group_check_hook(raid_group_p,  SCHEDULER_MONITOR_STATE_RAID_GROUP_JOURNAL_VERIFY, 
                                  FBE_RAID_GROUP_SUBSTATE_JOURNAL_VERIFY_STARTED, 0, &hook_status);
    if (hook_status == FBE_SCHED_STATUS_DONE) {return FBE_LIFECYCLE_STATUS_DONE;}

    raid_geometry_p = fbe_raid_group_get_raid_geometry(raid_group_p);
    fbe_raid_geometry_get_width(raid_geometry_p, &width);

    memory_request_p = fbe_transport_get_memory_request(packet_p);
    memory_request_priority = fbe_transport_get_resource_priority(packet_p);

    /* build the memory request for allocation. */
    status = fbe_memory_build_chunk_request(memory_request_p,
                                   FBE_MEMORY_CHUNK_SIZE_FOR_PACKET,
                                   width + 1,/* the +1 is for the sg elements*/
                                   memory_request_priority,
                                   fbe_transport_get_io_stamp(packet_p),
                                   (fbe_memory_completion_function_t)fbe_raid_group_journal_memory_allocation_completion, /* SAFEBUG - cast to supress warning but must fix because memory completion function shouldn't return status */
                                   raid_group_p);
    if(status == FBE_STATUS_GENERIC_FAILURE)
    {
        fbe_base_object_trace(in_object_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,                                        
                              "Setup for memory allocation failed, status:0x%x, line:%d\n", status, __LINE__);        
        
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    /* Set time stamp so we can use it to detect slow I/O.
     */
    fbe_transport_set_physical_drive_io_stamp(packet_p, fbe_get_time());
    fbe_transport_set_completion_function(packet_p, fbe_raid_group_monitor_run_verify_completion, raid_group_p);
    fbe_transport_set_completion_function(packet_p, fbe_raid_group_monitor_run_journal_verify_completion, raid_group_p); 

	fbe_transport_memory_request_set_io_master(memory_request_p, packet_p);

    /* Issue the memory request to memory service. */
    status = fbe_memory_request_entry(memory_request_p);
    if((status != FBE_STATUS_OK) && (status != FBE_STATUS_PENDING))
    {
        fbe_base_object_trace(in_object_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,                                        
                              "UZ:memory allocation failed, status:0x%x, line:%d\n", status, __LINE__);        
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);        
        fbe_transport_complete_packet(packet_p);
        return FBE_LIFECYCLE_STATUS_PENDING;
    }
    
    return FBE_LIFECYCLE_STATUS_PENDING;
}
// End fbe_raid_group_monitor_run_journal_verify()

/*!**************************************************************
 * fbe_raid_group_journal_memory_allocation_completion()
 ****************************************************************
 * @brief
 *  This function is the completion function for the memory
 *  allocation. It fills out the 64 block buffer with some pattern.
 *  The second 64 blocks is used to carver out the sg lements and to 
 *  create sg list. The subpackets are then send along the VD to do 
 *  write verify which will do the remap
 *
 * @param memory_request_p  
 * @param in_context   
 *
 * @return status - The status of the operation.
 *
 * @author
 *  03/15/2012 - Created. Ashwin Tamilarasan
 *
 ****************************************************************/
fbe_status_t fbe_raid_group_journal_memory_allocation_completion(fbe_memory_request_t * memory_request_p, 
                                                          fbe_memory_completion_context_t in_context)
{
    fbe_packet_t*                           sub_packet_p[FBE_RAID_GROUP_MAX_DISK_ARRAY_WIDTH];
    fbe_packet_t*                           packet_p = NULL;
    fbe_payload_ex_t*                       subpacket_payload_p = NULL;
    fbe_raid_group_t*                       raid_group_p = NULL;    
    fbe_memory_header_t*                    memory_header_p = NULL;
    fbe_memory_header_t*                    next_memory_header_p = NULL;
    fbe_sg_element_t*                       sg_list_p = NULL;
    fbe_block_edge_t*                       edge_p = NULL;
    fbe_payload_block_operation_t*          block_operation_p = NULL;
    fbe_u32_t                               index = 0;
    fbe_block_size_t                        optimal_block_size;
    fbe_u32_t                               sg_element_count=0;
    fbe_block_count_t                       block_count;
    fbe_raid_geometry_t*                    raid_geometry_p = NULL;
    fbe_u32_t                               width;
    fbe_u32_t                               edge_index;
    fbe_lba_t                               journal_verify_checkpoint;
    fbe_u32_t                               zero_bit_bucket_size_in_blocks;
    fbe_raid_group_nonpaged_metadata_t*     non_paged_metadata_p = NULL;
    fbe_u32_t                               used_sg_element = 0;
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_raid_group_metadata_positions_t     raid_group_metadata_positions;
    fbe_lba_t                               journal_limit;

    
    raid_group_p = (fbe_raid_group_t*)in_context;

    // get the pointer to original packet.
    packet_p = fbe_transport_memory_request_to_packet(memory_request_p);

    /*Get the optimal block size to build the block operation */
    raid_geometry_p = fbe_raid_group_get_raid_geometry(raid_group_p);
    fbe_raid_geometry_get_optimal_size(raid_geometry_p, &optimal_block_size);
    

    fbe_base_config_get_nonpaged_metadata_ptr((fbe_base_config_t *)raid_group_p, (void **)&non_paged_metadata_p);
    journal_verify_checkpoint = non_paged_metadata_p->journal_verify_checkpoint;

    /* Get the first packet chunk that was allocated. It will be used
       for the sg elements
    */
    memory_header_p = fbe_memory_get_first_memory_header(memory_request_p);
    sg_list_p = (fbe_sg_element_t*)fbe_memory_header_to_data(memory_header_p);

    /* Calculate the number of sg elements to write out 0x800 blocks(1MB) */
    fbe_memory_get_zero_bit_bucket_size_in_blocks(&zero_bit_bucket_size_in_blocks);
    block_count = fbe_raid_group_get_chunk_size(raid_group_p);
    sg_element_count = (fbe_u32_t)(block_count/zero_bit_bucket_size_in_blocks);

    /* Since verify requests can set the journal verify checkpoint to non-chunk-aligned values,
     * but we are going to verify chunks at a time,
     * truncate the block count if it goes past the end of the journal.
     */
    fbe_raid_group_get_metadata_positions(raid_group_p, &raid_group_metadata_positions);
    journal_limit =   raid_group_metadata_positions.journal_write_log_pba
                    + raid_group_metadata_positions.journal_write_log_physical_capacity;
    if (journal_verify_checkpoint + block_count > journal_limit)
    {
        block_count = journal_limit - journal_verify_checkpoint;
    }

    /* Populate the buffer with zeros and make the sgl to point to the zeroed buffer */
    status = fbe_raid_group_plant_sgl_with_zero_buffer(sg_list_p, sg_element_count, block_count, 
                                                       &used_sg_element);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s, Error in filling out the sgl with zeros\n", __FUNCTION__);

    }


    fbe_memory_get_next_memory_header(memory_header_p, &next_memory_header_p);

    while(next_memory_header_p != NULL)
    {
        sub_packet_p[index] = (fbe_packet_t*)fbe_memory_header_to_data(next_memory_header_p);
        fbe_transport_initialize_sep_packet(sub_packet_p[index]);
        subpacket_payload_p = fbe_transport_get_payload_ex(sub_packet_p[index]);

        // Setup the block operation in the packet.
        block_operation_p = fbe_payload_ex_allocate_block_operation(subpacket_payload_p);
        fbe_payload_block_build_operation(block_operation_p,
                                          FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_VERIFY,
                                          journal_verify_checkpoint,
                                          block_count,
                                          FBE_BE_BYTES_PER_BLOCK,
                                          optimal_block_size,
                                          NULL);

        /* Set the sg list pointer in the payload */
        fbe_payload_ex_set_sg_list( subpacket_payload_p, sg_list_p, sg_element_count );

        fbe_transport_add_subpacket(packet_p, sub_packet_p[index]);
        memory_header_p = next_memory_header_p;
        fbe_memory_get_next_memory_header(memory_header_p, &next_memory_header_p);
        index++;
    }

    /* Add the master packet to the terminator queue */
    fbe_raid_group_add_to_terminator_queue(raid_group_p, packet_p);

    fbe_raid_geometry_get_width(raid_geometry_p, &width);

    /* Send the subpackets along the edge to all the VDs connected to RAID */
    for(edge_index = 0; edge_index < width; edge_index++)
    {
        fbe_transport_set_completion_function(sub_packet_p[edge_index], fbe_raid_group_journal_verify_subpacket_completion,
                                              raid_group_p);

        fbe_base_config_get_block_edge((fbe_base_config_t*)raid_group_p, &edge_p, edge_index);
        fbe_block_transport_send_functional_packet(edge_p, sub_packet_p[edge_index]);
    }
    
    
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;

}   // End fbe_raid_group_journal_memory_allocation_completion()

/*!****************************************************************************
 * fbe_raid_group_journal_verify_subpacket_completion()
 ******************************************************************************
 * @brief
 *  This function checks the subpacket status, copies the status to the master packet.
 *  It releases the block operation and frees the subpacket. If the subpacket queue is empty
 *  complete the master packet
 *
 * @param packet_p  
 * @param context             
 *
 * @return status 
 *
 * @author
 *  03/16/2012 - Created. Ashwin Tamilarasan
 *
 ******************************************************************************/
fbe_status_t fbe_raid_group_journal_verify_subpacket_completion(fbe_packet_t* packet_p,
                                                       fbe_packet_completion_context_t context)
{ 
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_raid_group_t                        *raid_group_p = NULL;      
    fbe_packet_t                            *master_packet_p = NULL;    
    fbe_payload_ex_t                        *payload_p = NULL;
    fbe_payload_block_operation_t           *block_operation_p = NULL;            
    fbe_payload_block_operation_status_t     block_status;
	fbe_bool_t                               is_empty;
    
    
    raid_group_p = (fbe_raid_group_t*)context;
    
    master_packet_p = (fbe_packet_t *)fbe_transport_get_master_packet(packet_p);

    /* Get the subpacket payload and control operation. */    
    payload_p = fbe_transport_get_payload_ex(packet_p);
    block_operation_p = fbe_payload_ex_get_block_operation(payload_p);
    
    status = fbe_transport_get_status_code(packet_p); 
    fbe_payload_block_get_status(block_operation_p, &block_status);

    if((status != FBE_STATUS_OK) || (block_status != FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS) )
       
    {
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p,
                               FBE_TRACE_LEVEL_INFO,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "journal_verify failed, block_status:0x%x, status: 0x%x\n", 
                                block_status, status);
        fbe_transport_set_status(master_packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
    }    
   
    fbe_payload_ex_release_block_operation(payload_p, block_operation_p);  
    fbe_transport_remove_subpacket_is_queue_empty(packet_p, &is_empty);
               
    if (is_empty)
    {
		fbe_transport_destroy_subpackets(master_packet_p);
        /* Remove master packet from the terminator queue. */
        fbe_raid_group_remove_from_terminator_queue(raid_group_p, master_packet_p);
        //fbe_transport_set_status(master_packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(master_packet_p); 
    }
    
    return FBE_STATUS_OK;
}
/*************************************************************************** 
 * end of fbe_raid_group_journal_verify_subpacket_completion()
 ***************************************************************************/

/*!****************************************************************************
 * fbe_raid_group_monitor_run_journal_verify_completion()
 ******************************************************************************
 * @brief
 *  This function checks the block status and if it is successful, that means
 *  the remap is completed, increment the journal verify checkpoint and 
 *  if we are at the end, set the checkpoint to invalid
 *
 * @param packet_p  
 * @param context   
 *
 * @return status 
 *
 * @author
 *  03/16/2012 - Created. Ashwin Tamilarasan
 *
 ******************************************************************************/
fbe_status_t fbe_raid_group_monitor_run_journal_verify_completion(fbe_packet_t *packet_p,
                                                              fbe_packet_completion_context_t context)
{  
    fbe_raid_group_t                   *raid_group_p = NULL;
    fbe_payload_ex_t                   *sep_payload_p = NULL;
    fbe_memory_request_t               *memory_request_p = NULL;
    fbe_raid_group_nonpaged_metadata_t*  nonpaged_metadata_p = NULL;
    fbe_lba_t                            current_checkpoint;
    fbe_lba_t                            new_checkpoint;
    fbe_block_count_t                    block_count;
    fbe_lba_t                           imported_capacity;
    fbe_lba_t                           per_disk_imported_capacity;
    fbe_raid_geometry_t*                raid_geometry_p = NULL;
    fbe_u16_t                           data_disks;
    fbe_u64_t                           metadata_offset;
    fbe_scheduler_hook_status_t          hook_status = FBE_SCHED_STATUS_OK;
    fbe_status_t                        status;
     
    raid_group_p = (fbe_raid_group_t*)context;
    status = fbe_transport_get_status_code(packet_p);
            
    /* Free up the memory that was allocated        
    */
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    memory_request_p = fbe_transport_get_memory_request(packet_p);
    //fbe_memory_request_get_entry_mark_free(memory_request_p, &memory_ptr);
    //fbe_memory_free_entry(memory_ptr);               
	fbe_memory_free_request_entry(memory_request_p);

    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p,
                              FBE_TRACE_LEVEL_INFO, 
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s:status:0x%x\n", 
                              __FUNCTION__, status);

        fbe_raid_group_check_hook(raid_group_p,  SCHEDULER_MONITOR_STATE_RAID_GROUP_JOURNAL_VERIFY, 
                                  FBE_RAID_GROUP_SUBSTATE_JOURNAL_VERIFY_ERROR, 0, &hook_status);

        return FBE_STATUS_OK;

    }


    fbe_raid_group_get_imported_capacity(raid_group_p, &imported_capacity);
    raid_geometry_p = fbe_raid_group_get_raid_geometry(raid_group_p);
    fbe_raid_geometry_get_data_disks(raid_geometry_p, &data_disks);
    per_disk_imported_capacity = imported_capacity / data_disks;

    fbe_base_config_get_nonpaged_metadata_ptr((fbe_base_config_t*)raid_group_p, (void**)&nonpaged_metadata_p);

    current_checkpoint = nonpaged_metadata_p->journal_verify_checkpoint;
    block_count = fbe_raid_group_get_chunk_size(raid_group_p);

    metadata_offset = (fbe_u64_t)(&((fbe_raid_group_nonpaged_metadata_t*)0)->journal_verify_checkpoint);
    new_checkpoint = current_checkpoint + block_count;
    /* If we are at the end, get the NP lock and set the checkpoint to invalid */
    if(new_checkpoint >= per_disk_imported_capacity)
    {
        fbe_raid_group_get_NP_lock(raid_group_p,packet_p, fbe_raid_group_set_journal_checkpoint_to_invalid);

        return FBE_STATUS_MORE_PROCESSING_REQUIRED;

    }

    /* Otherwise just increment the checkpoint */
    else
    {
        fbe_base_config_metadata_nonpaged_incr_checkpoint((fbe_base_config_t *)raid_group_p,
                                                          packet_p,
                                                          metadata_offset,
                                                          0, /* There is only (1) checkpoint for a verify */
                                                          current_checkpoint,
                                                          block_count);

        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }
    
    return FBE_STATUS_OK;    

}
/*****************************************************************
 * end of fbe_raid_group_monitor_run_journal_verify_completion
 *****************************************************************/

/*!****************************************************************************
 * fbe_raid_group_set_journal_checkpoint_to_invalid()
 ******************************************************************************
 * @brief
 *   This function will set the journal verify checkpoint to invalid 
 * 
 * @param packet_p
 * @param context
 * 
 * @return  fbe_status_t  
 *
 * @author
 *   03/14/2012 - Created. Ashwin Tamilarasan
 *
 ******************************************************************************/
fbe_status_t fbe_raid_group_set_journal_checkpoint_to_invalid(fbe_packet_t * packet_p,                                    
                                        fbe_packet_completion_context_t context)

{
    fbe_raid_group_t*                    raid_group_p = NULL;   
    fbe_status_t                         status;
    fbe_lba_t                            new_checkpoint;
    fbe_u64_t                            metadata_offset = FBE_LBA_INVALID;
    
    raid_group_p = (fbe_raid_group_t*)context;

    /* If the status is not ok, that means we didn't get the 
       lock. Just return. we are already in the completion routine
    */
    status = fbe_transport_get_status_code (packet_p);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, 
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s can't get the NP lock, status: 0x%X", __FUNCTION__, status); 
        return FBE_STATUS_OK;
    }

    // Set the completion function to release the NP lock
    fbe_transport_set_completion_function(packet_p, fbe_raid_group_release_NP_lock, raid_group_p);  
    
    fbe_base_object_trace((fbe_base_object_t*)raid_group_p,
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s Setting the journal verify checkpoint to invalid.\n",
                          __FUNCTION__);
    
    new_checkpoint = FBE_LBA_INVALID;

    metadata_offset = (fbe_u64_t)(&((fbe_raid_group_nonpaged_metadata_t*)0)->journal_verify_checkpoint);
    status = fbe_base_config_metadata_nonpaged_write_persist((fbe_base_config_t*) raid_group_p, packet_p, 
                                                             metadata_offset, 
                                                             (fbe_u8_t*)&new_checkpoint,
                                                             sizeof(fbe_lba_t));


    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
               
} // End fbe_raid_group_set_journal_checkpoint_to_invalid()


/*!**************************************************************
 * fbe_raid_group_journal_remap_memory_allocation_completion()
 ****************************************************************
 * @brief
 *  This function is the completion function for the memory allocation.
 *  Resources are carved out and packets are sent along the VD to do 
 *  write verify which will do the remap. Zero bit-bucket is used as write
 *  buffer such that write_log slots will have all zeros. 
 *
 * @param memory_request_p  
 * @param in_context   
 *
 * @return status - The status of the operation.
 *
 * @author
 *  03/15/2012 - Created. Vamsi V.
 *
 ****************************************************************/
fbe_status_t fbe_raid_group_journal_remap_memory_allocation_completion(fbe_memory_request_t * memory_request_p, 
                                                                       fbe_memory_completion_context_t in_context)
{
    fbe_packet_t*                           sub_packet_p[FBE_RAID_GROUP_MAX_DISK_ARRAY_WIDTH];
    fbe_packet_t*                           packet_p = NULL;
    fbe_payload_ex_t*                       subpacket_payload_p = NULL;
    fbe_raid_group_t*                       raid_group_p = NULL;    
    fbe_memory_header_t*                    memory_header_p = NULL;
    fbe_memory_header_t*                    next_memory_header_p = NULL;
    fbe_sg_element_t*                       sg_list_p = NULL;
    fbe_block_edge_t*                       edge_p = NULL;
    fbe_payload_block_operation_t*          block_operation_p = NULL;
    fbe_u32_t                               index = 0;
    fbe_block_size_t                        optimal_block_size;
    fbe_u32_t                               sg_element_count=0;
    fbe_raid_geometry_t*                    raid_geometry_p = NULL;
    fbe_u32_t                               width;
    fbe_u32_t                               edge_index;
    fbe_u32_t                               zero_bit_bucket_size_in_blocks;
    fbe_u32_t                               used_sg_element = 0;
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_payload_ex_t *                      sep_payload_p;
    fbe_raid_iots_t *                       iots_p;
    fbe_u32_t                               slot_id;
    fbe_lba_t                               write_log_start_lba;
    fbe_u32_t                               write_log_slot_size;
    fbe_raid_position_bitmask_t             rl_bitmask;
    fbe_bool_t                              b_new_key = FBE_FALSE;

    raid_group_p = (fbe_raid_group_t*)in_context;

    /* Get the pointer to original packet */
    packet_p = fbe_transport_memory_request_to_packet(memory_request_p);

    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    fbe_payload_ex_get_iots(sep_payload_p, &iots_p);
    fbe_raid_iots_get_journal_slot_id(iots_p, &slot_id);
    if (slot_id == FBE_PARITY_WRITE_LOG_INVALID_SLOT)
    {
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s Write_log slot_id is invalid. iots_p 0x%p, slot 0x%x. \n",
                              __FUNCTION__, iots_p, slot_id);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Initialize the array */
    for (index = 0; index < FBE_RAID_GROUP_MAX_DISK_ARRAY_WIDTH; index++)
    {
        sub_packet_p[index] = NULL;
    }

    /* Get width and degraded postions */
    raid_geometry_p = fbe_raid_group_get_raid_geometry(raid_group_p);
    fbe_raid_geometry_get_width(raid_geometry_p, &width);
    status = fbe_raid_group_get_rb_logging_bitmask(raid_group_p, &rl_bitmask);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s, Write_log error getting rebuild logging bitmask \n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Get write_log slot information */
    fbe_raid_geometry_get_journal_log_start_lba(raid_geometry_p, &write_log_start_lba); 
    fbe_raid_geometry_get_journal_log_slot_size(raid_geometry_p, &write_log_slot_size);

    /* Increment lba to slot we need to flush */
    write_log_start_lba = (write_log_start_lba + (slot_id * write_log_slot_size));

    /*Get the optimal block size to build the block operation */
    fbe_raid_geometry_get_optimal_size(raid_geometry_p, &optimal_block_size);

    /* Get the first packet chunk that was allocated. It will be used
     * for the sg elements.
     */
    memory_header_p = fbe_memory_get_first_memory_header(memory_request_p);
    sg_list_p = (fbe_sg_element_t*)fbe_memory_header_to_data(memory_header_p);

    /* Calculate the number of sg elements to write out 0x800 blocks(1MB) */
    fbe_memory_get_zero_bit_bucket_size_in_blocks(&zero_bit_bucket_size_in_blocks);
    sg_element_count = (fbe_u32_t)(FBE_MAX (1, (write_log_slot_size/zero_bit_bucket_size_in_blocks)));

    /* Populate the buffer with zeros and make the sgl to point to the zeroed buffer */
    status = fbe_raid_group_plant_sgl_with_zero_buffer(sg_list_p, sg_element_count, write_log_slot_size, 
                                                       &used_sg_element);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s, Write_log error in filling out the sgl with zeros\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }


    for (edge_index = 0; edge_index < width; edge_index++)
    {
        if (((1 << edge_index) & rl_bitmask) == 0)
        {
            fbe_memory_get_next_memory_header(memory_header_p, &next_memory_header_p);

            if (next_memory_header_p == NULL)
            {
                fbe_base_object_trace((fbe_base_object_t*)raid_group_p,
                                      FBE_TRACE_LEVEL_ERROR,
                                      FBE_TRACE_MESSAGE_ID_INFO,
                                      "%s, Write_log error accessing memory for subpackets \n", __FUNCTION__);
                return FBE_STATUS_GENERIC_FAILURE;
            }

            sub_packet_p[edge_index] = (fbe_packet_t*)fbe_memory_header_to_data(next_memory_header_p);
            fbe_transport_initialize_sep_packet(sub_packet_p[edge_index]);
            subpacket_payload_p = fbe_transport_get_payload_ex(sub_packet_p[edge_index]);

            // Setup the block operation in the packet.
            block_operation_p = fbe_payload_ex_allocate_block_operation(subpacket_payload_p);
            fbe_payload_block_build_operation(block_operation_p,
                                              FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_VERIFY,
                                              write_log_start_lba,
                                              write_log_slot_size,
                                              FBE_BE_BYTES_PER_BLOCK,
                                              optimal_block_size,
                                              NULL);

            /* Set the sg list pointer in the payload */
            fbe_payload_ex_set_sg_list( subpacket_payload_p, sg_list_p, sg_element_count );
            
            /* During encryption we will set the new key for initializing the journal.
             */
            if (fbe_raid_group_is_encryption_state_set(raid_group_p, FBE_RAID_GROUP_ENCRYPTION_FLAG_PAGED_NEEDS_RECONSTRUCT) ||
                fbe_raid_group_is_encryption_state_set(raid_group_p, FBE_RAID_GROUP_ENCRYPTION_FLAG_PAGED_ENCRYPTED)) {
                fbe_payload_block_set_flag(block_operation_p, FBE_PAYLOAD_BLOCK_OPERATION_FLAGS_USE_NEW_KEY);
                b_new_key = FBE_TRUE;
            }
            fbe_transport_add_subpacket(packet_p, sub_packet_p[edge_index]);
            memory_header_p = next_memory_header_p;
            fbe_memory_get_next_memory_header(memory_header_p, &next_memory_header_p);
        }
    }

    /* Add the master packet to the terminator queue */
    fbe_raid_group_add_to_terminator_queue(raid_group_p, packet_p);

    fbe_base_object_trace((fbe_base_object_t*)raid_group_p,
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          " Issuing Write_log remap cmd for slot_id:0x%x. rg width:0x%x skipping pos rl_bm:0x%x nwky: %u\n", 
                          slot_id, width, rl_bitmask, b_new_key);

    /* Send the subpackets along the edge to all the VDs connected to RAID */
    for (edge_index = 0; edge_index < width; edge_index++)
    {
        if (((1 << edge_index) & rl_bitmask) == 0)
        {
            fbe_transport_set_completion_function(sub_packet_p[edge_index], fbe_raid_group_journal_remap_subpacket_completion,
                                                  raid_group_p);

            fbe_base_config_get_block_edge((fbe_base_config_t*)raid_group_p, &edge_p, edge_index);
            fbe_block_transport_send_functional_packet(edge_p, sub_packet_p[edge_index]);
        }
    }

    return FBE_STATUS_MORE_PROCESSING_REQUIRED;

}   // End fbe_raid_group_journal_remap_memory_allocation_completion()

/*!****************************************************************************
 * fbe_raid_group_journal_remap_subpacket_completion()
 ******************************************************************************
 * @brief
 *  This function checks the subpacket status, copies the status to the master packet.
 *  It releases the block operation and frees the subpacket. If the subpacket queue is empty
 *  complete the master packet
 *
 * @param packet_p  
 * @param context             
 *
 * @return status 
 *
 * @author
 *  08/03/2012 - Created. Vamsi V.
 *
 ******************************************************************************/
fbe_status_t fbe_raid_group_journal_remap_subpacket_completion(fbe_packet_t* packet_p,
                                                               fbe_packet_completion_context_t context)
{ 
    fbe_status_t                                status = FBE_STATUS_OK;
    fbe_raid_group_t *                          raid_group_p = NULL;      
    fbe_packet_t *                              master_packet_p = NULL;    
    fbe_payload_ex_t *                          payload_p = NULL;
    fbe_payload_block_operation_t *             block_operation_p = NULL;            
    fbe_payload_block_operation_status_t        block_status;
    fbe_payload_block_operation_qualifier_t     block_qual;
    fbe_bool_t                                  is_empty;


    raid_group_p = (fbe_raid_group_t*)context;

    master_packet_p = (fbe_packet_t *)fbe_transport_get_master_packet(packet_p);

    /* Get the subpacket payload and control operation. */    
    payload_p = fbe_transport_get_payload_ex(packet_p);
    block_operation_p = fbe_payload_ex_get_block_operation(payload_p);

    status = fbe_transport_get_status_code(packet_p); 
    fbe_payload_block_get_status(block_operation_p, &block_status);
    fbe_payload_block_get_qualifier(block_operation_p, &block_qual);

    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s Write_log remap failed, transport_status:0x%x, server_id: 0x%x, lba: 0x%x blks: 0x%x \n", 
                              __FUNCTION__, status, block_operation_p->block_edge_p->base_edge.server_id, (unsigned int)block_operation_p->lba, 
                              (unsigned int)block_operation_p->block_count);
        fbe_transport_set_status(master_packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
    }

    if (!((block_status == FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS) && (block_qual == FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE)))
    {
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s Write_log remap failed, blk_status:0x%x, blk_qual:0x%x, server_id: 0x%x, lba: 0x%x blks: 0x%x \n", 
                              __FUNCTION__, block_status, block_qual, block_operation_p->block_edge_p->base_edge.server_id, 
                              (unsigned int)block_operation_p->lba, (unsigned int)block_operation_p->block_count);
        fbe_transport_set_status(master_packet_p, FBE_STATUS_GENERIC_FAILURE, 0);       
    }

    fbe_payload_ex_release_block_operation(payload_p, block_operation_p);  
    fbe_transport_remove_subpacket_is_queue_empty(packet_p, &is_empty);

    if (is_empty)
    {
        fbe_transport_destroy_subpackets(master_packet_p);
        /* Remove master packet from the terminator queue. */
        fbe_raid_group_remove_from_terminator_queue(raid_group_p, master_packet_p);
        //fbe_transport_set_status(master_packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(master_packet_p); 
    }

    return FBE_STATUS_OK;
}
/*************************************************************************** 
 * end of fbe_raid_group_journal_remap_subpacket_completion()
 ***************************************************************************/


/*!**************************************************************
 * fbe_raid_group_monitor_initialize_journal()
 ****************************************************************
 * @brief
 *  This function initializes the journal to zeros.
 *  We must initialize the journal by writing it at init time
 *  because we do not want to allow zero on demand for journal slots.
 *  RAID always writes/reads the journal slots using the
 *  FBE_PACKET_FLAG_CONSUMED, which means we bypass all
 *  zero on demand logic.
 *
 * @param in_object_p - The pointer to the raid group.
 * @param in_packet_p - The control packet sent to us from the scheduler.
 *
 * @return fbe_lifecycle_status_t - FBE_LIFECYCLE_STATUS_PENDING 
 *
 * @author
 *  3/14/2013 - Created. Rob Foley
 *
 ****************************************************************/
fbe_lifecycle_status_t fbe_raid_group_monitor_initialize_journal(fbe_base_object_t*  in_object_p,
                                                                 fbe_packet_t*       packet_p)
{
    fbe_raid_group_t*                    raid_group_p = NULL;
    fbe_raid_geometry_t*                 raid_geometry_p = NULL;
    fbe_memory_request_t*                memory_request_p = NULL;
    fbe_packet_resource_priority_t       memory_request_priority;
    fbe_u32_t                            width;
    fbe_status_t                         status;
    fbe_scheduler_hook_status_t          hook_status = FBE_SCHED_STATUS_OK;
    
    FBE_RAID_GROUP_TRACE_FUNC_ENTRY(in_object_p);


    raid_group_p = (fbe_raid_group_t*)in_object_p;

    fbe_raid_group_check_hook(raid_group_p,  SCHEDULER_MONITOR_STATE_RAID_GROUP_JOURNAL_VERIFY, 
                                  FBE_RAID_GROUP_SUBSTATE_JOURNAL_VERIFY_STARTED, 0, &hook_status);
    if (hook_status == FBE_SCHED_STATUS_DONE) {return FBE_LIFECYCLE_STATUS_DONE;}

    raid_geometry_p = fbe_raid_group_get_raid_geometry(raid_group_p);
    fbe_raid_geometry_get_width(raid_geometry_p, &width);

    memory_request_p = fbe_transport_get_memory_request(packet_p);
    memory_request_priority = fbe_transport_get_resource_priority(packet_p);

    status = fbe_memory_build_chunk_request(memory_request_p,
                                   FBE_MEMORY_CHUNK_SIZE_FOR_PACKET,
                                   width + 1,/* the +1 is for the sg elements*/
                                   memory_request_priority,
                                   fbe_transport_get_io_stamp(packet_p),
                                   (fbe_memory_completion_function_t)fbe_raid_group_init_journal_memory_allocation_completion, /* SAFEBUG - cast to supress warning but must fix because memory completion function shouldn't return status */
                                   raid_group_p);
    if(status == FBE_STATUS_GENERIC_FAILURE)
    {
        fbe_base_object_trace(in_object_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,                                        
                              "Setup for memory allocation failed, status:0x%x, line:%d\n", status, __LINE__);        
        
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_set_completion_function(packet_p, fbe_raid_group_monitor_init_journal_completion, raid_group_p); 

	fbe_transport_memory_request_set_io_master(memory_request_p, packet_p);

    status = fbe_memory_request_entry(memory_request_p);
    if((status != FBE_STATUS_OK) && (status != FBE_STATUS_PENDING))
    {
        fbe_base_object_trace(in_object_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,                                        
                              "UZ:memory allocation failed, status:0x%x, line:%d\n", status, __LINE__);        
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);        
        fbe_transport_complete_packet(packet_p);
        return FBE_LIFECYCLE_STATUS_PENDING;
    }
    
    return FBE_LIFECYCLE_STATUS_PENDING;
}
// End fbe_raid_group_monitor_initialize_journal()


/*!**************************************************************
 * fbe_raid_group_init_journal_memory_allocation_completion()
 ****************************************************************
 * @brief
 *  This function is the completion function for the memory
 *  allocation. It fills out the 64 block buffer with some pattern.
 *  The second 64 blocks is used to carver out the sg lements and to 
 *  create sg list. The subpackets are then send along the VD to do 
 *  write verify which will init the slot.
 *
 * @param memory_request_p  
 * @param in_context   
 *
 * @return status - The status of the operation.
 *
 * @author
 *  3/14/2013 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t fbe_raid_group_init_journal_memory_allocation_completion(fbe_memory_request_t * memory_request_p, 
                                                                      fbe_memory_completion_context_t in_context)
{
    fbe_packet_t*                           sub_packet_p[FBE_RAID_GROUP_MAX_DISK_ARRAY_WIDTH];
    fbe_packet_t*                           packet_p = NULL;
    fbe_payload_ex_t*                       subpacket_payload_p = NULL;
    fbe_raid_group_t*                       raid_group_p = NULL;    
    fbe_memory_header_t*                    memory_header_p = NULL;
    fbe_memory_header_t*                    next_memory_header_p = NULL;
    fbe_sg_element_t*                       sg_list_p = NULL;
    fbe_block_edge_t*                       edge_p = NULL;
    fbe_payload_block_operation_t*          block_operation_p = NULL;
    fbe_u32_t                               index = 0;
    fbe_block_size_t                        optimal_block_size;
    fbe_u32_t                               sg_element_count=0;
    fbe_block_count_t                       block_count;
    fbe_raid_geometry_t*                    raid_geometry_p = NULL;
    fbe_u32_t                               width;
    fbe_u32_t                               edge_index;
    fbe_lba_t                               journal_verify_checkpoint;
    fbe_u32_t                               zero_bit_bucket_size_in_blocks;
    fbe_u32_t                               used_sg_element = 0;
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_raid_iots_t *iots_p = NULL;
    fbe_payload_ex_t *payload_p = NULL;
    fbe_raid_position_bitmask_t rl_bitmask;

    raid_group_p = (fbe_raid_group_t*)in_context;
    packet_p = fbe_transport_memory_request_to_packet(memory_request_p);
    raid_geometry_p = fbe_raid_group_get_raid_geometry(raid_group_p);
    fbe_raid_geometry_get_optimal_size(raid_geometry_p, &optimal_block_size);
    payload_p = fbe_transport_get_payload_ex(packet_p);

    /* Our iots was originally set with the lba we want to start at.
     */
    fbe_payload_ex_get_iots(payload_p, &iots_p);
    journal_verify_checkpoint = iots_p->lba;

    /* Our first buffer contains our sg list. 
     * We only need one sg list since we will be writing the same data (zeros) 
     * to all drives. 
     */
    memory_header_p = fbe_memory_get_first_memory_header(memory_request_p);
    sg_list_p = (fbe_sg_element_t*)fbe_memory_header_to_data(memory_header_p);

    /* Calculate the number of sg elements to write out 0x800 blocks(1MB) */
    fbe_memory_get_zero_bit_bucket_size_in_blocks(&zero_bit_bucket_size_in_blocks);
    block_count = fbe_raid_group_get_chunk_size(raid_group_p);
    sg_element_count = (fbe_u32_t)(block_count/zero_bit_bucket_size_in_blocks);

    /* Populate the buffer with zeros and make the sgl to point to the zeroed buffer
     */
    status = fbe_raid_group_plant_sgl_with_zero_buffer(sg_list_p, sg_element_count, block_count, 
                                                       &used_sg_element);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s, Error in filling out the sgl with zeros\n", __FUNCTION__);

    }
    /* The remaining buffers contain our packets.
     */
    fbe_memory_get_next_memory_header(memory_header_p, &next_memory_header_p);

    fbe_raid_group_get_rb_logging_bitmask(raid_group_p, &rl_bitmask);

    while(next_memory_header_p != NULL)
    {
        sub_packet_p[index] = (fbe_packet_t*)fbe_memory_header_to_data(next_memory_header_p);

        /* Only send to an edge if we are not initialized or if we not marked rebuild logging.
         */
        if ( ((1 << index) & rl_bitmask) == 0) {
            
            fbe_transport_initialize_sep_packet(sub_packet_p[index]);
            subpacket_payload_p = fbe_transport_get_payload_ex(sub_packet_p[index]);
    
            block_operation_p = fbe_payload_ex_allocate_block_operation(subpacket_payload_p);
            fbe_payload_block_build_operation(block_operation_p,
                                              FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_VERIFY,
                                              journal_verify_checkpoint,
                                              block_count,
                                              FBE_BE_BYTES_PER_BLOCK,
                                              optimal_block_size,
                                              NULL);
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, FBE_TRACE_LEVEL_DEBUG_MEDIUM, FBE_TRACE_MESSAGE_ID_INFO,
                                  "raid: send init of journal lba: 0x%llx bl: 0x%llx pos: [%d]\n",
                                  journal_verify_checkpoint,
                                  block_count,
                                  index);
            fbe_payload_ex_set_sg_list( subpacket_payload_p, sg_list_p, sg_element_count );

            /* During encryption we will set the new key for initializing the journal.
             */
            if (fbe_raid_group_is_encryption_state_set(raid_group_p, FBE_RAID_GROUP_ENCRYPTION_FLAG_PAGED_NEEDS_RECONSTRUCT) ||
                fbe_raid_group_is_encryption_state_set(raid_group_p, FBE_RAID_GROUP_ENCRYPTION_FLAG_PAGED_ENCRYPTED)) {
                fbe_payload_block_set_flag(block_operation_p, FBE_PAYLOAD_BLOCK_OPERATION_FLAGS_USE_NEW_KEY);
            }
            fbe_transport_add_subpacket(packet_p, sub_packet_p[index]);
        }
        memory_header_p = next_memory_header_p;
        fbe_memory_get_next_memory_header(memory_header_p, &next_memory_header_p);
        index++;
    }
    /* Set status to good here, we will set status to failure for any failures we find.
     */
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_raid_group_add_to_terminator_queue(raid_group_p, packet_p);
    fbe_raid_geometry_get_width(raid_geometry_p, &width);

    /* Send the subpackets along the edge to all the VDs connected to RAID.
     */
    for(edge_index = 0; edge_index < width; edge_index++)
    {
        if ( ((1 << edge_index) & rl_bitmask) == 0) {
            fbe_transport_set_completion_function(sub_packet_p[edge_index], fbe_raid_group_journal_init_subpacket_completion,
                                                  raid_group_p);
            fbe_base_config_get_block_edge((fbe_base_config_t*)raid_group_p, &edge_p, edge_index);
            fbe_block_transport_send_functional_packet(edge_p, sub_packet_p[edge_index]);
        }
    }
    
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;

}   // End fbe_raid_group_init_journal_memory_allocation_completion()

/*!****************************************************************************
 * fbe_raid_group_journal_init_subpacket_completion()
 ******************************************************************************
 * @brief
 *  This function checks the subpacket status, copies the status to the master packet.
 *  It releases the block operation and frees the subpacket. If the subpacket queue is empty
 *  complete the master packet
 *
 * @param packet_p  
 * @param context             
 *
 * @return status 
 *
 * @author
 *  3/14/2013 - Created. Rob Foley
 *
 ******************************************************************************/
fbe_status_t fbe_raid_group_journal_init_subpacket_completion(fbe_packet_t* packet_p,
                                                              fbe_packet_completion_context_t context)
{ 
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_raid_group_t                        *raid_group_p = NULL;      
    fbe_packet_t                            *master_packet_p = NULL;    
    fbe_payload_ex_t                        *payload_p = NULL;
    fbe_payload_block_operation_t           *block_operation_p = NULL;            
    fbe_payload_block_operation_status_t     block_status;
	fbe_bool_t                               is_empty;
    
    raid_group_p = (fbe_raid_group_t*)context;
    master_packet_p = (fbe_packet_t *)fbe_transport_get_master_packet(packet_p);
    payload_p = fbe_transport_get_payload_ex(packet_p);
    block_operation_p = fbe_payload_ex_get_block_operation(payload_p);
    
    status = fbe_transport_get_status_code(packet_p); 
    fbe_payload_block_get_status(block_operation_p, &block_status);

    if((status != FBE_STATUS_OK) || (block_status != FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS) )
       
    {
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p,
                               FBE_TRACE_LEVEL_INFO,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "journal_init failed, block_status:0x%x, status: 0x%x\n", 
                                block_status, status);
        fbe_transport_set_status(master_packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
    }    
   
    fbe_payload_ex_release_block_operation(payload_p, block_operation_p);
    fbe_transport_remove_subpacket_is_queue_empty(packet_p, &is_empty);
               
    if (is_empty)
    {
		fbe_transport_destroy_subpackets(master_packet_p);
        fbe_raid_group_remove_from_terminator_queue(raid_group_p, master_packet_p);
        fbe_transport_complete_packet(master_packet_p); 
    }
    
    return FBE_STATUS_OK;
}
/*************************************************************************** 
 * end of fbe_raid_group_journal_init_subpacket_completion()
 ***************************************************************************/
/*!****************************************************************************
 * fbe_raid_group_monitor_init_journal_completion()
 ******************************************************************************
 * @brief
 *  This is the completion for a pass of the journal init.
 *  If the status is bad we complete back to the monitor.
 *  If the status is good, then if we need to do more we kick that off.
 *  otherwise we are done and will free up memory and complete the packet.
 *
 * @param packet_p  
 * @param context   
 *
 * @return status 
 *
 * @author
 *  3/14/2013 - Created. Rob Foley
 *
 ******************************************************************************/
fbe_status_t fbe_raid_group_monitor_init_journal_completion(fbe_packet_t *packet_p,
                                                            fbe_packet_completion_context_t context)
{  
    fbe_raid_group_t                   *raid_group_p = NULL;
    fbe_payload_ex_t                   *sep_payload_p = NULL;
    fbe_memory_request_t               *memory_request_p = NULL;
    fbe_lba_t                            new_checkpoint;
    fbe_block_count_t                    block_count;
    fbe_lba_t                           imported_capacity;
    fbe_lba_t                           per_disk_imported_capacity;
    fbe_raid_geometry_t*                raid_geometry_p = NULL;
    fbe_u16_t                           data_disks;
    fbe_scheduler_hook_status_t          hook_status = FBE_SCHED_STATUS_OK;
    fbe_status_t                        status;
    fbe_raid_iots_t *iots_p = NULL;
     
    raid_group_p = (fbe_raid_group_t*)context;
    status = fbe_transport_get_status_code(packet_p);
    
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    memory_request_p = fbe_transport_get_memory_request(packet_p);

    /* Our iots was originally set with the lba we want to start at.
     */
    fbe_payload_ex_get_iots(sep_payload_p, &iots_p);

    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p,
                              FBE_TRACE_LEVEL_INFO, 
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s:status:0x%x\n", 
                              __FUNCTION__, status);
        
        /* Remove master packet from the terminator queue. */
        fbe_memory_free_request_entry(memory_request_p);
        fbe_raid_group_check_hook(raid_group_p,  SCHEDULER_MONITOR_STATE_RAID_GROUP_JOURNAL_VERIFY, 
                                  FBE_RAID_GROUP_SUBSTATE_JOURNAL_VERIFY_ERROR, 0, &hook_status);

        return FBE_STATUS_OK;
    }

    fbe_raid_group_get_imported_capacity(raid_group_p, &imported_capacity);
    raid_geometry_p = fbe_raid_group_get_raid_geometry(raid_group_p);
    fbe_raid_geometry_get_data_disks(raid_geometry_p, &data_disks);
    per_disk_imported_capacity = imported_capacity / data_disks;

    block_count = fbe_raid_group_get_chunk_size(raid_group_p);

    new_checkpoint = iots_p->lba + block_count;

    if(new_checkpoint >= per_disk_imported_capacity)
    {
        fbe_memory_free_request_entry(memory_request_p);
        fbe_transport_complete_packet(packet_p); 
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }
    else
    {
        /* Not done, continue with the next piece.
         */
        iots_p->lba += block_count;
        fbe_transport_set_completion_function(packet_p, fbe_raid_group_monitor_init_journal_completion, raid_group_p);
        fbe_raid_group_init_journal_memory_allocation_completion(memory_request_p, raid_group_p);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }
    return FBE_STATUS_OK;
}
/*****************************************************************
 * end of fbe_raid_group_monitor_init_journal_completion
 *****************************************************************/
/*********************************************
 * end file fbe_raid_group_journal_verify.c
 ********************************************/

