/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_api_esp_ps_mgmt_interface.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the sps_mgmt interface for the Environmental Services package.
 *
 * @ingroup fbe_api_esp_interface_class_files
 * @ingroup fbe_api_esp_ps_mgmt_interface
 *
 * @version
 *   04/16/2010:  Created. Joe Perry
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

#include "fbe/fbe_winddk.h"
#include "fbe/fbe_types.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_common_transport.h"
#include "fbe/fbe_api_esp_ps_mgmt_interface.h"


/*!***************************************************************
 * @fn fbe_api_esp_ps_mgmt_getPsCount(fbe_device_physical_location_t * pLocation,
 *                                    fbe_u32_t * pPsCount)
 ****************************************************************
 * @brief
 *  This function will send control code to retrieve PS Status
 *  from the ESP PS Mgmt object.
 *
 * @param 
 *   pLocation (INPUT) - 
 *   pPsCount (OUTPUT) -
 * 
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  03/16/10 - Created. Joe Perry
 *  08/25/10 PHE - added the input parameter pLocation.
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_esp_ps_mgmt_getPsCount(fbe_device_physical_location_t * pLocation,
                               fbe_u32_t * pPsCount)
{
    fbe_object_id_t                                 object_id;
    fbe_status_t                                    status;
    fbe_api_control_operation_status_info_t         status_info;
    fbe_esp_ps_mgmt_ps_count_t                      getPsCount = {0};

    // get PS Mgmt object ID
    status = fbe_api_esp_getObjectId(FBE_CLASS_ID_PS_MGMT, &object_id);
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, 
            "%s:PS Mgmt obj ID error:%d\n", 
            __FUNCTION__, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    getPsCount.location = *pLocation;

    status = fbe_api_common_send_control_packet (FBE_ESP_PS_MGMT_CONTROL_CODE_GET_PS_COUNT,
                                                 &getPsCount,
                                                 sizeof(fbe_esp_ps_mgmt_ps_count_t),
                                                 object_id,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &status_info,
                                                 FBE_PACKAGE_ID_ESP);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) 
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, 
            "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", 
            __FUNCTION__, status, 
            status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    *pPsCount = getPsCount.psCount;
    return status;

}   // end of fbe_api_esp_ps_mgmt_getPsCount

/*!***************************************************************
 * @fn fbe_api_esp_ps_mgmt_getPsInfo(fbe_esp_ps_mgmt_ps_info_t *psInfo)
 ****************************************************************
 * @brief
 *  This function will send control code to retrieve PS Status
 *  from the ESP PS Mgmt object.
 *
 * @param 
 *   spsStatusInfo - PS Status Info
 * 
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  03/16/10 - Created. Joe Perry
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_esp_ps_mgmt_getPsInfo(fbe_esp_ps_mgmt_ps_info_t *psInfo)
{
    fbe_object_id_t                                 object_id;
    fbe_status_t                                    status;
    fbe_api_control_operation_status_info_t         status_info;

    // get PS Mgmt object ID
    status = fbe_api_esp_getObjectId(FBE_CLASS_ID_PS_MGMT, &object_id);
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, 
            "%s:PS Mgmt obj ID error:%d\n", 
            __FUNCTION__, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_common_send_control_packet (FBE_ESP_PS_MGMT_CONTROL_CODE_GET_PS_STATUS,
                                                 psInfo,
                                                 sizeof(fbe_esp_ps_mgmt_ps_info_t),
                                                 object_id,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &status_info,
                                                 FBE_PACKAGE_ID_ESP);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) 
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, 
            "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", 
            __FUNCTION__, status, 
            status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;

}   // end of fbe_api_esp_ps_mgmt_getPsInfo


/*!***************************************************************
 * @fn fbe_api_esp_ps_mgmt_getCacheStatus(fbe_common_cache_status_t *cacheStatus);
 ****************************************************************
 * @brief
 *  This function will send control code to retrieve Power Fail
 *  caching status from the ESP PS Mgmt object.
 *
 * @param
 *   void
 * 
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  03/22/11 - Created. Joe Perry
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_esp_ps_mgmt_getCacheStatus(fbe_common_cache_status_t *cacheStatus)
{
    fbe_object_id_t                                 object_id;
    fbe_status_t                                    status;
    fbe_api_control_operation_status_info_t         status_info;

    // get PS Mgmt object ID
    status = fbe_api_esp_getObjectId(FBE_CLASS_ID_PS_MGMT, &object_id);
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, 
            "%s:PS Mgmt obj ID error:%d\n", 
            __FUNCTION__, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_common_send_control_packet (FBE_ESP_PS_MGMT_CONTROL_CODE_GET_PS_CACHE_STATUS,
                                                 cacheStatus,
                                                 sizeof(fbe_common_cache_status_t),
                                                 object_id,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &status_info,
                                                 FBE_PACKAGE_ID_ESP);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) 
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, 
            "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", 
            __FUNCTION__, status, 
            status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        if (status == FBE_STATUS_BUSY)
        {
            return status;
        }

        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;

}   // end of fbe_api_esp_sps_mgmt_getPsCacheStatus


/*!***************************************************************
 * @fn fbe_api_esp_ps_mgmt_powerdown(void);
 ****************************************************************
 * @brief
 *  This function will send control code to powerdown PS to
 *  the ESP PS Mgmt object.
 *
 * @param
 *   void
 * 
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  04/07/11 - Created. Joe Perry
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_esp_ps_mgmt_powerdown(void)
{
    fbe_object_id_t                                 object_id;
    fbe_status_t                                    status;
    fbe_api_control_operation_status_info_t         status_info;
	fbe_common_cache_status_t           			psCacheStatus;

    // get PS Mgmt object ID
    status = fbe_api_esp_getObjectId(FBE_CLASS_ID_PS_MGMT, &object_id);
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, 
            "%s:PS Mgmt obj ID error:%d\n", 
            __FUNCTION__, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

	psCacheStatus = FBE_CACHE_STATUS_FAILED;

    status = fbe_api_common_send_control_packet (FBE_ESP_PS_MGMT_CONTROL_CODE_PS_POWERDOWN,
                                                 &psCacheStatus,
                                                 sizeof(fbe_common_cache_status_t),
                                                 object_id,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &status_info,
                                                 FBE_PACKAGE_ID_ESP);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) 
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, 
            "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", 
            __FUNCTION__, status, 
            status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;

}   // end of fbe_api_esp_ps_mgmt_powerdown

/*!***************************************************************
 * @fn fbe_api_esp_ps_mgmt_set_expected_ps_type(void);
 ****************************************************************
 * @brief
 *  This function will send control code to set the expected power
 *  supply type. 
 * @param
 *   void
 * 
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  10/30/13 - Created. Brion Philbin
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_esp_ps_mgmt_set_expected_ps_type(fbe_ps_mgmt_expected_ps_type_t expected_ps_type)
{
    fbe_object_id_t                                 object_id;
    fbe_status_t                                    status;
    fbe_api_control_operation_status_info_t         status_info;

    // get PS Mgmt object ID
    status = fbe_api_esp_getObjectId(FBE_CLASS_ID_PS_MGMT, &object_id);
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, 
            "%s:PS Mgmt obj ID error:%d\n", 
            __FUNCTION__, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_common_send_control_packet (FBE_ESP_PS_MGMT_CONTROL_CODE_SET_EXPECTED_PS_TYPE,
                                                 &expected_ps_type,
                                                 sizeof(fbe_ps_mgmt_expected_ps_type_t),
                                                 object_id,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &status_info,
                                                 FBE_PACKAGE_ID_ESP);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) 
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, 
            "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", 
            __FUNCTION__, status, 
            status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;

}   // end of fbe_api_esp_ps_mgmt_powerdown

/*************************
 * end file fbe_api_esp_ps_mgmt_interface.c
 *************************/
