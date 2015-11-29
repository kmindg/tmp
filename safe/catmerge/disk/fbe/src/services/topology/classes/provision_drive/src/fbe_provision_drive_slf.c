/***************************************************************************
 * Copyright (C) EMC Corporation 2012
 * All rights reserved. Licensed material -- property of EMC
 * Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_provision_drive_slf.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the provision drive object single loop failure  
 *  functionalities.
 * 
 * @ingroup provision_drive_class_files
 * 
 * @version
 *   04/30/2012:  Created. Lili Chen
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_transport.h"
#include "fbe_cmi.h"
#include "fbe/fbe_base_config.h"
#include "fbe_provision_drive_private.h"
#include "fbe_database.h"

#define SLF_MAX_BLOCK_DS_DISABLED_SECONDS 3
#define SLF_MAX_BLOCK_DS_BROKEN_SECONDS 2
#define SLF_MAX_BLOCK_DS_BROKEN_SECONDS_FOR_SPECIALIZE_STATE 6

/* Forward declarations */
fbe_lifecycle_status_t fbe_provision_drive_start_slf_action(fbe_provision_drive_t *  provision_drive_p, fbe_packet_t * packet_p);
fbe_bool_t fbe_provision_drive_slf_block_ds_health_disabled(fbe_provision_drive_t * provision_drive_p);
fbe_status_t fbe_provision_drive_set_clear_path_attr(fbe_provision_drive_t *  provision_drive_p);


/*!**************************************************************
 * fbe_provision_drive_is_slf_eval_needed()
 ****************************************************************
 * @brief
 *  This function checks whether to start EVAL SLF when edge state
 *  or attributes changes. 
 *
 * @param
 *   path_state  - path state
 *   path_attr   - path attributes
 *
 * @return fbe_bool_t
 * 
 * @author
 *  10/30/2012 - Created. Lili Chen
 *
 ****************************************************************/
static __forceinline fbe_bool_t fbe_provision_drive_is_slf_eval_needed(fbe_path_state_t path_state, fbe_path_attr_t path_attr)
{
    if ((path_state == FBE_PATH_STATE_INVALID) ||
        (path_state == FBE_PATH_STATE_GONE) ||
        ((path_state == FBE_PATH_STATE_DISABLED) && (path_attr & FBE_BLOCK_PATH_ATTR_FLAGS_LINK_FAULT)) ||
        ((path_state == FBE_PATH_STATE_BROKEN) && (path_attr & FBE_BLOCK_PATH_ATTR_FLAGS_LINK_FAULT)))
    {
        return FBE_TRUE;
    }

    return FBE_FALSE;
}
/****************************************************************
 * end fbe_provision_drive_is_slf_eval_needed()
 ****************************************************************/

/*!**************************************************************
 * fbe_provision_drive_slf_send_packet_to_peer()
 ****************************************************************
 * @brief
 *  This function is used by the provision drive to send a packet
 *  to the peer. 
 *
 * @param
 *   packet - pointer to a packet
 *
 * @return fbe_status_t - FBE_STATUS_OK
 * 
 * @author
 *  04/30/2012 - Created. Lili Chen
 *
 ****************************************************************/
fbe_status_t
fbe_provision_drive_slf_send_packet_to_peer(fbe_provision_drive_t * provision_drive_p, fbe_packet_t * packet_p)
{
    fbe_object_id_t my_object_id;

    fbe_provision_drive_utils_trace(provision_drive_p,
                                    FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_INFO, 
                                    FBE_PROVISION_DRIVE_DEBUG_FLAG_SLF_TRACING,
                                    "SLF: packet send to peer: %p \n", 
                                    packet_p);

    fbe_base_object_get_object_id((fbe_base_object_t *)provision_drive_p, &my_object_id);
    fbe_transport_set_address(packet_p,
                              FBE_PACKAGE_ID_SEP_0,
                              FBE_SERVICE_ID_TOPOLOGY,
                              FBE_CLASS_ID_INVALID,
                              my_object_id);
    fbe_transport_set_completion_function(packet_p, fbe_provision_drive_send_to_cmi_completion, provision_drive_p);
    fbe_cmi_send_packet(packet_p);

    return FBE_STATUS_OK;
}
/****************************************************************
 * end fbe_provision_drive_slf_send_packet_to_peer()
 ****************************************************************/


/*!**************************************************************
 * fbe_provision_drive_eval_slf_request()
 ****************************************************************
 * @brief
 *  This function implements the local eval slf request state.
 *  In this state both SPs coordinate setting the request flag.
 *  When both SPs have this request flag set, we transition to the
 *  started state.
 *
 * @param
 *   provision_drive_p - pointer to the provision drive
 *   packet_p          - pointer to a packet
 *
 * @return fbe_lifecycle_status_t
 * 
 * @author
 *  05/02/2012 - Created. Lili Chen
 *
 ****************************************************************/
