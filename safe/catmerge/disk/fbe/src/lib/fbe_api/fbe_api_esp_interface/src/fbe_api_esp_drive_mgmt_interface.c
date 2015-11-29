/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_api_esp_drive_mgmt_interface.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the drive_mgmt interface for the Environmental Services package.
 *
 * @ingroup fbe_api_esp_interface_class_files
 * @ingroup fbe_api_esp_drive_mgmt_interface
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
#include "fbe/fbe_api_esp_drive_mgmt_interface.h"
#include "fbe/fbe_api_enclosure_interface.h"

/*************************
 *   FUNCTION DEFINITIONS
 *************************/
static fbe_status_t fbe_api_esp_write_bms_logpage_data_asynch_callback (fbe_packet_t * packet, fbe_packet_completion_context_t context);

 

/*!***************************************************************
 * @fn fbe_api_esp_drive_mgmt_get_drive_configuration_info(
 *         fbe_esp_drive_mgmt_info_t *dmo_info)
 ****************************************************************
 * @brief
 *  This function will send control code to retrieve 
 *  DriveConfiguration xml info.
 *
 * @param 
 *   config_info - Buffer to store the return information
 * 
 * @return 
 *   fbe_status_t - FBE_STATUS_OK - if no error.
 * 
 * 
 * @version
 *  07/24/15 - Created. Wayne Garrett
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_esp_drive_mgmt_get_drive_configuration_info(fbe_esp_drive_mgmt_driveconfig_info_t *config_info)
{
    fbe_object_id_t                                 object_id;
    fbe_status_t                                    status = FBE_STATUS_OK;
    fbe_api_control_operation_status_info_t         status_info;

    // get DRIVE Mgmt object ID
    status = fbe_api_esp_getObjectId(FBE_CLASS_ID_DRIVE_MGMT, &object_id);
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, 
            "%s:DRIVE Mgmt obj ID error:%d\n", 
            __FUNCTION__, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_common_send_control_packet (FBE_ESP_DRIVE_MGMT_CONTROL_CODE_GET_DRIVECONFIG_INFO,
                                                 config_info,
                                                 sizeof(*config_info),
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
 * @fn fbe_api_esp_drive_mgmt_get_drive_info(
 *         fbe_esp_drive_mgmt_drive_info_t *drive_info)
 ****************************************************************
 * @brief
 *  This function will send control code to retrieve drive
 *  information from the ESP DRIVE Mgmt object.
 *
 * @param 
 *   drive_info - Buffer to store the return information
 * 
 * @return 
 *   fbe_status_t - FBE_STATUS_OK - if no error.
 * 
 * @note
 *  The caller needs to fill in the location value in
 *  the fbe_esp_drive_mgmt_drive_info_t structure before calling
 *  this function
 * 
 * @version
 *  03/24/10 - Created. Ashok Tamilarasan
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_esp_drive_mgmt_get_drive_info(fbe_esp_drive_mgmt_drive_info_t *drive_info)
{
    fbe_object_id_t                                 object_id;
    fbe_status_t                                    status = FBE_STATUS_OK;
    fbe_api_control_operation_status_info_t         status_info;

    // get DRIVE Mgmt object ID
    status = fbe_api_esp_getObjectId(FBE_CLASS_ID_DRIVE_MGMT, &object_id);
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, 
            "%s:DRIVE Mgmt obj ID error:%d\n", 
            __FUNCTION__, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_common_send_control_packet (FBE_ESP_DRIVE_MGMT_CONTROL_CODE_GET_DRIVE_INFO,
                                                 drive_info,
                                                 sizeof(fbe_esp_drive_mgmt_drive_info_t),
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

}   // end of fbe_api_esp_drive_mgmt_get_drive_info


/*!***************************************************************
 * @fn fbe_api_esp_drive_mgmt_get_policy (
 *      fbe_esp_drive_mgmt_policy_t *policy )
 ****************************************************************
 * @brief
 *  This function will get a given policy
 *
 * @param 
 *   policy - Contains policy ID and value indicating if it should
 *      enabled or disabled.
 * 
 * @return 
 *   fbe_status_t - FBE_STATUS_OK - if no error.
 * 
 * @note
 * 
 * @version
 *  12/23/13 - Created. Wayne Garrett
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL 
 fbe_api_esp_drive_mgmt_get_policy (fbe_drive_mgmt_policy_t *policy )
{
    fbe_object_id_t                         object_id;
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_api_control_operation_status_info_t status_info;

    // get DRIVE Mgmt object ID
    status = fbe_api_esp_getObjectId(FBE_CLASS_ID_DRIVE_MGMT, &object_id);

    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, 
            "%s:DRIVE Mgmt obj ID error:%d\n", 
            __FUNCTION__, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }    

    status = fbe_api_common_send_control_packet (FBE_ESP_DRIVE_MGMT_CONTROL_CODE_GET_POLICY,
                                                 policy,
                                                 sizeof(fbe_drive_mgmt_policy_t),
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
 * @fn fbe_api_esp_drive_mgmt_change_policy (
 *      fbe_esp_drive_mgmt_policy_t *policy )
 ****************************************************************
 * @brief
 *  This function will enable/disable a given policy
 *
 * @param 
 *   policy - Contains policy ID and value indicating if it should
 *      enabled or disabled.
 * 
 * @return 
 *   fbe_status_t - FBE_STATUS_OK - if no error.
 * 
 * @note
 * 
 * @version
 *  05/20/10 - Created. Wayne Garrett
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL 
 fbe_api_esp_drive_mgmt_change_policy (fbe_drive_mgmt_policy_t *policy )
{
    fbe_object_id_t                         object_id;
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_api_control_operation_status_info_t status_info;
    
    // get DRIVE Mgmt object ID
    status = fbe_api_esp_getObjectId(FBE_CLASS_ID_DRIVE_MGMT, &object_id);
    
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, 
            "%s:DRIVE Mgmt obj ID error:%d\n", 
            __FUNCTION__, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }    
    
    status = fbe_api_common_send_control_packet (FBE_ESP_DRIVE_MGMT_CONTROL_CODE_CHANGE_POLICY,
                                                 policy,
                                                 sizeof(fbe_drive_mgmt_policy_t),
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
 * @fn fbe_api_esp_drive_mgmt_enable_fuel_gauge()
 ****************************************************************
 * @brief
 *  This function issues a command to enable/disable fuel gauge feature.
 *
 * @param 
 *   enable - enable/disable fuel gauge feature.
 * 
 * @return 
 *   fbe_status_t - FBE_STATUS_OK - if no error.
 *                - FBE_STATUS_GENERIC_FAILURE if there an error.
 * 
 * @version
 *  11/19/10 - Created. chenl6
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_esp_drive_mgmt_enable_fuel_gauge(fbe_u32_t enable)
{
    fbe_object_id_t                                 object_id;
    fbe_status_t                                    status = FBE_STATUS_OK;
    fbe_api_control_operation_status_info_t         status_info;

    /* get DRIVE Mgmt object ID */
    status = fbe_api_esp_getObjectId(FBE_CLASS_ID_DRIVE_MGMT, &object_id);
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, 
            "%s:DRIVE Mgmt obj ID error:%d\n", 
            __FUNCTION__, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_common_send_control_packet (FBE_ESP_DRIVE_MGMT_CONTROL_CODE_ENABLE_FUEL_GAUGE,
                                                 &enable,
                                                 sizeof(enable),
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
 * @fn fbe_api_esp_drive_mgmt_set_fuel_gauge_poll_interval()
 ****************************************************************
 * @brief
 *  This function issues a command to set minimum fuel gauge poll interval on memory.
 *
 * @param 
 *   interval - minimum fuel gauge poll interval.
 * 
 * @return 
 *   fbe_status_t - FBE_STATUS_OK - if no error.
 *                - FBE_STATUS_GENERIC_FAILURE if there an error.
 * 
 * @version
 *  11/05/12 - Created. chianc1
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_esp_drive_mgmt_set_fuel_gauge_poll_interval(fbe_u32_t interval)
{
    fbe_object_id_t                                 object_id;
    fbe_status_t                                    status = FBE_STATUS_OK;
    fbe_api_control_operation_status_info_t         status_info;

    /* get DRIVE Mgmt object ID */
    status = fbe_api_esp_getObjectId(FBE_CLASS_ID_DRIVE_MGMT, &object_id);
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, 
            "%s:DRIVE Mgmt obj ID error:%d\n", 
            __FUNCTION__, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_common_send_control_packet (FBE_ESP_DRIVE_MGMT_CONTROL_CODE_SET_FUEL_GAUGE_POLL_INTERVAL,
                                                 &interval,
                                                 sizeof(interval),
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
 * @fn fbe_api_esp_drive_mgmt_get_fuel_gauge_poll_interval()
 ****************************************************************
 * @brief
 *  This function issues a command to get minimum fuel gauge poll interval from registry.
 *
 * @param 
 *   None.
 * 
 * @return 
 *   fbe_status_t - FBE_STATUS_OK - if no error.
 *                - FBE_STATUS_GENERIC_FAILURE if there an error.
 * 
 * 
 * @version
 *  11/05/12 - Created. chianc1
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_esp_drive_mgmt_get_fuel_gauge_poll_interval(fbe_u32_t *interval)
{
    fbe_object_id_t                                 object_id;
    fbe_status_t                                    status = FBE_STATUS_OK;
    fbe_api_control_operation_status_info_t         status_info;

    /* get DRIVE Mgmt object ID */
    status = fbe_api_esp_getObjectId(FBE_CLASS_ID_DRIVE_MGMT, &object_id);
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, 
            "%s:DRIVE Mgmt obj ID error:%d\n", 
            __FUNCTION__, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_common_send_control_packet (FBE_ESP_DRIVE_MGMT_CONTROL_CODE_GET_FUEL_GAUGE_POLL_INTERVAL,
                                                 interval,
                                                 sizeof (fbe_u32_t),
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

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++/
/* TESTING ONLY INTERFACE FUNCTIONS */


/*!***************************************************************
 * @fn fbe_api_esp_drive_mgmt_send_simulation_command()
 ****************************************************************
 * @brief
 *
 * @param 
 *   n/a
 * 
 * @return 
 *   fbe_status_t - FBE_STATUS_OK - if no error.
 * 
 * @version
 *  08/26/10 - Created. Wayne Garrett
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_esp_drive_mgmt_send_simulation_command(void *command_data, fbe_u32_t data_size)
{
    fbe_object_id_t                                 object_id;
    fbe_status_t                                    status = FBE_STATUS_OK;
    fbe_api_control_operation_status_info_t         status_info;

    // get DRIVE Mgmt object ID
    status = fbe_api_esp_getObjectId(FBE_CLASS_ID_DRIVE_MGMT, &object_id);
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, 
            "%s:DRIVE Mgmt obj ID error:%d\n", 
            __FUNCTION__, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_common_send_control_packet (FBE_ESP_DRIVE_MGMT_SIMULATION_COMMAND,
                                                 command_data,
                                                 data_size,
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
 * @fn fbe_api_esp_drive_mgmt_get_drive_log
 ****************************************************************
 * @brief
 *  This function sends drivegetlog command to ESP.
 *
 * @param 
 *   drivegetlog_info - drivegetlog information.
 * 
 * @return 
 *   fbe_status_t - FBE_STATUS_OK - if no error.
 * 
 * @version
 *  03/25/11 - Created. chenl6
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_esp_drive_mgmt_get_drive_log(fbe_device_physical_location_t *location)
{
    fbe_object_id_t                                 object_id;
    fbe_status_t                                    status = FBE_STATUS_OK;
    fbe_api_control_operation_status_info_t         status_info;

    /* get DRIVE Mgmt object ID */
    status = fbe_api_esp_getObjectId(FBE_CLASS_ID_DRIVE_MGMT, &object_id);
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, 
            "%s:DRIVE Mgmt obj ID error:%d\n", 
            __FUNCTION__, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_common_send_control_packet (FBE_ESP_DRIVE_MGMT_CONTROL_CODE_GET_DRIVE_LOG,
                                                 location,
                                                 sizeof(fbe_device_physical_location_t),
                                                 object_id,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &status_info,
                                                 FBE_PACKAGE_ID_ESP);


    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) 
    {
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, 
            "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", 
            __FUNCTION__, status, 
            status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        if (FBE_STATUS_NO_OBJECT == status)
        {
            return FBE_STATUS_NO_OBJECT;
        }
        else
        {
           return FBE_STATUS_GENERIC_FAILURE;
        }
    }

    return status;
}

/*!***************************************************************
 * @fn fbe_api_esp_drive_mgmt_get_fuel_gauge
 ****************************************************************
 * @brief
 *  This function sends fuel gauge command to ESP.
 *
 * @param 
 *   fuel_gauge_info - fuel gauge information.
 * 
 * @return 
 *   fbe_status_t - FBE_STATUS_OK - if no error.
 * 
 * @version
 *  06/26/12 - Created. Christina Chiang
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_esp_drive_mgmt_get_fuel_gauge(fbe_u32_t *enable)
{
    fbe_object_id_t                                 object_id;
    fbe_status_t                                    status = FBE_STATUS_OK;
    fbe_api_control_operation_status_info_t         status_info;

    /* get DRIVE Mgmt object ID */
    status = fbe_api_esp_getObjectId(FBE_CLASS_ID_DRIVE_MGMT, &object_id);
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, 
            "%s:DRIVE Mgmt obj ID error:%d\n", 
            __FUNCTION__, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_common_send_control_packet (FBE_ESP_DRIVE_MGMT_CONTROL_CODE_GET_FUEL_GAUGE,
                                                 enable,
                                                 sizeof (fbe_u32_t),
                                                 object_id,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &status_info,
                                                 FBE_PACKAGE_ID_ESP);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) 
    {
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, 
            "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", 
            __FUNCTION__, status, 
            status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;

}

/*!***************************************************************
 * @fn fbe_api_esp_drive_mgmt_send_debug_control
 ****************************************************************
 * @brief
 *  This function will fill in struct fbe_esp_drive_mgmt_debug_control_t
 *  and send packet tp prcess drive debug control.
 *
 * @param 
 *   bus, enc, slot - position infor for the target drive
 *   maxslots - the number of drive slots in the enclosure
 *   command - the debug command
 * 
 * @return 
 *   fbe_status_t - FBE_STATUS_OK - if no error.
 * 
 * @note
 *  The caller needs to fill in the drive location in
 *  fbe_drive_mgmt_parent_obj_t structure before calling
 *  this function
 * 
 * @version
 *  07/24/11 - Created. GB
 ****************************************************************/
fbe_status_t fbe_api_esp_drive_mgmt_send_debug_control(fbe_u32_t bus, 
                                                       fbe_u32_t encl, 
                                                       fbe_u32_t slot, 
                                                       fbe_u32_t maxSlots,
                                                       fbe_base_object_mgmt_drv_dbg_action_t command,
                                                       fbe_bool_t defer,
                                                       fbe_bool_t dualsp)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_object_id_t                         object_id;
    fbe_esp_drive_mgmt_debug_control_t      command_info;
    fbe_api_control_operation_status_info_t status_info;

    // get DRIVE Mgmt object ID
    status = fbe_api_esp_getObjectId(FBE_CLASS_ID_DRIVE_MGMT, &object_id);
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, 
            "%s:DRIVE Mgmt obj ID error:%d\n", 
            __FUNCTION__, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_zero_memory(&command_info, sizeof(fbe_esp_drive_mgmt_debug_control_t));
    command_info.bus = bus;
    command_info.encl = encl;
    command_info.slot = slot;
    command_info.dbg_ctrl.driveCount = maxSlots;
    command_info.defer = defer;
    command_info.dualsp = dualsp;
    command_info.dbg_ctrl.driveDebugAction[slot] = command;
    status = fbe_api_common_send_control_packet (FBE_ESP_DRIVE_MGMT_CONTROL_CODE_DEBUG_COMMAND,
                                                 &command_info,
                                                 sizeof(fbe_esp_drive_mgmt_debug_control_t),
                                                 object_id,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &status_info,
                                                 FBE_PACKAGE_ID_ESP);

    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, 
            "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", 
            __FUNCTION__, status, 
            status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return(FBE_STATUS_OK);

} //fbe_api_esp_drive_mgmt_send_debug_control


/*!***************************************************************
 * @fn fbe_api_esp_drive_mgmt_dieh_load_config_file
 ****************************************************************
 * @brief
 *  This function will load the DIEH (Drive Improved Error Handling)
 *  configuration.
 *
 * @param 
 *   config_info - Contains an xml file to load the configuration from.
 *                 If the filename is empty ("") then the default XML
 *                 file will be loaded.
 * 
 * @return 
 *   fbe_dieh_load_status_t - status for loading the configuration
 * 
 * @note
 *    This call is synchronous.
 * 
 * @version
 *  03/23/12 : Wayne Garrett  -- created
 ****************************************************************/
fbe_dieh_load_status_t FBE_API_CALL fbe_api_esp_drive_mgmt_dieh_load_config_file(fbe_drive_mgmt_dieh_load_config_t *config_info)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_object_id_t                         object_id;
    fbe_api_control_operation_status_info_t status_info;

    // get DRIVE Mgmt object ID
    status = fbe_api_esp_getObjectId(FBE_CLASS_ID_DRIVE_MGMT, &object_id);
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, 
            "%s:DRIVE Mgmt obj ID error:%d\n", 
            __FUNCTION__, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_common_send_control_packet (FBE_ESP_DRIVE_MGMT_CONTROL_CODE_DIEH_LOAD_CONFIG,
                                                 config_info,
                                                 sizeof(fbe_drive_mgmt_dieh_load_config_t),
                                                 object_id,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &status_info,
                                                 FBE_PACKAGE_ID_ESP);

    if (status != FBE_STATUS_OK)  /* error sending packet */
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, 
            "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", 
            __FUNCTION__, status, 
            status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        status_info.control_operation_qualifier = FBE_DMO_THRSH_ERROR;
    }

    return status_info.control_operation_qualifier;    /* qualifier contains operation status*/

} // fbe_api_esp_drive_mgmt_dieh_load_config_file


