/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_api_esp_board_mgmt_interface.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the board_mgmt interface for the Environmental Services package.
 *
 * @ingroup fbe_api_esp_interface_class_files
 * @ingroup fbe_api_esp_board_mgmt_interface
 *
 * @version
 *  05-Aug-2010 : Created  Vaibhav Gaonkar
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_types.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_common_transport.h"
#include "fbe/fbe_api_esp_board_mgmt_interface.h"
#include "fbe/fbe_esp_board_mgmt.h"


/*************************
 *   FUNCTION DEFINITIONS
 *************************/
/*!***************************************************************
 * @fn fbe_api_esp_board_mgmt_getBoardInfo(fbe_esp_sps_mgmt_spsStatus_t *spsStatusInfo)
 ****************************************************************
 * @brief
 *  This function will send control code to retrieve board mgmt info
 *  from the ESP board Mgmt object.
 *
 * @param 
 *   board_info - handle to buffer to hold board info
 * 
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  05-Aug-2010 : Created  Vaibhav Gaonkar
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_esp_board_mgmt_getBoardInfo(fbe_esp_board_mgmt_board_info_t *board_info)
{
    fbe_object_id_t                                 object_id;
    fbe_status_t                                    status;
    fbe_api_control_operation_status_info_t         status_info;

    // get Board Mgmt object ID
    status = fbe_api_esp_getObjectId(FBE_CLASS_ID_BOARD_MGMT, &object_id);
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, 
            "%s:Board Mgmt obj ID error:%d\n", 
            __FUNCTION__, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_common_send_control_packet (FBE_ESP_BOARD_MGMT_CONTROL_CODE_GET_BOARD_INFO,
                                                 board_info,
                                                 sizeof(fbe_esp_board_mgmt_board_info_t),
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
}
/*************************************************
*   end of fbe_api_esp_board_mgmt_getBoardInfo()
*************************************************/

/*!***************************************************************
 * @fn fbe_api_esp_board_mgmt_reboot_sp(fbe_board_mgmt_set_PostAndOrReset_t *post_reset)
 ****************************************************************
 * @brief
 *  This function will send control code to reboot the sp with options
 *  to hold in post or reset.
 *
 * @param 
 *   post_reset - reboot options for hold in post or reset
 * 
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  04-March-2011 : Created  bphilbin
 * 
 ****************************************************************/
