/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_cooling_mgmt_fup.c
 ***************************************************************************
 * @brief
 *  This file contains the implemention for Fan firmware upgrade functionality.
 *
 * @version
 *   31-March-2011:  PHE - Created.
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_memory.h"
#include "fbe/fbe_api_enclosure_interface.h"
#include "fbe/fbe_api_cooling_interface.h"
#include "fbe_base_object.h"
#include "fbe_cooling_mgmt_private.h"

static fbe_status_t fbe_cooling_mgmt_fup_qualified(fbe_cooling_mgmt_t * pCoolingMgmt,
                                                   fbe_device_physical_location_t * pLocation,
                                                   fbe_cooling_info_t * pFanBasicInfo, 
                                                   fbe_bool_t * pUpgradeQualified);

static fbe_status_t fbe_cooling_mgmt_fup_policy_allowed(fbe_cooling_mgmt_t * pCoolingMgmt, 
                                        fbe_device_physical_location_t * pLocation,
                                        fbe_bool_t * pPolicyAllowed);

static fbe_status_t fbe_cooling_mgmt_fup_generate_logheader(fbe_device_physical_location_t * pLocation, 
                                       fbe_u8_t * pLogHeader, fbe_u32_t logHeaderSize);


/*!**************************************************************
 * @fn fbe_ps_mgmt_fup_handle_ps_status_change(fbe_ps_mgmt_t * pPsMgmt,
 *                                         fbe_device_physical_location_t * pLocation,
 *                                         fbe_power_supply_info_t * pPsInfo,
 *                                         fbe_power_supply_info_t * pPrevPsInfo)
 ****************************************************************
 * @brief
 *  This function initiates the PS upgrade or aborts the PS upgrade
 *  based on the PS state change.
 * 
 * @param pPsMgmt -
 * @param pLocation - The location of the PS.
 * @param pNewPsInfo - The new power supply info.
 * @param pOldPsInfo - The old power supply info.
 * 
 * @return fbe_status_t 
 *
 * @version
 *  31-March-2011:  PHE - Created. 
 *
 ****************************************************************/
fbe_status_t fbe_cooling_mgmt_fup_handle_fan_status_change(fbe_cooling_mgmt_t * pCoolingMgmt,
                                                 fbe_device_physical_location_t * pLocation,
                                                 fbe_cooling_info_t * pNewBasicFanInfo,
                                                 fbe_cooling_info_t * pOldBasicFanInfo)
{
    fbe_bool_t                     newQualified;
    fbe_bool_t                     oldQualified;
    fbe_status_t                   status = FBE_STATUS_OK;
    fbe_char_t                     logHeader[FBE_BASE_ENV_FUP_MAX_LOG_HEADER_SIZE + 1] = {0};
    fbe_bool_t                     policyAllowUpgrade = FBE_FALSE;

    /* Set log header for tracing purpose. */
    fbe_cooling_mgmt_fup_generate_logheader(pLocation, &logHeader[0], FBE_BASE_ENV_FUP_MAX_LOG_HEADER_SIZE);

    if((!pNewBasicFanInfo->inserted) && (pOldBasicFanInfo->inserted))
    {
        /* The power supply was removed. */
        fbe_base_object_trace((fbe_base_object_t *)pCoolingMgmt,
                                      FBE_TRACE_LEVEL_WARNING,
                                      FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                      "%s, Device removed.\n",
                                      &logHeader[0]);

        status = fbe_base_env_fup_handle_device_removal((fbe_base_environment_t *)pCoolingMgmt, 
                                                        FBE_DEVICE_TYPE_FAN, 
                                                        pLocation);
        if(status != FBE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t *)pCoolingMgmt,
                                  FBE_TRACE_LEVEL_WARNING,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s, %s, Failed to handle device removal, status 0x%X.\n",
                                  logHeader,
                                  __FUNCTION__,
                                  status);
    
            return status;
        }
    }
    else 
    {
        fbe_cooling_mgmt_fup_policy_allowed(pCoolingMgmt, pLocation, &policyAllowUpgrade);
        if(!policyAllowUpgrade)
        {
            fbe_base_object_trace((fbe_base_object_t *)pCoolingMgmt,
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s, %s, policy does not allow upgrade.\n",
                                  &logHeader[0],
                                  __FUNCTION__);
    
            return FBE_STATUS_OK;
        }

        fbe_cooling_mgmt_fup_qualified(pCoolingMgmt, pLocation, pOldBasicFanInfo, &oldQualified);
        fbe_cooling_mgmt_fup_qualified(pCoolingMgmt, pLocation, pNewBasicFanInfo, &newQualified);

        if(newQualified && !oldQualified)
        {
            fbe_base_object_trace((fbe_base_object_t *)pCoolingMgmt,
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s, %s, became qualified for upgrade.\n",
                                  logHeader,
                                  __FUNCTION__);

            status = fbe_cooling_mgmt_fup_initiate_upgrade(pCoolingMgmt, 
                                                      FBE_DEVICE_TYPE_FAN, 
                                                      pLocation,
                                                      FBE_BASE_ENV_FUP_FORCE_FLAG_NONE,
                                                      0);  //upgradeRetryCount
            if(status != FBE_STATUS_OK)
            {
                fbe_base_object_trace((fbe_base_object_t *)pCoolingMgmt,
                                      FBE_TRACE_LEVEL_WARNING,
                                      FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                      "%s, %s, Failed to initiate upgrade, status 0x%x.\n",
                                      logHeader,
                                      __FUNCTION__,
                                      status);

            }
        }
    }

    return status;
}

