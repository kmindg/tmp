/***************************************************************************
 * Copyright (C) EMC Corporation 2010 
 * All rights reserved. Licensed material -- property of EMC 
 * Corporation 
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_ps_mgmt_monitor.c
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
#include "fbe_ps_mgmt_util.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_power_supply_interface.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_api_board_interface.h"
#include "fbe/fbe_api_enclosure_interface.h"
#include "fbe_transport_memory.h"
#include "fbe_lifecycle.h"
#include "EspMessages.h"
#include "fbe/fbe_event_log_api.h"
#include "fbe_base_environment_private.h"
#include "fbe_base_environment_debug.h"
#include "HardwareAttributesLib.h"
#include "generic_utils_lib.h"

static fbe_status_t fbe_ps_mgmt_checkCacheStatus(fbe_ps_mgmt_t * pPsMgmt);
/*!***************************************************************
 * fbe_ps_mgmt_monitor_entry()
 ****************************************************************
 * @brief
 *  Entry routine for the PS Management monitor.
 *
 * @param object_handle - This is the object handle, or in our
 * case the ps_mgmt.
 * @param packet_p - The packet arriving from the monitor
 * scheduler.
 *
 * @return fbe_status_t - Status of monitor operation.             
 *
 * @author
 *  15-Apr-2010:  Created. Joe Perry
 *
 ****************************************************************/
fbe_status_t 
fbe_ps_mgmt_monitor_entry(fbe_object_handle_t object_handle, fbe_packet_t * packet)
{
    fbe_ps_mgmt_t * ps_mgmt = NULL;

    ps_mgmt = (fbe_ps_mgmt_t *)fbe_base_handle_to_pointer(object_handle);

    fbe_topology_class_trace(FBE_CLASS_ID_PS_MGMT,
                 FBE_TRACE_LEVEL_DEBUG_HIGH, 
                 FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                 "%s entry\n", __FUNCTION__);
                 
    return fbe_lifecycle_crank_object(&FBE_LIFECYCLE_CONST_DATA(ps_mgmt),
                                      (fbe_base_object_t*)ps_mgmt, packet);
}
/******************************************
 * end fbe_ps_mgmt_monitor_entry()
 ******************************************/

/*!**************************************************************
 * fbe_ps_mgmt_monitor_load_verify()
 ****************************************************************
 * @brief
 *  This function simply validates the constant lifecycle data
 *  that is associated with the PS management.
 *
 * @param  - None.               
 *
 * @return fbe_status_t - The status of the call to
 *                        fbe_lifecycle_class_const_verify()
 *                        for the PS management's
 *                        constant lifecycle data.
 *
 * @author
 *  15-Apr-2010:  Created. Joe Perry
 *
 ****************************************************************/
fbe_status_t fbe_ps_mgmt_monitor_load_verify(void)
{
    return fbe_lifecycle_class_const_verify(&FBE_LIFECYCLE_CONST_DATA(ps_mgmt));
}
/******************************************
 * end fbe_ps_mgmt_monitor_load_verify()
 ******************************************/

/*--- local function prototypes --------------------------------------------------------*/

static fbe_lifecycle_status_t fbe_ps_mgmt_register_state_change_callback_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet);
static fbe_lifecycle_status_t fbe_ps_mgmt_register_cmi_callback_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet);
static fbe_lifecycle_status_t fbe_ps_mgmt_register_resume_prom_callback_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet);
static fbe_lifecycle_status_t fbe_ps_mgmt_discover_ps_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet);
static fbe_status_t fbe_ps_mgmt_state_change_handler(update_object_msg_t *update_object_msg, void *context);
static fbe_lifecycle_status_t fbe_ps_mgmt_fup_register_callback_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet);

static fbe_status_t fbe_ps_mgmt_handle_object_lifecycle_change(fbe_ps_mgmt_t * pPsMgmt, update_object_msg_t * pUpdateObjectMsg);
static fbe_status_t fbe_ps_mgmt_handle_destroy_state(fbe_ps_mgmt_t * pPsMgmt, update_object_msg_t * pUpdateObjectMsg);

static fbe_status_t fbe_ps_mgmt_handle_object_data_change(fbe_ps_mgmt_t * pPsMgmt, update_object_msg_t * pUpdateObjectMsg);
static fbe_status_t fbe_ps_mgmt_handle_generic_info_change(fbe_ps_mgmt_t * pPsMgmt, update_object_msg_t * pUpdateObjectMsg);

static fbe_status_t fbe_ps_mgmt_checkForPsEvents(fbe_ps_mgmt_t *ps_mgmt,
                             fbe_device_physical_location_t * location,
                             fbe_power_supply_info_t * pNewPsInfo,
                             fbe_power_supply_info_t * pOldPsInfo);

static fbe_status_t fbe_ps_mgmt_get_encl_ps_info_by_object_id(fbe_ps_mgmt_t * pPsMgmt,
                                                       fbe_object_id_t objectId,
                                                       fbe_encl_ps_info_t **pEnclPsInfoPtr);

static fbe_status_t fbe_ps_mgmt_find_available_encl_ps_info(fbe_ps_mgmt_t * pPsMgmt,
                                                     fbe_encl_ps_info_t ** pEnclPsInfoPtr);
static fbe_status_t fbe_ps_mgmt_updateEnclFaultLedPs(fbe_ps_mgmt_t *pPsMgmt,
                                                     fbe_device_physical_location_t * pLocation,
                                                     fbe_power_supply_info_t *pNewPsInfo,
                                                     fbe_power_supply_info_t *pOldPsInfo);

static fbe_status_t fbe_ps_mgmt_cmi_message_handler(fbe_base_object_t *pBaseObject, 
                                             fbe_base_environment_cmi_message_info_t *pCmiMsg);

static fbe_status_t fbe_ps_mgmt_processReceivedCmiMessage(fbe_ps_mgmt_t * pPsMgmt,
                                                          fbe_ps_mgmt_cmi_packet_t *pCmiMsg);
static fbe_status_t fbe_ps_mgmt_processPeerNotPresent(fbe_ps_mgmt_t * pPsMgmt, 
                              fbe_ps_mgmt_cmi_packet_t * pPsCmiPkt);
static fbe_status_t fbe_ps_mgmt_processPeerBusy(fbe_ps_mgmt_t * pPsMgmt, 
                              fbe_ps_mgmt_cmi_packet_t * pPsCmiPkt);
static fbe_status_t fbe_ps_mgmt_processContactLost(fbe_ps_mgmt_t * pPsMgmt);
static fbe_status_t fbe_ps_mgmt_processFatalError(fbe_ps_mgmt_t * pPsMgmt, 
                              fbe_ps_mgmt_cmi_packet_t * pPsCmiPkt);
static fbe_status_t fbe_ps_mgmt_create_ps_fault_string(fbe_power_supply_info_t *pPsInfo, char* string);

static fbe_status_t fbe_ps_mgmt_process_peer_cache_status_update(fbe_ps_mgmt_t *pPsMgmt, 
                                                                 fbe_ps_mgmt_cmi_packet_t * cmiMsgPtr);
static fbe_lifecycle_status_t fbe_ps_mgmt_debounce_timer_cond_function(fbe_base_object_t * base_object, 
                                                                       fbe_packet_t * packet);
static fbe_lifecycle_status_t 
fbe_ps_mgmt_read_persistent_data_complete_cond_function(fbe_base_object_t * base_object, 
                                                        fbe_packet_t * packet);

static fbe_status_t fbe_ps_mgmt_calculatePsStateInfo(fbe_ps_mgmt_t *ps_mgmt,
                                                        fbe_device_physical_location_t *pLocation);

/*--- lifecycle callback functions -----------------------------------------------------*/


FBE_LIFECYCLE_DEF_CALLBACK_GET_INST_DATA(ps_mgmt);
FBE_LIFECYCLE_DEF_CALLBACK_GET_INST_COND(ps_mgmt);

/*  ps_mgmt_lifecycle_callbacks This variable has all the
 *  callbacks the lifecycle needs.
 */
static fbe_lifecycle_const_callback_t FBE_LIFECYCLE_CALLBACKS(ps_mgmt) = {
    FBE_LIFECYCLE_DEF_CONST_CALLBACKS(
        ps_mgmt,
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
static fbe_lifecycle_const_cond_t fbe_ps_mgmt_register_state_change_callback_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_ENV_LIFECYCLE_COND_REGISTER_STATE_CHANGE_CALLBACK,
        fbe_ps_mgmt_register_state_change_callback_cond_function)
};

/* FBE_BASE_ENV_LIFECYCLE_COND_REGISTER_CMI_CALLBACK condition
 *  - The purpose of this derived condition is to register
 *  notification with the base environment object which in turn
 *  registers with the CMI. The leaf class needs to implement
 *  the actual condition handler function providing the callback
 *  that needs to be called
 */
static fbe_lifecycle_const_cond_t fbe_ps_mgmt_register_cmi_callback_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_ENV_LIFECYCLE_COND_REGISTER_CMI_CALLBACK,
        fbe_ps_mgmt_register_cmi_callback_cond_function)
};

/* FBE_BASE_ENV_LIFECYCLE_COND_FUP_REGISTER_CALLBACK condition
 *  - The purpose of this derived condition is to register
 *  call back functions with the base environment object. 
 * The leaf class needs to implement the actual call back functions.
 */
static fbe_lifecycle_const_cond_t fbe_ps_mgmt_fup_register_callback_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_ENV_LIFECYCLE_COND_FUP_REGISTER_CALLBACK,
        fbe_ps_mgmt_fup_register_callback_cond_function)
};

static fbe_lifecycle_const_cond_t fbe_ps_mgmt_fup_abort_upgrade_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_ENV_LIFECYCLE_COND_FUP_ABORT_UPGRADE,
        fbe_base_env_fup_abort_upgrade_cond_function)
};

static fbe_lifecycle_const_cond_t fbe_ps_mgmt_fup_wait_before_upgrade_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_ENV_LIFECYCLE_COND_FUP_WAIT_BEFORE_UPGRADE,
        fbe_base_env_fup_wait_before_upgrade_cond_function)
};

static fbe_lifecycle_const_cond_t fbe_ps_mgmt_fup_wait_for_inter_device_delay_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_ENV_LIFECYCLE_COND_FUP_WAIT_FOR_INTER_DEVICE_DELAY,
        fbe_base_env_fup_wait_for_inter_device_delay_cond_function)
};

static fbe_lifecycle_const_cond_t fbe_ps_mgmt_fup_read_image_header_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_ENV_LIFECYCLE_COND_FUP_READ_IMAGE_HEADER,
        fbe_base_env_fup_read_image_header_cond_function)
};

static fbe_lifecycle_const_cond_t fbe_ps_mgmt_fup_check_rev_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_ENV_LIFECYCLE_COND_FUP_CHECK_REV,
        fbe_base_env_fup_check_rev_cond_function)
};

static fbe_lifecycle_const_cond_t fbe_ps_mgmt_fup_read_entire_image_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_ENV_LIFECYCLE_COND_FUP_READ_ENTIRE_IMAGE,
        fbe_base_env_fup_read_entire_image_cond_function)
};

static fbe_lifecycle_const_cond_t fbe_ps_mgmt_fup_download_image_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_ENV_LIFECYCLE_COND_FUP_DOWNLOAD_IMAGE,
        fbe_base_env_fup_download_image_cond_function)
};

static fbe_lifecycle_const_cond_t fbe_ps_mgmt_fup_get_download_status_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_ENV_LIFECYCLE_COND_FUP_GET_DOWNLOAD_STATUS,
        fbe_base_env_fup_get_download_status_cond_function)
};

static fbe_lifecycle_const_cond_t fbe_ps_mgmt_fup_get_peer_permission_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_ENV_LIFECYCLE_COND_FUP_GET_PEER_PERMISSION,
        fbe_base_env_fup_get_peer_permission_cond_function)
};

static fbe_lifecycle_const_cond_t fbe_ps_mgmt_fup_check_env_status_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_ENV_LIFECYCLE_COND_FUP_CHECK_ENV_STATUS,
        fbe_base_env_fup_check_env_status_cond_function)
};

static fbe_lifecycle_const_cond_t fbe_ps_mgmt_fup_activate_image_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_ENV_LIFECYCLE_COND_FUP_ACTIVATE_IMAGE,
        fbe_base_env_fup_activate_image_cond_function)
};

static fbe_lifecycle_const_cond_t fbe_ps_mgmt_fup_get_activate_status_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_ENV_LIFECYCLE_COND_FUP_GET_ACTIVATE_STATUS,
        fbe_base_env_fup_get_activate_status_cond_function)
};

static fbe_lifecycle_const_cond_t fbe_ps_mgmt_fup_check_result_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_ENV_LIFECYCLE_COND_FUP_CHECK_RESULT,
        fbe_base_env_fup_check_result_cond_function)
};

static fbe_lifecycle_const_cond_t fbe_ps_mgmt_fup_refresh_device_status_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_ENV_LIFECYCLE_COND_FUP_REFRESH_DEVICE_STATUS,
        fbe_base_env_fup_refresh_device_status_cond_function)
};

static fbe_lifecycle_const_cond_t fbe_ps_mgmt_fup_end_upgrade_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_ENV_LIFECYCLE_COND_FUP_END_UPGRADE,
        fbe_base_env_fup_end_upgrade_cond_function)
};

static fbe_lifecycle_const_cond_t fbe_ps_mgmt_fup_release_image_cond = {
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
static fbe_lifecycle_const_cond_t fbe_ps_mgmt_register_resume_prom_callback_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_ENV_LIFECYCLE_COND_RESUME_PROM_REGISTER_CALLBACK,
        fbe_ps_mgmt_register_resume_prom_callback_cond_function)
};


/*--- constant base condition entries --------------------------------------------------*/

/* FBE_PS_MGMT_DISCOVER_PS condition -
 *   The purpose of this condition is start the discovery process
 *   of a new PS that was added
 */
