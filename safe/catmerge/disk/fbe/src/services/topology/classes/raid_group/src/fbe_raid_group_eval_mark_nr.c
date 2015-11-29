/***************************************************************************
 * Copyright (C) EMC Corporation 2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_raid_group_eval_mark_nr.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the mark NR evaluation code for the raid group.
 *  This code coordinates the evaluation of marking NR
 *  between the two SPs and decides what other processing to kick in
 *  depending on the evaluation status.
 * 
 * @version
 *   7/12/2011:  Created. Rob Foley
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe_raid_group_needs_rebuild.h"
#include "fbe_raid_group_bitmap.h"
#include "fbe_raid_group_object.h"
#include "fbe_raid_group_rebuild.h"
#include "fbe_transport_memory.h"
#include "fbe_service_manager.h"
#include "fbe_raid_geometry.h"
#include "fbe_cmi.h"
#include "fbe_raid_group_logging.h"

/*****************************************
 * LOCAL FUNCTION PROTOTYPES
 *****************************************/

static fbe_status_t fbe_raid_group_eval_mark_nr_get_checkpoint(fbe_raid_group_t*   raid_group_p,
                                                     fbe_edge_index_t    edge_index, 
                                                     fbe_packet_t *      packet_p);
static fbe_status_t fbe_raid_group_eval_mark_nr_get_checkpoint_completion(fbe_packet_t *      packet_p,
                                                                fbe_packet_completion_context_t  context);
static fbe_status_t fbe_raid_group_eval_mark_nr_get_checkpoint_memory_allocation_completion(fbe_memory_request_t * memory_request_p, 
                                                          fbe_memory_completion_context_t in_context);
static fbe_status_t fbe_raid_group_eval_mark_nr_get_drive_tier_completion(fbe_packet_t * packet_p, fbe_packet_completion_context_t context);
static fbe_status_t fbe_raid_group_eval_mark_nr_complete(fbe_packet_t * packet_p, fbe_packet_completion_context_t context);

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

/*!****************************************************************************
 * fbe_raid_group_eval_mark_nr_get_checkpoint()
 ******************************************************************************
 * @brief
 *   This function sends a control packet to VD to get the checkpoint  
 *
 * 
 * @param raid_group_p       - pointer to a raid group object
 * @param edge_index         - disk position that is rebuild logging
 * @param packet_p           - pointer to a control packet from the scheduler
 * 
 * @return  fbe_status_t  
 *
 * @author
 *   02/14/2011 - Created. Ashwin Tamilarasan
 *
 ******************************************************************************/
fbe_status_t fbe_raid_group_eval_mark_nr_get_checkpoint(
                                        fbe_raid_group_t*   raid_group_p,
                                        fbe_edge_index_t    edge_index, 
                                        fbe_packet_t *      packet_p)
{
    fbe_status_t                       status; 
    fbe_memory_request_t*              memory_request_p = NULL;         
    fbe_memory_request_priority_t      memory_request_priority = 0;


    memory_request_p = fbe_transport_get_memory_request(packet_p);
    memory_request_priority = fbe_transport_get_resource_priority(packet_p);
    fbe_transport_set_resource_priority(packet_p, memory_request_priority + 1);
    
    /* build the memory request for allocation. */
    status = fbe_memory_build_chunk_request(memory_request_p,
                                   FBE_MEMORY_CHUNK_SIZE_FOR_PACKET,
                                   1,
                                   memory_request_priority,
                                   fbe_transport_get_io_stamp(packet_p),
                                   (fbe_memory_completion_function_t)fbe_raid_group_eval_mark_nr_get_checkpoint_memory_allocation_completion, /* SAFEBUG - cast to supress warning but must fix because memory completion function shouldn't return status */
                                   raid_group_p);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *) raid_group_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,                                        
                              "Setup for memory allocation failed, status:0x%x, line:%d\n", status, __LINE__);        
        fbe_transport_set_status(packet_p, status, 0);        
        fbe_transport_complete_packet(packet_p);
        return status;
    }

	fbe_transport_memory_request_set_io_master(memory_request_p, packet_p);

    /* Issue the memory request to memory service. */
    status = fbe_memory_request_entry(memory_request_p);
    if((status != FBE_STATUS_OK) && (status != FBE_STATUS_PENDING))
    {
        fbe_base_object_trace((fbe_base_object_t *) raid_group_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,                                        
                              "USER_VERIFY:memory allocation failed, status:0x%x, line:%d\n", status, __LINE__);        
        fbe_transport_set_status(packet_p, status, 0);        
        fbe_transport_complete_packet(packet_p);
        return status;
    }

    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
   
} // End fbe_raid_group_eval_mark_nr_get_checkpoint()


/*!**************************************************************
 * fbe_raid_group_eval_mark_nr_get_checkpoint_memory_allocation_completion()
 ****************************************************************
 * @brief
 *  This function will send a packet to down strem vd object
 *  to get the vd check points value.
 *
 * @param memory_request_p  
 * @param in_context   
 *
 * @return status - The status of the operation.
 *
 * @author
 *  091/26/2011 - Created. Vishnu Sharma
 *
 ****************************************************************/
