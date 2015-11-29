/***************************************************************************
 * Copyright (C) EMC Corporation 2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/
 
/*!*************************************************************************
 * @file fbe_raid_group_clear_rl.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the code for clearing rebuild logging
 * 
 *  This code gets called by the raid group clear rebuild logging condition.
 *
 * @version
 *   7/13/2011:  Created. Rob Foley
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
static fbe_status_t fbe_raid_group_write_clear_rl_completion(fbe_packet_t *packet_p,
                                                             fbe_packet_completion_context_t context_p);
static fbe_status_t fbe_raid_group_get_clear_rl_bits(fbe_raid_group_t *raid_group_p,
                                                     fbe_raid_position_bitmask_t *clear_bitmask_p);

static fbe_status_t fbe_raid_group_clear_rl_get_checkpoints(fbe_packet_t *packet_p,
                                                   fbe_raid_group_t *raid_group_p,
                                                   fbe_raid_position_bitmask_t clear_bitmask);

static fbe_status_t fbe_raid_group_clear_rl_get_checkpoints_subpacket_completion(fbe_packet_t *packet_p,
                                                                                     fbe_packet_completion_context_t context);

static fbe_status_t fbe_raid_group_clear_rl_get_checkpoints_completion(fbe_packet_t *packet_p,
                                                              fbe_packet_completion_context_t context);

static fbe_status_t fbe_raid_group_clear_rl_get_checkpoints_memory_allocation_completion(fbe_memory_request_t *memory_request_p, 
                                                                                fbe_memory_completion_context_t in_context);

static fbe_status_t fbe_raid_group_journal_rebuild_completion_clear_rl(fbe_packet_t *packet_p,
                                                                       fbe_packet_completion_context_t context_p);
static fbe_status_t fbe_raid_group_journal_rebuild_completion_break_context(fbe_packet_t *packet_p, 
                                                                            fbe_packet_completion_context_t context);

/*************************
 *   FUNCTION DEFINITIONS
 *************************/


/*!****************************************************************************
 * fbe_raid_group_clear_rl_request()
 ******************************************************************************
 * @brief
 *  This function initiates clear rebuild logging.
 *  If we are here then the FBE_RAID_GROUP_LOCAL_STATE_CLEAR_RL_REQUEST may be
 *  set if we initiated this or the peer initiated this.
 *  It will not be set if we are here as a result of a preset.
 *  We wait for both SPs to acknowledge that they are ready to clear rebuild logging.
 * 
 *  Once both SPs are ready the SPs will move to the started local state.
 *
 * @param raid_group_p - Raid group object
 * @param packet_p - packet pointer.
 * 
 * @return fbe_lifecycle_status_t
 *
 * @author
 *  7/13/2011 - Created. Rob Foley
 *
 ******************************************************************************/
fbe_lifecycle_status_t 
fbe_raid_group_clear_rl_request(fbe_raid_group_t *  raid_group_p, fbe_packet_t * packet_p)
{
    fbe_bool_t                  b_is_active;
    fbe_bool_t                  b_peer_flag_set = FBE_FALSE;
    fbe_scheduler_hook_status_t hook_status = FBE_SCHED_STATUS_OK;
    fbe_bool_t                  b_quiesce_in_progress = FBE_FALSE;
    fbe_u32_t                   count = 0;
    
	fbe_raid_group_monitor_check_active_state(raid_group_p, &b_is_active);

    fbe_raid_group_lock(raid_group_p);

    /* If this condition was a preset, then this local state needs to get set. 
     */
    if (!fbe_raid_group_is_local_state_set(raid_group_p, FBE_RAID_GROUP_LOCAL_STATE_CLEAR_RL_REQUEST))
    {
        fbe_raid_group_set_local_state(raid_group_p, FBE_RAID_GROUP_LOCAL_STATE_CLEAR_RL_REQUEST);
    }

    /* If there are outstanding request and we are not quiesced, return done 
     * and wait for the queue to drain.
     */
    b_quiesce_in_progress = fbe_base_config_is_any_clustered_flag_set((fbe_base_config_t *)raid_group_p, 
                                                                      (FBE_BASE_CONFIG_CLUSTERED_FLAG_QUIESCED|FBE_BASE_CONFIG_CLUSTERED_FLAG_QUIESCING));
    fbe_raid_group_get_quiesced_count(raid_group_p, &count);
    if (!b_quiesce_in_progress &&
        (count != 0)              )
    {
        /* Trace some information.
         */
        fbe_raid_group_unlock(raid_group_p);
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s CLEAR_RL_REQUEST cannot start: %d quiesced requests\n", 
                              b_is_active ? "Active" : "Passive",
                              count);
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /* Either the active or passive side can initiate this. 
     * We start the process by setting the peer REQ bit. 
     * This asks the peer to run same condition
     */
    if (!fbe_raid_group_is_clustered_flag_set(raid_group_p, FBE_RAID_GROUP_CLUSTERED_FLAG_CLEAR_RL_REQ))
    {
        fbe_raid_group_set_clustered_flag(raid_group_p, FBE_RAID_GROUP_CLUSTERED_FLAG_CLEAR_RL_REQ);

        fbe_raid_group_unlock(raid_group_p);
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s marking FBE_RAID_GROUP_CLUSTERED_FLAG_CLEAR_RL state: 0x%llx\n", 
                              b_is_active ? "Active" : "Passive",
                              (unsigned long long)raid_group_p->local_state);
       return FBE_LIFECYCLE_STATUS_RESCHEDULE;
    }

    fbe_check_rg_monitor_hooks(raid_group_p,
                               SCHEDULER_MONITOR_STATE_RAID_GROUP_CLEAR_RL,
                               FBE_RAID_GROUP_SUBSTATE_CLEAR_RL_REQUEST, 
                               &hook_status);
    if (hook_status == FBE_SCHED_STATUS_DONE) {
        fbe_raid_group_unlock(raid_group_p);
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    fbe_raid_group_is_peer_flag_set(raid_group_p, &b_peer_flag_set, 
                                    FBE_RAID_GROUP_CLUSTERED_FLAG_CLEAR_RL_REQ);

    if (!b_peer_flag_set)
    {
        /* Peer is not yet running the condition, wait for them to start running this condition.
         */
        fbe_raid_group_unlock(raid_group_p);
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "clear rl %s, wait peer request local: 0x%llx peer: 0x%llx state: 0x%llx\n", 
                              b_is_active ? "Active" : "Passive",
                              (unsigned long long)raid_group_p->raid_group_metadata_memory.flags,
                              (unsigned long long)raid_group_p->raid_group_metadata_memory_peer.flags,
                              (unsigned long long)raid_group_p->local_state);
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    else
    {
        /* The peer has set the flag, they are running this condition.
         * We can transition our local state.
         */
        fbe_raid_group_set_local_state(raid_group_p, FBE_RAID_GROUP_LOCAL_STATE_CLEAR_RL_STARTED);
        fbe_raid_group_clear_local_state(raid_group_p, FBE_RAID_GROUP_LOCAL_STATE_CLEAR_RL_REQUEST);
        fbe_raid_group_unlock(raid_group_p);
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "clear rl %s peer req set local: 0x%llx peer: 0x%llx state: 0x%llx 0x%x\n", 
                              b_is_active ? "Active" : "Passive",
                              (unsigned long long)raid_group_p->raid_group_metadata_memory.flags,
                              (unsigned long long)raid_group_p->raid_group_metadata_memory_peer.flags,
                              (unsigned long long)raid_group_p->local_state,
                              raid_group_p->raid_group_metadata_memory_peer.base_config_metadata_memory.lifecycle_state);
       return FBE_LIFECYCLE_STATUS_RESCHEDULE;
    }            
}
/**************************************************
 * end fbe_raid_group_clear_rl_request()
 **************************************************/

