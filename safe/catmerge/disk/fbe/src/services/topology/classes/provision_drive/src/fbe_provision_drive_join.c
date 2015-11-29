/***************************************************************************
 * Copyright (C) EMC Corporation 2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_provision_drive_join.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the code for synchronizing the two SPs.
 *  This code allows the passive side to join the active side.
 *
 * @version
 *   10/3/2011 - Created. Rob Foley
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_types.h"
#include "fbe/fbe_transport.h"
#include "fbe_provision_drive_private.h"
#include "fbe_testability.h"
#include "fbe_database.h"
#include "fbe_cmi.h"

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

/*!****************************************************************************
 * fbe_provision_drive_join_request()
 ******************************************************************************
 * @brief
 *  This function implements the local join request state.
 *  In this state both SPs coordinate setting the "request" clustered flag.
 *  When both SPs have this request clustered flag set, we transition to the
 *  join started state.
 *
 * @param provision_drive_p - provision drive object
 * @param packet_p - packet pointer.
 * 
 * @return fbe_lifecycle_status_t
 *
 * @author
 *  10/3/2011 - Created. Rob Foley
 *
 ******************************************************************************/
fbe_lifecycle_status_t 
fbe_provision_drive_join_request(fbe_provision_drive_t *  provision_drive_p, fbe_packet_t * packet_p)
{
    fbe_bool_t b_peer_flag_set = FBE_FALSE;
	fbe_bool_t b_is_active;
    fbe_scheduler_hook_status_t        status = FBE_SCHED_STATUS_OK;

	b_is_active = fbe_base_config_is_active((fbe_base_config_t*)provision_drive_p);

    fbe_provision_drive_lock(provision_drive_p);

    /* If we don't already have the join local state set, we should set it now since
     * the first time through this will not be set. 
     */
    if (!fbe_provision_drive_is_local_state_set(provision_drive_p, FBE_PVD_LOCAL_STATE_JOIN_REQUEST))
    {
        fbe_provision_drive_set_local_state(provision_drive_p, FBE_PVD_LOCAL_STATE_JOIN_REQUEST);
        fbe_base_config_clear_clustered_flag((fbe_base_config_t*)provision_drive_p, 
                                             FBE_BASE_CONFIG_CLUSTERED_FLAG_JOIN_MASK);
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s join request clear clustered flags local: 0x%llx peer: 0x%llx state: 0x%llx 0x%x\n", 
                              b_is_active ? "Active" : "Passive",
                              (unsigned long long)provision_drive_p->provision_drive_metadata_memory.base_config_metadata_memory.flags,
                              (unsigned long long)provision_drive_p->provision_drive_metadata_memory_peer.base_config_metadata_memory.flags,
                              (unsigned long long)provision_drive_p->local_state,
                              provision_drive_p->provision_drive_metadata_memory_peer.base_config_metadata_memory.lifecycle_state);
    }

    /* We start the process by setting join request. 
     * This gets the peer running the correct condition. 
     */
    if (!fbe_base_config_is_clustered_flag_set((fbe_base_config_t*)provision_drive_p, FBE_BASE_CONFIG_CLUSTERED_FLAG_JOIN_REQ))
    {
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s join marking request local state: 0x%llx\n",
                              b_is_active ? "Active" : "Passive",
                              (unsigned long long)provision_drive_p->local_state);
        fbe_base_config_set_clustered_flag((fbe_base_config_t*)provision_drive_p, FBE_BASE_CONFIG_CLUSTERED_FLAG_JOIN_REQ);
        fbe_provision_drive_unlock(provision_drive_p);
        return FBE_LIFECYCLE_STATUS_RESCHEDULE;
    }

    /* Next we wait for the peer to tell us it received the join request.
     */
    b_peer_flag_set = fbe_base_config_is_peer_clustered_flag_set((fbe_base_config_t*)provision_drive_p, 
                                                                 FBE_BASE_CONFIG_CLUSTERED_FLAG_JOIN_REQ);

    fbe_provision_drive_check_hook(provision_drive_p,
                                   SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_JOIN,
                                   FBE_PROVISION_DRIVE_SUBSTATE_JOIN_REQUEST, 
                                   0,
                                   &status);
    if (status == FBE_SCHED_STATUS_DONE) {
        fbe_provision_drive_unlock(provision_drive_p);
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    if (!b_peer_flag_set)
    {
        fbe_provision_drive_unlock(provision_drive_p);
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s join wait peer request local: 0x%llx peer: 0x%llx state: 0x%llx\n", 
                              b_is_active ? "Active" : "Passive",
                              (unsigned long long)provision_drive_p->provision_drive_metadata_memory.base_config_metadata_memory.flags,
                              (unsigned long long)provision_drive_p->provision_drive_metadata_memory_peer.base_config_metadata_memory.flags,
                              (unsigned long long)provision_drive_p->local_state);
        return FBE_LIFECYCLE_STATUS_DONE;
    }
    else
    {
        /* The peer has set the flag, we are ready to do the next set of checks.
         * We need to wait for the peer to get to the join request state as an acknowledgement that 
         * it knows we are starting this join. 
         */
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s join peer request set local: 0x%llx peer: 0x%llx state: 0x%llx 0x%x\n", 
                              b_is_active ? "Active" : "Passive",
                              (unsigned long long)provision_drive_p->provision_drive_metadata_memory.base_config_metadata_memory.flags,
                              (unsigned long long)provision_drive_p->provision_drive_metadata_memory_peer.base_config_metadata_memory.flags,
                              (unsigned long long)provision_drive_p->local_state,
                              provision_drive_p->provision_drive_metadata_memory_peer.base_config_metadata_memory.lifecycle_state);
        fbe_provision_drive_set_local_state(provision_drive_p, FBE_PVD_LOCAL_STATE_JOIN_STARTED);
        fbe_provision_drive_clear_local_state(provision_drive_p, FBE_PVD_LOCAL_STATE_JOIN_REQUEST);
        fbe_provision_drive_unlock(provision_drive_p);
        return FBE_LIFECYCLE_STATUS_RESCHEDULE;
    }
}
/**************************************
 * end fbe_provision_drive_join_request()
 **************************************/

