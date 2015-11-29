/***************************************************************************
 * Copyright (C) EMC Corporation 2010 
 * All rights reserved. Licensed material -- property of EMC 
 * Corporation 
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_module_mgmt_monitor.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the Module Management object lifecycle
 *  code.
 * 
 *  This includes the
 *  @ref fbe_module_mgmt_monitor_entry "module_mgmt monitor entry
 *  point", as well as all the lifecycle defines such as
 *  rotaries and conditions, along with the actual condition
 *  functions.
 * 
 * @ingroup module_mgmt_class_files
 * 
 * @version
 *   11-Mar-2010 - Created. Nayana Chaudhari
 *
 ***************************************************************************/
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_memory.h"
#include "fbe_topology.h"
#include "fbe_base_object.h"
#include "fbe_scheduler.h"
#include "fbe_module_mgmt_private.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_api_resume_prom_interface.h"
#include "fbe_transport_memory.h"
#include "fbe/fbe_api_board_interface.h"
#include "fbe/fbe_api_port_interface.h"
#include "fbe/fbe_api_enclosure_interface.h"
#include "fbe/fbe_registry.h"
#include "fbe/fbe_event_log_utils.h"
#include "fbe/fbe_event_log_api.h"
#include "fbe_base_environment_debug.h"
#include "fbe/fbe_esp_module_mgmt.h"
#include "fbe/fbe_api_esp_module_mgmt_interface.h"
#include "fbe_notification.h"
#include "generic_utils_lib.h"


static fbe_status_t fbe_module_mgmt_get_new_module_info(fbe_module_mgmt_t *module_mgmt,
                                                           SP_ID    sp,
                                                           fbe_u8_t slot,
                                                           fbe_u64_t device_type);
static fbe_status_t fbe_module_mgmt_get_new_io_port_info(fbe_module_mgmt_t *module_mgmt, SP_ID sp, fbe_u8_t slot, 
                                                         fbe_u8_t port, fbe_u64_t device_type);
static fbe_status_t fbe_module_mgmt_get_new_sfp_info(fbe_module_mgmt_t *module_mgmt, SP_ID sp, fbe_u8_t iom_num, 
                                                     fbe_u8_t port_num, fbe_object_id_t port_object_id);
static fbe_status_t fbe_module_mgmt_get_new_port_link_info(fbe_module_mgmt_t *module_mgmt, SP_ID sp, fbe_u8_t iom_num, 
                                                           fbe_u8_t port_num, fbe_object_id_t port_object_id);
static fbe_object_id_t fbe_module_mgmt_get_port_device_object_id(fbe_module_mgmt_t *module_mgmt, fbe_u8_t port_num );
static fbe_status_t fbe_module_mgmt_get_iom_num_port_num_from_port_device_object_id(fbe_module_mgmt_t *module_mgmt, 
                                                                                    fbe_object_id_t port_device_object, 
                                                                                    fbe_u8_t *iom_num, fbe_u8_t *port_num);
static fbe_status_t fbe_module_mgmt_get_new_mgmt_module_info(fbe_module_mgmt_t *module_mgmt, SP_ID sp, fbe_u8_t slot);
static fbe_bool_t fbe_module_mgmt_get_mgmt_module_logical_fault(fbe_board_mgmt_mgmt_module_info_t *mgmt_module_comp_info);
static fbe_status_t fbe_module_mgmt_processReceivedCmiMessage(fbe_module_mgmt_t *module_mgmt, fbe_module_mgmt_cmi_msg_t *cmiMsgPtr);
static fbe_status_t fbe_module_mgmt_processPeerNotPresent(fbe_module_mgmt_t * pModuleMgmt, 
                                    fbe_module_mgmt_cmi_msg_t * pModuleCmiPkt);
static fbe_status_t fbe_module_mgmt_processContactLost(fbe_module_mgmt_t * pModuleMgmt);
static fbe_status_t fbe_module_mgmt_processPeerBusy(fbe_module_mgmt_t * pModuleMgmt, 
                              fbe_module_mgmt_cmi_msg_t * pModuleCmiPkt);
static fbe_status_t fbe_module_mgmt_processFatalError(fbe_module_mgmt_t * pModuleMgmt, 
                                fbe_module_mgmt_cmi_msg_t * pModuleCmiPkt);
static fbe_status_t fbe_module_mgmt_handle_sfp_state_change(fbe_module_mgmt_t *module_mgmt,
                                                    fbe_u32_t iom_num,
                                                    fbe_u32_t port_num, 
                                                    fbe_port_sfp_info_t *old_sfp_info, 
                                                    fbe_port_sfp_info_t *new_sfp_info);
static void fbe_module_mgmt_check_for_combined_port(fbe_module_mgmt_t *module_mgmt,
                                                    fbe_u32_t iom_num,
                                                    fbe_u32_t port_num);
static void fbe_module_mgmt_check_for_configured_combined_port(fbe_module_mgmt_t *module_mgmt);

static fbe_status_t fbe_module_mgmt_get_bm_lcc_info(fbe_module_mgmt_t *module_mgmt,
                                 SP_ID sp,
                                 fbe_lcc_info_t * pLccInfo);

static fbe_bool_t fbe_module_mgmt_is_mgmt_port_speed_valid(fbe_mgmt_port_speed_t speed);
static fbe_bool_t fbe_module_mgmt_is_mgmt_port_duplex_valid(fbe_mgmt_port_duplex_mode_t duplex);
static fbe_bool_t fbe_module_mgmt_is_mgmt_port_auto_negotiate_valid(fbe_mgmt_port_auto_neg_t auto_negotiate);
static fbe_status_t fbe_module_mgmt_handle_connector_status_change(fbe_module_mgmt_t *module_mgmt, fbe_device_physical_location_t phys_location);

fbe_u32_t global_max_io_modules_per_blade; /* SAFERELOCATED */

/*!***************************************************************
 * fbe_module_mgmt_monitor_entry()
 ****************************************************************
 * @brief
 *  Entry routine for the Modules Management monitor.
 *
 * @param object_handle - This is the object handle, or in our
 * case the module_mgmt.
 * @param packet_p - The packet arriving from the monitor
 * scheduler.
 *
 * @return fbe_status_t - Status of monitor operation.             
 *
 * @author
 *  11-Mar-2010 - Created. Nayana Chaudhari
 *
 ****************************************************************/
fbe_status_t 
fbe_module_mgmt_monitor_entry(fbe_object_handle_t object_handle, fbe_packet_t * packet)
{
	fbe_module_mgmt_t * module_mgmt = NULL;

	module_mgmt = (fbe_module_mgmt_t *)fbe_base_handle_to_pointer(object_handle);

	fbe_topology_class_trace(FBE_CLASS_ID_MODULE_MGMT,
                FBE_TRACE_LEVEL_DEBUG_LOW,
			     FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
			     "%s entry class %d\n", __FUNCTION__, FBE_CLASS_ID_MODULE_MGMT);

	return fbe_lifecycle_crank_object(&FBE_LIFECYCLE_CONST_DATA(module_mgmt),
	                                  (fbe_base_object_t*)module_mgmt, packet);
}
/******************************************
 * end fbe_module_mgmt_monitor_entry()
 ******************************************/

/*!**************************************************************
 * fbe_module_mgmt_monitor_load_verify()
 ****************************************************************
 * @brief
 *  This function simply validates the constant lifecycle data
 *  that is associated with the module management.
 *
 * @param  - None.               
 *
 * @return fbe_status_t - The status of the call to
 *                        fbe_lifecycle_class_const_verify()
 *                        for the module management's
 *                        constant lifecycle data.
 *
 * @author
 *  11-Mar-2010 - Created. Nayana Chaudhari
 *
 ****************************************************************/
fbe_status_t fbe_module_mgmt_monitor_load_verify(void)
{
	return fbe_lifecycle_class_const_verify(&FBE_LIFECYCLE_CONST_DATA(module_mgmt));
}
/******************************************
 * end fbe_module_mgmt_monitor_load_verify()
 ******************************************/

/*--- local function prototypes --------------------------------------------------------*/

static fbe_lifecycle_status_t fbe_module_mgmt_register_state_change_callback_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet);
static fbe_lifecycle_status_t fbe_module_mgmt_register_cmi_callback_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet);
static fbe_lifecycle_status_t fbe_module_mgmt_fup_register_callback_cond_function(fbe_base_object_t * base_object,  fbe_packet_t * packet);
static fbe_lifecycle_status_t fbe_module_mgmt_register_resume_prom_callback_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet);
static fbe_lifecycle_status_t fbe_module_mgmt_discover_module_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet);
static fbe_lifecycle_status_t fbe_module_mgmt_configure_ports_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet);
static fbe_lifecycle_status_t fbe_module_mgmt_conversion_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet);
static fbe_lifecycle_status_t fbe_module_mgmt_slic_upgrade_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet);
static fbe_status_t fbe_module_mgmt_handle_new_module(fbe_base_object_t * base_object, fbe_packet_t * packet, fbe_object_id_t object_id);
static fbe_status_t	fbe_module_mgmt_state_change_handler(update_object_msg_t *notification_info, void *context);
static fbe_status_t	fbe_module_mgmt_cmi_message_handler(fbe_base_object_t *base_object, fbe_base_environment_cmi_message_info_t *cmi_message);
static fbe_lifecycle_status_t fbe_module_mgmt_read_persistent_data_complete_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet);
static fbe_lifecycle_status_t fbe_module_mgmt_check_registry_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet);
static fbe_lifecycle_status_t fbe_module_mgmt_set_mgmt_port_speed_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet);
static fbe_lifecycle_status_t fbe_module_mgmt_init_mgmt_port_config_request_cond_function(fbe_base_object_t * base_object, 
                                                             fbe_packet_t * packet);

static fbe_lifecycle_status_t fbe_module_mgmt_config_vlan_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet);
void fbe_module_mgmt_config_vlan_completion(fbe_board_mgmt_set_set_mgmt_vlan_mode_async_context_t *command_context);
void fbe_module_mgmt_set_mgmt_port_speed_completion(fbe_board_mgmt_set_mgmt_port_async_context_t  *command_context);
fbe_status_t fbe_module_mgmt_check_usurper_queue(fbe_module_mgmt_t * module_mgmt,
                                                fbe_esp_module_mgmt_control_code_t required_control_code,
                                                 fbe_status_t packet_status);
void fbe_module_mgmt_log_mgmt_module_event(fbe_module_mgmt_t * module_mgmt,
                                           fbe_board_mgmt_mgmt_module_info_t *new_mgmt_module_comp_info,
                                           fbe_board_mgmt_mgmt_module_info_t *old_mgmt_module_comp_info);
fbe_status_t fbe_module_mgmt_log_io_module_event(fbe_module_mgmt_t * module_mgmt,
                                            fbe_u64_t deviceType,
                                            fbe_device_physical_location_t *pLocation,
                                            fbe_board_mgmt_io_comp_info_t *new_io_comp_info,
                                            fbe_board_mgmt_io_comp_info_t *old_io_comp_info);

/*--- lifecycle callback functions -----------------------------------------------------*/


FBE_LIFECYCLE_DEF_CALLBACK_GET_INST_DATA(module_mgmt);
FBE_LIFECYCLE_DEF_CALLBACK_GET_INST_COND(module_mgmt);

/*  module_mgmt_lifecycle_callbacks This variable has all the
 *  callbacks the lifecycle needs.
 */
static fbe_lifecycle_const_callback_t FBE_LIFECYCLE_CALLBACKS(module_mgmt) = {
	FBE_LIFECYCLE_DEF_CONST_CALLBACKS(
		module_mgmt,
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
static fbe_lifecycle_const_cond_t fbe_module_mgmt_register_state_change_callback_cond = {
	FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
		FBE_BASE_ENV_LIFECYCLE_COND_REGISTER_STATE_CHANGE_CALLBACK,
		fbe_module_mgmt_register_state_change_callback_cond_function)
};

/*--- constant derived condition entries -----------------------------------------------*/
/* FBE_BASE_ENV_LIFECYCLE_COND_READ_PERSISTENT_DATA_COMPLETE condition
 *  - The purpose of this derived condition is to handle the completion
 *    of reading the data from persistent storage.  
 */
static fbe_lifecycle_const_cond_t fbe_module_mgmt_read_persistent_data_complete_cond = {
	FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
		FBE_BASE_ENV_LIFECYCLE_COND_READ_PERSISTENT_DATA_COMPLETE,
		fbe_module_mgmt_read_persistent_data_complete_cond_function)
};

/* FBE_BASE_ENV_LIFECYCLE_COND_REGISTER_CMI_CALLBACK condition
 *  - The purpose of this derived condition is to register
 *  notification with the base environment object which inturns
 *  registers with the CMI. The leaf class needs to implement
 *  the actual condition handler function providing the callback
 *  that needs to be called
 */
static fbe_lifecycle_const_cond_t fbe_module_mgmt_register_cmi_callback_cond = {
	FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
		FBE_BASE_ENV_LIFECYCLE_COND_REGISTER_CMI_CALLBACK,
        fbe_module_mgmt_register_cmi_callback_cond_function)
};

/* FBE_BASE_ENV_LIFECYCLE_COND_FUP_REGISTER_CALLBACK condition
 *  - The purpose of this derived condition is to register
 *  call back functions with the base environment object. 
 * The leaf class needs to implement the actual call back functions.
 */
static fbe_lifecycle_const_cond_t fbe_module_mgmt_fup_register_callback_cond = {
	FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
		FBE_BASE_ENV_LIFECYCLE_COND_FUP_REGISTER_CALLBACK,
        fbe_module_mgmt_fup_register_callback_cond_function)
};

static fbe_lifecycle_const_cond_t fbe_module_mgmt_fup_abort_upgrade_cond = {
	FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
		FBE_BASE_ENV_LIFECYCLE_COND_FUP_ABORT_UPGRADE,
        fbe_base_env_fup_abort_upgrade_cond_function)
};

static fbe_lifecycle_const_cond_t fbe_module_mgmt_fup_wait_before_upgrade_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_ENV_LIFECYCLE_COND_FUP_WAIT_BEFORE_UPGRADE,
        fbe_base_env_fup_wait_before_upgrade_cond_function)
};

static fbe_lifecycle_const_cond_t fbe_module_mgmt_fup_wait_for_inter_device_delay_cond = {
	FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
		FBE_BASE_ENV_LIFECYCLE_COND_FUP_WAIT_FOR_INTER_DEVICE_DELAY,
        fbe_base_env_fup_wait_for_inter_device_delay_cond_function)
};

static fbe_lifecycle_const_cond_t fbe_module_mgmt_fup_read_image_header_cond = {
	FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
		FBE_BASE_ENV_LIFECYCLE_COND_FUP_READ_IMAGE_HEADER,
        fbe_base_env_fup_read_image_header_cond_function)
};

static fbe_lifecycle_const_cond_t fbe_module_mgmt_fup_check_rev_cond = {
	FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
		FBE_BASE_ENV_LIFECYCLE_COND_FUP_CHECK_REV,
        fbe_base_env_fup_check_rev_cond_function)
};

static fbe_lifecycle_const_cond_t fbe_module_mgmt_fup_read_entire_image_cond = {
	FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
		FBE_BASE_ENV_LIFECYCLE_COND_FUP_READ_ENTIRE_IMAGE,
        fbe_base_env_fup_read_entire_image_cond_function)
};

static fbe_lifecycle_const_cond_t fbe_module_mgmt_fup_download_image_cond = {
	FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
		FBE_BASE_ENV_LIFECYCLE_COND_FUP_DOWNLOAD_IMAGE,
        fbe_base_env_fup_download_image_cond_function)
};

static fbe_lifecycle_const_cond_t fbe_module_mgmt_fup_get_download_status_cond = {
	FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
		FBE_BASE_ENV_LIFECYCLE_COND_FUP_GET_DOWNLOAD_STATUS,
        fbe_base_env_fup_get_download_status_cond_function)
};

static fbe_lifecycle_const_cond_t fbe_module_mgmt_fup_get_peer_permission_cond = {
	FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
		FBE_BASE_ENV_LIFECYCLE_COND_FUP_GET_PEER_PERMISSION,
        fbe_base_env_fup_get_peer_permission_cond_function)
};

static fbe_lifecycle_const_cond_t fbe_module_mgmt_fup_check_env_status_cond = {
	FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
		FBE_BASE_ENV_LIFECYCLE_COND_FUP_CHECK_ENV_STATUS,
        fbe_base_env_fup_check_env_status_cond_function)
};

static fbe_lifecycle_const_cond_t fbe_module_mgmt_fup_activate_image_cond = {
	FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
		FBE_BASE_ENV_LIFECYCLE_COND_FUP_ACTIVATE_IMAGE,
        fbe_base_env_fup_activate_image_cond_function)
};

static fbe_lifecycle_const_cond_t fbe_module_mgmt_fup_get_activate_status_cond = {
	FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
		FBE_BASE_ENV_LIFECYCLE_COND_FUP_GET_ACTIVATE_STATUS,
        fbe_base_env_fup_get_activate_status_cond_function)
};

static fbe_lifecycle_const_cond_t fbe_module_mgmt_fup_check_result_cond = {
	FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
		FBE_BASE_ENV_LIFECYCLE_COND_FUP_CHECK_RESULT,
        fbe_base_env_fup_check_result_cond_function)
};

static fbe_lifecycle_const_cond_t fbe_module_mgmt_fup_refresh_device_status_cond = {
	FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
		FBE_BASE_ENV_LIFECYCLE_COND_FUP_REFRESH_DEVICE_STATUS,
        fbe_base_env_fup_refresh_device_status_cond_function)
};

static fbe_lifecycle_const_cond_t fbe_module_mgmt_fup_end_upgrade_cond = {
	FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
		FBE_BASE_ENV_LIFECYCLE_COND_FUP_END_UPGRADE,
        fbe_base_env_fup_end_upgrade_cond_function)
};

static fbe_lifecycle_const_cond_t fbe_module_mgmt_fup_release_image_cond = {
	FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
		FBE_BASE_ENV_LIFECYCLE_COND_FUP_RELEASE_IMAGE,
        fbe_base_env_fup_release_image_cond_function)
};

/* FBE_BASE_ENV_LIFECYCLE_COND_RESUME_PROM_REGISTER_CALLBACK condition
 *  - The purpose of this derived condition is to register
 *  notification with the base environment object which inturns
 *  registers with the CMI. The leaf class needs to implement
 *  the actual condition handler function providing the callback
 *  that needs to be called
 */
static fbe_lifecycle_const_cond_t fbe_module_mgmt_register_resume_prom_callback_cond = {
	FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
		FBE_BASE_ENV_LIFECYCLE_COND_RESUME_PROM_REGISTER_CALLBACK,
        fbe_module_mgmt_register_resume_prom_callback_cond_function)
};

/*--- constant base condition entries --------------------------------------------------*/

/* FBE_MODULE_MGMT_DISCOVER_MODULE condition -
*   The purpose of this condition is start the discovery process
*   of a new module that was added
 */
static fbe_lifecycle_const_base_cond_t 
    fbe_module_mgmt_discover_module_cond = {
        FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "fbe_module_mgmt_discover_module_cond",
        FBE_MODULE_MGMT_DISCOVER_MODULE,
        fbe_module_mgmt_discover_module_cond_function),
	FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
		FBE_LIFECYCLE_STATE_SPECIALIZE,     /* SPECIALIZE */
		FBE_LIFECYCLE_STATE_ACTIVATE,         /* ACTIVATE */
		FBE_LIFECYCLE_STATE_READY,         /* READY */
		FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
		FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
		FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
		FBE_LIFECYCLE_STATE_DESTROY)         /* DESTROY */
};


/*--- constant base condition entries --------------------------------------------------*/

/* FBE_MODULE_MGMT_CONFIGURE_PORTS condition -
 * The purpose of this condition is to generate a persistent
 * configuration of ports based on the current hardware.
 */
static fbe_lifecycle_const_base_cond_t 
    fbe_module_mgmt_configure_ports_cond = {
        FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "fbe_module_mgmt_configure_ports_cond",
        FBE_MODULE_MGMT_CONFIGURE_PORTS,
        fbe_module_mgmt_configure_ports_cond_function),
	FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
		FBE_LIFECYCLE_STATE_SPECIALIZE,     /* SPECIALIZE */
		FBE_LIFECYCLE_STATE_ACTIVATE,         /* ACTIVATE */
		FBE_LIFECYCLE_STATE_READY,         /* READY */
		FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
		FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
		FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
		FBE_LIFECYCLE_STATE_DESTROY)         /* DESTROY */
};


/* FBE_MODULE_MGMT_LIFECYCLE_COND_CONVERSION condition -
 * The purpose of this condition is to move existing port entries to
 * compatible new locations in a new array type.  
 */
static fbe_lifecycle_const_base_cond_t 
    fbe_module_mgmt_conversion_cond = {
        FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "fbe_module_mgmt_conversion_cond",
        FBE_MODULE_MGMT_LIFECYCLE_COND_IN_FAMILY_PORT_CONVERSION,
        fbe_module_mgmt_conversion_cond_function),
	FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
		FBE_LIFECYCLE_STATE_SPECIALIZE,     /* SPECIALIZE */
		FBE_LIFECYCLE_STATE_ACTIVATE,         /* ACTIVATE */
		FBE_LIFECYCLE_STATE_READY,         /* READY */
		FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
		FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
		FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
		FBE_LIFECYCLE_STATE_DESTROY)         /* DESTROY */
};


/* FBE_MODULE_MGMT_SLIC_UPGRADE condition -
 * The purpose of this condition is to convert existing persistent entries to a
 * compatible upgraded configuration if the upgraded hardware is present.
 */
static fbe_lifecycle_const_base_cond_t 
    fbe_module_mgmt_slic_upgrade_cond = {
        FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "fbe_module_mgmt_slic_upgrade_cond",
        FBE_MODULE_MGMT_LIFECYCLE_COND_SLIC_UPGRADE,
        fbe_module_mgmt_slic_upgrade_cond_function),
	FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
		FBE_LIFECYCLE_STATE_SPECIALIZE,     /* SPECIALIZE */
		FBE_LIFECYCLE_STATE_ACTIVATE,         /* ACTIVATE */
		FBE_LIFECYCLE_STATE_READY,         /* READY */
		FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
		FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
		FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
		FBE_LIFECYCLE_STATE_DESTROY)         /* DESTROY */
};

/* FBE_MODULE_MGMT_LIFECYCLE_COND_CHECK_REGISTRY condition -
 * The purpose of this condition is to update the registry 
 * parameters with any changes to the hardware or configuration.
 */
static fbe_lifecycle_const_base_cond_t 
    fbe_module_mgmt_check_registry_cond = {
        FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "fbe_module_mgmt_check_registry_cond",
        FBE_MODULE_MGMT_LIFECYCLE_COND_CHECK_REGISTRY,
        fbe_module_mgmt_check_registry_cond_function),
	FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
		FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
		FBE_LIFECYCLE_STATE_ACTIVATE,       /* ACTIVATE */
		FBE_LIFECYCLE_STATE_READY,         /* READY */
		FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
		FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
		FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
		FBE_LIFECYCLE_STATE_DESTROY)         /* DESTROY */
};

/* FBE_MODULE_MGMT_LIFECYCLE_COND_CONFIG_VLAN -
 * The purpose of this coniditon is to set the vLan config 
 * mode to CLARiiON_VLAN_MODE.
 */
static fbe_lifecycle_const_base_cond_t 
    fbe_module_mgmt_config_vlan_cond = {
        FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "fbe_module_mgmt_config_vlan_cond",
        FBE_MODULE_MGMT_LIFECYCLE_COND_CONFIG_VLAN,
        fbe_module_mgmt_config_vlan_cond_function),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,         /* ACTIVATE */
        FBE_LIFECYCLE_STATE_READY,          /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)         /* DESTROY */
};

/* FBE_MODULE_MGMT_LIFECYCLE_COND_SET_MGMT_PORT_SPEED -
 * The condition is set from module_mgmt usuper to change the
 * mgmt port speed.
 */
static fbe_lifecycle_const_base_cond_t 
    fbe_module_mgmt_set_mgmt_port_speed_cond = {
        FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "fbe_module_mgmt_set_mgmt_port_speed_cond",
        FBE_MODULE_MGMT_LIFECYCLE_COND_SET_MGMT_PORT_SPEED,
        fbe_module_mgmt_set_mgmt_port_speed_cond_function),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,         /* ACTIVATE */
        FBE_LIFECYCLE_STATE_READY,          /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)         /* DESTROY */
};

/* FBE_MODULE_MGMT_LIFECYCLE_COND_INIT_MGMT_PORT_CONFIG_REQUEST condition -
 * The purpose of this condition is to init the management port config request 
 * based on the persistence data.
 */
static fbe_lifecycle_const_base_cond_t 
    fbe_module_mgmt_init_mgmt_port_config_request_cond = {
        FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "fbe_module_mgmt_init_mgmt_port_config_request_cond",
        FBE_MODULE_MGMT_LIFECYCLE_COND_INIT_MGMT_PORT_CONFIG_REQUEST,
        fbe_module_mgmt_init_mgmt_port_config_request_cond_function),
	FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
		FBE_LIFECYCLE_STATE_SPECIALIZE,     /* SPECIALIZE */
		FBE_LIFECYCLE_STATE_ACTIVATE,         /* ACTIVATE */
		FBE_LIFECYCLE_STATE_READY,         /* READY */
		FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
		FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
		FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
		FBE_LIFECYCLE_STATE_DESTROY)         /* DESTROY */
};


static fbe_lifecycle_const_base_cond_t * FBE_LIFECYCLE_BASE_COND_ARRAY(module_mgmt)[] = {
    &fbe_module_mgmt_discover_module_cond,
	&fbe_module_mgmt_configure_ports_cond,
    &fbe_module_mgmt_conversion_cond,
    &fbe_module_mgmt_slic_upgrade_cond,
    &fbe_module_mgmt_check_registry_cond,
    &fbe_module_mgmt_config_vlan_cond,
    &fbe_module_mgmt_set_mgmt_port_speed_cond,
    &fbe_module_mgmt_init_mgmt_port_config_request_cond,
};

FBE_LIFECYCLE_DEF_CONST_BASE_CONDITIONS(module_mgmt);

/*--- constant rotaries ----------------------------------------------------------------*/

static fbe_lifecycle_const_rotary_cond_t fbe_module_mgmt_specialize_rotary[] = {
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_module_mgmt_register_state_change_callback_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
	FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_module_mgmt_register_cmi_callback_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_module_mgmt_fup_register_callback_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_module_mgmt_register_resume_prom_callback_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
};

static fbe_lifecycle_const_rotary_cond_t fbe_module_mgmt_activate_rotary[] = {
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_module_mgmt_read_persistent_data_complete_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_module_mgmt_discover_module_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_module_mgmt_configure_ports_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_module_mgmt_conversion_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_module_mgmt_slic_upgrade_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_module_mgmt_init_mgmt_port_config_request_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_module_mgmt_check_registry_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
};

static fbe_lifecycle_const_rotary_cond_t fbe_module_mgmt_ready_rotary[] = {
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_module_mgmt_fup_wait_before_upgrade_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_module_mgmt_read_persistent_data_complete_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_module_mgmt_check_registry_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_module_mgmt_config_vlan_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_module_mgmt_set_mgmt_port_speed_cond,  FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_module_mgmt_fup_release_image_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),

    /* Moved the following two conditions up. (AR651486)
     * so that work item can be removed from the queue immediately after the device gets removed.
     */
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_module_mgmt_fup_refresh_device_status_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_module_mgmt_fup_end_upgrade_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),

    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_module_mgmt_fup_wait_for_inter_device_delay_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_module_mgmt_fup_read_image_header_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),  
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_module_mgmt_fup_check_rev_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_module_mgmt_fup_read_entire_image_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_module_mgmt_fup_get_peer_permission_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_module_mgmt_fup_check_env_status_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_module_mgmt_fup_download_image_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_module_mgmt_fup_get_download_status_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_module_mgmt_fup_activate_image_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_module_mgmt_fup_get_activate_status_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_module_mgmt_fup_check_result_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    
    /* Need to put the abort condition in the end of all the fup conditions. 
     * We want to complete the execution of other condition which was already set before running the abort upgrade 
     * condition. Otherwise, when resuming the upgrade, the condition which was set before would get to run and 
     * it would make the upgrade out of sequence.
     */
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_module_mgmt_fup_abort_upgrade_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
};

static fbe_lifecycle_const_rotary_t FBE_LIFECYCLE_ROTARY_ARRAY(module_mgmt)[] = {
    FBE_LIFECYCLE_DEF_CONST_ROTARY(FBE_LIFECYCLE_STATE_SPECIALIZE, fbe_module_mgmt_specialize_rotary),
    FBE_LIFECYCLE_DEF_CONST_ROTARY(FBE_LIFECYCLE_STATE_ACTIVATE, fbe_module_mgmt_activate_rotary),
    FBE_LIFECYCLE_DEF_CONST_ROTARY(FBE_LIFECYCLE_STATE_READY, fbe_module_mgmt_ready_rotary),
};

