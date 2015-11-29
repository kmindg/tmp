#include "fbe_virtual_drive_private.h"
#include "fbe_raid_group_rebuild.h"

/* Forward declarations */
static fbe_status_t fbe_virtual_drive_data_request_event_entry(fbe_virtual_drive_t* in_virtual_drive_p, fbe_event_context_t in_event_context);
static fbe_status_t fbe_virtual_drive_handle_remap_event(fbe_virtual_drive_t* in_virtual_drive_p, fbe_event_context_t in_event_context);
static fbe_status_t fbe_virtual_drive_state_change_event_entry(fbe_virtual_drive_t * virtual_drive_p, fbe_event_context_t event_context);
static fbe_status_t fbe_virtual_drive_permit_request_event_entry(fbe_virtual_drive_t* in_virtual_drive_p, fbe_event_context_t in_event_context);
static fbe_status_t fbe_virtual_drive_handle_verify_event(fbe_virtual_drive_t* in_virtual_drive_p, fbe_event_context_t in_event_context);
static fbe_status_t fbe_virtual_drive_handle_zero_event(fbe_virtual_drive_t* in_virtual_drive_p, fbe_event_context_t in_event_context);
static fbe_status_t fbe_virtual_drive_handle_download_request(fbe_virtual_drive_t* in_virtual_drive_p, fbe_path_attr_t path_attr);
static fbe_status_t fbe_virtual_drive_handle_attribute_change(fbe_virtual_drive_t *virtual_drive_p, fbe_event_context_t event_context);
static fbe_bool_t fbe_virtual_drive_check_medic_action_priority(fbe_virtual_drive_t* virtual_drive_p, fbe_event_t * event_p);
static fbe_status_t fbe_virtual_drive_handle_verify_invalidate_event(fbe_virtual_drive_t* in_virtual_drive_p, fbe_event_context_t in_event_context);
static fbe_status_t fbe_virtual_drive_download_request_event_entry(fbe_virtual_drive_t* in_virtual_drive_p, fbe_event_context_t  in_event_context);
static fbe_status_t fbe_virtual_drive_send_checkpoint_notification(fbe_virtual_drive_t *virtual_drive_p);
static fbe_status_t fbe_virtual_drive_peer_lost_event_entry(fbe_virtual_drive_t *virtual_drive_p);
static fbe_status_t fbe_virtual_drive_handle_attribute_change_for_slf(fbe_virtual_drive_t *virtual_drive_p, fbe_block_edge_t *edge_p);

/*!***************************************************************
 * fbe_virtual_drive_event_entry()
 ****************************************************************
 * @brief
 *  This function is called to pass an event to a given instance
 *  of the virtual_drive class.
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
fbe_status_t fbe_virtual_drive_event_entry(fbe_object_handle_t object_handle, 
                                           fbe_event_type_t event_type,
                                           fbe_event_context_t event_context)
{
    fbe_virtual_drive_t * virtual_drive_p = NULL;
    fbe_status_t status;

    virtual_drive_p = (fbe_virtual_drive_t *)fbe_base_handle_to_pointer(object_handle);

    fbe_base_object_trace((fbe_base_object_t*)virtual_drive_p,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry event_type %d context 0x%p\n",
                          __FUNCTION__, event_type, event_context);

    /* First handle the event we have received.
     */
    switch (event_type) 
    {
        case FBE_EVENT_TYPE_EDGE_STATE_CHANGE:
            status = fbe_virtual_drive_state_change_event_entry(virtual_drive_p, event_context);
            break;
        case FBE_EVENT_TYPE_DATA_REQUEST:
            status = fbe_virtual_drive_data_request_event_entry(virtual_drive_p, event_context);
            break;        
        case FBE_EVENT_TYPE_PERMIT_REQUEST:
            status = fbe_virtual_drive_permit_request_event_entry(virtual_drive_p, event_context);
            break;
        case FBE_EVENT_TYPE_ATTRIBUTE_CHANGED:
            status = fbe_virtual_drive_handle_attribute_change(virtual_drive_p, event_context);
            break;
        case FBE_EVENT_TYPE_DOWNLOAD_REQUEST:
            status = fbe_virtual_drive_download_request_event_entry(virtual_drive_p, event_context);
            break; 
        case FBE_EVENT_TYPE_PEER_NONPAGED_CHKPT_UPDATE:
            status = fbe_virtual_drive_send_checkpoint_notification(virtual_drive_p);
            break;
        case FBE_EVENT_TYPE_PEER_CONTACT_LOST:
            status = fbe_virtual_drive_peer_lost_event_entry(virtual_drive_p);
            /* We also need to call raid_group event handler since it will have more work to do. */
            status = fbe_raid_group_event_entry(object_handle, event_type, event_context);
            break;
        default:
            status = fbe_mirror_event_entry(object_handle, event_type, event_context);
            break;
    }

    return status;
}
/*****************************************
 * end fbe_virtual_drive_event_entry()
 *****************************************/
/*!***************************************************************
 * fbe_virtual_drive_event_entry()
 ****************************************************************
 * @brief
 *  This function is called to pass an event to a given instance
 *  of the virtual_drive class.
 *
 * @param virtual_drive_p - The virtual drive object.
 * @param event_context - the ptr to the context that arrived with the event.
 *
 * @return
 *  fbe_status_t
 *
 * @author
 *  11/23/2011 - Created. Rob Foley
 *
 ****************************************************************/

