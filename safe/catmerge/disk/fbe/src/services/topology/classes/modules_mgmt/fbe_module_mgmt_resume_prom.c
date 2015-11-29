/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_module_mgmt_resume_prom.c
 ***************************************************************************
 * @brief
 *  This file contains the implemention for Module management Resume Prom functionality.
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
#include "fbe/fbe_api_board_interface.h"
#include "fbe_base_object.h"
#include "fbe_module_mgmt_private.h"
#include "fbe_base_environment_debug.h"

static fbe_u8_t fbe_module_mgmt_validate_device_type(fbe_u64_t device_type);

static fbe_status_t 
fbe_module_mgmt_resume_prom_generate_logheader(fbe_u64_t device_type,
                                               fbe_device_physical_location_t * pLocation, 
                                               fbe_u8_t * pLogHeader, 
                                               fbe_u32_t logHeaderSize);

/*!**************************************************************
 * @fn fbe_board_mgmt_initiate_resume_prom_cmd()
 ****************************************************************
 * @brief
 *  This function initiates the async resume prom cmd.
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
fbe_status_t fbe_module_mgmt_initiate_resume_prom_read(fbe_module_mgmt_t * pModuleMgmt,
                                                     fbe_u64_t deviceType,
                                                     fbe_device_physical_location_t * pLocation)
{
    fbe_u8_t                          logHeader[FBE_BASE_ENV_RESUME_PROM_LOG_HEADER_SIZE + 1] = {0};
    fbe_base_env_resume_prom_info_t * pResumePromInfo = NULL;
    fbe_status_t                      status = FBE_STATUS_OK;

    fbe_module_mgmt_get_resume_prom_info_ptr(pModuleMgmt,deviceType,pLocation,&pResumePromInfo);

    /* Save the device type, Location and target object id */
    pResumePromInfo->device_type = deviceType;
    pResumePromInfo->location = *pLocation;
    pResumePromInfo->objectId = pModuleMgmt->board_object_id;

    /* Set log header for tracing purpose. */
    fbe_module_mgmt_resume_prom_generate_logheader(deviceType, pLocation, 
                                                   &logHeader[0], FBE_BASE_ENV_RESUME_PROM_LOG_HEADER_SIZE);


    /* Create and add the work item to the queue. */
    status = fbe_base_env_resume_prom_create_and_add_work_item((fbe_base_environment_t *)pModuleMgmt,
                                                               pModuleMgmt->board_object_id,
                                                               deviceType,
                                                               pLocation, 
                                                               &logHeader[0]);

    return status;
}

/*!**************************************************************
 * @fn fbe_module_mgmt_resume_prom_generate_logheader()
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
static fbe_status_t fbe_module_mgmt_resume_prom_generate_logheader(fbe_u64_t device_type, 
                                                                   fbe_device_physical_location_t * pLocation, 
                                                                   fbe_u8_t * pLogHeader, 
                                                                   fbe_u32_t logHeaderSize)
{
    fbe_zero_memory(pLogHeader, logHeaderSize);

    switch(device_type)
    {
        case FBE_DEVICE_TYPE_IOMODULE:
            csx_p_snprintf(pLogHeader, logHeaderSize, "RP:IOM(SP: %d, Slot: %d)",
                               pLocation->sp, pLocation->slot);
            break;

        case FBE_DEVICE_TYPE_BACK_END_MODULE:
            csx_p_snprintf(pLogHeader, logHeaderSize, "RP:BM(SP: %d, Slot: %d)",
                               pLocation->sp, pLocation->slot);
            break;

        case FBE_DEVICE_TYPE_MGMT_MODULE:
            csx_p_snprintf(pLogHeader, logHeaderSize, "RP:MGMT(SP: %d, Slot: %d)",
                               pLocation->sp, pLocation->slot);
            break;

        case FBE_DEVICE_TYPE_MEZZANINE:
            csx_p_snprintf(pLogHeader, logHeaderSize, "RP:MEZZ(SP: %d, Slot: %d)",
                               pLocation->sp, pLocation->slot);
            break;

        default:
            break;
    }

    return FBE_STATUS_OK;
}

/*!**************************************************************
 * @fn fbe_module_mgmt_validate_device_type(fbe_u64_t deviceType)
 ****************************************************************
 * @brief
 *  This function validates the Module Management device_types.
 *
 * @param deviceType - The PE Component device type.
 *
 * @return fbe_u8_t
 *
 * @version
 *  16-Dec-2010:  Arun S - Created. 
 *
 ****************************************************************/
