/***************************************************************************
 * Copyright (C) EMC Corporation 2012
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_ps_mgmt_fup.c
 ***************************************************************************
 * @brief
 *  This file contains the SPS and Battery specific firmware upgrade 
 *  functionality. These functions are based on similar functions for 
 *  power supply, cooling module,and lcc processing.
 *
 * @version
 *  08-Aug-2012: GB - Created. 
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_memory.h"
#include "fbe/fbe_api_enclosure_interface.h"
#include "fbe/fbe_api_sps_interface.h"
#include "fbe_base_object.h"
#include "fbe_sps_mgmt_private.h"
#include "generic_utils_lib.h"

//#include "generic_utils_lib.h"

static fbe_status_t fbe_sps_mgmt_fup_generate_logheader(fbe_device_physical_location_t * pLocation, 
                                                        fbe_u8_t * pLogHeader, 
                                                        fbe_u32_t logHeaderSize);
static fbe_status_t fbe_sps_mgmt_fup_policy_allowed(fbe_sps_mgmt_t * pSpsMgmt, 
                                                    fbe_device_physical_location_t * pLocation,
                                                    fbe_bool_t * pAllowed);
static fbe_status_t fbe_sps_mgmt_fup_qualified(fbe_sps_mgmt_t * pSpsMgmt,
                                               fbe_device_physical_location_t * pLocation,
                                               fbe_sps_info_t * pSpsInfo, 
                                               fbe_bool_t * pUpgradeQualified);

/*!**************************************************************
 * @fn fbe_sps_mgmt_fup_handle_sps_status_change(fbe_sps_mgmt_t *pSpsMgmt,
                                                 fbe_device_physical_location_t *pLocation,
                                                 fbe_sps_info_t *pSpsNewInfo,
                                                 fbe_sps_info_t *pSpsOldInfo)
 ****************************************************************
 * @brief
 *  This function initiates the SPS upgrade or aborts the SPS upgrade
 *  based on the SPS state change.
 * 
 * @param pSpsMgmt -
 * @param pLocation - The location of the SPS.
 * @param pNewSpsInfo - The new SPS info.
 * @param pOldSpsInfo - The old SPS info.
 * 
 * @return fbe_status_t 
 *
 * @version
 *  08-Aug-2012: GB - Created. 
 ****************************************************************/
fbe_status_t fbe_sps_mgmt_fup_handle_sps_status_change(fbe_sps_mgmt_t *pSpsMgmt,
                                                 fbe_device_physical_location_t *pLocation,
                                                 fbe_sps_info_t *pSpsNewInfo,
                                                 fbe_sps_info_t *pSpsOldInfo)
{
    fbe_bool_t                     newQualified;
    fbe_bool_t                     oldQualified;
    fbe_status_t                   status = FBE_STATUS_OK;
    fbe_char_t                     logHeader[FBE_BASE_ENV_FUP_MAX_LOG_HEADER_SIZE + 1] = {0};
    fbe_bool_t                     allowUpgrade = FBE_FALSE;
    fbe_sps_cid_enum_t             cid = FBE_SPS_COMPONENT_ID_MAX;

    if (pSpsMgmt->arraySpsInfo->simulateSpsInfo)
    {
        /* We are simulating SPS. 
         * No need to do the firmware upgrade related operations. 
         */
        return FBE_STATUS_OK;
    }

    // We should have done this in the physical package. But fbe_sps_mgmt_processSpsStatus expects
    // the slot as 0. So I can not change it now. - PINGHE
    pLocation->slot = (pSpsMgmt->base_environment.spid == SP_A)? 0 : 1;

    // handle sps removal
    if((!pSpsNewInfo->spsModulePresent) && 
       (pSpsOldInfo->spsModulePresent) && 
       (pSpsOldInfo->bSpsModuleDownloadable))
    {
        
        for(cid = FBE_SPS_COMPONENT_ID_PRIMARY; cid < pSpsOldInfo->programmableCount; cid ++) 
        {
            pLocation->componentId = cid;
            status = fbe_base_env_fup_handle_device_removal((fbe_base_environment_t *)pSpsMgmt, 
                                                            FBE_DEVICE_TYPE_SPS,
                                                            pLocation); 

            if(status != FBE_STATUS_OK)
            {
                /* Set log header for tracing purpose. */
                fbe_sps_mgmt_fup_generate_logheader(pLocation, &logHeader[0], FBE_BASE_ENV_FUP_MAX_LOG_HEADER_SIZE);
                fbe_base_object_trace((fbe_base_object_t *)pSpsMgmt,
                                      FBE_TRACE_LEVEL_WARNING,
                                      FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                      "%s, %s, Failed to handle device removal, status 0x%X.\n",
                                      &logHeader[0],
                                      __FUNCTION__,
                                      status);
        
                return status;
            }
        }

        return FBE_STATUS_OK;
    }

    // handle battery removal
    if((!pSpsNewInfo->spsBatteryPresent) && 
       (pSpsOldInfo->spsBatteryPresent) &&
       (pSpsOldInfo->bSpsBatteryDownloadable))
    {
        pLocation->componentId = FBE_SPS_COMPONENT_ID_BATTERY;
        status = fbe_base_env_fup_handle_device_removal((fbe_base_environment_t *)pSpsMgmt, 
                                                        FBE_DEVICE_TYPE_SPS,
                                                        pLocation);
        if(status != FBE_STATUS_OK)
        {
            /* Set log header for tracing purpose. */
            fbe_sps_mgmt_fup_generate_logheader(pLocation, &logHeader[0], FBE_BASE_ENV_FUP_MAX_LOG_HEADER_SIZE);
            fbe_base_object_trace((fbe_base_object_t *)pSpsMgmt,
                                  FBE_TRACE_LEVEL_WARNING,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s, %s, Failed to handle device removal, status 0x%X.\n",
                                  &logHeader[0],
                                  __FUNCTION__,
                                  status);
    
        }

        return status;
    }

    for(cid = FBE_SPS_COMPONENT_ID_PRIMARY; cid < pSpsNewInfo->programmableCount; cid ++) 
    {
        pLocation->componentId = cid;
    
        // make the policy checks
        fbe_sps_mgmt_fup_policy_allowed(pSpsMgmt, pLocation, &allowUpgrade);
        if(!allowUpgrade)
        {
            continue;
        }

        // check all ffids, etc in the ups: sps and battery
        fbe_sps_mgmt_fup_qualified(pSpsMgmt, pLocation, pSpsNewInfo, &newQualified);
        fbe_sps_mgmt_fup_qualified(pSpsMgmt, pLocation, pSpsOldInfo, &oldQualified);
        if(newQualified && !oldQualified)
        {
            /* Set log header for tracing purpose. */
            fbe_sps_mgmt_fup_generate_logheader(pLocation, &logHeader[0], FBE_BASE_ENV_FUP_MAX_LOG_HEADER_SIZE);
            fbe_base_object_trace((fbe_base_object_t *)pSpsMgmt,
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s, %s, became qualified for upgrade.\n",
                                  &logHeader[0],
                                  __FUNCTION__);

            status = fbe_sps_mgmt_fup_initiate_upgrade(pSpsMgmt, 
                                               FBE_DEVICE_TYPE_SPS,
                                               pLocation,
                                               FBE_BASE_ENV_FUP_FORCE_FLAG_NONE,
                                               0);  //upgradeRetryCount


            if(status != FBE_STATUS_OK)
            {
                 fbe_base_object_trace((fbe_base_object_t *)pSpsMgmt,
                                      FBE_TRACE_LEVEL_WARNING,
                                      FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                      "%s, %s, Failed to initiate upgrade, status 0x%X.\n",
                                      &logHeader[0],
                                      __FUNCTION__,
                                      status);
            }
        }
    }
    return(status);
} // fbe_sps_mgmt_fup_handle_sps_status_change

