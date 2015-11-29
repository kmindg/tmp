/***************************************************************************
 * Copyright (C) EMC Corporation 2012
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_sps_mgmt_resume_prom.c
 ***************************************************************************
 * @brief
 *  This file contains the implemention for battery Resume Prom functionality.
 *
 * @version
 *  8-Oct-2012  Rui Chang - Created. 
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_memory.h"
#include "fbe/fbe_api_enclosure_interface.h"
#include "fbe_base_object.h"
#include "fbe_base_environment_resume_prom_private.h"
#include "fbe_base_environment_debug.h"
#include "fbe_sps_mgmt_private.h"

static fbe_status_t 
fbe_sps_mgmt_resume_prom_generate_logheader(fbe_device_physical_location_t * pLocation, 
                                                fbe_u8_t * pLogHeader, 
                                                fbe_u32_t logHeaderSize,
                                                fbe_u64_t deviceType);

/*!**************************************************************
 * @fn fbe_sps_mgmt_initiate_resume_prom_read(void * pContext, 
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
 *  8-Oct-2012:  Rui Chang - Created. 
 ****************************************************************/
fbe_status_t fbe_sps_mgmt_initiate_resume_prom_read(void * pContext, 
                                                        fbe_u64_t deviceType,
                                                        fbe_device_physical_location_t * pLocation)
{
    fbe_sps_mgmt_t *           pSpsMgmt = (fbe_sps_mgmt_t *)pContext;
    fbe_array_bob_info_t       *pBobArrayInfo;
    fbe_base_battery_info_t    pBobInfo;
    fbe_base_env_resume_prom_info_t *pResumePromInfo = NULL;
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_u8_t                        logHeader[FBE_BASE_ENV_RESUME_PROM_LOG_HEADER_SIZE + 1] = {0};

    if(deviceType != FBE_DEVICE_TYPE_BATTERY)
    {
        fbe_base_object_trace((fbe_base_object_t *)pSpsMgmt,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "RP:%s,unsupported deviceType:%s\n",
                              __FUNCTION__,
                              fbe_base_env_decode_device_type(deviceType));

        return FBE_STATUS_GENERIC_FAILURE;
    }

    // get the bbu info for the specified bbu component
    pBobArrayInfo = pSpsMgmt->arrayBobInfo;
    pBobInfo = pBobArrayInfo->bob_info[pLocation->slot];


    if (pBobInfo.hasResumeProm == FBE_FALSE)
    {
        fbe_base_object_trace((fbe_base_object_t *)pSpsMgmt,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "RP: %d_%d_%d, %s, This BBU has no resume to read.\n",
                              pLocation->bus,
                              pLocation->enclosure,
                              pLocation->slot,
                              __FUNCTION__);
        return FBE_STATUS_OK;
    }

    pResumePromInfo = &(pBobArrayInfo->bobResumeInfo[pLocation->slot]);

    /* Save the device type, Location and target object id */
    pResumePromInfo->device_type = deviceType;
    pResumePromInfo->location = *pLocation;
    pResumePromInfo->objectId = pBobArrayInfo->bobObjectId;

    /* Set log header for tracing purpose. */
    fbe_sps_mgmt_resume_prom_generate_logheader(pLocation, &logHeader[0], FBE_BASE_ENV_RESUME_PROM_LOG_HEADER_SIZE, deviceType);

    status = fbe_base_env_resume_prom_create_and_add_work_item((fbe_base_environment_t *)pSpsMgmt, 
                                                               pBobArrayInfo->bobObjectId, 
                                                               FBE_DEVICE_TYPE_BATTERY,
                                                               pLocation,
                                                               &logHeader[0]);

    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)pSpsMgmt,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s,Failed to create work item, status 0x%X\n",
                              &logHeader[0],
                              status);
    }

    return status;
} //fbe_sps_mgmt_initiate_resume_prom_read

/*!**************************************************************
 * @fn fbe_sps_mgmt_resume_prom_generate_logheader()
 ****************************************************************
 * @brief
 *  This function sets the log header for the sps module.
 *
 * @param pLocation - The pointer to the physical location.
 * @param pLogHeader(output) - The pointer to the log header.
 * @param logHeaderSize - header size 
 * @param deviceType - device type 
 *
 * @return fbe_status_t
 *
 * @version
 *  3-Oct-2012:  Rui Chang - Created. 
 ****************************************************************/
