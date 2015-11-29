/***************************************************************************
 * Copyright (C) EMC Corporation 2010 
 * All rights reserved. Licensed material -- property of EMC 
 * Corporation 
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_cooling_mgmt_usurper.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the Cooling Management object lifecycle
 *  code.
 * 
 *  This includes the
 *  @ref fbe_cooling_mgmt_monitor_entry "cooling_mgmt monitor entry
 *  point", as well as all the lifecycle defines such as
 *  rotaries and conditions, along with the actual condition
 *  functions.
 * 
 * @ingroup cooling_mgmt_class_files
 * 
 * @version
 *   17-Jan-2011:  PHE - Created.
 *
 ***************************************************************************/
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_memory.h"
#include "fbe_topology.h"
#include "fbe_base_object.h"
#include "fbe_scheduler.h"
#include "fbe_cooling_mgmt_private.h"
#include "fbe/fbe_api_common.h"
#include "fbe_transport_memory.h"
#include "fbe/fbe_esp_cooling_mgmt.h"
#include "fbe/fbe_api_esp_cooling_mgmt_interface.h"
#include "fbe_base_environment_debug.h"


static fbe_status_t 
fbe_cooling_mgmt_get_fan_count(fbe_cooling_mgmt_t * pCoolingMgmt, 
                                   fbe_packet_t * packet);
static fbe_status_t 
fbe_cooling_mgmt_get_fan_info(fbe_cooling_mgmt_t * pCoolingMgmt, 
                                  fbe_packet_t * packet);
static fbe_status_t 
fbe_cooling_mgmt_getCacheStatus(fbe_cooling_mgmt_t * pCoolingMgmt, 
                                fbe_packet_t * packet);


/*!***************************************************************
 * fbe_cooling_mgmt_control_entry()
 ****************************************************************
 * @brief
 *  This function is entry point for control operation for this
 *  class
 *
 * @param  - None.
 *
 * @return fbe_status_t
 *
 * @author
 *  17-Jan-2011: PHE - Created.
 *
 ****************************************************************/
fbe_status_t fbe_cooling_mgmt_control_entry(fbe_object_handle_t object_handle, 
                                        fbe_packet_t * packet)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_payload_control_operation_opcode_t  control_code;
    fbe_cooling_mgmt_t                     *pCoolingMgmt = NULL;
    
    pCoolingMgmt = (fbe_cooling_mgmt_t *)fbe_base_handle_to_pointer(object_handle);

    control_code = fbe_transport_get_control_code(packet);
    fbe_base_object_trace((fbe_base_object_t*)pCoolingMgmt,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry, control_code 0x%x\n", 
                          __FUNCTION__, control_code);
    switch(control_code) 
    {
    case FBE_ESP_COOLING_MGMT_CONTROL_CODE_GET_FAN_INFO:
        status = fbe_cooling_mgmt_get_fan_info(pCoolingMgmt, packet);
        break;

    case FBE_ESP_COOLING_MGMT_CONTROL_CODE_GET_CACHE_STATUS:
        status = fbe_cooling_mgmt_getCacheStatus(pCoolingMgmt, packet);
        break;

    default:
        status =  fbe_base_environment_control_entry (object_handle, packet);
        break;
    }

    return status;

}
/******************************************
 * end fbe_cooling_mgmt_control_entry()
 ******************************************/


/*!***************************************************************
 * fbe_cooling_mgmt_get_fan_info()
 ****************************************************************
 * @brief
 *  This function gets the Fan info.
 *
 * @param pCoolingMgmt - 
 * @param packet - 
 *
 * @return fbe_status_t
 *
 * @author
 *  17-Jan-2011: PHE - Created. 
 *
 ****************************************************************/
