/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_api_esp_common_interface.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the base_environment interface for the Environmental Services package.
 *
 * @ingroup fbe_api_esp_interface_class_files
 * @ingroup fbe_api_esp_common_interface
 *
 * @version
 *   25-Jul-2010:  PHE - Created. 
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_types.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_common_transport.h"
#include "fbe/fbe_esp_base_environment.h"
#include "fbe/fbe_api_esp_common_interface.h"
#include "fbe/fbe_api_esp_sps_mgmt_interface.h"
#include "fbe/fbe_api_esp_ps_mgmt_interface.h"
#include "fbe/fbe_api_esp_encl_mgmt_interface.h"
#include "fbe/fbe_api_esp_board_mgmt_interface.h"
#include "fbe/fbe_api_esp_cooling_mgmt_interface.h"
#include "fbe/fbe_api_board_interface.h"

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

/*!***************************************************************
 * @fn fbe_api_esp_common_get_object_id_by_device_type(fbe_u64_t deviceType,
 *                                      fbe_object_id_t * pOjectId)
 ****************************************************************
 * @brief
 *  This function gets the object id based on the device type.
 *
 * @param deviceType - 
 * @param pOjectId (OUTPUT) -
 * 
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  22-Jul-2010: PHE - Created.
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_esp_common_get_object_id_by_device_type(fbe_u64_t deviceType, fbe_object_id_t * pOjectId)
{
    fbe_class_id_t      classId;  
    fbe_status_t        status;

    switch(deviceType)
    {
        case FBE_DEVICE_TYPE_PS:
            classId = FBE_CLASS_ID_PS_MGMT;
            break;

        case FBE_DEVICE_TYPE_ENCLOSURE:
        case FBE_DEVICE_TYPE_LCC:
        case FBE_DEVICE_TYPE_DRIVE_MIDPLANE:
            classId = FBE_CLASS_ID_ENCL_MGMT;
            break;

        case FBE_DEVICE_TYPE_FAN:
            classId = FBE_CLASS_ID_COOLING_MGMT;
            break;

        case FBE_DEVICE_TYPE_SPS:
        case FBE_DEVICE_TYPE_BATTERY:
            classId = FBE_CLASS_ID_SPS_MGMT;
            break;

        case FBE_DEVICE_TYPE_IOMODULE:
        case FBE_DEVICE_TYPE_BACK_END_MODULE:
        case FBE_DEVICE_TYPE_MEZZANINE:
        case FBE_DEVICE_TYPE_MGMT_MODULE:
            classId = FBE_CLASS_ID_MODULE_MGMT;
            break;
    

        case FBE_DEVICE_TYPE_DRIVE:
            classId = FBE_CLASS_ID_DRIVE_MGMT;
            break;


        case FBE_DEVICE_TYPE_SP:
        case FBE_DEVICE_TYPE_SLAVE_PORT:
        case FBE_DEVICE_TYPE_PLATFORM:
        case FBE_DEVICE_TYPE_SUITCASE:
        case FBE_DEVICE_TYPE_MISC:
        case FBE_DEVICE_TYPE_BMC:
        case FBE_DEVICE_TYPE_CACHE_CARD:
        case FBE_DEVICE_TYPE_SSD:
            classId = FBE_CLASS_ID_BOARD_MGMT;
            break;
    
        default:
            fbe_api_trace (FBE_TRACE_LEVEL_ERROR, 
                "%s:unsupported deviceType 0x%llX.\n", 
                __FUNCTION__, (fbe_u64_t)deviceType);
            return FBE_STATUS_GENERIC_FAILURE;
            break; 
    }

    /* Get the object ID. */
    status = fbe_api_esp_getObjectId(classId, pOjectId);
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, 
            "%s:fbe_api_esp_getObjectId for deviceType 0x%llX failed, status 0x%X.\n", 
            __FUNCTION__, (fbe_u64_t)deviceType, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return FBE_STATUS_OK;
}

