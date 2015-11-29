/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_ps_mgmt_fup.c
 ***************************************************************************
 * @brief
 *  This file contains the implemention for Power Supply firmware upgrade functionality.
 *
 * @version
 *   15-June-2010:  PHE - Created.
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_memory.h"
#include "fbe/fbe_api_enclosure_interface.h"
#include "fbe/fbe_api_power_supply_interface.h"
#include "fbe_base_object.h"
#include "fbe_ps_mgmt_private.h"
#include "fbe_ps_mgmt.h"
#include "fbe_base_environment_debug.h"

static fbe_status_t fbe_ps_mgmt_fup_qualified(fbe_ps_mgmt_t * pPsMgmt,
                                              fbe_device_physical_location_t * pLocation,
                                              fbe_power_supply_info_t * pPsInfo, 
                                              fbe_bool_t * pUpgradeQualified);

static fbe_status_t fbe_ps_mgmt_fup_policy_allowed(fbe_ps_mgmt_t * pPsMgmt, 
                                        fbe_power_supply_info_t * pPsInfo, 
                                        fbe_bool_t * pPolicyAllowed);

static void fbe_ps_mgmt_fup_restart_env_failed_fup(fbe_ps_mgmt_t *pPsMgmt, 
                                                   fbe_device_physical_location_t *pLocation, 
                                                   fbe_power_supply_info_t *pPsInfo);

static fbe_status_t fbe_ps_mgmt_fup_generate_logheader(fbe_device_physical_location_t * pLocation, 
                                       fbe_u8_t * pLogHeader, fbe_u32_t logHeaderSize);

static fbe_status_t fbe_ps_mgmt_fup_create_and_add_work_item_from_manifest(fbe_ps_mgmt_t * pPsMgmt,
                                                                          fbe_device_physical_location_t * pLocation,
                                                                          fbe_power_supply_info_t * pPsInfo,
                                                                          fbe_u32_t forceFlags,
                                                                          fbe_char_t * pLogHeader,
                                                                          fbe_u32_t interDeviceDelayInSec,
                                                                          fbe_u32_t upgradeRetryCount);

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
 *  12-Jul-2010:  PHE - Created. 
 *
 ****************************************************************/
fbe_status_t fbe_ps_mgmt_fup_handle_ps_status_change(fbe_ps_mgmt_t * pPsMgmt,
                                                     fbe_device_physical_location_t * pLocation,
                                                     fbe_power_supply_info_t * pNewPsInfo,
                                                     fbe_power_supply_info_t * pOldPsInfo,
                                                     fbe_bool_t debounceInProgress)
{
    fbe_bool_t                     newQualified;
    fbe_bool_t                     oldQualified;
    fbe_status_t                   status = FBE_STATUS_OK;
    fbe_char_t                     logHeader[FBE_BASE_ENV_FUP_MAX_LOG_HEADER_SIZE + 1] = {0};
    fbe_bool_t                     policyAllowUpgrade = FBE_FALSE;

    /* Set log header for tracing purpose. */
    fbe_ps_mgmt_fup_generate_logheader(pLocation, &logHeader[0], FBE_BASE_ENV_FUP_MAX_LOG_HEADER_SIZE);

    if((!pNewPsInfo->bInserted) && (pOldPsInfo->bInserted))
    {
        /* The power supply was removed. */
        fbe_base_object_trace((fbe_base_object_t *)pPsMgmt,
                                      FBE_TRACE_LEVEL_WARNING,
                                      FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                      "%s, Device removed.\n",
                                      &logHeader[0]);

        status = fbe_base_env_fup_handle_device_removal((fbe_base_environment_t *)pPsMgmt, 
                                                        FBE_DEVICE_TYPE_PS, 
                                                        pLocation);
        if(status != FBE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t *)pPsMgmt,
                                  FBE_TRACE_LEVEL_WARNING,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s, %s, Failed to handle device removal, status 0x%X.\n",
                                  &logHeader[0],
                                  __FUNCTION__,
                                  status);
    
            return status;
        }
    }
    else 
    {
        if(debounceInProgress)
        {
            fbe_base_object_trace((fbe_base_object_t *)pPsMgmt,
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s %s Debounceing PS Fault, FUP not allowed\n",
                                  &logHeader[0],
                                  __FUNCTION__);
            return FBE_STATUS_OK;
        }

        // need to call this before fbe_ps_mgmt_fup_policy_allowed()
        fbe_ps_mgmt_fup_restart_env_failed_fup(pPsMgmt, pLocation, pNewPsInfo);

        fbe_ps_mgmt_fup_policy_allowed(pPsMgmt, pNewPsInfo, &policyAllowUpgrade);
        if(!policyAllowUpgrade)
        {
            fbe_base_object_trace((fbe_base_object_t *)pPsMgmt,
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s spid:%d,%s, policy does not allow upgrade.\n",
                                  &logHeader[0],
                                  ((fbe_base_environment_t *)pPsMgmt)->spid,
                                  __FUNCTION__);
            return FBE_STATUS_OK;
        }

        fbe_ps_mgmt_fup_qualified(pPsMgmt, pLocation, pNewPsInfo, &newQualified);
        fbe_ps_mgmt_fup_qualified(pPsMgmt, pLocation, pOldPsInfo, &oldQualified);

        if(newQualified && !oldQualified)
        {
            
            fbe_base_object_trace((fbe_base_object_t *)pPsMgmt,
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s, %s, became qualified for upgrade.\n",
                                  &logHeader[0],
                                  __FUNCTION__);

            status = fbe_ps_mgmt_fup_initiate_upgrade(pPsMgmt, 
                                                      FBE_DEVICE_TYPE_PS, 
                                                      pLocation,
                                                      FBE_BASE_ENV_FUP_FORCE_FLAG_NONE,
                                                      0);  //upgradeRetryCount
            if(status != FBE_STATUS_OK)
            {
                 fbe_base_object_trace((fbe_base_object_t *)pPsMgmt,
                                      FBE_TRACE_LEVEL_WARNING,
                                      FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                      "%s, %s, Failed to initiate upgrade, status 0x%X.\n",
                                      &logHeader[0],
                                      __FUNCTION__,
                                      status);
            }
        }
    }

    return status;
}

/*!**************************************************************
 * @fn fbe_ps_mgmt_fup_policy_allowed(fbe_ps_mgmt_t * pPsMgmt, 
 *                                    fbe_device_physical_location_t * location)
 ****************************************************************
 * @brief
 *  This function checks whether the PS is qualified to the upgrade or not.
 *
 * @param pPsMgmt - The current power supply info.
 * @param pPsInfo -
 * @param pPolicyAllowed(OUTPUT) -
 * 
 * @return fbe_status_t - always return FBE_STATUS_OK;
 *
 * @version
 *  12-Jul-2010:  PHE - Created. 
 *
 ****************************************************************/
static fbe_status_t
fbe_ps_mgmt_fup_policy_allowed(fbe_ps_mgmt_t * pPsMgmt, 
                               fbe_power_supply_info_t * pPsInfo, 
                               fbe_bool_t * pPolicyAllowed)
{
    fbe_base_environment_t * pBaseEnv = (fbe_base_environment_t *)pPsMgmt; 
    *pPolicyAllowed = FALSE;

    if((pBaseEnv->spid == pPsInfo->associatedSp) ||
       (pPsMgmt->base_environment.isSingleSpSystem == TRUE))
    {
        *pPolicyAllowed = TRUE;
    }

    return FBE_STATUS_OK;
}


/*!**************************************************************
 * @fn fbe_ps_mgmt_fup_qualified(fbe_ps_mgmt_t * pPsMgmt,
 *                               fbe_power_supply_info_t * pPsInfo, 
 *                               fbe_bool_t * pUpgradeQualified)
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
 *  12-Jul-2010:  PHE - Created. 
 *
 ****************************************************************/
