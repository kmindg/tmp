/***************************************************************************
 * Copyright (C) EMC Corporation 2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *  @file fbe_api_terminator_persistent_memory.c
 ***************************************************************************
 *
 *  @brief
 *   Persistent memory related APIs 
 *
 *  @version
 *   2011-10-31 Matthew Ferson - Created
 *    
 ***************************************************************************/

#include "fbe/fbe_winddk.h"
#include "fbe/fbe_types.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_common_transport.h"
#include "fbe/fbe_terminator_service.h"

/**************************************
				Local variables
**************************************/

/*******************************************
				Local functions
********************************************/


/*! 
 * @brief Get the value of the global flag indicating whether or not the 
 * Terminator should persist memory across an SP reboot. Synchronization
 * is the responsibility of the caller, as it is on hardware. 
 * 
 * @version
 *   2011-10-31 - Created. Matthew Ferson
 */
fbe_status_t fbe_api_terminator_persistent_memory_get_persistence_request(fbe_bool_t * request)
{
	fbe_status_t							        status = FBE_STATUS_GENERIC_FAILURE;
	fbe_api_control_operation_status_info_t	        status_info;
    fbe_terminator_get_persistence_request_ioctl_t	get_persistence_request_ioctl;

	status = fbe_api_common_send_control_packet_to_service( FBE_TERMINATOR_CONTROL_CODE_GET_PERSISTENCE_REQUEST,
															&get_persistence_request_ioctl,
															sizeof(get_persistence_request_ioctl),
															FBE_SERVICE_ID_TERMINATOR,
															FBE_PACKET_FLAG_NO_ATTRIB,
															&status_info,
															FBE_PACKAGE_ID_PHYSICAL);

	if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
		fbe_api_trace (FBE_TRACE_LEVEL_DEBUG_LOW, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
					    status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
		
		return FBE_STATUS_GENERIC_FAILURE;
	}

    *request = get_persistence_request_ioctl.request;

	return FBE_STATUS_OK;
}


/*!
 * @brief Set a global flag indicating whether or not the Terminator 
 * should persist memory across an SP reboot. Synchronization is the 
 * responsibility of the caller, as it is on hardware. 
 *  
 * @version
 * 2011-10-31 Matthew Ferson - Created
 *    
 */
fbe_status_t fbe_api_terminator_persistent_memory_set_persistence_request(fbe_bool_t request)
{
	fbe_status_t							        status = FBE_STATUS_GENERIC_FAILURE;
	fbe_api_control_operation_status_info_t	        status_info;
    fbe_terminator_set_persistence_request_ioctl_t	set_persistence_request_ioctl;

    set_persistence_request_ioctl.request = request;

	status = fbe_api_common_send_control_packet_to_service( FBE_TERMINATOR_CONTROL_CODE_SET_PERSISTENCE_REQUEST,
															&set_persistence_request_ioctl,
															sizeof(set_persistence_request_ioctl),
															FBE_SERVICE_ID_TERMINATOR,
															FBE_PACKET_FLAG_NO_ATTRIB,
															&status_info,
															FBE_PACKAGE_ID_PHYSICAL);

	if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
		fbe_api_trace (FBE_TRACE_LEVEL_DEBUG_LOW, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
					    status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
		
		return FBE_STATUS_GENERIC_FAILURE;
	}

	return FBE_STATUS_OK;
}

/*! 
 * @brief Set a global flag indicating whether or not the Terminator 
 * has successfully persisted memory. This is primarily a test hook 
 * to be called by FBE tests. 
 * 
 * @version
 *   2011-10-31 - Created. Matthew Ferson
 */
fbe_status_t fbe_api_terminator_persistent_memory_set_persistence_status(fbe_cms_memory_persist_status_t persistence_status)
{
	fbe_status_t							        status = FBE_STATUS_GENERIC_FAILURE;
	fbe_api_control_operation_status_info_t	        status_info;
    fbe_terminator_set_persistence_status_ioctl_t	set_persistence_status_ioctl;

    set_persistence_status_ioctl.status = persistence_status;

	status = fbe_api_common_send_control_packet_to_service( FBE_TERMINATOR_CONTROL_CODE_SET_PERSISTENCE_STATUS,
															&set_persistence_status_ioctl,
															sizeof(set_persistence_status_ioctl),
															FBE_SERVICE_ID_TERMINATOR,
															FBE_PACKET_FLAG_NO_ATTRIB,
															&status_info,
															FBE_PACKAGE_ID_PHYSICAL);

	if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
		fbe_api_trace (FBE_TRACE_LEVEL_DEBUG_LOW, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
					    status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
		
		return FBE_STATUS_GENERIC_FAILURE;
	}

	return FBE_STATUS_OK;
}

/*! 
 * @brief Get the value of the global flag indicating whether or not the 
 * Terminator has successfully persisted memory across an SP reboot.
 * 
 * @version
 *   2011-10-31 - Created. Matthew Ferson
 */
fbe_status_t fbe_api_terminator_persistent_memory_get_persistence_status(fbe_cms_memory_persist_status_t *persistence_status)
{
	fbe_status_t							        status = FBE_STATUS_GENERIC_FAILURE;
	fbe_api_control_operation_status_info_t	        status_info;
    fbe_terminator_get_persistence_status_ioctl_t	get_persistence_status_ioctl;

	status = fbe_api_common_send_control_packet_to_service( FBE_TERMINATOR_CONTROL_CODE_GET_PERSISTENCE_STATUS,
															&get_persistence_status_ioctl,
															sizeof(get_persistence_status_ioctl),
															FBE_SERVICE_ID_TERMINATOR,
															FBE_PACKET_FLAG_NO_ATTRIB,
															&status_info,
															FBE_PACKAGE_ID_PHYSICAL);

	if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
		fbe_api_trace (FBE_TRACE_LEVEL_DEBUG_LOW, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
					    status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
		
		return FBE_STATUS_GENERIC_FAILURE;
	}

    *persistence_status = get_persistence_status_ioctl.status;

	return FBE_STATUS_OK;
}


/*! 
 * @brief Zero the contents of the Terminator's persistent memory 
 * to simulate a case where the contents of memory have been lost. 
 * This is primarily a test hook to be called by FBE tests. 
 * 
 * @version
 *   2011-10-31 - Created. Matthew Ferson
 */
fbe_status_t fbe_api_terminator_persistent_memory_zero_memory(void)
{
	fbe_status_t							        status = FBE_STATUS_GENERIC_FAILURE;
	fbe_api_control_operation_status_info_t	        status_info;
    fbe_terminator_zero_persistent_memory_ioctl_t	zero_persistent_memory_ioctl;

	status = fbe_api_common_send_control_packet_to_service( FBE_TERMINATOR_CONTROL_CODE_ZERO_PERSISTENT_MEMORY,
															&zero_persistent_memory_ioctl,
															sizeof(zero_persistent_memory_ioctl),
															FBE_SERVICE_ID_TERMINATOR,
															FBE_PACKET_FLAG_NO_ATTRIB,
															&status_info,
															FBE_PACKAGE_ID_PHYSICAL);

	if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
		fbe_api_trace (FBE_TRACE_LEVEL_DEBUG_LOW, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
					    status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
		
		return FBE_STATUS_GENERIC_FAILURE;
	}

	return FBE_STATUS_OK;
}

/* end fbe_api_terminator_persistent_memory.c */