/*!****************************************************************************
 * fbe_raid_group_clear_rl_passive()
 ******************************************************************************
 * @brief
 *  We are in the clear rl started local state.
 * 
 *  This is the passive side's handler for clearing rebuild logging.
 *  It will just set its clustered clear rl started flag and bitmap and wait for
 *  the active side to do the clear.
 * 
 *  When the Active side completes the clear, we will transition to the done
 *  local state.
 *
 * @param raid_group_p - Raid group object
 * @param packet_p - packet pointer.
 * 
 * @return fbe_lifecycle_status_t
 *
 * @author
 *  7/13/2011 - Created. Rob Foley
 *
 ******************************************************************************/
fbe_lifecycle_status_t 
fbe_raid_group_clear_rl_passive(fbe_raid_group_t *  raid_group_p, fbe_packet_t * packet_p)
{
    fbe_bool_t  b_peer_flag_set = FBE_FALSE;
    fbe_bool_t  b_local_flag_set = FBE_FALSE;
    fbe_u32_t   local_clear_rl_bitmap;
    fbe_u16_t   dead_drive_bitmask;
    fbe_raid_position_bitmask_t rl_bitmask;
    fbe_bool_t b_is_active;
    fbe_scheduler_hook_status_t        hook_status = FBE_SCHED_STATUS_OK;

    fbe_raid_group_lock(raid_group_p);

    fbe_raid_group_is_peer_flag_set(raid_group_p, &b_peer_flag_set, 
                                    FBE_RAID_GROUP_CLUSTERED_FLAG_CLEAR_RL_STARTED);

    if (fbe_raid_group_is_clustered_flag_set(raid_group_p, FBE_RAID_GROUP_CLUSTERED_FLAG_CLEAR_RL_STARTED))
    {
        b_local_flag_set = FBE_TRUE;
    }    

    /* We first wait for the peer to start the eval rebuild logging. 
     * This is needed since either active or passive can initiate the eval. 
     * Once we see this set, we will set our flag, which the peer waits for before it completes the eval.  
     */
    if (!b_peer_flag_set && !b_local_flag_set)
    {
        fbe_raid_group_unlock(raid_group_p);
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "Passive waiting peer CLEAR_RL_STARTED local: 0x%llx peer: 0x%llx state: 0x%llx\n",
                              (unsigned long long)raid_group_p->raid_group_metadata_memory.flags,
                              (unsigned long long)raid_group_p->raid_group_metadata_memory_peer.flags,
                              (unsigned long long)raid_group_p->local_state);
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /* At this point the peer started flag is set.
     * If we have not set this yet locally, set it and wait for the condition to run to send it to the peer.
     */
    if (!b_local_flag_set)
    {
        fbe_raid_group_unlock(raid_group_p);

        /* Determine which drives are rebuild logging but are back now.
         */
        fbe_raid_group_get_failed_pos_bitmask(raid_group_p, &dead_drive_bitmask);
        fbe_raid_group_get_rb_logging_bitmask(raid_group_p, &rl_bitmask);

        /* Everything that is alive (~dead_drive_bitmask) and is in the rl bitmask 
         * need to have rebuild logging cleared.
         */
        local_clear_rl_bitmap = rl_bitmask & (~dead_drive_bitmask);
    
        fbe_base_object_trace((fbe_base_object_t*) raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,  
                              "RL clear passive io local_clear_bitmap: 0x%x rl_b: 0x%x dead_b: 0x%x\n", local_clear_rl_bitmap,
                              rl_bitmask, dead_drive_bitmask);
        fbe_raid_group_lock(raid_group_p); 
        fbe_raid_group_set_clustered_flag_and_bitmap(raid_group_p, FBE_RAID_GROUP_CLUSTERED_FLAG_CLEAR_RL_STARTED,
                                                     local_clear_rl_bitmap, 0, /* not used */ 0);
        fbe_raid_group_unlock(raid_group_p);

        fbe_base_object_trace((fbe_base_object_t *)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "Passive mark CLEAR_RL_START locfl:0x%llx peerfl:0x%llx"
                              " locmap:0x%x peermap:0x%x state:0x%llx\n",
                              (unsigned long long)raid_group_p->raid_group_metadata_memory.flags,
                              (unsigned long long)raid_group_p->raid_group_metadata_memory_peer.flags,
                              raid_group_p->raid_group_metadata_memory.failed_drives_bitmap,
                              raid_group_p->raid_group_metadata_memory_peer.failed_drives_bitmap,
                              (unsigned long long)raid_group_p->local_state);

        return FBE_LIFECYCLE_STATUS_DONE;
    }

    fbe_raid_group_unlock(raid_group_p);

    fbe_check_rg_monitor_hooks(raid_group_p,
                               SCHEDULER_MONITOR_STATE_RAID_GROUP_CLEAR_RL,
                               FBE_RAID_GROUP_SUBSTATE_CLEAR_RL_STARTED, 
                               &hook_status);
    if (hook_status == FBE_SCHED_STATUS_DONE) {
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /* If we become active, just return out.
     * We need to handle the active's work ourself. 
     * When we come back into the condition we'll automatically execute the active code.
     */
	fbe_raid_group_monitor_check_active_state(raid_group_p, &b_is_active);
    if (b_is_active)
    {
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
        fbe_raid_group_lock(raid_group_p);
        fbe_raid_group_set_local_state(raid_group_p, FBE_RAID_GROUP_LOCAL_STATE_CLEAR_RL_DONE);
        fbe_raid_group_clear_local_state(raid_group_p, FBE_RAID_GROUP_LOCAL_STATE_CLEAR_RL_STARTED);
        fbe_raid_group_unlock(raid_group_p);

        fbe_base_object_trace((fbe_base_object_t *)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "clear rl Passive start cleanup local: 0x%llx peer: 0x%llx\n",
                              (unsigned long long)raid_group_p->raid_group_metadata_memory.flags,
                              (unsigned long long)raid_group_p->raid_group_metadata_memory_peer.flags);
        return FBE_LIFECYCLE_STATUS_RESCHEDULE;
    }
    else
    {
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "clear rl Passive wait for peer to complete\n");
        return FBE_LIFECYCLE_STATUS_DONE;   
    } 
}
/**************************************************
 * end fbe_raid_group_clear_rl_passive()
 **************************************************/
/*!****************************************************************************
 * fbe_raid_group_clear_rl_active()
 ******************************************************************************
 * @brief
 *  This function is the active SP function which clears and writes out the
 *  new rebuild logging bitmask.
 * 
 *  Once the peer has set started, the active side will evaluate what needs
 *  to get cleared using the bitmaps exchanged with the peer.
 * 
 *  The active side will then write out the new rebuild logging bitmap.
 *  
 *
 * @param raid_group_p - Raid group object
 * @param packet_p - packet pointer.
 * 
 * @return fbe_lifecycle_status_t
 *
 * @author
 *  7/13/2011 - Created. Rob Foley
 *
 ******************************************************************************/
