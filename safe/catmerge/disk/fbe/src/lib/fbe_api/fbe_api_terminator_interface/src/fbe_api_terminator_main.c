/***************************************************************************
 * Copyright (C) EMC Corporation 2001-2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *  fbe_api_terminator_main.c
 ***************************************************************************
 *
 *  Description
 *      Terminator related APIs 
 *
 *  History:
 *      10/28/09    guov - Created
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


/*********************************************************************
 *            fbe_api_terminator_init(void)
 *********************************************************************
 *
 *  Description: Intialize Terminator
 *
 *	Inputs: None
 *
 *  Return Value: success or failue
 *
 *  History:
 *    10/28/09: guov	created
 *    
 *********************************************************************/
fbe_status_t fbe_api_terminator_init(void)
{
	fbe_status_t							status = FBE_STATUS_GENERIC_FAILURE;
	fbe_api_control_operation_status_info_t	status_info;

	status = fbe_api_common_send_control_packet_to_service (FBE_TERMINATOR_CONTROL_CODE_INIT,
															NULL,
															0,
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

/*********************************************************************
 *            fbe_api_terminator_destroy(void)
 *********************************************************************
 *
 *  Description: Destroy Terminator
 *
 *	Inputs: None
 *
 *  Return Value: success or failue
 *
 *  History:
 *    10/28/09: guov	created
 *    
 *********************************************************************/
fbe_status_t fbe_api_terminator_destroy(void)
{
	fbe_status_t							status = FBE_STATUS_GENERIC_FAILURE;
	fbe_api_control_operation_status_info_t	status_info;

	status = fbe_api_common_send_control_packet_to_service (FBE_TERMINATOR_CONTROL_CODE_DESTROY,
															NULL,
															0,
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


