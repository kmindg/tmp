#include "fbe_provision_drive_private.h"
#include "fbe/fbe_notification_interface.h"
#include "fbe_notification.h"
#include "fbe_database.h"

/* Forward declarations */
static fbe_status_t fbe_provision_drive_state_change_event_entry(fbe_provision_drive_t * provision_drive_p, 
                                                                 fbe_event_context_t event_context);
static fbe_status_t fbe_provision_drive_peer_nonpaged_write_event_entry(fbe_object_handle_t object_handle, 
                                                                        fbe_event_type_t event_type,
                                                                        fbe_event_context_t event_context);
static fbe_status_t fbe_provision_drive_handle_peer_memory_update(fbe_provision_drive_t * provision_drive_p);
static fbe_status_t fbe_provision_drive_event_peer_contact_lost(fbe_provision_drive_t * provision_drive_p, 
                                                                fbe_event_context_t event_context);
static fbe_status_t fbe_provision_drive_handle_peer_drive_fault(fbe_provision_drive_t * provision_drive_p);

/*!***************************************************************
 * fbe_provision_drive_event_entry()
 ****************************************************************
 * @brief
 *  This function is called to pass an event to a given instance
 *  of the provision_drive class.
 *
 * @param object_handle - The object receiving the event.
 * @param event_type - Type of event that is arriving. e.g. state change.
 * @param event_context - Context that is associated with the event.
 *
 * @return
 *  fbe_status_t
 *
 * @author
 *  07/24/2009 - Created. Peter Puhov
 *
 ****************************************************************/
fbe_status_t 
fbe_provision_drive_event_entry(fbe_object_handle_t object_handle, 
                                fbe_event_type_t event_type,
                                fbe_event_context_t event_context)
{
    fbe_provision_drive_t * provision_drive_p = NULL;
    fbe_status_t status;
    
    provision_drive_p = (fbe_provision_drive_t *)fbe_base_handle_to_pointer(object_handle);

    fbe_base_object_trace((fbe_base_object_t*)provision_drive_p,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry event_type %d context 0x%p\n",
                          __FUNCTION__, event_type, event_context);

    /* First handle the event we have received.
     */
    switch (event_type)
    {
        case FBE_EVENT_TYPE_EDGE_STATE_CHANGE:
        case FBE_EVENT_TYPE_ATTRIBUTE_CHANGED:
            status = fbe_provision_drive_state_change_event_entry(provision_drive_p, event_context);
            break;        

        case FBE_EVENT_TYPE_PEER_NONPAGED_WRITE:
            status = fbe_provision_drive_peer_nonpaged_write_event_entry(object_handle,
                                                                         event_type,
                                                                         event_context);
            break;

        case FBE_EVENT_TYPE_PEER_NONPAGED_CHKPT_UPDATE:
            status = fbe_provision_drive_send_checkpoint_notification(provision_drive_p);
            break;

        case FBE_EVENT_TYPE_PEER_MEMORY_UPDATE:
            fbe_provision_drive_handle_peer_memory_update(provision_drive_p);
            /* We need to call both memory update handlers since both PVD and base config might have work to do.
             */
            status = fbe_base_config_event_entry(object_handle, event_type, event_context);
            break;

        case FBE_EVENT_TYPE_PEER_CONTACT_LOST:
            status = fbe_provision_drive_event_peer_contact_lost(provision_drive_p, event_context);  
            /* We need to call both event handlers since both PVD and base config might have work to do.
             */
            status = fbe_base_config_event_entry(object_handle, event_type, event_context);
            break;            

        default:
            status = fbe_base_config_event_entry(object_handle, event_type, event_context);
            break;
    }

    return status;
}
/******************************************
 * end fbe_provision_drive_event_entry()
 ******************************************/


/*!**************************************************************
 * fbe_provision_drive_state_change_event_entry()
 ****************************************************************
 * @brief
 *  This is the handler function for a change in the edge state 
 *  between Provision Virtual Drive object and Logical drive 
 *  object.
 *
 * @param logical_drive_p - provision drive object that is changing.
 * @param event_context - The context for the event.
 *
 * @return - Status of the handling.
 *
 * @author
 *  9/08/2009 - Created. DP.
 ****************************************************************/