/*!**************************************************************
 * @fn fbe_sps_mgmt_fup_generate_logheader(fbe_device_physical_location_t * pLocation, 
 *                               fbe_u8_t * logHeader)
 ****************************************************************
 * @brief
 *  This function sets the log header for the sps.
 *
 * @param pLocation - The pointer to the sps location.
 * @param pLogHeader(output) - The pointer to the log header.
 * @param logHeaderSize -
 *
 * @return fbe_status_t
 *
 * @version
 *  08-Aug-2012: GB - Created. 
 ****************************************************************/
static fbe_status_t fbe_sps_mgmt_fup_generate_logheader(fbe_device_physical_location_t * pLocation, 
                                                        fbe_u8_t * pLogHeader, 
                                                        fbe_u32_t logHeaderSize)
{
    fbe_zero_memory(pLogHeader, logHeaderSize);

    csx_p_snprintf(pLogHeader, logHeaderSize, "FUP:SPS(%d_%d_%d.%d)",
                   pLocation->bus, pLocation->enclosure, pLocation->slot, pLocation->componentId);

    return FBE_STATUS_OK;
} // fbe_sps_mgmt_fup_generate_logheader


/*!**************************************************************
 * @fn fbe_sps_mgmt_fup_policy_allowed(fbe_ps_mgmt_t * pPsMgmt, 
 *                                    fbe_device_physical_location_t * location)
 ****************************************************************
 * @brief
 *  Check the "policy" if this location (local) is ok to upgrade
 *
 * @param pSpsMgmt - The sps info.
 * @param pLocation -
 * @param pPolicyAllowed(OUTPUT) -
 * 
 * @return fbe_status_t - always return FBE_STATUS_OK;
 *
 * @version
 *  08-Aug-2012: GB - Created. 
 ****************************************************************/
static fbe_status_t fbe_sps_mgmt_fup_policy_allowed(fbe_sps_mgmt_t * pSpsMgmt, 
                                                    fbe_device_physical_location_t * pLocation,
                                                    fbe_bool_t * pAllowed)
{
    fbe_base_environment_t * pBaseEnv = (fbe_base_environment_t *)pSpsMgmt; 

    *pAllowed = FALSE;

    if(pBaseEnv->spid == pLocation->slot)
    {
        *pAllowed = TRUE;
    }

    return FBE_STATUS_OK;
} //fbe_sps_mgmt_fup_policy_allowed

/*!**************************************************************
 * @fn fbe_sps_mgmt_fup_qualified(fbe_sps_mgmt_t * pSpsMgmt,
 *                               fbe_sps_info_t * pSpsInfo, 
 *                               fbe_bool_t * pUpgradeQualified)
 ****************************************************************
 * @brief
 *  This function checks whether the PS is qualified to the upgrade or not.
 *
 * @param pSpsInfo - The power supply info for the physical package.
 * @param pUpgradeQualified(OUTPUT) - whether the power supply
 *                        is qualified to do the upgrade or not.
 * 
 * @return fbe_status_t - always return FBE_STATUS_OK;
 *
 * @version
 *  08-Aug-2012: GB - Created.
 *  06-Oct-2012: PHE - modified to check componentId.
 ****************************************************************/
static fbe_status_t fbe_sps_mgmt_fup_qualified(fbe_sps_mgmt_t * pSpsMgmt,
                                              fbe_device_physical_location_t * pLocation,
                                              fbe_sps_info_t * pSpsInfo, 
                                              fbe_bool_t * pUpgradeQualified)
{
    fbe_char_t                         logHeader[FBE_BASE_ENV_FUP_MAX_LOG_HEADER_SIZE + 1] = {0};
    fbe_base_env_fup_format_fw_rev_t   firmwareRevAfterFormatting = {0};
    
    /* Set log header for tracing purpose. */
    fbe_sps_mgmt_fup_generate_logheader(pLocation, &logHeader[0], FBE_BASE_ENV_FUP_MAX_LOG_HEADER_SIZE);
    
    *pUpgradeQualified = FBE_FALSE;

    if(pLocation->componentId == FBE_SPS_COMPONENT_ID_PRIMARY)
    {
        fbe_base_env_fup_format_fw_rev(&firmwareRevAfterFormatting, 
                                   &pSpsInfo->primaryFirmwareRev[0], 
                                   FBE_ENCLOSURE_MAX_FIRMWARE_REV_SIZE);

        if((firmwareRevAfterFormatting.majorNumber == 0) &&
           (firmwareRevAfterFormatting.minorNumber == 0)) 
        {
            fbe_base_object_trace((fbe_base_object_t *)pSpsMgmt,
                                  FBE_TRACE_LEVEL_WARNING,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s,not qualified, fwRev %s, fwRevAfterFormatting major %d, minor %d.\n",
                                  &logHeader[0],
                                  &pSpsInfo->primaryFirmwareRev[0],
                                  firmwareRevAfterFormatting.majorNumber,
                                  firmwareRevAfterFormatting.minorNumber);

            return FBE_STATUS_OK;
        }

        *pUpgradeQualified = ((pSpsInfo->spsModulePresent) &&
                              (pSpsInfo->bSpsModuleDownloadable) &&
                              (pSpsInfo->spsModuleFfid == LI_ION_SPS_2_2_KW_WITHOUT_BATTERY));
        
        if( !(*pUpgradeQualified))
        {
            fbe_base_object_trace((fbe_base_object_t *)pSpsMgmt,
                                  FBE_TRACE_LEVEL_WARNING,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s,not qualified, modIns %d modDnldable %d modFfid %s spsStatus %s.\n",
                                  &logHeader[0],
                                  pSpsInfo->spsModulePresent,
                                  pSpsInfo->bSpsModuleDownloadable,
                                  decodeFamilyFruID(pSpsInfo->spsModuleFfid),
                                  decodeSpsState(pSpsInfo->spsStatus));
        
        }

    }
    else if(pLocation->componentId == FBE_SPS_COMPONENT_ID_SECONDARY)
    {
        fbe_base_env_fup_format_fw_rev(&firmwareRevAfterFormatting, 
                                   &pSpsInfo->secondaryFirmwareRev[0], 
                                   FBE_ENCLOSURE_MAX_FIRMWARE_REV_SIZE);

        if((firmwareRevAfterFormatting.majorNumber == 0) &&
           (firmwareRevAfterFormatting.minorNumber == 0)) 
        {
            fbe_base_object_trace((fbe_base_object_t *)pSpsMgmt,
                                  FBE_TRACE_LEVEL_WARNING,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s,not qualified, fwRev %s, fwRevAfterFormatting major %d, minor %d.\n",
                                  &logHeader[0],
                                  &pSpsInfo->secondaryFirmwareRev[0],
                                  firmwareRevAfterFormatting.majorNumber,
                                  firmwareRevAfterFormatting.minorNumber);

            return FBE_STATUS_OK;
        }

        *pUpgradeQualified = ((pSpsInfo->spsModulePresent) &&
                              (pSpsInfo->bSpsModuleDownloadable) &&
                              (pSpsInfo->spsModuleFfid == LI_ION_SPS_2_2_KW_WITHOUT_BATTERY));
        
        if( !(*pUpgradeQualified))
        {
            fbe_base_object_trace((fbe_base_object_t *)pSpsMgmt,
                                  FBE_TRACE_LEVEL_WARNING,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s,not qualified, modIns %d modDnldable %d modFfid %s spsStatus %s.\n",
                                  &logHeader[0],
                                  pSpsInfo->spsModulePresent,
                                  pSpsInfo->bSpsModuleDownloadable,
                                  decodeFamilyFruID(pSpsInfo->spsModuleFfid),
                                  decodeSpsState(pSpsInfo->spsStatus));
        
        }
    }
    else if(pLocation->componentId == FBE_SPS_COMPONENT_ID_BATTERY) 
    {
        fbe_base_env_fup_format_fw_rev(&firmwareRevAfterFormatting, 
                                   &pSpsInfo->batteryFirmwareRev[0], 
                                   FBE_ENCLOSURE_MAX_FIRMWARE_REV_SIZE);

        if((firmwareRevAfterFormatting.majorNumber == 0) &&
           (firmwareRevAfterFormatting.minorNumber == 0)) 
        {
            fbe_base_object_trace((fbe_base_object_t *)pSpsMgmt,
                                  FBE_TRACE_LEVEL_WARNING,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s,not qualified, fwRev %s, fwRevAfterFormatting major %d, minor %d.\n",
                                  &logHeader[0],
                                  &pSpsInfo->batteryFirmwareRev[0],
                                  firmwareRevAfterFormatting.majorNumber,
                                  firmwareRevAfterFormatting.minorNumber);

            return FBE_STATUS_OK;
        }

        *pUpgradeQualified = ((pSpsInfo->spsModulePresent) &&
                              (pSpsInfo->spsBatteryPresent) &&
                              (pSpsInfo->bSpsBatteryDownloadable) &&
                              ((pSpsInfo->spsBatteryFfid == LI_ION_2_42_KW_BATTERY_PACK_082) || 
                               (pSpsInfo->spsBatteryFfid == LI_ION_2_42_KW_BATTERY_PACK_115)));

        if( !(*pUpgradeQualified))
        {
            fbe_base_object_trace((fbe_base_object_t *)pSpsMgmt,
                                  FBE_TRACE_LEVEL_WARNING,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s,not qualified, modIns %d batIns %d batDnldable %d batFfid %s spsStatus %s.\n",
                                  &logHeader[0],
                                  pSpsInfo->spsModulePresent,
                                  pSpsInfo->spsBatteryPresent,
                                  pSpsInfo->bSpsBatteryDownloadable,
                                  decodeFamilyFruID(pSpsInfo->spsBatteryFfid),
                                  decodeSpsState(pSpsInfo->spsStatus));

        }
    }
    else
    {
        fbe_base_object_trace((fbe_base_object_t *)pSpsMgmt,
                                  FBE_TRACE_LEVEL_WARNING,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s,not qualified, invalid compId %d.\n",
                                  &logHeader[0],
                                  pLocation->componentId);
    }
    
    return FBE_STATUS_OK;
} //fbe_sps_mgmt_fup_qualified


