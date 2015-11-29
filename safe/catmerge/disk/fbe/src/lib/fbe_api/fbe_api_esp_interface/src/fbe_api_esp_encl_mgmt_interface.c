/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_api_esp_encl_mgmt_interface.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the encl_mgmt interface for the Environmental Services package.
 *
 * @ingroup fbe_api_esp_interface_class_files
 * @ingroup fbe_api_esp_encl_mgmt_interface
 *
 * @version
 *   03/24/2010:  Created. Ashok Tamilarasan
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_types.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_common_transport.h"
#include "fbe/fbe_api_esp_encl_mgmt_interface.h"
#include "fbe/fbe_api_esp_drive_mgmt_interface.h"
#include "fbe/fbe_esp_encl_mgmt.h"
#include "fbe/fbe_api_enclosure_interface.h"
#include "fbe/fbe_api_discovery_interface.h"
 
/*************************
 *   FUNCTION DEFINITIONS
 *************************/

/*!***************************************************************
 * @fn fbe_api_esp_get_bus_info(fbe_u32_t bus,
 *                              fbe_esp_bus_info_t * pBusInfo)
 ****************************************************************
 * @brief
 *  This function gets the bus information for the specified bus.
 *
 * @param  bus (INPUT) - The bus NO.
 * @param  pBusInfo (OUTPUT) - The pointer to the bus info for the specified bus.
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  20-Feb-2012 Eric Zhou - Created.
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL
fbe_api_esp_get_bus_info(fbe_u32_t bus,
                         fbe_esp_bus_info_t * pBusInfo)
{
    fbe_object_id_t                                 objectId;
    fbe_status_t                                    status;
    fbe_api_control_operation_status_info_t         status_info;
    fbe_esp_get_bus_info_t                          getBusInfo = {0};

    // get ENCL Mgmt object ID
    status = fbe_api_esp_getObjectId(FBE_CLASS_ID_ENCL_MGMT, &objectId);
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR,
            "%s: Error in getting ENCL Mgmt obj ID, status: 0x%x.\n",
            __FUNCTION__, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    getBusInfo.bus = bus;

    status = fbe_api_common_send_control_packet (FBE_ESP_CONTROL_CODE_GET_BUS_INFO,
                                                 &getBusInfo,
                                                 sizeof(fbe_esp_get_bus_info_t),
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

    *pBusInfo = getBusInfo.busInfo;

    return status;

}   // end of fbe_api_esp_get_bus_info

/*!***************************************************************
 * @fn fbe_api_esp_get_max_be_bus_count(fbe_u32_t * pBeBusCount)
 ****************************************************************
 * @brief
 *  This function gets the max backend bus count on the system.
 *
 * @param  pBeBusCount (OUTPUT) - The pointer to the max backend bus count on the system.
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  20-Feb-2012 Eric Zhou - Created.
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL
fbe_api_esp_get_max_be_bus_count(fbe_u32_t * pBeBusCount)
{
    fbe_object_id_t                                 objectId;
    fbe_status_t                                    status;
    fbe_api_control_operation_status_info_t         status_info;

    // get ENCL Mgmt object ID
    status = fbe_api_esp_getObjectId(FBE_CLASS_ID_ENCL_MGMT, &objectId);
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR,
            "%s: Error in getting ENCL Mgmt obj ID, status: 0x%x.\n",
            __FUNCTION__, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_common_send_control_packet (FBE_ESP_CONTROL_CODE_GET_MAX_BE_BUS_COUNT,
                                                 pBeBusCount,
                                                 sizeof(fbe_u32_t),
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

    return status;

}   // end of fbe_api_esp_get_max_be_bus_count

/*!***************************************************************
 * @fn fbe_api_esp_encl_mgmt_get_total_encl_count(fbe_u32_t * pTotalEnclCount)
 ****************************************************************
 * @brief
 *  This function gets the total enclosure count on the system.
 *  If this is the XPE system, it includes the XPE.
 * 
 * @param  pTotalEnclCount (OUTPUT) - The pointer to the total encl count on the system.
 * 
 * @return 
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 * 
 * @version
 *  17-Jan-2011 PHE - Created. 
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_esp_encl_mgmt_get_total_encl_count(fbe_u32_t * pTotalEnclCount)
{
    fbe_object_id_t                                 objectId;
    fbe_status_t                                    status;
    fbe_api_control_operation_status_info_t         status_info;
    fbe_lifecycle_state_t                           current_state;

    // get ENCL Mgmt object ID
    status = fbe_api_esp_getObjectId(FBE_CLASS_ID_ENCL_MGMT, &objectId);
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, 
            "%s: Error in getting ENCL Mgmt obj ID, status: 0x%x.\n", 
            __FUNCTION__, status);

        // hard coded hack for encl mgmt object ID since it is always the first object created now
        status = fbe_api_get_object_lifecycle_state(0, &current_state, FBE_PACKAGE_ID_ESP);  

        if (status == FBE_STATUS_GENERIC_FAILURE){
            fbe_api_trace(FBE_TRACE_LEVEL_WARNING, "%s: Get Lifecycle state failed with status %d\n", __FUNCTION__, status);          
        }
        else
        {
            fbe_api_trace (FBE_TRACE_LEVEL_ERROR, 
                "%s: ENCL Mgmt object state %s.\n", 
                __FUNCTION__, fbe_api_state_to_string(current_state));
        }
        
        return FBE_STATUS_GENERIC_FAILURE;
    }
                                                 
    status = fbe_api_common_send_control_packet (FBE_ESP_ENCL_MGMT_CONTROL_CODE_GET_TOTAL_ENCL_COUNT,
                                                 pTotalEnclCount,
                                                 sizeof(fbe_u32_t),
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

    return status;

}   // end of fbe_api_esp_encl_mgmt_get_total_encl_count

/*!***************************************************************
 * @fn fbe_api_esp_encl_mgmt_get_encl_location(fbe_device_physical_location_t * pLocation)
 ****************************************************************
 * @brief
 *  This function gets the location of a specified enclosure.
 * 
 * @param  pEnclLocationInfo (INPUT/OUTPUT) - The pointer to the encl location info.
 * 
 * @return 
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 * 
 * @version
 *  09-Feb-2011     Joe Perry - Created. 
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_esp_encl_mgmt_get_encl_location(fbe_esp_encl_mgmt_get_encl_loc_t *pEnclLocationInfo)
{
    fbe_object_id_t                                 objectId;
    fbe_status_t                                    status;
    fbe_api_control_operation_status_info_t         status_info;

    // get ENCL Mgmt object ID
    status = fbe_api_esp_getObjectId(FBE_CLASS_ID_ENCL_MGMT, &objectId);
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, 
            "%s: Error in getting ENCL Mgmt obj ID, status: 0x%x.\n", 
            __FUNCTION__, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }
                                                 
    status = fbe_api_common_send_control_packet (FBE_ESP_ENCL_MGMT_CONTROL_CODE_GET_ENCL_LOCATION,
                                                 pEnclLocationInfo,
                                                 sizeof(fbe_esp_encl_mgmt_get_encl_loc_t),
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

    return status;

}   // end of fbe_api_esp_encl_mgmt_get_encl_location

/*!***************************************************************
 * @fn fbe_api_esp_encl_mgmt_get_encl_count_on_bus(fbe_device_physical_location_t * pLocation, 
 *                                                 fbe_u32_t * pEnclCountOnBus)
 ****************************************************************
 * @brief
 *  This function gets the enclosure count on the specified bus.
 *
 * @param  pLocation (INPUT) - The pointer to the bus location.
 * @param  pEnclCountOnBus (OUTPUT) - The pointer to the encl count on the bus.
 * 
 * @return 
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 * 
 * @version
 *  17-Jan-2011 PHE - Created. 
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_esp_encl_mgmt_get_encl_count_on_bus(fbe_device_physical_location_t * pLocation, 
                                            fbe_u32_t * pEnclCountOnBus)
{
    fbe_object_id_t                                 objectId;
    fbe_status_t                                    status;
    fbe_api_control_operation_status_info_t         status_info;
    fbe_esp_encl_mgmt_get_encl_count_on_bus_t       getEnclCountOnBus = {0};

    // get ENCL Mgmt object ID
    status = fbe_api_esp_getObjectId(FBE_CLASS_ID_ENCL_MGMT, &objectId);
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, 
            "%s: Error in getting ENCL Mgmt obj ID, status: 0x%x.\n", 
            __FUNCTION__, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    getEnclCountOnBus.location = *pLocation;

    status = fbe_api_common_send_control_packet (FBE_ESP_ENCL_MGMT_CONTROL_CODE_GET_ENCL_COUNT_ON_BUS,
                                                 &getEnclCountOnBus,
                                                 sizeof(fbe_esp_encl_mgmt_get_encl_count_on_bus_t),
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

    *pEnclCountOnBus = getEnclCountOnBus.enclCountOnBus;

    return status;

}   // end of fbe_api_esp_encl_mgmt_get_encl_count_on_bus

/*!***************************************************************
 * @fn fbe_api_esp_encl_mgmt_get_encl_info(fbe_device_physical_location * pLocation,
 *                                         fbe_esp_encl_mgmt_encl_info_t * pEnclInfo)
 ****************************************************************
 * @brief
 *  This function gets the enclosure information for the specified enclosure.
 *
 * @param  pLocation (INPUT) - The pointer to the enclosure location.
 * @param  pEnclInfo (OUTPUT) - The pointer to the encl info for the specified enclosure.
 * 
 * @return 
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *                 FBE_STATUS_NO_DEVICE - device not present.
 *                 FBE_GENERIC_FAILURE - other error.
 * 
 * @version
 *  03/24/10 - Created. Ashok Tamilarasan
 *  17-Jan-2011 PHE - Modified.
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_esp_encl_mgmt_get_encl_info(fbe_device_physical_location_t * pLocation,
                                    fbe_esp_encl_mgmt_encl_info_t * pEnclInfo)
{
    fbe_object_id_t                                 object_id;
    fbe_status_t                                    status;
    fbe_api_control_operation_status_info_t         status_info;
    fbe_esp_encl_mgmt_get_encl_info_t               getEnclInfo = {0};

    // get ENCL Mgmt object ID
    status = fbe_api_esp_getObjectId(FBE_CLASS_ID_ENCL_MGMT, &object_id);
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, 
            "%s:ENCL Mgmt obj ID error:%d\n", 
            __FUNCTION__, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    getEnclInfo.location = *pLocation;

    status = fbe_api_common_send_control_packet (FBE_ESP_ENCL_MGMT_CONTROL_CODE_GET_ENCL_INFO,
                                                 &getEnclInfo,
                                                 sizeof(fbe_esp_encl_mgmt_get_encl_info_t),
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

    *pEnclInfo = getEnclInfo.enclInfo;

    return status;

}   // end of fbe_api_esp_encl_mgmt_get_encl_info


/*!***************************************************************
 * @fn fbe_api_esp_encl_mgmt_get_encl_presence(fbe_device_physical_location * pLocation,
 *                                         fbe_esp_encl_mgmt_encl_info_t * pEnclInfo)
 ****************************************************************
 * @brief
 *  This function gets whether the enclosure is present or not.
 *
 * @param  pLocation (INPUT) - The pointer to the enclosure location.
 * @param  pEnclPresent (OUTPUT) - The pointer to the value which
 *                 indicates whether the enclosure is present or not.
 *                 TRUE - enclosure is present.
 *                 FALSE - enclosure is not present.
 * 
 * @return 
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *                 FBE_GENERIC_FAILURE - other error.
 * 
 * @version
 *  03-Mar-2013 PHE - Created. 
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_esp_encl_mgmt_get_encl_presence(fbe_device_physical_location_t * pLocation,
                                    fbe_bool_t * pEnclPresent)
{
    fbe_object_id_t                                 object_id;
    fbe_status_t                                    status;
    fbe_api_control_operation_status_info_t         status_info;
    fbe_esp_encl_mgmt_get_encl_presence_t           getEnclPresence = {0};

    // get ENCL Mgmt object ID
    status = fbe_api_esp_getObjectId(FBE_CLASS_ID_ENCL_MGMT, &object_id);
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, 
            "%s:ENCL Mgmt obj ID error:%d\n", 
            __FUNCTION__, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    getEnclPresence.location = *pLocation;

    status = fbe_api_common_send_control_packet (FBE_ESP_ENCL_MGMT_CONTROL_CODE_GET_ENCL_PRESENCE,
                                                 &getEnclPresence,
                                                 sizeof(fbe_esp_encl_mgmt_get_encl_presence_t),
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

    *pEnclPresent = getEnclPresence.enclPresent;

    return status;

}   // end of fbe_api_esp_encl_mgmt_get_encl_info

/*!***************************************************************
 * @fn fbe_api_esp_encl_mgmt_get_lcc_info(fbe_device_physical_location_t * pLocation,
 *                                        fbe_esp_encl_mgmt_lcc_info_t * pLccInfo)
 ****************************************************************
 * @brief
 *  This function gets the LCC information for the specified LCC.
 *
 * @param  pLocation (INPUT) - The pointer to the LCC location.
 * @param  pLccInfo (OUTPUT) - The pointer to the LCC info for the specified LCC.
 * 
 * @return 
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *                 FBE_STATUS_NO_DEVICE - device not present.
 *                 FBE_GENERIC_FAILURE - other error.
 * 
 * @version
 *  29-Sept-2010 PHE - Created.
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_esp_encl_mgmt_get_lcc_info(fbe_device_physical_location_t * pLocation,
                                   fbe_esp_encl_mgmt_lcc_info_t * pLccInfo)
{
    fbe_object_id_t                                 object_id;
    fbe_status_t                                    status;
    fbe_api_control_operation_status_info_t         status_info;
    fbe_esp_encl_mgmt_get_lcc_info_t                getLccInfo = {0};

    // get ENCL Mgmt object ID
    status = fbe_api_esp_getObjectId(FBE_CLASS_ID_ENCL_MGMT, &object_id);
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, 
            "%s:ENCL Mgmt obj ID error:%d\n", 
            __FUNCTION__, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    getLccInfo.location = *pLocation;
    status = fbe_api_common_send_control_packet (FBE_ESP_ENCL_MGMT_CONTROL_CODE_GET_LCC_INFO,
                                                 &getLccInfo,
                                                 sizeof(fbe_esp_encl_mgmt_get_lcc_info_t),
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

    *pLccInfo = getLccInfo.lccInfo; 
    return status;

}   // end of fbe_esp_encl_mgmt_get_lcc_info_t

/*!***************************************************************
 * @fn fbe_api_esp_encl_mgmt_get_connector_info(fbe_device_physical_location_t * pLocation,
 *                                        fbe_esp_encl_mgmt_connector_info_t * pConnectorInfo)
 ****************************************************************
 * @brief
 *  This function gets the Connector information for the specified LCC.
 *
 * @param  pLocation (INPUT) - The pointer to the Connector location.
 * @param  pConnectorInfo (OUTPUT) - The pointer to the Connector info for the specified connector.
 * 
 * @return 
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *                 FBE_STATUS_NO_DEVICE - device not present.
 *                 FBE_GENERIC_FAILURE - other error.
 * 
 * @version
 *  29-Sept-2010 PHE - Created.
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_esp_encl_mgmt_get_connector_info(fbe_device_physical_location_t * pLocation,
                                   fbe_esp_encl_mgmt_connector_info_t * pConnectorInfo)
{
    fbe_object_id_t                                 object_id;
    fbe_status_t                                    status;
    fbe_api_control_operation_status_info_t         status_info;
    fbe_esp_encl_mgmt_get_connector_info_t          getConnectorInfo = {0};

    // get ENCL Mgmt object ID
    status = fbe_api_esp_getObjectId(FBE_CLASS_ID_ENCL_MGMT, &object_id);
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, 
            "%s:ENCL Mgmt obj ID error:%d\n", 
            __FUNCTION__, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    getConnectorInfo.location = *pLocation;
    status = fbe_api_common_send_control_packet (FBE_ESP_ENCL_MGMT_CONTROL_CODE_GET_CONNECTOR_INFO,
                                                 &getConnectorInfo,
                                                 sizeof(fbe_esp_encl_mgmt_get_connector_info_t),
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

    *pConnectorInfo = getConnectorInfo.connectorInfo; 
    return status;

}   // end of fbe_api_esp_encl_mgmt_get_connector_info

/*!***************************************************************
 * @fn fbe_api_esp_encl_mgmt_get_ssc_info(fbe_device_physical_location_t * pLocation,
 *                                        fbe_esp_encl_mgmt_ssc_info_t * pSscInfo)
 ****************************************************************
 * @brief
 *  This function gets SSC info for the enclosure.
 *
 * @param  pLocation (INPUT) - The pointer to the LCC location.
 * @param  pLccInfo (OUTPUT) - The pointer to the LCC info for the specified LCC.
 * 
 * @return 
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *                 FBE_STATUS_NO_DEVICE - device not present.
 *                 FBE_GENERIC_FAILURE - other error.
 * 
 * @version
 *  17-Dec-2013 GB - Created.
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_esp_encl_mgmt_get_ssc_info(fbe_device_physical_location_t * pLocation,
                                   fbe_esp_encl_mgmt_ssc_info_t * pSscInfo)
{
    fbe_object_id_t                             object_id;
    fbe_status_t                                status;
    fbe_api_control_operation_status_info_t     status_info;
    fbe_esp_encl_mgmt_get_ssc_info_t            getSscInfo = {0};

    // get ENCL Mgmt object ID to send the packet to
    status = fbe_api_esp_getObjectId(FBE_CLASS_ID_ENCL_MGMT, &object_id);
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, 
            "%s:ENCL Mgmt obj ID error:%d\n", 
            __FUNCTION__, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    getSscInfo.location.bus = pLocation->bus;
    getSscInfo.location.enclosure = pLocation->enclosure;
    status = fbe_api_common_send_control_packet (FBE_ESP_ENCL_MGMT_CONTROL_CODE_GET_SSC_INFO,
                                                 &getSscInfo,
                                                 sizeof(fbe_esp_encl_mgmt_get_ssc_info_t),
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

    *pSscInfo = getSscInfo.sscInfo; 
    return status;

}   // end of fbe_api_esp_encl_mgmt_get_ssc_info

/*!***************************************************************
 * @fn fbe_api_esp_encl_mgmt_get_ssc_count(fbe_device_physical_location_t * pLocation, 
 *                                         fbe_u32_t * pCount)
 ****************************************************************
 * @brief
 *  This function gets the connector count in the specified enclosure.
 * 
 * @param pLocation (INPUT) - The pointer to the location of the enclosure.
 * @param pCount (OUTPUT) - The pointer to the count.
 * 
 * @return 
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 * 
 * @version
 *  17-Dec-2013 GB - Created. 
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_esp_encl_mgmt_get_ssc_count(fbe_device_physical_location_t * pLocation, 
                                                        fbe_u32_t * pCount)
{
    fbe_object_id_t                             objectId;
    fbe_status_t                                status;
    fbe_api_control_operation_status_info_t     status_info;
    fbe_esp_encl_mgmt_get_device_count_t        getDeviceCountCmd = {0};

    // get ENCL Mgmt object ID to send the packet to
    status = fbe_api_esp_getObjectId(FBE_CLASS_ID_ENCL_MGMT, &objectId);
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, 
            "%s: Error in getting ENCL Mgmt obj ID, status: 0x%x.\n", 
            __FUNCTION__, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    getDeviceCountCmd.location = *pLocation;

    status = fbe_api_common_send_control_packet (FBE_ESP_ENCL_MGMT_CONTROL_CODE_GET_SSC_COUNT,
                                                 &getDeviceCountCmd,
                                                 sizeof(fbe_esp_encl_mgmt_get_device_count_t),
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

    *pCount = getDeviceCountCmd.deviceCount;

    return status;

}   // end of fbe_api_esp_encl_mgmt_get_ssc_count

/*!***************************************************************
 * @fn fbe_api_esp_encl_mgmt_get_eir_info(pGetEirInfo *pGetEirInfo)
 ****************************************************************
 * @brief
 *  This function will send control code to retrieve EIR
 *  information from the ESP ENCL Mgmt object.
 *
 * @param 
 *   encl_info - Buffer to store the return information
 * 
 * @return 
 *   fbe_status_t - FBE_STATUS_OK - if no error.
 * 
 * @note
 *  The caller needs to fill in the location value in
 *  the fbe_esp_encl_mgmt_eir_info_t structure before calling
 *  this function
 * 
 * @version
 *  27-Dec-2010     Joe Perry - Created.
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_esp_encl_mgmt_get_eir_info(fbe_esp_encl_mgmt_get_eir_info_t *pGetEirInfo)
{
    fbe_object_id_t                                 object_id;
    fbe_status_t                                    status;
    fbe_api_control_operation_status_info_t         status_info;

    // get ENCL Mgmt object ID
    status = fbe_api_esp_getObjectId(FBE_CLASS_ID_ENCL_MGMT, &object_id);
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, 
            "%s:ENCL Mgmt obj ID error:%d\n", 
            __FUNCTION__, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_common_send_control_packet (FBE_ESP_ENCL_MGMT_CONTROL_CODE_GET_EIR_INFO,
                                                 pGetEirInfo,
                                                 sizeof(fbe_esp_encl_mgmt_get_eir_info_t),
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

}   // end of fbe_api_esp_encl_mgmt_get_eir_info

/*!***************************************************************
 * @fn fbe_api_esp_encl_mgmt_get_lcc_count(fbe_device_physical_location_t * pLocation, 
 *                                         fbe_u32_t * pCount)
 ****************************************************************
 * @brief
 *  This function gets the LCC count in the specified enclosure.
 * 
 * @param  pCount (OUTPUT) - The pointer to the count.
 * 
 * @return 
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 * 
 * @version
 *  17-Jan-2011 PHE - Created. 
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_esp_encl_mgmt_get_lcc_count(fbe_device_physical_location_t * pLocation, 
                                    fbe_u32_t * pCount)
{
    fbe_object_id_t                                 objectId;
    fbe_status_t                                    status;
    fbe_api_control_operation_status_info_t         status_info;
    fbe_esp_encl_mgmt_get_device_count_t            getDeviceCountCmd = {0};

    // get ENCL Mgmt object ID
    status = fbe_api_esp_getObjectId(FBE_CLASS_ID_ENCL_MGMT, &objectId);
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, 
            "%s: Error in getting ENCL Mgmt obj ID, status: 0x%x.\n", 
            __FUNCTION__, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }
            
    getDeviceCountCmd.location = *pLocation;                                     
    status = fbe_api_common_send_control_packet (FBE_ESP_ENCL_MGMT_CONTROL_CODE_GET_LCC_COUNT,
                                                 &getDeviceCountCmd,
                                                 sizeof(fbe_esp_encl_mgmt_get_device_count_t),
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

    *pCount = getDeviceCountCmd.deviceCount;
    return status;

}   // end of fbe_api_esp_encl_mgmt_get_lcc_count


/*!***************************************************************
 * @fn fbe_api_esp_encl_mgmt_get_ps_count(fbe_device_physical_location_t * pLocation, 
 *                                         fbe_u32_t * pCount)
 ****************************************************************
 * @brief
 *  This function gets the PS count in the specified enclosure.
 * 
 * @param pLocation (INPUT) - The pointer to the location of the enclosure.
 * @param pCount (OUTPUT) - The pointer to the count.
 * 
 * @return 
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 * 
 * @version
 *  17-Jan-2011 PHE - Created. 
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_esp_encl_mgmt_get_ps_count(fbe_device_physical_location_t * pLocation, 
                                   fbe_u32_t * pCount)
{
    fbe_object_id_t                                 objectId;
    fbe_status_t                                    status;
    fbe_api_control_operation_status_info_t         status_info;
    fbe_esp_encl_mgmt_get_device_count_t            getDeviceCountCmd = {0};

    // get ENCL Mgmt object ID
    status = fbe_api_esp_getObjectId(FBE_CLASS_ID_ENCL_MGMT, &objectId);
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, 
            "%s: Error in getting ENCL Mgmt obj ID, status: 0x%x.\n", 
            __FUNCTION__, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }
      
    getDeviceCountCmd.location = *pLocation;
                                               
    status = fbe_api_common_send_control_packet (FBE_ESP_ENCL_MGMT_CONTROL_CODE_GET_PS_COUNT,
                                                 &getDeviceCountCmd,
                                                 sizeof(fbe_esp_encl_mgmt_get_device_count_t),
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

    *pCount = getDeviceCountCmd.deviceCount;
    return status;

}   // end of fbe_api_esp_encl_mgmt_get_ps_count

/*!***************************************************************
 * @fn fbe_api_esp_encl_mgmt_get_fan_count(fbe_device_physical_location_t * pLocation, 
 *                                         fbe_u32_t * pCount)
 ****************************************************************
 * @brief
 *  This function gets the FAN count in the specified enclosure.
 * 
 * @param pLocation (INPUT) - The pointer to the location of the enclosure.
 * @param pCount (OUTPUT) - The pointer to the count.
 * 
 * @return 
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 * 
 * @version
 *  17-Jan-2011 PHE - Created. 
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_esp_encl_mgmt_get_fan_count(fbe_device_physical_location_t * pLocation, 
                                    fbe_u32_t * pCount)
{
    fbe_object_id_t                                 objectId;
    fbe_status_t                                    status;
    fbe_api_control_operation_status_info_t         status_info;
    fbe_esp_encl_mgmt_get_device_count_t            getDeviceCountCmd = {0};

    // get ENCL Mgmt object ID
    status = fbe_api_esp_getObjectId(FBE_CLASS_ID_ENCL_MGMT, &objectId);
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, 
            "%s: Error in getting ENCL Mgmt obj ID, status: 0x%x.\n", 
            __FUNCTION__, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    getDeviceCountCmd.location = *pLocation;
                                                  
    status = fbe_api_common_send_control_packet (FBE_ESP_ENCL_MGMT_CONTROL_CODE_GET_FAN_COUNT,
                                                 &getDeviceCountCmd,
                                                 sizeof(fbe_esp_encl_mgmt_get_device_count_t),
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

    *pCount = getDeviceCountCmd.deviceCount;

    return status;

}   // end of fbe_api_esp_encl_mgmt_get_fan_count

/*!***************************************************************
 * @fn fbe_api_esp_encl_mgmt_get_drive_slot_count(fbe_device_physical_location_t * pLocation, 
 *                                         fbe_u32_t * pCount)
 ****************************************************************
 * @brief
 *  This function gets the drive slot count in the specified enclosure.
 * 
 * @param pLocation (INPUT) - The pointer to the location of the enclosure.
 * @param pCount (OUTPUT) - The pointer to the count.
 * 
 * @return 
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 * 
 * @version
 *  17-Jan-2011 PHE - Created. 
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_esp_encl_mgmt_get_drive_slot_count(fbe_device_physical_location_t * pLocation, 
                                           fbe_u32_t * pCount)
{
    fbe_object_id_t                                 objectId;
    fbe_status_t                                    status;
    fbe_api_control_operation_status_info_t         status_info;
    fbe_esp_encl_mgmt_get_device_count_t            getDeviceCountCmd = {0};

    // get ENCL Mgmt object ID
    status = fbe_api_esp_getObjectId(FBE_CLASS_ID_ENCL_MGMT, &objectId);
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, 
            "%s: Error in getting ENCL Mgmt obj ID, status: 0x%x.\n", 
            __FUNCTION__, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    getDeviceCountCmd.location = *pLocation;
                                                 
    status = fbe_api_common_send_control_packet (FBE_ESP_ENCL_MGMT_CONTROL_CODE_GET_DRIVE_SLOT_COUNT,
                                                 &getDeviceCountCmd,
                                                 sizeof(fbe_esp_encl_mgmt_get_device_count_t),
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

    *pCount = getDeviceCountCmd.deviceCount;

    return status;

}   // end of fbe_api_esp_encl_mgmt_get_drive_slot_count

/*!***************************************************************
 * @fn fbe_api_esp_encl_mgmt_get_connector_count(fbe_device_physical_location_t * pLocation, 
 *                                         fbe_u32_t * pCount)
 ****************************************************************
 * @brief
 *  This function gets the connector count in the specified enclosure.
 * 
 * @param pLocation (INPUT) - The pointer to the location of the enclosure.
 * @param pCount (OUTPUT) - The pointer to the count.
 * 
 * @return 
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 * 
 * @version
 *  17-Jan-2011 PHE - Created. 
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_esp_encl_mgmt_get_connector_count(fbe_device_physical_location_t * pLocation, 
                                           fbe_u32_t * pCount)
{
    fbe_object_id_t                                 objectId;
    fbe_status_t                                    status;
    fbe_api_control_operation_status_info_t         status_info;
    fbe_esp_encl_mgmt_get_device_count_t            getDeviceCountCmd = {0};

    // get ENCL Mgmt object ID
    status = fbe_api_esp_getObjectId(FBE_CLASS_ID_ENCL_MGMT, &objectId);
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, 
            "%s: Error in getting ENCL Mgmt obj ID, status: 0x%x.\n", 
            __FUNCTION__, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    getDeviceCountCmd.location = *pLocation;
                                                 
    status = fbe_api_common_send_control_packet (FBE_ESP_ENCL_MGMT_CONTROL_CODE_GET_CONNECTOR_COUNT,
                                                 &getDeviceCountCmd,
                                                 sizeof(fbe_esp_encl_mgmt_get_device_count_t),
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

    *pCount = getDeviceCountCmd.deviceCount;

    return status;

}   // end of fbe_api_esp_encl_mgmt_get_connector_count

/*!***************************************************************
 * @fn fbe_api_esp_encl_mgmt_getCacheStatus(pGetEirInfo *pGetEirInfo)
 ****************************************************************
 * @brief
 *  This function will send control code to retrieve EIR
 *  information from the ESP ENCL Mgmt object.
 *
 * @param 
 *   encl_info - Buffer to store the return information
 * 
 * @return 
 *   fbe_status_t - FBE_STATUS_OK - if no error.
 * 
 * @note
 *  The caller needs to fill in the location value in
 *  the fbe_esp_encl_mgmt_eir_info_t structure before calling
 *  this function
 * 
 * @version
 *  24-Mar-2011     Joe Perry - Created.
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_esp_encl_mgmt_getCacheStatus(fbe_common_cache_status_t *cacheStatus)
{
    fbe_object_id_t                                 object_id;
    fbe_status_t                                    status;
    fbe_api_control_operation_status_info_t         status_info;

    // get ENCL Mgmt object ID
    status = fbe_api_esp_getObjectId(FBE_CLASS_ID_ENCL_MGMT, &object_id);
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, 
            "%s:ENCL Mgmt obj ID error:%d\n", 
            __FUNCTION__, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_common_send_control_packet (FBE_ESP_ENCL_MGMT_CONTROL_CODE_GET_ENCL_CACHE_STATUS,
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

}   // end of fbe_api_esp_encl_mgmt_getCacheStatus




/*!***************************************************************
 * @fn fbe_api_esp_encl_mgmt_flash_leds(pGetEirInfo *pGetEirInfo)
 ****************************************************************
 * @brief
 *  This function will send control code to flash fault or drive
 *  slot LEDs for an enclosure.
 *
 * @param 
 *   pFlashCommand - flash command parameters
 * 
 * @return 
 *   fbe_status_t - FBE_STATUS_OK - if no error.
 * 
 * @note
 * 
 * @version
 *  23-Feb-2012     bphilbin- Created.
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_esp_encl_mgmt_flash_leds(fbe_esp_encl_mgmt_set_led_flash_t   *pFlashCommand)
{
    fbe_object_id_t                                 object_id;
    fbe_status_t                                    status;
    fbe_api_control_operation_status_info_t         status_info;

    // get ENCL Mgmt object ID
    status = fbe_api_esp_getObjectId(FBE_CLASS_ID_ENCL_MGMT, &object_id);
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, 
            "%s:ENCL Mgmt obj ID error:%d\n", 
            __FUNCTION__, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_common_send_control_packet (FBE_ESP_ENCL_MGMT_CONTROL_CODE_FLASH_ENCL_LEDS,
                                                 pFlashCommand,
                                                 sizeof(fbe_esp_encl_mgmt_set_led_flash_t),
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
 * @fn fbe_api_esp_encl_mgmt_get_eir_input_power_sample(
 *               fbe_esp_encl_mgmt_eir_input_power_sample_t *pGetPowerSampleInfo)
 ****************************************************************
 * @brief
 *  This function will send control code to retrieve input power sample
 *  from the ESP ENCL Mgmt object.
 *
 * @param
 *   pGetPowerSampleInfo - Buffer to store the return information
 *
 * @return
 *   fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @note
 *  The caller needs to fill in the location value in
 *  the fbe_esp_encl_mgmt_eir_input_power_sample_t structure
 *  before calling this function
 *
 * @version
 *  9-May-2012     Dongz - Created.
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL
fbe_api_esp_encl_mgmt_get_eir_input_power_sample(fbe_esp_encl_mgmt_eir_input_power_sample_t *pGetPowerSampleInfo)
{
    fbe_object_id_t                                 object_id;
    fbe_status_t                                    status;
    fbe_api_control_operation_status_info_t         status_info;

    // get ENCL Mgmt object ID
    status = fbe_api_esp_getObjectId(FBE_CLASS_ID_ENCL_MGMT, &object_id);
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR,
            "%s:ENCL Mgmt obj ID error:%d\n",
            __FUNCTION__, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_common_send_control_packet (FBE_ESP_ENCL_MGMT_CONTROL_CODE_GET_EIR_INPUT_POWER_SAMPLE,
                                                 pGetPowerSampleInfo,
                                                 sizeof(fbe_esp_encl_mgmt_eir_input_power_sample_t),
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

}   // end of fbe_api_esp_encl_mgmt_get_eir_input_power_sample

/*!***************************************************************
 * @fn fbe_api_esp_encl_mgmt_get_eir_temp_sample(
 *          fbe_esp_encl_mgmt_eir_temp_sample_t *pGetTempSampleInfo)
 ****************************************************************
 * @brief
 *  This function will send control code to retrieve airinlet temp sample
 *  from the ESP ENCL Mgmt object.
 *
 * @param
 *   pGetTempSampleInfo - Buffer to store the return information
 *
 * @return
 *   fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @note
 *  The caller needs to fill in the location value in
 *  the fbe_esp_encl_mgmt_eir_temp_sample_t structure
 *  before calling this function
 *
 * @version
 *  9-May-2012     Dongz - Created.
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL
fbe_api_esp_encl_mgmt_get_eir_temp_sample(fbe_esp_encl_mgmt_eir_temp_sample_t *pGetTempSampleInfo)
{
    fbe_object_id_t                                 object_id;
    fbe_status_t                                    status;
    fbe_api_control_operation_status_info_t         status_info;

    // get ENCL Mgmt object ID
    status = fbe_api_esp_getObjectId(FBE_CLASS_ID_ENCL_MGMT, &object_id);
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR,
            "%s:ENCL Mgmt obj ID error:%d\n",
            __FUNCTION__, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_common_send_control_packet (FBE_ESP_ENCL_MGMT_CONTROL_CODE_GET_EIR_TEMP_SAMPLE,
                                                 pGetTempSampleInfo,
                                                 sizeof(fbe_esp_encl_mgmt_eir_temp_sample_t),
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

}   // end of fbe_api_esp_encl_mgmt_get_eir_input_power_sample

/*!***************************************************************
 * @fn fbe_api_esp_encl_mgmt_get_array_midplane_rp_wwn_seed(
 *               fbe_u32_t *wwn_seed)
 ****************************************************************
 * @brief
 *  This function will send control code to get resume prom wwn seed
 *  info from processor enclosure
 *
 * @param
 *   wwn_seed - Buffer to store the return information
 *
 * @return
 *   fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @note
 *
 * @version
 *  16-Aug-2012     Dongz - Created.
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL
fbe_api_esp_encl_mgmt_get_array_midplane_rp_wwn_seed(fbe_u32_t *wwn_seed)
{
    fbe_object_id_t                                 object_id;
    fbe_status_t                                    status;
    fbe_api_control_operation_status_info_t         status_info;

    // get ENCL Mgmt object ID
    status = fbe_api_esp_getObjectId(FBE_CLASS_ID_ENCL_MGMT, &object_id);
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR,
            "%s:ENCL Mgmt obj ID error:%d\n",
            __FUNCTION__, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_common_send_control_packet (FBE_ESP_ENCL_MGMT_CONTROL_CODE_GET_ARRAY_MIDPLANE_RP_WWN_SEED,
                                                 wwn_seed,
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

}   // end of fbe_api_esp_encl_mgmt_get_array_midplane_rp_wwn_seed

/*!***************************************************************
 * @fn fbe_api_esp_encl_mgmt_set_array_midplane_rp_wwn_seed(fbe_u32_t *wwn_seed)
 ****************************************************************
 * @brief
 *  This function will send control code to modify the wwn seed in resume prom
 *
 * @param
 *   wwn_seed - Buffer to store the wwn seed value
 *
 * @return
 *   fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @note
 *
 * @version
 *  16-Aug-2012     Dongz - Created.
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL
fbe_api_esp_encl_mgmt_set_array_midplane_rp_wwn_seed(fbe_u32_t *wwn_seed)
{
    fbe_object_id_t                                 object_id;
    fbe_status_t                                    status;
    fbe_api_control_operation_status_info_t         status_info;

    // get ENCL Mgmt object ID
    status = fbe_api_esp_getObjectId(FBE_CLASS_ID_ENCL_MGMT, &object_id);
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR,
            "%s:ENCL Mgmt obj ID error:%d\n",
            __FUNCTION__, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_common_send_control_packet (FBE_ESP_ENCL_MGMT_CONTROL_CODE_SET_ARRAY_MIDPLANE_RP_WWN_SEED,
                                                 wwn_seed,
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

}   // end of fbe_api_esp_encl_mgmt_set_array_rp_wwn_seed

/*!***************************************************************
 * @fn fbe_api_esp_encl_mgmt_get_user_modified_wwn_seed_flag(
 *               fbe_bool_t *user_modified_wwn_seed_flag)
 ****************************************************************
 * @brief
 *  This function will send control code to get user modified wwn
 *  seed flag
 *
 * @param
 *   user_modified_wwn_seed_flag - Buffer to store the return information
 *
 * @return
 *   fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @note
 *
 * @version
 *  16-Aug-2012     Dongz - Created.
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL
fbe_api_esp_encl_mgmt_get_user_modified_wwn_seed_flag(fbe_bool_t *user_modified_wwn_seed_flag)
{
    fbe_object_id_t                                 object_id;
    fbe_status_t                                    status;
    fbe_api_control_operation_status_info_t         status_info;

    // get ENCL Mgmt object ID
    status = fbe_api_esp_getObjectId(FBE_CLASS_ID_ENCL_MGMT, &object_id);
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR,
            "%s:ENCL Mgmt obj ID error:%d\n",
            __FUNCTION__, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_common_send_control_packet (FBE_ESP_ENCL_MGMT_CONTROL_CODE_GET_USER_MODIFIED_WWN_SEED_FLAG,
                                                 user_modified_wwn_seed_flag,
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

}   // end of fbe_api_esp_encl_mgmt_get_user_modified_wwn_seed_flag

/*!***************************************************************
 * @fn fbe_api_esp_encl_mgmt_clear_user_modified_wwn_seed_flag(
 *               void)
 ****************************************************************
 * @brief
 *  This function will send control code to get user modified wwn
 *  seed flag
 *
 * @param
 *   user_modified_wwn_seed_flag - Buffer to store the return information
 *
 * @return
 *   fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @note
 *
 * @version
 *  16-Aug-2012     Dongz - Created.
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL
fbe_api_esp_encl_mgmt_clear_user_modified_wwn_seed_flag(void)
{
    fbe_object_id_t                                 object_id;
    fbe_status_t                                    status;
    fbe_api_control_operation_status_info_t         status_info;

    // get ENCL Mgmt object ID
    status = fbe_api_esp_getObjectId(FBE_CLASS_ID_ENCL_MGMT, &object_id);
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR,
            "%s:ENCL Mgmt obj ID error:%d\n",
            __FUNCTION__, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_common_send_control_packet (FBE_ESP_ENCL_MGMT_CONTROL_CODE_CLEAR_USER_MODIFIED_WWN_SEED_FLAG,
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

}   // end of fbe_api_esp_encl_mgmt_clear_user_modified_wwn_seed_flag

/*!***************************************************************
 * @fn fbe_api_esp_encl_mgmt_user_modify_wwn_seed(
 *               fbe_u32_t *wwn_seed)
 ****************************************************************
 * @brief
 *  This function will send control code to modify the wwn seed in
 *  resume prom and set user modified wwn seed flag in reg
 *
 * @param
 *   wwn_seed - Buffer to store the wwn seed value
 *
 * @return
 *   fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @note
 *
 * @version
 *  16-Aug-2012     Dongz - Created.
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL
fbe_api_esp_encl_mgmt_user_modify_wwn_seed(fbe_u32_t *wwn_seed)
{
    fbe_object_id_t                                 object_id;
    fbe_status_t                                    status;
    fbe_api_control_operation_status_info_t         status_info;

    // get ENCL Mgmt object ID
    status = fbe_api_esp_getObjectId(FBE_CLASS_ID_ENCL_MGMT, &object_id);
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR,
            "%s:ENCL Mgmt obj ID error:%d\n",
            __FUNCTION__, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_common_send_control_packet (FBE_ESP_ENCL_MGMT_CONTROL_CODE_USER_MODIFY_ARRAY_MIDPLANE_WWN_SEED,
                                                 wwn_seed,
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

}   // end of fbe_api_esp_encl_mgmt_user_modify_wwn_seed

/*!***************************************************************
 * @fn fbe_api_esp_encl_mgmt_modify_system_id_info(
 *               fbe_esp_encl_mgmt_modify_system_id_info_t *pModifySystemIdInfo)
 ****************************************************************
 * @brief
 *  This function will send control code to modify the processor enclosure
 *  product part number and product serial number
 *
 * @param
 *   pModifySystemIdInfo - Buffer to store the return information
 *
 * @return
 *   fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @note
 *  The caller needs to fill in the location value in
 *  the fbe_esp_encl_mgmt_eir_input_power_sample_t structure
 *  before calling this function
 *
 * @version
 *  9-May-2012     Dongz - Created.
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL
fbe_api_esp_encl_mgmt_modify_system_id_info(fbe_esp_encl_mgmt_modify_system_id_info_t *pModifySystemIdInfo)
{
    fbe_object_id_t                                 object_id;
    fbe_status_t                                    status;
    fbe_api_control_operation_status_info_t         status_info;

    // get ENCL Mgmt object ID
    status = fbe_api_esp_getObjectId(FBE_CLASS_ID_ENCL_MGMT, &object_id);
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR,
            "%s:ENCL Mgmt obj ID error:%d\n",
            __FUNCTION__, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_common_send_control_packet (FBE_ESP_ENCL_MGMT_CONTROL_CODE_MODIFY_SYSTEM_ID_INFO,
                                                 pModifySystemIdInfo,
                                                 sizeof(fbe_esp_encl_mgmt_modify_system_id_info_t),
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

}   // end of fbe_api_esp_encl_mgmt_modify_system_id_info

/*!***************************************************************
 * @fn fbe_api_esp_encl_mgmt_get_system_id_info(
 *               fbe_esp_encl_mgmt_modify_system_id_info_t *pModifySystemIdInfo)
 ****************************************************************
 * @brief
 *  This function will send control code to modify the processor enclosure
 *  product part number and product serial number
 *
 * @param
 *   pModifySystemIdInfo - Buffer to store the return information
 *
 * @return
 *   fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @note
 *  The caller needs to fill in the location value in
 *  the fbe_esp_encl_mgmt_eir_input_power_sample_t structure
 *  before calling this function
 *
 * @version
 *  9-May-2012     Dongz - Created.
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL
fbe_api_esp_encl_mgmt_get_system_id_info(fbe_esp_encl_mgmt_system_id_info_t *pSystemIdInfo)
{
    fbe_object_id_t                                 object_id;
    fbe_status_t                                    status;
    fbe_api_control_operation_status_info_t         status_info;

    // get ENCL Mgmt object ID
    status = fbe_api_esp_getObjectId(FBE_CLASS_ID_ENCL_MGMT, &object_id);
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR,
            "%s:ENCL Mgmt obj ID error:%d\n",
            __FUNCTION__, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_common_send_control_packet (FBE_ESP_ENCL_MGMT_CONTROL_CODE_GET_SYSTEM_ID_INFO,
                                                 pSystemIdInfo,
                                                 sizeof(fbe_esp_encl_mgmt_system_id_info_t),
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

}   // end of fbe_api_esp_encl_mgmt_get_system_id_info

/*!***************************************************************
 * @fn fbe_api_esp_encl_mgmt_get_drive_insertedness_state(fbe_device_physical_location_t *location,
 *                                                        char * insertedness_str,                                                            
 *                                                        fbe_bool_t *inserted) 
 ****************************************************************
 * @brief
 *  This function gets the drive insertedness states in the specified location.
 * 
 * @param pLocation (INPUT) - The pointer to the location of the drive.
 * @param insertedness_str (OUTPUT) - return the drive state abbreviation like "RDY". 
 *                                    Ignored if passed in NULL.
 *                                    Caller must provide a valid memory.  
 *        inserted (OUTPUT) - return if drive is physically inserted in slot. 
 *                                    Ignored if passed in NULL.
 *
 * @return 
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 * 
 * @version
 *  20-Jan-2013 Michael Li - Created. 
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_esp_encl_mgmt_get_drive_insertedness_state(fbe_device_physical_location_t *location,
                                                   char * insertedness_str,
                                                   fbe_bool_t *inserted, 
                                                   fbe_led_status_t driveFaultLed)
{
    fbe_status_t                        status = FBE_STATUS_INVALID;
    fbe_esp_drive_mgmt_drive_info_t     drive_info = {0};
    fbe_object_id_t                     object_id = FBE_OBJECT_ID_INVALID;
    fbe_enclosure_mgmt_get_drive_slot_info_t getDriveSlotInfo = {0};

    //get drive info - move to inside fbe_api_esp_encl_mgmt_get_drive_insertedness_state()
    fbe_copy_memory(&drive_info.location, location, sizeof(fbe_device_physical_location_t));
    status = fbe_api_esp_drive_mgmt_get_drive_info(&drive_info);
    if(status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR,"%s: Get drive info fail, slot: %d.\n", __FUNCTION__, location->slot);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    //Check DMO for drive state first 
    if(drive_info.inserted)
    {
        //The drive is in a valid lifycycle state. Hence return "DMO"
        if(insertedness_str)
        {
            if (driveFaultLed == FBE_LED_STATUS_ON)
            {
                /* AR#551138 - If drive fault led is on and PDO is still in the ready state, the status is set to "FLT". */
                _snprintf(insertedness_str, FBE_ENCLOSURE_DRIVE_STATE_STR_MAX_LEN, "%s", (drive_info.logical_state == FBE_BLOCK_TRANSPORT_LOGICAL_STATE_FAILED_EOL)? "EOL": "FLT");
            }
            else
            {
                _snprintf(insertedness_str, FBE_ENCLOSURE_DRIVE_STATE_STR_MAX_LEN, "%s", fbe_api_esp_drive_mgmt_map_lifecycle_state_to_str(drive_info.state));
            }
            //_snprintf(insertedness_str, FBE_ENCLOSURE_DRIVE_STATE_STR_MAX_LEN, "%s", (char *)("DMO"));
            //*drive_lifecycle_state = drive_info.state; 
        }
        if(inserted)
        {
            *inserted = FBE_TRUE;
        }
    }
    else
    {
        //get enclosure object ID
        status = fbe_api_enclosure_get_disk_parent_object_id(location->bus,
                                                             location->enclosure,
                                                             location->slot,
                                                             &object_id);

        if(status != FBE_STATUS_OK)
        {
            fbe_api_trace(FBE_TRACE_LEVEL_ERROR,"Can't get enclosure Object Id by Location, Port:%d, Enclosure:%d, Slot:%d", 
                          location->bus, location->enclosure, location->slot);
            //no much we can do if failed to get drive slot info
            return FBE_STATUS_GENERIC_FAILURE;
        }
        
        //fill location information
        getDriveSlotInfo.location = *location;
        status = fbe_api_enclosure_get_drive_slot_info(object_id,
                                                       &getDriveSlotInfo);
        if (status != FBE_STATUS_OK)
        {
            fbe_api_trace(FBE_TRACE_LEVEL_ERROR, 
                          "%s:ESES enclosure get drive slot info error:%d\n", 
                          __FUNCTION__, status);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        else
        {
            if(getDriveSlotInfo.inserted)
            {
                //Drive is powering up. If get stuck at power up, the specialze timer will eventaully
                //expire and the drive will be transitioned to failed state.
                if(insertedness_str)
                {
                    _snprintf(insertedness_str, FBE_ENCLOSURE_DRIVE_STATE_STR_MAX_LEN, "%s", (char*)("PUP"));
                }
                if(inserted)
                { 
                    *inserted = FBE_TRUE;
                }
            }
            else
            {
                //The slot is empty (no drive)
                if(insertedness_str)
                {
                    _snprintf(insertedness_str, FBE_ENCLOSURE_DRIVE_STATE_STR_MAX_LEN, "%s", (char*)("---"));
                }
                if(inserted)
                { 
                    *inserted = FBE_FALSE;
                }
            }
        }
    }                                              
    
    return status;
}   // end of fbe_api_esp_encl_mgmt_get_drive_insertedness_state

/*************************
 * end file fbe_api_esp_encl_mgmt_interface.c
 *************************/
