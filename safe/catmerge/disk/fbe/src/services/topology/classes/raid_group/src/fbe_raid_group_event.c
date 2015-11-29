/***************************************************************************
 * Copyright (C) EMC Corporation 2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_raid_group_event.c
 ***************************************************************************
 *
 * @brief
 *  This file contains functions of the raid group event entry.
 *
 * @version
 *   6/23/2011:  Created. Rob Foley
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_raid_group.h"
#include "fbe_base_config_private.h"
#include "fbe_raid_group_object.h"
#include "fbe_raid_group_bitmap.h"    
#include "fbe/fbe_notification_interface.h"
#include "fbe_notification.h"
#include "fbe_raid_library.h"
#include "fbe_transport_memory.h"
#include "fbe_raid_group_needs_rebuild.h"
#include "fbe_raid_group_rebuild.h"
#include "fbe_raid_verify.h"


/*************************
 * LOCAL PROTOTYPES
 *************************/
static fbe_status_t fbe_raid_group_handle_peer_memory_update(fbe_raid_group_t * raid_group_p); 
static fbe_status_t fbe_raid_group_handle_peer_nonpaged_write(fbe_raid_group_t * raid_group_p); 
static fbe_status_t fbe_raid_group_main_handle_mark_nr_event(fbe_raid_group_t* in_raid_group_p,
                                                             fbe_event_context_t in_event_context);
static fbe_status_t fbe_raid_group_main_handle_remap_event(fbe_raid_group_t *in_raid_group_p,
                                                           fbe_event_context_t in_event_context);
static fbe_status_t fbe_raid_group_main_remap_event_mark_verify(fbe_event_t *in_event_p,
                                                                fbe_event_completion_context_t in_context);

static fbe_status_t fbe_raid_group_main_handle_permit_request_event(fbe_raid_group_t * raid_group_p,
                                                                   fbe_event_context_t *event_context);
static fbe_status_t fbe_raid_group_main_handle_data_request_event(fbe_raid_group_t *in_raid_group_p,
                                                                  fbe_event_context_t in_event_context);
static fbe_status_t fbe_raid_group_main_handle_zero_permit_request(fbe_raid_group_t *raid_grup_p,
                                                                   fbe_event_t * event_p);
static fbe_status_t fbe_raid_group_main_handle_verify_permit_request(fbe_raid_group_t *raid_grup_p,
                                                                     fbe_event_t * event_p);
static fbe_status_t fbe_raid_group_main_handle_verify_report_event(fbe_raid_group_t *in_raid_group_p,
                                                                   fbe_event_t *in_event_p);
static fbe_status_t fbe_raid_group_main_handle_log_event(fbe_raid_group_t *in_raid_group_p,
                                                         fbe_event_t *in_event_p);
static fbe_status_t fbe_raid_group_send_download_ack_completion(fbe_packet_t * packet_p, 
                                                                fbe_packet_completion_context_t context);
static fbe_bool_t fbe_raid_group_check_medic_action_priority(fbe_raid_group_t * raid_group_p,
                                                             fbe_event_t * event_p);
static fbe_bool_t fbe_raid_group_is_medic_action_allowable(fbe_raid_group_t * raid_group_p,
                                                           fbe_event_t * event_p);
static fbe_status_t fbe_raid_group_process_remap_request_completion(fbe_packet_t*       packet_p,
                                                                    fbe_packet_completion_context_t  context);

static fbe_status_t fbe_raid_group_main_handle_verify_invalidate_permit_request(fbe_raid_group_t * raid_group_p,
                                                                                fbe_event_t * event_p);

static fbe_status_t fbe_raid_group_event_peer_contact_lost(fbe_raid_group_t * const raid_group_p, 
                                                           fbe_event_context_t event_context);
static fbe_status_t fbe_raid_group_attribute_changed_for_slf(fbe_raid_group_t * raid_group_p, 
                                                             fbe_event_context_t event_context);
static fbe_status_t fbe_raid_group_eval_peer_slf_update(fbe_raid_group_t * raid_group_p);
static fbe_status_t fbe_raid_group_check_power_save_state(fbe_raid_group_t * raid_group_p);
static fbe_status_t fbe_raid_group_handle_download_request_event_entry(fbe_raid_group_t* in_raid_group_p,
                                                                       fbe_event_t* in_event_p);
static fbe_status_t fbe_raid_group_send_checkpoint_notification(fbe_raid_group_t * raid_group_p);
static fbe_status_t fbe_raid_group_set_change_config_cond(fbe_raid_group_t *raid_group_p);

/*!**************************************************************
 * fbe_raid_group_event_entry()
 ****************************************************************
 * @brief
 *  This function is called to pass an event to a given instance
 *  of the raid group class.
 *
 * @param object_handle - The object receiving the event.
 * @param event_type    - Type of event that is arriving.
 * @param event_context - Context that is associated with the event.
 *
 * @return
 *  fbe_status_t
 *
 * @author
 *  05/20/2009 - Created. RPF
 *
 ****************************************************************/
fbe_status_t 
fbe_raid_group_event_entry(fbe_object_handle_t object_handle, 
                           fbe_event_type_t event_type,
                           fbe_event_context_t event_context)
{
    fbe_raid_group_t * raid_group_p = NULL;
    fbe_status_t       status = FBE_STATUS_OK;

    raid_group_p = (fbe_raid_group_t *)fbe_base_handle_to_pointer(object_handle);

    fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                          FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry event_type %d context 0x%p\n", __FUNCTION__, event_type, event_context);

    /* Before handling the event at the raid group level change the disk based
     * lba range to raid lba range with new stack entry on event.
     */ 
    switch(event_type)
    {
        case FBE_EVENT_TYPE_DATA_REQUEST:   
        case FBE_EVENT_TYPE_PERMIT_REQUEST:
        case FBE_EVENT_TYPE_VERIFY_REPORT:
        case FBE_EVENT_TYPE_EVENT_LOG:
            /* allocate the new event stack with raid based lba and block count. */
            status = fbe_raid_group_allocate_event_stack_with_raid_lba(raid_group_p, event_context);
            if(status != FBE_STATUS_OK)
            {
                fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                                      FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                      "%s event stack allocation failed, status:%d, event_type:%d, context:0x%p\n",
                                      __FUNCTION__, status, event_type, event_context);
            }
            break;
    }

    //  First handle the event we have received
    switch (event_type)
    {
        case FBE_EVENT_TYPE_ATTRIBUTE_CHANGED:
            break;
        case FBE_EVENT_TYPE_EDGE_STATE_CHANGE:
            status = fbe_raid_group_state_change_event_entry(raid_group_p, event_context);
            break;
        case FBE_EVENT_TYPE_PERMIT_REQUEST:
            status = fbe_raid_group_main_handle_permit_request_event(raid_group_p, event_context);
            break;
        case FBE_EVENT_TYPE_DATA_REQUEST:
            status = fbe_raid_group_main_handle_data_request_event(raid_group_p, event_context);
            break;
        case FBE_EVENT_TYPE_VERIFY_REPORT:
            status = fbe_raid_group_main_handle_verify_report_event(raid_group_p, event_context);
            break;  
        case FBE_EVENT_TYPE_EVENT_LOG:
            status = fbe_raid_group_main_handle_log_event(raid_group_p, event_context);
            break;
        case FBE_EVENT_TYPE_PEER_NONPAGED_CHKPT_UPDATE:
            status = fbe_raid_group_send_checkpoint_notification(raid_group_p);
            break;
        case FBE_EVENT_TYPE_PEER_MEMORY_UPDATE:
            fbe_raid_group_handle_peer_memory_update(raid_group_p);
            /* We need to call both memory update handlers since both raid and base config might have work to do.
             */
            status = fbe_base_config_event_entry(object_handle, event_type, event_context);
            break;
        case FBE_EVENT_TYPE_PEER_NONPAGED_WRITE:
            status = fbe_raid_group_handle_peer_nonpaged_write(raid_group_p);
            if (status == FBE_STATUS_OK)
            {
                /*send the event to base config */
                status = fbe_base_config_event_entry(object_handle, event_type, event_context);
            }
            break;
        case FBE_EVENT_TYPE_PEER_CONTACT_LOST:
            status = fbe_raid_group_event_peer_contact_lost(raid_group_p, event_context);  
            /* 
            We need to call both event handlers since both raid and base config might have work to do.
             */
            status = fbe_base_config_event_entry(object_handle, event_type, event_context);
            break;            

        case FBE_EVENT_TYPE_DOWNLOAD_REQUEST:
            status = fbe_raid_group_handle_download_request_event_entry(raid_group_p, event_context);
            break;   

        default:
            status = fbe_base_config_event_entry(object_handle, event_type, event_context);
            break;
    }

    return status;
}
/* end fbe_raid_group_event_entry() */


/*!**************************************************************
 * fbe_raid_group_state_change_event_entry()
 ****************************************************************
 * @brief
 *  This is the handler for a change in the edge state.
 *
 * @param raid_group_p - raid group that is changing.
 * @param event_context - The context for the event.
 *
 * @return - Status of the handling.
 *
 * @author
 *  7/15/2008 - Created. RPF
 *
 ****************************************************************/

fbe_status_t 
fbe_raid_group_state_change_event_entry(fbe_raid_group_t * const raid_group_p, 
                                        fbe_event_context_t event_context)
{
    fbe_path_state_t path_state;
    fbe_status_t status;
    fbe_block_edge_t *edge_p = NULL;

    fbe_base_config_get_block_edge(&raid_group_p->base_config, &edge_p, 0);

    /*! Fetch the path state.  Note that while it returns a status, it currently
     * never fails. 
     * @todo We will need to iterate over the edges, but for now just look at the first 
     * one. 
     */ 
    status = fbe_block_transport_get_path_state(edge_p, &path_state);

    if (status != FBE_STATUS_OK)
    {
        /* Not able to get the path state.
         */
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p, 
                              FBE_TRACE_LEVEL_WARNING, 
                              FBE_TRACE_MESSAGE_ID_CREATE_OBJECT, 
                              "%s cannot get block edge path state 0x%x!!\n", __FUNCTION__, status);
        return status;
    }

    switch (path_state)
    {
        case FBE_PATH_STATE_INVALID:
        case FBE_PATH_STATE_ENABLED:
            break;
        case FBE_PATH_STATE_DISABLED:
        case FBE_PATH_STATE_BROKEN:
        case FBE_PATH_STATE_GONE:
            /* If the path state is no longer enabled (but still valid), 
             * then we will transition to activate. 
             * Activate will handle waiting for the path state to become good 
             * again. 
             */
            status = fbe_lifecycle_set_cond(&fbe_raid_group_lifecycle_const,
                                            (fbe_base_object_t*)raid_group_p,
                                            FBE_BASE_OBJECT_LIFECYCLE_COND_ACTIVATE);
            break;
        case FBE_PATH_STATE_SLUMBER:
            break;
        default:
            /* This is a path state we do not expect.
             */
            status = FBE_STATUS_GENERIC_FAILURE;
            fbe_base_object_trace((fbe_base_object_t *)raid_group_p, 
                                  FBE_TRACE_LEVEL_WARNING, 
                                  FBE_TRACE_MESSAGE_ID_CREATE_OBJECT, 
                                  "%s path state unexpected %d\n", __FUNCTION__, path_state);
            break;

    }
    return status;
}
/**********************************************
 * end fbe_raid_group_state_change_event_entry()
 ***********************************************/

/*!**************************************************************
 * fbe_raid_group_allocate_event_stack_with_raid_lba()
 ****************************************************************
 * @brief
 *  This function is used to allocate the event stack and convert
 *  the disk based lba to raid based lba.
 *
 * @param raid_group_p  - Pointer to RAID group object.
 * @param event_context - Context that is associated with event.
 *
 * @return
 *  fbe_status_t
 *
 * @author
 *  05/20/2009 - Created. RPF
 *
 ****************************************************************/
fbe_status_t
fbe_raid_group_allocate_event_stack_with_raid_lba(fbe_raid_group_t * raid_group_p,
                                                  fbe_event_context_t event_context)
{
    fbe_event_t *           event_p = NULL;
    fbe_event_stack_t *     event_stack_p = NULL;
    fbe_event_stack_t *     new_event_stack_p = NULL;
    fbe_lba_t               disk_lba;
    fbe_lba_t               raid_lba;
    fbe_event_type_t        event_type;
    fbe_block_count_t       block_count;
    fbe_block_count_t       raid_block_count;
    fbe_u16_t               data_disks;
    fbe_raid_geometry_t *   raid_geometry_p = NULL;

    /* type cast to the event */
    event_p  =(fbe_event_t*) event_context;

    /* get the event type. */
    fbe_event_get_type(event_p, &event_type);

    /* get current entry on event stack */
    event_stack_p = fbe_event_get_current_stack(event_p);

    /* get disk lba and block count of i/o range to remap */
    fbe_event_stack_get_extent(event_stack_p, &disk_lba, &block_count);

    /* get number of data disks in this RAID group */
    raid_geometry_p = fbe_raid_group_get_raid_geometry(raid_group_p);
    fbe_raid_geometry_get_data_disks(raid_geometry_p, &data_disks);

    /* determine RAID lba of i/o range to remap */
    raid_lba = disk_lba * data_disks;
    raid_block_count = block_count * data_disks;

    /* allocate the event stack with raid lba. */
    new_event_stack_p = fbe_event_allocate_stack(event_p);
    if(new_event_stack_p == NULL)
    {
        /* not enough stack entries with event object. */
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s event stack alloc failed, event_p:0x%p, event_type:%d\n",
                              __FUNCTION__, event_p, event_type);
        return FBE_STATUS_INSUFFICIENT_RESOURCES;
    }

    /* set new entry's extent info (raid lba and block count) */
    fbe_event_stack_set_extent(new_event_stack_p, raid_lba, raid_block_count);

    /* set new entry's current and previous offsets as zero. */
    fbe_event_stack_set_current_offset(new_event_stack_p, 0);
    fbe_event_stack_set_previous_offset(new_event_stack_p, 0);

    return FBE_STATUS_OK;
}
/* end fbe_raid_group_allocate_event_stack_with_raid_lba() */

/*!**************************************************************
 * fbe_raid_group_set_sig_eval_cond()
 ****************************************************************
 * @brief
 *  This function looks at the raid group clustered bits
 *  and sees if we need to set the evaluate signature condition.
 *
 * @param raid_group_p - raid group ptr.               
 *
 * @return fbe_status_t
 *
 * @author
 *  6/16/2011 - Created. Rob Foley
 *
 ****************************************************************/

static fbe_status_t fbe_raid_group_set_sig_eval_cond(fbe_raid_group_t *raid_group_p)
{
    fbe_raid_group_lock(raid_group_p);

    /* If the peer requested that we evaluate the signature, and we have not
     * acknowledged this yet by setting the request flag, then we need to also 
     * set the evaluate signature condition. 
     */
    if (fbe_raid_group_is_peer_clustered_flag_set(raid_group_p, FBE_RAID_GROUP_CLUSTERED_FLAG_EVAL_MARK_NR_REQ) &&
        !fbe_raid_group_is_peer_clustered_flag_set(raid_group_p, FBE_RAID_GROUP_CLUSTERED_FLAG_EVAL_MARK_NR_STARTED) &&
        !fbe_raid_group_is_clustered_flag_set(raid_group_p, FBE_RAID_GROUP_CLUSTERED_FLAG_EVAL_MARK_NR_REQ))
        
    {
        fbe_bool_t b_is_active;

        fbe_raid_group_monitor_check_active_state(raid_group_p, &b_is_active);
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s set condition to eval sig local: 0x%llx peer: 0x%llx state: 0x%llx\n", 
                              b_is_active ? "Active" : "Passive",
                              (unsigned long long)raid_group_p->raid_group_metadata_memory.flags,
                              (unsigned long long)raid_group_p->raid_group_metadata_memory_peer.flags,
                              (unsigned long long)raid_group_p->local_state);

        fbe_raid_group_set_local_state(raid_group_p, FBE_RAID_GROUP_LOCAL_STATE_EVAL_MARK_NR_REQUEST);
        fbe_lifecycle_set_cond(&fbe_raid_group_lifecycle_const,
                               (fbe_base_object_t *) raid_group_p,
                               FBE_RAID_GROUP_LIFECYCLE_COND_EVAL_MARK_NR);
    }
    fbe_raid_group_unlock(raid_group_p);
    
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_raid_group_set_sig_eval_cond()
 ******************************************/

/*!**************************************************************
 * fbe_raid_group_set_eval_rb_logging_cond()
 ****************************************************************
 * @brief
 *  This function looks at the raid group clustered bits
 *  and sees if we need to set the evaluate rb logging condition.
 *
 * @param raid_group_p - raid group ptr.               
 *
 * @return fbe_status_t
 *
 * @author
 *  6/27/2011 - Created. Ashwin Tamilarasan
 *
 ****************************************************************/

static fbe_status_t fbe_raid_group_set_eval_rb_logging_cond(fbe_raid_group_t *raid_group_p)
{
    fbe_raid_group_lock(raid_group_p);

    /* If the peer requested that we evaluate the nr on demand, and we have not
     * acknowledged this yet by setting the request flag, then we need to also 
     * set the evaluate nr on demand condition. 
     */
    if (fbe_raid_group_is_peer_clustered_flag_set(raid_group_p, FBE_RAID_GROUP_CLUSTERED_FLAG_EVAL_RL_REQ) &&
        !fbe_raid_group_is_peer_clustered_flag_set(raid_group_p, FBE_RAID_GROUP_CLUSTERED_FLAG_EVAL_RL_STARTED) &&
        !fbe_raid_group_is_clustered_flag_set(raid_group_p, FBE_RAID_GROUP_CLUSTERED_FLAG_EVAL_RL_REQ)&&
        !fbe_raid_group_is_local_state_set(raid_group_p, FBE_RAID_GROUP_LOCAL_STATE_EVAL_RL_REQUEST))
        
    {
        fbe_bool_t b_is_active;

        fbe_raid_group_monitor_check_active_state(raid_group_p, &b_is_active);
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s set condition to eval RL local: 0x%llx peer: 0x%llx state: 0x%llx\n", 
                              b_is_active ? "Active" : "Passive",
                              (unsigned long long)raid_group_p->raid_group_metadata_memory.flags,
                              (unsigned long long)raid_group_p->raid_group_metadata_memory_peer.flags,
                              (unsigned long long)raid_group_p->local_state);

        fbe_raid_group_set_local_state(raid_group_p, FBE_RAID_GROUP_LOCAL_STATE_EVAL_RL_REQUEST);
        fbe_lifecycle_set_cond(&fbe_raid_group_lifecycle_const,
                               (fbe_base_object_t *) raid_group_p,
                               FBE_RAID_GROUP_LIFECYCLE_COND_EVAL_RB_LOGGING);
    }
    /* If both side reached EVAL_RL_STARTED, we need to kick the object, instead of waiting for 3 seconds */
    else if (fbe_raid_group_is_peer_clustered_flag_set(raid_group_p, FBE_RAID_GROUP_CLUSTERED_FLAG_EVAL_RL_STARTED) &&
        !fbe_raid_group_is_peer_clustered_flag_set(raid_group_p, FBE_RAID_GROUP_CLUSTERED_FLAG_CLEAR_RL_REQ) &&
        fbe_raid_group_is_clustered_flag_set(raid_group_p, FBE_RAID_GROUP_CLUSTERED_FLAG_EVAL_RL_STARTED) &&
        !fbe_raid_group_is_clustered_flag_set(raid_group_p, FBE_RAID_GROUP_CLUSTERED_FLAG_CLEAR_RL_REQ))
    {
        fbe_lifecycle_reschedule(&fbe_raid_group_lifecycle_const,
                                (fbe_base_object_t *) raid_group_p,
                                (fbe_lifecycle_timer_msec_t) 0);    /* Immediate reschedule */	
    }
    fbe_raid_group_unlock(raid_group_p);
    
    return FBE_STATUS_OK;
}
/*************************************************
 * end fbe_raid_group_set_eval_rb_logging_cond()
 *************************************************/

