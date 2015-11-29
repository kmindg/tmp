/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_encl_mgmt_fup.c
 ***************************************************************************
 * @brief
 *  This file contains the implemention for enclosure firmware upgrade functionality.
 *
 * @version
 *   10-Aug-2010:  PHE - Created.
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_memory.h"
#include "fbe/fbe_api_enclosure_interface.h"
#include "fbe_base_object.h"
#include "fbe_encl_mgmt_private.h"
#include "fbe_encl_mgmt.h"
#include "fbe_base_environment_debug.h"

static fbe_status_t fbe_encl_mgmt_fup_lcc_qualified(fbe_encl_mgmt_t * pEnclMgmt,
                                                    fbe_device_physical_location_t * pLocation,
                                                    fbe_lcc_info_t * pLccInfo, 
                                                    fbe_bool_t * pUpgradeQualified);

static fbe_status_t fbe_encl_mgmt_fup_policy_allowed(fbe_encl_mgmt_t * pEnclMgmt, 
                                        fbe_device_physical_location_t * pLocation,
                                        fbe_bool_t * pPolicyAllowed);

static void fbe_encl_mgmt_fup_restart_env_failed_fup(fbe_encl_mgmt_t * pEnclMgmt,
                                                     fbe_device_physical_location_t * pLocation,
                                                     fbe_lcc_info_t * pLccInfo);
static fbe_status_t
fbe_encl_mgmt_fup_create_and_add_work_item_from_manifest(fbe_encl_mgmt_t * pEnclMgmt,
                                          fbe_device_physical_location_t * pLocation,
                                          fbe_u64_t deviceType,
                                          fbe_lcc_info_t * pLccInfo,
                                          fbe_u32_t forceFlags,
                                          fbe_char_t * pLogHeader,
                                          fbe_u32_t interDeviceDelayInSec,
                                          fbe_u32_t upgradeRetryCount);

/*!**************************************************************
 * @fn fbe_encl_mgmt_fup_generate_logheader(fbe_device_physical_location_t * pLocation, 
 *                               fbe_u8_t * logHeader)
 ****************************************************************
 * @brief
 *  This function sets the log header for the enclosure.
 *
 * @param pLocation - The pointer to the physical location.
 * @param pLogHeader(output) - The pointer to the log header.
 * @param logHeaderSize -
 *
 * @return fbe_status_t
 *
 * @version
 *  10-Aug-2010:  PHE - Created. 
 *
 ****************************************************************/
fbe_status_t fbe_encl_mgmt_fup_generate_logheader(fbe_device_physical_location_t * pLocation, 
                                                         fbe_u8_t * pLogHeader, 
                                                         fbe_u32_t logHeaderSize)
{
    fbe_zero_memory(pLogHeader, logHeaderSize);

    if(pLocation->componentId == 0) 
    {
        csx_p_snprintf(pLogHeader, logHeaderSize, "FUP:LCC(%d_%d_%d)",
                           pLocation->bus, pLocation->enclosure, pLocation->slot);
    }
    else
    {
        csx_p_snprintf(pLogHeader, logHeaderSize, "FUP:ENCL(%d_%d.%d) LCC%d",
                           pLocation->bus, pLocation->enclosure, pLocation->componentId, pLocation->slot);
    }

    return FBE_STATUS_OK;
}


/*!**************************************************************
 * @fn fbe_encl_mgmt_fup_handle_lcc_status_change(fbe_encl_mgmt_t * pEnclMgmt,
 *                                                fbe_device_physical_location_t * pLocation,
 *                                                fbe_lcc_info_t * pNewLccInfo,
 *                                                fbe_lcc_info_t * pOldLccInfo)
 ****************************************************************
 * @brief
 *  This function handles the LCC status change for firmware upgrade. 
 * 
 * @param pPsMgmt -
 * @param pLocation - The location of the PS.
 * @param pNewPsInfo - The new power supply info.
 * @param pOldPsInfo - The old power supply info.
 * 
 * @return fbe_status_t 
 *
 * @version
 *  30-Aug-2010:  PHE - Created. 
 *
 ****************************************************************/
fbe_status_t fbe_encl_mgmt_fup_handle_lcc_status_change(fbe_encl_mgmt_t * pEnclMgmt,
                                                 fbe_device_physical_location_t * pLocation,
                                                 fbe_lcc_info_t * pNewLccInfo,
                                                 fbe_lcc_info_t * pOldLccInfo)
{
    fbe_bool_t                     newQualified;
    fbe_bool_t                     oldQualified;
    fbe_status_t                   status = FBE_STATUS_OK;
    fbe_char_t                     logHeader[FBE_BASE_ENV_FUP_MAX_LOG_HEADER_SIZE + 1] = {0};
    fbe_bool_t                     policyAllowUpgrade = FBE_FALSE;

    /* Set log header for tracing purpose. */
    fbe_encl_mgmt_fup_generate_logheader(pLocation, &logHeader[0], FBE_BASE_ENV_FUP_MAX_LOG_HEADER_SIZE);

    fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt, 
                              FBE_TRACE_LEVEL_DEBUG_LOW,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s %s\n",
                              &logHeader[0],
                              __FUNCTION__);
    
    if((!pNewLccInfo->inserted) && (pOldLccInfo->inserted))
    {
        fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s, Device removed.\n",
                              &logHeader[0]);

       
        /* The LCC was removed. */
        status = fbe_base_env_fup_handle_device_removal((fbe_base_environment_t *)pEnclMgmt, 
                                                        FBE_DEVICE_TYPE_LCC, 
                                                        pLocation);
        if(status != FBE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt,
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
        fbe_encl_mgmt_fup_restart_env_failed_fup(pEnclMgmt,
                                                 pLocation,
                                                 pNewLccInfo);
        fbe_encl_mgmt_fup_policy_allowed(pEnclMgmt, pLocation, &policyAllowUpgrade);
        if(!policyAllowUpgrade)
        {
            fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt,
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s, %s, policy does not allow upgrade.\n",
                                  &logHeader[0],
                                  __FUNCTION__);
    
            return FBE_STATUS_OK;
        }

        fbe_encl_mgmt_fup_lcc_qualified(pEnclMgmt, pLocation, pNewLccInfo, &newQualified);
        fbe_encl_mgmt_fup_lcc_qualified(pEnclMgmt, pLocation, pOldLccInfo, &oldQualified);

        if(newQualified && !oldQualified)
        {
            fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt,
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s, %s, became qualified for upgrade.\n",
                                  &logHeader[0],
                                  __FUNCTION__);

            status = fbe_encl_mgmt_fup_initiate_upgrade(pEnclMgmt, 
                                                      FBE_DEVICE_TYPE_LCC, 
                                                      pLocation,
                                                      FBE_BASE_ENV_FUP_FORCE_FLAG_NONE,
                                                      0); //upgradeRetryCount

            if(status != FBE_STATUS_OK)
            {
                fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt,
                                      FBE_TRACE_LEVEL_WARNING,
                                      FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                      "%s, %s,Failed Initiate,status 0x%X\n",
                                      &logHeader[0],
                                      __FUNCTION__,
                                      status);
        
                return status;
            }
        }
    }

    return status;
}

/*!**************************************************************
 * @fn fbe_encl_mgmt_fup_restart_env_failed_fup(fbe_encl_mgmt_t * pEnclMgmt,
                                                fbe_device_physical_location_t * pLocation,
                                                fbe_lcc_info_t * pLccInfo)
 ****************************************************************
 * @brief
 *  When peer lcc is faulted it will prevent local lcc from getting
 *  upgraded. When the peer lcc fault clears we need to come through
 *  here and check if the failed upgrade (on the local lcc) needs to
 *  be restarted.
 * 
 * @param pEnclMgmt - encl mgmt object
 * @param pLccInfo - The lcc info for the status change
 * @param pLocation - bus, encl, slot for the lcc
 * 
 * @return void
 *
 * @version
 *  3-Mar-2015:  GB - Created. 
 ****************************************************************/
static void fbe_encl_mgmt_fup_restart_env_failed_fup(fbe_encl_mgmt_t * pEnclMgmt,
                                                     fbe_device_physical_location_t * pLocation,
                                                     fbe_lcc_info_t * pLccInfo)
{
    fbe_encl_info_t         * pEnclInfo = NULL;
    fbe_u8_t                savedSlot;
    fbe_u8_t                localLccSlot;
    fbe_status_t            status = FBE_STATUS_OK;
    fbe_char_t              logHeader[FBE_BASE_ENV_FUP_MAX_LOG_HEADER_SIZE + 1] = {0};

    // Set log header for tracing purpose
    fbe_encl_mgmt_fup_generate_logheader(pLocation, &logHeader[0], FBE_BASE_ENV_FUP_MAX_LOG_HEADER_SIZE);
    fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                              "%s entry\n",
                              __FUNCTION__);

    // if this is the local side, no need to check anything, only
    // need to check when the peer side info has changed
    if(pLccInfo->isLocal)
    {
        return;
    }

    status = fbe_encl_mgmt_get_encl_info_by_location(pEnclMgmt,
                                                     pLocation, 
                                                     &pEnclInfo);

    if(status != FBE_STATUS_OK)
    {
        // bug - Cannot find the enclosureIndex. This is coding error.
        fbe_base_object_trace((fbe_base_object_t*)pEnclMgmt,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s, get_encl_info_by_location failed.\n",
                              __FUNCTION__);
        return;
    }

    // this is peer info, if peer is not faulted check if local side failed for bad env status
    if((pLccInfo->inserted) &&
       (pLccInfo->faulted == FBE_MGMT_STATUS_FALSE))
    {
        // the local lcc slot will be the complement of the peer slot
        localLccSlot = (pLocation->slot == 0) ? 1 : 0;
        if(pEnclInfo->lccFupInfo[localLccSlot].completionStatus == FBE_BASE_ENV_FUP_COMPLETION_STATUS_FAIL_BAD_ENV_STATUS)
        {
            savedSlot = localLccSlot;
            pLocation->slot = localLccSlot;
            fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt,
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  " Restart the Failed Bad env Status FUP:LCC(%d_%d_%d)\n",
                                  pLocation->bus,
                                  pLocation->enclosure,
                                  pLocation->slot);

            // restart the upgrade
            status = fbe_encl_mgmt_fup_initiate_upgrade(pEnclMgmt, 
                                                        FBE_DEVICE_TYPE_LCC, 
                                                        pLocation,
                                                        FBE_BASE_ENV_FUP_FORCE_FLAG_NONE,
                                                        0);  //upgradeRetryCount
            if(status != FBE_STATUS_OK)
            {
                fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt,
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

    return;

} //fbe_encl_mgmt_fup_restart_env_failed_fup

/*!**************************************************************
 * @fn fbe_encl_mgmt_fup_policy_allowed(fbe_encl_mgmt_t * pEnclMgmt,
 *                                      fbe_device_physical_location_t * pLocation,
 *                                      fbe_bool_t * pPolicyAllowed)
 ****************************************************************
 * @brief
 *  This function checks whether the Encl is qualified to be upgraded or not.
 *
 * @param pEnclMgmt - The current encl info.
 * @param pLocation -
 * @param pPolicyAllowed(OUTPUT) -
 * 
 * @return fbe_status_t - always return FBE_STATUS_OK;
 *
 * @version
 *  12-Jul-2010:  PHE - Created. 
 *
 ****************************************************************/
static fbe_status_t 
fbe_encl_mgmt_fup_policy_allowed(fbe_encl_mgmt_t * pEnclMgmt, 
                                 fbe_device_physical_location_t * pLocation,
                                 fbe_bool_t * pPolicyAllowed)
{
    fbe_base_environment_t     * pBaseEnv = (fbe_base_environment_t *)pEnclMgmt; 
    fbe_char_t                   logHeader[FBE_BASE_ENV_FUP_MAX_LOG_HEADER_SIZE + 1] = {0};

    /* Set log header for tracing purpose. */
    fbe_encl_mgmt_fup_generate_logheader(pLocation, &logHeader[0], FBE_BASE_ENV_FUP_MAX_LOG_HEADER_SIZE);

    *pPolicyAllowed = FALSE;

    // for Single SP system, we allow fw upgrade for peer side
    if((pLocation->bus != FBE_XPE_PSEUDO_BUS_NUM) &&
       (pLocation->enclosure != FBE_XPE_PSEUDO_ENCL_NUM) &&
       ((pBaseEnv->spid == pLocation->slot%FBE_ESP_MAX_LCCS_PER_SUBENCL) ||
       (pEnclMgmt->base_environment.isSingleSpSystem == TRUE)))
    {
        *pPolicyAllowed = TRUE;
    }
    else
    {
        fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s, spid:%d Policy Check Fail\n",
                              &logHeader[0],
                              pBaseEnv->spid);
    }

    return FBE_STATUS_OK;
}

