/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_base_environment_debug.c
 ***************************************************************************
 * @brief
 *  This file contains the debug functions for base_environment.
 *
 * @version
 *   18-Aug-2010:  PHE - Created.
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_esp_base_environment.h"
#include "fbe_base_environment_debug.h"

static char enclFaultReasonString[500];

/*!**************************************************************
 * @fn fbe_base_env_decode_fup_completion_status(fbe_base_env_fup_completion_status_t completionStatus)
 ****************************************************************
 * @brief
 *  This function decodes the completion status.
 *
 * @param completionStatus - 
 *
 * @return the completion status string.
 *
 * @version
 *  15-June-2010:  PHE - Created. 
 *
 ****************************************************************/
char * fbe_base_env_decode_fup_completion_status(fbe_base_env_fup_completion_status_t completionStatus)
{
    char * completionStatusStr;

    switch(completionStatus)
    {
        case FBE_BASE_ENV_FUP_COMPLETION_STATUS_NONE:
            completionStatusStr = "NONE";
            break;
        
        case FBE_BASE_ENV_FUP_COMPLETION_STATUS_QUEUED:
            completionStatusStr = "QUEUED";
            break;

        case FBE_BASE_ENV_FUP_COMPLETION_STATUS_IN_PROGRESS:
            completionStatusStr = "IN PROGRESS";
            break;
               
        case FBE_BASE_ENV_FUP_COMPLETION_STATUS_SUCCESS_REV_CHANGED:
            completionStatusStr = "SUCCESS(REV CHANGED)";
            break;
                 
        case FBE_BASE_ENV_FUP_COMPLETION_STATUS_SUCCESS_NO_REV_CHANGE:
            completionStatusStr = "SUCCESS(NO REV CHANGE)";
            break;
              
        case FBE_BASE_ENV_FUP_COMPLETION_STATUS_EXIT_UP_TO_REV:
            completionStatusStr = "EXIT(UP TO REV)";
            break;
    
        case FBE_BASE_ENV_FUP_COMPLETION_STATUS_FAIL_NULL_IMAGE_PATH:
            completionStatusStr = "FAIL(NULL IMAGE PATH)";
            break;
    
        case FBE_BASE_ENV_FUP_COMPLETION_STATUS_FAIL_TO_READ_REG_IMAGE_PATH:
            completionStatusStr = "FAIL TO READ IMAGE PATH";
            break;
    
        case FBE_BASE_ENV_FUP_COMPLETION_STATUS_FAIL_NO_REV_CHANGE:
            completionStatusStr = "FAIL(NO REV CHANGE)";
            break;
    
        case FBE_BASE_ENV_FUP_COMPLETION_STATUS_FAIL_TO_READ_IMAGE_HEADER:
            completionStatusStr = "FAIL TO READ IMAGE HEADER";
            break;

        case FBE_BASE_ENV_FUP_COMPLETION_STATUS_FAIL_TO_PARSE_IMAGE_HEADER:  
            completionStatusStr = "FAIL TO PARSE IMAGE HEADER";
            break;
    
        case FBE_BASE_ENV_FUP_COMPLETION_STATUS_FAIL_TO_READ_ENTIRE_IMAGE:
            completionStatusStr = "FAIL TO READ ENTIRE IMAGE";
            break;
    
        case FBE_BASE_ENV_FUP_COMPLETION_STATUS_FAIL_BAD_IMAGE:  
            completionStatusStr = "FAIL(BAD IMAGE)";
            break;  

        case FBE_BASE_ENV_FUP_COMPLETION_STATUS_MISMATCHED_IMAGE:  
            completionStatusStr = "MISMATCHED IMAGE";
            break;
            
        case FBE_BASE_ENV_FUP_COMPLETION_STATUS_FAIL_IMAGE_TYPES_TOO_SMALL:  
            completionStatusStr = "FAIL(IMAGE TYPES TOO SMALL)";
            break;     
                  
        case FBE_BASE_ENV_FUP_COMPLETION_STATUS_FAIL_BAD_ENV_STATUS:
            completionStatusStr = "FAIL(BAD ENV STATUS)";
            break;
    
        case FBE_BASE_ENV_FUP_COMPLETION_STATUS_FAIL_NO_PEER_PERMISSION:
            completionStatusStr = "FAIL(NO PEER PERMISSION)";
            break;
    
        case FBE_BASE_ENV_FUP_COMPLETION_STATUS_FAIL_DOWNLOAD_CMD_DENIED:
            completionStatusStr = "FAIL(DOWNLOAD CMD DENIED)";
            break;

        case FBE_BASE_ENV_FUP_COMPLETION_STATUS_FAIL_TO_DOWNLOAD_IMAGE:
            completionStatusStr = "FAIL TO DOWNLOAD IMAGE";
            break;

        case FBE_BASE_ENV_FUP_COMPLETION_STATUS_FAIL_TO_GET_DOWNLOAD_STATUS:
            completionStatusStr = "FAIL TO GET DOWNLOAD STATUS";
            break;
    
        case FBE_BASE_ENV_FUP_COMPLETION_STATUS_FAIL_TO_ACTIVATE_IMAGE:
            completionStatusStr = "FAIL TO ACTIVATE IMAGE";
            break;
    
        case FBE_BASE_ENV_FUP_COMPLETION_STATUS_FAIL_TO_GET_ACTIVATE_STATUS:
            completionStatusStr = "FAIL TO GET ACTIVATE STATUS";
            break;

        case FBE_BASE_ENV_FUP_COMPLETION_STATUS_FAIL_CONTAINING_DEVICE_REMOVED:
            completionStatusStr = "FAIL(CONTAINING DEVICE REMOVED)";
            break;
    
        case FBE_BASE_ENV_FUP_COMPLETION_STATUS_FAIL_DEVICE_REMOVED:
            completionStatusStr = "FAIL(DEVICE REMOVED)";
            break;

        case FBE_BASE_ENV_FUP_COMPLETION_STATUS_FAIL_TO_TRANSITION_STATE:
            completionStatusStr = "FAIL TO TRANSITION STATE";
            break;

        case FBE_BASE_ENV_FUP_COMPLETION_STATUS_ABORTED:
            completionStatusStr = "ABORTED";
            break;

        case FBE_BASE_ENV_FUP_COMPLETION_STATUS_TERMINATED:
            completionStatusStr = "TERMINATED";
            break;

        case FBE_BASE_ENV_FUP_COMPLETION_STATUS_ACTIVATION_DEFERRED:
            completionStatusStr = "ACTIVATION DEFERRED";
            break;
    
        default:
            completionStatusStr = "UNKNOWN";
            break;
    }

    return completionStatusStr;
}

