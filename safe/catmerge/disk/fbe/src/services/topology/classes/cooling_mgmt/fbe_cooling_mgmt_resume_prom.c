/***************************************************************************
 * Copyright (C) EMC Corporation 2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_cool_mgmt_resume_prom.c
 ***************************************************************************
 * @brief
 *  This file contains the implemention for Cooling Module Resume Prom functionality.
 *
 * @version
 *  3-Oct-2011:  GB - cloned from PS mgmt code
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
#include "fbe_base_environment_resume_prom_private.h"
#include "fbe_base_environment_debug.h"

static fbe_status_t 
fbe_cooling_mgmt_resume_prom_generate_logheader(fbe_device_physical_location_t * pLocation, 
                                                fbe_u8_t * pLogHeader, 
                                                fbe_u32_t logHeaderSize);

/*!**************************************************************
 * @fn fbe_cooling_mgmt_initiate_resume_prom_read(void * pContext, 
 *                                      fbe_u64_t deviceType, 
 *                                      fbe_device_physical_location_t * pLocation)
 ****************************************************************
 * @brief
 * Determine the location to put the resume prom data and create a
 * work item to do the read.
 *
 * @param pContext - The pointer to the object.
 * @param deviceType - resume prom device type
 * @param pLocation - The pointer to the physical location that the work item needs to be added for.
 * 
 * @return fbe_status_t
 *
 * @version
 *  3-Oct-2011:  GB - cloned from PS mgmt code
 ****************************************************************/
fbe_status_t fbe_cooling_mgmt_initiate_resume_prom_read(void * pContext, 
                                                        fbe_u64_t deviceType,
                                                        fbe_device_physical_location_t * pLocation)
{
    fbe_cooling_mgmt_t *            pCoolMgmt = (fbe_cooling_mgmt_t *)pContext;
    fbe_cooling_mgmt_fan_info_t     *pFanInfo;
    fbe_base_env_resume_prom_info_t *pResumePromInfo = NULL;
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_u8_t                        logHeader[FBE_BASE_ENV_RESUME_PROM_LOG_HEADER_SIZE + 1] = {0};

    if(deviceType != FBE_DEVICE_TYPE_FAN)
    {
        fbe_base_object_trace((fbe_base_object_t *)pCoolMgmt,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "RP:%s,unsupported deviceType:%s\n",
                              __FUNCTION__,
                              fbe_base_env_decode_device_type(deviceType));

        return FBE_STATUS_GENERIC_FAILURE;
    }

    // get the cooling info for the specified cooling component
    status = fbe_cooling_mgmt_get_fan_info_by_location(pCoolMgmt,
                                                       pLocation,
                                                       &pFanInfo);

    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)pCoolMgmt,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "RP: %d_%d_%d, %s,get_fan_info_by_location failed.\n",
                              pLocation->bus,
                              pLocation->enclosure,
                              pLocation->slot,
                              __FUNCTION__);
        return status;
    }

    if (pFanInfo->basicInfo.hasResumeProm == FBE_FALSE)
    {
        fbe_base_object_trace((fbe_base_object_t *)pCoolMgmt,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "RP: %d_%d_%d, %s, This fan has no resume to read.\n",
                              pLocation->bus,
                              pLocation->enclosure,
                              pLocation->slot,
                              __FUNCTION__);
        return FBE_STATUS_OK;
    }

    pResumePromInfo = &pFanInfo->fanResumeInfo;

    /* Save the device type, Location and target object id */
    pResumePromInfo->device_type = deviceType;
    pResumePromInfo->location = *pLocation;
    pResumePromInfo->objectId = pFanInfo->objectId;

    /* Set log header for tracing purpose. */
    fbe_cooling_mgmt_resume_prom_generate_logheader(pLocation, &logHeader[0], FBE_BASE_ENV_RESUME_PROM_LOG_HEADER_SIZE);

    status = fbe_base_env_resume_prom_create_and_add_work_item((fbe_base_environment_t *)pCoolMgmt, 
                                                               pFanInfo->objectId, 
                                                               FBE_DEVICE_TYPE_FAN,
                                                               pLocation,
                                                               &logHeader[0]);

    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)pCoolMgmt,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s,Failed to create work item, status 0x%X\n",
                              &logHeader[0],
                              status);
    }

    return status;
} //fbe_cooling_mgmt_initiate_resume_prom_read

