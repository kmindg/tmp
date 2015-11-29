/***************************************************************************
 * Copyright (C) EMC Corporation 2007-2012
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_lun_software_errors.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the LUN object code for dealing with software errors.
 * 
 * @version
 *  10/3/2012 - Created. Rob Foley
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe_lun_private.h"
#include "fbe_database.h"
#include "fbe/fbe_service_manager_interface.h"
#include "../../../../../lib/lifecycle/fbe_lifecycle_private.h"

#include "EmcPAL_Misc.h"
#include "fbe/fbe_notification_interface.h"
#include "fbe_notification.h"
#include "fbe_private_space_layout.h"
#include "fbe/fbe_event_log_api.h"                      /*  for fbe_event_log_write */
#include "fbe/fbe_event_log_utils.h"                    /*  for message codes */

/*!**************************************************************
 * fbe_lun_is_unexpected_error_limit_hit()
 ****************************************************************
 * @brief
 *  Checks if our error limit has been exceeded or not.
 *
 * @param lun_p - Lun ptr.
 *
 * @return FBE_TRUE if the error limit is exceeded and LUN
 *                  needs to goto or stay in FAIL.
 *         FBE_FALSE Either we disabled this functionality or
 *                   we have not yet exceeded the limit.
 *
 * @author
 *  6/28/2012 - Created. Rob Foley
 *
 ****************************************************************/

fbe_bool_t fbe_lun_is_unexpected_error_limit_hit(fbe_lun_t *lun_p)
{
    fbe_u32_t num_errors;

    if (fbe_lun_is_flag_set(lun_p, FBE_LUN_FLAGS_DISABLE_FAIL_ON_UNEXPECTED_ERRORS)){
        return FBE_FALSE;
    }

    fbe_lun_get_num_unexpected_errors(lun_p, &num_errors);
    if (num_errors == 0) {
        return FBE_FALSE;
    }
    return FBE_TRUE;
}
/******************************************
 * end fbe_lun_is_unexpected_error_limit_hit()
 ******************************************/

/*!**************************************************************
 * fbe_lun_eval_err_request()
 ****************************************************************
 * @brief
 *  Checks if our error limit has been exceeded or not.
 *
 * @param lun_p - Lun ptr.
 *
 * @return FBE_TRUE if the error limit is exceeded and LUN
 *                  needs to goto or stay in FAIL.
 *         FBE_FALSE Either we disabled this functionality or
 *                   we have not yet exceeded the limit.
 *
 * @author
 *  9/25/2012 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t fbe_lun_eval_err_request(fbe_lun_t *lun_p)
{
    fbe_bool_t b_peer_flag_set = FBE_FALSE;
    fbe_bool_t b_is_active;

    b_is_active = fbe_base_config_is_active((fbe_base_config_t*)lun_p);

    fbe_lun_lock(lun_p);

    /* If we don't already have the join local state set, we should set it now since
     * the first time through this will not be set. 
     */
    if (!fbe_lun_is_local_state_set(lun_p, FBE_LUN_LOCAL_STATE_EVAL_ERR_REQUEST))
    {
        fbe_lun_set_local_state(lun_p, FBE_LUN_LOCAL_STATE_EVAL_ERR_REQUEST);
        fbe_lun_clear_clustered_flag(lun_p, FBE_LUN_CLUSTERED_FLAG_EVAL_ERR_MASK);
        fbe_base_object_trace((fbe_base_object_t *)lun_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "LUN %s ev err request clr clst fl local: 0x%llx peer: 0x%llx state: 0x%llx 0x%x\n", 
                              b_is_active ? "Active" : "Passive",
                              lun_p->lun_metadata_memory.flags,
                              lun_p->lun_metadata_memory_peer.flags,
                              lun_p->local_state,
                              lun_p->lun_metadata_memory_peer.base_config_metadata_memory.lifecycle_state);
    }

    /* We start the process by setting join request. 
     * This gets the peer running the correct condition. 
     */
    if (!fbe_lun_is_clustered_flag_set(lun_p, FBE_LUN_CLUSTERED_FLAG_EVAL_ERR_REQUEST))
    {
        fbe_base_object_trace((fbe_base_object_t *)lun_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "LUN %s eval err marking request local state: 0x%llx\n",
                              b_is_active ? "Active" : "Passive",
                              lun_p->local_state);
        fbe_lun_set_clustered_flag(lun_p, FBE_LUN_CLUSTERED_FLAG_EVAL_ERR_REQUEST);
        fbe_lun_unlock(lun_p);
        return FBE_STATUS_OK;
    }

    /* Next we wait for the peer to tell us it received the join request.
     */
    b_peer_flag_set = fbe_lun_is_peer_clustered_flag_set(lun_p, FBE_LUN_CLUSTERED_FLAG_EVAL_ERR_REQUEST);

    if (!b_peer_flag_set)
    {
        fbe_lun_unlock(lun_p);
        fbe_base_object_trace((fbe_base_object_t *)lun_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "LUN %s eval err wait peer request local: 0x%llx peer: 0x%llx state: 0x%llx\n", 
                              b_is_active ? "Active" : "Passive",
                              lun_p->lun_metadata_memory.flags,
                              lun_p->lun_metadata_memory_peer.flags,
                              lun_p->local_state);
        return FBE_STATUS_OK;
    }
    else
    {
        /* The peer has set the flag, we are ready to do the next set of checks.
         * We need to wait for the peer to get to the join request state as an acknowledgement that 
         * it knows we are starting this join. 
         */
        fbe_base_object_trace((fbe_base_object_t *)lun_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "LUN %s eval err peer request set local: 0x%llx peer: 0x%llx state: 0x%llx 0x%x\n", 
                              b_is_active ? "Active" : "Passive",
                              lun_p->lun_metadata_memory.flags,
                              lun_p->lun_metadata_memory_peer.flags,
                              lun_p->local_state,
                              lun_p->lun_metadata_memory_peer.base_config_metadata_memory.lifecycle_state);
        fbe_lun_set_local_state(lun_p, FBE_LUN_LOCAL_STATE_EVAL_ERR_STARTED);
        fbe_lun_clear_local_state(lun_p, FBE_LUN_LOCAL_STATE_EVAL_ERR_REQUEST);
        fbe_lun_unlock(lun_p);
        return FBE_STATUS_OK;
    }
    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_lun_eval_err_request()
 **************************************/
