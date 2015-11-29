/***************************************************************************
 * Copyright (C) EMC Corporation 2012
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_provision_drive_health_check.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the code for synchronizing the two SPs for drive
 *  health check.
 *
 * @version
 *   07/02/2012 Arun Joseph - Adapted from f/w download code
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
#include "fbe_service_manager.h"
#include "fbe_cmi.h"


/*************************
 *   FUNCTION DEFINITIONS
 *************************/
static fbe_bool_t               fbe_provision_drive_is_peer_ready_for_health_check(fbe_provision_drive_t * provision_drive_p);
static fbe_bool_t               fbe_provision_drive_is_peer_alive_for_health_check(fbe_provision_drive_t * provision_drive_p);
static fbe_lifecycle_status_t   fbe_provision_drive_send_health_check_command_to_pdo(fbe_provision_drive_t * provision_drive_p, fbe_packet_t * packet_p, fbe_physical_drive_health_check_req_status_t request_status);
static fbe_status_t             fbe_provision_drive_send_health_check_pdo_proceed_completion(fbe_packet_t * packet_p, fbe_packet_completion_context_t context);
static fbe_status_t             fbe_provision_drive_send_health_check_pdo_abort_completion(fbe_packet_t * packet_p, fbe_packet_completion_context_t context);


/*!****************************************************************************
 * fbe_provision_drive_health_check_request()
 ******************************************************************************
 * @brief
 *  This function implements the local health_check request state.
 *  In this state both SPs coordinate setting the "request" clustered flag.
 *  When both SPs have this request clustered flag set, the state is transitted
 *  to started state.
 *
 * @param provision_drive_p - provision drive object
 * @param packet_p - packet pointer.
 * 
 * @return fbe_lifecycle_status_t
 *
 * @author
 *   07/04/2012 - Arun Joseph - Created. 
 *
 ******************************************************************************/
fbe_lifecycle_status_t 
fbe_provision_drive_health_check_request(fbe_provision_drive_t *  provision_drive_p, fbe_packet_t * packet_p)
{
    fbe_bool_t b_peer_flag_set = FBE_FALSE;
    fbe_bool_t b_is_originator;
    fbe_provision_drive_clustered_flags_t peer_clustered_flags;

    b_is_originator = fbe_provision_drive_is_flag_set(provision_drive_p, FBE_PROVISION_DRIVE_FLAG_HC_ORIGINATOR);

    fbe_provision_drive_lock(provision_drive_p);

    /* If we are non-originator, and originator has cleared cluster flags, we should quit. 
     */
    fbe_provision_drive_get_peer_clustered_flags(provision_drive_p, &peer_clustered_flags);
    if (!b_is_originator &&
        !(peer_clustered_flags & (FBE_PROVISION_DRIVE_CLUSTERED_FLAG_HEALTH_CHECK_REQUEST |
                                  FBE_PROVISION_DRIVE_CLUSTERED_FLAG_HEALTH_CHECK_STARTED) ))
    {
        fbe_provision_drive_set_local_state(provision_drive_p, FBE_PVD_LOCAL_STATE_HEALTH_CHECK_DONE);
        fbe_provision_drive_clear_local_state(provision_drive_p, FBE_PVD_LOCAL_STATE_HEALTH_CHECK_REQUEST);
        fbe_provision_drive_clear_clustered_flag(provision_drive_p, 
                                             FBE_PROVISION_DRIVE_CLUSTERED_FLAG_HEALTH_CHECK_MASK);
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "PVD_HC Request %s: quit health_check: 0x%llx peer: 0x%llx\n", 
                              b_is_originator ? "Originator" : "Non-Originator",
                              (unsigned long long)provision_drive_p->provision_drive_metadata_memory.flags,
                              (unsigned long long)provision_drive_p->provision_drive_metadata_memory_peer.flags);
        fbe_provision_drive_unlock(provision_drive_p);
        return FBE_LIFECYCLE_STATUS_RESCHEDULE;
    }

    /* If we don't already have the request state set, we should set it now. 
     */
    if (!fbe_provision_drive_is_local_state_set(provision_drive_p, FBE_PVD_LOCAL_STATE_HEALTH_CHECK_REQUEST))
    {
        fbe_provision_drive_set_local_state(provision_drive_p, FBE_PVD_LOCAL_STATE_HEALTH_CHECK_REQUEST);
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "PVD_HC Request %s: clear clustered flags local: 0x%llx peer: 0x%llx\n", 
                              b_is_originator ? "Originator" : "Non-Originator",
                              (unsigned long long)provision_drive_p->provision_drive_metadata_memory.flags,
                              (unsigned long long)provision_drive_p->provision_drive_metadata_memory_peer.flags);
    }

    /* We start the process by setting health_check request. 
     * This gets the peer running the correct condition. 
     */
    if (!fbe_provision_drive_is_clustered_flag_set(provision_drive_p, FBE_PROVISION_DRIVE_CLUSTERED_FLAG_HEALTH_CHECK_REQUEST))
    {
        fbe_provision_drive_set_clustered_flag(provision_drive_p, FBE_PROVISION_DRIVE_CLUSTERED_FLAG_HEALTH_CHECK_REQUEST);
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "PVD_HC Request %s: marking request local state: 0x%llx\n",
                              b_is_originator ? "Originator" : "Non-Originator",
                              (unsigned long long)provision_drive_p->local_state);
        fbe_provision_drive_unlock(provision_drive_p);
        return FBE_LIFECYCLE_STATUS_RESCHEDULE;
    }

    /* Next we wait for the peer to tell us it received the health_check request.  If peer is gone or not in ready 
       state then go directly to next state.
     */
    if (b_is_originator && fbe_provision_drive_is_peer_ready_for_health_check(provision_drive_p))
    {
        b_peer_flag_set = fbe_provision_drive_is_peer_clustered_flag_set(provision_drive_p, 
                                                                 FBE_PROVISION_DRIVE_CLUSTERED_FLAG_HEALTH_CHECK_REQUEST);
        if (!b_peer_flag_set)
        {
            fbe_base_object_trace((fbe_base_object_t *)provision_drive_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                  "PVD_HC Request %s: wait peer request local: 0x%llx peer: 0x%llx\n", 
                                  b_is_originator ? "Originator" : "Non-Originator",
                                  (unsigned long long)provision_drive_p->provision_drive_metadata_memory.flags,
                                  (unsigned long long)provision_drive_p->provision_drive_metadata_memory_peer.flags);
            fbe_provision_drive_unlock(provision_drive_p);
            return FBE_LIFECYCLE_STATUS_DONE;
        }
    }

    /* Both local and peer has request flag set, we are ready for the next state.
     */
    fbe_base_object_trace((fbe_base_object_t *)provision_drive_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                          "PVD_HC Request %s: local: 0x%llx peer: 0x%llx state: 0x%llx life: %d\n", 
                          b_is_originator ? "Originator" : "Non-Originator",
                          provision_drive_p->provision_drive_metadata_memory.flags,
                          provision_drive_p->provision_drive_metadata_memory_peer.flags,
                          provision_drive_p->local_state,
                          provision_drive_p->provision_drive_metadata_memory_peer.base_config_metadata_memory.lifecycle_state);
    if (b_is_originator)
    {
        fbe_provision_drive_set_local_state(provision_drive_p, FBE_PVD_LOCAL_STATE_HEALTH_CHECK_STARTED);
    }
    else
    {
        fbe_provision_drive_set_local_state(provision_drive_p, FBE_PVD_LOCAL_STATE_HEALTH_CHECK_PEER_START);
    }
    fbe_provision_drive_clear_local_state(provision_drive_p, FBE_PVD_LOCAL_STATE_HEALTH_CHECK_REQUEST);
    fbe_provision_drive_unlock(provision_drive_p);

    /* Set the start time now.
     
    fbe_provision_drive_set_health_check_start_time(provision_drive_p, fbe_get_time());
    */
    return FBE_LIFECYCLE_STATUS_RESCHEDULE;
}
/**************************************
 * end fbe_provision_drive_health_check_request()
 **************************************/

