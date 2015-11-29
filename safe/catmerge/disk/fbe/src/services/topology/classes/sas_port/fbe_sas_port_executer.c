#include "sas_port_private.h"

/*
 * LOCAL FUNCTION DEFINITIONS
 */
static fbe_status_t
fbe_sas_port_edge_state_change_event_entry(fbe_sas_port_t *sas_port,
                                                 fbe_event_context_t event_context);


fbe_status_t fbe_sas_port_event_entry(fbe_object_handle_t object_handle, fbe_event_type_t event_type, fbe_event_context_t event_context)
{
    fbe_sas_port_t * sas_port = NULL;
    fbe_status_t status;

    sas_port = (fbe_sas_port_t *)fbe_base_handle_to_pointer(object_handle);

    fbe_base_object_trace((fbe_base_object_t*)sas_port,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n", __FUNCTION__);

    switch(event_type)
    {
    case FBE_EVENT_TYPE_EDGE_STATE_CHANGE:
    case FBE_EVENT_TYPE_ATTRIBUTE_CHANGED:
        status = fbe_sas_port_edge_state_change_event_entry(sas_port, event_context); 
        break;
    default:
        status = FBE_STATUS_MORE_PROCESSING_REQUIRED;
        break;
    }
    if(status == FBE_STATUS_MORE_PROCESSING_REQUIRED)
    {
        /*
         * This class did not handle the event, go to the superclass
         * to try to handle it.
         */
        status = fbe_base_port_event_entry(object_handle, event_type, event_context);
    }

    return status; 
}

/****************************************************************
 * fbe_sas_port_edge_state_change_event_entry()
 ****************************************************************
 * DESCRIPTION:
 *  This function handles edge state change events.
 *
 * PARAMETERS:
 *  sas_port - port object context
 *  event_context - hidden in this void* is a pointer to the edge
 *
 * RETURNS:
 *  fbe_status_t - FBE_STATUS_OK 
 *                   - no error.
 *                 FBE_STATUS_MORE_PROCESSING_REQUIRED
 *                   - imply the event could not be handled here.  
 *
 * HISTORY:
 *  9/14/11 - created - bphilbin
 *
 ****************************************************************/
static fbe_status_t
fbe_sas_port_edge_state_change_event_entry(fbe_sas_port_t *sas_port,
                                                 fbe_event_context_t event_context)
{
    fbe_status_t status = FBE_STATUS_MORE_PROCESSING_REQUIRED;
    fbe_transport_id_t transport_id;
    fbe_path_attr_t path_attr;
    fbe_port_number_t port_number;     

    fbe_base_port_get_port_number((fbe_base_port_t *)sas_port, &port_number);
    
    /* Get the transport type from the edge pointer. */
    fbe_base_transport_get_transport_id((fbe_base_edge_t *) event_context, &transport_id);

    /* Only a single discovery edge transition is looked at here */
    if(transport_id == FBE_TRANSPORT_ID_DISCOVERY)
    {
        fbe_base_discovered_get_path_attributes((fbe_base_discovered_t*)sas_port, &path_attr);
        if( (path_attr & FBE_DISCOVERY_PATH_ATTR_POWERFAIL_DETECTED) &&
            !sas_port->power_failure_detected)

        {
            fbe_base_object_trace((fbe_base_object_t*)sas_port,
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s Power failure detected by port %d\n",
                                  __FUNCTION__,port_number);
            sas_port->power_failure_detected = TRUE;
        }
        else if(!(path_attr & FBE_DISCOVERY_PATH_ATTR_POWERFAIL_DETECTED) &&
                sas_port->power_failure_detected)
        {
            fbe_base_object_trace((fbe_base_object_t*)sas_port,
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s Power failure cleared detected by port %d\n",
                                  __FUNCTION__,port_number);
            sas_port->power_failure_detected = FALSE;
        }
    }
    return status;
}

/*!*************************************************************************
* @fn sas_port_callback(                  
*						fbe_cpd_shim_callback_info_t * callback_info, 
*						fbe_cpd_shim_callback_context_t context)
***************************************************************************
*
* @brief
*			  Handles asynchronous event callback from shim.
*			  
*
* @param      callback_info - Callback information.
* @param      context      - Callback context that was provided during registration. 
*
* @return     fbe_status_t.
*
* NOTES
*
* HISTORY
*
***************************************************************************/
fbe_status_t 
fbe_sas_port_callback(fbe_cpd_shim_callback_info_t * callback_info, fbe_cpd_shim_callback_context_t context)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_sas_port_t * sas_port = NULL;
    fbe_port_number_t port_number;        

    sas_port = (fbe_sas_port_t *)context;

    status = fbe_base_port_get_port_number((fbe_base_port_t *)sas_port, &port_number);

    switch(callback_info->callback_type)
    {
    case FBE_CPD_SHIM_CALLBACK_TYPE_SFP:

        fbe_base_object_trace((fbe_base_object_t*)sas_port,
                                FBE_TRACE_LEVEL_DEBUG_HIGH,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s SFP event. Port %d \n",
                                __FUNCTION__,port_number);

        
        status = fbe_lifecycle_set_cond(&fbe_sas_port_lifecycle_const, 
                                (fbe_base_object_t*)sas_port, 
                                FBE_BASE_PORT_LIFECYCLE_COND_SFP_INFORMATION_CHANGE);
        if (status != FBE_STATUS_OK) {
            fbe_base_object_trace((fbe_base_object_t*)sas_port, 
                                    FBE_TRACE_LEVEL_ERROR,
                                    FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                    "%s can't set SFP change condition, status: 0x%X",
                                    __FUNCTION__, status);
        }        
        break;
    case FBE_CPD_SHIM_CALLBACK_TYPE_LINK_DOWN:
    case FBE_CPD_SHIM_CALLBACK_TYPE_LINK_UP:
    case FBE_CPD_SHIM_CALLBACK_TYPE_LANE_STATUS:

        fbe_base_object_trace((fbe_base_object_t*)sas_port,
                                FBE_TRACE_LEVEL_DEBUG_HIGH,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s Link event. Port %d \n",
                                __FUNCTION__,port_number);

        status = fbe_lifecycle_set_cond(&fbe_sas_port_lifecycle_const, 
                                (fbe_base_object_t*)sas_port, 
                                FBE_BASE_PORT_LIFECYCLE_COND_GET_PORT_DATA);
        if (status != FBE_STATUS_OK) {
            fbe_base_object_trace((fbe_base_object_t*)sas_port, 
                                    FBE_TRACE_LEVEL_ERROR,
                                    FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                    "%s can't set get port data condition, status: 0x%X",
                                    __FUNCTION__, status);
        }
        break;

    }

    return status;
}