FBE_LIFECYCLE_DEF_CONST_ROTARIES(module_mgmt);

/*--- global base board lifecycle constant data ----------------------------------------*/

fbe_lifecycle_const_t FBE_LIFECYCLE_CONST_DATA(module_mgmt) = {
	FBE_LIFECYCLE_DEF_CONST_DATA(
		module_mgmt,
		FBE_CLASS_ID_MODULE_MGMT,                  /* This class */
		FBE_LIFECYCLE_CONST_DATA(base_environment))    /* The super class */
};

/*--- Condition Functions --------------------------------------------------------------*/

/*!**************************************************************
 * fbe_module_mgmt_register_state_change_callback_cond_function()
 ****************************************************************
 * @brief
 *  This function registers with the physical package to get
 *  notified on module object life cycle state changes
 * 
 * @param object_handle - This is the object handle, or in our
 * case the module_mgmt.
 * @param packet_p - The packet arriving from the monitor
 * scheduler.
 * 
 * @return fbe_status_t - The status of the call to
 *                        fbe_lifecycle_class_const_verify() for
 *                        the module management's constant
 *                        lifecycle data.
 *
 * @author
 *  11-Mar-2010 - Created. Nayana Chaudhari
 *
 ****************************************************************/
static fbe_lifecycle_status_t
fbe_module_mgmt_register_state_change_callback_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet)
{
    fbe_status_t status;
    HW_ENCLOSURE_TYPE  processorEnclType = INVALID_ENCLOSURE_TYPE;

	status = fbe_lifecycle_clear_current_cond(base_object);
	if (status != FBE_STATUS_OK) {
		fbe_base_object_trace(base_object, 
								FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"%s can't clear current condition, status: 0x%X",
								__FUNCTION__, status);
	}

    status = fbe_base_env_get_processor_encl_type((fbe_base_environment_t *)base_object,
                                                   &processorEnclType);

    if((status == FBE_STATUS_OK) &&
        (processorEnclType == DPE_ENCLOSURE_TYPE))
    {
        /* 
         * Register for notifications from the board and port objects regarding the device types that we
         * are concerned with for module management.  SFP information comes from the port object, everything
         * else comes from the board object.  For DPE based systems register for connector status updates so
         * we can update SAS port 0 link status.
         */

        status = fbe_base_environment_register_event_notification((fbe_base_environment_t *) base_object,
                                                                  (fbe_api_notification_callback_function_t)fbe_module_mgmt_state_change_handler,
                                                                  FBE_NOTIFICATION_TYPE_OBJECT_DATA_CHANGED,
                                                                  (FBE_DEVICE_TYPE_IOMODULE| FBE_DEVICE_TYPE_PORT_LINK |
                                                                   FBE_DEVICE_TYPE_BACK_END_MODULE|FBE_DEVICE_TYPE_MEZZANINE| FBE_DEVICE_TYPE_MISC |
                                                                   FBE_DEVICE_TYPE_MGMT_MODULE|FBE_DEVICE_TYPE_SFP|FBE_DEVICE_TYPE_CONNECTOR),
                                                                  base_object,
                                                                  (FBE_TOPOLOGY_OBJECT_TYPE_PORT|FBE_TOPOLOGY_OBJECT_TYPE_BOARD|FBE_TOPOLOGY_OBJECT_TYPE_ENCLOSURE),
                                                                  FBE_PACKAGE_NOTIFICATION_ID_PHYSICAL);

    }
    else
    {
        /* 
         * Register for notifications from the board and port objects regarding the device types that we
         * are concerned with for module management.  SFP information comes from the port object, everything
         * else comes from the board object.
         */
        status = fbe_base_environment_register_event_notification((fbe_base_environment_t *) base_object,
                                                                  (fbe_api_notification_callback_function_t)fbe_module_mgmt_state_change_handler,
                                                                  FBE_NOTIFICATION_TYPE_OBJECT_DATA_CHANGED,
                                                                  (FBE_DEVICE_TYPE_IOMODULE| FBE_DEVICE_TYPE_PORT_LINK |
                                                                   FBE_DEVICE_TYPE_BACK_END_MODULE|FBE_DEVICE_TYPE_MEZZANINE| FBE_DEVICE_TYPE_MISC |
                                                                   FBE_DEVICE_TYPE_MGMT_MODULE|FBE_DEVICE_TYPE_SFP),
                                                                  base_object,
                                                                  (FBE_TOPOLOGY_OBJECT_TYPE_PORT|FBE_TOPOLOGY_OBJECT_TYPE_BOARD|FBE_TOPOLOGY_OBJECT_TYPE_ENCLOSURE),
                                                                  FBE_PACKAGE_NOTIFICATION_ID_PHYSICAL);
    }
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
 * fbe_module_mgmt_register_state_change_callback_cond_function() 
 *******************************************************************/

/*!**************************************************************
 * fbe_module_mgmt_register_cmi_callback_cond_function()
 ****************************************************************
 * @brief
 *  This function registers with the physical package to get
 *  notified on module object life cycle state changes
 * 
 * @param object_handle - This is the object handle, or in our
 * case the module_mgmt.
 * @param packet_p - The packet arriving from the monitor
 * scheduler.
 * 
 * @return fbe_status_t - The status of the call to
 *                        fbe_lifecycle_class_const_verify() for
 *                        the module management's constant
 *                        lifecycle data.
 *
 * @author
 *  11-Mar-2010 - Created. Nayana Chaudhari
 *
 ****************************************************************/
static fbe_lifecycle_status_t
fbe_module_mgmt_register_cmi_callback_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet)
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
															FBE_CMI_CLIENT_ID_MODULE_MGMT,
															fbe_module_mgmt_cmi_message_handler);

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
 * fbe_module_mgmt_register_state_change_callback_cond_function() 
 *******************************************************************/

/*!**************************************************************
 * fbe_module_mgmt_fup_register_callback_cond_function()
 ****************************************************************
 * @brief
 *  This function registers the callback functions with the base environment object.
 * 
 * @param base_object - This is the object handle, or in our
 * case the module_mgmt.
 * @param packet - The packet arriving from the monitor
 * scheduler.
 * 
 * @return fbe_status_t - The status of the call to
 *                        fbe_lifecycle_class_const_verify() for
 *                        the Encl management's constant
 *                        lifecycle data.
 *
 * @author
 *  01-Nov-2012: Rui Chang - Created. 
 *
 ****************************************************************/
static fbe_lifecycle_status_t
fbe_module_mgmt_fup_register_callback_cond_function(fbe_base_object_t * base_object, 
                                                 fbe_packet_t * packet)
{
	fbe_status_t status = FBE_STATUS_OK;
    fbe_base_environment_t * pBaseEnv = (fbe_base_environment_t *)base_object;

    fbe_base_object_trace(base_object, 
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s entry\n",
                          __FUNCTION__);

    fbe_base_env_set_fup_get_full_image_path_callback(pBaseEnv, &fbe_module_mgmt_fup_get_full_image_path);
    fbe_base_env_set_fup_get_manifest_file_full_path_callback(pBaseEnv, &fbe_module_mgmt_fup_get_manifest_file_full_path);
    fbe_base_env_set_fup_check_env_status_callback(pBaseEnv, &fbe_module_mgmt_fup_check_env_status);
    fbe_base_env_set_fup_info_ptr_callback(pBaseEnv, &fbe_module_mgmt_get_fup_info_ptr);
    fbe_base_env_set_fup_info_callback(pBaseEnv, &fbe_module_mgmt_get_fup_info);
    fbe_base_env_set_fup_get_firmware_rev_callback(pBaseEnv, &fbe_module_mgmt_fup_get_firmware_rev);
    fbe_base_env_set_fup_refresh_device_status_callback(pBaseEnv, NULL);
    fbe_base_env_set_fup_initiate_upgrade_callback(pBaseEnv, &fbe_module_mgmt_fup_initiate_upgrade);
    fbe_base_env_set_fup_resume_upgrade_callback(pBaseEnv, &fbe_module_mgmt_fup_resume_upgrade);
    fbe_base_env_set_fup_priority_check_callback(pBaseEnv, (void *)NULL);
    fbe_base_env_set_fup_copy_fw_rev_to_resume_prom(pBaseEnv, (void *)fbe_module_mgmt_fup_copy_fw_rev_to_resume_prom);

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
 * fbe_module_mgmt_fup_register_callback_cond_function() 
 *******************************************************************/

/*!**************************************************************
 * fbe_module_mgmt_state_change_notification_cond_function()
 ****************************************************************
 * @brief
 *  This is the handler function for state change notification.
 *  
 *
 * @param object_handle - This is the object handle, or in our
 * case the module_mgmt.
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
 *  11-Mar-2010 - Created. Nayana Chaudhari
 *
 ****************************************************************/
static fbe_status_t 
fbe_module_mgmt_state_change_handler(update_object_msg_t *update_object_msg, void *context)
{
	 
    fbe_notification_info_t *notification_info = &update_object_msg->notification_info;
    fbe_module_mgmt_t          *module_mgmt;
    fbe_base_object_t       *base_object;
    fbe_status_t            status;
    fbe_device_physical_location_t phys_location;
    fbe_bool_t              moduleInfoChange = FALSE;
    fbe_u8_t                iom_num = 0;
    fbe_u8_t                port_num = 0;
    fbe_u64_t       device_type;
    fbe_u32_t               slot;

    module_mgmt = (fbe_module_mgmt_t *)context;
    base_object = (fbe_base_object_t *)module_mgmt;
    
    fbe_base_object_trace(base_object, 
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s entry\n",
                          __FUNCTION__);

    phys_location = notification_info->notification_data.data_change_info.phys_location;

    /*
     * Process the notification
     */
    if (notification_info->notification_type == FBE_NOTIFICATION_TYPE_OBJECT_DATA_CHANGED)
    {
        if (notification_info->notification_data.data_change_info.data_type == FBE_DEVICE_DATA_TYPE_FUP_INFO)
        {
            status = fbe_base_env_fup_handle_firmware_status_change((fbe_base_environment_t *)module_mgmt, 
                                                               notification_info->notification_data.data_change_info.device_mask,
                                                               &notification_info->notification_data.data_change_info.phys_location);
        }
        else
        {
            switch (notification_info->notification_data.data_change_info.device_mask)
            {
            case FBE_DEVICE_TYPE_MISC:
            case FBE_DEVICE_TYPE_IOMODULE:
            case FBE_DEVICE_TYPE_BACK_END_MODULE:
            case FBE_DEVICE_TYPE_MEZZANINE:
                if(notification_info->notification_data.data_change_info.data_type == 
                                FBE_DEVICE_DATA_TYPE_GENERIC_INFO)
                {
                    // module state change reported by Board Object
                    status = fbe_module_mgmt_get_new_module_info(module_mgmt, 
                                phys_location.sp,
                                phys_location.slot,
                                notification_info->notification_data.data_change_info.device_mask);
        
                    if (status != FBE_STATUS_OK) 
                    {
                        fbe_base_object_trace(base_object, 
                                              FBE_TRACE_LEVEL_ERROR,
                                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                              "%s Error getting module status, status: 0x%X\n",
                                              __FUNCTION__, status);
                    }
                    else
                    {
                        moduleInfoChange = TRUE;
                    }
                }
                else if(notification_info->notification_data.data_change_info.data_type == 
                                FBE_DEVICE_DATA_TYPE_PORT_INFO)
                {
                    status = fbe_module_mgmt_get_new_io_port_info(module_mgmt,
                                                                  phys_location.sp,
                                                                  phys_location.slot,
                                                                  phys_location.port,
                                                                  notification_info->notification_data.data_change_info.device_mask);
                    if (status != FBE_STATUS_OK) 
                    {
                        fbe_base_object_trace(base_object, 
                                              FBE_TRACE_LEVEL_ERROR,
                                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                              "%s Error getting module status, status: 0x%X\n",
                                              __FUNCTION__, status);
                    }
                    else
                    {
                        moduleInfoChange = TRUE;
                    }
                }
                break;

            case FBE_DEVICE_TYPE_MGMT_MODULE:
                status = fbe_module_mgmt_get_new_mgmt_module_info(module_mgmt,
                    phys_location.sp,
                    phys_location.slot);
                if (status != FBE_STATUS_OK) 
                {
                    fbe_base_object_trace(base_object, 
                                          FBE_TRACE_LEVEL_ERROR,
                                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                          "%s Error getting module status, status: 0x%X\n",
                                          __FUNCTION__, status);
                }
                else
                {
                    moduleInfoChange = TRUE;
                }
                break;
    
            case FBE_DEVICE_TYPE_SFP:
                /*
                 * Port object notifications overload the port field with a port object ID.  This can
                 * be used to lookup the io module and port number via some other API calls.
                 */
                status = fbe_module_mgmt_get_iom_num_port_num_from_port_device_object_id(module_mgmt, phys_location.port, &iom_num, &port_num );
    
                if (status == FBE_STATUS_OK) 
                {
                    // we matched the port object to a known port, now request the sfp information
                    fbe_module_mgmt_get_new_sfp_info(module_mgmt,
                                                     module_mgmt->local_sp,
                                                     iom_num,
                                                     port_num,
                                                     phys_location.port); 

                    // check everything to be able to fault both ports in a combined connector when necessary
                    fbe_module_mgmt_check_all_module_and_port_states(module_mgmt);
                }
                if (status != FBE_STATUS_OK) 
                {
                    fbe_base_object_trace(base_object, 
                                          FBE_TRACE_LEVEL_INFO,
                                          FBE_TRACE_MESSAGE_ID_INFO,
                                          "%s (SFP),Error mapping port to port object, port object id %d, status: 0x%X\n",
                                          __FUNCTION__, phys_location.port, status);
                }
                else
                {
                    moduleInfoChange = TRUE;

                    //update the device type and also location info in notification to the physical location
                    if (module_mgmt->base_environment.processorEnclType == XPE_ENCLOSURE_TYPE)
                    {
                        phys_location.bus = FBE_XPE_PSEUDO_BUS_NUM;
                        phys_location.enclosure = FBE_XPE_PSEUDO_ENCL_NUM;
                    }
                    else if (module_mgmt->base_environment.processorEnclType == VPE_ENCLOSURE_TYPE)
                    {
                        phys_location.bus = FBE_XPE_PSEUDO_BUS_NUM;
                        phys_location.enclosure = FBE_XPE_PSEUDO_ENCL_NUM;
                    }
                    else
                    {
                        phys_location.bus = 0;
                        phys_location.enclosure = 0;
                    }

                    fbe_module_mgmt_iom_num_to_slot_and_device_type(module_mgmt, module_mgmt->local_sp,
                                                                    iom_num, &slot, &device_type);
                    phys_location.sp = module_mgmt->local_sp;
                    phys_location.port = port_num;
                    phys_location.slot = slot;
                    notification_info->notification_data.data_change_info.device_mask = device_type;
                }
                break;
    
            case FBE_DEVICE_TYPE_PORT_LINK:
                /* This is a link status change from the port object. */
                status = fbe_module_mgmt_get_iom_num_port_num_from_port_device_object_id(module_mgmt, phys_location.port, &iom_num, &port_num );
                if (status == FBE_STATUS_OK) 
                {
                    // we matched the port object to a known port, now request the link information
                    fbe_module_mgmt_get_new_port_link_info(module_mgmt,
                                                           module_mgmt->local_sp,
                                                           iom_num,
                                                           port_num,
                                                           phys_location.port); 
                }
                if (status != FBE_STATUS_OK) 
                {
                    fbe_base_object_trace(base_object, 
                                          FBE_TRACE_LEVEL_INFO,
                                          FBE_TRACE_MESSAGE_ID_INFO,
                                          "%s (PortLink), Error mapping port to port object, iom %d, port %d, status: 0x%X\n",
                                          __FUNCTION__, iom_num, port_num, status);
                }
                else
                {
                    moduleInfoChange = TRUE;

                    //update the device type and also location info in notification to the physical location
                    if (module_mgmt->base_environment.processorEnclType == XPE_ENCLOSURE_TYPE)
                    {
                        phys_location.bus = FBE_XPE_PSEUDO_BUS_NUM;
                        phys_location.enclosure = FBE_XPE_PSEUDO_ENCL_NUM;
                    }
                    else if (module_mgmt->base_environment.processorEnclType == VPE_ENCLOSURE_TYPE)
                    {
                        phys_location.bus = FBE_XPE_PSEUDO_BUS_NUM;
                        phys_location.enclosure = FBE_XPE_PSEUDO_ENCL_NUM;
                    }
                    else
                    {
                        phys_location.bus = 0;
                        phys_location.enclosure = 0;
                    }

                    fbe_module_mgmt_iom_num_to_slot_and_device_type(module_mgmt, module_mgmt->local_sp,
                                                                    iom_num, &slot, &device_type);
                    phys_location.sp = module_mgmt->local_sp;
                    phys_location.port = port_num;
                    phys_location.slot = slot;
                    notification_info->notification_data.data_change_info.device_mask = device_type;
                }
                break;

            case FBE_DEVICE_TYPE_CONNECTOR:
                fbe_module_mgmt_handle_connector_status_change(module_mgmt, phys_location);
                break;
                    
            default:
                return FBE_STATUS_OK;
                break;
            }
        }
    } // end of - if object data changed

    if(moduleInfoChange)
    {
        /* Send data change notification */
        fbe_base_environment_send_data_change_notification((fbe_base_environment_t*)module_mgmt,
                                                           FBE_CLASS_ID_MODULE_MGMT,
                                                           notification_info->notification_data.data_change_info.device_mask,
                                                           FBE_DEVICE_DATA_TYPE_GENERIC_INFO,
                                                           &phys_location);
    }
	return FBE_STATUS_OK;
}
/*******************************************************************
 * end fbe_module_mgmt_state_change_notification_cond_function() 
 *******************************************************************/

/*!**************************************************************
 * @fn fbe_module_mgmt_get_mgmt_module_logical_fault()
 ****************************************************************
 * @brief
 *  This function considers all the fields for the management module
 *  and decides whether the management module is logically faulted or not.
 *
 * @param mgmt_module_comp_info -The pointer to fbe_board_mgmt_mgmt_module_info_t.
 *
 * @return fbe_bool_t - The management module is faulted or not logically.
 *
 * @author
 *  14-Mar-2013: Lin Lou - Created.
 *
 ****************************************************************/
static fbe_bool_t
fbe_module_mgmt_get_mgmt_module_logical_fault(fbe_board_mgmt_mgmt_module_info_t *mgmt_module_comp_info)
{
    fbe_bool_t logicalFault = FBE_FALSE;

    if(mgmt_module_comp_info->envInterfaceStatus != FBE_ENV_INTERFACE_STATUS_GOOD
            || mgmt_module_comp_info->generalFault
            || !mgmt_module_comp_info->bInserted)
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
 * @fn fbe_module_mgmt_update_encl_fault_led(fbe_module_mgmt_t *module_mgmt,
 *                                  fbe_device_physical_location_t *pLocation,
 *                                  fbe_encl_fault_led_reason_t enclFaultLedReason)
 ****************************************************************
 * @brief
 *  This function checks the status to determine
 *  if the Enclosure Fault LED needs to be updated.
 *
 * @param module_mgmt -
 * @param pLocation - The location of the module.
 *
 * @return fbe_status_t -
 *
 * @author
 *  14-Mar-2013:  Lin Lou - Created.
 *
 ****************************************************************/
fbe_status_t fbe_module_mgmt_update_encl_fault_led(fbe_module_mgmt_t *module_mgmt,
                                    fbe_device_physical_location_t *pLocation,
                                    fbe_u64_t device_type,
                                    fbe_encl_fault_led_reason_t enclFaultLedReason)
{
    fbe_status_t                                status = FBE_STATUS_OK;
    fbe_u32_t                                   sp, slot;
    fbe_bool_t                                  enclFaultLedOn = FBE_FALSE;
    fbe_board_mgmt_mgmt_module_info_t           *mgmt_module_comp_info;
    fbe_base_env_resume_prom_info_t             *module_resume_info;
    fbe_u32_t                                   iom_index;

    switch(enclFaultLedReason)
    {
    case FBE_ENCL_FAULT_LED_MGMT_MODULE_FLT:
        for(sp = SP_A; sp < SP_ID_MAX; sp++)
        {
            for(slot = 0; slot < FBE_ESP_MGMT_MODULE_MAX_COUNT_PER_SP; slot++)
            {
                mgmt_module_comp_info = &module_mgmt->mgmt_module_info[slot].mgmt_module_comp_info[sp];
                enclFaultLedOn = fbe_module_mgmt_get_mgmt_module_logical_fault(mgmt_module_comp_info);

                if(enclFaultLedOn)
                {
                    break;
                }
            }
            if(enclFaultLedOn)
            {
                break;
            }
        }
        break;

    case FBE_ENCL_FAULT_LED_MGMT_MODULE_RESUME_PROM_FLT:
        for(sp = SP_A; sp < SP_ID_MAX; sp++)
        {
            for(slot = 0; slot < FBE_ESP_MGMT_MODULE_MAX_COUNT_PER_SP; slot++)
            {
                module_resume_info = &module_mgmt->mgmt_module_info[slot].mgmt_module_resume_info[sp];
                enclFaultLedOn = fbe_base_env_get_resume_prom_fault(module_resume_info->op_status);

                if(enclFaultLedOn)
                {
                    break;
                }
            }
            if(enclFaultLedOn)
            {
                break;
            }
        }
        break;

    case FBE_ENCL_FAULT_LED_IO_MODULE_RESUME_PROM_FLT:
    case FBE_ENCL_FAULT_LED_BEM_RESUME_PROM_FLT:
    case FBE_ENCL_FAULT_LED_MEZZANINE_RESUME_PROM_FLT:
        if(enclFaultLedReason == FBE_ENCL_FAULT_LED_IO_MODULE_RESUME_PROM_FLT)
        {
            device_type = FBE_DEVICE_TYPE_IOMODULE;
        }
        else if(enclFaultLedReason == FBE_ENCL_FAULT_LED_BEM_RESUME_PROM_FLT)
        {
            device_type = FBE_DEVICE_TYPE_BACK_END_MODULE;
        }
        else if(enclFaultLedReason == FBE_ENCL_FAULT_LED_MEZZANINE_RESUME_PROM_FLT)
        {
            device_type = FBE_DEVICE_TYPE_MEZZANINE;
        }

        for(sp = SP_A; sp < SP_ID_MAX; sp++)
        {
            for(slot = 0; slot < FBE_ESP_IO_MODULE_MAX_COUNT; slot++)
            {
                if(module_mgmt->io_module_info[slot].physical_info[sp].type != device_type)
                {
                    continue;
                }
                module_resume_info = &module_mgmt->io_module_info[slot].physical_info[sp].module_resume_info;
                enclFaultLedOn = fbe_base_env_get_resume_prom_fault(module_resume_info->op_status);

                if(enclFaultLedOn)
                {
                    break;
                }
            }
            if(enclFaultLedOn)
            {
                break;
            }
        }
        break;
    case FBE_ENCL_FAULT_LED_IO_MODULE_FLT:
        iom_index = fbe_module_mgmt_convert_device_type_and_slot_to_index(device_type,pLocation->slot);
        if(fbe_module_mgmt_get_module_state(module_mgmt,pLocation->sp,iom_index) == MODULE_STATE_FAULTED)
        {
            enclFaultLedOn = FBE_TRUE;
        }
        else
        {
            enclFaultLedOn = FBE_FALSE;
        }
        break;
    case FBE_ENCL_FAULT_LED_IO_PORT_FLT:
        iom_index = fbe_module_mgmt_convert_device_type_and_slot_to_index(device_type,pLocation->slot);
        if(fbe_module_mgmt_get_port_state(module_mgmt,pLocation->sp, iom_index, pLocation->port) == FBE_PORT_STATE_FAULTED)
        {
            enclFaultLedOn = FBE_TRUE;
        }
        else
        {
            enclFaultLedOn = FBE_FALSE;
        }
        break;


    default:
        fbe_base_object_trace((fbe_base_object_t *)module_mgmt,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "ModMgmt: unsupported enclFaultLedReason 0x%llX.\n",
                              enclFaultLedReason);

        return FBE_STATUS_GENERIC_FAILURE;
        break;
    }

    status = fbe_base_environment_updateEnclFaultLed((fbe_base_object_t *)module_mgmt,
                                                     module_mgmt->board_object_id,
                                                     enclFaultLedOn,
                                                     enclFaultLedReason);

    if (status == FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)module_mgmt,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "ModMgmt: %s enclFaultLedReason %s.\n",
                              enclFaultLedOn? "SET" : "CLEAR",
                              fbe_base_env_decode_encl_fault_led_reason(enclFaultLedReason));
    }
    else
    {
        fbe_base_object_trace((fbe_base_object_t *)module_mgmt,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "ModMgmt: error in %s enclFaultLedReason %s.\n",
                              enclFaultLedOn? "SET" : "CLEAR",
                              fbe_base_env_decode_encl_fault_led_reason(enclFaultLedReason));

    }

    return status;
}


/*!**************************************************************
 * fbe_module_mgmt_cmi_message_handler()
 ****************************************************************
 * @brief
 *  This is the handler function for CMI message notification.
 *  
 *
 * @param object_handle - This is the object handle, or in our
 * case the module_mgmt.
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
 *  11-Mar-2010 - Created. Nayana Chaudhari
 *
 ****************************************************************/
static fbe_status_t 
fbe_module_mgmt_cmi_message_handler(fbe_base_object_t *base_object, 
                                    fbe_base_environment_cmi_message_info_t *cmi_message)
{
	fbe_module_mgmt_t                  *module_mgmt;
    fbe_module_mgmt_cmi_msg_t          *cmiMsgPtr;
    fbe_status_t                    status = FBE_STATUS_OK;

    module_mgmt = (fbe_module_mgmt_t *)base_object;
    cmiMsgPtr = (fbe_module_mgmt_cmi_msg_t *)cmi_message->user_message;

    fbe_base_object_trace(base_object, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s, received message event:0x%x\n",
                              __FUNCTION__,
                              cmi_message->event);

    fbe_base_object_trace(base_object, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s, received message ptr:%p\n",
                              __FUNCTION__,
                              cmiMsgPtr);

    /*
     * process the message based on type
     */
    switch (cmi_message->event)
    {
        fbe_base_object_trace(base_object, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s, received %s\n",
                              __FUNCTION__,
                              fbe_module_mgmt_cmi_msg_to_string(fbe_base_environment_get_cmi_msg_opcode((fbe_base_environment_cmi_message_t *)cmiMsgPtr)));

        case FBE_CMI_EVENT_MESSAGE_RECEIVED:
            status = fbe_module_mgmt_processReceivedCmiMessage(module_mgmt, cmiMsgPtr);
            break;

        case FBE_CMI_EVENT_PEER_NOT_PRESENT:
            status = fbe_module_mgmt_processPeerNotPresent(module_mgmt, cmiMsgPtr);
            break;

        case FBE_CMI_EVENT_SP_CONTACT_LOST:
            status = fbe_module_mgmt_processContactLost(module_mgmt);
            break;

        case FBE_CMI_EVENT_PEER_BUSY:
            status = fbe_module_mgmt_processPeerBusy(module_mgmt, cmiMsgPtr);
            break;

        case FBE_CMI_EVENT_FATAL_ERROR:
            status = fbe_module_mgmt_processFatalError(module_mgmt, cmiMsgPtr);
            break;

        default:
            status = FBE_STATUS_MORE_PROCESSING_REQUIRED;
            break;
    }

    if (status == FBE_STATUS_MORE_PROCESSING_REQUIRED)
    {
        // handle the message in base env
        status = fbe_base_environment_cmi_message_handler(base_object, cmi_message);

        if(status != FBE_STATUS_OK) 
        {
            fbe_base_object_trace(base_object, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s failed to handle CMI msg.\n",
                               __FUNCTION__);
        }
    }

    return status;
}
/*******************************************************************
 * end fbe_module_mgmt_cmi_message_handler() 
 *******************************************************************/


/*!**************************************************************
 * fbe_module_mgmt_read_persistent_data_complete_cond_function()
 ****************************************************************
 * @brief
 *  This function handles the persistent storage read completion.
 *  
 *
 * @param object_handle - This is the object handle, or in our
 * case the module_mgmt.
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
 *  11-Jan-2010 - Created. bphilbin
 *
 ****************************************************************/