/*!****************************************************************************
 * fbe_provision_drive_health_check_abort_req_originator()
 ******************************************************************************
 * @brief
 *  This function aborts the originator health check request.
 *
 * @param provision_drive_p - provision drive object
 * @param packet_p - packet pointer.
 * 
 * @return fbe_status_t
 *
 * @author
 *   01/10/2013  Wayne Garrett - Created. 
 *
 ******************************************************************************/

fbe_lifecycle_status_t 
fbe_provision_drive_health_check_abort_req_originator(fbe_provision_drive_t *  provision_drive_p, fbe_packet_t * packet_p)
{
    /* note - this side could be acting as passive role when it also got request to do HC.  We need 
       to abort the request, but want it to still act as the HC peer (non-originator).  So we'll simply 
       clear the ABORT_REQ flag and leave everything else alone.
    */

    fbe_provision_drive_lock(provision_drive_p);
    fbe_provision_drive_clear_local_state(provision_drive_p, FBE_PVD_LOCAL_STATE_HEALTH_CHECK_ABORT_REQ);
    fbe_provision_drive_unlock(provision_drive_p);


    return fbe_provision_drive_send_health_check_command_to_pdo(provision_drive_p, packet_p, 
                                                                FBE_DRIVE_HC_STATUS_ABORT);
}

/*!****************************************************************************
 * fbe_provision_drive_health_check_started_originator()
 ******************************************************************************
 * @brief
 *  This function implements the health_check started state for originator side.
 *
 * @param provision_drive_p - provision drive object
 * @param packet_p - packet pointer.
 * 
 * @return fbe_status_t
 *
 * @author
 *   07/04/2012 - Arun Joseph. Created. 
 *
 ******************************************************************************/
fbe_lifecycle_status_t 
fbe_provision_drive_health_check_started_originator(fbe_provision_drive_t *  provision_drive_p, fbe_packet_t * packet_p)
{
    fbe_bool_t          b_peer_flag_set = FBE_FALSE;    
    fbe_lifecycle_status_t  return_status;

#if 0 //Transition to ACTIVATE should cause IO to quiesce. 
    /* Quiesce the IO before proceeding */
    fbe_provision_drive_utils_quiesce_io_if_needed(provision_drive_p, &b_io_already_quiesced);
    if (b_io_already_quiesced == FBE_FALSE)
    {
        return FBE_LIFECYCLE_STATUS_DONE;
    }
#endif
    fbe_base_object_trace((fbe_base_object_t*) provision_drive_p,      
                          FBE_TRACE_LEVEL_INFO,           
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,  
                          "PVD_HC Originator Started: io quiesced\n");

    fbe_provision_drive_lock(provision_drive_p);

    /* Now we set the bit that says we are starting. 
     * This bit always gets set first by the originator side. 
     * When the non-originator sees the started peer bit, it will also set the started bit. 
     * The non-originator side will thereafter know we are done when this bit is cleared. 
     */
    if (!fbe_provision_drive_is_clustered_flag_set(provision_drive_p, FBE_PROVISION_DRIVE_CLUSTERED_FLAG_HEALTH_CHECK_STARTED))
    {
        fbe_provision_drive_clear_clustered_flag(provision_drive_p, FBE_PROVISION_DRIVE_CLUSTERED_FLAG_HEALTH_CHECK_REQUEST);
        fbe_provision_drive_set_clustered_flag(provision_drive_p, FBE_PROVISION_DRIVE_CLUSTERED_FLAG_HEALTH_CHECK_STARTED);
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "PVD_HC Originator Started: mark started local: 0x%llx peer: 0x%llx state: 0x%llx\n",
                              (unsigned long long)provision_drive_p->provision_drive_metadata_memory.flags,
                              (unsigned long long)provision_drive_p->provision_drive_metadata_memory_peer.flags,
                              (unsigned long long)provision_drive_p->local_state);
        fbe_provision_drive_unlock(provision_drive_p);
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /* Now we need to wait for the peer to acknowledge that it saw us start.
     */
    if (fbe_provision_drive_is_peer_ready_for_health_check(provision_drive_p))
    {
        b_peer_flag_set = fbe_provision_drive_is_peer_clustered_flag_set(provision_drive_p, 
                                                                 FBE_PROVISION_DRIVE_CLUSTERED_FLAG_HEALTH_CHECK_STARTED);
        if (!b_peer_flag_set)
        {
            fbe_base_object_trace((fbe_base_object_t *)provision_drive_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                  "PVD_HC Originator Started: wait peer started local: 0x%llx peer: 0x%llx\n", 
                                  (unsigned long long)provision_drive_p->provision_drive_metadata_memory.flags,
                                  (unsigned long long)provision_drive_p->provision_drive_metadata_memory_peer.flags);
            fbe_provision_drive_unlock(provision_drive_p);
            return FBE_LIFECYCLE_STATUS_DONE;
        }
    }
    /* else we continue on as single SP case*/


    /* The following usurper cmd will cause PDO to issue QST then drive reset.  We are now waiting for that
       to complete.  If failure occurs sending control op to PDO then the completion function will handle correctly. 
    */
    fbe_provision_drive_set_local_state(provision_drive_p, FBE_PVD_LOCAL_STATE_HEALTH_CHECK_WAITING);
    fbe_provision_drive_clear_local_state(provision_drive_p, FBE_PVD_LOCAL_STATE_HEALTH_CHECK_STARTED);

    fbe_provision_drive_unlock(provision_drive_p);

    /* Start the actual health_check.
     */
    return_status = fbe_provision_drive_send_health_check_command_to_pdo(provision_drive_p, packet_p, 
                                                                         FBE_DRIVE_HC_STATUS_PROCEED);
    return return_status;
}
/**************************************
 * end fbe_provision_drive_health_check_started_originator()
 **************************************/

