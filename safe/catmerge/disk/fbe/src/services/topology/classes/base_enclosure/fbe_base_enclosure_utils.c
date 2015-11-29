/**********************************************************************
 *  Copyright (C)  EMC Corporation 2008
 *  All rights reserved.
 *  Licensed material - property of EMC Corporation.
 **********************************************************************/

/*!*************************************************************************
 * @file fbe_base_enclosure_utils.c
 ***************************************************************************
 *
 * @brief
 *  The routines in this file contains all the utility functions for base enclosure.
 *  
 *
 * @ingroup fbe_enclosure_class
 *
 * NOTE: 
 *
 * HISTORY
 *   02-Apr-2009 Dipak Patel - Created. 
 *
 ***************************************************************************/
#include "fbe/fbe_types.h"
#include "fbe_base_enclosure_debug.h"
#include "base_enclosure_private.h"

/*!**************************************************************
 * @fn fbe_base_enclosure_print_prvt_data(
 *                         void * enclPrvtData, 
 *                         fbe_trace_func_t trace_func,
 *                         fbe_trace_context_t trace_context)
 ****************************************************************
 * @brief
 *  This function will print private data of sas
 *  enclosure.
 *
 * @param enclPrvtData - pointer to the void.
 * @param trace_func    - callback to printing function.
 * @param trace_context - trace context
 *
 * @return nothing
 *
 * HISTORY:
 *   04/02/2009:  Created. Dipak Patel
 *
 ****************************************************************/ 
void fbe_base_enclosure_print_prvt_data(void * enclPrvtData,
                                              fbe_trace_func_t trace_func,
                                              fbe_trace_context_t trace_context)
{
    fbe_base_enclosure_t * basePrvtData = (fbe_base_enclosure_t *)enclPrvtData;
    trace_func(trace_context, "\nBASE Private Data:\n");

    fbe_base_enclosure_print_enclosure_number(basePrvtData->enclosure_number, trace_func, trace_context);
    fbe_base_enclosure_print_port_number(basePrvtData->port_number, trace_func, trace_context); 
    fbe_base_enclosure_print_number_of_slots(basePrvtData->number_of_slots, trace_func, trace_context);
    fbe_base_enclosure_print_first_slot_index(basePrvtData->first_slot_index, trace_func, trace_context);
    fbe_base_enclosure_print_first_expansion_port_index(basePrvtData->first_expansion_port_index, trace_func, trace_context);
    fbe_base_enclosure_print_number_of_expansion_ports(basePrvtData->number_of_expansion_ports, trace_func, trace_context);
    fbe_base_enclosure_print_time_ride_through_start(basePrvtData->time_ride_through_start, trace_func, trace_context);
    fbe_base_enclosure_print_expected_ride_through_period(basePrvtData->expected_ride_through_period, trace_func, trace_context);
    fbe_base_enclosure_print_default_ride_through_period(basePrvtData->default_ride_through_period, trace_func, trace_context);  
    fbe_base_enclosure_print_number_of_drives_spinningup(basePrvtData->number_of_drives_spinningup, trace_func, trace_context); 
    fbe_base_enclosure_print_max_number_of_drives_spinningup(basePrvtData->max_number_of_drives_spinningup, trace_func, trace_context);
    fbe_base_enclosure_print_encl_fault_symptom(basePrvtData->enclosure_faults.faultSymptom,trace_func,trace_context);
    fbe_base_enclosure_print_discovery_opcode(basePrvtData->discovery_opcode,trace_func,trace_context);
    fbe_base_enclosure_print_control_code(basePrvtData->control_code,trace_func, trace_context);
}

/****************************************************************
 * @fn fbe_base_enclosure_print_encl_fault_symptom(
 *                         fbe_u32_t scsi_error_code 
 ****************************************************************
 * @brief
 *  This function checks if passed argument is a scsi error code.
 *  enclosure.
 *
 * @param scsi_error_code - scsi_error_code to be checked.
 *
 * @return fbe_boot_t - TRUE if it is a SCSI error 
 *                    - FALSE otherwise
 *
 * HISTORY:
 *   05/25/2009:  Created. Nayana Chaudhari
 *
 ****************************************************************/
