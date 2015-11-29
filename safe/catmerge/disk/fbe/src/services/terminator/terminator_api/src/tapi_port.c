#include "fbe_terminator.h"
#include "fbe_terminator_miniport_interface.h"
#include "fbe_terminator_device_registry.h"
#include "terminator_port.h"

fbe_status_t fbe_terminator_api_start_port_reset (fbe_terminator_api_device_handle_t port_handle)
{
    fbe_status_t        status = FBE_STATUS_GENERIC_FAILURE;
    fbe_terminator_device_ptr_t port_ptr;
    tdr_status_t        tdr_status;
    fbe_u32_t           port_index;

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
                     FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                     "%s entry\n", __FUNCTION__);

    tdr_status = fbe_terminator_device_registry_get_device_ptr(port_handle, &port_ptr);
    if (tdr_status == TDR_STATUS_GENERIC_FAILURE)
    {
        return status;
    }
    status = terminator_get_miniport_port_index(port_ptr, &port_index);
    if (status != FBE_STATUS_OK)
    {
        return status;
    }
    status = fbe_terminator_miniport_api_start_port_reset(port_index);
    return status;
}
fbe_status_t fbe_terminator_api_complete_port_reset (fbe_terminator_api_device_handle_t port_handle)
{
    fbe_status_t        status = FBE_STATUS_GENERIC_FAILURE;
    fbe_terminator_device_ptr_t port_ptr;
    tdr_status_t tdr_status;
    fbe_u32_t           port_index;

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
                     FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                     "%s entry\n", __FUNCTION__);

    tdr_status = fbe_terminator_device_registry_get_device_ptr(port_handle, &port_ptr);
    if (tdr_status == TDR_STATUS_GENERIC_FAILURE)
    {
        return status;
    }
    status = terminator_get_miniport_port_index(port_ptr, &port_index);
    if (status != FBE_STATUS_OK)
    {
        return status;
    }
    status = fbe_terminator_miniport_api_complete_port_reset(port_index);

    return status;
}

/* This will do an auto-reset which is nothing but doing the following
 * sequence of events. 1. send reset-start 2. logout all devices on
 * port. 3. send reset-start 4. login all devices
 */
fbe_status_t fbe_terminator_api_auto_port_reset (fbe_terminator_api_device_handle_t port_handle)
{
    fbe_status_t        status = FBE_STATUS_GENERIC_FAILURE;

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
                     FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                     "%s entry\n", __FUNCTION__);

//  if (port_number >= FBE_PMC_SHIM_MAX_PORTS ) {
//      return FBE_STATUS_GENERIC_FAILURE;
//  }
//  status = fbe_terminator_miniport_api_auto_port_reset(port_number);

    return status;
}

// This should NOT generate login event for the port itself
fbe_status_t fbe_terminator_api_login_all_devices_on_port(fbe_terminator_api_device_handle_t port_handle)
{
    fbe_status_t        status = FBE_STATUS_GENERIC_FAILURE;

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
                     FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                     "%s entry\n", __FUNCTION__);

//  status = fbe_terminator_login_all_devices_on_port(handle);
//  RETURN_ON_ERROR_STATUS;
//
//  status = fbe_terminator_miniport_api_all_devices_on_port_logged_in(handle);
//  RETURN_ON_ERROR_STATUS;
//
//  status = FBE_STATUS_OK;
    return status;
}

fbe_status_t fbe_terminator_api_wait_port_logout_complete(fbe_terminator_api_device_handle_t port_handle)
{
	fbe_status_t    status = FBE_STATUS_GENERIC_FAILURE;
	fbe_terminator_device_ptr_t port_ptr;
	tdr_status_t	tdr_status;

	terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH,
					FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
					"%s entry\n", __FUNCTION__);

	tdr_status = fbe_terminator_device_registry_get_device_ptr(port_handle, &port_ptr);

	if(tdr_status != TDR_STATUS_GENERIC_FAILURE)
	{
		status = fbe_terminator_wait_on_port_logout_complete(port_ptr);
	}

	return status;
}