/*!****************************************************************************
 * fbe_provision_drive_health_check_started_originator()
 ******************************************************************************
 * @brief
 *  This function is waiting for PDO to complete the HC before cleaning up,
 *  which invovles notifying peer to go back to Ready state.
 *
 * @param provision_drive_p - provision drive object
 * @param packet_p - packet pointer.
 * 
 * @return fbe_status_t
 *
 * @author
 *   09/12/2012  Wayne Garrett - Created. 
 *
 ******************************************************************************/
fbe_lifecycle_status_t fbe_provision_drive_health_check_waiting_originator(fbe_provision_drive_t *  provision_drive_p, fbe_packet_t * packet_p)
{
    fbe_lifecycle_status_t return_status = FBE_LIFECYCLE_STATUS_DONE;

    /* wait for PDO to complete HC and go back to either Ready or Failed or Destroyed.*/
    if (FBE_DOWNSTREAM_HEALTH_DISABLED != fbe_provision_drive_verify_downstream_health(provision_drive_p))
    {
        /* HC completed, failed or PDO was destroyed.  Time to exist HC state machine. */
        fbe_provision_drive_lock(provision_drive_p);
        fbe_provision_drive_set_local_state(provision_drive_p, FBE_PVD_LOCAL_STATE_HEALTH_CHECK_DONE);
        fbe_provision_drive_clear_local_state(provision_drive_p, FBE_PVD_LOCAL_STATE_HEALTH_CHECK_WAITING);
        fbe_provision_drive_unlock(provision_drive_p);

        /* process next state in HC state machine */
        return_status = FBE_LIFECYCLE_STATUS_RESCHEDULE;  
    }
    else
    {
        /* HC is not finished yet, reschedule and return DONE indicating no further processing until
           HC completes this state. */        

        /* Reschedule for 0.5 second. */
        fbe_lifecycle_reschedule(&fbe_provision_drive_lifecycle_const, 
                                 (fbe_base_object_t*)provision_drive_p, (fbe_lifecycle_timer_msec_t)500);

        return_status = FBE_LIFECYCLE_STATUS_DONE;  
    }

    return return_status;
}


/*!****************************************************************************
 * fbe_provision_drive_health_check_done()
 ******************************************************************************
 * @brief
 *  This function implements the health_check done local state.
 *
 * @param provision_drive_p - provision drive object
 * @param packet_p - packet pointer.
 * 
 * @return fbe_lifecycle_status_t
 *
 * @author
 *   07/04/2012 - Arun Joseph - Created. 
 *
 ******************************************************************************/
fbe_lifecycle_status_t 
fbe_provision_drive_health_check_done(fbe_provision_drive_t *  provision_drive_p, fbe_packet_t * packet_p)
{
    fbe_status_t status;
    fbe_bool_t b_is_originator;
    fbe_base_config_downstream_health_state_t   downstream_health_state;
    fbe_path_attr_t                             path_attr = 0;

    b_is_originator = fbe_provision_drive_is_flag_set(provision_drive_p, FBE_PROVISION_DRIVE_FLAG_HC_ORIGINATOR);

    /* Clear local state.
     */
    fbe_provision_drive_lock(provision_drive_p);
    fbe_provision_drive_clear_local_state(provision_drive_p, 
                                          FBE_PVD_LOCAL_STATE_HEALTH_CHECK_MASK);
    fbe_base_object_trace((fbe_base_object_t *)provision_drive_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                          "PVD_HC %s Done: local:0x%llx peer:0x%llx state:0x%llx\n", 
                          b_is_originator ? "Originator" : "Non-Originator",
                          (unsigned long long)provision_drive_p->provision_drive_metadata_memory.flags,
                          (unsigned long long)provision_drive_p->provision_drive_metadata_memory_peer.flags,
                          (unsigned long long)provision_drive_p->local_state);
        
    fbe_provision_drive_unlock(provision_drive_p);


    /* If we are non-originator then we need to clear the condition that's holding us in 
       activate state.
     */
    if (!b_is_originator)
    {
        status = fbe_lifecycle_force_clear_cond(&fbe_provision_drive_lifecycle_const, (fbe_base_object_t*)provision_drive_p,
                                                FBE_PROVISION_DRIVE_LIFECYCLE_COND_HEALTH_CHECK_PASSIVE_SIDE);

        if (status != FBE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t *)provision_drive_p, 
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "PVD_HC Done: can't clear current condition, status: 0x%x\n", status);
        }
    
    }

    /* Get the current path attr.
     */
    status = fbe_block_transport_get_path_attributes(((fbe_base_config_t *)provision_drive_p)->block_edge_p,
                                                     &path_attr);
    if (status != FBE_STATUS_OK)
    {
        fbe_provision_drive_utils_trace(provision_drive_p, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                        FBE_PROVISION_DRIVE_DEBUG_FLAG_GENERAL_TRACING,
                                        "PVD_HC %s Done: failed to get path attr.\n", b_is_originator ? "Originator" : "Non-Originator");
    }

    /* We might be blocking any downstream edge state change event. Check the state now
     * to determine if we need state change. 
     */
    downstream_health_state = fbe_provision_drive_verify_downstream_health(provision_drive_p);

    /* Set the specific lifecycle condition based on health of the downtream path. */
    status =  fbe_provision_drive_set_condition_based_on_downstream_health(provision_drive_p,
                                                                           path_attr,
                                                                           downstream_health_state);
    /* Clear all clustered masks. */
    fbe_provision_drive_clear_clustered_flag(provision_drive_p, 
                                             FBE_PROVISION_DRIVE_CLUSTERED_FLAG_HEALTH_CHECK_MASK);

    fbe_provision_drive_set_health_check_originator(provision_drive_p, FBE_FALSE);

    /* HC prep is done.   Let remaining condition checks continue. */
    return FBE_LIFECYCLE_STATUS_CONTINUE;
}

