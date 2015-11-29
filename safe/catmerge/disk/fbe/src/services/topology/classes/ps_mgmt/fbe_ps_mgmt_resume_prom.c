/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_ps_mgmt_resume_prom.c
 ***************************************************************************
 * @brief
 *  This file contains the implemention for Power Supply Resume Prom functionality.
 *
 * @version
 *   31-Dec-2010:  Arun S - Created.
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
#include "fbe_base_environment_resume_prom_private.h"
#include "fbe_base_environment_debug.h"

static fbe_status_t 
fbe_ps_mgmt_resume_prom_generate_logheader(fbe_device_physical_location_t * pLocation, 
                                           fbe_u8_t * pLogHeader, 
                                           fbe_u32_t logHeaderSize);

/*!**************************************************************
 * @fn fbe_ps_mgmt_initiate_resume_prom_read(void * pContext, 
 *                                      fbe_u64_t deviceType, 
 *                                      fbe_device_physical_location_t * pLocation)
 ****************************************************************
 * @brief
 *  This function creates and queues the work item when the read needs to be issued.
 *
 * @param pContext - The pointer to the object.
 * @param deviceType -
 * @param pLocation - The pointer to the physical location that the work item needs to be added for.
 * 
 * @return fbe_status_t
 *
 * @version
 *  15-Dec-2010:  Arun S - Created. 
 *
 ****************************************************************/
fbe_status_t fbe_ps_mgmt_initiate_resume_prom_read(void * pContext, 
                                              fbe_u64_t deviceType,
                                              fbe_device_physical_location_t * pLocation)
{
    fbe_ps_mgmt_t           * pPsMgmt = (fbe_ps_mgmt_t *)pContext;
    fbe_encl_ps_info_t      * pEnclPsInfo = NULL;
    fbe_base_env_resume_prom_info_t *pResumePromInfo = NULL;
    fbe_status_t            status = FBE_STATUS_OK;
    fbe_u8_t                logHeader[FBE_BASE_ENV_RESUME_PROM_LOG_HEADER_SIZE + 1] = {0};

    if(deviceType != FBE_DEVICE_TYPE_PS)
    {
        fbe_base_object_trace((fbe_base_object_t *)pPsMgmt,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "RP: %s, unsupported deviceType: %s.\n",
                              __FUNCTION__,
                              fbe_base_env_decode_device_type(deviceType));

        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Set log header for tracing purpose. */
    fbe_ps_mgmt_resume_prom_generate_logheader(pLocation, &logHeader[0], FBE_BASE_ENV_RESUME_PROM_LOG_HEADER_SIZE);


    status = fbe_ps_mgmt_get_encl_ps_info_by_location(pPsMgmt, pLocation, &pEnclPsInfo);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)pPsMgmt,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s ps location info not found,\n",
                              &logHeader[0]);

        return status;
    }

    pResumePromInfo = &pEnclPsInfo->psResumeInfo[pLocation->slot];

    /* Save the device type, Location and target object id */
    pResumePromInfo->device_type = deviceType;
    pResumePromInfo->location = *pLocation;
    pResumePromInfo->objectId = pEnclPsInfo->objectId;

    status = fbe_base_env_resume_prom_create_and_add_work_item((fbe_base_environment_t *)pPsMgmt, 
                                                               pEnclPsInfo->objectId, 
                                                               FBE_DEVICE_TYPE_PS,
                                                               pLocation,  
                                                               &logHeader[0]);

    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)pPsMgmt,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s, Failed to create_and_add_work_item, status 0x%X.\n",
                              &logHeader[0],
                              status);
    }

    return status;
}

/*!**************************************************************
 * @fn fbe_ps_mgmt_resume_prom_generate_logheader()
 ****************************************************************
 * @brief
 *  This function sets the log header for the power supply.
 *
 * @param pLocation - The pointer to the physical location.
 * @param pLogHeader(output) - The pointer to the log header.
 * @param logHeaderSize -
 *
 * @return fbe_status_t
 *
 * @version
 *  18-Jan-2010:  Arun S - Created. 
 *
 ****************************************************************/