static fbe_lifecycle_const_base_cond_t 
    fbe_ps_mgmt_discover_ps_cond = {
        FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "fbe_ps_mgmt_discover_ps_cond",
        FBE_PS_MGMT_DISCOVER_PS,
        fbe_ps_mgmt_discover_ps_cond_function),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,         /* ACTIVATE */
        FBE_LIFECYCLE_STATE_READY,          /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)         /* DESTROY */
};

/* FBE_SPS_MGMT_DEBOUNCE_TIMER_COND condition 
 *   The purpose of this condition is verify a status has existed for a
 *   certain amount of time
 */
static fbe_lifecycle_const_base_timer_cond_t fbe_ps_mgmt_debounce_timer_cond = {
    {
        FBE_LIFECYCLE_DEF_CONST_BASE_TIMER_COND(
            "fbe_ps_mgmt_debounce_timer_cond",
            FBE_PS_MGMT_DEBOUNCE_TIMER_COND,
            fbe_ps_mgmt_debounce_timer_cond_function),
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
    600 /* fires every 6 seconds */
};

/*--- constant derived condition entries -----------------------------------------------*/
/* FBE_BASE_ENV_LIFECYCLE_COND_READ_PERSISTENT_DATA_COMPLETE condition
 *  - The purpose of this derived condition is to handle the completion
 *    of reading the data from persistent storage.  
 */
static fbe_lifecycle_const_cond_t fbe_ps_mgmt_read_persistent_data_complete_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_ENV_LIFECYCLE_COND_READ_PERSISTENT_DATA_COMPLETE,
        fbe_ps_mgmt_read_persistent_data_complete_cond_function)
};

static fbe_lifecycle_const_base_cond_t * FBE_LIFECYCLE_BASE_COND_ARRAY(ps_mgmt)[] = {
    &fbe_ps_mgmt_discover_ps_cond,
    (fbe_lifecycle_const_base_cond_t*)&fbe_ps_mgmt_debounce_timer_cond,
};

FBE_LIFECYCLE_DEF_CONST_BASE_CONDITIONS(ps_mgmt);

/*--- constant rotaries ----------------------------------------------------------------*/

static fbe_lifecycle_const_rotary_cond_t fbe_ps_mgmt_specialize_rotary[] = {
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_ps_mgmt_register_state_change_callback_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET), 
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_ps_mgmt_register_cmi_callback_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_ps_mgmt_fup_register_callback_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_ps_mgmt_register_resume_prom_callback_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
};

static fbe_lifecycle_const_rotary_cond_t fbe_ps_mgmt_activate_rotary[] = {
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_ps_mgmt_read_persistent_data_complete_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_ps_mgmt_discover_ps_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
};

static fbe_lifecycle_const_rotary_cond_t fbe_ps_mgmt_ready_rotary[] = {
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_ps_mgmt_debounce_timer_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_ps_mgmt_fup_wait_before_upgrade_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_ps_mgmt_fup_release_image_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),

    /* Moved the following two conditions up. (AR651486)
     * so that work item can be removed from the queue immediately after the device gets removed.
     */
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_ps_mgmt_fup_refresh_device_status_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_ps_mgmt_fup_end_upgrade_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),

    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_ps_mgmt_fup_wait_for_inter_device_delay_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_ps_mgmt_fup_read_image_header_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),  
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_ps_mgmt_fup_check_rev_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_ps_mgmt_fup_read_entire_image_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_ps_mgmt_fup_get_peer_permission_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_ps_mgmt_fup_check_env_status_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_ps_mgmt_fup_download_image_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_ps_mgmt_fup_get_download_status_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_ps_mgmt_fup_activate_image_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_ps_mgmt_fup_get_activate_status_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_ps_mgmt_fup_check_result_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    
    /* Need to put the abort condition in the end of all the fup conditions. 
     * We want to complete the execution of other condition which was already set before running the abort upgrade 
     * condition. Otherwise, when resuming the upgrade, the condition which was set before would get to run and 
     * it would make the upgrade out of sequence.
     */
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_ps_mgmt_fup_abort_upgrade_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
};

static fbe_lifecycle_const_rotary_t FBE_LIFECYCLE_ROTARY_ARRAY(ps_mgmt)[] = {
    FBE_LIFECYCLE_DEF_CONST_ROTARY(FBE_LIFECYCLE_STATE_SPECIALIZE, fbe_ps_mgmt_specialize_rotary),
    FBE_LIFECYCLE_DEF_CONST_ROTARY(FBE_LIFECYCLE_STATE_ACTIVATE, fbe_ps_mgmt_activate_rotary),
    FBE_LIFECYCLE_DEF_CONST_ROTARY(FBE_LIFECYCLE_STATE_READY, fbe_ps_mgmt_ready_rotary),
};

FBE_LIFECYCLE_DEF_CONST_ROTARIES(ps_mgmt);

/*--- global ps mgmt lifecycle constant data ----------------------------------------*/

fbe_lifecycle_const_t FBE_LIFECYCLE_CONST_DATA(ps_mgmt) = {
    FBE_LIFECYCLE_DEF_CONST_DATA(
        ps_mgmt,
        FBE_CLASS_ID_PS_MGMT,                  /* This class */
        FBE_LIFECYCLE_CONST_DATA(base_environment))    /* The super class */
};

/*--- Condition Functions --------------------------------------------------------------*/

/*!**************************************************************
 * fbe_ps_mgmt_register_state_change_callback_cond_function()
 ****************************************************************
 * @brief
 *  This function registers with the physical package to get
 *  notified on PS object life cycle state changes
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
 *  15-Apr-2010:  Created. Joe Perry
 *
 ****************************************************************/
