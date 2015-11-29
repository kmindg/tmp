#include "fbe_base_config_private.h"
#include "fbe_topology.h"
#include "EmcPAL_Misc.h"

static fbe_status_t base_config_process_traffic_load_change(fbe_base_config_t *base_config_p);
static fbe_status_t base_config_event_process_incoming_io_during_hibernate(fbe_base_config_t *base_config_p);
static fbe_status_t base_config_event_peer_object_alive(fbe_base_config_t * base_config, fbe_event_context_t event_context);
static fbe_status_t base_config_event_check_passive(fbe_base_config_t * base_config);

static void base_config_process_memory_update_message(fbe_base_config_t * base_config);
static void base_config_process_peer_contact_lost(fbe_base_config_t * base_config);
static void base_config_process_peer_contact_lost_nonpaged_persist_pending(fbe_base_config_t * base_config);
static void base_config_process_peer_contact_lost_release_sl(fbe_base_config_t * base_config);

/*!***************************************************************
 * fbe_base_config_event_entry()
 ****************************************************************
 * @brief
 *  This function is called to pass an event to a given instance
 *  of the base config class.
 *
 * @param object_handle - The object receiving the event.
 * @param event_type - Type of event that is arriving. e.g. state change.
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
fbe_base_config_event_entry(fbe_object_handle_t object_handle, 
                              fbe_event_type_t event_type,
                              fbe_event_context_t event_context)
{
    fbe_base_config_t * base_config_p = NULL;
    fbe_status_t status;
    fbe_event_t * event = NULL;
    
    base_config_p = (fbe_base_config_t *)fbe_base_handle_to_pointer(object_handle);

    fbe_base_object_trace((fbe_base_object_t*)base_config_p,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry event_type %d context 0x%p\n",
                          __FUNCTION__, event_type, event_context);

    /* First handle the event we have received.
     */
    switch (event_type)
    {
        case FBE_EVENT_TYPE_EDGE_STATE_CHANGE:
            break;
        case FBE_EVENT_TYPE_DATA_REQUEST:
        case FBE_EVENT_TYPE_PERMIT_REQUEST:
        case FBE_EVENT_TYPE_SPARING_REQUEST:
        case FBE_EVENT_TYPE_COPY_REQUEST:
        case FBE_EVENT_TYPE_ABORT_COPY_REQUEST:
        case FBE_EVENT_TYPE_VERIFY_REPORT:
        case FBE_EVENT_TYPE_EVENT_LOG:    
            event = (fbe_event_t *)event_context;
            fbe_event_set_status(event, FBE_EVENT_STATUS_OK);
            fbe_event_complete(event);
            break;
        case FBE_EVENT_TYPE_EDGE_TRAFFIC_LOAD_CHANGE:
            base_config_process_traffic_load_change(base_config_p);
            break;
        case FBE_EVENT_TYPE_PEER_OBJECT_ALIVE:
            status = base_config_event_peer_object_alive(base_config_p, event_context);
            break;
        case FBE_EVENT_TYPE_PEER_MEMORY_UPDATE:
            base_config_process_memory_update_message(base_config_p);
            break;
        case FBE_EVENT_TYPE_PEER_NONPAGED_WRITE:
            fbe_lifecycle_reschedule(&fbe_base_config_lifecycle_const,
                                 (fbe_base_object_t *) base_config_p,
                                 (fbe_lifecycle_timer_msec_t) 0);    /* Immediate reschedule */	
            break;
        case FBE_EVENT_TYPE_PEER_NONPAGED_CHANGED:
            break;
        case FBE_EVENT_TYPE_ATTRIBUTE_CHANGED:
            break;
        case FBE_EVENT_TYPE_PEER_CONTACT_LOST:
            base_config_process_peer_contact_lost(base_config_p);
            break;
        default:
            status = FBE_STATUS_GENERIC_FAILURE;
            event = (fbe_event_t *)event_context;
            fbe_base_object_trace((fbe_base_object_t*)base_config_p,
                                  FBE_TRACE_LEVEL_WARNING,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                  "%s Uknown event event_type %d context 0x%p\n",
                                  __FUNCTION__, event_type, event_context);

            fbe_event_set_status(event, FBE_EVENT_STATUS_GENERIC_FAILURE);
            fbe_event_complete(event);

            break;

    }

    return FBE_STATUS_OK;
}

static fbe_status_t base_config_process_traffic_load_change(fbe_base_config_t *base_config_p)
{
    fbe_status_t	status = FBE_STATUS_GENERIC_FAILURE;

    status = fbe_lifecycle_set_cond(&fbe_base_config_lifecycle_const,
                                    (fbe_base_object_t*)base_config_p,
                                     FBE_BASE_CONFIG_LIFECYCLE_COND_TRAFFIC_LOAD_CHANGE);

    if (status != FBE_STATUS_OK) {
         fbe_base_object_trace((fbe_base_object_t*)base_config_p,
                                FBE_TRACE_LEVEL_WARNING,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                "%s can't set condition to propagate load change\n",__FUNCTION__ );

         return FBE_STATUS_GENERIC_FAILURE;

    }

    return FBE_STATUS_OK;

}