/*!***************************************************************
 * @fn fbe_api_esp_common_get_any_upgrade_in_progress(fbe_u64_t  deviceType,
 *                                                    fbe_bool_t * pAnyUpgradeInProgress)
 ****************************************************************
 * @brief
 *  This function will return whether there is any firmware upgrade
 *  in progress for the specified device type.
 *
 * @param deviceType -
 * @param pAnyUpgradeInProgress (OUTPUT) - 
 * 
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  22-Jul-2010: PHE - Created.
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_esp_common_get_any_upgrade_in_progress(fbe_u64_t  deviceType,
                                               fbe_bool_t * pAnyUpgradeInProgress)
{
    fbe_object_id_t                                 objectId;
    fbe_status_t                                    status;
    fbe_api_control_operation_status_info_t         statusInfo;
    fbe_esp_base_env_get_any_upgrade_in_progress_t  getAnyUpgradeInProgress = {0};

    status = fbe_api_esp_common_get_object_id_by_device_type(deviceType, &objectId);
   
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, 
            "%s: deviceType 0x%llX, failed to get the object id, status 0x%X.\n", 
            __FUNCTION__, (fbe_u64_t)deviceType, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    getAnyUpgradeInProgress.deviceType = deviceType;

    status = fbe_api_common_send_control_packet (FBE_ESP_BASE_ENV_CONTROL_CODE_GET_ANY_UPGRADE_IN_PROGRESS,
                                                 &getAnyUpgradeInProgress,
                                                 sizeof(fbe_esp_base_env_get_any_upgrade_in_progress_t),
                                                 objectId,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &statusInfo,
                                                 FBE_PACKAGE_ID_ESP);

    if (status != FBE_STATUS_OK || statusInfo.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) 
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, 
            "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", 
            __FUNCTION__, status, 
            statusInfo.packet_qualifier, statusInfo.control_operation_status, statusInfo.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    *pAnyUpgradeInProgress = getAnyUpgradeInProgress.anyUpgradeInProgress;

    return status;

}   // end of fbe_api_esp_common_get_any_upgrade_in_progress

/*!***************************************************************
 * @fn fbe_api_esp_common_initiate_upgrade(fbe_u64_t deviceType,
 *                                   fbe_device_physical_location_t * pLocation,
 *                                   fbe_base_env_fup_force_flags_t forceFlags)
 ****************************************************************
 * @brief
 *  This function will send control code to initiate the firmware upgrade
 *  for the specified device.
 *
 * @param deviceType -
 * @param pLocation - The pointer to the device location.
 * @param forceFlags - 
 * 
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  22-Jul-2010: PHE - Created.
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_esp_common_initiate_upgrade(fbe_u64_t deviceType,
                                    fbe_device_physical_location_t * pLocation,
                                    fbe_base_env_fup_force_flags_t forceFlags)
{
    fbe_object_id_t                                 objectId;
    fbe_status_t                                    status;
    fbe_api_control_operation_status_info_t         statusInfo;
    fbe_esp_base_env_initiate_upgrade_t             initiateUpgrade = {0};
    
    status = fbe_api_esp_common_get_object_id_by_device_type(deviceType, &objectId);
   
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, 
            "%s: deviceType 0x%llX, failed to get the object id, status 0x%X.\n", 
            __FUNCTION__, (fbe_u64_t)deviceType, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    initiateUpgrade.deviceType = deviceType;
    initiateUpgrade.location = *pLocation;
    initiateUpgrade.forceFlags = forceFlags;

    status = fbe_api_common_send_control_packet (FBE_ESP_BASE_ENV_CONTROL_CODE_INITIATE_UPGRADE,
                                                 &initiateUpgrade,
                                                 sizeof(fbe_esp_base_env_initiate_upgrade_t),
                                                 objectId,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &statusInfo,
                                                 FBE_PACKAGE_ID_ESP);

    if (status != FBE_STATUS_OK || statusInfo.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) 
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, 
            "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", 
            __FUNCTION__, status, 
            statusInfo.packet_qualifier, statusInfo.control_operation_status, statusInfo.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;

}   // end of fbe_api_esp_common_initiate_upgrade


/*!***************************************************************
 * @fn fbe_api_esp_common_terminate_upgrade(void)
 ****************************************************************
 * @brief
 *  This function will send control code to terminate the firmware upgrade
 *  for all the devices other than the drives.
 *
 * 
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 * 
 * @note
 *   It does not include the drive upgrade.
 * 
 * @version
 *  04-Jan-2013: PHE - Created.
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_esp_common_terminate_upgrade(void)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_status_t returnValue;

    returnValue = status;
    status = fbe_api_esp_common_terminate_upgrade_by_class_id(FBE_CLASS_ID_COOLING_MGMT);
    if(status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, 
            "%s:failed to abort upgrade in FBE_CLASS_ID_COOLING_MGMT, status 0x%X.\n", __FUNCTION__, status);
        returnValue = FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_esp_common_terminate_upgrade_by_class_id(FBE_CLASS_ID_PS_MGMT);
    if(status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, 
            "%s:failed to abort upgrade in FBE_CLASS_ID_PS_MGMT, status 0x%X.\n", __FUNCTION__, status);
        returnValue = FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_esp_common_terminate_upgrade_by_class_id(FBE_CLASS_ID_ENCL_MGMT);
    if(status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, 
            "%s:failed to abort upgrade in FBE_CLASS_ID_ENCL_MGMT, status 0x%X.\n", __FUNCTION__, status);
        returnValue = FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_esp_common_terminate_upgrade_by_class_id(FBE_CLASS_ID_SPS_MGMT);
    if(status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, 
            "%s:failed to abort upgrade in FBE_CLASS_ID_SPS_MGMT, status 0x%X.\n", __FUNCTION__, status);
        returnValue = FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_esp_common_terminate_upgrade_by_class_id(FBE_CLASS_ID_MODULE_MGMT);
    if(status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, 
            "%s:failed to abort upgrade in FBE_CLASS_ID_MODULE_MGMT, status 0x%X.\n", __FUNCTION__, status);
        returnValue = FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_esp_common_terminate_upgrade_by_class_id(FBE_CLASS_ID_BOARD_MGMT);
    if(status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, 
            "%s:failed to abort upgrade in FBE_CLASS_ID_BOARD_MGMT, status 0x%X.\n", __FUNCTION__, status);
        returnValue = FBE_STATUS_GENERIC_FAILURE;
    }

    return returnValue;

}   // end of fbe_api_esp_common_abort_upgrade

/*!***************************************************************
 * @fn fbe_api_esp_common_abort_upgrade(void)
 ****************************************************************
 * @brief
 *  This function will send control code to abort the firmware upgrade
 *  for all the devices other than the drives.
 *
 * 
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 * 
 * @note
 *   It does not include the drive upgrade.
 * 
 * @version
 *  01-Sept-2010: PHE - Created.
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_esp_common_abort_upgrade(void)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_status_t returnValue;

    returnValue = status;
    status = fbe_api_esp_common_abort_upgrade_by_class_id(FBE_CLASS_ID_COOLING_MGMT);
    if(status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, 
            "%s:failed to abort upgrade in FBE_CLASS_ID_COOLING_MGMT, status 0x%X.\n", __FUNCTION__, status);
        returnValue = FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_esp_common_abort_upgrade_by_class_id(FBE_CLASS_ID_PS_MGMT);
    if(status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, 
            "%s:failed to abort upgrade in FBE_CLASS_ID_PS_MGMT, status 0x%X.\n", __FUNCTION__, status);
        returnValue = FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_esp_common_abort_upgrade_by_class_id(FBE_CLASS_ID_ENCL_MGMT);
    if(status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, 
            "%s:failed to abort upgrade in FBE_CLASS_ID_ENCL_MGMT, status 0x%X.\n", __FUNCTION__, status);
        returnValue = FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_esp_common_abort_upgrade_by_class_id(FBE_CLASS_ID_SPS_MGMT);
    if(status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, 
            "%s:failed to abort upgrade in FBE_CLASS_ID_SPS_MGMT, status 0x%X.\n", __FUNCTION__, status);
        returnValue = FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_esp_common_abort_upgrade_by_class_id(FBE_CLASS_ID_MODULE_MGMT);
    if(status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, 
            "%s:failed to abort upgrade in FBE_CLASS_ID_MODULE_MGMT, status 0x%X.\n", __FUNCTION__, status);
        returnValue = FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_esp_common_abort_upgrade_by_class_id(FBE_CLASS_ID_BOARD_MGMT);
    if(status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, 
            "%s:failed to abort upgrade in FBE_CLASS_ID_BOARD_MGMT, status 0x%X.\n", __FUNCTION__, status);
        returnValue = FBE_STATUS_GENERIC_FAILURE;
    }

    return returnValue;

}   // end of fbe_api_esp_common_abort_upgrade

/*!***************************************************************
 * @fn fbe_api_esp_common_resume_upgrade(void)
 ****************************************************************
 * @brief
 *  This function will send control code to resume the firmware upgrade
 *  for all the devices other than the drives.
 *
 * 
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 * 
 * @note
 *   It does not include the drive upgrade.
 * 
 * @version
 *  01-Sept-2010: PHE - Created.
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_esp_common_resume_upgrade(void)
{
    fbe_status_t status = FBE_STATUS_OK;    

    status = fbe_api_esp_common_resume_upgrade_by_class_id(FBE_CLASS_ID_COOLING_MGMT);
    if(status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, 
            "%s:failed to abort upgrade in FBE_CLASS_ID_COOLING_MGMT, status 0x%X.\n", __FUNCTION__, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_esp_common_resume_upgrade_by_class_id(FBE_CLASS_ID_PS_MGMT);
    if(status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, 
            "%s:failed to abort upgrade in FBE_CLASS_ID_PS_MGMT, status 0x%X.\n", __FUNCTION__, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_esp_common_resume_upgrade_by_class_id(FBE_CLASS_ID_ENCL_MGMT);
    if(status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, 
            "%s:failed to abort upgrade in FBE_CLASS_ID_ENCL_MGMT, status 0x%X.\n", __FUNCTION__, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_esp_common_resume_upgrade_by_class_id(FBE_CLASS_ID_SPS_MGMT);
    if(status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, 
            "%s:failed to abort upgrade in FBE_CLASS_ID_SPS_MGMT, status 0x%X.\n", __FUNCTION__, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_esp_common_resume_upgrade_by_class_id(FBE_CLASS_ID_MODULE_MGMT);
    if(status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, 
            "%s:failed to abort upgrade in FBE_CLASS_ID_MODULE_MGMT, status 0x%X.\n", __FUNCTION__, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_esp_common_resume_upgrade_by_class_id(FBE_CLASS_ID_BOARD_MGMT);
    if(status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, 
            "%s:failed to abort upgrade in FBE_CLASS_ID_BOARD_MGMT, status 0x%X.\n", __FUNCTION__, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;

}   // end of fbe_api_esp_common_resume_upgrade

/*!***************************************************************
 * @fn fbe_api_esp_common_terminate_upgrade_by_class_id(fbe_class_id_t classId)
 ****************************************************************
 * @brief
 *  This function will send control code to terminate the firmware upgrade
 *  for the specified class.
 *
 * @param deviceType -
 * 
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  04-Jan-2013: PHE - Created.
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_esp_common_terminate_upgrade_by_class_id(fbe_class_id_t classId)
{
    fbe_object_id_t                                 objectId;
    fbe_status_t                                    status;
    fbe_api_control_operation_status_info_t         statusInfo;
    
    status = fbe_api_esp_getObjectId(classId, &objectId);
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, 
            "%s: Getting objectId by classId failed, classId 0x%X, status 0x%X.\n", 
            __FUNCTION__, classId, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }
  
    status = fbe_api_common_send_control_packet (FBE_ESP_BASE_ENV_CONTROL_CODE_TERMINATE_UPGRADE,
                                                 NULL,
                                                 0,
                                                 objectId,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &statusInfo,
                                                 FBE_PACKAGE_ID_ESP);

    if (status != FBE_STATUS_OK || statusInfo.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) 
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, 
            "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", 
            __FUNCTION__, status, 
            statusInfo.packet_qualifier, statusInfo.control_operation_status, statusInfo.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;

}   // end of fbe_api_esp_common_terminate_upgrade_by_class_id


/*!***************************************************************
 * @fn fbe_api_esp_common_abort_upgrade_by_class_id(fbe_class_id_t classId)
 ****************************************************************
 * @brief
 *  This function will send control code to abort the firmware upgrade
 *  for the specified class.
 *
 * @param deviceType -
 * 
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  01-Sept-2010: PHE - Created.
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_esp_common_abort_upgrade_by_class_id(fbe_class_id_t classId)
{
    fbe_object_id_t                                 objectId;
    fbe_status_t                                    status;
    fbe_api_control_operation_status_info_t         statusInfo;
    
    status = fbe_api_esp_getObjectId(classId, &objectId);
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, 
            "%s: Getting objectId by classId failed, classId 0x%X, status 0x%X.\n", 
            __FUNCTION__, classId, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }
  
    status = fbe_api_common_send_control_packet (FBE_ESP_BASE_ENV_CONTROL_CODE_ABORT_UPGRADE,
                                                 NULL,
                                                 0,
                                                 objectId,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &statusInfo,
                                                 FBE_PACKAGE_ID_ESP);

    if (status != FBE_STATUS_OK || statusInfo.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) 
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, 
            "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", 
            __FUNCTION__, status, 
            statusInfo.packet_qualifier, statusInfo.control_operation_status, statusInfo.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;

}   // end of fbe_api_esp_common_abort_upgrade_by_class_id


/*!***************************************************************
 * @fn fbe_api_esp_common_resume_upgrade_by_class_id(fbe_class_id_t classId)
 ****************************************************************
 * @brief
 *  This function will send control code to resume the firmware upgrade
 *  for the specified class.
 *
 * @param classId -
 * 
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  01-Sept-2010: PHE - Created.
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_esp_common_resume_upgrade_by_class_id(fbe_class_id_t classId)
{
    fbe_object_id_t                                 objectId;
    fbe_status_t                                    status;
    fbe_api_control_operation_status_info_t         statusInfo;
    
    status = fbe_api_esp_getObjectId(classId, &objectId);
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, 
            "%s: Getting objectId by classId failed, classId 0x%X, status 0x%X.\n", 
            __FUNCTION__, classId, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_common_send_control_packet (FBE_ESP_BASE_ENV_CONTROL_CODE_RESUME_UPGRADE,
                                                 NULL,
                                                 0,
                                                 objectId,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &statusInfo,
                                                 FBE_PACKAGE_ID_ESP);

    if (status != FBE_STATUS_OK || statusInfo.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) 
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, 
            "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", 
            __FUNCTION__, status, 
            statusInfo.packet_qualifier, statusInfo.control_operation_status, statusInfo.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;

}   // end of fbe_api_esp_common_resume_upgrade_by_class_id


/*!***************************************************************
 * @fn fbe_api_esp_common_get_fup_work_state(fbe_esp_base_env_get_work_state_t * pGetWorkState)
 ****************************************************************
 * @brief
 *  This function will send control code to retrieve the firmware upgrade
 *  work state for the specified device.
 *
 * @param 
 *   pGetWorkState - The pointer to fbe_esp_base_env_get_work_state_t.
 * 
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  22-Jul-2010: PHE - Created.
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_esp_common_get_fup_work_state(fbe_u64_t deviceType,
                                      fbe_device_physical_location_t * pLocation,
                                      fbe_base_env_fup_work_state_t * pWorkState)
{
    fbe_object_id_t                                 objectId;
    fbe_status_t                                    status;
    fbe_api_control_operation_status_info_t         statusInfo;
    fbe_esp_base_env_get_fup_work_state_t           getWorkState = {0};

    status = fbe_api_esp_common_get_object_id_by_device_type(deviceType, &objectId);
   
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, 
            "%s: deviceType 0x%llX, failed to get the object id, status 0x%X.\n", 
            __FUNCTION__, (fbe_u64_t)deviceType, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    getWorkState.deviceType = deviceType;
    getWorkState.location = *pLocation;

    status = fbe_api_common_send_control_packet (FBE_ESP_BASE_ENV_CONTROL_CODE_GET_FUP_WORK_STATE,
                                                 &getWorkState,
                                                 sizeof(fbe_esp_base_env_get_fup_work_state_t),
                                                 objectId,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &statusInfo,
                                                 FBE_PACKAGE_ID_ESP);

    if (status != FBE_STATUS_OK || statusInfo.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) 
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, 
            "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", 
            __FUNCTION__, status, 
            statusInfo.packet_qualifier, statusInfo.control_operation_status, statusInfo.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    *pWorkState = getWorkState.workState;
    return status;

}   // end of fbe_api_esp_common_get_fup_work_state

/*!***************************************************************
 * @fn fbe_api_esp_common_get_fup_info(fbe_esp_base_env_get_fup_info_t * pGetFupInfo)
 * ****************************************************************
 * @brief
 *  This function will send control code to retrieve the firmware upgrade
 *  related information for the specified device.
 *
 * @param pGetFupInfo - 
 * 
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  22-Jul-2010: PHE - Created.
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_esp_common_get_fup_info(fbe_esp_base_env_get_fup_info_t * pGetFupInfo)
{
    fbe_object_id_t                                 objectId;
    fbe_status_t                                    status;
    fbe_api_control_operation_status_info_t         statusInfo;

    status = fbe_api_esp_common_get_object_id_by_device_type(pGetFupInfo->deviceType, &objectId);
   
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, 
            "%s: deviceType 0x%llX, failed to get the object id, status 0x%X.\n", 
            __FUNCTION__, (fbe_u64_t)pGetFupInfo->deviceType, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_common_send_control_packet (FBE_ESP_BASE_ENV_CONTROL_CODE_GET_FUP_INFO,
                                                 pGetFupInfo,
                                                 sizeof(fbe_esp_base_env_get_fup_info_t),
                                                 objectId,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &statusInfo,
                                                 FBE_PACKAGE_ID_ESP);

    if (status != FBE_STATUS_OK || statusInfo.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) 
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, 
            "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", 
            __FUNCTION__, status, 
            statusInfo.packet_qualifier, statusInfo.control_operation_status, statusInfo.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;

}   // end of fbe_api_esp_common_get_fup_info

/*!***************************************************************
 * @fn fbe_api_esp_common_get_sp_id(fbe_esp_module_mgmt_sp_t *sp_id)
 ****************************************************************
 * @brief
 *  This function will send control code to retrieve the sp id
 *
 * @param 
 *   fbe_u32_t               sp_id
 * 
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  02/03/11 - Created. Rashmi Sawale
 *  

 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_esp_common_get_sp_id(fbe_class_id_t classId,fbe_base_env_sp_t *sp_id)
{
    fbe_object_id_t                                 object_id;
    fbe_status_t                                    status;
    fbe_api_control_operation_status_info_t         status_info;
    fbe_esp_base_env_get_sp_id_t                      fbe_esp_common_sp_id;

    status = fbe_api_esp_getObjectId(classId, &object_id);

    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, 
            "%s: Getting objectId by classId failed, classId 0x%X, status 0x%X.\n", 
            __FUNCTION__, classId, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_common_send_control_packet (FBE_ESP_BASE_ENV_CONTROL_CODE_GET_SP_ID,
                                                 &fbe_esp_common_sp_id,
                                                 sizeof(fbe_esp_base_env_get_sp_id_t),
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

    *sp_id = fbe_esp_common_sp_id.sp_id;
    return status;
}

/*!***************************************************************
 * @fn fbe_api_esp_common_get_resume_prom_info(fbe_esp_base_env_get_work_state_t * pGetWorkState)
 ****************************************************************
 * @brief
 *  This function will send control code to retrieve the resume prom info
 *  for the specified device.
 *
 * @param 
 *   pGetResumePromInfo - The pointer to fbe_esp_base_env_get_resume_prom_info_cmd_t.
 * 
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  28-Oct-2010: Arun S - Created.
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_esp_common_get_resume_prom_info(fbe_esp_base_env_get_resume_prom_info_cmd_t * pGetResumePromInfo)
{
    fbe_object_id_t                                 objectId = FBE_OBJECT_ID_INVALID;
    fbe_status_t                                    status = FBE_STATUS_OK;
    fbe_api_control_operation_status_info_t         statusInfo = {0};

    status = fbe_api_esp_common_get_object_id_by_device_type(pGetResumePromInfo->deviceType, &objectId);
   
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, 
            "%s: deviceType 0x%llX, failed to get the object id, status 0x%X.\n", 
            __FUNCTION__, (fbe_u64_t)pGetResumePromInfo->deviceType, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_common_send_control_packet (FBE_ESP_BASE_ENV_CONTROL_CODE_GET_RESUME_PROM_INFO,
                                                 pGetResumePromInfo,
                                                 sizeof(fbe_esp_base_env_get_resume_prom_info_cmd_t),
                                                 objectId,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &statusInfo,
                                                 FBE_PACKAGE_ID_ESP);

    if (status != FBE_STATUS_OK || statusInfo.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) 
    {
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, 
            "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", 
            __FUNCTION__, status, 
            statusInfo.packet_qualifier, statusInfo.control_operation_status, statusInfo.control_operation_qualifier);

        if ((status == FBE_STATUS_NO_DEVICE) || (status == FBE_STATUS_COMPONENT_NOT_FOUND))
        {
            return status;
        }
        else
        {
            return FBE_STATUS_GENERIC_FAILURE;
        }
        
    }

    return status;

}   // end of fbe_api_esp_common_get_resume_prom_info

/*!***************************************************************
 * @fn fbe_api_esp_common_write_resume_prom(fbe_resume_prom_cmd_t * pWriteResumePromCmd)
 ****************************************************************
 * @brief
 *  This function will write the resume prom.
 *
 * @param 
 *   pWriteResumePromCmd - The pointer to fbe_esp_base_env_resume_prom_cmd_t.
 * 
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  19-Jul-2011: PHE - Created.
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_esp_common_write_resume_prom(fbe_resume_prom_cmd_t * pWriteResumePromCmd)
{
    fbe_object_id_t                             objectId = FBE_OBJECT_ID_INVALID;
    fbe_u8_t                                    sg_count = 0;
    fbe_api_control_operation_status_info_t     status_info = {0};
    fbe_status_t                                status = FBE_STATUS_OK;
    fbe_sg_element_t                            sg_element[2];

    EmcpalZeroVariable(sg_element);
    if (pWriteResumePromCmd == NULL) 
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: Command buffer is NULL\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if (( pWriteResumePromCmd->pBuffer == NULL ) &&
        (pWriteResumePromCmd->resumePromField != RESUME_PROM_CHECKSUM))
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: write buffer is NULL\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_esp_common_get_object_id_by_device_type(pWriteResumePromCmd->deviceType, &objectId);
   
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, 
            "%s: deviceType 0x%llX, failed to get the object id, status 0x%X.\n", 
            __FUNCTION__, (fbe_u64_t)pWriteResumePromCmd->deviceType, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Set the element size and address for the data buffer */
    fbe_sg_element_init(&sg_element[sg_count], 
                        pWriteResumePromCmd->bufferSize, 
                        pWriteResumePromCmd->pBuffer);
    sg_count++;

    /* No more elements. Terminate it. */
    fbe_sg_element_terminate(&sg_element[sg_count]);
    sg_count++;

    status = fbe_api_common_send_control_packet_with_sg_list(FBE_ESP_BASE_ENV_CONTROL_CODE_WRITE_RESUME_PROM,
                                                             pWriteResumePromCmd,
                                                             sizeof (fbe_resume_prom_cmd_t),
                                                             objectId,
                                                             FBE_PACKET_FLAG_NO_ATTRIB,
                                                             sg_element,
                                                             sg_count,
                                                             &status_info,
                                                             FBE_PACKAGE_ID_ESP);

   /* To compensate for possible corruption when sending from user to kernel, 
    * we restore the original pointers.
    * In the case of user space to kernel space, 
    * the data_buf_p would be overwritten by the kernel space address 
    * we need to copy back the user space address here in case the call needs to retrieve it. 
    */
    pWriteResumePromCmd->pBuffer = sg_element[0].address;

   if ((status != FBE_STATUS_OK) || 
       (status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) ||
       (status_info.control_operation_qualifier != FBE_PAYLOAD_CONTROL_STATUS_QUAILIFIER_OK))
   {
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                        status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        
        if ((status == FBE_STATUS_NO_DEVICE) || (status == FBE_STATUS_COMPONENT_NOT_FOUND))
        {
            return status;
        }
        else
        {
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }

   return status;
}   // end of fbe_api_esp_common_write_resume_prom