static fbe_lifecycle_status_t
fbe_ps_mgmt_register_state_change_callback_cond_function(fbe_base_object_t * base_object, 
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
                                                              (fbe_api_notification_callback_function_t)fbe_ps_mgmt_state_change_handler,
                                                              (FBE_NOTIFICATION_TYPE_OBJECT_DATA_CHANGED | FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_DESTROY),
                                                              (FBE_DEVICE_TYPE_PS),
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
 * fbe_ps_mgmt_register_state_change_callback_cond_function() 
 *******************************************************************/

/*!**************************************************************
 * fbe_ps_mgmt_register_cmi_callback_cond_function()
 ****************************************************************
 * @brief
 *  This function registers with the physical package to get
 *  notified on PS object life cycle state changes
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
 *  15-Apr-2010:  Created. Joe Perry
 *
 ****************************************************************/
static fbe_lifecycle_status_t
fbe_ps_mgmt_register_cmi_callback_cond_function(fbe_base_object_t * base_object, 
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
                                                            FBE_CMI_CLIENT_ID_PS_MGMT,
                                                            fbe_ps_mgmt_cmi_message_handler);

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
 * fbe_ps_mgmt_register_cmi_callback_cond_function() 
 *******************************************************************/

/*!**************************************************************
 * fbe_ps_mgmt_fup_register_callback_cond_function()
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
 *                        the PS management's constant
 *                        lifecycle data.
 *
 * @author
 *  02-July-2010: PHE - Created. 
 *
 ****************************************************************/
static fbe_lifecycle_status_t
fbe_ps_mgmt_fup_register_callback_cond_function(fbe_base_object_t * base_object, 
                                                 fbe_packet_t * packet)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_base_environment_t * pBaseEnv = (fbe_base_environment_t *)base_object;

    fbe_base_object_trace(base_object, 
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s entry\n",
                          __FUNCTION__);

    fbe_base_env_set_fup_get_full_image_path_callback(pBaseEnv, &fbe_ps_mgmt_fup_get_full_image_path);
    fbe_base_env_set_fup_get_manifest_file_full_path_callback(pBaseEnv, &fbe_ps_mgmt_fup_get_manifest_file_full_path);
    fbe_base_env_set_fup_check_env_status_callback(pBaseEnv, &fbe_ps_mgmt_fup_check_env_status);
    fbe_base_env_set_fup_info_ptr_callback(pBaseEnv, &fbe_ps_mgmt_get_fup_info_ptr);
    fbe_base_env_set_fup_info_callback(pBaseEnv, &fbe_ps_mgmt_get_fup_info);
    fbe_base_env_set_fup_get_firmware_rev_callback(pBaseEnv, &fbe_ps_mgmt_fup_get_firmware_rev);
    fbe_base_env_set_fup_refresh_device_status_callback(pBaseEnv, &fbe_ps_mgmt_fup_refresh_device_status);
    fbe_base_env_set_fup_initiate_upgrade_callback(pBaseEnv, &fbe_ps_mgmt_fup_initiate_upgrade);
    fbe_base_env_set_fup_resume_upgrade_callback(pBaseEnv, &fbe_ps_mgmt_fup_resume_upgrade);
    fbe_base_env_set_fup_priority_check_callback(pBaseEnv, (void *)NULL);
    fbe_base_env_set_fup_copy_fw_rev_to_resume_prom(pBaseEnv, (fbe_base_env_fup_copy_fw_rev_to_resume_prom_callback_func_ptr_t)&fbe_ps_mgmt_fup_copy_fw_rev_to_resume_prom);

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
 * fbe_ps_mgmt_fup_register_callback_cond_function() 
 *******************************************************************/

/*!**************************************************************
 * @fn fbe_ps_mgmt_state_change_handler()
 ****************************************************************
 * @brief
 *  This is the handler function for state change notification.
 *  
 *
 * @param object_handle - This is the object handle, or in our
 * case the ps_mgmt.
 * @param packet_p - The packet arriving from the monitor
 * scheduler.
 *
 * @return fbe_status_t - The status of the call to
 *                        fbe_lifecycle_class_const_verify()
 *                        for the PS management's
 *                        constant lifecycle data.
 * 
 * 
 * @author
 *  15-Apr-2010:  Created. Joe Perry
 *  08-Jul-2010: PHE - Created fbe_ps_mgmt_handle_object_data_change.
 *
 ****************************************************************/
static fbe_status_t 
fbe_ps_mgmt_state_change_handler(update_object_msg_t *update_object_msg, void *context)
{
    fbe_notification_info_t *notification_info = &update_object_msg->notification_info;
    fbe_ps_mgmt_t          *ps_mgmt;
    fbe_base_object_t       *base_object;
    fbe_status_t            status = FBE_STATUS_OK;
    fbe_notification_data_changed_info_t * pDataChangedInfo = NULL;

    ps_mgmt = (fbe_ps_mgmt_t *)context;
    base_object = (fbe_base_object_t *)ps_mgmt;
    
    fbe_base_object_trace(base_object, 
                          FBE_TRACE_LEVEL_INFO,
//                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s entry, objectId 0x%x, notifType 0x%llx, objType 0x%llx, devMsk 0x%llx\n",
                          __FUNCTION__, 
                          update_object_msg->object_id,
                          (unsigned long long)notification_info->notification_type, 
                          (unsigned long long)notification_info->object_type, 
                          notification_info->notification_data.data_change_info.device_mask);

    
    pDataChangedInfo = &(notification_info->notification_data.data_change_info);
    /*
     * Process the notification
     */

    if (notification_info->notification_type & FBE_NOTIFICATION_TYPE_LIFECYCLE_ANY_STATE_CHANGE) {
        status = fbe_ps_mgmt_handle_object_lifecycle_change(ps_mgmt, update_object_msg);
        return FBE_STATUS_OK;
    }

    switch (notification_info->notification_type)
    {
        case FBE_NOTIFICATION_TYPE_OBJECT_DATA_CHANGED:
            status = fbe_ps_mgmt_handle_object_data_change(ps_mgmt, update_object_msg);
            break;
  
        
        default:
            return FBE_STATUS_OK;
            break;
    }
    return FBE_STATUS_OK;
}//End of function fbe_ps_mgmt_state_change_handler


/*!**************************************************************
 * @fn fbe_ps_mgmt_handle_object_data_change()
 ****************************************************************
 * @brief
 *  This function is to handle the object data change.
 *
 * @param pPsMgmt - This is the object handle, or in our case the ps_mgmt.
 * @param pUpdateObjectMsg - Pointer to the notification info
 *
 * @return fbe_status_t - FBE Status
 * 
 * @author
 *  17-Jul-2010:  PHE - Created.
 *
 ****************************************************************/
static fbe_status_t fbe_ps_mgmt_handle_object_data_change(fbe_ps_mgmt_t * pPsMgmt, 
                                                          update_object_msg_t * pUpdateObjectMsg)
{
    fbe_notification_data_changed_info_t * pDataChangedInfo = NULL;
    fbe_device_physical_location_t       * pLocation = NULL;
    fbe_status_t                           status = FBE_STATUS_OK;

    pDataChangedInfo = &(pUpdateObjectMsg->notification_info.notification_data.data_change_info);
    pLocation = &(pDataChangedInfo->phys_location);

    fbe_base_object_trace((fbe_base_object_t *)pPsMgmt, 
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s entry, dataType 0x%x.\n",
                          __FUNCTION__, pDataChangedInfo->data_type);

    status = fbe_base_env_get_and_adjust_encl_location((fbe_base_environment_t *)pPsMgmt, 
                                                       pUpdateObjectMsg->object_id, 
                                                       pLocation);
    if (status != FBE_STATUS_OK) 
    {
        fbe_base_object_trace((fbe_base_object_t *)pPsMgmt, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s get and adjust location for objectId %d failed, status: 0x%x.\n",
                              __FUNCTION__, pUpdateObjectMsg->object_id, status);

        return FBE_LIFECYCLE_STATUS_DONE;
    }
    
    switch(pDataChangedInfo->data_type)
    {
        case FBE_DEVICE_DATA_TYPE_GENERIC_INFO:
            status = fbe_ps_mgmt_handle_generic_info_change(pPsMgmt, pUpdateObjectMsg);
            break;

        case FBE_DEVICE_DATA_TYPE_FUP_INFO:
            status = fbe_base_env_fup_handle_firmware_status_change((fbe_base_environment_t *)pPsMgmt, 
                                                               pDataChangedInfo->device_mask,
                                                               pLocation);
            break;

        default:
            break;
    }

    return status;
}// End of function fbe_ps_mgmt_handle_object_data_change

/*!**************************************************************
 * @fn fbe_ps_mgmt_handle_generic_info_change()
 ****************************************************************
 * @brief
 *  This function is to handle the generic info change.
 *
 * @param pPsMgmt - This is the object handle, or in our case the ps_mgmt.
 * @param pUpdateObjectMsg - Pointer to the notification info
 *
 * @return fbe_status_t - FBE Status
 * 
 * @author
 *  15-Apr-2010:  Created. Joe Perry
 *  17-Jul-2010:  PHE - Moved from the function fbe_ps_mgmt_state_change_handler.
 *
 ****************************************************************/
static fbe_status_t fbe_ps_mgmt_handle_generic_info_change(fbe_ps_mgmt_t * pPsMgmt, 
                                                          update_object_msg_t * pUpdateObjectMsg)
{
    fbe_notification_data_changed_info_t   * pDataChangedInfo = NULL;
    fbe_device_physical_location_t         * pLocation = NULL;
    fbe_status_t status = FBE_STATUS_OK;

    pDataChangedInfo = &(pUpdateObjectMsg->notification_info.notification_data.data_change_info);
    pLocation = &(pDataChangedInfo->phys_location);
    
    switch (pDataChangedInfo->device_mask)
    {
        case FBE_DEVICE_TYPE_PS:
            status = fbe_ps_mgmt_processPsStatus(pPsMgmt, pUpdateObjectMsg->object_id, pLocation);
            if (status != FBE_STATUS_OK) 
            {
                fbe_base_object_trace((fbe_base_object_t *)pPsMgmt, 
                                      FBE_TRACE_LEVEL_WARNING,
                                      FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                      "%s PS(%d_%d_%d) Error in processing status, status 0x%X.\n",
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
}// End of function fbe_ps_mgmt_handle_generic_info_change

/*!**************************************************************
 * @fn fbe_ps_mgmt_handle_object_lifecycle_change()
 ****************************************************************
 * @brief
 *  This function is to handle the lifecycle change.
 *
 * @param pPsMgmt - This is the object handle, or in our case the ps_mgmt.
 * @param pUpdateObjectMsg - Pointer to the notification info
 *
 * @return fbe_status_t - FBE Status
 * 
 * @author
 *  08-Jul-2010:  PHE - Created.
 *
 ****************************************************************/
static fbe_status_t fbe_ps_mgmt_handle_object_lifecycle_change(fbe_ps_mgmt_t * pPsMgmt, 
                                                               update_object_msg_t * pUpdateObjectMsg)
{
    fbe_status_t status;
    

    switch(pUpdateObjectMsg->notification_info.notification_type) 
    {
        case FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_DESTROY:
            status = fbe_ps_mgmt_handle_destroy_state(pPsMgmt, pUpdateObjectMsg);
            break;

        default:
            fbe_base_object_trace((fbe_base_object_t *)pPsMgmt, 
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s Unhandled state 0x%llX.\n",
                                  __FUNCTION__, (unsigned long long)pUpdateObjectMsg->notification_info.notification_type);
            status = FBE_STATUS_GENERIC_FAILURE;
            break;
    }

    return status;
}// End of function fbe_ps_mgmt_handle_object_lifecycle_change


/*!**************************************************************
 * @fn fbe_ps_mgmt_handle_destroy_state()
 ****************************************************************
 * @brief
 *  This is the handler function for life cycle change to the
 *  destroy state.
 *
 * @param pPsMgmt - This is the object handle, or in our case the fbe_ps_mgmt_t.
 * @param pUpdateObjectMsg - Pointer to the notification info
 *
 * @return fbe_status_t - FBE Status
 * 
 * @author
 *  08-Jul-2010:  PHE - Created. 
 *  03-Mar-2011:  Arun S - Added code to handle the PS removal for resume prom. 
 *
 ****************************************************************/
static fbe_status_t fbe_ps_mgmt_handle_destroy_state(fbe_ps_mgmt_t * pPsMgmt, 
                                                     update_object_msg_t * pUpdateObjectMsg)
{
    fbe_object_id_t objectId;
    fbe_encl_ps_info_t * pEnclPsInfo = NULL;
    fbe_u32_t            bus = 0;
    fbe_u32_t enclosure = 0;
    fbe_status_t         status = FBE_STATUS_OK;
    fbe_u32_t            psCount;
    fbe_u32_t            index;
    fbe_device_physical_location_t phys_location = {0};

    objectId = pUpdateObjectMsg->object_id;

    status = fbe_ps_mgmt_get_encl_ps_info_by_object_id(pPsMgmt, objectId, &pEnclPsInfo);

    if(status != FBE_STATUS_OK)
    {
        /* This is the coding error.*/
        fbe_base_object_trace((fbe_base_object_t *)pPsMgmt,  
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s get_encl_ps_info_by_object_id failed, objectId 0x%X.\n",
                              __FUNCTION__, objectId);
        return status;
    }
   
    bus = pEnclPsInfo->location.bus;
    enclosure = pEnclPsInfo->location.enclosure;
    psCount = pEnclPsInfo->psCount;

    fbe_base_object_trace((fbe_base_object_t *)pPsMgmt, 
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s Bus:%d Encl:%d Removed.\n",
                          __FUNCTION__, bus, enclosure);

    status = fbe_base_env_fup_handle_destroy_state((fbe_base_environment_t *)pPsMgmt, &pEnclPsInfo->location);

    if(status != FBE_STATUS_OK) 
    {
        fbe_base_object_trace((fbe_base_object_t *)pPsMgmt, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "FUP:Encl(%d_%d),%s,fup_handle_destroy_state failed, status 0x%X.\n",
                              bus, 
                              enclosure, 
                              __FUNCTION__,
                              status);
    }
   
    /* Containg Enclosure is destroyed/Removed. Clear the Resume Prom Info */
    status = fbe_base_env_resume_prom_handle_destroy_state((fbe_base_environment_t *)pPsMgmt, bus, enclosure);

    if(status != FBE_STATUS_OK) 
    {
        fbe_base_object_trace((fbe_base_object_t *)pPsMgmt, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "RP:Encl(%d_%d),%s,resume_prom_handle_destroy_state failed, status 0x%X.\n",
                              bus, 
                              enclosure,
                              __FUNCTION__,
                              status);
    }

    /* Since the enclosure was destroyed clean out the structure */
    status = fbe_ps_mgmt_init_encl_ps_info(pEnclPsInfo);
    if(status == FBE_STATUS_OK)
    {
        pPsMgmt->enclCount--;
        phys_location.bus = bus;
        phys_location.enclosure = enclosure;
        /* Send notification for ps_mgmt data change */
        for(index = 0; index < psCount; index++)
        {
            phys_location.slot = index;
            fbe_base_environment_send_data_change_notification((fbe_base_environment_t*)pPsMgmt, 
                                                                FBE_CLASS_ID_PS_MGMT, 
                                                                FBE_DEVICE_TYPE_PS, 
                                                                FBE_DEVICE_DATA_TYPE_GENERIC_INFO,
                                                                &phys_location);
        }            
    }
    return status;
}// End of function fbe_ps_mgmt_handle_destroy_state

/*!***************************************************************
 * fbe_ps_mgmt_get_encl_ps_info_by_object_id()
 ****************************************************************
 * @brief
 *  This function searches through the enclosure PS array and
 *  returns the pointer to the encl ps info.
 *
 * @param pPsMgmt -
 * @param objectId - 
 * @param pEnclPsInfoPtr (OUTPUT) -
 *
 * @return fbe_status_t
 *
 * @author
 *  08-Aug-2010 PHE - Created.
 *
 ****************************************************************/
static fbe_status_t fbe_ps_mgmt_get_encl_ps_info_by_object_id(fbe_ps_mgmt_t * pPsMgmt,
                                                       fbe_object_id_t objectId,
                                                       fbe_encl_ps_info_t **pEnclPsInfoPtr)
{
    fbe_u32_t             enclIndex = 0;
    fbe_encl_ps_info_t  * pEnclPsInfo = NULL;
    
    *pEnclPsInfoPtr = NULL;

    for(enclIndex = 0; enclIndex < FBE_ESP_MAX_ENCL_COUNT; enclIndex++)
    {
        pEnclPsInfo = &(pPsMgmt->psInfo->enclPsInfo[enclIndex]);

        if((pEnclPsInfo->objectId == objectId) &&
           (objectId != FBE_OBJECT_ID_INVALID))
        {
            // found enclosure
            *pEnclPsInfoPtr = pEnclPsInfo;
            return FBE_STATUS_OK;
        }
    }

    return FBE_STATUS_COMPONENT_NOT_FOUND;
}


/*!***************************************************************
 * fbe_ps_mgmt_get_encl_ps_info_by_location()
 ****************************************************************
 * @brief
 *  This function searches through the enclosure PS array and
 *  returns the pointer to the encl ps info.
 *
 * @param pPsMgmt -
 * @param objectId - 
 * @param pEnclPsInfoPtr (OUTPUT) -
 *
 * @return fbe_status_t
 *
 * @author
 *  08-Aug-2010 PHE - Created.
 *
 ****************************************************************/
fbe_status_t fbe_ps_mgmt_get_encl_ps_info_by_location(fbe_ps_mgmt_t * pPsMgmt,
                                                       fbe_device_physical_location_t * pLocation,
                                                       fbe_encl_ps_info_t **pEnclPsInfoPtr)
{
    fbe_u32_t             enclIndex = 0;
    fbe_encl_ps_info_t  * pEnclPsInfo = NULL;
    
    *pEnclPsInfoPtr = NULL;

    for(enclIndex = 0; enclIndex < FBE_ESP_MAX_ENCL_COUNT; enclIndex++)
    {
        pEnclPsInfo = &(pPsMgmt->psInfo->enclPsInfo[enclIndex]);

        if((pEnclPsInfo->objectId != FBE_OBJECT_ID_INVALID) &&
           (pEnclPsInfo->location.bus == pLocation->bus) &&
           (pEnclPsInfo->location.enclosure == pLocation->enclosure))
        {
            // found enclosure
            *pEnclPsInfoPtr = pEnclPsInfo;
            return FBE_STATUS_OK;
        }
    }

    return FBE_STATUS_NO_DEVICE;
}

/*!***************************************************************
 * fbe_ps_mgmt_get_encl_ps_fup_info_by_location()
 ****************************************************************
 * @brief
 *  This function searches through the enclosure PS array and
 *  returns the pointer to the encl ps fup info.
 *
 * @param pPsMgmt -
 * @param objectId - 
 * @param pEnclPsInfoPtr (OUTPUT) -
 *
 * @return fbe_status_t
 *
 * @author
 *  08-Aug-2010 PHE - Created.
 ****************************************************************/
fbe_status_t fbe_ps_mgmt_get_encl_ps_fup_info_by_location(fbe_ps_mgmt_t * pPsMgmt,
                                                       fbe_device_physical_location_t * pLocation,
                                                       fbe_base_env_fup_persistent_info_t **pPsFupInfo)
{
    fbe_status_t        status;
    fbe_encl_ps_info_t  * pEnclPsInfo = NULL;
    

    /* get the encl ps info. */
    status = fbe_ps_mgmt_get_encl_ps_info_by_location(pPsMgmt,
                                                      pLocation, 
                                                      &pEnclPsInfo);

    if (status != FBE_STATUS_OK)
    {
        // encl info not found
        fbe_base_object_trace((fbe_base_object_t *)pPsMgmt,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "FUP:PS(%d_%d_%d), %s, get_encl_ps_info_by_location failed.\n",
                              pLocation->bus,
                              pLocation->enclosure,
                              pLocation->slot,
                              __FUNCTION__);
        return status;
    }

    // save the address of the ps fup info
    *pPsFupInfo = &pEnclPsInfo->psFupInfo[pLocation->slot];

    return(FBE_STATUS_OK);
} //fbe_ps_mgmt_get_encl_ps_fup_info_by_location


/*!***************************************************************
 * fbe_ps_mgmt_find_available_encl_ps_info()
 ****************************************************************
 * @brief
 *  This function searches through the enclosure PS array and
 *  returns the pointer to the available encl ps info.
 *
 * @param  pPsMgmt - 
 * @param  pEnclPsInfoPtr(OUTPUT) - 
 *
 * @return fbe_status_t
 *
 * @author
 *  15-Aug-2010 PHE - Created. 
 *
 ****************************************************************/
static fbe_status_t fbe_ps_mgmt_find_available_encl_ps_info(fbe_ps_mgmt_t * pPsMgmt,
                                                     fbe_encl_ps_info_t ** pEnclPsInfoPtr)
{
    fbe_u8_t              enclIndex = 0;
    fbe_encl_ps_info_t  * pEnclPsInfo = NULL;
   
    *pEnclPsInfoPtr = NULL;
     
    for (enclIndex = 0; enclIndex < FBE_ESP_MAX_ENCL_COUNT; enclIndex++)
    {
        pEnclPsInfo = &(pPsMgmt->psInfo->enclPsInfo[enclIndex]);

        if(pEnclPsInfo->objectId == FBE_OBJECT_ID_INVALID)
        {
            // found enclosure
            pPsMgmt->enclCount++;
            pEnclPsInfo->inUse = TRUE;
            *pEnclPsInfoPtr = pEnclPsInfo;
            return FBE_STATUS_OK;
        }
    }

    return FBE_STATUS_COMPONENT_NOT_FOUND;
} //fbe_ps_mgmt_find_available_encl_ps_info


/*!**************************************************************
 * fbe_ps_mgmt_cmi_message_handler()
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
 *  15-Apr-2010:  Created. Joe Perry
 *  29-Apr-2011: PHE - Updated. 
 ****************************************************************/
fbe_status_t 
fbe_ps_mgmt_cmi_message_handler(fbe_base_object_t *pBaseObject, 
                                fbe_base_environment_cmi_message_info_t *pCmiMsg)
{
    fbe_ps_mgmt_t                      * pPsMgmt = (fbe_ps_mgmt_t *)pBaseObject;
    fbe_ps_mgmt_cmi_packet_t           * pPsCmiPkt = NULL;
    fbe_base_environment_cmi_message_t * pBaseCmiPkt = NULL;
    fbe_status_t                         status = FBE_STATUS_OK;

    // client message (if it exists)
    pPsCmiPkt = (fbe_ps_mgmt_cmi_packet_t *)pCmiMsg->user_message;
    pBaseCmiPkt = (fbe_base_environment_cmi_message_t *)pPsCmiPkt;

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
            status = fbe_ps_mgmt_processReceivedCmiMessage(pPsMgmt, pPsCmiPkt);
            break;

        case FBE_CMI_EVENT_PEER_NOT_PRESENT:
            status = fbe_ps_mgmt_processPeerNotPresent(pPsMgmt, pPsCmiPkt);
            break;

        case FBE_CMI_EVENT_SP_CONTACT_LOST:
            status = fbe_ps_mgmt_processContactLost(pPsMgmt);
            break;

        case FBE_CMI_EVENT_PEER_BUSY:
            status = fbe_ps_mgmt_processPeerBusy(pPsMgmt, pPsCmiPkt);
            break;

        case FBE_CMI_EVENT_FATAL_ERROR:
            status = fbe_ps_mgmt_processFatalError(pPsMgmt, pPsCmiPkt);
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

} //fbe_ps_mgmt_cmi_message_handler

/*!**************************************************************
 * fbe_ps_mgmt_discover_ps_cond_function()
 ****************************************************************
 * @brief
 *  This function gets Board Object ID (it interfaces with PS)
 *  and adds them to the queue to be processed later
 *
 * @param base_object - This is the object handle, or in our
 * case the ps_mgmt.
 * @param packet - The packet arriving from the monitor
 * scheduler.
 *
 * @return fbe_lifecycle_status_t - 
 *
 * @author
 *  15-Apr-2010:  Created. Joe Perry
 *  25-Aug-2010: PHE - combined the discovering for PE and DAE power supplies.
 *
 ****************************************************************/
static fbe_lifecycle_status_t 
fbe_ps_mgmt_discover_ps_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet)
{
    fbe_u32_t                                   psCount = 0;
    fbe_status_t                                status;
    fbe_ps_mgmt_t                               *ps_mgmt = (fbe_ps_mgmt_t *) base_object;
    fbe_object_id_t                             *object_id_list = NULL;
    fbe_u32_t                                   enclCount = 0;
    fbe_u32_t                                   enclIndex = 0;
    fbe_u32_t                                   slot = 0;
    fbe_device_physical_location_t              location = {0};

    fbe_base_object_trace(base_object, 
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n",
                          __FUNCTION__);

    /* Get the list of all the enclosures the system sees*/
    object_id_list = fbe_base_env_memory_ex_allocate((fbe_base_environment_t *)ps_mgmt, sizeof(fbe_object_id_t) * FBE_MAX_PHYSICAL_OBJECTS);
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
        fbe_base_env_memory_ex_release((fbe_base_environment_t *)ps_mgmt, object_id_list);
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /* The rest object ids in the list is for DAE enclosures.*/
    status = fbe_api_get_all_enclosure_object_ids(&object_id_list[1], FBE_MAX_PHYSICAL_OBJECTS - 1, &enclCount);

    if (status != FBE_STATUS_OK) 
    {
        fbe_base_object_trace(base_object, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s Error in getting all the enclosure objectIDs, status: 0x%X\n",
                              __FUNCTION__, status);

        fbe_base_env_memory_ex_release((fbe_base_environment_t *)ps_mgmt, object_id_list);
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    for(enclIndex = 0; enclIndex < enclCount + 1; enclIndex++) 
    {
        status = fbe_api_power_supply_get_ps_count(object_id_list[enclIndex], &psCount);
        fbe_base_object_trace(base_object, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s Encl:%d has PS Count %d\n",
                              __FUNCTION__, enclIndex, psCount);

        if (status != FBE_STATUS_OK) 
        {
            fbe_base_object_trace(base_object, 
                                  FBE_TRACE_LEVEL_WARNING,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s Error in getting ps count for object id %d, status: 0x%X\n",
                                  __FUNCTION__, object_id_list[enclIndex], status);

            continue;
        }
       
        for (slot = 0; slot < psCount; slot++)
        {
            fbe_zero_memory(&location, sizeof(fbe_device_physical_location_t));

            status = fbe_base_env_get_and_adjust_encl_location((fbe_base_environment_t *)ps_mgmt, 
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

            location.slot = slot;

            status = fbe_ps_mgmt_processPsStatus(ps_mgmt, object_id_list[enclIndex], &location);
            if (status != FBE_STATUS_OK) 
            {
                fbe_base_object_trace(base_object, 
                                      FBE_TRACE_LEVEL_WARNING,
                                      FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                      "%s PS(%d_%d_%d) Error in processing status, status 0x%X.\n",
                                      __FUNCTION__, 
                                      location.bus,
                                      location.enclosure,
                                      location.slot,
                                      status);

            }
        }

    }

    fbe_ps_mgmt_checkCacheStatus(ps_mgmt);

    status = fbe_lifecycle_clear_current_cond(base_object);
    if (status != FBE_STATUS_OK) 
    {
        fbe_base_object_trace(base_object, 
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s can't clear current condition, status: 0x%X",
                                __FUNCTION__, status);
    }

    fbe_base_env_memory_ex_release((fbe_base_environment_t *)ps_mgmt, object_id_list);
    return FBE_LIFECYCLE_STATUS_DONE;
}

/******************************************************
 * end fbe_ps_mgmt_discover_ps_cond_function() 
 ******************************************************/
   

/*!**************************************************************
 * fbe_ps_mgmt_processPsStatus()
 ****************************************************************
 * @brief
 *  This function processes the power supply status change.
 *
 * @param pPsMgmt - This is the object handle, or in our case the ps_mgmt.
 * @param objectId - The source object id. 
 *                   It is physical package enclosure object id for PS.
 * @param pLocation - The location of the PS.
 *
 * @return fbe_status_t - 
 *
 * @author
 *  15-Apr-2010:  Created. Joe Perry
 *  15-Aug-2010: PHE - Combined the processing for pePs and daePs.
 *
 ****************************************************************/
fbe_status_t fbe_ps_mgmt_processPsStatus(fbe_ps_mgmt_t * pPsMgmt,
                            fbe_object_id_t objectId,
                            fbe_device_physical_location_t * pLocation)
{
    fbe_base_object_t                     * base_object = (fbe_base_object_t *)pPsMgmt;
    fbe_encl_ps_info_t                    * pEnclPsInfo = NULL;
    fbe_power_supply_info_t                 newPsInfo = {0};
    fbe_power_supply_info_t                 oldPsInfo = {0};
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_u32_t                               psCount = 0;
    fbe_base_env_fup_persistent_info_t     * pPsFupInfo = NULL;
    fbe_object_id_t                         object_id;
    fbe_board_mgmt_platform_info_t          platform_info;
    fbe_bool_t                              eirSupported;
    HW_ENCLOSURE_TYPE                       processorEnclType;
    fbe_enclosure_types_t                   enclType;
    fbe_bool_t                              stopDebounceTimer = FBE_FALSE;


    if(objectId == FBE_OBJECT_ID_INVALID)
    {
        fbe_base_object_trace(base_object, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s, encl %d_%d, invalid object id.\n",
                              __FUNCTION__, pLocation->bus, pLocation->enclosure);
        return FBE_STATUS_GENERIC_FAILURE;
    }
   
    status = fbe_ps_mgmt_get_encl_ps_info_by_location(pPsMgmt, pLocation, &pEnclPsInfo);

    if (status != FBE_STATUS_OK) 
    {
        fbe_base_object_trace(base_object, 
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s, Populate encl ps info for encl %d_%d, status %d.\n",
                                  __FUNCTION__, pLocation->bus, pLocation->enclosure, status);

        status = fbe_ps_mgmt_find_available_encl_ps_info(pPsMgmt, &pEnclPsInfo);
 
        if (status != FBE_STATUS_OK) 
        {
            fbe_base_object_trace(base_object, 
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s, encl %d_%d, Not enough memory allocated for encl ps info, status: 0x%X.\n",
                                  __FUNCTION__, pLocation->bus, pLocation->enclosure, status);
            return status;
        }

        status = fbe_api_power_supply_get_ps_count(objectId, &psCount);

        if (status != FBE_STATUS_OK) 
        {
            fbe_base_object_trace(base_object, 
                                  FBE_TRACE_LEVEL_WARNING,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s encl %d_%d, failed to get ps count, object id %d, status: 0x%X.\n",
                                  __FUNCTION__, pLocation->bus, pLocation->enclosure, objectId, status);

            return FBE_LIFECYCLE_STATUS_DONE;
        }

        pEnclPsInfo->objectId = objectId;
        pEnclPsInfo->location = *pLocation;
        pEnclPsInfo->psCount = psCount;
    }
   
    fbe_zero_memory(&newPsInfo, sizeof(fbe_power_supply_info_t));
    newPsInfo.slotNumOnEncl = pLocation->slot;

    status = fbe_api_power_supply_get_power_supply_info(pEnclPsInfo->objectId, &newPsInfo);
    if (status != FBE_STATUS_OK) 
    {
        fbe_base_object_trace(base_object, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s Error getting PS info, status: 0x%X.\n",
                              __FUNCTION__, status);
        return status;
    }

    status = fbe_api_get_board_object_id(&object_id);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace(base_object,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s Error getting board object id, status: 0x%X.\n",
                              __FUNCTION__, status);
    }

    fbe_zero_memory(&platform_info, sizeof(fbe_board_mgmt_platform_info_t));
    status = fbe_api_board_get_platform_info(object_id, &platform_info);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace(base_object,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s Error getting platform info, status: 0x%X.\n",
                              __FUNCTION__, status);
    }

    fbe_base_environment_isPsSupportedByEncl((fbe_base_object_t *)pPsMgmt, 
                                             pLocation, 
                                             newPsInfo.uniqueId, 
                                             (newPsInfo.ACDCInput == DC_INPUT) ? TRUE : FALSE,
                                             &(newPsInfo.psSupported),
                                             &eirSupported);

// special processing to handle Viper with DC PS (Gen2 PS does not support CDES getting FFID)
    if (!newPsInfo.psSupported && strncmp(newPsInfo.subenclProductId, "EMC G2 PS", 9) == 0)
    {
        fbe_base_object_trace(base_object,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s, PS(%d_%d_%d) assumed to be Viper DC PS.\n",
                              __FUNCTION__, pLocation->bus, pLocation->enclosure, pLocation->slot);
        newPsInfo.psSupported = TRUE;
    }
// special processing to handle Viper with DC PS (Gen2 PS does not support CDES getting FFID)

    /*
     * For M4 and M5 Jetfire systems we need to support Octane power supplies.
     * Once a system is configured to run with an Octane power supply it will stay
     * that way.  Any other power supply detected will be marked unsupported.
     */

    status = fbe_base_env_get_processor_encl_type((fbe_base_environment_t *)pPsMgmt, &processorEnclType);
    
    if (status != FBE_STATUS_OK) 
    {
        fbe_base_object_trace(base_object,  
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s Error in getting peType, status: 0x%x.\n",
                              __FUNCTION__, status);
    }
    else
    {
        //Only check this value if we are an M4 or M5 Jetfire this is a DPE and the PS is inserted
        if( ((platform_info.hw_platform_info.platformType == SPID_DEFIANT_M4_HW_TYPE) ||
             (platform_info.hw_platform_info.platformType == SPID_DEFIANT_M5_HW_TYPE)) &&
             ((processorEnclType == DPE_ENCLOSURE_TYPE) &&
              (pLocation->bus == 0) && (pLocation->enclosure == 0) &&
              (newPsInfo.bInserted == FBE_TRUE)) )
            {
           
            // If nothing has been written to disk, do it now (this should happen during ICA)
            if(pPsMgmt->expected_power_supply == FBE_PS_MGMT_EXPECTED_PS_TYPE_NONE)
            {
                if(newPsInfo.uniqueId == AC_ACBEL_OCTANE)
                {
                    fbe_base_object_trace(base_object, 
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "PS_MGMT: Octane PS detected, setting this to the expected type\n");
                    pPsMgmt->expected_power_supply = FBE_PS_MGMT_EXPECTED_PS_TYPE_OCTANE;
                }
                else
                {
                    fbe_base_object_trace(base_object, 
                                          FBE_TRACE_LEVEL_INFO,
                                          FBE_TRACE_MESSAGE_ID_INFO,
                                          "PS_MGMT: NonOctane PS detected, UID: %s setting expected type other\n",
                                          decodeFamilyFruID(newPsInfo.uniqueId));
                    pPsMgmt->expected_power_supply = FBE_PS_MGMT_EXPECTED_PS_TYPE_OTHER;
                }
                // copy the new information to the persistent storage in memory copy and kick off a write.
                fbe_copy_memory(((fbe_base_environment_t *)base_object)->my_persistent_data, 
                                &pPsMgmt->expected_power_supply,
                                sizeof(fbe_ps_mgmt_expected_ps_type_t));
            
                fbe_base_object_trace(base_object, 
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "PS_MGMT: writing expected_power_supply:%d to disk\n",
                          pPsMgmt->expected_power_supply);
            
                status = fbe_base_env_write_persistent_data((fbe_base_environment_t *)pPsMgmt);
                if(status != FBE_STATUS_OK)
                {
                    fbe_base_object_trace(base_object, 
                                          FBE_TRACE_LEVEL_ERROR,
                                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                          "%s persistent write error, status: 0x%X",
                                          __FUNCTION__, status);
                }
            }
            else if(pPsMgmt->expected_power_supply == FBE_PS_MGMT_EXPECTED_PS_TYPE_OCTANE)
            {
                // only octane power supplies are supported for this platform
                if(newPsInfo.uniqueId != AC_ACBEL_OCTANE)
                {
                    newPsInfo.psSupported = FALSE;
                    fbe_base_object_trace(base_object, 
                                          FBE_TRACE_LEVEL_WARNING,
                                          FBE_TRACE_MESSAGE_ID_INFO,
                                          "PS_MGMT: Detected Non-Octane Power Supply for system configured with Octane PS\n");
                }
            
            
            }
            else if(pPsMgmt->expected_power_supply == FBE_PS_MGMT_EXPECTED_PS_TYPE_OTHER)
            {
                // only non-octane power supplies are supported for this platform
                if(newPsInfo.uniqueId == AC_ACBEL_OCTANE)
                {
                    newPsInfo.psSupported = FALSE;
                    fbe_base_object_trace(base_object, 
                                          FBE_TRACE_LEVEL_WARNING,
                                          FBE_TRACE_MESSAGE_ID_INFO,
                                          "PS_MGMT: Detected Octane Power Supply for system configured with Non-Octane PS\n");
                }
            }
        }
    }

    // special case for Pinecone and Ancho PS's (they don't report PS fault for removed power, just AcFail)
    if (newPsInfo.ACFail)
    {
        status = fbe_api_enclosure_get_encl_type(pEnclPsInfo->objectId, &enclType);
        if(status == FBE_STATUS_OK)
        {
            if ((enclType == FBE_ENCL_SAS_PINECONE) ||
                 (enclType == FBE_ENCL_SAS_ANCHO))
            {
                newPsInfo.generalFault = FBE_MGMT_STATUS_TRUE;
            }
        }
    }
    // special case for Pinecone and Ancho PS's (they don't report PS fault for removed power, just AcFail)


    fbe_base_object_trace(base_object, 
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s(%d_%d), PS %d: inserted %d, psFault %d, acFail %d, OT %d.\n",
                              __FUNCTION__, 
                          pLocation->bus,
                          pLocation->enclosure,
                          pLocation->slot,
                          newPsInfo.bInserted, 
                          newPsInfo.generalFault,
                          newPsInfo.ACFail,
                          newPsInfo.ambientOvertempFault);
    fbe_base_object_trace(base_object, 
                          FBE_TRACE_LEVEL_DEBUG_LOW,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s(%d_%d), PS %d: Margin mode %d, results %d\n",
                              __FUNCTION__, 
                          pLocation->bus,
                          pLocation->enclosure,
                          pLocation->slot,
                          newPsInfo.psMarginTestMode, 
                          newPsInfo.psMarginTestResults);
   
    /* Save the old ps info. */
    oldPsInfo = pEnclPsInfo->psInfo[pLocation->slot];
    // checks to suppress ps fault
    if( ((newPsInfo.generalFault) && (!oldPsInfo.generalFault)) ||
        ( newPsInfo.internalFanFault && !oldPsInfo.internalFanFault ))
    {
        // Suppress PS Fault during FUP Activate
        status = fbe_ps_mgmt_get_fup_info_ptr(pPsMgmt, FBE_DEVICE_TYPE_PS, pLocation, &pPsFupInfo);
        if (status == FBE_STATUS_OK)
        {
            if (pPsFupInfo->pWorkItem != NULL) 
            {
                // ignore fault when doing fup
                if ((pPsFupInfo->pWorkItem->firmwareStatus == FBE_ENCLOSURE_FW_STATUS_IN_PROGRESS) &&
                    (pPsFupInfo->pWorkItem->workState == FBE_BASE_ENV_FUP_WORK_STATE_ACTIVATE_IMAGE_SENT))
                {
                    // Suppress PS Faults during PS FUP Activate
                    newPsInfo.generalFault = FALSE;
                    newPsInfo.internalFanFault = FALSE;
                    fbe_base_object_trace((fbe_base_object_t *)pPsMgmt,
                                          FBE_TRACE_LEVEL_INFO,
                                          FBE_TRACE_MESSAGE_ID_INFO,
                                          "FUP:PS(%d_%d_%d),%s, FUP Suppress Fault, image activation in progress\n",
                                          pLocation->bus,
                                          pLocation->enclosure,
                                          pLocation->slot,
                                          __FUNCTION__);
                }
            }
            // ignore faults when peer is activating image
            if (pPsFupInfo->suppressFltDueToPeerFup)
            {
                // filter ps faults during peer ps fup
                newPsInfo.generalFault = FALSE;
                newPsInfo.internalFanFault = FALSE;
                fbe_base_object_trace((fbe_base_object_t *)pPsMgmt,
                                          FBE_TRACE_LEVEL_INFO,
                                          FBE_TRACE_MESSAGE_ID_INFO,
                                          "FUP:PS(%d_%d_%d),%s, Peer FUP Suppress Fault, peer image activation in progress\n",
                                          pLocation->bus,
                                          pLocation->enclosure,
                                          pLocation->slot,
                                          __FUNCTION__);
            }
        }

        // debounce PS faults 
        // the timer could still be running or it may have expired but hasn't been stopped yet
        if (pEnclPsInfo->psDebounceInfo[pLocation->slot].debounceTimerActive)
        {
            if(pEnclPsInfo->psDebounceInfo[pLocation->slot].debouncePeriodExpired)
            {
                if(pEnclPsInfo->psDebounceInfo[pLocation->slot].faultMasked)
                {
                    pEnclPsInfo->psDebounceInfo[pLocation->slot].faultMasked = FBE_FALSE;
                    fbe_base_object_trace(base_object, 
                                          FBE_TRACE_LEVEL_INFO,
                                          FBE_TRACE_MESSAGE_ID_INFO,
                                          "%s SUPPRESS PS FAULT(%d_%d_%d)Timer expired, stop ignoring the fault\n",
                                          __FUNCTION__,
                                          pLocation->bus,
                                          pLocation->enclosure,
                                          pLocation->slot);
                }
                stopDebounceTimer = FBE_TRUE;
            }
            else
            {
                pEnclPsInfo->psDebounceInfo[pLocation->slot].faultMasked = FBE_TRUE;
                // ktrace ignore fault
                fbe_base_object_trace(base_object, 
                                      FBE_TRACE_LEVEL_INFO,
                                      FBE_TRACE_MESSAGE_ID_INFO,
                                      "%s SUPPRESS PS FAULT(%d_%d_%d) Ignore the fault\n",
                                      __FUNCTION__,
                                      pLocation->bus,
                                      pLocation->enclosure,
                                      pLocation->slot);
            }
        }
        else
        {
            // else there is a new fault and the timer is not running
            // start the timer
            status = fbe_lifecycle_force_clear_cond(&FBE_LIFECYCLE_CONST_DATA(ps_mgmt), 
                                                    base_object, 
                                                    FBE_PS_MGMT_DEBOUNCE_TIMER_COND);
            if (status != FBE_STATUS_OK) 
            {
                fbe_base_object_trace(base_object, 
                                      FBE_TRACE_LEVEL_ERROR,
                                      FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                      "%s can't clear debounce timer condition, status: 0x%X\n",
                                      __FUNCTION__, status);
                return FBE_LIFECYCLE_STATUS_DONE;
            }
            else
            {
                fbe_base_object_trace(base_object, 
                                      FBE_TRACE_LEVEL_INFO,
                                      FBE_TRACE_MESSAGE_ID_INFO,
                                      "%s, debounce start fault suppression PS %d_%d_%d\n",
                                      __FUNCTION__, 
                                      pLocation->bus, pLocation->enclosure, pLocation->slot);
            }
            pEnclPsInfo->psDebounceInfo[pLocation->slot].faultMasked = FBE_TRUE;
            pEnclPsInfo->psDebounceInfo[pLocation->slot].debouncePeriodExpired = FBE_FALSE;
            pEnclPsInfo->psDebounceInfo[pLocation->slot].debounceTimerActive = FBE_TRUE;
            fbe_base_object_trace(base_object, 
                                      FBE_TRACE_LEVEL_INFO,
                                      FBE_TRACE_MESSAGE_ID_INFO,
                                      "%s SUPPRESS PS FAULT(%d_%d_%d) Start the timer, Ignore the fault\n",
                                      __FUNCTION__,
                                      pLocation->bus,
                                      pLocation->enclosure,
                                      pLocation->slot);
        }
    }
    else
    {
        // there is no new fault
        stopDebounceTimer = FBE_TRUE;
    }

    if (stopDebounceTimer)
    {
        pEnclPsInfo->psDebounceInfo[pLocation->slot].faultMasked = FBE_FALSE;
        // stop
        if(pEnclPsInfo->psDebounceInfo[pLocation->slot].debounceTimerActive ||
           pEnclPsInfo->psDebounceInfo[pLocation->slot].debouncePeriodExpired)
        {
            status = fbe_lifecycle_stop_timer(&FBE_LIFECYCLE_CONST_DATA(ps_mgmt),
                                              base_object,
                                              FBE_PS_MGMT_DEBOUNCE_TIMER_COND);
            if (status == FBE_STATUS_OK)
            {
                fbe_base_object_trace(base_object, 
                                      FBE_TRACE_LEVEL_INFO,
                                      FBE_TRACE_MESSAGE_ID_INFO,
                                      "%s, PS DEBOUNCE TIMER has been stopped\n",
                                      __FUNCTION__);
                pEnclPsInfo->psDebounceInfo[pLocation->slot].debounceTimerActive = FBE_FALSE;
                pEnclPsInfo->psDebounceInfo[pLocation->slot].debouncePeriodExpired = FBE_FALSE;
                pEnclPsInfo->psDebounceInfo[pLocation->slot].faultMasked = FBE_FALSE;
            }
            else
            {
                fbe_base_object_trace(base_object, 
                                      FBE_TRACE_LEVEL_ERROR,
                                      FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                      "%s, error stopping timer, status 0x%x\n",
                                      __FUNCTION__, status);
            }
        }
        else
        {
            fbe_base_object_trace(base_object, 
                                      FBE_TRACE_LEVEL_INFO,///////change to debug FBE_TRACE_LEVEL_DEBUG_LOW
                                      FBE_TRACE_MESSAGE_ID_INFO,
                                      "%s, PS DEBOUNCE TIMER is not running\n",
                                      __FUNCTION__);
                pEnclPsInfo->psDebounceInfo[pLocation->slot].debounceTimerActive = FBE_FALSE;
                pEnclPsInfo->psDebounceInfo[pLocation->slot].debouncePeriodExpired = FBE_FALSE;
                pEnclPsInfo->psDebounceInfo[pLocation->slot].faultMasked = FBE_FALSE;
        }
    }
    // suppress the fault as determined above
    if(pEnclPsInfo->psDebounceInfo[pLocation->slot].faultMasked)
    {
        newPsInfo.generalFault = oldPsInfo.generalFault;
        newPsInfo.internalFanFault = oldPsInfo.internalFanFault;
    }

    /* Copy over the new ps info to the fbe_ps_mgmt_t date structure. */
    pEnclPsInfo->psInfo[pLocation->slot] = newPsInfo;

    status = fbe_ps_mgmt_checkForPsEvents(pPsMgmt, pLocation, &newPsInfo, &oldPsInfo);
    if (status != FBE_STATUS_OK) 
    {
        fbe_base_object_trace(base_object, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s Error checking for PS events, status: 0x%X.\n",
                              __FUNCTION__, status);

    }

    status = fbe_ps_mgmt_fup_handle_ps_status_change(pPsMgmt, pLocation, &newPsInfo, &oldPsInfo, pEnclPsInfo->psDebounceInfo[pLocation->slot].faultMasked);
    if(status != FBE_STATUS_OK) 
    {
        fbe_base_object_trace(base_object, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "FUP:PS(%d_%d_%d) fup_handle_ps_status_change failed, status 0x%X.\n",
                              pLocation->bus,
                              pLocation->enclosure,
                              pLocation->slot,
                              status);
    }

    status = fbe_ps_mgmt_resume_prom_handle_ps_status_change(pPsMgmt, pLocation, &newPsInfo, &oldPsInfo);
    if(status != FBE_STATUS_OK) 
    {
        fbe_base_object_trace(base_object, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "RP: PS(%d_%d_%d) handle_state_change failed, status 0x%X.\n",
                              pLocation->bus,
                              pLocation->enclosure,
                              pLocation->slot,
                              status);

    }

    fbe_ps_mgmt_calculatePsStateInfo(pPsMgmt, pLocation);
    
    /*
     * Update PS Cache status since something has changed
     */
    fbe_ps_mgmt_checkCacheStatus(pPsMgmt);
    
    /*
     * Check if Enclosure Fault LED needs updating
     */
    status = fbe_ps_mgmt_update_encl_fault_led(pPsMgmt, pLocation, FBE_ENCL_FAULT_LED_PS_FLT);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace(base_object, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "PS(%d_%d_%d), error updating EnclFaultLed, status 0x%X.\n",
                              pLocation->bus,
                              pLocation->enclosure,
                              pLocation->slot,
                              status);
    }

    /* Send data change notification */
    fbe_base_environment_send_data_change_notification((fbe_base_environment_t *)pPsMgmt, 
                                                       FBE_CLASS_ID_PS_MGMT, 
                                                       FBE_DEVICE_TYPE_PS, 
                                                       FBE_DEVICE_DATA_TYPE_GENERIC_INFO,
                                                       pLocation);    

    return status;
}
/******************************************************
 * end fbe_ps_mgmt_processPsStatus() 
 ******************************************************/


/*!**************************************************************
 * fbe_ps_mgmt_checkForPsEvents()
 ****************************************************************
 * @brief
 *  This function gets Enclosure Object ID (it interfaces with PS)
 *  and adds them to the queue to be processed later
 *
 * @param pPsMgmt -
 * @param pLocation - The location of the PS.
 * @param pNewPsInfo - The new power supply info.
 * @param pOldPsInfo - The old power supply info.
 *
 * @return fbe_status_t - The status of the call to
 *                        fbe_lifecycle_class_const_verify()
 *                        for the PS management's
 *                        constant lifecycle data.
 *
 * @author
 *  15-Apr-2010:  Created. Joe Perry
 *  15-Aug-2010: PHE - added the input parameter pNewPsInfo and pOldPsInfo.
 *  31-Dec-2011: Chang Rui -  modify fault reporting logic.
 *
 ****************************************************************/
static fbe_status_t 
fbe_ps_mgmt_checkForPsEvents(fbe_ps_mgmt_t *ps_mgmt,
                             fbe_device_physical_location_t * location,
                             fbe_power_supply_info_t * pNewPsInfo,
                             fbe_power_supply_info_t * pOldPsInfo)
{
    fbe_base_object_t           * base_object = (fbe_base_object_t *)ps_mgmt;
    fbe_status_t                status = FBE_STATUS_OK;
    fbe_u8_t                    deviceStr[FBE_DEVICE_STRING_LENGTH]={0};
    fbe_u8_t                    psFaultString[FBE_EVENT_LOG_MESSAGE_STRING_LENGTH]={0};
    fbe_bool_t                  newFault=FBE_FALSE; 
    fbe_bool_t                  oldFault=FBE_FALSE; 

    status = fbe_base_env_create_device_string(FBE_DEVICE_TYPE_PS, 
                                               location, 
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

    if (pNewPsInfo->envInterfaceStatus != pOldPsInfo->envInterfaceStatus)
    {
        if (pNewPsInfo->envInterfaceStatus == FBE_ENV_INTERFACE_STATUS_XACTION_FAIL)
        {
            fbe_event_log_write(ESP_ERROR_ENV_INTERFACE_FAILURE_DETECTED,
                                NULL, 0,
                                "%s %s", 
                                &deviceStr[0],
                                SmbTranslateErrorString(pNewPsInfo->transactionStatus));
        }
        else if (pNewPsInfo->envInterfaceStatus != FBE_ENV_INTERFACE_STATUS_GOOD)
        {
            fbe_event_log_write(ESP_ERROR_ENV_INTERFACE_FAILURE_DETECTED,
                                NULL, 0,
                                "%s %s", 
                                &deviceStr[0],
                                fbe_base_env_decode_env_status(pNewPsInfo->envInterfaceStatus));
        }
        else
        {
            fbe_event_log_write(ESP_INFO_ENV_INTERFACE_FAILURE_CLEARED,
                                NULL, 0,
                                "%s", 
                                &deviceStr[0]);
        }
    }

    // check for PS removed, inserted, faulted or fault cleared
    if (!pOldPsInfo->bInserted && pNewPsInfo->bInserted)
    {
        fbe_event_log_write(ESP_INFO_PS_INSERTED,
                    NULL, 0,
                    "%s", 
                    &deviceStr[0]);

        newFault = fbe_ps_mgmt_get_ps_logical_fault(pNewPsInfo);

        if(newFault && !pNewPsInfo->isFaultRegFail) 
        {
            fbe_ps_mgmt_create_ps_fault_string(pNewPsInfo, &psFaultString[0]); 

            fbe_event_log_write(ESP_ERROR_PS_FAULTED,
                            NULL, 0,
                            "%s %s", 
                            &deviceStr[0], 
                            &psFaultString[0]);

        }

        if(pNewPsInfo->ambientOvertempFault == FBE_MGMT_STATUS_TRUE)
        {
            fbe_event_log_write(ESP_ERROR_PS_OVER_TEMP_DETECTED,
                                NULL, 0,
                                "%s", 
                                &deviceStr[0]);
        }
    }
    else if (pOldPsInfo->bInserted && !pNewPsInfo->bInserted)
    {
        fbe_event_log_write(ESP_ERROR_PS_REMOVED,
                            NULL, 0,
                            "%s", 
                            &deviceStr[0]);
    }
    else if(pOldPsInfo->bInserted && pNewPsInfo->bInserted) 
    {
        if((pOldPsInfo->ambientOvertempFault == FBE_MGMT_STATUS_FALSE) &&
           (pNewPsInfo->ambientOvertempFault == FBE_MGMT_STATUS_TRUE)) 
        {
            fbe_event_log_write(ESP_ERROR_PS_OVER_TEMP_DETECTED,
                                NULL, 0,
                                "%s", 
                                &deviceStr[0]);
        }
        else if((pOldPsInfo->ambientOvertempFault == FBE_MGMT_STATUS_TRUE) &&
                (pNewPsInfo->ambientOvertempFault == FBE_MGMT_STATUS_FALSE))
        {
            fbe_event_log_write(ESP_INFO_PS_OVER_TEMP_CLEARED,
                                NULL, 0,
                                "%s", 
                                &deviceStr[0]);
        }

        oldFault = fbe_ps_mgmt_get_ps_logical_fault(pOldPsInfo);
        newFault = fbe_ps_mgmt_get_ps_logical_fault(pNewPsInfo);
        if(!oldFault && newFault && !pNewPsInfo->isFaultRegFail) 
        {
            fbe_ps_mgmt_create_ps_fault_string(pNewPsInfo, &psFaultString[0]);

            fbe_event_log_write(ESP_ERROR_PS_FAULTED,
                            NULL, 0,
                            "%s %s", 
                            &deviceStr[0], 
                            &psFaultString[0]);
        }
        else if(oldFault && !newFault && !pOldPsInfo->isFaultRegFail)
        {
            fbe_event_log_write(ESP_INFO_PS_FAULT_CLEARED,
                            NULL, 0,
                            "%s", 
                            &deviceStr[0]);
        }
    }

    // check for any PS Margining Test failures
    if ((pNewPsInfo->psMarginTestMode == FBE_ESES_MARGINING_TEST_MODE_STATUS_TEST_SUCCESSFUL) &&
        (pOldPsInfo->psMarginTestResults == FBE_ESES_MARGINING_TEST_RESULTS_SUCCESS) &&
        (pNewPsInfo->psMarginTestResults != FBE_ESES_MARGINING_TEST_RESULTS_SUCCESS))
    {
        if (pNewPsInfo->psMarginTestResults == FBE_ESES_MARGINING_TEST_RESULTS_12V_1_SHORTED_TO_12_V2)
        {
            fbe_event_log_write(ESP_ERROR_PS_FAULTED,
                                NULL, 0,
                                "%s %s", 
                                &deviceStr[0], 
                                "PsMarginTestFailed - check PS's & Midplane");
        }
        else
        {
            fbe_event_log_write(ESP_ERROR_PS_FAULTED,
                                NULL, 0,
                                "%s %s", 
                                &deviceStr[0], 
                                "PsMarginTestFailed - replace PS");
        }
    }
    return status;
}

/*!**************************************************************
 * @fn fbe_ps_mgmt_processReceivedCmiMessage(fbe_ps_mgmt_t * pPsMgmt, 
 *                                            fbe_ps_mgmt_cmi_packet_t *pPsCmiPkt)
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
fbe_ps_mgmt_processReceivedCmiMessage(fbe_ps_mgmt_t * pPsMgmt, 
                                      fbe_ps_mgmt_cmi_packet_t *pPsCmiPkt)
{
    fbe_base_environment_t             * pBaseEnv = (fbe_base_environment_t *)pPsMgmt;
    fbe_base_environment_cmi_message_t * pBaseCmiMsg = (fbe_base_environment_cmi_message_t *)pPsCmiPkt;
    fbe_base_env_fup_cmi_packet_t      * pFupCmiPkt = (fbe_base_env_fup_cmi_packet_t *)pPsCmiPkt;
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
        status = fbe_ps_mgmt_process_peer_cache_status_update(pPsMgmt, 
                                                              pPsCmiPkt);
        break;

        case FBE_BASE_ENV_CMI_MSG_PEER_SP_ALIVE:
            // check if our earlier perm request failed
            status = fbe_ps_mgmt_fup_new_contact_init_upgrade(pPsMgmt);
            break;

        default:
            status = FBE_STATUS_MORE_PROCESSING_REQUIRED;
            break;
    }

    return(status);

} //fbe_ps_mgmt_processReceivedCmiMessage


/*!**************************************************************
 *  @fn fbe_ps_mgmt_processPeerNotPresent(fbe_ps_mgmt_t * pPsMgmt, 
 *                                        fbe_ps_mgmt_cmi_packet_t *pPsCmiPkt)
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
fbe_ps_mgmt_processPeerNotPresent(fbe_ps_mgmt_t * pPsMgmt, fbe_ps_mgmt_cmi_packet_t *pPsCmiPkt)
{
    fbe_base_environment_cmi_message_t * pBaseCmiMsg = (fbe_base_environment_cmi_message_t *)pPsCmiPkt;
    fbe_base_env_fup_cmi_packet_t      * pFupCmiPkt = (fbe_base_env_fup_cmi_packet_t *)pPsCmiPkt;
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
            status = fbe_base_env_fup_processGotPeerPerm((fbe_base_environment_t *)pPsMgmt,
                                                         pFupCmiPkt->pRequestorWorkItem);
     
            break;

        default:
            status = FBE_STATUS_MORE_PROCESSING_REQUIRED;
            break;
    }

    return(status);
}

/*!**************************************************************
 * @fn fbe_ps_mgmt_processContactLost(fbe_ps_mgmt_t * pPsMgmt)
 ****************************************************************
 * @brief
 *  This function gets called when recieving the CMI message with 
 *  FBE_CMI_EVENT_SP_CONTACT_LOST.
 *
 * @param pPsMgmt - 
 *
 * @return fbe_status_t - always return
 *   FBE_STATUS_MORE_PROCESSING_REQUIRED - need to further handled by the base environment.
 * 
 * @author
 *  27-Apr-2011: PHE - Created.
 ****************************************************************/
static fbe_status_t fbe_ps_mgmt_processContactLost(fbe_ps_mgmt_t * pPsMgmt)
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
fbe_ps_mgmt_processPeerBusy(fbe_ps_mgmt_t * pPsMgmt, 
                            fbe_ps_mgmt_cmi_packet_t * pPsCmiPkt)
{
    fbe_base_environment_cmi_message_t * pBaseCmiMsg = (fbe_base_environment_cmi_message_t *)pPsCmiPkt;
    fbe_base_env_fup_cmi_packet_t      * pFupCmiPkt = (fbe_base_env_fup_cmi_packet_t *)pPsCmiPkt;
    fbe_status_t                         status = FBE_STATUS_OK;

    switch (pBaseCmiMsg->opcode)
    {
        case FBE_BASE_ENV_CMI_MSG_FUP_PEER_PERMISSION_GRANT:
        case FBE_BASE_ENV_CMI_MSG_FUP_PEER_PERMISSION_DENY:
            fbe_base_object_trace((fbe_base_object_t *)pPsMgmt, 
                          FBE_TRACE_LEVEL_ERROR,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s, should never happen, opcode 0x%llx\n",
                          __FUNCTION__,
                          (unsigned long long)pBaseCmiMsg->opcode);

            status = FBE_STATUS_GENERIC_FAILURE;
            break;

        case FBE_BASE_ENV_CMI_MSG_FUP_PEER_PERMISSION_REQUEST:
            // peer SP is not present when sending the message to request permission.
            status = fbe_base_env_fup_peerPermRetry((fbe_base_environment_t *)pPsMgmt, 
                                                    pFupCmiPkt->pRequestorWorkItem);
            break;

        default:
            status = FBE_STATUS_MORE_PROCESSING_REQUIRED;
            break;
    }

    return(status);
}

/*!**************************************************************
 * @fn fbe_ps_mgmt_processFatalError(fbe_ps_mgmt_t * pPsMgmt, 
 *                                   fbe_ps_mgmt_cmi_packet_t * pPsCmiPkt)
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
fbe_ps_mgmt_processFatalError(fbe_ps_mgmt_t * pPsMgmt, 
                              fbe_ps_mgmt_cmi_packet_t * pPsCmiPkt)
{
    fbe_base_environment_cmi_message_t * pBaseCmiMsg = (fbe_base_environment_cmi_message_t *)pPsCmiPkt;
    fbe_base_env_fup_cmi_packet_t      * pFupCmiPkt = (fbe_base_env_fup_cmi_packet_t *)pPsCmiPkt;
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
            status = fbe_base_env_fup_processDeniedPeerPerm((fbe_base_environment_t *)pPsMgmt,
                                                         pFupCmiPkt->pRequestorWorkItem);
            break;

        default:
            status = FBE_STATUS_MORE_PROCESSING_REQUIRED;
            break;
    }

    return(status);
}

/*!**************************************************************
 * fbe_ps_mgmt_get_ps_logical_fault()
 ****************************************************************
 * @brief
 *  This function considers all the fields for the power supply
 *  and decides whether the power supply is faulted or not.
 *
 * @param pPsInfo -The pointer to fbe_power_supply_info_t.
 *
 * @return fbe_bool_t - The power supply is faulted or not logically.
 *
 * @author
 *  07-Apr-2011: PHE - Created.
 *
 ****************************************************************/
fbe_bool_t 
fbe_ps_mgmt_get_ps_logical_fault(fbe_power_supply_info_t *pPsInfo)
{
    fbe_bool_t psLogicalFault = FALSE;

    if(!pPsInfo->bInserted ||
       pPsInfo->generalFault == FBE_MGMT_STATUS_TRUE ||
       pPsInfo->internalFanFault == FBE_MGMT_STATUS_TRUE ||
       (!pPsInfo->psSupported && pPsInfo->uniqueIDFinal)  ||
       pPsInfo->isFaultRegFail == FBE_TRUE)
    {
        psLogicalFault = TRUE;
    }
    else if(pPsInfo->generalFault == FBE_MGMT_STATUS_UNKNOWN)
    {
        psLogicalFault = TRUE;
    }

    return psLogicalFault;
}

/*!**************************************************************
 * fbe_ps_mgmt_create_ps_fault_string()
 ****************************************************************
 * @brief
 *  This function formats PS fault string.
 *
 * @param pPsInfo - The pointer to fbe_power_supply_info_t.
 * @param string - The pointer to the array that will reserve the string.
 *
 * @return fbe_status_t - result of formatting the string.
 *
 * @author
 *  31-Dec-2011: Chang Rui - Created.
 *
 ****************************************************************/
static fbe_status_t 
fbe_ps_mgmt_create_ps_fault_string(fbe_power_supply_info_t *pPsInfo, char* string)
{
    if(pPsInfo->generalFault == FBE_MGMT_STATUS_TRUE)
    {
        if(pPsInfo->ACFail == FBE_MGMT_STATUS_TRUE)
        {
            strncat(string, "GenFlt/AcFail ", FBE_EVENT_LOG_MESSAGE_STRING_LENGTH-strlen(string)-1);
        }
        else
        {
            strncat(string, "GenFlt ", FBE_EVENT_LOG_MESSAGE_STRING_LENGTH-strlen(string)-1);
        }
    }
    if(pPsInfo->internalFanFault == FBE_MGMT_STATUS_TRUE)
    {
        strncat(string, "InternalFanFlt ", FBE_EVENT_LOG_MESSAGE_STRING_LENGTH-strlen(string)-1);
    }
    if(!pPsInfo->psSupported)
    {
        strncat(string, "Unsupported ", FBE_EVENT_LOG_MESSAGE_STRING_LENGTH-strlen(string)-1);
    }
    if(pPsInfo->isFaultRegFail == FBE_TRUE)
    {
        strncat(string, "FaultReg Fault ", FBE_EVENT_LOG_MESSAGE_STRING_LENGTH-strlen(string)-1);
    }
    if(pPsInfo->generalFault == FBE_MGMT_STATUS_UNKNOWN)
    {
        strncat(string, "UnknownFlt ", FBE_EVENT_LOG_MESSAGE_STRING_LENGTH-strlen(string)-1);
    }
    return FBE_STATUS_OK;
}

/*!**************************************************************
 * @fn fbe_ps_mgmt_update_encl_fault_led(fbe_encl_mgmt_t *pEnclMgmt,
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
 *  10-Feb-2011:  Created. Joe Perry
 *  14-Dec-2011:  PHE - Expanded to handle PS resume prom fault. 
 *
 ****************************************************************/
fbe_status_t fbe_ps_mgmt_update_encl_fault_led(fbe_ps_mgmt_t *pPsMgmt,
                                    fbe_device_physical_location_t *pLocation,
                                    fbe_encl_fault_led_reason_t enclFaultLedReason)
{
    fbe_status_t                                status = FBE_STATUS_OK;
    fbe_encl_ps_info_t                          *pEnclPsInfo = NULL;
    fbe_power_supply_info_t                     *pPsInfo = NULL;
    fbe_base_env_resume_prom_info_t             *pPsResumeInfo = NULL;
    fbe_u32_t                                   index = 0;
    fbe_bool_t                                  enclFaultLedOn = FBE_FALSE;
    fbe_bool_t                                  fault = FBE_FALSE;

    status = fbe_ps_mgmt_get_encl_ps_info_by_location(pPsMgmt, pLocation, &pEnclPsInfo);
    if (status != FBE_STATUS_OK) 
    {
        fbe_base_object_trace((fbe_base_object_t *)pPsMgmt, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s, encl %d_%d, get_encl_info_by_location failed, status: 0x%X.\n",
                              __FUNCTION__, pLocation->bus, pLocation->enclosure, status);
        return status;
    }

    switch(enclFaultLedReason) 
    {
    case FBE_ENCL_FAULT_LED_PS_FLT:
        for(index = 0; index < pEnclPsInfo->psCount; index ++) 
        {
            pPsInfo = &pEnclPsInfo->psInfo[index];
    
            fault = fbe_ps_mgmt_get_ps_logical_fault(pPsInfo);
    
            if(fault) 
            {
                enclFaultLedOn = TRUE;
                break;
            }
        }
        break;

    case FBE_ENCL_FAULT_LED_PS_RESUME_PROM_FLT:
        for(index = 0; index < pEnclPsInfo->psCount; index ++) 
        {
            pPsResumeInfo = &pEnclPsInfo->psResumeInfo[index];
    
            fault = fbe_base_env_get_resume_prom_fault(pPsResumeInfo->op_status);
    
            if(fault) 
            {
                enclFaultLedOn = FBE_TRUE;
                break;
            }
        }
        break;
   
    default:
        fbe_base_object_trace((fbe_base_object_t *)pPsMgmt, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "ENCL(%d_%d), unsupported enclFaultLedReason 0x%llX.\n",
                              pLocation->bus,
                              pLocation->enclosure,
                              enclFaultLedReason);
        return FBE_STATUS_GENERIC_FAILURE;
        break;
    }

    status = fbe_base_environment_updateEnclFaultLed((fbe_base_object_t *)pPsMgmt,
                                                     pEnclPsInfo->objectId,
                                                     enclFaultLedOn,
                                                     enclFaultLedReason);
   
    if (status == FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)pPsMgmt, 
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
        fbe_base_object_trace((fbe_base_object_t *)pPsMgmt, 
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
 * fbe_ps_mgmt_register_resume_prom_callback_cond_function()
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
 *                        the Encl management's constant
 *                        lifecycle data.
 *
 * @author
 *  03-Nov-2010: Arun S - Created. 
 *
 ****************************************************************/
static fbe_lifecycle_status_t
fbe_ps_mgmt_register_resume_prom_callback_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_base_environment_t * pBaseEnv = (fbe_base_environment_t *)base_object;

    fbe_base_object_trace(base_object, 
                          FBE_TRACE_LEVEL_DEBUG_HIGH,  
                          FBE_TRACE_MESSAGE_ID_INFO, 
                          "%s entry\n", 
                          __FUNCTION__);

    fbe_base_env_initiate_resume_prom_read_callback(pBaseEnv, &fbe_ps_mgmt_initiate_resume_prom_read);
    fbe_base_env_get_resume_prom_info_ptr_callback(pBaseEnv, (fbe_base_env_get_resume_prom_info_ptr_func_callback_ptr_t)&fbe_ps_mgmt_get_resume_prom_info_ptr);
    fbe_base_env_resume_prom_update_encl_fault_led_callback(pBaseEnv, (fbe_base_env_resume_prom_update_encl_fault_led_callback_func_ptr_t) &fbe_ps_mgmt_resume_prom_update_encl_fault_led);

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


/*!**************************************************************
 * fbe_ps_mgmt_checkCacheStatus()
 ****************************************************************
 * @brief
 *  This function checks the PS Mgmt Cache status.
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
fbe_ps_mgmt_checkCacheStatus(fbe_ps_mgmt_t * pPsMgmt)
{
    fbe_device_physical_location_t      location;
    fbe_status_t                        status;
    fbe_encl_ps_info_t                  enclPsInfo;
    fbe_encl_ps_info_t                  *pEnclPsInfo = &enclPsInfo;
    fbe_common_cache_status_t           psCacheStatus = FBE_CACHE_STATUS_FAILED;
    fbe_common_cache_status_t           pePsCacheStatus = FBE_CACHE_STATUS_FAILED;
    fbe_common_cache_status_t           dae0PsCacheStatus = FBE_CACHE_STATUS_FAILED;
    fbe_common_cache_status_t           PeerPsCacheStatus = FBE_CACHE_STATUS_OK;
    fbe_u32_t                           psFaultCount, psIndex;
    fbe_u32_t                           psOverTempCount = 0;

    /*
     * Check the enclosures involved with Caching (DPE or xPE/DAE0)
     */
    // Processor Enclosure
    if (pPsMgmt->base_environment.processorEnclType == DPE_ENCLOSURE_TYPE)
    {
        location = pPsMgmt->base_environment.bootEnclLocation;
    }
    else if (pPsMgmt->base_environment.processorEnclType == VPE_ENCLOSURE_TYPE)
    {
        location.bus = FBE_XPE_PSEUDO_BUS_NUM;
        location.enclosure = FBE_XPE_PSEUDO_ENCL_NUM;
    }
    else
    {
        location.bus = FBE_XPE_PSEUDO_BUS_NUM;
        location.enclosure = FBE_XPE_PSEUDO_ENCL_NUM;
    }
    status = fbe_ps_mgmt_get_encl_ps_info_by_location(pPsMgmt, &location, &pEnclPsInfo);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)pPsMgmt, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s, error getting PE PS info, status 0x%x\n",
                              __FUNCTION__, status);
        pePsCacheStatus = FBE_CACHE_STATUS_FAILED;
    }
    else
    {
        psFaultCount = 0;
        for (psIndex = 0; psIndex < pEnclPsInfo->psCount; psIndex++)
        {
            if (!pEnclPsInfo->psInfo[psIndex].bInserted || 
                !pEnclPsInfo->psInfo[psIndex].psSupported ||
                pEnclPsInfo->psInfo[psIndex].generalFault)
            {
                psFaultCount++;
            }
            // check for PS's reporting OverTemp & count (need two PS's with OT to confirm,
            // but don't count PS's with Env Interface faults)
            if (pEnclPsInfo->psInfo[psIndex].bInserted && 
               (pEnclPsInfo->psInfo[psIndex].envInterfaceStatus == FBE_ENV_INTERFACE_STATUS_GOOD) &&
                pEnclPsInfo->psInfo[psIndex].ambientOvertempFault)
            {
                psOverTempCount++;
            }
        }
        if (psFaultCount == 0)
        {
            pePsCacheStatus = FBE_CACHE_STATUS_OK;
        }
        else if ((psFaultCount == pEnclPsInfo->psCount) ||
                 ((pEnclPsInfo->psCount > 2) && 
                  (psFaultCount >= (pEnclPsInfo->psCount - 1))))
        {
            pePsCacheStatus = FBE_CACHE_STATUS_FAILED;
        }
        else
        {
            pePsCacheStatus = FBE_CACHE_STATUS_DEGRADED;
        }
        if (pePsCacheStatus != FBE_CACHE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t *)pPsMgmt, 
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s (PE), pePsCacheStatus %d, psCount %d, psFaultCount %d\n",
                                  __FUNCTION__, pePsCacheStatus, pEnclPsInfo->psCount, psFaultCount);
        }
    }

    // DAE0 if needed
    if (pPsMgmt->base_environment.processorEnclType == DPE_ENCLOSURE_TYPE)
    {
        dae0PsCacheStatus = FBE_CACHE_STATUS_OK;
    }
    else
    {
        location = pPsMgmt->base_environment.bootEnclLocation;
        status = fbe_ps_mgmt_get_encl_ps_info_by_location(pPsMgmt, &location, &pEnclPsInfo);
        if (status != FBE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t *)pPsMgmt, 
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s, error getting DAE0 PS info, status 0x%x\n",
                                  __FUNCTION__, status);
            dae0PsCacheStatus = FBE_CACHE_STATUS_FAILED;
        }
        else
        {
            psFaultCount = 0;
            for (psIndex = 0; psIndex < pEnclPsInfo->psCount; psIndex++)
            {
                if (!pEnclPsInfo->psInfo[psIndex].bInserted || 
                    !pEnclPsInfo->psInfo[psIndex].psSupported ||
                    pEnclPsInfo->psInfo[psIndex].generalFault)
                {
                    psFaultCount++;
                }
// JAP - don't look at OverTemp for DAE's (Expander will take care of that)
#if FALSE
                // check for PS's reporting OverTemp & count (need two PS's with OT to confirm)
                if (pEnclPsInfo->psInfo[psIndex].bInserted && 
                    pEnclPsInfo->psInfo[psIndex].ambientOvertempFault)
                {
                    psOverTempCount++;
                }
#endif 
            }
            if (psFaultCount == 0)
            {
                dae0PsCacheStatus = FBE_CACHE_STATUS_OK;
            }
            else if ((psFaultCount == pEnclPsInfo->psCount) ||
             ((pEnclPsInfo->psCount > 2) && 
              (psFaultCount >= (pEnclPsInfo->psCount - 1))))
            {

                dae0PsCacheStatus = FBE_CACHE_STATUS_FAILED;
            }
            else
            {
                dae0PsCacheStatus = FBE_CACHE_STATUS_DEGRADED;
            }
           if (dae0PsCacheStatus != FBE_CACHE_STATUS_OK)
           {
               fbe_base_object_trace((fbe_base_object_t *)pPsMgmt, 
                                     FBE_TRACE_LEVEL_INFO,
                                     FBE_TRACE_MESSAGE_ID_INFO,
                                     "%s (DAE0), dae0PsCacheStatus %d, psCount %d, psFaultCount %d\n",
                                     __FUNCTION__, dae0PsCacheStatus, pEnclPsInfo->psCount, psFaultCount);
           }
        }
    }

    // combine statuses
    if ((pePsCacheStatus == FBE_CACHE_STATUS_FAILED) ||
        (dae0PsCacheStatus == FBE_CACHE_STATUS_FAILED) ||
        (psOverTempCount >= 2))
    {
        psCacheStatus = FBE_CACHE_STATUS_FAILED;
    }
    else if ((pePsCacheStatus == FBE_CACHE_STATUS_DEGRADED) ||
        (dae0PsCacheStatus == FBE_CACHE_STATUS_DEGRADED) ||
        (psOverTempCount > 0))
    {
        psCacheStatus = FBE_CACHE_STATUS_DEGRADED;
    }
    else
    {
        psCacheStatus = FBE_CACHE_STATUS_OK;
    }
    if (psCacheStatus != FBE_CACHE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)pPsMgmt, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s, PsCacheStatus PE %d, DAE0 %d, psOvrTmpCnt %d, psCacheStatus %d\n",
                              __FUNCTION__, pePsCacheStatus, dae0PsCacheStatus, psOverTempCount, psCacheStatus);
    }

    /*
     * Check for status change & send notification if needed
     */
    if (psCacheStatus != pPsMgmt->psCacheStatus)
    {
        fbe_base_object_trace((fbe_base_object_t *)pPsMgmt, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s, psCacheStatus changes from %d, to %d\n",
                              __FUNCTION__,
                              pPsMgmt->psCacheStatus,
                              psCacheStatus);

        /* Log the event for cache status change */
        fbe_event_log_write(ESP_INFO_CACHE_STATUS_CHANGED,
                    NULL, 0,
                    "%s %s %s",
                    "PS_MGMT",
                    fbe_base_env_decode_cache_status(pPsMgmt->psCacheStatus),
                    fbe_base_env_decode_cache_status(psCacheStatus));


        status = fbe_base_environment_get_peerCacheStatus((fbe_base_environment_t *)pPsMgmt,
                                                             &PeerPsCacheStatus);
        /* Send CMI to peer */
        fbe_base_environment_send_cacheStatus_update_to_peer((fbe_base_environment_t *)pPsMgmt, 
                                                              psCacheStatus,
                                                             ((PeerPsCacheStatus == FBE_CACHE_STATUS_UNINITIALIZE)? FALSE : TRUE));

        pPsMgmt->psCacheStatus = psCacheStatus;
        fbe_zero_memory(&location, sizeof(fbe_device_physical_location_t));     // location not relevant
        fbe_base_environment_send_data_change_notification((fbe_base_environment_t *)pPsMgmt, 
                                                           FBE_CLASS_ID_PS_MGMT, 
                                                           FBE_DEVICE_TYPE_SP_ENVIRON_STATE, 
                                                           FBE_DEVICE_DATA_TYPE_GENERIC_INFO,
                                                           &location);    
        fbe_base_object_trace((fbe_base_object_t *)pPsMgmt, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s, FBE_DEVICE_TYPE_SP_ENVIRON_STATE sent\n",
                              __FUNCTION__);
    }

    return FBE_STATUS_OK;
}
/******************************************************
 * end fbe_ps_mgmt_checkCacheStatus() 
 ******************************************************/