fbe_lifecycle_status_t 
fbe_raid_group_clear_rl_active(fbe_raid_group_t *  raid_group_p, fbe_packet_t * packet_p)
{    
    fbe_status_t                status = FBE_STATUS_OK;
    fbe_bool_t                  b_peer_flag_set = FBE_FALSE;
    fbe_raid_position_bitmask_t clear_bitmap_local = 0;
    fbe_u16_t                   unused       = 0;
    fbe_raid_position_bitmask_t clear_bitmap_peer  = 0;
    fbe_raid_position_bitmask_t clear_rl_bitmask = 0;
    fbe_scheduler_hook_status_t hook_status = FBE_SCHED_STATUS_OK;
    fbe_class_id_t              raid_group_class_id;
    fbe_bool_t                  b_is_mark_nr_required = FBE_FALSE;
    
    /* Quiesce the IO before proceeding.
     * We need to attempt a full drain in case the block size needs to be changed. 
     * Block size change cannot occur unless there are no I/Os in flight. 
     */
    raid_group_class_id = fbe_base_object_get_class_id((fbe_object_handle_t)raid_group_p);
    status = fbe_raid_group_start_drain(raid_group_p);
    if (status == FBE_STATUS_PENDING)
    {
        return FBE_LIFECYCLE_STATUS_RESCHEDULE; 
    }

    fbe_base_object_trace((fbe_base_object_t*) raid_group_p, 
                          FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,  
                          "%s class: %d io drained\n", __FUNCTION__, raid_group_class_id);

    fbe_raid_group_lock(raid_group_p);
    fbe_raid_group_get_failed_pos_bitmask_to_clear(raid_group_p, &clear_bitmap_local);

    /* Now we set the bit that says we are starting. 
     * This bit always gets set first by the Active side. 
     * When passive sees the started peer bit, it will also set the started bit. 
     * The passive side will thereafter know we are done when this bit is cleared. 
     */
    if (!fbe_raid_group_is_clustered_flag_set(raid_group_p, FBE_RAID_GROUP_CLUSTERED_FLAG_CLEAR_RL_STARTED))
    {                        
        fbe_raid_group_set_clustered_flag_and_bitmap(raid_group_p, FBE_RAID_GROUP_CLUSTERED_FLAG_CLEAR_RL_STARTED,
                                                     clear_bitmap_local, 0,/* not used */ 0);        

        fbe_raid_group_unlock(raid_group_p);
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "Active mark CLEAR_RL_START locfl:0x%llx peerfl:0x%llx"
                              " locmap:0x%x peermap:0x%x state:0x%llx\n",
                              (unsigned long long)raid_group_p->raid_group_metadata_memory.flags,
                              (unsigned long long)raid_group_p->raid_group_metadata_memory_peer.flags,
                              raid_group_p->raid_group_metadata_memory.failed_drives_bitmap,
                              raid_group_p->raid_group_metadata_memory_peer.failed_drives_bitmap,
                              (unsigned long long)raid_group_p->local_state);
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    fbe_check_rg_monitor_hooks(raid_group_p,
                               SCHEDULER_MONITOR_STATE_RAID_GROUP_CLEAR_RL,
                               FBE_RAID_GROUP_SUBSTATE_CLEAR_RL_STARTED, 
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
    fbe_raid_group_is_peer_flag_set_and_get_peer_bitmap(raid_group_p, &b_peer_flag_set, 
                                                        FBE_LIFECYCLE_STATE_READY, 
                                                        FBE_RAID_GROUP_CLUSTERED_FLAG_CLEAR_RL_STARTED,
                                                        &clear_bitmap_peer,
                                                        &unused);
    if (!b_peer_flag_set)
    {
        fbe_raid_group_unlock(raid_group_p);
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "Active wait peer CLEAR_RL_START localfl:0x%llx peerfl:0x%llx"
                              " locmap:0x%x peermap:0x%x state:0x%llx\n",
                              (unsigned long long)raid_group_p->raid_group_metadata_memory.flags,
                              (unsigned long long)raid_group_p->raid_group_metadata_memory_peer.flags,
                              raid_group_p->raid_group_metadata_memory.failed_drives_bitmap,
                              raid_group_p->raid_group_metadata_memory_peer.failed_drives_bitmap,
                              (unsigned long long)raid_group_p->local_state);
        return FBE_LIFECYCLE_STATUS_DONE;
    } 

    // Combine the local and peer failed pos bitmap    
    fbe_raid_group_get_local_bitmap(raid_group_p, &clear_bitmap_local, &unused);
    fbe_raid_group_get_peer_bitmap(raid_group_p, FBE_LIFECYCLE_STATE_READY,
                                   &clear_bitmap_peer, &unused);
    fbe_raid_group_unlock(raid_group_p); 
    
    fbe_raid_group_get_clear_rl_bits(raid_group_p, &clear_rl_bitmask);

    /* We cannot clear rebuild logging if `mark NR required' is set.
     */
    if (raid_group_class_id == FBE_CLASS_ID_VIRTUAL_DRIVE)
    {
        /* Determine if mark NR is required.
         */
        status = fbe_raid_group_metadata_is_mark_nr_required(raid_group_p, &b_is_mark_nr_required);
        if (status != FBE_STATUS_OK)
        {
            /* Trace and error, leave the condition set and return done.
             */
            fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s Is vd mark nr required failed - status: 0x%x \n",
                              __FUNCTION__, status);
            return FBE_LIFECYCLE_STATUS_DONE;
        }

        /* If there are positions to clear and the mark NR required flag is set
         * we cannot proceed.
         */
        if ((b_is_mark_nr_required == FBE_TRUE) &&
            (clear_rl_bitmask != 0)                )
        {
            /* The eval mark NR is a preset so we simply need to clear this
             * condition.  When eval mark NR is complete it will be re-signalled.
             */
            fbe_base_object_trace((fbe_base_object_t*)raid_group_p,
                              FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "raid_group: clear rl bitmask: 0x%x wait for mark NR required to be clear\n",
                              clear_rl_bitmask);

            /* Mark clear rebuild logging as complete.
             */
            fbe_raid_group_lock(raid_group_p);
            fbe_raid_group_set_local_state(raid_group_p, FBE_RAID_GROUP_LOCAL_STATE_CLEAR_RL_DONE);
            fbe_raid_group_clear_local_state(raid_group_p, FBE_RAID_GROUP_LOCAL_STATE_CLEAR_RL_STARTED);
            fbe_raid_group_unlock(raid_group_p);
            return FBE_LIFECYCLE_STATUS_RESCHEDULE;
        }
    }

    /* Check the bitmask of positions that should have rebuild logging cleared.
     */
    if (clear_rl_bitmask != 0)
    {
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p,
                              FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "raid_group: get checkpoints then clear rl 0x%x \n",
                              clear_rl_bitmask);

        /* Get the checkpoint of each virtual drive that is coming out of 
         * rebuild logging.
         */
        status = fbe_raid_group_clear_rl_get_checkpoints(packet_p,raid_group_p,clear_rl_bitmask);
        return FBE_LIFECYCLE_STATUS_PENDING;
    }
    else
    {
        /* There is nothing to do, Just transition through to the done state.
         */
        fbe_raid_group_lock(raid_group_p);
        fbe_raid_group_set_local_state(raid_group_p, FBE_RAID_GROUP_LOCAL_STATE_CLEAR_RL_DONE);
        fbe_raid_group_clear_local_state(raid_group_p, FBE_RAID_GROUP_LOCAL_STATE_CLEAR_RL_STARTED);
        fbe_raid_group_unlock(raid_group_p);
        return FBE_LIFECYCLE_STATUS_RESCHEDULE;
    }
}
/**************************************************
 * end fbe_raid_group_clear_rl_active()
 **************************************************/

/*!****************************************************************************
 * fbe_raid_group_clear_rl_done()
 ******************************************************************************
 * @brief
 *  This function is the last step in clear RL where the clustered flags
 * and local flags will be cleared on both sides and the cond will be cleared.
 *
 * @param raid_group_p - Raid group object
 * @param packet_p - packet pointer.
 * 
 * @return fbe_lifecycle_status_t
 *
 * @author
 *  7/13/2011 - Created. Rob Foley
 *
 ******************************************************************************/