/*!***************************************************************
 * @fn cd ..fbe_api_esp_common_initiate_resume_prom_read(fbe_u64_t deviceType,
 *                                        fbe_device_physical_location_t * pLocation)
 ****************************************************************
 * @brief
 *  This function will write the resume prom.
 *
 * @param 
 *   deviceType - 
 *   pLocation -
 * 
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  19-Jul-2011: PHE - Created.
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_esp_common_initiate_resume_prom_read(fbe_u64_t deviceType,
                                          fbe_device_physical_location_t * pLocation)
{
    fbe_object_id_t                                 objectId = FBE_OBJECT_ID_INVALID;
    fbe_status_t                                    status = FBE_STATUS_OK;
    fbe_api_control_operation_status_info_t         statusInfo = {0};
    fbe_esp_base_env_initiate_resume_prom_read_cmd_t   initiateResumeReadCmd = {0};
   
    status = fbe_api_esp_common_get_object_id_by_device_type(deviceType, &objectId);
   
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, 
            "%s: deviceType 0x%llX, failed to get the object id, status 0x%X.\n", 
            __FUNCTION__, (fbe_u64_t)deviceType, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    initiateResumeReadCmd.deviceType = deviceType;
    initiateResumeReadCmd.deviceLocation = *pLocation;

    status = fbe_api_common_send_control_packet (FBE_ESP_BASE_ENV_CONTROL_CODE_INITIATE_RESUME_PROM_READ,
                                                 &initiateResumeReadCmd,
                                                 sizeof(fbe_esp_base_env_initiate_resume_prom_read_cmd_t),
                                                 objectId,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &statusInfo,
                                                 FBE_PACKAGE_ID_ESP);

    if (status != FBE_STATUS_OK || statusInfo.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) 
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, 
            "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", 
            __FUNCTION__, status, 
            statusInfo.packet_qualifier, statusInfo.control_operation_status, statusInfo.control_operation_qualifier);

        if (status == FBE_STATUS_NO_DEVICE)
        {
            return status;
        }
        else
        {
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }

    return status;

}   // end of fbe_api_esp_common_initiate_resume_prom_read

/*!***************************************************************
 * @fn fbe_api_esp_common_get_any_resume_prom_read_in_progress(fbe_class_id_t classId,
 *                                                     fbe_bool_t *pAnyReadInProgress)
 ****************************************************************
 * @brief
 *  This function will return whether there is any resume prom read
 *  in progress for the specified class.
 *
 * @param classId -
 * @param pAnyReadInProgress (OUTPUT) -
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  15-Nov-2011: loul - Created.
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL
fbe_api_esp_common_get_any_resume_prom_read_in_progress(fbe_class_id_t classId,
                                                        fbe_bool_t *pAnyReadInProgress)
{
    fbe_object_id_t                                 objectId;
    fbe_status_t                                    status;
    fbe_api_control_operation_status_info_t         statusInfo;
    fbe_esp_base_env_get_any_resume_prom_read_in_progress_t  getAnyReadInProgress = {0};

    status = fbe_api_esp_getObjectId(classId, &objectId);

	if (status != FBE_STATUS_OK)
	{
		fbe_api_trace (FBE_TRACE_LEVEL_ERROR,
			"%s: Getting objectId by classId failed, classId 0x%X, status 0x%X.\n",
			__FUNCTION__, classId, status);
		return FBE_STATUS_GENERIC_FAILURE;
	}

    status = fbe_api_common_send_control_packet (FBE_ESP_BASE_ENV_CONTROL_CODE_GET_ANY_RESUME_PROM_READ_IN_PROGRESS,
                                                 &getAnyReadInProgress,
                                                 sizeof(fbe_esp_base_env_get_any_resume_prom_read_in_progress_t),
                                                 objectId,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &statusInfo,
                                                 FBE_PACKAGE_ID_ESP);

    if (status != FBE_STATUS_OK || statusInfo.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR,
            "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n",
            __FUNCTION__, status,
            statusInfo.packet_qualifier, statusInfo.control_operation_status, statusInfo.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    *pAnyReadInProgress = getAnyReadInProgress.anyReadInProgress;

    return status;

}   // end of fbe_api_esp_common_get_any_resume_prom_read_in_progress



/*!***************************************************************
 * @fn fbe_api_esp_common_get_is_service_mode(fbe_class_id_t classId,
 *                                            fbe_bool_t *pIsServiceMode)
 ****************************************************************
 * @brief
 *  This function will return whether the SP is currently running in
 *  service mode.  This call is currently only valid for Objects that
 *  rely on SEP for persistent stroage.
 *
 * @param classId -
 * @param pIsServiceMode (OUTPUT) -
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  14-May-2014: bphilbin - Created.
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL
fbe_api_esp_common_get_is_service_mode(fbe_class_id_t classId,
                                       fbe_bool_t *pIsServiceMode)
{
    fbe_object_id_t                                 objectId;
    fbe_status_t                                    status;
    fbe_api_control_operation_status_info_t         statusInfo;
    fbe_esp_base_env_get_service_mode_t             getIsServiceMode = {0};

    status = fbe_api_esp_getObjectId(classId, &objectId);

	if (status != FBE_STATUS_OK)
	{
		fbe_api_trace (FBE_TRACE_LEVEL_ERROR,
			"%s: Getting objectId by classId failed, classId 0x%X, status 0x%X.\n",
			__FUNCTION__, classId, status);
		return FBE_STATUS_GENERIC_FAILURE;
	}

    status = fbe_api_common_send_control_packet (FBE_ESP_BASE_ENV_CONTROL_CODE_GET_SERVICE_MODE,
                                                 &getIsServiceMode,
                                                 sizeof(fbe_esp_base_env_get_service_mode_t),
                                                 objectId,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &statusInfo,
                                                 FBE_PACKAGE_ID_ESP);

    if (status != FBE_STATUS_OK || statusInfo.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR,
            "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n",
            __FUNCTION__, status,
            statusInfo.packet_qualifier, statusInfo.control_operation_status, statusInfo.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    *pIsServiceMode = getIsServiceMode.isServiceMode;

    return status;

}   // end of fbe_api_esp_common_get_is_service_mode


/*!***************************************************************
 * @fn fbe_api_esp_common_getCacheStatus(fbe_esp_common_cache_status_t *cacheStatusPtr)
 ****************************************************************
 * @brief
 *  This function will return the ESP Caching Status.
 *
 * @param 
 *   fbe_esp_common_cache_status_t      *cacheStatusPtr
 * 
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  03/22/11 - Created. Joe Perry
 *  
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_esp_common_getCacheStatus(fbe_common_cache_status_info_t *cacheStatusInfoPtr)
{
    fbe_status_t                                    status;
    fbe_common_cache_status_t                       spsCacheStatus = FBE_CACHE_STATUS_FAILED;
    fbe_common_cache_status_t                       psCacheStatus = FBE_CACHE_STATUS_FAILED;
    fbe_common_cache_status_t                       enclCacheStatus = FBE_CACHE_STATUS_FAILED;
    fbe_common_cache_status_t                       boardCacheStatus = FBE_CACHE_STATUS_FAILED;
    fbe_common_cache_status_t                       coolingCacheStatus = FBE_CACHE_STATUS_FAILED;
//    fbe_u32_t                                       localBatteryTime = 0;
    fbe_status_t                                    spsFbeStatus = FBE_STATUS_OK;
    fbe_status_t                                    psFbeStatus = FBE_STATUS_OK;
    fbe_status_t                                    enclFbeStatus = FBE_STATUS_OK;
    fbe_status_t                                    boardFbeStatus = FBE_STATUS_OK;
    fbe_status_t                                    collingFbeStatus = FBE_STATUS_OK;
    fbe_esp_board_mgmt_ssd_info_t                   ssdInfo={0};
    fbe_esp_board_mgmt_board_info_t                 boardInfo = {0};
    fbe_device_physical_location_t                  location = {0};
//    fbe_status_t                                    localBatteryFbeStatus = FBE_STATUS_OK;

    /*
     * Get SPS status & BatteryTime
     */
    spsFbeStatus = fbe_api_esp_sps_mgmt_getCacheStatus(&spsCacheStatus);

    if (spsFbeStatus != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_WARNING, 
            "%s: Getting spsCacheStatus failed, status 0x%X.\n", 
            __FUNCTION__, spsFbeStatus);
    }


    // Setting Battery Time to zero for now (need to come up with what the calculation should be)
    cacheStatusInfoPtr->batteryTime = 0;
