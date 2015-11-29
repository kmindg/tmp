/***************************************************************************
 * Copyright (C) EMC Corporation 2012
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_raid_group_journal_rebuild.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the raid group code that will rebuild the journal
 *  for a drive that is rebuilding.
 * 
 *  The journal rebuild is essentially a write of zeros to the rebuilding drive.
 * 
 * @version
 *  5/30/2012 - Created. Rob Foley
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
#include "fbe_raid_group_rebuild.h"

/*****************************************
 * LOCAL FUNCTION PROTOTYPES
 *****************************************/

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

fbe_status_t fbe_raid_group_journal_rebuild_allocate(fbe_raid_group_t *raid_group_p,
                                                     fbe_packet_t *packet_p);
fbe_status_t fbe_raid_group_journal_rebuild_allocation_completion(fbe_memory_request_t * memory_request_p, 
                                                                  fbe_memory_completion_context_t in_context);
fbe_status_t fbe_raid_group_journal_rebuild_op_completion(fbe_packet_t *packet_p,
                                                          fbe_packet_completion_context_t context);
fbe_status_t fbe_raid_group_journal_rebuild_subpacket_completion(fbe_packet_t* packet_p,
                                                                 fbe_packet_completion_context_t context);

/*!****************************************************************************
 * fbe_raid_group_journal_chunk_rebuild_completion()
 ******************************************************************************
 * @brief
 *   This is the completion function for the journal rebuild function.  It is invoked
 *   when a journal rebuild I/O operation has completed.  It launches the next io
 *   immediately, keeping track of progress via the rebuild request. Since journal
 *   rebuild is no longer part of the monitor background condition, we cannot use
 *   the same checkpoint mechanism used by metadata and user data rebuild.
 *
 * @param in_packet_p               - pointer to a control packet from the scheduler
 * @param in_context                - context, which is a pointer to the base object 
 * 
 * @return fbe_status_t             - Always FBE_STATUS_OK
 *   
 * Note: This function must always return FBE_STATUS_OK so that the packet gets
 * completed to the next level.  (If any other status is returned, the packet will 
 * get stuck.)
 *
 * @author
 *   30/04/2013 - Dave Agans  Created from fbe_raid_group_monitor_run_rebuild_completion.
 *
 ******************************************************************************/
