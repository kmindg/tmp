/***************************************************************************
 * Copyright (C) EMC Corporation 2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_api_cms_interface.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the APIs used to communicate with the Clustered
 *  Memory Service
 *
 * @ingroup fbe_api_storage_extent_package_interface_class_files
 * @ingroup fbe_api_cms_interface
 *
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_types.h"
#include "fbe/fbe_api_common.h"
#include "fbe_cms.h"
#include "fbe/fbe_api_common_transport.h"

/**************************************
                Local variables
**************************************/

/*******************************************
                Local functions
********************************************/


/*!***************************************************************
 * @fn fbe_api_cms_service_get_persistence_status(fbe_cms_memory_persist_status_t *status_p)
 ****************************************************************
 * @brief
 *  This function gets general information related to CMS operations
 *
 * @param cmi_info      - The structure to return the information in
 *
 * @return
 *  fbe_status_t
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL
fbe_api_cms_service_get_persistence_status(fbe_cms_memory_persist_status_t *status_p)
{
    fbe_status_t                                status = FBE_STATUS_OK;
    fbe_api_control_operation_status_info_t     status_info;
    fbe_trace_level_t                           trace_level = FBE_TRACE_LEVEL_ERROR;
    fbe_cms_service_get_persistence_status_t    persistence_status;

    status = fbe_api_common_send_control_packet_to_service (FBE_CMS_CONTROL_CODE_GET_PERSISTENCE_STATUS,
                                                            &persistence_status, 
                                                            sizeof(persistence_status),
                                                            FBE_SERVICE_ID_CMS,
                                                            FBE_PACKET_FLAG_NO_ATTRIB,
                                                            &status_info,
                                                            FBE_PACKAGE_ID_SEP_0);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK)
    {
        fbe_api_trace (trace_level, "%s:status:%d, qual:%d op status:%d, op qual:%d\n", __FUNCTION__,
                       status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    *status_p = persistence_status.status;
    
    return status;
}

fbe_status_t FBE_API_CALL
fbe_api_cms_service_request_persistence(fbe_cms_client_id_t client_id, fbe_bool_t request)
{
    fbe_status_t                                status = FBE_STATUS_OK;
    fbe_cms_service_request_persistence_t       request_persistence;
    fbe_api_control_operation_status_info_t     status_info;
    fbe_trace_level_t                           trace_level = FBE_TRACE_LEVEL_ERROR;
    
    request_persistence.client_id   = client_id;
    request_persistence.request     = request;

    status = fbe_api_common_send_control_packet_to_service (FBE_CMS_CONTROL_CODE_REQUEST_PERSISTENCE,
                                                            &request_persistence, 
                                                            sizeof(request_persistence),
                                                            FBE_SERVICE_ID_CMS,
                                                            FBE_PACKET_FLAG_NO_ATTRIB,
                                                            &status_info,
                                                            FBE_PACKAGE_ID_SEP_0);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK)
    {
        fbe_api_trace (trace_level, "%s:status:%d, qual:%d op status:%d, op qual:%d\n", __FUNCTION__,
                       status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    return status;
}

/*************************
 * end file fbe_api_cms_interface.c
 *************************/
