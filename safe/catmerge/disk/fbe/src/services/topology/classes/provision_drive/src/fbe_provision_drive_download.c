/***************************************************************************
 * Copyright (C) EMC Corporation 2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_provision_drive_download.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the code for synchronizing the two SPs for drive
 *  firmware download.
 *
 * @version
 *   10/24/2011 - Created. chenl6
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_types.h"
#include "fbe/fbe_transport.h"
#include "fbe/fbe_payload_ex.h"
#include "fbe_transport_memory.h"
#include "fbe_provision_drive_private.h"
#include "fbe_testability.h"
#include "fbe_service_manager.h"
#include "fbe_cmi.h"

/*!*******************************************************************
 * @def FBE_PROVISION_DRIVE_MAX_DOWNLOAD_SECONDS
 *********************************************************************
 * @brief max number of seconds to quiesce I/O.
 *
 *********************************************************************/
#define FBE_PROVISION_DRIVE_MAX_DOWNLOAD_SECONDS 20  /* changed to 20 since it can take ~14 seconds for drives with emulex 
                                                        paddle cards to respond after download. plus some margin */

/*************************
 *   FUNCTION DEFINITIONS
 *************************/
static fbe_status_t pvd_fw_download_permit_completion(fbe_packet_t * packet_p, fbe_packet_completion_context_t context);
static fbe_status_t pvd_fw_download_start_completion(fbe_packet_t * packet_p, fbe_packet_completion_context_t context);
static fbe_status_t pvd_fw_download_stop_completion(fbe_packet_t * packet_p, fbe_packet_completion_context_t context);
static fbe_bool_t   fbe_provision_drive_is_peer_ready_for_download(fbe_provision_drive_t * provision_drive_p);
static fbe_status_t fbe_provision_drive_ask_download_trial_run_permission_completion(fbe_event_t * event_p, fbe_event_completion_context_t context);
static fbe_status_t fbe_provision_drive_download_trial_run_usurper_completion(fbe_packet_t * packet_p, fbe_packet_completion_context_t context);

/*!****************************************************************************
 * fbe_provision_drive_download_request()
 ******************************************************************************
 * @brief
 *  This function implements the local download request state.
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
 *   10/24/2011 - Created. chenl6
 *
 ******************************************************************************/
fbe_lifecycle_status_t 
fbe_provision_drive_download_request(fbe_provision_drive_t *  provision_drive_p, fbe_packet_t * packet_p)
{
    fbe_bool_t b_peer_flag_set = FBE_FALSE;
    fbe_bool_t b_is_originator;
    fbe_provision_drive_clustered_flags_t peer_clustered_flags;

    b_is_originator = fbe_provision_drive_is_flag_set(provision_drive_p, FBE_PROVISION_DRIVE_FLAG_DOWNLOAD_ORIGINATOR);

    fbe_provision_drive_lock(provision_drive_p);

    /* If we are non-originator, and originator has cleared cluster flags, we should quit. 
     */
    fbe_provision_drive_get_peer_clustered_flags(provision_drive_p, &peer_clustered_flags);
    if (!b_is_originator &&
        !(peer_clustered_flags & FBE_PROVISION_DRIVE_CLUSTERED_FLAG_DOWNLOAD_MASK))
    {
        fbe_provision_drive_set_local_state(provision_drive_p, FBE_PVD_LOCAL_STATE_DOWNLOAD_DONE);
        fbe_provision_drive_clear_local_state(provision_drive_p, FBE_PVD_LOCAL_STATE_DOWNLOAD_REQUEST);
        fbe_provision_drive_clear_clustered_flag(provision_drive_p, FBE_PROVISION_DRIVE_CLUSTERED_FLAG_DOWNLOAD_MASK);
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "PVD fwdl request. %s quit download: 0x%llx peer: 0x%llx\n", 
                               b_is_originator ? "Originator" : "Non-Originator",
                              (unsigned long long)provision_drive_p->provision_drive_metadata_memory.flags,
                              (unsigned long long)provision_drive_p->provision_drive_metadata_memory_peer.flags);
        fbe_provision_drive_unlock(provision_drive_p);
        return FBE_LIFECYCLE_STATUS_RESCHEDULE;
    }

    /* If we don't already have the request state set, we should set it now. 
     */
    if (!fbe_provision_drive_is_local_state_set(provision_drive_p, FBE_PVD_LOCAL_STATE_DOWNLOAD_REQUEST))
    {
        fbe_provision_drive_set_local_state(provision_drive_p, FBE_PVD_LOCAL_STATE_DOWNLOAD_REQUEST);
        fbe_provision_drive_clear_clustered_flag(provision_drive_p, 
                                             FBE_PROVISION_DRIVE_CLUSTERED_FLAG_DOWNLOAD_MASK);
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "PVD fwdl request. %s clear clustered flags local: 0x%llx peer: 0x%llx\n", 
                              b_is_originator ? "Originator" : "Non-Originator",
                              (unsigned long long)provision_drive_p->provision_drive_metadata_memory.flags,
                              (unsigned long long)provision_drive_p->provision_drive_metadata_memory_peer.flags);
    }

    /* We start the process by setting download request. 
     * This gets the peer running the correct condition. 
     */
    if (!fbe_provision_drive_is_clustered_flag_set(provision_drive_p, FBE_PROVISION_DRIVE_CLUSTERED_FLAG_DOWNLOAD_REQUEST))
    {
        fbe_provision_drive_set_clustered_flag(provision_drive_p, FBE_PROVISION_DRIVE_CLUSTERED_FLAG_DOWNLOAD_REQUEST);
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "PVD fwdl request. %s marking request local state: 0x%llx\n",
                              b_is_originator ? "Originator" : "Non-Originator",
                              (unsigned long long)provision_drive_p->local_state);
        fbe_provision_drive_unlock(provision_drive_p);
        return FBE_LIFECYCLE_STATUS_RESCHEDULE;
    }

    /* Next we wait for the peer to tell us it received the download request.
     */
    if (b_is_originator && fbe_provision_drive_is_peer_ready_for_download(provision_drive_p))
    {
        b_peer_flag_set = fbe_provision_drive_is_peer_clustered_flag_set(provision_drive_p, 
                                                                 FBE_PROVISION_DRIVE_CLUSTERED_FLAG_DOWNLOAD_REQUEST);
        if (!b_peer_flag_set)
        {
            fbe_base_object_trace((fbe_base_object_t *)provision_drive_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                  "PVD fwdl request. %s wait peer request local: 0x%llx peer: 0x%llx\n", 
                                  b_is_originator ? "Originator" : "Non-Originator",
                                  (unsigned long long)provision_drive_p->provision_drive_metadata_memory.flags,
                                  (unsigned long long)provision_drive_p->provision_drive_metadata_memory_peer.flags);
            fbe_provision_drive_unlock(provision_drive_p);
            return FBE_LIFECYCLE_STATUS_DONE;
        }
    }

    /* The peer has set the flag, we are ready for the next state.
     */
    fbe_base_object_trace((fbe_base_object_t *)provision_drive_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                          "PVD fwdl request. %s local: 0x%llx peer: 0x%llx state: 0x%llx 0x%x\n", 
                          b_is_originator ? "Originator" : "Non-Originator",
                          (unsigned long long)provision_drive_p->provision_drive_metadata_memory.flags,
                          (unsigned long long)provision_drive_p->provision_drive_metadata_memory_peer.flags,
                          (unsigned long long)provision_drive_p->local_state,
                          provision_drive_p->provision_drive_metadata_memory_peer.base_config_metadata_memory.lifecycle_state);
    if (b_is_originator)
    {
        fbe_provision_drive_set_local_state(provision_drive_p, FBE_PVD_LOCAL_STATE_DOWNLOAD_STARTED);
    }
    else
    {
        fbe_provision_drive_set_local_state(provision_drive_p, FBE_PVD_LOCAL_STATE_DOWNLOAD_PEER_START);
    }
    fbe_provision_drive_clear_local_state(provision_drive_p, FBE_PVD_LOCAL_STATE_DOWNLOAD_REQUEST);
    fbe_provision_drive_unlock(provision_drive_p);

    /* Set the start time now.
     */
    fbe_provision_drive_set_download_start_time(provision_drive_p, fbe_get_time());
    return FBE_LIFECYCLE_STATUS_RESCHEDULE;
}
/**************************************
 * end fbe_provision_drive_download_request()
 **************************************/