static fbe_status_t 
fbe_cooling_mgmt_get_fan_info(fbe_cooling_mgmt_t * pCoolingMgmt, 
                                  fbe_packet_t * packet)
{  
    fbe_payload_ex_t                             * payload = NULL;
    fbe_payload_control_operation_t           * control_operation = NULL;
    fbe_payload_control_buffer_length_t         control_buffer_length = 0;
    fbe_esp_cooling_mgmt_get_fan_info_t       * pGetFanInfo = NULL;
    fbe_cooling_mgmt_fan_info_t               * pFanInfo = NULL;
    fbe_status_t                                status = FBE_STATUS_OK;

    payload = fbe_transport_get_payload_ex(packet);
    if(payload == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t *)pCoolingMgmt,
                            FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                            "%s, get_payload failed.\n", __FUNCTION__); 

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    control_operation = fbe_payload_ex_get_control_operation(payload);
    if(control_operation == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t *)pCoolingMgmt,
                            FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                            "%s, get_control_operation failed.\n", __FUNCTION__); 

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status =  fbe_payload_control_get_buffer(control_operation, &pGetFanInfo);
    if( (pGetFanInfo == NULL) || (status!=FBE_STATUS_OK))
    {
        fbe_base_object_trace((fbe_base_object_t *)pCoolingMgmt,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s, get_buffer failed, status: 0x%x, pGetFanInfo: %64p.\n",
                               __FUNCTION__, status, pGetFanInfo);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Verify the control buffer size with the size of the expected structure */
    status = fbe_payload_control_get_buffer_length(control_operation, &control_buffer_length);
    if((status != FBE_STATUS_OK) || 
       (control_buffer_length != sizeof(fbe_esp_cooling_mgmt_get_fan_info_t)))
    {
        fbe_base_object_trace((fbe_base_object_t *)pCoolingMgmt,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                                "%s, get_buffer_length, bufferLen %d, expectedLen: %llu, status: 0x%x.\n", 
                                __FUNCTION__,
                                control_buffer_length, 
                                (unsigned long long)sizeof(fbe_esp_cooling_mgmt_get_fan_info_t), status);
        
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    } 

    // get the cooling info for the specified cooling component.
    status = fbe_cooling_mgmt_get_fan_info_by_location(pCoolingMgmt,
                                                      &pGetFanInfo->location,
                                                      &pFanInfo);
    if (status != FBE_STATUS_OK)
    {
        pGetFanInfo->fanInfo.envInterfaceStatus = FBE_ENV_INTERFACE_STATUS_GOOD;
        pGetFanInfo->fanInfo.inserted = FBE_MGMT_STATUS_FALSE;
        pGetFanInfo->fanInfo.dataStale = FALSE;
        pGetFanInfo->fanInfo.environmentalInterfaceFault = FALSE;
    }
    else
    {
        pGetFanInfo->fanInfo.envInterfaceStatus = pFanInfo->basicInfo.envInterfaceStatus;
        pGetFanInfo->fanInfo.inProcessorEncl = pFanInfo->basicInfo.inProcessorEncl;
        pGetFanInfo->fanInfo.associatedSp = pFanInfo->basicInfo.associatedSp;
        pGetFanInfo->fanInfo.slotNumOnSpBlade = pFanInfo->basicInfo.slotNumOnSpBlade;
        pGetFanInfo->fanInfo.inserted = pFanInfo->basicInfo.inserted;
        pGetFanInfo->fanInfo.fanLocation = pFanInfo->basicInfo.fanLocation;
        pGetFanInfo->fanInfo.fanFaulted = pFanInfo->basicInfo.fanFaulted;
        pGetFanInfo->fanInfo.fanDegraded = pFanInfo->basicInfo.fanDegraded;
        pGetFanInfo->fanInfo.resumePromSupported = pFanInfo->basicInfo.hasResumeProm;
        pGetFanInfo->fanInfo.fwUpdatable = pFanInfo->basicInfo.bDownloadable;
        pGetFanInfo->fanInfo.isFaultRegFail = pFanInfo->basicInfo.isFaultRegFail;
        pGetFanInfo->fanInfo.uniqueId = pFanInfo->basicInfo.uniqueId;
        pGetFanInfo->fanInfo.state = pFanInfo->basicInfo.state;
        pGetFanInfo->fanInfo.subState = pFanInfo->basicInfo.subState;
        fbe_copy_memory(&pGetFanInfo->fanInfo.firmwareRev,
                &pFanInfo->basicInfo.firmwareRev,
                FBE_ENCLOSURE_MAX_FIRMWARE_REV_SIZE + 1);
        // set environmental status
        if (pFanInfo->basicInfo.envInterfaceStatus == FBE_ENV_INTERFACE_STATUS_DATA_STALE)
        {
            pGetFanInfo->fanInfo.dataStale = TRUE;
            pGetFanInfo->fanInfo.environmentalInterfaceFault = FALSE;
        }
        else if (pFanInfo->basicInfo.envInterfaceStatus == FBE_ENV_INTERFACE_STATUS_XACTION_FAIL)
        {
            pGetFanInfo->fanInfo.dataStale = FALSE;
            pGetFanInfo->fanInfo.environmentalInterfaceFault = TRUE;
        }
        else
        {
            pGetFanInfo->fanInfo.dataStale = FALSE;
            pGetFanInfo->fanInfo.environmentalInterfaceFault = FALSE;
        }
    }
        
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
	return FBE_STATUS_OK;
}