fbe_lifecycle_status_t 
fbe_provision_drive_eval_slf_request(fbe_provision_drive_t * provision_drive_p, fbe_packet_t * packet_p)
{
    fbe_bool_t b_peer_flag_set = FBE_FALSE;
    fbe_bool_t b_is_active;

    b_is_active = fbe_base_config_is_active((fbe_base_config_t*)provision_drive_p);

    fbe_provision_drive_lock(provision_drive_p);

    /* If we don't already have the REQUEST local state set, we should set it now since
     * the first time through this will not be set. 
     */
    if (!fbe_provision_drive_is_local_state_set(provision_drive_p, FBE_PVD_LOCAL_STATE_EVAL_SLF_REQUEST))
    {
        fbe_provision_drive_set_local_state(provision_drive_p, FBE_PVD_LOCAL_STATE_EVAL_SLF_REQUEST);
        fbe_provision_drive_clear_clustered_flag(provision_drive_p, 
                                             FBE_PROVISION_DRIVE_CLUSTERED_FLAG_EVAL_SLF_MASK);
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "EVAL SLF %s request clear clustered flags local: 0x%llx peer: 0x%llx state: 0x%llx 0x%x\n", 
                              b_is_active ? "Active" : "Passive",
                              (unsigned long long)provision_drive_p->provision_drive_metadata_memory.flags,
                              (unsigned long long)provision_drive_p->provision_drive_metadata_memory_peer.flags,
                              (unsigned long long)provision_drive_p->local_state,
                              provision_drive_p->provision_drive_metadata_memory_peer.base_config_metadata_memory.lifecycle_state);
    }

    /* We start the process by setting request. 
     * This gets the peer running the correct condition. 
     */
    if (!fbe_provision_drive_is_clustered_flag_set(provision_drive_p, FBE_PROVISION_DRIVE_CLUSTERED_FLAG_EVAL_SLF_REQUEST))
    {
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "EVAL SLF %s marking request local state: 0x%llx\n",
                              b_is_active ? "Active" : "Passive",
                              (unsigned long long)provision_drive_p->local_state);
        fbe_provision_drive_set_clustered_flag(provision_drive_p, FBE_PROVISION_DRIVE_CLUSTERED_FLAG_EVAL_SLF_REQUEST);
        fbe_provision_drive_unlock(provision_drive_p);
        return FBE_LIFECYCLE_STATUS_RESCHEDULE;
    }

    /* Next we wait for the peer to tell us it received the request.
     */
    b_peer_flag_set = fbe_provision_drive_is_peer_flag_set(provision_drive_p, 
                                                           FBE_PROVISION_DRIVE_CLUSTERED_FLAG_EVAL_SLF_REQUEST);
    if (!b_peer_flag_set)
    {
        fbe_provision_drive_unlock(provision_drive_p);
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "EVAL SLF %s wait peer request local: 0x%llx peer: 0x%llx state: 0x%llx\n", 
                              b_is_active ? "Active" : "Passive",
                              (unsigned long long)provision_drive_p->provision_drive_metadata_memory.flags,
                              (unsigned long long)provision_drive_p->provision_drive_metadata_memory_peer.flags,
                              (unsigned long long)provision_drive_p->local_state);
        /* Abort peer locks in case peer monitor is stuck */
        fbe_metadata_stripe_lock_element_abort_monitor_ops(&((fbe_base_config_t *)provision_drive_p)->metadata_element, FBE_TRUE);
        return FBE_LIFECYCLE_STATUS_DONE;
    }
    else
    {
        /* The peer has set the flag, we are ready to do the next set of checks.
         * We need to wait for the peer to get to the request state as an acknowledgement. 
         */
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "EVAL SLF %s peer request set local: 0x%llx peer: 0x%llx state: 0x%llx 0x%x\n", 
                              b_is_active ? "Active" : "Passive",
                              (unsigned long long)provision_drive_p->provision_drive_metadata_memory.flags,
                              (unsigned long long)provision_drive_p->provision_drive_metadata_memory_peer.flags,
                              (unsigned long long)provision_drive_p->local_state,
                              provision_drive_p->provision_drive_metadata_memory_peer.base_config_metadata_memory.lifecycle_state);
        fbe_provision_drive_set_local_state(provision_drive_p, FBE_PVD_LOCAL_STATE_EVAL_SLF_STARTED);
        fbe_provision_drive_clear_local_state(provision_drive_p, FBE_PVD_LOCAL_STATE_EVAL_SLF_REQUEST);
        fbe_provision_drive_unlock(provision_drive_p);
        fbe_provision_drive_set_ds_disabled_start_time(provision_drive_p, fbe_get_time());
        return FBE_LIFECYCLE_STATUS_RESCHEDULE;
    }
}
/****************************************************************
 * end fbe_provision_drive_eval_slf_request()
 ****************************************************************/

/*!**************************************************************
 * fbe_provision_drive_eval_slf_started_active()
 ****************************************************************
 * @brief
 *  This function implements the local eval_slf started state
 *  for active side. The active side will set "started" flag first,
 *  and wait for the peer to set started flag. After both sides
 *  set started flag, it transits to done local state.
 *
 * @param
 *   provision_drive_p - pointer to the provision drive
 *   packet_p          - pointer to a packet
 *
 * @return fbe_lifecycle_status_t
 * 
 * @author
 *  05/02/2012 - Created. Lili Chen
 *
 ****************************************************************/