static fbe_status_t fbe_ps_mgmt_fup_qualified(fbe_ps_mgmt_t * pPsMgmt,
                                              fbe_device_physical_location_t * pLocation,
                                              fbe_power_supply_info_t * pPsInfo, 
                                              fbe_bool_t * pUpgradeQualified)
{
    fbe_char_t                     logHeader[FBE_BASE_ENV_FUP_MAX_LOG_HEADER_SIZE + 1] = {0};

    /* Set log header for tracing purpose. */
    fbe_ps_mgmt_fup_generate_logheader(pLocation, &logHeader[0], FBE_BASE_ENV_FUP_MAX_LOG_HEADER_SIZE);

    *pUpgradeQualified = ((pPsInfo->bDownloadable) && 
              (pPsInfo->bInserted) &&
              (pPsInfo->generalFault == FBE_MGMT_STATUS_FALSE));

    if(*pUpgradeQualified != TRUE)
    {
        fbe_base_object_trace((fbe_base_object_t *)pPsMgmt,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s, not qualified for upgrade, bDownloadable %d, inserted %d, genFaulted %d.\n",
                              &logHeader[0],
                              pPsInfo->bDownloadable,
                              pPsInfo->bInserted,
                              pPsInfo->generalFault);
    }
    
        return FBE_STATUS_OK;
}

/*!**************************************************************
 * @fn fbe_ps_mgmt_fup_restart_env_failed_fup(fbe_ps_mgmt_t * pPsMgmt,
 *                               fbe_device_physical_location_t *pLocation,
 *                               fbe_power_supply_info_t *pPsInfo)
 ****************************************************************
 * @brief
 *  If the ps was failed for bad env status, initiate the upgrade.
 * 
 *  When peer ps is faulted it will prevent local ps from getting
 *  upgraded. When the fault clears we need to come through here
 *  to check if a new upgrade needs to be started.
 * 
 * @param pPsMgmt - ps mgmt object
 * @param pPsInfo - The power supply info for the physical package.
 * @param pLocation - bus, encl, slot for the ps
 * 
 * @return fbe_status_t - always return FBE_STATUS_OK;
 *
 * @version
 *  3-Mar-2015:  GB - Created. 
 ****************************************************************/
static void fbe_ps_mgmt_fup_restart_env_failed_fup(fbe_ps_mgmt_t *pPsMgmt, 
                                                   fbe_device_physical_location_t *pLocation, 
                                                   fbe_power_supply_info_t *pPsInfo)
{
    fbe_base_environment_t  * pBaseEnv =(fbe_base_environment_t *)pPsMgmt;
    fbe_u8_t                slot = 0;
    fbe_u8_t                savedSlot;
    fbe_encl_ps_info_t      * pEnclPsInfo = NULL;
    fbe_status_t            status = FBE_STATUS_OK;
    fbe_char_t                     logHeader[FBE_BASE_ENV_FUP_MAX_LOG_HEADER_SIZE + 1] = {0};

    /* Set log header for tracing purpose. */
    fbe_ps_mgmt_fup_generate_logheader(pLocation, &logHeader[0], FBE_BASE_ENV_FUP_MAX_LOG_HEADER_SIZE);
    fbe_base_object_trace((fbe_base_object_t *)pPsMgmt,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                              "%s entry\n",
                              __FUNCTION__);
    // if this is the local side, no need to check anything, only
    // need to check when the peer side info has changed
    if(pBaseEnv->spid == pPsInfo->associatedSp)
    {
        return;
    }

    status = fbe_ps_mgmt_get_encl_ps_info_by_location(pPsMgmt,
                                                      pLocation, 
                                                      &pEnclPsInfo);

    if(status != FBE_STATUS_OK)
    {
        // No enclosure index found...bug
        fbe_base_object_trace((fbe_base_object_t *)pPsMgmt,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s restart_env_failed_fup, get_encl_ps_info_by_location failed.\n",
                              &logHeader[0]);

        return;
    }

    for(slot = 0; slot < pEnclPsInfo->psCount; slot ++) 
    {
        fbe_base_object_trace((fbe_base_object_t *)pPsMgmt,
                              FBE_TRACE_LEVEL_INFO, //FBE_TRACE_LEVEL_DEBUG_MEDIUM,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s restart_env_failed_fup,slot index:%d completion status:0x%x\n",
                              &logHeader[0],
                              slot,
                              pEnclPsInfo->psFupInfo[slot].completionStatus);

        // It was determined above that this ps info is for the peer as is the location.slot. When the loop index doesn't match
        // location.slot we know loop index is for the local ps slot, so check it for bad env status
        if(slot != pLocation->slot) 
        {
            if(pEnclPsInfo->psFupInfo[slot].completionStatus == FBE_BASE_ENV_FUP_COMPLETION_STATUS_FAIL_BAD_ENV_STATUS)
            {
                savedSlot = pLocation->slot;
                pLocation->slot = slot;
                fbe_base_object_trace((fbe_base_object_t *)pPsMgmt,
                                      FBE_TRACE_LEVEL_INFO,
                                      FBE_TRACE_MESSAGE_ID_INFO,
                                      " Restart the Failed Bad env Status FUP:PS(%d_%d_%d)\n",
                                      pLocation->bus,
                                      pLocation->enclosure,
                                      pLocation->slot);

                // restart the upgrade
                status = fbe_ps_mgmt_fup_initiate_upgrade(pPsMgmt, 
                                                          FBE_DEVICE_TYPE_PS, 
                                                          pLocation,
                                                          FBE_BASE_ENV_FUP_FORCE_FLAG_NONE,
                                                          0);  //upgradeRetryCount
                if(status != FBE_STATUS_OK)
                {
                    fbe_base_object_trace((fbe_base_object_t *)pPsMgmt,
                                          FBE_TRACE_LEVEL_WARNING,
                                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                          "Restart_env_failed_fup, Failed to initiate upgrade for PS(%d_%d_%d), status 0x%X.\n",
                                          pLocation->bus,
                                          pLocation->enclosure,
                                          pLocation->slot,
                                          status);
                }
                pLocation->slot = savedSlot;
            }
        }
    }
    return;

} //fbe_ps_mgmt_fup_restart_env_failed_fup

/*!**************************************************************
 * @fn fbe_ps_mgmt_fup_initiate_upgrade(void * pContext, 
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
 * 
 * @return fbe_status_t
 *
 * @version
 *  15-June-2010:  PHE - Created. 
 *
 ****************************************************************/