/*!***************************************************************
 * @fn fbe_api_esp_drive_mgmt_get_all_drives_info(fbe_esp_drive_mgmt_drive_info_t *drive_info, fbe_u32_t expected_count, fbe_u32_t *actual_count)
 ****************************************************************
 * @brief
 *  This function will fill the buffer from the user with the fbe_esp_drive_mgmt_drive_info_t for all drive present in the system
 *  The user is in charge of buffer allocation and release.
 *  The user must call fbe_api_esp_drive_mgmt_get_total_drives_count so it can tell how much memory to allocate
 *
 * @param 
 *   drive_info - Buffer to store the return information
 *   expected_count - how many drives the user allocated for
 *   actual_count - OUT to say how many we returned
 * 
 * @return 
 *   fbe_status_t - FBE_STATUS_OK - if no error.
 * 
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_esp_drive_mgmt_get_all_drives_info(fbe_esp_drive_mgmt_drive_info_t *drive_info, fbe_u32_t expected_count, fbe_u32_t *actual_count)
{
    fbe_status_t                            status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t status_info;
    fbe_sg_element_t *                      sg_list = NULL;
    dmo_get_all_drives_info_t               get_all_drives;
    fbe_object_id_t                         object_id;

    if (expected_count == 0 || actual_count == NULL || drive_info == NULL){
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:Illegal input\n", __FUNCTION__);
        return FBE_STATUS_OK;
    }

    // get DRIVE Mgmt object ID
    status = fbe_api_esp_getObjectId(FBE_CLASS_ID_DRIVE_MGMT, &object_id);
    if (status != FBE_STATUS_OK){
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, 
            "%s:DRIVE Mgmt obj ID error:%d\n", 
            __FUNCTION__, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    sg_list = fbe_api_allocate_memory(sizeof(fbe_sg_element_t) * 2);
    if (sg_list == NULL) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:out of memory\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*set up the sg list to point to the list of objects the user wants to get*/
    sg_list[0].address = (fbe_u8_t *)drive_info;
    sg_list[0].count = expected_count * sizeof(fbe_esp_drive_mgmt_drive_info_t);
    sg_list[1].address = NULL;
    sg_list[1].count = 0;

    get_all_drives.expected_count = expected_count;
    get_all_drives.actual_count = 0;

    /* upon completion, the user memory will be filled with the data, this is taken care
     * of by the FBE API common code 
     */
    status = fbe_api_common_send_control_packet_with_sg_list(FBE_ESP_DRIVE_MGMT_CONTROL_CODE_GET_ALL_DRIVES_INFO,
                                                             &get_all_drives,
                                                             sizeof (dmo_get_all_drives_info_t),
                                                             object_id,
                                                             FBE_PACKET_FLAG_NO_CONTIGUOS_ALLOCATION_NEEDED,
                                                             sg_list,
                                                             2,
                                                             &status_info,
                                                             FBE_PACKAGE_ID_ESP);


    fbe_api_free_memory (sg_list);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    *actual_count = get_all_drives.actual_count;

    return FBE_STATUS_OK;

}   // end of fbe_api_esp_drive_mgmt_get_all_drives_info