static fbe_status_t fbe_sps_mgmt_resume_prom_generate_logheader(fbe_device_physical_location_t * pLocation, 
                                                                    fbe_u8_t * pLogHeader, 
                                                                    fbe_u32_t logHeaderSize,
                                                                    fbe_u64_t deviceType)
{
    fbe_zero_memory(pLogHeader, logHeaderSize);
    if (deviceType == FBE_DEVICE_TYPE_BATTERY)
    {
        csx_p_snprintf(pLogHeader, logHeaderSize, "RP:BBU(%d_%d_%d)",
                           pLocation->bus, pLocation->enclosure, pLocation->slot);
    }
    return FBE_STATUS_OK;
}

/*!**************************************************************
 * @fn fbe_sps_mgmt_get_resume_prom_info_ptr(void * pContext,
 *                                         fbe_base_env_resume_prom_work_item_t * pResumeInfo)
 ****************************************************************
 * @brief
 *  This function gets the pointer to the resume prom info.
 *
 * @param pSpsMgmt - poiner to the sps mgmt struct where bbu info is located
 * @param deviceType - specifies resume prom device type
 * @param pLocation - location of the target bbu
 * @param pResumePromInfoPtr - return a pointer to the resume prom info (out)
 *
 * @return fbe_status_t
 *
 * @version
 *  3-Oct-2012:  Rui Chang - Created. 
 ****************************************************************/