fbe_status_t fbe_ps_mgmt_fup_initiate_upgrade(void * pContext, 
                                              fbe_u64_t deviceType,
                                              fbe_device_physical_location_t * pLocation,
                                              fbe_u32_t forceFlags,
                                              fbe_u32_t upgradeRetryCount)
{
    fbe_ps_mgmt_t              * pPsMgmt = (fbe_ps_mgmt_t *)pContext;
    fbe_encl_ps_info_t         * pEnclPsInfo = NULL;
    fbe_power_supply_info_t    * pPsInfo = NULL;
    fbe_bool_t                   policyAllowUpgrade = FALSE;
    fbe_bool_t                   upgradeQualified = FALSE;
    fbe_char_t                   logHeader[FBE_BASE_ENV_FUP_MAX_LOG_HEADER_SIZE + 1] = {0};
    fbe_status_t                 status = FBE_STATUS_OK;

    if(deviceType != FBE_DEVICE_TYPE_PS)
    {
        fbe_base_object_trace((fbe_base_object_t *)pPsMgmt,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "FUP: %s, unsupported deviceType 0x%llX.\n",
                              __FUNCTION__,
                              deviceType);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_ps_mgmt_get_encl_ps_info_by_location(pPsMgmt, pLocation, &pEnclPsInfo);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)pPsMgmt,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "FUP:PS(%d_%d_%d), %s, get_encl_ps_info_by_location failed.\n",
                              pLocation->bus,
                              pLocation->enclosure,
                              pLocation->slot,
                              __FUNCTION__);

        return status;
    }

    pPsInfo = &pEnclPsInfo->psInfo[pLocation->slot];

    fbe_ps_mgmt_fup_policy_allowed(pPsMgmt, pPsInfo, &policyAllowUpgrade);
    if(!policyAllowUpgrade)
    {
        fbe_base_object_trace((fbe_base_object_t *)pPsMgmt,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "FUP:PS(%d_%d_%d) spid:%d,%s, policy does not allow upgrade.\n",
                              pLocation->bus,
                              pLocation->enclosure,
                              pLocation->slot,
                              ((fbe_base_environment_t *)pPsMgmt)->spid,
                              __FUNCTION__);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_ps_mgmt_fup_qualified(pPsMgmt, pLocation, pPsInfo, &upgradeQualified);
    if(!upgradeQualified)
    {
        fbe_base_object_trace((fbe_base_object_t *)pPsMgmt,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "FUP:PS(%d_%d_%d), %s, not qualified for upgrade.\n",
                              pLocation->bus,
                              pLocation->enclosure,
                              pLocation->slot,
                              __FUNCTION__);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Set log header for tracing and logging purpose. */
    fbe_ps_mgmt_fup_generate_logheader(pLocation, &logHeader[0], FBE_BASE_ENV_FUP_MAX_LOG_HEADER_SIZE);

    if(pPsInfo->esesVersionDesc == ESES_REVISION_2_0)
    {
        // get image detail from manifest
        status = fbe_ps_mgmt_fup_create_and_add_work_item_from_manifest(pPsMgmt,                                           
                                                                        pLocation,
                                                                        pPsInfo,
                                                                        forceFlags,
                                                                        &logHeader[0],
                                                                        0,  // inter device delay, 0 for ps
                                                                        upgradeRetryCount);   
    }
    else
    {
        status = fbe_base_env_fup_create_and_add_work_item((fbe_base_environment_t *)pPsMgmt,
                                                           pLocation,
                                                           FBE_DEVICE_TYPE_PS,
                                                           FBE_FW_TARGET_PS,
                                                           &pPsInfo->firmwareRev[0],
                                                           &pPsInfo->subenclProductId[0],
                                                           pPsInfo->uniqueId,
                                                           forceFlags,
                                                           &logHeader[0],
                                                           0,   // inter device delay, 0 for ps
                                                           upgradeRetryCount,
                                                           ESES_REVISION_1_0);

    }

    // create_and_add_work_item traces fail status, is this trace also needed?
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)pPsMgmt,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "FUP:PS(%d_%d_%d), %s, Failed to create_and_add_work_item, status 0x%X.\n",
                              pLocation->bus,
                              pLocation->enclosure,
                              pLocation->slot,
                              __FUNCTION__,
                              status);
    }

    return status;
}
    
    

/*!**************************************************************
 * @fn fbe_ps_mgmt_fup_generate_logheader(fbe_device_physical_location_t * pLocation, 
 *                               fbe_u8_t * logHeader)
 ****************************************************************
 * @brief
 *  This function sets the log header for the power supply.
 *
 * @param pLocation - The pointer to the ps location.
 * @param pLogHeader(output) - The pointer to the log header.
 * @param logHeaderSize -
 *
 * @return fbe_status_t
 *
 * @version
 *  15-June-2010:  PHE - Created. 
 *
 ****************************************************************/
static fbe_status_t fbe_ps_mgmt_fup_generate_logheader(fbe_device_physical_location_t * pLocation, 
                                   fbe_u8_t * pLogHeader, fbe_u32_t logHeaderSize)
{
    fbe_zero_memory(pLogHeader, logHeaderSize);

    csx_p_snprintf(pLogHeader, logHeaderSize, "FUP:PS(%d_%d_%d)",
                       pLocation->bus, pLocation->enclosure, pLocation->slot);

    return FBE_STATUS_OK;
}


 /*!*******************************************************************************
 * @fn fbe_ps_mgmt_fup_get_full_image_path
 *********************************************************************************
 * @brief
 *  This function gets the full power supply image path. It includes the file name. 
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
 *  15-June-2010 PHE -- Created.
 *******************************************************************************/