static fbe_lifecycle_status_t fbe_module_mgmt_read_persistent_data_complete_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet)
{
    fbe_module_mgmt_t *module_mgmt = (fbe_module_mgmt_t *)base_object;
    fbe_u32_t port_index, persistent_port_index;
    fbe_io_port_persistent_info_t *persistent_port_info;
    fbe_u32_t iom_num;
    fbe_status_t status;
    SP_ID sp_id;
    fbe_u32_t port_num;
    fbe_u32_t   mgmt_id;
    fbe_mgmt_port_config_info_t *persistent_data_p;

    status = fbe_lifecycle_clear_current_cond(base_object);
    module_mgmt->loading_config = FALSE;

    /*
     * Copy data over from persistent storage into the structures.
     */
    persistent_port_info = (fbe_io_port_persistent_info_t *)((fbe_base_environment_t *)module_mgmt)->my_persistent_data;

    for(sp_id = 0; sp_id < SP_ID_MAX; sp_id++)
    {
        for(persistent_port_index = 0; persistent_port_index < FBE_ESP_MAX_CONFIGURED_PORTS_PER_SP; persistent_port_index++)
        {
            if( (persistent_port_info[persistent_port_index].port_location.type_32_bit != FBE_DEVICE_TYPE_INVALID) &&
                (persistent_port_info[persistent_port_index].port_location.slot != INVALID_MODULE_NUMBER))
            {
                iom_num = fbe_module_mgmt_convert_device_type_and_slot_to_index(persistent_port_info[persistent_port_index].port_location.type_32_bit,
                                                                persistent_port_info[persistent_port_index].port_location.slot);
                port_num = persistent_port_info[persistent_port_index].port_location.port;
                if(port_num != INVALID_PORT_U8)
                {
                    port_index = fbe_module_mgmt_get_io_port_index(iom_num, port_num);

                    if((module_mgmt->io_port_info[port_index].port_logical_info[sp_id].port_configured_info.logical_num == 0) &&
                       (module_mgmt->io_port_info[port_index].port_logical_info[sp_id].port_configured_info.port_role == IOPORT_PORT_ROLE_BE))
                    {
                        /*
                         * We don't want to overwrite an existing BE 0 configuration as we would have set this during bootup as we detected
                         * the boot port.
                         */
                        continue;
                    }

                    module_mgmt->port_configuration_loaded = FBE_TRUE;

                    fbe_copy_memory(&module_mgmt->io_port_info[port_index].port_logical_info[sp_id].port_configured_info,
                                    &persistent_port_info[persistent_port_index], sizeof(fbe_io_port_persistent_info_t));
                    
                    fbe_base_object_trace((fbe_base_object_t *) module_mgmt, 
                                            FBE_TRACE_LEVEL_INFO,
                                            FBE_TRACE_MESSAGE_ID_INFO,
                                            "ModMgmt: Loaded Role:%s%d, Group:%d, for SP:%d IOM:%d Port:%d\n", 
                                            fbe_module_mgmt_port_role_to_string(persistent_port_info[persistent_port_index].port_role),
                                            persistent_port_info[persistent_port_index].logical_num,
                                            persistent_port_info[persistent_port_index].iom_group,
                                            sp_id, iom_num, port_num);


                }
            }
        }
        /*
         * The management module data is tagged on at the end of all the port data, copy this into the 
         * management module memory as well.
         */ 
        persistent_data_p = (fbe_mgmt_port_config_info_t *) (((fbe_base_environment_t *)module_mgmt)->my_persistent_data + 
                       FBE_MGMT_PORT_PERSISTENT_DATA_OFFSET);
        for(mgmt_id = 0; mgmt_id < FBE_ESP_MGMT_MODULE_MAX_COUNT_PER_SP; mgmt_id++)
        {
            fbe_copy_memory(&module_mgmt->mgmt_module_info[mgmt_id].mgmt_port_persistent_info,
                            persistent_data_p,
                            sizeof(fbe_mgmt_port_config_info_t));
            fbe_base_object_trace((fbe_base_object_t *) module_mgmt, 
                                    FBE_TRACE_LEVEL_INFO,
                                    FBE_TRACE_MESSAGE_ID_INFO,
                                    "Loaded MgmtID:%d AutoNeg:%d Duplex:%d Speed%d from persistent storage\n",
                                    mgmt_id,
                                    module_mgmt->mgmt_module_info[mgmt_id].mgmt_port_persistent_info.mgmtPortAutoNeg, 
                                    module_mgmt->mgmt_module_info[mgmt_id].mgmt_port_persistent_info.mgmtPortDuplex,
                                    module_mgmt->mgmt_module_info[mgmt_id].mgmt_port_persistent_info.mgmtPortSpeed);
            persistent_data_p++;
        }
    }
    fbe_module_mgmt_check_for_configured_combined_port(module_mgmt);

    return FBE_LIFECYCLE_STATUS_DONE;
}

/*!**************************************************************
 * fbe_module_mgmt_discover_module_cond_function()
 ****************************************************************
 * @brief
 *  This function gets the list of all the new module that
 *  was added and adds them to the queue to be processed later
 *
 * @param object_handle - This is the object handle, or in our
 * case the module_mgmt.
 * @param packet_p - The packet arriving from the monitor
 * scheduler.
 *
 * @return fbe_status_t - The status of the call to
 *                        fbe_lifecycle_class_const_verify()
 *                        for the module management's
 *                        constant lifecycle data.
 *
 * @author
 *  11-Mar-2010 - Created. Nayana Chaudhari
 *
 ****************************************************************/
static fbe_lifecycle_status_t 
fbe_module_mgmt_discover_module_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet)
{
    fbe_object_id_t      object_id;
    fbe_u32_t            io_mod_count_per_blade = 0;
    fbe_u32_t            bem_count_per_blade = 0;
    fbe_u32_t            mezzanine_count_per_blade = 0;
    fbe_u32_t            mgmt_mod_count_per_blade = 0;
    fbe_u32_t            io_port_count_per_blade = 0;
    fbe_u32_t            iom_num = 0, slot = 0;
    fbe_u32_t            io_port = 0;
    fbe_u32_t            port_index = 0;
    fbe_u32_t            port_num = 0;
    fbe_u32_t            count=0, spCount=SP_ID_MAX;
    fbe_status_t         status;
    SP_ID                sp;
    fbe_module_mgmt_t    *module_mgmt = (fbe_module_mgmt_t *) base_object;
    fbe_board_mgmt_misc_info_t    miscInfoPtr = {0};
    fbe_u64_t    device_type;
    fbe_board_mgmt_platform_info_t platform_info;
    fbe_object_id_t     port_object_id = FBE_OBJECT_ID_INVALID;
    fbe_device_physical_location_t phys_location = {0};
    HW_ENCLOSURE_TYPE processorEnclType;
 
    if(module_mgmt->loading_config == TRUE)
    {
        /*
         * We have not completed reading our configuration, just
         * return here and we will be rescheduled again.
         */
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    status = fbe_lifecycle_clear_current_cond(base_object);
    module_mgmt->discovering_hardware = TRUE;
    if (status != FBE_STATUS_OK) {
        fbe_base_object_trace(base_object, 
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s can't clear current condition, status: 0x%X",
                                __FUNCTION__, status);
    }
    // Get the Board Object ID 
    status = fbe_api_get_board_object_id(&object_id);
    if (status != FBE_STATUS_OK) 
    {
        fbe_base_object_trace(base_object, 
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s Error in getting the Board Object ID, status: 0x%X\n",
                                __FUNCTION__, status);
    }
    else
    {
        /*
         * Save the board object id here so we don't have to request it every time
         * we issue an API request to the physical package.
         */
        module_mgmt->board_object_id = object_id;

        status = fbe_api_board_get_platform_info(object_id, &platform_info);
        if (status != FBE_STATUS_OK) 
        {
            fbe_base_object_trace(base_object, 
                                    FBE_TRACE_LEVEL_ERROR,
                                    FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                    "%s Error in getting the platform information, status: 0x%X\n",
                                    __FUNCTION__, status);
        }

        fbe_environment_limit_get_platform_hardware_limits(&(module_mgmt->platform_hw_limit));
        fbe_environment_limit_get_platform_port_limits(&(module_mgmt->platform_port_limit));
        
        fbe_base_object_trace(base_object, 
                                FBE_TRACE_LEVEL_INFO,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "ModMgmt: Discover - PlatformType: %s, Hardware Limits - Mezzanines:%d, SLICs:%d\n",
                                decodeSpidHwType(platform_info.hw_platform_info.platformType),
                                module_mgmt->platform_hw_limit.max_mezzanines,
                                module_mgmt->platform_hw_limit.max_slics);
        fbe_base_object_trace(base_object, 
                                FBE_TRACE_LEVEL_INFO,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "ModuleMgmt: Discover - Port Limits SAS BE:%d, SAS FE:%d, FC FE:%d, FCoE FE:%d\n",
                                module_mgmt->platform_port_limit.max_sas_be,
                                module_mgmt->platform_port_limit.max_sas_fe,
                                module_mgmt->platform_port_limit.max_fc_fe,
                                module_mgmt->platform_port_limit.max_fcoe_fe);
        fbe_base_object_trace(base_object, 
                                FBE_TRACE_LEVEL_INFO,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "ModuleMgmt: Discover - Port Limits 10G iSCSI:%d, 1G iSCSI:%d\n",
                                module_mgmt->platform_port_limit.max_iscsi_10g_fe,
                                module_mgmt->platform_port_limit.max_iscsi_1g_fe);

        status = fbe_api_board_get_io_module_count_per_blade(object_id, &io_mod_count_per_blade);
        if (status != FBE_STATUS_OK) 
        {
		    fbe_base_object_trace(base_object, 
								    FBE_TRACE_LEVEL_ERROR,
								    FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								    "%s Error in getting the io_module_count_per_blade, status: 0x%X\n",
								    __FUNCTION__, status);
        }
        module_mgmt->discovered_hw_limit.num_slic_slots = io_mod_count_per_blade;

        status = fbe_api_board_get_bem_count_per_blade(object_id, &bem_count_per_blade);
        if (status != FBE_STATUS_OK) 
        {
		    fbe_base_object_trace(base_object, 
								    FBE_TRACE_LEVEL_ERROR,
								    FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								    "%s Error in getting bem_count_per_blade, status: 0x%X\n",
								    __FUNCTION__, status);
        }
        module_mgmt->discovered_hw_limit.num_bem = bem_count_per_blade;

        status = fbe_api_board_get_io_port_count_per_blade(object_id, &io_port_count_per_blade);
        if (status != FBE_STATUS_OK) 
        {
		    fbe_base_object_trace(base_object, 
								    FBE_TRACE_LEVEL_ERROR,
								    FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								    "%s Error in getting io_port_count_per_blade, status: 0x%X\n",
								    __FUNCTION__, status);
        }
        module_mgmt->discovered_hw_limit.num_ports = io_port_count_per_blade;

        status = fbe_api_board_get_mezzanine_count_per_blade(object_id, &mezzanine_count_per_blade);
        if (status != FBE_STATUS_OK) 
        {
		    fbe_base_object_trace(base_object, 
								    FBE_TRACE_LEVEL_ERROR,
								    FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								    "%s Error in getting mezzanine_count_per_blade, status: 0x%X\n",
								    __FUNCTION__, status);
        }

        /*
         * Temporary hack for now to force the mezzanine count to 0 until specl can report it for us.
         */
        if( (platform_info.hw_platform_info.platformType == SPID_DEFIANT_M1_HW_TYPE) ||
            (platform_info.hw_platform_info.platformType == SPID_DEFIANT_M2_HW_TYPE) ||
            (platform_info.hw_platform_info.platformType == SPID_DEFIANT_M3_HW_TYPE) ||
            (platform_info.hw_platform_info.platformType == SPID_DEFIANT_M4_HW_TYPE) ||
            (platform_info.hw_platform_info.platformType == SPID_DEFIANT_M5_HW_TYPE) ||
            (platform_info.hw_platform_info.platformType == SPID_DEFIANT_K2_HW_TYPE) ||
            (platform_info.hw_platform_info.platformType == SPID_DEFIANT_K3_HW_TYPE))

        {
            fbe_base_object_trace(base_object, 
                                    FBE_TRACE_LEVEL_INFO,
                                    FBE_TRACE_MESSAGE_ID_INFO,
                                    "%s Forcing mezzanine count to 0\n",
                                    __FUNCTION__);

            module_mgmt->discovered_hw_limit.num_mezzanine_slots = 0;
            mezzanine_count_per_blade = 0;
        }
        else
        {
            module_mgmt->discovered_hw_limit.num_mezzanine_slots = mezzanine_count_per_blade;
        }

        status = fbe_api_board_get_mgmt_module_count_per_blade(object_id, &mgmt_mod_count_per_blade);
        if (status != FBE_STATUS_OK) 
        {
		    fbe_base_object_trace(base_object, 
								    FBE_TRACE_LEVEL_ERROR,
								    FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								    "%s Error in getting mgmt_module_count_per_blade, status: 0x%X\n",
								    __FUNCTION__, status);
        }
        module_mgmt->discovered_hw_limit.num_mgmt_modules = mgmt_mod_count_per_blade;

        fbe_base_object_trace(base_object,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "ModuleMgmt: Discover - SLIC Slots:%d Ports:%d Mezzanine Slots:%d\n",
                              module_mgmt->discovered_hw_limit.num_slic_slots,
                              module_mgmt->discovered_hw_limit.num_ports,
                              module_mgmt->discovered_hw_limit.num_mezzanine_slots);

        fbe_base_object_trace(base_object,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "ModuleMgmt: Discover - Base Module Slots:%d\n",
                              module_mgmt->discovered_hw_limit.num_bem);

        status = fbe_api_board_get_misc_info(object_id, &miscInfoPtr);
        sp = miscInfoPtr.spid;
        module_mgmt->local_sp = miscInfoPtr.spid;

        if (module_mgmt->base_environment.isSingleSpSystem == TRUE)
        {
            spCount = SP_COUNT_SINGLE;
        }
        else
        {
            spCount = SP_ID_MAX;
        }
       
        for(sp = SP_A; sp < spCount; sp++)
        {
            // get io module info for all io modules
            for(count = 0; count < io_mod_count_per_blade; count++)
            {
                status = fbe_module_mgmt_get_new_module_info(module_mgmt, sp, count, FBE_DEVICE_TYPE_IOMODULE);
                if (status != FBE_STATUS_OK) 
                {
                    fbe_base_object_trace(base_object, 
                                          FBE_TRACE_LEVEL_ERROR,
                                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                          "%s Error getting module info, status: 0x%X\n",
                                          __FUNCTION__, status);
                }
            }
            // get the info for all back end modules
            for(count = 0; count < bem_count_per_blade; count++)
            {
                status = fbe_module_mgmt_get_new_module_info(module_mgmt, sp, count, FBE_DEVICE_TYPE_BACK_END_MODULE);
                if (status != FBE_STATUS_OK) 
                {
                    fbe_base_object_trace(base_object, 
                                          FBE_TRACE_LEVEL_ERROR,
                                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                          "%s Error getting BEM info, status: 0x%X\n",
                                          __FUNCTION__, status);
                }
            }
            // get the info for all all mezzanines
            for(count = 0; count < mezzanine_count_per_blade; count++)
            {
                status = fbe_module_mgmt_get_new_module_info(module_mgmt, sp, count, FBE_DEVICE_TYPE_MEZZANINE);
                if (status != FBE_STATUS_OK) 
                {
                    fbe_base_object_trace(base_object, 
                                          FBE_TRACE_LEVEL_ERROR,
                                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                          "%s Error getting mezzanine info, status: 0x%X\n",
                                          __FUNCTION__, status);
                }
            }
            // get the info for all io ports - only for local SP (for both SPs we will require Physical Package support).
            if (miscInfoPtr.spid == sp)
            {
                for(iom_num = 0; iom_num < FBE_ESP_IO_MODULE_MAX_COUNT; iom_num++)
                {
                    if(module_mgmt->io_module_info[iom_num].physical_info[sp].module_comp_info.inserted ==
                        FBE_MGMT_STATUS_TRUE)
                    {
                        fbe_base_object_trace(base_object,
                                          FBE_TRACE_LEVEL_INFO,
                                          FBE_TRACE_MESSAGE_ID_INFO,
                                          "%s Discovered %s %d IOM index:%d Ports:%d\n",
                                          __FUNCTION__, 
                                          fbe_module_mgmt_device_type_to_string(fbe_module_mgmt_get_device_type(module_mgmt, sp, iom_num)),
                                          fbe_module_mgmt_get_slot_num(module_mgmt, sp, iom_num),
                                          iom_num, 
                                          module_mgmt->io_module_info[iom_num].physical_info[sp].module_comp_info.ioPortCount);
                        for(io_port = 0; 
                             io_port < module_mgmt->io_module_info[iom_num].physical_info[sp].module_comp_info.ioPortCount; 
                             io_port++)
                        {
                           
                            fbe_module_mgmt_iom_num_to_slot_and_device_type(module_mgmt, sp, iom_num, &slot, &device_type);
                            status = fbe_module_mgmt_get_new_io_port_info(module_mgmt, sp, slot, io_port, device_type);
                            if (status != FBE_STATUS_OK) 
                            {
                                fbe_base_object_trace(base_object, 
                                                    FBE_TRACE_LEVEL_ERROR,
                                                    FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                                    "%s Error getting io port info, %s %d,port: %d,status: 0x%X\n",
                                                    __FUNCTION__,
                                                    fbe_module_mgmt_device_type_to_string(fbe_module_mgmt_get_device_type(module_mgmt, sp, iom_num)),
                                                    fbe_module_mgmt_get_slot_num(module_mgmt, sp, iom_num),
                                                    io_port, status);
                            }

                           
                            port_index = fbe_module_mgmt_get_io_port_index(iom_num, io_port);
                            fbe_module_mgmt_check_for_combined_port(module_mgmt, iom_num, io_port);
                            port_object_id = fbe_module_mgmt_get_port_device_object_id(module_mgmt, port_index);
                            if(port_object_id != FBE_OBJECT_ID_INVALID)
                            {
                                status = fbe_module_mgmt_get_new_sfp_info(module_mgmt, sp, iom_num, io_port, port_object_id);
                                if(status != FBE_STATUS_OK)
                                {
                                    fbe_base_object_trace(base_object, 
                                                          FBE_TRACE_LEVEL_INFO,
                                                          FBE_TRACE_MESSAGE_ID_INFO,
                                                          "ModMgmt: Discover - Error in getting SFP information for %s:%d Port:%d status:0x%X\n",
                                                          fbe_module_mgmt_device_type_to_string(fbe_module_mgmt_get_device_type(module_mgmt, sp, iom_num)), 
                                                          fbe_module_mgmt_get_slot_num(module_mgmt, sp, iom_num),
                                                          port_num, status);
                                    continue;
                                }
                                status = fbe_module_mgmt_get_new_port_link_info(module_mgmt, sp, iom_num, io_port, port_object_id);
                                if(status != FBE_STATUS_OK)
                                {
                                    fbe_base_object_trace(base_object, 
                                                          FBE_TRACE_LEVEL_INFO,
                                                          FBE_TRACE_MESSAGE_ID_INFO,
                                                          "ModMgmt: Discover - Error in getting Link information for %s:%d Port:%d status:0x%X\n",
                                                          fbe_module_mgmt_device_type_to_string(fbe_module_mgmt_get_device_type(module_mgmt, sp, iom_num)), 
                                                          fbe_module_mgmt_get_slot_num(module_mgmt, sp, iom_num),
                                                          port_num, status);
                                    continue;
                                }
                            }
                            else
                            {
                                fbe_base_object_trace(base_object, 
                                                      FBE_TRACE_LEVEL_INFO,
                                                      FBE_TRACE_MESSAGE_ID_INFO,
                                                      "ModMgmt: cannot gather SFP info for %s %d Port:%d no valid port object\n",
                                                      fbe_module_mgmt_device_type_to_string(fbe_module_mgmt_get_device_type(module_mgmt, sp, iom_num)), 
                                                      fbe_module_mgmt_get_slot_num(module_mgmt, sp, iom_num),
                                                      port_num);
                            }
                        }// end of - for io_port
                    }// end of - if inserted
                }// end of - for iom_num
            }// end of - local sp only

            // get the info for all management modules
            for(count = 0; count < mgmt_mod_count_per_blade; count++)
            {
                status = fbe_module_mgmt_get_new_mgmt_module_info(module_mgmt, sp, count);
                if (status != FBE_STATUS_OK) 
                {
                    fbe_base_object_trace(base_object, 
                                        FBE_TRACE_LEVEL_ERROR,
                                        FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                        "%s Error getting mgmt module info, status: 0x%X\n",
                                        __FUNCTION__, status);
                }
            }
        }
        /* 
         * If we have looped through all IO Modules but have not located the boot device we are loading with a bad configuration
         * data from lower levels.  This must be a bug and needs to be diagnosed.
         */

        if(!module_mgmt->boot_device_found)
        {
            fbe_base_object_trace(base_object, 
                                  FBE_TRACE_LEVEL_CRITICAL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "ModMgmt: Discover - Failed to locate boot device, check device information from PP and SPECL\n");
        }
    }
    module_mgmt->discovering_hardware = FALSE;

    fbe_module_mgmt_check_all_module_and_port_states(module_mgmt);

    //If ESP is in service mode, skip check CNA mode
    if(((fbe_base_environment_t *)base_object)->isBootFlash == TRUE )
    {
        fbe_base_object_trace(base_object,FBE_TRACE_LEVEL_INFO,FBE_TRACE_MESSAGE_ID_INFO,
                          "%s bootflash mode detected, skip CNA mode check.\n",
                              __FUNCTION__);
    }
    else
    {
    fbe_module_mgmt_check_cna_port_protocol(module_mgmt);
    }


    // If this is a DPE based system, get the link status information for the Expansion port of the DPE
    status = fbe_base_env_get_processor_encl_type((fbe_base_environment_t *)base_object, &processorEnclType);

    if((status == FBE_STATUS_OK) &&
        (processorEnclType == DPE_ENCLOSURE_TYPE))
    {
        phys_location.bus = 0;
        phys_location.enclosure = 0;
        phys_location.slot = 0;
        fbe_module_mgmt_handle_connector_status_change(module_mgmt, phys_location);
    }

    return FBE_LIFECYCLE_STATUS_DONE;
}
/******************************************************
 * end fbe_module_mgmt_discover_module_cond_function() 
 ******************************************************/


/*!**************************************************************
 * fbe_module_mgmt_configure_ports_cond_function()
 ****************************************************************
 * @brief
 *  This function goes through the current port configuration and
 *  assigns a logical number to any ports that do not have a logical
 *  FE/BE number assigned to them.  It then writes the configuration
 *  to persistent storage.
 *
 * @param object_handle - This is the object handle, or in our
 * case the module_mgmt.
 * @param packet_p - The packet arriving from the monitor
 * scheduler.
 *
 * @return fbe_status_t - The status of the call to
 *                        fbe_lifecycle_class_const_verify()
 *                        for the module management's
 *                        constant lifecycle data.
 *
 * @author
 *  15-Sep-2010 - Created. bphilbin
 *
 ****************************************************************/
static fbe_lifecycle_status_t fbe_module_mgmt_configure_ports_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet)
{
    fbe_status_t status;
    fbe_module_mgmt_t *module_mgmt = (fbe_module_mgmt_t *) base_object;
    fbe_base_environment_t *base_env = (fbe_base_environment_t *) base_object;
    fbe_bool_t changes_made = FALSE;
    fbe_board_mgmt_misc_info_t misc_info;
    fbe_u8_t port_num;
    fbe_io_port_persistent_info_t *persistent_data_p;
    fbe_u8_t persistent_port_index;

    //If persist DB not disabled intentionally and not available then we are in service
    if( (base_env->persist_db_disabled != TRUE) && (base_env->isBootFlash == TRUE) )
    {
        fbe_base_object_trace(base_object,FBE_TRACE_LEVEL_INFO,FBE_TRACE_MESSAGE_ID_INFO,
                          "%s service mode detected, no persistent storage access.\n",
                              __FUNCTION__);

        status = fbe_lifecycle_clear_current_cond(base_object);
        if(status != FBE_STATUS_OK)
        {
            fbe_base_object_trace(base_object, 
                                      FBE_TRACE_LEVEL_ERROR,
                                      FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                      "%s failed to clear condition, status: 0x%X\n",
                                      __FUNCTION__, status);
        }
        return FBE_LIFECYCLE_STATUS_DONE;

    }

    fbe_base_object_trace(base_object,
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s Enter, attempting to configure ports.\n",
                          __FUNCTION__);


    status = fbe_lifecycle_clear_current_cond(base_object);
	if (status != FBE_STATUS_OK) {
		fbe_base_object_trace(base_object, 
								FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"%s can't clear current condition, status: 0x%X",
								__FUNCTION__, status);
	}

    /* Get the SP ID from board */
    status = fbe_api_board_get_misc_info(module_mgmt->board_object_id, &misc_info);
	if (status != FBE_STATUS_OK)
	{
	    /* report the problem in getting the correct SP ID */
        fbe_base_object_trace((fbe_base_object_t *) module_mgmt, 
                                 FBE_TRACE_LEVEL_INFO,
                                 FBE_TRACE_MESSAGE_ID_INFO,
                                 "%s error in getting SP ID status: %d\n",
                                            __FUNCTION__, status);
        misc_info.spid = 0; /* Assume SPA in this case ?*/
	}

    if(module_mgmt->port_persist_enabled)
    {
        /* Based on the configuration we have now assign BE# and FE# to the ports. */
        fbe_module_mgmt_assign_port_subrole(module_mgmt, misc_info.spid);
        changes_made = fbe_module_mgmt_assign_port_logical_numbers(module_mgmt, misc_info.spid);
        /* Write this configuration out to persistent storage. */
        if(changes_made)
        {

            module_mgmt->configuration_change_made = FBE_TRUE;

            // disabling file based persistence to instead use persistent service
            
            persistent_port_index = 0;    
            persistent_data_p = (fbe_io_port_persistent_info_t *) ((fbe_base_environment_t *)module_mgmt)->my_persistent_data;
            for(port_num = 0; port_num < FBE_ESP_IO_PORT_MAX_COUNT; port_num++)
            {
                /* This is not a 1 to 1 mapping, we have a lot of possible port data but only a certain number of ports that can be
                 * configured.  This will make use of all persistent data slots when possible.
                 */
                if( ((module_mgmt->io_port_info[port_num].port_physical_info.port_comp_info[module_mgmt->local_sp].present) ||
                     (module_mgmt->io_port_info[port_num].port_logical_info[module_mgmt->local_sp].port_configured_info.logical_num != INVALID_PORT_U8)) &&
                    (module_mgmt->io_port_info[port_num].port_logical_info[module_mgmt->local_sp].port_configured_info.iom_group != FBE_IOM_GROUP_UNKNOWN))
                    
                {
                    fbe_copy_memory(&persistent_data_p[persistent_port_index],
                                &module_mgmt->io_port_info[port_num].port_logical_info[module_mgmt->local_sp].port_configured_info,
                                sizeof(fbe_io_port_persistent_info_t));
                    persistent_port_index++;
                }
            }

            //Write to persist DB if it is not disabled in the registry
            if( base_env->persist_db_disabled != TRUE )
            {
                status = fbe_base_env_write_persistent_data((fbe_base_environment_t *)module_mgmt);
                module_mgmt->port_configuration_loaded = FBE_TRUE;
                if(status != FBE_STATUS_OK)
                {
                    fbe_base_object_trace(base_object, 
                                      FBE_TRACE_LEVEL_ERROR,
                                      FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                      "%s persistent write error, status: 0x%X",
                                      __FUNCTION__, status);
                }
            }
        }
    }

    // If persist DB disabled then keep registry flag untouched
    // to continue loading ports config from registry at each boot
    // and do not force reboot to avoid infinite reboot loop

    if( base_env->persist_db_disabled != TRUE )
    {
        //  Clear the registry flag to load config from persist DB at next boot.
        fbe_module_mgmt_set_persist_port_info(module_mgmt, FALSE);

        if(changes_made)
        {
            // Set a flag to kick off a reboot when we reach the activate state.
            module_mgmt->reboot_sp = REBOOT_LOCAL;
        }
    }

    return FBE_LIFECYCLE_STATUS_DONE;

}
/******************************************************
 * end fbe_module_mgmt_configure_ports_cond_function() 
 ******************************************************/


/*!**************************************************************
 * fbe_module_mgmt_conversion_cond_function()
 ****************************************************************
 * @brief
 *  This function kicks off the process that goes through the 
 *  current port configuration and relocates ports as needed 
 *  to support the new platform.
 *
 * @param base_object - This is the object context.
 * @param packet_p - The packet arriving from the monitor
 * scheduler.
 *
 * @return fbe_status_t - The status of the call to
 *                        fbe_lifecycle_class_const_verify()
 *                        for the module management's
 *                        constant lifecycle data.
 *
 * @author
 *  03-March-2011 - Created. bphilbin
 *
 ****************************************************************/
static fbe_lifecycle_status_t fbe_module_mgmt_conversion_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet)
{
    fbe_status_t status;
    fbe_module_mgmt_t *module_mgmt = (fbe_module_mgmt_t *) base_object;

    fbe_base_object_trace(base_object,
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s in-family conversion type: %s.\n",
                          __FUNCTION__, fbe_module_mgmt_conversion_type_to_string(module_mgmt->conversion_type));


    status = fbe_lifecycle_clear_current_cond(base_object);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace(base_object, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s Failed to clear condition, status: 0x%X",
                              __FUNCTION__, status);
    }

    if(module_mgmt->conversion_type == FBE_MODULE_MGMT_CONVERSION_HCL_TO_SENTRY)
    {
        fbe_module_mgmt_convert_hellcat_lite_to_sentry(module_mgmt);
    }
    else if(module_mgmt->conversion_type == FBE_MODULE_MGMT_CONVERSION_SENTRY_TO_ARGONAUT)
    {
        fbe_module_mgmt_convert_sentry_to_argonaut(module_mgmt);
    }

    return FBE_LIFECYCLE_STATUS_DONE;
}



