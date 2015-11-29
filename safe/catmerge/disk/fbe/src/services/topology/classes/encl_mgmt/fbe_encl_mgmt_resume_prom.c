/***************************************************************************
 * Copyright (C) EMC Corporation 2010 
 * All rights reserved. Licensed material -- property of EMC 
 * Corporation 
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_encl_mgmt_resume_prom.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the Enclosure Management object resume prom code.
 * 
 * @version
 *   31-Dec-2010:  Created. Arun S
 *
 ***************************************************************************/

#include "fbe/fbe_winddk.h"
#include "fbe/fbe_memory.h"
#include "fbe/fbe_api_enclosure_interface.h"
#include "fbe/fbe_api_board_interface.h"
#include "fbe_base_object.h"
#include "fbe_encl_mgmt_private.h"
#include "fbe_encl_mgmt.h"
#include "fbe_base_environment_debug.h"

static fbe_status_t 
fbe_encl_mgmt_resume_prom_generate_logheader(fbe_u64_t device_type, 
                                             fbe_device_physical_location_t * pLocation, 
                                             fbe_u8_t * pLogHeader, 
                                             fbe_u32_t logHeaderSize);

/*!**************************************************************
 * @fn fbe_encl_mgmt_initiate_resume_prom_read(void * pContext, 
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
fbe_status_t fbe_encl_mgmt_initiate_resume_prom_read(void * pContext, 
                                                     fbe_u64_t deviceType, 
                                                     fbe_device_physical_location_t * pLocation)
{
    fbe_encl_mgmt_t                  * pEnclMgmt = (fbe_encl_mgmt_t *)pContext;
    fbe_encl_info_t                  * pEnclInfo = NULL;
    fbe_status_t                       status = FBE_STATUS_OK;
    fbe_u8_t                           logHeader[FBE_BASE_ENV_RESUME_PROM_LOG_HEADER_SIZE + 1] = {0};
    fbe_base_env_resume_prom_info_t  * pResumePromInfo = NULL;
    fbe_bool_t                         processorEncl = FALSE;
    fbe_object_id_t                    objectId;

    if((deviceType != FBE_DEVICE_TYPE_SSC) && 
       (deviceType != FBE_DEVICE_TYPE_LCC) && 
       (deviceType != FBE_DEVICE_TYPE_ENCLOSURE) &&
       (deviceType != FBE_DEVICE_TYPE_DRIVE_MIDPLANE))
    {
        fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "RP: %s, unsupported deviceType: %s.\n",
                              __FUNCTION__,
                              fbe_base_env_decode_device_type(deviceType));

        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_encl_mgmt_get_encl_info_by_location(pEnclMgmt, pLocation, &pEnclInfo);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "RP: %d_%d_%d, %s, get_encl_info_by_location failed.\n",
                              pLocation->bus,
                              pLocation->enclosure,
                              pLocation->slot,
                              __FUNCTION__);

        return status;
    }

    if(deviceType == FBE_DEVICE_TYPE_LCC)
    {
        /* Get the Resume Info for the LCC */
        pResumePromInfo = &pEnclInfo->lccResumeInfo[pLocation->slot];
        objectId = pEnclInfo->object_id;

        /* Some LCC does not have resume */
        if (pEnclInfo->lccInfo[pLocation->slot].hasResumeProm == FBE_FALSE)
        {
            fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt,
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "RP: %d_%d_%d, %s, This LCC has no resume to read.\n",
                                  pLocation->bus,
                                  pLocation->enclosure,
                                  pLocation->slot,
                                  __FUNCTION__);
            return FBE_STATUS_OK;
        }
    }
    else if(deviceType == FBE_DEVICE_TYPE_ENCLOSURE) 
    {
        /* Get the Resume Info for the Enclosure (midplane) */
        pResumePromInfo = &pEnclInfo->enclResumeInfo;

        status = fbe_base_env_is_processor_encl((fbe_base_environment_t *)pEnclMgmt, 
                                                 pLocation,
                                                 &processorEncl);
        if (status != FBE_STATUS_OK) 
        {
            fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt, 
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s Error in checking whether it is PE, status 0x%x.\n",
                                  __FUNCTION__, status);
    
            return status;
        }

        if(processorEncl) 
        {
            status = fbe_api_get_board_object_id(&objectId);
            if (status != FBE_STATUS_OK) 
            {
                fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt,  
                                      FBE_TRACE_LEVEL_ERROR,
                                      FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                      "%s Error in getting the Board Object ID, status: 0x%X\n",
                                      __FUNCTION__, status);
                return status;
            }
        }
        else
        {
            objectId = pEnclInfo->object_id;
        }
    }
    else if(deviceType == FBE_DEVICE_TYPE_DRIVE_MIDPLANE)
    {
        /* Get the Resume Info for the Drive Midplane */
        if (pEnclInfo->driveMidplaneCount == 1)
        {
           pResumePromInfo = &pEnclInfo->driveMidplaneResumeInfo;
           objectId = pEnclInfo->object_id;
        }
        else
        {
               status = FBE_STATUS_NO_DEVICE;

               fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt,  
                                      FBE_TRACE_LEVEL_INFO,
                                      FBE_TRACE_MESSAGE_ID_INFO,
                                      "%s Invalid device type: %s, status: 0x%X\n",
                                      __FUNCTION__,
                                      fbe_base_env_decode_device_type(deviceType), 
                                      status);
                return status;
        }
        
    }
    else if(deviceType == FBE_DEVICE_TYPE_SSC)
    {
        /* Get the Resume Info for the Enclosure (midplane) */
        pResumePromInfo = &pEnclInfo->sscResumeInfo;
        objectId = pEnclInfo->object_id;
    }

    /* Save the device type, Location and target object id */
    pResumePromInfo->device_type = deviceType;
    pResumePromInfo->location = *pLocation;
    pResumePromInfo->objectId = objectId;

    /* Set log header for tracing purpose. */
    fbe_encl_mgmt_resume_prom_generate_logheader(deviceType, pLocation, &logHeader[0], FBE_BASE_ENV_RESUME_PROM_LOG_HEADER_SIZE);

    status = fbe_base_env_resume_prom_create_and_add_work_item((fbe_base_environment_t *)pEnclMgmt,
                                                               objectId, 
                                                               deviceType,
                                                               pLocation,
                                                               &logHeader[0]);

    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)pEnclMgmt, 
                                               FBE_TRACE_LEVEL_ERROR, 
                                               &logHeader[0], 
                                               "Failed to create_and_add_work_item, status 0x%X.\n",
                                               status);
    }
    
    return status;
}