fbe_status_t fbe_terminator_api_wait_on_port_reset_clear(fbe_terminator_api_device_handle_t port_handle, fbe_cpd_shim_driver_reset_event_type_t reset_event)
{
	fbe_status_t    status = FBE_STATUS_GENERIC_FAILURE;
	fbe_terminator_device_ptr_t port_ptr;
	tdr_status_t	tdr_status;

	terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH,
					FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
					"%s entry\n", __FUNCTION__);

	tdr_status = fbe_terminator_device_registry_get_device_ptr(port_handle, &port_ptr);

	if(tdr_status != TDR_STATUS_GENERIC_FAILURE)
	{
		status = fbe_terminator_wait_on_port_reset_clear(port_ptr, reset_event);
	}
	return status;
}

//This does NOT generate a logout event for the port itself
fbe_status_t fbe_terminator_api_logout_all_devices_on_port(fbe_terminator_api_device_handle_t port_handle)
{
    fbe_status_t        status = FBE_STATUS_GENERIC_FAILURE;

//  terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH,
//                   FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
//                   "%s entry\n", __FUNCTION__);
//
//  status = fbe_terminator_logout_all_devices_on_port(handle);
//  RETURN_ON_ERROR_STATUS;
//
//  status = fbe_terminator_miniport_api_all_devices_on_port_logged_out(handle);
//  RETURN_ON_ERROR_STATUS;
//
//  status = FBE_STATUS_OK;
    return status;
}

fbe_status_t fbe_terminator_api_get_hardware_info(fbe_terminator_api_device_handle_t port_handle, fbe_cpd_shim_hardware_info_t *hdw_info)
{
    fbe_status_t        status = FBE_STATUS_GENERIC_FAILURE;
    fbe_terminator_device_ptr_t port_ptr;
    tdr_status_t    tdr_status;

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH,
                    FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                    "%s entry\n", __FUNCTION__);

    tdr_status = fbe_terminator_device_registry_get_device_ptr(port_handle, &port_ptr);

    if(tdr_status != TDR_STATUS_OK)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = port_get_hardware_info(port_ptr, hdw_info);

    return status;
}

fbe_status_t fbe_terminator_api_set_hardware_info(fbe_terminator_api_device_handle_t port_handle, fbe_cpd_shim_hardware_info_t *hdw_info)
{
    fbe_status_t        status = FBE_STATUS_GENERIC_FAILURE;
    fbe_terminator_device_ptr_t port_ptr;
    tdr_status_t    tdr_status;

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH,
                    FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                    "%s entry\n", __FUNCTION__);

    tdr_status = fbe_terminator_device_registry_get_device_ptr(port_handle, &port_ptr);

    if(tdr_status != TDR_STATUS_OK)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = port_set_hardware_info(port_ptr, hdw_info);

    return status;
}

fbe_status_t fbe_terminator_api_get_sfp_media_interface_info(fbe_terminator_api_device_handle_t port_handle, fbe_cpd_shim_sfp_media_interface_info_t *sfp_media_interface_info)
{
    fbe_status_t        status = FBE_STATUS_GENERIC_FAILURE;
    fbe_terminator_device_ptr_t port_ptr;
    tdr_status_t    tdr_status;

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH,
                    FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                    "%s entry\n", __FUNCTION__);

    tdr_status = fbe_terminator_device_registry_get_device_ptr(port_handle, &port_ptr);

    if(tdr_status != TDR_STATUS_OK)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = port_get_sfp_media_interface_info(port_ptr, sfp_media_interface_info);

    return status;
}