fbe_lifecycle_status_t 
fbe_provision_drive_eval_slf_started_active(fbe_provision_drive_t *  provision_drive_p, fbe_packet_t * packet_p)
{
    fbe_bool_t b_peer_flag_set = FBE_FALSE;
    fbe_bool_t b_is_active;
    fbe_bool_t b_io_already_quiesced = FBE_FALSE;

    b_is_active = fbe_base_config_is_active((fbe_base_config_t*)provision_drive_p);

    /* Quiesce the IO before proceeding */
    fbe_provision_drive_utils_quiesce_io_if_needed(provision_drive_p, &b_io_already_quiesced);
    if (b_io_already_quiesced == FBE_FALSE)
    {
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    fbe_provision_drive_lock(provision_drive_p);
    fbe_provision_drive_set_slf_flag_based_on_downstream_edge(provision_drive_p);
    if (!fbe_provision_drive_is_clustered_flag_set(provision_drive_p, FBE_PROVISION_DRIVE_CLUSTERED_FLAG_EVAL_SLF_STARTED))
    {

        fbe_provision_drive_set_clustered_flag(provision_drive_p, FBE_PROVISION_DRIVE_CLUSTERED_FLAG_EVAL_SLF_STARTED);
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "EVAL SLF %s mark started local: 0x%llx peer: 0x%x state: 0x%llx\n",
                              b_is_active ? "Active" : "Passive",
                              (unsigned long long)provision_drive_p->provision_drive_metadata_memory.flags,
                              (unsigned int)provision_drive_p->provision_drive_metadata_memory_peer.flags,
                              (unsigned long long)provision_drive_p->local_state);
        fbe_provision_drive_unlock(provision_drive_p);
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    if (fbe_provision_drive_slf_block_ds_health_disabled(provision_drive_p))
    {
        fbe_provision_drive_unlock(provision_drive_p);
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s wait ds health enabled or broken\n", __FUNCTION__);
        /* We wait for 0.2 seconds for edge change information. */
        fbe_lifecycle_reschedule(&fbe_provision_drive_lifecycle_const, 
                                 (fbe_base_object_t*)provision_drive_p, (fbe_lifecycle_timer_msec_t)200); 
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    b_peer_flag_set = fbe_provision_drive_is_peer_flag_set(provision_drive_p,
                                                           FBE_PROVISION_DRIVE_CLUSTERED_FLAG_EVAL_SLF_STARTED);
    if (!b_peer_flag_set)
    {
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                  "EVAL SLF %s wait peer started local: 0x%llx peer: 0x%llx\n", 
                                  b_is_active ? "Active" : "Passive",
                                  (unsigned long long)provision_drive_p->provision_drive_metadata_memory.flags,
                                  (unsigned long long)provision_drive_p->provision_drive_metadata_memory_peer.flags);
        fbe_provision_drive_unlock(provision_drive_p);
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    fbe_provision_drive_set_clear_path_attr(provision_drive_p);
    fbe_provision_drive_set_local_state(provision_drive_p, FBE_PVD_LOCAL_STATE_EVAL_SLF_DONE);
    fbe_provision_drive_clear_local_state(provision_drive_p, FBE_PVD_LOCAL_STATE_EVAL_SLF_STARTED);
    fbe_provision_drive_unlock(provision_drive_p);

    return FBE_LIFECYCLE_STATUS_RESCHEDULE;
}
/****************************************************************
 * end fbe_provision_drive_eval_slf_started_active()
 ****************************************************************/

/*!**************************************************************
 * fbe_provision_drive_eval_slf_started_passive()
 ****************************************************************
 * @brief
 *  This function implements the local eval_slf started state
 *  for passive side. The passive side will wait the peer to set,
 *  started flag, and then sets its started flag. It will transits
 *  to done state after the peer clears started flag.
 *
 * @param
 *   provision_drive_p - pointer to the provision drive
 *   packet_p          - pointer to a packet
 *
 * @return fbe_lifecycle_status_t
 * 
 * @author
 *  05/02/2012 - Created. Lili Chen
 *
 ****************************************************************/
fbe_lifecycle_status_t 
fbe_provision_drive_eval_slf_started_passive(fbe_provision_drive_t *  provision_drive_p, fbe_packet_t * packet_p)
{
    fbe_bool_t b_peer_flag_set = FBE_FALSE;
    fbe_bool_t b_local_flag_set = FBE_FALSE;
    fbe_bool_t b_is_active;

    b_is_active = fbe_base_config_is_active((fbe_base_config_t*)provision_drive_p);

    fbe_provision_drive_lock(provision_drive_p);

    b_peer_flag_set = fbe_provision_drive_is_peer_flag_set(provision_drive_p, 
                                                           FBE_PROVISION_DRIVE_CLUSTERED_FLAG_EVAL_SLF_STARTED);
    b_local_flag_set = fbe_provision_drive_is_clustered_flag_set(provision_drive_p, 
                                                                 FBE_PROVISION_DRIVE_CLUSTERED_FLAG_EVAL_SLF_STARTED);
    /* We first wait for the peer to start. 
     */
    if (!b_peer_flag_set && !b_local_flag_set)
    {
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "EVAL SLF %s waiting peer started local: 0x%llx peer: 0x%llx state: 0x%llx\n",
                              "Passive", 
                              (unsigned long long)provision_drive_p->provision_drive_metadata_memory.flags,
                              (unsigned long long)provision_drive_p->provision_drive_metadata_memory_peer.flags,
                              (unsigned long long)provision_drive_p->local_state);
        fbe_provision_drive_unlock(provision_drive_p);
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    fbe_provision_drive_set_slf_flag_based_on_downstream_edge(provision_drive_p);

    if (fbe_provision_drive_slf_block_ds_health_disabled(provision_drive_p))
    {
        fbe_provision_drive_unlock(provision_drive_p);
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s wait ds health enabled or broken\n", __FUNCTION__);
        /* We wait for 0.2 seconds for edge change information. */
        fbe_lifecycle_reschedule(&fbe_provision_drive_lifecycle_const, 
                                 (fbe_base_object_t*)provision_drive_p, (fbe_lifecycle_timer_msec_t)200); 
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /* At this point the peer started flag is set.
     */
    if (!b_local_flag_set)
    {
        fbe_provision_drive_set_clustered_flag(provision_drive_p, FBE_PROVISION_DRIVE_CLUSTERED_FLAG_EVAL_SLF_STARTED);
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "EVAL SLF %s mark started local: 0x%llx peer: 0x%x state: 0x%llx\n",
                              "Passive",
                              (unsigned long long)provision_drive_p->provision_drive_metadata_memory.flags,
                              (unsigned int)provision_drive_p->provision_drive_metadata_memory_peer.flags,
                              (unsigned long long)provision_drive_p->local_state);
        fbe_provision_drive_unlock(provision_drive_p);
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    if (!b_peer_flag_set || 
        !fbe_base_config_has_peer_joined((fbe_base_config_t *)provision_drive_p))
    {
        fbe_provision_drive_set_clear_path_attr(provision_drive_p);
        fbe_provision_drive_set_local_state(provision_drive_p, FBE_PVD_LOCAL_STATE_EVAL_SLF_DONE);
        fbe_provision_drive_clear_local_state(provision_drive_p, FBE_PVD_LOCAL_STATE_EVAL_SLF_STARTED);
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "EVAL SLF Passive start cleanup local: 0x%llx peer: 0x%llx lstate: 0x%llx\n",
                              (unsigned long long)provision_drive_p->provision_drive_metadata_memory.flags,
                              (unsigned long long)provision_drive_p->provision_drive_metadata_memory_peer.flags,
                              (unsigned long long)provision_drive_p->local_state);
        fbe_provision_drive_unlock(provision_drive_p);

        return FBE_LIFECYCLE_STATUS_RESCHEDULE;
    }
    else
    {
        fbe_provision_drive_unlock(provision_drive_p);
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "EVAL SLF Passive wait peer complete local: 0x%llx peer: 0x%llx lstate: 0x%llx\n",
                              (unsigned long long)provision_drive_p->provision_drive_metadata_memory.flags,
                              (unsigned long long)provision_drive_p->provision_drive_metadata_memory_peer.flags,
                              (unsigned long long)provision_drive_p->local_state);
        return FBE_LIFECYCLE_STATUS_DONE;
    }
}
/****************************************************************
 * end fbe_provision_drive_eval_slf_started_passive()
 ****************************************************************/

/*!**************************************************************
 * fbe_provision_drive_eval_slf_done()
 ****************************************************************
 * @brief
 *  This function implements local eval_slf done state. 
 *
 * @param
 *   provision_drive_p - pointer to the provision drive
 *   packet_p          - pointer to a packet
 *
 * @return fbe_lifecycle_status_t
 * 
 * @author
 *  05/02/2012 - Created. Lili Chen
 *
 ****************************************************************/
fbe_lifecycle_status_t 
fbe_provision_drive_eval_slf_done(fbe_provision_drive_t *  provision_drive_p, fbe_packet_t * packet_p)
{
    fbe_status_t status;
    fbe_bool_t b_is_active;
    fbe_bool_t need_to_clear_cond = FBE_TRUE;
    fbe_lifecycle_status_t lifecycle_status;

    b_is_active = fbe_base_config_is_active((fbe_base_config_t*)provision_drive_p);

    /* Clear local state.
     */
    fbe_provision_drive_lock(provision_drive_p);
    if (fbe_provision_drive_is_clustered_flag_set(provision_drive_p, FBE_PROVISION_DRIVE_CLUSTERED_FLAG_EVAL_SLF_STARTED))
    {
        fbe_provision_drive_clear_clustered_flag(provision_drive_p, 
                                                 FBE_PROVISION_DRIVE_CLUSTERED_FLAG_EVAL_SLF_MASK);
        fbe_provision_drive_unlock(provision_drive_p);
        return FBE_LIFECYCLE_STATUS_RESCHEDULE;
    }

    if (fbe_provision_drive_is_peer_clustered_flag_set(provision_drive_p, FBE_PROVISION_DRIVE_CLUSTERED_FLAG_EVAL_SLF_STARTED))
    {
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "EVAL SLF %s wait peer done local: 0x%llx peer: 0x%llx state:0x%llx\n", 
                              b_is_active ? "Active" : "Passive",
                              (unsigned long long)provision_drive_p->provision_drive_metadata_memory.flags,
                              (unsigned long long)provision_drive_p->provision_drive_metadata_memory_peer.flags,
                              (unsigned long long)provision_drive_p->local_state);
        fbe_provision_drive_unlock(provision_drive_p);
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    fbe_provision_drive_clear_local_state(provision_drive_p, 
                                          FBE_PVD_LOCAL_STATE_EVAL_SLF_MASK);
    fbe_provision_drive_unlock(provision_drive_p);

    fbe_base_object_trace((fbe_base_object_t *)provision_drive_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
						  "EVAL SLF %s cleanup done local: 0x%llx peer: 0x%llx state:0x%llx\n", 
						  b_is_active ? "Active" : "Passive",
						  (unsigned long long)provision_drive_p->provision_drive_metadata_memory.flags,
						  (unsigned long long)provision_drive_p->provision_drive_metadata_memory_peer.flags,
						  (unsigned long long)provision_drive_p->local_state);

    if (fbe_provision_drive_is_peer_clustered_flag_set(provision_drive_p, FBE_PROVISION_DRIVE_CLUSTERED_FLAG_EVAL_SLF_REQUEST) &&
        !fbe_provision_drive_is_clustered_flag_set(provision_drive_p, FBE_PROVISION_DRIVE_CLUSTERED_FLAG_EVAL_SLF_REQUEST))
    {
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
						      "EVAL SLF %s peer request set again: 0x%llx peer: 0x%llx\n", 
						      b_is_active ? "Active" : "Passive",
						      (unsigned long long)provision_drive_p->provision_drive_metadata_memory.flags,
						      (unsigned long long)provision_drive_p->provision_drive_metadata_memory_peer.flags);
        need_to_clear_cond = FBE_FALSE;
    }
    /* Let's clear the condition.
     */
    if (need_to_clear_cond)
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
    }
    fbe_provision_drive_set_ds_disabled_start_time(provision_drive_p, 0);

    /* Start action */
    lifecycle_status = fbe_provision_drive_start_slf_action(provision_drive_p, packet_p);

    return lifecycle_status;
}
/****************************************************************
 * end fbe_provision_drive_eval_slf_done()
 ****************************************************************/

