/***************************************************************************
 * Copyright (C) EMC Corporation 2010 
 * All rights reserved. Licensed material -- property of EMC 
 * Corporation 
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_cooling_mgmt_monitor.c
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
 *   17-Jan-2011 PHE - Created.
 *
 ***************************************************************************/
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_memory.h"
#include "fbe_topology.h"
#include "fbe_base_object.h"
#include "fbe_scheduler.h"
#include "fbe_cooling_mgmt_private.h"
#include "fbe_cooling_mgmt_util.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_cooling_interface.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_api_board_interface.h"
#include "fbe/fbe_api_enclosure_interface.h"
#include "fbe_transport_memory.h"
#include "fbe_lifecycle.h"
#include "fbe_base_environment_debug.h"
#include "EspMessages.h"
#include "fbe/fbe_event_log_api.h"
#include "generic_utils_lib.h"

static fbe_status_t fbe_cooling_mgmt_processPsStatus(fbe_cooling_mgmt_t * pCoolingMgmt, 
                                                fbe_object_id_t objectId,
                                                fbe_device_physical_location_t * pLocation);

static fbe_status_t fbe_cooling_mgmt_find_available_fan_info(fbe_cooling_mgmt_t * pCoolingMgmt,
                                                     fbe_cooling_mgmt_fan_info_t ** pFanInfoPtr);
static fbe_status_t fbe_cooling_mgmt_generate_fan_logs(fbe_cooling_mgmt_t *pCoolingMgmt,
                                                fbe_device_physical_location_t *pLocation,
                                                fbe_cooling_info_t *pNewCoolingInfo,
                                                fbe_cooling_info_t *pOldCoolingInfo);
static fbe_bool_t 
fbe_cooling_mgmt_get_fan_logical_fault(fbe_cooling_mgmt_fan_info_t *pFanInfo);

static fbe_status_t 
fbe_cooling_mgmt_checkCacheStatus(fbe_cooling_mgmt_t * pCoolingMgmt);

static fbe_status_t fbe_cooling_mgmt_process_peer_cache_status_update(fbe_cooling_mgmt_t * pCoolingMgmt, 
                                                                      fbe_cooling_mgmt_cmi_packet_t * cmiMsgPtr);

/*!***************************************************************
 * fbe_cooling_mgmt_monitor_entry()
 ****************************************************************
 * @brief
 *  Entry routine for the COOLING Management monitor.
 *
 * @param object_handle - This is the object handle.
 * @param packet_p - The packet arriving from the monitor
 * scheduler.
 *
 * @return fbe_status_t - Status of monitor operation.             
 *
 * @author
 *  17-Jan-2011 PHE - Created.
 *
 ****************************************************************/
fbe_status_t 
fbe_cooling_mgmt_monitor_entry(fbe_object_handle_t object_handle, fbe_packet_t * packet)
{
	fbe_cooling_mgmt_t * cooling_mgmt = NULL;

	cooling_mgmt = (fbe_cooling_mgmt_t *)fbe_base_handle_to_pointer(object_handle);

	fbe_topology_class_trace(FBE_CLASS_ID_COOLING_MGMT,
			     FBE_TRACE_LEVEL_DEBUG_HIGH, 
			     FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
			     "%s entry\n", __FUNCTION__);
                 
	return fbe_lifecycle_crank_object(&FBE_LIFECYCLE_CONST_DATA(cooling_mgmt),
	                                  (fbe_base_object_t*)cooling_mgmt, packet);
}
/******************************************
 * end fbe_cooling_mgmt_monitor_entry()
 ******************************************/

/*!**************************************************************
 * fbe_cooling_mgmt_monitor_load_verify()
 ****************************************************************
 * @brief
 *  This function simply validates the constant lifecycle data
 *  that is associated with the cooling management.
 *
 * @param  - None.               
 *
 * @return fbe_status_t - The status of the call to
 *                        fbe_lifecycle_class_const_verify()
 *                        for the cooling management's
 *                        constant lifecycle data.
 *
 * @author
 *  17-Jan-2011 PHE - Created.
 *
 ****************************************************************/
fbe_status_t fbe_cooling_mgmt_monitor_load_verify(void)
{
	return fbe_lifecycle_class_const_verify(&FBE_LIFECYCLE_CONST_DATA(cooling_mgmt));
}
/******************************************
 * end fbe_cooling_mgmt_monitor_load_verify()
 ******************************************/

/*--- local function prototypes --------------------------------------------------------*/

static fbe_lifecycle_status_t fbe_cooling_mgmt_register_state_change_callback_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet);
static fbe_lifecycle_status_t fbe_cooling_mgmt_register_cmi_callback_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet);
static fbe_lifecycle_status_t fbe_cooling_mgmt_fup_register_callback_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet);
static fbe_lifecycle_status_t fbe_cooling_mgmt_register_resume_prom_callback_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet);
static fbe_lifecycle_status_t fbe_cooling_mgmt_discover_cooling_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet);
static fbe_status_t fbe_cooling_mgmt_state_change_handler(update_object_msg_t *update_object_msg, void *context);
static fbe_status_t	fbe_cooling_mgmt_cmi_message_handler(fbe_base_object_t *base_object, fbe_base_environment_cmi_message_info_t *cmi_message);

static fbe_status_t fbe_cooling_mgmt_handle_object_lifecycle_change(fbe_cooling_mgmt_t * pCoolingMgmt, update_object_msg_t * pUpdateObjectMsg);
static fbe_status_t fbe_cooling_mgmt_handle_destroy_state(fbe_cooling_mgmt_t * pCoolingMgmt, update_object_msg_t * pUpdateObjectMsg);

static fbe_status_t fbe_cooling_mgmt_handle_object_data_change(fbe_cooling_mgmt_t * pCoolingMgmt, update_object_msg_t * pUpdateObjectMsg);
static fbe_status_t fbe_cooling_mgmt_handle_generic_info_change(fbe_cooling_mgmt_t * pCoolingMgmt, update_object_msg_t * pUpdateObjectMsg);

static fbe_status_t fbe_cooling_mgmt_cmi_message_handler(fbe_base_object_t *pBaseObject, 
                                fbe_base_environment_cmi_message_info_t *pBaseEnvCmiMsgInfo);
static fbe_status_t fbe_cooling_mgmt_processReceivedCmiMessage(fbe_cooling_mgmt_t * pCoolingMgmt, 
                                           fbe_cooling_mgmt_cmi_packet_t *pCoolingCmiPkt);
static fbe_status_t fbe_cooling_mgmt_processPeerNotPresent(fbe_cooling_mgmt_t * pCoolingMgmt, 
                                       fbe_cooling_mgmt_cmi_packet_t * pCoolingCmiPkt);
static fbe_status_t fbe_cooling_mgmt_processContactLost(fbe_cooling_mgmt_t * pCoolingMgmt);
static fbe_status_t fbe_cooling_mgmt_processPeerBusy(fbe_cooling_mgmt_t * pCoolingMgmt, 
                                 fbe_cooling_mgmt_cmi_packet_t * pCoolingCmiPkt);
static fbe_status_t fbe_cooling_mgmt_processFatalError(fbe_cooling_mgmt_t * pCoolingMgmt, 
                                   fbe_cooling_mgmt_cmi_packet_t * pCoolingCmiPkt);
static fbe_status_t fbe_cooling_mgmt_calculateFanStateInfo(fbe_cooling_mgmt_t *cooling_mgmt,
                                                           fbe_device_physical_location_t *pLocation);

/*--- lifecycle callback functions -----------------------------------------------------*/


FBE_LIFECYCLE_DEF_CALLBACK_GET_INST_DATA(cooling_mgmt);
FBE_LIFECYCLE_DEF_CALLBACK_GET_INST_COND(cooling_mgmt);

/*  cooling_mgmt_lifecycle_callbacks This variable has all the
 *  callbacks the lifecycle needs.
 */
static fbe_lifecycle_const_callback_t FBE_LIFECYCLE_CALLBACKS(cooling_mgmt) = {
	FBE_LIFECYCLE_DEF_CONST_CALLBACKS(
		cooling_mgmt,
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
static fbe_lifecycle_const_cond_t fbe_cooling_mgmt_register_state_change_callback_cond = {
	FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
		FBE_BASE_ENV_LIFECYCLE_COND_REGISTER_STATE_CHANGE_CALLBACK,
		fbe_cooling_mgmt_register_state_change_callback_cond_function)
};

/* FBE_BASE_ENV_LIFECYCLE_COND_REGISTER_CMI_CALLBACK condition
 *  - The purpose of this derived condition is to register
 *  notification with the base environment object which inturns
 *  registers with the CMI. The leaf class needs to implement
 *  the actual condition handler function providing the callback
 *  that needs to be called
 */
static fbe_lifecycle_const_cond_t fbe_cooling_mgmt_register_cmi_callback_cond = {
	FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
		FBE_BASE_ENV_LIFECYCLE_COND_REGISTER_CMI_CALLBACK,
        fbe_cooling_mgmt_register_cmi_callback_cond_function)
};

/* FBE_BASE_ENV_LIFECYCLE_COND_FUP_REGISTER_CALLBACK condition
 *  - The purpose of this derived condition is to register
 *  call back functions with the base environment object. 
 * The leaf class needs to implement the actual call back functions.
 */
static fbe_lifecycle_const_cond_t fbe_cooling_mgmt_fup_register_callback_cond = {
	FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
		FBE_BASE_ENV_LIFECYCLE_COND_FUP_REGISTER_CALLBACK,
        fbe_cooling_mgmt_fup_register_callback_cond_function)
};

static fbe_lifecycle_const_cond_t fbe_cooling_mgmt_fup_abort_upgrade_cond = {
	FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
		FBE_BASE_ENV_LIFECYCLE_COND_FUP_ABORT_UPGRADE,
        fbe_base_env_fup_abort_upgrade_cond_function)
};

static fbe_lifecycle_const_cond_t fbe_cooling_mgmt_fup_wait_before_upgrade_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_ENV_LIFECYCLE_COND_FUP_WAIT_BEFORE_UPGRADE,
        fbe_base_env_fup_wait_before_upgrade_cond_function)
};

static fbe_lifecycle_const_cond_t fbe_cooling_mgmt_fup_wait_for_inter_device_delay_cond = {
	FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
		FBE_BASE_ENV_LIFECYCLE_COND_FUP_WAIT_FOR_INTER_DEVICE_DELAY,
        fbe_base_env_fup_wait_for_inter_device_delay_cond_function)
};

static fbe_lifecycle_const_cond_t fbe_cooling_mgmt_fup_read_image_header_cond = {
	FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
		FBE_BASE_ENV_LIFECYCLE_COND_FUP_READ_IMAGE_HEADER,
        fbe_base_env_fup_read_image_header_cond_function)
};

static fbe_lifecycle_const_cond_t fbe_cooling_mgmt_fup_check_rev_cond = {
	FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
		FBE_BASE_ENV_LIFECYCLE_COND_FUP_CHECK_REV,
        fbe_base_env_fup_check_rev_cond_function)
};