/*!**************************************************************
 * @fn fbe_sps_mgmt_fup_initiate_upgrade(void * pContext, 
 *                                      fbe_u64_t deviceType, 
 *                                      fbe_device_physical_location_t * pLocation,
 *                                      fbe_u32_t forceFlags,
 *                                      fbe_u32_t upgradeRetryCount)
 ****************************************************************
 * @brief
 *  This function creates and queues the work item when the
 *  upgrade needs to be issued.
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
 *  08-Aug-2012: GB - Created. 
 *  05-Oct-2012: PHE - Updated.
 ****************************************************************/
fbe_status_t fbe_sps_mgmt_fup_initiate_upgrade(void * pContext, 
                                               fbe_u64_t deviceType,
                                               fbe_device_physical_location_t * pLocation,
                                               fbe_u32_t forceFlags,
                                               fbe_u32_t upgradeRetryCount)
{
    fbe_sps_mgmt_t              *pSpsMgmt = (fbe_sps_mgmt_t *)pContext;
    fbe_sps_info_t              *pSpsInfo = NULL;
    fbe_bool_t                  policyAllowUpgrade = FALSE;
    fbe_bool_t                  upgradeQualified = FALSE;
    fbe_char_t                  logHeader[FBE_BASE_ENV_FUP_MAX_LOG_HEADER_SIZE + 1] = {0};
    fbe_status_t                status = FBE_STATUS_INVALID;
    fbe_enclosure_fw_target_t   firmwareTarget = FBE_FW_TARGET_INVALID;
    HW_MODULE_TYPE              uniqueId;
    fbe_char_t                  imageFileNameUidString[FBE_ENCLOSURE_MAX_SUBENCL_PRODUCT_ID_SIZE + 1] = {0};
    fbe_u8_t                    firmwareRev[FBE_ENCLOSURE_MAX_FIRMWARE_REV_SIZE + 1] = {0};

    /* Set log header for tracing and logging purpose. */
    fbe_sps_mgmt_fup_generate_logheader(pLocation, &logHeader[0], FBE_BASE_ENV_FUP_MAX_LOG_HEADER_SIZE);

    if(deviceType != FBE_DEVICE_TYPE_SPS)
    {
        fbe_base_object_trace((fbe_base_object_t *)pSpsMgmt,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s, %s, unsupported deviceType 0x%llX.\n",
                              &logHeader[0],
                              __FUNCTION__,
                              deviceType);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_sps_mgmt_fup_policy_allowed(pSpsMgmt, pLocation, &policyAllowUpgrade);
    if(!policyAllowUpgrade)
    {
        fbe_base_object_trace((fbe_base_object_t *)pSpsMgmt,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s, spid:%d,%s, policy does not allow upgrade.\n",
                              &logHeader[0],
                              ((fbe_base_environment_t *)pSpsMgmt)->spid,
                              __FUNCTION__);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_sps_mgmt_get_sps_info_ptr(pSpsMgmt, FBE_DEVICE_TYPE_SPS, pLocation, &pSpsInfo);

    if(status != FBE_STATUS_OK) 
    {
        fbe_base_object_trace((fbe_base_object_t*)pSpsMgmt, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s, %s, failed to get sps info ptr.\n",
                              &logHeader[0],
                              __FUNCTION__);

        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    fbe_sps_mgmt_fup_qualified(pSpsMgmt, pLocation, pSpsInfo, &upgradeQualified);
    if(!upgradeQualified)
    {
        fbe_base_object_trace((fbe_base_object_t *)pSpsMgmt,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s, %s, not qualified for upgrade.\n",
                              &logHeader[0],
                              __FUNCTION__);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    // Get uniqueId based on the component id and set up the firmware target
    if (pLocation->componentId == FBE_SPS_COMPONENT_ID_PRIMARY)
    {
        uniqueId = pSpsInfo->spsModuleFfid;
        firmwareTarget = FBE_FW_TARGET_SPS_PRIMARY;
        fbe_copy_memory(&firmwareRev[0], &pSpsInfo->primaryFirmwareRev[0], FBE_ENCLOSURE_MAX_FIRMWARE_REV_SIZE);
    }
    else if (pLocation->componentId == FBE_SPS_COMPONENT_ID_SECONDARY)
    {
        uniqueId = pSpsInfo->spsModuleFfid;
        firmwareTarget = FBE_FW_TARGET_SPS_SECONDARY;
        fbe_copy_memory(&firmwareRev[0], &pSpsInfo->secondaryFirmwareRev[0], FBE_ENCLOSURE_MAX_FIRMWARE_REV_SIZE);
    }
    else if(pLocation->componentId == FBE_SPS_COMPONENT_ID_BATTERY)
    {
        uniqueId = pSpsInfo->spsBatteryFfid;
        firmwareTarget = FBE_FW_TARGET_SPS_BATTERY;
        fbe_copy_memory(&firmwareRev[0], &pSpsInfo->batteryFirmwareRev[0], FBE_ENCLOSURE_MAX_FIRMWARE_REV_SIZE);
    }
    else
    {
        fbe_base_object_trace((fbe_base_object_t *)pSpsMgmt,
                                  FBE_TRACE_LEVEL_WARNING,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s, %s, invalid compId.\n",
                                  &logHeader[0],
                                  __FUNCTION__);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    switch(uniqueId) 
    {
        case LI_ION_SPS_2_2_KW_WITHOUT_BATTERY:
            fbe_copy_memory(&imageFileNameUidString[0], "000E0008", 8);
            break;
        case LI_ION_2_42_KW_BATTERY_PACK_082:
            fbe_copy_memory(&imageFileNameUidString[0], "000E0009", 8);
            break;
        case LI_ION_2_42_KW_BATTERY_PACK_115:
            fbe_copy_memory(&imageFileNameUidString[0], "000E000D", 8);
            break;
        default:
            fbe_base_object_trace((fbe_base_object_t *)pSpsMgmt,
                                      FBE_TRACE_LEVEL_WARNING,
                                      FBE_TRACE_MESSAGE_ID_INFO,
                                      "%s, %s, invalid ffid.\n",
                                      &logHeader[0],
                                      __FUNCTION__);
    
            return FBE_STATUS_GENERIC_FAILURE;
            break;
    }

    status = fbe_base_env_fup_create_and_add_work_item((fbe_base_environment_t *)pSpsMgmt,
                                                       pLocation,
                                                       deviceType,
                                                       firmwareTarget,
                                                       &firmwareRev[0],
                                                       &imageFileNameUidString[0],
                                                       uniqueId,
                                                       forceFlags,
                                                       &logHeader[0],
                                                       FBE_SPS_MGMT_FUP_INTER_DEVICE_DELAY,
                                                       upgradeRetryCount,
                                                       ESES_REVISION_1_0);


    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)pSpsMgmt,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s, %s, Failed to create_and_add_work_item, status 0x%X.\n",
                              &logHeader[0],
                              __FUNCTION__,
                              status);
    }

    return status;

} //fbe_sps_mgmt_fup_initiate_upgrade


/*!*******************************************************************************
 * @fn fbe_sps_mgmt_fup_get_full_image_path
 *********************************************************************************
 * @brief
 *  This function gets the full SPS image path. It includes the file name. 
 *
 *  All the power supply images will have a filename of the format 
 *  SXP36x6g_Power_Supply_xxxxxxxx.bin where xxxxxxxx is the subenclosure product id. 
 *  For example:
 *  a Power Supply with a subenclosure product id of 000B0015, will 
 *  be listed as �000B0015� in the config page (subenclosure product id), 
 *  and it will take firmware file SXP36x6g_Power_Supply_000B0015.bin
 *
 * @param  pContext -
 * @param  pWorkItem -
 *
 * @return fbe_status_t - 
 *
 * @version
 *  08-Aug-2012: GB - Created. 
 *******************************************************************************/
fbe_status_t fbe_sps_mgmt_fup_get_full_image_path(void * pContext,                               
                               fbe_base_env_fup_work_item_t * pWorkItem)
{
    fbe_base_environment_t  *pBaseEnv =(fbe_base_environment_t *)pContext;
    fbe_char_t              imageFileName[FBE_BASE_ENV_FUP_MAX_IMAGE_FILE_NAME_LEN] = {0};
    fbe_u32_t               imageFileNameLen = 0;
    fbe_char_t              imageFileConstantPortionName[FBE_BASE_ENV_FUP_MAX_IMAGE_FILE_NAME_LEN] = {0};
    fbe_char_t              imagePathKey[256] = {0};
    fbe_u32_t               imagePathIndex = 0;
    fbe_status_t            status = FBE_STATUS_GENERIC_FAILURE;
    fbe_device_physical_location_t * pLocation = NULL;
    
    
    imagePathIndex = 0;

    // get the location out of the work item
    pLocation = fbe_base_env_get_fup_work_item_location_ptr(pWorkItem);

    switch(pLocation->componentId) 
    {
        case FBE_SPS_COMPONENT_ID_PRIMARY:
            if(LI_ION_SPS_2_2_KW_WITHOUT_BATTERY == pWorkItem->uniqueId) 
            {
                // sps primary
                fbe_copy_memory(&imageFileConstantPortionName[0], 
                                FBE_SPS_MGMT_PRI_CDES_FUP_IMAGE_FILE_NAME,
                                sizeof(FBE_SPS_MGMT_PRI_CDES_FUP_IMAGE_FILE_NAME));
        
                fbe_copy_memory(&imagePathKey[0], 
                                FBE_SPS_MGMT_CDES_FUP_IMAGE_PATH_KEY,
                                sizeof(FBE_SPS_MGMT_CDES_FUP_IMAGE_PATH_KEY));
                status = FBE_STATUS_OK;
            }
            imagePathIndex = 0;
            break;
    
        case FBE_SPS_COMPONENT_ID_SECONDARY:
            if(LI_ION_SPS_2_2_KW_WITHOUT_BATTERY == pWorkItem->uniqueId) 
            {
                // sps secondary
                fbe_copy_memory(&imageFileConstantPortionName[0], 
                                FBE_SPS_MGMT_SEC_CDES_FUP_IMAGE_FILE_NAME,
                                sizeof(FBE_SPS_MGMT_SEC_CDES_FUP_IMAGE_FILE_NAME));
        
                fbe_copy_memory(&imagePathKey[0], 
                                FBE_SPS_MGMT_CDES_FUP_IMAGE_PATH_KEY,
                                sizeof(FBE_SPS_MGMT_CDES_FUP_IMAGE_PATH_KEY));
                status = FBE_STATUS_OK;
            }
            imagePathIndex = 1;
            break;
    
        case FBE_SPS_COMPONENT_ID_BATTERY:
            if((LI_ION_2_42_KW_BATTERY_PACK_082 == pWorkItem->uniqueId) ||
               (LI_ION_2_42_KW_BATTERY_PACK_115 == pWorkItem->uniqueId))
            {
                // sps battery
                fbe_copy_memory(&imageFileConstantPortionName[0], 
                            FBE_SPS_MGMT_BAT_CDES_FUP_IMAGE_FILE_NAME,
                            sizeof(FBE_SPS_MGMT_BAT_CDES_FUP_IMAGE_FILE_NAME));
    
                fbe_copy_memory(&imagePathKey[0], 
                                FBE_SPS_MGMT_CDES_FUP_IMAGE_PATH_KEY,
                                sizeof(FBE_SPS_MGMT_CDES_FUP_IMAGE_PATH_KEY));
                status = FBE_STATUS_OK;
            }
            imagePathIndex = 2;
    
        default:
            break;
    }
    
    if(status != FBE_STATUS_OK) 
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)pBaseEnv,
                                        FBE_TRACE_LEVEL_ERROR,
                                        fbe_base_env_get_fup_work_item_logheader(pWorkItem),
                                        "%s, unsupported cid %d or uniqueId %d.\n",
                                        __FUNCTION__,
                                        pLocation->componentId,
                                        pWorkItem->uniqueId);
        return status;
    }

    status = fbe_base_env_fup_build_image_file_name(&imageFileName[0], 
                                                   &imageFileConstantPortionName[0], 
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
                                              &imagePathKey[0],  
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

} //fbe_sps_mgmt_fup_get_full_image_path

