/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_api_cooling_interface.c
 ***************************************************************************
 *
 * @brief
 *  This file contains Cooling related APIs.  It may call more
 *  specific FBE_API functions (Board Object, Enclosure Object, ...)
 * 
 * @ingroup fbe_api_physical_package_interface_class_files
 * @ingroup fbe_api_cooling_interface
 * 
 * @version
 *   13-Jan-2011 PHE - Created
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_types.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_board.h"
#include "fbe/fbe_api_common_transport.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_api_board_interface.h"
#include "fbe/fbe_api_enclosure_interface.h"
#include "fbe/fbe_esp_cooling_mgmt.h"

/**************************************
                Local variables
**************************************/

/*******************************************
                Local functions
********************************************/



/*!***************************************************************
 * @fn fbe_api_cooling_get_fan_info(fbe_object_id_t objectId, 
 *                                  fbe_cooling_info_t * pFanInfo)
 *****************************************************************
 * @brief
 *  This function gets Cooling Info from the appropriate
 *  Physical Package Object. 
 *
 * @param object_id - The object id to send request to.
 * @param ps_info - Power Supply Info passed from the board.
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @note
 *  The client needs to supply the slotNumOnEncl value to 
 *  specify which power supply's info it is getting.
 *  Physical package expects the client to fill out this value.
 *
 * @version
 *    13-Jan-2011 PHE - created
 * 
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_cooling_get_fan_info(fbe_object_id_t objectId, 
                             fbe_cooling_info_t * pFanInfo)
{
    fbe_status_t                                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_topology_object_type_t                      objectType;

    if (pFanInfo == NULL) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: Fan Info buffer is NULL\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*
     * Determine the type of Object this is directed at
     */
    status = fbe_api_get_object_type(objectId, &objectType, FBE_PACKAGE_ID_PHYSICAL);
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, 
                       "%s: cannot get Object Type from ID 0x%x, status %d\n", 
                       __FUNCTION__, objectId, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    switch (objectType)
    {
    case FBE_TOPOLOGY_OBJECT_TYPE_BOARD:
        status = fbe_api_board_get_cooling_info(objectId, pFanInfo);
        break;
    case FBE_TOPOLOGY_OBJECT_TYPE_ENCLOSURE:
        status = fbe_api_enclosure_get_fan_info(objectId, pFanInfo);
        break;
    default:
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, 
                       "%s: Object Type %d not supported\n", 
                       __FUNCTION__, (int)objectType);
        return FBE_STATUS_GENERIC_FAILURE;
        break;
    }

    return status;

}   // end of fbe_api_cooling_get_fan_info

/*!***************************************************************
 * @fn fbe_api_cooling_get_fan_count(fbe_object_id_t objectId, 
 *                             fbe_u32_t * pFanCount)
 *****************************************************************
 * @brief
 *  This function gets Fan count from the appropriate
 *  Physical Package Object. 
 *
 * @param objectId(INTPUT) - The object id in the physical package to send request to.
 * @param pPsCount(OUTPUT) - Power Supply count reported by the specified object id.
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @note
 *
 * @version
 *    25-Aug-2010: PHE - Created.
 * 
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_cooling_get_fan_count(fbe_object_id_t objectId, 
                              fbe_u32_t * pFanCount)
{
    fbe_status_t                  status = FBE_STATUS_GENERIC_FAILURE;
    fbe_topology_object_type_t    objectType;

    if (pFanCount == NULL) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: Cooling Count buffer is NULL\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*
     * Determine the type of Object this is directed at
     */
    status = fbe_api_get_object_type(objectId, &objectType, FBE_PACKAGE_ID_PHYSICAL);
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, 
                       "%s: cannot get Object Type for objectId 0x%x, status %d\n", 
                       __FUNCTION__, objectId, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    switch (objectType)
    {
    case FBE_TOPOLOGY_OBJECT_TYPE_BOARD:
        status = fbe_api_board_get_cooling_count(objectId, pFanCount);
        break;
    case FBE_TOPOLOGY_OBJECT_TYPE_ENCLOSURE:
        status = fbe_api_enclosure_get_fan_count(objectId, pFanCount);
        break;
    default:
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, 
                       "%s: Object Type 0x%x not supported\n", 
                       __FUNCTION__, (int)objectType);
        return FBE_STATUS_GENERIC_FAILURE;
        break;
    }

    return status;

}   // end of fbe_api_cooling_get_fan_count


/*!***************************************************************
 * @fn fbe_api_esp_cooling_mgmt_getCacheStatus(void)
 ****************************************************************
 * @brief
 *  This function will send control code to retrieve caching
 *  status from the ESP Cooling Mgmt object.
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
fbe_api_esp_cooling_mgmt_getCacheStatus(fbe_common_cache_status_t *cacheStatus)
{
    fbe_object_id_t                            object_id;
    fbe_status_t                               status;
    fbe_api_control_operation_status_info_t    status_info;

    // get Cooling Mgmt object ID
    status = fbe_api_esp_getObjectId(FBE_CLASS_ID_COOLING_MGMT, &object_id);
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, 
            "%s:Cooling Mgmt obj ID error:%d\n", 
            __FUNCTION__, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_common_send_control_packet (FBE_ESP_COOLING_MGMT_CONTROL_CODE_GET_CACHE_STATUS,
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

}   // end of fbe_api_esp_cooling_mgmt_getCacheStatus