static fbe_lifecycle_const_cond_t fbe_cooling_mgmt_fup_read_entire_image_cond = {
	FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
		FBE_BASE_ENV_LIFECYCLE_COND_FUP_READ_ENTIRE_IMAGE,
        fbe_base_env_fup_read_entire_image_cond_function)
};

static fbe_lifecycle_const_cond_t fbe_cooling_mgmt_fup_download_image_cond = {
	FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
		FBE_BASE_ENV_LIFECYCLE_COND_FUP_DOWNLOAD_IMAGE,
        fbe_base_env_fup_download_image_cond_function)
};

static fbe_lifecycle_const_cond_t fbe_cooling_mgmt_fup_get_download_status_cond = {
	FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
		FBE_BASE_ENV_LIFECYCLE_COND_FUP_GET_DOWNLOAD_STATUS,
        fbe_base_env_fup_get_download_status_cond_function)
};

static fbe_lifecycle_const_cond_t fbe_cooling_mgmt_fup_get_peer_permission_cond = {
	FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
		FBE_BASE_ENV_LIFECYCLE_COND_FUP_GET_PEER_PERMISSION,
        fbe_base_env_fup_get_peer_permission_cond_function)
};

static fbe_lifecycle_const_cond_t fbe_cooling_mgmt_fup_check_env_status_cond = {
	FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
		FBE_BASE_ENV_LIFECYCLE_COND_FUP_CHECK_ENV_STATUS,
        fbe_base_env_fup_check_env_status_cond_function)
};

static fbe_lifecycle_const_cond_t fbe_cooling_mgmt_fup_activate_image_cond = {
	FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
		FBE_BASE_ENV_LIFECYCLE_COND_FUP_ACTIVATE_IMAGE,
        fbe_base_env_fup_activate_image_cond_function)
};

static fbe_lifecycle_const_cond_t fbe_cooling_mgmt_fup_get_activate_status_cond = {
	FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
		FBE_BASE_ENV_LIFECYCLE_COND_FUP_GET_ACTIVATE_STATUS,
        fbe_base_env_fup_get_activate_status_cond_function)
};

static fbe_lifecycle_const_cond_t fbe_cooling_mgmt_fup_check_result_cond = {
	FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
		FBE_BASE_ENV_LIFECYCLE_COND_FUP_CHECK_RESULT,
        fbe_base_env_fup_check_result_cond_function)
};

static fbe_lifecycle_const_cond_t fbe_cooling_mgmt_fup_refresh_device_status_cond = {
	FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
		FBE_BASE_ENV_LIFECYCLE_COND_FUP_REFRESH_DEVICE_STATUS,
        fbe_base_env_fup_refresh_device_status_cond_function)
};

static fbe_lifecycle_const_cond_t fbe_cooling_mgmt_fup_end_upgrade_cond = {
	FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
		FBE_BASE_ENV_LIFECYCLE_COND_FUP_END_UPGRADE,
        fbe_base_env_fup_end_upgrade_cond_function)
};

static fbe_lifecycle_const_cond_t fbe_cooling_mgmt_fup_release_image_cond = {
	FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
		FBE_BASE_ENV_LIFECYCLE_COND_FUP_RELEASE_IMAGE,
        fbe_base_env_fup_release_image_cond_function)
};

/* FBE_BASE_ENV_LIFECYCLE_COND_RESUME_PROM_REGISTER_CALLBACK condition
 * - The purpose of this derived condition is to register
 * call back functions with the base environment object. 
 * The leaf class needs to implement the actual call back functions.
 */
static fbe_lifecycle_const_cond_t fbe_cooling_mgmt_register_resume_prom_callback_cond = {
	FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
		FBE_BASE_ENV_LIFECYCLE_COND_RESUME_PROM_REGISTER_CALLBACK,
        fbe_cooling_mgmt_register_resume_prom_callback_cond_function)
};


/*--- constant base condition entries --------------------------------------------------*/

/* FBE_COOLING_MGMT_LIFECYCLE_COND_DISCOVER_COOLING_COMPONENT condition -
 *   The purpose of this condition is start the discovery process
 *   for all the cooling components
 */
static fbe_lifecycle_const_base_cond_t 
    fbe_cooling_mgmt_discover_cooling_cond = {
        FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "fbe_cooling_mgmt_discover_cooling_cond",
        FBE_COOLING_MGMT_LIFECYCLE_COND_DISCOVER_COOLING,
        fbe_cooling_mgmt_discover_cooling_cond_function),
	FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
		FBE_LIFECYCLE_STATE_SPECIALIZE,     /* SPECIALIZE */
		FBE_LIFECYCLE_STATE_ACTIVATE,         /* ACTIVATE */
		FBE_LIFECYCLE_STATE_READY,         /* READY */
		FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
		FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
		FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
		FBE_LIFECYCLE_STATE_DESTROY)         /* DESTROY */
};


static fbe_lifecycle_const_base_cond_t * FBE_LIFECYCLE_BASE_COND_ARRAY(cooling_mgmt)[] = {
    &fbe_cooling_mgmt_discover_cooling_cond,
};

FBE_LIFECYCLE_DEF_CONST_BASE_CONDITIONS(cooling_mgmt);

/*--- constant rotaries ----------------------------------------------------------------*/

static fbe_lifecycle_const_rotary_cond_t fbe_cooling_mgmt_specialize_rotary[] = {
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_cooling_mgmt_register_state_change_callback_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET), 
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_cooling_mgmt_register_cmi_callback_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_cooling_mgmt_fup_register_callback_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_cooling_mgmt_register_resume_prom_callback_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_cooling_mgmt_discover_cooling_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
};

static fbe_lifecycle_const_rotary_cond_t fbe_cooling_mgmt_ready_rotary[] = {
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_cooling_mgmt_fup_wait_before_upgrade_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_cooling_mgmt_fup_release_image_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),

    /* Moved the following two conditions up. (AR651486)
     * so that work item can be removed from the queue immediately after the device gets removed.
     */
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_cooling_mgmt_fup_refresh_device_status_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_cooling_mgmt_fup_end_upgrade_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),

    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_cooling_mgmt_fup_wait_for_inter_device_delay_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_cooling_mgmt_fup_read_image_header_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),  
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_cooling_mgmt_fup_check_rev_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_cooling_mgmt_fup_read_entire_image_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_cooling_mgmt_fup_get_peer_permission_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_cooling_mgmt_fup_check_env_status_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_cooling_mgmt_fup_download_image_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_cooling_mgmt_fup_get_download_status_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_cooling_mgmt_fup_activate_image_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_cooling_mgmt_fup_get_activate_status_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_cooling_mgmt_fup_check_result_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    
    /* Need to put the abort condition in the end of all the fup conditions. 
     * We want to complete the execution of other condition which was already set before running the abort upgrade 
     * condition. Otherwise, when resuming the upgrade, the condition which was set before would get to run and 
     * it would make the upgrade out of sequence.
     */
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_cooling_mgmt_fup_abort_upgrade_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL), 
};

static fbe_lifecycle_const_rotary_t FBE_LIFECYCLE_ROTARY_ARRAY(cooling_mgmt)[] = {
	FBE_LIFECYCLE_DEF_CONST_ROTARY(FBE_LIFECYCLE_STATE_SPECIALIZE, fbe_cooling_mgmt_specialize_rotary),
    FBE_LIFECYCLE_DEF_CONST_ROTARY(FBE_LIFECYCLE_STATE_READY, fbe_cooling_mgmt_ready_rotary),
};

FBE_LIFECYCLE_DEF_CONST_ROTARIES(cooling_mgmt);

/*--- global cooling mgmt lifecycle constant data ----------------------------------------*/

fbe_lifecycle_const_t FBE_LIFECYCLE_CONST_DATA(cooling_mgmt) = {
	FBE_LIFECYCLE_DEF_CONST_DATA(
		cooling_mgmt,
		FBE_CLASS_ID_COOLING_MGMT,                  /* This class */
		FBE_LIFECYCLE_CONST_DATA(base_environment))    /* The super class */
};

/*--- Condition Functions --------------------------------------------------------------*/

/*!**************************************************************
 * fbe_cooling_mgmt_register_state_change_callback_cond_function()
 ****************************************************************
 * @brief
 *  This function registers with the physical package to get
 *  notified on cooling component status change.
 * 
 * @param object_handle - This is the object handle.
 * @param packet_p - The packet arriving from the monitor
 * scheduler.
 * 
 * @return fbe_status_t - 
 *
 * @author
 *  17-Jan-2011 PHE - Created.
 *
 ****************************************************************/
static fbe_lifecycle_status_t
fbe_cooling_mgmt_register_state_change_callback_cond_function(fbe_base_object_t * base_object, 
                                                          fbe_packet_t * packet)
{
	fbe_status_t status;

    fbe_base_object_trace(base_object, 
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s entry\n",
                          __FUNCTION__);

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
															  (fbe_api_notification_callback_function_t)fbe_cooling_mgmt_state_change_handler,
                                                              (FBE_NOTIFICATION_TYPE_OBJECT_DATA_CHANGED | FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_DESTROY),
                                                              (FBE_DEVICE_TYPE_FAN),
                                                              base_object,
															  (FBE_TOPOLOGY_OBJECT_TYPE_BOARD | FBE_TOPOLOGY_OBJECT_TYPE_ENCLOSURE),
															  FBE_PACKAGE_NOTIFICATION_ID_PHYSICAL);
	if (status != FBE_STATUS_OK) {
		fbe_base_object_trace(base_object, 
								FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"%s API callback init (Board notification) failed, status: 0x%X",
								__FUNCTION__, status);
	}
	return FBE_LIFECYCLE_STATUS_DONE;
}
/*******************************************************************
 * end 
 * fbe_cooling_mgmt_register_state_change_callback_cond_function() 
 *******************************************************************/

/*!**************************************************************
 * fbe_cooling_mgmt_register_cmi_callback_cond_function()
 ****************************************************************
 * @brief
 *  This function registers CMI call back function.
 * 
 * @param object_handle - This is the object handle, or in our
 * case the ps_mgmt.
 * @param packet_p - The packet arriving from the monitor
 * scheduler.
 * 
 * @return fbe_status_t - The status of the call to
 *                        fbe_lifecycle_class_const_verify() for
 *                        the PS management's constant
 *                        lifecycle data.
 *
 * @author
 *  17-Jan-2011 PHE - Created.
 *
 ****************************************************************/
static fbe_lifecycle_status_t
fbe_cooling_mgmt_register_cmi_callback_cond_function(fbe_base_object_t * base_object, 
                                                 fbe_packet_t * packet)
{
	fbe_status_t status;

    fbe_base_object_trace(base_object, 
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s entry\n",
                          __FUNCTION__);

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
															FBE_CMI_CLIENT_ID_COOLING_MGMT,
															fbe_cooling_mgmt_cmi_message_handler);

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
 * fbe_cooling_mgmt_register_cmi_callback_cond_function() 
 *******************************************************************/