fbe_status_t fbe_raid_group_journal_chunk_rebuild_completion(
                                fbe_packet_t*                   in_packet_p,
                                fbe_packet_completion_context_t in_context)
{

    fbe_status_t                        status;                     //  fbe status
    fbe_raid_group_t                   *raid_group_p;               //  pointer to the raid group 
    fbe_payload_ex_t                   *sep_payload_p;              //  pointer to the payload
    fbe_payload_control_operation_t    *control_operation_p;        //  pointer to the control operation
    fbe_raid_group_rebuild_context_t   *rebuild_context_p;          //  rebuild monitor's context information
    fbe_payload_control_status_t        control_status;
    fbe_lba_t                           start_lba;
    fbe_lba_t                           block_count;
    fbe_lba_t                           journal_log_end_lba;
    fbe_raid_geometry_t                *raid_geometry_p;

    //  Cast the context to a pointer to a raid group object and get journal info
    raid_group_p = (fbe_raid_group_t*) in_context;
    raid_geometry_p = fbe_raid_group_get_raid_geometry(raid_group_p);
    fbe_raid_geometry_get_journal_log_end_lba(raid_geometry_p, &journal_log_end_lba);

    //  Get the sep payload and control operation
    sep_payload_p = fbe_transport_get_payload_ex(in_packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(sep_payload_p);
    status = fbe_transport_get_status_code(in_packet_p);

    //  Release the payload buffer
    fbe_payload_control_get_buffer(control_operation_p, &rebuild_context_p);
    fbe_payload_control_get_status(control_operation_p, &control_status);

    /* If enabled trace some information.
     */
    fbe_raid_group_trace(raid_group_p,
                         FBE_TRACE_LEVEL_INFO,
                         FBE_RAID_GROUP_DEBUG_FLAG_REBUILD_TRACING,
                         "raid_group: journal rebuild complete lba: 0x%llx blocks: 0x%llx state: %d status: 0x%x \n",
                         (unsigned long long)rebuild_context_p->start_lba,
                         (unsigned long long)rebuild_context_p->block_count,
                         rebuild_context_p->current_state, status);

    //  Check for a failure on the I/O.  If one occurred, trace and finish the loop,
    //    journal rebuild is not a critical function.
    if ((status != FBE_STATUS_OK) || (control_status != FBE_PAYLOAD_CONTROL_STATUS_OK))
    {
        fbe_base_object_trace((fbe_base_object_t*) raid_group_p, FBE_TRACE_LEVEL_WARNING, 
                              FBE_TRACE_MESSAGE_ID_INFO, "%s rebuild failed, status: 0x%x\n", __FUNCTION__, status);

        //  Release the context and control operation
        fbe_raid_group_rebuild_release_rebuild_context(in_packet_p);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    // Get current rebuild lba and block count
    start_lba = rebuild_context_p->start_lba;
    block_count = rebuild_context_p->block_count;

    if ((start_lba + block_count) > journal_log_end_lba) 
    {
        //  We're done, finish up this loop and pop to the next level
        //  Release the context and control operation
        fbe_raid_group_rebuild_release_rebuild_context(in_packet_p);

        return FBE_STATUS_OK;
    }
    else
    {
        // Do the next pass
        start_lba += block_count;

        // If we are going past the end, truncate (shouldn't happen)
        if ((start_lba + block_count) > (journal_log_end_lba + 1))
        {
            block_count = (journal_log_end_lba + 1) - start_lba;
        }

        fbe_raid_group_rebuild_context_set_rebuild_start_lba(rebuild_context_p, start_lba);
        fbe_raid_group_rebuild_context_set_rebuild_block_count(rebuild_context_p, block_count);

        // now run the function again (this is completing to here until done or error)
        fbe_transport_set_completion_function(in_packet_p,
                                              fbe_raid_group_journal_chunk_rebuild_completion,
                                              raid_group_p);

        /* Callback is always called, no need to check status.
         */
        fbe_raid_group_journal_rebuild_allocate(raid_group_p, in_packet_p);

        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }

} // End fbe_raid_group_journal_chunk_rebuild_completion()

/*!****************************************************************************
 * fbe_raid_group_journal_rebuild()
 ******************************************************************************
 * @brief
 *  Perform the journal rebuild (which is just zeroing the slots).
 *  This function rebuilds the journal in a loop, without touching the
 *   rebuild checkpoints, since they are controlled by logic in the background condition.
 *  We will use the rebuild context to track progress through journal chunks to rebuild.
 * 
 * @param raid_group_p                  - pointer to a base object
 * @param packet_p                      - pointer to a control packet from the scheduler
 * @param fbe_raid_position_bitmask_t   - bitmask of positions that are ready to rebuild journal
 *
 * @return fbe_status_t                 - FBE_STATUS_MORE_PROCESSING_REQUIRED -- if successfully launched rebuild
                                        - error status if not, though this is ignored
 * @author
 *  4/24/2013 - Created. Dave Agans
 *
 ******************************************************************************/
fbe_status_t fbe_raid_group_journal_rebuild(fbe_raid_group_t *raid_group_p,
                                            fbe_packet_t *packet_p,
                                            fbe_raid_position_bitmask_t positions_to_rebuild)
{
    //fbe_raid_position_bitmask_t         all_degraded_positions = 0;
    fbe_raid_group_rebuild_context_t*   rebuild_context_p = NULL;
    fbe_lba_t                           journal_log_start_lba;
    fbe_lba_t                           journal_log_end_lba;
    fbe_raid_geometry_t                *raid_geometry_p = fbe_raid_group_get_raid_geometry(raid_group_p);

    fbe_raid_geometry_get_journal_log_start_lba(raid_geometry_p, &journal_log_start_lba);
    fbe_raid_geometry_get_journal_log_end_lba(raid_geometry_p, &journal_log_end_lba);

    fbe_raid_group_rebuild_allocate_rebuild_context(raid_group_p, packet_p, &rebuild_context_p);
    if (rebuild_context_p == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "raid_group: run rebuild: failed to allocate rebuild context \n");
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);        
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_raid_group_rebuild_initialize_rebuild_context(raid_group_p, 
                                                      rebuild_context_p,
                                                      positions_to_rebuild,
                                                      journal_log_start_lba);
    fbe_raid_group_rebuild_context_set_rebuild_block_count(rebuild_context_p, FBE_RAID_DEFAULT_CHUNK_SIZE);


    /* Set a special journal rebuild completion which will loop through journal chunks until done.
     */
    fbe_transport_set_completion_function(packet_p, fbe_raid_group_journal_chunk_rebuild_completion,
                                          raid_group_p);

    /* Callback is always called, no need to check status.
     */
    fbe_raid_group_journal_rebuild_allocate(raid_group_p, packet_p);

    return FBE_STATUS_MORE_PROCESSING_REQUIRED;

}
/**************************************
 * end fbe_raid_group_journal_rebuild()
 **************************************/

/*!**************************************************************
 * fbe_raid_group_journal_rebuild_allocate()
 ****************************************************************
 * @brief
 *  This function allocates memory to do the journal rebuild.
 *
 * @param in_object_p - The pointer to the raid group.
 * @param in_packet_p - The control packet sent to us from the scheduler.
 *
 * @return fbe_status_t - FBE_STATUS_PENDING or error 
 *
 * @author
 *  5/30/2012 - Created. Rob Foley
 *  5/1/2013  - Modified for new location in clear RL condition completion path. Dave Agans
 *
 ****************************************************************/
fbe_status_t fbe_raid_group_journal_rebuild_allocate(fbe_raid_group_t *raid_group_p,
                                                               fbe_packet_t *packet_p)
{
    fbe_memory_request_t*                memory_request_p = NULL;
    fbe_packet_resource_priority_t       memory_request_priority;
    fbe_u32_t                            num_rb_positions;
    fbe_status_t                         status;
    fbe_raid_group_rebuild_context_t*    rebuild_context_p = NULL;

    fbe_raid_group_rebuild_get_rebuild_context(raid_group_p, packet_p, &rebuild_context_p);

    memory_request_p = fbe_transport_get_memory_request(packet_p);
    memory_request_priority = fbe_transport_get_resource_priority(packet_p);

    num_rb_positions = fbe_raid_get_bitcount(rebuild_context_p->rebuild_bitmask);

    /* We need one packet per position to write.
     * Plus we allocate one more packet for our SG that will point to the zero bitbucket. 
     */  
    status = fbe_memory_build_chunk_request(memory_request_p,
                                            FBE_MEMORY_CHUNK_SIZE_FOR_PACKET,
                                            num_rb_positions + 1,/* the +1 is for the sg elements*/
                                            memory_request_priority,
                                            fbe_transport_get_io_stamp(packet_p),
                                            (fbe_memory_completion_function_t)fbe_raid_group_journal_rebuild_allocation_completion, /* SAFEBUG - cast to supress warning but must fix because memory completion function shouldn't return status */
                                            raid_group_p);
    if(status == FBE_STATUS_GENERIC_FAILURE)
    {
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,                                        
                              "Setup for memory allocation failed, status:0x%x, line:%d\n", status, __LINE__);        
        
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);        
        fbe_transport_complete_packet(packet_p);
        return status;
    }

    fbe_transport_set_completion_function(packet_p, fbe_raid_group_journal_rebuild_op_completion, raid_group_p);

    /* We set master packet status OK, assuming success. 
     * If a subpacket gets a failure it will set status to bad. 
     */
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);

	fbe_transport_memory_request_set_io_master(memory_request_p, packet_p);

    status = fbe_memory_request_entry(memory_request_p);
    if((status != FBE_STATUS_OK) && (status != FBE_STATUS_PENDING))
    {
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,                                        
                              "UZ:memory allocation failed, status:0x%x, line:%d\n", status, __LINE__);        
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);        
        fbe_transport_complete_packet(packet_p);
        return status;
    }
    
    return FBE_STATUS_PENDING;
}
/**************************************
 * end fbe_raid_group_journal_rebuild_allocate()
 **************************************/