/*!***************************************************************
 * @fn fbe_api_esp_drive_mgmt_get_total_drives_count(fbe_u32_t *total_drive)
 ****************************************************************
 * @brief
 *  This function will tell the user how many physical drive are present in the system
 *
 * @param 
 *   total_drive - pointer to return total into
 * 
 * @return 
 *   fbe_status_t - FBE_STATUS_OK - if no error.
 * 
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_esp_drive_mgmt_get_total_drives_count(fbe_u32_t *total_drive)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_object_id_t                         object_id;
    dmo_get_total_drives_count_t            get_count;
    fbe_api_control_operation_status_info_t status_info;

    if (total_drive == NULL) {
         fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s NULL pointer\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    // get DRIVE Mgmt object ID
    status = fbe_api_esp_getObjectId(FBE_CLASS_ID_DRIVE_MGMT, &object_id);
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, 
            "%s:DRIVE Mgmt obj ID error:%d\n", 
            __FUNCTION__, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    *total_drive = 0;

    status = fbe_api_common_send_control_packet (FBE_ESP_DRIVE_MGMT_CONTROL_CODE_GET_TOTAL_DRIVES_COUNT,
                                                 &get_count,
                                                 sizeof(dmo_get_total_drives_count_t),
                                                 object_id,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &status_info,
                                                 FBE_PACKAGE_ID_ESP);

    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, 
            "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", 
            __FUNCTION__, status, 
            status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    *total_drive = get_count.total_count;

    return(FBE_STATUS_OK);

} // end of fbe_api_esp_drive_mgmt_get_total_drives_count


/*!***************************************************************
 * @fn fbe_api_esp_drive_mgmt_get_max_platform_drive_count(fbe_u32_t * max_platfrom_drive_count)
 ****************************************************************
 * @brief
 *  This function will tell the user the maximum number of drives the platform allows.
 *
 * @param 
 *   pMaxPlatfromDriveCount - The pointer to the maximum number of drives the platform allows
 * 
 * @return 
 *   fbe_status_t - FBE_STATUS_OK - if no error.
 * 
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_esp_drive_mgmt_get_max_platform_drive_count(fbe_u32_t * pMaxPlatfromDriveCount)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_object_id_t                         objectId = FBE_OBJECT_ID_INVALID;
    fbe_api_control_operation_status_info_t status_info = {0};

    if (pMaxPlatfromDriveCount == NULL) {
         fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s NULL pointer\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    // get DRIVE Mgmt object ID
    status = fbe_api_esp_getObjectId(FBE_CLASS_ID_DRIVE_MGMT, &objectId);
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, 
            "%s: failed to get DMO obj ID, status 0x%X.\n", 
            __FUNCTION__, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_common_send_control_packet (FBE_ESP_DRIVE_MGMT_CONTROL_CODE_GET_MAX_PLATFORM_DRIVE_COUNT,
                                                 pMaxPlatfromDriveCount,
                                                 sizeof(fbe_u32_t),
                                                 objectId,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &status_info,
                                                 FBE_PACKAGE_ID_ESP);

    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, 
            "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", 
            __FUNCTION__, status, 
            status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return(FBE_STATUS_OK);

} // end of fbe_api_esp_drive_mgmt_get_max_platform_drive_count

/*!***************************************************************
 * @fn fbe_api_esp_drive_mgmt_set_crc_actions(fbe_pdo_logical_error_action_t *action)
 ****************************************************************
 * @brief
 *   This function will set the Physical Drive CRC actions.  
 *
 * @param 
 *   action - A bitmap of actions to be used for the various CRC errors.
 * 
 * @note
 *   This setting is not persistent across SP reboots.  A reboot will
 *   restore from the registry
 * 
 * @return 
 *   fbe_status_t - FBE_STATUS_OK - if no error.
 * 
 * @author
 *   05/11/2012  Wayne Garrett - Created.
 * 
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_esp_drive_mgmt_set_crc_actions(fbe_pdo_logical_error_action_t *action)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_api_control_operation_status_info_t status_info;
    fbe_object_id_t object_id;


    // get DRIVE Mgmt object ID
    status = fbe_api_esp_getObjectId(FBE_CLASS_ID_DRIVE_MGMT, &object_id);
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:DRIVE Mgmt obj ID error:%d\n",  __FUNCTION__, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_common_send_control_packet (FBE_ESP_DRIVE_MGMT_CONTROL_SET_CRC_ACTION,
                                                 action,
                                                 sizeof(fbe_pdo_logical_error_action_t),
                                                 object_id,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &status_info,
                                                 FBE_PACKAGE_ID_ESP);

    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, 
            "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", 
            __FUNCTION__, status, 
            status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return FBE_STATUS_OK ;
}



/*!*************************************************************************
 * @fn fbe_api_esp_drive_mgmt_map_lifecycle_state_to_str(fbe_lifecycle_state_t state) 
 **************************************************************************
 *
 *  @brief
 *      This function will get int Lifecycle_state and convert into text,
 *      the text is short for display slot state.
 *
 *  @param    fbe_lifecycle_state_t - state
 *
 *  @return     char * - string
 *
 *  NOTES:
 *
 *  HISTORY:
 *    09-Jan-2012: Zhenhua Dong- created.
 *    22-Jan-2013: Michael Li - Renamed and moved here to keep the lifecycle
 *                              state to string mapping internal.
 *
 **************************************************************************/