/**************************************
 * end fbe_provision_drive_health_check_done()
 **************************************/


/*!****************************************************************************
 * fbe_provision_drive_health_check_failed()
 ******************************************************************************
 * @brief
 *  This function implements the health_check failed state for the originator side.
 *
 * @param provision_drive_p - provision drive object
 * @param packet_p - packet pointer.
 * 
 * @return fbe_status_t
 *
 * @author
 *   07/04/2012 - Arun Joseph - Created. 
 *
 ******************************************************************************/
fbe_lifecycle_status_t 
fbe_provision_drive_health_check_failed(fbe_provision_drive_t *  provision_drive_p, fbe_packet_t * packet_p)
{
    fbe_bool_t          b_is_originator;

    b_is_originator = fbe_provision_drive_is_flag_set(provision_drive_p, FBE_PROVISION_DRIVE_FLAG_HC_ORIGINATOR);

    fbe_provision_drive_lock(provision_drive_p);

    /* We want to clear our local flags first.
     */
    if (fbe_provision_drive_is_clustered_flag_set(provision_drive_p, FBE_PROVISION_DRIVE_CLUSTERED_FLAG_HEALTH_CHECK_REQUEST) ||
        fbe_provision_drive_is_clustered_flag_set(provision_drive_p, FBE_PROVISION_DRIVE_CLUSTERED_FLAG_HEALTH_CHECK_STARTED))
    {
        /* Set health_checked failed so that we notify peer of failure.
         */
        fbe_provision_drive_set_clustered_flag(provision_drive_p, FBE_PROVISION_DRIVE_CLUSTERED_FLAG_HEALTH_CHECK_FAILED);

        /* At this point only failed should be set,  we're clearing everything else here.
         */
        fbe_provision_drive_clear_clustered_flag(provision_drive_p, 
                                             (FBE_PROVISION_DRIVE_CLUSTERED_FLAG_HEALTH_CHECK_REQUEST |
                                              FBE_PROVISION_DRIVE_CLUSTERED_FLAG_HEALTH_CHECK_STARTED));

        fbe_provision_drive_unlock(provision_drive_p);
        return FBE_LIFECYCLE_STATUS_RESCHEDULE;
    }

    /* Wait till peer has acknowledges the failed state of HC. If peer is not alive or broken then skip peer handshaking and go 
       to next state.  Treat as a single SP scenario */
    if (fbe_provision_drive_is_peer_ready_for_health_check(provision_drive_p) &&
        !fbe_base_config_is_peer_clustered_flag_set((fbe_base_config_t*)provision_drive_p, FBE_BASE_CONFIG_CLUSTERED_FLAG_BROKEN))
    {
        /* waiting for peer to either to go failed or clean up. */
        if (!fbe_provision_drive_is_peer_clustered_flag_set(provision_drive_p, FBE_PROVISION_DRIVE_CLUSTERED_FLAG_HEALTH_CHECK_FAILED) &&
            ( fbe_provision_drive_is_peer_clustered_flag_set(provision_drive_p, FBE_PROVISION_DRIVE_CLUSTERED_FLAG_HEALTH_CHECK_REQUEST) ||
              fbe_provision_drive_is_peer_clustered_flag_set(provision_drive_p, FBE_PROVISION_DRIVE_CLUSTERED_FLAG_HEALTH_CHECK_STARTED) ))            
        {
            fbe_base_object_trace((fbe_base_object_t *)provision_drive_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                  "PVD_HC %s Failed: wait peer. local: 0x%llx peer: 0x%llx state:0x%llx\n",                                   
                                  b_is_originator ? "Originator" : "Non-Originator",
                                  (unsigned long long)provision_drive_p->provision_drive_metadata_memory.flags,
                                  (unsigned long long)provision_drive_p->provision_drive_metadata_memory_peer.flags,
                                  (unsigned long long)provision_drive_p->local_state);
            fbe_provision_drive_unlock(provision_drive_p);
            return FBE_LIFECYCLE_STATUS_DONE;
        }
    }

    /* Peer has acknowledged the failed state of HC. */    

    fbe_provision_drive_unlock(provision_drive_p);


    fbe_base_object_trace((fbe_base_object_t *)provision_drive_p, FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                            "PVD_HC %s Failed: Stopping.\n", b_is_originator ? "Originator" : "Non-Originator");

    return fbe_provision_drive_health_check_done(provision_drive_p, packet_p);
}
/**************************************
 * end fbe_provision_drive_health_check_failed()
 **************************************/

/*!****************************************************************************
 * fbe_provision_drive_health_check_start_non_originator()
 ******************************************************************************
 * @brief
 *  This function implements the health_check started state for non-originator side.
 * 
 * @param provision_drive_p - provision drive object
 * @param packet_p - packet pointer.
 * 
 * @return fbe_status_t
 *
 * @author
 *   07/04/2012 - Arun Joseph - Created. 
 *
 ******************************************************************************/