/*!**************************************************************
 * @fn fbe_base_env_decode_device_type(fbe_u64_t deviceType)
 ****************************************************************
 * @brief
 *  This function decodes the device type.
 *
 * @param deviceType - 
 *
 * @return the device type string.
 *
 * @version
 *  30-Sept-2010:  PHE - Created. 
 *
 ****************************************************************/
char * fbe_base_env_decode_device_type(fbe_u64_t deviceType)
{
    char * deviceTypeStr;

    switch(deviceType)
    {
        case FBE_DEVICE_TYPE_SP:
            deviceTypeStr = "SP";
            break;

        case FBE_DEVICE_TYPE_ENCLOSURE:
            deviceTypeStr = "ENCLOSURE";
            break;

        case FBE_DEVICE_TYPE_PS:
            deviceTypeStr = "PS";
            break;

        case FBE_DEVICE_TYPE_LCC:
            deviceTypeStr = "LCC";
            break;

        case FBE_DEVICE_TYPE_FAN:
            deviceTypeStr = "FAN";
            break;

        case FBE_DEVICE_TYPE_SPS:
            deviceTypeStr = "SPS";
            break;

        case FBE_DEVICE_TYPE_IOMODULE:
            deviceTypeStr = "IOMODULE";
            break;

        case FBE_DEVICE_TYPE_BACK_END_MODULE:
            deviceTypeStr = "BASE_MODULE";
            break;

        case FBE_DEVICE_TYPE_DRIVE:
            deviceTypeStr = "DRIVE";
            break;

        case FBE_DEVICE_TYPE_MEZZANINE:
            deviceTypeStr = "MEZZANINE";
            break;

        case FBE_DEVICE_TYPE_MGMT_MODULE:
            deviceTypeStr = "MGMT_MODULE";
            break;

        case FBE_DEVICE_TYPE_SLAVE_PORT:
            deviceTypeStr = "SLAVE_PORT";
            break;

        case FBE_DEVICE_TYPE_PLATFORM:
            deviceTypeStr = "PLATFORM";
            break;

        case FBE_DEVICE_TYPE_SUITCASE:
            deviceTypeStr = "SUITCASE";
            break;

        case FBE_DEVICE_TYPE_MISC:
            deviceTypeStr = "MISC";
            break;

        case FBE_DEVICE_TYPE_BMC:
            deviceTypeStr = "BMC";
            break;

        case FBE_DEVICE_TYPE_SFP:
            deviceTypeStr = "SFP";
            break;

        case FBE_DEVICE_TYPE_CONNECTOR:
            deviceTypeStr = "CONNECTOR";
            break;

        case FBE_DEVICE_TYPE_DRIVE_MIDPLANE:
            deviceTypeStr = "DRIVE_MIDPLANE";
            break;

        case FBE_DEVICE_TYPE_BATTERY:
            deviceTypeStr = "BBU";
            break;

        case FBE_DEVICE_TYPE_CACHE_CARD:
            deviceTypeStr = "CACHE_CARD";
            break;

        default:
            deviceTypeStr = "UNKNOWN";
            break;
    }

    return deviceTypeStr;
}

/*!**************************************************************
 * @fn fbe_base_env_decode_fup_work_state(fbe_base_env_fup_work_state_t workState)
 ****************************************************************
 * @brief
 *  This function decodes the work state.
 *
 * @param workState - 
 *
 * @return the work state string.
 *
 * @version
 * 30-SEPTEMBER-2010:  VISHNU SHARMA - Created. 
 *
 ****************************************************************/
char * fbe_base_env_decode_fup_work_state(fbe_base_env_fup_work_state_t workState)
{
    char *workStateStr;

    switch(workState)
    {
        case FBE_BASE_ENV_FUP_WORK_STATE_NONE:
             workStateStr = "NONE";
            break;

        case FBE_BASE_ENV_FUP_WORK_STATE_WAIT_BEFORE_UPGRADE:
             workStateStr = "WAIT BEFORE UPGRADE";
            break;
    
        case FBE_BASE_ENV_FUP_WORK_STATE_QUEUED:
             workStateStr = "FIRMWARE UPGRADE QUEUED";
            break;
               
        case FBE_BASE_ENV_FUP_WORK_STATE_WAIT_FOR_INTER_DEVICE_DELAY_DONE:
             workStateStr = "WAIT FOR INTER DEVICE DELAY DONE";
            break;

        case FBE_BASE_ENV_FUP_WORK_STATE_READ_IMAGE_HEADER_DONE:
             workStateStr = "IMAGE HEADER READ DONE";
            break;
                 
        case FBE_BASE_ENV_FUP_WORK_STATE_CHECK_REV_DONE:
             workStateStr = "REV CHECK DONE";
            break;
              
        case FBE_BASE_ENV_FUP_WORK_STATE_READ_ENTIRE_IMAGE_DONE:
             workStateStr = "IMAGE READ DONE";
            break;
    
        case FBE_BASE_ENV_FUP_WORK_STATE_DOWNLOAD_IMAGE_SENT:
             workStateStr = "DOWNLOAD IMAGE SENT";
            break;
    
        case FBE_BASE_ENV_FUP_WORK_STATE_DOWNLOAD_IMAGE_DONE:
             workStateStr = "IMAGE DOWNLOAD DONE";
            break;
    
        case FBE_BASE_ENV_FUP_WORK_STATE_PEER_PERMISSION_REQUESTED:
             workStateStr = "PEER PERMISSION REQUESTED";
            break;
    
        case FBE_BASE_ENV_FUP_WORK_STATE_PEER_PERMISSION_RECEIVED:
             workStateStr = "PEER PERMISSION RECEIVED";
            break;
    
        case FBE_BASE_ENV_FUP_WORK_STATE_CHECK_ENV_STATUS_DONE:
             workStateStr = "ENV STATUS CHECK DONE";
            break;
    
        case FBE_BASE_ENV_FUP_WORK_STATE_ACTIVATE_IMAGE_SENT:  
             workStateStr = "ACTIVATE IMAGE MESSAGE SENT";
            break;       
                  
        case FBE_BASE_ENV_FUP_WORK_STATE_ACTIVATE_IMAGE_DONE:
             workStateStr = "IMAGE ACTIVATION DONE";
            break;
    
        case FBE_BASE_ENV_FUP_WORK_STATE_CHECK_RESULT_DONE:
             workStateStr = "RESULT CHECK DONE";
            break;

        case FBE_BASE_ENV_FUP_WORK_STATE_REFRESH_DEVICE_STATUS_DONE:
             workStateStr = "REFRESH DEVICE STATUS DONE";
            break;

        case FBE_BASE_ENV_FUP_WORK_STATE_END_UPGRADE_DONE:
             workStateStr = "END UPGRADE DONE";
            break;
    
        case FBE_BASE_ENV_FUP_WORK_STATE_ABORT_CMD_SENT:
             workStateStr = "ABORT CMD SENT";
            break;

        default:
            workStateStr = "INVALID";
            break;
    }

    return workStateStr;
}