static fbe_status_t fbe_virtual_drive_handle_attribute_change(fbe_virtual_drive_t *virtual_drive_p, 
                                                              fbe_event_context_t event_context)
{
    fbe_status_t                                status;
    fbe_block_edge_t                           *edge_p = (fbe_block_edge_t *)event_context;
    fbe_path_attr_t                             path_attr = 0;
	fbe_base_config_downstream_health_state_t   downstream_health_state;

    /* If there is no edge something when wrong.
     */
    if (edge_p == NULL)
    {
        /* Trace the error.
         */
        fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s event context is NULL \n",
                              __FUNCTION__);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Fetch the path attributes of the edge. 
     */
    status = fbe_block_transport_get_path_attributes(edge_p, &path_attr);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p, 
                              FBE_TRACE_LEVEL_ERROR, 
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                              "%s cannot get block edge path attr - status:0x%x \n", 
                              __FUNCTION__, status);
        return status;
    }

    /* Trace some information if enabled.
     */
    fbe_virtual_drive_trace(virtual_drive_p,
                            FBE_TRACE_LEVEL_INFO,
                            FBE_VIRTUAL_DRIVE_DEBUG_FLAG_RL_TRACING,
                            "virtual_drive: attribute change index: %d attr: 0x%x \n",
                            edge_p->base_edge.client_index, path_attr);

    /* We purposefully ignore the status.
     */
    fbe_virtual_drive_handle_download_request(virtual_drive_p,
                                              (path_attr & FBE_BLOCK_PATH_ATTR_FLAGS_CLIENT_DOWNLOAD_MASK));

    /* If timeout errors propagate upstream.
     */
    if (path_attr & FBE_BLOCK_PATH_ATTR_FLAGS_TIMEOUT_ERRORS)
    {
        /* Update the timeout errors attributes on the edge with upstream object.
         */
        fbe_block_transport_server_set_path_attr_all_servers(&virtual_drive_p->spare.raid_group.base_config.block_transport_server,
                                                             FBE_BLOCK_PATH_ATTR_FLAGS_TIMEOUT_ERRORS);
    }

    /* If key is required propagate upstream.
     */
    if (path_attr & FBE_BLOCK_PATH_ATTR_FLAGS_KEYS_REQUIRED)
    {
        fbe_base_config_encryption_state_t state;
        fbe_base_config_get_encryption_state((fbe_base_config_t *)virtual_drive_p, &state);
        if ((state != FBE_BASE_CONFIG_ENCRYPTION_STATE_SWAP_OUT_OLD_KEY_NEED_REMOVED) &&
            (state != FBE_BASE_CONFIG_ENCRYPTION_STATE_SWAP_IN_NEW_KEY_REQUIRED))
        {
            /* Update the key required attributes on the edge with upstream object.
             */
            fbe_block_transport_server_set_path_attr_all_servers(&virtual_drive_p->spare.raid_group.base_config.block_transport_server,
                                                                 FBE_BLOCK_PATH_ATTR_FLAGS_KEYS_REQUIRED);
        }
    }

    /* Handle attr change for medic priority and slf.
     */
    fbe_virtual_drive_attribute_changed(virtual_drive_p, edge_p);

    /* Determine the health of the path with downstream objects. */
    downstream_health_state = fbe_virtual_drive_verify_downstream_health(virtual_drive_p);

    /* Set the specific lifecycle condition based on health of the path with downstream objects. */
    status = fbe_virtual_drive_set_condition_based_on_downstream_health(virtual_drive_p, downstream_health_state, __FUNCTION__);
    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_virtual_drive_handle_attribute_change()
 **************************************/
/*!***************************************************************
 * fbe_virtual_drive_data_request_event_entry
 *****************************************************************
 * @brief
 *    This function handles "data request"  event  processing  for
 *    virtual drive objects.  This function determines the type of
 *    data request and initiates the appropriate action to process
 *    it.
 *
 * @param   in_virtual_drive_p  -  pointer to virtual drive
 * @param   in_event_context    -  context associated with event
 *
 * @return  fbe_status_t        -  event processing status
 *
 * @author
 *    07/09/2010 - Created. Randy Black
 *
 ****************************************************************/

fbe_status_t
fbe_virtual_drive_data_request_event_entry( fbe_virtual_drive_t* in_virtual_drive_p,
                                            fbe_event_context_t  in_event_context    )
{
    fbe_data_event_type_t           data_event_type;    // type of data event
    fbe_event_data_request_t        data_request;       // data request
    fbe_event_t*                    event_p;            // pointer to event
    fbe_status_t                    status;             // fbe status

    // cast context to event object
    event_p = (fbe_event_t *) in_event_context;

    // get data event type in data request
    fbe_event_get_data_request_data( event_p, &data_request );
    data_event_type = data_request.data_event_type;

    // dispatch on data event type
    switch ( data_event_type )
    {
        // remap data event type
        case FBE_DATA_EVENT_TYPE_REMAP:
            status = fbe_virtual_drive_handle_remap_event( in_virtual_drive_p, in_event_context );
            break;

        // unsupported data event type
        default:

            // set generic failure status 
            status = FBE_STATUS_GENERIC_FAILURE;

            // trace error in data event type
            fbe_base_object_trace( (fbe_base_object_t *) in_virtual_drive_p,
                                   FBE_TRACE_LEVEL_ERROR,
                                   FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                   "%s unsupported data event type: 0x%x\n",
                                   __FUNCTION__, data_event_type );

            // set failure status in event and complete event
            fbe_event_set_status( event_p, FBE_EVENT_STATUS_GENERIC_FAILURE );
            fbe_event_complete( event_p );
            break;
    }

    return status;
}   // end fbe_virtual_drive_data_request_event_entry()


/*!***************************************************************
 * fbe_virtual_drive_handle_remap_event
 *****************************************************************
 * @brief
 *    ** tbs **
 *
 * @param   in_virtual_drive_p  -  pointer to virtual drive
 * @param   in_event_context    -  context associated with event
 *
 * @return  fbe_status_t        -  remap event processing status
 *
 * @author
 *    07/09/2010 - Created. Randy Black
 *
 ****************************************************************/

fbe_status_t
fbe_virtual_drive_handle_remap_event( fbe_virtual_drive_t* in_virtual_drive_p,
                                      fbe_event_context_t  in_event_context    )
{
    fbe_block_count_t               block_count;        // block count of i/o range to remap
    fbe_event_t*                    event_p;            // pointer to event
    fbe_event_stack_t*              event_stack_p;      // event stack pointer
    fbe_lba_t                       external_capacity;  // capacity
    fbe_lba_t                       lba;                // starting lba of i/o range to remap

    // cast context to event object
    event_p = (fbe_event_t *)in_event_context;

    // get current entry on event stack
    event_stack_p = fbe_event_get_current_stack( event_p );

    // get starting lba and block count of i/o range to remap
    fbe_event_stack_get_extent( event_stack_p, &lba, &block_count );

    // get the external capacity of virtual drive which describes the
    // area on the disk available for user data;  it does not include
    // any storage reserved for virtual drive's metadata or signature
    fbe_base_config_get_capacity( (fbe_base_config_t*)in_virtual_drive_p, &external_capacity );

    // if the i/o range to remap falls within  the  virtual drive's
    // metadata or signature, virtual drive always remaps  the  i/o
    // range itself; return FBE_EVENT_STATUS_OK so  that  provision
    // drive doesn't take any further action to remap the i/o range
    if (lba >= external_capacity)
    {
        // set status in event and complete event
        fbe_event_set_status( event_p, FBE_EVENT_STATUS_OK );
        fbe_event_complete( event_p );

        return FBE_STATUS_OK;
    }

    // send "data request" event to ask for remap action from upstream object
    fbe_base_config_send_event((fbe_base_config_t *)in_virtual_drive_p, FBE_EVENT_TYPE_DATA_REQUEST, event_p );

    return FBE_STATUS_OK;
}   // end fbe_virtual_drive_handle_remap_event()