fbe_lifecycle_status_t 
fbe_raid_group_clear_rl_done(fbe_raid_group_t *  raid_group_p, fbe_packet_t * packet_p)
{
    fbe_status_t status;
	fbe_bool_t b_is_active;
    fbe_bool_t b_peer_flag_set = FBE_FALSE;
    fbe_scheduler_hook_status_t        hook_status = FBE_SCHED_STATUS_OK;

	fbe_raid_group_monitor_check_active_state(raid_group_p, &b_is_active);

    fbe_raid_group_lock(raid_group_p);

    /* We want to clear our local flags first.
     */
    if (fbe_raid_group_is_clustered_flag_set(raid_group_p, FBE_RAID_GROUP_CLUSTERED_FLAG_CLEAR_RL_REQ) ||
        fbe_raid_group_is_clustered_flag_set(raid_group_p, FBE_RAID_GROUP_CLUSTERED_FLAG_CLEAR_RL_STARTED))
    {
        fbe_raid_group_clear_clustered_flag_and_bitmap(raid_group_p, 
                                                       (FBE_RAID_GROUP_CLUSTERED_FLAG_CLEAR_RL_REQ | 
                                                        FBE_RAID_GROUP_CLUSTERED_FLAG_CLEAR_RL_STARTED));
    }

    fbe_check_rg_monitor_hooks(raid_group_p,
                               SCHEDULER_MONITOR_STATE_RAID_GROUP_CLEAR_RL,
                               FBE_RAID_GROUP_SUBSTATE_CLEAR_RL_DONE, 
                               &hook_status);
    if (hook_status == FBE_SCHED_STATUS_DONE) {
        fbe_raid_group_unlock(raid_group_p);
        return FBE_LIFECYCLE_STATUS_DONE;
    }


    /* Next we wait for the peer to cleanup by clearing the started bit.
     */
    fbe_raid_group_is_peer_flag_set(raid_group_p, &b_peer_flag_set,
                                    FBE_RAID_GROUP_CLUSTERED_FLAG_CLEAR_RL_STARTED);
    if (fbe_raid_group_is_peer_present(raid_group_p) &&
        b_peer_flag_set)
    {
        fbe_raid_group_unlock(raid_group_p);
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "clear rl cleanup %s, wait peer done local: 0x%llx peer: 0x%llx state:0x%llx\n", 
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
                                         (FBE_RAID_GROUP_LOCAL_STATE_CLEAR_RL_STARTED | 
                                          FBE_RAID_GROUP_LOCAL_STATE_CLEAR_RL_DONE)); 

        /* It is possible that we received a request to clear either locally or from the peer. If a request is not set,
         * let's clear the condition.  Otherwise we will run this condition again, and when we see that the started and 
         * done flags are not set, we'll re-evaluate rebuild logging. 
         */
        if (!fbe_raid_group_is_local_state_set(raid_group_p, FBE_RAID_GROUP_LOCAL_STATE_CLEAR_RL_REQUEST))
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

        /* Now unlock and trace some information.
         */
        fbe_raid_group_unlock(raid_group_p);
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "clear rl %s cleanup complete local: 0x%llx peer: 0x%llx state: 0x%llx\n",
                              b_is_active ? "Active" : "Passive",
                              (unsigned long long)raid_group_p->raid_group_metadata_memory.flags,
                              (unsigned long long)raid_group_p->raid_group_metadata_memory_peer.flags,
                              (unsigned long long)raid_group_p->local_state);

        /* We are done with clear rebuild logging, let's issue the continue.
         * Before we issue the continue, it is necessary to clear hold. 
         */
        fbe_metadata_element_clear_abort(&raid_group_p->base_config.metadata_element);
        fbe_raid_group_handle_continue(raid_group_p);
        return FBE_LIFECYCLE_STATUS_RESCHEDULE;
    }    
}
/**************************************************
 * end fbe_raid_group_clear_rl_done()
 **************************************************/

/*!****************************************************************************
 * fbe_raid_group_write_clear_rl_completion()
 ******************************************************************************
 * @brief
 *  This function is the completion function for writing out
 *  the cleared rebuild logging bits.
 * 
 * @param in_packet_p   - pointer to a control packet from the scheduler
 * @param in_context    - completion context, which is a pointer to the raid 
 *                        group object
 * 
 * @return  fbe_status_t  
 *
 * @author
 *  7/14/2011 - Created. Rob Foley
 *
 ******************************************************************************/
static fbe_status_t 
fbe_raid_group_write_clear_rl_completion(fbe_packet_t *packet_p,
                                         fbe_packet_completion_context_t context_p)

{
    fbe_raid_group_t            *raid_group_p = (fbe_raid_group_t*) context_p;                   
    fbe_status_t                packet_status;
    fbe_payload_ex_t           *sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    fbe_raid_iots_t            *iots_p = NULL;
    fbe_raid_position_t         position;
    fbe_u32_t                   width;
    fbe_raid_position_bitmask_t clear_rl_bitmask = 0; 
    
    /* If packet failed */
    packet_status = fbe_transport_get_status_code(packet_p);
    if (packet_status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p, 
                              FBE_TRACE_LEVEL_WARNING, 
                              FBE_TRACE_MESSAGE_ID_INFO, 
                              "%s: error %d on nonpaged write call\n", 
                              __FUNCTION__,  packet_status);
        return FBE_STATUS_OK;
    }

    /* We are done with the writing, the bits are cleared, so we can log now.
     */
    fbe_payload_ex_get_iots(sep_payload_p, &iots_p);
    fbe_raid_iots_get_rebuild_logging_bitmask(iots_p, &clear_rl_bitmask);

    /* If the rebuild logging mask is 0 we shouldn't be here.
     */
    if (clear_rl_bitmask == 0)
    {
        /* Trace an error and return.
         */
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s - Unexpected zero clear_rl_bitmask: 0x%x \n",
                              __FUNCTION__, clear_rl_bitmask);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Log an event for each of the associated positions that are no longer
     * rebuild logging.
     */
    position = 0;
    fbe_raid_group_get_width(raid_group_p, &width);
    for ( position = 0; position < width; position++)
    {
        if (clear_rl_bitmask & (1 << position))
        {
            fbe_raid_group_logging_log_rl_cleared(raid_group_p, packet_p, position);
        }
    }

    /* We are successful clearing rebuild logging, so clear the started bit and set the done bit.
     */
    fbe_raid_group_lock(raid_group_p);
    fbe_raid_group_set_local_state(raid_group_p, FBE_RAID_GROUP_LOCAL_STATE_CLEAR_RL_DONE);
    fbe_raid_group_clear_local_state(raid_group_p, FBE_RAID_GROUP_LOCAL_STATE_CLEAR_RL_STARTED);
    fbe_raid_group_unlock(raid_group_p);
    
    /* we have some work to do and so reschedule the monitor immediately */
    fbe_lifecycle_reschedule(&fbe_raid_group_lifecycle_const, (fbe_base_object_t*) raid_group_p, 0); 

    return FBE_STATUS_OK; 
} 
/**************************************************
 * end fbe_raid_group_write_clear_rl_completion()
 **************************************************/
/*!**************************************************************
 * fbe_raid_group_get_clear_rl_bits()
 ****************************************************************
 * @brief
 *  This function looks for positions to clear rebuild logging on.
 *
 * @param raid_group_p - Ptr to current rg.
 * @param change_bitmask_p - The bitmask of 
 * 
 * @todo revisit for single loop failure
 * 
 * @return fbe_status_t
 *
 * @author
 *  7/13/2011 - Created. Rob Foley
 *
 ****************************************************************/

static fbe_status_t fbe_raid_group_get_clear_rl_bits(fbe_raid_group_t *raid_group_p,
                                                     fbe_raid_position_bitmask_t *clear_bitmask_p)

