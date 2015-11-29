/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_api_esp_module_mgmt_interface.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the module_mgmt interface for the Environmental Services package.
 *
 * @ingroup fbe_api_esp_interface_class_files
 * @ingroup fbe_api_esp_module_mgmt_interface
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
#include "fbe/fbe_api_esp_module_mgmt_interface.h"

/*!***************************************************************
 * @fn fbe_api_esp_module_mgmt_general_status(fbe_esp_module_mgmt_general_status_t  *general_status)
 ****************************************************************
 * @brief
 *  This function will send control code to retrieve general Status
 *  from the ESP MODULE Mgmt object.
 *
 * @param 
 *   general_status - General Status Info
 * 
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  03/06/13 - Created. Brion Philbin
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_esp_module_mgmt_general_status(fbe_esp_module_mgmt_general_status_t  *general_status)
{
    fbe_object_id_t                                 object_id;
    fbe_status_t                                    status;
    fbe_api_control_operation_status_info_t         status_info;

    // get Module Mgmt object ID
    status = fbe_api_esp_getObjectId(FBE_CLASS_ID_MODULE_MGMT, &object_id);
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, 
            "%s:MODULE Mgmt obj ID error:%d\n", 
            __FUNCTION__, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_common_send_control_packet (FBE_ESP_MODULE_MGMT_CONTROL_CODE_GET_GENERAL_STATUS,
                                                          general_status,
                                                          sizeof(fbe_esp_module_mgmt_general_status_t),
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

}   // end of fbe_api_esp_module_mgmt_general_status


/*!***************************************************************
 * @fn fbe_api_esp_module_mgmt_getSpsStatus(fbe_esp_module_mgmt_moduleStatus_t *moduleStatusInfo)
 ****************************************************************
 * @brief
 *  This function will send control code to retrieve MODULE Status
 *  from the ESP MODULE Mgmt object.
 *
 * @param 
 *   moduleStatusInfo - MODULE Status Info
 * 
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  03/16/10 - Created. Joe Perry
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_esp_module_mgmt_get_module_status(fbe_esp_module_mgmt_module_status_t  *module_status_info)
{
    fbe_object_id_t                                 object_id;
    fbe_status_t                                    status;
    fbe_api_control_operation_status_info_t         status_info;

    // get Module Mgmt object ID
    status = fbe_api_esp_getObjectId(FBE_CLASS_ID_MODULE_MGMT, &object_id);
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, 
            "%s:MODULE Mgmt obj ID error:%d\n", 
            __FUNCTION__, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

//    status = fbe_api_common_send_control_packet_to_class (FBE_ESP_MODULE_MGMT_CONTROL_CODE_GET_MODULE_STATUS,
    status = fbe_api_common_send_control_packet (FBE_ESP_MODULE_MGMT_CONTROL_CODE_GET_MODULE_STATUS,
                                                          module_status_info,
                                                          sizeof(fbe_esp_module_mgmt_module_status_t),
//                                                          FBE_CLASS_ID_MODULE_MGMT,
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

}   // end of fbe_api_esp_module_mgmt_getSpsStatus

/*!***************************************************************
 * @fn fbe_api_esp_module_mgmt_getIOModuleInfo(fbe_esp_module_io_module_info_t *io_module_info)
 ****************************************************************
 * @brief
 *  This function will send control code to retrieve IO Module
 *  Info from the ESP MODULE Mgmt object.
 *
 * @param 
 *   io_module_info - IO Module Info
 * 
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  05/26/10 - Created. Nayana Chaudhari
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_esp_module_mgmt_getIOModuleInfo(fbe_esp_module_io_module_info_t *io_module_info)
{
    fbe_object_id_t                                 object_id;
    fbe_status_t                                    status;
    fbe_api_control_operation_status_info_t         status_info;

    // get Module Mgmt object ID
    status = fbe_api_esp_getObjectId(FBE_CLASS_ID_MODULE_MGMT, &object_id);
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, 
            "%s:MODULE Mgmt obj ID error:%d\n", 
            __FUNCTION__, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

//    status = fbe_api_common_send_control_packet_to_class (FBE_ESP_MODULE_MGMT_CONTROL_CODE_GET_MODULE_STATUS,
    status = fbe_api_common_send_control_packet (FBE_ESP_MODULE_MGMT_CONTROL_CODE_GET_IO_MODULE_INFO,
                                                          io_module_info,
                                                          sizeof(fbe_esp_module_io_module_info_t),
//                                                          FBE_CLASS_ID_MODULE_MGMT,
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

}// end of fbe_api_esp_module_mgmt_getIOModuleInfo

/*!***************************************************************
 * @fn fbe_api_esp_module_mgmt_getIOModulePortInfo(fbe_esp_module_io_module_info_t *io_module_info)
 ****************************************************************
 * @brief
 *  This function will send control code to retrieve IO Module
 *  Port Info from the ESP MODULE Mgmt object.
 *
 * @param 
 *   io_module_info - IO Module Info
 * 
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  05/26/10 - Created. Nayana Chaudhari
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_esp_module_mgmt_getIOModulePortInfo(fbe_esp_module_io_port_info_t *io_port_info)
{
    fbe_object_id_t                                 object_id;
    fbe_status_t                                    status;
    fbe_api_control_operation_status_info_t         status_info;

    // get Module Mgmt object ID
    status = fbe_api_esp_getObjectId(FBE_CLASS_ID_MODULE_MGMT, &object_id);
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, 
            "%s:MODULE Mgmt obj ID error:%d\n", 
            __FUNCTION__, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

//    status = fbe_api_common_send_control_packet_to_class (FBE_ESP_MODULE_MGMT_CONTROL_CODE_GET_MODULE_STATUS,
    status = fbe_api_common_send_control_packet (FBE_ESP_MODULE_MGMT_CONTROL_CODE_GET_IO_PORT_INFO,
                                                          io_port_info,
                                                          sizeof(fbe_esp_module_io_port_info_t),
//                                                          FBE_CLASS_ID_MODULE_MGMT,
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

}// end of fbe_api_esp_module_mgmt_getIOModulePortInfo

/*!***************************************************************
 * @fn fbe_api_esp_module_mgmt_get_sfp_info(fbe_esp_module_sfp_info_t *sfp_info)
 ****************************************************************
 * @brief
 *  This function will send control code to retrieve IO Module
 *  Port Info from the ESP MODULE Mgmt object.
 *
 * @param 
 *   io_module_info - IO Module Info
 * 
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  07/08/10 - Created. bphilbin
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_esp_module_mgmt_get_sfp_info(fbe_esp_module_sfp_info_t *sfp_info)
{
    fbe_object_id_t                                 object_id;
    fbe_status_t                                    status;
    fbe_api_control_operation_status_info_t         status_info;

    // get Module Mgmt object ID
    status = fbe_api_esp_getObjectId(FBE_CLASS_ID_MODULE_MGMT, &object_id);
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, 
            "%s:MODULE Mgmt obj ID error:%d\n", 
            __FUNCTION__, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

//    status = fbe_api_common_send_control_packet_to_class (FBE_ESP_MODULE_MGMT_CONTROL_CODE_GET_MODULE_STATUS,
    status = fbe_api_common_send_control_packet (FBE_ESP_MODULE_MGMT_CONTROL_CODE_GET_SFP_INFO,
                                                          sfp_info,
                                                          sizeof(fbe_esp_module_sfp_info_t),
//                                                          FBE_CLASS_ID_MODULE_MGMT,
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

}// end of fbe_api_esp_module_mgmt_get_sfp_info

/*!***************************************************************
 * @fn fbe_api_esp_module_mgmt_getMezzanineInfo(fbe_esp_module_io_module_info_t *mezz_info)
 ****************************************************************
 * @brief
 *  This function will send control code to retrieve IO Module
 *  Port Info from the ESP MODULE Mgmt object.
 *
 * @param 
 *   mezzanine_info - Mezzanine Info
 * 
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  05/26/10 - Created. Nayana Chaudhari
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_esp_module_mgmt_getMezzanineInfo(fbe_esp_module_io_module_info_t *mezzanine_info)
{
    fbe_object_id_t                                 object_id;
    fbe_status_t                                    status;
    fbe_api_control_operation_status_info_t         status_info;

    // get Module Mgmt object ID
    status = fbe_api_esp_getObjectId(FBE_CLASS_ID_MODULE_MGMT, &object_id);
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, 
            "%s:MODULE Mgmt obj ID error:%d\n", 
            __FUNCTION__, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

//    status = fbe_api_common_send_control_packet_to_class (FBE_ESP_MODULE_MGMT_CONTROL_CODE_GET_MODULE_STATUS,
    status = fbe_api_common_send_control_packet (FBE_ESP_MODULE_MGMT_CONTROL_CODE_GET_MEZZANINE_INFO,
                                                          mezzanine_info,
                                                          sizeof(fbe_esp_module_io_module_info_t),
//                                                          FBE_CLASS_ID_MODULE_MGMT,
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

}// end of fbe_api_esp_module_mgmt_getMezzanineInfo

/*!***************************************************************
 * @fn fbe_api_esp_module_mgmt_get_limits_info(fbe_esp_module_limits_info_t *limits_info)
 ****************************************************************
 * @brief
 *  This function will send control code to retrieve the limits
 *  information on a per SP basis.
 *
 * @param 
 *   limits_info - Limits information structure
 * 
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  09/02/10 - Created. bphilbin
 *  
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_esp_module_mgmt_get_limits_info(fbe_esp_module_limits_info_t *limits_info)
{
    fbe_object_id_t                                 object_id;
    fbe_status_t                                    status;
    fbe_api_control_operation_status_info_t         status_info;

    // get Module Mgmt object ID
    status = fbe_api_esp_getObjectId(FBE_CLASS_ID_MODULE_MGMT, &object_id);
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, 
            "%s:MODULE Mgmt obj ID error:%d\n", 
            __FUNCTION__, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    status = fbe_api_common_send_control_packet (FBE_ESP_MODULE_MGMT_CONTROL_CODE_GET_LIMITS_INFO,
                                                          limits_info,
                                                          sizeof(fbe_esp_module_limits_info_t),
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

}// end of fbe_api_esp_module_mgmt_get_limits_info

/*!***************************************************************
 * @fn fbe_api_esp_module_mgmt_set_port_config(fbe_esp_module_mgmt_set_port_t *set_port_cmd)
 ****************************************************************
 * @brief
 *  This function will send control code to issue a set port request,
 *  be it a persist, remove or upgrade request.
 *
 * @param 
 *   set_port_cmd - Set Port command structure
 * 
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  11/11/10 - Created. bphilbin
 *  
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_esp_module_mgmt_set_port_config(fbe_esp_module_mgmt_set_port_t *set_port_cmd)
{
     fbe_object_id_t                                 object_id;
    fbe_status_t                                    status;
    fbe_api_control_operation_status_info_t         status_info;

    // get Module Mgmt object ID
    status = fbe_api_esp_getObjectId(FBE_CLASS_ID_MODULE_MGMT, &object_id);
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, 
            "%s:MODULE Mgmt obj ID error:%d\n", 
            __FUNCTION__, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    status = fbe_api_common_send_control_packet (FBE_ESP_MODULE_MGMT_CONTROL_CODE_SET_PORT_CONFIG,
                                                 set_port_cmd,
                                                 sizeof(fbe_esp_module_mgmt_set_port_t),
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
}// end of fbe_api_esp_module_mgmt_set_port_config

/*!***************************************************************
 *  fbe_api_esp_module_mgmt_set_mgmt_port_config()
 ****************************************************************
 * @brief
 *  This function will send control code to ESP; to initiate 
 *  mgmt port config request.
 *
 * @param 
 *   mgmt_port_speed - handle to fbe_esp_module_mgmt_set_mgmt_port_config_t
 * 
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  24-Mar-2011 : Created  Vaibhav Gaonkar
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_esp_module_mgmt_set_mgmt_port_config(fbe_esp_module_mgmt_set_mgmt_port_config_t *mgmt_port_config)
{
    fbe_status_t    status;
    fbe_object_id_t object_id;
    fbe_api_control_operation_status_info_t     status_info;

    /* get Module Mgmt object ID */
    status = fbe_api_esp_getObjectId(FBE_CLASS_ID_MODULE_MGMT, &object_id);
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, 
            "%s:MODULE Mgmt obj ID error:%d\n", 
            __FUNCTION__, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    status = fbe_api_common_send_control_packet (FBE_ESP_MODULE_MGMT_CONTROL_CODE_SET_MGMT_PORT_CONFIG,
                                                 mgmt_port_config,
                                                 sizeof(fbe_esp_module_mgmt_set_mgmt_port_config_t),
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
/**********************************************************
 *  end of fbe_api_esp_module_mgmt_set_mgmt_port_speed()
 *********************************************************/
/*!***************************************************************
 *  fbe_api_esp_module_mgmt_get_mgmt_comp_info()
 ****************************************************************
 * @brief
 *  This function will send control code to ESP; to get the 
 *  mgmt_comp info
 *
 * @param 
 *   mgmt_comp_info - handle to fbe_esp_module_mgmt_get_mgmt_comp_info_t
 * 
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  25-Mar-2011 : Created  Vaibhav Gaonkar
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_esp_module_mgmt_get_mgmt_comp_info(fbe_esp_module_mgmt_get_mgmt_comp_info_t *mgmt_comp_info)
{
    fbe_status_t    status;
    fbe_object_id_t object_id;
    fbe_api_control_operation_status_info_t     status_info;

    /* get Module Mgmt object ID */
    status = fbe_api_esp_getObjectId(FBE_CLASS_ID_MODULE_MGMT, &object_id);
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, 
            "%s:MODULE Mgmt obj ID error:%d\n", 
            __FUNCTION__, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    status = fbe_api_common_send_control_packet (FBE_ESP_MODULE_MGMT_CONTROL_CODE_GET_MGMT_COMP_INFO,
                                                 mgmt_comp_info,
                                                 sizeof(fbe_esp_module_mgmt_get_mgmt_comp_info_t),
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
/**********************************************************
 *  end of fbe_api_esp_module_mgmt_get_mgmt_comp_info()
 *********************************************************/
/*!***************************************************************
 *  fbe_api_esp_module_mgmt_get_config_mgmt_port_info()
 ****************************************************************
 * @brief
 *  This function will send control code to ESP to get the 
 *  user requested management port config.
 *
 * @param 
 *   config_mgmt_port_info - handle to fbe_esp_module_mgmt_get_config_mgmt_port_info_t
 * 
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  15-April-2011 : Created  Vaibhav Gaonkar
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_esp_module_mgmt_get_config_mgmt_port_info(fbe_esp_module_mgmt_get_mgmt_port_config_t *mgmt_port_config)
{
    fbe_status_t    status;
    fbe_object_id_t object_id;
    fbe_api_control_operation_status_info_t     status_info;

    /* get Module Mgmt object ID */
    status = fbe_api_esp_getObjectId(FBE_CLASS_ID_MODULE_MGMT, &object_id);
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, 
            "%s:MODULE Mgmt obj ID error:%d\n", 
            __FUNCTION__, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    status = fbe_api_common_send_control_packet (FBE_ESP_MODULE_MGMT_CONTROL_CODE_GET_REQUESTED_MGMT_PORT_CONFIG,
                                                 mgmt_port_config,
                                                 sizeof(fbe_esp_module_mgmt_get_mgmt_port_config_t),
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
/**************************************************************
 *  end of fbe_api_esp_module_mgmt_get_config_mgmt_port_info()
 *************************************************************/

/*!***************************************************************
 * @fn fbe_api_esp_module_mgmt_markIoPort(fbe_esp_module_mgmt_set_port_t *set_port_cmd)
 ****************************************************************
 * @brief
 *  This function will send control code to mark an I/O Port.
 *
 * @param 
 *   set_port_cmd - Set Port command structure
 * 
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  11/11/10 - Created. Joe Perry
 *  
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_esp_module_mgmt_markIoPort(fbe_esp_module_mgmt_mark_port_t *markPortCmd)
{
    fbe_object_id_t                                 object_id;
    fbe_status_t                                    status;
    fbe_api_control_operation_status_info_t         status_info;

    // get Module Mgmt object ID
    status = fbe_api_esp_getObjectId(FBE_CLASS_ID_MODULE_MGMT, &object_id);
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, 
            "%s:MODULE Mgmt obj ID error:%d\n", 
            __FUNCTION__, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    status = fbe_api_common_send_control_packet (FBE_ESP_MODULE_MGMT_CONTROL_CODE_MARK_IOPORT,
                                                 markPortCmd,
                                                 sizeof(fbe_esp_module_mgmt_mark_port_t),
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
}   // end of fbe_api_esp_module_mgmt_markIoPort


/*!***************************************************************
 * @fn fbe_api_esp_module_mgmt_markIoModule(fbe_esp_module_mgmt_mark_module_t *markModuleCmd)
 ****************************************************************
 * @brief
 *  This function will send control code to mark all ports on an IO Module.
 *
 * @param 
 *   markModuleCmd - Mark Module Command Structure
 * 
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  11/11/10 - Created. Joe Perry
 *  
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_esp_module_mgmt_markIoModule(fbe_esp_module_mgmt_mark_module_t *markModuleCmd)
{
    fbe_object_id_t                                 object_id;
    fbe_status_t                                    status;
    fbe_api_control_operation_status_info_t         status_info;

    // get Module Mgmt object ID
    status = fbe_api_esp_getObjectId(FBE_CLASS_ID_MODULE_MGMT, &object_id);
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, 
            "%s:MODULE Mgmt obj ID error:%d\n", 
            __FUNCTION__, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    status = fbe_api_common_send_control_packet (FBE_ESP_MODULE_MGMT_CONTROL_CODE_MARK_IOM,
                                                 markModuleCmd,
                                                 sizeof(fbe_esp_module_mgmt_mark_module_t),
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
}   // end of fbe_api_esp_module_mgmt_markIoPort

/*!***************************************************************
 * @fn fbe_api_esp_module_mgmt_get_port_affinity(fbe_esp_module_mgmt_port_affinity_t *port_affinity)
 ****************************************************************
 * @brief
 *  This function will send control code to retrieve the port
 *  interrupt affinity settings from management object.
 *
 * @param
 *   port_affinity - Port interrupt affinity structure
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  24-Apr-2012 - Created. Lin Lou
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL
fbe_api_esp_module_mgmt_get_port_affinity(fbe_esp_module_mgmt_port_affinity_t *port_affinity)
{
    fbe_object_id_t                                 object_id;
    fbe_status_t                                    status;
    fbe_api_control_operation_status_info_t         status_info;

    // get Module Mgmt object ID
    status = fbe_api_esp_getObjectId(FBE_CLASS_ID_MODULE_MGMT, &object_id);
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR,
            "%s:MODULE Mgmt obj ID error:%d\n",
            __FUNCTION__, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    status = fbe_api_common_send_control_packet (FBE_ESP_MODULE_MGMT_CONTROL_CODE_GET_PORT_AFFINITY,
                                                 port_affinity,
                                                 sizeof(fbe_esp_module_mgmt_port_affinity_t),
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

}// end of fbe_api_esp_module_mgmt_get_port_affinity

/*************************
 * end file fbe_api_esp_module_mgmt_interface.c
 *************************/
