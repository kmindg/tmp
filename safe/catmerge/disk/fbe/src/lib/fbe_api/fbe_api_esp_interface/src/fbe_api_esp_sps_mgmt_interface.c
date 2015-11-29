/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_api_esp_sps_mgmt_interface.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the sps_mgmt interface for the Environmental Services package.
 *
 * @ingroup fbe_api_esp_interface_class_files
 * @ingroup fbe_api_esp_sps_mgmt_interface
 *
 * @version
 *   03/16/2010:  Created. Joe Perry
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
#include "fbe/fbe_api_esp_sps_mgmt_interface.h"
#include "fbe/fbe_lun.h"


/*!***************************************************************
 * @fn fbe_api_esp_sps_mgmt_getSpsCount(fbe_u32_t *spsCount)
 ****************************************************************
 * @brief
 *  This function will send control code to retrieve SPS Count
 *  from the ESP SPS Mgmt object.
 *
 * @param 
 *   spsCount - SPS Count
 * 
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  02/30/12 - Created. Joe Perry
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_esp_sps_mgmt_getSpsCount(fbe_esp_sps_mgmt_spsCounts_t *spsCountInfo)
{
    fbe_object_id_t                                 object_id;
    fbe_status_t                                    status;
    fbe_api_control_operation_status_info_t         status_info;

    // get SPS Mgmt object ID
    status = fbe_api_esp_getObjectId(FBE_CLASS_ID_SPS_MGMT, &object_id);
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, 
            "%s:SPS Mgmt obj ID error:%d\n", 
            __FUNCTION__, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_common_send_control_packet (FBE_ESP_SPS_MGMT_CONTROL_CODE_GET_SPS_COUNT,
                                                 spsCountInfo,
                                                 sizeof(fbe_esp_sps_mgmt_spsCounts_t),
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

}   // end of fbe_api_esp_sps_mgmt_getSpsCount

/*!***************************************************************
 * @fn fbe_api_esp_sps_mgmt_getBobCount(fbe_u32_t *bobCount)
 ****************************************************************
 * @brief
 *  This function will send control code to retrieve BoB Count
 *  from the ESP SPS Mgmt object.
 *
 * @param 
 *   bobCount - BoB Count
 * 
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  04/19/12 - Created. Joe Perry
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_esp_sps_mgmt_getBobCount(fbe_u32_t *bobCount)
{
    fbe_object_id_t                                 object_id;
    fbe_status_t                                    status;
    fbe_api_control_operation_status_info_t         status_info;

    // get SPS Mgmt object ID
    status = fbe_api_esp_getObjectId(FBE_CLASS_ID_SPS_MGMT, &object_id);
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, 
            "%s:SPS Mgmt obj ID error:%d\n", 
            __FUNCTION__, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_common_send_control_packet (FBE_ESP_SPS_MGMT_CONTROL_CODE_GET_BOB_COUNT,
                                                          bobCount,
                                                          sizeof(fbe_u32_t),
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

}   // end of fbe_api_esp_sps_mgmt_getBobCount

/*!***************************************************************
 * @fn fbe_api_esp_sps_mgmt_getSpsStatus(fbe_esp_sps_mgmt_spsStatus_t *spsStatusInfo)
 ****************************************************************
 * @brief
 *  This function will send control code to retrieve SPS Status
 *  from the ESP SPS Mgmt object.
 *
 * @param 
 *   spsStatusInfo - SPS Status Info
 * 
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  03/16/10 - Created. Joe Perry
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_esp_sps_mgmt_getSpsStatus(fbe_esp_sps_mgmt_spsStatus_t *spsStatusInfo)
{
    fbe_object_id_t                                 object_id;
    fbe_status_t                                    status;
    fbe_api_control_operation_status_info_t         status_info;

    // get SPS Mgmt object ID
    status = fbe_api_esp_getObjectId(FBE_CLASS_ID_SPS_MGMT, &object_id);
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, 
            "%s:SPS Mgmt obj ID error:%d\n", 
            __FUNCTION__, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_common_send_control_packet (FBE_ESP_SPS_MGMT_CONTROL_CODE_GET_SPS_STATUS,
                                                          spsStatusInfo,
                                                          sizeof(fbe_esp_sps_mgmt_spsStatus_t),
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

}   // end of fbe_api_esp_sps_mgmt_getSpsStatus


/*!***************************************************************
 * @fn fbe_api_esp_sps_mgmt_getBobStatus(fbe_esp_sps_mgmt_bobStatus_t *bobStatusInfo)
 ****************************************************************
 * @brief
 *  This function will send control code to retrieve SPS Status
 *  from the ESP SPS Mgmt object.
 *
 * @param 
 *   spsStatusInfo - SPS Status Info
 * 
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  04/20/12 - Created. Joe Perry
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_esp_sps_mgmt_getBobStatus(fbe_esp_sps_mgmt_bobStatus_t *bobStatusInfo)
{
    fbe_object_id_t                                 object_id;
    fbe_status_t                                    status;
    fbe_api_control_operation_status_info_t         status_info;

    // get SPS Mgmt object ID
    status = fbe_api_esp_getObjectId(FBE_CLASS_ID_SPS_MGMT, &object_id);
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, 
            "%s:SPS Mgmt obj ID error:%d\n", 
            __FUNCTION__, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_common_send_control_packet (FBE_ESP_SPS_MGMT_CONTROL_CODE_GET_BOB_STATUS,
                                                          bobStatusInfo,
                                                          sizeof(fbe_esp_sps_mgmt_bobStatus_t),
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

}   // end of fbe_api_esp_sps_mgmt_getBobStatus

/*!***************************************************************
 * @fn fbe_api_esp_sps_mgmt_getObjectData(fbe_esp_sps_mgmt_bobStatus_t *bobStatusInfo)
 ****************************************************************
 * @brief
 *  This function will send control code to retrieve SPS Status
 *  from the ESP SPS Mgmt object.
 *
 * @param 
 *   spsMgmtObjectData - sps_mgmt object private data.
 * 
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  04/20/12 - Created. Joe Perry
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_esp_sps_mgmt_getObjectData(fbe_esp_sps_mgmt_objectData_t *spsMgmtObjectData)
{
    fbe_object_id_t                                 object_id;
    fbe_status_t                                    status;
    fbe_api_control_operation_status_info_t         status_info;

    // get SPS Mgmt object ID
    status = fbe_api_esp_getObjectId(FBE_CLASS_ID_SPS_MGMT, &object_id);
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, 
            "%s:SPS Mgmt obj ID error:%d\n", 
            __FUNCTION__, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_common_send_control_packet (FBE_ESP_SPS_MGMT_CONTROL_CODE_GET_OBJECT_DATA,
                                                 spsMgmtObjectData,
                                                 sizeof(fbe_esp_sps_mgmt_objectData_t),
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

}   // end of fbe_api_esp_sps_mgmt_getObjectData

/*!***************************************************************
 * @fn fbe_api_esp_sps_mgmt_getSpsTestTime(fbe_esp_sps_mgmt_spsStatus_t *spsStatusInfo)
 ****************************************************************
 * @brief
 *  This function will send control code to retrieve SPS Test
 *  Time from the ESP SPS Mgmt object.
 *
 * @param 
 *   spsStatusInfo - SPS Test Time Info
 * 
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  06/18/10 - Created. Joe Perry
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_esp_sps_mgmt_getSpsTestTime(fbe_esp_sps_mgmt_spsTestTime_t *spsTestTimeInfo)
{
    fbe_object_id_t                                 object_id;
    fbe_status_t                                    status;
    fbe_api_control_operation_status_info_t         status_info;

    // get SPS Mgmt object ID
    status = fbe_api_esp_getObjectId(FBE_CLASS_ID_SPS_MGMT, &object_id);
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, 
            "%s:SPS Mgmt obj ID error:%d\n", 
            __FUNCTION__, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_common_send_control_packet (FBE_ESP_SPS_MGMT_CONTROL_CODE_GET_SPS_TEST_TIME,
                                                 spsTestTimeInfo,
                                                 sizeof(fbe_esp_sps_mgmt_spsTestTime_t),
                                                 object_id,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &status_info,
                                                 FBE_PACKAGE_ID_ESP);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) 
    {
        if ( (status == FBE_STATUS_NOT_INITIALIZED) || (status == FBE_STATUS_COMPONENT_NOT_FOUND) )
        {
            fbe_api_trace (FBE_TRACE_LEVEL_DEBUG_HIGH,
                "%s:status:%d, qual:%d op status:%d, op qual:%d\n",
                __FUNCTION__, status,
                status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

            return status;
        }

        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, 
            "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", 
            __FUNCTION__, status, 
            status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;

}   // end of fbe_api_esp_sps_mgmt_getSpsTestTime


/*!***************************************************************
 * @fn fbe_api_esp_sps_mgmt_setSpsTestTime(fbe_esp_sps_mgmt_spsStatus_t *spsStatusInfo)
 ****************************************************************
 * @brief
 *  This function will send control code to set SPS Test
 *  Time from the ESP SPS Mgmt object.
 *
 * @param 
 *   spsStatusInfo - SPS Test Time Info
 * 
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  06/18/10 - Created. Joe Perry
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_esp_sps_mgmt_setSpsTestTime(fbe_esp_sps_mgmt_spsTestTime_t *spsTestTimeInfo)
{
    fbe_object_id_t                                 object_id;
    fbe_status_t                                    status;
    fbe_api_control_operation_status_info_t         status_info;

    // get SPS Mgmt object ID
    status = fbe_api_esp_getObjectId(FBE_CLASS_ID_SPS_MGMT, &object_id);
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, 
            "%s:SPS Mgmt obj ID error:%d\n", 
            __FUNCTION__, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_common_send_control_packet (FBE_ESP_SPS_MGMT_CONTROL_CODE_SET_SPS_TEST_TIME,
                                                 spsTestTimeInfo,
                                                 sizeof(fbe_esp_sps_mgmt_spsTestTime_t),
                                                 object_id,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &status_info,
                                                 FBE_PACKAGE_ID_ESP);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) 
    {
        if ( (status == FBE_STATUS_NOT_INITIALIZED) || (status == FBE_STATUS_COMPONENT_NOT_FOUND) )
        {
            fbe_api_trace (FBE_TRACE_LEVEL_DEBUG_HIGH,
                "%s:status:%d, qual:%d op status:%d, op qual:%d\n",
                __FUNCTION__, status,
                status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

            return status;
        }

        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, 
            "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", 
            __FUNCTION__, status, 
            status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;

}   // end of fbe_api_esp_sps_mgmt_setSpsTestTime


/*!***************************************************************
 * @fn fbe_api_esp_sps_mgmt_getSpsInputPower(fbe_esp_sps_mgmt_spsInputPower_t *spsInputPowerInfo)
 ****************************************************************
 * @brief
 *  This function will send control code to retrieve SPS Input
 *  Power from the ESP SPS Mgmt object.
 *
 * @param 
 *   spsInputPowerInfo - SPS Input Power Info
 * 
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  06/18/10 - Created. Joe Perry
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_esp_sps_mgmt_getSpsInputPower(fbe_esp_sps_mgmt_spsInputPower_t *spsInputPowerInfo)
{
    fbe_object_id_t                                 object_id;
    fbe_status_t                                    status;
    fbe_api_control_operation_status_info_t         status_info;

    // get SPS Mgmt object ID
    status = fbe_api_esp_getObjectId(FBE_CLASS_ID_SPS_MGMT, &object_id);
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, 
            "%s:SPS Mgmt obj ID error:%d\n", 
            __FUNCTION__, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_common_send_control_packet (FBE_ESP_SPS_MGMT_CONTROL_CODE_GET_SPS_INPUT_POWER,
                                                          spsInputPowerInfo,
                                                          sizeof(fbe_esp_sps_mgmt_spsInputPower_t),
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

}   // end of fbe_api_esp_sps_mgmt_getSpsInputPower

/*!***************************************************************
 * @fn fbe_api_esp_sps_mgmt_getSpsInputPowerTotal(fbe_esp_sps_mgmt_spsInputPowerTotal_t *spsInputPowerInfo)
 ****************************************************************
 * @brief
 *  This function will send control code to retrieve total SPS
 *  Input Power from the ESP SPS Mgmt object.
 *
 * @param 
 *   spsInputPowerInfo - SPS Input Power Info
 * 
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  01/18/11 - Created. Joe Perry
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_esp_sps_mgmt_getSpsInputPowerTotal(fbe_esp_sps_mgmt_spsInputPowerTotal_t *spsInputPowerInfo)
{
    fbe_object_id_t                                 object_id;
    fbe_status_t                                    status;
    fbe_api_control_operation_status_info_t         status_info;

    // get SPS Mgmt object ID
    status = fbe_api_esp_getObjectId(FBE_CLASS_ID_SPS_MGMT, &object_id);
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, 
            "%s:SPS Mgmt obj ID error:%d\n", 
            __FUNCTION__, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_common_send_control_packet (FBE_ESP_SPS_MGMT_CONTROL_CODE_GET_SPS_INPUT_POWER_TOTAL,
                                                          spsInputPowerInfo,
                                                          sizeof(fbe_esp_sps_mgmt_spsInputPowerTotal_t),
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

}   // end of fbe_api_esp_sps_mgmt_getSpsInputPowerTotal

/*!***************************************************************
 * @fn fbe_api_esp_sps_mgmt_getManufInfo(fbe_esp_sps_mgmt_spsManufInfo_t *spsManufInfo)
 ****************************************************************
 * @brief
 *  This function will send control code to retrieve SPS ManufInfo
 *  from the ESP SPS Mgmt object.
 *
 * @param 
 *   spsManufInfo - SPS Manuf Info
 * 
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  04/08/10 - Created. Vishnu Sharma
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_esp_sps_mgmt_getManufInfo(fbe_esp_sps_mgmt_spsManufInfo_t *spsManufInfo)
{
    fbe_object_id_t                            object_id;
    fbe_status_t                               status;
    fbe_api_control_operation_status_info_t    status_info;

    // get SPS Mgmt object ID
    status = fbe_api_esp_getObjectId(FBE_CLASS_ID_SPS_MGMT, &object_id);
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, 
            "%s:SPS Mgmt obj ID error:%d\n", 
            __FUNCTION__, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_common_send_control_packet (FBE_ESP_SPS_MGMT_CONTROL_CODE_GET_SPS_MANUF_INFO,
                                                 spsManufInfo,
                                                 sizeof(fbe_esp_sps_mgmt_spsManufInfo_t),
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

}   // end of fbe_api_esp_sps_mgmt_getManufInfo


/*!***************************************************************
 * @fn fbe_api_esp_sps_mgmt_getCacheStatus(void)
 ****************************************************************
 * @brief
 *  This function will send control code to retrieve SPS caching
 *  status from the ESP SPS Mgmt object.
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
fbe_api_esp_sps_mgmt_getCacheStatus(fbe_common_cache_status_t *cacheStatus)
{
    fbe_object_id_t                            object_id;
    fbe_status_t                               status;
    fbe_api_control_operation_status_info_t    status_info;

    // get SPS Mgmt object ID
    status = fbe_api_esp_getObjectId(FBE_CLASS_ID_SPS_MGMT, &object_id);
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, 
            "%s:SPS Mgmt obj ID error:%d\n", 
            __FUNCTION__, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_common_send_control_packet (FBE_ESP_SPS_MGMT_CONTROL_CODE_GET_SPS_CACHE_STATUS,
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

}   // end of fbe_api_esp_sps_mgmt_getCacheStatus


/*!***************************************************************
 * @fn fbe_api_esp_sps_mgmt_powerdown(void)
 ****************************************************************
 * @brief
 *  This function will send control code to powerdown SPS to
 *  the ESP SPS Mgmt object.
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
fbe_status_t FBE_API_CALL fbe_api_esp_sps_mgmt_powerdown(void)
{
    fbe_object_id_t                            object_id;
    fbe_status_t                               status;
    fbe_api_control_operation_status_info_t    status_info;


    /* 
     *  LUN objects normally do a "lazy clean" at least 5 seconds after the last non-cached write
     *  is processed. Since we're about to shut off the power, we want to give the LUNs a chance to
     *  mark themselves clean before we do. In particular, it's very likely that we've just done a
     *  vault dump, and we want to prevent unnecessary background verifies on the Vault and CDR LUNs.
     */
    fbe_api_trace (FBE_TRACE_LEVEL_INFO, 
        "%s, send LUN Class Prepare for Shutdown.\n", 
        __FUNCTION__);
    status = fbe_api_common_send_control_packet_to_class(FBE_LUN_CONTROL_CODE_CLASS_PREPARE_FOR_POWER_SHUTDOWN,
                                                         NULL,
                                                         0,
                                                         FBE_CLASS_ID_LUN,
                                                         FBE_PACKET_FLAG_NO_ATTRIB, 
                                                         &status_info,
                                                         FBE_PACKAGE_ID_SEP_0);
    if(status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, 
            "%s LUN Class Prepare for Shutdown failed: %d\n", 
            __FUNCTION__, status);
        return status;
    }
    else
    {
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, 
            "%s, LUN Class Prepare for Shutdown completed.\n", 
            __FUNCTION__);
    }

    // get SPS Mgmt object ID
    status = fbe_api_esp_getObjectId(FBE_CLASS_ID_SPS_MGMT, &object_id);
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, 
            "%s:SPS Mgmt obj ID error:%d\n", 
            __FUNCTION__, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_api_trace (FBE_TRACE_LEVEL_INFO, 
        "%s, send SPS Powerdown.\n", 
        __FUNCTION__);
    status = fbe_api_common_send_control_packet (FBE_ESP_SPS_MGMT_CONTROL_CODE_SPS_POWERDOWN,
                                                 NULL,
                                                 0,
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

    fbe_api_trace (FBE_TRACE_LEVEL_INFO, 
        "%s, SPS Powerdown completed.\n", 
        __FUNCTION__);


    return status;

}   // end of fbe_api_esp_sps_mgmt_powerdown