/*!**************************************************************
 * @fn fbe_cooling_mgmt_fup_policy_allowed(fbe_cooling_mgmt_t * pCoolingMgmt, 
 *                                    fbe_device_physical_location_t * location)
 ****************************************************************
 * @brief
 *  This function checks whether the PS is qualified to the upgrade or not.
 *
 * @param pCoolingMgmt - The pointer to fbe_cooling_mgmt_t.
 * @param pLocation - The pointer to the location of the Fan.
 * @param pPolicyAllowed(OUTPUT) -
 * 
 * @return fbe_status_t - always return FBE_STATUS_OK;
 *
 * @version
 *  31-March-2011:  PHE - Created. 
 *
 ****************************************************************/
static fbe_status_t
fbe_cooling_mgmt_fup_policy_allowed(fbe_cooling_mgmt_t * pCoolingMgmt, 
                               fbe_device_physical_location_t * pLocation,
                               fbe_bool_t * pPolicyAllowed)
{

    if(!fbe_base_env_is_active_sp_esp_cmi((fbe_base_environment_t *) pCoolingMgmt))
    {
        fbe_base_object_trace((fbe_base_object_t *)pCoolingMgmt, 
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "FUP:FAN(%d_%d_%d), %s, passive SP, do not start Fan upgrade.\n",
                                  pLocation->bus, pLocation->enclosure, pLocation->slot, __FUNCTION__);
        *pPolicyAllowed = FALSE;
    }
    else
    {
        *pPolicyAllowed = TRUE;
    }
    
    return FBE_STATUS_OK;
}


/*!**************************************************************
 * @fn fbe_cooling_mgmt_fup_qualified(fbe_cooling_mgmt_t * pCoolingMgmt,
 *                                    fbe_cooling_mgmt_fan_info_t * pFanInfo, 
 *                                    fbe_bool_t * pUpgradeQualified)
 ****************************************************************
 * @brief
 *  This function checks whether the PS is qualified to the upgrade or not.
 *
 * @param pPsInfo - The power supply info for the physical package.
 * @param pUpgradeQualified(OUTPUT) - whether the power supply
 *                        is qualified to do the upgrade or not.
 * 
 * @return fbe_status_t - always return FBE_STATUS_OK;
 *
 * @version
 *  31-Mar-2011:  PHE - Created. 
 *
 ****************************************************************/
static fbe_status_t fbe_cooling_mgmt_fup_qualified(fbe_cooling_mgmt_t * pCoolingMgmt,
                                                   fbe_device_physical_location_t * pLocation,
                                                   fbe_cooling_info_t * pFanBasicInfo, 
                                                   fbe_bool_t * pUpgradeQualified)
{
    fbe_char_t                     logHeader[FBE_BASE_ENV_FUP_MAX_LOG_HEADER_SIZE + 1] = {0};

    /* Set log header for tracing purpose. */
    fbe_cooling_mgmt_fup_generate_logheader(pLocation, &logHeader[0], FBE_BASE_ENV_FUP_MAX_LOG_HEADER_SIZE);

    *pUpgradeQualified = ((pFanBasicInfo->bDownloadable) && 
              (pFanBasicInfo->inserted == FBE_MGMT_STATUS_TRUE) &&
              (pFanBasicInfo->fanFaulted == FBE_MGMT_STATUS_FALSE));

    
    if(*pUpgradeQualified != TRUE)
    {
        fbe_base_object_trace((fbe_base_object_t *)pCoolingMgmt,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s, not qualified for upgrade, bDownloadable %d, inserted %d, fanFaulted %d.\n",
                              &logHeader[0],
                              pFanBasicInfo->bDownloadable,
                              pFanBasicInfo->inserted,
                              pFanBasicInfo->fanFaulted);
    }
	return FBE_STATUS_OK;
}


/*!**************************************************************
 * @fn fbe_cooling_mgmt_fup_initiate_upgrade(void * pContext, 
 *                                      fbe_u64_t deviceType, 
 *                                      fbe_device_physical_location_t * pLocation,
 *                                      fbe_u32_t forceFlags,
 *                                      fbe_u32_t upgradeRetryCount)
 ****************************************************************
 * @brief
 *  This function creates and queues the work item when the upgrade needs to be issued.
 *
 * @param pContext - The pointer to the object.
 * @param deviceType -
 * @param pLocation - The pointer to the physical location that the work item needs to be added for.
 * @param forceFlags-
 * @param upgradeRetryCount - 
 * 
 * @return fbe_status_t
 *
 * @version
 *  15-June-2010:  PHE - Created. 
 *
 ****************************************************************/
