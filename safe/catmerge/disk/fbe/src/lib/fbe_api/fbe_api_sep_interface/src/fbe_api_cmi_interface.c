/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_api_cmi_interface.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the APIs used to communicate wi the CMI service
 *
 * @ingroup fbe_api_storage_extent_package_interface_class_files
 * @ingroup fbe_api_cmi_interface
 *
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_types.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_cmi_interface.h"
#include "fbe/fbe_api_common_transport.h"

/**************************************
                Local variables
**************************************/

/*******************************************
                Local functions
********************************************/

static fbe_status_t fbe_api_cmi_service_get_info_from_package(fbe_cmi_service_get_info_t *cmi_info, fbe_package_id_t package_id);

/*!***************************************************************13yy
 * @fn fbe_api_cmi_service_get_info(fbe_cmi_service_get_info_t *cmi_info)

 ****************************************************************
 * @brief
 *  This function gets general information related to CMI operations for SEP
 *
 * @param cmi_info      - The structure to return the information in
 *
 * @return
 *  fbe_status_t
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL
fbe_api_cmi_service_get_info(fbe_cmi_service_get_info_t *cmi_info)
{
    fbe_status_t                                status = FBE_STATUS_OK;
    
    status = fbe_api_cmi_service_get_info_from_package(cmi_info, FBE_PACKAGE_ID_SEP_0);

    return status;
}

/*!***************************************************************
 * @fn fbe_api_esp_cmi_service_get_info(fbe_cmi_service_get_info_t *cmi_info)

 ****************************************************************
 * @brief
 *  This function gets general information related to CMI operations for ESP
 *
 * @param cmi_info      - The structure to return the information in
 *
 * @return
 *  fbe_status_t
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL
fbe_api_esp_cmi_service_get_info(fbe_cmi_service_get_info_t *cmi_info)
{
    fbe_status_t                                status = FBE_STATUS_OK;
    
    status = fbe_api_cmi_service_get_info_from_package(cmi_info, FBE_PACKAGE_ID_ESP);

    return status;
}

/*!***************************************************************
 * @fn fbe_api_cmi_service_get_info_from_package(fbe_cmi_service_get_info_t *cmi_info, 
 *                                               fbe_package_id_t package_id)
 ****************************************************************
 * @brief
 *  This function gets general information related to CMI operations for
 * specific package.
 *
 * @param cmi_info      - The structure to return the information in
 * @param package_id    - package id
 *
 * @return
 *  fbe_status_t
 *
 ****************************************************************/
static fbe_status_t 
fbe_api_cmi_service_get_info_from_package(fbe_cmi_service_get_info_t *cmi_info, fbe_package_id_t package_id)
{
    fbe_status_t                                status = FBE_STATUS_OK;
    fbe_api_control_operation_status_info_t     status_info;
    
    if (cmi_info == NULL) {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_common_send_control_packet_to_service (FBE_CMI_CONTROL_CODE_GET_INFO,
                                                            cmi_info, 
                                                            sizeof(fbe_cmi_service_get_info_t),
                                                            FBE_SERVICE_ID_CMI,
                                                            FBE_PACKET_FLAG_NO_ATTRIB,
                                                            &status_info,
                                                            package_id);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK)
    {
        /* It perfectly valid for the CMI to be down.
        */
        if ( (status == FBE_STATUS_NOT_INITIALIZED) || (status == FBE_STATUS_COMPONENT_NOT_FOUND) )
        {
            fbe_api_trace (FBE_TRACE_LEVEL_DEBUG_HIGH, "%s:status:%d, qual:%d op status:%d, op qual:%d\n", __FUNCTION__,
                           status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

            return status;
        }
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:status:%d, qual:%d op status:%d, op qual:%d\n", __FUNCTION__,
                       status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    return status;
}

/*!***************************************************************
 * @fn fbe_api_cmi_service_get_io_statistics()

 ****************************************************************
 * @brief
 *  This function gets general information related to CMI operations
 *
 * @param cmi_stat      - The structure to return the information in
 *
 * @return
 *  fbe_status_t
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL
fbe_api_cmi_service_get_io_statistics(fbe_cmi_service_get_io_statistics_t *cmi_stat)
{
    fbe_status_t                                status = FBE_STATUS_OK;
    fbe_api_control_operation_status_info_t     status_info;
    fbe_trace_level_t                           trace_level = FBE_TRACE_LEVEL_ERROR;
    
    if (cmi_stat == NULL) {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_common_send_control_packet_to_service (FBE_CMI_CONTROL_CODE_GET_IO_STATISTICS,
                                                            cmi_stat, 
                                                            sizeof(fbe_cmi_service_get_io_statistics_t),
                                                            FBE_SERVICE_ID_CMI,
                                                            FBE_PACKET_FLAG_NO_ATTRIB,
                                                            &status_info,
                                                            FBE_PACKAGE_ID_SEP_0);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK)
    {
        /* It perfectly valid for the CMI to be down.
        */
        if (status == FBE_STATUS_NOT_INITIALIZED)
        {
            trace_level = FBE_TRACE_LEVEL_DEBUG_LOW;
        }
        fbe_api_trace (trace_level, "%s:status:%d, qual:%d op status:%d, op qual:%d\n", __FUNCTION__,
                       status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    return status;
}

/*!***************************************************************
 * @fn fbe_api_cmi_service_clear_io_statistics()

 ****************************************************************
 * @brief
 *  This function gets general information related to CMI operations
 *
 * @param None
 *
 * @return
 *  fbe_status_t
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL
fbe_api_cmi_service_clear_io_statistics(void)
{
    fbe_status_t                                status = FBE_STATUS_OK;
    fbe_api_control_operation_status_info_t     status_info;
    fbe_trace_level_t                           trace_level = FBE_TRACE_LEVEL_ERROR;

    status = fbe_api_common_send_control_packet_to_service (FBE_CMI_CONTROL_CODE_CLEAR_IO_STATISTICS,
                                                            NULL, 
                                                            0,
                                                            FBE_SERVICE_ID_CMI,
                                                            FBE_PACKET_FLAG_NO_ATTRIB,
                                                            &status_info,
                                                            FBE_PACKAGE_ID_SEP_0);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK)
    {
        /* It perfectly valid for the CMI to be down.
        */
        if (status == FBE_STATUS_NOT_INITIALIZED)
        {
            trace_level = FBE_TRACE_LEVEL_DEBUG_LOW;
        }
        fbe_api_trace (trace_level, "%s:status:%d, qual:%d op status:%d, op qual:%d\n", __FUNCTION__,
                       status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    return status;
}

/*************************
 * end file fbe_api_cmi_interface.c
 *************************/