/*!**************************************************************
 * @fn fbe_ps_mgmt_process_peer_cache_status_update()
 ****************************************************************
 * @brief
 *  This function will handle the message we received from peer 
 *  to update peerCacheStatus.
 *
 * @param pPsMgmt -
 * @param cmiMsgPtr - Pointer to ps_mgmt cmi packet
 * @param classId - ps_mgmt class id.
 *
 * @return fbe_status_t 
 *
 * @author
 *  20-Nov-2012:  Dipak Patel - Created. 
 *
 ****************************************************************/
static fbe_status_t fbe_ps_mgmt_process_peer_cache_status_update(fbe_ps_mgmt_t *pPsMgmt, 
                                                                 fbe_ps_mgmt_cmi_packet_t * cmiMsgPtr)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_base_env_cacheStatus_cmi_packet_t *baseCmiPtr = (fbe_base_env_cacheStatus_cmi_packet_t *)cmiMsgPtr;
    fbe_common_cache_status_t prevCombineCacheStatus = FBE_CACHE_STATUS_OK;
    fbe_common_cache_status_t peerCacheStatus = FBE_CACHE_STATUS_OK;
    
    fbe_base_object_trace((fbe_base_object_t *)pPsMgmt, 
                   FBE_TRACE_LEVEL_INFO,
                   FBE_TRACE_MESSAGE_ID_INFO,
                   "%s entry.\n",
                   __FUNCTION__); 

    status = fbe_base_environment_get_peerCacheStatus((fbe_base_environment_t *)pPsMgmt,
                                                       &peerCacheStatus); 

    prevCombineCacheStatus = fbe_base_environment_combine_cacheStatus((fbe_base_environment_t *)pPsMgmt,
                                                                       pPsMgmt->psCacheStatus,
                                                                       peerCacheStatus,
                                                                       FBE_CLASS_ID_PS_MGMT);


    /* Update Peer cache status for this side */
    status = fbe_base_environment_set_peerCacheStatus((fbe_base_environment_t *)pPsMgmt,
                                                       baseCmiPtr->CacheStatus);

    /* Compare it with local cache status and send notification to cache if require.*/ 
    status = fbe_base_environment_compare_cacheStatus((fbe_base_environment_t *)pPsMgmt,
                                                      prevCombineCacheStatus,
                                                      baseCmiPtr->CacheStatus,
                                                      FBE_CLASS_ID_PS_MGMT);

    /* Check if peer cache status is not initialized at peer side.
       If so, we need to send CMI message to peer to update it's
       peer cache status */
    if (baseCmiPtr->isPeerCacheStatusInitilized == FALSE)
    {
        /* Send CMI to peer.
           Over here, peerCacheStatus for this SP will intilized as we have update it above.
           So we are returning it with TRUE */

        fbe_base_object_trace((fbe_base_object_t *)pPsMgmt, 
                   FBE_TRACE_LEVEL_INFO,
                   FBE_TRACE_MESSAGE_ID_INFO,
                   "%s, peerCachestatus is uninitialized at peer\n",
                   __FUNCTION__);

        status = fbe_base_environment_send_cacheStatus_update_to_peer((fbe_base_environment_t *)pPsMgmt, 
                                                                       pPsMgmt->psCacheStatus,
                                                                       TRUE);
    }

    return status;

}