/*!****************************************************************************
 * fbe_provision_drive_download_started_originator()
 ******************************************************************************
 * @brief
 *  This function implements the download started state for originator side.
 *
 * @param provision_drive_p - provision drive object
 * @param packet_p - packet pointer.
 * 
 * @return fbe_status_t
 *
 * @author
 *   10/24/2011 - Created. chenl6
 *
 ******************************************************************************/
fbe_lifecycle_status_t 
fbe_provision_drive_download_started_originator(fbe_provision_drive_t *  provision_drive_p, fbe_packet_t * packet_p)
{
    fbe_bool_t          b_peer_flag_set = FBE_FALSE;
    fbe_bool_t          b_io_already_quiesced = FBE_FALSE;
    fbe_scheduler_hook_status_t hook_status = FBE_SCHED_STATUS_OK;

    /* Quiesce the IO before proceeding */
    fbe_provision_drive_utils_quiesce_io_if_needed(provision_drive_p, &b_io_already_quiesced);
    if (b_io_already_quiesced == FBE_FALSE)
    {
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    fbe_base_object_trace((fbe_base_object_t*) provision_drive_p,      
                          FBE_TRACE_LEVEL_INFO,           
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,  
                          "PVD fwdl started_originator. io quiesced\n");

    fbe_provision_drive_lock(provision_drive_p);

    /* Now we set the bit that says we are starting. 
     * This bit always gets set first by the originator side. 
     * When the non-originator sees the started peer bit, it will also set the started bit. 
     * The non-originator side will thereafter know we are done when this bit is cleared. 
     */
    if (!fbe_provision_drive_is_clustered_flag_set(provision_drive_p, FBE_PROVISION_DRIVE_CLUSTERED_FLAG_DOWNLOAD_STARTED))
    {
        fbe_provision_drive_clear_clustered_flag(provision_drive_p, FBE_PROVISION_DRIVE_CLUSTERED_FLAG_DOWNLOAD_REQUEST);
        fbe_provision_drive_set_clustered_flag(provision_drive_p, FBE_PROVISION_DRIVE_CLUSTERED_FLAG_DOWNLOAD_STARTED);
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "PVD fwdl started_originator. mark started local: 0x%llx peer: 0x%llx state: 0x%llx\n",
                              (unsigned long long)provision_drive_p->provision_drive_metadata_memory.flags,
                              (unsigned long long)provision_drive_p->provision_drive_metadata_memory_peer.flags,
                              (unsigned long long)provision_drive_p->local_state);
        fbe_provision_drive_unlock(provision_drive_p);
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /* Now we need to wait for the peer to acknowledge that it saw us start.
     */
    if (fbe_provision_drive_is_peer_ready_for_download(provision_drive_p))
    {
        b_peer_flag_set = fbe_provision_drive_is_peer_clustered_flag_set(provision_drive_p, 
                                                                 FBE_PROVISION_DRIVE_CLUSTERED_FLAG_DOWNLOAD_STARTED);
        if (!b_peer_flag_set)
        {
            fbe_base_object_trace((fbe_base_object_t *)provision_drive_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                  "PVD fwdl started_originator. wait peer started local: 0x%llx peer: 0x%llx\n", 
                                  (unsigned long long)provision_drive_p->provision_drive_metadata_memory.flags,
                                  (unsigned long long)provision_drive_p->provision_drive_metadata_memory_peer.flags);
            fbe_provision_drive_unlock(provision_drive_p);
            return FBE_LIFECYCLE_STATUS_DONE;
        }
    }

    fbe_provision_drive_unlock(provision_drive_p);

    /* If a debug hook is set, we need to execute the specified action.
       This hook will hold pvd from send down permit command to PDO */
    fbe_provision_drive_check_hook(provision_drive_p,  
                                   SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_DOWNLOAD_PERMISSION, 
                                   FBE_PROVISION_DRIVE_SUBSTATE_DOWNLOAD_PERMISSION, 0, &hook_status);
    if (hook_status == FBE_SCHED_STATUS_DONE) 
    {
        return FBE_LIFECYCLE_STATUS_DONE;
    }
    
    /* Start the real download.
     */
    return fbe_provision_drive_send_download_command_to_pdo(provision_drive_p, packet_p,
                                                            FBE_PHYSICAL_DRIVE_CONTROL_CODE_FW_DOWNLOAD_PERMIT,
                                                            pvd_fw_download_permit_completion);
}
/**************************************
 * end fbe_provision_drive_download_started_originator()
 **************************************/

/*!****************************************************************************
 * pvd_fw_download_permit_completion()
 ******************************************************************************
 * @brief
 *  This function is used as completion function for sending usurper commands
 *  to PDO for fw download permit
 * 
 * @param packet_p   - Pointer to the packet.
 * @param context    - context, which is a pointer to the base object.
 *
 * @return fbe_status_t
 *
 * @author
 *  02/05/2013  Wayne Garrett - Created. 
 * 
 ******************************************************************************/
static fbe_status_t 
pvd_fw_download_permit_completion(fbe_packet_t * packet_p,
                                  fbe_packet_completion_context_t context)
{
    fbe_provision_drive_t *             provision_drive_p = (fbe_provision_drive_t*)context;
    fbe_payload_ex_t *                  payload_p = NULL;
    fbe_payload_control_operation_t *   control_operation_p = NULL;
    fbe_payload_control_status_t        control_status;
    fbe_status_t                        pkt_status = FBE_STATUS_GENERIC_FAILURE;

    /* Get the packet status. */
    pkt_status = fbe_transport_get_status_code (packet_p);

    /* Get control status */
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);
    fbe_payload_control_get_status(control_operation_p, &control_status);

    if (pkt_status != FBE_STATUS_OK || control_status != FBE_PAYLOAD_CONTROL_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p, FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s failed. pkt_status:%d control_status:%d\n", __FUNCTION__, pkt_status, control_status);

        /* The download is aborted, or something bad happened. */
        fbe_provision_drive_lock(provision_drive_p);
        fbe_provision_drive_set_local_state(provision_drive_p, FBE_PVD_LOCAL_STATE_DOWNLOAD_PUP);
        fbe_provision_drive_clear_local_state(provision_drive_p, FBE_PVD_LOCAL_STATE_DOWNLOAD_STARTED);
        fbe_provision_drive_unlock(provision_drive_p);        
    }

    /*TODO: to avoid having download state machine send multiple permit requests, it should change its
            local state to something like download_completed. */

    return FBE_STATUS_OK;
}