static fbe_status_t
fbe_provision_drive_state_change_event_entry(fbe_provision_drive_t * provision_drive_p,
                                             fbe_event_context_t event_context)
{

    fbe_base_config_downstream_health_state_t   downstream_health_state;
    fbe_status_t                                status = FBE_STATUS_OK;
    fbe_path_state_t                            path_state;
    fbe_path_attr_t                             path_attr = 0;
    fbe_base_edge_t *                           base_edge_p = NULL;
    
    /* Get the edge from the event context and verify that it is not NULL. */
    base_edge_p = (fbe_base_edge_t *) event_context;
    if (base_edge_p == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p, 
                              FBE_TRACE_LEVEL_ERROR, 
                              FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                              "%s Invalid event context\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* fetch the path state of the edge. */ 
    status = fbe_block_transport_get_path_state(&(((fbe_base_config_t *)provision_drive_p)->block_edge_p[base_edge_p->client_index]),
                                                &path_state);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p, 
                              FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                              "%s cannot get block edge path state, status:0x%x!!\n", __FUNCTION__, status);
        return status;
    }

    /* fetch the path attributes of the edge. */
    status = fbe_block_transport_get_path_attributes(&(((fbe_base_config_t *)provision_drive_p)->block_edge_p[base_edge_p->client_index]),
                                                     &path_attr);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p, 
                              FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                              "%s cannot get block edge path attr, status:0x%x!!\n", __FUNCTION__, status);
        return status;
    }

    fbe_provision_drive_utils_trace(provision_drive_p,
                                    FBE_TRACE_LEVEL_DEBUG_HIGH,
                                    FBE_TRACE_MESSAGE_ID_INFO,
                                    FBE_PROVISION_DRIVE_DEBUG_FLAG_GENERAL_TRACING, 
                                    "%s path state %d\n", __FUNCTION__, path_state);

    switch (path_state)
    {
        case FBE_PATH_STATE_ENABLED:
        case FBE_PATH_STATE_DISABLED:
        case FBE_PATH_STATE_BROKEN:
        case FBE_PATH_STATE_GONE:

            if (fbe_provision_drive_process_download_path_attributes(provision_drive_p, path_attr))
            {
                /* Drive firmware download is in progress. We'll mask the downstream state change for now. 
                 */
                fbe_base_object_trace((fbe_base_object_t *)provision_drive_p, 
                                      FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_EXIT, 
                                      "%s download in process. ignoring path state %d\n", __FUNCTION__, path_state);
                break;
            }
            if (path_attr & FBE_BLOCK_PATH_ATTR_FLAGS_TIMEOUT_ERRORS)
            {
                fbe_block_transport_server_propagate_path_attr_with_mask_all_servers(
                        &((fbe_base_config_t *)provision_drive_p)->block_transport_server,
                        FBE_BLOCK_PATH_ATTR_FLAGS_TIMEOUT_ERRORS,  /* Path attribute to set */
                        FBE_BLOCK_PATH_ATTR_FLAGS_TIMEOUT_ERRORS   /* Mask of attributes to not set if already set*/);
            }

            fbe_provision_drive_process_health_check_path_attributes(provision_drive_p, path_attr);

            /* determine health of the downstream path. */
            downstream_health_state = fbe_provision_drive_verify_downstream_health(provision_drive_p);

            fbe_base_object_trace((fbe_base_object_t *)provision_drive_p, 
                                  FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY, 
                                  "%s  path state %d, path attr 0x%x health state %d\n", 
                                  __FUNCTION__, path_state, path_attr, downstream_health_state);

            if (!fbe_provision_drive_process_state_change_for_faulted_drives(provision_drive_p, path_state, path_attr, downstream_health_state))
            {
                /* We'll mask the downstream state change if SLF is going on. 
                 */
                fbe_base_object_trace((fbe_base_object_t *)provision_drive_p, 
                                      FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_EXIT, 
                                      "%s SLF in process. ignoring state %d attr 0x%x\n", __FUNCTION__, path_state, path_attr);
                break;
            }

            /* Abort all monitor ops from terminator queue if disk is going to down */
            if(downstream_health_state == FBE_DOWNSTREAM_HEALTH_BROKEN)
            {
                fbe_metadata_stripe_lock_element_abort(&provision_drive_p->base_config.metadata_element);
                fbe_provision_drive_abort_monitor_ops(provision_drive_p);
            }

            /* Set the specific lifecycle condition based on health of the downtream path. */
            status =  fbe_provision_drive_set_condition_based_on_downstream_health(provision_drive_p,
                                                                                   path_attr,
                                                                                   downstream_health_state);
            break;

        case FBE_PATH_STATE_SLUMBER:
            break;

        default:
            /* This is a path state we do not expect.
             */
            status = FBE_STATUS_GENERIC_FAILURE;
            fbe_base_object_trace((fbe_base_object_t*)provision_drive_p, 
                                  FBE_TRACE_LEVEL_WARNING, 
                                  FBE_TRACE_MESSAGE_ID_CREATE_OBJECT, 
                                  "%s path state unexpected %d\n", __FUNCTION__, path_state);
            break;

    }

    return status;
}
/****************************************************
 * end fbe_provision_drive_state_change_event_entry()
 ****************************************************/