fbe_status_t fbe_raid_group_eval_mark_nr_get_checkpoint_memory_allocation_completion(fbe_memory_request_t * memory_request_p, 
                                                          fbe_memory_completion_context_t in_context)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_packet_t                           *packet_p = NULL;
    fbe_raid_group_get_checkpoint_info_t   *rg_server_obj_info_p = NULL; 
    fbe_payload_control_operation_t        *control_operation_p = NULL;
    fbe_payload_ex_t                       *payload_p = NULL;    
    fbe_raid_group_t                       *raid_group_p = NULL;
    fbe_object_id_t                         raid_group_object_id;
    fbe_memory_header_t                    *memory_header_p = NULL;
    fbe_raid_geometry_t                    *raid_geometry_p = NULL;
    fbe_class_id_t                          class_id; 
    fbe_block_edge_t                       *edge_p = NULL;
    fbe_edge_index_t                        edge_index = FBE_EDGE_INDEX_INVALID;
    fbe_u32_t                               width;
    fbe_raid_position_bitmask_t             rl_bitmask;
    fbe_bool_t                              is_sys_rg = FBE_FALSE;
    fbe_payload_control_operation_opcode_t  opcode = FBE_VIRTUAL_DRIVE_CONTROL_CODE_GET_CHECKPOINT;
    
    raid_group_p = (fbe_raid_group_t*)in_context;
    
    
    // get the pointer to original packet.
    packet_p = fbe_transport_memory_request_to_packet(memory_request_p);    
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);

    /* Check allocation status */
    if (fbe_memory_request_is_allocation_complete(memory_request_p) == FBE_FALSE)
    {
        /* Currently this callback doesn't expect any aborted requests.
         */
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_INFO,                                        
                              "raid_group: eval mark nr memory request: 0x%p state: %d allocation failed \n",
                              memory_request_p, memory_request_p->request_state);        
        fbe_transport_set_status(packet_p, FBE_STATUS_INSUFFICIENT_RESOURCES, 0);        
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    // Get the memory that was allocated
    memory_header_p = fbe_memory_get_first_memory_header(memory_request_p);
    rg_server_obj_info_p = (fbe_raid_group_get_checkpoint_info_t*)fbe_memory_header_to_data(memory_header_p);
    
    /* Get the raid group information.
     */
    raid_geometry_p = fbe_raid_group_get_raid_geometry(raid_group_p);
    class_id = fbe_raid_geometry_get_class_id(raid_geometry_p);  
    fbe_raid_geometry_get_width(raid_geometry_p, &width);

    /* Get the rl bitmask and find the first position that needs to
     * be marked nr.
     */ 
    fbe_raid_group_get_rb_logging_bitmask(raid_group_p, &rl_bitmask);
    fbe_raid_group_find_rl_disk(raid_group_p, packet_p, &edge_index);

    /*! @note It is possible that the position that we were marking rebuild
     *        logging for is now disabled etc.  Generate a warning, free the
     *        any allocated memory and leave this condition set.
     */
    if (edge_index >= width)
    {
        /* Generate a warning, free the allocated resources and return success.
         */
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INFO,                                        
                              "raid_group: eval mark nr currently no positions to mark. rl_bitmask: 0x%x\n",
                              rl_bitmask);
        //fbe_memory_request_get_entry_mark_free(&packet_p->memory_request, &memory_ptr);
        //fbe_memory_free_entry(memory_ptr);
		fbe_memory_free_request_entry(&packet_p->memory_request);

        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);        
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_OK;
    }
    rg_server_obj_info_p->edge_index = edge_index;
    rg_server_obj_info_p->requestor_class_id = class_id;
    rg_server_obj_info_p->request_type = FBE_RAID_GROUP_GET_CHECKPOINT_REQUEST_TYPE_EVAL_MARK_NR;
    
    fbe_raid_group_utils_is_sys_rg(raid_group_p, &is_sys_rg);
    if(is_sys_rg)
    {
        opcode = FBE_PROVISION_DRIVE_CONTROL_CODE_GET_CHECKPOINT;
    }
    else
    {
        opcode = FBE_VIRTUAL_DRIVE_CONTROL_CODE_GET_CHECKPOINT;
    }

    fbe_raid_group_allocate_and_build_control_operation(raid_group_p,packet_p,
                                                        opcode,
                                                        rg_server_obj_info_p,
                                                        sizeof(fbe_raid_group_get_checkpoint_info_t));

    fbe_transport_set_completion_function(packet_p, fbe_raid_group_eval_mark_nr_get_checkpoint_completion,
                                          raid_group_p);
    
    if(class_id == FBE_CLASS_ID_VIRTUAL_DRIVE)
    {        
        //  Get the RAID group object-id.
        fbe_base_object_get_object_id((fbe_base_object_t *) raid_group_p, &raid_group_object_id);
    
        //  We are sending control packet to itself. So form the address manually
        fbe_transport_set_address(packet_p,
                                  FBE_PACKAGE_ID_SEP_0,
                                  FBE_SERVICE_ID_TOPOLOGY,
                                  FBE_CLASS_ID_INVALID,
                                  raid_group_object_id);
    
         //  Control packets should be sent via service manager.
         status = fbe_service_manager_send_control_packet(packet_p);  
         return status;      
    }

    else
    {
        // Send the packet to the VD object on this edge.
        fbe_base_config_get_block_edge((fbe_base_config_t*)raid_group_p, &edge_p, edge_index);
        fbe_block_transport_send_control_packet(edge_p, packet_p);
    }    
   
    return FBE_STATUS_MORE_PROCESSING_REQUIRED; 
   

}   // End fbe_raid_group_eval_mark_nr_get_checkpoint_memory_allocation_completion()