fbe_bool_t fbe_base_enclosure_if_scsi_error(fbe_u32_t scsi_error_code)
{
    fbe_bool_t is_scsi_error = FALSE;

    switch(scsi_error_code)
    {
        case FBE_SCSI_UNEXPECTEDDISCONNECT:
        case FBE_SCSI_BADINTERRUPT:
        case FBE_SCSI_BADBUSPHASE:
        case FBE_SCSI_BADMESSAGE:
        case FBE_SCSI_BADSTATUS:
        case FBE_SCSI_BADRESELECTION:
        case FBE_SCSI_PARITYERROR:
        case FBE_SCSI_MSGPARERR:
        case FBE_SCSI_STATUSPARITYERROR:
        case FBE_SCSI_INT_SCB_NOT_FOUND:
        case FBE_SCSI_DEVICE_NOT_PRESENT:
        //case FBE_SCSI_SELECTIONTIMEOUT:
        case FBE_SCSI_BUSRESET:
        //case FBE_SCSI_DRVABORT:
        case FBE_SCSI_USERRESET:
        case FBE_SCSI_USERABORT:
        case FBE_SCSI_XFERCOUNTNOTZERO:
        case FBE_SCSI_TOOMUCHDATA:
        case FBE_SCSI_BUSSHUTDOWN:
        case FBE_SCSI_INVALIDSCB:
        case FBE_SCSI_SLOT_BUSY:
        case FBE_SCSI_CHANNEL_INACCESSIBLE:
        case FBE_SCSI_FCP_INVALIDRSP:
        case FBE_SCSI_DEVICE_NOT_READY:
        case FBE_SCSI_IO_TIMEOUT_ABORT:
        case FBE_SCSI_SATA_IO_TIMEOUT_ABORT:
        case FBE_SCSI_IO_LINKDOWN_ABORT:
        case FBE_SCSI_NO_DEVICE_DRIVER:
        case FBE_SCSI_UNSUPPORTEDFUNCTION:
        case FBE_SCSI_UNSUPPORTED_IOCTL_OPTION:
        case FBE_SCSI_INVALIDREQUEST:
        case FBE_SCSI_UNKNOWNRESPONSE:
        case FBE_SCSI_DEVUNITIALIZED:
        case FBE_SCSI_DEVICE_RESERVED:
        case FBE_SCSI_DEVICE_BUSY:
        case FBE_SCSI_AUTO_SENSE_FAILED:
        case FBE_SCSI_CHECK_CONDITION_PENDING:
        case FBE_SCSI_CHECK_COND_OTHER_SLOT:
        case FBE_SCSI_CC_NOERR:
        case FBE_SCSI_CC_AUTO_REMAPPED:
        case FBE_SCSI_CC_RECOVERED_BAD_BLOCK:
        case FBE_SCSI_CC_RECOVERED_ERR_CANT_REMAP:
        case FBE_SCSI_CC_RECOVERED_ERR_NOSYNCH:
        case FBE_SCSI_CC_RECOVERED_ERR:
        case FBE_SCSI_CC_BECOMING_READY:
        case FBE_SCSI_CC_NOT_SPINNING:
        case FBE_SCSI_CC_NOT_READY:
        case FBE_SCSI_CC_FORMAT_CORRUPTED:
        case FBE_SCSI_CC_MEDIA_ERROR_BAD_DEFECT_LIST:
        case FBE_SCSI_CC_HARD_BAD_BLOCK:
        case FBE_SCSI_CC_MEDIA_ERR_CANT_REMAP:
        case FBE_SCSI_CC_HARDWARE_ERROR_PARITY:        
        case FBE_SCSI_CC_HARDWARE_ERROR:
        case FBE_SCSI_CC_ILLEGAL_REQUEST:
        case FBE_SCSI_CC_DEV_RESET:
        case FBE_SCSI_CC_MODE_SELECT_OCCURRED:
        case FBE_SCSI_CC_SYNCH_SUCCESS:
        case FBE_SCSI_CC_SYNCH_FAIL:
        case FBE_SCSI_CC_UNIT_ATTENTION:
        case FBE_SCSI_CC_ABORTED_CMD_PARITY_ERROR:
        case FBE_SCSI_CC_ABORTED_CMD:
        case FBE_SCSI_CC_UNEXPECTED_SENSE_KEY:
        case FBE_SCSI_CC_MEDIA_ERR_DO_REMAP:
        case FBE_SCSI_CC_MEDIA_ERR_DONT_REMAP:
        case FBE_SCSI_CC_HARDWARE_ERROR_FIRMWARE:
        case FBE_SCSI_CC_HARDWARE_ERROR_NO_SPARE:
        case FBE_SCSI_CC_GENERAL_FIRMWARE_ERROR:
        case FBE_SCSI_CC_MEDIA_ERR_WRITE_ERROR:
        case FBE_SCSI_CC_DEFECT_LIST_ERROR:
        case FBE_SCSI_CC_VENDOR_SPECIFIC_09_80_00:
        case FBE_SCSI_CC_SEEK_POSITIONING_ERROR:
        case FBE_SCSI_CC_SEL_ID_ERROR:
        case FBE_SCSI_CC_RECOVERED_WRITE_FAULT:
        case FBE_SCSI_CC_MEDIA_ERROR_WRITE_FAULT:
        case FBE_SCSI_CC_HARDWARE_ERROR_WRITE_FAULT:
        case FBE_SCSI_CC_INTERNAL_TARGET_FAILURE:
        case FBE_SCSI_CC_RECOVERED_DEFECT_LIST_ERROR:
        case FBE_SCSI_CC_ABORTED_CMD_ATA_TO:
        case FBE_SCSI_CC_INVALID_LUN:
        case FBE_SCSI_LOGICAL_UNIT_FAILED_SELF_CONFIGURATION:
        case FBE_SCSI_CC_DATA_OFFSET_ERROR:
        case FBE_SCSI_CC_SUPER_CAP_FAILURE:
        case FBE_SCSI_CC_DRIVE_TABLE_REBUILD:
        case FBE_SCSI_CC_DEFERRED_ERROR:
        case FBE_SCSI_CC_TARGET_OPERATING_COND_CHANGED:
        case FBE_SCSI_CC_UNSUPPORTED_ENCL_FUNC:
        case FBE_SCSI_CC_MODE_PARAM_CHANGED:
        case FBE_SCSI_CC_ENCLOSURE_SERVICES_TRANSFER_REFUSED:
        case FBE_SCSI_CC_DIE_RETIREMENT_START:
        case FBE_SCSI_CC_DIE_RETIREMENT_END:
            is_scsi_error = TRUE;
            break;
        default:
            is_scsi_error = FALSE;
            break;
    }
    return is_scsi_error;
}

