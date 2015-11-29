/***************************************************************************
 * Copyright (C) EMC Corporation 2012
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_module_mgmt_fup.c
 ***************************************************************************
 * @brief
 *  This file contains the implemention for module firmware upgrade functionality.
 *
 * @version
 *   29-Oct-2012:  Rui Chang - Created.
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_memory.h"
#include "fbe/fbe_api_enclosure_interface.h"
#include "fbe_base_object.h"
#include "fbe_module_mgmt_private.h"
#include "fbe_module_mgmt.h"
#include "fbe_base_environment_debug.h"

static fbe_status_t fbe_module_mgmt_fup_bem_qualified(fbe_module_mgmt_t * pModuleMgmt,
                                                    fbe_device_physical_location_t * pLocation,
                                                    fbe_lcc_info_t * pLccInfo, 
                                                    fbe_bool_t * pUpgradeQualified);

static fbe_status_t fbe_module_mgmt_fup_policy_allowed(fbe_module_mgmt_t * pModuleMgmt, 
                                        fbe_device_physical_location_t * pLocation,
                                        fbe_bool_t * pPolicyAllowed);

static fbe_status_t fbe_module_mgmt_fup_generate_logheader(fbe_device_physical_location_t * pLocation, 
                                       fbe_u8_t * pLogHeader, fbe_u32_t logHeaderSize);

static fbe_status_t 
fbe_module_mgmt_fup_create_and_add_work_item_from_manifest(fbe_module_mgmt_t * pModuleMgmt,
                                          fbe_device_physical_location_t * pLocation,
                                          fbe_u64_t deviceType,
                                          fbe_lcc_info_t * pLccInfo,
                                          fbe_u32_t forceFlags,
                                          fbe_char_t * pLogHeader,
                                          fbe_u32_t interDeviceDelayInSec,
                                          fbe_u32_t upgradeRetryCount);

static void fbe_module_mgmt_fup_restart_env_failed_fup(fbe_module_mgmt_t * pModuleMgmt,
                                                       fbe_device_physical_location_t * pLocation,
                                                       fbe_lcc_info_t * pLccInfo);

fbe_status_t fbe_module_mgmt_get_bem_info_ptr(fbe_module_mgmt_t * pModuleMgmt,
                                                      fbe_device_physical_location_t * pLocation,
                                                      fbe_io_module_physical_info_t ** pBemInfoPtr);



/*!**************************************************************
 * @fn fbe_module_mgmt_fup_generate_logheader(fbe_device_physical_location_t * pLocation, 
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
 *  29-Oct-2012:  Rui Chang - Created. 
 *
 ****************************************************************/
static fbe_status_t fbe_module_mgmt_fup_generate_logheader(fbe_device_physical_location_t * pLocation, 
                                                         fbe_u8_t * pLogHeader, 
                                                         fbe_u32_t logHeaderSize)
{
    fbe_zero_memory(pLogHeader, logHeaderSize);

    csx_p_snprintf(pLogHeader, logHeaderSize, "FUP:BM(SP: %d, Slot: %d)",
                           pLocation->sp, pLocation->slot);

    return FBE_STATUS_OK;
}


/*!**************************************************************
 * @fn fbe_module_mgmt_fup_handle_bem_status_change(fbe_module_mgmt_t * pModuleMgmt,
 *                                                fbe_device_physical_location_t * pLocation,
 *                                                fbe_lcc_info_t * pNewLccInfo,
 *                                                fbe_lcc_info_t * pOldLccInfo)
 ****************************************************************
 * @brief
 *  This function handles the BEM status change for firmware upgrade. 
 * 
 * @param pPsMgmt -
 * @param pLocation - The location of the PS.
 * @param pNewPsInfo - The new power supply info.
 * @param pOldPsInfo - The old power supply info.
 * 
 * @return fbe_status_t 
 *
 * @version
 *  29-Oct-2012:  Rui Chang - Created. 
 *
 ****************************************************************/