/*!****************************************************************************
 * fbe_provision_drive_download_start_non_originator()
 ******************************************************************************
 * @brief
 *  This function implements the download started state for non-originator side.
 * 
 * @param provision_drive_p - provision drive object
 * @param packet_p - packet pointer.
 * 
 * @return fbe_status_t
 *
 * @author
 *   10/24/2011 - Created. chenl6
 *
 ******************************************************************************/
fbe_lifecycle_status_t 
fbe_provision_drive_download_start_non_originator(fbe_provision_drive_t *  provision_drive_p, fbe_packet_t * packet_p)
{
    fbe_bool_t b_peer_flag_set = FBE_FALSE;
    fbe_bool_t b_local_flag_set = FBE_FALSE;

    fbe_provision_drive_lock(provision_drive_p);

    /* If the originator is gone, or the originator quit download, change state. */
    if (!fbe_provision_drive_is_peer_ready_for_download(provision_drive_p) ||
		(!fbe_provision_drive_is_peer_clustered_flag_set(provision_drive_p, FBE_PROVISION_DRIVE_CLUSTERED_FLAG_DOWNLOAD_REQUEST) && 
		 !fbe_provision_drive_is_peer_clustered_flag_set(provision_drive_p, FBE_PROVISION_DRIVE_CLUSTERED_FLAG_DOWNLOAD_STARTED)))
    {
        fbe_provision_drive_set_local_state(provision_drive_p, FBE_PVD_LOCAL_STATE_DOWNLOAD_DONE);
        fbe_provision_drive_clear_local_state(provision_drive_p, FBE_PVD_LOCAL_STATE_DOWNLOAD_PEER_START);
        fbe_provision_drive_clear_clustered_flag(provision_drive_p, FBE_PROVISION_DRIVE_CLUSTERED_FLAG_DOWNLOAD_MASK);
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "PVD fwdl start_non_originator. peer not ready local: 0x%llx peer: 0x%llx state: 0x%llx\n", 
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
                                                                     FBE_PROVISION_DRIVE_CLUSTERED_FLAG_DOWNLOAD_STARTED);

    b_local_flag_set = fbe_provision_drive_is_clustered_flag_set(provision_drive_p, 
                                                                 FBE_PROVISION_DRIVE_CLUSTERED_FLAG_DOWNLOAD_STARTED);

    /* We first wait for the peer to start the download. 
     */
    if (!b_peer_flag_set && !b_local_flag_set)
    {
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "PVD fwdl start_non_originator. waiting peer download started local: 0x%llx peer: 0x%llx state: 0x%llx\n", 
                              (unsigned long long)provision_drive_p->provision_drive_metadata_memory.flags,
                              (unsigned long long)provision_drive_p->provision_drive_metadata_memory_peer.flags,
                              (unsigned long long)provision_drive_p->local_state);
        fbe_provision_drive_unlock(provision_drive_p);
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /* At this point the peer started flag is set.
     */
    if (!b_local_flag_set)
    {
        /* Peer is starting the download.  Send DOWNLOAD_START to PDO. It's required to send this since this will force PDO to do the inquiry
           command after download completes.
         */
        fbe_provision_drive_unlock(provision_drive_p);

        return fbe_provision_drive_send_download_command_to_pdo(provision_drive_p, packet_p,
                                                                FBE_PHYSICAL_DRIVE_CONTROL_CODE_FW_DOWNLOAD_START,
                                                                pvd_fw_download_start_completion);        

    }

    fbe_provision_drive_unlock(provision_drive_p);

    return FBE_LIFECYCLE_STATUS_DONE;
}
/**************************************
 * end fbe_provision_drive_download_start_non_originator()
 **************************************/

/*!****************************************************************************
 * pvd_fw_download_start_completion()
 ******************************************************************************
 * @brief
 *  This function is used as completion function for sending usurper commands
 *  to PDO for fw download non-originator start.  
 * 
 * @param packet_p   - Pointer to the packet.
 * @param context    - context, which is a pointer to the base object.
 *
 * @return fbe_status_t
 *
 * @author
 *  02/05/2013  Wayne Garrett - Created. 
 * 
 ******************************************************************************/
static fbe_status_t 
pvd_fw_download_start_completion(fbe_packet_t * packet_p,
                                  fbe_packet_completion_context_t context)
{
    fbe_provision_drive_t *             provision_drive_p = (fbe_provision_drive_t*)context;
    fbe_payload_ex_t *                  payload_p = NULL;
    fbe_payload_control_operation_t *   control_operation_p = NULL;
    fbe_payload_control_status_t        control_status;
    fbe_status_t                        pkt_status = FBE_STATUS_GENERIC_FAILURE;

    /* Get the packet status. */
    pkt_status = fbe_transport_get_status_code (packet_p);

    /* Get control status */
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);
    fbe_payload_control_get_status(control_operation_p, &control_status);

    if (pkt_status != FBE_STATUS_OK || control_status != FBE_PAYLOAD_CONTROL_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s failed. pkt_status:%d control_status:%d\n", __FUNCTION__, pkt_status, control_status);
		
        /*This is non-originator side, so even incase failure (i.e. object is not there, bad status etc) we are going to allow originator 
	     to continue. */
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p, FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                "%sAllowing fwdl to continue at non-originator side.\n", __FUNCTION__);		
    }
	 
    fbe_base_object_trace((fbe_base_object_t *)provision_drive_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                          "%s mark started state: 0x%llx\n", __FUNCTION__, provision_drive_p->local_state);

    fbe_provision_drive_lock(provision_drive_p);
    fbe_provision_drive_clear_clustered_flag(provision_drive_p, FBE_PROVISION_DRIVE_CLUSTERED_FLAG_DOWNLOAD_REQUEST);
    fbe_provision_drive_set_clustered_flag(provision_drive_p, FBE_PROVISION_DRIVE_CLUSTERED_FLAG_DOWNLOAD_STARTED);

    /* Both sides set started flag. 
     */
    fbe_provision_drive_set_local_state(provision_drive_p, FBE_PVD_LOCAL_STATE_DOWNLOAD_PEER_STOP);
    fbe_provision_drive_clear_local_state(provision_drive_p, FBE_PVD_LOCAL_STATE_DOWNLOAD_PEER_START);
    fbe_provision_drive_unlock(provision_drive_p);


    return FBE_STATUS_OK;
}