#if 0
/*!**************************************************************
 * fbe_provision_drive_set_slf_flag_based_on_downstream_health()
 ****************************************************************
 * @brief
 *  This function decides whether to set/clear SLF clustered flag
 *  and state based on downstream health. 
 *
 * @param
 *   provision_drive_p - pointer to the provision drive
 *
 * @return fbe_lifecycle_status_t
 * 
 * @author
 *  05/02/2012 - Created. Lili Chen
 *
 ****************************************************************/
fbe_status_t fbe_provision_drive_set_slf_flag_based_on_downstream_health(fbe_provision_drive_t *  provision_drive_p)
{
    fbe_base_config_downstream_health_state_t   downstream_health_state;

    downstream_health_state = fbe_provision_drive_verify_downstream_health(provision_drive_p);
    if (downstream_health_state == FBE_DOWNSTREAM_HEALTH_BROKEN ||
        downstream_health_state == FBE_DOWNSTREAM_HEALTH_DISABLED)
    {
        fbe_provision_drive_set_clustered_flag(provision_drive_p, FBE_PROVISION_DRIVE_CLUSTERED_FLAG_SLF);
        fbe_provision_drive_set_local_state(provision_drive_p, FBE_PVD_LOCAL_STATE_SLF);
    }
    else if (downstream_health_state == FBE_DOWNSTREAM_HEALTH_OPTIMAL)
    {
        fbe_provision_drive_clear_clustered_flag(provision_drive_p, FBE_PROVISION_DRIVE_CLUSTERED_FLAG_SLF);
        fbe_provision_drive_clear_local_state(provision_drive_p, FBE_PVD_LOCAL_STATE_SLF);
    }

    return FBE_STATUS_OK;
}
/****************************************************************
 * end fbe_provision_drive_set_slf_flag_based_on_downstream_health()
 ****************************************************************/
#endif

/*!**************************************************************
 * fbe_provision_drive_set_slf_flag_based_on_downstream_edge()
 ****************************************************************
 * @brief
 *  This function decides whether to set/clear SLF clustered flag
 *  based on downstream edge. 
 *
 * @param
 *   provision_drive_p - pointer to the provision drive
 *
 * @return fbe_status_t
 * 
 * @author
 *  10/29/2012 - Created. Lili Chen
 *
 ****************************************************************/
fbe_status_t fbe_provision_drive_set_slf_flag_based_on_downstream_edge(fbe_provision_drive_t *  provision_drive_p)
{
    fbe_block_edge_t *      edge_p = NULL;
    fbe_path_state_t        path_state;
    fbe_path_attr_t         path_attr;

    /* Check the path state of the edge */
    fbe_base_config_get_block_edge( (fbe_base_config_t *) provision_drive_p, &edge_p, 0);
    fbe_block_transport_get_path_state(edge_p, &path_state);
    fbe_block_transport_get_path_attributes(edge_p, &path_attr);

    if (fbe_provision_drive_is_slf_eval_needed(path_state, path_attr))
    {
        fbe_provision_drive_set_clustered_flag(provision_drive_p, FBE_PROVISION_DRIVE_CLUSTERED_FLAG_SLF);
    }
    else
    {
        fbe_provision_drive_clear_clustered_flag(provision_drive_p, FBE_PROVISION_DRIVE_CLUSTERED_FLAG_SLF);
    }

    return FBE_STATUS_OK;
}
/****************************************************************
 * end fbe_provision_drive_set_slf_flag_based_on_downstream_edge()
 ****************************************************************/