fbe_status_t FBE_API_CALL
fbe_api_esp_board_mgmt_reboot_sp(fbe_board_mgmt_set_PostAndOrReset_t *post_reset)
{
    fbe_object_id_t                                 object_id;
    fbe_status_t                                    status;
    fbe_api_control_operation_status_info_t         status_info;

    // get Board Mgmt object ID
    status = fbe_api_esp_getObjectId(FBE_CLASS_ID_BOARD_MGMT, &object_id);
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, 
            "%s:Board Mgmt obj ID error:%d\n", 
            __FUNCTION__, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Set Status Retry count    */
    post_reset->retryStatusCount = POST_AND_OR_RESET_STATUS_RETRY_COUNT;


    status = fbe_api_common_send_control_packet (FBE_ESP_BOARD_MGMT_CONTROL_CODE_REBOOT_SP,
                                                 post_reset,
                                                 sizeof(fbe_board_mgmt_set_PostAndOrReset_t),
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
}


/*!***************************************************************
 * @fn fbe_api_esp_board_mgmt_flushSystem(void)
 ****************************************************************
 * @brief
 *  This function will send control code to flush the system
 *  (file system, registry, ...).
 *
 * @param 
 *   post_reset - reboot options for hold in post or reset
 * 
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  19-March-2013 : Created  Joe Perry
 * 
 ****************************************************************/
fbe_status_t FBE_API_CALL
fbe_api_esp_board_mgmt_flushSystem(void)
{
    fbe_object_id_t                                 object_id;
    fbe_status_t                                    status;
    fbe_api_control_operation_status_info_t         status_info;

    // get Board Mgmt object ID
    status = fbe_api_esp_getObjectId(FBE_CLASS_ID_BOARD_MGMT, &object_id);
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, 
            "%s:Board Mgmt obj ID error:%d\n", 
            __FUNCTION__, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_common_send_control_packet (FBE_ESP_BOARD_MGMT_CONTROL_CODE_FLUSH_SYSTEM,
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
}


/*!***************************************************************
 * @fn fbe_api_esp_board_mgmt_getCacheStatus(void)
 ****************************************************************
 * @brief
 *  This function will send control code to retrieve Board caching
 *  status from the ESP Board Mgmt object.
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
fbe_api_esp_board_mgmt_getCacheStatus(fbe_common_cache_status_t *cacheStatus)
{
    fbe_object_id_t                            object_id;
    fbe_status_t                               status;
    fbe_api_control_operation_status_info_t    status_info;

    // get Board Mgmt object ID
    status = fbe_api_esp_getObjectId(FBE_CLASS_ID_BOARD_MGMT, &object_id);
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, 
            "%s:Board Mgmt obj ID error:%d\n", 
            __FUNCTION__, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_common_send_control_packet (FBE_ESP_BOARD_MGMT_CONTROL_CODE_GET_CACHE_STATUS,
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

}   // end of fbe_api_esp_board_mgmt_getCacheStatus
/*!***************************************************************
 * @fn fbe_api_esp_board_mgmt_getPeerBootInfo()
 ****************************************************************
 * @brief
 *  This function will send control code to retrieve peer SP booting
 *  state
 *
 * @param 
 *   peerBootInfo - buffer to return peer boot info
 * 
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  21-Feb-2011 : Created  Vaibhav Gaonkar
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_esp_board_mgmt_getPeerBootInfo(fbe_esp_board_mgmt_peer_boot_info_t 
                                       *peerBootInfo)
{
    fbe_status_t    status;
    fbe_object_id_t object_id;
    fbe_api_control_operation_status_info_t status_info;

    // get Board Mgmt object ID
    status = fbe_api_esp_getObjectId(FBE_CLASS_ID_BOARD_MGMT, &object_id);
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, 
            "%s:Board Mgmt obj ID error:%d\n", 
            __FUNCTION__, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_common_send_control_packet (FBE_ESP_BOARD_MGMT_CONTROL_CODE_GET_PEER_BOOT_INFO,
                                                 peerBootInfo,
                                                 sizeof(fbe_esp_board_mgmt_peer_boot_info_t),
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
}
/*************************************************
*   end of fbe_api_esp_board_mgmt_getPeerBootInfo()
*************************************************/
/*!***************************************************************
 * @fn fbe_api_esp_board_mgmt_get_cpu_info()
 ****************************************************************
 * @brief
 *  This function will send control code to retrieve peer SP CPU 
 *  status
 *
 * @param 
 *   peerBootInfo - buffer to return peer CPU status
 * 
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  3-Aug-2012 : Chengkai Hu - Created
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_esp_board_mgmt_getPeerCpuStatus(fbe_bool_t *cpuStatus)
{
    fbe_status_t    status;
    fbe_object_id_t object_id;
    fbe_api_control_operation_status_info_t status_info;

    // get Board Mgmt object ID
    status = fbe_api_esp_getObjectId(FBE_CLASS_ID_BOARD_MGMT, &object_id);
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, 
            "%s:Board Mgmt obj ID error:%d\n", 
            __FUNCTION__, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_common_send_control_packet (FBE_ESP_BOARD_MGMT_CONTROL_CODE_GET_PEER_CPU_STATUS,
                                                 cpuStatus,
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
}
/*************************************************
*   end of fbe_api_esp_board_mgmt_get_cpu_info()
*************************************************/
/*!***************************************************************
 * @fn fbe_api_esp_board_mgmt_get_dimm_info()
 ****************************************************************
 * @brief
 *  This function will send control code to retrieve peer SP DIMM 
 *  status
 *
 * @param 
 *   peerBootInfo - buffer to return peer DIMM status
 * 
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  3-Aug-2012 : Chengkai Hu - Created
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_esp_board_mgmt_getPeerDimmStatus(fbe_bool_t *dimmStatus, fbe_u32_t size)
{
    fbe_status_t    status;
    fbe_object_id_t object_id;
    fbe_api_control_operation_status_info_t status_info;

    // get Board Mgmt object ID
    status = fbe_api_esp_getObjectId(FBE_CLASS_ID_BOARD_MGMT, &object_id);
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, 
            "%s:Board Mgmt obj ID error:%d\n", 
            __FUNCTION__, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_common_send_control_packet (FBE_ESP_BOARD_MGMT_CONTROL_CODE_GET_PEER_DIMM_STATUS,
                                                 dimmStatus,
                                                 size,
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
}
/*************************************************
*   end of fbe_api_esp_board_mgmt_get_dimm_info()
*************************************************/

/*!***************************************************************
 * fbe_api_esp_board_mgmt_getSuitcaseInfo()
 ****************************************************************
 * @brief
 *  This function will return Suitcase info.
 *
 * @param 
 *   suitcaseInfo - pointer to Suitcase info structure
 * 
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  19-Oct-2011 : Created  Joe Perry
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_esp_board_mgmt_getSuitcaseInfo(fbe_esp_board_mgmt_suitcaseInfo_t *suitcaseInfo)
{
    fbe_status_t        status;
    fbe_object_id_t     object_id;
    fbe_api_control_operation_status_info_t     status_info;

    /* get Board Mgmt object ID */
    status = fbe_api_esp_getObjectId(FBE_CLASS_ID_BOARD_MGMT, &object_id);
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, 
            "%s:Board Mgmt obj ID error:%d\n", 
            __FUNCTION__, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_common_send_control_packet (FBE_ESP_BOARD_MGMT_CONTROL_CODE_GET_SUITCASE_INFO,
                                                 suitcaseInfo,
                                                 sizeof(fbe_esp_board_mgmt_suitcaseInfo_t),
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
}
/*************************************************
*   end of fbe_api_esp_board_mgmt_getSuitcaseInfo()
*************************************************/

/*!***************************************************************
 * fbe_api_esp_board_mgmt_getBmcInfo()
 ****************************************************************
 * @brief
 *  This function will return BMC info.
 *
 * @param 
 *   suitcaseInfo - pointer to BMC info structure
 * 
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  14-Sep-2012 : Eric Zhou     Created 
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_esp_board_mgmt_getBmcInfo(fbe_esp_board_mgmt_bmcInfo_t *bmcInfo)
{
    fbe_status_t        status;
    fbe_object_id_t     object_id;
    fbe_api_control_operation_status_info_t     status_info;

    /* get Board Mgmt object ID */
    status = fbe_api_esp_getObjectId(FBE_CLASS_ID_BOARD_MGMT, &object_id);
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, 
            "%s:Board Mgmt obj ID error:%d\n", 
            __FUNCTION__, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_common_send_control_packet (FBE_ESP_BOARD_MGMT_CONTROL_CODE_GET_BMC_INFO,
                                                 bmcInfo,
                                                 sizeof(fbe_esp_board_mgmt_bmcInfo_t),
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
}
/*************************************************
*   end of fbe_api_esp_board_mgmt_getBmcInfo()
*************************************************/

/*!***************************************************************
 * @fn fbe_api_esp_board_mgmt_getCacheCardCount(void)
 ****************************************************************
 * @brief
 *  This function will send control code to retrieve Cache
 *  Card count from the ESP Board Mgmt object.
 *
 * @param
 *   void
 * 
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  02/17/13 - Created. Rui Chang
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_esp_board_mgmt_getCacheCardCount(fbe_u32_t * pCount)
{
    fbe_object_id_t                            object_id;
    fbe_status_t                               status;
    fbe_api_control_operation_status_info_t    status_info;

    // get Board Mgmt object ID
    status = fbe_api_esp_getObjectId(FBE_CLASS_ID_BOARD_MGMT, &object_id);
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, 
            "%s:Board Mgmt obj ID error:%d\n", 
            __FUNCTION__, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_common_send_control_packet (FBE_ESP_BOARD_MGMT_CONTROL_CODE_GET_CACHE_CARD_COUNT,
                                                 pCount,
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

}   // end of fbe_api_esp_board_mgmt_getCacheStatus

/*!***************************************************************
 * @fn fbe_api_esp_board_mgmt_getCacheCardStatus(void)
 ****************************************************************
 * @brief
 *  This function will send control code to retrieve Cache
 *  Card status from the ESP Board Mgmt object.
 *
 * @param
 *   void
 * 
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  17/02/13 - Created. Rui Chang
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_esp_board_mgmt_getCacheCardStatus(fbe_board_mgmt_cache_card_info_t *cacheCardStatus)
{
    fbe_object_id_t                            object_id;
    fbe_status_t                               status;
    fbe_api_control_operation_status_info_t    status_info;

    // get Board Mgmt object ID
    status = fbe_api_esp_getObjectId(FBE_CLASS_ID_BOARD_MGMT, &object_id);
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, 
            "%s:Board Mgmt obj ID error:%d\n", 
            __FUNCTION__, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_common_send_control_packet (FBE_ESP_BOARD_MGMT_CONTROL_CODE_GET_CACHE_CARD_STATUS,
                                                 cacheCardStatus,
                                                 sizeof(fbe_board_mgmt_cache_card_info_t),
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

}   // end of fbe_api_esp_board_mgmt_getCacheStatus

/*!***************************************************************
 * @fn fbe_api_esp_board_mgmt_getDimmCount(void)
 ****************************************************************
 * @brief
 *  This function will send control code to retrieve DIMM
 *  count from the ESP Board Mgmt object.
 *
 * @param
 *   void
 * 
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  05/17/13 - Created. Rui Chang
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_esp_board_mgmt_getDimmCount(fbe_u32_t * pCount)
{
    fbe_object_id_t                            object_id;
    fbe_status_t                               status;
    fbe_api_control_operation_status_info_t    status_info;

    // get Board Mgmt object ID
    status = fbe_api_esp_getObjectId(FBE_CLASS_ID_BOARD_MGMT, &object_id);
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, 
            "%s:Board Mgmt obj ID error:%d\n", 
            __FUNCTION__, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_common_send_control_packet (FBE_ESP_BOARD_MGMT_CONTROL_CODE_GET_DIMM_COUNT,
                                                 pCount,
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

}   // end of fbe_api_esp_board_mgmt_getDimmCount

/*!***************************************************************
 * @fn fbe_api_esp_board_mgmt_getDimmInfo(void)
 ****************************************************************
 * @brief
 *  This function will send control code to retrieve DIMM
 *  status from the ESP Board Mgmt object.
 *
 * @param
 *   void
 * 
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  05/17/13 - Created. Rui Chang
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_esp_board_mgmt_getDimmInfo(fbe_device_physical_location_t * pLocation,
                                   fbe_esp_board_mgmt_dimm_info_t * pDimmInfo)
{
    fbe_object_id_t                            object_id;
    fbe_status_t                               status;
    fbe_api_control_operation_status_info_t    status_info;
    fbe_esp_board_mgmt_get_dimm_info_t         getDimmInfo = {0};

    // get Board Mgmt object ID
    status = fbe_api_esp_getObjectId(FBE_CLASS_ID_BOARD_MGMT, &object_id);
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, 
            "%s:Board Mgmt obj ID error:%d\n", 
            __FUNCTION__, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    getDimmInfo.location = *pLocation;

    status = fbe_api_common_send_control_packet (FBE_ESP_BOARD_MGMT_CONTROL_CODE_GET_DIMM_INFO,
                                                 &getDimmInfo,
                                                 sizeof(fbe_esp_board_mgmt_get_dimm_info_t),
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

    *pDimmInfo = getDimmInfo.dimmInfo;

    return status;

}   // end of fbe_api_esp_board_mgmt_getDimmInfo


/*!***************************************************************
 * @fn fbe_api_esp_board_mgmt_getSSDCount(void)
 ****************************************************************
 * @brief
 *  This function will send control code to retrieve SSD
 *  count from the ESP Board Mgmt object.
 *
 * @param
 *   void
 * 
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  10/14/14 - Created. bphilbin
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_esp_board_mgmt_getSSDCount(fbe_u32_t * pCount)
{
    fbe_object_id_t                            object_id;
    fbe_status_t                               status;
    fbe_api_control_operation_status_info_t    status_info;

    // get Board Mgmt object ID
    status = fbe_api_esp_getObjectId(FBE_CLASS_ID_BOARD_MGMT, &object_id);
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, 
            "%s:Board Mgmt obj ID error:%d\n", 
            __FUNCTION__, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_common_send_control_packet (FBE_ESP_BOARD_MGMT_CONTROL_CODE_GET_SSD_COUNT,
                                                 pCount,
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

}   // end of fbe_api_esp_board_mgmt_getSSDCount

/*!***************************************************************
 * @fn fbe_api_esp_board_mgmt_getSSDInfo(fbe_device_physical_location_t * pLocation,
                                  fbe_esp_board_mgmt_ssd_info_t *pSsdInfo)
 ****************************************************************
 * @brief
 *  This function will send control code to retrieve SSD
 *  status from the ESP Board Mgmt object.
 *
 * @param
 *   pLocation
 *   pSsdInfo
 * 
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  10/14/14 - Created. bphilbin
 *  
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_esp_board_mgmt_getSSDInfo(fbe_device_physical_location_t * pLocation,
                                  fbe_esp_board_mgmt_ssd_info_t *pSsdInfo)
{
    fbe_object_id_t                            object_id;
    fbe_status_t                               status;
    fbe_api_control_operation_status_info_t    status_info;
    fbe_esp_board_mgmt_get_ssd_info_t          getSsdInfo = {0};

    // get Board Mgmt object ID
    status = fbe_api_esp_getObjectId(FBE_CLASS_ID_BOARD_MGMT, &object_id);
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, 
            "%s:Board Mgmt obj ID error:%d\n", 
            __FUNCTION__, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    getSsdInfo.location = *pLocation;

    status = fbe_api_common_send_control_packet (FBE_ESP_BOARD_MGMT_CONTROL_CODE_GET_SSD_INFO,
                                                 &getSsdInfo,
                                                 sizeof(fbe_esp_board_mgmt_get_ssd_info_t),
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

    *pSsdInfo = getSsdInfo.ssdInfo;

    return status;
}   // end of fbe_api_esp_board_mgmt_getSSDInfo

