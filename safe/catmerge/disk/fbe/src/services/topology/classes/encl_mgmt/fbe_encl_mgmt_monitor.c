/***************************************************************************
 * Copyright (C) EMC Corporation 2011 
 * All rights reserved. Licensed material -- property of EMC 
 * Corporation 
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_encl_mgmt_monitor.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the Enclosure Management object lifecycle
 *  code.
 * 
 *  This includes the
 *  @ref fbe_encl_mgmt_monitor_entry "encl_mgmt monitor entry
 *  point", as well as all the lifecycle defines such as
 *  rotaries and conditions, along with the actual condition
 *  functions.
 * 
 * @ingroup encl_mgmt_class_files
 * 
 * @version
 *   23-Feb-2010:  Created. Ashok Tamilarasan
 *
 ***************************************************************************/
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_memory.h"
#include "fbe_topology.h"
#include "fbe_base_object.h"
#include "fbe_scheduler.h"
#include "fbe_encl_mgmt_private.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_api_power_supply_interface.h"
#include "fbe/fbe_api_physical_drive_interface.h"
#include "fbe_transport_memory.h"
#include "fbe/fbe_api_enclosure_interface.h"
#include "fbe/fbe_event_log_api.h"
#include "fbe_base_environment_debug.h"
#include "fbe/fbe_api_board_interface.h"
#include "fbe/fbe_eir_info.h"
#include "fbe/fbe_esp_encl_mgmt.h"
#include "fbe_encl_mgmt_utils.h"
#include "EspMessages.h"
#include "speeds.h"
#include "HardwareAttributesLib.h"
#include "fbe/fbe_eses.h"

/*!***************************************************************
 * fbe_encl_mgmt_monitor_entry()
 ****************************************************************
 * @brief
 *  Entry routine for the Enclosure Management monitor.
 *
 * @param object_handle - This is the object handle, or in our
 * case the encl_mgmt.
 * @param packet_p - The packet arriving from the monitor
 * scheduler.
 *
 * @return fbe_status_t - Status of monitor operation.             
 *
 * @author
 *  23-Feb-2010:  Created. Ashok Tamilarasan
 *
 ****************************************************************/
fbe_status_t 
fbe_encl_mgmt_monitor_entry(fbe_object_handle_t object_handle, fbe_packet_t * packet)
{
    fbe_encl_mgmt_t * encl_mgmt = NULL;

    encl_mgmt = (fbe_encl_mgmt_t *)fbe_base_handle_to_pointer(object_handle);

    fbe_topology_class_trace(FBE_CLASS_ID_ENCL_MGMT,
                             FBE_TRACE_LEVEL_DEBUG_HIGH, 
                             FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                             "%s entry\n", __FUNCTION__);

    return fbe_lifecycle_crank_object(&FBE_LIFECYCLE_CONST_DATA(encl_mgmt),
                                     (fbe_base_object_t*)encl_mgmt, packet);
}
/******************************************
 * end fbe_encl_mgmt_monitor_entry()
 ******************************************/

/*!**************************************************************
 * fbe_encl_mgmt_monitor_load_verify()
 ****************************************************************
 * @brief
 *  This function simply validates the constant lifecycle data
 *  that is associated with the enclosure management.
 *
 * @param  - None.               
 *
 * @return fbe_status_t - The status of the call to
 *                        fbe_lifecycle_class_const_verify()
 *                        for the enclosure management's
 *                        constant lifecycle data.
 *
 * @author
 *  23-Feb-2010:  Created. Ashok Tamilarasan
 *
 ****************************************************************/
fbe_status_t fbe_encl_mgmt_monitor_load_verify(void)
{
    return fbe_lifecycle_class_const_verify(&FBE_LIFECYCLE_CONST_DATA(encl_mgmt));
}
/******************************************
 * end fbe_encl_mgmt_monitor_load_verify()
 ******************************************/

/*--- local function prototypes --------------------------------------------------------*/

static fbe_lifecycle_status_t fbe_encl_mgmt_register_state_change_callback_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet);
static fbe_lifecycle_status_t fbe_encl_mgmt_register_cmi_callback_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet);
static fbe_lifecycle_status_t fbe_encl_mgmt_fup_register_callback_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet);
static fbe_lifecycle_status_t fbe_encl_mgmt_resume_prom_register_callback_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet);
static fbe_lifecycle_status_t fbe_encl_mgmt_discover_enclosure_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet);
static fbe_lifecycle_status_t fbe_encl_mgmt_get_eir_data_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet);
static fbe_lifecycle_status_t fbe_encl_mgmt_check_encl_shutdown_delay_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet);
static fbe_lifecycle_status_t fbe_encl_mgmt_reset_encl_shutdown_timer_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet);
static fbe_lifecycle_status_t fbe_encl_mgmt_process_cache_status_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet);
static fbe_status_t fbe_encl_mgmt_handle_new_encl(fbe_encl_mgmt_t *encl_mgmt, fbe_object_id_t object_id);
static fbe_status_t fbe_encl_mgmt_handle_new_subencl(fbe_encl_mgmt_t *encl_mgmt, fbe_object_id_t subEnclObjectId);
static fbe_status_t fbe_encl_mgmt_add_new_encl(fbe_encl_mgmt_t * pEnclMgmt,
                                        fbe_object_id_t object_id,
                                        fbe_device_physical_location_t * pLocation,
                                        fbe_encl_info_t ** pEnclInfoPtr);
static fbe_status_t fbe_encl_mgmt_new_encl_check_basic_fault(fbe_encl_mgmt_t * pEnclMgmt,
                                                       fbe_object_id_t objectId,
                                                       fbe_encl_info_t * pEnclInfo);
static fbe_status_t fbe_encl_mgmt_new_encl_check_other_fault(fbe_encl_mgmt_t * pEnclMgmt,
                                                       fbe_object_id_t objectId,
                                                       fbe_encl_info_t * pEnclInfo);
static fbe_status_t fbe_encl_mgmt_new_encl_handle_other_fault(fbe_encl_mgmt_t * encl_mgmt,
                                                    fbe_object_id_t objectId,
                                                    fbe_encl_info_t * pEnclInfo);
static fbe_status_t fbe_encl_mgmt_new_encl_populate_attr(fbe_encl_mgmt_t * pEnclMgmt, 
                                                             fbe_object_id_t objectId,
                                                             fbe_encl_info_t * pEnclInfo);
static fbe_status_t fbe_encl_mgmt_state_change_handler(update_object_msg_t *notification_info, void *context);
static fbe_status_t fbe_encl_mgmt_cmi_message_handler(fbe_base_object_t *base_object, fbe_base_environment_cmi_message_info_t *cmi_message);
static fbe_status_t fbe_encl_mgmt_handle_encl_ready_state(fbe_encl_mgmt_t *encl_mgmt, update_object_msg_t *update_object_msg);
static fbe_status_t fbe_encl_mgmt_handle_subencl_ready_state(fbe_encl_mgmt_t *encl_mgmt, 
                                                     update_object_msg_t *update_object_msg);
static fbe_status_t fbe_encl_mgmt_handle_encl_fail_state(fbe_encl_mgmt_t * pEnclMgmt, 
                                                     update_object_msg_t *update_object_msg);
static fbe_status_t fbe_encl_mgmt_handle_subencl_fail_state(fbe_encl_mgmt_t * pEnclMgmt, 
                                                     update_object_msg_t *update_object_msg);
static fbe_status_t fbe_encl_mgmt_handle_encl_destroy_state(fbe_encl_mgmt_t *encl_mgmt, update_object_msg_t *update_object_msg);
static fbe_status_t fbe_encl_mgmt_handle_subencl_destroy_state(fbe_encl_mgmt_t *encl_mgmt, update_object_msg_t *update_object_msg);
static fbe_status_t fbe_encl_mgmt_get_port_encl_number(fbe_object_id_t object_id,fbe_port_number_t *port_number,
                                                       fbe_enclosure_number_t *enclosure_number);
static fbe_status_t fbe_encl_mgmt_get_encl_info_by_object_id(fbe_encl_mgmt_t *encl_mgmt, fbe_object_id_t object_id,
                                                             fbe_encl_info_t **encl_info);
static fbe_status_t fbe_encl_mgmt_get_subencl_info_by_subencl_object_id(fbe_encl_mgmt_t * pEnclMgmt,
                                                             fbe_object_id_t subEnclObjectId,
                                                             fbe_sub_encl_info_t ** pSubEnclInfoPtr);
static fbe_status_t fbe_encl_mgmt_handle_object_data_change(fbe_encl_mgmt_t * pEnclMgmt, 
                                                          update_object_msg_t * pUpdateObjectMsg);
static fbe_status_t fbe_encl_mgmt_handle_generic_info_change(fbe_encl_mgmt_t * pEnclMgmt, 
                                                          update_object_msg_t * pUpdateObjectMsg);
static fbe_status_t 
fbe_encl_mgmt_process_enclosure_status(fbe_encl_mgmt_t * pEnclMgmt,
                                       fbe_object_id_t objectId,
                                       fbe_device_physical_location_t * pLocation);

static fbe_status_t 
fbe_encl_mgmt_process_encl_fault_led_status(fbe_encl_mgmt_t * pEnclMgmt,
                                       fbe_object_id_t objectId,
                                       fbe_device_physical_location_t * pLocation);

static fbe_status_t fbe_encl_mgmt_get_encl_type(fbe_encl_mgmt_t * pEnclMgmt,
                                                fbe_object_id_t objectId,
                                                fbe_esp_encl_type_t * pEnclType);
static fbe_status_t 
fbe_encl_mgmt_derive_encl_attr(fbe_encl_mgmt_t * pEnclMgmt,
                               fbe_encl_info_t * pEnclInfo);

void fbe_encl_mgmt_processEnclEirData(fbe_encl_mgmt_t *pEnclMgmt,
                                      fbe_device_physical_location_t *enclLocation,
                                      fbe_eir_input_power_sample_t *inputPower,
                                      fbe_eir_temp_sample_t *airInletTemp);
void fbe_encl_mgmt_clearAirInletTempSample(fbe_eir_temp_sample_t *samplePtr);
static fbe_status_t fbe_encl_mgmt_processReceivedCmiMessage(fbe_encl_mgmt_t * pPsMgmt,
                                                          fbe_encl_mgmt_cmi_packet_t *pEnclCmiPkt);
static fbe_status_t fbe_encl_mgmt_processPeerNotPresent(fbe_encl_mgmt_t * pPsMgmt, 
                              fbe_encl_mgmt_cmi_packet_t * pEnclCmiPkt);
static fbe_status_t fbe_encl_mgmt_processPeerBusy(fbe_encl_mgmt_t * pPsMgmt, 
                              fbe_encl_mgmt_cmi_packet_t * pEnclCmiPkt);
static fbe_status_t fbe_encl_mgmt_processContactLost(fbe_encl_mgmt_t * pPsMgmt);
static fbe_status_t fbe_encl_mgmt_processFatalError(fbe_encl_mgmt_t * pPsMgmt, 
                              fbe_encl_mgmt_cmi_packet_t * pEnclCmiPkt);

static fbe_status_t 
fbe_encl_mgmt_process_connector_status(fbe_encl_mgmt_t * pEnclMgmt,
                               fbe_object_id_t objectId,
                               fbe_device_physical_location_t * pLocation);

static fbe_status_t 
fbe_encl_mgmt_check_connector_events(fbe_encl_mgmt_t * pEnclMgmt,
                             fbe_device_physical_location_t * pLocation,
                             fbe_connector_info_t * pNewConnectorInfo,
                             fbe_connector_info_t * pOldConnectorInfo);

static fbe_status_t fbe_encl_mgmt_process_subencl_lcc_status(fbe_encl_mgmt_t * pEnclMgmt,
                                                             fbe_object_id_t subEnclObjectId,
                                                             fbe_device_physical_location_t * psubEnclLccLocation);

static fbe_status_t 
fbe_encl_mgmt_check_subencl_lcc_fault(fbe_encl_mgmt_t * pEnclMgmt,
                                     fbe_device_physical_location_t * pSubEnclLccLocation);

static fbe_status_t fbe_encl_mgmt_log_lcc_status_change(fbe_encl_mgmt_t * pEnclMgmt,
                                                 fbe_device_physical_location_t * pLocation,
                                                 fbe_lcc_info_t *pNewLccInfo,
                                                 fbe_lcc_info_t *pOldLccInfo);

static fbe_status_t 
fbe_encl_mgmt_check_overlimit_enclosures(fbe_base_object_t * base_object);

static fbe_bool_t 
fbe_encl_mgmt_get_lcc_logical_fault(fbe_lcc_info_t *pLccInfo);

static fbe_bool_t 
fbe_encl_mgmt_get_connector_logical_fault(fbe_connector_info_t *pConnectorInfo);

static fbe_status_t
fbe_encl_mgmt_set_enclosure_display(fbe_encl_mgmt_t * pEnclMgmt,
                                       fbe_object_id_t objectId,
                                       fbe_device_physical_location_t * pLocation);
static fbe_status_t 
fbe_encl_mgmt_process_display_status(fbe_encl_mgmt_t * pEnclMgmt,
                                       fbe_object_id_t objectId,
                                       fbe_device_physical_location_t * pLocation);

static fbe_status_t 
fbe_encl_mgmt_find_available_subencl_info(fbe_encl_mgmt_t * pEnclMgmt, 
                                          fbe_encl_info_t * pEnclInfo,
                                          fbe_sub_encl_info_t ** pSubEnclInfoPtr);

static fbe_status_t
fbe_encl_mgmt_process_slot_led_status(fbe_encl_mgmt_t * pEnclMgmt,
                                       fbe_object_id_t objectId,
                                       fbe_device_physical_location_t * pLocation);

static fbe_status_t 
fbe_encl_mgmt_encl_log_event(fbe_encl_mgmt_t * pEnclMgmt,
                                fbe_device_physical_location_t * pLocation,
                                fbe_encl_info_t * pNewEnclInfo,
                                fbe_encl_info_t * pOldEnclInfo);
static fbe_u32_t fbe_encl_mgmt_getEnclBbuCount(fbe_encl_mgmt_t *pEnclMgmt,
                                               fbe_device_physical_location_t *location);
static fbe_u32_t fbe_encl_mgmt_getEnclSpsCount(fbe_encl_mgmt_t *pEnclMgmt,
                                               fbe_device_physical_location_t *location);

static fbe_status_t 
fbe_encl_mgmt_check_system_id_info_mismatch(fbe_encl_mgmt_system_id_info_t * pSystemIdInfoFromPersistentStorage,
                                            RESUME_PROM_STRUCTURE * pResumeProm,
                                            fbe_bool_t * pMismatch);

static fbe_lifecycle_status_t
fbe_encl_mgmt_read_array_midplane_resume_prom_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet);

fbe_status_t fbe_encl_mgmt_process_user_modified_wwn_seed_flag_change(fbe_encl_mgmt_t * pEnclMgmt,
                                                                      fbe_encl_mgmt_cmi_packet_t *pEnclCmiPkt);

static fbe_lifecycle_status_t
fbe_encl_mgmt_check_system_id_info_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet);


fbe_status_t fbe_encl_mgmt_write_sys_info_to_persistent(fbe_esp_encl_mgmt_modify_system_id_info_t);

static fbe_lifecycle_status_t fbe_encl_mgmt_read_persistent_data_complete_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet);

static fbe_lifecycle_status_t
fbe_encl_mgmt_write_array_midplane_resume_prom_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet);

static fbe_status_t fbe_encl_mgmt_process_peer_cache_status_update(fbe_encl_mgmt_t * pEnclMgmt, 
                                                                   fbe_encl_mgmt_cmi_packet_t * cmiMsgPtr);
static fbe_lifecycle_status_t 
fbe_encl_mgmt_shutdown_debounce_timer_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet);
static fbe_status_t fbe_encl_mgmt_check_fru_limit(fbe_encl_mgmt_t *encl_mgmt, fbe_device_physical_location_t *location);

static fbe_status_t fbe_encl_mgmt_suppress_lcc_fault_if_needed(fbe_encl_mgmt_t * pEnclMgmt,
                                           fbe_device_physical_location_t * pLocation,
                                           fbe_lcc_info_t * pLccInfo);

static fbe_status_t fbe_encl_mgmt_process_ssc_status(fbe_encl_mgmt_t *pEnclMgmt,
                                                     fbe_object_id_t objectId,
                                                     fbe_device_physical_location_t *pLocation);
static fbe_bool_t fbe_encl_mgmt_validEnclPsTypes(fbe_encl_mgmt_t *pEnclMgmt, fbe_encl_info_t *pEnclInfo);

/*--- lifecycle callback functions -----------------------------------------------------*/


FBE_LIFECYCLE_DEF_CALLBACK_GET_INST_DATA(encl_mgmt);
FBE_LIFECYCLE_DEF_CALLBACK_GET_INST_COND(encl_mgmt);

/*  encl_mgmt_lifecycle_callbacks This variable has all the
 *  callbacks the lifecycle needs.
 */
static fbe_lifecycle_const_callback_t FBE_LIFECYCLE_CALLBACKS(encl_mgmt) = {
    FBE_LIFECYCLE_DEF_CONST_CALLBACKS(
        encl_mgmt,
        FBE_LIFECYCLE_NULL_FUNC,       /* online function */
        FBE_LIFECYCLE_NULL_FUNC)         /* no pending function */
};


/*--- constant derived condition entries -----------------------------------------------*/
/* FBE_BASE_ENV_LIFECYCLE_COND_REGISTER_STATE_CHANGE_CALLBACK condition
 *  - The purpose of this derived condition is to register
 *  notification with the base environment object which inturns
 *  registers with the FBE API. The leaf class needs to
 *  implement the actual condition handler function providing
 *  the callback that needs to be called
 */
static fbe_lifecycle_const_cond_t fbe_encl_mgmt_register_state_change_callback_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_ENV_LIFECYCLE_COND_REGISTER_STATE_CHANGE_CALLBACK,
        fbe_encl_mgmt_register_state_change_callback_cond_function)
};

/*--- constant derived condition entries -----------------------------------------------*/
/* FBE_BASE_ENV_LIFECYCLE_COND_READ_PERSISTENT_DATA_COMPLETE condition
 *  - The purpose of this derived condition is to handle the completion
 *    of reading the data from persistent storage.
 */
static fbe_lifecycle_const_cond_t fbe_encl_mgmt_read_persistent_data_complete_cond = {
FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
FBE_BASE_ENV_LIFECYCLE_COND_READ_PERSISTENT_DATA_COMPLETE,
fbe_encl_mgmt_read_persistent_data_complete_cond_function)
};

/* FBE_BASE_ENV_LIFECYCLE_COND_REGISTER_CMI_CALLBACK condition
 *  - The purpose of this derived condition is to register
 *  notification with the base environment object which inturns
 *  registers with the CMI. The leaf class needs to implement
 *  the actual condition handler function providing the callback
 *  that needs to be called
 */
static fbe_lifecycle_const_cond_t fbe_encl_mgmt_register_cmi_callback_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_ENV_LIFECYCLE_COND_REGISTER_CMI_CALLBACK,
        fbe_encl_mgmt_register_cmi_callback_cond_function)
};

/* FBE_BASE_ENV_LIFECYCLE_COND_FUP_REGISTER_CALLBACK condition
 *  - The purpose of this derived condition is to register
 *  call back functions with the base environment object. 
 * The leaf class needs to implement the actual call back functions.
 */
static fbe_lifecycle_const_cond_t fbe_encl_mgmt_fup_register_callback_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_ENV_LIFECYCLE_COND_FUP_REGISTER_CALLBACK,
        fbe_encl_mgmt_fup_register_callback_cond_function)
};

static fbe_lifecycle_const_cond_t fbe_encl_mgmt_fup_abort_upgrade_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_ENV_LIFECYCLE_COND_FUP_ABORT_UPGRADE,
        fbe_base_env_fup_abort_upgrade_cond_function)
};

static fbe_lifecycle_const_cond_t fbe_encl_mgmt_fup_wait_before_upgrade_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_ENV_LIFECYCLE_COND_FUP_WAIT_BEFORE_UPGRADE,
        fbe_base_env_fup_wait_before_upgrade_cond_function)
};

static fbe_lifecycle_const_cond_t fbe_encl_mgmt_fup_wait_for_inter_device_delay_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_ENV_LIFECYCLE_COND_FUP_WAIT_FOR_INTER_DEVICE_DELAY,
        fbe_base_env_fup_wait_for_inter_device_delay_cond_function)
};

static fbe_lifecycle_const_cond_t fbe_encl_mgmt_fup_read_image_header_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_ENV_LIFECYCLE_COND_FUP_READ_IMAGE_HEADER,
        fbe_base_env_fup_read_image_header_cond_function)
};

static fbe_lifecycle_const_cond_t fbe_encl_mgmt_fup_check_rev_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_ENV_LIFECYCLE_COND_FUP_CHECK_REV,
        fbe_base_env_fup_check_rev_cond_function)
};

static fbe_lifecycle_const_cond_t fbe_encl_mgmt_fup_read_entire_image_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_ENV_LIFECYCLE_COND_FUP_READ_ENTIRE_IMAGE,
        fbe_base_env_fup_read_entire_image_cond_function)
};

static fbe_lifecycle_const_cond_t fbe_encl_mgmt_fup_download_image_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_ENV_LIFECYCLE_COND_FUP_DOWNLOAD_IMAGE,
        fbe_base_env_fup_download_image_cond_function)
};

static fbe_lifecycle_const_cond_t fbe_encl_mgmt_fup_get_download_status_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_ENV_LIFECYCLE_COND_FUP_GET_DOWNLOAD_STATUS,
        fbe_base_env_fup_get_download_status_cond_function)
};

static fbe_lifecycle_const_cond_t fbe_encl_mgmt_fup_get_peer_permission_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_ENV_LIFECYCLE_COND_FUP_GET_PEER_PERMISSION,
        fbe_base_env_fup_get_peer_permission_cond_function)
};

static fbe_lifecycle_const_cond_t fbe_encl_mgmt_fup_check_env_status_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_ENV_LIFECYCLE_COND_FUP_CHECK_ENV_STATUS,
        fbe_base_env_fup_check_env_status_cond_function)
};

static fbe_lifecycle_const_cond_t fbe_encl_mgmt_fup_activate_image_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_ENV_LIFECYCLE_COND_FUP_ACTIVATE_IMAGE,
        fbe_base_env_fup_activate_image_cond_function)
};

static fbe_lifecycle_const_cond_t fbe_encl_mgmt_fup_get_activate_status_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_ENV_LIFECYCLE_COND_FUP_GET_ACTIVATE_STATUS,
        fbe_base_env_fup_get_activate_status_cond_function)
};

static fbe_lifecycle_const_cond_t fbe_encl_mgmt_fup_check_result_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_ENV_LIFECYCLE_COND_FUP_CHECK_RESULT,
        fbe_base_env_fup_check_result_cond_function)
};

static fbe_lifecycle_const_cond_t fbe_encl_mgmt_fup_refresh_device_status_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_ENV_LIFECYCLE_COND_FUP_REFRESH_DEVICE_STATUS,
        fbe_base_env_fup_refresh_device_status_cond_function)
};

static fbe_lifecycle_const_cond_t fbe_encl_mgmt_fup_end_upgrade_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_ENV_LIFECYCLE_COND_FUP_END_UPGRADE,
        fbe_base_env_fup_end_upgrade_cond_function)
};

static fbe_lifecycle_const_cond_t fbe_encl_mgmt_fup_release_image_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_ENV_LIFECYCLE_COND_FUP_RELEASE_IMAGE,
        fbe_base_env_fup_release_image_cond_function)
};

static fbe_lifecycle_const_cond_t fbe_encl_mgmt_resume_prom_register_callback_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_ENV_LIFECYCLE_COND_RESUME_PROM_REGISTER_CALLBACK,
        fbe_encl_mgmt_resume_prom_register_callback_cond_function)
};

static fbe_lifecycle_const_cond_t fbe_encl_mgmt_update_array_midplane_resume_prom_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_ENV_LIFECYCLE_COND_UPDATE_RESUME_PROM,
        fbe_base_env_update_resume_prom_cond_function)
};

/*--- constant base condition entries --------------------------------------------------*/

/* FBE_ENCL_MGMT_DISCOVER_ENCLOSURE condition -
*   The purpose of this condition is start the discovery process
*   of a new enclosure that was added
 */
static fbe_lifecycle_const_base_cond_t 
    fbe_encl_mgmt_discover_enclosure_cond = {
        FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "fbe_encl_mgmt_discover_enclosure_cond",
        FBE_ENCL_MGMT_DISCOVER_ENCLOSURE,
        fbe_encl_mgmt_discover_enclosure_cond_function),
        FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
            FBE_LIFECYCLE_STATE_SPECIALIZE,     /* SPECIALIZE */
            FBE_LIFECYCLE_STATE_ACTIVATE,         /* ACTIVATE */
            FBE_LIFECYCLE_STATE_READY,         /* READY */
            FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
            FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
            FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
            FBE_LIFECYCLE_STATE_DESTROY)         /* DESTROY */
};


/* FBE_ENCL_MGMT_EIR_GET_DATA_COND condition 
 *   This condition drives the requesting of environmental data from the enclosure objects every
 *   30 seconds.  
 */
static fbe_lifecycle_const_base_timer_cond_t fbe_encl_mgmt_get_eir_data_cond = {
    {
        FBE_LIFECYCLE_DEF_CONST_BASE_TIMER_COND(
            "fbe_encl_mgmt_get_eir_data_cond",
            FBE_ENCL_MGMT_GET_EIR_DATA_COND,
            fbe_encl_mgmt_get_eir_data_cond_function),
        FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
            FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
            FBE_LIFECYCLE_STATE_ACTIVATE,         /* ACTIVATE */
            FBE_LIFECYCLE_STATE_READY,          /* READY */
            FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
            FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
            FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
            FBE_LIFECYCLE_STATE_DESTROY)         /* DESTROY */
    },
    FBE_LIFECYCLE_CANARY_CONST_BASE_TIMER_COND,
    3000  /* fires every 30 seconds */
};

/* FBE_ENCL_MGMT_SHUTDOWN_DEBOUNCE_TIMER_COND condition 
 *   This condition is used to debounce the shutdown reason. The timer gets fired every 5 seconds
 */
static fbe_lifecycle_const_base_timer_cond_t fbe_encl_mgmt_shutdown_debounce_timer_cond = {
    {
        FBE_LIFECYCLE_DEF_CONST_BASE_TIMER_COND(
            "fbe_encl_mgmt_shutdown_debounce_timer_cond",
            FBE_ENCL_MGMT_SHUTDOWN_DEBOUNCE_TIMER_COND,
            fbe_encl_mgmt_shutdown_debounce_timer_cond_function),
        FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
            FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
            FBE_LIFECYCLE_STATE_ACTIVATE,         /* ACTIVATE */
            FBE_LIFECYCLE_STATE_READY,          /* READY */
            FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
            FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
            FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
            FBE_LIFECYCLE_STATE_DESTROY)         /* DESTROY */
    },
    FBE_LIFECYCLE_CANARY_CONST_BASE_TIMER_COND,
    500  /* fires every 5 seconds */
};

/* FBE_ENCL_MGMT_CHECK_ENCL_SHUTDOWN_DELAY_COND condition -
*   The purpose of this condition is check whether we should reset the enclosure shutdown timer. 
 */
static fbe_lifecycle_const_base_cond_t 
    fbe_encl_mgmt_check_encl_shutdown_delay_cond = {
        FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "fbe_encl_mgmt_check_encl_shutdown_delay_cond",
        FBE_ENCL_MGMT_CHECK_ENCL_SHUTDOWN_DELAY_COND,
        fbe_encl_mgmt_check_encl_shutdown_delay_cond_function),
        FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
            FBE_LIFECYCLE_STATE_SPECIALIZE,     /* SPECIALIZE */
            FBE_LIFECYCLE_STATE_ACTIVATE,         /* ACTIVATE */
            FBE_LIFECYCLE_STATE_READY,         /* READY */
            FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
            FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
            FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
            FBE_LIFECYCLE_STATE_DESTROY)         /* DESTROY */
};

/*   FBE_ENCL_MGMT_RESET_ENCL_SHUTDOWN_TIMER_COND condition 
 *   This condition is used to reset the enclosure shutdown timer after 120 seconds.
 */
static fbe_lifecycle_const_base_timer_cond_t fbe_encl_mgmt_reset_encl_shutdown_timer_cond = {
    {
        FBE_LIFECYCLE_DEF_CONST_BASE_TIMER_COND(
            "fbe_encl_mgmt_reset_encl_shutdown_timer_cond",
            FBE_ENCL_MGMT_RESET_ENCL_SHUTDOWN_TIMER_COND,
            fbe_encl_mgmt_reset_encl_shutdown_timer_cond_function),
        FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
            FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
            FBE_LIFECYCLE_STATE_ACTIVATE,         /* ACTIVATE */
            FBE_LIFECYCLE_STATE_READY,          /* READY */
            FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
            FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
            FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
            FBE_LIFECYCLE_STATE_DESTROY)         /* DESTROY */
    },
    FBE_LIFECYCLE_CANARY_CONST_BASE_TIMER_COND,
    120 * 100  /* fires every 120 seconds */
};

/*   FBE_ENCL_MGMT_PROCESS_CACHE_STATUS_COND condition 
 *   This condition is used to process the enclosure cache status.
 */
static fbe_lifecycle_const_base_cond_t
    fbe_encl_mgmt_process_cache_status_cond = {
        FBE_LIFECYCLE_DEF_CONST_BASE_COND(
            "fbe_encl_mgmt_process_cache_status_cond",
            FBE_ENCL_MGMT_PROCESS_CACHE_STATUS_COND,
            fbe_encl_mgmt_process_cache_status_cond_function),
        FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
            FBE_LIFECYCLE_STATE_SPECIALIZE,     /* SPECIALIZE */
            FBE_LIFECYCLE_STATE_ACTIVATE,         /* ACTIVATE */
            FBE_LIFECYCLE_STATE_READY,         /* READY */
            FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
            FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
            FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
            FBE_LIFECYCLE_STATE_DESTROY)         /* DESTROY */
};

static fbe_lifecycle_const_base_cond_t
    fbe_encl_mgmt_read_array_midplane_resume_prom_cond = {
        FBE_LIFECYCLE_DEF_CONST_BASE_COND(
            "fbe_encl_mgmt_read_array_midplane_resume_prom_cond",
            FBE_ENCL_MGMT_READ_ARRAY_MIDPLANE_RP_COND,
            fbe_encl_mgmt_read_array_midplane_resume_prom_cond_function),
        FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
            FBE_LIFECYCLE_STATE_SPECIALIZE,     /* SPECIALIZE */
            FBE_LIFECYCLE_STATE_ACTIVATE,         /* ACTIVATE */
            FBE_LIFECYCLE_STATE_READY,         /* READY */
            FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
            FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
            FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
            FBE_LIFECYCLE_STATE_DESTROY)         /* DESTROY */
};

static fbe_lifecycle_const_base_cond_t
    fbe_encl_mgmt_check_system_id_info_cond = {
        FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "fbe_encl_mgmt_check_system_id_info_cond",
        FBE_BASE_ENV_LIFECYCLE_COND_CHECK_SYSTEM_ID_INFO,
        fbe_encl_mgmt_check_system_id_info_cond_function),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,         /* ACTIVATE */
        FBE_LIFECYCLE_STATE_READY,          /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)         /* DESTROY */
};

static fbe_lifecycle_const_base_cond_t
    fbe_encl_mgmt_write_array_midplane_resume_prom_cond = {
        FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "fbe_encl_mgmt_write_array_midplane_resume_prom_cond",
        FBE_ENCL_MGMT_WRITE_ARRAY_MIDPLANE_RP_COND,
        fbe_encl_mgmt_write_array_midplane_resume_prom_cond_function),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,     /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,         /* ACTIVATE */
        FBE_LIFECYCLE_STATE_READY,         /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)         /* DESTROY */
};
static fbe_lifecycle_const_base_cond_t * FBE_LIFECYCLE_BASE_COND_ARRAY(encl_mgmt)[] = {
    &fbe_encl_mgmt_discover_enclosure_cond,
    (fbe_lifecycle_const_base_cond_t*)&fbe_encl_mgmt_get_eir_data_cond,
    &fbe_encl_mgmt_check_encl_shutdown_delay_cond,
    (fbe_lifecycle_const_base_cond_t*)&fbe_encl_mgmt_reset_encl_shutdown_timer_cond,
    &fbe_encl_mgmt_process_cache_status_cond,
    &fbe_encl_mgmt_read_array_midplane_resume_prom_cond,
    &fbe_encl_mgmt_check_system_id_info_cond,
    &fbe_encl_mgmt_write_array_midplane_resume_prom_cond,
    (fbe_lifecycle_const_base_cond_t*)&fbe_encl_mgmt_shutdown_debounce_timer_cond,
};

FBE_LIFECYCLE_DEF_CONST_BASE_CONDITIONS(encl_mgmt);

/*--- constant rotaries ----------------------------------------------------------------*/

static fbe_lifecycle_const_rotary_cond_t fbe_encl_mgmt_specialize_rotary[] = {
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_encl_mgmt_register_state_change_callback_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET), 
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_encl_mgmt_register_cmi_callback_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_encl_mgmt_fup_register_callback_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_encl_mgmt_resume_prom_register_callback_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_encl_mgmt_discover_enclosure_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_encl_mgmt_update_array_midplane_resume_prom_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_encl_mgmt_read_array_midplane_resume_prom_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_encl_mgmt_read_persistent_data_complete_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_encl_mgmt_check_system_id_info_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
};

static fbe_lifecycle_const_rotary_cond_t fbe_encl_mgmt_activate_rotary[] = {
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_encl_mgmt_read_persistent_data_complete_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_encl_mgmt_check_system_id_info_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_encl_mgmt_write_array_midplane_resume_prom_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
};

static fbe_lifecycle_const_rotary_cond_t fbe_encl_mgmt_ready_rotary[] = {
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_encl_mgmt_write_array_midplane_resume_prom_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_encl_mgmt_fup_wait_before_upgrade_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_encl_mgmt_check_encl_shutdown_delay_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_encl_mgmt_process_cache_status_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_encl_mgmt_shutdown_debounce_timer_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_encl_mgmt_reset_encl_shutdown_timer_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_encl_mgmt_get_eir_data_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_encl_mgmt_fup_release_image_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),

    /* Moved the following two conditions up. (AR651486)
     * so that work item can be removed from the queue immediately after the device gets removed.
     */
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_encl_mgmt_fup_refresh_device_status_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_encl_mgmt_fup_end_upgrade_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),

    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_encl_mgmt_fup_wait_for_inter_device_delay_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_encl_mgmt_fup_read_image_header_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),  
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_encl_mgmt_fup_check_rev_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_encl_mgmt_fup_read_entire_image_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_encl_mgmt_fup_get_peer_permission_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_encl_mgmt_fup_check_env_status_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_encl_mgmt_fup_download_image_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_encl_mgmt_fup_get_download_status_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_encl_mgmt_fup_activate_image_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_encl_mgmt_fup_get_activate_status_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_encl_mgmt_fup_check_result_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    
    /* Need to put the abort condition in the end of all the fup conditions. 
     * We want to complete the execution of other condition which was already set before running the abort upgrade 
     * condition. Otherwise, when resuming the upgrade, the condition which was set before would get to run and 
     * it would make the upgrade out of sequence.
     */
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_encl_mgmt_fup_abort_upgrade_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL), 
};

static fbe_lifecycle_const_rotary_t FBE_LIFECYCLE_ROTARY_ARRAY(encl_mgmt)[] = {
    FBE_LIFECYCLE_DEF_CONST_ROTARY(FBE_LIFECYCLE_STATE_SPECIALIZE, fbe_encl_mgmt_specialize_rotary),
    FBE_LIFECYCLE_DEF_CONST_ROTARY(FBE_LIFECYCLE_STATE_ACTIVATE, fbe_encl_mgmt_activate_rotary),
    FBE_LIFECYCLE_DEF_CONST_ROTARY(FBE_LIFECYCLE_STATE_READY, fbe_encl_mgmt_ready_rotary)
};

FBE_LIFECYCLE_DEF_CONST_ROTARIES(encl_mgmt);

/*--- global encl mgmt lifecycle constant data ----------------------------------------*/

fbe_lifecycle_const_t FBE_LIFECYCLE_CONST_DATA(encl_mgmt) = {
    FBE_LIFECYCLE_DEF_CONST_DATA(
        encl_mgmt,
        FBE_CLASS_ID_ENCL_MGMT,                  /* This class */
        FBE_LIFECYCLE_CONST_DATA(base_environment))    /* The super class */
};

/*--- Condition Functions --------------------------------------------------------------*/

/*!**************************************************************
 * fbe_encl_mgmt_register_state_change_callback_cond_function()
 ****************************************************************
 * @brief
 *  This function registers with the physical package to get
 *  notified on enclosure object life cycle state changes
 * 
 * @param object_handle - This is the object handle, or in our
 * case the encl_mgmt.
 * @param packet_p - The packet arriving from the monitor
 * scheduler.
 * 
 * @return fbe_status_t - The status of the call to
 *                        fbe_lifecycle_class_const_verify() for
 *                        the enclosure management's constant
 *                        lifecycle data.
 *
 * @author
 *  23-Feb-2010:  Created. Ashok Tamilarasan
 *
 ****************************************************************/
static fbe_lifecycle_status_t
fbe_encl_mgmt_register_state_change_callback_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet)
{
    fbe_status_t status;

    status = fbe_lifecycle_clear_current_cond(base_object);
    if (status != FBE_STATUS_OK) {
        fbe_base_object_trace(base_object, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s can't clear current condition, status: 0x%X",
                              __FUNCTION__, status);
    }

    /* Register with the base environment for the conditions we care
     * about.
     */
    status = fbe_base_environment_register_event_notification((fbe_base_environment_t *) base_object,
                                                              (fbe_api_notification_callback_function_t)fbe_encl_mgmt_state_change_handler,
                                                              (FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_READY|FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_FAIL|FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_DESTROY|FBE_NOTIFICATION_TYPE_OBJECT_DATA_CHANGED),
                                                              (FBE_DEVICE_TYPE_ENCLOSURE|FBE_DEVICE_TYPE_LCC|FBE_DEVICE_TYPE_CONNECTOR|FBE_DEVICE_TYPE_MISC|FBE_DEVICE_TYPE_DISPLAY|FBE_DEVICE_TYPE_DRIVE|FBE_DEVICE_TYPE_SSC),
                                                              base_object,
                                                              FBE_TOPOLOGY_OBJECT_TYPE_ENCLOSURE|FBE_TOPOLOGY_OBJECT_TYPE_LCC|FBE_TOPOLOGY_OBJECT_TYPE_BOARD,
                                                              FBE_PACKAGE_NOTIFICATION_ID_PHYSICAL);
    if (status != FBE_STATUS_OK) {
        fbe_base_object_trace(base_object, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s API callback init failed, status: 0x%X",
                              __FUNCTION__, status);
    }
    return FBE_LIFECYCLE_STATUS_DONE;
}
/*******************************************************************
 * end 
 * fbe_encl_mgmt_register_state_change_callback_cond_function() 
 *******************************************************************/


/*!**************************************************************
 * fbe_encl_mgmt_register_cmi_callback_cond_function()
 ****************************************************************
 * @brief
 *  This function registers with the physical package to get
 *  notified on enclosure object life cycle state changes
 * 
 * @param object_handle - This is the object handle, or in our
 * case the encl_mgmt.
 * @param packet_p - The packet arriving from the monitor
 * scheduler.
 * 
 * @return fbe_status_t - The status of the call to
 *                        fbe_lifecycle_class_const_verify() for
 *                        the enclosure management's constant
 *                        lifecycle data.
 *
 * @author
 *  23-Feb-2010:  Created. Ashok Tamilarasan
 *
 ****************************************************************/
static fbe_lifecycle_status_t
fbe_encl_mgmt_register_cmi_callback_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet)
{
    fbe_status_t status;

    status = fbe_lifecycle_clear_current_cond(base_object);
    if (status != FBE_STATUS_OK) {
        fbe_base_object_trace(base_object, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s can't clear current condition, status: 0x%X",
                              __FUNCTION__, status);
    }
    /* Register with the CMI for the conditions we care about.
     */
    status = fbe_base_environment_register_cmi_notification((fbe_base_environment_t *)base_object,
                                                            FBE_CMI_CLIENT_ID_ENCL_MGMT,
                                                            fbe_encl_mgmt_cmi_message_handler);

    if (status != FBE_STATUS_OK) {
        fbe_base_object_trace(base_object, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s API callback init failed, status: 0x%X",
                              __FUNCTION__, status);
    }
    return FBE_LIFECYCLE_STATUS_DONE;
}
/*******************************************************************
 * end 
 * fbe_encl_mgmt_register_state_change_callback_cond_function() 
 *******************************************************************/

/*!**************************************************************
 * fbe_encl_mgmt_fup_register_callback_cond_function()
 ****************************************************************
 * @brief
 *  This function registers the callback functions with the base environment object.
 * 
 * @param base_object - This is the object handle, or in our
 * case the encl_mgmt.
 * @param packet - The packet arriving from the monitor
 * scheduler.
 * 
 * @return fbe_status_t - The status of the call to
 *                        fbe_lifecycle_class_const_verify() for
 *                        the Encl management's constant
 *                        lifecycle data.
 *
 * @author
 *  10-Aug-2010: PHE - Created. 
 *
 ****************************************************************/
static fbe_lifecycle_status_t
fbe_encl_mgmt_fup_register_callback_cond_function(fbe_base_object_t * base_object, 
                                                 fbe_packet_t * packet)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_base_environment_t * pBaseEnv = (fbe_base_environment_t *)base_object;

    fbe_base_object_trace(base_object, 
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s entry\n",
                          __FUNCTION__);

    fbe_base_env_set_fup_get_full_image_path_callback(pBaseEnv, &fbe_encl_mgmt_fup_get_full_image_path);
    fbe_base_env_set_fup_get_manifest_file_full_path_callback(pBaseEnv, &fbe_encl_mgmt_fup_get_manifest_file_full_path);
    fbe_base_env_set_fup_check_env_status_callback(pBaseEnv, &fbe_encl_mgmt_fup_check_env_status);
    fbe_base_env_set_fup_info_ptr_callback(pBaseEnv, &fbe_encl_mgmt_get_fup_info_ptr);
    fbe_base_env_set_fup_info_callback(pBaseEnv, &fbe_encl_mgmt_get_fup_info);
    fbe_base_env_set_fup_get_firmware_rev_callback(pBaseEnv, &fbe_encl_mgmt_fup_get_firmware_rev);
    fbe_base_env_set_fup_refresh_device_status_callback(pBaseEnv, &fbe_encl_mgmt_fup_refresh_device_status);
    fbe_base_env_set_fup_initiate_upgrade_callback(pBaseEnv, &fbe_encl_mgmt_fup_initiate_upgrade);
    fbe_base_env_set_fup_resume_upgrade_callback(pBaseEnv, &fbe_encl_mgmt_fup_resume_upgrade);
    fbe_base_env_set_fup_priority_check_callback(pBaseEnv, &fbe_encl_mgmt_fup_lcc_priority_check);
    fbe_base_env_set_fup_copy_fw_rev_to_resume_prom(pBaseEnv, (void *)fbe_encl_mgmt_fup_copy_fw_rev_to_resume_prom);

    status = fbe_lifecycle_clear_current_cond(base_object);
    if (status != FBE_STATUS_OK) {
        fbe_base_object_trace(base_object, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "FUP: %s can't clear current condition, status: 0x%X",
                              __FUNCTION__, status);
    }
   
    return FBE_LIFECYCLE_STATUS_DONE;
}
/*******************************************************************
 * end 
 * fbe_encl_mgmt_fup_register_callback_cond_function() 
 *******************************************************************/

/*!**************************************************************
 * fbe_encl_mgmt_resume_prom_register_callback_cond_function()
 ****************************************************************
 * @brief
 *  This function registers the callback functions with the base environment object.
 * 
 * @param base_object - This is the object handle, or in our
 * case the encl_mgmt.
 * @param packet - The packet arriving from the monitor
 * scheduler.
 * 
 * @return fbe_status_t - The status of the call to
 *                        fbe_lifecycle_class_const_verify() for
 *                        the Encl management's constant
 *                        lifecycle data.
 *
 * @author
 *  15-Nov-2010: Arun S - Created. 
 *
 ****************************************************************/
static fbe_lifecycle_status_t
fbe_encl_mgmt_resume_prom_register_callback_cond_function(fbe_base_object_t * base_object, 
                                                          fbe_packet_t * packet)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_base_environment_t * pBaseEnv = (fbe_base_environment_t *)base_object;

    fbe_base_object_trace(base_object, 
                          FBE_TRACE_LEVEL_DEBUG_HIGH,  
                          FBE_TRACE_MESSAGE_ID_INFO, 
                          "%s entry\n", 
                          __FUNCTION__);

    fbe_base_env_initiate_resume_prom_read_callback(pBaseEnv, &fbe_encl_mgmt_initiate_resume_prom_read);
    fbe_base_env_get_resume_prom_info_ptr_callback(pBaseEnv, (fbe_base_env_get_resume_prom_info_ptr_func_callback_ptr_t) fbe_encl_mgmt_get_resume_prom_info_ptr);
    fbe_base_env_resume_prom_update_encl_fault_led_callback(pBaseEnv, (fbe_base_env_resume_prom_update_encl_fault_led_callback_func_ptr_t) fbe_encl_mgmt_resume_prom_update_encl_fault_led);

    status = fbe_lifecycle_clear_current_cond(base_object);
    if (status != FBE_STATUS_OK) 
    {
        fbe_base_object_trace(base_object, 
                            FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                            "RP: %s can't clear current condition, status: 0x%X.\n",
                            __FUNCTION__, status);
    }
   
    return FBE_LIFECYCLE_STATUS_DONE;
}

/*******************************************************************
 * end - fbe_encl_mgmt_fup_register_callback_cond_function() 
 *******************************************************************/

/*!**************************************************************
 * fbe_encl_mgmt_state_change_notification_cond_function()
 ****************************************************************
 * @brief
 *  This is the handler function for state change notification.
 *  
 *
 * @param object_handle - This is the object handle, or in our
 * case the encl_mgmt.
 * @param packet_p - The packet arriving from the monitor
 * scheduler.
 *
 * @return fbe_status_t - The status of the call to
 *                        fbe_lifecycle_class_const_verify()
 *                        for the enclosure management's
 *                        constant lifecycle data.
 * 
 * 
 * @author
 *  23-Feb-2010:  Created. Ashok Tamilarasan
 *
 ****************************************************************/
static fbe_status_t 
fbe_encl_mgmt_state_change_handler(update_object_msg_t *update_object_msg, void *context)
{
    fbe_notification_info_t *notification_info = &update_object_msg->notification_info;
    fbe_encl_mgmt_t          *encl_mgmt;
    fbe_base_object_t       *base_object;
    fbe_status_t            status = FBE_STATUS_OK;

    encl_mgmt = (fbe_encl_mgmt_t *)context;
    base_object = (fbe_base_object_t *)encl_mgmt;

    fbe_base_object_trace(base_object, 
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry, objectId 0x%x, notifType 0x%llx, objType 0x%llx, devMsk 0x%llx\n",
                          __FUNCTION__, 
                          update_object_msg->object_id,
                          (unsigned long long)notification_info->notification_type, 
                          (unsigned long long)notification_info->object_type, 
                          notification_info->notification_data.data_change_info.device_mask);

    /*
     * Process the notification
     */

    switch (notification_info->notification_type)
    { 
        case FBE_NOTIFICATION_TYPE_OBJECT_DATA_CHANGED:
            status = fbe_encl_mgmt_handle_object_data_change(encl_mgmt, update_object_msg);
            break;

        case FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_READY:
            if(notification_info->object_type == FBE_TOPOLOGY_OBJECT_TYPE_ENCLOSURE) 
            {
                status = fbe_encl_mgmt_handle_encl_ready_state(encl_mgmt, update_object_msg);
            }
            else if(notification_info->object_type == FBE_TOPOLOGY_OBJECT_TYPE_LCC) 
            {
                status = fbe_encl_mgmt_handle_subencl_ready_state(encl_mgmt, update_object_msg);
            }
            break;


        case FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_FAIL:
            if(notification_info->object_type == FBE_TOPOLOGY_OBJECT_TYPE_ENCLOSURE) 
            {
                status = fbe_encl_mgmt_handle_encl_fail_state(encl_mgmt, update_object_msg);
            }
            else if(notification_info->object_type == FBE_TOPOLOGY_OBJECT_TYPE_LCC) 
            {
                status = fbe_encl_mgmt_handle_subencl_fail_state(encl_mgmt, update_object_msg);
            }
            break;
    
        case FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_DESTROY:
            if(notification_info->object_type == FBE_TOPOLOGY_OBJECT_TYPE_ENCLOSURE) 
            {
                status = fbe_encl_mgmt_handle_encl_destroy_state(encl_mgmt, update_object_msg);
            }
            else if(notification_info->object_type == FBE_TOPOLOGY_OBJECT_TYPE_LCC) 
            {
                status = fbe_encl_mgmt_handle_subencl_destroy_state(encl_mgmt, update_object_msg);
            }
            break;
            
        default:
            fbe_base_object_trace(base_object, 
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s Unhandled noticiation type:%llu\n",
                                  __FUNCTION__,
                                  (unsigned long long)notification_info->notification_type);
            status = FBE_STATUS_GENERIC_FAILURE;
            break;
    }
    return status;
}
/*******************************************************************
 * end fbe_encl_mgmt_state_change_handler() 
 *******************************************************************/

/*!**************************************************************
 * @fn fbe_encl_mgmt_handle_object_data_change()
 ****************************************************************
 * @brief
 *  This function is to handle the object data change.
 *
 * @param pEnclMgmt - This is the object handle, or in our case the ps_mgmt.
 * @param pUpdateObjectMsg - Pointer to the notification info
 *
 * @return fbe_status_t - FBE Status
 * 
 * @author
 *  10-Aug-2010:  PHE - Created.
 *
 ****************************************************************/
static fbe_status_t fbe_encl_mgmt_handle_object_data_change(fbe_encl_mgmt_t * pEnclMgmt, 
                                                          update_object_msg_t * pUpdateObjectMsg)
{
    fbe_notification_data_changed_info_t * pDataChangedInfo = NULL;
    fbe_status_t status = FBE_STATUS_OK;

    pDataChangedInfo = &(pUpdateObjectMsg->notification_info.notification_data.data_change_info);
    
    fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt, 
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry, dataType 0x%x.\n",
                          __FUNCTION__, pDataChangedInfo->data_type);
    
    switch(pDataChangedInfo->data_type)
    {
        case FBE_DEVICE_DATA_TYPE_GENERIC_INFO:
            status = fbe_encl_mgmt_handle_generic_info_change(pEnclMgmt, pUpdateObjectMsg);
            break;

        case FBE_DEVICE_DATA_TYPE_FUP_INFO:
            status = fbe_base_env_fup_handle_firmware_status_change((fbe_base_environment_t *)pEnclMgmt,
                                                                    pDataChangedInfo->device_mask,
                                                                    &(pDataChangedInfo->phys_location));
            break;

        default:
            fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt, 
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s Object data changed, unhandled data type %d.\n",
                                  __FUNCTION__, 
                                  pDataChangedInfo->data_type);
            break;
    }

    return status;
}// End of function fbe_encl_mgmt_handle_object_data_change

/*!**************************************************************
 * @fn fbe_encl_mgmt_handle_generic_info_change()
 ****************************************************************
 * @brief
 *  This function is to handle the generic info change.
 *
 * @param pEnclMgmt - This is the object handle, or in our case the encl_mgmt.
 * @param pUpdateObjectMsg - Pointer to the notification info
 *
 * @return fbe_status_t - FBE Status
 * 
 * @author
 *  30-Aug-2010:  PHE - Created.
 *
 ****************************************************************/
static fbe_status_t fbe_encl_mgmt_handle_generic_info_change(fbe_encl_mgmt_t * pEnclMgmt, 
                                                          update_object_msg_t * pUpdateObjectMsg)
{
    fbe_notification_data_changed_info_t   * pDataChangedInfo = NULL;
    fbe_device_physical_location_t         * pLocation = NULL;
    fbe_status_t                             status = FBE_STATUS_OK;
    fbe_notification_info_t                * pNotificationInfo = &pUpdateObjectMsg->notification_info;

    pDataChangedInfo = &(pUpdateObjectMsg->notification_info.notification_data.data_change_info);
    
    pLocation = &(pDataChangedInfo->phys_location);
    
    status = fbe_base_env_get_and_adjust_encl_location((fbe_base_environment_t *)pEnclMgmt, 
                                                       pUpdateObjectMsg->object_id, 
                                                       pLocation);
    if (status != FBE_STATUS_OK) {
        fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s Failed to get encl location, ObjectId %d, status 0x%x.\n",
                              __FUNCTION__, pUpdateObjectMsg->object_id, status);

        return status;
    }


    switch (pDataChangedInfo->device_mask)
    {
        case FBE_DEVICE_TYPE_LCC:
            if(pNotificationInfo->object_type == FBE_TOPOLOGY_OBJECT_TYPE_ENCLOSURE) 
            {
            
                status = fbe_encl_mgmt_process_lcc_status(pEnclMgmt,
                                                          pUpdateObjectMsg->object_id, 
                                                          pLocation);
            }
            else if(pNotificationInfo->object_type == FBE_TOPOLOGY_OBJECT_TYPE_LCC) 
            {
                status = fbe_encl_mgmt_process_subencl_lcc_status(pEnclMgmt,
                                                                  pUpdateObjectMsg->object_id, 
                                                                  pLocation);
            }
            break;

        case FBE_DEVICE_TYPE_ENCLOSURE:
            if(pNotificationInfo->object_type == FBE_TOPOLOGY_OBJECT_TYPE_ENCLOSURE) 
            {
                status = fbe_encl_mgmt_process_encl_fault_led_status(pEnclMgmt,
                                                                         pUpdateObjectMsg->object_id,
                                                                         pLocation);
                if(status == FBE_STATUS_OK) 
                {
                    status = fbe_encl_mgmt_process_enclosure_status(pEnclMgmt,
                                                                pUpdateObjectMsg->object_id, 
                                                                pLocation);
                }
                
            }
            break;

        case FBE_DEVICE_TYPE_DISPLAY:
            if(pNotificationInfo->object_type == FBE_TOPOLOGY_OBJECT_TYPE_ENCLOSURE) 
            {
                status = fbe_encl_mgmt_process_display_status(pEnclMgmt,
                                                              pUpdateObjectMsg->object_id,
                                                              pLocation);
                
            }
            break;

        case FBE_DEVICE_TYPE_CONNECTOR:
            if(pNotificationInfo->object_type == FBE_TOPOLOGY_OBJECT_TYPE_ENCLOSURE) 
            {
                status = fbe_encl_mgmt_process_connector_status(pEnclMgmt,
                                                                pUpdateObjectMsg->object_id, 
                                                                pLocation);
            }
            break;

        case FBE_DEVICE_TYPE_MISC:
            if(pNotificationInfo->object_type == FBE_TOPOLOGY_OBJECT_TYPE_BOARD) 
            {
                status = fbe_encl_mgmt_process_encl_fault_led_status(pEnclMgmt,
                                                                     pUpdateObjectMsg->object_id,
                                                                     pLocation);
            }
            break;
        case FBE_DEVICE_TYPE_DRIVE:
            status = fbe_encl_mgmt_process_slot_led_status(pEnclMgmt,
                                                           pUpdateObjectMsg->object_id,
                                                           pLocation);

            break;

        case FBE_DEVICE_TYPE_SSC:
            if(pNotificationInfo->object_type == FBE_TOPOLOGY_OBJECT_TYPE_ENCLOSURE) 
            {
                status = fbe_encl_mgmt_process_ssc_status(pEnclMgmt,
                                                                pUpdateObjectMsg->object_id, 
                                                                pLocation);
            }
            break;

        default:
            fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt, 
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s Generic info changed for deviceType %s, No action taken.\n",
                                  __FUNCTION__, 
                                  fbe_base_env_decode_device_type(pDataChangedInfo->device_mask));
            break;
    }
    
    if (status != FBE_STATUS_OK) 
    {
        fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt, 
                                      FBE_TRACE_LEVEL_WARNING,
                                      FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                      "%s(%d_%d_%d) Error in handling generic info change, status 0x%x.\n",
                                      fbe_base_env_decode_device_type(pDataChangedInfo->device_mask), 
                                      pLocation->bus,
                                      pLocation->enclosure,
                                      pLocation->slot,
                                      status);
    }

    return status;
}// End of function fbe_encl_mgmt_handle_generic_info_change


/*!**************************************************************
 * fbe_encl_mgmt_handle_encl_ready_state()
 ****************************************************************
 * @brief
 *  This is the handler function for life cycle change to a
 *  ready state.
 *  
 *
 * @param encl_mgmt - This is the object handle, or in our case
 * the encl_mgmt.
 * @param update_object_msg - Pointer to the notification info
 *
 * @return fbe_status_t - FBE Status
 * 
 * @author
 *  04-Jun-2010:  Created. Ashok Tamilarasan
 *
 ****************************************************************/
static fbe_status_t fbe_encl_mgmt_handle_encl_ready_state(fbe_encl_mgmt_t *encl_mgmt, 
                                                     update_object_msg_t *update_object_msg)
{
    fbe_status_t status;
    fbe_object_id_t object_id;
    fbe_encl_info_t * pEnclInfo = NULL;
    fbe_encl_info_t * oldEnclInfo;
    fbe_device_physical_location_t   location = {0};

    object_id = update_object_msg->object_id;

    status = fbe_encl_mgmt_get_encl_info_by_object_id(encl_mgmt, object_id, &pEnclInfo);

    oldEnclInfo = (fbe_encl_info_t *) fbe_base_env_memory_ex_allocate((fbe_base_environment_t *)encl_mgmt,
                                                                      sizeof(fbe_encl_info_t));
    if (oldEnclInfo == NULL) {
        fbe_base_object_trace((fbe_base_object_t *)encl_mgmt,
                FBE_TRACE_LEVEL_INFO,
                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                "%s: fail to allocate memory for encl info\n",
                __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if(status == FBE_STATUS_OK)
    {
        /* We know about the enclosure before but we dont care about
         * this transistion to ready state because it could be from
         * activate to ready or one of the transistioning state to ready
         * state
         */
        /* Save the encl info before updating it. */
        fbe_copy_memory(oldEnclInfo, pEnclInfo, sizeof(fbe_encl_info_t));

        fbe_spinlock_lock(&pEnclInfo->encl_info_lock);
        pEnclInfo->enclState = FBE_ESP_ENCL_STATE_OK;
        pEnclInfo->enclFaultSymptom = FBE_ESP_ENCL_FAULT_SYM_NO_FAULT;
        fbe_spinlock_unlock(&pEnclInfo->encl_info_lock);
        
        /* Send the notification for enclosure state change*/
        location.bus = pEnclInfo->location.bus;
        location.enclosure = pEnclInfo->location.enclosure;

        /* Trace and log event. */
        status = fbe_encl_mgmt_encl_log_event(encl_mgmt, &location, pEnclInfo, oldEnclInfo);

        if(status != FBE_STATUS_OK) 
        {
            fbe_base_object_trace((fbe_base_object_t *)encl_mgmt, 
                                  FBE_TRACE_LEVEL_WARNING,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s encl_mgmt_encl_log_event failed, status 0x%X.\n",
                                  __FUNCTION__, status);
        }   
        
        fbe_base_environment_send_data_change_notification((fbe_base_environment_t*) encl_mgmt,
                                                           FBE_CLASS_ID_ENCL_MGMT, 
                                                           FBE_DEVICE_TYPE_ENCLOSURE, 
                                                           FBE_DEVICE_DATA_TYPE_GENERIC_INFO,
                                                           &location);
    }
    else
    {
        /* We did not find the enclosure before but now it is coming
         * online so handle it
         */
        status = fbe_encl_mgmt_handle_new_encl(encl_mgmt,object_id);

        if (status != FBE_STATUS_OK) 
        {
            fbe_base_object_trace((fbe_base_object_t *)encl_mgmt, 
                                  FBE_TRACE_LEVEL_WARNING,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s failed to handle new encl, status: 0x%x.\n",
                                  __FUNCTION__, status);
        }
    }

    fbe_base_env_memory_ex_release((fbe_base_environment_t *)encl_mgmt, oldEnclInfo);

    return status;
}
/*******************************************************************
 * end fbe_encl_mgmt_handle_ready_state() 
 *******************************************************************/

/*!**************************************************************
 * fbe_encl_mgmt_handle_subencl_ready_state()
 ****************************************************************
 * @brief
 *  This is the handler function for life cycle change to a
 *  ready state for the subencl.
 *  
 *
 * @param encl_mgmt - This is the object handle, or in our case
 * the encl_mgmt.
 * @param update_object_msg - Pointer to the notification info
 *
 * @return fbe_status_t - FBE Status
 * 
 * @author
 *  08-Dec-2011:  PHE - Created.
 *
 ********************************************.********************/
static fbe_status_t fbe_encl_mgmt_handle_subencl_ready_state(fbe_encl_mgmt_t * pEnclMgmt, 
                                                     update_object_msg_t *update_object_msg)
{
    fbe_status_t                     status = FBE_STATUS_OK;
    fbe_object_id_t                  subEnclObjectId;
    fbe_sub_encl_info_t            * pSubEnclInfo = NULL;

    subEnclObjectId = update_object_msg->object_id;

    status = fbe_encl_mgmt_get_subencl_info_by_subencl_object_id(pEnclMgmt, subEnclObjectId, &pSubEnclInfo);

    if(status == FBE_STATUS_OK)
    {
        /* We know about the enclosure before but we dont care about
         * this transistion to ready state because it could be from
         * activate to ready or one of the transistioning state to ready
         * state
         */
        /* Do nothing.*/
        
    }
    else
    {
        status = fbe_encl_mgmt_handle_new_subencl(pEnclMgmt,subEnclObjectId);
    
        if (status != FBE_STATUS_OK) 
        {
            fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt, 
                                  FBE_TRACE_LEVEL_WARNING,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s failed to handle new subEnclObjectId 0x%X, status 0x%X.\n",
                                  __FUNCTION__, subEnclObjectId, status);
        }
    }

    return status;
}
/*******************************************************************
 * end fbe_encl_mgmt_handle_subencl_ready_state() 
 *******************************************************************/

/*!**************************************************************
 * fbe_encl_mgmt_handle_encl_fail_state()
 ****************************************************************
 * @brief
 *  This is the handler function for life cycle change to a
 *  fail state.
 *  
 *
 * @param encl_mgmt - This is the object handle, or in our case
 * the encl_mgmt.
 * @param update_object_msg - Pointer to the notification info
 *
 * @return fbe_status_t - FBE Status
 * 
 * @author
 *  23-Feb-2012:  PHE - Created.
 *
 ****************************************************************/
static fbe_status_t fbe_encl_mgmt_handle_encl_fail_state(fbe_encl_mgmt_t *encl_mgmt, 
                                                     update_object_msg_t *update_object_msg)
{
    fbe_status_t                      status = FBE_STATUS_OK;
    fbe_object_id_t                   objectId;
    fbe_encl_info_t                 * pEnclInfo = NULL;
    fbe_encl_info_t                 * pOldEnclInfo = NULL;
    fbe_enclosure_fault_t             enclosure_fault_info = {0};

    objectId = update_object_msg->object_id;

    status = fbe_encl_mgmt_get_encl_info_by_object_id(encl_mgmt, objectId, &pEnclInfo);

    if (status != FBE_STATUS_OK) 
    {
        /* We cannot get the enclosure info. 
         * It means the enclosure never got to the ready state and must now be coming up failed. 
         * So we need to handle it since it is a new enclosure.
         */
        status = fbe_encl_mgmt_handle_new_encl(encl_mgmt,objectId);

        if (status != FBE_STATUS_OK) 
        {
            fbe_base_object_trace((fbe_base_object_t *)encl_mgmt, 
                                  FBE_TRACE_LEVEL_WARNING,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s failed to handle new encl, status: 0x%x.\n",
                                  __FUNCTION__, status);
        }

        return status;
    }

    fbe_base_object_trace((fbe_base_object_t *)encl_mgmt, 
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "Encl(%d_%d), %s entry.\n",
                          pEnclInfo->location.bus, pEnclInfo->location.enclosure, __FUNCTION__);

    pOldEnclInfo = (fbe_encl_info_t *) fbe_base_env_memory_ex_allocate((fbe_base_environment_t *)encl_mgmt,
                                                                       sizeof(fbe_encl_info_t));
    if (pOldEnclInfo == NULL) {
        fbe_base_object_trace((fbe_base_object_t *)encl_mgmt,
                FBE_TRACE_LEVEL_INFO,
                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                "%s: fail to allocate memory for encl info\n",
                __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    fbe_base_object_trace((fbe_base_object_t *)encl_mgmt, 
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "Encl(%d_%d), %s entry.\n",
                          pEnclInfo->location.bus, pEnclInfo->location.enclosure, __FUNCTION__);
    /*Save old encl info before updating it */
    fbe_copy_memory(pOldEnclInfo, pEnclInfo, sizeof(fbe_encl_info_t));

    status = fbe_api_enclosure_get_fault_info(objectId, &enclosure_fault_info);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)encl_mgmt,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "Encl(%d_%d), %s Error in getting fault info, status: 0x%X.\n",
                              pEnclInfo->location.bus, pEnclInfo->location.enclosure, __FUNCTION__, status);

        fbe_base_env_memory_ex_release((fbe_base_environment_t *)encl_mgmt, pOldEnclInfo);
        return status;
    }
    else if (enclosure_fault_info.faultSymptom == FBE_ENCL_FLTSYMPT_EXCEEDS_MAX_LIMITS)
    {
        fbe_spinlock_lock(&pEnclInfo->encl_info_lock);
        pEnclInfo->enclState = FBE_ESP_ENCL_STATE_FAILED;
        pEnclInfo->enclFaultSymptom = FBE_ESP_ENCL_FAULT_SYM_EXCEEDED_MAX;
        fbe_spinlock_unlock(&pEnclInfo->encl_info_lock); 
        fbe_base_object_trace((fbe_base_object_t *)encl_mgmt, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "Encl(%d_%d), failed encl detected setting enclosure fault symptom to EXCEED_MAX.\n",
                              pEnclInfo->location.bus, 
                              pEnclInfo->location.enclosure); 
    }
    /* Try and preserve the the FBE_ESP_ENCL_FAULT_SYP_EXCEEDED_MAX fault */
    else if ((pOldEnclInfo->enclState != FBE_ESP_ENCL_STATE_FAILED) ||
                  (pOldEnclInfo->enclFaultSymptom == FBE_ESP_ENCL_FAULT_SYM_NO_FAULT))
    {
        fbe_spinlock_lock(&pEnclInfo->encl_info_lock);
        pEnclInfo->enclState = FBE_ESP_ENCL_STATE_FAILED;
        pEnclInfo->enclFaultSymptom = FBE_ESP_ENCL_FAULT_SYM_LIFECYCLE_STATE_FAIL;
        fbe_spinlock_unlock(&pEnclInfo->encl_info_lock);   
    }

    /* Trace and Log event. */
    status = fbe_encl_mgmt_encl_log_event(encl_mgmt, 
                                        &pEnclInfo->location, 
                                        pEnclInfo, 
                                        pOldEnclInfo);

    if(status != FBE_STATUS_OK) 
    {
        fbe_base_object_trace((fbe_base_object_t *)encl_mgmt, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s encl_mgmt_encl_log_event failed, status 0x%X.\n",
                              __FUNCTION__, status);
    }    

    status = fbe_encl_mgmt_update_encl_fault_led(encl_mgmt, 
                                                     &pEnclInfo->location, 
                                                     FBE_ENCL_FAULT_LED_ENCL_LIFECYCLE_FAIL);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)encl_mgmt, 
                          FBE_TRACE_LEVEL_WARNING,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "Encl(%d_%d), LIFECYCLE_FAIL, update_encl_fault_led failed, status: 0x%X.\n",
                          pEnclInfo->location.bus, 
                          pEnclInfo->location.enclosure, 
                          status);
    }

    /* Send the notification for enclosure state change*/
    fbe_base_environment_send_data_change_notification((fbe_base_environment_t*) encl_mgmt,
                                                       FBE_CLASS_ID_ENCL_MGMT, 
                                                       FBE_DEVICE_TYPE_ENCLOSURE, 
                                                       FBE_DEVICE_DATA_TYPE_GENERIC_INFO,
                                                       &pEnclInfo->location);

    fbe_base_env_memory_ex_release((fbe_base_environment_t *)encl_mgmt, pOldEnclInfo);

    return status;
}
/*******************************************************************
 * end fbe_encl_mgmt_handle_encl_fail_state() 
 *******************************************************************/
/*!**************************************************************
 * fbe_encl_mgmt_handle_subencl_fail_state()
 ****************************************************************
 * @brief
 *  This is the handler function for life cycle change to a
 *  fail state for the subencl. 
 *  For example, the voyager edge expander object went to the fail state.
 *  
 *
 * @param encl_mgmt - This is the object handle, or in our case
 * the encl_mgmt.
 * @param update_object_msg - Pointer to the notification info
 *
 * @return fbe_status_t - FBE Status
 * 
 * @author
 *  23-Feb-2012:  PHE - Created.
 *
 ********************************************.********************/
static fbe_status_t fbe_encl_mgmt_handle_subencl_fail_state(fbe_encl_mgmt_t * pEnclMgmt, 
                                                     update_object_msg_t *update_object_msg)
{
    fbe_status_t                      status = FBE_STATUS_OK;
    fbe_object_id_t                   subEnclObjectId;
    fbe_sub_encl_info_t             * pSubEnclInfo = NULL;
    fbe_device_physical_location_t  * pSubEnclLocation = NULL;

    subEnclObjectId = update_object_msg->object_id;

    status = fbe_encl_mgmt_get_subencl_info_by_subencl_object_id(pEnclMgmt, subEnclObjectId, &pSubEnclInfo);
    if (status != FBE_STATUS_OK) 
    {
        fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s get_subencl_info_by_subencl_object_id failed.\n",
                              __FUNCTION__);
        fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "subEnclObjectId 0x%X status 0x%X.\n",
                              subEnclObjectId, status);

        return status;
    }

    pSubEnclLocation = &pSubEnclInfo->location;

    
    status = fbe_encl_mgmt_update_encl_fault_led(pEnclMgmt, 
                                                 pSubEnclLocation, 
                                                 FBE_ENCL_FAULT_LED_SUBENCL_LIFECYCLE_FAIL);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt, 
                          FBE_TRACE_LEVEL_WARNING,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "Encl(%d_%d.%d), LIFECYCLE_FAIL, update_encl_fault_led failed, status: 0x%X.\n",
                          pSubEnclLocation->bus, 
                          pSubEnclLocation->enclosure, 
                          pSubEnclLocation->componentId,
                          status);
    }
    return status;
}
/*******************************************************************
 * end fbe_encl_mgmt_handle_subencl_fail_state() 
 *******************************************************************/

/*!**************************************************************
 * fbe_encl_mgmt_handle_encl_destroy_state()
 ****************************************************************
 * @brief
 *  This is the handler function for life cycle change to a
 *  destroy state.
 *
 * @param encl_mgmt - This is the object handle, or in our case
 * the encl_mgmt.
 * @param update_object_msg - Pointer to the notification info
 *
 * @return fbe_status_t - FBE Status
 * 
 * @author
 *  04-Jun-2010:  Created. Ashok Tamilarasan
 *
 ****************************************************************/
static fbe_status_t fbe_encl_mgmt_handle_encl_destroy_state(fbe_encl_mgmt_t *encl_mgmt, 
                                                       update_object_msg_t *update_object_msg)
{
    fbe_status_t                    status;
    fbe_object_id_t                 object_id;
    fbe_encl_info_t               * pEnclInfo = NULL;
    fbe_encl_info_t               * oldEnclInfo = NULL;
    fbe_device_physical_location_t  phys_location = {0};

    object_id = update_object_msg->object_id;

    status = fbe_encl_mgmt_get_encl_info_by_object_id(encl_mgmt, object_id, &pEnclInfo);
    if (status != FBE_STATUS_OK) 
    {
        fbe_base_object_trace((fbe_base_object_t *)encl_mgmt, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s get_encl_info_by_object_id failed, objectId %d status 0x%x.\n",
                              __FUNCTION__, object_id, status);
        return status;
    }

    oldEnclInfo = (fbe_encl_info_t *)fbe_base_env_memory_ex_allocate((fbe_base_environment_t *)encl_mgmt, sizeof(fbe_encl_info_t));
    if(oldEnclInfo == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t*)encl_mgmt,
                                    FBE_TRACE_LEVEL_WARNING,
                                    FBE_TRACE_MESSAGE_ID_INFO,
                                    "%s encl_info memory allocation failed.\n",
                                    __FUNCTION__);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Save the old encl info before updating it. */
    fbe_copy_memory(oldEnclInfo, pEnclInfo, sizeof(fbe_encl_info_t));

    /* Save the bus and enclosure for tracing or logging purpose in the end of the function. */
    phys_location.bus = pEnclInfo->location.bus;
    phys_location.enclosure = pEnclInfo->location.enclosure;

    
    fbe_spinlock_lock(&pEnclInfo->encl_info_lock);
    if(pEnclInfo->enclFaultSymptom != FBE_ESP_ENCL_FAULT_SYM_EXCEEDED_MAX)
    {
        encl_mgmt->total_drive_slot_count -= pEnclInfo->driveSlotCount;
    }
    fbe_base_object_trace((fbe_base_object_t *)encl_mgmt, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s: (%d_%d)Remove disk encl slots:%d,total:%d.\n",
                              __FUNCTION__,
                              phys_location.bus,
                              phys_location.enclosure,
                              pEnclInfo->driveSlotCount,
                              encl_mgmt->total_drive_slot_count);
    
    /* Since the enclosure was destroyed clean out the structure */
    status = fbe_encl_mgmt_init_encl_info(encl_mgmt, pEnclInfo);
    encl_mgmt->total_encl_count--;
    fbe_spinlock_unlock(&pEnclInfo->encl_info_lock);

    fbe_base_object_trace((fbe_base_object_t *)encl_mgmt, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "(%d_%d), %s totaldriveSlotCount is now %d.\n",
                              phys_location.bus, phys_location.enclosure,__FUNCTION__, encl_mgmt->total_drive_slot_count);

    

    /* Trace and Log event. */
    status = fbe_encl_mgmt_encl_log_event(encl_mgmt, 
                                        &phys_location,  
                                        pEnclInfo, 
                                        oldEnclInfo);

    if(status != FBE_STATUS_OK) 
    {
        fbe_base_object_trace((fbe_base_object_t *)encl_mgmt, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s encl_mgmt_encl_log_event failed, status 0x%X.\n",
                              __FUNCTION__, status);
    }  

    status = fbe_base_env_fup_handle_destroy_state((fbe_base_environment_t *)encl_mgmt, 
                                                   &phys_location);

    if(status != FBE_STATUS_OK) 
    {
        fbe_base_object_trace((fbe_base_object_t *)encl_mgmt, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "FUP: %s, fup_handle_destroy_state failed, bus %d encl %d status 0x%X.\n",
                              __FUNCTION__,
                              pEnclInfo->location.bus, 
                              pEnclInfo->location.enclosure, 
                              status);
    }
   
    /* Enclosure is destroyed/Removed. Clear the Resume Prom Info */
    status = fbe_base_env_resume_prom_handle_destroy_state((fbe_base_environment_t *)encl_mgmt, 
                                                           phys_location.bus, 
                                                           phys_location.enclosure);

    if(status != FBE_STATUS_OK) 
    {
        fbe_base_object_trace((fbe_base_object_t *)encl_mgmt, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "RP: resume_prom_handle_destroy_state failed, bus %d encl %d.\n",
                              phys_location.bus, 
                              phys_location.enclosure);
    }
   
    /* Send notification for enclosure data change*/
    fbe_base_environment_send_data_change_notification((fbe_base_environment_t*)encl_mgmt,
                                                        FBE_CLASS_ID_ENCL_MGMT,
                                                        FBE_DEVICE_TYPE_ENCLOSURE, 
                                                        FBE_DEVICE_DATA_TYPE_GENERIC_INFO,
                                                        &phys_location);

    fbe_base_env_memory_ex_release((fbe_base_environment_t *)encl_mgmt, oldEnclInfo);

    return status;
}
/*******************************************************************
 * end fbe_encl_mgmt_handle_encl_destroy_state() 
 *******************************************************************/

/*!**************************************************************
 * fbe_encl_mgmt_handle_subencl_destroy_state()
 ****************************************************************
 * @brief
 *  This is the handler function for life cycle change to a
 *  destroy state.
 *
 * @param encl_mgmt - This is the object handle, or in our case
 * the encl_mgmt.
 * @param update_object_msg - Pointer to the notification info
 *
 * @return fbe_status_t - FBE Status
 * 
 * @author
 *  03-Feb-2012:  PHE - Created.
 *
 ****************************************************************/
static fbe_status_t fbe_encl_mgmt_handle_subencl_destroy_state(fbe_encl_mgmt_t * pEnclMgmt, 
                                                       update_object_msg_t *update_object_msg)
{
    fbe_status_t                      status = FBE_STATUS_OK;
    fbe_object_id_t                   subEnclObjectId;
    fbe_sub_encl_info_t             * pSubEnclInfo = NULL;
    fbe_encl_info_t                 * pEnclInfo = NULL;
    fbe_device_physical_location_t  * pSubEnclLocation = NULL;

    subEnclObjectId = update_object_msg->object_id;

    status = fbe_encl_mgmt_get_subencl_info_by_subencl_object_id(pEnclMgmt, subEnclObjectId, &pSubEnclInfo);
    if (status != FBE_STATUS_OK) 
    {
        /* It is possible that the containing enclosure got destroyed before the subenclosure.
         * So we just put the warning trace here.
         */
        fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s get_subencl_info_by_subencl_object_id failed.\n",
                              __FUNCTION__);
        fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "subEnclObjectId 0x%X status 0x%X.\n",
                              subEnclObjectId, status);

        return status;
    }

    pSubEnclLocation = &pSubEnclInfo->location;

    status = fbe_base_env_fup_handle_destroy_state((fbe_base_environment_t *)pEnclMgmt, pSubEnclLocation);

    if(status != FBE_STATUS_OK) 
    {
        fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s, Encl %d_%d.%d, fup_handle_destroy_state failed, status 0x%X.\n",
                              __FUNCTION__,
                              pSubEnclLocation->bus, 
                              pSubEnclLocation->enclosure,
                              pSubEnclLocation->componentId,
                              status);
    }
    
    /* Find the pointer to the containing encl info. */
    status = fbe_encl_mgmt_get_encl_info_by_location(pEnclMgmt, pSubEnclLocation, &pEnclInfo);
    if(status != FBE_STATUS_OK) 
    {
        fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s, get_encl_info_by_location failed, status 0x%X.\n",
                              __FUNCTION__,
                              status);

        return status;
    }
   
    pEnclInfo->currSubEnclCount --;

    fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt, 
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s,Encl %d_%d.%d,subEnclObjectId 0x%X Removed,currSubEnclCount %d.\n",
                          __FUNCTION__,
                          pSubEnclLocation->bus, 
                          pSubEnclLocation->enclosure,
                          pSubEnclLocation->componentId,
                          subEnclObjectId,
                          pEnclInfo->currSubEnclCount);

    fbe_encl_mgmt_init_subencl_info(pSubEnclInfo);
    
    /* Send notification for enclosure data change*/
    fbe_base_environment_send_data_change_notification((fbe_base_environment_t*)pEnclMgmt,
                                                        FBE_CLASS_ID_ENCL_MGMT,
                                                        FBE_DEVICE_TYPE_ENCLOSURE, 
                                                        FBE_DEVICE_DATA_TYPE_GENERIC_INFO,
                                                        &pEnclInfo->location);

    return status;
}
/*******************************************************************
 * end fbe_encl_mgmt_handle_subencl_destroy_state() 
 *******************************************************************/


/*!**************************************************************
 * fbe_encl_mgmt_cmi_message_handler()
 ****************************************************************
 * @brief
 *  This is the handler function for CMI message notification.
 *  
 * @param pBaseObject - This is the object handle, or in our case the ps_mgmt.
 * @param pCmiMsg - The point to fbe_base_environment_cmi_message_info_t
 *
 * @return fbe_status_t -  
 * 
 * @author
 *  21-Jan-2011 : Created GB
 *  29-Apr-2011: PHE - Updated. 
 ****************************************************************/
fbe_status_t 
fbe_encl_mgmt_cmi_message_handler(fbe_base_object_t *pBaseObject, 
                                  fbe_base_environment_cmi_message_info_t *pCmiMsg)
{
    fbe_encl_mgmt_t                    * pEnclMgmt = (fbe_encl_mgmt_t *)pBaseObject;
    fbe_encl_mgmt_cmi_packet_t         * pEnclCmiPkt = NULL;
    fbe_base_environment_cmi_message_t * pBaseCmiPkt = NULL;
    fbe_status_t                         status = FBE_STATUS_OK;

    // client message (if it exists)
    pEnclCmiPkt = (fbe_encl_mgmt_cmi_packet_t *)pCmiMsg->user_message;
    pBaseCmiPkt = (fbe_base_environment_cmi_message_t *)pEnclCmiPkt;

    if(pBaseCmiPkt == NULL) 
    {
        fbe_base_object_trace(pBaseObject, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s, Event 0x%x, no msgType\n",
                              __FUNCTION__,
                              pCmiMsg->event);
    }
    else
    {
        fbe_base_object_trace(pBaseObject, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s, Event 0x%X, msgType %s(0x%llX).\n",
                              __FUNCTION__,
                              pCmiMsg->event, 
                              fbe_encl_mgmt_getCmiMsgTypeString(pBaseCmiPkt->opcode),
                              (unsigned long long)pBaseCmiPkt->opcode);
    }
   
    switch (pCmiMsg->event)
    {
        case FBE_CMI_EVENT_MESSAGE_RECEIVED:
            status = fbe_encl_mgmt_processReceivedCmiMessage(pEnclMgmt, pEnclCmiPkt);
            break;

        case FBE_CMI_EVENT_PEER_NOT_PRESENT:
            status = fbe_encl_mgmt_processPeerNotPresent(pEnclMgmt, pEnclCmiPkt);
            break;

        case FBE_CMI_EVENT_SP_CONTACT_LOST:
            status = fbe_encl_mgmt_processContactLost(pEnclMgmt);
            break;

        case FBE_CMI_EVENT_PEER_BUSY:
            status = fbe_encl_mgmt_processPeerBusy(pEnclMgmt, pEnclCmiPkt);
            break;

        case FBE_CMI_EVENT_FATAL_ERROR:
            status = fbe_encl_mgmt_processFatalError(pEnclMgmt, pEnclCmiPkt);
            break;

        default:
            status = FBE_STATUS_MORE_PROCESSING_REQUIRED;
            break;
    }

    if (status == FBE_STATUS_MORE_PROCESSING_REQUIRED)
    {
        // handle the message in base env
        status = fbe_base_environment_cmi_message_handler(pBaseObject, pCmiMsg);

        if(status != FBE_STATUS_OK) 
        {
            fbe_base_object_trace(pBaseObject, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s failed to handle CMI msg.\n",
                               __FUNCTION__);
        }
    }
    return status;

} //fbe_encl_mgmt_cmi_message_handler

/*******************************************************************
 * end fbe_encl_mgmt_cmi_message_handler() 
 *******************************************************************/

/*!**************************************************************
 * fbe_encl_mgmt_discover_enclosure_cond_function()
 ****************************************************************
 * @brief
 *  This function gets the list of all the new enclosure that
 *  was added and adds them to the queue to be processed later
 *
 * @param object_handle - This is the object handle, or in our
 * case the encl_mgmt.
 * @param packet_p - The packet arriving from the monitor
 * scheduler.
 *
 * @return fbe_lifecycle_status_t - The status of the call to
 *                        fbe_lifecycle_class_const_verify()
 *                        for the enclosure management's
 *                        constant lifecycle data.
 *
 * @author
 *  23-Feb-2010:  Created. Ashok Tamilarasan
 *
 ****************************************************************/
static fbe_lifecycle_status_t 
fbe_encl_mgmt_discover_enclosure_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet)
{
    fbe_encl_mgmt_t  * encl_mgmt = (fbe_encl_mgmt_t *) base_object;
    fbe_object_id_t    object_id;
    fbe_object_id_t  * object_id_list = NULL;
    fbe_object_id_t  * subEnclObjectIdList = NULL;
    fbe_u32_t          encl = 0;
    fbe_u32_t          enclosure_count = 0; 
    fbe_u32_t          subEncl = 0;
    fbe_u32_t          subEnclCount = 0;
    HW_ENCLOSURE_TYPE  processorEnclType = INVALID_ENCLOSURE_TYPE;
    fbe_status_t       status = FBE_STATUS_OK;
    fbe_lifecycle_state_t  lifecycle_state = FBE_LIFECYCLE_STATE_INVALID;

    fbe_base_object_trace(base_object, 
                          FBE_TRACE_LEVEL_INFO,  
                          FBE_TRACE_MESSAGE_ID_INFO, 
                          "%s entry\n", 
                          __FUNCTION__);
    
    /* Get the list of all the enclosures the system sees*/
    object_id_list = fbe_base_env_memory_ex_allocate((fbe_base_environment_t *)encl_mgmt, sizeof(fbe_object_id_t) * FBE_MAX_PHYSICAL_OBJECTS);
    fbe_set_memory(object_id_list, (fbe_u8_t)0xFF, sizeof(fbe_object_id_t) * FBE_MAX_PHYSICAL_OBJECTS);

    /* The first object id in the list is for the board. */
    status = fbe_api_get_board_object_id(&object_id);
    if (status != FBE_STATUS_OK) 
    {
        fbe_base_object_trace(base_object, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s Error in getting the Board Object ID, status: 0x%X\n",
                              __FUNCTION__, status);
        fbe_base_env_memory_ex_release((fbe_base_environment_t *)encl_mgmt, object_id_list);
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    status = fbe_base_env_get_processor_encl_type((fbe_base_environment_t *)base_object,
                                                   &processorEnclType);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace(base_object, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s Error getting peType(XPE or DPE), status: 0x%x.\n",
                              __FUNCTION__, status);
        fbe_base_env_memory_ex_release((fbe_base_environment_t *)encl_mgmt, object_id_list);
        return FBE_LIFECYCLE_STATUS_DONE;
    }
   
    if( processorEnclType == XPE_ENCLOSURE_TYPE ||
        processorEnclType == VPE_ENCLOSURE_TYPE )
    {
        object_id_list[0] = object_id;

        /* The rest object ids in the list is for DAE enclosures.*/
        status = fbe_api_get_all_enclosure_object_ids(&object_id_list[1], 
                                                      FBE_MAX_PHYSICAL_OBJECTS - 1, 
                                                      &enclosure_count);
        enclosure_count++;
    }
    else if(processorEnclType == DPE_ENCLOSURE_TYPE)
    {
        /* Get the list of all the enclosures the system sees*/
        status = fbe_api_get_all_enclosure_object_ids(&object_id_list[0], 
                                                      FBE_MAX_PHYSICAL_OBJECTS, 
                                                      &enclosure_count);
    }
    else
    {
        fbe_base_object_trace(base_object, 
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s Invalid peType %d.\n",
                                __FUNCTION__, processorEnclType); 

        fbe_base_env_memory_ex_release((fbe_base_environment_t *)encl_mgmt, object_id_list);
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    if (status != FBE_STATUS_OK) 
    {
        fbe_base_object_trace(base_object, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s Error in getting the enclosure object ids, status: 0x%x.\n",
                              __FUNCTION__, status);

        fbe_base_env_memory_ex_release((fbe_base_environment_t *)encl_mgmt, object_id_list);
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    subEnclObjectIdList = fbe_base_env_memory_ex_allocate((fbe_base_environment_t *)encl_mgmt, sizeof(fbe_object_id_t) * FBE_ESP_MAX_SUBENCLS_PER_ENCL);

    for(encl = 0; encl < enclosure_count; encl ++) 
    {
        status = fbe_api_get_object_lifecycle_state(object_id_list[encl], 
                                                    &lifecycle_state, 
                                                    FBE_PACKAGE_ID_PHYSICAL);

        if((status != FBE_STATUS_OK) ||
           (lifecycle_state == FBE_LIFECYCLE_STATE_SPECIALIZE) ||  
           (lifecycle_state == FBE_LIFECYCLE_STATE_ACTIVATE))
        {
            fbe_base_object_trace(base_object, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s, skip obj %d, lifecycleState %d, status %d.\n",
                              __FUNCTION__, object_id_list[encl], lifecycle_state, status);

            /* We will get the notification when the enclosure object transitions to READY state.
             * So skip here
             */
            continue;
        }

        status = fbe_encl_mgmt_handle_new_encl(encl_mgmt, object_id_list[encl]);

        if (status != FBE_STATUS_OK) 
        {
            fbe_base_object_trace(base_object, 
                                  FBE_TRACE_LEVEL_WARNING,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s failed to handle new enclObject 0x%X, status 0x%X.\n",
                                  __FUNCTION__, object_id_list[encl], status);
            continue;
        }
        
        fbe_set_memory(subEnclObjectIdList, (fbe_u8_t)0xFF, sizeof(fbe_object_id_t) * FBE_ESP_MAX_SUBENCLS_PER_ENCL);

        status = fbe_api_get_enclosure_lcc_object_ids(object_id_list[encl],
                                                      subEnclObjectIdList,
                                                      FBE_ESP_MAX_SUBENCLS_PER_ENCL, 
                                                      &subEnclCount);
        
        if (status != FBE_STATUS_OK) 
        {
            fbe_base_object_trace(base_object, 
                                  FBE_TRACE_LEVEL_WARNING,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s Failed to get subEnclObjectIdList, enclObjectId 0x%X, status 0x%X.\n",
                                  __FUNCTION__, object_id_list[encl], status);
            continue;
        }

        for(subEncl = 0; subEncl < subEnclCount; subEncl ++) 
        {
            status = fbe_encl_mgmt_handle_new_subencl(encl_mgmt, subEnclObjectIdList[subEncl]);
    
            if (status != FBE_STATUS_OK) 
            {
                fbe_base_object_trace(base_object, 
                                      FBE_TRACE_LEVEL_WARNING,
                                      FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                      "%s Failed to handle new subEnclObjectId 0x%X, status 0x%X.\n",
                                      __FUNCTION__, subEnclObjectIdList[subEncl], status);
                
                continue;
            }
        }
    }

    fbe_encl_mgmt_check_overlimit_enclosures(base_object);
    
    status = fbe_lifecycle_clear_current_cond(base_object);
    if (status != FBE_STATUS_OK) {
        fbe_base_object_trace(base_object, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s can't clear current condition, status: 0x%x.\n",
                              __FUNCTION__, status);
    }

    fbe_base_env_memory_ex_release((fbe_base_environment_t *)encl_mgmt, object_id_list);
    fbe_base_env_memory_ex_release((fbe_base_environment_t *)encl_mgmt, subEnclObjectIdList);

    return FBE_LIFECYCLE_STATUS_DONE;
}
/******************************************************
 * end fbe_encl_mgmt_discover_enclosure_cond_function() 
 ******************************************************/

/*!**************************************************************
 * fbe_encl_mgmt_handle_new_encl()
 ****************************************************************
 * @brief
 *  This function handles the new enclsoure. It populates the enclosure attributes
 *  for the new enclosure.
 *
 * @param object_handle - This is the object handle, or in our
 * case the encl_mgmt.
 * @param packet_p - The packet arriving from the monitor
 * scheduler.
 * @param object_id - Object ID of the enclosure that just got
 * added
 *
 * @return fbe_status_t - FBE Status
 *
 * @author
 *  23-Feb-2010:  Created. Ashok Tamilarasan
 *  13-Jan-2011: PHE - Reorganized the code.
 *
 ****************************************************************/
static fbe_status_t 
fbe_encl_mgmt_handle_new_encl(fbe_encl_mgmt_t *encl_mgmt, fbe_object_id_t object_id)
{
    fbe_encl_info_t                * pEnclInfo = NULL;
    fbe_device_physical_location_t  location = {0};
    fbe_u32_t                       slot = 0;
    fbe_status_t                    status = FBE_STATUS_OK;

    status = fbe_base_env_get_and_adjust_encl_location((fbe_base_environment_t *)encl_mgmt, 
                                                       object_id, 
                                                       &location);
    if (status != FBE_STATUS_OK) {
        fbe_base_object_trace((fbe_base_object_t *)encl_mgmt, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s, Error in getting encl location, ObjectId %d, status 0x%X.\n",
                              __FUNCTION__, object_id, status);

        /* Need to return here because of failing to get the enclosure location. */
        return status;
    }

    /*In AR616891,Voyager enclosure added twice, since it has multiple enclosure objects. So add a check here before add new enclosure.*/
    status = fbe_encl_mgmt_get_encl_info_by_location(encl_mgmt, &location, &pEnclInfo);
    if (status == FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)encl_mgmt, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s, Bus %d Enclosure %d is already created, needn't to create again.\n",
                              __FUNCTION__, location.bus, location.enclosure);
        return status;
    }

    /* Add the new enclosure even if the enclosure has any fault.
     * The fault will be handled afterwards.
     */
    status = fbe_encl_mgmt_add_new_encl(encl_mgmt, object_id, &location, &pEnclInfo);
                                        
    if (status != FBE_STATUS_OK) 
    {
        fbe_base_object_trace((fbe_base_object_t *)encl_mgmt, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "Encl(%d_%d), Error in adding new encl, status 0x%X.\n",
                              location.bus, location.enclosure, status);

        /* Need to return here because the enclosure could not be added. */
        return status;
    }

    status = fbe_encl_mgmt_new_encl_check_basic_fault(encl_mgmt, object_id, pEnclInfo);
                                        
    if (status != FBE_STATUS_OK) 
    {
        fbe_base_object_trace((fbe_base_object_t *)encl_mgmt, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "Encl(%d_%d) has basic fault, do not continue processing, status 0x%X.\n",
                              location.bus, location.enclosure, status);

        /* If the enclosure has the basic fault, we can not continue. */
        return FBE_STATUS_OK;
    }

    status = fbe_encl_mgmt_new_encl_populate_attr(encl_mgmt, object_id, pEnclInfo);
    if(status != FBE_STATUS_OK) 
    {
         fbe_base_object_trace((fbe_base_object_t *)encl_mgmt, 
                                  FBE_TRACE_LEVEL_WARNING,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "Encl(%d_%d), Error in populating attr, status 0x%X.\n",
                                  location.bus, location.enclosure, status);
    }

    status = fbe_encl_mgmt_new_encl_handle_other_fault(encl_mgmt, object_id, pEnclInfo);
                                        
    if (status != FBE_STATUS_OK) 
    {
        fbe_base_object_trace((fbe_base_object_t *)encl_mgmt, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "Encl(%d_%d), Error in handling fault, status 0x%X.\n",
                              location.bus, location.enclosure, status);

    }

    /* Process Enclosure Status
     */
    status = fbe_encl_mgmt_process_enclosure_status(encl_mgmt,
                                                    object_id, 
                                                    &(pEnclInfo->location));

    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)encl_mgmt, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "Encl(%d_%d), Error in processing status, status 0x%X.\n",
                              location.bus,
                              location.enclosure,
                              status);

    }

    if(encl_mgmt->base_environment.base_object.lifecycle.state == FBE_LIFECYCLE_STATE_READY)
    {
        // if we've made it to the ready state check for overlimit enclosures.
        fbe_encl_mgmt_check_fru_limit(encl_mgmt, &location);
    }

    /* Initiate the midplane resume prom read. */
    status = fbe_encl_mgmt_initiate_resume_prom_read(encl_mgmt,
                                                     FBE_DEVICE_TYPE_ENCLOSURE,
                                                     &(pEnclInfo->location));
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)encl_mgmt,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "RP: Midplane(%d_%d), Failed to initiate midplane resume read, status 0x%X.\n",
                              location.bus,
                              location.enclosure,
                              status);
    }

    /* Initiate the Drive Midplane resume prom read if this enclosure has drive midplane */

    if (pEnclInfo->driveMidplaneCount > 0)
    {
        status = fbe_encl_mgmt_initiate_resume_prom_read(encl_mgmt,
                                                     FBE_DEVICE_TYPE_DRIVE_MIDPLANE,
                                                     &(pEnclInfo->location));
        if(status != FBE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t *)encl_mgmt,
                                  FBE_TRACE_LEVEL_WARNING,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "RP: Drive Midplane(%d_%d), Failed to initiate drive midplane resume read, status 0x%X.\n",
                                  location.bus,
                                  location.enclosure,
                                  status);
        } 
      }    

    /* Process Enclosure Fault Led Status
     */
    status = fbe_encl_mgmt_process_encl_fault_led_status(encl_mgmt,
                                                    object_id, 
                                                    &(pEnclInfo->location));
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)encl_mgmt, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s Encl(%d_%d) Error in processing enclFaultLedStatus, status 0x%X.\n",
                              __FUNCTION__, 
                              location.bus,
                              location.enclosure,
                              status);
    }

    /* Process LCC Status
     */
    for(slot = 0; slot < pEnclInfo->lccCount; slot ++)
    {
        location.bus = pEnclInfo->location.bus;
        location.enclosure = pEnclInfo->location.enclosure;
        location.slot = slot;
        
        status = fbe_encl_mgmt_process_lcc_status(encl_mgmt, object_id, &location);
        if(status != FBE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t *)encl_mgmt, 
                                  FBE_TRACE_LEVEL_WARNING,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s LCC(%d_%d_%d) Error in processing status, status 0x%X.\n",
                                  __FUNCTION__, 
                                  location.bus,
                                  location.enclosure,
                                  location.slot,
                                  status);
        }
    }
  

    /* Process Connector Status
     */
    for(slot = 0; slot < pEnclInfo->connectorCount; slot ++)
    {
        location.bus = pEnclInfo->location.bus;
        location.enclosure = pEnclInfo->location.enclosure;
        location.slot = slot;
        
        status = fbe_encl_mgmt_process_connector_status(encl_mgmt, object_id, &location);
        if(status != FBE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t *)encl_mgmt, 
                                  FBE_TRACE_LEVEL_WARNING,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s Connector(%d_%d_%d) Error in processing status, status 0x%X.\n",
                                  __FUNCTION__, 
                                  location.bus,
                                  location.enclosure,
                                  location.slot,
                                  status);
        }
    }

    // if non ssc encl, we're done
    for(slot = 0; slot < pEnclInfo->sscCount; slot ++)
    {
        location.bus = pEnclInfo->location.bus;
        location.enclosure = pEnclInfo->location.enclosure;
        location.slot = slot;

        status = fbe_encl_mgmt_process_ssc_status(encl_mgmt, object_id, &location);
        if (status != FBE_STATUS_OK) 
        {
            fbe_base_object_trace((fbe_base_object_t *)encl_mgmt, 
                                  FBE_TRACE_LEVEL_WARNING,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s SSC(%d_%d_%d) Error in processing status, status 0x%X.\n",
                                  __FUNCTION__, 
                                  location.bus,
                                  location.enclosure,
                                  location.slot,
                                  status);
            
        }
    }

    /*
     * Set the enclosure display to the current bus and position.
     */
    if(!pEnclInfo->processorEncl)
    {
        status = fbe_encl_mgmt_process_display_status(encl_mgmt, object_id, &location);
        if(status != FBE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t *)encl_mgmt, 
                                  FBE_TRACE_LEVEL_WARNING,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s Encl(%d_%d) Error in setting the enclosure display, status 0x%X.\n",
                                  __FUNCTION__, 
                                  location.bus,
                                  location.enclosure,
                                  status);
        }
    }

    /* Send the notification for enclosure state change*/
    fbe_base_environment_send_data_change_notification((fbe_base_environment_t*) encl_mgmt,
                                                       FBE_CLASS_ID_ENCL_MGMT,
                                                       FBE_DEVICE_TYPE_ENCLOSURE,
                                                       FBE_DEVICE_DATA_TYPE_GENERIC_INFO,
                                                       &pEnclInfo->location);

    return FBE_STATUS_OK;
}

/*!**************************************************************
 * fbe_encl_mgmt_handle_new_subencl()
 ****************************************************************
 * @brief
 *  This function handles the new subenclsoure. 
 *
 * @param pEnclMgmt - This is the object handle, or in our
 * case the encl_mgmt.
 * @param subEnclObjectId - Object ID of the subenclosure
 * 
 * @return fbe_status_t - FBE Status
 *
 * @author
 *  03-Feb-2012: PHE - Created. 
 *
 ****************************************************************/
static fbe_status_t 
fbe_encl_mgmt_handle_new_subencl(fbe_encl_mgmt_t * pEnclMgmt, fbe_object_id_t subEnclObjectId)
{
    fbe_encl_info_t                  * pEnclInfo = NULL;
    fbe_sub_encl_info_t              * pSubEnclInfo = NULL;
    fbe_device_physical_location_t     enclLocation = {0};
    fbe_device_physical_location_t   * pSubEnclLocation = NULL;
    fbe_device_physical_location_t     subEnclLccLocation = {0};
    fbe_u32_t                          driveSlotCount = 0;
    fbe_component_id_t                 componentId;
    fbe_status_t                       status = FBE_STATUS_OK;
             

    /* Get subencl bus and enclosure */
    status = fbe_base_env_get_and_adjust_encl_location((fbe_base_environment_t *)pEnclMgmt, 
                                                       subEnclObjectId, 
                                                       &enclLocation);
    if (status != FBE_STATUS_OK) {
        fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s Failed to get encl location for subEnclObjectId 0x%X status 0x%X.\n",
                              __FUNCTION__, subEnclObjectId, status);

        return status;
    }

    /* Get the containing enclosure info. */
    status = fbe_encl_mgmt_get_encl_info_by_location(pEnclMgmt, &enclLocation, &pEnclInfo);

    if (status != FBE_STATUS_OK) 
    {
        /* The enclosure was not initialized yet.  
         * When the enclosure gets initialized, 
         * it will call fbe_encl_mgmt_process_lcc_status anyway.
         * Just return FBE_STATUS_OK here.
         */
        fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt, 
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s, encl %d_%d, enclInfo not populated, ignore the change.\n",
                                  __FUNCTION__, enclLocation.bus, enclLocation.enclosure);

        return FBE_STATUS_OK;
    }

    /* Get subencl component id */
    status = fbe_api_get_object_component_id (subEnclObjectId, &componentId);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s Failed to get componentId for object 0x%X, status 0x%X.\n",
                              __FUNCTION__,
                              subEnclObjectId,
                              status);
        return status;
    }

    status = fbe_encl_mgmt_find_available_subencl_info(pEnclMgmt, pEnclInfo, &pSubEnclInfo);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s Failed to find_available_subencl_info object 0x%X, status 0x%X.\n",
                              __FUNCTION__,
                              subEnclObjectId,
                              status);
        return status;
    }

    pSubEnclInfo->objectId = subEnclObjectId;
    pSubEnclLocation = &pSubEnclInfo->location;
    pSubEnclLocation->bus = enclLocation.bus;
    pSubEnclLocation->enclosure = enclLocation.enclosure;
    pSubEnclLocation->componentId = componentId;

    pEnclInfo->currSubEnclCount ++;

    fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s,Encl %d_%d.%d,subEnclObjectId 0x%X Added,currSubEnclCount %d.\n",
                              __FUNCTION__,
                              pSubEnclLocation->bus, 
                              pSubEnclLocation->enclosure,
                              pSubEnclLocation->componentId,
                              pSubEnclInfo->objectId,
                              pEnclInfo->currSubEnclCount);

    /* Get subencl slot count */
    status = fbe_api_enclosure_get_slot_count(subEnclObjectId, &driveSlotCount);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "Encl(%d_%d.%d), %s Error in getting lcc drive slot count, status 0x%x.\n",
                              pSubEnclLocation->bus, 
                              pSubEnclLocation->enclosure,
                              pSubEnclLocation->componentId,
                              __FUNCTION__,
                              status);
        return status;
    }

    subEnclLccLocation = *pSubEnclLocation;
    status = fbe_encl_mgmt_process_subencl_all_lcc_status(pEnclMgmt, 
                                                          pEnclInfo->subEnclLccStartSlot, 
                                                          subEnclObjectId, 
                                                          pSubEnclLocation);

    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "Encl(%d_%d.%d), %s Error in process_subencl_all_lcc_status, status 0x%x.\n",
                              pSubEnclLocation->bus, 
                              pSubEnclLocation->enclosure,
                              pSubEnclLocation->componentId,
                              __FUNCTION__,
                              status);
        return status;
    }


    return FBE_STATUS_OK;
}

/*!**************************************************************
 * @fn fbe_encl_mgmt_process_subencl_all_lcc_status(fbe_encl_mgmt_t * pEnclMgmt,
 *                         fbe_object_id_t subEnclObjectId,
 *                         fbe_device_physical_location_t * pSubEnclLocation)
 ****************************************************************
 * @brief
 *  This function refresh the device status. We might filter the status change
 *  during the upgrade so we would like to refresh the device status when
 *  the upgrade is done.
 *
 * @param pEnclMgmt - The pointer to encl_mgmt.
 * @param subEnclLccStartSlot - The subEnclLccStartSlot(2 for Voyager and 0 for others).
 * @param subEnclObjectId - subEnclObjectId
 * @param pSubEnclLocation - The pointer to the physical location of the subEncl.
 *
 * @return fbe_status_t
 *
 * @version
 *  02-May-2013: PHE - Created.
 *
 ****************************************************************/
fbe_status_t fbe_encl_mgmt_process_subencl_all_lcc_status(fbe_encl_mgmt_t * pEnclMgmt,
                                             fbe_u32_t subEnclLccStartSlot,
                                             fbe_object_id_t subEnclObjectId,
                                             fbe_device_physical_location_t * pSubEnclLocation)
{
    fbe_u32_t                         subEnclLccCount = 0;
    fbe_device_physical_location_t    tempSubEnclLocation = {0};
    fbe_status_t                      status = FBE_STATUS_OK;

    tempSubEnclLocation = *pSubEnclLocation;

    for(subEnclLccCount = 0; subEnclLccCount < FBE_ESP_MAX_LCCS_PER_SUBENCL; subEnclLccCount++)
    {
        tempSubEnclLocation.slot = subEnclLccStartSlot + subEnclLccCount; 

        fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt, 
                              FBE_TRACE_LEVEL_DEBUG_MEDIUM,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "Processing Encl(%d_%d.%d)LCC%d, subEnclObjectId:0x%X.\n",
                              tempSubEnclLocation.bus,
                              tempSubEnclLocation.enclosure,
                              tempSubEnclLocation.componentId,
                              tempSubEnclLocation.slot,
                              subEnclObjectId);

        status = fbe_encl_mgmt_process_subencl_lcc_status(pEnclMgmt, 
                                                          subEnclObjectId, 
                                                          &tempSubEnclLocation);
        if(status != FBE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt, 
                                  FBE_TRACE_LEVEL_WARNING,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s Encl(%d_%d.%d)LCC%d process_subencl_lcc_status failed,status:0x%X.\n",
                                  __FUNCTION__, 
                                  tempSubEnclLocation.bus,
                                  tempSubEnclLocation.enclosure,
                                  tempSubEnclLocation.componentId,
                                  tempSubEnclLocation.slot,
                                  status);
            continue;
        }
    }

    return FBE_STATUS_OK;
}


/*!**************************************************************
 * fbe_encl_mgmt_check_overlimit_enclosures()
 ****************************************************************
 * @brief
 *  This function loops through all enclosures counting their
 *  drive slots and comparing them against the platform limit. 
 *
 * @param base_object - This is the object handle.
 * 
 * @return fbe_status_t - FBE Status
 *
 * @author
 *  21-Feb-2013: bphilbin - Created. 
 *
 ****************************************************************/
static fbe_status_t 
fbe_encl_mgmt_check_overlimit_enclosures(fbe_base_object_t * base_object)
{
    fbe_encl_mgmt_t *pEnclMgmt = (fbe_encl_mgmt_t *)base_object;
    fbe_u32_t drive_slot;
    fbe_encl_info_t * pEnclInfo = NULL;
    fbe_object_id_t drive_object_id;
    fbe_u32_t bus = 0;
    fbe_u32_t enclosure_num = 0;
    fbe_device_physical_location_t location = {0};
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_base_enclosure_fail_encl_cmd_t fail_cmd = {0};

    fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s entry.\n",
                              __FUNCTION__);


    
    for(bus = 0; bus < FBE_PHYSICAL_BUS_COUNT; bus++) 
    {
        for(enclosure_num = 0; enclosure_num < FBE_ENCLOSURES_PER_BUS; enclosure_num++)
        {
            location.bus = bus;
            location.enclosure = enclosure_num;
            status = fbe_encl_mgmt_get_encl_info_by_location(pEnclMgmt, &location, &pEnclInfo);
            if(status != FBE_STATUS_OK)
            {
                fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt, 
                                          FBE_TRACE_LEVEL_DEBUG_LOW,
                                          FBE_TRACE_MESSAGE_ID_INFO,
                                          "%s enclosure (%d_%d) not found.\n",
                                          __FUNCTION__, bus, enclosure_num);
                continue;

            }
            if( (pEnclInfo->driveSlotCount > 0) && ( (pEnclMgmt->total_drive_slot_count + pEnclInfo->driveSlotCount) > pEnclMgmt->platformFruLimit))
            {
                 char deviceStr[FBE_DEVICE_STRING_LENGTH] = {0};

                //enclosure is overlimit
                for(drive_slot=0;drive_slot<pEnclInfo->driveSlotCount;drive_slot++)
                {
                    fbe_api_get_physical_drive_object_id_by_location(pEnclInfo->location.bus,pEnclInfo->location.enclosure,
                                                                     drive_slot, &drive_object_id);
                    if(drive_object_id != FBE_OBJECT_ID_INVALID)
                    {
                         fbe_port_number_t port;
                        fbe_enclosure_number_t enclosure;
                        fbe_enclosure_slot_number_t slot;
                        fbe_device_physical_location_t phys_location = {0};
                        fbe_physical_drive_information_t drive_info;                

                        /* attempt to get b_e_s and drive_info from PDO object ID. */
                        fbe_api_get_object_port_number(drive_object_id, &port);
                        fbe_api_get_object_enclosure_number(drive_object_id, &enclosure);
                        fbe_api_get_object_drive_number(drive_object_id, &slot);

                        phys_location.bus = (fbe_u8_t)port;
                        phys_location.enclosure = (fbe_u8_t)enclosure;
                        phys_location.slot = (fbe_u8_t)slot;

                        fbe_api_physical_drive_get_drive_information(drive_object_id, &drive_info, FBE_PACKET_FLAG_NO_ATTRIB);

                        fbe_base_env_create_device_string(FBE_DEVICE_TYPE_DRIVE,        
                                                          &phys_location, 
                                                          &deviceStr[0],
                                                          FBE_DEVICE_STRING_LENGTH);               

                        fbe_event_log_write(ESP_ERROR_DRIVE_PLATFORM_LIMITS_EXCEEDED, NULL, 0, "%s %d %s %s %s %s",
                                  &deviceStr[0], pEnclMgmt->platformFruLimit, drive_info.product_serial_num, drive_info.tla_part_number, drive_info.product_rev, "");                             
                    }
                }
    
                 status = fbe_base_env_create_device_string(FBE_DEVICE_TYPE_ENCLOSURE,
                                                           &pEnclInfo->location,
                                                           &deviceStr[0],
                                                           FBE_DEVICE_STRING_LENGTH);
                if(status != FBE_STATUS_OK)
                {
                    fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                          "%s, Failed to create device string.\n", __FUNCTION__);
                }

                status = fbe_event_log_write(ESP_ERROR_ENCL_EXCEEDED_MAX, NULL, 0, "%s", &deviceStr[0]);

                if(status != FBE_STATUS_OK)
                {
                    fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt, FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                          "%s encl_mgmt_encl_log_event failed, status 0x%X.\n", __FUNCTION__, status);
                }
    
                    
                fail_cmd.led_reason = FBE_ENCL_FAULT_LED_EXCEEDED_MAX;
                fail_cmd.flt_symptom = FBE_ENCL_FLTSYMPT_EXCEEDS_MAX_LIMITS;
                fbe_api_enclosure_set_enclosure_failed(pEnclInfo->object_id, &fail_cmd);

                fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                      "EnclMgmt: (%d_%d) overlimit: enclosure exceeds platform limits. slot count %d>%d\n",
                                      pEnclInfo->location.bus, pEnclInfo->location.enclosure, (pEnclMgmt->total_drive_slot_count + pEnclInfo->driveSlotCount), pEnclMgmt->platformFruLimit);                
                
                
            }
            else
            {
                //add this enclosure to the overall count
                fbe_spinlock_lock(&pEnclInfo->encl_info_lock);
                pEnclMgmt->total_drive_slot_count += pEnclInfo->driveSlotCount;
                fbe_spinlock_unlock(&pEnclInfo->encl_info_lock);
            }
        }
    }
    return FBE_STATUS_OK;
}


/*!**************************************************************
 * fbe_encl_mgmt_new_encl_check_basic_fault()
 ****************************************************************
 * @brief
 *  This function checks the enclosure object lifecycle state.
 *  If the enclosure object lifecycle state is failed,
 *  we can NOT continue processing status because the enclosure object
 *  might NOT be in the state to provide any information or accept
 *  the write commands. If ESP sent the read, it could get the bogus info.
 *  If ESP sent the write, it may cause the panic in the physical package.
 *
 * @param pEnclMgmt - This is the object handle, or in our
 * case the encl_mgmt.
 * @param objectId - Object ID of the enclosure
 * @param pEnclInfo - enclosure info pointer
 *
 * @return fbe_status_t -
 *       FBE_STATUS_GENERIC_FAILURE - the enclosure has the basic fault.
 *       FBE_STATUS_OK - the enclosure does not have the basic fault.
 * 
 * @author
 *  10-Oct-2012: LL - Created.
 *  15-Feb-2013: PHE - Changed the function name.
 *
 ****************************************************************/
static fbe_status_t
fbe_encl_mgmt_new_encl_check_basic_fault(fbe_encl_mgmt_t * pEnclMgmt,
                             fbe_object_id_t objectId,
                             fbe_encl_info_t * pEnclInfo)
{
    fbe_status_t                                    status = FBE_STATUS_OK;
    fbe_lifecycle_state_t                           lifecycleState;
    fbe_esp_encl_fault_sym_t                        enclFaultSymptom = FBE_ESP_ENCL_FAULT_SYM_NO_FAULT;
    fbe_enclosure_fault_t                           enclosure_fault_info = {0};
    fbe_topology_object_type_t                      objectType;
    fbe_u8_t                                        deviceStr[FBE_DEVICE_STRING_LENGTH];

    status = fbe_api_get_object_type(objectId, &objectType, FBE_PACKAGE_ID_PHYSICAL);
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_INFO,
                       "%s: cannot get Object Type for objectId 0x%x, status %d\n",
                       __FUNCTION__, objectId, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* only enclosure object may have cabling faults */
    if(objectType != FBE_TOPOLOGY_OBJECT_TYPE_ENCLOSURE)
    {
        return FBE_STATUS_OK;
    }

    status = fbe_base_env_create_device_string(FBE_DEVICE_TYPE_ENCLOSURE,
                                               &pEnclInfo->location,
                                               &deviceStr[0],
                                               FBE_DEVICE_STRING_LENGTH);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s, Failed to create device string.\n",
                              __FUNCTION__);
    }
    
    /* lifecycle state fail */
    status = fbe_api_get_object_lifecycle_state(objectId, &lifecycleState, FBE_PACKAGE_ID_PHYSICAL);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "Encl(%d_%d), %s Error in getting lifecycle state, status: 0x%X.\n",
                              pEnclInfo->location.bus, pEnclInfo->location.enclosure, __FUNCTION__, status);
        return status;
    }

    if(lifecycleState == FBE_LIFECYCLE_STATE_FAIL)
    {
        status = fbe_api_enclosure_get_fault_info(objectId, &enclosure_fault_info);
        if(status != FBE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt,
                                  FBE_TRACE_LEVEL_WARNING,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "Encl(%d_%d), %s Error in getting fault info, status: 0x%X.\n",
                                  pEnclInfo->location.bus, pEnclInfo->location.enclosure, __FUNCTION__, status);
            return status;
        }

        /* Overlimit cabling */
        if(enclosure_fault_info.faultSymptom == FBE_ENCL_FLTSYMPT_MINIPORT_FAULT_ENCL
                && enclosure_fault_info.additional_status == FBE_PAYLOAD_SMP_ELEMENT_STATUS_EXT_TOO_MANY_END_DEVICES)
        {
            enclFaultSymptom = FBE_ESP_ENCL_FAULT_SYM_EXCEEDED_MAX;

            fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt,
                                      FBE_TRACE_LEVEL_WARNING,
                                      FBE_TRACE_MESSAGE_ID_INFO,
                                      "Encl(%d_%d), has exceeded the maximum drives per bus.\n",
                                      pEnclInfo->location.bus, pEnclInfo->location.enclosure);

            status = fbe_event_log_write(ESP_ERROR_ENCL_EXCEEDED_MAX,
                                         NULL,
                                         0,
                                         "%s",
                                         &deviceStr[0]);
            if(status != FBE_STATUS_OK)
            {
                fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt,
                                      FBE_TRACE_LEVEL_WARNING,
                                      FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                      "%s encl_mgmt_encl_log_event failed, status 0x%X.\n",
                                      __FUNCTION__, status);
            }
        }
        else if(enclosure_fault_info.faultSymptom == FBE_ENCL_FLTSYMPT_EXCEEDS_MAX_LIMITS)
        {
            enclFaultSymptom = FBE_ESP_ENCL_FAULT_SYM_EXCEEDED_MAX;

            fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt,
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "Encl(%d_%d), enclosure state fail, fault symptom:%s\n",
                                  pEnclInfo->location.bus,
                                  pEnclInfo->location.enclosure,
                                  fbe_encl_mgmt_decode_encl_fault_symptom(enclFaultSymptom));
    
            status = fbe_event_log_write(ESP_ERROR_ENCL_EXCEEDED_MAX,
                                                     NULL,
                                                     0,
                                                     "%s",
                                                     &deviceStr[0]);
            if(status != FBE_STATUS_OK)
            {
                fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt,
                                      FBE_TRACE_LEVEL_WARNING,
                                      FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                      "%s encl_mgmt_encl_log_event failed, status 0x%X.\n",
                                      __FUNCTION__, status);
            }
    
            fbe_spinlock_lock(&pEnclInfo->encl_info_lock);
            pEnclInfo->enclState = FBE_ESP_ENCL_STATE_FAILED;
            pEnclInfo->enclFaultSymptom = enclFaultSymptom;
            fbe_spinlock_unlock(&pEnclInfo->encl_info_lock);
            return status;
        }
        else
        {
            enclFaultSymptom = FBE_ESP_ENCL_FAULT_SYM_LIFECYCLE_STATE_FAIL;
        }

        fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "Encl(%d_%d), enclosure state fail, fault symptom:%s\n",
                              pEnclInfo->location.bus,
                              pEnclInfo->location.enclosure,
                              fbe_encl_mgmt_decode_encl_fault_symptom(enclFaultSymptom));


        fbe_spinlock_lock(&pEnclInfo->encl_info_lock);
        pEnclInfo->enclState = FBE_ESP_ENCL_STATE_FAILED;
        pEnclInfo->enclFaultSymptom = enclFaultSymptom;
        fbe_spinlock_unlock(&pEnclInfo->encl_info_lock);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    return FBE_STATUS_OK;
}



/*!**************************************************************
 * fbe_encl_mgmt_new_encl_check_other_fault()
 ****************************************************************
 * @brief
 *  This function checks the faults other than the basic fault.
 *  We can continue processing status even if the enclosure got these faults.
 *
 * @param pEnclMgmt - This is the object handle, or in our
 * case the encl_mgmt.
 * @param objectId - Object ID of the enclosure
 * @param pEnclInfo - enclosure info pointer
 *
 * @return fbe_status_t - FBE Status
 *
 * @author
 *  10-Oct-2012: LL - Created.
 *  15-Feb-2013: PHE - Changed the function name.
 *
 ****************************************************************/
static fbe_status_t
fbe_encl_mgmt_new_encl_check_other_fault(fbe_encl_mgmt_t * pEnclMgmt,
                             fbe_object_id_t objectId,
                             fbe_encl_info_t * pEnclInfo)
{
    fbe_status_t                                    status = FBE_STATUS_OK;
    fbe_esp_encl_fault_sym_t                        enclFaultSymptom = FBE_ESP_ENCL_FAULT_SYM_NO_FAULT;
    fbe_encl_mgmt_encl_info_cmi_packet_t            enclMsgPkt = {0};
    fbe_topology_object_type_t                      objectType;
    fbe_u8_t                                        deviceStr[FBE_DEVICE_STRING_LENGTH];
    fbe_u32_t                                       maxEnclCntPerBus;
    fbe_u32_t                                       enclIndex, enclCountOnBus = 0;
    fbe_object_id_t                                 boardObjectId;
    fbe_board_mgmt_platform_info_t                  current_platform_info;
    fbe_esp_encl_type_t                             enclType = FBE_ESP_ENCL_TYPE_INVALID;

    status = fbe_api_get_object_type(objectId, &objectType, FBE_PACKAGE_ID_PHYSICAL);
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_INFO,
                       "%s: cannot get Object Type for objectId 0x%x, status %d\n",
                       __FUNCTION__, objectId, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* only enclosure object may have cabling faults */
    if(objectType != FBE_TOPOLOGY_OBJECT_TYPE_ENCLOSURE)
    {
        return FBE_STATUS_OK;
    }

    status = fbe_base_env_create_device_string(FBE_DEVICE_TYPE_ENCLOSURE,
                                               &pEnclInfo->location,
                                               &deviceStr[0],
                                               FBE_DEVICE_STRING_LENGTH);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s, Failed to create device string.\n",
                              __FUNCTION__);
    }

    maxEnclCntPerBus = fbe_environment_limit_get_platform_max_encl_count_per_bus();

    for(enclIndex = 0; enclIndex < FBE_ESP_MAX_ENCL_COUNT; enclIndex ++)
    {
        if((pEnclMgmt->encl_info[enclIndex]->object_id != FBE_OBJECT_ID_INVALID) &&
           (pEnclMgmt->encl_info[enclIndex]->location.bus == pEnclInfo->location.bus))
        {
            enclCountOnBus ++;
        }
    }

    if(enclCountOnBus > maxEnclCntPerBus)
    {
        enclFaultSymptom = FBE_ESP_ENCL_FAULT_SYM_EXCEEDED_MAX;

        fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "Encl(%d_%d), enclosure state fail, fault symptom:%s\n",
                              pEnclInfo->location.bus,
                              pEnclInfo->location.enclosure,
                              fbe_encl_mgmt_decode_encl_fault_symptom(enclFaultSymptom));

        status = fbe_event_log_write(ESP_ERROR_ENCL_EXCEEDED_MAX,
                                                 NULL,
                                                 0,
                                                 "%s",
                                                 &deviceStr[0]);
        if(status != FBE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt,
                                  FBE_TRACE_LEVEL_WARNING,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s encl_mgmt_encl_log_event failed, status 0x%X.\n",
                                  __FUNCTION__, status);
        }

        fbe_spinlock_lock(&pEnclInfo->encl_info_lock);
        pEnclInfo->enclState = FBE_ESP_ENCL_STATE_FAILED;
        pEnclInfo->enclFaultSymptom = enclFaultSymptom;
        fbe_spinlock_unlock(&pEnclInfo->encl_info_lock);

        return status;
    }
   
    /* The following code is to check whether the enclosure is supported or not by the platform.
     * This needs to be done after the lifecycle state check. 
     * The reason is that if the enclosure is in Failed lifecyle state, it is possible that we don't
     * even know the enclosure type.
     */
    // get platform type from Processor Enclosure
    status = fbe_api_get_board_object_id(&boardObjectId);
    if (status != FBE_STATUS_OK) 
    {
        fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s Error in getting the Board Object ID, status: 0x%X\n",
                              __FUNCTION__, status);
        return status;
    }
    status =  fbe_api_board_get_platform_info(boardObjectId, 
                                              &current_platform_info);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s Error getting Platform Info Status, status: 0x%X\n",
                              __FUNCTION__, status);

        return status;
    }
    
    enclType = pEnclInfo->enclType;

    // verify enclosure type
    if (!fbe_encl_mgmt_isEnclosureSupported(pEnclMgmt, &current_platform_info.hw_platform_info, enclType))
    {
        enclFaultSymptom = FBE_ESP_ENCL_FAULT_SYM_UNSUPPORTED;
        fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "Encl(%d_%d), enclType %d NOT SUPPORTED by platform %d.\n",
                              pEnclInfo->location.bus, pEnclInfo->location.enclosure, 
                              enclType, current_platform_info.hw_platform_info.platformType);
        status = fbe_event_log_write(ESP_ERROR_ENCL_TYPE_NOT_SUPPORTED,
                                     NULL, 0,
                                     "%s %s", 
                                     &deviceStr[0],
                                     fbe_encl_mgmt_get_encl_type_string(enclType));
        fbe_spinlock_lock(&pEnclInfo->encl_info_lock);
        pEnclInfo->enclState = FBE_ESP_ENCL_STATE_FAILED;
        pEnclInfo->enclFaultSymptom = enclFaultSymptom;
        fbe_spinlock_unlock(&pEnclInfo->encl_info_lock);
        return status;
    }
    else
    {
        fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "Encl(%d_%d), enclType %d supported by platform %d.\n",
                              pEnclInfo->location.bus, pEnclInfo->location.enclosure, 
                              enclType, current_platform_info.hw_platform_info.platformType);
    }

    // check if PS types are mixed & fault enclosure if detected
    if (!fbe_encl_mgmt_validEnclPsTypes(pEnclMgmt, pEnclInfo))
    {
        enclFaultSymptom = FBE_ESP_ENCL_FAULT_SYM_PS_TYPE_MIX;
        fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "Encl(%d_%d), mixed PS types detected - fault enclosure.\n",
                              pEnclInfo->location.bus, pEnclInfo->location.enclosure);
        status = fbe_event_log_write(ESP_ERROR_ENCL_TYPE_NOT_SUPPORTED,
                                     NULL, 0,
                                     "%s %s", 
                                     &deviceStr[0],
                                     fbe_encl_mgmt_get_encl_type_string(enclType));
        fbe_spinlock_lock(&pEnclInfo->encl_info_lock);
        pEnclInfo->enclState = FBE_ESP_ENCL_STATE_FAILED;
        pEnclInfo->enclFaultSymptom = enclFaultSymptom;
        fbe_spinlock_unlock(&pEnclInfo->encl_info_lock);
        return status;
    }

    if (pEnclInfo->crossCable) 
    {
        enclFaultSymptom = FBE_ESP_ENCL_FAULT_SYM_CROSS_CABLED;

        fbe_spinlock_lock(&pEnclInfo->encl_info_lock);
        pEnclInfo->enclState = FBE_ESP_ENCL_STATE_FAILED;
        pEnclInfo->enclFaultSymptom = enclFaultSymptom;
        fbe_spinlock_unlock(&pEnclInfo->encl_info_lock);

        fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "Encl(%d_%d), enclosure state fail, fault symptom:%s\n",
                              pEnclInfo->location.bus,
                              pEnclInfo->location.enclosure,
                              fbe_encl_mgmt_decode_encl_fault_symptom(enclFaultSymptom));

        status = fbe_event_log_write(ESP_ERROR_ENCL_CROSS_CABLED,
                                     NULL,
                                     0,
                                     "%s",
                                     &deviceStr[0]);
        if(status != FBE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt,
                                  FBE_TRACE_LEVEL_WARNING,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "Encl(%d_%d), %s encl_mgmt_encl_log_event failed, status 0x%X.\n",
                                  pEnclInfo->location.bus,
                                  pEnclInfo->location.enclosure,
                                  __FUNCTION__, status);
        }

        return status;
    }

    /* Invalid enclosure serial number */
    if(fbe_equal_memory(pEnclInfo->serial_number, "INVALID", sizeof("INVALID")-1) ||
            fbe_equal_memory(pEnclInfo->serial_number, FBE_ESES_ENCL_SN_DEFAULT, sizeof(FBE_ESES_ENCL_SN_DEFAULT)-1))
    {
        enclFaultSymptom = FBE_ESP_ENCL_FAULT_SYM_LCC_INVALID_UID;

        fbe_spinlock_lock(&pEnclInfo->encl_info_lock);
        pEnclInfo->enclState = FBE_ESP_ENCL_STATE_FAILED;
        pEnclInfo->enclFaultSymptom = enclFaultSymptom;
        fbe_spinlock_unlock(&pEnclInfo->encl_info_lock);

        fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "Encl(%d_%d), enclosure state fail, fault symptom:%s\n",
                              pEnclInfo->location.bus,
                              pEnclInfo->location.enclosure,
                              fbe_encl_mgmt_decode_encl_fault_symptom(enclFaultSymptom));

        status = fbe_event_log_write(ESP_ERROR_ENCL_LCC_INVALID_UID,
                                     NULL,
                                     0,
                                     "%s",
                                     &deviceStr[0]);
        if(status != FBE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt,
                                  FBE_TRACE_LEVEL_WARNING,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s encl_mgmt_encl_log_event failed, status 0x%X.\n",
                                  __FUNCTION__, status);
        }

        return status;
    }

    /* Miscabling - Send enclosure info to peer side to determine if it's miscabled. */
    enclMsgPkt.baseCmiMsg.opcode = FBE_BASE_ENV_CMI_MSG_ENCL_CABLING_REQUEST;
    enclMsgPkt.encl_info.location = pEnclInfo->location;
    fbe_copy_memory(enclMsgPkt.encl_info.serial_number, pEnclInfo->serial_number, sizeof(enclMsgPkt.encl_info.serial_number));

    fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt,
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "Encl(%d_%d), sending cabling request to peer.\n",
                          pEnclInfo->location.bus, pEnclInfo->location.enclosure);

    status = fbe_base_environment_send_cmi_message((fbe_base_environment_t *)pEnclMgmt,
                                                   sizeof(fbe_encl_mgmt_encl_info_cmi_packet_t),
                                                   &enclMsgPkt);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s Error sending FBE_BASE_ENV_CMI_MSG_ENCL_CABLING_REQUEST to peer, status: 0x%X\n",
                              __FUNCTION__,
                              status);
    }

    return status;
}

/*!**************************************************************
 * fbe_encl_mgmt_new_encl_handle_other_fault()
 ****************************************************************
 * @brief
 *  This function checks any fault for the newly added enclosure.
 *  It logs the event if the enclosure state changes due to the detected fault.
 *  It also turns on the enclosure fault led if needed.
 *
 * @param encl_mgmt - Enclosure management object handle.
 * @param pEnclInfo - Enclosure info pointer.
 *
 * @return fbe_status_t - FBE Status
 *
 * @author
 *  23-Nov-2012: LL Created.
 *  15-Feb-2013: PHE - Changed the function name.
 *
 ****************************************************************/
static fbe_status_t fbe_encl_mgmt_new_encl_handle_other_fault(fbe_encl_mgmt_t * encl_mgmt,
                                                    fbe_object_id_t objectId,
                                                    fbe_encl_info_t * pEnclInfo)
{
    fbe_status_t                      status = FBE_STATUS_OK;
    fbe_encl_info_t                 * oldEnclInfo = NULL;

    oldEnclInfo = (fbe_encl_info_t *)fbe_base_env_memory_ex_allocate((fbe_base_environment_t *)encl_mgmt, sizeof(fbe_encl_info_t));
    if(oldEnclInfo == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t*)encl_mgmt,
                                    FBE_TRACE_LEVEL_WARNING,
                                    FBE_TRACE_MESSAGE_ID_INFO,
                                    "Encl(%d_%d) encl_info memory allocation failed.\n",
                                    pEnclInfo->location.bus,
                                    pEnclInfo->location.enclosure);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* save old enclosure info */
    fbe_copy_memory(oldEnclInfo, pEnclInfo, sizeof(fbe_encl_info_t));

    /* Check any fault on the new added enclosure. */
    status = fbe_encl_mgmt_new_encl_check_other_fault(encl_mgmt, objectId, pEnclInfo);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)encl_mgmt,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "Encl(%d_%d), %s Error in populating enclosure state, status 0x%X.\n",
                              pEnclInfo->location.bus, pEnclInfo->location.enclosure, __FUNCTION__, status);
    }

    /* Trace and Log event. */
    status = fbe_encl_mgmt_encl_log_event(encl_mgmt,
                                        &pEnclInfo->location,
                                        pEnclInfo,
                                        oldEnclInfo);

    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)encl_mgmt,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s encl_mgmt_encl_log_event failed, status 0x%X.\n",
                              __FUNCTION__, status);
    }

    fbe_base_env_memory_ex_release((fbe_base_environment_t *)encl_mgmt, oldEnclInfo);

    status = fbe_encl_mgmt_update_encl_fault_led(encl_mgmt,
                                                &pEnclInfo->location,
                                                FBE_ENCL_FAULT_LED_ENCL_LIFECYCLE_FAIL
                                                | FBE_ENCL_FAULT_LED_LCC_CABLING_FLT
                                                | FBE_ENCL_FAULT_LED_EXCEEDED_MAX);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)encl_mgmt,
                          FBE_TRACE_LEVEL_WARNING,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "Encl(%d_%d), LIFECYCLE_FAIL/CABLING_FLT/EXCEEDED_MAX, update_encl_fault_led failed, status: 0x%X.\n",
                          pEnclInfo->location.bus,
                          pEnclInfo->location.enclosure,
                          status);
    }

    return status;
}

/*!**************************************************************
 * fbe_encl_mgmt_new_encl_populate_attr()
 ****************************************************************
 * @brief
 *  This function gets called while having a new enclsoure. 
 *  It populates the enclosure attributes for the new enclosure.
 *
 * @param pEnclMgmt - This is the object handle.
 * @param objectId - 
 * @param pEnclInfo - The pointer to the enclosure information
 *                              
 * @return fbe_status_t - FBE Status
 *
 * @author
 *  23-Feb-2010:  Created. Ashok Tamilarasan
 *  13-Jan-2011: PHE - Added the handling of LCC count and FAN count.
 *
 ****************************************************************/
static fbe_status_t 
fbe_encl_mgmt_new_encl_populate_attr(fbe_encl_mgmt_t * pEnclMgmt, 
                                     fbe_object_id_t objectId,
                                     fbe_encl_info_t * pEnclInfo)
{
    fbe_device_physical_location_t              location = {0};
    fbe_esp_encl_type_t                         enclType = FBE_ESP_ENCL_TYPE_INVALID;
    fbe_u32_t                                   lccCount = 0;
    fbe_u32_t                                   psCount = 0;
    fbe_u32_t                                   fanCount = 0, dpeBoardObjFanCount = 0;
    fbe_u32_t                                   driveSlotCount = 0;
    fbe_u32_t                                   connectorCount = 0;
    fbe_u32_t                                   bbuCount = 0;
    fbe_u32_t                                   spsCount = 0;
    fbe_u32_t                                   spCount = 0;
    fbe_u32_t                                   subEnclCount = 0;
    fbe_u8_t                                    driveMidplaneCount = 0;
    fbe_u8_t                                    sscCount = 0;
    fbe_status_t                                status = FBE_STATUS_OK;
    fbe_bool_t                                  processorEncl = FALSE;
    fbe_object_id_t                             tempObjectId, dpeEnclObjId = FBE_OBJECT_ID_INVALID;
    fbe_u32_t                                   enclSide = 0;
    SP_ID                                       localSp = SP_ID_MAX;
    fbe_bool_t                                  crossCable = FBE_FALSE;
    HW_ENCLOSURE_TYPE                           processorEnclType = INVALID_ENCLOSURE_TYPE;
    fbe_base_object_mgmt_get_enclosure_basic_info_t enclosure_info = {0};
    fbe_u32_t                                   localDimmCount = 0;
    fbe_u32_t                                   dimmCount = 0;
    fbe_u32_t                                   localSsdCount = 0;
    fbe_u32_t                                   ssdCount = 0;
    fbe_object_id_t                             boardObjectId = FBE_OBJECT_ID_INVALID;

    location = pEnclInfo->location;

    status = fbe_base_env_is_processor_encl((fbe_base_environment_t *)pEnclMgmt, 
                                                 &location,
                                                 &processorEncl);
    if (status != FBE_STATUS_OK) {
        fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "Encl(%d_%d), Error in checking whether it is PE, status 0x%x.\n",
                              location.bus, location.enclosure, status);

        return status;
    }

    if(processorEncl) 
    {
        if (pEnclMgmt->base_environment.isSingleSpSystem == TRUE)
        {
            spCount = SP_COUNT_SINGLE;
        }
        else
        {
            spCount = SP_ID_MAX;
        }

        crossCable = FBE_FALSE;
    }
    else
    {
        spCount = 0;
        status = fbe_api_enclosure_get_encl_side(objectId, &enclSide);
        if(status != FBE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt, 
                                  FBE_TRACE_LEVEL_WARNING,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "Encl(%d_%d), %s Error in getting encl side, status: 0x%X.\n",
                                  location.bus, location.enclosure, __FUNCTION__, status);
        }
        else
        {
            fbe_base_env_get_local_sp_id((fbe_base_environment_t *)pEnclMgmt, &localSp);
            crossCable = (localSp == enclSide)? FBE_FALSE : FBE_TRUE;
        }
    }

    status = fbe_encl_mgmt_get_encl_type(pEnclMgmt, objectId, &enclType);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "Encl(%d_%d), %s Error in getting encl type, status: 0x%X.\n",
                              location.bus, location.enclosure, __FUNCTION__, status);
    }

    /* Get DIMM Count */
    if(processorEncl) 
    {
        status = fbe_api_get_board_object_id(&boardObjectId);
        if (status != FBE_STATUS_OK) 
        {
            fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt,  
                                  FBE_TRACE_LEVEL_WARNING,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s Error in getting the Board Object ID, status: 0x%X.\n",
                                  __FUNCTION__, status);
    
            localDimmCount = 0;
            dimmCount = 0;
        }
        else
        {
            // get local DIMM count first.
            status = fbe_api_board_get_dimm_count(boardObjectId, &localDimmCount);
            if(status != FBE_STATUS_OK)
            {
                fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt,
                                      FBE_TRACE_LEVEL_WARNING,
                                      FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                      "%s Error getting local DIMM count, status: 0x%X.\n",
                                      __FUNCTION__, status);
                dimmCount = 0;
            }
            else
            {
                dimmCount = spCount * localDimmCount;
            }
        }
    }
    else
    {
        dimmCount = 0;
    }

    /* Get SSD Count */
    if(processorEncl) 
    {
        status = fbe_api_get_board_object_id(&boardObjectId);
        if (status != FBE_STATUS_OK) 
        {
            fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt,  
                                  FBE_TRACE_LEVEL_WARNING,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s Error in getting the Board Object ID, status: 0x%X.\n",
                                  __FUNCTION__, status);
    
            localSsdCount = 0;
            ssdCount = 0;
        }
        else
        {
            // get local SSD count first.
            status = fbe_api_board_get_ssd_count(boardObjectId, &localSsdCount);
            if(status != FBE_STATUS_OK)
            {
                fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt,
                                      FBE_TRACE_LEVEL_WARNING,
                                      FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                      "%s Error getting local SSD count, status: 0x%X.\n",
                                      __FUNCTION__, status);
                ssdCount = 0;
            }
            else
            {
                ssdCount = spCount * localSsdCount;
            }
        }
    }
    else
    {
        ssdCount = 0;
    }

    /* We only deal with stand alone LCC in encl_mgmt */
    /* DPE like Jetfire have LCC info in physical package, but we do not deal with it here */
    /* We deal with it as part of Base Module in module_mgmt */
    status = fbe_api_enclosure_get_stand_alone_lcc_count(objectId, &lccCount);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "Encl(%d_%d), %s Error in getting LCC count, status 0x%x.\n",
                              location.bus, location.enclosure, __FUNCTION__, status);
    }

    status = fbe_api_enclosure_get_slot_count(objectId, &driveSlotCount);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s Encl(%d_%d), Error in getting drive slot count, status 0x%x.\n",
                              __FUNCTION__, location.bus, location.enclosure, status);
    }

    status = fbe_api_enclosure_get_connector_count(objectId, &connectorCount);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "Encl(%d_%d), %s Error in getting connectorCount, status 0x%x.\n",
                              location.bus, location.enclosure, __FUNCTION__, status);
    }

    status = fbe_api_enclosure_get_ssc_count(objectId, &sscCount);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt, 
                                                   FBE_TRACE_LEVEL_WARNING,
                                                   FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                                   "Encl(%d_%d), %s Error getting SSC count, status 0x%x.\n",
                                                   location.bus, 
                                                   location.enclosure, 
                                                   __FUNCTION__, 
                                                   status);
    }
    
    if(processorEncl) 
    {
        status = fbe_api_get_board_object_id(&tempObjectId);
        if (status != FBE_STATUS_OK) 
        {
            fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt,  
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s Error in getting the Board Object ID, status: 0x%x.\n",
                                  __FUNCTION__, status);
    
            return status;
        }
        if (pEnclMgmt->base_environment.processorEnclType == DPE_ENCLOSURE_TYPE)
        {
            dpeEnclObjId = objectId; 
        }
    }
    else
    {
        tempObjectId = objectId;
    }

    
    status = fbe_api_enclosure_get_ps_count(tempObjectId, &psCount);
    

    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt, 
                                                          FBE_TRACE_LEVEL_WARNING,
                                                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                                          "Encl(%d_%d), %s Error in getting PS count, status 0x%x.\n",
                                                          location.bus, location.enclosure, __FUNCTION__, status);
    }

    // Some DPE's have fans reported by Board Object & Enclosure Object
    status = fbe_api_enclosure_get_fan_count(tempObjectId, &fanCount);

    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "Encl(%d_%d), %s Error in getting FAN count, status 0x%x.\n",
                              location.bus, location.enclosure, __FUNCTION__, status);
    }
    if (processorEncl &&
        (pEnclMgmt->base_environment.processorEnclType == DPE_ENCLOSURE_TYPE) &&
        (dpeEnclObjId != FBE_OBJECT_ID_INVALID))
    {
        dpeBoardObjFanCount = fanCount;
        // Some DPE's have fans reported by Board Object & Enclosure Object
        status = fbe_api_enclosure_get_fan_count(dpeEnclObjId, &fanCount);
        if(status != FBE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt, 
                                  FBE_TRACE_LEVEL_WARNING,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "Encl(%d_%d), %s Error in getting FAN count, status 0x%x.\n",
                                  location.bus, location.enclosure, __FUNCTION__, status);
        }
    }

    bbuCount = fbe_encl_mgmt_getEnclBbuCount(pEnclMgmt, &location);
    spsCount = fbe_encl_mgmt_getEnclSpsCount(pEnclMgmt, &location);

    status = fbe_api_enclosure_get_number_of_drive_midplane(objectId,&driveMidplaneCount);

    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt, 
                                                          FBE_TRACE_LEVEL_WARNING,
                                                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                                          "Encl(%d_%d), %s Error in getting driveMidplaneCount, status 0x%x.\n",
                                                          location.bus, location.enclosure, __FUNCTION__, status);
    }

    status = fbe_base_env_get_processor_encl_type((fbe_base_environment_t *)pEnclMgmt, &processorEnclType);

    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt,
                       FBE_TRACE_LEVEL_ERROR,
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "%s Error in getting peType, status: 0x%x.\n",
                       __FUNCTION__, status);
    }

    if(!((processorEncl) &&
        ((processorEnclType == XPE_ENCLOSURE_TYPE) || 
         (processorEnclType == VPE_ENCLOSURE_TYPE))))
    {
        status = fbe_api_enclosure_get_basic_info(objectId, &enclosure_info);
        if(status != FBE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt,
                                  FBE_TRACE_LEVEL_WARNING,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "Encl(%d_%d), %s Error in getting basic info, status: 0x%X.\n",
                                  location.bus, location.enclosure, __FUNCTION__, status);
        }
        else
        {
            fbe_copy_memory(pEnclInfo->serial_number, enclosure_info.enclUniqueId, sizeof(pEnclInfo->serial_number));
            subEnclCount = enclosure_info.enclRelatedExpanders;
        }
    }

    fbe_spinlock_lock(&pEnclInfo->encl_info_lock);
    pEnclInfo->processorEncl = processorEncl;
    pEnclInfo->enclType = enclType;
    pEnclInfo->lccCount = lccCount; 
    if(lccCount > FBE_ESP_EE_LCC_START_SLOT) 
    {
        pEnclInfo->subEnclLccStartSlot = FBE_ESP_EE_LCC_START_SLOT;
    }
    else
    {
        pEnclInfo->subEnclLccStartSlot = 0;
    }
    pEnclInfo->psCount = psCount;
    pEnclInfo->fanCount = fanCount + dpeBoardObjFanCount; 
    pEnclInfo->driveSlotCount = driveSlotCount;
    pEnclInfo->connectorCount = connectorCount;
    pEnclInfo->bbuCount = bbuCount;
    pEnclInfo->spsCount = spsCount;
    pEnclInfo->spCount = spCount;
    pEnclInfo->dimmCount = dimmCount;
    pEnclInfo->ssdCount = ssdCount;
    pEnclInfo->subEnclCount = subEnclCount;
    pEnclInfo->crossCable = crossCable;
    pEnclInfo->driveMidplaneCount = driveMidplaneCount; 
    pEnclInfo->sscCount = sscCount;
    fbe_spinlock_unlock(&pEnclInfo->encl_info_lock);

    /* This function needs to be called after the enclType was populated. */
    status = fbe_encl_mgmt_derive_encl_attr(pEnclMgmt, pEnclInfo);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "Encl(%d_%d), %s Error in drive encl attr, status 0x%X.\n",
                              location.bus, location.enclosure, __FUNCTION__, status);
        return status;
    }

    return FBE_STATUS_OK;
}


/*!**************************************************************
 * fbe_encl_mgmt_get_encl_type()
 ****************************************************************
 * @brief
 *  This function discovers the status for all the LCCs in the enclosure.
 *
 * @param pEnclMgmt - This is the object handle, or in our
 * case the encl_mgmt.
 * @param pEnclInfo - The pointer to the encl info.
 *
 * @return fbe_status_t - FBE Status
 *
 * @author
 *  13-Jan-2011: PHE - Created.
 *
 ****************************************************************/
static fbe_status_t fbe_encl_mgmt_get_encl_type(fbe_encl_mgmt_t * pEnclMgmt,
                                                fbe_object_id_t objectId,
                                                fbe_esp_encl_type_t * pEnclType)
{
    fbe_topology_object_type_t       objectType;
    fbe_board_mgmt_platform_info_t   platformInfo = {0};
    fbe_enclosure_types_t            enclType = FBE_ENCL_TYPE_INVALID;
    fbe_status_t                     status = FBE_STATUS_OK;

    status = fbe_api_get_object_type(objectId, &objectType, FBE_PACKAGE_ID_PHYSICAL);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s Error getting object type for objectId %d, status: 0x%x.\n",
                              __FUNCTION__, objectId, status);
        return status;
    }

    if(objectType == FBE_TOPOLOGY_OBJECT_TYPE_BOARD) 
    {
        status = fbe_api_board_get_platform_info(objectId, &platformInfo);
        if (status != FBE_STATUS_OK) {
            fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt, 
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s Error in getting platformInfo, status 0x%x.\n",
                                  __FUNCTION__, status);
            return status;
        }

        /* We only get the enclType for XPE. For DPE, we will use the enclosure object id
         * not board object id.
         */
        if((platformInfo.hw_platform_info.enclosureType == XPE_ENCLOSURE_TYPE) ||
           (platformInfo.hw_platform_info.enclosureType == VPE_ENCLOSURE_TYPE)) 
        {
            switch(platformInfo.hw_platform_info.midplaneType)
            {
                case FOGBOW_MIDPLANE:
                    *pEnclType = FBE_ESP_ENCL_TYPE_FOGBOW;
                    break;

                case BROADSIDE_MIDPLANE:
                    *pEnclType  = FBE_ESP_ENCL_TYPE_BROADSIDE;
                    break;

                case SIDESWIPE_MIDPLANE:
                    *pEnclType  = FBE_ESP_ENCL_TYPE_SIDESWIPE;
                    break;

                case RATCHET_MIDPLANE:
                    *pEnclType  = FBE_ESP_ENCL_TYPE_RATCHET;
                    break;

                case PROTEUS_MIDPLANE:
                    *pEnclType  = FBE_ESP_ENCL_TYPE_PROTEUS;
                    break;

                case TELESTO_MIDPLANE:
                    *pEnclType  = FBE_ESP_ENCL_TYPE_TELESTO;
                    break;
               case MERIDIAN_MIDPLANE:
                    *pEnclType = FBE_ESP_ENCL_TYPE_MERIDIAN;
                    break; 
                default:
                    *pEnclType = FBE_ESP_ENCL_TYPE_UNKNOWN;
                    break;
            }
        }
    }
    else
    {
        status = fbe_api_enclosure_get_encl_type(objectId, &enclType);
        if (status != FBE_STATUS_OK) 
        {
            fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt, 
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s Getting enclosure type failed for objectId %d, status 0x%x.\n",
                                  __FUNCTION__, objectId, status);
            return status;
        }
    
        switch(enclType) 
        {
            case FBE_ENCL_SAS_VIPER:
                *pEnclType = FBE_ESP_ENCL_TYPE_VIPER;
                break;
            case FBE_ENCL_SAS_DERRINGER:
                *pEnclType = FBE_ESP_ENCL_TYPE_DERRINGER;
                break;
            case FBE_ENCL_SAS_BUNKER:
                *pEnclType = FBE_ESP_ENCL_TYPE_BUNKER;
                break;
            case FBE_ENCL_SAS_CITADEL:
                *pEnclType = FBE_ESP_ENCL_TYPE_CITADEL;
                break;
            case FBE_ENCL_SAS_FALLBACK:
                *pEnclType = FBE_ESP_ENCL_TYPE_FALLBACK;
                break;
            case FBE_ENCL_SAS_BOXWOOD:
                *pEnclType = FBE_ESP_ENCL_TYPE_BOXWOOD;
                break;
            case FBE_ENCL_SAS_KNOT:
                *pEnclType = FBE_ESP_ENCL_TYPE_KNOT;
                break;
            case FBE_ENCL_SAS_PINECONE:
                *pEnclType = FBE_ESP_ENCL_TYPE_PINECONE;
                break;
            case FBE_ENCL_SAS_STEELJAW:
                *pEnclType = FBE_ESP_ENCL_TYPE_STEELJAW;
                break;
            case FBE_ENCL_SAS_ANCHO:
                *pEnclType = FBE_ESP_ENCL_TYPE_ANCHO;
                break;
            case FBE_ENCL_SAS_RAMHORN:
                *pEnclType = FBE_ESP_ENCL_TYPE_RAMHORN;
                break;
            case FBE_ENCL_SAS_VOYAGER_ICM:
                *pEnclType = FBE_ESP_ENCL_TYPE_VOYAGER;
                break;
            case FBE_ENCL_SAS_VIKING_IOSXP:
                *pEnclType = FBE_ESP_ENCL_TYPE_VIKING;
                break;
            case FBE_ENCL_SAS_CAYENNE_IOSXP:
                *pEnclType = FBE_ESP_ENCL_TYPE_CAYENNE;
                break;
            case FBE_ENCL_SAS_NAGA_IOSXP:
                *pEnclType = FBE_ESP_ENCL_TYPE_NAGA;
                break;

            case FBE_ENCL_SAS_TABASCO:
                *pEnclType = FBE_ESP_ENCL_TYPE_TABASCO;
                break;
            case FBE_ENCL_SAS_CALYPSO:
                *pEnclType = FBE_ESP_ENCL_TYPE_CALYPSO;
                break;
            case FBE_ENCL_SAS_MIRANDA:
                *pEnclType = FBE_ESP_ENCL_TYPE_MIRANDA;
                break;
            case FBE_ENCL_SAS_RHEA:
                *pEnclType = FBE_ESP_ENCL_TYPE_RHEA;
                break;
            default:
                 *pEnclType = FBE_ESP_ENCL_TYPE_UNKNOWN;
                break;
        }
    }

    return status;
}


/*!**************************************************************
 * fbe_encl_mgmt_derive_encl_attr()
 ****************************************************************
 * @brief
 *  This function derives the enclosure attributes based on the enclosure type.
 *
 * @param pEnclMgmt - The point to fbe_encl_mgmt_t.
 * @param pEnclInfo - The pointer to the specific enclosure info.
 *
 * @return fbe_status_t - FBE Status
 *
 * @author
 *  08-Dec-2011: PHE - Created.
 *
 ****************************************************************/
static fbe_status_t 
fbe_encl_mgmt_derive_encl_attr(fbe_encl_mgmt_t * pEnclMgmt,
                               fbe_encl_info_t * pEnclInfo)
{
    switch(pEnclInfo->enclType)
    {   
        case FBE_ESP_ENCL_TYPE_BUNKER:             // 15 drive sentry DPE
        case FBE_ESP_ENCL_TYPE_CITADEL:            // 25 drive sentry DPE
        case FBE_ESP_ENCL_TYPE_VIPER:              // 6GB 15 drive SAS DAE
        case FBE_ESP_ENCL_TYPE_DERRINGER:          // 6GB 25 drive SAS DAE
        case FBE_ESP_ENCL_TYPE_FALLBACK:           // 6GB 25 drive SAS DPE
        case FBE_ESP_ENCL_TYPE_BOXWOOD:            // 6GB 12 drive SAS DPE
        case FBE_ESP_ENCL_TYPE_KNOT:               // 6GB 25 drive SAS DPE
        case FBE_ESP_ENCL_TYPE_PINECONE:           // 6GB 12 drive SAS DAE
        case FBE_ESP_ENCL_TYPE_STEELJAW:           // 6GB 12 drive SAS DPE
        case FBE_ESP_ENCL_TYPE_RAMHORN:            // 6GB 25 drive SAS DPE
            pEnclInfo->maxEnclSpeed = SPEED_SIX_GIGABIT;
            pEnclInfo->currentEnclSpeed = SPEED_SIX_GIGABIT;
            pEnclInfo->driveSlotCountPerBank = FBE_ENCL_MGMT_DRIVE_COUNT_PER_BANK_NA;
            break;

        case FBE_ESP_ENCL_TYPE_VOYAGER:
            pEnclInfo->maxEnclSpeed = SPEED_SIX_GIGABIT;
            pEnclInfo->currentEnclSpeed = SPEED_SIX_GIGABIT;
            pEnclInfo->driveSlotCountPerBank = FBE_ENCL_MGMT_DRIVE_COUNT_PER_BANK_VOYAGER;
            break;

        case FBE_ESP_ENCL_TYPE_VIKING:
            pEnclInfo->maxEnclSpeed = SPEED_SIX_GIGABIT;
            pEnclInfo->currentEnclSpeed = SPEED_SIX_GIGABIT;
            pEnclInfo->driveSlotCountPerBank = FBE_ENCL_MGMT_DRIVE_COUNT_PER_BANK_VIKING;
            break;

        case FBE_ESP_ENCL_TYPE_CAYENNE:
            pEnclInfo->maxEnclSpeed = SPEED_TWELVE_GIGABIT;
            pEnclInfo->currentEnclSpeed = SPEED_TWELVE_GIGABIT;
            pEnclInfo->driveSlotCountPerBank = FBE_ENCL_MGMT_DRIVE_COUNT_PER_BANK_CAYENNE;
            break;

        case FBE_ESP_ENCL_TYPE_NAGA:
            pEnclInfo->maxEnclSpeed = SPEED_TWELVE_GIGABIT;
            pEnclInfo->currentEnclSpeed = SPEED_TWELVE_GIGABIT;
            pEnclInfo->driveSlotCountPerBank = FBE_ENCL_MGMT_DRIVE_COUNT_PER_BANK_NAGA;
            break;
    

        case FBE_ESP_ENCL_TYPE_ANCHO:              // 12GB 15 drive SAS DAE
        case FBE_ESP_ENCL_TYPE_TABASCO:            // 12GB 25 drive SAS DAE
        case FBE_ESP_ENCL_TYPE_CALYPSO:            // 12GB 25 drive SAS DPE
        case FBE_ESP_ENCL_TYPE_RHEA:               // 12GB 12 drive SAS DPE
        case FBE_ESP_ENCL_TYPE_MIRANDA:            // 12GB 25 drive SAS DPE
            pEnclInfo->maxEnclSpeed = SPEED_TWELVE_GIGABIT;
            pEnclInfo->currentEnclSpeed = SPEED_TWELVE_GIGABIT;
            pEnclInfo->driveSlotCountPerBank = FBE_ENCL_MGMT_DRIVE_COUNT_PER_BANK_NA;
            break;

        case FBE_ESP_ENCL_TYPE_FOGBOW:
        case FBE_ESP_ENCL_TYPE_BROADSIDE:
        case FBE_ESP_ENCL_TYPE_RATCHET:
        case FBE_ESP_ENCL_TYPE_PROTEUS:
        case FBE_ESP_ENCL_TYPE_TELESTO:
            pEnclInfo->maxEnclSpeed = SPEED_HARDWARE_DEFAULT;
            pEnclInfo->currentEnclSpeed = SPEED_HARDWARE_DEFAULT;
            pEnclInfo->driveSlotCountPerBank = FBE_ENCL_MGMT_DRIVE_COUNT_PER_BANK_NA;
            break;
        case FBE_ESP_ENCL_TYPE_MERIDIAN:
            pEnclInfo->maxEnclSpeed = SPEED_HARDWARE_DEFAULT;
            pEnclInfo->currentEnclSpeed = SPEED_HARDWARE_DEFAULT;
            pEnclInfo->driveSlotCountPerBank = FBE_ENCL_MGMT_DRIVE_COUNT_PER_BANK_NA;
            break;

        case FBE_ESP_ENCL_TYPE_UNKNOWN:
        case FBE_ESP_ENCL_TYPE_INVALID:
        default:
            pEnclInfo->maxEnclSpeed = SPEED_HARDWARE_DEFAULT;
            pEnclInfo->currentEnclSpeed = SPEED_HARDWARE_DEFAULT;
            pEnclInfo->driveSlotCountPerBank = FBE_ENCL_MGMT_DRIVE_COUNT_PER_BANK_NA;

            fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt, 
                                  FBE_TRACE_LEVEL_WARNING,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "Encl(%d_%d), %s enclType %d error.\n",
                                  pEnclInfo->location.bus,
                                  pEnclInfo->location.enclosure,
                                  __FUNCTION__, 
                                  pEnclInfo->enclType);

            return FBE_STATUS_GENERIC_FAILURE;
            break;
    }
    
    return FBE_STATUS_OK;
}

/*!**************************************************************
 * fbe_encl_mgmt_get_encl_info_by_object_id()
 ****************************************************************
 * @brief
 *  This function gets the pointer to the encl info by the encl object id.  
 *
 * @param encl_mgmt - 
 * @param object_id - 
 * @param encl_info (OUTPUT) - 
 *
 * @return fbe_status_t - 
 *
 * @author
 *  13-Jan-2011:  PHE - Added the function header. 
 *
 ****************************************************************/
static fbe_status_t fbe_encl_mgmt_get_encl_info_by_object_id(fbe_encl_mgmt_t *encl_mgmt,
                                                             fbe_object_id_t object_id,
                                                             fbe_encl_info_t **encl_info)
{
    fbe_u32_t encl_count;

    *encl_info = NULL;
    for(encl_count = 0; encl_count < FBE_ESP_MAX_ENCL_COUNT; encl_count++) {
        if(encl_mgmt->encl_info[encl_count]->object_id == object_id){
           *encl_info = encl_mgmt->encl_info[encl_count];
           return FBE_STATUS_OK;
        }
    }
    
    return FBE_STATUS_COMPONENT_NOT_FOUND;
}

/*!**************************************************************
 * fbe_encl_mgmt_get_subencl_info_by_subencl_object_id()
 ****************************************************************
 * @brief
 *  This function gets the pointer to the sub encl info by the subencl object id. 
 *
 * @param pEnclMgmt - 
 * @param subEnclObjectId - The object id could be either the encl object id or the subencl object id.
 * @param pSubEnclInfoPtr (OUTPUT) - The pointer to the subencl info.
 *
 * @return fbe_status_t - 
 *
 * @author
 *  06-Feb-2012:  PHE - Created. 
 *
 ****************************************************************/
static fbe_status_t fbe_encl_mgmt_get_subencl_info_by_subencl_object_id(fbe_encl_mgmt_t * pEnclMgmt,
                                                             fbe_object_id_t subEnclObjectId,
                                                             fbe_sub_encl_info_t ** pSubEnclInfoPtr)
{
    fbe_u32_t         encl_count;
    fbe_u32_t         subencl_count;
    fbe_encl_info_t * pEnclInfo = NULL;

    for(encl_count = 0; encl_count < FBE_ESP_MAX_ENCL_COUNT; encl_count++) 
    {
        pEnclInfo = pEnclMgmt->encl_info[encl_count];
    
        for(subencl_count = 0; subencl_count < pEnclInfo->subEnclCount; subencl_count++) 
        {
            if(pEnclInfo->subEnclInfo[subencl_count].objectId == subEnclObjectId)
            {
               *pSubEnclInfoPtr = &pEnclInfo->subEnclInfo[subencl_count];
               return FBE_STATUS_OK;
            }
        }
    }
    
    return FBE_STATUS_COMPONENT_NOT_FOUND;
}

/*!**************************************************************
 * fbe_encl_mgmt_get_encl_info_by_location()
 ****************************************************************
 * @brief
 *  This function gets the pointer to the encl info by the location. 
 *
 * @param pEnclMgmt - 
 * @param pLocation - 
 * @param pEnclInfoPtr (OUTPUT) - 
 *
 * @return fbe_status_t - 
 *
 * @author
 *  19-Aug-2010:  PHE - Created. 
 *
 ****************************************************************/
fbe_status_t fbe_encl_mgmt_get_encl_info_by_location(fbe_encl_mgmt_t * pEnclMgmt,
                                                     fbe_device_physical_location_t * pLocation,
                                                     fbe_encl_info_t ** pEnclInfoPtr)
{
    fbe_u32_t enclIndex = 0;
    fbe_u32_t enclCount = 0;
    fbe_encl_info_t * pEnclInfo = NULL;

    *pEnclInfoPtr = NULL;

    for(enclIndex = 0; enclIndex < FBE_ESP_MAX_ENCL_COUNT; enclIndex++) 
    {
        pEnclInfo = pEnclMgmt->encl_info[enclIndex];

        if(pEnclInfo->object_id != FBE_OBJECT_ID_INVALID)
        {
            if((pEnclInfo->location.bus == pLocation->bus) &&
               (pEnclInfo->location.enclosure == pLocation->enclosure))
            {
                *pEnclInfoPtr = pEnclInfo;
                return FBE_STATUS_OK;
            }

            if(++enclCount >= pEnclMgmt->total_encl_count)
            {
                /* Reach the total enclosure count, no need to continue.*/
                break;
            }
        }
    }
    
    return FBE_STATUS_NO_DEVICE;
}

/*!**************************************************************
 * fbe_encl_mgmt_get_lcc_info_ptr()
 ****************************************************************
 * @brief
 *  This function gets the pointer to the lcc info. 
 *
 * @param pEnclMgmt -
 * @param pLocation - 
 * @param pLccInfoPtr (OUTPUT) - 
 *
 * @return fbe_status_t - 
 *
 * @author
 *  20-Sept-2011:  PHE - Created. 
 *
 ****************************************************************/
fbe_status_t fbe_encl_mgmt_get_lcc_info_ptr(fbe_encl_mgmt_t * pEnclMgmt,
                                        fbe_device_physical_location_t * pLocation,
                                        fbe_lcc_info_t ** pLccInfoPtr)
{
    fbe_u32_t       subenclLccIndex;
    fbe_encl_info_t * pEnclInfo = NULL;
    fbe_u32_t       index = 0;
    fbe_status_t    status = FBE_STATUS_OK;

    *pLccInfoPtr = NULL;

    status = fbe_encl_mgmt_get_encl_info_by_location(pEnclMgmt, pLocation, &pEnclInfo);

    if (status != FBE_STATUS_OK) 
    {
        return status;
    }

    if(pLocation->componentId == 0) 
    {
        *pLccInfoPtr = &pEnclInfo->lccInfo[pLocation->slot];
        return FBE_STATUS_OK;
    }
   
    for(index = 0; index < pEnclInfo->subEnclCount; index ++ ) 
    {
        if(pEnclInfo->subEnclInfo[index].location.componentId == pLocation->componentId) 
        {
            /* Convert the subenclosure lcc slot to subenclosuer lcc index. */
            subenclLccIndex = pLocation->slot%FBE_ESP_MAX_LCCS_PER_SUBENCL;

            if (subenclLccIndex < FBE_ESP_MAX_SUBENCLS_PER_ENCL)
            {
                *pLccInfoPtr = &pEnclInfo->subEnclInfo[index].lccInfo[subenclLccIndex];
                return FBE_STATUS_OK;
            }
            else
            {
                fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt, 
                                      FBE_TRACE_LEVEL_WARNING,
                                      FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                      "Encl(%d_%d.%d)LCC%d, %s Error SubenclLccInfo index out of range:%d\n",
                                      pLocation->bus, 
                                      pLocation->enclosure,
                                      pLocation->componentId,
                                      pLocation->slot,
                                      __FUNCTION__,
                                      subenclLccIndex);
                return FBE_STATUS_GENERIC_FAILURE;
            }
        }
    }

    return FBE_STATUS_GENERIC_FAILURE;
}

/*!***************************************************************
 * fbe_encl_mgmt_find_available_encl_info()
 ****************************************************************
 * @brief
 *  This function searches through the enclosures and
 *  returns the pointer to the available encl info.
 *
 * @param  pEnclMgmt - 
 * @param  pEnclPsInfoPtr(OUTPUT) - 
 *
 * @return fbe_status_t
 *
 * @author
 *  30-Aug-2010 PHE - Created. 
 *
 ****************************************************************/
fbe_status_t fbe_encl_mgmt_find_available_encl_info(fbe_encl_mgmt_t * pEnclMgmt, 
                                                     fbe_encl_info_t ** pEnclInfoPtr)
{ 
    fbe_u32_t             encl_count;
    fbe_encl_info_t     * pEnclInfo = NULL;
   
    *pEnclInfoPtr = NULL;
   
    for(encl_count = 0; encl_count < FBE_ESP_MAX_ENCL_COUNT; encl_count++) 
    {
        pEnclInfo = pEnclMgmt->encl_info[encl_count];

        if(pEnclInfo->object_id == FBE_OBJECT_ID_INVALID)
        {
            // found enclosure
            *pEnclInfoPtr = pEnclInfo;
            return FBE_STATUS_OK;
        }
    }

    return FBE_STATUS_COMPONENT_NOT_FOUND;
}


/*!***************************************************************
 * fbe_encl_mgmt_find_available_subencl_info()
 ****************************************************************
 * @brief
 *  This function searches through the enclosures and
 *  returns the pointer to the available encl info.
 *
 * @param  pEnclMgmt -
 * @param  pEnclInfo - 
 * @param  pEnclPsInfoPtr(OUTPUT) - 
 *
 * @return fbe_status_t
 *
 * @author
 *  30-Aug-2012 PHE - Created. 
 *
 ****************************************************************/
static fbe_status_t 
fbe_encl_mgmt_find_available_subencl_info(fbe_encl_mgmt_t * pEnclMgmt, 
                                          fbe_encl_info_t * pEnclInfo,
                                          fbe_sub_encl_info_t ** pSubEnclInfoPtr)
{ 
    fbe_u32_t                 subencl_count;
  
    for(subencl_count = 0; subencl_count < FBE_ESP_MAX_SUBENCLS_PER_ENCL; subencl_count++) 
    {
        if(pEnclInfo->subEnclInfo[subencl_count].objectId == FBE_OBJECT_ID_INVALID) 
        {
            // found subenclosure
            *pSubEnclInfoPtr = &pEnclInfo->subEnclInfo[subencl_count];
            return FBE_STATUS_OK;
        }
    }
   
    return FBE_STATUS_COMPONENT_NOT_FOUND;
}

/*!**************************************************************
 * fbe_encl_mgmt_shutdown_debounce_timer_cond_function()
 ****************************************************************
 * @brief
 *  This is timer function that handles debouncing of shutdown Reason
 *
 * @param object_handle - This is the object handle, or in our
 * case the encl_mgmt.
 * @param packet_p - The packet arriving from the monitor
 * scheduler.
 *
 * @return fbe_lifecycle_status_t - The status of the call to
 *                        fbe_lifecycle_class_const_verify()
 *                        for the enclosure management's
 *                        constant lifecycle data.
 *
 * @author
 *  12-Dec-2012:  Created Ashok Tamilarasan
 ****************************************************************/
static fbe_lifecycle_status_t 
fbe_encl_mgmt_shutdown_debounce_timer_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet)
{
    fbe_encl_mgmt_t               * encl_mgmt = (fbe_encl_mgmt_t *) base_object;
    fbe_u32_t                       enclIndex = 0;
    fbe_encl_info_t               * pEnclInfo = NULL;
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_bool_t                      debounce_in_progress = FBE_FALSE;

    for(enclIndex = 0; enclIndex < FBE_ESP_MAX_ENCL_COUNT; enclIndex ++) 
    {
        pEnclInfo = encl_mgmt->encl_info[enclIndex];

        if(pEnclInfo->object_id != FBE_OBJECT_ID_INVALID)
        {
           if(pEnclInfo->shutdownReasonDebounceInProgress) 
           {
               /* There is debounce in progress and so we dont want to clear the timer */
               debounce_in_progress = FBE_TRUE;

               /* Check if we have reached the debouce timeout */
               if(fbe_get_elapsed_seconds(pEnclInfo->debounceStartTime) > FBE_ENCL_MGMT_SHUTDOWN_REASON_DEBOUNCE_TIMEOUT)
               {
                   pEnclInfo->shutdownReasonDebounceComplete = FBE_TRUE;
                   pEnclInfo->shutdownReasonDebounceInProgress = FBE_FALSE;
                   fbe_encl_mgmt_process_enclosure_status(encl_mgmt, pEnclInfo->object_id,
                                                          &pEnclInfo->location);
               }
           }
        } // End of - if(encl_mgmt->encl_info[encl_count]->object_id != FBE_OBJECT_ID_INVALID)
    } // End of for loop.

    if(!debounce_in_progress)
    {   
        fbe_base_object_trace(base_object, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s Stop the timer\n.",
                              __FUNCTION__);
        /* There are no debouncing in progress and so stop the timer as it is not needed anymore */
        status = fbe_lifecycle_stop_timer(&FBE_LIFECYCLE_CONST_DATA(encl_mgmt),
                                        base_object, 
                                        FBE_ENCL_MGMT_SHUTDOWN_DEBOUNCE_TIMER_COND);
    }
    else
    {
        status = fbe_lifecycle_clear_current_cond(base_object);
        if (status != FBE_STATUS_OK) 
        {
            fbe_base_object_trace(base_object, 
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s can't clear current condition, status: 0x%x.\n.",
                                  __FUNCTION__, status);
        }
    
    }
    return FBE_LIFECYCLE_STATUS_DONE;
}

/*!**************************************************************
 * fbe_encl_mgmt_eir_get_data_cond_function()
 ****************************************************************
 * @brief
 *  This function initiates the process of reading environmental data from
 *  all enclosure objects.  The data received from each enclosure is copied to the
 *  structure that contains all enclosures.
 *
 * @param object_handle - This is the object handle, or in our
 * case the encl_mgmt.
 * @param packet_p - The packet arriving from the monitor
 * scheduler.
 *
 * @return fbe_lifecycle_status_t - The status of the call to
 *                        fbe_lifecycle_class_const_verify()
 *                        for the enclosure management's
 *                        constant lifecycle data.
 *
 * @author
 *  25-Jun-2010:  Created. D. McFarland
 *  17-Mar-2011:  PHE - Reorganized the code. 
 ****************************************************************/
static fbe_lifecycle_status_t 
fbe_encl_mgmt_get_eir_data_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet)
{
    fbe_encl_mgmt_t               * encl_mgmt = (fbe_encl_mgmt_t *) base_object;
    fbe_object_id_t          board_object_id;
    fbe_u32_t                       enclIndex = 0;
    fbe_u32_t                       enclCount = 0;
    fbe_eses_encl_eir_info_t eir_data;
    fbe_base_board_get_eir_info_t   peEirData;
    fbe_encl_info_t               * pEnclInfo = NULL;
    fbe_eir_input_power_sample_t  * pEnclCurrentInputPower = NULL;
    fbe_eir_temp_sample_t         * pEnclCurrentAirInletTemp = NULL;
    fbe_status_t                    status = FBE_STATUS_OK;

    // update count
    if (encl_mgmt->eirSampleCount < FBE_SAMPLE_DATA_SIZE)
    {
        encl_mgmt->eirSampleCount++;
    }

    status = fbe_api_get_board_object_id(&board_object_id);
    if (status != FBE_STATUS_OK) 
    {
        fbe_base_object_trace(base_object, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s Error in getting the Board Object ID, status: 0x%X\n",
                              __FUNCTION__, status);

        return FBE_LIFECYCLE_STATUS_DONE;
    }

    for(enclIndex = 0; enclIndex < FBE_ESP_MAX_ENCL_COUNT; enclIndex ++) 
    {
        pEnclInfo = encl_mgmt->encl_info[enclIndex];

        if(pEnclInfo->object_id != FBE_OBJECT_ID_INVALID)
        {
           if(pEnclInfo->processorEncl) 
           {
                /* This is the processor enclosure.
                 * Read the EIR info from the board object.
                 */
               fbe_zero_memory(&peEirData, sizeof(fbe_base_board_get_eir_info_t));
               status = fbe_api_board_get_eir_info(board_object_id, &peEirData);

                if(status != FBE_STATUS_OK) 
                {
                    fbe_base_object_trace(base_object, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s Error in getting Board EIR info, status: 0x%x.\n",
                              __FUNCTION__, status);

                    return FBE_LIFECYCLE_STATUS_DONE;
                }

                pEnclCurrentInputPower = &(peEirData.peEirData.enclCurrentInputPower);
                pEnclCurrentAirInletTemp = &(peEirData.peEirData.enclCurrentAirInletTemp);
                //the value we got form base board is 10th of real value, while the value we get from
                //encl is 100 times of real value.
                pEnclCurrentAirInletTemp->airInletTemp *= 10;
            }
            else
            {
                /* This is NOT processor enclosure.
                 * Read the EIR info from the enclosure object.
                 */
                status = fbe_api_enclosure_get_eir_info(pEnclInfo->object_id, &eir_data);

                if(status != FBE_STATUS_OK) 
                {
                    fbe_base_object_trace(base_object, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s Error in getting Encl EIR info, status: 0x%x.\n",
                              __FUNCTION__, status);

                    return FBE_LIFECYCLE_STATUS_DONE;
                }

                pEnclCurrentInputPower = &(eir_data.enclEirData.enclCurrentInputPower);
                pEnclCurrentAirInletTemp = &(eir_data.enclEirData.enclCurrentAirInletTemp);
            }

                
            // save EIR data
            fbe_encl_mgmt_processEnclEirData(encl_mgmt, 
                                             &pEnclInfo->location,
                                             pEnclCurrentInputPower,
                                             pEnclCurrentAirInletTemp);

            if(++enclCount >= encl_mgmt->total_encl_count)
            {
                /* Reach the total enclosure count, not need to continue.*/
                break;
            }
        } // End of - if(encl_mgmt->encl_info[encl_count]->object_id != FBE_OBJECT_ID_INVALID)
    } // End of for loop.

    // update index
    if (++encl_mgmt->eirSampleIndex >= FBE_SAMPLE_DATA_SIZE)
    {
        encl_mgmt->eirSampleIndex = 0;
    }

    status = fbe_lifecycle_clear_current_cond(base_object);
    if (status != FBE_STATUS_OK) 
    {
        fbe_base_object_trace(base_object, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s can't clear current condition, status: 0x%x.\n.",
                              __FUNCTION__, status);
    }

    return FBE_LIFECYCLE_STATUS_DONE;
}

/*!**************************************************************
 * fbe_encl_mgmt_check_encl_shutdown_delay_cond_function()
 ****************************************************************
 * @brief
 *  This function check whether we should reset enclosure shutdown timer.
 *
 * @param object_handle - This is the object handle, or in our
 * case the encl_mgmt.
 * @param packet_p - The packet arriving from the monitor
 * scheduler.
 *
 * @return fbe_lifecycle_status_t - The status of the call to
 *                        fbe_lifecycle_class_const_verify()
 *                        for the enclosure management's
 *                        constant lifecycle data.
 *
 * @author
 *  10-July-2012:  Created. Zhou Eric
 ****************************************************************/
static fbe_lifecycle_status_t 
fbe_encl_mgmt_check_encl_shutdown_delay_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_encl_mgmt_t                         * pEnclMgmt = (fbe_encl_mgmt_t *)base_object;
    fbe_u8_t                                deviceStr[FBE_EVENT_LOG_MESSAGE_STRING_LENGTH];
    fbe_device_physical_location_t          location = {0};
    fbe_encl_info_t                         * pEnclInfo = NULL;
    fbe_bool_t                              psRemoved = FALSE;
    fbe_power_supply_info_t                 psInfo;
    fbe_u32_t                               psIndex;

    fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s, entry.\n", __FUNCTION__);

    location = pEnclMgmt->base_environment.bootEnclLocation;
    status = fbe_encl_mgmt_get_encl_info_by_location(pEnclMgmt, &location, &pEnclInfo);
    if (status != FBE_STATUS_OK) 
    {
        fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt, 
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s, Error when getting enclosure info 0x%X.\n",
                                  __FUNCTION__, status);
    }

    if (pEnclInfo->shutdownReason == FBE_ESES_SHUTDOWN_REASON_CRITICAL_COOLIG_FAULT)
    {
        //Check if there are removed PS.
        for (psIndex = 0; psIndex < pEnclInfo->psCount; psIndex++)
        {
            fbe_zero_memory(&psInfo, sizeof(fbe_power_supply_info_t));
            psInfo.slotNumOnEncl = psIndex;
            status = fbe_api_power_supply_get_power_supply_info(pEnclInfo->object_id, &psInfo);
            if (status != FBE_STATUS_OK) 
            {
                fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt, 
                                        FBE_TRACE_LEVEL_ERROR,
                                        FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                        "%s, Error getting power supply info, status: 0x%X.\n",
                                        __FUNCTION__, status);
                return FBE_LIFECYCLE_STATUS_DONE;
            }

            if (psInfo.bInserted == FALSE)
            {
                psRemoved = TRUE;
                break;
            }
        }

        if (psRemoved == TRUE)
        {
            if (pEnclInfo->enclShutdownDelayInProgress == FBE_FALSE)
            {
                status = fbe_lifecycle_force_clear_cond(&FBE_LIFECYCLE_CONST_DATA(encl_mgmt),
                                                        base_object,
                                                        FBE_ENCL_MGMT_RESET_ENCL_SHUTDOWN_TIMER_COND);
                if (status != FBE_STATUS_OK)
                {
                    fbe_base_object_trace(base_object,
                                        FBE_TRACE_LEVEL_ERROR,
                                        FBE_TRACE_MESSAGE_ID_INFO,
                                        "%s, failed to start timer condition FBE_ENCL_MGMT_RESET_ENCL_SHUTDOWN_TIMER_COND, status: 0x%X\n", 
                                        __FUNCTION__, status);
                    return FBE_LIFECYCLE_STATUS_DONE;
                }
                else
                {
                    fbe_base_object_trace(base_object,
                                        FBE_TRACE_LEVEL_INFO,
                                        FBE_TRACE_MESSAGE_ID_INFO,
                                        "%s, Encl %d_%d shutdown timer will be reset.\n", 
                                        __FUNCTION__, location.bus, location.enclosure);

                    // generate appropriate event
                    status = fbe_base_env_create_device_string(FBE_DEVICE_TYPE_ENCLOSURE, 
                                                               &location, 
                                                               &deviceStr[0],
                                                               FBE_DEVICE_STRING_LENGTH);

                    fbe_event_log_write(ESP_INFO_ENCL_SHUTDOWN_DELAY_INPROGRESS,
                                NULL, 0, 
                                "%s", 
                                &deviceStr[0]);

                    pEnclInfo->enclShutdownDelayInProgress = FBE_TRUE;
                }
            }
        }
    }
    else if (pEnclInfo->enclShutdownDelayInProgress == FBE_TRUE)
    {
        pEnclInfo->enclShutdownDelayInProgress = FBE_FALSE;
    }

    status = fbe_lifecycle_clear_current_cond(base_object);
    if (status != FBE_STATUS_OK) {
        fbe_base_object_trace(base_object, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s can't clear current condition, status: 0x%x.\n",
                              __FUNCTION__, status);
    }

    return FBE_LIFECYCLE_STATUS_DONE;
}

/*!**************************************************************
 * fbe_encl_mgmt_reset_encl_shutdown_timer_cond_function()
 ****************************************************************
 * @brief
 *  This function reset the enclosure shutdown timer.
 *
 * @param object_handle - This is the object handle, or in our
 * case the encl_mgmt.
 * @param packet_p - The packet arriving from the monitor
 * scheduler.
 *
 * @return fbe_lifecycle_status_t - The status of the call to
 *                        fbe_lifecycle_class_const_verify()
 *                        for the enclosure management's
 *                        constant lifecycle data.
 *
 * @author
 *  10-July-2012:  Created. Zhou Eric
 ****************************************************************/
static fbe_lifecycle_status_t 
fbe_encl_mgmt_reset_encl_shutdown_timer_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet)
{
    fbe_encl_mgmt_t                                         * pEnclMgmt = (fbe_encl_mgmt_t *)base_object;
    fbe_device_physical_location_t                          location = {0};
    fbe_encl_info_t                                         * pEnclInfo = NULL;
    fbe_u8_t                                                deviceStr[FBE_EVENT_LOG_MESSAGE_STRING_LENGTH];
    fbe_status_t                                            status = FBE_STATUS_OK;

    fbe_base_object_trace(base_object, 
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s, entry\n", __FUNCTION__);

    location = pEnclMgmt->base_environment.bootEnclLocation;
    status = fbe_encl_mgmt_get_encl_info_by_location(pEnclMgmt, &location, &pEnclInfo);
    if (status != FBE_STATUS_OK) 
    {
        fbe_base_object_trace(base_object, 
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s, Error when getting enclosure info 0x%X.\n",
                                  __FUNCTION__, status);
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    if (pEnclInfo->enclShutdownDelayInProgress == FBE_TRUE)
    {
        // generate appropriate event
        status = fbe_base_env_create_device_string(FBE_DEVICE_TYPE_ENCLOSURE, 
                                                   &location, 
                                                   &deviceStr[0],
                                                   FBE_DEVICE_STRING_LENGTH);
        if(status != FBE_STATUS_OK)
        {
            fbe_base_object_trace(base_object, 
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                                  "%s, Failed to create device string.\n", 
                                  __FUNCTION__);
            return FBE_LIFECYCLE_STATUS_DONE;
        }
        
        fbe_event_log_write(ESP_INFO_RESET_ENCL_SHUTDOWN_TIMER,
                    NULL, 0, 
                    "%s", 
                    &deviceStr[0]);

        pEnclInfo->enclShutdownDelayInProgress = FBE_FALSE;

        status = fbe_lifecycle_set_cond(&FBE_LIFECYCLE_CONST_DATA(encl_mgmt), 
                                        base_object, 
                                        FBE_ENCL_MGMT_PROCESS_CACHE_STATUS_COND);
        if (status != FBE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt, 
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s, can't set condition FBE_ENCL_MGMT_PROCESS_CACHE_STATUS_COND, status: 0x%X.\n",
                                  __FUNCTION__, status);
            pEnclInfo->enclShutdownDelayInProgress = FBE_TRUE;
            return FBE_LIFECYCLE_STATUS_DONE;
        }

        //Reset enclosure shutdown timer.
        status = fbe_api_enclosure_resetShutdownTimer(pEnclInfo->object_id);
        if (status != FBE_STATUS_OK)
        {
            fbe_base_object_trace(base_object, 
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s, failed to reset enclosure shutdown timer, status: 0x%X\n",
                                  __FUNCTION__, status);
            pEnclInfo->enclShutdownDelayInProgress = FBE_TRUE;
            return FBE_LIFECYCLE_STATUS_DONE;
        }
        else
        {
            fbe_base_object_trace(base_object, 
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s, Resetting Encl%d_%d shutdown timer.\n",
                                  __FUNCTION__, location.bus, location.enclosure);
        }
    }

    status = fbe_lifecycle_stop_timer(&FBE_LIFECYCLE_CONST_DATA(encl_mgmt),
                                    base_object, 
                                    FBE_ENCL_MGMT_RESET_ENCL_SHUTDOWN_TIMER_COND);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace(base_object,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s, can't stop DUMP CACHE timer, status: 0x%X\n",
                              __FUNCTION__, status);
        pEnclInfo->enclShutdownDelayInProgress = FBE_TRUE;
    }

    return FBE_LIFECYCLE_STATUS_DONE;
}

/*!**************************************************************
 * fbe_encl_mgmt_process_cache_status_cond_function()
 ****************************************************************
 * @brief
 *  This function process the enclosure cache status.
 *
 * @param object_handle - This is the object handle, or in our
 * case the encl_mgmt.
 * @param packet_p - The packet arriving from the monitor
 * scheduler.
 *
 * @return fbe_lifecycle_status_t - The status of the call to
 *                        fbe_lifecycle_class_const_verify()
 *                        for the enclosure management's
 *                        constant lifecycle data.
 *
 * @author
 *  10-July-2012:  Created. Zhou Eric
 ****************************************************************/
static fbe_lifecycle_status_t 
fbe_encl_mgmt_process_cache_status_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet)
{
    fbe_encl_mgmt_t                         * pEnclMgmt = (fbe_encl_mgmt_t *)base_object;
    fbe_device_physical_location_t          location = {0};
    fbe_encl_info_t                         * pEnclInfo = NULL;
    fbe_common_cache_status_t               enclCacheStatus;
    fbe_common_cache_status_t               peerEnclCacheStatus = FBE_CACHE_STATUS_OK;
    fbe_status_t                            status = FBE_STATUS_OK;

    fbe_base_object_trace(base_object, 
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s, entry\n", __FUNCTION__);

    location = pEnclMgmt->base_environment.bootEnclLocation;
    status = fbe_encl_mgmt_get_encl_info_by_location(pEnclMgmt, &location, &pEnclInfo);
    if (status != FBE_STATUS_OK) 
    {
        fbe_base_object_trace(base_object, 
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s, Error when getting enclosure info 0x%X.\n",
                                  __FUNCTION__, status);
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    if (pEnclInfo->enclShutdownDelayInProgress != FBE_TRUE)
    {
        if (pEnclInfo->shutdownReason != FBE_ESES_SHUTDOWN_REASON_NOT_SCHEDULED)
        {
            enclCacheStatus = FBE_CACHE_STATUS_FAILED_SHUTDOWN;
        }
        else
        {
            enclCacheStatus = FBE_CACHE_STATUS_OK;
        }
        
        if (pEnclMgmt->enclCacheStatus != enclCacheStatus)
        {
            fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt, 
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s, Encl %d_%d, enclCacheStatus changed from %d to %d\n",
                                  __FUNCTION__, 
                                  location.bus,
                                  location.enclosure,
                                  pEnclMgmt->enclCacheStatus,
                                  enclCacheStatus);

            /* Log the event for cache status change */
            fbe_event_log_write(ESP_INFO_CACHE_STATUS_CHANGED,
                                NULL, 0,
                                "%s %s %s",
                                "ENCL_MGMT",
                                fbe_base_env_decode_cache_status(pEnclMgmt->enclCacheStatus),
                                fbe_base_env_decode_cache_status(enclCacheStatus));

            status = fbe_base_environment_get_peerCacheStatus((fbe_base_environment_t *)pEnclMgmt,
                                                             &peerEnclCacheStatus);
            /* Send CMI to peer */
            fbe_base_environment_send_cacheStatus_update_to_peer((fbe_base_environment_t *)pEnclMgmt, 
                                                              enclCacheStatus,
                                                             ((peerEnclCacheStatus == FBE_CACHE_STATUS_UNINITIALIZE)? FALSE : TRUE));

            pEnclMgmt->enclCacheStatus = enclCacheStatus;
            fbe_base_environment_send_data_change_notification((fbe_base_environment_t *)pEnclMgmt, 
                                                               FBE_CLASS_ID_ENCL_MGMT, 
                                                               FBE_DEVICE_TYPE_SP_ENVIRON_STATE, 
                                                               FBE_DEVICE_DATA_TYPE_GENERIC_INFO,
                                                               &location);    
            fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt, 
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s, FBE_DEVICE_TYPE_SP_ENVIRON_STATE sent\n",
                                  __FUNCTION__);
        }
    }

    status = fbe_lifecycle_clear_current_cond(base_object);
    if (status != FBE_STATUS_OK) {
        fbe_base_object_trace(base_object, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s, can't clear current condition, status: 0x%x.\n",
                              __FUNCTION__, status);
    }

    return FBE_LIFECYCLE_STATUS_DONE;
}

/*!**************************************************************
 * fbe_encl_mgmt_derive_lcc_info()
 ****************************************************************
 * @brief
 *  This function derive some LCC info from enclosure info
 *
 * @param pEnclMgmt - This is the object handle, or in our case the encl_mgmt.
 * @param pEnclInfo - The Enclosure info
 * @param pLccInfo - The LCC info
 *
 * @return fbe_status_t - 
 *
 * @author
 *  28-Aug-2012:  Rui Chang - Created.
 *
 ****************************************************************/
static fbe_status_t 
fbe_encl_mgmt_derive_lcc_info(fbe_encl_mgmt_t * pEnclMgmt,
                               fbe_encl_info_t        * pEnclInfo,
                               fbe_lcc_info_t          * pLccInfo)
{
    switch(pEnclInfo->enclType)
    {   
        case FBE_ESP_ENCL_TYPE_VIPER:               // 6GB 15 drive SAS DAE
            pLccInfo->lccType = FBE_LCC_TYPE_6G_VIPER;
            break;
        case FBE_ESP_ENCL_TYPE_PINECONE:            // 6GB 12 drive SAS DAE
            pLccInfo->lccType = FBE_LCC_TYPE_6G_PINECONE;
            break;
        case FBE_ESP_ENCL_TYPE_DERRINGER:           // 6GB 25 drive SAS DAE
            pLccInfo->lccType = FBE_LCC_TYPE_6G_DERRINGER;
            break;
        case FBE_ESP_ENCL_TYPE_VOYAGER:
            if (pLccInfo->uniqueId  == VOYAGER_ICM)
            {
                pLccInfo->lccType = FBE_LCC_TYPE_6G_VOYAGER_ICM;
            }
            else if(pLccInfo->uniqueId == VOYAGER_LCC)
            {
                pLccInfo->lccType = FBE_LCC_TYPE_6G_VOYAGER_EE;
            }
            else
            {
                pLccInfo->lccType = FBE_LCC_TYPE_UNKNOWN;
            }
           
            break;
        case FBE_ESP_ENCL_TYPE_BUNKER:              // 6GB 15 drive Sentry
            pLccInfo->lccType = FBE_LCC_TYPE_6G_SENTRY_15;
            break;
        case FBE_ESP_ENCL_TYPE_CITADEL:             // 6GB 25 drive Sentry
            pLccInfo->lccType = FBE_LCC_TYPE_6G_SENTRY_25;
            break;
        case FBE_ESP_ENCL_TYPE_FALLBACK:            // 6GB 25 drive jetfire
            pLccInfo->lccType = FBE_LCC_TYPE_6G_JETFIRE;
            break;
        case FBE_ESP_ENCL_TYPE_BOXWOOD:             // 6GB 12 drive silverbolt
            pLccInfo->lccType = FBE_LCC_TYPE_6G_SILVERBOLT_12;
            break;
        case FBE_ESP_ENCL_TYPE_KNOT:                // 6GB 25 drive silverbolt
            pLccInfo->lccType = FBE_LCC_TYPE_6G_SILVERBOLT_25;
            break;
        case FBE_ESP_ENCL_TYPE_STEELJAW:            // 6GB 12 drive beachcomber
            pLccInfo->lccType = FBE_LCC_TYPE_6G_BEACHCOMBER_12;
            break;
        case FBE_ESP_ENCL_TYPE_RAMHORN:             // 6GB 25 drive beachcomber
            pLccInfo->lccType = FBE_LCC_TYPE_6G_BEACHCOMBER_25;
            break;
        case FBE_ESP_ENCL_TYPE_CAYENNE:
            pLccInfo->lccType = FBE_LCC_TYPE_12G_CAYENNE;
            break;
        case FBE_ESP_ENCL_TYPE_VIKING:
            pLccInfo->lccType = FBE_LCC_TYPE_6G_VIKING;
            break;
        case FBE_ESP_ENCL_TYPE_NAGA:
            pLccInfo->lccType = FBE_LCC_TYPE_12G_NAGA;
            break;
        case FBE_ESP_ENCL_TYPE_ANCHO:               // 12GB 12 drive SAS DAE
            pLccInfo->lccType = FBE_LCC_TYPE_12G_ANCHO;
            break;
        case FBE_ESP_ENCL_TYPE_TABASCO:             // 12GB 25 drive SAS DAE
            pLccInfo->lccType = FBE_LCC_TYPE_12G_TABASCO;
            break;
        case FBE_ESP_ENCL_TYPE_CALYPSO:             // 12GB 25 drive Hyperion
            pLccInfo->lccType = FBE_LCC_TYPE_12G_CALYPSO;
            break;
        case FBE_ESP_ENCL_TYPE_MIRANDA:             // 12GB 25 drive Oberon/Charon
            pLccInfo->lccType = FBE_LCC_TYPE_12G_MIRANDA;
            break;
        case FBE_ESP_ENCL_TYPE_RHEA:                // 12GB 12 drive Oberon/Charon
            pLccInfo->lccType = FBE_LCC_TYPE_12G_RHEA;
            break;
        case FBE_ESP_ENCL_TYPE_MERIDIAN:
            pLccInfo->lccType = FBE_LCC_TYPE_MERIDIAN;  // vVNX VPE
            break;
        default:
            pLccInfo->lccType = FBE_LCC_TYPE_UNKNOWN;
            break;
    }

    pLccInfo->currentSpeed = pEnclInfo->currentEnclSpeed;
    pLccInfo->maxSpeed     = pEnclInfo->maxEnclSpeed;
    pLccInfo->shunted       = FBE_FALSE;

    return FBE_STATUS_OK;
}


/*!**************************************************************
 * fbe_encl_mgmt_process_lcc_status()
 ****************************************************************
 * @brief
 *  This function processes the LCC status change at startup time discovery or
 *  at runtime.
 *
 * @param pEnclMgmt - This is the object handle, or in our case the encl_mgmt.
 * @param objectId - 
 * @param pLocation - The location of the LCC.
 *
 * @return fbe_status_t - 
 *
 * @author
 *  30-Aug-2010:  PHE - Created.
 *
 ****************************************************************/
fbe_status_t 
fbe_encl_mgmt_process_lcc_status(fbe_encl_mgmt_t * pEnclMgmt,
                               fbe_object_id_t objectId,
                               fbe_device_physical_location_t * pLocation)
{
    fbe_encl_info_t                    * pEnclInfo = NULL;
    fbe_lcc_info_t                     * pLccInfo = NULL;
    fbe_lcc_info_t                       oldLccInfo = {0};
    fbe_status_t                         status = FBE_STATUS_OK;

    if(objectId == FBE_OBJECT_ID_INVALID)
    {
        fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s, encl %d_%d, invalid objectId.\n",
                              __FUNCTION__, 
                              pLocation->bus, 
                              pLocation->enclosure);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_encl_mgmt_get_encl_info_by_object_id(pEnclMgmt, objectId, &pEnclInfo);

    if (status != FBE_STATUS_OK) 
    {
        /* The enclosure was not initialized yet.  
         * When the enclosure gets initialized, 
         * it will call fbe_encl_mgmt_process_lcc_status anyway.
         * Just return FBE_STATUS_OK here.
         */
        fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s, Encl %d_%d, objectId 0x%X is not populated, no action taken.\n",
                              __FUNCTION__, 
                              pLocation->bus, 
                              pLocation->enclosure,
                              objectId);

        return FBE_STATUS_OK;
    }

    /* We will receive LCC status change notification when Base Module (jetfire) or 
     * SP board (other DPE) status changes. Because in eses, they are all treated as LCC.
     * But we do not deal with that here in encl_mgmt, we will deal with in in module_mgmt
     * (for jetfire) or board_mgmt. 
     * lccCount in encl_mgmt keeps the stand alone LCC componet number. 
     */
    if (pEnclInfo->lccCount == 0)
    {
        fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s, Encl %d_%d, This is not stand alone LCC, ignore.\n",
                              __FUNCTION__, 
                              pLocation->bus, 
                              pLocation->enclosure);

        return FBE_STATUS_OK;
    }

    status = fbe_encl_mgmt_get_lcc_info_ptr(pEnclMgmt, pLocation, &pLccInfo);

    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s Error in finding lccInfo for LCC(%d_%d_%d), status: 0x%X.\n",
                              __FUNCTION__,
                              pLocation->bus,
                              pLocation->enclosure,
                              pLocation->slot,
                              status);
    }

    /* Save the old LCC info. */
    oldLccInfo = *pLccInfo;
   
    /* Get the new LCC info. */
    status = fbe_api_enclosure_get_lcc_info(pLocation, pLccInfo);
    if (status != FBE_STATUS_OK) 
    {
        fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s Error getting LCC(%d_%d_%d) info, status: 0x%X.\n",
                              __FUNCTION__,
                              pLocation->bus,
                              pLocation->enclosure,
                              pLocation->slot,
                              status);
        return status;
    }

    fbe_encl_mgmt_derive_lcc_info(pEnclMgmt, pEnclInfo, pLccInfo);

    /* Suppress LCC Fault during FUP Activate */
    status = fbe_encl_mgmt_suppress_lcc_fault_if_needed(pEnclMgmt, pLocation, pLccInfo);
    if (status != FBE_STATUS_OK) 
    {
        fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "LCC(%d_%d_%d),suppress_lcc_fault_if_needed failed,status: 0x%X.\n",
                              pLocation->bus,
                              pLocation->enclosure,
                              pLocation->slot,
                              status);
    }

    status = fbe_encl_mgmt_log_lcc_status_change(pEnclMgmt, pLocation, pLccInfo, &oldLccInfo);
    fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt, 
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "LCC(%d_%d_%d) inserted:%d,faulted:%d\n",
                          pLocation->bus,
                          pLocation->enclosure,
                          pLocation->slot,
                          pLccInfo->inserted, 
                          pLccInfo->faulted);

    status = fbe_encl_mgmt_fup_handle_lcc_status_change(pEnclMgmt, 
                                                pLocation, 
                                                pLccInfo, 
                                                &oldLccInfo);
    if(status != FBE_STATUS_OK) 
    {
        fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "FUP:LCC(%d_%d_%d) fup_handle_lcc_status_change failed, status 0x%X.\n",
                              pLocation->bus,
                              pLocation->enclosure,
                              pLocation->slot,
                              status);

    }
    
    status = fbe_encl_mgmt_resume_prom_handle_lcc_status_change(pEnclMgmt, 
                                                                pLocation, 
                                                                pLccInfo, 
                                                                &oldLccInfo);
    if(status != FBE_STATUS_OK) 
    {
        fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "RP: LCC(%d_%d_%d) resume_prom_handle_status_change failed, status 0x%X.\n",
                              pLocation->bus,
                              pLocation->enclosure,
                              pLocation->slot,
                              status);

    }

    /*
     * Update the enclosure fault led.
     */
    status = fbe_encl_mgmt_update_encl_fault_led(pEnclMgmt, pLocation, FBE_ENCL_FAULT_LED_LCC_FLT);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt, 
                          FBE_TRACE_LEVEL_WARNING,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "LCC(%d_%d_%d), update_encl_fault_led failed, status: 0x%X.\n",
                          pLocation->bus, pLocation->enclosure, pLocation->slot, status);
    }

    /* Send LCC data change */
    fbe_base_environment_send_data_change_notification((fbe_base_environment_t*)pEnclMgmt,
                                                           FBE_CLASS_ID_ENCL_MGMT, 
                                                           FBE_DEVICE_TYPE_LCC, 
                                                           FBE_DEVICE_DATA_TYPE_GENERIC_INFO,
                                                           pLocation);
   
    
    return status;
}
     
/*!**************************************************************
 * fbe_encl_mgmt_process_subencl_lcc_status()
 ****************************************************************
 * @brief
 *  This function processes the encl component status change at
 *  startup time discovery or when object data change is detected.
 * 
 *  Check for fault in the specified location as well as the sibling
 *  lcc info. Log an event when either indicates a fault.
 * 
 *  Call fup_mgmt when insertion is detected at the specified location.
 *
 * @param pEnclMgmt - This is the object handle, or in our case the encl_mgmt.
 * @param subEnclObjectId - The subEncl object id.
 * @param pSubEnclLccLocation - The pointer to the subEncl Lcc location.
 *
 * @return fbe_status_t - 
 *
 * @author
 *  19-Sept-2011:  PHE - Created.
 *
 ****************************************************************/
static fbe_status_t 
fbe_encl_mgmt_process_subencl_lcc_status(fbe_encl_mgmt_t * pEnclMgmt,
                                     fbe_object_id_t subEnclObjectId,
                                     fbe_device_physical_location_t * pSubEnclLccLocation)
{
    fbe_sub_encl_info_t                 * pSubEnclInfo = NULL;
    fbe_lcc_info_t                      * pSubEnclLccInfo = NULL;
    fbe_lcc_info_t                        oldSubEnclLccInfo = {0};
    fbe_status_t                          status = FBE_STATUS_OK;
    fbe_encl_info_t                   * pEnclInfo = NULL;

    // input validation
    if(subEnclObjectId == FBE_OBJECT_ID_INVALID)
    {
        fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s Encl(%d_%d.%d), invalid subEnclObjectId.\n",
                              __FUNCTION__, 
                              pSubEnclLccLocation->bus, 
                              pSubEnclLccLocation->enclosure,
                              pSubEnclLccLocation->componentId);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    // use the location to get the encl info
    status = fbe_encl_mgmt_get_subencl_info_by_subencl_object_id(pEnclMgmt, 
                                                                 subEnclObjectId, 
                                                                 &pSubEnclInfo);
    if (status != FBE_STATUS_OK) 
    {
        /* The containing enclosure is not populated yet. We don't need to do 
         * anything here. When the containing enclosure becomes available, it will 
         * query the encl component status.
         */
        
         fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s Encl(%d_%d.%d), subEnclObjectid 0x%X is not populated, no action taken.\n",
                               __FUNCTION__, 
                              pSubEnclLccLocation->bus, 
                              pSubEnclLccLocation->enclosure,
                              pSubEnclLccLocation->componentId,
                              subEnclObjectId);
         return FBE_STATUS_OK;
    }
     
    /* Check the subEnclLcc fault */
    status = fbe_encl_mgmt_check_subencl_lcc_fault(pEnclMgmt, pSubEnclLccLocation);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s Encl(%d_%d.%d) LCC%d, check_subencl_lcc_fault failed, status: 0x%X.\n",
                              __FUNCTION__,
                              pSubEnclLccLocation->bus,
                              pSubEnclLccLocation->enclosure,
                              pSubEnclLccLocation->componentId,
                              pSubEnclLccLocation->slot,
                              status);
    }

    /* get the pointer to the subEnclLccInfo */
    status = fbe_encl_mgmt_get_lcc_info_ptr(pEnclMgmt, pSubEnclLccLocation, &pSubEnclLccInfo);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s Encl(%d_%d.%d) LCC%d, get_lcc_info_ptr failed, status: 0x%X.\n",
                              __FUNCTION__,
                              pSubEnclLccLocation->bus,
                              pSubEnclLccLocation->enclosure,
                              pSubEnclLccLocation->componentId,
                              pSubEnclLccLocation->slot,
                              status);
        return status;
    }

    /* Save the old subEnclLccInfo. */
    oldSubEnclLccInfo = *pSubEnclLccInfo;

    /* Get the new subEnclLccInfo */
    status = fbe_api_enclosure_get_lcc_info(pSubEnclLccLocation, pSubEnclLccInfo);
    if (status != FBE_STATUS_OK) 
    {
        fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s Encl(%d_%d.%d) LCC%d, Error getting LCC info,status:0x%X\n",
                              __FUNCTION__, 
                              pSubEnclLccLocation->bus, 
                              pSubEnclLccLocation->enclosure,
                              pSubEnclLccLocation->componentId,
                              pSubEnclLccLocation->slot,
                              status);
        return status;
    }

    status = fbe_encl_mgmt_get_encl_info_by_location(pEnclMgmt, pSubEnclLccLocation, &pEnclInfo);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s Encl(%d_%d.%d) LCC%d, Error getting encl info,status:0x%X\n",
                              __FUNCTION__,
                              pSubEnclLccLocation->bus,
                              pSubEnclLccLocation->enclosure,
                              pSubEnclLccLocation->componentId,
                              pSubEnclLccLocation->slot,
                              status);
        return status;
    }
    fbe_encl_mgmt_derive_lcc_info(pEnclMgmt, pEnclInfo, pSubEnclLccInfo);

    // send to fup for processing
    status = fbe_encl_mgmt_fup_handle_lcc_status_change(pEnclMgmt, 
                                                pSubEnclLccLocation, 
                                                pSubEnclLccInfo, 
                                                &oldSubEnclLccInfo);

    if(status != FBE_STATUS_OK) 
    {
        fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "Encl(%d_%d.%d) LCC%d, fup_handle_lcc_status_change failed, status:0x%X\n",
                              pSubEnclLccLocation->bus,
                              pSubEnclLccLocation->enclosure,
                              pSubEnclLccLocation->componentId,
                              pSubEnclLccLocation->slot,
                              status);

    }

    return status;
} //fbe_encl_mgmt_process_subencl_lcc_status
     
/*!**************************************************************
 * fbe_encl_mgmt_check_subencl_lcc_fault()
 ****************************************************************
 * @brief
 *  This function checks the subencl LCC fault.
 *  We don't log the subEncl LCC fault because it is not a FRU.
 *  When the subEncl LCC is faulted, we log the containing LCC as faulted and
 *  we also update the containing LCC fault status.
 *
 * @param pEnclMgmt - This is the object handle, or in our case the encl_mgmt.
 * @param pSubEnclLccLocation - The pointer to the subEncl Lcc location.
 *
 * @return fbe_status_t - 
 * 
 * @Note
 *  Do not update the subEncl LCC info in this functions.
 * 
 * @author
 *  01-Feb-2012:  PHE - Created.
 *
 ****************************************************************/     
static fbe_status_t 
fbe_encl_mgmt_check_subencl_lcc_fault(fbe_encl_mgmt_t * pEnclMgmt,
                                     fbe_device_physical_location_t * pSubEnclLccLocation)
{
    fbe_encl_info_t                   * pEnclInfo = NULL;
    fbe_lcc_info_t                    * pEnclLccInfo = NULL;
    fbe_lcc_info_t                      oldEnclLccInfo = {0};
    fbe_lcc_info_t                      subEnclLccInfo = {0};
    fbe_device_physical_location_t      enclLccLocation = {0};
    fbe_device_physical_location_t      tempSubEnclLccLocation = {0};
    fbe_u32_t                           subEnclIndex = 0;
    fbe_bool_t                          newFault = FBE_FALSE;
    fbe_status_t                        status = FBE_STATUS_OK;

    status = fbe_encl_mgmt_get_encl_info_by_location(pEnclMgmt, pSubEnclLccLocation, &pEnclInfo);
    if (status != FBE_STATUS_OK) 
    {
        /* The containing enclosure is not populated yet. We don't need to do 
         * anything here. When the containing enclosure becomes available, it will 
         * query the encl component status.
         */
        
         fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s Encl(%d_%d.%d) Encl is not populated, no action taken.\n",
                               __FUNCTION__, 
                              pSubEnclLccLocation->bus, 
                              pSubEnclLccLocation->enclosure,
                              pSubEnclLccLocation->componentId);
         return FBE_STATUS_OK;
    }

    /* Get the location of the Encl LCC (not the subencl LCC) */
    enclLccLocation = *pSubEnclLccLocation;
    enclLccLocation.componentId = 0;

    /* Get the pointer to the Encl LCC info */
    status = fbe_encl_mgmt_get_lcc_info_ptr(pEnclMgmt, &enclLccLocation, &pEnclLccInfo);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s LCC(%d_%d_%d) get_lcc_info_ptr failed, status: 0x%X.\n",
                              __FUNCTION__,
                              enclLccLocation.bus,
                              enclLccLocation.enclosure,
                              enclLccLocation.slot,
                              status);
        return status;
    }

    /* Save the original Encl LCC info which will be used later. */
    oldEnclLccInfo = *pEnclLccInfo;

    /* Loop through each subenclosure and determine if either ee is faulted 
     * by making the call to the physical package to get the NEW subEncl lccInfo.
     */
    tempSubEnclLccLocation = *pSubEnclLccLocation;
    for(subEnclIndex = 0; subEnclIndex < pEnclInfo->subEnclCount; subEnclIndex++)
    {
        if(pEnclInfo->subEnclInfo[subEnclIndex].objectId != FBE_OBJECT_ID_INVALID) 
        {
            // need to set the location cid to get the subencl lccinfo
            tempSubEnclLccLocation.componentId = pEnclInfo->subEnclInfo[subEnclIndex].location.componentId;
    
            fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt, 
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s Encl(%d_%d.%d) LCC%d, ObjId:0x%X.\n",
                                  __FUNCTION__, 
                                  tempSubEnclLccLocation.bus, 
                                  tempSubEnclLccLocation.enclosure,
                                  tempSubEnclLccLocation.componentId,
                                  tempSubEnclLccLocation.slot,
                                  pEnclInfo->subEnclInfo[subEnclIndex].objectId);
    
            status = fbe_api_enclosure_get_lcc_info(&tempSubEnclLccLocation, &subEnclLccInfo);
    
            if(status != FBE_STATUS_OK) 
            {
                fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt, 
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s Encl(%d_%d.%d) LCC%d, failed to get LCC info, status 0x%X.\n",
                                  __FUNCTION__, 
                                  tempSubEnclLccLocation.bus, 
                                  tempSubEnclLccLocation.enclosure,
                                  tempSubEnclLccLocation.componentId,
                                  tempSubEnclLccLocation.slot,
                                  status);
                return status;
            }
    
            /* Suppress subEncl LCC Fault during FUP Activate */
            status = fbe_encl_mgmt_suppress_lcc_fault_if_needed(pEnclMgmt, &tempSubEnclLccLocation, &subEnclLccInfo);
            if (status != FBE_STATUS_OK) 
            {
                fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt, 
                                      FBE_TRACE_LEVEL_WARNING,
                                      FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                      "Encl(%d_%d.%d) LCC%d,suppress_lcc_fault_if_needed failed,status: 0x%X.\n",
                                      tempSubEnclLccLocation.bus, 
                                      tempSubEnclLccLocation.enclosure,
                                      tempSubEnclLccLocation.componentId,
                                      tempSubEnclLccLocation.slot,
                                      status);
            }
            
            // save new fault status
            newFault = newFault || subEnclLccInfo.faulted;
        }
    }
        
    // if fault status changed, newly faulted, set it in encl.lccinfo
    if(newFault != oldEnclLccInfo.faulted)
    {
        /* Update the containing LCC fault status */
        pEnclLccInfo->faulted = newFault;

        /* Log the event. */
        status = fbe_encl_mgmt_log_lcc_status_change(pEnclMgmt, &enclLccLocation, pEnclLccInfo, &oldEnclLccInfo);
        if(status != FBE_STATUS_OK) 
        {
            fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s LCC(%d_%d_%d) failed to log LCC status change, status 0x%X.\n",
                              __FUNCTION__, 
                              enclLccLocation.bus, 
                              enclLccLocation.enclosure,
                              enclLccLocation.slot,
                              status);
        }

        /*
         * Update the enclosure fault led.
         */
        status = fbe_encl_mgmt_update_encl_fault_led(pEnclMgmt, &enclLccLocation, FBE_ENCL_FAULT_LED_LCC_FLT);
        if(status != FBE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s LCC(%d_%d_%d), update_encl_fault_led failed, status: 0x%X.\n",
                              __FUNCTION__,
                              enclLccLocation.bus, 
                              enclLccLocation.enclosure,
                              enclLccLocation.slot,
                              status);
        }
    
        /* Send LCC data change */
        fbe_base_environment_send_data_change_notification((fbe_base_environment_t*)pEnclMgmt,
                                                               FBE_CLASS_ID_ENCL_MGMT, 
                                                               FBE_DEVICE_TYPE_LCC, 
                                                               FBE_DEVICE_DATA_TYPE_GENERIC_INFO,
                                                               &enclLccLocation);
    }
    
    return status;
}

/*!**************************************************************
 * fbe_encl_mgmt_process_enclosure_status()
 ****************************************************************
 * @brief
 *  This function processes the Enclosure status change at
 *  startup time discovery or at runtime.
 *
 * @param pEnclMgmt - This is the object handle, or in our case the encl_mgmt.
 * @param objectId - 
 * @param pLocation - The location of the LCC.
 *
 * @return fbe_status_t - 
 *
 * @author
 *  24-Mar-2011:  Joe Perry - Created.
 *
 ****************************************************************/
static fbe_status_t 
fbe_encl_mgmt_process_enclosure_status(fbe_encl_mgmt_t * pEnclMgmt,
                                       fbe_object_id_t objectId,
                                       fbe_device_physical_location_t * pLocation)
{
    fbe_encl_info_t                         * pEnclInfo = NULL;
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_enclosure_mgmt_get_shutdown_info_t  getShutdownInfo;
    fbe_enclosure_mgmt_get_overtemp_info_t  getOverTempInfo={0};
    fbe_base_object_t                       * base_object = (fbe_base_object_t *)pEnclMgmt;
    fbe_u8_t                                deviceStr[FBE_EVENT_LOG_MESSAGE_STRING_LENGTH];
    fbe_bool_t                              processorEncl = FALSE;

    if(objectId == FBE_OBJECT_ID_INVALID)
    {
        fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt, 
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s, encl %d_%d, invalid object id.\n",
                                  __FUNCTION__, pLocation->bus, pLocation->enclosure);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_encl_mgmt_get_encl_info_by_location(pEnclMgmt, pLocation, &pEnclInfo);

    if (status != FBE_STATUS_OK) 
    {
        /* The enclosure was not initialized yet.  
         * When the enclosure gets initialized, 
         * it will call fbe_encl_mgmt_process_enclosure_status anyway.
         * Just return FBE_STATUS_OK here.
         */
        fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt, 
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s, encl %d_%d, enclInfo not populated, ignore the change.\n",
                                  __FUNCTION__, pLocation->bus, pLocation->enclosure);

        return FBE_STATUS_OK;
    }
   
    status = fbe_base_env_create_device_string(FBE_DEVICE_TYPE_ENCLOSURE, 
                                               pLocation, 
                                               &deviceStr[0],
                                               FBE_DEVICE_STRING_LENGTH);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace(base_object, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                              "%s, Failed to create device string.\n", 
                              __FUNCTION__); 

        return status;
    }
    
    if((pLocation->bus != FBE_XPE_PSEUDO_BUS_NUM) &&
       (pLocation->enclosure != FBE_XPE_PSEUDO_ENCL_NUM))
    {
        /* This is not XPE enclosure. 
         * Get the new Shutdown info & determine if it has changed
         */
        status = fbe_api_enclosure_get_shutdown_info(pLocation, &getShutdownInfo);
        if (status != FBE_STATUS_OK) 
        {
            fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt, 
                                  FBE_TRACE_LEVEL_WARNING,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s Error getting Shutdown info, status: 0x%X.\n",
                                  __FUNCTION__, status);
        }
        else if(getShutdownInfo.shutdownReason != pEnclInfo->shutdownReason)
        {
            /* If debounce in progress? */
            if (pEnclInfo->shutdownReasonDebounceInProgress)
            {
                /* Do nothing since debounce is in progress*/
            }
            /* Did the debounce start?*/
            else if((!pEnclInfo->shutdownReasonDebounceInProgress) &&
                    (!pEnclInfo->shutdownReasonDebounceComplete))
            {
                /* Start the timer */
                pEnclInfo->shutdownReasonDebounceInProgress = FBE_TRUE;
                pEnclInfo->debounceStartTime = fbe_get_time();
                status = fbe_lifecycle_set_cond(&FBE_LIFECYCLE_CONST_DATA(encl_mgmt), 
                                                base_object, 
                                                FBE_ENCL_MGMT_SHUTDOWN_DEBOUNCE_TIMER_COND);
                fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt, 
                                      FBE_TRACE_LEVEL_INFO,
                                      FBE_TRACE_MESSAGE_ID_INFO,
                                      "ShutdownReason debounce started. Changed from %s to %s\n",
                                      fbe_encl_mgmt_get_enclShutdownReasonString(pEnclInfo->shutdownReason), 
                                      fbe_encl_mgmt_get_enclShutdownReasonString(getShutdownInfo.shutdownReason));
            }
            else
            {
                fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt, 
                                      FBE_TRACE_LEVEL_INFO,
                                      FBE_TRACE_MESSAGE_ID_INFO,
                                      "ShutdownReason changed from %s to %s\n",
                                      fbe_encl_mgmt_get_enclShutdownReasonString(pEnclInfo->shutdownReason), 
                                      fbe_encl_mgmt_get_enclShutdownReasonString(getShutdownInfo.shutdownReason));
                pEnclInfo->shutdownReason = getShutdownInfo.shutdownReason;
    
                // check whether we should delay the enclosure shutdown timer.
                if ((pEnclMgmt->base_environment.processorEnclType == XPE_ENCLOSURE_TYPE) && 
                    (pEnclInfo->enclType == FBE_ESP_ENCL_TYPE_VIPER) && 
                    (pEnclInfo->location.bus == pEnclMgmt->base_environment.bootEnclLocation.bus) && 
                    (pEnclInfo->location.enclosure == pEnclMgmt->base_environment.bootEnclLocation.enclosure))
                {
                    status = fbe_lifecycle_set_cond(&FBE_LIFECYCLE_CONST_DATA(encl_mgmt), 
                                                    base_object, 
                                                    FBE_ENCL_MGMT_CHECK_ENCL_SHUTDOWN_DELAY_COND);
                    if (status != FBE_STATUS_OK)
                    {
                        fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt, 
                                              FBE_TRACE_LEVEL_ERROR,
                                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                              "%s can't set condition FBE_ENCL_MGMT_CHECK_ENCL_SHUTDOWN_DELAY_COND, status: 0x%X.\n",
                                              __FUNCTION__, status);
                    }
                }
    
                // generate appropriate event
                if (pEnclInfo->shutdownReason != FBE_ESES_SHUTDOWN_REASON_NOT_SCHEDULED)
                {
                    fbe_event_log_write(ESP_ERROR_ENCL_SHUTDOWN_DETECTED,
                                NULL, 0, 
                                "%s %s", 
                                &deviceStr[0],
                                fbe_encl_mgmt_get_enclShutdownReasonString(pEnclInfo->shutdownReason));
                }
                else
                {
                    fbe_event_log_write(ESP_INFO_ENCL_SHUTDOWN_CLEARED,
                                NULL, 0, 
                                "%s", 
                                &deviceStr[0]);
                }
            }
        }
    }

    status = fbe_base_env_is_processor_encl((fbe_base_environment_t *)pEnclMgmt,
                                             pLocation,
                                             &processorEncl);
    if (status != FBE_STATUS_OK) {
        fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s Error in checking whether it is PE, status 0x%x.\n",
                              __FUNCTION__, status);

        return status;
    }

    /* OverTemp warning and failure not valid on xPE */
    if (!processorEncl ||
        ((pEnclMgmt->base_environment.processorEnclType != XPE_ENCLOSURE_TYPE) && 
         (pEnclMgmt->base_environment.processorEnclType != VPE_ENCLOSURE_TYPE)))
    {
        /* Get new Over Temperature status */
        status = fbe_api_enclosure_get_overtemp_info(pLocation, &getOverTempInfo);
        if (status != FBE_STATUS_OK) 
        {
            fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt, 
                                  FBE_TRACE_LEVEL_WARNING,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s Error getting OverTemp info, status: 0x%X.\n",
                                  __FUNCTION__, status);
        }
        else
        { 
            /* check OverTemp warning */
            if(getOverTempInfo.overTempWarning != pEnclInfo->overTempWarning )
            {
                fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt, 
                                      FBE_TRACE_LEVEL_INFO,
                                      FBE_TRACE_MESSAGE_ID_INFO,
                                      "%s, OverTemp Warning changed from %d to %d\n",
                                      __FUNCTION__, 
                                      pEnclInfo->overTempWarning,
                                      getOverTempInfo.overTempWarning);

                // generate appropriate event
                if (getOverTempInfo.overTempWarning != FBE_FALSE)
                {
                    fbe_event_log_write(ESP_INFO_ENCL_OVER_TEMP_WARNING,
                                NULL, 0, 
                                "%s", 
                                &deviceStr[0]);
                }
                else if (getOverTempInfo.overTempWarning == FBE_FALSE)
                {
                    fbe_event_log_write(ESP_INFO_ENCL_OVER_TEMP_CLEARED,
                                NULL, 0, 
                                "%s %s", 
                                &deviceStr[0],
                                "Warning");
                }

                pEnclInfo->overTempWarning = getOverTempInfo.overTempWarning;

                status = fbe_encl_mgmt_update_encl_fault_led(pEnclMgmt, pLocation, FBE_ENCL_FAULT_LED_OVERTEMP_FLT);
                if(status != FBE_STATUS_OK)
                {
                    fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt, 
                                  FBE_TRACE_LEVEL_WARNING,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "Encl(%d_%d), update_encl_fault_led failed for OverTemp warning, status: 0x%X.\n",
                                  pLocation->bus, pLocation->enclosure, status);
                }
            }
            
            /* check OverTemp failure */
            if(getOverTempInfo.overTempFailure != pEnclInfo->overTempFailure)
            {
                fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt, 
                                      FBE_TRACE_LEVEL_INFO,
                                      FBE_TRACE_MESSAGE_ID_INFO,
                                      "%s, OverTemp Failure changed from %d to %d\n",
                                      __FUNCTION__, 
                                      pEnclInfo->overTempFailure,
                                      getOverTempInfo.overTempFailure);

                // generate appropriate event
                if (getOverTempInfo.overTempFailure != FBE_FALSE)
                {
                    fbe_event_log_write(ESP_ERROR_ENCL_OVER_TEMP_DETECTED,
                                NULL, 0, 
                                "%s", 
                                &deviceStr[0]);
                }
                else if (getOverTempInfo.overTempFailure == FBE_FALSE)
                {
                    fbe_event_log_write(ESP_INFO_ENCL_OVER_TEMP_CLEARED,
                                NULL, 0, 
                                "%s %s", 
                                &deviceStr[0],
                                "Failure");
                }

                pEnclInfo->overTempFailure = getOverTempInfo.overTempFailure;

                status = fbe_encl_mgmt_update_encl_fault_led(pEnclMgmt, pLocation, FBE_ENCL_FAULT_LED_OVERTEMP_FLT);
                if(status != FBE_STATUS_OK)
                {
                    fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt, 
                                  FBE_TRACE_LEVEL_WARNING,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "Encl(%d_%d), update_encl_fault_led failed for OverTemp failure, status: 0x%X.\n",
                                  pLocation->bus, pLocation->enclosure, status);
                }

            }
        }
    }

    /*
     * Checking Cache status (ShutdownReason only relevant for Vault DAE)
     */
    if (((pEnclMgmt->base_environment.processorEnclType == XPE_ENCLOSURE_TYPE) ||
         (pEnclMgmt->base_environment.processorEnclType == VPE_ENCLOSURE_TYPE)) &&
        (pEnclInfo->location.bus == pEnclMgmt->base_environment.bootEnclLocation.bus) && 
        (pEnclInfo->location.enclosure == pEnclMgmt->base_environment.bootEnclLocation.enclosure))
    {
        status = fbe_lifecycle_set_cond(&FBE_LIFECYCLE_CONST_DATA(encl_mgmt), 
                                        base_object, 
                                        FBE_ENCL_MGMT_PROCESS_CACHE_STATUS_COND);
        if (status != FBE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt, 
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s can't set condition FBE_ENCL_MGMT_PROCESS_CACHE_STATUS_COND, status: 0x%X.\n",
                                  __FUNCTION__, status);
        }
    }

    /* Send the notification for enclosure */
    fbe_base_environment_send_data_change_notification((fbe_base_environment_t *) pEnclMgmt,
                                                           FBE_CLASS_ID_ENCL_MGMT, 
                                                           FBE_DEVICE_TYPE_ENCLOSURE, 
                                                           FBE_DEVICE_DATA_TYPE_GENERIC_INFO,
                                                           pLocation);

    return status;
}   // end of fbe_encl_mgmt_process_enclosure_status

/*!**************************************************************
 * fbe_encl_mgmt_process_encl_fault_led_status()
 ****************************************************************
 * @brief
 *  This function processes the Enclosure fault led status at
 *  startup time discovery or at runtime. It includes the XPE,
 *  DPE and DAE enclosure fault led status processing.
 * @param pEnclMgmt - This is the object handle, or in our case the encl_mgmt.
 * @param objectId - 
 * @param pLocation - The location of the LCC.
 *
 * @return fbe_status_t - 
 *
 * @author
 *  24-Mar-2011:  Joe Perry - Created.
 *
 ****************************************************************/
static fbe_status_t 
fbe_encl_mgmt_process_encl_fault_led_status(fbe_encl_mgmt_t * pEnclMgmt,
                                       fbe_object_id_t objectId,
                                       fbe_device_physical_location_t * pLocation)
{
    fbe_encl_info_t                         * pEnclInfo = NULL;
    fbe_encl_fault_led_info_t                 enclFaultLedInfo = {0};
    fbe_status_t                              status = FBE_STATUS_OK;

    if(objectId == FBE_OBJECT_ID_INVALID)
    {
        fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt, 
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s, encl %d_%d, invalid object id.\n",
                                  __FUNCTION__, pLocation->bus, pLocation->enclosure);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_encl_mgmt_get_encl_info_by_location(pEnclMgmt, pLocation, &pEnclInfo);

    if (status != FBE_STATUS_OK) 
    {
        /* The enclosure was not initialized yet.  
         * When the enclosure gets initialized, 
         * it will call fbe_encl_mgmt_process_encl_fault_led_status anyway.
         * Just return FBE_STATUS_OK here.
         */
        fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt, 
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s, encl %d_%d, enclInfo not populated, ignore the change.\n",
                                  __FUNCTION__, pLocation->bus, pLocation->enclosure);

        return FBE_STATUS_OK;
    }

    if ((pEnclMgmt->base_environment.processorEnclType == DPE_ENCLOSURE_TYPE) &&
             (pLocation->bus == 0) && (pLocation->enclosure == 0))
    {
        status = fbe_api_get_board_object_id(&objectId);
        if (status != FBE_STATUS_OK) {
            fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt,
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "Encl(%d_%d), failed to get board object, status: 0x%X.\n",
                                  pLocation->bus,
                                  pLocation->enclosure,
                                  status);
            return status;
        }
    }

    status = fbe_api_enclosure_get_encl_fault_led_info(objectId, &enclFaultLedInfo);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt,  
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                              "%s, Encl(%d_%d), Failed to get enclFaultLedInfo.\n", 
                              __FUNCTION__,
                              pLocation->bus,
                              pLocation->enclosure); 

        return status;
    }

    if(pEnclInfo->enclFaultLedStatus != enclFaultLedInfo.enclFaultLedStatus) 
    {
        pEnclInfo->enclFaultLedStatus = enclFaultLedInfo.enclFaultLedStatus;

        fbe_base_environment_send_data_change_notification((fbe_base_environment_t *) pEnclMgmt,
                                                               FBE_CLASS_ID_ENCL_MGMT, 
                                                               FBE_DEVICE_TYPE_ENCLOSURE, 
                                                               FBE_DEVICE_DATA_TYPE_GENERIC_INFO,
                                                               pLocation);


        fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt, 
                              FBE_TRACE_LEVEL_INFO, 
                              FBE_TRACE_MESSAGE_ID_INFO, 
                              "Encl(%d_%d) enclFaultLedStatus %s.\n",
                              pLocation->bus,
                              pLocation->enclosure,
                              fbe_base_env_decode_led_status(pEnclInfo->enclFaultLedStatus));

        if(enclFaultLedInfo.enclFaultLedStatus == FBE_LED_STATUS_OFF) 
        {
            /* It is possible that the enclosure fault led was turned off by the peer side.
             * The peer side thought there was no fault. 
             * However, this SP might think there was a fault on this own side.
             * For example, we only turn on the enclosure fault led for local connector fault.
             * If this SP turned on the enclosure fault led for the local connector fault.
             * the peer SP could turn off the enclosure fault led because 
             * there was no fault on peer side connectors. 
             * So we need to turn on the encl fault led again if needed when we see the enclosure
             * fault led was turned off.
             */
            status = fbe_encl_mgmt_update_encl_fault_led(pEnclMgmt, pLocation, FBE_ENCL_FAULT_LED_CONNECTOR_FLT);
            if(status != FBE_STATUS_OK)
            {
                fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt, 
                                  FBE_TRACE_LEVEL_WARNING,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "Encl(%d_%d), update_encl_fault_led failed, status: 0x%X.\n",
                                  pLocation->bus, pLocation->enclosure, status);
            }
        }
    }

    if(pEnclInfo->enclFaultLedReason != enclFaultLedInfo.enclFaultLedReason) 
    {
        pEnclInfo->enclFaultLedReason = enclFaultLedInfo.enclFaultLedReason;

        fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt, 
                              FBE_TRACE_LEVEL_INFO, 
                              FBE_TRACE_MESSAGE_ID_INFO, 
                              "Encl(%d_%d) enclFaultLedReason %s(0x%llX).\n",
                              pLocation->bus,
                              pLocation->enclosure,
                              fbe_base_env_decode_encl_fault_led_reason(pEnclInfo->enclFaultLedReason),
                              pEnclInfo->enclFaultLedReason);
    }

    return FBE_STATUS_OK;
}
/*!**************************************************************
 * fbe_encl_mgmt_process_connector_status()
 ****************************************************************
 * @brief
 *  This function processes the Connector status change at startup time discovery or
 *  at runtime.
 *
 * @param pEnclMgmt - This is the object handle, or in our case the encl_mgmt.
 * @param objectId - 
 * @param pLocation - The location of the LCC.
 *
 * @return fbe_status_t - 
 *
 * @author
 *  23-Aug-2011:  PHE - Created.
 *
 ****************************************************************/
static fbe_status_t 
fbe_encl_mgmt_process_connector_status(fbe_encl_mgmt_t * pEnclMgmt,
                               fbe_object_id_t objectId,
                               fbe_device_physical_location_t * pLocation)
{
    fbe_encl_info_t                    * pEnclInfo = NULL;
    fbe_connector_info_t                 oldConnectorInfo = {0};
    fbe_status_t                         status = FBE_STATUS_OK;

    if(objectId == FBE_OBJECT_ID_INVALID)
    {
        fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt, 
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s, encl %d_%d, invalid object id.\n",
                                  __FUNCTION__, pLocation->bus, pLocation->enclosure);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_encl_mgmt_get_encl_info_by_location(pEnclMgmt, pLocation, &pEnclInfo);

    if (status != FBE_STATUS_OK) 
    {
        /* The enclosure was not initialized yet.  
         * When the enclosure gets initialized, 
         * it will call fbe_encl_mgmt_process_enclosure_status anyway.
         * Just return FBE_STATUS_OK here.
         */
        fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt, 
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s, encl %d_%d, enclInfo not populated, ignore the change.\n",
                                  __FUNCTION__, pLocation->bus, pLocation->enclosure);

        return FBE_STATUS_OK;
    }
   
    if(pLocation->slot >= pEnclInfo->connectorCount) 
    {
        fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt, 
                                      FBE_TRACE_LEVEL_ERROR,
                                      FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                      "%s connectorLocation %d exceeds max count %d.\n",
                                      __FUNCTION__, pLocation->slot, pEnclInfo->connectorCount);

        return FBE_STATUS_INSUFFICIENT_RESOURCES;
    }
    /* Save the old Connector info. */
    oldConnectorInfo = pEnclInfo->connectorInfo[pLocation->slot];
   
    /* Get the new Connector info. */
    status = fbe_api_enclosure_get_connector_info(pLocation, &pEnclInfo->connectorInfo[pLocation->slot]);
    if (status != FBE_STATUS_OK) 
    {
        fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s Error getting connector info, status: 0x%X.\n",
                              __FUNCTION__, status);
        return status;
    }

    status = fbe_encl_mgmt_check_connector_events(pEnclMgmt, 
                                       pLocation,
                                       &pEnclInfo->connectorInfo[pLocation->slot],
                                       &oldConnectorInfo);

    
    /*
     * Update the enclosure fault led.
     */
    status = fbe_encl_mgmt_update_encl_fault_led(pEnclMgmt, pLocation, FBE_ENCL_FAULT_LED_CONNECTOR_FLT);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt, 
                          FBE_TRACE_LEVEL_WARNING,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "CONN(%d_%d_%d), update_encl_fault_led failed, status: 0x%X.\n",
                          pLocation->bus, pLocation->enclosure, pLocation->slot, status);
    }

    /* Send Connector data change */
    fbe_base_environment_send_data_change_notification((fbe_base_environment_t*)pEnclMgmt,
                                                           FBE_CLASS_ID_ENCL_MGMT, 
                                                           FBE_DEVICE_TYPE_CONNECTOR, 
                                                           FBE_DEVICE_DATA_TYPE_GENERIC_INFO,
                                                           pLocation);
   
    
    return status;
}

/*!**************************************************************
 * fbe_encl_mgmt_check_connector_events()
 ****************************************************************
 * @brief
 *  This function checks the connector events and will log the event if needed.
 *
 * @param pEnclMgmt -
 * @param pLocation - The location of the connector.
 * @param pNewConnectorInfo - The new connector info.
 * @param pOldConnectorInfo - The old connector info.
 *
 * @return fbe_status_t - 
 *
 * @author
 *  23-Aug-2011: PHE - Created.
 *
 ****************************************************************/
static fbe_status_t 
fbe_encl_mgmt_check_connector_events(fbe_encl_mgmt_t * pEnclMgmt,
                             fbe_device_physical_location_t * pLocation,
                             fbe_connector_info_t * pNewConnectorInfo,
                             fbe_connector_info_t * pOldConnectorInfo)
{
    fbe_u8_t                        deviceStr[FBE_DEVICE_STRING_LENGTH];
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_device_physical_location_t  location = *pLocation;
   
    if(!pNewConnectorInfo->isLocalFru) 
    {
        // Only care about the local connector events.
        return FBE_STATUS_OK;
    }
    /* here we use bank to indicate the type of the connector,
      */
    location.bank = pNewConnectorInfo->connectorType;
    location.componentId = pNewConnectorInfo->connectorId;
    
    status = fbe_base_env_create_device_string(FBE_DEVICE_TYPE_CONNECTOR,
                                               &location,
                                               &deviceStr[0],
                                               FBE_DEVICE_STRING_LENGTH);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                              "%s, Failed to create device string for CONNECTOR.\n", 
                              __FUNCTION__); 

        return status;
    }

    /* check for insertion/removal */
    if (pNewConnectorInfo->inserted && !pOldConnectorInfo->inserted)
    {
        fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s is inserted.\n",
                              &deviceStr[0]);

        fbe_event_log_write(ESP_INFO_CONNECTOR_INSERTED,
                        NULL, 0,
                        "%s", 
                        &deviceStr[0]);

    }
    else if (!pNewConnectorInfo->inserted && pOldConnectorInfo->inserted)
    {
        fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s is removed.\n",
                              &deviceStr[0]);

        fbe_event_log_write(ESP_INFO_CONNECTOR_REMOVED,
                                NULL, 0,
                                "%s", 
                                &deviceStr[0]);

    }
    
    /* check for cableStatus */
    if (pNewConnectorInfo->cableStatus != pOldConnectorInfo->cableStatus)
    {
        fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s CS changed from %s to %s.\n",
                              &deviceStr[0],
                              fbe_base_env_decode_connector_cable_status(pOldConnectorInfo->cableStatus),
                              fbe_base_env_decode_connector_cable_status(pNewConnectorInfo->cableStatus));

        fbe_event_log_write(ESP_INFO_CONNECTOR_STATUS_CHANGE,
                                NULL, 0,
                                "%s %s %s", 
                                &deviceStr[0],
                                fbe_base_env_decode_connector_cable_status(pOldConnectorInfo->cableStatus),
                                fbe_base_env_decode_connector_cable_status(pNewConnectorInfo->cableStatus));

    }
        
    return status;
}

/*!**************************************************************
 * @fn fbe_encl_mgmt_processReceivedCmiMessage(fbe_encl_mgmt_t * pEnclMgmt, 
 *                                      fbe_encl_mgmt_cmi_packet_t *pEnclCmiPkt)
 ****************************************************************
 * @brief
 *  This function gets called when recieving the CMI message with 
 *  FBE_CMI_EVENT_MESSAGE_RECEIVED.
 *
 * @param pCoolingMgmt - 
 * @param pCoolingCmiPkt - pointer to user message of CMI message info.
 *
 * @return fbe_status_t -
 *   FBE_STATUS_MORE_PROCESSING_REQUIRED - need to further handled by the base environment.
 *   FBE_STATUS_OK - successful.
 *   others - failed.
 * 
 * @author
 *  25-Jan-2010:  Created. GB
 *  28-Apr-2011: PHE - updated.
 ****************************************************************/
static fbe_status_t 
fbe_encl_mgmt_processReceivedCmiMessage(fbe_encl_mgmt_t * pEnclMgmt, 
                                        fbe_encl_mgmt_cmi_packet_t *pEnclCmiPkt)
{
    fbe_base_environment_t             * pBaseEnv = (fbe_base_environment_t *)pEnclMgmt;
    fbe_base_environment_cmi_message_t * pBaseCmiMsg = (fbe_base_environment_cmi_message_t *)pEnclCmiPkt;
    fbe_base_env_fup_cmi_packet_t      * pFupCmiPkt = (fbe_base_env_fup_cmi_packet_t *)pEnclCmiPkt;
    fbe_encl_mgmt_encl_info_cmi_packet_t *pEnclInfoPkt = (fbe_encl_mgmt_encl_info_cmi_packet_t *)pEnclCmiPkt;
    fbe_status_t                         status = FBE_STATUS_OK;

    switch (pBaseCmiMsg->opcode)
    {
        case FBE_BASE_ENV_CMI_MSG_FUP_PEER_PERMISSION_GRANT:
            // peer has granted permission to fup
            status = fbe_base_env_fup_processGotPeerPerm(pBaseEnv, 
                                                         pFupCmiPkt->pRequestorWorkItem);
            break;

        case FBE_BASE_ENV_CMI_MSG_FUP_PEER_PERMISSION_DENY:
            // peer has denied permission to sart fup
            status = fbe_base_env_fup_processDeniedPeerPerm(pBaseEnv,
                                                            pFupCmiPkt->pRequestorWorkItem);
            break;

        case FBE_BASE_ENV_CMI_MSG_FUP_PEER_PERMISSION_REQUEST:
            // peer requests permission to fup
            status = fbe_base_env_fup_processRequestForPeerPerm(pBaseEnv, pFupCmiPkt);
            break;

        case FBE_BASE_ENV_CMI_MSG_USER_MODIFY_WWN_SEED_FLAG_CHANGE:
            status = fbe_encl_mgmt_process_user_modified_wwn_seed_flag_change(pEnclMgmt, pEnclCmiPkt);
            break;

        case FBE_BASE_ENV_CMI_MSG_USER_MODIFIED_SYS_ID_FLAG:
            status = fbe_encl_mgmt_process_user_modified_sys_id_flag_change(pEnclMgmt, pEnclCmiPkt);
            break;

        case FBE_BASE_ENV_CMI_MSG_PERSISTENT_SYS_ID_CHANGE:
            status = fbe_encl_mgmt_process_persistent_sys_id_change(pEnclMgmt, pEnclCmiPkt);
            break;

        case FBE_BASE_ENV_CMI_MSG_ENCL_CABLING_REQUEST:
            status = fbe_encl_mgmt_process_encl_cabling_request(pEnclMgmt, &pEnclInfoPkt->encl_info);
            break;

        case FBE_BASE_ENV_CMI_MSG_ENCL_CABLING_ACK:
            status = fbe_encl_mgmt_process_encl_cabling_ack(pEnclMgmt, &pEnclInfoPkt->encl_info);
            break;

        case FBE_BASE_ENV_CMI_MSG_ENCL_CABLING_NACK:
            status = fbe_encl_mgmt_process_encl_cabling_nack(pEnclMgmt, &pEnclInfoPkt->encl_info);
            break;
        case FBE_BASE_ENV_CMI_MSG_CACHE_STATUS_UPDATE:
             status = fbe_encl_mgmt_process_peer_cache_status_update(pEnclMgmt, 
                                                                 pEnclCmiPkt);
             break;
        case FBE_BASE_ENV_CMI_MSG_PEER_SP_ALIVE:
            // check if our earlier perm request failed
            status = fbe_encl_mgmt_fup_new_contact_init_upgrade(pEnclMgmt);
            break;
        default:
            status = FBE_STATUS_MORE_PROCESSING_REQUIRED;
            break;
    }

    return(status);

} //fbe_encl_mgmt_processReceivedCmiMessage


/*!**************************************************************
 *  @fn fbe_encl_mgmt_processPeerNotPresent(fbe_encl_mgmt_t * pEnclMgmt,
 *                                        fbe_encl_mgmt_cmi_packet_t * pEnclCmiPkt)
 ****************************************************************
 * @brief
 *  This function gets called when recieving the CMI message with 
 *  FBE_CMI_EVENT_PEER_NOT_PRESENT.
 *
 * @param pPsMgmt - 
 * @param pPsCmiPkt - pointer to user message of CMI message info.
 *
 * @return fbe_status_t -
 *   FBE_STATUS_MORE_PROCESSING_REQUIRED - need to further handled by the base environment.
 *   FBE_STATUS_OK - successful.
 *   others - failed.
 * 
 * @author
 *  27-Apr-2011: PHE - Created.
 ****************************************************************/
static fbe_status_t 
fbe_encl_mgmt_processPeerNotPresent(fbe_encl_mgmt_t * pEnclMgmt, 
                                    fbe_encl_mgmt_cmi_packet_t * pEnclCmiPkt)
{
    fbe_base_environment_cmi_message_t * pBaseCmiMsg = (fbe_base_environment_cmi_message_t *)pEnclCmiPkt;
    fbe_base_env_fup_cmi_packet_t      * pFupCmiPkt = (fbe_base_env_fup_cmi_packet_t *)pEnclCmiPkt;
    fbe_status_t                         status = FBE_STATUS_OK;

    switch (pBaseCmiMsg->opcode)
    {
        case FBE_BASE_ENV_CMI_MSG_FUP_PEER_PERMISSION_GRANT:
        case FBE_BASE_ENV_CMI_MSG_FUP_PEER_PERMISSION_DENY:
            // Peer SP is not present when sending the peermission grant or deny.
            status = FBE_STATUS_OK;
            break;

        case FBE_BASE_ENV_CMI_MSG_FUP_PEER_PERMISSION_REQUEST:
            // peer SP is not present when sending the message to request permission.
            status = fbe_base_env_fup_processDeniedPeerPerm((fbe_base_environment_t *)pEnclMgmt,
                                                         pFupCmiPkt->pRequestorWorkItem);
     
            break;

        default:
            status = FBE_STATUS_MORE_PROCESSING_REQUIRED;
            break;
    }

    return(status);
}

/*!**************************************************************
 * @fn fbe_encl_mgmt_processContactLost(fbe_encl_mgmt_t * pEnclMgmt)
 ****************************************************************
 * @brief
 *  This function gets called when recieving the CMI message with 
 *  FBE_CMI_EVENT_SP_CONTACT_LOST.
 *
 * @param pEnclMgmt - 
 *
 * @return fbe_status_t - always return
 *   FBE_STATUS_MORE_PROCESSING_REQUIRED - need to further handled by the base environment.
 * 
 * @author
 *  27-Apr-2011: PHE - Created.
 ****************************************************************/
static fbe_status_t fbe_encl_mgmt_processContactLost(fbe_encl_mgmt_t * pEnclMgmt)
{
    /* No need to handle the fup work items here. If the contact is lost, we will also receive
     * Peer Not Present message and we will handle the fup work item there. 
     */
    /* Return FBE_STATUS_MORE_PROCESSING_REQUIRED so that it will do further processing in base_environment.*/
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}

/*!**************************************************************
 * @fn fbe_ps_mgmt_processPeerBusy(fbe_ps_mgmt_t * pPsMgmt, 
 *                          fbe_ps_mgmt_cmi_packet_t * pPsCmiPkt)
 ****************************************************************
 * @brief
 *  This function gets called when recieving the CMI message with 
 *  FBE_CMI_EVENT_PEER_NOT_PRESENT.
 *
 * @param pEnclMgmt - 
 * @param pEnclCmiPkt - pointer to user message of CMI message info.
 *
 * @return fbe_status_t -
 *   FBE_STATUS_MORE_PROCESSING_REQUIRED - need to further handled by the base environment.
 *   FBE_STATUS_OK - successful.
 *   others - failed.
 * 
 * @author
 *  27-Apr-2011: PHE - Created.
 ****************************************************************/
static fbe_status_t 
fbe_encl_mgmt_processPeerBusy(fbe_encl_mgmt_t * pEnclMgmt, 
                              fbe_encl_mgmt_cmi_packet_t * pEnclCmiPkt)
{
    fbe_base_environment_cmi_message_t * pBaseCmiMsg = (fbe_base_environment_cmi_message_t *)pEnclCmiPkt;
    fbe_base_env_fup_cmi_packet_t      * pFupCmiPkt = (fbe_base_env_fup_cmi_packet_t *)pEnclCmiPkt;
    fbe_status_t                         status = FBE_STATUS_OK;

    switch (pBaseCmiMsg->opcode)
    {
        case FBE_BASE_ENV_CMI_MSG_FUP_PEER_PERMISSION_GRANT:
        case FBE_BASE_ENV_CMI_MSG_FUP_PEER_PERMISSION_DENY:
            fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt, 
                          FBE_TRACE_LEVEL_ERROR,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s, should never happen, opcode 0x%llx\n",
                          __FUNCTION__,
                          (unsigned long long)pBaseCmiMsg->opcode);

            status = FBE_STATUS_GENERIC_FAILURE;
            break;

        case FBE_BASE_ENV_CMI_MSG_FUP_PEER_PERMISSION_REQUEST:
            // peer SP is not present when sending the message to request permission.
            status = fbe_base_env_fup_peerPermRetry((fbe_base_environment_t *)pEnclMgmt, 
                                                    pFupCmiPkt->pRequestorWorkItem);
            break;

        default:
            status = FBE_STATUS_MORE_PROCESSING_REQUIRED;
            break;
    }

    return(status);
}

/*!**************************************************************
 * @fn fbe_encl_mgmt_processFatalError(fbe_encl_mgmt_t * pEnclMgmt, 
 *                                     fbe_encl_mgmt_cmi_packet_t * pEnclCmiPkt)
 ****************************************************************
 * @brief
 *  This function gets called when recieving the CMI message with 
 *  FBE_CMI_EVENT_FATAL_ERROR.
 *
 * @param pEnclMgmt - 
 * @param pEnclCmiPkt - pointer to user message of CMI message info.
 *
 * @return fbe_status_t -
 *   FBE_STATUS_MORE_PROCESSING_REQUIRED - need to further handled by the base environment.
 *   FBE_STATUS_OK - successful.
 *   others - failed.
 * 
 * @author
 *  27-Apr-2011: PHE - Created.
 ****************************************************************/
static fbe_status_t 
fbe_encl_mgmt_processFatalError(fbe_encl_mgmt_t * pEnclMgmt, 
                                fbe_encl_mgmt_cmi_packet_t * pEnclCmiPkt)
{
    fbe_base_environment_cmi_message_t * pBaseCmiMsg = (fbe_base_environment_cmi_message_t *)pEnclCmiPkt;
    fbe_base_env_fup_cmi_packet_t      * pFupCmiPkt = (fbe_base_env_fup_cmi_packet_t *)pEnclCmiPkt;
    fbe_status_t                         status = FBE_STATUS_OK;

    switch (pBaseCmiMsg->opcode)
    {
        case FBE_BASE_ENV_CMI_MSG_FUP_PEER_PERMISSION_GRANT:
        case FBE_BASE_ENV_CMI_MSG_FUP_PEER_PERMISSION_DENY:
            /* No need to retry.
             * The peer did not receive the response in certain period time,
             * it will do whatever it needs to do. 
             */ 
            status = FBE_STATUS_GENERIC_FAILURE;
            break;

        case FBE_BASE_ENV_CMI_MSG_FUP_PEER_PERMISSION_REQUEST:
            // peer SP is not present when sending the message to request permission.
            status = fbe_base_env_fup_processDeniedPeerPerm((fbe_base_environment_t *)pEnclMgmt,
                                                         pFupCmiPkt->pRequestorWorkItem);
            break;

        default:
            status = FBE_STATUS_MORE_PROCESSING_REQUIRED;
            break;
    }

    return(status);
}


/*!**************************************************************
 * fbe_encl_mgmt_processEnclEirData()
 ****************************************************************
 * @brief
 *  This function takes an enclosure location and EIR values and:
 *      -save values in sample array
 *      -calculates Average & Max values
 *
 * @param pEnclMgmt - This is the object handle, or in our case the encl_mgmt.
 * @param pLocation - The location of the LCC.
 * @param inputPower - Input Power value to process
 * @param  airInletTemp - Air Inlet Temperature value to process
 *
 * @return fbe_status_t - 
 *
 * @author
 *  10-Feb-2011:  Joe Perry - Created.
 *
 ****************************************************************/
void fbe_encl_mgmt_processEnclEirData(fbe_encl_mgmt_t * pEnclMgmt,
                                      fbe_device_physical_location_t *enclLocation,
                                      fbe_eir_input_power_sample_t *inputPower,
                                      fbe_eir_temp_sample_t *airInletTemp)
{
    fbe_encl_info_t                     *pEnclInfo = NULL;
    fbe_status_t                        status;
    fbe_u32_t                           sampleIndex;
    fbe_eir_input_power_sample_t        inputPowerSum;
    fbe_eir_temp_sample_t               airInletTempSum;
    fbe_u32_t                           inputPowerCnt, airInletTempCnt;
    fbe_eir_input_power_sample_t        *inputPowerSamplePtr = NULL;
    fbe_eir_temp_sample_t               *airInletTempSamplePtr = NULL;

    /*
     * Find enclosure (should already exist)
     */
    status = fbe_encl_mgmt_get_encl_info_by_location(pEnclMgmt, enclLocation, &pEnclInfo);
    if (status != FBE_STATUS_OK) 
    {
        fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s, encl %d_%d, get_encl_info_by_location failed, status: 0x%X.\n",
                              __FUNCTION__, enclLocation->bus, enclLocation->enclosure, status);
            return;
    }

    /*
     * set current values & sample
     */
    pEnclInfo->eir_data.enclCurrentInputPower = *inputPower;
    pEnclInfo->eir_data.currentAirInletTemp = *airInletTemp;

    pEnclInfo->eirSampleData->inputPowerSamples[pEnclMgmt->eirSampleIndex] = *inputPower;
    pEnclInfo->eirSampleData->airInletTempSamples[pEnclMgmt->eirSampleIndex] = *airInletTemp;

    /*
     * Perform calculations
     */
    // clear stats & initialize pointers
    fbe_encl_mgmt_clearInputPowerSample(&inputPowerSum);
    fbe_encl_mgmt_clearAirInletTempSample(&airInletTempSum);
    fbe_encl_mgmt_clearInputPowerSample(&(pEnclInfo->eir_data.enclAverageInputPower));
    fbe_encl_mgmt_clearInputPowerSample(&(pEnclInfo->eir_data.enclMaxInputPower));
    fbe_encl_mgmt_clearAirInletTempSample(&(pEnclInfo->eir_data.averageAirInletTemp));
    fbe_encl_mgmt_clearAirInletTempSample(&(pEnclInfo->eir_data.maxAirInletTemp));
    inputPowerCnt = 0;
    airInletTempCnt = 0;
    inputPowerSamplePtr = &(pEnclInfo->eirSampleData->inputPowerSamples[0]);
    airInletTempSamplePtr = &(pEnclInfo->eirSampleData->airInletTempSamples[0]);

    // loop through all samples & determine Average & Max values
    for (sampleIndex = 0; sampleIndex < pEnclMgmt->eirSampleCount; sampleIndex++)
    {
        // calculate InputPower stats
        inputPowerSum.status |= inputPowerSamplePtr->status;
        if (inputPowerSamplePtr->status == FBE_ENERGY_STAT_VALID)
        {
            inputPowerCnt++;
            inputPowerSum.inputPower += inputPowerSamplePtr->inputPower;
            if (inputPowerSamplePtr->inputPower > pEnclInfo->eir_data.enclMaxInputPower.inputPower)
            {
                pEnclInfo->eir_data.enclMaxInputPower.inputPower = inputPowerSamplePtr->inputPower;
                pEnclInfo->eir_data.enclMaxInputPower.status = FBE_ENERGY_STAT_VALID;
            }
        }
        inputPowerSamplePtr++;

        // calculate AirInletTemp stats
        airInletTempSum.status |= airInletTempSamplePtr->status;
        if (airInletTempSamplePtr->status == FBE_ENERGY_STAT_VALID)
        {
            airInletTempCnt++;
            airInletTempSum.airInletTemp += airInletTempSamplePtr->airInletTemp;
            if (airInletTempSamplePtr->airInletTemp > pEnclInfo->eir_data.maxAirInletTemp.airInletTemp)
            {
                pEnclInfo->eir_data.maxAirInletTemp.airInletTemp = airInletTempSamplePtr->airInletTemp;
                pEnclInfo->eir_data.maxAirInletTemp.status = FBE_ENERGY_STAT_VALID;
            }
        }
        airInletTempSamplePtr++;

    }

    // calculate averages
    pEnclInfo->eir_data.enclAverageInputPower.status = inputPowerSum.status;
    if (inputPowerCnt > 0)
    {
        pEnclInfo->eir_data.enclAverageInputPower.inputPower = inputPowerSum.inputPower / inputPowerCnt;
        //if sample too small, set the bit
        if (inputPowerCnt < FBE_SAMPLE_DATA_SIZE)
        {
            pEnclInfo->eir_data.enclAverageInputPower.status |= FBE_ENERGY_STAT_SAMPLE_TOO_SMALL;
        }
        fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt, 
                              FBE_TRACE_LEVEL_DEBUG_LOW,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "encl %d_%d, InputPower sum %d, cnt %d, status 0x%x\n",
                              enclLocation->bus, enclLocation->enclosure,
                              inputPowerSum.inputPower, inputPowerCnt, inputPowerSum.status);
    }
    pEnclInfo->eir_data.averageAirInletTemp.status = airInletTempSum.status;
    if (airInletTempCnt > 0)
    {
        pEnclInfo->eir_data.averageAirInletTemp.airInletTemp = airInletTempSum.airInletTemp / airInletTempCnt;
        //if sample too small, set the bit
        if (airInletTempCnt < FBE_SAMPLE_DATA_SIZE)
        {
            pEnclInfo->eir_data.averageAirInletTemp.status |= FBE_ENERGY_STAT_SAMPLE_TOO_SMALL;
        }
        fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt, 
                              FBE_TRACE_LEVEL_DEBUG_LOW,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "encl %d_%d, AirInletTemp, sum %d, cnt %d, status 0x%x\n",
                              enclLocation->bus, enclLocation->enclosure,
                              airInletTempSum.airInletTemp, airInletTempCnt, airInletTempSum.status);
    }

    fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt, 
                          FBE_TRACE_LEVEL_DEBUG_LOW,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s, encl %d_%d, InputPower\n",
                          __FUNCTION__, enclLocation->bus, enclLocation->enclosure);
    fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt, 
                          FBE_TRACE_LEVEL_DEBUG_LOW,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "  Current %d, status 0x%x\n",
                          pEnclInfo->eir_data.enclCurrentInputPower.inputPower, 
                          pEnclInfo->eir_data.enclCurrentInputPower.status);
    fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt, 
                          FBE_TRACE_LEVEL_DEBUG_LOW,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "  Average %d, status 0x%x\n",
                          pEnclInfo->eir_data.enclAverageInputPower.inputPower, 
                          pEnclInfo->eir_data.enclAverageInputPower.status);
    fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt, 
                          FBE_TRACE_LEVEL_DEBUG_LOW,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "  Max %d, status 0x%x\n",
                          pEnclInfo->eir_data.enclMaxInputPower.inputPower, 
                          pEnclInfo->eir_data.enclMaxInputPower.status);
    fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt, 
                          FBE_TRACE_LEVEL_DEBUG_LOW,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s, encl %d_%d, AirInletTemp\n",
                          __FUNCTION__, enclLocation->bus, enclLocation->enclosure);
    fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt, 
                          FBE_TRACE_LEVEL_DEBUG_LOW,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "  Current %d, status 0x%x\n",
                          pEnclInfo->eir_data.currentAirInletTemp.airInletTemp, 
                          pEnclInfo->eir_data.currentAirInletTemp.status);
    fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt, 
                          FBE_TRACE_LEVEL_DEBUG_LOW,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "  Average %d, status 0x%x\n",
                          pEnclInfo->eir_data.averageAirInletTemp.airInletTemp, 
                          pEnclInfo->eir_data.averageAirInletTemp.status);
    fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt, 
                          FBE_TRACE_LEVEL_DEBUG_LOW,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "  Max %d, status 0x%x\n",
                          pEnclInfo->eir_data.maxAirInletTemp.airInletTemp, 
                          pEnclInfo->eir_data.maxAirInletTemp.status);


}   // end of fbe_encl_mgmt_processEnclEirData

void fbe_encl_mgmt_clearInputPowerSample(fbe_eir_input_power_sample_t *samplePtr)
{
    samplePtr->inputPower = 0;
    samplePtr->status = FBE_ENERGY_STAT_UNINITIALIZED;
}   // end of fbe_encl_mgmt_clearInputPowerSample

void fbe_encl_mgmt_clearAirInletTempSample(fbe_eir_temp_sample_t *samplePtr)
{
    samplePtr->airInletTemp = 0;
    samplePtr->status = FBE_ENERGY_STAT_UNINITIALIZED;
}   // end of fbe_encl_mgmt_clearAirInletTempSample

/*!**************************************************************
 * @fn fbe_encl_mgmt_log_lcc_status_change(fbe_device_physical_location_t * pLocation,
 *                                         fbe_lcc_info_t *pNewLccInfo,
 *                                         fbe_lcc_info_t *pOldLccInfo)
 ****************************************************************
 * @brief
 *  Check for lcc status change and log events for them.
 *
 * @param pLocation - 
 * @param pNewLccInfo - 
 * @param pOldLccInfo -
 *
 * @return fbe_status_t - the status of the event log write, else OK
 * @author
 *   7-Nov-2011: GB - Created.
 ****************************************************************/
static fbe_status_t 
fbe_encl_mgmt_log_lcc_status_change(fbe_encl_mgmt_t * pEnclMgmt,
                                    fbe_device_physical_location_t * pLocation,
                                    fbe_lcc_info_t *pNewLccInfo,
                                    fbe_lcc_info_t *pOldLccInfo)
{
    fbe_status_t                status = FBE_STATUS_OK;
    fbe_u8_t                    deviceStr[FBE_DEVICE_STRING_LENGTH];
    fbe_base_env_resume_prom_info_t    * pResumePromInfo = NULL;
    fbe_u8_t encl_sn[RESUME_PROM_PRODUCT_SN_SIZE + 1] = {'\0'};
    fbe_u8_t encl_pn[RESUME_PROM_PRODUCT_PN_SIZE + 1] = {'\0'};

    status = fbe_base_env_create_device_string(FBE_DEVICE_TYPE_LCC,
                                               pLocation,
                                               &deviceStr[0],
                                               FBE_DEVICE_STRING_LENGTH);

    status = fbe_encl_mgmt_get_resume_prom_info_ptr(pEnclMgmt,
                                                    FBE_DEVICE_TYPE_LCC,
                                                    pLocation,
                                                    &pResumePromInfo);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s, failed to get %s resume prom.\n",
                              __FUNCTION__,
                              deviceStr);
        return status;
    }

    if((pNewLccInfo->inserted) && (!pOldLccInfo->inserted))
    {
        // lcc was inserted
        fbe_event_log_write(ESP_INFO_LCC_INSERTED,
                            NULL, 0,
                            "%s", 
                            &deviceStr[0]);
    }
    else if((!pNewLccInfo->inserted) && (pOldLccInfo->inserted))
    {
        // The LCC was removed
        status = fbe_event_log_write(ESP_ERROR_LCC_REMOVED,
                                     NULL, 
                                     0,
                                     "%s", 
                                     &deviceStr[0]);
    }
    if((pNewLccInfo->faulted) && (!pOldLccInfo->faulted))
    {
        // there two types of LCC faults, so look at additional Status to determine type
        if (pNewLccInfo->additionalStatus == SES_STAT_CODE_CRITICAL)
        {
            if(pResumePromInfo->op_status == FBE_RESUME_PROM_STATUS_READ_SUCCESS)
            {
                fbe_copy_memory(encl_sn, pResumePromInfo->resume_prom_data.product_serial_number, RESUME_PROM_PRODUCT_SN_SIZE);
                fbe_copy_memory(encl_pn, pResumePromInfo->resume_prom_data.product_part_number, RESUME_PROM_PRODUCT_PN_SIZE);
            }
            else
            {
                fbe_copy_memory(encl_sn, "Not Available", (fbe_u32_t)(FBE_MIN(strlen("Not Available"), RESUME_PROM_PRODUCT_SN_SIZE)));
                fbe_copy_memory(encl_pn, "Not Available", (fbe_u32_t)(FBE_MIN(strlen("Not Available"), RESUME_PROM_PRODUCT_PN_SIZE)));
            }

            // Critical, lcc faulted
            status = fbe_event_log_write(ESP_ERROR_LCC_FAULTED,
                                         NULL, 
                                         0,
                                         "%s %s %s",
                                         &deviceStr[0],
                                         encl_sn,
                                         encl_pn);
        }
        else
        {
            // Else, lcc fault may be caused by other components
// ESP_STAND_ALONE_ALERT - remove when CP support available
            status = fbe_event_log_write(ESP_ERROR_LCC_COMPONENT_FAULTED,
                                         NULL, 
                                         0,
                                         "%s", 
                                         &deviceStr[0]);
        }
    }
    if((!pNewLccInfo->faulted) && (pOldLccInfo->faulted))
    {
        // lcc fault cleared
        status = fbe_event_log_write(ESP_INFO_LCC_FAULT_CLEARED,
                                     NULL, 
                                     0,
                                     "%s", 
                                     &deviceStr[0]);
    }

    return(status);
}

/*!**************************************************************
 * @fn fbe_encl_mgmt_update_encl_fault_led(fbe_encl_mgmt_t *pEnclMgmt,
 *                                  fbe_device_physical_location_t *pLocation,
 *                                  fbe_encl_fault_led_reason_t enclFaultLedReason)
 ****************************************************************
 * @brief
 *  This function checks the status to determine
 *  if the Enclosure Fault LED needs to be updated.
 *
 * @param pEnclMgmt - The pointer to the enclosure mgmt object.
 * @param pLocation - The location of the enclosure.
 * @param enclFaultLedReason - The reason to turn on/off the enclosure fault led.
 * 
 * @return fbe_status_t - 
 *
 * @author
 *  14-Dec-2011:  PHE - Created. 
 *
 ****************************************************************/
fbe_status_t fbe_encl_mgmt_update_encl_fault_led(fbe_encl_mgmt_t *pEnclMgmt,
                                    fbe_device_physical_location_t *pLocation,
                                    fbe_encl_fault_led_reason_t enclFaultLedReason)
{
    fbe_status_t                                status = FBE_STATUS_OK;
    fbe_encl_info_t                             *pEnclInfo = NULL;
    fbe_lcc_info_t                              *pLccInfo = NULL;
    fbe_base_env_resume_prom_info_t             *pResumeInfo = NULL;
    fbe_connector_info_t                        *pConnectorInfo = NULL;
    fbe_u32_t                                   index = 0;
    fbe_bool_t                                  enclFaultLedOn = FBE_FALSE;
    fbe_u32_t                                   bit = 0;
    fbe_encl_fault_led_reason_t                 faultLedReason;
    fbe_object_id_t                             encl_object_id;

    status = fbe_encl_mgmt_get_encl_info_by_location(pEnclMgmt, pLocation, &pEnclInfo);
    if (status != FBE_STATUS_OK) 
    {
        fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s, encl %d_%d, get_encl_info_by_location failed, status: 0x%X.\n",
                              __FUNCTION__, pLocation->bus, pLocation->enclosure, status);
        return status;
    }

    if (enclFaultLedReason == FBE_ENCL_FAULT_LED_NO_FLT)
    {
        return FBE_STATUS_OK;
    }
    else
    {
        for(bit = 0; bit < 64; bit++)
        {
            enclFaultLedOn = FBE_FALSE;

            if((1ULL << bit) >= FBE_ENCL_FAULT_LED_BITMASK_MAX)
            {
                break;
            }

            faultLedReason = enclFaultLedReason & (1ULL << bit);
            switch (faultLedReason)
            {
            case FBE_ENCL_FAULT_LED_NO_FLT:
                break;
            case FBE_ENCL_FAULT_LED_LCC_FLT:
                for(index = 0; index < pEnclInfo->lccCount; index ++)
                {
                    pLccInfo = &pEnclInfo->lccInfo[index];

                    enclFaultLedOn = fbe_encl_mgmt_get_lcc_logical_fault(pLccInfo);

                    if(enclFaultLedOn)
                    {
                        break;
                    }
                }
                break;

            case FBE_ENCL_FAULT_LED_LCC_RESUME_PROM_FLT:
                for(index = 0; index < pEnclInfo->lccCount; index ++)
                {
                    pResumeInfo = &pEnclInfo->lccResumeInfo[index];

                    enclFaultLedOn = fbe_base_env_get_resume_prom_fault(pResumeInfo->op_status);

                    if(enclFaultLedOn)
                    {
                        break;
                    }
                }
                break;

            case FBE_ENCL_FAULT_LED_MIDPLANE_RESUME_PROM_FLT:
                pResumeInfo = &pEnclInfo->enclResumeInfo;

                enclFaultLedOn = fbe_base_env_get_resume_prom_fault(pResumeInfo->op_status);

                break;

            case FBE_ENCL_FAULT_LED_DRIVE_MIDPLANE_RESUME_PROM_FLT:
                pResumeInfo = &pEnclInfo->driveMidplaneResumeInfo;

                enclFaultLedOn = fbe_base_env_get_resume_prom_fault(pResumeInfo->op_status);

                break;

            case FBE_ENCL_FAULT_LED_CONNECTOR_FLT:
                for(index = 0; index < pEnclInfo->connectorCount; index ++)
                {
                    pConnectorInfo = &pEnclInfo->connectorInfo[index];

                    if(!pConnectorInfo->isLocalFru) 
                    {
                        // Only care about the local connector events.
                        continue;
                    }

                    enclFaultLedOn = fbe_encl_mgmt_get_connector_logical_fault(pConnectorInfo);

                    if(enclFaultLedOn)
                    {
                        break;
                    }
                }
                break;

            case FBE_ENCL_FAULT_LED_ENCL_LIFECYCLE_FAIL:
                if((pEnclInfo->enclState == FBE_ESP_ENCL_STATE_FAILED) &&
                   (pEnclInfo->enclFaultSymptom == FBE_ESP_ENCL_FAULT_SYM_LIFECYCLE_STATE_FAIL))
                {
                    enclFaultLedOn = FBE_TRUE;
                }
                break;

            case FBE_ENCL_FAULT_LED_LCC_CABLING_FLT:
                if((pEnclInfo->enclState == FBE_ESP_ENCL_STATE_FAILED) &&
                   ((pEnclInfo->enclFaultSymptom == FBE_ESP_ENCL_FAULT_SYM_CROSS_CABLED)
                     || (pEnclInfo->enclFaultSymptom == FBE_ESP_ENCL_FAULT_SYM_BE_LOOP_MISCABLED)
                     || (pEnclInfo->enclFaultSymptom == FBE_ESP_ENCL_FAULT_SYM_LCC_INVALID_UID)))
                {
                    enclFaultLedOn = FBE_TRUE;
                }
                break;

            case FBE_ENCL_FAULT_LED_EXCEEDED_MAX:
                if((pEnclInfo->enclState == FBE_ESP_ENCL_STATE_FAILED) &&
                   (pEnclInfo->enclFaultSymptom == FBE_ESP_ENCL_FAULT_SYM_EXCEEDED_MAX))
                {
                    enclFaultLedOn = FBE_TRUE;
                }
                break;

            case FBE_ENCL_FAULT_LED_SUBENCL_LIFECYCLE_FAIL:
                enclFaultLedOn = FBE_TRUE;
                break;

            case FBE_ENCL_FAULT_LED_OVERTEMP_FLT:
                if (pEnclInfo->overTempFailure || pEnclInfo->overTempWarning)
                {
                    enclFaultLedOn = FBE_TRUE;
                }
                break;

            case FBE_ENCL_FAULT_LED_SYSTEM_SERIAL_NUMBER_FLT:
                enclFaultLedOn = FBE_TRUE;
                break;

            case FBE_ENCL_FAULT_LED_SSC_FLT:
                if((pEnclInfo->sscCount > 0) &&
                    (!pEnclInfo->sscInfo.inserted || pEnclInfo->sscInfo.faulted))
                {
                    enclFaultLedOn = FBE_TRUE;
                }
                break;

            case FBE_ENCL_FAULT_LED_SSC_RESUME_PROM_FLT:
                pResumeInfo = &pEnclInfo->sscResumeInfo;
                enclFaultLedOn = fbe_base_env_get_resume_prom_fault(pResumeInfo->op_status);
                break;

            default:
                fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt,
                                      FBE_TRACE_LEVEL_WARNING,
                                      FBE_TRACE_MESSAGE_ID_INFO,
                                      "ENCL(%d_%d), unsupported enclFaultLedReason 0x%llX\n",
                                      pLocation->bus,
                                      pLocation->enclosure,
                                      enclFaultLedReason);
                return FBE_STATUS_GENERIC_FAILURE;
                break;
            }   // end of switch

            if(faultLedReason != FBE_ENCL_FAULT_LED_NO_FLT)
            {
                fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt,
                                          FBE_TRACE_LEVEL_INFO,
                                          FBE_TRACE_MESSAGE_ID_INFO,
                                          "ENCL(%d_%d), %s enclFaultLedReason %s.\n",
                                          pLocation->bus,
                                          pLocation->enclosure,
                                          enclFaultLedOn? "SET" : "CLEAR",
                                          fbe_base_env_decode_encl_fault_led_reason(faultLedReason));


                if ((pEnclMgmt->base_environment.processorEnclType == DPE_ENCLOSURE_TYPE) &&
                         (pLocation->bus == 0) && (pLocation->enclosure == 0))
                {
                    status = fbe_api_get_board_object_id(&encl_object_id);
                    if (status != FBE_STATUS_OK) {
                        fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt, 
                                              FBE_TRACE_LEVEL_ERROR,
                                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                              "ENCL(%d_%d), failed to get board object, status: 0x%X.\n",
                                              pLocation->bus,
                                              pLocation->enclosure,
                                              status);
                        return status;
                    }
                }
                else
                {
                    encl_object_id = pEnclInfo->object_id;
                }


                status = fbe_base_environment_updateEnclFaultLed((fbe_base_object_t *)pEnclMgmt,
                                                                 encl_object_id,
                                                                 enclFaultLedOn,
                                                                 faultLedReason);

                if (status != FBE_STATUS_OK)
                {
                    fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt,
                                          FBE_TRACE_LEVEL_ERROR,
                                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                          "ENCL(%d_%d), error in %s enclFaultLedReason %s.\n",
                                          pLocation->bus,
                                          pLocation->enclosure,
                                          enclFaultLedOn? "SET" : "CLEAR",
                                          fbe_base_env_decode_encl_fault_led_reason(faultLedReason));

                }
            }
        }   // for bit
    }
       
    return status;
}

/*!**************************************************************
 * @fn fbe_encl_mgmt_get_lcc_logical_fault(fbe_lcc_info_t *pLccInfo)
 ****************************************************************
 * @brief
 *  This function considers all the fields for the LCC
 *  and decides whether the LCC is logically faulted or not.
 *
 * @param pLccInfo -The pointer to fbe_lcc_info_t.
 *
 * @return fbe_bool_t - The LCC is faulted or not logically.
 *
 * @author
 *  14-Dec-2011: PHE - Created.
 *
 ****************************************************************/
static fbe_bool_t 
fbe_encl_mgmt_get_lcc_logical_fault(fbe_lcc_info_t *pLccInfo)
{
    fbe_bool_t logicalFault = FBE_FALSE;

    if(!pLccInfo->inserted || pLccInfo->faulted)
    {
        logicalFault = FBE_TRUE;
    }
    else
    {
        logicalFault = FBE_FALSE;
    }

    return logicalFault;
}


/*!**************************************************************
 * @fn fbe_encl_mgmt_get_connector_logical_fault(fbe_connector_info_t *pConnectorInfo)
 ****************************************************************
 * @brief
 *  This function considers all the fields for the Connector
 *  and decides whether the Connector is logically faulted or not.
 *
 * @param pConnectorInfo -The pointer to fbe_lcc_info_t.
 *
 * @return fbe_bool_t - The Connector is faulted or not logically.
 *
 * @author
 *  14-Dec-2011: PHE - Created.
 *
 ****************************************************************/
static fbe_bool_t 
fbe_encl_mgmt_get_connector_logical_fault(fbe_connector_info_t *pConnectorInfo)
{
    fbe_bool_t logicalFault = FBE_FALSE;

    if(pConnectorInfo->cableStatus == FBE_CABLE_STATUS_DEGRADED)
    {
        logicalFault = FBE_TRUE;
    }
    else
    {
        logicalFault = FBE_FALSE;
    }

    return logicalFault;
}


/*!**************************************************************
 * fbe_encl_mgmt_process_encl_fault_led_status()
 ****************************************************************
 * @brief
 *  This function checks if the current enclosure display is set
 *  to the expected values.
 *
 * @param objectId - enclosure object ID in the physical package
 * @param pLocation - pointer to enclosure location information structure
 *
 * @return fbe_bool_t - The display was set successfully.
 *
 * @author
 *  19-Jan-2012: bphilbin - Created.
 *
 ****************************************************************/
static fbe_status_t 
fbe_encl_mgmt_process_display_status(fbe_encl_mgmt_t * pEnclMgmt,
                                       fbe_object_id_t objectId,
                                       fbe_device_physical_location_t * pLocation)
{
    fbe_encl_info_t                         * pEnclInfo = NULL;
    fbe_encl_display_info_t                   enclDisplayInfo = {0};
    fbe_status_t                              status = FBE_STATUS_OK;
    fbe_char_t                                bus_string[3];

    if(objectId == FBE_OBJECT_ID_INVALID)
    {
        fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt, 
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s, encl %d_%d, invalid object id.\n",
                                  __FUNCTION__, pLocation->bus, pLocation->enclosure);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_encl_mgmt_get_encl_info_by_location(pEnclMgmt, pLocation, &pEnclInfo);

    if (status != FBE_STATUS_OK) 
    {
        /* The enclosure was not initialized yet.  
         * When the enclosure gets initialized, 
         * it will call fbe_encl_mgmt_process_display_status anyway.
         * Just return FBE_STATUS_OK here.
         */
        fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt, 
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s, encl %d_%d, enclInfo not populated, ignore the change.\n",
                                  __FUNCTION__, pLocation->bus, pLocation->enclosure);

        return FBE_STATUS_OK;
    }

    status = fbe_api_enclosure_get_display_info(objectId, &enclDisplayInfo);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt,  
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                              "%s, Encl(%d_%d), Failed to get enclDisplayInfo.\n", 
                              __FUNCTION__,
                              pLocation->bus,
                              pLocation->enclosure); 

        return status;
    }

    /* Save the current display values in the enclmgmt context */
    pEnclInfo->display.busNumber[0] = enclDisplayInfo.busNumber[0];
    pEnclInfo->display.busNumber[1] = enclDisplayInfo.busNumber[1];
    pEnclInfo->display.enclNumber = enclDisplayInfo.enclNumber;

    if(pEnclInfo->location.bus >= 10)
    {
        // two digits, put them both in the string
        fbe_sprintf(bus_string, 3, "%d", pEnclInfo->location.bus);
    }
    else
    {
        // one digit, add a leading 0
        fbe_sprintf(bus_string, 3, "0%d", pEnclInfo->location.bus);
    }

    /*
     * If the display characters don't match what they should be, go update them.
     */
    if( (enclDisplayInfo.busNumber[0] != bus_string[0]) || 
        (enclDisplayInfo.busNumber[1] != bus_string[1]) ||
        (enclDisplayInfo.enclNumber != (0x30 + pEnclInfo->location.enclosure)))
    {
        fbe_encl_mgmt_set_enclosure_display(pEnclMgmt,objectId,pLocation);
    }

    return FBE_STATUS_OK;
}

/*!**************************************************************
 * @fn fbe_encl_mgmt_set_enclosure_display
 ****************************************************************
 * @brief
 *  This function sets the enclosure display to the current bus and
 *  enclosure position.  This function is for DAE enclosures.
 *
 * @param objectId - enclosure object ID in the physical package
 * @param pLocation - pointer to enclosure location information structure
 *
 * @return fbe_bool_t - The display was set successfully.
 *
 * @author
 *  19-Jan-2012: bphilbin - Created.
 *
 ****************************************************************/
static fbe_status_t
fbe_encl_mgmt_set_enclosure_display(fbe_encl_mgmt_t * pEnclMgmt,
                                       fbe_object_id_t objectId,
                                       fbe_device_physical_location_t * pLocation)
{
    fbe_encl_info_t                         * pEnclInfo = NULL;
    fbe_status_t                              status = FBE_STATUS_OK;
    fbe_base_enclosure_led_status_control_t   ledCommandInfo;
    fbe_char_t                                bus_string[3];

    if(objectId == FBE_OBJECT_ID_INVALID)
    {
        fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt, 
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s, encl %d_%d, invalid object id.\n",
                                  __FUNCTION__, pLocation->bus, pLocation->enclosure);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_encl_mgmt_get_encl_info_by_location(pEnclMgmt, pLocation, &pEnclInfo);

    if (status != FBE_STATUS_OK) 
    {
        /* The enclosure was not initialized yet.  
         * When the enclosure gets initialized, 
         * it will call fbe_encl_mgmt_process_encl_fault_led_status anyway.
         * Just return FBE_STATUS_OK here.
         */
        fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt, 
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s, encl %d_%d, enclInfo not populated, ignore the change.\n",
                                  __FUNCTION__, pLocation->bus, pLocation->enclosure);

        return FBE_STATUS_OK;
    }

    /* set up the command */
    fbe_zero_memory(&ledCommandInfo, sizeof(fbe_base_enclosure_led_status_control_t));
    ledCommandInfo.ledAction = FBE_ENCLOSURE_LED_CONTROL;
    if(pEnclInfo->location.bus >= 10)
    {
        // two digits, put them both in the string
        fbe_sprintf(bus_string, 3, "%d", pEnclInfo->location.bus);
    }
    else
    {
        // one digit, add a leading 0
        fbe_sprintf(bus_string, 3, "0%d", pEnclInfo->location.bus);
    }
    ledCommandInfo.ledInfo.busNumber[1] = bus_string[1];
    ledCommandInfo.ledInfo.busNumber[0] = bus_string[0];
    
    ledCommandInfo.ledInfo.busDisplay = FBE_ENCLOSURE_LED_BEHAVIOR_DISPLAY;
    ledCommandInfo.ledInfo.enclNumber = 0x30 + pEnclInfo->location.enclosure;
    ledCommandInfo.ledInfo.enclDisplay = FBE_ENCLOSURE_LED_BEHAVIOR_DISPLAY;

    fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt, 
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s, encl %d_%d, setting display.\n",
                                  __FUNCTION__, pLocation->bus, pLocation->enclosure);

    /* Issue the LED command to the enclosure */
    status = fbe_api_enclosure_send_set_led(objectId, &ledCommandInfo);
    if (status != FBE_STATUS_OK) 
    {
        fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt, 
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s, encl %d_%d, setting the LED failed with status 0x%x.\n",
                                  __FUNCTION__, pLocation->bus, pLocation->enclosure, status);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    return FBE_STATUS_OK;

}

/*!**************************************************************
 * @fn fbe_encl_mgmt_process_slot_led_status
 ****************************************************************
 * @brief
 *  This function checks the slot LED status for an enclosure.
 *
 * @param objectId - enclosure object ID in the physical package
 * @param pLocation - pointer to enclosure location information structure
 *
 * @return fbe_status_t 
 *
 * @author
 *  19-Feb-2012: bphilbin - Created.
 *
 ****************************************************************/
static fbe_status_t
fbe_encl_mgmt_process_slot_led_status(fbe_encl_mgmt_t * pEnclMgmt,
                                       fbe_object_id_t objectId,
                                       fbe_device_physical_location_t * pLocation)
{
    fbe_encl_info_t                         * pEnclInfo = NULL;
    fbe_status_t                              status = FBE_STATUS_OK;
    fbe_led_status_t                        * pEnclDriveFaultLeds = NULL;
    fbe_u32_t                                 slot;

    fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt, 
                          FBE_TRACE_LEVEL_DEBUG_LOW,  
                          FBE_TRACE_MESSAGE_ID_INFO, 
                          "%s entry\n", 
                          __FUNCTION__);

    if(objectId == FBE_OBJECT_ID_INVALID)
    {
        fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt, 
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s, encl %d_%d, invalid object id.\n",
                                  __FUNCTION__, pLocation->bus, pLocation->enclosure);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_encl_mgmt_get_encl_info_by_location(pEnclMgmt, pLocation, &pEnclInfo);

    if (status != FBE_STATUS_OK) 
    {
        /* The enclosure was not initialized yet.  
         * When the enclosure gets initialized, 
         * it will call fbe_encl_mgmt_process_encl_fault_led_status anyway.
         * Just return FBE_STATUS_OK here.
         */
        fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt, 
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s, encl %d_%d, enclInfo not populated, ignore the change.\n",
                                  __FUNCTION__, pLocation->bus, pLocation->enclosure);

        return FBE_STATUS_OK;
    }

    pEnclDriveFaultLeds = fbe_memory_ex_allocate(sizeof(fbe_base_enclosure_led_behavior_t) * FBE_ENCLOSURE_MAX_NUMBER_OF_DRIVE_SLOTS);

    if(pEnclDriveFaultLeds==NULL)
    {
        fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt, 
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s, encl %d_%d, failed to allocate memory for drive led status.\n",
                                  __FUNCTION__, pLocation->bus, pLocation->enclosure);
        return FBE_STATUS_INSUFFICIENT_RESOURCES;
    }

    status = fbe_api_enclosure_get_encl_drive_slot_led_info(objectId, pEnclDriveFaultLeds);

    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt, 
                                  FBE_TRACE_LEVEL_WARNING,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s, encl %d_%d, failed to get drive LED info.\n",
                                  __FUNCTION__, pLocation->bus, pLocation->enclosure);
        fbe_memory_ex_release(pEnclDriveFaultLeds);
        return status;
    }

    for(slot = 0; slot < pEnclInfo->driveSlotCount; slot++)
    {
        fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt, 
                                  FBE_TRACE_LEVEL_DEBUG_LOW,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                              "%s, encl %d_%d, slot %d LED status 0x%x\n",
                              __FUNCTION__, pLocation->bus, pLocation->enclosure,
                              slot, pEnclDriveFaultLeds[slot]);

    }
    fbe_copy_memory(pEnclInfo->enclDriveFaultLeds, 
                    pEnclDriveFaultLeds, 
                    (sizeof(fbe_led_status_t) * FBE_ENCLOSURE_MAX_NUMBER_OF_DRIVE_SLOTS));
    fbe_memory_ex_release(pEnclDriveFaultLeds);
    return status;
}

/*!**************************************************************
 * @fn fbe_encl_mgmt_encl_log_event(fbe_encl_mgmt_t * pEnclMgmt,
 *                                     fbe_device_physical_location_t * pLocation,
 *                                     fbe_encl_info_t * pNewEnclInfo,
 *                                     fbe_encl_info_t * pOldEnclInfo)
 ****************************************************************
 * @brief
 *  Log the encl related events if needed.
 * 
 * @param pEnclMgmt - 
 * @param pLocation - 
 * @param pNewEnclInfo - 
 * @param pOldEnclInfo -
 *
 * @return fbe_status_t - the status of the event log write, else OK
 * @author
 *   23-Feb-2012: PHE - Created.
 ****************************************************************/
static fbe_status_t 
fbe_encl_mgmt_encl_log_event(fbe_encl_mgmt_t * pEnclMgmt,
                                fbe_device_physical_location_t * pLocation,
                                fbe_encl_info_t * pNewEnclInfo,
                                fbe_encl_info_t * pOldEnclInfo)
{
    fbe_status_t                status = FBE_STATUS_OK;
    fbe_u8_t                    deviceStr[FBE_DEVICE_STRING_LENGTH];
    char *enclStateStr;

    status = fbe_base_env_create_device_string(FBE_DEVICE_TYPE_ENCLOSURE, 
                                               pLocation, 
                                               &deviceStr[0],
                                               FBE_DEVICE_STRING_LENGTH);

    if(((pOldEnclInfo != NULL) && (pNewEnclInfo->enclState != pOldEnclInfo->enclState)) ||
       (pOldEnclInfo == NULL)) 
    {
        fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s %s, totalEnclCount %d.\n",
                              &deviceStr[0],
                              fbe_encl_mgmt_decode_encl_state(pNewEnclInfo->enclState),
                              pEnclMgmt->total_encl_count);
    }

    if((pNewEnclInfo->enclState == FBE_ESP_ENCL_STATE_OK) &&
       (pOldEnclInfo == NULL))
        {
            /* The encl was added OK. */
            status = fbe_event_log_write(ESP_INFO_ENCL_ADDED,
                                         NULL,
                                         0,
                                         "%s",
                                         &deviceStr[0]);
        }
    else if((pNewEnclInfo->enclState == FBE_ESP_ENCL_STATE_OK) &&
       (pOldEnclInfo->enclState == FBE_ESP_ENCL_STATE_MISSING))
    {
        /* The encl was added OK. */
        status = fbe_event_log_write(ESP_INFO_ENCL_ADDED,
                                     NULL, 
                                     0,
                                     "%s", 
                                     &deviceStr[0]);
    }
    else if((pOldEnclInfo == NULL) ||
            (pNewEnclInfo->enclState != pOldEnclInfo->enclState))
    {
        if(pOldEnclInfo == NULL)
        {
            enclStateStr = "MISSING";
        }
        else
        {
            enclStateStr = fbe_encl_mgmt_decode_encl_state(pOldEnclInfo->enclState);
        }
        /* The encl state changed. */
        status = fbe_event_log_write(ESP_ERROR_ENCL_STATE_CHANGED,
                            NULL, 0,
                            "%s %s %s", 
                            &deviceStr[0],
                            enclStateStr,
                            fbe_encl_mgmt_decode_encl_state(pNewEnclInfo->enclState));
    }    
    return(status);
}


/*!**************************************************************
 * fbe_encl_mgmt_getEnclBbuCount()
 ****************************************************************
 * @brief
 *  This function determines the Bbu count for this enclosure.
 *
 * @param pEnclMgmt - This is the object handle, or in our
 * case the encl_mgmt.
 *
 * @return fbe_status_t - FBE Status
 *
 * @author
 *  27-Jun-2014: Joe Perry - Created.
 *
 ****************************************************************/
static fbe_u32_t fbe_encl_mgmt_getEnclBbuCount(fbe_encl_mgmt_t *pEnclMgmt,
                                               fbe_device_physical_location_t *location)
{
    fbe_object_id_t                     objectId;
    fbe_board_mgmt_platform_info_t      boardPlatformInfo;
    fbe_u32_t                           bbuCount = 0;
    fbe_status_t                        status;

    status = fbe_api_get_board_object_id(&objectId);
    if (status != FBE_STATUS_OK) 
    {
        fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s can't get Board Object Id, status: 0x%X\n",
                              __FUNCTION__, status);
        return bbuCount;
    }

    status = fbe_api_board_get_platform_info(objectId, &boardPlatformInfo);
    if (status != FBE_STATUS_OK) 
    {
        fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s can't get Board Platform Info, status: 0x%X\n",
                              __FUNCTION__, status);
        return bbuCount;
    }

    /*
     * Currently only DPE or SPE have BBU's
     */
    if (((boardPlatformInfo.hw_platform_info.enclosureType == XPE_ENCLOSURE_TYPE) && 
        (location->bus == FBE_XPE_PSEUDO_BUS_NUM) && (location->enclosure == FBE_XPE_PSEUDO_ENCL_NUM)) ||
        ((boardPlatformInfo.hw_platform_info.enclosureType == VPE_ENCLOSURE_TYPE) && 
        (location->bus == FBE_XPE_PSEUDO_BUS_NUM) && (location->enclosure == FBE_XPE_PSEUDO_ENCL_NUM)) ||
        ((boardPlatformInfo.hw_platform_info.enclosureType == DPE_ENCLOSURE_TYPE) && 
            (location->bus == 0) && (location->enclosure == 0)))
    {

        status = fbe_api_enclosure_get_battery_count(objectId, &bbuCount);
        if (status != FBE_STATUS_OK) 
        {
            fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt, 
                                    FBE_TRACE_LEVEL_ERROR,
                                    FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                    "%s, Error getting bbuCount, status: 0x%X\n",
                                    __FUNCTION__, status);
            return bbuCount;
        }
        else
        {
            fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt, 
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s, bus 0x%x, encl 0x%x, bbuCount %d\n",
                                  __FUNCTION__,
                                  location->bus,
                                  location->enclosure,
                                  bbuCount);
        }
    }

    return bbuCount;

}   // end of fbe_encl_mgmt_getEnclBbuCount


/*!**************************************************************
 * fbe_encl_mgmt_getEnclSpsCount()
 ****************************************************************
 * @brief
 *  This function determines the SPS count for this enclosure.
 *
 * @param pEnclMgmt - This is the object handle, or in our
 * case the encl_mgmt.
 *
 * @return fbe_status_t - FBE Status
 *
 * @author
 *  27-Jun-2012: Joe Perry - Created.
 *
 ****************************************************************/
static fbe_u32_t fbe_encl_mgmt_getEnclSpsCount(fbe_encl_mgmt_t *pEnclMgmt,
                                               fbe_device_physical_location_t *location)
{
    fbe_object_id_t                     objectId;
    fbe_board_mgmt_platform_info_t      boardPlatformInfo;
    fbe_u32_t                           spsCount = 0;
    fbe_bool_t                          processorEnclosure = FALSE;
    fbe_status_t                        status;

    status = fbe_api_get_board_object_id(&objectId);
    if (status != FBE_STATUS_OK) 
    {
        fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s can't get Board Object Id, status: 0x%X\n",
                              __FUNCTION__, status);
        return spsCount;
    }

    status = fbe_api_board_get_platform_info(objectId, &boardPlatformInfo);
    if (status != FBE_STATUS_OK) 
    {
        fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s can't get Board Platform Info, status: 0x%X\n",
                              __FUNCTION__, status);
        return spsCount;
    }

    if (((boardPlatformInfo.hw_platform_info.enclosureType == XPE_ENCLOSURE_TYPE) && 
        (location->bus == FBE_XPE_PSEUDO_BUS_NUM) && (location->enclosure == FBE_XPE_PSEUDO_ENCL_NUM)) ||
        ((boardPlatformInfo.hw_platform_info.enclosureType == DPE_ENCLOSURE_TYPE) && 
            (location->bus == 0) && (location->enclosure == 0)) ||
        ((boardPlatformInfo.hw_platform_info.enclosureType == VPE_ENCLOSURE_TYPE) && 
            (location->bus == FBE_XPE_PSEUDO_BUS_NUM) && (location->enclosure == FBE_XPE_PSEUDO_ENCL_NUM)))
    {
        processorEnclosure = TRUE;
    }
    spsCount = HwAttrLib_getEnclosureSpsCount(&boardPlatformInfo.hw_platform_info,
                                              processorEnclosure,
                                              location->bus,
                                              location->enclosure);

    fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt, 
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s, PE %d, bus 0x%x, encl 0x%x, spsCount %d\n",
                          __FUNCTION__,
                          processorEnclosure,
                          location->bus,
                          location->enclosure,
                          spsCount);

    return spsCount;

}   // end of fbe_encl_mgmt_getEnclSpsCount

/*!**************************************************************
 * fbe_encl_mgmt_process_user_modified_wwn_seed_flag_change()
 ****************************************************************
 * @brief
 *  This function gets peer user modified wwn seed flag info from cmi,
 *  update local user modified wwn seed flag.
 *
 * @param pEnclMgmt -
 * @param pEnclCmiPkt - cmi info
 *
 * @return fbe_status_t - The status of the call to
 *                        fbe_lifecycle_class_const_verify() for
 *                        the enclosure management's constant
 *                        lifecycle data.
 *
 * @author
 *  23-Aug-2012:  Created. Dongz
 *
 ****************************************************************/
fbe_status_t fbe_encl_mgmt_process_user_modified_wwn_seed_flag_change(fbe_encl_mgmt_t * pEnclMgmt, fbe_encl_mgmt_cmi_packet_t *pEnclCmiPkt)
{
    fbe_encl_mgmt_user_modified_wwn_seed_flag_cmi_packet_t *cmi_packet;
    fbe_bool_t user_modify_wwn_seed_flag;
    cmi_packet = (fbe_encl_mgmt_user_modified_wwn_seed_flag_cmi_packet_t *) pEnclCmiPkt;
    user_modify_wwn_seed_flag = cmi_packet->user_modified_wwn_seed_flag;
    return fbe_encl_mgmt_reg_set_user_modified_wwn_seed_info(pEnclMgmt, user_modify_wwn_seed_flag);
}

/*!**************************************************************
 * fbe_encl_mgmt_check_system_id_info_cond_function()
 ****************************************************************
 * @brief
 *  This function determines which system id info we should use.
 *
 * @param object_handle - This is the object handle, or in our
 * case the encl_mgmt.
 * @param packet_p - The packet arriving from the monitor
 * scheduler.
 *
 * @return fbe_status_t - The status of the call to
 *                        fbe_lifecycle_class_const_verify()
 *                        for the enclosure management's
 *                        constant lifecycle data.
 *
 * @author
 *  27-Aug-2012: Dongz - Created.
 *
 ****************************************************************/
static fbe_lifecycle_status_t
fbe_encl_mgmt_check_system_id_info_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_encl_mgmt_system_id_info_t *system_id_info;
    fbe_encl_mgmt_t *encl_mgmt = (fbe_encl_mgmt_t *) base_object;
    fbe_encl_info_t *pe_info;
    RESUME_PROM_STRUCTURE *pe_rp_data;
    fbe_bool_t user_modified = FALSE;
    fbe_encl_mgmt_modify_system_id_info_t modify_resume_prom_system_id_info, modify_persistent_system_id_info;
    fbe_bool_t sys_id_sn_valid, rp_sn_valid, sys_id_pn_valid, rp_pn_valid, sys_id_rev_valid, rp_rev_valid;
    fbe_bool_t systemIdMismatch = FBE_FALSE;

    fbe_base_object_trace((fbe_base_object_t*)encl_mgmt,
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s, entry.\n",
                          __FUNCTION__);

    //get persistent storage system_id_info
    system_id_info = &encl_mgmt->system_id_info;

    /*
     * Get Processor Enclosure Info
     */
    pe_info = fbe_encl_mgmt_get_pe_info(encl_mgmt);

    if (pe_info == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t*)encl_mgmt,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "CHECK_SYSTEM_ID, can't find processor enclosure info.\n");

        fbe_transport_set_status(packet, status, 0);
        fbe_transport_complete_packet(packet);
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    pe_rp_data = &pe_info->enclResumeInfo.resume_prom_data;

    if(pe_info->enclResumeInfo.op_status == FBE_RESUME_PROM_STATUS_UNINITIATED)
    {
        fbe_base_object_trace((fbe_base_object_t*)encl_mgmt,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "CHECK_SYSTEM_ID, pe resume prom not initialized!\n");
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    fbe_encl_mgmt_check_system_id_info_mismatch(system_id_info, pe_rp_data, &systemIdMismatch);

    if(systemIdMismatch) 
    {
        fbe_base_object_trace((fbe_base_object_t*)encl_mgmt,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "CHECK_SYSTEM_ID, systemIdInfo MISMATCH!!!\n");
    }
    else
    { 
        fbe_base_object_trace((fbe_base_object_t*)encl_mgmt,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "CHECK_SYSTEM_ID, systemIdInfo MATCH!!!\n");

        status = fbe_lifecycle_clear_current_cond(base_object);
        if(status != FBE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t*)encl_mgmt,
                                  FBE_TRACE_LEVEL_WARNING,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "CHECK_SYSTEM_ID, clear condidion fail, status: %d.\n", status);
        }
    
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    fbe_zero_memory(&modify_resume_prom_system_id_info, sizeof(fbe_encl_mgmt_modify_system_id_info_t));
    fbe_zero_memory(&modify_persistent_system_id_info, sizeof(fbe_encl_mgmt_modify_system_id_info_t));

    sys_id_sn_valid = fbe_encl_mgmt_validate_serial_number(system_id_info->product_serial_number);
    if(!sys_id_sn_valid)
    {
        fbe_base_object_trace((fbe_base_object_t*)encl_mgmt,
                                  FBE_TRACE_LEVEL_WARNING,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "CHECK_SYSTEM_ID, detected invalid persistent serial number %s.\n",
                                  system_id_info->product_serial_number);
    }
    else
    {
        fbe_base_object_trace((fbe_base_object_t*)encl_mgmt,
                                  FBE_TRACE_LEVEL_WARNING,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "CHECK_SYSTEM_ID, detected valid persistent serial number %s.\n",
                                  system_id_info->product_serial_number);
    }
    rp_sn_valid = fbe_encl_mgmt_validate_serial_number(pe_rp_data->product_serial_number);
    if(!rp_sn_valid)
    {
        fbe_base_object_trace((fbe_base_object_t*)encl_mgmt,
                                  FBE_TRACE_LEVEL_WARNING,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "CHECK_SYSTEM_ID, detected invalid resume serial number %s.\n",
                                  pe_rp_data->product_serial_number);
    }
    else
    {
        fbe_base_object_trace((fbe_base_object_t*)encl_mgmt,
                                  FBE_TRACE_LEVEL_WARNING,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "CHECK_SYSTEM_ID, detected valid resume serial number %s.\n",
                                  pe_rp_data->product_serial_number);
    }

    sys_id_pn_valid = fbe_encl_mgmt_validate_part_number(system_id_info->product_part_number);
    rp_pn_valid = fbe_encl_mgmt_validate_part_number(pe_rp_data->product_part_number);

    sys_id_rev_valid = fbe_encl_mgmt_validate_rev(system_id_info->product_revision);
    rp_rev_valid = fbe_encl_mgmt_validate_rev(pe_rp_data->product_revision);

    //get user modified system id info
    status = fbe_encl_mgmt_reg_get_user_modified_system_id_flag(encl_mgmt, &user_modified);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*)encl_mgmt,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "CHECK_SYSTEM_ID, could not get user modified system id flag.\n");
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    //if user modified system id info flag set
    if(user_modified)
    {
        //if the persistent serial number is valid
        if(sys_id_sn_valid)
        {
            //if the resume prom serial number is valid
            if(rp_sn_valid)
            {
                fbe_base_object_trace((fbe_base_object_t*)encl_mgmt,
                                      FBE_TRACE_LEVEL_INFO,
                                      FBE_TRACE_MESSAGE_ID_INFO,
                                      "CHECK_SYSTEM_ID, user modify: yes, persistent valid, rp valid.\n");
                //User modified system id info,
                //SN valid in persistent storage and reusme prom, save sn to persistent storage
                //Update the SN, PN and Rev value in persistent storage.
                fbe_encl_mgmt_string_check_and_copy(modify_persistent_system_id_info.serialNumber,
                        pe_rp_data->product_serial_number,
                        RESUME_PROM_PRODUCT_SN_SIZE,
                        &modify_persistent_system_id_info.changeSerialNumber);

                //We do not need to validate PN and Rev here, for in persistent storage system id
                //write function we will check it.
                fbe_encl_mgmt_string_check_and_copy(modify_persistent_system_id_info.partNumber,
                        pe_rp_data->product_part_number,
                        RESUME_PROM_PRODUCT_PN_SIZE,
                        &modify_persistent_system_id_info.changePartNumber);

                fbe_encl_mgmt_string_check_and_copy(modify_persistent_system_id_info.revision,
                        pe_rp_data->product_revision,
                        RESUME_PROM_PRODUCT_REV_SIZE,
                        &modify_persistent_system_id_info.changeRev);

            }
            else
            {
                fbe_base_object_trace((fbe_base_object_t*)encl_mgmt,
                                      FBE_TRACE_LEVEL_INFO,
                                      FBE_TRACE_MESSAGE_ID_INFO,
                                      "CHECK_SYSTEM_ID, user modify: yes, persistent valid, rp invalid.\n");
                //User modified system id info
                //sn in persistent storage is valid while not valid in resume prom
                //save serial number to resume prom, not process PN and Rev
                fbe_encl_mgmt_string_check_and_copy(modify_resume_prom_system_id_info.serialNumber,
                        system_id_info->product_serial_number,
                        RESUME_PROM_PRODUCT_SN_SIZE,
                        &modify_resume_prom_system_id_info.changeSerialNumber);
            }
        }
        else
        {
            //if the resume prom serial number is valid
            if(rp_sn_valid)
            {
                fbe_base_object_trace((fbe_base_object_t*)encl_mgmt,
                                      FBE_TRACE_LEVEL_INFO,
                                      FBE_TRACE_MESSAGE_ID_INFO,
                                      "CHECK_SYSTEM_ID, user modify: yes, persistent invalid, rp valid.\n");
                //User modified system id info,
                //sn in resume prom is valid while not valid in persistent storage,
                //save SN, PN, Rev to persistent storage
                fbe_encl_mgmt_string_check_and_copy(modify_persistent_system_id_info.serialNumber,
                        pe_rp_data->product_serial_number,
                        RESUME_PROM_PRODUCT_SN_SIZE,
                        &modify_persistent_system_id_info.changeSerialNumber);

                //We do not need to validate PN and Rev here, for in persistent storage system id
                //write function we will check it.
                fbe_encl_mgmt_string_check_and_copy(modify_persistent_system_id_info.partNumber,
                                pe_rp_data->product_part_number,
                                RESUME_PROM_PRODUCT_PN_SIZE,
                                &modify_persistent_system_id_info.changePartNumber);

                fbe_encl_mgmt_string_check_and_copy(modify_persistent_system_id_info.revision,
                        pe_rp_data->product_revision,
                        RESUME_PROM_PRODUCT_REV_SIZE,
                        &modify_persistent_system_id_info.changeRev);
            }
            else
            {
                fbe_base_object_trace((fbe_base_object_t*)encl_mgmt,
                                      FBE_TRACE_LEVEL_INFO,
                                      FBE_TRACE_MESSAGE_ID_INFO,
                                      "CHECK_SYSTEM_ID, user modify: yes, persistent invalid, rp invalid.\n");
                //User modified system id info,
                //Sn invalid in resume prom and persistent storage
                // use persistent sn and update rp, don't process the pn or rev
                fbe_encl_mgmt_string_check_and_copy(modify_resume_prom_system_id_info.serialNumber,
                        system_id_info->product_serial_number,
                        RESUME_PROM_PRODUCT_SN_SIZE,
                        &modify_resume_prom_system_id_info.changeSerialNumber);
            }
        }
    }
    else
    {
        //if the persistent serial number is valid
        if(sys_id_sn_valid)
        {
            //if the resume prom serial number is valid
            if(rp_sn_valid)
            {
                fbe_base_object_trace((fbe_base_object_t*)encl_mgmt,
                                      FBE_TRACE_LEVEL_INFO,
                                      FBE_TRACE_MESSAGE_ID_INFO,
                                      "CHECK_SYSTEM_ID, user modify: no, persistent valid, rp valid.\n");
                //SN valid in persistent storage and reusme prom, save sn to resuem prom
                //Update the SN, PN and Rev value in resume prom
                fbe_encl_mgmt_string_check_and_copy(modify_resume_prom_system_id_info.serialNumber,
                        system_id_info->product_serial_number,
                        RESUME_PROM_PRODUCT_SN_SIZE,
                        &modify_resume_prom_system_id_info.changeSerialNumber);

                //We do not need to validate PN and Rev here, for in resume prom system id
                //write function we will check it.
                fbe_encl_mgmt_string_check_and_copy(modify_resume_prom_system_id_info.partNumber,
                        system_id_info->product_part_number,
                        RESUME_PROM_PRODUCT_PN_SIZE,
                        &modify_resume_prom_system_id_info.changePartNumber);

                fbe_encl_mgmt_string_check_and_copy(modify_resume_prom_system_id_info.revision,
                        system_id_info->product_revision,
                        RESUME_PROM_PRODUCT_REV_SIZE,
                        &modify_resume_prom_system_id_info.changeRev);

            }
            else
            {
                fbe_base_object_trace((fbe_base_object_t*)encl_mgmt,
                                      FBE_TRACE_LEVEL_INFO,
                                      FBE_TRACE_MESSAGE_ID_INFO,
                                      "CHECK_SYSTEM_ID, user modify: no, persistent valid, rp invalid.\n");
                //sn in persistent storage is valid while not valid in resume prom
                //if the rp.pn and rp.rev fields are not valid, update them from the persistent storage
                fbe_encl_mgmt_string_check_and_copy(modify_resume_prom_system_id_info.serialNumber,
                        system_id_info->product_serial_number,
                        RESUME_PROM_PRODUCT_SN_SIZE,
                        &modify_resume_prom_system_id_info.changeSerialNumber);

                if(!rp_pn_valid)
                {
                    fbe_encl_mgmt_string_check_and_copy(modify_resume_prom_system_id_info.partNumber,
                            system_id_info->product_part_number,
                            RESUME_PROM_PRODUCT_PN_SIZE,
                            &modify_resume_prom_system_id_info.changePartNumber);
                }

                if(!rp_rev_valid)
                {
                    fbe_encl_mgmt_string_check_and_copy(modify_resume_prom_system_id_info.revision,
                            system_id_info->product_revision,
                            RESUME_PROM_PRODUCT_REV_SIZE,
                            &modify_resume_prom_system_id_info.changeRev);
                }
            }
        }
        else
        {
            //if the resume prom serial number is valid
            if(rp_sn_valid)
            {
                fbe_base_object_trace((fbe_base_object_t*)encl_mgmt,
                                      FBE_TRACE_LEVEL_INFO,
                                      FBE_TRACE_MESSAGE_ID_INFO,
                                      "CHECK_SYSTEM_ID, user modify: no, persistent invalid, rp valid.\n");
                //sn in resume prom is valid while not valid in persistent storage,
                //save sn to persistent storage
                //If valid Product PN and / or rev in resume update fields in persistent storage.
                fbe_encl_mgmt_string_check_and_copy(modify_persistent_system_id_info.serialNumber,
                        pe_rp_data->product_serial_number,
                        RESUME_PROM_PRODUCT_SN_SIZE,
                        &modify_persistent_system_id_info.changeSerialNumber);

                //We do not need to validate PN and Rev here, for in persistent storage system id
                //write function we will check it.
                fbe_encl_mgmt_string_check_and_copy(modify_persistent_system_id_info.partNumber,
                        pe_rp_data->product_part_number,
                        RESUME_PROM_PRODUCT_PN_SIZE,
                        &modify_persistent_system_id_info.changePartNumber);

                fbe_encl_mgmt_string_check_and_copy(modify_persistent_system_id_info.revision,
                        pe_rp_data->product_revision,
                        RESUME_PROM_PRODUCT_REV_SIZE,
                        &modify_persistent_system_id_info.changeRev);
            }
            else
            {
                fbe_base_object_trace((fbe_base_object_t*)encl_mgmt,
                                      FBE_TRACE_LEVEL_INFO,
                                      FBE_TRACE_MESSAGE_ID_INFO,
                                      "CHECK_SYSTEM_ID, user modify: no, persistent invalid, rp invalid.\n");
                //Sn invalid in resume prom and persistent storage
                //this should only appear in test, DO NOT copy resume prom info to persistent storage.
                //report invalid for resume SN
                //If valid Product PN and / or rev in resume update fields in persistent storage.
                fbe_copy_memory(&pe_rp_data->product_serial_number,"INVALID_SER_NUM\0",RESUME_PROM_PRODUCT_SN_SIZE);
                fbe_copy_memory(&encl_mgmt->system_id_info.product_serial_number,"INVALID_SER_NUM\0",RESUME_PROM_PRODUCT_SN_SIZE);

                #if 0
                fbe_encl_mgmt_string_check_and_copy(modify_persistent_system_id_info.serialNumber,
                        "INVALID_SER_NUM\0",
                        RESUME_PROM_PRODUCT_SN_SIZE,
                        &modify_persistent_system_id_info.changeSerialNumber);

                //We do not need to validate PN and Rev here, for in persistent storage system id
                //write function we will check it.
                fbe_encl_mgmt_string_check_and_copy(modify_persistent_system_id_info.partNumber,
                        pe_rp_data->product_part_number,
                        RESUME_PROM_PRODUCT_PN_SIZE,
                        &modify_persistent_system_id_info.changePartNumber);

                fbe_encl_mgmt_string_check_and_copy(modify_persistent_system_id_info.revision,
                        pe_rp_data->product_revision,
                        RESUME_PROM_PRODUCT_REV_SIZE,
                        &modify_persistent_system_id_info.changeRev);
                #endif

                //add a trace here
                fbe_base_object_trace((fbe_base_object_t*)encl_mgmt,
                        FBE_TRACE_LEVEL_WARNING,
                                      FBE_TRACE_MESSAGE_ID_INFO,
                                      "CHECK_SYSTEM_ID, serial number invalid.\n");

                status = fbe_encl_mgmt_update_encl_fault_led(encl_mgmt,
                            &pe_info->location,
                            FBE_ENCL_FAULT_LED_SYSTEM_SERIAL_NUMBER_FLT);
                if(status != FBE_STATUS_OK)
                {
                    fbe_base_object_trace((fbe_base_object_t *)encl_mgmt,
                                      FBE_TRACE_LEVEL_WARNING,
                                      FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                      "CHECK_SYSTEM_ID, PE update_encl_fault_led failed, status: 0x%X.\n",
                                      status);
                }

            }
        }
    }

    /* Do not update the hardware resume prom here. The reason is that it would take long time to update.
     * ESP driver entry waits for the encl_mgmt object getting out of the specialize state.
     * We don't want hold up on the ESP driver entry too long.
     */
    fbe_copy_memory(&encl_mgmt->modify_rp_sys_id_info,
            &modify_resume_prom_system_id_info,
            sizeof(fbe_encl_mgmt_modify_system_id_info_t));

    //save persistent storage system id info
    status = fbe_encl_mgmt_set_persistent_system_id_info(encl_mgmt, &modify_persistent_system_id_info);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*)encl_mgmt,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "CHECK_SYSTEM_ID, write persistent storage system id fail, status: %d.\n",
                              status);
    }

    if ((encl_mgmt->modify_rp_sys_id_info.changeSerialNumber) || 
        (encl_mgmt->modify_rp_sys_id_info.changePartNumber) ||
        (encl_mgmt->modify_rp_sys_id_info.changeRev))
    {

        fbe_base_object_trace((fbe_base_object_t*)encl_mgmt,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "CHECK_SYSTEM_ID, setting WRITE_ARRAY_MIDPLANE_RP_COND.\n");
       
        status = fbe_lifecycle_set_cond(&fbe_encl_mgmt_lifecycle_const,
                                        (fbe_base_object_t*)encl_mgmt,
                                        FBE_ENCL_MGMT_WRITE_ARRAY_MIDPLANE_RP_COND);
        if(status != FBE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t*)encl_mgmt,
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "CHECK_SYSTEM_ID, can't set check system id info condition.\n");
        }
    }
    else
    {
        if (user_modified)
        {
            status = fbe_encl_mgmt_reg_set_user_modified_system_id_flag(encl_mgmt, FALSE);
        }
    }

    status = fbe_lifecycle_clear_current_cond(base_object);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*)encl_mgmt,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "CHECK_SYSTEM_ID, clear condidion fail, status: %d.\n", status);
    }

    return FBE_LIFECYCLE_STATUS_DONE;
}

/*!**************************************************************
 * @fn fbe_encl_mgmt_check_system_id_info_mismatch(fbe_encl_mgmt_system_id_info_t * pSystemIdInfoFromPersistentStorage,
 *           RESUME_PROM_STRUCTURE * pResumeProm,
 *           fbe_bool_t * pMismatch)
 ****************************************************************
 * @brief
 *  This function compares the system id info from the persistent storage
 *  and the system id info from the array midplane resume prom.
 *  to see whether then mismatch.
 *
 * @param pSystemIdInfoFromPersistentStorage -
 * @param pResumeProm - The pointer to the array midplane resume prom data
 * @param pMismatch (OUTPUT) -
 * 
 * @return FBE_STATUS_OK 
 *
 * @version
 * 05-Nov-2012:  PHE - Created.
 *
 ****************************************************************/
static fbe_status_t 
fbe_encl_mgmt_check_system_id_info_mismatch(fbe_encl_mgmt_system_id_info_t * pSystemIdInfoFromPersistentStorage,
                                            RESUME_PROM_STRUCTURE * pResumeProm,
                                            fbe_bool_t * pMismatch)
{
    *pMismatch = FBE_FALSE;
    if(strncmp(&pSystemIdInfoFromPersistentStorage->product_serial_number[0], 
               &pResumeProm->product_serial_number[0], 
               RESUME_PROM_PRODUCT_SN_SIZE) != 0)
    {
        *pMismatch = FBE_TRUE;
    }
    else if(strncmp(&pSystemIdInfoFromPersistentStorage->product_part_number[0], 
                    &pResumeProm->product_part_number[0], 
                    RESUME_PROM_PRODUCT_PN_SIZE) != 0)
    {
        *pMismatch = FBE_TRUE;
    }
    else if(strncmp(&pSystemIdInfoFromPersistentStorage->product_revision[0], 
                    &pResumeProm->product_revision[0], 
                    RESUME_PROM_PRODUCT_REV_SIZE) != 0)
    {
        *pMismatch = FBE_TRUE;
    }
    else
    {
        *pMismatch = FBE_FALSE;
    }

    return FBE_STATUS_OK;
}

/*!**************************************************************
 * fbe_encl_mgmt_read_persistent_data_complete_cond_function()
 ****************************************************************
 * @brief
 *  This function handles the persistent storage read completion.
 *
 *
 * @param object_handle - This is the object handle, or in our
 * case the encl_mgmt.
 * @param packet_p - The packet arriving from the monitor
 * scheduler.
 *
 * @return fbe_status_t - The status of the call to
 *                        fbe_lifecycle_class_const_verify()
 *                        for the module management's
 *                        constant lifecycle data.
 *
 *
 * @author
 *  28-Aug-2012 - Created. Dongz
 *
 ****************************************************************/
static fbe_lifecycle_status_t fbe_encl_mgmt_read_persistent_data_complete_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet)
{
    fbe_encl_mgmt_t *encl_mgmt = (fbe_encl_mgmt_t *)base_object;
    fbe_encl_mgmt_persistent_info_t *persistent_system_id_info;
    fbe_status_t status;

    //if system id has already initialized, skip following code.
    if(encl_mgmt->system_id_info_initialized == FBE_FALSE)
    {
        /*
         * Copy data over from persistent storage into the structures.
         */
        persistent_system_id_info = (fbe_encl_mgmt_persistent_info_t *)((fbe_base_environment_t *)encl_mgmt)->my_persistent_data;

        fbe_copy_memory(&encl_mgmt->system_id_info, &persistent_system_id_info->system_id_info, sizeof(fbe_encl_mgmt_system_id_info_t));

        //set check system id info condition
        status = fbe_lifecycle_set_cond(&fbe_encl_mgmt_lifecycle_const,
                                            (fbe_base_object_t*)encl_mgmt,
                                            FBE_BASE_ENV_LIFECYCLE_COND_CHECK_SYSTEM_ID_INFO);
        if(status != FBE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t*)encl_mgmt,
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s, can't set check system id info condition.\n",
                                  __FUNCTION__);
            return FBE_LIFECYCLE_STATUS_RESCHEDULE;
        }

        encl_mgmt->system_id_info_initialized = FBE_TRUE;
    }

    status = fbe_lifecycle_clear_current_cond(base_object);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*)encl_mgmt,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s, clear condidion fail, status: %d.\n",
                              __FUNCTION__, status);
    }

    return FBE_LIFECYCLE_STATUS_DONE;
}

/*!**************************************************************
 * fbe_encl_mgmt_read_array_midplane_resume_prom_cond_function()
 ****************************************************************
 * @brief
 *  This function read array midplane rp info from board object
 *
 * @param object_handle - This is the object handle, or in our
 * case the encl_mgmt.
 * @param packet_p - The packet arriving from the monitor
 * scheduler.
 * 
 * @note
 *   It may take long time to read the resume prom. We
 *   don't want to block other ESP objects to get out of the specialize state.
 *   So use the asynchronous read.
 *   The condition will be cleared when the read completes which sets the update resume prom condition.
 *   While running update resume prom condition funcition, the work item will be removed from the queue.
 *   So this condition will be cleared.
 * 
 * @return fbe_status_t - The status of the call to
 *                        fbe_lifecycle_class_const_verify() for
 *                        the enclosure management's constant
 *                        lifecycle data.
 *
 * @author
 *  23-Aug-2012:  Created. Dongz
 *  05-Nov-2012: PHE - updated to use the asynchronous read for the array midplane resuem prom.
 ****************************************************************/
static fbe_lifecycle_status_t
fbe_encl_mgmt_read_array_midplane_resume_prom_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet)
{
    fbe_status_t                   status = FBE_STATUS_OK;
    fbe_encl_mgmt_t              * pEnclMgmt = (fbe_encl_mgmt_t *)base_object;
    fbe_base_environment_t       * pBaseEnv = (fbe_base_environment_t *)base_object;
    fbe_encl_info_t              * pEnclInfo = NULL;
    fbe_bool_t                     needToIssueRead = FBE_FALSE;
    fbe_base_env_resume_prom_work_item_t * pWorkItem = NULL;

    fbe_base_object_trace((fbe_base_object_t*)pEnclMgmt,
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s, entry.\n",
                          __FUNCTION__);

    /*
     * Get Processor Enclosure Info
     */
    pEnclInfo = fbe_encl_mgmt_get_pe_info(pEnclMgmt);

    if (pEnclInfo == NULL)
    {
        /* This is coding error. */
        fbe_base_object_trace((fbe_base_object_t*)pEnclMgmt,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s, can't find processor enclosure info.\n",
                              __FUNCTION__);

        status = fbe_lifecycle_clear_current_cond(base_object);
        if(status != FBE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t*)pEnclMgmt,
                FBE_TRACE_LEVEL_ERROR,
                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                "%s, clear condition fail.\n",
                __FUNCTION__);
        }
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /* Check whether there is a work item for the array midplane resume prom. */
    fbe_base_env_resume_prom_find_work_item(pBaseEnv, 
                                            FBE_DEVICE_TYPE_ENCLOSURE, 
                                            &pEnclInfo->location, 
                                            &pWorkItem);

    if(pWorkItem == NULL)
    {
        status = fbe_lifecycle_clear_current_cond(base_object);
        if(status != FBE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t*)pEnclMgmt,
                FBE_TRACE_LEVEL_ERROR,
                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                "%s, clear condition fail.\n",
                __FUNCTION__);
        }
        else
        {
            fbe_base_object_trace((fbe_base_object_t*)pEnclMgmt,
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s, array midplane resume prom read is done.\n",
                          __FUNCTION__);
        }

        return FBE_LIFECYCLE_STATUS_DONE;
    }

    if (pWorkItem->workState == FBE_BASE_ENV_RESUME_PROM_READ_QUEUED)
    {
        if (pWorkItem->readCmdTimeStamp == 0
                || (fbe_get_elapsed_milliseconds(pWorkItem->readCmdTimeStamp)>= FBE_BASE_ENV_RESUME_PROM_RETRY_INTERVAL))
        {
            needToIssueRead = FBE_TRUE;
        }
    }
   
    if(needToIssueRead) 
    {
        /* Issue the resume prom read */
        status = fbe_base_env_send_resume_prom_read_async_cmd(pBaseEnv, pWorkItem);
        pWorkItem->readCmdTimeStamp = fbe_get_time();
    
        if(status != FBE_STATUS_OK && status != FBE_STATUS_PENDING)
        {
            /* Retry required */
            fbe_base_object_customizable_trace((fbe_base_object_t*)pBaseEnv,
                                                   FBE_TRACE_LEVEL_ERROR,
                                                   fbe_base_env_get_resume_prom_work_item_logheader(pWorkItem),
                                                   "send_resume_prom_read_async_cmd failed, status 0x%x, will retry.\n",
                                                   status);
    
            /* Set the workState so that it can be retried
             * when the condition function gets to run again.
             */
            pWorkItem->workState = FBE_BASE_ENV_RESUME_PROM_READ_QUEUED;
        }
        else
        {
            fbe_base_object_customizable_trace((fbe_base_object_t*)pBaseEnv,
                                                   FBE_TRACE_LEVEL_INFO,
                                                   fbe_base_env_get_resume_prom_work_item_logheader(pWorkItem),
                                                   "send_resume_prom_read_async_cmd is sent successfully, retry count: %d.\n",
                                                   pWorkItem->retryCount);
        }
    }
   
    return FBE_LIFECYCLE_STATUS_DONE;
}

/*!**************************************************************
 * fbe_encl_mgmt_write_array_midplane_resume_prom_cond_function()
 ****************************************************************
 * @brief
 *  This function read array midplane rp info from board object
 *
 * @param object_handle - This is the object handle, or in our
 * case the encl_mgmt.
 * @param packet_p - The packet arriving from the monitor
 * scheduler.
 *
 * @return fbe_status_t - The status of the call to
 *                        fbe_lifecycle_class_const_verify() for
 *                        the enclosure management's constant
 *                        lifecycle data.
 *
 * @author
 *  23-Aug-2012:  Created. Dongz
 *
 ****************************************************************/
static fbe_lifecycle_status_t
fbe_encl_mgmt_write_array_midplane_resume_prom_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet)
{
    fbe_status_t status;
    fbe_encl_mgmt_t *pEnclMgmt = (fbe_encl_mgmt_t *)base_object;
    fbe_encl_info_t                       * pEnclInfo = NULL;

    fbe_base_object_trace((fbe_base_object_t*)pEnclMgmt,
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s, entry.\n",
                          __FUNCTION__);

    if(!fbe_base_env_is_active_sp_esp_cmi((fbe_base_environment_t*)pEnclMgmt))
    {
        //if local sp is not active, there is no change needed
        fbe_base_object_trace((fbe_base_object_t*)pEnclMgmt,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s, this is passive sp.\n",
                              __FUNCTION__);
        status = fbe_lifecycle_clear_current_cond(base_object);
        if(status != FBE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t*)pEnclMgmt,
                FBE_TRACE_LEVEL_ERROR,
                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                "%s, clear condition fail.\n",
                __FUNCTION__);
        }
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /*
     * Get Processor Enclosure Info
     */
    pEnclInfo = fbe_encl_mgmt_get_pe_info(pEnclMgmt);

    if (pEnclInfo == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t*)pEnclMgmt,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s, can't find processor enclosure info.\n",
                              __FUNCTION__);
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    //save the resume prom system id info
    status = fbe_encl_mgmt_set_resume_prom_system_id_info_async(pEnclMgmt, pEnclInfo->location, &pEnclMgmt->modify_rp_sys_id_info);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*)pEnclMgmt,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s, write resume prom system id fail, status: %d.\n",
                              __FUNCTION__, status);
        return FBE_LIFECYCLE_STATUS_DONE;
    }


    status = fbe_lifecycle_clear_current_cond((fbe_base_object_t *)pEnclMgmt);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*)pEnclMgmt,
            FBE_TRACE_LEVEL_ERROR,
            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
            "%s, clear condition fail.\n",
            __FUNCTION__);
    }
    return FBE_LIFECYCLE_STATUS_DONE;
}

/*!**************************************************************
 * fbe_encl_mgmt_resume_prom_write_async_completion_function()
 ****************************************************************
 * @brief
 *  Callback function to handle async write resume prom.
 *  It will read resume prom again if require and completes
 *  the write_array_midplane_resume_prom condition which was pending.
 *
 * @param object_handle - This is the object handle, or in our
 * case the encl_mgmt.
 * @param packet_p - The packet arriving from the monitor
 * scheduler.
 *
 * @return fbe_status_t - The status of the call to
 *                        fbe_lifecycle_class_const_verify() for
 *                        the enclosure management's constant
 *                        lifecycle data.
 *
 * @author
 *  17-Dec-2012:  Created. Dipak
 *
 ****************************************************************/
fbe_status_t 
fbe_encl_mgmt_resume_prom_write_async_completion_function(fbe_packet_completion_context_t pContext, 
                                                          void * rpWriteAsynchOp)
{
    fbe_status_t                               status = FBE_STATUS_OK;
    fbe_u64_t                          device_type;
    fbe_device_physical_location_t           * pLocation = NULL;
    fbe_encl_mgmt_t                          *pEnclMgmt = (fbe_encl_mgmt_t *)pContext;
    fbe_resume_prom_write_resume_async_op_t * rpWriteAsynch = (fbe_resume_prom_write_resume_async_op_t *) rpWriteAsynchOp;



    device_type = rpWriteAsynch->rpWriteAsynchCmd.deviceType;
    pLocation = &rpWriteAsynch->rpWriteAsynchCmd.deviceLocation;
    
    /* Read the write resume prom command status*/
            fbe_base_object_trace((fbe_base_object_t*)pEnclMgmt,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s, Write async completed, Status %d.\n",
                              __FUNCTION__, rpWriteAsynch->status);

    /* Read resume prom*/
    if(rpWriteAsynch->status == FBE_STATUS_OK) 
    {
        /* Update flags in modify_sp_sys_is_info structures */
        pEnclMgmt->modify_rp_sys_id_info.changePartNumber = FALSE;
        pEnclMgmt->modify_rp_sys_id_info.changeSerialNumber = FALSE;
        pEnclMgmt->modify_rp_sys_id_info.changeRev = FALSE;

        //issue rp read
        status = fbe_encl_mgmt_initiate_resume_prom_read(pEnclMgmt,
                                              FBE_DEVICE_TYPE_ENCLOSURE,
                                              pLocation);
    
        if(status != FBE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t*)pEnclMgmt,
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s, failed to initiate resume read after write\n",
                                  __FUNCTION__);
        }

        status = fbe_encl_mgmt_reg_set_user_modified_system_id_flag(pEnclMgmt, FALSE);
    }
    else
    {
        status = fbe_lifecycle_set_cond(&fbe_encl_mgmt_lifecycle_const,
                                        (fbe_base_object_t*)pEnclMgmt,
                                        FBE_ENCL_MGMT_WRITE_ARRAY_MIDPLANE_RP_COND);
        if(status != FBE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t*)pEnclMgmt,
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s, can't set check system id info condition.\n",
                                  __FUNCTION__);
        }

    }

    /* Free the allocated memory */
    fbe_transport_release_buffer(rpWriteAsynch->rpWriteAsynchCmd.pBuffer);
    rpWriteAsynch->rpWriteAsynchCmd.pBuffer = NULL;
   
    fbe_base_env_memory_ex_release((fbe_base_environment_t *)pEnclMgmt, rpWriteAsynch);
    rpWriteAsynch = NULL;

    return FBE_STATUS_OK;
} /* End of function fbe_base_env_resume_prom_read_completion_function */
/*!**************************************************************
 * fbe_encl_mgmt_set_persistent_system_id_info()
 ****************************************************************
 * @brief
 *  This function set persistent system id info.
 *
 * @param encl_mgmt - context
 * @param modify_sys_info - The command indicate which field
 *                          should update
 *
 * @return fbe_status_t - The status of the call to
 *                        fbe_lifecycle_class_const_verify()
 *                        for the enclosure management's
 *                        constant lifecycle data.
 *
 * @author
 *  27-Aug-2012: Dongz - Created.
 *
 ****************************************************************/
fbe_status_t fbe_encl_mgmt_set_persistent_system_id_info(fbe_encl_mgmt_t * encl_mgmt,
                                                         fbe_encl_mgmt_modify_system_id_info_t *modify_sys_info)
{
    fbe_encl_mgmt_persistent_info_t *persistent_data_p;
    fbe_status_t status = FBE_STATUS_OK;

    fbe_base_object_trace((fbe_base_object_t*)encl_mgmt,
            FBE_TRACE_LEVEL_INFO,
            FBE_TRACE_MESSAGE_ID_INFO,
            "%s, function entry.\n",
            __FUNCTION__);

    if(!(modify_sys_info->changeSerialNumber || modify_sys_info->changePartNumber || modify_sys_info->changeRev))
    {
        //if nothing need to change, we will quit.
        fbe_base_object_trace((fbe_base_object_t*)encl_mgmt,
                FBE_TRACE_LEVEL_INFO,
                FBE_TRACE_MESSAGE_ID_INFO,
                "%s, nothing need to change.\n",
                __FUNCTION__);
        return FBE_STATUS_OK;

    }
    if(modify_sys_info->changeSerialNumber &&
            fbe_encl_mgmt_validate_serial_number(modify_sys_info->serialNumber))
    {
        fbe_copy_memory(encl_mgmt->system_id_info.product_serial_number,
                &modify_sys_info->serialNumber,
                RESUME_PROM_PRODUCT_SN_SIZE);
    }

    if(modify_sys_info->changePartNumber &&
            fbe_encl_mgmt_validate_part_number(modify_sys_info->partNumber))
    {
        fbe_copy_memory(encl_mgmt->system_id_info.product_part_number,
                &modify_sys_info->partNumber,
                RESUME_PROM_PRODUCT_PN_SIZE);
    }

    if(modify_sys_info->changeRev &&
            fbe_encl_mgmt_validate_rev(modify_sys_info->revision))
    {
        fbe_copy_memory(encl_mgmt->system_id_info.product_revision,
                &modify_sys_info->revision,
                RESUME_PROM_PRODUCT_REV_SIZE);
    }

    fbe_base_object_trace((fbe_base_object_t*)encl_mgmt,
            FBE_TRACE_LEVEL_INFO,
            FBE_TRACE_MESSAGE_ID_INFO,
            "%s, write persistent sn: %s, pn: %s, rev: %s.\n",
            __FUNCTION__,
            encl_mgmt->system_id_info.product_serial_number,
            encl_mgmt->system_id_info.product_part_number,
            encl_mgmt->system_id_info.product_revision);

    if(!fbe_base_env_is_active_sp_esp_cmi((fbe_base_environment_t*)encl_mgmt))
    {
        //if local sp is not active, there is no change needed
        fbe_base_object_trace((fbe_base_object_t*)encl_mgmt,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s, this is passive sp.\n",
                              __FUNCTION__);

        return FBE_STATUS_OK;
    }
    persistent_data_p = (fbe_encl_mgmt_persistent_info_t *) ((fbe_base_environment_t *)encl_mgmt)->my_persistent_data;

    fbe_copy_memory(&persistent_data_p->system_id_info,
                    &encl_mgmt->system_id_info,
                    sizeof(fbe_encl_mgmt_system_id_info_t));

    status = fbe_base_env_write_persistent_data((fbe_base_environment_t *)encl_mgmt);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)encl_mgmt,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s persistent write error, status: 0x%X",
                              __FUNCTION__, status);
    }
    return FBE_STATUS_OK;
}

/*!**************************************************************
 * fbe_encl_mgmt_process_user_modified_sys_id_flag_change()
 ****************************************************************
 * @brief
 *  This function gets peer user modified system id flag info from cmi,
 *  update local user modified system id flag.
 *
 * @param pEnclMgmt -
 * @param pEnclCmiPkt - cmi info
 *
 * @return fbe_status_t - The status of the call to
 *                        fbe_lifecycle_class_const_verify() for
 *                        the enclosure management's constant
 *                        lifecycle data.
 *
 * @author
 *  23-Aug-2012:  Created. Dongz
 *
 ****************************************************************/
fbe_status_t fbe_encl_mgmt_process_user_modified_sys_id_flag_change(fbe_encl_mgmt_t * pEnclMgmt,
                                                                    fbe_encl_mgmt_cmi_packet_t *pEnclCmiPkt)
{
    fbe_encl_mgmt_user_modified_sys_id_flag_cmi_packet_t *cmi_packet;
    fbe_bool_t user_modified_sys_id_flag;
    cmi_packet = (fbe_encl_mgmt_user_modified_sys_id_flag_cmi_packet_t *) pEnclCmiPkt;

    fbe_base_object_trace((fbe_base_object_t*)pEnclMgmt,
            FBE_TRACE_LEVEL_INFO,
            FBE_TRACE_MESSAGE_ID_INFO,
            "%s, entry.\n",
            __FUNCTION__);

    user_modified_sys_id_flag = cmi_packet->user_modified_sys_id_flag;
    return fbe_encl_mgmt_reg_set_user_modified_system_id_flag(pEnclMgmt, user_modified_sys_id_flag);
}

/*!**************************************************************
 * fbe_encl_mgmt_process_persistent_sys_id_change()
 ****************************************************************
 * @brief
 *  This function gets peer persistent sys id change info from cmi,
 *  update local persistent storage system id info.
 *
 * @param pEnclMgmt -
 * @param pEnclCmiPkt - cmi info
 *
 * @return fbe_status_t - The status of the call to
 *                        fbe_lifecycle_class_const_verify() for
 *                        the enclosure management's constant
 *                        lifecycle data.
 *
 * @author
 *  23-Aug-2012:  Created. Dongz
 *
 ****************************************************************/
fbe_status_t fbe_encl_mgmt_process_persistent_sys_id_change(fbe_encl_mgmt_t * pEnclMgmt,
                                                                    fbe_encl_mgmt_cmi_packet_t *pEnclCmiPkt)
{
    fbe_encl_mgmt_persistent_sys_id_change_cmi_packet_t *cmi_packet;
    cmi_packet = (fbe_encl_mgmt_persistent_sys_id_change_cmi_packet_t *) pEnclCmiPkt;

    fbe_base_object_trace((fbe_base_object_t*)pEnclMgmt,
            FBE_TRACE_LEVEL_INFO,
            FBE_TRACE_MESSAGE_ID_INFO,
            "%s, entry.\n",
            __FUNCTION__);
    return fbe_encl_mgmt_set_persistent_system_id_info(pEnclMgmt, &cmi_packet->persistent_sys_id_change_info);
}

/*!**************************************************************
 * fbe_encl_mgmt_process_encl_cabling_request()
 ****************************************************************
 * @brief
 *  This function processes peer enclosure cabling request message,
 *  determine if the peer side is miscabled and then send back the
 *  response.
 *
 * @param pEnclMgmt - the pointer to enclosure management
 * @param peerEnclInfo - peer enclosure cabling info
 *
 * @return fbe_status_t - FBE Status
 *
 * @author
 *  22-Nov-2012:  Created. LL
 *
 ****************************************************************/
fbe_status_t fbe_encl_mgmt_process_encl_cabling_request(fbe_encl_mgmt_t * pEnclMgmt,
                                                  fbe_encl_mgmt_encl_info_t * peerEnclInfo)
{
    fbe_status_t                           status = FBE_STATUS_OK;
    fbe_u32_t                              enclIndex = 0;
    fbe_u32_t                              enclCount = 0;
    fbe_encl_info_t                      * pEnclInfo = NULL;
    fbe_bool_t                             miscabled = FBE_FALSE;
    fbe_encl_mgmt_encl_info_cmi_packet_t   enclMsgPkt = {0};

    fbe_base_object_trace((fbe_base_object_t*)pEnclMgmt,
            FBE_TRACE_LEVEL_INFO,
            FBE_TRACE_MESSAGE_ID_INFO,
            "Encl(%d_%d), %s, entry.\n",
            peerEnclInfo->location.bus, peerEnclInfo->location.enclosure, __FUNCTION__);

    for(enclIndex = 0; enclIndex < FBE_ESP_MAX_ENCL_COUNT; enclIndex++)
    {
        pEnclInfo = pEnclMgmt->encl_info[enclIndex];

        if(pEnclInfo->object_id != FBE_OBJECT_ID_INVALID)
        {
            /* same location, different serial number */
            if((pEnclInfo->location.bus == peerEnclInfo->location.bus) &&
               (pEnclInfo->location.enclosure == peerEnclInfo->location.enclosure))
            {
                if(!fbe_equal_memory(peerEnclInfo->serial_number, pEnclInfo->serial_number, sizeof(pEnclInfo->serial_number)))
                {
                    miscabled = FBE_TRUE;
                    break;
                }
            }
            /* same serial number, different location */
            else if(fbe_equal_memory(peerEnclInfo->serial_number, pEnclInfo->serial_number, sizeof(pEnclInfo->serial_number)))
            {
                miscabled = FBE_TRUE;
                break;
            }

            if(++enclCount >= pEnclMgmt->total_encl_count)
            {
                /* Reach the total enclosure count, no need to continue.*/
                break;
            }
        }
    }

    enclMsgPkt.baseCmiMsg.opcode = miscabled ? FBE_BASE_ENV_CMI_MSG_ENCL_CABLING_NACK : FBE_BASE_ENV_CMI_MSG_ENCL_CABLING_ACK;
    enclMsgPkt.encl_info.location = peerEnclInfo->location;

    fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt,
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "Encl(%d_%d), sending cabling response to peer: %s.\n",
                          peerEnclInfo->location.bus, peerEnclInfo->location.enclosure, miscabled ? "NACK" : "ACK");

    status = fbe_base_environment_send_cmi_message((fbe_base_environment_t *)pEnclMgmt,
                                                   sizeof(fbe_encl_mgmt_encl_info_cmi_packet_t),
                                                   &enclMsgPkt);

    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s Error sending enclosure cabling response to peer, status: 0x%X\n",
                              __FUNCTION__,
                              status);
    }

    if(!miscabled) 
    {
        /* When the peer enclosure has the symetric cabling with this enclosure, 
         * need to check whether this local enclosure was marked as miscabled before. 
         * If yes, we need to clear the miscabling fault. 
         */
        fbe_encl_mgmt_process_encl_cabling_ack(pEnclMgmt, peerEnclInfo);
    }

    return status;
}

/*!**************************************************************
 * fbe_encl_mgmt_process_encl_cabling_ack()
 ****************************************************************
 * @brief
 *  This function processes peer enclosure cabling ack message and
 *  update local enclosure miscabling fault status.
 *
 * @param pEnclMgmt - the pointer to enclosure management
 * @param peerEnclInfo - peer enclosure cabling info
 *
 * @return fbe_status_t - FBE Status
 *
 * @author
 *  22-Nov-2012:  Created. LL
 *
 ****************************************************************/
fbe_status_t fbe_encl_mgmt_process_encl_cabling_ack(fbe_encl_mgmt_t * pEnclMgmt,
                                                  fbe_encl_mgmt_encl_info_t * peerEnclInfo)
{
    fbe_status_t                           status = FBE_STATUS_OK;
    fbe_encl_info_t                      * pEnclInfo = NULL;

    fbe_base_object_trace((fbe_base_object_t*)pEnclMgmt,
            FBE_TRACE_LEVEL_INFO,
            FBE_TRACE_MESSAGE_ID_INFO,
            "Encl(%d_%d), %s, entry.\n",
            peerEnclInfo->location.bus, peerEnclInfo->location.enclosure, __FUNCTION__);

    status = fbe_encl_mgmt_get_encl_info_by_location(pEnclMgmt, &peerEnclInfo->location, &pEnclInfo);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "Encl(%d_%d) get_encl_info_by_location failed, status 0x%x.\n",
                              peerEnclInfo->location.bus,
                              peerEnclInfo->location.enclosure,
                              status);
        return status;
    }

    fbe_spinlock_lock(&pEnclInfo->encl_info_lock);
    if(pEnclInfo->enclFaultSymptom == FBE_ESP_ENCL_FAULT_SYM_BE_LOOP_MISCABLED)
    {
        pEnclInfo->enclState = FBE_ESP_ENCL_STATE_OK;
        pEnclInfo->enclFaultSymptom = FBE_ESP_ENCL_FAULT_SYM_NO_FAULT;
    }
    fbe_spinlock_unlock(&pEnclInfo->encl_info_lock);

    status = fbe_encl_mgmt_update_encl_fault_led(pEnclMgmt,
                &pEnclInfo->location,
                FBE_ENCL_FAULT_LED_LCC_CABLING_FLT);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt,
                          FBE_TRACE_LEVEL_WARNING,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "Encl(%d_%d), CABLING_FLT, update_encl_fault_led failed, status: 0x%X.\n",
                          pEnclInfo->location.bus,
                          pEnclInfo->location.enclosure,
                          status);
    }

    fbe_base_environment_send_data_change_notification((fbe_base_environment_t*) pEnclMgmt,
                                                       FBE_CLASS_ID_ENCL_MGMT,
                                                       FBE_DEVICE_TYPE_ENCLOSURE,
                                                       FBE_DEVICE_DATA_TYPE_GENERIC_INFO,
                                                       &pEnclInfo->location);

    return status;
}

/*!**************************************************************
 * fbe_encl_mgmt_process_encl_cabling_nack()
 ****************************************************************
 * @brief
 *  This function processes peer enclosure cabling no ack message.
 *  The peer marks local enclosure miscabled, so local side should
 *  update enclosure miscabling fault status.
 *
 * @param pEnclMgmt - the pointer to enclosure management
 * @param peerEnclInfo - peer enclosure cabling info
 *
 * @return fbe_status_t - FBE Status
 *
 * @author
 *  22-Nov-2012:  Created. LL
 *
 ****************************************************************/
fbe_status_t fbe_encl_mgmt_process_encl_cabling_nack(fbe_encl_mgmt_t * pEnclMgmt,
                                                  fbe_encl_mgmt_encl_info_t * peerEnclInfo)
{
    fbe_status_t                           status = FBE_STATUS_OK;
    fbe_encl_info_t                      * pEnclInfo = NULL;
    fbe_encl_info_t                      * oldEnclInfo = NULL;
    fbe_u8_t                               deviceStr[FBE_DEVICE_STRING_LENGTH];

    fbe_base_object_trace((fbe_base_object_t*)pEnclMgmt,
            FBE_TRACE_LEVEL_INFO,
            FBE_TRACE_MESSAGE_ID_INFO,
            "Encl(%d_%d), %s, entry.\n",
            peerEnclInfo->location.bus, peerEnclInfo->location.enclosure, __FUNCTION__);

    status = fbe_encl_mgmt_get_encl_info_by_location(pEnclMgmt, &peerEnclInfo->location, &pEnclInfo);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "Encl(%d_%d) get_encl_info_by_location failed, status 0x%x.\n",
                              peerEnclInfo->location.bus,
                              peerEnclInfo->location.enclosure,
                              status);
        return status;
    }

    status = fbe_base_env_create_device_string(FBE_DEVICE_TYPE_ENCLOSURE,
                                               &pEnclInfo->location,
                                               &deviceStr[0],
                                               FBE_DEVICE_STRING_LENGTH);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s, Failed to create device string.\n",
                              __FUNCTION__);
    }

    status = fbe_event_log_write(ESP_ERROR_ENCL_BE_LOOP_MISCABLED,
                                 NULL,
                                 0,
                                 "%s",
                                 &deviceStr[0]);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s encl_mgmt_encl_log_event failed, status 0x%X.\n",
                              __FUNCTION__, status);
    }

    oldEnclInfo = (fbe_encl_info_t *)fbe_base_env_memory_ex_allocate((fbe_base_environment_t *)pEnclMgmt, sizeof(fbe_encl_info_t));
    if(oldEnclInfo == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t*)pEnclMgmt,
                                    FBE_TRACE_LEVEL_WARNING,
                                    FBE_TRACE_MESSAGE_ID_INFO,
                                    "Encl(%d_%d) encl_info memory allocation failed.\n",
                                    pEnclInfo->location.bus,
                                    pEnclInfo->location.enclosure);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* save old enclosure info */
    fbe_copy_memory(oldEnclInfo, pEnclInfo, sizeof(fbe_encl_info_t));

    fbe_spinlock_lock(&pEnclInfo->encl_info_lock);
    pEnclInfo->enclState = FBE_ESP_ENCL_STATE_FAILED;
    pEnclInfo->enclFaultSymptom = FBE_ESP_ENCL_FAULT_SYM_BE_LOOP_MISCABLED;
    fbe_spinlock_unlock(&pEnclInfo->encl_info_lock);

    fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt,
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "Encl(%d_%d), enclosure state fail, fault symptom:%s\n",
                          pEnclInfo->location.bus,
                          pEnclInfo->location.enclosure,
                          fbe_encl_mgmt_decode_encl_fault_symptom(pEnclInfo->enclFaultSymptom));

    /* Trace and Log event. */
    status = fbe_encl_mgmt_encl_log_event(pEnclMgmt,
                                        &pEnclInfo->location,
                                        pEnclInfo,
                                        oldEnclInfo);

    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s encl_mgmt_encl_log_event failed, status 0x%X.\n",
                              __FUNCTION__, status);
    }

    fbe_base_env_memory_ex_release((fbe_base_environment_t *)pEnclMgmt, oldEnclInfo);

    /* update enclosure fault led */
    status = fbe_encl_mgmt_update_encl_fault_led(pEnclMgmt,
                                                &pEnclInfo->location,
                                                FBE_ENCL_FAULT_LED_LCC_CABLING_FLT);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt,
                          FBE_TRACE_LEVEL_WARNING,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "Encl(%d_%d), CABLING_FLT, update_encl_fault_led failed, status: 0x%X.\n",
                          pEnclInfo->location.bus,
                          pEnclInfo->location.enclosure,
                          status);
    }

    fbe_base_environment_send_data_change_notification((fbe_base_environment_t*) pEnclMgmt,
                                                       FBE_CLASS_ID_ENCL_MGMT,
                                                       FBE_DEVICE_TYPE_ENCLOSURE,
                                                       FBE_DEVICE_DATA_TYPE_GENERIC_INFO,
                                                       &pEnclInfo->location);

    return status;
}

/*!**************************************************************
 * @fn fbe_encl_mgmt_process_peer_cache_status_update()
 ****************************************************************
 * @brief
 *  This function will handle the message we received from peer 
 *  to update peerCacheStatus.
 *
 * @param pEnclMgmt -
 * @param cmiMsgPtr - Pointer to encl_mgmt cmi packet
 * @param classId - encl_mgmt class id.
 *
 * @return fbe_status_t 
 *
 * @author
 *  20-Nov-2012:  Dipak Patel - Created. 
 *
 ****************************************************************/
static fbe_status_t fbe_encl_mgmt_process_peer_cache_status_update(fbe_encl_mgmt_t * pEnclMgmt, 
                                                                   fbe_encl_mgmt_cmi_packet_t * cmiMsgPtr)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_base_env_cacheStatus_cmi_packet_t *baseCmiPtr = (fbe_base_env_cacheStatus_cmi_packet_t *)cmiMsgPtr;
    fbe_common_cache_status_t prevCombineCacheStatus = FBE_CACHE_STATUS_OK;
    fbe_common_cache_status_t peerCacheStatus = FBE_CACHE_STATUS_OK; 

        fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt, 
                           FBE_TRACE_LEVEL_INFO,
                           FBE_TRACE_MESSAGE_ID_INFO,
                           "%s entry.\n",
                           __FUNCTION__); 

    status = fbe_base_environment_get_peerCacheStatus((fbe_base_environment_t *)pEnclMgmt,
                                                       &peerCacheStatus); 

    prevCombineCacheStatus = fbe_base_environment_combine_cacheStatus((fbe_base_environment_t *)pEnclMgmt,
                                                                       pEnclMgmt->enclCacheStatus,
                                                                       peerCacheStatus,
                                                                       FBE_CLASS_ID_ENCL_MGMT);

    /* Update Peer cache status for this side */
    status = fbe_base_environment_set_peerCacheStatus((fbe_base_environment_t *)pEnclMgmt,
                                                       baseCmiPtr->CacheStatus);

    /* Compare it with local cache status and send notification to cache if require.*/ 
    status = fbe_base_environment_compare_cacheStatus((fbe_base_environment_t *)pEnclMgmt,
                                                      prevCombineCacheStatus,
                                                      baseCmiPtr->CacheStatus,
                                                      FBE_CLASS_ID_ENCL_MGMT);

    /* Check if peer cache status is not initialized at peer side.
       If so, we need to send CMI message to peer to update it's
       peer cache status */
    if (baseCmiPtr->isPeerCacheStatusInitilized == FALSE)
    {
        /* Send CMI to peer.
           Over here, peerCacheStatus for this SP will intilized as we have update it above.
           So we are returning it with TRUE */

        fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt, 
                           FBE_TRACE_LEVEL_INFO,
                           FBE_TRACE_MESSAGE_ID_INFO,
                           "%s, peerCachestatus is uninitialized at peer\n",
                           __FUNCTION__);

        status = fbe_base_environment_send_cacheStatus_update_to_peer((fbe_base_environment_t *)pEnclMgmt, 
                                                                       pEnclMgmt->enclCacheStatus,
                                                                       TRUE);
    }

    return status;
}

fbe_status_t fbe_encl_mgmt_check_fru_limit(fbe_encl_mgmt_t *encl_mgmt, fbe_device_physical_location_t *location)
{
    fbe_encl_info_t * pEnclInfo = NULL;
    fbe_u32_t drive_slot;
    fbe_object_id_t drive_object_id;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_base_enclosure_fail_encl_cmd_t fail_cmd = {0};

    status = fbe_encl_mgmt_get_encl_info_by_location(encl_mgmt, location, &pEnclInfo);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)encl_mgmt,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "Encl(%d_%d) get_encl_info_by_location failed, status 0x%x.\n",
                              location->bus,
                              location->enclosure,
                              status);
        return status;
    }

    fbe_base_object_trace((fbe_base_object_t *)encl_mgmt, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "EnclMgmt: (%d_%d)checking enclosure against limit %d,total %d,encl slots %d.\n",
                              location->bus,
                              location->enclosure,
                              encl_mgmt->platformFruLimit,
                              encl_mgmt->total_drive_slot_count,
                              pEnclInfo->driveSlotCount);
    if( (encl_mgmt->total_drive_slot_count + pEnclInfo->driveSlotCount) > encl_mgmt->platformFruLimit)
    {
        char deviceStr[FBE_DEVICE_STRING_LENGTH] = {0};

        /* overlmit enclosure detected.  generate event logs for user.
        */
        for(drive_slot=0;drive_slot<pEnclInfo->driveSlotCount;drive_slot++)
        {
            fbe_api_get_physical_drive_object_id_by_location(pEnclInfo->location.bus,pEnclInfo->location.enclosure,
                                                             drive_slot, &drive_object_id);
            if(drive_object_id != FBE_OBJECT_ID_INVALID)
            {
               fbe_port_number_t port;
                fbe_enclosure_number_t enclosure;
                fbe_enclosure_slot_number_t slot;
                fbe_device_physical_location_t phys_location = {0};
                fbe_physical_drive_information_t drive_info;                

                /* attempt to get b_e_s and drive_info from PDO object ID. */
                fbe_api_get_object_port_number(drive_object_id, &port);
                fbe_api_get_object_enclosure_number(drive_object_id, &enclosure);
                fbe_api_get_object_drive_number(drive_object_id, &slot);

                phys_location.bus = (fbe_u8_t)port;
                phys_location.enclosure = (fbe_u8_t)enclosure;
                phys_location.slot = (fbe_u8_t)slot;

                fbe_api_physical_drive_get_drive_information(drive_object_id, &drive_info, FBE_PACKET_FLAG_NO_ATTRIB);

                fbe_base_env_create_device_string(FBE_DEVICE_TYPE_DRIVE,        
                                                  &phys_location, 
                                                  &deviceStr[0],
                                                  FBE_DEVICE_STRING_LENGTH);               

                fbe_event_log_write(ESP_ERROR_DRIVE_PLATFORM_LIMITS_EXCEEDED, NULL, 0, "%s %d %s %s %s %s",
                          &deviceStr[0], encl_mgmt->platformFruLimit, drive_info.product_serial_num, drive_info.tla_part_number, drive_info.product_rev, "");                             
            }
        }

         status = fbe_base_env_create_device_string(FBE_DEVICE_TYPE_ENCLOSURE,
                                                   &pEnclInfo->location,
                                                   &deviceStr[0],
                                                   FBE_DEVICE_STRING_LENGTH);
        if(status != FBE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t *)encl_mgmt, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s, Failed to create device string.\n", __FUNCTION__);
        }
                
        status = fbe_event_log_write(ESP_ERROR_ENCL_EXCEEDED_MAX, NULL, 0, "%s", &deviceStr[0]);

        if(status != FBE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t *)encl_mgmt, FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s encl_mgmt_encl_log_event failed, status 0x%X.\n",__FUNCTION__, status);
        }

        fail_cmd.led_reason = FBE_ENCL_FAULT_LED_EXCEEDED_MAX;
        fail_cmd.flt_symptom = FBE_ENCL_FLTSYMPT_EXCEEDS_MAX_LIMITS;
        fbe_api_enclosure_set_enclosure_failed(pEnclInfo->object_id, &fail_cmd);

        fbe_base_object_trace((fbe_base_object_t *)encl_mgmt, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "EnclMgmt: (%d_%d) fru_limit: enclosure exceeds platform limits. slot count %d>%d\n",
                              pEnclInfo->location.bus, pEnclInfo->location.enclosure, (encl_mgmt->total_drive_slot_count+pEnclInfo->driveSlotCount), encl_mgmt->platformFruLimit);

    }
    else
    {
        // enclosure does not exceed the platform limits add it to the count
        fbe_spinlock_lock(&pEnclInfo->encl_info_lock);
        encl_mgmt->total_drive_slot_count += pEnclInfo->driveSlotCount;
        fbe_spinlock_unlock(&pEnclInfo->encl_info_lock);
    }
    return FBE_STATUS_OK;

}

/*!**************************************************************
 * fbe_encl_mgmt_add_new_encl()
 ****************************************************************
 * @brief
 *  This function adds the new enclosure.
 *
 * @param pEnclMgmt - This is the object handle.
 * @param objectId - The enclsoure object for the new enclosure.
 * @param pLocation - The pointer to the location of the new enclosure.
 * @param pEnclInfoPtr(OUTPUT) - The pointer to the enclosure information
 *                               pointer for the new enclosure.
 *
 * @return fbe_status_t - FBE Status
 *
 * @author
 *  15-Feb-2013:  PHE - Created. 
 *
 ****************************************************************/
static fbe_status_t fbe_encl_mgmt_add_new_encl(fbe_encl_mgmt_t * pEnclMgmt,
                                        fbe_object_id_t objectId,
                                        fbe_device_physical_location_t * pLocation,
                                        fbe_encl_info_t ** pEnclInfoPtr)
{
    fbe_encl_info_t                           * pEnclInfo = NULL;
    fbe_status_t                                status = FBE_STATUS_OK;
    fbe_eir_sample_data_t                     * pEirSampleData = NULL;

    status = fbe_encl_mgmt_get_encl_info_by_object_id(pEnclMgmt, objectId, &pEnclInfo);
    
    if(status != FBE_STATUS_OK)
    {
        /* Find the enclosure info memory space for the new enclsoure. */
        status = fbe_encl_mgmt_find_available_encl_info(pEnclMgmt, &pEnclInfo);
    
        if (status != FBE_STATUS_OK) {
            fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt, 
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "Encl(%d_%d), Can not find available encl info, status:0x%x.\n",
                                  pLocation->bus, pLocation->enclosure, status);
    
            return status;
        }
    }

    /*
     * Allocate memory for EIR Samples
     */
    pEirSampleData = (fbe_eir_sample_data_t *)fbe_base_env_memory_ex_allocate((fbe_base_environment_t *)pEnclMgmt, sizeof(fbe_eir_sample_data_t));
    if(pEirSampleData == NULL) {
        fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s, eirSampleData Memory Allocation failed\n",
                              __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_spinlock_lock(&pEnclInfo->encl_info_lock);
    pEnclInfo->object_id = objectId; // For XPE, this is board object id. For DPE, this is encl0_0 object id.
    pEnclInfo->location = *pLocation;
    pEnclInfo->enclState = FBE_ESP_ENCL_STATE_OK;// Initialize to OK. The fault will be checked later.
    pEnclInfo->enclFaultSymptom = FBE_ESP_ENCL_FAULT_SYM_NO_FAULT;
    pEnclInfo->eirSampleData = pEirSampleData;

    pEnclMgmt->total_encl_count ++;
    fbe_spinlock_unlock(&pEnclInfo->encl_info_lock);

    fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt, 
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s, Encl(%d_%d) Added, totalEnclCount %d.\n",
                          __FUNCTION__,
                          pLocation->bus, 
                          pLocation->enclosure,
                          pEnclMgmt->total_encl_count);

    /* Trace and log event. */
    status = fbe_encl_mgmt_encl_log_event(pEnclMgmt, 
                                            pLocation, 
                                            pEnclInfo, 
                                            NULL);

    if(status != FBE_STATUS_OK) 
    {
        fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "Encl(%d_%d) %s encl_mgmt_encl_log_event failed, status 0x%X.\n",
                              pLocation->bus, 
                              pLocation->enclosure,
                              __FUNCTION__, 
                              status);
    }

    *pEnclInfoPtr = pEnclInfo;

    return FBE_STATUS_OK;
}


/*!*************************************************************************
 *  @fn fbe_encl_mgmt_suppress_lcc_fault_if_needed()
 **************************************************************************
 *  @brief
 *      This function checks whether the LCC is faulted.
 *      If so, check whether the LCC firmware upgrade is in activation.
 *      If yes, suppress the LCC fault.
 *
 *  @param fbe_encl_mgmt_t - The pointer to the fbe_encl_mgmt_t object.
 *  @param pLocation - The pointer to the LCC location.
 *  @param pLccInfo(OUTPUT) - The pointer to the LCC info.
 * 
 *  @return  fbe_status_t
 *
 *  @version
 *  01-Mar-2013 PHE - Created.
 *
 **************************************************************************/
static fbe_status_t fbe_encl_mgmt_suppress_lcc_fault_if_needed(fbe_encl_mgmt_t * pEnclMgmt,
                                           fbe_device_physical_location_t * pLocation,
                                           fbe_lcc_info_t * pLccInfo)
{
    fbe_bool_t                            activateInProgress = FBE_FALSE;
    fbe_status_t                          status = FBE_STATUS_OK;
    fbe_char_t                            logHeader[FBE_BASE_ENV_FUP_MAX_LOG_HEADER_SIZE + 1] = {0};

    if(pLccInfo->faulted)
    {
        /* Set log header for tracing purpose. */
        fbe_encl_mgmt_fup_generate_logheader(pLocation, &logHeader[0], FBE_BASE_ENV_FUP_MAX_LOG_HEADER_SIZE);

        // Suppress LCC Fault during FUP Activate
        status = fbe_encl_mgmt_fup_is_activate_in_progress(pEnclMgmt, 
                                                           FBE_DEVICE_TYPE_LCC,
                                                           pLocation,
                                                           &activateInProgress);

        if(status != FBE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt,
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s, activateInProgress check failed!!!\n",
                                  &logHeader[0]);

        }
        else if(activateInProgress) 
        {
            // Suppress LCC Faults during LCC FUP Activate
            pLccInfo->faulted = FBE_FALSE;
            fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt,
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s, LCC fault suppressed!!!\n",
                                  &logHeader[0]);
        }
    }

    return status;
}

/*!**************************************************************
 * @fn fbe_encl_mgmt_log_ssc_status_change(fbe_device_physical_location_t * pLocation,
 *                                         fbe_ssc_info_t *pNewInfo,
 *                                         fbe_ssc_info_t *pOldInfo)
 ****************************************************************
 * @brief
 *  Check for SSC status change and log events for them.
 *
 * @param pLocation - 
 * @param pNewInfo - 
 * @param pOldInfo -
 *
 * @return fbe_status_t - the status of the event log write, else OK
 * @author
 *   19-Dec-2013: GB - Created.
 ****************************************************************/
static fbe_status_t fbe_encl_mgmt_log_ssc_status_change(fbe_device_physical_location_t * pLocation,
                                                        fbe_ssc_info_t *pNewInfo,
                                                        fbe_ssc_info_t *pOldInfo)
{
    fbe_status_t                status = FBE_STATUS_OK;
    fbe_u8_t                    deviceStr[FBE_DEVICE_STRING_LENGTH];

    status = fbe_base_env_create_device_string(FBE_DEVICE_TYPE_SSC, 
                                               pLocation, 
                                               &deviceStr[0],
                                               FBE_DEVICE_STRING_LENGTH);

    if((pNewInfo->inserted) && (!pOldInfo->inserted))
    {
        /* SSC gets inserted */
        fbe_event_log_write(ESP_INFO_SSC_INSERTED,
                            NULL, 0,
                            "%s", 
                            &deviceStr[0]);
        
    }
    else if((!pNewInfo->inserted) && (pOldInfo->inserted))
    {
        /* SSC gets removed.
         * When the Viking SSC (system status card) is removed, CDES reports 
         * Fan 0 and Fan 7 as CRITICAL as they lose power at that time.
         */
        status = fbe_event_log_write(ESP_ERROR_SSC_REMOVED,
                                     NULL, 
                                     0,
                                     "%s", 
                                     &deviceStr[0]);
    }

    if((pNewInfo->faulted) && (!pOldInfo->faulted))
    {
        /* SSC becomes faulted. */
        status = fbe_event_log_write(ESP_ERROR_SSC_FAULTED,
                                     NULL, 
                                     0,
                                     "%s", 
                                     &deviceStr[0]);
    }
    else if((!pNewInfo->faulted) && (pOldInfo->faulted))
    {
        /* SSC fault is cleared. */
        status = fbe_event_log_write(ESP_INFO_SSC_FAULT_CLEARED,
                                     NULL, 
                                     0,
                                     "%s", 
                                     &deviceStr[0]);
    }

    return(status);
} //fbe_encl_mgmt_log_ssc_status_change


/*!**************************************************************
 * @fn fbe_encl_mgmt_process_ssc_status(fbe_encl_mgmt_t *pEnclMgmt
 *                                      fbe_object_id_t objectId,
 *                                      fbe_device_physical_location_t *pLocation
 ****************************************************************
 * @brief
 *  Get the old and the new SSC info and compare them for 
 *  any change.
 *
 * @param pLocation - enclosure management object
 * @param pNewInfo - object id for the enclosure
 * @param pOldInfo - location for the enclosure
 *
 * @return fbe_status_t
 * @author
 *   19-Dec-2013: GB - Created.
 ****************************************************************/
static fbe_status_t fbe_encl_mgmt_process_ssc_status(fbe_encl_mgmt_t *pEnclMgmt,
                                                     fbe_object_id_t objectId,
                                                     fbe_device_physical_location_t *pLocation)
{
    fbe_encl_info_t                    *pEnclInfo = NULL;
    fbe_ssc_info_t                     *pSscInfo = NULL;
    fbe_ssc_info_t                      oldSscInfo = {0};
    fbe_status_t                        status = FBE_STATUS_OK;

    if(objectId == FBE_OBJECT_ID_INVALID)
    {
        fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s, encl %d_%d, invalid objectId.\n",
                              __FUNCTION__, 
                              pLocation->bus, 
                              pLocation->enclosure);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_encl_mgmt_get_encl_info_by_object_id(pEnclMgmt, objectId, &pEnclInfo);

    if (status != FBE_STATUS_OK) 
    {
        // The enclosure was not initialized yet.
        // Just return FBE_STATUS_OK here.
        fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s, Encl %d_%d, objectId 0x%X is not populated, no action taken.\n",
                              __FUNCTION__, 
                              pLocation->bus, 
                              pLocation->enclosure,
                              objectId);

        return FBE_STATUS_OK;
    }

    pSscInfo = &pEnclInfo->sscInfo;

    // save the old info
    oldSscInfo = *pSscInfo;

    // get the new info
    status = fbe_api_enclosure_get_ssc_info(objectId, pSscInfo);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s Error getting SSC(%d_%d) info, status: 0x%X.\n",
                              __FUNCTION__,
                              pLocation->bus,
                              pLocation->enclosure,
                              status);
        return status;
    }

    status = fbe_encl_mgmt_log_ssc_status_change(pLocation, pSscInfo, &oldSscInfo);

    fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt, 
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "SSC(%d_%d) inserted:%d,faulted:%d\n",
                          pLocation->bus,
                          pLocation->enclosure,
                          pSscInfo->inserted, 
                          pSscInfo->faulted);

    status = fbe_encl_mgmt_resume_prom_handle_ssc_status_change(pEnclMgmt, 
                                                                pLocation, 
                                                                pSscInfo, 
                                                                &oldSscInfo);
    if(status != FBE_STATUS_OK) 
    {
        fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "RP: SSC(%d_%d_%d) resume_prom_handle_status_change failed, status 0x%X.\n",
                              pLocation->bus,
                              pLocation->enclosure,
                              pLocation->slot,
                              status);

    }


    /*
     * Update the enclosure fault led.
     */
    status = fbe_encl_mgmt_update_encl_fault_led(pEnclMgmt, pLocation, FBE_ENCL_FAULT_LED_SSC_FLT);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt, 
                          FBE_TRACE_LEVEL_WARNING,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "SSC(%d_%d_%d), update_encl_fault_led failed, status: 0x%X.\n",
                          pLocation->bus, pLocation->enclosure, pLocation->slot, status);
    }

    // SSC data change
    fbe_base_environment_send_data_change_notification((fbe_base_environment_t*)pEnclMgmt,
                                                           FBE_CLASS_ID_ENCL_MGMT, 
                                                           FBE_DEVICE_TYPE_SSC, 
                                                           FBE_DEVICE_DATA_TYPE_GENERIC_INFO,
                                                           pLocation);

    return status;
} //fbe_encl_mgmt_process_ssc_status

/*!**************************************************************
 * @fn fbe_encl_mgmt_validEnclPsTypes(fbe_encl_mgmt_t *pEnclMgmt
 *                                    fbe_encl_info_t *pEnclInfo)
 ****************************************************************
 * @brief
 *  Get the old and the new SSC info and compare them for 
 *  any change.
 *
 * @param pEnclMgmt - pointer to encl_mgmt object
 * @param pEnclInfo - pointer to a specific enclosure's info
 *
 * @return fbe_bool_t
 * @author
 *   28-Dec-2014: Joe Perry - Created.
 ****************************************************************/
static fbe_bool_t fbe_encl_mgmt_validEnclPsTypes(fbe_encl_mgmt_t *pEnclMgmt, fbe_encl_info_t *pEnclInfo)
{
    fbe_status_t                status;
    fbe_bool_t                  validEnclPsTypes = TRUE;
    fbe_u32_t                   psIndex;
    fbe_power_supply_info_t     pPsInfo;
    HW_MODULE_TYPE              firstPsFfid = NO_MODULE;

    // verify there is not a mix of PS types
    switch (pEnclInfo->enclType)
    {
    case FBE_ESP_ENCL_TYPE_NAGA:
        for (psIndex = 0; psIndex < pEnclInfo->psCount; psIndex++)
        {
            fbe_zero_memory(&pPsInfo, sizeof(fbe_power_supply_info_t));
            pPsInfo.slotNumOnEncl = psIndex;

            status = fbe_api_power_supply_get_power_supply_info(pEnclInfo->object_id, &pPsInfo);
            if (status != FBE_STATUS_OK) 
            {
                fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt, 
                                      FBE_TRACE_LEVEL_WARNING,
                                      FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                      "%s, Error getting PS info %d_%d_%d, status: 0x%X.\n",
                                      __FUNCTION__,
                                      pEnclInfo->location.bus, pEnclInfo->location.enclosure, psIndex, status);
                continue;
            }
            if (!pPsInfo.bInserted)
            {
                continue;
            }
            else if ((firstPsFfid == NO_MODULE) && (pPsInfo.uniqueId != NO_MODULE))
            {
                firstPsFfid = pPsInfo.uniqueId;
            }
            else if ((firstPsFfid != NO_MODULE) && (firstPsFfid != pPsInfo.uniqueId))
            {
                validEnclPsTypes = FALSE;
                fbe_base_object_trace((fbe_base_object_t *)pEnclMgmt, 
                                      FBE_TRACE_LEVEL_WARNING,
                                      FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                      "%s, mismatched PS types in %d_%d, FFID's 0x%x, 0x%x\n",
                                      __FUNCTION__, 
                                      pEnclInfo->location.bus, pEnclInfo->location.enclosure,
                                      firstPsFfid, pPsInfo.uniqueId);
                break;
            }
        }
        break; 
    default:
        break;
    }

    return validEnclPsTypes;

}   // end of fbe_encl_mgmt_validEnclPsTypes

/* End of file fbe_encl_mgmt_monitor.c */