fbe_status_t fbe_ps_mgmt_fup_get_full_image_path(void * pContext,                               
                                                 fbe_base_env_fup_work_item_t * pWorkItem)
{
    fbe_base_environment_t * pBaseEnv =(fbe_base_environment_t *)pContext;
    fbe_char_t              imageFileName[FBE_BASE_ENV_FUP_MAX_IMAGE_FILE_NAME_LEN] = {0};
    fbe_u32_t               imageFileNameLen = 0;
    fbe_char_t              imageFileConstantPortionName[FBE_BASE_ENV_FUP_MAX_IMAGE_FILE_NAME_LEN] = {0};
    fbe_char_t              imagePathKey[256] = {0};
    fbe_u32_t               imagePathIndex = 0;
    fbe_char_t              *pImageFileName = NULL;
    fbe_status_t            status = FBE_STATUS_OK;
    
    // test for eses 2 because some ps are common to eses 1
    if(pWorkItem->esesVersion == ESES_REVISION_1_0)
    {
        if(pWorkItem->uniqueId == AC_ACBEL_VOYAGER)
        {
            imagePathIndex = FBE_PS_MGMT_CDES1_CDES11_INDEX;
    
            fbe_copy_memory(&imageFileConstantPortionName[0], 
                            FBE_PS_MGMT_CDES11_FUP_IMAGE_FILE_NAME,
                            sizeof(FBE_PS_MGMT_CDES11_FUP_IMAGE_FILE_NAME));
    
            fbe_copy_memory(&imagePathKey[0], 
                            FBE_PS_MGMT_CDES11_FUP_IMAGE_PATH_KEY,
                            sizeof(FBE_PS_MGMT_CDES11_FUP_IMAGE_PATH_KEY));
        }
        else if(pWorkItem->uniqueId == DAE_AC_1080) 
        {
            imagePathIndex = FBE_PS_MGMT_CDES1_UNIFIED_INDEX;
    
            fbe_copy_memory(&imageFileConstantPortionName[0], 
                            FBE_PS_MGMT_CDES_UNIFIED_FUP_IMAGE_FILE_NAME,
                            sizeof(FBE_PS_MGMT_CDES_UNIFIED_FUP_IMAGE_FILE_NAME));
    
            fbe_copy_memory(&imagePathKey[0], 
                            FBE_PS_MGMT_CDES_FUP_IMAGE_PATH_KEY,
                            sizeof(FBE_PS_MGMT_CDES_FUP_IMAGE_PATH_KEY));
        }
        else
        {
            imagePathIndex = FBE_PS_MGMT_CDES1_CDES_INDEX;
    
            fbe_copy_memory(&imageFileConstantPortionName[0], 
                            FBE_PS_MGMT_CDES_FUP_IMAGE_FILE_NAME,
                            sizeof(FBE_PS_MGMT_CDES_FUP_IMAGE_FILE_NAME));
    
            fbe_copy_memory(&imagePathKey[0], 
                            FBE_PS_MGMT_CDES_FUP_IMAGE_PATH_KEY,
                            sizeof(FBE_PS_MGMT_CDES_FUP_IMAGE_PATH_KEY));
        }
    }
    else if (pWorkItem->esesVersion == ESES_REVISION_2_0)
    {
        status = fbe_base_env_fup_find_file_name_from_manifest_info(pBaseEnv, pWorkItem, &pImageFileName);
        if(status != FBE_STATUS_OK)
        {
            fbe_base_object_customizable_trace((fbe_base_object_t*)pBaseEnv,
                                                FBE_TRACE_LEVEL_ERROR,
                                                fbe_base_env_get_fup_work_item_logheader(pWorkItem),
                                                "%s, fwTarge %d, failed to find the file name.\n",
                                                __FUNCTION__, pWorkItem->firmwareTarget);
    
            return status;
        }
    
        if(strncmp((char *)pImageFileName, FBE_PS_MGMT_CDES2_FUP_FILE_JUNO2, sizeof(FBE_PS_MGMT_CDES2_FUP_FILE_JUNO2)-1) == 0) 
        {
            /* Do not need to copy .bin because in simulation, the file name will be cdef_pidxxxx.bin. 
            * sizeof(FBE_ENCL_MGMT_CDES2_FUP_CDEF_IMAGE_FILE_NAME) is 5 but we only want to compare with "cdef" only. So we have to minus 1.
            */
            imagePathIndex = FBE_PS_MGMT_CDES2_JUNO_INDEX;

            fbe_copy_memory(&imageFileConstantPortionName[0], 
                            FBE_PS_MGMT_CDES2_FUP_FILE_JUNO2,
                            sizeof(FBE_PS_MGMT_CDES2_FUP_FILE_JUNO2));
        }
        else if((strncmp((char *)pImageFileName, FBE_PS_MGMT_CDES2_FUP_FILE_LASERBEAK, sizeof(FBE_PS_MGMT_CDES2_FUP_FILE_LASERBEAK)-1) == 0))
        {
            imagePathIndex = FBE_PS_MGMT_CDES2_LASERBEAK_INDEX;

            fbe_copy_memory(&imageFileConstantPortionName[0], 
                            FBE_PS_MGMT_CDES2_FUP_FILE_LASERBEAK,
                            sizeof(FBE_PS_MGMT_CDES2_FUP_FILE_LASERBEAK));
        }
        else if((strncmp((char *)pImageFileName, FBE_PS_MGMT_CDES2_FUP_FILE_3GVE, sizeof(FBE_PS_MGMT_CDES2_FUP_FILE_3GVE)-1) == 0))
        {
            imagePathIndex = FBE_PS_MGMT_CDES2_3GVE_INDEX;

            fbe_copy_memory(&imageFileConstantPortionName[0], 
                            FBE_PS_MGMT_CDES2_FUP_FILE_3GVE,
                            sizeof(FBE_PS_MGMT_CDES2_FUP_FILE_3GVE));
        }
        else if ((strncmp((char *)pImageFileName, FBE_PS_MGMT_CDES2_FUP_FILE_OCTANE, sizeof(FBE_PS_MGMT_CDES2_FUP_FILE_OCTANE)-1) == 0))
        {
            imagePathIndex = FBE_PS_MGMT_CDES2_OCTANE_INDEX;

            fbe_copy_memory(&imageFileConstantPortionName[0], 
                            FBE_PS_MGMT_CDES2_FUP_FILE_OCTANE,
                            sizeof(FBE_PS_MGMT_CDES2_FUP_FILE_OCTANE));
        }
        else if ((strncmp((char *)pImageFileName, FBE_PS_MGMT_CDES2_FUP_FILE_OPTIMUS_ACBEL, sizeof(FBE_PS_MGMT_CDES2_FUP_FILE_OPTIMUS_ACBEL)-1) == 0))
        {
            imagePathIndex = FBE_PS_MGMT_CDES2_OPTIMUS_ACBEL_INDEX;

            fbe_copy_memory(&imageFileConstantPortionName[0], 
                            FBE_PS_MGMT_CDES2_FUP_FILE_OPTIMUS_ACBEL,
                            sizeof(FBE_PS_MGMT_CDES2_FUP_FILE_OPTIMUS_ACBEL));
        }
        else if ((strncmp((char *)pImageFileName, FBE_PS_MGMT_CDES2_FUP_FILE_OPTIMUS_FLEX, sizeof(FBE_PS_MGMT_CDES2_FUP_FILE_OPTIMUS_FLEX)-1) == 0))
        {
            imagePathIndex = FBE_PS_MGMT_CDES2_OPTIMUS_FLEX_INDEX;

            fbe_copy_memory(&imageFileConstantPortionName[0], 
                            FBE_PS_MGMT_CDES2_FUP_FILE_OPTIMUS_FLEX,
                            sizeof(FBE_PS_MGMT_CDES2_FUP_FILE_OPTIMUS_FLEX));
        }
        else if ((strncmp((char *)pImageFileName, FBE_PS_MGMT_CDES2_FUP_FILE_TABASCO_DC, sizeof(FBE_PS_MGMT_CDES2_FUP_FILE_TABASCO_DC)-1) == 0))
        {
            imagePathIndex = FBE_PS_MGMT_CDES2_TABASCO_DC_INDEX;

            fbe_copy_memory(&imageFileConstantPortionName[0], 
                            FBE_PS_MGMT_CDES2_FUP_FILE_TABASCO_DC,
                            sizeof(FBE_PS_MGMT_CDES2_FUP_FILE_TABASCO_DC));
        }
        else if ((strncmp((char *)pImageFileName, FBE_PS_MGMT_CDES2_FUP_FILE_OPTIMUS_FLEX2, sizeof(FBE_PS_MGMT_CDES2_FUP_FILE_OPTIMUS_FLEX2)-1) == 0))
        {
            imagePathIndex = FBE_PS_MGMT_CDES2_OPTIMUS_FLEX2_INDEX;

            fbe_copy_memory(&imageFileConstantPortionName[0], 
                            FBE_PS_MGMT_CDES2_FUP_FILE_OPTIMUS_FLEX2,
                            sizeof(FBE_PS_MGMT_CDES2_FUP_FILE_OPTIMUS_FLEX2));
        }
        else if ((strncmp((char *)pImageFileName, FBE_PS_MGMT_CDES2_FUP_FILE_OPTIMUS_ACBEL2, sizeof(FBE_PS_MGMT_CDES2_FUP_FILE_OPTIMUS_ACBEL2)-1) == 0))
        {
            imagePathIndex = FBE_PS_MGMT_CDES2_OPTIMUS_ACBEL2_INDEX;

            fbe_copy_memory(&imageFileConstantPortionName[0], 
                            FBE_PS_MGMT_CDES2_FUP_FILE_OPTIMUS_ACBEL2,
                            sizeof(FBE_PS_MGMT_CDES2_FUP_FILE_OPTIMUS_ACBEL2));
        }
        else
        {
            fbe_base_object_customizable_trace((fbe_base_object_t*)pBaseEnv,
                                        FBE_TRACE_LEVEL_ERROR,
                                        fbe_base_env_get_fup_work_item_logheader(pWorkItem),
                                        "%s, unrecognized image file name %s.\n",
                                        __FUNCTION__, pImageFileName);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        fbe_copy_memory(&imagePathKey[0], 
                        FBE_PS_MGMT_CDES2_FUP_IMAGE_PATH_KEY,
                        sizeof(FBE_PS_MGMT_CDES2_FUP_IMAGE_PATH_KEY));
    }
    else
    {
        // error
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_base_env_fup_build_image_file_name(&imageFileName[0], 
                                                   &imageFileConstantPortionName[0], 
                                                   FBE_BASE_ENV_FUP_MAX_IMAGE_FILE_NAME_LEN, 
                                                   &pWorkItem->subenclProductId[0],
                                                   &imageFileNameLen,
                                                   pWorkItem->esesVersion);

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

} //End of function fbe_ps_mgmt_fup_get_full_image_path

/*!*******************************************************************************
 * @fn fbe_encl_ps_fup_get_manifest_file_full_path
 *********************************************************************************
 * @brief
 *  This function
 *
 * @param  pContext (INPUT)-
 * @param  pManifestFileFullPath (OUTPUT) -
 *
 * @return fbe_status_t - 
 *
 * @version
 *  12-Sep-2014 PHE -- Created.
 *******************************************************************************/
fbe_status_t fbe_ps_mgmt_fup_get_manifest_file_full_path(void * pContext, fbe_char_t * pManifestPath)
{
    fbe_base_environment_t * pBaseEnv =(fbe_base_environment_t *)pContext;
    fbe_char_t               manifestName[FBE_BASE_ENV_FUP_MAX_IMAGE_FILE_NAME_LEN] = {0};
    fbe_u32_t                manifestNameLength = 0;
    //fbe_char_t               manifestPathKey[FBE_BASE_ENV_FUP_MAX_IMAGE_PATH_KEY_LEN] = {0};
    fbe_status_t             status = FBE_STATUS_OK;

    // Build the manifest file name. In simulation mode, append _pidxxx to the manifest name. 
    status = fbe_base_env_fup_build_manifest_file_name(pBaseEnv,
                                                       &manifestName[0], 
                                                       FBE_PS_MGMT_CDES2_FUP_MANIFEST_NAME,  // name without type extension
                                                       FBE_BASE_ENV_FUP_MAX_IMAGE_FILE_NAME_LEN,
                                                       &manifestNameLength);

    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)pBaseEnv,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "FUP: get_manifest_file_full_path, failed to build manifest file name.\n");
        return status;
    }

    // Get the manifest file path amd file name
    status = fbe_base_env_fup_get_manifest_file_full_path(pBaseEnv,
                                                          FBE_PS_MGMT_CDES2_FUP_MANIFEST_PATH_KEY,//&manifestPathKey[0], 
                                                          &manifestName[0],
                                                          manifestNameLength,
                                                          pManifestPath);

    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)pBaseEnv,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "FUP: get_manifest_file_full_path, failed to get manifest file full path.\n");
        return status;
    }

    return FBE_STATUS_OK;
} //fbe_ps_mgmt_fup_get_manifest_file_full_path