/*!*************************************************************************
 * @fn fbe_base_env_decode_firmware_op_code(fbe_enclosure_firmware_opcode_t firmwareOpCode)
 **************************************************************************
 *  @brief
 *  This function will translate fbe_enclosure_firmware_opcode_t to string.
 *
 *  @param    fbe_enclosure_firmware_opcode_t
 *
 *  @return     char * - string
 *
 *  HISTORY:
 *    07-Jan-2011: PHE- created.
 **************************************************************************/
char * fbe_base_env_decode_firmware_op_code(fbe_enclosure_firmware_opcode_t firmwareOpCode)
{
    char * fwOpStr;

    switch(firmwareOpCode)
    {
    case FBE_ENCLOSURE_FIRMWARE_OP_NONE :
        fwOpStr = "NONE";
        break;
    case FBE_ENCLOSURE_FIRMWARE_OP_DOWNLOAD :
        fwOpStr = "DOWNLOAD";
        break;
    case FBE_ENCLOSURE_FIRMWARE_OP_ACTIVATE :
        fwOpStr = "ACTIVATE";
        break;
    case FBE_ENCLOSURE_FIRMWARE_OP_ABORT :
        fwOpStr = "ABORT";
        break;

    case FBE_ENCLOSURE_FIRMWARE_OP_GET_STATUS :
        fwOpStr = "GET STATUS";
        break;

    case FBE_ENCLOSURE_FIRMWARE_OP_NOTIFY_COMPLETION :
        fwOpStr = "NOTIFY COMPLETION";
        break;

    case FBE_ENCLOSURE_FIRMWARE_OP_GET_LOCAL_PERMISSION :
        fwOpStr = "GET PERMISSION";
        break;

    case FBE_ENCLOSURE_FIRMWARE_OP_RETURN_LOCAL_PERMISSION :
        fwOpStr = "RETURN PERMISSION";
        break;

    default:
        // not supported
        fwOpStr = "Not Supported";
        break;
    }
    return(fwOpStr);
} // fbe_base_env_fup_decode_firmware_op_code

/*!**************************************************************
 * @fn fbe_base_env_map_devicetype_to_classid(fbe_u64_t deviceType,fbe_class_id_t *classId)
 ****************************************************************
 * @brief
 *  This function decodes the work state.
 *
 * @param deviceType device type of device you want to map on class
 * @param classId - classid ptr
 
 * @return the status.
 *
 * @version
 * 30-SEPTEMBER-2010:  VISHNU SHARMA - Created. 
 *
 ****************************************************************/
fbe_status_t fbe_base_env_map_devicetype_to_classid(fbe_u64_t deviceType,fbe_class_id_t *classId)
{
    fbe_status_t  status = FBE_STATUS_OK;
    switch(deviceType)
    {
        case FBE_DEVICE_TYPE_PS:
            *classId = FBE_CLASS_ID_PS_MGMT;
            break;

        case FBE_DEVICE_TYPE_SP:
            *classId = FBE_CLASS_ID_BOARD_MGMT;
            break;

        case FBE_DEVICE_TYPE_MEZZANINE:
        case FBE_DEVICE_TYPE_IOMODULE:
        case FBE_DEVICE_TYPE_BACK_END_MODULE:
        case FBE_DEVICE_TYPE_MGMT_MODULE:
            *classId = FBE_CLASS_ID_MODULE_MGMT;
            break;

        case FBE_DEVICE_TYPE_ENCLOSURE:
        case FBE_DEVICE_TYPE_LCC:
            *classId = FBE_CLASS_ID_ENCL_MGMT;
            break;

        case FBE_DEVICE_TYPE_FAN:
            *classId = FBE_CLASS_ID_COOLING_MGMT;
            break;
    
        default:
            fbe_printf("\n\t Invalid device type    deviceType = %lld \n",deviceType);
            return FBE_STATUS_GENERIC_FAILURE;
            
    }
    return status;
}

/*!**************************************************************
 * @fn fbe_base_env_create_device_string(fbe_u64_t deviceType, 
 *                                       fbe_device_physical_location_t * pLocation,
 *                                       char * deviceStr,
 *                                       fbe_u32_t bufferLen)
 ****************************************************************
 * @brief
 *  This function creates the string for the specified device used in the event log
 *  message.
 *
 *  
 * @param deviceType (IN)- 
 * @param pLocation (IN)-
 * @param deviceStr (OUT)-
 * @param bufferLen (OUT)-
 * 
 * @return fbe_status_t.
 *
 * @version
 *   14-Mar-2011: PHE  - Created. 
 *
 ****************************************************************/