/*!**************************************************************
 * fbe_module_mgmt_slic_upgrade_cond_function()
 ****************************************************************
 * @brief
 *  This function goes through the current port configuration and
 *  upgrades the persistent IO module groups if upgraded hardware
 *  is detected.
 *
 * @param object_handle - This is the object handle, or in our
 * case the module_mgmt.
 * @param packet_p - The packet arriving from the monitor
 * scheduler.
 *
 * @return fbe_status_t - The status of the call to
 *                        fbe_lifecycle_class_const_verify()
 *                        for the module management's
 *                        constant lifecycle data.
 *
 * @author
 *  11-Nov-2010 - Created. bphilbin
 *
 ****************************************************************/
static fbe_lifecycle_status_t fbe_module_mgmt_slic_upgrade_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet)
{
    fbe_status_t status;
    fbe_module_mgmt_t *module_mgmt = (fbe_module_mgmt_t *) base_object;
    fbe_bool_t changes_made = FALSE;
    fbe_module_mgmt_upgrade_type_t slic_upgrade_type;
    fbe_u8_t port_num;
    fbe_io_port_persistent_info_t *persistent_data_p;
    fbe_u8_t persistent_port_index;

    fbe_base_object_trace(base_object,
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s Enter, attempting to upgrade ports.\n",
                          __FUNCTION__);


    status = fbe_lifecycle_clear_current_cond(base_object);
	if (status != FBE_STATUS_OK) {
		fbe_base_object_trace(base_object, 
								FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"%s can't clear current condition, status: 0x%X",
								__FUNCTION__, status);
	}

    /* Based on the configuration we have now assign BE# and FE# to the ports. */
    changes_made = fbe_module_mgmt_upgrade_ports(module_mgmt);
    /* Write this configuration out to persistent storage. */

    if(changes_made)
    {

        fbe_base_object_trace(base_object, 
                                FBE_TRACE_LEVEL_INFO,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s SLIC upgrade changes were made, updating the DB.\n",
                                __FUNCTION__);

        module_mgmt->configuration_change_made = FBE_TRUE;

        // disabling file based persistence to instead use persistent service
        
        persistent_port_index = 0;    
        persistent_data_p = (fbe_io_port_persistent_info_t *) ((fbe_base_environment_t *)module_mgmt)->my_persistent_data;
        for(port_num = 0; port_num < FBE_ESP_IO_PORT_MAX_COUNT; port_num++)
        {
            /* This is not a 1 to 1 mapping, we have a lot of possible port data but only a certain number of ports that can be
             * configured.  This will make use of all persistent data slots when possible.
             */
            if( (module_mgmt->io_port_info[port_num].port_physical_info.port_comp_info[module_mgmt->local_sp].present) ||
                (module_mgmt->io_port_info[port_num].port_logical_info[module_mgmt->local_sp].port_configured_info.logical_num != INVALID_PORT_U8))
            {
                fbe_copy_memory(&persistent_data_p[persistent_port_index],
                            &module_mgmt->io_port_info[port_num].port_logical_info[module_mgmt->local_sp].port_configured_info,
                            sizeof(fbe_io_port_persistent_info_t));
                persistent_port_index++;
            }
        }
        status = fbe_base_env_write_persistent_data((fbe_base_environment_t *)module_mgmt);
        if(status != FBE_STATUS_OK)
        {
            fbe_base_object_trace(base_object, 
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s persistent write error, status: 0x%X",
                                  __FUNCTION__, status);
        }
    }

    /*
     *  Update the registry flag.
     */
    slic_upgrade_type = FBE_MODULE_MGMT_SLIC_UPGRADE_PEER_SP;
    fbe_module_mgmt_set_slics_marked_for_upgrade(&slic_upgrade_type);

    /* Reboot the SP if the configuration has changed. */
    if(changes_made)
    {
        // Set a flag to kick off a reboot when we reach the activate state.
        module_mgmt->reboot_sp = REBOOT_LOCAL;
    }

    // For we may skipped some port and module check due to Slic Upgrade,
    // We got to check module and port status.
    fbe_module_mgmt_check_all_module_and_port_states(module_mgmt);

    return FBE_LIFECYCLE_STATUS_DONE;

}
/******************************************************
 * end fbe_module_mgmt_slic_upgrade_cond_function() 
 ******************************************************/



/*!**************************************************************
 * fbe_module_mgmt_check_registry_cond_function()
 ****************************************************************
 * @brief
 *  This function checks the current port configuration in persistent
 *  storage and checks it against the configuration in the Registry.
 *
 * @param object_handle - This is the object handle, or in our
 * case the module_mgmt.
 * @param packet_p - The packet arriving from the monitor
 * scheduler.
 *
 * @return fbe_status_t - The status of the call to
 *                        fbe_lifecycle_class_const_verify()
 *                        for the module management's
 *                        constant lifecycle data.
 *
 * @author
 *  11-Dec-2010 - Created. bphilbin
 *
 ****************************************************************/
static fbe_lifecycle_status_t fbe_module_mgmt_check_registry_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet)
{
    fbe_status_t status;
    fbe_module_mgmt_t *module_mgmt = (fbe_module_mgmt_t *) base_object;
    fbe_base_environment_t *base_env = (fbe_base_environment_t *) base_object;
    fbe_bool_t changes_made = FALSE;
    fbe_bool_t disable_update_info = 0;
    SP_ID sp;

    fbe_base_object_trace(base_object,
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s Enter, checking if CPD entries need to be updated in the registry.\n",
                          __FUNCTION__);


    status = fbe_lifecycle_clear_current_cond(base_object);

    /*
     * Verify that the PortParams entries in the Registry match what we have 
     * configured in persistent storage.
     */
    fbe_module_mgmt_get_disable_reg_update_info(module_mgmt, &disable_update_info);
    if(disable_update_info == 1)
    {
        fbe_base_object_trace(base_object,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s disable reg update key set, skip registry update.\n",
                              __FUNCTION__);

        return FBE_LIFECYCLE_STATUS_DONE;
    }

    changes_made |= fbe_module_mgmt_reg_check_and_update_registry(module_mgmt);
    module_mgmt->configuration_change_made = changes_made;

    if(module_mgmt->port_affinity_mode)
    {
        changes_made |=fbe_module_mgmt_check_and_update_interrupt_affinities(module_mgmt, FALSE, module_mgmt->port_affinity_mode, changes_made);
    }

    fbe_base_object_trace(base_object,
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s Changes_made:%d reboot_sp:%d.\n",
                          __FUNCTION__, changes_made, module_mgmt->reboot_sp);

    if(changes_made || (module_mgmt->reboot_sp != REBOOT_NONE))
    {
        

        if( (module_mgmt->reboot_sp == REBOOT_PEER) ||
            (module_mgmt->reboot_sp == REBOOT_BOTH) )
        {
            /* 
             * Reboot the peer SP
             */
            sp = (module_mgmt->local_sp == SP_A) ? SP_B: SP_A;
            fbe_base_object_trace(base_object,
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s Rebooting Peer SP %d.\n",
                          __FUNCTION__, sp);
            fbe_base_env_reboot_sp((fbe_base_environment_t *) module_mgmt, sp, FALSE, FALSE, FBE_FALSE);
        }

        if(module_mgmt->reboot_sp == REBOOT_BOTH)
        {
            // the reboot both case happens during runtime so we can't use newSP to reboot the sp
            //flush the registry
            fbe_flushRegistry();

            fbe_module_mgmt_handle_file_write_completion(base_object, FBE_MODULE_MGMT_FLUSH_THREAD_DELAY_IN_MSECS);

            fbe_base_object_trace(base_object,
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s: Rebooting Local SP.\n",
                          __FUNCTION__);
            //reboot this sp
            fbe_base_env_reboot_sp((fbe_base_environment_t *) module_mgmt, module_mgmt->local_sp, FALSE, FALSE, FBE_TRUE);
        }

       if( (module_mgmt->reboot_sp == REBOOT_LOCAL) ||
           (changes_made))
       {
           //flush the registry
            fbe_flushRegistry();

            fbe_base_object_trace(base_object,
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s: Setting reboot for Local SP.\n",
                          __FUNCTION__);

            fbe_event_log_write(ESP_WARN_IO_PORT_CONFIGURATION_CHANGED,
                                NULL, 0, 
                                NULL);
            //reboot this sp
            fbe_module_mgmt_set_reboot_required();
       }

        fbe_module_mgmt_check_all_module_and_port_states(module_mgmt);
    }
    else
    {
        if( base_env->persist_db_disabled == TRUE )
        {
            // If persist DB disabled then we load config from registry but lose port states after each boot
            // So update states unconditionally
            fbe_module_mgmt_check_all_module_and_port_states(module_mgmt);
        }
    }

    return FBE_LIFECYCLE_STATUS_DONE;
}

/*!**************************************************************
 * fbe_module_mgmt_get_bm_lcc_info()
 ****************************************************************
 * @brief
 *  This function get lcc info from physical package for the base module on the specified side. 
 *  Jetfire BEM is also treated as lcc in physical package enclosure object.
 *
 * @param module_mgmt - object handle
 * @param pLocation - the pointer to the BEM location (SP, SLOT based). 
 * @param pLccInfo - pointer to save Lcc info
 *
 * @return fbe_status_t - FBE Status
 *
 * @author
 *  31-Oct-2012 - Created. Rui Chang
 *  21-Jan-2012 PHE - Added the parameter to pass in the SP side for the Base module. 
 *
 ****************************************************************/
static fbe_status_t 
fbe_module_mgmt_get_bm_lcc_info(fbe_module_mgmt_t *module_mgmt,
                                 SP_ID sp,
                                 fbe_lcc_info_t * pLccInfo)
{
    fbe_device_physical_location_t          enclBasedLocation = {0};
    fbe_status_t                            status = FBE_STATUS_OK;

    /* The input pLocation points to the location which is sp/slot based for the BEM.
     * fbe_api_enclosure_get_lcc_info takes the location with bus/enclosure/slot based.
     * so we need to do the conversion here.
     */
    enclBasedLocation.bus = 0;
    enclBasedLocation.enclosure = 0;
    enclBasedLocation.slot = (sp == SP_A) ? 0 : 1;

    status = fbe_api_enclosure_get_lcc_info(&enclBasedLocation, pLccInfo);
    if (status != FBE_STATUS_OK) 
    {
        fbe_base_object_trace((fbe_base_object_t *)module_mgmt, 
                                  FBE_TRACE_LEVEL_WARNING,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s Error getting %s BM's LCC info, status: 0x%X.\n",
                                  __FUNCTION__,
                                  (sp == SP_A)? "SPA" : "SPB",
                                  status);
        return status;
    }

    return FBE_STATUS_OK;

}
/**************************************
 * end fbe_module_mgmt_get_bm_lcc_info() 
 **************************************/  

/*!**************************************************************
 * fbe_module_mgmt_handle_new_module()
 ****************************************************************
 * @brief
 *  This function simply validates the constant lifecycle data
 *  that is associated with the module management.
 *
 * @param object_handle - This is the object handle, or in our
 * case the module_mgmt.
 * @param packet_p - The packet arriving from the monitor
 * scheduler.
 * @param object_id - Object ID of the module that just got
 * added
 *
 * @return fbe_status_t - FBE Status
 *
 * @author
 *  11-Mar-2010 - Created. Nayana Chaudhari
 *
 ****************************************************************/
static fbe_status_t 
fbe_module_mgmt_handle_new_module(fbe_base_object_t * base_object, fbe_packet_t * packet, fbe_object_id_t object_id)
{
    /* Do module addition specific
       functions here e.g. Validation of module, kicking the 
       process of Resume PROM read etc*/
    return FBE_STATUS_OK;

}
/**************************************
 * end fbe_module_mgmt_handle_new_module() 
 **************************************/     

fbe_status_t fbe_module_mgmt_log_io_module_event(fbe_module_mgmt_t * module_mgmt,
                                            fbe_u64_t deviceType,
                                            fbe_device_physical_location_t *pLocation,
                                            fbe_board_mgmt_io_comp_info_t *new_io_comp_info,
                                            fbe_board_mgmt_io_comp_info_t *old_io_comp_info)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_u8_t                                deviceStr[FBE_DEVICE_STRING_LENGTH];

    // generate the device string (used for tracing & events)  
    status = fbe_base_env_create_device_string(deviceType,
                                               pLocation,
                                               &deviceStr[0],
                                               FBE_DEVICE_STRING_LENGTH);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)module_mgmt, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                              "%s, Failed to create device string.\n", 
                              __FUNCTION__); 
    
        return status;
    }
    
    if (new_io_comp_info->envInterfaceStatus != old_io_comp_info->envInterfaceStatus)
    {
        if (new_io_comp_info->envInterfaceStatus == FBE_ENV_INTERFACE_STATUS_XACTION_FAIL)
        {
            fbe_event_log_write(ESP_ERROR_ENV_INTERFACE_FAILURE_DETECTED,
                        NULL, 0,
                        "%s %s", 
                        &deviceStr[0],
                        SmbTranslateErrorString(new_io_comp_info->transactionStatus));
        }
        else if (new_io_comp_info->envInterfaceStatus != FBE_ENV_INTERFACE_STATUS_GOOD)
        {
            fbe_event_log_write(ESP_ERROR_ENV_INTERFACE_FAILURE_DETECTED,
                                NULL, 0,
                                "%s %s", 
                                &deviceStr[0],
                                fbe_base_env_decode_env_status(new_io_comp_info->envInterfaceStatus));
        }
        else
        {
            fbe_event_log_write(ESP_INFO_ENV_INTERFACE_FAILURE_CLEARED,
                                NULL, 0,
                                "%s", 
                                &deviceStr[0]);
        }
    }

    return status;
}

/*!**************************************************************
 * fbe_module_mgmt_get_new_module_info()
 ****************************************************************
 * @brief
 *  This function gets Board Object ID (it interfaces with MODULE)
 *  and adds them to the queue to be processed later
 *
 * @param object_handle - This is the object handle, or in our
 * case the module_mgmt.
 * @param packet_p - The packet arriving from the monitor
 * @param slot - slot of io module
 * @param device_type - device type
 * scheduler.
 *
 * @return fbe_status_t - The status of the call to
 *                        fbe_lifecycle_class_const_verify()
 *                        for the MODULE management's
 *                        constant lifecycle data.
 *
 * @author
 *  14-April-2010:  Created. bphilbin 
 *
 ****************************************************************/
static fbe_status_t 
fbe_module_mgmt_get_new_module_info(fbe_module_mgmt_t *module_mgmt,
                                       SP_ID    sp,
                                       fbe_u8_t slot,
                                       fbe_u64_t device_type)
{
    fbe_base_object_t                       *base_object = (fbe_base_object_t *)module_mgmt;
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_board_mgmt_io_comp_info_t           new_io_comp_info = {0};
    fbe_board_mgmt_io_comp_info_t           old_io_comp_info = {0};
    fbe_module_slic_type_t                  slic_type = FBE_SLIC_TYPE_UNKNOWN;
    fbe_u32_t                               index = 0;
    fbe_device_physical_location_t          location = {0};
    fbe_device_physical_location_t          bmLocation = {0};
    fbe_bool_t                                 bemCoolingFault=FALSE;
    fbe_object_id_t                         object_id;
    FBE_IO_MODULE_PROTOCOL                  iom_protocol;
    fbe_board_mgmt_misc_info_t              miscInfo;
    fbe_u8_t                                deviceStr[FBE_DEVICE_STRING_LENGTH];

    new_io_comp_info.associatedSp = sp;
    new_io_comp_info.slotNumOnBlade = slot;

    location.sp = sp;
    location.slot = slot;

    index = fbe_module_mgmt_convert_device_type_and_slot_to_index(device_type, slot);

    switch (device_type)
    {
        case FBE_DEVICE_TYPE_IOMODULE:
        {
             status = fbe_api_board_get_iom_info(module_mgmt->board_object_id, 
                                                 &new_io_comp_info);

            if (status != FBE_STATUS_OK) 
            {
                fbe_base_object_trace(base_object, 
                                        FBE_TRACE_LEVEL_ERROR,
                                        FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                        "%s Error in getting new_module_info, status: 0x%X\n",
                                        __FUNCTION__, status);
                return status;
            }

            /* Save the Old IO component info */
            old_io_comp_info = module_mgmt->io_module_info[index].physical_info[sp].module_comp_info;

            fbe_copy_memory(&(module_mgmt->io_module_info[index].physical_info[sp].module_comp_info), 
                &new_io_comp_info, sizeof(fbe_board_mgmt_io_comp_info_t));

            /* Process the Resume prom read */
            status = fbe_module_mgmt_resume_prom_handle_io_comp_status_change(module_mgmt, 
                                                                              &location, 
                                                                              device_type, 
                                                                              &new_io_comp_info, 
                                                                              &old_io_comp_info);
            if(status != FBE_STATUS_OK)
            {
                fbe_base_object_trace(base_object, 
                                      FBE_TRACE_LEVEL_ERROR,
                                      FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                      "RP: resume_prom_handle_io_comp_status_change failed, SP: %d, Slot: %d.\n",
                                      location.sp, location.slot);
            }
        }
        break;

        case FBE_DEVICE_TYPE_BACK_END_MODULE:
        {
             status = fbe_api_board_get_bem_info(module_mgmt->board_object_id, 
                                                      &new_io_comp_info);
            if (status != FBE_STATUS_OK) 
            {
                fbe_base_object_trace(base_object, 
                                        FBE_TRACE_LEVEL_ERROR,
                                        FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                        "%s Error in getting new_module_info, status: 0x%X\n",
                                        __FUNCTION__, status);
                return status;
            }

            if (location.sp == module_mgmt->base_environment.spid)
            {
                /* Get BEM internal fan status */;
                status = fbe_api_get_enclosure_object_id_by_location(0, 0, &object_id);
                if (status != FBE_STATUS_OK) 
                {
                    fbe_base_object_trace(base_object, 
                                            FBE_TRACE_LEVEL_ERROR,
                                            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                            "%s Error in getting DAE0 Encl Object ID, status: 0x%X\n",
                                            __FUNCTION__, status);
                }
    
                status = fbe_api_enclosure_get_chassis_cooling_status(object_id, &bemCoolingFault);
                if (status != FBE_STATUS_OK) 
                {
                    fbe_base_object_trace(base_object, 
                                            FBE_TRACE_LEVEL_ERROR,
                                            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                            "%s Error in getting Base Module cooling status, status: 0x%X\n",
                                            __FUNCTION__, status);
                }
    
                new_io_comp_info.internalFanFault = bemCoolingFault;
            }

            /* Get the LCC info. BEM is also treated as a LCC in eses enclosure object, so we need to get that info */
            status = fbe_module_mgmt_get_bm_lcc_info(module_mgmt, location.sp, &new_io_comp_info.lccInfo);
            if (status != FBE_STATUS_OK) 
            {
                fbe_base_object_trace(base_object, 
                                  FBE_TRACE_LEVEL_WARNING,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s Error getting BM's LCC(SP %d, Slot %d) info, status: 0x%X.\n",
                                  __FUNCTION__,
                                  location.sp,
                                  location.slot,
                                  status);
                return status;
            }

            /* Save the Old IO component info */
            old_io_comp_info = module_mgmt->io_module_info[index].physical_info[sp].module_comp_info;

            /* Log the event for change */
            fbe_module_mgmt_log_bm_lcc_event(module_mgmt,
                                          &new_io_comp_info, 
                                          &old_io_comp_info);

            fbe_base_object_trace(base_object, 
                                    FBE_TRACE_LEVEL_INFO,
                                    FBE_TRACE_MESSAGE_ID_INFO,
                                    "%s Got Base Module info for SP %d Slot:%d UID:0x%x Ins:%d\n",
                                    __FUNCTION__, sp, slot, new_io_comp_info.uniqueID, new_io_comp_info.inserted);

            fbe_copy_memory(&(module_mgmt->io_module_info[index].physical_info[sp].module_comp_info), 
                &new_io_comp_info, sizeof(fbe_board_mgmt_io_comp_info_t));

            /* Process CDES firmware upgrade */
            status = fbe_module_mgmt_fup_handle_bem_status_change(module_mgmt, 
                                                        &location, 
                                                        &new_io_comp_info.lccInfo, 
                                                        &old_io_comp_info.lccInfo);
            if(status != FBE_STATUS_OK) 
            {
                fbe_base_object_trace((fbe_base_object_t *)module_mgmt, 
                                      FBE_TRACE_LEVEL_WARNING,
                                      FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                      "FUP:BM(SP %d, Slot %d) fbe_module_mgmt_fup_handle_bem_status_change failed, status 0x%X.\n",
                                      location.sp,
                                      location.slot,
                                      status);
            }

            /* Process the Resume prom read */
            status = fbe_module_mgmt_resume_prom_handle_io_comp_status_change(module_mgmt, 
                                                                              &location, 
                                                                              device_type, 
                                                                              &new_io_comp_info, 
                                                                              &old_io_comp_info);
            if(status != FBE_STATUS_OK)
            {
                fbe_base_object_trace(base_object, 
                                      FBE_TRACE_LEVEL_ERROR,
                                      FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                      "RP: resume_prom_handle_io_comp_status_change failed, SP: %d, Slot: %d.\n",
                                      location.sp, location.slot);
            }
        }
        break;

        case FBE_DEVICE_TYPE_MEZZANINE:
        {
            status = fbe_api_board_get_mezzanine_info(module_mgmt->board_object_id, 
                                                      &new_io_comp_info);
            if (status != FBE_STATUS_OK) 
            {
                fbe_base_object_trace(base_object, 
                                        FBE_TRACE_LEVEL_ERROR,
                                        FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                        "%s Error in getting mezzanine new_module_info, status: 0x%X\n",
                                        __FUNCTION__, status);
                return status;
            }

            /* Save the Old IO component info */
            old_io_comp_info = module_mgmt->io_module_info[index].physical_info[sp].module_comp_info;

            /* Power status is not reported for the mezzanine, just set it to good */
            new_io_comp_info.powerStatus = FBE_POWER_STATUS_POWER_ON;

            fbe_copy_memory(&(module_mgmt->io_module_info[index].physical_info[sp].module_comp_info), 
                &new_io_comp_info, sizeof(fbe_board_mgmt_io_comp_info_t));

            /* Process the Resume prom read */
            status = fbe_module_mgmt_resume_prom_handle_io_comp_status_change(module_mgmt, 
                                                                              &location, 
                                                                              device_type, 
                                                                              &new_io_comp_info, 
                                                                              &old_io_comp_info);
            if(status != FBE_STATUS_OK)
            {
                fbe_base_object_trace(base_object, 
                                      FBE_TRACE_LEVEL_ERROR,
                                      FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                      "RP: resume_prom_handle_io_comp_status_change failed, SP: %d, Slot: %d.\n",
                                      location.sp, location.slot);
            }
        }
        break;

        case FBE_DEVICE_TYPE_MISC:
            // Get the additional BM info (power ECB status) 
            status = fbe_api_board_get_misc_info(module_mgmt->board_object_id, &miscInfo);
            if (status != FBE_STATUS_OK) 
            {
                fbe_base_object_trace(base_object, 
                                  FBE_TRACE_LEVEL_WARNING,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s Error getting MiscInfo info, status: 0x%X.\n",
                                  __FUNCTION__,
                                  status);
                return status;
            }
            else
            {
                old_io_comp_info.localPowerECBFault = module_mgmt->io_module_info[index].physical_info[sp].module_comp_info.localPowerECBFault;
                old_io_comp_info.peerPowerECBFault = module_mgmt->io_module_info[index].physical_info[sp].module_comp_info.peerPowerECBFault;
                new_io_comp_info.localPowerECBFault = miscInfo.localPowerECBFault;
                new_io_comp_info.peerPowerECBFault = miscInfo.peerPowerECBFault;

                fbe_base_object_trace(base_object, 
                                      FBE_TRACE_LEVEL_INFO,
                                      FBE_TRACE_MESSAGE_ID_INFO,
                                      "%s, powerEcbFault local %d %d, peer %d %d.\n",
                                      __FUNCTION__,
                                      old_io_comp_info.localPowerECBFault,
                                      new_io_comp_info.localPowerECBFault,
                                      old_io_comp_info.peerPowerECBFault,
                                      new_io_comp_info.peerPowerECBFault);
                if ((new_io_comp_info.localPowerECBFault != old_io_comp_info.localPowerECBFault) ||
                    (new_io_comp_info.peerPowerECBFault != old_io_comp_info.peerPowerECBFault))
                {
                    fbe_zero_memory(&bmLocation, sizeof(fbe_device_physical_location_t));
                    bmLocation.sp = module_mgmt->base_environment.spid;
                    status = fbe_base_env_create_device_string(FBE_DEVICE_TYPE_BACK_END_MODULE, 
                                                               &bmLocation, 
                                                               &(deviceStr[0]), 
                                                               FBE_DEVICE_STRING_LENGTH);

                    if ((!old_io_comp_info.localPowerECBFault && !old_io_comp_info.peerPowerECBFault) &&
                        (new_io_comp_info.localPowerECBFault || new_io_comp_info.peerPowerECBFault))
                    {
                        fbe_event_log_write(ESP_ERROR_IO_MODULE_FAULTED, 
                                            NULL, 0, 
                                            "%s %s", 
                                            &deviceStr[0],
                                            "Power ECB Fault");
                    }
                    else if ((old_io_comp_info.localPowerECBFault || old_io_comp_info.peerPowerECBFault) &&
                        (!new_io_comp_info.localPowerECBFault && !new_io_comp_info.peerPowerECBFault))
                    {
                        fbe_base_object_trace(base_object, 
                                              FBE_TRACE_LEVEL_INFO,
                                              FBE_TRACE_MESSAGE_ID_INFO,
                                              "%s, powerEcbFaults cleared.\n",
                                              __FUNCTION__);
                    }
                }
            }
            module_mgmt->io_module_info[index].physical_info[sp].module_comp_info.localPowerECBFault = new_io_comp_info.localPowerECBFault;
            module_mgmt->io_module_info[index].physical_info[sp].module_comp_info.peerPowerECBFault = new_io_comp_info.peerPowerECBFault;

            return FBE_STATUS_OK;
            break;

        default:
        {
            fbe_base_object_trace(base_object, 
                        FBE_TRACE_MESSAGE_ID_INFO,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s Invalid device_type, 0x%llX\n",
                        __FUNCTION__, device_type);

            return status;
            break;
        }
    }

    status = fbe_module_mgmt_log_io_module_event(module_mgmt, device_type, &location, &new_io_comp_info, &old_io_comp_info);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace(base_object, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s: log %s event failed, status: %d.\n",
                              __FUNCTION__, fbe_base_env_decode_device_type(device_type), status);
    }

    /* 
     * Fill in some required physical data and derive some logical state information about the
     * io module now that we have updated the information.
     */
    slic_type = fbe_module_mgmt_derive_slic_type(module_mgmt, module_mgmt->io_module_info[index].physical_info[sp].module_comp_info.uniqueID);
    fbe_module_mgmt_derive_label_name(module_mgmt, module_mgmt->io_module_info[index].physical_info[sp].module_comp_info.uniqueID, module_mgmt->io_module_info[index].physical_info[sp].label_name);

    fbe_module_mgmt_set_slic_type(module_mgmt, sp, index, slic_type);
    fbe_module_mgmt_set_device_type(module_mgmt, sp, index, device_type);
    iom_protocol = fbe_module_mgmt_derive_io_module_protocol(module_mgmt, module_mgmt->io_module_info[index].physical_info[sp].module_comp_info.uniqueID);
    fbe_module_mgmt_set_io_module_protocol(module_mgmt, sp, index, iom_protocol);

    fbe_base_object_trace(base_object, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                          "%s found %s in SP:%d, Slot:%d, is %s\n",
                          __FUNCTION__, 
                          fbe_module_mgmt_slic_type_to_string(fbe_module_mgmt_get_slic_type(module_mgmt, sp, index)),
                          sp, slot,
                          fbe_module_mgmt_is_iom_inserted(module_mgmt, sp, index)?"inserted":"removed");

    fbe_module_mgmt_check_module_state(module_mgmt, sp, index);
    
   
    return status;
}

/******************************************************
 * end fbe_module_mgmt_get_new_module_info() 
 ******************************************************/

/*!**************************************************************
 * fbe_module_mgmt_get_new_io_port_info()
 ****************************************************************
 * @brief
 *  This function gets Board Object ID (it interfaces with MODULE)
 *  and adds them to the queue to be processed later for io port
 *  info
 *
 * @param object_handle - This is the object handle, or in our
 * case the module_mgmt.
 * @param packet_p - The packet arriving from the monitor
 * @param port - port number
 * scheduler.
 *
 * @return fbe_status_t - The status of the call to
 *                        fbe_lifecycle_class_const_verify()
 *                        for the MODULE management's
 *                        constant lifecycle data.
 *
 * @author
 *  19-May-2010:  Created. Nayana Chaudhari 
 *
 ****************************************************************/