/*!**************************************************************
 * fbe_cooling_mgmt_register_resume_prom_callback_cond_function()
 ****************************************************************
 * @brief
 *  This function registers the callback functions with the base environment object.
 * 
 * @param base_object - This is the object handle, or in our
 * case the cooling_mgmt.
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
fbe_cooling_mgmt_register_resume_prom_callback_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_base_environment_t * pBaseEnv = (fbe_base_environment_t *)base_object;

    fbe_base_object_trace(base_object, 
                          FBE_TRACE_LEVEL_DEBUG_HIGH,  
                          FBE_TRACE_MESSAGE_ID_INFO, 
                          "%s entry\n", 
                          __FUNCTION__);

    fbe_base_env_initiate_resume_prom_read_callback(pBaseEnv, &fbe_cooling_mgmt_initiate_resume_prom_read);
    fbe_base_env_get_resume_prom_info_ptr_callback(pBaseEnv, (fbe_base_env_get_resume_prom_info_ptr_func_callback_ptr_t)&fbe_cooling_mgmt_get_resume_prom_info_ptr);
    fbe_base_env_resume_prom_update_encl_fault_led_callback(pBaseEnv, (fbe_base_env_resume_prom_update_encl_fault_led_callback_func_ptr_t) &fbe_cooling_mgmt_resume_prom_update_encl_fault_led);

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
} // fbe_cooling_mgmt_register_resume_prom_callback_cond_function


/*!**************************************************************
 * @fn fbe_cooling_mgmt_state_change_handler()
 ****************************************************************
 * @brief
 *  This is the handler function for state change notification.
 *  
 * @param update_object_msg - 
 * @param context - 
 *
 * @return fbe_status_t - 
 * 
 * @author
 *  17-Jan-2011: PHE - Created.
 *
 ****************************************************************/
static fbe_status_t 
fbe_cooling_mgmt_state_change_handler(update_object_msg_t *pUpdateObjectMsg, void *context)
{
    fbe_notification_info_t    * pNotificationInfo = &pUpdateObjectMsg->notification_info;
    fbe_cooling_mgmt_t         * pCoolingMgmt;
    fbe_notification_data_changed_info_t * pDataChangedInfo = NULL;
    fbe_status_t                 status = FBE_STATUS_OK;

    pCoolingMgmt = (fbe_cooling_mgmt_t *)context;
    
    fbe_base_object_trace((fbe_base_object_t *)pCoolingMgmt, 
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry, objectId 0x%x, notifType 0x%llx, objType 0x%llx, devMsk 0x%llx\n",
                          __FUNCTION__, 
                          pUpdateObjectMsg->object_id,
                          (unsigned long long)pNotificationInfo->notification_type, 
                          (unsigned long long)pNotificationInfo->object_type, 
                          pNotificationInfo->notification_data.data_change_info.device_mask);

    
    pDataChangedInfo = &(pNotificationInfo->notification_data.data_change_info);

    switch (pNotificationInfo->notification_type)
    {
        case FBE_NOTIFICATION_TYPE_OBJECT_DATA_CHANGED:
            status = fbe_cooling_mgmt_handle_object_data_change(pCoolingMgmt, pUpdateObjectMsg);
            break;
            
        case FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_DESTROY:
            status = fbe_cooling_mgmt_handle_destroy_state(pCoolingMgmt, pUpdateObjectMsg);
            break;

        default:
            fbe_base_object_trace((fbe_base_object_t *)pCoolingMgmt, 
                                  FBE_TRACE_LEVEL_WARNING,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s Unhandled state 0x%llX.\n",
                                  __FUNCTION__, 
                                  (unsigned long long)pNotificationInfo->notification_type);
            break;
    }
	return FBE_STATUS_OK;
}//End of function fbe_cooling_mgmt_state_change_handler


/*!**************************************************************
 * @fn fbe_cooling_mgmt_handle_object_data_change()
 ****************************************************************
 * @brief
 *  This function is to handle the object data change.
 *
 * @param pCoolingMgmt - This is the object handle.
 * @param pUpdateObjectMsg - Pointer to the notification info
 *
 * @return fbe_status_t - FBE Status
 * 
 * @author
 *  17-Jan-2011: PHE - Created.
 *
 ****************************************************************/
static fbe_status_t fbe_cooling_mgmt_handle_object_data_change(fbe_cooling_mgmt_t * pCoolingMgmt, 
                                                          update_object_msg_t * pUpdateObjectMsg)
{
    fbe_notification_data_changed_info_t * pDataChangedInfo = NULL;
    fbe_device_physical_location_t       * pLocation = NULL;
    fbe_status_t                           status = FBE_STATUS_OK;

    pDataChangedInfo = &(pUpdateObjectMsg->notification_info.notification_data.data_change_info);
    pLocation = &(pDataChangedInfo->phys_location);

    fbe_base_object_trace((fbe_base_object_t *)pCoolingMgmt, 
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s entry, dataType 0x%x.\n",
                          __FUNCTION__, pDataChangedInfo->data_type);
    
    switch(pDataChangedInfo->data_type)
    {
        case FBE_DEVICE_DATA_TYPE_GENERIC_INFO:
            status = fbe_cooling_mgmt_handle_generic_info_change(pCoolingMgmt, pUpdateObjectMsg);
            break;

        case FBE_DEVICE_DATA_TYPE_FUP_INFO:
            status = fbe_base_env_fup_handle_firmware_status_change((fbe_base_environment_t *)pCoolingMgmt, 
                                                               pDataChangedInfo->device_mask,
                                                               pLocation);
            break;

        default:
            break;
    }

    return status;
}// End of function fbe_cooling_mgmt_handle_object_data_change

/*!**************************************************************
 * @fn fbe_cooling_mgmt_handle_generic_info_change()
 ****************************************************************
 * @brief
 *  This function is to handle the generic info change.
 *
 * @param pCoolingMgmt - This is the object handle.
 * @param pUpdateObjectMsg - Pointer to the notification info
 *
 * @return fbe_status_t - FBE Status
 * 
 * @author
 *  17-Jan-2011:  PHE - Created.
 *
 ****************************************************************/
static fbe_status_t fbe_cooling_mgmt_handle_generic_info_change(fbe_cooling_mgmt_t * pCoolingMgmt, 
                                                          update_object_msg_t * pUpdateObjectMsg)
{
    fbe_notification_data_changed_info_t   * pDataChangedInfo = NULL;
    fbe_device_physical_location_t         * pLocation = NULL;
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_topology_object_type_t              objectType;
    fbe_u32_t                               dpeFanShiftCount = 0;

    pDataChangedInfo = &(pUpdateObjectMsg->notification_info.notification_data.data_change_info);
    pLocation = &(pDataChangedInfo->phys_location);
    
    status = fbe_base_env_get_and_adjust_encl_location((fbe_base_environment_t *)pCoolingMgmt, 
                                                       pUpdateObjectMsg->object_id, 
                                                       pLocation);
    if (status != FBE_STATUS_OK) 
    {
        fbe_base_object_trace((fbe_base_object_t *)pCoolingMgmt, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s get and adjust encl location for objectId %d failed, status: 0x%x.\n",
                              __FUNCTION__, pUpdateObjectMsg->object_id, status);

        return FBE_LIFECYCLE_STATUS_DONE;
    }


    switch (pDataChangedInfo->device_mask)
    {
        case FBE_DEVICE_TYPE_FAN:
            status = fbe_api_get_object_type(pUpdateObjectMsg->object_id, &objectType, FBE_PACKAGE_ID_PHYSICAL);
            if (status != FBE_STATUS_OK) 
            {
                fbe_base_object_trace((fbe_base_object_t *)pCoolingMgmt, 
                                      FBE_TRACE_LEVEL_WARNING,
                                      FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                      "%s, could not get ObjType, objId %d, status: 0x%x.\n",
                                      __FUNCTION__, pUpdateObjectMsg->object_id, status);

            }
            if ((pCoolingMgmt->base_environment.processorEnclType == DPE_ENCLOSURE_TYPE) && 
                (pLocation->bus == 0) && (pLocation->enclosure == 0) &&
                (pCoolingMgmt->dpeFanSourceObjectType == objectType))
            {
                dpeFanShiftCount = pCoolingMgmt->dpeFanSourceOffset;
            }
            else
            {
                dpeFanShiftCount = 0;
            }
            status = fbe_cooling_mgmt_process_fan_status(pCoolingMgmt, pUpdateObjectMsg->object_id, pLocation, dpeFanShiftCount);
            if (status != FBE_STATUS_OK) 
            {
                fbe_base_object_trace((fbe_base_object_t *)pCoolingMgmt, 
                                      FBE_TRACE_LEVEL_WARNING,
                                      FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                      "%s FAN(%d_%d_%d) Error in processing status, status 0x%X.\n",
                                      __FUNCTION__, 
                                      pLocation->bus,
                                      pLocation->enclosure,
                                      pLocation->slot,
                                      status);

            }
            break;

        default:
            break;
    }
    
    return status;
}// End of function fbe_cooling_mgmt_handle_generic_info_change

/*!**************************************************************
 * @fn fbe_cooling_mgmt_handle_object_lifecycle_change()
 ****************************************************************
 * @brief
 *  This function is to handle the lifecycle change.
 *
 * @param pCoolingMgmt - This is the object handle.
 * @param pUpdateObjectMsg - Pointer to the notification info
 *
 * @return fbe_status_t - FBE Status
 * 
 * @author
 *  17-Jan-2011:  PHE - Created.
 *
 ****************************************************************/
static fbe_status_t fbe_cooling_mgmt_handle_object_lifecycle_change(fbe_cooling_mgmt_t * pCoolingMgmt, 
                                                               update_object_msg_t * pUpdateObjectMsg)
{
    fbe_status_t status;

    switch(pUpdateObjectMsg->notification_info.notification_type) 
    {
        case FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_DESTROY:
            status = fbe_cooling_mgmt_handle_destroy_state(pCoolingMgmt, pUpdateObjectMsg);
            break;

        default:
            fbe_base_object_trace((fbe_base_object_t *)pCoolingMgmt, 
                                  FBE_TRACE_LEVEL_WARNING,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s Unhandled state 0x%llX.\n",
                                  __FUNCTION__,
				  (unsigned long long)pUpdateObjectMsg->notification_info.notification_type);
            status = FBE_STATUS_GENERIC_FAILURE;
            break;
    }

    return status;
}// End of function fbe_cooling_mgmt_handle_object_lifecycle_change


/*!**************************************************************
 * @fn fbe_cooling_mgmt_handle_destroy_state()
 ****************************************************************
 * @brief
 *  This is the handler function for life cycle change to the
 *  destroy state.
 *
 * @param pPsMgmt - This is the object handle.
 * @param pUpdateObjectMsg - Pointer to the notification info
 *
 * @return fbe_status_t - FBE Status
 * 
 * @author
 *  17-Jan-2011:  PHE - Created.
 *
 ****************************************************************/