char * FBE_API_CALL fbe_api_esp_drive_mgmt_map_lifecycle_state_to_str(fbe_lifecycle_state_t state)
{
    char * LifeCycType;
    switch (state)
    {
        case FBE_LIFECYCLE_STATE_SPECIALIZE:
            LifeCycType = (char *)("SPE");
        break;
        case FBE_LIFECYCLE_STATE_ACTIVATE:
            LifeCycType = (char *)("ACT");
        break;
        case FBE_LIFECYCLE_STATE_READY:
            LifeCycType = (char *)("RDY");
        break;
        case FBE_LIFECYCLE_STATE_HIBERNATE:
            LifeCycType = (char *)("HBE");
        break;
        case FBE_LIFECYCLE_STATE_OFFLINE:
            LifeCycType = (char *)("OFF");
        break;
        case FBE_LIFECYCLE_STATE_FAIL:
            LifeCycType = (char *)("FLT");
        break;
        case FBE_LIFECYCLE_STATE_DESTROY:
            LifeCycType = (char *)("DES");
        break;
        case FBE_LIFECYCLE_STATE_NON_PENDING_MAX:
            LifeCycType = (char *)("NPMAX");
        break;
       case FBE_LIFECYCLE_STATE_PENDING_ACTIVATE:
            LifeCycType = (char *)("P_ACT");
        break;
        case FBE_LIFECYCLE_STATE_PENDING_HIBERNATE:
            LifeCycType = (char *)("P_HIB");
        break;
        case FBE_LIFECYCLE_STATE_PENDING_OFFLINE:
            LifeCycType = (char *)("P_OFF");
        break;
        case FBE_LIFECYCLE_STATE_PENDING_FAIL:
            LifeCycType = (char *)("P_FIL");
        break;
        case FBE_LIFECYCLE_STATE_PENDING_DESTROY:
            LifeCycType = (char *)("P_DES");
        break;
        case FBE_LIFECYCLE_STATE_PENDING_MAX:
            LifeCycType = (char *)("P_MAX");
        break;
        case FBE_LIFECYCLE_STATE_INVALID:
            LifeCycType = (char *)("INV");
        break;
        default:
            LifeCycType = (char *)( "UNKWN");
        break;
    }
    return (LifeCycType);
} //End of fbe_cli_print_slot_LifeCycleState