/*!**************************************************************
 * @fn fbe_encl_mgmt_fup_lcc_priority_check(void *pContext,
 *                                          fbe_base_env_fup_work_item_t *pWorkItem,
 *                                          fbe_bool_t *pWait)
 ****************************************************************
 * @brief
 *  Check to see if EE expanders are done upgrading before
 *  allowing ICM upgrade.
 *
 * @param pContext - The current encl info.
 * @param pWorkItem -
 * @param pWait(OUTPUT) -
 * 
 * @return fbe_status_t - always return FBE_STATUS_OK;
 *
 * @version
 *  01-Dec-2011: Greg - Created.
 *  18-Jan-2012: PHE - Updated to check the subenclosure LCCs on the same side as ICM.
 ****************************************************************/
fbe_status_t fbe_encl_mgmt_fup_lcc_priority_check(void *pContext,
                                                  fbe_base_env_fup_work_item_t *pWorkItem,
                                                  fbe_bool_t *pWait)
{
    fbe_encl_mgmt_t                     *pEnclMgmt = (fbe_encl_mgmt_t *)pContext;
    fbe_u8_t                            index = 0;
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_object_id_t                     subEnclObjectId;
    fbe_encl_info_t                     *pEnclInfo = NULL;
    fbe_base_env_fup_persistent_info_t  *pFupInfo = NULL;
    fbe_char_t                           logHeader[FBE_BASE_ENV_FUP_MAX_LOG_HEADER_SIZE + 1] = {0};


    // no check needed if not lcc
    // no check needed if this is ee (cid != 0)
    if((pWorkItem->deviceType != FBE_DEVICE_TYPE_LCC) || (pWorkItem->location.componentId != 0))
    {
        *pWait = FALSE;
        return FBE_STATUS_OK;
    }

    /* Set log header for tracing purpose. */
    fbe_encl_mgmt_fup_generate_logheader(&pWorkItem->location, &logHeader[0], FBE_BASE_ENV_FUP_MAX_LOG_HEADER_SIZE);

    status = fbe_encl_mgmt_get_encl_info_by_location(pEnclMgmt, &pWorkItem->location, &pEnclInfo);

    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s, get_encl_info_by_location failed, status 0x%x.\n", 
                              __FUNCTION__, 
                              status);
        *pWait = TRUE;
        return(status);
    }

    // get the subenclosure count and check if ee upgrades are done
    for(index = 0; index < pEnclInfo->subEnclCount; index ++)
    {
        // for each subenclosure 
        subEnclObjectId = pEnclInfo->subEnclInfo[index].objectId;
        if(subEnclObjectId == FBE_OBJECT_ID_INVALID) 
        {
            fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt, 
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "Encl(%d_%d) InvObj Id (not added yet) for SubEncl[%d]\n",
                                  pWorkItem->location.bus, 
                                  pWorkItem->location.enclosure,
                                  index);
            *pWait = TRUE;
            return FBE_STATUS_OK;
        }

        pFupInfo = &pEnclInfo->subEnclInfo[index].lccFupInfo[pWorkItem->location.slot];
      
        if(pFupInfo->completionStatus <= FBE_BASE_ENV_FUP_COMPLETION_STATUS_IN_PROGRESS)
        {
            *pWait = TRUE;
            if(status == FBE_STATUS_OK)
            {
                fbe_base_object_customizable_trace((fbe_base_object_t*)pEnclMgmt,
                                FBE_TRACE_LEVEL_INFO,
                                fbe_base_env_get_fup_work_item_logheader(pWorkItem),
                                "%s, EE FUP incomplete, delay ICM FUP\n",   
                                __FUNCTION__);
            }
            break;
        }
    }
    return(status);
} //fbe_encl_mgmt_fup_lcc_priority_check
    

/*!**************************************************************
 * @fn fbe_encl_mgmt_fup_lcc_qualified(fbe_encl_mgmt_t * pEnclMgmt,
 *                                     fbe_device_physical_location_t * pLocation,
 *                                     fbe_lcc_info_t * pLccInfo, 
 *                                     fbe_bool_t * pUpgradeQualified)
 ****************************************************************
 * @brief
 *  This function checks whether the LCC is qualified to the upgrade or not.
 * 
 * @param pEnclMgmt -
 * @param pLocation -
 * @param pLccInfo - The LCC info from the physical package.
 * @param pUpgradeQualified(OUTPUT) - whether the power supply
 *                        is qualified to do the upgrade or not.
 * 
 * @return fbe_status_t - always return FBE_STATUS_OK;
 *
 * @version
 *  30-Aug-2010:  PHE - Created. 
 *
 ****************************************************************/
static fbe_status_t fbe_encl_mgmt_fup_lcc_qualified(fbe_encl_mgmt_t * pEnclMgmt,
                                                    fbe_device_physical_location_t * pLocation,
                                                    fbe_lcc_info_t * pLccInfo, 
                                                    fbe_bool_t * pUpgradeQualified)
{
    fbe_char_t                     logHeader[FBE_BASE_ENV_FUP_MAX_LOG_HEADER_SIZE + 1] = {0};

    /* Set log header for tracing purpose. */
    fbe_encl_mgmt_fup_generate_logheader(pLocation, &logHeader[0], FBE_BASE_ENV_FUP_MAX_LOG_HEADER_SIZE);

    *pUpgradeQualified = (pLccInfo->inserted && !pLccInfo->faulted && pLccInfo->bDownloadable);
   
    if(*pUpgradeQualified != TRUE)
    {
        fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s, not qualified for upgrade, bDownloadable %d, inserted %d, faulted %d.\n",
                              &logHeader[0],
                              pLccInfo->bDownloadable,
                              pLccInfo->inserted,
                              pLccInfo->faulted);
    }

    return FBE_STATUS_OK;
}


/*!**************************************************************
 * @fn fbe_encl_mgmt_fup_initiate_upgrade(void * pContext, 
 *                                      fbe_u64_t deviceType, 
 *                                      fbe_device_physical_location_t * pLocation,
 *                                      fbe_u32_t forceFlags,
 *                                      fbe_u32_t upgradeRetryCount)
 ****************************************************************
 * @brief
 *  This function initiates the upgrade.
 *
 * @param pBaseEnv - The pointer to the fbe_base_environment_t.
 * @param deviceType -
 * @param pLocation - The pointer to the physical location that the work item needs to be added for.
 * @param forceFlags-
 * @param upgradeRetryCount-
 * 
 * @return fbe_status_t
 *
 * @version
 *  15-June-2010:  PHE - Created. 
 *
 ****************************************************************/
fbe_status_t fbe_encl_mgmt_fup_initiate_upgrade(void * pContext, 
                                              fbe_u64_t deviceType,
                                              fbe_device_physical_location_t * pLocation,
                                              fbe_u32_t forceFlags,
                                              fbe_u32_t upgradeRetryCount)
{
    fbe_encl_mgmt_t            * pEnclMgmt = (fbe_encl_mgmt_t *)pContext;
    fbe_encl_info_t            * pEnclInfo = NULL;
    fbe_lcc_info_t             * pLccInfo = NULL;
    fbe_bool_t                   policyAllowUpgrade = FALSE;
    fbe_bool_t                   upgradeQualified = FALSE;
    fbe_char_t                   logHeader[FBE_BASE_ENV_FUP_MAX_LOG_HEADER_SIZE + 1] = {0};
    fbe_status_t                 status = FBE_STATUS_OK;

    if(deviceType != FBE_DEVICE_TYPE_LCC)
    {
        fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "FUP: %s, unsupported deviceType 0x%llX.\n",
                              __FUNCTION__,
                              deviceType);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Set log header for tracing purpose. */
    fbe_encl_mgmt_fup_generate_logheader(pLocation, &logHeader[0], FBE_BASE_ENV_FUP_MAX_LOG_HEADER_SIZE);

    fbe_encl_mgmt_fup_policy_allowed(pEnclMgmt, pLocation, &policyAllowUpgrade);
    if(!policyAllowUpgrade)
    {
        fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s, %s, policy does not allow upgrade.\n",
                              &logHeader[0],
                              __FUNCTION__);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_encl_mgmt_get_encl_info_by_location(pEnclMgmt, pLocation, &pEnclInfo);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s, %s, get_encl_info_by_location failed.\n",
                              &logHeader[0],
                              __FUNCTION__);

        return status;
    }

    status = fbe_encl_mgmt_get_lcc_info_ptr(pEnclMgmt, pLocation, &pLccInfo);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s, %s, get lcc info ptr failed.\n",
                              &logHeader[0],
                              __FUNCTION__);

        return status;
    }
    
    fbe_encl_mgmt_fup_lcc_qualified(pEnclMgmt, pLocation, pLccInfo, &upgradeQualified);
    if(!upgradeQualified)
    {
        fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s, %s, not qualified for upgrade.\n",
                              &logHeader[0],
                              __FUNCTION__);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    if(pLccInfo->eses_version_desc == ESES_REVISION_2_0)
    {
        status = fbe_encl_mgmt_fup_create_and_add_work_item_from_manifest(pEnclMgmt,                                           
                                                                 pLocation,
                                                                 FBE_DEVICE_TYPE_LCC,
                                                                 pLccInfo,
                                                                 forceFlags,
                                                                 &logHeader[0],
                                                                 0,  // There is no interDeviceDelay for LCC FUP.
                                                                 upgradeRetryCount);   

        if(status != FBE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt,
                                  FBE_TRACE_LEVEL_WARNING,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s, Failed to create_and_add_work_item from manifest, status 0x%X.\n",
                                  &logHeader[0], status);
        }
    }
    else
    {
        status = fbe_base_env_fup_create_and_add_work_item((fbe_base_environment_t *)pEnclMgmt,
                                                   pLocation,
                                                   FBE_DEVICE_TYPE_LCC,
                                                   FBE_FW_TARGET_LCC_MAIN,
                                                   &pLccInfo->firmwareRev[0],
                                                   &pLccInfo->subenclProductId[0],
                                                   pLccInfo->uniqueId,
                                                   forceFlags,
                                                   &logHeader[0],
                                                   0,   // There is no interDeviceDelay for LCC FUP.
                                                   upgradeRetryCount,
                                                   pLccInfo->eses_version_desc);   

        if(status != FBE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt,
                                  FBE_TRACE_LEVEL_WARNING,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s, %s, Failed to create_and_add_work_item, status 0x%X.\n",
                                  &logHeader[0],
                                  __FUNCTION__,
                                  status);
        }
    }

    return status;
}
    