fbe_lifecycle_status_t 
fbe_provision_drive_health_check_start_non_originator(fbe_provision_drive_t *  provision_drive_p, fbe_packet_t * packet_p)
{
    fbe_bool_t b_peer_flag_set = FBE_FALSE;
    fbe_bool_t b_local_flag_set = FBE_FALSE;

    fbe_provision_drive_lock(provision_drive_p);
    
    /* If the originator (our peer) is gone, broken or done, then mark ourselves as done. */
    if (!fbe_provision_drive_is_peer_ready_for_health_check(provision_drive_p) ||
        fbe_base_config_is_peer_clustered_flag_set((fbe_base_config_t*)provision_drive_p, FBE_BASE_CONFIG_CLUSTERED_FLAG_BROKEN) ||
		(!fbe_provision_drive_is_peer_clustered_flag_set(provision_drive_p, FBE_PROVISION_DRIVE_CLUSTERED_FLAG_HEALTH_CHECK_REQUEST) && 
		 !fbe_provision_drive_is_peer_clustered_flag_set(provision_drive_p, FBE_PROVISION_DRIVE_CLUSTERED_FLAG_HEALTH_CHECK_STARTED)))
    {
        fbe_provision_drive_set_local_state(provision_drive_p, FBE_PVD_LOCAL_STATE_HEALTH_CHECK_DONE);  /* this will clean up HC states. */
        fbe_provision_drive_clear_local_state(provision_drive_p, FBE_PVD_LOCAL_STATE_HEALTH_CHECK_PEER_START);
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "PVD_HC: start_non_originator. peer done or gone. local: 0x%llx peer: 0x%llx state: 0x%llx\n", 
                              (unsigned long long)provision_drive_p->provision_drive_metadata_memory.flags,
                              (unsigned long long)provision_drive_p->provision_drive_metadata_memory_peer.flags,
                              (unsigned long long)provision_drive_p->local_state);
        fbe_provision_drive_unlock(provision_drive_p);
        return FBE_LIFECYCLE_STATUS_RESCHEDULE;
    }

    if (fbe_provision_drive_is_peer_clustered_flag_set(provision_drive_p, FBE_PROVISION_DRIVE_CLUSTERED_FLAG_HEALTH_CHECK_FAILED))
    {
        fbe_provision_drive_set_local_state(provision_drive_p, FBE_PVD_LOCAL_STATE_HEALTH_CHECK_PEER_FAILED);
        fbe_provision_drive_clear_clustered_flag(provision_drive_p, FBE_PROVISION_DRIVE_CLUSTERED_FLAG_HEALTH_CHECK_STARTED);
        fbe_provision_drive_set_clustered_flag(provision_drive_p, FBE_PROVISION_DRIVE_CLUSTERED_FLAG_HEALTH_CHECK_FAILED);  /* this will clean up HC states.*/

        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "PVD_HC: start_non_originator. peer failed ready. local: 0x%llx peer: 0x%llx state: 0x%llx\n", 
                              (unsigned long long)provision_drive_p->provision_drive_metadata_memory.flags,
                              (unsigned long long)provision_drive_p->provision_drive_metadata_memory_peer.flags,
                              (unsigned long long)provision_drive_p->local_state);
        fbe_provision_drive_unlock(provision_drive_p);
        return FBE_LIFECYCLE_STATUS_RESCHEDULE;
    }

    /* The active side always starts the operation. 
     * Once we see the active side has started the operation we will send over our bitmask. 
     */
    b_peer_flag_set = fbe_provision_drive_is_peer_clustered_flag_set(provision_drive_p, 
                                                                     FBE_PROVISION_DRIVE_CLUSTERED_FLAG_HEALTH_CHECK_STARTED);

    b_local_flag_set = fbe_provision_drive_is_clustered_flag_set(provision_drive_p, 
                                                                 FBE_PROVISION_DRIVE_CLUSTERED_FLAG_HEALTH_CHECK_STARTED);

    /* We first wait for the peer to start the health_check. 
     */
    if (!b_peer_flag_set && !b_local_flag_set)
    {
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "PVD_HC: start_non_originator. waiting peer health_check started. local: 0x%llx peer: 0x%llx state: 0x%llx\n", 
                              (unsigned long long)provision_drive_p->provision_drive_metadata_memory.flags,
                              (unsigned long long)provision_drive_p->provision_drive_metadata_memory_peer.flags,
                              (unsigned long long)provision_drive_p->local_state);
        fbe_provision_drive_unlock(provision_drive_p);
        /* Reschedule for 0.5 second. */
        fbe_lifecycle_reschedule(&fbe_provision_drive_lifecycle_const, 
                                 (fbe_base_object_t*)provision_drive_p, (fbe_lifecycle_timer_msec_t)500);
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /* At this point the peer started flag is set.
     */
    if (!b_local_flag_set)
    {
        /* Peer is starting the health_check.  Keep PVD in ACTIVATE state.
         */
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "PVD_HC: start_non_originator. mark started state: 0x%llx\n", (unsigned long long)provision_drive_p->local_state);
        fbe_provision_drive_clear_clustered_flag(provision_drive_p, FBE_PROVISION_DRIVE_CLUSTERED_FLAG_HEALTH_CHECK_REQUEST);
        fbe_provision_drive_set_clustered_flag(provision_drive_p, FBE_PROVISION_DRIVE_CLUSTERED_FLAG_HEALTH_CHECK_STARTED);
        fbe_provision_drive_unlock(provision_drive_p);
        return FBE_LIFECYCLE_STATUS_RESCHEDULE;
    }


    fbe_provision_drive_unlock(provision_drive_p);

    return FBE_LIFECYCLE_STATUS_DONE;
}
/**************************************
 * end fbe_provision_drive_health_check_start_non_originator()
 **************************************/


/*!****************************************************************************
 * fbe_provision_drive_health_check_peer_failed()
 ******************************************************************************
 * @brief
 *  This function implements the health_check peer failed state for the non-originator side.
 *
 * @param provision_drive_p - provision drive object
 * @param packet_p - packet pointer.
 * 
 * @return fbe_status_t
 *
 * @author
 *   07/04/2012 - Arun Joseph - Created. 
 *
 ******************************************************************************/