fbe_status_t fbe_base_env_create_device_string(fbe_u64_t deviceType, 
                                         fbe_device_physical_location_t * pLocation,
                                         char * deviceStr,
                                         fbe_u32_t bufferLen)
{
    fbe_status_t status = FBE_STATUS_OK;

    switch(deviceType)
    {
        case FBE_DEVICE_TYPE_MEZZANINE:
            _snprintf(deviceStr, bufferLen, "%s Mezzanine %d", (pLocation->sp == SP_A)? "SPA" : "SPB", pLocation->slot);
            break;

        case FBE_DEVICE_TYPE_SUITCASE:
            _snprintf(deviceStr, bufferLen, "%s Suitcase %d", (pLocation->sp == SP_A)? "SPA" : "SPB", pLocation->slot);
            break;

        case FBE_DEVICE_TYPE_BMC:
            _snprintf(deviceStr, bufferLen, "%s BMC %d", (pLocation->sp == SP_A)? "SPA" : "SPB", pLocation->slot);
            break;

        case FBE_DEVICE_TYPE_IOMODULE:
            _snprintf(deviceStr, bufferLen, "%s IO Module %d", (pLocation->sp == SP_A)? "SPA" : "SPB", pLocation->slot);
            break;

        case FBE_DEVICE_TYPE_BACK_END_MODULE:
            _snprintf(deviceStr, bufferLen, "%s Base Module %d", (pLocation->sp == SP_A)? "SPA" : "SPB", pLocation->slot);
                    
            break;

        case FBE_DEVICE_TYPE_MGMT_MODULE:
            _snprintf(deviceStr, bufferLen, "%s management module %d", (pLocation->sp == SP_A)? "SPA" : "SPB", pLocation->slot);
            break;
    
        case FBE_DEVICE_TYPE_PS:
            if((pLocation->bus == FBE_XPE_PSEUDO_BUS_NUM) &&
               (pLocation->enclosure == FBE_XPE_PSEUDO_ENCL_NUM)) 
            {
                _snprintf(deviceStr, bufferLen, "xPE Power Supply %d", pLocation->slot);
            }
            else
            {
                _snprintf(deviceStr, bufferLen, "Bus %d Enclosure %d Power Supply %d",
                        pLocation->bus,
                        pLocation->enclosure,
                        pLocation->slot);
            }
            break;
            
        case FBE_DEVICE_TYPE_FAN:
            if((pLocation->bus == FBE_XPE_PSEUDO_BUS_NUM) &&
               (pLocation->enclosure == FBE_XPE_PSEUDO_ENCL_NUM)) 
            {
                _snprintf(deviceStr, bufferLen, "xPE Fan %d", pLocation->slot);
            }
            else
            {
                _snprintf(deviceStr, bufferLen, "Bus %d Enclosure %d Fan %d",
                           pLocation->bus,
                           pLocation->enclosure,
                           pLocation->slot );
            }
            break;

        case FBE_DEVICE_TYPE_ENCLOSURE:
            if((pLocation->bus == FBE_XPE_PSEUDO_BUS_NUM) &&
               (pLocation->enclosure == FBE_XPE_PSEUDO_ENCL_NUM)) 
            {
                _snprintf(deviceStr, bufferLen, "xPE");
            }
            else
            {
                _snprintf(deviceStr, bufferLen, "Bus %d Enclosure %d",
                        pLocation->bus,
                        pLocation->enclosure);
            }
            break;

        case FBE_DEVICE_TYPE_LCC:
            if(pLocation->componentId == 0) 
            {
            
                _snprintf(deviceStr, bufferLen, "Bus %d Enclosure %d LCC %d",
                        pLocation->bus,
                        pLocation->enclosure,
                        pLocation->slot);
            }
            else
            {
                _snprintf(deviceStr, bufferLen, "Bus %d Enclosure %d Component %d LCC %d",
                        pLocation->bus,
                        pLocation->enclosure,
                        pLocation->componentId,
                        pLocation->slot);
            }

            break;

        case FBE_DEVICE_TYPE_DRIVE:
            if (pLocation->bank != '\0')
            {
                _snprintf(deviceStr, bufferLen, "Bus %d Enclosure %d Disk %d (%c%d)",
                        pLocation->bus,
                        pLocation->enclosure,
                        pLocation->slot,
                        pLocation->bank,
                        pLocation->bank_slot);
            }
            else
            {
                _snprintf(deviceStr, bufferLen, "Bus %d Enclosure %d Disk %d",
                        pLocation->bus,
                        pLocation->enclosure,
                        pLocation->slot);
            }
            break;

        case FBE_DEVICE_TYPE_SPS:
            switch(pLocation->componentId)
            {
                case FBE_SPS_COMPONENT_ID_PRIMARY:
                    if (pLocation->enclosure == FBE_XPE_PSEUDO_ENCL_NUM)
                    {
                        _snprintf(deviceStr, bufferLen, (pLocation->slot == 0)? "SPE SPSA" : "SPE SPSB");
                    }
                    else
                    {
                        _snprintf(deviceStr, bufferLen, (pLocation->slot == 0)? "0_0 SPSA" : "0_0 SPSB");
                        
                    }
                    break;
    
                case FBE_SPS_COMPONENT_ID_SECONDARY:
                    if (pLocation->enclosure == FBE_XPE_PSEUDO_ENCL_NUM)
                    {
                        _snprintf(deviceStr, bufferLen, (pLocation->slot == 0)? "SPE SPSA Secondary" : "SPE SPSB Secondary");
                    }
                    else
                    {
                        _snprintf(deviceStr, bufferLen, (pLocation->slot == 0)? "0_0 SPSA Secondary" : "0_0 SPSB Secondary");
                    }
                    break;

                case FBE_SPS_COMPONENT_ID_BATTERY:
                    if (pLocation->enclosure == FBE_XPE_PSEUDO_ENCL_NUM)
                    {
                        _snprintf(deviceStr, bufferLen, (pLocation->slot == 0)? "SPE SPSA Battery" : "SPE SPSB Battery");
                    }
                    else
                    {
                        _snprintf(deviceStr, bufferLen, (pLocation->slot == 0)? "0_0 SPSA Battery" : "0_0 SPSB Battery");
                    }
                    break;
    
                default:
                    status = FBE_STATUS_GENERIC_FAILURE;
                    break;
            }
            break;

        case FBE_DEVICE_TYPE_SPS_BATTERY:
            if (pLocation->enclosure == FBE_XPE_PSEUDO_ENCL_NUM)
            {
                _snprintf(deviceStr, bufferLen, (pLocation->slot == 0)? "SPE SPSA Battery" : "SPE SPSB Battery");
            }
            else
            {
                _snprintf(deviceStr, bufferLen, (pLocation->slot == 0)? "0_0 SPSA Battery" : "0_0 SPSB Battery");
            }
            break;

        case FBE_DEVICE_TYPE_BATTERY:
            if((pLocation->bus == FBE_XPE_PSEUDO_BUS_NUM) &&
               (pLocation->enclosure == FBE_XPE_PSEUDO_ENCL_NUM)) 
            {
                _snprintf(deviceStr, bufferLen, "xPE BBU%s%d", 
                          (pLocation->sp == SP_A ? "A" : "B"),
                          pLocation->slot);
            }
            else
            {
                _snprintf(deviceStr, bufferLen, "Bus %d Enclosure %d BBU%s%d",
                          pLocation->bus,
                          pLocation->enclosure,
                          (pLocation->sp == SP_A ? "A" : "B"),
                          pLocation->slot);
            }
            break;

        case FBE_DEVICE_TYPE_SP:
            _snprintf(deviceStr, bufferLen, (pLocation->sp == SP_A)? "SPA" : "SPB");
            break;

        case FBE_DEVICE_TYPE_CONNECTOR:
            _snprintf(deviceStr, bufferLen, "Bus %d Enclosure %d Connector %d (%s, connector id %d)",
                    pLocation->bus,
                    pLocation->enclosure,
                    pLocation->slot,
                    fbe_base_env_decode_connector_type(pLocation->bank),
                    pLocation->componentId);
            break;
            
        case FBE_DEVICE_TYPE_SSC:
            _snprintf(deviceStr, bufferLen, "Bus %d Enclosure %d System Status Card",
                    pLocation->bus,
                    pLocation->enclosure);
            break;

        case FBE_DEVICE_TYPE_CACHE_CARD:
            sprintf(deviceStr, "Cache Card %d", pLocation->slot);
            break;

        case FBE_DEVICE_TYPE_DIMM:
            sprintf(deviceStr, "%s DIMM %d", (pLocation->sp == SP_A)? "SPA" : "SPB", pLocation->slot);
            break;

        case FBE_DEVICE_TYPE_DRIVE_MIDPLANE:
            sprintf(deviceStr, "Bus %d Enclosure %d Drive Midplane", 
                    pLocation->bus,
                    pLocation->enclosure);
            break;

        case FBE_DEVICE_TYPE_SSD:
            sprintf(deviceStr, "%s SSD %d", (pLocation->sp == SP_A)? "SPA" : "SPB", pLocation->slot);
            break;

        default:
            _snprintf(deviceStr, bufferLen, "Unknown Device type %d", deviceType);
            status = FBE_STATUS_GENERIC_FAILURE;
            break;
    }

    if(strlen(deviceStr) >= bufferLen) 
    {
        status = FBE_STATUS_INSUFFICIENT_RESOURCES; 
    }

    return status;
}
 