/*!**************************************************************
 * fbe_lun_eval_err_active()
 ****************************************************************
 * @brief
 *  Checks if our error limit has been exceeded or not.
 *
 * @param lun_p - Lun ptr.
 *
 * @return fbe_status_t
 *
 * @author
 *  9/25/2012 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t fbe_lun_eval_err_active(fbe_lun_t *lun_p)
{
    fbe_status_t status;
    fbe_scheduler_hook_status_t hook_status = FBE_SCHED_STATUS_OK;

    /* What if neither SP reached threshold??  Just clear the flags.
     */
    fbe_lun_lock(lun_p);
    if (!fbe_lun_is_clustered_flag_set(lun_p, FBE_LUN_CLUSTERED_FLAG_THRESHOLD_REACHED) &&
        !fbe_lun_is_peer_clustered_flag_set(lun_p, FBE_LUN_CLUSTERED_FLAG_THRESHOLD_REACHED))
    {
        fbe_base_object_trace((fbe_base_object_t *)lun_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "LUN eval err active, no thresholds\n");
        fbe_lun_clear_local_state(lun_p, FBE_LUN_LOCAL_STATE_EVAL_ERR_MASK);
        fbe_lun_clear_clustered_flag(lun_p, FBE_LUN_CLUSTERED_FLAG_EVAL_ERR_MASK);
        
        fbe_lun_unlock(lun_p);
        status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)lun_p);

        if (status != FBE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t*)lun_p, 
                                  FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s failed to clear condition %d\n", __FUNCTION__, status);
        }
        return FBE_STATUS_OK;
    }

    /* If this SP reached the threshold, then it will be rebooting.
     */
    if (fbe_lun_is_clustered_flag_set(lun_p, FBE_LUN_CLUSTERED_FLAG_THRESHOLD_REACHED))
    {
        status = fbe_cmi_panic_get_permission(FBE_TRUE /* Yes panic this SP */);
        if (status == FBE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t *)lun_p, FBE_TRACE_LEVEL_CRITICAL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                  "LUN err threshold reached.  Reboot this SP.\n");
            fbe_lun_check_hook(lun_p, SCHEDULER_MONITOR_STATE_LUN_EVAL_ERR,
                               FBE_LUN_SUBSTATE_EVAL_ERR_ACTIVE_GRANTED, 0, &hook_status);
        }
        else
        {
            fbe_base_object_trace((fbe_base_object_t *)lun_p, FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                  "LUN err threshold reached.  Reboot this SP denied st:%d.\n", status);
            fbe_lun_check_hook(lun_p, SCHEDULER_MONITOR_STATE_LUN_EVAL_ERR,
                               FBE_LUN_SUBSTATE_EVAL_ERR_ACTIVE_DENIED, 0, &hook_status);
        }
    }
    else if (!fbe_lun_is_clustered_flag_set(lun_p, FBE_LUN_CLUSTERED_FLAG_EVAL_ERR_REBOOT_PEER))
    {
        status = fbe_cmi_panic_get_permission(FBE_FALSE/* not this sp, peer sp */);
        if (status == FBE_STATUS_OK)
        {
            /* Otherwise we give the peer permission to reboot.
             * Eventually they go away and we will go through the "!alive case above".
             */
            fbe_lun_set_clustered_flag(lun_p, FBE_LUN_CLUSTERED_FLAG_EVAL_ERR_REBOOT_PEER);
            fbe_base_object_trace((fbe_base_object_t *)lun_p, FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                  "LUN eval err active grant peer reboot. local: 0x%llx peer: 0x%llx st: %d\n",                               
                                  lun_p->lun_metadata_memory.flags,
                                  lun_p->lun_metadata_memory_peer.flags,
                                  status);
            fbe_lun_check_hook(lun_p, SCHEDULER_MONITOR_STATE_LUN_EVAL_ERR,
                               FBE_LUN_SUBSTATE_EVAL_ERR_PEER_GRANTED, 0, &hook_status);
        }
        else
        {
            fbe_base_object_trace((fbe_base_object_t *)lun_p, FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                  "LUN eval err active denied peer reboot. local: 0x%llx peer: 0x%llx pr_alive: %d\n",                               
                                  lun_p->lun_metadata_memory.flags,
                                  lun_p->lun_metadata_memory_peer.flags,
                                  fbe_cmi_is_peer_alive());
            fbe_lun_check_hook(lun_p, SCHEDULER_MONITOR_STATE_LUN_EVAL_ERR,
                               FBE_LUN_SUBSTATE_EVAL_ERR_PEER_DENIED, 0, &hook_status);
        }
    }
    else
    {
        fbe_base_object_trace((fbe_base_object_t *)lun_p, FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                              "LUN err threshold reached...waiting for peer to reboot.\n");
    }
    fbe_lun_unlock(lun_p);
    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_lun_eval_err_active()
 **************************************/