/*!****************************************************************************
 * fbe_provision_drive_download_stop_non_originator()
 ******************************************************************************
 * @brief
 *  This function implements the download stopped state for non-originator side.
 * 
 * @param provision_drive_p - provision drive object
 * @param packet_p - packet pointer.
 * 
 * @return fbe_status_t
 *
 * @author
 *   10/24/2011 - Created. chenl6
 *
 ******************************************************************************/
fbe_lifecycle_status_t 
fbe_provision_drive_download_stop_non_originator(fbe_provision_drive_t *  provision_drive_p, fbe_packet_t * packet_p)
{
    fbe_bool_t b_peer_flag_set = FBE_FALSE;
    fbe_bool_t b_local_flag_set = FBE_FALSE;

    fbe_provision_drive_lock(provision_drive_p);

    b_peer_flag_set = fbe_provision_drive_is_peer_clustered_flag_set(provision_drive_p, 
                                                                     FBE_PROVISION_DRIVE_CLUSTERED_FLAG_DOWNLOAD_STARTED);
    b_local_flag_set = fbe_provision_drive_is_clustered_flag_set(provision_drive_p, 
                                                                 FBE_PROVISION_DRIVE_CLUSTERED_FLAG_DOWNLOAD_STARTED);
    if (fbe_provision_drive_is_peer_ready_for_download(provision_drive_p) &&
		b_peer_flag_set)
    {
        /* Wait originator to clear started flag. 
         */
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "PVD fwdl stop_non_originator. waiting peer local: 0x%llx peer: 0x%llx state: 0x%llx\n", 
                              (unsigned long long)provision_drive_p->provision_drive_metadata_memory.flags,
                              (unsigned long long)provision_drive_p->provision_drive_metadata_memory_peer.flags,
                              (unsigned long long)provision_drive_p->local_state);
        fbe_provision_drive_unlock(provision_drive_p);
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    fbe_provision_drive_unlock(provision_drive_p);

    /* The originator has completed download. 
     * Send DOWNLOAD_STOP to PDO.
     */

    return fbe_provision_drive_send_download_command_to_pdo(provision_drive_p, packet_p,
                                                            FBE_PHYSICAL_DRIVE_CONTROL_CODE_FW_DOWNLOAD_STOP,
                                                            pvd_fw_download_stop_completion);    
}
/**************************************
 * end fbe_provision_drive_download_stop_non_originator()
 **************************************/

/*!****************************************************************************
 * pvd_fw_download_stop_completion()
 ******************************************************************************
 * @brief
 *  This function is used as completion function for sending usurper commands
 *  to PDO for fw download non-originator stop.  
 * 
 * @param packet_p   - Pointer to the packet.
 * @param context    - context, which is a pointer to the base object.
 *
 * @return fbe_status_t
 *
 * @author
 *  02/05/2013  Wayne Garrett - Created. 
 * 
 ******************************************************************************/
static fbe_status_t 
pvd_fw_download_stop_completion(fbe_packet_t * packet_p,
                                fbe_packet_completion_context_t context)
{
    fbe_provision_drive_t *             provision_drive_p = (fbe_provision_drive_t*)context;
    fbe_payload_ex_t *                  payload_p = NULL;
    fbe_payload_control_operation_t *   control_operation_p = NULL;
    fbe_payload_control_status_t        control_status;
    fbe_status_t                        pkt_status = FBE_STATUS_GENERIC_FAILURE;

    /* Get the packet status. */
    pkt_status = fbe_transport_get_status_code (packet_p);

    /* Get control status */
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);
    fbe_payload_control_get_status(control_operation_p, &control_status);

    if (pkt_status != FBE_STATUS_OK || control_status != FBE_PAYLOAD_CONTROL_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s failed. pkt_status:%d control_status:%d\n", __FUNCTION__, pkt_status, control_status); 
		
        /*This is non-originator side, so even incase failure (i.e. object is not there, bad status etc) we are going to allow non-originator 
        to go into PUP. It will go into download done state at the end.  Also it will clean up the states for us (AR:557915) */
        
	 fbe_base_object_trace((fbe_base_object_t *)provision_drive_p, FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                "%sAllowing fwdl to continue at non-originator side.\n", __FUNCTION__);
    }

    fbe_base_object_trace((fbe_base_object_t *)provision_drive_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                          "%s mark started state: 0x%llx\n", __FUNCTION__, provision_drive_p->local_state);

    fbe_provision_drive_lock(provision_drive_p);
    fbe_provision_drive_set_local_state(provision_drive_p, FBE_PVD_LOCAL_STATE_DOWNLOAD_PUP);
    fbe_provision_drive_clear_local_state(provision_drive_p, FBE_PVD_LOCAL_STATE_DOWNLOAD_PEER_STOP);
    fbe_provision_drive_unlock(provision_drive_p);


    return FBE_STATUS_OK;
}




/*!****************************************************************************
 * fbe_provision_drive_download_pup()
 ******************************************************************************
 * @brief
 *  This function implements the local download powerup state.
 *
 * @param provision_drive_p - provision drive object
 * @param packet_p - packet pointer.
 * 
 * @return fbe_status_t
 *
 * @author
 *   10/24/2011 - Created. chenl6
 *
 ******************************************************************************/