/*!**************************************************************
 * fbe_raid_group_set_clear_rl_cond()
 ****************************************************************
 * @brief
 *  This function looks at the raid group clustered bits
 *  and sees if we need to set the clear rl condition.
 *
 * @param raid_group_p - raid group ptr.               
 *
 * @return fbe_status_t
 *
 * @author
 *  7/14/2011 - Created. Rob Foley
 *
 ****************************************************************/

static fbe_status_t fbe_raid_group_set_clear_rl_cond(fbe_raid_group_t *raid_group_p)
{
    fbe_raid_group_lock(raid_group_p);

    /* If the peer requested that we clear NR, and we have not acknowledged this yet by setting the request flag,
     * then we need to also set the clear NR condition.
     */
    if (fbe_raid_group_is_peer_clustered_flag_set(raid_group_p, FBE_RAID_GROUP_CLUSTERED_FLAG_CLEAR_RL_REQ) &&
        !fbe_raid_group_is_peer_clustered_flag_set(raid_group_p, FBE_RAID_GROUP_CLUSTERED_FLAG_CLEAR_RL_STARTED) &&
        !fbe_raid_group_is_clustered_flag_set(raid_group_p, FBE_RAID_GROUP_CLUSTERED_FLAG_CLEAR_RL_REQ))
    {
        fbe_bool_t b_is_active;

        fbe_raid_group_monitor_check_active_state(raid_group_p, &b_is_active);
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s set clear rl cond local: 0x%llx peer: 0x%llx state: 0x%llx\n", 
                              b_is_active ? "Active" : "Passive",
                              (unsigned long long)raid_group_p->raid_group_metadata_memory.flags,
                              (unsigned long long)raid_group_p->raid_group_metadata_memory_peer.flags,
                              (unsigned long long)raid_group_p->local_state);

        fbe_raid_group_set_local_state(raid_group_p, FBE_RAID_GROUP_LOCAL_STATE_CLEAR_RL_REQUEST);
        fbe_lifecycle_set_cond(&fbe_raid_group_lifecycle_const,
                               (fbe_base_object_t *) raid_group_p,
                               FBE_RAID_GROUP_LIFECYCLE_COND_CLEAR_RB_LOGGING);
    }
    fbe_raid_group_unlock(raid_group_p);
    
    return FBE_STATUS_OK;
}
/*************************************************
 * end fbe_raid_group_set_clear_rl_cond()
 *************************************************/

/*!**************************************************************
 * fbe_raid_group_peer_memory_update_swap_request_in_progress_if_needed()
 ****************************************************************
 * @brief
 *  This function looks at the raid group clustered bits
 *  and sees if we need to set the clear rl condition.
 *
 * @param raid_group_p - raid group ptr.               
 *
 * @return fbe_status_t
 *
 * @author
 *  05/01/2013  Ron Proulx  - Created.
 *
 ****************************************************************/

static fbe_status_t fbe_raid_group_peer_memory_update_swap_request_in_progress_if_needed(fbe_raid_group_t *raid_group_p)
{
    fbe_class_id_t  class_id;
    fbe_bool_t      b_is_active;

    /* Get the class id and determine if active or passive
     */
    class_id = fbe_raid_group_get_class_id(raid_group_p);
    fbe_raid_group_monitor_check_active_state(raid_group_p, &b_is_active);
  
    /* If enabled trace some information
     */
    fbe_raid_group_trace(raid_group_p,
                         FBE_TRACE_LEVEL_INFO,
                         FBE_RAID_GROUP_DEBUG_FLAGS_EVENT_TRACING,
                         "raid_group: peer update swap in-progress class: %d active: %d local: %d peer: %d \n",
                         class_id, b_is_active,
                         fbe_raid_group_is_clustered_flag_set(raid_group_p, FBE_RAID_GROUP_CLUSTERED_FLAG_SWAP_JOB_IN_PROGRESS),
                         fbe_raid_group_is_peer_clustered_flag_set(raid_group_p, FBE_RAID_GROUP_CLUSTERED_FLAG_SWAP_JOB_IN_PROGRESS));

    /* Only a passive virtual drive object should update it's local
     * clustered flags.
     */
    if ((class_id != FBE_CLASS_ID_VIRTUAL_DRIVE) ||
        (b_is_active == FBE_TRUE)                   )
    {
        /* Nothing to do if not a virtual drive or it is the active virtual drive.
         */
        return FBE_STATUS_OK;
    }

    /* We must check and clear under lock
     */
    fbe_raid_group_lock(raid_group_p);

    /* If the peer requested that we set the flag then set it.
     */
    if ( fbe_raid_group_is_peer_clustered_flag_set(raid_group_p, FBE_RAID_GROUP_CLUSTERED_FLAG_SWAP_JOB_IN_PROGRESS) &&
        !fbe_raid_group_is_clustered_flag_set(raid_group_p, FBE_RAID_GROUP_CLUSTERED_FLAG_SWAP_JOB_IN_PROGRESS)         )
    {
        /* Set the local
         */
        fbe_raid_group_set_clustered_flag(raid_group_p, FBE_RAID_GROUP_CLUSTERED_FLAG_SWAP_JOB_IN_PROGRESS);
    }
    else if (!fbe_raid_group_is_peer_clustered_flag_set(raid_group_p, FBE_RAID_GROUP_CLUSTERED_FLAG_SWAP_JOB_IN_PROGRESS) &&
              fbe_raid_group_is_clustered_flag_set(raid_group_p, FBE_RAID_GROUP_CLUSTERED_FLAG_SWAP_JOB_IN_PROGRESS)         )
    {
        /* Clear the local
         */
        fbe_raid_group_clear_clustered_flag(raid_group_p, FBE_RAID_GROUP_CLUSTERED_FLAG_SWAP_JOB_IN_PROGRESS);
    }

    fbe_raid_group_unlock(raid_group_p);
    
    return FBE_STATUS_OK;
}
/*************************************************
 * end fbe_raid_group_peer_memory_update_swap_request_in_progress_if_needed()
 *************************************************/

/*!**************************************************************
 * fbe_raid_group_handle_peer_memory_update()
 ****************************************************************
 * @brief
 *  Handle the peer updating its metadata memory.
 *
 * @param raid_group_p - raid group ptr.               
 *
 * @return fbe_status_t
 *
 * @author
 *  6/16/2011 - Created. Rob Foley
 *
 ****************************************************************/

static fbe_status_t fbe_raid_group_handle_peer_memory_update(fbe_raid_group_t * raid_group_p)
{
    /* We will simply call functions to evaluate the various raid group bits.
     */
    fbe_raid_group_check_power_save_state(raid_group_p);
    fbe_raid_group_eval_peer_slf_update(raid_group_p);
    fbe_raid_group_set_clear_rl_cond(raid_group_p);
    fbe_raid_group_set_eval_rb_logging_cond(raid_group_p);
    fbe_raid_group_set_change_config_cond(raid_group_p);
    fbe_raid_group_set_sig_eval_cond(raid_group_p);
    fbe_raid_group_rebuild_peer_memory_udpate(raid_group_p);
    fbe_raid_group_peer_memory_update_swap_request_in_progress_if_needed(raid_group_p);
    fbe_raid_group_emeh_peer_memory_update(raid_group_p);
    
    /* We might need to transition the state when the peer updates its checkpoint.
     */
    if (!fbe_base_config_is_active((fbe_base_config_t*)raid_group_p) && 
        (fbe_raid_group_encryption_is_rekey_needed(raid_group_p) ||
         (fbe_base_config_is_encrypted_mode((fbe_base_config_t*)raid_group_p) &&
          !fbe_base_config_is_encrypted_state((fbe_base_config_t*)raid_group_p)))) {
        /* On the passive side we might need to change our state. */
        fbe_raid_group_encryption_change_passive_state(raid_group_p);
    }
    return FBE_STATUS_OK;
}
/************************************************
 * end fbe_raid_group_handle_peer_memory_update()
 ************************************************/

/*!**************************************************************
 * fbe_raid_group_handle_peer_nonpaged_write()
 ****************************************************************
 * @brief
 *  Handle the peer updating its non-paged metadata.
 *
 * @param raid_group_p - raid group ptr.               
 *
 * @return fbe_status_t
 *
 * @author
 *  04/14/2013  Ron Proulx  - Created.
 *
 ****************************************************************/
static fbe_status_t fbe_raid_group_handle_peer_nonpaged_write(fbe_raid_group_t *raid_group_p)
{
    fbe_status_t    status = FBE_STATUS_OK;
    fbe_class_id_t  class_id;

    /* Currently only the virtual drive sub-class needs to act upon a non-paged
     * write from the peer.
     */
    class_id = fbe_raid_group_get_class_id(raid_group_p);
    if (class_id == FBE_CLASS_ID_VIRTUAL_DRIVE)
    {
        status = fbe_raid_group_handle_virtual_drive_peer_nonpaged_write(raid_group_p);
    }

    /* Return the execution status
     */
    return status;
}
/************************************************
 * end fbe_raid_group_handle_peer_nonpaged_write()
 ************************************************/

/*!****************************************************************************
 * fbe_raid_group_main_handle_permit_request_event()
 ******************************************************************************
 * @brief
 *  This function will handle a "permit request" event, downstream object asks
 *  permission from the upstream object before starting specific background i/o
 *  operation.
 *
 * @param in_raid_group_p       - pointer to a raid group object
 * @param in_event_context      - event context  
 *
 * @return fbe_status_t   
 *
 * @author
 *
 ******************************************************************************/
static fbe_status_t
fbe_raid_group_main_handle_permit_request_event(fbe_raid_group_t * raid_group_p,
                                               fbe_event_context_t * event_context)
{
    fbe_event_t *               event_p = NULL;
    fbe_status_t                status;
    fbe_event_permit_request_t  permit_request; 
    fbe_permit_event_type_t     permit_event_type;

    //  Trace function entry 
    FBE_RAID_GROUP_TRACE_FUNC_ENTRY(raid_group_p);

    event_p  =(fbe_event_t *) event_context;
    fbe_event_get_permit_request_data(event_p, &permit_request);
    permit_event_type = permit_request.permit_event_type;

    /* Check medic action priority first.
     */ 
    if (!fbe_raid_group_check_medic_action_priority(raid_group_p, event_p))
    {
        fbe_event_set_flags(event_p, FBE_EVENT_FLAG_DENY);
        fbe_event_set_status(event_p, FBE_EVENT_STATUS_OK);
        fbe_event_complete(event_p);
        return FBE_STATUS_OK;
    }

    /* Check if the action is allowable based on the action and the raid group
     * health.
     */
    if (!fbe_raid_group_is_medic_action_allowable(raid_group_p, event_p))
    {
        fbe_event_set_flags(event_p, FBE_EVENT_FLAG_DENY);
        fbe_event_set_status(event_p, FBE_EVENT_STATUS_OK);
        fbe_event_complete(event_p);
        return FBE_STATUS_OK;
    }

    /* If request is allowed, continue checks.
     */
    switch (permit_event_type)
    {
        case FBE_PERMIT_EVENT_TYPE_ZERO_REQUEST:
            status = fbe_raid_group_main_handle_zero_permit_request(raid_group_p, event_p);
            break;

        case FBE_PERMIT_EVENT_TYPE_VERIFY_INVALIDATE_REQUEST:
            status = fbe_raid_group_main_handle_verify_invalidate_permit_request(raid_group_p, event_p);
            break;

        case FBE_PERMIT_EVENT_TYPE_IS_CONSUMED_REQUEST:
            status = fbe_raid_group_handle_consumed_permit_request(raid_group_p, event_p);
            break;

        case FBE_PERMIT_EVENT_TYPE_VERIFY_REQUEST:
            status = fbe_raid_group_main_handle_verify_permit_request(raid_group_p, event_p);
            break;

        default:
            fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                                   FBE_TRACE_LEVEL_WARNING,
                                   FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                   "%s unsupported permit event type: %d\n", 
                                  __FUNCTION__, permit_event_type);
            fbe_event_set_status(event_p, FBE_EVENT_STATUS_INVALID_EVENT);
            fbe_event_complete(event_p);
            return FBE_STATUS_OK;
    }

    return FBE_STATUS_OK;

} // End fbe_raid_group_main_handle_permit_request_event()


/*!****************************************************************************
 * fbe_raid_group_main_handle_zero_permit_request()
 ******************************************************************************
 * @brief
 *   This function is used to handle the zero permit request event, it looks
 *   at the current stack lba range and raid group object details to decide
 *   whether it has to deny the zero permission request.
 *
 * @param in_raid_group_p       - pointer to a raid group object
 * @param in_event_context      - event context  
 *
 * @return fbe_status_t   
 *
 * @author
 *   10/04/2010 - Created. Dhaval Patel
 *
 ******************************************************************************/
static fbe_status_t
fbe_raid_group_main_handle_zero_permit_request(fbe_raid_group_t * raid_group_p,
                                               fbe_event_t * event_p)
{
    fbe_status_t status;
    fbe_event_stack_t * event_stack_p = NULL;
    fbe_lba_t           lba;                    // converted RAID lba.
    fbe_block_count_t   block_count;
    fbe_lba_t           exported_capacity;
    fbe_lifecycle_state_t  lifecycle_state;
    fbe_raid_geometry_t * raid_geometry_p = NULL;
	//fbe_base_config_encryption_mode_t encryption_mode;
	//fbe_lba_t				rekey_checkpoint;
	//fbe_base_config_encryption_state_t	encryption_state;

    FBE_RAID_GROUP_TRACE_FUNC_ENTRY(raid_group_p);

    event_stack_p = fbe_event_get_current_stack(event_p);
    fbe_event_stack_get_extent(event_stack_p, &lba, &block_count);

    raid_geometry_p = fbe_raid_group_get_raid_geometry(raid_group_p);
    /* We do not want zeroing the c4mirror raid groups.
     * The c4mirror has already been zeroed when ICA.
     */
    if (fbe_raid_geometry_is_c4_mirror(raid_geometry_p))
    {
        fbe_event_set_status(event_p, FBE_EVENT_STATUS_NO_USER_DATA);
        fbe_event_complete(event_p);
        return FBE_STATUS_OK;
    }

    status = fbe_base_object_get_lifecycle_state((fbe_base_object_t *) raid_group_p, &lifecycle_state);

    /* We do not want zeroing to continue on shutdown raid groups.
     * This is simply a policy. If the rg is broken we do not want other objects in the 
     * topology to continue. 
     */
    if ((lifecycle_state != FBE_LIFECYCLE_STATE_READY)&&(lifecycle_state != FBE_LIFECYCLE_STATE_HIBERNATE))
    {
        fbe_event_set_flags(event_p, FBE_EVENT_FLAG_DENY);
    }

    /* We do not want zeroing to continue on degraded raid groups.
     * We prefer not to generate additional I/O load (by zero) during degraded mode.
     */
    if (fbe_raid_group_is_degraded(raid_group_p))
    {
        fbe_event_set_flags(event_p, FBE_EVENT_FLAG_DENY);
    }

#if 0
	/* No zeroing if RG in rekey situation 
	   Check if raid is in encryption related mode 
	 */
    fbe_base_config_get_encryption_mode((fbe_base_config_t *)raid_group_p, &encryption_mode);
	if(encryption_mode > FBE_BASE_CONFIG_ENCRYPTION_MODE_UNENCRYPTED){ /* Some CBE related state */
		rekey_checkpoint = fbe_raid_group_get_rekey_checkpoint(raid_group_p);
		fbe_base_config_get_encryption_state((fbe_base_config_t *)raid_group_p, &encryption_state);
		/* The question here is which state is final FBE_BASE_CONFIG_ENCRYPTION_STATE_ENCRYPTED or FBE_BASE_CONFIG_ENCRYPTION_STATE_KEYS_PROVIDED */
		if(rekey_checkpoint != FBE_LBA_INVALID || 
			(encryption_state != FBE_BASE_CONFIG_ENCRYPTION_STATE_KEYS_PROVIDED && encryption_state != FBE_BASE_CONFIG_ENCRYPTION_STATE_ENCRYPTED))
		{ /* Rekey is somewhat in progress */
			fbe_event_set_flags(event_p, FBE_EVENT_FLAG_DENY);
	        fbe_event_set_status(event_p, FBE_EVENT_STATUS_OK);
		    fbe_event_complete(event_p);
			return FBE_STATUS_OK;
		}
	}
#endif


    /* look at the event extent (lba range) before completion of the event if
     * extent belongs to exported lba range then send the event to upstream
     * edge to process at the next object level. 
     */ 
    fbe_base_config_get_capacity((fbe_base_config_t *)raid_group_p, &exported_capacity);
    if (lba >= exported_capacity)
    {
        /* raid group object has no objection with zeroing operation for the
         * private data range and so complete the event.
         */
        fbe_event_set_status(event_p, FBE_EVENT_STATUS_OK);
        fbe_event_complete(event_p);
        return FBE_STATUS_OK;
    }

    /* send the event to upstream edge to process the event. */
    fbe_base_config_send_event((fbe_base_config_t *) raid_group_p, FBE_EVENT_TYPE_PERMIT_REQUEST, event_p);

    //  Return success.
    return FBE_STATUS_OK;

} // End fbe_raid_group_main_handle_zero_permit_request()

/*!****************************************************************************
 * fbe_raid_group_main_handle_verify_invalidate_permit_request()
 ******************************************************************************
 * @brief
 *   This function is used to handle the verify invalidate permit request event, it looks
 *   at the current stack lba range and raid group object details to decide
 *   whether it has to deny the permission request. It also gets the next consumed
 *   LBA and returns to the caller the unconsumed block count so that they can decide
 *   what needs to be done
 *
 * @param in_raid_group_p       - pointer to a raid group object
 * @param in_event_context      - event context  
 *
 * @return fbe_status_t   
 *
 * @author
 *   04/09/2012 - Created. Ashok Tamilarasan
 *
 ******************************************************************************/