/*!**************************************************************
 * fbe_virtual_drive_state_change_event_entry()
 ****************************************************************
 * @brief
 *  This is the handler function for a change in the edge state between 
 *  Provision Virtual Drive object and Logical drive object.
 *
 * @param virtual_drive_p - virtual drive object which has received event.
 * @param event_context - The context for the event.
 *
 * @return - Status of the handling.
 *
 * @author
 *  8/9/2009 - Created. Dhaval Patel.
 *
 ****************************************************************/
static fbe_status_t
fbe_virtual_drive_state_change_event_entry(fbe_virtual_drive_t * virtual_drive_p,
                                           fbe_event_context_t event_context)
{
    fbe_status_t                                status = FBE_STATUS_OK;
    fbe_base_config_downstream_health_state_t   downstream_health_state;
    fbe_edge_index_t                            path_state_changed_edge_index = FBE_EDGE_INDEX_INVALID;
    fbe_path_state_t                            path_state;
    fbe_base_edge_t                            *base_edge_p = NULL;
    fbe_bool_t                                  b_is_metadata_initialized = FBE_TRUE;

    /* Get the edge from the event context and verify that it is not NULL. */
    base_edge_p = (fbe_base_edge_t *)event_context;
    if (base_edge_p == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t*)virtual_drive_p, 
                              FBE_TRACE_LEVEL_ERROR, 
                              FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                              "%s Invalid event context\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    path_state_changed_edge_index = base_edge_p->client_index;

    /* Fetch the path state of the edge. */ 
    status = fbe_block_transport_get_path_state(&(((fbe_base_config_t *)virtual_drive_p)->block_edge_p[base_edge_p->client_index]),
                                                &path_state);
    if (status != FBE_STATUS_OK)
    {
        /* Not able to get the path state. */
        fbe_base_object_trace((fbe_base_object_t*)virtual_drive_p, 
                              FBE_TRACE_LEVEL_ERROR, 
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                              "%s cannot get block edge path state 0x%x \n",
                              __FUNCTION__, status);
        return status;
    }

    /* Only process the event if the metadata has been initialized.
     */
    fbe_base_config_metadata_is_initialized((fbe_base_config_t *)virtual_drive_p, &b_is_metadata_initialized);
    if (b_is_metadata_initialized == FBE_FALSE)
    {
        fbe_base_object_trace((fbe_base_object_t*)virtual_drive_p, 
                              FBE_TRACE_LEVEL_INFO, 
                              FBE_TRACE_MESSAGE_ID_INFO, 
                              "%s edge index: %d path state: %d metadata not inited\n", 
                              __FUNCTION__, path_state_changed_edge_index, path_state);

        /* we need to kick the object after the edge is ready */
        fbe_lifecycle_reschedule(&fbe_virtual_drive_lifecycle_const,
                                (fbe_base_object_t *)virtual_drive_p,
                                (fbe_lifecycle_timer_msec_t) 0);    /* Immediate reschedule */	
        return FBE_STATUS_OK;
    }

    /* Validate the path state.
     */
    switch (path_state)
    {
        case FBE_PATH_STATE_ENABLED:
            /* The `need replacement' condition will take care of clearing the
             * `need replacement drive' condition as needed.
             */
            break;
        case FBE_PATH_STATE_DISABLED:
            /* Disabled is a transitory state.  Nothing to do.
             */
            break;
        case FBE_PATH_STATE_BROKEN:
            /* Set condition based on downstream health will take any actions required.
             */
            break;
        case FBE_PATH_STATE_GONE:
        case FBE_PATH_STATE_INVALID:            
            break;
        case FBE_PATH_STATE_SLUMBER:
            return FBE_STATUS_OK;/*if the drive below us went to sleep, we are fine with that, no need to do anything*/
            break;
        default:
            /* This is a path state we do not expect. */
            status = FBE_STATUS_GENERIC_FAILURE;
            fbe_base_object_trace((fbe_base_object_t*)virtual_drive_p, 
                                  FBE_TRACE_LEVEL_ERROR, 
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                                  "%s index: %d invalid path state: %d\n", 
                                  __FUNCTION__, path_state_changed_edge_index, path_state);
            return status;
    }

    /* Determine the health of the path with downstream objects. 
     */
    downstream_health_state = fbe_virtual_drive_verify_downstream_health(virtual_drive_p);

    /* Trace the edge state change.
     */
	fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p, 
                          FBE_TRACE_LEVEL_INFO, 
                          FBE_TRACE_MESSAGE_ID_INFO, 
                          "%s: health: %d edge index %d, path state %d\n", 
                          __FUNCTION__, downstream_health_state, path_state_changed_edge_index, base_edge_p->path_state);

    /* Perform any NR/rebuild processing needed based on the new edge state.
     */
    fbe_virtual_drive_copy_handle_processing_on_edge_state_change(virtual_drive_p,
                                                                  path_state_changed_edge_index,
                                                                  path_state,
                                                                  downstream_health_state);

    /* Set the specific lifecycle condition based on health of the path with 
     * downstream objects. 
     */
    status = fbe_virtual_drive_set_condition_based_on_downstream_health(virtual_drive_p, downstream_health_state, __FUNCTION__);

    return status;
}
/****************************************************
 * end fbe_virtual_drive_state_change_event_entry()
 ****************************************************/