/*!**************************************************************
 * @fn fbe_encl_mgmt_resume_prom_generate_logheader()
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
 *  18-Jan-2010:  Arun S - Created. 
 *
 ****************************************************************/
static fbe_status_t 
fbe_encl_mgmt_resume_prom_generate_logheader(fbe_u64_t device_type, 
                                             fbe_device_physical_location_t * pLocation, 
                                             fbe_u8_t * pLogHeader, 
                                             fbe_u32_t logHeaderSize)
{
    fbe_zero_memory(pLogHeader, logHeaderSize);

    switch(device_type)
    {
        case FBE_DEVICE_TYPE_ENCLOSURE:
            csx_p_snprintf(pLogHeader, logHeaderSize, "RP:Midplane(%d_%d)",
                               pLocation->bus, pLocation->enclosure);
            break;

        case FBE_DEVICE_TYPE_LCC:
            csx_p_snprintf(pLogHeader, logHeaderSize, "RP:LCC(%d_%d_%d)",
                               pLocation->bus, pLocation->enclosure, pLocation->slot);
            break;

        case FBE_DEVICE_TYPE_SSC:
            csx_p_snprintf(pLogHeader, logHeaderSize, "RP:SSC(%d_%d_%d)",
                           pLocation->bus, pLocation->enclosure, pLocation->slot);
            break;

        case FBE_DEVICE_TYPE_DRIVE_MIDPLANE:
            csx_p_snprintf(pLogHeader, logHeaderSize, "RP:Drive Midplane(%d_%d)",
                               pLocation->bus, pLocation->enclosure);
            break;

        default:
            break;
    }

    return FBE_STATUS_OK;
}

/*!**************************************************************
 * @fn fbe_encl_mgmt_get_resume_prom_info_ptr(fbe_encl_mgmt_t * pEnclMgmt,
 *                                                 fbe_u64_t deviceType,
 *                                                 fbe_device_physical_location_t * pLocation,
 *                                                 fbe_base_env_resume_prom_info_t ** pResumePromInfoPtr)
 ****************************************************************
 * @brief
 *  This function updates the Enclosure Management Resume prom info
 *  from Enclosure object.
 *
 * @param pContext - The pointer to the callback context.
 * @param pResumeInfo - Pointer to the resume prom information.
 *
 * @return fbe_status_t
 *
 * @version
 *  19-Jul-2011:  PHE - Created. 
 *
 ****************************************************************/