/*!*******************************************************************************
 * @fn fbe_encl_mgmt_fup_get_full_image_path
 *********************************************************************************
 * @brief
 *  This function gets the full enclosure image path. It includes the file name. 
 *
 * @param  pContext -
 * @param  pWorkItem -
 *
 * @return fbe_status_t - 
 *
 * @version
 *  10-Aug-2010 PHE -- Created.
 *******************************************************************************/
fbe_status_t fbe_encl_mgmt_fup_get_full_image_path(void * pContext,                               
                               fbe_base_env_fup_work_item_t * pWorkItem)
{
    fbe_base_environment_t * pBaseEnv =(fbe_base_environment_t *)pContext;
    fbe_encl_mgmt_t        * pEnclMgmt =(fbe_encl_mgmt_t *)pContext;
    fbe_char_t imageFileName[FBE_BASE_ENV_FUP_MAX_IMAGE_FILE_NAME_LEN] = {0};
    fbe_u32_t imageFileNameLen = 0;
    fbe_char_t imageFileConstantPortionName[FBE_BASE_ENV_FUP_MAX_IMAGE_FILE_NAME_LEN] = {0};
    fbe_char_t imagePathKey[FBE_BASE_ENV_FUP_MAX_IMAGE_PATH_KEY_LEN] = {0};
    fbe_u32_t imagePathIndex = 0;
    fbe_status_t   status = FBE_STATUS_OK;
    fbe_char_t *pImageFileName = NULL;

    if((pWorkItem->uniqueId == VOYAGER_ICM) ||
       (pWorkItem->uniqueId == VOYAGER_LCC))
    {
        imagePathIndex = 1;

        fbe_copy_memory(&imageFileConstantPortionName[0], 
                        FBE_ENCL_MGMT_CDES11_FUP_IMAGE_FILE_NAME,
                        sizeof(FBE_ENCL_MGMT_CDES11_FUP_IMAGE_FILE_NAME));

        fbe_copy_memory(&imagePathKey[0], 
                        FBE_ENCL_MGMT_CDES11_FUP_IMAGE_PATH_KEY,
                        sizeof(FBE_ENCL_MGMT_CDES11_FUP_IMAGE_PATH_KEY));
    }
    else if (pWorkItem->uniqueId == JETFIRE_SP_SANDYBRIDGE)
    {
        imagePathIndex = 2;

        fbe_copy_memory(&imageFileConstantPortionName[0], 
                        FBE_ENCL_MGMT_JDES_FUP_IMAGE_FILE_NAME,
                        sizeof(FBE_ENCL_MGMT_JDES_FUP_IMAGE_FILE_NAME));

        fbe_copy_memory(&imagePathKey[0], 
                        FBE_ENCL_MGMT_JDES_FUP_IMAGE_PATH_KEY,
                        sizeof(FBE_ENCL_MGMT_JDES_FUP_IMAGE_PATH_KEY));
    }
    else if((pWorkItem->uniqueId == VIKING_IO_EXP) ||
            (pWorkItem->uniqueId == VIKING_LCC))
    {
        imagePathIndex = 3;

        fbe_copy_memory(&imageFileConstantPortionName[0], 
                        FBE_ENCL_MGMT_CDES_UNIFIED_FUP_IMAGE_FILE_NAME,
                        sizeof(FBE_ENCL_MGMT_CDES_UNIFIED_FUP_IMAGE_FILE_NAME));

        fbe_copy_memory(&imagePathKey[0], 
                        FBE_ENCL_MGMT_CDES_UNIFIED_FUP_IMAGE_PATH_KEY,
                        sizeof(FBE_ENCL_MGMT_CDES_UNIFIED_FUP_IMAGE_PATH_KEY));
    }
    else if((pWorkItem->firmwareTarget == FBE_FW_TARGET_LCC_EXPANDER) || 
            (pWorkItem->firmwareTarget == FBE_FW_TARGET_LCC_INIT_STRING) ||
            (pWorkItem->firmwareTarget == FBE_FW_TARGET_LCC_FPGA))
    {
        /* Go find the file name for &imageFileConstantPortionName[0]*/
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

        
        if((strncmp((char *)pImageFileName, FBE_ENCL_MGMT_CDES2_FUP_CDEF_DUAL_IMAGE_FILE_NAME, sizeof(FBE_ENCL_MGMT_CDES2_FUP_CDEF_DUAL_IMAGE_FILE_NAME)-1) == 0)) 
        {
            /* pImageFileName is cdef_dual.bin.  
             * Do not need to copy .bin because in simulation, the file name will be cdef_dual_pidxxxx.bin.
             * We have to compare cdef_dual.bin first. Otherwise, for cdef_dual.bin, 
             * it will satisfy the comaprison with cdef.bin as well because it only compares the first 4 chacactors.
             */ 
             
            imagePathIndex = 5;

            fbe_copy_memory(&imageFileConstantPortionName[0], 
                        FBE_ENCL_MGMT_CDES2_FUP_CDEF_DUAL_IMAGE_FILE_NAME,
                        sizeof(FBE_ENCL_MGMT_CDES2_FUP_CDEF_DUAL_IMAGE_FILE_NAME));
        }
        else if(strncmp((char *)pImageFileName, FBE_ENCL_MGMT_CDES2_FUP_CDEF_IMAGE_FILE_NAME, sizeof(FBE_ENCL_MGMT_CDES2_FUP_CDEF_IMAGE_FILE_NAME)-1) == 0) 
        {
            /* pImageFileName is cdef.bin.  
             * Do not need to copy .bin because in simulation, the file name will be cdef_pidxxxx.bin. 
             * sizeof(FBE_ENCL_MGMT_CDES2_FUP_CDEF_IMAGE_FILE_NAME) is 5 but we only want to compare with "cdef" only. So we have to minus 1.
             */
            imagePathIndex = 4;

            fbe_copy_memory(&imageFileConstantPortionName[0], 
                        FBE_ENCL_MGMT_CDES2_FUP_CDEF_IMAGE_FILE_NAME,
                        sizeof(FBE_ENCL_MGMT_CDES2_FUP_CDEF_IMAGE_FILE_NAME));
        }
        else if(strncmp((char *)pImageFileName, FBE_ENCL_MGMT_CDES2_FUP_ISTR_IMAGE_FILE_NAME, sizeof(FBE_ENCL_MGMT_CDES2_FUP_ISTR_IMAGE_FILE_NAME)-1) == 0)
        {
            /* pImageFileName is istr.bin.  
             * Do not need to copy .bin because in simulation, the file name will be istr_pidxxxx.bin. 
             */
            imagePathIndex = 6;

            fbe_copy_memory(&imageFileConstantPortionName[0], 
                        FBE_ENCL_MGMT_CDES2_FUP_ISTR_IMAGE_FILE_NAME,
                        sizeof(FBE_ENCL_MGMT_CDES2_FUP_ISTR_IMAGE_FILE_NAME));
        }
        else if(strncmp((char *)pImageFileName, FBE_ENCL_MGMT_CDES2_FUP_CDES_ROM_IMAGE_FILE_NAME, sizeof(FBE_ENCL_MGMT_CDES2_FUP_CDES_ROM_IMAGE_FILE_NAME)-1) == 0)
        {
            /* pImageFileName is cdes_rom.bin.  
             * Do not need to copy .bin because in simulation, the file name will be cdes_rom_pidxxxx.bin. 
             */
            imagePathIndex = 7;

            fbe_copy_memory(&imageFileConstantPortionName[0], 
                        FBE_ENCL_MGMT_CDES2_FUP_CDES_ROM_IMAGE_FILE_NAME,
                        sizeof(FBE_ENCL_MGMT_CDES2_FUP_CDES_ROM_IMAGE_FILE_NAME));
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
                        FBE_ENCL_MGMT_CDES2_FUP_IMAGE_PATH_KEY,
                        sizeof(FBE_ENCL_MGMT_CDES2_FUP_IMAGE_PATH_KEY));

    }
    else
    {
        /* This is for Viper or Derringer. */
        imagePathIndex = 0;

        fbe_copy_memory(&imageFileConstantPortionName[0], 
                        FBE_ENCL_MGMT_CDES_FUP_IMAGE_FILE_NAME,
                        sizeof(FBE_ENCL_MGMT_CDES_FUP_IMAGE_FILE_NAME));

        fbe_copy_memory(&imagePathKey[0], 
                        FBE_ENCL_MGMT_CDES_FUP_IMAGE_PATH_KEY,
                        sizeof(FBE_ENCL_MGMT_CDES_FUP_IMAGE_PATH_KEY));
    }

    status = fbe_encl_mgmt_fup_build_image_file_name(pEnclMgmt,
                                                     pWorkItem,
                                                     &imageFileName[0], 
                                                     &imageFileConstantPortionName[0],
                                                     FBE_BASE_ENV_FUP_MAX_IMAGE_FILE_NAME_LEN,
                                                     &imageFileNameLen);

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

} 