/*!****************************************************************************
 * fbe_provision_drive_peer_nonpaged_write_event_entry()
 ******************************************************************************
 * @brief
 *  This function handles a "peer nonpaged write" event. This is an event to write nonpaged
 *  metadat. The appropriate condition is set accordingly.
 *  This is used as part of peer-to-peer communication to write
 *  nopaged metadat of provision drive object across SPs.
 *
 * @param object_handle - The object receiving the event.
 * @param event_type - Type of event that is arriving. e.g. state change.
 * @param event_context - Context that is associated with the event.
 *
 *
 * @return fbe_status_t   
 *
 * @author
 *   03-2011 - Created.  Vishnu Sharma 
 *
 ******************************************************************************/
static fbe_status_t
fbe_provision_drive_peer_nonpaged_write_event_entry(fbe_object_handle_t object_handle,
                                                    fbe_event_type_t event_type,
                                                    fbe_event_context_t event_context)
{    
    fbe_status_t            status = FBE_STATUS_OK;
    fbe_provision_drive_t  *provision_drive_p = NULL;
    fbe_bool_t              end_of_life_state = FBE_FALSE;
    fbe_lifecycle_state_t   lifecycle_state;

    provision_drive_p = (fbe_provision_drive_t *)fbe_base_handle_to_pointer(object_handle);
    fbe_base_object_get_lifecycle_state((fbe_base_object_t *)provision_drive_p, &lifecycle_state);

    /* First check if the drive should be taken offline.
     */
    if (fbe_provision_drive_metadata_is_np_flag_set(provision_drive_p, 
                                                           FBE_PROVISION_DRIVE_NP_FLAG_EAS_STARTED)) {
        /* Set the condition to take the provision drive offline.
         */
        if (lifecycle_state != FBE_LIFECYCLE_STATE_FAIL) {
            fbe_lifecycle_set_cond(&fbe_provision_drive_lifecycle_const,
                                   (fbe_base_object_t *)provision_drive_p,
                                   FBE_BASE_CONFIG_LIFECYCLE_COND_DOWNSTREAM_HEALTH_BROKEN);
        }
    } else if (lifecycle_state == FBE_LIFECYCLE_STATE_FAIL) {
        /* Else set the condition to check if we can exit the failed state.
         */
        fbe_lifecycle_set_cond(&fbe_provision_drive_lifecycle_const,
                               (fbe_base_object_t *)provision_drive_p,
                               FBE_BASE_CONFIG_LIFECYCLE_COND_DOWNSTREAM_HEALTH_BROKEN);
    }

    /* Now handle the EOL flag.
     */
    fbe_provision_drive_metadata_get_end_of_life_state(provision_drive_p, &end_of_life_state);
    if (end_of_life_state == FBE_TRUE) {
        /* set the verify end of life state condition to update the upstream edge if needed. */
        fbe_lifecycle_set_cond(&fbe_provision_drive_lifecycle_const,
                             (fbe_base_object_t *)provision_drive_p,
                             FBE_PROVISION_DRIVE_LIFECYCLE_COND_VERIFY_END_OF_LIFE_STATE);
         
    }
    /*send the event to base config */
    status = fbe_base_config_event_entry(object_handle, event_type, event_context);
    return status;
} 
/****************************************************
 * end fbe_provision_drive_peer_nonpaged_write_event_entry()
 ****************************************************/

/*!**************************************************************
 * fbe_provision_drive_handle_peer_memory_update()
 ****************************************************************
 * @brief
 *  Handle the peer updating its metadata memory.
 *
 * @param provision_drive_p - ptr to PVD.               
 *
 * @return fbe_status_t
 *
 * @author
 *  10/24/2011 - Created. chenl6
 *
 ****************************************************************/