fbe_status_t fbe_cooling_mgmt_fup_initiate_upgrade(void * pContext, 
                                              fbe_u64_t deviceType,
                                              fbe_device_physical_location_t * pLocation,
                                              fbe_u32_t forceFlags,
                                              fbe_u32_t upgradeRetryCount)
{
    fbe_cooling_mgmt_t             * pCoolingMgmt = (fbe_cooling_mgmt_t *)pContext;
    fbe_cooling_mgmt_fan_info_t    * pFanInfo = NULL;
    fbe_bool_t                       policyAllowUpgrade = FALSE;
    fbe_bool_t                       upgradeQualified = FALSE;
    fbe_char_t                       logHeader[FBE_BASE_ENV_FUP_MAX_LOG_HEADER_SIZE + 1] = {0};
    fbe_status_t                     status = FBE_STATUS_OK;

    if(deviceType != FBE_DEVICE_TYPE_FAN)
    {
        fbe_base_object_trace((fbe_base_object_t *)pCoolingMgmt,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "FUP: %s, unsupported deviceType 0x%llX.\n",
                              __FUNCTION__,
                              deviceType);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_cooling_mgmt_get_fan_info_by_location(pCoolingMgmt, pLocation, &pFanInfo);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)pCoolingMgmt,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "FUP:FAN(%d_%d_%d), %s, get_fan_info_by_location failed.\n",
                              pLocation->bus,
                              pLocation->enclosure,
                              pLocation->slot,
                              __FUNCTION__);

        return status;
    }

    fbe_cooling_mgmt_fup_policy_allowed(pCoolingMgmt, pLocation, &policyAllowUpgrade);
    if(!policyAllowUpgrade)
    {
        return FBE_STATUS_OK;
    }

    fbe_cooling_mgmt_fup_qualified(pCoolingMgmt, pLocation, &pFanInfo->basicInfo, &upgradeQualified);
    if(!upgradeQualified)
    {
        fbe_base_object_trace((fbe_base_object_t *)pCoolingMgmt,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "FUP:FAN(%d_%d_%d), %s, not qualified for upgrade.\n",
                              pLocation->bus,
                              pLocation->enclosure,
                              pLocation->slot,
                              __FUNCTION__);

        return FBE_STATUS_OK;
    }

    /* Set log header for tracing and logging purpose. */
    fbe_cooling_mgmt_fup_generate_logheader(pLocation, &logHeader[0], FBE_BASE_ENV_FUP_MAX_LOG_HEADER_SIZE);


    status = fbe_base_env_fup_create_and_add_work_item((fbe_base_environment_t *)pCoolingMgmt,
                                               pLocation,
                                               FBE_DEVICE_TYPE_FAN,
                                               FBE_FW_TARGET_COOLING,
                                               &pFanInfo->basicInfo.firmwareRev[0],
                                               &pFanInfo->basicInfo.subenclProductId[0],
                                               pFanInfo->basicInfo.uniqueId,
                                               forceFlags,
                                               &logHeader[0],
                                               0,   // There is no interDeviceDelay for FAN FUP.
                                               upgradeRetryCount,
                                               ESES_REVISION_1_0); 
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)pCoolingMgmt,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "FUP:FAN(%d_%d_%d), %s, Failed to create_and_add_work_item, status 0x%X.\n",
                              pLocation->bus,
                              pLocation->enclosure,
                              pLocation->slot,
                              __FUNCTION__,
                              status);
    }

    return status;
}
    
    

/*!**************************************************************
 * @fn fbe_cooling_mgmt_fup_generate_logheader(fbe_device_physical_location_t * pLocation, 
 *                               fbe_u8_t * logHeader)
 ****************************************************************
 * @brief
 *  This function sets the log header for the FAN.
 *
 * @param pLocation - The pointer to the FAN location.
 * @param pLogHeader(output) - The pointer to the log header.
 * @param logHeaderSize -
 *
 * @return fbe_status_t
 *
 * @version
 *  15-June-2010:  PHE - Created. 
 *
 ****************************************************************/
static fbe_status_t fbe_cooling_mgmt_fup_generate_logheader(fbe_device_physical_location_t * pLocation, 
                                   fbe_u8_t * pLogHeader, fbe_u32_t logHeaderSize)
{
    fbe_zero_memory(pLogHeader, logHeaderSize);

    csx_p_snprintf(pLogHeader, logHeaderSize, "FUP:FAN(%d_%d_%d)",
                       pLocation->bus, pLocation->enclosure, pLocation->slot);

    return FBE_STATUS_OK;
}

/*!*******************************************************************************
 * @fn fbe_cooling_mgmt_fup_get_full_image_path
 *********************************************************************************
 * @brief
 *  This function gets the full power supply image path. It includes the file name. 
 *
 *  All the power supply images will have a filename of the format 
 *  SXP36x6g_Power_Supply_xxxxxxxx.bin where xxxxxxxx is the subenclosure product id. 
 *  For example:
 *  a Power Supply with a subenclosure product id of 000B0015, will 
 *  be listed as 00B0015 in the config page (subenclosure product id), 
 *  and it will take firmware file SXP36x6g_Power_Supply_000B0015.bin
 *
 * @param  pContext -
 * @param  pWorkItem -
 *
 * @return fbe_status_t - 
 *
 * @version
 *  15-June-2010 PHE -- Created.
 *******************************************************************************/