fbe_lifecycle_status_t 
fbe_provision_drive_health_check_peer_failed(fbe_provision_drive_t *  provision_drive_p, fbe_packet_t * packet_p)
{
    fbe_bool_t          b_is_originator;

    b_is_originator = fbe_provision_drive_is_flag_set(provision_drive_p, FBE_PROVISION_DRIVE_FLAG_HC_ORIGINATOR);

    fbe_provision_drive_lock(provision_drive_p);

    /* We want to clear our local flags first.
     */
    if (fbe_provision_drive_is_clustered_flag_set(provision_drive_p, FBE_PROVISION_DRIVE_CLUSTERED_FLAG_HEALTH_CHECK_REQUEST) ||
        fbe_provision_drive_is_clustered_flag_set(provision_drive_p, FBE_PROVISION_DRIVE_CLUSTERED_FLAG_HEALTH_CHECK_STARTED))
    {
        /* Set health_checked failed so that we notify peer of failure ack.
         */
        fbe_provision_drive_set_clustered_flag(provision_drive_p, FBE_PROVISION_DRIVE_CLUSTERED_FLAG_HEALTH_CHECK_FAILED);

        /* At this point only failed should be set,  we're clearing everything else here.
         */
        fbe_provision_drive_clear_clustered_flag(provision_drive_p, 
                                             (FBE_PROVISION_DRIVE_CLUSTERED_FLAG_HEALTH_CHECK_REQUEST |
                                              FBE_PROVISION_DRIVE_CLUSTERED_FLAG_HEALTH_CHECK_STARTED));
        /* Set health_checked so that we acknowledge the peer is present.
         */
        fbe_provision_drive_unlock(provision_drive_p);
        return FBE_LIFECYCLE_STATUS_RESCHEDULE;
    }
    
    fbe_provision_drive_unlock(provision_drive_p);

    fbe_base_object_trace((fbe_base_object_t *)provision_drive_p, FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                            "%s PVD_HC: Failure detected. Stopping.\n", __FUNCTION__);

    return fbe_provision_drive_health_check_done(provision_drive_p, packet_p);    
}
/**************************************
 * end fbe_provision_drive_health_check_peer_failed()
 **************************************/

/*!****************************************************************************
 * fbe_provision_drive_send_health_check_command_to_pdo()
 ******************************************************************************
 * @brief
 *  This function sends health_check request to PDO.
 *
 * @param provision_drive_p - The provision drive.
 * @param packet_p - The packet requesting this operation.
 * @param request_status - status of HC request that was sent to us
 *
 * @return status - The status of the operation.
 *
 * @author
 *  07/04/2012 - Arun Joseph - Created. 
 *
 ******************************************************************************/
static fbe_lifecycle_status_t
fbe_provision_drive_send_health_check_command_to_pdo(fbe_provision_drive_t * provision_drive_p, 
                                                     fbe_packet_t * packet_p,
                                                     fbe_physical_drive_health_check_req_status_t request_status)
{
    fbe_payload_ex_t *                  payload_p = NULL;
    fbe_payload_control_operation_t *   control_operation_p = NULL;

    fbe_provision_drive_utils_trace(provision_drive_p, FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_INFO,
                                    FBE_PROVISION_DRIVE_DEBUG_FLAG_HEALTH_CHECK_TRACING,
                                    "%s PVD_HC entry\n", __FUNCTION__);

    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_allocate_control_operation(payload_p);

    fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_payload_control_set_status_qualifier(control_operation_p, 0);

    fbe_payload_control_build_operation (control_operation_p,
                                         FBE_PHYSICAL_DRIVE_CONTROL_CODE_HEALTH_CHECK_ACK,
                                         &request_status,
                                         sizeof(request_status));
        
    fbe_transport_set_packet_attr(packet_p, FBE_PACKET_FLAG_TRAVERSE); 

    if (FBE_DRIVE_HC_STATUS_PROCEED == request_status)
    {
        fbe_transport_set_completion_function(packet_p, fbe_provision_drive_send_health_check_pdo_proceed_completion, provision_drive_p);
    }
    else // FBE_DRIVE_HC_STATUS_ABORT
    {
        fbe_transport_set_completion_function(packet_p, fbe_provision_drive_send_health_check_pdo_abort_completion, provision_drive_p);
    }
    
    fbe_block_transport_send_control_packet(((fbe_base_config_t *)provision_drive_p)->block_edge_p, packet_p);
                      
    /* returning pending since monitor packet is being reused to send usurper cmd.   We need to 
       pend the monitor until this packet completes. */
    return FBE_LIFECYCLE_STATUS_PENDING;    
}
/******************************************************************************
 * end fbe_provision_drive_send_health_check_command_to_pdo()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_provision_drive_send_health_check_pdo_proceed_completion()
 ******************************************************************************
 * @brief
 *  This function is used as completion function for sending HC usurper commands
 *  to PDO.
 * 
 * @param packet_p   - Pointer to the packet.
 * @param context    - context, which is a pointer to the base object.
 *
 * @return fbe_status_t
 *
 * @author
 *  02/01/2013  Wayne Garrett - Created.
 * 
 ******************************************************************************/
static fbe_status_t 
fbe_provision_drive_send_health_check_pdo_proceed_completion(fbe_packet_t * packet_p,
                                                             fbe_packet_completion_context_t context)
{
    fbe_provision_drive_t *             provision_drive_p = (fbe_provision_drive_t*)context;
    fbe_payload_ex_t *                  payload_p = NULL;
    fbe_payload_control_operation_t *   control_operation_p = NULL;
    fbe_payload_control_status_t        control_status;
    fbe_status_t                        pkt_status = FBE_STATUS_GENERIC_FAILURE;

    fbe_provision_drive_utils_trace(provision_drive_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                    FBE_PROVISION_DRIVE_DEBUG_FLAG_HEALTH_CHECK_TRACING,
                                    "%s PVD_HC entry\n", __FUNCTION__);
    /* Get the packet status. */
    pkt_status = fbe_transport_get_status_code (packet_p);

    /* Get control status */
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);
    fbe_payload_control_get_status(control_operation_p, &control_status);

    if (FBE_STATUS_OK != pkt_status ||
        FBE_PAYLOAD_CONTROL_STATUS_OK != control_status)
    {
        /* PDO failed to complete the HC .*/
        fbe_provision_drive_utils_trace(provision_drive_p, FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                        FBE_PROVISION_DRIVE_DEBUG_FLAG_HEALTH_CHECK_TRACING,
                                        "%s PVD_HC failed. pkt:%d cntrl:%d\n", __FUNCTION__, pkt_status, control_status);
            
        /* Since there is no real action that PVD can take, we simply clean up (as if successful) and let PDO decide
           what action to take.*/
    }

    fbe_payload_ex_release_control_operation(payload_p, control_operation_p);

    /* wake monitor up to process next state.  Note, this might be overkill. Monitor may automatically wakeup up due to the packet completing.*/
    fbe_lifecycle_reschedule(&fbe_provision_drive_lifecycle_const, 
                             (fbe_base_object_t*)provision_drive_p, (fbe_lifecycle_timer_msec_t)0);
       
    return FBE_STATUS_OK;  
}