static fbe_status_t
fbe_raid_group_main_handle_verify_invalidate_permit_request(fbe_raid_group_t * raid_group_p,
                                                            fbe_event_t * event_p)
{
    fbe_event_stack_t * event_stack_p = NULL;
    fbe_lba_t           lba;                    // converted RAID lba.
    fbe_block_count_t   block_count;
    fbe_lba_t           exported_capacity;
    fbe_lba_t                       next_consumed_lba;
    
    FBE_RAID_GROUP_TRACE_FUNC_ENTRY(raid_group_p);

    event_stack_p = fbe_event_get_current_stack(event_p);
    fbe_event_stack_get_extent(event_stack_p, &lba, &block_count);

    /* look at the event extent (lba range) before completion of the event if
     * extent belongs to exported lba range then send the event to upstream
     * edge to process at the next object level. 
     */ 
    fbe_base_config_get_capacity((fbe_base_config_t *)raid_group_p, &exported_capacity);
    if (lba >= exported_capacity)
    {
        /* raid group object has no objection with zeroing operation for the
         * private data range and so complete the event.
         */
        fbe_event_set_status(event_p, FBE_EVENT_STATUS_OK);
        fbe_event_complete(event_p);
        return FBE_STATUS_OK;
    }

     fbe_block_transport_server_find_next_consumed_lba(&((fbe_base_config_t *)raid_group_p)->block_transport_server,
                                                       lba,
                                                       &next_consumed_lba);

    //  If the next consumed LBA is not invalid, and it is bigger than the starting LBA that we are 
    //  getting from the Event context, then calculate the remaining blocks to be done and set it
    //  in the permit request data structure.  Then, complete the Event with the No User Data 
    //  status. There is no need to send to the upstream.
    if ((next_consumed_lba != FBE_LBA_INVALID) && (next_consumed_lba > lba))
    {                        
        //  Update the block count and stores in the permit request info data structure.
        fbe_block_count_t new_block_count = (fbe_block_count_t) (next_consumed_lba - lba);             
        event_p->event_data.permit_request.unconsumed_block_count = new_block_count;

        //  Complete the Event with the No User Data status
        fbe_event_set_status(event_p, FBE_EVENT_STATUS_NO_USER_DATA);
        fbe_event_complete(event_p);

        return FBE_STATUS_OK;
    }
    else if (next_consumed_lba == FBE_LBA_INVALID)
    {
        event_p->event_data.permit_request.unconsumed_block_count = FBE_LBA_INVALID;
        fbe_event_set_status(event_p, FBE_EVENT_STATUS_NO_USER_DATA);
        fbe_event_complete(event_p);

        return FBE_STATUS_OK;
    }

    /* send the event to upstream edge to process the event. */
    fbe_base_config_send_event((fbe_base_config_t *) raid_group_p, FBE_EVENT_TYPE_PERMIT_REQUEST, event_p);

    //  Return success.
    return FBE_STATUS_OK;

} // End fbe_raid_group_main_handle_verify_invalidate_permit_request()

/*!****************************************************************************
 * fbe_raid_group_handle_consumed_permit_request()
 ******************************************************************************
 * @brief
 *   This function is used to handle an "is_consumed" permit request event.  It
 *   finds the next consumed lba and calculates the block count.  The block count
 *   then stores in the permit request info data structure.  In addition, it
 *   checks to see if the LBA in the event is outside of the RAID group's exported
 *   capacity (ie. in the paged metadata or signature area), the function will
 *   complete the event.  If the LBA is within the RG's exported capacity, it will
 *   send the event to the upstream object to be processed.
 *
 * @param in_striper_p       - pointer to a striper object
 * @param in_event_p         - pointer to an event 
 *
 * @return fbe_status_t   
 *
 * @author
 *   5/18/2011 - KMai
 *
 ******************************************************************************/
fbe_status_t 
fbe_raid_group_handle_consumed_permit_request(fbe_raid_group_t *raid_group_p, fbe_event_t* in_event_p)

{
    fbe_event_stack_t*               event_stack_p = NULL;
    fbe_lba_t                        start_lba;
    fbe_lba_t                        end_lba; 
    fbe_lba_t                        new_end_lba;
    fbe_block_count_t                block_count;
    fbe_lba_t                        exported_disk_capacity;
    fbe_lba_t                        next_consumed_lba;
    fbe_event_permit_request_t       permit_request_info;
    fbe_u16_t                        data_disks;
    fbe_raid_geometry_t*             raid_geometry_p = NULL;
    fbe_block_count_t                new_block_count;
    fbe_lba_t                        metadata_start_lba;
    fbe_lba_t                        metadata_capacity;
    fbe_lba_t                        metadata_end_lba;
    fbe_lifecycle_state_t            lifecycle_state;
    fbe_status_t                     status;
    fbe_scheduler_hook_status_t      hook_status;
    fbe_bool_t                       b_refresh;

    status = fbe_base_object_get_lifecycle_state((fbe_base_object_t *) raid_group_p, &lifecycle_state);

    if(lifecycle_state != FBE_LIFECYCLE_STATE_READY && lifecycle_state != FBE_LIFECYCLE_STATE_HIBERNATE)
    {
        /* Do not allow any downstream ops if we are not ready.
         */
        fbe_event_set_flags(in_event_p, FBE_EVENT_FLAG_DENY);
        fbe_event_set_status(in_event_p, FBE_STATUS_OK);
        fbe_event_complete(in_event_p);
        return FBE_STATUS_OK;
    }

    //  Get the start lba and block count out of the event 
    event_stack_p = fbe_event_get_current_stack(in_event_p);
    fbe_event_stack_get_extent(event_stack_p, &start_lba, &block_count);
    end_lba = start_lba + block_count - 1;
    new_block_count = block_count;

    //  Get the permit request data from the event
    fbe_event_get_permit_request_data(in_event_p, &permit_request_info);  

    //  Get the data disks
    raid_geometry_p = fbe_raid_group_get_raid_geometry(raid_group_p);
    fbe_raid_geometry_get_data_disks(raid_geometry_p, &data_disks);
    fbe_raid_geometry_get_metadata_start_lba(raid_geometry_p, &metadata_start_lba);
    fbe_raid_geometry_get_metadata_capacity(raid_geometry_p, &metadata_capacity);
    metadata_end_lba = metadata_start_lba + metadata_capacity - 1;

    //  Get the external capacity of the RG, which is the area available for data and does not include the paged
    //  metadata or signature area.  This is stored in the base config's capacity.
    fbe_base_config_get_capacity((fbe_base_config_t*) raid_group_p, &exported_disk_capacity);

    /* We should not allow any operations if we need to refresh our block size. 
     * We will wait for that refresh to take place before we  
     */
    fbe_raid_group_lock(raid_group_p);
    fbe_raid_geometry_should_refresh_block_sizes(raid_geometry_p, &b_refresh, NULL);
    if (b_refresh) {
        fbe_raid_group_unlock(raid_group_p);
        fbe_raid_group_check_hook(raid_group_p,  SCHEDULER_MONITOR_STATE_RAID_GROUP_EVENT, 
                                  FBE_RAID_GROUP_SUBSTATE_EVENT_COPY_BL_REFRESH_NEEDED, 0, &hook_status);
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "raid_group: permit request event denied since bl/s refresh needed.\n"); 
        fbe_event_set_flags(in_event_p, FBE_EVENT_FLAG_DENY);
        fbe_event_set_status(in_event_p, FBE_EVENT_STATUS_OK);
        fbe_event_complete(in_event_p);
        return FBE_STATUS_OK;
    }
    fbe_raid_group_unlock(raid_group_p);

    /* Trace some information if enabled.
     */
    fbe_raid_group_trace((fbe_raid_group_t *)raid_group_p,
                         FBE_TRACE_LEVEL_INFO,
                         FBE_RAID_GROUP_DEBUG_FLAGS_EVENT_TRACING,
                         "rg permit: event stack start: 0x%llx blocks: 0x%llx exported: 0x%llx \n",
                         (unsigned long long)start_lba, (unsigned long long)block_count, (unsigned long long)exported_disk_capacity);

    if (start_lba > metadata_end_lba)
    {
        /* If we are a parity, then we should allow for journal area too.
         */
        if (fbe_raid_geometry_is_parity_type(raid_geometry_p))
        {
            fbe_lba_t journal_start_lba;
            fbe_lba_t journal_end_lba;

            fbe_raid_geometry_get_journal_log_start_lba(raid_geometry_p, &journal_start_lba);

            journal_start_lba *= data_disks;
            journal_end_lba = journal_start_lba + (FBE_RAID_GROUP_WRITE_LOG_SIZE * data_disks) - 1;

            /* If our start address is in the journal area, return consumed.
             */
            if ((start_lba >= journal_start_lba) &&
                (start_lba <= journal_end_lba))
            {
                fbe_raid_group_trace((fbe_raid_group_t *)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_RAID_GROUP_DEBUG_FLAGS_EVENT_TRACING,
                                     "rg permit: start in journal lba: 0x%llx blocks: 0x%llx j_start: 0x%llx\n",
                                     (unsigned long long)start_lba, (unsigned long long)block_count, (unsigned long long)journal_start_lba);

                /* If we go past the journal area limit to the journal end lba.
                 */
                if (end_lba > journal_end_lba)
                {
                    permit_request_info.unconsumed_block_count = (end_lba - journal_end_lba) / data_disks;
                    fbe_event_set_permit_request_data(in_event_p, &permit_request_info);
                }
                fbe_event_set_status(in_event_p, FBE_EVENT_STATUS_OK);
                fbe_event_complete(in_event_p);
                return FBE_STATUS_OK;
            }
        }
        fbe_raid_group_trace((fbe_raid_group_t *)raid_group_p,
                             FBE_TRACE_LEVEL_INFO,
                             FBE_RAID_GROUP_DEBUG_FLAGS_EVENT_TRACING,
                             "rg permit: start beyond md end lba: 0x%llx blocks: 0x%llx md_end: 0x%llx\n",
                             (unsigned long long)start_lba, (unsigned long long)block_count, (unsigned long long)metadata_end_lba);

        fbe_event_set_status(in_event_p, FBE_EVENT_STATUS_NO_USER_DATA);
        fbe_event_complete(in_event_p);
        return FBE_STATUS_OK;
    }
    //  If the LBA range of the event is greater than the exported capacity, then the event does not 
    //  need to be send upstream, because we know that it is not seen by an upstream object.  Complete 
    //  the event with status "okay" because it is consumed by this object. 
    else if (start_lba >= exported_disk_capacity)
    {
        if (end_lba > metadata_end_lba)
        {
            /* If we went beyond the end of the imported capacity, then set unconsumed blocks to the blocks remaining.
             */
            permit_request_info.unconsumed_block_count = (end_lba - metadata_end_lba) / data_disks;
            fbe_event_set_permit_request_data(in_event_p, &permit_request_info);
        }
        fbe_raid_group_trace((fbe_raid_group_t *)raid_group_p,
                             FBE_TRACE_LEVEL_INFO, FBE_RAID_GROUP_DEBUG_FLAGS_EVENT_TRACING,
                             "rg permit: start beyond exported start lba: 0x%llx blocks: 0x%llx exp_cap: 0x%llx uncons:0x%llx\n",
                             (unsigned long long)start_lba, (unsigned long long)block_count, (unsigned long long)exported_disk_capacity, (unsigned long long)permit_request_info.unconsumed_block_count);

        fbe_event_set_status(in_event_p, FBE_EVENT_STATUS_OK);
        fbe_event_complete(in_event_p);
        return FBE_STATUS_OK;
    }

    //  Find the next consumed lba
    fbe_block_transport_server_find_next_consumed_lba(&raid_group_p->base_config.block_transport_server,
                                                      start_lba,
                                                      &next_consumed_lba);

    //  If the next consumed LBA is not invalid, and it is bigger than the starting LBA that we are 
    //  getting from the Event context, then calculate the remaining blocks to be done and set it
    //  in the permit request data structure.  Then, complete the Event with the No User Data 
    //  status. There is no need to send to the upstream.
    if ((next_consumed_lba != FBE_LBA_INVALID) && (next_consumed_lba > start_lba))
    {
        fbe_raid_group_trace((fbe_raid_group_t *)raid_group_p,
                             FBE_TRACE_LEVEL_INFO,
                             FBE_RAID_GROUP_DEBUG_FLAGS_EVENT_TRACING,
                             "rg permit: start not consumed lba: 0x%llx blocks: 0x%llx nxt_lba: 0x%llx \n",
                             (unsigned long long)start_lba, (unsigned long long)block_count, (unsigned long long)next_consumed_lba);
        new_block_count =  (fbe_block_count_t) (next_consumed_lba - start_lba);             
        permit_request_info.unconsumed_block_count = new_block_count/data_disks;
        fbe_event_set_permit_request_data(in_event_p, &permit_request_info);

        fbe_event_set_status(in_event_p, FBE_EVENT_STATUS_NO_USER_DATA);
        fbe_event_complete(in_event_p);

        return FBE_STATUS_OK;
    }

    /* Nothing consumed in request.
     */
    if (next_consumed_lba == FBE_LBA_INVALID)     
    {
        fbe_raid_group_trace((fbe_raid_group_t *)raid_group_p,
                             FBE_TRACE_LEVEL_INFO, FBE_RAID_GROUP_DEBUG_FLAGS_EVENT_TRACING,
                             "rg permit: not consumed lba: 0x%llx blocks: 0x%llx\n",
                             (unsigned long long)start_lba, (unsigned long long)block_count);
        //  if next consumed lba is invalid then there is nothing rebuild in exported capacity region and 
        //  so increment the checkpoint to exported capacity
        next_consumed_lba = exported_disk_capacity;

        new_block_count =  (fbe_block_count_t) (next_consumed_lba - start_lba);
        permit_request_info.unconsumed_block_count = new_block_count/data_disks;        
        fbe_event_set_permit_request_data(in_event_p, &permit_request_info);

        fbe_event_set_status(in_event_p, FBE_EVENT_STATUS_NO_USER_DATA);
        fbe_event_complete(in_event_p);

        return FBE_STATUS_OK;

    }    

    fbe_block_transport_server_get_end_of_extent(&raid_group_p->base_config.block_transport_server,
                                                 start_lba, block_count, &new_end_lba);
    if (new_end_lba != start_lba + block_count - 1)
    {
        /* The extent ends before end_lba.
         * Report that we are consumed, but also report the extent size that is unconsumed.
         */
        new_block_count =  (fbe_block_count_t) (new_end_lba + 1 - start_lba); 
        permit_request_info.unconsumed_block_count = (block_count - new_block_count) / data_disks;
        fbe_raid_group_trace((fbe_raid_group_t *)raid_group_p,
                             FBE_TRACE_LEVEL_INFO, FBE_RAID_GROUP_DEBUG_FLAGS_EVENT_TRACING,
                             "rg permit: edge boundary lba: 0x%llx blocks: 0x%llx new_bl: 0x%llx uncons: 0x%llx\n",
                             (unsigned long long)start_lba, (unsigned long long)block_count, (unsigned long long)new_block_count, (unsigned long long)permit_request_info.unconsumed_block_count);
        fbe_event_set_permit_request_data(in_event_p, &permit_request_info);
        fbe_event_stack_set_extent(event_stack_p, start_lba, new_block_count);
        fbe_base_config_send_event((fbe_base_config_t*) raid_group_p, FBE_EVENT_TYPE_PERMIT_REQUEST, in_event_p);
        return FBE_STATUS_OK;
    }

    /* If we are here the starting lba is consumed but the ending lba may not 
     * be consumed.  Set the `unconsumed' blocks of the request accordingly.
     */
    fbe_block_transport_server_find_next_consumed_lba(&raid_group_p->base_config.block_transport_server,
                                                      end_lba,
                                                      &next_consumed_lba);
    if (next_consumed_lba == FBE_LBA_INVALID)
    {
        fbe_block_transport_server_find_last_consumed_lba(&raid_group_p->base_config.block_transport_server,
                                                          &next_consumed_lba);

        /* If next_consumed_lba is greater or equal something is wrong.
         */
        if (next_consumed_lba >= end_lba)
        {
            fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                                  FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                  "raid_group: permit request - last consumed: 0x%llx is greater than end: 0x%llx \n",
                                  (unsigned long long)next_consumed_lba, (unsigned long long)end_lba);
            fbe_event_set_status(in_event_p, FBE_EVENT_STATUS_GENERIC_FAILURE);
            fbe_event_complete(in_event_p);
            return FBE_STATUS_OK;
        }

        /* Set the `unconsumed' value to those downstream blocks that are not 
         * consumed.  We still forward the request upstream to determine if it
         * consumed or not.
         */
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "raid_group: permit request - last consumed: 0x%llx iend: 0x%llx data_disks: %d\n",
                              (unsigned long long)next_consumed_lba, (unsigned long long)end_lba, data_disks);
        new_block_count = end_lba - next_consumed_lba;
        permit_request_info.unconsumed_block_count = new_block_count / data_disks;        
        fbe_event_set_permit_request_data(in_event_p, &permit_request_info);
    }

    fbe_raid_group_trace((fbe_raid_group_t *)raid_group_p,
                         FBE_TRACE_LEVEL_INFO,
                         FBE_RAID_GROUP_DEBUG_FLAGS_EVENT_TRACING,
                         "raid_group: permit event type: %d status: 0x%x obj: 0x%x is_start: %d is_end: %d unconsumed: 0x%llx \n",
                         permit_request_info.permit_event_type, /*! @todo Status is always OK*/ FBE_EVENT_STATUS_OK,
                         permit_request_info.object_id, permit_request_info.is_start_b,
                         permit_request_info.is_end_b, permit_request_info.unconsumed_block_count);

    fbe_base_config_send_event((fbe_base_config_t*) raid_group_p, FBE_EVENT_TYPE_PERMIT_REQUEST, in_event_p);

    return FBE_STATUS_OK;
} // End fbe_raid_group_handle_consumed_permit_request()

/*!****************************************************************************
 * fbe_raid_group_main_handle_verify_report_event()
 ******************************************************************************
 * @brief
 *   This function is used to handle an verify report event for consumed area.  If
 *   the LBA in the event is outside of the RAID group's exported capacity (ie.
 *   in the paged metadata or signature area), the function will complete the 
 *   event.  If the LBA is within the RG's exported capacity, it will send the  
 *   event to the upstream object to be processed.
 *
 * @param in_raid_group_p       - pointer to a raid group object
 * @param in_event_p            - pointer to an event 
 *
 * @return fbe_status_t   
 *
 * @author
 *   1/28/2011 - Created. Ashwin Tamilarasan.
 *
 ******************************************************************************/
static fbe_status_t fbe_raid_group_main_handle_verify_report_event(
                                                fbe_raid_group_t*       in_raid_group_p,
                                                fbe_event_t*            in_event_p)

{
    fbe_event_stack_t*                          event_stack_p;          //  event stack pointer
    fbe_lba_t                                   start_lba;              //  start LBA (a RAID-based LBA)
    fbe_block_count_t                           block_count;            //  block count (unused) 
    fbe_lba_t                                   exported_capacity;      //  capacity of the RG available for data
    fbe_raid_geometry_t*                        raid_geometry_p = NULL;
    fbe_u16_t                                   data_disks;
    fbe_raid_group_type_t                       raid_type;
    fbe_event_verify_report_t                   verify_event_data;
                                                                               

    raid_geometry_p = fbe_raid_group_get_raid_geometry(in_raid_group_p); 
    fbe_raid_geometry_get_raid_type(raid_geometry_p, &raid_type);
    fbe_raid_geometry_get_data_disks(raid_geometry_p, &data_disks);
    if(raid_type == FBE_RAID_GROUP_TYPE_RAID10)
    {
        fbe_event_get_verify_report_data(in_event_p, &verify_event_data);
        verify_event_data.data_disks = data_disks;
        fbe_event_set_verify_report_data(in_event_p, &verify_event_data);

    }
    //  Get the start lba out of the event 
    event_stack_p = fbe_event_get_current_stack(in_event_p);
    fbe_event_stack_get_extent(event_stack_p, &start_lba, &block_count);

    //  Get the external capacity of the RG, which is the area available for data and does not include the paged
    //  metadata or signature area.  This is stored in the base config's capacity.
    fbe_base_config_get_capacity((fbe_base_config_t*) in_raid_group_p, &exported_capacity);

    //  If the LBA range of the event is greater than the exported capacity, then the event does not need to be
    //  send upstream, because we know that it is not seen by an upstream object.  Complete the event with status
    //  "okay" because it is consumed by this object. 
    if (start_lba >= exported_capacity)
    {
        fbe_event_set_status(in_event_p, FBE_STATUS_OK);
        fbe_event_complete(in_event_p);
        return FBE_STATUS_OK;
    }

    //  Send the event to the upstream object for it to process it further 
    fbe_base_config_send_event((fbe_base_config_t*) in_raid_group_p, FBE_EVENT_TYPE_VERIFY_REPORT, in_event_p);

    //  Return success.
    return FBE_STATUS_OK;

} // End fbe_raid_group_main_handle_verify_report_event()

