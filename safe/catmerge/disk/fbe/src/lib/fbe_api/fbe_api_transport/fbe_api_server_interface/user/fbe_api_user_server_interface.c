/***************************************************************************
 * Copyright (C) EMC Corporation 2001-2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *  fbe_api_user_server_main.c
 ***************************************************************************
 *
 *  Description
 *      Define the interfaces for server controls
 ****************************************************************************/
#include "fbe/fbe_types.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_transport.h"
#include "fbe/fbe_api_common_transport.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_api_database_interface.h"

/**************************************
	    Local variables
**************************************/

/*******************************************
	    Local functions
********************************************/

/*********************************************************************
 *            fbe_api_user_server_set_driver_entries ()
 *********************************************************************
 *
 *  Description: set the entry points for the drivers
 *
 *  Inputs: none
 *
 *  Return Value: success or failure
 *
 *  History:
 *    12/02/10: bo gao    created
 *
 *********************************************************************/
fbe_status_t FBE_API_CALL fbe_api_user_server_set_driver_entries(void)
{
    fbe_status_t    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t status_info;

    status = fbe_api_common_send_control_packet_to_service (FBE_USER_SERVER_CONTROL_CODE_SET_DRIVER_ENTRIES,
                                NULL,
                                0,
                                FBE_SERVICE_ID_USER_SERVER,
                                FBE_PACKET_FLAG_NO_ATTRIB,
                                &status_info,
                                FBE_PACKAGE_ID_PHYSICAL); /* package_id is not checked by the other end, just to pass thru the transport */

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
    fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
               status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;
}

/*********************************************************************
 *            fbe_api_user_server_register_notifications ()
 *********************************************************************
 *
 *  Description: register notification elements
 *
 *  Inputs: none
 *
 *  Return Value: success or failure
 *
 *  History:
 *    12/02/10: bo gao    created
 *
 *********************************************************************/
fbe_status_t FBE_API_CALL fbe_api_user_server_register_notifications(void)
{
    fbe_status_t    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t status_info;

    status = fbe_api_common_send_control_packet_to_service (FBE_USER_SERVER_CONTROL_CODE_REGISTER_NTIFICATIONS,
                                NULL,
                                0,
                                FBE_SERVICE_ID_USER_SERVER,
                                FBE_PACKET_FLAG_NO_ATTRIB,
                                &status_info,
                                FBE_PACKAGE_ID_PHYSICAL); /* package_id is not checked by the other end, just to pass thru the transport */

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
    fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
               status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;
}
