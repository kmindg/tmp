/***************************************************************************
 * Copyright (C) EMC Corporation 2010 
 * All rights reserved. Licensed material -- property of EMC 
 * Corporation 
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_ps_mgmt_usurper.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the PS Management object lifecycle
 *  code.
 * 
 *  This includes the
 *  @ref fbe_ps_mgmt_monitor_entry "ps_mgmt monitor entry
 *  point", as well as all the lifecycle defines such as
 *  rotaries and conditions, along with the actual condition
 *  functions.
 * 
 * @ingroup ps_mgmt_class_files
 * 
 * @version
 *   15-Apr-2010:  Created. Joe Perry
 *
 ***************************************************************************/
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_memory.h"
#include "fbe_topology.h"
#include "fbe_base_object.h"
#include "fbe_scheduler.h"
#include "fbe_ps_mgmt_private.h"
#include "fbe/fbe_api_common.h"
#include "fbe_transport_memory.h"
#include "fbe/fbe_esp_ps_mgmt.h"
#include "fbe/fbe_api_esp_ps_mgmt_interface.h"
#include "fbe_base_environment_debug.h"


fbe_status_t fbe_ps_mgmt_getPsStatus(fbe_ps_mgmt_t *psMgmtPtr, 
                                       fbe_packet_t * packet);
fbe_status_t fbe_ps_mgmt_getPsCount(fbe_ps_mgmt_t *psMgmtPtr, 
                                    fbe_packet_t * packet);
fbe_status_t fbe_ps_mgmt_getCacheStatus(fbe_ps_mgmt_t *psMgmtPtr, 
                                          fbe_packet_t * packet);

static fbe_status_t 
fbe_ps_mgmt_ctrl_get_fup_completion_status(fbe_ps_mgmt_t * pPsMgmt, fbe_packet_t * packet);
fbe_status_t fbe_ps_mgmt_powerdownPs(fbe_ps_mgmt_t *psMgmtPtr, fbe_packet_t * packet);
fbe_status_t fbe_ps_mgmt_set_expected_ps_type(fbe_ps_mgmt_t *psMgmtPtr, fbe_packet_t * packet);

/*!***************************************************************
 * fbe_ps_mgmt_control_entry()
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
 *  15-Apr-2010 - Created. Joe Perry
 *
 ****************************************************************/
fbe_status_t fbe_ps_mgmt_control_entry(fbe_object_handle_t object_handle, 
                                        fbe_packet_t * packet)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_payload_control_operation_opcode_t  control_code;
    fbe_ps_mgmt_t                          *psMgmtPtr = NULL;
    
    psMgmtPtr = (fbe_ps_mgmt_t *)fbe_base_handle_to_pointer(object_handle);

    control_code = fbe_transport_get_control_code(packet);
    fbe_base_object_trace((fbe_base_object_t*)psMgmtPtr,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry, control_code 0x%x\n", 
                          __FUNCTION__, control_code);
    switch(control_code) 
    {
    case FBE_ESP_PS_MGMT_CONTROL_CODE_GET_PS_COUNT:
        status = fbe_ps_mgmt_getPsCount(psMgmtPtr, packet);
        break;

    case FBE_ESP_PS_MGMT_CONTROL_CODE_GET_PS_STATUS:
        status = fbe_ps_mgmt_getPsStatus(psMgmtPtr, packet);
        break;

    case FBE_ESP_PS_MGMT_CONTROL_CODE_GET_PS_CACHE_STATUS:
        status = fbe_ps_mgmt_getCacheStatus(psMgmtPtr, packet);
        break;

    case FBE_ESP_PS_MGMT_CONTROL_CODE_PS_POWERDOWN:
        status = fbe_ps_mgmt_powerdownPs(psMgmtPtr, packet);
        break;

    case FBE_ESP_PS_MGMT_CONTROL_CODE_SET_EXPECTED_PS_TYPE:
        status = fbe_ps_mgmt_set_expected_ps_type(psMgmtPtr, packet);
        break;

    default:
        status =  fbe_base_environment_control_entry (object_handle, packet);
        break;
    }

    return status;

}
/******************************************
 * end fbe_ps_mgmt_control_entry()
 ******************************************/

/*!***************************************************************
 * fbe_ps_mgmt_getPsStatus()
 ****************************************************************
 * @brief
 *  This function processes the GetPsStatus Control Code.
 *  class
 *
 * @param  - None.
 *
 * @return fbe_status_t
 *
 * @author
 *  15-Apr-2010 - Created. Joe Perry
 *
 ****************************************************************/