fbe_lifecycle_status_t 
fbe_provision_drive_download_pup(fbe_provision_drive_t *  provision_drive_p, fbe_packet_t * packet_p)
{
    fbe_bool_t          b_is_originator;
    fbe_path_state_t    path_state = FBE_PATH_STATE_INVALID;
    fbe_block_edge_t    * edge_p = NULL;
    fbe_status_t        status;
    fbe_time_t          start_time, elapsed_seconds;

    b_is_originator = fbe_provision_drive_is_flag_set(provision_drive_p, FBE_PROVISION_DRIVE_FLAG_DOWNLOAD_ORIGINATOR);

    fbe_provision_drive_lock(provision_drive_p);

    /* We want to clear our local flags first.
     */
    if (fbe_provision_drive_is_clustered_flag_set(provision_drive_p, FBE_PROVISION_DRIVE_CLUSTERED_FLAG_DOWNLOAD_REQUEST) ||
        fbe_provision_drive_is_clustered_flag_set(provision_drive_p, FBE_PROVISION_DRIVE_CLUSTERED_FLAG_DOWNLOAD_STARTED))
    {
        /* At this point only started should be set, but we're clearing everything here.
         */
        fbe_provision_drive_clear_clustered_flag(provision_drive_p, 
                                             (FBE_PROVISION_DRIVE_CLUSTERED_FLAG_DOWNLOAD_REQUEST |
                                              FBE_PROVISION_DRIVE_CLUSTERED_FLAG_DOWNLOAD_STARTED));
        /* Set downloaded so that we acknowledge the peer is present.
         */
        fbe_provision_drive_unlock(provision_drive_p);
        return FBE_LIFECYCLE_STATUS_RESCHEDULE;
    }

    if (fbe_provision_drive_is_peer_ready_for_download(provision_drive_p))
    {
        if (fbe_provision_drive_is_peer_clustered_flag_set(provision_drive_p, 
                                                           FBE_PROVISION_DRIVE_CLUSTERED_FLAG_DOWNLOAD_STARTED))
        {
            fbe_base_object_trace((fbe_base_object_t *)provision_drive_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                  "PVD fwdl pup. %s wait peer local: 0x%llx peer: 0x%llx state:0x%llx\n", 
                                  b_is_originator ? "Originator" : "Non-Originator",
                                  (unsigned long long)provision_drive_p->provision_drive_metadata_memory.flags,
                                  (unsigned long long)provision_drive_p->provision_drive_metadata_memory_peer.flags,
                                  (unsigned long long)provision_drive_p->local_state);
            fbe_provision_drive_unlock(provision_drive_p);
            return FBE_LIFECYCLE_STATUS_DONE;
        }
    }

    /* We check whether the time has expired.
     */
    start_time = fbe_provision_drive_get_download_start_time(provision_drive_p);
    elapsed_seconds = fbe_get_elapsed_seconds(start_time);
    if (elapsed_seconds > FBE_PROVISION_DRIVE_MAX_DOWNLOAD_SECONDS)
    {
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "PVD fwdl pup. %s download time exceeded: 0x%llx peer: 0x%llx state:0x%llx\n",  
                              b_is_originator ? "Originator" : "Non-Originator",
                              (unsigned long long)provision_drive_p->provision_drive_metadata_memory.flags,
                              (unsigned long long)provision_drive_p->provision_drive_metadata_memory_peer.flags,
                              (unsigned long long)provision_drive_p->local_state);
        fbe_provision_drive_clear_local_state(provision_drive_p, 
                                              FBE_PVD_LOCAL_STATE_DOWNLOAD_PUP);
        fbe_provision_drive_set_local_state(provision_drive_p, FBE_PVD_LOCAL_STATE_DOWNLOAD_DONE);
        fbe_provision_drive_clear_clustered_flag(provision_drive_p, 
                                             FBE_PROVISION_DRIVE_CLUSTERED_FLAG_DOWNLOAD_MASK);
        fbe_provision_drive_unlock(provision_drive_p);
        return FBE_LIFECYCLE_STATUS_RESCHEDULE;
    }
    fbe_provision_drive_unlock(provision_drive_p);


    /* We check the edge status to see whether the PDO goes to ready/fail state.
     */
    fbe_base_config_get_block_edge( (fbe_base_config_t *) provision_drive_p, &edge_p, 0);
    status = fbe_block_transport_get_path_state(edge_p, &path_state);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p, 
                              FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                              "%s cannot get block edge path state, status:0x%x!!\n", __FUNCTION__, status);
        return status;
    }

    if (path_state != FBE_PATH_STATE_ENABLED &&
		path_state != FBE_PATH_STATE_BROKEN &&
		path_state != FBE_PATH_STATE_GONE)
    {	 
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "PVD fwdl pup. wait PDO ready local: 0x%llx peer: 0x%llx state: 0x%x\n", 
                              (unsigned long long)provision_drive_p->provision_drive_metadata_memory.flags,
                              (unsigned long long)provision_drive_p->provision_drive_metadata_memory_peer.flags,
                              path_state);
        /* Reschedule for 0.5 second. */
        fbe_lifecycle_reschedule(&fbe_provision_drive_lifecycle_const, 
                                 (fbe_base_object_t*)provision_drive_p, (fbe_lifecycle_timer_msec_t)500);
        return FBE_LIFECYCLE_STATUS_DONE;
    }


    /* Now we are done and change local state.
     */
    fbe_provision_drive_lock(provision_drive_p);
    fbe_provision_drive_clear_local_state(provision_drive_p, 
                                          FBE_PVD_LOCAL_STATE_DOWNLOAD_PUP);
    fbe_provision_drive_set_local_state(provision_drive_p, FBE_PVD_LOCAL_STATE_DOWNLOAD_DONE);
    fbe_provision_drive_clear_clustered_flag(provision_drive_p, 
                                             FBE_PROVISION_DRIVE_CLUSTERED_FLAG_DOWNLOAD_MASK);
    fbe_provision_drive_unlock(provision_drive_p);

    return FBE_LIFECYCLE_STATUS_RESCHEDULE;
}
/**************************************
 * end fbe_provision_drive_download_pup()
 **************************************/

/*!****************************************************************************
 * fbe_provision_drive_download_done()
 ******************************************************************************
 * @brief
 *  This function implements the download done local state.
 *
 * @param provision_drive_p - provision drive object
 * @param packet_p - packet pointer.
 * 
 * @return fbe_lifecycle_status_t
 *
 * @author
 *   10/24/2011 - Created. chenl6
 *
 ******************************************************************************/