fbe_status_t fbe_encl_mgmt_get_resume_prom_info_ptr(fbe_encl_mgmt_t * pEnclMgmt,
                                                  fbe_u64_t deviceType,
                                                  fbe_device_physical_location_t * pLocation,
                                                  fbe_base_env_resume_prom_info_t ** pResumePromInfoPtr)
{
    fbe_status_t                         status = FBE_STATUS_OK;
    fbe_encl_info_t                    * pEnclInfo = NULL;

    *pResumePromInfoPtr = NULL;

    if((deviceType != FBE_DEVICE_TYPE_SSC) &&
       (deviceType != FBE_DEVICE_TYPE_LCC) && 
       (deviceType != FBE_DEVICE_TYPE_ENCLOSURE)&&
       (deviceType != FBE_DEVICE_TYPE_DRIVE_MIDPLANE))
    {
        fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "RP: %s, unsupported deviceType %s(%lld).\n",
                              __FUNCTION__,
                              fbe_base_env_decode_device_type(deviceType),
                              deviceType);

        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    /* Get the Enclosure information by location */    
    status = fbe_encl_mgmt_get_encl_info_by_location(pEnclMgmt, pLocation, &pEnclInfo);

    if (status != FBE_STATUS_OK) 
    {
        fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "RP: SP: %d, Bus_Encl: %d_%d get_encl_info_by_location failed.\n",
                              pLocation->sp,
                              pLocation->bus,
                              pLocation->enclosure);

        return status;
    }

    if(deviceType == FBE_DEVICE_TYPE_LCC)
    {
        if(pLocation->slot < pEnclInfo->lccCount)
        {
            *pResumePromInfoPtr = &pEnclInfo->lccResumeInfo[pLocation->slot];
        }
        else
        {
            *pResumePromInfoPtr = NULL;
            return FBE_STATUS_NO_DEVICE;
        }
    }
    else if(deviceType == FBE_DEVICE_TYPE_ENCLOSURE) 
    {
        *pResumePromInfoPtr = &pEnclInfo->enclResumeInfo;
    }
    else if(deviceType == FBE_DEVICE_TYPE_DRIVE_MIDPLANE) 
    {
        if (pEnclInfo->driveMidplaneCount == 1)
        {
           *pResumePromInfoPtr = &pEnclInfo->driveMidplaneResumeInfo;
        }
        else
        {
           *pResumePromInfoPtr = NULL;
           return FBE_STATUS_NO_DEVICE;
        }
    }
    else if(deviceType == FBE_DEVICE_TYPE_SSC) 
    {
        *pResumePromInfoPtr = &pEnclInfo->sscResumeInfo;
    }


    return FBE_STATUS_OK;
} // End of function fbe_encl_mgmt_get_resume_prom_info_ptr

/*!**************************************************************
 * @fn fbe_encl_mgmt_resume_prom_handle_lcc_status_change(fbe_encl_mgmt_t * pEnclMgmt,
 *                                         fbe_device_physical_location_t * pLocation,
 *                                         fbe_lcc_info_t * pNewLccInfo,
 *                                         fbe_lcc_info_t * pOldLccInfo)
 ****************************************************************
 * @brief
 *  This function initiates the LCC Resume prom reads or clears it 
 *  based on the LCC state change.
 * 
 * @param pEnclMgmt -
 * @param pLocation - The location of the LCC.
 * @param pNewLccInfo - The new LCC info.
 * @param pOldLccInfo - The old LCC info.
 * 
 * @return fbe_status_t 
 *
 * @version
 *  15-Dec-2010:  Arun S - Created. 
 *
 ****************************************************************/