/*!**************************************************************
 * @fn fbe_ps_mgmt_fup_check_env_status(void * pContext, 
 *                                      fbe_base_env_fup_work_item_t * pWorkItem)
 ****************************************************************
 * @brief
 *  This function checks the environmental status for the specified work item
 *  to see whether it allows the upgrade. If the peer SP is present and not fauled,
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
 *  15-June-2010:  PHE - Created. 
 *
 ****************************************************************/
fbe_status_t fbe_ps_mgmt_fup_check_env_status(void * pContext,
                                 fbe_base_env_fup_work_item_t * pWorkItem)
{
    fbe_ps_mgmt_t * pPsMgmt = (fbe_ps_mgmt_t *)pContext;
    fbe_base_environment_t * pBaseEnv =(fbe_base_environment_t *)pContext;
    fbe_u32_t slot = 0;
    fbe_device_physical_location_t * pLocation = NULL;
    fbe_encl_ps_info_t             * pEnclPsInfo = NULL;
    fbe_power_supply_info_t * pPeerPowerSupplyInfo = NULL;
    fbe_status_t status = FBE_STATUS_OK;

    pLocation = fbe_base_env_get_fup_work_item_location_ptr(pWorkItem),

    status = fbe_ps_mgmt_get_encl_ps_info_by_location(pPsMgmt,
                                              pLocation, 
                                                      &pEnclPsInfo);

    if(status != FBE_STATUS_OK)
    {
        /* Can not find the enclosureIndex. This is coding error. */
        fbe_base_object_customizable_trace((fbe_base_object_t*)pBaseEnv,
                                        FBE_TRACE_LEVEL_ERROR,
                                        fbe_base_env_get_fup_work_item_logheader(pWorkItem),
                                        "%s, get_encl_ps_info_by_location failed.\n",
                                        __FUNCTION__);

        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    for(slot = 0; slot < pEnclPsInfo->psCount; slot ++) 
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)pBaseEnv,
                                        FBE_TRACE_LEVEL_DEBUG_MEDIUM,
                                        fbe_base_env_get_fup_work_item_logheader(pWorkItem),
                                        "%s slot:%d localFru:%d location.slot:%d\n",
                                        __FUNCTION__,
                                           slot,
                                           pEnclPsInfo->psInfo[slot].isLocalFru,
                                           pLocation->slot);
        // isLocalFru really means supplies power to the local side. However, the power supply
        // may not be physically located on the local side.
        // The intent is to check the redundant power supply before allowing the intended power supply upgrade
        if(pEnclPsInfo->psInfo[slot].isLocalFru && slot != pLocation->slot) 
        {
            pPeerPowerSupplyInfo = &pEnclPsInfo->psInfo[slot];

            if((pPeerPowerSupplyInfo->bInserted == TRUE) &&
               (pPeerPowerSupplyInfo->generalFault == FBE_MGMT_STATUS_FALSE))
            {
                return FBE_STATUS_OK;   
            }
        }
    }
    
    return FBE_STATUS_GENERIC_FAILURE; 

} // End of function fbe_ps_mgmt_fup_check_env_status


/*!**************************************************************
 * @fn fbe_ps_mgmt_fup_get_firmware_rev(void * pContext,
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
fbe_status_t fbe_ps_mgmt_fup_get_firmware_rev(void * pContext,
                                              fbe_base_env_fup_work_item_t * pWorkItem,
                                              fbe_u8_t * pFirmwareRev)
{
    fbe_ps_mgmt_t                  * pPsMgmt = (fbe_ps_mgmt_t *)pContext;
    fbe_device_physical_location_t * pLocation = NULL;
    fbe_encl_ps_info_t             * pEnclPsInfo = NULL;
    fbe_power_supply_info_t          psInfo = {0};
    fbe_status_t                     status = FBE_STATUS_OK;
   
    pLocation = fbe_base_env_get_fup_work_item_location_ptr(pWorkItem);

    status = fbe_ps_mgmt_get_encl_ps_info_by_location(pPsMgmt, pLocation, &pEnclPsInfo);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*)pPsMgmt, 
                                    FBE_TRACE_LEVEL_INFO,
                                    FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                    "%s, PS(%d_%d_%d)Failed to get_encl_ps_info_by_location, status 0x%x.\n",
                                    __FUNCTION__,
                                    pLocation->bus,
                                    pLocation->enclosure,
                                    pLocation->slot,
                                    status);

        return status;
    } 

    psInfo.slotNumOnEncl = pLocation->slot;

    status = fbe_api_power_supply_get_power_supply_info(pEnclPsInfo->objectId, &psInfo);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*)pPsMgmt, 
                                    FBE_TRACE_LEVEL_INFO,
                                    FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                    "%s, PS(%d_%d_%d) fbe_api_power_supply_get_power_supply_info failed, status 0x%x.\n",
                                    __FUNCTION__,
                                    pLocation->bus,
                                    pLocation->enclosure,
                                    pLocation->slot,
                                    status);

        return status;
    }

    fbe_copy_memory(pFirmwareRev,
                    &psInfo.firmwareRev[0], 
                    FBE_ENCLOSURE_MAX_FIRMWARE_REV_SIZE);

    return FBE_STATUS_OK;
}


/*!**************************************************************
 * @fn fbe_ps_mgmt_get_fup_info_ptr(void * pContext,
 *                         fbe_u64_t deviceType,
 *                         fbe_device_physical_location_t * pLocation,
 *                         fbe_base_env_fup_persistent_info_t ** pPsFupInfoPtr)
 ****************************************************************
 * @brief
 *  This function gets the pointer which points to the ps firmware upgrade info.
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
fbe_status_t fbe_ps_mgmt_get_fup_info_ptr(void * pContext,
                                            fbe_u64_t deviceType,
                                            fbe_device_physical_location_t * pLocation,
                                            fbe_base_env_fup_persistent_info_t ** pFupInfoPtr)
{
    fbe_ps_mgmt_t                          * pPsMgmt = (fbe_ps_mgmt_t *)pContext;
    fbe_encl_ps_info_t                     * pEnclPsInfo = NULL;
    fbe_status_t                             status = FBE_STATUS_OK;

    *pFupInfoPtr = NULL;

    if(deviceType != FBE_DEVICE_TYPE_PS)
    {
        fbe_base_object_trace((fbe_base_object_t*)pPsMgmt, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s, unsupported deviceType 0x%llX.\n",
                              __FUNCTION__,
                              deviceType);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* get the encl ps info. */
    status = fbe_ps_mgmt_get_encl_ps_info_by_location(pPsMgmt,
                                                      pLocation, 
                                                      &pEnclPsInfo);
    
    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*)pPsMgmt,
                               FBE_TRACE_LEVEL_INFO,
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "%s, PS(%d_%d_%d) get_encl_ps_info_by_location failed, status 0x%x.\n",
                               __FUNCTION__, 
                               pLocation->bus,
                               pLocation->enclosure,
                               pLocation->slot,
                               status);

        return status;
    }

    *pFupInfoPtr = &pEnclPsInfo->psFupInfo[pLocation->slot];
    
    return FBE_STATUS_OK;
} // End of function fbe_ps_mgmt_get_fup_info_ptr