static fbe_u8_t fbe_module_mgmt_validate_device_type(fbe_u64_t device_type)
{
    fbe_u8_t is_device_valid = TRUE;

    switch(device_type)
    {
        case FBE_DEVICE_TYPE_BACK_END_MODULE:
        case FBE_DEVICE_TYPE_IOMODULE:
        case FBE_DEVICE_TYPE_MEZZANINE:
        case FBE_DEVICE_TYPE_MGMT_MODULE:
            is_device_valid = TRUE;
            break;
    
        default:
            is_device_valid = FALSE;
            break;
    }

    return is_device_valid;
}

/*!**************************************************************
 * @fn fbe_module_mgmt_get_resume_prom_info_ptr(void * pContext,
 *                                         fbe_u64_t deviceType,
 *                                         fbe_device_physical_location_t * pLocation,
 *                                         fbe_base_env_resume_prom_info_t ** pResumePromInfoPtr)
 ****************************************************************
 * @brief
 *  This function gets the pointer which points to the resume prom info
 *  for the specifid device.
 * 
 * @param pContext - The pointer to the callback context.
 * @param deviceType - The PE Component device type.
 * @param pLocation - Location of the device.
 * @param pResumeInfo - Pointer to the resume prom information.
 *
 * @return fbe_status_t
 *
 * @version
 *  08-Nov-2010:  PHE - Created. 
 *
 ****************************************************************/
fbe_status_t fbe_module_mgmt_get_resume_prom_info_ptr(fbe_module_mgmt_t * pModuleMgmt,
                                                      fbe_u64_t deviceType,
                                                      fbe_device_physical_location_t * pLocation,
                                                      fbe_base_env_resume_prom_info_t ** pResumePromInfoPtr)
{
    fbe_u32_t                                module_num = 0;
    fbe_u8_t                                 is_device_valid = TRUE;

    *pResumePromInfoPtr = NULL;

    is_device_valid = fbe_module_mgmt_validate_device_type(deviceType);

    if(!is_device_valid)
    {
        fbe_base_object_trace((fbe_base_object_t *)pModuleMgmt,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "RP: %s, unsupported deviceType %s(%lld).\n",
                              __FUNCTION__,
                              fbe_base_env_decode_device_type(deviceType),
                              deviceType);

        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    module_num = fbe_module_mgmt_convert_device_type_and_slot_to_index(deviceType, pLocation->slot);

    /* Copy the data from the work item to the appropriate data structure */
    if(deviceType == FBE_DEVICE_TYPE_MGMT_MODULE)
    {
        *pResumePromInfoPtr = &pModuleMgmt->mgmt_module_info[module_num].mgmt_module_resume_info[pLocation->sp];
    }
    else
    {
        *pResumePromInfoPtr = &pModuleMgmt->io_module_info[module_num].physical_info[pLocation->sp].module_resume_info;
    }
    
    return FBE_STATUS_OK;
} // End of function fbe_module_mgmt_get_resume_prom_info_ptr


/*!**************************************************************
 * @fn fbe_module_mgmt_resume_prom_handle_io_comp_status_change(fbe_module_mgmt_t * pModuleMgmt,
 *                                         fbe_device_physical_location_t * pLocation,
 *                                         fbe_board_mgmt_io_comp_info_t * pNewIoCompInfo,
 *                                         fbe_board_mgmt_io_comp_info_t * pOldIoCompInfo)
 ****************************************************************
 * @brief
 *  This function initiates the Module mgmt Resume prom reads or clears it 
 *  based on the IO module state change.
 * 
 * @param pModuleMgmt -
 * @param pLocation - The location of the module.
 * @param pNewIoCompInfo - The new io component info.
 * @param pOldIoCompInfo - The old io component info.
 * 
 * @return fbe_status_t 
 *
 * @version
 *  19-Jan-2011:  Arun S - Created. 
 *
 ****************************************************************/
fbe_status_t fbe_module_mgmt_resume_prom_handle_io_comp_status_change(fbe_module_mgmt_t * pModuleMgmt, 
                                                                      fbe_device_physical_location_t * pLocation, 
                                                                      fbe_u64_t device_type, 
                                                                      fbe_board_mgmt_io_comp_info_t * pNewIoCompInfo, 
                                                                      fbe_board_mgmt_io_comp_info_t * pOldIoCompInfo)
{
    fbe_status_t                     status = FBE_STATUS_OK;

    /* Check the state of the inserted bit for IO component */
    if((!pNewIoCompInfo->inserted) && (pOldIoCompInfo->inserted))
    {
        /* The IO Module component was removed. */
        fbe_base_object_trace((fbe_base_object_t *)pModuleMgmt, 
                              FBE_TRACE_LEVEL_WARNING, 
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                              "RP: Module Mgmt - Device removed. SP: %d, Slot: %d.\n", 
                              pLocation->sp, 
                              pLocation->slot);

        /* The IO Module component was removed. */
        status = fbe_base_env_resume_prom_handle_device_removal((fbe_base_environment_t *)pModuleMgmt, 
                                                                device_type, 
                                                                pLocation);
        if(status != FBE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t *)pModuleMgmt,
                                  FBE_TRACE_LEVEL_WARNING,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "RP: Module Mgmt - Failed to handle device removal, SP: %d, Slot: %d.\n",
                                  pLocation->sp,
                                  pLocation->slot);

            return status;
        }
    }
    else 
    {
        /* Check the state of the inserted bit for IO Component. Issue the resume prom read 
         * 'only' if the status is changed from NOT INSERTED to INSERTED. This is to avoid 
         * issuing the resume read for all the other cases. 
         */
        if((pNewIoCompInfo->inserted) && (!pOldIoCompInfo->inserted))
        {
            /* Create and add the work item to the queue. */
            status = fbe_module_mgmt_initiate_resume_prom_read(pModuleMgmt,
                                                               device_type, 
                                                               pLocation);
            if(status != FBE_STATUS_OK)
            {
                fbe_base_object_trace((fbe_base_object_t *)pModuleMgmt,
                                      FBE_TRACE_LEVEL_WARNING,
                                      FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                      "RP: Module Mgmt - Failed to create_and_add_work_item, SP: %d, Slot: %d.\n",
                                      pLocation->sp,
                                      pLocation->slot);
            }
        }
    }

    return status;
}