/*!***************************************************************
 * fbe_virtual_drive_permit_request_event_entry
 *****************************************************************
 * @brief
 *    This function handles "permit request"  event  processing  for
 *    virtual drive objects.  This function determines the type of
 *    permit request and initiates the appropriate action to process
 *    it.
 *
 * @param   in_virtual_drive_p  -  pointer to virtual drive
 * @param   in_event_context    -  context associated with event
 *
 * @return  fbe_status_t        -  event processing status
 *
 * @author
 *    12/14/2010 - Created. Maya Dagon
 *
 ****************************************************************/

fbe_status_t
fbe_virtual_drive_permit_request_event_entry( fbe_virtual_drive_t* in_virtual_drive_p,
                                            fbe_event_context_t  in_event_context    )
{
    fbe_permit_event_type_t         permit_event_type;    // type of permit event
    fbe_event_permit_request_t      permit_request;       // permit request
    fbe_event_t*                    event_p;              // pointer to event
    fbe_status_t                    status;               // fbe status

    // cast context to event object
    event_p = (fbe_event_t *) in_event_context;

    // get permit event type in permit request
    fbe_event_get_permit_request_data( event_p, &permit_request );
    permit_event_type = permit_request.permit_event_type;

    //  Check medic action priority first 
    if (!fbe_virtual_drive_check_medic_action_priority(in_virtual_drive_p, event_p))
    {
        fbe_event_set_flags(event_p, FBE_EVENT_FLAG_DENY);
        fbe_event_set_status(event_p, FBE_EVENT_STATUS_OK);
        fbe_event_complete(event_p);
        return FBE_STATUS_OK;
    }

    // dispatch on permit event type
    switch ( permit_event_type )
    {
        // verify permit event type
	    case FBE_PERMIT_EVENT_TYPE_VERIFY_REQUEST:
            status = fbe_virtual_drive_handle_verify_event( in_virtual_drive_p, in_event_context );
            break;

		// zero permit event type
	    case FBE_PERMIT_EVENT_TYPE_ZERO_REQUEST :
			status = fbe_virtual_drive_handle_zero_event( in_virtual_drive_p, in_event_context );
            break;

        case FBE_PERMIT_EVENT_TYPE_VERIFY_INVALIDATE_REQUEST:
            status = fbe_virtual_drive_handle_verify_invalidate_event( in_virtual_drive_p, in_event_context );
            break;

        // unsupported permit event type
        default:

            // set generic failure status 
            status = FBE_STATUS_GENERIC_FAILURE;

            // trace error in permit event type
            fbe_base_object_trace( (fbe_base_object_t *) in_virtual_drive_p,
                                   FBE_TRACE_LEVEL_ERROR,
                                   FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                   "%s unsupported permit event type: 0x%x\n",
                                   __FUNCTION__, permit_event_type );

            // set failure status in event and complete event
            fbe_event_set_status( event_p, FBE_EVENT_STATUS_GENERIC_FAILURE );
            fbe_event_complete( event_p );
            break;
    }

    return status;
}   // end fbe_virtual_drive_permit_request_event_entry()


/*!***************************************************************
 * fbe_virtual_drive_handle_verify_event
 *****************************************************************
 * @brief
 *  	 This function handles verify permission request event.
 *
 * @param   in_virtual_drive_p  -  pointer to virtual drive
 * @param   in_event_context    -  context associated with event
 *
 * @return  fbe_status_t        -  verify event processing status
 *
 * @author
 *    12/15/2010 - Created. Maya Dagon
 *
 ****************************************************************/

