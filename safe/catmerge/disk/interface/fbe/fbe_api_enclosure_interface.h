#ifndef FBE_API_ENCLOSURE_INTERFACE_H
#define FBE_API_ENCLOSURE_INTERFACE_H

/***************************************************************************
 * Copyright (C) EMC Corporation 2000-2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_api_enclosure_interface.h
 ***************************************************************************
 *
 * @brief
 *  This file contains the Enclosure APIs.
 * 
 * @ingroup fbe_api_physical_package_interface_class_files
 * 
 * @version
 *   04/08/00 sharel    Created
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_types.h"
#include "fbe/fbe_enclosure.h"
#include "fbe/fbe_eses.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_eir_info.h"
#include "fbe/fbe_resume_prom_info.h"
#include "fbe/fbe_api_board_interface.h"

//----------------------------------------------------------------
// Define the top level group for the FBE Physical Package APIs 
//----------------------------------------------------------------
/*! @defgroup fbe_physical_package_class FBE Physical Package APIs 
 *  @brief This is the set of definitions for FBE Physical Package APIs.
 *  @ingroup fbe_api_physical_package_interface
 */ 
//----------------------------------------------------------------

//----------------------------------------------------------------
// Define the FBE API Enclosure Interface for the Usurper Interface. 
// This is where all the data structures defined. 
//----------------------------------------------------------------
/*! @defgroup fbe_api_enclosure_usurper_interface FBE API Enclosure Usurper Interface
 *  @brief 
 *    This is the set of definitions that comprise the FBE API Enclosure Interface class
 *    usurper interface.
 *
 *  @ingroup fbe_api_classes_usurper_interface
 *  @{
 */ 
//----------------------------------------------------------------

/*!********************************************************************* 
 * @struct fbe_enclosure_mgmt_resume_read_asynch_t
 *  
 * @brief 
 *   defines async resume prom read operation. 
 *   @ref FBE_BASE_ENCLOSURE_CONTROL_CODE_RESUME_READ. 
 *   The user request needs to fill in all the inputs and the output 
 *   will be returned by the physical package.
 **********************************************************************/
typedef struct fbe_enclosure_mgmt_resume_read_asynch_s{
    fbe_enclosure_mgmt_ctrl_op_t        operation;
    fbe_sg_element_t                    sg_element[2]; /* sg_list to send physical package */ 
    fbe_resume_prom_get_resume_async_t  *pGetResumeProm;
}fbe_enclosure_mgmt_resume_read_asynch_t;

FBE_API_CPP_EXPORT_START

/*! @} */ /* end of fbe_api_enclosure_usurper_interface */

//----------------------------------------------------------------
// Define the group for the FBE API Enclosure Interface.  
// This is where all the function prototypes for the Enclosure API.
//----------------------------------------------------------------
/*! @defgroup fbe_api_enclosure_interface FBE API Enclosure Interface
 *  @brief 
 *    This is the set of FBE API Enclosure Interface.
 *
 *  @details 
 *    In order to access this library, please include fbe_api_enclosure_interface.h.
 *
 *  @ingroup fbe_physical_package_class
 *  @{
 */