/*!*******************************************************************************
 * @fn fbe_encl_mgmt_fup_get_manifest_file_full_path
 *********************************************************************************
 * @brief
 *  This function gets the full manifest file path including the file name. 
 *
 * @param  pContext (INPUT)-
 * @param  pManifestFileFullPath (OUTPUT) -
 *
 * @return fbe_status_t - 
 *
 * @version
 *  12-Sep-2014 PHE -- Created.
 *******************************************************************************/
fbe_status_t fbe_encl_mgmt_fup_get_manifest_file_full_path(void * pContext,
                                                           fbe_char_t * pManifestFileFullPath)
{
    fbe_base_environment_t * pBaseEnv =(fbe_base_environment_t *)pContext;
    fbe_char_t               manifestFileName[FBE_BASE_ENV_FUP_MAX_IMAGE_FILE_NAME_LEN] = {0};
    fbe_u32_t                manifestFileNameLen = 0;
    fbe_char_t               manifestFilePathKey[FBE_BASE_ENV_FUP_MAX_IMAGE_PATH_KEY_LEN] = {0};
    fbe_status_t             status = FBE_STATUS_OK;

    /* Build the manifest file name. In the simulation mode, 
     * we need to insert the _pidxxx to the manifest file name. 
     */
    status = fbe_base_env_fup_build_manifest_file_name(pBaseEnv,
                                                       &manifestFileName[0], 
                                                       FBE_ENCL_MGMT_CDES2_FUP_MANIFEST_FILE_NAME,  /* It does not have .json */
                                                       FBE_BASE_ENV_FUP_MAX_IMAGE_FILE_NAME_LEN,
                                                       &manifestFileNameLen);

    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)pBaseEnv,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "FUP: get_manifest_file_full_path, failed to build manifest file name.\n");
        return status;
    }

    /* Get the manifest file path key. */
    fbe_copy_memory(&manifestFilePathKey[0], 
                    FBE_ENCL_MGMT_CDES2_FUP_MANIFEST_FILE_PATH_KEY,
                    sizeof(FBE_ENCL_MGMT_CDES2_FUP_MANIFEST_FILE_PATH_KEY));

    /* Get the manifest file full path including path and file name. */
    status = fbe_base_env_fup_get_manifest_file_full_path(pBaseEnv,
                                              &manifestFilePathKey[0], 
                                              &manifestFileName[0],
                                              manifestFileNameLen,
                                              pManifestFileFullPath);

    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)pBaseEnv,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "FUP: get_manifest_file_full_path, failed to get manifest file full path.\n");
        return status;
    }

    return FBE_STATUS_OK;
} 

/*!**************************************************************
 * @fn fbe_encl_mgmt_fup_check_env_status(void * pContext, 
 *                                      fbe_base_env_fup_work_item_t * pWorkItem)
 ****************************************************************
 * @brief
 *  This function checks the environmental status for the specified work item
 *  to see whether it allows the upgrade.  
 * 
 * @param pContext - The pointer to the call back context.
 * @param pWorkItem - The pointer to the work item.
 *
 * @return fbe_status_t
 *     FBE_STATUS_OK - env status is good for upgrade.
 *     otherwise - env status is no good for upgrade.
 *
 * @version
 *  10-Aug-2010 PHE -- Created. 
 *
 ****************************************************************/
fbe_status_t fbe_encl_mgmt_fup_check_env_status(void * pContext,
                                 fbe_base_env_fup_work_item_t * pWorkItem)
{
    fbe_encl_mgmt_t                * pEnclMgmt = (fbe_encl_mgmt_t *)pContext;
    fbe_base_environment_t         * pBaseEnv =(fbe_base_environment_t *)pContext;
    fbe_device_physical_location_t * pLocation = NULL;
    fbe_encl_info_t                * pEnclInfo = NULL;
    fbe_u32_t                       slot = 0;
    fbe_status_t                    status = FBE_STATUS_OK;

    if (pEnclMgmt->base_environment.isSingleSpSystem == TRUE)
    {
        return FBE_STATUS_OK;   
    }

    pLocation = fbe_base_env_get_fup_work_item_location_ptr(pWorkItem),

    status = fbe_encl_mgmt_get_encl_info_by_location(pEnclMgmt,
                                                      pLocation, 
                                                      &pEnclInfo);

    if(status != FBE_STATUS_OK)
    {
        /* Can not find the enclosureIndex. This is coding error. */
        fbe_base_object_customizable_trace((fbe_base_object_t*)pBaseEnv,
                                        FBE_TRACE_LEVEL_ERROR,
                                        fbe_base_env_get_fup_work_item_logheader(pWorkItem),
                                        "%s, get_encl_info_by_location failed.\n",
                                        __FUNCTION__);

        return FBE_STATUS_GENERIC_FAILURE;
    }
    // check the peer lcc slots in the enclosure
    for (slot=0; slot < pEnclInfo->lccCount; slot++) 
    {
        // if the peer slot is not inserted or is faulted then return fail
        if (!pEnclInfo->lccInfo[slot].isLocal) 
        {
            if ((!pEnclInfo->lccInfo[slot].inserted) || (pEnclInfo->lccInfo[slot].faulted))
            {
                fbe_base_object_customizable_trace((fbe_base_object_t*)pBaseEnv,
                                                   FBE_TRACE_LEVEL_ERROR,
                                                   fbe_base_env_get_fup_work_item_logheader(pWorkItem),
                                                   "%s, peer slot:%d inserted:%d faulted:%d\n",
                                                   __FUNCTION__,
                                                   slot,
                                                   pEnclInfo->lccInfo[slot].inserted,
                                                   pEnclInfo->lccInfo[slot].faulted);
                return FBE_STATUS_GENERIC_FAILURE;
            }
        }
    }
    return FBE_STATUS_OK;
 
} // End of function fbe_encl_mgmt_fup_check_env_status


/*!**************************************************************
 * @fn fbe_encl_mgmt_fup_get_firmware_rev(void * pContext,
 *                         fbe_base_env_fup_work_item_t * pWorkItem,
 *                         fbe_u8_t * pPsFirmwareRev)
 ****************************************************************
 * @brief
 *  This function gets PS firmware rev.
 *
 * @param context - The pointer to the call back context.
 * @param pWorkItem - The pointer to the work item.
 * @param pFirmwareRev(OUTPUT) - The pointer to the new firmware rev. 
 *
 * @return fbe_status_t
 *
 * @version
 *  10-Aug-2010:  PHE - Created. 
 *
 ****************************************************************/