/*!**************************************************************
 * @fn fbe_ps_mgmt_get_fup_info(void * pContext,
 *                         fbe_u64_t deviceType,
 *                         fbe_device_physical_location_t * pLocation,
 *                         fbe_base_env_fup_persistent_info_t ** pPsFupInfoPtr)
 ****************************************************************
 * @brief
 *  This function gets the firmware upgrade info for a specific ps slot.
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
fbe_status_t fbe_ps_mgmt_get_fup_info(void * pContext,
                                            fbe_u64_t deviceType,
                                            fbe_device_physical_location_t * pLocation,
                                            fbe_esp_base_env_get_fup_info_t *  pGetFupInfo)
{
    fbe_ps_mgmt_t                          * pPsMgmt = (fbe_ps_mgmt_t *)pContext;
    fbe_encl_ps_info_t                     * pEnclPsInfo = NULL;
    fbe_status_t                             status = FBE_STATUS_OK;
    fbe_base_env_fup_work_item_t      * pWorkItem = NULL;

    if(deviceType != FBE_DEVICE_TYPE_PS)
    {
        fbe_base_object_trace((fbe_base_object_t*)pPsMgmt, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s, unsupported deviceType 0x%llX.\n",
                              __FUNCTION__,
                              deviceType);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* get the encl ps info. */
    status = fbe_ps_mgmt_get_encl_ps_info_by_location(pPsMgmt,
                                                      pLocation, 
                                                      &pEnclPsInfo);
    
    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*)pPsMgmt,
                               FBE_TRACE_LEVEL_INFO,
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "%s, PS(%d_%d_%d) get_encl_ps_info_by_location failed, status 0x%x.\n",
                               __FUNCTION__, 
                               pLocation->bus,
                               pLocation->enclosure,
                               pLocation->slot,
                               status);

        return status;
    }

    pGetFupInfo->programmableCount = FBE_PS_MGMT_MAX_PROGRAMMABLE_COUNT_PER_PS_SLOT;

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
    pGetFupInfo->fupInfo[0].completionStatus = pEnclPsInfo->psFupInfo[pLocation->slot].completionStatus;
    fbe_copy_memory(&pGetFupInfo->fupInfo[0].imageRev[0],
                        &pEnclPsInfo->psFupInfo[pLocation->slot].imageRev[0],
                        FBE_ENCLOSURE_MAX_FIRMWARE_REV_SIZE + 1);
    fbe_copy_memory(&pGetFupInfo->fupInfo[0].currentFirmwareRev[0],
                        &pEnclPsInfo->psFupInfo[pLocation->slot].currentFirmwareRev[0],
                        FBE_ENCLOSURE_MAX_FIRMWARE_REV_SIZE + 1);
    fbe_copy_memory(&pGetFupInfo->fupInfo[0].preFirmwareRev[0],
                        &pEnclPsInfo->psFupInfo[pLocation->slot].preFirmwareRev[0],
                        FBE_ENCLOSURE_MAX_FIRMWARE_REV_SIZE + 1);
    
    return FBE_STATUS_OK;
} // End of function fbe_ps_mgmt_get_fup_info

/*!***************************************************************
 * fbe_ps_mgmt_fup_resume_upgrade()
 ****************************************************************
 * @brief
 *  This function resumes the ugrade for the devices which were aborted.
 *
 * @param pContext -
 * 
 * @return fbe_status_t
 *
 * @author
 *  01-Sept-2010: PHE - Created. 
 *
 ****************************************************************/
fbe_status_t fbe_ps_mgmt_fup_resume_upgrade(void * pContext)
{
    fbe_ps_mgmt_t                          * pPsMgmt = (fbe_ps_mgmt_t *)pContext;
    fbe_encl_ps_info_t                     * pEnclPsInfo = NULL;
    fbe_u32_t                                enclIndex = 0;
    fbe_u32_t                                slot = 0;
    fbe_device_physical_location_t           location = {0};
    fbe_status_t                             status = FBE_STATUS_OK;

    fbe_base_object_trace((fbe_base_object_t *)pPsMgmt,
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "FUP: Resume upgrade for PS.\n");

    for(enclIndex = 0; enclIndex < FBE_ESP_MAX_ENCL_COUNT; enclIndex++)
    {
        pEnclPsInfo = &(pPsMgmt->psInfo->enclPsInfo[enclIndex]);

        if(pEnclPsInfo->objectId != FBE_OBJECT_ID_INVALID)
        {
            for(slot = 0; slot < pEnclPsInfo->psCount; slot ++)
            {
                if(pEnclPsInfo->psFupInfo[slot].completionStatus == 
                         FBE_BASE_ENV_FUP_COMPLETION_STATUS_ABORTED)
                { 
                    location.bus = pEnclPsInfo->location.bus;
                    location.enclosure = pEnclPsInfo->location.enclosure;
                    location.slot = slot;
                    status = fbe_ps_mgmt_fup_initiate_upgrade(pPsMgmt, 
                                                      FBE_DEVICE_TYPE_PS, 
                                                      &location,
                                                      pEnclPsInfo->psFupInfo[slot].forceFlags,
                                                      0);  //upgradeRetryCount
                    if(status != FBE_STATUS_OK)
                    {
                        fbe_base_object_trace((fbe_base_object_t *)pPsMgmt,
                                              FBE_TRACE_LEVEL_WARNING,
                                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                              "FUP:PS(%d_%d_%d), %s, Failed to initiate upgrade, status 0x%X.\n",
                                              location.bus,
                                              location.enclosure,
                                              location.slot,
                                              __FUNCTION__,
                                              status);
                    }
                }
            }
        }
    }
    
    return FBE_STATUS_OK;
} /* End of function fbe_ps_mgmt_fup_resume_upgrade */