static fbe_status_t fbe_ps_mgmt_resume_prom_generate_logheader(fbe_device_physical_location_t * pLocation, 
                                                               fbe_u8_t * pLogHeader, 
                                                               fbe_u32_t logHeaderSize)
{
    fbe_zero_memory(pLogHeader, logHeaderSize);

    csx_p_snprintf(pLogHeader, logHeaderSize, "RP:PS(%d_%d_%d)",
                       pLocation->bus, pLocation->enclosure, pLocation->slot);

    return FBE_STATUS_OK;
}

/*!**************************************************************
 * @fn fbe_ps_mgmt_get_resume_prom_info_ptr(void * pContext,
 *                                         fbe_base_env_resume_prom_work_item_t * pResumeInfo)
 ****************************************************************
 * @brief
 *  This function gets the pointer to the resume prom info.
 *
 * @param pContext - The pointer to the callback context.
 * @param pResumeInfo - Pointer to the resume prom information.
 *
 * @return fbe_status_t
 *
 * @version
 *  08-Nov-2010:  Arun S - Created. 
 *
 ****************************************************************/
fbe_status_t fbe_ps_mgmt_get_resume_prom_info_ptr(fbe_ps_mgmt_t * pPsMgmt,
                                                  fbe_u64_t deviceType,
                                                  fbe_device_physical_location_t * pLocation,
                                                  fbe_base_env_resume_prom_info_t ** pResumePromInfoPtr)
{
    fbe_status_t                      status = FBE_STATUS_OK;
    fbe_encl_ps_info_t              * pEnclPsInfo = NULL;
 
    *pResumePromInfoPtr = NULL;
    if(deviceType != FBE_DEVICE_TYPE_PS)
    {
        fbe_base_object_trace((fbe_base_object_t *)pPsMgmt,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "RP: %s, unsupported deviceType: %s(%lld).\n",
                              __FUNCTION__,
                              fbe_base_env_decode_device_type(deviceType),
                              deviceType);

        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    /* get the encl ps info. */
    status = fbe_ps_mgmt_get_encl_ps_info_by_location(pPsMgmt, pLocation, &pEnclPsInfo);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)pPsMgmt,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "RP: %s, getPsLocation failed, %d_%d_%d, status %d.\n",
                              __FUNCTION__ , 
                              pLocation->bus, pLocation->enclosure, pLocation->slot,
                              status);

        return status;
    }

    if(pLocation->slot < pEnclPsInfo->psCount) 
    {
        *pResumePromInfoPtr = &pEnclPsInfo->psResumeInfo[pLocation->slot];
        return FBE_STATUS_OK;
    }
    else
    {
        *pResumePromInfoPtr = NULL;
        return FBE_STATUS_NO_DEVICE;
    }
    
} // End of function fbe_ps_mgmt_get_resume_prom_info_ptr

/*****************************************************************
 * @fn fbe_ps_mgmt_resume_prom_handle_ps_status_change(fbe_ps_mgmt_t * pPsMgmt,
 *                                         fbe_device_physical_location_t * pLocation,
 *                                         fbe_power_supply_info_t * pPsInfo,
 *                                         fbe_power_supply_info_t * pPrevPsInfo)
 ****************************************************************
 * @brief
 *  This function initiates the PS Resume prom reads or clears it 
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
 *  15-Dec-2010:  Arun S - Created. 
 *
 ****************************************************************/