static fbe_status_t fbe_module_mgmt_get_new_io_port_info(fbe_module_mgmt_t *module_mgmt,
                                                         SP_ID sp_id,
                                                         fbe_u8_t slot,
                                                         fbe_u8_t port,
                                                         fbe_u64_t device_type)
{
    fbe_base_object_t                       *base_object = (fbe_base_object_t *)module_mgmt;
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_board_mgmt_io_port_info_t           port_comp_info;
    fbe_u8_t                                port_index;
    fbe_u32_t                               io_mod_count_per_blade;
    fbe_bool_t                              configuration_change = FALSE;
    fbe_iom_group_t                         iom_group = FBE_IOM_GROUP_UNKNOWN;
    fbe_u32_t                               index = 0;
    HW_MODULE_TYPE                          uniqueID;
    fbe_u32_t                               combined_port_count = 0, temp_port = 0;

   
    port_comp_info.associatedSp = sp_id;
    port_comp_info.slotNumOnBlade = slot;
    port_comp_info.portNumOnModule = port;
    port_comp_info.deviceType = device_type;

    status = fbe_api_board_get_io_port_info(module_mgmt->board_object_id, &port_comp_info);

    if (status != FBE_STATUS_OK) 
    {
        fbe_base_object_trace(base_object, 
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s Error in getting io_port_info, status: 0x%X, for slot %d, port %d, device_type %lld\n",
                                __FUNCTION__, status, slot, port, device_type);
        return status;
    }

    status = fbe_api_board_get_io_module_count_per_blade(module_mgmt->board_object_id, &io_mod_count_per_blade);
    if (status != FBE_STATUS_OK) 
    {
        fbe_base_object_trace(base_object, 
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s Error in getting the io_module_count_per_blade, status: 0x%X\n",
                                __FUNCTION__, status);
        return status;
    }

    /* 
     * The external APIs use device type and slot to reference IO modules.  Internally we simply use
     * a list of all io modules and index this with an io module number.  Also port information is
     * stored in a similar list fashion so we derive a port index from an io module number and a port
     * number on that io module.  Even ports that are not physically present in the system now are
     * accounted for in this list, so if we get an INVALID_PORT_U8 the iom_num and port_num are beyond
     * what we have sized for.
     */
    index = fbe_module_mgmt_convert_device_type_and_slot_to_index(device_type, slot);
    port_index = fbe_module_mgmt_get_io_port_index(index, port);

    if(port_index == INVALID_PORT_U8)
    {
        fbe_base_object_trace(base_object, 
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s Error Invalid port number: 0x%X\n",
                                __FUNCTION__, port_index);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if(module_mgmt->io_port_info[port_index].port_physical_info.port_comp_info[sp_id].present != 
       port_comp_info.present)
    {
        /*
         * The configuration for this port has recently changed, set the configuration change
         * flag so we know to update logical information about this port as well.
         */
        configuration_change = TRUE;

    }

    if(port_comp_info.protocol == IO_CONTROLLER_PROTOCOL_FIBRE)
    {
        // FC ports don't use SPECL to gather SFP data but they do gather SFP data.
        port_comp_info.SFPcapable = FBE_TRUE;
    }

    /* Copy the port info structure from the physical package into our own port information structure. */
    fbe_copy_memory(&(module_mgmt->io_port_info[port_index].port_physical_info.port_comp_info[sp_id]), 
        &port_comp_info, 
        sizeof(fbe_board_mgmt_io_port_info_t));

    // Determine the active PCI function for this port and save it
    fbe_module_mgmt_set_selected_pci_function(module_mgmt, port_index, sp_id);

    if(port_comp_info.protocol == IO_CONTROLLER_PROTOCOL_SAS)
    {
        /*
         * This is sas, the function field in the pci address is overloaded with the portal number
         * in the case of combined connectors we need to adjust the portal numbers to offset the combined
         * ports.
         */
        for(temp_port = 0; temp_port < port_comp_info.portal_number; temp_port++)
        {
            if(fbe_module_mgmt_is_port_configured_combined_port(module_mgmt,sp_id,index,temp_port))
            {
                combined_port_count++;
            }
        }
        if(combined_port_count > 0)
        {
            if(port_comp_info.portal_number < 2) 
            {
                // ports 0 and 1 
                port_comp_info.portal_number = 0;
                
            }
            else if(port_comp_info.portal_number == combined_port_count)
            {
                // ports 0 and 1 are already configured as a combined connector set the portal to 1 for ports 2 and 3
                port_comp_info.portal_number = 1;
            }
            else if((port_comp_info.portal_number == 2) && (combined_port_count < 2))
            {
                // ports 0 and 1 are not configured as combined connectors, set the portal to 2
                port_comp_info.portal_number = 2;
            }
            else if(port_comp_info.portal_number == 3)
            {
                /*
                 * Either ports 0 and 1 are configured as combined connectors or 2 and 3 are, either way the
                 * portal number is 2.
                 */
                port_comp_info.portal_number = 2;
            }
        }
        fbe_base_object_trace(base_object, 
                                FBE_TRACE_LEVEL_INFO,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s IOM:%d Port:%d Portal:%d CombinedPortCount:%d:\n",
                                __FUNCTION__, index, port, port_comp_info.portal_number, combined_port_count);
        
        module_mgmt->io_port_info[port_index].port_physical_info.selected_pci_function.function = port_comp_info.portal_number;
    }

    if(configuration_change == TRUE)
    {
        /*
         * If the logical number is not set, the port is not configured, go and derive
         * the port role and io module grouping based on the hardware now.
         */
        if(!fbe_module_mgmt_is_port_initialized(module_mgmt, sp_id, index, port))
        {
            fbe_ioport_role_t temp_port_role;

            temp_port_role = fbe_module_mgmt_derive_port_role(module_mgmt, sp_id, index, port);
            fbe_module_mgmt_set_port_role(module_mgmt, sp_id, index, port, temp_port_role);
            uniqueID = module_mgmt->io_module_info[index].physical_info[sp_id].module_comp_info.uniqueID;
            iom_group = fbe_module_mgmt_derive_iom_group(module_mgmt, sp_id, index, port, uniqueID);
            fbe_module_mgmt_set_iom_group(module_mgmt, sp_id, index, port, iom_group);

            // We discovered BE 0 here and it is not configured, set it now so other drivers can run.
            if(port_comp_info.boot_device == TRUE)
            {
                fbe_base_object_trace(base_object, 
                                FBE_TRACE_LEVEL_INFO,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s Boot Port Detected at IOM:%d Port:%d setting BE 0 here\n",
                                __FUNCTION__, index, port);
                fbe_module_mgmt_set_port_logical_number(module_mgmt, sp_id, index, port, 0);
                module_mgmt->boot_device_found = FBE_TRUE;
            }

        }
        
        if(port != INVALID_PORT_U8)
        {
            fbe_module_mgmt_check_port_state(module_mgmt, sp_id, index, port);
        }
        
    }

    return status;
}
/******************************************************
 * end fbe_module_mgmt_get_new_io_port_info() 
 ******************************************************/


/*!**************************************************************
 * fbe_module_mgmt_get_new_sfp_info()
 ****************************************************************
 * @brief
 *  This function gets gets the latest SFP status information 
 *  for the specified port.
 *
 * @param object_handle - This is the object handle, or in our
 * case the module_mgmt.
 * @param sp - sp id.
 * @param iom_num - io module index number.
 * @param port_num - Port number on that io module
 * @param port_object_id - object id for the specified port.
 *
 * @return fbe_status_t - The status of the call to
 *                        fbe_lifecycle_class_const_verify()
 *                        for the MODULE management's
 *                        constant lifecycle data.
 *
 * @author
 *  9-July-2010:  Created. bphilbin 
 *
 ****************************************************************/
static fbe_status_t fbe_module_mgmt_get_new_sfp_info(fbe_module_mgmt_t *module_mgmt, SP_ID sp, fbe_u8_t iom_num, 
                                                     fbe_u8_t port_num, fbe_object_id_t port_object_id)
{
    fbe_status_t        status = FBE_STATUS_OK;

    fbe_base_object_t   *base_object = (fbe_base_object_t *)module_mgmt;
    fbe_port_sfp_info_t port_sfp_info;
    fbe_u8_t            port_index;
    
    fbe_base_object_trace(base_object, 
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "ModMgmt: fetching SFP info for %s %d Port:%d\n",
                          fbe_module_mgmt_device_type_to_string(fbe_module_mgmt_get_device_type(module_mgmt, sp, iom_num)),
                          fbe_module_mgmt_get_slot_num(module_mgmt, sp, iom_num),
                          port_num);                                                          

    status = fbe_api_get_port_sfp_information(port_object_id, &port_sfp_info);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace(base_object, 
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s Error get_port_sfp_info for Port Object:%d failed with status: 0x%X\n",
                                __FUNCTION__, port_object_id, status);
        return status;
    }

    port_index = fbe_module_mgmt_get_io_port_index(iom_num, port_num);
    if(port_index != INVALID_PORT_U8)
    {
        fbe_module_mgmt_handle_sfp_state_change(module_mgmt, iom_num, port_num,
                                                &module_mgmt->io_port_info[port_index].port_physical_info.sfp_info, 
                                                &port_sfp_info);
    
        fbe_copy_memory(&(module_mgmt->io_port_info[port_index].port_physical_info.sfp_info), 
            &port_sfp_info, 
            sizeof(fbe_port_sfp_info_t));

        module_mgmt->io_port_info[port_index].port_physical_info.sfp_info.supported_protocols = 
            fbe_module_mgmt_derive_sfp_protocols(module_mgmt, module_mgmt->local_sp, iom_num, port_num);
        
        fbe_base_object_trace(base_object, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "ModMgmt: new_sfp_info %s %d, Port %d, PortObj:%d, SFP condition:%d, subcondtion:%d, inserted:%d\n",
                              fbe_module_mgmt_device_type_to_string(fbe_module_mgmt_get_device_type(module_mgmt, sp, iom_num)),
                              fbe_module_mgmt_get_slot_num(module_mgmt, sp, iom_num),
                              port_num,
                              port_object_id, 
                              port_sfp_info.condition_type,
                              port_sfp_info.condition_additional_info,
                              module_mgmt->io_port_info[port_index].port_physical_info.sfp_inserted);

        fbe_module_mgmt_check_for_combined_port(module_mgmt, iom_num, port_num);

    }
    else
    {
        fbe_base_object_trace(base_object, 
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s Fetching port index failed for %s %d Port:%d\n",
                                __FUNCTION__, 
                                fbe_module_mgmt_device_type_to_string(fbe_module_mgmt_get_device_type(module_mgmt, sp, iom_num)),
                                fbe_module_mgmt_get_slot_num(module_mgmt, sp, iom_num), 
                                port_num);
    }
    return status;
}

/*!**************************************************************
 * fbe_module_mgmt_get_new_port_link_info()
 ****************************************************************
 * @brief
 *  This function gets gets the latest link status information 
 *  for the specified port.
 *
 * @param object_handle - This is the object handle, or in our
 * case the module_mgmt.
 * @param sp - sp id.
 * @param iom_num - io module index number.
 * @param port_num - Port number on that io module
 * @param port_object_id - object id for the specified port.
 *
 * @return fbe_status_t - The status of the call to
 *                        fbe_lifecycle_class_const_verify()
 *                        for the MODULE management's
 *                        constant lifecycle data.
 *
 * @author
 *  17-October-2011:  Created. bphilbin 
 *
 ****************************************************************/
static fbe_status_t fbe_module_mgmt_get_new_port_link_info(fbe_module_mgmt_t *module_mgmt, SP_ID sp, fbe_u8_t iom_num, 
                                                           fbe_u8_t port_num, fbe_object_id_t port_object_id)
{
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_base_object_t               *base_object = (fbe_base_object_t *)module_mgmt;
    fbe_port_link_information_t     port_link_info;
    fbe_port_link_state_t           old_link_state;
    fbe_bool_t                      send_notify = FALSE;
    fbe_device_physical_location_t  location = {0};
    fbe_notification_info_t         notification;
    fbe_object_id_t                 objectId;
    fbe_char_t                      log_num_string[255];
    fbe_u64_t                       device_type;
    HW_ENCLOSURE_TYPE               processorEnclType;
    fbe_u8_t                        port_index;

    port_index = fbe_module_mgmt_get_io_port_index(iom_num, port_num);
    
    //Do not report the link state for the internal connector on a DPE
    device_type = fbe_module_mgmt_get_device_type(module_mgmt, sp, iom_num);
    fbe_base_env_get_processor_encl_type((fbe_base_environment_t *)module_mgmt, &processorEnclType);

    status = fbe_api_port_get_link_information(port_object_id, &port_link_info);

    if ( ((device_type == FBE_DEVICE_TYPE_MEZZANINE) || (device_type == FBE_DEVICE_TYPE_BACK_END_MODULE)) &&
         (processorEnclType == DPE_ENCLOSURE_TYPE) && 
         (port_num == 0))
    {
        if (status == FBE_STATUS_OK) 
        {
            // fill in all the relevant fields but skip the link state and processing
            //module_mgmt->io_port_info[port_index].port_physical_info.link_info.link_speed = port_link_info.link_speed; 
            module_mgmt->io_port_info[port_index].port_physical_info.link_info.port_connect_class = port_link_info.port_connect_class;
            module_mgmt->io_port_info[port_index].port_physical_info.link_info.portal_number = port_link_info.portal_number;
            fbe_copy_memory(&module_mgmt->io_port_info[port_index].port_physical_info.link_info.u.sas_port, &port_link_info.u.sas_port, sizeof(fbe_port_sas_link_information_t));
        }

        return FBE_STATUS_OK;
    }

    if(status == FBE_STATUS_OK)
    {
        old_link_state = fbe_module_mgmt_get_port_link_state(module_mgmt,sp,iom_num,port_num);
        fbe_base_object_trace(base_object, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "ModMgmt: %s%d port %d link status changed from %s to %s.\n",
                              fbe_module_mgmt_device_type_to_string(device_type), 
                              fbe_module_mgmt_get_slot_num(module_mgmt, sp, iom_num),
                              port_num, fbe_module_mgmt_convert_link_state_to_string(old_link_state),
                              fbe_module_mgmt_convert_link_state_to_string(port_link_info.link_state));
        
        /*
         * Log an event if we make a significant transition in the link state.
         */
        if(old_link_state != port_link_info.link_state)
        {
            switch (port_link_info.link_state)
            {
            case FBE_PORT_LINK_STATE_DEGRADED:
                send_notify = TRUE;
                // log a link degraded event
                break;
            case FBE_PORT_LINK_STATE_DOWN:
                send_notify = TRUE;
                // log a link down event
                break;
            case FBE_PORT_LINK_STATE_UP:
                send_notify = TRUE;
                // log a link up event
                break;
            default:
                // do nothing
                break;
            }
            fbe_module_mgmt_set_port_link_info(module_mgmt, sp, iom_num, port_num, &port_link_info);

            /*
             * Link state has changed Update the Port LED if port is not faulted
             */
            if (fbe_module_mgmt_get_port_state(module_mgmt,sp,iom_num,port_num) != FBE_PORT_STATE_FAULTED)
            {
                if (fbe_module_mgmt_could_set_io_port_led(module_mgmt, iom_num, sp))
                {
                    fbe_module_mgmt_setIoPortLedBasedOnLink(module_mgmt,iom_num,port_num);
                }
            }
            
            if (send_notify)
            {
                location.port = port_num;
                location.slot = fbe_module_mgmt_get_slot_num(module_mgmt, sp, iom_num);
                location.sp = sp;

                fbe_base_object_get_object_id((fbe_base_object_t *)module_mgmt, &objectId);
            
                notification.notification_type = FBE_NOTIFICATION_TYPE_OBJECT_DATA_CHANGED;
                notification.class_id = FBE_CLASS_ID_MODULE_MGMT;
                notification.object_type = FBE_TOPOLOGY_OBJECT_TYPE_ENVIRONMENT_MGMT;
                notification.notification_data.data_change_info.data_type = FBE_DEVICE_DATA_TYPE_PORT_INFO;
                notification.notification_data.data_change_info.device_mask= fbe_module_mgmt_get_device_type(module_mgmt, sp, iom_num);
                notification.notification_data.data_change_info.phys_location = location;
            
                fbe_notification_send(objectId, notification);
                fbe_module_mgmt_get_port_logical_number_string(fbe_module_mgmt_get_port_logical_number(module_mgmt, sp, iom_num, port_num), log_num_string);
                fbe_event_log_write(ESP_INFO_LINK_STATUS_CHANGE, 
                                    NULL, 0, 
                                    "%s %d %d %s %s %s %s", 
                                    fbe_module_mgmt_device_type_to_string(fbe_module_mgmt_get_device_type(module_mgmt, sp, iom_num)), 
                                    fbe_module_mgmt_get_slot_num(module_mgmt, sp, iom_num),
                                    port_num,
                                    fbe_module_mgmt_port_role_to_string(fbe_module_mgmt_get_port_role(module_mgmt, sp,iom_num, port_num)),
                                    log_num_string,
                                    fbe_module_mgmt_convert_link_state_to_string(old_link_state),
                                    fbe_module_mgmt_convert_link_state_to_string(port_link_info.link_state));
            }
        }
    }
    else
    {
        fbe_base_object_trace(base_object, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "ModMgmt: Failed to get port link info for %s%d Port %d, Port Object:%d with status: 0x%X\n",
                              fbe_module_mgmt_device_type_to_string(fbe_module_mgmt_get_device_type(module_mgmt, module_mgmt->local_sp, iom_num)),
                              fbe_module_mgmt_get_slot_num(module_mgmt, module_mgmt->local_sp, iom_num),
                              port_num, port_object_id, status);
    }
    return status;
}
    

/*!**************************************************************
 * fbe_module_mgmt_get_new_mgmt_module_info()
 ****************************************************************
 * @brief
 *  This function gets Board Object ID (it interfaces with MODULE)
 *  and adds them to the queue to be processed later for management
 *  module info.
 *
 * @param object_handle - This is the object handle, or in our
 * case the module_mgmt.
 * @param packet_p - The packet arriving from the monitor
 * @param slot - management module slot
 * scheduler.
 *
 * @return fbe_status_t - The status of the call to
 *                        fbe_lifecycle_class_const_verify()
 *                        for the MODULE management's
 *                        constant lifecycle data.
 *
 * @author
 *  19-May-2010:  Created. Nayana Chaudhari 
 *
 ****************************************************************/
static fbe_status_t fbe_module_mgmt_get_new_mgmt_module_info(fbe_module_mgmt_t *module_mgmt,
                                                             SP_ID sp_id,
                                                             fbe_u8_t slot)
{
    fbe_base_object_t                       *base_object = (fbe_base_object_t *)module_mgmt;
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_board_mgmt_mgmt_module_info_t       new_mgmt_module_comp_info = {0};
    fbe_board_mgmt_mgmt_module_info_t       old_mgmt_module_comp_info = {0};
    fbe_device_physical_location_t          location = {0};
    LED_BLINK_RATE                          blink_rate = LED_BLINK_OFF;

    new_mgmt_module_comp_info.associatedSp = sp_id;
    new_mgmt_module_comp_info.mgmtID = slot;

    status = fbe_api_board_get_mgmt_module_info(module_mgmt->board_object_id,
                                &new_mgmt_module_comp_info);
    if (status != FBE_STATUS_OK) 
    {
        fbe_base_object_trace(base_object, 
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s Error in getting mgmt_module_info, status: 0x%X\n",
                                __FUNCTION__, status);
        return status;
    }

    if(fbe_module_mgmt_get_mgmt_module_logical_fault(&new_mgmt_module_comp_info))
    {
        blink_rate = LED_BLINK_ON;
    }

    status = fbe_api_board_set_mgmt_module_fault_LED(module_mgmt->board_object_id,
                                        sp_id,
                                        blink_rate);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)module_mgmt,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s set_mgmt_module_fault_LED failed, SP: %d, Slot: %d, status: 0x%X.\n",
                              __FUNCTION__, sp_id, slot, status);
    }

    /* Save the old mgmt module information */
    old_mgmt_module_comp_info = module_mgmt->mgmt_module_info[slot].mgmt_module_comp_info[sp_id];

    /* Log the event for change */
    fbe_module_mgmt_log_mgmt_module_event(module_mgmt,
                                          &new_mgmt_module_comp_info, 
                                          &old_mgmt_module_comp_info);
    fbe_copy_memory(&(module_mgmt->mgmt_module_info[slot].mgmt_module_comp_info[sp_id]), 
        &new_mgmt_module_comp_info, 
        sizeof(fbe_board_mgmt_mgmt_module_info_t));

    
    /* Queue up the Resume prom read only if the inserted bit is set */
    location.sp = sp_id;
    location.slot = slot;

    /* Process the Resume prom read */
    status = fbe_module_mgmt_resume_prom_handle_mgmt_module_status_change(module_mgmt, 
                                                                          &location, 
                                                                          FBE_DEVICE_TYPE_MGMT_MODULE, 
                                                                          &new_mgmt_module_comp_info, 
                                                                          &old_mgmt_module_comp_info);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace(base_object, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "RP: resume_prom_handle_mgmt_module_status_change failed, SP: %d, Slot: %d.\n",
                              sp_id, slot);
    }

    /*
     * Check if Enclosure Fault LED needs updating
     */
    status = fbe_module_mgmt_update_encl_fault_led(module_mgmt, &location, FBE_DEVICE_TYPE_MGMT_MODULE, FBE_ENCL_FAULT_LED_MGMT_MODULE_FLT);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace(base_object,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "ModMgmt: error updating EnclFaultLed for management module, SP: %d, Slot: %d, status 0x%X.\n",
                              sp_id, slot, status);
    }

    /* Send data change notification */
    fbe_base_environment_send_data_change_notification((fbe_base_environment_t *)module_mgmt,
                                                       FBE_CLASS_ID_MODULE_MGMT,
                                                       FBE_DEVICE_TYPE_MGMT_MODULE,
                                                       FBE_DEVICE_DATA_TYPE_GENERIC_INFO,
                                                       &location);

    if( (new_mgmt_module_comp_info.isLocalFru == FBE_TRUE) &&
        (new_mgmt_module_comp_info.bInserted == FBE_TRUE) &&
        (old_mgmt_module_comp_info.bInserted == FBE_FALSE) &&
        (new_mgmt_module_comp_info.envInterfaceStatus == FBE_ENV_INTERFACE_STATUS_GOOD) )
    {
        // newly inserted mgmt_module check if speed or vlan need to be set
        fbe_base_object_trace((fbe_base_object_t *)module_mgmt,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s Mgmt Module Newly Inserted SP: %d, Slot: %d.\n",
                              __FUNCTION__, sp_id, slot);
        status = fbe_lifecycle_set_cond(&fbe_module_mgmt_lifecycle_const,
                                        (fbe_base_object_t*)module_mgmt,
                                        FBE_MODULE_MGMT_LIFECYCLE_COND_CONFIG_VLAN);
        if(status != FBE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t *)module_mgmt, 
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s, Failed to set condition to configure vLan mode, status: 0x%X",
                                   __FUNCTION__, status);
        }
        
        if( (fbe_module_mgmt_is_mgmt_port_duplex_valid(module_mgmt->mgmt_module_info[slot].mgmt_port_persistent_info.mgmtPortDuplex) &&
             (new_mgmt_module_comp_info.managementPort.mgmtPortDuplex != module_mgmt->mgmt_module_info[slot].mgmt_port_persistent_info.mgmtPortDuplex)) ||
            (fbe_module_mgmt_is_mgmt_port_speed_valid(module_mgmt->mgmt_module_info[slot].mgmt_port_persistent_info.mgmtPortSpeed) && 
              (new_mgmt_module_comp_info.managementPort.mgmtPortSpeed != module_mgmt->mgmt_module_info[slot].mgmt_port_persistent_info.mgmtPortSpeed)) ||
            (fbe_module_mgmt_is_mgmt_port_auto_negotiate_valid(module_mgmt->mgmt_module_info[slot].mgmt_port_persistent_info.mgmtPortAutoNeg) &&
             (new_mgmt_module_comp_info.managementPort.mgmtPortAutoNeg != module_mgmt->mgmt_module_info[slot].mgmt_port_persistent_info.mgmtPortAutoNeg)))
        {
            // the management port settings are different than what we want, re-configure the port

            fbe_base_object_trace((fbe_base_object_t *)module_mgmt,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "ModMgmt: Set Mgmt Port Settings for SP: %d, Slot: %d Current Dplx:%d Spd:%d AtNg:%d\n",
                               sp_id, slot, 
                               new_mgmt_module_comp_info.managementPort.mgmtPortDuplex,
                               new_mgmt_module_comp_info.managementPort.mgmtPortSpeed, 
                               new_mgmt_module_comp_info.managementPort.mgmtPortAutoNeg);

            if(fbe_module_mgmt_is_mgmt_port_duplex_valid(module_mgmt->mgmt_module_info[slot].mgmt_port_persistent_info.mgmtPortDuplex))
            {
                module_mgmt->mgmt_module_info[slot].mgmt_port_config_op.mgmtPortDuplexModeRequested = module_mgmt->mgmt_module_info[slot].mgmt_port_persistent_info.mgmtPortDuplex;
            }
            else
            {
                module_mgmt->mgmt_module_info[slot].mgmt_port_config_op.mgmtPortDuplexModeRequested = new_mgmt_module_comp_info.managementPort.mgmtPortDuplex;
            }
            if(fbe_module_mgmt_is_mgmt_port_speed_valid(module_mgmt->mgmt_module_info[slot].mgmt_port_persistent_info.mgmtPortSpeed))
            {
                module_mgmt->mgmt_module_info[slot].mgmt_port_config_op.mgmtPortSpeedRequested = module_mgmt->mgmt_module_info[slot].mgmt_port_persistent_info.mgmtPortSpeed;
            }
            else
            {
                module_mgmt->mgmt_module_info[slot].mgmt_port_config_op.mgmtPortSpeedRequested = new_mgmt_module_comp_info.managementPort.mgmtPortSpeed;
            }
            if(fbe_module_mgmt_is_mgmt_port_auto_negotiate_valid(module_mgmt->mgmt_module_info[slot].mgmt_port_persistent_info.mgmtPortAutoNeg))
            {
                module_mgmt->mgmt_module_info[slot].mgmt_port_config_op.mgmtPortAutoNegRequested = module_mgmt->mgmt_module_info[slot].mgmt_port_persistent_info.mgmtPortAutoNeg;
            }
            else
            {
                module_mgmt->mgmt_module_info[slot].mgmt_port_config_op.mgmtPortAutoNegRequested = new_mgmt_module_comp_info.managementPort.mgmtPortAutoNeg;
            }

            module_mgmt->mgmt_module_info[slot].mgmt_port_config_op.previousMgmtPortAutoNeg = new_mgmt_module_comp_info.managementPort.mgmtPortAutoNeg;
            module_mgmt->mgmt_module_info[slot].mgmt_port_config_op.previousMgmtPortSpeed = new_mgmt_module_comp_info.managementPort.mgmtPortSpeed;
            module_mgmt->mgmt_module_info[slot].mgmt_port_config_op.previousMgmtPortDuplexMode = new_mgmt_module_comp_info.managementPort.mgmtPortDuplex;

            module_mgmt->mgmt_module_info[slot].mgmt_port_config_op.revertMgmtPortConfig = FBE_FALSE;
            module_mgmt->mgmt_module_info[slot].mgmt_port_config_op.sendMgmtPortConfig = FBE_TRUE;
            status = fbe_lifecycle_set_cond(&fbe_module_mgmt_lifecycle_const,
                                        (fbe_base_object_t*)module_mgmt,
                                        FBE_MODULE_MGMT_LIFECYCLE_COND_SET_MGMT_PORT_SPEED);
            if(status != FBE_STATUS_OK)
            {
                fbe_base_object_trace((fbe_base_object_t *)module_mgmt, 
                                      FBE_TRACE_LEVEL_ERROR,
                                      FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                      "%s, Failed to set condition to configure vLan mode, status: 0x%X",
                                       __FUNCTION__, status);
            }
        }
    }

    return status;
}
/******************************************************
 * end fbe_module_mgmt_get_new_mgmt_module_info() 
 ******************************************************/

fbe_bool_t fbe_module_mgmt_is_mgmt_port_speed_valid(fbe_mgmt_port_speed_t speed)
{
    if( (speed == FBE_MGMT_PORT_SPEED_1000MBPS) ||
        (speed == FBE_MGMT_PORT_SPEED_100MBPS) ||
        (speed == FBE_MGMT_PORT_SPEED_10MBPS) )
    {
        return FBE_TRUE;
    }
    else
    {
        return FBE_FALSE;
    }
}
fbe_bool_t fbe_module_mgmt_is_mgmt_port_duplex_valid(fbe_mgmt_port_duplex_mode_t duplex)
{
    if( (duplex == FBE_PORT_DUPLEX_MODE_HALF) ||
        (duplex == FBE_PORT_DUPLEX_MODE_FULL) )
    {
        return FBE_TRUE;
    }
    else
    {
        return FBE_FALSE;
    }
}

