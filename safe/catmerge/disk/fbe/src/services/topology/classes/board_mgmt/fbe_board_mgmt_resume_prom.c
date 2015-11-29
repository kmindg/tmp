/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_board_mgmt_resume_prom.c
 ***************************************************************************
 * @brief
 *  This file contains the implemention for Board management Resume Prom functionality.
 *
 * @version
 *   19-Jan-2011:  Arun S - Created.
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_memory.h"
#include "fbe/fbe_api_board_interface.h"
#include "fbe_base_object.h"
#include "fbe_board_mgmt_private.h"
#include "fbe_base_environment_debug.h"

static fbe_status_t fbe_board_mgmt_resume_prom_generate_logheader(fbe_u64_t deviceType,
                                                           fbe_device_physical_location_t * pLocation, 
                                                           fbe_u8_t * pLogHeader, 
                                                           fbe_u32_t logHeaderSize);

/*!**************************************************************
 * @fn fbe_board_mgmt_initiate_resume_prom_cmd()
 ****************************************************************
 * @brief
 *  This function initiates the async resume cmd.
 * 
 * @param pBoardMgmt -
 * @param deviceType - 
 * @param pLocation - The pointer to the physical location.
 *
 * @return fbe_status_t
 *
 * @version
 *  15-Jul-2011:  PHE - Created. 
 *
 ****************************************************************/
fbe_status_t fbe_board_mgmt_initiate_resume_prom_cmd(fbe_board_mgmt_t * pBoardMgmt,
                                                     fbe_u64_t deviceType,
                                                     fbe_device_physical_location_t * pLocation)
{
    fbe_u8_t                          logHeader[FBE_BASE_ENV_RESUME_PROM_LOG_HEADER_SIZE + 1] = {0};
    fbe_base_env_resume_prom_info_t * pResumePromInfo = NULL;
    fbe_status_t                      status = FBE_STATUS_OK;

    fbe_board_mgmt_get_resume_prom_info_ptr(pBoardMgmt, deviceType, pLocation, &pResumePromInfo);
                                                     
    /* Save the device type and location */
    pResumePromInfo->device_type = deviceType;
    pResumePromInfo->location = *pLocation;
    pResumePromInfo->objectId = pBoardMgmt->object_id;

    /* Set log header for tracing purpose. */
    fbe_board_mgmt_resume_prom_generate_logheader(deviceType, pLocation, &logHeader[0], FBE_BASE_ENV_RESUME_PROM_LOG_HEADER_SIZE);

    status = fbe_base_env_resume_prom_create_and_add_work_item((fbe_base_environment_t *)pBoardMgmt,
                                                               pBoardMgmt->object_id,
                                                               deviceType,
                                                               pLocation, 
                                                               &logHeader[0]);

    return status;
}


/*!**************************************************************
 * @fn fbe_board_mgmt_resume_prom_generate_logheader()
 ****************************************************************
 * @brief
 *  This function sets the log header for the module mgmt components.
 *
 * @param pLocation - The pointer to the physical location.
 * @param pLogHeader(output) - The pointer to the log header.
 * @param logHeaderSize -
 *
 * @return fbe_status_t
 *
 * @version
 *  19-Jan-2011:  Arun S - Created. 
 *
 ****************************************************************/
static fbe_status_t fbe_board_mgmt_resume_prom_generate_logheader(fbe_u64_t deviceType,
                                                           fbe_device_physical_location_t * pLocation, 
                                                           fbe_u8_t * pLogHeader, 
                                                           fbe_u32_t logHeaderSize)
{
    fbe_zero_memory(pLogHeader, logHeaderSize);
    
    if (deviceType == FBE_DEVICE_TYPE_SP)
    {
        csx_p_snprintf(pLogHeader, logHeaderSize, "RP:%s", pLocation->sp == SP_A? "SPA" : "SPB");
    }
    else if (deviceType == FBE_DEVICE_TYPE_CACHE_CARD)
    {
        csx_p_snprintf(pLogHeader, logHeaderSize, "RP:CACHE CARD(%s, Slot %d)", pLocation->sp == SP_A? "SPA" : "SPB", pLocation->slot);
    }

    return FBE_STATUS_OK;
}