{         
    fbe_u32_t disk_index;
    fbe_raid_position_bitmask_t rl_change_bitmask;
    fbe_raid_position_bitmask_t clear_disk_pos_bitmap_local;
    fbe_raid_position_bitmask_t failed_ios_bitmap;
    fbe_raid_position_bitmask_t clear_disk_pos_bitmap_peer;
    fbe_u32_t  clear_disk_pos_bitmap;
    fbe_u32_t pos;
    fbe_u32_t width;

    /* By default we have no bits to clear.  If we find bits to clear we will add them here.
     */
    *clear_bitmask_p = 0;

    fbe_raid_group_get_local_bitmap(raid_group_p, &clear_disk_pos_bitmap_local, 
                                    &failed_ios_bitmap /* not used */);
    fbe_raid_group_get_peer_bitmap(raid_group_p, FBE_LIFECYCLE_STATE_READY, 
                                   &clear_disk_pos_bitmap_peer,
                                   &failed_ios_bitmap /* not used */);

    fbe_base_object_trace((fbe_base_object_t*) raid_group_p,      
                          FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,  
                          "clear rl Active local clear bitmap 0x%x peer clear bitmap 0x%x\n",
                          clear_disk_pos_bitmap_local, clear_disk_pos_bitmap_peer); 

    /* For now, combine the bitmaps and operate on them together
     */ 
    clear_disk_pos_bitmap = clear_disk_pos_bitmap_local | clear_disk_pos_bitmap_peer;

    if (fbe_raid_group_is_peer_present(raid_group_p) &&
        (clear_disk_pos_bitmap_local != clear_disk_pos_bitmap_peer))
    {
        /* Since we disagree, there is nothing to clear now.  We cannot clear rl if one of the sides still does not
         * see the drive.  Just tell the peer we are done, it will inform us if it sees the drive come back or this 
         * active side will run this code again if it sees the drive come back. 
         */
        fbe_base_object_trace((fbe_base_object_t*) raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY, 
                              "clear rl local bitmap 0x%x != peer bitmap 0x%x\n",
                              clear_disk_pos_bitmap_local, clear_disk_pos_bitmap_peer);
        return FBE_STATUS_OK;
    }
    
    /*  Loop to clear rl for all disks that need it.
     */
    fbe_raid_group_get_width(raid_group_p, &width);
    rl_change_bitmask = 0; 
    for (pos = 0; pos < width; pos++)
    {
        //  If bit not set, continue
        if (!(clear_disk_pos_bitmap & (1 << pos)))
        {
            continue;
        }

        //  Set the disk_index
        disk_index = pos;

        //  Set this disk position in the bitmask indicating which rebuild logging bits are to be changed 
        rl_change_bitmask = rl_change_bitmask | (1 << disk_index);
    }
    *clear_bitmask_p = rl_change_bitmask;

    //  Return the status from the call, which will indicate if we have a packet outstanding or not 
    return FBE_STATUS_OK; 
}
/******************************************
 * end fbe_raid_group_get_clear_rl_bits()
 ******************************************/

/*!***************************************************************************
 * fbe_raid_group_clear_rl_get_checkpoints()
 *****************************************************************************
 * @brief
 *  This function allocates memory for subpackets to be sent to vd
 *  We need to get the vd checkpoints before clearing rl because
 *  there is a use case where a hotspare swaps in, but before we 
 *  could mark nr the the SP went down and came back. In that case
 *  we should not clear rl until we have marked nr. We will only
 *  clear rl when the vd checkpoint is invalid. In the use case 
 *  that I mentioned the vd checkpoint will not be invalid.
 *  
 * @param packet_p   
 * @param raid_group_p 
 * @param clear_bitmask 
 * 
 * 
 * @return fbe_status_t
 *
 * @author
 *  09/01/2011 - Created. Ashwin Tamilarasan
 *
 *****************************************************************************/
static fbe_status_t fbe_raid_group_clear_rl_get_checkpoints(fbe_packet_t *packet_p,
                                                   fbe_raid_group_t *raid_group_p,
                                                   fbe_raid_position_bitmask_t clear_bitmask)                                                                       
{
    
    fbe_status_t                                status;    
    fbe_u32_t                                   bitcount;
    fbe_base_config_memory_allocation_chunks_t  mem_alloc_chunks = {0};
    fbe_atomic_t                                *new_edges_bitmask_p = NULL;   // We need to use the atomic variable. 
    fbe_payload_ex_t                           *sep_payload_p = NULL;
    fbe_class_id_t                              raid_group_class_id = fbe_raid_group_get_class_id(raid_group_p);


    /*! @note This method now supports both normal raid groups and the 
     *        virtual drive class.
     */
    new_edges_bitmask_p = (fbe_atomic_t *)fbe_transport_allocate_buffer();
    /* We are setting the bitmask to zero here. 
     * As the checkpoint reads complete they will OR in any position that has a -1 checkpoint. 
     */
    *new_edges_bitmask_p = 0;
    fbe_raid_group_allocate_and_build_control_operation(raid_group_p,packet_p,
                                                        FBE_VIRTUAL_DRIVE_CONTROL_CODE_GET_CHECKPOINT,
                                                        (void *)new_edges_bitmask_p,
                                                        sizeof(fbe_atomic_t));

    //  Increment the control operation
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);    
    fbe_payload_ex_increment_control_operation_level(sep_payload_p);
    fbe_transport_set_completion_function(packet_p, fbe_raid_group_clear_rl_get_checkpoints_completion, raid_group_p);
    bitcount = fbe_raid_get_bitcount((fbe_u32_t)clear_bitmask); 

    // Set the various counts and buffer size 
    mem_alloc_chunks.buffer_count = bitcount;
    mem_alloc_chunks.number_of_packets = bitcount;
    mem_alloc_chunks.sg_element_count = 0;
    mem_alloc_chunks.buffer_size = sizeof(fbe_raid_group_get_checkpoint_info_t);
    mem_alloc_chunks.alloc_pre_read_desc = FALSE; 

    /* Trace this method.
     */
    fbe_base_object_trace((fbe_base_object_t*)raid_group_p,
                          FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                          "raid_group: get checkpoints clear_bitmask: 0x%x bitcount: %d class: %d\n",
                          (fbe_raid_position_bitmask_t)clear_bitmask, bitcount, raid_group_class_id);

    // allocate the subpackets to collect the rebuild checkpoint for all the edges. 
    status = fbe_base_config_memory_allocate_chunks((fbe_base_config_t *) raid_group_p,
                                                    packet_p,
                                                    mem_alloc_chunks,
                                                    (fbe_memory_completion_function_t)fbe_raid_group_clear_rl_get_checkpoints_memory_allocation_completion); /* SAFEBUG - cast to supress warning but must fix because memory completion function shouldn't return status */
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s memory allocation failed, status:0x%x\n", __FUNCTION__, status);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);        
        fbe_transport_complete_packet(packet_p);
        return status; 
    }    

    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
      

}/**************************************************
 * end fbe_raid_group_clear_rl_get_checkpoints()
 ***************************************************/


/*!**************************************************************************
 * fbe_raid_group_clear_rl_get_checkpoints_memory_allocation_completion()
 *****************************************************************************
 * @brief
 *  This function access the memory that was allocated and carves into
 *  subpackets and other buffers that is needed. Adds the subpackets to
 *  the master packet queue.
 *  This function gets the bit position which indicates the edge index
 *  and send the packets only to those edges. Raid sends usurper command
 *  to VD to get the VD checkpoint. 
 *
 * @param memory_request_p  
 * @param in_context   
 *
 * @return status - The status of the operation.
 *
 * @author
 *  09/06/2011 - Created. Ashwin Tamilarasan
 *
 ****************************************************************/