/*!****************************************************************************
 * fbe_provision_drive_send_health_check_pdo_abort_completion()
 ******************************************************************************
 * @brief
 *  This function is used as completion function for sending HC usurper commands
 *  to PDO.
 * 
 * @param packet_p   - Pointer to the packet.
 * @param context    - context, which is a pointer to the base object.
 *
 * @return fbe_status_t
 *
 * @author
 *  02/01/2013  Wayne Garrett - Created.
 * 
 ******************************************************************************/
static fbe_status_t 
fbe_provision_drive_send_health_check_pdo_abort_completion(fbe_packet_t * packet_p,
                                                           fbe_packet_completion_context_t context)
{

    fbe_provision_drive_t *             provision_drive_p = (fbe_provision_drive_t*)context;
    fbe_payload_ex_t *                  payload_p = NULL;
    fbe_payload_control_operation_t *   control_operation_p = NULL;
    fbe_payload_control_status_t        control_status;
    fbe_status_t                        pkt_status = FBE_STATUS_GENERIC_FAILURE;

    fbe_provision_drive_utils_trace(provision_drive_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                    FBE_PROVISION_DRIVE_DEBUG_FLAG_HEALTH_CHECK_TRACING,
                                    "%s PVD_HC entry\n", __FUNCTION__);

    /* Get the packet status. */
    pkt_status = fbe_transport_get_status_code (packet_p);

    /* Get control status */
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);
    fbe_payload_control_get_status(control_operation_p, &control_status);

    if (FBE_STATUS_OK != pkt_status ||
        FBE_PAYLOAD_CONTROL_STATUS_OK != control_status)
    {
        /* Something bad happened. Since we are already aborting, nothing to do .*/
        fbe_provision_drive_utils_trace(provision_drive_p, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                        FBE_PROVISION_DRIVE_DEBUG_FLAG_HEALTH_CHECK_TRACING,
                                        "%s PVD_HC failed. pkt:%d cntrl:%d\n", __FUNCTION__, pkt_status, control_status);

        /* Note, In the Abort Req path, no local state was set so nothing to clean up if we failed to send it.   
        */
    }

    fbe_payload_ex_release_control_operation(payload_p, control_operation_p);

    /* wake monitor up to process next state*/
    fbe_lifecycle_reschedule(&fbe_provision_drive_lifecycle_const, 
                             (fbe_base_object_t*)provision_drive_p, (fbe_lifecycle_timer_msec_t)0);

    return FBE_STATUS_OK;  
}



/*!****************************************************************************
 * fbe_provision_drive_is_peer_ready_for_health_check()
 ******************************************************************************
 * @brief
 *  This function checks whether the peer is ready for health_check process.
 *
 * @param provision_drive_p - The provision drive.
 * @param packet_p - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  07/04/2012 - Arun Joseph - Created. 
 *
 ******************************************************************************/
static fbe_bool_t 
fbe_provision_drive_is_peer_ready_for_health_check(fbe_provision_drive_t * provision_drive_p)
{
    fbe_lifecycle_state_t	                   peer_state = FBE_LIFECYCLE_STATE_INVALID;

    if (!fbe_cmi_is_peer_alive() ||
        !fbe_metadata_is_peer_object_alive(&((fbe_base_config_t *)provision_drive_p)->metadata_element))
    {
        /* If the cmi or metadata does not believe the peer is there, then there is no 
         * need to check peer lifecycle state. 
         */
        return FBE_FALSE;
    }

    /* Check the peer lifecycle state.  
     */
    fbe_base_config_metadata_get_peer_lifecycle_state((fbe_base_config_t *)provision_drive_p, &peer_state);
    if (peer_state != FBE_LIFECYCLE_STATE_PENDING_ACTIVATE &&
        peer_state != FBE_LIFECYCLE_STATE_ACTIVATE         &&
        peer_state != FBE_LIFECYCLE_STATE_PENDING_READY    &&
        peer_state != FBE_LIFECYCLE_STATE_READY         
        )
    {
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p, FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                              "PVD_HC: peer not ready. state=%d\n", peer_state);
        return FBE_FALSE;
    }

    return FBE_TRUE;
}
/******************************************
 * end fbe_provision_drive_is_peer_ready_for_health_check()
 ******************************************/

/*!****************************************************************************
 * fbe_provision_drive_is_peer_alive_for_health_check()
 ******************************************************************************
 * @brief
 *  This function checks whether the peer is alive, regardless of it's
 *  lifecycle state.
 *
 * @param provision_drive_p - The provision drive.
 * @param packet_p - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  09/18/2012  Wayne Garrett - Created. 
 *
 ******************************************************************************/
static fbe_bool_t 
fbe_provision_drive_is_peer_alive_for_health_check(fbe_provision_drive_t * provision_drive_p)
{
    if (!fbe_cmi_is_peer_alive() ||
        !fbe_metadata_is_peer_object_alive(&((fbe_base_config_t *)provision_drive_p)->metadata_element))
    {
        /* If the cmi or metadata does not believe the peer is there, then there is no 
         * need to check peer lifecycle state. 
         */
        return FBE_FALSE;
    }

    return FBE_TRUE;
}
/******************************************
 * end fbe_provision_drive_is_peer_alive_for_health_check()
 ******************************************/