/*!**************************************************************
 * @fn fbe_sps_mgmt_fup_check_env_status(void * pContext, 
 *                                      fbe_base_env_fup_work_item_t * pWorkItem)
 ****************************************************************
 * @brief
 *  This function checks the environmental status for the specified work item
 *  to see whether it allows the upgrade. If the peer SPS is present and available,
 *  the env status is good to allow the upgrade. 
 * 
 * @param pContext - The pointer to the call back context.
 * @param pWorkItem - The pointer to the work item.
 *
 * @return fbe_status_t
 *     FBE_STATUS_OK - env status is good for upgrade.
 *     otherwise - env status is no good for upgrade.
 *
 * @version
 *  12-Oct-2012: PHE - Created. 
 *
 ****************************************************************/
fbe_status_t fbe_sps_mgmt_fup_check_env_status(void * pContext,
                                 fbe_base_env_fup_work_item_t * pWorkItem)
{
    fbe_sps_mgmt_t                        * pSpsMgmt = (fbe_sps_mgmt_t *)pContext;
    fbe_device_physical_location_t        * pLocation = NULL;
    fbe_device_physical_location_t          peerSpsLocation = {0};
    fbe_sps_info_t                        * pPeerSpsInfo = NULL;
    fbe_char_t                              logHeader[FBE_BASE_ENV_FUP_MAX_LOG_HEADER_SIZE + 1] = {0};
    fbe_status_t                            status = FBE_STATUS_OK;

    pLocation = fbe_base_env_get_fup_work_item_location_ptr(pWorkItem);

    /* Set log header for tracing purpose. */
    fbe_sps_mgmt_fup_generate_logheader(pLocation, &logHeader[0], FBE_BASE_ENV_FUP_MAX_LOG_HEADER_SIZE);

    peerSpsLocation.bus = pLocation->bus;
    peerSpsLocation.enclosure = pLocation->enclosure;
    peerSpsLocation.slot = (pLocation->slot == 0)? 1 : 0;

    status = fbe_sps_mgmt_get_sps_info_ptr(pSpsMgmt, FBE_DEVICE_TYPE_SPS, &peerSpsLocation, &pPeerSpsInfo);

    if(status != FBE_STATUS_OK) 
    {
        fbe_base_object_trace((fbe_base_object_t*)pSpsMgmt, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s, %s, failed to get its peer sps info ptr.\n",
                              &logHeader[0],
                              __FUNCTION__);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_base_object_trace((fbe_base_object_t*)pSpsMgmt, 
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s, %s, peerSps, spsModulePresent %d,spsBatteryPresent %d,spsStatus %d.\n",
                          &logHeader[0],
                          __FUNCTION__,
                          pPeerSpsInfo->spsModulePresent,
                          pPeerSpsInfo->spsBatteryPresent,
                          pPeerSpsInfo->spsStatus);

    if((pPeerSpsInfo->spsModulePresent) && 
       (pPeerSpsInfo->spsBatteryPresent) &&
       (pPeerSpsInfo->spsStatus == SPS_STATE_AVAILABLE))
    {
        // add another check for testing because the spsstatus change may not be been picked up from the poll
        // if sps sent peer permission to test there is no stopping it, so check here
        if((pSpsMgmt->testingState != FBE_SPS_LOCAL_SPS_TESTING) && 
           (pSpsMgmt->testingState != FBE_SPS_WAIT_FOR_PEER_PERMISSION))
        {
            return FBE_STATUS_OK;   
        }
    }

    return FBE_STATUS_GENERIC_FAILURE; 
} //fbe_sps_mgmt_fup_check_env_status

/*!**************************************************************
 * @fn fbe_sps_mgmt_fup_get_firmware_rev(void * pContext,
 *                         fbe_device_physical_location_t * pLocation,
 *                        fbe_u8_t * pFirmwareRev)
 ****************************************************************
 * @brief
 *  This function gets SPS firmware rev from the sps info.
 *
 * @param context - The pointer to the call back context.
 * @param pLocation - The pointer to the physical location of the power supply.
 * @param pFirmwareRev(OUTPUT) - The pointer to the new firmware rev. 
 *
 * @return fbe_status_t
 *
 * @version
 *  08-Aug-2012: GB - Created.
 *  05-Oct-2012: PHE - Updated by using fbe_sps_mgmt_convertSpsLocation.
 *
 ****************************************************************/
fbe_status_t fbe_sps_mgmt_fup_get_firmware_rev(void * pContext,
                                              fbe_base_env_fup_work_item_t * pWorkItem,
                                              fbe_u8_t * pFirmwareRev)
{
    fbe_sps_mgmt_t                        * pSpsMgmt = (fbe_sps_mgmt_t *)pContext;
    fbe_device_physical_location_t        * pLocation = NULL;
    fbe_sps_get_sps_status_t                getSpsInfo = {0};
    fbe_u32_t                               spsEnclIndex = 0;
    fbe_u32_t                               spsIndex = 0;
    fbe_object_id_t                         objectId = FBE_OBJECT_ID_INVALID;
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_char_t                              logHeader[FBE_BASE_ENV_FUP_MAX_LOG_HEADER_SIZE + 1] = {0};

    
    // get the location out of the work item
    pLocation = fbe_base_env_get_fup_work_item_location_ptr(pWorkItem);

    /* Set log header for tracing purpose. */
    fbe_sps_mgmt_fup_generate_logheader(pLocation, &logHeader[0], FBE_BASE_ENV_FUP_MAX_LOG_HEADER_SIZE);

    status = fbe_sps_mgmt_convertSpsLocation(pSpsMgmt, pLocation, &spsEnclIndex, &spsIndex);

    if(status != FBE_STATUS_OK) 
    {
        fbe_base_object_trace((fbe_base_object_t*)pSpsMgmt, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s, %s, failed to convertFromLocation.\n",
                              &logHeader[0],
                              __FUNCTION__);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    objectId = pSpsMgmt->arraySpsInfo->spsObjectId[spsEnclIndex];

    // get the sps info 
    getSpsInfo.spsIndex = spsIndex;
    status = fbe_api_sps_get_sps_info(objectId, &getSpsInfo);

    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*)pSpsMgmt, 
                                    FBE_TRACE_LEVEL_INFO,
                                    FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                    "%s, %s, failed to get sps info.\n",
                                    &logHeader[0],
                                    __FUNCTION__);

        return status;
    }

   
    switch(pLocation->componentId) 
    {
        case FBE_SPS_COMPONENT_ID_PRIMARY:
            fbe_copy_memory(pFirmwareRev,
                            &getSpsInfo.spsInfo.primaryFirmwareRev[0],
                            FBE_ENCLOSURE_MAX_FIRMWARE_REV_SIZE);
            break;
    
        case FBE_SPS_COMPONENT_ID_SECONDARY:
            fbe_copy_memory(pFirmwareRev,
                            &getSpsInfo.spsInfo.secondaryFirmwareRev[0],
                            FBE_ENCLOSURE_MAX_FIRMWARE_REV_SIZE);
            break;
    
        case FBE_SPS_COMPONENT_ID_BATTERY:
            
            fbe_copy_memory(pFirmwareRev,
                            &getSpsInfo.spsInfo.batteryFirmwareRev[0],
                            FBE_ENCLOSURE_MAX_FIRMWARE_REV_SIZE);
            break;
    
        default:
            fbe_base_object_trace((fbe_base_object_t*)pSpsMgmt, 
                                        FBE_TRACE_LEVEL_INFO,
                                        FBE_TRACE_MESSAGE_ID_INFO,
                                        "%s, %s, invalid cid.\n",
                                        &logHeader[0],
                                        __FUNCTION__);

            return FBE_STATUS_GENERIC_FAILURE;
            break;
    }
    
    return FBE_STATUS_OK;
    
} // fbe_sps_mgmt_fup_get_firmware_rev