/*!**************************************************************
 * @fn fbe_cooling_mgmt_resume_prom_generate_logheader()
 ****************************************************************
 * @brief
 *  This function sets the log header for the cooling module.
 *
 * @param pLocation - The pointer to the physical location.
 * @param pLogHeader(output) - The pointer to the log header.
 * @param logHeaderSize - header size 
 *
 * @return fbe_status_t
 *
 * @version
 *  3-Oct-2011:  GB - cloned from PS mgmt code
 ****************************************************************/
static fbe_status_t fbe_cooling_mgmt_resume_prom_generate_logheader(fbe_device_physical_location_t * pLocation, 
                                                                    fbe_u8_t * pLogHeader, 
                                                                    fbe_u32_t logHeaderSize)
{
    fbe_zero_memory(pLogHeader, logHeaderSize);
    csx_p_snprintf(pLogHeader, logHeaderSize, "RP:FAN(%d_%d_%d)",
                       pLocation->bus, pLocation->enclosure, pLocation->slot);

    return FBE_STATUS_OK;
}

/*!**************************************************************
 * @fn fbe_cooling_mgmt_get_resume_prom_info_ptr(void * pContext,
 *                                         fbe_base_env_resume_prom_work_item_t * pResumeInfo)
 ****************************************************************
 * @brief
 *  This function gets the pointer to the resume prom info.
 *
 * @param pCoolMgmt - poiner to the cooling mgmt struct where fan info is located
 * @param deviceType - specifies resume prom device type
 * @param pLocation - location of the target fan
 * @param pResumePromInfoPtr - return a pointer to the resume prom info (out)
 *
 * @return fbe_status_t
 *
 * @version
 *  3-Oct-2011:  GB - cloned from PS mgmt code
 ****************************************************************/
fbe_status_t fbe_cooling_mgmt_get_resume_prom_info_ptr(fbe_cooling_mgmt_t *pCoolMgmt,
                                                       fbe_u64_t deviceType,
                                                       fbe_device_physical_location_t * pLocation,
                                                       fbe_base_env_resume_prom_info_t ** pResumePromInfoPtr)
{
    fbe_status_t                      status = FBE_STATUS_OK;
    fbe_cooling_mgmt_fan_info_t       *pFanInfo = NULL;
 
    if(deviceType != FBE_DEVICE_TYPE_FAN)
    {
        fbe_base_object_trace((fbe_base_object_t *)pCoolMgmt,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "RP: %s, unsupported deviceType: %s(%lld).\n",
                              __FUNCTION__,
                              fbe_base_env_decode_device_type(deviceType),
                              deviceType);

        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    /* get the encl ps info. */
    status = fbe_cooling_mgmt_get_fan_info_by_location(pCoolMgmt, pLocation, &pFanInfo);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)pCoolMgmt,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "RP: %s, get_fan_info_by_location failed.\n",
                              __FUNCTION__);

        return status;
    }

    if(pFanInfo->basicInfo.hasResumeProm) 
    {
        *pResumePromInfoPtr = &pFanInfo->fanResumeInfo;
        return FBE_STATUS_OK;
    }
    else
    {
        *pResumePromInfoPtr = NULL;
        return FBE_STATUS_COMPONENT_NOT_FOUND;
    }
    
} // End of function fbe_cooling_mgmt_get_resume_prom_info_ptr


/*!**************************************************************
 * @fn fbe_cooling_mgmt_resume_prom_handle_status_change(fbe_cooling_mgmt_t * pCoolMgmt,
 *                                         fbe_device_physical_location_t * pLocation,
 *                                         fbe_cooling_info_t * pNewCoolInfo,
 *                                         fbe_cooling_info_t * pOldCoolInfo)
 ****************************************************************
 * @brief
 *  This function initiates the Fan Resume prom read after device
 *  insertion. When device is removed, kill any panding resume
 *  operation.
 * 
 * @param pCoolMgmt -
 * @param pLocation - The location of the Fan
 * @param pNewFanInfo - The new power supply info.
 * @param pOldFanInfo - The old power supply info.
 * 
 * @return fbe_status_t 
 *
 * @version
 *   3-Oct-2011:  GB - cloned from PS code
 ****************************************************************/