//----------------------------------------------------------------
fbe_status_t FBE_API_CALL fbe_api_enclosure_firmware_download (fbe_object_id_t object_id, fbe_enclosure_mgmt_download_op_t* download_firmware);
fbe_status_t FBE_API_CALL fbe_api_enclosure_firmware_download_status(fbe_object_id_t object_id, fbe_enclosure_mgmt_download_op_t* download_status);
fbe_status_t FBE_API_CALL fbe_api_enclosure_firmware_update(fbe_object_id_t object_id,fbe_enclosure_mgmt_download_op_t* firmware_download_status);
fbe_status_t FBE_API_CALL fbe_api_enclosure_firmware_update_status(fbe_object_id_t object_id,fbe_enclosure_mgmt_download_op_t* firmware_download_status);
fbe_status_t FBE_API_CALL fbe_api_enclosure_get_basic_info(fbe_object_id_t object_id, fbe_base_object_mgmt_get_enclosure_basic_info_t *enclosure_info);
fbe_status_t FBE_API_CALL fbe_api_enclosure_get_info(fbe_object_id_t object_id, fbe_base_object_mgmt_get_enclosure_info_t *enclosure_info);
fbe_status_t FBE_API_CALL fbe_api_enclosure_get_prvt_info(fbe_object_id_t object_id, fbe_base_object_mgmt_get_enclosure_prv_info_t *enclosure_prvt_info);
fbe_status_t FBE_API_CALL fbe_api_enclosure_bypass_drive(fbe_object_id_t object_id, fbe_base_enclosure_drive_bypass_command_t *command);
fbe_status_t FBE_API_CALL fbe_api_enclosure_unbypass_drive(fbe_object_id_t object_id, fbe_base_enclosure_drive_unbypass_command_t *command);
fbe_status_t FBE_API_CALL fbe_api_enclosure_getSetupInfo(fbe_object_id_t object_id, fbe_base_object_mgmt_get_enclosure_setup_info_t *enclosureSetupInfo);
fbe_status_t FBE_API_CALL fbe_api_enclosure_elem_group_info(fbe_object_id_t object_id, fbe_eses_elem_group_t *elem_group_info, fbe_u8_t max);
fbe_status_t FBE_API_CALL fbe_api_enclosure_get_max_elem_group(fbe_object_id_t object_id, fbe_enclosure_mgmt_max_elem_group *elem_group_no);
fbe_status_t FBE_API_CALL fbe_api_enclosure_get_dl_info(fbe_object_id_t object_id, fbe_eses_encl_fup_info_t *dl_info, fbe_u32_t num_bytes);
fbe_status_t FBE_API_CALL fbe_api_enclosure_get_eir_info(fbe_object_id_t object_id, fbe_eses_encl_eir_info_t *eir_info);
fbe_status_t FBE_API_CALL fbe_api_enclosure_setControl(fbe_u32_t object_handle_p, fbe_base_object_mgmt_set_enclosure_control_t *enclosureControlInfo);
fbe_status_t FBE_API_CALL  fbe_api_enclosure_setLeds(fbe_u32_t object_handle_p, fbe_base_object_mgmt_set_enclosure_led_t *enclosureLedInfo);
fbe_status_t FBE_API_CALL  fbe_api_enclosure_resetShutdownTimer(fbe_u32_t object_handle_p);
fbe_status_t FBE_API_CALL  fbe_api_enclosure_send_resume_write(fbe_object_id_t object_id, fbe_enclosure_mgmt_ctrl_op_t *operation);
fbe_status_t FBE_API_CALL  fbe_api_enclosure_resume_read(fbe_object_id_t object_id, 
                                           fbe_enclosure_mgmt_ctrl_op_t *operation,
                                           fbe_enclosure_status_t *enclosure_status);
fbe_status_t FBE_API_CALL fbe_api_enclosure_resume_read_synch(fbe_object_id_t object_id, 
                                                               fbe_resume_prom_cmd_t * pReadResumeProm);
fbe_status_t FBE_API_CALL fbe_api_enclosure_resume_write_synch(fbe_object_id_t object_id, 
                                                               fbe_resume_prom_cmd_t * pWriteResumeProm);
fbe_status_t FBE_API_CALL fbe_api_enclosure_resume_read_asynch(fbe_object_id_t object_id, 
                                                               fbe_resume_prom_get_resume_async_t *pGetResumeProm);
fbe_status_t FBE_API_CALL fbe_api_enclosure_read_buffer(fbe_object_id_t object_id, 
                                           fbe_enclosure_mgmt_ctrl_op_t *operation,
                                           fbe_enclosure_status_t *enclosure_status);