fbe_status_t fbe_ps_mgmt_getPsStatus(fbe_ps_mgmt_t *psMgmtPtr, 
                                     fbe_packet_t * packet)
{
    fbe_esp_ps_mgmt_ps_info_t          *psStatusInfoPtr = NULL;
    fbe_payload_control_buffer_length_t len = 0;
    fbe_payload_ex_t                       *payload = NULL;
    fbe_payload_control_operation_t     *control_operation = NULL;
    fbe_encl_ps_info_t                 * pEnclPsInfo = NULL;
    fbe_status_t                        status;

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);    

    fbe_payload_control_get_buffer(control_operation, &psStatusInfoPtr);
    if (psStatusInfoPtr == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t*)psMgmtPtr, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "fbe_payload_control_get_buffer fail\n");
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_esp_ps_mgmt_ps_info_t))
    {
        fbe_base_object_trace((fbe_base_object_t*)psMgmtPtr, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                              "%X len invalid\n", len);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    // get encl ps info.
    status = fbe_ps_mgmt_get_encl_ps_info_by_location(psMgmtPtr,
                                              &(psStatusInfoPtr->location),
                                              &pEnclPsInfo);
    if (status != FBE_STATUS_OK)
    {
        psStatusInfoPtr->psInfo.bInserted = FALSE;
    }
    else
    {
        psStatusInfoPtr->psInfo = pEnclPsInfo->psInfo[psStatusInfoPtr->location.slot];
        // set environmental status
        if (pEnclPsInfo->psInfo[psStatusInfoPtr->location.slot].envInterfaceStatus == FBE_ENV_INTERFACE_STATUS_DATA_STALE)
        {
            psStatusInfoPtr->environmentalInterfaceFault = FALSE;
            psStatusInfoPtr->dataStale = TRUE;
        }
        else if (pEnclPsInfo->psInfo[psStatusInfoPtr->location.slot].envInterfaceStatus == FBE_ENV_INTERFACE_STATUS_XACTION_FAIL)
        {
            psStatusInfoPtr->environmentalInterfaceFault = TRUE;
            psStatusInfoPtr->dataStale = FALSE;
        }
        else
        {
            psStatusInfoPtr->environmentalInterfaceFault = FALSE;
            psStatusInfoPtr->dataStale = FALSE;
        }
    }
    fbe_base_object_trace((fbe_base_object_t*)psMgmtPtr, 
                          FBE_TRACE_LEVEL_DEBUG_LOW,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s, ps %d, insert %d, flt %d", 
                          __FUNCTION__,
                          psStatusInfoPtr->location.slot,
                          psStatusInfoPtr->psInfo.bInserted,
                          psStatusInfoPtr->psInfo.generalFault);

    psStatusInfoPtr->psLogicalFault = fbe_ps_mgmt_get_ps_logical_fault(&(psStatusInfoPtr->psInfo));
    psStatusInfoPtr->expected_ps_type = psMgmtPtr->expected_power_supply;
    
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
	return FBE_STATUS_OK;

}
/******************************************
 * end fbe_ps_mgmt_getPsStatus()
 ******************************************/


/*!***************************************************************
 * fbe_ps_mgmt_getPsCount()
 ****************************************************************
 * @brief
 *  This function processes the GetPsCount Control Code.
 *  class
 *
 * @param  - None.
 *
 * @return fbe_status_t
 *
 * @author
 *  15-Apr-2010 - Created. Joe Perry
 *
 ****************************************************************/
fbe_status_t fbe_ps_mgmt_getPsCount(fbe_ps_mgmt_t *psMgmtPtr, 
                                     fbe_packet_t * packet)
{
    fbe_esp_ps_mgmt_ps_count_t          *psCountInfoPtr = NULL;
    fbe_payload_control_buffer_length_t len = 0;
    fbe_payload_ex_t                       *payload = NULL;
    fbe_payload_control_operation_t     *control_operation = NULL;
    fbe_encl_ps_info_t                  *pEnclPsInfo = NULL;
    fbe_status_t                        status;

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);    

    fbe_payload_control_get_buffer(control_operation, &psCountInfoPtr);
    if (psCountInfoPtr == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t*)psMgmtPtr, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "fbe_payload_control_get_buffer fail\n");
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_esp_ps_mgmt_ps_count_t))
    {
        fbe_base_object_trace((fbe_base_object_t*)psMgmtPtr, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                              "%X len invalid\n", len);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    // get encl ps info.
    status = fbe_ps_mgmt_get_encl_ps_info_by_location(psMgmtPtr,
                                              &(psCountInfoPtr->location),
                                              &pEnclPsInfo);
    if (status != FBE_STATUS_OK)
    {
        psCountInfoPtr->psCount = 0;
    }
    else
    {
        psCountInfoPtr->psCount = pEnclPsInfo->psCount;
    }
        
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
	return FBE_STATUS_OK;

}
/******************************************
 * end fbe_ps_mgmt_getPsCount()
 ******************************************/