/*event from block transport itself, not the edges*/
fbe_status_t fbe_base_config_process_block_transport_event(fbe_block_transport_event_type_t event_type, fbe_block_trasnport_event_context_t context)
{
    fbe_base_config_t *base_config_p = (fbe_base_config_t *)context;

    switch(event_type) {
    case FBE_BLOCK_TRASPORT_EVENT_TYPE_IO_WAITING_ON_QUEUE:
        /*there is an incomming IO, we need to wake up the object and make sure once it's in ready, to send the IO*/
        base_config_event_process_incoming_io_during_hibernate(base_config_p);
        break;
    default:
        fbe_base_object_trace((fbe_base_object_t*)base_config_p,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s can't process event:%d from block transport\n",__FUNCTION__, event_type);

    }
    

    return FBE_STATUS_OK;
}

static fbe_status_t base_config_event_process_incoming_io_during_hibernate(fbe_base_config_t *base_config_p)
{
    fbe_status_t	status = FBE_STATUS_GENERIC_FAILURE;

    /*set a condition on the READY rotary once the object becomes alive*/
    status = fbe_lifecycle_set_cond(&fbe_base_config_lifecycle_const,
                                    (fbe_base_object_t*)base_config_p,
                                     FBE_BASE_CONFIG_LIFECYCLE_COND_DEQUEUE_PENDING_IO);

    if (status != FBE_STATUS_OK) {
         fbe_base_object_trace((fbe_base_object_t*)base_config_p,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s can't set condition to process IO after drive wakeup !\n",__FUNCTION__ );

         return FBE_STATUS_GENERIC_FAILURE;

    }

    return FBE_STATUS_OK;
}

static fbe_status_t
base_config_event_peer_object_alive(fbe_base_config_t * base_config, fbe_event_context_t event_context)
{
    fbe_status_t status;

    /* Allow to talk to peer first */	
    fbe_metadata_element_clear_peer_dead(&base_config->metadata_element);
    fbe_metadata_element_clear_sl_peer_lost(&base_config->metadata_element);
    fbe_metadata_element_clear_abort_monitor_ops(&base_config->metadata_element);

    if(fbe_base_config_is_active(base_config)) {
        /* Mark the metadata element so that we will know the peer had requested to synch with us. */
        fbe_metadata_element_set_nonpaged_request(&base_config->metadata_element);
    }

    /* All what we need to do is to update peer with our metadata memory */
    status = fbe_lifecycle_set_cond(&fbe_base_config_lifecycle_const, 
                                    (fbe_base_object_t*)base_config, 
                                    FBE_BASE_CONFIG_LIFECYCLE_COND_UPDATE_METADATA_MEMORY);

    if (status != FBE_STATUS_OK)
    {
         fbe_base_object_trace((fbe_base_object_t*)base_config,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s can't set condition \n",__FUNCTION__ );

        return FBE_STATUS_GENERIC_FAILURE;
    }
    

    return FBE_STATUS_OK;
}

/*!**************************************************************
 * base_config_event_check_join_state()
 ****************************************************************
 * @brief
 *  This function looks at the clustered bits and sees if
 *  we need to set join condition or manipulate our clustered bits
 *  in response to something the peer changed.
 *
 * @param base_config_p - object             
 *
 * @return fbe_status_t
 *
 * @author
 *  7/22/2011 - Created. Rob Foley
 *
 ****************************************************************/

static fbe_status_t 
base_config_event_check_join_state(fbe_base_config_t * base_config_p)
{
    fbe_bool_t b_peer_flag_set = FBE_FALSE;
    fbe_base_config_metadata_memory_t * metadata_memory_p = NULL;
    fbe_base_config_metadata_memory_t * metadata_memory_peer_p = NULL;
    fbe_bool_t b_is_active;
    fbe_lifecycle_state_t		my_state, peer_state;

    if(base_config_p->base_object.class_id == FBE_CLASS_ID_LUN)
    {
        fbe_base_object_trace((fbe_base_object_t*)base_config_p,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                              "LUN %s entry\n", __FUNCTION__);
    }

    b_is_active = fbe_base_config_is_active(base_config_p);
    fbe_base_config_metadata_get_lifecycle_state(base_config_p, &my_state);
    fbe_base_config_metadata_get_peer_lifecycle_state(base_config_p, &peer_state);

    /* Get a pointer to metadata memory */
    fbe_metadata_element_get_metadata_memory(&base_config_p->metadata_element, (void **)&metadata_memory_p);
    fbe_metadata_element_get_peer_metadata_memory(&base_config_p->metadata_element, (void **)&metadata_memory_peer_p);

    fbe_base_config_lock(base_config_p);
    b_peer_flag_set = fbe_base_config_is_peer_clustered_flag_set(base_config_p, FBE_BASE_CONFIG_CLUSTERED_FLAG_JOIN_REQ);

    /* If the peer requested a join, then set our join condition if we are the active side.
     * This condition is only needed on the active side. 
     * There's a window that active side clears JOIN_REQ and peer hasn't, we don't want to 
     * restart JOIN process if we've already done so.  
     * The logic after this makes sure JOINED flag is cleared whenever necessary.
     */
    if (b_peer_flag_set &&
        !fbe_base_config_is_any_clustered_flag_set(base_config_p,(FBE_BASE_CONFIG_CLUSTERED_FLAG_JOIN_REQ |
                                                                  FBE_BASE_CONFIG_CLUSTERED_FLAG_JOINED)))
    {
        if (b_is_active)
        {
            fbe_base_object_trace((fbe_base_object_t*)base_config_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s set join cond local: 0x%llx peer: 0x%llx\n", 
                                  b_is_active ? "Active" : "Passive",
                                  (unsigned long long)metadata_memory_p->flags,
                                  (unsigned long long)metadata_memory_peer_p->flags);
            fbe_lifecycle_set_cond(&fbe_base_config_lifecycle_const,
                                   (fbe_base_object_t *) base_config_p,
                                   FBE_BASE_CONFIG_LIFECYCLE_COND_ACTIVE_ALLOW_JOIN);
        }
        else 
        {
            if(base_config_p->base_object.class_id == FBE_CLASS_ID_LUN)
            {
                fbe_base_object_trace((fbe_base_object_t*)base_config_p,
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                  "LUN %s is not active\n", __FUNCTION__);
            }
        }
    }
    else
    {
        if(base_config_p->base_object.class_id == FBE_CLASS_ID_LUN)
        {
            fbe_base_object_trace((fbe_base_object_t*)base_config_p,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                              "LUN %s peer flag: %s \n", __FUNCTION__, b_peer_flag_set ? "Set" : "Not Set");
        }
    }

    /* If we see that the peer has joined with us, it is necessary for us to immediately set joined here. 
     * This is required since when this is set the peer might now start to coordinate with us. 
     * Which means that a condition that is higher priority than the join condition could get set 
     * in response to a message from the peer and that condition needs to coordinate with the peer. 
     */
    if (fbe_base_config_is_peer_clustered_flag_set(base_config_p, FBE_BASE_CONFIG_CLUSTERED_FLAG_JOINED) &&
        fbe_base_config_is_clustered_flag_set(base_config_p, FBE_BASE_CONFIG_CLUSTERED_FLAG_JOIN_SYNCING) &&
        !fbe_base_config_is_clustered_flag_set(base_config_p, FBE_BASE_CONFIG_CLUSTERED_FLAG_JOINED))
    {
        fbe_base_object_trace((fbe_base_object_t*)base_config_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "peer has joined: set joined flag local: 0x%llx peer: 0x%llx\n",
                              (unsigned long long)metadata_memory_p->flags,
                  (unsigned long long)metadata_memory_peer_p->flags);
        fbe_base_config_set_clustered_flag(base_config_p, FBE_BASE_CONFIG_CLUSTERED_FLAG_JOINED);
        /* Now that we joined, clear JOIN_REQ so that we don't accidentally start JOIN again */
        fbe_base_config_clear_clustered_flag(base_config_p,FBE_BASE_CONFIG_CLUSTERED_FLAG_JOIN_REQ);
    }
    else if (fbe_base_config_is_clustered_flag_set(base_config_p, FBE_BASE_CONFIG_CLUSTERED_FLAG_JOINED) &&
             !fbe_base_config_is_clustered_flag_set(base_config_p, FBE_BASE_CONFIG_CLUSTERED_FLAG_JOIN_SYNCING) &&
             fbe_base_config_is_peer_clustered_flag_set(base_config_p, FBE_BASE_CONFIG_CLUSTERED_FLAG_JOIN_REQ) &&
             !fbe_base_config_is_any_peer_clustered_flag_set(base_config_p, (FBE_BASE_CONFIG_CLUSTERED_FLAG_JOIN_STARTED|
                                                                             FBE_BASE_CONFIG_CLUSTERED_FLAG_JOIN_SYNCING)))
    {
        /* If we think the peer is there, and we are not in the midst of a join, and they are attempting to rejoin, then
         * clear our joined flag. 
         */
        fbe_base_object_trace((fbe_base_object_t*)base_config_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "peer is rejoining: clear joined flag local: 0x%llx peer: 0x%llx\n",
                              (unsigned long long)metadata_memory_p->flags,
                  (unsigned long long)metadata_memory_peer_p->flags);
        fbe_base_config_clear_clustered_flag(base_config_p, FBE_BASE_CONFIG_CLUSTERED_FLAG_JOINED);
    }
    else if (fbe_base_config_is_clustered_flag_set(base_config_p, FBE_BASE_CONFIG_CLUSTERED_FLAG_JOINED) &&
             !fbe_base_config_is_any_peer_clustered_flag_set(base_config_p, (FBE_BASE_CONFIG_CLUSTERED_FLAG_JOINED|
                                                                             FBE_BASE_CONFIG_CLUSTERED_FLAG_JOIN_SYNCING)) ) 
    {
        /* If the Peer Joined flag has cleared and the local Joined flag still set,
         * then clear the local Joined Flag and request for the Passive Join cond. 
         */
        fbe_base_object_trace((fbe_base_object_t*)base_config_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "bc_set_join_cond: %s-Clear flag- local=0x%llx, lstate=0x%x, peer=0x%llx, pstate=0x%x\n",
                              b_is_active ? "Active" : "Passive",
                              (unsigned long long)metadata_memory_p->flags,
                  metadata_memory_p->lifecycle_state, 
                              (unsigned long long)metadata_memory_peer_p->flags,
                  metadata_memory_peer_p->lifecycle_state);

        /* Clear the local JOINED flag */
        fbe_base_config_clear_clustered_flag(base_config_p, FBE_BASE_CONFIG_CLUSTERED_FLAG_JOINED);  

        /* If this is not Active side and the local lifecycle state is ready and the peer is in the pending activate,
         * then set request passive Join cond. Note, for other pending cases (hibernate, fail, and offline,
         * we do not see the problem and the passive will be joined again once it is out of the pending hibernate, 
         * fail, or offline state. 
         * If peer is failed, PVD needs to follow.
         */ 
        if ((!b_is_active) && 
            ((my_state == FBE_LIFECYCLE_STATE_READY) &&
             ((peer_state == FBE_LIFECYCLE_STATE_ACTIVATE)||
              (peer_state == FBE_LIFECYCLE_STATE_FAIL)))) 
        {    
            fbe_base_object_trace((fbe_base_object_t*)base_config_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                  "bc_set_join_cond: %s-Set Passive Req Join-local=0x%llx, lstate=0x%x, peer=0x%llx, pstate=0x%x\n",
                                  b_is_active ? "Active" : "Passive",
                                  (unsigned long long)metadata_memory_p->flags, my_state, 
                                  (unsigned long long)metadata_memory_peer_p->flags, peer_state);

            fbe_lifecycle_set_cond(&fbe_base_config_lifecycle_const,
                                   (fbe_base_object_t *) base_config_p,
                                   FBE_BASE_CONFIG_LIFECYCLE_COND_PASSIVE_REQUEST_JOIN);    
        }
    }
    else if ((fbe_base_config_is_clustered_flag_set(base_config_p, FBE_BASE_CONFIG_CLUSTERED_FLAG_JOIN_SYNCING) &&
             !fbe_base_config_is_any_peer_clustered_flag_set(base_config_p, (FBE_BASE_CONFIG_CLUSTERED_FLAG_JOIN_STARTED|
                                                                             FBE_BASE_CONFIG_CLUSTERED_FLAG_JOIN_SYNCING |
                                                                             FBE_BASE_CONFIG_CLUSTERED_FLAG_JOIN_REQ)) ) )
    {
        /* If this is not Active side and the local lifecycle state is ready and the peer is in the pending activate,
         * we need to move out of join_sync condition in ready state, when JOIN_SYNCING set.
         */ 
        if ((!b_is_active) && 
            (my_state == FBE_LIFECYCLE_STATE_READY &&
             peer_state == FBE_LIFECYCLE_STATE_ACTIVATE))
        {    
            fbe_base_object_trace((fbe_base_object_t*)base_config_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                  "bc_set_join_cond: %s-Set Passive Req Join-local=0x%llx, lstate=0x%x, peer=0x%llx, pstate=0x%x\n",
                                  b_is_active ? "Active" : "Passive",
                                  (unsigned long long)metadata_memory_p->flags, my_state, 
                                  (unsigned long long)metadata_memory_peer_p->flags, peer_state);

            fbe_lifecycle_set_cond(&fbe_base_config_lifecycle_const,
                                   (fbe_base_object_t *) base_config_p,
                                   FBE_BASE_CONFIG_LIFECYCLE_COND_PASSIVE_REQUEST_JOIN);    
        }
        /* AR 568762: For LUN if the passive side is in activate state, the active side should
         * clear JOIN flags to let join start again.
         */ 
        else if (b_is_active && 
            (base_config_p->base_object.class_id == FBE_CLASS_ID_LUN) &&
            (my_state == FBE_LIFECYCLE_STATE_READY &&
             peer_state == FBE_LIFECYCLE_STATE_ACTIVATE))
        {    
            fbe_base_object_trace((fbe_base_object_t*)base_config_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                  "bc_set_join_cond: %s-Clear Join Flags-local=0x%llx, lstate=0x%x, peer=0x%llx, pstate=0x%x\n",
                                  b_is_active ? "Active" : "Passive",
                                  (unsigned long long)metadata_memory_p->flags, my_state, 
                                  (unsigned long long)metadata_memory_peer_p->flags, peer_state);

            /* Clear the local JOIN flags */
            fbe_base_config_clear_clustered_flag(base_config_p, FBE_BASE_CONFIG_CLUSTERED_FLAG_JOIN_MASK);  
        }
    }
    /* If the passive is ready without join flag side, it need to rejoin */
    else if ((!b_is_active) &&
             (my_state == FBE_LIFECYCLE_STATE_READY) &&
             (peer_state == FBE_LIFECYCLE_STATE_READY) &&
             (!fbe_base_config_is_any_clustered_flag_set(base_config_p, (FBE_BASE_CONFIG_CLUSTERED_FLAG_JOINED|FBE_BASE_CONFIG_CLUSTERED_FLAG_JOIN_SYNCING))||
              !fbe_base_config_is_any_peer_clustered_flag_set(base_config_p, (FBE_BASE_CONFIG_CLUSTERED_FLAG_JOINED|FBE_BASE_CONFIG_CLUSTERED_FLAG_JOIN_SYNCING))))
    {
        /* pending state is reflected in metadata */
        fbe_lifecycle_get_state(&fbe_base_config_lifecycle_const, (fbe_base_object_t*)base_config_p, &my_state);

        if (my_state == FBE_LIFECYCLE_STATE_READY && base_config_p->base_object.class_id != FBE_CLASS_ID_EXTENT_POOL)
        {
            fbe_base_object_trace((fbe_base_object_t*)base_config_p, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                  "INVALID JOINED state: %s, local=0x%llx, lstate=0x%x, peer=0x%llx, pstate=0x%x\n",
                                  b_is_active ? "Active" : "Passive",
                                  (unsigned long long)metadata_memory_p->flags, my_state, 
                                  (unsigned long long)metadata_memory_peer_p->flags, peer_state);
            /*
            fbe_lifecycle_set_cond(&fbe_base_config_lifecycle_const,
                                   (fbe_base_object_t *) base_config_p,
                                   FBE_BASE_CONFIG_LIFECYCLE_COND_PASSIVE_REQUEST_JOIN);    
            */
        }
    }

    fbe_base_config_unlock(base_config_p);

    return FBE_STATUS_OK;
}
/*************************************************
 * end base_config_event_check_join_state()
 *************************************************/

/*!**************************************************************
 * base_config_event_check_activate_state()
 ****************************************************************
 * @brief
 *  This function looks at the clustered bits and sees if
 *  we need to set clustered activate condition
 *
 * @param base_config_p - object             
 *
 * @return fbe_status_t
 *
 * @author
 *  01/20/2012 - Created. Ashok Tamilarasan
 *
 ****************************************************************/

static fbe_status_t base_config_event_check_activate_state(fbe_base_config_t * base_config_p)
{
    fbe_bool_t b_peer_flag_set = FBE_FALSE;
    fbe_base_config_metadata_memory_t * metadata_memory_p = NULL;
    fbe_base_config_metadata_memory_t * metadata_memory_peer_p = NULL;
    fbe_object_id_t                     object_id = FBE_OBJECT_ID_INVALID;
    fbe_lifecycle_state_t               lifecycle_state;

    fbe_base_config_lock(base_config_p);
    /* Get a pointer to metadata memory */
    fbe_metadata_element_get_metadata_memory(&base_config_p->metadata_element, (void **)&metadata_memory_p);
    fbe_metadata_element_get_peer_metadata_memory(&base_config_p->metadata_element, (void **)&metadata_memory_peer_p);

    
    b_peer_flag_set = fbe_base_config_is_peer_clustered_flag_set(base_config_p, FBE_BASE_CONFIG_CLUSTERED_FLAG_ACTIVATE_REQ);

    /* If the peer requested to put he object in activate state and we have not started the process, 
     * then set the condition
     */
    if (b_peer_flag_set &&
        !fbe_base_config_is_clustered_flag_set(base_config_p, FBE_BASE_CONFIG_CLUSTERED_FLAG_ACTIVATE_REQ) &&
        !fbe_base_config_is_clustered_flag_set(base_config_p, FBE_BASE_CONFIG_CLUSTERED_FLAG_ACTIVATE_STARTED))
    {
        fbe_base_object_trace((fbe_base_object_t*)base_config_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "Clustered Activate local: 0x%llx peer: 0x%llx\n", 
                              (unsigned long long)metadata_memory_p->flags,
                              (unsigned long long)metadata_memory_peer_p->flags);

        /* We only need to abort if we're transitioning from READY
         */
        fbe_base_config_metadata_get_lifecycle_state(base_config_p, &lifecycle_state);
        if (lifecycle_state == FBE_LIFECYCLE_STATE_READY) 
        {
            /* aborts stripe locks
             */
            fbe_metadata_stripe_lock_element_abort(&base_config_p->metadata_element);
    
            /* Abort paged since a monitor operation might be waiting for paged. 
             * The monitor will not run to start eval rb logging or quiesce until the monitor op completes.
             */
            fbe_metadata_element_abort_paged(&base_config_p->metadata_element);
    
            /* Monitor operations might be waiting for memory.  Make sure these get aborted.
             */
            fbe_base_config_abort_monitor_ops(base_config_p);
    
            /* Monitor operations need to get aborted if they are queued in the block transport server 
             * due to bandwidth limitations.
             */
            fbe_base_object_get_object_id((fbe_base_object_t*)base_config_p, &object_id);
            fbe_block_transport_server_abort_monitor_operations(&base_config_p->block_transport_server,
                                                                object_id);
        }

        fbe_lifecycle_set_cond(&fbe_base_config_lifecycle_const,
                               (fbe_base_object_t *) base_config_p,
                               FBE_BASE_CONFIG_LIFECYCLE_COND_CLUSTERED_ACTIVATE);
    }

    fbe_base_config_unlock(base_config_p);
    return FBE_STATUS_OK;
}
/*************************************************
 * end base_config_event_check_activate_state()
 *************************************************/

static void base_config_process_memory_update_message(fbe_base_config_t * base_config)
{
    fbe_lifecycle_state_t		my_state, peer_state;
    fbe_status_t 				status;
    fbe_power_save_state_t		ps_state;
    fbe_base_config_metadata_memory_flags_t    	local_quiesce_state;
    fbe_base_config_metadata_memory_flags_t    	peer_quiesce_state;

    fbe_base_config_metadata_memory_t * metadata_memory_ptr = NULL;
    fbe_base_config_metadata_memory_t * metadata_memory_peer_ptr = NULL;

    if(base_config->metadata_element.metadata_element_state == FBE_METADATA_ELEMENT_STATE_INVALID){
        /* Nothing can be done - we are not initialized yet */
        return;
    }

    fbe_metadata_element_get_metadata_memory(&base_config->metadata_element, (void **)&metadata_memory_ptr);
    fbe_metadata_element_get_peer_metadata_memory(&base_config->metadata_element, (void **)&metadata_memory_peer_ptr);

    fbe_base_object_trace((fbe_base_object_t*)base_config,
                          FBE_TRACE_LEVEL_DEBUG_LOW, FBE_TRACE_MESSAGE_ID_INFO,
                          "bcfg_mem_update_msg local flags: 0x%llx peer flags: 0x%llx\n",  
                          (unsigned long long)((metadata_memory_ptr) ? metadata_memory_ptr->flags : 0),
                          (unsigned long long)((metadata_memory_peer_ptr) ? metadata_memory_peer_ptr->flags : 0));

    /* Check if peer requested Nonpaged update */
    if(fbe_base_config_is_peer_clustered_flag_set(base_config, FBE_BASE_CONFIG_METADATA_MEMORY_PEER_NONPAGED_REQUEST)){
        status = fbe_lifecycle_set_cond(&fbe_base_config_lifecycle_const, 
                                        (fbe_base_object_t*)base_config, 
                                        FBE_BASE_CONFIG_LIFECYCLE_COND_UPDATE_METADATA_MEMORY);
    }

    /*let's see if there is something special we need to do(i.e. process state changes and so on*/
    fbe_base_config_metadata_get_lifecycle_state(base_config, &my_state);
    fbe_base_config_metadata_get_peer_lifecycle_state(base_config, &peer_state);

    if(peer_state == FBE_LIFECYCLE_STATE_DESTROY){
        fbe_metadata_element_set_peer_dead(&base_config->metadata_element);
        //fbe_base_config_clear_clustered_flag(base_config, FBE_BASE_CONFIG_CLUSTERED_FLAG_JOINED);
    }

    /* Lock base config for updating the state */
    fbe_base_config_lock(base_config);

    /* Get power saving state */
    fbe_base_config_metadata_get_power_save_state(base_config, &ps_state);

    /*if the other side is either waking up or failing, let's go to ACTIVATE and either end us as READY or FAIL*/
    
    if ((my_state == FBE_LIFECYCLE_STATE_HIBERNATE) && 
         (ps_state != FBE_POWER_SAVE_STATE_EXITING_POWER_SAVE) &&
         (peer_state == FBE_LIFECYCLE_STATE_ACTIVATE || peer_state == FBE_LIFECYCLE_STATE_FAIL)) {

        fbe_base_config_metadata_set_power_save_state(base_config, FBE_POWER_SAVE_STATE_EXITING_POWER_SAVE);

        status = fbe_lifecycle_set_cond(&fbe_base_config_lifecycle_const, 
                                        (fbe_base_object_t*)base_config, 
                                        FBE_BASE_OBJECT_LIFECYCLE_COND_ACTIVATE);

        if (peer_state == FBE_LIFECYCLE_STATE_ACTIVATE) {
            fbe_base_object_trace((fbe_base_object_t*)base_config,
                                        FBE_TRACE_LEVEL_INFO,
                                        FBE_TRACE_MESSAGE_ID_INFO,
                                        "bcfg_mem_update_msg: Peer is waking up from hibernation, waking up too\n");
        }else{
            fbe_base_object_trace((fbe_base_object_t*)base_config,
                                        FBE_TRACE_LEVEL_WARNING,
                                        FBE_TRACE_MESSAGE_ID_INFO,
                                        "bcfg_mem_update_msg: Peer is dead, waking up from hibernation\n");

        }
    
        if (status != FBE_STATUS_OK) {
             fbe_base_object_trace((fbe_base_object_t*)base_config,
                                    FBE_TRACE_LEVEL_ERROR,
                                    FBE_TRACE_MESSAGE_ID_INFO,
                                    "bcfg_mem_update_msg: can't set condition \n");
    
        }
    }
    /* Unlock base config for updating the state */
    fbe_base_config_unlock(base_config);

    /* Get the object's local and peer metadata memory puiesce state */
    fbe_base_config_metadata_get_quiesce_state_local(base_config, &local_quiesce_state);
    fbe_base_config_metadata_get_quiesce_state_peer(base_config, &peer_quiesce_state);

    /* Check the quiesce state to see if the quiesce/unquiesce conditions should be set on the object on this SP */
    /* if peer want to quiesce, but we haven't start, or we've gone too far into unquiesce, we need
     * to restart with quiesce.
     */
    if ((((local_quiesce_state == FBE_BASE_CONFIG_METADATA_MEMORY_QUIESCE_NOT_STARTED) &&
          (!fbe_base_config_is_any_clustered_flag_set(base_config,FBE_BASE_CONFIG_CLUSTERED_FLAG_QUIESCE_MASK))) ||
         (local_quiesce_state == FBE_BASE_CONFIG_METADATA_MEMORY_UNQUIESCE_READY))&&
        (peer_quiesce_state == FBE_BASE_CONFIG_METADATA_MEMORY_QUIESCE_READY ))
    {
        /* The peer is quiescing I/O on object and this SP is not; ensure the quiesce and unquiesce conditions are set here */

        fbe_base_object_trace((fbe_base_object_t*)base_config,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "bcfg_mem_update_msg: Peer Quiesce qst l/p: 0x%llx/0x%llx; set cond 0x%llx/0x%llx\n",
                              (unsigned long long)local_quiesce_state,
                              (unsigned long long)peer_quiesce_state,
                              (unsigned long long)((metadata_memory_ptr) ? metadata_memory_ptr->flags : 0),
                              (unsigned long long)((metadata_memory_peer_ptr) ? metadata_memory_peer_ptr->flags : 0));
        /* join_sync condition is in ready state, checking for ready state is not sufficient any more */
        if(fbe_base_config_is_clustered_flag_set(base_config, FBE_BASE_CONFIG_CLUSTERED_FLAG_JOINED) )  {

            /* If the peer has the hold set and we are not in the middle of a prior quiesce, set clustered qh also.
             */
            if (!fbe_base_config_is_clustered_flag_set(base_config, FBE_BASE_CONFIG_METADATA_MEMORY_FLAGS_QUIESCE_HOLD) &&
                fbe_base_config_is_peer_clustered_flag_set(base_config, FBE_BASE_CONFIG_METADATA_MEMORY_FLAGS_QUIESCE_HOLD) &&
                !fbe_metadata_is_abort_set(&base_config->metadata_element)){
                fbe_base_config_set_clustered_flag(base_config, FBE_BASE_CONFIG_METADATA_MEMORY_FLAGS_QUIESCE_HOLD);
            }
            /* This kind of quiesce will want to hold I/Os at bts instead of aborting them midstream.
             */
            if (!fbe_base_config_is_clustered_flag_set(base_config, FBE_BASE_CONFIG_METADATA_MEMORY_FLAGS_QUIESCE_HOLD)) {
                fbe_metadata_element_abort_paged(&base_config->metadata_element);
                fbe_metadata_stripe_lock_element_abort(&base_config->metadata_element);
            }
            fbe_lifecycle_set_cond(&fbe_base_config_lifecycle_const, (fbe_base_object_t*)base_config,
                                    FBE_BASE_CONFIG_LIFECYCLE_COND_QUIESCE);
        }
        else {
            fbe_base_object_trace((fbe_base_object_t *)base_config,
                        FBE_TRACE_LEVEL_INFO,
                        FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                  "bcfg_mem_update_msg: drop set condition local: 0x%llx peer: 0x%llx\n",  
                        (unsigned long long)((metadata_memory_ptr) ? metadata_memory_ptr->flags : 0),
                        (unsigned long long)((metadata_memory_peer_ptr) ? metadata_memory_peer_ptr->flags : 0));
        }
         
    }

    /* Check to see if we need to set the join condition.
     */
    base_config_event_check_join_state(base_config);

    /* Check to see if we need to put the object in activate state
     */
    base_config_event_check_activate_state(base_config);
    
    /* Check to see if we need to become Active
     */
    base_config_event_check_passive(base_config);
    
    fbe_lifecycle_reschedule(&fbe_base_config_lifecycle_const,
                                 (fbe_base_object_t *) base_config,
                                 (fbe_lifecycle_timer_msec_t) 0);    /* Immediate reschedule */	
    
    return;
}

/*!**************************************************************
 * base_config_process_peer_contact_lost()
 ****************************************************************
 * @brief
 *  This is the umbrella function to call others who needs to taken
 *  action when the peer goes away
 *
 * @param base_config_p - object             
 *
 * @return fbe_status_t
 *
 * @author
 *  04/20/2012 - Created. Ashok Tamilarasan
 *
 ****************************************************************/
static void base_config_process_peer_contact_lost(fbe_base_config_t * base_config)
{
    /* Clear passive request when peer lost */
    if (fbe_base_config_is_any_clustered_flag_set(base_config, FBE_BASE_CONFIG_CLUSTERED_FLAG_PASSIVE_MASK)){
        fbe_base_object_trace(  (fbe_base_object_t *)base_config,
                                FBE_TRACE_LEVEL_INFO,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s: Clear PASSIVE flags\n", __FUNCTION__);

        fbe_base_config_clear_clustered_flag(base_config, FBE_BASE_CONFIG_CLUSTERED_FLAG_PASSIVE_MASK);
    }

    /* use update metadata memory condition to figure out if we need to read non-paged from disk. */
    fbe_lifecycle_set_cond(&fbe_base_config_lifecycle_const, 
                           (fbe_base_object_t*)base_config, 
                           FBE_BASE_CONFIG_LIFECYCLE_COND_UPDATE_METADATA_MEMORY);

    /* Process the peer persist pending if needed */
    base_config_process_peer_contact_lost_nonpaged_persist_pending(base_config);

    /* Since the peer is dead, we need to handle the stripe locks that is owned
     * by the peer
     */
    base_config_process_peer_contact_lost_release_sl(base_config);

    /* AR752962:
     * clear the FBE_BASE_CONFIG_METADATA_MEMORY_FLAGS_QUIESCE_HOLD flag 
     * After adding the 4k feature, in the condition fbe_raid_group_clear_rl_cond_function,
     * we will attempt to drain all the IO instead of doing the normal quiesce.
     * But some IO might be waiting for peer to grant SL while peer panic.
     * These IO could not be drained until FBE_BASE_CONFIG_LIFECYCLE_COND_PEER_DEATH_RELEASE_SL runs.
     * So draing IO procedure will be stucked.
     * By clearing the HOLD flag, we will transfer to do the normal quiesce.
     * */
    if (fbe_base_config_is_clustered_flag_set(base_config, FBE_BASE_CONFIG_METADATA_MEMORY_FLAGS_QUIESCE_HOLD))
    {
        fbe_base_object_trace((fbe_base_object_t *)base_config,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s peer lost, going to clear FBE_BASE_CONFIG_METADATA_MEMORY_FLAGS_QUIESCE_HOLD\n", __FUNCTION__);
        fbe_base_config_clear_clustered_flag(base_config, 
                                             FBE_BASE_CONFIG_METADATA_MEMORY_FLAGS_QUIESCE_HOLD);
    }
}

/*!**************************************************************
 * base_config_process_peer_contact_lost_nonpaged_persist_pending()
 ****************************************************************
 * @brief
 *  This handles the case where the peer was in the middle of
 * persisting the non-paged and went down
 *
 * @param base_config_p - object             
 *
 * @return fbe_status_t
 *
 * @author
 *  04/20/2012 - Created. Ashok Tamilarasan
 *
 ****************************************************************/
static void 
base_config_process_peer_contact_lost_nonpaged_persist_pending(fbe_base_config_t * base_config)
{
    /* Check if the peer is in the process of updating the non-paged and it went down
     * If so set the condition that persists the non-paged data */
    if(fbe_metadata_element_is_peer_persist_pending(&base_config->metadata_element))
    {
        /* Now that we have kicked of the processing, clear the flag */
        fbe_metadata_element_clear_peer_persist_pending(&base_config->metadata_element);

        fbe_lifecycle_set_cond(&fbe_base_config_lifecycle_const, 
                               (fbe_base_object_t *)base_config, 
                               FBE_BASE_CONFIG_LIFECYCLE_COND_PERSIST_NON_PAGED_METADATA);
        fbe_base_object_trace((fbe_base_object_t *)base_config,
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                  "%s Write out NP\n", __FUNCTION__);
    }
    
}

static fbe_status_t 
base_config_event_check_passive(fbe_base_config_t * base_config)
{	
    if(fbe_base_config_is_any_peer_clustered_flag_set(base_config, FBE_BASE_CONFIG_CLUSTERED_FLAG_PASSIVE_MASK)){
        fbe_lifecycle_set_cond(&fbe_base_config_lifecycle_const, 
                                (fbe_base_object_t*)base_config, 
                                FBE_BASE_CONFIG_LIFECYCLE_COND_UPDATE_METADATA_MEMORY);
    }

    return FBE_STATUS_OK;
}
/*!**************************************************************
 * base_config_process_peer_contact_lost_release_sl()
 ****************************************************************
 * @brief
 *  Set a condition so that we can process the release of stripe
 *  locks that is owned by the peer
 *
 * @param base_config_p - object             
 *
 * @return fbe_status_t
 *
 * @author
 *  07/31/2012 - Created. Ashok Tamilarasan
 *
 ****************************************************************/
static void 
base_config_process_peer_contact_lost_release_sl(fbe_base_config_t * base_config)
{
    fbe_object_id_t                     object_id = FBE_OBJECT_ID_INVALID;

    fbe_lifecycle_set_cond(&fbe_base_config_lifecycle_const, 
                           (fbe_base_object_t *)base_config, 
                           FBE_BASE_CONFIG_LIFECYCLE_COND_PEER_DEATH_RELEASE_SL);
    fbe_base_object_trace((fbe_base_object_t *)base_config,
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s. Set condition to release SL \n", __FUNCTION__);

    /* Aborts stripe locks for monitor ops.
     */
    fbe_metadata_stripe_lock_element_abort_monitor_ops(&base_config->metadata_element, FBE_FALSE);

    /* Monitor operations might be waiting for memory.  Make sure these get aborted.
     */
    fbe_base_config_abort_monitor_ops(base_config);

    /* Monitor operations need to get aborted if they are queued in the block transport server 
     * due to bandwidth limitations.
     */
    fbe_base_object_get_object_id((fbe_base_object_t*)base_config, &object_id);
    fbe_block_transport_server_abort_monitor_operations(&base_config->block_transport_server,
                                                        object_id);

}