/*!**************************************************************
 * @fn fbe_ps_mgmt_fup_copy_fw_rev_to_resume_prom(fbe_ps_mgmt_t *pPsMgmt,
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
fbe_status_t fbe_ps_mgmt_fup_copy_fw_rev_to_resume_prom(fbe_ps_mgmt_t *pPsMgmt,
                                                        fbe_u64_t deviceType,
                                                        fbe_device_physical_location_t *pLocation)
{
    fbe_char_t                             logHeader[FBE_BASE_ENV_FUP_MAX_LOG_HEADER_SIZE + 1] = {0};
    fbe_status_t                           status = FBE_STATUS_OK;
    fbe_base_env_resume_prom_info_t      * pResumePromInfo = NULL;
    fbe_encl_ps_info_t                   * pEnclPsInfo = NULL;
    fbe_power_supply_info_t              * pPsInfo = NULL;
    fbe_u32_t                              revIndex = 0;
    fbe_u8_t                               fwRevForResume[RESUME_PROM_PROG_REV_SIZE] = {0};

    fbe_ps_mgmt_fup_generate_logheader(pLocation, &logHeader[0], FBE_BASE_ENV_FUP_MAX_LOG_HEADER_SIZE);

    status = fbe_ps_mgmt_get_resume_prom_info_ptr(pPsMgmt, 
                                                  deviceType, 
                                                  pLocation, 
                                                  &pResumePromInfo);

    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)pPsMgmt,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s, %s, get_resume_prom_info_ptr failed.\n",
                              &logHeader[0], __FUNCTION__);

        return status;
    }

    status = fbe_ps_mgmt_get_encl_ps_info_by_location(pPsMgmt, pLocation, &pEnclPsInfo);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)pPsMgmt,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s, %s, get_encl_ps_info_by_location failed.\n",
                              &logHeader[0], __FUNCTION__);

        return status;
    }

    pPsInfo = &pEnclPsInfo->psInfo[pLocation->slot];
    
    //The value of num_prog[0] is depend on ps programables number 
    if(pPsInfo->uniqueId == DAE_AC_1080)
    {
        pResumePromInfo->resume_prom_data.num_prog[0] = 0;
    
        for(revIndex = 0; revIndex < FBE_ENCLOSURE_MAX_REV_COUNT_PER_PS; revIndex ++) 
        {
            if(pPsInfo->psRevInfo[revIndex].firmwareRevValid)
            {
                pResumePromInfo->resume_prom_data.num_prog[0]++;

                fbe_base_env_fup_convert_fw_rev_for_rp((fbe_base_environment_t *)pPsMgmt,
                                                   &fwRevForResume[0],
                                                   &pPsInfo->psRevInfo[revIndex].firmwareRev[0]);

                fbe_copy_memory(pResumePromInfo->resume_prom_data.prog_details[revIndex].prog_rev,
                        &fwRevForResume[0],
                        RESUME_PROM_PROG_REV_SIZE);
            }
        }
    }
    else
    {
        pResumePromInfo->resume_prom_data.num_prog[0] = 1;

        fbe_base_env_fup_convert_fw_rev_for_rp((fbe_base_environment_t *)pPsMgmt,
                                                   &fwRevForResume[0],
                                                   &pPsInfo->firmwareRev[0]);

        fbe_copy_memory(pResumePromInfo->resume_prom_data.prog_details[0].prog_rev,
                    &fwRevForResume[0],
                    RESUME_PROM_PROG_REV_SIZE);
    }
    
    return status;
}

/*!**************************************************************
 * @fn fbe_ps_mgmt_fup_refresh_device_status(void * pContext,
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
fbe_status_t fbe_ps_mgmt_fup_refresh_device_status(void * pContext,
                                                   fbe_base_env_fup_work_item_t * pWorkItem)
{
    fbe_ps_mgmt_t                       * pPsMgmt = (fbe_ps_mgmt_t *)pContext;
    fbe_encl_ps_info_t                  * pEnclPsInfo = NULL;
    fbe_status_t                          status = FBE_STATUS_OK;
    fbe_device_physical_location_t      * pLocation = NULL;

    if(pWorkItem->deviceType != FBE_DEVICE_TYPE_PS)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)pPsMgmt,
                        FBE_TRACE_LEVEL_ERROR,
                        fbe_base_env_get_fup_work_item_logheader(pWorkItem),
                        "%s unsupported deviceType 0x%llX.\n",   
                        __FUNCTION__, pWorkItem->deviceType);

        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    if(pWorkItem->workState >= FBE_BASE_ENV_FUP_WORK_STATE_ACTIVATE_IMAGE_SENT)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)pPsMgmt,
                        FBE_TRACE_LEVEL_INFO,
                        fbe_base_env_get_fup_work_item_logheader(pWorkItem),
                        "Refresh device status...\n");
    }
    else
    {
        return FBE_STATUS_OK;
    }
    
    pLocation = &pWorkItem->location;
    status = fbe_ps_mgmt_get_encl_ps_info_by_location(pPsMgmt, pLocation, &pEnclPsInfo);
  
    if (status != FBE_STATUS_OK) 
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)pPsMgmt,
                        FBE_TRACE_LEVEL_INFO,
                        fbe_base_env_get_fup_work_item_logheader(pWorkItem),
                        "Refresh device status, get_encl_ps_info_by_location failed, status 0x%X.\n",
                        status);

        /* 
         * The enclosure which contains the PS might already be removed.
         * So we just return OK here. 
         */ 
        return FBE_STATUS_OK;
    }

    status = fbe_ps_mgmt_processPsStatus(pPsMgmt, pEnclPsInfo->objectId, pLocation);

    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)pPsMgmt,
                        FBE_TRACE_LEVEL_INFO,
                        fbe_base_env_get_fup_work_item_logheader(pWorkItem),
                        "Refresh device status, processPsStatus failed, status 0x%X.\n",
                        status);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    return FBE_STATUS_OK;
}

/*!***************************************************************
 * fbe_ps_fup_init_manifest_info(fbe_base_environment_t * pBaseEnv)
 ****************************************************************
 * @brief
 *  The product id field in fup_manifest_info is initialized to 
 *  spaces. For power supplies we need to clear the product id 
 *  field where the manifest info will go. Otherwise the strings 
 *  won't match.

 * 
 * @param pBaseEnv.
 *
 * @return fbe_status_t
 *
 * @author
 *  06-Oct-2014 PHE - Created. 
 ****************************************************************/
fbe_status_t fbe_ps_fup_init_manifest_info(fbe_base_environment_t *pBaseEnv)
{
    fbe_u32_t                      subEnclIndex = 0;
    fbe_base_env_fup_manifest_info_t * pManifestInfo = NULL;

    // clear the product id field
    for(subEnclIndex = 0; subEnclIndex < FBE_BASE_ENV_FUP_MAX_SUBENCL_COUNT_IN_MANIFEST; subEnclIndex ++) 
    {
        pManifestInfo = fbe_base_env_fup_get_manifest_info_ptr(pBaseEnv, subEnclIndex);
        fbe_zero_memory(&pManifestInfo->subenclProductId[0], FBE_ENCLOSURE_MAX_SUBENCL_PRODUCT_ID_SIZE);
    }
return FBE_STATUS_OK;
}

/*!**************************************************************
 * @fn fbe_ps_mgmt_fup_create_and_add_work_item_from_manifest(fbe_encl_mgmt_t * pPsMgmt,                                           
 *                                         fbe_device_physical_location_t * pLocation,
 *                                         fbe_power_supply_info_t * pPsInfo,
 *                                         fbe_u32_t forceFlags,
 *                                         fbe_char_t * pLogHeader,
 *                                         fbe_u32_t interDeviceDelayInSec,
 *                                         fbe_u32_t upgradeRetryCount)
 ****************************************************************
 * @brief
 *  This function creates the work items for the LCC upgrade based on the manifest info.
 *
 * @param pPsMgmt - The pointer to the fbe_base_environment_t.
 * @param pLocation - The pointer to the physical location that the work item needs to be added for.
 * @param deviceType - 
 * @param pPsInfo -
 * @param forceFlags -
 * @param pLogHeader -
 * @param interDeviceDelayInSec -
 * @param upgradeRetryCount - 
 *
 * @return fbe_status_t
 * 
 * @version
 *  07-Aug-2014:  PHE - Created. 
 ****************************************************************/