fbe_bool_t fbe_module_mgmt_is_mgmt_port_auto_negotiate_valid(fbe_mgmt_port_auto_neg_t auto_negotiate)
{
    if( (auto_negotiate == FBE_PORT_AUTO_NEG_OFF) ||
        (auto_negotiate == FBE_PORT_AUTO_NEG_ON) )
    {
        return FBE_TRUE;
    }
    else
    {
        return FBE_FALSE;
    }
}


/*!**************************************************************
 * fbe_module_mgmt_get_port_device_object_id()
 ****************************************************************
 * @brief
 *  This function looks for a port device object id based upon
 *  matching pci address information.  This whole process is matching
 *  port information based on pci numbers between the board object and
 *  port object in the physical package.
 * 
 * @param module_mgmt - object context
 * @param port_num - Global Port Index
 * @return fbe_object_id_t - Port Device Object ID
 * @author
 *  01-Jul-2010:  Created. Brion Philbin 
 *
 ****************************************************************/
static
fbe_object_id_t fbe_module_mgmt_get_port_device_object_id(fbe_module_mgmt_t *module_mgmt, fbe_u8_t port_num )
{
    fbe_u8_t port_index;
    fbe_object_id_t object_id;
    fbe_status_t status;
    fbe_port_hardware_info_t port_hardware_info;
    fbe_port_link_information_t port_information;
    SPECL_PCI_DATA port_object_pci_data;
    fbe_port_role_t port_role;

    fbe_base_object_trace((fbe_base_object_t *) module_mgmt,
                                  FBE_TRACE_LEVEL_DEBUG_LOW,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "ModMgmt: searching for port object matching port_num:%d\n",
                                  port_num);

    /*
     * Iterate through the possible ports.
     */
    for(port_role=FBE_PORT_ROLE_FE; port_role < FBE_PORT_ROLE_MAX; port_role++)
    {
        for(port_index = 0; port_index < FBE_ESP_MAX_CONFIGURED_PORTS_PER_SP; port_index++)
        {
            /*
             * Get the object ID for the specified port index.
             */
            //status = fbe_api_get_port_object_id_by_location(port_index, &object_id);
            status = fbe_api_get_port_object_id_by_location_and_role(port_index,
                                                                     port_role,
                                                                     &object_id);
            fbe_base_object_trace((fbe_base_object_t *) module_mgmt,
                                      FBE_TRACE_LEVEL_DEBUG_LOW,
                                      FBE_TRACE_MESSAGE_ID_INFO,
                                      "ModMgmt: port_role:%d port_index:%d returned status:%d to get port_object ID:%d\n",
                                      port_role, port_index, status, object_id);
    
    
            if ((status == FBE_STATUS_OK) && (object_id != FBE_OBJECT_ID_INVALID))
            {
                fbe_zero_memory(&port_hardware_info, sizeof(fbe_port_hardware_info_t));
                fbe_zero_memory(&port_information, sizeof(fbe_port_link_information_t));
                /*
                 * With the object ID get the pci information from the port hardware information.
                 */
                status = fbe_api_get_port_hardware_information(object_id, &port_hardware_info);
                /*
                 * We get the portal number from the link_information
                 */
                status = fbe_api_port_get_link_information(object_id, &port_information);
    
                port_object_pci_data.bus = port_hardware_info.pci_bus;
                port_object_pci_data.device = port_hardware_info.pci_slot;
                port_object_pci_data.function = port_hardware_info.pci_function + port_information.portal_number;
    
                fbe_base_object_trace((fbe_base_object_t *) module_mgmt,
                                      FBE_TRACE_LEVEL_DEBUG_LOW,
                                      FBE_TRACE_MESSAGE_ID_INFO,
                                      "ModMgmt: Matching port object at PCI %d.%d.%d to a port\n",
                                      port_object_pci_data.bus,
                                      port_object_pci_data.device,
                                      port_object_pci_data.function);
    
                /*
                 * If the pci address matches what we have saved for this port in our object 
                 * then the port index is our enumerated device number. Save this device number 
                 * along with the hardware information. 
                 */
                if( (status == FBE_STATUS_OK) &&
                    (port_object_pci_data.bus == 
                     module_mgmt->io_port_info[port_num].port_physical_info.selected_pci_function.bus) &&
                    (port_object_pci_data.device == 
                     module_mgmt->io_port_info[port_num].port_physical_info.selected_pci_function.device) &&
                    (port_object_pci_data.function == 
                     module_mgmt->io_port_info[port_num].port_physical_info.selected_pci_function.function) )
    
                {
    
                    fbe_base_object_trace((fbe_base_object_t *) module_mgmt,
                                      FBE_TRACE_LEVEL_DEBUG_LOW,
                                      FBE_TRACE_MESSAGE_ID_INFO,
                                      "ModMgmt: Matched port object:%d to port:%d\n",
                                      object_id, port_num);
                    module_mgmt->io_port_info[port_num].port_object_id = object_id;
                    fbe_copy_memory(&(module_mgmt->io_port_info[port_num].port_physical_info.port_hardware_info), 
                        &port_hardware_info, 
                        sizeof(fbe_port_hardware_info_t));
    
                    return object_id;
                }
            }
        }
    }
    fbe_base_object_trace((fbe_base_object_t *) module_mgmt,
                          FBE_TRACE_LEVEL_DEBUG_LOW,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "ModMgmt: Err match port obj ID %d to IO mod and port\n",
                          object_id);
    return FBE_OBJECT_ID_INVALID;
}
/******************************************************
 * end fbe_module_mgmt_get_port_device_object_id() 
 ******************************************************/

/*!**************************************************************
 * fbe_module_mgmt_get_iom_num_port_num_from_port_device_object_id()
 ****************************************************************
 * @brief
 *  This function matches a port device object to a corresponding
 *  io module and port number by matching the pci and portal number
 *  information for the port.
 * 
 * @param module_mgmt - object context
 * @param port_device_object - Port Device Object ID
 * @return iom_num, - IO module number
 * @return port_num - Port Number
 * @author
 *  23-Mar-2011:  Created. Brion Philbin 
 *
 ****************************************************************/
static
fbe_status_t fbe_module_mgmt_get_iom_num_port_num_from_port_device_object_id(fbe_module_mgmt_t *module_mgmt, 
                                                                             fbe_object_id_t port_device_object, 
                                                                             fbe_u8_t *iom_num, fbe_u8_t *port_num)
{
    fbe_u8_t port_index;
    SP_ID sp_id = module_mgmt->base_environment.spid;
    fbe_u32_t port_count;
    fbe_object_id_t     port_object_id = FBE_OBJECT_ID_INVALID;

    /*
     * During the discovery of all the ports we saved away the port object ID
     * associated with the port, we can just match that now.
    */

    for(*iom_num = 0; *iom_num < FBE_ESP_IO_MODULE_MAX_COUNT; (*iom_num)++)
    {
        port_count = module_mgmt->io_module_info[*iom_num].physical_info[sp_id].module_comp_info.ioPortCount;
        for(*port_num = 0; *port_num < port_count; (*port_num)++ )
        {
            port_index = fbe_module_mgmt_get_io_port_index(*iom_num, *port_num);

            /* The port object id is invalid, it may caused by the
             * port object did not initialized when module mgmt discovery,
            * so retry to get port object id from physical package.
            */
            if(module_mgmt->io_port_info[port_index].port_object_id == FBE_OBJECT_ID_INVALID)
            {
                port_object_id = fbe_module_mgmt_get_port_device_object_id(module_mgmt, port_index);
                if(port_object_id != FBE_OBJECT_ID_INVALID)
                {
                    module_mgmt->io_port_info[port_index].port_object_id = port_object_id;
                    fbe_base_object_trace((fbe_base_object_t *)module_mgmt,
                                          FBE_TRACE_LEVEL_INFO,
                                          FBE_TRACE_MESSAGE_ID_INFO,
                                          "ModMgmt: find port obj id 0x%x for port index: %d.\n",
                                          port_object_id, port_index);
                }
                else
                {
                    fbe_base_object_trace((fbe_base_object_t *)module_mgmt,
                                          FBE_TRACE_LEVEL_INFO,
                                          FBE_TRACE_MESSAGE_ID_INFO,
                                          "ModMgmt: cannot find port obj id for port index: %d.\n",
                                          port_index);
                }
            }

            if(module_mgmt->io_port_info[port_index].port_object_id == port_device_object )
            {
                return FBE_STATUS_OK;
            }
        }
    }

    return FBE_STATUS_GENERIC_FAILURE;
}
/************************************************************************
 * end fbe_module_mgmt_get_iom_num_port_num_from_port_device_object_id() 
 ************************************************************************/



/*!**************************************************************
 * fbe_module_mgmt_processReceivedCmiMessage()
 ****************************************************************
 * @brief
 *  This function handles fbe_module_mgmt related messages from
 *  the peer SP.
 * 
 * @param module_mgmt - object context
 * @param cmiMsgPtr - peer message
 * 
 * @return fbe_status_t - success, if message was handled.
 * 
 * @author
 *  12-Nov-2010:  Created. Brion Philbin 
 *
 ****************************************************************/
static fbe_status_t fbe_module_mgmt_processReceivedCmiMessage(fbe_module_mgmt_t *module_mgmt, fbe_module_mgmt_cmi_msg_t *cmiMsgPtr)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u64_t opcode;
    fbe_base_environment_t             * pBaseEnv = (fbe_base_environment_t *)module_mgmt;
    fbe_base_env_fup_cmi_packet_t      * pFupCmiPkt = (fbe_base_env_fup_cmi_packet_t *)cmiMsgPtr;

    opcode = fbe_base_environment_get_cmi_msg_opcode((fbe_base_environment_cmi_message_t *) cmiMsgPtr);
    fbe_base_object_trace((fbe_base_object_t *)module_mgmt, 
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s, Message: %llu\n",
                          __FUNCTION__, (unsigned long long)opcode);

    switch (opcode)
    {

    case FBE_MODULE_MGMT_PORTS_CONFIGURED_MSG:
        break;

    case FBE_MODULE_MGMT_SLIC_UPGRADE_MSG:
        if(module_mgmt->slic_upgrade == FBE_MODULE_MGMT_SLIC_UPGRADE_PEER_SP)
        {
            // Peer SP has completed, clear the slic upgrade in progress
            module_mgmt->slic_upgrade = FBE_MODULE_MGMT_NO_SLIC_UPGRADE;
            fbe_module_mgmt_set_slics_marked_for_upgrade(&module_mgmt->slic_upgrade);
        }
        else
        {
            module_mgmt->slic_upgrade = FBE_MODULE_MGMT_SLIC_UPGRADE_PEER_SP;
        }
        break;

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
    case FBE_BASE_ENV_CMI_MSG_PEER_SP_ALIVE:
        // check if our earlier perm request failed
        status = fbe_module_mgmt_fup_new_contact_init_upgrade(module_mgmt, FBE_DEVICE_TYPE_BACK_END_MODULE);
        break;
    default:
        status = fbe_base_environment_cmi_process_received_message((fbe_base_environment_t *)module_mgmt, 
                                                                   (fbe_base_environment_cmi_message_t *) cmiMsgPtr);
        break;
    }
    return status;
}

/*!**************************************************************
 *  @fn fbe_module_mgmt_processPeerNotPresent(fbe_module_mgmt_t * pModuleMgmt,
 *                                        fbe_module_mgmt_cmi_packet_t * pModuleCmiPkt)
 ****************************************************************
 * @brief
 *  This function gets called when recieving the CMI message with 
 *  FBE_CMI_EVENT_PEER_NOT_PRESENT.
 *
 * @param pModuleMgmt - 
 * @param pModuleCmiPkt - pointer to user message of CMI message info.
 *
 * @return fbe_status_t -
 *   FBE_STATUS_MORE_PROCESSING_REQUIRED - need to further handled by the base environment.
 *   FBE_STATUS_OK - successful.
 *   others - failed.
 * 
 * @author
 *  2-Nov-2012: Rui Chang - Created.
 ****************************************************************/
static fbe_status_t 
fbe_module_mgmt_processPeerNotPresent(fbe_module_mgmt_t * pModuleMgmt, 
                                    fbe_module_mgmt_cmi_msg_t * pModuleCmiPkt)
{
    fbe_base_environment_cmi_message_t * pBaseCmiMsg = (fbe_base_environment_cmi_message_t *)pModuleCmiPkt;
    fbe_base_env_fup_cmi_packet_t      * pFupCmiPkt = (fbe_base_env_fup_cmi_packet_t *)pModuleCmiPkt;
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
            status = fbe_base_env_fup_processDeniedPeerPerm((fbe_base_environment_t *)pModuleMgmt,
                                                         pFupCmiPkt->pRequestorWorkItem);
     
            break;

        default:
            status = FBE_STATUS_MORE_PROCESSING_REQUIRED;
            break;
    }

    return(status);
}

/*!**************************************************************
 * @fn fbe_module_mgmt_processContactLost(fbe_module_mgmt_t * pModuleMgmt)
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
 *  2-Nov-2012: Rui Chang - Created.
 ****************************************************************/
static fbe_status_t fbe_module_mgmt_processContactLost(fbe_module_mgmt_t * pModuleMgmt)
{
    /* No need to handle the fup work items here. If the contact is lost, we will also receive
     * Peer Not Present message and we will handle the fup work item there. 
     */
    /* Return FBE_STATUS_MORE_PROCESSING_REQUIRED so that it will do further processing in base_environment.*/
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}

/*!**************************************************************
 * @fn fbe_ps_mgmt_processPeerBusy(fbe_ps_mgmt_t * pModuleMgmt, 
 *                          fbe_ps_mgmt_cmi_packet_t * pModuleCmiPkt)
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
 *  2-Nov-2012: Rui Chang - Created.
 ****************************************************************/
static fbe_status_t 
fbe_module_mgmt_processPeerBusy(fbe_module_mgmt_t * pModuleMgmt, 
                              fbe_module_mgmt_cmi_msg_t * pModuleCmiPkt)
{
    fbe_base_environment_cmi_message_t * pBaseCmiMsg = (fbe_base_environment_cmi_message_t *)pModuleCmiPkt;
    fbe_base_env_fup_cmi_packet_t      * pFupCmiPkt = (fbe_base_env_fup_cmi_packet_t *)pModuleCmiPkt;
    fbe_status_t                         status = FBE_STATUS_OK;

    switch (pBaseCmiMsg->opcode)
    {
        case FBE_BASE_ENV_CMI_MSG_FUP_PEER_PERMISSION_GRANT:
        case FBE_BASE_ENV_CMI_MSG_FUP_PEER_PERMISSION_DENY:
            fbe_base_object_trace((fbe_base_object_t *)pModuleMgmt, 
                          FBE_TRACE_LEVEL_ERROR,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s, should never happen, opcode 0x%x\n",
                          __FUNCTION__,
                          pBaseCmiMsg->opcode);

            status = FBE_STATUS_GENERIC_FAILURE;
            break;

        case FBE_BASE_ENV_CMI_MSG_FUP_PEER_PERMISSION_REQUEST:
            // peer SP is not present when sending the message to request permission.
            status = fbe_base_env_fup_peerPermRetry((fbe_base_environment_t *)pModuleMgmt, 
                                                    pFupCmiPkt->pRequestorWorkItem);
            break;

        default:
            status = FBE_STATUS_MORE_PROCESSING_REQUIRED;
            break;
    }

    return(status);
}

/*!**************************************************************
 * @fn fbe_module_mgmt_processFatalError(fbe_module_mgmt_t * pModuleMgmt, 
 *                                     fbe_ps_mgmt_cmi_packet_t * pModuleCmiPkt)
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
 *  2-Nov-2012: Rui Chang - Created.
 ****************************************************************/
static fbe_status_t 
fbe_module_mgmt_processFatalError(fbe_module_mgmt_t * pModuleMgmt, 
                                fbe_module_mgmt_cmi_msg_t * pModuleCmiPkt)
{
    fbe_base_environment_cmi_message_t * pBaseCmiMsg = (fbe_base_environment_cmi_message_t *)pModuleCmiPkt;
    fbe_base_env_fup_cmi_packet_t      * pFupCmiPkt = (fbe_base_env_fup_cmi_packet_t *)pModuleCmiPkt;
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
            status = fbe_base_env_fup_processDeniedPeerPerm((fbe_base_environment_t *)pModuleMgmt,
                                                         pFupCmiPkt->pRequestorWorkItem);
            break;

        default:
            status = FBE_STATUS_MORE_PROCESSING_REQUIRED;
            break;
    }

    return(status);
}


/*!**************************************************************
 * fbe_module_mgmt_register_resume_prom_callback_cond_function()
 ****************************************************************
 * @brief
 *  This function registers the callback functions with the base environment object.
 * 
 * @param base_object - This is the object handle, or in our
 * case the module_mgmt.
 * @param packet - The packet arriving from the monitor
 * scheduler.
 * 
 * @return fbe_status_t - The status of the call to
 *                        fbe_lifecycle_class_const_verify() for
 *                        the Encl management's constant
 *                        lifecycle data.
 *
 * @author
 *  03-Nov-2010: Arun S - Created. 
 *
 ****************************************************************/
static fbe_lifecycle_status_t
fbe_module_mgmt_register_resume_prom_callback_cond_function(fbe_base_object_t * base_object, 
                                                            fbe_packet_t * packet)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_base_environment_t * pBaseEnv = (fbe_base_environment_t *)base_object;

    fbe_base_object_trace(base_object, 
                          FBE_TRACE_LEVEL_DEBUG_HIGH,  
                          FBE_TRACE_MESSAGE_ID_INFO, 
                          "%s entry\n", 
                          __FUNCTION__);

    fbe_base_env_initiate_resume_prom_read_callback(pBaseEnv, (fbe_base_env_initiate_resume_prom_read_callback_func_ptr_t) &fbe_module_mgmt_initiate_resume_prom_read);
    fbe_base_env_get_resume_prom_info_ptr_callback(pBaseEnv, (fbe_base_env_get_resume_prom_info_ptr_func_callback_ptr_t)&fbe_module_mgmt_get_resume_prom_info_ptr);
    fbe_base_env_resume_prom_update_encl_fault_led_callback(pBaseEnv, (fbe_base_env_resume_prom_update_encl_fault_led_callback_func_ptr_t)&fbe_module_mgmt_resume_prom_update_encl_fault_led);

    status = fbe_lifecycle_clear_current_cond(base_object);
    if (status != FBE_STATUS_OK) 
    {
        fbe_base_object_trace(base_object, 
                            FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                            "RP: %s can't clear current condition, status: 0x%X",
                            __FUNCTION__, status);
    }
   
    return FBE_LIFECYCLE_STATUS_DONE;
}
/*!***************************************************************
 * fbe_module_mgmt_config_vlan_cond_function()
 ****************************************************************
 * @brief
 *  This is condtion function used to set the vLan config mode to
 *  CLARiiON_VLAN_MODE
 *
 * @param base_object - This is the object handle, 
 * @param packet - The packet arriving from the monitor
 *                 scheduler
 *
 * @return fbe_status_t
 *
 * @author 
 *  25-Mar-2010: Created  Vaibhav Gaonkar
 *
 ****************************************************************/
static fbe_lifecycle_status_t 
fbe_module_mgmt_config_vlan_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet)
{
    fbe_status_t        status;
    fbe_module_mgmt_t   *module_mgmt = (fbe_module_mgmt_t *) base_object;
    fbe_board_mgmt_mgmt_module_info_t                       *module_mgmt_info;
    fbe_board_mgmt_set_set_mgmt_vlan_mode_async_context_t   *vlan_config_context;

    fbe_base_object_trace(base_object,
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s Enter.\n",
                          __FUNCTION__);
    /* Clear condition */
    status = fbe_lifecycle_clear_current_cond(base_object);
    if (status != FBE_STATUS_OK) 
    {
        fbe_base_object_trace(base_object, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s, failed to clear condition %d\n", __FUNCTION__, status);                
    }

    /* Get module mgmt info for local SP */
    module_mgmt_info = &(module_mgmt->mgmt_module_info->mgmt_module_comp_info[module_mgmt->base_environment.spid]);

    if((module_mgmt_info->envInterfaceStatus == FBE_ENV_INTERFACE_STATUS_GOOD) &&
        ((module_mgmt_info->vLanConfigMode != CLARiiON_VLAN_MODE) && (module_mgmt_info->vLanConfigMode != CUSTOM_VLAN_MODE)) &&
        (module_mgmt_info->managementPort.mgmtPortAutoNeg))
    {
        /* Send command to set VLAN Mode */
        vlan_config_context = fbe_base_env_memory_ex_allocate((fbe_base_environment_t *)module_mgmt, sizeof(fbe_board_mgmt_set_set_mgmt_vlan_mode_async_context_t));
        if(vlan_config_context == NULL)
        {
            fbe_base_object_trace((fbe_base_object_t*)module_mgmt, 
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s, failed to allocate a command context\n",
                                  __FUNCTION__);
            return FBE_LIFECYCLE_STATUS_DONE;
        }
        fbe_zero_memory(vlan_config_context, sizeof(fbe_board_mgmt_set_set_mgmt_vlan_mode_async_context_t));

        fbe_base_object_trace((fbe_base_object_t *)module_mgmt,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s , Configuring vLan to CLARiiON_VLAN_MODE\n",
                              __FUNCTION__);
        vlan_config_context->status = FBE_STATUS_GENERIC_FAILURE;
        vlan_config_context->command.sp_id = module_mgmt->base_environment.spid;
        vlan_config_context->command.vlan_config_mode = CLARiiON_VLAN_MODE;
        vlan_config_context->object_context = module_mgmt;
        vlan_config_context->completion_function = fbe_module_mgmt_config_vlan_completion;
        status = fbe_api_board_set_mgmt_module_vlan_config_mode_async(module_mgmt->board_object_id, 
                                                                      vlan_config_context);
        if(status != FBE_STATUS_PENDING)
        {
            fbe_base_object_trace(base_object, 
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s, failed to send command to config vLan mode \n",
                                  __FUNCTION__);
        }
    }
    return FBE_LIFECYCLE_STATUS_DONE;
}
/************ **************************************************
 *  end of fbe_module_mgmt_config_vlan_cond_function
 **************************************************************/
 /*!***************************************************************
 * fbe_module_mgmt_config_vlan_completion()
 ****************************************************************
 * @brief
 *  This is completion function for set vLan config mode condition
 *
 * @param command_context - Ptr to command context that set in 
 *                          condition function 
 *
 * @return void
 *
 * @author 
 *  25-Mar-2010: Created  Vaibhav Gaonkar
 *
 ****************************************************************/
void 
fbe_module_mgmt_config_vlan_completion(fbe_board_mgmt_set_set_mgmt_vlan_mode_async_context_t *command_context)
{
    fbe_status_t        status;
    fbe_module_mgmt_t   *module_mgmt = (fbe_module_mgmt_t *)command_context->object_context;
     
    if(command_context->status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)module_mgmt,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s , vLan mode set failed; set the condition again\n",
                              __FUNCTION__);

        /* Release the context memory that allocated in condition function
         * and set the condition again
         */
        fbe_base_env_memory_ex_release((fbe_base_environment_t *)module_mgmt, command_context);
        status = fbe_lifecycle_set_cond(&fbe_module_mgmt_lifecycle_const,
                                        (fbe_base_object_t*)module_mgmt,
                                        FBE_MODULE_MGMT_LIFECYCLE_COND_CONFIG_VLAN);
        if(status != FBE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t *)module_mgmt, 
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s, Failed to set condition to configure vLan mode, status: 0x%X",
                                   __FUNCTION__, status);
        }
        return;
    }
    fbe_base_object_trace((fbe_base_object_t *)module_mgmt,
                           FBE_TRACE_LEVEL_INFO,
                           FBE_TRACE_MESSAGE_ID_INFO,
                           "%s , Successfully configure the vLan mode to CLARiiON_VLAN_MODE \n",
                           __FUNCTION__);
    /*Release the context memory that allocated in condition function */
    fbe_base_env_memory_ex_release((fbe_base_environment_t *)module_mgmt, command_context);
}
/************ *****************************************
 *  end of fbe_module_mgmt_config_vlan_completion
 *****************************************************/
/*!***************************************************************
 * fbe_module_mgmt_set_mgmt_port_speed_cond_function()
 ****************************************************************
 * @brief
 *  This is condtion function for FBE_MODULE_MGMT_LIFECYCLE_COND_SET_MGMT_PORT_SPEED.
 *  This initiate the MGMT port speed config.
 *
 * @param base_object - This is the object handle, 
 * @param packet - The packet arriving from the monitor
 *                 scheduler
 *
 * @return fbe_lifecycle_status_t
 *
 * @author 
 *  25-Mar-2010: Created  Vaibhav Gaonkar
 *
 ****************************************************************/