fbe_status_t
fbe_virtual_drive_handle_verify_event( fbe_virtual_drive_t* in_virtual_drive_p,
                                       fbe_event_context_t  in_event_context    )
{
    fbe_block_count_t               block_count;        // block count of i/o range to verify
    fbe_event_t*                    event_p;            // pointer to event
    fbe_event_stack_t*              event_stack_p;      // event stack pointer
    fbe_lba_t                       external_capacity;  // capacity
    fbe_lba_t                       lba;                // starting lba of i/o range to verify
    fbe_lba_t                       next_consumed_lba;

    // cast context to event object
    event_p = (fbe_event_t *)in_event_context;

    // get current entry on event stack
    event_stack_p = fbe_event_get_current_stack( event_p );

    // get starting lba and block count of i/o range to verify
    fbe_event_stack_get_extent( event_stack_p, &lba, &block_count );

    // get the external capacity of virtual drive which describes the
    // area on the disk available for user data;  it does not include
    // any storage reserved for virtual drive's metadata or signature
    fbe_base_config_get_capacity( (fbe_base_config_t*)in_virtual_drive_p, &external_capacity );

    // if the i/o range to verify falls within  the  virtual drive's
    // metadata or signature, virtual drive always verifies  the  i/o
    // range itself; return FBE_EVENT_STATUS_OK so  that  provision
    // drive doesn't take any further action to verify the i/o range
    if (lba >= external_capacity)
    {
        // set status in event and complete event
        fbe_event_set_status( event_p, FBE_EVENT_STATUS_OK );
        fbe_event_complete( event_p );

        return FBE_STATUS_OK;
    }

    fbe_block_transport_server_find_next_consumed_lba(&in_virtual_drive_p->spare.raid_group.base_config.block_transport_server,
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

    /* We have already check priority.  If we are here the request was higher
     * priority than the copy operation.
     */

    // send "permit request" event to ask for verify action from upstream object
    fbe_base_config_send_event((fbe_base_config_t *)in_virtual_drive_p,FBE_EVENT_TYPE_PERMIT_REQUEST, event_p );

    return FBE_STATUS_OK;
}   // end fbe_virtual_drive_handle_verify_event()


/*!***************************************************************
 * fbe_virtual_drive_handle_zero_event
 *****************************************************************
 * @brief
 *  	 This function handles zero permission request event.
 *
 * @param   in_virtual_drive_p  -  pointer to virtual drive
 * @param   in_event_context    -  context associated with event
 *
 * @return  fbe_status_t        -  verify event processing status
 *
 * @author
 *    12/21/2010 - Created. Maya Dagon
 *
 ****************************************************************/

fbe_status_t
fbe_virtual_drive_handle_zero_event( fbe_virtual_drive_t* in_virtual_drive_p,
                                       fbe_event_context_t  in_event_context    )
{
    fbe_block_count_t               block_count;        // block count of i/o range to zero
    fbe_event_t*                    event_p;            // pointer to event
    fbe_event_stack_t*              event_stack_p;      // event stack pointer
    fbe_lba_t                       external_capacity;  // capacity
    fbe_lba_t                       lba;                // starting lba of i/o range to zero

    // cast context to event object
    event_p = (fbe_event_t *)in_event_context;

    // get current entry on event stack
    event_stack_p = fbe_event_get_current_stack( event_p );

    // get starting lba and block count of i/o range to verify
    fbe_event_stack_get_extent( event_stack_p, &lba, &block_count );

    // get the external capacity of virtual drive which describes the
    // area on the disk available for user data;  it does not include
    // any storage reserved for virtual drive's metadata or signature
    fbe_base_config_get_capacity( (fbe_base_config_t*)in_virtual_drive_p, &external_capacity );

    // if the i/o range to zero falls within  the  virtual drive's
    // metadata or signature, virtual drive always zeros  the  i/o
    // range itself; return FBE_EVENT_STATUS_OK so  that  provision
    // drive doesn't take any further action to zero the i/o range
    if (lba >= external_capacity)
    {
        // set status in event and complete event
        fbe_event_set_status( event_p, FBE_EVENT_STATUS_OK );
        fbe_event_complete( event_p );

        return FBE_STATUS_OK;
    }

    // send "permit request" event to ask for zero action from upstream object
    fbe_base_config_send_event((fbe_base_config_t *)in_virtual_drive_p,FBE_EVENT_TYPE_PERMIT_REQUEST, event_p );

    return FBE_STATUS_OK;
}   // end fbe_virtual_drive_handle_zero_event()

/*!***************************************************************
 * fbe_virtual_drive_handle_verify_invalidate_event
 *****************************************************************
 * @brief
 *  	 This function handles verify invalidate permission request event.
 *
 * @param   in_virtual_drive_p  -  pointer to virtual drive
 * @param   in_event_context    -  context associated with event
 *
 * @return  fbe_status_t        -  verify invalidate event processing status
 *
 * @author
 *    04/09/2012 - Created. Ashok Tamilarasan
 *
 ****************************************************************/

static fbe_status_t
fbe_virtual_drive_handle_verify_invalidate_event( fbe_virtual_drive_t* in_virtual_drive_p,
                                                  fbe_event_context_t  in_event_context    )
{
    fbe_block_count_t               block_count;        // block count of i/o range
    fbe_event_t*                    event_p;            // pointer to event
    fbe_event_stack_t*              event_stack_p;      // event stack pointer
    fbe_lba_t                       external_capacity;  // capacity
    fbe_lba_t                       lba;                // starting lba of i/o range
    fbe_lba_t                       next_consumed_lba;

    // cast context to event object
    event_p = (fbe_event_t *)in_event_context;

    // get current entry on event stack
    event_stack_p = fbe_event_get_current_stack( event_p );

    // get starting lba and block count of i/o range
    fbe_event_stack_get_extent( event_stack_p, &lba, &block_count );

    // get the external capacity of virtual drive which describes the
    // area on the disk available for user data;  it does not include
    // any storage reserved for virtual drive's metadata or signature
    fbe_base_config_get_capacity( (fbe_base_config_t*)in_virtual_drive_p, &external_capacity );

    // if the i/o range to verify invalidate falls within  the  virtual drive's
    // metadata or signature, virtual drive always verifies  the  i/o
    // range itself; return FBE_EVENT_STATUS_OK so  that  provision
    // drive doesn't take any further action to verify invalidate the i/o range
    if (lba >= external_capacity)
    {
        // set status in event and complete event
        fbe_event_set_status( event_p, FBE_EVENT_STATUS_OK );
        fbe_event_complete( event_p );

        return FBE_STATUS_OK;
    }

    fbe_block_transport_server_find_next_consumed_lba(&in_virtual_drive_p->spare.raid_group.base_config.block_transport_server,
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

    /* We have already check priority.  If we are here the request was higher
     * priority than the copy operation.
     */

    // send "permit request" event to ask for verify invalidate action from upstream object
    fbe_base_config_send_event((fbe_base_config_t *)in_virtual_drive_p,FBE_EVENT_TYPE_PERMIT_REQUEST, event_p );

    return FBE_STATUS_OK;
}   // end fbe_virtual_drive_handle_verify_invalidate_event()

/*!***************************************************************
 * fbe_virtual_drive_handle_download_request
 *****************************************************************
 * @brief
 *    This function handles the request to allow drive firmware download on
 *    a raid group. We let upper level to determine path through mode, and
 *    disallow download if it is a swap-in.
 *
 * @param   in_virtual_drive_p  -  pointer to virtual drive
 * @param   download_attr - The path attributes related to download
 *
 * @return  fbe_status_t        -  event processing status
 *
 * @author
 *    04/30/2011  - Created. Ruomu Gao
 *
 ****************************************************************/
fbe_status_t fbe_virtual_drive_handle_download_request(fbe_virtual_drive_t *virtual_drive_p,
                                                       fbe_path_attr_t download_attr)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_virtual_drive_configuration_mode_t  configuration_mode;
    fbe_base_config_t                       *base_config_p = (fbe_base_config_t *)virtual_drive_p;
    fbe_block_transport_server_t            *block_transport_server_p;

    fbe_base_object_trace((fbe_base_object_t *) virtual_drive_p,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n", __FUNCTION__);

    /* Get the configuration mode of the virtual drive object */
    fbe_virtual_drive_get_configuration_mode(virtual_drive_p, &configuration_mode);
    switch(configuration_mode)
    {
        case FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_FIRST_EDGE:
        case FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_SECOND_EDGE:
            /* Let monitor condition to decide.
             */
            status = fbe_lifecycle_set_cond(&fbe_virtual_drive_lifecycle_const,
                                 (fbe_base_object_t *)virtual_drive_p,
                                 FBE_VIRTUAL_DRIVE_LIFECYCLE_COND_BACKGROUND_MONITOR_OPERATION);

            status = FBE_STATUS_GENERIC_FAILURE;
            break;

        case FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_FIRST_EDGE:
        case FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_SECOND_EDGE:
            block_transport_server_p = &base_config_p->block_transport_server;
            if (download_attr & FBE_BLOCK_PATH_ATTR_FLAGS_CLIENT_DOWNLOAD_REQ)
            {
                /* The upstream edge may already set GRANT attr */
                fbe_block_transport_server_propagate_path_attr_with_mask_all_servers(block_transport_server_p, 
                    FBE_BLOCK_PATH_ATTR_FLAGS_CLIENT_DOWNLOAD_REQ, 
                    FBE_BLOCK_PATH_ATTR_FLAGS_CLIENT_DOWNLOAD_REQ);
            }
            else
            {
                fbe_block_transport_server_propagate_path_attr_with_mask_all_servers(block_transport_server_p, 
                    (download_attr & FBE_BLOCK_PATH_ATTR_FLAGS_CLIENT_DOWNLOAD_MASK), 
                    FBE_BLOCK_PATH_ATTR_FLAGS_CLIENT_DOWNLOAD_MASK);
            }
            break;

        default:
            /* Unsupported mode
             */
            status = FBE_STATUS_GENERIC_FAILURE;
            fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s unknown mode: %d not supported for download attr: 0x%x \n",
                                  __FUNCTION__, configuration_mode, download_attr);
            break;
    }

    return status;
}
/*************************************************
 * end fbe_virtual_drive_handle_download_request()
 *************************************************/

/*!**************************************************************
 * fbe_virtual_drive_trace()
 ****************************************************************
 * @brief
 *  Trace for a virtual drive at a given trace level. 
 * 
 * @param virtual_drive_p - pointer to virtual drive to trace for
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

void fbe_virtual_drive_trace(fbe_virtual_drive_t *virtual_drive_p,
                             fbe_trace_level_t trace_level,
                             fbe_virtual_drive_debug_flags_t trace_debug_flags,
                             fbe_char_t * string_p, ...)
{   
    fbe_virtual_drive_debug_flags_t vd_debug_flags;
    fbe_virtual_drive_get_debug_flags(virtual_drive_p, &vd_debug_flags);

    /* We trace if there are no flags specified or if the input trace flag is enabled.
     */
    if ((trace_debug_flags == FBE_VIRTUAL_DRIVE_DEBUG_FLAG_NONE) ||
        (trace_debug_flags & vd_debug_flags))
    {
        char buffer[FBE_VIRTUAL_DRIVE_MAX_TRACE_CHARS];
        fbe_u32_t num_chars_in_string = 0;
        va_list argList;
    
        if (string_p != NULL)
        {
            va_start(argList, string_p);
            num_chars_in_string = _vsnprintf(buffer, FBE_VIRTUAL_DRIVE_MAX_TRACE_CHARS, string_p, argList);
            buffer[FBE_VIRTUAL_DRIVE_MAX_TRACE_CHARS - 1] = '\0';
            va_end(argList);
        }
        /* Display the string, if it has some characters.
         */
        if (num_chars_in_string > 0)
        {
            fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p, trace_level, FBE_TRACE_MESSAGE_ID_INFO, buffer);
        }
    }
    return;
}
/**************************************
 * end fbe_virtual_drive_trace()
 **************************************/