/*!****************************************************************************
 * fbe_raid_group_eval_mark_nr_get_checkpoint_completion()
 ******************************************************************************
 * @brief
 *   This is the completion function for getting the VD checkpoint. It uses
 *   the VD checkpoint to determine if rebuild logging should be marked or cleared.
 * 
 * @param packet_p  
 * @param context           
 * 
 * @return  fbe_status_t  
 *
 * @author
 *   02/14/2011 - Created. Ashwin Tamilarasan
 *
 ******************************************************************************/
fbe_status_t fbe_raid_group_eval_mark_nr_get_checkpoint_completion(fbe_packet_t *      packet_p,
                                                                   fbe_packet_completion_context_t  context)
{
    fbe_raid_group_t                   *raid_group_p = NULL;  
    fbe_raid_group_get_checkpoint_info_t *rg_server_obj_info_p = NULL; 
    fbe_lba_t                           vd_checkpoint;
    fbe_u32_t                           disk_position;
    fbe_payload_ex_t                   *sep_payload_p = NULL;
    fbe_payload_control_operation_t    *control_operation_p = NULL;    
    fbe_lba_t                           configured_disk_capacity;
    fbe_block_count_t                   block_count;    
    fbe_lba_t                           offset;
    fbe_status_t                        status;
    fbe_payload_control_status_t        control_status;
    fbe_raid_geometry_t                *raid_geometry_p = NULL;
    fbe_class_id_t                      class_id;

    /* Get the raid group information
     */
    raid_group_p = (fbe_raid_group_t*)context; 
    raid_geometry_p = fbe_raid_group_get_raid_geometry(raid_group_p);
    class_id = fbe_raid_geometry_get_class_id(raid_geometry_p); 

    /*The base edge might have been populated when we sent the packet
      from RAID to VD object. Clear out the edge data in packet so
      that it does not get reused in the monitor context    
    */
    packet_p->base_edge = NULL;
    
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(sep_payload_p);  
    fbe_payload_control_get_buffer(control_operation_p, &rg_server_obj_info_p);  

    /* If the status is not ok release the control operation
       and return ok so that packet gets completed
     */
    status = fbe_transport_get_status_code(packet_p);
    fbe_payload_control_get_status(control_operation_p,&control_status);
    
    if(status != FBE_STATUS_OK || control_status != FBE_PAYLOAD_CONTROL_STATUS_OK)
    {
        //fbe_memory_request_get_entry_mark_free(&packet_p->memory_request, &memory_ptr);
        //fbe_memory_free_entry(memory_ptr);
		fbe_memory_free_request_entry(&packet_p->memory_request);

        fbe_payload_ex_release_control_operation(sep_payload_p, control_operation_p);
        return FBE_STATUS_OK;
    }

    /* Copy the checkpoint and free the buffer 
     */
    vd_checkpoint = rg_server_obj_info_p->downstream_checkpoint;
    disk_position = rg_server_obj_info_p->edge_index;    
    
    //fbe_memory_request_get_entry_mark_free(&packet_p->memory_request, &memory_ptr);
    //fbe_memory_free_entry(memory_ptr);
	fbe_memory_free_request_entry(&packet_p->memory_request);

    fbe_payload_ex_release_control_operation(sep_payload_p, control_operation_p);
     
    /* If this is not the virtual drive and the checkpoint is not 0 or the
     * end marker, subtract the offset.  The raid group treats the checkpoint
     * as a physical (since the rebuild is physical) and therefore we don't
     * convert it to logical.
     */
    if ((class_id != FBE_CLASS_ID_VIRTUAL_DRIVE) &&
        (vd_checkpoint != 0)                     &&
        (vd_checkpoint != FBE_LBA_INVALID)          )
    {
        /* First adjust the checkpoint by this raid groups offset.
         */
        fbe_raid_group_get_offset(raid_group_p, &offset, disk_position);
        if (vd_checkpoint > offset)
        {
            /* Subtract this raid groups's offset.
             */
            vd_checkpoint = vd_checkpoint - offset;
        }
        else
        {
            /* Else the checkpoint is below this raid group so set the 
             * checkpoint to 0.
             */
            vd_checkpoint = 0;
        }
    }

    /* we need mark NR here because on passive side we don't have job service to mark NR for the c4mirror raid group
     */
    if (fbe_raid_group_is_c4_mirror_raid_group(raid_group_p)) {
        fbe_u32_t new_pvd_bitmap = 0;
        fbe_raid_group_c4_mirror_get_new_pvd(raid_group_p, &new_pvd_bitmap);
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p, 
                              FBE_TRACE_LEVEL_INFO, 
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "c4mirror: mark NR for pos: %u, new_pvd: 0x%x\n",
                              disk_position, new_pvd_bitmap);
        if (new_pvd_bitmap & (1 << disk_position)) {
            vd_checkpoint = 0;
        }
    }

    /* Get the disk capacity consumed for this position.
     */
    configured_disk_capacity = fbe_raid_group_get_disk_capacity(raid_group_p);

    /* Trace some information.
     */
    fbe_base_object_trace((fbe_base_object_t *)raid_group_p, 
                          FBE_TRACE_LEVEL_INFO, 
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "raid_group: eval mark nr class_id: %d edge: %d checkpoint: 0x%llx cap: 0x%llx\n",
                          class_id, disk_position, (unsigned long long)vd_checkpoint, (unsigned long long)configured_disk_capacity);

    /* evaluate the lowest drive tier in the raid group */
    if (fbe_raid_group_is_drive_tier_query_needed(raid_group_p)) 
    {
        /* set completion function*/
        fbe_transport_set_completion_function(packet_p,
                                              fbe_raid_group_eval_mark_nr_get_drive_tier_completion,
                                              raid_group_p);
        fbe_raid_group_monitor_update_nonpaged_drive_tier(((fbe_base_object_t*) raid_group_p), packet_p);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }
    
    /* We set request so that we will go through and see if there are any other drives 
     * that need evaluate when we finish evaluating this drive.  For a virtual drive
     * there are no other edges to mark.
     */
    if (class_id != FBE_CLASS_ID_VIRTUAL_DRIVE)
    {
        fbe_raid_group_set_local_state(raid_group_p, FBE_RAID_GROUP_LOCAL_STATE_EVAL_MARK_NR_REQUEST);
    }

    /* If the VD checkpoint is invalid that means it is the same drive that is 
     * coming back set the clear RL condition.
     */
     if (vd_checkpoint == FBE_LBA_INVALID)
     {
         fbe_raid_group_lock(raid_group_p);
         fbe_raid_group_set_local_state(raid_group_p, FBE_RAID_GROUP_LOCAL_STATE_EVAL_MARK_NR_DONE);
         fbe_raid_group_clear_local_state(raid_group_p, FBE_RAID_GROUP_LOCAL_STATE_EVAL_MARK_NR_STARTED);
         fbe_raid_group_set_local_state(raid_group_p, FBE_RAID_GROUP_LOCAL_STATE_CLEAR_RL_REQUEST);
         fbe_raid_group_unlock(raid_group_p);
         fbe_lifecycle_set_cond(&fbe_raid_group_lifecycle_const, (fbe_base_object_t*) raid_group_p, 
                                FBE_RAID_GROUP_LIFECYCLE_COND_CLEAR_RB_LOGGING);            
         return FBE_STATUS_OK;
     }

     /* It could be a permanent sparing case where vd checkpoint will be zero
        or it could a PC died use case in the middle of proactive copy.
        Mark NR according to the vd checkpoint
      */
     if (vd_checkpoint != FBE_LBA_INVALID)
     {
         fbe_base_object_trace((fbe_base_object_t*) raid_group_p,
                               FBE_TRACE_LEVEL_DEBUG_LOW,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                               "%s mark NR above the checkpoint: 0x%llx\n",
			       __FUNCTION__, (unsigned long long)vd_checkpoint);

        block_count = configured_disk_capacity - vd_checkpoint;

        fbe_raid_group_trace(raid_group_p,
                             FBE_TRACE_LEVEL_INFO,
                             FBE_RAID_GROUP_DEBUG_FLAGS_TRACE_PMD_NR,
                             "raid_group: eval mark nr for edge: %d blocks: 0x%llx checkpoint: 0x%llx \n",
                             disk_position, (unsigned long long)block_count, (unsigned long long)vd_checkpoint);

        fbe_transport_set_completion_function(packet_p, fbe_raid_group_eval_mark_nr_complete, raid_group_p);

        fbe_raid_group_needs_rebuild_mark_nr(raid_group_p, packet_p, disk_position,
                                             vd_checkpoint, block_count);
        
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
     }
    
    return FBE_STATUS_OK; 

} // End fbe_raid_group_eval_mark_nr_get_checkpoint_completion()