/*!**************************************************************
 * fbe_ps_mgmt_debounce_timer_cond_function()
 ****************************************************************
 * @brief
 *  This timer function performs Debounce logic for PS
 *  Fault (we want to ignore short glitches).
 *
 * @param object_handle - This is the object handle, or in our
 * case the ps_mgmt.
 * @param packet_p - The packet arriving from the monitor
 * scheduler.
 *
 * @return fbe_status_t - The status of the call to
 *                        fbe_lifecycle_class_const_verify()
 *                        for the PS management's
 *                        constant lifecycle data.
 *
 * @author
 *  21-Mar-2013:  Created. Joe Perry
 *
 ****************************************************************/
static fbe_lifecycle_status_t 
fbe_ps_mgmt_debounce_timer_cond_function(fbe_base_object_t * base_object, 
                                          fbe_packet_t * packet)
{
    fbe_status_t                    status;
    fbe_ps_mgmt_t                   *ps_mgmt = (fbe_ps_mgmt_t *) base_object;
    fbe_u32_t                       enclIndex;
    fbe_u32_t                       psIndex;
    fbe_encl_ps_info_t              *psInfoPtr;
    fbe_bool_t                      stopTheTimer = FBE_TRUE;

    fbe_base_object_trace(base_object, 
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s entry, enclCount %d\n",
                          __FUNCTION__, ps_mgmt->enclCount);

    /*
     * Go through all enclosures & PS's looking for active Debounces
     */
    for (enclIndex = 0; enclIndex < FBE_ESP_MAX_ENCL_COUNT; enclIndex++)
    {
        psInfoPtr = &ps_mgmt->psInfo->enclPsInfo[enclIndex];
        if (psInfoPtr == NULL)
        {
            continue;
        }
        if (psInfoPtr->inUse)
        {
            for (psIndex = 0; psIndex < psInfoPtr->psCount; psIndex++)
            {
                // if there's a fault masked we'll process the status
                if (psInfoPtr->psDebounceInfo[psIndex].debounceTimerActive &&
                    (psInfoPtr->psDebounceInfo[psIndex].faultMasked))
                {
                    psInfoPtr->location.slot = psIndex;
                    psInfoPtr->psDebounceInfo[psIndex].debouncePeriodExpired = FBE_TRUE;
                    fbe_base_object_trace(base_object, 
                                          FBE_TRACE_LEVEL_INFO,
                                          FBE_TRACE_MESSAGE_ID_INFO,
                                          "%s, %d_%d_%d, stop debouncing\n",
                                          __FUNCTION__, 
                                          psInfoPtr->location.bus, psInfoPtr->location.enclosure, psInfoPtr->location.slot);
                    status = fbe_ps_mgmt_processPsStatus(ps_mgmt, psInfoPtr->objectId, &psInfoPtr->location);
                    stopTheTimer = FBE_FALSE;
                }
            }
        }
    }

    // this function will only stop the timer when we don't find fault masked
    // otherwise the timer will be stopped in fbe_ps_mgmt_processPsStatus()
    if(stopTheTimer)
    {
        status = fbe_lifecycle_stop_timer(&FBE_LIFECYCLE_CONST_DATA(ps_mgmt),
                                          (fbe_base_object_t*)ps_mgmt,
                                          FBE_PS_MGMT_DEBOUNCE_TIMER_COND);
        if (status == FBE_STATUS_OK)
        {
            fbe_base_object_trace(base_object, 
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s, timer stopped\n",
                                  __FUNCTION__);
        }
        else
        {
            fbe_base_object_trace(base_object, 
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s, error stopping timer, status 0x%x\n",
                                  __FUNCTION__, status);
        }
    }
    // timer is stopped, clean the debounce info
    if (psInfoPtr->inUse)
    {
        for (psIndex = 0; psIndex < psInfoPtr->psCount; psIndex++)
        {
            psInfoPtr->psDebounceInfo[psIndex].debouncePeriodExpired = FBE_TRUE;
            psInfoPtr->psDebounceInfo[psIndex].debounceTimerActive = FBE_FALSE;
            psInfoPtr->psDebounceInfo[psIndex].faultMasked = FBE_FALSE;
        }
    }


    return FBE_LIFECYCLE_STATUS_DONE;
    
}
/******************************************************
 * end fbe_ps_mgmt_debounce_timer_cond_function() 
 ******************************************************/