/*!**************************************************************
 * fbe_provision_drive_set_clear_path_attr()
 ****************************************************************
 * @brief
 *  This function sets/clears NOT_PREFERRED path attribute. 
 *  This function sets/clears SLF local state after evaluation is done. 
 *
 * @param
 *   provision_drive_p - pointer to the provision drive
 *
 * @return fbe_status_t
 * 
 * @author
 *  08/28/2012 - Created. Lili Chen
 *
 ****************************************************************/
fbe_status_t fbe_provision_drive_set_clear_path_attr(fbe_provision_drive_t *  provision_drive_p)
{
    fbe_bool_t b_peer_flag_set = FBE_FALSE;
    fbe_bool_t b_local_flag_set = FBE_FALSE;

    /* Make decisions based on SLF flag on both SPs */
    b_local_flag_set = fbe_provision_drive_is_clustered_flag_set(provision_drive_p, FBE_PROVISION_DRIVE_CLUSTERED_FLAG_SLF);
    if (!fbe_base_config_is_peer_present((fbe_base_config_t *)provision_drive_p) ||
        !fbe_base_config_is_any_clustered_flag_set((fbe_base_config_t*)provision_drive_p,
                                                   FBE_BASE_CONFIG_CLUSTERED_FLAG_JOIN_STARTED |
                                                   FBE_BASE_CONFIG_CLUSTERED_FLAG_JOIN_SYNCING |
                                                   FBE_BASE_CONFIG_CLUSTERED_FLAG_JOINED))
    {
        b_peer_flag_set = FBE_TRUE;
    }
    else
    {
        b_peer_flag_set = fbe_provision_drive_is_peer_clustered_flag_set(provision_drive_p, FBE_PROVISION_DRIVE_CLUSTERED_FLAG_SLF);
    }

    if (b_local_flag_set && !b_peer_flag_set)
    {
        /* Set block attribute */
        fbe_block_transport_server_propagate_path_attr_with_mask_all_servers(
                &((fbe_base_config_t *)provision_drive_p)->block_transport_server, FBE_BLOCK_PATH_ATTR_FLAGS_PATH_NOT_PREFERRED,
                FBE_BLOCK_PATH_ATTR_FLAGS_PATH_NOT_PREFERRED);

        /* Set SLF local state */
        fbe_provision_drive_set_local_state(provision_drive_p, FBE_PVD_LOCAL_STATE_SLF);
    }
    else
    {
        /* Clear block attribute */
        fbe_block_transport_server_propagate_path_attr_with_mask_all_servers(
                &((fbe_base_config_t *)provision_drive_p)->block_transport_server, 0,
                FBE_BLOCK_PATH_ATTR_FLAGS_PATH_NOT_PREFERRED);

        /* Clear SLF local state */
        fbe_provision_drive_clear_local_state(provision_drive_p, FBE_PVD_LOCAL_STATE_SLF);
    }

    return FBE_STATUS_OK;
}
/****************************************************************
 * end fbe_provision_drive_set_clear_path_attr()
 ****************************************************************/

/*!**************************************************************
 * fbe_provision_drive_start_slf_action()
 ****************************************************************
 * @brief
 *  This function decides slf action by both SPs after eval_slf.
 *
 * @param
 *   provision_drive_p - pointer to the provision drive
 *
 * @return fbe_lifecycle_status_t
 * 
 * @author
 *  05/02/2012 - Created. Lili Chen
 *
 ****************************************************************/