/*!***************************************************************
 * fbe_api_esp_drive_mgmt_reduce_system_drive_queue_depth()
 *****************************************************************
 * @brief
 *  Reduce the system drive queue depth to a value defined in the
 *  DIEH XML file.
 *  
 * @return fbe_status_t.
 *
 * @author
 *  05-02-13  Matthew Ferson -- created
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_esp_drive_mgmt_reduce_system_drive_queue_depth(void)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_api_control_operation_status_info_t status_info;
    fbe_object_id_t object_id;


    // get DRIVE Mgmt object ID
    status = fbe_api_esp_getObjectId(FBE_CLASS_ID_DRIVE_MGMT, &object_id);
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:DRIVE Mgmt obj ID error:%d\n",  __FUNCTION__, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_common_send_control_packet(FBE_ESP_DRIVE_MGMT_CONTROL_CODE_REDUCE_SYSTEM_DRIVE_QUEUE_DEPTH,
                                                 NULL,
                                                 0,
                                                 object_id,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &status_info,
                                                 FBE_PACKAGE_ID_ESP);

    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, 
            "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", 
            __FUNCTION__, status, 
            status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return FBE_STATUS_OK ;
}

/*!***************************************************************
 * fbe_api_esp_drive_mgmt_restore_system_drive_queue_depth()
 *****************************************************************
 * @brief
 *  Restores the system drive queue depth to a normal value 
 *  defined in the DIEH XML file.
 *  
 * @return fbe_status_t.
 *
 * @author
 *  05-02-13  Matthew Ferson -- created
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_esp_drive_mgmt_restore_system_drive_queue_depth(void)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_api_control_operation_status_info_t status_info;
    fbe_object_id_t object_id;


    // get DRIVE Mgmt object ID
    status = fbe_api_esp_getObjectId(FBE_CLASS_ID_DRIVE_MGMT, &object_id);
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:DRIVE Mgmt obj ID error:%d\n",  __FUNCTION__, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_common_send_control_packet(FBE_ESP_DRIVE_MGMT_CONTROL_CODE_RESTORE_SYSTEM_DRIVE_QUEUE_DEPTH,
                                                 NULL,
                                                 0,
                                                 object_id,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &status_info,
                                                 FBE_PACKAGE_ID_ESP);

    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, 
            "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", 
            __FUNCTION__, status, 
            status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return FBE_STATUS_OK ;
}

/*!***************************************************************
 * fbe_api_esp_drive_mgmt_reset_system_drive_queue_depth()
 *****************************************************************
 * @brief
 *  Restores the system drive queue depth to a normal value 
 *  defined in the DIEH XML file.
 *  
 * @return fbe_status_t.
 *
 * @author
 *  05-02-13  Matthew Ferson -- created
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_esp_drive_mgmt_reset_system_drive_queue_depth(void)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_api_control_operation_status_info_t status_info;
    fbe_object_id_t object_id;


    // get DRIVE Mgmt object ID
    status = fbe_api_esp_getObjectId(FBE_CLASS_ID_DRIVE_MGMT, &object_id);
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:DRIVE Mgmt obj ID error:%d\n",  __FUNCTION__, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_common_send_control_packet(FBE_ESP_DRIVE_MGMT_CONTROL_CODE_RESET_SYSTEM_DRIVE_QUEUE_DEPTH,
                                                 NULL,
                                                 0,
                                                 object_id,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &status_info,
                                                 FBE_PACKAGE_ID_ESP);

    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, 
            "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", 
            __FUNCTION__, status, 
            status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return FBE_STATUS_OK ;
}


/*!*************************************************************************
 * @fn fbe_api_esp_drive_mgmt_map_death_reason_to_str(fbe_u32_t death_reason) 
 **************************************************************************
 *
 *  @brief
 *      This function will get death reason and convert into text.
 *
 *  @param    fbe_u32_t - death_reason
 *
 *  @return     char * - string
 *
 *  NOTES:
 *
 *  HISTORY:
 *    28-Oct-2013: Created
 *
 **************************************************************************/