/*!**************************************************************
 * fbe_raid_group_journal_rebuild_allocation_completion()
 ****************************************************************
 * @brief
 *  Fill out the packets we just allocated to be a write operation.
 *  Fill out the sg list we allocated to point to the zero bitbucket.
 *  Then kick off the packets to do the journal rebuild.
 *
 * @param memory_request_p  
 * @param in_context   
 *
 * @return status - The status of the operation.
 *
 * @author
 *  5/30/2012 - Created from journal verify. Rob Foley
 *
 ****************************************************************/
fbe_status_t fbe_raid_group_journal_rebuild_allocation_completion(fbe_memory_request_t * memory_request_p, 
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
    fbe_u32_t                               sub_packet_index = 0;
    fbe_block_size_t                        optimal_block_size;
    fbe_u32_t                               sg_element_count=0;
    fbe_block_count_t                       block_count;
    fbe_raid_geometry_t*                    raid_geometry_p = NULL;
    fbe_u32_t                               width;
    fbe_u32_t                               edge_index;
    fbe_u32_t                               zero_bit_bucket_size_in_blocks;
    fbe_u32_t                               used_sg_element = 0;
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_u32_t                               num_rb_positions;
    fbe_raid_group_rebuild_context_t*       rebuild_context_p = NULL;
    
    raid_group_p = (fbe_raid_group_t*)in_context;

    packet_p = fbe_transport_memory_request_to_packet(memory_request_p);

    fbe_raid_group_rebuild_get_rebuild_context(raid_group_p, packet_p, &rebuild_context_p);
    num_rb_positions = fbe_raid_get_bitcount(rebuild_context_p->rebuild_bitmask);

    raid_geometry_p = fbe_raid_group_get_raid_geometry(raid_group_p);
    fbe_raid_geometry_get_optimal_size(raid_geometry_p, &optimal_block_size);
    
    /* Get the first packet chunk that was allocated. It will be used
       for the sg elements
    */
    memory_header_p = fbe_memory_get_first_memory_header(memory_request_p);
    sg_list_p = (fbe_sg_element_t*)fbe_memory_header_to_data(memory_header_p);
    /* Calculate the number of sg elements to write out 1 chunk = 0x800 blocks(1MB) */
    fbe_memory_get_zero_bit_bucket_size_in_blocks(&zero_bit_bucket_size_in_blocks);
    block_count = fbe_raid_group_get_chunk_size(raid_group_p);
    sg_element_count = (fbe_u32_t)(block_count/zero_bit_bucket_size_in_blocks);

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
        sub_packet_p[sub_packet_index] = (fbe_packet_t*)fbe_memory_header_to_data(next_memory_header_p);
        fbe_transport_initialize_sep_packet(sub_packet_p[sub_packet_index]);
        subpacket_payload_p = fbe_transport_get_payload_ex(sub_packet_p[sub_packet_index]);

        block_operation_p = fbe_payload_ex_allocate_block_operation(subpacket_payload_p);
        fbe_payload_block_build_operation(block_operation_p,
                                          FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_VERIFY,
                                          rebuild_context_p->start_lba,
                                          rebuild_context_p->block_count,
                                          FBE_BE_BYTES_PER_BLOCK,
                                          optimal_block_size,
                                          NULL);
        /* Use forced write since this area is being rebuilt by us.
         */
        fbe_payload_block_set_flag(block_operation_p, FBE_PAYLOAD_BLOCK_OPERATION_FLAGS_FORCED_WRITE);

        /* During encryption we will set the new key for initializing the journal.
         */
        if (fbe_raid_group_is_encryption_state_set(raid_group_p, FBE_RAID_GROUP_ENCRYPTION_FLAG_PAGED_NEEDS_RECONSTRUCT) ||
            fbe_raid_group_is_encryption_state_set(raid_group_p, FBE_RAID_GROUP_ENCRYPTION_FLAG_PAGED_ENCRYPTED)) {
            fbe_payload_block_set_flag(block_operation_p, FBE_PAYLOAD_BLOCK_OPERATION_FLAGS_USE_NEW_KEY);
        }
        fbe_payload_ex_set_sg_list( subpacket_payload_p, sg_list_p, sg_element_count );

        fbe_transport_add_subpacket(packet_p, sub_packet_p[sub_packet_index]);
        memory_header_p = next_memory_header_p;
        fbe_memory_get_next_memory_header(memory_header_p, &next_memory_header_p);
        sub_packet_index++;
    }

    fbe_raid_group_add_to_terminator_queue(raid_group_p, packet_p);

    fbe_raid_geometry_get_width(raid_geometry_p, &width);

    /* Find packets to send by a scan across the width until we find an edge that matches our rebuild bitmask.
     */
    sub_packet_index = 0;
    for(edge_index = 0; edge_index < width; edge_index++)
    {
        if ((1 << edge_index) & rebuild_context_p->rebuild_bitmask)
        {
            fbe_base_object_trace((fbe_base_object_t*)raid_group_p, 
                                  FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                  "raid_group: journal rb lba: 0x%llx bl: 0x%llx pos: %d\n",
                                  (unsigned long long)rebuild_context_p->start_lba, (unsigned long long)rebuild_context_p->block_count, edge_index);
            fbe_transport_set_completion_function(sub_packet_p[sub_packet_index], fbe_raid_group_journal_rebuild_subpacket_completion,
                                                  raid_group_p);

            fbe_base_config_get_block_edge((fbe_base_config_t*)raid_group_p, &edge_p, edge_index);
            fbe_block_transport_send_functional_packet(edge_p, sub_packet_p[sub_packet_index]);
            sub_packet_index++;
        }
    }
    
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/**************************************
 * end fbe_raid_group_journal_rebuild_allocation_completion
 **************************************/