/*!**************************************************************
 * @fn fbe_base_env_create_simple_device_string(fbe_u64_t deviceType, 
 *                                              fbe_device_physical_location_t * pLocation,
 *                                              char * deviceStr,
 *                                              fbe_u32_t bufferLen)
 ****************************************************************
 * @brief
 *  This function creates the string for the specified device used in the trace.
 *
 *  
 * @param deviceType (IN)- 
 * @param pLocation (IN)-
 * @param simpleDeviceStr (OUT)-
 * @param bufferLen (IN)-
 * 
 * @return fbe_status_t.
 *
 * @version
 *   27-Cct30-2010: PHE  - Created. 
 *
 ****************************************************************/
fbe_status_t fbe_base_env_create_simple_device_string(fbe_u64_t deviceType, 
                                                      fbe_device_physical_location_t * pLocation,
                                                      char * simpleDeviceStr,
                                                      fbe_u32_t bufferLen)
{
    fbe_status_t status = FBE_STATUS_OK;

    switch(deviceType)
    {
        case FBE_DEVICE_TYPE_PS:
            // TO DO - Need to log different string for XPE PS and other PS.
            _snprintf(simpleDeviceStr, bufferLen, "PS(%d_%d_%d)",
                    pLocation->bus,
                    pLocation->enclosure,
                    pLocation->slot);
            break;

        case FBE_DEVICE_TYPE_LCC:
            if(pLocation->componentId == 0) 
            {
                _snprintf(simpleDeviceStr, bufferLen, "LCC(%d_%d_%d)",
                        pLocation->bus,
                        pLocation->enclosure,
                        pLocation->slot);
            }
            else
            {
                _snprintf(simpleDeviceStr, bufferLen, "ENCL(%d_%d.%d)LCC%d",
                        pLocation->bus,
                        pLocation->enclosure,
                        pLocation->componentId,
                        pLocation->slot);
            }
            break;

        case FBE_DEVICE_TYPE_DRIVE:
            _snprintf(simpleDeviceStr, bufferLen, "Drive(%d_%d_%d)",
                    pLocation->bus,
                    pLocation->enclosure,
                    pLocation->slot);
            break;

        case FBE_DEVICE_TYPE_SPS:
            _snprintf(simpleDeviceStr, bufferLen, (pLocation->slot == 0)? "SPSA" : "SPSB");
            break;

        case FBE_DEVICE_TYPE_BATTERY:
            _snprintf(simpleDeviceStr, bufferLen, (pLocation->slot == 0)? "BBUA" : "BBUB");
            break;

        default:
            _snprintf(simpleDeviceStr, bufferLen, "Unknown(%d_%d_%d)",
                    pLocation->bus,
                    pLocation->enclosure,
                    pLocation->slot);
            status = FBE_STATUS_GENERIC_FAILURE;
            break;
    }
   
    if(strlen(simpleDeviceStr) >= bufferLen) 
    {
        status = FBE_STATUS_INSUFFICIENT_RESOURCES; 
    }
    
    return status;
}

/*!**************************************************************
 * fbe_base_env_decode_env_status()
 ****************************************************************
 * @brief
 *  This function converts the status to a text string.
 *
 * @param 
 *        fbe_env_inferface_status_t             status
 * 
 * @return
 *        fbe_char_t *string for given status
 * 
 * @author
 *  03/24/2011:  Created. Rashmi Sawale 
 *
 ****************************************************************/
char * fbe_base_env_decode_env_status(fbe_env_inferface_status_t  envStatus)
{  
     switch(envStatus)
    {
        case FBE_ENV_INTERFACE_STATUS_DATA_STALE:
             return "DATA STALE";
             break;      
        case FBE_ENV_INTERFACE_STATUS_XACTION_FAIL:
            return "TRANSACTION FAIL";
            break; 
        case FBE_ENV_INTERFACE_STATUS_GOOD:
            return "GOOD";
            break; 
        case FBE_ENV_INTERFACE_STATUS_NA:
            return "NA";
            break; 
        default:
           return "UNKNOWN";//"unknown status code";
           break;
    }
}

/*!**************************************************************
 * fbe_base_env_decode_status()
 ****************************************************************
 * @brief
 *  This function converts the status to a text string.
 *
 * @param 
 *        fbe_mgmt_status_t             status
 * 
 * @return
 *        fbe_char_t *string for given status
 * 
 * @author
 *  03/24/2011:  Created. Rashmi Sawale 
 *
 ****************************************************************/
char * fbe_base_env_decode_status(fbe_mgmt_status_t  status)
{
    switch(status)
    {
        case FBE_MGMT_STATUS_FALSE:
            return "NO";
            break;
        case FBE_MGMT_STATUS_TRUE:
            return "YES";
            break;
        case FBE_MGMT_STATUS_UNKNOWN:
            return "UNKNOWN";
            break;
       case FBE_MGMT_STATUS_NA:
           return "NA";
           break;
       default:
           return "UNKNOWN";
           break;
    }
}

/*!**************************************************************
 * @fn fbe_base_env_decode_resume_prom_status(fbe_resume_prom_status_t resumeStatus)
 ****************************************************************
 * @brief
 *  This function decodes the resume prom status.
 *
 * @param completionStatus - 
 *
 * @return the completion status string.
 *
 * @version
 *  02-Aug-2011:  PHE - Created. 
 *
 ****************************************************************/