char * FBE_API_CALL fbe_api_esp_drive_mgmt_map_death_reason_to_str(fbe_u32_t death_reason) 
{
    char *death_reason_desc = NULL;

    switch (death_reason) {
    // Physical drive death reason
    case FBE_BASE_PHYSICAL_DRIVE_DEATH_REASON_INVALID_CAPACITY:
        death_reason_desc = "FBE_BASE_PHYSICAL_DRIVE_DEATH_REASON_INVALID_CAPACITY";
        break;
    case FBE_BASE_PHYSICAL_DRIVE_DEATH_REASON_PRICE_CLASS_MISMATCH:
        death_reason_desc = "FBE_BASE_PHYSICAL_DRIVE_DEATH_REASON_PRICE_CLASS_MISMATCH";
        break;
    case FBE_BASE_PHYSICAL_DRIVE_DEATH_REASON_ENHANCED_QUEUING_NOT_SUPPORTED:
        death_reason_desc = "FBE_BASE_PHYSICAL_DRIVE_DEATH_REASON_ENHANCED_QUEUING_NOT_SUPPORTED";
        break;
    case FBE_BASE_PHYSICAL_DRIVE_DEATH_REASON_SIMULATED:
        death_reason_desc = "FBE_BASE_PHYSICAL_DRIVE_DEATH_REASON_SIMULATED";
        break;
    case FBE_BASE_PHYSICAL_DRIVE_DEATH_REASON_CRC_MULTI_BIT:
        death_reason_desc = "FBE_BASE_PHYSICAL_DRIVE_DEATH_REASON_CRC_MULTI_BIT";
        break;
    case FBE_BASE_PHYSICAL_DRIVE_DEATH_REASON_CRC_SINGLE_BIT:
        death_reason_desc = "FBE_BASE_PHYSICAL_DRIVE_DEATH_REASON_CRC_SINGLE_BITSingle-bit CRC";
        break;
    case FBE_BASE_PHYSICAL_DRIVE_DEATH_REASON_CRC_OTHER:
        death_reason_desc = "FBE_BASE_PHYSICAL_DRIVE_DEATH_REASON_CRC_OTHER";
        break;
    case FBE_BASE_PHYSICAL_DRIVE_DEATH_REASON_SPECIALIZED_TIMER_EXPIRED:
        death_reason_desc = "FBE_BASE_PHYSICAL_DRIVE_DEATH_REASON_SPECIALIZED_TIMER_EXPIRED";
        break;
    case FBE_BASE_PHYSICAL_DRIVE_DEATH_REASON_LINK_THRESHOLD_EXCEEDED:
        death_reason_desc = "FBE_BASE_PHYSICAL_DRIVE_DEATH_REASON_LINK_THRESHOLD_EXCEEDED";
        break;
    case FBE_BASE_PHYSICAL_DRIVE_DEATH_REASON_RECOVERED_THRESHOLD_EXCEEDED:
        death_reason_desc = "FBE_BASE_PHYSICAL_DRIVE_DEATH_REASON_RECOVERED_THRESHOLD_EXCEEDED";
        break;
    case FBE_BASE_PHYSICAL_DRIVE_DEATH_REASON_MEDIA_THRESHOLD_EXCEEDED:
        death_reason_desc = "FBE_BASE_PHYSICAL_DRIVE_DEATH_REASON_MEDIA_THRESHOLD_EXCEEDED";
        break;
    case FBE_BASE_PHYSICAL_DRIVE_DEATH_REASON_HARDWARE_THRESHOLD_EXCEEDED:
        death_reason_desc = "FBE_BASE_PHYSICAL_DRIVE_DEATH_REASON_HARDWARE_THRESHOLD_EXCEEDED";
        break;
    case FBE_BASE_PHYSICAL_DRIVE_DEATH_REASON_HEALTHCHECK_THRESHOLD_EXCEEDED:
        death_reason_desc = "FBE_BASE_PHYSICAL_DRIVE_DEATH_REASON_HEALTHCHECK_THRESHOLD_EXCEEDED";
        break;
    case FBE_BASE_PHYSICAL_DRIVE_DEATH_REASON_DATA_THRESHOLD_EXCEEDED:
        death_reason_desc = "FBE_BASE_PHYSICAL_DRIVE_DEATH_REASON_DATA_THRESHOLD_EXCEEDED";
        break;
    case FBE_BASE_PHYSICAL_DRIVE_DEATH_REASON_FATAL_ERROR:
        death_reason_desc = "FBE_BASE_PHYSICAL_DRIVE_DEATH_REASON_FATAL_ERROR";
        break;

    case FBE_BASE_PHYSICAL_DRIVE_DEATH_REASON_ENCLOSURE_OBJECT_FAILED:
        death_reason_desc = "FBE_BASE_PHYSICAL_DRIVE_DEATH_REASON_ENCLOSURE_OBJECT_FAILED";
        break;

    case FBE_BASE_PHYSICAL_DRIVE_DEATH_REASON_INVALID_VAULT_CONFIGURATION:
        death_reason_desc = "FBE_BASE_PHYSICAL_DRIVE_DEATH_REASON_INVALID_VAULT_CONFIGURATION";
        break;

    case FBE_BASE_PHYSICAL_DRIVE_DEATH_REASON_INVALID_ID:
        death_reason_desc = "FBE_BASE_PHYSICAL_DRIVE_DEATH_REASON_INVALID_ID";
        break;

    // SAS death reason
    case FBE_SAS_PHYSICAL_DRIVE_DEATH_REASON_DEFECT_LIST_ERRORS:
        death_reason_desc = "FBE_SAS_PHYSICAL_DRIVE_DEATH_REASON_DEFECT_LIST_ERRORS";
        break;
    case FBE_SAS_PHYSICAL_DRIVE_DEATH_REASON_MAX_REMAPS_EXCEEDED:
        death_reason_desc = "FBE_SAS_PHYSICAL_DRIVE_DEATH_REASON_MAX_REMAPS_EXCEEDED";
        break;
    case FBE_SAS_PHYSICAL_DRIVE_DEATH_REASON_FW_UPGRADE_REQUIRED:
        death_reason_desc = "FBE_SAS_PHYSICAL_DRIVE_DEATH_REASON_FW_UPGRADE_REQUIRED";
        break;
    case FBE_SAS_PHYSICAL_DRIVE_DEATH_REASON_FAILED_TO_SET_DEV_INFO:
        death_reason_desc = "FBE_SAS_PHYSICAL_DRIVE_DEATH_REASON_FAILED_TO_SET_DEV_INFO";
        break;
    case FBE_SAS_PHYSICAL_DRIVE_DEATH_REASON_FAILED_TO_PROCESS_MODE_SENSE:
        death_reason_desc = "FBE_SAS_PHYSICAL_DRIVE_DEATH_REASON_FAILED_TO_PROCESS_MODE_SENSE";
        break;
    case FBE_SAS_PHYSICAL_DRIVE_DEATH_REASON_FAILED_TO_PROCESS_READ_CAPACITY:
        death_reason_desc = "FBE_SAS_PHYSICAL_DRIVE_DEATH_REASON_FAILED_TO_PROCESS_READ_CAPACITY";
        break;
    case FBE_SAS_PHYSICAL_DRIVE_DEATH_REASON_FAILED_TO_REASSIGN:
        death_reason_desc = "FBE_SAS_PHYSICAL_DRIVE_DEATH_REASON_FAILED_TO_REASSIGN";
        break;
    case FBE_SAS_PHYSICAL_DRIVE_DEATH_REASON_DRIVE_NOT_SPINNING:
        death_reason_desc = "FBE_SAS_PHYSICAL_DRIVE_DEATH_REASON_DRIVE_NOT_SPINNING";
        break;
    case FBE_SAS_PHYSICAL_DRIVE_DEATH_REASON_FAILED_TO_SPIN_DOWN:
        death_reason_desc = "FBE_SAS_PHYSICAL_DRIVE_DEATH_REASON_FAILED_TO_SPIN_DOWN";
        break;
    case FBE_SAS_PHYSICAL_DRIVE_DEATH_REASON_WRITE_PROTECT:
        death_reason_desc = "FBE_SAS_PHYSICAL_DRIVE_DEATH_REASON_WRITE_PROTECT";
        break;
    case FBE_SAS_PHYSICAL_DRIVE_DEATH_REASON_UNEXPECTED_FAILURE:
        death_reason_desc = "FBE_SAS_PHYSICAL_DRIVE_DEATH_REASON_UNEXPECTED_FAILURE";
        break;
    case FBE_SAS_PHYSICAL_DRIVE_DEATH_REASON_DC_FLUSH_FAILED:
        death_reason_desc = "FBE_SAS_PHYSICAL_DRIVE_DEATH_REASON_DC_FLUSH_FAILED";
        break;
    case FBE_SAS_PHYSICAL_DRIVE_DEATH_REASON_HEALTHCHECK_FAILED:
        death_reason_desc = "FBE_SAS_PHYSICAL_DRIVE_DEATH_REASON_HEALTHCHECK_FAILED";
        break;
    case FBE_SAS_PHYSICAL_DRIVE_DEATH_REASON_FW_UPGRADE_FAILED:
        death_reason_desc = "FBE_SAS_PHYSICAL_DRIVE_DEATH_REASON_FW_UPGRADE_FAILED";
        break;

    // Discovered death reason
    case FBE_BASE_DISCOVERED_DEATH_REASON_ACTIVATE_TIMER_EXPIRED:
        death_reason_desc = "FBE_BASE_DISCOVERED_DEATH_REASON_ACTIVATE_TIMER_EXPIRED";
        break;
    case FBE_BASE_PHYSICAL_DRIVE_DEATH_REASON_ACTIVATE_TIMER_EXPIRED:
        death_reason_desc = "FBE_BASE_PHYSICAL_DRIVE_DEATH_REASON_ACTIVATE_TIMER_EXPIRED";
        break;
    case FBE_BASE_DISCOVERED_DEATH_REASON_HARDWARE_NOT_READY:
        death_reason_desc = "FBE_BASE_DISCOVERED_DEATH_REASON_HARDWARE_NOT_READY";
        break;

    // SATA death reason
    case FBE_SATA_PHYSICAL_DRIVE_DEATH_REASON_INVALID_INSCRIPTION:
        death_reason_desc = "FBE_SATA_PHYSICAL_DRIVE_DEATH_REASON_INVALID_INSCRIPTION";
        break;
    case FBE_SATA_PHYSICAL_DRIVE_DEATH_REASON_PIO_MODE_UNSUPPORTED:
        death_reason_desc = "FBE_SATA_PHYSICAL_DRIVE_DEATH_REASON_PIO_MODE_UNSUPPORTED";
        break;
    case FBE_SATA_PHYSICAL_DRIVE_DEATH_REASON_UDMA_MODE_UNSUPPORTED:
        death_reason_desc = "FBE_SATA_PHYSICAL_DRIVE_DEATH_REASON_UDMA_MODE_UNSUPPORTED";
        break;
    case FBE_BASE_ENCLOSURE_DEATH_REASON_FAULTY_COMPONENT:
        death_reason_desc = "FBE_BASE_ENCLOSURE_DEATH_REASON_FAULTY_COMPONENT";
        break;

    // DMO death reason
    case DMO_DRIVE_OFFLINE_REASON_PERSISTED_EOL:
        death_reason_desc = "DMO_DRIVE_OFFLINE_REASON_PERSISTED_EOL";
        break;
    case DMO_DRIVE_OFFLINE_REASON_PERSISTED_DF:
        death_reason_desc = "DMO_DRIVE_OFFLINE_REASON_PERSISTED_DF";
        break;
    case DMO_DRIVE_OFFLINE_REASON_DRIVE_REMOVED:
        death_reason_desc = "DMO_DRIVE_OFFLINE_REASON_DRIVE_REMOVED";
        break;
    case DMO_DRIVE_OFFLINE_REASON_NON_EQ:
        death_reason_desc = "DMO_DRIVE_OFFLINE_REASON_NON_EQ";
        break;
    case DMO_DRIVE_OFFLINE_REASON_INVALID_ID:
        death_reason_desc = "DMO_DRIVE_OFFLINE_REASON_INVALID_ID";
        break;
    case DMO_DRIVE_OFFLINE_REASON_SSD_LE:
        death_reason_desc = "DMO_DRIVE_OFFLINE_REASON_SSD_LE";
        break;
    case DMO_DRIVE_OFFLINE_REASON_SSD_RI:
        death_reason_desc = "DMO_DRIVE_OFFLINE_REASON_SSD_RI";
        break;
    case DMO_DRIVE_OFFLINE_REASON_HDD_520:
        death_reason_desc = "DMO_DRIVE_OFFLINE_REASON_HDD_520";
        break;
    case DMO_DRIVE_OFFLINE_REASON_UNKNOWN_FAILURE:
        death_reason_desc = "DMO_DRIVE_OFFLINE_REASON_UNKNOWN_FAILURE";
        break;

    // Not set death reason
    case DMO_DRIVE_OFFLINE_REASON_INVALID:
        death_reason_desc = "DMO_DRIVE_OFFLINE_REASON_INVALID";
        break;
    case FBE_BASE_DISCOVERED_DEATH_REASON_INVALID:
        death_reason_desc = "FBE_BASE_DISCOVERED_DEATH_REASON_INVALID";
        break;
    case FBE_BASE_PHYSICAL_DRIVE_DEATH_REASON_INVALID:
        death_reason_desc = "FBE_BASE_PHYSICAL_DRIVE_DEATH_REASON_INVALID";
        break;
    case FBE_BASE_ENCLOSURE_DEATH_REASON_INVALID:
        death_reason_desc = "FBE_BASE_ENCLOSURE_DEATH_REASON_INVALID";
        break;
    case FBE_DEATH_REASON_INVALID:
        death_reason_desc = "FBE_DEATH_REASON_INVALID";
        break;

    // Default:
    default:
        death_reason_desc = "Unknown Death Reason";
        break;
    }

    return death_reason_desc;
} //End of fbe_api_esp_drive_mgmt_map_death_reason_to_str