static fbe_status_t fbe_raid_group_clear_rl_get_checkpoints_memory_allocation_completion(fbe_memory_request_t * memory_request_p, 
                                                                                fbe_memory_completion_context_t in_context)
{
    fbe_packet_t**                          sub_packet_p = NULL;
    fbe_raid_group_get_checkpoint_info_t**           rg_server_obj_info_p = NULL; 
    fbe_u8_t**                              buffer_p = NULL;   
    fbe_packet_t*                           packet_p = NULL;
    fbe_raid_group_t*                       raid_group_p = NULL;    
    fbe_raid_position_bitmask_t             clear_rl_bitmask;            
    fbe_u32_t                               edge_index = 0;
    fbe_u32_t                               index = 0;    
    fbe_u32_t                               bitcount = 0;
    fbe_status_t                            status;
    fbe_bool_t                              added_to_queue = FBE_FALSE;
    fbe_block_edge_t*                       edge_p = NULL; 
    fbe_base_config_memory_alloc_chunks_address_t   mem_alloc_chunks_addr = {0};
    fbe_sg_element_t                       *pre_read_sg_list_p = NULL;
    fbe_payload_pre_read_descriptor_t      *pre_read_desc_p = NULL;
    fbe_raid_geometry_t                    *raid_geometry_p;
    fbe_object_id_t                         raid_group_object_id;
    fbe_class_id_t                          class_id; 
    fbe_bool_t                              is_sys_rg = FBE_FALSE;
    fbe_payload_control_operation_opcode_t  opcode = FBE_VIRTUAL_DRIVE_CONTROL_CODE_GET_CHECKPOINT;

    /* Get some information to determine if this is a virtual drive or not.
     */
    raid_group_p = (fbe_raid_group_t*)in_context;    
    raid_geometry_p = fbe_raid_group_get_raid_geometry(raid_group_p);
    class_id = fbe_raid_geometry_get_class_id(raid_geometry_p); 
        
    // get the pointer to original packet.
    packet_p = fbe_transport_memory_request_to_packet(memory_request_p);

    // Check allocation status 
    if (fbe_memory_request_is_allocation_complete(memory_request_p) == FBE_FALSE)
    {        
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,                                        
                              "%s: Memory request: 0x%p state: %d failed.\n",
                              __FUNCTION__, memory_request_p, memory_request_p->request_state);        
        fbe_transport_set_status(packet_p, FBE_STATUS_INSUFFICIENT_RESOURCES, 0);        
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }


    fbe_raid_group_get_clear_rl_bits(raid_group_p, &clear_rl_bitmask);
    bitcount = fbe_raid_get_bitcount((fbe_u32_t)clear_rl_bitmask); 

    /* Trace some information.
     */
    fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,                                        
                          "raid_group: clear rl get checkpoints mem complete class: %d bitcount: %d clear_rl_bitmask: 0x%x\n",
                          class_id, bitcount, clear_rl_bitmask);        

    // If more than (1) edge needs rl cleared something is wrong.  
    if ((class_id == FBE_CLASS_ID_VIRTUAL_DRIVE) &&
        (bitcount >   1)                            )
    { 
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s bitcount: %d too large for class: %d\n",
                              __FUNCTION__, bitcount, class_id);
        //fbe_memory_request_get_entry_mark_free(memory_request_p, &memory_ptr);
        //fbe_memory_free_entry(memory_ptr);        
		fbe_memory_free_request_entry(memory_request_p);

        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }
        
    // subpacket control buffer size.
    mem_alloc_chunks_addr.buffer_size = sizeof(fbe_raid_group_get_checkpoint_info_t);
    mem_alloc_chunks_addr.number_of_packets = bitcount;
    mem_alloc_chunks_addr.buffer_count = bitcount;
    mem_alloc_chunks_addr.sg_element_count = 0;
    mem_alloc_chunks_addr.sg_list_p = NULL;
    mem_alloc_chunks_addr.assign_pre_read_desc = FALSE;
    mem_alloc_chunks_addr.pre_read_sg_list_p = &pre_read_sg_list_p;
    mem_alloc_chunks_addr.pre_read_desc_p = &pre_read_desc_p;
    
    // get the zod sgl pointer and and subpacket pointer from the allocated memory.
    status = fbe_base_config_memory_assign_address_from_allocated_chunks((fbe_base_config_t *) raid_group_p,
                                                                         memory_request_p,
                                                                         &mem_alloc_chunks_addr);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s memory distribution failed, status:0x%x, bitcount:0x%x, buffer_size:0x%x\n",
                              __FUNCTION__, status, bitcount, mem_alloc_chunks_addr.buffer_size);
        //fbe_memory_request_get_entry_mark_free(memory_request_p, &memory_ptr);
        //fbe_memory_free_entry(memory_ptr);        
		fbe_memory_free_request_entry(memory_request_p);

        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return status;
    }

    sub_packet_p = mem_alloc_chunks_addr.packet_array_p;
    buffer_p = mem_alloc_chunks_addr.buffer_array_p;
    rg_server_obj_info_p = (fbe_raid_group_get_checkpoint_info_t**)buffer_p;

    // Access the memory request and add the subpackets to the master packet queue
    while(index < bitcount)
    {
                
        fbe_transport_initialize_sep_packet(sub_packet_p[index]);            
        rg_server_obj_info_p[index] = (fbe_raid_group_get_checkpoint_info_t*)buffer_p[index]; 
        fbe_zero_memory(rg_server_obj_info_p[index], sizeof(fbe_raid_group_get_checkpoint_info_t));
                         
    
        // Add the newly created packet in subpacket queue and put the master packet to usurper queue.
        fbe_transport_add_subpacket(packet_p, sub_packet_p[index]);
        if(added_to_queue == FBE_FALSE)
        {
            fbe_transport_set_cancel_function(packet_p, fbe_base_object_packet_cancel_function, raid_group_p);
            fbe_base_object_add_to_usurper_queue((fbe_base_object_t*)raid_group_p, packet_p);
            added_to_queue = FBE_TRUE;
        }                    
        index++;           
    } 
    
    fbe_raid_group_utils_is_sys_rg(raid_group_p, &is_sys_rg);
    if(is_sys_rg)
    {
        opcode = FBE_PROVISION_DRIVE_CONTROL_CODE_GET_CHECKPOINT;
    }
    else
    {
        opcode = FBE_VIRTUAL_DRIVE_CONTROL_CODE_GET_CHECKPOINT;
    }

    // Get the edge index that was set from the bit position and send
    //   the packet along that edge to get the vd checkpoint       
    for(index = 0; index < bitcount; index++)
    {        
        fbe_raid_group_get_and_clear_next_set_position(raid_group_p, &clear_rl_bitmask, &edge_index);
        rg_server_obj_info_p[index]->edge_index = edge_index;
        rg_server_obj_info_p[index]->requestor_class_id = class_id;       
        rg_server_obj_info_p[index]->request_type = FBE_RAID_GROUP_GET_CHECKPOINT_REQUEST_TYPE_CLEAR_RL;       
        fbe_raid_group_allocate_and_build_control_operation(raid_group_p, sub_packet_p[index],
                                                            opcode,
                                                            rg_server_obj_info_p[index],
                                                            sizeof(fbe_raid_group_get_checkpoint_info_t));
        // Set our completion function.
        fbe_transport_set_completion_function(sub_packet_p[index], 
                                             fbe_raid_group_clear_rl_get_checkpoints_subpacket_completion,
                                             raid_group_p);

        if(class_id == FBE_CLASS_ID_VIRTUAL_DRIVE)
        {    
            //  Get the RAID group object-id.
            fbe_base_object_get_object_id((fbe_base_object_t *) raid_group_p, &raid_group_object_id);

            //  We are sending control packet to itself. So form the address manually
            fbe_transport_set_address(sub_packet_p[index],
                                      FBE_PACKAGE_ID_SEP_0,
                                      FBE_SERVICE_ID_TOPOLOGY,
                                      FBE_CLASS_ID_INVALID,
                                      raid_group_object_id);

             //  Control packets should be sent via service manager.
             status = fbe_service_manager_send_control_packet(sub_packet_p[index]);  
             return status;      
        }
        else
        {
            // Send the packet to the VD object on this edge.
            fbe_base_config_get_block_edge((fbe_base_config_t*)raid_group_p, &edge_p, edge_index);
            fbe_block_transport_send_control_packet(edge_p, sub_packet_p[index]);
        }
    }
    
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;

}
/**********************************************
 * end fbe_raid_group_clear_rl_get_checkpoints_memory_allocation_completion()
 ************************************************/