char * fbe_base_env_decode_resume_prom_status(fbe_resume_prom_status_t resumeStatus)
{
    char * resumeStatusStr;

    switch(resumeStatus)
    {
        case FBE_RESUME_PROM_STATUS_UNINITIATED:
            resumeStatusStr = "UNINITIATED";
            break;
        
        case FBE_RESUME_PROM_STATUS_QUEUED:
            resumeStatusStr = "QUEUED";
            break;
       
        case FBE_RESUME_PROM_STATUS_READ_SUCCESS:
            resumeStatusStr = "READ_SUCCESS";
            break;
        
        case FBE_RESUME_PROM_STATUS_READ_IN_PROGRESS:
            resumeStatusStr = "READ_IN_PROGRESS";
            break;

        case FBE_RESUME_PROM_STATUS_DEVICE_DEAD:
            resumeStatusStr = "DEVICE_DEAD";
            break;
        
        case FBE_RESUME_PROM_STATUS_CHECKSUM_ERROR:
            resumeStatusStr = "CHECKSUM_ERROR";
            break; 
        
        case FBE_RESUME_PROM_STATUS_DEVICE_ERROR:
            resumeStatusStr = "DEVICE_ERROR";
            break;

        case FBE_RESUME_PROM_STATUS_BUFFER_SMALL:
            resumeStatusStr = "BUFFER_SMALL";
            break;

        case FBE_RESUME_PROM_STATUS_INVALID_FIELD:
            resumeStatusStr = "INVALID_FIELD";
            break;

        case FBE_RESUME_PROM_STATUS_FIELD_NOT_WRITABLE:
            resumeStatusStr = "FIELD_NOT_WRITABLE";
            break;

        case FBE_RESUME_PROM_STATUS_UNKNOWN_DEVICE_ID:
            resumeStatusStr = "UNKNOWN_DEVICE_ID";
            break;

        case FBE_RESUME_PROM_STATUS_SMBUS_ERROR:
            resumeStatusStr = "SMBUS_ERROR";
            break;

        case FBE_RESUME_PROM_STATUS_INSUFFICIENT_RESOURCES:
            resumeStatusStr = "INSUFFICIENT_RESOURCES";
            break;

        case FBE_RESUME_PROM_STATUS_DEVICE_NOT_PRESENT:
            resumeStatusStr = "DEVICE_NOT_PRESENT";
            break;

        case FBE_RESUME_PROM_STATUS_DEVICE_TIMEOUT:
            resumeStatusStr = "DEVICE_TIMEOUT";
            break;

        case FBE_RESUME_PROM_STATUS_DEVICE_NOT_VALID_FOR_PLATFORM:
            resumeStatusStr = "DEVICE_NOT_VALID_FOR_PLATFORM";
            break;

        case FBE_RESUME_PROM_STATUS_ARB_ERROR:
            resumeStatusStr = "ARB_ERROR";
            break;

        case FBE_RESUME_PROM_STATUS_INVALID:
            resumeStatusStr = "INVALID";
            break;

        case FBE_RESUME_PROM_STATUS_DEVICE_POWERED_OFF:
            resumeStatusStr = "DEVICE_POWERED_OFF";
            break;

        default:
            resumeStatusStr = "UNDEFINED";
            break;
    }

    return resumeStatusStr;
}

/*!**************************************************************
 * @fn fbe_base_env_decode_resume_prom_work_state(fbe_base_env_resume_prom_work_state_t workState)
 ****************************************************************
 * @brief
 *  This function decodes the resume prom work item work state.
 *
 * @param workState - 
 *
 * @return the work state string.
 *
 * @version
 * 02-Aug-2011:  PHE - Created. 
 *
 ****************************************************************/
char * fbe_base_env_decode_resume_prom_work_state(fbe_base_env_resume_prom_work_state_t workState)
{
    char *workStateStr;

    switch(workState)
    {
        case FBE_BASE_ENV_RESUME_PROM_READ_QUEUED:
            workStateStr = "READ_QUEUED";
            break;
    
        case FBE_BASE_ENV_RESUME_PROM_READ_SENT:
            workStateStr = "READ_SENT";
            break;

        case FBE_BASE_ENV_RESUME_PROM_READ_COMPLETED:
            workStateStr = "READ_COMPLETED";
            break;

        default:
            workStateStr = "UNDEFINED";
            break;
    }

    return workStateStr;
} 

/*!**************************************************************
 * @fn fbe_base_env_decode_connector_cable_status(fbe_cable_status_t cableStatus)
 ****************************************************************
 * @brief
 *  This function decodes the connector cable status.
 *
 * @param cableStatus - 
 *
 * @return the cable status string.
 *
 * @version
 *  24-Aug-2011:  PHE - Created. 
 *
 ****************************************************************/
char * fbe_base_env_decode_connector_cable_status(fbe_cable_status_t cableStatus)
{
    char * statusStr;

    switch(cableStatus)
    {
        case FBE_CABLE_STATUS_UNKNOWN:
            statusStr = "UNKNOWN";
            break;

        case FBE_CABLE_STATUS_GOOD:
            statusStr = "GOOD";
            break;

        case FBE_CABLE_STATUS_DEGRADED:
            statusStr = "DEGRADED";
            break;

        case FBE_CABLE_STATUS_DISABLED:
            statusStr = "DISABLED";
            break;

        case FBE_CABLE_STATUS_INVALID:
            statusStr = "INVALID";
            break;

        case FBE_CABLE_STATUS_MISSING:
            statusStr = "MISSING";
            break;

        case FBE_CABLE_STATUS_CROSSED:
            statusStr = "CROSSED";
            break;

        default:
            statusStr = "UNDEFINED";
            break;
    }

    return statusStr;
}

/*!**************************************************************
 * @fn fbe_base_env_decode_connector_type(fbe_connector_type_t connectorType)
 ****************************************************************
 * @brief
 *  This function decodes the connector type.
 *
 * @param connectorType
 *
 * @return the connector type string.
 *
 * @version
 *  30-Oct-2012:  zhoue1 - Created. 
 *
 ****************************************************************/
char * fbe_base_env_decode_connector_type(fbe_connector_type_t connectorType)
{
    char * statusStr;

    switch(connectorType)
    {
        case FBE_CONNECTOR_TYPE_INTERNAL:
            statusStr = "internal port";
            break;

        case FBE_CONNECTOR_TYPE_PRIMARY:
            statusStr = "primary port";
            break;

        case FBE_CONNECTOR_TYPE_EXPANSION:
            statusStr = "expansion port";
            break;

        case FBE_CONNECTOR_TYPE_UNUSED:
            statusStr = "unused port";
            break;

        case FBE_CONNECTOR_TYPE_UNKNOWN:
            statusStr = "port type unknown";
            break;

        default:
            statusStr = "undefined";
            break;
    }

    return statusStr;
}

/*!**************************************************************
 * @fn fbe_base_env_decode_led_status(fbe_led_status_t ledStatus)
 ****************************************************************
 * @brief
 *  This function decodes the LED status.
 *
 * @param ledStatus - 
 *
 * @return the LED status string.
 *
 * @version
 *  13-Dec-2011:  PHE - Created. 
 *
 ****************************************************************/