/*!***************************************************************
 * fbe_ps_mgmt_getCacheStatus()
 ****************************************************************
 * @brief
 *  This function processes the GetPsStatus Control Code.
 *  class
 *
 * @param  - None.
 *
 * @return fbe_status_t
 *
 * @author
 *  22-Mar-2011 - Created. Joe Perry
 *
 ****************************************************************/
fbe_status_t fbe_ps_mgmt_getCacheStatus(fbe_ps_mgmt_t *psMgmtPtr, 
                                          fbe_packet_t * packet)
{
    fbe_common_cache_status_t           *psCacheStatusPtr = NULL;
    fbe_common_cache_status_t           PeerPsCacheStatus = FBE_CACHE_STATUS_OK;
    fbe_payload_control_buffer_length_t len = 0;
    fbe_payload_ex_t                       *payload = NULL;
    fbe_payload_control_operation_t     *control_operation = NULL;
    fbe_status_t status = FBE_STATUS_OK;

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);    

    fbe_payload_control_get_buffer(control_operation, &psCacheStatusPtr);
    if (psCacheStatusPtr == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t*)psMgmtPtr, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "fbe_payload_control_get_buffer fail\n");
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_common_cache_status_t))
    {
        fbe_base_object_trace((fbe_base_object_t*)psMgmtPtr, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                              "%X len invalid\n", len);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_base_environment_get_peerCacheStatus((fbe_base_environment_t *) psMgmtPtr,
                                                      &PeerPsCacheStatus);

    *psCacheStatusPtr = fbe_base_environment_combine_cacheStatus((fbe_base_environment_t *) psMgmtPtr,
                                                                 psMgmtPtr->psCacheStatus,
                                                                 PeerPsCacheStatus,
                                                                 FBE_CLASS_ID_PS_MGMT);

    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;

}
/******************************************
 * end fbe_ps_mgmt_getCacheStatus()
 ******************************************/


/*!***************************************************************
 * fbe_ps_mgmt_powerdownPs()
 ****************************************************************
 * @brief
 *  This function processes the powerdown PS Control Code.
 *  class
 *
 * @param  - None.
 *
 * @return fbe_status_t
 *
 * @author
 *  07-Apr-2011 - Created. Joe Perry
 *
 ****************************************************************/
fbe_status_t fbe_ps_mgmt_powerdownPs(fbe_ps_mgmt_t *psMgmtPtr, 
                                     fbe_packet_t * packet)
{
    fbe_common_cache_status_t           *psCacheStatusPtr = NULL;
    fbe_payload_control_buffer_length_t len = 0;
    fbe_payload_ex_t                       *payload = NULL;
    fbe_payload_control_operation_t     *control_operation = NULL;

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);    

    fbe_payload_control_get_buffer(control_operation, &psCacheStatusPtr);
    if (psCacheStatusPtr == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t*)psMgmtPtr, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "fbe_payload_control_get_buffer fail\n");
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != 0)
    {
        fbe_base_object_trace((fbe_base_object_t*)psMgmtPtr, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                              "%X len invalid\n", len);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* 
     * determine if PS's needs to be shutdown
     */
    if (psMgmtPtr->psCacheStatus == FBE_CACHE_STATUS_FAILED)
    {
        fbe_base_object_trace((fbe_base_object_t*)psMgmtPtr, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s, PS Shutdown not implemented yet\n", 
                              __FUNCTION__);
    }
        
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;

}
/******************************************
 * end fbe_ps_mgmt_powerdownPs()
 ******************************************/

/*!***************************************************************
 * fbe_ps_mgmt_set_expected_ps_type()
 ****************************************************************
 * @brief
 *  This function processes the powerdown PS Control Code.
 *  class
 *
 * @param  - None.
 *
 * @return fbe_status_t
 *
 * @author
 *  30-Oct-2013 - Created. Brion Philbin
 *
 ****************************************************************/