/*!****************************************************************************
 * fbe_raid_group_clear_rl_get_checkpoints_subpacket_completion()
 ******************************************************************************
 * @brief
 *  This function checks the subpacket status, copies the status to the master packet.
 *  It releases the control operation and frees the subpacket. We get the VD checkpoint
 *  and use it to decide whether we are going to clear rebuild logging on this drive or not.
 *  If the VD checkpoint is invalid that means we are good to clear rebuild logging else
 *  we should not clear rebuild logging
 *
 * @param packet_p  
 * @param context             
 *
 * @return status 
 *
 * @author
 *  09/06/2011 - Created. Ashwin Tamilarasan
 *
 ******************************************************************************/
static fbe_status_t fbe_raid_group_clear_rl_get_checkpoints_subpacket_completion(fbe_packet_t* packet_p,
                                                       fbe_packet_completion_context_t context)
{ 
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_raid_group_t                   *raid_group_p = NULL;      
    fbe_packet_t                       *master_packet_p = NULL;    
    fbe_payload_ex_t                   *payload_p = NULL;
    fbe_payload_control_operation_t    *control_operation_p = NULL;            
    fbe_payload_ex_t                   *master_payload_p = NULL;
    fbe_payload_control_operation_t    *master_control_operation_p = NULL;            
    fbe_payload_control_status_t        control_status;
    fbe_raid_group_get_checkpoint_info_t       *rg_server_obj_info_p = NULL; 
    fbe_atomic_t                       *new_edges_bitmask_p = NULL;
    fbe_atomic_t                       tmp_bitmask = 0;
    fbe_lba_t                           vd_checkpoint;
    fbe_u32_t                           disk_position;
	fbe_bool_t                          is_empty;
    fbe_class_id_t                      raid_group_class_id;
    
    raid_group_p = (fbe_raid_group_t*)context;
    raid_group_class_id = fbe_raid_group_get_class_id(raid_group_p);
    
    master_packet_p = (fbe_packet_t *)fbe_transport_get_master_packet(packet_p);        

    /* Get the subpacket payload and control operation. */    
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);     
    fbe_payload_control_get_buffer(control_operation_p, &rg_server_obj_info_p);
    
    status = fbe_transport_get_status_code(packet_p); 
    fbe_payload_control_get_status(control_operation_p, &control_status);    

    /*! If status is not good then complete the master packet with error. 
        If we complete the master packet when other subpackets are pending
        that is wrong. what to do? */
    if((status != FBE_STATUS_OK) || (control_status != FBE_PAYLOAD_CONTROL_STATUS_OK))
    {
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "raid_group: clear rl get checkpoint subpacket index: %d status: 0x%x control_status: 0x%x\n", 
                               rg_server_obj_info_p->edge_index, status, control_status);        
        fbe_transport_set_status(master_packet_p, FBE_STATUS_GENERIC_FAILURE, 0);        
        fbe_payload_ex_release_control_operation(payload_p, control_operation_p);
    } else {    

        /*! @note The virtual drive will now return the correct checkpoint
         *        based on:
         *          o Class id (either virtual drive or other)
         *          o For the virtual drive class the edge index will be
         *            used to determine which check point is returned.
         */  
        vd_checkpoint = rg_server_obj_info_p->downstream_checkpoint;
        disk_position = rg_server_obj_info_p->edge_index;        

        /* Free the resources we allocated previously. */    
        fbe_payload_ex_release_control_operation(payload_p, control_operation_p);

        /* Get the master packet control operation */
        master_payload_p = fbe_transport_get_payload_ex(master_packet_p);
        master_control_operation_p = fbe_payload_ex_get_control_operation(master_payload_p);     
        fbe_payload_control_get_buffer(master_control_operation_p, &new_edges_bitmask_p);

        /*! @note If the vd checkpoint has been set the end marked it 
         *        signifies that all parent raid groups can clear the rl flag.
         */
        if (vd_checkpoint == FBE_LBA_INVALID)

        {
            /* Make sure the following operation is atomic. Because mutilple completion may 
             * access this value concurently */
            tmp_bitmask = ((fbe_u64_t)1 << disk_position);
            fbe_atomic_or(new_edges_bitmask_p, tmp_bitmask);
        }

        fbe_base_object_trace((fbe_base_object_t*)raid_group_p,
                              FBE_TRACE_LEVEL_INFO, 
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "raid_group: clear rl vd_checkpoint: 0x%llx subpacket index: %d new bitmask: 0x%x \n",
                              (unsigned long long)vd_checkpoint, rg_server_obj_info_p->edge_index, (fbe_raid_position_bitmask_t)*new_edges_bitmask_p);
    }

	fbe_transport_remove_subpacket_is_queue_empty(packet_p, &is_empty);
    if (is_empty)
    {
        fbe_transport_destroy_subpackets(master_packet_p);
        /* Remove master packet from the usurper queue. */
        fbe_base_object_remove_from_usurper_queue((fbe_base_object_t*)raid_group_p, master_packet_p);
        fbe_transport_complete_packet(master_packet_p); 
    }
    
    return status;
}
/*************************************************************************** 
 * end of fbe_raid_group_clear_rl_get_checkpoints_subpacket_completion()
 ***************************************************************************/

/*!****************************************************************************
 * fbe_raid_group_clear_rl_get_checkpoints_completion()
 ******************************************************************************
 * @brief
 *  This function looks at the subpacket queue and if it is empty,
 *  retrieves the information from the edges bitmask that was populated
 *  by the subpackets and calls a function to clear the rebuild logging
 *
 * @param packet_p  
 * @param context   
 *
 * @return status 
 *
 * @author
 *  09/07/2011 - Created. Ashwin Tamilarasan
 *
 ******************************************************************************/