/*!**************************************************************
 * fbe_ps_mgmt_read_persistent_data_complete_cond_function()
 ****************************************************************
 * @brief
 *  This timer function handles the completion of a persistent
 *  data read request.  It will copy any persistent data read
 *  into the ps_mgmt data.  Checking against this value happens
 *  when power supplies are discovered.
 *
 * @param object_handle - This is the object handle, or in our
 * case the ps_mgmt.
 * @param packet_p - The packet arriving from the monitor
 * scheduler.
 *
 * @return fbe_status_t - The status of the call to
 *                        fbe_lifecycle_class_const_verify()
 *                        for the PS management's
 *                        constant lifecycle data.
 *
 * @author
 *  28-Oct-2013:  Created. Brion Philbin
 *  
 ****************************************************************/
static fbe_lifecycle_status_t 
fbe_ps_mgmt_read_persistent_data_complete_cond_function(fbe_base_object_t * base_object, 
                                                        fbe_packet_t * packet)
{
    fbe_ps_mgmt_t *ps_mgmt = (fbe_ps_mgmt_t *) base_object;
    fbe_status_t status;

    status = fbe_lifecycle_clear_current_cond(base_object);
    if (status != FBE_STATUS_OK) {
        fbe_base_object_trace(base_object, 
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s can't clear current condition, status: 0x%X",
                                __FUNCTION__, status);
    }

    /*
     * Copy data over from persistent storage into the structures.
     */
    fbe_copy_memory(&ps_mgmt->expected_power_supply, ((fbe_base_environment_t *)base_object)->my_persistent_data, sizeof(fbe_ps_mgmt_expected_ps_type_t));

    fbe_base_object_trace(base_object, 
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "PS_MGMT: read expected_power_supply:%d from disk\n",
                          ps_mgmt->expected_power_supply);

    
    return FBE_LIFECYCLE_STATUS_DONE;
}
/***************************************************************
 * end fbe_ps_mgmt_read_persistent_data_complete_cond_function() 
 ***************************************************************/