/*!**************************************************************
 * @fn fbe_base_enclosure_getFaultSymptomString() 
 ****************************************************************
 * @brief
 *  This function returns the enclosure fault symptom in String.
 *
 * @param fault_symptom - fault symptom to be printed.
 *
 * @return nothing
 *
 * HISTORY:
 *   18/09/2009:  Created. Arunkumar Selvapillai
 *
 ****************************************************************/
char * fbe_base_enclosure_getFaultSymptomString(fbe_u32_t fault_symptom) 
{
    char * fault_String;

    switch (fault_symptom)
    {
        case FBE_ENCL_FLTSYMPT_NONE:
            fault_String = (char *)("No Fault");
            break;
        case FBE_ENCL_FLTSYMPT_DUPLICATE_UID:
            fault_String = (char *)("Duplicate UID");
            break;
        case FBE_ENCL_FLTSYMPT_ALLOCATED_MEMORY_INSUFFICIENT:
            fault_String = (char *)("Allocated Memory Insufficient");
            break;
        case FBE_ENCL_FLTSYMPT_PROCESS_PAGE_FAILED:
            fault_String = (char *)("Process Page Failed");
            break;
        case FBE_ENCL_FLTSYMPT_CMD_FAILED:
            fault_String = (char *)("Command Failed");
            break;
        case FBE_ENCL_FLTSYMPT_ENCL_FUNC_UNSUPPORTED:
            fault_String = (char *)("Unsupported Function");
            break;
        case FBE_ENCL_FLTSYMPT_PARAMETER_INVALID:
            fault_String = (char *)("Invalid Parameter");
            break;
        case FBE_ENCL_FLTSYMPT_HARDWARE_ERROR:
            fault_String = (char *)("Hardware Error");
            break;
        case FBE_ENCL_FLTSYMPT_CDB_REQUEST_FAILED:
            fault_String = (char *)("CDB Request Failed");
            break;
        case FBE_ENCL_FLTSYMPT_LIFECYCLE_FAILED:
            fault_String = (char *)("Lifecycle Failed");
            break;
        case FBE_ENCL_FLTSYMPT_EDAL_FAILED:
            fault_String = (char *)("EDAL Failed");
            break;
        case FBE_ENCL_FLTSYMPT_CONFIG_INVALID:
            fault_String = (char *)("Invalid Configuration");
            break;
        case FBE_ENCL_FLTSYMPT_MAPPING_FAILED:
            fault_String = (char *)("Mapping Failed");
            break;
        case FBE_ENCL_FLTSYMPT_COMP_UNSUPPORTED:
            fault_String = (char *)("Unsupported Component");
            break;
        case FBE_ENCL_FLTSYMPT_ESES_PAGE_INVALID:
            fault_String = (char *)("Invalid ESES Page");
            break;
        case FBE_ENCL_FLTSYMPT_CTRL_CODE_UNSUPPORTED:
            fault_String = (char *)("Unsupported Control Code");
            break;
        case FBE_ENCL_FLTSYMPT_DATA_ILLEGAL:
            fault_String = (char *)("Illegal Data");
            break;
        case FBE_ENCL_FLTSYMPT_BUSY:
            fault_String = (char *)("Busy");
            break;
        case FBE_ENCL_FLTSYMPT_DATA_ACCESS_FAILED:
            fault_String = (char *)("Data Access Failed");
            break;
        case FBE_ENCL_FLTSYMPT_UNSUPPORTED_PAGE_HANDLED:
            fault_String = (char *)("Unsupported Page Handled");
            break;
        case FBE_ENCL_FLTSYMPT_MODE_SELECT_NEEDED:
            fault_String = (char *)("Mode Select Needed");
            break;
        case FBE_ENCL_FLTSYMPT_MEM_ALLOC_FAILED:
            fault_String = (char *)("Memory Allocation Failed");
            break;
        case FBE_ENCL_FLTSYMPT_PACKET_FAILED:
            fault_String = (char *)("Packet Failed");
            break;
        case FBE_ENCL_FLTSYMPT_EDAL_NOT_NEEDED:
            fault_String = (char *)("EDAL Not Needed");
            break;
        case FBE_ENCL_FLTSYMPT_TARGET_NOT_FOUND:
            fault_String = (char *)("Target Not Found");
            break;
        case FBE_ENCL_FLTSYMPT_ELEM_GROUP_INVALID:
            fault_String = (char *)("Invalid Element Group");
            break;
        case FBE_ENCL_FLTSYMPT_PATH_ATTR_UNAVAIL:
            fault_String = (char *)("Path Attribute Unavailable");
            break;
        case FBE_ENCL_FLTSYMPT_CHECKSUM_ERROR:
            fault_String = (char *)("Checksum Error");
            break;
        case FBE_ENCL_FLTSYMPT_ILLEGAL_REQUEST:
            fault_String = (char *)("Illegal Request");
            break;
        case FBE_ENCL_FLTSYMPT_BUILD_CDB_FAILED:
            fault_String = (char *)("Build CDB Failed");
            break;
        case FBE_ENCL_FLTSYMPT_BUILD_PAGE_FAILED:
            fault_String = (char *)("Build Page Failed");
            break;
        case FBE_ENCL_FLTSYMPT_NULL_PTR:
            fault_String = (char *)("Null Pointer Error");
            break;
        case FBE_ENCL_FLTSYMPT_INVALID_CANARY:
            fault_String = (char *)("Invalid Canary Error");
            break;
        case FBE_ENCL_FLTSYMPT_EDAL_ERROR:
            fault_String = (char *)("EDAL Error");
            break;
        case FBE_ENCL_FLTSYMPT_COMPONENT_NOT_FOUND:
            fault_String = (char *)("Component Not Found");
            break;
        case FBE_ENCL_FLTSYMPT_ATTRIBUTE_NOT_FOUND:
            fault_String = (char *)("Attribute Not Found");
            break;
        case FBE_ENCL_FLTSYMPT_COMP_TYPE_INDEX_INVALID:
            fault_String = (char *)("Invalid Component Type Index");
            break;
        case FBE_ENCL_FLTSYMPT_COMP_TYPE_UNSUPPORTED:
            fault_String = (char *)("Unsupported Component Type");
            break;
        case FBE_ENCL_FLTSYMPT_UNSUPPORTED_ENCLOSURE:
            fault_String = (char *)("Unsupported Enclosure");
            break;
        case FBE_ENCL_FLTSYMPT_INSUFFICIENT_RESOURCE:
            fault_String = (char *)("Insufficient Resource");
            break;
        case FBE_ENCL_FLTSYMPT_MAP_COMP_INDEX_FAILED:
            fault_String = (char *) ("Component Index Mapping Failed");
            break;
        case FBE_ENCL_FLTSYMPT_INVALID_ENCL_PLATFORM_TYPE:
            fault_String = (char *) ("Invalid Enclosure Platform Type");
            break;
        case FBE_ENCL_FLTSYMPT_MINIPORT_FAULT_ENCL:
            fault_String = (char *) ("Miniport Fault Enclosure");
            break;
        case FBE_ENCL_FLTSYMPT_UPSTREAM_ENCL_FAULTED:
            fault_String = (char *) ("Upstream Enclosure Faulted");
            break;
        case FBE_ENCL_FLTSYMPT_INVALID_ESES_VERSION_DESCRIPTOR:
            fault_String = (char *) ("Invalid ESES version descriptor");
            break;
        case FBE_ENCL_FLTSYMPT_EXCEEDS_MAX_LIMITS:
            fault_String = (char *) ("Exceeds platform limits");
            break;
        case FBE_ENCL_FLTSYMPT_CONFIG_UPDATE_FAILED:
            fault_String = (char *) ("Config update failed");
            break;
        case FBE_ENCL_FLTSYMPT_INVALID:
        default:
            fault_String = (char *)( "Unknown ");
        break;
    }

    return fault_String;
}