/*!****************************************************************************
 * fbe_provision_drive_process_health_check_path_attributes()
 ******************************************************************************
 * @brief
 *  This function processes health_check path attribute changes.
 *
 * @param provision_drive_p - The provision drive.
 * @param path_attr - The path attribute.
 *
 * @return - whether we should lock the state/attr change.
 *
 * @author
 *  07/04/2012 - Arun Joseph - Created. 
 *
 ******************************************************************************/
fbe_bool_t 
fbe_provision_drive_process_health_check_path_attributes(fbe_provision_drive_t * provision_drive_p,
                                                     fbe_path_attr_t path_attr)
{
    fbe_provision_drive_local_state_t           local_state;
    fbe_path_attr_t                             health_check_path_attr;
    fbe_bool_t                                  done = FBE_FALSE;
    fbe_u32_t                                   number_of_clients = 0;
    fbe_provision_drive_clustered_flags_t       peer_clstr_flags = 0;

    fbe_base_object_trace((fbe_base_object_t *)provision_drive_p, FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_INFO,
                          "%s entry. PVD_HC\n", __FUNCTION__);

    health_check_path_attr = (path_attr & FBE_BLOCK_PATH_ATTR_FLAGS_HEALTH_CHECK_REQUEST);
    fbe_block_transport_server_get_number_of_clients(&((fbe_base_config_t *)provision_drive_p)->block_transport_server, 
                                                     &number_of_clients);

    fbe_provision_drive_lock(provision_drive_p);
    fbe_provision_drive_get_local_state(provision_drive_p, &local_state);

    if (health_check_path_attr == FBE_BLOCK_PATH_ATTR_FLAGS_HEALTH_CHECK_REQUEST)
    {
        /* verify that peer hasn't initiated its own HC.  If so then abort this request
           since it's being done on the peer side
        */
        (void)fbe_provision_drive_get_peer_clustered_flags(provision_drive_p, &peer_clstr_flags);

        if (!fbe_provision_drive_is_flag_set(provision_drive_p, FBE_PROVISION_DRIVE_FLAG_HC_ORIGINATOR) &&
            (peer_clstr_flags & FBE_PROVISION_DRIVE_CLUSTERED_FLAG_HEALTH_CHECK_MASK))
        {
            fbe_provision_drive_set_local_state(provision_drive_p, FBE_PVD_LOCAL_STATE_HEALTH_CHECK_ABORT_REQ);
            fbe_provision_drive_unlock(provision_drive_p);

            fbe_base_object_trace((fbe_base_object_t *)provision_drive_p, FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                   "PVD_HC: peer already initiated HC. Aborting. peer_clstr: 0x%llx attr: 0x%x\n",    
								   peer_clstr_flags, path_attr);                        
            return FBE_TRUE;
        }

        /* If we already have an HC local state set, then we got a second request from PDO or something else caused a state change.
           Ignore it since we are already processing the HC. */
        if (local_state & FBE_PVD_LOCAL_STATE_HEALTH_CHECK_MASK)
        {
            fbe_provision_drive_unlock(provision_drive_p);

            fbe_base_object_trace((fbe_base_object_t *)provision_drive_p, FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                   "PVD_HC: already requested. state: 0x%llx attr: 0x%x\n",    
								   local_state, path_attr);
            return FBE_TRUE;
        }        

        /* If we already have SLF state set, then we got a request from PDO.
         * Abort this reqeust since pdo can't handle HC in this case.
         */
        if (path_attr & FBE_BLOCK_PATH_ATTR_FLAGS_LINK_FAULT)
        {
            fbe_provision_drive_set_local_state(provision_drive_p, FBE_PVD_LOCAL_STATE_HEALTH_CHECK_ABORT_REQ);
            fbe_provision_drive_unlock(provision_drive_p);

            fbe_base_object_trace((fbe_base_object_t *)provision_drive_p, FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                   "PVD_HC: local in SLF. Aborting. peer_clstr: 0x%llx attr: 0x%x\n",    
								   peer_clstr_flags, path_attr);
            return FBE_TRUE;
        }
        
        fbe_provision_drive_set_local_state(provision_drive_p, FBE_PVD_LOCAL_STATE_HEALTH_CHECK_REQUEST);

        /* This is the Originator side of the health_check. It's expected that PDO has or will transition into Activate,
           which will cause PVD to process the HC state machine.
         */

        /* Mark as originator of HC request. */
        fbe_provision_drive_set_health_check_originator(provision_drive_p, FBE_TRUE);
        /* Update non-paged metadata to persist the need for Health Check.*/
        fbe_provision_drive_set_clustered_flag(provision_drive_p, FBE_PROVISION_DRIVE_CLUSTERED_FLAG_HEALTH_CHECK_REQUEST);
    }

    if (local_state & FBE_PVD_LOCAL_STATE_HEALTH_CHECK_MASK)
    {
        done = FBE_TRUE;
    }
    fbe_provision_drive_unlock(provision_drive_p);

    return done;
}
/******************************************
 * end fbe_provision_drive_process_health_check_path_attributes()
 ******************************************/

/*!**************************************************************
 * fbe_provision_drive_is_health_check_needed()
 ****************************************************************
 * @brief
 *  This function checks whether to start Health Check when edge state
 *  or attributes changes. 
 *
 * @param
 *   path_state  - path state
 *   path_attr   - path attributes
 *
 * @return fbe_bool_t
 * 
 * @author
 *  07/13/2015 - Created. Jibing Dong 
 *
 ****************************************************************/
fbe_bool_t fbe_provision_drive_is_health_check_needed(fbe_path_state_t path_state, fbe_path_attr_t path_attr)
{
    if ((path_state == FBE_PATH_STATE_DISABLED) && (path_attr & FBE_BLOCK_PATH_ATTR_FLAGS_HEALTH_CHECK_REQUEST))
    {
        return FBE_TRUE;
    }

    return FBE_FALSE;
}
/******************************************
 * end fbe_provision_drive_is_health_check_needed()
 ******************************************/

/******************************************
 * end file fbe_provision_drive_health_check.c
 ******************************************/