fbe_status_t fbe_cooling_mgmt_fup_get_full_image_path(void * pContext,                               
                               fbe_base_env_fup_work_item_t * pWorkItem)
{
    fbe_base_environment_t * pBaseEnv =(fbe_base_environment_t *)pContext;
    char imageFileName[FBE_BASE_ENV_FUP_MAX_IMAGE_FILE_NAME_LEN] = {0};
    fbe_u32_t imageFileNameLen = 0;
    fbe_char_t imageFileConstantPortionName[FBE_BASE_ENV_FUP_MAX_IMAGE_FILE_NAME_LEN] = {0};
    fbe_char_t imagePathKey[256] = {0};
    fbe_u32_t imagePathIndex = 0;
    fbe_status_t   status = FBE_STATUS_OK;
    
    if(pWorkItem->uniqueId == VOYAGER_FRONT_FAN_COTROL)
    {
        imagePathIndex = 0;

        fbe_copy_memory(&imageFileConstantPortionName[0], 
                        FBE_COOLING_MGMT_CDES11_FUP_IMAGE_FILE_NAME,
                        sizeof(FBE_COOLING_MGMT_CDES11_FUP_IMAGE_FILE_NAME));

        fbe_copy_memory(&imagePathKey[0], 
                        FBE_COOLING_MGMT_CDES11_FUP_IMAGE_PATH_KEY,
                        sizeof(FBE_COOLING_MGMT_CDES11_FUP_IMAGE_PATH_KEY));
    }
    else
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_base_env_fup_build_image_file_name(&imageFileName[0], 
                                                   FBE_COOLING_MGMT_CDES11_FUP_IMAGE_FILE_NAME,
                                                   FBE_BASE_ENV_FUP_MAX_IMAGE_FILE_NAME_LEN,                                                    
                                                   &pWorkItem->subenclProductId[0],
                                                   &imageFileNameLen,
                                                   ESES_REVISION_1_0);

    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)pBaseEnv,
                                        FBE_TRACE_LEVEL_ERROR,
                                        fbe_base_env_get_fup_work_item_logheader(pWorkItem),
                                        "%s, build_image_file_name failed.\n",
                                        __FUNCTION__);
        return status;
    }

    status = fbe_base_env_fup_get_full_image_path(pBaseEnv,
                                              pWorkItem,
                                              imagePathIndex,
                                              FBE_COOLING_MGMT_CDES11_FUP_IMAGE_PATH_KEY,  
                                              &imageFileName[0],
                                              imageFileNameLen);

    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)pBaseEnv,
                                        FBE_TRACE_LEVEL_INFO,
                                        fbe_base_env_get_fup_work_item_logheader(pWorkItem),
                                        "%s, get_full_image_path failed.\n",
                                        __FUNCTION__);
        return status;
    }
    
    return FBE_STATUS_OK;

} //End of function fbe_ps_mgmt_fup_get_full_image_path
 

/*!**************************************************************
 * @fn fbe_cooling_mgmt_fup_check_env_status(void * pContext, 
 *                                      fbe_base_env_fup_work_item_t * pWorkItem)
 ****************************************************************
 * @brief
 *  This function checks the environmental status for the specified work item
 *  to see whether it allows the upgrade. It alreays return OK.
 * 
 * @param pContext - The pointer to the call back context.
 * @param pWorkItem - The pointer to the work item.
 *
 * @return fbe_status_t - Always return FBE_STATUS_OK.
 *
 * @version
 *  31-Mar-2011:  PHE - Created. 
 *
 ****************************************************************/
fbe_status_t fbe_cooling_mgmt_fup_check_env_status(void * pContext,
                                              fbe_base_env_fup_work_item_t * pWorkItem)
{
    return FBE_STATUS_OK; 
} // End of function fbe_cooling_mgmt_fup_check_env_status


/*!**************************************************************
 * @fn fbe_cooling_mgmt_fup_get_firmware_rev(void * pContext,
 *                         fbe_device_physical_location_t * pLocation,
 *                        fbe_u8_t * pFirmwareRev)
 ****************************************************************
 * @brief
 *  This function gets PS firmware rev.
 *
 * @param context - The pointer to the call back context.
 * @param pLocation - The pointer to the physical location of the power supply.
 * @param pFirmwareRev(OUTPUT) - The pointer to the new firmware rev. 
 *
 * @return fbe_status_t
 *
 * @version
 *  02-July-2010:  PHE - Created. 
 *
 ****************************************************************/
fbe_status_t fbe_cooling_mgmt_fup_get_firmware_rev(void * pContext,
                                              fbe_base_env_fup_work_item_t * pWorkItem,
                                              fbe_u8_t * pFirmwareRev)
{
    fbe_cooling_mgmt_t             * pCoolingMgmt = (fbe_cooling_mgmt_t *)pContext;
    fbe_device_physical_location_t * pLocation = NULL;
    fbe_cooling_mgmt_fan_info_t    * pFanInfo = NULL;
    fbe_cooling_info_t               basicFanInfo = {0};
    fbe_status_t                     status = FBE_STATUS_OK;
    
   
    pLocation = fbe_base_env_get_fup_work_item_location_ptr(pWorkItem);

    status = fbe_cooling_mgmt_get_fan_info_by_location(pCoolingMgmt, pLocation, &pFanInfo);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*)pCoolingMgmt, 
                                    FBE_TRACE_LEVEL_INFO,
                                    FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                    "%s, FAN(%d_%d_%d)Failed to get_fan_info_by_location, status 0x%x.\n",
                                    __FUNCTION__,
                                    pLocation->bus,
                                    pLocation->enclosure,
                                    pLocation->slot,
                                    status);

        return status;
    } 

    basicFanInfo.slotNumOnEncl = pLocation->slot;

    status = fbe_api_cooling_get_fan_info(pFanInfo->objectId, &basicFanInfo);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*)pCoolingMgmt, 
                                    FBE_TRACE_LEVEL_INFO,
                                    FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                    "%s, FAN(%d_%d_%d) fbe_api_cooling_get_fan_info failed, status 0x%x.\n",
                                    __FUNCTION__,
                                    pLocation->bus,
                                    pLocation->enclosure,
                                    pLocation->slot,
                                    status);

        return status;
    }

    fbe_copy_memory(pFirmwareRev,
                    &basicFanInfo.firmwareRev[0], 
                    FBE_ENCLOSURE_MAX_FIRMWARE_REV_SIZE);

    return FBE_STATUS_OK;
}