/*!***************************************************************
 * @fn fbe_api_esp_write_bms_logpage_data_asynch
 ****************************************************************
 * @brief
 *  This function sends the logpage data to ESP.
 *
 * @param 
 *   bms_op_p -  BMS logpage op information.
 * 
 * @return 
 *   fbe_status_t - status .
 * 
 * @version
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_esp_write_bms_logpage_data_asynch(fbe_physical_drive_bms_op_asynch_t   *bms_op_p)                                           
{
    fbe_status_t                            status = FBE_STATUS_OK;    
    fbe_u32_t                               sg_item = 0;
    fbe_packet_t                            * packet = NULL;

    if (bms_op_p == NULL || bms_op_p->esp_object_id >= FBE_OBJECT_ID_INVALID) 
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: ObjID Invalid or bms op NULL\n", __FUNCTION__); 
        return FBE_STATUS_GENERIC_FAILURE;
    }  

    if (bms_op_p->completion_function == NULL || bms_op_p->response_buf == NULL) 
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: Must have a valid completion function and response buf\n", __FUNCTION__); 
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_sg_element_init(&(bms_op_p->sg_element[sg_item]), 
                        bms_op_p->rw_buffer_length, 
                        bms_op_p->response_buf);
    sg_item++;
    fbe_sg_element_terminate(&bms_op_p->sg_element[sg_item]);
    sg_item++;

    packet = fbe_api_get_contiguous_packet();//no need to touch or initialize, it's taken from a pool and taken care of
    if (packet == NULL) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: unable to allocate memory for packet\n", __FUNCTION__); 
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_common_send_control_packet_with_sg_list_async(packet,
                                                                   FBE_ESP_DRIVE_MGMT_CONTROL_CODE_WRITE_LOG_PAGE,
                                                                   bms_op_p,
                                                                   sizeof(fbe_physical_drive_bms_op_asynch_t),                                           
                                                                   bms_op_p->esp_object_id, 
                                                                   FBE_PACKET_FLAG_NO_ATTRIB,
                                                                   bms_op_p->sg_element, 
                                                                   sg_item,
                                                                   fbe_api_esp_write_bms_logpage_data_asynch_callback, 
                                                                   bms_op_p,
                                                                   FBE_PACKAGE_ID_ESP);

    if (status != FBE_STATUS_OK && status != FBE_STATUS_PENDING){
 
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s failed, status: 0x%x.\n", __FUNCTION__, status); 
        return status;
    }

    return status;
} // end fbe_api_esp_write_bms_logpage_data_asynch


/*********************************************************************
 * @fn fbe_api_esp_write_bms_logpage_data_asynch_callback()
 *********************************************************************
 * @brief:
 *  This is the write BMS logpage asynch callback for the nice format data.
 *
 *  @param packet - pointer to fbe_packet_t.
 *  @param context - packet completion context
 *
 *  @return fbe_status_t, success or failure
 *
 *  History:
 *********************************************************************/