static fbe_lifecycle_status_t 
fbe_module_mgmt_set_mgmt_port_speed_cond_function(fbe_base_object_t * base_object,
                                                  fbe_packet_t * packet)
{
    fbe_u32_t           mgmt_id;
    fbe_status_t        status;
    fbe_module_mgmt_t   *module_mgmt = (fbe_module_mgmt_t *) base_object;
    fbe_board_mgmt_set_mgmt_port_async_context_t    *mgmt_port_context = NULL;

    fbe_base_object_trace(base_object,
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s Entry, Setting MGMT port speed.\n",
                          __FUNCTION__);
    
    /* Clear condition */
    status = fbe_lifecycle_clear_current_cond(base_object);
    if (status != FBE_STATUS_OK) 
    {
        fbe_base_object_trace(base_object, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s failed to clear condition %d\n", __FUNCTION__, status);                
    }
    
    for(mgmt_id = 0; mgmt_id < module_mgmt->discovered_hw_limit.num_mgmt_modules; mgmt_id++)
    {
        if(module_mgmt->mgmt_module_info[mgmt_id].mgmt_port_config_op.sendMgmtPortConfig)
        {
            /* Send command to set MGMT port speed */
            mgmt_port_context = fbe_base_env_memory_ex_allocate((fbe_base_environment_t *)module_mgmt, sizeof(fbe_board_mgmt_set_mgmt_port_async_context_t));
            if(mgmt_port_context == NULL)
            {
                fbe_base_object_trace(base_object, 
                                      FBE_TRACE_LEVEL_ERROR,
                                      FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                      "%s, failed to allocate a command context\n",
                                      __FUNCTION__);
                return FBE_LIFECYCLE_STATUS_DONE;
            }

            /* Initializing the command context */
            if(module_mgmt->mgmt_module_info[mgmt_id].mgmt_port_config_op.revertMgmtPortConfig)
            {
                /* In case of revert used previous values */
                mgmt_port_context->command.mgmtPortConfig.mgmtPortAutoNeg = module_mgmt->mgmt_module_info[mgmt_id].mgmt_port_config_op.previousMgmtPortAutoNeg;
                mgmt_port_context->command.mgmtPortConfig.mgmtPortSpeed = module_mgmt->mgmt_module_info[mgmt_id].mgmt_port_config_op.previousMgmtPortSpeed;
                mgmt_port_context->command.mgmtPortConfig.mgmtPortDuplex = module_mgmt->mgmt_module_info[mgmt_id].mgmt_port_config_op.previousMgmtPortDuplexMode;
            }
            else
            {
                mgmt_port_context->command.mgmtPortConfig.mgmtPortAutoNeg = module_mgmt->mgmt_module_info[mgmt_id].mgmt_port_config_op.mgmtPortAutoNegRequested;
                mgmt_port_context->command.mgmtPortConfig.mgmtPortSpeed = module_mgmt->mgmt_module_info[mgmt_id].mgmt_port_config_op.mgmtPortSpeedRequested;
                mgmt_port_context->command.mgmtPortConfig.mgmtPortDuplex = module_mgmt->mgmt_module_info[mgmt_id].mgmt_port_config_op.mgmtPortDuplexModeRequested;
            }
            if(!(module_mgmt->mgmt_module_info[mgmt_id].mgmt_port_config_op.mgmtPortConfigInProgress))
            {
                module_mgmt->mgmt_module_info[mgmt_id].mgmt_port_config_op.configRequestTimeStamp = fbe_get_time();
                module_mgmt->mgmt_module_info[mgmt_id].mgmt_port_config_op.mgmtPortConfigInProgress = FBE_TRUE;
            }
            mgmt_port_context->command.portIDType = MANAGEMENT_PORT0;
            mgmt_port_context->command.sp_id = module_mgmt->base_environment.spid;
            mgmt_port_context->command.mgmtID = module_mgmt->mgmt_module_info[mgmt_id].mgmt_port_config_op.mgmtId;

            fbe_base_object_trace(base_object, 
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "set_mgmt_port_speed setting SP:%d Mgmt:%d AutoNeg:%d PortSpeed:%d Duplex:%d\n",
                                  mgmt_port_context->command.sp_id,
                                  mgmt_port_context->command.mgmtID,
                                  mgmt_port_context->command.mgmtPortConfig.mgmtPortAutoNeg,
                                  mgmt_port_context->command.mgmtPortConfig.mgmtPortSpeed,
                                  mgmt_port_context->command.mgmtPortConfig.mgmtPortDuplex);
                   
            mgmt_port_context->object_context = module_mgmt;
            mgmt_port_context->completion_function = fbe_module_mgmt_set_mgmt_port_speed_completion;
            status = fbe_api_board_set_mgmt_module_port_async(module_mgmt->board_object_id,
                                                              mgmt_port_context); 
            if(status != FBE_STATUS_PENDING)
            {
                fbe_base_object_trace(base_object, 
                                      FBE_TRACE_LEVEL_ERROR,
                                      FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                      "%s, failed to send mgmt port speed set command\n",
                                      __FUNCTION__);
            }
            /* Initialized the command so changed the sendMgmtPortConfig to false
             * If need to initialized it again that will handle in completion function 
             * by making this variable TRUE
             */
             module_mgmt->mgmt_module_info[mgmt_id].mgmt_port_config_op.sendMgmtPortConfig = FALSE;
        }
    }
    return FBE_LIFECYCLE_STATUS_DONE;
}
/**************************************************************
 *  end of fbe_module_mgmt_set_mgmt_port_speed_cond_function()
 *************************************************************/
/*!***************************************************************
 * fbe_module_mgmt_set_mgmt_port_speed_completion()
 ****************************************************************
 * @brief
 *  This is completion function for mgmt port speed change condition.
 *
 * @param command_context - Ptr to fbe_board_mgmt_set_mgmt_port_async_context_t
 *                          command context
 *
 * @return - void
 *
 * @author 
 *  25-Mar-2010: Created  Vaibhav Gaonkar
 *
 ****************************************************************/
void 
fbe_module_mgmt_set_mgmt_port_speed_completion(fbe_board_mgmt_set_mgmt_port_async_context_t  *command_context)    
{
    fbe_u32_t       mgmt_id;
    fbe_u32_t       timeElapsed;
    fbe_u8_t        deviceStr[FBE_DEVICE_STRING_LENGTH];
    fbe_status_t    status;
    fbe_module_mgmt_t *module_mgmt = (fbe_module_mgmt_t *)command_context->object_context;
    fbe_base_object_t *base_object = (fbe_base_object_t *)module_mgmt;
    fbe_mgmt_module_info_t         *mgmt_module_info_p;
    fbe_device_physical_location_t pLocation;

    fbe_zero_memory(&deviceStr, (FBE_DEVICE_STRING_LENGTH));
    pLocation.sp = module_mgmt->base_environment.spid;
    pLocation.slot = command_context->command.mgmtID;
    /* Get the device string */
    status = fbe_base_env_create_device_string(FBE_DEVICE_TYPE_MGMT_MODULE, 
                                               &pLocation, 
                                               &(deviceStr[0]), 
                                               FBE_DEVICE_STRING_LENGTH);

    fbe_base_object_trace((fbe_base_object_t *)module_mgmt, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s, entry for %s status 0x%X\n",
                              __FUNCTION__, &(deviceStr[0]), command_context->status);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)module_mgmt, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s, Failed to device string for mgmt module, status: 0x%X\n",
                              __FUNCTION__, status);
    }

    /* Get the mgmt_id for which get response */
    mgmt_id = command_context->command.mgmtID;
    mgmt_module_info_p = &(module_mgmt->mgmt_module_info[mgmt_id]);

    if(mgmt_module_info_p->mgmt_port_config_op.mgmtPortConfigInProgress)
    {
        if(command_context->status == FBE_STATUS_OK)
        {
            if(mgmt_module_info_p->mgmt_port_config_op.revertMgmtPortConfig)
            {
                /* Revert complete */
                fbe_base_object_trace((fbe_base_object_t*)module_mgmt, 
                                      FBE_TRACE_LEVEL_INFO,
                                      FBE_TRACE_MESSAGE_ID_INFO,
                                      "MgmtPortConfig for MgmtID %d, revert mgmt port succeeded\n",
                                      mgmt_id);
                fbe_event_log_write(ESP_INFO_MGMT_PORT_RESTORE_COMPLETED, 
                                    NULL, 
                                    0, 
                                    "%s",
                                    &(deviceStr[0]));

                /* The config failed but the revert was successful. 
                 * set FBE_STATUS_GENERIC_FAILURE to return to the caller.
                 */
                status = FBE_STATUS_GENERIC_FAILURE;
            }
            else
            {
                fbe_base_object_trace((fbe_base_object_t*)module_mgmt, 
                                      FBE_TRACE_LEVEL_INFO,
                                      FBE_TRACE_MESSAGE_ID_INFO,
                                      "MgmtPortConfig for MgmtID %d, config mgmt port succeeded\n",
                                      mgmt_id);
                fbe_event_log_write(ESP_INFO_MGMT_PORT_CONFIG_COMPLETED, 
                                    NULL, 
                                    0, 
                                    "%s",
                                    &(deviceStr[0]));

                /* The config failed but the revert was successful. 
                 * set FBE_STATUS_GENERIC_FAILURE to return to the caller.
                 */
                status = FBE_STATUS_OK;

                /* Persist the port speed change */
                fbe_module_mgmt_persist_mgmt_port_speed(module_mgmt);
            }            

            mgmt_module_info_p->mgmt_port_config_op.mgmtPortConfigInProgress = FBE_FALSE;

            /* Release the packet that used to track the port speed change */
            // BRION - todo this is a bug waiting to happen, this assumes the request came from the user
            fbe_module_mgmt_check_usurper_queue(module_mgmt,
                                               FBE_ESP_MODULE_MGMT_CONTROL_CODE_SET_MGMT_PORT_CONFIG,
                                               status);
            /* Reinitialized mgmt_port_config_op */
            fbe_module_mgmt_init_mgmt_port_config_op(&(mgmt_module_info_p->mgmt_port_config_op));            
            /* Release command context memory */
            fbe_base_env_memory_ex_release((fbe_base_environment_t *)module_mgmt, command_context);
            return;
        }
        else
        {
           /* Check the time stamp */
            timeElapsed = fbe_get_elapsed_seconds(mgmt_module_info_p->mgmt_port_config_op.configRequestTimeStamp);
            if(timeElapsed <= FBE_ESP_MGMT_PORT_CONFIG_TIMEOUT)
            {
                /* Set port speed change command failed so initializ it again */
                mgmt_module_info_p->mgmt_port_config_op.sendMgmtPortConfig = FBE_TRUE;

                fbe_base_object_trace(base_object, 
                                      FBE_TRACE_LEVEL_INFO,
                                      FBE_TRACE_MESSAGE_ID_INFO,
                                      "MgmtPortConfig for MgmtID %d failed retrying, status: 0x%X\n",
                                      mgmt_id, command_context->status);

                /* Release command context memory */
                fbe_base_env_memory_ex_release((fbe_base_environment_t *)module_mgmt, command_context);

                
                /* Set port command failed set it again */
                status = fbe_lifecycle_set_cond(&fbe_module_mgmt_lifecycle_const,
                                      (fbe_base_object_t*)module_mgmt,
                                       FBE_MODULE_MGMT_LIFECYCLE_COND_SET_MGMT_PORT_SPEED);
                if(status != FBE_STATUS_OK)
                {
                    fbe_base_object_trace(base_object, 
                                          FBE_TRACE_LEVEL_ERROR,
                                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                          "Failed to set condition to change MGMT port speed, status: 0x%X\n",
                                          status);
                }
                return;
            }
            else if(timeElapsed > FBE_ESP_MGMT_PORT_CONFIG_TIMEOUT &&
                    timeElapsed <= (2* FBE_ESP_MGMT_PORT_CONFIG_TIMEOUT) &&
                    mgmt_module_info_p->mgmt_port_config_op.revertEnabled)
            {
                if(!mgmt_module_info_p->mgmt_port_config_op.revertMgmtPortConfig)
                {
                    fbe_event_log_write(ESP_INFO_MGMT_PORT_CONFIG_FAILED, 
                                        NULL, 
                                        0, 
                                        "%s",
                                        &(deviceStr[0]));
                    fbe_base_object_trace((fbe_base_object_t*)module_mgmt, 
                                           FBE_TRACE_LEVEL_INFO,
                                           FBE_TRACE_MESSAGE_ID_INFO,
                                           "MgmtPortConfig for MgmtID %d,  Config mgmt port failed,will revert\n",
                                           mgmt_id);

                    /* Start the revert */
                    mgmt_module_info_p->mgmt_port_config_op.revertMgmtPortConfig = TRUE;
                }
                /* Release command context memory */
                fbe_base_env_memory_ex_release((fbe_base_environment_t *)module_mgmt, command_context);

                /* Set port speed change command failed so initializ it again */
                mgmt_module_info_p->mgmt_port_config_op.sendMgmtPortConfig = FBE_TRUE;
                status = fbe_lifecycle_set_cond(&fbe_module_mgmt_lifecycle_const,
                                                (fbe_base_object_t*)module_mgmt,
                                                FBE_MODULE_MGMT_LIFECYCLE_COND_SET_MGMT_PORT_SPEED);
                if(status != FBE_STATUS_OK)
                {
                    fbe_base_object_trace(base_object, 
                                          FBE_TRACE_LEVEL_ERROR,
                                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                          "Failed to set condition to change MGMT port speed, status: 0x%X\n",
                                          status);
                }
                return;
            }
            else
            {
                if(!mgmt_module_info_p->mgmt_port_config_op.revertEnabled)
                {
                    fbe_base_object_trace((fbe_base_object_t*)module_mgmt, 
                                           FBE_TRACE_LEVEL_INFO,
                                           FBE_TRACE_MESSAGE_ID_INFO,
                                           "MgmtPortConfig for MgmtID %d,  Config mgmt port failed,no revert\n",
                                           mgmt_id);
                    fbe_event_log_write(ESP_INFO_MGMT_PORT_CONFIG_FAILED, 
                                        NULL, 
                                        0, 
                                        "%s",
                                        &(deviceStr[0]));
                }
                else
                {
                    fbe_base_object_trace((fbe_base_object_t*)module_mgmt, 
                                           FBE_TRACE_LEVEL_INFO,
                                           FBE_TRACE_MESSAGE_ID_INFO,
                                           "MgmtPortConfig for MgmtID %d, Revert mgmt port failed\n",
                                           mgmt_id);
                    
                    fbe_event_log_write(ESP_INFO_MGMT_PORT_RESTORE_FAILED, 
                                        NULL, 
                                        0, 
                                        "%s",
                                        &(deviceStr[0]));
                }
                fbe_module_mgmt_check_usurper_queue(module_mgmt,
                                                    FBE_ESP_MODULE_MGMT_CONTROL_CODE_SET_MGMT_PORT_CONFIG,
                                                    FBE_STATUS_GENERIC_FAILURE);

                fbe_base_env_memory_ex_release((fbe_base_environment_t *)module_mgmt, command_context);
                /* Reinitialized mgmt_port_config_op */
                fbe_module_mgmt_init_mgmt_port_config_op(&(mgmt_module_info_p->mgmt_port_config_op));
                return;
            }
        }
    }
}
/**************************************************************
 *  end of fbe_module_mgmt_set_mgmt_port_speed_completion()
 *************************************************************/
 /*!***************************************************************
 * fbe_module_mgmt_check_usurper_queue()
 ****************************************************************
 * @brief
 *  This is check the usuper queue for particular packet; if found then
 *  complete the packet with given status.
 *
 * @param module_mgmt - Ptr to base object of module_mgmt
 * @param required_control_code - Control code for which packet search in queue.
 *
 * @return - fbe_staus_t - FBE_STATUS_OK if required packet found on queue.
 *
 * @author 
 *  25-Mar-2010: Created  Vaibhav Gaonkar
 *
 ****************************************************************/
fbe_status_t    
fbe_module_mgmt_check_usurper_queue(fbe_module_mgmt_t * module_mgmt,
                                   fbe_esp_module_mgmt_control_code_t required_control_code,
                                    fbe_status_t packet_status)
{
    fbe_base_object_t     * base_object = (fbe_base_object_t *)module_mgmt;
    fbe_packet_t          * usurper_packet = NULL;
    fbe_payload_ex_t         * payload = NULL;    
    fbe_queue_head_t      * queue_head = NULL;
    fbe_queue_element_t   * queue_element = NULL;
    fbe_payload_control_operation_t       * control_operation = NULL;
    fbe_payload_control_operation_opcode_t  control_code = 0;
    
    queue_head = fbe_base_object_get_usurper_queue_head(base_object);
    fbe_base_object_usurper_queue_lock(base_object);
    queue_element = fbe_queue_front(queue_head);
    while(queue_element)
    {
        usurper_packet = fbe_transport_queue_element_to_packet(queue_element);
        control_code = fbe_transport_get_control_code(usurper_packet);
        payload = fbe_transport_get_payload_ex(usurper_packet);
        control_operation = fbe_payload_ex_get_control_operation(payload);
        if(required_control_code == control_code)
        {
            fbe_base_object_usurper_queue_unlock(base_object);
            fbe_base_object_remove_from_usurper_queue(base_object, usurper_packet);
            
            fbe_base_object_trace((fbe_base_object_t *)module_mgmt, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s, complete usurper packet, ctrlCode 0x%X, status 0x%X.\n",
                              __FUNCTION__, required_control_code, packet_status);

            fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
            fbe_transport_set_status(usurper_packet, packet_status, 0);
            fbe_transport_complete_packet(usurper_packet);
            return FBE_STATUS_OK;
        }
        queue_element = fbe_queue_next(queue_head, queue_element);
    }
    fbe_base_object_usurper_queue_unlock(base_object);
    return FBE_STATUS_GENERIC_FAILURE;
}
/*************************************************
 *  end of fbe_module_mgmt_check_usurper_queue()
 ************************************************/
 
/*!***************************************************************
 * fbe_module_mgmt_log_mgmt_module_event()
 ****************************************************************
 * @brief
 *  This is function log the event for mgmt module data change.
 *
 * @param mgmt_module - Ptr to  module_mgmt
 * @param new_mgmt_module_comp_info - Ptr to change data  
 * @param old_mgmt_module_comp_info - Ptr to existing data
 *
 * @return - void - 
 *
 * @author 
 *  25-Mar-2010: Created  Vaibhav Gaonkar
 *
 ****************************************************************/
 void
 fbe_module_mgmt_log_mgmt_module_event(fbe_module_mgmt_t * module_mgmt,
                                       fbe_board_mgmt_mgmt_module_info_t *new_mgmt_module_comp_info,
                                       fbe_board_mgmt_mgmt_module_info_t *old_mgmt_module_comp_info)
{
    fbe_u8_t        deviceStr[FBE_DEVICE_STRING_LENGTH];
    fbe_status_t    status;
    fbe_device_physical_location_t pLocation;

    fbe_zero_memory(&deviceStr, (FBE_DEVICE_STRING_LENGTH));
    pLocation.sp = new_mgmt_module_comp_info->associatedSp;
    pLocation.slot = new_mgmt_module_comp_info->mgmtID;
    /* Get the device string */
    status = fbe_base_env_create_device_string(FBE_DEVICE_TYPE_MGMT_MODULE, 
                                               &pLocation, 
                                               &(deviceStr[0]), 
                                               FBE_DEVICE_STRING_LENGTH);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)module_mgmt, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s, Failed to device string for mgmt module, status: 0x%X\n",
                              __FUNCTION__, status);
    }

    if (new_mgmt_module_comp_info->envInterfaceStatus != old_mgmt_module_comp_info->envInterfaceStatus)
    {
        if (new_mgmt_module_comp_info->envInterfaceStatus == FBE_ENV_INTERFACE_STATUS_XACTION_FAIL)
        {
            fbe_event_log_write(ESP_ERROR_ENV_INTERFACE_FAILURE_DETECTED,
                                NULL, 0,
                                "%s %s", 
                                &deviceStr[0],
                                SmbTranslateErrorString(new_mgmt_module_comp_info->transactionStatus));
        }
        else if (new_mgmt_module_comp_info->envInterfaceStatus != FBE_ENV_INTERFACE_STATUS_GOOD)
        {
            fbe_event_log_write(ESP_ERROR_ENV_INTERFACE_FAILURE_DETECTED,
                                NULL, 0,
                                "%s %s", 
                                &deviceStr[0],
                                fbe_base_env_decode_env_status(new_mgmt_module_comp_info->envInterfaceStatus));
        }
        else
        {
            fbe_event_log_write(ESP_INFO_ENV_INTERFACE_FAILURE_CLEARED,
                                NULL, 0,
                                "%s", 
                                &deviceStr[0]);
        }
    }

    if(new_mgmt_module_comp_info->bInserted)
    {
        if(!old_mgmt_module_comp_info->bInserted)
        {
            fbe_event_log_write(ESP_INFO_MGMT_FRU_INSERTED, 
                                NULL, 
                                0, 
                                "%s",
                                &(deviceStr[0]));
            if(new_mgmt_module_comp_info->generalFault)
            {
                fbe_event_log_write(ESP_ERROR_MGMT_FRU_FAULTED,
                                    NULL, 
                                    0, 
                                    "%s",
                                    &(deviceStr[0]));
            }
        }
        else
        {
            if((new_mgmt_module_comp_info->generalFault) &&
               (!old_mgmt_module_comp_info->generalFault))
            {
                fbe_event_log_write(ESP_ERROR_MGMT_FRU_FAULTED,
                                    NULL, 
                                    0, 
                                    "%s",
                                    &(deviceStr[0]));
            }
            else if((!new_mgmt_module_comp_info->generalFault) &&
                    (old_mgmt_module_comp_info->generalFault))
            {
                fbe_event_log_write(ESP_INFO_MGMT_FRU_FAULT_CLEARED, 
                                    NULL, 
                                    0, 
                                    "%s",
                                    &(deviceStr[0]));
            }
        }
        
        /* Check if there is network connection change and log it.*/
        if(new_mgmt_module_comp_info->envInterfaceStatus == FBE_ENV_INTERFACE_STATUS_GOOD &&
           old_mgmt_module_comp_info->envInterfaceStatus == FBE_ENV_INTERFACE_STATUS_GOOD)
        {
            /* Check if management port link up or down and log it.*/
            if((new_mgmt_module_comp_info->managementPort.linkStatus) &&
               (!old_mgmt_module_comp_info->managementPort.linkStatus))
            {
                fbe_event_log_write(ESP_INFO_MGMT_FRU_LINK_UP, 
                                    NULL, 
                                    0, 
                                    "%s",
                                    &(deviceStr[0]));
            }
            else if((!new_mgmt_module_comp_info->managementPort.linkStatus) &&
                     (old_mgmt_module_comp_info->managementPort.linkStatus))
            {
                fbe_event_log_write(ESP_INFO_MGMT_FRU_NO_NETWORK_CONNECTION, 
                                    NULL, 
                                    0, 
                                    "%s",
                                    &(deviceStr[0]));
            }

            /* Check if service port link up or down and log it. */
            if((new_mgmt_module_comp_info->servicePort.linkStatus) &&
               (!old_mgmt_module_comp_info->servicePort.linkStatus))
            {
                fbe_event_log_write(ESP_INFO_MGMT_FRU_SERVICE_PORT_LINK_UP, 
                                    NULL, 
                                    0, 
                                    "%s",
                                    &(deviceStr[0]));
            }
            else if((!new_mgmt_module_comp_info->servicePort.linkStatus) &&
                    (old_mgmt_module_comp_info->servicePort.linkStatus))
            {
                fbe_event_log_write(ESP_INFO_MGMT_FRU_SERVICE_PORT_NO_LINK, 
                                    NULL, 
                                    0, 
                                    "%s",
                                    &(deviceStr[0]));
            }
        }
    }
    else if((!new_mgmt_module_comp_info->bInserted) &&
             (old_mgmt_module_comp_info->bInserted))
    {
        fbe_event_log_write(ESP_ERROR_MGMT_FRU_REMOVED,
                            NULL, 
                            0, 
                            "%s",
                            &(deviceStr[0]));
    }
    return;
}
/*************************************************
 *  end of fbe_module_mgmt_log_mgmt_module_event()
 ************************************************/


/*!***************************************************************
 * fbe_module_mgmt_handle_sfp_state_change()
 ****************************************************************
 * @brief
 *  This function compares the previous sfp data to the new
 *  sfp data and kicks off led changes, events and port state
 *  changes when necessary
 *
 * @param module_mgmt - Ptr to  module_mgmt
 * @param iom_num - io module index
 * @param port_num - port number
 * @param old_sfp_info - Ptr to previous sfp information
 * @param new_sfp_info - Ptr to new sfp information
 *
 * @return - fbe_status_t - 
 *
 * @author 
 *  15-June-2011: Created  bphilbin
 ****************************************************************/
static fbe_status_t fbe_module_mgmt_handle_sfp_state_change(fbe_module_mgmt_t *module_mgmt,
                                                    fbe_u32_t iom_num,
                                                    fbe_u32_t port_num, 
                                                    fbe_port_sfp_info_t *old_sfp_info, 
                                                    fbe_port_sfp_info_t *new_sfp_info)
{
    fbe_u32_t port_index;
    fbe_bool_t update_port_state = FALSE;

    port_index = fbe_module_mgmt_get_io_port_index(iom_num, port_num);

    fbe_base_object_trace((fbe_base_object_t *)module_mgmt,
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s IOM:%d Port:%d sfp cond:0x%x subcond:0x%x\n",
                                  __FUNCTION__, 
                                  iom_num, port_num,
                                  new_sfp_info->condition_type,
                                  new_sfp_info->sub_condition_type);
    if((new_sfp_info->condition_type == FBE_PORT_SFP_CONDITION_REMOVED) ||
       (new_sfp_info->condition_type == FBE_PORT_SFP_CONDITION_INVALID))
    {
        module_mgmt->io_port_info[port_index].port_physical_info.sfp_inserted = FALSE;
        if(new_sfp_info->condition_type != old_sfp_info->condition_type)
        {
            update_port_state = TRUE;
            fbe_event_log_write(ESP_INFO_SFP_REMOVED,
                                 NULL, 0,
                                 "%s %d %d", 
                                 fbe_module_mgmt_device_type_to_string(
                                     fbe_module_mgmt_get_device_type(module_mgmt, module_mgmt->local_sp, iom_num)),
                                 fbe_module_mgmt_get_slot_num(module_mgmt, module_mgmt->local_sp, iom_num),
                                 port_num);
                                 
            // if SFP removed, update port LED

        }
    }
    else if(new_sfp_info->condition_type == FBE_PORT_SFP_CONDITION_FAULT)
    {
        module_mgmt->io_port_info[port_index].port_physical_info.sfp_inserted = TRUE;

        if(new_sfp_info->condition_additional_info == FBE_PORT_SFP_SUBCONDITION_DEVICE_ERROR)
        {
            //TODO
            //update LED informing the user that the SFP is faulted
        }
        else if(new_sfp_info->condition_additional_info == FBE_PORT_SFP_SUBCONDITION_UNSUPPORTED)
        {
            //TODO
            //update LED informing the user that the SFP is unsupported

            /*
             * If these are unconfigured CNA ports and we have FC or iSCSI SFPs the fault needs to be cleared as 
             * the miniport cannot support the other SFP type but it is technically not a fault, because when the
             * protocol changes as the port is configured the SFP will then be supported.
             */

            fbe_base_object_trace((fbe_base_object_t *)module_mgmt,
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s UNSUPP SFP Is CNA %d Initialized %d CNA Capable %d\n",
                                  __FUNCTION__, 
                                  fbe_module_mgmt_is_cna_port(module_mgmt,iom_num,port_num), 
                                  fbe_module_mgmt_is_port_initialized(module_mgmt, module_mgmt->local_sp, iom_num, port_num), 
                                  fbe_module_mgmt_is_sfp_cna_capable(module_mgmt, module_mgmt->local_sp, iom_num, port_num));
            if (fbe_module_mgmt_is_cna_port(module_mgmt,iom_num,port_num) &&
                (!fbe_module_mgmt_is_port_initialized(module_mgmt, module_mgmt->local_sp, iom_num, port_num)) &&
                fbe_module_mgmt_is_sfp_cna_capable(module_mgmt, module_mgmt->local_sp, iom_num, port_num))
            {
                // Ignore this status and simply set the previous state (should be inserted)
                fbe_base_object_trace((fbe_base_object_t *)module_mgmt,
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s UNSUPP SFP Is UNC CNA forcing SFP data back to previous information.\n",
                                  __FUNCTION__);
                fbe_copy_memory(new_sfp_info, old_sfp_info, sizeof(new_sfp_info));
                return FBE_STATUS_OK; 
            }
        }
        if(new_sfp_info->condition_type != old_sfp_info->condition_type)
        {
            update_port_state = TRUE;
            fbe_event_log_write(ESP_ERROR_SFP_FAULTED,
                                 NULL, 0,
                                 "%s %d %d", 
                                 fbe_module_mgmt_device_type_to_string(
                                     fbe_module_mgmt_get_device_type(module_mgmt, module_mgmt->local_sp, iom_num)),
                                 fbe_module_mgmt_get_slot_num(module_mgmt, module_mgmt->local_sp, iom_num),
                                 port_num);
        }
        
    }
    else // Either Good, Inserted or something equivalent
    {
        
        module_mgmt->io_port_info[port_index].port_physical_info.sfp_inserted = TRUE;

        if(new_sfp_info->condition_additional_info == FBE_PORT_SFP_SUBCONDITION_CHECKSUM_PENDING)
        {
            //nothing to do here, we are waiting for valid data
        }
        else
        {
            if( (old_sfp_info->condition_type == FBE_PORT_SFP_CONDITION_REMOVED) || 
                (old_sfp_info->condition_type == FBE_PORT_SFP_CONDITION_FAULT) ||
                (old_sfp_info->condition_additional_info == FBE_PORT_SFP_SUBCONDITION_CHECKSUM_PENDING))
            {
                //TODO
                // we are transitioning from a different state set LEDs change port state and log an event
                update_port_state = TRUE;
                fbe_event_log_write(ESP_INFO_SFP_INSERTED,
                                 NULL, 0,
                                 "%s %d %d", 
                                 fbe_module_mgmt_device_type_to_string(
                                     fbe_module_mgmt_get_device_type(module_mgmt, module_mgmt->local_sp, iom_num)),
                                 fbe_module_mgmt_get_slot_num(module_mgmt, module_mgmt->local_sp, iom_num),
                                 port_num);
            }
        }
    }
    if(update_port_state)
    {
        old_sfp_info->condition_type = new_sfp_info->condition_type;
        // something has changed, update the port state and substate
        if(port_num != INVALID_PORT_U8)
        {
            fbe_module_mgmt_check_port_state(module_mgmt, module_mgmt->local_sp, iom_num, port_num);
        }
    }
    return FBE_STATUS_OK;
}
/***************************************************
 *  end of fbe_module_mgmt_handle_sfp_state_change()
 ***************************************************/

/*!***************************************************************
 * fbe_module_mgmt_check_for_combined_port()
 ****************************************************************
 * @brief
 *  This function checks new SFP data to determine if a combined
 *  port connector has been inserted.  This is determined by the
 *  IO module type and the SFP data indicating the part number of a
 *  combined connector and duplicate serial numbers for adjacent ports.
 *
 * @param module_mgmt - Ptr to  module_mgmt
 * @param iom_num - io module index
 * @param port_num - port number
 *
 * @return - fbe_status_t - 
 *
 * @author 
 *  15-June-2011: Created  bphilbin
 ****************************************************************/