/*!****************************************************************************
 * fbe_provision_drive_join_passive()
 ******************************************************************************
 * @brief
 *  This function is for the passive side's implementation of the
 *  local state "join started".
 *  The passive side enters this state when it sees that the Active side is
 *  requesting a join.
 * 
 *  This function will wait until it sees the "started" clustered flag.
 *  Then it will set its failed drives bitmap and set the "started" clustered flag.
 *  This passive SP needs to provide the failed drives bitmap so that the Active SP
 *  is able to perform the eval of rebuild logging.
 * 
 *  Finally this state will wait for the peer's "Join started" flag to get cleared.
 *  That will be an indication that the join has finished.  And then this state will
 *  set the "Join Done" local state.
 * 
 * @param provision_drive_p - provision drive object
 * @param packet_p - packet pointer.
 * 
 * @return fbe_status_t
 *
 * @author
 *  10/3/2011 - Created. Rob Foley
 *
 ******************************************************************************/
fbe_lifecycle_status_t 
fbe_provision_drive_join_passive(fbe_provision_drive_t *  provision_drive_p, fbe_packet_t * packet_p)
{
    fbe_bool_t b_peer_flag_set = FBE_FALSE;
    fbe_bool_t b_local_flag_set = FBE_FALSE;
	fbe_bool_t b_is_active;
    fbe_scheduler_hook_status_t        status = FBE_SCHED_STATUS_OK;

    fbe_provision_drive_lock(provision_drive_p);

    /* The active side always starts the operation. 
     * Once we see the active side has started the operation we will send over our bitmask. 
     */
    b_peer_flag_set = fbe_base_config_is_peer_clustered_flag_set((fbe_base_config_t*)provision_drive_p, 
                                                                 FBE_BASE_CONFIG_CLUSTERED_FLAG_JOIN_STARTED);

    b_local_flag_set = fbe_base_config_is_clustered_flag_set((fbe_base_config_t*)provision_drive_p, 
                                                             FBE_BASE_CONFIG_CLUSTERED_FLAG_JOIN_STARTED);

    fbe_base_object_trace((fbe_base_object_t *)provision_drive_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "PASSIVE join started clustered flags local: 0x%llx peer: 0x%llx state: 0x%llx 0x%x\n", 
                              (unsigned long long)provision_drive_p->provision_drive_metadata_memory.base_config_metadata_memory.flags,
                              (unsigned long long)provision_drive_p->provision_drive_metadata_memory_peer.base_config_metadata_memory.flags,
                              (unsigned long long)provision_drive_p->local_state,
                              provision_drive_p->provision_drive_metadata_memory_peer.base_config_metadata_memory.lifecycle_state);

    /* We first wait for the peer to start the join. 
     * This is needed since either active or passive can initiate the join. 
     * Once we see this set, we will set our flag, which the peer waits for before it starts the join.
     */
    if (!b_peer_flag_set && !b_local_flag_set)
    {
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "Passive waiting peer join started local: 0x%llx peer: 0x%llx state: 0x%llx\n",
                              (unsigned long long)provision_drive_p->provision_drive_metadata_memory.base_config_metadata_memory.flags,
                              (unsigned long long)provision_drive_p->provision_drive_metadata_memory_peer.base_config_metadata_memory.flags,
                              (unsigned long long)provision_drive_p->local_state);
        fbe_provision_drive_unlock(provision_drive_p);
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /* At this point the peer started flag is set.
     */
    if (!b_local_flag_set)
    {
        /* Peer is starting the join.  We have not yet responded.
         */
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "Passive marking join started state: 0x%llx\n",
			      (unsigned long long)provision_drive_p->local_state);
        /*! @todo This is the point where we would set in metadata memory the details of our health.
         */
        fbe_base_config_set_clustered_flag((fbe_base_config_t*)provision_drive_p, FBE_BASE_CONFIG_CLUSTERED_FLAG_JOIN_STARTED);
        /* Set SLF flag here */
        fbe_provision_drive_set_slf_flag_based_on_downstream_edge(provision_drive_p);
        if (fbe_provision_drive_is_clustered_flag_set(provision_drive_p, FBE_PROVISION_DRIVE_CLUSTERED_FLAG_SLF))
        {
            /* If passive side is in SLF, start eval_slf in ready state. */
            fbe_lifecycle_set_cond(&fbe_provision_drive_lifecycle_const,
                                   (fbe_base_object_t*)provision_drive_p,
                                    FBE_PROVISION_DRIVE_LIFECYCLE_COND_EVAL_SLF);
        }
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "Passive marking slf state: 0x%llx flag 0x%x peer 0x%x\n", (unsigned long long)provision_drive_p->local_state,
							  (unsigned int)provision_drive_p->provision_drive_metadata_memory.flags,
							  (unsigned int)provision_drive_p->provision_drive_metadata_memory_peer.flags);
        fbe_provision_drive_unlock(provision_drive_p);
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    fbe_provision_drive_check_hook(provision_drive_p, SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_JOIN,
                               FBE_PROVISION_DRIVE_SUBSTATE_JOIN_STARTED, 0, &status);
    if (status == FBE_SCHED_STATUS_DONE) {
        fbe_provision_drive_unlock(provision_drive_p);
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /* If we become active, just return out.
     * We need to handle the active's work ourself. 
     * When we come back into the condition we'll automatically execute the active code.
     */
	b_is_active = fbe_base_config_is_active((fbe_base_config_t*)provision_drive_p);
    if (b_is_active)
    {
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p, FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                              "Passive join became active local: 0x%llx peer: 0x%llx lstate: 0x%llx\n",
                              (unsigned long long)provision_drive_p->provision_drive_metadata_memory.base_config_metadata_memory.flags,
                              (unsigned long long)provision_drive_p->provision_drive_metadata_memory_peer.base_config_metadata_memory.flags,
                              (unsigned long long)provision_drive_p->local_state);
        fbe_provision_drive_unlock(provision_drive_p);
        return FBE_LIFECYCLE_STATUS_RESCHEDULE;
    }
    /* Once we have set the started flag, we simply wait for the active side to complete. 
     * We know Active is done when it clears the flags. 
     */
    if (!b_peer_flag_set)
    {
        fbe_provision_drive_set_local_state(provision_drive_p, FBE_PVD_LOCAL_STATE_JOIN_DONE);
        fbe_provision_drive_clear_local_state(provision_drive_p, FBE_PVD_LOCAL_STATE_JOIN_STARTED);
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "Passive join start cleanup local: 0x%llx peer: 0x%llx lstate: 0x%llx\n",
                              (unsigned long long)provision_drive_p->provision_drive_metadata_memory.base_config_metadata_memory.flags,
                              (unsigned long long)provision_drive_p->provision_drive_metadata_memory_peer.base_config_metadata_memory.flags,
                              (unsigned long long)provision_drive_p->local_state);
        fbe_provision_drive_unlock(provision_drive_p);
        return FBE_LIFECYCLE_STATUS_RESCHEDULE;
    }
    else
    {
        fbe_provision_drive_unlock(provision_drive_p);
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "Passive join wait peer complete local: 0x%llx peer: 0x%llx lstate: 0x%llx\n",
                              (unsigned long long)provision_drive_p->provision_drive_metadata_memory.base_config_metadata_memory.flags,
                              (unsigned long long)provision_drive_p->provision_drive_metadata_memory_peer.base_config_metadata_memory.flags,
                              (unsigned long long)provision_drive_p->local_state);
    }
    return FBE_LIFECYCLE_STATUS_DONE;
}
/**************************************
 * end fbe_provision_drive_join_passive()
 **************************************/