fbe_status_t fbe_encl_mgmt_fup_get_firmware_rev(void * pContext,
                          fbe_base_env_fup_work_item_t * pWorkItem,
                          fbe_u8_t * pFirmwareRev)
{
    fbe_encl_mgmt_t * pEnclMgmt = (fbe_encl_mgmt_t *)pContext;
    fbe_lcc_info_t lccInfo = {0};
    fbe_device_physical_location_t * pLocation = NULL;
    fbe_status_t status = FBE_STATUS_OK;
   
    if(pWorkItem->deviceType != FBE_DEVICE_TYPE_LCC)
    {
        /* The is the coding error. */
        fbe_base_object_customizable_trace((fbe_base_object_t*)pEnclMgmt,
                                           FBE_TRACE_LEVEL_ERROR,
                                           fbe_base_env_get_fup_work_item_logheader(pWorkItem),
                                           "%s, unsupported deviceType 0x%llX.\n",
                                           __FUNCTION__,
                                           pWorkItem->deviceType);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    pLocation = fbe_base_env_get_fup_work_item_location_ptr(pWorkItem);

    status = fbe_api_enclosure_get_lcc_info(pLocation, &lccInfo);
    if(status != FBE_STATUS_OK)
    {
        return status;
    }

    fbe_copy_memory(pFirmwareRev,
                    &lccInfo.firmwareRev[0], 
                    FBE_ENCLOSURE_MAX_FIRMWARE_REV_SIZE);

    return FBE_STATUS_OK;
}

/*!**************************************************************
 *  @fn fbe_encl_mgmt_get_lcc_fup_info_ptr(void * pContext,
 *                                         fbe_u64_t deviceType,
 *                                         fbe_device_physical_location_t * pLocation,
 *                                         fbe_base_env_fup_persistent_info_t ** pFupInfoPtr)
 ****************************************************************
 * @brief
 *  This function gets the pointer to the lcc firmware upgrade info. 
 *
 * @param pContext - The pointer to the callback context.
 * @param deviceType -
 * @param pLocation - 
 * @param pFupInfoPtr - 
 *
 * @return fbe_status_t - 
 *
 * @author
 *  20-Sept-2011:  PHE - Created. 
 *
 ****************************************************************/
fbe_status_t fbe_encl_mgmt_get_fup_info_ptr(void * pContext,
                                            fbe_u64_t deviceType,
                                            fbe_device_physical_location_t * pLocation,
                                            fbe_base_env_fup_persistent_info_t ** pFupInfoPtr)
{
    fbe_encl_mgmt_t                        * pEnclMgmt = (fbe_encl_mgmt_t *)pContext;
    fbe_encl_info_t                        * pEnclInfo = NULL;
    fbe_u32_t                                index = 0;
    fbe_u32_t                                subenclLccIndex = 0;
    fbe_status_t                             status = FBE_STATUS_OK;

    *pFupInfoPtr = NULL;

    if(deviceType != FBE_DEVICE_TYPE_LCC)
    {
        fbe_base_object_trace((fbe_base_object_t*)pEnclMgmt, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s, unsupported deviceType 0x%llX.\n",
                              __FUNCTION__,
                              deviceType);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_encl_mgmt_get_encl_info_by_location(pEnclMgmt, pLocation, &pEnclInfo);

    if (status != FBE_STATUS_OK) 
    {
        return status;
    }

    if(pLocation->componentId == 0) 
    {
        *pFupInfoPtr = &pEnclInfo->lccFupInfo[pLocation->slot];
        return FBE_STATUS_OK;
    }
    
    for(index = 0; index < pEnclInfo->subEnclCount; index ++ ) 
    {
        if(pEnclInfo->subEnclInfo[index].location.componentId == pLocation->componentId) 
        {
            /* Convert the subenclosure lcc slot to subenclosure lcc index. */
            subenclLccIndex = pLocation->slot%FBE_ESP_MAX_LCCS_PER_SUBENCL;
            *pFupInfoPtr = &pEnclInfo->subEnclInfo[index].lccFupInfo[subenclLccIndex];
            return FBE_STATUS_OK;
        }
    }

    return FBE_STATUS_NO_DEVICE;
}

/*!**************************************************************
 *  @fn fbe_encl_mgmt_get_fup_info(void * pContext,
 *                                         fbe_u64_t deviceType,
 *                                         fbe_device_physical_location_t * pLocation,
 *                                         fbe_base_env_fup_persistent_info_t ** pFupInfoPtr)
 ****************************************************************
 * @brief
 *  This function gets the pointer to the lcc firmware upgrade info. 
 *
 * @param pContext - The pointer to the callback context.
 * @param deviceType -
 * @param pLocation - 
 * @param pGetFupInfo - 
 *
 * @return fbe_status_t - 
 *
 * @author
 *  23-Aug-2012:  Rui Chang - Created.
 *  25-Mar-2013: PHE - Added the comparison with FBE_ESP_MAX_PROGRAMMABLE_COUNT
 *                     while setting the programmableCount to prevent from the buffer overflow.
 *
 ****************************************************************/
fbe_status_t fbe_encl_mgmt_get_fup_info(void * pContext,
                                            fbe_u64_t deviceType,
                                            fbe_device_physical_location_t * pLocation,
                                            fbe_esp_base_env_get_fup_info_t *  pGetFupInfo)
{
    fbe_encl_mgmt_t                        * pEnclMgmt = (fbe_encl_mgmt_t *)pContext;
    fbe_encl_info_t                        * pEnclInfo = NULL;
    fbe_u32_t                                index = 0;
    fbe_u32_t                                subenclLccIndex = 0;
    fbe_status_t                             status = FBE_STATUS_OK;
    fbe_base_env_fup_work_item_t           * pWorkItem = NULL;

    if(deviceType != FBE_DEVICE_TYPE_LCC)
    {
        fbe_base_object_trace((fbe_base_object_t*)pEnclMgmt, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s, unsupported deviceType 0x%llX.\n",
                              __FUNCTION__,
                              deviceType);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_encl_mgmt_get_encl_info_by_location(pEnclMgmt, pLocation, &pEnclInfo);

    if (status != FBE_STATUS_OK) 
    {
        return status;
    }
   
    /* Voyager EE lCC has two expander Fup info to get */
    if(pEnclInfo->enclType == FBE_ESP_ENCL_TYPE_VOYAGER &&
        pLocation->slot >= pEnclInfo->subEnclLccStartSlot)
    {  
        /* Compare the pEnclInfo->subEnclCount with FBE_ESP_MAX_PROGRAMMABLE_COUNT to prevent
         * from the buffer overflow.
         */
        pGetFupInfo->programmableCount = (pEnclInfo->subEnclCount <= FBE_ESP_MAX_PROGRAMMABLE_COUNT)?
                               pEnclInfo->subEnclCount : FBE_ESP_MAX_PROGRAMMABLE_COUNT;

        for(index = 0; index < pGetFupInfo->programmableCount; index ++ ) 
        {

            pLocation->componentId = pEnclInfo->subEnclInfo[index].location.componentId;

            pGetFupInfo->fupInfo[index].workState = FBE_BASE_ENV_FUP_WORK_STATE_NONE;
            status = fbe_base_env_fup_find_work_item((fbe_base_environment_t *)pContext, deviceType, pLocation, &pWorkItem);

            if(pWorkItem != NULL)
            {
                pGetFupInfo->fupInfo[index].workState = pWorkItem->workState;

                if (pGetFupInfo->fupInfo[index].workState == FBE_BASE_ENV_FUP_WORK_STATE_WAIT_BEFORE_UPGRADE)  
                {
                    pGetFupInfo->fupInfo[index].waitTimeBeforeUpgrade = fbe_base_env_get_wait_time_before_upgrade((fbe_base_environment_t *)pContext);
                }
            }

            subenclLccIndex = pLocation->slot%FBE_ESP_MAX_LCCS_PER_SUBENCL;

            pGetFupInfo->fupInfo[index].componentId = pEnclInfo->subEnclInfo[index].location.componentId; 
            pGetFupInfo->fupInfo[index].completionStatus = pEnclInfo->subEnclInfo[index].lccFupInfo[subenclLccIndex].completionStatus;
            fbe_copy_memory(&pGetFupInfo->fupInfo[index].imageRev[0],
                                &pEnclInfo->subEnclInfo[index].lccFupInfo[subenclLccIndex].imageRev[0],
                                FBE_ENCLOSURE_MAX_FIRMWARE_REV_SIZE + 1);
            fbe_copy_memory(&pGetFupInfo->fupInfo[index].currentFirmwareRev[0],
                                &pEnclInfo->subEnclInfo[index].lccFupInfo[subenclLccIndex].currentFirmwareRev[0],
                                FBE_ENCLOSURE_MAX_FIRMWARE_REV_SIZE + 1);
            fbe_copy_memory(&pGetFupInfo->fupInfo[index].preFirmwareRev[0],
                                &pEnclInfo->subEnclInfo[index].lccFupInfo[subenclLccIndex].preFirmwareRev[0],
                                FBE_ENCLOSURE_MAX_FIRMWARE_REV_SIZE + 1);

        }
    }
    else if(pEnclInfo->subEnclCount > 0)
    {
        /* Compare the 1 + pEnclInfo->subEnclCount with FBE_ESP_MAX_PROGRAMMABLE_COUNT to prevent
         * from the buffer overflow.
         */
        pGetFupInfo->programmableCount = ((1 + pEnclInfo->subEnclCount) <= FBE_ESP_MAX_PROGRAMMABLE_COUNT)?
                               (1 + pEnclInfo->subEnclCount) : FBE_ESP_MAX_PROGRAMMABLE_COUNT;

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
        pGetFupInfo->fupInfo[0].completionStatus = pEnclInfo->lccFupInfo[pLocation->slot].completionStatus;
        fbe_copy_memory(&pGetFupInfo->fupInfo[0].imageRev[0],
                            &pEnclInfo->lccFupInfo[pLocation->slot].imageRev[0],
                            FBE_ENCLOSURE_MAX_FIRMWARE_REV_SIZE + 1);
        fbe_copy_memory(&pGetFupInfo->fupInfo[0].currentFirmwareRev[0],
                            &pEnclInfo->lccFupInfo[pLocation->slot].currentFirmwareRev[0],
                            FBE_ENCLOSURE_MAX_FIRMWARE_REV_SIZE + 1);
        fbe_copy_memory(&pGetFupInfo->fupInfo[0].preFirmwareRev[0],
                            &pEnclInfo->lccFupInfo[pLocation->slot].preFirmwareRev[0],
                            FBE_ENCLOSURE_MAX_FIRMWARE_REV_SIZE + 1);

        if(pEnclInfo->subEnclCount > 0)
        {
            for(index = 1; index < pGetFupInfo->programmableCount; index ++ ) 
            {
                pLocation->componentId = pEnclInfo->subEnclInfo[index - 1].location.componentId;

                pGetFupInfo->fupInfo[index].workState = FBE_BASE_ENV_FUP_WORK_STATE_NONE;
                status = fbe_base_env_fup_find_work_item((fbe_base_environment_t *)pContext, deviceType, pLocation, &pWorkItem);

                if(pWorkItem != NULL)
                {
                    pGetFupInfo->fupInfo[index].workState = pWorkItem->workState;

                    if (pGetFupInfo->fupInfo[index].workState == FBE_BASE_ENV_FUP_WORK_STATE_WAIT_BEFORE_UPGRADE)  
                    {
                        pGetFupInfo->fupInfo[index].waitTimeBeforeUpgrade = fbe_base_env_get_wait_time_before_upgrade((fbe_base_environment_t *)pContext);
                    }
                }

                subenclLccIndex = pLocation->slot%FBE_ESP_MAX_LCCS_PER_SUBENCL;

                pGetFupInfo->fupInfo[index].componentId = pEnclInfo->subEnclInfo[index - 1].location.componentId;
                pGetFupInfo->fupInfo[index].completionStatus = pEnclInfo->subEnclInfo[index - 1].lccFupInfo[subenclLccIndex].completionStatus;

                fbe_copy_memory(&pGetFupInfo->fupInfo[index].imageRev[0],
                                    &pEnclInfo->subEnclInfo[index - 1].lccFupInfo[subenclLccIndex].imageRev[0],
                                    FBE_ENCLOSURE_MAX_FIRMWARE_REV_SIZE + 1);
                fbe_copy_memory(&pGetFupInfo->fupInfo[index].currentFirmwareRev[0],
                                    &pEnclInfo->subEnclInfo[index - 1].lccFupInfo[subenclLccIndex].currentFirmwareRev[0],
                                    FBE_ENCLOSURE_MAX_FIRMWARE_REV_SIZE + 1);
                fbe_copy_memory(&pGetFupInfo->fupInfo[index].preFirmwareRev[0],
                                    &pEnclInfo->subEnclInfo[index - 1].lccFupInfo[subenclLccIndex].preFirmwareRev[0],
                                    FBE_ENCLOSURE_MAX_FIRMWARE_REV_SIZE + 1);

            }
        }
    }
    else
    {
        pGetFupInfo->programmableCount = 1;

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
        pGetFupInfo->fupInfo[0].completionStatus = pEnclInfo->lccFupInfo[pLocation->slot].completionStatus;
        fbe_copy_memory(&pGetFupInfo->fupInfo[0].imageRev[0],
                            &pEnclInfo->lccFupInfo[pLocation->slot].imageRev[0],
                            FBE_ENCLOSURE_MAX_FIRMWARE_REV_SIZE + 1);
        fbe_copy_memory(&pGetFupInfo->fupInfo[0].currentFirmwareRev[0],
                            &pEnclInfo->lccFupInfo[pLocation->slot].currentFirmwareRev[0],
                            FBE_ENCLOSURE_MAX_FIRMWARE_REV_SIZE + 1);
        fbe_copy_memory(&pGetFupInfo->fupInfo[0].preFirmwareRev[0],
                            &pEnclInfo->lccFupInfo[pLocation->slot].preFirmwareRev[0],
                            FBE_ENCLOSURE_MAX_FIRMWARE_REV_SIZE + 1);

    }

    return FBE_STATUS_OK;
}

/*!***************************************************************
 * fbe_encl_mgmt_fup_resume_upgrade()
 ****************************************************************
 * @brief
 *  This function resumes the ugrade for all the devices which were aborted in this class.
 *
 * @param pContext -
 *
 * @return fbe_status_t
 *
 * @author
 *  01-Sept-2010: PHE - Created. 
 *
 ****************************************************************/
fbe_status_t fbe_encl_mgmt_fup_resume_upgrade(void * pContext)
{
    fbe_encl_mgmt_t                        * pEnclMgmt = (fbe_encl_mgmt_t *)pContext;
    fbe_encl_info_t                        * pEnclInfo = NULL;
    fbe_sub_encl_info_t                    * pSubEnclInfo = NULL;
    fbe_u32_t                                enclCount;
    fbe_u32_t                                slot = 0;
    fbe_device_physical_location_t           location = {0};
    fbe_status_t                             status = FBE_STATUS_OK;
    fbe_u32_t                                subenclIndex = 0;
    fbe_u32_t                                subenclLccIndex = 0;
    fbe_device_physical_location_t           subEnclLccLocation = {0};

    fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt,
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "FUP: Resume upgrade for LCC.\n");
   
    for(enclCount = 0; enclCount < FBE_ESP_MAX_ENCL_COUNT; enclCount ++) 
    {
        pEnclInfo = pEnclMgmt->encl_info[enclCount];

        if(pEnclInfo->object_id != FBE_OBJECT_ID_INVALID)
        {
            for(slot = 0; slot < pEnclInfo->lccCount; slot ++)
            {
                if(pEnclInfo->lccFupInfo[slot].completionStatus == 
                         FBE_BASE_ENV_FUP_COMPLETION_STATUS_ABORTED)
                { 
                    location.bus = pEnclInfo->location.bus;
                    location.enclosure = pEnclInfo->location.enclosure;
                    location.slot = slot;
                    status = fbe_encl_mgmt_fup_initiate_upgrade(pEnclMgmt, 
                                                      FBE_DEVICE_TYPE_LCC, 
                                                      &location,
                                                      pEnclInfo->lccFupInfo[slot].forceFlags,
                                                      0);  // upgradeRetryCount
                    if(status != FBE_STATUS_OK)
                    {
                        fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt,
                                              FBE_TRACE_LEVEL_WARNING,
                                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                              "FUP:Encl(%d_%d.%d)Lcc%d, %s, Failed to initiate upgrade, status 0x%X.\n",
                                              location.bus,
                                              location.enclosure,
                                              location.componentId,
                                              location.slot,
                                              __FUNCTION__,
                                              status);
                
                    }
                }

                for(subenclIndex = 0; subenclIndex < pEnclInfo->subEnclCount; subenclIndex ++) 
                {
                    pSubEnclInfo = &pEnclInfo->subEnclInfo[subenclIndex];

                    for(subenclLccIndex = 0; subenclLccIndex < FBE_ESP_MAX_LCCS_PER_SUBENCL; subenclLccIndex ++)
                    {
                        if(pSubEnclInfo->lccFupInfo[subenclLccIndex].completionStatus == 
                                    FBE_BASE_ENV_FUP_COMPLETION_STATUS_ABORTED)
                        {
                            subEnclLccLocation = pSubEnclInfo->location;
                            subEnclLccLocation.slot = pEnclInfo->subEnclLccStartSlot + subenclLccIndex;
                        
                            status = fbe_encl_mgmt_fup_initiate_upgrade(pEnclMgmt, 
                                                              FBE_DEVICE_TYPE_LCC, 
                                                              &subEnclLccLocation,
                                                              pSubEnclInfo->lccFupInfo[subenclLccIndex].forceFlags,
                                                              0); // upgradeRetryCount
                            if(status != FBE_STATUS_OK)
                            {
                                fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt,
                                                      FBE_TRACE_LEVEL_WARNING,
                                                      FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                                      "FUP:Encl(%d_%d.%d)LCC%d, %s, Failed to initiate upgrade, status 0x%X.\n",
                                                      subEnclLccLocation.bus,
                                                      subEnclLccLocation.enclosure,
                                                      subEnclLccLocation.componentId,
                                                      subEnclLccLocation.slot,
                                                      __FUNCTION__,
                                                      status);
                            }
                        }
                    }
                }
            }
        }
    }
    return FBE_STATUS_OK;
} /* End of function fbe_encl_mgmt_fup_resume_upgrade */