static fbe_status_t fbe_cooling_mgmt_handle_destroy_state(fbe_cooling_mgmt_t * pCoolingMgmt, 
                                                     update_object_msg_t * pUpdateObjectMsg)
{
    fbe_object_id_t               objectId;
    fbe_u32_t                     fanIndex = 0;
    fbe_status_t                  status = FBE_STATUS_OK;
    fbe_cooling_mgmt_fan_info_t * pFanInfo = NULL;
    fbe_device_physical_location_t phys_location = {0};
    
    objectId = pUpdateObjectMsg->object_id;

    for (fanIndex = 0; fanIndex < FBE_COOLING_MGMT_MAX_FAN_COUNT; fanIndex++)
    {
        pFanInfo = &pCoolingMgmt->pFanInfo[fanIndex];

        if((pFanInfo->objectId == objectId) &&
           (objectId != FBE_OBJECT_ID_INVALID))
        {
            /* Found the cooling info. 
             * Since the containing enclosure was destroyed, 
             * clean out the structure.
             * Get the physical location to send notification.
             */
            phys_location = pFanInfo->location;

            status = fbe_base_env_fup_handle_destroy_state((fbe_base_environment_t *)pCoolingMgmt, 
                                                            &phys_location);

            if(status != FBE_STATUS_OK) 
            {
                fbe_base_object_trace((fbe_base_object_t *)pCoolingMgmt, 
                                      FBE_TRACE_LEVEL_INFO,
                                      FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                      "FUP:Encl(%d_%d),%s,fup_handle_destroy_state failed,status 0x%X.\n",
                                      phys_location.bus, 
                                      phys_location.enclosure,
                                      __FUNCTION__, 
                                      status);
            }
           
            /* Containing Enclosure is destroyed/Removed. Clear the Resume Prom Info */
            status = fbe_base_env_resume_prom_handle_destroy_state((fbe_base_environment_t *)pCoolingMgmt, 
                                                                   phys_location.bus,
                                                                   phys_location.enclosure);
        
            if(status != FBE_STATUS_OK) 
            {
                fbe_base_object_trace((fbe_base_object_t *)pCoolingMgmt, 
                                      FBE_TRACE_LEVEL_INFO,
                                      FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                      "RP:Encl(%d_%d),%s,resume_prom_handle_destroy_state failed,status 0x%X.\n",
                                      phys_location.bus, 
                                      phys_location.enclosure,
                                      __FUNCTION__,
                                      status);
            }

            status = fbe_cooling_mgmt_init_fan_info(pFanInfo);
            if(status == FBE_STATUS_OK)
            {
                /* Send notitication */
                fbe_base_environment_send_data_change_notification((fbe_base_environment_t*)pCoolingMgmt,
                                                                   FBE_CLASS_ID_COOLING_MGMT, 
                                                                   FBE_DEVICE_TYPE_FAN, 
                                                                   FBE_DEVICE_DATA_TYPE_GENERIC_INFO,
                                                                   &phys_location);
            }
        }
    }
   
    return status;
}// End of function fbe_cooling_mgmt_handle_destroy_state


/*!***************************************************************
 * fbe_cooling_mgmt_get_fan_info_by_location()
 ****************************************************************
 * @brief
 *  This function returns the pointer to the Fan info 
 *  for the specified location.
 *
 * @param pCoolingMgmt -
 * @param pLocation - 
 * @param pCoolingInfoPtr (OUTPUT) -
 *
 * @return fbe_status_t
 *
 * @author
 *  17-Jan-2011:  PHE - Created.
 *
 ****************************************************************/
fbe_status_t fbe_cooling_mgmt_get_fan_info_by_location(fbe_cooling_mgmt_t * pCoolingMgmt,
                                                       fbe_device_physical_location_t * pLocation,
                                                       fbe_cooling_mgmt_fan_info_t **pFanInfoPtr)
{
    fbe_u32_t                          fanIndex = 0;
    fbe_cooling_mgmt_fan_info_t      * pFanInfo = NULL;
   
    *pFanInfoPtr = NULL;

    for (fanIndex = 0; fanIndex < FBE_COOLING_MGMT_MAX_FAN_COUNT; fanIndex++)
    {
        pFanInfo = &pCoolingMgmt->pFanInfo[fanIndex];

            if((pFanInfo->location.bus == pLocation->bus) &&
               (pFanInfo->location.enclosure == pLocation->enclosure) &&
               (pFanInfo->location.slot == pLocation->slot))
            {
                // found the cooling info.
                *pFanInfoPtr = pFanInfo;
                return FBE_STATUS_OK;
            }
    }

    return FBE_STATUS_NO_DEVICE;
}


/*!***************************************************************
 * fbe_cooling_mgmt_find_available_fan_info()
 ****************************************************************
 * @brief
 *  This function returns the pointer to the available FAN info.
 *
 * @param  pCoolingMgmt - 
 * @param  pCoolingInfoPtr(OUTPUT) - 
 *
 * @return fbe_status_t
 *
 * @author
 *  17-Jan-2011:  PHE - Created.
 *
 ****************************************************************/
static fbe_status_t 
fbe_cooling_mgmt_find_available_fan_info(fbe_cooling_mgmt_t * pCoolingMgmt,
                                         fbe_cooling_mgmt_fan_info_t ** pFanInfoPtr)
{
    fbe_u32_t                          fanIndex = 0;
    fbe_cooling_mgmt_fan_info_t      * pFanInfo = NULL;
   
    *pFanInfoPtr = NULL;
     
    for (fanIndex = 0; fanIndex < FBE_COOLING_MGMT_MAX_FAN_COUNT; fanIndex++)
    {
        pFanInfo = &pCoolingMgmt->pFanInfo[fanIndex];

        if(pFanInfo->objectId == FBE_OBJECT_ID_INVALID)
        {
            // found the cooling info.
            *pFanInfoPtr = pFanInfo;
            return FBE_STATUS_OK;
        }
    }

    return FBE_STATUS_COMPONENT_NOT_FOUND;
}

/******************************************
 * end fbe_cooling_mgmt_find_available_fan_info()
 ******************************************/


/*!**************************************************************
 * fbe_cooling_mgmt_discover_cooling_cond_function()
 ****************************************************************
 * @brief
 *  This function gets called for the condition when 
 *  FBE_COOLING_MGMT_LIFECYCLE_COND_DISCOVER_COOLING gets called.
 *  It discovers all the cooling components on the system.
 *
 * @param base_object - This is the object handle.
 * @param packet - The packet arriving from the monitor
 * scheduler.
 *
 * @return fbe_lifecycle_status_t - 
 *
 * @author
 *  17-Jan-2011: PHE - Created. 
 *
 ****************************************************************/
static fbe_lifecycle_status_t 
fbe_cooling_mgmt_discover_cooling_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet)
{
    fbe_cooling_mgmt_t                        * pCoolingMgmt = (fbe_cooling_mgmt_t *) base_object;
    fbe_object_id_t                           * object_id_list = NULL;
    fbe_u32_t                                   enclCount = 0;
    fbe_u32_t                                   enclIndex = 0;
    fbe_u32_t                                   fanIndex = 0;
    fbe_device_physical_location_t              location = {0};
    fbe_u32_t                                   fanCount = 0;
    fbe_status_t                                status = FBE_STATUS_OK;
    fbe_topology_object_type_t                  objectType;
    fbe_u32_t                                   previousDpeFanCount = 0;
    fbe_u32_t                                   dpeFanShiftCount = 0;

    fbe_base_object_trace(base_object, 
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n",
                          __FUNCTION__);

    /* Get the list of all the enclosures including the process enclosure the system sees*/
    object_id_list = fbe_base_env_memory_ex_allocate((fbe_base_environment_t *)pCoolingMgmt, sizeof(fbe_object_id_t) * FBE_MAX_PHYSICAL_OBJECTS);
    fbe_set_memory(object_id_list, (fbe_u8_t)FBE_OBJECT_ID_INVALID, sizeof(fbe_object_id_t) * FBE_MAX_PHYSICAL_OBJECTS);

    /* The first object id in the list is for the board. */
    status = fbe_api_get_board_object_id(&object_id_list[0]);
    if (status != FBE_STATUS_OK) 
    {
        fbe_base_object_trace(base_object, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s Error in getting the Board Object ID, status: 0x%X\n",
                              __FUNCTION__, status);
        fbe_base_env_memory_ex_release((fbe_base_environment_t *)pCoolingMgmt, object_id_list);
        return FBE_LIFECYCLE_STATUS_DONE;
    }
   
    /* The rest object ids in the list is for DPE or DAE enclosures.*/
    status = fbe_api_get_all_enclosure_object_ids(&object_id_list[1], FBE_MAX_PHYSICAL_OBJECTS-1, &enclCount);

    if (status != FBE_STATUS_OK) 
    {
        fbe_base_object_trace(base_object, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s Error in getting all the enclosure objectIDs, status: 0x%X\n",
                              __FUNCTION__, status);

        fbe_base_env_memory_ex_release((fbe_base_environment_t *)pCoolingMgmt, object_id_list);
        return FBE_LIFECYCLE_STATUS_DONE;
    }
   
    for(enclIndex = 0; enclIndex < enclCount + 1; enclIndex ++) 
    {
        status = fbe_api_enclosure_get_fan_count(object_id_list[enclIndex], &fanCount);

        if (status != FBE_STATUS_OK) 
        {
            fbe_base_object_trace(base_object, 
                                  FBE_TRACE_LEVEL_WARNING,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s Error in getting FAN count for objectId %d, status 0x%x.\n",
                                  __FUNCTION__, object_id_list[enclIndex], status);

          continue;
        }
       
        fbe_zero_memory(&location, sizeof(fbe_device_physical_location_t));

        status = fbe_base_env_get_and_adjust_encl_location((fbe_base_environment_t *)pCoolingMgmt, 
                                                           object_id_list[enclIndex], 
                                                           &location);
        if (status != FBE_STATUS_OK) 
        {
            fbe_base_object_trace(base_object, 
                                  FBE_TRACE_LEVEL_WARNING,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s get and adjust encl location for objectId %d failed, status: 0x%x.\n",
                                  __FUNCTION__, object_id_list[enclIndex], status);

            continue;
        }

        status = fbe_api_get_object_type(object_id_list[enclIndex], &objectType, FBE_PACKAGE_ID_PHYSICAL);
        if (status != FBE_STATUS_OK) 
        {
            fbe_base_object_trace(base_object, 
                                  FBE_TRACE_LEVEL_WARNING,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s, could not get ObjType, objId %d, status: 0x%x.\n",
                                  __FUNCTION__, object_id_list[enclIndex], status);

            continue;
        }

//        if (objectType == FBE_TOPOLOGY_OBJECT_TYPE_BOARD)
        if ((location.bus == 0) && (location.enclosure == 0))
        {
            if (previousDpeFanCount == 0)
            {
                // save Fan count (combine with other fan counts)
                previousDpeFanCount = fanCount; 
            }
            else
            {
                // shift any new Fans to new locations
                dpeFanShiftCount = previousDpeFanCount;
                pCoolingMgmt->dpeFanSourceObjectType = objectType;
                pCoolingMgmt->dpeFanSourceOffset = previousDpeFanCount;
                previousDpeFanCount = 0;
            }
        }
               
        for (fanIndex = 0; fanIndex < fanCount; fanIndex++)
        {
            // adjust slot if this enclosure already has fans (add new fans, don't overwrite existing ones
            if ((pCoolingMgmt->base_environment.processorEnclType == DPE_ENCLOSURE_TYPE) && 
//                (objectType == FBE_TOPOLOGY_OBJECT_TYPE_ENCLOSURE) &&
                (location.bus == 0) && (location.enclosure == 0))
            {
                if (pCoolingMgmt->dpeFanSourceObjectType == objectType)
                {
                    dpeFanShiftCount = pCoolingMgmt->dpeFanSourceOffset;
                }
                else
                {
                    dpeFanShiftCount = 0;
                }
            }
            else
            {
                dpeFanShiftCount = 0;
            }

            location.slot = fanIndex; 
            location.sp = SP_INVALID;

            status = fbe_cooling_mgmt_process_fan_status(pCoolingMgmt, object_id_list[enclIndex], &location, dpeFanShiftCount);
            if (status != FBE_STATUS_OK) 
            {
                fbe_base_object_trace(base_object, 
                                      FBE_TRACE_LEVEL_WARNING,
                                      FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                      "%s FAN(%d_%d_%d) Error in processing status, status 0x%x.\n",
                                      __FUNCTION__, 
                                      location.bus,
                                      location.enclosure,
                                      location.slot,
                                      status);
        	}
        }
    }

    fbe_cooling_mgmt_checkCacheStatus(pCoolingMgmt);

    status = fbe_lifecycle_clear_current_cond(base_object);
	if (status != FBE_STATUS_OK) 
    {
		fbe_base_object_trace(base_object, 
								FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"%s can't clear current condition, status: 0x%x.\n",
								__FUNCTION__, status);
	}

    fbe_base_env_memory_ex_release((fbe_base_environment_t *)pCoolingMgmt, object_id_list);
    return FBE_LIFECYCLE_STATUS_DONE;
}