/*!****************************************************************************
 * fbe_raid_group_eval_mark_nr_complete()
 ******************************************************************************
 * @brief
 *  The mark NR is completed.  If it is successful, transition to next state.
 *
 * @param packet_p - packet pointer.
 * @param context - pointer to the raid group
 * 
 * @return fbe_status_t
 *
 * @author
 *  5/16/2014 - Created. Rob Foley
 *
 ******************************************************************************/
static fbe_status_t fbe_raid_group_eval_mark_nr_complete(fbe_packet_t * packet_p, fbe_packet_completion_context_t context) 
{
    fbe_status_t                        status;
    fbe_raid_group_t                   *raid_group_p = (fbe_raid_group_t *)context;

    raid_group_p = (fbe_raid_group_t *)context;

    status = fbe_transport_get_status_code(packet_p);
    
    if (status != FBE_STATUS_OK) {
        return FBE_STATUS_OK;
    }
    fbe_raid_group_lock(raid_group_p);
    fbe_raid_group_set_local_state(raid_group_p, FBE_RAID_GROUP_LOCAL_STATE_EVAL_MARK_NR_DONE);
    fbe_raid_group_clear_local_state(raid_group_p, FBE_RAID_GROUP_LOCAL_STATE_EVAL_MARK_NR_STARTED);
    fbe_raid_group_unlock(raid_group_p);
    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_raid_group_eval_mark_nr_complete()
 **************************************/
/*!****************************************************************************
 * fbe_raid_group_eval_mark_nr_get_drive_tier_completion()
 ******************************************************************************
 * @brief
 *  This function is used to eval mark NR for the RAID group object after
 *  the drive tier has been persisted to the nonpaged
 *
 * @param packet_p - packet pointer.
 * @param context - pointer to the raid group
 * 
 * @return fbe_status_t
 *
 * @author
 *  2/6/2014 - Created. Deanna Heng
 *
 ******************************************************************************/
fbe_status_t fbe_raid_group_eval_mark_nr_get_drive_tier_completion(fbe_packet_t * packet_p, fbe_packet_completion_context_t context) 
{
    fbe_raid_group_t                *raid_group_p = (fbe_raid_group_t *)context;


    raid_group_p = (fbe_raid_group_t *)context;

    /* clear RL condition for the system drive.
     */
    fbe_raid_group_lock(raid_group_p);
    /* We set request so that we will go through and see if there are any other drives 
     * that need evaluate when we finish evaluating this drive.  
     */
    fbe_raid_group_set_local_state(raid_group_p, FBE_RAID_GROUP_LOCAL_STATE_EVAL_MARK_NR_REQUEST);
    fbe_raid_group_set_local_state(raid_group_p, FBE_RAID_GROUP_LOCAL_STATE_EVAL_MARK_NR_DONE);
    fbe_raid_group_clear_local_state(raid_group_p, FBE_RAID_GROUP_LOCAL_STATE_EVAL_MARK_NR_STARTED);
    fbe_raid_group_set_local_state(raid_group_p, FBE_RAID_GROUP_LOCAL_STATE_CLEAR_RL_REQUEST);
    fbe_raid_group_unlock(raid_group_p);
    fbe_lifecycle_set_cond(&fbe_raid_group_lifecycle_const, (fbe_base_object_t*) raid_group_p, 
                           FBE_RAID_GROUP_LIFECYCLE_COND_CLEAR_RB_LOGGING);            
    return FBE_STATUS_OK;
    
}
/**************************************
 * end fbe_raid_group_eval_mark_nr_get_drive_tier_completion()
 **************************************/

/*!****************************************************************************
 * fbe_raid_group_eval_mark_nr_passive()
 ******************************************************************************
 * @brief
 *  This function is used to eval mark NR for the RAID group object on 
 *  passive side.
 *
 * @param raid_group_p - Raid group object
 * @param packet_p - packet pointer.
 * 
 * @return fbe_status_t
 *
 * @author
 *  6/16/2011 - Created. Rob Foley
 *
 ******************************************************************************/
fbe_lifecycle_status_t 
fbe_raid_group_eval_mark_nr_passive(fbe_raid_group_t *  raid_group_p, fbe_packet_t * packet_p)
{
    fbe_bool_t b_peer_flag_set = FBE_FALSE;
    fbe_bool_t b_local_flag_set = FBE_FALSE;
	fbe_bool_t b_is_active;
    fbe_scheduler_hook_status_t        hook_status = FBE_SCHED_STATUS_OK;

    fbe_raid_group_lock(raid_group_p);
    fbe_raid_group_is_peer_flag_set(raid_group_p, &b_peer_flag_set, 
                                    FBE_RAID_GROUP_CLUSTERED_FLAG_EVAL_MARK_NR_STARTED);

    if (fbe_raid_group_is_clustered_flag_set(raid_group_p, FBE_RAID_GROUP_CLUSTERED_FLAG_EVAL_MARK_NR_STARTED))
    {
        b_local_flag_set = FBE_TRUE;
    }

    /* We first wait for the peer to start the mark NR eval. 
     * This is needed since either active or passive can initiate the eval. 
     * Once we see this set, we will set our flag, which the peer waits for before it completes the eval.  
     */
    if (!b_peer_flag_set && !b_local_flag_set)
    {
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
			      FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "Passive waiting peer EVAL_MARK_NR_STARTED local: 0x%llx peer: 0x%llx state: 0x%llx\n",
                              (unsigned long long)raid_group_p->raid_group_metadata_memory.flags,
                              (unsigned long long)raid_group_p->raid_group_metadata_memory_peer.flags,
                              (unsigned long long)raid_group_p->local_state);
        fbe_raid_group_unlock(raid_group_p);
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /* At this point the peer started flag is set.
     * If we have not set this yet locally, set it and wait for the condition to run to send it to the peer.
     */
    if (!b_local_flag_set)
    {
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
			      FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "Passive marking EVAL_MARK_NR_STARTED state: 0x%llx\n",
			      (unsigned long long)raid_group_p->local_state);
        fbe_raid_group_set_clustered_flag(raid_group_p, FBE_RAID_GROUP_CLUSTERED_FLAG_EVAL_MARK_NR_STARTED);
        fbe_raid_group_unlock(raid_group_p);
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    fbe_check_rg_monitor_hooks(raid_group_p,
                               SCHEDULER_MONITOR_STATE_RAID_GROUP_EVAL_MARK_NR,
                               FBE_RAID_GROUP_SUBSTATE_EVAL_MARK_NR_STARTED, 
                               &hook_status);
    if (hook_status == FBE_SCHED_STATUS_DONE) {
        fbe_raid_group_unlock(raid_group_p);
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /* If we become active, just return out.
     * We need to handle the active's work ourself. 
     * When we come back into the condition we'll automatically execute the active code.
     */
	fbe_raid_group_monitor_check_active_state(raid_group_p, &b_is_active);
    if (b_is_active)
    {
        fbe_raid_group_unlock(raid_group_p);
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p, FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                              "Passive join became active local: 0x%llx peer: 0x%llx lstate: 0x%llx\n",
                              (unsigned long long)raid_group_p->raid_group_metadata_memory.base_config_metadata_memory.flags,
                              (unsigned long long)raid_group_p->raid_group_metadata_memory_peer.base_config_metadata_memory.flags,
                              (unsigned long long)raid_group_p->local_state);
        return FBE_LIFECYCLE_STATUS_RESCHEDULE;
    }

    /* Once we have set the started flag, we simply wait for the active side to complete. 
     * We know Active is done when it clears the flags. 
     */
    if (!b_peer_flag_set)
    {
        fbe_raid_group_set_local_state(raid_group_p, FBE_RAID_GROUP_LOCAL_STATE_EVAL_MARK_NR_DONE);
        fbe_raid_group_clear_local_state(raid_group_p, FBE_RAID_GROUP_LOCAL_STATE_EVAL_MARK_NR_STARTED);
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "eval mark nr Passive start cleanup local: 0x%llx peer: 0x%llx\n",
                              (unsigned long long)raid_group_p->raid_group_metadata_memory.flags,
                              (unsigned long long)raid_group_p->raid_group_metadata_memory_peer.flags);
        fbe_raid_group_unlock(raid_group_p);
        return FBE_LIFECYCLE_STATUS_RESCHEDULE;
    }
    else
    {
        fbe_raid_group_unlock(raid_group_p);
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "eval mark nr Passive wait for peer to complete\n");
    }
    return FBE_LIFECYCLE_STATUS_DONE;
}
/**************************************
 * end fbe_raid_group_eval_mark_nr_passive()
 **************************************/