/*!**************************************************************
 * @fn fbe_cooling_mgmt_get_fup_info_ptr(void * pContext,
 *                                           fbe_u64_t deviceType,
 *                                           fbe_device_physical_location_t * pLocation,
 *                                           fbe_base_env_fup_persistent_info_t ** pFanFupInfoPtr)
 ****************************************************************
 * @brief
 *  This function gets the pionter which points to the fan firmware upgrade info.
 *
 * @param pContext - The pointer to the callback context.
 * @param deviceType -
 * @param pLocation - 
 * @param pFupInfoPtr - 
 *
 * @return fbe_status_t
 *
 * @version
 *  20-Sept-2011:  PHE - Created. 
 *
 ****************************************************************/
fbe_status_t fbe_cooling_mgmt_get_fup_info_ptr(void * pContext,
                                                   fbe_u64_t deviceType,
                                                   fbe_device_physical_location_t * pLocation,
                                                   fbe_base_env_fup_persistent_info_t ** pFupInfoPtr)
{
    fbe_cooling_mgmt_t                     * pCoolingMgmt = (fbe_cooling_mgmt_t *)pContext;
    fbe_cooling_mgmt_fan_info_t            * pFanInfo = NULL;
    fbe_status_t                             status = FBE_STATUS_OK;

    *pFupInfoPtr = NULL;

    if(deviceType != FBE_DEVICE_TYPE_FAN)
    {
        fbe_base_object_trace((fbe_base_object_t*)pCoolingMgmt, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s, unsupported deviceType 0x%llX.\n",
                              __FUNCTION__,
                              deviceType);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* get the fan info. */
    status = fbe_cooling_mgmt_get_fan_info_by_location(pCoolingMgmt,
                                                      pLocation, 
                                                      &pFanInfo);
    
    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*)pCoolingMgmt, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s, FAN(%d_%d_%d) get_fan_info_by_location failed, status 0x%x.\n",
                              __FUNCTION__,
                              pLocation->bus,
                              pLocation->enclosure,
                              pLocation->slot, status);

        return status;
    }

    *pFupInfoPtr = &pFanInfo->fanFupInfo;

    return FBE_STATUS_OK;
} // End of function fbe_cooling_mgmt_get_fup_info_ptr

/*!**************************************************************
 * @fn fbe_cooling_mgmt_get_fup_info(void * pContext,
 *                                           fbe_u64_t deviceType,
 *                                           fbe_device_physical_location_t * pLocation,
 *                                           fbe_esp_base_env_get_fup_info_t *  pGetFupInfo)
 ****************************************************************
 * @brief
 *  This function gets the fan firmware upgrade info for a specific fan slot.
 *
 * @param pContext - The pointer to the callback context.
 * @param deviceType -
 * @param pLocation - 
 * @param pGetFupInfo - 
 *
 * @return fbe_status_t
 *
 * @version
 *  23-Aug-2012:  Rui Chang - Created. 
 *
 ****************************************************************/