/******************************************************
 * end fbe_cooling_mgmt_discover_cooling_cond_function() 
 ******************************************************/
   

/*!**************************************************************
 * fbe_cooling_mgmt_process_fan_status()
 ****************************************************************
 * @brief
 *  This function processes the Fan status change.
 *
 * @param pCoolingMgmt - This is the object handle.
 * @param objectId - The source object id. 
 *                   It is physical package enclosure object id for the Cooling Component.
 * @param pLocation - The location of the cooling component.
 * @param fanIndex - The index of the cooling component.
 *
 * @return fbe_status_t - 
 *
 * @author
 *  17-Jan-2011: PHE - Created.
 *
 ****************************************************************/
fbe_status_t fbe_cooling_mgmt_process_fan_status(fbe_cooling_mgmt_t * pCoolingMgmt,
                                                 fbe_object_id_t objectId,
                                                 fbe_device_physical_location_t * pLocation,
                                                 fbe_u32_t secondaryFanSourceOffset)
{
    fbe_base_object_t                     * base_object = (fbe_base_object_t *)pCoolingMgmt;
    fbe_cooling_mgmt_fan_info_t           * pFanInfo = NULL;
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_cooling_info_t                      newCoolingInfo = {0};
    fbe_cooling_info_t                      oldCoolingInfo = {0};
    fbe_device_physical_location_t          offsetLocation;

    if(objectId == FBE_OBJECT_ID_INVALID)
    {
        fbe_base_object_trace(base_object, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s, encl %d_%d, invalid object id.\n",
                              __FUNCTION__, pLocation->bus, pLocation->enclosure);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    offsetLocation = *pLocation;
    offsetLocation.slot += secondaryFanSourceOffset;

    status = fbe_cooling_mgmt_get_fan_info_by_location(pCoolingMgmt, &offsetLocation, &pFanInfo);

    if (status != FBE_STATUS_OK) 
    {
        fbe_base_object_trace(base_object, 
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s, FAN(%d_%d_%d), no fan info ptr, need to assign fan info ptr.\n",
                                  __FUNCTION__, offsetLocation.bus, offsetLocation.enclosure, offsetLocation.slot);

        status = fbe_cooling_mgmt_find_available_fan_info(pCoolingMgmt, &pFanInfo);
 
        if (status != FBE_STATUS_OK) 
        {
            fbe_base_object_trace(base_object, 
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s, encl %d_%d, Not enough memory allocated for cooling info, status: 0x%X.\n",
                                  __FUNCTION__, pLocation->bus, pLocation->enclosure, status);
            return status;
        }

        pCoolingMgmt->fanCount++;
        pFanInfo->objectId = objectId;
        pFanInfo->location = offsetLocation;
    }

    fbe_zero_memory(&newCoolingInfo, sizeof(fbe_cooling_info_t));
    newCoolingInfo.slotNumOnEncl = pLocation->slot;
    newCoolingInfo.slotNumOnSpBlade = FBE_INVALID_SLOT_NUM;

    status = fbe_api_cooling_get_fan_info(objectId, &newCoolingInfo);
    if (status != FBE_STATUS_OK) 
    {
        fbe_base_object_trace(base_object, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s Error getting Cooling basic info for object id %d, status: 0x%X.\n",
                              __FUNCTION__, objectId, status);
        return status;
    }

    /* Save the old fan basic info. */
    oldCoolingInfo = pFanInfo->basicInfo;
    /* Copy over the new fan basic info to the fbe_cooling_mgmt_t date structure. */
    pFanInfo->basicInfo = newCoolingInfo;
    pFanInfo->location.sp = newCoolingInfo.associatedSp;

    if(!fbe_equal_memory(&oldCoolingInfo, &newCoolingInfo, sizeof(fbe_cooling_info_t)))
    {
        fbe_base_object_trace(base_object, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s, inserted %d, flt %d, degraded %d, loc %d\n",
                              __FUNCTION__,
                              pFanInfo->basicInfo.inserted,
                              pFanInfo->basicInfo.fanFaulted,
                              pFanInfo->basicInfo.fanDegraded,
                              pFanInfo->basicInfo.fanLocation);
        fbe_base_object_trace(base_object, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s, envInf 0x%x, multiFanModFlt %d, persMultFanModFlt %d\n",
                              __FUNCTION__,
                              pFanInfo->basicInfo.envInterfaceStatus,
                              pFanInfo->basicInfo.multiFanModuleFault,
                              pFanInfo->basicInfo.persistentMultiFanModuleFault);
        status = fbe_cooling_mgmt_generate_fan_logs(pCoolingMgmt,
                                                    &offsetLocation,
                                                    &newCoolingInfo,
                                                    &oldCoolingInfo);

    }

    fbe_cooling_mgmt_checkCacheStatus(pCoolingMgmt);
    
    status = fbe_cooling_mgmt_fup_handle_fan_status_change(pCoolingMgmt, &offsetLocation, &newCoolingInfo, &oldCoolingInfo);
    if(status != FBE_STATUS_OK) 
    {
        fbe_base_object_trace(base_object, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "FUP:FAN(%d_%d_%d) fup_handle_fan_status_change failed, status 0x%X.\n",
                              offsetLocation.bus,
                              offsetLocation.enclosure,
                              offsetLocation.slot,
                              status);
    }
    
    status =  fbe_cooling_mgmt_resume_prom_handle_status_change(pCoolingMgmt,
                                                                &offsetLocation,
                                                                &newCoolingInfo,
                                                                &oldCoolingInfo);

    fbe_cooling_mgmt_calculateFanStateInfo(pCoolingMgmt, pLocation);

    /*
     * Check if Enclosure Fault LED needs updating
     */
    status = fbe_cooling_mgmt_update_encl_fault_led(pCoolingMgmt, pLocation, FBE_ENCL_FAULT_LED_FAN_FLT);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace(base_object, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "FAN(%d_%d_%d), error updating EnclFaultLed, status 0x%X.\n",
                              pLocation->bus,
                              pLocation->enclosure,
                              pLocation->slot,
                              status);
    }
    
    /* Send notification */
    fbe_base_environment_send_data_change_notification((fbe_base_environment_t*)pCoolingMgmt,
                                                       FBE_CLASS_ID_COOLING_MGMT, 
                                                       FBE_DEVICE_TYPE_FAN, 
                                                       FBE_DEVICE_DATA_TYPE_GENERIC_INFO,
                                                       &offsetLocation);
    return status;
}
/******************************************************
 * end fbe_cooling_mgmt_process_fan_status() 
 ******************************************************/

/*!**************************************************************
 * @fn fbe_cooling_mgmt_update_encl_fault_led(fbe_cooling_mgmt_t *pCoolingMgmt,
 *                                  fbe_device_physical_location_t *pLocation,
 *                                  fbe_encl_fault_led_reason_t enclFaultLedReason)
 ****************************************************************
 * @brief
 *  This function checks the status to determine
 *  if the Enclosure Fault LED needs to be updated.
 *
 * @param pEnclMgmt -
 * @param pLocation - The location of the enclosure.
 *
 * @return fbe_status_t - 
 *
 * @author
 *  14-Dec-2011:  PHE - Created. 
 *
 ****************************************************************/
fbe_status_t fbe_cooling_mgmt_update_encl_fault_led(fbe_cooling_mgmt_t *pCoolingMgmt,
                                    fbe_device_physical_location_t *pLocation,
                                    fbe_encl_fault_led_reason_t enclFaultLedReason)
{
    fbe_status_t                                status = FBE_STATUS_OK;
    fbe_cooling_mgmt_fan_info_t                 *pFanInfo = NULL;
    fbe_u32_t                                   index = 0;
    fbe_bool_t                                  enclFaultLedOn = FBE_FALSE;

    switch(enclFaultLedReason) 
    {
    case FBE_ENCL_FAULT_LED_FAN_FLT:
        for (index = 0; index < FBE_COOLING_MGMT_MAX_FAN_COUNT; index++)
        {
            pFanInfo = &pCoolingMgmt->pFanInfo[index];
     
            if((pFanInfo->location.bus == pLocation->bus) && 
               (pFanInfo->location.enclosure == pLocation->enclosure))
            {
                enclFaultLedOn = fbe_cooling_mgmt_get_fan_logical_fault(pFanInfo);
    
                if(enclFaultLedOn)
                {
                    break;
                }
            }
        }
        break;

    case FBE_ENCL_FAULT_LED_FAN_RESUME_PROM_FLT:
        for (index = 0; index < FBE_COOLING_MGMT_MAX_FAN_COUNT; index++)
        {
            pFanInfo = &pCoolingMgmt->pFanInfo[index];
     
            if((pFanInfo->location.bus == pLocation->bus) && 
               (pFanInfo->location.enclosure == pLocation->enclosure))
            {
                enclFaultLedOn = fbe_base_env_get_resume_prom_fault(pFanInfo->fanResumeInfo.op_status);
        
                if(enclFaultLedOn)
                {
                    break;
                }
            }
        }
        break;
   
    default:
        fbe_base_object_trace((fbe_base_object_t *)pCoolingMgmt, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "ENCL(%d_%d), unsupported enclFaultLedReason 0x%llX.\n",
                              pLocation->bus,
                              pLocation->enclosure,
                              enclFaultLedReason);

        return FBE_STATUS_GENERIC_FAILURE;
        break;
    }

    status = fbe_cooling_mgmt_get_fan_info_by_location(pCoolingMgmt, pLocation, &pFanInfo);
    if (status != FBE_STATUS_OK) 
    {
        fbe_base_object_trace((fbe_base_object_t *)pCoolingMgmt, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s, encl %d_%d, get_encl_info_by_location failed, status: 0x%X.\n",
                              __FUNCTION__, pLocation->bus, pLocation->enclosure, status);
        return status;
    }

    status = fbe_base_environment_updateEnclFaultLed((fbe_base_object_t *)pCoolingMgmt,
                                                     pFanInfo->objectId,
                                                     enclFaultLedOn,
                                                     enclFaultLedReason);
   
    if (status == FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)pCoolingMgmt, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "ENCL(%d_%d), %s enclFaultLedReason %s.\n",
                              pLocation->bus,
                              pLocation->enclosure,
                              enclFaultLedOn? "SET" : "CLEAR",
                              fbe_base_env_decode_encl_fault_led_reason(enclFaultLedReason));
    }
    else
    {
        fbe_base_object_trace((fbe_base_object_t *)pCoolingMgmt, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "ENCL(%d_%d), error in %s enclFaultLedReason %s.\n",
                              pLocation->bus,
                              pLocation->enclosure,
                              enclFaultLedOn? "SET" : "CLEAR",
                              fbe_base_env_decode_encl_fault_led_reason(enclFaultLedReason));
        
    }
       
    return status;
}