/*!***************************************************************
 * @fn fbe_encl_mgmt_fup_new_contact_init_upgrade(fbe_encl_mgmt_t *pEnclMgmt)
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
 * @param pEnclMgmt -pointer to fbe_encl_mgmt_t
 * 
 * @return fbe_status_t
 *
 * @author
 *  21-Mar-2011: GB - Created. 
 *
 ****************************************************************/
fbe_status_t fbe_encl_mgmt_fup_new_contact_init_upgrade(fbe_encl_mgmt_t *pEnclMgmt)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_device_physical_location_t          location;
    fbe_base_env_fup_persistent_info_t    * pFupInfo = NULL;
    fbe_char_t                              logHeader[FBE_BASE_ENV_FUP_MAX_LOG_HEADER_SIZE + 1] = {0};
    fbe_encl_info_t *pEnclInfo;
    fbe_u32_t enclIndex, lccIndex;

    // Peer contact back, update local LCC FW. 
    fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt,
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s peer SP contact is back.\n",
                                  __FUNCTION__);

    //visit all encl info
    for(enclIndex = 0; enclIndex < FBE_ESP_MAX_ENCL_COUNT; enclIndex++)
    {
        pEnclInfo = pEnclMgmt->encl_info[enclIndex];

        //skip empty slot and xpe
        if(pEnclInfo->object_id == FBE_OBJECT_ID_INVALID ||
                (pEnclInfo->location.bus == FBE_XPE_PSEUDO_BUS_NUM &&
                        pEnclInfo->location.enclosure == FBE_XPE_PSEUDO_ENCL_NUM))
        {
            continue;
        }

        location = pEnclInfo->location;

        for(lccIndex = 0; lccIndex < pEnclInfo->lccCount; lccIndex++)
        {

            //we only upgrade local lcc
            if(!pEnclInfo->lccInfo[lccIndex].isLocal)
            {
                continue;
            }

            if(pEnclInfo->lccInfo[lccIndex].inserted)
            {
                location.slot = lccIndex;
                /* Set log header for tracing purpose. */
                fbe_encl_mgmt_fup_generate_logheader(&location, &logHeader[0], FBE_BASE_ENV_FUP_MAX_LOG_HEADER_SIZE);

                // Check the fup status for no permission
                // because the peer was not present.
                status = fbe_encl_mgmt_get_fup_info_ptr(pEnclMgmt, FBE_DEVICE_TYPE_LCC, &location, &pFupInfo);

                if((status == FBE_STATUS_OK) &&
                   (pFupInfo->completionStatus == FBE_BASE_ENV_FUP_COMPLETION_STATUS_FAIL_NO_PEER_PERMISSION))
                {
                    fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt,
                                          FBE_TRACE_LEVEL_INFO,
                                          FBE_TRACE_MESSAGE_ID_INFO,
                                          "%s, %s, New FUP Attempt,Peer now Present\n",
                                          &logHeader[0],
                                          __FUNCTION__);
                    // We arrived here because the peer has contacted us for permission to upgrade.
                    // If we had a fup halted due to no peer present start a new fup attempt.
                    status = fbe_encl_mgmt_fup_initiate_upgrade(pEnclMgmt,
                                                                FBE_DEVICE_TYPE_LCC,
                                                                &location,
                                                                pFupInfo->forceFlags,  // use the forceFlags saved
                                                                0);  // upgradeRetryCount
                }
            }
        }
    }

    return FBE_STATUS_OK;
} //fbe_encl_mgmt_fup_new_contact_init_upgrade

/*!**************************************************************
 * @fn fbe_encl_mgmt_fup_copy_fw_rev_to_resume_prom(fbe_encl_mgmt_t *pEnclMgmt,
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
fbe_status_t fbe_encl_mgmt_fup_copy_fw_rev_to_resume_prom(fbe_encl_mgmt_t *pEnclMgmt,
                                                        fbe_u64_t deviceType,
                                                        fbe_device_physical_location_t *pLocation)
{
    fbe_char_t                             logHeader[FBE_BASE_ENV_FUP_MAX_LOG_HEADER_SIZE + 1] = {0};
    fbe_status_t                           status = FBE_STATUS_OK;
    fbe_base_env_resume_prom_info_t      * pResumePromInfo = NULL;
    fbe_device_physical_location_t         location = {0};
    fbe_lcc_info_t                       * pLccInfo = NULL;
    fbe_u32_t                              programmableIndex = 0;
    fbe_lcc_type_t                         lccType = FBE_LCC_TYPE_UNKNOWN;
    fbe_u8_t                               fwRevForResume[RESUME_PROM_PROG_REV_SIZE] = {0};

    fbe_encl_mgmt_fup_generate_logheader(pLocation, &logHeader[0], FBE_BASE_ENV_FUP_MAX_LOG_HEADER_SIZE);

    fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt,
                              FBE_TRACE_LEVEL_DEBUG_HIGH,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s, %s entry.\n",
                              &logHeader[0], __FUNCTION__);

    status = fbe_encl_mgmt_get_resume_prom_info_ptr(pEnclMgmt, 
                                                  deviceType, 
                                                  pLocation, 
                                                  &pResumePromInfo);

    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s, %s, get_resume_prom_info_ptr failed.\n",
                              &logHeader[0], __FUNCTION__);

        return status;
    }

   status = fbe_encl_mgmt_get_lcc_info_ptr(pEnclMgmt, pLocation, &pLccInfo);

    if (status != FBE_STATUS_OK) 
    {
        fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s, %s, get_lcc_info_ptr failed.\n",
                              &logHeader[0], __FUNCTION__);

        return status;
    }

    location.bus = pLocation->bus;
    location.enclosure = pLocation->enclosure;
    location.slot = pLocation->slot;
    lccType = pLccInfo->lccType;

    /* We should loop through all the subenclosures instead of hardcoding component id to 4.
     * Please refer to fbe_encl_mgmt_get_fup_info. 
     * But I don't want to change at this late stage of the program. 
     */

    if(lccType == FBE_LCC_TYPE_6G_VOYAGER_EE) 
    {
        pResumePromInfo->resume_prom_data.num_prog[0] = 2;
        location.componentId = 4;
    }
    else if(lccType == FBE_LCC_TYPE_6G_VIKING) 
    {
        pResumePromInfo->resume_prom_data.num_prog[0] = 5;
        location.componentId = 0;
    }
    else if(lccType == FBE_LCC_TYPE_12G_CAYENNE) 
    {
        pResumePromInfo->resume_prom_data.num_prog[0] = 2;
        location.componentId = 0;
    }
    else if(lccType == FBE_LCC_TYPE_12G_NAGA) 
    {
        pResumePromInfo->resume_prom_data.num_prog[0] = 3;
        location.componentId = 0;
    }
    else
    {
        pResumePromInfo->resume_prom_data.num_prog[0] = 1;
        location.componentId = 0;
    }

    for(programmableIndex = 0; 
        programmableIndex < pResumePromInfo->resume_prom_data.num_prog[0]; 
        programmableIndex++)
    {
       status = fbe_encl_mgmt_get_lcc_info_ptr(pEnclMgmt, &location, &pLccInfo);

        if (status != FBE_STATUS_OK) 
        {
            /* For Voyager enclosure, if local LCC is removed, there is no EE expanders.
             * If we want to get the LCC info for the subenclosures in order to get the 
             * EE firmware revision for the local LCC or peer LCC, we would come here.
             * Zero out the rev to make sure there is no garbage.
             */
            fbe_encl_mgmt_fup_generate_logheader(&location, &logHeader[0], FBE_BASE_ENV_FUP_MAX_LOG_HEADER_SIZE);

            fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt,
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s, %s, get_lcc_info_ptr failed.\n",
                                  &logHeader[0], __FUNCTION__);

            fbe_zero_memory(pResumePromInfo->resume_prom_data.prog_details[programmableIndex].prog_rev,
                            RESUME_PROM_PROG_REV_SIZE);
        }
        else
        {
            fbe_copy_memory(pResumePromInfo->resume_prom_data.prog_details[programmableIndex].prog_name,
                            "CDES\0",
                            5);

            fbe_base_env_fup_convert_fw_rev_for_rp((fbe_base_environment_t *)pEnclMgmt,
                                                   &fwRevForResume[0],
                                                   &pLccInfo->firmwareRev[0]);

            fbe_copy_memory(pResumePromInfo->resume_prom_data.prog_details[programmableIndex].prog_rev,
                            &fwRevForResume[0],
                            RESUME_PROM_PROG_REV_SIZE);
        }

        if((lccType == FBE_LCC_TYPE_6G_VIKING) && 
           (location.componentId == 0))
        {
            location.componentId = 2;
        }
        else if(((lccType == FBE_LCC_TYPE_12G_CAYENNE) || (lccType == FBE_LCC_TYPE_12G_NAGA)) && 
           (location.componentId == 0))
        {
            location.componentId = 4;
        }
        else
        {
            location.componentId++;
        }
    }
        
    return FBE_STATUS_OK;
}