char * fbe_base_env_decode_led_status(fbe_led_status_t ledStatus)
{
    char * statusStr;

    switch(ledStatus)
    {
        case FBE_LED_STATUS_OFF:
            statusStr = "OFF";
            break;

        case FBE_LED_STATUS_ON:
            statusStr = "ON";
            break;

        case FBE_LED_STATUS_MARKED:
            statusStr = "MARKED";
            break;

        case FBE_LED_STATUS_INVALID:
            statusStr = "INVALID";
            break;

        default:
            statusStr = "UNKNOWN";
            break;
    }

    return statusStr;
}

/*!**************************************************************
 * @fn fbe_encl_mgmt_decode_encl_fault_led_reason(fbe_u32_t enclFaultLedReason)
 ****************************************************************
 * @brief
 *  This function decodes the enclFaultLedReason.
 *
 * @param enclFaultLedReason - 
 * 
 * @return string for enclFaultLedReason
 * 
 * @author
 *  13-Dec-2011: PHE - Created.
 *
 ****************************************************************/
char * fbe_base_env_decode_encl_fault_led_reason(fbe_encl_fault_led_reason_t enclFaultLedReason)
{
    char        *string = enclFaultReasonString;
    fbe_u64_t    bit = 0;

    string[0] = 0;
    if (enclFaultLedReason == FBE_ENCL_FAULT_LED_NO_FLT)
    {
        strncat(string, "No Fault", sizeof(enclFaultReasonString)-strlen(string)-1);
    }
    else
    {
        for(bit = 0; bit < 64; bit++)
        {

            if((1ULL << bit) >= FBE_ENCL_FAULT_LED_BITMASK_MAX)
            {
                break;
            }

            switch (enclFaultLedReason & (1ULL << bit))
            {
            case FBE_ENCL_FAULT_LED_NO_FLT:
                break;
            case FBE_ENCL_FAULT_LED_PS_FLT:
                strncat(string, " PS Fault,", sizeof(enclFaultReasonString)-strlen(string)-1);
                break;
            case FBE_ENCL_FAULT_LED_FAN_FLT:
                strncat(string, " Fan Fault,", sizeof(enclFaultReasonString)-strlen(string)-1);
                break;
            case FBE_ENCL_FAULT_LED_DRIVE_FLT:
                strncat(string, " Drive Fault,", sizeof(enclFaultReasonString)-strlen(string)-1);
                break;
            case FBE_ENCL_FAULT_LED_SPS_FLT:
                strncat(string, " SPS Fault,", sizeof(enclFaultReasonString)-strlen(string)-1);
                break;
            case FBE_ENCL_FAULT_LED_OVERTEMP_FLT:
                strncat(string, " Overtemp Fault,", sizeof(enclFaultReasonString)-strlen(string)-1);
                break;
            case FBE_ENCL_FAULT_LED_LCC_FLT:
                strncat(string, " LCC Fault,", sizeof(enclFaultReasonString)-strlen(string)-1);
                break;
            case FBE_ENCL_FAULT_LED_LCC_RESUME_PROM_FLT:
                strncat(string, " LCC RP Fault,", sizeof(enclFaultReasonString)-strlen(string)-1);
                break;
            case FBE_ENCL_FAULT_LED_CONNECTOR_FLT:
                strncat(string, " Connecter Fault,", sizeof(enclFaultReasonString)-strlen(string)-1);
                break;
            case FBE_ENCL_FAULT_LED_PS_RESUME_PROM_FLT:
                strncat(string, " PS RP Fault,", sizeof(enclFaultReasonString)-strlen(string)-1);
                break;
            case FBE_ENCL_FAULT_LED_FAN_RESUME_PROM_FLT:
                strncat(string, " Fan RP Fault,", sizeof(enclFaultReasonString)-strlen(string)-1);
                break;
            case FBE_ENCL_FAULT_LED_ENCL_LIFECYCLE_FAIL:
                strncat(string, " Encl Lifecycle Fail,", sizeof(enclFaultReasonString)-strlen(string)-1);
                break;
            case FBE_ENCL_FAULT_LED_SUBENCL_LIFECYCLE_FAIL:
                strncat(string, " Subencl Lifecycle Fail,", sizeof(enclFaultReasonString)-strlen(string)-1);
                break;
            case FBE_ENCL_FAULT_LED_LCC_CABLING_FLT:
                strncat(string, " LCC Cabling Fault,", sizeof(enclFaultReasonString)-strlen(string)-1);
                break;
            case FBE_ENCL_FAULT_LED_EXCEEDED_MAX:
                strncat(string, " Exceed Max Encl Limit,", sizeof(enclFaultReasonString)-strlen(string)-1);
                break;
            case FBE_ENCL_FAULT_LED_BATTERY_FLT:
                strncat(string, " BBU Fault,", sizeof(enclFaultReasonString)-strlen(string)-1);
                break;
            case FBE_ENCL_FAULT_LED_BATTERY_RESUME_PROM_FLT:
                strncat(string, " BBU RP Fault,", sizeof(enclFaultReasonString)-strlen(string)-1);
                break;
            case FBE_ENCL_FAULT_LED_SP_FLT:
                strncat(string, " SP Fault,", sizeof(enclFaultReasonString)-strlen(string)-1);
                break;
            case FBE_ENCL_FAULT_LED_SP_FAULT_REG_FLT:
                strncat(string, " FaultReg Fault,", sizeof(enclFaultReasonString)-strlen(string)-1);
                break;
            case FBE_ENCL_FAULT_LED_SP_RESUME_PROM_FLT:
                strncat(string, " SP RP Fault,", sizeof(enclFaultReasonString)-strlen(string)-1);
                break;
            case FBE_ENCL_FAULT_LED_SYSTEM_SERIAL_NUMBER_FLT:
                strncat(string, " System Serial Number Invalid,", sizeof(enclFaultReasonString)-strlen(string)-1);
                break;
            case FBE_ENCL_FAULT_LED_MGMT_MODULE_FLT:
                strncat(string, " Management Module Fault,", sizeof(enclFaultReasonString)-strlen(string)-1);
                break;
            case FBE_ENCL_FAULT_LED_MGMT_MODULE_RESUME_PROM_FLT:
                strncat(string, " Management Module RP Fault,", sizeof(enclFaultReasonString)-strlen(string)-1);
                break;
            case FBE_ENCL_FAULT_LED_PEER_SP_FLT:
                strncat(string, " Peer SP Fault,", sizeof(enclFaultReasonString)-strlen(string)-1);
                break;
            case FBE_ENCL_FAULT_LED_CACHE_CARD_FLT:
                strcat(string, " Cache Card Fault,");
                break;
            case FBE_ENCL_FAULT_LED_DIMM_FLT:
                strcat(string, " DIMM Fault,");
                break;
            case FBE_ENCL_FAULT_LED_IO_MODULE_RESUME_PROM_FLT:
                strncat(string, " IO Module RP Fault,", sizeof(enclFaultReasonString)-strlen(string)-1);
                break;
            case FBE_ENCL_FAULT_LED_BEM_RESUME_PROM_FLT:
                strncat(string, " Back End Module RP Fault,", sizeof(enclFaultReasonString)-strlen(string)-1);
                break;
            case FBE_ENCL_FAULT_LED_MEZZANINE_RESUME_PROM_FLT:
                strncat(string, " Mezzanine RP Fault,", sizeof(enclFaultReasonString)-strlen(string)-1);
                break;
            case FBE_ENCL_FAULT_LED_MIDPLANE_RESUME_PROM_FLT:
                strncat(string, " Midplane RP Fault,", sizeof(enclFaultReasonString)-strlen(string)-1);
                break;
            case FBE_ENCL_FAULT_LED_DRIVE_MIDPLANE_RESUME_PROM_FLT:
                strncat(string, " Drive Midplane RP Fault,", sizeof(enclFaultReasonString)-strlen(string)-1);
                break;
            case FBE_ENCL_FAULT_LED_IO_PORT_FLT:
                strncat(string, " IO Port Fault,", sizeof(enclFaultReasonString)-strlen(string)-1);
                break;
            case FBE_ENCL_FAULT_LED_IO_MODULE_FLT:
                strncat(string, " IO Module Fault,", sizeof(enclFaultReasonString)-strlen(string)-1);
                break;
            case FBE_ENCL_FAULT_LED_SSC_FLT:
                strncat(string, " SSC Fault,", sizeof(enclFaultReasonString)-strlen(string)-1);
                break;
            case FBE_ENCL_FAULT_LED_NOT_SUPPORTED_FLT:
                strncat(string, " Unsupported Encl Fault,", sizeof(enclFaultReasonString)-strlen(string)-1);
                break;
            case FBE_ENCL_FAULT_LED_INTERNAL_CABLE_FLT:
                strncat(string, " SP Internal Cable Fault,", sizeof(enclFaultReasonString)-strlen(string)-1);
                break;
            default:
                strncat(string, " Unknown Fault,", sizeof(enclFaultReasonString)-strlen(string)-1);
                break;
            }   // end of switch
        }   // for bit
        string[strlen(string)-1] = '\0';    //omit the lass comma
    }

    return(string);
}   