/*!****************************************************************************
 * fbe_raid_group_eval_mark_nr_active()
 ******************************************************************************
 * @brief
 *  This function is used to eval mark NR on active side for the RAID group object.
 *
 * @param raid_group_p - Raid group object
 * @param packet_p - packet pointer.
 * 
 * @return fbe_lifecycle_status_t
 *
 * @author
 *  6/16/2011 - Created. Rob Foley
 *
 ******************************************************************************/
fbe_lifecycle_status_t 
fbe_raid_group_eval_mark_nr_active(fbe_raid_group_t *  raid_group_p, fbe_packet_t * packet_p)
{
    fbe_u32_t                   disk_position;
    fbe_bool_t                  b_peer_flag_set = FBE_FALSE;
    fbe_scheduler_hook_status_t hook_status = FBE_SCHED_STATUS_OK;

    /* Find the disk that has rl and the disk is up (before taking the lock) 
    */
    fbe_raid_group_find_rl_disk(raid_group_p, packet_p, &disk_position);

    /* Now take the lock.
     */
    fbe_raid_group_lock(raid_group_p);

    /* Now we set the bit that says we are starting. 
     * This bit always gets set first by the Active side. 
     * When passive sees the started peer bit, it will also set the started bit. 
     * The passive side will thereafter know we are done when this bit is cleared. 
     */
    if (!fbe_raid_group_is_clustered_flag_set(raid_group_p, FBE_RAID_GROUP_CLUSTERED_FLAG_EVAL_MARK_NR_STARTED))
    {
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "Active marking EVAL_MARK_NR_STARTED loc: 0x%llx peer: 0x%llx state: 0x%llx\n",
                              (unsigned long long)raid_group_p->raid_group_metadata_memory.flags,
                              (unsigned long long)raid_group_p->raid_group_metadata_memory_peer.flags,
                              (unsigned long long)raid_group_p->local_state);
        fbe_raid_group_set_clustered_flag(raid_group_p, FBE_RAID_GROUP_CLUSTERED_FLAG_EVAL_MARK_NR_STARTED);

        fbe_raid_group_unlock(raid_group_p);
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    fbe_check_rg_monitor_hooks(raid_group_p,
                               SCHEDULER_MONITOR_STATE_RAID_GROUP_EVAL_MARK_NR,
                               FBE_RAID_GROUP_SUBSTATE_EVAL_MARK_NR_STARTED, 
                               &hook_status);
    if (hook_status == FBE_SCHED_STATUS_DONE) {
        fbe_raid_group_unlock(raid_group_p);
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /* Now we need to wait for the peer to acknowledge that it saw us start.
     * By waiting for the passive side to get to this point, it allows the 
     * active side to clear the bits when it is complete. 
     * The passive side will also be able to know we are done when the bits are cleared.
     */
    fbe_raid_group_is_peer_flag_set(raid_group_p, &b_peer_flag_set, 
                                    FBE_RAID_GROUP_CLUSTERED_FLAG_EVAL_MARK_NR_STARTED);
    if (!b_peer_flag_set)
    {
        fbe_raid_group_unlock(raid_group_p);
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "eval mark nr Active wait peer MARK_NR_STARTED local fl: 0x%llx peer fl: 0x%llx state: 0x%llx\n", 
                              (unsigned long long)raid_group_p->raid_group_metadata_memory.flags,
                              (unsigned long long)raid_group_p->raid_group_metadata_memory_peer.flags,
                              (unsigned long long)raid_group_p->local_state);
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /* Check if there is a position to mark NR for
     */
    if (disk_position == FBE_RAID_INVALID_DISK_POSITION) 
    {
        /* We do not see anything that needs the mark nr evaluated. 
         * We might have come through here before the drive came up on the peer side. 
         * In that case clear nr would not have cleared and we need to kick it now
         * See if we have anything we need to clear rebuild logging for.
         */        
        fbe_raid_group_set_local_state(raid_group_p, FBE_RAID_GROUP_LOCAL_STATE_EVAL_MARK_NR_DONE);
        fbe_raid_group_clear_local_state(raid_group_p, FBE_RAID_GROUP_LOCAL_STATE_EVAL_MARK_NR_STARTED);
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "eval mark nr Active no action required. local fl: 0x%llx peer fl: 0x%llx state: 0x%llx\n", 
                              (unsigned long long)raid_group_p->raid_group_metadata_memory.flags,
                              (unsigned long long)raid_group_p->raid_group_metadata_memory_peer.flags,
                              (unsigned long long)raid_group_p->local_state);        
        fbe_raid_group_unlock(raid_group_p);
        return FBE_LIFECYCLE_STATUS_RESCHEDULE;
    }
    fbe_raid_group_unlock(raid_group_p);
    
    // Send an usurper command to VD based on the disk position to get its checkpoint
    // and take actions accordingly
    fbe_raid_group_eval_mark_nr_get_checkpoint(raid_group_p, disk_position, packet_p);  
    return FBE_LIFECYCLE_STATUS_PENDING;
}
/**************************************
 * end fbe_raid_group_eval_mark_nr_active()
 **************************************/