/*!****************************************************************************
 * fbe_raid_group_main_handle_log_event()
 ******************************************************************************
 * @brief
 *   This function is used to handle a log event recieved from a downstream object.
 *   Determine if te request is beyond the exported raid group capacity or not.
 *   If not forward the requet upstream (i.e. send to lun object).
 *
 * @param in_raid_group_p       - pointer to a raid group object
 * @param in_event_p            - pointer to an event 
 *
 * @return fbe_status_t   
 *
 * @author
 *   1/28/2011 - Created. Ashwin Tamilarasan.
 *
 ******************************************************************************/
static fbe_status_t fbe_raid_group_main_handle_log_event(
                                                fbe_raid_group_t*       in_raid_group_p,
                                                fbe_event_t*            in_event_p)

{
    fbe_event_stack_t*                          event_stack_p;          //  event stack pointer
    fbe_lba_t                                   start_lba;              //  start LBA (a RAID-based LBA)
    fbe_block_count_t                           block_count;            //  block count (unused) 
    fbe_lba_t                                   exported_capacity;      //  capacity of the RG available for data
    fbe_raid_geometry_t*                        raid_geometry_p = NULL;
    fbe_raid_group_type_t                       raid_type;

    // Currently nothing special for RAID-10.
    raid_geometry_p = fbe_raid_group_get_raid_geometry(in_raid_group_p); 
    fbe_raid_geometry_get_raid_type(raid_geometry_p, &raid_type);                                                                          
    if (raid_type == FBE_RAID_GROUP_TYPE_RAID10)
    {
        // Nothing special to do (send upstream if applicable)
    }

    //  Get the start lba out of the event 
    event_stack_p = fbe_event_get_current_stack(in_event_p);
    fbe_event_stack_get_extent(event_stack_p, &start_lba, &block_count);

    //  Get the external capacity of the RG, which is the area available for data and does not include the paged
    //  metadata or signature area.  This is stored in the base config's capacity.
    fbe_base_config_get_capacity((fbe_base_config_t*) in_raid_group_p, &exported_capacity);

    //  If the LBA range of the event is greater than the exported capacity, then the event does not need to be
    //  send upstream, because we know that it is not seen by an upstream object.  Complete the event with status
    //  "okay" because it is consumed by this object. 
    if (start_lba >= exported_capacity)
    {
        fbe_event_set_status(in_event_p, FBE_STATUS_OK);
        fbe_event_complete(in_event_p);
        return FBE_STATUS_OK;
    }

    //  Send the event to the upstream object for it to process it further 
    fbe_base_config_send_event((fbe_base_config_t*) in_raid_group_p, FBE_EVENT_TYPE_EVENT_LOG, in_event_p);

    //  Return success.
    return FBE_STATUS_OK;

} // End fbe_raid_group_main_handle_log_event()

/*!****************************************************************************
 * fbe_raid_group_main_handle_verify_permit_request()
 ******************************************************************************
 * @brief
 *   This function is used to handle the verify permit request event. It looks
 *   at the current stack lba range and raid group object details to decide
 *   whether it has to deny the verify permission request.
 *
 * @param in_raid_group_p       - pointer to a raid group object
 * @param in_event_context      - event context  
 *
 * @return fbe_status_t   
 *
 * @author
 *   11/18/2010 - Created. Maya Dagon
 *
 ******************************************************************************/
static fbe_status_t
fbe_raid_group_main_handle_verify_permit_request(fbe_raid_group_t * raid_group_p,
                                               fbe_event_t * event_p)
{
    fbe_event_stack_t *    event_stack_p = NULL;
    fbe_lba_t              lba;                    // converted RAID lba.
    fbe_block_count_t      block_count;
    fbe_lifecycle_state_t  lifecycle_state;
    fbe_status_t           status;
    fbe_lba_t              exported_capacity;

    FBE_RAID_GROUP_TRACE_FUNC_ENTRY(raid_group_p);

    event_stack_p = fbe_event_get_current_stack(event_p);
    fbe_event_stack_get_extent(event_stack_p, &lba, &block_count);

    /*!@todo verify request will be denied if didn't finish handling the previous remap request
     */

    if(fbe_raid_group_is_degraded(raid_group_p)){
        fbe_event_set_flags(event_p, FBE_EVENT_FLAG_DENY);
    }

    status = fbe_base_object_get_lifecycle_state((fbe_base_object_t *) raid_group_p, &lifecycle_state);

    if(lifecycle_state != FBE_LIFECYCLE_STATE_READY){
        fbe_event_set_flags(event_p, FBE_EVENT_FLAG_DENY);
    }

    /* look at the event extent (lba range) before completion of the event if
     * extent belongs to exported lba range then send the event to upstream
     * edge to process at the next object level. 
     */ 
    fbe_base_config_get_capacity((fbe_base_config_t *)raid_group_p, &exported_capacity);
    if (lba >= exported_capacity)
    {
        /* raid group object has no objection with sniff operation for the
         * private data range and so complete the event.
         */
        fbe_event_set_status(event_p, FBE_EVENT_STATUS_OK);
        fbe_event_complete(event_p);
        return FBE_STATUS_OK;
    }

    /* send the event to upstream edge to process the event. */
    fbe_base_config_send_event((fbe_base_config_t *) raid_group_p, FBE_EVENT_TYPE_PERMIT_REQUEST, event_p);
    return FBE_STATUS_OK;
   

} //End fbe_raid_group_main_handle_verify_permit_request()

/*!****************************************************************************
 * fbe_raid_group_main_handle_data_request_event()
 ******************************************************************************
 * @brief
 *   This function will handle a "data request" event. This function
 *   determines the type of data event and initiates action accordingly.
 *
 * @param in_raid_group_p       - pointer to a raid group object
 * @param in_event_context      - event context  
 *
 * @return fbe_status_t   
 *
 * @author
 *   11/11/2009 - Created. Jean Montes.
 *   03/31/2010 - Modified Ashwin
 *
 ******************************************************************************/
static fbe_status_t fbe_raid_group_main_handle_data_request_event(fbe_raid_group_t * in_raid_group_p,
                                                                  fbe_event_context_t in_event_context)
{

    fbe_event_t*                event_p;
    fbe_event_data_request_t    data_request; 
    fbe_data_event_type_t       data_event_type;

    //  Trace function entry 
    FBE_RAID_GROUP_TRACE_FUNC_ENTRY(in_raid_group_p);

    event_p  =(fbe_event_t*) in_event_context;
    fbe_event_get_data_request_data(event_p, &data_request);
    data_event_type = data_request.data_event_type;

    switch (data_event_type)
    {
        case FBE_DATA_EVENT_TYPE_MARK_NEEDS_REBUILD:
        case FBE_DATA_EVENT_TYPE_MARK_VERIFY:
        case FBE_DATA_EVENT_TYPE_MARK_IW_VERIFY:
            fbe_raid_group_main_handle_mark_nr_event(in_raid_group_p, in_event_context);
            break;
        case FBE_DATA_EVENT_TYPE_REMAP:
            fbe_raid_group_main_handle_remap_event(in_raid_group_p, in_event_context);
            break;
        default:
            fbe_base_object_trace((fbe_base_object_t*)in_raid_group_p,
                                   FBE_TRACE_LEVEL_WARNING,
                                   FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                   "%s unsupported data event type\n", __FUNCTION__);
            fbe_event_set_status(event_p, FBE_EVENT_STATUS_INVALID_EVENT);
            fbe_event_complete(event_p);
    }

    return FBE_STATUS_OK;

} // End fbe_raid_group_main_handle_data_request_event()


/*!****************************************************************************
 * fbe_raid_group_main_handle_mark_nr_event()
 ******************************************************************************
 * @brief
 *   This function will handle a "data request" event.  This event is sent by
 *   the VD to indicate that NR should be marked on a disk.  The function will 
 *   set a condition to mark NR, after storing the event information so that it 
 *   can be accessed by the condition function. 
 *
 * @param in_raid_group_p       - pointer to a raid group object
 * @param in_event_context      - event context  
 *
 * @return fbe_status_t   
 *
 * @author
 *   11/11/2009 - Created. Jean Montes.
 *
 ******************************************************************************/
static fbe_status_t fbe_raid_group_main_handle_mark_nr_event(
                                fbe_raid_group_t*       in_raid_group_p,
                                fbe_event_context_t     in_event_context)
{
    fbe_block_count_t               block_count;        // block count of i/o range to remap
    fbe_event_t*                    event_p;            // pointer to event
    fbe_event_stack_t*              event_stack_p;      // event stack pointer
    fbe_lba_t                       external_capacity;  // RAID group's capacity
    fbe_lba_t                       raid_lba;           // RAID lba of i/o range to remap

    /* Trace function entry */
    FBE_RAID_GROUP_TRACE_FUNC_ENTRY( in_raid_group_p );

    /* Cast event context to event object */
    event_p = (fbe_event_t *)in_event_context;

    /* Trace function entry */
    FBE_RAID_GROUP_TRACE_FUNC_ENTRY(in_raid_group_p);

    /* Cast context to event object */
    event_p = (fbe_event_t *)in_event_context;

    /* Get current entry on event stack */
    event_stack_p = fbe_event_get_current_stack(event_p);

    /* Get disk lba and block count of i/o range to remap */
    fbe_event_stack_get_extent(event_stack_p, &raid_lba, &block_count);

    /* Get the exported capacity of the raid group object. */
    fbe_base_config_get_capacity((fbe_base_config_t *)in_raid_group_p, &external_capacity);

    /*!@todo 
     * Before queueing event we should define whether data event range maps to user
     * data range or metadata region. 
     * If it is in signature or write log region then how it will be handled? 
     */
    /* Just queue up the event which will be processed by the background monitor case */
    fbe_base_config_event_queue_push_event((fbe_base_config_t *) in_raid_group_p, event_p);

    /* Mark the specific region for the needs rebuild. */
    return FBE_STATUS_OK;

} // End fbe_raid_group_main_handle_mark_nr_event()


/*!***************************************************************
 * fbe_raid_group_main_handle_remap_event
 *****************************************************************
 * @brief
 *    This function handles processing remap events. This event is sent
 *    by the provision drive to determine whether the specified lba  is
 *    part of bound LUN.
 *
 * @param   in_raid_group_p   -  pointer to RAID group
 * @param   in_event_context  -  context associated with event
 *
 * @return  fbe_status_t      -  remap event processing status
 *
 * @author
 *    07/09/2010 - Created. Randy Black
 *
 ****************************************************************/

fbe_status_t
fbe_raid_group_main_handle_remap_event( fbe_raid_group_t*   in_raid_group_p,
                                        fbe_event_context_t in_event_context )
{
    fbe_block_count_t               block_count;        // block count of i/o range to remap
    fbe_event_t*                    event_p;            // pointer to event
    fbe_event_stack_t*              event_stack_p;      // event stack pointer
    fbe_lba_t                       external_capacity;  // RAID group's capacity
    fbe_lba_t                       raid_lba;           // RAID lba of i/o range to remap
    fbe_lba_t                       configured_capacity;

    // trace function entry
    FBE_RAID_GROUP_TRACE_FUNC_ENTRY( in_raid_group_p );

    // cast context to event object
    event_p = (fbe_event_t *)in_event_context;

    // get current entry on event stack
    event_stack_p = fbe_event_get_current_stack( event_p );

    // get disk lba and block count of i/o range to remap
    fbe_event_stack_get_extent( event_stack_p, &raid_lba, &block_count );

    // get the external capacity of the RAID group  which  describes
    // the area available for user data; it does not include storage
    // reserved for paged metadata or signature
    fbe_base_config_get_capacity((fbe_base_config_t*)in_raid_group_p, &external_capacity );

    fbe_raid_group_get_configured_capacity(in_raid_group_p, &configured_capacity);


    // if the i/o range to remap  falls within the raid group's
    // metadata or signature, raid always remaps the i/o range;
    // return FBE_EVENT_STATUS_OK so that  the  provision drive
    // does not take any further action to remap this i/o range
    if ( (raid_lba + block_count) >= external_capacity )
    {
        fbe_base_config_event_queue_push_event((fbe_base_config_t *) in_raid_group_p, event_p);

        /* we have some work to do and so reschedule the monitor immediately */
        fbe_lifecycle_reschedule(&fbe_raid_group_lifecycle_const, (fbe_base_object_t*) in_raid_group_p, 0);

        return FBE_STATUS_OK;
    }

    // set new entry's completion function
    fbe_event_stack_set_completion_function(event_stack_p,
                                            fbe_raid_group_main_remap_event_mark_verify,
                                            in_raid_group_p);

    // send "data request" event to ask for remap action from upstream object
    fbe_base_config_send_event( (fbe_base_config_t *)in_raid_group_p, FBE_EVENT_TYPE_DATA_REQUEST, event_p );

    return FBE_STATUS_OK;
}   // end fbe_raid_group_main_handle_remap_event()


/*!***************************************************************
 * fbe_raid_group_main_remap_event_mark_verify
 *****************************************************************
 * @brief
 *    This is the completion function used for the remap event to
 *    mark a system verify, as needed, for the specified chunk.
 *
 * @param   in_event_p    -  pointer ask remap action event
 * @param   in_context    -  context, which is a pointer to RAID group
 *
 * @return  fbe_status_t  -  always FBE_STATUS_OK
 *
 * @author
 *    07/16/2010 - Created. Randy Black
 *
 ****************************************************************/

fbe_status_t
fbe_raid_group_main_remap_event_mark_verify( fbe_event_t * in_event_p,
                                             fbe_event_completion_context_t in_context)
{
    fbe_event_stack_t*                      event_stack_p;      // event stack pointer
    fbe_event_status_t                      event_status;       // event status
    
    fbe_raid_group_t*                       raid_group_p;       // pointer to RAID group
    fbe_raid_geometry_t*     geo_p = NULL;

    
    // trace function entry
    FBE_RAID_GROUP_TRACE_FUNC_ENTRY( in_context );

    // cast context to RAID group
    raid_group_p = (fbe_raid_group_t *)in_context;

    geo_p = fbe_raid_group_get_raid_geometry(raid_group_p);

    // get current entry on event stack
    event_stack_p = fbe_event_get_current_stack( in_event_p );

    // get event status from event
    fbe_event_get_status( in_event_p, &event_status );

    // trace error in marking a chunk for system verify
    fbe_base_object_trace( (fbe_base_object_t *)raid_group_p, 
                           FBE_TRACE_LEVEL_INFO,
                           FBE_TRACE_MESSAGE_ID_INFO,
                           "%s Marking for verify, status: 0x%x. RG_type:%d\n",
                           __FUNCTION__, event_status, geo_p->raid_type);

    if((event_status == FBE_EVENT_STATUS_OK) && (geo_p->raid_type != FBE_RAID_GROUP_TYPE_RAID10))
    {
        /* Need to queue this up only for the good case and also if the raid type is not R10 because
         * verifies will be handled by the Mirror and NOT the striper
         */
        fbe_base_config_event_queue_push_event((fbe_base_config_t *)raid_group_p, in_event_p);

        /* we have some work to do and so reschedule the monitor immediately */

        fbe_lifecycle_reschedule(&fbe_raid_group_lifecycle_const, (fbe_base_object_t*) raid_group_p, 0);
        
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }
    else
    {
        fbe_event_release_stack( in_event_p, event_stack_p );
        return FBE_STATUS_OK;
    }

}   // fbe_raid_group_main_remap_event_mark_verify()


/*!****************************************************************************
 * fbe_raid_group_process_remap_request()
 ******************************************************************************
 * @brief
 *       This function will mark the chunk for error verify
 * 
 * @param in_raid_group_p       - pointer to a raid group object
 * @param in_packet_p           - pointer to a control packet from the scheduler
 * @param in_data_event_p       - data event to mark needs rebuild
 * 
 * @return fbe_status_t        
 *
 * @author
 *   03/06/2012 - Created. Ashwin Tamilarasan
 *
 ******************************************************************************/
fbe_lifecycle_status_t fbe_raid_group_process_remap_request(
                                    fbe_raid_group_t*   raid_group_p,
                                    fbe_event_t*        data_event_p,
                                    fbe_packet_t*       packet_p)
{
    fbe_lba_t                                   raid_lba;  
    fbe_block_count_t                           block_count;
    fbe_event_t*                                event_p = NULL;
    fbe_event_stack_t*                          event_stack_p;
    fbe_payload_ex_t*                           sep_payload_p;
    fbe_chunk_index_t                           chunk_index;
    fbe_chunk_count_t                           chunk_count;
    fbe_scheduler_hook_status_t                 hook_status = FBE_SCHED_STATUS_OK;
    fbe_lba_t                                   imported_capacity;
    fbe_lba_t                                   configured_capacity;
    fbe_payload_block_operation_t *             block_operation_p = NULL;
    fbe_optimum_block_size_t                    optimum_block_size;

    event_stack_p = fbe_event_get_current_stack(data_event_p);

    /* If the raid grooup is degraded, remove it from the queue
       and complete the event with a failure 
    */
    if(fbe_raid_group_is_degraded(raid_group_p))
    {
        fbe_base_config_event_queue_pop_event((fbe_base_config_t*)raid_group_p, &event_p);
        fbe_event_set_status(event_p, FBE_EVENT_STATUS_GENERIC_FAILURE);
        fbe_event_complete(event_p);
        return FBE_LIFECYCLE_STATUS_DONE;

    }

    fbe_raid_group_check_hook(raid_group_p,  SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_SNIFF_VERIFY, 
                                  FBE_PROVISION_DRIVE_SUBSTATE_SNIFF_EVENT_QUEUED, 0, &hook_status);
    if (hook_status == FBE_SCHED_STATUS_DONE) {return FBE_LIFECYCLE_STATUS_DONE;}

    //  Get the extent of the event stack
    fbe_event_stack_get_extent(event_stack_p, &raid_lba, &block_count);

    /* Get  the imported capacity and see whether the lba is in the journal area or not */
    fbe_raid_group_get_imported_capacity(raid_group_p, &imported_capacity);
    fbe_raid_group_get_configured_capacity(raid_group_p, &configured_capacity);
    
    if(raid_lba >= configured_capacity && raid_lba < imported_capacity)
    {
        fbe_transport_set_completion_function(packet_p, fbe_raid_group_mark_journal_for_verify_completion, raid_group_p);
        fbe_raid_group_mark_journal_for_verify(packet_p, raid_group_p, raid_lba);
        return FBE_LIFECYCLE_STATUS_PENDING;

    }

    fbe_raid_group_bitmap_get_chunk_index_and_chunk_count(raid_group_p, raid_lba, block_count, &chunk_index, &chunk_count);

    // allocate block operation
    sep_payload_p = fbe_transport_get_payload_ex(packet_p); 
    block_operation_p   = fbe_payload_ex_allocate_block_operation(sep_payload_p);
    
    /* get optimum block size for this i/o request */
    fbe_block_transport_edge_get_optimum_block_size(
        ((fbe_base_config_t *)raid_group_p)->block_edge_p,
        &optimum_block_size);

    /* next, build block operation in sep payload */
    fbe_payload_block_build_operation(block_operation_p,
                                      FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INITIATE_ERROR_VERIFY,
                                      raid_lba,
                                      block_count,
                                      FBE_BE_BYTES_PER_BLOCK,
                                      optimum_block_size,
                                      NULL);

    fbe_payload_ex_increment_block_operation_level(sep_payload_p);

    fbe_transport_set_completion_function(packet_p,
                                          fbe_raid_group_process_remap_request_completion,
                                          raid_group_p);

    // now, mark specified chunk for system verify
    fbe_raid_group_verify_process_verify_request(packet_p, raid_group_p, chunk_index, FBE_RAID_VERIFY_TYPE_ERROR, chunk_count); 

    return FBE_LIFECYCLE_STATUS_PENDING;

} // End fbe_raid_group_process_remap_request()