fbe_status_t fbe_cooling_mgmt_get_fup_info(void * pContext,
                                                   fbe_u64_t deviceType,
                                                   fbe_device_physical_location_t * pLocation,
                                                   fbe_esp_base_env_get_fup_info_t *  pGetFupInfo)
{
    fbe_cooling_mgmt_t                     * pCoolingMgmt = (fbe_cooling_mgmt_t *)pContext;
    fbe_cooling_mgmt_fan_info_t            * pFanInfo = NULL;
    fbe_status_t                             status = FBE_STATUS_OK;
    fbe_base_env_fup_work_item_t           * pWorkItem = NULL;


    if(deviceType != FBE_DEVICE_TYPE_FAN)
    {
        fbe_base_object_trace((fbe_base_object_t*)pCoolingMgmt, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s, unsupported deviceType 0x%llX.\n",
                              __FUNCTION__,
                              deviceType);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* get the fan info. */
    status = fbe_cooling_mgmt_get_fan_info_by_location(pCoolingMgmt,
                                                      pLocation, 
                                                      &pFanInfo);
    
    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*)pCoolingMgmt, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s, FAN(%d_%d_%d) get_fan_info_by_location failed, status 0x%x.\n",
                              __FUNCTION__,
                              pLocation->bus,
                              pLocation->enclosure,
                              pLocation->slot,
                              status);

        return status;
    }

    pGetFupInfo->programmableCount = FBE_COOLING_MGMT_MAX_PROGRAMMABLE_COUNT_PER_COOLING_SLOT;

    pGetFupInfo->fupInfo[0].workState = FBE_BASE_ENV_FUP_WORK_STATE_NONE;
    status = fbe_base_env_fup_find_work_item((fbe_base_environment_t *)pContext, deviceType, pLocation, &pWorkItem);
    if(pWorkItem != NULL)
    {
        pGetFupInfo->fupInfo[0].workState = pWorkItem->workState;

        if (pGetFupInfo->fupInfo[0].workState == FBE_BASE_ENV_FUP_WORK_STATE_WAIT_BEFORE_UPGRADE)  
        {
            pGetFupInfo->fupInfo[0].waitTimeBeforeUpgrade = fbe_base_env_get_wait_time_before_upgrade((fbe_base_environment_t *)pContext);
        }
    }

    pGetFupInfo->fupInfo[0].componentId = 0;
    pGetFupInfo->fupInfo[0].completionStatus = pFanInfo->fanFupInfo.completionStatus;
    fbe_copy_memory(&pGetFupInfo->fupInfo[0].imageRev[0],
                        &pFanInfo->fanFupInfo.imageRev[0],
                        FBE_ENCLOSURE_MAX_FIRMWARE_REV_SIZE + 1);
    fbe_copy_memory(&pGetFupInfo->fupInfo[0].currentFirmwareRev[0],
                        &pFanInfo->fanFupInfo.currentFirmwareRev[0],
                        FBE_ENCLOSURE_MAX_FIRMWARE_REV_SIZE + 1);
    fbe_copy_memory(&pGetFupInfo->fupInfo[0].preFirmwareRev[0],
                        &pFanInfo->fanFupInfo.preFirmwareRev[0],
                        FBE_ENCLOSURE_MAX_FIRMWARE_REV_SIZE + 1);

    return FBE_STATUS_OK;
} // End of function fbe_cooling_mgmt_get_fup_info

/*!***************************************************************
 * fbe_cooling_mgmt_fup_resume_upgrade()
 ****************************************************************
 * @brief
 *  This function resumes the ugrade for the devices which were aborted.
 *
 * @param pContext -
 * 
 * @return fbe_status_t
 *
 * @author
 *  01-Mar-2011: PHE - Created. 
 *
 ****************************************************************/
fbe_status_t fbe_cooling_mgmt_fup_resume_upgrade(void * pContext)
{
    fbe_cooling_mgmt_t                     * pCoolingMgmt = (fbe_cooling_mgmt_t *)pContext;
    fbe_cooling_mgmt_fan_info_t            * pFanInfo = NULL;
    fbe_u32_t                                fanIndex = 0;
    fbe_device_physical_location_t           location = {0};
    fbe_status_t                             status = FBE_STATUS_OK;

    fbe_base_object_trace((fbe_base_object_t *)pCoolingMgmt,
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "FUP: Resume upgrade for FAN.\n");

    for(fanIndex = 0; fanIndex < FBE_COOLING_MGMT_MAX_FAN_COUNT; fanIndex++)
    {
         pFanInfo = &pCoolingMgmt->pFanInfo[fanIndex];

        if(pFanInfo->objectId != FBE_OBJECT_ID_INVALID)
        {
            if(pFanInfo->fanFupInfo.completionStatus == 
                     FBE_BASE_ENV_FUP_COMPLETION_STATUS_ABORTED)
            { 
                location.bus = pFanInfo->location.bus;
                location.enclosure = pFanInfo->location.enclosure;
                location.slot = pFanInfo->location.slot;
                status = fbe_cooling_mgmt_fup_initiate_upgrade(pCoolingMgmt, 
                                                  FBE_DEVICE_TYPE_FAN, 
                                                  &location,
                                                  pFanInfo->fanFupInfo.forceFlags,
                                                  0);  // upgradeRetryCount;
                if(status != FBE_STATUS_OK)
                {
                    fbe_base_object_trace((fbe_base_object_t *)pCoolingMgmt,
                                          FBE_TRACE_LEVEL_WARNING,
                                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                          "FUP:FAN(%d_%d_%d), %s, Failed to initiate upgrade, status 0x%X.\n",
                                          location.bus,
                                          location.enclosure,
                                          location.slot,
                                          __FUNCTION__,
                                          status);
            
                }
            }
        }
    }
    
    return FBE_STATUS_OK;
} /* End of function fbe_cooling_mgmt_fup_resume_upgrade */

/*!***************************************************************
 * fbe_cooling_mgmt_fup_initiate_upgrade_for_all()
 ****************************************************************
 * @brief
 *  This function initiate the ugrade for the devices which does not have the completion status
 *  from local SP point of view. It gets called when the peer SP went away.
 *
 * @param pCoolingMgmt -
 * 
 * @return fbe_status_t
 *
 * @author
 *  27-Apr-2011: PHE - Created. 
 *
 ****************************************************************/