/*
    localBatteryFbeStatus = fbe_api_esp_sps_mgmt_getLocalBatteryTime(&localBatteryTime);

    if (localBatteryFbeStatus != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_WARNING, 
            "%s: Getting localBatteryTime failed, status 0x%X.\n", 
            __FUNCTION__, localBatteryFbeStatus);
    }
    else
    {
        cacheStatusInfoPtr->batteryTime = localBatteryTime;
    }
*/

    /*
     * Get Power Fail status
     */
    psFbeStatus = fbe_api_esp_ps_mgmt_getCacheStatus(&psCacheStatus);

    if (psFbeStatus != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_WARNING, 
            "%s: Getting powerFailCacheStatus failed, status 0x%X.\n", 
            __FUNCTION__, psFbeStatus);
    }

    /*
     * Get Shutdown Reason/Warning status
     */
    enclFbeStatus = fbe_api_esp_encl_mgmt_getCacheStatus(&enclCacheStatus);

    if (enclFbeStatus != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_WARNING, 
            "%s: Getting encl CacheStatus failed, status 0x%X.\n", 
            __FUNCTION__, enclFbeStatus);
    }

    boardFbeStatus = fbe_api_esp_board_mgmt_getCacheStatus(&boardCacheStatus);

    if (boardFbeStatus != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_WARNING, 
            "%s: Getting board CacheStatus failed, status 0x%X.\n", 
            __FUNCTION__, boardFbeStatus);
    }

    /*
     * Get Cooling status
     */
    collingFbeStatus = fbe_api_esp_cooling_mgmt_getCacheStatus(&coolingCacheStatus);

    if (collingFbeStatus != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_WARNING, 
            "%s: Getting coolingCacheStatus failed, status 0x%X.\n", 
            __FUNCTION__, collingFbeStatus);
    }

    /*
     * Get the SSD status (only local SP's)
     */
    status = fbe_api_esp_board_mgmt_getBoardInfo(&boardInfo);
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_WARNING, 
            "%s: Getting BoardInfo failed, status 0x%X.\n", 
            __FUNCTION__, status);
    }
    else
    {
        location.sp = boardInfo.sp_id;
        location.slot= 0;
        status = fbe_api_esp_board_mgmt_getSSDInfo(&location, &ssdInfo);
        if (status != FBE_STATUS_OK)
        {
            fbe_api_trace (FBE_TRACE_LEVEL_WARNING, 
                "%s: Getting local SsdInfo failed, status 0x%X.\n", 
                __FUNCTION__, status);
            cacheStatusInfoPtr->ssdFaulted = FALSE;
        }
        else if ((ssdInfo.ssdState == FBE_SSD_STATE_FAULTED) ||
                 (ssdInfo.ssdState == FBE_SSD_STATE_DEGRADED))
        {
            cacheStatusInfoPtr->ssdFaulted = FBE_TRUE;
        }
        else
        {
            cacheStatusInfoPtr->ssdFaulted = FBE_FALSE;
        }
    }

    /* 
     * Combine cache statuses
     */
    if ((spsCacheStatus == FBE_CACHE_STATUS_FAILED_SHUTDOWN) ||
        (psCacheStatus == FBE_CACHE_STATUS_FAILED_SHUTDOWN) ||
        (enclCacheStatus == FBE_CACHE_STATUS_FAILED_SHUTDOWN) ||
        (boardCacheStatus == FBE_CACHE_STATUS_FAILED_SHUTDOWN) ||
        (coolingCacheStatus == FBE_CACHE_STATUS_FAILED_SHUTDOWN))
    {
        cacheStatusInfoPtr->cacheStatus = FBE_CACHE_STATUS_FAILED_SHUTDOWN;
    }
    else if (boardCacheStatus == FBE_CACHE_STATUS_APPROACHING_OVER_TEMP)
    {
        cacheStatusInfoPtr->cacheStatus = FBE_CACHE_STATUS_APPROACHING_OVER_TEMP;
    }
    else if ((spsCacheStatus == FBE_CACHE_STATUS_FAILED) ||
        (psCacheStatus == FBE_CACHE_STATUS_FAILED) ||
        (enclCacheStatus == FBE_CACHE_STATUS_FAILED) ||
        (boardCacheStatus == FBE_CACHE_STATUS_FAILED) ||
        (coolingCacheStatus == FBE_CACHE_STATUS_FAILED))
    {
        cacheStatusInfoPtr->cacheStatus = FBE_CACHE_STATUS_FAILED;
    }
    else if ((spsCacheStatus == FBE_CACHE_STATUS_DEGRADED) ||
             (psCacheStatus == FBE_CACHE_STATUS_DEGRADED) ||
             (enclCacheStatus == FBE_CACHE_STATUS_DEGRADED) ||
             (boardCacheStatus == FBE_CACHE_STATUS_DEGRADED) ||
             (coolingCacheStatus == FBE_CACHE_STATUS_DEGRADED))
    {
        cacheStatusInfoPtr->cacheStatus = FBE_CACHE_STATUS_DEGRADED;
    }
    else
    {
        cacheStatusInfoPtr->cacheStatus = FBE_CACHE_STATUS_OK;
    }

    /* 
     * Combine Fbe statuses
     */
    if ((spsFbeStatus == FBE_STATUS_BUSY) ||
        (psFbeStatus == FBE_STATUS_BUSY) ||
        (enclFbeStatus == FBE_STATUS_BUSY) ||
        (boardFbeStatus == FBE_STATUS_BUSY) ||
        (collingFbeStatus == FBE_STATUS_BUSY))
    {
        status = FBE_STATUS_BUSY;
    }
    else if ((spsFbeStatus != FBE_STATUS_OK) ||
        (psFbeStatus != FBE_STATUS_OK) ||
        (enclFbeStatus != FBE_STATUS_OK) ||
        (boardFbeStatus != FBE_STATUS_OK) ||
        (collingFbeStatus != FBE_STATUS_OK))
    {
        status = FBE_STATUS_GENERIC_FAILURE;
    }
    else
    {
        status = FBE_STATUS_OK;
    }

    /* Now handle special cases.
       Take a decision which cache status and Fbe status we should return.*/

    if (cacheStatusInfoPtr->cacheStatus == FBE_CACHE_STATUS_FAILED_SHUTDOWN) 
    {   
        /* Return STATUS_OK as caller of the API need not to retry eventhough 
           for some components, we got BUSY status */ 
        status = FBE_STATUS_OK;
    }

    fbe_api_trace (FBE_TRACE_LEVEL_DEBUG_HIGH, 
        "%s: CacheStatus overall %d, sps %d, ps %d, encl %d, board %d, cool %d\n", 
        __FUNCTION__, cacheStatusInfoPtr->cacheStatus, spsCacheStatus, psCacheStatus, enclCacheStatus, boardCacheStatus, coolingCacheStatus);

    return status;

}   // end of fbe_api_esp_common_getCacheStatus