static void fbe_module_mgmt_check_for_combined_port(fbe_module_mgmt_t *module_mgmt,
                                                    fbe_u32_t iom_index,
                                                    fbe_u32_t port_num)
{
    fbe_u32_t port_index;
    fbe_u32_t next_port_index;
    fbe_bool_t found_combined_port = FBE_FALSE;
    fbe_u8_t primary_emc_part_number[FBE_PORT_SFP_EMC_DATA_LENGTH];
    fbe_u8_t primary_emc_serial_number[FBE_PORT_SFP_EMC_DATA_LENGTH];
    fbe_u8_t secondary_emc_part_number[FBE_PORT_SFP_EMC_DATA_LENGTH];
    fbe_u8_t secondary_emc_serial_number[FBE_PORT_SFP_EMC_DATA_LENGTH];
    fbe_u8_t connector_type;

    port_index = fbe_module_mgmt_get_io_port_index(iom_index, port_num);

    if(port_index != INVALID_PORT_U8)
    {
        
        if( (module_mgmt->io_port_info[port_index].port_physical_info.port_comp_info[module_mgmt->local_sp].sfpStatus.inserted == FBE_TRUE) &&
            ((fbe_module_mgmt_get_slic_type(module_mgmt, module_mgmt->local_sp, iom_index) == FBE_SLIC_TYPE_6G_SAS_3) ||
             (fbe_module_mgmt_get_slic_type(module_mgmt, module_mgmt->local_sp, iom_index) == FBE_SLIC_TYPE_12G_SAS )))
        {
            connector_type = decodeConnectorType(&module_mgmt->io_port_info[port_index].port_physical_info.port_comp_info[module_mgmt->local_sp].sfpStatus);

            decodeEMCPartNumber(&module_mgmt->io_port_info[port_index].port_physical_info.port_comp_info[module_mgmt->local_sp].sfpStatus, 
                                connector_type, primary_emc_part_number);
            decodeEMCSerialNumber(&module_mgmt->io_port_info[port_index].port_physical_info.port_comp_info[module_mgmt->local_sp].sfpStatus, 
                                  connector_type, primary_emc_serial_number);
            if( (module_mgmt->io_port_info[port_index].port_physical_info.port_comp_info[module_mgmt->local_sp].sfpStatus.type == 2) &&
                (module_mgmt->io_port_info[port_index].port_physical_info.port_comp_info[module_mgmt->local_sp].sfpStatus.type2.miniSasPage00.transactionStatus == EMCPAL_STATUS_SUCCESS) &&
                (strlen(primary_emc_part_number) > 0) && (strlen(primary_emc_serial_number) > 0) )
            {
    
                if( ((port_num + 1) < MAX_IO_PORTS_PER_MODULE) &&
                    (!module_mgmt->io_port_info[port_index].port_logical_info[module_mgmt->local_sp].combined_port))
                {
                    next_port_index = fbe_module_mgmt_get_io_port_index(iom_index, port_num + 1);
                    if(next_port_index != INVALID_PORT_U8) 
                    {
                        connector_type = decodeConnectorType(&module_mgmt->io_port_info[next_port_index].port_physical_info.port_comp_info[module_mgmt->local_sp].sfpStatus);
    
                        decodeEMCPartNumber(&module_mgmt->io_port_info[next_port_index].port_physical_info.port_comp_info[module_mgmt->local_sp].sfpStatus, 
                                            connector_type, secondary_emc_part_number);
                        decodeEMCSerialNumber(&module_mgmt->io_port_info[next_port_index].port_physical_info.port_comp_info[module_mgmt->local_sp].sfpStatus, 
                                              connector_type, secondary_emc_serial_number);
    
                        if( (fbe_compare_string(secondary_emc_part_number, 
                                                FBE_PORT_SFP_EMC_DATA_LENGTH, 
                                                primary_emc_part_number, 
                                                FBE_PORT_SFP_EMC_DATA_LENGTH, FBE_FALSE) == 0) &&
                            (fbe_compare_string(secondary_emc_serial_number, 
                                                FBE_PORT_SFP_EMC_DATA_LENGTH, 
                                                primary_emc_serial_number, 
                                                FBE_PORT_SFP_EMC_DATA_LENGTH, FBE_FALSE) == 0))
                        {
                            fbe_base_object_trace((fbe_base_object_t *)module_mgmt,
                                                    FBE_TRACE_LEVEL_INFO,
                                                    FBE_TRACE_MESSAGE_ID_INFO,
                                                    "%s Found combined connector at IOM:%d Port:%d and Port%d\n",
                                                    __FUNCTION__, iom_index, port_num, port_num+1);
                    
                            module_mgmt->io_port_info[port_index].port_logical_info[module_mgmt->local_sp].combined_port = FBE_TRUE;
                            module_mgmt->io_port_info[next_port_index].port_logical_info[module_mgmt->local_sp].combined_port = FBE_TRUE;
                            module_mgmt->io_port_info[port_index].port_logical_info[module_mgmt->local_sp].secondary_combined_port = port_num + 1;
                            module_mgmt->io_port_info[next_port_index].port_logical_info[module_mgmt->local_sp].secondary_combined_port = port_num;
                            found_combined_port = FBE_TRUE;
                        }
                    }
                }
                if( port_num >= 1)
                {
                    next_port_index = fbe_module_mgmt_get_io_port_index(iom_index, port_num - 1);
                    if( (next_port_index != INVALID_PORT_U8) &&
                        (module_mgmt->io_port_info[next_port_index].port_physical_info.port_comp_info[module_mgmt->local_sp].sfpStatus.inserted))
                        
                    {
                        connector_type = decodeConnectorType(&module_mgmt->io_port_info[next_port_index].port_physical_info.port_comp_info[module_mgmt->local_sp].sfpStatus);
    
                        decodeEMCPartNumber(&module_mgmt->io_port_info[next_port_index].port_physical_info.port_comp_info[module_mgmt->local_sp].sfpStatus, 
                                            connector_type, secondary_emc_part_number);
                        decodeEMCSerialNumber(&module_mgmt->io_port_info[next_port_index].port_physical_info.port_comp_info[module_mgmt->local_sp].sfpStatus, 
                                              connector_type, secondary_emc_serial_number);
    
                        if( (fbe_compare_string(secondary_emc_part_number, 
                                                FBE_PORT_SFP_EMC_DATA_LENGTH, 
                                                primary_emc_part_number, 
                                                FBE_PORT_SFP_EMC_DATA_LENGTH, FBE_FALSE) == 0) &&
                            (fbe_compare_string(secondary_emc_serial_number, 
                                                FBE_PORT_SFP_EMC_DATA_LENGTH, 
                                                primary_emc_serial_number, 
                                                FBE_PORT_SFP_EMC_DATA_LENGTH, FBE_FALSE) == 0))
                        {
                            fbe_base_object_trace((fbe_base_object_t *)module_mgmt,
                                                    FBE_TRACE_LEVEL_INFO,
                                                    FBE_TRACE_MESSAGE_ID_INFO,
                                                    "%s Found combined connector at IOM:%d Port:%d and Port%d\n",
                                                    __FUNCTION__, iom_index, port_num, port_num-1);
                            module_mgmt->io_port_info[port_index].port_logical_info[module_mgmt->local_sp].combined_port = FBE_TRUE;
                            module_mgmt->io_port_info[next_port_index].port_logical_info[module_mgmt->local_sp].combined_port = FBE_TRUE;
                            module_mgmt->io_port_info[port_index].port_logical_info[module_mgmt->local_sp].secondary_combined_port = port_num - 1;
                            module_mgmt->io_port_info[next_port_index].port_logical_info[module_mgmt->local_sp].secondary_combined_port = port_num;
                            found_combined_port = FBE_TRUE;
                        }
                    }
                }
            }
            
            if(!found_combined_port)
            {
                fbe_base_object_trace((fbe_base_object_t *)module_mgmt,
                                                    FBE_TRACE_LEVEL_DEBUG_LOW,
                                                    FBE_TRACE_MESSAGE_ID_INFO,
                                                    "%s NOT a combined connector at IOM:%d Port:%d\n",
                                                    __FUNCTION__, iom_index, port_num);
                module_mgmt->io_port_info[port_index].port_logical_info[module_mgmt->local_sp].combined_port = FBE_FALSE;
                module_mgmt->io_port_info[port_index].port_logical_info[module_mgmt->local_sp].secondary_combined_port = INVALID_PORT_U8;
                if( (module_mgmt->io_port_info[port_index].port_physical_info.port_comp_info[module_mgmt->local_sp].sfpStatus.type == 2) &&
                    (module_mgmt->io_port_info[port_index].port_physical_info.port_comp_info[module_mgmt->local_sp].sfpStatus.type2.miniSasPage00.transactionStatus != EMCPAL_STATUS_SUCCESS))
                {
                    fbe_base_object_trace((fbe_base_object_t *)module_mgmt,
                                            FBE_TRACE_LEVEL_INFO,
                                            FBE_TRACE_MESSAGE_ID_INFO,
                                            "ModMgmt: MiniSASPage has bad TransactionStatus 0x%x\n",
                                            module_mgmt->io_port_info[port_index].port_physical_info.port_comp_info[module_mgmt->local_sp].sfpStatus.type2.miniSasPage00.transactionStatus);

                }

            }
        }
    }
    else
    {
        fbe_base_object_trace((fbe_base_object_t *)module_mgmt, 
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s Fetching port index failed for IOM:%d Port:%d\n",
                                __FUNCTION__, iom_index, port_num);
    }
    return;

}
 /*!***************************************************************
 * fbe_module_mgmt_persist_mgmt_port_speed()
 ****************************************************************
 * @brief
 *  This is function stored the Mgmt port speed data on ESP lun.
 *
 * @param mgmt_module - Ptr to module_mgmt
 *
 * @return - void - 
 *
 * @author 
 *  2-June-2011: Created  Vaibhav Gaonkar
 *
 ****************************************************************/
void
fbe_module_mgmt_persist_mgmt_port_speed(fbe_module_mgmt_t *module_mgmt)
{
    SP_ID       localSpid;
    fbe_u32_t   mgmt_id;
    fbe_mgmt_module_info_t *mgmt_module_info_p;
    fbe_status_t    status;
    fbe_mgmt_port_config_info_t *persistent_data_p;

    localSpid = module_mgmt->base_environment.spid;

    persistent_data_p = (fbe_mgmt_port_config_info_t *) (((fbe_base_environment_t *)module_mgmt)->my_persistent_data +
                       FBE_MGMT_PORT_PERSISTENT_DATA_OFFSET);

    for(mgmt_id = 0; mgmt_id < FBE_ESP_MGMT_MODULE_MAX_COUNT_PER_SP; mgmt_id++)
    {
        //copy the config op into persistent data.
        mgmt_module_info_p = &(module_mgmt->mgmt_module_info[mgmt_id]);

        mgmt_module_info_p->mgmt_port_persistent_info.mgmtPortAutoNeg = module_mgmt->mgmt_module_info[mgmt_id].mgmt_port_config_op.mgmtPortAutoNegRequested;
        mgmt_module_info_p->mgmt_port_persistent_info.mgmtPortDuplex = module_mgmt->mgmt_module_info[mgmt_id].mgmt_port_config_op.mgmtPortDuplexModeRequested;
        mgmt_module_info_p->mgmt_port_persistent_info.mgmtPortSpeed = module_mgmt->mgmt_module_info[mgmt_id].mgmt_port_config_op.mgmtPortSpeedRequested;

        fbe_copy_memory(persistent_data_p,
                        &(mgmt_module_info_p->mgmt_port_persistent_info),
                        sizeof(fbe_mgmt_port_config_info_t));
        persistent_data_p++;
        fbe_base_object_trace((fbe_base_object_t *) module_mgmt,
                                FBE_TRACE_LEVEL_INFO,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "Wrote MgmtID:%d AutoNeg:%d Duplex:%d Speed%d to persistent storage\n",
                                mgmt_id,
                                module_mgmt->mgmt_module_info[mgmt_id].mgmt_port_persistent_info.mgmtPortAutoNeg,
                                module_mgmt->mgmt_module_info[mgmt_id].mgmt_port_persistent_info.mgmtPortDuplex,
                                module_mgmt->mgmt_module_info[mgmt_id].mgmt_port_persistent_info.mgmtPortSpeed);

    }
    status = fbe_base_env_write_persistent_data((fbe_base_environment_t *)module_mgmt);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)module_mgmt,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s persistent write error, status: 0x%X",
                              __FUNCTION__, status);
    }
}
/****************************************************
 *  end of fbe_module_mgmt_persist_mgmt_port_speed()
 ***************************************************/
 /*!***************************************************************
 * fbe_module_mgmt_init_mgmt_port_config_request_cond_function()
 ****************************************************************
 * @brief
 *  This is  condition function FBE_MODULE_MGMT_LIFECYCLE_COND_INIT_MGMT_PORT_CONFIG_REQUEST.
 *  It initializes the management port config request based on the persistence info.
 *  
 * @param base_object - Ptr to  base object
 * @param packet - Ptr to packet 
 *
 * @return - fbe_lifecycle_status_t - 
 *
 * @author 
 *  2-June-2011: Created  Vaibhav Gaonkar
 *
 ****************************************************************/
static fbe_lifecycle_status_t 
fbe_module_mgmt_init_mgmt_port_config_request_cond_function(fbe_base_object_t * base_object, 
                                                             fbe_packet_t * packet)
{
    fbe_u32_t       mgmt_id;
    fbe_status_t    status;
    fbe_module_mgmt_t   *module_mgmt = (fbe_module_mgmt_t *)base_object;
    fbe_mgmt_module_info_t *mgmt_module_info;
    
    fbe_base_object_trace(base_object,
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s Entry.\n",
                          __FUNCTION__);
    /* Clear condition */
    status = fbe_lifecycle_clear_current_cond(base_object);
    if (status != FBE_STATUS_OK) 
    {
        fbe_base_object_trace(base_object, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s, failed to clear condition %d\n", __FUNCTION__, status);                
    }

    /* Initialized the Mgmt port config that will configure the mgmt port */
    for(mgmt_id = 0; mgmt_id < module_mgmt->discovered_hw_limit.num_mgmt_modules; mgmt_id++)
    {
        mgmt_module_info = &(module_mgmt->mgmt_module_info[mgmt_id]);
        mgmt_module_info->mgmt_port_config_op.revertEnabled = TRUE;
        status = fbe_module_mgmt_convert_mgmt_port_config_request(module_mgmt,
                                                                  mgmt_id,
                                                                  FBE_REQUEST_SOURCE_INTERNAL,
                                                                  &(mgmt_module_info->mgmt_port_persistent_info));
        if(status != FBE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t*)module_mgmt, 
                                  FBE_TRACE_LEVEL_WARNING,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s,convert_mgmt_port_config_request failed.\n", __FUNCTION__);

            fbe_module_mgmt_init_mgmt_port_config_op(&mgmt_module_info->mgmt_port_config_op);
        }
    }    
    return FBE_LIFECYCLE_STATUS_DONE;
}
/************************************************************************
 *  end of fbe_module_mgmt_convert_mgmt_module_persistent_cond_function()
 ***********************************************************************/

 /*!***************************************************************
 * fbe_module_mgmt_init_mgmt_port_config_request_cond_function()
 ****************************************************************
 * @brief
 *  This function loops through the configured port data looking for duplicate
 *  back end bus numbers assigned to different ports.  If we find the duplicate
 *  we mark the ports as combined connectors.
 *  
 * @param module_mgmt - Ptr to our object context
 *
 * @return - N/A
 *
 * @author 
 *  05-December-2012: Created  Brion Philbin
 *
 ****************************************************************/
void fbe_module_mgmt_check_for_configured_combined_port(fbe_module_mgmt_t *module_mgmt)
{
    fbe_u32_t iom_num, port_num, port_index;
    fbe_u32_t log_num;
    fbe_u32_t primary_connector_port_num, primary_port_index;
    fbe_bool_t primary_port_found;
    fbe_u32_t combined_port_count;

    for(log_num=0;log_num<module_mgmt->platform_port_limit.max_sas_be; log_num++)
    {
        primary_port_found = FBE_FALSE;
        primary_connector_port_num = INVALID_PORT_U8;
        for(iom_num = 0; iom_num < FBE_ESP_IO_MODULE_MAX_COUNT; iom_num++)
        {
            for(port_num = 0; 
                 port_num < module_mgmt->io_module_info[iom_num].physical_info[module_mgmt->local_sp].module_comp_info.ioPortCount; 
                 port_num++)
            {
                port_index = fbe_module_mgmt_get_io_port_index(iom_num, port_num);
                if((module_mgmt->io_port_info[port_index].port_logical_info[module_mgmt->local_sp].port_configured_info.logical_num == log_num) &&
                       (module_mgmt->io_port_info[port_index].port_logical_info[module_mgmt->local_sp].port_configured_info.port_role == IOPORT_PORT_ROLE_BE))
                {
                    if(primary_port_found)
                    {
                        // we have found the secondary connector of this combined port pair mark it a combined port
                        module_mgmt->io_port_info[port_index].port_logical_info[module_mgmt->local_sp].combined_port = FBE_TRUE;
                        module_mgmt->io_port_info[port_index].port_logical_info[module_mgmt->local_sp].secondary_combined_port = primary_connector_port_num;

                        // mark the primary connector as a combined connector as well
                        primary_port_index = fbe_module_mgmt_get_io_port_index(iom_num, primary_connector_port_num);
                        module_mgmt->io_port_info[primary_port_index].port_logical_info[module_mgmt->local_sp].combined_port = FBE_TRUE;
                        module_mgmt->io_port_info[primary_port_index].port_logical_info[module_mgmt->local_sp].secondary_combined_port = port_num;

                    }
                    else
                    {
                        // we have found the first port for this logical number, if we find another it will be a combined port
                        primary_port_found = FBE_TRUE;
                        primary_connector_port_num = port_num;
                    }
                }
            }
        }
    }

    // update portal numbers for combined ports that were already discovered
    for(iom_num = 0; iom_num < FBE_ESP_IO_MODULE_MAX_COUNT; iom_num++)
    {
        combined_port_count = 0;
        // The port count will be 0 here for slics that have not yet been discovered or are removed. 
        for(port_num = 0; 
             port_num < module_mgmt->io_module_info[iom_num].physical_info[module_mgmt->local_sp].module_comp_info.ioPortCount; 
             port_num++)
        {
            port_index = fbe_module_mgmt_get_io_port_index(iom_num, port_num);
            if(fbe_module_mgmt_is_port_configured_combined_port(module_mgmt,module_mgmt->local_sp,iom_num,port_num))
            {
                combined_port_count++;
                if(port_num < 2)
                {
                    //ports 0 and 1
                    fbe_module_mgmt_set_portal_number(module_mgmt,module_mgmt->local_sp,iom_num,port_num, 0);
                    module_mgmt->io_port_info[port_index].port_physical_info.selected_pci_function.function = 0;
                }
                else if(port_num == combined_port_count-1)
                {
                    // ports 0 and 1 are already configured as a combined connector set the portal to 1 for ports 2 and 3
                    fbe_module_mgmt_set_portal_number(module_mgmt,module_mgmt->local_sp,iom_num,port_num, 1);
                    module_mgmt->io_port_info[port_index].port_physical_info.selected_pci_function.function = 1;
                }
                else if((port_num == 2) && (combined_port_count < 2))
                {
                    // ports 0 and 1 are not configured as combined connectors, set the portal to 2
                    fbe_module_mgmt_set_portal_number(module_mgmt,module_mgmt->local_sp,iom_num,port_num, 2);
                    module_mgmt->io_port_info[port_index].port_physical_info.selected_pci_function.function = 2;
                }
                else if(port_num == 3)
                {
                    /*
                     * Either ports 0 and 1 are configured as combined connectors or 2 and 3 are, either way the
                     * portal number is 2.
                     */
                    fbe_module_mgmt_set_portal_number(module_mgmt,module_mgmt->local_sp,iom_num,port_num, 2);
                    module_mgmt->io_port_info[port_index].port_physical_info.selected_pci_function.function = 2;
                }
                fbe_base_object_trace((fbe_base_object_t *)module_mgmt,
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "ModMgmt: loaded combined port config setting IOM %d Port %d to portal %d.\n",
                          iom_num, port_num, 
                          fbe_module_mgmt_get_portal_number(module_mgmt,module_mgmt->local_sp, iom_num,port_num));

            }
            else if(combined_port_count > 0)
            {
                // There are combined ports configured ahead of this port, adjust the portal number
                if(port_num == 2)
                {
                    fbe_module_mgmt_set_portal_number(module_mgmt,module_mgmt->local_sp,iom_num,port_num, 1);
                    module_mgmt->io_port_info[port_index].port_physical_info.selected_pci_function.function = 1;
                }
                else if(port_num == 3)
                {
                    fbe_module_mgmt_set_portal_number(module_mgmt,module_mgmt->local_sp,iom_num,port_num, 2);
                    module_mgmt->io_port_info[port_index].port_physical_info.selected_pci_function.function = 2;
                }
                fbe_base_object_trace((fbe_base_object_t *)module_mgmt,
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "ModMgmt: first two ports are combined setting IOM %d Port %d to portal %d.\n",
                          iom_num, port_num, 
                          fbe_module_mgmt_get_portal_number(module_mgmt,module_mgmt->local_sp, iom_num,port_num));
            }
        }
    }
    return;
}


 /*!***************************************************************
 * fbe_module_mgmt_handle_connector_status_change()
 ****************************************************************
 * @brief
 *  For DPE systems the port 0 link status is not very useful as it is
 *  the state of the internal connection between the SAS controller and the
 *  internal expander.  Instead we report the DPE expansion port link status
 *  to the user, which is more intuitive for them.
 *  
 * @param module_mgmt - Ptr to our object context
 * @param phys_location - Connector Location Information
 *
 * @return - N/A
 *
 * @author 
 *  27-April-2015: Created  Brion Philbin
 *
 ****************************************************************/
static fbe_status_t fbe_module_mgmt_handle_connector_status_change(fbe_module_mgmt_t *module_mgmt, fbe_device_physical_location_t phys_location)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_connector_info_t connector_info = {0};
    fbe_bool_t notify = FBE_FALSE;
    fbe_u32_t iom_index = 0;
    fbe_u32_t port_index = 0;
    fbe_device_physical_location_t location = {0};
    fbe_notification_info_t notification = {0};
    fbe_port_link_state_t old_link_state = {0};
    fbe_object_id_t objectId;
    fbe_char_t log_num_string[255];
    HW_ENCLOSURE_TYPE processorEnclType;
    fbe_u32_t slot = 0;
    fbe_u32_t port_speed = 0;
    HW_MODULE_TYPE unique_id = 0;

    fbe_base_object_trace((fbe_base_object_t *)module_mgmt,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                              "%s entry.\n",
                              __FUNCTION__);

    status = fbe_base_env_get_processor_encl_type((fbe_base_environment_t *)module_mgmt,
                                                   &processorEnclType);

    if((status != FBE_STATUS_OK) ||
        (processorEnclType != DPE_ENCLOSURE_TYPE))
    {
        // Only for DPE systems
        return status;
    }

    if (!((phys_location.bus == 0) &&
         (phys_location.enclosure == 0)))
    {
        // Only for the DPE enclosure
        fbe_base_object_trace((fbe_base_object_t *)module_mgmt,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                              "%s NON DPE enclosure %d_%d ignoring.\n",
                              __FUNCTION__, phys_location.bus, phys_location.enclosure);
        return status;
    }
    // Get the BE 0 port information
    iom_index = fbe_module_mgmt_convert_device_type_and_slot_to_index(FBE_DEVICE_TYPE_MEZZANINE, 0);
    port_index = fbe_module_mgmt_get_io_port_index(iom_index, 0);

    for (slot = 0; slot < 10; slot++) 
    {
        phys_location.slot = slot;
        status = fbe_api_enclosure_get_connector_info(&phys_location, &connector_info);
        if (status != FBE_STATUS_OK) 
        {
            fbe_base_object_trace((fbe_base_object_t *)module_mgmt, 
                                  FBE_TRACE_LEVEL_WARNING,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s Error getting connector info, status: 0x%X.\n",
                                  __FUNCTION__, status);
            continue;
        }
        if (connector_info.connectorType == FBE_CONNECTOR_TYPE_EXPANSION) 
        {
            // we have found the expansion port connector information
            fbe_base_object_trace((fbe_base_object_t *)module_mgmt,
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s found expansion port connector slot %d\n",
                                  __FUNCTION__, slot);
            break;
        }
        fbe_base_object_trace((fbe_base_object_t *)module_mgmt,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s slot %d type:%d \n",
                              __FUNCTION__, slot, connector_info.connectorType);

        
    }

    if (connector_info.connectorType != FBE_CONNECTOR_TYPE_EXPANSION) 
    {
        // could not locate the expansion port connector information
        fbe_base_object_trace((fbe_base_object_t *)module_mgmt,
                          FBE_TRACE_LEVEL_WARNING,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s failed to find expansion port slot:%d\n",
                          __FUNCTION__, slot);
        return FBE_STATUS_GENERIC_FAILURE;
    }


       

    fbe_base_object_trace((fbe_base_object_t *)module_mgmt,
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "ModMgmt: Connector:%d Inserted:%d CableStatus:%d Type:%d.\n",
                          phys_location.slot,
                          connector_info.inserted,
                          connector_info.cableStatus,
                          connector_info.connectorType);

    // The speeds here are a temporary workaround, the upper layer code does not pay attention to link state yet, they rely on speed for link status

    unique_id = module_mgmt->io_module_info[iom_index].physical_info[module_mgmt->local_sp].module_comp_info.uniqueID;

    if((unique_id >= IRONHIDE_6G_SAS && unique_id <= IRONHIDE_6G_SAS_REV_C) || 
       (unique_id >= BEACHCOMBER_PCB && unique_id <= BEACHCOMBER_PCB_SIM) ||
       (unique_id >= MERIDIAN_SP_ESX &&  unique_id <= MERIDIAN_SP_HYPERV))
    {
        port_speed = SPEED_SIX_GIGABIT; // these are 6G ports
    }
    else if((unique_id >= OBERON_SP_85W_REV_A && unique_id <= OBERON_SP_E5_2609V3_REV_B) || 
            (unique_id ==OBERON_SP_E5_2660V3_REV_B) ||
            (unique_id == LAPETUS_12G_SAS_REV_A) || 
            (unique_id == LAPETUS_12G_SAS_REV_B))
    {
        port_speed = SPEED_TWELVE_GIGABIT;  // these are 12G ports
    }
    
    if( (connector_info.inserted) && 
        (connector_info.cableStatus == FBE_CABLE_STATUS_GOOD) )
    {
        // link up
        if(module_mgmt->io_port_info[port_index].port_physical_info.link_info.link_state != FBE_PORT_LINK_STATE_UP)
        {
            // data changed
            old_link_state = module_mgmt->io_port_info[port_index].port_physical_info.link_info.link_state;
            module_mgmt->io_port_info[port_index].port_physical_info.link_info.link_state = FBE_PORT_LINK_STATE_UP;
            module_mgmt->io_port_info[port_index].port_physical_info.link_info.link_speed = port_speed;
            notify = FBE_TRUE;
        }
    }
    else if((connector_info.inserted) && 
            (connector_info.cableStatus == FBE_CABLE_STATUS_DEGRADED))
    {
        // link degraded
        if(module_mgmt->io_port_info[port_index].port_physical_info.link_info.link_state != FBE_PORT_LINK_STATE_DEGRADED)
        {
            // data changed
            old_link_state = module_mgmt->io_port_info[port_index].port_physical_info.link_info.link_state;
            module_mgmt->io_port_info[port_index].port_physical_info.link_info.link_state = FBE_PORT_LINK_STATE_DEGRADED;
            module_mgmt->io_port_info[port_index].port_physical_info.link_info.link_speed = port_speed;
            notify = FBE_TRUE;
        }
    }
    else
    {
        // link down
        if(module_mgmt->io_port_info[port_index].port_physical_info.link_info.link_state != FBE_PORT_LINK_STATE_DOWN)
        {
            // data changed
            old_link_state = module_mgmt->io_port_info[port_index].port_physical_info.link_info.link_state;
            module_mgmt->io_port_info[port_index].port_physical_info.link_info.link_state = FBE_PORT_LINK_STATE_DOWN;
            module_mgmt->io_port_info[port_index].port_physical_info.link_info.link_speed = SPEED_HARDWARE_DEFAULT;
            notify = FBE_TRUE;
        }

    }
        


    // Send notification if the link status has changed
    if (notify)
    {
        location.port = 0;
        location.slot = fbe_module_mgmt_get_slot_num(module_mgmt, module_mgmt->local_sp, iom_index);
        location.sp = module_mgmt->local_sp;

        fbe_base_object_get_object_id((fbe_base_object_t *)module_mgmt, &objectId);
    
        notification.notification_type = FBE_NOTIFICATION_TYPE_OBJECT_DATA_CHANGED;
        notification.class_id = FBE_CLASS_ID_MODULE_MGMT;
        notification.object_type = FBE_TOPOLOGY_OBJECT_TYPE_ENVIRONMENT_MGMT;
        notification.notification_data.data_change_info.data_type = FBE_DEVICE_DATA_TYPE_PORT_INFO;
        notification.notification_data.data_change_info.device_mask= fbe_module_mgmt_get_device_type(module_mgmt, module_mgmt->local_sp, iom_index);
        notification.notification_data.data_change_info.phys_location = location;
    
        fbe_notification_send(objectId, notification);
        fbe_module_mgmt_get_port_logical_number_string(fbe_module_mgmt_get_port_logical_number(module_mgmt, module_mgmt->local_sp, iom_index, 0), log_num_string);
        fbe_event_log_write(ESP_INFO_LINK_STATUS_CHANGE, 
                            NULL, 0, 
                            "%s %d %d %s %s %s %s", 
                            fbe_module_mgmt_device_type_to_string(fbe_module_mgmt_get_device_type(module_mgmt, module_mgmt->local_sp, iom_index)), 
                            fbe_module_mgmt_get_slot_num(module_mgmt, module_mgmt->local_sp, iom_index),
                            0,
                            fbe_module_mgmt_port_role_to_string(fbe_module_mgmt_get_port_role(module_mgmt, module_mgmt->local_sp, iom_index, 0)),
                            log_num_string,
                            fbe_module_mgmt_convert_link_state_to_string(old_link_state),
                            fbe_module_mgmt_convert_link_state_to_string(module_mgmt->io_port_info[port_index].port_physical_info.link_info.link_state));
    }
    return status;

}