/*!**************************************************************
 * @fn fbe_module_mgmt_resume_prom_handle_mgmt_module_status_change(fbe_module_mgmt_t * pModuleMgmt,
 *                                         fbe_device_physical_location_t * pLocation,
 *                                         fbe_board_mgmt_mgmt_module_info_t * pNewMgmtModInfo,
 *                                         fbe_board_mgmt_mgmt_module_info_t * pOldMgmtModInfo)
 ****************************************************************
 * @brief
 *  This function initiates the Module mgmt Resume prom reads or clears it 
 *  based on the management module state change.
 * 
 * @param pModuleMgmt -
 * @param pLocation - The location of the module.
 * @param pNewMgmtModInfo - The new management module info.
 * @param pOldMgmtModInfo - The old management module info.
 * 
 * @return fbe_status_t 
 *
 * @version
 *  19-Jan-2011:  Arun S - Created. 
 *
 ****************************************************************/
fbe_status_t fbe_module_mgmt_resume_prom_handle_mgmt_module_status_change(fbe_module_mgmt_t * pModuleMgmt, 
                                                                          fbe_device_physical_location_t * pLocation, 
                                                                          fbe_u64_t device_type, 
                                                                          fbe_board_mgmt_mgmt_module_info_t * pNewMgmtModInfo, 
                                                                          fbe_board_mgmt_mgmt_module_info_t * pOldMgmtModInfo)
{
    fbe_status_t                    status = FBE_STATUS_OK;

    /* Check the state of the inserted bit for Management module */
    if((!pNewMgmtModInfo->bInserted) && (pOldMgmtModInfo->bInserted))
    {
        /* The Management Module was removed. */
        fbe_base_object_trace((fbe_base_object_t *)pModuleMgmt, 
                              FBE_TRACE_LEVEL_WARNING, 
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                              "RP: Module Mgmt - Device removed. SP: %d, Slot: %d.\n", 
                              pLocation->sp, 
                              pLocation->slot);

        /* The Management Module was removed. */
        status = fbe_base_env_resume_prom_handle_device_removal((fbe_base_environment_t *)pModuleMgmt, 
                                                                FBE_DEVICE_TYPE_MGMT_MODULE, 
                                                                pLocation);
        if(status != FBE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t *)pModuleMgmt,
                                  FBE_TRACE_LEVEL_WARNING,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "RP: Module Mgmt - Failed to handle device removal, SP: %d, Slot: %d.\n",
                                  pLocation->sp,
                                  pLocation->slot);

            return status;
        }
    }
    else 
    {
        /* Check the state of the inserted bit for Management module.
         * Issue the resume prom read 'only' if the status is changed from
         * NOT INSERTED to INSERTED. This is to avoid issuing the resume read 
         * for all the other cases. 
         */
        if((pNewMgmtModInfo->bInserted) && (!pOldMgmtModInfo->bInserted))
        {
            /* Create and add the work item to the queue. */
            status = fbe_module_mgmt_initiate_resume_prom_read(pModuleMgmt,
                                                               device_type,
                                                               pLocation);
    
            if(status != FBE_STATUS_OK)
            {
                fbe_base_object_trace((fbe_base_object_t *)pModuleMgmt,
                                      FBE_TRACE_LEVEL_WARNING,
                                      FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                      "RP: Module Mgmt - Failed to initiate resume read for SP %d, Slot %d.\n",
                                      pLocation->sp,
                                      pLocation->slot);
            }
        }
    }

    return status;
}