static fbe_status_t 
fbe_provision_drive_handle_peer_memory_update(fbe_provision_drive_t * provision_drive_p)
{
    fbe_lifecycle_state_t   local_state= FBE_LIFECYCLE_STATE_INVALID, peer_state = FBE_LIFECYCLE_STATE_INVALID;

    fbe_provision_drive_lock(provision_drive_p);

    /* If the peer requested download, and we have not acknowledged this yet, 
     * we need to set the download condition. 
     */
    if (fbe_provision_drive_is_peer_clustered_flag_set(provision_drive_p, FBE_PROVISION_DRIVE_CLUSTERED_FLAG_DOWNLOAD_REQUEST) &&
        !fbe_provision_drive_is_peer_clustered_flag_set(provision_drive_p, FBE_PROVISION_DRIVE_CLUSTERED_FLAG_DOWNLOAD_STARTED) &&
        !fbe_provision_drive_is_clustered_flag_set(provision_drive_p, FBE_PROVISION_DRIVE_CLUSTERED_FLAG_DOWNLOAD_REQUEST))
        
    {        
        fbe_lifecycle_set_cond(&fbe_provision_drive_lifecycle_const,
                               (fbe_base_object_t *) provision_drive_p,
                               FBE_PROVISION_DRIVE_LIFECYCLE_COND_DOWNLOAD);

    }

    /* If the peer requested health check, and we have not acknowledged this yet, 
     * we need to set the health check condition.  This will force PVD into activate state which will hold off IO, allowing
     * HC to be sent on originator side. 
     */
    if (fbe_provision_drive_is_peer_clustered_flag_set(provision_drive_p, FBE_PROVISION_DRIVE_CLUSTERED_FLAG_HEALTH_CHECK_REQUEST) &&
        !fbe_provision_drive_is_peer_clustered_flag_set(provision_drive_p, FBE_PROVISION_DRIVE_CLUSTERED_FLAG_HEALTH_CHECK_STARTED) &&
        !fbe_provision_drive_is_clustered_flag_set(provision_drive_p, FBE_PROVISION_DRIVE_CLUSTERED_FLAG_HEALTH_CHECK_REQUEST) &&
        !fbe_provision_drive_is_flag_set(provision_drive_p, FBE_PROVISION_DRIVE_FLAG_HC_ORIGINATOR))        
    {
        fbe_provision_drive_set_local_state(provision_drive_p, FBE_PVD_LOCAL_STATE_HEALTH_CHECK_REQUEST);
        fbe_lifecycle_set_cond(&fbe_provision_drive_lifecycle_const,
                               (fbe_base_object_t *) provision_drive_p,
                               FBE_PROVISION_DRIVE_LIFECYCLE_COND_HEALTH_CHECK_PASSIVE_SIDE);

        fbe_base_object_trace((fbe_base_object_t*)provision_drive_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "PVD_HC: Non-orig request received. local_state: 0x%llx local_clstr: 0x%llx peer_clstr: 0x%llx\n", 
                              provision_drive_p->local_state,
                              provision_drive_p->provision_drive_metadata_memory.flags,
                              provision_drive_p->provision_drive_metadata_memory_peer.flags);
    }

    /* Handle cases when peer has drive fault.
     */
    fbe_provision_drive_handle_peer_drive_fault(provision_drive_p);

    /* Check clear end of life flag */
    if (fbe_provision_drive_is_peer_clustered_flag_set(provision_drive_p, FBE_PROVISION_DRIVE_CLUSTERED_FLAG_CLEAR_END_OF_LIFE))
    {
        /* Set the flag to indicate that we are clearing EOL */
        fbe_provision_drive_set_local_state(provision_drive_p, FBE_PVD_LOCAL_STATE_CLEAR_EOL_REQUEST);

        /* set a condition to clear EOL state */
        fbe_lifecycle_set_cond(&fbe_provision_drive_lifecycle_const,
                               (fbe_base_object_t*)provision_drive_p,
                               FBE_PROVISION_DRIVE_LIFECYCLE_COND_CLEAR_END_OF_LIFE);

        /* Jump to Activate state */            
        fbe_lifecycle_set_cond(&fbe_provision_drive_lifecycle_const,
                               (fbe_base_object_t *)provision_drive_p, FBE_BASE_OBJECT_LIFECYCLE_COND_ACTIVATE);
    }

#if 0
    /* If we are passive and in FAIL state, and peer goes to READY state,
     * we need to go to activate state to join. 
     */
    fbe_base_config_metadata_get_peer_lifecycle_state((fbe_base_config_t *)provision_drive_p, &peer_state);
    fbe_base_config_metadata_get_lifecycle_state((fbe_base_config_t *)provision_drive_p, &local_state);
    if (!fbe_base_config_is_active((fbe_base_config_t*)provision_drive_p) &&
        (local_state == FBE_LIFECYCLE_STATE_FAIL) &&
        (peer_state == FBE_LIFECYCLE_STATE_READY))
    {
        fbe_base_object_trace((fbe_base_object_t*)provision_drive_p, 
                              FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s peer goes to ready, start SLF \n", __FUNCTION__);
        fbe_lifecycle_set_cond(&fbe_provision_drive_lifecycle_const,
                               (fbe_base_object_t *) provision_drive_p,
                               FBE_BASE_OBJECT_LIFECYCLE_COND_ACTIVATE);
    }
#endif

    /* If we are passive and not in HIBERNATE state, and peer goes to HIBERNATE state,
     * we need to follow after we have synced with peer.
     */
    fbe_base_config_metadata_get_peer_lifecycle_state((fbe_base_config_t *)provision_drive_p, &peer_state);
    fbe_base_config_metadata_get_lifecycle_state((fbe_base_config_t *)provision_drive_p, &local_state);
    if (!fbe_base_config_is_active((fbe_base_config_t*)provision_drive_p) &&
        (local_state != FBE_LIFECYCLE_STATE_HIBERNATE) &&
        (peer_state == FBE_LIFECYCLE_STATE_HIBERNATE) &&
        fbe_base_config_is_clustered_flag_set((fbe_base_config_t *)provision_drive_p, FBE_BASE_CONFIG_CLUSTERED_FLAG_HIBERNATE_READY))
    {
        fbe_base_object_trace((fbe_base_object_t*)provision_drive_p, 
                              FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s peer goes to hibernate, start hibernation \n", __FUNCTION__);
        fbe_lifecycle_set_cond(&fbe_provision_drive_lifecycle_const,
                               (fbe_base_object_t *) provision_drive_p,
                               FBE_BASE_OBJECT_LIFECYCLE_COND_HIBERNATE);
    }

    /* If we are in SLF state, and peer goes to FAIL state,
     * we need to go to FAIL state. 
     */
    if (/* !fbe_base_config_is_active((fbe_base_config_t*)provision_drive_p) && */
        fbe_provision_drive_is_local_state_set(provision_drive_p, FBE_PVD_LOCAL_STATE_SLF) &&
        /* fbe_provision_drive_is_peer_clustered_flag_set(provision_drive_p, FBE_PROVISION_DRIVE_CLUSTERED_FLAG_SLF) && */
        (local_state != FBE_LIFECYCLE_STATE_FAIL) && (local_state != FBE_LIFECYCLE_STATE_PENDING_FAIL) &&
        (peer_state == FBE_LIFECYCLE_STATE_FAIL))  
    {
        fbe_base_object_trace((fbe_base_object_t*)provision_drive_p, 
                              FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s peer failed during SLF, go to fail\n", __FUNCTION__);
        fbe_lifecycle_set_cond(&fbe_provision_drive_lifecycle_const,
                               (fbe_base_object_t*)provision_drive_p,
                               FBE_BASE_CONFIG_LIFECYCLE_COND_DOWNSTREAM_HEALTH_BROKEN);
    }

    /* If the peer requested EVAL SLF, and we have not acknowledged this yet, 
     * we need to set the EVAL SLF condition. 
     */
    if (fbe_provision_drive_is_peer_clustered_flag_set(provision_drive_p, FBE_PROVISION_DRIVE_CLUSTERED_FLAG_EVAL_SLF_REQUEST) &&
        !fbe_provision_drive_is_clustered_flag_set(provision_drive_p, FBE_PROVISION_DRIVE_CLUSTERED_FLAG_EVAL_SLF_REQUEST) &&
        fbe_base_config_is_clustered_flag_set((fbe_base_config_t *)provision_drive_p, FBE_BASE_CONFIG_CLUSTERED_FLAG_JOINED) )
        
    {
        fbe_lifecycle_state_t lifecycle_state;
        fbe_base_object_get_lifecycle_state((fbe_base_object_t *)provision_drive_p, &lifecycle_state);
        fbe_base_object_trace((fbe_base_object_t*)provision_drive_p, 
                              FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s EVAL SLF requested from peer state: 0x%llx\n", __FUNCTION__, (unsigned long long)lifecycle_state);
        fbe_lifecycle_set_cond(&fbe_provision_drive_lifecycle_const,
                               (fbe_base_object_t *) provision_drive_p,
                               FBE_PROVISION_DRIVE_LIFECYCLE_COND_EVAL_SLF);
    }
	
	if (fbe_provision_drive_is_peer_clustered_flag_set(provision_drive_p, FBE_PROVISION_DRIVE_CLUSTERED_FLAG_ZERO_STARTED) &&
		!fbe_provision_drive_is_local_state_set(provision_drive_p, FBE_PVD_LOCAL_STATE_ZERO_STARTED)){

		if (fbe_provision_drive_is_local_state_set(provision_drive_p, FBE_PVD_LOCAL_STATE_ZERO_ENDED)) {
			fbe_provision_drive_clear_local_state(provision_drive_p, FBE_PVD_LOCAL_STATE_ZERO_ENDED);
		}

		fbe_provision_drive_set_local_state(provision_drive_p, FBE_PVD_LOCAL_STATE_ZERO_STARTED);

        fbe_lifecycle_set_cond(&fbe_provision_drive_lifecycle_const,
                               (fbe_base_object_t *) provision_drive_p,
                               FBE_PROVISION_DRIVE_LIFECYCLE_COND_ZERO_STARTED);

		fbe_provision_drive_unlock(provision_drive_p);

		return FBE_STATUS_OK;
    }

	if (fbe_provision_drive_is_peer_clustered_flag_set(provision_drive_p, FBE_PROVISION_DRIVE_CLUSTERED_FLAG_ZERO_ENDED) &&
		!fbe_provision_drive_is_local_state_set(provision_drive_p, FBE_PVD_LOCAL_STATE_ZERO_ENDED)){

		if (fbe_provision_drive_is_local_state_set(provision_drive_p, FBE_PVD_LOCAL_STATE_ZERO_STARTED)) {
			fbe_provision_drive_clear_local_state(provision_drive_p, FBE_PVD_LOCAL_STATE_ZERO_STARTED);
		}

		fbe_provision_drive_set_local_state(provision_drive_p, FBE_PVD_LOCAL_STATE_ZERO_ENDED);

        fbe_lifecycle_set_cond(&fbe_provision_drive_lifecycle_const,
                               (fbe_base_object_t *) provision_drive_p,
                               FBE_PROVISION_DRIVE_LIFECYCLE_COND_ZERO_ENDED);

		fbe_provision_drive_unlock(provision_drive_p);

		return FBE_STATUS_OK;
		
    }

    if (fbe_provision_drive_is_peer_clustered_flag_set(provision_drive_p, FBE_PROVISION_DRIVE_CLUSTERED_FLAG_SCRUB_ENDED) &&
		!fbe_provision_drive_is_local_state_set(provision_drive_p, FBE_PVD_LOCAL_STATE_SCRUB_ENDED)){

		if (fbe_provision_drive_is_local_state_set(provision_drive_p, FBE_PVD_LOCAL_STATE_ZERO_STARTED)) {
			fbe_provision_drive_clear_local_state(provision_drive_p, FBE_PVD_LOCAL_STATE_ZERO_STARTED);
		}

		fbe_provision_drive_set_local_state(provision_drive_p, FBE_PVD_LOCAL_STATE_SCRUB_ENDED);

        fbe_lifecycle_set_cond(&fbe_provision_drive_lifecycle_const,
                               (fbe_base_object_t *) provision_drive_p,
                               FBE_PROVISION_DRIVE_LIFECYCLE_COND_ZERO_ENDED);

		fbe_provision_drive_unlock(provision_drive_p);

		return FBE_STATUS_OK;
		
    }

	fbe_provision_drive_unlock(provision_drive_p);

	return FBE_STATUS_OK;
    
}
/*****************************************************
 * end fbe_provision_drive_handle_peer_memory_update()
 *****************************************************/

/*!**************************************************************
 * fbe_provision_drive_send_checkpoint_notification()
 ****************************************************************
 * @brief
 *  Handle a notification that the peer moved the checkpoint.
 *  We will check to see what operation is ongoing and
 *  send the notification for this.
 *
 * @param provision_drive_p - ptr to PVD.               
 *
 * @return fbe_status_t
 *
 * @author
 *  3/20/2012 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t 
fbe_provision_drive_send_checkpoint_notification(fbe_provision_drive_t * provision_drive_p)
{
    fbe_status_t status;
    fbe_lba_t zero_chkpt;
    fbe_u32_t zero_percent;
    fbe_notification_info_t notify_info;
    fbe_lba_t capacity;
    fbe_object_id_t object_id;

    /* Simply get the zero checkpoint and compare it to capacity to see the zero percentage. */
    fbe_provision_drive_metadata_get_background_zero_checkpoint(provision_drive_p, &zero_chkpt);
    fbe_base_config_get_capacity((fbe_base_config_t *)provision_drive_p, &capacity);

    if ((zero_chkpt == FBE_LBA_INVALID) || (zero_chkpt > capacity)) {
        zero_percent = 100;
    } else {
        zero_percent = (fbe_u32_t)((zero_chkpt * 100)/capacity);
    }

	/*we don't want to flood user space with too many notifications every two minutes, 
	we'll do it only when there was some change. This is critical when there are many drives
	To reduce even more, we'll pop up notifications only on even progress */
	if (provision_drive_p->last_zero_percent_notification != zero_percent && ((zero_percent % 2) == 0)){
		provision_drive_p->last_zero_percent_notification = zero_percent;
		notify_info.notification_type = FBE_NOTIFICATION_TYPE_ZEROING;
		notify_info.class_id = FBE_CLASS_ID_PROVISION_DRIVE;
		notify_info.object_type = FBE_TOPOLOGY_OBJECT_TYPE_PROVISIONED_DRIVE;
		notify_info.notification_data.object_zero_notification.zero_operation_status = FBE_OBJECT_ZERO_IN_PROGRESS;
		notify_info.notification_data.object_zero_notification.zero_operation_progress_percent = zero_percent;
		fbe_base_object_trace((fbe_base_object_t*)provision_drive_p, 
							  FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
							  "BG_ZERO: Peer chkpt update. Percent zeroed: %d chkpt: 0x%llx\n", zero_percent, (unsigned long long)zero_chkpt);
		fbe_base_object_get_object_id((fbe_base_object_t*)provision_drive_p, &object_id);

		status = fbe_notification_send(object_id, notify_info);
		if (status != FBE_STATUS_OK) 
		{ 
			fbe_base_object_trace((fbe_base_object_t*)provision_drive_p, 
								  FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO, 
								  "%s unexpected status: %d on notification send \n", __FUNCTION__, status);
		}
	}

	return FBE_STATUS_OK;
}
/*****************************************************
 * end fbe_provision_drive_send_checkpoint_notification()
 *****************************************************/

/*!**************************************************************
 * fbe_provision_drive_event_peer_contact_lost()
 ****************************************************************
 * @brief
 *  This function processes peer contact lost event for PVD.
 *
 * @param provision_drive_p - Pointer to provision drive.
 * @param event_context - The context for the event.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  05/07/2012 - Created. Lili Chen
 *
 ****************************************************************/
static fbe_status_t
fbe_provision_drive_event_peer_contact_lost(fbe_provision_drive_t * provision_drive_p, 
                                            fbe_event_context_t event_context)
{
    fbe_status_t    status = FBE_STATUS_OK;
    fbe_lifecycle_state_t lifecycle_state;
    fbe_bool_t b_is_system_drive;
    fbe_object_id_t object_id;
    
    if (fbe_provision_drive_is_clustered_flag_set(provision_drive_p, 
                                                  FBE_PROVISION_DRIVE_CLUSTERED_FLAG_SLF))
    {
        status = fbe_lifecycle_set_cond(&fbe_provision_drive_lifecycle_const,
                                        (fbe_base_object_t*)provision_drive_p,
                                        FBE_BASE_CONFIG_LIFECYCLE_COND_DOWNSTREAM_HEALTH_BROKEN);
    }

    /* AR573531: set non-paged drive info in case active side hasn't set it yet. */
    fbe_lifecycle_get_state(&fbe_provision_drive_lifecycle_const, (fbe_base_object_t *)provision_drive_p, &lifecycle_state);
    if ((lifecycle_state == FBE_LIFECYCLE_STATE_ACTIVATE) && 
        !fbe_base_config_is_active((fbe_base_config_t *)provision_drive_p))
    {
        status = fbe_lifecycle_set_cond(&fbe_provision_drive_lifecycle_const,
                                        (fbe_base_object_t*)provision_drive_p,
                                        FBE_PROVISION_DRIVE_LIFECYCLE_COND_UPDATE_PHYSICAL_DRIVE_INFO);
    }

    fbe_base_object_get_object_id((fbe_base_object_t*)provision_drive_p, &object_id);
    b_is_system_drive = fbe_database_is_object_system_pvd(object_id);

    if (fbe_base_config_is_rekey_mode((fbe_base_config_t *)provision_drive_p) &&
        !fbe_provision_drive_metadata_is_np_flag_set(provision_drive_p, 
                                                     FBE_PROVISION_DRIVE_NP_FLAG_PAGED_VALID) &&
        fbe_provision_drive_metadata_is_np_flag_set(provision_drive_p, 
                                                    FBE_PROVISION_DRIVE_NP_FLAG_PAGED_NEEDS_CONSUMED) &&
        !b_is_system_drive)
    {
        fbe_provision_drive_np_flags_t flags;
        fbe_provision_drive_metadata_get_np_flag(provision_drive_p, &flags); 
        fbe_base_object_trace((fbe_base_object_t*)provision_drive_p, FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO, 
                              "provision_drive: Peer contact lost enc mode flags: 0x%x\n", flags);
        status = fbe_lifecycle_set_cond(&fbe_provision_drive_lifecycle_const,
                                        (fbe_base_object_t*)provision_drive_p,
                                        FBE_PROVISION_DRIVE_LIFECYCLE_COND_PAGED_RECONSTRUCT_READY);
    }
    return status;
}
/*****************************************************
 * end fbe_provision_drive_event_peer_contact_lost()
 *****************************************************/

/*!**************************************************************
 * fbe_provision_drive_handle_peer_drive_fault()
 ****************************************************************
 * @brief
 *  This function handles drive_falut clustered flag changes
 *  from peer.
 *
 * @param provision_drive_p - Pointer to provision drive.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  07/11/2012 - Created. Lili Chen
 *
 ****************************************************************/
static fbe_status_t
fbe_provision_drive_handle_peer_drive_fault(fbe_provision_drive_t * provision_drive_p)
{
    fbe_lifecycle_state_t   local_state, peer_state;
    fbe_bool_t b_peer_flag_set, b_local_flag_set;
    fbe_status_t status = FBE_STATUS_OK;

    /* Check clear drive fault */
    if (fbe_provision_drive_is_peer_clustered_flag_set(provision_drive_p, FBE_PROVISION_DRIVE_CLUSTERED_FLAG_CLEAR_DRIVE_FAULT))
    {
        status = fbe_lifecycle_set_cond(&fbe_provision_drive_lifecycle_const,
                                        (fbe_base_object_t*)provision_drive_p,
                                        FBE_BASE_CONFIG_LIFECYCLE_COND_CLEAR_DRIVE_FAULT);
        return FBE_STATUS_OK;
    }

    /* Check drive fault */
    fbe_base_config_metadata_get_peer_lifecycle_state((fbe_base_config_t *)provision_drive_p, &peer_state);
    fbe_base_config_metadata_get_lifecycle_state((fbe_base_config_t *)provision_drive_p, &local_state);
    b_local_flag_set = fbe_provision_drive_is_clustered_flag_set(provision_drive_p, 
                                                                 FBE_PROVISION_DRIVE_CLUSTERED_FLAG_DRIVE_FAULT);
    b_peer_flag_set = fbe_provision_drive_is_peer_clustered_flag_set(provision_drive_p, 
                                                                     FBE_PROVISION_DRIVE_CLUSTERED_FLAG_DRIVE_FAULT);

    if (b_peer_flag_set)
    {
        switch (peer_state)
        {
            case FBE_LIFECYCLE_STATE_PENDING_ACTIVATE:
            case FBE_LIFECYCLE_STATE_ACTIVATE:
                status = fbe_lifecycle_set_cond(&fbe_provision_drive_lifecycle_const,
                                                (fbe_base_object_t*)provision_drive_p,
                                                FBE_BASE_CONFIG_LIFECYCLE_COND_DOWNSTREAM_HEALTH_DISABLED);
                break;
            case FBE_LIFECYCLE_STATE_PENDING_FAIL:
            case FBE_LIFECYCLE_STATE_FAIL:
            case FBE_LIFECYCLE_STATE_PENDING_DESTROY:
            case FBE_LIFECYCLE_STATE_DESTROY:
                status = fbe_lifecycle_set_cond(&fbe_provision_drive_lifecycle_const,
                                                (fbe_base_object_t*)provision_drive_p,
                                                FBE_BASE_CONFIG_LIFECYCLE_COND_SET_DRIVE_FAULT);
                break;
            default:
                fbe_base_object_trace((fbe_base_object_t*)provision_drive_p, 
                                      FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO, 
                                      "%s unexpected state: local 0x%x peer 0x%x \n", __FUNCTION__, local_state, peer_state);
                break;
        }
    }

    return FBE_STATUS_OK;
}
/*****************************************************
 * end fbe_provision_drive_handle_peer_drive_fault()
 *****************************************************/

/*************************************
 * end fbe_provision_drive_event.c
 *************************************/