/*!**************************************************************
 * @fn fbe_base_env_decode_class_id(fbe_class_id_t classId)
 ****************************************************************
 * @brief
 *  This function decodes the ESP class id.
 *
 * @param classId - 
 *
 * @return the class id string.
 *
 * @version
 *  12-Dec-2012:  PHE - Created. 
 *
 ****************************************************************/
char * fbe_base_env_decode_class_id(fbe_class_id_t classId)
{
    char * classIdStr;

    switch(classId)
    {
        case FBE_CLASS_ID_BASE_ENVIRONMENT:
            classIdStr = "BASE_ENVIRONMENT";
            break;

        case FBE_CLASS_ID_ENCL_MGMT:
            classIdStr = "ENCL_MGMT";
            break;

        case FBE_CLASS_ID_SPS_MGMT:
            classIdStr = "SPS_MGMT";
            break;
    
        case FBE_CLASS_ID_DRIVE_MGMT:
            classIdStr = "DRIVE_MGMT";
            break;
    
        case FBE_CLASS_ID_MODULE_MGMT:
            classIdStr = "MODULE_MGMT";
            break;
    
        case FBE_CLASS_ID_PS_MGMT:
            classIdStr = "PS_MGMT";
            break;
    
        case FBE_CLASS_ID_BOARD_MGMT:
            classIdStr = "BOARD_MGMT";
            break;
    
        case FBE_CLASS_ID_COOLING_MGMT:
            classIdStr = "COOLING_MGMT";
            break;
        
        default:
            classIdStr = "UNDEFINED CLASS ID";
            break;
    }

    return classIdStr;
}

/*!**************************************************************
 * @fn fbe_base_env_decode_cache_status(fbe_common_cache_status_t status)
 ****************************************************************
 * @brief
 *  This function decodes the common cache status.
 *
 * @param status - cache status enumeration
 *
 * @return the cache status string.
 *
 * @version
 *  31-Jan-2013:  Lin Lou - Created.
 *
 ****************************************************************/
char * fbe_base_env_decode_cache_status(fbe_common_cache_status_t status)
{
    char * cacheStatus;
    switch (status)
    {
        case FBE_CACHE_STATUS_OK:
            cacheStatus = "OK";
            break;
        case FBE_CACHE_STATUS_DEGRADED:
            cacheStatus = "DEGRADED";
            break;
        case FBE_CACHE_STATUS_FAILED:
            cacheStatus = "FAILED";
            break;
        case FBE_CACHE_STATUS_FAILED_SHUTDOWN:
            cacheStatus = "SHUTDOWN";
            break;
        case FBE_CACHE_STATUS_FAILED_ENV_FLT:
            cacheStatus = "FAILED_EF";
            break;
        case FBE_CACHE_STATUS_FAILED_SHUTDOWN_ENV_FLT:
            cacheStatus = "SHUTDOWN_EF";
            break;
        case FBE_CACHE_STATUS_APPROACHING_OVER_TEMP:
            cacheStatus = "APPROACH_OT";
            break;
        case FBE_CACHE_STATUS_UNINITIALIZE:
            cacheStatus = "UNINITIALIZE";
            break;
        default:
            cacheStatus = "UNKNOWN";
            break;
    }
    return cacheStatus;
}

/*!*******************************************************************************
 * @fn fbe_base_env_decode_firmware_target
 *********************************************************************************
 * @brief
 *   This function is to decode the firmware target.
 *
 * @param firmwareTarget - 
 * 
 * @return the string of the firmware target.
 *
 * @version
 *   01-Aug-2014 PHE - Created.
 *******************************************************************************/
char * fbe_base_env_decode_firmware_target(fbe_enclosure_fw_target_t firmwareTarget)
{
    char * firmwareTargetStr;

    switch (firmwareTarget)
    {
        case FBE_FW_TARGET_LCC_EXPANDER:
            firmwareTargetStr = "Expander";
            break;
            
        case FBE_FW_TARGET_LCC_INIT_STRING:
            firmwareTargetStr = "initStr";
            break;

        case FBE_FW_TARGET_LCC_FPGA:
            firmwareTargetStr = "CDEF";
            break;

        case FBE_FW_TARGET_LCC_MAIN: 
            firmwareTargetStr = "Main";
            break;

        case FBE_FW_TARGET_PS:
            firmwareTargetStr = "PowerSupply";
            break;

        default:
            firmwareTargetStr = "Unhandled";
            break;
    }

    return firmwareTargetStr;
}

//End of file fbe_base_environment_debug.c