/******************************************
 * end fbe_cooling_mgmt_get_fan_info()
 ******************************************/

/*!***************************************************************
 * fbe_cooling_mgmt_getCacheStatus()
 ****************************************************************
 * @brief
 *  This function gets the Cooling Mgmt Cache status.
 *
 * @param pCoolingMgmt - 
 * @param packet - 
 *
 * @return fbe_status_t
 *
 * @author
 *  25-Mar-2011:    Joe Perry - Created. 
 *
 ****************************************************************/
static fbe_status_t 
fbe_cooling_mgmt_getCacheStatus(fbe_cooling_mgmt_t * pCoolingMgmt, 
                                fbe_packet_t * packet)
{  
    fbe_payload_ex_t                             * payload = NULL;
    fbe_payload_control_operation_t           * control_operation = NULL;
    fbe_payload_control_buffer_length_t         control_buffer_length = 0;
    fbe_common_cache_status_t                 * pGetCacheStatus = NULL;
    fbe_common_cache_status_t                 peerGetCacheStatus = 0;
    fbe_status_t                                status = FBE_STATUS_OK;

    payload = fbe_transport_get_payload_ex(packet);
    if(payload == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t *)pCoolingMgmt,
                            FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                            "%s, get_payload failed.\n", __FUNCTION__); 

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    control_operation = fbe_payload_ex_get_control_operation(payload);
    if(control_operation == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t *)pCoolingMgmt,
                            FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                            "%s, get_control_operation failed.\n", __FUNCTION__); 

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status =  fbe_payload_control_get_buffer(control_operation, &pGetCacheStatus);
    if( (pGetCacheStatus == NULL) || (status!=FBE_STATUS_OK))
    {
        fbe_base_object_trace((fbe_base_object_t *)pCoolingMgmt,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s, get_buffer failed, pGetCacheStatus 0x%p, status: 0x%x\n",
                               __FUNCTION__, pGetCacheStatus, status);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Verify the control buffer size with the size of the expected structure */
    status = fbe_payload_control_get_buffer_length(control_operation, &control_buffer_length);
    if((status != FBE_STATUS_OK) || 
       (control_buffer_length != sizeof(fbe_common_cache_status_t)))
    {
        fbe_base_object_trace((fbe_base_object_t *)pCoolingMgmt,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                                "%s, get_buffer_length, bufferLen %d, expectedLen: %llu, status: 0x%x.\n", 
                                __FUNCTION__,
                                control_buffer_length, 
                                (unsigned long long)sizeof(fbe_common_cache_status_t), status);
        
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    status = fbe_base_environment_get_peerCacheStatus((fbe_base_environment_t *) pCoolingMgmt,
                                                      &peerGetCacheStatus);

    *pGetCacheStatus = fbe_base_environment_combine_cacheStatus((fbe_base_environment_t *) pCoolingMgmt,
                                                                 pCoolingMgmt->coolingCacheStatus,
                                                                 peerGetCacheStatus,
                                                                 FBE_CLASS_ID_COOLING_MGMT); 

    fbe_base_object_trace((fbe_base_object_t*)pCoolingMgmt, 
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s, pGetCacheStatus %d\n",
                          __FUNCTION__, *pGetCacheStatus);
        
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
	return FBE_STATUS_OK;
}

/******************************************
 * end fbe_cooling_mgmt_getCacheStatus()
 ******************************************/



/*!***************************************************************
 * fbe_cooling_mgmt_updateFanFailStatus()
 ****************************************************************
 * @brief
 *  This function will set the appropriate Fan failed status.
 *
 * @param  - None.
 *
 * @return fbe_status_t
 *
 * @author
 *  24-Apr-2014 - Created. Joe Perry
 *
 ****************************************************************/
fbe_status_t 
fbe_cooling_mgmt_updateFanFailStatus(fbe_u8_t *mgmtPtr,
                                     fbe_device_physical_location_t *pLocation,
                                     fbe_base_environment_failure_type_t failureType,
                                     fbe_bool_t newValue)
{
    fbe_status_t                    status;
    fbe_cooling_mgmt_t              *coolingMgmtPtr;
    fbe_u8_t                        deviceStr[FBE_DEVICE_STRING_LENGTH]={0};
    fbe_cooling_mgmt_fan_info_t     *pFanInfo = NULL;

    coolingMgmtPtr = (fbe_cooling_mgmt_t *)mgmtPtr;

    status = fbe_base_env_create_device_string(FBE_DEVICE_TYPE_FAN, 
                                               pLocation, 
                                               &deviceStr[0],
                                               FBE_DEVICE_STRING_LENGTH);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)coolingMgmtPtr, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                              "%s, Failed to create device string.\n", 
                              __FUNCTION__); 

        return status;
    }

    // get the cooling info for the specified cooling component.
    status = fbe_cooling_mgmt_get_fan_info_by_location(coolingMgmtPtr,
                                                      pLocation,
                                                      &pFanInfo);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)coolingMgmtPtr, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                              "%s, Failed to location Fan, status %d.\n", 
                              __FUNCTION__, status); 

        return status;
    }

    switch (failureType)
    {
    case FBE_BASE_ENV_RP_READ_FAILURE:
        fbe_base_object_trace((fbe_base_object_t *)coolingMgmtPtr, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s, %s, resPromReadFailed changed %d to %d.\n",
                              __FUNCTION__, 
                              &deviceStr[0],
                              pFanInfo->basicInfo.resumePromReadFailed,
                              newValue);
        pFanInfo->basicInfo.resumePromReadFailed = newValue;
        break;
    case FBE_BASE_ENV_FUP_FAILURE:
        fbe_base_object_trace((fbe_base_object_t *)coolingMgmtPtr, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s, %s, fupFailed changed %d to %d.\n",
                              __FUNCTION__, 
                              &deviceStr[0],
                              pFanInfo->basicInfo.fupFailure,
                              newValue);
        pFanInfo->basicInfo.fupFailure = newValue;
        break;
    default:
        fbe_base_object_trace((fbe_base_object_t *)coolingMgmtPtr, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                              "%s, Unsupported failureType %d.\n", 
                              __FUNCTION__, failureType); 
        return FBE_STATUS_COMPONENT_NOT_FOUND;
        break;
    }

    return FBE_STATUS_OK;

}   // end of fbe_cooling_mgmt_updateFanFailStatus

/* End of file fbe_cooling_mgmt_usurper.c*/