fbe_status_t fbe_sps_mgmt_get_resume_prom_info_ptr(fbe_sps_mgmt_t *pSpsMgmt,
                                                       fbe_u64_t deviceType,
                                                       fbe_device_physical_location_t * pLocation,
                                                       fbe_base_env_resume_prom_info_t ** pResumePromInfoPtr)
{
    fbe_u32_t                               bbuSlotIndex;

    if(deviceType != FBE_DEVICE_TYPE_BATTERY)
    {
        fbe_base_object_trace((fbe_base_object_t *)pSpsMgmt,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "RP: %s, unsupported deviceType: %s(%lld).\n",
                              __FUNCTION__,
                              fbe_base_env_decode_device_type(deviceType),
                              deviceType);

        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    // ESP stores BBU data index 0 to BbuMaxCount-1.  pLocation has slot index per SP.
    if (pLocation->sp == SP_A)
    {
        bbuSlotIndex = pLocation->slot;
    }
    else
    {
        bbuSlotIndex = pLocation->slot + (pSpsMgmt->total_bob_count/2);
    }

    /* get the BoB resume info. */
    *pResumePromInfoPtr = &pSpsMgmt->arrayBobInfo->bobResumeInfo[bbuSlotIndex];

    return FBE_STATUS_OK;
} // End of function fbe_sps_mgmt_get_resume_prom_info_ptr


/*!**************************************************************
 * @fn fbe_sps_mgmt_resume_prom_handle_bob_status_change(fbe_sps_mgmt_t * pSpsMgmt,
 *                                         fbe_device_physical_location_t * pLocation,
 *                                         fbe_sps_info_t * pNewBobInfo,
 *                                         fbe_sps_info_t * pOldBobInfo)
 ****************************************************************
 * @brief
 *  This function initiates the Fan Resume prom read after device
 *  insertion. When device is removed, kill any panding resume
 *  operation.
 * 
 * @param pSpsMgmt -
 * @param pLocation - The location of the Fan
 * @param pNewBobInfo - The new power supply info.
 * @param pOldBobInfo - The old power supply info.
 * 
 * @return fbe_status_t 
 *
 * @version
 *   3-Oct-2012:  Rui Chang - Created. 
 ****************************************************************/
fbe_status_t fbe_sps_mgmt_resume_prom_handle_bob_status_change(fbe_sps_mgmt_t * pSpsMgmt,
                                                               fbe_device_physical_location_t * pLocation,
                                                               fbe_base_battery_info_t * pNewBobInfo,
                                                               fbe_base_battery_info_t * pOldBobInfo)
{
    fbe_base_env_resume_prom_info_t    * pResumePromInfo = NULL;
    fbe_status_t                         status = FBE_STATUS_OK;

    status = fbe_sps_mgmt_get_resume_prom_info_ptr(pSpsMgmt, 
                                                       FBE_DEVICE_TYPE_BATTERY, 
                                                       pLocation, 
                                                       &pResumePromInfo);

    if(status != FBE_STATUS_OK) 
    {
        return status;
    }

    if((pNewBobInfo->batteryInserted != FBE_MGMT_STATUS_TRUE) && (pOldBobInfo->batteryInserted == FBE_MGMT_STATUS_TRUE))
    {
        fbe_base_object_trace((fbe_base_object_t *)pSpsMgmt, 
                              FBE_TRACE_LEVEL_WARNING, 
                              FBE_TRACE_MESSAGE_ID_INFO , 
                              "RP:BBU(%d_%d_%d), Device removed.\n", 
                              pLocation->bus, 
                              pLocation->enclosure, 
                              pLocation->slot);

        /* The bbu was removed. */
        status = fbe_base_env_resume_prom_handle_device_removal((fbe_base_environment_t *)pSpsMgmt, 
                                                                FBE_DEVICE_TYPE_BATTERY, 
                                                                pLocation);
        if(status != FBE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t *)pSpsMgmt,
                                  FBE_TRACE_LEVEL_WARNING,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "RP:BBU(%d_%d_%d), %s, Failed to handle device removal, status 0x%X.\n",
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
        if(((pNewBobInfo->batteryInserted == FBE_MGMT_STATUS_TRUE) && (pOldBobInfo->batteryInserted != FBE_MGMT_STATUS_TRUE)) ||
           ((pNewBobInfo->batteryInserted == FBE_MGMT_STATUS_TRUE) && (pResumePromInfo->op_status == FBE_RESUME_PROM_STATUS_UNINITIATED)))
        {
            status = fbe_sps_mgmt_initiate_resume_prom_read(pSpsMgmt, 
                                                                FBE_DEVICE_TYPE_BATTERY,
                                                                pLocation);
            if(status != FBE_STATUS_OK)
            {
                fbe_base_object_trace((fbe_base_object_t *)pSpsMgmt,
                                      FBE_TRACE_LEVEL_WARNING,
                                      FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                      "RP:BBU(%d_%d_%d), %s, Failed to initiate read, status 0x%X.\n",
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
} //fbe_sps_mgmt_resume_prom_handle_status_change

/*!**************************************************************
 * @fn fbe_sps_mgmt_resume_prom_update_encl_fault_led(fbe_sps_mgmt_t *pCoolingMgmt,
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
 *  8-Oct-2012:  Rui Chang - Created. 
 *  3-Apr-2013:  Lin Lou - Modified.
 *
 ****************************************************************/

fbe_status_t fbe_sps_mgmt_resume_prom_update_encl_fault_led(fbe_sps_mgmt_t *pSpsMgmt,
                                    fbe_u64_t deviceType,
                                    fbe_device_physical_location_t *pLocation)
{
    fbe_status_t status;
    fbe_encl_fault_led_reason_t enclFaultLedReason;

    switch(deviceType)
    {
    case FBE_DEVICE_TYPE_BATTERY:
        enclFaultLedReason = FBE_ENCL_FAULT_LED_BATTERY_RESUME_PROM_FLT;
        break;

    default:
        fbe_base_object_trace((fbe_base_object_t *)pSpsMgmt,
                          FBE_TRACE_LEVEL_WARNING,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "RP:SPSMGMT(%d_%d_%d), unsupported deviceType %s(%lld).\n",
                          pLocation->bus, pLocation->enclosure, pLocation->slot,
                          fbe_base_env_decode_device_type(deviceType),
                          deviceType);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_sps_mgmt_update_encl_fault_led(pSpsMgmt, 
                                                pLocation,
                                                enclFaultLedReason);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)pSpsMgmt, 
                          FBE_TRACE_LEVEL_WARNING,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "RP:%s(%d_%d_%d), update_encl_fault_led failed, status: 0x%X.\n",
                          fbe_base_env_decode_device_type(deviceType),
                          pLocation->bus, pLocation->enclosure, pLocation->slot, status);
    }
    return status;
}
/* End of file fbe_sps_mgmt_resume_prom.c */