fbe_status_t FBE_API_CALL fbe_api_enclosure_send_raw_rcv_diag_pg(fbe_object_id_t object_id, 
                                                    fbe_enclosure_mgmt_ctrl_op_t *operation, 
                                                    fbe_enclosure_status_t *enclosure_status);
fbe_status_t fbe_api_enclosure_setup_info_memory(fbe_base_object_mgmt_get_enclosure_info_t **fbe_enclosure_info);
void fbe_api_enclosure_release_info_memory(fbe_base_object_mgmt_get_enclosure_info_t *fbe_enclosure_info);
fbe_status_t FBE_API_CALL  fbe_api_enclosure_send_raw_inquiry_pg(fbe_object_id_t object_id, 
                                                   fbe_enclosure_mgmt_ctrl_op_t *operation,
                                                   fbe_enclosure_status_t *enclosure_status);

fbe_status_t FBE_API_CALL  fbe_api_enclosure_send_mgmt_ctrl_op_w_rtn_data(fbe_object_id_t object_id, 
                                                   fbe_enclosure_mgmt_ctrl_op_t *operation,
                                                   fbe_base_enclosure_control_code_t control_code,
                                                   fbe_enclosure_status_t *enclosure_status);

fbe_status_t FBE_API_CALL  fbe_api_enclosure_send_mgmt_ctrl_op_no_rtn_data(fbe_object_id_t object_id, 
                                                     fbe_enclosure_mgmt_ctrl_op_t *esesPageOpPtr,
                                                     fbe_base_enclosure_control_code_t control_code);
fbe_status_t FBE_API_CALL
fbe_api_enclosure_send_encl_debug_control(fbe_object_id_t object_id,
                                          fbe_base_object_mgmt_encl_dbg_ctrl_t *enclosureDebugInfo);
fbe_status_t FBE_API_CALL  
fbe_api_enclosure_send_drive_debug_control(fbe_object_id_t object_id, 
                                           fbe_base_object_mgmt_drv_dbg_ctrl_t *enclosureDriveDebugInfo);
fbe_status_t FBE_API_CALL  
fbe_api_enclosure_send_drive_power_control(fbe_object_id_t object_id, 
                                           fbe_base_object_mgmt_drv_power_ctrl_t *drivPowerInfo);
fbe_status_t FBE_API_CALL  
fbe_api_enclosure_send_ps_margin_test_control(fbe_object_id_t object_id, 
                                              fbe_base_enclosure_ps_margin_test_ctrl_t *psMarginTestInfo);
fbe_status_t 
fbe_api_enclosure_send_expander_control(fbe_object_id_t object_id, 
                                        fbe_base_object_mgmt_exp_ctrl_t *expanderInfo);

fbe_status_t FBE_API_CALL  
fbe_api_enclosure_send_exp_test_mode_control(fbe_object_id_t object_id, 
                                             fbe_enclosure_mgmt_ctrl_op_t *esesPageOpPtr);
fbe_status_t FBE_API_CALL  
fbe_api_enclosure_send_exp_string_out_control(fbe_object_id_t object_id, 
                                              fbe_enclosure_mgmt_ctrl_op_t *esesPageOpPtr);
fbe_status_t FBE_API_CALL  
fbe_api_enclosure_send_exp_threshold_out_control(fbe_object_id_t object_id, 
                                              fbe_enclosure_mgmt_ctrl_op_t *esesPageOpPtr);
fbe_status_t FBE_API_CALL 
fbe_api_enclosure_update_drive_fault_led(fbe_u32_t bus, 
                                         fbe_u32_t enclosure, 
                                         fbe_u32_t slot,
                                         fbe_base_enclosure_led_behavior_t  ledBehavior);

fbe_status_t 
fbe_api_enclosure_send_set_led(fbe_object_id_t object_id, 
                               fbe_base_enclosure_led_status_control_t *ledStatusControlInfo);

fbe_status_t FBE_API_CALL  fbe_api_enclosure_get_trace_buffer_info(fbe_object_id_t object_id, 
                                                     fbe_enclosure_mgmt_ctrl_op_t *eses_page_op,
                                                     fbe_enclosure_status_t *enclosure_status);