/*!***************************************************************
 * @fn fbe_api_esp_common_powerdown(fbe_esp_common_cache_status_t *cacheStatusPtr)
 ****************************************************************
 * @brief
 *  This function will appropriately power down the array.
 *
 * @param 
 * 
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  04/07/11 - Created. Joe Perry
 *  
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_esp_common_powerdown(void)
{
    fbe_status_t                                    status;
    fbe_board_mgmt_set_PostAndOrReset_t             post_reset;
    fbe_u32_t                                       rebootRetryCount;

    /*
     * Flush data prior to shutdown (if anything fails, continue to powerdown/reboot)
     */
    fbe_api_trace (FBE_TRACE_LEVEL_INFO, 
                   "%s, initiate Flush before Shutdown/Reboot.\n",
                   __FUNCTION__);
    status = fbe_api_esp_board_mgmt_flushSystem();
    if(status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, 
                       "%s, Flush failed, status %d\n",
                       __FUNCTION__, status);
    }
    else
    {
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, 
                       "%s, Flush successful\n",
                       __FUNCTION__);
    }

    /*
     * Check if power fail & SPS needs to be shutdown
     */
    status = fbe_api_esp_sps_mgmt_powerdown();
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, 
            "%s: sps powerdown failed, status 0x%X.\n", 
            __FUNCTION__, status);
    }
    else
    {
        /*
         * Delay for shutdown & if SP still up then reboot
         */
        fbe_api_sleep(1000);
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, 
            "%s: SP still up, so reboot SP.\n", 
            __FUNCTION__);
    }

    /*
     * If we cannot shutdown SPS's, reboot the SP
     */
    for (rebootRetryCount = 0; rebootRetryCount < 5; rebootRetryCount++)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, 
            "%s: attempt to reboot SP (SPS Shutdown failed), count %d\n", 
            __FUNCTION__, rebootRetryCount);
        // send SP reset
        fbe_zero_memory(&post_reset, sizeof(fbe_board_mgmt_set_PostAndOrReset_t));
        post_reset.rebootLocalSp = TRUE;
        post_reset.flushBeforeReboot = FALSE;
        post_reset.rebootBlade = TRUE;
        status = fbe_api_esp_board_mgmt_reboot_sp(&post_reset);
        if (status != FBE_STATUS_OK)
        {
            fbe_api_trace (FBE_TRACE_LEVEL_ERROR, 
                "%s: SP Reboot failed, status 0x%X.\n", 
                __FUNCTION__, status);
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }

    return FBE_STATUS_OK;

}   // end of fbe_api_esp_common_powerdown