fbe_status_t fbe_encl_mgmt_resume_prom_handle_lcc_status_change(fbe_encl_mgmt_t * pEnclMgmt, 
                                                                fbe_device_physical_location_t * pLocation, 
                                                                fbe_lcc_info_t * pNewLccInfo, 
                                                                fbe_lcc_info_t * pOldLccInfo)
{
    fbe_base_env_resume_prom_info_t    * pResumePromInfo = NULL;
    fbe_status_t                         status = FBE_STATUS_OK;

    status = fbe_encl_mgmt_get_resume_prom_info_ptr(pEnclMgmt, 
                                                    FBE_DEVICE_TYPE_LCC, 
                                                    pLocation, 
                                                    &pResumePromInfo);

    if(status != FBE_STATUS_OK) 
    {
        return status;
    }

    /* Check the status of the insert bit for LCC */
    if((!pNewLccInfo->inserted) && (pOldLccInfo->inserted))
    {
        /* The LCC was removed. */
        fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt, 
                              FBE_TRACE_LEVEL_WARNING, 
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                              "RP: LCC(%d_%d_%d), Device removed.\n", 
                              pLocation->bus, 
                              pLocation->enclosure, 
                              pLocation->slot);

        /* The LCC was removed. */
        status = fbe_base_env_resume_prom_handle_device_removal((fbe_base_environment_t *)pEnclMgmt, 
                                                                FBE_DEVICE_TYPE_LCC, 
                                                                pLocation);
        if(status != FBE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt,
                                  FBE_TRACE_LEVEL_WARNING,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "RP: LCC(%d_%d_%d), %s, Failed to handle device removal, status 0x%X.\n",
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
        /* Check the state of the inserted bit for LCC's.
         * Issue the resume prom read 'only' if the status is changed from
         * NOT INSERTED to INSERTED. This is to avoid issuing the resume read 
         * for all the other cases. 
         * Added the check for op_status in case LCC was initiated to INSERTED.
         */
        if(((pNewLccInfo->inserted) && (!pOldLccInfo->inserted)) ||
           ((pNewLccInfo->inserted) && (pResumePromInfo->op_status == FBE_RESUME_PROM_STATUS_UNINITIATED)))
        {
            status = fbe_encl_mgmt_initiate_resume_prom_read(pEnclMgmt, 
                                                             FBE_DEVICE_TYPE_LCC, 
                                                             pLocation);
            if(status != FBE_STATUS_OK)
            {
                fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt,
                                      FBE_TRACE_LEVEL_WARNING,
                                      FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                      "RP: LCC(%d_%d_%d), %s, Failed to initiate read, status 0x%X.\n",
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
 * @fn fbe_encl_mgmt_resume_prom_handle_ssc_status_change(fbe_encl_mgmt_t * pEnclMgmt,
 *                                         fbe_device_physical_location_t * pLocation,
 *                                         fbe_ssc_info_t * pNewSscInfo,
 *                                         fbe_ssc_info_t * pOldSscInfo)
 ****************************************************************
 * @brief
 *  This function initiates the SSC Resume prom read or clears it 
 *  based on the SSC state change.
 * 
 * @param pEnclMgmt -
 * @param pLocation - The location (bus, encl, slot)of the SSC.
 * @param pNewLccInfo - The new SSC info.
 * @param pOldLccInfo - The old SSC info.
 * 
 * @return fbe_status_t 
 *
 * @version
 *  15-Dec-2010:  Arun S - Created. 
 ****************************************************************/
fbe_status_t fbe_encl_mgmt_resume_prom_handle_ssc_status_change(fbe_encl_mgmt_t *pEnclMgmt, 
                                                                fbe_device_physical_location_t *pLocation, 
                                                                fbe_ssc_info_t *pNewSscInfo, 
                                                                fbe_ssc_info_t *pOldSscInfo)
{
    fbe_base_env_resume_prom_info_t    * pResumePromInfo = NULL;
    fbe_status_t                         status = FBE_STATUS_OK;

    status = fbe_encl_mgmt_get_resume_prom_info_ptr(pEnclMgmt, 
                                                    FBE_DEVICE_TYPE_SSC, 
                                                    pLocation, 
                                                    &pResumePromInfo);

    if(status != FBE_STATUS_OK) 
    {
        return status;
    }

    // check for removed
    if((!pNewSscInfo->inserted) && (pOldSscInfo->inserted))
    {
        fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt, 
                              FBE_TRACE_LEVEL_WARNING, 
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                              "RP: SSC(%d_%d_%d), Device removed.\n", 
                              pLocation->bus, 
                              pLocation->enclosure, 
                              pLocation->slot);

        // handle the removal
        status = fbe_base_env_resume_prom_handle_device_removal((fbe_base_environment_t *)pEnclMgmt, 
                                                                FBE_DEVICE_TYPE_SSC, 
                                                                pLocation);
        if(status != FBE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt,
                                  FBE_TRACE_LEVEL_WARNING,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "RP: SSC(%d_%d_%d), %s, Failed to handle device removal, status 0x%X.\n",
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
         // Issue the resume prom read 'only' if the status is changed from
         // NOT INSERTED to INSERTED. This is to avoid issuing the resume read 
         // for all the other cases. 
         // Added the check for op_status in case status was initiated to INSERTED.
        if(((pNewSscInfo->inserted) && (!pOldSscInfo->inserted)) ||
           ((pNewSscInfo->inserted) && (pResumePromInfo->op_status == FBE_RESUME_PROM_STATUS_UNINITIATED)))
        {
            status = fbe_encl_mgmt_initiate_resume_prom_read(pEnclMgmt, 
                                                             FBE_DEVICE_TYPE_SSC, 
                                                             pLocation);
            if(status != FBE_STATUS_OK)
            {
                fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt,
                                      FBE_TRACE_LEVEL_WARNING,
                                      FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                      "RP: SSC(%d_%d_%d), %s, Failed to initiate read, status 0x%X.\n",
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
} //fbe_encl_mgmt_resume_prom_handle_ssc_status_change

/*!**************************************************************
 * @fn fbe_encl_mgmt_resume_prom_handle_encl_status_change(fbe_encl_mgmt_t * pEnclMgmt,
 *                                         fbe_device_physical_location_t * pLocation)
 ****************************************************************
 * @brief
 *  This function initiates the Encl Resume prom reads or clears it 
 *  based on the Encl state change.
 * 
 * @param pEnclMgmt -
 * @param pLocation - The location of the PS.
 * 
 * @return fbe_status_t 
 *
 * @version
 *  15-Dec-2010:  Arun S - Created. 
 *
 ****************************************************************/
fbe_status_t fbe_encl_mgmt_resume_prom_handle_encl_status_change(fbe_encl_mgmt_t * pEnclMgmt, 
                                                                 fbe_device_physical_location_t * pLocation)
{
    fbe_status_t                 status = FBE_STATUS_OK;

    /* Check the status of the insert bit for LCC */
    if(0) //(!pNewLccInfo->inserted) && (pOldLccInfo->inserted))
    {
        /* The LCC was removed. */
        fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt, 
                              FBE_TRACE_LEVEL_WARNING, 
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                              "RP: Encl(%d_%d_%d), Device removed.\n", 
                              pLocation->bus, 
                              pLocation->enclosure, 
                              pLocation->slot);

        /* The LCC was removed. */
        status = fbe_base_env_resume_prom_handle_device_removal((fbe_base_environment_t *)pEnclMgmt, 
                                                                FBE_DEVICE_TYPE_ENCLOSURE, 
                                                                pLocation);
        if(status != FBE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt,
                                  FBE_TRACE_LEVEL_WARNING,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "RP: Encl(%d_%d_%d), Failed to handle device removal.\n",
                                  pLocation->bus,
                                  pLocation->enclosure,
                                  pLocation->slot);

            return status;
        }
    }
    else 
    {
        status = fbe_encl_mgmt_initiate_resume_prom_read(pEnclMgmt, 
                                                         FBE_DEVICE_TYPE_ENCLOSURE, 
                                                         pLocation);
        if(status != FBE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt,
                                  FBE_TRACE_LEVEL_WARNING,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "RP: Encl(%d_%d_%d), Failed to initiate read.\n",
                                  pLocation->bus,
                                  pLocation->enclosure,
                                  pLocation->slot);

            return status;
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
 * @param pEnclMgmt -
 * @param pLocation - The location of the LCC.
 * 
 * @return fbe_status_t 
 *
 * @version
 *  15-Dec-2011:  PHE- Created. 
 *  03-Apr-2013:  Lin Lou - Modified.
 *
 ****************************************************************/
fbe_status_t fbe_encl_mgmt_resume_prom_update_encl_fault_led(fbe_encl_mgmt_t *pEnclMgmt,
                                    fbe_u64_t deviceType,
                                    fbe_device_physical_location_t *pLocation)
{
    fbe_status_t status;
    fbe_encl_fault_led_reason_t enclFaultLedReason;

    switch(deviceType)
    {
    case FBE_DEVICE_TYPE_LCC:
        enclFaultLedReason = FBE_ENCL_FAULT_LED_LCC_RESUME_PROM_FLT;
        break;
    case FBE_DEVICE_TYPE_ENCLOSURE:
        enclFaultLedReason = FBE_ENCL_FAULT_LED_MIDPLANE_RESUME_PROM_FLT;
        break;
    case FBE_DEVICE_TYPE_DRIVE_MIDPLANE:
        enclFaultLedReason = FBE_ENCL_FAULT_LED_DRIVE_MIDPLANE_RESUME_PROM_FLT;
        break;
    case FBE_DEVICE_TYPE_SSC:
        enclFaultLedReason = FBE_ENCL_FAULT_LED_SSC_RESUME_PROM_FLT;
        break;

    default:
        fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt,
                          FBE_TRACE_LEVEL_WARNING,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "RP:ENCLMGMT(%d_%d_%d), unsupported deviceType %s(%lld).\n",
                          pLocation->bus, pLocation->enclosure, pLocation->slot,
                          fbe_base_env_decode_device_type(deviceType),
                          deviceType);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_encl_mgmt_update_encl_fault_led(pEnclMgmt, 
                                                 pLocation, 
                                                 enclFaultLedReason);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt, 
                          FBE_TRACE_LEVEL_WARNING,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "RP:%s(%d_%d_%d), update_encl_fault_led failed, status: 0x%X.\n",
                          fbe_base_env_decode_device_type(deviceType),
                          pLocation->bus, pLocation->enclosure, pLocation->slot, status);
    }
    return status;
}

/*!**************************************************************
 * @fn fbe_encl_mgmt_resume_prom_write(fbe_encl_mgmt_t * encl_mgmt,
                                             fbe_device_physical_location_t location,
                                             fbe_u8_t * buffer,
                                             fbe_u32_t buffer_size,
                                             RESUME_PROM_FIELD field)
 ****************************************************************
 * @brief
 *  This function write resume prom info into enclosure.
 *
 * @param pEnclMgmt -
 * @param location - The location of the enclosure/LCC.
 * @param buffer - write info
 * @param buffer_size - buffer size.
 * @param field - enclosure or lcc
 *
 * @return fbe_status_t
 *
 * @version
 *  16-Aug-2012:  Dongz - Created.
 *
 ****************************************************************/
fbe_status_t fbe_encl_mgmt_resume_prom_write(fbe_encl_mgmt_t * encl_mgmt,
                                             fbe_device_physical_location_t location,
                                             fbe_u8_t * buffer,
                                             fbe_u32_t buffer_size,
                                             RESUME_PROM_FIELD field,
                                             fbe_u64_t device_type,
                                             fbe_bool_t issue_read)
{
    fbe_resume_prom_cmd_t                   writeResumePromCmd;
    fbe_status_t status;

    //build resume prom command
    fbe_zero_memory(&writeResumePromCmd, sizeof(fbe_resume_prom_cmd_t));
    writeResumePromCmd.deviceType = device_type;
    writeResumePromCmd.deviceLocation.bus = location.bus;
    writeResumePromCmd.deviceLocation.enclosure = location.enclosure;
    writeResumePromCmd.deviceLocation.slot = 0;
    writeResumePromCmd.pBuffer = buffer;
    writeResumePromCmd.bufferSize = buffer_size;
    writeResumePromCmd.resumePromField = field;
    writeResumePromCmd.offset = 0;
    writeResumePromCmd.readOpStatus = FBE_RESUME_PROM_STATUS_INVALID;

    //call base envrionment api
    status = fbe_base_env_send_resume_prom_write_sync_cmd(
            (fbe_base_environment_t *)encl_mgmt,
            &writeResumePromCmd,
            issue_read);
    return status;
}

/*!**************************************************************
 * @fn fbe_encl_mgmt_set_resume_prom_system_id_info(fbe_encl_mgmt_t * encl_mgmt,
                                            fbe_device_physical_location_t pe_location,
                                            fbe_encl_mgmt_modify_system_id_info_t *modify_sys_info)
 ****************************************************************
 * @brief
 *  This function write pe resume prom by the modify command.
 *
 * @param encl_mgmt -
 * @param pe_location - The location of processor enclosure.
 * @param modify_sys_info - indicate which field need to update
 *
 * @return fbe_status_t
 *
 * @version
 *  28-Aug-2012:  Dongz - Created.
 *
 ****************************************************************/
fbe_status_t fbe_encl_mgmt_set_resume_prom_system_id_info(fbe_encl_mgmt_t * encl_mgmt,
                                            fbe_device_physical_location_t pe_location,
                                            fbe_encl_mgmt_modify_system_id_info_t *modify_sys_info)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_bool_t   needToIssueRead = FBE_FALSE;

    fbe_base_object_trace((fbe_base_object_t*)encl_mgmt,
            FBE_TRACE_LEVEL_INFO,
            FBE_TRACE_MESSAGE_ID_INFO,
            "%s change sn: %d, change pn: %d, change rev: %d.\n",
            __FUNCTION__,
            modify_sys_info->changeSerialNumber,
            modify_sys_info->changePartNumber,
            modify_sys_info->changeRev);

    //send resume prom write command
    if(modify_sys_info->changePartNumber &&
            fbe_encl_mgmt_validate_part_number(modify_sys_info->partNumber))
    {
        status = fbe_encl_mgmt_resume_prom_write(encl_mgmt,
                pe_location,
                modify_sys_info->partNumber,
                RESUME_PROM_PRODUCT_PN_SIZE,
                RESUME_PROM_PRODUCT_PN,
                FBE_DEVICE_TYPE_ENCLOSURE,
                FALSE);
        fbe_base_object_trace((fbe_base_object_t*)encl_mgmt,
                FBE_TRACE_LEVEL_INFO,
                FBE_TRACE_MESSAGE_ID_INFO,
                "%s, part number %s.\n",
                __FUNCTION__, modify_sys_info->partNumber);

        if(status != FBE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t*)encl_mgmt,
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s, can't write system info part number!.\n",
                                  __FUNCTION__);
            return status;
        }
        modify_sys_info->changePartNumber = FALSE;
        needToIssueRead = FBE_TRUE;
    }

    if(modify_sys_info->changeSerialNumber &&
            fbe_encl_mgmt_validate_serial_number(modify_sys_info->serialNumber))
    {
        status = fbe_encl_mgmt_resume_prom_write(encl_mgmt,
                pe_location,
                modify_sys_info->serialNumber,
                RESUME_PROM_PRODUCT_SN_SIZE,
                RESUME_PROM_PRODUCT_SN,
                FBE_DEVICE_TYPE_ENCLOSURE,
                FALSE);

        fbe_base_object_trace((fbe_base_object_t*)encl_mgmt,
                FBE_TRACE_LEVEL_INFO,
                FBE_TRACE_MESSAGE_ID_INFO,
                "%s, serial number %s.\n",
                __FUNCTION__, modify_sys_info->serialNumber);

        if(status != FBE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t*)encl_mgmt,
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s, can't write system info serial number!.\n",
                                  __FUNCTION__);
            return status;
        }
        modify_sys_info->changeSerialNumber = FALSE;
        needToIssueRead = FBE_TRUE;
    }

    if(modify_sys_info->changeRev &&
            fbe_encl_mgmt_validate_rev(modify_sys_info->revision))
    {
        status = fbe_encl_mgmt_resume_prom_write(encl_mgmt,
                pe_location,
                modify_sys_info->revision,
                RESUME_PROM_PRODUCT_REV_SIZE,
                RESUME_PROM_PRODUCT_REV,
                FBE_DEVICE_TYPE_ENCLOSURE,
                FALSE);

        fbe_base_object_trace((fbe_base_object_t*)encl_mgmt,
                FBE_TRACE_LEVEL_INFO,
                FBE_TRACE_MESSAGE_ID_INFO,
                "%s, revision %s.\n",
                __FUNCTION__, modify_sys_info->revision);

        if(status != FBE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t*)encl_mgmt,
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s, can't write system info revision!.\n",
                                  __FUNCTION__);
            return status;
        }
        modify_sys_info->changeRev = FALSE;
        needToIssueRead = FBE_TRUE;
    }

    if(needToIssueRead) 
    {
        //issue rp read
        status = fbe_encl_mgmt_initiate_resume_prom_read(encl_mgmt,
                                              FBE_DEVICE_TYPE_ENCLOSURE,
                                              &pe_location);
    
        if(status != FBE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t*)encl_mgmt,
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s, failed to initiate resume read after write\n",
                                  __FUNCTION__);
        }
    }

    return FBE_STATUS_OK;
}


