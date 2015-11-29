/***************************************************************************
 * Copyright (C) EMC Corporation 2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_eses_enclosure_usurper.c
 ***************************************************************************
 *
 * @brief
 *  This file contains usurper functions for the ESES enclosure class.
 *
 * @ingroup fbe_enclosure_class
 *
 * HISTORY
 *   10/13/2008:  Created file header. NC
 *
 ***************************************************************************/

#include "fbe_base_discovered.h"
#include "fbe_ses.h"
#include "fbe_eses_enclosure_private.h"
#include "fbe_enclosure_data_access_private.h"
#include "fbe_transport_memory.h"
#include "fbe_eses_enclosure_config.h"
#include "fbe_eses.h"
#include "fbe_enclosure.h"
#include "familyfruid.h"
#include "edal_eses_enclosure_data.h"
#include "fbe_api_sps_interface.h"
#include "fbe/fbe_event_log_api.h"
#include "PhysicalPackageMessages.h"
#include "fbe_eses_enclosure_debug.h"

/* Forward declaration */
static fbe_status_t 
eses_enclosure_getEnclosureBasicInfo(fbe_eses_enclosure_t *esesEnclosurePtr, 
                                     fbe_packet_t *packetPtr);
static fbe_status_t 
eses_enclosure_getEnclosureInfo(fbe_eses_enclosure_t *esesEnclosurePtr, 
                                     fbe_packet_t *packetPtr);
static fbe_status_t 
eses_enclosure_getEnclosureSetupInfo(fbe_eses_enclosure_t *esesEnclosurePtr,
                                     fbe_packet_t *packetPtr);
static fbe_status_t
eses_enclosure_getEnclosure_ElemGroupInfo(fbe_eses_enclosure_t *esesEnclosurePtr, 
                                     fbe_packet_t *packetPtr);

static fbe_status_t
eses_enclosure_getEnclosure_MaxElemGroupInfo(fbe_eses_enclosure_t *esesEnclosurePtr, 
                                          fbe_packet_t *packetPtr);
static fbe_status_t
eses_enclosure_getEnclosureDlInfo(fbe_eses_enclosure_t *esesEnclosurePtr, 
                                     fbe_packet_t *packetPtr);

static fbe_status_t 
eses_enclosure_getEirInfo(fbe_eses_enclosure_t *esesEnclosurePtr, 
                          fbe_packet_t *packetPtr);

static fbe_status_t 
eses_enclosure_setEnclosureControl(fbe_eses_enclosure_t *esesEnclosurePtr,
                                          fbe_packet_t *packetPtr);
static fbe_status_t 
eses_enclosure_setEnclosureLeds(fbe_eses_enclosure_t *esesEnclosurePtr,
                                          fbe_packet_t *packetPtr);
static fbe_status_t
eses_enclosure_resetEnclosureShutdownTimer(fbe_eses_enclosure_t *esesEnclosurePtr,
                                          fbe_packet_t *packetPtr);
static fbe_status_t
eses_enclosure_processEnclosureDebugControl(fbe_eses_enclosure_t *esesEnclosurePtr,
                                            fbe_packet_t *packetPtr);
static fbe_enclosure_status_t
eses_enclosure_debug_downstream_encl_removal_insertion(fbe_eses_enclosure_t *esesEnclosurePtr,
                                                          fbe_u8_t connector_id,
                                                          fbe_bool_t bypass);
static fbe_status_t 
eses_enclosure_processDriveDebugControl(fbe_eses_enclosure_t *esesEnclosurePtr,
                                        fbe_packet_t *packetPtr);
static fbe_status_t 
eses_enclosure_processDrivePowerControl(fbe_eses_enclosure_t *esesEnclosurePtr,
                                        fbe_packet_t *packetPtr);
static fbe_status_t 
eses_enclosure_processPsMarginTestControl(fbe_eses_enclosure_t *esesEnclosurePtr,
                                          fbe_packet_t *packetPtr);
static fbe_status_t 
eses_enclosure_processExpanderControl(fbe_eses_enclosure_t *esesEnclosurePtr,
                                      fbe_packet_t *packetPtr);
static fbe_status_t 
eses_enclosure_processLedStatusControl(fbe_eses_enclosure_t *esesEnclosurePtr,
                                       fbe_packet_t *packetPtr);
static fbe_status_t 
eses_enclosure_process_firmware_op(fbe_eses_enclosure_t *eses_enclosure_p, 
                                          fbe_packet_t *packet);

static fbe_status_t 
eses_enclosure_get_specific_fw_activation_in_progress(fbe_eses_enclosure_t *eses_enclosure_p,
                                             fbe_enclosure_fw_target_t fw_target,
                                             fbe_bool_t *activationInProgress);

static fbe_status_t 
fbe_eses_enclosure_repack_ctrl_op(fbe_eses_enclosure_t *eses_enclosure, 
                                           fbe_packet_t *packet,
                                           fbe_base_enclosure_control_code_t ctrl_code);

static fbe_status_t 
fbe_eses_enclosure_repack_ctrl_op_completion(fbe_packet_t * packet, 
                                           fbe_packet_completion_context_t context);

static fbe_status_t 
fbe_eses_enclosure_process_ctrl_op(fbe_eses_enclosure_t *eses_enclosure, 
                                           fbe_packet_t *packet,
                                           fbe_base_enclosure_control_code_t ctrl_code);

static fbe_status_t 
fbe_eses_enclosure_process_getPsInfo(fbe_eses_enclosure_t *eses_enclosure, 
                                     fbe_packet_t *packet,
                                     fbe_base_enclosure_control_code_t ctrl_code);


fbe_enclosure_status_t 
fbe_eses_enclosure_formatDriveLedInfo(fbe_eses_enclosure_t * eses_enclosure,
                                      fbe_eses_led_actions_t ledAction);

fbe_enclosure_status_t 
fbe_eses_enclosure_setLedControl(fbe_eses_enclosure_t *eses_enclosure,
                                 fbe_base_enclosure_led_status_control_t *ledControInfo);
static fbe_enclosure_status_t 
fbe_eses_enclosure_DriveLedControl(fbe_eses_enclosure_t *eses_enclosure,
                                   fbe_base_enclosure_led_behavior_t  ledBehavior,
                                   fbe_u32_t index);
fbe_enclosure_status_t 
fbe_eses_enclosure_DriveLedStatus(fbe_eses_enclosure_t *eses_enclosure,
                                  fbe_led_status_t *pEnclDriveFaultLeds,
                                  fbe_u32_t maxNumberOfDrives);
fbe_enclosure_status_t 
fbe_eses_enclosure_setLedStatus(fbe_eses_enclosure_t *eses_enclosure,
                                fbe_base_enclosure_led_status_control_t *ledControInfo);

static fbe_status_t 
eses_enclosure_process_fail_enclosure_request(fbe_eses_enclosure_t *eses_enclosure, fbe_packet_t *packet);

static fbe_enclosure_status_t 
fbe_eses_enclosure_get_slot_to_phy_mapping(fbe_eses_enclosure_t * eses_enclosure,
                                          fbe_enclosure_mgmt_ctrl_op_t * ctrl_op_p);

static fbe_status_t 
fbe_eses_enclosure_parse_image_header(fbe_eses_enclosure_t *eses_enclosure, 
                                      fbe_packet_t *packet);

static fbe_bool_t 
fbe_eses_enclosure_firmware_op_acceptable(fbe_eses_enclosure_t * pEsesEncl, 
                                          fbe_enclosure_mgmt_download_op_t * pFirmwareOp);

static fbe_status_t
fbe_eses_enclosure_process_firmware_download_or_activate(fbe_eses_enclosure_t *eses_enclosure_p, 
                                               fbe_enclosure_mgmt_download_op_t *download_p,
                                               fbe_sg_element_t  * sg_element_p);

static fbe_status_t 
fbe_eses_enclosure_prep_firmware_op(fbe_eses_enclosure_t * pEsesEncl, 
                                          fbe_enclosure_mgmt_download_op_t * pFirmwareOp);

static fbe_status_t 
eses_enclosure_fetch_download_op_status(fbe_eses_enclosure_t *eses_enclosure_p,
                                             fbe_enclosure_mgmt_download_op_t *mgmt_download_status_p);

static fbe_status_t 
fbe_eses_enclosure_process_firmware_abort(fbe_eses_enclosure_t * pEsesEncl, 
                                          fbe_enclosure_mgmt_download_op_t * pFirmwareOp);

static fbe_status_t
fbe_eses_enclosure_process_firmware_notify_completion(fbe_eses_enclosure_t * pEsesEncl, 
                                          fbe_enclosure_mgmt_download_op_t * pFirmwareOp);

static fbe_status_t 
fbe_eses_enclosure_get_lcc_info(fbe_eses_enclosure_t *eses_enclosure, 
                            fbe_packet_t *packet);

static fbe_status_t 
fbe_eses_enclosure_get_stand_alone_lcc_count(fbe_eses_enclosure_t *eses_enclosure, 
                            fbe_packet_t *packet);

static fbe_status_t 
fbe_eses_enclosure_get_fan_info(fbe_eses_enclosure_t *eses_enclosure, 
                                fbe_packet_t *packet);

static fbe_status_t 
fbe_eses_enclosure_get_chassis_cooling_status(fbe_eses_enclosure_t *eses_enclosure, 
                                fbe_packet_t *packet);

static fbe_bool_t 
fbe_eses_enclosure_firmware_upgrade_has_highest_priority(fbe_eses_enclosure_t * pEsesEncl,
                                                         fbe_enclosure_fw_target_t fwTarget,
                                                         fbe_u32_t sideId);

static fbe_status_t 
fbe_eses_enclosure_get_drive_slot_info(fbe_eses_enclosure_t *eses_enclosure, 
                                fbe_packet_t *packet);
static fbe_status_t 
fbe_eses_enclosure_getShutdownInfo(fbe_eses_enclosure_t *eses_enclosure, 
                                   fbe_packet_t *packet);

static fbe_status_t 
fbe_eses_enclosure_get_drive_phy_info(fbe_eses_enclosure_t * eses_enclosure, fbe_packet_t * packet);


extern const fbe_eses_enclosure_comp_type_hdlr_t * fbe_eses_enclosure_comp_type_hdlr_table[]; 

static fbe_bool_t fbe_eses_enclosure_fup_is_download_acceptable(fbe_eses_enclosure_t *pEsesEncl, 
                                                                fbe_enclosure_mgmt_download_op_t * pFirmwareOp);

static fbe_status_t 
fbe_eses_enclosure_get_connector_info(fbe_eses_enclosure_t *eses_enclosure, 
                            fbe_packet_t *packet);
static fbe_status_t fbe_eses_enclosure_get_lcc_start_slot(fbe_eses_enclosure_t *eses_enclosure, 
                                                          fbe_packet_t *packet);
static fbe_status_t fbe_eses_enclosure_get_drive_midplane_count(fbe_eses_enclosure_t *eses_enclosure, 
                                                          fbe_packet_t *packet);

static fbe_status_t 
fbe_eses_enclosure_get_overtemp_info(fbe_eses_enclosure_t *eses_enclosure, 
                                   fbe_packet_t *packet);
static fbe_status_t 
fbe_eses_enclosure_get_sps_info(fbe_eses_enclosure_t *eses_enclosure, 
                                fbe_packet_t *packet);

static fbe_status_t 
fbe_eses_enclosure_get_battery_backed_info(fbe_eses_enclosure_t * eses_enclosure, fbe_packet_t * packet);

static fbe_status_t fbe_eses_enclosure_get_ssc_count(fbe_eses_enclosure_t * eses_enclosure, fbe_packet_t * packet);

static fbe_status_t fbe_eses_enclosure_get_ssc_info(fbe_eses_enclosure_t * eses_enclosure, fbe_packet_t * packet);

/* String used to save enclosure fault reason */
static char enclFaultReasonString[500];
static fbe_status_t 
fbe_eses_enclosure_get_display_info(fbe_eses_enclosure_t *eses_enclosure, 
                            fbe_packet_t *packet);

/*!**************************************************************
 * @fn fbe_eses_enclosure_control_entry(
 *                         fbe_object_handle_t object_handle, 
 *                         fbe_packet_t * packet_p)
 ****************************************************************
 * @brief
 *  This function is the entry point for control
 *  operations to the eses enclosure object.
 *
 * @param object_handle - Handler to the enclosure object.
 * @param packet_p - The control packet that is arriving.
 *
 * @return fbe_status_t - FBE_STATUS_OK, if no error
 *
 * HISTORY:
 *   10/13/2008:  Created file header. NC
 *
 ****************************************************************/
fbe_status_t 
fbe_eses_enclosure_control_entry(fbe_object_handle_t object_handle, fbe_packet_t * packet)
{
    fbe_status_t status;
    fbe_eses_enclosure_t * eses_enclosure = NULL;
     fbe_payload_control_operation_opcode_t control_code;

    eses_enclosure = (fbe_eses_enclosure_t *)fbe_base_handle_to_pointer(object_handle);

    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                         FBE_TRACE_LEVEL_DEBUG_HIGH,
                         fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                         "%s entry \n", __FUNCTION__);

    control_code = fbe_transport_get_control_code(packet);

    switch(control_code) 
    {
        case FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_ENCLOSURE_BASIC_INFO:
            status = eses_enclosure_getEnclosureBasicInfo(eses_enclosure, packet);
            break;
        case FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_ENCLOSURE_INFO:
            status = eses_enclosure_getEnclosureInfo(eses_enclosure, packet);
            break;
        case FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_ENCLOSURE_SETUP_INFO:
            status = eses_enclosure_getEnclosureSetupInfo(eses_enclosure, packet);
            break;
        case FBE_BASE_ENCLOSURE_CONTROL_CODE_SET_ENCLOSURE_CONTROL:
            status = eses_enclosure_setEnclosureControl(eses_enclosure, packet);
            break;
        case FBE_BASE_ENCLOSURE_CONTROL_CODE_SET_ENCLOSURE_LEDS:  // used for testing only.
            status = eses_enclosure_setEnclosureLeds(eses_enclosure, packet);
            break;
        case FBE_BASE_ENCLOSURE_CONTROL_CODE_RESET_ENCLOSURE_SHUTDOWN_TIMER:
            status = eses_enclosure_resetEnclosureShutdownTimer(eses_enclosure, packet);
            break;
        case FBE_BASE_ENCLOSURE_CONTROL_CODE_ENCLOSURE_DEBUG_CONTROL:
            status = eses_enclosure_processEnclosureDebugControl(eses_enclosure, packet);
            break;
        case FBE_BASE_ENCLOSURE_CONTROL_CODE_DRIVE_DEBUG_CONTROL:
            status = eses_enclosure_processDriveDebugControl(eses_enclosure, packet);
            break;
        case FBE_BASE_ENCLOSURE_CONTROL_CODE_DRIVE_POWER_CONTROL:
            status = eses_enclosure_processDrivePowerControl(eses_enclosure, packet);
            break;
        case FBE_BASE_ENCLOSURE_CONTROL_CODE_EXPANDER_CONTROL:
            status = eses_enclosure_processExpanderControl(eses_enclosure, packet);
            break;
        case FBE_BASE_ENCLOSURE_CONTROL_CODE_PS_MARGIN_TEST_CONTROL:
            status = eses_enclosure_processPsMarginTestControl(eses_enclosure, packet);
            break;
        case FBE_BASE_ENCLOSURE_CONTROL_CODE_LED_STATUS_CONTROL:
            status = eses_enclosure_processLedStatusControl(eses_enclosure, packet);
            break;
        case FBE_BASE_ENCLOSURE_CONTROL_CODE_FIRMWARE_OP:
            status = eses_enclosure_process_firmware_op(eses_enclosure, packet);
            break;
        case FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_ELEM_GROUP_INFO:
            status = eses_enclosure_getEnclosure_ElemGroupInfo(eses_enclosure, packet);
            break;
        case FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_MAX_ELEM_GROUPS:
            status = eses_enclosure_getEnclosure_MaxElemGroupInfo(eses_enclosure, packet);
            break;
        case FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_DL_INFO:
            status = eses_enclosure_getEnclosureDlInfo(eses_enclosure, packet);
            break;
        case FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_EIR_INFO:
            status = eses_enclosure_getEirInfo(eses_enclosure, packet);
            break;
        case FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_TRACE_BUF_INFO_STATUS:
        case FBE_BASE_ENCLOSURE_CONTROL_CODE_SET_TRACE_BUF_INFO_CTRL:
        case FBE_BASE_ENCLOSURE_CONTROL_CODE_DEBUG_CMD:
        case FBE_BASE_ENCLOSURE_CONTROL_CODE_RESUME_READ:
        case FBE_BASE_ENCLOSURE_CONTROL_CODE_RESUME_WRITE:
        case FBE_BASE_ENCLOSURE_CONTROL_CODE_READ_BUF:
        case FBE_BASE_ENCLOSURE_CONTROL_CODE_EXPANDER_STRING_OUT_CONTROL:
        case FBE_BASE_ENCLOSURE_CONTROL_CODE_EXPANDER_TEST_MODE_CONTROL:
        case FBE_BASE_ENCLOSURE_CONTROL_CODE_RAW_RCV_DIAG:
        case FBE_BASE_ENCLOSURE_CONTROL_CODE_RAW_INQUIRY:
        case FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_ENCLOSURE_STATISTICS:
        case FBE_BASE_ENCLOSURE_CONTROL_CODE_EXPANDER_THRESHOLD_IN_CONTROL:
        case FBE_BASE_ENCLOSURE_CONTROL_CODE_EXPANDER_THRESHOLD_OUT_CONTROL: 
        case FBE_BASE_ENCLOSURE_CONTROL_CODE_SPS_COMMAND:
        case FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_SPS_MANUF_INFO:
            // These control codes need to create a new control operation and send down the SCSI command.
            status = fbe_eses_enclosure_repack_ctrl_op(eses_enclosure, packet, control_code);
            break;

        case FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_SLOT_TO_PHY_MAPPING:
            // These control codes do NOT need to create a new control operation and send down the SCSI command.
            status = fbe_eses_enclosure_process_ctrl_op(eses_enclosure, packet, control_code);
            break;

        case FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_PS_INFO:
            status = fbe_eses_enclosure_process_getPsInfo(eses_enclosure, packet, control_code);
            break;

        case FBE_BASE_ENCLOSURE_CONTROL_CODE_PARSE_IMAGE_HEADER:
            status = fbe_eses_enclosure_parse_image_header(eses_enclosure, packet);
            break;

        case FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_LCC_INFO:
            status = fbe_eses_enclosure_get_lcc_info(eses_enclosure, packet);
            break;

        case FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_STAND_ALONE_LCC_COUNT:
            status = fbe_eses_enclosure_get_stand_alone_lcc_count(eses_enclosure, packet);
            break;

        case FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_FAN_INFO:
            status = fbe_eses_enclosure_get_fan_info(eses_enclosure, packet);
            break;

        case FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_CHASSIS_COOLING_FAULT_STATUS:
            status = fbe_eses_enclosure_get_chassis_cooling_status(eses_enclosure, packet);
            break;

        case FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_DRIVE_SLOT_INFO:
            status = fbe_eses_enclosure_get_drive_slot_info(eses_enclosure, packet);
            break;

        case FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_SHUTDOWN_INFO:
            status = fbe_eses_enclosure_getShutdownInfo(eses_enclosure, packet);
            break;

        case FBE_DISCOVERY_TRANSPORT_CONTROL_CODE_GET_DRIVE_PHY_INFO:
            status = fbe_eses_enclosure_get_drive_phy_info(eses_enclosure, packet);
            break;

        case FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_CONNECTOR_INFO:
            status = fbe_eses_enclosure_get_connector_info(eses_enclosure, packet);
            break;
            
        case FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_LCC_START_SLOT:
            status = fbe_eses_enclosure_get_lcc_start_slot(eses_enclosure, packet);
            break;

        case FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_DISPLAY_INFO:
            status = fbe_eses_enclosure_get_display_info(eses_enclosure, packet);
            break;

        case FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_OVERTEMP_INFO:
            status = fbe_eses_enclosure_get_overtemp_info(eses_enclosure, packet);
            break;

        case FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_SPS_INFO:
            status = fbe_eses_enclosure_get_sps_info(eses_enclosure, packet);
            break;
                        
        case FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_NUMBER_OF_DRIVE_MIDPLANE:
            status = fbe_eses_enclosure_get_drive_midplane_count(eses_enclosure, packet);
            break;

        case FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_BATTERY_BACKED_INFO:
            status = fbe_eses_enclosure_get_battery_backed_info(eses_enclosure, packet);
            break;

        case FBE_BASE_ENCLOSURE_CONTROL_CODE_SET_ENCLOSURE_FAILED:
            status = eses_enclosure_process_fail_enclosure_request(eses_enclosure, packet);
            break;
            
        case FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_SSC_COUNT:
            status = fbe_eses_enclosure_get_ssc_count(eses_enclosure, packet);
            break;
            
        case FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_SSC_INFO:
            status = fbe_eses_enclosure_get_ssc_info(eses_enclosure, packet);
            break;

        default:
            status = fbe_sas_enclosure_control_entry(object_handle, packet);
            if (status != FBE_STATUS_OK)
            {
                fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                     FBE_TRACE_LEVEL_ERROR,
                                     fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                      "%s, failed to process cntrlCode 0x%x, status %d\n",
                                      __FUNCTION__, control_code, status);
                
            }
            break;
    }

   fbe_base_enclosure_record_control_opcode((fbe_base_enclosure_t *)eses_enclosure, control_code);
        
   return status;
}



/*!*************************************************************************
 *  @fn eses_enclosure_getEnclosureBasicInfo(
 *                   fbe_eses_enclosure_t *esesEnclosurePtr, 
 *                   fbe_packet_t *packetPtr)
 **************************************************************************
 *
 *  @brief
 *      This function will process the FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_ENCLOSURE_BASIC_INFO
 *      control code.
 *
 *  @param    esesEnclosurePtr - pointer to a Eses Enclosure object
 *  @param    packetPtr - pointer to an FBE control code packet
 *
 *  @return    Status from the processing of this control code.
 *
 *  NOTES:
 *
 *  HISTORY:
 *    15-Jan-2009: Joe Perry - Created
 *
 **************************************************************************/
static fbe_status_t 
eses_enclosure_getEnclosureBasicInfo(fbe_eses_enclosure_t *esesEnclosurePtr, 
                                     fbe_packet_t *packetPtr)
{
    fbe_payload_control_buffer_length_t  bufferLength = 0;
    fbe_base_object_mgmt_get_enclosure_basic_info_t   *bufferPtr;
    fbe_enclosure_status_t              enclStatus;
    fbe_enclosure_status_t              overallEnclStatus = FBE_ENCLOSURE_STATUS_OK;
    fbe_status_t                        status;
    fbe_u64_t                           sasAddress;
    fbe_u64_t                           timeOfLastGoodStatusPage;
    fbe_u32_t                           overallStateChangeCount;
    fbe_u8_t                            enclPosition, enclSide;
    fbe_lifecycle_state_t               lifecycle_state;


    fbe_base_object_customizable_trace((fbe_base_object_t*)esesEnclosurePtr,
                         FBE_TRACE_LEVEL_DEBUG_HIGH,
                         fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)esesEnclosurePtr),
                         "%s entry \n", __FUNCTION__);

   
    bufferLength = sizeof(fbe_base_object_mgmt_get_enclosure_basic_info_t);
    status = fbe_base_enclosure_get_packet_payload_control_data(packetPtr,(fbe_base_enclosure_t*)esesEnclosurePtr,TRUE,(fbe_payload_control_buffer_t *)&bufferPtr,bufferLength);
    if(status == FBE_STATUS_OK)
    {
        status = fbe_lifecycle_get_state((fbe_lifecycle_const_t*)&fbe_eses_enclosure_lifecycle_const, 
            (fbe_base_object_t*)esesEnclosurePtr, &lifecycle_state);

        if ((FBE_STATUS_OK == status) && (FBE_LIFECYCLE_STATE_FAIL != lifecycle_state))
        {
            /*
             * Retrieve the necessary data
             */
            // SAS Address (determine Local side & get Expander's SAS Address)
            enclStatus = fbe_base_enclosure_edal_getEnclosureSide((fbe_base_enclosure_t *)esesEnclosurePtr,
                                                                  &enclSide);
            if (enclStatus == FBE_ENCLOSURE_STATUS_OK)
            {
                bufferPtr->enclSideId = enclSide;
                enclStatus = fbe_base_enclosure_edal_getU64_thread_safe((fbe_base_enclosure_t *)esesEnclosurePtr,
                                                            FBE_ENCL_EXP_SAS_ADDRESS,
                                                            FBE_ENCL_EXPANDER,
                                                            enclSide,
                                                            &sasAddress);
                if (enclStatus == FBE_ENCLOSURE_STATUS_OK)
                {
                    bufferPtr->enclSasAddress = sasAddress;
                }
                else
                {
                    overallEnclStatus = enclStatus;
                }
            }
            else
            {
                overallEnclStatus = enclStatus;
            }
            // Unique ID which is the Enclosure SN
            fbe_eses_enclosure_get_serial_number(esesEnclosurePtr,
                                                FBE_SCSI_INQUIRY_ENCLOSURE_SERIAL_NUMBER_SIZE,
                                                bufferPtr->enclUniqueId);
            // Enclosure Position
            enclStatus = fbe_base_enclosure_edal_getU8_thread_safe((fbe_base_enclosure_t *)esesEnclosurePtr,
                                                       FBE_ENCL_POSITION,
                                                       FBE_ENCL_ENCLOSURE,
                                                       0,
                                                       &enclPosition);
            if (enclStatus == FBE_ENCLOSURE_STATUS_OK)
            {
                bufferPtr->enclPosition = enclPosition;
            }
            else
            {
                overallEnclStatus = enclStatus;
            }
            // State Change Count
            enclStatus = fbe_base_enclosure_edal_getOverallStateChangeCount((fbe_base_enclosure_t *)esesEnclosurePtr,
                                                                            &overallStateChangeCount);
            if (enclStatus == FBE_ENCLOSURE_STATUS_OK)
            {
                bufferPtr->enclChangeCount = overallStateChangeCount;
            }
            else
            {
                overallEnclStatus = enclStatus;
            }
            // calculate the Timestamp for last successful Status read
            enclStatus = fbe_base_enclosure_edal_getU64_thread_safe((fbe_base_enclosure_t *)esesEnclosurePtr,
                                                        FBE_ENCL_TIME_OF_LAST_GOOD_STATUS_PAGE,
                                                        FBE_ENCL_ENCLOSURE,
                                                        0,
                                                        &timeOfLastGoodStatusPage);
            if (enclStatus == FBE_ENCLOSURE_STATUS_OK)
            {
                if (timeOfLastGoodStatusPage == 0)
                {
                    bufferPtr->enclTimeSinceLastGoodStatus = 0;
                }
                else
                {
                    bufferPtr->enclTimeSinceLastGoodStatus = fbe_get_elapsed_milliseconds(timeOfLastGoodStatusPage);
                }
            }
            fbe_base_object_customizable_trace((fbe_base_object_t*)esesEnclosurePtr,
                                 FBE_TRACE_LEVEL_DEBUG_HIGH,
                                 fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)esesEnclosurePtr),
                                 "%s, enclTimeSinceLastGoodStatus %d\n",
                                 __FUNCTION__, bufferPtr->enclTimeSinceLastGoodStatus);
        
            // get enclosure fault symptom
            bufferPtr->enclFaultSymptom = fbe_base_enclosure_get_enclosure_fault_symptom(
                (fbe_base_enclosure_t *)esesEnclosurePtr);
        
            if (overallEnclStatus != FBE_ENCLOSURE_STATUS_OK)
            {
                status = FBE_STATUS_GENERIC_FAILURE;
            }
            else
            {
                status = FBE_STATUS_OK;
            }
            // Now return the number of expanders associated with this enclosure.
            bufferPtr->enclRelatedExpanders = fbe_eses_enclosure_get_number_of_related_expanders(esesEnclosurePtr);
        }
        else
        {
            // We have a FAILED enclosure object, so lets get default mgmt basic info.
            enclStatus = fbe_base_enclosure_get_mgmt_basic_info((fbe_base_enclosure_t*)esesEnclosurePtr, bufferPtr);
            status = FBE_STATUS_OK;
        }
    }
    fbe_base_enclosure_set_packet_payload_status(packetPtr, overallEnclStatus);

    fbe_transport_complete_packet(packetPtr);

    return (status);

}   // end of eses_enclosure_getEnclosureBasicInfo

/*!*************************************************************************
 *  @fn eses_enclosure_getEnclosureInfo(
 *                   fbe_eses_enclosure_t *esesEnclosurePtr, 
 *                   fbe_packet_t *packetPtr)
 **************************************************************************
 *
 *  @brief
 *      This function will process the FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_ENCLOSURE_INFO
 *      control code.
 *
 *  @param    esesEnclosurePtr - pointer to a Eses Enclosure object
 *  @param    packetPtr - pointer to an FBE control code packet
 *
 *  @return    Status from the processing of this control code.
 *
 *  NOTES:
 *
 *  HISTORY:
 *    05-Sep-2008: Joe Perry - Created
 *
 **************************************************************************/
static fbe_status_t 
eses_enclosure_getEnclosureInfo(fbe_eses_enclosure_t *esesEnclosurePtr, 
                                     fbe_packet_t *packetPtr)
{
    fbe_payload_control_buffer_length_t        bufferLength = 0;
    fbe_base_object_mgmt_get_enclosure_info_t   *bufferPtr;
    fbe_enclosure_component_block_t * encl_comp_block_p = NULL;
    fbe_edal_status_t edal_status;
    fbe_status_t status;
    fbe_enclosure_status_t encl_status = FBE_ENCLOSURE_STATUS_PACKET_FAILED;
    fbe_bool_t activeEdalValid = TRUE;


    fbe_base_object_customizable_trace((fbe_base_object_t*)esesEnclosurePtr,
                         FBE_TRACE_LEVEL_DEBUG_HIGH,
                         fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)esesEnclosurePtr),
                         "%s entry \n", __FUNCTION__);

    status = fbe_base_enclosure_get_activeEdalValid((fbe_base_enclosure_t *)esesEnclosurePtr,
                                                                         &activeEdalValid);

    if(activeEdalValid == FALSE)
    {
        /* make sure the edal is there  and get a backup copy*/
       fbe_base_enclosure_get_component_block_backup_ptr((fbe_base_enclosure_t *)esesEnclosurePtr,
                                               &encl_comp_block_p);
    }
    else
    {

        /* make sure the edal is there  and get actual copy*/
        fbe_base_enclosure_get_component_block_ptr((fbe_base_enclosure_t *)esesEnclosurePtr,
                                                   &encl_comp_block_p);
    }
    if (encl_comp_block_p == NULL)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)esesEnclosurePtr,
                             FBE_TRACE_LEVEL_INFO,
                             fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)esesEnclosurePtr),
                             "%s, EDAL not initialized\n", 
                             __FUNCTION__);
    
        fbe_transport_set_status(packetPtr, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packetPtr);
        return FBE_STATUS_GENERIC_FAILURE;
    }


    bufferLength = sizeof(fbe_base_object_mgmt_get_enclosure_info_t);
    status = fbe_base_enclosure_get_packet_payload_control_data(packetPtr,(fbe_base_enclosure_t*)esesEnclosurePtr,TRUE,(fbe_payload_control_buffer_t *)&bufferPtr,bufferLength);
    if(status == FBE_STATUS_OK)
    {
    /*
     * Copy the buffer and return the packet if enclosure is stable (no config change in progress)
     */
   
        // use spin lock when copying data to ensure we return consistent data (not partial updated)
        fbe_spinlock_lock(&esesEnclosurePtr->enclGetInfoLock);
        edal_status = fbe_edal_copyBlockData(encl_comp_block_p, 
                                             (fbe_u8_t *)bufferPtr,
                                             sizeof(fbe_base_object_mgmt_get_enclosure_info_t));
        fbe_spinlock_unlock(&esesEnclosurePtr->enclGetInfoLock);
        if (edal_status==FBE_EDAL_STATUS_OK)
        {
            encl_status = FBE_ENCLOSURE_STATUS_OK;
            status = FBE_STATUS_OK;
        }
        else
        {
            encl_status = FBE_ENCLOSURE_STATUS_PACKET_FAILED;
            status = FBE_STATUS_GENERIC_FAILURE;
        }
    }
   
    fbe_base_enclosure_set_packet_payload_status(packetPtr, encl_status);

    fbe_transport_complete_packet(packetPtr);

    return (status);

}   // end of eses_enclosure_getEnclosureInfo

/*!*************************************************************************
 *  @fn fbe_eses_enclosure_get_ssc_count(
 *                   fbe_eses_enclosure_t *esesEnclosurePtr, 
 *                   fbe_packet_t *packetPtr)
 **************************************************************************
 *
 *  @brief
 *  Get the count for SSC module.
 *
 *  @param    esesEnclosurePtr - pointer to a Eses Enclosure object
 *  @param    packetPtr - pointer to an FBE control code packet
 *
 *  @return    Status from the processing of this control code.
 *  HISTORY:
 *    18-Dec-2013: GB - Created
 **************************************************************************/
static fbe_status_t 
fbe_eses_enclosure_get_ssc_count(fbe_eses_enclosure_t *esesEnclosurePtr, 
                                 fbe_packet_t *packetPtr)
{

    fbe_u8_t                            *pGetSscCount;
    fbe_status_t                        status;    
    fbe_payload_control_buffer_length_t bufferLength = 0;
    fbe_enclosure_status_t              encl_stat = FBE_ENCLOSURE_STATUS_PACKET_FAILED;
        

    fbe_base_object_customizable_trace((fbe_base_object_t*)esesEnclosurePtr,
                         FBE_TRACE_LEVEL_DEBUG_HIGH,
                         fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)esesEnclosurePtr),
                         "%s entry \n", __FUNCTION__);

    bufferLength = sizeof(fbe_u8_t);
    fbe_base_object_customizable_trace((fbe_base_object_t*)esesEnclosurePtr,
                         FBE_TRACE_LEVEL_INFO,
                         fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)esesEnclosurePtr),
                         "%s entry buflen:%d\n", __FUNCTION__, bufferLength);

    status = fbe_base_enclosure_get_packet_payload_control_data(packetPtr,
                                                                (fbe_base_enclosure_t*)esesEnclosurePtr,
                                                                TRUE,
                                                                (fbe_payload_control_buffer_t *)&pGetSscCount,
                                                                bufferLength);
    if(status == FBE_STATUS_OK)
    {
        // copy the data
        *pGetSscCount = fbe_eses_enclosure_get_number_of_ssc(esesEnclosurePtr);
        encl_stat = FBE_ENCLOSURE_STATUS_OK;       
        fbe_base_object_customizable_trace((fbe_base_object_t*)esesEnclosurePtr,
                         FBE_TRACE_LEVEL_INFO,
                         fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)esesEnclosurePtr),
                         "%s SSC count:%d\n", __FUNCTION__, *pGetSscCount);
    }
    else
    {
    fbe_base_object_customizable_trace((fbe_base_object_t*)esesEnclosurePtr,
                         FBE_TRACE_LEVEL_INFO,
                         fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)esesEnclosurePtr),
                         "%s status:0x%x\n", __FUNCTION__, status);
    }
    fbe_base_enclosure_set_packet_payload_status(packetPtr, encl_stat);
    fbe_transport_complete_packet(packetPtr);

    return (status);
} //fbe_eses_enclosure_get_ssc_count

/*!*************************************************************************
 *  @fn fbe_eses_enclosure_get_ssc_info(
 *                   fbe_eses_enclosure_t *esesEnclosurePtr, 
 *                   fbe_packet_t *packetPtr)
 **************************************************************************
 *
 *  @brief
 *  Get the SSC info and copy it to the buffer passed in.
 *
 *  @param    esesEnclosurePtr - pointer to a Eses Enclosure object
 *  @param    packetPtr - pointer to an FBE control code packet
 *
 *  @return    Status from the processing of this control code.
 *
 *  HISTORY:
 *    18-Dec-2013: GB - Created
 **************************************************************************/
static fbe_status_t
fbe_eses_enclosure_get_ssc_info(fbe_eses_enclosure_t *esesEnclosurePtr, 
                                     fbe_packet_t *packetPtr)
{
    fbe_payload_control_buffer_length_t bufferLength = 0;
    fbe_ssc_info_t                      *pSscInfo = NULL;
    fbe_status_t                        status;
    fbe_enclosure_status_t              encl_status = FBE_ENCLOSURE_STATUS_PACKET_FAILED;
        
    fbe_base_object_customizable_trace((fbe_base_object_t*)esesEnclosurePtr,
                         FBE_TRACE_LEVEL_DEBUG_HIGH,
                         fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)esesEnclosurePtr),
                         "%s entry \n", __FUNCTION__);


    bufferLength = (sizeof(fbe_ssc_info_t));
    fbe_base_object_customizable_trace((fbe_base_object_t*)esesEnclosurePtr,
                         FBE_TRACE_LEVEL_INFO,
                         fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)esesEnclosurePtr),
                         "  %s buflen:%d \n", 
                         __FUNCTION__, 
                         bufferLength);

    status = fbe_base_enclosure_get_packet_payload_control_data(packetPtr,
                                                                (fbe_base_enclosure_t*)esesEnclosurePtr,
                                                                TRUE,
                                                                (fbe_payload_control_buffer_t *)&pSscInfo,
                                                                bufferLength);
    if(status == FBE_STATUS_OK)
    {
        // Get the SSC info from EDLA, put it in pSscInfo
        encl_status = fbe_base_enclosure_edal_getBool((fbe_base_enclosure_t *)esesEnclosurePtr,
                                                       FBE_ENCL_COMP_INSERTED,      // attribute
                                                       FBE_ENCL_SSC,           // component
                                                       0,                           // component index 0
                                                       &pSscInfo->inserted);  // copy the data here

        fbe_base_object_customizable_trace((fbe_base_object_t*)esesEnclosurePtr,
                         FBE_TRACE_LEVEL_INFO,
                         fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)esesEnclosurePtr),
                         "  %s edalsts:0x%x insert:%d\n", 
                         __FUNCTION__, 
                         encl_status,
                         pSscInfo->inserted);
        encl_status = fbe_base_enclosure_edal_getBool((fbe_base_enclosure_t *)esesEnclosurePtr,
                                                       FBE_ENCL_COMP_FAULTED,
                                                       FBE_ENCL_SSC,
                                                       0,
                                                       &pSscInfo->faulted);

        fbe_base_object_customizable_trace((fbe_base_object_t*)esesEnclosurePtr,
                         FBE_TRACE_LEVEL_INFO,
                         fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)esesEnclosurePtr),
                         "  %s edalsts:0x%x fault:%d\n", 
                         __FUNCTION__, 
                         encl_status,
                         pSscInfo->faulted);
        encl_status = FBE_ENCLOSURE_STATUS_OK;       
    }
    else
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)esesEnclosurePtr,
                         FBE_TRACE_LEVEL_INFO,
                         fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)esesEnclosurePtr),
                         "  %s sts:0x%x \n", 
                         __FUNCTION__, 
                         status);
    }
    fbe_base_enclosure_set_packet_payload_status(packetPtr, encl_status);
    fbe_transport_complete_packet(packetPtr);

    return (status);
} // fbe_eses_enclosure_get_ssc_info

/*!*************************************************************************
 *  @fn eses_enclosure_getEnclosure_ElemGroupInfo(
 *                   fbe_eses_enclosure_t *esesEnclosurePtr, 
 *                   fbe_packet_t *packetPtr)
 **************************************************************************
 *
 *  @brief
 *      This function will process the FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_ELEM_GROUP_INFO
 *      control code.
 *
 *  @param    esesEnclosurePtr - pointer to a Eses Enclosure object
 *  @param    packetPtr - pointer to an FBE control code packet
 *
 *  @return    Status from the processing of this control code.
 *
 *  NOTES:
 *
 *  HISTORY:
 *    29-Jan-2009: Dipak Patel - Created
 *
 **************************************************************************/
static fbe_status_t
eses_enclosure_getEnclosure_ElemGroupInfo(fbe_eses_enclosure_t *esesEnclosurePtr, 
                                     fbe_packet_t *packetPtr)
{
    fbe_payload_control_buffer_length_t        bufferLength = 0;
    fbe_eses_elem_group_t *bufferPtr = NULL;
    fbe_eses_elem_group_t *elem_group = NULL;   
    fbe_status_t status;    
    fbe_enclosure_status_t encl_stat = FBE_ENCLOSURE_STATUS_PACKET_FAILED;
        
    fbe_base_object_customizable_trace((fbe_base_object_t*)esesEnclosurePtr,
                         FBE_TRACE_LEVEL_DEBUG_HIGH,
                         fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)esesEnclosurePtr),
                         "%s entry, \n", __FUNCTION__);

    elem_group = fbe_eses_enclosure_get_elem_group_ptr((fbe_eses_enclosure_t *)esesEnclosurePtr);   
    if (elem_group == NULL)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)esesEnclosurePtr,
                             FBE_TRACE_LEVEL_INFO,
                             fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)esesEnclosurePtr),
                             "%s, ElemGroup is not initialized\n", 
                              __FUNCTION__);
    
        fbe_transport_set_status(packetPtr, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packetPtr);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    bufferLength = (sizeof(fbe_eses_elem_group_t) * (fbe_eses_enclosure_get_number_of_actual_elem_groups((fbe_eses_enclosure_t *)esesEnclosurePtr)));
    status = fbe_base_enclosure_get_packet_payload_control_data(packetPtr,(fbe_base_enclosure_t*)esesEnclosurePtr,TRUE,(fbe_payload_control_buffer_t *)&bufferPtr,bufferLength);
    if(status == FBE_STATUS_OK)
    {
    /* Identify requierd element group and copy element group data one by one */
    memcpy(bufferPtr,elem_group, sizeof(fbe_eses_elem_group_t) * (fbe_eses_enclosure_get_number_of_actual_elem_groups((fbe_eses_enclosure_t *)esesEnclosurePtr))); 
    encl_stat = FBE_ENCLOSURE_STATUS_OK;       
    }
    fbe_base_enclosure_set_packet_payload_status(packetPtr, encl_stat);
    fbe_transport_complete_packet(packetPtr);

    return (status);

}

/*!*************************************************************************
 *  @fn eses_enclosure_getEnclosure_MaxElemGroupInfo(
 *                   fbe_eses_enclosure_t *esesEnclosurePtr, 
 *                   fbe_packet_t *packetPtr)
 **************************************************************************
 *
 *  @brief
 *      This function will process the FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_MAX_ELEM_GROUPS
 *      control code.
 *
 *  @param    esesEnclosurePtr - pointer to a Eses Enclosure object
 *  @param    packetPtr - pointer to an FBE control code packet
 *
 *  @return    Status from the processing of this control code.
 *
 *  NOTES:
 *
 *  HISTORY:
 *    29-Jan-2009: Dipak Patel - Created
 *
 **************************************************************************/
static fbe_status_t
eses_enclosure_getEnclosure_MaxElemGroupInfo(fbe_eses_enclosure_t *esesEnclosurePtr, 
                                     fbe_packet_t *packetPtr)
{

    fbe_payload_control_buffer_length_t        bufferLength = 0;   
    fbe_enclosure_mgmt_max_elem_group   *bufferPtr; 
    fbe_status_t status;
    fbe_enclosure_status_t encl_stat = FBE_ENCLOSURE_STATUS_PACKET_FAILED;
        
    fbe_base_object_customizable_trace((fbe_base_object_t*)esesEnclosurePtr,
                         FBE_TRACE_LEVEL_DEBUG_HIGH,
                         fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)esesEnclosurePtr),
                         "%s entry \n", __FUNCTION__);


    bufferLength = sizeof(fbe_enclosure_mgmt_max_elem_group);
    status = fbe_base_enclosure_get_packet_payload_control_data(packetPtr,(fbe_base_enclosure_t*)esesEnclosurePtr,TRUE,(fbe_payload_control_buffer_t *)&bufferPtr,bufferLength);
    if(status == FBE_STATUS_OK)
    {

    /*
      * get the max no of element groups.
      */

    bufferPtr->max_elem_group = fbe_eses_enclosure_get_number_of_actual_elem_groups((fbe_eses_enclosure_t *)esesEnclosurePtr);

    encl_stat = FBE_ENCLOSURE_STATUS_OK;       
    } 
    fbe_base_enclosure_set_packet_payload_status(packetPtr, encl_stat);
    fbe_transport_complete_packet(packetPtr);

    return (status);

}
/*!*************************************************************************
 *  @fn eses_enclosure_getEnclosureDlInfo(
 *                   fbe_eses_enclosure_t *esesEnclosurePtr, 
 *                   fbe_packet_t *packetPtr)
 **************************************************************************
 *
 *  @brief
 *  Get a buffer provided from the payload and copy the 
 *  fbe_eses_encl_fup_info_t into it.
 *
 *  @param    esesEnclosurePtr - pointer to a Eses Enclosure object
 *  @param    packetPtr - pointer to an FBE control code packet
 *
 *  @return    fbe_status_t
 *
 *  HISTORY:
 *    17-Mar-2009: GB - Created
 **************************************************************************/
static fbe_status_t
eses_enclosure_getEnclosureDlInfo(fbe_eses_enclosure_t *esesEnclosurePtr, 
                                     fbe_packet_t *packetPtr)
{
    fbe_status_t                        status;    
    fbe_payload_control_buffer_length_t bufferLength = 0;
    fbe_eses_encl_fup_info_t            *destPtr = NULL;
    fbe_eses_encl_fup_info_t            *fup_info = NULL;   
    fbe_enclosure_status_t encl_stat = FBE_ENCLOSURE_STATUS_PACKET_FAILED;
        
    // trace the enclosure number
    fbe_base_object_customizable_trace((fbe_base_object_t*)esesEnclosurePtr,
                         FBE_TRACE_LEVEL_DEBUG_HIGH,
                         fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)esesEnclosurePtr),
                         "%s entry \n", __FUNCTION__);

    // set the source pointer
    fup_info = &esesEnclosurePtr->enclCurrentFupInfo;


    bufferLength = sizeof(fbe_eses_encl_fup_info_t);
    status = fbe_base_enclosure_get_packet_payload_control_data(packetPtr,(fbe_base_enclosure_t*)esesEnclosurePtr,TRUE,(fbe_payload_control_buffer_t *)&destPtr,bufferLength);
    if(status == FBE_STATUS_OK)
    {
    
    // copy the data
    *destPtr = *fup_info;
    encl_stat = FBE_ENCLOSURE_STATUS_OK;       
    }
    fbe_base_enclosure_set_packet_payload_status(packetPtr, encl_stat);
    fbe_transport_complete_packet(packetPtr);

    return (status);

}

/*!*************************************************************************
 *  @fn eses_enclosure_getEirInfo(
 *                   fbe_eses_enclosure_t *esesEnclosurePtr, 
 *                   fbe_packet_t *packetPtr)
 **************************************************************************
 *
 *  @brief
 *  Get a buffer provided from the payload and copy the 
 *  Enclosure EIR into it.
 *
 *  @param    esesEnclosurePtr - pointer to a Eses Enclosure object
 *  @param    packetPtr - pointer to an FBE control code packet
 *
 *  @return    fbe_status_t
 *
 *  HISTORY:
 *    27-Dec-2010:  Joe Perry - Created
 **************************************************************************/
static fbe_status_t
eses_enclosure_getEirInfo(fbe_eses_enclosure_t *esesEnclosurePtr, 
                          fbe_packet_t *packetPtr)
{
    fbe_status_t                        status;    
    fbe_payload_control_buffer_length_t bufferLength = 0;
    fbe_eses_encl_eir_info_t            *bufferPtr = NULL;
    fbe_edal_status_t                   edalStatus = FBE_EDAL_STATUS_OK;
    fbe_enclosure_status_t              encl_status = FBE_ENCLOSURE_STATUS_PACKET_FAILED;
    fbe_enclosure_status_t              encl_status1 = FBE_ENCLOSURE_STATUS_PACKET_FAILED;
    fbe_u32_t                           psIndex;
    fbe_u32_t                           psCount;
    fbe_u8_t                            inputPowerStatus;
    fbe_u16_t                           inputPower;
    fbe_bool_t                          airInletTempValid;
    fbe_u16_t                           airInletTemp;
    fbe_u32_t                           tsIndex;
        
    // trace the enclosure number
    fbe_base_object_customizable_trace((fbe_base_object_t*)esesEnclosurePtr,
                         FBE_TRACE_LEVEL_DEBUG_HIGH,
                         fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)esesEnclosurePtr),
                         "%s entry \n", __FUNCTION__);

    bufferLength = sizeof(fbe_eses_encl_eir_info_t);
    status = fbe_base_enclosure_get_packet_payload_control_data(packetPtr,
                                                                (fbe_base_enclosure_t*)esesEnclosurePtr,
                                                                TRUE,
                                                                (fbe_payload_control_buffer_t *)&bufferPtr,
                                                                bufferLength);
    if(status == FBE_STATUS_OK)
    {
        /*
         * Get InputPower info from PS's
         */
        edalStatus = fbe_edal_getSpecificComponentCount(((fbe_base_enclosure_t *)esesEnclosurePtr)->enclosureComponentTypeBlock,
                                                        FBE_ENCL_POWER_SUPPLY,
                                                        &psCount);
        if ((edalStatus != FBE_EDAL_STATUS_OK) || (psCount == 0))
        {
            fbe_base_object_customizable_trace((fbe_base_object_t *) esesEnclosurePtr, 
                                   FBE_TRACE_LEVEL_ERROR,
                                   fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *) esesEnclosurePtr),
                                   "%s, No power supplies in enclosure number:%d\n", 
                                   __FUNCTION__, esesEnclosurePtr->properties.number_of_encl);
            fbe_transport_set_status(packetPtr, FBE_STATUS_GENERIC_FAILURE, 0);
            fbe_transport_complete_packet(packetPtr);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        
        bufferPtr->enclEirData.enclCurrentInputPower.inputPower = 0;
        bufferPtr->enclEirData.enclCurrentInputPower.status = 0;
        for (psIndex = 0; psIndex < psCount; psIndex++)
        {
            encl_status = fbe_base_enclosure_edal_getU8((fbe_base_enclosure_t *)esesEnclosurePtr,
                                                         FBE_ENCL_PS_INPUT_POWER_STATUS,
                                                         FBE_ENCL_POWER_SUPPLY,
                                                         psIndex,
                                                         &inputPowerStatus); 
            encl_status1 = fbe_base_enclosure_edal_getU16((fbe_base_enclosure_t *)esesEnclosurePtr,
                                                          FBE_ENCL_PS_INPUT_POWER,
                                                          FBE_ENCL_POWER_SUPPLY,
                                                          psIndex,
                                                          &inputPower); 
            if((!ENCL_STAT_OK(encl_status)) || (!ENCL_STAT_OK(encl_status1)))
            {
                fbe_base_object_customizable_trace((fbe_base_object_t *) esesEnclosurePtr, 
                                       FBE_TRACE_LEVEL_ERROR,
                                       fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *) esesEnclosurePtr),
                                       "%s, error getting InputPower (%d) & Status (%d)\n", 
                                       __FUNCTION__, encl_status, encl_status1);
                bufferPtr->enclEirData.enclCurrentInputPower.status = FBE_ENERGY_STAT_FAILED;
                fbe_transport_set_status(packetPtr, FBE_STATUS_GENERIC_FAILURE, 0);
                fbe_transport_complete_packet(packetPtr);
                return FBE_STATUS_GENERIC_FAILURE;
            }

            bufferPtr->enclEirData.enclCurrentInputPower.inputPower += inputPower;
            bufferPtr->enclEirData.enclCurrentInputPower.status |= inputPowerStatus;
        }
        fbe_base_object_customizable_trace((fbe_base_object_t*)esesEnclosurePtr,
                                           FBE_TRACE_LEVEL_DEBUG_LOW,
                                           fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)esesEnclosurePtr),
                                           "%s, inputPower %d, status 0x%x\n", 
                                           __FUNCTION__,
                                           bufferPtr->enclEirData.enclCurrentInputPower.inputPower,
                                           bufferPtr->enclEirData.enclCurrentInputPower.status);

        // AirInletTemp in different locations for 6G & 12G enclosures
        switch (fbe_sas_enclosure_get_enclosure_type((fbe_sas_enclosure_t *)esesEnclosurePtr))
        {
        case FBE_SAS_ENCLOSURE_TYPE_VIPER:
        case FBE_SAS_ENCLOSURE_TYPE_DERRINGER:
        case FBE_SAS_ENCLOSURE_TYPE_VIKING_IOSXP:
        case FBE_SAS_ENCLOSURE_TYPE_VOYAGER_ICM:
        case FBE_SAS_ENCLOSURE_TYPE_PINECONE:
            tsIndex = FBE_EDAL_STATUS_CHASSIS_INDIVIDUAL;
            break;
        case FBE_SAS_ENCLOSURE_TYPE_ANCHO:
        case FBE_SAS_ENCLOSURE_TYPE_TABASCO:
        case FBE_SAS_ENCLOSURE_TYPE_CAYENNE_IOSXP:
        case FBE_SAS_ENCLOSURE_TYPE_NAGA_IOSXP:
            tsIndex = FBE_EDAL_STATUS_12G_CHASSIS_OVERALL;
            break;
        default:
            fbe_base_object_customizable_trace((fbe_base_object_t *) esesEnclosurePtr, 
                                               FBE_TRACE_LEVEL_ERROR,
                                               fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *) esesEnclosurePtr),
                                               "%s, Unknown TempSensor index, enclType %d\n", 
                                               __FUNCTION__, 
                                               fbe_sas_enclosure_get_enclosure_type((fbe_sas_enclosure_t *)esesEnclosurePtr));

            fbe_transport_set_status(packetPtr, FBE_STATUS_GENERIC_FAILURE, 0);
            fbe_transport_complete_packet(packetPtr);
            return FBE_STATUS_GENERIC_FAILURE;
            break;
        }

        /*
         * Get AirInletTemp info from Temp Sensor
         */
        
        encl_status = 
            fbe_base_enclosure_edal_getTempSensorBool_thread_safe((fbe_base_enclosure_t *)esesEnclosurePtr,
                                                                  FBE_ENCL_TEMP_SENSOR_TEMPERATURE_VALID,
                                                                  tsIndex,
                                                                  &airInletTempValid); 
        encl_status1 = 
            fbe_base_enclosure_edal_getTempSensorU16_thread_safe((fbe_base_enclosure_t *)esesEnclosurePtr,
                                                                 FBE_ENCL_TEMP_SENSOR_TEMPERATURE,
                                                                 tsIndex,
                                                                 &airInletTemp); 
        fbe_base_object_customizable_trace((fbe_base_object_t *) esesEnclosurePtr, 
                                           FBE_TRACE_LEVEL_DEBUG_HIGH,
                                           fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *) esesEnclosurePtr),
                                           "%s (index %d), valid %d (%d), temp %d (%d)\n", 
                                           __FUNCTION__, 
                                           tsIndex,
                                           airInletTempValid, encl_status, 
                                           airInletTemp, encl_status1);
        if((!ENCL_STAT_OK(encl_status)) || (!ENCL_STAT_OK(encl_status1)))
        {
            fbe_base_object_customizable_trace((fbe_base_object_t *) esesEnclosurePtr, 
                                               FBE_TRACE_LEVEL_ERROR,
                                               fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *) esesEnclosurePtr),
                                               "%s, error getting AirInletTemp (%d) & Valid (%d)\n", 
                                               __FUNCTION__, encl_status, encl_status1);
            bufferPtr->enclEirData.enclCurrentInputPower.status = FBE_ENERGY_STAT_FAILED;
            fbe_transport_set_status(packetPtr, FBE_STATUS_GENERIC_FAILURE, 0);
            fbe_transport_complete_packet(packetPtr);
            return FBE_STATUS_GENERIC_FAILURE;
       }

        if (airInletTempValid)
        {
            bufferPtr->enclEirData.enclCurrentAirInletTemp.airInletTemp = airInletTemp;
            bufferPtr->enclEirData.enclCurrentAirInletTemp.status = FBE_ENERGY_STAT_VALID;
        }
        else
        {
            bufferPtr->enclEirData.enclCurrentAirInletTemp.airInletTemp = 0;
            bufferPtr->enclEirData.enclCurrentAirInletTemp.status = FBE_ENERGY_STAT_UNSUPPORTED;
        }
        fbe_base_object_customizable_trace((fbe_base_object_t*)esesEnclosurePtr,
                                           FBE_TRACE_LEVEL_DEBUG_LOW,
                                           fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)esesEnclosurePtr),
                                           "%s, airInletTemp %d, AirInletTempstatus 0x%x\n", 
                                           __FUNCTION__,
                                           bufferPtr->enclEirData.enclCurrentAirInletTemp.airInletTemp,
                                           bufferPtr->enclEirData.enclCurrentAirInletTemp.status);

        encl_status = FBE_ENCLOSURE_STATUS_OK;    
           
    }
    fbe_base_enclosure_set_packet_payload_status(packetPtr, encl_status);
    fbe_transport_complete_packet(packetPtr);

    return (status);

}   // end of eses_enclosure_getEirInfo

/*!*************************************************************************
 *  @fn eses_enclosure_getEnclosureSetupInfo(
 *                    fbe_eses_enclosure_t *esesEnclosurePtr,
 *                    fbe_packet_t *packetPtr)
 **************************************************************************
 *
 *  @brief
 *      This function will process the FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_ENCLOSURE_SETUP_INFO
 *      control code.
 *
 *  @param    esesEnclosurePtr - pointer to a Eses Enclosure object
 *  @param    packetPtr - pointer to an FBE control code packet
 *
 *  @return    Status from the processing of this control code.
 *
 *  NOTES:
 *
 *  HISTORY:
 *    08-Oct-2008: Joe Perry - Created
 *
 **************************************************************************/
static fbe_status_t 
eses_enclosure_getEnclosureSetupInfo(fbe_eses_enclosure_t *esesEnclosurePtr,
                                          fbe_packet_t *packetPtr)
{
    fbe_payload_control_buffer_length_t            bufferLength = 0;
    fbe_base_object_mgmt_get_enclosure_setup_info_t *bufferPtr;
    fbe_status_t   status;
    fbe_enclosure_status_t encl_stat = FBE_ENCLOSURE_STATUS_PACKET_FAILED;

    fbe_base_object_customizable_trace((fbe_base_object_t*)esesEnclosurePtr,
                         FBE_TRACE_LEVEL_DEBUG_HIGH,
                         fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)esesEnclosurePtr),
                         "%s entry\n", __FUNCTION__);


    bufferLength = sizeof(fbe_base_object_mgmt_get_enclosure_setup_info_t);
    status = fbe_base_enclosure_get_packet_payload_control_data(packetPtr,(fbe_base_enclosure_t*)esesEnclosurePtr,TRUE,(fbe_payload_control_buffer_t *)&bufferPtr,bufferLength);
    if(status == FBE_STATUS_OK)
    {
    /* Fill in the buffer and return the packet */
    memset(bufferPtr, 
           0,
           sizeof(fbe_base_object_mgmt_get_enclosure_setup_info_t));
    bufferPtr->enclosureBufferSize = VIPER_ENCL_TOTAL_DATA_SIZE;
    encl_stat = FBE_ENCLOSURE_STATUS_OK;       
    }
    fbe_base_enclosure_set_packet_payload_status(packetPtr, encl_stat);
    fbe_transport_complete_packet(packetPtr);

    return status;

}   // end of eses_enclosure_getEnclosureSetupInfo


/*!*************************************************************************
 * @fn eses_enclosure_setEnclosureControl
 *                    fbe_eses_enclosure_t *esesEnclosurePtr,
 *                    fbe_packet_t *packetPtr)
 **************************************************************************
 *
 *  @brief
 *      This function will process the FBE_BASE_ENCLOSURE_CONTROL_CODE_SET_ENCLOSURE_CONTROL
 *      control code.
 *
 *  @param    esesEnclosurePtr - pointer to a Eses Enclosure object
 *  @param    packetPtr - pointer to an FBE control code packet
 *
 *  @return    Status from the processing of this control code.
 *
 *  NOTES:
 *
 *  HISTORY:
 *    08-Oct-2008: Joe Perry - Created
 *
 **************************************************************************/
static fbe_status_t 
eses_enclosure_setEnclosureControl(fbe_eses_enclosure_t *esesEnclosurePtr,
                                        fbe_packet_t *packetPtr)
{
    fbe_payload_control_buffer_length_t             bufferLength = 0;
    fbe_base_object_mgmt_set_enclosure_control_t    *bufferPtr;
    fbe_status_t                                    status;
    fbe_edal_status_t                               edalStatus;
    fbe_bool_t                                      writeDataAvailable = FALSE;
    fbe_enclosure_status_t encl_stat = FBE_ENCLOSURE_STATUS_PACKET_FAILED;
    fbe_u32_t                                       active_edal_genCount = 0;
    fbe_u32_t                                       UserReq_edal_genCount = 0;


    fbe_base_object_customizable_trace((fbe_base_object_t*)esesEnclosurePtr,
                        FBE_TRACE_LEVEL_DEBUG_HIGH,
                        fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)esesEnclosurePtr),
                        "%s entry\n", __FUNCTION__);

    bufferLength = sizeof(fbe_base_object_mgmt_set_enclosure_control_t);
    status = fbe_base_enclosure_get_packet_payload_control_data(packetPtr,(fbe_base_enclosure_t*)esesEnclosurePtr,TRUE,(fbe_payload_control_buffer_t *)&bufferPtr,bufferLength);
    if(status == FBE_STATUS_OK)
    {
        /* As we have eses_enclosure to get active EDAL generation count, we need to call base enclosure function
            but incase of user requested EDAL we have EDAL pointer itself inform of bufferPtr. So call EDAL dunction
            directly to get generation count */
            
        encl_stat = fbe_base_enclosure_edal_get_generationCount((fbe_base_enclosure_t*)esesEnclosurePtr, &active_edal_genCount);
        edalStatus = fbe_edal_getGenerationCount(bufferPtr, &UserReq_edal_genCount);

            if (encl_stat == FBE_ENCLOSURE_STATUS_OK && edalStatus == FBE_EDAL_STATUS_OK)
            {
                 //encl_stat = fbe_base_enclosure_translate_edal_status(edalStatus);
                 if((active_edal_genCount == UserReq_edal_genCount) && 
                     (esesEnclosurePtr->enclosureConfigBeingUpdated == FALSE))
                 {
                     /*
                       * Process the buffer and return the packet
                       */
                    edalStatus = fbe_edal_checkForWriteData(bufferPtr, &writeDataAvailable);
                    if (edalStatus == FBE_EDAL_STATUS_OK)
                    {
                        encl_stat = FBE_ENCLOSURE_STATUS_OK;       
                        if (writeDataAvailable)
                        {
                            fbe_eses_enclosure_saveControlPageInfo(esesEnclosurePtr, bufferPtr);
                            /*
                            * Set condition to write out ESES Control
                            */
                            status = fbe_lifecycle_set_cond(&fbe_eses_enclosure_lifecycle_const, 
                                                            (fbe_base_object_t*)esesEnclosurePtr, 
                                                            FBE_ESES_ENCLOSURE_LIFECYCLE_COND_EXPANDER_CONTROL_NEEDED);
                            if (status != FBE_STATUS_OK) 
                            {
                                fbe_base_object_customizable_trace((fbe_base_object_t*)esesEnclosurePtr,
                                                    FBE_TRACE_LEVEL_ERROR,
                                                    fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)esesEnclosurePtr),
                                                    "%s,Can't set ControlNeeded condition, status: 0x%x.\n",
                                                    __FUNCTION__, status);
                                
                            }
                        }
                    }
                    else
                    {
                        status = FBE_STATUS_FAILED;
                    } 
                 }
                 else
                 {
                     encl_stat = FBE_ENCLOSURE_STATUS_BUSY;
                     status = FBE_STATUS_BUSY;

                     fbe_base_object_customizable_trace((fbe_base_object_t*)esesEnclosurePtr,
                                                    FBE_TRACE_LEVEL_ERROR,
                                                    fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)esesEnclosurePtr),
                                                    "%s,User_Req and ActiveEDAL GenCounts Mismatch, Active EDAL GenCount: %d, User_Req EDAL GenCount: %d, enclosureConfigBeingUpdated: %d.\n",
                                                    __FUNCTION__, active_edal_genCount, UserReq_edal_genCount, esesEnclosurePtr->enclosureConfigBeingUpdated );
                 }
            }
            else
            {
                status = FBE_STATUS_FAILED;
                if(encl_stat == FBE_ENCLOSURE_STATUS_OK)
                {
                    encl_stat = fbe_base_enclosure_translate_edal_status(edalStatus);
                    fbe_base_object_customizable_trace((fbe_base_object_t*)esesEnclosurePtr,
                                                    FBE_TRACE_LEVEL_ERROR,
                                                    fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)esesEnclosurePtr),
                                                    "%s,Can't get generation count for user requested EDAL, edal_status: 0x%x.\n",
                                                    __FUNCTION__, edalStatus);
                }
                else
                {
                    fbe_base_object_customizable_trace((fbe_base_object_t*)esesEnclosurePtr,
                                                    FBE_TRACE_LEVEL_ERROR,
                                                    fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)esesEnclosurePtr),
                                                    "%s,Can't get generation count for active EDAL, encl_status: 0x%x.\n",
                                                    __FUNCTION__, encl_stat);
                }
            }       
   }
    fbe_base_enclosure_set_packet_payload_status(packetPtr, encl_stat);
    fbe_transport_complete_packet(packetPtr);

    return status;

}   // end of eses_enclosure_setEnclosureControl


/*!*************************************************************************
 * @fn eses_enclosure_setEnclosureLeds
 *                    fbe_eses_enclosure_t *esesEnclosurePtr,
 *                    fbe_packet_t *packetPtr)
 **************************************************************************
 *
 *  @brief
 *      This function will process the FBE_BASE_ENCLOSURE_CONTROL_CODE_SET_ENCLOSURE_LEDS
 *      control code.
 *
 *  @param    esesEnclosurePtr - pointer to a Eses Enclosure object
 *  @param    packetPtr - pointer to an FBE control code packet
 *
 *  @return    Status from the processing of this control code.
 *
 *  NOTES:
 *
 *  HISTORY:
 *    08-Oct-2008: Joe Perry - Created
 *
 **************************************************************************/
static fbe_status_t 
eses_enclosure_setEnclosureLeds(fbe_eses_enclosure_t *esesEnclosurePtr,
                                fbe_packet_t *packetPtr)
{
    fbe_payload_control_buffer_length_t         bufferLength = 0;
    fbe_base_object_mgmt_set_enclosure_led_t    *bufferPtr;
    fbe_status_t                                status = FBE_STATUS_OK;
    fbe_eses_led_actions_t                      ledAction;
    fbe_enclosure_status_t encl_stat = FBE_ENCLOSURE_STATUS_PACKET_FAILED;

    fbe_base_object_customizable_trace((fbe_base_object_t*)esesEnclosurePtr,
                        FBE_TRACE_LEVEL_DEBUG_HIGH,
                        fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)esesEnclosurePtr),
                        "%s entry\n", __FUNCTION__);

    bufferLength = sizeof(fbe_base_object_mgmt_set_enclosure_led_t);
    status = fbe_base_enclosure_get_packet_payload_control_data(packetPtr,(fbe_base_enclosure_t*)esesEnclosurePtr,TRUE,(fbe_payload_control_buffer_t *)&bufferPtr,bufferLength);
    if(status == FBE_STATUS_OK)
    {
        encl_stat = FBE_ENCLOSURE_STATUS_OK;

    /*
     * Process the buffer and return the packet
     */
    // is the Flash LED's operation active
    if (bufferPtr->flashLedsActive)
    {
        if (bufferPtr->flashLedsOn)
        {
            fbe_base_object_customizable_trace((fbe_base_object_t*)esesEnclosurePtr,
                                FBE_TRACE_LEVEL_INFO,
                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)esesEnclosurePtr),
                                "%s, Start Flashing LED's\n", __FUNCTION__);
            
            ledAction = FBE_ESES_FLASH_ON_ALL_DRIVES;
        }
        else
        {
            fbe_base_object_customizable_trace((fbe_base_object_t*)esesEnclosurePtr,
                                FBE_TRACE_LEVEL_INFO,
                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)esesEnclosurePtr),
                                "%s, Stop Flashing LED's\n", __FUNCTION__);
            
            ledAction = FBE_ESES_FLASH_OFF_ALL_DRIVES;
        }

        fbe_eses_enclosure_formatDriveLedInfo(esesEnclosurePtr, ledAction);
        /*
        * Set condition to write out ESES Control
        */
        status = fbe_lifecycle_set_cond(&fbe_eses_enclosure_lifecycle_const, 
                                        (fbe_base_object_t*)esesEnclosurePtr, 
                                        FBE_ESES_ENCLOSURE_LIFECYCLE_COND_EXPANDER_CONTROL_NEEDED);
        if (status != FBE_STATUS_OK) 
        {
            encl_stat = FBE_ENCLOSURE_STATUS_PACKET_FAILED;
            fbe_base_object_customizable_trace((fbe_base_object_t*)esesEnclosurePtr,
                                FBE_TRACE_LEVEL_ERROR,
                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)esesEnclosurePtr),
                                "%s, can't set ControlNeeded condition, status: 0x%x.\n",
                                __FUNCTION__, status);
        }
    }
}
    fbe_base_enclosure_set_packet_payload_status(packetPtr, encl_stat);
    fbe_transport_complete_packet(packetPtr);

    return status;

}   // end of eses_enclosure_setEnclosureLeds

/*!*************************************************************************
 * @fn eses_enclosure_resetEnclosureShutdownTimer
 *                    fbe_eses_enclosure_t *esesEnclosurePtr,
 *                    fbe_packet_t *packetPtr)
 **************************************************************************
 *
 *  @brief
 *      This function will process the FBE_BASE_ENCLOSURE_CONTROL_CODE_RESET_ENCLOSURE_SHUTDOWN_TIMER
 *      control code.
 *
 *  @param    esesEnclosurePtr - pointer to a Eses Enclosure object
 *  @param    packetPtr - pointer to an FBE control code packet
 *
 *  @return    Status from the processing of this control code.
 *
 *  NOTES:
 *
 *  HISTORY:
 *    11-July-2012: Zhou Eric - Created
 *
 **************************************************************************/
static fbe_status_t 
eses_enclosure_resetEnclosureShutdownTimer(fbe_eses_enclosure_t *esesEnclosurePtr,
                                fbe_packet_t *packetPtr)
{
    fbe_status_t                                status = FBE_STATUS_OK;

    fbe_base_object_customizable_trace((fbe_base_object_t*)esesEnclosurePtr,
                        FBE_TRACE_LEVEL_DEBUG_HIGH,
                        fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)esesEnclosurePtr),
                        "%s entry\n", __FUNCTION__);

    esesEnclosurePtr->reset_shutdown_timer = TRUE;

    /* Set condition to write out EMC enclosure control page. */
    status = fbe_lifecycle_set_cond(&fbe_eses_enclosure_lifecycle_const, 
                                (fbe_base_object_t*)esesEnclosurePtr, 
                                FBE_ESES_ENCLOSURE_LIFECYCLE_COND_EMC_SPECIFIC_CONTROL_NEEDED);
    if (status != FBE_STATUS_OK) 
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)esesEnclosurePtr,
                                FBE_TRACE_LEVEL_ERROR,
                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)esesEnclosurePtr),
                                "%s, can't set EMC_SPECIFIC_CONTROL_NEEDED condition, status: 0x%x.\n",
                                __FUNCTION__,  status);
    }

    fbe_transport_set_status(packetPtr, status, 0);
    fbe_transport_complete_packet(packetPtr);

    return status;
}   // end of eses_enclosure_resetEnclosureShutdownTimer

/*!*************************************************************************
* @fn eses_enclosure_process_firmware_op(
*                   fbe_eses_enclosure_t *eses_enclosure_p, 
*                   fbe_packet_t *packet)                  
***************************************************************************
*
* @brief
*       This function save the firmware download request, and sets
*  the firmware download condition to start the action.
*       This function also handles firmware activate and get_status.
*
* @param      eses_enclosure - The pointer to the fbe_eses_enclosure_t.
* @param      packet - The pointer to the fbe_packet_t.
*
* @return      fbe_status_t.
*
* NOTES
*
* HISTORY
*   27-Jul-2008 GB - Created.
*   19-Sept-2008 NC - combined all firmware operation.
***************************************************************************/
static fbe_status_t 
eses_enclosure_process_firmware_op(fbe_eses_enclosure_t *eses_enclosure_p, 
                                           fbe_packet_t *packet)
{
    fbe_enclosure_mgmt_download_op_t * eses_enclosure_process_firmware_op = NULL;
    fbe_payload_control_buffer_length_t out_len = 0;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t item_count = 0;
    fbe_sg_element_t  * sg_list = NULL;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    
    payload = fbe_transport_get_payload_ex(packet);

    control_operation = fbe_payload_ex_get_control_operation(payload); 
    if(control_operation == NULL)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure_p,
                            FBE_TRACE_LEVEL_ERROR,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure_p),
                            "%s, get_control_operation failed.\n", __FUNCTION__); 

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    // get a pointer to the io packet
    fbe_payload_control_get_buffer(control_operation, &eses_enclosure_process_firmware_op); 
    if(eses_enclosure_process_firmware_op == NULL)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure_p,
                            FBE_TRACE_LEVEL_WARNING,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure_p),
                            "%s, fbe_payload_control_get_buffer fail\n",
                            __FUNCTION__);
        
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    // verify the the size of the expected structure
    fbe_payload_control_get_buffer_length(control_operation, &out_len); 
    if(out_len != sizeof(fbe_enclosure_mgmt_download_op_t))
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure_p,
                            FBE_TRACE_LEVEL_WARNING,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure_p),
                            "%s Invalid out_buffer_length %X \n", 
                            __FUNCTION__, out_len);
        
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure_p,
                        FBE_TRACE_LEVEL_INFO,
                        fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure_p),
                        "%s, fwopcode:%s,target:%s,side:%d\n",
                        __FUNCTION__, 
                        fbe_eses_enclosure_fw_op_to_text(eses_enclosure_process_firmware_op->op_code),
                        fbe_eses_enclosure_fw_targ_to_text(eses_enclosure_process_firmware_op->target),
                        eses_enclosure_process_firmware_op->side_id);

    if(!fbe_eses_enclosure_firmware_op_acceptable(eses_enclosure_p, eses_enclosure_process_firmware_op))
    {
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        /* Return FBE_STATUS_OK so that the subclass won't trace the error as unsupported controlCode*/
        return FBE_STATUS_OK;
    }

    switch (eses_enclosure_process_firmware_op->op_code)
    {
    case FBE_ENCLOSURE_FIRMWARE_OP_DOWNLOAD:
    case FBE_ENCLOSURE_FIRMWARE_OP_ACTIVATE:
            fbe_payload_ex_get_sg_list(payload, &sg_list, &item_count);
        status = fbe_eses_enclosure_process_firmware_download_or_activate(eses_enclosure_p, 
                                                       eses_enclosure_process_firmware_op,
                                                       sg_list);
        break;

    case FBE_ENCLOSURE_FIRMWARE_OP_GET_STATUS:
        // get the status information
        status = eses_enclosure_fetch_download_op_status(eses_enclosure_p, 
                                                            eses_enclosure_process_firmware_op);
        break;

    case FBE_ENCLOSURE_FIRMWARE_OP_ABORT:
        status = fbe_eses_enclosure_process_firmware_abort(eses_enclosure_p, 
                                                           eses_enclosure_process_firmware_op);
        break;

    case FBE_ENCLOSURE_FIRMWARE_OP_NOTIFY_COMPLETION:
        status = fbe_eses_enclosure_process_firmware_notify_completion(eses_enclosure_p, 
                                                                       eses_enclosure_process_firmware_op);
        break;

    default:
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure_p,
                            FBE_TRACE_LEVEL_WARNING,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure_p),
                            "%s unknown firmware update operation code %d. \n", 
                            __FUNCTION__, eses_enclosure_process_firmware_op->op_code);
        /* not handled */
        status = FBE_STATUS_GENERIC_FAILURE;
        break;
    }

    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure_p,
                        FBE_TRACE_LEVEL_DEBUG_LOW,
                        fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure_p),
                        "%s, return status %d \n",
                        __FUNCTION__, status);

    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);

    return FBE_STATUS_OK;
} // eses_enclosure_process_firmware_op

/*!*************************************************************************
* @fn eses_enclosure_get_specific_fw_activation_in_progress                  
***************************************************************************
* @brief
*   This function return if the specific firmware activatin is in progress 
*
* @param    eses_enclosure_p - The pointer to the fbe_eses_enclosure_t.
* @param    fw_target - The specific component
* @param    activationInProgress - the return value.
*
*
* @version
*   10-Mar-2013 zhoue1 - Created.
***************************************************************************/
static fbe_status_t 
eses_enclosure_get_specific_fw_activation_in_progress(fbe_eses_enclosure_t *eses_enclosure_p,
                                             fbe_enclosure_fw_target_t fw_target,
                                             fbe_bool_t *activationInProgress)
{
    if ((eses_enclosure_p->enclCurrentFupInfo.enclFupComponent == fw_target) &&
        (eses_enclosure_p->enclCurrentFupInfo.enclFupOperationStatus == FBE_ENCLOSURE_FW_STATUS_IN_PROGRESS) &&
        (eses_enclosure_p->enclCurrentFupInfo.enclFupOperation == FBE_ENCLOSURE_FIRMWARE_OP_ACTIVATE))
    {
        *activationInProgress = FBE_TRUE;
    }
    else
    {
        *activationInProgress = FBE_FALSE;
    }

    return FBE_STATUS_OK;
}

/*!*************************************************************************
* @fn fbe_eses_enclosure_firmware_op_acceptable                  
***************************************************************************
* @brief
*   This function replaces duplicate occurrences of inline code. Based on 
*   the input paramaters it will call a function to check operation 
*   priority and return a boolena
*
* @param    pEsesEncl - The pointer to the fbe_eses_enclosure_t.
* @param    pFirmwareOp - The pointer to the firmware op.
*
* @return    fbe_bool_t.
*
* @version
*   28-Mar-2011 GB - Created.
***************************************************************************/
static fbe_bool_t 
fbe_eses_enclosure_fup_is_download_acceptable(fbe_eses_enclosure_t *pEsesEncl,
                                              fbe_enclosure_mgmt_download_op_t * pFirmwareOp)
{
    fbe_bool_t acceptable = FALSE;
    if(pFirmwareOp->checkPriority)
    {
        if(fbe_eses_enclosure_firmware_upgrade_has_highest_priority(pEsesEncl, 
                                                                    pFirmwareOp->target,
                                                                    pFirmwareOp->side_id))
        {
            acceptable = TRUE;
        }
    }
    else
    {
        acceptable = TRUE;
    }
    return(acceptable);
}

/*!*************************************************************************
* @fn fbe_eses_enclosure_firmware_op_acceptable                  
***************************************************************************
* @brief
*       This function checks whether the firmware op is acceptable.
*
* @param     pEsesEncl - The pointer to the fbe_eses_enclosure_t.
* @param     pFirmwareOp - The pointer to the firmware op.
*
* @return    fbe_bool_t.
*
* @note
*
* @version
*   08-Jul-2010 PHE - Created.
***************************************************************************/
static fbe_bool_t 
fbe_eses_enclosure_firmware_op_acceptable(fbe_eses_enclosure_t * pEsesEncl, 
                                          fbe_enclosure_mgmt_download_op_t * pFirmwareOp)
{
    fbe_bool_t                          firmwareOpAcceptable = FALSE;

    switch(pFirmwareOp->op_code)
    {
        case FBE_ENCLOSURE_FIRMWARE_OP_DOWNLOAD:
            if((pEsesEncl->enclCurrentFupInfo.enclFupOperation == FBE_ENCLOSURE_FIRMWARE_OP_NONE) &&
               (pEsesEncl->enclCurrentFupInfo.enclFupOperationStatus != FBE_ENCLOSURE_FW_STATUS_IN_PROGRESS) &&
               (pEsesEncl->enclCurrentFupInfo.enclFupOperationAdditionalStatus != FBE_ENCLOSURE_FW_EXT_STATUS_IMAGE_LOADED))
            {
                // previous op is none and not in progress
                firmwareOpAcceptable = fbe_eses_enclosure_fup_is_download_acceptable(pEsesEncl, 
                                                                                     pFirmwareOp);
            }
            else if((pEsesEncl->enclCurrentFupInfo.enclFupOperationAdditionalStatus == FBE_ENCLOSURE_FW_EXT_STATUS_IMAGE_LOADED) &&
               (pEsesEncl->enclCurrentFupInfo.enclFupComponent == pFirmwareOp->target) &&
               (pEsesEncl->enclCurrentFupInfo.enclFupComponentSide == pFirmwareOp->side_id))
            {
                // if download to same target and side it's ok
                firmwareOpAcceptable = fbe_eses_enclosure_fup_is_download_acceptable(pEsesEncl, 
                                                                                     pFirmwareOp);
            }
            else if((pEsesEncl->enclCurrentFupInfo.enclFupOperationAdditionalStatus == FBE_ENCLOSURE_FW_EXT_STATUS_IMAGE_LOADED) &&
               (pEsesEncl->enclCurrentFupInfo.enclFupComponent < FBE_FW_TARGET_LCC_MAIN) &&
               (pFirmwareOp->target < FBE_FW_TARGET_LCC_MAIN) &&
               (pEsesEncl->enclCurrentFupInfo.enclFupComponentSide == pFirmwareOp->side_id))
            {
                /* If the previous target is one part of LCC image such as cdef.bin and 
                 * the current target is another part of LCC image such as initStr.bin and the side is the same, it's ok
                 */
                firmwareOpAcceptable = fbe_eses_enclosure_fup_is_download_acceptable(pEsesEncl, 
                                                                                     pFirmwareOp);
            }
            break;

        case FBE_ENCLOSURE_FIRMWARE_OP_ACTIVATE:
            if((pEsesEncl->enclCurrentFupInfo.enclFupComponent == pFirmwareOp->target) &&
               (pEsesEncl->enclCurrentFupInfo.enclFupComponentSide == pFirmwareOp->side_id))
            {
                // target and side for previous operation (download) match the current 
                firmwareOpAcceptable = TRUE;
            }
            break;

        case FBE_ENCLOSURE_FIRMWARE_OP_GET_STATUS:
        case FBE_ENCLOSURE_FIRMWARE_OP_NOTIFY_COMPLETION:
        case FBE_ENCLOSURE_FIRMWARE_OP_SET_PRIORITY:
            firmwareOpAcceptable = TRUE;
            break;

        case FBE_ENCLOSURE_FIRMWARE_OP_ABORT:
            /* Only accept the command when it is in the middle of downloading or just finished the downloading.
             * There is no way to stop if it is in the middle of activating.
             * If it is already aborted, no need to the accept the command again.
             */
            if((pEsesEncl->enclCurrentFupInfo.enclFupComponent == pFirmwareOp->target) &&
               (pEsesEncl->enclCurrentFupInfo.enclFupComponentSide == pFirmwareOp->side_id))
            {
    
                if((pEsesEncl->enclCurrentFupInfo.enclFupOperation == FBE_ENCLOSURE_FIRMWARE_OP_DOWNLOAD) ||
                   ((pEsesEncl->enclCurrentFupInfo.enclFupOperation == FBE_ENCLOSURE_FIRMWARE_OP_NONE) &&
                    (pEsesEncl->enclCurrentFupInfo.enclFupOperationStatus == FBE_ENCLOSURE_FW_STATUS_NONE) &&
                    (pEsesEncl->enclCurrentFupInfo.enclFupOperationAdditionalStatus == FBE_ENCLOSURE_FW_EXT_STATUS_IMAGE_LOADED)))
                {
                    firmwareOpAcceptable = TRUE;
                }
            }
            
            break;

        default:
            firmwareOpAcceptable = FALSE;
            break;
    }
          
    
    if(!firmwareOpAcceptable) 
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)pEsesEncl,
                            FBE_TRACE_LEVEL_INFO,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)pEsesEncl),
                            "Firmware op %s for esesFwTarget %s, side %d is not acceptable.\n", 
                            fbe_eses_enclosure_fw_op_to_text(pFirmwareOp->op_code),
                            fbe_eses_enclosure_fw_targ_to_text(pFirmwareOp->target),
                            pFirmwareOp->side_id);

        fbe_base_object_customizable_trace((fbe_base_object_t*)pEsesEncl,
                            FBE_TRACE_LEVEL_INFO,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)pEsesEncl),
                            "Current fw op %s esesFwTarget %s side %d sts/extsts 0x%x/0x%x\n",
                            fbe_eses_enclosure_fw_op_to_text(pEsesEncl->enclCurrentFupInfo.enclFupOperation),
                            fbe_eses_enclosure_fw_targ_to_text(pEsesEncl->enclCurrentFupInfo.enclFupComponent),
                            pEsesEncl->enclCurrentFupInfo.enclFupComponentSide,
                                           pEsesEncl->enclCurrentFupInfo.enclFupOperationStatus,
                                           pEsesEncl->enclCurrentFupInfo.enclFupOperationAdditionalStatus);
    }

    return firmwareOpAcceptable;
}

/*!*************************************************************************
 *  @fn fbe_eses_enclosure_firmware_upgrade_has_highest_priority(fbe_eses_enclosure_t * pEsesEncl,
 *                                                               fbe_enclosure_fw_target_t fwTarget,
 *                                                               fbe_u32_t sideId)
 **************************************************************************
 *  @brief
 *      This function checks whether this target has the highest priority for upgrade. 
 *
 *  @param    pEsesEncl - 
 *  @param    esesFwTarget - 
 *
 *  @return   fbe_bool_t
 *           TRUE - has the highest priority.
 *           FALSE - does not have the highest priority.
 *  HISTORY:
 *    05-Jan-2011: PHE - Created.
 *
 **************************************************************************/
static fbe_bool_t 
fbe_eses_enclosure_firmware_upgrade_has_highest_priority(fbe_eses_enclosure_t * pEsesEncl,
                                                         fbe_enclosure_fw_target_t fwTarget,
                                                         fbe_u32_t sideId)
{
    fbe_u32_t                           index = 0;
    fbe_eses_encl_fw_component_info_t * pFwInfo = NULL;
    fbe_status_t                        status = FBE_STATUS_OK;

    status = fbe_eses_enclosure_get_fw_target_addr(pEsesEncl,
                                                   fwTarget, 
                                                   sideId, 
                                                   &pFwInfo);

    if(status != FBE_STATUS_OK) 
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)pEsesEncl,
                                       FBE_TRACE_LEVEL_ERROR,
                                       fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)pEsesEncl),
                                       "%s, invalid fw target %s(%d), side %d.\n", 
                                       __FUNCTION__, 
                                       fbe_eses_enclosure_fw_targ_to_text(pEsesEncl->enclCurrentFupInfo.enclFupComponent),
                                       pEsesEncl->enclCurrentFupInfo.enclFupComponent,
                                       pEsesEncl->enclCurrentFupInfo.enclFupComponentSide);

        return FALSE;
    }

    for(index = 0; index < FBE_FW_TARGET_MAX; index ++)
    {
        if(pEsesEncl->enclFwInfo->subencl_fw_info[index].fwPriority > pFwInfo->fwPriority)
        {
            fbe_base_object_customizable_trace((fbe_base_object_t*)pEsesEncl,
                            FBE_TRACE_LEVEL_INFO,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)pEsesEncl),
                            "%s has lower upgrade priority than %s, need to wait.\n", 
                            fbe_eses_enclosure_fw_targ_to_text(fwTarget),
                            fbe_eses_enclosure_fw_targ_to_text(index));

            return FALSE;
        }
    }
  
    return TRUE;
}

/**************************************************************************
* fbe_eses_enclosure_process_firmware_download_or_activate()                  
***************************************************************************
*
* DESCRIPTION
*       Send IO block to get the status page.
*
* PARAMETERS
*       eses_enclosure - The pointer to the fbe_eses_enclosure_t.
*       packet - The pointer to the fbe_packet_t.
*       page_code - The page code of the requested ESES page.
*
* RETURN VALUES
*       fbe_status_t.
*
* NOTES
*
* HISTORY
*   27-Jul-2008 GB - Created.
***************************************************************************/
static fbe_status_t
fbe_eses_enclosure_process_firmware_download_or_activate(fbe_eses_enclosure_t *eses_enclosure_p, 
                                               fbe_enclosure_mgmt_download_op_t *download_p,
                                               fbe_sg_element_t  * sg_element_p)
{
    fbe_status_t                        status;
    fbe_u8_t                            *image_p = NULL;
    fbe_u8_t                            *allocate_image_p = NULL;
    fbe_u32_t                           image_size;
    fbe_enclosure_status_t              enclStatus;
    fbe_eses_encl_fw_component_info_t   *fw_info_p = NULL;
    fbe_edal_fw_info_t                  edal_fw_info;
    fbe_u8_t                            local_side_id = 0;

    // this function will check the target and side parameters
    status = fbe_eses_enclosure_get_fw_target_addr(eses_enclosure_p,
                                                   download_p->target, 
                                                   download_p->side_id, 
                                                   &fw_info_p);

    if(status != FBE_STATUS_OK) 
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure_p,
                                       FBE_TRACE_LEVEL_ERROR,
                                       fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure_p),
                                       "%s, invalid fw target %s(%d), side %d.\n", 
                                       __FUNCTION__, 
                                       fbe_eses_enclosure_fw_targ_to_text(eses_enclosure_p->enclCurrentFupInfo.enclFupComponent),
                                       eses_enclosure_p->enclCurrentFupInfo.enclFupComponent,
                                       eses_enclosure_p->enclCurrentFupInfo.enclFupComponentSide);

        return status;
    }

    // this check is needed until fbe_api_enclosure_firmware_download is
    // changed to always use the sg list for the image
    if (sg_element_p == NULL)
    {
        // use image pointer
        image_p = download_p->image_p;
        image_size = download_p->size;
    }
    else
    {
        // use sg list pointer
        image_p = fbe_sg_element_address(sg_element_p);
        image_size = fbe_sg_element_count(sg_element_p);
    }

    // some operation dependent checks
    if (download_p->op_code == FBE_ENCLOSURE_FIRMWARE_OP_DOWNLOAD)
    {
        /* image size need to be 4 bytes aligned */
        if ((image_size == 0) ||
            ((image_size & 0x3) != 0) ||
            (image_p == NULL))
        {
            fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure_p,
                                    FBE_TRACE_LEVEL_WARNING,
                                    fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure_p),
                                   "%s ImageNotAligned size:0x%x pointer 0x%p\n", 
                                   __FUNCTION__,
                                   image_size,
                                   image_p);

            return FBE_STATUS_GENERIC_FAILURE;
        }

        if (image_size != download_p->size)
        {
            // only support processing the image from a single list item
            fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure_p,
                                    FBE_TRACE_LEVEL_WARNING,
                                    fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure_p),
                                     "%s Image Size Mismatch exp:0x%x act:0x%x\n", 
                                    __FUNCTION__,download_p->size,
                                    image_size);
           
            return FBE_STATUS_GENERIC_FAILURE;
        }

        /* We should not expect the caller allocate the physically contiguous memory.
        * So allocate the physically contiguous memory and copy the image over.
        * The memory will be released when the download condition is cleared.
        * Anther reason to do so is for the fbe_test wallis_pond. 
        * The fbe_test wallis_pond was removed before because we used the impage_p directly while
        * building the download control page. However, the sg_area was released after 
        * we completed the packet. So the sg_area became inaccesible at that time.
        * By allocating the memory here, we would not hit the previous issue again.
        * So I put back the wallis_pond test. This test is helpful doing the download testing
        * for the physical package - PINGHE. 
        */
        allocate_image_p = (fbe_u8_t *)fbe_memory_ex_allocate(image_size * sizeof(fbe_u8_t));
        if(allocate_image_p == NULL) 
        {
            fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure_p,
                                FBE_TRACE_LEVEL_WARNING,
                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure_p),
                                "DOWNLOAD, Failed to allocate native mem for image,image_size:%d,image_p %p.\n",
                                image_size, image_p);
            return FBE_STATUS_INSUFFICIENT_RESOURCES;
        }
        
        fbe_copy_memory(allocate_image_p, image_p, image_size);
    
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure_p,
                                FBE_TRACE_LEVEL_INFO,
                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure_p),
                                "DOWNLOAD, NativeAllocImagegp:%p, image_size:%d\n",
                                allocate_image_p, image_size);
    }
    else if (download_p->op_code == FBE_ENCLOSURE_FIRMWARE_OP_ACTIVATE)
    {
        // for activate image size should be 0 as ahould ucode data length
        if (image_size != 0)
        {
            fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure_p,
                                    FBE_TRACE_LEVEL_WARNING,
                                    fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure_p),
                                    "%s ActivateSize: act:0x%x exp:0\n", 
                                    __FUNCTION__, image_size);
            
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }

    fbe_eses_enclosure_prep_firmware_op(eses_enclosure_p, download_p);
    // save the command details
    eses_enclosure_p->enclCurrentFupInfo.enclFupImagePointer = allocate_image_p;
    eses_enclosure_p->enclCurrentFupInfo.enclFupImageSize = image_size;
    
    // save from the control packet
    eses_enclosure_p->enclCurrentFupInfo.enclFupComponent = download_p->target;
    eses_enclosure_p->enclCurrentFupInfo.enclFupComponentSide = download_p->side_id;
    eses_enclosure_p->enclCurrentFupInfo.enclFupBytesTransferred = 0;
    eses_enclosure_p->enclCurrentFupInfo.enclFupNoBytesSent = 0;

    // if upgrade peer side, then we need use tunnelling feature. 
    enclStatus = fbe_base_enclosure_edal_getU8((fbe_base_enclosure_t *)eses_enclosure_p,
                                                FBE_ENCL_COMP_SIDE_ID,  // Attribute
                                                FBE_ENCL_ENCLOSURE, 
                                                0,
                                                &local_side_id); 
    if (enclStatus != FBE_ENCLOSURE_STATUS_OK)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure_p,
                                    FBE_TRACE_LEVEL_WARNING,
                                    fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure_p),
                                    "%s Get local sideID failed. Status: %d\n", 
                                    __FUNCTION__, enclStatus);

        return FBE_STATUS_GENERIC_FAILURE;
    }
#if 1
    // disable tunneling until code is tested
    // esp should not send commands targeted to peer
    eses_enclosure_p->enclCurrentFupInfo.enclFupUseTunnelling = FALSE;
#else
    if (download_p->side_id == local_side_id)
    {
        eses_enclosure_p->enclCurrentFupInfo.enclFupUseTunnelling = FALSE;
    }
    else
    {
        eses_enclosure_p->enclCurrentFupInfo.enclFupUseTunnelling = TRUE;
    }
#endif
    // init the fup op status
    eses_enclosure_p->enclCurrentFupInfo.enclFupOperationStatus = FBE_ENCLOSURE_FW_STATUS_IN_PROGRESS;
    eses_enclosure_p->enclCurrentFupInfo.enclFupOperationAdditionalStatus = FBE_ENCLOSURE_FW_EXT_STATUS_NONE;

    /* The LCC CDEF and ISTR image is small and it took less time to be downloaded.
     * It is possible that we miss this status change if we wait to check the change after the poll.
     * For example, after the CDEF image is download, CDEF status/addlStatus is 0x0/0x2.
     * When the retry is needed due to the failure of the expander image, we need to download CDES image again.
     * Then the status change will be 0x0/0x2 -> 0x1/0x0 -> 0x0/0x2.
     * If we miss the middle status 0x1/0x0, ESP will not get the status change notification.
     */
    status = fbe_lifecycle_set_cond(&fbe_eses_enclosure_lifecycle_const, 
                                    (fbe_base_object_t*)eses_enclosure_p,  
                                    FBE_BASE_DISCOVERING_LIFECYCLE_COND_DISCOVERY_UPDATE);

    if (status != FBE_STATUS_OK) 
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure_p,
                            FBE_TRACE_LEVEL_WARNING,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure_p),
                            "%s can't set DISCOVERY_UPDATE condition, status: 0x%x.\n",
                            __FUNCTION__, status);

    }

    // update the operation field
    eses_enclosure_p->enclCurrentFupInfo.enclFupOperation = (fbe_u8_t) download_p->op_code;

    if (download_p->op_code == FBE_ENCLOSURE_FIRMWARE_OP_DOWNLOAD)
    {
        fw_info_p->isDownloadOp = FBE_TRUE;

        // clear the old revision field, fill it when activate issued
        fbe_zero_memory(fw_info_p->fwEsesFwRevision, FBE_ESES_FW_REVISION_SIZE);

        if(eses_enclosure_p->enclCurrentFupInfo.enclFupUseTunnelling == TRUE)
        {
            // Initialize the tunnel firmware upgrade FSM context. 
            fbe_eses_tunnel_fup_context_init(&(eses_enclosure_p->tunnel_fup_context_p),
                                                     eses_enclosure_p,
                                                     FBE_ESES_ST_TUNNEL_FUP_INIT,
                                                     FBE_ESES_EV_TUNNEL_FUP_TUNNEL_DL,
                                                     FBE_ESES_TUNNEL_DOWNLOAD_MAX_FAILURE_RETRY,
                                                     FBE_ESES_TUNNEL_DOWNLOAD_MAX_BUSY_RETRY);
        }
    } 
    else if (download_p->op_code == FBE_ENCLOSURE_FIRMWARE_OP_ACTIVATE)
    {
        fw_info_p->isDownloadOp = FBE_FALSE;

        // get the rev info based on fw target and side id
        enclStatus = fbe_eses_enclosure_get_edal_fw_info(eses_enclosure_p, 
                                            download_p->target,
                                            download_p->side_id,
                                            &edal_fw_info);


        if(enclStatus == FBE_ENCLOSURE_STATUS_OK)
        {
            fbe_copy_memory(fw_info_p->fwEsesFwRevision, &edal_fw_info.fwRevision, FBE_ESES_FW_REVISION_SIZE);
        }
        else
        {
            fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure_p,
                                                FBE_TRACE_LEVEL_WARNING,
                                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure_p),
                                                "%s Failed to get FwRevInfo, enclStatus:0x%x.\n", 
                                                __FUNCTION__, enclStatus);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        if (eses_enclosure_p->enclCurrentFupInfo.enclFupUseTunnelling == TRUE)
        {
            fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure_p,
                                                FBE_TRACE_LEVEL_INFO,
                                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure_p),
                                                "FBE FUP %s tunnel activate: old rev %s\n",
                                                __FUNCTION__, fw_info_p->fwEsesFwRevision);
    
            // Initialize the tunnel firmware upgrade FSM context. 
            fbe_eses_tunnel_fup_context_init(&(eses_enclosure_p->tunnel_fup_context_p),
                                                     eses_enclosure_p,
                                                     FBE_ESES_ST_TUNNEL_FUP_READY,
                                                     FBE_ESES_EV_TUNNEL_FUP_TUNNEL_DL,
                                                     FBE_ESES_TUNNEL_ACTIVATE_MAX_FAILURE_RETRY,
                                                     FBE_ESES_TUNNEL_ACTIVATE_MAX_BUSY_RETRY);
        }
    }

    // save the start time
    eses_enclosure_p->enclCurrentFupInfo.enclFupStartTime = fbe_get_time();

    /* set the condition to let monitor pick up the request */
    status = fbe_lifecycle_set_cond(&fbe_eses_enclosure_lifecycle_const, 
                                    (fbe_base_object_t*)eses_enclosure_p,  
                                    FBE_ESES_ENCLOSURE_LIFECYCLE_COND_FIRMWARE_DOWNLOAD);

    if (status != FBE_STATUS_OK) 
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure_p,
                            FBE_TRACE_LEVEL_WARNING,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure_p),
                            "%s can't set firmware download condition, status: 0x%x.\n",
                            __FUNCTION__, status);
        /* clean up so next request can be processed */
        eses_enclosure_p->enclCurrentFupInfo.enclFupOperationStatus = FBE_ENCLOSURE_FW_STATUS_FAIL;
        eses_enclosure_p->enclCurrentFupInfo.enclFupOperationAdditionalStatus = FBE_ENCLOSURE_FW_EXT_STATUS_ERR_LFC;
    
        if (download_p->op_code == FBE_ENCLOSURE_FIRMWARE_OP_DOWNLOAD) 
        {
            if(eses_enclosure_p->enclCurrentFupInfo.enclFupImagePointer != NULL)
            {
                fbe_memory_ex_release(eses_enclosure_p->enclCurrentFupInfo.enclFupImagePointer);
                eses_enclosure_p->enclCurrentFupInfo.enclFupImagePointer = NULL;
            }
        }
    }

    return status;
} // fbe_eses_enclosure_process_firmware_download_or_activate

/*!*************************************************************************
* @fn fbe_eses_enclosure_prep_firmware_op                  
***************************************************************************
* @brief
*       This function does the preparation before accepting the new op.
*
* @param     pEsesEncl - The pointer to the fbe_eses_enclosure_t.
* @param     pFirmwareOp - The pointer to the firmware op.
*
* @return    fbe_bool_t.
*
* @note
*
* @version
*   02-Sept-2010 PHE - Created.
***************************************************************************/
static fbe_status_t 
fbe_eses_enclosure_prep_firmware_op(fbe_eses_enclosure_t * pEsesEncl, 
                                          fbe_enclosure_mgmt_download_op_t * pFirmwareOp)
{
    fbe_eses_encl_fw_component_info_t  * pFwInfo = NULL;
    fbe_status_t                         status = FBE_STATUS_OK;

    if((pFirmwareOp->op_code != FBE_ENCLOSURE_FIRMWARE_OP_DOWNLOAD) &&
       (pFirmwareOp->op_code != FBE_ENCLOSURE_FIRMWARE_OP_ACTIVATE))
    {
        return FBE_STATUS_OK;
    }

    /*Split trace to two lines*/
    fbe_base_object_customizable_trace((fbe_base_object_t*)pEsesEncl,
                                       FBE_TRACE_LEVEL_INFO,
                                       fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)pEsesEncl),
                                       "eses_encl_prep_fw_op, New opcode %s,esesFwTarget %s.\n", 
                                       fbe_eses_enclosure_fw_op_to_text(pFirmwareOp->op_code), 
                                       fbe_eses_enclosure_fw_targ_to_text(pFirmwareOp->target));

    fbe_base_object_customizable_trace((fbe_base_object_t*)pEsesEncl,
                                       FBE_TRACE_LEVEL_INFO,
                                       fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)pEsesEncl),
                                       "Current enclFupOperation %s, enclFupComponent %s, enclFupComponentSide %d.\n",
                                       fbe_eses_enclosure_fw_op_to_text(pEsesEncl->enclCurrentFupInfo.enclFupOperation),
                                       fbe_eses_enclosure_fw_targ_to_text(pEsesEncl->enclCurrentFupInfo.enclFupComponent),
                                       pEsesEncl->enclCurrentFupInfo.enclFupComponentSide);

    /* It is possible that this is the first firmware op for the enclosure 
    * so enclCurrentFupInfo.enclFupComponent is FBE_FW_TARGET_INVALID
    * In this case, we don't need to continue to save anything.
    */
    if(pEsesEncl->enclCurrentFupInfo.enclFupComponent == FBE_FW_TARGET_INVALID) 
    {
        return FBE_STATUS_OK;
    }

    status = fbe_eses_enclosure_get_fw_target_addr(pEsesEncl,
                                                   pEsesEncl->enclCurrentFupInfo.enclFupComponent, 
                                                   pEsesEncl->enclCurrentFupInfo.enclFupComponentSide, 
                                                   &pFwInfo);
    if(status != FBE_STATUS_OK) 
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)pEsesEncl,
                                       FBE_TRACE_LEVEL_ERROR,
                                       fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)pEsesEncl),
                                       "%s, invalid fw target %s(%d), side %d.\n", 
                                       __FUNCTION__, 
                                       fbe_eses_enclosure_fw_targ_to_text(pEsesEncl->enclCurrentFupInfo.enclFupComponent),
                                       pEsesEncl->enclCurrentFupInfo.enclFupComponent,
                                       pEsesEncl->enclCurrentFupInfo.enclFupComponentSide);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    if((pFirmwareOp->target == pEsesEncl->enclCurrentFupInfo.enclFupComponent) &&
       (pFirmwareOp->side_id == pEsesEncl->enclCurrentFupInfo.enclFupComponentSide))
    {
        /* The same opcode as the previous op. */
        if(pFirmwareOp->op_code == FBE_ENCLOSURE_FIRMWARE_OP_DOWNLOAD)
        {
            /* The opcode is download. 
             * Set to the default vaule so that the change will be detected
             * when the image download completes.
             */
            pFwInfo->prevEnclFupOperationStatus = FBE_ENCLOSURE_FW_STATUS_NONE;
            pFwInfo->prevEnclFupOperationAdditionalStatus = FBE_ENCLOSURE_FW_EXT_STATUS_NONE;
        }
        else if(pFirmwareOp->op_code == FBE_ENCLOSURE_FIRMWARE_OP_ACTIVATE)
        {
            /* The opcode is activate. 
             * Set to the default value so that the change will be detected
             * when the image activation completes.
             */
            pFwInfo->prevEnclFupOperationStatus = FBE_ENCLOSURE_FW_STATUS_NONE;
            pFwInfo->prevEnclFupOperationAdditionalStatus = FBE_ENCLOSURE_FW_EXT_STATUS_IMAGE_LOADED;
        }
    }
    else
    {
        /* Different target from the previous op. 
         * Save the current status and additional status for the previous op
         * so that they can be queried.
         */
        pFwInfo->enclFupOperationStatus = pEsesEncl->enclCurrentFupInfo.enclFupOperationStatus;
        pFwInfo->enclFupOperationAdditionalStatus = pEsesEncl->enclCurrentFupInfo.enclFupOperationAdditionalStatus;
    }

    return FBE_STATUS_OK;
}

/**************************************************************************
* eses_enclosure_fetch_download_op_status()                  
***************************************************************************
*
* DESCRIPTION
*       Send IO block to get the status page.
*
* PARAMETERS
*       eses_enclosure - The pointer to the fbe_eses_enclosure_t.
*       packet - The pointer to the fbe_packet_t.
*       page_code - The page code of the requested ESES page.
*
* RETURN VALUES
*       fbe_status_t.
*
* NOTES
*
* HISTORY
*   27-Jul-2008 GB - Created.
***************************************************************************/
static fbe_status_t 
eses_enclosure_fetch_download_op_status(fbe_eses_enclosure_t *eses_enclosure_p,
                                             fbe_enclosure_mgmt_download_op_t *mgmt_download_status_p)
{
    fbe_eses_encl_fw_component_info_t * fw_info_p = NULL;
    fbe_status_t                        status = FBE_STATUS_OK;

    // get the operation
    status = fbe_eses_enclosure_get_fw_target_addr(eses_enclosure_p,
                                                   mgmt_download_status_p->target, 
                                                   mgmt_download_status_p->side_id, 
                                                   &fw_info_p);

    if(status != FBE_STATUS_OK) 
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure_p,
                                       FBE_TRACE_LEVEL_ERROR,
                                       fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure_p),
                                       "%s, invalid fw target %s(%d), side %d.\n", 
                                       __FUNCTION__, 
                                       fbe_eses_enclosure_fw_targ_to_text(eses_enclosure_p->enclCurrentFupInfo.enclFupComponent),
                                       eses_enclosure_p->enclCurrentFupInfo.enclFupComponent,
                                       eses_enclosure_p->enclCurrentFupInfo.enclFupComponentSide);

        return status;
    }

    // update status info to be returned to shim/flare
    // if ESES_DOWNLOAD_STATUS_NONE, non-zero value in extended_status indicates operation has failed
    if(fw_info_p->enclFupOperationStatus == FBE_ENCLOSURE_FW_STATUS_FAIL)
    {
        mgmt_download_status_p->status = (fw_info_p->isDownloadOp)? 
                FBE_ENCLOSURE_FW_STATUS_DOWNLOAD_FAILED : FBE_ENCLOSURE_FW_STATUS_ACTIVATE_FAILED;
    }
    else
    {
        mgmt_download_status_p->status = fw_info_p->enclFupOperationStatus;
    }
   
    mgmt_download_status_p->extended_status = fw_info_p->enclFupOperationAdditionalStatus;
    mgmt_download_status_p->bytes_downloaded = eses_enclosure_p->enclCurrentFupInfo.enclFupBytesTransferred;

    return FBE_STATUS_OK;

} // eses_enclosure_fetch_download_op_status

/*!*************************************************************************
* @fn fbe_eses_enclosure_process_firmware_abort                  
***************************************************************************
* @brief
*       This function gets called when the firmware op is abort.
*
* @param     pEsesEncl - The pointer to the fbe_eses_enclosure_t.
* @param     pFirmwareOp - The pointer to the firmware op.
*
* @return    fbe_status_t.
*
* @note
*
* @version
*   08-Jul-2010 PHE - Created.
***************************************************************************/
static fbe_status_t 
fbe_eses_enclosure_process_firmware_abort(fbe_eses_enclosure_t * pEsesEncl, 
                                          fbe_enclosure_mgmt_download_op_t * pFirmwareOp)
{
    fbe_status_t status = FBE_STATUS_OK;

    /* update the operation field. */
    pEsesEncl->enclCurrentFupInfo.enclFupOperation = FBE_ENCLOSURE_FW_STATUS_ABORT;

    /* set the condition to let monitor pick up the request */
    status = fbe_lifecycle_set_cond(&fbe_eses_enclosure_lifecycle_const, 
                                    (fbe_base_object_t*)pEsesEncl,  
                                    FBE_ESES_ENCLOSURE_LIFECYCLE_COND_FIRMWARE_DOWNLOAD);

    if (status != FBE_STATUS_OK) 
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)pEsesEncl,
                            FBE_TRACE_LEVEL_WARNING,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)pEsesEncl),
                            "%s can't set firmware download condition, status: 0x%x.\n",
                            __FUNCTION__, status);
    }

    return status;
}


/*!*************************************************************************
* @fn fbe_eses_enclosure_process_firmware_notify_completion                  
***************************************************************************
* @brief
*       This function gets called when the firmware op is notify completion.
*       It drops the firmware priority so that other firmware upgrade can proceed.
*
* @param     pEsesEncl - The pointer to the fbe_eses_enclosure_t.
* @param     pFirmwareOp - The pointer to the firmware op.
*
* @return    fbe_status_t.
*
* @note
*
* @version
*   08-Jul-2010 PHE - Created.
***************************************************************************/
static fbe_status_t 
fbe_eses_enclosure_process_firmware_notify_completion(fbe_eses_enclosure_t * pEsesEncl, 
                                          fbe_enclosure_mgmt_download_op_t * pFirmwareOp)
{
    switch (pFirmwareOp->target)
    {
        case FBE_FW_TARGET_LCC_EXPANDER:
        case FBE_FW_TARGET_LCC_MAIN:
           /* 
            * FBE_FW_TARGET_LCC_EXPANDER image is the last image to be downloaded. 
            * We will do activation for all the images of LCC at the same time. 
            * When the upgrade of FBE_FW_TARGET_LCC_EXPANDER image completed, 
            * we can assume other LCC images are done as well.
            */
            pEsesEncl->enclFwInfo->subencl_fw_info[FBE_FW_TARGET_LCC_BOOT_LOADER].fwPriority = 
                                         FBE_ESES_ENCLOSURE_FW_PRIORITY_LOW;
            pEsesEncl->enclFwInfo->subencl_fw_info[FBE_FW_TARGET_LCC_INIT_STRING].fwPriority = 
                                         FBE_ESES_ENCLOSURE_FW_PRIORITY_LOW;
            pEsesEncl->enclFwInfo->subencl_fw_info[FBE_FW_TARGET_LCC_FPGA].fwPriority = 
                                         FBE_ESES_ENCLOSURE_FW_PRIORITY_LOW;
            pEsesEncl->enclFwInfo->subencl_fw_info[FBE_FW_TARGET_LCC_EXPANDER].fwPriority = 
                                         FBE_ESES_ENCLOSURE_FW_PRIORITY_LOW;
            pEsesEncl->enclFwInfo->subencl_fw_info[FBE_FW_TARGET_LCC_MAIN].fwPriority = 
                                         FBE_ESES_ENCLOSURE_FW_PRIORITY_LOW;
            break;

        case FBE_FW_TARGET_LCC_BOOT_LOADER:
        case FBE_FW_TARGET_LCC_INIT_STRING:
        case FBE_FW_TARGET_LCC_FPGA:
            /*Do not change the prority level until FBE_FW_TARGET_LCC_EXPANDER upgrade completes.
             */
            break;

        case FBE_FW_TARGET_SPS_PRIMARY:
        case FBE_FW_TARGET_SPS_SECONDARY:
        case FBE_FW_TARGET_SPS_BATTERY:
            pEsesEncl->enclFwInfo->subencl_fw_info[pFirmwareOp->target].fwPriority = 
                                     FBE_ESES_ENCLOSURE_FW_PRIORITY_LOW;
            break;

        case FBE_FW_TARGET_PS:
        case FBE_FW_TARGET_COOLING:
            pEsesEncl->enclFwInfo->subencl_fw_info[pFirmwareOp->target + pFirmwareOp->side_id].fwPriority = 
                                    FBE_ESES_ENCLOSURE_FW_PRIORITY_LOW;
            break;
    
        default:
            return FBE_STATUS_GENERIC_FAILURE;
            break;
    }
            
    return FBE_STATUS_OK;
}


/*!*************************************************************************
* @fn fbe_eses_enclosure_repack_ctrl_op()                  
***************************************************************************
* @brief
*       This function repacks the control operation. It allocates
*       a new control operation and send down the SCSI command directly. 
*       The operation is ESES page related.
*
* @param     eses_enclosure - The pointer to the fbe_eses_enclosure_t.
* @param     packet - The pointer to the fbe_packet_t.
* @param     ctrl_code - The control code.
*
* @return    fbe_status_t.
*
* NOTES
*
* HISTORY
*   06-Oct-2008 PHE - Created.
*   16-Oct-2009 Prasenjeet - Added resume prom request type in fbe_eses_enclosure_prom_id_to_buf_id
***************************************************************************/
static fbe_status_t 
fbe_eses_enclosure_repack_ctrl_op(fbe_eses_enclosure_t *eses_enclosure, 
                                           fbe_packet_t *packet,
                                           fbe_base_enclosure_control_code_t ctrl_code)
{
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;   
    fbe_payload_control_operation_t * new_control_operation = NULL;
    fbe_payload_control_buffer_t control_buffer = NULL;    
    fbe_payload_control_buffer_length_t control_buffer_length = 0;
    fbe_enclosure_mgmt_ctrl_op_t * eses_page_op = NULL;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_enclosure_mgmt_read_buf_cmd_t *read_buf_cmd_p = NULL;       // trace read buffer
    fbe_enclosure_mgmt_resume_read_cmd_t *resume_read_cmd_p = NULL; // resume prom read buffer
    fbe_base_object_mgmt_exp_string_out_ctrl_t  *stringOutCtrlPtr = NULL;
    fbe_enclosure_mgmt_exp_threshold_info_t * thresholdOutCtrlPtr = NULL;
    fbe_base_object_mgmt_exp_test_mode_ctrl_t   *testModeCtrlPtr = NULL;
    fbe_sg_element_t    *sg_element = NULL;
    fbe_payload_control_operation_opcode_t testModeOpcode = FBE_ESES_CTRL_OPCODE_INVALID;
    fbe_u32_t bufferSize = 0;

    /********
     * BEGIN
     ********/

    /* The original packet status is FBE_STATUS_INVALID (initialized in fbe_transport_initialize_packet)
     * we set it to FBE_STATUS_OK to start with. 
     * It will be updated as needed later on. 
     */
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);

    payload = fbe_transport_get_payload_ex(packet);

    control_operation = fbe_payload_ex_get_control_operation(payload);
    if(control_operation == NULL)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                            FBE_TRACE_LEVEL_ERROR,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                            "%s, get_control_operation failed.\n", __FUNCTION__); 

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer(control_operation, &control_buffer);
    if( control_buffer == NULL )
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                            FBE_TRACE_LEVEL_WARNING,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                            "%s,  fbe_payload_control_get_buffer failed\n", __FUNCTION__);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Set the control operation status and status qualifier to OK to start with. 
     * They will be updated as needed later on.
     */
    fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_payload_control_set_status_qualifier(control_operation, FBE_ENCLOSURE_STATUS_OK);

    eses_page_op = (fbe_enclosure_mgmt_ctrl_op_t *)control_buffer;

    /* Verify the control buffer size with the size of the expected structure */
    status = fbe_payload_control_get_buffer_length(control_operation, &control_buffer_length);
    if((status != FBE_STATUS_OK) || 
       (control_buffer_length != sizeof(fbe_enclosure_mgmt_ctrl_op_t)))
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                            FBE_TRACE_LEVEL_WARNING,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                            "%s, control_buffer_length %d, expected_length: %llu, status: 0x%x.\n", 
                            __FUNCTION__, control_buffer_length, 
                            (unsigned long long)sizeof(fbe_enclosure_mgmt_ctrl_op_t), status);
        
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    } 

    /*
     * Check whether there is config change in progress.
     */
    if(eses_enclosure->enclosureConfigBeingUpdated)
    {
        //config change in progress.
        fbe_base_object_customizable_trace((fbe_base_object_t *) eses_enclosure, 
                               FBE_TRACE_LEVEL_INFO,
                               fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *) eses_enclosure),
                               "repack_ctrl_op, ctrl_code 0x%x, return Busy (Config Change In Progress).\n",
                               ctrl_code);

        status = FBE_STATUS_BUSY;
        fbe_transport_set_status(packet, status, 0);
        fbe_transport_complete_packet(packet);
        return status;
    }
    
    /* Allocate a new control operation. */
    new_control_operation = fbe_payload_ex_allocate_control_operation(payload);
    if(new_control_operation == NULL)
    {    
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                        FBE_TRACE_LEVEL_ERROR,
                        fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                        "%s, allocate_control_operation failed.\n", __FUNCTION__);

        // Set the packet status to FBE_STATUS_GENERIC_FAILURE.
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Set the completion function. */
    status = fbe_transport_set_completion_function(packet, 
                                                fbe_eses_enclosure_repack_ctrl_op_completion, 
                                                eses_enclosure);
    /* Save the sg_list info back to the response_buf_p and response_buf_size for some control codes. */
    switch(ctrl_code)
    {
        case FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_TRACE_BUF_INFO_STATUS: 
        case FBE_BASE_ENCLOSURE_CONTROL_CODE_RESUME_READ:
        case FBE_BASE_ENCLOSURE_CONTROL_CODE_READ_BUF:
        case FBE_BASE_ENCLOSURE_CONTROL_CODE_RAW_RCV_DIAG:
        case FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_ENCLOSURE_STATISTICS:
        case FBE_BASE_ENCLOSURE_CONTROL_CODE_EXPANDER_THRESHOLD_IN_CONTROL:         
        case FBE_BASE_ENCLOSURE_CONTROL_CODE_RAW_INQUIRY:
        case FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_SPS_MANUF_INFO:
            // get the sg element and the sg count
            fbe_payload_ex_get_sg_list(payload, &sg_element, NULL);

            if (sg_element != NULL)
            {
                eses_page_op->response_buf_p = sg_element[0].address;
                eses_page_op->response_buf_size = sg_element[0].count;
            }
            else
            {
                fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                    FBE_TRACE_LEVEL_WARNING,
                                    fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                    "%s, SG_LIST is NULL, ctrl_code: 0x%x.\n", __FUNCTION__, ctrl_code);

                fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
                fbe_transport_complete_packet(packet);
                return FBE_STATUS_GENERIC_FAILURE;
            }
            break;

        case FBE_BASE_ENCLOSURE_CONTROL_CODE_RESUME_WRITE:
            {
                // get the sg element and the sg count
                fbe_payload_ex_get_sg_list(payload, &sg_element, NULL);

                if (sg_element != NULL)
                {
                    eses_page_op->cmd_buf.resume_write.data_buf_p = sg_element[0].address;
                    eses_page_op->cmd_buf.resume_write.data_buf_size = sg_element[0].count;
                }
                else
                {
                    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                        FBE_TRACE_LEVEL_WARNING,
                                        fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                        "%s, SG_LIST is NULL, ctrl_code: 0x%x.\n", __FUNCTION__, ctrl_code);

                    fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
                    fbe_transport_complete_packet(packet);
                    return FBE_STATUS_GENERIC_FAILURE;
                }
            }
            break;

        default:
            break;
    }

    switch (ctrl_code)
    {
        case FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_TRACE_BUF_INFO_STATUS: 
            status = fbe_payload_control_build_operation(new_control_operation,  // control operation
                                    FBE_ESES_CTRL_OPCODE_GET_TRACE_BUF_INFO_STATUS,  // opcode
                                    eses_page_op, // buffer
                                    sizeof(fbe_enclosure_mgmt_ctrl_op_t));   // buffer_length   
            
            /* send the command to get the trace buf info in the EMC specific status page. */
            status = fbe_eses_enclosure_send_scsi_cmd(eses_enclosure, packet);            
            break;

        case FBE_BASE_ENCLOSURE_CONTROL_CODE_SET_TRACE_BUF_INFO_CTRL: 
            status = fbe_payload_control_build_operation(new_control_operation,  // control operation
                                    FBE_ESES_CTRL_OPCODE_SET_TRACE_BUF_INFO_CTRL,  // opcode
                                    eses_page_op, // buffer
                                    sizeof(fbe_enclosure_mgmt_ctrl_op_t));   // buffer_length   
           
            /* send the command to set the trace buf info in the EMC specific control page. */
            status = fbe_eses_enclosure_send_scsi_cmd(eses_enclosure, packet);              
            break; 

        case FBE_BASE_ENCLOSURE_CONTROL_CODE_EXPANDER_STRING_OUT_CONTROL:
#if FALSE
            // we must be in Test Mode to issue String Out commands
            if (!eses_enclosure->test_mode_status)
            {
                fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                    FBE_TRACE_LEVEL_WARNING,
                                    fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                    "%s, enclosure not in Test Mode\n", __FUNCTION__);

                fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
                fbe_transport_complete_packet(packet);
                return FBE_STATUS_GENERIC_FAILURE;
            }
#endif

            // get info from command buffer
            stringOutCtrlPtr = (fbe_base_object_mgmt_exp_string_out_ctrl_t *)control_buffer;
            fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                FBE_TRACE_LEVEL_INFO,
                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                "%s, testMode %d, StringOut %s\n", 
                                __FUNCTION__, 
                                eses_enclosure->test_mode_status,
                                stringOutCtrlPtr->stringOutCmd);
            fbe_move_memory(&(eses_page_op->cmd_buf.stringOutInfo), 
                            stringOutCtrlPtr->stringOutCmd, 
                            FBE_BASE_EXP_STRING_OUT_CMD_MAX_LENGTH);

            status = fbe_payload_control_build_operation(new_control_operation,      // control operation
                                    FBE_ESES_CTRL_OPCODE_STRING_OUT_CMD,    // opcode
                                    eses_page_op,                           // buffer
                                    FBE_BASE_EXP_STRING_OUT_CMD_MAX_LENGTH);// buffer_length         
            /* send the debug command. */
            status = fbe_eses_enclosure_send_scsi_cmd(eses_enclosure, packet);
            break;

        case FBE_BASE_ENCLOSURE_CONTROL_CODE_EXPANDER_THRESHOLD_OUT_CONTROL:
            // get info from command buffer
            thresholdOutCtrlPtr = (fbe_enclosure_mgmt_exp_threshold_info_t *)control_buffer;
            fbe_move_memory(&(eses_page_op->cmd_buf.thresholdOutInfo), 
                            thresholdOutCtrlPtr, 
                            sizeof(fbe_enclosure_mgmt_exp_threshold_info_t));

            fbe_payload_control_build_operation(new_control_operation,         // control operation
                                    FBE_ESES_CTRL_OPCODE_THRESHOLD_OUT_CMD,    // opcode
                                    eses_page_op,                              // buffer
                                    sizeof(fbe_enclosure_mgmt_exp_threshold_info_t));    // buffer_length         
            /* send the debug command. */
            status = fbe_eses_enclosure_send_scsi_cmd(eses_enclosure, packet);
            break;
        case FBE_BASE_ENCLOSURE_CONTROL_CODE_EXPANDER_TEST_MODE_CONTROL:
            // Process Test Mode Control
            testModeCtrlPtr = (fbe_base_object_mgmt_exp_test_mode_ctrl_t *)control_buffer;
            /*
             * Process the buffer and return the packet
             */
            switch (testModeCtrlPtr->testModeAction)
            {
            case FBE_EXP_TEST_MODE_NO_ACTION:
                break;
            case FBE_EXP_TEST_MODE_DISABLE:
                fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                    FBE_TRACE_LEVEL_INFO,
                                    fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                    "FBE_EXP_TEST_MODE_DISABLE, \n");
                
                testModeOpcode = FBE_ESES_CTRL_OPCODE_MODE_SELECT;
                eses_enclosure->test_mode_rqstd = FALSE;
                eses_enclosure->mode_select_needed |= FBE_ESES_MODE_SELECT_EENP_MODE_PAGE;
                break;
            case FBE_EXP_TEST_MODE_ENABLE:
                fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                    FBE_TRACE_LEVEL_INFO,
                                    fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                    "FBE_EXP_TEST_MODE_ENABLE \n");
                
                testModeOpcode = FBE_ESES_CTRL_OPCODE_MODE_SELECT;
                eses_enclosure->test_mode_rqstd = TRUE;
                eses_enclosure->mode_select_needed |= FBE_ESES_MODE_SELECT_EENP_MODE_PAGE;
                break;
            case FBE_EXP_TEST_MODE_STATUS:
                fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                    FBE_TRACE_LEVEL_INFO,
                                    fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                    "FBE_EXP_TEST_MODE_STATUS \n");
                
                testModeOpcode = FBE_ESES_CTRL_OPCODE_MODE_SENSE;
                break;
            default:
                fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                    FBE_TRACE_LEVEL_ERROR,
                                    fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                    "%s, testModeAction %d invalid\n", 
                                    __FUNCTION__, testModeCtrlPtr->testModeAction);
                
                fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
                fbe_transport_complete_packet(packet);
                return FBE_STATUS_GENERIC_FAILURE;
            }
            // send appropriate command off
            status = fbe_payload_control_build_operation(new_control_operation,      // control operation
                                    testModeOpcode,                         // opcode
                                    eses_page_op,                           // buffer
                                    sizeof(fbe_base_object_mgmt_exp_test_mode_ctrl_t)); // buffer_length         
            /* send the debug command. */
            status = fbe_eses_enclosure_send_scsi_cmd(eses_enclosure, packet);
            break;

        case FBE_BASE_ENCLOSURE_CONTROL_CODE_RESUME_READ:
            // use read buffer command to get resume data
            resume_read_cmd_p = (fbe_enclosure_mgmt_resume_read_cmd_t *)(&eses_page_op->cmd_buf.resume_read);

            // set the mode to use the buffer id for target data to read
            resume_read_cmd_p->read_buf.mode = FBE_ESES_READ_BUF_MODE_DATA;

            
            /* Get the buffer id for the device for the resume read operation */
            status = fbe_eses_enclosure_get_buf_id(eses_enclosure, 
                                                   resume_read_cmd_p->deviceType, 
                                                   &resume_read_cmd_p->deviceLocation,
                                                   &resume_read_cmd_p->read_buf.buf_id, 
                                                   FBE_ENCLOSURE_READ_RESUME_PROM);

            if (status != FBE_STATUS_OK)
            {
                // no buffer id found for this subenclosure type, fail and complete
                fbe_payload_control_set_status(new_control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
                fbe_transport_complete_packet(packet);
            }

            if((resume_read_cmd_p->deviceType == FBE_DEVICE_TYPE_PS) &&
                fbe_eses_enclosure_is_ps_resume_format_special(eses_enclosure)) 
            {
                status = fbe_eses_enclosure_get_buf_size(eses_enclosure, 
                                                   resume_read_cmd_p->deviceType, 
                                                   &resume_read_cmd_p->deviceLocation,
                                                   &bufferSize, 
                                                   FBE_ENCLOSURE_READ_RESUME_PROM);

                if (status != FBE_STATUS_OK) 
                {
                    // no buffer id found for this subenclosure type, fail and complete
                    fbe_payload_control_set_status(new_control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
                    fbe_transport_complete_packet(packet);
                    return FBE_STATUS_GENERIC_FAILURE;
                }
                else if(bufferSize == 0) 
                {
                    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                    FBE_TRACE_LEVEL_INFO,
                                    fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                    "PS bufferSize not available, set rp size unknown cond. Please retry.\n");

                    status = fbe_lifecycle_set_cond(&fbe_eses_enclosure_lifecycle_const, 
                                        (fbe_base_object_t*)eses_enclosure, 
                                        FBE_ESES_ENCLOSURE_LIFECYCLE_COND_RP_SIZE_UNKNOWN);

                    if (status != FBE_STATUS_OK) 
                    {
                        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                    FBE_TRACE_LEVEL_ERROR,
                                    fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                    "%s, can't set rp size unknown cond, status 0x%X.\n", 
                                    __FUNCTION__,
                                    status);
                    }

                    fbe_payload_control_set_status(new_control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
                    fbe_payload_control_set_status_qualifier(new_control_operation, FBE_ENCLOSURE_STATUS_BUSY);
                    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
                    payload = fbe_transport_get_payload_ex(packet);
                    /* Make control operation allocated in usurper as the current operation. */
                    status = fbe_payload_ex_increment_control_operation_level(payload);
                    fbe_transport_complete_packet(packet);
                    return FBE_STATUS_OK;
                }
                else
                {
                    eses_page_op->response_buf_size = bufferSize;
                }
            }
            
            // build the operation
            status = fbe_payload_control_build_operation(new_control_operation,  // control operation
                            FBE_ESES_CTRL_OPCODE_READ_RESUME,           // opcode
                            eses_page_op,                               // buffer
                            sizeof(fbe_enclosure_mgmt_ctrl_op_t)); // buffer_length   

            status = fbe_eses_enclosure_send_scsi_cmd(eses_enclosure, packet);
            
            break;

        case FBE_BASE_ENCLOSURE_CONTROL_CODE_RESUME_WRITE:
            status = fbe_payload_control_build_operation(new_control_operation,  // control operation
                                    FBE_ESES_CTRL_OPCODE_WRITE_RESUME,  // opcode
                                    eses_page_op, // buffer
                                    sizeof(fbe_enclosure_mgmt_ctrl_op_t));   // buffer_length   
           
            /* send the command to set the trace buf info in the EMC specific control page. */
            status = fbe_eses_enclosure_send_scsi_cmd(eses_enclosure, packet);              
            break;

         case FBE_BASE_ENCLOSURE_CONTROL_CODE_READ_BUF:        
            read_buf_cmd_p = &eses_page_op->cmd_buf.read_buf;
         
            switch(read_buf_cmd_p->mode)
            {
                case FBE_ESES_READ_BUF_MODE_DATA:                 
                case FBE_ESES_READ_BUF_MODE_DESC:
                    status = fbe_payload_control_build_operation(new_control_operation,  // control operation
                                    FBE_ESES_CTRL_OPCODE_READ_BUF,  // opcode
                                    eses_page_op, // buffer
                                    sizeof(fbe_enclosure_mgmt_ctrl_op_t));   // buffer_length  
                    /* send the command to read buffer iin the specified mode. */
                    status = fbe_eses_enclosure_send_scsi_cmd(eses_enclosure, packet);
                    break;

                default:
                    /* not handled */
                    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                        FBE_TRACE_LEVEL_WARNING,
                                        fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                        "%s, unhandled ctrl_code: 0x%x.\n", 
                                        __FUNCTION__, ctrl_code);

                    fbe_payload_control_set_status(new_control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
                    fbe_transport_complete_packet(packet);
                    break;
            }// End of - switch(eses_enclosure->read_buf.mode)
            
            break;  
      
         case FBE_BASE_ENCLOSURE_CONTROL_CODE_RAW_RCV_DIAG:
            status = fbe_payload_control_build_operation(new_control_operation,  // control operation
                                    FBE_ESES_CTRL_OPCODE_RAW_RCV_DIAG,  // opcode
                                    eses_page_op, // buffer
                                    sizeof(fbe_enclosure_mgmt_ctrl_op_t));   // buffer_length   
           
            /* send the command to set the trace buf info in the EMC specific control page. */
            status = fbe_eses_enclosure_send_scsi_cmd(eses_enclosure, packet);              
            break;

         case FBE_BASE_ENCLOSURE_CONTROL_CODE_RAW_INQUIRY:
            status = fbe_payload_control_build_operation(new_control_operation,  // control operation
                                    FBE_ESES_CTRL_OPCODE_RAW_INQUIRY,   // opcode
                                    eses_page_op, // buffer
                                    sizeof(fbe_enclosure_mgmt_ctrl_op_t));   // buffer_length   
           
            /* send the command to set the trace buf info in the EMC specific control page. */
            status = fbe_eses_enclosure_send_scsi_cmd(eses_enclosure, packet);              
            break;

         case FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_ENCLOSURE_STATISTICS:
             // build the operation
             status = fbe_payload_control_build_operation(new_control_operation,             // control operation
                                    FBE_ESES_CTRL_OPCODE_EMC_STATISTICS_STATUS_CMD, // eses control opcode
                                    eses_page_op,                                   // the control op struct
                                    sizeof(fbe_enclosure_mgmt_ctrl_op_t));          // control op size   
             // send the command
             status = fbe_eses_enclosure_send_scsi_cmd(eses_enclosure, packet);
             break;

         case FBE_BASE_ENCLOSURE_CONTROL_CODE_EXPANDER_THRESHOLD_IN_CONTROL:
             // build the operation
             status = fbe_payload_control_build_operation(new_control_operation,             // control operation
                                    FBE_ESES_CTRL_OPCODE_THRESHOLD_IN_CMD, // eses control opcode
                                    eses_page_op,                                   // the control op struct
                                    sizeof(fbe_enclosure_mgmt_ctrl_op_t));          // control op size   
             // send the command
             status = fbe_eses_enclosure_send_scsi_cmd(eses_enclosure, packet);
             break;

        case FBE_BASE_ENCLOSURE_CONTROL_CODE_SPS_COMMAND:
            // build the operation
            status = fbe_payload_control_build_operation(new_control_operation,             // control operation
                                                         FBE_ESES_CTRL_OPCODE_SPS_IN_BUF_CMD, // eses control opcode
                                                         eses_page_op,                                   // the control op struct
                                                         sizeof(fbe_enclosure_mgmt_ctrl_op_t));          // control op size   
            // send the command
            status = fbe_eses_enclosure_send_scsi_cmd(eses_enclosure, packet);
            break;

        case FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_SPS_MANUF_INFO:
            // build the operation
            status = fbe_payload_control_build_operation(new_control_operation,             // control operation
                                                         FBE_ESES_CTRL_OPCODE_SPS_EEPROM_CMD, // eses control opcode
                                                         eses_page_op,                                   // the control op struct
                                                         sizeof(fbe_enclosure_mgmt_ctrl_op_t));          // control op size   
            // send the command
            status = fbe_eses_enclosure_send_scsi_cmd(eses_enclosure, packet);
            break;

         default:
            /* not supported */
            fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                FBE_TRACE_LEVEL_WARNING,
                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                "%s, unsupported ctrl_code: 0x%x.\n", 
                                 __FUNCTION__, ctrl_code);

            fbe_payload_control_set_status(new_control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
            fbe_transport_complete_packet(packet);
            break;
    }

    return FBE_STATUS_OK;
    
} // fbe_eses_enclosure_repack_ctrl_op

/*!*************************************************************************
* @fn fbe_eses_enclosure_repack_ctrl_op_completion(fbe_packet_t * packet, 
                                           fbe_packet_completion_context_t context)
***************************************************************************
*
* @brief
*       This is the completion routine for "process page control op".
*       It copies the control operation status and status qualifier to the
*       previous control operation allocated by the user.
*
* @param      packet - The pointer to the fbe_packet_t.
* @param      context - The completion context.
*
* @return     fbe_status_t.
*       Always returns FBE_STATUS_OK.
*
* NOTES
*
* HISTORY
*   09-Dec-2008 PHE - Created.
***************************************************************************/
static fbe_status_t 
fbe_eses_enclosure_repack_ctrl_op_completion(fbe_packet_t * packet, 
                                           fbe_packet_completion_context_t context)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_eses_enclosure_t * eses_enclosure = NULL;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t   * curr_control_operation = NULL;
    fbe_payload_control_operation_t   * prev_control_operation = NULL;
    fbe_payload_control_status_t   curr_ctrl_status = FBE_PAYLOAD_CONTROL_STATUS_FAILURE;
    fbe_payload_control_status_qualifier_t   curr_ctrl_status_qualifier = FBE_ENCLOSURE_STATUS_OK;

    /********
     * BEGIN
     ********/

    eses_enclosure = (fbe_eses_enclosure_t*)context;


    status = fbe_transport_get_status_code(packet);    

    /* We HAVE to check status of packet before we continue */
    if(status == FBE_STATUS_OK)
    {
        payload = fbe_transport_get_payload_ex(packet);
         if(payload == NULL)
         {
             fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                            FBE_TRACE_LEVEL_ERROR,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                            "%s, get_payload failed.\n", __FUNCTION__); 
             return FBE_STATUS_GENERIC_FAILURE;
         }

        // Get the current control operation.
        curr_control_operation = fbe_payload_ex_get_control_operation(payload);
        if(curr_control_operation == NULL)
        {
            fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                FBE_TRACE_LEVEL_ERROR,
                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                "%s, get_control_operation failed.\n", __FUNCTION__); 
            
        }
        else
        {
            fbe_payload_control_get_status(curr_control_operation, &curr_ctrl_status);
            fbe_payload_control_get_status_qualifier(curr_control_operation, &curr_ctrl_status_qualifier);

            // Get the previous control operation from the user.
            prev_control_operation = fbe_payload_ex_get_prev_control_operation(payload);
            if(prev_control_operation == NULL)
            {
                fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                    FBE_TRACE_LEVEL_ERROR,
                                    fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                    "%s, get_prev_control_operation failed.\n", 
                                    __FUNCTION__); 
                
            }
            else
            {
                /*
                * Set the status and the status qualifier of the previous control operation allocated by
                * by the user to the status and the status qualifier of the current control operation.
                * The user can decide whether the request needs to be retried 
                * based on the status and the status qualifier.
                */
                fbe_payload_control_set_status(prev_control_operation, curr_ctrl_status);
                fbe_payload_control_set_status_qualifier(prev_control_operation, curr_ctrl_status_qualifier);
            }              

            //Release the control operation.
            fbe_payload_ex_release_control_operation(payload, curr_control_operation);
        }
    } 
    else
    {
        /* Something bad happen to child */
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                            FBE_TRACE_LEVEL_ERROR,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                            "%s, fbe_transport_get_status_code failed, status: 0x%x.\n", 
                            __FUNCTION__, status); 
        
    }

    return FBE_STATUS_OK;
}// End of - fbe_eses_enclosure_repack_ctrl_op_completion


/*!*************************************************************************
* @fn fbe_eses_enclosure_saveControlPageInfo(                  
*                    fbe_eses_enclosure_t * eses_enclosure)                  
***************************************************************************
*
* @brief
*       This function saves the control info into edal.
*
*   PARAMETERS
*       @param      fbe_eses_enclosure_t - The pointer to the ESES enclosure.
*       @param       - pointer to EDAL data
*
*   RETURN VALUE
*       @return     fbe_bool_t - TRUE, if can send.
*
* NOTES
*
* HISTORY
*   04-Nov-2008     Joe Perry - Created.
***************************************************************************/
fbe_enclosure_status_t 
fbe_eses_enclosure_saveControlPageInfo(fbe_eses_enclosure_t * eses_enclosure,
                                  void *newControlInfoPtr)
{
    fbe_enclosure_status_t                  encl_status = FBE_ENCLOSURE_STATUS_CMD_FAILED;
    fbe_edal_status_t                       edalStatus;
    fbe_u32_t                               index;
    fbe_bool_t                              writeDataFound;
    fbe_u8_t                                elemIndex;
    const fbe_eses_enclosure_comp_type_hdlr_t         *handler;
    fbe_u32_t                               componentOverallStatus;
    fbe_u32_t                               maxNumberOfComponents;
    fbe_enclosure_component_block_t         *enclBlockPtr;
    fbe_enclosure_component_types_t         componentType;



    enclBlockPtr = (fbe_enclosure_component_block_t *)newControlInfoPtr;

    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                        FBE_TRACE_LEVEL_DEBUG_HIGH,
                        fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                        "%s, entry \n", __FUNCTION__); 

    for (componentType = 0; componentType < FBE_ESES_ENCLOSURE_MAX_COMPONENT_TYPE; componentType++)
    {
        // check if there is any Write/Control data for this component type
        edalStatus = fbe_edal_getComponentOverallStatus(enclBlockPtr,
                                                        componentType,
                                                        &componentOverallStatus);
        if ((edalStatus == FBE_EDAL_STATUS_OK) &&
            (componentOverallStatus == FBE_ENCL_COMPONENT_TYPE_STATUS_WRITE_NEEDED))
        {
            /* record the overall status */
            edalStatus = fbe_base_enclosure_edal_setOverallStatus((fbe_base_enclosure_t *)eses_enclosure,
                                                                componentType,
                                                                componentOverallStatus);
            if (edalStatus != FBE_EDAL_STATUS_OK)
            {
                continue;
            }
            // loop through all components
            edalStatus = fbe_edal_getSpecificComponentCount(enclBlockPtr,
                                                            componentType,
                                                            &maxNumberOfComponents);
            if (edalStatus != FBE_EDAL_STATUS_OK)
            {
                continue;
            }
            for (index = 0; index < maxNumberOfComponents; index++)
            {
                // check if this component has Write/Control data
                edalStatus = fbe_edal_getBool(enclBlockPtr,
                                            FBE_ENCL_COMP_WRITE_DATA,
                                            componentType,
                                            index,
                                            &writeDataFound);
                if ((edalStatus == FBE_EDAL_STATUS_OK) && writeDataFound)
                {
                    // Get the element index for the specified component
                    encl_status = fbe_base_enclosure_edal_getU8_thread_safe((fbe_base_enclosure_t *)eses_enclosure,
                                                                FBE_ENCL_COMP_ELEM_INDEX,
                                                                componentType,
                                                                index,
                                                                &elemIndex);
                    if (encl_status == FBE_ENCLOSURE_STATUS_OK)
                    {
                        // copy Control info to enclosure's Status (call the correct handler)
                        handler = fbe_eses_enclosure_get_comp_type_hdlr(fbe_eses_enclosure_comp_type_hdlr_table, 
                                                                   componentType);
                        // It could be possible that we did not implement the status extraction method
                        // for the particular element type. It shoud not be considered as a fault.
                        if((handler != NULL) && (handler->copy_control != NULL))
                        {
                            encl_status = handler->copy_control(eses_enclosure, 
                                                                index,
                                                                componentType,
                                                                newControlInfoPtr);
                            if (encl_status != FBE_ENCLOSURE_STATUS_OK)
                            {
                                fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                                    FBE_TRACE_LEVEL_ERROR,
                                                    fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                                    "%s, copyControlHandler failed, encl_status: 0x%x.\n",
                                                    __FUNCTION__, encl_status);
                                
                                continue;
                            }
                        }
                        else
                        {
                            fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                                FBE_TRACE_LEVEL_ERROR,
                                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                                "%s, handler pointer NULL\n",
                                                __FUNCTION__);
                            
                            encl_status = FBE_ENCLOSURE_STATUS_ENCL_FUNC_UNSUPPORTED;
                            continue;
                        }

                    }
                    else
                    {
                        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                            FBE_TRACE_LEVEL_ERROR,
                                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                           "%s, can't get compElemIndex, enclStatus: 0x%x.\n",
                                           __FUNCTION__, encl_status);
                    }

                }

            }   // for index

        }

    }   // for componentType


    return FBE_ENCLOSURE_STATUS_OK;

}   // end of fbe_eses_enclosure_saveControlPageInfo


/*!*************************************************************************
*   @fn fbe_eses_enclosure_array_dev_slot_copy_control()                  
***************************************************************************
*
* DESCRIPTION
*       @brief
*       This function will copy Control/Write attributes to the 
*       enclosure's Status/Read EDAL data.
*
* PARAMETERS
*       @param enclosure - The pointer to the fbe_eses_enclosure_t.
*       @param index - index to a specific component
*       @param compType - EDAL component type 
*       @param controlDataPtr - pointer to the Control EDAL data
*
* RETURN VALUES
*       @return fbe_status_t.
*
* NOTES
*
* HISTORY
*   25-Nov-2008     Joe Perry - Created.
***************************************************************************/
fbe_enclosure_status_t 
fbe_eses_enclosure_array_dev_slot_copy_control(void *enclosure,
                                               fbe_u32_t index,
                                               fbe_u32_t compType,
                                               void *controlDataPtr)
{
    fbe_enclosure_status_t                  enclStatus;
    fbe_edal_status_t                       edalStatus;
    fbe_base_enclosure_t                    *baseEnclPtr;
    fbe_bool_t                              turnOnFaultLed, markComponent;

    baseEnclPtr = (fbe_base_enclosure_t *)enclosure;

    fbe_base_object_customizable_trace((fbe_base_object_t*)enclosure,
                        FBE_TRACE_LEVEL_DEBUG_HIGH,
                        fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)enclosure),
                        "%s entry, index %d\n", __FUNCTION__, index);  
    /*
     * Read Control/Write attributes from control data & copy them to enclosure
     */
    // process Fault LED control
    edalStatus = fbe_edal_getBool(controlDataPtr,
                                  FBE_ENCL_COMP_TURN_ON_FAULT_LED,
                                  FBE_ENCL_DRIVE_SLOT,
                                  index,
                                  &turnOnFaultLed);

    if(!EDAL_STAT_OK(edalStatus))
    {
        return FBE_ENCLOSURE_STATUS_EDAL_FAILED;
    }
    else
    {
        enclStatus = fbe_base_enclosure_edal_setBool_thread_safe(baseEnclPtr,
                                                     FBE_ENCL_COMP_TURN_ON_FAULT_LED,
                                                     FBE_ENCL_DRIVE_SLOT,
                                                     index,
                                                     turnOnFaultLed);
        if(ENCL_STAT_OK(enclStatus))
        {
            enclStatus = FBE_ENCLOSURE_STATUS_OK;
        }
    }

    // process Mark/Identify control
    edalStatus = fbe_edal_getBool(controlDataPtr,
                                  FBE_ENCL_COMP_MARK_COMPONENT,
                                  FBE_ENCL_DRIVE_SLOT,
                                  index,
                                  &markComponent);

    if(!EDAL_STAT_OK(edalStatus))
    {
        return FBE_ENCLOSURE_STATUS_EDAL_FAILED;
    }
    else
    {
        enclStatus = fbe_base_enclosure_edal_setBool_thread_safe(baseEnclPtr,
                                                     FBE_ENCL_COMP_MARK_COMPONENT,
                                                     FBE_ENCL_DRIVE_SLOT,
                                                     index,
                                                     markComponent);
        if(ENCL_STAT_OK(enclStatus))
        {
            enclStatus = FBE_ENCLOSURE_STATUS_OK;
        }
    }

    // set the Write Data flag

    return enclStatus; 

}   // end of fbe_eses_enclosure_array_dev_slot_copy_control


/*!*************************************************************************
*   @fn fbe_eses_enclosure_exp_phy_copy_control()                  
***************************************************************************
*
* DESCRIPTION
*       @brief
*       This function will copy Control/Write attributes to the 
*       enclosure's Status/Read EDAL data.
*
* PARAMETERS
*       @param enclosure - The pointer to the fbe_eses_enclosure_t.
*       @param index - index to a specific component
*       @param compType - EDAL component type 
*       @param controlDataPtr - pointer to the Control EDAL data
*
* RETURN VALUES
*       @return fbe_status_t.
*
* NOTES
*
* HISTORY
*   25-Nov-2008     Joe Perry - Created.
***************************************************************************/
fbe_enclosure_status_t 
fbe_eses_enclosure_exp_phy_copy_control(void *enclosure,
                                        fbe_u32_t index,
                                        fbe_u32_t compType,
                                        void *controlDataPtr)
{
    fbe_enclosure_status_t                  enclStatus;
    fbe_edal_status_t                       edalStatus;
    fbe_base_enclosure_t                    *baseEnclPtr;
    fbe_bool_t                              disablePhy;

    baseEnclPtr = (fbe_base_enclosure_t *)enclosure;

    fbe_base_object_customizable_trace((fbe_base_object_t*)enclosure,
                        FBE_TRACE_LEVEL_DEBUG_HIGH,
                        fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)enclosure),
                        "%s entry, index %d\n", __FUNCTION__, index);  

    /*
     * Read Control/Write attributes from control data & copy them to enclosure
     */
    // process PHY Disable
    edalStatus = fbe_edal_getBool(controlDataPtr,
                                  FBE_ENCL_EXP_PHY_DISABLE,
                                  FBE_ENCL_EXPANDER_PHY,
                                  index,
                                  &disablePhy);
    if (edalStatus == FBE_EDAL_STATUS_OK)
    {
        enclStatus = fbe_base_enclosure_edal_setBool_thread_safe(baseEnclPtr,
                                                     FBE_ENCL_EXP_PHY_DISABLE,
                                                     FBE_ENCL_EXPANDER_PHY,
                                                     index,
                                                     disablePhy);
    }
    else
    {
        return FBE_ENCLOSURE_STATUS_EDAL_FAILED;
    }

    // set the Write Data flag

    return enclStatus; 

}   // end of fbe_eses_enclosure_exp_phy_copy_control

/*!*************************************************************************
*   @fn fbe_eses_enclosure_connector_copy_control()                  
***************************************************************************
*
* DESCRIPTION
*       @brief
*       This function will copy Control/Write attributes to the 
*       enclosure's Status/Read EDAL data.
*
* PARAMETERS
*       @param enclosure - The pointer to the fbe_eses_enclosure_t.
*       @param index - index to a specific component
*       @param compType - EDAL component type 
*       @param controlDataPtr - pointer to the Control EDAL data
*
* RETURN VALUES
*       @return fbe_status_t.
*
* NOTES
*
* HISTORY
*   4-Jun-2001     D. McFarland - Created.
***************************************************************************/

fbe_enclosure_status_t 
fbe_eses_enclosure_connector_copy_control(void *enclosure,
                                          fbe_u32_t index,
                                          fbe_u32_t compType,
                                          void *controlDataPtr)
{
    fbe_enclosure_status_t                  enclStatus;
    fbe_edal_status_t                       edalStatus;
    fbe_base_enclosure_t                    *baseEnclPtr;
    fbe_bool_t                              turnOnMezConnectorLed, markMezConnectorComponent;

    baseEnclPtr = (fbe_base_enclosure_t *)enclosure;

    fbe_base_object_customizable_trace((fbe_base_object_t*)enclosure,
                        FBE_TRACE_LEVEL_DEBUG_HIGH,
                        fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)enclosure),
                        "%s entry, index %d\n", __FUNCTION__, index);  
    /*
     * Read Control/Write attributes from control data & copy them to enclosure
     */
    // process Mezzanine Connector LED control
    edalStatus = fbe_edal_getBool(controlDataPtr,
                                  FBE_ENCL_COMP_TURN_ON_FAULT_LED,
                                  FBE_ENCL_CONNECTOR,
                                  index,
                                  &turnOnMezConnectorLed);

    if(!EDAL_STAT_OK(edalStatus))
    {
        return FBE_ENCLOSURE_STATUS_EDAL_FAILED;
    }
    else
    {
        enclStatus = fbe_base_enclosure_edal_setBool_thread_safe(baseEnclPtr,
                                                     FBE_ENCL_COMP_TURN_ON_FAULT_LED,
                                                     FBE_ENCL_CONNECTOR,
                                                     index,
                                                     turnOnMezConnectorLed);
        if(ENCL_STAT_OK(enclStatus))
        {
            enclStatus = FBE_ENCLOSURE_STATUS_OK;
        }
    }

    // process Mezzanine Connector Mark/Identify control
    edalStatus = fbe_edal_getBool(controlDataPtr,
                                  FBE_ENCL_COMP_MARK_COMPONENT,
                                  FBE_ENCL_CONNECTOR,
                                  index,
                                  &markMezConnectorComponent);

    if(!EDAL_STAT_OK(edalStatus))
    {
        return FBE_ENCLOSURE_STATUS_EDAL_FAILED;
    }
    else
    {
        enclStatus = fbe_base_enclosure_edal_setBool_thread_safe(baseEnclPtr,
                                                     FBE_ENCL_COMP_MARK_COMPONENT,
                                                     FBE_ENCL_CONNECTOR,
                                                     index,
                                                     markMezConnectorComponent);
        if(ENCL_STAT_OK(enclStatus))
        {
            enclStatus = FBE_ENCLOSURE_STATUS_OK;
        }
    }

    return enclStatus; 
}

fbe_enclosure_status_t 
fbe_eses_enclosure_ps_copy_control(void *enclosure,
                                   fbe_u32_t index,
                                   fbe_u32_t compType,
                                   void *controlDataPtr)
{
    fbe_base_object_customizable_trace((fbe_base_object_t*)enclosure,
                        FBE_TRACE_LEVEL_DEBUG_HIGH,
                        fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)enclosure),
                        "%s entry \n", __FUNCTION__ );  
    
    /*TODO*/
    return FBE_ENCLOSURE_STATUS_OK; 

}

fbe_enclosure_status_t 
fbe_eses_enclosure_cooling_copy_control(void *enclosure,
                                        fbe_u32_t index,
                                        fbe_u32_t compType,
                                        void *controlDataPtr)
{

    fbe_base_object_customizable_trace((fbe_base_object_t*)enclosure,
                        FBE_TRACE_LEVEL_DEBUG_HIGH,
                        fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)enclosure),
                        "%s entry \n", __FUNCTION__ );  
    
    /*TODO*/
    return FBE_ENCLOSURE_STATUS_OK; 

}

/*!*************************************************************************
*   @fn fbe_eses_enclosure_encl_copy_control()                  
***************************************************************************
*
* DESCRIPTION
*       @brief
*       This function will copy Control/Write attributes to the 
*       enclosure's Status/Read EDAL data.
*
* PARAMETERS
*       @param enclosure - The pointer to the fbe_eses_enclosure_t.
*       @param index - index to a specific component
*       @param compType - EDAL component type 
*       @param controlDataPtr - pointer to the Control EDAL data
*
* RETURN VALUES
*       @return fbe_status_t.
*
* NOTES
*
* HISTORY
*   25-Nov-2008     Joe Perry - Created.
***************************************************************************/
fbe_enclosure_status_t 
fbe_eses_enclosure_encl_copy_control(void *enclosure,
                                     fbe_u32_t index,
                                     fbe_u32_t compType,
                                     void *controlDataPtr)
{
    fbe_enclosure_status_t                  enclStatus;
    fbe_edal_status_t                       edalStatus;
    fbe_base_enclosure_t                    *baseEnclPtr;
    fbe_bool_t                              turnOnLed, markComponent;

    baseEnclPtr = (fbe_base_enclosure_t *)enclosure;


    fbe_base_object_customizable_trace((fbe_base_object_t*)enclosure,
                        FBE_TRACE_LEVEL_DEBUG_HIGH,
                        fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)enclosure),
                        "%s entry, index %d, compType %s\n", 
                        __FUNCTION__, index,
                        enclosure_access_printComponentType(compType));

    /*
     * Read Control/Write attributes from control data & copy them to enclosure
     */
    // process Fault LED control
    edalStatus = fbe_edal_getBool(controlDataPtr,
                                  FBE_ENCL_COMP_TURN_ON_FAULT_LED,
                                  compType,
                                  index,
                                  &turnOnLed);
    if (edalStatus == FBE_EDAL_STATUS_OK)
    {
        enclStatus = fbe_base_enclosure_edal_setBool_thread_safe(baseEnclPtr,
                                                     FBE_ENCL_COMP_TURN_ON_FAULT_LED,
                                                     compType,
                                                     index,
                                                     turnOnLed);
    }
    else
    {
        return FBE_ENCLOSURE_STATUS_EDAL_FAILED;
    }

    // process Mark
    edalStatus = fbe_edal_getBool(controlDataPtr,
                                  FBE_ENCL_COMP_MARK_COMPONENT,
                                  compType,
                                  index,
                                  &markComponent);
    if (edalStatus == FBE_EDAL_STATUS_OK)
    {
        enclStatus = fbe_base_enclosure_edal_setBool_thread_safe(baseEnclPtr,
                                                     FBE_ENCL_COMP_MARK_COMPONENT,
                                                     compType,
                                                     index,
                                                     markComponent);
    }
    else
    {
        return FBE_ENCLOSURE_STATUS_EDAL_FAILED;
    }

    // set the Write Data flag

    return enclStatus; 

}   // end of fbe_eses_enclosure_encl_copy_control


fbe_enclosure_status_t 
fbe_eses_enclosure_temp_sensor_copy_control(void *enclosure,
                                            fbe_u32_t index,
                                            fbe_u32_t compType,
                                            void *controlDataPtr)
{
    fbe_base_object_customizable_trace((fbe_base_object_t*)enclosure,
                        FBE_TRACE_LEVEL_DEBUG_HIGH,
                        fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)enclosure),
                        "%s entry \n", __FUNCTION__);  
    
    /*TODO*/
    return FBE_ENCLOSURE_STATUS_OK; 

}


/*!*************************************************************************
*   @fn fbe_eses_enclosure_display_copy_control()                  
***************************************************************************
*
* DESCRIPTION
*       @brief
*       This function will copy Control/Write attributes to the 
*       enclosure's Status/Read EDAL data.
*
* PARAMETERS
*       @param enclosure - The pointer to the fbe_eses_enclosure_t.
*       @param index - index to a specific component
*       @param compType - EDAL component type 
*       @param controlDataPtr - pointer to the Control EDAL data
*
* RETURN VALUES
*       @return fbe_status_t.
*
* NOTES
*
* HISTORY
*   18-Dec-2008     Joe Perry - Created.
***************************************************************************/
fbe_enclosure_status_t 
fbe_eses_enclosure_display_copy_control(void *enclosure,
                                        fbe_u32_t index,
                                        fbe_u32_t compType,
                                        void *controlDataPtr)
{
    fbe_enclosure_status_t                  enclStatus;
    fbe_edal_status_t                       edalStatus;
    fbe_base_enclosure_t                    *baseEnclPtr;
    fbe_u8_t                                displayMode;
    fbe_u8_t                                displayChar;
    fbe_bool_t                              displayMarked;

    baseEnclPtr = (fbe_base_enclosure_t *)enclosure;


    fbe_base_object_customizable_trace((fbe_base_object_t*)enclosure,
                        FBE_TRACE_LEVEL_DEBUG_HIGH,
                        fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)enclosure),
                        "%s entry, index %d, compType %s\n", 
                        __FUNCTION__,  index,
                        enclosure_access_printComponentType(compType)); 

    /*
     * Read Control/Write attributes from control data & copy them to enclosure
     */
    // process Display Mode
    edalStatus = fbe_edal_getU8(controlDataPtr,
                                FBE_DISPLAY_MODE,
                                FBE_ENCL_DISPLAY,
                                index,
                                &displayMode);
    if (edalStatus == FBE_EDAL_STATUS_OK)
    {
        enclStatus = fbe_base_enclosure_edal_setU8_thread_safe(baseEnclPtr,
                                                   FBE_DISPLAY_MODE,
                                                   FBE_ENCL_DISPLAY,
                                                   index,
                                                   displayMode);
    }
    else
    {
        return FBE_ENCLOSURE_STATUS_EDAL_FAILED;
    }

    // process Display Character
    edalStatus = fbe_edal_getU8(controlDataPtr,
                                FBE_DISPLAY_CHARACTER,
                                FBE_ENCL_DISPLAY,
                                index,
                                &displayChar);
    if (edalStatus == FBE_EDAL_STATUS_OK)
    {
        // Temporary trace to track down DIMS 239447
        if (displayChar == 0 || displayMode == 0)
        {
            fbe_base_object_customizable_trace((fbe_base_object_t*)enclosure,
                        FBE_TRACE_LEVEL_ERROR,  
                        fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)enclosure),
                        "%s DISPLAY ERROR: copy_control Mode is 0x%x Display char is 0x%x\n", 
                        __FUNCTION__, displayMode, displayChar); 
        }
        enclStatus = fbe_base_enclosure_edal_setU8_thread_safe(baseEnclPtr,
                                                   FBE_DISPLAY_CHARACTER,
                                                   FBE_ENCL_DISPLAY,
                                                   index,
                                                   displayChar);
    }
    else
    {
        return FBE_ENCLOSURE_STATUS_EDAL_FAILED;
    }

    // process Display Marked
    edalStatus = fbe_edal_getBool(controlDataPtr,
                                  FBE_ENCL_COMP_MARK_COMPONENT,
                                  FBE_ENCL_DISPLAY,
                                  index,
                                  &displayMarked);
    if (edalStatus == FBE_EDAL_STATUS_OK)
    {
        enclStatus = fbe_base_enclosure_edal_setBool_thread_safe(baseEnclPtr,
                                                     FBE_ENCL_COMP_MARK_COMPONENT,
                                                     FBE_ENCL_DISPLAY,
                                                     index,
                                                     displayMarked);
    }
    else
    {
        return FBE_ENCLOSURE_STATUS_EDAL_FAILED;
    }

    return enclStatus; 

}   // end of fbe_eses_enclosure_display_copy_control

/*!*************************************************************************
*   @fn fbe_eses_enclosure_formatDriveLedInfo()                  
***************************************************************************
*
* DESCRIPTION
*       @brief
*       This function will 
*
* PARAMETERS
*       @param eses_enclosure - The pointer to the fbe_eses_enclosure_t.
*       @param ledAction - type of LED action to perform
*
* RETURN VALUES
*       @return fbe_enclosure_status_t.
*
* NOTES
*
* HISTORY
*   25-Nov-2008     Joe Perry - Created.
***************************************************************************/
fbe_enclosure_status_t 
fbe_eses_enclosure_formatDriveLedInfo(fbe_eses_enclosure_t * eses_enclosure,
                                      fbe_eses_led_actions_t ledAction)
{
    fbe_enclosure_status_t      enclStatus;
    fbe_enclosure_status_t      returnEnclStatus = FBE_ENCLOSURE_STATUS_OK;
    fbe_bool_t                  markDrive = FALSE;
    fbe_u32_t                   index;
    fbe_u32_t                   maxNumberOfDrives;

    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                        FBE_TRACE_LEVEL_DEBUG_HIGH,
                        fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                        "%s entry \n", __FUNCTION__ );  

    switch (ledAction)
    {
    case FBE_ESES_FLASH_ON_ALL_DRIVES:
        markDrive = TRUE;
        break;

    case FBE_ESES_FLASH_OFF_ALL_DRIVES:
        markDrive = FALSE;
        break;

    default:
        returnEnclStatus = FBE_ENCLOSURE_STATUS_ENCL_FUNC_UNSUPPORTED;
        break;
    }


    if(returnEnclStatus == FBE_ENCLOSURE_STATUS_OK)
    {
        // set bit to flash Drive LED on all drives of this enclosure
        maxNumberOfDrives = fbe_eses_enclosure_get_number_of_slots(eses_enclosure);
    
        for (index = 0; index < maxNumberOfDrives; index++)
        {
             fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                        FBE_TRACE_LEVEL_INFO,
                        fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                        "%s setting marked %d for slot %d\n", __FUNCTION__, markDrive, index );  
            // set bit to turn on/off Flashing
            enclStatus = fbe_base_enclosure_edal_setBool_thread_safe((fbe_base_enclosure_t *)eses_enclosure,
                                                         FBE_ENCL_COMP_MARK_COMPONENT,
                                                         FBE_ENCL_DRIVE_SLOT,
                                                         index,
                                                         markDrive);
            if (enclStatus != FBE_ENCLOSURE_STATUS_OK)
            {
                returnEnclStatus = enclStatus;      // save a bad status to return
            }
            // set bit to force write of Control data
        }
    }

    return returnEnclStatus;

}   // end of fbe_eses_enclosure_formatDriveLedInfo

/*!*************************************************************************
* @fn eses_enclosure_processEnclosureDebugControl(
*                    fbe_eses_enclosure_t * esesEnclosurePtr)
                     fbe_packet_t *packetPtr);
***************************************************************************
*
* @brief
*       This function will process the ENCLOSURE_DEBUG_CONTROL control code.
*
* @param      fbe_eses_enclosure_t - The pointer to the ESES enclosure.
* @param      fbe_packet_t - pointer to control code packet
*
* @return     fbe_status_t - status of processing the control packet
*
* NOTES
*
* HISTORY
*   10-Aug-2012     Lin Lou - Created.
*   31-Jan-2014  PHE - Updated the code to support 8x configuration (two expansion ports). 
***************************************************************************/
static fbe_status_t
eses_enclosure_processEnclosureDebugControl(fbe_eses_enclosure_t *esesEnclosurePtr,
                                            fbe_packet_t *packetPtr)
{
    fbe_payload_control_buffer_length_t         bufferLength = 0;
    fbe_base_object_mgmt_encl_dbg_ctrl_t        *bufferPtr;
    fbe_status_t                                status = FBE_STATUS_OK;
    fbe_enclosure_status_t                      encl_status = FBE_ENCLOSURE_STATUS_OK;
    fbe_bool_t                                  bypass = FBE_FALSE;
    fbe_u8_t                                    connector_id[FBE_ESES_ENCLOSURE_MAX_EXPANSION_PORT_COUNT];
    fbe_u32_t                                   port = 0; // for loop.

    fbe_base_object_customizable_trace((fbe_base_object_t*)esesEnclosurePtr,
                        FBE_TRACE_LEVEL_INFO,
                        fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)esesEnclosurePtr),
                        "%s entry \n", __FUNCTION__);

    bufferLength = sizeof(fbe_base_object_mgmt_encl_dbg_ctrl_t);
    status = fbe_base_enclosure_get_packet_payload_control_data(packetPtr,(fbe_base_enclosure_t*)esesEnclosurePtr,TRUE,(fbe_payload_control_buffer_t *)&bufferPtr,bufferLength);

    if(status == FBE_STATUS_OK)
    {
        switch(bufferPtr->enclDebugAction)
        {
            case FBE_ENCLOSURE_ACTION_NONE:
                fbe_transport_set_status(packetPtr, status, 0);
                fbe_transport_complete_packet(packetPtr);
                return status;

            case FBE_ENCLOSURE_ACTION_INSERT:
                bypass = FBE_FALSE;
                break;

            case FBE_ENCLOSURE_ACTION_REMOVE:
                bypass = FBE_TRUE;
                break;

            default:
                fbe_base_object_customizable_trace((fbe_base_object_t*)esesEnclosurePtr,
                                       FBE_TRACE_LEVEL_ERROR,
                                       fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)esesEnclosurePtr),
                                       "%s, enclDebugAction %d invalid for next enclosure\n",
                                       __FUNCTION__, bufferPtr->enclDebugAction);

                fbe_transport_set_status(packetPtr, FBE_STATUS_GENERIC_FAILURE, 0);
                fbe_transport_complete_packet(packetPtr);
                return FBE_STATUS_GENERIC_FAILURE;
        }

        fbe_base_object_customizable_trace((fbe_base_object_t*)esesEnclosurePtr,
                        FBE_TRACE_LEVEL_INFO,
                        fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)esesEnclosurePtr),
                        "%s, %s.\n", __FUNCTION__, bypass? "REMOVE" : "INSERT");

        for(port = 0; port < FBE_ESES_ENCLOSURE_MAX_EXPANSION_PORT_COUNT; port ++)
        {
            connector_id[port] = FBE_ENCLOSURE_VALUE_INVALID;
        }

        encl_status = fbe_eses_enclosure_get_expansion_port_connector_id(esesEnclosurePtr, &connector_id[0], FBE_ENCLOSURE_MAX_EXPANSION_PORT_COUNT);
        if (encl_status != FBE_ENCLOSURE_STATUS_OK)
        {
            fbe_transport_set_status(packetPtr, FBE_STATUS_GENERIC_FAILURE, 0);
            fbe_transport_complete_packet(packetPtr);
            return status;
        }

        for(port = 0; port < FBE_ESES_ENCLOSURE_MAX_EXPANSION_PORT_COUNT; port++)
        {
            if(connector_id[port] == FBE_ENCLOSURE_VALUE_INVALID)
            {
                break;
            }

            encl_status = eses_enclosure_debug_downstream_encl_removal_insertion(esesEnclosurePtr, 
                                                                                 connector_id[port], 
                                                                                 bypass);

            if(encl_status != FBE_ENCLOSURE_STATUS_OK)
            {
                fbe_base_object_customizable_trace((fbe_base_object_t*)esesEnclosurePtr,
                                       FBE_TRACE_LEVEL_ERROR,
                                       fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)esesEnclosurePtr),
                                       "%s, enclDebugAction %d failed, enclStatus 0x%X.\n",
                                       __FUNCTION__, bufferPtr->enclDebugAction, encl_status);

                break;
            }
        }

        if (encl_status == FBE_ENCLOSURE_STATUS_OK)
        {
            /* Set condition to write out ESES Control */
            status = fbe_lifecycle_set_cond(&fbe_eses_enclosure_lifecycle_const,
                                                (fbe_base_object_t*)esesEnclosurePtr,
                                                FBE_ESES_ENCLOSURE_LIFECYCLE_COND_EXPANDER_CONTROL_NEEDED);

            if (status != FBE_STATUS_OK)
            {
                fbe_base_object_customizable_trace((fbe_base_object_t*)esesEnclosurePtr,
                                    FBE_TRACE_LEVEL_ERROR,
                                    fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)esesEnclosurePtr),
                                    "%s, can't set ControlNeeded condition, status: 0x%x.\n",
                                    __FUNCTION__, status);
            }
        }
    }
    fbe_transport_set_status(packetPtr, status, 0);
    fbe_transport_complete_packet(packetPtr);

    return status;

}   // end of eses_enclosure_processEnclosureDebugControl


/*!*************************************************************************
* @fn eses_enclosure_debug_downstream_encl_removal_insertion(fbe_eses_enclosure_t *esesEnclosurePtr,
*                                                          fbe_u8_t connector_id,
*                                                          fbe_bool_t bypass)
***************************************************************************
*
* @brief
*       This function will process the ENCLOSURE_DEBUG_CONTROL control code.
*
* @param      fbe_eses_enclosure_t - The pointer to the ESES enclosure.
* @param      connector_id - The connector id to the downstream enclosure.
* @param      bypass - removal (FBE_TRUE) and insertion (FBE_FALSE)
*
* @return     fbe_status_t - status of processing the control packet
*
* NOTES
*
* HISTORY
*   10-Aug-2012     Lin Lou - Created.
*   31-Jan-2014     PHE - created a seperate function while adding 8X configuration support for Viking.
***************************************************************************/
static fbe_enclosure_status_t
eses_enclosure_debug_downstream_encl_removal_insertion(fbe_eses_enclosure_t *esesEnclosurePtr,
                                                       fbe_u8_t connector_id,
                                                       fbe_bool_t bypass)
{
    fbe_enclosure_status_t                      encl_status = FBE_ENCLOSURE_STATUS_OK;
    fbe_u32_t                                   matching_conn_index = FBE_ENCLOSURE_VALUE_INVALID;
    fbe_bool_t                                  is_local = FBE_FALSE;

    fbe_base_object_customizable_trace((fbe_base_object_t*)esesEnclosurePtr,
                        FBE_TRACE_LEVEL_INFO,
                        fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)esesEnclosurePtr),
                        "EnclosureDebugControl, %s downsteam encl with connector id %d.\n", bypass? "REMOVE" : "INSERT", connector_id);

    // set up connector control to enable/disable connector phy
    encl_status = fbe_eses_enclosure_setup_connector_control(esesEnclosurePtr, connector_id, bypass);

    if (encl_status != FBE_ENCLOSURE_STATUS_OK)
    {
        return encl_status;
    }

    matching_conn_index =
        fbe_base_enclosure_find_first_edal_match_Bool((fbe_base_enclosure_t *)esesEnclosurePtr,
                                                FBE_ENCL_COMP_IS_LOCAL,  //attribute
                                                FBE_ENCL_CONNECTOR,  // Component type
                                                0, //starting index
                                                FBE_TRUE);

    if(matching_conn_index != FBE_ENCLOSURE_VALUE_INVALID)
    {
        /* There must be expansion port among the local connectors.
         * Second, find the first matching connector id.
         */
        matching_conn_index =
            fbe_base_enclosure_find_first_edal_match_U8((fbe_base_enclosure_t *)esesEnclosurePtr,
                                                    FBE_ENCL_CONNECTOR_ID,  //attribute
                                                    FBE_ENCL_CONNECTOR,  // Component type
                                                    matching_conn_index, //starting index
                                                    connector_id);

        while (matching_conn_index != FBE_ENCLOSURE_VALUE_INVALID)
        {
            encl_status = fbe_base_enclosure_edal_getBool((fbe_base_enclosure_t *)esesEnclosurePtr,
                                                    FBE_ENCL_COMP_IS_LOCAL,  // Attribute.
                                                    FBE_ENCL_CONNECTOR,  // Component type
                                                    matching_conn_index,     // Component index.
                                                    &is_local);

            if(encl_status != FBE_ENCLOSURE_STATUS_OK)
            {
                return encl_status;;
            }
            if (is_local == FBE_FALSE)
            {
                // Only process local side connector.
                break;
            }

            encl_status = fbe_base_enclosure_edal_setBool_thread_safe((fbe_base_enclosure_t *)esesEnclosurePtr,
                                                            FBE_ENCL_CONNECTOR_INSERT_MASKED,
                                                            FBE_ENCL_CONNECTOR,
                                                            matching_conn_index,
                                                            bypass);

            if (encl_status != FBE_ENCLOSURE_STATUS_OK)
            {
                return encl_status;
            }
            /* Find the next matching connector index */

            matching_conn_index =
                fbe_base_enclosure_find_first_edal_match_U8((fbe_base_enclosure_t *)esesEnclosurePtr,
                                                        FBE_ENCL_CONNECTOR_ID,  //attribute
                                                        FBE_ENCL_CONNECTOR,  // Component type
                                                        matching_conn_index + 1, //starting index with next connector.
                                                        connector_id);
        }// while loop
    }

    return encl_status;
}


/*!*************************************************************************
* @fn eses_enclosure_processDriveDebugControl(                  
*                    fbe_eses_enclosure_t * esesEnclosurePtr)                  
                     fbe_packet_t *packetPtr);
***************************************************************************
*
* @brief
*       This function will process the DRIVE_DEBUG_CONTROL control code.
*
* @param      fbe_eses_enclosure_t - The pointer to the ESES enclosure.
* @param      fbe_packet_t - pointer to control code packet
*
* @return     fbe_status_t - status of processing the control packet
*
* NOTES
*
* HISTORY
*   02-Dec-2008     Joe Perry - Created.
***************************************************************************/
static fbe_status_t 
eses_enclosure_processDriveDebugControl(fbe_eses_enclosure_t *esesEnclosurePtr,
                                        fbe_packet_t *packetPtr)
{
    fbe_payload_control_buffer_length_t         bufferLength = 0;
    fbe_base_object_mgmt_drv_dbg_ctrl_t         *bufferPtr;
    fbe_u8_t                                    index;
    fbe_u8_t                                    phyIndex;
    fbe_status_t                                status = FBE_STATUS_OK;
    fbe_enclosure_status_t                      enclStatus;
    fbe_enclosure_status_t                      returnEnclStatus = FBE_ENCLOSURE_STATUS_OK;
    fbe_u8_t                                    slot_num = 0;


    fbe_base_object_customizable_trace((fbe_base_object_t*)esesEnclosurePtr,
                        FBE_TRACE_LEVEL_DEBUG_HIGH,
                        fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)esesEnclosurePtr),
                        "%s entry \n", __FUNCTION__);

    bufferLength = sizeof(fbe_base_object_mgmt_drv_dbg_ctrl_t);
    status =fbe_base_enclosure_get_packet_payload_control_data(packetPtr,(fbe_base_enclosure_t*)esesEnclosurePtr,TRUE,(fbe_payload_control_buffer_t *)&bufferPtr,bufferLength);
     if(status == FBE_STATUS_OK)
     {

    /*
        * Process the buffer and return the packet
     */

    // loop through all the drives & process the action setting
        for (index = 0; index < fbe_eses_enclosure_get_number_of_slots(esesEnclosurePtr); index++)
        {
            enclStatus = fbe_base_enclosure_edal_getU8((fbe_base_enclosure_t *)esesEnclosurePtr,
                                                    FBE_ENCL_DRIVE_SLOT_NUMBER,
                                                    FBE_ENCL_DRIVE_SLOT,
                                                    index,
                                                    &slot_num);
            if(!(ENCL_STAT_OK(enclStatus)))
            {
                fbe_base_object_customizable_trace((fbe_base_object_t *) esesEnclosurePtr, 
                                FBE_TRACE_LEVEL_ERROR,
                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *) esesEnclosurePtr),
                                "%s, Failed to get slot number for drive index:%d.\n", 
                                __FUNCTION__, index);

                fbe_transport_set_status(packetPtr, FBE_STATUS_GENERIC_FAILURE, 0);
                fbe_transport_complete_packet(packetPtr);
                return FBE_STATUS_GENERIC_FAILURE;    
            }
            
            switch (bufferPtr->driveDebugAction[slot_num])
        {
        case FBE_DRIVE_ACTION_NONE:
            break;

        case FBE_DRIVE_ACTION_REMOVE:
             fbe_base_object_customizable_trace((fbe_base_object_t*)esesEnclosurePtr,
                              FBE_TRACE_LEVEL_INFO,
                              fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)esesEnclosurePtr),
                              "%s, drive %d Remove\n", __FUNCTION__, index);
            
            // Disable associated PHY
            enclStatus = fbe_base_enclosure_edal_getU8_thread_safe((fbe_base_enclosure_t *)esesEnclosurePtr,
                                                       FBE_ENCL_DRIVE_PHY_INDEX,
                                                       FBE_ENCL_DRIVE_SLOT,
                                                       index,
                                                       &phyIndex);
            if (enclStatus != FBE_ENCLOSURE_STATUS_OK)
            {
                returnEnclStatus = enclStatus;      // save a bad status to return
            }
            enclStatus = fbe_base_enclosure_edal_setBool_thread_safe((fbe_base_enclosure_t *)esesEnclosurePtr,
                                                         FBE_ENCL_EXP_PHY_DISABLE,
                                                         FBE_ENCL_EXPANDER_PHY,
                                                         phyIndex,
                                                         TRUE);
            if (enclStatus != FBE_ENCLOSURE_STATUS_OK)
            {
                returnEnclStatus = enclStatus;      // save a bad status to return
            }
            // Mask drive Insert
            enclStatus = fbe_base_enclosure_edal_setBool_thread_safe((fbe_base_enclosure_t *)esesEnclosurePtr,
                                                         FBE_ENCL_DRIVE_INSERT_MASKED,
                                                         FBE_ENCL_DRIVE_SLOT,
                                                         index,
                                                         TRUE);
            if (enclStatus != FBE_ENCLOSURE_STATUS_OK)
            {
                returnEnclStatus = enclStatus;      // save a bad status to return
            }
            break;

        case FBE_DRIVE_ACTION_INSERT:
              fbe_base_object_customizable_trace((fbe_base_object_t*)esesEnclosurePtr,
                              FBE_TRACE_LEVEL_INFO,
                              fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)esesEnclosurePtr),
                               "%s, drive %d Insert\n", __FUNCTION__, index);
            
            // Enable associated PHY
            enclStatus = fbe_base_enclosure_edal_getU8_thread_safe((fbe_base_enclosure_t *)esesEnclosurePtr,
                                                       FBE_ENCL_DRIVE_PHY_INDEX,
                                                       FBE_ENCL_DRIVE_SLOT,
                                                       index,
                                                       &phyIndex);
            if (enclStatus != FBE_ENCLOSURE_STATUS_OK)
            {
                returnEnclStatus = enclStatus;      // save a bad status to return
            }
            enclStatus = fbe_base_enclosure_edal_setBool_thread_safe((fbe_base_enclosure_t *)esesEnclosurePtr,
                                                         FBE_ENCL_EXP_PHY_DISABLE,
                                                         FBE_ENCL_EXPANDER_PHY,
                                                         phyIndex,
                                                         FALSE);
            if (enclStatus != FBE_ENCLOSURE_STATUS_OK)
            {
                returnEnclStatus = enclStatus;      // save a bad status to return
            }
            // Unmask drive Insert
            enclStatus = fbe_base_enclosure_edal_setBool_thread_safe((fbe_base_enclosure_t *)esesEnclosurePtr,
                                                         FBE_ENCL_DRIVE_INSERT_MASKED,
                                                         FBE_ENCL_DRIVE_SLOT,
                                                         index,
                                                         FALSE);
            if (enclStatus != FBE_ENCLOSURE_STATUS_OK)
            {
                returnEnclStatus = enclStatus;      // save a bad status to return
            }
            break;
        default:
            fbe_base_object_customizable_trace((fbe_base_object_t*)esesEnclosurePtr,
                                   FBE_TRACE_LEVEL_ERROR,
                                   fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)esesEnclosurePtr),
                                   "%s, index %d, driveDebugAction %d invalid\n", 
                                   __FUNCTION__, index, bufferPtr->driveDebugAction[index]);
            
            fbe_transport_set_status(packetPtr, FBE_STATUS_GENERIC_FAILURE, 0);
            fbe_transport_complete_packet(packetPtr);
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }

// jap - think about error handling
    if (returnEnclStatus == FBE_ENCLOSURE_STATUS_OK)
    {
        /* Set condition to write out ESES Control */
        status = fbe_lifecycle_set_cond(&fbe_eses_enclosure_lifecycle_const, 
                                            (fbe_base_object_t*)esesEnclosurePtr, 
                                            FBE_ESES_ENCLOSURE_LIFECYCLE_COND_EXPANDER_CONTROL_NEEDED);

        if (status != FBE_STATUS_OK) 
        {
            fbe_base_object_customizable_trace((fbe_base_object_t*)esesEnclosurePtr,
                                FBE_TRACE_LEVEL_ERROR,
                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)esesEnclosurePtr),
                                "%s, can't set ControlNeeded condition, status: 0x%x.\n",
                                __FUNCTION__, status);
        }
    }
  }
    fbe_transport_set_status(packetPtr, status, 0);
    fbe_transport_complete_packet(packetPtr);

    return status;

}   // end of eses_enclosure_processDriveDebugControl

/*!*************************************************************************
* @fn eses_enclosure_processDrivePowerControl(                  
*                    fbe_eses_enclosure_t * esesEnclosurePtr)                  
                     fbe_packet_t *packetPtr);
***************************************************************************
*
* @brief
*       This function will process the DRIVE_POWER_CONTROL control code.
*
* @param      fbe_eses_enclosure_t - The pointer to the ESES enclosure.
* @param      fbe_packet_t - pointer to control code packet
*
* @return     fbe_status_t - status of processing the control packet
*
* NOTES
*
* HISTORY
*   02-Dec-2008     Joe Perry - Created.
***************************************************************************/
static fbe_status_t 
eses_enclosure_processDrivePowerControl(fbe_eses_enclosure_t *esesEnclosurePtr,
                                        fbe_packet_t *packetPtr)
{
    fbe_payload_control_buffer_length_t     bufferLength = 0;
    fbe_base_object_mgmt_drv_power_ctrl_t   *bufferPtr;
    fbe_u32_t                               index;
    fbe_enclosure_status_t                  enclStatus;
    fbe_bool_t                              drivePowerCycle = FALSE;
    fbe_bool_t                              driveOtherPowerCtrl = FALSE;
    fbe_status_t                            status= FBE_STATUS_OK;
    fbe_u8_t                                slot_num = 0;
    fbe_bool_t                              logDrivePowerAction = FALSE;
    fbe_u8_t                                deviceStr[FBE_DEVICE_STRING_LENGTH];
    fbe_device_physical_location_t          location = {0};
    fbe_u32_t                               slot_per_bank = 0;
    fbe_port_number_t                       port_num = 0;
    fbe_enclosure_number_t                  enclosure_num = 0;

    fbe_base_object_customizable_trace((fbe_base_object_t *)esesEnclosurePtr,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *) esesEnclosurePtr),
                          "%s entry.\n", __FUNCTION__);

    bufferLength = sizeof(fbe_base_object_mgmt_drv_power_ctrl_t);
    status =fbe_base_enclosure_get_packet_payload_control_data(packetPtr,(fbe_base_enclosure_t*)esesEnclosurePtr,TRUE,(fbe_payload_control_buffer_t *)&bufferPtr,bufferLength);
     if(status == FBE_STATUS_OK)
     {
         fbe_base_enclosure_get_port_number((fbe_base_enclosure_t *)esesEnclosurePtr, &port_num);
         fbe_base_enclosure_get_enclosure_number((fbe_base_enclosure_t *)esesEnclosurePtr, &enclosure_num);
         location.bus = port_num;
         location.enclosure = enclosure_num;

    /* Process the buffer and return the packet */
    // loop through all the drives & process the action setting.
        for (index = 0; index < fbe_eses_enclosure_get_number_of_slots(esesEnclosurePtr); index++)
    {
            enclStatus = fbe_base_enclosure_edal_getU8((fbe_base_enclosure_t *)esesEnclosurePtr,
                                                    FBE_ENCL_DRIVE_SLOT_NUMBER,
                                                    FBE_ENCL_DRIVE_SLOT,
                                                    index,
                                                    &slot_num);
            if(!(ENCL_STAT_OK(enclStatus)))
            {
                fbe_base_object_customizable_trace((fbe_base_object_t *) esesEnclosurePtr, 
                                FBE_TRACE_LEVEL_ERROR,
                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *) esesEnclosurePtr),
                                "%s, Failed to get slot number for drive index:%d.\n", 
                                __FUNCTION__, index);

                fbe_transport_set_status(packetPtr, FBE_STATUS_GENERIC_FAILURE, 0);
                fbe_transport_complete_packet(packetPtr);
                return FBE_STATUS_GENERIC_FAILURE;    
            }

            location.slot = slot_num;
            slot_per_bank = fbe_base_enclosure_get_number_of_slots_per_bank((fbe_base_enclosure_t *)esesEnclosurePtr);
            if (slot_per_bank != 0)
            {
                location.bank = (location.slot / slot_per_bank) + 'A';
                location.bank_slot = location.slot % slot_per_bank;
                fbe_sprintf(&deviceStr[0], sizeof(deviceStr), "Bus %d Enclosure %d Drive %d (%c%d)",
                        location.bus,
                        location.enclosure,
                        location.slot,
                        location.bank,
                        location.bank_slot);
            }
            else
            {
                fbe_sprintf(&deviceStr[0], sizeof(deviceStr), "Bus %d Enclosure %d Drive %d",
                        location.bus,
                        location.enclosure,
                        location.slot);
            }

            logDrivePowerAction = FALSE;

            switch (bufferPtr->drivePowerAction[slot_num])
        {
        
        case FBE_DRIVE_POWER_ACTION_NONE:
            break;

        case FBE_DRIVE_POWER_ACTION_POWER_DOWN:
            fbe_base_object_customizable_trace((fbe_base_object_t *) esesEnclosurePtr, 
                               FBE_TRACE_LEVEL_INFO,
                               fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *) esesEnclosurePtr),
                               "processDrivePowerControl, drive %d Power Down.\n", index);
            // Power Off drive
            enclStatus = fbe_base_enclosure_edal_setBool_thread_safe((fbe_base_enclosure_t *)esesEnclosurePtr,
                                                         FBE_ENCL_DRIVE_DEVICE_OFF,
                                                         FBE_ENCL_DRIVE_SLOT,
                                                         index,
                                                         TRUE);
            if(ENCL_STAT_OK(enclStatus))
            {
                driveOtherPowerCtrl = TRUE;    
            }
            
            logDrivePowerAction = TRUE;
            break;

        case FBE_DRIVE_POWER_ACTION_POWER_UP:
            fbe_base_object_customizable_trace((fbe_base_object_t *) esesEnclosurePtr, 
                               FBE_TRACE_LEVEL_INFO,
                               fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *) esesEnclosurePtr),
                               "processDrivePowerControl, drive %d Power Up.\n", index);
            // Power Up drive
            enclStatus = fbe_base_enclosure_edal_setBool_thread_safe((fbe_base_enclosure_t *)esesEnclosurePtr,
                                                         FBE_ENCL_DRIVE_DEVICE_OFF,
                                                         FBE_ENCL_DRIVE_SLOT,
                                                         index,
                                                         FALSE);
            if(ENCL_STAT_OK(enclStatus))
            {
                driveOtherPowerCtrl = TRUE;  
            }
            
            logDrivePowerAction = TRUE;
            break;

        case FBE_DRIVE_POWER_ACTION_POWER_CYCLE:
            if(bufferPtr->drivePowerCycleDuration[index] > FBE_ESES_DRIVE_POWER_CYCLE_DURATION_MAX)
            {
                fbe_base_object_customizable_trace((fbe_base_object_t *) esesEnclosurePtr, 
                               FBE_TRACE_LEVEL_INFO,
                               fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *) esesEnclosurePtr),
                               "processDrivePowerControl, drive %d Power Cycle, invalid duration %d, expected <= %d.\n",
                               index, 
                               bufferPtr->drivePowerCycleDuration[index],
                               FBE_ESES_DRIVE_POWER_CYCLE_DURATION_MAX);

                fbe_transport_set_status(packetPtr, FBE_STATUS_GENERIC_FAILURE, 0);
                fbe_transport_complete_packet(packetPtr);
                return FBE_STATUS_GENERIC_FAILURE;
            }
            else
            {
                fbe_base_object_customizable_trace((fbe_base_object_t *) esesEnclosurePtr, 
                               FBE_TRACE_LEVEL_INFO,
                               fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *) esesEnclosurePtr),
                               "processDrivePowerControl, drive %d Power Cycle, duration: %d.\n",
                               index, bufferPtr->drivePowerCycleDuration[index]);
            
                fbe_eses_enclosure_set_drive_power_cycle_duration(esesEnclosurePtr, 
                                                        index,
                                                        bufferPtr->drivePowerCycleDuration[index]);
                fbe_eses_enclosure_set_drive_power_cycle_request(esesEnclosurePtr, 
                                                    (fbe_u8_t)index,
                                                    TRUE);

                /* Set the FBE_ENCL_COMP_EMC_ENCL_CTRL_WRITE_DATA attribute to TRUE. The function also
                 * sets FBE_ENCL_COMP_EMC_ENCL_CTRL_WRITE_DATA_SENT to FALSE. 
                 */
                enclStatus = fbe_base_enclosure_edal_setBool_thread_safe((fbe_base_enclosure_t *)esesEnclosurePtr,
                                                            FBE_ENCL_COMP_EMC_ENCL_CTRL_WRITE_DATA,
                                                            FBE_ENCL_DRIVE_SLOT,
                                                            index,
                                                            TRUE);
              
                drivePowerCycle = TRUE;
            }

            logDrivePowerAction = TRUE;
            break;

        default:
            fbe_base_object_customizable_trace((fbe_base_object_t *) esesEnclosurePtr, 
                                   FBE_TRACE_LEVEL_ERROR,
                                   fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *) esesEnclosurePtr),
                                   "processDrivePowerControl, drive %d, invalid drivePowerAction %d.\n", 
                                   index, 
                                   bufferPtr->drivePowerAction[index]);

            fbe_transport_set_status(packetPtr, FBE_STATUS_GENERIC_FAILURE, 0);
            fbe_transport_complete_packet(packetPtr);
            return FBE_STATUS_GENERIC_FAILURE;
        }

            if (logDrivePowerAction == TRUE)
            {
                fbe_event_log_write(PHYSICAL_PACKAGE_INFO_USER_INITIATED_DRIVE_POWER_CTRL_CMD,
                        NULL,0,
                        "%s %s",
                        fbe_eses_enclosure_print_power_action(bufferPtr->drivePowerAction[slot_num]),
                        &deviceStr[0]);
            }
    }

    if(drivePowerCycle)
    {
        /* Set condition to write out EMC enclosure control page. */
        status = fbe_lifecycle_set_cond(&fbe_eses_enclosure_lifecycle_const, 
                                    (fbe_base_object_t*)esesEnclosurePtr, 
                                    FBE_ESES_ENCLOSURE_LIFECYCLE_COND_EMC_SPECIFIC_CONTROL_NEEDED);   

        if (status != FBE_STATUS_OK) 
        {
            fbe_base_object_customizable_trace((fbe_base_object_t*)esesEnclosurePtr,
                                    FBE_TRACE_LEVEL_ERROR,
                                    fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)esesEnclosurePtr),
                                    "%s, can't set EMC_SPECIFIC_CONTROL_NEEDED condition, status: 0x%x.\n",
                                    __FUNCTION__,  status);

            fbe_transport_set_status(packetPtr, status, 0);
            fbe_transport_complete_packet(packetPtr);
            return status;
            
        }
    }

    if(driveOtherPowerCtrl)
    {
       /* Set condition to read the status page for the drive slot status. 
        * The latest status of the power off attribute will be used while building
        * the enclosure control page for drive slot.
        */
        status = fbe_lifecycle_set_cond(&fbe_eses_enclosure_lifecycle_const, 
                                    (fbe_base_object_t*)esesEnclosurePtr, 
                                    FBE_ESES_ENCLOSURE_LIFECYCLE_COND_STATUS_UNKNOWN);  

        if (status != FBE_STATUS_OK) 
        {
            fbe_base_object_customizable_trace((fbe_base_object_t*)esesEnclosurePtr,
                                    FBE_TRACE_LEVEL_ERROR,
                                    fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)esesEnclosurePtr),
                                    "%s, can't set STATUS_UNKNOWN condition, status: 0x%x.\n",
                                    __FUNCTION__,  status);

            fbe_transport_set_status(packetPtr, status, 0);
            fbe_transport_complete_packet(packetPtr);
            return status;
            
        }
    
        /* Set condition to write out enclosure control page. */
        status = fbe_lifecycle_set_cond(&fbe_eses_enclosure_lifecycle_const, 
                                    (fbe_base_object_t*)esesEnclosurePtr, 
                                    FBE_ESES_ENCLOSURE_LIFECYCLE_COND_EXPANDER_CONTROL_NEEDED);

        if (status != FBE_STATUS_OK) 
        {
            fbe_base_object_customizable_trace((fbe_base_object_t*)esesEnclosurePtr,
                                    FBE_TRACE_LEVEL_ERROR,
                                    fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)esesEnclosurePtr),
                                    "%s, can't set EXPANDER_CONTROL_NEEDED condition, status: 0x%x.\n",
                                    __FUNCTION__,  status);

            fbe_transport_set_status(packetPtr, status, 0);
            fbe_transport_complete_packet(packetPtr);
            return status;
            
        }
    }
    }

    fbe_transport_set_status(packetPtr, status, 0);
    fbe_transport_complete_packet(packetPtr);

    return status;

}   // end of eses_enclosure_processDrivePowerControl

/*!*************************************************************************
* @fn eses_enclosure_processPsMarginTestControl(                  
*                    fbe_eses_enclosure_t * esesEnclosurePtr)                  
                     fbe_packet_t *packetPtr);
***************************************************************************
*
* @brief
*       This function will process the PS_MARGIN_TEST_CONTROL control code.
*
* @param      fbe_eses_enclosure_t - The pointer to the ESES enclosure.
* @param      fbe_packet_t - pointer to control code packet
*
* @return     fbe_status_t - status of processing the control packet
*
* NOTES
*
* HISTORY
*   18-Dec-2009     Joe Perry - Created.
***************************************************************************/
static fbe_status_t 
eses_enclosure_processPsMarginTestControl(fbe_eses_enclosure_t *esesEnclosurePtr,
                                          fbe_packet_t *packetPtr)
{
    fbe_payload_control_buffer_length_t     bufferLength = 0;
    fbe_base_enclosure_ps_margin_test_ctrl_t *bufferPtr;
    fbe_u32_t                               index;
    fbe_u8_t                                psMarginTestModeControl = 0;
    fbe_u8_t                                psCount;
    fbe_enclosure_status_t                  enclStatus;
    fbe_bool_t                              psMarginTestControl = FALSE;
    fbe_status_t                            status= FBE_STATUS_OK;


    fbe_base_object_customizable_trace((fbe_base_object_t *)esesEnclosurePtr,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *) esesEnclosurePtr),
                          "%s entry.\n", __FUNCTION__);

    bufferLength = sizeof(fbe_base_enclosure_ps_margin_test_ctrl_t);
    status =fbe_base_enclosure_get_packet_payload_control_data(packetPtr,(fbe_base_enclosure_t*)esesEnclosurePtr,TRUE,(fbe_payload_control_buffer_t *)&bufferPtr,bufferLength);
    if(status == FBE_STATUS_OK)
    {
    
        /* Validate the PS count value */
        status = fbe_base_enclosure_get_number_of_components((fbe_base_enclosure_t *)esesEnclosurePtr,
                                                          FBE_ENCL_POWER_SUPPLY,
                                                          &psCount); 
        if(status != FBE_STATUS_OK)
        {
            fbe_base_object_customizable_trace((fbe_base_object_t *) esesEnclosurePtr, 
                                   FBE_TRACE_LEVEL_ERROR,
                                   fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *) esesEnclosurePtr),
                                   "%s, Failed to get psCount, status: 0x%X.\n", 
                                   __FUNCTION__, status);

            fbe_transport_set_status(packetPtr, FBE_STATUS_GENERIC_FAILURE, 0);
            fbe_transport_complete_packet(packetPtr);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        if (psCount == 0)
        {
            fbe_base_object_customizable_trace((fbe_base_object_t *) esesEnclosurePtr, 
                                   FBE_TRACE_LEVEL_ERROR,
                                   fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *) esesEnclosurePtr),
                                   "%s, No power supplies in enclosure number:%d\n", 
                                   __FUNCTION__, esesEnclosurePtr->properties.number_of_encl);

            fbe_transport_set_status(packetPtr, FBE_STATUS_GENERIC_FAILURE, 0);
            fbe_transport_complete_packet(packetPtr);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        
        /* Process the buffer and return the packet */
        // loop through all the PS's & process the action setting.
        for (index = 0; index < psCount; index++)
        {
            psMarginTestModeControl = 0;
            switch (bufferPtr->psMarginTestAction)
            {
            
            case FBE_PS_MARGIN_TEST_NO_ACTION:
                break;
        
            case FBE_PS_MARGIN_TEST_DISABLE:
                fbe_base_object_customizable_trace((fbe_base_object_t *) esesEnclosurePtr, 
                                   FBE_TRACE_LEVEL_INFO,
                                   fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *) esesEnclosurePtr),
                                   "processPsMarginTestControl, ps %d Disable.\n", index);
                // Disable PS Margining Test
                psMarginTestModeControl = FBE_ESES_MARGINING_TEST_MODE_CTRL_DISABLE_AUTO_TEST;
                enclStatus = fbe_base_enclosure_edal_setU8_thread_safe((fbe_base_enclosure_t *)esesEnclosurePtr,
                                                                       FBE_ENCL_PS_MARGIN_TEST_MODE_CONTROL,
                                                                       FBE_ENCL_POWER_SUPPLY,
                                                                       index,
                                                                       psMarginTestModeControl);
                if(ENCL_STAT_OK(enclStatus))
                {
                    psMarginTestControl = TRUE;    
                }
                break;
        
            case FBE_PS_MARGIN_TEST_ENABLE:
                fbe_base_object_customizable_trace((fbe_base_object_t *) esesEnclosurePtr, 
                                   FBE_TRACE_LEVEL_INFO,
                                   fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *) esesEnclosurePtr),
                                   "processPsMarginTestControl, ps %d Enable.\n", index);
                // Enable PS Margining Test
                psMarginTestModeControl = FBE_ESES_MARGINING_TEST_MODE_CTRL_ENABLE_AUTO_TEST;
                enclStatus = fbe_base_enclosure_edal_setU8_thread_safe((fbe_base_enclosure_t *)esesEnclosurePtr,
                                                                       FBE_ENCL_PS_MARGIN_TEST_MODE_CONTROL,
                                                                       FBE_ENCL_POWER_SUPPLY,
                                                                       index,
                                                                       psMarginTestModeControl);
                if(ENCL_STAT_OK(enclStatus))
                {
                    psMarginTestControl = TRUE;  
                }
                break;
             
            case FBE_PS_MARGIN_TEST_STATUS:    
        
            default:
                fbe_base_object_customizable_trace((fbe_base_object_t *) esesEnclosurePtr, 
                                       FBE_TRACE_LEVEL_ERROR,
                                       fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *) esesEnclosurePtr),
                                       "processPsMarginTestControl, ps %d, invalid psMarginTestMode %d.\n", 
                                       index, 
                                       bufferPtr->psMarginTestAction);
        
                fbe_transport_set_status(packetPtr, FBE_STATUS_GENERIC_FAILURE, 0);
                fbe_transport_complete_packet(packetPtr);
                return FBE_STATUS_GENERIC_FAILURE;
            }
        }
        
        if(psMarginTestControl)
        {
            /* Set condition to write out EMC enclosure control page. */
            status = fbe_lifecycle_set_cond(&fbe_eses_enclosure_lifecycle_const, 
                                        (fbe_base_object_t*)esesEnclosurePtr, 
                                        FBE_ESES_ENCLOSURE_LIFECYCLE_COND_EMC_SPECIFIC_CONTROL_NEEDED);   
        
            if (status != FBE_STATUS_OK) 
            {
                fbe_base_object_customizable_trace((fbe_base_object_t*)esesEnclosurePtr,
                                        FBE_TRACE_LEVEL_ERROR,
                                        fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)esesEnclosurePtr),
                                        "%s, can't set EMC_SPECIFIC_CONTROL_NEEDED condition, status: 0x%x.\n",
                                        __FUNCTION__,  status);
        
                fbe_transport_set_status(packetPtr, status, 0);
                fbe_transport_complete_packet(packetPtr);
                return status;
                
            }
        }
    
    }

    fbe_transport_set_status(packetPtr, status, 0);
    fbe_transport_complete_packet(packetPtr);
    
    return status;
    
}   // end of eses_enclosure_processPsMarginTestControl
   
/*!*************************************************************************
* @fn eses_enclosure_processExpanderControl(                  
*                    fbe_eses_enclosure_t * esesEnclosurePtr)                  
                     fbe_packet_t *packetPtr);
***************************************************************************
*
* @brief
*       This function will process the EXPANDER_CONTROL control code.
*
* @param      fbe_eses_enclosure_t - The pointer to the ESES enclosure.
* @param      fbe_packet_t - pointer to control code packet
*
* @return     fbe_status_t - status of processing the control packet
*
* NOTES
*
* HISTORY
*   02-Dec-2008     Joe Perry - Created.
***************************************************************************/
static fbe_status_t 
eses_enclosure_processExpanderControl(fbe_eses_enclosure_t *esesEnclosurePtr,
                                      fbe_packet_t *packetPtr)
{
    fbe_payload_control_buffer_length_t     bufferLength = 0;
    fbe_base_object_mgmt_exp_ctrl_t         *bufferPtr;
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_enclosure_status_t                  enclStatus = FBE_ENCLOSURE_STATUS_OK;
    fbe_enclosure_status_t                  returnEnclStatus = FBE_ENCLOSURE_STATUS_OK;
    fbe_u32_t                               index;


    fbe_base_object_customizable_trace((fbe_base_object_t*)esesEnclosurePtr,
                           FBE_TRACE_LEVEL_DEBUG_HIGH,
                           fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)esesEnclosurePtr),
                           "%s entry\n", __FUNCTION__);


    bufferLength = sizeof(fbe_base_object_mgmt_exp_ctrl_t);
    status = fbe_base_enclosure_get_packet_payload_control_data(packetPtr,(fbe_base_enclosure_t*)esesEnclosurePtr,TRUE,(fbe_payload_control_buffer_t *)&bufferPtr,bufferLength);
     if(status == FBE_STATUS_OK)
     {

    /*
     * Process the buffer and return the packet
     */
    switch (bufferPtr->expanderAction)
    {
    case FBE_EXPANDER_ACTION_NONE:
        break;
    case FBE_EXPANDER_ACTION_COLD_RESET:
        fbe_base_object_customizable_trace((fbe_base_object_t*)esesEnclosurePtr,
                               FBE_TRACE_LEVEL_INFO,
                               fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)esesEnclosurePtr),
                               "FBE_EXPANDER_ACTION_COLD_RESET, duration %d, delay %d\n", 
                               bufferPtr->powerCycleDuration, bufferPtr->powerCycleDelay);

        // determine which Expander we are
        index = 
            fbe_base_enclosure_find_first_edal_match_Bool((fbe_base_enclosure_t *)esesEnclosurePtr,
                                                          FBE_ENCL_COMP_IS_LOCAL,  //attribute
                                                          FBE_ENCL_LCC,  // Component type
                                                          0, //starting index
                                                          TRUE);   
        enclStatus = fbe_base_enclosure_edal_setU8_thread_safe((fbe_base_enclosure_t *)esesEnclosurePtr,
                                                    FBE_ENCL_LCC_POWER_CYCLE_REQUEST,
                                                    FBE_ENCL_LCC,
                                                    index,
                                                    FBE_ESES_ENCL_POWER_CYCLE_REQ_BEGIN);
        if (enclStatus != FBE_ENCLOSURE_STATUS_OK)
        {
            returnEnclStatus = enclStatus;      // save a bad status to return
        }
        enclStatus = fbe_base_enclosure_edal_setU8_thread_safe((fbe_base_enclosure_t *)esesEnclosurePtr,
                                                    FBE_ENCL_LCC_POWER_CYCLE_DURATION,
                                                    FBE_ENCL_LCC,
                                                    index,
                                                    bufferPtr->powerCycleDuration);
        if (enclStatus != FBE_ENCLOSURE_STATUS_OK)
        {
            returnEnclStatus = enclStatus;      // save a bad status to return
        }
        enclStatus = fbe_base_enclosure_edal_setU8_thread_safe((fbe_base_enclosure_t *)esesEnclosurePtr,
                                                    FBE_ENCL_LCC_POWER_CYCLE_DELAY,
                                                    FBE_ENCL_LCC,
                                                    index,
                                                    bufferPtr->powerCycleDelay);
        if (enclStatus != FBE_ENCLOSURE_STATUS_OK)
        {
            returnEnclStatus = enclStatus;      // save a bad status to return
        }
        break;
    case FBE_EXPANDER_ACTION_SILENT_MODE:
        fbe_base_object_customizable_trace((fbe_base_object_t*)esesEnclosurePtr,
                               FBE_TRACE_LEVEL_INFO,
                               fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)esesEnclosurePtr),
                               "FBE_EXPANDER_ACTION_SILENT_MODE, duration %d\n", 
                                bufferPtr->powerCycleDuration);
        
        break;
    default:
        fbe_base_object_customizable_trace((fbe_base_object_t*)esesEnclosurePtr,
                               FBE_TRACE_LEVEL_ERROR,
                               fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)esesEnclosurePtr),
                               "%s, expanderAction %d invalid\n", 
                               __FUNCTION__, bufferPtr->expanderAction);
        
        fbe_transport_set_status(packetPtr, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packetPtr);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    // jap - think about error handling
    if (returnEnclStatus == FBE_ENCLOSURE_STATUS_OK)
    {
        /* Set condition to write out ESES Control */
        status = fbe_lifecycle_set_cond(&fbe_eses_enclosure_lifecycle_const, 
                                        (fbe_base_object_t*)esesEnclosurePtr, 
                                        FBE_ESES_ENCLOSURE_LIFECYCLE_COND_EXPANDER_CONTROL_NEEDED);

        if (status != FBE_STATUS_OK) 
        {
            fbe_base_object_customizable_trace((fbe_base_object_t*)esesEnclosurePtr,
                                FBE_TRACE_LEVEL_ERROR,
                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)esesEnclosurePtr),
                                "%s, can't set ControlNeeded condition, status: 0x%x.\n",
                                __FUNCTION__, status);
        }
    }
  }
    fbe_transport_set_status(packetPtr, status, 0);
    fbe_transport_complete_packet(packetPtr);

//    return returnEnclStatus;
    return status;

}   // end of eses_enclosure_processExpanderControl

static fbe_status_t 
eses_enclosure_process_fail_enclosure_request(fbe_eses_enclosure_t *eses_enclosure, fbe_packet_t *packet)
{
    fbe_payload_ex_t *                      payload = NULL;
    fbe_payload_control_operation_t *       control_operation = NULL;
    fbe_base_enclosure_fail_encl_cmd_t      *fail_cmd = NULL;
    fbe_status_t                            status;
    fbe_base_enclosure_led_status_control_t led_status;

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);
    fbe_payload_control_get_buffer(control_operation, &fail_cmd);

    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                        FBE_TRACE_LEVEL_INFO,
                                        fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                        "%s, failing enclosure Reason:0x%llx Symptom:0x%x.\n",
                                        __FUNCTION__, fail_cmd->led_reason, fail_cmd->flt_symptom);

    led_status.ledInfo.enclFaultReason = fail_cmd->led_reason;
    led_status.ledInfo.enclFaultLed = FBE_ENCLOSURE_LED_BEHAVIOR_TURN_ON;
    led_status.ledAction = FBE_ENCLOSURE_LED_CONTROL;

    fbe_eses_enclosure_setLedStatus(eses_enclosure, &led_status);
    fbe_base_enclosure_handle_errors((fbe_base_enclosure_t *)eses_enclosure, 
                                                  FBE_ENCL_ENCLOSURE,
                                                  0,
                                                  fail_cmd->flt_symptom,
                                                  0);
        
    status = fbe_lifecycle_set_cond(&fbe_eses_enclosure_lifecycle_const, 
                                        (fbe_base_object_t*)eses_enclosure, 
                                        FBE_BASE_OBJECT_LIFECYCLE_COND_FAIL); 
                                                                          
    fbe_payload_control_set_status(control_operation,(status == FBE_STATUS_OK) ? FBE_PAYLOAD_CONTROL_STATUS_OK: FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    
    return FBE_STATUS_OK;

}


/*!*************************************************************************
* @fn eses_enclosure_processLedStatusControl(                  
*                    fbe_eses_enclosure_t * esesEnclosurePtr)                  
                     fbe_packet_t *packetPtr);
***************************************************************************
*
* @brief
*       This function will process the EXPANDER_CONTROL control code.
*
* @param      fbe_eses_enclosure_t - The pointer to the ESES enclosure.
* @param      fbe_packet_t - pointer to control code packet
*
* @return     fbe_status_t - status of processing the control packet
*
* NOTES
*
* HISTORY
*   02-Dec-2008     Joe Perry - Created.
***************************************************************************/
static fbe_status_t 
eses_enclosure_processLedStatusControl(fbe_eses_enclosure_t *esesEnclosurePtr,
                                      fbe_packet_t *packetPtr)
{
    fbe_payload_control_buffer_length_t     bufferLength = 0;
    fbe_base_enclosure_led_status_control_t *bufferPtr;
    fbe_status_t                            status;
    fbe_enclosure_status_t                  enclStatus = FBE_ENCLOSURE_STATUS_OK;


    fbe_base_object_customizable_trace((fbe_base_object_t*)esesEnclosurePtr,
                           FBE_TRACE_LEVEL_INFO,
                           fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)esesEnclosurePtr),
                           "%s entry \n", __FUNCTION__);

    /*
     * Check whether there is config change in progress.
     */
    if(esesEnclosurePtr->enclosureConfigBeingUpdated)
    {
        //config change in progress.
        fbe_base_object_customizable_trace((fbe_base_object_t *) esesEnclosurePtr, 
                               FBE_TRACE_LEVEL_INFO,
                               fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *) esesEnclosurePtr),
                               "%s, return Busy (Config Change In Progress).\n",
                               __FUNCTION__);

        status = FBE_STATUS_BUSY;
        fbe_transport_set_status(packetPtr, status, 0);
        fbe_transport_complete_packet(packetPtr);
        return status;
    }

    bufferLength = sizeof(fbe_base_enclosure_led_status_control_t);
    status = fbe_base_enclosure_get_packet_payload_control_data(packetPtr,(fbe_base_enclosure_t*)esesEnclosurePtr,TRUE,(fbe_payload_control_buffer_t *)&bufferPtr,bufferLength);
      if(status == FBE_STATUS_OK)
      {


    /*
     * Process the buffer and return the packet
     */
    switch (bufferPtr->ledAction)
    {
    case FBE_ENCLOSURE_LED_NO_ACTION:
        status = FBE_STATUS_OK;
        break;

    case FBE_ENCLOSURE_LED_CONTROL:
        enclStatus = fbe_eses_enclosure_setLedControl(esesEnclosurePtr, bufferPtr);
        if (enclStatus != FBE_ENCLOSURE_STATUS_OK)
        {
            status = FBE_STATUS_GENERIC_FAILURE;
        }
        else
        {
            status = FBE_STATUS_OK;
        }
        break;

    case FBE_ENCLOSURE_LED_STATUS:
        enclStatus = fbe_eses_enclosure_setLedStatus(esesEnclosurePtr, bufferPtr);
        if (enclStatus != FBE_ENCLOSURE_STATUS_OK)
        {
            status = FBE_STATUS_GENERIC_FAILURE;
        }
        else
        {
            status = FBE_STATUS_OK;
        }
        break;

    default:
        fbe_base_object_customizable_trace((fbe_base_object_t*)esesEnclosurePtr,
                               FBE_TRACE_LEVEL_ERROR,
                               fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)esesEnclosurePtr),
                               "%s, expanderAction %d invalid\n", 
                               __FUNCTION__, bufferPtr->ledAction);
        
        fbe_transport_set_status(packetPtr, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packetPtr);
        return FBE_STATUS_GENERIC_FAILURE;
    }
}
    fbe_transport_set_status(packetPtr, status, 0);
    fbe_transport_complete_packet(packetPtr);

//    return returnEnclStatus;
    return status;

}   // end of eses_enclosure_processLedStatusControl

/*!*************************************************************************
* @fn fbe_eses_enclosure_decode_fault_led_reason(                  
*                    fbe_u32_t enclFaultLedReason)                  
***************************************************************************
*
* @brief
*       This function will decode enclosure fault LED reason.
*
* @param      enclFaultLedReason - fault code
*
* @return     char * -  string of fault type
*
* NOTES
*
* HISTORY
*   12-Dec-2011     Rui Chang - Created.
***************************************************************************/
char * fbe_eses_enclosure_decode_fault_led_reason(fbe_encl_fault_led_reason_t enclFaultLedReason)
{

    char        *string = enclFaultReasonString;
    fbe_u32_t    bit = 0;

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
            case FBE_ENCL_FAULT_LED_BATTERY_RESUME_PROM_FLT:
                strncat(string, " BBU RP Fault,", sizeof(enclFaultReasonString)-strlen(string)-1);
                break;
            case FBE_ENCL_FAULT_LED_BATTERY_FLT:
                strncat(string, " BBU Fault,", sizeof(enclFaultReasonString)-strlen(string)-1);
                break;
            case FBE_ENCL_FAULT_LED_LCC_CABLING_FLT:
                strncat(string, " LCC Cabling Fault,", sizeof(enclFaultReasonString)-strlen(string)-1);
                break;
            case FBE_ENCL_FAULT_LED_EXCEEDED_MAX:
                strncat(string, " Exceed Max Encl Limit,", sizeof(enclFaultReasonString)-strlen(string)-1);
                break;
            case FBE_ENCL_FAULT_LED_SP_FLT:
                strncat(string, " SP Fault,", sizeof(enclFaultReasonString)-strlen(string)-1);
                break;
            case FBE_ENCL_FAULT_LED_SP_FAULT_REG_FLT:
                strncat(string, " FaultReg Fault,", sizeof(enclFaultReasonString)-strlen(string)-1);
                break;
            case FBE_ENCL_FAULT_LED_SYSTEM_SERIAL_NUMBER_FLT:
                strncat(string, " System Serial Number Invalid,", sizeof(enclFaultReasonString)-strlen(string)-1);
                break;
            case FBE_ENCL_FAULT_LED_MIDPLANE_RESUME_PROM_FLT:
                strncat(string, " Midplane RP Fault,", sizeof(enclFaultReasonString)-strlen(string)-1);
                break;
            case FBE_ENCL_FAULT_LED_DRIVE_MIDPLANE_RESUME_PROM_FLT:
                strncat(string, " Drive Midplane RP Fault,", sizeof(enclFaultReasonString)-strlen(string)-1);
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
                strncat(string, " Cache Card Fault,", sizeof(enclFaultReasonString)-strlen(string)-1);
                break;
            case FBE_ENCL_FAULT_LED_DIMM_FLT:
                strncat(string, " DIMM Fault,", sizeof(enclFaultReasonString)-strlen(string)-1);
                break;
            case FBE_ENCL_FAULT_LED_SSC_FLT:
                strncat(string, " SSC,", sizeof(enclFaultReasonString)-strlen(string)-1);
                break;
            case FBE_ENCL_FAULT_LED_INTERNAL_CABLE_FLT:
                strncat(string, "SP Internal Cable Fault ",sizeof(enclFaultReasonString)-strlen(string)-1);
                break;
            case FBE_ENCL_FAULT_LED_NOT_SUPPORTED_FLT:
                strncat(string, " Unsupported Encl Fault,", sizeof(enclFaultReasonString)-strlen(string)-1);
                break;
            default:
                strncat(string, " Unknown Fault,", sizeof(enclFaultReasonString)-strlen(string)-1);
                break;
            }   // end of switch
        }   // for bit
        string[strlen(string)-1] = '\0';    //omit the lass comma
    }

    return(string);

}//end of fbe_eses_enclosure_decode_fault_led_reason
/*!*************************************************************************
* @fn fbe_eses_enclosure_DriveLedControl(                  
*                    fbe_eses_enclosure_t * eses_enclosure,
*                    fbe_base_enclosure_led_behavior_t  ledBehavior)                  
*                    fbe_u32_t index
***************************************************************************
*
* @brief
*       This function will set LED Control for a drive in this enclosure.
*
* @param      fbe_eses_enclosure_t - pointer to ESES enclosure
* @param      ledBehavior - LED Control
* @param      index - Drive index
*
* @return     fbe_status_t - status of processing the control packet
*
* NOTES
*
* HISTORY
*   03-Aug-2012     Randall Porteus - Created.
***************************************************************************/
fbe_enclosure_status_t 
fbe_eses_enclosure_DriveLedControl(fbe_eses_enclosure_t *eses_enclosure,
                                   fbe_base_enclosure_led_behavior_t  ledBehavior,
                                   fbe_u32_t index)
{
    fbe_enclosure_status_t      enclStatus;

    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                           FBE_TRACE_LEVEL_DEBUG_HIGH,
                           fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                           "%s entry\n", __FUNCTION__);

    if (ledBehavior != FBE_ENCLOSURE_LED_BEHAVIOR_NONE)
    {
        switch (ledBehavior)
        {
        case FBE_ENCLOSURE_LED_BEHAVIOR_TURN_ON:
            enclStatus = fbe_base_enclosure_edal_setBool_thread_safe((fbe_base_enclosure_t *)eses_enclosure,
                                                         FBE_ENCL_COMP_TURN_ON_FAULT_LED,
                                                         FBE_ENCL_DRIVE_SLOT,
                                                         index,
                                                         TRUE);
            if (enclStatus != FBE_ENCLOSURE_STATUS_OK)
            {
                return enclStatus;
            }
            break;
        case FBE_ENCLOSURE_LED_BEHAVIOR_TURN_OFF:
            enclStatus = fbe_base_enclosure_edal_setBool_thread_safe((fbe_base_enclosure_t *)eses_enclosure,
                                                         FBE_ENCL_COMP_TURN_ON_FAULT_LED,
                                                         FBE_ENCL_DRIVE_SLOT,
                                                         index,
                                                         FALSE);
            if (enclStatus != FBE_ENCLOSURE_STATUS_OK)
            {
                return enclStatus;
            }
            break;
        case FBE_ENCLOSURE_LED_BEHAVIOR_MARK_ON:
            enclStatus = fbe_base_enclosure_edal_setBool_thread_safe((fbe_base_enclosure_t *)eses_enclosure,
                                                         FBE_ENCL_COMP_MARK_COMPONENT,
                                                         FBE_ENCL_DRIVE_SLOT,
                                                         index,
                                                         TRUE);
            if (enclStatus != FBE_ENCLOSURE_STATUS_OK)
            {
                return enclStatus;
            }
            break;
        case FBE_ENCLOSURE_LED_BEHAVIOR_MARK_OFF:
            enclStatus = fbe_base_enclosure_edal_setBool_thread_safe((fbe_base_enclosure_t *)eses_enclosure,
                                                         FBE_ENCL_COMP_MARK_COMPONENT,
                                                         FBE_ENCL_DRIVE_SLOT,
                                                         index,
                                                         FALSE);
            if (enclStatus != FBE_ENCLOSURE_STATUS_OK)
            {
                return enclStatus;
            }
            break;
        default:
            break;
        }
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                               FBE_TRACE_LEVEL_INFO,
                               fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                              "%s, control enclDriveFaultLeds (drive %d) %d\n", 
                              __FUNCTION__, index, ledBehavior);
    }
    return FBE_ENCLOSURE_STATUS_OK;

} //end of fbe_eses_enclosure_DriveLedControl

/*!*************************************************************************
* @fn fbe_eses_enclosure_setLedControl(                  
*                    fbe_eses_enclosure_t * eses_enclosure,
*                    fbe_base_enclosure_led_status_control_t *ledControInfo)
***************************************************************************
*
* @brief
*       This function will set LED Control for this enclosure.
*
* @param      fbe_eses_enclosure_t - pointer to ESES enclosure
* @param      fbe_base_enclosure_led_status_control_t - The pointer LED Control
*
* @return     fbe_status_t - status of processing the control packet
*
* NOTES
*
* HISTORY
*   29-Jan-2009     Joe Perry - Created.
***************************************************************************/
fbe_enclosure_status_t 
fbe_eses_enclosure_setLedControl(fbe_eses_enclosure_t *eses_enclosure,
                                 fbe_base_enclosure_led_status_control_t *ledControInfo)
{
    fbe_u32_t                   maxNumberOfDrives;
    fbe_status_t                status;
    fbe_enclosure_status_t      enclStatus = FBE_ENCLOSURE_STATUS_OK;
    fbe_u32_t                   index;
    fbe_bool_t                  edalBool;

    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                           FBE_TRACE_LEVEL_DEBUG_HIGH,
                           fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                           "%s entry\n", __FUNCTION__);

    /*
     * Determine which LED's to control
     */
    // Enclosure Fault LED
    if (ledControInfo->ledInfo.enclFaultLed != FBE_ENCLOSURE_LED_BEHAVIOR_NONE)
    {
        switch (ledControInfo->ledInfo.enclFaultLed)
        {
        case FBE_ENCLOSURE_LED_BEHAVIOR_TURN_ON:
            enclStatus = fbe_base_enclosure_edal_setBool_thread_safe((fbe_base_enclosure_t *)eses_enclosure,
                                                         FBE_ENCL_COMP_TURN_ON_FAULT_LED,
                                                         FBE_ENCL_ENCLOSURE,
                                                         0,
                                                         TRUE);     // Here's the desired state of the LED, TRUE = on.
            if (enclStatus != FBE_ENCLOSURE_STATUS_OK)
            {
                return enclStatus;
            }

            // save reason Fault LED is lit
            eses_enclosure->enclFaultLedReason |= ledControInfo->ledInfo.enclFaultReason;

            // Save it in EDAL as well as that it can notify ESP when it gets changed.
            enclStatus = fbe_base_enclosure_edal_setU64_thread_safe((fbe_base_enclosure_t *)eses_enclosure,
                                                         FBE_ENCL_COMP_FAULT_LED_REASON,
                                                         FBE_ENCL_ENCLOSURE,
                                                         0,
                                                         eses_enclosure->enclFaultLedReason);     
            if (enclStatus != FBE_ENCLOSURE_STATUS_OK)
            {
                return enclStatus;
            }
            
            fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                   FBE_TRACE_LEVEL_INFO,
                                   fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                   "%s, TurnOnFaultLed On, reason 0x%llx %s\n",
                                   __FUNCTION__,
                                   eses_enclosure->enclFaultLedReason,
                                   fbe_eses_enclosure_decode_fault_led_reason(eses_enclosure->enclFaultLedReason));
            break;
        case FBE_ENCLOSURE_LED_BEHAVIOR_TURN_OFF:
            // clear reason Fault LED is lit
            eses_enclosure->enclFaultLedReason &= ~(ledControInfo->ledInfo.enclFaultReason);

            // Save it in EDAL as well as that it can notify ESP when it gets changed.
            enclStatus = fbe_base_enclosure_edal_setU64_thread_safe((fbe_base_enclosure_t *)eses_enclosure,
                                                         FBE_ENCL_COMP_FAULT_LED_REASON,
                                                         FBE_ENCL_ENCLOSURE,
                                                         0,
                                                         eses_enclosure->enclFaultLedReason);     
            if (enclStatus != FBE_ENCLOSURE_STATUS_OK)
            {
                return enclStatus;
            }

            // if no reason for Fault LED to be on, turn it off
            if (eses_enclosure->enclFaultLedReason == FBE_ENCL_FAULT_LED_NO_FLT)
            {
                enclStatus = fbe_base_enclosure_edal_setBool_thread_safe((fbe_base_enclosure_t *)eses_enclosure,
                                                             FBE_ENCL_COMP_TURN_ON_FAULT_LED,
                                                             FBE_ENCL_ENCLOSURE,
                                                             0,
                                                             FALSE);
                if (enclStatus != FBE_ENCLOSURE_STATUS_OK)
                {
                    return enclStatus;
                }
                fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                       FBE_TRACE_LEVEL_INFO,
                                       fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                       "%s, TurnOnFaultLed Off, reason 0x%llx %s\n",
                                       __FUNCTION__,
                                       eses_enclosure->enclFaultLedReason,
                                       fbe_eses_enclosure_decode_fault_led_reason(eses_enclosure->enclFaultLedReason));
            }
            break;
        case FBE_ENCLOSURE_LED_BEHAVIOR_MARK_ON:
            enclStatus = fbe_base_enclosure_edal_setBool_thread_safe((fbe_base_enclosure_t *)eses_enclosure,
                                                         FBE_ENCL_COMP_MARK_COMPONENT,
                                                         FBE_ENCL_ENCLOSURE,
                                                         0,
                                                         TRUE);
            if (enclStatus != FBE_ENCLOSURE_STATUS_OK)
            {
                return enclStatus;
            }
            break;
        case FBE_ENCLOSURE_LED_BEHAVIOR_MARK_OFF:
            enclStatus = fbe_base_enclosure_edal_setBool_thread_safe((fbe_base_enclosure_t *)eses_enclosure,
                                                         FBE_ENCL_COMP_MARK_COMPONENT,
                                                         FBE_ENCL_ENCLOSURE,
                                                         0,
                                                         FALSE);
            if (enclStatus != FBE_ENCLOSURE_STATUS_OK)
            {
                return enclStatus;
            }
            break;
        default:
            break;
        }

        enclStatus = fbe_base_enclosure_edal_getBool_thread_safe((fbe_base_enclosure_t *)eses_enclosure,
                                                     FBE_ENCL_COMP_FAULT_LED_ON,
                                                     FBE_ENCL_ENCLOSURE,
                                                     0,
                                                     &edalBool);
        if (enclStatus == FBE_ENCLOSURE_STATUS_OK)
        {
            fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                   FBE_TRACE_LEVEL_INFO,
                                   fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                   "%s, enclFaultLedStatus %d, enclFaultReason 0x%llx %s\n",
                                   __FUNCTION__,
                                   edalBool,
                                   eses_enclosure->enclFaultLedReason,
                                   fbe_eses_enclosure_decode_fault_led_reason(eses_enclosure->enclFaultLedReason));
        }
 }

    // Displays for Bus & Enclosure
    if (ledControInfo->ledInfo.busDisplay == FBE_ENCLOSURE_LED_BEHAVIOR_DISPLAY)
    {
        enclStatus = fbe_base_enclosure_edal_setU8_thread_safe((fbe_base_enclosure_t *)eses_enclosure,
                                                   FBE_DISPLAY_MODE,
                                                   FBE_ENCL_DISPLAY,
                                                   FBE_EDAL_DISPLAY_BUS_0,
                                                   SES_DISPLAY_MODE_DISPLAY_CHAR);
        enclStatus = fbe_base_enclosure_edal_setU8_thread_safe((fbe_base_enclosure_t *)eses_enclosure,
                                                   FBE_DISPLAY_CHARACTER,
                                                   FBE_ENCL_DISPLAY,
                                                   FBE_EDAL_DISPLAY_BUS_0,                 // jap - set to correct index
                                                   ledControInfo->ledInfo.busNumber[0]);
        enclStatus = fbe_base_enclosure_edal_setU8_thread_safe((fbe_base_enclosure_t *)eses_enclosure,
                                                   FBE_DISPLAY_MODE,
                                                   FBE_ENCL_DISPLAY,
                                                   FBE_EDAL_DISPLAY_BUS_1,
                                                   SES_DISPLAY_MODE_DISPLAY_CHAR);
        enclStatus = fbe_base_enclosure_edal_setU8_thread_safe((fbe_base_enclosure_t *)eses_enclosure,
                                                   FBE_DISPLAY_CHARACTER,
                                                   FBE_ENCL_DISPLAY,
                                                   FBE_EDAL_DISPLAY_BUS_1,                 // jap - set to correct index
                                                   ledControInfo->ledInfo.busNumber[1]);
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                               FBE_TRACE_LEVEL_INFO,
                               fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                              "%s, control busNumber %c%c\n", 
                              __FUNCTION__, 
                              ledControInfo->ledInfo.busNumber[1],
                              ledControInfo->ledInfo.busNumber[0]);

        if (enclStatus != FBE_ENCLOSURE_STATUS_OK)
        {
            return enclStatus;
        }
    }
    if (ledControInfo->ledInfo.enclDisplay == FBE_ENCLOSURE_LED_BEHAVIOR_DISPLAY)
    {
        enclStatus = fbe_base_enclosure_edal_setU8_thread_safe((fbe_base_enclosure_t *)eses_enclosure,
                                                   FBE_DISPLAY_MODE,
                                                   FBE_ENCL_DISPLAY,
                                                   FBE_EDAL_DISPLAY_ENCL_0,
                                                   SES_DISPLAY_MODE_DISPLAY_CHAR);
        enclStatus = fbe_base_enclosure_edal_setU8_thread_safe((fbe_base_enclosure_t *)eses_enclosure,
                                                   FBE_DISPLAY_CHARACTER,
                                                   FBE_ENCL_DISPLAY,
                                                   FBE_EDAL_DISPLAY_ENCL_0,                 // jap - set to correct index
                                                   ledControInfo->ledInfo.enclNumber);
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                               FBE_TRACE_LEVEL_INFO,
                               fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                              "%s, control enclNumber %c\n", 
                              __FUNCTION__, ledControInfo->ledInfo.enclNumber);

        if (enclStatus != FBE_ENCLOSURE_STATUS_OK)
        {
            return enclStatus;
        }
    }

    // Drive Fault LED's
    maxNumberOfDrives = fbe_eses_enclosure_get_number_of_slots(eses_enclosure);
    if (ledControInfo->driveCount == 1)
    {

        index =  fbe_base_enclosure_find_first_edal_match_U8((fbe_base_enclosure_t *)eses_enclosure,
                                                                  FBE_ENCL_DRIVE_SLOT_NUMBER,         // attribute
                                                                  FBE_ENCL_DRIVE_SLOT,                // component type
                                                                  0,                                  // starting index
                                                                  ledControInfo->driveSlot);  // drive slot to match
        if (index >= maxNumberOfDrives)  
        {
            fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                   FBE_TRACE_LEVEL_DEBUG_HIGH,
                                   fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                   "%s -- Unable to get drive slot number for DriveSlot %d\n", __FUNCTION__,
                                   ledControInfo->driveSlot);
            return FBE_ENCLOSURE_STATUS_DATA_ILLEGAL;
        }
        enclStatus = fbe_eses_enclosure_DriveLedControl((fbe_eses_enclosure_t *)eses_enclosure,
                                 ledControInfo->ledInfo.enclDriveFaultLeds[ledControInfo->driveSlot],
                                 index);
        if (enclStatus != FBE_ENCLOSURE_STATUS_OK)  
        {
            return enclStatus;
        }
    }
    else
    {
        for (index = 0; index < maxNumberOfDrives; index++)
        {
            enclStatus = fbe_eses_enclosure_DriveLedControl((fbe_eses_enclosure_t *)eses_enclosure,
                                                            ledControInfo->ledInfo.enclDriveFaultLeds[index],
                                                            index);
            if (enclStatus != FBE_ENCLOSURE_STATUS_OK)  
            {
                return enclStatus;
            }
        }
    }


    // This mez_connectorLed is for the I/O expansion connector in the mezzanine.  
    if (ledControInfo->ledInfo.mez_connectorLed != FBE_ENCLOSURE_LED_BEHAVIOR_NONE)
    {
        fbe_u8_t connectorIndex;
        // Figure out the index for the mezanine connector LED, return it in connectorIndex:

        enclStatus = fbe_edal_getConnectorIndex ( ((fbe_base_enclosure_t *)eses_enclosure)->enclosureComponentTypeBlock, // enclosureComponentBlockPtr, 
                                     FBE_ENCL_CONNECTOR_IS_ENTIRE_CONNECTOR,
                                     FBE_ENCL_COMP_IS_LOCAL,
                                     FBE_ENCL_CONNECTOR_PRIMARY_PORT,
                                     &connectorIndex );

        if (enclStatus != FBE_ENCLOSURE_STATUS_OK)  
        {
            fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                   FBE_TRACE_LEVEL_DEBUG_HIGH,
                                   fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                   "%s -- Unable to locate mezanine connector index to reach the LED\n", __FUNCTION__);
            return enclStatus;
        }

        switch (ledControInfo->ledInfo.mez_connectorLed)
        {
        case FBE_ENCLOSURE_LED_BEHAVIOR_TURN_ON:
            enclStatus = fbe_base_enclosure_edal_setBool_thread_safe((fbe_base_enclosure_t *)eses_enclosure,
                                                         FBE_ENCL_COMP_TURN_ON_FAULT_LED,
                                                         FBE_ENCL_CONNECTOR,
                                                         connectorIndex,
                                                         TRUE);     // Here's the desired state of the LED, TRUE = on.
            if (enclStatus != FBE_ENCLOSURE_STATUS_OK)
            {
                return enclStatus;
            }
            break;
        case FBE_ENCLOSURE_LED_BEHAVIOR_TURN_OFF:
            enclStatus = fbe_base_enclosure_edal_setBool_thread_safe((fbe_base_enclosure_t *)eses_enclosure,
                                                         FBE_ENCL_COMP_TURN_ON_FAULT_LED,
                                                         FBE_ENCL_CONNECTOR,
                                                         connectorIndex,
                                                         FALSE);
            if (enclStatus != FBE_ENCLOSURE_STATUS_OK)
            {
                return enclStatus;
            }
            break;
        case FBE_ENCLOSURE_LED_BEHAVIOR_MARK_ON:
            enclStatus = fbe_base_enclosure_edal_setBool_thread_safe((fbe_base_enclosure_t *)eses_enclosure,
                                                         FBE_ENCL_COMP_MARK_COMPONENT,
                                                         FBE_ENCL_CONNECTOR,
                                                         connectorIndex,
                                                         TRUE);
            if (enclStatus != FBE_ENCLOSURE_STATUS_OK)
            {
                return enclStatus;
            }
            break;
        case FBE_ENCLOSURE_LED_BEHAVIOR_MARK_OFF:
            enclStatus = fbe_base_enclosure_edal_setBool_thread_safe((fbe_base_enclosure_t *)eses_enclosure,
                                                         FBE_ENCL_COMP_MARK_COMPONENT,
                                                         FBE_ENCL_CONNECTOR,
                                                         connectorIndex,
                                                         FALSE);
            if (enclStatus != FBE_ENCLOSURE_STATUS_OK)
            {
                return enclStatus;
            }
            break;
        default:
            break;
        }

        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                               FBE_TRACE_LEVEL_INFO,
                               fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                              "%s, control mez_connectorLed %d, index %d\n", 
                              __FUNCTION__, ledControInfo->ledInfo.mez_connectorLed, connectorIndex);

    }

    /*
     * Set condition to write out ESES Control
     */
    status = fbe_lifecycle_set_cond(&fbe_eses_enclosure_lifecycle_const, 
                                    (fbe_base_object_t*)eses_enclosure, 
                                    FBE_ESES_ENCLOSURE_LIFECYCLE_COND_EXPANDER_CONTROL_NEEDED);
    if (status != FBE_STATUS_OK) 
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                               FBE_TRACE_LEVEL_ERROR,
                               fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                              "%s, can't set ControlNeeded condition, status: 0x%x.\n",
                              __FUNCTION__, status);
        
    }

    return FBE_ENCLOSURE_STATUS_OK;

}   // end of fbe_eses_enclosure_setLedControl


/*!*************************************************************************
* @fn fbe_eses_enclosure_setLedStatus(                  
*                    fbe_eses_enclosure_t * eses_enclosure,
*                    fbe_base_enclosure_led_status_control_t *ledControInfo)                  
***************************************************************************
*
* @brief
*       This function will return LED Status for this enclosure.
*
* @param      fbe_eses_enclosure_t - pointer to ESES enclosure
* @param      fbe_base_enclosure_led_status_control_t - The pointer LED Control
*
* @return     fbe_status_t - status of processing the control packet
*
* NOTES
*
* HISTORY
*   01-Feb-2011     Joe Perry - Created.
***************************************************************************/
fbe_enclosure_status_t 
fbe_eses_enclosure_setLedStatus(fbe_eses_enclosure_t *eses_enclosure,
                                 fbe_base_enclosure_led_status_control_t *ledControInfo)
{
    fbe_enclosure_status_t      enclStatus;
    fbe_bool_t                  edalBool;
    fbe_u8_t                    maxNumberOfDrives;

    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                           FBE_TRACE_LEVEL_DEBUG_HIGH,
                           fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                           "%s entry\n", __FUNCTION__);

    
    enclStatus = fbe_base_enclosure_edal_getBool_thread_safe((fbe_base_enclosure_t *)eses_enclosure,
                                                 FBE_ENCL_COMP_MARKED,
                                                 FBE_ENCL_ENCLOSURE,
                                                 0,
                                                 &edalBool);
    if (enclStatus != FBE_ENCLOSURE_STATUS_OK)
    {
        return enclStatus;
    }

    ledControInfo->ledInfo.enclFaultLedMarked = edalBool;

    enclStatus = fbe_base_enclosure_edal_getBool_thread_safe((fbe_base_enclosure_t *)eses_enclosure,
                                                 FBE_ENCL_COMP_FAULT_LED_ON,
                                                 FBE_ENCL_ENCLOSURE,
                                                 0,
                                                 &edalBool);
    if (enclStatus != FBE_ENCLOSURE_STATUS_OK)
    {
        return enclStatus;
    }

    ledControInfo->ledInfo.enclFaultLedStatus = edalBool;
    ledControInfo->ledInfo.enclFaultReason = eses_enclosure->enclFaultLedReason;

    /*
     * Fill in the drive slot fault LED status for each drive slot.
     */
    maxNumberOfDrives = fbe_eses_enclosure_get_number_of_slots(eses_enclosure);
    ledControInfo->driveCount = maxNumberOfDrives;

    enclStatus = fbe_eses_enclosure_DriveLedStatus(eses_enclosure, ledControInfo->ledInfo.enclDriveFaultLedStatus, maxNumberOfDrives);
    if(enclStatus != FBE_ENCLOSURE_STATUS_OK)
    {
        return enclStatus;
    }

    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                           FBE_TRACE_LEVEL_INFO,
                           fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                           "%s, enclFaultLedStatus %d, enclFaultReason 0x%llx %s\n",
                           __FUNCTION__,
                           ledControInfo->ledInfo.enclFaultLedStatus,
                           ledControInfo->ledInfo.enclFaultReason,
                           fbe_eses_enclosure_decode_fault_led_reason(ledControInfo->ledInfo.enclFaultReason));

    return FBE_ENCLOSURE_STATUS_OK;

}   // end of fbe_eses_enclosure_setLedStatus


/*!*************************************************************************
* @fn fbe_eses_enclosure_DriveLedStatus(                  
*                    fbe_eses_enclosure_t * eses_enclosure,
*                    fbe_led_status_t *pEnclDriveFaultLeds,                  
*                    fbe_u32_t maxNumberOfDrives
***************************************************************************
*
* @brief
*       This function will get LED status for a drive in this enclosure.
*
* @param      fbe_eses_enclosure_t - pointer to ESES enclosure
* @param      pEnclDriveFaultLeds pointer to drive fault leds
* @param      maxNumberOfDrives
*
* @return     fbe_status_t - status of drive led reads
*
* NOTES
*
* HISTORY
*   12-Oct-2012     Randall Porteus - Created.
***************************************************************************/
fbe_enclosure_status_t 
fbe_eses_enclosure_DriveLedStatus(fbe_eses_enclosure_t *eses_enclosure,
                                 fbe_led_status_t *pEnclDriveFaultLeds,
								 fbe_u32_t maxNumberOfDrives)
{
    fbe_enclosure_status_t      enclStatus;
    fbe_bool_t                  on;
    fbe_u32_t                   drive_index;
    fbe_u8_t                    led_index;

    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                               FBE_TRACE_LEVEL_DEBUG_HIGH,
                               fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                               "%s entry\n", __FUNCTION__);

    for (drive_index = 0; drive_index < maxNumberOfDrives; drive_index++)
    {
        enclStatus = fbe_base_enclosure_edal_getU8((fbe_base_enclosure_t *)eses_enclosure,
                                                    FBE_ENCL_DRIVE_SLOT_NUMBER,
                                                    FBE_ENCL_DRIVE_SLOT,
                                                    drive_index,
                                                    &led_index);
        if(enclStatus != FBE_ENCLOSURE_STATUS_OK)
        {
            return enclStatus;
        }

        if (led_index >= FBE_ENCLOSURE_MAX_NUMBER_OF_DRIVE_SLOTS) 
        {
            fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                       FBE_TRACE_LEVEL_WARNING,
                                       fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                       "%s: Invalid LED index: %d \n", __FUNCTION__, led_index);
            continue;
        }

        enclStatus = fbe_base_enclosure_edal_getBool_thread_safe((fbe_base_enclosure_t *)eses_enclosure,
                                                     FBE_ENCL_COMP_INSERTED,
                                                     FBE_ENCL_DRIVE_SLOT,
                                                     drive_index,
                                                     &on);
        if(enclStatus != FBE_ENCLOSURE_STATUS_OK)
        {
            return enclStatus;
        }
        if(!on)
        {
            pEnclDriveFaultLeds[led_index] = FBE_LED_STATUS_INVALID;
            continue;
        }
        // get bit for Flashing
        enclStatus = fbe_base_enclosure_edal_getBool_thread_safe((fbe_base_enclosure_t *)eses_enclosure,
                                                     FBE_ENCL_COMP_MARKED,
                                                     FBE_ENCL_DRIVE_SLOT,
                                                     drive_index,
                                                     &on);
        if(enclStatus != FBE_ENCLOSURE_STATUS_OK)
        {
            return enclStatus;
        }
        if(on)
        {
            pEnclDriveFaultLeds[led_index] = FBE_LED_STATUS_MARKED;
        }
        else
        { 
            // it is not marked, check if it is faulted
            enclStatus = fbe_base_enclosure_edal_getBool_thread_safe((fbe_base_enclosure_t *)eses_enclosure,
                                                         FBE_ENCL_COMP_FAULT_LED_ON,
                                                         FBE_ENCL_DRIVE_SLOT,
                                                         drive_index,
                                                         &on);
            if(enclStatus != FBE_ENCLOSURE_STATUS_OK)
            {
                return enclStatus;
            }
            if(on)
            {
                pEnclDriveFaultLeds[led_index] = FBE_LED_STATUS_ON;
            }
            else
            {
                // the LED is not marked or faulted
                pEnclDriveFaultLeds[led_index] = FBE_LED_STATUS_OFF;
            }
        }
    }
    return FBE_ENCLOSURE_STATUS_OK;

} //end of fbe_eses_enclosure_DriveLedStatus

/*!*************************************************************************
* @fn fbe_eses_enclosure_process_ctrl_op()                  
***************************************************************************
* @brief
*       This function processes the control operation which does NOT 
*       need to allocate a new control operation and send down the SCSI 
*       command directly.
*
* @param     eses_enclosure - The pointer to the fbe_eses_enclosure_t.
* @param     packet - The pointer to the fbe_packet_t.
* @param     ctrl_code - The control code.
*
* @return    fbe_status_t.
*
* NOTES
*
* HISTORY
*   18-Mar-2009 PHE - Created.
***************************************************************************/
static fbe_status_t 
fbe_eses_enclosure_process_ctrl_op(fbe_eses_enclosure_t *eses_enclosure, 
                                           fbe_packet_t *packet,
                                           fbe_base_enclosure_control_code_t ctrl_code)
{
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;   
    fbe_payload_control_buffer_t control_buffer = NULL;    
    fbe_payload_control_buffer_length_t control_buffer_length = 0;
    fbe_enclosure_mgmt_ctrl_op_t * ctrl_op = NULL;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_enclosure_status_t encl_status = FBE_ENCLOSURE_STATUS_OK;
    fbe_sg_element_t    *sg_element = NULL;


    /********
     * BEGIN
     ********/

    /* The original packet status is FBE_STATUS_INVALID (initialized in fbe_transport_initialize_packet)
     * we set it to FBE_STATUS_OK to start with. 
     * It will be updated as needed later on. 
     */
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);

    payload = fbe_transport_get_payload_ex(packet);
     if(payload == NULL)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                            FBE_TRACE_LEVEL_ERROR,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                            "%s, get_payload failed.\n", __FUNCTION__);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet); 
        return FBE_STATUS_GENERIC_FAILURE;
    }

    control_operation = fbe_payload_ex_get_control_operation(payload);
    if(control_operation == NULL)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                            FBE_TRACE_LEVEL_ERROR,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                            "%s, get_control_operation failed.\n", __FUNCTION__); 

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

  status =  fbe_payload_control_get_buffer(control_operation, &control_buffer);
    if( (control_buffer == NULL) || (status!=FBE_STATUS_OK))
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                               FBE_TRACE_LEVEL_WARNING,
                               fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                "process_ctrl_op, ctrl_code: 0x%x, fbe_payload_control_get_buffer failed.\n",
                                 ctrl_code);

        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                               FBE_TRACE_LEVEL_WARNING,
                               fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                               "status: 0x%x, control_buffer: %64p.\n",
                               status, control_buffer);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Set the control operation status and status qualifier to OK to start with. 
     * They will be updated as needed later on.
     */
    fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_payload_control_set_status_qualifier(control_operation, FBE_ENCLOSURE_STATUS_OK);

    ctrl_op = (fbe_enclosure_mgmt_ctrl_op_t *)control_buffer;

    /* Verify the control buffer size with the size of the expected structure */
    status = fbe_payload_control_get_buffer_length(control_operation, &control_buffer_length);
    if((status != FBE_STATUS_OK) || 
       (control_buffer_length != sizeof(fbe_enclosure_mgmt_ctrl_op_t)))
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                               FBE_TRACE_LEVEL_WARNING,
                               fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                "Process_ctrl_op, ctrl_code: 0x%x, fbe_payload_control_get_buffer_length failed.\n", 
                                ctrl_code);

        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                               FBE_TRACE_LEVEL_WARNING,
                               fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                "Control_buffer_length %d, expected_length: %llu, status: 0x%x.\n", 
                                control_buffer_length, 
                                (unsigned long long)sizeof(fbe_enclosure_mgmt_ctrl_op_t), status);
        
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    } 

    /*
     * Check whether there is config change in progress.
     */
    if(eses_enclosure->enclosureConfigBeingUpdated)
    {
        //config change in progress.
        fbe_base_object_customizable_trace((fbe_base_object_t *) eses_enclosure, 
                               FBE_TRACE_LEVEL_INFO,
                               fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *) eses_enclosure),
                               "process_ctrl_op, ctrl_code 0x%x, return Busy (Config Change In Progress).\n",
                               ctrl_code);

        status = FBE_STATUS_BUSY;
        fbe_transport_set_status(packet, status, 0);
        fbe_transport_complete_packet(packet);
        return status;
    }
  
    /* ctrl_op->response_buf_p could point to the user space address if the command is from user space.
     * Make it point to the kernel space address saved in sg_element[0].address.
     */
    switch(ctrl_code)
    {
        case FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_SLOT_TO_PHY_MAPPING: 
            fbe_payload_ex_get_sg_list(payload, &sg_element, NULL);

            if (sg_element != NULL)
            {
                ctrl_op->response_buf_p = sg_element[0].address;
                ctrl_op->response_buf_size = sg_element[0].count;
            }
            else
            {
                fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                       FBE_TRACE_LEVEL_WARNING,
                                       fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                       "%s, SG_LIST is NULL, ctrl_code: 0x%x.\n", __FUNCTION__, ctrl_code);

                fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
                fbe_transport_complete_packet(packet);
                return FBE_STATUS_GENERIC_FAILURE;
            }
            break;

        default:
            break;
    }

    switch (ctrl_code)
    {
        case FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_SLOT_TO_PHY_MAPPING: 
            encl_status = fbe_eses_enclosure_get_slot_to_phy_mapping(eses_enclosure, ctrl_op);   
            break;
       
         default:
            /* not supported */
            fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                   FBE_TRACE_LEVEL_WARNING,
                                   fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                    "%s, unsupported ctrl_code: 0x%x.\n", 
                                    __FUNCTION__, ctrl_code);

            fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
            fbe_transport_complete_packet(packet);
            break;
    }

    /* Set the control operation status qualifier with the encl_status. */
    fbe_payload_control_set_status_qualifier(control_operation, encl_status);
    /* Complete the packet. */
    fbe_transport_complete_packet(packet);

    return FBE_STATUS_OK;
    
} // End of function - fbe_eses_enclosure_process_ctrl_op

/*!*************************************************************************
 *  @fn fbe_eses_enclosure_get_slot_to_phy_mapping(
 *                     fbe_eses_enclosure_t * eses_enclosure,
 *                     fbe_enclosure_mgmt_ctrl_op_t * ctrl_op_p)
 **************************************************************************
 *  @brief
 *      This function processes the FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_SLOT_TO_PHY_MAPPING
 *      control code. The phy mapping response is saved in the response buffer 
 *      in SLOT ordered manner. 
 *
 *  @param    eses_enclosure - The pointer to a Eses Enclosure object.
 *  @param    ctrl_op_p - The pointer to fbe_enclosure_mgmt_ctrl_op_t.
 *
 *  @return   fbe_enclosure_status_t
 *       FBE_ENCLOSURE_STATUS_OK - no error.
 *       Otherwise, error found.
 *
 *  NOTES:
 *
 *  HISTORY:
 *    18-Mar-2009: PHE - Created
 *
 **************************************************************************/
static fbe_enclosure_status_t 
fbe_eses_enclosure_get_slot_to_phy_mapping(fbe_eses_enclosure_t * eses_enclosure,
                                          fbe_enclosure_mgmt_ctrl_op_t * ctrl_op_p)
{
    fbe_enclosure_mgmt_slot_to_phy_mapping_cmd_t *cmd_p = NULL;
    fbe_enclosure_mgmt_slot_to_phy_mapping_info_t *mapping_info_p = NULL;
    fbe_u8_t phy_index;
    fbe_u8_t phy_id;
    fbe_u8_t slot_index;
    fbe_u8_t expander_index;
    fbe_u8_t expander_element_index;
    fbe_enclosure_slot_number_t number_of_slots = 0;
    fbe_enclosure_slot_number_t slot_num = 0;
    fbe_sas_address_t expander_sas_address;
    fbe_enclosure_status_t encl_status = FBE_ENCLOSURE_STATUS_OK;

    cmd_p = &(ctrl_op_p->cmd_buf.slot_to_phy_mapping_cmd);

    number_of_slots = fbe_eses_enclosure_get_number_of_slots(eses_enclosure);

    /* Validate the command. */
    if((cmd_p->slot_num_start >= number_of_slots) || 
       (cmd_p->slot_num_end >= number_of_slots) ||
       (cmd_p->slot_num_start > cmd_p->slot_num_end))
    {
        return FBE_ENCLOSURE_STATUS_PARAMETER_INVALID;
    }

    ctrl_op_p->required_response_buf_size = 
                (cmd_p->slot_num_end - cmd_p->slot_num_start + 1) * 
                sizeof(fbe_enclosure_mgmt_slot_to_phy_mapping_info_t);

    mapping_info_p = (fbe_enclosure_mgmt_slot_to_phy_mapping_info_t *)(ctrl_op_p->response_buf_p);

    for(slot_num = cmd_p->slot_num_start; slot_num <= cmd_p->slot_num_end; slot_num ++)
    {
        /* Check whether the user has allocated enough memory for the response. */
        if((slot_num - cmd_p->slot_num_start + 1) * sizeof(fbe_enclosure_mgmt_slot_to_phy_mapping_info_t) >
                    ctrl_op_p->response_buf_size)
        {
            return FBE_ENCLOSURE_STATUS_ALLOCATED_MEMORY_INSUFFICIENT;
        }        
        slot_index =  fbe_base_enclosure_find_first_edal_match_U8((fbe_base_enclosure_t *)eses_enclosure,
                                                    FBE_ENCL_DRIVE_SLOT_NUMBER,  //attribute
                                                    FBE_ENCL_DRIVE_SLOT,  // Component type
                                                    0, //starting index
                                                    slot_num);   
        // if there isn't a valid slot index
        if(slot_index == FBE_ENCLOSURE_VALUE_INVALID)
        {
            continue;
        }

        // slot_index -> phy_index 
        encl_status = fbe_base_enclosure_edal_getU8_thread_safe((fbe_base_enclosure_t *)eses_enclosure,
                                                            FBE_ENCL_DRIVE_PHY_INDEX, //attribute
                                                            FBE_ENCL_DRIVE_SLOT, // Component type
                                                            slot_index, // Component index
                                                            &phy_index);
        
        if(encl_status != FBE_ENCLOSURE_STATUS_OK)
        {
            return encl_status;
        }

        // phy_index -> phy_id 
        encl_status = fbe_base_enclosure_edal_getU8_thread_safe((fbe_base_enclosure_t *)eses_enclosure,
                                                            FBE_ENCL_EXP_PHY_ID, //attribute
                                                            FBE_ENCL_EXPANDER_PHY,// Component type
                                                            phy_index, // Component index
                                                            &phy_id);

        if(encl_status != FBE_ENCLOSURE_STATUS_OK)
        {
            return encl_status;
        }

        // phy_index -> expander element index. 
        encl_status = fbe_base_enclosure_edal_getU8_thread_safe((fbe_base_enclosure_t *)eses_enclosure,
                                                            FBE_ENCL_EXP_PHY_EXPANDER_ELEM_INDEX, //attribute
                                                            FBE_ENCL_EXPANDER_PHY,// Component type
                                                            phy_index, // Component index
                                                            &expander_element_index);

        if(encl_status != FBE_ENCLOSURE_STATUS_OK)
        {
            return encl_status;
        }

        // Find the expander_index which has the specified expander_element_index.
        expander_index = 
        fbe_base_enclosure_find_first_edal_match_U8((fbe_base_enclosure_t *)eses_enclosure,
                                                    FBE_ENCL_COMP_ELEM_INDEX,  //attribute
                                                    FBE_ENCL_EXPANDER,  // Component type
                                                    0, //starting index
                                                    expander_element_index);   

        if(expander_index == FBE_ENCLOSURE_VALUE_INVALID)
        {
            return FBE_ENCLOSURE_STATUS_TARGET_NOT_FOUND;
        }

   
        encl_status = fbe_base_enclosure_edal_getU64_thread_safe((fbe_base_enclosure_t *)eses_enclosure,
                                                            FBE_ENCL_EXP_SAS_ADDRESS, //attribute
                                                            FBE_ENCL_EXPANDER,// Component type
                                                            expander_index,  // Component index
                                                            &expander_sas_address);


        if(encl_status != FBE_ENCLOSURE_STATUS_OK)
        {
            return encl_status;
        } 

        mapping_info_p->expander_sas_address = expander_sas_address;
        mapping_info_p->phy_id = phy_id;
        mapping_info_p ++;         
    }// End of - for(slot_num = cmd_p->slot_num_start; slot_num < cmd_p->slot_num_end; slot_num ++)

    return FBE_ENCLOSURE_STATUS_OK;

}// End of - fbe_eses_enclosure_get_slot_to_phy_mapping


/*!*************************************************************************
* @fn fbe_eses_enclosure_process_getPsInfo()                  
***************************************************************************
* @brief
*       This function processes the control operation for retrieving
*       PS info from the enclosure.
*
* @param     eses_enclosure - The pointer to the fbe_eses_enclosure_t.
* @param     packet - The pointer to the fbe_packet_t.
* @param     ctrl_code - The control code.
*
* @return    fbe_status_t.
*
* NOTES
*
* HISTORY
*   29-Apr-2010     Joe Perry - Created.
***************************************************************************/
static fbe_status_t 
fbe_eses_enclosure_process_getPsInfo(fbe_eses_enclosure_t *eses_enclosure, 
                                     fbe_packet_t *packet,
                                     fbe_base_enclosure_control_code_t ctrl_code)
{
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;   
    fbe_payload_control_buffer_t control_buffer = NULL;    
    fbe_payload_control_buffer_length_t control_buffer_length = 0;
    fbe_power_supply_info_t *getPsInfo = NULL;
    fbe_status_t            status = FBE_STATUS_OK;
    fbe_enclosure_status_t  encl_status = FBE_ENCLOSURE_STATUS_OK;
    fbe_bool_t              inserted, faulted, acFail;
    fbe_u8_t                subenclProductId[FBE_ESES_CONFIG_SUBENCL_PRODUCT_ID_SIZE] = {0};
    fbe_u8_t                number_of_components = 0;
    fbe_u32_t               psIndex;
    fbe_u32_t               psAcFailIndex = 0;
    fbe_edal_fw_info_t      edal_fw_info;
    fbe_u8_t                coolingComponentCount = 0;
    fbe_u32_t               coolingIndex = 0;
    fbe_u8_t                subtype = FBE_ENCL_COOLING_SUBTYPE_INVALID;
    fbe_u8_t                targetSubtype = FBE_ENCL_COOLING_SUBTYPE_INVALID;
    fbe_u8_t                sideId = 0;
    fbe_u8_t                containerIndex = 0;
    fbe_u32_t               slotNum = 0;
    fbe_u8_t                psMarginTestMode, psMarginTestResults;
    fbe_u8_t                sp_id = 0;
    fbe_u8_t                revIndex = 0;
    fbe_u8_t                psSide = 0;
    fbe_u8_t                psCountPerSide = 0;
    fbe_u8_t                psSlotNumBasedOnSide = 0;

    /********
     * BEGIN
     ********/

    /* The original packet status is FBE_STATUS_INVALID (initialized in fbe_transport_initialize_packet)
     * we set it to FBE_STATUS_OK to start with. 
     * It will be updated as needed later on. 
     */
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);

    payload = fbe_transport_get_payload_ex(packet);
    if(payload == NULL)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                            FBE_TRACE_LEVEL_ERROR,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                            "%s, get_payload failed.\n", __FUNCTION__); 

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    control_operation = fbe_payload_ex_get_control_operation(payload);
    if(control_operation == NULL)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                            FBE_TRACE_LEVEL_ERROR,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                            "%s, get_control_operation failed.\n", __FUNCTION__); 

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status =  fbe_payload_control_get_buffer(control_operation, &control_buffer);
    if( (control_buffer == NULL) || (status!=FBE_STATUS_OK))
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                               FBE_TRACE_LEVEL_WARNING,
                               fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                "process_ctrl_op, ctrl_code: 0x%x, fbe_payload_control_get_buffer failed.\n",
                                 ctrl_code);

        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                               FBE_TRACE_LEVEL_WARNING,
                               fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                               "status: 0x%x, control_buffer: %64p.\n",
                               status, control_buffer);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Set the control operation status and status qualifier to OK to start with. 
     * They will be updated as needed later on.
     */
    fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_payload_control_set_status_qualifier(control_operation, FBE_ENCLOSURE_STATUS_OK);

    /*
     * Check whether there is config change in progress.
     */
    if(eses_enclosure->enclosureConfigBeingUpdated)
    {
        //config change in progress.
        fbe_base_object_customizable_trace((fbe_base_object_t *) eses_enclosure, 
                               FBE_TRACE_LEVEL_INFO,
                               fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *) eses_enclosure),
                               "process_ctrl_op, ctrl_code 0x%x, return Busy (Config Change In Progress).\n",
                               ctrl_code);

        status = FBE_STATUS_BUSY;
        fbe_transport_set_status(packet, status, 0);
        fbe_transport_complete_packet(packet);
        return status;
    }

    getPsInfo = (fbe_power_supply_info_t *)control_buffer;

    /* Verify the control buffer size with the size of the expected structure */
    status = fbe_payload_control_get_buffer_length(control_operation, &control_buffer_length);
    if((status != FBE_STATUS_OK) || 
       (control_buffer_length != sizeof(fbe_power_supply_info_t)))
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                               FBE_TRACE_LEVEL_WARNING,
                               fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                "Process_ctrl_op, ctrl_code: 0x%x, fbe_payload_control_get_buffer_length failed.\n", 
                                ctrl_code);

        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                               FBE_TRACE_LEVEL_WARNING,
                               fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                "Control_buffer_length %d, expected_length: %llu, status: 0x%x.\n", 
                                control_buffer_length, 
                                (unsigned long long)sizeof(fbe_power_supply_info_t), status);
        
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    slotNum = getPsInfo->slotNumOnEncl;

    /*
     * Get enclosure Power Supply info
     */
    status = fbe_base_enclosure_get_number_of_components((fbe_base_enclosure_t *)eses_enclosure,
                                                          FBE_ENCL_POWER_SUPPLY,
                                                          &number_of_components); 
    if(status != FBE_STATUS_OK)
    {
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                           FBE_TRACE_LEVEL_DEBUG_MEDIUM,
                                           fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                           "  numberofcomponents %d\n", 
                                           number_of_components);
    
    // get the version descriptor level (cdes1, cdes2) from the enclosure
    getPsInfo->esesVersionDesc = eses_enclosure->eses_version_desc;

    /* Expander would not return the process enclosure power supply status.
     * So we should return FALSE here always.
     */
    getPsInfo->inProcessorEncl = FALSE;

    // Loop through the number of ps components and set the data.
    // Use the slot from the location (input) to determine if the data
    // belongs to this ps. The ps items with side 0 will go with slot 0.
    // The ps items with side 1 will go with slot 1.
    for(psIndex=0; psIndex<number_of_components; psIndex++)
    {
        // get the side id
        encl_status = fbe_base_enclosure_edal_getU8_thread_safe((fbe_base_enclosure_t *)eses_enclosure,
                                                                FBE_ENCL_COMP_SIDE_ID,  // Attribute.
                                                                FBE_ENCL_POWER_SUPPLY,  // Component type.
                                                                psIndex,                // Component index.
                                                                &sideId);               // The value to be returned.
        if (encl_status != FBE_ENCLOSURE_STATUS_OK)
        {
            fbe_transport_set_status(packet, FBE_ENCLOSURE_STATUS_CMD_FAILED, 0);
            fbe_transport_complete_packet(packet);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        // get the container index
        encl_status = fbe_base_enclosure_edal_getU8_thread_safe((fbe_base_enclosure_t *)eses_enclosure,
                                                                FBE_ENCL_COMP_CONTAINER_INDEX,  // Attribute.
                                                                FBE_ENCL_POWER_SUPPLY,  // Component type.
                                                                psIndex,                // Component index.
                                                                &containerIndex);               // The value to be returned.
        if (encl_status != FBE_ENCLOSURE_STATUS_OK)
        {
            fbe_transport_set_status(packet, FBE_ENCLOSURE_STATUS_CMD_FAILED, 0);
            fbe_transport_complete_packet(packet);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        // if this is the correct sub element
        if((sideId == slotNum) && (containerIndex != FBE_ENCLOSURE_CONTAINER_INDEX_SELF_CONTAINED))
        {
            // get insert status
            encl_status = fbe_base_enclosure_edal_getBool_thread_safe((fbe_base_enclosure_t *)eses_enclosure,
                                                                      FBE_ENCL_COMP_INSERTED,   // Attribute.
                                                                      FBE_ENCL_POWER_SUPPLY,    // Component type.
                                                                      psIndex,                  // Component index.
                                                                      &inserted);               // The value to be returned.

            if (encl_status == FBE_ENCLOSURE_STATUS_OK)
            {
                getPsInfo->bInserted |= inserted;
            }
            else
            {
                fbe_transport_set_status(packet, FBE_ENCLOSURE_STATUS_CMD_FAILED, 0);
                fbe_transport_complete_packet(packet);
                return FBE_STATUS_GENERIC_FAILURE;
            }
            
            // get fault bit
            encl_status = fbe_base_enclosure_edal_getBool_thread_safe((fbe_base_enclosure_t *)eses_enclosure,
                                                            FBE_ENCL_COMP_FAULTED,  // Attribute.
                                                            FBE_ENCL_POWER_SUPPLY,  // Component type.
                                                            psIndex,                // Component index.
                                                            &faulted);              // The value to be returned.
            if (encl_status == FBE_ENCLOSURE_STATUS_OK)
            {
                getPsInfo->generalFault |= faulted;
            }
            else
            {
                fbe_transport_set_status(packet, FBE_ENCLOSURE_STATUS_CMD_FAILED, 0);
                fbe_transport_complete_packet(packet);
                return FBE_STATUS_GENERIC_FAILURE;
            }

            // get ac fail bit
            encl_status = fbe_base_enclosure_edal_getBool_thread_safe((fbe_base_enclosure_t *)eses_enclosure,
                                                            FBE_ENCL_PS_AC_FAIL,    // Attribute.
                                                            FBE_ENCL_POWER_SUPPLY,  // Component type.
                                                            psIndex,                // Component index.
                                                            &acFail);               // The value to be returned.
            if (encl_status == FBE_ENCLOSURE_STATUS_OK)
            {
                getPsInfo->ACFail |= acFail;
                getPsInfo->ACFailDetails[psAcFailIndex] = acFail;
                psAcFailIndex++;
            }
            else
            {
                fbe_transport_set_status(packet, FBE_ENCLOSURE_STATUS_CMD_FAILED, 0);
                fbe_transport_complete_packet(packet);
                return FBE_STATUS_GENERIC_FAILURE;
            }

            // get PS Margining info
            encl_status = fbe_base_enclosure_edal_getU8_thread_safe((fbe_base_enclosure_t *)eses_enclosure,
                                                            FBE_ENCL_PS_MARGIN_TEST_MODE,    // Attribute.
                                                            FBE_ENCL_POWER_SUPPLY,  // Component type.
                                                            psIndex,                // Component index.
                                                            &psMarginTestMode);     // The value to be returned.
            if (encl_status != FBE_ENCLOSURE_STATUS_OK)
            {
                fbe_transport_set_status(packet, FBE_ENCLOSURE_STATUS_CMD_FAILED, 0);
                fbe_transport_complete_packet(packet);
                return FBE_STATUS_GENERIC_FAILURE;
            }
            else
            {
                getPsInfo->psMarginTestMode = psMarginTestMode;
            }
            encl_status = fbe_base_enclosure_edal_getU8_thread_safe((fbe_base_enclosure_t *)eses_enclosure,
                                                            FBE_ENCL_PS_MARGIN_TEST_RESULTS,    // Attribute.
                                                            FBE_ENCL_POWER_SUPPLY,  // Component type.
                                                            psIndex,                // Component index.
                                                            &psMarginTestResults);  // The value to be returned.
            if (encl_status != FBE_ENCLOSURE_STATUS_OK)
            {
                fbe_transport_set_status(packet, FBE_ENCLOSURE_STATUS_CMD_FAILED, 0);
                fbe_transport_complete_packet(packet);
                return FBE_STATUS_GENERIC_FAILURE;
            }
            else
            {
                getPsInfo->psMarginTestResults = psMarginTestResults;
            }

            // Get detailed FW Rev
            encl_status = fbe_base_enclosure_edal_getStr_thread_safe((fbe_base_enclosure_t *)eses_enclosure,
                                            FBE_ENCL_COMP_FW_INFO,
                                            FBE_ENCL_POWER_SUPPLY,      // Component type.
                                            psIndex,                    // Component index.
                                            sizeof(fbe_edal_fw_info_t), 
                                            (char *)&edal_fw_info);     // copy the string here

            if (encl_status != FBE_ENCLOSURE_STATUS_OK)
            {
                fbe_transport_set_status(packet, FBE_ENCLOSURE_STATUS_CMD_FAILED, 0);
                fbe_transport_complete_packet(packet);
                return FBE_STATUS_GENERIC_FAILURE;
            }
            else
            {
                if (revIndex < FBE_ENCLOSURE_MAX_REV_COUNT_PER_PS)
                {
                    getPsInfo->psRevInfo[revIndex].firmwareRevValid = edal_fw_info.valid;
                    getPsInfo->psRevInfo[revIndex].bDownloadable = edal_fw_info.downloadable;
                    fbe_zero_memory(&getPsInfo->psRevInfo[revIndex].firmwareRev[0],
                                    FBE_ENCLOSURE_MAX_FIRMWARE_REV_SIZE + 1);

                    fbe_copy_memory(&getPsInfo->psRevInfo[revIndex].firmwareRev[0],
                                    &edal_fw_info.fwRevision,
                                    FBE_ESES_FW_REVISION_SIZE);
                }
                else
                {
                    fbe_base_object_customizable_trace((fbe_base_object_t *)eses_enclosure,
                                                       FBE_TRACE_LEVEL_WARNING,
                                                       fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                                       "  Rev not copied!RevIndex(%d) out of RANGE!! \n", revIndex);
                }
                revIndex++;
            }
        }

        if((sideId == slotNum) && (containerIndex == FBE_ENCLOSURE_CONTAINER_INDEX_SELF_CONTAINED))
        {
            // Get the PS firmware rev (the lowest rev of the MCUs)
            encl_status = fbe_base_enclosure_edal_getStr_thread_safe((fbe_base_enclosure_t *)eses_enclosure,
                                            FBE_ENCL_COMP_FW_INFO,
                                            FBE_ENCL_POWER_SUPPLY,      // Component type.
                                            psIndex,                    // Component index.
                                            sizeof(fbe_edal_fw_info_t), 
                                            (char *)&edal_fw_info);     // copy the string here

            if (encl_status == FBE_ENCLOSURE_STATUS_OK)
            {
                fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                                       FBE_TRACE_LEVEL_INFO,
                                                       fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                                       "psIndex:%d, getPSrev %.16s\n", 
                                                       psIndex,
                                                       &edal_fw_info.fwRevision[0]);
                getPsInfo->firmwareRevValid = edal_fw_info.valid;

                fbe_zero_memory(&getPsInfo->firmwareRev[0],
                                FBE_ENCLOSURE_MAX_FIRMWARE_REV_SIZE + 1);

                fbe_copy_memory(&getPsInfo->firmwareRev[0],
                                &edal_fw_info.fwRevision,
                                FBE_ESES_FW_REVISION_SIZE);
            }
            else
            {
                fbe_transport_set_status(packet, FBE_ENCLOSURE_STATUS_CMD_FAILED, 0);
                fbe_transport_complete_packet(packet);
                return FBE_STATUS_GENERIC_FAILURE;
            }

            // Get Subencl product id.
            encl_status = fbe_base_enclosure_edal_getStr_thread_safe((fbe_base_enclosure_t *)eses_enclosure,
                                            FBE_ENCL_SUBENCL_PRODUCT_ID,
                                            FBE_ENCL_POWER_SUPPLY,      // Component type.
                                            psIndex,                    // Component index.
                                            FBE_ESES_CONFIG_SUBENCL_PRODUCT_ID_SIZE,
                                            &subenclProductId[0]);
        
            if (encl_status == FBE_ENCLOSURE_STATUS_OK)
            {
                fbe_zero_memory(&getPsInfo->subenclProductId[0],
                                FBE_ENCLOSURE_MAX_SUBENCL_PRODUCT_ID_SIZE + 1);

                fbe_copy_memory(&getPsInfo->subenclProductId[0],
                                 &subenclProductId[0],
                                 FBE_ESES_CONFIG_SUBENCL_PRODUCT_ID_SIZE);
            }
            else
            {
                fbe_transport_set_status(packet, FBE_ENCLOSURE_STATUS_CMD_FAILED, 0);
                fbe_transport_complete_packet(packet);
                return FBE_STATUS_GENERIC_FAILURE;
            }
        
            getPsInfo->uniqueIDFinal = TRUE;
            fbe_eses_enclosure_convert_subencl_product_id_to_unique_id((fbe_eses_enclosure_t *)eses_enclosure,
                                                                       &getPsInfo->subenclProductId[0], 
                                                                       &getPsInfo->uniqueId);
        }
    }

    // Check whether it is downloadable.
    if(fbe_eses_enclosure_is_ps_downloadable(eses_enclosure, 
                                        slotNum,
                                        &getPsInfo->firmwareRev[0],
                                        getPsInfo->uniqueId))
    {
        getPsInfo->bDownloadable = TRUE;
    }
    else
    {
        getPsInfo->bDownloadable = FALSE;
    }

    /* Get PS AC/DC type */
    getPsInfo->ACDCInput = fbe_eses_enclosure_get_ps_ac_dc_type(eses_enclosure, getPsInfo->uniqueId);

    /* Get the ps internal fan info.
     */ 
    targetSubtype = FBE_ENCL_COOLING_SUBTYPE_PS_INTERNAL;

    // Get the total number of Cooling Components including the overall elements.
    status = fbe_base_enclosure_get_number_of_components((fbe_base_enclosure_t *)eses_enclosure,
                                                             FBE_ENCL_COOLING_COMPONENT,
                                                             &coolingComponentCount);
    if(status != FBE_STATUS_OK)
    {
        fbe_transport_set_status(packet, status, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    getPsInfo->internalFanFault = FBE_MGMT_STATUS_FALSE;

    for (coolingIndex = 0; coolingIndex < coolingComponentCount; coolingIndex++)
    {
        /* Get subtype. */
        encl_status = fbe_base_enclosure_edal_getU8_thread_safe((fbe_base_enclosure_t *)eses_enclosure,
                                                               FBE_ENCL_COMP_SUBTYPE,
                                                               FBE_ENCL_COOLING_COMPONENT,
                                                               coolingIndex,
                                                               &subtype);
        if (encl_status != FBE_ENCLOSURE_STATUS_OK)
        {
            fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
            fbe_transport_complete_packet(packet);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        /* Get containerIndex */
        encl_status = fbe_base_enclosure_edal_getU8_thread_safe((fbe_base_enclosure_t *)eses_enclosure,
                                                               FBE_ENCL_COMP_CONTAINER_INDEX,
                                                               FBE_ENCL_COOLING_COMPONENT,
                                                               coolingIndex,
                                                               &containerIndex);
        if (encl_status != FBE_ENCLOSURE_STATUS_OK)
        {
            fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
            fbe_transport_complete_packet(packet);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        /* Get sideId. */
        encl_status = fbe_base_enclosure_edal_getU8_thread_safe((fbe_base_enclosure_t *)eses_enclosure,
                                                               FBE_ENCL_COMP_SIDE_ID, // Attribute.
                                                               FBE_ENCL_COOLING_COMPONENT, // Component type.
                                                               coolingIndex, // Component index.
                                                               &sideId); // The value to be returned.
        if (encl_status != FBE_ENCLOSURE_STATUS_OK)
        {
            fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
            fbe_transport_complete_packet(packet);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        
        if((subtype == targetSubtype) &&
           (sideId == slotNum) &&
           (containerIndex != FBE_ENCLOSURE_CONTAINER_INDEX_SELF_CONTAINED))
        {
            /**/

            encl_status = fbe_base_enclosure_edal_getBool_thread_safe((fbe_base_enclosure_t *)eses_enclosure,
                                                            FBE_ENCL_COMP_FAULTED, // Attribute.
                                                            FBE_ENCL_COOLING_COMPONENT,  // Component type.
                                                            coolingIndex, // Component index.
                                                            &faulted); // The value to be returned.

            if (encl_status != FBE_ENCLOSURE_STATUS_OK)
            {
                fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
                fbe_transport_complete_packet(packet);
                return FBE_STATUS_GENERIC_FAILURE;
            }
            
            if (faulted)
            {
                getPsInfo->internalFanFault = FBE_MGMT_STATUS_TRUE; 
            }
      
        }
    }
    
    // determine which SP & SPS the PS is associated with
    encl_status = fbe_eses_enclosure_get_local_lcc_side_id(eses_enclosure, &sp_id);
    if (encl_status != FBE_ENCLOSURE_STATUS_OK)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                            FBE_TRACE_LEVEL_WARNING,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                            "%s, get_local_lcc_side_id failed.\n", __FUNCTION__); 

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* We could get the psSide(not psSlot) from ESES configuration page.
     * However we already use FBE_ENCL_COMP_SIDE_ID for psSlot.
     * My another concern is that CDES also uses side id 0, 1, 2, 3 for Voyager LCC 0, 1, 2 ,3.
     * So I am not sure whether one day CDES would use side id 0, 1, 2, 3 for power supplies as well. 
     * So for simplicity, I just use this approach to get the psSide (It is A side or B side, not slot).
     */
    if ((psCountPerSide = fbe_eses_enclosure_get_number_of_power_supplies_per_side(eses_enclosure)) == 0)
    {
        psCountPerSide = 1;
    }

    /* When the Naga enclosure is connected as a boot enclosure,
     * the power supplies in slot 0(PSA1) and slot 1(PSA0) are connected to SPS A.
     * and the power supplies in slot 2(PSB1) and slot 3(PSB0) are connected to SPS B.
     */
    psSide = slotNum/psCountPerSide;
    if(psSide == SP_A)
    {
        getPsInfo->associatedSps = SP_A;
        getPsInfo->associatedSp = SP_A;
    }
    else
    {
        getPsInfo->associatedSps = SP_B;
        getPsInfo->associatedSp = SP_B;
    }

    if (psCountPerSide > 1)
    {
        // The power supplies in slot 0(PSA1) and slot 2(PSB1) supply the power to LCC A
        // The power supplies in slot 1(PSA0) and slot 3(PSB0) supply the power to LCC B
        psSlotNumBasedOnSide = slotNum % psCountPerSide; 
        getPsInfo->slotNumOnSpBlade = psSlotNumBasedOnSide;
        if(psSlotNumBasedOnSide == 0)
        {  
            getPsInfo->isLocalFru = (sp_id == SP_A) ? FBE_TRUE : FBE_FALSE;

        }
        else
        {
            getPsInfo->isLocalFru = (sp_id == SP_A) ? FBE_FALSE : FBE_TRUE;
        }
    }
    else
    {
        /* If this is not handled by the leaf class and come here. 
         * We consider the PS supplies the power to both LCCs.
         */
        getPsInfo->isLocalFru = FBE_TRUE; 
    }
    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                       FBE_TRACE_LEVEL_DEBUG_MEDIUM,
                                       fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                       "   slot:%d CountperSide:%d SlotBasedOnSide:%d associatedSP:%d isLocalFru:%d \n", 
                                       slotNum,
                                       psCountPerSide,
                                       psSlotNumBasedOnSide,
                                       getPsInfo->associatedSp,
                                       getPsInfo->isLocalFru);

    getPsInfo->ambientOvertempFault = FBE_MGMT_STATUS_NA;

    fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                        FBE_TRACE_LEVEL_DEBUG_HIGH,
                                        fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                        "%s, ps %d, insert %d, fault %d, acFail %d\n", 
                                        __FUNCTION__,
                                        getPsInfo->slotNumOnEncl,
                                        getPsInfo->bInserted,
                                        getPsInfo->generalFault,
                                        getPsInfo->ACFail);

    /* Set the control operation status qualifier with the encl_status. */
    fbe_payload_control_set_status_qualifier(control_operation, encl_status);
    /* Complete the packet. */
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);

    return FBE_STATUS_OK;
    
} // End of function - fbe_eses_enclosure_process_getPsInfo


/*!*******************************************************************************
 * @fn fbe_eses_enclosure_convert_subencl_product_id_to_unique_id(fbe_eses_enclosure_t * eses_enclosure,
 *                                        fbe_char_t * pSubenclProductId,
 *                                        HW_MODULE_TYPE * pUniqueId)
 *********************************************************************************
 * @brief
 *  Converts the subenclosure product id to the unique id(ffid).
 *
 * @param  eses_enclosure(INPUT) - The pointer to fbe_eses_enclosure_t.
 * @param  pSubenclProductId(INPUT) - The pointer to the subenclosure product id.
 * @return pUniqueId(OUTPUT) - The pointer to HW_MODULE_TYPE(ffid).
 *
 * @return fbe_status_t - always return FBE_STATUS_OK.
 *
 * @note pPsSubenclProductId has to point to the NULL-terminated string.
 *
 * @version
 *  01-April-2011 PHE - Created.
 *******************************************************************************/
fbe_status_t
fbe_eses_enclosure_convert_subencl_product_id_to_unique_id(fbe_eses_enclosure_t * eses_enclosure,
                                                           fbe_char_t * pSubenclProductId,
                                                           HW_MODULE_TYPE * pUniqueId)
{
    fbe_u32_t    pid = 0;

    /* ESES SPEC states that if the subenclosure is not present or 
     * its product identification is inaccessible to this EMA,
     * subencl product is would be all spaces.
     */
    if(fbe_equal_memory(pSubenclProductId, "     ", 5))
    {
        *pUniqueId = NO_MODULE;
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                                FBE_TRACE_LEVEL_INFO,
                                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                                "%s, subencl product id not availabe, uniqueId 0x%x.\n", 
                                                __FUNCTION__,
                                                *pUniqueId);
    }
    else
    {
        /* Convert the ASCII ffid to Hex. */
        pid = fbe_eses_enclosure_convert_string_to_hex(pSubenclProductId);
    
        /* Power supply product id is actually 4 bytes.
         * swap bytes to match familyfruid.h 
         * Unique id is the ffid.
         */
        *pUniqueId = (((pid&0xFF000000)>>8) | 
                  ((pid&0xFF0000)<< 8)|
                    ((pid&0xFF00)>> 8)|
                      ((pid&0xFF)<< 8));
    }
    
    return FBE_STATUS_OK;
} //fbe_eses_enclosure_convert_subencl_product_id_to_unique_id

/*!*******************************************************************************
 * @fn fbe_eses_enclosure_lcc_lookup_ffid(fbe_eses_enclosure_t * eses_enclosure,
 *                                        fbe_char_t * pSubenclProductId,
 *                                        HW_MODULE_TYPE * pUniqueId)
 *********************************************************************************
 * @brief
 *  Converts the subenclosure product id to the unique id(ffid).
 *
 * @param  eses_enclosure(INPUT) - The pointer to fbe_eses_enclosure_t.
 * @param  pSubenclProductId(INPUT) - The pointer to the subenclosure product id.
 * @return pUniqueId(OUTPUT) - The pointer to HW_MODULE_TYPE(ffid).
 *
 * @return fbe_status_t - always return FBE_STATUS_OK.
 *
 * @note pPsSubenclProductId has to point to the NULL-terminated string.
 *
 * @version
 *  01-April-2011 PHE - Created.
 *******************************************************************************/
static fbe_status_t
fbe_eses_enclosure_lcc_lookup_ffid(fbe_eses_enclosure_t * eses_enclosure,
                                                       fbe_char_t * pSubenclProductId,
                                                       HW_MODULE_TYPE *pffid)
{
    /* ESES SPEC states that if the subenclosure is not present or 
     * its product identification is inaccessible to this EMA,
     * subencl product is would be all spaces.
     */
    *pffid = NO_MODULE;
    if(fbe_equal_memory(pSubenclProductId, "     ", 5))
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                                FBE_TRACE_LEVEL_INFO,
                                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                                "%s, subencl product id %s not availabe, uniqueId 0x%x.\n", 
                                                __FUNCTION__,
                                                pSubenclProductId,
                                                *pffid);
        return FBE_STATUS_OK;
    }


    switch(eses_enclosure->sas_enclosure_type)
    {
    case FBE_SAS_ENCLOSURE_TYPE_BUNKER:
    case FBE_SAS_ENCLOSURE_TYPE_CITADEL:
        if(strncmp((char *)pSubenclProductId, FBE_ESES_ENCL_BUNKER_CITADEL_LCC_PRODUCT_ID_STR, 11) == 0)
        {
            // this is Sentry board
            // we only care about the sentry family identifier, mask off the remaining bits because 
            // we don't know which sentry board
            *pffid = (SENTRY_SP_2_0_GHZ_GROWLER_0_GB) & (0xFFFF << 16);
        }
        break;
    case FBE_SAS_ENCLOSURE_TYPE_BOXWOOD:
    case FBE_SAS_ENCLOSURE_TYPE_KNOT:
        if(strncmp((char *)pSubenclProductId, FBE_ESES_ENCL_BOXWOOD_KNOT_LCC_PRODUCT_ID_STR, 11) == 0)
        {
            // this is beachcomber board
            // we only care about the beachcomber family identifier, mask off the remaining bits because 
            // we don't know which beachcomber board
            *pffid = BEACHCOMBER_PCB;
        }
        break;
    case FBE_SAS_ENCLOSURE_TYPE_STEELJAW:
    case FBE_SAS_ENCLOSURE_TYPE_RAMHORN:
        if(strncmp((char *)pSubenclProductId, FBE_ESES_ENCL_STEELJAW_RAMHORN_LCC_PRODUCT_ID_STR, 11) == 0)
        {
            // this is beachcomber board
            // we only care about the beachcomber family identifier, mask off the remaining bits because 
            // we don't know which beachcomber board
            *pffid = BEACHCOMBER_PCB;
        }
        break;
    case FBE_SAS_ENCLOSURE_TYPE_RHEA:
    case FBE_SAS_ENCLOSURE_TYPE_MIRANDA:
        if(strncmp((char *)pSubenclProductId, FBE_ESES_ENCL_RHEA_MIRANDA_LCC_PRODUCT_ID_STR, 11) == 0)
        {
            // this is beachcomber board
            // we only care about the beachcomber family identifier, mask off the remaining bits because 
            // we don't know which beachcomber board
// ENCL_CLEANUP - correct?
            *pffid = OBERON_SP_85W_REV_A;
        }
        break;
    case FBE_SAS_ENCLOSURE_TYPE_PINECONE:
        if(strncmp((char *)pSubenclProductId, FBE_ESES_ENCL_PINECONE_LCC_PRODUCT_ID_STR, 11) == 0)
        {
            // this is Pinecone LCC
            *pffid = PINECONE_LCC;
        }
        break;
    case FBE_SAS_ENCLOSURE_TYPE_FALLBACK:
        if(strncmp((char *)pSubenclProductId, FBE_ESES_ENCL_FALLBACK_LCC_PRODUCT_ID_STR, 11) == 0)
        {
            // this is jetfire board
            // we only care about the jetfire family identifier, mask off the remaining bits because 
            // we don't know which jetfire board
            *pffid = JETFIRE_SP_SANDYBRIDGE;
        }
        break;
    case FBE_SAS_ENCLOSURE_TYPE_CALYPSO:
        if(strncmp((char *)pSubenclProductId, FBE_ESES_ENCL_LAPETUS_LCC_PRODUCT_ID_STR, 11) == 0)
        {
            // this is jetfire board
            // we only care about the jetfire family identifier, mask off the remaining bits because 
            // we don't know which jetfire board
// ENCL_CLEANUP - correct?
            *pffid = HYPERION_SP_REV_A;
        }
        break;
    case FBE_SAS_ENCLOSURE_TYPE_DERRINGER:
        if(strncmp((char *)pSubenclProductId, FBE_ESES_ENCL_DERRINGER_LCC_PRODUCT_ID_STR, 11) == 0)
        {
            // this is Derringer EE board
            *pffid = DERRINGER_LCC;
        }
        break;
    case FBE_SAS_ENCLOSURE_TYPE_TABASCO:
        if(strncmp((char *)pSubenclProductId, FBE_ESES_ENCL_TABASCO_LCC_PRODUCT_ID_STR, 11) == 0)
        {
            // this is Tabasco EE board
            *pffid = TABASCO_LCC;
        }
        break;
    case FBE_SAS_ENCLOSURE_TYPE_VIPER:
        if(strncmp((char *)pSubenclProductId, FBE_ESES_ENCL_VIPER_LCC_PRODUCT_ID_STR, 11) == 0)
        {
            // this is Viper LCC
            *pffid = VIPER_LCC;
        }
        break;    
    case FBE_SAS_ENCLOSURE_TYPE_ANCHO:
        if(strncmp((char *)pSubenclProductId, FBE_ESES_ENCL_ANCHO_LCC_PRODUCT_ID_STR, 11) == 0)
        {
            // this is Ancho LCC
            *pffid = ANCHO_LCC;
        }
        break;
    case FBE_SAS_ENCLOSURE_TYPE_VOYAGER_EE:
        if(strncmp((char *)pSubenclProductId, FBE_ESES_ENCL_VOYAGER_EE_PRODUCT_ID_STR, 11) == 0)
        {
            // this is Voyager EE board
            *pffid = VOYAGER_LCC;
        }
        break;
    case FBE_SAS_ENCLOSURE_TYPE_VOYAGER_ICM:
        if(strncmp((char *)pSubenclProductId, FBE_ESES_ENCL_VOYAGER_ICM_PRODUCT_ID_STR, 11) == 0)
        {
            // this is Voyager ICM
            *pffid = VOYAGER_ICM;

        }
        else if(strncmp((char *)pSubenclProductId, FBE_ESES_ENCL_VOYAGER_EE_PRODUCT_ID_STR, 11) == 0)
        {
            // this is Voyager EE board
            *pffid = VOYAGER_LCC;
        }
            
        break;
    case FBE_SAS_ENCLOSURE_TYPE_VIKING_IOSXP:
        if(strncmp((char *)pSubenclProductId, FBE_ESES_ENCL_VIKING_IOSXP_LCC_PRODUCT_ID_STR, 16) == 0)
        {
            *pffid = VIKING_IO_EXP;
        }
        break;
    case FBE_SAS_ENCLOSURE_TYPE_VIKING_DRVSXP:
        if(strncmp((char *)pSubenclProductId, FBE_ESES_ENCL_VIKING_DRVSXP_LCC_PRODUCT_ID_STR, 16) == 0)
        {
            *pffid = VIKING_LCC;
        }
        break;
    case FBE_SAS_ENCLOSURE_TYPE_CAYENNE_IOSXP:
        if(strncmp((char *)pSubenclProductId, FBE_ESES_ENCL_CAYENNE_IOSXP_LCC_PRODUCT_ID_STR, 16) == 0)
        {
            *pffid = CAYENNE_LCC;
        }
        break;
    case FBE_SAS_ENCLOSURE_TYPE_CAYENNE_DRVSXP:
        if(strncmp((char *)pSubenclProductId, FBE_ESES_ENCL_CAYENNE_DRVSXP_LCC_PRODUCT_ID_STR, 16) == 0)
        {
            *pffid = CAYENNE_LCC;
        }
        break;

    case FBE_SAS_ENCLOSURE_TYPE_NAGA_IOSXP:
        if(strncmp((char *)pSubenclProductId, FBE_ESES_ENCL_NAGA_IOSXP_LCC_PRODUCT_ID_STR, 16) == 0)
        {
            *pffid = NAGA_LCC;
        }
        break;

    case FBE_SAS_ENCLOSURE_TYPE_NAGA_DRVSXP:
        if(strncmp((char *)pSubenclProductId, FBE_ESES_ENCL_NAGA_DRVSXP_LCC_PRODUCT_ID_STR, 16) == 0)
        {
            *pffid = NAGA_LCC;
        }
        break;
    default:
        // default is already set to NO_MODULE
        break;
    }
    
    return FBE_STATUS_OK;
} //fbe_eses_enclosure_lcc_lookup_ffid

/*!*******************************************************************************
 * @fn fbe_eses_enclosure_is_ps_downloadable(fbe_eses_enclosure_t *eses_enclosure, 
 *                                    fbe_u32_t psSlot,                                    
 *                                    fbe_char_t * pPsFirmwareRev,
 *                                    HW_MODULE_TYPE  pUniqueId)
 *********************************************************************************
 * @brief
 *  Check whether the auto upgrade is supported by 
 *  comparing the firmware rev with the minimum acceptable firmware rev.
 *
 * @param  eses_enclosure - The pointer to the fbe_eses_enclosure_t. 
 * @param  psSlot- The PS slot.
 * @param  pPsFirmwareRev - The pointer to the PS firmware rev.
 * @param  psUniqueId - The PS unique id (ffid).
 *
 * @return TRUE - ps is downloadable.
 *         FALSE - ps is not downloadable.
 *
 * @note pPsSubenclProductId and pPsFirmwareRev have to point to the NULL-terminated string.
 *
 * @version
 *  15-June-2010 PHE - Created.
 *******************************************************************************/
fbe_bool_t
fbe_eses_enclosure_is_ps_downloadable(fbe_eses_enclosure_t *eses_enclosure,   
                                      fbe_u32_t psSlot,
                                      fbe_char_t * pPsFirmwareRev,
                                      HW_MODULE_TYPE psUniqueId)
{
    fbe_u8_t     component_index = 0;
    fbe_u8_t     minAcceptablePsRev[FBE_ESES_FW_REVISION_SIZE + 1] = {0};
    fbe_enclosure_status_t enclStatus = FBE_ENCLOSURE_STATUS_OK;
    fbe_edal_fw_info_t                  edal_fw_info;
    fbe_u8_t     psCountPerSide = 0;
    fbe_u8_t     psSide = 0;
    fbe_eses_enclosure_format_fw_rev_t psFwRevAfterFormatting = {0};

    /* We could get the psSide(not psSlot) from ESES configuration page.
     * However we already use FBE_ENCL_COMP_SIDE_ID for psSlot.
     * My another concern is that CDES also uses side id 0, 1, 2, 3 for Voyager LCC 0, 1, 2 ,3.
     * So I am not sure whether one day CDES would use side id 0, 1, 2, 3 for power supplies as well. 
     * So for simplicity, I just use this approach to get the psSide (It is A side or B side, not slot).
     */
    if ((psCountPerSide = fbe_eses_enclosure_get_number_of_power_supplies_per_side(eses_enclosure)) == 0)
    {
        psCountPerSide = 1;
    }

    psSide = psSlot/psCountPerSide;

    enclStatus = fbe_eses_enclosure_map_fw_target_type_to_component_index(eses_enclosure, 
                                                                      FBE_ENCL_LCC,
                                                                      psSide,
                                                                      0, // Overall Rev.
                                                                      &component_index);
    if (enclStatus != FBE_ENCLOSURE_STATUS_OK)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                       FBE_TRACE_LEVEL_ERROR,
                       fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                       "unsupported firmware target %d, component type %d\n", 
                       FBE_FW_TARGET_LCC_MAIN, FBE_ENCL_LCC);

        return FBE_FALSE;
    }

    // Get CDES bundle firmware FW.
    enclStatus = fbe_base_enclosure_edal_getStr_thread_safe((fbe_base_enclosure_t *)eses_enclosure,
                                    FBE_ENCL_COMP_FW_INFO,// Component Attribute
                                    FBE_ENCL_LCC,  // Component type.
                                    component_index, // Component index.
                                    sizeof(fbe_edal_fw_info_t), 
                                    (char *)&edal_fw_info);     // copy the string here

    if(enclStatus != FBE_ENCLOSURE_STATUS_OK)
    {
        return FBE_FALSE;
    }
    else if(eses_enclosure->eses_version_desc == ESES_REVISION_1_0) 
    {
        if(fbe_eses_enclosure_at_lower_rev(eses_enclosure, &edal_fw_info.fwRevision[0], FBE_ESES_ENCL_PS_FUP_MIN_REQUIRED_CDES_REV_STR, FBE_ESES_FW_REVISION_SIZE))
        {
            return FBE_FALSE;
        }
    }

    switch(psUniqueId)
    {
        case AC_ACBEL_DAE_GEN3:
        case AC_EMERSON_DAE_GEN3:
            strncpy(&minAcceptablePsRev[0], 
                    FBE_ESES_ENCL_PS_FUP_MIN_REQUIRED_GEN3_PS_REV_STR, 
                    FBE_ESES_FW_REVISION_SIZE);           
            break;

        case AC_ACBEL_DERRINGER_071:
        case DERRINGER_EMERSON:
             strncpy(&minAcceptablePsRev[0], 
                     FBE_ESES_ENCL_PS_FUP_MIN_REQUIRED_DERRINGER_PS_REV_STR, 
                     FBE_ESES_FW_REVISION_SIZE);
            break;

        case TABASCO_PS_COOLING_MODULE:
             strncpy(&minAcceptablePsRev[0], 
                     FBE_ESES_ENCL_PS_FUP_MIN_REQUIRED_TABASCO_PS_REV_STR, 
                     FBE_ESES_FW_REVISION_SIZE);
            break;
        case AC_JUNO_2:                         //cayenne
            strncpy(&minAcceptablePsRev[0], 
                     FBE_ESES_ENCL_PS_FUP_MIN_REQUIRED_JUNO2_PS_REV_STR, 
                     FBE_ESES_FW_REVISION_SIZE);
            break;
        case ACBEL_3GEN_VE_DAE:                 //ancho
            strncpy(&minAcceptablePsRev[0], 
                     FBE_ESES_ENCL_PS_FUP_MIN_REQUIRED_ACBEL_3GVE_PS_REV_STR, 
                     FBE_ESES_FW_REVISION_SIZE);
            break;
        case EMERSON_3GEN_VE_DAE:               //ancho
            strncpy(&minAcceptablePsRev[0], 
                     FBE_ESES_ENCL_PS_FUP_MIN_REQUIRED_EMER_3GVE_PS_REV_STR, 
                     FBE_ESES_FW_REVISION_SIZE);
            break;

        case AC_ACBEL_VOYAGER:
        case VOYAGER_EMERSON:
            strncpy(&minAcceptablePsRev[0], 
                    FBE_ESES_ENCL_PS_FUP_MIN_REQUIRED_VOYAGER_PS_REV_STR, 
                    FBE_ESES_FW_REVISION_SIZE);
            break;

        case AC_DC_JUNIPER:
            strncpy(&minAcceptablePsRev[0], 
                    FBE_ESES_ENCL_PS_FUP_MIN_REQUIRED_JUNIPER_PS_REV_STR, 
                    FBE_ESES_FW_REVISION_SIZE);
            break;

        case DAE_AC_1080: //viking, naga
            strncpy(&minAcceptablePsRev[0], 
                        FBE_ESES_ENCL_PS_FUP_MIN_REQUIRED_AC_1080_PS_REV_STR, 
                        FBE_ESES_FW_REVISION_SIZE);
            break;
        case AC_OPTIMUS: // naga
            strncpy(&minAcceptablePsRev[0], 
                        FBE_ESES_ENCL_PS_FUP_MIN_REQUIRED_OPTIMUS_ACBEL_PS_REV_STR, 
                        FBE_ESES_FW_REVISION_SIZE);
            break;
        case AC_FLEXPOWER_OPTIMUS: //naga
            strncpy(&minAcceptablePsRev[0], 
                        FBE_ESES_ENCL_PS_FUP_MIN_REQUIRED_OPTIMUS_FLEX_PS_REV_STR, 
                        FBE_ESES_FW_REVISION_SIZE);
            break;
        case DC_TABASCO_PS_COOLING_MODULE: //tabasco
            strncpy(&minAcceptablePsRev[0],
                    FBE_ESES_ENCL_PS_FUP_MIN_REQUIRED_TABASCO_DC_PS_REV_STR,
                    FBE_ESES_FW_REVISION_SIZE);
            break;
        case AC_ACBEL_OPTIMUS_GEN_1_PLUS: // optimus acbel2 - naga
            strncpy(&minAcceptablePsRev[0],
                    FBE_ESES_ENCL_PS_FUP_MIN_REQUIRED_TABASCO_DC_PS_REV_STR,
                    FBE_ESES_FW_REVISION_SIZE);
            break;
        case AC_FLEXPOWER_OPTIMUS_GEN_1_PLUS: // optimus flex2 - naga
            strncpy(&minAcceptablePsRev[0],
                    FBE_ESES_ENCL_PS_FUP_MIN_REQUIRED_OPTIMUS_FLEX2_PS_REV_STR,
                    FBE_ESES_FW_REVISION_SIZE);
            break;
        default:
            fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                                FBE_TRACE_LEVEL_INFO,
                                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                                "%s, unhandled ps uniqueId 0x%x, rev %s.\n", 
                                                __FUNCTION__,
                                                psUniqueId,
                                                pPsFirmwareRev);
            return FBE_FALSE;
            break;
    }

    fbe_eses_enclosure_format_fw_rev(eses_enclosure, &psFwRevAfterFormatting, pPsFirmwareRev, FBE_ESES_FW_REVISION_SIZE);
                                                  
    if((psFwRevAfterFormatting.majorNumber == 0) && 
       (psFwRevAfterFormatting.majorNumber == 0) &&
       (psFwRevAfterFormatting.majorNumber == 0))
    {
        /* No rev is availale. Set to TRUE temporarily because 
        * the configuration page used by terminator does not have the rev populated.
        * But we want to do the upgrade for the power supply.
        */
        return FBE_TRUE;
    }
    else if(fbe_eses_enclosure_at_lower_rev(eses_enclosure, pPsFirmwareRev, &minAcceptablePsRev[0], FBE_ESES_FW_REVISION_SIZE))
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                            FBE_TRACE_LEVEL_INFO,
                                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                            "%s, PsFwrev %s, minPsFwRev %s, PS fw is NOT downloadable.\n", 
                                            __FUNCTION__,
                                            pPsFirmwareRev,
                                            &minAcceptablePsRev[0]);

        return FBE_FALSE;
    }

    return FBE_TRUE;

} //fbe_eses_enclosure_is_ps_downloadable

/*!*************************************************************************
* @fn fbe_eses_enclosure_parse_image_header()                  
***************************************************************************
* @brief
*       This function parses the mcode image header to return the image rev and 
*       whether there is a subencl product id in the subencl product id list
*       of the image header which matches the specified subencl product id.
*
* @param     eses_enclosure - The pointer to the fbe_eses_enclosure_t.
* @param     packet - The pointer to the fbe_packet_t.
*
* @return    fbe_status_t.
*
* @version
*   22-June-2010     PHE - Created.
***************************************************************************/
static fbe_status_t 
fbe_eses_enclosure_parse_image_header(fbe_eses_enclosure_t *eses_enclosure, 
                                            fbe_packet_t *packet)
{
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;   
    fbe_payload_control_buffer_t control_buffer = NULL;    
    fbe_payload_control_buffer_length_t control_buffer_length = 0;
    fbe_enclosure_mgmt_parse_image_header_t * pParseImageHeader = NULL;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t byteOffset = 0;
    fbe_u8_t  numOfSubEnclProductId = 0;
    fbe_u8_t  index = 0;
    fbe_u8_t * pImageHeader = NULL;
    fbe_u32_t sizeTemp = 0;
    fbe_u32_t mcodeDataSizeTemp = 0;
    ses_comp_type_enum fwCompTypeTemp = SES_COMP_TYPE_UNKNOWN;
    ses_comp_type_enum fwCompType = SES_COMP_TYPE_UNKNOWN;

    /********
     * BEGIN
     ********/

    /* The original packet status is FBE_STATUS_INVALID (initialized in fbe_transport_initialize_packet)
     * we set it to FBE_STATUS_OK to start with. 
     * It will be updated as needed later on. 
     */
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);

    payload = fbe_transport_get_payload_ex(packet);
    if(payload == NULL)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                            FBE_TRACE_LEVEL_ERROR,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                            "%s, get_payload failed.\n", __FUNCTION__); 

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    control_operation = fbe_payload_ex_get_control_operation(payload);
    if(control_operation == NULL)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                            FBE_TRACE_LEVEL_ERROR,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                            "%s, get_control_operation failed.\n", __FUNCTION__); 

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status =  fbe_payload_control_get_buffer(control_operation, &control_buffer);
    if( (control_buffer == NULL) || (status!=FBE_STATUS_OK))
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                               FBE_TRACE_LEVEL_WARNING,
                               fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                               "%s, get_buffer failed, status: 0x%x, control_buffer: %64p.\n",
                               __FUNCTION__, status, control_buffer);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }
  
    pParseImageHeader = (fbe_enclosure_mgmt_parse_image_header_t *)control_buffer;

    /* Verify the control buffer size with the size of the expected structure */
    status = fbe_payload_control_get_buffer_length(control_operation, &control_buffer_length);
    if((status != FBE_STATUS_OK) || 
       (control_buffer_length != sizeof(fbe_enclosure_mgmt_parse_image_header_t)))
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                               FBE_TRACE_LEVEL_WARNING,
                               fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                "%s, get_buffer_length, bufferLen %d, expectedLen: %llu, status: 0x%x.\n", 
                                __FUNCTION__,
                                control_buffer_length, 
                                (unsigned long long)sizeof(fbe_enclosure_mgmt_parse_image_header_t), 
                                status);
        
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    } 

    pImageHeader = &pParseImageHeader->imageHeader[0];

    if(eses_enclosure->eses_version_desc == ESES_REVISION_1_0) 
    {
        numOfSubEnclProductId = pImageHeader[FBE_ESES_MCODE_NUM_SUBENCL_PRODUCT_ID_OFFSET];
        if(numOfSubEnclProductId > FBE_ENCLOSURE_MAX_NUM_SUBENCL_PRODUCT_ID_IN_IMAGE)
        {
            fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                               FBE_TRACE_LEVEL_ERROR,
                                               fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                               "%s, Too many subencl prod ids act:%d, limit:%d\n",
                                                __FUNCTION__,
                                               numOfSubEnclProductId,
                                               FBE_ENCLOSURE_MAX_NUM_SUBENCL_PRODUCT_ID_IN_IMAGE); 
            fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
            fbe_transport_complete_packet(packet);
            return FBE_STATUS_GENERIC_FAILURE;
    
        }
        /* Loop through the subencl product id list in the image header to 
         * save all the subEnclosureProductid. 
         */
        for(index = 0; index < numOfSubEnclProductId; index ++)
        {
            byteOffset = FBE_ESES_MCODE_SUBENCL_PRODUCT_ID_OFFSET + (index * FBE_ESES_CONFIG_SUBENCL_PRODUCT_ID_SIZE);
    
            fbe_copy_memory(&pParseImageHeader->subenclProductId[index][0], 
                            &pImageHeader[byteOffset], 
                            FBE_ESES_CONFIG_SUBENCL_PRODUCT_ID_SIZE);
        }
    
        fwCompType = pImageHeader[FBE_ESES_MCODE_IMAGE_COMPONENT_TYPE_OFFSET];

        fbe_eses_enclosure_eses_comp_type_to_fw_target(fwCompType,
                                        &pParseImageHeader->firmwareTarget);
        
        // save the header size, used to find the start of the image,
        // only used for board obj but copy it here
        fbe_copy_memory((fbe_u8_t *)&sizeTemp, 
                         pImageHeader + FBE_ESES_MCODE_IMAGE_HEADER_SIZE_OFFSET,
                         FBE_ESES_MCODE_IMAGE_HEADER_SIZE_BYTES);
        pParseImageHeader->headerSize = swap32(sizeTemp);
    
        /* Save image rev. */
        fbe_copy_memory(&pParseImageHeader->imageRev[0], 
                        &pImageHeader[FBE_ESES_MCODE_IMAGE_REV_OFFSET], 
                        FBE_ESES_MCODE_IMAGE_REV_SIZE_BYTES);
        
        /* Save image size. */
        fbe_copy_memory((fbe_u8_t *)&sizeTemp, 
                         pImageHeader + FBE_ESES_MCODE_IMAGE_SIZE_OFFSET,
                         FBE_ESES_MCODE_IMAGE_SIZE_BYTES);
        pParseImageHeader->imageSize = swap32(sizeTemp);
    }
    else if(eses_enclosure->eses_version_desc == ESES_REVISION_2_0) 
    {
        numOfSubEnclProductId = pImageHeader[FBE_ESES_CDES2_MCODE_NUM_SUBENCL_PRODUCT_ID_OFFSET];
        if(numOfSubEnclProductId > FBE_ENCLOSURE_MAX_NUM_SUBENCL_PRODUCT_ID_IN_IMAGE)
        {
            fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                               FBE_TRACE_LEVEL_ERROR,
                                               fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                               "%s, Too many subencl prod ids act:%d, limit:%d\n",
                                                __FUNCTION__,
                                               numOfSubEnclProductId,
                                               FBE_ENCLOSURE_MAX_NUM_SUBENCL_PRODUCT_ID_IN_IMAGE); 
            fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
            fbe_transport_complete_packet(packet);
            return FBE_STATUS_GENERIC_FAILURE;
    
        }
        /* Loop through the subencl product id list in the image header to 
         * save all the subEnclosureProductid. 
         */
        for(index = 0; index < numOfSubEnclProductId; index ++)
        {
            byteOffset = FBE_ESES_CDES2_MCODE_SUBENCL_PRODUCT_ID_OFFSET + (index * FBE_ESES_CONFIG_SUBENCL_PRODUCT_ID_SIZE);
    
            fbe_copy_memory(&pParseImageHeader->subenclProductId[index][0], 
                            &pImageHeader[byteOffset], 
                            FBE_ESES_CONFIG_SUBENCL_PRODUCT_ID_SIZE);
        }
    
        fbe_copy_memory((fbe_u8_t *)&fwCompTypeTemp, 
                         pImageHeader + FBE_ESES_CDES2_MCODE_IMAGE_COMPONENT_TYPE_OFFSET,
                         FBE_ESES_CDES2_MCODE_IMAGE_COMPONENT_TYPE_SIZE_BYTES);

        fwCompType = swap32(fwCompTypeTemp);
        fbe_eses_enclosure_eses_comp_type_to_fw_target(fwCompType,
                                        &pParseImageHeader->firmwareTarget);
        
        /* save the header size */
        fbe_copy_memory((fbe_u8_t *)&sizeTemp, 
                         pImageHeader + FBE_ESES_CDES2_MCODE_IMAGE_HEADER_SIZE_OFFSET,
                         FBE_ESES_CDES2_MCODE_IMAGE_HEADER_SIZE_BYTES);
        pParseImageHeader->headerSize = swap32(sizeTemp);

        /* Save image size. */
        fbe_copy_memory((fbe_u8_t *)&mcodeDataSizeTemp, 
                         pImageHeader + FBE_ESES_CDES2_MCODE_DATA_SIZE_OFFSET,
                         FBE_ESES_CDES2_MCODE_DATA_SIZE_BYTES);

        pParseImageHeader->imageSize = pParseImageHeader->headerSize + swap32(mcodeDataSizeTemp);

        /* Save image rev. */
        fbe_copy_memory(&pParseImageHeader->imageRev[0], 
                        &pImageHeader[FBE_ESES_CDES2_MCODE_IMAGE_REV_OFFSET], 
                        FBE_ESES_CDES2_MCODE_IMAGE_REV_SIZE_BYTES);
    }

    /* Set the payload control status. */
    fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    /* Set the payload control status qualifier with the encl_status. */
    fbe_payload_control_set_status_qualifier(control_operation, FBE_ENCLOSURE_STATUS_OK);
    /* Set the packet status. */
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);

    /* Complete the packet. */
    fbe_transport_complete_packet(packet);

    return FBE_STATUS_OK;
    
} // End of function - fbe_eses_enclosure_parse_image_header



/*!*************************************************************************
* @fn fbe_eses_enclosure_get_lcc_info()                  
***************************************************************************
* @brief
*       This function gets the lcc info.
*
* @param     eses_enclosure - The pointer to the fbe_eses_enclosure_t.
* @param     packet - The pointer to the fbe_packet_t.
*
* @return    fbe_status_t.
*
* @version
*   10-Aug-2010     PHE - Created.
***************************************************************************/
static fbe_status_t 
fbe_eses_enclosure_get_lcc_info(fbe_eses_enclosure_t *eses_enclosure, 
                            fbe_packet_t *packet)
{
    fbe_payload_ex_t                       * payload = NULL;
    fbe_payload_control_operation_t     * control_operation = NULL;   
    fbe_payload_control_buffer_t        control_buffer = NULL;    
    fbe_payload_control_buffer_length_t control_buffer_length = 0;
    fbe_enclosure_mgmt_get_lcc_info_t * pGetLccInfo = NULL;
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_enclosure_status_t              enclStatus = FBE_ENCLOSURE_STATUS_OK;
    fbe_edal_status_t                   edalStatus;
    fbe_bool_t                          inserted; 
    fbe_bool_t                          faulted;
    fbe_u8_t                            additionalStatus;
    fbe_bool_t                          isLocal;
    fbe_bool_t                          ecbFault = FBE_FALSE;
    fbe_u8_t                            subenclProductId[FBE_ESES_CONFIG_SUBENCL_PRODUCT_ID_SIZE] = {0};
    fbe_edal_fw_info_t                  edal_fw_info;
    fbe_u32_t                           component_index = 0;
    fbe_enclosure_component_id_t        component_id = 0;
    fbe_bool_t                          expansion_port_inserted;
    
    /********
     * BEGIN
     ********/

    /* The original packet status is FBE_STATUS_INVALID (initialized in fbe_transport_initialize_packet)
     * we set it to FBE_STATUS_OK to start with. 
     * It will be updated as needed later on. 
     */
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);

    payload = fbe_transport_get_payload_ex(packet);
    if(payload == NULL)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                            FBE_TRACE_LEVEL_ERROR,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *)eses_enclosure),
                            "%s, get_payload failed.\n", __FUNCTION__); 
        return FBE_STATUS_GENERIC_FAILURE;
    }

    control_operation = fbe_payload_ex_get_control_operation(payload);
    if(control_operation == NULL)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                            FBE_TRACE_LEVEL_ERROR,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *)eses_enclosure),
                            "%s, get_control_operation failed.\n", __FUNCTION__); 

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status =  fbe_payload_control_get_buffer(control_operation, &control_buffer);
    if( (control_buffer == NULL) || (status!=FBE_STATUS_OK))
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                               FBE_TRACE_LEVEL_WARNING,
                               fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *)eses_enclosure),
                               "%s, get_buffer failed, status: 0x%x, control_buffer: %64p.\n",
                               __FUNCTION__, status, control_buffer);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Set the control operation status and status qualifier to OK to start with. 
     * They will be updated as needed later on.
     */
    fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_payload_control_set_status_qualifier(control_operation, FBE_ENCLOSURE_STATUS_OK);

    pGetLccInfo = (fbe_enclosure_mgmt_get_lcc_info_t *)control_buffer;

   
    /* Verify the control buffer size with the size of the expected structure */
    status = fbe_payload_control_get_buffer_length(control_operation, &control_buffer_length);
    if((status != FBE_STATUS_OK) || 
       (control_buffer_length != sizeof(fbe_enclosure_mgmt_get_lcc_info_t)))
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                               FBE_TRACE_LEVEL_WARNING,
                               fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *)eses_enclosure),
                                "%s, get_buffer_length, bufferLen %d, expectedLen: %llu, status: 0x%x.\n", 
                                __FUNCTION__, control_buffer_length, 
                                (unsigned long long)sizeof(fbe_enclosure_mgmt_get_lcc_info_t), status);
        
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    } 

    component_index =  fbe_base_enclosure_find_first_edal_match_U8((fbe_base_enclosure_t *)eses_enclosure,
                                                FBE_ENCL_COMP_LOCATION,  //attribute
                                                FBE_ENCL_LCC, // Component type
                                                0, //starting index
                                                pGetLccInfo->location.slot);

    if(component_index == FBE_ENCLOSURE_VALUE_INVALID) 
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                               FBE_TRACE_LEVEL_WARNING,
                               fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *)eses_enclosure),
                                "%s, failed to find LCC %d.\n", 
                                __FUNCTION__, pGetLccInfo->location.slot);
        
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    /* Inserted */
    enclStatus = fbe_base_enclosure_edal_getBool_thread_safe((fbe_base_enclosure_t *)eses_enclosure,
                                                            FBE_ENCL_COMP_INSERTED, // Attribute.
                                                            FBE_ENCL_LCC,  // Component type.
                                                            component_index, // Component index.
                                                            &inserted); // The value to be returned.

    if (enclStatus != FBE_ENCLOSURE_STATUS_OK)
    {
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    else
    {
        pGetLccInfo->lccInfo.inserted = inserted;
    }

    /* Faulted */       
    enclStatus = fbe_base_enclosure_edal_getBool_thread_safe((fbe_base_enclosure_t *)eses_enclosure,
                                                            FBE_ENCL_COMP_FAULTED, // Attribute.
                                                            FBE_ENCL_LCC,  // Component type.
                                                            component_index, // Component index.
                                                            &faulted); // The value to be returned.
    if (enclStatus != FBE_ENCLOSURE_STATUS_OK)
    {
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    else
    {
        pGetLccInfo->lccInfo.faulted = faulted;
    }

    /* Additional Status */       
    enclStatus = fbe_base_enclosure_edal_getU8_thread_safe((fbe_base_enclosure_t *)eses_enclosure,
                                                            FBE_ENCL_COMP_ADDL_STATUS, // Attribute.
                                                            FBE_ENCL_LCC,  // Component type.
                                                            component_index, // Component index.
                                                            &additionalStatus); // The value to be returned.
    if (enclStatus != FBE_ENCLOSURE_STATUS_OK)
    {
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    else
    {
        pGetLccInfo->lccInfo.additionalStatus = additionalStatus;
    }

    /* IsLocal */
    enclStatus = fbe_base_enclosure_edal_getBool_thread_safe((fbe_base_enclosure_t *)eses_enclosure,
                                                            FBE_ENCL_COMP_IS_LOCAL, // Attribute.
                                                            FBE_ENCL_LCC,  // Component type.
                                                            component_index, // Component index.
                                                            &isLocal); // The value to be returned.
    if (enclStatus != FBE_ENCLOSURE_STATUS_OK)
    {
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    else
    {
        pGetLccInfo->lccInfo.isLocal = isLocal;
    }
            
    /* ECB fault */
    enclStatus = fbe_base_enclosure_edal_getBool_thread_safe((fbe_base_enclosure_t *)eses_enclosure,
                                    FBE_LCC_ECB_FAULT,
                                    FBE_ENCL_LCC,  // Component type.
                                    component_index, // Component index.
                                    &ecbFault);

    if (enclStatus != FBE_ENCLOSURE_STATUS_OK)
    {
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    } 
    else
    {
        pGetLccInfo->lccInfo.ecbFault = ecbFault;
    }

    // Get eses version.
    pGetLccInfo->lccInfo.eses_version_desc = eses_enclosure->eses_version_desc;

    // Get FW Rev
    enclStatus = fbe_base_enclosure_edal_getStr_thread_safe((fbe_base_enclosure_t *)eses_enclosure,
                                    FBE_ENCL_COMP_FW_INFO,// Component Attribute
                                    FBE_ENCL_LCC,  // Component type.
                                    component_index, // Component index.
                                    sizeof(fbe_edal_fw_info_t), 
                                    (char *)&edal_fw_info);     // copy the string here

    if (enclStatus != FBE_ENCLOSURE_STATUS_OK)
    {
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    else
    {
        fbe_zero_memory(&pGetLccInfo->lccInfo.firmwareRev[0], 
                        FBE_ENCLOSURE_MAX_FIRMWARE_REV_SIZE + 1);

        fbe_copy_memory(&pGetLccInfo->lccInfo.firmwareRev[0],
                         edal_fw_info.fwRevision,
                         FBE_ESES_FW_REVISION_SIZE);
    }

    // Get init string fw rev.
    enclStatus = fbe_base_enclosure_edal_getStr_thread_safe((fbe_base_enclosure_t *)eses_enclosure,
                                    FBE_LCC_INIT_FW_INFO,// Component Attribute
                                    FBE_ENCL_LCC,  // Component type.
                                    component_index, // Component index.
                                    sizeof(fbe_edal_fw_info_t), 
                                    (char *)&edal_fw_info);     // copy the string here

    if (enclStatus != FBE_ENCLOSURE_STATUS_OK)
    {
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    else
    {
        fbe_zero_memory(&pGetLccInfo->lccInfo.initStrFirmwareRev[0], 
                        FBE_ENCLOSURE_MAX_FIRMWARE_REV_SIZE + 1);

        fbe_copy_memory(&pGetLccInfo->lccInfo.initStrFirmwareRev[0],
                         edal_fw_info.fwRevision,
                         FBE_ESES_FW_REVISION_SIZE);
    }

    // Get FPGA fw rev.
    enclStatus = fbe_base_enclosure_edal_getStr_thread_safe((fbe_base_enclosure_t *)eses_enclosure,
                                    FBE_LCC_FPGA_FW_INFO,// Component Attribute
                                    FBE_ENCL_LCC,  // Component type.
                                    component_index, // Component index.
                                    sizeof(fbe_edal_fw_info_t), 
                                    (char *)&edal_fw_info);     // copy the string here

    if (enclStatus != FBE_ENCLOSURE_STATUS_OK)
    {
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    else
    {
        fbe_zero_memory(&pGetLccInfo->lccInfo.fpgaFirmwareRev[0], 
                        FBE_ENCLOSURE_MAX_FIRMWARE_REV_SIZE + 1);

        fbe_copy_memory(&pGetLccInfo->lccInfo.fpgaFirmwareRev[0],
                         edal_fw_info.fwRevision,
                         FBE_ESES_FW_REVISION_SIZE);
    }
    
    /* Subencl product id. */
    enclStatus = fbe_base_enclosure_edal_getStr_thread_safe((fbe_base_enclosure_t *)eses_enclosure,
                                    FBE_ENCL_SUBENCL_PRODUCT_ID,
                                    FBE_ENCL_LCC,  // Component type.
                                    component_index, // Component index.
                                    FBE_ESES_CONFIG_SUBENCL_PRODUCT_ID_SIZE,
                                    &subenclProductId[0]);

    if (enclStatus != FBE_ENCLOSURE_STATUS_OK)
    {
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    else
    {
        fbe_zero_memory(&pGetLccInfo->lccInfo.subenclProductId[0], 
                        FBE_ENCLOSURE_MAX_SUBENCL_PRODUCT_ID_SIZE + 1);

        fbe_copy_memory(&pGetLccInfo->lccInfo.subenclProductId[0],
                         &subenclProductId[0],
                         FBE_ESES_CONFIG_SUBENCL_PRODUCT_ID_SIZE);
    }

    // fill in the ffid(i.e. uniqueId) based on product id string
    fbe_eses_enclosure_lcc_lookup_ffid(eses_enclosure,
                                       &pGetLccInfo->lccInfo.subenclProductId[0], 
                                       &pGetLccInfo->lccInfo.uniqueId);

    /* Expansion port openness */
    if ((pGetLccInfo->lccInfo.uniqueId == VOYAGER_LCC ) ||
        (pGetLccInfo->lccInfo.uniqueId == VIKING_LCC )  ||
        (pGetLccInfo->lccInfo.uniqueId == CAYENNE_LCC ) ||
        (pGetLccInfo->lccInfo.uniqueId == NAGA_LCC ))
    {
        pGetLccInfo->lccInfo.expansionPortOpen = FBE_ENCLOSURE_PORT_OPEN_STATUS_NA;
    }
    else if (!(pGetLccInfo->lccInfo.isLocal))
    {
        pGetLccInfo->lccInfo.expansionPortOpen = FBE_ENCLOSURE_PORT_OPEN_STATUS_UNKNOWN;
    }
    else
    {
        edalStatus = fbe_edal_getConnectorBool(((fbe_base_enclosure_t *)eses_enclosure)->enclosureComponentTypeBlock, 
                                               FBE_ENCL_COMP_INSERTED, 
                                               FBE_EDAL_EXPANSION_CONNECTOR,
                                               &expansion_port_inserted);
        if(edalStatus != FBE_EDAL_STATUS_OK && edalStatus != FBE_EDAL_STATUS_ATTRIBUTE_NOT_FOUND)
        {
            fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
            fbe_transport_complete_packet(packet);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        else
        {
            pGetLccInfo->lccInfo.expansionPortOpen = expansion_port_inserted?FBE_ENCLOSURE_PORT_OPEN_STATUS_FALSE:FBE_ENCLOSURE_PORT_OPEN_STATUS_TRUE;
        }
    }

    // for Voyager (ICM and EE) only slots 0 & 1 support upgrade from board object 
    component_id = fbe_base_enclosure_get_component_id((fbe_base_enclosure_t *)eses_enclosure);
    if(component_id == 0) 
    {
        if (pGetLccInfo->location.slot > 1)
        {
            pGetLccInfo->lccInfo.bDownloadable = FALSE;
        }
        else
        {
            pGetLccInfo->lccInfo.bDownloadable = TRUE;
        }
    }
    else
    {
        pGetLccInfo->lccInfo.bDownloadable = TRUE;
    }

    /* Jetfire LCC does not have resume */
    if (pGetLccInfo->lccInfo.uniqueId == JETFIRE_SP_SANDYBRIDGE )
    {
        pGetLccInfo->lccInfo.hasResumeProm = FALSE;
    }
    else
    {
        pGetLccInfo->lccInfo.hasResumeProm = TRUE;
    }

    /* some lcc is not a replacable hardware component */
    if ((pGetLccInfo->lccInfo.uniqueId == JETFIRE_SP_SANDYBRIDGE) ||
        (pGetLccInfo->lccInfo.uniqueId == BEACHCOMBER_PCB) ||
        (pGetLccInfo->lccInfo.uniqueId == BEACHCOMBER_PCB_SIM) ||
        (pGetLccInfo->lccInfo.uniqueId == MERIDIAN_SP_ESX) ||
        (pGetLccInfo->lccInfo.uniqueId == TUNGSTEN_SP))
    {
        pGetLccInfo->lccInfo.isCru = FALSE;
    }
    else
    {
        pGetLccInfo->lccInfo.isCru = TRUE;
    }

    pGetLccInfo->lccInfo.lccActualHwType = fbe_base_enclosure_get_lcc_device_type((fbe_base_enclosure_t *)eses_enclosure);

    /* Set the control operation status qualifier with the encl_status. */
    fbe_payload_control_set_status_qualifier(control_operation, FBE_ENCLOSURE_STATUS_OK);
    /* Complete the packet. */
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);

    return FBE_STATUS_OK;
    
} // End of function - fbe_eses_enclosure_get_lcc_info

/*!*************************************************************************
* @fn fbe_eses_enclosure_get_lcc_start_slot()                  
***************************************************************************
* @brief
*   Get the lcc slot number that contains the edge expander lcc.
*
* @param     eses_enclosure - The pointer to the fbe_eses_enclosure_t.
* @param     packet - The pointer to the fbe_packet_t.
*
* @return    fbe_status_t.
*
* @version
*   11-Nov-2011     PHE - Created.
***************************************************************************/
static fbe_status_t fbe_eses_enclosure_get_lcc_start_slot(fbe_eses_enclosure_t *eses_enclosure, 
                                                  fbe_packet_t *packet)
{
    fbe_u32_t       index;
    fbe_payload_ex_t                       *payload = NULL;
    fbe_payload_control_operation_t     *control_operation = NULL;   
    fbe_payload_control_buffer_t        control_buffer = NULL;    
    fbe_payload_control_buffer_length_t control_buffer_length = 0;
    fbe_u8_t                            *pSlot = NULL;
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_enclosure_status_t              enclStatus = FBE_ENCLOSURE_STATUS_OK;

    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);

    payload = fbe_transport_get_payload_ex(packet);
    if(payload == NULL)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                           FBE_TRACE_LEVEL_ERROR,
                                           fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *)eses_enclosure),
                                           "%s, get_payload failed\n", 
                                           __FUNCTION__); 
        return FBE_STATUS_GENERIC_FAILURE;
    }

    control_operation = fbe_payload_ex_get_control_operation(payload);
    if(control_operation == NULL)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                           FBE_TRACE_LEVEL_ERROR,
                                           fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *)eses_enclosure),
                                           "%s, get_control_operation failed\n",
                                           __FUNCTION__); 
        
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status =  fbe_payload_control_get_buffer(control_operation, &control_buffer);
    if( (control_buffer == NULL) || (status!=FBE_STATUS_OK))
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                           FBE_TRACE_LEVEL_WARNING,
                                           fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *)eses_enclosure),
                                           "%s, get_buffer failed, status:0x%x, control_buffer:%64p\n",
                                           __FUNCTION__, 
                                           status, 
                                           control_buffer);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    // init the control operation status and status qualifier to OK
    fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_payload_control_set_status_qualifier(control_operation, FBE_ENCLOSURE_STATUS_OK);

    pSlot = (fbe_u8_t *)control_buffer;

    // verify the control buffer size with the size of the expected structure
    status = fbe_payload_control_get_buffer_length(control_operation, &control_buffer_length);
    if((status != FBE_STATUS_OK) || 
       (control_buffer_length != sizeof(fbe_u32_t)))
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                           FBE_TRACE_LEVEL_WARNING,
                                           fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *)eses_enclosure),
                                           "%s, buffer length act:%d exp:%llu, status:0x%x\n",
                                           __FUNCTION__,
                                           control_buffer_length, 
                                           (unsigned long long)sizeof(fbe_u32_t),
                                           status);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    } 

    // get the index to reference 'SPA'
    index = fbe_base_enclosure_find_first_edal_match_U8((fbe_base_enclosure_t *)eses_enclosure,
                                                        FBE_ENCL_COMP_SIDE_ID,  // attribute
                                                        FBE_ENCL_LCC,           // Component type
                                                        0,                      // starting index
                                                        0);                     // 0 is side A
    if (index == FBE_ENCLOSURE_VALUE_INVALID)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                           FBE_TRACE_LEVEL_WARNING,
                                           fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *)eses_enclosure),
                                           "%s, edal returned FBE_ENCLOSURE_VALUE_INVALID\n",
                                           __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    // get the first slot for this object
    enclStatus =  fbe_base_enclosure_edal_getU8_thread_safe((fbe_base_enclosure_t *)eses_enclosure,
                                                            FBE_ENCL_COMP_LOCATION, // attribute
                                                            FBE_ENCL_LCC,           // Component type
                                                            index,                  // index
                                                            pSlot);                 // returned slot#

    if (enclStatus != FBE_ENCLOSURE_STATUS_OK)
    {
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    // set the control operation status qualifier with the encl_status
    fbe_payload_control_set_status_qualifier(control_operation, FBE_ENCLOSURE_STATUS_OK);
    // complete the packet
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    
    return FBE_STATUS_OK;

} // end fbe_eses_enclosure_get_lcc_start_slot

/*!*************************************************************************
* @fn fbe_eses_enclosure_get_drive_midplane_count()                  
***************************************************************************
* @brief
*   Get the drive midplane count.
*
* @param     eses_enclosure - The pointer to the fbe_eses_enclosure_t.
* @param     packet - The pointer to the fbe_packet_t.
*
* @return    fbe_status_t.
*
* @version
*   22-Oct-2012     Dipak Patel - Created.
***************************************************************************/
static fbe_status_t fbe_eses_enclosure_get_drive_midplane_count(fbe_eses_enclosure_t *eses_enclosure, 
                                                  fbe_packet_t *packet)
{
    fbe_payload_ex_t                       *payload = NULL;
    fbe_payload_control_operation_t     *control_operation = NULL;   
    fbe_payload_control_buffer_t        control_buffer = NULL;    
    fbe_payload_control_buffer_length_t control_buffer_length = 0;
    fbe_u8_t                            *pDriveMidplaneCount = NULL;
    fbe_status_t                        status = FBE_STATUS_OK;

    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);

    payload = fbe_transport_get_payload_ex(packet);
    if(payload == NULL)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                           FBE_TRACE_LEVEL_ERROR,
                                           fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *)eses_enclosure),
                                           "%s, get_payload failed\n", 
                                           __FUNCTION__); 
        return FBE_STATUS_GENERIC_FAILURE;
    }

    control_operation = fbe_payload_ex_get_control_operation(payload);
    if(control_operation == NULL)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                           FBE_TRACE_LEVEL_ERROR,
                                           fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *)eses_enclosure),
                                           "%s, get_control_operation failed\n",
                                           __FUNCTION__); 
        
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status =  fbe_payload_control_get_buffer(control_operation, &control_buffer);
    if( (control_buffer == NULL) || (status!=FBE_STATUS_OK))
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                           FBE_TRACE_LEVEL_WARNING,
                                           fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *)eses_enclosure),
                                           "%s, get_buffer failed, status:0x%x, control_buffer:%64p\n",
                                           __FUNCTION__, 
                                           status, 
                                           control_buffer);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    // init the control operation status and status qualifier to OK
    fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_payload_control_set_status_qualifier(control_operation, FBE_ENCLOSURE_STATUS_OK);

    pDriveMidplaneCount = (fbe_u8_t *)control_buffer;

    // verify the control buffer size with the size of the expected structure
    status = fbe_payload_control_get_buffer_length(control_operation, &control_buffer_length);
    if((status != FBE_STATUS_OK) || 
       (control_buffer_length != sizeof(fbe_u8_t)))
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                           FBE_TRACE_LEVEL_WARNING,
                                           fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *)eses_enclosure),
                                           "%s, buffer length act:%d exp:%d, status:0x%x\n",
                                           __FUNCTION__,
                                           control_buffer_length, 
                                           (int)sizeof(fbe_u32_t),
                                           status);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    } 

    *pDriveMidplaneCount = fbe_eses_enclosure_get_number_of_drive_midplane(eses_enclosure);

    // set the control operation status qualifier with the encl_status
    fbe_payload_control_set_status_qualifier(control_operation, FBE_ENCLOSURE_STATUS_OK);
    // complete the packet
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    
    return FBE_STATUS_OK;

} // end fbe_eses_enclosure_get_drive_midplane_count

/*!*************************************************************************
* @fn fbe_eses_enclosure_get_stand_alone_lcc_count()                  
***************************************************************************
* @brief
*       This function gets the count for stand alone LCC in the enclosure.
*
* @param     base_enclosure - The pointer to the fbe_base_enclosure_t.
* @param     packet - The pointer to the fbe_packet_t.
*
* @return    fbe_status_t.
*
* @version
*   30-Oct-2012     Rui Chang - Created.
***************************************************************************/
static fbe_status_t 
fbe_eses_enclosure_get_stand_alone_lcc_count(fbe_eses_enclosure_t *eses_enclosure, 
                            fbe_packet_t *packet)
{
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;   
    fbe_payload_control_buffer_t control_buffer = NULL;    
    fbe_payload_control_buffer_length_t control_buffer_length = 0;
    fbe_u32_t * pComponentCount = NULL;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_edal_status_t edalStatus = FBE_EDAL_STATUS_OK;
    
    /********
     * BEGIN
     ********/

    /* The original packet status is FBE_STATUS_INVALID (initialized in fbe_transport_initialize_packet)
     * we set it to FBE_STATUS_OK to start with. 
     * It will be updated as needed later on. 
     */
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);

    payload = fbe_transport_get_payload_ex(packet);
    if(payload == NULL)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                            FBE_TRACE_LEVEL_ERROR,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                            "%s, get_payload failed.\n", __FUNCTION__); 
      return FBE_STATUS_GENERIC_FAILURE;
    }

    control_operation = fbe_payload_ex_get_control_operation(payload);
    if(control_operation == NULL)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                            FBE_TRACE_LEVEL_ERROR,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                            "%s, get_control_operation failed.\n", __FUNCTION__); 

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status =  fbe_payload_control_get_buffer(control_operation, &control_buffer);
    if( (control_buffer == NULL) || (status!=FBE_STATUS_OK))
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                               FBE_TRACE_LEVEL_WARNING,
                               fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                               "%s, get_buffer failed, status: 0x%x, control_buffer: %64p.\n",
                               __FUNCTION__, status, control_buffer);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Set the control operation status and status qualifier to OK to start with. 
     * They will be updated as needed later on.
     */
    fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_payload_control_set_status_qualifier(control_operation, FBE_ENCLOSURE_STATUS_OK);

    pComponentCount = (fbe_u32_t *)control_buffer;

    /* Verify the control buffer size with the size of the expected structure */
    status = fbe_payload_control_get_buffer_length(control_operation, &control_buffer_length);
    if((status != FBE_STATUS_OK) || 
       (control_buffer_length != sizeof(fbe_u32_t)))
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                               FBE_TRACE_LEVEL_WARNING,
                               fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure),
                                "%s, get_buffer_length, bufferLen %d, expectedLen: %d, status: 0x%x.\n", 
                                __FUNCTION__, control_buffer_length, 
                                (int)sizeof(fbe_u32_t), status);
        
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    } 

    switch (eses_enclosure->sas_enclosure_type)
    {
        case FBE_SAS_ENCLOSURE_TYPE_CITADEL:
        case FBE_SAS_ENCLOSURE_TYPE_BUNKER:
        case FBE_SAS_ENCLOSURE_TYPE_FALLBACK:
        case FBE_SAS_ENCLOSURE_TYPE_BOXWOOD:
        case FBE_SAS_ENCLOSURE_TYPE_KNOT:
        case FBE_SAS_ENCLOSURE_TYPE_STEELJAW:
        case FBE_SAS_ENCLOSURE_TYPE_RAMHORN:
        case FBE_SAS_ENCLOSURE_TYPE_CALYPSO:
        case FBE_SAS_ENCLOSURE_TYPE_RHEA:
        case FBE_SAS_ENCLOSURE_TYPE_MIRANDA:
            pComponentCount = 0;
            break;

        default:
            edalStatus = 
                fbe_edal_getSpecificComponentCount(((fbe_base_enclosure_t *)eses_enclosure)->enclosureComponentTypeBlock,
                                                   FBE_ENCL_LCC,
                                                   pComponentCount);
        
            if(!EDAL_STAT_OK(edalStatus))
            {
                fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
                fbe_transport_complete_packet(packet);
                return FBE_STATUS_GENERIC_FAILURE;
            }
            break;
    }

    /* Set the control operation status qualifier with the encl_status. */
    fbe_payload_control_set_status_qualifier(control_operation, FBE_ENCLOSURE_STATUS_OK);
    /* Complete the packet. */
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);

    return FBE_STATUS_OK;
    
} // End of function - fbe_eses_enclosure_get_stand_alone_lcc_count

/*!*************************************************************************
* @fn fbe_eses_enclosure_get_fan_info()                  
***************************************************************************
* @brief
*       This function gets the fan info.
*
* @param     eses_enclosure - The pointer to the fbe_eses_enclosure_t.
* @param     packet - The pointer to the fbe_packet_t.
*
* @return    fbe_status_t.
*
* @version
*   16-Dec-2010     Rajesh V. - Created.
***************************************************************************/
static fbe_status_t 
fbe_eses_enclosure_get_fan_info(fbe_eses_enclosure_t *eses_enclosure, 
                                fbe_packet_t *packet)
{
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;   
    fbe_payload_control_buffer_length_t control_buffer_length = 0;
    fbe_cooling_info_t * pGetFanInfo = NULL;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_enclosure_status_t enclStatus = FBE_ENCLOSURE_STATUS_OK;
    fbe_u8_t coolingComponentCount = 0;
    fbe_u32_t coolingIndex = 0;
    fbe_u8_t containerIndex = 0; 
    fbe_u8_t sideId = 0;
    fbe_bool_t inserted = FALSE; 
    fbe_bool_t faulted = FALSE; 
    fbe_u8_t subtype = FBE_ENCL_COOLING_SUBTYPE_INVALID;
    fbe_u8_t targetSubtype = FBE_ENCL_COOLING_SUBTYPE_INVALID;
    fbe_edal_fw_info_t edalFwInfo = {0};
    fbe_u8_t subenclProductId[FBE_ESES_CONFIG_SUBENCL_PRODUCT_ID_SIZE] = {0};

    /********
     * BEGIN
     ********/

    /* The original packet status is FBE_STATUS_INVALID (initialized in fbe_transport_initialize_packet)
     * we set it to FBE_STATUS_OK to start with. 
     * It will be updated as needed later on. 
     */
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);

    payload = fbe_transport_get_payload_ex(packet);
    if(payload == NULL)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                            FBE_TRACE_LEVEL_ERROR,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *)eses_enclosure),
                            "%s, get_payload failed.\n", __FUNCTION__); 

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    control_operation = fbe_payload_ex_get_control_operation(payload);
    if(control_operation == NULL)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                            FBE_TRACE_LEVEL_ERROR,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *)eses_enclosure),
                            "%s, get_control_operation failed.\n", __FUNCTION__); 

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status =  fbe_payload_control_get_buffer(control_operation, &pGetFanInfo);
    if( (pGetFanInfo == NULL) || (status!=FBE_STATUS_OK))
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                               FBE_TRACE_LEVEL_WARNING,
                               fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *)eses_enclosure),
                               "%s, get_buffer failed, status 0x%x, pGetFanInfo %p.\n",
                               __FUNCTION__, status, pGetFanInfo);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Verify the control buffer size with the size of the expected structure */
    status = fbe_payload_control_get_buffer_length(control_operation, &control_buffer_length);
    if((status != FBE_STATUS_OK) || 
       (control_buffer_length != sizeof(fbe_cooling_info_t)))
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                               FBE_TRACE_LEVEL_WARNING,
                               fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *)eses_enclosure),
                               "%s, get_buffer_length, bufferLen %d, expectedLen %llu, status 0x%x.\n", 
                               __FUNCTION__, 
                               control_buffer_length, 
                               (unsigned long long)sizeof(fbe_cooling_info_t),
                               status);
        
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    } 

    /* We only return the standalone fan.
     */ 
    targetSubtype = FBE_ENCL_COOLING_SUBTYPE_STANDALONE;

    // Get the total number of Cooling Components including the overall elements.
    status = fbe_base_enclosure_get_number_of_components((fbe_base_enclosure_t *)eses_enclosure,
                                                             FBE_ENCL_COOLING_COMPONENT,
                                                             &coolingComponentCount);
    if(status != FBE_STATUS_OK)
    {
        fbe_transport_set_status(packet, status, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    for (coolingIndex = 0; coolingIndex < coolingComponentCount; coolingIndex++)
    {
        /* Get subtype. */
        enclStatus = fbe_base_enclosure_edal_getU8_thread_safe((fbe_base_enclosure_t *)eses_enclosure,
                                                               FBE_ENCL_COMP_SUBTYPE,
                                                               FBE_ENCL_COOLING_COMPONENT,
                                                               coolingIndex,
                                                               &subtype);
        if (enclStatus != FBE_ENCLOSURE_STATUS_OK)
        {
            fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
            fbe_transport_complete_packet(packet);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        /* Get containerIndex */
        enclStatus = fbe_base_enclosure_edal_getU8_thread_safe((fbe_base_enclosure_t *)eses_enclosure,
                                                               FBE_ENCL_COMP_CONTAINER_INDEX,
                                                               FBE_ENCL_COOLING_COMPONENT,
                                                               coolingIndex,
                                                               &containerIndex);
        if (enclStatus != FBE_ENCLOSURE_STATUS_OK)
        {
            fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
            fbe_transport_complete_packet(packet);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        /* Get sideId. */
        enclStatus = fbe_base_enclosure_edal_getU8_thread_safe((fbe_base_enclosure_t *)eses_enclosure,
                                                               FBE_ENCL_COMP_SIDE_ID, // Attribute.
                                                               FBE_ENCL_COOLING_COMPONENT, // Component type.
                                                               coolingIndex, // Component index.
                                                               &sideId); // The value to be returned.
        if (enclStatus != FBE_ENCLOSURE_STATUS_OK)
        {
            fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
            fbe_transport_complete_packet(packet);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        
        if((subtype == targetSubtype) &&
           (sideId == pGetFanInfo->slotNumOnEncl) &&
           (containerIndex == FBE_ENCLOSURE_CONTAINER_INDEX_SELF_CONTAINED))
        {
            /* Only check the overall element, not the individual element. */
            // Inserted.
            enclStatus = fbe_base_enclosure_edal_getBool_thread_safe((fbe_base_enclosure_t *)eses_enclosure,
                                                            FBE_ENCL_COMP_INSERTED,  // Attribute.
                                                            FBE_ENCL_COOLING_COMPONENT, // Component type.
                                                            coolingIndex, // Component index.
                                                            &inserted);  // The value to be returned.

            if (enclStatus != FBE_ENCLOSURE_STATUS_OK)
            {
                fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
                fbe_transport_complete_packet(packet);
                return FBE_STATUS_GENERIC_FAILURE;
            }
            pGetFanInfo->inserted = inserted ? FBE_MGMT_STATUS_TRUE : FBE_MGMT_STATUS_FALSE;

            // Faulted.
            enclStatus = fbe_base_enclosure_edal_getBool_thread_safe((fbe_base_enclosure_t *)eses_enclosure,
                                                            FBE_ENCL_COMP_FAULTED, // Attribute.
                                                            FBE_ENCL_COOLING_COMPONENT,  // Component type.
                                                            coolingIndex, // Component index.
                                                            &faulted); // The value to be returned.

            if (enclStatus != FBE_ENCLOSURE_STATUS_OK)
            {
                fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
                fbe_transport_complete_packet(packet);
                return FBE_STATUS_GENERIC_FAILURE;
            }
            pGetFanInfo->fanFaulted = faulted ? FBE_MGMT_STATUS_TRUE : FBE_MGMT_STATUS_FALSE;

            // Firmware Rev.
            enclStatus = fbe_base_enclosure_edal_getStr_thread_safe((fbe_base_enclosure_t *)eses_enclosure,
                                FBE_ENCL_COMP_FW_INFO,// Component Attribute
                                FBE_ENCL_COOLING_COMPONENT,  // Component type.
                                coolingIndex, // Component index.
                                sizeof(fbe_edal_fw_info_t), 
                                (char *)&edalFwInfo);     // copy the string here

            if (enclStatus != FBE_ENCLOSURE_STATUS_OK)
            {
                fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
                fbe_transport_complete_packet(packet);
                return FBE_STATUS_GENERIC_FAILURE;
            }
            else
            {
                fbe_zero_memory(&pGetFanInfo->firmwareRev[0],
                                FBE_ENCLOSURE_MAX_FIRMWARE_REV_SIZE + 1);
        
                fbe_copy_memory(&pGetFanInfo->firmwareRev[0], 
                                &edalFwInfo.fwRevision[0], 
                                FBE_ESES_FW_REVISION_SIZE);
            }

            // Subencl product id.
            enclStatus = fbe_base_enclosure_edal_getStr_thread_safe((fbe_base_enclosure_t *)eses_enclosure,
                                            FBE_ENCL_SUBENCL_PRODUCT_ID,
                                            FBE_ENCL_COOLING_COMPONENT,  // Component type.
                                            coolingIndex, // Component index.
                                            FBE_ESES_CONFIG_SUBENCL_PRODUCT_ID_SIZE,
                                            &subenclProductId[0]);
        
            if (enclStatus != FBE_ENCLOSURE_STATUS_OK)
            {
                fbe_transport_set_status(packet, FBE_ENCLOSURE_STATUS_CMD_FAILED, 0);
                fbe_transport_complete_packet(packet);
                return FBE_STATUS_GENERIC_FAILURE;
            }
            else
            {
                fbe_zero_memory(&pGetFanInfo->subenclProductId[0],
                                FBE_ENCLOSURE_MAX_SUBENCL_PRODUCT_ID_SIZE + 1);
        
                fbe_copy_memory(&pGetFanInfo->subenclProductId[0],
                                 &subenclProductId[0],
                                 FBE_ESES_CONFIG_SUBENCL_PRODUCT_ID_SIZE);
            }

            break;
        } // End of the overall element found.
    }// End of - for (coolingIndex = 0; coolingIndex < coolingComponentCount; coolingIndex++)

    fbe_eses_enclosure_convert_subencl_product_id_to_unique_id(eses_enclosure,
                                                               &pGetFanInfo->subenclProductId[0], 
                                                               &pGetFanInfo->uniqueId);
    /* The enclosure object does not report the processor enclosure FANs.
     So set inProcessEncl to FALSE */
    pGetFanInfo->inProcessorEncl = FALSE;
    pGetFanInfo->bDownloadable = TRUE;
    pGetFanInfo->fanDegraded = FBE_MGMT_STATUS_FALSE;
    pGetFanInfo->multiFanModuleFault = FBE_MGMT_STATUS_NA;
    pGetFanInfo->persistentMultiFanModuleFault = FBE_MGMT_STATUS_NA;
    pGetFanInfo->envInterfaceStatus = FBE_ENV_INTERFACE_STATUS_GOOD;
    /* Currently only voyager has standalone fans, and they have no associatedSp. */
    pGetFanInfo->associatedSp= SP_INVALID;
    pGetFanInfo->slotNumOnEncl = sideId;
    pGetFanInfo->slotNumOnSpBlade = FBE_INVALID_SLOT_NUM;
    pGetFanInfo->hasResumeProm = TRUE;

    /* Set the payload control status. */
    fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    /* Set the payload control status qualifier with the encl_status. */
    fbe_payload_control_set_status_qualifier(control_operation, FBE_ENCLOSURE_STATUS_OK);
    /* Set packet status. */
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);

    /* Complete the packet. */
    fbe_transport_complete_packet(packet);

    return FBE_STATUS_OK;
    
} // End of function - fbe_eses_enclosure_get_fan_info

/*!*************************************************************************
* @fn fbe_eses_enclosure_get_chassis_cooling_status()                  
***************************************************************************
* @brief
*       This function gets cooling status of the chassis.
*
* @param     eses_enclosure - The pointer to the fbe_eses_enclosure_t.
* @param     packet - The pointer to the fbe_packet_t.
*
* @return    fbe_status_t.
*
* @version
*   25-Aug-2012     Rui Chang. - Created.
***************************************************************************/
static fbe_status_t 
fbe_eses_enclosure_get_chassis_cooling_status(fbe_eses_enclosure_t *eses_enclosure, 
                                fbe_packet_t *packet)
{
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;   
    fbe_payload_control_buffer_length_t control_buffer_length = 0;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_enclosure_status_t enclStatus = FBE_ENCLOSURE_STATUS_OK;
    fbe_u8_t coolingComponentCount = 0;
    fbe_u32_t coolingIndex = 0;
    fbe_u8_t containerIndex = 0; 
    fbe_u8_t subtype = FBE_ENCL_COOLING_SUBTYPE_INVALID;
    fbe_u8_t  additionalStatus;
    fbe_bool_t  * pCoolingFault;
    fbe_bool_t  foundFlag=0; 

    /********
     * BEGIN
     ********/

    /* The original packet status is FBE_STATUS_INVALID (initialized in fbe_transport_initialize_packet)
     * we set it to FBE_STATUS_OK to start with. 
     * It will be updated as needed later on. 
     */
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);

    payload = fbe_transport_get_payload_ex(packet);
    if(payload == NULL)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                            FBE_TRACE_LEVEL_ERROR,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *)eses_enclosure),
                            "%s, get_payload failed.\n", __FUNCTION__); 

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    control_operation = fbe_payload_ex_get_control_operation(payload);
    if(control_operation == NULL)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                            FBE_TRACE_LEVEL_ERROR,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *)eses_enclosure),
                            "%s, get_control_operation failed.\n", __FUNCTION__); 

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status =  fbe_payload_control_get_buffer(control_operation, &pCoolingFault);
    if( (pCoolingFault == NULL) || (status!=FBE_STATUS_OK))
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                               FBE_TRACE_LEVEL_WARNING,
                               fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *)eses_enclosure),
                               "%s, get_buffer failed, status 0x%x, pCoolingFault %p.\n",
                               __FUNCTION__, status, pCoolingFault);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Verify the control buffer size with the size of the expected structure */
    status = fbe_payload_control_get_buffer_length(control_operation, &control_buffer_length);
    if((status != FBE_STATUS_OK) || 
       (control_buffer_length != sizeof(fbe_bool_t)))
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                               FBE_TRACE_LEVEL_WARNING,
                               fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *)eses_enclosure),
                               "%s, get_buffer_length, bufferLen %d, expectedLen %d, status 0x%x.\n", 
                               __FUNCTION__, 
                               control_buffer_length, 
                               (int)sizeof(fbe_bool_t), status);
        
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    } 

    // Get the total number of Cooling Components including the overall elements.
    status = fbe_base_enclosure_get_number_of_components((fbe_base_enclosure_t *)eses_enclosure,
                                                             FBE_ENCL_COOLING_COMPONENT,
                                                             &coolingComponentCount);
    if(status != FBE_STATUS_OK)
    {
        fbe_transport_set_status(packet, status, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    for (coolingIndex = 0; coolingIndex < coolingComponentCount; coolingIndex++)
    {
        /* Get subtype. */
        enclStatus = fbe_base_enclosure_edal_getU8_thread_safe((fbe_base_enclosure_t *)eses_enclosure,
                                                               FBE_ENCL_COMP_SUBTYPE,
                                                               FBE_ENCL_COOLING_COMPONENT,
                                                               coolingIndex,
                                                               &subtype);
        if (enclStatus != FBE_ENCLOSURE_STATUS_OK)
        {
            fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
            fbe_transport_complete_packet(packet);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        /* Get containerIndex */
        enclStatus = fbe_base_enclosure_edal_getU8_thread_safe((fbe_base_enclosure_t *)eses_enclosure,
                                                               FBE_ENCL_COMP_CONTAINER_INDEX,
                                                               FBE_ENCL_COOLING_COMPONENT,
                                                               coolingIndex,
                                                               &containerIndex);
        if (enclStatus != FBE_ENCLOSURE_STATUS_OK)
        {
            fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
            fbe_transport_complete_packet(packet);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        if((subtype == FBE_ENCL_COOLING_SUBTYPE_CHASSIS) &&
           (containerIndex == FBE_ENCLOSURE_CONTAINER_INDEX_SELF_CONTAINED))
        {
            /* Only check the overall element, not the individual element. */
            // Inserted.
            enclStatus = fbe_base_enclosure_edal_getU8_thread_safe((fbe_base_enclosure_t *)eses_enclosure,
                                                            FBE_ENCL_COMP_ADDL_STATUS,  // Attribute.
                                                            FBE_ENCL_COOLING_COMPONENT, // Component type.
                                                            coolingIndex, // Component index.
                                                            &additionalStatus);  // The value to be returned.

            if (enclStatus != FBE_ENCLOSURE_STATUS_OK)
            {
                fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
                fbe_transport_complete_packet(packet);
                return FBE_STATUS_GENERIC_FAILURE;
            }

            foundFlag=TRUE;

            break;
        } // End of the overall element found.
    }// End of - for (coolingIndex = 0; coolingIndex < coolingComponentCount; coolingIndex++)

    /* Set the payload control status. */
    fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    /* Set the payload control status qualifier with the encl_status. */
    fbe_payload_control_set_status_qualifier(control_operation, FBE_ENCLOSURE_STATUS_OK);
    /* Set packet status. */
    if (foundFlag==TRUE)
    {
        if (additionalStatus == SES_STAT_CODE_CRITICAL)
        {
            *pCoolingFault = TRUE;
        }
        else
        {
            *pCoolingFault = FALSE;
        }

        fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    }
    else
    {
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
    }
    /* Complete the packet. */
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
    
} // End of function - fbe_eses_enclosure_get_chassis_cooling_status



/*!*************************************************************************
* @fn fbe_eses_enclosure_get_drive_slot_info()                  
***************************************************************************
* @brief
*       This function gets the drive slot info.
*
* @param     eses_enclosure - The pointer to the fbe_eses_enclosure_t.
* @param     packet - The pointer to the fbe_packet_t.
*
* @return    fbe_status_t.
*
* @version
*   14-Mar-2011 PHE - Created.
***************************************************************************/
static fbe_status_t 
fbe_eses_enclosure_get_drive_slot_info(fbe_eses_enclosure_t *eses_enclosure, 
                                fbe_packet_t *packet)
{
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;   
    fbe_payload_control_buffer_length_t control_buffer_length = 0;
    fbe_enclosure_mgmt_get_drive_slot_info_t * pGetDriveSlotInfo = NULL;
    fbe_u8_t phyIndex = 0;
    fbe_u8_t driveSlotCount = 0;
    fbe_u32_t   driveSlotIndex;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_enclosure_status_t enclStatus = FBE_ENCLOSURE_STATUS_OK;
    /********
     * BEGIN
     ********/

    /* The original packet status is FBE_STATUS_INVALID (initialized in fbe_transport_initialize_packet)
     * we set it to FBE_STATUS_OK to start with. 
     * It will be updated as needed later on. 
     */
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);

    payload = fbe_transport_get_payload_ex(packet);
    if(payload == NULL)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                            FBE_TRACE_LEVEL_ERROR,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *)eses_enclosure),
                            "%s, get_payload failed.\n", __FUNCTION__); 

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    control_operation = fbe_payload_ex_get_control_operation(payload);
    if(control_operation == NULL)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                            FBE_TRACE_LEVEL_ERROR,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *)eses_enclosure),
                            "%s, get_control_operation failed.\n", __FUNCTION__); 

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status =  fbe_payload_control_get_buffer(control_operation, &pGetDriveSlotInfo);
    if( (pGetDriveSlotInfo == NULL) || (status != FBE_STATUS_OK))
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                               FBE_TRACE_LEVEL_WARNING,
                               fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *)eses_enclosure),
                               "%s, get_buffer failed, status 0x%x, pGetDriveSlotInfo %p.\n",
                               __FUNCTION__, status, pGetDriveSlotInfo);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Verify the control buffer size with the size of the expected structure */
    status = fbe_payload_control_get_buffer_length(control_operation, &control_buffer_length);
    if((status != FBE_STATUS_OK) || 
       (control_buffer_length != sizeof(fbe_enclosure_mgmt_get_drive_slot_info_t)))
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                               FBE_TRACE_LEVEL_WARNING,
                               fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *)eses_enclosure),
                               "%s, get_buffer_length, bufferLen %d, expectedLen: %llu, status: 0x%x.\n", 
                               __FUNCTION__, 
                               control_buffer_length, 
                               (unsigned long long)sizeof(fbe_enclosure_mgmt_get_drive_slot_info_t), status);
        
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    } 

    /* Get the drive slot count. */
    status = fbe_base_enclosure_get_number_of_components((fbe_base_enclosure_t *)eses_enclosure,
                                                             FBE_ENCL_DRIVE_SLOT,
                                                             &driveSlotCount);
    
    // get the slot index for this drive, needed because voyager drives are split between lccs
    driveSlotIndex =  fbe_base_enclosure_find_first_edal_match_U8((fbe_base_enclosure_t *)eses_enclosure,
                                                                  FBE_ENCL_DRIVE_SLOT_NUMBER,         // attribute
                                                                  FBE_ENCL_DRIVE_SLOT,                // component type
                                                                  0,                                  // starting index
                                                                  pGetDriveSlotInfo->location.slot);  // drive slot to match
    
    if((status != FBE_STATUS_OK) || (driveSlotIndex >= driveSlotCount))
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                               FBE_TRACE_LEVEL_WARNING,
                               fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *)eses_enclosure),
                               "%s, get_number_of_components fail, driveSlotCount %d, encl drive slot %d, slot index %d, status 0x%x.\n", 
                               __FUNCTION__, 
                               driveSlotCount, 
                               pGetDriveSlotInfo->location.slot, 
                               driveSlotIndex,
                               status);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    enclStatus = fbe_eses_enclosure_get_local_lcc_side_id(eses_enclosure, 
                                                          &pGetDriveSlotInfo->localSideId);

    if (enclStatus != FBE_ENCLOSURE_STATUS_OK)
    {
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    } 

    enclStatus = fbe_base_enclosure_edal_getU8_thread_safe((fbe_base_enclosure_t *)eses_enclosure,
                                                           FBE_ENCL_DRIVE_PHY_INDEX,
                                                           FBE_ENCL_DRIVE_SLOT,
                                                           driveSlotIndex,
                                                           &phyIndex);

    if (enclStatus != FBE_ENCLOSURE_STATUS_OK)
    {
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    /* Get bypassed. */
    enclStatus = fbe_base_enclosure_edal_getBool_thread_safe((fbe_base_enclosure_t *)eses_enclosure,
                                                           FBE_ENCL_EXP_PHY_DISABLED,
                                                           FBE_ENCL_EXPANDER_PHY,
                                                           phyIndex,
                                                           &pGetDriveSlotInfo->bypassed);
    if (enclStatus != FBE_ENCLOSURE_STATUS_OK)
    {
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Get selfBypassed (bypassed by EMA due to a problem it self-diagnosed)*/
    enclStatus = fbe_base_enclosure_edal_getBool_thread_safe((fbe_base_enclosure_t *)eses_enclosure,
                                                           FBE_ENCL_EXP_PHY_FORCE_DISABLED,
                                                           FBE_ENCL_EXPANDER_PHY,
                                                           phyIndex,
                                                           &pGetDriveSlotInfo->selfBypassed);
    if (enclStatus != FBE_ENCLOSURE_STATUS_OK)
    {
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Get poweredOff */
    enclStatus = fbe_base_enclosure_edal_getBool_thread_safe((fbe_base_enclosure_t *)eses_enclosure,
                                                           FBE_ENCL_COMP_POWERED_OFF,
                                                           FBE_ENCL_DRIVE_SLOT,
                                                           driveSlotIndex,
                                                           &pGetDriveSlotInfo->poweredOff);
    if (enclStatus != FBE_ENCLOSURE_STATUS_OK)
    {
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Get insertedness in this slot */
    enclStatus = fbe_base_enclosure_edal_getBool_thread_safe((fbe_base_enclosure_t *)eses_enclosure,
                                                              FBE_ENCL_COMP_INSERTED,
                                                              FBE_ENCL_DRIVE_SLOT,
                                                              driveSlotIndex,
                                                              &pGetDriveSlotInfo->inserted);

    if (enclStatus != FBE_ENCLOSURE_STATUS_OK)
    {
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }   
     
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);

    return FBE_STATUS_OK;
    
} // End of function - fbe_eses_enclosure_get_drive_slot_info

/*!*************************************************************************
* @fn fbe_eses_enclosure_getShutdownInfo()                  
***************************************************************************
* @brief
*       This function gets the enclosure's Shutdown info.
*
* @param     eses_enclosure - The pointer to the fbe_eses_enclosure_t.
* @param     packet - The pointer to the fbe_packet_t.
*
* @return    fbe_status_t.
*
* @version
*   24-Mar-2011     Joe Perry - Created.
***************************************************************************/
static fbe_status_t 
fbe_eses_enclosure_getShutdownInfo(fbe_eses_enclosure_t *eses_enclosure, 
                                   fbe_packet_t *packet)
{
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;   
    fbe_payload_control_buffer_length_t control_buffer_length = 0;
    fbe_enclosure_mgmt_get_shutdown_info_t * pGetShutdownInfo = NULL;
    fbe_u8_t shutdownReason = 0;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_enclosure_status_t enclStatus = FBE_ENCLOSURE_STATUS_OK;
    /********
     * BEGIN
     ********/

    /* The original packet status is FBE_STATUS_INVALID (initialized in fbe_transport_initialize_packet)
     * we set it to FBE_STATUS_OK to start with. 
     * It will be updated as needed later on. 
     */
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);

    payload = fbe_transport_get_payload_ex(packet);
    if(payload == NULL)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                            FBE_TRACE_LEVEL_ERROR,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *)eses_enclosure),
                            "%s, get_payload failed.\n", __FUNCTION__); 

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    control_operation = fbe_payload_ex_get_control_operation(payload);
    if(control_operation == NULL)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                            FBE_TRACE_LEVEL_ERROR,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *)eses_enclosure),
                            "%s, get_control_operation failed.\n", __FUNCTION__); 

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status =  fbe_payload_control_get_buffer(control_operation, &pGetShutdownInfo);
    if( (pGetShutdownInfo == NULL) || (status != FBE_STATUS_OK))
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                               FBE_TRACE_LEVEL_WARNING,
                               fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *)eses_enclosure),
                               "%s, get_buffer failed, status 0x%x, pGetShutdownInfo %p.\n",
                               __FUNCTION__, status, pGetShutdownInfo);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Verify the control buffer size with the size of the expected structure */
    status = fbe_payload_control_get_buffer_length(control_operation, &control_buffer_length);
    if((status != FBE_STATUS_OK) || 
       (control_buffer_length != sizeof(fbe_enclosure_mgmt_get_shutdown_info_t)))
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                               FBE_TRACE_LEVEL_WARNING,
                               fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *)eses_enclosure),
                               "%s, get_buffer_length, bufferLen %d, expectedLen: %llu, status: 0x%x.\n", 
                               __FUNCTION__, 
                               control_buffer_length, 
                               (unsigned long long)sizeof(fbe_enclosure_mgmt_get_shutdown_info_t), status);
        
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    } 

    // Get the enclosure Shutdown Reason
    enclStatus = fbe_base_enclosure_edal_getU8_thread_safe((fbe_base_enclosure_t *)eses_enclosure,
                                                           FBE_ENCL_SHUTDOWN_REASON,
                                                           FBE_ENCL_ENCLOSURE,
                                                           0,
                                                           &shutdownReason);

    if (enclStatus == FBE_ENCLOSURE_STATUS_OK)
    {
        pGetShutdownInfo->shutdownReason = shutdownReason;
    }
    else
    {
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);

    return FBE_STATUS_OK;
    
} // End of function - fbe_eses_enclosure_getShutdownInfo


/*!*************************************************************************
* @fn fbe_eses_enclosure_get_overtemp_info()                  
***************************************************************************
* @brief
*       This function gets the enclosure's OverTemp info for DAE. 
*       There are lots of temp sensors in a DAE, but we just get the
*       overall temp sensor in chassis sub-enclosure for this OT info.
*
* @param     eses_enclosure - The pointer to the fbe_eses_enclosure_t.
* @param     packet - The pointer to the fbe_packet_t.
*
* @return    fbe_status_t.
*
* @version
*   06-Feb-2012     Rui Chang - Created.
***************************************************************************/
static fbe_status_t 
fbe_eses_enclosure_get_overtemp_info(fbe_eses_enclosure_t *eses_enclosure, 
                                   fbe_packet_t *packet)
{
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;   
    fbe_payload_control_buffer_length_t control_buffer_length = 0;
    fbe_enclosure_mgmt_get_overtemp_info_t* pGetOverTempInfo = NULL;
    fbe_bool_t    overTempWarning=0;
    fbe_bool_t    overTempFailure=0;
//    fbe_u8_t      elemIndex=0;
    fbe_u8_t      subType=FBE_ENCL_TEMP_SENSOR_SUBTYPE_INVALID;
    fbe_u32_t      sensorNum=0;
    fbe_u32_t      sensorIndex=0;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_enclosure_status_t enclStatus = FBE_ENCLOSURE_STATUS_OK;
    fbe_enclosure_status_t enclStatus1 = FBE_ENCLOSURE_STATUS_OK;
    /********
     * BEGIN
     ********/

    /* The original packet status is FBE_STATUS_INVALID (initialized in fbe_transport_initialize_packet)
     * we set it to FBE_STATUS_OK to start with. 
     * It will be updated as needed later on. 
     */
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);

    payload = fbe_transport_get_payload_ex(packet);
    if(payload == NULL)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                            FBE_TRACE_LEVEL_ERROR,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *)eses_enclosure),
                            "%s, get_payload failed.\n", __FUNCTION__); 

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    control_operation = fbe_payload_ex_get_control_operation(payload);
    if(control_operation == NULL)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                            FBE_TRACE_LEVEL_ERROR,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *)eses_enclosure),
                            "%s, get_control_operation failed.\n", __FUNCTION__); 

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status =  fbe_payload_control_get_buffer(control_operation, &pGetOverTempInfo);
    if( (pGetOverTempInfo == NULL) || (status != FBE_STATUS_OK))
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                               FBE_TRACE_LEVEL_WARNING,
                               fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *)eses_enclosure),
                               "%s, get_buffer failed, status 0x%x, pGetOverTempInfo %p.\n",
                               __FUNCTION__, status,pGetOverTempInfo);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Verify the control buffer size with the size of the expected structure */
    status = fbe_payload_control_get_buffer_length(control_operation, &control_buffer_length);
    if((status != FBE_STATUS_OK) || 
       (control_buffer_length != sizeof(fbe_enclosure_mgmt_get_overtemp_info_t)))
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                               FBE_TRACE_LEVEL_WARNING,
                               fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *)eses_enclosure),
                               "%s, get_buffer_length, bufferLen %d, expectedLen: %llu, status: 0x%x.\n", 
                               __FUNCTION__, 
                               control_buffer_length, 
                               (unsigned long long)sizeof(fbe_enclosure_mgmt_get_overtemp_info_t), status);
        
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    } 

    /* if there is no temp sensor, just return */
    enclStatus = fbe_edal_getSpecificComponentCount(((fbe_base_enclosure_t *)eses_enclosure)->enclosureComponentTypeBlock,
                                           FBE_ENCL_TEMP_SENSOR,
                                           &sensorNum);
    if (sensorNum == 0)
    {
        pGetOverTempInfo->overTempWarning = FBE_FALSE;
        pGetOverTempInfo->overTempFailure = FBE_FALSE;
        
        fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_OK;
    }

    /* Get the enclosure OverTemp warning and failure status.
     * We get OT failure and warning from every temp sensor of chassis sub enclosure
     */
    for (sensorIndex = 0; sensorIndex < sensorNum; sensorIndex++)
    {
        enclStatus = fbe_base_enclosure_edal_getU8_thread_safe((fbe_base_enclosure_t *)eses_enclosure,
                                                                FBE_ENCL_COMP_SUBTYPE,
                                                                FBE_ENCL_TEMP_SENSOR,
                                                                sensorIndex,
                                                                &subType);

        if (enclStatus != FBE_ENCLOSURE_STATUS_OK ||
            subType != FBE_ENCL_TEMP_SENSOR_SUBTYPE_CHASSIS)
        {
            continue;
        }

        enclStatus = fbe_base_enclosure_edal_getBool((fbe_base_enclosure_t *)eses_enclosure,
                                                           FBE_ENCL_TEMP_SENSOR_OT_WARNING,
                                                           FBE_ENCL_TEMP_SENSOR,
                                                           sensorIndex,
                                                           &overTempWarning);

        enclStatus1 = fbe_base_enclosure_edal_getBool((fbe_base_enclosure_t *)eses_enclosure,
                                                           FBE_ENCL_TEMP_SENSOR_OT_FAILURE,
                                                           FBE_ENCL_TEMP_SENSOR,
                                                           sensorIndex,
                                                           &overTempFailure);

        if ((enclStatus == FBE_ENCLOSURE_STATUS_OK) &&
            (enclStatus1 == FBE_ENCLOSURE_STATUS_OK))
        {
            pGetOverTempInfo->overTempWarning  = (overTempWarning || pGetOverTempInfo->overTempWarning);
            pGetOverTempInfo->overTempFailure = (overTempFailure || pGetOverTempInfo->overTempFailure);
        }

    }

    // if OT failure is set, we want to clear OT warning
    if (pGetOverTempInfo->overTempFailure)
    {
        pGetOverTempInfo->overTempWarning = FALSE;
    }

    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;

} // End of function - fbe_eses_enclosure_get_overtemp_info

/*!*************************************************************************
* @fn fbe_eses_enclosure_get_sps_info()                  
***************************************************************************
* @brief
*       This function gets the enclosure's SPS info for DAE. 
*
* @param     eses_enclosure - The pointer to the fbe_eses_enclosure_t.
* @param     packet - The pointer to the fbe_packet_t.
*
* @return    fbe_status_t.
*
* @version
*   06-Jun-2012     Joe Perry - Created.
***************************************************************************/
static fbe_status_t 
fbe_eses_enclosure_get_sps_info(fbe_eses_enclosure_t *eses_enclosure, 
                                fbe_packet_t *packet)
{
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;   
    fbe_payload_control_buffer_length_t control_buffer_length = 0;
    fbe_sps_get_sps_status_t *pGetSpsInfo = NULL;
    fbe_u32_t       spsCount;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_enclosure_status_t enclStatus = FBE_ENCLOSURE_STATUS_OK;
    fbe_bool_t  spsInserted;
    fbe_u16_t   spsStatus, spsBatTime;
    fbe_u32_t   componentIndex = 0;
    fbe_edal_fw_info_t   edal_fw_info;
    fbe_u32_t   spsFfid;

    /********
     * BEGIN
     ********/

    /* The original packet status is FBE_STATUS_INVALID (initialized in fbe_transport_initialize_packet)
     * we set it to FBE_STATUS_OK to start with. 
     * It will be updated as needed later on. 
     */
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);

    payload = fbe_transport_get_payload_ex(packet);
    if(payload == NULL)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                            FBE_TRACE_LEVEL_ERROR,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *)eses_enclosure),
                            "%s, get_payload failed.\n", __FUNCTION__); 

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    control_operation = fbe_payload_ex_get_control_operation(payload);
    if(control_operation == NULL)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                            FBE_TRACE_LEVEL_ERROR,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *)eses_enclosure),
                            "%s, get_control_operation failed.\n", __FUNCTION__); 

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status =  fbe_payload_control_get_buffer(control_operation, &pGetSpsInfo);
    if( (pGetSpsInfo == NULL) || (status != FBE_STATUS_OK))
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                               FBE_TRACE_LEVEL_WARNING,
                               fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *)eses_enclosure),
                               "%s, get_buffer failed, status 0x%x, pGetSpsInfo %p.\n",
                               __FUNCTION__, status,pGetSpsInfo);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Verify the control buffer size with the size of the expected structure */
    status = fbe_payload_control_get_buffer_length(control_operation, &control_buffer_length);
    if((status != FBE_STATUS_OK) || 
       (control_buffer_length != sizeof(fbe_sps_get_sps_status_t)))
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                               FBE_TRACE_LEVEL_WARNING,
                               fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *)eses_enclosure),
                               "%s, get_buffer_length, bufferLen %d, expectedLen: %d, status: 0x%x.\n", 
                               __FUNCTION__, 
                               control_buffer_length, 
                               (int)sizeof(fbe_sps_get_sps_status_t), status);
        
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    } 

    /* if there is no SPS, just return */
    enclStatus = fbe_edal_getSpecificComponentCount(((fbe_base_enclosure_t *)eses_enclosure)->enclosureComponentTypeBlock,
                                           FBE_ENCL_SPS,
                                           &spsCount);
    if ((enclStatus != FBE_ENCLOSURE_STATUS_OK) || (pGetSpsInfo->spsIndex >= spsCount))
    {
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*
     * Fill in the SPS info
     */
    fbe_set_memory(&pGetSpsInfo->spsInfo, 0, sizeof(fbe_base_sps_info_t));
    // get Insert status
    enclStatus = fbe_base_enclosure_edal_getBool((fbe_base_enclosure_t *)eses_enclosure,
                                                 FBE_ENCL_COMP_INSERTED,
                                                 FBE_ENCL_SPS,
                                                 pGetSpsInfo->spsIndex,
                                                 &spsInserted);
    if (enclStatus != FBE_ENCLOSURE_STATUS_OK)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                               FBE_TRACE_LEVEL_ERROR,
                               fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *)eses_enclosure),
                               "%s, error getting SpsInserted, enclStatus %d\n", 
                               __FUNCTION__, enclStatus);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    pGetSpsInfo->spsInfo.spsModulePresent = spsInserted;
    if (spsInserted)
    {
        // get SPS Status & derive values from it
        enclStatus = fbe_base_enclosure_edal_getU16_thread_safe((fbe_base_enclosure_t *)eses_enclosure,
                                                                FBE_SPS_STATUS,
                                                                FBE_ENCL_SPS,
                                                                pGetSpsInfo->spsIndex,
                                                                &spsStatus);
        if (enclStatus != FBE_ENCLOSURE_STATUS_OK)
        {
            fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                   FBE_TRACE_LEVEL_ERROR,
                                   fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *)eses_enclosure),
                                   "%s, error getting SpsStatus, enclStatus %d\n", 
                                   __FUNCTION__, enclStatus);
            fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
            fbe_transport_complete_packet(packet);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        pGetSpsInfo->spsInfo.spsBatteryPresent = !(spsStatus & FBE_HW_SPS_STATUS_BATTERY_NOT_ENGAGED);
        // set SPS Status
        if (!pGetSpsInfo->spsInfo.spsBatteryPresent)
        {
            pGetSpsInfo->spsInfo.spsStatus = SPS_STATE_FAULTED;
        }
        else if ((spsStatus & FBE_HW_SPS_STATUS_BATTERY_AVAILABLE) &&
            !(spsStatus & FBE_HW_SPS_STATUS_ON_BATTERY) &&
            !(spsStatus & FBE_HW_SPS_STATUS_TESTING))
        {
            pGetSpsInfo->spsInfo.spsStatus = SPS_STATE_AVAILABLE;
        }
        else if (!(spsStatus & FBE_HW_SPS_STATUS_BATTERY_AVAILABLE) &&
            (spsStatus & FBE_HW_SPS_STATUS_ON_BATTERY) &&
            !(spsStatus & FBE_HW_SPS_STATUS_TESTING))
        {
            pGetSpsInfo->spsInfo.spsStatus = SPS_STATE_ON_BATTERY;
        }
        else if (!(spsStatus & FBE_HW_SPS_STATUS_BATTERY_AVAILABLE) &&
            !(spsStatus & FBE_HW_SPS_STATUS_ON_BATTERY) &&
            (spsStatus & FBE_HW_SPS_STATUS_TESTING))
        {
            pGetSpsInfo->spsInfo.spsStatus = SPS_STATE_TESTING;
        }
        else if (!(spsStatus & FBE_HW_SPS_STATUS_BATTERY_AVAILABLE) &&
            !(spsStatus & FBE_HW_SPS_STATUS_ON_BATTERY) &&
            !(spsStatus & FBE_HW_SPS_STATUS_TESTING))
        {
            // check if any faults, else Charging
            if (spsStatus & FBE_HW_SPS_STATUS_INTERNAL_FAULT)
            {
                pGetSpsInfo->spsInfo.spsStatus = SPS_STATE_FAULTED;
            }
            else
            {
                pGetSpsInfo->spsInfo.spsStatus = SPS_STATE_CHARGING;
            }
        }
        // set SPS Faults
        pGetSpsInfo->spsInfo.spsFaultInfo.spsChargerFailure = 
            (spsStatus & FBE_HW_SPS_STATUS_CHARGER_FAILURE);
        pGetSpsInfo->spsInfo.spsFaultInfo.spsBatteryEOL = 
            (spsStatus & FBE_HW_SPS_STATUS_BATTERY_EOL);
        pGetSpsInfo->spsInfo.spsFaultInfo.spsBatteryNotEngagedFault = 
            (spsStatus & FBE_HW_SPS_STATUS_BATTERY_NOT_ENGAGED);
        // ignore Internal Fault if Battery removed
        if (pGetSpsInfo->spsInfo.spsBatteryPresent)
        {
            pGetSpsInfo->spsInfo.spsFaultInfo.spsInternalFault = 
                (spsStatus & FBE_HW_SPS_STATUS_INTERNAL_FAULT);
        }
        pGetSpsInfo->spsInfo.spsFaultInfo.spsCableConnectivityFault = 
            (spsStatus & FBE_HW_SPS_STATUS_CABLE_CONNECTIVITY);
    
        // get SPS Battime
        enclStatus = fbe_base_enclosure_edal_getU16_thread_safe((fbe_base_enclosure_t *)eses_enclosure,
                                                                FBE_SPS_BATTIME,
                                                                FBE_ENCL_SPS,
                                                                pGetSpsInfo->spsIndex,
                                                                &spsBatTime);
        if (enclStatus != FBE_ENCLOSURE_STATUS_OK)
        {
            fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                   FBE_TRACE_LEVEL_ERROR,
                                   fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *)eses_enclosure),
                                   "%s, error getting SpsBatTime, enclStatus %d\n", 
                                   __FUNCTION__, enclStatus);
            fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
            fbe_transport_complete_packet(packet);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        pGetSpsInfo->spsInfo.spsBatteryTime = spsBatTime;
        // get SPS FFID's
        enclStatus = fbe_base_enclosure_edal_getU32_thread_safe((fbe_base_enclosure_t *)eses_enclosure,
                                                                FBE_SPS_FFID,
                                                                FBE_ENCL_SPS,
                                                                pGetSpsInfo->spsIndex,
                                                                &spsFfid);
        if (enclStatus != FBE_ENCLOSURE_STATUS_OK)
        {
            fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                   FBE_TRACE_LEVEL_ERROR,
                                   fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *)eses_enclosure),
                                   "%s, error getting SpsFfid, enclStatus %d\n", 
                                   __FUNCTION__, enclStatus);
            fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
            fbe_transport_complete_packet(packet);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        pGetSpsInfo->spsInfo.spsModuleFfid = spsFfid;
        enclStatus = fbe_base_enclosure_edal_getU32_thread_safe((fbe_base_enclosure_t *)eses_enclosure,
                                                                FBE_SPS_BATTERY_FFID,
                                                                FBE_ENCL_SPS,
                                                                pGetSpsInfo->spsIndex,
                                                                &spsFfid);
        if (enclStatus != FBE_ENCLOSURE_STATUS_OK)
        {
            fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                   FBE_TRACE_LEVEL_ERROR,
                                   fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *)eses_enclosure),
                                   "%s, error getting SpsFfid, enclStatus %d\n", 
                                   __FUNCTION__, enclStatus);
            fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
            fbe_transport_complete_packet(packet);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        pGetSpsInfo->spsInfo.spsBatteryFfid = spsFfid;
        // set spsModel based on FFID
        if (pGetSpsInfo->spsInfo.spsModuleFfid == LI_ION_SPS_2_2_KW_WITHOUT_BATTERY)
        {
            pGetSpsInfo->spsInfo.spsModel = SPS_TYPE_LI_ION_2_2_KW;
        }
        else
        {
            pGetSpsInfo->spsInfo.spsModel = SPS_TYPE_UNKNOWN;
        }
    }

    /* 
     * Since there is only local SPS in the edal, componentIndex should be 0.
     * pGetSpsInfo->spsIndex is 0 for local SPS in ESP. So it happened to work. 
     * But it is incorrect to use spsIndex directly. It needs to be fixed - PINGHE. 
     */
    componentIndex = pGetSpsInfo->spsIndex;
    // Get SPS primary FW Rev
    enclStatus = fbe_base_enclosure_edal_getStr_thread_safe((fbe_base_enclosure_t *)eses_enclosure,
                                    FBE_ENCL_COMP_FW_INFO,
                                    FBE_ENCL_SPS,      // Component type.
                                    componentIndex,                    // Component index.
                                    sizeof(fbe_edal_fw_info_t), 
                                    (char *)&edal_fw_info);     // copy the string here

    if (enclStatus != FBE_ENCLOSURE_STATUS_OK)
    {
        fbe_transport_set_status(packet, FBE_ENCLOSURE_STATUS_CMD_FAILED, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    else
    {
        fbe_zero_memory(&pGetSpsInfo->spsInfo.primaryFirmwareRev[0],
                        FBE_ENCLOSURE_MAX_FIRMWARE_REV_SIZE + 1);

        fbe_copy_memory(&pGetSpsInfo->spsInfo.primaryFirmwareRev[0],
                     &edal_fw_info.fwRevision,
                         FBE_ESES_FW_REVISION_SIZE);
    }

    // Get SPS secondary FW Rev
    enclStatus = fbe_base_enclosure_edal_getStr_thread_safe((fbe_base_enclosure_t *)eses_enclosure,
                                    FBE_SPS_SECONDARY_FW_INFO,
                                    FBE_ENCL_SPS,      // Component type.
                                    componentIndex,                    // Component index.
                                    sizeof(fbe_edal_fw_info_t), 
                                    (char *)&edal_fw_info);     // copy the string here

    if (enclStatus != FBE_ENCLOSURE_STATUS_OK)
    {
        fbe_transport_set_status(packet, FBE_ENCLOSURE_STATUS_CMD_FAILED, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    else
    {
        fbe_zero_memory(&pGetSpsInfo->spsInfo.secondaryFirmwareRev[0],
                        FBE_ENCLOSURE_MAX_FIRMWARE_REV_SIZE + 1);

        fbe_copy_memory(&pGetSpsInfo->spsInfo.secondaryFirmwareRev[0],
                     &edal_fw_info.fwRevision,
                         FBE_ESES_FW_REVISION_SIZE);
    }

    // Get SPS battery FW Rev
    enclStatus = fbe_base_enclosure_edal_getStr_thread_safe((fbe_base_enclosure_t *)eses_enclosure,
                                    FBE_SPS_BATTERY_FW_INFO,
                                    FBE_ENCL_SPS,      // Component type.
                                    componentIndex,                    // Component index.
                                    sizeof(fbe_edal_fw_info_t), 
                                    (char *)&edal_fw_info);     // copy the string here

    if (enclStatus != FBE_ENCLOSURE_STATUS_OK)
    {
        fbe_transport_set_status(packet, FBE_ENCLOSURE_STATUS_CMD_FAILED, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    else
    {
        fbe_zero_memory(&pGetSpsInfo->spsInfo.batteryFirmwareRev[0],
                        FBE_ENCLOSURE_MAX_FIRMWARE_REV_SIZE + 1);

        fbe_copy_memory(&pGetSpsInfo->spsInfo.batteryFirmwareRev[0],
                     &edal_fw_info.fwRevision,
                         FBE_ESES_FW_REVISION_SIZE);
    }

    if(pGetSpsInfo->spsInfo.spsModuleFfid == LI_ION_SPS_2_2_KW_WITHOUT_BATTERY)
    {
        pGetSpsInfo->spsInfo.bSpsModuleDownloadable = FBE_TRUE;
        pGetSpsInfo->spsInfo.programmableCount = 2;
    }
    else
    {
        pGetSpsInfo->spsInfo.bSpsModuleDownloadable = FBE_FALSE;
        pGetSpsInfo->spsInfo.programmableCount = 0;
    }

    if((pGetSpsInfo->spsInfo.spsBatteryFfid == LI_ION_2_42_KW_BATTERY_PACK_082) ||
       (pGetSpsInfo->spsInfo.spsBatteryFfid == LI_ION_2_42_KW_BATTERY_PACK_115))
    {
        pGetSpsInfo->spsInfo.bSpsBatteryDownloadable = FBE_TRUE;
        pGetSpsInfo->spsInfo.programmableCount ++;
    }
    else
    {
        pGetSpsInfo->spsInfo.bSpsBatteryDownloadable = FBE_FALSE;
    }

    eses_enclosure_get_specific_fw_activation_in_progress(eses_enclosure, 
                                    FBE_FW_TARGET_LCC_MAIN, 
                                    &(pGetSpsInfo->spsInfo.lccFwActivationInProgress));

    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;

} // End of function - fbe_eses_enclosure_get_sps_info


/*!*************************************************************************
 *  @fn get_drive_phy_info(fbe_eses_enclosure_t * eses_enclosure, fbe_packet_t * packet)
 **************************************************************************
 *  @brief
 *      This function will process the FBE_DISCOVERY_TRANSPORT_CONTROL_CODE_GET_DRIVE_PHY_INFO
 *      control code.
 *
 *  @param eses_enclosure - pointer to a Eses Enclosure object
 *  @param packet - pointer to an FBE control code packet
 *
 *  @return    Status from the processing of this control code.
 *
 *  @version 
 *    06-June-2011: PHE  - Created
 *
 **************************************************************************/
static fbe_status_t 
fbe_eses_enclosure_get_drive_phy_info(fbe_eses_enclosure_t * eses_enclosure, fbe_packet_t * packet)
{
    fbe_base_enclosure_t                 * base_enclosure = (fbe_base_enclosure_t *)eses_enclosure;
    fbe_payload_control_buffer_length_t    buffer_length = 0;
    fbe_drive_phy_info_t                 * drive_phy_info_p = NULL;
    fbe_discovery_edge_t                 * discovery_edge = NULL;
    fbe_edge_index_t                       server_index;
    fbe_port_number_t                      port_number;
    fbe_enclosure_number_t                 enclosure_number;
    fbe_u8_t                               slot_number;
    fbe_u8_t                               phy_index;
    fbe_u8_t                               phy_id;
    fbe_u8_t                               expander_index;
    fbe_u64_t                              expander_sas_address;
    fbe_u8_t                               expander_element_index;
    fbe_status_t                           status = FBE_STATUS_OK;
    fbe_enclosure_status_t                 encl_status = FBE_ENCLOSURE_STATUS_OK;

    fbe_base_object_trace((fbe_base_object_t*)eses_enclosure,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n", __FUNCTION__);

    discovery_edge = (fbe_discovery_edge_t *)fbe_transport_get_edge(packet);
    fbe_discovery_transport_get_server_index(discovery_edge,&server_index);

    if(server_index >= fbe_base_enclosure_get_number_of_slots(base_enclosure)) 
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                FBE_TRACE_LEVEL_ERROR,
                                fbe_base_enclosure_get_logheader(base_enclosure),
                                "get_drive_phy_info, invalid slot index %d.\n", 
                                server_index);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    buffer_length = sizeof(fbe_drive_phy_info_t);
    status = fbe_base_enclosure_get_packet_payload_control_data(packet,
                                                               (fbe_base_enclosure_t*)eses_enclosure,
                                                                TRUE,
                                                                (fbe_payload_control_buffer_t *)&drive_phy_info_p,
                                                                buffer_length);

    if(status == FBE_STATUS_OK)
    {
        // Get port number
        fbe_base_enclosure_get_port_number(base_enclosure, &port_number);

        // Get enclosure number
        fbe_base_enclosure_get_enclosure_number(base_enclosure, &enclosure_number);


        // Get slot number(server_index -> slot number)
        encl_status = fbe_base_enclosure_edal_getU8_thread_safe(base_enclosure,
                                                    FBE_ENCL_DRIVE_SLOT_NUMBER, // attribute
                                                    FBE_ENCL_DRIVE_SLOT,        // Component type
                                                    server_index,               // drive component index
                                                    &slot_number); 

        if (encl_status != FBE_ENCLOSURE_STATUS_OK)
        {
            fbe_base_enclosure_set_packet_payload_status(packet, encl_status);
            fbe_transport_complete_packet(packet);
            return status; 
        }

        // server_index(slot_index) -> phy_index 
        encl_status = fbe_base_enclosure_edal_getU8_thread_safe((fbe_base_enclosure_t *)eses_enclosure,
                                                            FBE_ENCL_DRIVE_PHY_INDEX, //attribute
                                                            FBE_ENCL_DRIVE_SLOT, // Component type
                                                            server_index, // Component index
                                                            &phy_index);
        
        if(encl_status != FBE_ENCLOSURE_STATUS_OK)
        {
            fbe_base_enclosure_set_packet_payload_status(packet, encl_status);
            fbe_transport_complete_packet(packet);
            return status; 
        }
    
        // Get phy id(phy_index -> phy_id)
        encl_status = fbe_base_enclosure_edal_getU8_thread_safe((fbe_base_enclosure_t *)eses_enclosure,
                                                            FBE_ENCL_EXP_PHY_ID, //attribute
                                                            FBE_ENCL_EXPANDER_PHY,// Component type
                                                            phy_index, // Component index
                                                            &phy_id);
    
        if(encl_status != FBE_ENCLOSURE_STATUS_OK)
        {
            fbe_base_enclosure_set_packet_payload_status(packet, encl_status);
            fbe_transport_complete_packet(packet);
            return status; 
        }
    
        // phy_index -> expander element index. 
        encl_status = fbe_base_enclosure_edal_getU8_thread_safe((fbe_base_enclosure_t *)eses_enclosure,
                                                            FBE_ENCL_EXP_PHY_EXPANDER_ELEM_INDEX, //attribute
                                                            FBE_ENCL_EXPANDER_PHY,// Component type
                                                            phy_index, // Component index
                                                            &expander_element_index);
    
        if(encl_status != FBE_ENCLOSURE_STATUS_OK)
        {
            fbe_base_enclosure_set_packet_payload_status(packet, encl_status);
            fbe_transport_complete_packet(packet);
            return status; 
        }
    
        // Find the expander_index which has the specified expander_element_index.
        expander_index = 
        fbe_base_enclosure_find_first_edal_match_U8((fbe_base_enclosure_t *)eses_enclosure,
                                                    FBE_ENCL_COMP_ELEM_INDEX,  //attribute
                                                    FBE_ENCL_EXPANDER,  // Component type
                                                    0, //starting index
                                                    expander_element_index);   
    
        if(expander_index == FBE_ENCLOSURE_VALUE_INVALID)
        {
            fbe_base_enclosure_set_packet_payload_status(packet, encl_status);
            fbe_transport_complete_packet(packet);
            return status; 
        }
    
        // Get expander sas address
        encl_status = fbe_base_enclosure_edal_getU64_thread_safe((fbe_base_enclosure_t *)eses_enclosure,
                                                            FBE_ENCL_EXP_SAS_ADDRESS, //attribute
                                                            FBE_ENCL_EXPANDER,// Component type
                                                            expander_index,  // Component index
                                                            &expander_sas_address);
    
    
        if(encl_status != FBE_ENCLOSURE_STATUS_OK)
        {
            fbe_base_enclosure_set_packet_payload_status(packet, encl_status);
            fbe_transport_complete_packet(packet);
            return status; 
        } 
    
        drive_phy_info_p->port_number = port_number;
        drive_phy_info_p->enclosure_number = enclosure_number;
        drive_phy_info_p->slot_number = slot_number;
        drive_phy_info_p->expander_sas_address = expander_sas_address;
        drive_phy_info_p->phy_id = phy_id;

        fbe_base_enclosure_set_packet_payload_status(packet, encl_status);
    }
    else
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                                FBE_TRACE_LEVEL_ERROR,
                                fbe_base_enclosure_get_logheader(base_enclosure),
                                "get_drive_phy_info, get_packet_payload_control_data failed, status 0x%x.\n", 
                                status);

        fbe_transport_set_status(packet, status, 0);
    }
    
    fbe_transport_complete_packet(packet);
    return status; 
}
    

/*!*************************************************************************
* @fn fbe_eses_enclosure_get_connector_info()                  
***************************************************************************
* @brief
*       This function gets the connector info.
*
* @param     eses_enclosure - The pointer to the fbe_eses_enclosure_t.
* @param     packet - The pointer to the fbe_packet_t.
*
* @return    fbe_status_t.
*
* @version
*   23-Aug-2011     PHE - Created.
***************************************************************************/
static fbe_status_t 
fbe_eses_enclosure_get_connector_info(fbe_eses_enclosure_t *eses_enclosure, 
                            fbe_packet_t *packet)
{
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;   
    fbe_payload_control_buffer_t control_buffer = NULL;    
    fbe_payload_control_buffer_length_t control_buffer_length = 0;
    fbe_enclosure_mgmt_get_connector_info_t * pGetConnectorInfo = NULL;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_enclosure_status_t enclStatus = FBE_ENCLOSURE_STATUS_OK;
    fbe_bool_t          boolReturnVal = FBE_FALSE;
    fbe_u32_t           component_index = 0;
    fbe_bool_t          isLocalFru = FBE_FALSE;
    fbe_bool_t          isPrimaryPort = FBE_FALSE;
    fbe_u8_t            attachedSubEnclId = FBE_ESES_SUBENCL_SIDE_ID_INVALID;
    fbe_u8_t            connectorId = 0;
    fbe_u8_t            connectorType = FBE_ESES_CONN_TYPE_UNKNOWN;
    fbe_u64_t           attachedSasAddress = FBE_SAS_ADDRESS_INVALID;

    /********
     * BEGIN
     ********/

    /* The original packet status is FBE_STATUS_INVALID (initialized in fbe_transport_initialize_packet)
     * we set it to FBE_STATUS_OK to start with. 
     * It will be updated as needed later on. 
     */
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);

    payload = fbe_transport_get_payload_ex(packet);
    if(payload == NULL)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                            FBE_TRACE_LEVEL_ERROR,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *)eses_enclosure),
                            "%s, get_payload failed.\n", __FUNCTION__); 
        return FBE_STATUS_GENERIC_FAILURE;
    }

    control_operation = fbe_payload_ex_get_control_operation(payload);
    if(control_operation == NULL)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                            FBE_TRACE_LEVEL_ERROR,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *)eses_enclosure),
                            "%s, get_control_operation failed.\n", __FUNCTION__); 

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status =  fbe_payload_control_get_buffer(control_operation, &control_buffer);
    if( (control_buffer == NULL) || (status!=FBE_STATUS_OK))
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                               FBE_TRACE_LEVEL_WARNING,
                               fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *)eses_enclosure),
                               "%s, get_buffer failed, status: 0x%x, control_buffer: %64p.\n",
                               __FUNCTION__, status, control_buffer);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Set the control operation status and status qualifier to OK to start with. 
     * They will be updated as needed later on.
     */
    fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_payload_control_set_status_qualifier(control_operation, FBE_ENCLOSURE_STATUS_OK);

    pGetConnectorInfo = (fbe_enclosure_mgmt_get_connector_info_t *)control_buffer;

    /* Verify the control buffer size with the size of the expected structure */
    status = fbe_payload_control_get_buffer_length(control_operation, &control_buffer_length);
    if((status != FBE_STATUS_OK) || 
       (control_buffer_length != sizeof(fbe_enclosure_mgmt_get_connector_info_t)))
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                               FBE_TRACE_LEVEL_WARNING,
                               fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *)eses_enclosure),
                                "%s, get_buffer_length, bufferLen %d, expectedLen: %llu, status: 0x%x.\n", 
                                __FUNCTION__, control_buffer_length, 
                                (unsigned long long)sizeof(fbe_enclosure_mgmt_get_connector_info_t), status);
        
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    } 

    /* We want to find the entire connector with the specified location.
     * The entire connector is populated before its individual lanes.
     * By looking for the location, the first connector that we find must be the entire connector. 
     */
    component_index =  fbe_base_enclosure_find_first_edal_match_U8((fbe_base_enclosure_t *)eses_enclosure,
                                                FBE_ENCL_COMP_LOCATION,  //attribute
                                                FBE_ENCL_CONNECTOR, // Component type
                                                0, //starting index
                                                pGetConnectorInfo->location.slot);

    if(component_index == FBE_ENCLOSURE_VALUE_INVALID) 
    {
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* isLocalFru */
    enclStatus = fbe_base_enclosure_edal_getBool_thread_safe((fbe_base_enclosure_t *)eses_enclosure,
                                                            FBE_ENCL_COMP_IS_LOCAL, // Attribute.
                                                            FBE_ENCL_CONNECTOR,  // Component type.
                                                            component_index, // Component index.
                                                            &isLocalFru); // The value to be returned.

    if (enclStatus != FBE_ENCLOSURE_STATUS_OK)
    {
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    else
    {
        pGetConnectorInfo->connectorInfo.isLocalFru = isLocalFru;
    }
    
    /* Inserted */
    enclStatus = fbe_base_enclosure_edal_getBool_thread_safe((fbe_base_enclosure_t *)eses_enclosure,
                                                            FBE_ENCL_COMP_INSERTED, // Attribute.
                                                            FBE_ENCL_CONNECTOR,  // Component type.
                                                            component_index, // Component index.
                                                            &boolReturnVal); // The value to be returned.

    if (enclStatus != FBE_ENCLOSURE_STATUS_OK)
    {
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    else
    {
        pGetConnectorInfo->connectorInfo.inserted = boolReturnVal;
    }

    /* Disabled */       
    enclStatus = fbe_base_enclosure_edal_getBool_thread_safe((fbe_base_enclosure_t *)eses_enclosure,
                                                            FBE_ENCL_CONNECTOR_DISABLED, // Attribute.
                                                            FBE_ENCL_CONNECTOR,  // Component type.
                                                            component_index, // Component index.
                                                            &boolReturnVal); // The value to be returned.
    if (enclStatus != FBE_ENCLOSURE_STATUS_OK)
    {
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    else if(boolReturnVal) 
    {
        pGetConnectorInfo->connectorInfo.cableStatus = FBE_CABLE_STATUS_DISABLED;
    }
    else
    {
    
        /* Degraded */       
        enclStatus = fbe_base_enclosure_edal_getBool_thread_safe((fbe_base_enclosure_t *)eses_enclosure,
                                                                FBE_ENCL_CONNECTOR_DEGRADED, // Attribute.
                                                                FBE_ENCL_CONNECTOR,  // Component type.
                                                                component_index, // Component index.
                                                                &boolReturnVal); // The value to be returned.
        if (enclStatus != FBE_ENCLOSURE_STATUS_OK)
        {
            fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
            fbe_transport_complete_packet(packet);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        else if(boolReturnVal) 
        {
            pGetConnectorInfo->connectorInfo.cableStatus = FBE_CABLE_STATUS_DEGRADED;
        }
        else
        {
            pGetConnectorInfo->connectorInfo.cableStatus = FBE_CABLE_STATUS_GOOD;
        }
    }

    /* primiary port */
    enclStatus = fbe_base_enclosure_edal_getBool_thread_safe((fbe_base_enclosure_t *)eses_enclosure,
                                                                    FBE_ENCL_CONNECTOR_PRIMARY_PORT, // Attribute.
                                                                    FBE_ENCL_CONNECTOR,  // Component type.
                                                                    component_index, // Component index.
                                                                    &isPrimaryPort); // The value to be returned.
    if (enclStatus != FBE_ENCLOSURE_STATUS_OK)
    {
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* attached subenclosure id */
    enclStatus = fbe_base_enclosure_edal_getU8_thread_safe((fbe_base_enclosure_t *)eses_enclosure,
                                                                    FBE_ENCL_CONNECTOR_ATTACHED_SUB_ENCL_ID, // Attribute.
                                                                    FBE_ENCL_CONNECTOR,  // Component type.
                                                                    component_index, // Component index.
                                                                    &attachedSubEnclId); // The value to be returned.
    if (enclStatus != FBE_ENCLOSURE_STATUS_OK)
    {
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* connector id */
    enclStatus = fbe_base_enclosure_edal_getU8_thread_safe((fbe_base_enclosure_t *)eses_enclosure,
                                                                    FBE_ENCL_CONNECTOR_ID, // Attribute.
                                                                    FBE_ENCL_CONNECTOR,  // Component type.
                                                                    component_index, // Component index.
                                                                    &connectorId); // The value to be returned.
    if (enclStatus != FBE_ENCLOSURE_STATUS_OK)
    {
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    else
    {
        pGetConnectorInfo->connectorInfo.connectorId = connectorId;
    }

    /* attached SAS address */
    enclStatus = fbe_base_enclosure_edal_getU64_thread_safe((fbe_base_enclosure_t *)eses_enclosure,
                                                                    FBE_ENCL_CONNECTOR_ATTACHED_SAS_ADDRESS, // Attribute.
                                                                    FBE_ENCL_CONNECTOR,  // Component type.
                                                                    component_index, // Component index.
                                                                    &attachedSasAddress); // The value to be returned.
    if (enclStatus != FBE_ENCLOSURE_STATUS_OK)
    {
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    /* connector type */
    enclStatus = fbe_base_enclosure_edal_getU8_thread_safe((fbe_base_enclosure_t *)eses_enclosure,
                                                                    FBE_ENCL_CONNECTOR_TYPE, // Attribute.
                                                                    FBE_ENCL_CONNECTOR,  // Component type.
                                                                    component_index, // Component index.
                                                                    &connectorType); // The value to be returned.
    if (enclStatus != FBE_ENCLOSURE_STATUS_OK)
    {
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    else
    {
        /* determine the connector type*/
        if (isLocalFru == FBE_TRUE)
        {
            if(connectorType == FBE_ESES_CONN_TYPE_INTERNAL_CONN) 
            {
                pGetConnectorInfo->connectorInfo.connectorType = FBE_CONNECTOR_TYPE_INTERNAL;
            }
            else if (isPrimaryPort == FBE_TRUE)
            {
                pGetConnectorInfo->connectorInfo.connectorType = FBE_CONNECTOR_TYPE_PRIMARY;
            }
            else if((attachedSasAddress != 0) && (attachedSasAddress != FBE_SAS_ADDRESS_INVALID)) 
            {
                pGetConnectorInfo->connectorInfo.connectorType = FBE_CONNECTOR_TYPE_EXPANSION;
            }
            else
            {
                pGetConnectorInfo->connectorInfo.connectorType = FBE_CONNECTOR_TYPE_UNUSED;
            }
        }
        else
        {
            pGetConnectorInfo->connectorInfo.connectorType = FBE_CONNECTOR_TYPE_UNKNOWN;
        }
    }

    /* Set the control operation status qualifier with the encl_status. */
    fbe_payload_control_set_status_qualifier(control_operation, FBE_ENCLOSURE_STATUS_OK);
    /* Complete the packet. */
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);

    return FBE_STATUS_OK;
    
} // End of function - fbe_eses_enclosure_get_connector_info



/*!*************************************************************************
* @fn fbe_eses_enclosure_get_display_info()                  
***************************************************************************
* @brief
*       This function gets the enclosure display information
*
* @param     eses_enclosure - The pointer to the fbe_eses_enclosure_t.
* @param     packet - The pointer to the fbe_packet_t.
*
* @return    fbe_status_t.
*
* @version
*   30-Jan-2012     bphilbin - Created.
***************************************************************************/
static fbe_status_t 
fbe_eses_enclosure_get_display_info(fbe_eses_enclosure_t *eses_enclosure, 
                            fbe_packet_t *packet)
{
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;   
    fbe_payload_control_buffer_t control_buffer = NULL;    
    fbe_encl_display_info_t * pGetDisplayInfo = NULL;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_enclosure_status_t enclStatus = FBE_ENCLOSURE_STATUS_OK;


    
    /********
     * BEGIN
     ********/

    /* The original packet status is FBE_STATUS_INVALID (initialized in fbe_transport_initialize_packet)
     * we set it to FBE_STATUS_OK to start with. 
     * It will be updated as needed later on. 
     */
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);

    payload = fbe_transport_get_payload_ex(packet);
    if(payload == NULL)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                            FBE_TRACE_LEVEL_ERROR,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *)eses_enclosure),
                            "%s, get_payload failed.\n", __FUNCTION__); 
        return FBE_STATUS_GENERIC_FAILURE;
    }

    control_operation = fbe_payload_ex_get_control_operation(payload);
    if(control_operation == NULL)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                            FBE_TRACE_LEVEL_ERROR,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *)eses_enclosure),
                            "%s, get_control_operation failed.\n", __FUNCTION__); 

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status =  fbe_payload_control_get_buffer(control_operation, &control_buffer);
    if( (control_buffer == NULL) || (status!=FBE_STATUS_OK))
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                               FBE_TRACE_LEVEL_WARNING,
                               fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *)eses_enclosure),
                               "%s, get_buffer failed, status: 0x%x, control_buffer: %64p.\n",
                               __FUNCTION__, status, control_buffer);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Set the control operation status and status qualifier to OK to start with. 
     * They will be updated as needed later on.
     */
    fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_payload_control_set_status_qualifier(control_operation, FBE_ENCLOSURE_STATUS_OK);

    pGetDisplayInfo = (fbe_encl_display_info_t *)control_buffer;

    enclStatus = fbe_base_enclosure_edal_getU8_thread_safe((fbe_base_enclosure_t *)eses_enclosure,
                                                            FBE_DISPLAY_CHARACTER_STATUS, // Attribute.
                                                            FBE_ENCL_DISPLAY,  // Component type.
                                                            FBE_EDAL_DISPLAY_BUS_0, // Component index.
                                                            &pGetDisplayInfo->busNumber[0]); // The value to be returned.
    if (enclStatus != FBE_ENCLOSURE_STATUS_OK)
    {
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    enclStatus = fbe_base_enclosure_edal_getU8_thread_safe((fbe_base_enclosure_t *)eses_enclosure,
                                                            FBE_DISPLAY_CHARACTER_STATUS, // Attribute.
                                                            FBE_ENCL_DISPLAY,  // Component type.
                                                            FBE_EDAL_DISPLAY_BUS_1, // Component index.
                                                            &pGetDisplayInfo->busNumber[1]); // The value to be returned.
    if (enclStatus != FBE_ENCLOSURE_STATUS_OK)
    {
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    enclStatus = fbe_base_enclosure_edal_getU8_thread_safe((fbe_base_enclosure_t *)eses_enclosure,
                                                            FBE_DISPLAY_CHARACTER_STATUS, // Attribute.
                                                            FBE_ENCL_DISPLAY,  // Component type.
                                                            FBE_EDAL_DISPLAY_ENCL_0, // Component index.
                                                            &pGetDisplayInfo->enclNumber); // The value to be returned.
    if (enclStatus != FBE_ENCLOSURE_STATUS_OK)
    {
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Set the control operation status qualifier with the encl_status. */
    fbe_payload_control_set_status_qualifier(control_operation, FBE_ENCLOSURE_STATUS_OK);
    /* Complete the packet. */
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);

    return FBE_STATUS_OK;
}

/*!*************************************************************************
* @fn fbe_eses_enclosure_get_edal_fw_info()                  
***************************************************************************
* @brief
*       This function gets the EDAL firmware info for the specified fw target.
*
* @param     eses_enclosure_p - The pointer to the fbe_eses_enclosure_t.
* @param     fw_target - The pointer to the fbe_packet_t.
* @param     side
* @param     edal_fw_info_p (OUTPUT) -
* 
* @return    fbe_enclosure_status_t.
*
* @version
*   09-Oct-2012     PHE - Created.
***************************************************************************/
fbe_enclosure_status_t 
fbe_eses_enclosure_get_edal_fw_info(fbe_eses_enclosure_t *eses_enclosure_p, 
                                    fbe_enclosure_fw_target_t fw_target,
                                    fbe_u32_t side,
                                    fbe_edal_fw_info_t * edal_fw_info_p)
{
    fbe_enclosure_component_types_t     fw_comp_type; 
    fbe_base_enclosure_attributes_t     fw_comp_attr;
    fbe_u8_t                            index = 0; // EDAL component index.
    fbe_enclosure_status_t              enclStatus = FBE_ENCLOSURE_STATUS_OK;

    fbe_edal_get_fw_target_component_type_attr(fw_target,&fw_comp_type,&fw_comp_attr);
    enclStatus = fbe_eses_enclosure_map_fw_target_type_to_component_index(eses_enclosure_p,
                                fw_comp_type, 
                                side, 
                                0,  // Overall Rev.
                                &index);

    if (enclStatus != FBE_ENCLOSURE_STATUS_OK)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure_p,
                    FBE_TRACE_LEVEL_ERROR,
                    fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)eses_enclosure_p),
                    "%s, unsupported fw_target %d, fw_comp_type %d, side %d.\n", 
                    __FUNCTION__, fw_target, fw_comp_type, side);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    // Firmware Rev.
    enclStatus = fbe_base_enclosure_edal_getStr_thread_safe((fbe_base_enclosure_t *)eses_enclosure_p,
                            fw_comp_attr,// Component Attribute
                            fw_comp_type,  // Component type.
                            index, // Component index.
                            sizeof(fbe_edal_fw_info_t), 
                            (char *)edal_fw_info_p);     // copy the rev info

    return enclStatus;
}

/*!*************************************************************************
 *  @fn fbe_eses_enclosure_get_battery_backed_info(fbe_eses_enclosure_t * eses_enclosure, fbe_packet_t * packet)
 **************************************************************************
 *  @brief
 *      This function will process the FBE_BASE_ENCLOSURE_CONTROL_CODE_GET_BATTERY_BACKED_INFO
 *      control code.
 *
 *  @param eses_enclosure - pointer to a Eses Enclosure object
 *  @param packet - pointer to an FBE control code packet
 *
 *  @return    Status from the processing of this control code.
 *
 *  @version 
 *    08-Nov-2012: Rui Chang  - Created
 *
 **************************************************************************/
static fbe_status_t 
fbe_eses_enclosure_get_battery_backed_info(fbe_eses_enclosure_t * eses_enclosure, fbe_packet_t * packet)
{
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;   
    fbe_payload_control_buffer_length_t control_buffer_length = 0;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_enclosure_status_t enclStatus = FBE_ENCLOSURE_STATUS_OK;
    fbe_u8_t driveComponentCount = 0;
    fbe_u32_t driveIndex = 0;
    fbe_bool_t  * pBatteryBackedBitSet;
    fbe_bool_t  batteryBacked=0;
    fbe_u8_t        drive_slot_number;

    /********
     * BEGIN
     ********/

    /* The original packet status is FBE_STATUS_INVALID (initialized in fbe_transport_initialize_packet)
     * we set it to FBE_STATUS_OK to start with. 
     * It will be updated as needed later on. 
     */
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);

    payload = fbe_transport_get_payload_ex(packet);
    if(payload == NULL)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                            FBE_TRACE_LEVEL_ERROR,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *)eses_enclosure),
                            "%s, get_payload failed.\n", __FUNCTION__); 

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    control_operation = fbe_payload_ex_get_control_operation(payload);
    if(control_operation == NULL)
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                            FBE_TRACE_LEVEL_ERROR,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *)eses_enclosure),
                            "%s, get_control_operation failed.\n", __FUNCTION__); 

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status =  fbe_payload_control_get_buffer(control_operation, &pBatteryBackedBitSet);
    if( (pBatteryBackedBitSet == NULL) || (status!=FBE_STATUS_OK))
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                               FBE_TRACE_LEVEL_WARNING,
                               fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *)eses_enclosure),
                               "%s, get_buffer failed, status 0x%x, pBatteryBackedBitSet %p.\n",
                               __FUNCTION__, status, pBatteryBackedBitSet);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Verify the control buffer size with the size of the expected structure */
    status = fbe_payload_control_get_buffer_length(control_operation, &control_buffer_length);
    if((status != FBE_STATUS_OK) || 
       (control_buffer_length != sizeof(fbe_bool_t)))
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)eses_enclosure,
                               FBE_TRACE_LEVEL_WARNING,
                               fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *)eses_enclosure),
                               "%s, get_buffer_length, bufferLen %d, expectedLen %d, status 0x%x.\n", 
                               __FUNCTION__, 
                               control_buffer_length, 
                               (int)sizeof(fbe_bool_t), status);
        
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    } 

    //For jetfire, we need set battery back bits correctly, for other types of enclosure, we just return OK.
    if (eses_enclosure->sas_enclosure_type != FBE_SAS_ENCLOSURE_TYPE_FALLBACK)
    {
        *pBatteryBackedBitSet = TRUE;
        fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
        /* Complete the packet. */
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_OK;
    }

    // Get the total number of drive Components including the overall elements.
    status = fbe_base_enclosure_get_number_of_components((fbe_base_enclosure_t *)eses_enclosure,
                                                             FBE_ENCL_DRIVE_SLOT,
                                                             &driveComponentCount);
    if(status != FBE_STATUS_OK)
    {
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    for (driveIndex = 0; driveIndex < driveComponentCount; driveIndex++)
    {
        enclStatus = fbe_base_enclosure_edal_getU8((fbe_base_enclosure_t *)eses_enclosure,
                                                    FBE_ENCL_DRIVE_SLOT_NUMBER,
                                                    FBE_ENCL_DRIVE_SLOT,
                                                    driveIndex,
                                                    &drive_slot_number);
        if(enclStatus != FBE_ENCLOSURE_STATUS_OK)
        {
            fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
            fbe_transport_complete_packet(packet);
            return FBE_STATUS_GENERIC_FAILURE;        }

        /* Get subtype. */
        enclStatus = fbe_base_enclosure_edal_getBool_thread_safe((fbe_base_enclosure_t *)eses_enclosure,
                                                               FBE_ENCL_DRIVE_BATTERY_BACKED,
                                                               FBE_ENCL_DRIVE_SLOT,
                                                               driveIndex,
                                                               &batteryBacked);
        if (enclStatus != FBE_ENCLOSURE_STATUS_OK)
        {
            fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
            fbe_transport_complete_packet(packet);
            return FBE_STATUS_GENERIC_FAILURE;
        }
    
        //We only use first 4 drives for vaulting in jetfire 
        if ((drive_slot_number > FBE_ESES_ENCLOSURE_MAX_DRIVE_NUMBER_FOR_VAULTING && batteryBacked == TRUE) ||
          (drive_slot_number <= FBE_ESES_ENCLOSURE_MAX_DRIVE_NUMBER_FOR_VAULTING && batteryBacked == FALSE))
        {
            *pBatteryBackedBitSet = FALSE;
            fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
            fbe_transport_complete_packet(packet);
            return FBE_STATUS_OK;
        }
    }// End of - for (coolingIndex = 0; coolingIndex < coolingComponentCount; coolingIndex++)

    *pBatteryBackedBitSet = TRUE;
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
    
}
    
/*!*******************************************************************************
 * @fn fbe_eses_enclosure_get_ps_ac_dc_type(fbe_eses_enclosure_t *eses_enclosure,   
                                     HW_MODULE_TYPE psUniqueId)
 *********************************************************************************
 * @brief
 *  Get the PS type (AC or DC).
 *
 * @param  eses_enclosure - The pointer to the fbe_eses_enclosure_t. 
 * @param  psUniqueId - The PS unique id (ffid).
 *
 * @return AC_DC_INPUT
 *
 * @version
 *  15-Oct-2015 PHE - Created.
 *******************************************************************************/
AC_DC_INPUT
fbe_eses_enclosure_get_ps_ac_dc_type(fbe_eses_enclosure_t *eses_enclosure,   
                                     HW_MODULE_TYPE psUniqueId)
{
    AC_DC_INPUT ac_dc_type = INVALID_AC_DC_INPUT;

    FBE_UNREFERENCED_PARAMETER(eses_enclosure); 

    switch(psUniqueId)
    {
        case DC_TABASCO_PS_COOLING_MODULE:  //DC PS for Tabasco
        case DC_ACBEL_STILETTO:  // DC PS for Ancho
        case DC_ACBEL_DERRINGER:  // DC PS for Derringer
        case HVDC_ACBEL_DERRINGER: // DC PS for Derringer
        case DC_JUNIPER_700: // DC PS for pinecone. We don't support DC pinecone for Beachcomber or Bearcat. 
            ac_dc_type = DC_INPUT;
            break;
        default:
            ac_dc_type = AC_INPUT;
            break;
    }
    
    return ac_dc_type;
}
//End of file fbe_eses_enclosure_usurper.c