/*!****************************************************************************
 * fbe_provision_drive_join_active()
 ******************************************************************************
 * @brief
 *  This function implements the active side's Join Started local state.
 *  This state will set the "Join started" clustered flags
 *  and wait for the peer to do the same.
 * 
 *  Once we see the peer's "Join started" clustered flags we will
 *  set the eval rebuild logging condition and transition to the
 *  Join done local state so that when we come through this condition again
 *  we will complete the join.
 *
 * @param provision_drive_p - provision drive object
 * @param packet_p - packet pointer.
 * 
 * @return fbe_status_t
 *
 * @author
 *  10/3/2011 - Created. Rob Foley
 *
 ******************************************************************************/
fbe_lifecycle_status_t 
fbe_provision_drive_join_active(fbe_provision_drive_t *  provision_drive_p, fbe_packet_t * packet_p)
{
    fbe_bool_t          b_peer_flag_set = FBE_FALSE;
    fbe_scheduler_hook_status_t        status = FBE_SCHED_STATUS_OK;

    fbe_provision_drive_lock(provision_drive_p);

    /* Now we set the bit that says we are starting. 
     * This bit always gets set first by the Active side. 
     * When passive sees the started peer bit, it will also set the started bit. 
     * The passive side will thereafter know we are done when this bit is cleared. 
     */
    if (!fbe_base_config_is_clustered_flag_set((fbe_base_config_t*)provision_drive_p, FBE_BASE_CONFIG_CLUSTERED_FLAG_JOIN_STARTED))
    {
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "Active join mark started loc: 0x%llx peer: 0x%llx state: 0x%llx\n",
                              (unsigned long long)provision_drive_p->provision_drive_metadata_memory.base_config_metadata_memory.flags,
                              (unsigned long long)provision_drive_p->provision_drive_metadata_memory_peer.base_config_metadata_memory.flags,
                              (unsigned long long)provision_drive_p->local_state);
        fbe_base_config_set_clustered_flag((fbe_base_config_t*)provision_drive_p, FBE_BASE_CONFIG_CLUSTERED_FLAG_JOIN_STARTED);
        fbe_provision_drive_unlock(provision_drive_p);
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /* Now we need to wait for the peer to acknowledge that it saw us start.
     * By waiting for the passive side to get to this point, it allows the 
     * active side to clear the bits when it is complete. 
     * The passive side will also be able to know we are done when the bits are cleared.
     */
    b_peer_flag_set = fbe_base_config_is_peer_clustered_flag_set((fbe_base_config_t*)provision_drive_p, 
                                                                 FBE_BASE_CONFIG_CLUSTERED_FLAG_JOIN_STARTED);
    if (!b_peer_flag_set)
    {
        fbe_provision_drive_unlock(provision_drive_p);
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "Active join wait peer started local fl: 0x%llx peer fl: 0x%llx state: 0x%llx\n", 
                              (unsigned long long)provision_drive_p->provision_drive_metadata_memory.base_config_metadata_memory.flags,
                              (unsigned long long)provision_drive_p->provision_drive_metadata_memory_peer.base_config_metadata_memory.flags,
                              (unsigned long long)provision_drive_p->local_state);
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    fbe_provision_drive_check_hook(provision_drive_p,
                               SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_JOIN,
                               FBE_PROVISION_DRIVE_SUBSTATE_JOIN_STARTED, 
                               0,
                               &status);
    if (status == FBE_SCHED_STATUS_DONE) {
        fbe_provision_drive_unlock(provision_drive_p);
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /* Peer sees we are starting and has sent us the bitmask.
     */
    fbe_provision_drive_set_local_state(provision_drive_p, FBE_PVD_LOCAL_STATE_JOIN_DONE);
    fbe_provision_drive_clear_local_state(provision_drive_p, FBE_PVD_LOCAL_STATE_JOIN_STARTED);

    fbe_lifecycle_set_cond(&fbe_provision_drive_lifecycle_const, (fbe_base_object_t*) provision_drive_p, 
                           FBE_PROVISION_DRIVE_LIFECYCLE_COND_EVAL_SLF);

    /*! @todo theoretically if neither side has anything to do, we could just transition to done.
     */
    fbe_provision_drive_unlock(provision_drive_p);
    return FBE_LIFECYCLE_STATUS_RESCHEDULE;
}
/**************************************
 * end fbe_provision_drive_join_active()
 **************************************/

/*!****************************************************************************
 * fbe_provision_drive_join_done()
 ******************************************************************************
 * @brief
 *  This function implements the Join Done local state.
 *  We clear our Join started clustered flag and set our JOINED clustered flag.
 *  We then wait for the peer to also clear its Join started clustered flag and
 *  set its JOINED clustered flag.
 * 
 *  At that point we will clear our join started local state and
 *  set the joined local state.
 *
 * @param provision_drive_p - provision drive object
 * @param packet_p - packet pointer.
 * 
 * @return fbe_lifecycle_status_t
 *
 * @author
 *  10/3/2011 - Created. Rob Foley
 *
 ******************************************************************************/
fbe_lifecycle_status_t 
fbe_provision_drive_join_done(fbe_provision_drive_t *  provision_drive_p, fbe_packet_t * packet_p)
{
    fbe_status_t status;
	fbe_bool_t b_is_active;
    fbe_bool_t b_peer_started_set = FBE_FALSE;
    fbe_scheduler_hook_status_t        hook_status = FBE_SCHED_STATUS_OK;

	b_is_active = fbe_base_config_is_active((fbe_base_config_t*)provision_drive_p);

    fbe_provision_drive_lock(provision_drive_p);

    /* We want to clear our local flags first.
     */
    if (fbe_base_config_is_clustered_flag_set((fbe_base_config_t*)provision_drive_p, FBE_BASE_CONFIG_CLUSTERED_FLAG_JOIN_STARTED))
    {
        /* At this point only started should be set, but we're clearing everything here.
         */
        fbe_base_config_clear_clustered_flag((fbe_base_config_t*)provision_drive_p, FBE_BASE_CONFIG_CLUSTERED_FLAG_JOIN_STARTED);
        /* Set join_syncing so that we acknowledge the peer the active side is ready.
         * The passive side sets the syncing bit in pending ready.
         */
        if ((b_is_active) &&
            !(fbe_base_config_is_clustered_flag_set((fbe_base_config_t*)provision_drive_p, FBE_BASE_CONFIG_CLUSTERED_FLAG_JOIN_SYNCING)))
        {
            fbe_base_config_set_clustered_flag((fbe_base_config_t*)provision_drive_p, FBE_BASE_CONFIG_CLUSTERED_FLAG_JOIN_SYNCING);
        }
        fbe_provision_drive_unlock(provision_drive_p);
        return FBE_LIFECYCLE_STATUS_RESCHEDULE;
    }

    fbe_provision_drive_check_hook(provision_drive_p,
                                   SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_JOIN,
                                   FBE_PROVISION_DRIVE_SUBSTATE_JOIN_DONE, 
                                   0,
                                   &hook_status);
    if (hook_status == FBE_SCHED_STATUS_DONE) {
        fbe_provision_drive_unlock(provision_drive_p);
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    b_peer_started_set = fbe_base_config_is_peer_clustered_flag_set((fbe_base_config_t*)provision_drive_p, 
                                                                    FBE_BASE_CONFIG_CLUSTERED_FLAG_JOIN_SYNCING);
    if (!b_peer_started_set)
    {
        fbe_provision_drive_unlock(provision_drive_p);
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s join cleanup, wait peer done local: 0x%llx peer: 0x%llx state:0x%llx\n", 
                              b_is_active ? "Active" : "Passive",
                              (unsigned long long)provision_drive_p->provision_drive_metadata_memory.base_config_metadata_memory.flags,
                              (unsigned long long)provision_drive_p->provision_drive_metadata_memory_peer.base_config_metadata_memory.flags,
                              (unsigned long long)provision_drive_p->local_state);
        return FBE_LIFECYCLE_STATUS_DONE;
    }
    else
    {
        /* The local flags tell us that we are waiting for the peer to clear their flags.
         * Since it seems they are complete, it is safe to clear our local flags. 
         */
        fbe_provision_drive_clear_local_state(provision_drive_p, 
                                         (FBE_PVD_LOCAL_STATE_JOIN_STARTED | 
                                          FBE_PVD_LOCAL_STATE_JOIN_DONE));

        /* It is possible that we received a request to join either locally or from the peer.
         * If a request is not set, let's clear the condition.  Otherwise we will run this condition again. 
         */
        if (!fbe_provision_drive_is_local_state_set(provision_drive_p, FBE_PVD_LOCAL_STATE_JOIN_REQUEST))
        {
            status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)provision_drive_p);
            if (status != FBE_STATUS_OK)
            {
                fbe_base_object_trace((fbe_base_object_t *)provision_drive_p, 
                                      FBE_TRACE_LEVEL_ERROR,
                                      FBE_TRACE_MESSAGE_ID_INFO,
                                      "%s can't clear current condition, status: 0x%x\n",
                                      __FUNCTION__, status);
            }
            if (b_is_active)
            {
                /* We need to move into join_sync condition to finish up, if the passive side has gone through pending ready */
                status = fbe_lifecycle_set_cond(&fbe_provision_drive_lifecycle_const, (fbe_base_object_t*) provision_drive_p, 
                                       FBE_BASE_CONFIG_LIFECYCLE_COND_JOIN_SYNC);
                if (status != FBE_STATUS_OK)
                {
                    fbe_base_object_trace((fbe_base_object_t*)provision_drive_p, 
                                            FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                            "%s failed to set join_sync condition %d\n", __FUNCTION__, status);
                }
            }
        }
        /* Set the joined local state to note that we should now coordinate with the peer.
         */
        //fbe_provision_drive_set_local_state(provision_drive_p, FBE_PVD_LOCAL_STATE_JOINED);
        fbe_provision_drive_unlock(provision_drive_p);
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s join cleanup complete local: 0x%llx peer: 0x%llx state: 0x%llx\n",
                              b_is_active ? "Active" : "Passive",
                              (unsigned long long)provision_drive_p->provision_drive_metadata_memory.base_config_metadata_memory.flags,
                              (unsigned long long)provision_drive_p->provision_drive_metadata_memory_peer.base_config_metadata_memory.flags,
                              (unsigned long long)provision_drive_p->local_state);
        return FBE_LIFECYCLE_STATUS_RESCHEDULE;
    }
}
/**************************************
 * end fbe_provision_drive_join_done()
 **************************************/