/*!**************************************************************
 * @fn fbe_sps_mgmt_get_sps_fup_info_ptr(void * pContext,
 *                         fbe_u64_t deviceType,
 *                         fbe_device_physical_location_t * pLocation,
 *                         fbe_base_env_fup_persistent_info_t ** pPsFupInfoPtr)
 ****************************************************************
 * @brief
 *  Find the pointer for the sps persistent fup info which including the fup info for all components.
 *
 * @param pContext - The pointer to the callback context.
 * @param deviceType -
 * @param pLocation - 
 * @param pComponentFupInfoPtr - 
 *
 * @return fbe_status_t
 *
 * @version
 *  05-Oct-2012: PHE - Created.
 ****************************************************************/
fbe_status_t fbe_sps_mgmt_get_sps_fup_info_ptr(void * pContext,
                                            fbe_u64_t deviceType,
                                            fbe_device_physical_location_t * pLocation,
                                            fbe_sps_fup_info_t ** pSpsFupInfoPtr)
{
    fbe_sps_mgmt_t                          *pSpsMgmt = (fbe_sps_mgmt_t *)pContext;
    fbe_status_t                             status = FBE_STATUS_OK;
    fbe_u32_t                                spsEnclIndex;
    fbe_u32_t                                spsIndex;
    fbe_char_t                               logHeader[FBE_BASE_ENV_FUP_MAX_LOG_HEADER_SIZE + 1] = {0};

    /* Set log header for tracing purpose. */
    fbe_sps_mgmt_fup_generate_logheader(pLocation, &logHeader[0], FBE_BASE_ENV_FUP_MAX_LOG_HEADER_SIZE);


    if(deviceType != FBE_DEVICE_TYPE_SPS) 
    {
        fbe_base_object_trace((fbe_base_object_t*)pSpsMgmt, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s, %s, unsupported deviceType 0x%llX.\n",
                              &logHeader[0],
                              __FUNCTION__,
                              deviceType);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_sps_mgmt_convertSpsLocation(pSpsMgmt, pLocation, &spsEnclIndex, &spsIndex);

    if(status != FBE_STATUS_OK) 
    {
        fbe_base_object_trace((fbe_base_object_t*)pSpsMgmt, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s, %s, failed to convert location.\n",
                              &logHeader[0],
                              __FUNCTION__);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    *pSpsFupInfoPtr = &(pSpsMgmt->arraySpsFupInfo->spsFupInfo[spsEnclIndex][spsIndex]);

    return status;
} //fbe_sps_mgmt_get_sps_fup_info_ptr


/*!**************************************************************
 * @fn fbe_sps_mgmt_get_component_fup_info_ptr(void * pContext,
 *                         fbe_u64_t deviceType,
 *                         fbe_device_physical_location_t * pLocation,
 *                         fbe_base_env_fup_persistent_info_t ** pComponentFupInfoPtr)
 ****************************************************************
 * @brief
 *  Find the pointer for the individual sps component (primary, secondary or battery ) persistent fup info.
 *
 * @param pContext - The pointer to the callback context.
 * @param deviceType -
 * @param pLocation - 
 * @param pComponentFupInfoPtr - 
 *
 * @return fbe_status_t
 *
 * @version
 *  08-Aug-2012: GB - Created.
 *  05-Oct-2012: PHE - Updated to use fbe_sps_mgmt_convertSpsLocation.
 ****************************************************************/
fbe_status_t fbe_sps_mgmt_get_component_fup_info_ptr(void * pContext,
                                            fbe_u64_t deviceType,
                                            fbe_device_physical_location_t * pLocation,
                                            fbe_base_env_fup_persistent_info_t ** pComponentFupInfoPtr)
{
    fbe_sps_mgmt_t                          *pSpsMgmt = (fbe_sps_mgmt_t *)pContext;
    fbe_sps_fup_info_t                      *pSpsFupInfo = NULL;
    fbe_status_t                             status = FBE_STATUS_OK;
    fbe_u32_t                                spsEnclIndex;
    fbe_u32_t                                spsIndex;
    fbe_char_t                               logHeader[FBE_BASE_ENV_FUP_MAX_LOG_HEADER_SIZE + 1] = {0};

    /* Set log header for tracing purpose. */
    fbe_sps_mgmt_fup_generate_logheader(pLocation, &logHeader[0], FBE_BASE_ENV_FUP_MAX_LOG_HEADER_SIZE);


    if(deviceType != FBE_DEVICE_TYPE_SPS) 
    {
        fbe_base_object_trace((fbe_base_object_t*)pSpsMgmt, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s, %s, unsupported deviceType 0x%llX.\n",
                              &logHeader[0],
                              __FUNCTION__,
                              deviceType);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_sps_mgmt_convertSpsLocation(pSpsMgmt, pLocation, &spsEnclIndex, &spsIndex);

    if(status != FBE_STATUS_OK) 
    {
        fbe_base_object_trace((fbe_base_object_t*)pSpsMgmt, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s, %s, failed to convert location.\n",
                              &logHeader[0],
                              __FUNCTION__);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    pSpsFupInfo = &(pSpsMgmt->arraySpsFupInfo->spsFupInfo[spsEnclIndex][spsIndex]);

    *pComponentFupInfoPtr = &(pSpsFupInfo->fupPersistentInfo[pLocation->componentId]);

    return status;
} //fbe_sps_mgmt_get_fup_info_ptr


/*!***************************************************************
 * fbe_sps_mgmt_fup_resume_upgrade()
 ****************************************************************
 * @brief
 *  This function resumes the ugrade for the devices which were aborted.
 *
 * @param pContext -
 * 
 * @return fbe_status_t
 *
 * @author
 *  08-Aug-2012: GB - Created.
 *  11-Oct-2012: PHE - modified to use fbe_sps_mgmt_convertSpsIndex
 ****************************************************************/
fbe_status_t fbe_sps_mgmt_fup_resume_upgrade(void * pContext)
{
    fbe_sps_mgmt_t                          * pSpsMgmt = (fbe_sps_mgmt_t *)pContext;
    fbe_sps_fup_info_t                      * pSpsFupInfo = NULL;
    fbe_sps_info_t                          * pSpsInfo = NULL;
    fbe_u32_t                                 spsEnclIndex = 0;
    fbe_u32_t                                 spsIndex = 0;
    fbe_sps_cid_enum_t                        cid = FBE_SPS_COMPONENT_ID_MAX;
    fbe_device_physical_location_t            location = {0};
    fbe_status_t                              status = FBE_STATUS_OK;

    fbe_base_object_trace((fbe_base_object_t *)pSpsMgmt,
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "FUP: Resume upgrade for SPS.\n");

    for(spsEnclIndex = 0; spsEnclIndex < FBE_SPS_MGMT_ENCL_MAX; spsEnclIndex ++)
    {
        for(spsIndex = 0; spsIndex < FBE_SPS_MAX_COUNT; spsIndex ++) 
        {
            pSpsFupInfo = &(pSpsMgmt->arraySpsFupInfo->spsFupInfo[spsEnclIndex][spsIndex]); 
            pSpsInfo = &(pSpsMgmt->arraySpsInfo->sps_info[spsEnclIndex][spsIndex]); 

            for(cid = FBE_SPS_COMPONENT_ID_PRIMARY; cid < pSpsInfo->programmableCount; cid ++) 
            {
                if(cid >= FBE_SPS_COMPONENT_ID_MAX) 
                {
                    /* Added the check to avoid the panic when the data got corrupted.*/
                    break;
                }
                if(pSpsFupInfo->fupPersistentInfo[cid].completionStatus == FBE_BASE_ENV_FUP_COMPLETION_STATUS_ABORTED)
                {
                    status = fbe_sps_mgmt_convertSpsIndex(pSpsMgmt, spsEnclIndex, spsIndex, &location);
                    if(status != FBE_STATUS_OK)
                    {
                        fbe_base_object_trace((fbe_base_object_t *)pSpsMgmt,
                                              FBE_TRACE_LEVEL_INFO,
                                              FBE_TRACE_MESSAGE_ID_INFO,
                                              "FUP: %s,failed to covertSpsIndex, spsEnclIndex %d, spsIndex %d.\n",
                                              __FUNCTION__, spsEnclIndex, spsIndex);
                        continue;
                    }

                    location.componentId = cid;
                    status = fbe_sps_mgmt_fup_initiate_upgrade(pSpsMgmt, 
                                                      FBE_DEVICE_TYPE_SPS, 
                                                      &location,
                                                      pSpsFupInfo->fupPersistentInfo[cid].forceFlags,
                                                      0);  //upgradeRetryCount
                    if(status != FBE_STATUS_OK)
                    {
                        fbe_base_object_trace((fbe_base_object_t *)pSpsMgmt,
                                              FBE_TRACE_LEVEL_WARNING,
                                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                              "FUP:SPS(%d_%d_%d.%d), %s, Failed to initiate upgrade, status 0x%X.\n",
                                              location.bus,
                                              location.enclosure,
                                              location.slot,
                                              location.componentId,
                                              __FUNCTION__,
                                              status);
                    }
                }
            }
        }
    }

    return FBE_STATUS_OK;
}/* End of function fbe_sps_mgmt_fup_resume_upgrade */
       
/*!**************************************************************
 * @fn fbe_sps_mgmt_get_fup_info(void * pContext,
 *                         fbe_u64_t deviceType,
 *                         fbe_device_physical_location_t * pLocation,
 *                         fbe_base_env_fup_persistent_info_t ** pPsFupInfoPtr)
 ****************************************************************
 * @brief
 *  This function copies the firmware upgrade info to the location with given pointer
 *  for a specific sps(including all the programmables, primary, secondary, and battery).
 *
 * @param pContext - The pointer to the callback context.
 * @param deviceType -
 * @param pLocation - 
 * @param pGetFupInfo - 
 *
 * @return fbe_status_t
 *
 * @version
 *  06-Oct-2012:  PHE - Created. 
 *
 ****************************************************************/
fbe_status_t fbe_sps_mgmt_get_fup_info(void * pContext,
                                            fbe_u64_t deviceType,
                                            fbe_device_physical_location_t * pLocation,
                                            fbe_esp_base_env_get_fup_info_t *  pGetFupInfo)
{
    fbe_sps_mgmt_t                         * pSpsMgmt = (fbe_sps_mgmt_t *)pContext;
    fbe_sps_info_t                         * pSpsInfo = NULL;
    fbe_sps_fup_info_t                     * pSpsFupInfo = NULL;
    fbe_status_t                             status = FBE_STATUS_OK;
    fbe_base_env_fup_work_item_t           * pWorkItem = NULL;
    fbe_sps_cid_enum_t                       cid = FBE_SPS_COMPONENT_ID_MAX;
    fbe_u32_t                                spsEnclIndex = 0;
    fbe_u32_t                                spsIndex = 0;
    fbe_char_t                               logHeader[FBE_BASE_ENV_FUP_MAX_LOG_HEADER_SIZE + 1] = {0};
    fbe_device_physical_location_t           location = {0};

    if(deviceType != FBE_DEVICE_TYPE_SPS)
    {
        fbe_base_object_trace((fbe_base_object_t*)pSpsMgmt, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s, unsupported deviceType 0x%llX.\n",
                              __FUNCTION__,
                              deviceType);

        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    /* Set log header for tracing purpose. */
    fbe_sps_mgmt_fup_generate_logheader(pLocation, &logHeader[0], FBE_BASE_ENV_FUP_MAX_LOG_HEADER_SIZE);

    status = fbe_sps_mgmt_convertSpsLocation(pSpsMgmt, pLocation, &spsEnclIndex, &spsIndex);

    if(status != FBE_STATUS_OK) 
    {
        fbe_base_object_trace((fbe_base_object_t*)pSpsMgmt, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s, %s, failed to convert location.\n",
                              &logHeader[0],
                              __FUNCTION__);

        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    pSpsInfo = &(pSpsMgmt->arraySpsInfo->sps_info[spsEnclIndex][spsIndex]);
    pSpsFupInfo = &(pSpsMgmt->arraySpsFupInfo->spsFupInfo[spsEnclIndex][spsIndex]);

    pGetFupInfo->programmableCount = pSpsInfo->programmableCount;

    location.bus = pLocation->bus;
    location.enclosure = pLocation->enclosure;
    location.slot = pLocation->slot;

    for(cid = FBE_SPS_COMPONENT_ID_PRIMARY; cid < pGetFupInfo->programmableCount; cid ++ ) 
    {
        // Initialize the workState to FBE_BASE_ENV_FUP_WORK_STATE_NONE. 
        pGetFupInfo->fupInfo[cid].workState = FBE_BASE_ENV_FUP_WORK_STATE_NONE;
        
        location.componentId = cid;
        status = fbe_base_env_fup_find_work_item((fbe_base_environment_t *)pContext, deviceType, &location, &pWorkItem);
        if(pWorkItem != NULL)
        {
            pGetFupInfo->fupInfo[cid].workState = pWorkItem->workState;

            if (pGetFupInfo->fupInfo[cid].workState == FBE_BASE_ENV_FUP_WORK_STATE_WAIT_BEFORE_UPGRADE)  
            {
                pGetFupInfo->fupInfo[cid].waitTimeBeforeUpgrade = fbe_base_env_get_wait_time_before_upgrade((fbe_base_environment_t *)pContext);
            }
        }
    
        pGetFupInfo->fupInfo[cid].componentId = cid;
        pGetFupInfo->fupInfo[cid].completionStatus = pSpsFupInfo->fupPersistentInfo[cid].completionStatus;
        fbe_copy_memory(&pGetFupInfo->fupInfo[cid].imageRev[0],
                            &pSpsFupInfo->fupPersistentInfo[cid].imageRev[0],
                            FBE_ENCLOSURE_MAX_FIRMWARE_REV_SIZE + 1);
        fbe_copy_memory(&pGetFupInfo->fupInfo[cid].currentFirmwareRev[0],
                            &pSpsFupInfo->fupPersistentInfo[cid].currentFirmwareRev[0],
                            FBE_ENCLOSURE_MAX_FIRMWARE_REV_SIZE + 1);
        fbe_copy_memory(&pGetFupInfo->fupInfo[cid].preFirmwareRev[0],
                            &pSpsFupInfo->fupPersistentInfo[cid].preFirmwareRev[0],
                            FBE_ENCLOSURE_MAX_FIRMWARE_REV_SIZE + 1);
    }
    
    return FBE_STATUS_OK;
} // End of function fbe_ps_mgmt_get_fup_info

/*!**************************************************************
 * @fn fbe_sps_mgmt_fup_refresh_device_status(void * pContext,
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
fbe_status_t fbe_sps_mgmt_fup_refresh_device_status(void * pContext,
                                             fbe_base_env_fup_work_item_t * pWorkItem)
{
    fbe_sps_mgmt_t                   * pSpsMgmt = (fbe_sps_mgmt_t *)pContext;
    fbe_u32_t                          spsEnclIndex = 0;
    fbe_u32_t                          spsIndex = 0;
    fbe_status_t                       status = FBE_STATUS_OK;
    fbe_device_physical_location_t   * pLocation = NULL;

    if(pWorkItem->deviceType != FBE_DEVICE_TYPE_SPS)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)pSpsMgmt,
                        FBE_TRACE_LEVEL_ERROR,
                        fbe_base_env_get_fup_work_item_logheader(pWorkItem),
                        "%s unsupported deviceType 0x%llX.\n",   
                        __FUNCTION__, pWorkItem->deviceType);

        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    pLocation = &pWorkItem->location;

    status = fbe_sps_mgmt_convertSpsLocation(pSpsMgmt, pLocation, &spsEnclIndex, &spsIndex);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)pSpsMgmt,
                        FBE_TRACE_LEVEL_INFO,
                        fbe_base_env_get_fup_work_item_logheader(pWorkItem),
                        "Refresh device status, convertSpsLocation failed.\n");

        return FBE_STATUS_GENERIC_FAILURE;
    }

    if(((spsEnclIndex == FBE_SPS_MGMT_PE) && 
       (pWorkItem->workState >= FBE_BASE_ENV_FUP_WORK_STATE_DOWNLOAD_IMAGE_SENT)) ||
       ((spsEnclIndex == FBE_SPS_MGMT_DAE0) && 
       (pWorkItem->workState >= FBE_BASE_ENV_FUP_WORK_STATE_ACTIVATE_IMAGE_SENT)))
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)pSpsMgmt,
                        FBE_TRACE_LEVEL_INFO,
                        fbe_base_env_get_fup_work_item_logheader(pWorkItem),
                        "Refresh device status...\n");
    }
    else
    {
        return FBE_STATUS_OK;
    }

    status = fbe_sps_mgmt_processSpsStatus(pSpsMgmt, spsEnclIndex, spsIndex);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)pSpsMgmt,
                        FBE_TRACE_LEVEL_INFO,
                        fbe_base_env_get_fup_work_item_logheader(pWorkItem),
                        "Refresh device status, processSpsStatusfailed, spsEnclIndex %d spsIndex %d.\n",
                         spsEnclIndex,
                         spsIndex);

        return FBE_STATUS_GENERIC_FAILURE;
    }
    

    return FBE_STATUS_OK;
}

/*!***************************************************************
 * @fn fbe_sps_mgmt_fup_new_contact_init_upgrade(fbe_sps_mgmt_t *pSpsMgmt)
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
 * @param pSpsMgmt -pointer to fbe_sps_mgmt_t
 *      
 * @return fbe_status_t
 *
 * @author
 *  211-Dec-2014: PHE - Created. 
 *
 ****************************************************************/
fbe_status_t fbe_sps_mgmt_fup_new_contact_init_upgrade(fbe_sps_mgmt_t *pSpsMgmt)
{
    fbe_sps_fup_info_t                      * pSpsFupInfo = NULL;
    fbe_sps_info_t                          * pSpsInfo = NULL;
    fbe_u32_t                                 spsEnclIndex = 0;
    fbe_u32_t                                 spsIndex = 0;
    fbe_sps_cid_enum_t                        cid = FBE_SPS_COMPONENT_ID_MAX;
    fbe_device_physical_location_t            location = {0};
    fbe_status_t                              status = FBE_STATUS_OK;

    // Peer contact back, update SPS FW. 
    fbe_base_object_trace((fbe_base_object_t *)pSpsMgmt,
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s peer SP contact is back.\n",
                                  __FUNCTION__);

    for(spsEnclIndex = 0; spsEnclIndex < FBE_SPS_MGMT_ENCL_MAX; spsEnclIndex ++)
    {
        for(spsIndex = 0; spsIndex < FBE_SPS_MAX_COUNT; spsIndex ++) 
        {
            pSpsFupInfo = &(pSpsMgmt->arraySpsFupInfo->spsFupInfo[spsEnclIndex][spsIndex]); 
            pSpsInfo = &(pSpsMgmt->arraySpsInfo->sps_info[spsEnclIndex][spsIndex]); 

            for(cid = FBE_SPS_COMPONENT_ID_PRIMARY; cid < pSpsInfo->programmableCount; cid ++) 
            {
                if(cid >= FBE_SPS_COMPONENT_ID_MAX) 
                {
                    /* Added the check to avoid the panic when the data got corrupted.*/
                    break;
                }
                if(pSpsFupInfo->fupPersistentInfo[cid].completionStatus == FBE_BASE_ENV_FUP_COMPLETION_STATUS_FAIL_NO_PEER_PERMISSION)
                {
                    status = fbe_sps_mgmt_convertSpsIndex(pSpsMgmt, spsEnclIndex, spsIndex, &location);
                    if(status != FBE_STATUS_OK)
                    {
                        fbe_base_object_trace((fbe_base_object_t *)pSpsMgmt,
                                              FBE_TRACE_LEVEL_INFO,
                                              FBE_TRACE_MESSAGE_ID_INFO,
                                              "FUP: %s,failed to covertSpsIndex, spsEnclIndex %d, spsIndex %d.\n",
                                              __FUNCTION__, spsEnclIndex, spsIndex);
                        continue;
                    }

                    location.componentId = cid;
                    status = fbe_sps_mgmt_fup_initiate_upgrade(pSpsMgmt, 
                                                      FBE_DEVICE_TYPE_SPS, 
                                                      &location,
                                                      pSpsFupInfo->fupPersistentInfo[cid].forceFlags,
                                                      0);  //upgradeRetryCount
                    if(status != FBE_STATUS_OK)
                    {
                        fbe_base_object_trace((fbe_base_object_t *)pSpsMgmt,
                                              FBE_TRACE_LEVEL_WARNING,
                                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                              "FUP:SPS(%d_%d_%d.%d), %s, Failed to initiate upgrade, status 0x%X.\n",
                                              location.bus,
                                              location.enclosure,
                                              location.slot,
                                              location.componentId,
                                              __FUNCTION__,
                                              status);
                    }
                }
            }
        }
    }

    return FBE_STATUS_OK;
} //fbe_sps_mgmt_fup_new_contact_init_upgrade

//End of file fbe_sps_mgmt_fup.c