/*!****************************************************************************
 * fbe_raid_group_process_remap_request_completion()
 ******************************************************************************
 * @brief
 *       This is the completion function for marking for error verify as 
 *       part of remap request. This will remove the event from the queue
 *      and complete the event
 * 
 * @param packet_p           - pointer to a control packet from the scheduler
 * @param context
 * 
 * @return fbe_status_t        
 *
 * @author
 *   03/06/2012 - Created. Ashwin Tamilarasan
 *
 ******************************************************************************/
fbe_status_t fbe_raid_group_process_remap_request_completion(
                                                           fbe_packet_t*       packet_p,
                                                           fbe_packet_completion_context_t  context)

{
    fbe_status_t                            packet_status;
    fbe_raid_group_t*                       raid_group_p = NULL;
    fbe_event_t*                            data_event_p = NULL;
    fbe_payload_ex_t*                       payload_p = NULL;
    fbe_payload_block_operation_t*          block_operation_p = NULL;
    fbe_event_stack_t*                      current_stack = NULL;
    fbe_payload_block_operation_status_t    block_status;

    raid_group_p = (fbe_raid_group_t*)context;

    fbe_base_config_event_queue_pop_event((fbe_base_config_t*)raid_group_p, &data_event_p);
    if(data_event_p == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                              FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s: No event in the queue\n",
                              __FUNCTION__);
        return FBE_STATUS_OK;
    }

    current_stack = fbe_event_get_current_stack(data_event_p);
    fbe_event_release_stack(data_event_p, current_stack);

    packet_status = fbe_transport_get_status_code(packet_p);

    payload_p = fbe_transport_get_payload_ex(packet_p);
    block_operation_p = fbe_payload_ex_get_block_operation(payload_p);
    fbe_payload_block_get_status(block_operation_p, &block_status);   

    fbe_payload_ex_release_block_operation(payload_p, block_operation_p);

    if((packet_status != FBE_STATUS_OK) || (block_status != FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS))
    {
        fbe_event_set_status(data_event_p, FBE_EVENT_STATUS_GENERIC_FAILURE);
    }
    else
    {
        fbe_event_set_status(data_event_p, FBE_EVENT_STATUS_OK);
    }

    fbe_event_complete(data_event_p);

    /* we have some work to do and so reschedule the monitor immediately */
    fbe_lifecycle_reschedule(&fbe_raid_group_lifecycle_const, (fbe_base_object_t*) raid_group_p, 0);

    return FBE_STATUS_OK;


} // End fbe_raid_group_process_remap_request_completion


/*!****************************************************************************
 * fbe_raid_group_empty_event_queue()
 ******************************************************************************
 * @brief
 *       This function will empty the event queue
 * 
 * @param in_raid_group_p       - pointer to a raid group object
 * 
 * @return fbe_status_t        
 *
 * @author
 *   03/12/2012 - Created. Ashwin Tamilarasan
 *
 ******************************************************************************/
fbe_status_t fbe_raid_group_empty_event_queue(fbe_raid_group_t*   raid_group_p)
{
    fbe_event_t*                            data_event_p = NULL;
    fbe_scheduler_hook_status_t            hook_status;

    while(!fbe_base_config_event_queue_is_empty( (fbe_base_config_t*)raid_group_p) )
    {

        fbe_raid_group_check_hook(raid_group_p,  SCHEDULER_MONITOR_STATE_RAID_GROUP_REBUILD, 
                                  FBE_RAID_GROUP_SUBSTATE_GOING_BROKEN, 0, &hook_status);
        
        fbe_base_config_event_queue_pop_event((fbe_base_config_t*)raid_group_p, &data_event_p);
        fbe_event_set_status(data_event_p, FBE_EVENT_STATUS_GENERIC_FAILURE);
        fbe_event_complete(data_event_p);
    }

    return FBE_STATUS_OK;

} // End fbe_raid_group_empty_event_queue


/*!****************************************************************************
 * fbe_raid_group_determine_edge_health() 
/*!****************************************************************************
 * @brief
 *   This function will determine the health of all the edges.  First it will get  
 *   the path state counters contained in the object.  Then it will adjust them for 
 *   any edge that is up (enabled) but marked NR or NR on demand.  Such an edge  
 *   is technically "unavailable" when determining the raid object's state.  
 *
 *   The edge counters will be adjusted if needed and then returned in the output 
 *   parameter. 
 *
 * @param in_raid_group_p               - pointer to a raid group object
 * @param out_edge_state_counters_p     - pointer to structure that gets populated
 *                                        with counters indicating the states of the
 *                                        edges 
 *
 * @return fbe_status_t   
 *
 * @author
 *   03/08/2010 - Created. Jean Montes. 
 *
 ****************************************************************/
fbe_status_t fbe_raid_group_determine_edge_health(
                                    fbe_raid_group_t*                       in_raid_group_p,
                                    fbe_base_config_path_state_counters_t*  out_edge_state_counters_p)
{

    fbe_base_config_t*              base_config_p;              // pointer to a base config object
    fbe_block_edge_t*               edge_p;                     // pointer to an edge 
    fbe_u32_t                       edge_index;                 // index to the current edge
    fbe_path_state_t                path_state;                 // path state of the edge 
    fbe_bool_t                      nr_on_demand_b;             // true if nr_on_demand is set for the edge
    fbe_lba_t                       checkpoint;                 // rebuild checkpoint for the edge/disk 
    fbe_path_attr_t                 path_attrib;
    fbe_u32_t                       new_pvd_bitmap = 0;

    //  Trace function entry 
    FBE_RAID_GROUP_TRACE_FUNC_ENTRY(in_raid_group_p);

    //  Set up a pointer to the object as a base config object 
    base_config_p = (fbe_base_config_t*) in_raid_group_p; 

    //  Get the path counters, which contain the number of edges enabled, disabled, and broken
    fbe_base_config_get_path_state_counters(base_config_p, out_edge_state_counters_p);

    if (fbe_raid_group_is_c4_mirror_raid_group(in_raid_group_p)) {
        fbe_raid_group_c4_mirror_get_new_pvd(in_raid_group_p, &new_pvd_bitmap);
    }

    //  Loop through all the edges to find if any are enabled but marked NR.  An edge that is enabled but marked 
    //  NR is "unhealthy" and need to be considered broken when determining the raid object's health. 
    for (edge_index = 0; edge_index < base_config_p->width; edge_index++)
    {
        fbe_bool_t is_new_pvd;
        //  Get a pointer to the next edge
        fbe_base_config_get_block_edge(base_config_p, &edge_p, edge_index);

        //  Get the edge's path state 
         fbe_block_transport_get_path_state(edge_p, &path_state);
        fbe_block_transport_get_path_attributes(edge_p, &path_attrib);

        //  If the edge is anything other than enabled, or if the timeout errors attribute is set,
        //  then these edges are considered bad.  We don't need to do anything more - move on to the next 
        //  edge
        if ((path_state != FBE_PATH_STATE_ENABLED) || 
            (path_attrib & FBE_BLOCK_PATH_ATTR_FLAGS_TIMEOUT_ERRORS)) 
        {
            continue;
        }

        //  The edge is enabled.  If it is marked NR on demand or the rebuild checkpoint is not at the logical
        //  marker, meaning it is marked NR, we'll need to adjust its state.  If not, we don't need to do 
        //  anything - move on to the next edge. 
        //  (NR on demand should not be marked when the edge is enabled, but we'll check it anyway.)
        //
        fbe_raid_group_get_rb_logging(in_raid_group_p, edge_index, &nr_on_demand_b); 
        fbe_raid_group_get_rebuild_checkpoint(in_raid_group_p, edge_index, &checkpoint);
        is_new_pvd = (1 << edge_index) & new_pvd_bitmap;
        /* c4mirror raid group: new PVD will be set to broken */
        if ((nr_on_demand_b == FBE_FALSE) && (checkpoint == FBE_LBA_INVALID) && !is_new_pvd)
        {
            continue; 
        }

        //  The edge is marked NR/NR on demand so it is unavailable.  Change the edge state counters accordingly.
        out_edge_state_counters_p->broken_counter++;
        out_edge_state_counters_p->enabled_counter--; 

    } // end for 

    //  Return success
    return FBE_STATUS_OK;

} // End fbe_raid_group_determine_edge_health()

/*!**************************************************************
 * fbe_raid_group_trace()
 ****************************************************************
 * @brief
 *  Trace for a raid group at a given trace level. 
 * 
 * @param raid_group_p - siots to transition to failed.
 * @param trace_level - trace level to create this trace at.
 * @param trace_debug_flags - Debug flag that needs to be on to trace this.
 * @param string_p - String to display.
 *
 * @return - None.
 *
 * @author
 *  4/27/2011 - Created. Rob Foley
 *
 ****************************************************************/

void fbe_raid_group_trace(fbe_raid_group_t *raid_group_p,
                          fbe_trace_level_t trace_level,
                          fbe_raid_group_debug_flags_t trace_debug_flags,
                          fbe_char_t * string_p, ...)
{   
    fbe_raid_group_debug_flags_t rg_debug_flags;
    fbe_raid_group_get_debug_flags(raid_group_p, &rg_debug_flags);

    /* We trace if there are no flags specified or if the input trace flag is enabled.
     */
    if ((trace_debug_flags == FBE_RAID_GROUP_DEBUG_FLAG_NONE) ||
        (trace_debug_flags & rg_debug_flags))
    {
        char buffer[FBE_RAID_GROUP_MAX_TRACE_CHARS];
        fbe_u32_t num_chars_in_string = 0;
        va_list argList;
    
        if (string_p != NULL)
        {
            va_start(argList, string_p);
            num_chars_in_string = _vsnprintf(buffer, FBE_RAID_GROUP_MAX_TRACE_CHARS, string_p, argList);
            buffer[FBE_RAID_GROUP_MAX_TRACE_CHARS - 1] = '\0';
            va_end(argList);
        }
        /* Display the string, if it has some characters.
         */
        if (num_chars_in_string > 0)
        {
            fbe_base_object_trace((fbe_base_object_t *)raid_group_p, trace_level, FBE_TRACE_MESSAGE_ID_INFO, buffer);
        }
    }
    return;
}
/**************************************
 * end fbe_raid_group_trace()
 **************************************/

/*!****************************************************************************
 * fbe_raid_group_edge_state_change_event_entry()
 ******************************************************************************
 * @brief
 *   This function handles a change in the state of a downstream edge.  It 
 *   will evaluate the downstream health of all of the edges connected to the 
 *   RAID object and take actions as needed. 
 *
 * @param in_parity_p           - pointer to a parity object 
 * @param in_event_context      - context for the event; this is a pointer to 
 *                                the edge whose state changed
 *
 * @return fbe_status_t         - status from called function 
 *
 * @author
 *   8/18/2009 - Created. Dhaval Patel
 *
 ******************************************************************************/
fbe_status_t fbe_raid_group_edge_state_change_event_entry(fbe_raid_group_t* const    in_raid_group_p,
                                                     fbe_event_context_t    in_event_context)
{

    fbe_base_edge_t*                            edge_p;                 // pointer to the edge which changed
    fbe_base_config_downstream_health_state_t   downstream_health;      // overall health of all edges
    fbe_status_t                                status;                 // fbe status
    fbe_path_attr_t 							path_attrib = 0;
    fbe_path_state_t                            path_state = FBE_PATH_STATE_INVALID;      

    //  Trace function entry
    FBE_RAID_GROUP_TRACE_FUNC_ENTRY(in_raid_group_p);

    //  Set up a pointer to the edge which changed its state
    edge_p = (fbe_base_edge_t*) in_event_context;

    path_state = edge_p->path_state;

    fbe_base_transport_get_path_attributes(edge_p, &path_attrib);

    /* If this path attribute is set, it overrides the path state.
     */
    if (path_attrib & FBE_BLOCK_PATH_ATTR_FLAGS_TIMEOUT_ERRORS)
    {
        path_state = FBE_PATH_STATE_BROKEN; 
    }	

    /* Only process the event if the raid group object is initialized.
     */
    if (!fbe_raid_group_is_flag_set(in_raid_group_p, FBE_RAID_GROUP_FLAG_INITIALIZED))
    {
        fbe_base_object_trace((fbe_base_object_t*) in_raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO, 
                              "rg_edge_state_chg_evt_entry: not initted edge index: %d, path state: %d\n", 
                              edge_p->client_index, path_state);
        /* we need to kick the object after the edge is ready */
        fbe_lifecycle_reschedule(&fbe_raid_group_lifecycle_const,
                                (fbe_base_object_t *) in_raid_group_p,
                                (fbe_lifecycle_timer_msec_t) 0);    /* Immediate reschedule */	
        return FBE_STATUS_OK;
    }
    
    fbe_base_object_trace((fbe_base_object_t*) in_raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO, 
                          "rg_edge_state_chg_evt_entry: edge index %d, path state %d\n", 
                          edge_p->client_index, path_state);

    //  Determine the overall health of the paths to the downstream objects
    downstream_health = fbe_raid_group_verify_downstream_health(in_raid_group_p);

    //  Perform any NR/rebuild processing needed based on the new edge state 
    fbe_raid_group_needs_rebuild_handle_processing_on_edge_state_change(in_raid_group_p, 
                edge_p->client_index, path_state, downstream_health);

    //  Set condition(s) as needed based on the downstream path health 
    status = fbe_raid_group_set_condition_based_on_downstream_health(in_raid_group_p, downstream_health,
                                                                     path_state,
                                                                     edge_p);

    //  Return the status from the above function
    return status;

} // End fbe_raid_group_edge_state_change_event_entry()


/*!****************************************************************************
 * fbe_raid_group_verify_downstream_health()
/*!****************************************************************************
 * @brief
 *   This function is used to check the health of the downstream paths. It 
 *   returns a "health value" based only on the current state of the downstream
 *   edges.  Note that it does not return the health of the raid group object 
 *   itself.
 *
 * @param in_parity_p               - pointer to a parity object 
 *
 * @return fbe_base_config_downstream_health_state_t:
 *                              
 *   FBE_DOWNSTREAM_HEALTH_OPTIMAL   - All edges are enabled or in slumber 
 *   FBE_DOWNSTREAM_HEALTH_DEGRADED  - One edge is unavailable for a R3/R5/R6, or 
 *                                     two edges are unavailable for a R6
 *   FBE_DOWNSTREAM_HEALTH_BROKEN    - Two or more edges are broken for a 
 *                                     R3/R5; or three or more edges for a R6 
 *   FBE_DOWNSTREAM_HEALTH_DISABLED  - For a R3/R5:
 *                                       - two or more edges are disabled or 
 *                                       - one is broken and one or more is disabled
 *                                   - For a R6: to be implemented 
 *
 * @author
 *  8/14/2009 - Created. Dhaval Patel
 *
 ****************************************************************/
fbe_base_config_downstream_health_state_t fbe_raid_group_verify_downstream_health(fbe_raid_group_t*  in_raid_group_p)
{
    fbe_base_config_path_state_counters_t   path_state_counters;        // summary of path states for all edges 
    fbe_u32_t                               number_of_edges_down;       // number of edges unavailable
    fbe_raid_geometry_t*                    raid_geometry_p;            // geometry of raid group
    fbe_u16_t                               num_data_disks;             // number of data disks in raid group
    fbe_u16_t                               num_parity_disks;           // number of parity disks in raid group
    fbe_u32_t                               width;                      // width of raid group    

    FBE_RAID_GROUP_TRACE_FUNC_ENTRY(in_raid_group_p);
    
    fbe_raid_group_determine_edge_health(in_raid_group_p, &path_state_counters);

    //  Set the number of edges which are unavailable 
    number_of_edges_down = path_state_counters.disabled_counter + path_state_counters.broken_counter +
                           path_state_counters.invalid_counter;    
    //
    //  If none of the edges are down, return healthy 
    if (number_of_edges_down == 0)
    {
        return FBE_DOWNSTREAM_HEALTH_OPTIMAL;
    }

    //  There are edges down. The downstream health will vary based on whether those edges are
    //  disabled or broken (== BROKEN or GONE) along with the RAID type as follows:
    //
    //  RAID-5 and RAID-3:
    //      If more than one of the down edges is broken    -> health is broken
    //      If only one of the down edges is broken         -> health is degraded 
    //      If all of the down edges are disabled           -> health is disabled
    //
    //  RAID-6
    //      If more than two of the down edges is broken    -> health is broken
    //      If one or two of the down edges is broken       -> health is degraded
    //      If all of the down edges are disabled           -> health is disabled


    //  Get the geometry of the raid group
    raid_geometry_p = fbe_raid_group_get_raid_geometry(in_raid_group_p);

    //  Get the number of data disks in the raid group
    fbe_raid_geometry_get_data_disks(raid_geometry_p, &num_data_disks);

    // Get the width of the raid group
    fbe_raid_geometry_get_width(raid_geometry_p, &width);

    //  Get the number of parity disks in the raid group
    num_parity_disks = width - num_data_disks;

    //  If the number of edges down is not greater than the number of parity disks, health is degraded
    if (number_of_edges_down <= num_parity_disks)
    {
        return FBE_DOWNSTREAM_HEALTH_DEGRADED;
    }

    //  If the number of edges down is greater than the number of parity disks, health is broken
    if (path_state_counters.broken_counter > num_parity_disks)
    {
        return FBE_DOWNSTREAM_HEALTH_BROKEN;
    }

    //  At most one edge is broken and the rest of the down edges are disabled - health is disabled
    return FBE_DOWNSTREAM_HEALTH_DISABLED; 

} // End fbe_raid_group_verify_downstream_health()


/*!****************************************************************************
 * fbe_raid_group_set_condition_based_on_downstream_health()
 ******************************************************************************
 * @brief
 *   This function will set the conditions as needed based on the health of the 
 *   communication paths with its downstream objects.
 *
 * @param in_parity_p                   - pointer to a parity object 
 * @param in_downstream_health_state    - overall health state of the downstream 
 *                                        objects
 *
 * @return  fbe_status_t
 *
 * @author
 *  8/14/2009 - Created. Dhaval Patel
 *
 ******************************************************************************/