/*!****************************************************************************
 * fbe_raid_group_eval_mark_nr_request()
 ******************************************************************************
 * @brief
 *  This function is used to handle eval mark NR request for the RAID group object.
 *
 * @param raid_group_p - Raid group object
 * @param packet_p - packet pointer.
 * 
 * @return fbe_lifecycle_status_t
 *
 * @author
 *  6/17/2011 - Created. Rob Foley
 *
 ******************************************************************************/
fbe_lifecycle_status_t 
fbe_raid_group_eval_mark_nr_request(fbe_raid_group_t *  raid_group_p, fbe_packet_t * packet_p)
{
	fbe_bool_t b_is_active;
    fbe_bool_t b_peer_flag_set = FBE_FALSE;
    fbe_scheduler_hook_status_t        hook_status = FBE_SCHED_STATUS_OK;

	fbe_raid_group_monitor_check_active_state(raid_group_p, &b_is_active);

    fbe_raid_group_lock(raid_group_p);

    if (!fbe_raid_group_is_local_state_set(raid_group_p, FBE_RAID_GROUP_LOCAL_STATE_EVAL_MARK_NR_REQUEST))
    {
        fbe_raid_group_set_local_state(raid_group_p, FBE_RAID_GROUP_LOCAL_STATE_EVAL_MARK_NR_REQUEST);
    }
    /* Either the active or passive side can initiate this. 
     * We start the process by setting eval mark nr. 
     * This gets the peer running the same condition. 
     */
    if (!fbe_raid_group_is_clustered_flag_set(raid_group_p, 
                                              FBE_RAID_GROUP_CLUSTERED_FLAG_EVAL_MARK_NR_REQ))
    {
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s marking FBE_RAID_GROUP_CLUSTERED_FLAG_EVAL_MARK_NR state: 0x%llx\n", 
                              b_is_active ? "Active" : "Passive",
                              (unsigned long long)raid_group_p->local_state);
        fbe_raid_group_set_clustered_flag(raid_group_p, FBE_RAID_GROUP_CLUSTERED_FLAG_EVAL_MARK_NR_REQ);

        fbe_raid_group_unlock(raid_group_p);
        return FBE_LIFECYCLE_STATUS_RESCHEDULE;
    }

    fbe_check_rg_monitor_hooks(raid_group_p,
                               SCHEDULER_MONITOR_STATE_RAID_GROUP_EVAL_MARK_NR,
                               FBE_RAID_GROUP_SUBSTATE_EVAL_MARK_NR_REQUEST, 
                               &hook_status);
    if (hook_status == FBE_SCHED_STATUS_DONE) {
        fbe_raid_group_unlock(raid_group_p);
        return FBE_LIFECYCLE_STATUS_DONE;
    } 

    fbe_raid_group_is_peer_flag_set(raid_group_p, &b_peer_flag_set, 
                                    FBE_RAID_GROUP_CLUSTERED_FLAG_EVAL_MARK_NR_REQ);

    if (!b_peer_flag_set)
    {
        fbe_raid_group_unlock(raid_group_p);
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "eval mark nr %s, wait peer request local: 0x%llx peer: 0x%llx state: 0x%llx\n", 
                              b_is_active ? "Active" : "Passive",
                              (unsigned long long)raid_group_p->raid_group_metadata_memory.flags,
                              (unsigned long long)raid_group_p->raid_group_metadata_memory_peer.flags,
                              (unsigned long long)raid_group_p->local_state);
        return FBE_LIFECYCLE_STATUS_DONE;
    }
    else
    {
        /* The peer has set the flag, we are ready to do the next set of checks.
         */
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "eval mark nr %s peer req set local: 0x%llx peer: 0x%llx state: 0x%llx 0x%x\n", 
                              b_is_active ? "Active" : "Passive",
                              (unsigned long long)raid_group_p->raid_group_metadata_memory.flags,
                              (unsigned long long)raid_group_p->raid_group_metadata_memory_peer.flags,
                              (unsigned long long)raid_group_p->local_state,
                              raid_group_p->raid_group_metadata_memory_peer.base_config_metadata_memory.lifecycle_state);
        fbe_raid_group_set_local_state(raid_group_p, FBE_RAID_GROUP_LOCAL_STATE_EVAL_MARK_NR_STARTED);
        fbe_raid_group_clear_local_state(raid_group_p, FBE_RAID_GROUP_LOCAL_STATE_EVAL_MARK_NR_REQUEST);
        fbe_raid_group_unlock(raid_group_p);
        return FBE_LIFECYCLE_STATUS_RESCHEDULE;
    }
}
/**************************************
 * end fbe_raid_group_eval_mark_nr_request()
 **************************************/