fbe_status_t FBE_API_CALL  fbe_api_enclosure_send_trace_buffer_control(fbe_object_id_t object_id, fbe_enclosure_mgmt_ctrl_op_t *eses_page_op);
fbe_status_t FBE_API_CALL  fbe_api_enclosure_build_read_buf_cmd(fbe_enclosure_mgmt_ctrl_op_t * eses_page_op_p, 
                           fbe_u8_t buf_id, 
                           fbe_eses_read_buf_mode_t mode,
                           fbe_u32_t buf_offset,
                           fbe_u8_t * response_buf_p, 
                           fbe_u32_t response_buf_size);
fbe_status_t FBE_API_CALL  fbe_api_enclosure_build_trace_buf_cmd(fbe_enclosure_mgmt_ctrl_op_t * eses_page_op_p, 
                                    fbe_u8_t buf_id,  
                                    fbe_enclosure_mgmt_trace_buf_op_t buf_op,
                                    fbe_u8_t * response_buf_p, 
                                    fbe_u32_t response_buf_size);
fbe_status_t FBE_API_CALL  fbe_api_enclosure_build_slot_to_phy_mapping_cmd(fbe_enclosure_mgmt_ctrl_op_t * ctrl_op_p, 
                                    fbe_enclosure_slot_number_t slot_num_start, 
                                    fbe_enclosure_slot_number_t slot_num_end,
                                    fbe_u8_t * response_buf_p, 
                                    fbe_u32_t response_buf_size);
fbe_status_t FBE_API_CALL  fbe_api_enclosure_get_slot_to_phy_mapping(fbe_object_id_t object_id, 
                                                     fbe_enclosure_mgmt_ctrl_op_t *ctrl_op,
                                                     fbe_enclosure_status_t *enclosure_status);
fbe_status_t FBE_API_CALL  fbe_api_enclosure_get_fault_info(fbe_object_id_t object_id, 
                                              fbe_enclosure_fault_t *enclosure_fault_info);
fbe_status_t FBE_API_CALL  fbe_api_enclosure_get_statistics(fbe_object_id_t object_id, 
                                           fbe_enclosure_mgmt_ctrl_op_t *operation,
                                           fbe_enclosure_status_t *enclosure_status);
fbe_status_t FBE_API_CALL  fbe_api_enclosure_build_statistics_cmd(fbe_enclosure_mgmt_ctrl_op_t * op_p,
                                                    fbe_enclosure_element_type_t element_type,
                                                    fbe_enclosure_slot_number_t start_slot,
                                                    fbe_enclosure_slot_number_t end_slot,
                                                    fbe_u8_t * response_bufp,
                                                    fbe_u32_t response_buf_size);
fbe_status_t FBE_API_CALL  fbe_api_enclosure_get_threshold_in(fbe_object_id_t object_id, 
                                           fbe_enclosure_mgmt_ctrl_op_t *operation,
                                           fbe_enclosure_status_t *enclosure_status);
                                           
fbe_status_t FBE_API_CALL  fbe_api_enclosure_build_threshold_in_cmd(fbe_enclosure_mgmt_ctrl_op_t * op_p,
                                                    fbe_enclosure_component_types_t componentType,
                                                    fbe_u8_t * response_bufp,
                                                    fbe_u32_t response_buf_size);
    
fbe_status_t FBE_API_CALL  fbe_api_enclosure_get_scsi_error_info(fbe_object_id_t object_id, 
                                              fbe_enclosure_scsi_error_info_t *enclosure_fault_info);

fbe_status_t FBE_API_CALL  fbe_api_get_enclosure_object_id_by_location(fbe_u32_t port, fbe_u32_t enclosure, fbe_object_id_t *object_id);
fbe_status_t FBE_API_CALL  fbe_api_get_enclosure_object_ids_array_by_location(fbe_u32_t port, fbe_u32_t enclosure, 
                                                                                                                                           fbe_topology_control_get_enclosure_by_location_t *enclosure_by_location);