fbe_status_t fbe_ps_mgmt_set_expected_ps_type(fbe_ps_mgmt_t *psMgmtPtr, 
                                     fbe_packet_t * packet)
{
    fbe_ps_mgmt_expected_ps_type_t *expected_ps_type = NULL;
    fbe_payload_control_buffer_length_t len = 0;
    fbe_payload_ex_t                       *payload = NULL;
    fbe_payload_control_operation_t     *control_operation = NULL;
    fbe_status_t status;

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);    

    fbe_payload_control_get_buffer(control_operation, &expected_ps_type);
    if (expected_ps_type == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t*)psMgmtPtr, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "fbe_payload_control_get_buffer fail\n");
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if (len != sizeof(fbe_ps_mgmt_expected_ps_type_t))
    {
        fbe_base_object_trace((fbe_base_object_t*)psMgmtPtr, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                              "%X len invalid\n", len);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if( (*expected_ps_type != FBE_PS_MGMT_EXPECTED_PS_TYPE_NONE) &&
        (*expected_ps_type != FBE_PS_MGMT_EXPECTED_PS_TYPE_OCTANE) &&
        (*expected_ps_type != FBE_PS_MGMT_EXPECTED_PS_TYPE_OTHER))
    {
        fbe_base_object_trace((fbe_base_object_t*)psMgmtPtr, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s invalid expected_ps_type:%d\n", __FUNCTION__, *expected_ps_type);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    psMgmtPtr->expected_power_supply = *expected_ps_type;

    // copy the new information to the persistent storage in memory copy and kick off a write.
    fbe_copy_memory(((fbe_base_environment_t *)psMgmtPtr)->my_persistent_data, 
                    &psMgmtPtr->expected_power_supply,
                    sizeof(fbe_ps_mgmt_expected_ps_type_t));

    fbe_base_object_trace((fbe_base_object_t *)psMgmtPtr, 
              FBE_TRACE_LEVEL_INFO,
              FBE_TRACE_MESSAGE_ID_INFO,
              "PS_MGMT: writing expected_power_supply:%d to disk\n",
              psMgmtPtr->expected_power_supply);

    status = fbe_base_env_write_persistent_data((fbe_base_environment_t *)psMgmtPtr);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)psMgmtPtr, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s persistent write error, status: 0x%X",
                              __FUNCTION__, status);
    }
        
    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;

}
/******************************************
 * end fbe_ps_mgmt_set_expected_ps_type()
 ******************************************/


/*!***************************************************************
 * fbe_ps_mgmt_updatePsFailStatus()
 ****************************************************************
 * @brief
 *  This function will set the appropriate PS failed status.
 *
 * @param  - None.
 *
 * @return fbe_status_t
 *
 * @author
 *  10-Mar-2014 - Created. Joe Perry
 *
 ****************************************************************/
fbe_status_t 
fbe_ps_mgmt_updatePsFailStatus(fbe_u8_t *mgmtPtr,
                               fbe_device_physical_location_t *pLocation,
                               fbe_base_environment_failure_type_t failureType,
                               fbe_bool_t newValue)
{
    fbe_status_t                status;
    fbe_ps_mgmt_t               *psMgmtPtr;
    fbe_encl_ps_info_t          *pEnclPsInfo = NULL;
    fbe_u8_t                    deviceStr[FBE_DEVICE_STRING_LENGTH]={0};

    psMgmtPtr = (fbe_ps_mgmt_t *)mgmtPtr;
    status = fbe_ps_mgmt_get_encl_ps_info_by_location(psMgmtPtr, pLocation, &pEnclPsInfo);

    if (status != FBE_STATUS_OK) 
    {
        fbe_base_object_trace((fbe_base_object_t *)psMgmtPtr, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s, enclosure for PS does not exist %d_%d, status %d.\n",
                              __FUNCTION__, pLocation->bus, pLocation->enclosure, status);
        return FBE_STATUS_COMPONENT_NOT_FOUND;
    }

    status = fbe_base_env_create_device_string(FBE_DEVICE_TYPE_PS, 
                                               pLocation, 
                                               &deviceStr[0],
                                               FBE_DEVICE_STRING_LENGTH);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)psMgmtPtr, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                              "%s, Failed to create device string.\n", 
                              __FUNCTION__); 

        return status;
    }

    switch (failureType)
    {
    case FBE_BASE_ENV_RP_READ_FAILURE:
        fbe_base_object_trace((fbe_base_object_t *)psMgmtPtr, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s, %s, resPromReadFailed changed %d to %d.\n",
                              __FUNCTION__, 
                              &deviceStr[0],
                              pEnclPsInfo->psInfo[pLocation->slot].resumePromReadFailed,
                              newValue);
        pEnclPsInfo->psInfo[pLocation->slot].resumePromReadFailed = newValue;
        break;
    case FBE_BASE_ENV_FUP_FAILURE:
        fbe_base_object_trace((fbe_base_object_t *)psMgmtPtr, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s, %s, fupFailed changed %d to %d.\n",
                              __FUNCTION__, 
                              &deviceStr[0],
                              pEnclPsInfo->psInfo[pLocation->slot].fupFailure,
                              newValue);
        pEnclPsInfo->psInfo[pLocation->slot].fupFailure = newValue;
        break;
    default:
        fbe_base_object_trace((fbe_base_object_t *)psMgmtPtr, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                              "%s, Unsupported failureType %d.\n", 
                              __FUNCTION__, failureType); 
        return FBE_STATUS_COMPONENT_NOT_FOUND;
        break;
    }

    return FBE_STATUS_OK;

}   // end of fbe_ps_mgmt_updatePsFailStatus

/* End of file fbe_ps_mgmt_usurper.c*/