/*!*************************************************************************
 *  @fn fbe_encl_mgmt_fup_is_activate_in_progress()
 **************************************************************************
 *  @brief
 *      This function checks whether the device is in activation.
 *
 *  @param fbe_encl_mgmt_t - The pointer to the fbe_encl_mgmt_t object.
 *  @param deviceType - The device type.
 *  @param pLocation - The pointer to the device location.
 *  @param pActivateInProgress(OUTPUT) - whether the device is in activation or not.
 * 
 *  @return  fbe_status_t
 *
 *  @version
 *  01-Mar-2013 PHE - Created.
 *
 **************************************************************************/
fbe_status_t fbe_encl_mgmt_fup_is_activate_in_progress(fbe_encl_mgmt_t * pEnclMgmt,
                                         fbe_u64_t deviceType,
                                         fbe_device_physical_location_t * pLocation,
                                         fbe_bool_t * pActivateInProgress)
{
    fbe_base_env_fup_persistent_info_t  * pFupInfo = NULL;
    fbe_encl_info_t                     * pEnclInfo = NULL;
    fbe_u32_t                             subEnclIndex = 0;
    fbe_u32_t                             subenclLccIndex = 0;
    fbe_status_t                          status = FBE_STATUS_OK;
    fbe_char_t                            logHeader[FBE_BASE_ENV_FUP_MAX_LOG_HEADER_SIZE + 1] = {0};
   
    /* Initialize to FBE_FALSE.*/
    *pActivateInProgress = FBE_FALSE;

    if(deviceType != FBE_DEVICE_TYPE_LCC) 
    {
        fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s, (%d_%d_%d) unsupported device type 0x%llX.\n",
                              __FUNCTION__,
                              pLocation->bus,
                              pLocation->enclosure,
                              pLocation->slot,
                              deviceType);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Set log header for tracing purpose. */
    fbe_encl_mgmt_fup_generate_logheader(pLocation, &logHeader[0], FBE_BASE_ENV_FUP_MAX_LOG_HEADER_SIZE);

    status = fbe_encl_mgmt_get_fup_info_ptr(pEnclMgmt, deviceType, pLocation, &pFupInfo);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt,
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s, %s, get_lcc_info_ptr failed.\n",
                                  &logHeader[0], __FUNCTION__);

        return status;
    }

    if((pFupInfo->completionStatus == FBE_BASE_ENV_FUP_COMPLETION_STATUS_IN_PROGRESS) &&
       (pFupInfo->pWorkItem != NULL))
    {
        if(fbe_base_env_get_fup_work_item_state(pFupInfo->pWorkItem) == 
           FBE_BASE_ENV_FUP_WORK_STATE_ACTIVATE_IMAGE_SENT)
        {
            fbe_base_object_customizable_trace((fbe_base_object_t*)pEnclMgmt,
                        FBE_TRACE_LEVEL_INFO,
                        &logHeader[0],
                        "FUP_SUPPRESS_FLT ACTIVATE in progress!!!\n");

            *pActivateInProgress = FBE_TRUE;
            return FBE_STATUS_OK;
        }
    }

    if(pFupInfo->suppressFltDueToPeerFup) 
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)pEnclMgmt,
                        FBE_TRACE_LEVEL_INFO,
                        &logHeader[0],
                        "PEER_FUP_SUPPRESS_FLT, peer SP initiated ACTIVATE in progress!!!\n");

        *pActivateInProgress = FBE_TRUE;
        return FBE_STATUS_OK;
    }

    status = fbe_encl_mgmt_get_encl_info_by_location(pEnclMgmt, pLocation, &pEnclInfo);

    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s, get_encl_info_by_location failed, status 0x%x.\n", 
                              &logHeader[0], 
                              status);
        return(status);
    }

    /* Voyager EE lCC has two expanders.
     * Viking LCC has one IOSXP and four DRVSXP.
     * Cayenne LCC has one IOSXP and one DRVSXP.
     * Naga LCC has one IOSXP and two DRVSXP.
     * If any EE expander is in activation, we return activateInProgress as TRUE.
     */
    if((pLocation->slot >= pEnclInfo->subEnclLccStartSlot) && 
       (pLocation->componentId == 0))
    {
        for(subEnclIndex = 0; subEnclIndex < pEnclInfo->subEnclCount; subEnclIndex ++ ) 
        {
            subenclLccIndex = pLocation->slot%FBE_ESP_MAX_LCCS_PER_SUBENCL;

            pFupInfo = &pEnclInfo->subEnclInfo[subEnclIndex].lccFupInfo[subenclLccIndex];

            if((pFupInfo->completionStatus == FBE_BASE_ENV_FUP_COMPLETION_STATUS_IN_PROGRESS) &&
               (pFupInfo->pWorkItem != NULL))
            {
                if(fbe_base_env_get_fup_work_item_state(pFupInfo->pWorkItem) == 
                   FBE_BASE_ENV_FUP_WORK_STATE_ACTIVATE_IMAGE_SENT)
                {
                    fbe_base_object_customizable_trace((fbe_base_object_t*)pEnclMgmt,
                        FBE_TRACE_LEVEL_INFO,
                        fbe_base_env_get_fup_work_item_logheader(pFupInfo->pWorkItem),
                        "ACTIVATE in progress!!!\n");

                    *pActivateInProgress = FBE_TRUE;
                    break;
                }
            }
            if(pFupInfo->suppressFltDueToPeerFup) 
            {
                fbe_base_object_customizable_trace((fbe_base_object_t*)pEnclMgmt,
                        FBE_TRACE_LEVEL_INFO,
                        fbe_base_env_get_fup_work_item_logheader(pFupInfo->pWorkItem),
                        "ACTIVATE in progress initiatied by peer SP!!!\n");

                *pActivateInProgress = FBE_TRUE;
                break;
            }
        }
    }

    return FBE_STATUS_OK;
} // End of fbe_encl_mgmt_fup_is_activate_in_progress

/*!**************************************************************
 * @fn fbe_encl_mgmt_fup_refresh_device_status(void * pContext,
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
fbe_status_t fbe_encl_mgmt_fup_refresh_device_status(void * pContext,
                                             fbe_base_env_fup_work_item_t * pWorkItem)
{
    fbe_encl_mgmt_t                   * pEnclMgmt = (fbe_encl_mgmt_t *)pContext;
    fbe_encl_info_t                   * pEnclInfo = NULL;
    fbe_device_physical_location_t      tempLocation = {0};
    fbe_u32_t                           slot = 0;
    fbe_u32_t                           index = 0;
    fbe_object_id_t                     subEnclObjectId = FBE_OBJECT_ID_INVALID;
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_device_physical_location_t    * pLocation = NULL;

    if(pWorkItem->deviceType != FBE_DEVICE_TYPE_LCC)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)pEnclMgmt,
                        FBE_TRACE_LEVEL_ERROR,
                        fbe_base_env_get_fup_work_item_logheader(pWorkItem),
                        "%s unsupported deviceType 0x%llX.\n",   
                        __FUNCTION__, pWorkItem->deviceType);

        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    if(pWorkItem->workState >= FBE_BASE_ENV_FUP_WORK_STATE_ACTIVATE_IMAGE_SENT)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)pEnclMgmt,
                        FBE_TRACE_LEVEL_INFO,
                        fbe_base_env_get_fup_work_item_logheader(pWorkItem),
                        "Refresh device status...\n");
    }
    else
    {
        return FBE_STATUS_OK;
    }

    pLocation = &pWorkItem->location;
    status = fbe_encl_mgmt_get_encl_info_by_location(pEnclMgmt, pLocation, &pEnclInfo);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)pEnclMgmt,
                        FBE_TRACE_LEVEL_INFO,
                        fbe_base_env_get_fup_work_item_logheader(pWorkItem),
                        "Refresh device status, get_encl_info_by_location failed, status 0x%X.\n",
                        status);

        /* 
         * The enclosure which contains the LCC might already be removed.
         * So we just return OK here. 
         */ 
        return FBE_STATUS_OK;
    }

    for(slot = 0; slot < pEnclInfo->lccCount; slot ++)
    {
        tempLocation.bus = pLocation->bus;
        tempLocation.enclosure = pLocation->enclosure;
        tempLocation.slot = slot;

        status = fbe_encl_mgmt_process_lcc_status(pEnclMgmt, pEnclInfo->object_id, &tempLocation);

        if(status != FBE_STATUS_OK)
        {
            fbe_base_object_customizable_trace((fbe_base_object_t*)pEnclMgmt,
                        FBE_TRACE_LEVEL_INFO,
                        fbe_base_env_get_fup_work_item_logheader(pWorkItem),
                        "Refresh device status, process_lcc_status failed, status 0x%X.\n",
                        status);
    
            continue;
        }
    }
    
    for(index = 0; index < pEnclInfo->subEnclCount; index ++)
    {
        subEnclObjectId = pEnclInfo->subEnclInfo[index].objectId;
        status = fbe_encl_mgmt_process_subencl_all_lcc_status(pEnclMgmt,
                                                     pEnclInfo->subEnclLccStartSlot, 
                                                     subEnclObjectId, 
                                                     &pEnclInfo->subEnclInfo[index].location);
        if(status != FBE_STATUS_OK)
        {
            fbe_base_object_customizable_trace((fbe_base_object_t*)pEnclMgmt,
                        FBE_TRACE_LEVEL_INFO,
                        fbe_base_env_get_fup_work_item_logheader(pWorkItem),
                        "Refresh device status, process_subencl_all_lcc_status failed, status 0x%X.\n",
                        status);

            continue;
       }
    }

    return FBE_STATUS_OK;
}