/*!**************************************************************
 * @fn fbe_board_mgmt_get_resume_prom_info_ptr(fbe_module_mgmt_t * pModuleMgmt,
 *                                                     fbe_u64_t deviceType,
 *                                                     fbe_device_physical_location_t * pLocation,
 *                                                     fbe_base_env_resume_prom_info_t ** pResumePromInfoPtr)
 ****************************************************************
 * @brief
 *  This function gets the pointer to the resume prom info.
 *
 * @param pContext - The pointer to the callback context.
 * @param deviceType - The PE Component device type.
 * @param pLocation - Location of the device.
 * @param pResumePromInfoPtr - Pointer to the resume prom information.
 *
 * @return fbe_status_t
 *
 * @version
 *  19-Jul-2011: PHE - Created. 
 *
 ****************************************************************/
fbe_status_t fbe_board_mgmt_get_resume_prom_info_ptr(fbe_board_mgmt_t * board_mgmt,
                                                      fbe_u64_t deviceType,
                                                      fbe_device_physical_location_t * pLocation,
                                                      fbe_base_env_resume_prom_info_t ** pResumePromInfoPtr)
{
    *pResumePromInfoPtr = NULL;

    if((deviceType != FBE_DEVICE_TYPE_SP) &&
       (deviceType != FBE_DEVICE_TYPE_CACHE_CARD))
    {
        fbe_base_object_trace((fbe_base_object_t *)board_mgmt,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "RP: %s, unsupported deviceType 0x%llX.\n",
                              __FUNCTION__,
                              deviceType);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    if(deviceType == FBE_DEVICE_TYPE_SP)
    {
        *pResumePromInfoPtr = &board_mgmt->resume_info[pLocation->sp];
    }
    else if(deviceType == FBE_DEVICE_TYPE_CACHE_CARD)
    {
        *pResumePromInfoPtr = &board_mgmt->cacheCardResumeInfo[pLocation->slot];
    }    

    return FBE_STATUS_OK;
} // End of function fbe_board_mgmt_get_resume_prom_info_ptr

/*****************************************************************
 * @fn fbe_board_mgmt_resume_prom_handle_cache_card_status_change(fbe_board_mgmt_t * pBoardMgmt,
 *                                         fbe_device_physical_location_t * pLocation,
 *                                         fbe_board_mgmt_cache_card_info_t * pNewCacheCardInfo,
 *                                         fbe_board_mgmt_cache_card_info_t * pOldCacheCardInfo)
 ****************************************************************
 * @brief
 *  This function initiates the Cache Card Resume prom reads or clears it 
 *  based on the cache card state change.
 * 
 * @param pBoardMgmt -
 * @param pLocation - The location of the cache card.
 * @param pNewCacheCardInfo - The new Cache Card info.
 * @param pOldCacheCardInfo - The old Cache Card info.
 * 
 * @return fbe_status_t 
 *
 * @version
 *  16-Feb-2013:  Rui Chang - Created. 
 *
 ****************************************************************/
fbe_status_t fbe_board_mgmt_resume_prom_handle_cache_card_status_change(fbe_board_mgmt_t * pBoardMgmt,
                                                 fbe_device_physical_location_t * pLocation,
                                                 fbe_board_mgmt_cache_card_info_t * pNewCacheCardInfo,
                                                 fbe_board_mgmt_cache_card_info_t * pOldCacheCardInfo)
{
    fbe_base_env_resume_prom_info_t    * pResumePromInfo = NULL;
    fbe_status_t                         status = FBE_STATUS_OK;

    status = fbe_board_mgmt_get_resume_prom_info_ptr(pBoardMgmt, 
                                                    FBE_DEVICE_TYPE_CACHE_CARD, 
                                                    pLocation, 
                                                    &pResumePromInfo);

    if(status != FBE_STATUS_OK) 
    {
        return status;
    }

    if((pNewCacheCardInfo->inserted == FBE_MGMT_STATUS_FALSE) && (pOldCacheCardInfo->inserted == FBE_MGMT_STATUS_TRUE))
    {
        fbe_base_object_trace((fbe_base_object_t *)pBoardMgmt, 
                              FBE_TRACE_LEVEL_WARNING, 
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                              "RP:CACHE CARD(%s, Slot %d), Device removed.\n", 
                              (pLocation->sp == SP_A)?"SPA":"SPB", 
                               pLocation->slot);

        /* The Cache Card was removed. */
        status = fbe_base_env_resume_prom_handle_device_removal((fbe_base_environment_t *)pBoardMgmt, 
                                                                FBE_DEVICE_TYPE_CACHE_CARD, 
                                                                pLocation);
        if(status != FBE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t *)pBoardMgmt,
                                  FBE_TRACE_LEVEL_WARNING,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "RP:CACHE CARD(%s, Slot %d), Failed to handle device removal, status 0x%X.\n",
                                  (pLocation->sp == SP_A)?"SPA":"SPB", 
                                  pLocation->slot,
                                  status);

            return status;
        }
    }
    else 
    {
        /* Check the state of the inserted bit for Cache Card.
         * Issue the resume prom read 'only' if the status is changed from
         * NOT INSERTED to INSERTED. This is to avoid issuing the resume read 
         * for all the other cases. 
         * Because we init bInserted to INSERTED, so for SP just came up, 
         * we need to check the op_status. 
         */
        if(((pNewCacheCardInfo->inserted == FBE_MGMT_STATUS_TRUE) && (pOldCacheCardInfo->inserted == FBE_MGMT_STATUS_FALSE)) ||
           ((pNewCacheCardInfo->inserted == FBE_MGMT_STATUS_TRUE) && (pResumePromInfo->op_status == FBE_RESUME_PROM_STATUS_UNINITIATED)))
        {
            status = fbe_board_mgmt_initiate_resume_prom_cmd(pBoardMgmt, 
                                                           FBE_DEVICE_TYPE_CACHE_CARD, 
                                                           pLocation);
            if(status != FBE_STATUS_OK)
            {
                fbe_base_object_trace((fbe_base_object_t *)pBoardMgmt,
                                      FBE_TRACE_LEVEL_WARNING,
                                      FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                      "RP:CACHE CARD(%s, Slot %d), Failed to initiate read, status 0x%X.\n",
                                      (pLocation->sp == SP_A)?"SPA":"SPB", 
                                      pLocation->slot,
                                      status);
                  return status;
            }
        }
    }

    return status;
}

