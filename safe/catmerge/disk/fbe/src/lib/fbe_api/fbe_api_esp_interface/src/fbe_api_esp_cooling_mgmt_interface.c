/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_api_esp_cooling_mgmt_interface.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the cooling_mgmt interface for the Environmental Services package.
 *
 * @ingroup fbe_api_esp_interface_class_files
 * @ingroup fbe_api_esp_encl_mgmt_interface
 *
 * @version
 *   17-Jan-2011:  PHE - Created. 
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_types.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_common_transport.h"
#include "fbe/fbe_api_esp_cooling_mgmt_interface.h"
#include "fbe/fbe_esp_cooling_mgmt.h"
/*************************
 *   FUNCTION DEFINITIONS
 *************************/


/*!***************************************************************
 * @fn fbe_api_esp_cooling_mgmt_get_fan_info(fbe_device_physical_location_t * pLocation,
 *                                            fbe_esp_cooling_mgmt_fan_info_t * pCoolingInfo)
 ****************************************************************
 * @brief
 *  This function gets the FAN information for the specified FAN.
 *
 * @param pLocation (INPUT) - The pointer to the FAN location.
 * @param pCoolingInfo (OUTPUT) - The pointer to the fan info for the specified FAN.
 * 
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *                 FBE_STATUS_NO_DEVICE - device not present.
 *                 FBE_GENERIC_FAILURE - other error.
 *
 * @version
 *  21-Dec-2010 - Rajesh V. Created.
 *  17-Jan-2011 PHE - Modified.
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_esp_cooling_mgmt_get_fan_info(fbe_device_physical_location_t * pLocation,
                                       fbe_esp_cooling_mgmt_fan_info_t * pFanInfo)
{
    fbe_object_id_t                                 objectId;
    fbe_status_t                                    status;
    fbe_api_control_operation_status_info_t         status_info;
    fbe_esp_cooling_mgmt_get_fan_info_t             getFanInfo = {0};

    // get ENCL Mgmt object ID
    status = fbe_api_esp_getObjectId(FBE_CLASS_ID_COOLING_MGMT, &objectId);
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, 
            "%s Error in getting COOLING Mgmt obj ID, status: 0x%x.\n", 
            __FUNCTION__, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    getFanInfo.location = *pLocation;

    status = fbe_api_common_send_control_packet (FBE_ESP_COOLING_MGMT_CONTROL_CODE_GET_FAN_INFO,
                                                 &getFanInfo,
                                                 sizeof(fbe_esp_cooling_mgmt_get_fan_info_t),
                                                 objectId,
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

    *pFanInfo = getFanInfo.fanInfo;

    return status;

}   // end of fbe_api_esp_cooling_mgmt_get_fan_info

/*************************
 * end file fbe_api_esp_cooling_mgmt_interface.c
 *************************/