/*!**************************************************************
 * @fn fbe_encl_mgmt_get_fan_logical_fault(fbe_cooling_mgmt_fan_info_t *pFanInfo)
 ****************************************************************
 * @brief
 *  This function considers all the fields for the FAN
 *  and decides whether the FAN is logically faulted or not.
 *
 * @param pFanInfo -The pointer to fbe_cooling_mgmt_fan_info_t.
 *
 * @return fbe_bool_t - The FAN is faulted or not logically.
 *
 * @author
 *  14-Dec-2011: PHE - Moved from fbe_cooling_mgmt_updateEnclFaultLed.
 *
 ****************************************************************/
static fbe_bool_t 
fbe_cooling_mgmt_get_fan_logical_fault(fbe_cooling_mgmt_fan_info_t *pFanInfo)
{
    fbe_bool_t logicalFault = FBE_FALSE;

    if (!pFanInfo->basicInfo.inserted ||
        pFanInfo->basicInfo.fanFaulted ||
        (pFanInfo->basicInfo.envInterfaceStatus == FBE_ENV_INTERFACE_STATUS_XACTION_FAIL) ||
        (pFanInfo->basicInfo.envInterfaceStatus == FBE_ENV_INTERFACE_STATUS_DATA_STALE))
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
 * fbe_cooling_mgmt_checkCacheStatus()
 ****************************************************************
 * @brief
 *  This function checks the Cooling Mgmt Cache status.
 *
 * @param pPsMgmt -
 *
 * @return fbe_status_t - The status of the call to
 *                        fbe_lifecycle_class_const_verify()
 *                        for the PS management's
 *                        constant lifecycle data.
 *
 * @author
 *  25-Mar-2011:  Created. Joe Perry
 *
 ****************************************************************/
static fbe_status_t 
fbe_cooling_mgmt_checkCacheStatus(fbe_cooling_mgmt_t * pCoolingMgmt)
{
    fbe_cooling_mgmt_fan_info_t             fanInfo;
    fbe_cooling_mgmt_fan_info_t             *pFanInfo = &fanInfo;
    fbe_common_cache_status_t               coolingCacheStatus = FBE_CACHE_STATUS_OK;
    fbe_common_cache_status_t               peerCoolingCacheStatus = FBE_CACHE_STATUS_OK;
    fbe_u32_t                               fanIndex;
    fbe_device_physical_location_t          location;
    fbe_status_t                            status = FBE_STATUS_OK;

    /*
     * Check the Multi-Fan fault status of enclosures involved with Caching
     */
    for (fanIndex = 0; fanIndex < pCoolingMgmt->fanCount; fanIndex++)
    {
        pFanInfo = &pCoolingMgmt->pFanInfo[fanIndex];

        if(((pFanInfo->location.bus == 0) && (pFanInfo->location.enclosure == 0)) ||
            (pFanInfo->location.bus == FBE_XPE_PSEUDO_BUS_NUM) && (pFanInfo->location.enclosure == FBE_XPE_PSEUDO_ENCL_NUM))
        {
            // This is PE fans or enclosure 0_0 fans
            if ((pFanInfo->basicInfo.multiFanModuleFault == FBE_MGMT_STATUS_TRUE) ||
                (pFanInfo->basicInfo.multiFanModuleFault == FBE_MGMT_STATUS_UNKNOWN) ||
                (pFanInfo->basicInfo.persistentMultiFanModuleFault == FBE_MGMT_STATUS_TRUE) ||
                (pFanInfo->basicInfo.persistentMultiFanModuleFault == FBE_MGMT_STATUS_UNKNOWN))
            {
                // only mark as Degraded.  We will let ShutdownReason notify us that enclosure is shutting down
                coolingCacheStatus = FBE_CACHE_STATUS_DEGRADED;
                fbe_base_object_trace((fbe_base_object_t *)pCoolingMgmt, 
                                      FBE_TRACE_LEVEL_INFO,
                                      FBE_TRACE_MESSAGE_ID_INFO,
                                      "%s, enclosure %d_%d failed cooling\n",
                                      __FUNCTION__,
                                      pFanInfo->location.bus,
                                      pFanInfo->location.enclosure);
                break;
            }
        }
    }

    /*
     * Check for a CacheStatus change
     */
    if (coolingCacheStatus != pCoolingMgmt->coolingCacheStatus)
    {
        fbe_base_object_trace((fbe_base_object_t *)pCoolingMgmt, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s, coolingCacheStatus changes from %d, to %d\n",
                              __FUNCTION__,
                              pCoolingMgmt->coolingCacheStatus,
                              coolingCacheStatus);

        /* Log the event for cache status change */
        fbe_event_log_write(ESP_INFO_CACHE_STATUS_CHANGED,
                    NULL, 0,
                    "%s %s %s",
                    "COOLING_MGMT",
                    fbe_base_env_decode_cache_status(pCoolingMgmt->coolingCacheStatus),
                    fbe_base_env_decode_cache_status(coolingCacheStatus));


        status = fbe_base_environment_get_peerCacheStatus((fbe_base_environment_t *)pCoolingMgmt,
                                                             &peerCoolingCacheStatus);
        /* Send CMI to peer */
        fbe_base_environment_send_cacheStatus_update_to_peer((fbe_base_environment_t *)pCoolingMgmt, 
                                                              coolingCacheStatus,
                                                             ((peerCoolingCacheStatus == FBE_CACHE_STATUS_UNINITIALIZE)? FALSE : TRUE));

        pCoolingMgmt->coolingCacheStatus = coolingCacheStatus;

        fbe_zero_memory(&location, sizeof(fbe_device_physical_location_t));     // location not relevant
        fbe_base_environment_send_data_change_notification((fbe_base_environment_t *)pCoolingMgmt, 
                                                           FBE_CLASS_ID_COOLING_MGMT, 
                                                           FBE_DEVICE_TYPE_SP_ENVIRON_STATE, 
                                                           FBE_DEVICE_DATA_TYPE_GENERIC_INFO,
                                                           &location);    
        fbe_base_object_trace((fbe_base_object_t *)pCoolingMgmt, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s, FBE_DEVICE_TYPE_SP_ENVIRON_STATE sent\n",
                              __FUNCTION__);
    }

    return FBE_STATUS_OK;

}

/*!**************************************************************
 * fbe_cooling_mgmt_fup_register_callback_cond_function()
 ****************************************************************
 * @brief
 *  This function registers the callback functions with the base environment object.
 * 
 * @param base_object - This is the object handle, or in our
 * case the ps_mgmt.
 * @param packet - The packet arriving from the monitor
 * scheduler.
 * 
 * @return fbe_status_t - The status of the call to
 *                        fbe_lifecycle_class_const_verify() for
 *                        the cooling management's constant
 *                        lifecycle data.
 *
 * @author
 *  25-Apr-2011: PHE - Created. 
 *
 ****************************************************************/
static fbe_lifecycle_status_t
fbe_cooling_mgmt_fup_register_callback_cond_function(fbe_base_object_t * base_object, 
                                                 fbe_packet_t * packet)
{
	fbe_status_t status = FBE_STATUS_OK;
    fbe_base_environment_t * pBaseEnv = (fbe_base_environment_t *)base_object;

    fbe_base_object_trace(base_object, 
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s entry\n",
                          __FUNCTION__);

    fbe_base_env_set_fup_get_full_image_path_callback(pBaseEnv, &fbe_cooling_mgmt_fup_get_full_image_path);
    fbe_base_env_set_fup_check_env_status_callback(pBaseEnv, &fbe_cooling_mgmt_fup_check_env_status);
    fbe_base_env_set_fup_info_ptr_callback(pBaseEnv, &fbe_cooling_mgmt_get_fup_info_ptr);
    fbe_base_env_set_fup_info_callback(pBaseEnv, &fbe_cooling_mgmt_get_fup_info);
    fbe_base_env_set_fup_get_firmware_rev_callback(pBaseEnv, &fbe_cooling_mgmt_fup_get_firmware_rev);
    fbe_base_env_set_fup_refresh_device_status_callback(pBaseEnv, &fbe_cooling_mgmt_fup_refresh_device_status);
    fbe_base_env_set_fup_initiate_upgrade_callback(pBaseEnv, &fbe_cooling_mgmt_fup_initiate_upgrade);
    fbe_base_env_set_fup_resume_upgrade_callback(pBaseEnv, &fbe_cooling_mgmt_fup_resume_upgrade);
    fbe_base_env_set_fup_priority_check_callback(pBaseEnv, (void *)NULL);
    fbe_base_env_set_fup_copy_fw_rev_to_resume_prom(pBaseEnv, (void *)fbe_cooling_mgmt_fup_copy_fw_rev_to_resume_prom);

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
 * fbe_cooling_mgmt_fup_register_callback_cond_function() 
 *******************************************************************/


/*!**************************************************************
 * fbe_cooling_mgmt_cmi_message_handler()
 ****************************************************************
 * @brief
 *  This is the handler function for CMI message notification.
 *  
 * @param pBaseObject - This is the object handle, or in our case the cooling_mgmt.
 * @param pCmiMsg - The point to fbe_base_environment_cmi_message_info_t
 *
 * @return fbe_status_t -  
 * 
 * @author
 *  28-Apr-2011:  PHE - Created.
 ****************************************************************/

fbe_status_t 
fbe_cooling_mgmt_cmi_message_handler(fbe_base_object_t *pBaseObject, 
                                fbe_base_environment_cmi_message_info_t * pCmiMsg)
{
    fbe_cooling_mgmt_t                 * pCoolingMgmt = (fbe_cooling_mgmt_t *)pBaseObject;
    fbe_cooling_mgmt_cmi_packet_t      * pCoolingCmiPkt = NULL;
    fbe_base_environment_cmi_message_t * pBaseCmiPkt = NULL;
    fbe_status_t                         status = FBE_STATUS_OK;

    // client message (if it exists)
    pCoolingCmiPkt = (fbe_cooling_mgmt_cmi_packet_t *)pCmiMsg->user_message;
    pBaseCmiPkt = (fbe_base_environment_cmi_message_t *)pCoolingCmiPkt;

    if(pBaseCmiPkt == NULL) 
    {
        fbe_base_object_trace(pBaseObject, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s, Event 0x%x, no msgType.\n",
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
                              fbe_base_env_decode_cmi_msg_opcode(pBaseCmiPkt->opcode),
                              (unsigned long long)pBaseCmiPkt->opcode);
    }

    switch (pCmiMsg->event)
    {
        case FBE_CMI_EVENT_MESSAGE_RECEIVED:
            status = fbe_cooling_mgmt_processReceivedCmiMessage(pCoolingMgmt, pCoolingCmiPkt);
            break;

        case FBE_CMI_EVENT_PEER_NOT_PRESENT:
            status = fbe_cooling_mgmt_processPeerNotPresent(pCoolingMgmt, pCoolingCmiPkt);
            break;

        case FBE_CMI_EVENT_SP_CONTACT_LOST:
            status = fbe_cooling_mgmt_processContactLost(pCoolingMgmt);
            break;

        case FBE_CMI_EVENT_PEER_BUSY:
            status = fbe_cooling_mgmt_processPeerBusy(pCoolingMgmt, pCoolingCmiPkt);
            break;

        case FBE_CMI_EVENT_FATAL_ERROR:
            status = fbe_cooling_mgmt_processFatalError(pCoolingMgmt, pCoolingCmiPkt);
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

} //fbe_cooling_mgmt_cmi_message_handler

/*!**************************************************************
 * @fn fbe_cooling_mgmt_processReceivedCmiMessage(fbe_cooling_mgmt_t * pCoolingMgmt, 
 *                                          fbe_cooling_mgmt_cmi_packet_t *pCoolingCmiPkt)
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
 *  27-Apr-2011: PHE - Created.
 ****************************************************************/
static fbe_status_t 
fbe_cooling_mgmt_processReceivedCmiMessage(fbe_cooling_mgmt_t * pCoolingMgmt, 
                                           fbe_cooling_mgmt_cmi_packet_t *pCoolingCmiPkt)
{
    fbe_base_environment_t             * pBaseEnv = (fbe_base_environment_t *)pCoolingMgmt;
    fbe_base_environment_cmi_message_t * pBaseCmiMsg = (fbe_base_environment_cmi_message_t *)pCoolingCmiPkt;
    fbe_base_env_fup_cmi_packet_t      * pFupCmiPkt = (fbe_base_env_fup_cmi_packet_t *)pCoolingCmiPkt;
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

        case FBE_BASE_ENV_CMI_MSG_CACHE_STATUS_UPDATE:
        status = fbe_cooling_mgmt_process_peer_cache_status_update(pCoolingMgmt, 
                                                                   pCoolingCmiPkt);
            break;

        case FBE_BASE_ENV_CMI_MSG_PEER_SP_ALIVE:
            // check if our earlier perm request failed
            status = fbe_cooling_mgmt_fup_new_contact_init_upgrade(pCoolingMgmt);
            break;

        default:
            status = FBE_STATUS_MORE_PROCESSING_REQUIRED;
            break;
    }

    return(status);

} //fbe_cooling_mgmt_processReceivedCmiMessage

/*!**************************************************************
 *  @fn fbe_cooling_mgmt_processPeerNotPresent(fbe_cooling_mgmt_t * pCoolingMgmt, 
 *                                      fbe_cooling_mgmt_cmi_packet_t *pCoolingCmiPkt)
 ****************************************************************
 * @brief
 *  This function gets called when recieving the CMI message with 
 *  FBE_CMI_EVENT_PEER_NOT_PRESENT.
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
 *  27-Apr-2011: PHE - Created.
 ****************************************************************/
static fbe_status_t 
fbe_cooling_mgmt_processPeerNotPresent(fbe_cooling_mgmt_t * pCoolingMgmt, 
                                       fbe_cooling_mgmt_cmi_packet_t * pCoolingCmiPkt)
{
    fbe_base_environment_cmi_message_t * pBaseCmiMsg = (fbe_base_environment_cmi_message_t *)pCoolingCmiPkt;
    fbe_base_env_fup_cmi_packet_t      * pFupCmiPkt = (fbe_base_env_fup_cmi_packet_t *)pCoolingCmiPkt;
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
            status = fbe_base_env_fup_processGotPeerPerm((fbe_base_environment_t *)pCoolingMgmt,
                                                         pFupCmiPkt->pRequestorWorkItem);
     
            break;

        default:
            status = FBE_STATUS_MORE_PROCESSING_REQUIRED;
            break;
    }

    return(status);
}

/*!**************************************************************
 * @fn fbe_cooling_mgmt_processContactLost(fbe_cooling_mgmt_t * pCoolingMgmt)
 ****************************************************************
 * @brief
 *  This function gets called when recieving the CMI message with 
 *  FBE_CMI_EVENT_SP_CONTACT_LOST.
 *
 * @param pCoolingMgmt - 
 *
 * @return fbe_status_t - always return
 *   FBE_STATUS_MORE_PROCESSING_REQUIRED - need to further handled by the base environment.
 * 
 * @author
 *  27-Apr-2011: PHE - Created.
 ****************************************************************/
static fbe_status_t fbe_cooling_mgmt_processContactLost(fbe_cooling_mgmt_t * pCoolingMgmt)
{
    fbe_status_t                    status = FBE_STATUS_OK;

    /* No need to handle the fup work items here. If the contact is lost, we will also receive
     * Peer Not Present message and we will handle the fup work item there. 
     */
    /* Peer SP went away. We need to check whether any FAN upgrade is needed from this SP's point of view.
     * Because if the peer SP went away before the upgrade completes, 
     * this SP needs to kick off the upgrade.
     */
    status = fbe_cooling_mgmt_fup_initiate_upgrade_for_all(pCoolingMgmt);

    /* Return FBE_STATUS_MORE_PROCESSING_REQUIRED so that it will do further processing in base_environment.*/
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}

/*!**************************************************************
 * @fn fbe_cooling_mgmt_processPeerBusy(fbe_cooling_mgmt_t * pCoolingMgmt, 
 *                                  fbe_cooling_mgmt_cmi_packet_t * pCoolingCmiPkt)
 ****************************************************************
 * @brief
 *  This function gets called when recieving the CMI message with 
 *  FBE_CMI_EVENT_PEER_NOT_PRESENT.
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
 *  27-Apr-2011: PHE - Created.
 ****************************************************************/
static fbe_status_t 
fbe_cooling_mgmt_processPeerBusy(fbe_cooling_mgmt_t * pCoolingMgmt, 
                                 fbe_cooling_mgmt_cmi_packet_t * pCoolingCmiPkt)
{
    fbe_base_environment_cmi_message_t * pBaseCmiMsg = (fbe_base_environment_cmi_message_t *)pCoolingCmiPkt;
    fbe_base_env_fup_cmi_packet_t      * pFupCmiPkt = (fbe_base_env_fup_cmi_packet_t *)pCoolingCmiPkt;
    fbe_status_t                         status = FBE_STATUS_OK;

    switch (pBaseCmiMsg->opcode)
    {
        case FBE_BASE_ENV_CMI_MSG_FUP_PEER_PERMISSION_GRANT:
        case FBE_BASE_ENV_CMI_MSG_FUP_PEER_PERMISSION_DENY:
            fbe_base_object_trace((fbe_base_object_t *)pCoolingMgmt, 
                          FBE_TRACE_LEVEL_ERROR,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s, should never happen, opcode 0x%llx\n",
                          __FUNCTION__,
                          (unsigned long long)pBaseCmiMsg->opcode);

            status = FBE_STATUS_GENERIC_FAILURE;
            break;

        case FBE_BASE_ENV_CMI_MSG_FUP_PEER_PERMISSION_REQUEST:
            // peer SP is not present when sending the message to request permission.
            status = fbe_base_env_fup_peerPermRetry((fbe_base_environment_t *)pCoolingMgmt, 
                                                    pFupCmiPkt->pRequestorWorkItem);
            break;

        default:
            status = FBE_STATUS_MORE_PROCESSING_REQUIRED;
            break;
    }

    return(status);
}

/*!**************************************************************
 * @fn fbe_cooling_mgmt_processFatalError(fbe_cooling_mgmt_t * pCoolingMgmt, 
 *                                        fbe_cooling_mgmt_cmi_packet_t * pCoolingCmiPkt)
 ****************************************************************
 * @brief
 *  This function gets called when recieving the CMI message with 
 *  FBE_CMI_EVENT_FATAL_ERROR.
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
 *  27-Apr-2011: PHE - Created.
 ****************************************************************/
static fbe_status_t 
fbe_cooling_mgmt_processFatalError(fbe_cooling_mgmt_t * pCoolingMgmt, 
                                   fbe_cooling_mgmt_cmi_packet_t * pCoolingCmiPkt)
{
    fbe_base_environment_cmi_message_t * pBaseCmiMsg = (fbe_base_environment_cmi_message_t *)pCoolingCmiPkt;
    fbe_base_env_fup_cmi_packet_t      * pFupCmiPkt = (fbe_base_env_fup_cmi_packet_t *)pCoolingCmiPkt;
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
            status = fbe_base_env_fup_processDeniedPeerPerm((fbe_base_environment_t *)pCoolingMgmt,
                                                         pFupCmiPkt->pRequestorWorkItem);
            break;

        default:
            status = FBE_STATUS_MORE_PROCESSING_REQUIRED;
            break;
    }

    return(status);
}