/*!****************************************************************************
 * fbe_provision_drive_join_sync_complete()
 ******************************************************************************
 * @brief
 *  This function complete the join process by syncing with peer.
 *  We clear our Join syncing clustered flag and set our JOINED clustered flag.
 *  We then wait for the peer to also clear its Join started clustered flag and
 *  set its JOINED clustered flag.
 * 
 *  At that point we will clear our join started local state and
 *  set the joined local state.
 *
 *  This function is called from join_sync condition.
 *
 * @param provision_drive_p - provision drive object
 * @param packet_p - packet pointer.
 * 
 * @return fbe_lifecycle_status_t
 *
 * @author
 *  2/23/2012 - Created. Naizhong
 *
 ******************************************************************************/
fbe_lifecycle_status_t 
fbe_provision_drive_join_sync_complete(fbe_provision_drive_t *  provision_drive_p, fbe_packet_t * packet_p)
{
    fbe_bool_t b_is_active;
    fbe_bool_t b_peer_started_set = FBE_FALSE;
    fbe_scheduler_hook_status_t        hook_status = FBE_SCHED_STATUS_OK;
    fbe_status_t        fbe_status = FBE_STATUS_OK;
    fbe_lifecycle_state_t   local_state= FBE_LIFECYCLE_STATE_INVALID, peer_state = FBE_LIFECYCLE_STATE_INVALID;
    fbe_base_config_clustered_flags_t local_flags = 0, peer_flags = 0;
	fbe_provision_drive_nonpaged_metadata_drive_info_t  np_metadata_drive_info;
    fbe_object_id_t last_system_object_id;
    fbe_object_id_t pvd_object_id;
    fbe_cmi_sp_state_t sp_state;

	b_is_active = fbe_base_config_is_active((fbe_base_config_t*)provision_drive_p);
    fbe_base_config_metadata_get_peer_lifecycle_state((fbe_base_config_t *)provision_drive_p, &peer_state);
    fbe_base_config_metadata_get_lifecycle_state((fbe_base_config_t *)provision_drive_p, &local_state);
    fbe_base_config_get_clustered_flags((fbe_base_config_t *) provision_drive_p, &local_flags);
    fbe_base_config_get_peer_clustered_flags((fbe_base_config_t *) provision_drive_p, &peer_flags);

    /* active side can move forward, but the passive side need to wait for peer to signal it's in sync_join condition */
    if ((!b_is_active) &&
        (!fbe_base_config_is_peer_clustered_flag_set((fbe_base_config_t*)provision_drive_p, 
                                                      FBE_BASE_CONFIG_CLUSTERED_FLAG_JOINED)))
    {
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p, FBE_TRACE_LEVEL_DEBUG_LOW, FBE_TRACE_MESSAGE_ID_INFO,
                              "Passive join sync, wait peer sync local: 0x%llx peer: 0x%llx state:0x%x pState:0x%x\n", 
                              (unsigned long long)local_flags, (unsigned long long)peer_flags,
                              local_state, peer_state);

        return FBE_LIFECYCLE_STATUS_DONE;
    }

    fbe_provision_drive_lock(provision_drive_p);

    /* We want to clear our local flags first.
     */
    if (fbe_base_config_is_any_clustered_flag_set((fbe_base_config_t*)provision_drive_p, 
                                (FBE_BASE_CONFIG_CLUSTERED_FLAG_JOIN_REQ |
                                 FBE_BASE_CONFIG_CLUSTERED_FLAG_JOIN_STARTED |
                                 FBE_BASE_CONFIG_CLUSTERED_FLAG_JOIN_SYNCING)))
    {
        /* At this point only started should be set, but we're clearing everything here.
         */
        fbe_base_config_clear_clustered_flag((fbe_base_config_t*)provision_drive_p, 
                                             (FBE_BASE_CONFIG_CLUSTERED_FLAG_JOIN_REQ |
                                              FBE_BASE_CONFIG_CLUSTERED_FLAG_JOIN_STARTED |
                                              FBE_BASE_CONFIG_CLUSTERED_FLAG_JOIN_SYNCING));
        /* Set joined so that we acknowledge the peer is present.
         */
        fbe_base_config_set_clustered_flag((fbe_base_config_t*)provision_drive_p, FBE_BASE_CONFIG_CLUSTERED_FLAG_JOINED);
        fbe_provision_drive_unlock(provision_drive_p);
        return FBE_LIFECYCLE_STATUS_RESCHEDULE;
    }

    fbe_provision_drive_check_hook(provision_drive_p,
                                   SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_JOIN,
                                   FBE_PROVISION_DRIVE_SUBSTATE_JOIN_SYNCED, 
                                   0,
                                   &hook_status);
    if (hook_status == FBE_SCHED_STATUS_DONE) {
        fbe_provision_drive_unlock(provision_drive_p);
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    b_peer_started_set = fbe_base_config_is_peer_clustered_flag_set((fbe_base_config_t*)provision_drive_p, 
                                                                    FBE_BASE_CONFIG_CLUSTERED_FLAG_JOIN_SYNCING);
    fbe_base_config_get_clustered_flags((fbe_base_config_t *) provision_drive_p, &local_flags);
    fbe_base_config_get_peer_clustered_flags((fbe_base_config_t *) provision_drive_p, &peer_flags);
    if (b_peer_started_set)
    {
        fbe_provision_drive_unlock(provision_drive_p);
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p, FBE_TRACE_LEVEL_DEBUG_LOW, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s join sync, wait peer done local: 0x%llx peer: 0x%llx state:0x%x\n", 
                              b_is_active ? "Active" : "Passive",
                              (unsigned long long)local_flags, (unsigned long long)peer_flags, local_state);
        return FBE_LIFECYCLE_STATUS_DONE;
    }
    else
    {
        fbe_status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)provision_drive_p);

        fbe_provision_drive_unlock(provision_drive_p);
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s join sync complete local: 0x%llx peer: 0x%llx state: 0x%x\n",
                              b_is_active ? "Active" : "Passive",
                              (unsigned long long)local_flags, (unsigned long long)peer_flags, local_state);

        /* If the PVD does not have same port/encl/slot, we need to fail the PVD.
         */
        if (!fbe_provision_drive_check_drive_location(provision_drive_p))
        {
            fbe_lifecycle_set_cond(&fbe_provision_drive_lifecycle_const,
                                   (fbe_base_object_t*)provision_drive_p,
                                   FBE_BASE_CONFIG_LIFECYCLE_COND_DOWNSTREAM_HEALTH_BROKEN);
        }

        if(fbe_base_config_is_load_balance_enabled() && b_is_active) {
            /* We don't load balance for system objects so that the system objects will 
             * not change sides at init time. 
             * This is expecially an issue for raw mirrors which cannot do a np persist 
             * on the passive side until the np LUN is up.
             */
            fbe_database_get_last_system_object_id(&last_system_object_id);
            fbe_base_object_get_object_id((fbe_base_object_t*)provision_drive_p, &pvd_object_id);
			fbe_provision_drive_metadata_get_nonpaged_metadata_drive_info(provision_drive_p, &np_metadata_drive_info);
            fbe_cmi_get_current_sp_state(&sp_state);

			if ((np_metadata_drive_info.slot_number % 2) && (pvd_object_id > last_system_object_id) && 
				(sp_state == FBE_CMI_STATE_ACTIVE) &&
                !fbe_provision_drive_is_peer_flag_set(provision_drive_p, FBE_PROVISION_DRIVE_CLUSTERED_FLAG_SLF))
            {
				fbe_base_config_passive_request((fbe_base_config_t *) provision_drive_p);
			}
		}
        return FBE_LIFECYCLE_STATUS_DONE;
    }
}
/**************************************
 * end fbe_provision_drive_join_sync_complete()
 **************************************/
/*************************
 * end file fbe_provision_drive_join.c
 *************************/