/*!***************************************************************
 * @fn fbe_api_esp_common_get_fup_manifest_info(fbe_u64_t deviceType,
 *                                              fbe_base_env_fup_manifest_info_t  * pFupManifestInfo)
 ****************************************************************
 * @brief
 *  This function gets the firmware upgrade manifest info for the specified device type.
 *
 * @param deviceType (Input) -
 * @param pFupManifestInfo (Input/OUTUT) -
 * 
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  18-Sept-2014: PHE - Created.
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_esp_common_get_fup_manifest_info(fbe_u64_t deviceType, 
                                         fbe_base_env_fup_manifest_info_t  * pFupManifestInfo)
{
    fbe_object_id_t                                 objectId;
    fbe_status_t                                    status;
    fbe_api_control_operation_status_info_t         statusInfo;
    fbe_esp_base_env_get_fup_manifest_info_t        getFupManifestInfo = {0};
    
    status = fbe_api_esp_common_get_object_id_by_device_type(deviceType, &objectId);
    
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, 
            "%s: deviceType 0x%llX, failed to get the object id, status 0x%X.\n", 
            __FUNCTION__, deviceType, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    getFupManifestInfo.deviceType = deviceType;
    
    status = fbe_api_common_send_control_packet (FBE_ESP_BASE_ENV_CONTROL_CODE_GET_FUP_MANIFEST_INFO,
                                                 &getFupManifestInfo,
                                                 sizeof(fbe_esp_base_env_get_fup_manifest_info_t),
                                                 objectId,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &statusInfo,
                                                 FBE_PACKAGE_ID_ESP);
    
    if (status != FBE_STATUS_OK || statusInfo.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) 
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, 
            "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", 
            __FUNCTION__, status, 
            statusInfo.packet_qualifier, statusInfo.control_operation_status, statusInfo.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_copy_memory(pFupManifestInfo, 
                    &getFupManifestInfo.manifestInfo[0], 
                    FBE_BASE_ENV_FUP_MAX_SUBENCL_COUNT_IN_MANIFEST * sizeof(fbe_base_env_fup_manifest_info_t));
    
    return status;

}   // end of fbe_api_esp_common_get_fup_manifest_info

/*!***************************************************************
 * @fn fbe_api_esp_common_modify_fup_delay(fbe_u64_t deviceType,
 *                                             fbe_u32_t delayInSec)
 ****************************************************************
 * @brief
 *  This function modifies the firmware upgrade delay time after the SP reboot.
 *
 * @param deviceType (Input) -
 * @param delayInSec (Input) -
 * 
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  18-Sept-2014: PHE - Created.
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_esp_common_modify_fup_delay(fbe_u64_t deviceType, 
                                    fbe_u32_t delayInSec)
{
    fbe_object_id_t                                 objectId;
    fbe_status_t                                    status;
    fbe_api_control_operation_status_info_t         statusInfo;
    fbe_esp_base_env_modify_fup_delay_t             modifyFupDelay = {0};
    
    status = fbe_api_esp_common_get_object_id_by_device_type(deviceType, &objectId);
    
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, 
            "FUP: modifyDelay, deviceType 0x%llX, failed to get the object id, status 0x%X.\n", 
            deviceType, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    modifyFupDelay.deviceType = deviceType; 
    modifyFupDelay.delayInSec = delayInSec;
    
    status = fbe_api_common_send_control_packet (FBE_ESP_BASE_ENV_CONTROL_CODE_MODIFY_FUP_DELAY,
                                                 &modifyFupDelay,
                                                 sizeof(fbe_esp_base_env_modify_fup_delay_t),
                                                 objectId,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 &statusInfo,
                                                 FBE_PACKAGE_ID_ESP);
    
    if (status != FBE_STATUS_OK || statusInfo.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) 
    {
        fbe_api_trace (FBE_TRACE_LEVEL_WARNING, 
            "FUP: modifyDelay, packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", 
            status, 
            statusInfo.packet_qualifier, statusInfo.control_operation_status, statusInfo.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;
}// end of fbe_api_esp_common_modify_fup_delay

/* End of file fbe_api_esp_common_interface.c */