fbe_status_t fbe_terminator_api_set_sfp_media_interface_info(fbe_terminator_api_device_handle_t port_handle, fbe_cpd_shim_sfp_media_interface_info_t *sfp_media_interface_info)
{
    fbe_status_t        status = FBE_STATUS_GENERIC_FAILURE;
    fbe_terminator_device_ptr_t port_ptr;
    tdr_status_t    tdr_status;
    fbe_cpd_shim_sfp_media_interface_info_t old_sfp_media_interface_info;

    tdr_status = fbe_terminator_device_registry_get_device_ptr(port_handle, &port_ptr);

    if(tdr_status != TDR_STATUS_OK)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* check and call sfp event callback function if necessary */
    status = port_get_sfp_media_interface_info(port_ptr, &old_sfp_media_interface_info);
    RETURN_ON_ERROR_STATUS;
    /* set */
    status = port_set_sfp_media_interface_info(port_ptr, sfp_media_interface_info);
    if(old_sfp_media_interface_info.condition_type != sfp_media_interface_info->condition_type)
    {
        status = fbe_terminator_miniport_api_event_notify(port_ptr, sfp_media_interface_info->condition_type);
        RETURN_ON_ERROR_STATUS;
    }  
    
    return status;
}

fbe_status_t fbe_terminator_api_get_port_link_info(fbe_terminator_api_device_handle_t port_handle, fbe_cpd_shim_port_lane_info_t *port_lane_info)
{
    fbe_status_t        status = FBE_STATUS_GENERIC_FAILURE;
    fbe_terminator_device_ptr_t port_ptr;
    tdr_status_t    tdr_status;

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH,
                    FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                    "%s entry\n", __FUNCTION__);

    tdr_status = fbe_terminator_device_registry_get_device_ptr(port_handle, &port_ptr);

    if(tdr_status != TDR_STATUS_OK)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = port_get_port_link_info(port_ptr, port_lane_info);

    return status;
}

fbe_status_t fbe_terminator_api_get_encryption_key(fbe_terminator_api_device_handle_t port_handle, 
                                                   fbe_key_handle_t key_handle,
                                                   fbe_u8_t *key_buffer)
{
    fbe_status_t        status = FBE_STATUS_GENERIC_FAILURE;
    fbe_terminator_device_ptr_t port_ptr;
    tdr_status_t    tdr_status;

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH,
                    FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                    "%s entry\n", __FUNCTION__);

    tdr_status = fbe_terminator_device_registry_get_device_ptr(port_handle, &port_ptr);

    if(tdr_status != TDR_STATUS_OK)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = port_get_encryption_key(port_ptr, key_handle, key_buffer);

    return status;
}

fbe_status_t fbe_terminator_api_get_port_address(fbe_terminator_api_device_handle_t port_handle,
                                                 fbe_address_t *address)
{
     fbe_status_t        status = FBE_STATUS_GENERIC_FAILURE;
    fbe_terminator_device_ptr_t port_ptr;
    tdr_status_t    tdr_status;
    fbe_u32_t           port_index;

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
                     FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                     "%s entry\n", __FUNCTION__);

    tdr_status = fbe_terminator_device_registry_get_device_ptr(port_handle, &port_ptr);
    if (tdr_status == TDR_STATUS_GENERIC_FAILURE)
    {
        return status;
    }
    status = terminator_get_miniport_port_index(port_ptr, &port_index);
    if (status != FBE_STATUS_OK)
    {
        return status;
    }
    status = fbe_terminator_miniport_api_get_port_address(port_index, address);

    return status;

}
fbe_status_t fbe_terminator_api_set_port_link_info(fbe_terminator_api_device_handle_t port_handle, fbe_cpd_shim_port_lane_info_t *port_lane_info)
{
    fbe_status_t        status = FBE_STATUS_GENERIC_FAILURE;
    fbe_terminator_device_ptr_t port_ptr;
    tdr_status_t    tdr_status;

    terminator_trace(FBE_TRACE_LEVEL_DEBUG_HIGH,
                    FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                    "%s entry\n", __FUNCTION__);

    tdr_status = fbe_terminator_device_registry_get_device_ptr(port_handle, &port_ptr);

    if(tdr_status != TDR_STATUS_OK)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }
    /* set */
    status = port_set_port_link_info(port_ptr, port_lane_info);

    status = fbe_terminator_miniport_api_link_event_notify(port_ptr, port_lane_info->link_state);

    return status;
}