fbe_lifecycle_status_t fbe_provision_drive_start_slf_action(fbe_provision_drive_t *  provision_drive_p,
												  fbe_packet_t * packet_p)
{
    fbe_bool_t b_peer_flag_set = FBE_FALSE;
    fbe_bool_t b_local_flag_set = FBE_FALSE;
    fbe_status_t status;
    fbe_bool_t b_is_active;
    fbe_block_edge_t *      edge_p = NULL;
    fbe_path_state_t        path_state;
    fbe_path_attr_t         path_attr;
    fbe_lifecycle_status_t lifecycle_state;

    b_is_active = fbe_base_config_is_active((fbe_base_config_t*)provision_drive_p);

    /* Make decisions based on SLF flag on both SPs */
    b_local_flag_set = fbe_provision_drive_is_clustered_flag_set(provision_drive_p, FBE_PROVISION_DRIVE_CLUSTERED_FLAG_SLF);
    if (!fbe_base_config_is_peer_present((fbe_base_config_t *)provision_drive_p) ||
        !fbe_base_config_is_any_clustered_flag_set((fbe_base_config_t*)provision_drive_p,
                                                   FBE_BASE_CONFIG_CLUSTERED_FLAG_JOIN_STARTED |
                                                   FBE_BASE_CONFIG_CLUSTERED_FLAG_JOIN_SYNCING |
                                                   FBE_BASE_CONFIG_CLUSTERED_FLAG_JOINED))
    {
        b_peer_flag_set = FBE_TRUE;
    }
    else
    {
        b_peer_flag_set = fbe_provision_drive_is_peer_clustered_flag_set(provision_drive_p, FBE_PROVISION_DRIVE_CLUSTERED_FLAG_SLF);
    }

    if (b_local_flag_set && b_peer_flag_set)
    {
        /* We go to broken */
        status = fbe_lifecycle_set_cond(&fbe_provision_drive_lifecycle_const,
                                        (fbe_base_object_t*)provision_drive_p,
                                        FBE_BASE_CONFIG_LIFECYCLE_COND_DOWNSTREAM_HEALTH_BROKEN);

        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /* Detach block edge here */
    fbe_base_config_get_block_edge( (fbe_base_config_t *) provision_drive_p, &edge_p, 0);
    fbe_block_transport_get_path_state(edge_p, &path_state);
    fbe_block_transport_get_path_attributes(edge_p, &path_attr);

    if (path_state == FBE_PATH_STATE_GONE)
    {
        fbe_base_config_detach_edge((fbe_base_config_t *) provision_drive_p, packet_p, edge_p);
        fbe_provision_drive_set_drive_location(provision_drive_p, FBE_PORT_NUMBER_INVALID, FBE_ENCLOSURE_NUMBER_INVALID, FBE_SLOT_NUMBER_INVALID);
    }
    else if(!fbe_provision_drive_is_local_state_set(provision_drive_p, FBE_PVD_LOCAL_STATE_SLF))
    {
        fbe_bool_t is_bes_valid;

        is_bes_valid = fbe_provision_drive_is_location_valid(provision_drive_p);

        if (!is_bes_valid) {
            status = fbe_lifecycle_set_cond(&fbe_provision_drive_lifecycle_const,
                                            (fbe_base_object_t*)provision_drive_p,
                                            FBE_PROVISION_DRIVE_LIFECYCLE_COND_UPDATE_PHYSICAL_DRIVE_INFO);
        }
    }

    /* Set the flag to check whether we need send passive request in monitor condition. */
    fbe_provision_drive_set_flag(provision_drive_p, FBE_PROVISION_DRIVE_FLAG_CHECK_PASSIVE_REQUEST);

    if((path_state == FBE_PATH_STATE_ENABLED) ||
       (path_state == FBE_PATH_STATE_SLUMBER))
    {
        /* The path is now enabled. Check if we need to push the keys down*/
       lifecycle_state =  fbe_provision_drive_check_key_info(provision_drive_p, packet_p);
       if(lifecycle_state == FBE_LIFECYCLE_STATUS_PENDING)
       {
           return FBE_LIFECYCLE_STATUS_PENDING;
       }
    }

    /* Abort the health check if local in SLF */
    if (b_local_flag_set &&
        fbe_provision_drive_is_health_check_needed(path_state, path_attr))
    {
        return fbe_provision_drive_health_check_abort_req_originator(provision_drive_p, packet_p);
    }
    
    return FBE_LIFECYCLE_STATUS_DONE;
}
/****************************************************************
 * end fbe_provision_drive_start_slf_action()
 ****************************************************************/

/*!**************************************************************
 * fbe_provision_drive_initiate_eval_slf()
 ****************************************************************
 * @brief
 *  This function is used to set eval_slf condition in certain 
 *  conditions when the PVD gets state change events. 
 *
 * @param
 *   provision_drive_p - pointer to the provision drive
 *   path_attr  - path attributes
 *   downstream_health_state  - downstream health
 *
 * @return fbe_bool_t - TRUE if the caller should contiue executing
 * 
 * @author
 *  05/02/2012 - Created. Lili Chen
 *
 ****************************************************************/
fbe_bool_t fbe_provision_drive_initiate_eval_slf(fbe_provision_drive_t *  provision_drive_p,
                                                 fbe_path_attr_t path_state,
                                                 fbe_path_attr_t path_attr)
{
    fbe_bool_t b_continue = FBE_TRUE;
    fbe_status_t status;
    fbe_provision_drive_clustered_flags_t flags;

    if (!fbe_provision_drive_is_slf_enabled())
    {
        return b_continue;
    }

    /* Drive fault has higher priority */
    if (path_attr & FBE_BLOCK_PATH_ATTR_FLAGS_DRIVE_FAULT)
    {
        return b_continue;
    }

    /* Set/clear SLF clustered flag */
    fbe_provision_drive_set_slf_flag_based_on_downstream_edge(provision_drive_p);

    /* If the peer is gone, we should start EVAL SLF only if our state is set */
    if (!fbe_base_config_is_peer_present((fbe_base_config_t *)provision_drive_p) &&
        !fbe_provision_drive_is_local_state_set(provision_drive_p, FBE_PVD_LOCAL_STATE_SLF))
    {
        return b_continue;
    }

    switch (path_state)
    {
        case FBE_PATH_STATE_ENABLED:
            if (fbe_provision_drive_is_local_state_set(provision_drive_p, FBE_PVD_LOCAL_STATE_SLF))
            {
                fbe_base_object_trace((fbe_base_object_t *)provision_drive_p, 
                                      FBE_TRACE_LEVEL_INFO,
                                      FBE_TRACE_MESSAGE_ID_INFO,
                                      "%s drive came back start slf\n",
                                      __FUNCTION__);
                status = fbe_lifecycle_set_cond(&fbe_provision_drive_lifecycle_const,
                                                (fbe_base_object_t*)provision_drive_p,
                                                FBE_PROVISION_DRIVE_LIFECYCLE_COND_EVAL_SLF);
            }
            break;
        case FBE_PATH_STATE_DISABLED:
        case FBE_PATH_STATE_BROKEN:
        case FBE_PATH_STATE_GONE:
        case FBE_PATH_STATE_INVALID:
            /* If peer is not joined/joining, no need for EVAL SLF. 
             */
            if (!fbe_base_config_is_peer_present((fbe_base_config_t *)provision_drive_p) ||
                !fbe_base_config_is_any_clustered_flag_set((fbe_base_config_t*)provision_drive_p,
                                                           FBE_BASE_CONFIG_CLUSTERED_FLAG_JOIN_MASK))
            {
                return FBE_TRUE;
            }
            /* If peer is already in SLF, we should go failed directly, no need for EVAL SLF. 
             */
            fbe_provision_drive_get_clustered_flags(provision_drive_p, &flags);
            if (!fbe_provision_drive_is_local_state_set(provision_drive_p, FBE_PVD_LOCAL_STATE_SLF) &&
                !(flags & FBE_PROVISION_DRIVE_CLUSTERED_FLAG_EVAL_SLF_MASK) &&
                fbe_provision_drive_is_peer_clustered_flag_set(provision_drive_p, FBE_PROVISION_DRIVE_CLUSTERED_FLAG_SLF))
            {
                fbe_base_object_trace((fbe_base_object_t *)provision_drive_p, 
                                      FBE_TRACE_LEVEL_INFO,
                                      FBE_TRACE_MESSAGE_ID_INFO,
                                      "%s go to fail, path_state %d path_attr 0x%x\n",
                                      __FUNCTION__, path_state, path_attr);
                //fbe_provision_drive_set_local_state(provision_drive_p, FBE_PVD_LOCAL_STATE_SLF);
                //fbe_provision_drive_set_clustered_flag(provision_drive_p, FBE_PROVISION_DRIVE_CLUSTERED_FLAG_SLF) 
                if (path_state == FBE_PATH_STATE_DISABLED)
                {
                    status = fbe_lifecycle_set_cond(&fbe_provision_drive_lifecycle_const,
                                                    (fbe_base_object_t*)provision_drive_p,
                                                    FBE_BASE_CONFIG_LIFECYCLE_COND_DOWNSTREAM_HEALTH_DISABLED);
                }
                else
                {
                    status = fbe_lifecycle_set_cond(&fbe_provision_drive_lifecycle_const,
                                                    (fbe_base_object_t*)provision_drive_p,
                                                    FBE_BASE_CONFIG_LIFECYCLE_COND_DOWNSTREAM_HEALTH_BROKEN);
                }
                return FBE_TRUE;
            }
            /* Start SLF if LINK_FAULT is set, or the edge is gone.
             */
            else if (!fbe_provision_drive_is_local_state_set(provision_drive_p, FBE_PVD_LOCAL_STATE_SLF) &&
                     fbe_provision_drive_is_slf_eval_needed(path_state, path_attr))
            {
                fbe_base_object_trace((fbe_base_object_t *)provision_drive_p, 
                                      FBE_TRACE_LEVEL_INFO,
                                      FBE_TRACE_MESSAGE_ID_INFO,
                                      "%s start slf path_state %d path_attr 0x%x\n",
                                      __FUNCTION__, path_state, path_attr);
                status = fbe_lifecycle_set_cond(&fbe_provision_drive_lifecycle_const,
                                                (fbe_base_object_t*)provision_drive_p,
                                                FBE_PROVISION_DRIVE_LIFECYCLE_COND_EVAL_SLF);
            }
            /* Block the link changes */
            if (fbe_provision_drive_is_slf_eval_needed(path_state, path_attr) ||
                fbe_provision_drive_is_local_state_set(provision_drive_p, FBE_PVD_LOCAL_STATE_SLF))
            {
                b_continue = FBE_FALSE;
            }
            break;
        default:
            break;
    }

    return b_continue;
}
/****************************************************************
 * end fbe_provision_drive_initiate_eval_slf()
 ****************************************************************/

/*!**************************************************************
 * fbe_provision_drive_slf_block_ds_health_disabled()
 ****************************************************************
 * @brief
 *  This function checks whether to block downstream health disabled 
 *  state during EVAL SLF. 
 *
 * @param
 *   provision_drive_p - pointer to the provision drive
 *
 * @return fbe_bool_t
 * 
 * @author
 *  05/02/2012 - Created. Lili Chen
 *
 ****************************************************************/
fbe_bool_t fbe_provision_drive_slf_block_ds_health_disabled(fbe_provision_drive_t * provision_drive_p)
{
    fbe_time_t start_time;
    fbe_bool_t b_peer_flag_set = FBE_FALSE;
    fbe_bool_t b_local_flag_set = FBE_FALSE;
    fbe_base_config_downstream_health_state_t   downstream_health_state;
    fbe_bool_t need_to_block = FBE_FALSE;

    /* If the edge is disabled, we need to wait it for enabled/broken */
    downstream_health_state = fbe_provision_drive_verify_downstream_health(provision_drive_p);
    if (downstream_health_state == FBE_DOWNSTREAM_HEALTH_DISABLED)
    {
        need_to_block = FBE_TRUE;
    }

    /* If both side are up or down, we don't need to block */
    if (!need_to_block && fbe_base_config_has_peer_joined((fbe_base_config_t *)provision_drive_p))
    {
        b_local_flag_set = fbe_provision_drive_is_clustered_flag_set(provision_drive_p, FBE_PROVISION_DRIVE_CLUSTERED_FLAG_SLF);
        b_peer_flag_set = fbe_provision_drive_is_peer_clustered_flag_set(provision_drive_p, FBE_PROVISION_DRIVE_CLUSTERED_FLAG_SLF);
        if (b_local_flag_set != b_peer_flag_set)
        {
            need_to_block = FBE_TRUE;
        }
    }

    if (!need_to_block)
    {
        return FBE_FALSE;
    }

    /* Otherwise wait 3 seconds */
    start_time = fbe_provision_drive_get_ds_disabled_start_time(provision_drive_p);
    if (fbe_get_elapsed_seconds(start_time) >= SLF_MAX_BLOCK_DS_DISABLED_SECONDS)
    {
        return FBE_FALSE;
    }

    return FBE_TRUE;
}
/****************************************************************
 * end fbe_provision_drive_slf_block_ds_health_disabled()
 ****************************************************************/

/*!**************************************************************
 * fbe_provision_drive_slf_need_redirect()
 ****************************************************************
 * @brief
 *  This function checks whether we need to redirect IO based on slf. 
 *
 * @param
 *   provision_drive_p - pointer to the provision drive
 *
 * @return fbe_bool_t
 * 
 * @author
 *  06/12/2012 - Created. Lili Chen
 *
 ****************************************************************/
fbe_bool_t fbe_provision_drive_slf_need_redirect(fbe_provision_drive_t *provision_drive_p)
{
    if (fbe_provision_drive_is_slf_enabled() &&
        fbe_provision_drive_is_local_state_set(provision_drive_p, FBE_PVD_LOCAL_STATE_SLF) &&
        !fbe_provision_drive_is_peer_flag_set(provision_drive_p, FBE_PROVISION_DRIVE_CLUSTERED_FLAG_SLF))
    {
        return FBE_TRUE;
    }

    return FBE_FALSE;
}
/****************************************************************
 * end fbe_provision_drive_slf_need_redirect()
 ****************************************************************/

/*!**************************************************************
 * fbe_provision_drive_slf_need_to_send_passive_request()
 ****************************************************************
 * @brief
 *  This function checks whether we need to send passive request. 
 *
 * @param
 *   provision_drive_p - pointer to the provision drive
 *
 * @return fbe_bool_t: TRUE if we need to exit the condition
 * 
 * @author
 *  07/31/2012 - Created. Lili Chen
 *
 ****************************************************************/
fbe_bool_t fbe_provision_drive_slf_need_to_send_passive_request(fbe_provision_drive_t *provision_drive_p)
{
    fbe_bool_t                                  b_is_active;
    fbe_time_t start_time = fbe_provision_drive_get_ds_disabled_start_time(provision_drive_p);
    fbe_time_t current_time = fbe_get_time();
    fbe_status_t status;
    fbe_u32_t seconds_to_wait = SLF_MAX_BLOCK_DS_BROKEN_SECONDS;
    fbe_lifecycle_state_t local_state;

    b_is_active = fbe_base_config_is_active((fbe_base_config_t*)provision_drive_p);
    if (!b_is_active)
    {
        return FBE_FALSE;
    }

    if (fbe_provision_drive_is_slf_enabled() &&
        fbe_provision_drive_is_clustered_flag_set(provision_drive_p, FBE_PROVISION_DRIVE_CLUSTERED_FLAG_SLF) &&
        fbe_base_config_is_peer_present((fbe_base_config_t *)provision_drive_p) &&
        !fbe_provision_drive_is_peer_clustered_flag_set(provision_drive_p, FBE_PROVISION_DRIVE_CLUSTERED_FLAG_SLF))
    {
        if (start_time == 0)
        {
            fbe_provision_drive_set_ds_disabled_start_time(provision_drive_p, current_time);
            return FBE_FALSE;
        }

        fbe_base_object_get_lifecycle_state((fbe_base_object_t *)provision_drive_p, &local_state);
        if (local_state == FBE_LIFECYCLE_STATE_SPECIALIZE)
        {
            seconds_to_wait = SLF_MAX_BLOCK_DS_BROKEN_SECONDS_FOR_SPECIALIZE_STATE;
        }
        if (fbe_get_elapsed_seconds(start_time) >= seconds_to_wait)
        {
            status = fbe_base_config_passive_request((fbe_base_config_t *) provision_drive_p);
            if (status != FBE_STATUS_OK)
            {
                fbe_base_object_trace((fbe_base_object_t *)provision_drive_p, 
                                      FBE_TRACE_LEVEL_WARNING,
                                      FBE_TRACE_MESSAGE_ID_INFO,
                                      "%s passive request failed status: 0x%x\n",
                                      __FUNCTION__, status);
                fbe_provision_drive_set_ds_disabled_start_time(provision_drive_p, current_time);
                return FBE_FALSE;
            }
            fbe_provision_drive_set_ds_disabled_start_time(provision_drive_p, 0);
            return FBE_TRUE;
        }
    }

    return FBE_FALSE;
}
/****************************************************************
 * end fbe_provision_drive_slf_need_to_send_passive_request()
 ****************************************************************/

/*!**************************************************************
 * fbe_provision_drive_slf_passive_need_to_join()
 ****************************************************************
 * @brief
 *  This function checks whether we need to join the active side
 *  to start SLF. 
 *
 * @param
 *   provision_drive_p - pointer to the provision drive
 *
 * @return fbe_bool_t: TRUE if we need to join to start slf
 * 
 * @author
 *  10/29/2012 - Created. Lili Chen
 *
 ****************************************************************/
fbe_bool_t fbe_provision_drive_slf_passive_need_to_join(fbe_provision_drive_t *provision_drive_p)
{
    fbe_bool_t                                  b_is_active;
    fbe_lifecycle_state_t                       local_state= FBE_LIFECYCLE_STATE_INVALID;
    fbe_lifecycle_state_t                       peer_state= FBE_LIFECYCLE_STATE_INVALID;
    fbe_time_t start_time = fbe_provision_drive_get_ds_disabled_start_time(provision_drive_p);
    fbe_time_t current_time = fbe_get_time();

    b_is_active = fbe_base_config_is_active((fbe_base_config_t*)provision_drive_p);
    if (b_is_active)
    {
        return FBE_FALSE;
    }

    fbe_base_config_metadata_get_lifecycle_state((fbe_base_config_t *)provision_drive_p, &local_state);
    fbe_base_config_metadata_get_peer_lifecycle_state((fbe_base_config_t *)provision_drive_p, &peer_state);
    if (fbe_provision_drive_is_slf_enabled() &&
        fbe_provision_drive_is_clustered_flag_set(provision_drive_p, FBE_PROVISION_DRIVE_CLUSTERED_FLAG_SLF) &&
        fbe_base_config_is_peer_present((fbe_base_config_t *)provision_drive_p) &&
        (peer_state == FBE_LIFECYCLE_STATE_READY))
    {
        if (start_time == 0)
        {
            fbe_provision_drive_set_ds_disabled_start_time(provision_drive_p, current_time);
            return FBE_FALSE;
        }

        if (fbe_get_elapsed_seconds(start_time) >= SLF_MAX_BLOCK_DS_BROKEN_SECONDS)
        {
            fbe_provision_drive_set_ds_disabled_start_time(provision_drive_p, 0);
            return FBE_TRUE;
        }
    }

    return FBE_FALSE;
}
/****************************************************************
 * end fbe_provision_drive_slf_passive_need_to_join()
 ****************************************************************/

/*!**************************************************************
 * fbe_provision_drive_slf_check_passive_request_after_eval_slf()
 ****************************************************************
 * @brief
 *  This function checks whether we need to send passive request 
 *  after EVAL SLF. 
 *
 * @param
 *   provision_drive_p - pointer to the provision drive
 *
 * @return fbe_bool_t: TRUE if we need to send passive request
 * 
 * @author
 *  05/03/2013 - Created. Lili Chen
 *
 ****************************************************************/
fbe_bool_t fbe_provision_drive_slf_check_passive_request_after_eval_slf(fbe_provision_drive_t *provision_drive_p)
{
    fbe_bool_t b_is_active;
    fbe_object_id_t last_system_object_id;
    fbe_object_id_t pvd_object_id;
    fbe_provision_drive_nonpaged_metadata_drive_info_t  np_metadata_drive_info;
    fbe_cmi_sp_state_t sp_state;
    fbe_bool_t is_active_sp = FBE_FALSE;
    fbe_bool_t active_on_active_sp = FBE_TRUE;

    fbe_provision_drive_clear_flag(provision_drive_p, FBE_PROVISION_DRIVE_FLAG_CHECK_PASSIVE_REQUEST);

    b_is_active = fbe_base_config_is_active((fbe_base_config_t*)provision_drive_p);
    if (!b_is_active)
    {
        return FBE_FALSE;
    }

    if (!fbe_base_config_has_peer_joined((fbe_base_config_t *)provision_drive_p))
    {
        return FBE_FALSE;
    }

    if (fbe_provision_drive_slf_need_redirect(provision_drive_p))
    {
        /* If we are active and in SLF, send passive request. */
        return FBE_TRUE;
    }

    fbe_database_get_last_system_object_id(&last_system_object_id);
    fbe_base_object_get_object_id((fbe_base_object_t*)provision_drive_p, &pvd_object_id);
    fbe_provision_drive_metadata_get_nonpaged_metadata_drive_info(provision_drive_p, &np_metadata_drive_info);
    fbe_cmi_get_current_sp_state(&sp_state);
    is_active_sp = (sp_state == FBE_CMI_STATE_ACTIVE) ? FBE_TRUE : FBE_FALSE;

    if (fbe_base_config_is_load_balance_enabled() &&
        (np_metadata_drive_info.slot_number % 2) && 
        (pvd_object_id > last_system_object_id))
    {
        active_on_active_sp = FBE_FALSE;
    }

    if ((is_active_sp ^ active_on_active_sp) &&
        !fbe_provision_drive_is_local_state_set(provision_drive_p, FBE_PVD_LOCAL_STATE_SLF) &&
        !fbe_provision_drive_is_peer_flag_set(provision_drive_p, FBE_PROVISION_DRIVE_CLUSTERED_FLAG_SLF))
    {
        /* No SLF, we need to do load balance again. */
        return FBE_TRUE;
    }

    return FBE_FALSE;
}
/****************************************************************
 * end fbe_provision_drive_slf_check_passive_request_after_eval_slf()
 ****************************************************************/


/*******************************
 * end fbe_provision_drive_slf.c
 *******************************/