fbe_lifecycle_status_t 
fbe_provision_drive_download_done(fbe_provision_drive_t *  provision_drive_p, fbe_packet_t * packet_p)
{
    fbe_status_t status;
    fbe_bool_t b_is_originator;
    fbe_base_config_downstream_health_state_t   downstream_health_state;
    fbe_path_attr_t                             path_attr = 0;
    fbe_provision_drive_clustered_flags_t       peer_clustered_flags = 0;
    fbe_scheduler_hook_status_t                 hook_status;

    fbe_provision_drive_check_hook(provision_drive_p, SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_DOWNLOAD,
                                   FBE_PROVISION_DRIVE_SUBSTATE_DOWNLOAD_DONE, 0, &hook_status);
    if (hook_status == FBE_SCHED_STATUS_DONE) {
        return FBE_LIFECYCLE_STATUS_DONE;
    }


    b_is_originator = fbe_provision_drive_is_flag_set(provision_drive_p, FBE_PROVISION_DRIVE_FLAG_DOWNLOAD_ORIGINATOR);

    fbe_provision_drive_lock(provision_drive_p);

    /* Wait until peer cleans up before completing the DONE state.  Also, if peer doesn't exist
       or isn't in the expected Ready state then proceed with completing the DONE state.
    */
    fbe_provision_drive_get_peer_clustered_flags(provision_drive_p, &peer_clustered_flags);

    if (fbe_provision_drive_is_peer_ready_for_download(provision_drive_p) &&
        (peer_clustered_flags & FBE_PROVISION_DRIVE_CLUSTERED_FLAG_DOWNLOAD_MASK))
    {
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "PVD fwdl done. %s waiting. local: 0x%llx peer: 0x%llx state:0x%llx\n",
                              b_is_originator ? "Originator" : "Non-Originator",
                              (unsigned long long)provision_drive_p->provision_drive_metadata_memory.flags,
                              (unsigned long long)provision_drive_p->provision_drive_metadata_memory_peer.flags,
                              (unsigned long long)provision_drive_p->local_state);

        fbe_lifecycle_reschedule(&fbe_provision_drive_lifecycle_const, 
                                 (fbe_base_object_t*)provision_drive_p, (fbe_lifecycle_timer_msec_t)50); //msec
        fbe_provision_drive_unlock(provision_drive_p);
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /* Clear local state.
     */
    fbe_provision_drive_clear_local_state(provision_drive_p, 
                                          FBE_PVD_LOCAL_STATE_DOWNLOAD_MASK);
    fbe_base_object_trace((fbe_base_object_t *)provision_drive_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                          "PVD fwdl done. %s cleanup. local: 0x%llx peer: 0x%llx state:0x%llx\n",
                          b_is_originator ? "Originator" : "Non-Originator",
                          (unsigned long long)provision_drive_p->provision_drive_metadata_memory.flags,
                          (unsigned long long)provision_drive_p->provision_drive_metadata_memory_peer.flags,
                          (unsigned long long)provision_drive_p->local_state);
    fbe_provision_drive_unlock(provision_drive_p);

    /* Let's clear the condition.
     */
    status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)provision_drive_p);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s can't clear current condition, status: 0x%x\n",
                              __FUNCTION__, status);
    }

    /* Clear edge attributes.
     */
    if (fbe_provision_drive_is_flag_set(provision_drive_p, FBE_PROVISION_DRIVE_FLAG_DOWNLOAD_ORIGINATOR))
    {
        fbe_provision_drive_set_download_originator(provision_drive_p, FBE_FALSE);
        /* Download request is done. Clear edge attr now. */
        fbe_block_transport_server_propagate_path_attr_with_mask_all_servers(
		        &((fbe_base_config_t *)provision_drive_p)->block_transport_server, 0,
                FBE_BLOCK_PATH_ATTR_FLAGS_CLIENT_DOWNLOAD_MASK);
    }

    /* Get the current path attr.
     */
    status = fbe_block_transport_get_path_attributes(((fbe_base_config_t *)provision_drive_p)->block_edge_p,
                                                     &path_attr);
    if (status != FBE_STATUS_OK)
    {
        fbe_provision_drive_utils_trace(provision_drive_p,
                                        FBE_TRACE_LEVEL_ERROR,
                                        FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                        FBE_PROVISION_DRIVE_DEBUG_FLAG_GENERAL_TRACING,
                                        "%s failed to get path attr.\n", __FUNCTION__);
    }
    /* We might be blocking any downstream edge state change event. Check the state now
     * to determine if we need state change. 
     */
    downstream_health_state = fbe_provision_drive_verify_downstream_health(provision_drive_p);

    /* Set the specific lifecycle condition based on health of the downtream path. */
    status =  fbe_provision_drive_set_condition_based_on_downstream_health(provision_drive_p,
                                                                           path_attr,
                                                                           downstream_health_state);

    /* If PDO set DOWNLOAD request attribute again, we may ignore the notification due to
     * download in progress. Process download attribute here to make sure we don't lost this event
     */
    if (path_attr & FBE_BLOCK_PATH_ATTR_FLAGS_CLIENT_DOWNLOAD_MASK) {
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p,
                              FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s: path attr: 0x%x, restart download\n",
                              __FUNCTION__, path_attr);
        fbe_provision_drive_process_download_path_attributes(provision_drive_p, path_attr);
    } else {
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p,
                              FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s: download finished\n", __FUNCTION__);
    }

    return FBE_LIFECYCLE_STATUS_RESCHEDULE;
}

/**************************************
 * end fbe_provision_drive_download_done()
 **************************************/

/*!****************************************************************************
 * fbe_provision_drive_send_download_command_to_pdo()
 ******************************************************************************
 * @brief
 *  This function sends download request to PDO.
 *
 * @param provision_drive_p - The provision drive.
 * @param packet_p - The packet requesting this operation.
 * @param command - Usurper control code to send to PDO.
 * @param completion_function - Completion call-back function.
 *
 * @return fbe_lifecycle_status_t - status of condition
 *
 * @author
 *  03/09/2011 - Created. chenl6
 *  12/11/2013 - Wayne Garrett - Made Asynch
 *
 ******************************************************************************/
fbe_lifecycle_status_t
fbe_provision_drive_send_download_command_to_pdo(fbe_provision_drive_t * provision_drive_p, 
                                                 fbe_packet_t * packet_p,
												 fbe_physical_drive_control_code_t command,
                                                 fbe_packet_completion_function_t completion_function)
{
    fbe_payload_ex_t *                  payload_p = NULL;
    fbe_payload_control_operation_t *   control_operation_p = NULL;

    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);

    fbe_payload_control_build_operation (control_operation_p,
                                         command,
                                         NULL,
                                         0);
        
    fbe_transport_set_packet_attr(packet_p, FBE_PACKET_FLAG_TRAVERSE);
    fbe_transport_set_completion_function(packet_p, completion_function, provision_drive_p);
    fbe_block_transport_send_control_packet(((fbe_base_config_t *)provision_drive_p)->block_edge_p, packet_p);
       
    return FBE_LIFECYCLE_STATUS_PENDING;    
}
/******************************************************************************
 * end fbe_provision_drive_send_download_command_to_pdo()
 ******************************************************************************/


/*!****************************************************************************
 * fbe_provision_drive_is_peer_ready_for_download()
 ******************************************************************************
 * @brief
 *  This function checks whether the peer is ready for download process.
 *
 * @param provision_drive_p - The provision drive.
 * @param packet_p - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  10/25/2011 - Created. chenl6
 *
 ******************************************************************************/
static fbe_bool_t 
fbe_provision_drive_is_peer_ready_for_download(fbe_provision_drive_t * provision_drive_p)
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
    if (peer_state != FBE_LIFECYCLE_STATE_READY)
    {
        return FBE_FALSE;
    }

    return FBE_TRUE;
}
/******************************************
 * end fbe_provision_drive_is_peer_ready_for_download()
 ******************************************/

/*!****************************************************************************
 * fbe_provision_drive_process_download_path_attributes()
 ******************************************************************************
 * @brief
 *  This function processes download path attribute changes.
 *
 * @param provision_drive_p - The provision drive.
 * @param path_attr - The path attribute.
 *
 * @return - whether we should lock the state/attr change.
 *
 * @author
 *  10/25/2011 - Created. chenl6
 *  03/02/2013 - Added handling of PVD lifecycle state not READY. Darren Insko
 *
 ******************************************************************************/