fbe_status_t fbe_cooling_mgmt_fup_initiate_upgrade_for_all(fbe_cooling_mgmt_t * pCoolingMgmt)
{
    fbe_cooling_mgmt_fan_info_t            * pFanInfo = NULL;
    fbe_u32_t                                fanIndex = 0;
    fbe_device_physical_location_t           location = {0};
    fbe_status_t                             status = FBE_STATUS_OK;

    fbe_base_object_trace((fbe_base_object_t *)pCoolingMgmt,
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "FUP: %s.\n", __FUNCTION__);

    for(fanIndex = 0; fanIndex < FBE_COOLING_MGMT_MAX_FAN_COUNT; fanIndex++)
    {
         pFanInfo = &pCoolingMgmt->pFanInfo[fanIndex];

        if(pFanInfo->objectId != FBE_OBJECT_ID_INVALID)
        {
            if(pFanInfo->fanFupInfo.completionStatus == 
                     FBE_BASE_ENV_FUP_COMPLETION_STATUS_NONE)
            { 
                location.bus = pFanInfo->location.bus;
                location.enclosure = pFanInfo->location.enclosure;
                location.slot = pFanInfo->location.slot;
                status = fbe_cooling_mgmt_fup_initiate_upgrade(pCoolingMgmt, 
                                                  FBE_DEVICE_TYPE_FAN, 
                                                  &location,
                                                  pFanInfo->fanFupInfo.forceFlags,
                                                  0); // upgradeRetryCount
                if(status != FBE_STATUS_OK)
                {
                    fbe_base_object_trace((fbe_base_object_t *)pCoolingMgmt,
                                          FBE_TRACE_LEVEL_WARNING,
                                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                          "FUP:FAN(%d_%d_%d), %s, Failed to initiate upgrade, status 0x%X.\n",
                                          location.bus,
                                          location.enclosure,
                                          location.slot,
                                          __FUNCTION__,
                                          status);
                }
            }
        }
    }
    
    return FBE_STATUS_OK;
} /* End of function fbe_cooling_mgmt_fup_resume_upgrade */

/*!**************************************************************
 * @fn fbe_cooling_mgmt_fup_copy_fw_rev_to_resume_prom(fbe_cooling_mgmt_t *pCoolingMgmt,
 *                                                fbe_u64_t deviceType,
 *                                                fbe_device_physical_location_t *pLocation)
 ****************************************************************
 * @brief
 *  This function copies the firmware revision to the resume prom after
 *  the firmware upgrade completes.
 * 
 * @param pPsMgmt - The pointer to ps_mgmt.
 * @param deviceType - The device type.
 * @param pLocation - The location of the PS.
 * 
 * @return fbe_status_t 
 *
 * @version
 *  18-Jan-2012:  PHE- Created. 
 *
 ****************************************************************/
fbe_status_t fbe_cooling_mgmt_fup_copy_fw_rev_to_resume_prom(fbe_cooling_mgmt_t *pCoolingMgmt,
                                                        fbe_u64_t deviceType,
                                                        fbe_device_physical_location_t *pLocation)
{
    fbe_char_t                             logHeader[FBE_BASE_ENV_FUP_MAX_LOG_HEADER_SIZE + 1] = {0};
    fbe_status_t                           status = FBE_STATUS_OK;
    fbe_base_env_resume_prom_info_t      * pResumePromInfo = NULL;
    fbe_cooling_mgmt_fan_info_t          * pFanInfo = NULL;
    fbe_u8_t                               fwRevForResume[RESUME_PROM_PROG_REV_SIZE] = {0};

    fbe_cooling_mgmt_fup_generate_logheader(pLocation, &logHeader[0], FBE_BASE_ENV_FUP_MAX_LOG_HEADER_SIZE);

    status = fbe_cooling_mgmt_get_resume_prom_info_ptr(pCoolingMgmt, 
                                                  deviceType, 
                                                  pLocation, 
                                                  &pResumePromInfo);

    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)pCoolingMgmt,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s, %s, get_resume_prom_info_ptr failed.\n",
                              &logHeader[0], __FUNCTION__);

        return status;
    }

    status = fbe_cooling_mgmt_get_fan_info_by_location(pCoolingMgmt, pLocation, &pFanInfo);

    if (status != FBE_STATUS_OK) 
    {
        fbe_base_object_trace((fbe_base_object_t *)pCoolingMgmt,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s, %s, get_fan_info_by_location failed.\n",
                              &logHeader[0], __FUNCTION__);

        return status;
    }

    pResumePromInfo->resume_prom_data.num_prog[0] = 1;

    fbe_base_env_fup_convert_fw_rev_for_rp((fbe_base_environment_t *)pCoolingMgmt,
                                                   &fwRevForResume[0],
                                                   &pFanInfo->basicInfo.firmwareRev[0]);

    fbe_copy_memory(pResumePromInfo->resume_prom_data.prog_details[0].prog_rev,
                    &fwRevForResume[0],
                    RESUME_PROM_PROG_REV_SIZE);
    
    return status;
}


/*!**************************************************************
 * @fn fbe_cooling_mgmt_fup_refresh_device_status(void * pContext,
 *                         fbe_u64_t deviceType,
 *                         fbe_device_physical_location_t * pLocation)
 ****************************************************************
 * @brief
 *  This function refresh the device status. We might filter the status change
 *  during the upgrade so we would like to refresh the device status when
 *  the upgrade is done.
 *
 * @param context - The pointer to the call back context.
 * @param deviceType - 
 * @param pLocation - The pointer to the physical location of the SPS.
 *
 * @return fbe_status_t
 *
 * @version
 *  02-May-2013: PHE - Created.
 *
 ****************************************************************/