/*!****************************************************************************
 * fbe_raid_group_eval_mark_nr_done()
 ******************************************************************************
 * @brief
 *  We are waiting for the peer to tell us they are done by clearing
 *  the flags related to evaluating NR marking.
 *
 * @param raid_group_p - Raid group object
 * @param packet_p - packet pointer.
 * 
 * @return fbe_lifecycle_status_t
 *
 * @author
 *  6/17/2011 - Created. Rob Foley
 *
 ******************************************************************************/
fbe_lifecycle_status_t 
fbe_raid_group_eval_mark_nr_done(fbe_raid_group_t *  raid_group_p, fbe_packet_t * packet_p)
{
    fbe_status_t status;
	fbe_bool_t b_is_active;
    fbe_bool_t b_peer_flag_set = FBE_FALSE;
    fbe_scheduler_hook_status_t        hook_status = FBE_SCHED_STATUS_OK;

	fbe_raid_group_monitor_check_active_state(raid_group_p, &b_is_active);

    fbe_raid_group_lock(raid_group_p);

    /* We want to clear our local flags first.
     */
    if (fbe_raid_group_is_clustered_flag_set(raid_group_p, FBE_RAID_GROUP_CLUSTERED_FLAG_EVAL_MARK_NR_REQ) ||
        fbe_raid_group_is_clustered_flag_set(raid_group_p, FBE_RAID_GROUP_CLUSTERED_FLAG_EVAL_MARK_NR_STARTED))
    {
        fbe_raid_group_clear_clustered_flag(raid_group_p, 
                                          (FBE_RAID_GROUP_CLUSTERED_FLAG_EVAL_MARK_NR_REQ | 
                                          FBE_RAID_GROUP_CLUSTERED_FLAG_EVAL_MARK_NR_STARTED));
        fbe_raid_group_unlock(raid_group_p);
        return FBE_LIFECYCLE_STATUS_RESCHEDULE;
    }

    fbe_check_rg_monitor_hooks(raid_group_p,
                               SCHEDULER_MONITOR_STATE_RAID_GROUP_EVAL_MARK_NR,
                               FBE_RAID_GROUP_SUBSTATE_EVAL_MARK_NR_DONE, 
                               &hook_status);
    if (hook_status == FBE_SCHED_STATUS_DONE) {
        fbe_raid_group_unlock(raid_group_p);
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    fbe_raid_group_is_peer_flag_set(raid_group_p, &b_peer_flag_set,
                                    FBE_RAID_GROUP_CLUSTERED_FLAG_EVAL_MARK_NR_STARTED);
    if (fbe_raid_group_is_peer_present(raid_group_p) && 
        b_peer_flag_set)
    {
        fbe_raid_group_unlock(raid_group_p);
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "eval mark nr cleanup %s, wait peer done local: 0x%llx peer: 0x%llx state:0x%llx\n", 
                              b_is_active ? "Active" : "Passive",
                              (unsigned long long)raid_group_p->raid_group_metadata_memory.flags,
                              (unsigned long long)raid_group_p->raid_group_metadata_memory_peer.flags,
                              (unsigned long long)raid_group_p->local_state);
        return FBE_LIFECYCLE_STATUS_DONE;
    }
    else
    {
        /* The local flags tell us that we are waiting for the peer to clear their flags.
         * Since it seems they are complete, it is safe to clear our local flags. 
         */
        fbe_raid_group_clear_local_state(raid_group_p, 
                                         (FBE_RAID_GROUP_LOCAL_STATE_EVAL_MARK_NR_STARTED | 
                                          FBE_RAID_GROUP_LOCAL_STATE_EVAL_MARK_NR_DONE));

        /* It is possible that we received a request to eval mark nr either locally or from the peer.
         * If a request is not set, let's clear the condition.  Otherwise we will run this condition again, 
         * and when we see that the started and done flags are not set, we'll re-evaluate the mark nr. 
         */
        if (!fbe_raid_group_is_local_state_set(raid_group_p, FBE_RAID_GROUP_LOCAL_STATE_EVAL_MARK_NR_REQUEST))
        {
            status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)raid_group_p);
            if (status != FBE_STATUS_OK)
            {
                fbe_base_object_trace((fbe_base_object_t *)raid_group_p, 
                                      FBE_TRACE_LEVEL_ERROR,
                                      FBE_TRACE_MESSAGE_ID_INFO,
                                      "%s can't clear current condition, status: 0x%x\n",
                                      __FUNCTION__, status);
            }
        }
        fbe_raid_group_unlock(raid_group_p);
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "eval mark nr %s cleanup complete local: 0x%llx peer: 0x%llx state: 0x%llx\n",
                              b_is_active ? "Active" : "Passive",
                              (unsigned long long)raid_group_p->raid_group_metadata_memory.flags,
                              (unsigned long long)raid_group_p->raid_group_metadata_memory_peer.flags,
                              (unsigned long long)raid_group_p->local_state);
        return FBE_LIFECYCLE_STATUS_RESCHEDULE;
    }
}
/**************************************
 * end fbe_raid_group_eval_mark_nr_done()
 **************************************/
/*************************
 * end file fbe_raid_group_eval_mark_nr.c
 *************************/