fbe_status_t 
fbe_raid_group_set_condition_based_on_downstream_health(fbe_raid_group_t*  in_raid_group_p, 
                                                        fbe_base_config_downstream_health_state_t in_downstream_health_state,                                                         
                                                        fbe_path_state_t       path_state,
                                                        fbe_base_edge_t* edge_p)
{
    fbe_status_t                 status = FBE_STATUS_OK;
    fbe_bool_t                   b_is_rb_logging;        
    fbe_u32_t                    in_position = edge_p->client_index;
    fbe_path_attr_t              path_attrib = 0;     

    
    FBE_RAID_GROUP_TRACE_FUNC_ENTRY(in_raid_group_p);

    switch (in_downstream_health_state)
    {
        //  Downstream health optimal - no action needed so far 
        case FBE_DOWNSTREAM_HEALTH_OPTIMAL:
        {
            return FBE_STATUS_OK;
        }
        
        case FBE_DOWNSTREAM_HEALTH_DEGRADED:
        {            
            /* AR669621 & AR706412 & AR758338: 
             * we will invoke eval_rl processing when receiving a path NOT_ENABLED event if:
             * 1. any of the mark nr or clear rl clustered flags are set then ignore the rl mask and invoke the eval rl processing
             * or
             * 2. the rebuild_logging_bitmask has not been set on this pos  
             *
             * AR774579 & AR764323:
             * Timeout errors are being set from the vd (i.e. paged metadata verify) while the raid group was already rebuild logging 
             * In one case, the raid group would get stuck in clear rl since the peer had timeout errors which was never cleared.
             * In another case, the raid group would get stuck in mark nr because timeout errors were still set so we skip those edges.
             * Ensure eval rl evaluates the edges and takes appropriate actions when timeout errors are set on an already degraded position.       
             */ 
            fbe_raid_group_get_rb_logging(in_raid_group_p, in_position, &b_is_rb_logging);
            fbe_base_transport_get_path_attributes(edge_p, &path_attrib);
            if(path_state != FBE_PATH_STATE_ENABLED 
               && ((fbe_raid_group_is_any_clustered_flag_set(in_raid_group_p, FBE_RAID_GROUP_CLUSTERED_FLAG_EVAL_RL_MASK | FBE_RAID_GROUP_CLUSTERED_FLAG_CLEAR_RL_MASK)
                    || (b_is_rb_logging == FBE_FALSE))
                   || (b_is_rb_logging == FBE_TRUE) && (path_attrib & FBE_BLOCK_PATH_ATTR_FLAGS_TIMEOUT_ERRORS)))
            {
                fbe_raid_group_needs_rebuild_handle_processing_on_edge_unavailable(in_raid_group_p, in_position, 
                                                                                   in_downstream_health_state);
                fbe_raid_group_lock(in_raid_group_p);
                /* If we are in activate and a drive a went down, we need to reevaluate all the
                   conditions in activate state. So set the conds accordingly                
                */
                if(fbe_raid_group_is_local_state_set(in_raid_group_p, FBE_RAID_GROUP_LOCAL_STATE_ACTIVATE))
                {     
                    fbe_lifecycle_set_cond(&fbe_raid_group_lifecycle_const, (fbe_base_object_t*) in_raid_group_p,
                                                FBE_RAID_GROUP_LIFECYCLE_COND_EVAL_RB_LOGGING_ACTIVATE);
                    fbe_lifecycle_set_cond(&fbe_raid_group_lifecycle_const, (fbe_base_object_t*) in_raid_group_p,
                                                FBE_RAID_GROUP_LIFECYCLE_COND_VERIFY_PAGED_METADATA);
                }
                fbe_raid_group_set_local_state(in_raid_group_p, FBE_RAID_GROUP_LOCAL_STATE_EVAL_RL_REQUEST);
                fbe_raid_group_unlock(in_raid_group_p); 
                
                status = fbe_lifecycle_set_cond(&fbe_raid_group_lifecycle_const, (fbe_base_object_t*) in_raid_group_p,
                                                FBE_RAID_GROUP_LIFECYCLE_COND_EVAL_RB_LOGGING);
                return status;
            }
            
            return FBE_STATUS_OK;
        }
        
        case FBE_DOWNSTREAM_HEALTH_DISABLED:
        {          
            fbe_raid_group_get_rb_logging(in_raid_group_p, in_position, &b_is_rb_logging);
            if (b_is_rb_logging == FBE_FALSE)
            {
                fbe_raid_group_needs_rebuild_handle_processing_on_edge_unavailable(in_raid_group_p, in_position, 
                                                                                   in_downstream_health_state);
                fbe_raid_group_lock(in_raid_group_p);
                if(fbe_raid_group_is_local_state_set(in_raid_group_p, FBE_RAID_GROUP_LOCAL_STATE_ACTIVATE))
                {     
                    fbe_lifecycle_set_cond(&fbe_raid_group_lifecycle_const, (fbe_base_object_t*) in_raid_group_p,
                                                FBE_RAID_GROUP_LIFECYCLE_COND_EVAL_RB_LOGGING_ACTIVATE);
                    fbe_lifecycle_set_cond(&fbe_raid_group_lifecycle_const, (fbe_base_object_t*) in_raid_group_p,
                                                FBE_RAID_GROUP_LIFECYCLE_COND_VERIFY_PAGED_METADATA);
                }
                fbe_raid_group_set_local_state(in_raid_group_p, FBE_RAID_GROUP_LOCAL_STATE_EVAL_RL_REQUEST);
                fbe_raid_group_unlock(in_raid_group_p);
                status = fbe_lifecycle_set_cond(&fbe_raid_group_lifecycle_const, (fbe_base_object_t*) in_raid_group_p,
                                                FBE_RAID_GROUP_LIFECYCLE_COND_EVAL_RB_LOGGING);
                return status;
            }

            return FBE_STATUS_OK;
        }
        
        case FBE_DOWNSTREAM_HEALTH_BROKEN:
        {            
            fbe_raid_group_needs_rebuild_handle_processing_on_edge_unavailable(in_raid_group_p, in_position, 
                                                                               in_downstream_health_state);
            /* When the downstream health is broken, we do not want to immediately go to broken since lot of times
               drives will have a glitch. Eval RB logging will evaluate the downstream health and if the DS health is
               broken, it will transition to fail
            */
            fbe_base_object_trace((fbe_base_object_t*)in_raid_group_p, FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s: RAID is going broken thru RL cond\n", __FUNCTION__);
            fbe_raid_group_lock(in_raid_group_p);
            /* If we are in activate and a drive a went down, we need to reevaluate all the
             * conditions in activate state. So set the conds accordingly                
             */
            if(fbe_raid_group_is_local_state_set(in_raid_group_p, FBE_RAID_GROUP_LOCAL_STATE_ACTIVATE))
            {     
                fbe_lifecycle_set_cond(&fbe_raid_group_lifecycle_const, (fbe_base_object_t*) in_raid_group_p,
                                            FBE_BASE_CONFIG_LIFECYCLE_COND_DOWNSTREAM_HEALTH_DISABLED);
                fbe_lifecycle_set_cond(&fbe_raid_group_lifecycle_const, (fbe_base_object_t*) in_raid_group_p,
                                            FBE_RAID_GROUP_LIFECYCLE_COND_EVAL_RB_LOGGING_ACTIVATE);
                fbe_lifecycle_set_cond(&fbe_raid_group_lifecycle_const, (fbe_base_object_t*) in_raid_group_p,
                                            FBE_RAID_GROUP_LIFECYCLE_COND_VERIFY_PAGED_METADATA);
            }
            fbe_raid_group_set_local_state(in_raid_group_p, FBE_RAID_GROUP_LOCAL_STATE_EVAL_RL_REQUEST);
            fbe_raid_group_unlock(in_raid_group_p);
            status = fbe_lifecycle_set_cond(&fbe_raid_group_lifecycle_const, (fbe_base_object_t*) in_raid_group_p,
                                                FBE_RAID_GROUP_LIFECYCLE_COND_EVAL_RB_LOGGING);
            return status;
        }
        
        default:
        {
            fbe_base_object_trace((fbe_base_object_t*) in_raid_group_p, FBE_TRACE_LEVEL_ERROR, 
                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, "%s Invalid downstream health state:: 0x%x!!\n", 
                __FUNCTION__, in_downstream_health_state);
            return FBE_STATUS_GENERIC_FAILURE;
        }

    } 

} // End fbe_raid_group_set_condition_based_on_downstream_health()

/*!****************************************************************************
 * fbe_raid_group_handle_sparing_request_event()
 ******************************************************************************
 * @brief
 *   This function will handle a "sparing request" event.  This event is sent by
 *   the VD when it wants to swap in a permanent spare.  The function 
 *   will decide if a spare is allowed to swap in and complete the event with the  
 *   appropriate response. 
 *
 * @param in_mirror_p           - pointer to a mirror object
 * @param in_event_context      - event context  
 *
 * @return fbe_status_t   
 *
 * @author
 *   11/18/2009 - Created. JMM
 *
 ******************************************************************************/
fbe_status_t fbe_raid_group_handle_sparing_request_event(
                                                fbe_raid_group_t*       in_raid_group_p,
                                                fbe_event_context_t     in_event_context)
{
    fbe_bool_t                                  b_is_raid_group_in_use = FBE_TRUE;
    fbe_bool_t                                  b_is_raid_group_healthy = FBE_TRUE;
    fbe_bool_t                                  b_is_rb_logging = FBE_TRUE;
    fbe_event_t                                 *event_p = NULL;
    fbe_base_edge_t                             *edge_p = NULL;
    fbe_lifecycle_state_t                       lifecycle_state;
    
    FBE_RAID_GROUP_TRACE_FUNC_ENTRY(in_raid_group_p);
    
    event_p = (fbe_event_t*) in_event_context;  
    
    edge_p = event_p->base_edge;

    /* First check if raid group is in use or not. 
     */
    fbe_raid_group_util_is_raid_group_in_use(in_raid_group_p, &b_is_raid_group_in_use);

    /* We only allow the swap if the raid group is Ready/Hibernating
     */
    fbe_base_object_get_lifecycle_state((fbe_base_object_t *)in_raid_group_p, &lifecycle_state);
    if ((lifecycle_state != FBE_LIFECYCLE_STATE_READY)     &&
        (lifecycle_state != FBE_LIFECYCLE_STATE_HIBERNATE)    )
    {
        b_is_raid_group_healthy = FBE_FALSE;
    }
   
    /* We only allow the swap if we have detected the broken edge.  Otherwise we
     * wait until we detect the broken edge.
     */
    fbe_raid_group_get_rb_logging(in_raid_group_p, edge_p->client_index, &b_is_rb_logging);

    /*! @note All of the following must be True before was allow the spare
     *        operation:
     *          o Raid group must be in-use (i.e. LU bound)
     *          o Raid group must be healthy (i.e. not broken)
     *          o Raid group must have detected the edge going away (i.e. rebuild logging)
     */
    if ((b_is_raid_group_in_use == FBE_FALSE)  ||
        (b_is_raid_group_healthy == FBE_FALSE) ||
        (b_is_rb_logging == FBE_FALSE)            )
    {
        /* Log an informational message and deny the sparing request.
         */
        fbe_base_object_trace((fbe_base_object_t *)in_raid_group_p,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "raid_group: spare request in-use: %d healthly: %d rl: %d, lifecycle %d, denied. \n",
                              b_is_raid_group_in_use, b_is_raid_group_healthy, b_is_rb_logging, lifecycle_state); 

        fbe_event_set_flags(event_p, FBE_EVENT_FLAG_DENY);
    }

    fbe_event_set_status(event_p, FBE_EVENT_STATUS_OK);
    fbe_event_complete(event_p);
    
    return FBE_STATUS_OK;

} // End fbe_raid_group_handle_sparing_request_event()

/*!***************************************************************************
 *          fbe_raid_group_handle_copy_request_event()
 *****************************************************************************
 *
 * @brief   This function will handle a "copy request" event.  This event is 
 *          sent by the virtual drive when it wants to start (i.e. swap-in) a 
 *          copy operation for (1) of (3) reasons:
 *              o System initiated proactive copy
 *              o User initiated proactive copy
 *              o User initiated user copy
 *
 * @param   raid_group_p - Pointer to raid group object.
 * @param   event_context - Pointer to event context 
 *
 * @return  status - FBE_STATUS_OK
 *
 * @note    Currently all copy operation request types are treated the same.
 *          If the raid group is degraded the request is denied.
 *
 * @author
 *  05/22/2012  Ron Proulx  - Created.
 *
 *****************************************************************************/
fbe_status_t fbe_raid_group_handle_copy_request_event(fbe_raid_group_t *raid_group_p,
                                                      fbe_event_context_t event_context)
{
    fbe_bool_t  b_is_raid_group_in_use = FBE_TRUE;
    fbe_bool_t  b_is_raid_group_healthy = FBE_TRUE;
    fbe_event_t *event_p = (fbe_event_t*)event_context;
    fbe_base_config_encryption_state_t encryption_state;

    FBE_RAID_GROUP_TRACE_FUNC_ENTRY(raid_group_p);
    
    /* First check if raid group is in use or not.
     */
    fbe_raid_group_util_is_raid_group_in_use(raid_group_p, &b_is_raid_group_in_use);

    /* If the raid groups is degraded (i.e. if a single drive is missing or is
     * rebuilding) the request is denied.  Otherwise the request is granted.
     */
    if (fbe_raid_group_is_degraded(raid_group_p) == FBE_TRUE)
    {
        b_is_raid_group_healthy = FBE_FALSE;
        /* Log an informational message and deny the request.
         */
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "raid_group: raid group degraded, copy request denied.\n");
        fbe_event_set_flags(event_p, FBE_EVENT_FLAG_DENY);
    }

    /* If the raid group is not in-use or isn't healthy deny the sparing request.
     */
    if ((b_is_raid_group_in_use == FBE_FALSE)  ||
        (b_is_raid_group_healthy == FBE_FALSE)    )
    {
        /* Log an informational message and deny the sparing request.
         */
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "raid_group: copy request in-use: %d healthly: %d denied. \n",
                              b_is_raid_group_in_use, b_is_raid_group_healthy); 
        fbe_event_set_flags(event_p, FBE_EVENT_FLAG_DENY);
    } 

    fbe_base_config_get_encryption_state((fbe_base_config_t *)raid_group_p,
                                          &encryption_state);

    /* If the raid groups is in encryption, the request is denied. or
     * if we are encrypted but not really completed the key update process deny the request
     */
    if ((fbe_base_config_is_rekey_mode((fbe_base_config_t *)raid_group_p)) ||
        (fbe_base_config_is_encrypted_mode((fbe_base_config_t *)raid_group_p) &&
         (encryption_state != FBE_BASE_CONFIG_ENCRYPTION_STATE_KEYS_PROVIDED)))
    {
        /* Log an informational message and deny the sparing request.
         */
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "raid_group: encryption in progress or key update in progress :%d\n", encryption_state); 
        fbe_event_set_flags(event_p, FBE_EVENT_FLAG_DENY);
    } 

    /* Complete the event.
     */
    fbe_event_set_status(event_p, FBE_EVENT_STATUS_OK);
    fbe_event_complete(event_p);
    
    return FBE_STATUS_OK;
}
/*************************************************
 * end fbe_raid_group_handle_copy_request_event()
 *************************************************/

/*!***************************************************************************
 *          fbe_raid_group_handle_abort_copy_request_event()
 *****************************************************************************
 *
 * @brief   This function will handle a "abort copy request" event.  This event 
 *          is sent by the virtual drive when it wants to abort (i.e. swap-out)
 *          the copy operation before it is complete.  The result is that virtual
 *          drive will be degraded.  We deny this request if:
 *              o Raid group is broken
 *              o Raid group is degraded 
 *
 * @param   raid_group_p - Pointer to raid group object.
 * @param   event_context - Pointer to event context 
 *
 * @return  status - FBE_STATUS_OK
 *
 * @note    Currently all copy operation request types are treated the same.
 *          If the raid group is degraded the request is denied.
 *
 * @author
 *  09/17/2012  Ron Proulx  - Created.
 *
 *****************************************************************************/
fbe_status_t fbe_raid_group_handle_abort_copy_request_event(fbe_raid_group_t *raid_group_p,
                                                       fbe_event_context_t event_context)
{
    fbe_lifecycle_state_t       lifecycle_state;
    fbe_bool_t                  b_are_other_positions_rl = FBE_FALSE;  
    fbe_bool_t                  b_is_raid_group_rebuilding = FBE_FALSE;
    fbe_bool_t                  b_is_raid_group_healthy = FBE_TRUE;
    fbe_event_t                *event_p = (fbe_event_t*)event_context;
    fbe_base_edge_t            *edge_p = NULL;
    fbe_raid_position_bitmask_t rl_bitmask;
    fbe_raid_position_bitmask_t rebuild_bitmask;
    
    FBE_RAID_GROUP_TRACE_FUNC_ENTRY(raid_group_p);
    
    /* We will use the edge to determine if this is the only position that is
     * is degraded.
     */
    edge_p = event_p->base_edge;

    /* Get the rebuild logging bitmask*/
    fbe_raid_group_get_rb_logging_bitmask(raid_group_p, &rl_bitmask);
    b_are_other_positions_rl = ((rl_bitmask & ~(1 << edge_p->client_index)) == 0) ? FBE_FALSE : FBE_TRUE;

    /* If the raid group is rebuilding don't allow the abort.
     */
    fbe_raid_group_rebuild_get_all_rebuild_position_mask(raid_group_p, &rebuild_bitmask);
    b_is_raid_group_rebuilding = ((rebuild_bitmask & ~(1 << edge_p->client_index)) == 0) ? FBE_FALSE : FBE_TRUE;

    /* We only allow the swap if the raid group is Ready/Hibernating
     */
    fbe_base_object_get_lifecycle_state((fbe_base_object_t *)raid_group_p, &lifecycle_state);
    if ((lifecycle_state != FBE_LIFECYCLE_STATE_READY)     &&
        (lifecycle_state != FBE_LIFECYCLE_STATE_HIBERNATE)    )
    {
        b_is_raid_group_healthy = FBE_FALSE;
    }
     
    /* If the raid group is not healthy or other positions are degraded
     * deny the request.
     */
    if ((b_is_raid_group_healthy == FBE_FALSE)   ||
        (b_are_other_positions_rl == FBE_TRUE)   ||
        (b_is_raid_group_rebuilding == FBE_TRUE)    )
    {
        /* Log an informational message and deny the sparing request.
         */
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "raid_group: abort copy request healthly: %d other rl: %d rebuilding: %d denied. \n",
                              b_is_raid_group_healthy, b_are_other_positions_rl, b_is_raid_group_rebuilding);
        fbe_event_set_flags(event_p, FBE_EVENT_FLAG_DENY);
    } 

    /* Complete the event.
     */
    fbe_event_set_status(event_p, FBE_EVENT_STATUS_OK);
    fbe_event_complete(event_p);
    
    return FBE_STATUS_OK;
}
/******************************************************
 * end fbe_raid_group_handle_abort_copy_request_event()
 ******************************************************/

/*!****************************************************************************
 * fbe_raid_group_get_medic_action_priority()
 ******************************************************************************
 * @brief
 *   This function gets medic action priority for the raid group.
 *
 * @param in_raid_group_p           - pointer to a raid group
 * @param medic_action_priority     - pointer to priority
 *
 * @return fbe_status_t   
 *
 * @author
 *   02/29/2012 - Created. chenl6
 *
 ******************************************************************************/