/*!***************************************************************
 * fbe_ps_mgmt_calculatePsStateInfo()
 ****************************************************************
 * @brief
 *  This function will calculate & update the specified
 *  PS's State & SubState attributes.
 *
 * @param ps_mgmt - Handle to ps_mgmt.
 * @param pLocation - pointer to PS location info
 *
 * @return fbe_status_t
 *
 * @author
 *  5-Mar-2015: Created  Joe Perry
 *
 ****************************************************************/
static fbe_status_t 
fbe_ps_mgmt_calculatePsStateInfo(fbe_ps_mgmt_t *ps_mgmt,
                                    fbe_device_physical_location_t *pLocation)
{
    fbe_status_t            status;
    fbe_encl_ps_info_t      *pEnclPsInfo = NULL;

    status = fbe_ps_mgmt_get_encl_ps_info_by_location(ps_mgmt, pLocation, &pEnclPsInfo);
    if (status != FBE_STATUS_OK) 
    {
        fbe_base_object_trace((fbe_base_object_t*)ps_mgmt, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s, Populate encl ps info for encl %d_%d, status %d.\n",
                              __FUNCTION__, pLocation->bus, pLocation->enclosure, status);
        return status;
    }

    if (pEnclPsInfo->psInfo[pLocation->slot].isFaultRegFail)
    {
        pEnclPsInfo->psInfo[pLocation->slot].state = FBE_PS_STATE_FAULTED;
        pEnclPsInfo->psInfo[pLocation->slot].subState = FBE_PS_SUBSTATE_FLT_STATUS_REG_FAULT;
    }
    else if (pEnclPsInfo->psInfo[pLocation->slot].fupFailure &&
             pEnclPsInfo->psInfo[pLocation->slot].bInserted)
    {
        pEnclPsInfo->psInfo[pLocation->slot].state = FBE_PS_STATE_DEGRADED;
        pEnclPsInfo->psInfo[pLocation->slot].subState = FBE_PS_SUBSTATE_FUP_FAILURE;
    } 
    else if (pEnclPsInfo->psInfo[pLocation->slot].resumePromReadFailed &&
             pEnclPsInfo->psInfo[pLocation->slot].bInserted)
    {
        pEnclPsInfo->psInfo[pLocation->slot].state = FBE_PS_STATE_DEGRADED;
        pEnclPsInfo->psInfo[pLocation->slot].subState = FBE_PS_SUBSTATE_RP_READ_FAILURE;
    } 
    else if ((pEnclPsInfo->psInfo[pLocation->slot].envInterfaceStatus == FBE_ENV_INTERFACE_STATUS_XACTION_FAIL) ||
        (pEnclPsInfo->psInfo[pLocation->slot].envInterfaceStatus == FBE_ENV_INTERFACE_STATUS_DATA_STALE))
    {
        pEnclPsInfo->psInfo[pLocation->slot].state = FBE_PS_STATE_DEGRADED;
        pEnclPsInfo->psInfo[pLocation->slot].subState = FBE_PS_SUBSTATE_ENV_INTF_FAILURE;
    } 
    else 
    {
        pEnclPsInfo->psInfo[pLocation->slot].state = FBE_PS_STATE_OK;
        pEnclPsInfo->psInfo[pLocation->slot].subState = FBE_PS_SUBSTATE_NO_FAULT;
    }

    fbe_base_object_trace((fbe_base_object_t*)ps_mgmt, 
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s, %d_%d_%d, state %s, substate %s\n",
                          __FUNCTION__,
                          pLocation->bus, pLocation->enclosure, pLocation->slot,
                          fbe_ps_mgmt_decode_ps_state(pEnclPsInfo->psInfo[pLocation->slot].state),
                          fbe_ps_mgmt_decode_ps_subState(pEnclPsInfo->psInfo[pLocation->slot].subState));
    return FBE_STATUS_OK;

}   // end of fbe_ps_mgmt_calculatePsStateInfo


/* End of file fbe_ps_mgmt_monitor.c */