/*!**************************************************************
 * @fn fbe_board_mgmt_resume_prom_update_encl_fault_led(fbe_board_mgmt_t * board_mgmt,
                                    fbe_device_physical_location_t *pLocation)
 ****************************************************************
 * @brief
 *  This function updates the enclosure fault led for the SP resume prom fault.
 *
 * @param board_mgmt -
 * @param pLocation - The device location.
 *
 * @return fbe_status_t
 *
 * @version
 *  02-Apr-2013:  Lin Lou - Created.
 *
 ****************************************************************/

fbe_status_t fbe_board_mgmt_resume_prom_update_encl_fault_led(fbe_board_mgmt_t * board_mgmt,
                                                              fbe_u64_t deviceType,
                                                              fbe_device_physical_location_t *pLocation)
{
    fbe_status_t status;
    fbe_encl_fault_led_reason_t enclFaultLedReason;

    switch(deviceType)
    {
    case FBE_DEVICE_TYPE_SP:
        enclFaultLedReason = FBE_ENCL_FAULT_LED_SP_RESUME_PROM_FLT;
        break;

    default:
        fbe_base_object_trace((fbe_base_object_t *)board_mgmt,
                          FBE_TRACE_LEVEL_WARNING,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "RP:BoardMgmt, %s, unsupported deviceType %s(%lld), sp:%d.\n",
                          __FUNCTION__,
                          fbe_base_env_decode_device_type(deviceType),
                          deviceType,
                          pLocation->sp);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_board_mgmt_update_encl_fault_led(board_mgmt,
                                                  pLocation,
                                                  enclFaultLedReason);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)board_mgmt,
                          FBE_TRACE_LEVEL_WARNING,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "RP:%s - update_encl_fault_led failed for SP %d, status: 0x%X.\n",
                          fbe_base_env_decode_device_type(deviceType),
                          pLocation->sp, status);
    }
    return status;
}
/*!**************************************************************
 * @fn fbe_board_mgmt_resume_prom_handle_resume_prom_info_change(fbe_board_mgmt_t * pBoardMgmt,
 *                                        fbe_u64_t deviceType,
 *                                        fbe_device_physical_location_t * pLocation)
 ****************************************************************
 * @brief
 *  This function gets called when the notification was recieved that
 *  the resume prom info including status got changed. It will initiate
 *  the resume prom read.
 *
 * @param pBaseEnv - The pointer to the fbe_base_environment_t.
 * @param deviceType - The device type.
 * @param pLocation - The pointer to the physical location that the work item needs to be added for.
 *
 * @return fbe_status_t
 *
 * @version
 *  26-June-2013:  PHE - Created. 
 *
 ****************************************************************/