fbe_status_t fbe_module_mgmt_fup_handle_bem_status_change(fbe_module_mgmt_t * pModuleMgmt,
                                                 fbe_device_physical_location_t * pLocation,
                                                 fbe_lcc_info_t * pNewLccInfo,
                                                 fbe_lcc_info_t * pOldLccInfo)
{
    fbe_bool_t                     newQualified;
    fbe_bool_t                     oldQualified;
    fbe_status_t                   status = FBE_STATUS_OK;
    fbe_char_t                     logHeader[FBE_BASE_ENV_FUP_MAX_LOG_HEADER_SIZE + 1] = {0};
    fbe_bool_t                     policyAllowUpgrade = FBE_FALSE;

    if(pLocation->sp >= SP_ID_MAX)
    {
        fbe_base_object_trace((fbe_base_object_t *)pModuleMgmt,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "FUP: %s, Invalid SP ID 0x%X.\n",
                              __FUNCTION__,
                              pLocation->sp);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Bem is part of jetfire which is a DPE, so set bus and encl number as 0 */
    pLocation->bus = 0;
    pLocation->enclosure = 0;

    /* Set log header for tracing purpose. */
    fbe_module_mgmt_fup_generate_logheader(pLocation, &logHeader[0], FBE_BASE_ENV_FUP_MAX_LOG_HEADER_SIZE);

    fbe_base_object_trace((fbe_base_object_t *)pModuleMgmt, 
                              FBE_TRACE_LEVEL_DEBUG_LOW,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s %s\n",
                              &logHeader[0],
                              __FUNCTION__);
    
    if((!pNewLccInfo->inserted) && (pOldLccInfo->inserted))
    {
        fbe_base_object_trace((fbe_base_object_t *)pModuleMgmt,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s, Device removed.\n",
                              &logHeader[0]);

       
        /* The BEM was removed. */
        status = fbe_base_env_fup_handle_device_removal((fbe_base_environment_t *)pModuleMgmt, 
                                                        FBE_DEVICE_TYPE_BACK_END_MODULE, 
                                                        pLocation);
        if(status != FBE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t *)pModuleMgmt,
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
        // must call fbe_module_mgmt_fup_restart_env_failed_fup() before the policy check
        fbe_module_mgmt_fup_restart_env_failed_fup(pModuleMgmt,
                                                   pLocation,
                                                   pNewLccInfo);

        fbe_module_mgmt_fup_policy_allowed(pModuleMgmt, pLocation, &policyAllowUpgrade);
        if(!policyAllowUpgrade)
        {
            fbe_base_object_trace((fbe_base_object_t *)pModuleMgmt,
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s, %s, policy does not allow upgrade.\n",
                                  &logHeader[0],
                                  __FUNCTION__);
    
            return FBE_STATUS_OK;
        }

        fbe_module_mgmt_fup_bem_qualified(pModuleMgmt, pLocation, pNewLccInfo, &newQualified);
        fbe_module_mgmt_fup_bem_qualified(pModuleMgmt, pLocation, pOldLccInfo, &oldQualified);

        if(newQualified && !oldQualified)
        {
            fbe_base_object_trace((fbe_base_object_t *)pModuleMgmt,
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s, %s, became qualified for upgrade.\n",
                                  &logHeader[0],
                                  __FUNCTION__);

            status = fbe_module_mgmt_fup_initiate_upgrade(pModuleMgmt, 
                                                      FBE_DEVICE_TYPE_BACK_END_MODULE, 
                                                      pLocation,
                                                      FBE_BASE_ENV_FUP_FORCE_FLAG_NONE,
                                                      0);  //upgradeRetryCount

            if(status != FBE_STATUS_OK)
            {
                fbe_base_object_trace((fbe_base_object_t *)pModuleMgmt,
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
 * @fn fbe_module_mgmt_fup_restart_env_failed_fup(fbe_module_mgmt_t * pModuleMgmt,
                                                  fbe_device_physical_location_t * pLocation,
                                                  fbe_lcc_info_t * pNewLccInfo)
 ****************************************************************
 * @brief
 *  When peer BEM is faulted it will prevent local BEM from getting
 *  upgraded. When the peer fault clears we need to come through
 *  here and check if the failed upgrade (on the local side) needs to
 *  be restarted.
 * 
 * @param fbe_module_mgmt_t module_mgmt object
 * @param pLccInfo - The lcc info for the status change
 * @param pLocation - sp, slot for the bem
 * 
 * @return void
 *
 * @version
 *  3-Mar-2015:  GB - Created. 
 ****************************************************************/
static void fbe_module_mgmt_fup_restart_env_failed_fup(fbe_module_mgmt_t * pModuleMgmt,
                                                       fbe_device_physical_location_t * pLocation,
                                                       fbe_lcc_info_t * pLccInfo)
{
    fbe_base_environment_t          * pBaseEnv = (fbe_base_environment_t *)pModuleMgmt; 
    fbe_io_module_physical_info_t   * pLocalBemInfo = NULL;
    fbe_u8_t                        savedSpid;
    fbe_u8_t                        savedSlot;
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_char_t                      logHeader[FBE_BASE_ENV_FUP_MAX_LOG_HEADER_SIZE + 1] = {0};

    // Set log header for tracing purpose
    fbe_module_mgmt_fup_generate_logheader(pLocation, &logHeader[0], FBE_BASE_ENV_FUP_MAX_LOG_HEADER_SIZE);
    fbe_base_object_trace((fbe_base_object_t *)pModuleMgmt,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                              "%s entry\n",
                              __FUNCTION__);

    // if this is the local sp, no need to check anything, just return
    if(pBaseEnv->spid == pLocation->sp)
    {
        return;
    }

    // save the location info that we'll change
    savedSpid = pLocation->sp;
    savedSlot = pLocation->slot;

    // set the location for the local info and get the local info
    if(pLocation->sp == 0)
    {
        pLocation->sp = 1;
    }
    else
    {
        pLocation->sp = 0;
    }
    pLocation->slot = 0;

    status = fbe_module_mgmt_get_bem_info_ptr(pModuleMgmt, pLocation, &pLocalBemInfo);
    if(status != FBE_STATUS_OK)
    {
        // bug - Cannot find the enclosureIndex. This is coding error.
        fbe_base_object_trace((fbe_base_object_t*)pModuleMgmt,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s, get bem(%d) info failed.\n",
                              __FUNCTION__,
                              pLocation->sp);
        pLocation->sp = savedSpid;
        pLocation->slot = savedSlot;
        return;
    }
    // if peer bem is inserted and no fault
    if(pLccInfo->inserted && !pLccInfo->faulted)
    {
        // check for failed fup on local bem
        if(pLocalBemInfo->module_fup_info.completionStatus == FBE_BASE_ENV_FUP_COMPLETION_STATUS_FAIL_BAD_ENV_STATUS)
        {
            fbe_base_object_trace((fbe_base_object_t *)pModuleMgmt,
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  " Restart the Failed Bad env Status FUP:BM(SP:%d,Slot:%d)\n",
                                  pLocation->sp,
                                  pLocation->slot);

            // restart the upgrade
            status = fbe_module_mgmt_fup_initiate_upgrade(pModuleMgmt, 
                                                          FBE_DEVICE_TYPE_BACK_END_MODULE, 
                                                          pLocation,
                                                          FBE_BASE_ENV_FUP_FORCE_FLAG_NONE, //pBemInfo->module_fup_info.forceFlags,
                                                          0);
            if(status != FBE_STATUS_OK)
            {
                fbe_base_object_trace((fbe_base_object_t *)pModuleMgmt,
                                      FBE_TRACE_LEVEL_WARNING,
                                      FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                      "Restart_env_failed_fup, Failed to initiate upgrade for BM(SP:%d,Slot:%d), status 0x%X.\n",
                                      pLocation->sp,
                                      pLocation->slot,
                                      status);
            }
       }
    }

    pLocation->sp = savedSpid;
    pLocation->slot = savedSlot;

    return;
} //fbe_module_mgmt_fup_restart_env_failed_fup

/*!**************************************************************
 * @fn fbe_module_mgmt_fup_policy_allowed(fbe_module_mgmt_t * pModuleMgmt,
 *                                      fbe_device_physical_location_t * pLocation,
 *                                      fbe_bool_t * pPolicyAllowed)
 ****************************************************************
 * @brief
 *  This function checks whether the Base Module is qualified to be upgraded or not.
 *
 * @param pModuleMgmt - The current encl info.
 * @param pLocation -
 * @param pPolicyAllowed(OUTPUT) -
 * 
 * @return fbe_status_t - always return FBE_STATUS_OK;
 *
 * @version
 *  29-Oct-2012:  Rui Chang - Created. 
 *
 ****************************************************************/
static fbe_status_t 
fbe_module_mgmt_fup_policy_allowed(fbe_module_mgmt_t * pModuleMgmt, 
                                 fbe_device_physical_location_t * pLocation,
                                 fbe_bool_t * pPolicyAllowed)
{
    fbe_base_environment_t     * pBaseEnv = (fbe_base_environment_t *)pModuleMgmt; 
    fbe_char_t                   logHeader[FBE_BASE_ENV_FUP_MAX_LOG_HEADER_SIZE + 1] = {0};

    if(pLocation->sp >= SP_ID_MAX)
    {
        fbe_base_object_trace((fbe_base_object_t *)pModuleMgmt,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "FUP: %s, Invalid SP ID 0x%X.\n",
                              __FUNCTION__,
                              pLocation->sp);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Set log header for tracing purpose. */
    fbe_module_mgmt_fup_generate_logheader(pLocation, &logHeader[0], FBE_BASE_ENV_FUP_MAX_LOG_HEADER_SIZE);

    *pPolicyAllowed = FALSE;

    if(pBaseEnv->spid == pLocation->sp)
    {
        *pPolicyAllowed = TRUE;
    }
    else
    {
        fbe_base_object_trace((fbe_base_object_t *)pModuleMgmt,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s, spid:%d Policy Check Fail\n",
                              &logHeader[0],
                              pBaseEnv->spid);
    }

    return FBE_STATUS_OK;
}

 /*!**************************************************************
 * @fn fbe_module_mgmt_fup_bem_qualified(fbe_module_mgmt_t * pModuleMgmt,
 *                                     fbe_device_physical_location_t * pLocation,
 *                                     fbe_board_mgmt_io_comp_info_t * pBemInfo,
 *                                     fbe_bool_t * pUpgradeQualified)
 ****************************************************************
 * @brief
 *  This function checks whether the BEM is qualified to the upgrade or not.
 * 
 * @param pModuleMgmt -
 * @param pLocation -
 * @param pBemInfo - The BEM info from the physical package.
 * @param pUpgradeQualified(OUTPUT) - whether the BEM
 *                        is qualified to do the upgrade or not.
 * 
 * @return fbe_status_t - always return FBE_STATUS_OK;
 *
 * @version
 *  29-Oct-2012:  Rui Chang - Created. 
 *
 ****************************************************************/
static fbe_status_t fbe_module_mgmt_fup_bem_qualified(fbe_module_mgmt_t * pModuleMgmt,
                                                    fbe_device_physical_location_t * pLocation,
                                                    fbe_lcc_info_t * pLccInfo,
                                                    fbe_bool_t * pUpgradeQualified)
{
    fbe_char_t                     logHeader[FBE_BASE_ENV_FUP_MAX_LOG_HEADER_SIZE + 1] = {0};

    if(pLocation->sp >= SP_ID_MAX)
    {
        fbe_base_object_trace((fbe_base_object_t *)pModuleMgmt,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "FUP: %s, Invalid SP ID 0x%X.\n",
                              __FUNCTION__,
                              pLocation->sp);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Set log header for tracing purpose. */
    fbe_module_mgmt_fup_generate_logheader(pLocation, &logHeader[0], FBE_BASE_ENV_FUP_MAX_LOG_HEADER_SIZE);

    *pUpgradeQualified = (pLccInfo->inserted && !pLccInfo->faulted && pLccInfo->bDownloadable);
   
    if(*pUpgradeQualified != TRUE)
    {
        fbe_base_object_trace((fbe_base_object_t *)pModuleMgmt,
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
 * @fn fbe_module_mgmt_get_bem_info_ptr(void * pContext,
 *                                         fbe_u64_t deviceType,
 *                                         fbe_device_physical_location_t * pLocation,
 *                                         fbe_io_module_physical_info_t ** pBemInfoPtr)
 ****************************************************************
 * @brief
 *  This function gets the pointer which points to the bem info
 * 
 * @param pContext - The pointer to the callback context.
 * @param pLocation - Location of the device.
 * @param pBemInfoPtr - Pointer to the Bem information.
 *
 * @return fbe_status_t
 *
 * @version
 *  29-Oct-2012:  Rui Chang - Created. 
 *
 ****************************************************************/
fbe_status_t fbe_module_mgmt_get_bem_info_ptr(fbe_module_mgmt_t * pModuleMgmt,
                                                      fbe_device_physical_location_t * pLocation,
                                                      fbe_io_module_physical_info_t ** pBemInfoPtr)
{
    fbe_u32_t                                module_num = 0;

    *pBemInfoPtr = NULL;

    module_num = fbe_module_mgmt_convert_device_type_and_slot_to_index(FBE_DEVICE_TYPE_BACK_END_MODULE, pLocation->slot);

    /* Copy the data from the work item to the appropriate data structure */
    *pBemInfoPtr = &pModuleMgmt->io_module_info[module_num].physical_info[pLocation->sp];
    
    return FBE_STATUS_OK;
} // End of function fbe_module_mgmt_get_bem_info_ptr


/*!**************************************************************
 * @fn fbe_module_mgmt_fup_initiate_upgrade(void * pContext, 
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
 * @param upgradeRetryCount -
 * 
 * @return fbe_status_t
 *
 * @version
 *  29-Oct-2012:  Rui Chang - Created. 
 *
 ****************************************************************/
fbe_status_t fbe_module_mgmt_fup_initiate_upgrade(void * pContext, 
                                              fbe_u64_t deviceType,
                                              fbe_device_physical_location_t * pLocation,
                                              fbe_u32_t forceFlags,
                                              fbe_u32_t upgradeRetryCount)
{
    fbe_module_mgmt_t            * pModuleMgmt = (fbe_module_mgmt_t *)pContext;
    fbe_io_module_physical_info_t  * pBemInfo = NULL;
    fbe_bool_t                   policyAllowUpgrade = FALSE;
    fbe_bool_t                   upgradeQualified = FALSE;
    fbe_char_t                   logHeader[FBE_BASE_ENV_FUP_MAX_LOG_HEADER_SIZE + 1] = {0};
    fbe_status_t                 status = FBE_STATUS_OK;

    if(deviceType != FBE_DEVICE_TYPE_BACK_END_MODULE)
    {
        fbe_base_object_trace((fbe_base_object_t *)pModuleMgmt,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "FUP: %s, unsupported deviceType 0x%llX.\n",
                              __FUNCTION__,
                              deviceType);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    if(pLocation->sp >= SP_ID_MAX)
    {
        fbe_base_object_trace((fbe_base_object_t *)pModuleMgmt,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "FUP: %s, Invalid SP ID 0x%X.\n",
                              __FUNCTION__,
                              pLocation->sp);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Set log header for tracing purpose. */
    fbe_module_mgmt_fup_generate_logheader(pLocation, &logHeader[0], FBE_BASE_ENV_FUP_MAX_LOG_HEADER_SIZE);

    fbe_module_mgmt_fup_policy_allowed(pModuleMgmt, pLocation, &policyAllowUpgrade);
    if(!policyAllowUpgrade)
    {
        fbe_base_object_trace((fbe_base_object_t *)pModuleMgmt,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s, %s, policy does not allow upgrade.\n",
                              &logHeader[0],
                              __FUNCTION__);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_module_mgmt_get_bem_info_ptr(pModuleMgmt, pLocation, &pBemInfo);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)pModuleMgmt,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s, %s, get BM info ptr failed.\n",
                              &logHeader[0],
                              __FUNCTION__);

        return status;
    }
    
    fbe_module_mgmt_fup_bem_qualified(pModuleMgmt, pLocation, &pBemInfo->module_comp_info.lccInfo, &upgradeQualified);
    if(!upgradeQualified)
    {
        fbe_base_object_trace((fbe_base_object_t *)pModuleMgmt,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s, %s, not qualified for upgrade.\n",
                              &logHeader[0],
                              __FUNCTION__);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    if(pBemInfo->module_comp_info.lccInfo.eses_version_desc == ESES_REVISION_2_0)
    {
        status = fbe_module_mgmt_fup_create_and_add_work_item_from_manifest(pModuleMgmt,                                           
                                                                 pLocation,
                                                                 FBE_DEVICE_TYPE_BACK_END_MODULE,
                                                                 &pBemInfo->module_comp_info.lccInfo,
                                                                 forceFlags,
                                                                 &logHeader[0],
                                                                 0,  // There is no interDeviceDelay for LCC FUP.
                                                                 upgradeRetryCount);   

        if(status != FBE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t *)pModuleMgmt,
                                  FBE_TRACE_LEVEL_WARNING,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s, Failed to create_and_add_work_item from manifest, status 0x%X.\n",
                                  &logHeader[0], status);
        }
    }
    else
    {
        status = fbe_base_env_fup_create_and_add_work_item((fbe_base_environment_t *)pModuleMgmt,
                                                   pLocation,
                                                   FBE_DEVICE_TYPE_BACK_END_MODULE,
                                                   FBE_FW_TARGET_LCC_MAIN,
                                                   &pBemInfo->module_comp_info.lccInfo.firmwareRev[0],
                                                   &pBemInfo->module_comp_info.lccInfo.subenclProductId[0],
                                                   pBemInfo->module_comp_info.lccInfo.uniqueId,
                                                   forceFlags,
                                                   &logHeader[0],
                                                   0,  // There is no interDeviceDelay for BM FUP.
                                                   upgradeRetryCount,
                                                   pBemInfo->module_comp_info.lccInfo.eses_version_desc);   
    
        if(status != FBE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t *)pModuleMgmt,
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
 * @fn fbe_module_mgmt_fup_get_full_image_path
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
 *  29-Oct-2012 Rui Chang -- Created.
 *******************************************************************************/
fbe_status_t fbe_module_mgmt_fup_get_full_image_path(void * pContext,                               
                               fbe_base_env_fup_work_item_t * pWorkItem)
{
    fbe_base_environment_t * pBaseEnv =(fbe_base_environment_t *)pContext;
    fbe_module_mgmt_t      * pModuleMgmt =(fbe_module_mgmt_t *)pContext;
    fbe_char_t imageFileName[FBE_BASE_ENV_FUP_MAX_IMAGE_FILE_NAME_LEN] = {0};
    fbe_u32_t imageFileNameLen = 0;
    fbe_char_t imageFileConstantPortionName[FBE_BASE_ENV_FUP_MAX_IMAGE_FILE_NAME_LEN] = {0};
    fbe_char_t imagePathKey[FBE_BASE_ENV_FUP_MAX_IMAGE_PATH_KEY_LEN] = {0};
    fbe_u32_t imagePathIndex = 0;
    fbe_status_t   status = FBE_STATUS_OK;
    fbe_char_t *pImageFileName = NULL;

    if (pWorkItem->uniqueId == JETFIRE_SP_SANDYBRIDGE)
    {
        imagePathIndex = 0;

        fbe_copy_memory(&imageFileConstantPortionName[0], 
                        FBE_MODULE_MGMT_JDES_FUP_IMAGE_FILE_NAME,
                        sizeof(FBE_MODULE_MGMT_JDES_FUP_IMAGE_FILE_NAME));

        fbe_copy_memory(&imagePathKey[0], 
                        FBE_MODULE_MGMT_JDES_FUP_IMAGE_PATH_KEY,
                        sizeof(FBE_MODULE_MGMT_JDES_FUP_IMAGE_PATH_KEY));
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

        if(strncmp((char *)pImageFileName, FBE_MODULE_MGMT_CDES2_FUP_CDEF_IMAGE_FILE_NAME, sizeof(FBE_MODULE_MGMT_CDES2_FUP_CDEF_IMAGE_FILE_NAME)-1) == 0) 
        {
            /* pImageFileName is cdef.bin.  
             * Do not need to copy .bin because in simulation, the file name will be cdef_pidxxxx.bin. 
             * sizeof(FBE_MODULE_MGMT_CDES2_FUP_CDEF_IMAGE_FILE_NAME) is 5 but we only want to compare with "cdef" only. So we have to minus 1.
             */
            imagePathIndex = 1;

            fbe_copy_memory(&imageFileConstantPortionName[0], 
                        FBE_MODULE_MGMT_CDES2_FUP_CDEF_IMAGE_FILE_NAME,
                        sizeof(FBE_MODULE_MGMT_CDES2_FUP_CDEF_IMAGE_FILE_NAME));
        }
        else if((strncmp((char *)pImageFileName, FBE_MODULE_MGMT_CDES2_FUP_CDEF_DUAL_IMAGE_FILE_NAME, sizeof(FBE_MODULE_MGMT_CDES2_FUP_CDEF_DUAL_IMAGE_FILE_NAME)-1) == 0)) 
        {
            /* pImageFileName is cdef_dual.bin.  
             * Do not need to copy .bin because in simulation, the file name will be cdef_dual_pidxxxx.bin.
             */ 
             
            imagePathIndex = 2;

            fbe_copy_memory(&imageFileConstantPortionName[0], 
                        FBE_MODULE_MGMT_CDES2_FUP_CDEF_DUAL_IMAGE_FILE_NAME,
                        sizeof(FBE_MODULE_MGMT_CDES2_FUP_CDEF_DUAL_IMAGE_FILE_NAME));
        }
        else if(strncmp((char *)pImageFileName, FBE_MODULE_MGMT_CDES2_FUP_ISTR_IMAGE_FILE_NAME, sizeof(FBE_MODULE_MGMT_CDES2_FUP_ISTR_IMAGE_FILE_NAME)-1) == 0)
        {
            /* pImageFileName is istr.bin.  
             * Do not need to copy .bin because in simulation, the file name will be istr_pidxxxx.bin. 
             */
            imagePathIndex = 3;

            fbe_copy_memory(&imageFileConstantPortionName[0], 
                        FBE_MODULE_MGMT_CDES2_FUP_ISTR_IMAGE_FILE_NAME,
                        sizeof(FBE_MODULE_MGMT_CDES2_FUP_ISTR_IMAGE_FILE_NAME));
        }
        else if(strncmp((char *)pImageFileName, FBE_MODULE_MGMT_CDES2_FUP_CDES_ROM_IMAGE_FILE_NAME, sizeof(FBE_MODULE_MGMT_CDES2_FUP_CDES_ROM_IMAGE_FILE_NAME)-1) == 0)
        {
            /* pImageFileName is cdes_rom.bin.  
             * Do not need to copy .bin because in simulation, the file name will be cdes_rom_pidxxxx.bin. 
             */
            imagePathIndex = 4;

            fbe_copy_memory(&imageFileConstantPortionName[0], 
                        FBE_MODULE_MGMT_CDES2_FUP_CDES_ROM_IMAGE_FILE_NAME,
                        sizeof(FBE_MODULE_MGMT_CDES2_FUP_CDES_ROM_IMAGE_FILE_NAME));
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
                        FBE_MODULE_MGMT_CDES2_FUP_IMAGE_PATH_KEY,
                        sizeof(FBE_MODULE_MGMT_CDES2_FUP_IMAGE_PATH_KEY));

    }
    else
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_module_mgmt_fup_build_image_file_name(pModuleMgmt,
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
 * @fn fbe_module_mgmt_fup_get_manifest_file_full_path
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
fbe_status_t fbe_module_mgmt_fup_get_manifest_file_full_path(void * pContext,
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
                                                       FBE_MODULE_MGMT_CDES2_FUP_MANIFEST_FILE_NAME,  /* It does not have .json */
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
                    FBE_MODULE_MGMT_CDES2_FUP_MANIFEST_FILE_PATH_KEY,
                    sizeof(FBE_MODULE_MGMT_CDES2_FUP_MANIFEST_FILE_PATH_KEY));

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
 * @fn fbe_module_mgmt_fup_check_env_status(void * pContext, 
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
 *  29-Oct-2012 Rui Chang -- Created. 
 *
 ****************************************************************/
fbe_status_t fbe_module_mgmt_fup_check_env_status(void * pContext,
                                 fbe_base_env_fup_work_item_t * pWorkItem)
{
    fbe_module_mgmt_t                * pModuleMgmt = (fbe_module_mgmt_t *)pContext;
    fbe_device_physical_location_t  location = {0};
    fbe_char_t                   logHeader[FBE_BASE_ENV_FUP_MAX_LOG_HEADER_SIZE + 1] = {0};
    fbe_io_module_physical_info_t  * pPeerBemInfo = NULL;
    fbe_status_t                     status = FBE_STATUS_OK;

    if (pWorkItem->deviceType == FBE_DEVICE_TYPE_BACK_END_MODULE)
    {
        location.sp = (pModuleMgmt->base_environment.spid == 0)? 1:0;
        location.slot = 0;

        /* Set log header for tracing purpose. */
        fbe_module_mgmt_fup_generate_logheader(&location, &logHeader[0], FBE_BASE_ENV_FUP_MAX_LOG_HEADER_SIZE);
    
        status = fbe_module_mgmt_get_bem_info_ptr(pModuleMgmt, &location, &pPeerBemInfo);
        if(status != FBE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t *)pModuleMgmt,
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s, %s, get peer BM info ptr failed.\n",
                                  &logHeader[0],
                                  __FUNCTION__);
    
            return FBE_STATUS_GENERIC_FAILURE;
        }
    
        if((pPeerBemInfo->module_comp_info.lccInfo.inserted == TRUE) &&
           (pPeerBemInfo->module_comp_info.lccInfo.faulted == FBE_MGMT_STATUS_FALSE))
        {
            return FBE_STATUS_OK;   
        }
    
        return FBE_STATUS_GENERIC_FAILURE; 
    }
    else
    {
        return FBE_STATUS_OK;   
    }
 
} // End of function fbe_module_mgmt_fup_check_env_status


/*!**************************************************************
 * @fn fbe_module_mgmt_fup_get_firmware_rev(void * pContext,
 *                         fbe_base_env_fup_work_item_t * pWorkItem,
 *                         fbe_u8_t * pPsFirmwareRev)
 ****************************************************************
 * @brief
 *  This function gets BEM firmware rev.
 *
 * @param context - The pointer to the call back context.
 * @param pWorkItem - The pointer to the work item.
 * @param pFirmwareRev(OUTPUT) - The pointer to the new firmware rev. 
 *
 * @return fbe_status_t
 *
 * @version
 *  29-Oct-2012:  Rui Chang - Created. 
 *
 ****************************************************************/
fbe_status_t fbe_module_mgmt_fup_get_firmware_rev(void * pContext,
                          fbe_base_env_fup_work_item_t * pWorkItem,
                          fbe_u8_t * pFirmwareRev)
{
    fbe_module_mgmt_t * pModuleMgmt = (fbe_module_mgmt_t *)pContext;
    fbe_lcc_info_t lccInfo = {0};
    fbe_device_physical_location_t * pLocation = NULL;
    fbe_device_physical_location_t location = {0};
    fbe_status_t status = FBE_STATUS_OK;

    if(pWorkItem->deviceType != FBE_DEVICE_TYPE_BACK_END_MODULE)
    {
        /* The is the coding error. */
        fbe_base_object_customizable_trace((fbe_base_object_t*)pModuleMgmt,
                                           FBE_TRACE_LEVEL_ERROR,
                                           fbe_base_env_get_fup_work_item_logheader(pWorkItem),
                                           "%s, unsupported deviceType 0x%llX.\n",
                                           __FUNCTION__,
                                           pWorkItem->deviceType);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    pLocation = fbe_base_env_get_fup_work_item_location_ptr(pWorkItem);
    
    fbe_copy_memory(&location,
                    pLocation, 
                    sizeof(fbe_device_physical_location_t));

    //we may mark slot as 0 for SPB BEM, but when try to get lcc info for that BEM, we need slot as 1.
    location.slot = pLocation->sp;

    status = fbe_api_enclosure_get_lcc_info(&location, &lccInfo);
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
 *  @fn fbe_module_mgmt_get_lcc_fup_info_ptr(void * pContext,
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
 *  29-Oct-2012:  Rui Chang - Created. 
 *
 ****************************************************************/
fbe_status_t fbe_module_mgmt_get_fup_info_ptr(void * pContext,
                                            fbe_u64_t deviceType,
                                            fbe_device_physical_location_t * pLocation,
                                            fbe_base_env_fup_persistent_info_t ** pFupInfoPtr)
{
    fbe_module_mgmt_t                        * pModuleMgmt = (fbe_module_mgmt_t *)pContext;
    fbe_io_module_physical_info_t  * pBemInfo = NULL;
    fbe_status_t                             status = FBE_STATUS_OK;

    *pFupInfoPtr = NULL;

    if(deviceType != FBE_DEVICE_TYPE_BACK_END_MODULE)
    {
        fbe_base_object_trace((fbe_base_object_t*)pModuleMgmt, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s, unsupported deviceType 0x%llX.\n",
                              __FUNCTION__,
                              deviceType);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    if(pLocation->sp >= SP_ID_MAX)
    {
        fbe_base_object_trace((fbe_base_object_t *)pModuleMgmt,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "FUP: %s, Invalid SP ID 0x%X.\n",
                              __FUNCTION__,
                              pLocation->sp);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_module_mgmt_get_bem_info_ptr(pModuleMgmt, pLocation, &pBemInfo);

    if (status != FBE_STATUS_OK) 
    {
        return status;
    }

    *pFupInfoPtr = &pBemInfo->module_fup_info;
    return FBE_STATUS_OK;
}

/*!**************************************************************
 *  @fn fbe_module_mgmt_get_fup_info(void * pContext,
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
 *  29-Oct-2012:  Rui Chang - Created. 
 *
 ****************************************************************/
fbe_status_t fbe_module_mgmt_get_fup_info(void * pContext,
                                            fbe_u64_t deviceType,
                                            fbe_device_physical_location_t * pLocation,
                                            fbe_esp_base_env_get_fup_info_t *  pGetFupInfo)
{
    fbe_module_mgmt_t                      * pModuleMgmt = (fbe_module_mgmt_t *)pContext;
    fbe_io_module_physical_info_t          * pBemInfo = NULL;
    fbe_status_t                             status = FBE_STATUS_OK;
    fbe_base_env_fup_work_item_t           * pWorkItem = NULL;

    if(deviceType != FBE_DEVICE_TYPE_BACK_END_MODULE)
    {
        fbe_base_object_trace((fbe_base_object_t*)pModuleMgmt, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s, unsupported deviceType 0x%llX.\n",
                              __FUNCTION__,
                              deviceType);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    if(pLocation->sp >= SP_ID_MAX)
    {
        fbe_base_object_trace((fbe_base_object_t *)pModuleMgmt,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "FUP: %s, Invalid SP ID 0x%X.\n",
                              __FUNCTION__,
                              pLocation->sp);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_module_mgmt_get_bem_info_ptr(pModuleMgmt, pLocation, &pBemInfo);

    if (status != FBE_STATUS_OK) 
    {
        return status;
    }

    pGetFupInfo->programmableCount = FBE_MODULE_MGMT_MAX_PROGRAMMABLE_COUNT_PER_BEM_SLOT;

    pGetFupInfo->fupInfo[0].workState = FBE_BASE_ENV_FUP_WORK_STATE_NONE;
    status = fbe_base_env_fup_find_work_item((fbe_base_environment_t *)pContext, FBE_DEVICE_TYPE_BACK_END_MODULE, pLocation, &pWorkItem);
    if(pWorkItem != NULL)
    {
        pGetFupInfo->fupInfo[0].workState = pWorkItem->workState;

        if (pGetFupInfo->fupInfo[0].workState == FBE_BASE_ENV_FUP_WORK_STATE_WAIT_BEFORE_UPGRADE)  
        {
            pGetFupInfo->fupInfo[0].waitTimeBeforeUpgrade = fbe_base_env_get_wait_time_before_upgrade((fbe_base_environment_t *)pContext);
        }
    }

    pGetFupInfo->fupInfo[0].completionStatus = pBemInfo->module_fup_info.completionStatus;
    fbe_copy_memory(&pGetFupInfo->fupInfo[0].imageRev[0],
                            &pBemInfo->module_fup_info.imageRev[0],
                            FBE_ENCLOSURE_MAX_FIRMWARE_REV_SIZE + 1);
    fbe_copy_memory(&pGetFupInfo->fupInfo[0].currentFirmwareRev[0],
                            &pBemInfo->module_fup_info.currentFirmwareRev[0],
                            FBE_ENCLOSURE_MAX_FIRMWARE_REV_SIZE + 1);
    fbe_copy_memory(&pGetFupInfo->fupInfo[0].preFirmwareRev[0],
                            &pBemInfo->module_fup_info.preFirmwareRev[0],
                            FBE_ENCLOSURE_MAX_FIRMWARE_REV_SIZE + 1);

    return FBE_STATUS_OK;

}


/*!***************************************************************
 * fbe_module_mgmt_fup_resume_upgrade()
 ****************************************************************
 * @brief
 *  This function resumes the ugrade for all the devices which were aborted in this class.
 *
 * @param pContext -
 *
 * @return fbe_status_t
 *
 * @author
 *  29-Oct-2012: Rui Chang - Created. 
 *
 ****************************************************************/
fbe_status_t fbe_module_mgmt_fup_resume_upgrade(void * pContext)
{
    fbe_module_mgmt_t                        * pModuleMgmt = (fbe_module_mgmt_t *)pContext;
    fbe_io_module_physical_info_t  * pBemInfo = NULL;
    fbe_device_physical_location_t           location = {0};
    fbe_status_t                             status = FBE_STATUS_OK;
    fbe_char_t                   logHeader[FBE_BASE_ENV_FUP_MAX_LOG_HEADER_SIZE + 1] = {0};

    fbe_base_object_trace((fbe_base_object_t *)pModuleMgmt,
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "FUP: Resume upgrade for BM.\n");
   
    /* Currently we only deal with BEM upgrade */
    location.sp = pModuleMgmt->base_environment.spid;
    location.bus = 0;
    location.enclosure = 0;
    location.slot = 0;

    /* Set log header for tracing purpose. */
    fbe_module_mgmt_fup_generate_logheader(&location, &logHeader[0], FBE_BASE_ENV_FUP_MAX_LOG_HEADER_SIZE);


    status = fbe_module_mgmt_get_bem_info_ptr(pModuleMgmt, &location, &pBemInfo);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)pModuleMgmt,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s, %s, get BM info ptr failed.\n",
                              &logHeader[0],
                              __FUNCTION__);

        return status;
    }

    if(pBemInfo->module_fup_info.completionStatus == 
                         FBE_BASE_ENV_FUP_COMPLETION_STATUS_ABORTED)
    { 
        status = fbe_module_mgmt_fup_initiate_upgrade(pModuleMgmt, 
                                                      FBE_DEVICE_TYPE_BACK_END_MODULE, 
                                                      &location,
                                                      pBemInfo->module_fup_info.forceFlags,
                                                      0);  // upgradeRetryCount
        if(status != FBE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t *)pModuleMgmt,
                                              FBE_TRACE_LEVEL_WARNING,
                                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                              "FUP:BM(SP %d, Slot %d), %s, Failed to initiate upgrade, status 0x%X.\n",
                                              location.sp,
                                              location.slot,
                                              __FUNCTION__,
                                              status);
        }
    }

    return FBE_STATUS_OK;
} /* End of function fbe_module_mgmt_fup_resume_upgrade */

/*!***************************************************************
 * fbe_module_mgmt_fup_new_contact_init_upgrade()
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
 * @param pBaseEnv -pointer to base environment struct
 *      deviceType - specifies the device type (LCC, PS, etc)
 *      pPeerLocation - physical location of the requestng device
 * @return fbe_status_t
 *
 * @author
 *  29-Oct-2012: Rui Chang - Created. 
 *
 ****************************************************************/
fbe_status_t fbe_module_mgmt_fup_new_contact_init_upgrade(fbe_module_mgmt_t *pModuleMgmt,
                                                         fbe_u64_t deviceType)
{
    fbe_status_t                            status;
    fbe_device_physical_location_t          location={0};
    fbe_base_env_fup_persistent_info_t    * pFupInfo = NULL;
    fbe_char_t                              logHeader[FBE_BASE_ENV_FUP_MAX_LOG_HEADER_SIZE + 1] = {0};


    // Peer contact back, update local BEM FW. 
    fbe_base_object_trace((fbe_base_object_t *)pModuleMgmt,
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s peer SP contact is back.\n",
                                  __FUNCTION__);

    location.sp = pModuleMgmt->base_environment.spid;
    location.bus = 0;
    location.enclosure = 0;
    location.slot = 0;

    /* Set log header for tracing purpose. */
    fbe_module_mgmt_fup_generate_logheader(&location, &logHeader[0], FBE_BASE_ENV_FUP_MAX_LOG_HEADER_SIZE);

    // Check the fup status for no permission
    // because the peer was not present.
    status = fbe_module_mgmt_get_fup_info_ptr(pModuleMgmt, deviceType, &location, &pFupInfo);

    if((status == FBE_STATUS_OK) &&
       (pFupInfo->completionStatus == FBE_BASE_ENV_FUP_COMPLETION_STATUS_FAIL_NO_PEER_PERMISSION))
    {
        fbe_base_object_trace((fbe_base_object_t *)pModuleMgmt,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s, %s, New FUP Attempt,Peer now Present\n",
                              &logHeader[0],
                              __FUNCTION__);
        // We arrived here because the peer has contacted us for permission to upgrade.
        // If we had a fup halted due to no peer present start a new fup attempt.
        status = fbe_module_mgmt_fup_initiate_upgrade(pModuleMgmt, 
                                                    deviceType, 
                                                    &location,
                                                    pFupInfo->forceFlags,
                                                    0);  //upgradeRetryCount
    }

    return status;
} //fbe_module_mgmt_fup_new_contact_init_upgrade

/*!**************************************************************
 * @fn fbe_module_mgmt_fup_copy_fw_rev_to_resume_prom(fbe_module_mgmt_t *pModuleMgmt,
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
 *  29-Oct-2012:  Rui Chang- Created. 
 *
 ****************************************************************/
fbe_status_t fbe_module_mgmt_fup_copy_fw_rev_to_resume_prom(fbe_module_mgmt_t *pModuleMgmt,
                                                        fbe_u64_t deviceType,
                                                        fbe_device_physical_location_t *pLocation)
{
    fbe_char_t                             logHeader[FBE_BASE_ENV_FUP_MAX_LOG_HEADER_SIZE + 1] = {0};
    fbe_status_t                           status = FBE_STATUS_OK;
    fbe_base_env_resume_prom_info_t      * pResumePromInfo = NULL;
    fbe_io_module_physical_info_t        * pBemInfo = NULL;
    fbe_u32_t                              programmableIndex = 0;
    fbe_u8_t                               fwRevForResume[RESUME_PROM_PROG_REV_SIZE] = {0};
    fbe_bool_t                             cdesFwAdded=FBE_FALSE;

    // We only copy Base Module revision from status to resume
    if (deviceType != FBE_DEVICE_TYPE_BACK_END_MODULE)
    {
        return FBE_STATUS_OK;
    }

    if(pLocation->sp >= SP_ID_MAX)
    {
        fbe_base_object_trace((fbe_base_object_t *)pModuleMgmt,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "FUP: %s, Invalid SP ID 0x%X.\n",
                              __FUNCTION__,
                              pLocation->sp);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_module_mgmt_fup_generate_logheader(pLocation, &logHeader[0], FBE_BASE_ENV_FUP_MAX_LOG_HEADER_SIZE);

    status = fbe_module_mgmt_get_resume_prom_info_ptr(pModuleMgmt, 
                                                  deviceType, 
                                                  pLocation, 
                                                  &pResumePromInfo);

    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)pModuleMgmt,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s, %s, get_resume_prom_info_ptr failed.\n",
                              &logHeader[0], __FUNCTION__);

        return status;
    }

    status = fbe_module_mgmt_get_bem_info_ptr(pModuleMgmt, pLocation, &pBemInfo);

    if (status != FBE_STATUS_OK) 
    {
        fbe_base_object_trace((fbe_base_object_t *)pModuleMgmt,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s, %s, get_bem_info_ptr failed.\n",
                              &logHeader[0], __FUNCTION__);

        return status;
    }

    if(deviceType == FBE_DEVICE_TYPE_BACK_END_MODULE)
    {
        // check if CDESf firmware info is already added in resume.
        for(programmableIndex=0; programmableIndex < pResumePromInfo->resume_prom_data.num_prog[0]; programmableIndex++)
        {
            if (strncmp(pResumePromInfo->resume_prom_data.prog_details[programmableIndex].prog_name, "CDES", 4) == 0)
            {
              cdesFwAdded=FBE_TRUE;
              break;
            } 
        }

        if(cdesFwAdded == FBE_FALSE) 
        {
            if(pResumePromInfo->resume_prom_data.num_prog[0] == 0xFF) 
            {
                /* The resume prom is not programmed correctly. Handle it as follow so it won't cause the memory corruption. */
                programmableIndex = 0;
                pResumePromInfo->resume_prom_data.num_prog[0] = 1;
            }
            else if(pResumePromInfo->resume_prom_data.num_prog[0] >= RESUME_PROM_MAX_PROG_COUNT) 
            {
                fbe_base_object_trace((fbe_base_object_t *)pModuleMgmt,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s, invalid programmable count %d in resume, max count %d.\n",
                              &logHeader[0], 
                              pResumePromInfo->resume_prom_data.num_prog[0],
                              RESUME_PROM_MAX_PROG_COUNT);

                return FBE_STATUS_GENERIC_FAILURE;
            }
            else
            {
                programmableIndex = pResumePromInfo->resume_prom_data.num_prog[0];
                pResumePromInfo->resume_prom_data.num_prog[0] = pResumePromInfo->resume_prom_data.num_prog[0] + 1;
            }

            fbe_copy_memory(pResumePromInfo->resume_prom_data.prog_details[programmableIndex].prog_name,
                            "CDES\0",
                            5);

            fbe_base_env_fup_convert_fw_rev_for_rp((fbe_base_environment_t *)pModuleMgmt,
                                               &fwRevForResume[0],
                                               &pBemInfo->module_comp_info.lccInfo.firmwareRev[0]);

            fbe_copy_memory(pResumePromInfo->resume_prom_data.prog_details[programmableIndex].prog_rev,
                            &fwRevForResume[0],
                            RESUME_PROM_PROG_REV_SIZE);
            
        }
    }
    
    return status;
}

/*!**************************************************************
 * @fn fbe_module_mgmt_fup_create_and_add_work_item_from_manifest(fbe_module_mgmt_t * pEnclMgmt,                                           
 *                                         fbe_device_physical_location_t * pLocation,
 *                                         fbe_u64_t deviceType,
 *                                         fbe_lcc_info_t * pLccInfo,
 *                                         fbe_u32_t forceFlags,
 *                                         fbe_char_t * pLogHeader,
 *                                         fbe_u32_t interDeviceDelayInSec)
 ****************************************************************
 * @brief
 *  This function creates the work items for the Base module LCC upgrade based on the manifest info.
 *
 * @param pModuleMgmt - The pointer to the fbe_base_environment_t.
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
fbe_module_mgmt_fup_create_and_add_work_item_from_manifest(fbe_module_mgmt_t * pModuleMgmt,                                           
                                          fbe_device_physical_location_t * pLocation,
                                          fbe_u64_t deviceType,
                                          fbe_lcc_info_t * pLccInfo,
                                          fbe_u32_t forceFlags,
                                          fbe_char_t * pLogHeader,
                                          fbe_u32_t interDeviceDelayInSec,
                                          fbe_u32_t upgradeRetryCount)
{
    fbe_base_environment_t  * pBaseEnv = (fbe_base_environment_t *)pModuleMgmt;
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

//End of file fbe_module_mgmt_fup.c