/*!***************************************************************
 * @fn fbe_api_esp_sps_mgmt_verifyPowerdown(void)
 ****************************************************************
 * @brief
 *  This function will send control code to the ESP SPS Mgmt
 *  object to verify that SPS's have been powered down.
 *
 * @param
 *   void
 * 
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  08/13/12 - Created. Joe Perry
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_esp_sps_mgmt_verifyPowerdown(void)
{
    fbe_object_id_t                            object_id;
    fbe_status_t                               status;
    fbe_api_control_operation_status_info_t    status_info;

    // get SPS Mgmt object ID
    status = fbe_api_esp_getObjectId(FBE_CLASS_ID_SPS_MGMT, &object_id);
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, 
            "%s:SPS Mgmt obj ID error:%d\n", 
            __FUNCTION__, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_common_send_control_packet (FBE_ESP_SPS_MGMT_CONTROL_CODE_SPS_VERIFY_POWERDOWN,
                                                 NULL,
                                                 0,
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

}   // end of fbe_api_esp_sps_mgmt_verifyPowerdown


/*!***************************************************************
 * @fn fbe_api_esp_sps_mgmt_getSimulateSps(fbe_bool_t *simulateSps)
 ****************************************************************
 * @brief
 *  This function will send control code to get SimulateSPS
 *  status from the ESP SPS Mgmt object.
 *
 * @param
 *   *simulateSps - pointer to simulateSPS status
 * 
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  11/10/11 - Created. Joe Perry
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_esp_sps_mgmt_getSimulateSps(fbe_bool_t *simulateSps)
{
    fbe_object_id_t                            object_id;
    fbe_status_t                               status;
    fbe_api_control_operation_status_info_t    status_info;

    // get SPS Mgmt object ID
    status = fbe_api_esp_getObjectId(FBE_CLASS_ID_SPS_MGMT, &object_id);
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, 
            "%s:SPS Mgmt obj ID error:%d\n", 
            __FUNCTION__, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_common_send_control_packet (FBE_ESP_SPS_MGMT_CONTROL_CODE_GET_SIMULATE_SPS,
                                                 simulateSps,
                                                 sizeof(fbe_bool_t),
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

}   // end of fbe_api_esp_sps_mgmt_getSimulateSps


/*!***************************************************************
 * @fn fbe_api_esp_sps_mgmt_setSimulateSps(fbe_bool_t simulateSps)
 ****************************************************************
 * @brief
 *  This function will send control code to set SimulateSPS
 *  status from the ESP SPS Mgmt object.
 *
 * @param
 *   simulateSps - simulateSPS status
 * 
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  11/10/11 - Created. Joe Perry
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_esp_sps_mgmt_setSimulateSps(fbe_bool_t simulateSps)
{
    fbe_object_id_t                            object_id;
    fbe_status_t                               status;
    fbe_api_control_operation_status_info_t    status_info;

    // get SPS Mgmt object ID
    status = fbe_api_esp_getObjectId(FBE_CLASS_ID_SPS_MGMT, &object_id);
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, 
            "%s:SPS Mgmt obj ID error:%d\n", 
            __FUNCTION__, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_common_send_control_packet (FBE_ESP_SPS_MGMT_CONTROL_CODE_SET_SIMULATE_SPS,
                                                 &simulateSps,
                                                 sizeof(fbe_bool_t),
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

}   // end of fbe_api_esp_sps_mgmt_setSimulateSps

/*!***************************************************************
 * @fn fbe_api_esp_sps_mgmt_getSpsInputPowerSample(
 *           fbe_esp_sps_mgmt_spsInputPowerSample_t *spsSampleInfo)
 ****************************************************************
 * @brief
 *  This function will send control code to retrieve SPS Input
 *  Power sample from the ESP SPS Mgmt object.
 *
 * @param
 *   spsSampleInfo - SPS Input Power sample Info
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  05/09/12 - Created. Dongz
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL
fbe_api_esp_sps_mgmt_getSpsInputPowerSample(fbe_esp_sps_mgmt_spsInputPowerSample_t *spsSampleInfo)
{
    fbe_object_id_t                                 object_id;
    fbe_status_t                                    status;
    fbe_api_control_operation_status_info_t         status_info;

    // get SPS Mgmt object ID
    status = fbe_api_esp_getObjectId(FBE_CLASS_ID_SPS_MGMT, &object_id);
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR,
            "%s:SPS Mgmt obj ID error:%d\n",
            __FUNCTION__, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_common_send_control_packet (FBE_ESP_SPS_MGMT_CONTROL_CODE_SET_SPS_INPUT_POWER_SAMPLE,
                                                          spsSampleInfo,
                                                          sizeof(fbe_esp_sps_mgmt_spsInputPowerSample_t),
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

}   // end of fbe_api_esp_sps_mgmt_getSpsInputPowerSample


/*!***************************************************************
 * @fn fbe_api_esp_sps_mgmt_getLocalBatteryTime(
 *           fbe_u32_t *localbatteryTime)
 ****************************************************************
 * @brief
 *  This function will send control code to retrieve the local
 *  Battery Time.
 *
 * @param
 *   localBatteryTime - pointer to local battery time value
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  07/11/12 - Created. Joe Perry
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL
fbe_api_esp_sps_mgmt_getLocalBatteryTime(fbe_u32_t *localBatteryTime)
{
    fbe_object_id_t                                 object_id;
    fbe_status_t                                    status;
    fbe_api_control_operation_status_info_t         status_info;

    // get SPS Mgmt object ID
    status = fbe_api_esp_getObjectId(FBE_CLASS_ID_SPS_MGMT, &object_id);
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR,
            "%s:SPS Mgmt obj ID error:%d\n",
            __FUNCTION__, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_common_send_control_packet (FBE_ESP_SPS_MGMT_CONTROL_CODE_GET_LOCAL_BATTERY_TIME,
                                                          localBatteryTime,
                                                          sizeof(fbe_u32_t),
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

}   // end of fbe_api_esp_sps_mgmt_getLocalBatteryTime


/*!***************************************************************
 * @fn fbe_api_esp_sps_mgmt_initiateSelfTest(
 *           fbe_u32_t *localbatteryTime)
 ****************************************************************
 * @brief
 *  This function will send control code to initiate a Self Test
 *  on Battery.
 *
 * @param
 *   void
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  12/20/12 - Created. Joe Perry
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL
fbe_api_esp_sps_mgmt_initiateSelfTest(void)
{
    fbe_object_id_t                                 object_id;
    fbe_status_t                                    status;
    fbe_api_control_operation_status_info_t         status_info;

    // get SPS Mgmt object ID
    status = fbe_api_esp_getObjectId(FBE_CLASS_ID_SPS_MGMT, &object_id);
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR,
            "%s:SPS Mgmt obj ID error:%d\n",
            __FUNCTION__, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_common_send_control_packet (FBE_ESP_SPS_MGMT_CONTROL_CODE_INITIATE_SELF_TEST,
                                                          NULL,
                                                          0,
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

}   // end of fbe_api_esp_sps_mgmt_initiateSelfTest


/*************************
 * end file fbe_api_esp_sps_mgmt_interface.c
 *************************/