/*!*************************************************************************
 * @fn fbe_base_enclosure_getControlCodeString(
 *                    fbe_payload_control_operation_opcode_t controlCode)
 **************************************************************************
 *
 *  @brief
 *      This function will take a control code & return a string for
 *      display use.
 *
 *  @param    controlCode - value of a control code supported by Enclosure
 *
 *  @return    pointer to a string.
 *
 *  NOTES:
 *
 *  HISTORY:
 *    05-Sep-2008: Joe Perry - Created
 *    01-Oct-2009: Arun S - Moved from ESES to Base as we are printing the 
 *                          Base enclosure control codes. 
 *
 **************************************************************************/
char * fbe_base_enclosure_getControlCodeString(fbe_u32_t controlCode)
{
    char    *controlCodeString;

    switch(controlCode) 
    {
        case FBE_DISCOVERY_TRANSPORT_CONTROL_CODE_ATTACH_EDGE:
            controlCodeString = "TransportAttachEdge";
            break;
        case FBE_DISCOVERY_TRANSPORT_CONTROL_CODE_DETACH_EDGE:
            controlCodeString = "TransportDetachEdge";
            break;
        case FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_ENCLOSURE_NUMBER:
            controlCodeString = "GetEnclosureNumber";
            break;
        case FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_COMPONENT_ID:
            controlCodeString = "GetComponentId";
            break;
        case FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_SLOT_NUMBER:
            controlCodeString = "GetSlotNumber";
            break;
        case FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_ELEM_GROUP_INFO:
            controlCodeString = "GetElemGroupInfo";
            break;
        case FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_MAX_ELEM_GROUPS:
            controlCodeString = "GetMaxElemGroup";
            break;
        case FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_MGMT_ADDRESS:
            controlCodeString = "GetMgmtAddress";
            break;
        case FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_ENCLOSURE_FAULT_INFO:
            controlCodeString = "GetEnclosureFaultInfo";
            break;
        case FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_SCSI_ERROR_INFO:
            controlCodeString = "GetSCSIErrorInfo";
            break;
        case FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_ENCLOSURE_BASIC_INFO:
            controlCodeString = "GetEnclosureBasicInfo";
            break;
        case FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_ENCLOSURE_INFO:
            controlCodeString = "GetEnclosureInfo";
            break;
        case FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_ENCLOSURE_PRVT_INFO:
            controlCodeString = "GetEnclosurePrivateInfo";
            break;
        case FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_RESUME_PROM_INFO:
            controlCodeString = "GetResumePromInfo";
            break;
        case FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_ENCLOSURE_STATE:
            controlCodeString = "GetEnclosureState";
            break;
        case FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_ENCLOSURE_SETUP_INFO:
            controlCodeString = "GetEnclosureSetupInfo";
            break;
        case FBE_BASE_ENCLOSURE_CONTROL_CODE_UNBYPASS_PARENT:
            controlCodeString = "UnByPassParent";
            break;
        case FBE_BASE_ENCLOSURE_CONTROL_CODE_BYPASS_PARENT:
            controlCodeString = "ByPassParent";
            break;
        case FBE_BASE_ENCLOSURE_CONTROL_CODE_DRIVE_BYPASS:
            controlCodeString = "DriveByPass";
            break;
        case FBE_BASE_ENCLOSURE_CONTROL_CODE_DRIVE_UNBYPASS:
            controlCodeString = "DriveUnByPass";
            break;
        case FBE_BASE_ENCLOSURE_CONTROL_CODE_EXPANSION_PORT_BYPASS:
            controlCodeString = "ExpansionPortByPass";
            break;
        case FBE_BASE_ENCLOSURE_CONTROL_CODE_EXPANSION_PORT_UNBYPASS:
            controlCodeString = "ExpansionPortUnByPass";
            break;
        case FBE_BASE_ENCLOSURE_CONTROL_CODE_SET_ENCLOSURE_READ_STATE:
            controlCodeString = "SetEnclosureReadState";
            break;
        case FBE_BASE_ENCLOSURE_CONTROL_CODE_SET_DRIVE_FAULT_LED:
            controlCodeString = "SetDriveFaultLED";
            break;
        case FBE_BASE_ENCLOSURE_CONTROL_CODE_SET_ENCLOSURE_CONTROL:
            controlCodeString = "SetEnclosureControl";
            break;
        case FBE_BASE_ENCLOSURE_CONTROL_CODE_SET_ENCLOSURE_LEDS:
            controlCodeString = "SetEnclosureLeds";
            break;
        case FBE_BASE_ENCLOSURE_CONTROL_CODE_RESET_ENCLOSURE_SHUTDOWN_TIMER:
            controlCodeString = "ResetEnclosureShutdownTimer";
            break;
        case FBE_BASE_ENCLOSURE_CONTROL_CODE_ENCLOSURE_DEBUG_CONTROL:
            controlCodeString = "EnclosureDebugControl";
            break;
        case FBE_BASE_ENCLOSURE_CONTROL_CODE_DRIVE_DEBUG_CONTROL:
            controlCodeString = "DriveDebugControl";
            break;
        case FBE_BASE_ENCLOSURE_CONTROL_CODE_DRIVE_POWER_CONTROL:
            controlCodeString = "DrivePowerControl";
            break;
        case FBE_BASE_ENCLOSURE_CONTROL_CODE_EXPANDER_CONTROL:
            controlCodeString = "ExpanderControl";
            break;
        case FBE_BASE_ENCLOSURE_CONTROL_CODE_PS_MARGIN_TEST_CONTROL:
            controlCodeString = "PsMarginTestControl";
            break;
        case FBE_BASE_ENCLOSURE_CONTROL_CODE_EXPANDER_STRING_OUT_CONTROL:
            controlCodeString = "ExpanderStringOutControl";
            break;
        case FBE_BASE_ENCLOSURE_CONTROL_CODE_EXPANDER_TEST_MODE_CONTROL:
            controlCodeString = "ExpanderTestModeControl";
            break;
        case FBE_BASE_ENCLOSURE_CONTROL_CODE_LED_STATUS_CONTROL:
            controlCodeString = "LEDStatusControl";
            break;
        case FBE_BASE_ENCLOSURE_CONTROL_CODE_FIRMWARE_OP:
            controlCodeString = "FirmwareOp";
            break;
        case FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_DL_INFO:
            controlCodeString = "GetDLInfo";
            break;
        case FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_TRACE_BUF_INFO_STATUS:
            controlCodeString = "GetTraceBufferInfoStatus";
            break;
        case FBE_BASE_ENCLOSURE_CONTROL_CODE_SET_TRACE_BUF_INFO_CTRL:
            controlCodeString = "SetTraceBufferInfoControl";
            break;
        case FBE_BASE_ENCLOSURE_CONTROL_CODE_READ_BUF:
            controlCodeString = "ReadBuffer";
            break;
        case FBE_BASE_ENCLOSURE_CONTROL_CODE_DEBUG_CMD:
            controlCodeString = "DebugCommand";
            break;
        case FBE_BASE_ENCLOSURE_CONTROL_CODE_RESUME_READ:
            controlCodeString = "ResumeRead";
            break;
        case FBE_BASE_ENCLOSURE_CONTROL_CODE_RESUME_WRITE:
            controlCodeString = "ResumeWrite";
            break;
        case FBE_BASE_ENCLOSURE_CONTROL_CODE_RAW_RCV_DIAG:
            controlCodeString = "RawRecieveDiagnostic";
            break;
        case FBE_BASE_ENCLOSURE_CONTROL_CODE_RAW_INQUIRY:
            controlCodeString = "RawInquiry";
            break;
        case FBE_BASE_ENCLOSURE_CONTROL_CODE_RAW_MODE_SENSE:
            controlCodeString = "RawModeSense";
            break;
        case FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_SLOT_TO_PHY_MAPPING:
            controlCodeString = "GetSlotToPhyMapping";
            break;
        case FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_ENCLOSURE_STATISTICS:
            controlCodeString = "GetEnclosureStatistics";
            break;
        case FBE_BASE_ENCLOSURE_CONTROL_CODE_CLEAR_ENCLOSURE_STATISTICS:
            controlCodeString = "ClearEnclosureStatistics";
            break;
        case FBE_BASE_ENCLOSURE_CONTROL_CODE_PARSE_IMAGE_HEADER:
            controlCodeString = "ParseImageHeader";
            break;
        case FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_LCC_INFO:
            controlCodeString = "GetLccInfo";
            break;
        case FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_LCC_COUNT:
            controlCodeString = "GetLccCount";
            break;
        case FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_STAND_ALONE_LCC_COUNT:
            controlCodeString = "GetStandAloneLccCount";
            break;
        case FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_CONNECTOR_INFO:
            controlCodeString = "GetConnectorInfo";
            break;
        case FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_CONNECTOR_COUNT:
            controlCodeString = "GetConnectorCount";
            break;
        case FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_FAN_INFO:
            controlCodeString = "GetFanInfo";
            break;
        case FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_FAN_COUNT:
            controlCodeString = "GetFanCount";
            break;
        case FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_PS_INFO:
            controlCodeString = "GetPsInfo";
            break;
        case FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_PS_COUNT:
            controlCodeString = "GetPsCount";
            break;
        case FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_CHASSIS_COOLING_FAULT_STATUS:
            controlCodeString = "GetChassisCoolingFaultStatus";
            break;
        case FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_SSC_COUNT:
            controlCodeString = "GetSSCCount";
            break;
        case FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_SSC_INFO:
            controlCodeString = "GetSSCInfo";
            break;
        default:
            controlCodeString = "UnknownEnclosureControlCode";
            break;
    }

    return controlCodeString;

}   // end of fbe_base_enclosure_getControlCodeString