fbe_status_t
fbe_raid_group_get_medic_action_priority(fbe_raid_group_t *raid_group_p, 
                                         fbe_medic_action_priority_t *medic_action_priority)
{
    fbe_medic_action_priority_t current_priority, highest_priority;
    fbe_lifecycle_state_t  lifecycle_state;
    fbe_status_t status;

    /* Init medic action priority to IDLE. */
    fbe_medic_get_action_priority(FBE_MEDIC_ACTION_IDLE, &highest_priority);

    status = fbe_base_object_get_lifecycle_state((fbe_base_object_t *) raid_group_p, &lifecycle_state);
    if(status != FBE_STATUS_OK || lifecycle_state != FBE_LIFECYCLE_STATE_READY)
    {
        /* The background operations are not running at this time. So the priority 
         * is set to IDLE.
         * If we want to reject the event when the RG is not ready (sniff), we should
         * do it explicitly.
         */
        *medic_action_priority = highest_priority;
        return FBE_STATUS_OK;
    }

    /* Check whether the raid group is rebuilding. */
    if (fbe_raid_group_background_op_is_metadata_rebuild_need_to_run(raid_group_p) ||
        fbe_raid_group_background_op_is_rebuild_need_to_run(raid_group_p))
    {
        fbe_medic_get_action_priority(FBE_MEDIC_ACTION_REBUILD, &current_priority);
        highest_priority = fbe_medic_get_highest_priority(highest_priority, current_priority);
    }

    /* Check whether the raid group is verifying. */
    if (fbe_raid_group_background_op_is_metadata_verify_need_to_run(raid_group_p, FBE_RAID_BITMAP_VERIFY_ALL) ||
        fbe_raid_group_background_op_is_journal_verify_need_to_run(raid_group_p))
    {
        fbe_medic_get_action_priority(FBE_MEDIC_ACTION_METADATA_VERIFY, &current_priority);
        highest_priority = fbe_medic_get_highest_priority(highest_priority, current_priority);
    }
    else if (!fbe_raid_group_is_degraded(raid_group_p))
    {
        fbe_lba_t                            ro_verify_checkpoint;
        fbe_lba_t                            rw_verify_checkpoint;
        fbe_lba_t                            error_verify_checkpoint;
        fbe_lba_t                            incomplete_write_verify_checkpoint;
        fbe_lba_t                            system_verify_checkpoint;        
        fbe_raid_group_nonpaged_metadata_t*  raid_group_non_paged_metadata_p = NULL;

        fbe_raid_group_lock(raid_group_p);
        fbe_base_config_get_nonpaged_metadata_ptr((fbe_base_config_t *)raid_group_p, (void **)&raid_group_non_paged_metadata_p);
        error_verify_checkpoint = raid_group_non_paged_metadata_p->error_verify_checkpoint; 
        rw_verify_checkpoint = raid_group_non_paged_metadata_p->rw_verify_checkpoint;   
        ro_verify_checkpoint = raid_group_non_paged_metadata_p->ro_verify_checkpoint;   
        incomplete_write_verify_checkpoint = raid_group_non_paged_metadata_p->incomplete_write_verify_checkpoint;
        system_verify_checkpoint = raid_group_non_paged_metadata_p->system_verify_checkpoint;
        fbe_raid_group_unlock(raid_group_p);

        /* check if verify checkpoint needs to be driven forward
         */
        if(error_verify_checkpoint != FBE_LBA_INVALID)
        {
            fbe_medic_get_action_priority(FBE_MEDIC_ACTION_ERROR_VERIFY, &current_priority);
            highest_priority = fbe_medic_get_highest_priority(highest_priority, current_priority);
        }

        if (rw_verify_checkpoint != FBE_LBA_INVALID)
        {
            fbe_medic_get_action_priority(FBE_MEDIC_ACTION_USER_VERIFY, &current_priority);
            highest_priority = fbe_medic_get_highest_priority(highest_priority, current_priority);
        }

        if (ro_verify_checkpoint != FBE_LBA_INVALID)
        {
            fbe_medic_get_action_priority(FBE_MEDIC_ACTION_RO_VERIFY, &current_priority);
            highest_priority = fbe_medic_get_highest_priority(highest_priority, current_priority);
        }

        if (incomplete_write_verify_checkpoint != FBE_LBA_INVALID)
        {
            fbe_medic_get_action_priority(FBE_MEDIC_ACTION_INCOMPLETE_WRITE_VERIFY, &current_priority);
            highest_priority = fbe_medic_get_highest_priority(highest_priority, current_priority);
        }

        if (system_verify_checkpoint != FBE_LBA_INVALID)
        {
            fbe_medic_get_action_priority(FBE_MEDIC_ACTION_SYSTEM_VERIFY, &current_priority);
            highest_priority = fbe_medic_get_highest_priority(highest_priority, current_priority);
        } 

        if (fbe_raid_group_io_rekeying(raid_group_p))
        {
            fbe_medic_get_action_priority(FBE_MEDIC_ACTION_ENCRYPTION_REKEY, &current_priority);
            highest_priority = fbe_medic_get_highest_priority(highest_priority, current_priority);
        }        
    }

    *medic_action_priority = highest_priority;
    return FBE_STATUS_OK;
}
/************************************************
 * end fbe_raid_group_get_medic_action_priority()
 ************************************************/

/*!****************************************************************************
 * fbe_raid_group_set_upstream_medic_action_priority()
 ******************************************************************************
 * @brief
 *   This function sets medic action priority for the raid group on the 
 *   upstream edge based on the raid group's medic priority and the 
 *   downstream medic priority
 *
 * @param in_raid_group_p           - pointer to a raid group
 *
 * @return fbe_status_t   
 *
 * @author
 *   02/19/2015 - Created. Deanna Heng
 *
 ******************************************************************************/
fbe_status_t fbe_raid_group_set_upstream_medic_action_priority(fbe_raid_group_t *raid_group_p)
{
    fbe_medic_action_priority_t raid_group_medic_priority, downstream_medic_priority;
    fbe_status_t status = FBE_STATUS_OK;

    downstream_medic_priority = fbe_base_config_get_downstream_edge_priority((fbe_base_config_t*)raid_group_p);
    fbe_raid_group_get_medic_action_priority(raid_group_p, &raid_group_medic_priority);
    if (downstream_medic_priority > raid_group_medic_priority)
    {
        status = fbe_base_config_set_upstream_edge_priority((fbe_base_config_t*)raid_group_p, downstream_medic_priority);
        if (status != FBE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t *)raid_group_p, 
                              FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s set upstream from ds: %d failed - status: 0x%x\n",
                              __FUNCTION__, downstream_medic_priority, status);
        }
    }
    else
    {
        status = fbe_base_config_set_upstream_edge_priority((fbe_base_config_t*)raid_group_p, raid_group_medic_priority);
        if (status != FBE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t *)raid_group_p, 
                              FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s set upstream from rg: %d failed - status: 0x%x\n",
                              __FUNCTION__, raid_group_medic_priority, status);
        }
    }

    return status;
}
/************************************************
 * end fbe_raid_group_set_upstream_medic_action_priority()
 ************************************************/


/*!****************************************************************************
 * fbe_raid_group_check_medic_action_priority()
 ******************************************************************************
 * @brief
 *   This function checks medic action priority between the raid group
 * and the event.
 *
 * @param raid_group_p           - pointer to a raid group
 * @param event_p                - pointer to an event
 *
 * @return FBE_TRUE if the event has higher priority.   
 *
 * @author
 *   02/29/2012 - Created. chenl6
 *
 ******************************************************************************/
static fbe_bool_t
fbe_raid_group_check_medic_action_priority(fbe_raid_group_t * raid_group_p,
                                           fbe_event_t * event_p)
{
    fbe_bool_t                  b_allow_new_action = FBE_TRUE;
    fbe_medic_action_priority_t raid_group_priority, event_priority;

    fbe_event_get_medic_action_priority(event_p, &event_priority);
    if (event_priority == FBE_MEDIC_ACTION_HIGHEST_PRIORITY)
    {
        /* This is the case when the event has a default priority.
         * That means we don't need to check for RG priority. 
         */
        return FBE_TRUE;
    }

    fbe_raid_group_get_medic_action_priority(raid_group_p, &raid_group_priority);

    if (event_priority < raid_group_priority)
    {
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                              FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s Permission denied: event_priority %d, rg_priority %d\n", 
                              __FUNCTION__, event_priority, raid_group_priority);
        b_allow_new_action = FBE_FALSE;
    }

    /* If enabled trace some information.
     */
    fbe_raid_group_trace(raid_group_p,
                         FBE_TRACE_LEVEL_INFO,
                         FBE_RAID_GROUP_DEBUG_FLAG_MEDIC_ACTION_TRACING,
                         "raid_group: new action: %d pri: %d cur pri: %d granted: %d \n",
                         event_p->event_data.permit_request.permit_event_type,
                         event_priority, raid_group_priority, b_allow_new_action);

    /* Return if the the new action is allowed or not.
     */
    return b_allow_new_action;
}
/************************************************
 * end fbe_raid_group_check_medic_action_priority()
 ************************************************/

/*!***************************************************************************
 *          fbe_raid_group_is_medic_action_allowable()
 *****************************************************************************
 *
 * @brief   This function checks if the medic action requested is `allowable' 
 *          based on:
 *              o The action requested
 *              o The raid group health
 *
 * @param   raid_group_p           - pointer to a raid group
 * @param   event_p                - pointer to an event
 *
 * @return  bool - FBE_TRUE - The medic action is allowable.
 *                 FBE_FALSE - Due to the health etc, of the raid group, the
 *                  specific medic action is currently not allowable.
 *
 * @author
 *  06/01/2012  Ron Proulx  - Created.
 *
 *****************************************************************************/
static fbe_bool_t fbe_raid_group_is_medic_action_allowable(fbe_raid_group_t * raid_group_p,
                                                           fbe_event_t * event_p)
{
    fbe_bool_t                  b_allow_new_action = FBE_TRUE;
    fbe_bool_t                  b_is_raid_group_degraded = FBE_FALSE;
    fbe_bool_t                  b_is_lifecycle_state_ready = FBE_TRUE;
    fbe_lifecycle_state_t       lifecycle_state;
    fbe_medic_action_priority_t event_priority;

    /* Get the priority (action) of the request.
     */
    fbe_event_get_medic_action_priority(event_p, &event_priority);
    if (event_priority == FBE_MEDIC_ACTION_HIGHEST_PRIORITY)
    {
        /* This is the case when the event has a default priority.
         * That means we don't need to check for RG health.
         */
        return FBE_TRUE;
    }

    /* Check if the raid group is degraded or not.
     */
    b_is_raid_group_degraded = fbe_raid_group_is_degraded(raid_group_p);

    /* Check the lifecycle state of the raid group.
     */
    fbe_base_object_get_lifecycle_state((fbe_base_object_t *)raid_group_p, &lifecycle_state);
    if ((lifecycle_state != FBE_LIFECYCLE_STATE_READY) &&
        (lifecycle_state != FBE_LIFECYCLE_STATE_HIBERNATE))
    {
        b_is_lifecycle_state_ready = FBE_FALSE;
    }

    /* Certain medic actionS will not be allowed based on raid group health etc.
     */
    switch(event_priority)
    {
        /*! @note Currently only copy is denied based on raid group health.
         */
        case FBE_MEDIC_ACTION_COPY:
            /* The above actions will be denied if the raid group is degraded.
             */
            if ((b_is_raid_group_degraded == FBE_TRUE)    ||
                (b_is_lifecycle_state_ready == FBE_FALSE)    )
            {
                b_allow_new_action = FBE_FALSE;
            }
            break;

        default:
            /* All other actions are allowed even if the raid group is degraded.
             */
            break;
    }

    /* Trace some information if enabled and denied.
     */
    if (b_allow_new_action == FBE_FALSE)
    {
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                              FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s Permission denied: type: %d pri: %d degraded: %d ready: %d\n", 
                              __FUNCTION__, event_p->event_data.permit_request.permit_event_type, 
                              event_priority, b_is_raid_group_degraded, b_is_lifecycle_state_ready);

        /* If enabled trace some information.
         */
        fbe_raid_group_trace(raid_group_p,
                         FBE_TRACE_LEVEL_INFO,
                         FBE_RAID_GROUP_DEBUG_FLAG_MEDIC_ACTION_TRACING,
                         "raid_group: type: %d pri: %d degraded: %d ready: %d granted: %d \n",
                         event_p->event_data.permit_request.permit_event_type, 
                         event_priority, b_is_raid_group_degraded, 
                         b_is_lifecycle_state_ready, b_allow_new_action);
    }

    /* Return if the the new action is allowed or not.
     */
    return b_allow_new_action;
}
/************************************************
 * end fbe_raid_group_is_medic_action_allowable()
 ************************************************/


/*****************************************************
 * end fbe_raid_group_send_checkpoint_notification()
 *****************************************************/

/******************************************
 * end fbe_raid_group_set_sig_eval_cond()
 ******************************************/
/*!**************************************************************
 * fbe_raid_group_attribute_changed()
 ****************************************************************
 * @brief
 *  Handle the edge attribute changing.
 *  Currently we try to propagate the medic priority upstream.
 *
 * @param raid_group_p -
 * @param event_context -               
 *
 * @return fbe_status_t 
 *
 * @author
 *  4/3/2012 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_raid_group_attribute_changed(fbe_raid_group_t* const raid_group_p,
                                              fbe_event_context_t     event_context)
{
    fbe_status_t                status;
    fbe_medic_action_priority_t ds_medic_priority;
    fbe_medic_action_priority_t rg_medic_priority;
    fbe_medic_action_priority_t highest_medic_priority;
    fbe_class_id_t              class_id;
    fbe_bool_t                  raid_group_degraded;

    /* This method was not intended for the virtual drive.
     */
    if (fbe_raid_group_get_class_id(raid_group_p) == FBE_CLASS_ID_VIRTUAL_DRIVE)
    {
        fbe_block_edge_t *  edge_p = NULL;
        fbe_u32_t           edge_index = 0;

        for (edge_index = 0; edge_index < ((fbe_base_config_t *)raid_group_p)->width; edge_index++){
            fbe_base_config_get_block_edge(((fbe_base_config_t *)raid_group_p), &edge_p, edge_index);

             if((edge_p != NULL) && (((fbe_base_edge_t *)edge_p)->path_attr & FBE_BLOCK_PATH_ATTR_FLAGS_KEYS_REQUIRED)) {
                fbe_block_transport_server_set_path_attr_all_servers(&((fbe_base_config_t *)raid_group_p)->block_transport_server,
                                                             FBE_BLOCK_PATH_ATTR_FLAGS_KEYS_REQUIRED);

                /* It is enough to have one downstream edge in desparate key need to notify all the clients */
                return FBE_STATUS_OK;                
             }
        } /* Iterate throu edges */


        return FBE_STATUS_OK;
    }

    ds_medic_priority = fbe_base_config_get_downstream_edge_priority((fbe_base_config_t*)raid_group_p);
    fbe_raid_group_get_medic_action_priority(raid_group_p, &rg_medic_priority);
    
    highest_medic_priority = fbe_medic_get_highest_priority(rg_medic_priority, ds_medic_priority);

    fbe_base_object_trace((fbe_base_object_t*)raid_group_p, 
                          FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO, 
                          "raid_group: attr changed set priority: %d rg priority: %d downstream priority: %d\n", 
                          highest_medic_priority, rg_medic_priority, ds_medic_priority);
    status = fbe_base_config_set_upstream_edge_priority((fbe_base_config_t*)raid_group_p, highest_medic_priority);

    status = fbe_raid_group_attribute_changed_for_slf(raid_group_p, event_context);

    /*propagate the degraded attribute to LUN if attaching edge*/
    class_id = fbe_raid_group_get_class_id(raid_group_p);
    raid_group_degraded = fbe_raid_group_is_degraded(raid_group_p);
    if(FBE_CLASS_ID_VIRTUAL_DRIVE != class_id && FBE_TRUE == raid_group_degraded)
    {
        fbe_block_transport_server_set_path_attr_all_servers(&((fbe_base_config_t *)raid_group_p)->block_transport_server,
                                                             FBE_BLOCK_PATH_ATTR_FLAGS_DEGRADED);
    }

    return status;
}
/******************************************
 * end fbe_raid_group_attribute_changed()
 ******************************************/

/*!**************************************************************
 * fbe_raid_group_peer_lost_handle_outstanding_paged_metadata_write()
 ****************************************************************
 * @brief
 * This function is used to trigger paged metdata incomplete write verify if required in
 * case of peer death. The peer went away and it might be in middle of writing 
 * paged region. This function checks if paged metadata region stripe locks 
 * are held and if so, there could be writes outstanding. In this case it
 * kick off incomplete write verify
 *
 * @param raid_group_p - Raid group object.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  04/10/2012 - Created. Prahlad Purohit
 *
 ****************************************************************/
static fbe_status_t
fbe_raid_group_peer_lost_handle_outstanding_paged_metadata_write(fbe_raid_group_t * const raid_group_p)
{
    fbe_metadata_element_t  *metadata_element_p = &raid_group_p->base_config.metadata_element;
    fbe_bool_t               peer_held_metadata_lock = FALSE;
    fbe_bool_t               is_verify_needed = FALSE;
    fbe_bool_t               is_rebuild_needed = FALSE;
    fbe_raid_geometry_t*     geo_p = NULL;

    geo_p = fbe_raid_group_get_raid_geometry(raid_group_p);

    if ((((fbe_base_object_t *)raid_group_p)->class_id == FBE_CLASS_ID_VIRTUAL_DRIVE) ||
        (geo_p->raid_type == FBE_RAID_GROUP_TYPE_RAID10))
    {
        return FBE_STATUS_OK;
    }
    /* Determine if the metdata stripe range was locked by peer when it died.
     * If it did hold a lock in metadata range then we need to reconstruct pmd.
     */
    peer_held_metadata_lock = fbe_stripe_lock_was_paged_slot_held_by_peer(metadata_element_p);

    is_verify_needed = fbe_raid_group_is_verify_needed(raid_group_p);
    is_rebuild_needed = fbe_raid_group_is_degraded(raid_group_p);

    /* The stripe lock are sticky locks in that even though writes might not
     * be outstanding, it will be owned by the SP. So we want to check if 
     * the locks is owned by the peer that went away AND if there COULD be any
     * paged operation outstanding by looking at the checkpoints */
    /*! @note The following check should consider all checkpoints in non paged
     *  metadata region. If there are new checkpoints introduced this must be
     *  updated for same.
     */
    if(peer_held_metadata_lock && (is_verify_needed || is_rebuild_needed))
    {
        //  Set the condition to mark metdata region for incomplete write verify.
        fbe_lifecycle_set_cond(&fbe_raid_group_lifecycle_const, 
                               (fbe_base_object_t *) raid_group_p,
                               FBE_RAID_GROUP_LIFECYCLE_COND_MD_MARK_INCOMPLETE_WRITE);
    }

    return FBE_STATUS_OK;
}// End fbe_raid_group_peer_lost_handle_outstanding_paged_metadata_write