/*!**************************************************************
 * @fn fbe_cooling_mgmt_generate_fan_logs(fbe_cooling_mgmt_t * pCoolingMgmt, 
 *                                        fbe_device_physical_location_t *pLocation,
 *                                        fbe_cooling_info_t *pNewCoolingInfo,
 *                                        fbe_cooling_info_t *pOldCoolingInfo)
 ****************************************************************
 * @brief
 *  Generate logs pertaining to Fan insertion/removal and faults.
 *
 * @param pCoolingMgmt - 
 * @param pLocation - fan bus-encl-slot location info
 * @param pNewCoolingInfo - the new/current fan info to compare
 * @param pOldCoolingInfo - the previous fan info to comapre
 *
 * @return fbe_status_t -
 *   FBE_STATUS_MORE_PROCESSING_REQUIRED - need to further handled by the base environment.
 *   FBE_STATUS_OK - successful.
 *   others - failed.
 * 
 * @author
 *   7-Oct-2011: GB - Created.
 ****************************************************************/
fbe_status_t fbe_cooling_mgmt_generate_fan_logs(fbe_cooling_mgmt_t *pCoolingMgmt,
                                                fbe_device_physical_location_t *pLocation,
                                                fbe_cooling_info_t *pNewCoolingInfo,
                                                fbe_cooling_info_t *pOldCoolingInfo)
{
    fbe_status_t                status = FBE_STATUS_OK;
    fbe_u8_t                    deviceStr[FBE_DEVICE_STRING_LENGTH];

    status = fbe_base_env_create_device_string(FBE_DEVICE_TYPE_FAN, 
                                               pLocation, 
                                               &deviceStr[0],
                                               FBE_DEVICE_STRING_LENGTH);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)pCoolingMgmt,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                              "%s, Failed to create device string.\n", 
                              __FUNCTION__); 

        return status;
    }

    if (pNewCoolingInfo->envInterfaceStatus != pOldCoolingInfo->envInterfaceStatus)
    {
        if (pNewCoolingInfo->envInterfaceStatus == FBE_ENV_INTERFACE_STATUS_XACTION_FAIL)
        {
            fbe_event_log_write(ESP_ERROR_ENV_INTERFACE_FAILURE_DETECTED,
                                NULL, 0,
                                "%s %s", 
                                &deviceStr[0],
                                SmbTranslateErrorString(pNewCoolingInfo->transactionStatus));
        }
        else if (pNewCoolingInfo->envInterfaceStatus != FBE_ENV_INTERFACE_STATUS_GOOD)
        {
            fbe_event_log_write(ESP_ERROR_ENV_INTERFACE_FAILURE_DETECTED,
                                NULL, 0,
                                "%s %s", 
                                &deviceStr[0],
                                fbe_base_env_decode_env_status(pNewCoolingInfo->envInterfaceStatus));
        }
        else
        {
            fbe_event_log_write(ESP_INFO_ENV_INTERFACE_FAILURE_CLEARED,
                                NULL, 0,
                                "%s", 
                                &deviceStr[0]);
        }
    }

    // log inserted, removed or fault events
    if (pOldCoolingInfo->inserted == FBE_MGMT_STATUS_FALSE && 
        pNewCoolingInfo->inserted == FBE_MGMT_STATUS_TRUE)
    {
        status = fbe_event_log_write(ESP_INFO_FAN_INSERTED,
                                     NULL, 
                                     0,
                                     "%s", 
                                     &deviceStr[0]);
    }
    else if (pOldCoolingInfo->inserted == FBE_MGMT_STATUS_TRUE && 
             pNewCoolingInfo->inserted == FBE_MGMT_STATUS_FALSE)
    {
        status = fbe_event_log_write(ESP_ERROR_FAN_REMOVED,
                                     NULL, 
                                     0,
                                     "%s", 
                                     &deviceStr[0]);
    }

    if (pOldCoolingInfo->fanFaulted == FBE_MGMT_STATUS_TRUE && 
        pNewCoolingInfo->fanFaulted == FBE_MGMT_STATUS_FALSE)
    {
        status = fbe_event_log_write(ESP_INFO_FAN_FAULT_CLEARED,
                                     NULL, 
                                     0,
                                     "%s %s", 
                                     &deviceStr[0],
                                     "General Fault");
    }

    if (pOldCoolingInfo->fanFaulted == FBE_MGMT_STATUS_FALSE && 
        pNewCoolingInfo->fanFaulted == FBE_MGMT_STATUS_TRUE)
    {
        status = fbe_event_log_write(ESP_ERROR_FAN_FAULTED,
                                     NULL, 
                                     0,
                                     "%s %s", 
                                     &deviceStr[0],
                                     "General Fault");
    }

    if (pOldCoolingInfo->fanDegraded == FBE_MGMT_STATUS_FALSE && 
        pNewCoolingInfo->fanDegraded == FBE_MGMT_STATUS_TRUE)
    {
        status = fbe_event_log_write(ESP_WARN_FAN_DERGADED,
                                     NULL, 
                                     0,
                                     "%s %s", 
                                     &deviceStr[0], "Degraded");
    }

    return status;
} //fbe_cooling_mgmt_generate_fan_logs

