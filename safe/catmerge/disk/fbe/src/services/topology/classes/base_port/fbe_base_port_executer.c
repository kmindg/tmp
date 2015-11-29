#include "base_port_private.h"

fbe_status_t 
fbe_base_port_event_entry(fbe_object_handle_t object_handle, fbe_event_type_t event_type, fbe_event_context_t event_context)
{
	fbe_base_port_t * base_port = NULL;
	fbe_status_t status;

	base_port = (fbe_base_port_t *)fbe_base_handle_to_pointer(object_handle);

	fbe_base_object_trace((fbe_base_object_t*)base_port,
	                      FBE_TRACE_LEVEL_DEBUG_HIGH,
	                      FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
	                      "%s entry\n", __FUNCTION__);

	/* We are not interested in any event at this time, so just forward it to the super class */
	status = fbe_base_discovering_event_entry(object_handle, event_type, event_context);

	return status;
}

/*!*************************************************************************
* @fn fbe_base_port_callback(                  
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
fbe_base_port_callback(fbe_cpd_shim_callback_info_t * callback_info, fbe_cpd_shim_callback_context_t context)
{
	fbe_status_t status = FBE_STATUS_OK;
	fbe_base_port_t * base_port = NULL;
	fbe_port_number_t port_number;        

	base_port = (fbe_base_port_t *)context;

    fbe_base_object_trace((fbe_base_object_t*)base_port,
	                      FBE_TRACE_LEVEL_DEBUG_LOW,
	                      FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
	                      "%s entry\n", __FUNCTION__);

	status = fbe_base_port_get_port_number((fbe_base_port_t *)base_port, &port_number);

    switch(callback_info->callback_type)
    {
    case FBE_CPD_SHIM_CALLBACK_TYPE_SFP:

		fbe_base_object_trace((fbe_base_object_t*)base_port,
								FBE_TRACE_LEVEL_INFO,
								FBE_TRACE_MESSAGE_ID_INFO,
								"%s SFP event. Port %d \n",
								__FUNCTION__,port_number);

        
        status = fbe_lifecycle_set_cond(&fbe_base_port_lifecycle_const, 
								(fbe_base_object_t*)base_port, 
								FBE_BASE_PORT_LIFECYCLE_COND_SFP_INFORMATION_CHANGE);
	    if (status != FBE_STATUS_OK) {
		    fbe_base_object_trace((fbe_base_object_t*)base_port, 
								    FBE_TRACE_LEVEL_ERROR,
								    FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								    "%s can't set SFP change condition, status: 0x%X",
								    __FUNCTION__, status);
	    }        
        break;
    case FBE_CPD_SHIM_CALLBACK_TYPE_LINK_DOWN:
    case FBE_CPD_SHIM_CALLBACK_TYPE_LINK_UP:
    case FBE_CPD_SHIM_CALLBACK_TYPE_LANE_STATUS:

        fbe_base_object_trace((fbe_base_object_t*)base_port,
                                FBE_TRACE_LEVEL_INFO,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s Link event. Port %d \n",
                                __FUNCTION__,port_number);

        status = fbe_lifecycle_set_cond(&fbe_base_port_lifecycle_const, 
                                (fbe_base_object_t*)base_port, 
                                FBE_BASE_PORT_LIFECYCLE_COND_GET_PORT_DATA);
        if (status != FBE_STATUS_OK) {
            fbe_base_object_trace((fbe_base_object_t*)base_port, 
                                    FBE_TRACE_LEVEL_ERROR,
                                    FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                    "%s can't set get port data condition, status: 0x%X",
                                    __FUNCTION__, status);
        }
        break;
    }

    return status;
}