/*!**************************************************************
 * fbe_raid_group_peer_lost_handle_encryption_iw()
 ****************************************************************
 * @brief
 *  Decide if we need to handle this incomplete write during encryption.
 *
 * @param raid_group_p
 * 
 * @return fbe_status_t
 *
 * @author
 *  1/9/2014 - Created. Rob Foley
 *
 ****************************************************************/

static fbe_status_t
fbe_raid_group_peer_lost_handle_encryption_iw(fbe_raid_group_t * const raid_group_p)
{
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_group_get_raid_geometry(raid_group_p);
    
    if (!fbe_raid_geometry_is_raid10(raid_geometry_p) &&
        (fbe_raid_group_encryption_is_active(raid_group_p) ||
         fbe_raid_group_is_encryption_state_set(raid_group_p, FBE_RAID_GROUP_ENCRYPTION_FLAG_PAGED_NEEDS_RECONSTRUCT))){
        /* Evaluate the condition where we might need to handle the incomplete write.
         */
        fbe_lifecycle_set_cond(&fbe_raid_group_lifecycle_const, 
                               (fbe_base_object_t *) raid_group_p,
                               FBE_RAID_GROUP_LIFECYCLE_COND_JOURNAL_FLUSH);
    }
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_raid_group_peer_lost_handle_encryption_iw()
 ******************************************/

/*!**************************************************************
 * fbe_raid_group_event_peer_contact_lost()
 ****************************************************************
 * @brief
 *  This function processes peer contact lost event for raid group.
 *
 * @param raid_group_p - Raid group object which has received event.
 * @param event_context - The context for the event.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  04/03/2012 - Created. Prahlad Purohit
 *
 ****************************************************************/
static fbe_status_t
fbe_raid_group_event_peer_contact_lost(fbe_raid_group_t * const raid_group_p, 
                                       fbe_event_context_t event_context)
{
    fbe_status_t    status;
    
    fbe_raid_group_eval_peer_slf_update(raid_group_p);
    fbe_raid_group_peer_lost_handle_encryption_iw(raid_group_p);
    status = fbe_raid_group_peer_lost_handle_outstanding_paged_metadata_write(raid_group_p);

    return status;
} // End fbe_raid_group_event_peer_contact_lost

/*!**************************************************************
 * fbe_raid_group_event_check_slf_attr()
 ****************************************************************
 * @brief
 *  This is the handler for a change in the edge attribute.
 *
 * @param raid_group_p - pointer to the raid group.
 * @param edge_p - pointer to the block edge.
 * @param b_set - whether to set the flag.
 * @param b_clear - whether to clear the flag.
 *
 * @return - Status of the handling.
 *
 * @author
 *  09/06/2012 - Created. Lili Chen
 *
 ****************************************************************/
fbe_status_t 
fbe_raid_group_event_check_slf_attr(fbe_raid_group_t * raid_group_p, 
                                    fbe_block_edge_t * edge_p,
                                    fbe_bool_t *b_set, fbe_bool_t *b_clear)
{
    fbe_path_state_t path_state;
    fbe_path_attr_t path_attr;
    fbe_u16_t total_slf_drives = 0;
    fbe_u32_t position, width;

    *b_set = *b_clear = FBE_FALSE;
    fbe_block_transport_get_path_state(edge_p, &path_state);
    fbe_block_transport_get_path_attributes(edge_p, &path_attr);
    /* Check the edge with the event */
    if ((path_state != FBE_PATH_STATE_INVALID) &&
        (path_attr & FBE_BLOCK_PATH_ATTR_FLAGS_PATH_NOT_PREFERRED))
    {
        *b_set = FBE_TRUE;
        return FBE_STATUS_OK;
    }

    /* Scan the block edges for slf drives */
    fbe_raid_group_get_width(raid_group_p, &width);
    for (position = 0; position < width; position++)
    {
        fbe_base_config_get_block_edge((fbe_base_config_t*)raid_group_p, &edge_p, position);
        fbe_block_transport_get_path_state(edge_p, &path_state);
        fbe_block_transport_get_path_attributes(edge_p, &path_attr);
        if ((path_state != FBE_PATH_STATE_INVALID) &&
            (path_attr & FBE_BLOCK_PATH_ATTR_FLAGS_PATH_NOT_PREFERRED))
        {
            total_slf_drives++;
        }
    }

    if (total_slf_drives == 0)
    {
        *b_clear = FBE_TRUE;
    }

    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_raid_group_event_check_slf_attr()
 ******************************************/

/*!**************************************************************
 * fbe_raid_group_attribute_changed_for_slf()
 ****************************************************************
 * @brief
 *  This is the handler for a change in the edge attribute.
 *
 * @param raid_group_p - raid group that is changing.
 * @param event_context - The context for the event.
 *
 * @return - Status of the handling.
 *
 * @author
 *  4/30/2012 - Created. Lili Chen
 *
 ****************************************************************/
fbe_status_t 
fbe_raid_group_attribute_changed_for_slf(fbe_raid_group_t * raid_group_p, 
                                         fbe_event_context_t event_context)
{
    fbe_block_edge_t *edge_p = (fbe_block_edge_t *)event_context;
    fbe_bool_t b_set, b_clear;

    /* Don't bother if the edge is just attached..*/
    if (edge_p == NULL)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Set or clear SLF clustered flag */
    fbe_raid_group_lock(raid_group_p);
    fbe_raid_group_event_check_slf_attr(raid_group_p, edge_p, &b_set, &b_clear);
    if (b_set)
    {
        fbe_raid_group_set_clustered_flag(raid_group_p, FBE_RAID_GROUP_CLUSTERED_FLAG_SLF_REQ);
    }
    else if (b_clear)
    {
        fbe_raid_group_clear_clustered_flag(raid_group_p, FBE_RAID_GROUP_CLUSTERED_FLAG_SLF_REQ);
    }
    fbe_raid_group_unlock(raid_group_p);

    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_raid_group_attribute_changed_for_slf()
 ******************************************/

/*!**************************************************************
 * fbe_raid_group_eval_peer_slf_update()
 ****************************************************************
 * @brief
 *  This function handles clustered memory update for slf.
 *
 * @param raid_group_p - raid group that is changing.
 *
 * @return - Status of the handling.
 *
 * @author
 *  4/30/2012 - Created. Lili Chen
 *
 ****************************************************************/
fbe_status_t 
fbe_raid_group_eval_peer_slf_update(fbe_raid_group_t * raid_group_p)
{
    if (((fbe_base_object_t *)raid_group_p)->class_id == FBE_CLASS_ID_VIRTUAL_DRIVE)
    {
        return FBE_STATUS_OK;
    }

    /* only passive side take action on the flag */
    if (fbe_base_config_is_active((fbe_base_config_t*)raid_group_p))
    {
        return FBE_STATUS_OK;
    }

    if (fbe_raid_group_is_peer_clustered_flag_set(raid_group_p, FBE_RAID_GROUP_CLUSTERED_FLAG_SLF_PEER_REQ))
    {
        /* Set global_path_attr */
        fbe_base_config_set_global_path_attr((fbe_base_config_t *)raid_group_p, FBE_BLOCK_PATH_ATTR_FLAGS_PATH_NOT_PREFERRED);
        /* Set edge attr */
        fbe_block_transport_server_propagate_path_attr_with_mask_all_servers( 
                    &((fbe_base_config_t *)raid_group_p)->block_transport_server, 
                    FBE_BLOCK_PATH_ATTR_FLAGS_PATH_NOT_PREFERRED, 
                    FBE_BLOCK_PATH_ATTR_FLAGS_PATH_NOT_PREFERRED);
    }
    /* If we have SLF_REQ flag set this SP but not peer SP, we don't want to clear upstream NOT_PREFERRED attr,
     * since the peer (active side) may have not made the decision yet.
     */
    else if (fbe_base_config_is_global_path_attr_set((fbe_base_config_t *)raid_group_p, FBE_BLOCK_PATH_ATTR_FLAGS_PATH_NOT_PREFERRED) &&
             (!fbe_raid_group_is_clustered_flag_set(raid_group_p, FBE_RAID_GROUP_CLUSTERED_FLAG_SLF_REQ) ||
              fbe_raid_group_is_peer_clustered_flag_set(raid_group_p, FBE_RAID_GROUP_CLUSTERED_FLAG_SLF_REQ)))
    {
        /* Clear global_path_attr */
        fbe_base_config_clear_global_path_attr((fbe_base_config_t *)raid_group_p, FBE_BLOCK_PATH_ATTR_FLAGS_PATH_NOT_PREFERRED);
        /* Clear edge attr */
        fbe_block_transport_server_propagate_path_attr_with_mask_all_servers( 
                    &((fbe_base_config_t *)raid_group_p)->block_transport_server, 
                    0, 
                    FBE_BLOCK_PATH_ATTR_FLAGS_PATH_NOT_PREFERRED);
    }

    return FBE_STATUS_OK;
}

/*!****************************************************************************
 * fbe_raid_group_handle_download_request_event_entry()
 ******************************************************************************
 * @brief
 *   This function is used to handle download trial run request from PVD.
 *
 * @param in_raid_group_p       - pointer to a raid group object
 * @param in_event_p            - pointer to an event 
 *
 * @return fbe_status_t   
 *
 * @author
 *   07/30/2011 - Created. Lili Chen
 *   03/19/2014 - Allow R0 downloads
 *
 ******************************************************************************/
static fbe_status_t fbe_raid_group_handle_download_request_event_entry(
                                                fbe_raid_group_t*       in_raid_group_p,
                                                fbe_event_t*            in_event_p)
{
    fbe_base_config_downstream_health_state_t   downstream_health;
                                                                               
    downstream_health = fbe_raid_group_verify_downstream_health(in_raid_group_p);
    
    if (downstream_health != FBE_DOWNSTREAM_HEALTH_OPTIMAL ||
        fbe_raid_group_is_degraded(in_raid_group_p))
    {
        fbe_base_object_trace((fbe_base_object_t *)in_raid_group_p,
                              FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s trial run request rejected\n", __FUNCTION__);
        fbe_event_set_flags(in_event_p, FBE_EVENT_FLAG_DENY);
    }

    fbe_event_set_status(in_event_p, FBE_EVENT_STATUS_OK);
    fbe_event_complete(in_event_p);
    return FBE_STATUS_OK;
}
/*****************************************************
 * end fbe_raid_group_handle_download_request_event_entry()
 *****************************************************/

/*!**************************************************************
 * fbe_raid_group_send_checkpoint_notification()
 ****************************************************************
 * @brief
 *  Handle a notification that the peer moved the checkpoint.
 *  We will check to see what operation is ongoing and
 *  send the notification for this.
 *
 * @param raid_group_p - ptr to Raid Group.            
 *
 * @return fbe_status_t
 *
 * @author
 *  3/20/2012 - Created. Rob Foley
 *
 ****************************************************************/
static fbe_status_t fbe_raid_group_send_checkpoint_notification(fbe_raid_group_t * raid_group_p)
{

    
    fbe_status_t status;

    /*since the event comes at DPC context via CMI and we'll have to do some packet alloactions.
     let's delegate theat to the monitor
     */
     status = fbe_lifecycle_set_cond(&fbe_raid_group_lifecycle_const,
                                     (fbe_base_object_t *) raid_group_p,
                                     FBE_BASE_CONFIG_LIFECYCLE_COND_BACKGROUND_MONITOR_OPERATION);

     if (status != FBE_STATUS_OK) {
         fbe_base_object_trace((fbe_base_object_t*)raid_group_p, 
                                              FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO, 
                                              "%s failed to set condition \n", __FUNCTION__);

     }

    return status;
}
/*****************************************************
 * end fbe_raid_group_send_checkpoint_notification()
 *****************************************************/

/*!***************************************************************************
 *          fbe_raid_group_background_op_peer_died_cleanup()
 ***************************************************************************** 
 * 
 * @brief   The peer died.  Perform any cleanup related to any background
 *          service.  This SP will become Active for all background services.
 *
 * @param   raid_group_p - ptr to Raid Group.            
 *
 * @return  fbe_status_t
 * 
 * @note    This is purposefully invoked from the monitor context (as opposed
 *          to the event context) so that there is no overlap with the
 *          background monitor condition.
 *
 * @author
 *  03/28/2013  Ron Proulx  - Created.
 *
 *****************************************************************************/
fbe_status_t fbe_raid_group_background_op_peer_died_cleanup(fbe_raid_group_t *raid_group_p)
{
    fbe_raid_group_nonpaged_metadata_t *nonpaged_metadata_p;
    fbe_u32_t                           rebuild_index;
    fbe_lba_t                           blocks_rebuilt = 0;

    /* Clean up any background operations.
     */

    /* If the non-paged metadata is not initialized yet, then return the
     * default value with a status indicating such.
     */
    if (fbe_raid_group_is_nonpaged_metadata_initialized(raid_group_p) == FBE_FALSE)
    {
        return FBE_STATUS_NOT_INITIALIZED;
    }

    /* Get a pointer to the non-paged metadata.
     */
    fbe_base_config_get_nonpaged_metadata_ptr((fbe_base_config_t *)raid_group_p, (void **)&nonpaged_metadata_p);

    /* For any position that is rebuilding, reset the `blocks_rebuilt' to the
     * checkpoint value.
     */
    for (rebuild_index = 0; rebuild_index < FBE_RAID_GROUP_MAX_REBUILD_POSITIONS; rebuild_index++)
    {
        if (nonpaged_metadata_p->rebuild_info.rebuild_checkpoint_info[rebuild_index].position != FBE_RAID_INVALID_DISK_POSITION)
        {
            /* Only update the blocks rebuilt if the checkpoint is within the user area
             */
            if (nonpaged_metadata_p->rebuild_info.rebuild_checkpoint_info[rebuild_index].checkpoint <= fbe_raid_group_get_exported_disk_capacity(raid_group_p)) 
            {
                 blocks_rebuilt = nonpaged_metadata_p->rebuild_info.rebuild_checkpoint_info[rebuild_index].checkpoint;
            }

            fbe_raid_group_trace(raid_group_p,
                                 FBE_TRACE_LEVEL_INFO,
                                 FBE_RAID_GROUP_DEBUG_FLAG_REBUILD_NOTIFICATION_TRACING,
                                 "raid_group: RB peer died reset pos: %d index: %d blocks rebuilt from: 0x%llx to checkpoint: 0x%llx \n",
                                 nonpaged_metadata_p->rebuild_info.rebuild_checkpoint_info[rebuild_index].position,
                                 rebuild_index,
                                 (unsigned long long)raid_group_p->raid_group_metadata_memory.blocks_rebuilt[rebuild_index],
                                 (unsigned long long)blocks_rebuilt);
            raid_group_p->raid_group_metadata_memory.blocks_rebuilt[rebuild_index] = blocks_rebuilt;
        }
    }
   
    /* Return success.
     */
    return FBE_STATUS_OK;
}
/******************************************************
 * end fbe_raid_group_background_op_peer_died_cleanup()
 ******************************************************/

/*!**************************************************************
 * fbe_raid_group_check_power_save_state()
 ****************************************************************
 * @brief
 *  This function handles clustered memory update for hibernation.
 *
 * @param raid_group_p - raid group that is changing.
 *
 * @return - Status of the handling.
 *
 * @author
 *  4/22/2013 - Created. NCHIU
 *
 ****************************************************************/
fbe_status_t 
fbe_raid_group_check_power_save_state(fbe_raid_group_t * raid_group_p)
{
    fbe_lifecycle_state_t		my_state, peer_state;

    fbe_base_config_metadata_get_lifecycle_state((fbe_base_config_t *)raid_group_p, &my_state);
    fbe_base_config_metadata_get_peer_lifecycle_state((fbe_base_config_t *)raid_group_p, &peer_state);

    /* if peer is in hibernate state, but we are not, and we are passive, we need to follow, 
     * after we have synced with peer.
     */
    if ((peer_state == FBE_LIFECYCLE_STATE_HIBERNATE) &&
        (my_state != FBE_LIFECYCLE_STATE_HIBERNATE) &&
        !fbe_base_config_is_active((fbe_base_config_t *)raid_group_p) &&
        fbe_base_config_is_clustered_flag_set((fbe_base_config_t *)raid_group_p, FBE_BASE_CONFIG_CLUSTERED_FLAG_HIBERNATE_READY))
    {
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p,
                            FBE_TRACE_LEVEL_INFO,
                            FBE_TRACE_MESSAGE_ID_INFO,
                            "raid_group: Peer went into hibernation, passive side follows\n");
        /*we are good to go, everyone above us is hibernating*/
        fbe_lifecycle_set_cond(&fbe_raid_group_lifecycle_const, 
                            (fbe_base_object_t*)raid_group_p, 
                            FBE_BASE_OBJECT_LIFECYCLE_COND_HIBERNATE);
    }

    /* Return success.
     */
    return FBE_STATUS_OK;
}
/******************************************************
 * end fbe_raid_group_check_power_save_state()
 ******************************************************/

/*!**************************************************************
 * fbe_raid_group_set_change_config_cond()
 ****************************************************************
 * @brief
 *  This function looks at the raid group clustered bits
 *  and sees if we need to set the change config condition.
 *
 * @param raid_group_p - raid group ptr.               
 *
 * @return fbe_status_t
 *
 * @author
 *  10/23/2013 - Created. Rob Foley
 *
 ****************************************************************/

static fbe_status_t fbe_raid_group_set_change_config_cond(fbe_raid_group_t *raid_group_p)
{
    fbe_raid_group_lock(raid_group_p);

    /* If the peer requested that we chane config,
     * and we have not acknowledged this yet by setting the request flag, 
     * then we need to also set the condition.
     */
    if (fbe_raid_group_is_peer_clustered_flag_set(raid_group_p, FBE_RAID_GROUP_CLUSTERED_FLAG_CHANGE_CONFIG_REQ) &&
        !fbe_raid_group_is_peer_clustered_flag_set(raid_group_p, FBE_RAID_GROUP_CLUSTERED_FLAG_CHANGE_CONFIG_STARTED) &&
        !fbe_raid_group_is_clustered_flag_set(raid_group_p, FBE_RAID_GROUP_CLUSTERED_FLAG_CHANGE_CONFIG_REQ)&&
        !fbe_raid_group_is_local_state_set(raid_group_p, FBE_RAID_GROUP_LOCAL_STATE_CHANGE_CONFIG_REQUEST))
        
    {
        fbe_bool_t b_is_active;

        fbe_raid_group_monitor_check_active_state(raid_group_p, &b_is_active);
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s set condition to change config local: 0x%llx peer: 0x%llx state: 0x%llx\n", 
                              b_is_active ? "Active" : "Passive",
                              (unsigned long long)raid_group_p->raid_group_metadata_memory.flags,
                              (unsigned long long)raid_group_p->raid_group_metadata_memory_peer.flags,
                              (unsigned long long)raid_group_p->local_state);

        fbe_raid_group_set_local_state(raid_group_p, FBE_RAID_GROUP_LOCAL_STATE_CHANGE_CONFIG_REQUEST);
        fbe_lifecycle_set_cond(&fbe_raid_group_lifecycle_const,
                               (fbe_base_object_t *) raid_group_p,
                               FBE_RAID_GROUP_LIFECYCLE_COND_CONFIGURATION_CHANGE);
    }
    fbe_raid_group_unlock(raid_group_p);
    
    return FBE_STATUS_OK;
}
/*************************************************
 * end fbe_raid_group_set_change_config_cond()
 *************************************************/
/**********************************
 * end file fbe_raid_group_event.c
 **********************************/