static fbe_status_t fbe_api_esp_write_bms_logpage_data_asynch_callback (fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_physical_drive_bms_op_asynch_t   *bms_op_p = (fbe_physical_drive_bms_op_asynch_t *)context;
    fbe_payload_ex_t *                     payload = NULL;
    fbe_payload_control_operation_t *   control_operation = NULL;
    fbe_payload_control_status_t control_status = FBE_PAYLOAD_CONTROL_STATUS_OK;
    fbe_payload_control_status_qualifier_t control_status_qualifier = 0;


    status = fbe_transport_get_status_code(packet);
    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);
    fbe_payload_control_get_status(control_operation, &control_status);
    fbe_payload_control_get_status_qualifier(control_operation, &control_status_qualifier);

    if ((status != FBE_STATUS_OK) || 
        (control_status != FBE_PAYLOAD_CONTROL_STATUS_OK))
    {
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, "%s:status:%d, payload status:%d & qulf:%d, page 0x%X\n", __FUNCTION__,
                       status, control_status, control_status_qualifier, bms_op_p->get_log_page.page_code);

        status = FBE_STATUS_COMPONENT_NOT_FOUND;
    }          

    // Fill the status
    bms_op_p->status = status;

    fbe_api_return_contiguous_packet(packet);//no need to destory or free it's returned to a queue and reused

    // call callback fundtion
    (bms_op_p->completion_function) (context);

    return status;

} // end of fbe_api_esp_write_bms_logpage_data_asynch_callback

/**********************************************
 * end file fbe_api_esp_drive_mgmt_interface.c
**********************************************/