/*!****************************************************************************
 * fbe_virtual_drive_get_medic_action_priority()
 ******************************************************************************
 * @brief
 *   This function gets medic action priority for a virtual drive.
 *
 * @param virtual_drive_p           - pointer to a virtual drive
 * @param medic_action_priority     - pointer to priority
 *
 * @return fbe_status_t   
 *
 * @author
 *   03/08/2012 - Created. chenl6
 *
 ******************************************************************************/
fbe_status_t fbe_virtual_drive_get_medic_action_priority(fbe_virtual_drive_t* virtual_drive_p,
                                                         fbe_medic_action_priority_t *medic_action_priority)
{
    fbe_medic_action_priority_t current_priority, highest_priority;
    fbe_lifecycle_state_t       lifecycle_state;
    fbe_status_t                status;
    fbe_raid_group_t           *raid_group_p = (fbe_raid_group_t *)virtual_drive_p;
    fbe_bool_t                  b_pass_thru_mode = FBE_TRUE;

    /* Init medic action priority to IDLE. */
    fbe_medic_get_action_priority(FBE_MEDIC_ACTION_IDLE, &highest_priority);

    /* Get the configuration mode.  If the virtual drive is in pass-thru mode
     * we will simply propagate the downstream priority.
     */
    b_pass_thru_mode = fbe_virtual_drive_is_pass_thru_configuration_mode_set(virtual_drive_p);

    /* If we are not in a Ready state we should not be updating the medic priority.
     */
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

    /* If we are in pass-thru mode simply propagate the downstream priority
     * by returning Idle.
     */
    if (b_pass_thru_mode == FBE_TRUE)
    {
        /* Return Idle so that what every is downstream is propagated upstream.
         */
        *medic_action_priority = highest_priority;
        return FBE_STATUS_OK;
    }

    /* Check whether the raid group is rebuilding. */
    if (fbe_raid_group_background_op_is_metadata_rebuild_need_to_run(raid_group_p) ||
        fbe_raid_group_background_op_is_rebuild_need_to_run(raid_group_p))
    {
        fbe_medic_get_action_priority(FBE_MEDIC_ACTION_COPY, &current_priority);
        highest_priority = fbe_medic_get_highest_priority(highest_priority, current_priority);
    }

    /* Update the return value.
     */
    *medic_action_priority = highest_priority;
    return FBE_STATUS_OK;
}
/************************************************
 * end fbe_virtual_drive_get_medic_action_priority()
 ************************************************/

/*!****************************************************************************
 * fbe_virtual_drive_check_medic_action_priority()
 ******************************************************************************
 * @brief
 *   This function checks medic action priority between the virtual drive
 * and the event.
 *
 * @param virtual_drive_p        - pointer to a virtual drive
 * @param event_p                - pointer to an event
 *
 * @return FBE_TRUE if the event has higher priority.   
 *
 * @author
 *   03/08/2012 - Created. chenl6
 *
 ******************************************************************************/