/*!**************************************************************
 * fbe_lun_eval_err_passive()
 ****************************************************************
 * @brief
 *  Checks if our error limit has been exceeded or not.
 *
 * @param lun_p - Lun ptr.
 *
 * @return fbe_status_t
 *
 * @author
 *  9/25/2012 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t fbe_lun_eval_err_passive(fbe_lun_t *lun_p)
{
    fbe_status_t status;
    fbe_lun_lock(lun_p);
    if (fbe_cmi_is_peer_alive() &&
        !fbe_lun_is_peer_clustered_flag_set(lun_p, FBE_LUN_CLUSTERED_FLAG_EVAL_ERR_REQUEST))
    {
        fbe_base_object_trace((fbe_base_object_t *)lun_p, FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                              "LUN eval err passive peer cleared all flags.\n");
        fbe_lun_clear_local_state(lun_p, FBE_LUN_LOCAL_STATE_EVAL_ERR_MASK);
        fbe_lun_clear_clustered_flag(lun_p, FBE_LUN_CLUSTERED_FLAG_EVAL_ERR_MASK);
        fbe_lun_unlock(lun_p);
        status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)lun_p);

        if (status != FBE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t*)lun_p, 
                                  FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s failed to clear condition %d\n", __FUNCTION__, status);
        }
        return FBE_STATUS_OK;
    }
    if (fbe_lun_is_peer_clustered_flag_set(lun_p, FBE_LUN_CLUSTERED_FLAG_EVAL_ERR_REBOOT_PEER))
    {
        /* If we did not hit any software errors, but they are asking us to reboot, then 
         * log an error trace. 
         */
        if (!fbe_lun_is_clustered_flag_set(lun_p, FBE_LUN_CLUSTERED_FLAG_THRESHOLD_REACHED))
        {
            fbe_base_object_trace((fbe_base_object_t *)lun_p, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                  "LUN eval err passive did not encounter errors. fl: 0x%x pfl: 0x%x err: 0x%x\n",
                                  (unsigned int)lun_p->lun_metadata_memory.flags,
                                  (unsigned int)lun_p->lun_metadata_memory_peer.flags,
                                  lun_p->num_unexpected_errors);
            fbe_lun_clear_local_state(lun_p, FBE_LUN_LOCAL_STATE_EVAL_ERR_MASK);
            fbe_lun_clear_clustered_flag(lun_p, FBE_LUN_CLUSTERED_FLAG_EVAL_ERR_MASK);

            status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)lun_p);
            if (status != FBE_STATUS_OK)
            {
                fbe_base_object_trace((fbe_base_object_t*)lun_p, 
                                      FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                      "%s failed to clear condition %d\n", __FUNCTION__, status);
            }
        }
        else
        {
            /* Peer gave us permission to reboot.  Let's reboot.
             */
            fbe_base_object_trace((fbe_base_object_t *)lun_p, FBE_TRACE_LEVEL_CRITICAL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                  "LUN err threshold reached.  Peer granted permission to reboot this SP.\n");
        }
    }
    fbe_lun_unlock(lun_p);
    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_lun_eval_err_passive()
 **************************************/
/*******************************
 * end fbe_lun_software_errors.c
 *******************************/


