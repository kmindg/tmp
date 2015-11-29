/***************************************************************************
 * Copyright (C) EMC Corporation 2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_extent_pool_lun_join.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the code for synchronizing the two SPs.
 *  This code allows the passive side to join the active side.
 *
 * @version
 *   03/27/2012 - Created. MFerson
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_types.h"
#include "fbe/fbe_transport.h"
#include "fbe_ext_pool_lun_private.h"
#include "fbe_testability.h"

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

/*!****************************************************************************
 * fbe_ext_pool_lun_join_request()
 ******************************************************************************
 * @brief
 *  This function implements the local join request state.
 *  In this state both SPs coordinate setting the "request" clustered flag.
 *  When both SPs have this request clustered flag set, we transition to the
 *  join started state.
 *
 * @param lun_p - lun object
 * @param packet_p - packet pointer.
 * 
 * @return fbe_lifecycle_status_t
 *
 * @author
 *  03/27/2012 - Created. MFerson
 *
 ******************************************************************************/
fbe_lifecycle_status_t 
fbe_ext_pool_lun_join_request(fbe_ext_pool_lun_t *  lun_p, fbe_packet_t * packet_p)
{
    fbe_bool_t b_peer_flag_set = FBE_FALSE;
    fbe_bool_t b_is_active;

    b_is_active = fbe_base_config_is_active((fbe_base_config_t*)lun_p);

    fbe_ext_pool_lun_lock(lun_p);

    /* If we don't already have the join local state set, we should set it now since
     * the first time through this will not be set. 
     */
    if (!fbe_lun_is_local_state_set(lun_p, FBE_LUN_LOCAL_STATE_JOIN_REQUEST))
    {
        //fbe_lun_clear_local_state(lun_p, FBE_LUN_LOCAL_STATE_JOIN_MASK);
        fbe_lun_set_local_state(lun_p, FBE_LUN_LOCAL_STATE_JOIN_REQUEST);
        fbe_base_config_clear_clustered_flag((fbe_base_config_t*)lun_p, 
                                             FBE_BASE_CONFIG_CLUSTERED_FLAG_JOIN_MASK);
        fbe_base_object_trace((fbe_base_object_t *)lun_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "LUN %s join request clear clustered flags local: 0x%llx peer: 0x%llx state: 0x%llx 0x%x\n", 
                              b_is_active ? "Active" : "Passive",
                              (unsigned long long)lun_p->lun_metadata_memory.base_config_metadata_memory.flags,
                              (unsigned long long)lun_p->lun_metadata_memory_peer.base_config_metadata_memory.flags,
                              (unsigned long long)lun_p->local_state,
                              lun_p->lun_metadata_memory_peer.base_config_metadata_memory.lifecycle_state);
    }

    /* We start the process by setting join request. 
     * This gets the peer running the correct condition. 
     */
    if (!fbe_base_config_is_clustered_flag_set((fbe_base_config_t*)lun_p, FBE_BASE_CONFIG_CLUSTERED_FLAG_JOIN_REQ))
    {
        fbe_base_object_trace((fbe_base_object_t *)lun_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "LUN %s join marking request local state: 0x%llx\n",
                              b_is_active ? "Active" : "Passive",
                              (unsigned long long)lun_p->local_state);
        fbe_base_config_set_clustered_flag((fbe_base_config_t*)lun_p, FBE_BASE_CONFIG_CLUSTERED_FLAG_JOIN_REQ);
        fbe_ext_pool_lun_unlock(lun_p);
        return FBE_LIFECYCLE_STATUS_RESCHEDULE;
    }

    /* Next we wait for the peer to tell us it received the join request.
     */
    b_peer_flag_set = fbe_base_config_is_peer_clustered_flag_set((fbe_base_config_t*)lun_p, 
                                                                 FBE_BASE_CONFIG_CLUSTERED_FLAG_JOIN_REQ);

    if (!b_peer_flag_set)
    {
        fbe_ext_pool_lun_unlock(lun_p);
        fbe_base_object_trace((fbe_base_object_t *)lun_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "LUN %s join wait peer request local: 0x%llx peer: 0x%llx state: 0x%llx\n", 
                              b_is_active ? "Active" : "Passive",
                              (unsigned long long)lun_p->lun_metadata_memory.base_config_metadata_memory.flags,
                              (unsigned long long)lun_p->lun_metadata_memory_peer.base_config_metadata_memory.flags,
                              (unsigned long long)lun_p->local_state);
        return FBE_LIFECYCLE_STATUS_DONE;
    }
    else
    {
        /* The peer has set the flag, we are ready to do the next set of checks.
         * We need to wait for the peer to get to the join request state as an acknowledgement that 
         * it knows we are starting this join. 
         */
        fbe_base_object_trace((fbe_base_object_t *)lun_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "LUN %s join peer request set local: 0x%llx peer: 0x%llx state: 0x%llx 0x%x\n", 
                              b_is_active ? "Active" : "Passive",
                              (unsigned long long)lun_p->lun_metadata_memory.base_config_metadata_memory.flags,
                              (unsigned long long)lun_p->lun_metadata_memory_peer.base_config_metadata_memory.flags,
                              (unsigned long long)lun_p->local_state,
                              lun_p->lun_metadata_memory_peer.base_config_metadata_memory.lifecycle_state);
        fbe_lun_set_local_state(lun_p, FBE_LUN_LOCAL_STATE_JOIN_STARTED);
        fbe_lun_clear_local_state(lun_p, FBE_LUN_LOCAL_STATE_JOIN_REQUEST);
        fbe_ext_pool_lun_unlock(lun_p);
        return FBE_LIFECYCLE_STATUS_RESCHEDULE;
    }
}
/**************************************
 * end fbe_ext_pool_lun_join_request()
 **************************************/