static fbe_bool_t fbe_virtual_drive_check_medic_action_priority(fbe_virtual_drive_t* virtual_drive_p,
                                              fbe_event_t * event_p)
{
    fbe_medic_action_priority_t virtual_drive_priority, event_priority;

    fbe_event_get_medic_action_priority(event_p, &event_priority);
    if (event_priority == FBE_MEDIC_ACTION_HIGHEST_PRIORITY)
    {
        /* This is the case when the event has a default priority.
         * That means we don't need to check for VD priority. 
         */
        return FBE_TRUE;
    }

    fbe_virtual_drive_get_medic_action_priority(virtual_drive_p, &virtual_drive_priority);

    if (event_priority < virtual_drive_priority)
    {
        fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                              FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s Permission denied: event_priority %d, vd_priority %d\n", 
                              __FUNCTION__, event_priority, virtual_drive_priority);
        return FBE_FALSE;
    }

    return FBE_TRUE;
}
/************************************************
 * end fbe_virtual_drive_check_medic_action_priority()
 ************************************************/

/*!****************************************************************************
 * fbe_virtual_drive_download_request_event_entry()
 ******************************************************************************
 * @brief
 *   This function processes download trial run request from PVD.
 *
 * @param in_virtual_drive_p        - pointer to a virtual drive
 * @param in_event_context          - event_context
 *
 * @return fbe_status_t.   
 *
 * @author
 *   07/30/2012 - Created. Lili Chen
 *
 ******************************************************************************/