/*!**************************************************************
 * @fn fbe_cooling_mgmt_process_peer_cache_status_update()
 ****************************************************************
 * @brief
 *  This function will handle the message we received from peer 
 *  to update peerCacheStatus.
 *
 * @param pCoolingMgmt -
 * @param cmiMsgPtr - Pointer to cooling_mgmt cmi packet
 * @param classId - cooling_mgmt class id.
 *
 * @return fbe_status_t 
 *
 * @author
 *  20-Nov-2012:  Dipak Patel - Created. 
 *
 ****************************************************************/
static fbe_status_t fbe_cooling_mgmt_process_peer_cache_status_update(fbe_cooling_mgmt_t * pCoolingMgmt, 
                                                                      fbe_cooling_mgmt_cmi_packet_t * cmiMsgPtr)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_base_env_cacheStatus_cmi_packet_t *baseCmiPtr = (fbe_base_env_cacheStatus_cmi_packet_t *)cmiMsgPtr;
        fbe_common_cache_status_t prevCombineCacheStatus = FBE_CACHE_STATUS_OK;
    fbe_common_cache_status_t peerCacheStatus = FBE_CACHE_STATUS_OK;  

    fbe_base_object_trace((fbe_base_object_t *)pCoolingMgmt, 
               FBE_TRACE_LEVEL_INFO,
               FBE_TRACE_MESSAGE_ID_INFO,
               "%s entry.\n",
               __FUNCTION__);


    status = fbe_base_environment_get_peerCacheStatus((fbe_base_environment_t *)pCoolingMgmt,
                                                       &peerCacheStatus); 

    prevCombineCacheStatus = fbe_base_environment_combine_cacheStatus((fbe_base_environment_t *)pCoolingMgmt,
                                                                       pCoolingMgmt->coolingCacheStatus,
                                                                       peerCacheStatus,
                                                                       FBE_CLASS_ID_COOLING_MGMT);


    /* Update Peer cache status for this side */
    status = fbe_base_environment_set_peerCacheStatus((fbe_base_environment_t *)pCoolingMgmt,
                                                       baseCmiPtr->CacheStatus);

    /* Compare it with local cache status and send notification to cache if require.*/ 
    status = fbe_base_environment_compare_cacheStatus((fbe_base_environment_t *)pCoolingMgmt,
                                                      prevCombineCacheStatus,
                                                      baseCmiPtr->CacheStatus,
                                                      FBE_CLASS_ID_COOLING_MGMT);

    /* Check if peer cache status is not initialized at peer side.
       If so, we need to send CMI message to peer to update it's
       peer cache status */
    if (baseCmiPtr->isPeerCacheStatusInitilized == FALSE)
    {
        /* Send CMI to peer.
           Over here, peerCacheStatus for this SP will intilized as we have update it above.
           So we are returning it with TRUE */

        fbe_base_object_trace((fbe_base_object_t *)pCoolingMgmt, 
                   FBE_TRACE_LEVEL_INFO,
                   FBE_TRACE_MESSAGE_ID_INFO,
                   "%s, peerCachestatus is uninitialized at peer\n",
                   __FUNCTION__);

        status = fbe_base_environment_send_cacheStatus_update_to_peer((fbe_base_environment_t *)pCoolingMgmt, 
                                                                       pCoolingMgmt->coolingCacheStatus,
                                                                       TRUE);
    }

    return status;
}

/*!***************************************************************
 * fbe_cooling_mgmt_calculateFanStateInfo()
 ****************************************************************
 * @brief
 *  This function will calculate & update the specified
 *  Fans State & SubState attributes.
 *
 * @param cooling_mgmt - Handle to cooling_mgmt.
 * @param pLocation - pointer to Fan location info
 *
 * @return fbe_status_t
 *
 * @author
 *  5-Mar-2015: Created  Joe Perry
 *
 ****************************************************************/
static fbe_status_t 
fbe_cooling_mgmt_calculateFanStateInfo(fbe_cooling_mgmt_t *cooling_mgmt,
                                       fbe_device_physical_location_t *pLocation)
{
    fbe_status_t                    status;
    fbe_cooling_mgmt_fan_info_t     *pFanInfo = NULL;

    status = fbe_cooling_mgmt_get_fan_info_by_location(cooling_mgmt, pLocation, &pFanInfo);
    if (status != FBE_STATUS_OK) 
    {
        fbe_base_object_trace((fbe_base_object_t*)cooling_mgmt, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s, getFanInfoByLoc failed, %d_%d_%d, status %d.\n",
                              __FUNCTION__, 
                              pLocation->bus, pLocation->enclosure, pLocation->slot, 
                              status);
        return status;
    }

    if (pFanInfo->basicInfo.isFaultRegFail)
    {
        pFanInfo->basicInfo.state = FBE_FAN_STATE_FAULTED;
        pFanInfo->basicInfo.subState = FBE_FAN_SUBSTATE_FLT_STATUS_REG_FAULT;
    }
    else if (pFanInfo->basicInfo.fupFailure && pFanInfo->basicInfo.inserted)
    {
        pFanInfo->basicInfo.state = FBE_FAN_STATE_DEGRADED;
        pFanInfo->basicInfo.subState = FBE_FAN_SUBSTATE_FUP_FAILURE;
    }
    else if (pFanInfo->basicInfo.resumePromReadFailed && pFanInfo->basicInfo.inserted)
    {
        pFanInfo->basicInfo.state = FBE_FAN_STATE_DEGRADED;
        pFanInfo->basicInfo.subState = FBE_FAN_SUBSTATE_RP_READ_FAILURE;
    }
    else if ((pFanInfo->basicInfo.envInterfaceStatus == FBE_ENV_INTERFACE_STATUS_XACTION_FAIL) ||
        (pFanInfo->basicInfo.envInterfaceStatus == FBE_ENV_INTERFACE_STATUS_DATA_STALE))
    {
        pFanInfo->basicInfo.state = FBE_FAN_STATE_DEGRADED;
        pFanInfo->basicInfo.subState = FBE_FAN_SUBSTATE_ENV_INTF_FAILURE;
    }
    else
    {
        pFanInfo->basicInfo.state = FBE_FAN_STATE_OK;
        pFanInfo->basicInfo.subState = FBE_FAN_SUBSTATE_NO_FAULT;
    }

    fbe_base_object_trace((fbe_base_object_t*)cooling_mgmt, 
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s, %d_%d_%d, state %s, substate %s\n",
                          __FUNCTION__,
                          pLocation->bus, pLocation->enclosure, pLocation->slot,
                          fbe_cooling_mgmt_decode_fan_state(pFanInfo->basicInfo.state),
                          fbe_cooling_mgmt_decode_fan_subState(pFanInfo->basicInfo.subState));
    return FBE_STATUS_OK;

}   // end of fbe_cooling_mgmt_calculateFanStateInfo

/* End of file fbe_cooling_mgmt_monitor.c */