/*!**************************************************************
 * @fn fbe_encl_mgmt_fup_create_and_add_work_item_from_manifest(fbe_encl_mgmt_t * pEnclMgmt,                                           
 *                                         fbe_device_physical_location_t * pLocation,
 *                                         fbe_u64_t deviceType,
 *                                         fbe_lcc_info_t * pLccInfo,
 *                                         fbe_u32_t forceFlags,
 *                                         fbe_char_t * pLogHeader,
 *                                         fbe_u32_t interDeviceDelayInSec)
 ****************************************************************
 * @brief
 *  This function creates the work items for the LCC upgrade based on the manifest info.
 *
 * @param pEnclMgmt - The pointer to the fbe_base_environment_t.
 * @param pLocation - The pointer to the physical location that the work item needs to be added for.
 * @param deviceType - 
 * @param pLccInfo -
 * @param forceFlags -
 * @param pLogHeader -
 * @param interDeviceDelayInSec -
 * @param upgradeRetryCount - 
 *
 * @return fbe_status_t
 * 
 * @note The primary expander image "cdes_rom.bin" with the target as FBE_FW_TARGET_LCC_EXPANDER
 *       should be downloaded as the last image for the LCC.
 *       We need to make sure the primary expander image was saved in the manifest as the last image.
 * 
 * @version
 *  07-Aug-2014:  PHE - Created. 
 *
 ****************************************************************/
static fbe_status_t
fbe_encl_mgmt_fup_create_and_add_work_item_from_manifest(fbe_encl_mgmt_t * pEnclMgmt,                                           
                                          fbe_device_physical_location_t * pLocation,
                                          fbe_u64_t deviceType,
                                          fbe_lcc_info_t * pLccInfo,
                                          fbe_u32_t forceFlags,
                                          fbe_char_t * pLogHeader,
                                          fbe_u32_t interDeviceDelayInSec,
                                          fbe_u32_t upgradeRetryCount)
{
    fbe_base_environment_t  * pBaseEnv = (fbe_base_environment_t *)pEnclMgmt;
    fbe_u32_t                 subEnclIndex = 0;
    fbe_u32_t                 imageIndex = 0;
    fbe_status_t              status = FBE_STATUS_OK;
    fbe_base_env_fup_manifest_info_t * pManifestInfo = NULL;
    fbe_char_t              * pLccFirmwareRev = NULL;
    fbe_enclosure_fw_target_t firmwareTarget = FBE_FW_TARGET_INVALID;
    fbe_bool_t                createWorkItemForExpanderImage = FBE_FALSE;
    fbe_char_t                logHeaderWithFwTarget[FBE_BASE_ENV_FUP_MAX_LOG_HEADER_SIZE + 1] = {0};
    fbe_bool_t                manifestInfoAvail = FBE_FALSE;
    fbe_base_env_fup_work_item_t * pWorkItem = NULL;
    fbe_base_env_fup_format_fw_rev_t hwFirmwareRevAfterFormatting = {0};
    fbe_base_env_fup_format_fw_rev_t imageRevAfterFormatting = {0};

    fbe_base_env_fup_find_work_item(pBaseEnv, deviceType, pLocation, &pWorkItem);

    if (pWorkItem != NULL) 
    {
        /* There is already a work item for it. It could be CDEF, initStr or LCC expander. No need to continue. */
        fbe_base_object_customizable_trace((fbe_base_object_t*)pBaseEnv,
                                        FBE_TRACE_LEVEL_INFO,
                                        fbe_base_env_get_fup_work_item_logheader(pWorkItem),
                                        "create_and_add_work_item_from_manifest, Upgrade already started, no need to start again.\n");
        
        return FBE_STATUS_OK;
    }

    manifestInfoAvail = fbe_base_env_fup_get_manifest_info_avail(pBaseEnv);

    if((!manifestInfoAvail) || 
       (forceFlags & FBE_BASE_ENV_FUP_FORCE_FLAG_READ_MANIFEST_FILE))
    {
        status = fbe_base_env_fup_init_manifest_info(pBaseEnv);
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

    for(subEnclIndex = 0; subEnclIndex < FBE_BASE_ENV_FUP_MAX_SUBENCL_COUNT_IN_MANIFEST; subEnclIndex ++)
    {
        pManifestInfo = fbe_base_env_fup_get_manifest_info_ptr(pBaseEnv, subEnclIndex);

        if(strncmp(&pManifestInfo->subenclProductId[0], 
                   &pLccInfo->subenclProductId[0],
                   FBE_ENCLOSURE_MAX_SUBENCL_PRODUCT_ID_SIZE) == 0)
        {
            /* 
             * Skip the env check if the hardware CDES rev majorNumber is less than 2.
             * Starting from CDES 2.0.0, the peer to peer communication changed. 
             * If one LCC is running at a a rev lower than 2.0.0 and another LCC is running at a rev 2.0.0 or above,                                                                .
             * the LCC which is running at a rev lower than 2.0.0 would report peer LCC as faulted                                                                                                                                                                                                                                 .
             * In this case, the LCC upgrade would fail at env check in the normal upgrade process.                                                                                                                                                                                                                                                          .
             * So we need to skip the env check here.                                                                                                                                                                                                                                                                                                                   .
             */
            for(imageIndex = 0; imageIndex < pManifestInfo->imageFileCount; imageIndex ++)
            {
                firmwareTarget = fbe_base_env_fup_get_manifest_fw_target(pBaseEnv, 
                                                                  subEnclIndex,
                                                                  imageIndex);

                if(firmwareTarget == FBE_FW_TARGET_LCC_EXPANDER) 
                {
                    fbe_base_env_fup_format_fw_rev(&hwFirmwareRevAfterFormatting, 
                                   &pLccInfo->firmwareRev[0],
                                   FBE_ENCLOSURE_MAX_FIRMWARE_REV_SIZE);

                    fbe_base_env_fup_format_fw_rev(&imageRevAfterFormatting, 
                                   &pManifestInfo->imageRev[imageIndex][0],
                                   FBE_ENCLOSURE_MAX_FIRMWARE_REV_SIZE);

                    if((hwFirmwareRevAfterFormatting.majorNumber < 2) && 
                       (imageRevAfterFormatting.majorNumber > 1))
                    {
                        forceFlags = (forceFlags | FBE_BASE_ENV_FUP_FORCE_FLAG_NO_ENV_CHECK);
                        fbe_base_object_trace((fbe_base_object_t*)pBaseEnv,
                                              FBE_TRACE_LEVEL_INFO,
                                              FBE_TRACE_MESSAGE_ID_INFO,
                                              "FUP:CDES Rev Lock, Set Flag NoEnvCheck Flags:0x%x\n",
                                              forceFlags);
                    }
                }
            }

            for(imageIndex = 0; imageIndex < pManifestInfo->imageFileCount; imageIndex ++)
            {
                firmwareTarget = fbe_base_env_fup_get_manifest_fw_target(pBaseEnv, 
                                                                  subEnclIndex,
                                                                  imageIndex);

                switch(firmwareTarget)
                {
                    case FBE_FW_TARGET_LCC_EXPANDER:
                        pLccFirmwareRev = &pLccInfo->firmwareRev[0];
                        createWorkItemForExpanderImage = FBE_TRUE;
                        /* We do not create the work item at this time.
                         * Set the flag and create the work item after the work items for init string and
                         * fpga are created. 
                         */ 
                        continue;
                        break;
            
                    case FBE_FW_TARGET_LCC_INIT_STRING:
                        pLccFirmwareRev = &pLccInfo->initStrFirmwareRev[0];
                        break;
    
                    case FBE_FW_TARGET_LCC_FPGA:
                        pLccFirmwareRev = &pLccInfo->fpgaFirmwareRev[0];
                        break;
    
                    default:
                        return FBE_STATUS_GENERIC_FAILURE;
                        break;
                }

                fbe_sprintf(&logHeaderWithFwTarget[0], 
                            (FBE_BASE_ENV_FUP_MAX_LOG_HEADER_SIZE + 1), 
                            "%s %s", 
                            pLogHeader,
                            fbe_base_env_decode_firmware_target(firmwareTarget));

                status = fbe_base_env_fup_create_and_add_work_item(pBaseEnv,
                                                       pLocation,
                                                       deviceType,
                                                       firmwareTarget,
                                                       pLccFirmwareRev,
                                                       &pLccInfo->subenclProductId[0],
                                                       pLccInfo->uniqueId,
                                                       (forceFlags|FBE_BASE_ENV_FUP_FORCE_FLAG_ACTIVATION_DEFERRED),
                                                       &logHeaderWithFwTarget[0],
                                                       interDeviceDelayInSec,
                                                       upgradeRetryCount,
                                                       pLccInfo->eses_version_desc);
    
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
            }

            /* We create the work item for the primary image (i.e. the expander image) at last to 
             * make sure we do the activation after the init string and fpga images are downloaded.
             */
            if(createWorkItemForExpanderImage)
            {
                forceFlags &= ~FBE_BASE_ENV_FUP_FORCE_FLAG_ACTIVATION_DEFERRED;

                fbe_sprintf(&logHeaderWithFwTarget[0], 
                            (FBE_BASE_ENV_FUP_MAX_LOG_HEADER_SIZE + 1), 
                            "%s %s", 
                            pLogHeader,
                            fbe_base_env_decode_firmware_target(firmwareTarget));

                status = fbe_base_env_fup_create_and_add_work_item(pBaseEnv,
                                                       pLocation,
                                                       deviceType,
                                                       firmwareTarget,
                                                       pLccFirmwareRev,
                                                       &pLccInfo->subenclProductId[0],
                                                       pLccInfo->uniqueId,
                                                       forceFlags,
                                                       &logHeaderWithFwTarget[0],
                                                       interDeviceDelayInSec,
                                                       upgradeRetryCount,
                                                       pLccInfo->eses_version_desc);
    
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
            }// Emd of - if(creatWorkItemForPrimaryImageLater)

            /* We have found the subEncl, no need to continue in the loop.
             */
            break;
        }// End of - if(pManifestInfo->subenclUniqueId == pLccInfo->uniqueId)
    }// End of - for(subEnclIndex = 0;
        
    return status;
}

//End of file fbe_encl_mgmt_fup.c