/*!****************************************************************************
 * fbe_raid_group_journal_rebuild_subpacket_completion()
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
 *  5/30/2012 - Created from Journal Verify. Rob Foley
 *  
 ******************************************************************************/
fbe_status_t fbe_raid_group_journal_rebuild_subpacket_completion(fbe_packet_t* packet_p,
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

    if ((status != FBE_STATUS_OK) || (block_status != FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS) )
    {
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p,
                               FBE_TRACE_LEVEL_INFO,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "journal_rebuild failed, block_status:0x%x, status: 0x%x\n", 
                                block_status, status);
        fbe_transport_set_status(master_packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
    }
   
    fbe_payload_ex_release_block_operation(payload_p, block_operation_p);  
    fbe_transport_remove_subpacket_is_queue_empty(packet_p, &is_empty);
               
    if (is_empty)
    {
		fbe_transport_destroy_subpackets(master_packet_p);
        fbe_raid_group_remove_from_terminator_queue(raid_group_p, master_packet_p);
        /* Note that when we started the journal rebuild, we set the packet status to FBE_STATUS_OK. 
         * If any subpacket received an error it would set the status to FBE_STATUS_GENERIC_FAILURE. 
         */
        fbe_transport_complete_packet(master_packet_p); 
    }
    
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/*************************************************************************** 
 * end of fbe_raid_group_journal_rebuild_subpacket_completion()
 ***************************************************************************/

/*!****************************************************************************
 * fbe_raid_group_journal_rebuild_op_completion()
 ******************************************************************************
 * @brief
 *  The journal rebuild operation is done.  Release the operation memory.
 *
 * @param packet_p  
 * @param context   
 *
 * @return status 
 *
 * @author
 *  05/30/2012 - Created. Rob Foley
 *  05/01/2013 - Modified for new journal rebuild location in clear RL condition
 *
 ******************************************************************************/
fbe_status_t fbe_raid_group_journal_rebuild_op_completion(fbe_packet_t *packet_p,
                                                          fbe_packet_completion_context_t context)
{  
    fbe_memory_request_t               *memory_request_p = NULL;
                 
    /* Free up the memory that was allocated        
     */
    memory_request_p = fbe_transport_get_memory_request(packet_p);

	fbe_memory_free_request_entry(memory_request_p);

    return FBE_STATUS_OK;
}
/*****************************************************************
 * end of fbe_raid_group_journal_rebuild_op_completion
 *****************************************************************/
/*********************************************
 * end file fbe_raid_group_journal_rebuild.c
 ********************************************/