fbe_status_t FBE_API_CALL fbe_api_enclosure_getPsInfo(fbe_object_id_t object_id, 
                                                      fbe_power_supply_info_t *enclosurePsInfo);

fbe_status_t FBE_API_CALL fbe_api_enclosure_get_ps_count(fbe_object_id_t objectId,
                                                         fbe_u32_t * pPsCount);
fbe_status_t FBE_API_CALL fbe_api_enclosure_get_slot_count(fbe_object_id_t objectId,
                                                         fbe_u32_t * pSlotCount);
fbe_status_t FBE_API_CALL fbe_api_enclosure_get_slot_count_per_bank(fbe_object_id_t objectId,
                                                         fbe_u32_t * pSlotCountPerBank);
fbe_status_t FBE_API_CALL fbe_api_enclosure_parse_image_header(fbe_object_id_t object_id, 
                                                      fbe_enclosure_mgmt_parse_image_header_t * pParseImageHeader);
fbe_status_t FBE_API_CALL fbe_api_enclosure_get_lcc_info(fbe_device_physical_location_t * pLocation,
                                                         fbe_lcc_info_t * pLccInfo);
fbe_status_t FBE_API_CALL fbe_api_enclosure_get_subencl_lcc_startSlot(fbe_object_id_t objectId, 
                                                         fbe_u32_t  *pSlot);
fbe_status_t FBE_API_CALL fbe_api_enclosure_get_stand_alone_lcc_count(fbe_object_id_t objectId,
                                                         fbe_u32_t * pLccCount);
fbe_status_t FBE_API_CALL fbe_api_enclosure_get_lcc_count(fbe_object_id_t objectId,
                                                         fbe_u32_t * pLccCount);
fbe_status_t FBE_API_CALL fbe_api_enclosure_get_fan_info(fbe_object_id_t object_id, 
                                                        fbe_cooling_info_t *enclosureFanInfo);
fbe_status_t FBE_API_CALL fbe_api_enclosure_get_chassis_cooling_status(fbe_object_id_t object_id, 
                                                        fbe_bool_t *pCoolingFault);
fbe_status_t FBE_API_CALL fbe_api_enclosure_get_fan_count(fbe_object_id_t objectId,
                                                          fbe_u32_t * pFanCount);
fbe_status_t FBE_API_CALL fbe_api_enclosure_get_encl_type(fbe_object_id_t objectId,
                                                          fbe_enclosure_types_t * pEnclType);
fbe_status_t FBE_API_CALL fbe_api_lcc_get_lcc_count(fbe_object_id_t objectId, 
                          fbe_u32_t * pLccCount);

fbe_status_t FBE_API_CALL fbe_api_enclosure_get_drive_slot_info(fbe_object_id_t objectId,
                                           fbe_enclosure_mgmt_get_drive_slot_info_t * getDriveSlotInfo);
fbe_status_t FBE_API_CALL fbe_api_enclosure_get_disk_battery_backed_status(fbe_object_id_t object_id, 
                                                         fbe_bool_t * diskBatteryBackedSet);
fbe_status_t FBE_API_CALL fbe_api_enclosure_set_enclosure_failed(fbe_object_id_t object_id, 
                                                         fbe_base_enclosure_fail_encl_cmd_t *fail_cmd);

fbe_resume_prom_status_t 
fbe_api_enclosure_get_resume_status(fbe_status_t status,
                                    fbe_payload_control_status_t control_status,
                                    fbe_payload_control_status_qualifier_t control_status_qualifier);
fbe_status_t FBE_API_CALL fbe_api_enclosure_get_shutdown_info(fbe_device_physical_location_t * pLocation,
                                                              fbe_enclosure_mgmt_get_shutdown_info_t * getShutdownInfo);