fbe_status_t fbe_cooling_mgmt_resume_prom_handle_status_change(fbe_cooling_mgmt_t * pCoolMgmt,
                                                               fbe_device_physical_location_t * pLocation,
                                                               fbe_cooling_info_t * pNewFanInfo,
                                                               fbe_cooling_info_t * pOldFanInfo)
{
    fbe_base_env_resume_prom_info_t    * pResumePromInfo = NULL;
    fbe_status_t                         status = FBE_STATUS_OK;

    status = fbe_cooling_mgmt_get_resume_prom_info_ptr(pCoolMgmt, 
                                                       FBE_DEVICE_TYPE_FAN, 
                                                       pLocation, 
                                                       &pResumePromInfo);

    if(status != FBE_STATUS_OK) 
    {
        return status;
    }

    if((pNewFanInfo->inserted != FBE_MGMT_STATUS_TRUE) && (pOldFanInfo->inserted == FBE_MGMT_STATUS_TRUE))
    {
        fbe_base_object_trace((fbe_base_object_t *)pCoolMgmt, 
                              FBE_TRACE_LEVEL_WARNING, 
                              FBE_TRACE_MESSAGE_ID_INFO , 
                              "RP:Fan(%d_%d_%d), Device removed.\n", 
                              pLocation->bus, 
                              pLocation->enclosure, 
                              pLocation->slot);

        /* The fan was removed. */
        status = fbe_base_env_resume_prom_handle_device_removal((fbe_base_environment_t *)pCoolMgmt, 
                                                                FBE_DEVICE_TYPE_FAN, 
                                                                pLocation);
        if(status != FBE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t *)pCoolMgmt,
                                  FBE_TRACE_LEVEL_WARNING,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "RP:Fan(%d_%d_%d), %s, Failed to handle device removal, status 0x%X.\n",
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
        /* Check the state of the inserted status.
         * Issue the resume prom read only if 
         *  1) the inserted status changed from NOT TRUE to TRUE
         *  2) inserted status is TRUE but resume prom has not yet been started (boot up case)
         */
        if(((pNewFanInfo->inserted == FBE_MGMT_STATUS_TRUE) && (pOldFanInfo->inserted != FBE_MGMT_STATUS_TRUE)) ||
           ((pNewFanInfo->inserted == FBE_MGMT_STATUS_TRUE) && (pResumePromInfo->op_status == FBE_RESUME_PROM_STATUS_UNINITIATED)))
        {
            status = fbe_cooling_mgmt_initiate_resume_prom_read(pCoolMgmt, 
                                                                FBE_DEVICE_TYPE_FAN,
                                                                pLocation);
            if(status != FBE_STATUS_OK)
            {
                fbe_base_object_trace((fbe_base_object_t *)pCoolMgmt,
                                      FBE_TRACE_LEVEL_WARNING,
                                      FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                      "RP:Fan(%d_%d_%d), %s, Failed to initiate read, status 0x%X.\n",
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
} //fbe_cooling_mgmt_resume_prom_handle_status_change

/*!**************************************************************
 * @fn fbe_cooling_mgmt_resume_prom_update_encl_fault_led(fbe_cooling_mgmt_t *pCoolingMgmt,
 *                                         fbe_device_physical_location_t * pLocation)
 ****************************************************************
 * @brief
 *  This function updates the enclosure fault led for the resume prom fault.
 * 
 * @param pEnclMgmt -
 * @param pLocation - The location of the FAN.
 * 
 * @return fbe_status_t 
 *
 * @version
 *  15-Dec-2011:  PHE- Created. 
 *  03-Apr-2013:  Lin Lou - Modified.
 *
 ****************************************************************/
fbe_status_t fbe_cooling_mgmt_resume_prom_update_encl_fault_led(fbe_cooling_mgmt_t *pCoolingMgmt,
                                    fbe_u64_t deviceType,
                                    fbe_device_physical_location_t *pLocation)
{
    fbe_status_t status;

    if(deviceType != FBE_DEVICE_TYPE_FAN)
    {
        fbe_base_object_trace((fbe_base_object_t *)pCoolingMgmt,
                          FBE_TRACE_LEVEL_WARNING,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "RP:CoolingMgmt(%d_%d_%d), unsupported deviceType %s(%lld).\n",
                          pLocation->bus, pLocation->enclosure, pLocation->slot,
                          fbe_base_env_decode_device_type(deviceType),
                          deviceType);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_cooling_mgmt_update_encl_fault_led(pCoolingMgmt, 
                                                 pLocation, 
                                                 FBE_ENCL_FAULT_LED_FAN_RESUME_PROM_FLT);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)pCoolingMgmt, 
                          FBE_TRACE_LEVEL_WARNING,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "RP:%s(%d_%d_%d), update_encl_fault_led failed, status: 0x%X.\n",
                          fbe_base_env_decode_device_type(deviceType),
                          pLocation->bus, pLocation->enclosure, pLocation->slot, status);
    }
    return status;
}

/* End of file fbe_cooling_mgmt_resume_prom.c */