fbe_status_t fbe_cooling_mgmt_fup_refresh_device_status(void * pContext,
                                             fbe_base_env_fup_work_item_t * pWorkItem)
{
    fbe_cooling_mgmt_t                 * pCoolingMgmt = (fbe_cooling_mgmt_t *)pContext;
    fbe_cooling_mgmt_fan_info_t        * pFanInfo = NULL;
    fbe_status_t                         status = FBE_STATUS_OK;
    fbe_device_physical_location_t     * pLocation = NULL;

    if(pWorkItem->deviceType != FBE_DEVICE_TYPE_FAN)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)pCoolingMgmt,
                        FBE_TRACE_LEVEL_ERROR,
                        fbe_base_env_get_fup_work_item_logheader(pWorkItem),
                        "%s unsupported deviceType 0x%llX.\n",   
                        __FUNCTION__, pWorkItem->deviceType);

        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    if(pWorkItem->workState >= FBE_BASE_ENV_FUP_WORK_STATE_ACTIVATE_IMAGE_SENT)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)pCoolingMgmt,
                        FBE_TRACE_LEVEL_INFO,
                        fbe_base_env_get_fup_work_item_logheader(pWorkItem),
                        "Refresh device status...\n");
    }
    else
    {
        return FBE_STATUS_OK;
    }

    pLocation = &pWorkItem->location;
    status = fbe_cooling_mgmt_get_fan_info_by_location(pCoolingMgmt, pLocation, &pFanInfo);

    if (status != FBE_STATUS_OK) 
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)pCoolingMgmt,
                        FBE_TRACE_LEVEL_INFO,
                        fbe_base_env_get_fup_work_item_logheader(pWorkItem),
                        "Refresh device status, get_fan_info_by_location failed, status 0x%X.\n",
                        status);

        /* 
         * The enclosure which contains the FAN might already be removed.
         * So we just return OK here. 
         */ 
        return FBE_STATUS_OK;
    }

// JAP - last argument zero (no FUP on Hyperion Fans)
    status = fbe_cooling_mgmt_process_fan_status(pCoolingMgmt, pFanInfo->objectId, pLocation, 0);

    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)pCoolingMgmt,
                        FBE_TRACE_LEVEL_INFO,
                        fbe_base_env_get_fup_work_item_logheader(pWorkItem),
                        "Refresh device status, process_fan_status failed, status 0x%X.\n",
                        status);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    return FBE_STATUS_OK;
}

/*!***************************************************************
 * @fn fbe_cooling_mgmt_fup_new_contact_init_upgrade(fbe_cooling_mgmt_t *pCoolingMgmt)
 ****************************************************************
 * @brief
 * When we receive peer permission request it also implies peer
 * contact was made. Go back and check if we already killed an
 * update because the peer was not yet present.
 * 
 * Look up the completion status for the local device that is
 * relative to the peer requesting device (pPeerLocation). If we
 * stopped fup because the peer was not present, send the command
 * to kick off a new upgrade attempt.
 *
 * @param pCoolingMgmt -pointer to fbe_cooling_mgmt_t
 *      
 * @return fbe_status_t
 *
 * @author
 *  211-Dec-2014: PHE - Created. 
 *
 ****************************************************************/
fbe_status_t fbe_cooling_mgmt_fup_new_contact_init_upgrade(fbe_cooling_mgmt_t *pCoolingMgmt)
{
    fbe_cooling_mgmt_fan_info_t            * pFanInfo = NULL;
    fbe_u32_t                                fanIndex = 0;
    fbe_device_physical_location_t           location = {0};
    fbe_status_t                             status = FBE_STATUS_OK;

    // Peer contact back, update PS FW. 
    fbe_base_object_trace((fbe_base_object_t *)pCoolingMgmt,
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s peer SP contact is back.\n",
                                  __FUNCTION__);

    for(fanIndex = 0; fanIndex < FBE_COOLING_MGMT_MAX_FAN_COUNT; fanIndex++)
    {
         pFanInfo = &pCoolingMgmt->pFanInfo[fanIndex];

        if(pFanInfo->objectId != FBE_OBJECT_ID_INVALID)
        {
            if(pFanInfo->fanFupInfo.completionStatus == 
                     FBE_BASE_ENV_FUP_COMPLETION_STATUS_FAIL_NO_PEER_PERMISSION)
            { 
                location.bus = pFanInfo->location.bus;
                location.enclosure = pFanInfo->location.enclosure;
                location.slot = pFanInfo->location.slot;
                status = fbe_cooling_mgmt_fup_initiate_upgrade(pCoolingMgmt, 
                                                  FBE_DEVICE_TYPE_FAN, 
                                                  &location,
                                                  pFanInfo->fanFupInfo.forceFlags,
                                                  0);  // upgradeRetryCount;
                if(status != FBE_STATUS_OK)
                {
                    fbe_base_object_trace((fbe_base_object_t *)pCoolingMgmt,
                                          FBE_TRACE_LEVEL_WARNING,
                                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                          "FUP:FAN(%d_%d_%d), %s, Failed to initiate upgrade, status 0x%X.\n",
                                          location.bus,
                                          location.enclosure,
                                          location.slot,
                                          __FUNCTION__,
                                          status);
            
                }
            }
        }
    }
    
    return FBE_STATUS_OK;
} //fbe_cooling_mgmt_fup_new_contact_init_upgrade

//End of file fbe_cooling_mgmt_fup.c