fbe_bool_t 
fbe_provision_drive_process_download_path_attributes(fbe_provision_drive_t * provision_drive_p,
                                                     fbe_path_attr_t path_attr)
{
    fbe_path_attr_t                             download_path_attr;
    fbe_provision_drive_local_state_t           local_state;
    fbe_status_t                                status;
    fbe_lifecycle_state_t                       lifecycle_state = FBE_LIFECYCLE_STATE_INVALID;
    fbe_bool_t                                  done = FBE_FALSE;
    fbe_u32_t                                   number_of_clients = 0;

    download_path_attr = (path_attr & FBE_BLOCK_PATH_ATTR_FLAGS_CLIENT_DOWNLOAD_MASK);
    fbe_block_transport_server_get_number_of_clients(&((fbe_base_config_t *)provision_drive_p)->block_transport_server, 
                                                     &number_of_clients);

    fbe_provision_drive_lock(provision_drive_p);
    fbe_provision_drive_get_local_state(provision_drive_p, &local_state);
    if (download_path_attr & FBE_BLOCK_PATH_ATTR_FLAGS_CLIENT_DOWNLOAD_MASK)
    {
        if (local_state & FBE_PVD_LOCAL_STATE_DOWNLOAD_MASK)
        {
            fbe_provision_drive_unlock(provision_drive_p);
            fbe_base_object_trace((fbe_base_object_t *)provision_drive_p,
                                  FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                  "PVD fwdl_process_path_attributes, Already in download. local_state: 0x%llx path_attr: 0x%x\n",    
                                  local_state, download_path_attr);
            return FBE_TRUE;
        }

        status = fbe_lifecycle_get_state(&fbe_provision_drive_lifecycle_const,
                                         (fbe_base_object_t *)provision_drive_p,
                                         &lifecycle_state);
        if (lifecycle_state != FBE_LIFECYCLE_STATE_READY)
        {
            fbe_base_object_trace((fbe_base_object_t *)provision_drive_p,
                                  FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                  "PVD fwdl_process_path_attributes. Not Ready. lifecycle_state: %d path_attr: 0x%x\n",    
                                  lifecycle_state, download_path_attr);

            fbe_provision_drive_set_local_state(provision_drive_p, FBE_PVD_LOCAL_STATE_DOWNLOAD_DONE);
            fbe_provision_drive_clear_clustered_flag(provision_drive_p, 
                                             FBE_PROVISION_DRIVE_CLUSTERED_FLAG_DOWNLOAD_MASK);

            if (lifecycle_state == FBE_LIFECYCLE_STATE_FAIL)
            {
                fbe_lifecycle_set_cond(&fbe_provision_drive_lifecycle_const,
                                       (fbe_base_object_t *) provision_drive_p,
                                       FBE_BASE_CONFIG_LIFECYCLE_COND_DOWNSTREAM_HEALTH_BROKEN);        
            }
            else
            {
                fbe_lifecycle_set_cond(&fbe_provision_drive_lifecycle_const,
                                       (fbe_base_object_t *) provision_drive_p,
                                       FBE_BASE_CONFIG_LIFECYCLE_COND_DOWNSTREAM_HEALTH_DISABLED);        
            }

            fbe_provision_drive_unlock(provision_drive_p);
            return FBE_TRUE;
        }
    }

    if (download_path_attr & FBE_BLOCK_PATH_ATTR_FLAGS_CLIENT_DOWNLOAD_REQ_TRIAL_RUN)
    {
        /* Process trial run request. */
        fbe_provision_drive_set_local_state(provision_drive_p, FBE_PVD_LOCAL_STATE_DOWNLOAD_TRIAL_RUN);

        fbe_lifecycle_set_cond(&fbe_provision_drive_lifecycle_const,
                                   (fbe_base_object_t *) provision_drive_p,
                                   FBE_PROVISION_DRIVE_LIFECYCLE_COND_DOWNLOAD);        
    }
    else if (download_path_attr == FBE_BLOCK_PATH_ATTR_FLAGS_CLIENT_DOWNLOAD_REQ ||
             download_path_attr == FBE_BLOCK_PATH_ATTR_FLAGS_CLIENT_DOWNLOAD_REQ_FAST_DL)
    {
        /* Process regular or fast download request.    */
        /* This is the Originator side of the download. */
        fbe_provision_drive_set_download_originator(provision_drive_p, FBE_TRUE);

        if (number_of_clients == 0 ||
            download_path_attr == FBE_BLOCK_PATH_ATTR_FLAGS_CLIENT_DOWNLOAD_REQ_FAST_DL)
        {
            /* For a fast download or no VD above, no need to notify RAID so start download immediately. */
            fbe_provision_drive_set_local_state(provision_drive_p, FBE_PVD_LOCAL_STATE_DOWNLOAD_REQUEST);
            fbe_lifecycle_set_cond(&fbe_provision_drive_lifecycle_const,
                                   (fbe_base_object_t *) provision_drive_p,
                                   FBE_PROVISION_DRIVE_LIFECYCLE_COND_DOWNLOAD);
        }
        else
        {
            /* Otherwise, propagate request to RAID. */
            fbe_block_transport_server_propagate_path_attr_with_mask_all_servers(
		        &((fbe_base_config_t *)provision_drive_p)->block_transport_server, download_path_attr,
                FBE_BLOCK_PATH_ATTR_FLAGS_CLIENT_DOWNLOAD_MASK);
        }
    }
    else if(download_path_attr == FBE_BLOCK_PATH_ATTR_FLAGS_CLIENT_DOWNLOAD_GRANT)
    {
        /* We should already set the grant bit now */
    }
    else
    {
        /* No download attr */
        if ((local_state & FBE_PVD_LOCAL_STATE_DOWNLOAD_STARTED) ||
            (local_state & FBE_PVD_LOCAL_STATE_DOWNLOAD_REQUEST))
        {
            /* The download is aborted */
            fbe_provision_drive_set_local_state(provision_drive_p, FBE_PVD_LOCAL_STATE_DOWNLOAD_PUP);
            fbe_provision_drive_clear_local_state(provision_drive_p, 
                                                  FBE_PVD_LOCAL_STATE_DOWNLOAD_STARTED | FBE_PVD_LOCAL_STATE_DOWNLOAD_REQUEST);
        }

        if (local_state & FBE_PVD_LOCAL_STATE_DOWNLOAD_PUP)
        {
            /* Set download condition to let it run immediately. */
            fbe_lifecycle_set_cond(&fbe_provision_drive_lifecycle_const,
                                   (fbe_base_object_t *)provision_drive_p,
                                   FBE_PROVISION_DRIVE_LIFECYCLE_COND_DOWNLOAD);
        }

        if (!(local_state & FBE_PVD_LOCAL_STATE_DOWNLOAD_MASK))
        {
            /* If we are not started yet, propagate the attribute to RAID. 
             * Otherwise the attribute will be cleared after the PVD is done.
             */
            fbe_block_transport_server_propagate_path_attr_with_mask_all_servers(
		        &((fbe_base_config_t *)provision_drive_p)->block_transport_server, download_path_attr,
                FBE_BLOCK_PATH_ATTR_FLAGS_CLIENT_DOWNLOAD_MASK);
        }
    }

    if (local_state & FBE_PVD_LOCAL_STATE_DOWNLOAD_MASK)
    {
        done = FBE_TRUE;
    }
    fbe_provision_drive_unlock(provision_drive_p);

    return done;
}
/******************************************
 * end fbe_provision_drive_process_download_path_attributes()
 ******************************************/

													 
/*!****************************************************************************
 * fbe_provision_drive_ask_download_trial_run_permission()
 ******************************************************************************
 * @brief
 *  This function allocates an event and sends to upstream objects for 
 *  trial run permission.
 *
 * @param provision_drive_p - The provision drive.
 * @param packet_p - The packet.
 *
 * @return fbe_lifecycle_status_t
 *
 * @author
 *  07/27/2012 - Created. Lili Chen
 *
 ******************************************************************************/