static fbe_status_t
fbe_ps_mgmt_fup_create_and_add_work_item_from_manifest(fbe_ps_mgmt_t * pPsMgmt,
                                          fbe_device_physical_location_t * pLocation,
                                          fbe_power_supply_info_t * pPsInfo,
                                          fbe_u32_t forceFlags,
                                          fbe_char_t * pLogHeader,
                                          fbe_u32_t interDeviceDelayInSec,
                                          fbe_u32_t upgradeRetryCount)
{
    fbe_base_environment_t  *   pBaseEnv = (fbe_base_environment_t *)pPsMgmt;
    fbe_u32_t                   subEnclosure = 0;
    fbe_u32_t                   imageIndex = 0;
    fbe_status_t                status = FBE_STATUS_OK;
    fbe_enclosure_fw_target_t   firmwareTarget = FBE_FW_TARGET_INVALID;
    fbe_char_t                  logHeaderWithFwTarget[FBE_BASE_ENV_FUP_MAX_LOG_HEADER_SIZE + 1] = {0};
    fbe_bool_t                  manifestInfoAvail = FBE_FALSE;
    fbe_bool_t                  productIdMatch = FBE_FALSE;
    fbe_base_env_fup_manifest_info_t * pManifestInfo = NULL;

    // check if the manifest has been read
    manifestInfoAvail = fbe_base_env_fup_get_manifest_info_avail(pBaseEnv);
    if((!manifestInfoAvail) || 
       (forceFlags & FBE_BASE_ENV_FUP_FORCE_FLAG_READ_MANIFEST_FILE))
    {
        status = fbe_base_env_fup_init_manifest_info(pBaseEnv);
        // clear the ps subencl id, the base env inits it to spaces
        status = fbe_ps_fup_init_manifest_info(pBaseEnv);
        // parse the manifest
        status = fbe_base_env_fup_read_and_parse_manifest_file(pBaseEnv);
        if (status != FBE_STATUS_OK) 
        {
            fbe_base_object_trace((fbe_base_object_t *)pBaseEnv, 
                                  FBE_TRACE_LEVEL_WARNING,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "FUP: failed to read and parse manifest file, status: 0x%X.\n",
                                  status);

            return FBE_STATUS_GENERIC_FAILURE;
        }
    }

    // for each subenclosure look for a match in the manifest
    for(subEnclosure = 0; subEnclosure < FBE_BASE_ENV_FUP_MAX_SUBENCL_COUNT_IN_MANIFEST; subEnclosure++)
    {
        pManifestInfo = fbe_base_env_fup_get_manifest_info_ptr(pBaseEnv, subEnclosure);
        if(strncmp(&pManifestInfo->subenclProductId[0], 
                   &pPsInfo->subenclProductId[0],
                   FBE_ENCLOSURE_MAX_SUBENCL_PRODUCT_ID_SIZE) == 0)
        {
            productIdMatch = FBE_TRUE;
            // It is single image per ps subenclosure, so the loop's not really needed but makes it flexible
            for(imageIndex = 0; imageIndex < pManifestInfo->imageFileCount; imageIndex ++)
            {
                productIdMatch = FBE_TRUE;
                firmwareTarget = fbe_base_env_fup_get_manifest_fw_target(pBaseEnv, 
                                                                         subEnclosure,
                                                                         imageIndex);

                fbe_sprintf(&logHeaderWithFwTarget[0], 
                            (FBE_BASE_ENV_FUP_MAX_LOG_HEADER_SIZE + 1), 
                            "%s %s", 
                            pLogHeader,
                            fbe_base_env_decode_firmware_target(firmwareTarget));

                status = fbe_base_env_fup_create_and_add_work_item(pBaseEnv,
                                                                   pLocation,
                                                                   FBE_DEVICE_TYPE_PS,
                                                                   firmwareTarget,
                                                                   &pPsInfo->firmwareRev[0], //pPsFirmwareRev,
                                                                   &pPsInfo->subenclProductId[0],
                                                                   pPsInfo->uniqueId,
                                                                   forceFlags,
                                                                   &logHeaderWithFwTarget[0],
                                                                   interDeviceDelayInSec,
                                                                   upgradeRetryCount,
                                                                   ESES_REVISION_2_0);
    
                if(status != FBE_STATUS_OK)
                {
                    fbe_base_object_trace((fbe_base_object_t *)pBaseEnv,
                                          FBE_TRACE_LEVEL_WARNING,
                                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                          "%s, fwTarget %d, Failed to create_and_add_work_item, status 0x%X.\n",
                                          pLogHeader, 
                                          firmwareTarget,
                                          status);
                }
            } // for subenclosure image
            break; // done 
        }// End of - if(pManifestInfo->subenclUniqueId == pPsInfo->uniqueId)
    }// End of - for(subEnclIndex = 0;

    if(!productIdMatch)
    {
        fbe_base_object_trace((fbe_base_object_t *)pBaseEnv,
                                          FBE_TRACE_LEVEL_WARNING,
                                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                          "%s, fwTarget %d, ProductId %s not found in manifest.\n",
                                          pLogHeader, 
                                          firmwareTarget,
                                          &pPsInfo->subenclProductId[0]);
    }
    return status;
} //fbe_ps_mgmt_fup_create_and_add_work_item_from_manifest


/*!***************************************************************
 * @fn fbe_ps_mgmt_fup_new_contact_init_upgrade(fbe_ps_mgmt_t *pPsMgmt)
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
 * @param pPsMgmt -pointer to fbe_ps_mgmt_t
 *      
 * @return fbe_status_t
 *
 * @author
 *  211-Dec-2014: PHE - Created. 
 *
 ****************************************************************/
fbe_status_t fbe_ps_mgmt_fup_new_contact_init_upgrade(fbe_ps_mgmt_t *pPsMgmt)
{
    fbe_encl_ps_info_t                     * pEnclPsInfo = NULL;
    fbe_u32_t                                enclIndex = 0;
    fbe_u32_t                                slot = 0;
    fbe_device_physical_location_t           location = {0};
    fbe_status_t                             status = FBE_STATUS_OK;

    // Peer contact back, update PS FW. 
    fbe_base_object_trace((fbe_base_object_t *)pPsMgmt,
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s peer SP contact is back.\n",
                                  __FUNCTION__);

    for(enclIndex = 0; enclIndex < FBE_ESP_MAX_ENCL_COUNT; enclIndex++)
    {
        pEnclPsInfo = &(pPsMgmt->psInfo->enclPsInfo[enclIndex]);

        if(pEnclPsInfo->objectId != FBE_OBJECT_ID_INVALID)
        {
            for(slot = 0; slot < pEnclPsInfo->psCount; slot ++)
            {
                if(pEnclPsInfo->psFupInfo[slot].completionStatus == 
                         FBE_BASE_ENV_FUP_COMPLETION_STATUS_FAIL_NO_PEER_PERMISSION)
                { 
                    location.bus = pEnclPsInfo->location.bus;
                    location.enclosure = pEnclPsInfo->location.enclosure;
                    location.slot = slot;
                    status = fbe_ps_mgmt_fup_initiate_upgrade(pPsMgmt, 
                                                      FBE_DEVICE_TYPE_PS, 
                                                      &location,
                                                      pEnclPsInfo->psFupInfo[slot].forceFlags,
                                                      0);  //upgradeRetryCount
                    if(status != FBE_STATUS_OK)
                    {
                        fbe_base_object_trace((fbe_base_object_t *)pPsMgmt,
                                              FBE_TRACE_LEVEL_WARNING,
                                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                              "FUP:PS(%d_%d_%d), %s, Failed to initiate upgrade, status 0x%X.\n",
                                              location.bus,
                                              location.enclosure,
                                              location.slot,
                                              __FUNCTION__,
                                              status);
                    }
                }
            }
        }
    }

    return FBE_STATUS_OK;
} //fbe_ps_mgmt_fup_new_contact_init_upgrade


//End of file fbe_ps_mgmt_fup.c