fbe_status_t fbe_ps_mgmt_resume_prom_handle_ps_status_change(fbe_ps_mgmt_t * pPsMgmt,
                                                 fbe_device_physical_location_t * pLocation,
                                                 fbe_power_supply_info_t * pNewPsInfo,
                                                 fbe_power_supply_info_t * pOldPsInfo)
{
    fbe_base_env_resume_prom_info_t    * pResumePromInfo = NULL;
    fbe_status_t                         status = FBE_STATUS_OK;

    status = fbe_ps_mgmt_get_resume_prom_info_ptr(pPsMgmt, 
                                                    FBE_DEVICE_TYPE_PS, 
                                                    pLocation, 
                                                    &pResumePromInfo);

    if(status != FBE_STATUS_OK) 
    {
        return status;
    }

    if((!pNewPsInfo->bInserted) && (pOldPsInfo->bInserted))
    {
        fbe_base_object_trace((fbe_base_object_t *)pPsMgmt, 
                              FBE_TRACE_LEVEL_WARNING, 
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                              "RP: PS(%d_%d_%d), Device removed.\n", 
                              pLocation->bus, 
                              pLocation->enclosure, 
                              pLocation->slot);

        /* The power supply was removed. */
        status = fbe_base_env_resume_prom_handle_device_removal((fbe_base_environment_t *)pPsMgmt, 
                                                                FBE_DEVICE_TYPE_PS, 
                                                                pLocation);
        if(status != FBE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t *)pPsMgmt,
                                  FBE_TRACE_LEVEL_WARNING,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "RP: PS(%d_%d_%d), %s, Failed to handle device removal, status 0x%X.\n",
                                  pLocation->bus,
                                  pLocation->enclosure,
                                  pLocation->slot,
                                  __FUNCTION__,
                                  status);

            return status;
        }
    }
    else 
    {
        /* Check the state of the inserted bit for Power Supplies.
         * Issue the resume prom read 'only' if the status is changed from
         * NOT INSERTED to INSERTED. This is to avoid issuing the resume read 
         * for all the other cases. 
         * Because we init bInserted to INSERTED, so for SP just came up, 
         * we need to check the op_status. 
         */
        if(((pNewPsInfo->bInserted) && (!pOldPsInfo->bInserted)) ||
           ((pNewPsInfo->bInserted) && (pResumePromInfo->op_status == FBE_RESUME_PROM_STATUS_UNINITIATED)))
        {
            status = fbe_ps_mgmt_initiate_resume_prom_read(pPsMgmt, 
                                                           FBE_DEVICE_TYPE_PS, 
                                                           pLocation);
            if(status != FBE_STATUS_OK)
            {
                fbe_base_object_trace((fbe_base_object_t *)pPsMgmt,
                                      FBE_TRACE_LEVEL_WARNING,
                                      FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                      "RP: PS(%d_%d_%d), %s, Failed to initiate read, status 0x%X.\n",
                                      pLocation->bus,
                                      pLocation->enclosure,
                                      pLocation->slot,
                                      __FUNCTION__,
                                      status);
    
                return status;
            }
        }
    }

    return status;
}

/*!**************************************************************
 * @fn fbe_encl_mgmt_resume_prom_update_encl_fault_led(fbe_encl_mgmt_t * pEnclMgmt,
 *                                         fbe_device_physical_location_t * pLocation)
 ****************************************************************
 * @brief
 *  This function updates the enclosure fault led for LCC resume prom fault.
 * 
 * @param pPsMgmt - The pointer to ps_mgmt
 * @param pLocation - The location of the PS.
 * 
 * @return fbe_status_t 
 *
 * @version
 *  15-Dec-2011:  PHE- Created. 
 *  03-Apr-2013:  Lin Lou - Modified.
 *
 ****************************************************************/
fbe_status_t fbe_ps_mgmt_resume_prom_update_encl_fault_led(fbe_ps_mgmt_t *pPsMgmt,
                                    fbe_u64_t deviceType,
                                    fbe_device_physical_location_t *pLocation)
{
    fbe_status_t status;

    if(deviceType != FBE_DEVICE_TYPE_PS)
    {
        fbe_base_object_trace((fbe_base_object_t *)pPsMgmt,
                          FBE_TRACE_LEVEL_WARNING,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "RP:PS(%d_%d_%d), unsupported deviceType %s(%lld).\n",
                          pLocation->bus, pLocation->enclosure, pLocation->slot,
                          fbe_base_env_decode_device_type(deviceType),
                          deviceType);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_ps_mgmt_update_encl_fault_led(pPsMgmt, 
                                                 pLocation, 
                                                 FBE_ENCL_FAULT_LED_PS_RESUME_PROM_FLT);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)pPsMgmt, 
                          FBE_TRACE_LEVEL_WARNING,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "RP:PS(%d_%d_%d), update_encl_fault_led failed, status: 0x%X.\n",
                          pLocation->bus, pLocation->enclosure, pLocation->slot, status);
    }
    return status;
}

/* End of file fbe_ps_mgmt_resume_prom.c */