fbe_lifecycle_status_t 
fbe_provision_drive_ask_download_trial_run_permission(fbe_provision_drive_t * provision_drive_p,
                                                      fbe_packet_t * packet_p)
{
    fbe_event_t *               event_p = NULL;
    fbe_event_stack_t *         event_stack = NULL;
    fbe_lba_t                   capacity;

    event_p = fbe_memory_ex_allocate(sizeof(fbe_event_t));
    if(event_p == NULL)
    { 
        /* we should send it later */
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /* Initialize the event. */
    fbe_event_init(event_p);
    fbe_event_set_master_packet(event_p, packet_p);
    event_stack = fbe_event_allocate_stack(event_p);

    /* Fill the LBA range. */
    fbe_base_config_get_capacity((fbe_base_config_t *)provision_drive_p, &capacity);
    fbe_event_stack_set_extent(event_stack, 0, (fbe_block_count_t) capacity);

    /* Set the event completion function. */
    fbe_event_stack_set_completion_function(event_stack,
                                            fbe_provision_drive_ask_download_trial_run_permission_completion,
                                            provision_drive_p);

    fbe_base_config_send_event((fbe_base_config_t *)provision_drive_p, FBE_EVENT_TYPE_DOWNLOAD_REQUEST, event_p);
    return FBE_LIFECYCLE_STATUS_PENDING;
}
/******************************************************************************
 * end fbe_provision_drive_ask_download_trial_run_permission()
*******************************************************************************/

/*!****************************************************************************
 * fbe_provision_drive_ask_download_trial_run_permission_completion()
 ******************************************************************************
 * @brief
 *  This function is used as completion function for the event to ask for the
 *  permission from RAID object.
 * 
 * @param event_p   - Event pointer.
 * @param context_p - Context which was passed with event.
 *
 * @return fbe_status_t
 *  The status of the operation typically FBE_STATUS_OK.
 *
 * @author
 *  07/27/2012 - Created. Lili Chen
 * 
 ******************************************************************************/
static fbe_status_t 
fbe_provision_drive_ask_download_trial_run_permission_completion(fbe_event_t * event_p,
                                                                 fbe_event_completion_context_t context)
{
    fbe_event_stack_t *                     event_stack = NULL;
    fbe_packet_t *                          packet_p = NULL;
    fbe_provision_drive_t *                 provision_drive_p = NULL;
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_u32_t                               event_flags = 0;
    fbe_event_status_t                      event_status = FBE_EVENT_STATUS_INVALID;
    fbe_physical_drive_control_code_t       command;
    fbe_payload_ex_t *                      payload_p = NULL;
    fbe_payload_control_operation_t *       control_operation_p = NULL;
    fbe_bool_t                              trial_run = FBE_TRUE;

    event_stack = fbe_event_get_current_stack(event_p);
    fbe_event_get_master_packet(event_p, &packet_p);

    provision_drive_p = (fbe_provision_drive_t *) context;

    fbe_base_object_trace((fbe_base_object_t*) provision_drive_p,      
                          FBE_TRACE_LEVEL_INFO,           
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,  
                          "%s: entry\n", __FUNCTION__);

    /* Get flag and status of the event. */
    fbe_event_get_flags (event_p, &event_flags);
    fbe_event_get_status(event_p, &event_status);

    /* Release the event. */
    fbe_event_release_stack(event_p, event_stack);
    fbe_event_destroy(event_p);
    fbe_memory_ex_release(event_p);

    /* Send usurper command to PDO. */
    if (((event_status == FBE_EVENT_STATUS_OK) && !(event_flags & FBE_EVENT_FLAG_DENY)) ||
        (event_status == FBE_EVENT_STATUS_NO_USER_DATA))
    {
        command = FBE_PHYSICAL_DRIVE_CONTROL_CODE_FW_DOWNLOAD_PERMIT;
    }
    else 
    {
        command = FBE_PHYSICAL_DRIVE_CONTROL_CODE_FW_DOWNLOAD_REJECT;
    }

    fbe_base_object_trace((fbe_base_object_t *)provision_drive_p,
                          FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                          "PVD ask_download_trial_run_permission_completion  %s event_status: %d event_flags: 0x%x\n", 
                          (command == FBE_PHYSICAL_DRIVE_CONTROL_CODE_FW_DOWNLOAD_REJECT ? "REJECT" : "PERMIT"),
                          event_status, event_flags);

    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);
    fbe_payload_control_build_operation (control_operation_p,
                                         command,
                                         &trial_run,
                                         sizeof(fbe_bool_t));
    /* Set our completion function. */
    fbe_transport_set_completion_function(packet_p, 
                                          fbe_provision_drive_download_trial_run_usurper_completion,
                                          provision_drive_p);
    fbe_transport_set_packet_attr(packet_p, FBE_PACKET_FLAG_TRAVERSE);
    status = fbe_block_transport_send_control_packet(((fbe_base_config_t *)provision_drive_p)->block_edge_p, packet_p);

    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_provision_drive_ask_download_trial_run_permission_completion()
*******************************************************************************/

/*!****************************************************************************
 * fbe_provision_drive_download_trial_run_usurper_completion()
 ******************************************************************************
 * @brief
 *  This function is used as completion function for sending usurper commands
 *  to PDO for permit/rejection.
 * 
 * @param packet_p   - Pointer to the packet.
 * @param context    - context, which is a pointer to the base object.
 *
 * @return fbe_status_t
 *
 * @author
 *  07/27/2012 - Created. Lili Chen
 * 
 ******************************************************************************/
static fbe_status_t 
fbe_provision_drive_download_trial_run_usurper_completion(fbe_packet_t * packet_p,
                                                          fbe_packet_completion_context_t context)
{
    fbe_provision_drive_t *             provision_drive_p = (fbe_provision_drive_t*)context;
    fbe_payload_ex_t *                  payload_p = NULL;
    fbe_payload_control_operation_t *   control_operation_p = NULL;
    fbe_payload_control_status_t        control_status;
    fbe_status_t                        status = FBE_STATUS_GENERIC_FAILURE;

    /* Get the packet status. */
    status = fbe_transport_get_status_code (packet_p);

    /* Get control status */
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);
    fbe_payload_control_get_status(control_operation_p, &control_status);

    if((control_status == FBE_PAYLOAD_CONTROL_STATUS_OK) && (status == FBE_STATUS_OK))
    {    
        fbe_provision_drive_lock(provision_drive_p);
        fbe_provision_drive_clear_local_state(provision_drive_p, FBE_PVD_LOCAL_STATE_DOWNLOAD_TRIAL_RUN);
        fbe_provision_drive_unlock(provision_drive_p);

        status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)provision_drive_p);
        if (status != FBE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t*)provision_drive_p, 
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s can't clear condition, status: 0x%x", __FUNCTION__, status);
        }
    }
    else
    {
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p,
                              FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                              "PVD download trial_run_completion: Failed. pkt_status:%d cntrl_status:%d \n", status, control_status); 
    }

    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_provision_drive_download_trial_run_usurper_completion()
*******************************************************************************/

/******************************************
 * end file fbe_provision_drive_download.c
 ******************************************/