fbe_status_t fbe_board_mgmt_resume_prom_handle_resume_prom_info_change(fbe_board_mgmt_t * pBoardMgmt,
                                                      fbe_u64_t deviceType,
                                                      fbe_device_physical_location_t * pLocation)
{
    fbe_status_t                       status = FBE_STATUS_OK;
    fbe_u8_t                           logHeader[FBE_BASE_ENV_RESUME_PROM_LOG_HEADER_SIZE + 1] = {0};
    fbe_base_env_resume_prom_info_t  * pResumePromInfo = NULL;

    if(deviceType != FBE_DEVICE_TYPE_SP)
    {
        fbe_base_object_trace((fbe_base_object_t *)pBoardMgmt,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "RP: %s, unsupported deviceType 0x%llX.\n",
                              __FUNCTION__,
                              deviceType);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_board_mgmt_resume_prom_generate_logheader(deviceType, pLocation, &logHeader[0], FBE_BASE_ENV_RESUME_PROM_LOG_HEADER_SIZE);

    fbe_base_object_trace((fbe_base_object_t *)pBoardMgmt,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s, handle_resume_prom_info_change entry, deviceType 0x%llX.\n",
                              &logHeader[0],
                              deviceType);

    fbe_board_mgmt_get_resume_prom_info_ptr(pBoardMgmt, deviceType, pLocation, &pResumePromInfo);

    if(pResumePromInfo == NULL) 
    {
        fbe_base_object_trace((fbe_base_object_t *)pBoardMgmt,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s, resume prom info ptr is NULL.\n",
                              &logHeader[0]);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    // got notified of a change, so if old status was "off", initate the read
    if(pResumePromInfo->op_status == FBE_RESUME_PROM_STATUS_DEVICE_POWERED_OFF)
    {
        fbe_base_object_trace((fbe_base_object_t *)pBoardMgmt,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s, RP op_status changed from DEVICE_POWERED_OFF, initiate RP read.\n",
                              &logHeader[0]);

        status = fbe_board_mgmt_initiate_resume_prom_cmd(pBoardMgmt,
                                                         FBE_DEVICE_TYPE_SP,
                                                         pLocation); 

        if(status != FBE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t *)pBoardMgmt,
                                  FBE_TRACE_LEVEL_WARNING,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "RP: Failed to initiate resume prom read for %s.\n",
                                  (pLocation->sp == SP_A) ? "SPA" : "SPB");
        }
    }

    return status;
} //fbe_board_mgmt_resume_prom_handle_resume_prom_info_change