fbe_status_t FBE_API_CALL fbe_api_enclosure_get_overtemp_info(fbe_device_physical_location_t * pLocation,
                                                              fbe_enclosure_mgmt_get_overtemp_info_t * getOverTempInfo);

fbe_status_t FBE_API_CALL fbe_api_enclosure_get_connector_count(fbe_object_id_t objectId,
                                                         fbe_u32_t * pConnectorCount);
fbe_status_t FBE_API_CALL fbe_api_enclosure_get_connector_info(fbe_device_physical_location_t * pLocation,
                                                         fbe_connector_info_t * pConnectorInfo);
fbe_status_t FBE_API_CALL fbe_api_enclosure_get_sps_info(fbe_object_id_t objectId,
                                                         fbe_sps_get_sps_status_t * getSpsInfo);
fbe_status_t FBE_API_CALL fbe_api_enclosure_get_sps_manuf_info(fbe_object_id_t objectId,
                                                               fbe_sps_get_sps_manuf_info_t *getSpsManufInfo);
fbe_status_t FBE_API_CALL fbe_api_enclosure_send_sps_command(fbe_object_id_t objectId,
                                                             fbe_sps_action_type_t spsCommand);

fbe_status_t FBE_API_CALL fbe_api_enclosure_get_sps_count(fbe_object_id_t objectId,
                                                          fbe_u32_t * pSpsCount);

fbe_status_t FBE_API_CALL fbe_api_enclosure_get_battery_count(fbe_object_id_t objectId,
                                                              fbe_u32_t * pBatteryCount);

fbe_status_t FBE_API_CALL fbe_api_enclosure_get_encl_fault_led_info(fbe_object_id_t objectId,
                                                          fbe_encl_fault_led_info_t * pEnclFaultLedInfo);

fbe_status_t FBE_API_CALL fbe_api_enclosure_get_encl_drive_slot_led_info(fbe_object_id_t objectId,
                                                                         fbe_led_status_t *pEnclDriveFaultLeds);

fbe_status_t FBE_API_CALL fbe_api_enclosure_get_encl_side(fbe_object_id_t objectId,
                                                          fbe_u32_t * pEnclSide);

fbe_status_t FBE_API_CALL fbe_api_enclosure_get_number_of_drive_midplane(fbe_object_id_t objectId,
                                                          fbe_u8_t * pNumDriveMidplane);

fbe_status_t FBE_API_CALL fbe_api_enclosure_get_display_info(fbe_object_id_t objectId,
                                                          fbe_encl_display_info_t * pEnclDisplayInfo);

fbe_status_t FBE_API_CALL fbe_api_enclosure_get_disk_parent_object_id(fbe_u32_t bus, 
                                                                      fbe_u32_t enclosure, 
                                                                      fbe_u32_t slot,
                                                                      fbe_object_id_t *pParentObjId);
                                                                      
fbe_status_t FBE_API_CALL fbe_api_enclosure_get_ssc_count(fbe_object_id_t objectId,
                                                          fbe_u8_t * pSscCount);
                                                          
fbe_status_t FBE_API_CALL fbe_api_enclosure_get_ssc_info(fbe_object_id_t objectId,
                                                         fbe_ssc_info_t * pSscInfo);
/*! @} */ /* end of group fbe_api_enclosure_interface */

FBE_API_CPP_EXPORT_END

//----------------------------------------------------------------
// Define the group for all FBE Physical Package APIs Interface class files.  
// This is where all the class files that belong to the FBE API Physical 
// Package define. In addition, this group is displayed in the FBE Classes
// module.
//----------------------------------------------------------------
/*! @defgroup fbe_api_physical_package_interface_class_files FBE Physical Package APIs Interface Class Files 
 *  @brief 
 *    This is the set of files for the FBE Physical Package Interface.
 *
 *  @ingroup fbe_api_classes
 */
//----------------------------------------------------------------

#endif /*FBE_API_ENCLOSURE_INTERFACE_H*/