/*!**************************************************************
 * @fn fbe_base_enclosure_print_discovery_opcode(fbe_payload_discovery_opcode_t discovery_opcode,
 *                                                 fbe_trace_func_t trace_func,
 *                                                 fbe_trace_context_t trace_context)
 ****************************************************************
 * @brief
 *  This function will print fw activating related info
 *
 * @param discovery_opcode.
 * @param trace_func    - callback to printing function.
 * @param trace_context - trace context
 *
 * @return nothing
 *
 * HISTORY:
 *   09/08/2010:  Created. Prasenjeet Ghaywat
 *
 ****************************************************************/
void  fbe_base_enclosure_print_discovery_opcode(fbe_payload_discovery_opcode_t discovery_opcode,
                                                        fbe_trace_func_t trace_func, 
                                                        fbe_trace_context_t trace_context)                                                 
{
     trace_func(trace_context, "discovery opcode: %d\n", discovery_opcode);
}

/*!**************************************************************
 * @fn fbe_base_enclosure_print_control_code(fbe_payload_control_operation_opcode_t control_code,
 *                                                fbe_trace_func_t trace_func,
 *                                                fbe_trace_context_t trace_context)
 ****************************************************************
 * @brief
 *  This function will print fw activating related info
 *
 * @param control_code.
 * @param trace_func    - callback to printing function.
 * @param trace_context - trace context
 *
 * @return nothing
 *
 * HISTORY:
 *   09/08/2010:  Created. Prasenjeet Ghaywat
 *
 ****************************************************************/
void  fbe_base_enclosure_print_control_code(fbe_payload_control_operation_opcode_t control_code,
                                                        fbe_trace_func_t trace_func, 
                                                        fbe_trace_context_t trace_context)
{
     trace_func(trace_context, "control code: %s (0x%x)\n",fbe_base_enclosure_getControlCodeString(control_code), control_code);
}
// End of file fbe_base_enclosure_utils.c