static fbe_status_t fbe_raid_group_clear_rl_get_checkpoints_completion(fbe_packet_t *packet_p,
                                                                       fbe_packet_completion_context_t context)
{  
    fbe_raid_group_t                   *raid_group_p = NULL;
    fbe_payload_ex_t                   *sep_payload_p = NULL;
    fbe_payload_control_operation_t    *control_operation_p = NULL;
    fbe_atomic_t                        *new_edges_bitmask_p = NULL;
    fbe_memory_request_t               *memory_request_p = NULL;
    fbe_raid_position_bitmask_t         clear_rl_bitmask = 0;
    fbe_raid_geometry_t                *raid_geometry_p;  

    raid_group_p = (fbe_raid_group_t*)context;
    raid_geometry_p = fbe_raid_group_get_raid_geometry(raid_group_p);
            
    /* Grab the clear_rl_bitmask and free up the memory request -- keep the chunk buffer    
    */
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(sep_payload_p);
    fbe_payload_control_get_buffer(control_operation_p, &new_edges_bitmask_p);
    clear_rl_bitmask = (fbe_raid_position_bitmask_t)*new_edges_bitmask_p;
    memory_request_p = fbe_transport_get_memory_request(packet_p);
    fbe_memory_free_request_entry(memory_request_p);

    if (fbe_raid_group_is_c4_mirror_raid_group(raid_group_p)) {
        fbe_u32_t                          new_pvd_bitmap = 0;
        /* Don't clear RL on new PVD */
        fbe_raid_group_c4_mirror_get_new_pvd(raid_group_p, &new_pvd_bitmap);
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p,
                              FBE_TRACE_LEVEL_INFO, 
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "raid_group: mask out new PVD: 0x%x, current: 0x%x\n",
                              new_pvd_bitmap, clear_rl_bitmask);
        clear_rl_bitmask &= ~new_pvd_bitmap;
    }

    if (clear_rl_bitmask != 0)
    {
        /* Need to rebuild (ie., zero) write log slots before we clear RL, or we can get collisions on the
         * drive between rebuild and normal-io-caused writes to the log
         */
        if (fbe_raid_geometry_is_parity_type(raid_geometry_p))
        {
            /* It's a parity, so rebuild the write log
             */
            fbe_base_object_trace((fbe_base_object_t*)raid_group_p,
                                  FBE_TRACE_LEVEL_INFO, 
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "raid_group: get checkpoints complete clear rl Rebuild journal positions: 0x%x\n", 
                                  clear_rl_bitmask);

            fbe_transport_set_completion_function(packet_p, fbe_raid_group_journal_rebuild_completion_break_context, 
                                                  (fbe_packet_completion_context_t) raid_group_p);    
            fbe_raid_group_journal_rebuild(raid_group_p, packet_p, clear_rl_bitmask);                   

        }
        else
        {
            /* No journal, release memory and continue to clearing RL
             */
            fbe_transport_release_buffer((void *)new_edges_bitmask_p);
            fbe_payload_ex_release_control_operation(sep_payload_p, control_operation_p); 

            /* We have released the buffer containing the clear_rl_bitmask.
             * It is up to fbe_raid_group_write_rl_get_dl to properly populate 
             * the iots with the correct information (clear_b etc.). 
             */
            fbe_base_object_trace((fbe_base_object_t*)raid_group_p,
                                  FBE_TRACE_LEVEL_INFO, 
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "raid_group: get checkpoints complete clear rl Active bits to clear: 0x%x\n", 
                                  clear_rl_bitmask);

            fbe_transport_set_completion_function(packet_p, fbe_raid_group_write_clear_rl_completion, 
                                                  (fbe_packet_completion_context_t) raid_group_p);    
            fbe_raid_group_write_rl_get_dl(raid_group_p, packet_p, clear_rl_bitmask,
                                           FBE_TRUE/* Yes, this is clear rl */);                   
        }

        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }
    else
    {
        /* No rebuild logging bitmask to clear just free the memory and control op and
         * set the local state so that condition gets cleared
        */
        fbe_transport_release_buffer((void *)new_edges_bitmask_p);
        fbe_payload_ex_release_control_operation(sep_payload_p, control_operation_p); 

        fbe_raid_group_lock(raid_group_p);
        fbe_raid_group_set_local_state(raid_group_p, FBE_RAID_GROUP_LOCAL_STATE_CLEAR_RL_DONE);
        fbe_raid_group_clear_local_state(raid_group_p, FBE_RAID_GROUP_LOCAL_STATE_CLEAR_RL_STARTED);
        fbe_raid_group_unlock(raid_group_p);

        /* we have some work to do and so reschedule the monitor immediately */
        fbe_lifecycle_reschedule(&fbe_raid_group_lifecycle_const, (fbe_base_object_t*) raid_group_p, 0); 
    }
    
    return FBE_STATUS_OK;    

}
/*****************************************************************
 * end of fbe_raid_group_clear_rl_get_checkpoints_completion
 *****************************************************************/

/*!****************************************************************************
 * fbe_raid_group_journal_rebuild_completion_break_context()
 ******************************************************************************
 * @brief
 *  This function is the completion function for rebuild journal to break context.
 * 
 * @param in_packet_p   - pointer to a control packet from the scheduler
 * @param in_context    - completion context, which is a pointer to the raid 
 *                        group object
 * 
 * @return  fbe_status_t  
 *
 * @author
 *  7/2/2013 - Created. Lili Chen
 *
 ******************************************************************************/
static fbe_status_t 
fbe_raid_group_journal_rebuild_completion_break_context(fbe_packet_t *packet_p, 
                                                        fbe_packet_completion_context_t context)
{
    fbe_raid_group_t   *raid_group_p;

    /* Cast the context to a pointer to a raid group object.
     */ 
    raid_group_p = (fbe_raid_group_t *)context;

    /* AR 570508: break context here to avoid stack overflow.
     */
    fbe_transport_set_completion_function(packet_p, fbe_raid_group_journal_rebuild_completion_clear_rl, 
                                          (fbe_packet_completion_context_t) raid_group_p);    
    fbe_transport_run_queue_push_packet(packet_p, FBE_TRANSPORT_RQ_METHOD_SAME_CORE);

    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/*****************************************************************
 * end of fbe_raid_group_journal_rebuild_completion_break_context
 *****************************************************************/

/*!****************************************************************************
 * fbe_raid_group_journal_rebuild_completion_clear_rl()
 ******************************************************************************
 * @brief
 *  This function is the completion function for rebuilding the journal.
 * 
 * @param in_packet_p   - pointer to a control packet from the scheduler
 * @param in_context    - completion context, which is a pointer to the raid 
 *                        group object
 * 
 * @return  fbe_status_t  
 *
 * @author
 *  4/24/2013 - Created. Dave Agans
 *
 ******************************************************************************/
static fbe_status_t 
fbe_raid_group_journal_rebuild_completion_clear_rl(fbe_packet_t *packet_p,
                                                   fbe_packet_completion_context_t context_p)

{
    fbe_raid_group_t                   *raid_group_p = NULL;
    fbe_payload_ex_t                   *sep_payload_p = NULL;
    fbe_payload_control_operation_t    *control_operation_p = NULL;
    fbe_atomic_t                        *clear_rl_bitmask_p = NULL;
    fbe_raid_position_bitmask_t         clear_rl_bitmask = 0;  
    //fbe_memory_request_t               *memory_request_p = NULL;

    raid_group_p = (fbe_raid_group_t*)context_p;

    /* Free up the memory that was allocated        
    */
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(sep_payload_p);
    fbe_payload_control_get_buffer(control_operation_p, &clear_rl_bitmask_p);
    clear_rl_bitmask = (fbe_raid_position_bitmask_t)*clear_rl_bitmask_p;
    fbe_transport_release_buffer((void *)clear_rl_bitmask_p);
    fbe_payload_ex_release_control_operation(sep_payload_p, control_operation_p); 

    /* We have released the buffer containing the clear_rl_bitmask.
     * It is up to fbe_raid_group_write_rl_get_dl to properly populate 
     * the iots with the correct information (clear_b etc.). 
     * Continue to clearing RL
     */
    fbe_base_object_trace((fbe_base_object_t*)raid_group_p,
                          FBE_TRACE_LEVEL_INFO, 
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "raid_group: rebuild journal complete clear rl Active bits to clear: 0x%x\n", 
                          clear_rl_bitmask);

    fbe_transport_set_completion_function(packet_p, fbe_raid_group_write_clear_rl_completion, 
                                          (fbe_packet_completion_context_t) raid_group_p);    
    fbe_raid_group_write_rl_get_dl(raid_group_p, packet_p, clear_rl_bitmask,
                                   FBE_TRUE/* Yes, this is clear rl */);                   

    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/*****************************************************************
 * end of fbe_raid_group_journal_rebuild_completion_clear_rl
 *****************************************************************/

/************************************
 * end file fbe_raid_group_clear_rl.c
 ************************************/