/*!**************************************************************
 * @fn fbe_encl_mgmt_set_resume_prom_system_id_info_async(fbe_encl_mgmt_t * encl_mgmt,
                                            fbe_device_physical_location_t pe_location,
                                            fbe_encl_mgmt_modify_system_id_info_t *modify_sys_info)
 ****************************************************************
 * @brief
 *  This function write pe resume prom async by the modify command.
 *
 * @param encl_mgmt -
 * @param pe_location - The location of processor enclosure.
 * @param modify_sys_info - indicate which field need to update
 *
 * @return fbe_status_t
 *
 * @version
 *  17-Dec-2012:  Dipak - Created.
 *
 ****************************************************************/
fbe_status_t fbe_encl_mgmt_set_resume_prom_system_id_info_async(fbe_encl_mgmt_t * encl_mgmt,
                                            fbe_device_physical_location_t pe_location,
                                            fbe_encl_mgmt_modify_system_id_info_t *modify_sys_info)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_resume_prom_write_resume_async_op_t *rpWriteAsynchOp = NULL;
    fbe_resume_prom_system_id_info_t rpSysInfo = {0};
    fbe_base_env_resume_prom_info_t          * pResumePromInfo = NULL;
    fbe_u8_t * pdst = NULL;


    rpWriteAsynchOp = fbe_base_env_memory_ex_allocate((fbe_base_environment_t *)encl_mgmt, 
                                                  sizeof(fbe_resume_prom_write_resume_async_op_t));
   
    fbe_base_object_trace((fbe_base_object_t*)encl_mgmt,
            FBE_TRACE_LEVEL_INFO,
            FBE_TRACE_MESSAGE_ID_INFO,
            "%s change sn: %d, change pn: %d, change rev: %d.\n",
            __FUNCTION__,
            modify_sys_info->changeSerialNumber,
            modify_sys_info->changePartNumber,
            modify_sys_info->changeRev);

    fbe_base_object_trace((fbe_base_object_t*)encl_mgmt,
            FBE_TRACE_LEVEL_INFO,
            FBE_TRACE_MESSAGE_ID_INFO,
            "%s sn: %.16s, pn: %.16s, rev: %.16s.\n",
            __FUNCTION__,
            modify_sys_info->serialNumber,
            modify_sys_info->partNumber,
            modify_sys_info->revision);

    /* Call the appropriate device to get the pointer to the resume prom info. */
    status = ((fbe_base_environment_t *)encl_mgmt)->resume_prom_element.pGetResumePromInfoPtrCallback((fbe_base_environment_t *) encl_mgmt, 
                                                       FBE_DEVICE_TYPE_ENCLOSURE,
                                                       &pe_location,
                                                       &pResumePromInfo);

    /* Get offset as starting address*/
    pdst = (fbe_u8_t *)(&pResumePromInfo->resume_prom_data) + fbe_base_env_resume_prom_getFieldOffset(RESUME_PROM_PRODUCT_PN);

    /* Buffer size */
    rpWriteAsynchOp->rpWriteAsynchCmd.bufferSize = RESUME_PROM_PRODUCT_PN_SIZE +
                                                   RESUME_PROM_PRODUCT_SN_SIZE +
                                                   RESUME_PROM_PRODUCT_REV_SIZE;

    /* Copy resume prom data into system id info structure */
    fbe_copy_memory(&rpSysInfo, 
                    pdst,
                    rpWriteAsynchOp->rpWriteAsynchCmd.bufferSize);

    /* Verify the changed system ID infomation */
    if(modify_sys_info->changePartNumber &&
            fbe_encl_mgmt_validate_part_number(modify_sys_info->partNumber))
    {
        fbe_copy_memory(rpSysInfo.rpPartNum,
            modify_sys_info->partNumber,
            RESUME_PROM_PRODUCT_PN_SIZE);
    }

    if(modify_sys_info->changeSerialNumber &&
            fbe_encl_mgmt_validate_serial_number(modify_sys_info->serialNumber))
    {
        fbe_copy_memory(rpSysInfo.rpSerialNum,
            modify_sys_info->serialNumber,
            RESUME_PROM_PRODUCT_SN_SIZE);
    }

    if(modify_sys_info->changeRev &&
            fbe_encl_mgmt_validate_rev(modify_sys_info->revision))
    {
        fbe_copy_memory(rpSysInfo.rpRev,
            modify_sys_info->revision,
            RESUME_PROM_PRODUCT_REV_SIZE);
    }

    //build the write resume prom command
    rpWriteAsynchOp->rpWriteAsynchCmd.deviceType = FBE_DEVICE_TYPE_ENCLOSURE;
    rpWriteAsynchOp->rpWriteAsynchCmd.deviceLocation.bus = pe_location.bus;
    rpWriteAsynchOp->rpWriteAsynchCmd.deviceLocation.enclosure = pe_location.enclosure;
    rpWriteAsynchOp->rpWriteAsynchCmd.deviceLocation.slot = 0;
    rpWriteAsynchOp->rpWriteAsynchCmd.pBuffer = &rpSysInfo;
    rpWriteAsynchOp->rpWriteAsynchCmd.offset = fbe_base_env_resume_prom_getFieldOffset(RESUME_PROM_PRODUCT_PN);
    rpWriteAsynchOp->rpWriteAsynchCmd.writeOpStatus = FBE_RESUME_PROM_STATUS_INVALID;

    /* Set the completion function and context */
    rpWriteAsynchOp->completion_function = fbe_encl_mgmt_resume_prom_write_async_completion_function; 
    rpWriteAsynchOp->encl_mgmt = encl_mgmt; 
    
    /* Now send resume write command */
    status = fbe_base_env_send_resume_prom_write_async_cmd((fbe_base_environment_t *) encl_mgmt, 
                                                           rpWriteAsynchOp);

    if (status != FBE_STATUS_OK && status != FBE_STATUS_PENDING)
    {
        fbe_base_object_trace((fbe_base_object_t*)encl_mgmt,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s, Failed to write resume prom data\n",
                              __FUNCTION__);

        /* Free the allocated memory */
        fbe_base_env_memory_ex_release((fbe_base_environment_t *)encl_mgmt, rpWriteAsynchOp);
        rpWriteAsynchOp = NULL;

        return status;
    }
    return FBE_STATUS_OK;
}