/*!****************************************************************************
 * fbe_ext_pool_lun_join_passive()
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
 * @param lun_p - lun object
 * @param packet_p - packet pointer.
 * 
 * @return fbe_status_t
 *
 * @author
 *  03/27/2012 - Created. MFerson
 *
 ******************************************************************************/
fbe_lifecycle_status_t 
fbe_ext_pool_lun_join_passive(fbe_ext_pool_lun_t *  lun_p, fbe_packet_t * packet_p)
{
    fbe_bool_t b_peer_flag_set = FBE_FALSE;
    fbe_bool_t b_local_flag_set = FBE_FALSE;
    fbe_bool_t b_is_active;

    fbe_ext_pool_lun_lock(lun_p);

    /* The active side always starts the operation. 
     * Once we see the active side has started the operation we will send over our bitmask. 
     */
    b_peer_flag_set = fbe_base_config_is_peer_clustered_flag_set((fbe_base_config_t*)lun_p, 
                                                                 FBE_BASE_CONFIG_CLUSTERED_FLAG_JOIN_STARTED);

    b_local_flag_set = fbe_base_config_is_clustered_flag_set((fbe_base_config_t*)lun_p, 
                                                             FBE_BASE_CONFIG_CLUSTERED_FLAG_JOIN_STARTED);

    fbe_base_object_trace((fbe_base_object_t *)lun_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "LUN PASSIVE join started clustered flags local: 0x%llx peer: 0x%llx state: 0x%llx 0x%x\n", 
                              (unsigned long long)lun_p->lun_metadata_memory.base_config_metadata_memory.flags,
                              (unsigned long long)lun_p->lun_metadata_memory_peer.base_config_metadata_memory.flags,
                              (unsigned long long)lun_p->local_state,
                              lun_p->lun_metadata_memory_peer.base_config_metadata_memory.lifecycle_state);

    /* We first wait for the peer to start the join. 
     * This is needed since either active or passive can initiate the join. 
     * Once we see this set, we will set our flag, which the peer waits for before it starts the join.
     */
    if (!b_peer_flag_set && !b_local_flag_set)
    {
        fbe_base_object_trace((fbe_base_object_t *)lun_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "LUN Passive waiting peer join started local: 0x%llx peer: 0x%llx state: 0x%llx\n",
                              (unsigned long long)lun_p->lun_metadata_memory.base_config_metadata_memory.flags,
                              (unsigned long long)lun_p->lun_metadata_memory_peer.base_config_metadata_memory.flags,
                              (unsigned long long)lun_p->local_state);
        fbe_ext_pool_lun_unlock(lun_p);
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /* At this point the peer started flag is set.
     */
    if (!b_local_flag_set)
    {
        /* Peer is starting the join.  We have not yet responded.
         */
        fbe_base_object_trace((fbe_base_object_t *)lun_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "LUN Passive marking join started state: 0x%llx\n", (unsigned long long)lun_p->local_state);
        /*! @todo This is the point where we would set in metadata memory the details of our health.
         */
        fbe_base_config_set_clustered_flag((fbe_base_config_t*)lun_p, FBE_BASE_CONFIG_CLUSTERED_FLAG_JOIN_STARTED);
        //fbe_base_config_clear_clustered_flag((fbe_base_config_t*)lun_p, FBE_BASE_CONFIG_CLUSTERED_FLAG_JOIN_REQ);
        fbe_ext_pool_lun_unlock(lun_p);
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /* If we become active, just return out.
     * We need to handle the active's work ourself. 
     * When we come back into the condition we'll automatically execute the active code.
     */
    b_is_active = fbe_base_config_is_active((fbe_base_config_t*)lun_p);
    if (b_is_active)
    {
        fbe_base_object_trace((fbe_base_object_t *)lun_p, FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                              "LUN Passive join became active local: 0x%llx peer: 0x%llx lstate: 0x%llx\n",
                              (unsigned long long)lun_p->lun_metadata_memory.base_config_metadata_memory.flags,
                              (unsigned long long)lun_p->lun_metadata_memory_peer.base_config_metadata_memory.flags,
                              (unsigned long long)lun_p->local_state);
        fbe_ext_pool_lun_unlock(lun_p);
        return FBE_LIFECYCLE_STATUS_RESCHEDULE;
    }
    /* Once we have set the started flag, we simply wait for the active side to complete. 
     * We know Active is done when it clears the flags. 
     */
    if (!b_peer_flag_set)
    {
        fbe_lun_set_local_state(lun_p, FBE_LUN_LOCAL_STATE_JOIN_DONE);
        fbe_lun_clear_local_state(lun_p, FBE_LUN_LOCAL_STATE_JOIN_STARTED);
        fbe_base_object_trace((fbe_base_object_t *)lun_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "LUN Passive join start cleanup local: 0x%llx peer: 0x%llx lstate: 0x%llx\n",
                              (unsigned long long)lun_p->lun_metadata_memory.base_config_metadata_memory.flags,
                              (unsigned long long)lun_p->lun_metadata_memory_peer.base_config_metadata_memory.flags,
                              (unsigned long long)lun_p->local_state);
        fbe_ext_pool_lun_unlock(lun_p);
        return FBE_LIFECYCLE_STATUS_RESCHEDULE;
    }
    else
    {
        fbe_ext_pool_lun_unlock(lun_p);
        fbe_base_object_trace((fbe_base_object_t *)lun_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "LUN Passive join wait peer complete local: 0x%llx peer: 0x%llx lstate: 0x%llx\n",
                              (unsigned long long)lun_p->lun_metadata_memory.base_config_metadata_memory.flags,
                              (unsigned long long)lun_p->lun_metadata_memory_peer.base_config_metadata_memory.flags,
                              (unsigned long long)lun_p->local_state);
    }
    return FBE_LIFECYCLE_STATUS_DONE;
}
/**************************************
 * end fbe_ext_pool_lun_join_passive()
 **************************************/

/*!****************************************************************************
 * fbe_ext_pool_lun_join_active()
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
 * @param lun_p - lun object
 * @param packet_p - packet pointer.
 * 
 * @return fbe_status_t
 *
 * @author
 *  03/27/2012 - Created. MFerson
 *
 ******************************************************************************/
fbe_lifecycle_status_t 
fbe_ext_pool_lun_join_active(fbe_ext_pool_lun_t *  lun_p, fbe_packet_t * packet_p)
{
    fbe_bool_t          b_peer_flag_set = FBE_FALSE;

    fbe_ext_pool_lun_lock(lun_p);

    /* Now we set the bit that says we are starting. 
     * This bit always gets set first by the Active side. 
     * When passive sees the started peer bit, it will also set the started bit. 
     * The passive side will thereafter know we are done when this bit is cleared. 
     */
    if (!fbe_base_config_is_clustered_flag_set((fbe_base_config_t*)lun_p, FBE_BASE_CONFIG_CLUSTERED_FLAG_JOIN_STARTED))
    {
        fbe_base_object_trace((fbe_base_object_t *)lun_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "LUN Active join mark started loc: 0x%llx peer: 0x%x state: 0x%llx\n",
                              (unsigned long long)lun_p->lun_metadata_memory.base_config_metadata_memory.flags,
                              (unsigned int)lun_p->lun_metadata_memory_peer.base_config_metadata_memory.flags,
                              (unsigned long long)lun_p->local_state);
        fbe_base_config_set_clustered_flag((fbe_base_config_t*)lun_p, FBE_BASE_CONFIG_CLUSTERED_FLAG_JOIN_STARTED);
        fbe_ext_pool_lun_unlock(lun_p);
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /* Now we need to wait for the peer to acknowledge that it saw us start.
     * By waiting for the passive side to get to this point, it allows the 
     * active side to clear the bits when it is complete. 
     * The passive side will also be able to know we are done when the bits are cleared.
     */
    b_peer_flag_set = fbe_base_config_is_peer_clustered_flag_set((fbe_base_config_t*)lun_p, 
                                                                 FBE_BASE_CONFIG_CLUSTERED_FLAG_JOIN_STARTED);
    if (!b_peer_flag_set)
    {
        fbe_ext_pool_lun_unlock(lun_p);
        fbe_base_object_trace((fbe_base_object_t *)lun_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "LUN Active join wait peer started local fl: 0x%llx peer fl: 0x%llx state: 0x%llx\n", 
                              (unsigned long long)lun_p->lun_metadata_memory.base_config_metadata_memory.flags,
                              (unsigned long long)lun_p->lun_metadata_memory_peer.base_config_metadata_memory.flags,
                              (unsigned long long)lun_p->local_state);
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /* Peer sees we are starting and has sent us the bitmask.
     */
    fbe_lun_set_local_state(lun_p, FBE_LUN_LOCAL_STATE_JOIN_DONE);
    fbe_lun_clear_local_state(lun_p, FBE_LUN_LOCAL_STATE_JOIN_STARTED);

    /*! @todo theoretically if neither side has anything to do, we could just transition to done.
     */
    fbe_ext_pool_lun_unlock(lun_p);
    return FBE_LIFECYCLE_STATUS_RESCHEDULE;
}
/**************************************
 * end fbe_ext_pool_lun_join_active()
 **************************************/

/*!****************************************************************************
 * fbe_ext_pool_lun_join_done()
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
 * @param lun_p - lun object
 * @param packet_p - packet pointer.
 * 
 * @return fbe_lifecycle_status_t
 *
 * @author
 *  03/27/2012 - Created. MFerson
 *
 ******************************************************************************/
fbe_lifecycle_status_t 
fbe_ext_pool_lun_join_done(fbe_ext_pool_lun_t *  lun_p, fbe_packet_t * packet_p)
{
    fbe_status_t status;
    fbe_bool_t b_is_active;
    fbe_bool_t b_peer_started_set = FBE_FALSE;

    b_is_active = fbe_base_config_is_active((fbe_base_config_t*)lun_p);

    fbe_ext_pool_lun_lock(lun_p);


    /* We want to clear our local flags first.
     */
    if (b_is_active && fbe_base_config_is_clustered_flag_set((fbe_base_config_t*)lun_p, FBE_BASE_CONFIG_CLUSTERED_FLAG_JOIN_STARTED))
    {
        /* At this point only started should be set
         */
        fbe_base_config_clear_clustered_flag((fbe_base_config_t*)lun_p, FBE_BASE_CONFIG_CLUSTERED_FLAG_JOIN_STARTED);
        /* Set join_syncing so that we acknowledge the peer the active side is ready.
         * The passive side sets the syncing bit in pending ready.
         */
        if (!fbe_base_config_is_clustered_flag_set((fbe_base_config_t*)lun_p, FBE_BASE_CONFIG_CLUSTERED_FLAG_JOIN_SYNCING))
        {
            fbe_base_config_set_clustered_flag((fbe_base_config_t*)lun_p, FBE_BASE_CONFIG_CLUSTERED_FLAG_JOIN_SYNCING);
        }
        fbe_ext_pool_lun_unlock(lun_p);
        return FBE_LIFECYCLE_STATUS_RESCHEDULE;
    }

    b_peer_started_set = fbe_base_config_is_peer_clustered_flag_set((fbe_base_config_t*)lun_p, 
                                                                    FBE_BASE_CONFIG_CLUSTERED_FLAG_JOIN_SYNCING);
    if (!b_peer_started_set)
    {
        fbe_ext_pool_lun_unlock(lun_p);
        fbe_base_object_trace((fbe_base_object_t *)lun_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "LUN %s join cleanup, wait peer done local: 0x%llx peer: 0x%llx state:0x%llx\n", 
                              b_is_active ? "Active" : "Passive",
                              (unsigned long long)lun_p->lun_metadata_memory.base_config_metadata_memory.flags,
                              (unsigned long long)lun_p->lun_metadata_memory_peer.base_config_metadata_memory.flags,
                              (unsigned long long)lun_p->local_state);
        return FBE_LIFECYCLE_STATUS_DONE;
    }
    else
    {
        /* The local flags tell us that we are waiting for the peer to clear their flags.
         * Since it seems they are complete, it is safe to clear our local flags. 
         */
        fbe_lun_clear_local_state(lun_p, FBE_LUN_LOCAL_STATE_JOIN_STARTED | FBE_LUN_LOCAL_STATE_JOIN_DONE);

        /* It is possible that we received a request to join either locally or from the peer.
         * If a request is not set, let's clear the condition.  Otherwise we will run this condition again. 
         */
        if (!fbe_lun_is_local_state_set(lun_p, FBE_LUN_LOCAL_STATE_JOIN_REQUEST))
        {
            fbe_base_object_trace((fbe_base_object_t *)lun_p, 
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "LUN %s clearing current condition\n", __FUNCTION__);

            status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)lun_p);
            
            if (status != FBE_STATUS_OK)
            {
                fbe_base_object_trace((fbe_base_object_t *)lun_p, 
                                      FBE_TRACE_LEVEL_ERROR,
                                      FBE_TRACE_MESSAGE_ID_INFO,
                                      "LUN %s can't clear current condition, status: 0x%x\n",
                                      __FUNCTION__, status);
            }
            if (b_is_active)
            {
                /* We need to move into join_sync condition to finish up, if the passive side has gone through pending ready */
                status = fbe_lifecycle_set_cond(&fbe_ext_pool_lun_lifecycle_const, (fbe_base_object_t*) lun_p, 
                                       FBE_BASE_CONFIG_LIFECYCLE_COND_JOIN_SYNC);
                if (status != FBE_STATUS_OK)
                {
                    fbe_base_object_trace((fbe_base_object_t*)lun_p, 
                                            FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                            "LUN %s failed to set join_sync condition %d\n", __FUNCTION__, status);
                }
            }
            fbe_ext_pool_lun_unlock(lun_p);
            fbe_base_object_trace((fbe_base_object_t *)lun_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                  "LUN %s join cleanup complete local: 0x%llx peer: 0x%llx state: 0x%llx\n",
                                  b_is_active ? "Active" : "Passive",
                                  (unsigned long long)lun_p->lun_metadata_memory.base_config_metadata_memory.flags,
                                  (unsigned long long)lun_p->lun_metadata_memory_peer.base_config_metadata_memory.flags,
                                  (unsigned long long)lun_p->local_state);
        }


        return FBE_LIFECYCLE_STATUS_RESCHEDULE;
    }
}
/**************************************
 * end fbe_ext_pool_lun_join_done()
 **************************************/

/*!****************************************************************************
 * fbe_ext_pool_lun_join_sync_complete()
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
 * @param lun_p - lun object
 * @param packet_p - packet pointer.
 * 
 * @return fbe_lifecycle_status_t
 *
 * @author
 *  03/27/2012 - Created. MFerson
 *
 ******************************************************************************/
fbe_lifecycle_status_t 
fbe_ext_pool_lun_join_sync_complete(fbe_ext_pool_lun_t *  lun_p, fbe_packet_t * packet_p)
{
    fbe_bool_t b_is_active;
    fbe_bool_t b_peer_started_set = FBE_FALSE;
    fbe_status_t        fbe_status = FBE_STATUS_OK;
    fbe_lifecycle_state_t   local_state= FBE_LIFECYCLE_STATE_INVALID, peer_state = FBE_LIFECYCLE_STATE_INVALID;
    fbe_base_config_clustered_flags_t local_flags = 0, peer_flags = 0;

    b_is_active = fbe_base_config_is_active((fbe_base_config_t*)lun_p);
    fbe_base_config_metadata_get_peer_lifecycle_state((fbe_base_config_t *)lun_p, &peer_state);
    fbe_base_config_metadata_get_lifecycle_state((fbe_base_config_t *)lun_p, &local_state);
    fbe_base_config_get_clustered_flags((fbe_base_config_t *) lun_p, &local_flags);
    fbe_base_config_get_peer_clustered_flags((fbe_base_config_t *) lun_p, &peer_flags);

    /* active side can move forward, but the passive side need to wait for peer to signal it's in sync_join condition */
    if ((!b_is_active) &&
        (!fbe_base_config_is_peer_clustered_flag_set((fbe_base_config_t*)lun_p, 
                                                      FBE_BASE_CONFIG_CLUSTERED_FLAG_JOINED)))
    {
        fbe_base_object_trace((fbe_base_object_t *)lun_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "LUN Passive join sync, wait peer sync local: 0x%llx peer: 0x%llx state:0x%llx pState:0x%llx\n", 
                              (unsigned long long)local_flags, (unsigned long long)peer_flags,
                              (unsigned long long)local_state, (unsigned long long)peer_state);
        return FBE_LIFECYCLE_STATUS_DONE;
    }
    fbe_ext_pool_lun_lock(lun_p);

    /* We want to clear our local flags first.
     */
    if (fbe_base_config_is_any_clustered_flag_set((fbe_base_config_t*)lun_p, 
                                (FBE_BASE_CONFIG_CLUSTERED_FLAG_JOIN_REQ |
                                 FBE_BASE_CONFIG_CLUSTERED_FLAG_JOIN_STARTED |
                                 FBE_BASE_CONFIG_CLUSTERED_FLAG_JOIN_SYNCING)))
    {
        /* At this point only started should be set, but we're clearing everything here.
         */
        fbe_base_config_clear_clustered_flag((fbe_base_config_t*)lun_p, 
                                             (FBE_BASE_CONFIG_CLUSTERED_FLAG_JOIN_REQ |
                                              FBE_BASE_CONFIG_CLUSTERED_FLAG_JOIN_STARTED |
                                              FBE_BASE_CONFIG_CLUSTERED_FLAG_JOIN_SYNCING));
        /* Set joined so that we acknowledge the peer is present.
         */
        fbe_base_config_set_clustered_flag((fbe_base_config_t*)lun_p, FBE_BASE_CONFIG_CLUSTERED_FLAG_JOINED);
        fbe_ext_pool_lun_unlock(lun_p);
        return FBE_LIFECYCLE_STATUS_RESCHEDULE;
    }

    b_peer_started_set = fbe_base_config_is_peer_clustered_flag_set((fbe_base_config_t*)lun_p, 
                                                                    FBE_BASE_CONFIG_CLUSTERED_FLAG_JOIN_SYNCING);
    fbe_base_config_get_clustered_flags((fbe_base_config_t *) lun_p, &local_flags);
    fbe_base_config_get_peer_clustered_flags((fbe_base_config_t *) lun_p, &peer_flags);
    if (b_peer_started_set)
    {
        fbe_ext_pool_lun_unlock(lun_p);
        fbe_base_object_trace((fbe_base_object_t *)lun_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "LUN %s join sync, wait peer done local: 0x%llx peer: 0x%llx state:0x%llx\n", 
                              b_is_active ? "Active" : "Passive",
                              (unsigned long long)local_flags, (unsigned long long)peer_flags, (unsigned long long)local_state);
        return FBE_LIFECYCLE_STATUS_DONE;
    }
    else
    {
        fbe_status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)lun_p);
        if (fbe_status != FBE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t *)lun_p, 
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "LUN %s can't clear current condition, status: 0x%x\n",
                                  __FUNCTION__, fbe_status);
        }
        fbe_ext_pool_lun_unlock(lun_p);
        fbe_base_object_trace((fbe_base_object_t *)lun_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "LUN %s join sync complete local: 0x%llx peer: 0x%llx state: 0x%llx\n",
                              b_is_active ? "Active" : "Passive",
                              (unsigned long long)local_flags, (unsigned long long)peer_flags, (unsigned long long)local_state);

        return FBE_LIFECYCLE_STATUS_RESCHEDULE;
    }
}
/**************************************
 * end fbe_ext_pool_lun_join_sync_complete()
 **************************************/
/*************************
 * end file fbe_extent_pool_lun_join.c
 *************************/