fbe_status_t
fbe_virtual_drive_download_request_event_entry(fbe_virtual_drive_t* in_virtual_drive_p,
                                               fbe_event_context_t  in_event_context)
{
    fbe_event_t*                            event_p;
    fbe_virtual_drive_configuration_mode_t  configuration_mode;

    fbe_base_object_trace((fbe_base_object_t *) in_virtual_drive_p,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n", __FUNCTION__);

    // cast context to event object
    event_p = (fbe_event_t *) in_event_context;

    /* Get the configuration mode of the virtual drive object */
    fbe_virtual_drive_get_configuration_mode(in_virtual_drive_p, &configuration_mode);

    if ((configuration_mode != FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_FIRST_EDGE) &&
        (configuration_mode != FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_SECOND_EDGE))
    {
        fbe_base_object_trace((fbe_base_object_t *)in_virtual_drive_p,
                              FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s trial run request rejected\n", __FUNCTION__);
        fbe_event_set_flags(event_p, FBE_EVENT_FLAG_DENY);
        fbe_event_set_status(event_p, FBE_EVENT_STATUS_OK);
        fbe_event_complete(event_p);
        return FBE_STATUS_OK;
    }

    /* Send event to ask for from upstream object */
    fbe_base_config_send_event((fbe_base_config_t *)in_virtual_drive_p,FBE_EVENT_TYPE_DOWNLOAD_REQUEST, event_p );

    return FBE_STATUS_OK;
}
/************************************************
 * end fbe_virtual_drive_download_request_event_entry()
 ************************************************/

/*!**************************************************************
 * fbe_virtual_drive_send_checkpoint_notification()
 ****************************************************************
 * @brief
 *  Handle a notification that the peer moved the checkpoint.
 *  We will check to see what operation is ongoing and
 *  send the notification for this.
 *
 * @param virtual_drive_p - Pointer to virtual drive           
 *
 * @return fbe_status_t
 *
 * @author
 *  3/20/2012 - Created. Rob Foley
 *
 ****************************************************************/
static fbe_status_t fbe_virtual_drive_send_checkpoint_notification(fbe_virtual_drive_t *virtual_drive_p)
{

	
	fbe_status_t status;
	/*since the event comes at DPC context via CMI and we'll have to do some packet alloactions.
	let's delegate theat to the monitor
	*/

	 status = fbe_lifecycle_set_cond(&fbe_virtual_drive_lifecycle_const,
									 (fbe_base_object_t *)virtual_drive_p,
                                     FBE_VIRTUAL_DRIVE_LIFECYCLE_COND_BACKGROUND_MONITOR_OPERATION);

	 if (status != FBE_STATUS_OK) 
     {
		 fbe_base_object_trace((fbe_base_object_t*)virtual_drive_p, 
                               FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO, 
                               "%s failed to set condition \n", __FUNCTION__);

	 }

    return status;
}
/*****************************************************
 * end fbe_virtual_drive_send_checkpoint_notification()
 *****************************************************/

/*!***************************************************************************
 *          fbe_virtual_drive_peer_lost_event_entry()
 ***************************************************************************** 
 * 
 * @brief   Handle the `peer lost' event.  The virtual drive has now become
 *          active so it needs to take over any in-progress copy operations.
 *          Evaluate downstream health/set condition based on downstream health
 *          will drive any active copy operation.
 *
 * @param   virtual_drive_p - Pointer to virtual drive           
 *
 * @return  fbe_status_t
 *
 * @author
 *  08/06/2012  Ron Proulx  - Created
 *
 *****************************************************************************/
static fbe_status_t fbe_virtual_drive_peer_lost_event_entry(fbe_virtual_drive_t *virtual_drive_p)
{
    fbe_bool_t                                  b_is_active;
    fbe_virtual_drive_configuration_mode_t      configuration_mode;
    fbe_base_config_downstream_health_state_t   downstream_health_state;
    fbe_bool_t                                  b_job_in_progress;
    fbe_virtual_drive_flags_t                   flags;
    fbe_spare_swap_command_t                    swap_command_type = FBE_SPARE_SWAP_INVALID_COMMAND;

    /* Get the configuration mode.
     */
    b_is_active = fbe_base_config_is_active((fbe_base_config_t*)virtual_drive_p);
    fbe_virtual_drive_get_copy_request_type(virtual_drive_p, &swap_command_type);

    /* Determine if we are active or not.
     */
    b_is_active = fbe_base_config_is_active((fbe_base_config_t*)virtual_drive_p);

    /* Get the configuration mode.
     */
    fbe_virtual_drive_get_configuration_mode(virtual_drive_p, &configuration_mode);

    /* Get the downstream health then set any conditions based on it. 
     */ 
    downstream_health_state = fbe_virtual_drive_verify_downstream_health(virtual_drive_p);

    /* Check if there is a job in progress.
     */
    b_job_in_progress = fbe_raid_group_is_clustered_flag_set((fbe_raid_group_t *)virtual_drive_p, FBE_RAID_GROUP_CLUSTERED_FLAG_SWAP_JOB_IN_PROGRESS);

    /* The peer is lost and currently this is not the active SP (it will
     * shortly become the active) and there was a job in progress we
     * need to set the `swap request in progress' flag.
    */
    if ((b_job_in_progress == FBE_TRUE) &&
        (b_is_active == FBE_FALSE)         )
    {
        fbe_virtual_drive_set_flag(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_SWAP_REQUEST_IN_PROGRESS);
        if (swap_command_type == FBE_SPARE_SWAP_INVALID_COMMAND)
        {
            fbe_virtual_drive_set_copy_request_type(virtual_drive_p, FBE_SPARE_SWAP_IN_PERMANENT_SPARE_COMMAND);
        }
    }

    /* Update the original pvd id as required.
     */
    fbe_virtual_drive_swap_save_original_pvd_object_id(virtual_drive_p);

    /* Get the virtual drive flags.
     */
    fbe_virtual_drive_get_flags(virtual_drive_p, &flags);

    /* Trace some information.
     */
    fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "virtual_drive: peer lost event active: %d mode: %d health: %d flags: 0x%x jobinprg: %d\n",
                          b_is_active, configuration_mode, downstream_health_state, flags, b_job_in_progress);
    
    /* Set the specific lifecycle condition based on health of the path with
     * downstream objects.
     */
    fbe_virtual_drive_set_condition_based_on_downstream_health(virtual_drive_p, downstream_health_state, __FUNCTION__);
     
    /* Return success
     */
    return FBE_STATUS_OK;
}
/*****************************************************
 * end fbe_virtual_drive_peer_lost_event_entry()
 *****************************************************/

/*!**************************************************************
 * fbe_virtual_drive_handle_attribute_change_for_slf()
 ****************************************************************
 * @brief
 *  This is the handler for a change in the edge attribute.
 *
 * @param virtual_drive_p - virtual drive that is changing.
 * @param edge_p - The context for the event.
 *
 * @return - Status of the handling.
 *
 * @author
 *  4/30/2012 - Created. Lili Chen
 *
 ****************************************************************/
static fbe_status_t fbe_virtual_drive_handle_attribute_change_for_slf(fbe_virtual_drive_t *virtual_drive_p, 
                                                                      fbe_block_edge_t *edge_p)
{
    fbe_bool_t b_set, b_clear;


    /* Don't bother if the edge is just attached..*/
    if (edge_p == NULL)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Set or clear NOT_PREFERRED attr */
    fbe_raid_group_event_check_slf_attr((fbe_raid_group_t *)virtual_drive_p, edge_p, &b_set, &b_clear);
    if (b_set)
    {
        fbe_block_transport_server_propagate_path_attr_with_mask_all_servers(
                    &virtual_drive_p->spare.raid_group.base_config.block_transport_server, 
                    FBE_BLOCK_PATH_ATTR_FLAGS_PATH_NOT_PREFERRED, 
                    FBE_BLOCK_PATH_ATTR_FLAGS_PATH_NOT_PREFERRED);
        fbe_raid_group_set_clustered_flag((fbe_raid_group_t *)virtual_drive_p, FBE_RAID_GROUP_CLUSTERED_FLAG_SLF_REQ);
    }
    else if (b_clear)
    {
        fbe_block_transport_server_propagate_path_attr_with_mask_all_servers(
                    &virtual_drive_p->spare.raid_group.base_config.block_transport_server, 
                    0, 
                    FBE_BLOCK_PATH_ATTR_FLAGS_PATH_NOT_PREFERRED);
        fbe_raid_group_clear_clustered_flag((fbe_raid_group_t *)virtual_drive_p, FBE_RAID_GROUP_CLUSTERED_FLAG_SLF_REQ);
    }

    return FBE_STATUS_OK;
}
/*********************************************************
 * end fbe_virtual_drive_handle_attribute_change_for_slf()
 *********************************************************/

/*!***************************************************************************
 *          fbe_virtual_drive_attribute_changed()
 *****************************************************************************
 *
 * @brief   Handle the edge attribute changing. Currently we try to propagate 
 *          the medic priority upstream and update the slf state.
 *
 * @param   virtual_drive_p - Pointer to virtual drive object
 * @param   edge_p - Pointer to edge that just had it's attributes 
 *
 * @return  fbe_status_t 
 *
 * @author
 *  12/06/2012  Ron Proulx  - Created.
 *
 *****************************************************************************/
fbe_status_t fbe_virtual_drive_attribute_changed(fbe_virtual_drive_t *virtual_drive_p,  
                                                 fbe_block_edge_t *edge_p)              
{
    fbe_status_t                status;
    fbe_medic_action_priority_t ds_medic_priority;
    fbe_medic_action_priority_t rg_medic_priority;
    fbe_medic_action_priority_t highest_medic_priority;

    ds_medic_priority = fbe_base_config_get_downstream_edge_priority((fbe_base_config_t*)virtual_drive_p);
    fbe_virtual_drive_get_medic_action_priority(virtual_drive_p, &rg_medic_priority);
    
    highest_medic_priority = fbe_medic_get_highest_priority(rg_medic_priority, ds_medic_priority);

    /* If the priority changed trace the information
     */
    if (highest_medic_priority != rg_medic_priority)
    {
        fbe_base_object_trace((fbe_base_object_t*)virtual_drive_p, 
                              FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO, 
                              "virtual_drive: attr changed set priority: %d rg priority: %d downstream priority: %d\n", 
                              highest_medic_priority, rg_medic_priority, ds_medic_priority);
    }

    /* Always update the priority.
     */
    status = fbe_base_config_set_upstream_edge_priority((fbe_base_config_t*)virtual_drive_p, highest_medic_priority);

    /* Now handle any changed related to slf.
     */
    status = fbe_virtual_drive_handle_attribute_change_for_slf(virtual_drive_p, edge_p);
    
    return status;
}
/******************************************
 * end fbe_virtual_drive_attribute_changed()
 ******************************************/

/*!***************************************************************************
 *          fbe_virtual_drive_peer_lost_event_entry()
 ***************************************************************************** 
 * 
 * @brief   Handle the `peer lost' event.  The virtual drive has now become
 *          active so it needs to take over any in-progress copy operations.
 *          Evaluate downstream health/set condition based on downstream health
 *          will drive any active copy operation.
 *
 * @param   virtual_drive_p - Pointer to virtual drive           
 *
 * @return  fbe_status_t
 *
 * @author
 *  08/06/2012  Ron Proulx  - Created
 *
 *****************************************************************************/
/**************************************
 * end file fbe_virtual_drive_event.c
 **************************************/