/*!**************************************************************
 * @fn fbe_module_mgmt_resume_prom_update_encl_fault_led(fbe_module_mgmt_t * pModuleMgmt,
                                    fbe_device_physical_location_t *pLocation)
 ****************************************************************
 * @brief
 *  This function updates the enclosure fault led for the resume prom fault.
 *
 * @param pModuleMgmt -
 * @param pLocation - The location of the management module.
 *
 * @return fbe_status_t
 *
 * @version
 *  28-Mar-2013:  Lin Lou - Created.
 *
 ****************************************************************/

fbe_status_t fbe_module_mgmt_resume_prom_update_encl_fault_led(fbe_module_mgmt_t * pModuleMgmt,
                                                               fbe_u64_t deviceType,
                                                               fbe_device_physical_location_t *pLocation)
{
    fbe_status_t status;
    fbe_encl_fault_led_reason_t enclFaultLedReason;

    fbe_base_object_trace((fbe_base_object_t *)pModuleMgmt,
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "RP:ModuleMgmt, entry for deviceType %s(%lld), sp:%d, slot:%d.\n",
                          fbe_base_env_decode_device_type(deviceType),
                          deviceType,
                          pLocation->sp, pLocation->slot);

    switch(deviceType)
    {
    case FBE_DEVICE_TYPE_MGMT_MODULE:
        enclFaultLedReason = FBE_ENCL_FAULT_LED_MGMT_MODULE_RESUME_PROM_FLT;
        break;
    case FBE_DEVICE_TYPE_IOMODULE:
        enclFaultLedReason = FBE_ENCL_FAULT_LED_IO_MODULE_RESUME_PROM_FLT;
        break;
    case FBE_DEVICE_TYPE_BACK_END_MODULE:
        enclFaultLedReason = FBE_ENCL_FAULT_LED_BEM_RESUME_PROM_FLT;
        break;
    case FBE_DEVICE_TYPE_MEZZANINE:
        enclFaultLedReason = FBE_ENCL_FAULT_LED_MEZZANINE_RESUME_PROM_FLT;
        break;

    default:
        fbe_base_object_trace((fbe_base_object_t *)pModuleMgmt,
                          FBE_TRACE_LEVEL_WARNING,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "RP:ModuleMgmt, unsupported deviceType %s(%lld), sp:%d, slot:%d.\n",
                          fbe_base_env_decode_device_type(deviceType),
                          deviceType,
                          pLocation->sp, pLocation->slot);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_module_mgmt_update_encl_fault_led(pModuleMgmt,
                                                   pLocation,
                                                   deviceType,
                                                   enclFaultLedReason);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)pModuleMgmt,
                          FBE_TRACE_LEVEL_WARNING,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "RP:%s - update_encl_fault_led failed for SP %d, Slot %d, status: 0x%X.\n",
                          fbe_base_env_decode_device_type(deviceType),
                          pLocation->sp, pLocation->slot, status);
    }
    return status;
}

/*End of file fbe_module_mgmt_resume_prom.c */
