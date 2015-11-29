/***************************************************************************
 * Copyright (C) EMC Corporation 2010 
 * All rights reserved. Licensed material -- property of EMC 
 * Corporation 
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_board_mgmt_monitor.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the Board Management object lifecycle
 *  code.
 * 
 * @ingroup board_mgmt_class_files
 * 
 * @version
 *  29-July-2010: Created  Vaibhav Gaonkar
 *
 ***************************************************************************/
/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_memory.h"
#include "fbe_topology.h"
#include "fbe_base_object.h"
#include "fbe_scheduler.h"
#include "fbe_board_mgmt_private.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_api_enclosure_interface.h"
#include "fbe_transport_memory.h"
#include "generic_types.h"
#include "fbe/fbe_api_board_interface.h"
#include "fbe_base_environment_debug.h"
#include "fbe/fbe_event_log_api.h"
#include "EspMessages.h"
#include "generic_utils_lib.h"
#include "fbe_board_mgmt_utils.h"

#define BMC_FAULT_STRING_LENGTH 500
#define BMC_ACTION_STRING_LENGTH 120

/*!***************************************************************
 *  fbe_board_mgmt_monitor_entry()
 ****************************************************************
 * @brief
 *  Entry routine for the Board Management monitor.
 *
 * @param object_handle - This is the object handle, or in our
 * case the board_mgmt.
 * @param packet_p - The packet arriving from the monitor
 * scheduler.
 *
 * @return fbe_status_t - Status of monitor operation.
 *
 * @author
 *  29-July-2010: Created  Vaibhav Gaonkar
 *
 ****************************************************************/
fbe_status_t 
fbe_board_mgmt_monitor_entry(fbe_object_handle_t object_handle, fbe_packet_t * packet)
{
    fbe_board_mgmt_t * board_mgmt = NULL;

    board_mgmt = (fbe_board_mgmt_t *)fbe_base_handle_to_pointer(object_handle);

    fbe_topology_class_trace(FBE_CLASS_ID_BOARD_MGMT,
                             FBE_TRACE_LEVEL_DEBUG_HIGH, 
                             FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                             "%s entry\n", __FUNCTION__);
                 
    return fbe_lifecycle_crank_object(&FBE_LIFECYCLE_CONST_DATA(board_mgmt),
                                      (fbe_base_object_t*)board_mgmt, packet);
}
/******************************************
 * end fbe_board_mgmt_monitor_entry()
 ******************************************/
/*!**************************************************************
 * fbe_board_mgmt_monitor_load_verify()
 ****************************************************************
 * @brief
 *  This function simply validates the constant lifecycle data
 *  that is associated with the board management.
 *
 * @param  - None.               
 *
 * @return fbe_status_t - The status of the call to
 *                        fbe_lifecycle_class_const_verify()
 *                        for the board management's constant
 *                        lifecycle data.
 *
 * @author
 *  29-July-2010: Created  Vaibhav Gaonkar
 *
 ****************************************************************/
fbe_status_t fbe_board_mgmt_monitor_load_verify(void)
{
    return fbe_lifecycle_class_const_verify(&FBE_LIFECYCLE_CONST_DATA(board_mgmt));
}
/******************************************
 * end fbe_board_mgmt_monitor_load_verify()
 ******************************************/

/*--- local function prototypes --------------------------------------------------------*/
static fbe_lifecycle_status_t fbe_board_mgmt_register_state_change_callback_cond_function(fbe_base_object_t * base_object, 
                                                                                          fbe_packet_t * packet);
static fbe_lifecycle_status_t fbe_board_mgmt_register_cmi_callback_cond_function(fbe_base_object_t * base_object, 
                                                                                 fbe_packet_t * packet);
static fbe_lifecycle_status_t fbe_board_mgmt_register_resume_prom_callback_cond_function(fbe_base_object_t * base_object, 
                                                                                         fbe_packet_t * packet);

static fbe_status_t fbe_board_mgmt_state_change_handler(update_object_msg_t *update_object_msg, void *context);
static fbe_status_t fbe_board_mgmt_handle_object_data_change(fbe_board_mgmt_t * pBoardMgmt, 
                                                             update_object_msg_t * pUpdateObjectMsg);
static fbe_status_t fbe_board_mgmt_handle_generic_info_change(fbe_board_mgmt_t * board_mgmt, 
                                                              update_object_msg_t * pUpdateObjectMsg);

static fbe_status_t fbe_board_mgmt_cmi_message_handler(fbe_base_object_t *base_object, 
                                                       fbe_base_environment_cmi_message_info_t *cmi_message);
static char * fbe_board_mgmt_getCmiMsgTypeString(fbe_u32_t boardMsgType);
static fbe_status_t fbe_board_mgmt_sendCmiMessage(fbe_board_mgmt_t *board_mgmt, 
                                                  fbe_u32_t boardMsgType);
static fbe_status_t fbe_board_mgmt_processReceivedCmiMessage(fbe_board_mgmt_t * board_mgmt, 
                                         fbe_board_mgmt_cmi_packet_t *pBoardCmiPkt);
static void fbe_board_mgmt_discoverSpPhysicalMemory(fbe_board_mgmt_t *board_mgmt);
static void fbe_board_mgmt_updatePeerPhysicalMemory(fbe_board_mgmt_t *board_mgmt, 
                                        fbe_board_mgmt_cmi_msg_t *pBoardCmiPkt);
static fbe_lifecycle_status_t fbe_board_mgmt_discover_board_cond_function(fbe_base_object_t * base_object, 
                                                                          fbe_packet_t * packet);
static fbe_status_t  fbe_board_mgmt_processPlatformStatus(fbe_board_mgmt_t  *board_mgmt,
                                                       fbe_device_physical_location_t *pLocation);
static fbe_status_t fbe_board_mgmt_processMiscStatus(fbe_board_mgmt_t *board_mgmt);
static fbe_status_t fbe_board_mgmt_processSuitcaseStatus(fbe_board_mgmt_t *board_mgmt,
                                                         fbe_device_physical_location_t *pLocation);
static fbe_status_t fbe_board_mgmt_processBmcStatus(fbe_board_mgmt_t *board_mgmt,
                                                    fbe_device_physical_location_t *pLocation);
static void fbe_board_mgmt_discoverFltRegisterInfo(fbe_board_mgmt_t *board_mgmt);
static fbe_status_t fbe_board_mgmt_processFaultRegisterStatus(fbe_board_mgmt_t *board_mgmt,
                                                              fbe_device_physical_location_t *pLocation);
static fbe_lifecycle_status_t fbe_board_mgmt_peer_boot_check_cond_function(fbe_base_object_t * base_object,
                                                                           fbe_packet_t * packet);
static const fbe_char_t * fbe_board_mgmt_decodeBmcShutdownReason(SHUTDOWN_REASON *bmcShutdownReason);
static fbe_status_t fbe_board_mgmt_processBmcBistStatus(fbe_board_mgmt_t *board_mgmt,
                                                        fbe_device_physical_location_t *pLocation,
                                                        fbe_board_mgmt_bist_info_t *newBistStatus,
                                                        fbe_board_mgmt_bist_info_t *prevBistStatus);
static fbe_status_t fbe_board_mgmt_processSlavePortStatus(fbe_board_mgmt_t *board_mgmt,
                                                          fbe_device_physical_location_t *pLocation);

static fbe_status_t fbe_board_mgmt_process_peer_cache_status_update(fbe_board_mgmt_t *board_mgmt, 
                                                                    fbe_board_mgmt_cmi_packet_t * cmiMsgPtr);

static fbe_status_t fbe_board_mgmt_processPeerNotPresent(fbe_board_mgmt_t * pBoardMgmt, 
                                    fbe_board_mgmt_cmi_packet_t * pBoardCmiPkt);
static fbe_status_t fbe_board_mgmt_processPeerBusy(fbe_board_mgmt_t * pBoardMgmt, 
                              fbe_board_mgmt_cmi_packet_t * pBoardCmiPkt);
static fbe_status_t fbe_board_mgmt_processFatalError(fbe_board_mgmt_t * pBoardMgmt, 
                                fbe_board_mgmt_cmi_packet_t * pBoardCmiPkt);
static fbe_status_t fbe_board_mgmt_get_sp_lcc_info(fbe_board_mgmt_t *board_mgmt,
                                                         fbe_lcc_info_t * pLccInfo,
                                                         fbe_u8_t slot);
static fbe_status_t fbe_board_mgmt_processSPLccStatus(fbe_board_mgmt_t *board_mgmt,
                                  fbe_device_physical_location_t *pLocation);
static fbe_status_t fbe_board_mgmt_processCacheCardStatus(fbe_board_mgmt_t *board_mgmt,
                                fbe_device_physical_location_t *pLocation);
static fbe_status_t fbe_board_mgmt_processDimmStatus(fbe_board_mgmt_t *board_mgmt,
                                fbe_device_physical_location_t *pLocation);

static fbe_status_t fbe_board_mgmt_processSSDStatus(fbe_board_mgmt_t *board_mgmt,
                                fbe_device_physical_location_t *pLocation);
static fbe_status_t fbe_board_mgmt_create_cache_card_fault_string(fbe_board_mgmt_cache_card_info_t *pCacheCardInfo, 
                                fbe_bool_t *isFaulted, char* string);

static fbe_lifecycle_status_t fbe_board_mgmt_fup_register_callback_cond_function(fbe_base_object_t * base_object,  fbe_packet_t * packet);


static fbe_status_t fbe_board_mgmt_checkCacheStatus(fbe_board_mgmt_t *board_mgmt,
                                                    fbe_device_physical_location_t *pLocation);


static fbe_bool_t fbe_board_mgmt_realFltRegFaultDetected(fbe_board_mgmt_t *board_mgmt);

static fbe_status_t fbe_board_mgmt_calculateSuitcaseStateInfo(fbe_board_mgmt_t *board_mgmt,
                                                              SP_ID sp,
                                                              fbe_u32_t slot);
static fbe_status_t fbe_board_mgmt_calculateDimmStateInfo(fbe_board_mgmt_t *board_mgmt,
                                                          fbe_board_mgmt_dimm_info_t * pDimmInfo);

static fbe_status_t fbe_board_mgmt_calculateSsdStateInfo(fbe_board_mgmt_t *board_mgmt,
                                                         fbe_board_mgmt_ssd_info_t * pSsdInfo);

static fbe_status_t fbe_board_mgmt_handle_destroy_state(fbe_board_mgmt_t * pBoardMgmt, 
                                                     update_object_msg_t * pUpdateObjectMsg);

static fbe_status_t fbe_board_mgmt_handle_ready_state(fbe_board_mgmt_t * pBoardMgmt, 
                                                     update_object_msg_t * pUpdateObjectMsg);

/*--- lifecycle callback functions -----------------------------------------------------*/

FBE_LIFECYCLE_DEF_CALLBACK_GET_INST_DATA(board_mgmt);
FBE_LIFECYCLE_DEF_CALLBACK_GET_INST_COND(board_mgmt);

/*  board_mgmt_lifecycle_callbacks This variable has all the
 *  callbacks the lifecycle needs.
 */
static fbe_lifecycle_const_callback_t FBE_LIFECYCLE_CALLBACKS(board_mgmt) = {
        FBE_LIFECYCLE_DEF_CONST_CALLBACKS(
        board_mgmt,
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
static fbe_lifecycle_const_cond_t fbe_board_mgmt_register_state_change_callback_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_ENV_LIFECYCLE_COND_REGISTER_STATE_CHANGE_CALLBACK,
        fbe_board_mgmt_register_state_change_callback_cond_function)
};

/* FBE_BASE_ENV_LIFECYCLE_COND_REGISTER_CMI_CALLBACK condition
 *  - The purpose of this derived condition is to register
 *  notification with the base environment object which inturns
 *  registers with the CMI. The leaf class needs to implement
 *  the actual condition handler function providing the callback
 *  that needs to be called
 */
static fbe_lifecycle_const_cond_t fbe_board_mgmt_register_cmi_callback_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_ENV_LIFECYCLE_COND_REGISTER_CMI_CALLBACK,
        fbe_board_mgmt_register_cmi_callback_cond_function)
};

/* FBE_BASE_ENV_LIFECYCLE_COND_FUP_REGISTER_CALLBACK condition
 *  - The purpose of this derived condition is to register
 *  call back functions with the base environment object. 
 * The leaf class needs to implement the actual call back functions.
 */
static fbe_lifecycle_const_cond_t fbe_board_mgmt_fup_register_callback_cond = {
	FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
		FBE_BASE_ENV_LIFECYCLE_COND_FUP_REGISTER_CALLBACK,
        fbe_board_mgmt_fup_register_callback_cond_function)
};

static fbe_lifecycle_const_cond_t fbe_board_mgmt_fup_abort_upgrade_cond = {
	FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
		FBE_BASE_ENV_LIFECYCLE_COND_FUP_ABORT_UPGRADE,
        fbe_base_env_fup_abort_upgrade_cond_function)
};

static fbe_lifecycle_const_cond_t fbe_board_mgmt_fup_wait_before_upgrade_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_ENV_LIFECYCLE_COND_FUP_WAIT_BEFORE_UPGRADE,
        fbe_base_env_fup_wait_before_upgrade_cond_function)
};

static fbe_lifecycle_const_cond_t fbe_board_mgmt_fup_wait_for_inter_device_delay_cond = {
	FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
		FBE_BASE_ENV_LIFECYCLE_COND_FUP_WAIT_FOR_INTER_DEVICE_DELAY,
        fbe_base_env_fup_wait_for_inter_device_delay_cond_function)
};

static fbe_lifecycle_const_cond_t fbe_board_mgmt_fup_read_image_header_cond = {
	FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
		FBE_BASE_ENV_LIFECYCLE_COND_FUP_READ_IMAGE_HEADER,
        fbe_base_env_fup_read_image_header_cond_function)
};

static fbe_lifecycle_const_cond_t fbe_board_mgmt_fup_check_rev_cond = {
	FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
		FBE_BASE_ENV_LIFECYCLE_COND_FUP_CHECK_REV,
        fbe_base_env_fup_check_rev_cond_function)
};

static fbe_lifecycle_const_cond_t fbe_board_mgmt_fup_read_entire_image_cond = {
	FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
		FBE_BASE_ENV_LIFECYCLE_COND_FUP_READ_ENTIRE_IMAGE,
        fbe_base_env_fup_read_entire_image_cond_function)
};

static fbe_lifecycle_const_cond_t fbe_board_mgmt_fup_download_image_cond = {
	FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
		FBE_BASE_ENV_LIFECYCLE_COND_FUP_DOWNLOAD_IMAGE,
        fbe_base_env_fup_download_image_cond_function)
};

static fbe_lifecycle_const_cond_t fbe_board_mgmt_fup_get_download_status_cond = {
	FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
		FBE_BASE_ENV_LIFECYCLE_COND_FUP_GET_DOWNLOAD_STATUS,
        fbe_base_env_fup_get_download_status_cond_function)
};

static fbe_lifecycle_const_cond_t fbe_board_mgmt_fup_get_peer_permission_cond = {
	FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
		FBE_BASE_ENV_LIFECYCLE_COND_FUP_GET_PEER_PERMISSION,
        fbe_base_env_fup_get_peer_permission_cond_function)
};

static fbe_lifecycle_const_cond_t fbe_board_mgmt_fup_check_env_status_cond = {
	FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
		FBE_BASE_ENV_LIFECYCLE_COND_FUP_CHECK_ENV_STATUS,
        fbe_base_env_fup_check_env_status_cond_function)
};

static fbe_lifecycle_const_cond_t fbe_board_mgmt_fup_activate_image_cond = {
	FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
		FBE_BASE_ENV_LIFECYCLE_COND_FUP_ACTIVATE_IMAGE,
        fbe_base_env_fup_activate_image_cond_function)
};

static fbe_lifecycle_const_cond_t fbe_board_mgmt_fup_get_activate_status_cond = {
	FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
		FBE_BASE_ENV_LIFECYCLE_COND_FUP_GET_ACTIVATE_STATUS,
        fbe_base_env_fup_get_activate_status_cond_function)
};

static fbe_lifecycle_const_cond_t fbe_board_mgmt_fup_check_result_cond = {
	FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
		FBE_BASE_ENV_LIFECYCLE_COND_FUP_CHECK_RESULT,
        fbe_base_env_fup_check_result_cond_function)
};

static fbe_lifecycle_const_cond_t fbe_board_mgmt_fup_refresh_device_status_cond = {
	FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
		FBE_BASE_ENV_LIFECYCLE_COND_FUP_REFRESH_DEVICE_STATUS,
        fbe_base_env_fup_refresh_device_status_cond_function)
};

static fbe_lifecycle_const_cond_t fbe_board_mgmt_fup_end_upgrade_cond = {
	FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
		FBE_BASE_ENV_LIFECYCLE_COND_FUP_END_UPGRADE,
        fbe_base_env_fup_end_upgrade_cond_function)
};

static fbe_lifecycle_const_cond_t fbe_board_mgmt_fup_release_image_cond = {
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
static fbe_lifecycle_const_cond_t fbe_board_mgmt_register_resume_prom_callback_cond = {
	FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
		FBE_BASE_ENV_LIFECYCLE_COND_RESUME_PROM_REGISTER_CALLBACK,
        fbe_board_mgmt_register_resume_prom_callback_cond_function)
};

/*--- constant base condition entries --------------------------------------------------*/
/* FBE_BOARD_MGMT_LIFECYCLE_COND_DISCOVER_BOARD condition -
 *   The purpose of this condition is start the discovery process
 *   of a new Board that was added
 */
static fbe_lifecycle_const_base_cond_t 
    fbe_board_mgmt_discover_board_cond = {
        FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "fbe_board_mgmt_discover_board_cond",
        FBE_BOARD_MGMT_LIFECYCLE_COND_DISCOVER_BOARD,
        fbe_board_mgmt_discover_board_cond_function),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,     /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,         /* ACTIVATE */
        FBE_LIFECYCLE_STATE_READY,         /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)         /* DESTROY */
};

/*  FBE_BOARD_MGMT_PEER_BOOT_CHECK_COND condition -
 *  This condition call for every 30Sec. to check the whether
 *  fault expander state change. 
 */
static fbe_lifecycle_const_base_timer_cond_t fbe_board_mgmt_check_peer_boot_info_cond = {
    {
        FBE_LIFECYCLE_DEF_CONST_BASE_TIMER_COND(
            "fbe_board_mgmt_check_peer_boot_info_cond",
            FBE_BOARD_MGMT_PEER_BOOT_CHECK_COND,
            fbe_board_mgmt_peer_boot_check_cond_function),
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

static fbe_lifecycle_const_base_cond_t * FBE_LIFECYCLE_BASE_COND_ARRAY(board_mgmt)[] = {
    &fbe_board_mgmt_discover_board_cond,
    (fbe_lifecycle_const_base_cond_t*)&fbe_board_mgmt_check_peer_boot_info_cond,
};

FBE_LIFECYCLE_DEF_CONST_BASE_CONDITIONS(board_mgmt);

/*--- constant rotaries ----------------------------------------------------------------*/
static fbe_lifecycle_const_rotary_cond_t fbe_board_mgmt_specialize_rotary[] = {
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_board_mgmt_register_state_change_callback_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_board_mgmt_register_cmi_callback_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_board_mgmt_fup_register_callback_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_board_mgmt_register_resume_prom_callback_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_board_mgmt_discover_board_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
};

static fbe_lifecycle_const_rotary_cond_t fbe_board_mgmt_ready_rotary[] = {   
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_board_mgmt_fup_wait_before_upgrade_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_board_mgmt_check_peer_boot_info_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_board_mgmt_fup_release_image_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),

    /* Moved the following two conditions up. (AR651486)
     * so that work item can be removed from the queue immediately after the device gets removed.
     */
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_board_mgmt_fup_refresh_device_status_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_board_mgmt_fup_end_upgrade_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),

    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_board_mgmt_fup_wait_for_inter_device_delay_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_board_mgmt_fup_read_image_header_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),  
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_board_mgmt_fup_check_rev_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_board_mgmt_fup_read_entire_image_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_board_mgmt_fup_get_peer_permission_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_board_mgmt_fup_check_env_status_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_board_mgmt_fup_download_image_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_board_mgmt_fup_get_download_status_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_board_mgmt_fup_activate_image_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_board_mgmt_fup_get_activate_status_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_board_mgmt_fup_check_result_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    
    /* Need to put the abort condition in the end of all the fup conditions. 
     * We want to complete the execution of other condition which was already set before running the abort upgrade 
     * condition. Otherwise, when resuming the upgrade, the condition which was set before would get to run and 
     * it would make the upgrade out of sequence.
     */
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_board_mgmt_fup_abort_upgrade_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
};

static fbe_lifecycle_const_rotary_t FBE_LIFECYCLE_ROTARY_ARRAY(board_mgmt)[] = {
    FBE_LIFECYCLE_DEF_CONST_ROTARY(FBE_LIFECYCLE_STATE_SPECIALIZE, fbe_board_mgmt_specialize_rotary),
    FBE_LIFECYCLE_DEF_CONST_ROTARY(FBE_LIFECYCLE_STATE_READY, fbe_board_mgmt_ready_rotary),
};

FBE_LIFECYCLE_DEF_CONST_ROTARIES(board_mgmt);

/*--- global board mgmt lifecycle constant data ----------------------------------------*/
fbe_lifecycle_const_t FBE_LIFECYCLE_CONST_DATA(board_mgmt) = {
    FBE_LIFECYCLE_DEF_CONST_DATA(
        board_mgmt,
        FBE_CLASS_ID_BOARD_MGMT,                       /* This class */
        FBE_LIFECYCLE_CONST_DATA(base_environment))    /* The super class */
};


/*--- Condition Functions --------------------------------------------------------------*/
/*!**************************************************************
 * fbe_board_mgmt_discover_board_cond_function(fbe_base_object_t * base_object, 
 *                                             fbe_packet_t * packet)
 ****************************************************************
 * @brief
 *  This function get the board object information
 *
 * @param object_handle - This is the object handle, or in our
 * case the board_mgmt.
 * @param packet_p - The packet arriving from the monitor
 * scheduler.
 *
 * @return fbe_status_t - The status of the call to
 *                        fbe_lifecycle_class_const_verify()
 *                        for the board management's
 *                        constant lifecycle data.
 *
 * @author
 *  29-July-2010: Created  Vaibhav Gaonkar
 *
 ****************************************************************/
static fbe_lifecycle_status_t 
fbe_board_mgmt_discover_board_cond_function(fbe_base_object_t * base_object, 
                                            fbe_packet_t * packet)
{
    fbe_status_t status;
    fbe_object_id_t object_id;
    fbe_device_physical_location_t location = {0};
    fbe_board_mgmt_t *board_mgmt = (fbe_board_mgmt_t *)base_object;
    fbe_u32_t                               suitcaseCount, suitcaseIndex;
    fbe_u32_t                               bmcCount, bmcIndex;
    fbe_u32_t                               cacheCardCount, cacheCardIndex;
    fbe_u32_t                               localDimmCount, dimmIndex;
    SP_ID                                   sp = SP_A;
    fbe_u32_t                               lccIndex = 0;
    fbe_u32_t                               spCount=SP_ID_MAX;
    fbe_u32_t                               localSsdCount, ssd_index;
    fbe_object_id_t                         peObjectId = FBE_OBJECT_ID_INVALID;

    fbe_base_object_trace(base_object, 
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s entry\n",
                          __FUNCTION__);

    // Get the Board Object ID
    status = fbe_api_get_board_object_id(&object_id);
    if(status != FBE_STATUS_OK) 
    {
        fbe_base_object_trace(base_object, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s, Error in getting the Board Object ID, status: 0x%X\n",
                              __FUNCTION__, status);

        // Do not clear the condition, so that we will try later.
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    // Set SP count
    if (board_mgmt->base_environment.isSingleSpSystem == TRUE)
    {
        spCount = SP_COUNT_SINGLE;
    }
    else 
    {
        spCount = SP_ID_MAX;
    }
    
    // Set the Object ID
    board_mgmt->object_id = object_id;

    /* Platform status. The location is actually not used.  */
    fbe_board_mgmt_processPlatformStatus(board_mgmt, &location);

    /* Misc status. The location is actually not used.  */
    fbe_board_mgmt_processMiscStatus(board_mgmt);
       
    // get suitcase info if present
    status = fbe_api_board_get_suitcase_count_per_blade(object_id, &suitcaseCount);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace(base_object, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s Error getting Suitcase count, status: 0x%X\n",
                              __FUNCTION__, status);
    }
    else
    {
        board_mgmt->suitcaseCountPerBlade = suitcaseCount;
        if (board_mgmt->suitcaseCountPerBlade == 0)
        {
            fbe_base_object_trace(base_object, 
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s, suitcaseCountPerBlade %d, set CacheStatus to OK\n",
                                  __FUNCTION__, board_mgmt->suitcaseCountPerBlade);
            board_mgmt->boardCacheStatus = FBE_CACHE_STATUS_OK;
        }

        for(sp = SP_A; sp < spCount; sp ++)
        {
            for (suitcaseIndex = 0; suitcaseIndex < suitcaseCount; suitcaseIndex++)
            {
                location.sp = sp;
                location.slot = suitcaseIndex;
                status = fbe_board_mgmt_processSuitcaseStatus(board_mgmt, &location);
                if(status != FBE_STATUS_OK)
                {
                    fbe_base_object_trace(base_object, 
                                          FBE_TRACE_LEVEL_ERROR,
                                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                          "%s Error getting Suitcase Status, status: 0x%X\n",
                                          __FUNCTION__, status);
                }
            }
        }
    }

    // get BMC info if present
    status = fbe_api_board_get_bmc_count_per_blade(object_id, &bmcCount);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace(base_object, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s Error getting BMC count, status: 0x%X\n",
                              __FUNCTION__, status);
    }
    else
    {

        for(sp = SP_A; sp < spCount; sp ++)
        {
            for (bmcIndex = 0; bmcIndex < bmcCount; bmcIndex++)
            {
                location.sp = sp;
                location.slot = bmcIndex;
                status = fbe_board_mgmt_processBmcStatus(board_mgmt, &location);
                if(status != FBE_STATUS_OK)
                {
                    fbe_base_object_trace(base_object, 
                                          FBE_TRACE_LEVEL_ERROR,
                                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                          "%s Error getting BMC Status, status: 0x%X\n",
                                          __FUNCTION__, status);
                }
            }
        }
    }

    /* Get the DPE enclosure object id if it is a DPE.
     * If this is XPE or it failed to get DPE enclosure object id, the value will be FBE_OBJECT_ID_INVALID.
     */
    if(board_mgmt->platform_info.enclosureType == DPE_ENCLOSURE_TYPE)
    {
        

        status = fbe_api_get_enclosure_object_id_by_location(0, //bus
                                                             0, // enclosure
                                                             &peObjectId);
        if(status != FBE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t *)board_mgmt,
                                  FBE_TRACE_LEVEL_WARNING,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s Error in getting peObjectId for encl 0_0, status: 0x%X\n",
                                  __FUNCTION__, 
                                  status);

            board_mgmt->peObjectId = FBE_OBJECT_ID_INVALID;
        }
        else
        {
            board_mgmt->peObjectId = peObjectId;
        }
    }
    else
    {
        board_mgmt->peObjectId = FBE_OBJECT_ID_INVALID;
    }

    //get LCC info
    for (lccIndex = 0; lccIndex < spCount; lccIndex++)
    {
        location.sp = lccIndex;
        status = fbe_board_mgmt_processSPLccStatus(board_mgmt, &location);
        if(status != FBE_STATUS_NO_DEVICE)
        {
            board_mgmt->lccCount++;
        }
    }
   
    // get Cache Card info if present
    status = fbe_api_board_get_cache_card_count(object_id, &cacheCardCount);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace(base_object, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s Error getting Cache Card count, status: 0x%X\n",
                              __FUNCTION__, status);
    }
    else
    {
        board_mgmt->cacheCardCount = cacheCardCount;
        for (cacheCardIndex = 0; cacheCardIndex < cacheCardCount; cacheCardIndex++)
        {
            location.sp = board_mgmt->base_environment.spid;
            location.slot = cacheCardIndex;
            status = fbe_board_mgmt_processCacheCardStatus(board_mgmt, &location);
            if(status != FBE_STATUS_OK)
            {
                fbe_base_object_trace(base_object, 
                                      FBE_TRACE_LEVEL_ERROR,
                                      FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                      "%s Error processing Cache Card Status, status: 0x%X\n",
                                      __FUNCTION__, status);
            }
        }
    }
 
    // get DIMM info if present
    status = fbe_api_board_get_dimm_count(object_id, &localDimmCount);
    
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace(base_object, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s Error getting local DIMM count, status: 0x%X\n",
                              __FUNCTION__, status);
    }
    else
    {
        /* Get total DIMM count.*/
        if (board_mgmt->base_environment.isSingleSpSystem)
        {
            board_mgmt->dimmCount = localDimmCount; 
        }
        else
        {
            board_mgmt->dimmCount = SP_ID_MAX * localDimmCount; 
        }

        fbe_base_object_trace(base_object, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "discover_board, localDimmCount %d, total dimmCount %d\n",
                              localDimmCount, board_mgmt->dimmCount);

        /* We only knows the state of the peer DIMM through the local DIMM fault register status. 
         * we will process the local DIMM and peer DIMM status together.
         */
        for (dimmIndex = 0; dimmIndex < localDimmCount; dimmIndex++)
        {
            location.sp = board_mgmt->base_environment.spid;
            location.slot = dimmIndex;
            status = fbe_board_mgmt_processDimmStatus(board_mgmt, &location);
            if(status != FBE_STATUS_OK)
            {
                fbe_base_object_trace(base_object, 
                                      FBE_TRACE_LEVEL_ERROR,
                                      FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                      "discover_board, Error processing SP %d DIMM %d Status, status: 0x%X.\n",
                                      location.sp, location.slot, status);
            }
        }
    }

    // get SSD info if Present
    status = fbe_api_board_get_ssd_count (object_id, &localSsdCount);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace(base_object, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s Error getting local SSD count, status: 0x%X\n",
                              __FUNCTION__, status);
    }
    else
    {
        /* Get total SSD count.*/
        if (board_mgmt->base_environment.isSingleSpSystem)
        {
            board_mgmt->ssdCount = localSsdCount; 
        }
        else
        {
            board_mgmt->ssdCount = SP_ID_MAX * localSsdCount; 
        }
   
        fbe_base_object_trace(base_object, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "discover_board, localSsdCount %d, total ssdCount %d\n",
                              localSsdCount, board_mgmt->ssdCount);

        /* We only knows the state of the peer DIMM through the local DIMM fault register status. 
         * we will process the local DIMM and peer DIMM status together.
         */
        for (ssd_index = 0; ssd_index < localSsdCount; ssd_index++)
        {
            location.sp = board_mgmt->base_environment.spid;
            location.slot = ssd_index;
            status = fbe_board_mgmt_processSSDStatus(board_mgmt, &location);
            if(status != FBE_STATUS_OK)
            {
                fbe_base_object_trace(base_object, 
                                      FBE_TRACE_LEVEL_ERROR,
                                      FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                      "discover_board, Error processing SP %d SSD %d Status, status: 0x%X.\n",
                                      location.sp, location.slot, status);
            }
        }
    }

    /* Get the Fault Register state and initialized peerBootInfo*/
    fbe_board_mgmt_discoverFltRegisterInfo(board_mgmt);

    /* Get physical memory on all SP blades */
    fbe_board_mgmt_discoverSpPhysicalMemory(board_mgmt);
    
    status = fbe_lifecycle_clear_current_cond(base_object);
    if(status != FBE_STATUS_OK) 
    {
        fbe_base_object_trace(base_object, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s, can't clear current condition, status: 0x%X\n",
                              __FUNCTION__, status);
    }

    return FBE_LIFECYCLE_STATUS_DONE;
}
/********************************************************
*   end of fbe_board_mgmt_discover_board_cond_function()
*********************************************************/

/*!**************************************************************
 * fbe_board_mgmt_register_state_change_callback_cond_function(fbe_base_object_t * base_object, 
 *                                                             fbe_packet_t * packet)
 ****************************************************************
 * @brief
 *  This function registers with the physical package to get
 *  notified on Board object life cycle state changes
 *
 * @param base_object - This is the base object handle
 * @param packet - The packet arriving from the monitor
 * scheduler.
 * 
 * @return fbe_status_t - The status of the call to
 *                        fbe_lifecycle_class_const_verify() for
 *                        the Board management's constant
 *                        lifecycle data.
 *
 * @author
 *  29-July-2010: Created  Vaibhav Gaonkar
 *
 ****************************************************************/
static fbe_lifecycle_status_t
fbe_board_mgmt_register_state_change_callback_cond_function(fbe_base_object_t * base_object, 
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
                                                              (fbe_api_notification_callback_function_t)fbe_board_mgmt_state_change_handler,
							      (FBE_NOTIFICATION_TYPE_OBJECT_DATA_CHANGED | FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_DESTROY |
                                                               FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_READY),                       
                                                              (FBE_DEVICE_TYPE_MISC | FBE_DEVICE_TYPE_PLATFORM | FBE_DEVICE_TYPE_SUITCASE | 
                                                               FBE_DEVICE_TYPE_FLT_REG | FBE_DEVICE_TYPE_SLAVE_PORT | FBE_DEVICE_TYPE_BMC | FBE_DEVICE_TYPE_SP |
                                                               FBE_DEVICE_TYPE_CACHE_CARD | FBE_DEVICE_TYPE_DIMM | FBE_DEVICE_TYPE_SSD),
                                                              base_object,
                                                              FBE_TOPOLOGY_OBJECT_TYPE_BOARD | FBE_TOPOLOGY_OBJECT_TYPE_ENCLOSURE,
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
/*************************************************************************
*   end of fbe_board_mgmt_register_state_change_callback_cond_function()
**************************************************************************/

/*!**************************************************************
 * fbe_board_mgmt_register_cmi_callback_cond_function(fbe_base_object_t * base_object, 
 *                                                    fbe_packet_t * packet)
 ****************************************************************
 * @brief
 *  This function registers with the physical package to get
 *  notified on Board object life cycle state changes
 *
 * @param object_handle - This is the object handle, or in our
 * case the board_mgmt.
 * @param packet_p - The packet arriving from the monitor
 * scheduler.
 * 
 * @return fbe_status_t - The status of the call to
 *                        fbe_lifecycle_class_const_verify() for
 *                        the Board management's constant
 *                        lifecycle data.
 *
 * @author
 *  29-July-2010: Created  Vaibhav Gaonkar
 *
 ****************************************************************/
static fbe_lifecycle_status_t
fbe_board_mgmt_register_cmi_callback_cond_function(fbe_base_object_t * base_object, 
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
                                                            FBE_CMI_CLIENT_ID_BOARD_MGMT,
                                                            fbe_board_mgmt_cmi_message_handler);

    if (status != FBE_STATUS_OK) {
        fbe_base_object_trace(base_object, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s API callback init failed, status: 0x%X",
                              __FUNCTION__, status);
    }
    return FBE_LIFECYCLE_STATUS_DONE;
}
/*****************************************************************
*   end of fbe_board_mgmt_register_cmi_callback_cond_function()
******************************************************************/

/*!**************************************************************
 * fbe_board_mgmt_fup_register_callback_cond_function()
 ****************************************************************
 * @brief
 *  This function registers the callback functions with the base environment object.
 * 
 * @param base_object - This is the object handle, or in our
 * case the board_mgmt.
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
fbe_board_mgmt_fup_register_callback_cond_function(fbe_base_object_t * base_object, 
                                                 fbe_packet_t * packet)
{
	fbe_status_t status = FBE_STATUS_OK;
    fbe_base_environment_t * pBaseEnv = (fbe_base_environment_t *)base_object;

    fbe_base_object_trace(base_object, 
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s entry\n",
                          __FUNCTION__);

    fbe_base_env_set_fup_get_full_image_path_callback(pBaseEnv, &fbe_board_mgmt_fup_get_full_image_path);
    fbe_base_env_set_fup_get_manifest_file_full_path_callback(pBaseEnv, &fbe_board_mgmt_fup_get_manifest_file_full_path);
    fbe_base_env_set_fup_check_env_status_callback(pBaseEnv, &fbe_board_mgmt_fup_check_env_status);
    fbe_base_env_set_fup_info_ptr_callback(pBaseEnv, &fbe_board_mgmt_get_fup_info_ptr);
    fbe_base_env_set_fup_info_callback(pBaseEnv, &fbe_board_mgmt_get_fup_info);
    fbe_base_env_set_fup_get_firmware_rev_callback(pBaseEnv, &fbe_board_mgmt_fup_get_firmware_rev);
    fbe_base_env_set_fup_refresh_device_status_callback(pBaseEnv, NULL);
    fbe_base_env_set_fup_initiate_upgrade_callback(pBaseEnv, &fbe_board_mgmt_fup_initiate_upgrade);
    fbe_base_env_set_fup_resume_upgrade_callback(pBaseEnv, &fbe_board_mgmt_fup_resume_upgrade);
    fbe_base_env_set_fup_priority_check_callback(pBaseEnv, (void *)NULL);
    fbe_base_env_set_fup_copy_fw_rev_to_resume_prom(pBaseEnv, (void *)fbe_board_mgmt_fup_copy_fw_rev_to_resume_prom);

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
 * fbe_board_mgmt_fup_register_callback_cond_function() 
 *******************************************************************/

/*!**************************************************************
 * fbe_board_mgmt_cmi_message_handler(fbe_base_object_t *base_object, 
 *                              fbe_base_environment_cmi_message_info_t *cmi_message)
 ****************************************************************
 * @brief
 *  This function registers with the physical package to get
 *  notified on Board object life cycle state changes
 *
 * @param object_handle - This is the object handle, or in our
 * case the board_mgmt.
 * @param packet_p - The packet arriving from the monitor
 * scheduler.
 * 
 * @return fbe_status_t - The status of the call to
 *                        fbe_lifecycle_class_const_verify() for
 *                        the Board management's constant
 *                        lifecycle data.
 *
 * @author
 *  29-July-2010: Created  Vaibhav Gaonkar
 *  02-July-2012: Chengkai Implement this function to req/send board info via CMI 
 ****************************************************************/
static fbe_status_t 
fbe_board_mgmt_cmi_message_handler(fbe_base_object_t *base_object, 
                                fbe_base_environment_cmi_message_info_t *cmi_message)
{
    fbe_board_mgmt_t                   * pBoardMgmt = (fbe_board_mgmt_t *)base_object;
    fbe_board_mgmt_cmi_packet_t        * pBoardCmiPkt = NULL;
    fbe_base_environment_cmi_message_t * pBaseCmiPkt = NULL;
    fbe_status_t                       status = FBE_STATUS_OK;
    SP_ID localSp;
    fbe_device_physical_location_t peerLocation = {0};

    fbe_base_env_get_local_sp_id((fbe_base_environment_t *)pBoardMgmt, &localSp);
    peerLocation.sp = (localSp == SP_A)? SP_B : SP_A;
    if (pBoardMgmt->base_environment.processorEnclType == XPE_ENCLOSURE_TYPE)
    {
        peerLocation.bus = FBE_XPE_PSEUDO_BUS_NUM;
        peerLocation.enclosure = FBE_XPE_PSEUDO_ENCL_NUM;
    }

    if (pBoardMgmt->base_environment.processorEnclType == VPE_ENCLOSURE_TYPE)
    {
        peerLocation.bus = FBE_XPE_PSEUDO_BUS_NUM;
        peerLocation.enclosure = FBE_XPE_PSEUDO_ENCL_NUM;
    }

    // client message (if it exists)
    pBoardCmiPkt = (fbe_board_mgmt_cmi_packet_t *)cmi_message->user_message;
    pBaseCmiPkt = (fbe_base_environment_cmi_message_t *)pBoardCmiPkt;

    if(pBaseCmiPkt == NULL) 
    {
        fbe_base_object_trace(base_object, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s, Event 0x%x, no msgType.\n",
                              __FUNCTION__,
                              cmi_message->event);
    }

    switch (cmi_message->event)
    {
        case FBE_CMI_EVENT_MESSAGE_RECEIVED:
            // if peer SP went away previously, it is now back up so we need to check Fault LED and CacheStatus again
            if (pBoardMgmt->peerSpDown)
            {
                pBoardMgmt->peerSpDown = FALSE;

                status = fbe_board_mgmt_update_encl_fault_led(pBoardMgmt, &peerLocation, FBE_ENCL_FAULT_LED_PEER_SP_FLT);
                if (status != FBE_STATUS_OK)
                {
                    fbe_base_object_trace((fbe_base_object_t *)pBoardMgmt,
                                          FBE_TRACE_LEVEL_ERROR,
                                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                          "%s:error updating EnclFaultLed, status 0x%X.\n",
                                          __FUNCTION__,
                                          status);
                }
                fbe_base_environment_send_data_change_notification((fbe_base_environment_t *)pBoardMgmt, 
                                                               FBE_CLASS_ID_BOARD_MGMT, 
                                                               FBE_DEVICE_TYPE_SP, 
                                                               FBE_DEVICE_DATA_TYPE_GENERIC_INFO,
                                                               &peerLocation);
                
                fbe_base_object_trace(base_object, 
                                      FBE_TRACE_LEVEL_INFO,
                                      FBE_TRACE_MESSAGE_ID_INFO,
                                      "%s, clear peerSpDown, call checkCacheStatus\n",
                                      __FUNCTION__);
                // check if CacheStatus may have changed
                fbe_board_mgmt_checkCacheStatus(pBoardMgmt, &peerLocation); 
            }
            status = fbe_board_mgmt_processReceivedCmiMessage(pBoardMgmt, pBoardCmiPkt);
            break;

        case FBE_CMI_EVENT_PEER_NOT_PRESENT:
            status = fbe_board_mgmt_processPeerNotPresent(pBoardMgmt, pBoardCmiPkt);
            break;

        case FBE_CMI_EVENT_SP_CONTACT_LOST:
            pBoardMgmt->peerSpDown = TRUE;
            fbe_base_object_trace(base_object, 
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s, set peerSpDown\n",
                                  __FUNCTION__);
            status = fbe_board_mgmt_update_encl_fault_led(pBoardMgmt, &peerLocation, FBE_ENCL_FAULT_LED_PEER_SP_FLT);
            if (status != FBE_STATUS_OK)
            {
                fbe_base_object_trace((fbe_base_object_t *)pBoardMgmt,
                                      FBE_TRACE_LEVEL_ERROR,
                                      FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                      "%s:error updating EnclFaultLed, status 0x%X.\n",
                                      __FUNCTION__,
                                      status);
            }
            fbe_base_environment_send_data_change_notification((fbe_base_environment_t *)pBoardMgmt, 
                                                           FBE_CLASS_ID_BOARD_MGMT, 
                                                           FBE_DEVICE_TYPE_SP, 
                                                           FBE_DEVICE_DATA_TYPE_GENERIC_INFO,
                                                           &peerLocation);    
            // check if CacheStatus may have changed
            fbe_board_mgmt_checkCacheStatus(pBoardMgmt, &peerLocation); 

            status = FBE_STATUS_MORE_PROCESSING_REQUIRED;
            break;

        case FBE_CMI_EVENT_PEER_BUSY:
            status = fbe_board_mgmt_processPeerBusy(pBoardMgmt, pBoardCmiPkt);
            break;

        case FBE_CMI_EVENT_FATAL_ERROR:
            status = fbe_board_mgmt_processFatalError(pBoardMgmt, pBoardCmiPkt);
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
/************************************************
*   end of fbe_board_mgmt_cmi_message_handler()
*************************************************/
/*!**************************************************************
 * @fn fbe_board_mgmt_getCmiMsgTypeString(fbe_board_mgmt_cmi_msg_type_t boardMsgType)
 ****************************************************************
 * @brief
 *  This function gets string from board mgmt CMI message type
 *
 * @param boardMsgType - message type
 *
 * @return  - string pointer of message string
 * 
 * @author
 *  2-July-2012:  Created. Chengkai
 ****************************************************************/
static char *
fbe_board_mgmt_getCmiMsgTypeString(fbe_u32_t boardMsgType)
{
    switch (boardMsgType)
    {
        case FBE_BASE_ENV_CMI_MSG_BOARD_INFO_REQUEST:
            return "board info request";
            break;
        case FBE_BASE_ENV_CMI_MSG_BOARD_INFO_RESPONSE:
            return "board info response";
            break;
        case FBE_BASE_ENV_CMI_MSG_CACHE_STATUS_UPDATE:
            return "cache status update";
            break;
        case FBE_BASE_ENV_CMI_MSG_FUP_PEER_PERMISSION_GRANT:
            return "fup peer permissoin grant";
            break;
        case FBE_BASE_ENV_CMI_MSG_FUP_PEER_PERMISSION_DENY:
            return "fup peer permission deny";
            break;
        case FBE_BASE_ENV_CMI_MSG_FUP_PEER_PERMISSION_REQUEST:
            return "fup peer permission request";
        default:
            break;
    }
    return "invalid board CMI message";
}
/************************************************
*   end of fbe_board_mgmt_getCmiMsgTypeString()
*************************************************/
/*!**************************************************************
 * fbe_board_mgmt_sendCmiMessage()
 ****************************************************************
 * @brief
 *  This function sends the specified BOARD Mgmt message to the
 *  peer SP.
 *
 * @param object_handle - This is the object handle, or in our
 * case the board_mgmt.
 * @param packet_p - The packet arriving from the monitor
 * scheduler.
 *
 * @return fbe_status_t - The status of the call to
 *                        fbe_base_environment_send_cmi_message()
 *
 * @author
 *  2-July-2012:  Created. Chengkai
 *
 ****************************************************************/
static fbe_status_t
fbe_board_mgmt_sendCmiMessage(fbe_board_mgmt_t *board_mgmt, 
                              fbe_u32_t boardMsgType)
{
    fbe_board_mgmt_cmi_msg_t    boardMsgPkt;
    fbe_status_t                status;

    // format the message
    boardMsgPkt.baseCmiMsg.opcode = boardMsgType;
    boardMsgPkt.spPhysicalMemorySize = fbe_board_mgmt_get_local_sp_physical_memory();


    // send the message to peer SP
    status = fbe_base_environment_send_cmi_message((fbe_base_environment_t *)board_mgmt,
                                                   sizeof(fbe_board_mgmt_cmi_msg_t),
                                                   &boardMsgPkt);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)board_mgmt, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s Error sending %s(%d) to peer, status: 0x%X\n",
                              __FUNCTION__, 
                              fbe_board_mgmt_getCmiMsgTypeString(boardMsgType),
                              boardMsgType,
                              status);
    }
    else
    {
        fbe_base_object_trace((fbe_base_object_t *)board_mgmt, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s sent %s(%d) to peer\n",
                              __FUNCTION__, 
                              fbe_board_mgmt_getCmiMsgTypeString(boardMsgType),
                              boardMsgType);
    }

    return status;
}
/******************************************************
 * end fbe_board_mgmt_sendCmiMessage() 
 ******************************************************/

/*!**************************************************************
 * @fn fbe_board_mgmt_processReceivedCmiMessage(fbe_board_mgmt_t * pBoardMgmt, 
 *                                            fbe_board_mgmt_cmi_packet_t *pBoardCmiPkt)
 ****************************************************************
 * @brief
 *  This function gets called when recieving the CMI message with 
 *  FBE_CMI_EVENT_MESSAGE_RECEIVED.
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
 *  2-July-2012:  Created. Chengkai
 ****************************************************************/
static fbe_status_t 
fbe_board_mgmt_processReceivedCmiMessage(fbe_board_mgmt_t * board_mgmt, 
                                         fbe_board_mgmt_cmi_packet_t *pBoardCmiPkt)
{
    fbe_status_t                         status = FBE_STATUS_OK;
    fbe_base_environment_cmi_message_t   * pBaseCmiMsg = (fbe_base_environment_cmi_message_t *)pBoardCmiPkt;
    fbe_board_mgmt_cmi_msg_t             * cmiMsgPtr = (fbe_board_mgmt_cmi_msg_t *)pBoardCmiPkt;
    fbe_base_environment_t             * pBaseEnv = (fbe_base_environment_t *)board_mgmt;
    fbe_base_env_fup_cmi_packet_t      * pFupCmiPkt = (fbe_base_env_fup_cmi_packet_t *)pBoardCmiPkt;

    fbe_base_object_trace((fbe_base_object_t *)board_mgmt, 
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s, msgType: %s(0x%llX)\n",
                          __FUNCTION__,
                          fbe_board_mgmt_getCmiMsgTypeString(pBaseCmiMsg->opcode),
                          (unsigned long long)pBaseCmiMsg->opcode);

    switch (pBaseCmiMsg->opcode)
    {
        case FBE_BASE_ENV_CMI_MSG_BOARD_INFO_REQUEST:
            // peer is requesting our local BOARD info, so send it
            status = fbe_board_mgmt_sendCmiMessage(board_mgmt, FBE_BASE_ENV_CMI_MSG_BOARD_INFO_RESPONSE);
            if (status != FBE_STATUS_OK)
            {
                fbe_base_object_trace((fbe_base_object_t *)board_mgmt, 
                                      FBE_TRACE_LEVEL_ERROR,
                                      FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                      "%s Error sending BOARD Info response to peer, status: 0x%X\n",
                                      __FUNCTION__, status);
            }

            // if peer requesting message carries the memory size, update it in local board mgmt
            if (cmiMsgPtr->spPhysicalMemorySize != 0)
            {
                fbe_board_mgmt_updatePeerPhysicalMemory(board_mgmt, cmiMsgPtr);
            }
            break;

        case FBE_BASE_ENV_CMI_MSG_BOARD_INFO_RESPONSE:
            // receive peer board info
            fbe_board_mgmt_updatePeerPhysicalMemory(board_mgmt, cmiMsgPtr);
            break;

        case FBE_BASE_ENV_CMI_MSG_CACHE_STATUS_UPDATE:
        status = fbe_board_mgmt_process_peer_cache_status_update(board_mgmt, 
                                                                 pBoardCmiPkt);
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
            status = fbe_board_mgmt_fup_new_contact_init_upgrade(board_mgmt, FBE_DEVICE_TYPE_SP);
            break;

        default:
            status = FBE_STATUS_MORE_PROCESSING_REQUIRED;
            break;
    }

    return(status);
}
/************************************************
*   end of fbe_board_mgmt_processReceivedCmiMessage()
*************************************************/

/*!**************************************************************
 *  @fn fbe_board_mgmt_processPeerNotPresent(fbe_board_mgmt_t * pBoardMgmt,
 *                                        fbe_board_mgmt_cmi_packet_t * pBoardCmiPkt)
 ****************************************************************
 * @brief
 *  This function gets called when recieving the CMI message with 
 *  FBE_CMI_EVENT_PEER_NOT_PRESENT.
 *
 * @param pBoardMgmt - 
 * @param pBoardCmiPkt - pointer to user message of CMI message info.
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
fbe_board_mgmt_processPeerNotPresent(fbe_board_mgmt_t * pBoardMgmt, 
                                    fbe_board_mgmt_cmi_packet_t * pBoardCmiPkt)
{
    fbe_base_environment_cmi_message_t * pBaseCmiMsg = (fbe_base_environment_cmi_message_t *)pBoardCmiPkt;
    fbe_base_env_fup_cmi_packet_t      * pFupCmiPkt = (fbe_base_env_fup_cmi_packet_t *)pBoardCmiPkt;
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
            status = fbe_base_env_fup_processDeniedPeerPerm((fbe_base_environment_t *)pBoardMgmt,
                                                         pFupCmiPkt->pRequestorWorkItem);
     
            break;


        default:
            status = FBE_STATUS_MORE_PROCESSING_REQUIRED;
            break;
    }

    return(status);
}



/*!**************************************************************
 * @fn fbe_ps_mgmt_processPeerBusy(fbe_ps_mgmt_t * pBoardMgmt, 
 *                          fbe_ps_mgmt_cmi_packet_t * pBoardCmiPkt)
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
fbe_board_mgmt_processPeerBusy(fbe_board_mgmt_t * pBoardMgmt, 
                              fbe_board_mgmt_cmi_packet_t * pBoardCmiPkt)
{
    fbe_base_environment_cmi_message_t * pBaseCmiMsg = (fbe_base_environment_cmi_message_t *)pBoardCmiPkt;
    fbe_base_env_fup_cmi_packet_t      * pFupCmiPkt = (fbe_base_env_fup_cmi_packet_t *)pBoardCmiPkt;
    fbe_status_t                         status = FBE_STATUS_OK;

    switch (pBaseCmiMsg->opcode)
    {
        case FBE_BASE_ENV_CMI_MSG_FUP_PEER_PERMISSION_GRANT:
        case FBE_BASE_ENV_CMI_MSG_FUP_PEER_PERMISSION_DENY:
            fbe_base_object_trace((fbe_base_object_t *)pBoardMgmt, 
                          FBE_TRACE_LEVEL_ERROR,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s, should never happen, opcode 0x%x\n",
                          __FUNCTION__,
                          pBaseCmiMsg->opcode);

            status = FBE_STATUS_GENERIC_FAILURE;
            break;

        case FBE_BASE_ENV_CMI_MSG_FUP_PEER_PERMISSION_REQUEST:
            // peer SP is not present when sending the message to request permission.
            status = fbe_base_env_fup_peerPermRetry((fbe_base_environment_t *)pBoardMgmt, 
                                                    pFupCmiPkt->pRequestorWorkItem);
            break;

        default:
            status = FBE_STATUS_MORE_PROCESSING_REQUIRED;
            break;
    }

    return(status);
}

/*!**************************************************************
 * @fn fbe_board_mgmt_processFatalError(fbe_board_mgmt_t * pBoardMgmt, 
 *                                     fbe_ps_mgmt_cmi_packet_t * pBoardCmiPkt)
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
fbe_board_mgmt_processFatalError(fbe_board_mgmt_t * pBoardMgmt, 
                                fbe_board_mgmt_cmi_packet_t * pBoardCmiPkt)
{
    fbe_base_environment_cmi_message_t * pBaseCmiMsg = (fbe_base_environment_cmi_message_t *)pBoardCmiPkt;
    fbe_base_env_fup_cmi_packet_t      * pFupCmiPkt = (fbe_base_env_fup_cmi_packet_t *)pBoardCmiPkt;
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
            status = fbe_base_env_fup_processDeniedPeerPerm((fbe_base_environment_t *)pBoardMgmt,
                                                         pFupCmiPkt->pRequestorWorkItem);
            break;

        default:
            status = FBE_STATUS_MORE_PROCESSING_REQUIRED;
            break;
    }

    return(status);
}

/*!**************************************************************
 * fbe_board_mgmt_get_sp_lcc_info()
 ****************************************************************
 * @brief
 *  This function get lcc info for SP board from physical package.
 *  DPE's SP board is also treated as lcc in physical package enclosure object.
 *
 * @param board_mgmt - object handle
 * @param pLccInfo - pointer to save Lcc info
 *
 * @return fbe_status_t - FBE Status
 *
 * @author
 *  31-Oct-2012 - Created. Rui Chang
 *
 ****************************************************************/
static fbe_status_t 
fbe_board_mgmt_get_sp_lcc_info(fbe_board_mgmt_t *board_mgmt,
                                                         fbe_lcc_info_t * pLccInfo,
                                                         fbe_u8_t slot)
{
    fbe_device_physical_location_t          location = {0};
    fbe_status_t                            status = FBE_STATUS_OK;

    location.bus = 0;
    location.enclosure = 0;
    location.slot = slot;

    status = fbe_api_enclosure_get_lcc_info(&location, pLccInfo);
    if (status != FBE_STATUS_OK) 
    {
        fbe_base_object_trace((fbe_base_object_t *)board_mgmt, 
                                  FBE_TRACE_LEVEL_WARNING,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s Error getting SP's LCC info for slot %d, status: 0x%X.\n",
                                  __FUNCTION__,
                                  slot,
                                  status);
        return status;
    }

    return FBE_STATUS_OK;

}
/**************************************
 * end fbe_board_mgmt_get_sp_lcc_info() 
 **************************************/  

/*!**************************************************************
 * fbe_board_mgmt_processSPLccStatus()
 ****************************************************************
 * @brief
 *  This function processes the LCC status change at
 *  startup time discovery or at runtime.
 *
 * @param board_mgmt - object handle
 * @param pLccInfo - pointer to save Lcc info
 *
 * @return fbe_status_t - FBE Status
 *
 * @author
 *  31-Oct-2012 - Created. Rui Chang
 *
 ****************************************************************/

static fbe_status_t 
fbe_board_mgmt_processSPLccStatus(fbe_board_mgmt_t *board_mgmt,
                                  fbe_device_physical_location_t *pLocation)
{
    fbe_base_object_t                       *base_object = (fbe_base_object_t *)board_mgmt;
    SP_ID                                   spid = board_mgmt->base_environment.spid;
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_lcc_info_t                          oldLccInfo={0};
    fbe_lcc_info_t                          newLccInfo={0};

    /* We only deal with the DPE's LCC, ie, encl 0 bus 0 */
    if (pLocation->bus != 0 || pLocation->enclosure != 0 || board_mgmt->platform_info.enclosureType != DPE_ENCLOSURE_TYPE)
    {
        return FBE_STATUS_OK;
    }

    oldLccInfo = board_mgmt->lccInfo[pLocation->sp];

    status = fbe_board_mgmt_get_sp_lcc_info(board_mgmt, &newLccInfo, pLocation->sp);
    if (status != FBE_STATUS_OK) 
    {
        fbe_base_object_trace(base_object, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s Error getting %s's LCC info, status: 0x%X.\n",
                              __FUNCTION__,
                              ((pLocation->sp) == SP_A)? "SPA" : "SPB",
                              status);
        return status;
    }

    /* Only interested in LCC's on SP.
     * The LCC on the backend module will be handled in module mgmt.
     */
    if (newLccInfo.lccActualHwType != FBE_DEVICE_TYPE_SP)
    {
        return FBE_STATUS_NO_DEVICE;
    }

    board_mgmt->lccInfo[pLocation->sp] = newLccInfo;    

    status = fbe_board_mgmt_fup_handle_sp_lcc_status_change(board_mgmt, 
                                                        pLocation, 
                                                        &board_mgmt->lccInfo[pLocation->sp], 
                                                        &oldLccInfo);
    if(status != FBE_STATUS_OK) 
    {
        fbe_base_object_trace(base_object, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "FUP:%s fbe_board_mgmt_fup_handle_sp_lcc_status_change failed for lcc %d, status 0x%X.\n",
                              (spid == SP_A)? "SPA" : "SPB",
                              pLocation->sp,
                              status);
    }

    return status;

}
/**************************************
 * end fbe_board_mgmt_processSPLccStatus() 
 **************************************/  

/*!**************************************************************
 * fbe_board_mgmt_state_change_handler(update_object_msg_t *update_object_msg, 
 *                                     void *context)
 ****************************************************************
 * @brief
 *  This is the handler function for state change notification.
 *
 * @param update_object_msg - 
 * @param context - 
 *
 * @return fbe_status_t - The status of the call to
 *                        fbe_lifecycle_class_const_verify()
 *                        for the board management's
 *                        constant lifecycle data.
 * 
 * 
 * @author
 *  29-July-2010: Created  Vaibhav Gaonkar
 *
 ****************************************************************/
static fbe_status_t 
fbe_board_mgmt_state_change_handler(update_object_msg_t *update_object_msg, 
                                    void *context)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_board_mgmt_t    *board_mgmt;
    fbe_base_object_t   *base_object;
    fbe_notification_info_t *notification_info = &update_object_msg->notification_info;
 
    board_mgmt = (fbe_board_mgmt_t *)context;
    base_object = (fbe_base_object_t *)board_mgmt;
        
    fbe_base_object_trace(base_object, 
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s entry\n",
                          __FUNCTION__);

    /*
     * Process the notification
     */
    switch (notification_info->notification_type)
    {
        case FBE_NOTIFICATION_TYPE_OBJECT_DATA_CHANGED:

            status = fbe_board_mgmt_handle_object_data_change(board_mgmt, update_object_msg);
            break;

        case FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_DESTROY:
            status = fbe_board_mgmt_handle_destroy_state(board_mgmt, update_object_msg);
            break;

        case FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_READY:
            status = fbe_board_mgmt_handle_ready_state(board_mgmt, update_object_msg);
            break;

        default:
            break;
    }
    return FBE_STATUS_OK;
}
/**********************************************
* end of fbe_board_mgmt_state_change_handler()
***********************************************/


/*!**************************************************************
 * @fn fbe_board_mgmt_handle_object_data_change()
 ****************************************************************
 * @brief
 *  This function is to handle the object data change.
 *
 * @param pBoardMgmt - This is the object handle, or in our case the board_mgmt.
 * @param pUpdateObjectMsg - Pointer to the notification info
 *
 * @return fbe_status_t - FBE Status
 * 
 * @author
 *  26-Jun-2013:  PHE - Seperated from fbe_board_mgmt_state_change_handler.
 *
 ****************************************************************/
static fbe_status_t fbe_board_mgmt_handle_object_data_change(fbe_board_mgmt_t * pBoardMgmt, 
                                                          update_object_msg_t * pUpdateObjectMsg)
{
    fbe_notification_data_changed_info_t   * pDataChangedInfo = NULL;
    fbe_status_t                             status = FBE_STATUS_OK;

    pDataChangedInfo = &(pUpdateObjectMsg->notification_info.notification_data.data_change_info);
    
    fbe_base_object_trace((fbe_base_object_t *)pBoardMgmt, 
                          FBE_TRACE_LEVEL_INFO,//FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry, dataType 0x%x.\n",
                          __FUNCTION__, pDataChangedInfo->data_type);
    
    switch(pDataChangedInfo->data_type)
    {
        case FBE_DEVICE_DATA_TYPE_GENERIC_INFO:
            status = fbe_board_mgmt_handle_generic_info_change(pBoardMgmt, pUpdateObjectMsg);
            break;

        case FBE_DEVICE_DATA_TYPE_RESUME_PROM_INFO:
            status = fbe_board_mgmt_resume_prom_handle_resume_prom_info_change(pBoardMgmt,
                                                                    pDataChangedInfo->device_mask,
                                                                    &(pDataChangedInfo->phys_location));
            break;

        case FBE_DEVICE_DATA_TYPE_FUP_INFO:
            status = fbe_base_env_fup_handle_firmware_status_change((fbe_base_environment_t *)pBoardMgmt, 
                                                                   pDataChangedInfo->device_mask,
                                                                   &pDataChangedInfo->phys_location);
            break;

        default:
            fbe_base_object_trace((fbe_base_object_t *)pBoardMgmt, 
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
 * @fn fbe_board_mgmt_handle_generic_info_change()
 ****************************************************************
 * @brief
 *  This function is to handle the generic info change.
 *
 * @param board_mgmt - This is the object handle, or in our case the board_mgmt.
 * @param pUpdateObjectMsg - Pointer to the notification info
 *
 * @return fbe_status_t - FBE Status
 * 
 * @author
 *  26-Jun-2013:  PHE - Seperated from fbe_board_mgmt_state_change_handler.
 *
 ****************************************************************/
static fbe_status_t fbe_board_mgmt_handle_generic_info_change(fbe_board_mgmt_t * board_mgmt, 
                                                              update_object_msg_t * pUpdateObjectMsg)
{
    fbe_status_t                             status = FBE_STATUS_OK;
    fbe_notification_info_t                * notification_info = &pUpdateObjectMsg->notification_info;
    fbe_device_physical_location_t          location = {0};

    fbe_base_object_trace((fbe_base_object_t *)board_mgmt, 
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry, device_mask 0x%llx.\n",
                          __FUNCTION__, notification_info->notification_data.data_change_info.device_mask);
 
    switch (notification_info->notification_data.data_change_info.device_mask)
    {
        case FBE_DEVICE_TYPE_PLATFORM :
            status = fbe_board_mgmt_processPlatformStatus(board_mgmt,
                                                       &(notification_info->notification_data.data_change_info.phys_location));
            break;

        case FBE_DEVICE_TYPE_MISC:
            status = fbe_board_mgmt_processMiscStatus(board_mgmt);
            break;

        case FBE_DEVICE_TYPE_SUITCASE:
            status = fbe_board_mgmt_processSuitcaseStatus(board_mgmt,
                                                          &(notification_info->notification_data.data_change_info.phys_location));
            break;

        case FBE_DEVICE_TYPE_BMC:
            status = fbe_board_mgmt_processBmcStatus(board_mgmt,
                                                     &(notification_info->notification_data.data_change_info.phys_location));
            break;

       case FBE_DEVICE_TYPE_SLAVE_PORT:
            status = fbe_board_mgmt_processSlavePortStatus(board_mgmt,
                                                               &(notification_info->notification_data.data_change_info.phys_location));
            break;

        case FBE_DEVICE_TYPE_SP:
            /* Currently DPE like beachcobmer will send this type of notification when doing LCC upgrade */
            location = notification_info->notification_data.data_change_info.phys_location;
            status = fbe_board_mgmt_processSPLccStatus(board_mgmt, &location);
            break;

        case FBE_DEVICE_TYPE_CACHE_CARD:
            status = fbe_board_mgmt_processCacheCardStatus(board_mgmt,
                                                       &(notification_info->notification_data.data_change_info.phys_location));
            break;

        case FBE_DEVICE_TYPE_DIMM:
            status = fbe_board_mgmt_processDimmStatus(board_mgmt,
                                                       &(notification_info->notification_data.data_change_info.phys_location));
            break;

        case FBE_DEVICE_TYPE_SSD:
            status = fbe_board_mgmt_processSSDStatus(board_mgmt, 
                                                      &(notification_info->notification_data.data_change_info.phys_location));
            break;


        case FBE_DEVICE_TYPE_FLT_REG:
            status = fbe_board_mgmt_processFaultRegisterStatus(board_mgmt,
                                                               &(notification_info->notification_data.data_change_info.phys_location));
            break;
 
        default:
            break;
    }

    if(status != FBE_STATUS_OK) 
    {
        fbe_base_object_trace((fbe_base_object_t *)board_mgmt, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s Error processing %s Status, status: 0x%X\n",
                              __FUNCTION__, 
                              fbe_base_env_decode_device_type(notification_info->notification_data.data_change_info.device_mask),
                              status);
    }
    return status;
}// End of function fbe_encl_mgmt_handle_generic_info_change



/*!**************************************************************
 * fbe_board_mgmt_processPlatformStatus(fbe_board_mgmt_t *board_mgmt)
 ****************************************************************
 * @brief
 *  This is to process any platform status change. 
 *  It is called at the discovery time and runtime.
 *
 * @param board_mgmt - This handle to board mgmt object 
 * @param pLocation -
 *
 * @return fbe_status_t - 
 * 
 * @author
 *  29-July-2010: Created  Vaibhav Gaonkar
 *
 ****************************************************************/
static fbe_status_t
fbe_board_mgmt_processPlatformStatus(fbe_board_mgmt_t *board_mgmt,
                             fbe_device_physical_location_t *pLocation)
{
    fbe_status_t status;
    fbe_board_mgmt_platform_info_t current_platform_info;

    // Get platform_info
    status =  fbe_api_board_get_platform_info(board_mgmt->object_id, &current_platform_info);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)board_mgmt, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s Error getting Platform Info Status, status: 0x%X\n",
                              __FUNCTION__, status);

        return status;
    }
    
    if(board_mgmt->platform_info.platformType != current_platform_info.hw_platform_info.platformType)
    {
        board_mgmt->platform_info = current_platform_info.hw_platform_info;

        /* Send notification for data change */
        fbe_base_environment_send_data_change_notification((fbe_base_environment_t *) board_mgmt, 
                                                           FBE_CLASS_ID_BOARD_MGMT, 
                                                           FBE_DEVICE_TYPE_PLATFORM,
                                                           FBE_DEVICE_DATA_TYPE_GENERIC_INFO,
                                                           pLocation);
    }

    return status;
}
/**********************************************
* end of fbe_board_processPlatformStatus()
***********************************************/


/*!**************************************************************
 * fbe_board_mgmt_processMiscStatus(fbe_board_mgmt_t *board_mgmt)
 ****************************************************************
 * @brief
 *  This is to process any platform status change.
 *  It is called at the discovery time and runtime.
 *
 * @param board_mgmt - This handle to board mgmt object 
 *
 * @return fbe_status_t - 
 * 
 * @author
 *  29-July-2010: Created  Vaibhav Gaonkar
 *
 ****************************************************************/
static fbe_status_t
fbe_board_mgmt_processMiscStatus(fbe_board_mgmt_t *pBoardMgmt)
{
    fbe_status_t status;
    fbe_board_mgmt_misc_info_t     current_misc_info;
    SP_ID                          localSp;
    fbe_device_physical_location_t spLocation = {0};
    fbe_device_physical_location_t peerSpLocation = {0};
    fbe_device_physical_location_t location = {0};
    fbe_u8_t                       deviceStr[FBE_DEVICE_STRING_LENGTH];

    fbe_base_env_get_local_sp_id((fbe_base_environment_t *)pBoardMgmt, &localSp);
    spLocation.sp = (localSp == SP_A)? SP_A : SP_B;
    peerSpLocation.sp = (localSp == SP_A)? SP_B : SP_A;

    if(!pBoardMgmt->spInserted)
    {
        pBoardMgmt->spInserted = FBE_TRUE;

        status = fbe_board_mgmt_initiate_resume_prom_cmd(pBoardMgmt,
                                                      FBE_DEVICE_TYPE_SP,
                                                      &spLocation); 
    }

    status = fbe_api_board_get_misc_info(pBoardMgmt->object_id, &current_misc_info);
    
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)pBoardMgmt, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s Error getting misc info, status: 0x%X\n",
                              __FUNCTION__, status);

        return status;
    }

    pBoardMgmt->engineIdFault = current_misc_info.engineIdFault;
    pBoardMgmt->UnsafeToRemoveLED = current_misc_info.UnsafeToRemoveLED;

    if((!current_misc_info.peerInserted) && (pBoardMgmt->peerInserted) &&
       (pBoardMgmt->base_environment.isSingleSpSystem == FALSE))
    {
        /* The peer SP was just removed. */
        fbe_base_object_trace((fbe_base_object_t *)pBoardMgmt, 
                              FBE_TRACE_LEVEL_INFO, 
                              FBE_TRACE_MESSAGE_ID_INFO, 
                              "Peer SP was removed.\n");

        fbe_event_log_write(ESP_ERROR_PEER_SP_REMOVED, 
                                 NULL, 
                                 0, 
                                 "%s",
                                 decodeSpId(peerSpLocation.sp));

        pBoardMgmt->peerInserted = current_misc_info.peerInserted;
        
        /* The SP was removed. */
        status = fbe_base_env_resume_prom_handle_device_removal((fbe_base_environment_t *)pBoardMgmt, 
                                                                FBE_DEVICE_TYPE_SP, 
                                                                &peerSpLocation);
        if(status != FBE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t *)pBoardMgmt,
                                  FBE_TRACE_LEVEL_WARNING,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "RP: Failed to handle device removal for %s.\n",
                                  (peerSpLocation.sp == SP_A) ? "SPA" : "SPB");

        }
        fbe_base_environment_send_data_change_notification((fbe_base_environment_t *)pBoardMgmt, 
                                                           FBE_CLASS_ID_BOARD_MGMT, 
                                                           FBE_DEVICE_TYPE_SP, 
                                                           FBE_DEVICE_DATA_TYPE_GENERIC_INFO,
                                                           &peerSpLocation);    
    }
    else if((current_misc_info.peerInserted) && (!pBoardMgmt->peerInserted) &&
            (pBoardMgmt->base_environment.isSingleSpSystem == FALSE))
    {
        /* Peer SP was just inserted. */
        /* The peer SP was just removed. */
        fbe_base_object_trace((fbe_base_object_t *)pBoardMgmt, 
                              FBE_TRACE_LEVEL_INFO, 
                              FBE_TRACE_MESSAGE_ID_INFO, 
                              "Peer SP was inserted.\n");

        fbe_event_log_write(ESP_INFO_PEER_SP_INSERTED, 
                                 NULL, 
                                 0, 
                                 "%s",
                                 decodeSpId(peerSpLocation.sp));

        pBoardMgmt->peerInserted = current_misc_info.peerInserted;

        status = fbe_board_mgmt_initiate_resume_prom_cmd(pBoardMgmt,
                                                      FBE_DEVICE_TYPE_SP,
                                                      &peerSpLocation); 
        
        if(status != FBE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t *)pBoardMgmt,
                                  FBE_TRACE_LEVEL_WARNING,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "RP: Failed to initiate resume prom read for %s.\n",
                                  (peerSpLocation.sp == SP_A) ? "SPA" : "SPB");
        }
    }

    // generate the device string (used for tracing & events)
    location.sp = current_misc_info.spid;
    status = fbe_base_env_create_device_string(FBE_DEVICE_TYPE_SP,
                                               &location,
                                               &deviceStr[0],
                                               FBE_DEVICE_STRING_LENGTH);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)pBoardMgmt,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s, Failed to create device string.\n",
                              __FUNCTION__);
    }

    if (pBoardMgmt->lowBattery != current_misc_info.lowBattery)
    {
        if (current_misc_info.lowBattery)
        {
            fbe_event_log_write(ESP_ERROR_LOW_BATTERY_FAULT_DETECTED,
                                NULL, 0,
                                "%s",
                                &deviceStr[0]);
        }
        else
        {
            fbe_event_log_write(ESP_INFO_LOW_BATTERY_FAULT_CLEARED,
                                NULL, 0,
                                "%s",
                                &deviceStr[0]);
        }
        
        pBoardMgmt->lowBattery = current_misc_info.lowBattery;

        fbe_board_mgmt_calculateSuitcaseStateInfo(pBoardMgmt, 
                                                  pBoardMgmt->base_environment.spid,
                                                  0);    

        // check if Enclosure Fault LED needs updating
        status = fbe_board_mgmt_update_encl_fault_led(pBoardMgmt, &location, FBE_ENCL_FAULT_LED_SP_FLT); 

    }
    if (current_misc_info.fbeInternalCablePort1 != pBoardMgmt->fbeInternalCablePort1)
    {
        fbe_base_object_trace((fbe_base_object_t *)pBoardMgmt,
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "processMISCstatus, internalCableStat changed from %d to %d.\n",
                                  pBoardMgmt->fbeInternalCablePort1,
                                  current_misc_info.fbeInternalCablePort1);
        pBoardMgmt->fbeInternalCablePort1 = current_misc_info.fbeInternalCablePort1;
        fbe_board_mgmt_calculateSuitcaseStateInfo(pBoardMgmt, 
                                                  pBoardMgmt->base_environment.spid,
                                                  0); 
        // update fault led
        fbe_base_object_trace((fbe_base_object_t *)pBoardMgmt,
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "update faultled .\n");
        status = fbe_board_mgmt_update_encl_fault_led(pBoardMgmt, &location, FBE_ENCL_FAULT_LED_INTERNAL_CABLE_FLT); 

        if (status != FBE_STATUS_OK) 
        {
            fbe_base_object_trace((fbe_base_object_t *)pBoardMgmt,
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s, error updating EnclFaultLed, status 0x%X.\n",
                                  &deviceStr[0],
                                  status);
        }

    }
    return status; 
}

/**********************************************
* end of fbe_board_mgmt_processMiscStatus()
***********************************************/

/*!**************************************************************
 * @fn fbe_board_mgmt_update_encl_fault_led(fbe_board_mgmt_t *board_mgmt,
                                    fbe_device_physical_location_t *pLocation,
                                    fbe_encl_fault_led_reason_t enclFaultLedReason)
 ****************************************************************
 * @brief
 *  This function checks the status to determine
 *  if the Enclosure Fault LED needs to be updated.
 *
 * @param board_mgmt -
 * @param pLocation - The location of the enclosure.
 * @param enclFaultLedReason - The enclosure fault led reason.
 *
 * @return fbe_status_t - 
 *
 * @author
 *  05-Dec-2012:  Created. Eric Zhou
 *  02-Apr-2013:  Modified. Lin Lou
 *
 ****************************************************************/
fbe_status_t fbe_board_mgmt_update_encl_fault_led(fbe_board_mgmt_t *board_mgmt,
                                    fbe_device_physical_location_t *pLocation,
                                    fbe_encl_fault_led_reason_t enclFaultLedReason)
{
    fbe_status_t                                status = FBE_STATUS_OK;
    fbe_bool_t                                  enclFaultLedOn = FBE_FALSE;
    fbe_object_id_t                             objectId = FBE_OBJECT_ID_INVALID;
    fbe_u32_t                                   sp;
    fbe_u32_t                                   dimmIndex = 0;

    FBE_UNREFERENCED_PARAMETER(pLocation);

    switch(enclFaultLedReason) 
    {
    case FBE_ENCL_FAULT_LED_SP_FLT:
        if (board_mgmt->lowBattery)
        {
            enclFaultLedOn = FBE_TRUE;
        }
        break;

    case FBE_ENCL_FAULT_LED_INTERNAL_CABLE_FLT:
        if ((board_mgmt->fbeInternalCablePort1 == FBE_CABLE_STATUS_CROSSED) ||
            (board_mgmt->fbeInternalCablePort1 == FBE_CABLE_STATUS_MISSING))
        {
            enclFaultLedOn = FBE_TRUE;
        }
        break;

    case FBE_ENCL_FAULT_LED_PEER_SP_FLT:
        if (!fbe_cmi_is_peer_alive() && board_mgmt->base_environment.isSingleSpSystem == FALSE)
        {
            enclFaultLedOn = FBE_TRUE;
        }
        break;

    case FBE_ENCL_FAULT_LED_SP_FAULT_REG_FLT:
        /* In case fault is cleared but additional reboot not executed on peer SP,
         * the biosPostFail is set, then lit SP enclosure LED.
         */
        if ((board_mgmt->peerBootInfo.biosPostFail == FBE_TRUE) ||
            fbe_board_mgmt_realFltRegFaultDetected(board_mgmt) ||
            (board_mgmt->peerBootInfo.peerBootState == FBE_PEER_HUNG))
        {
            enclFaultLedOn = FBE_TRUE;
        }
        if(enclFaultLedOn)
        {
            fbe_base_object_trace((fbe_base_object_t *)board_mgmt, 
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "PBL: Light SP enclosure LED:biosPostFail:%d, anyFaultsFound:%d.Hung:%d\n",
                                  board_mgmt->peerBootInfo.biosPostFail,
                                  board_mgmt->peerBootInfo.fltRegStatus.anyFaultsFound,
                                  (board_mgmt->peerBootInfo.peerBootState == FBE_PEER_HUNG));
        }
        break;

    case FBE_ENCL_FAULT_LED_SP_RESUME_PROM_FLT:
        for(sp = SP_A; sp < SP_ID_MAX; sp++)
        {
            enclFaultLedOn = fbe_base_env_get_resume_prom_fault(board_mgmt->resume_info[sp].op_status);
            if(enclFaultLedOn)
            {
                break;
            }
        }
        break;

    case FBE_ENCL_FAULT_LED_DIMM_FLT:
        for(dimmIndex = 0; dimmIndex < board_mgmt->dimmCount; dimmIndex ++) 
        {
            if(board_mgmt->dimmInfo[dimmIndex].state == FBE_DIMM_STATE_FAULTED ||
               board_mgmt->dimmInfo[dimmIndex].state == FBE_DIMM_STATE_MISSING ||
               board_mgmt->dimmInfo[dimmIndex].state == FBE_DIMM_STATE_NEED_TO_REMOVE)
            {
                enclFaultLedOn = FBE_TRUE;
                break;
            }
        }
        break;

    default:
        fbe_base_object_trace((fbe_base_object_t *)board_mgmt,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s, unsupported enclFaultLedReason 0x%llX.\n",
                              __FUNCTION__,
                              enclFaultLedReason);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_get_board_object_id(&objectId);
    if (status != FBE_STATUS_OK) 
    {
        fbe_base_object_trace((fbe_base_object_t *)board_mgmt,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s Error in getting the Board Object ID, status: 0x%X\n",
                              __FUNCTION__, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_base_environment_updateEnclFaultLed((fbe_base_object_t *)board_mgmt,
                                                     objectId,
                                                     enclFaultLedOn,
                                                     enclFaultLedReason);

    if (status == FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)board_mgmt, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s, %s enclFaultLedReason %s.\n",
                              board_mgmt->base_environment.spid == SP_A ? "SPA" : "SPB",
                              enclFaultLedOn? "SET" : "CLEAR",
                              fbe_base_env_decode_encl_fault_led_reason(enclFaultLedReason));
    }
    else
    {
        fbe_base_object_trace((fbe_base_object_t *)board_mgmt, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s, error in %s enclFaultLedReason %s.\n",
                              board_mgmt->base_environment.spid == SP_A ? "SPA" : "SPB",
                              enclFaultLedOn? "SET" : "CLEAR",
                              fbe_base_env_decode_encl_fault_led_reason(enclFaultLedReason));
    }

    return status;
}

/*!**************************************************************
 * fbe_board_mgmt_register_resume_prom_callback_cond_function()
 ****************************************************************
 * @brief
 *  This function registers the callback functions with the base environment object.
 * 
 * @param base_object - This is the object handle, or in our
 * case the board_mgmt.
 * @param packet - The packet arriving from the monitor
 * scheduler.
 * 
 * @return fbe_status_t - The status of the call to
 *                        fbe_lifecycle_class_const_verify() for
 *                        the Encl management's constant
 *                        lifecycle data.
 *
 * @author
 *  19-Jan-2011: Arun S - Created. 
 *
 ****************************************************************/
static fbe_lifecycle_status_t
fbe_board_mgmt_register_resume_prom_callback_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_base_environment_t * pBaseEnv = (fbe_base_environment_t *)base_object;

    fbe_base_object_trace(base_object, 
                          FBE_TRACE_LEVEL_DEBUG_HIGH,  
                          FBE_TRACE_MESSAGE_ID_INFO, 
                          "%s entry\n", 
                          __FUNCTION__);

    fbe_base_env_initiate_resume_prom_read_callback(pBaseEnv, (fbe_base_env_initiate_resume_prom_read_callback_func_ptr_t) &fbe_board_mgmt_initiate_resume_prom_cmd);
    fbe_base_env_get_resume_prom_info_ptr_callback(pBaseEnv, (fbe_base_env_get_resume_prom_info_ptr_func_callback_ptr_t)&fbe_board_mgmt_get_resume_prom_info_ptr);
    fbe_base_env_resume_prom_update_encl_fault_led_callback(pBaseEnv, (fbe_base_env_resume_prom_update_encl_fault_led_callback_func_ptr_t)&fbe_board_mgmt_resume_prom_update_encl_fault_led);

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
 * fbe_board_mgmt_processSuitcaseStatus()
 ****************************************************************
 * @brief
 *  This function processes the Suitcase status change at
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
fbe_board_mgmt_processSuitcaseStatus(fbe_board_mgmt_t *board_mgmt,
                                     fbe_device_physical_location_t *pLocation)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_board_mgmt_suitcase_info_t          getSuitcaseInfo;
    fbe_bool_t                              suitcaseStatusChange = FALSE;
    fbe_u8_t                                deviceStr[FBE_DEVICE_STRING_LENGTH];
    fbe_bool_t                              shutdownSuitcase = FALSE;

    // Get the new Suitcase info (only valid from Board Object)
    getSuitcaseInfo.associatedSp = pLocation->sp;
    getSuitcaseInfo.suitcaseID = pLocation->slot;
  
    // get the latest suitcase status  
    status = fbe_api_board_get_suitcase_info(board_mgmt->object_id, &getSuitcaseInfo);
    if (status != FBE_STATUS_OK) 
    {
        fbe_base_object_trace((fbe_base_object_t *)board_mgmt, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s Error getting Suitcase info, status: 0x%X.\n",
                              __FUNCTION__, status);
        return status;
    }

    // generate the device string (used for tracing & events)  
    status = fbe_base_env_create_device_string(FBE_DEVICE_TYPE_SUITCASE, 
                                               pLocation, 
                                               &deviceStr[0],
                                               FBE_DEVICE_STRING_LENGTH);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)board_mgmt, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                              "%s, Failed to create device string.\n", 
                              __FUNCTION__); 

        return status;
    }

    if (getSuitcaseInfo.envInterfaceStatus != board_mgmt->suitcase_info[pLocation->sp][pLocation->slot].envInterfaceStatus)
    {
        if (getSuitcaseInfo.envInterfaceStatus == FBE_ENV_INTERFACE_STATUS_XACTION_FAIL)
        {
            fbe_event_log_write(ESP_ERROR_ENV_INTERFACE_FAILURE_DETECTED,
                                NULL, 0,
                                "%s %s", 
                                &deviceStr[0],
                                SmbTranslateErrorString(getSuitcaseInfo.transactionStatus));
        }
        else if (getSuitcaseInfo.envInterfaceStatus != FBE_ENV_INTERFACE_STATUS_GOOD)
        {
            fbe_event_log_write(ESP_ERROR_ENV_INTERFACE_FAILURE_DETECTED,
                                NULL, 0,
                                "%s %s", 
                                &deviceStr[0],
                                fbe_base_env_decode_env_status(getSuitcaseInfo.envInterfaceStatus));
        }
        else
        {
            fbe_event_log_write(ESP_INFO_ENV_INTERFACE_FAILURE_CLEARED,
                                NULL, 0,
                                "%s", 
                                &deviceStr[0]);
        }
        board_mgmt->suitcase_info[pLocation->sp][pLocation->slot].envInterfaceStatus = getSuitcaseInfo.envInterfaceStatus;
    }

    // check for ShutdownWarning change
    if (getSuitcaseInfo.shutdownWarning != 
        board_mgmt->suitcase_info[pLocation->sp][pLocation->slot].shutdownWarning)
    {
        suitcaseStatusChange = TRUE;
        fbe_base_object_trace((fbe_base_object_t *)board_mgmt, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s, suitcase %d, shutdownWarning changed from %d to %d\n",
                              __FUNCTION__, 
                              pLocation->slot,
                              board_mgmt->suitcase_info[pLocation->sp][pLocation->slot].shutdownWarning, 
                              getSuitcaseInfo.shutdownWarning);
        if ((board_mgmt->suitcase_info[pLocation->sp][pLocation->slot].shutdownWarning == FBE_MGMT_STATUS_FALSE) &&
            (getSuitcaseInfo.shutdownWarning == FBE_MGMT_STATUS_TRUE))
        {
            fbe_event_log_write(ESP_ERROR_ENCL_SHUTDOWN_DETECTED,
                        NULL, 0, 
                        "%s %s", 
                        &deviceStr[0],
                        "Suitcase");
            shutdownSuitcase = TRUE;
        }
        else if ((board_mgmt->suitcase_info[pLocation->sp][pLocation->slot].shutdownWarning == FBE_MGMT_STATUS_TRUE) &&
                (getSuitcaseInfo.shutdownWarning == FBE_MGMT_STATUS_FALSE))
        {
            fbe_event_log_write(ESP_INFO_ENCL_SHUTDOWN_CLEARED,
                        NULL, 0,
                        "%s",
                        &deviceStr[0]);
        }
        board_mgmt->suitcase_info[pLocation->sp][pLocation->slot].shutdownWarning = 
            getSuitcaseInfo.shutdownWarning;
    }
    // check for ambientOvertempFault change
    if (getSuitcaseInfo.ambientOvertempFault != 
        board_mgmt->suitcase_info[pLocation->sp][pLocation->slot].ambientOvertempFault)
    {
        suitcaseStatusChange = TRUE;
        fbe_base_object_trace((fbe_base_object_t *)board_mgmt, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s, suitcase %d, ambientOvertempFault changed from %d to %d\n",
                              __FUNCTION__, 
                              pLocation->slot,
                              board_mgmt->suitcase_info[pLocation->sp][pLocation->slot].ambientOvertempFault, 
                              getSuitcaseInfo.ambientOvertempFault);
        if ((board_mgmt->suitcase_info[pLocation->sp][pLocation->slot].ambientOvertempFault == FBE_MGMT_STATUS_FALSE) &&
            (getSuitcaseInfo.ambientOvertempFault == FBE_MGMT_STATUS_TRUE))
        {
            fbe_event_log_write(ESP_ERROR_SUITCASE_FAULT_DETECTED,
                                NULL, 0, 
                                "%s %s", 
                                &deviceStr[0],
                                "ambientOverTemp");
        }
        else if ((board_mgmt->suitcase_info[pLocation->sp][pLocation->slot].ambientOvertempFault == FBE_MGMT_STATUS_TRUE) &&
                (getSuitcaseInfo.ambientOvertempFault == FBE_MGMT_STATUS_FALSE))
        {
            fbe_event_log_write(ESP_INFO_SUITCASE_FAULT_CLEARED,
                                NULL, 0,
                                "%s %s", 
                                &deviceStr[0],
                                "ambientOverTemp");
        }
        board_mgmt->suitcase_info[pLocation->sp][pLocation->slot].ambientOvertempFault = 
            getSuitcaseInfo.ambientOvertempFault;
    }
    // check for ambientOvertempWarning change
    if (getSuitcaseInfo.ambientOvertempWarning != 
        board_mgmt->suitcase_info[pLocation->sp][pLocation->slot].ambientOvertempWarning)
    {
        suitcaseStatusChange = TRUE;
        fbe_base_object_trace((fbe_base_object_t *)board_mgmt, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s, suitcase %d, ambientOvertempWarning changed from %d to %d\n",
                              __FUNCTION__, 
                              pLocation->slot,
                              board_mgmt->suitcase_info[pLocation->sp][pLocation->slot].ambientOvertempWarning, 
                              getSuitcaseInfo.ambientOvertempWarning);
        board_mgmt->suitcase_info[pLocation->sp][pLocation->slot].ambientOvertempWarning = 
            getSuitcaseInfo.ambientOvertempWarning;
    }
    // check for Tap12VMissing change
    if (getSuitcaseInfo.Tap12VMissing != 
        board_mgmt->suitcase_info[pLocation->sp][pLocation->slot].Tap12VMissing)
    {
        suitcaseStatusChange = TRUE;
        fbe_base_object_trace((fbe_base_object_t *)board_mgmt, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s, suitcase %d, Tap12VMissing changed from %d to %d\n",
                              __FUNCTION__, 
                              pLocation->slot,
                              board_mgmt->suitcase_info[pLocation->sp][pLocation->slot].Tap12VMissing, 
                              getSuitcaseInfo.Tap12VMissing);
        if ((board_mgmt->suitcase_info[pLocation->sp][pLocation->slot].Tap12VMissing == FBE_MGMT_STATUS_FALSE) &&
            (getSuitcaseInfo.Tap12VMissing == FBE_MGMT_STATUS_TRUE))
        {
            fbe_event_log_write(ESP_ERROR_SUITCASE_FAULT_DETECTED,
                                NULL, 0, 
                                "%s %s", 
                                &deviceStr[0],
                                "Tap12VMissing");
        }
        else if ((board_mgmt->suitcase_info[pLocation->sp][pLocation->slot].Tap12VMissing == FBE_MGMT_STATUS_TRUE) &&
                (getSuitcaseInfo.Tap12VMissing == FBE_MGMT_STATUS_FALSE))
        {
            fbe_event_log_write(ESP_INFO_SUITCASE_FAULT_CLEARED,
                                NULL, 0,
                                "%s %s", 
                                &deviceStr[0],
                                "Tap12VMissing");
        }
        board_mgmt->suitcase_info[pLocation->sp][pLocation->slot].Tap12VMissing = 
            getSuitcaseInfo.Tap12VMissing;
    }

    // process other Suitcase faults
    if (board_mgmt->suitcase_info[pLocation->sp][pLocation->slot].isCPUFaultRegFail != getSuitcaseInfo.isCPUFaultRegFail)
    {
        suitcaseStatusChange = TRUE;
        fbe_base_object_trace((fbe_base_object_t *)board_mgmt, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s, suitcase %d, isCPUFaultRegFail changed from %d to %d\n",
                              __FUNCTION__, 
                              pLocation->slot,
                              board_mgmt->suitcase_info[pLocation->sp][pLocation->slot].isCPUFaultRegFail,
                              getSuitcaseInfo.isCPUFaultRegFail);
        board_mgmt->suitcase_info[pLocation->sp][pLocation->slot].isCPUFaultRegFail = 
            getSuitcaseInfo.isCPUFaultRegFail;
    }
    if (board_mgmt->suitcase_info[pLocation->sp][pLocation->slot].hwErrMonFault != getSuitcaseInfo.hwErrMonFault)
    {
        suitcaseStatusChange = TRUE;
        fbe_base_object_trace((fbe_base_object_t *)board_mgmt, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s, suitcase %d, hwErrMonFault changed from %d to %d\n",
                              __FUNCTION__, 
                              pLocation->slot,
                              board_mgmt->suitcase_info[pLocation->sp][pLocation->slot].hwErrMonFault,
                              getSuitcaseInfo.hwErrMonFault);
        board_mgmt->suitcase_info[pLocation->sp][pLocation->slot].hwErrMonFault = 
            getSuitcaseInfo.hwErrMonFault;
    }
    
    fbe_board_mgmt_calculateSuitcaseStateInfo(board_mgmt, pLocation->sp, pLocation->slot);

    // send Suitcase event notification if necessary
    if (suitcaseStatusChange)
    {
        /* Send Suitcase data change */
        fbe_base_environment_send_data_change_notification((fbe_base_environment_t*)board_mgmt,
                                                               FBE_CLASS_ID_BOARD_MGMT, 
                                                               FBE_DEVICE_TYPE_SUITCASE, 
                                                               FBE_DEVICE_DATA_TYPE_GENERIC_INFO,
                                                               pLocation);
    }


    // check if CacheStatus may have changed
    fbe_board_mgmt_checkCacheStatus(board_mgmt, pLocation);


    return status;

}   // end of fbe_board_mgmt_processSuitcaseStatus

/*!**************************************************************
 * fbe_board_mgmt_processBmcStatus()
 ****************************************************************
 * @brief
 *  This function processes the Bmc status change at
 *  startup time discovery or at runtime.
 *
 * @param pEnclMgmt - This is the object handle, or in our case the encl_mgmt.
 * @param objectId - 
 * @param pLocation - 
 *
 * @return fbe_status_t - 
 *
 * @author
 *  06-Sep-2012:  Eric Zhou - Created.
 *
 ****************************************************************/
static fbe_status_t 
fbe_board_mgmt_processBmcStatus(fbe_board_mgmt_t *board_mgmt,
                                fbe_device_physical_location_t *pLocation)
{
        fbe_status_t                            status = FBE_STATUS_OK;
        fbe_board_mgmt_bmc_info_t               getBmcInfo;
        fbe_bool_t                              bmcStatusChange = FALSE;
        fbe_u8_t                                deviceStr[FBE_DEVICE_STRING_LENGTH] = {0};
        fbe_suitcase_state_t                    prevSuitcaseState;

        // Get the new Bmc info (only valid from Board Object)
        getBmcInfo.associatedSp = pLocation->sp;
        getBmcInfo.bmcID = pLocation->slot;
      
        // get the latest bmc status  
        status = fbe_api_board_get_bmc_info(board_mgmt->object_id, &getBmcInfo);
        if (status != FBE_STATUS_OK) 
        {
            fbe_base_object_trace((fbe_base_object_t *)board_mgmt, 
                                  FBE_TRACE_LEVEL_WARNING,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s Error getting Bmc info, status: 0x%X.\n",
                                  __FUNCTION__, status);
            return status;
        }
    
        // generate the device string (used for tracing & events)  
        status = fbe_base_env_create_device_string(FBE_DEVICE_TYPE_BMC, 
                                                   pLocation, 
                                                   &deviceStr[0],
                                                   FBE_DEVICE_STRING_LENGTH);
        if(status != FBE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t *)board_mgmt, 
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                                  "%s, Failed to create device string.\n", 
                                  __FUNCTION__); 
    
            return status;
        }

        if (getBmcInfo.envInterfaceStatus != board_mgmt->bmc_info[pLocation->sp][pLocation->slot].envInterfaceStatus)
        {
            if (getBmcInfo.envInterfaceStatus == FBE_ENV_INTERFACE_STATUS_XACTION_FAIL)
            {
                fbe_event_log_write(ESP_ERROR_ENV_INTERFACE_FAILURE_DETECTED,
                                    NULL, 0,
                                    "%s %s", 
                                    &deviceStr[0],
                                    SmbTranslateErrorString(getBmcInfo.transactionStatus));
            }
            else if (getBmcInfo.envInterfaceStatus != FBE_ENV_INTERFACE_STATUS_GOOD)
            {
                fbe_event_log_write(ESP_ERROR_ENV_INTERFACE_FAILURE_DETECTED,
                                    NULL, 0,
                                    "%s %s", 
                                    &deviceStr[0],
                                    fbe_base_env_decode_env_status(getBmcInfo.envInterfaceStatus));
            }
            else
            {
                fbe_event_log_write(ESP_INFO_ENV_INTERFACE_FAILURE_CLEARED,
                                    NULL, 0,
                                    "%s", 
                                    &deviceStr[0]);
            }
            board_mgmt->bmc_info[pLocation->sp][pLocation->slot].envInterfaceStatus = getBmcInfo.envInterfaceStatus;
        }

        // check for ShutdownWarning change
        if (getBmcInfo.shutdownWarning !=
            board_mgmt->bmc_info[pLocation->sp][pLocation->slot].shutdownWarning)
        {
            bmcStatusChange = TRUE;
            fbe_base_object_trace((fbe_base_object_t *)board_mgmt, 
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s, bmc %d, shutdownWarning changed from %d to %d\n",
                                  __FUNCTION__, 
                                  pLocation->slot,
                                  board_mgmt->bmc_info[pLocation->sp][pLocation->slot].shutdownWarning,
                                  getBmcInfo.shutdownWarning);

            if (getBmcInfo.shutdownWarning)
            {
                fbe_event_log_write(ESP_ERROR_BMC_SHUTDOWN_WARNING_DETECTED,
                            NULL, 0,
                            "%s %s",
                            &deviceStr[0],
                            fbe_board_mgmt_decodeBmcShutdownReason(&(getBmcInfo.shutdownReason)));
            }
            else
            {
                fbe_event_log_write(ESP_INFO_BMC_SHUTDOWN_WARNING_CLEARED,
                            NULL, 0,
                            "%s",
                            &deviceStr[0]);
            }
        }

        prevSuitcaseState = board_mgmt->suitcase_info[pLocation->sp][pLocation->slot].state;
        fbe_board_mgmt_processBmcBistStatus(board_mgmt, 
                                            pLocation,
                                            &getBmcInfo.bistReport, 
                                            &board_mgmt->bmc_info[pLocation->sp][pLocation->slot].bistReport);
        if (prevSuitcaseState != board_mgmt->suitcase_info[pLocation->sp][pLocation->slot].state)
        {
            bmcStatusChange = TRUE;
            fbe_base_object_trace((fbe_base_object_t *)board_mgmt, 
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s, %s status change detected\n",
                                  __FUNCTION__, 
                                  &deviceStr[0]);
        }


        fbe_copy_memory(&(board_mgmt->bmc_info[pLocation->sp][pLocation->slot]), &getBmcInfo, sizeof(fbe_board_mgmt_bmc_info_t));

        // send Bmc event notification if necessary
        if (bmcStatusChange)
        {
            /* Send Bmc data change */
            fbe_base_environment_send_data_change_notification((fbe_base_environment_t*)board_mgmt,
                                                                   FBE_CLASS_ID_BOARD_MGMT, 
                                                                   FBE_DEVICE_TYPE_BMC, 
                                                                   FBE_DEVICE_DATA_TYPE_GENERIC_INFO,
                                                                   pLocation);
        }

        return status;
}


/*!**************************************************************
 * fbe_board_mgmt_create_cache_card_fault_string()
 ****************************************************************
 * @brief
 *  This function formats Cache Card fault string.
 *
 * @param pPsInfo - input, pointer to cache_card info
 * @param isFaulted - output, whether cache_card is faulted
 * @param string - output, fault string.
 *
 * @return fbe_status_t - result of formatting the string.
 *
 * @author
 *  16-Feb-2013: Chang Rui - Created.
 *
 ****************************************************************/
static fbe_status_t 
fbe_board_mgmt_create_cache_card_fault_string(fbe_board_mgmt_cache_card_info_t *pCacheCardInfo, fbe_bool_t *isFaulted, char* string)
{
    *isFaulted = FALSE;

    if((pCacheCardInfo->inserted == FBE_MGMT_STATUS_TRUE)&& (pCacheCardInfo->powerGood == FBE_MGMT_STATUS_FALSE))
    {
        *isFaulted = TRUE;
        strcat(string, "PowerFlt ");
    }

    if((pCacheCardInfo->inserted == FBE_MGMT_STATUS_TRUE)&& (pCacheCardInfo->powerUpFailure == FBE_MGMT_STATUS_TRUE))
    {
        *isFaulted = TRUE;
        strcat(string, "PowerUpFlt ");
    }

    if (!(*isFaulted))
    {
        strcat(string, "NoFlt ");
    }

    return FBE_STATUS_OK;
}


/*!**************************************************************
 * fbe_board_mgmt_processCacheCardStatus()
 ****************************************************************
 * @brief
 *  This function processes the cache card status change at
 *  startup time discovery or at runtime.
 *
 * @param board_mgmt - This is the object handle, or in our case the board_mgmt.
 * @param objectId - 
 * @param pLocation - 
 *
 * @return fbe_status_t - 
 *
 * @author
 *  06-Feb-2013:  Rui Chang - Created.
 *
 ****************************************************************/
static fbe_status_t 
fbe_board_mgmt_processCacheCardStatus(fbe_board_mgmt_t *board_mgmt,
                                fbe_device_physical_location_t *pLocation)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_board_mgmt_cache_card_info_t           newCacheCardInfo={0};
    fbe_board_mgmt_cache_card_info_t           oldCacheCardInfo={0};
    fbe_bool_t                              cacheCardStatusChange = FALSE;
    fbe_bool_t                              ledControlNeeded = FALSE;
    fbe_u8_t                                deviceStr[FBE_EVENT_LOG_MESSAGE_STRING_LENGTH]={0};
    fbe_u8_t                                oldFaultString[FBE_EVENT_LOG_MESSAGE_STRING_LENGTH]={0};
    fbe_u8_t                                newFaultString[FBE_EVENT_LOG_MESSAGE_STRING_LENGTH]={0};
    fbe_bool_t                              isFaultedOld=FALSE;
    fbe_bool_t                              isFaultedNew=FALSE;

    oldCacheCardInfo = board_mgmt->cacheCardInfo[pLocation->slot];

    // Get the new Cache Card info (only valid from Board Object)
    newCacheCardInfo.associatedSp = pLocation->sp;
    newCacheCardInfo.cacheCardID = pLocation->slot;
  
    // get the latest Cache Card status  
    status = fbe_api_board_get_cache_card_info(board_mgmt->object_id, &newCacheCardInfo);
    if (status != FBE_STATUS_OK) 
    {
        fbe_base_object_trace((fbe_base_object_t *)board_mgmt, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s Error getting Cache Card info, status: 0x%X.\n",
                              __FUNCTION__, status);
        return status;
    }

    // generate the device string (used for tracing & events)  
    status = fbe_base_env_create_device_string(FBE_DEVICE_TYPE_CACHE_CARD, 
                                               pLocation, 
                                               &deviceStr[0],
                                               FBE_DEVICE_STRING_LENGTH);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)board_mgmt, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                              "%s, Failed to create device string.\n", 
                              __FUNCTION__); 

        return status;
    }


    /* check for status change
      1. insert status change
      2. fault status change 
    */
    if ((oldCacheCardInfo.inserted == FBE_MGMT_STATUS_FALSE) && (newCacheCardInfo.inserted == FBE_MGMT_STATUS_TRUE))
    {
        cacheCardStatusChange=TRUE;
        fbe_event_log_write(ESP_INFO_CACHE_CARD_INSERTED,
                    NULL, 0,
                    "%s", 
                    &deviceStr[0]);

        fbe_base_object_trace((fbe_base_object_t *)board_mgmt, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s, %s is inserted.\n",
                              __FUNCTION__, 
                              &deviceStr[0]);
    }

    if ((oldCacheCardInfo.inserted == FBE_MGMT_STATUS_TRUE) && (newCacheCardInfo.inserted == FBE_MGMT_STATUS_FALSE) &&
       (board_mgmt->base_environment.isSingleSpSystem))
    {
        cacheCardStatusChange=TRUE;
        fbe_event_log_write(ESP_ERROR_CACHE_CARD_REMOVED,
                    NULL, 0,
                    "%s", 
                    &deviceStr[0]);

        fbe_base_object_trace((fbe_base_object_t *)board_mgmt, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s, %s is removed.\n",
                              __FUNCTION__, 
                              &deviceStr[0]);
    }

    if (newCacheCardInfo.inserted == FBE_MGMT_STATUS_TRUE)
    {
        fbe_board_mgmt_create_cache_card_fault_string(&oldCacheCardInfo, &isFaultedOld, &oldFaultString[0]);
        fbe_board_mgmt_create_cache_card_fault_string(&newCacheCardInfo, &isFaultedNew, &newFaultString[0]);
        if (!isFaultedOld && isFaultedNew)
        {
            cacheCardStatusChange=TRUE; 
            ledControlNeeded = TRUE;
            
            fbe_event_log_write(ESP_ERROR_CACHE_CARD_FAULTED,
                        NULL, 0,
                        "%s %s", 
                        &deviceStr[0],
                        &newFaultString[0]);
    
            fbe_base_object_trace((fbe_base_object_t *)board_mgmt, 
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s, %s is faulted (%s).\n",
                                  __FUNCTION__, 
                                  &deviceStr[0],
                                  &newFaultString[0]);
        }
        if (isFaultedOld && !isFaultedNew)
        {
            cacheCardStatusChange=TRUE; 
            ledControlNeeded = TRUE;
            
            fbe_event_log_write(ESP_INFO_CACHE_CARD_FAULT_CLEARED,
                        NULL, 0,
                        "%s", 
                        &deviceStr[0]);
    
            fbe_base_object_trace((fbe_base_object_t *)board_mgmt, 
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s, %s fault cleared.\n",
                                  __FUNCTION__, 
                                  &deviceStr[0]);
        }
    }

    /* check configuration
      1. single SP array (eg. single SP beachcomber) must have cacheCard
      2. dual SP array must not have cacheCard
     */
    if ((board_mgmt->base_environment.isSingleSpSystem) &&
        (newCacheCardInfo.inserted == FBE_MGMT_STATUS_FALSE))
    {
        board_mgmt->cacheCardConfigFault = TRUE;
        ledControlNeeded = TRUE;
        fbe_event_log_write(ESP_ERROR_CACHE_CARD_CONFIGURATION_ERROR,
                    NULL, 0,
                    "%s %s",
                    &deviceStr[0],
                    "Single SP array must have Cache Card");
        fbe_base_object_trace((fbe_base_object_t *)board_mgmt, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s, Cache Card is missing for this single SP array.\n",
                              __FUNCTION__);
    }

    if ((!board_mgmt->base_environment.isSingleSpSystem) &&
        (newCacheCardInfo.inserted == FBE_MGMT_STATUS_TRUE))
    {
        board_mgmt->cacheCardConfigFault = TRUE;
        ledControlNeeded = TRUE;
        fbe_event_log_write(ESP_ERROR_CACHE_CARD_CONFIGURATION_ERROR,
                    NULL, 0,
                    "%s %s",
                    &deviceStr[0],
                    "Dual SP array cannot have Cache Card");

        fbe_base_object_trace((fbe_base_object_t *)board_mgmt, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s, Cache Card should be removed for this dual SP array.\n",
                              __FUNCTION__);

        board_mgmt->cacheCardInfo[pLocation->slot].inserted = newCacheCardInfo.inserted;
        /* We do not continure processing since this config is wrong*/
        return FBE_STATUS_OK;
    }

    if (((board_mgmt->base_environment.isSingleSpSystem) && (newCacheCardInfo.inserted == FBE_MGMT_STATUS_TRUE) && (oldCacheCardInfo.inserted == FBE_MGMT_STATUS_FALSE))||
        ((!board_mgmt->base_environment.isSingleSpSystem) && (newCacheCardInfo.inserted == FBE_MGMT_STATUS_FALSE) && (oldCacheCardInfo.inserted == FBE_MGMT_STATUS_TRUE)))
    {
        board_mgmt->cacheCardConfigFault = FALSE;
        ledControlNeeded = TRUE;
        fbe_event_log_write(ESP_INFO_CACHE_CARD_CONFIGURATION_ERROR_CLEARED,
                    NULL, 0,
                    "%s",
                    &deviceStr[0]);
        fbe_base_object_trace((fbe_base_object_t *)board_mgmt, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s, Cache Card configuration becomes correct.\n",
                              __FUNCTION__);
    }

    /* resume prom process */
    status = fbe_board_mgmt_resume_prom_handle_cache_card_status_change(board_mgmt, pLocation, &newCacheCardInfo, &oldCacheCardInfo);
    if(status != FBE_STATUS_OK) 
    {
        fbe_base_object_trace((fbe_base_object_t *)board_mgmt, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "RP:Cache Card(%s, Slot %d), handle_state_change failed, status 0x%X.\n",
                              (pLocation->sp == SP_A)?"SPA":"SPB", 
                              pLocation->slot,
                              status);

    }

    fbe_copy_memory(&(board_mgmt->cacheCardInfo[pLocation->slot]), &(newCacheCardInfo), sizeof(fbe_board_mgmt_cache_card_info_t));

    if (ledControlNeeded == TRUE)
    {
        /*
         * Check if Enclosure Fault LED needs updating
         */
        status = fbe_board_mgmt_update_encl_fault_led(board_mgmt, pLocation, FBE_ENCL_FAULT_LED_CACHE_CARD_FLT);
        if (status != FBE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t *)board_mgmt,
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s, error updating EnclFaultLed, status 0x%X.\n",
                                  &deviceStr[0],
                                  status);
        }
    }

    // send Cache Card event notification if necessary
    if (cacheCardStatusChange)
    {
        /* Send Cache Card data change */
        fbe_base_environment_send_data_change_notification((fbe_base_environment_t*)board_mgmt,
                                                               FBE_CLASS_ID_BOARD_MGMT, 
                                                               FBE_DEVICE_TYPE_CACHE_CARD, 
                                                               FBE_DEVICE_DATA_TYPE_GENERIC_INFO,
                                                               pLocation);
    }

    return status;
}

/*!**************************************************************
 * fbe_board_mgmt_processDimmStatus()
 ****************************************************************
 * @brief
 *  This function processes the DIMM status change at
 *  startup time discovery or at runtime.
 *
 * @param board_mgmt - This is the object handle, or in our case the board_mgmt.
 * @param objectId - 
 * @param pLocation - 
 *
 * @return fbe_status_t - 
 *
 * @author
 *  06-May-2013:  Rui Chang - Created.
 *
 ****************************************************************/
static fbe_status_t 
fbe_board_mgmt_processDimmStatus(fbe_board_mgmt_t *board_mgmt,
                                fbe_device_physical_location_t *pLocation)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_board_mgmt_dimm_info_t              newDimmInfo={0};
    fbe_board_mgmt_dimm_info_t              newPeerDimmInfo={0};
    fbe_board_mgmt_dimm_info_t              oldDimmInfo={0};
    fbe_board_mgmt_dimm_info_t              oldPeerDimmInfo={0};
    fbe_bool_t                              dimmStatusChange = FALSE;
    fbe_bool_t                              peerDimmStatusChange = FALSE;
    fbe_u8_t                                deviceStr[FBE_EVENT_LOG_MESSAGE_STRING_LENGTH]={0};
    fbe_u8_t                                peerDeviceStr[FBE_EVENT_LOG_MESSAGE_STRING_LENGTH]={0};
    fbe_bool_t                              ledControlNeeded = FALSE;
    fbe_u32_t                               dimmIndex = 0;
    fbe_u32_t                               peerDimmIndex = 0;
    fbe_device_physical_location_t          peerLocation = {0};

    /* Process local DIMM status */
    status = fbe_board_mgmt_get_dimm_index(board_mgmt, pLocation, &dimmIndex);
    if (status != FBE_STATUS_OK) 
    {
        fbe_base_object_trace((fbe_base_object_t *)board_mgmt, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s Error getting DIMM index, sp %d, slot %d.\n",
                              __FUNCTION__, pLocation->sp, pLocation->slot);
        return status;
    }

    // Save old DIMM info
    oldDimmInfo = board_mgmt->dimmInfo[dimmIndex];

    // Get the new DIMM info (only valid from Board Object)
    newDimmInfo.associatedSp = pLocation->sp;
    newDimmInfo.dimmID = pLocation->slot;

    // get the latest DIMM status  
    status = fbe_api_board_get_dimm_info(board_mgmt->object_id, &newDimmInfo);
    if (status != FBE_STATUS_OK) 
    {
        fbe_base_object_trace((fbe_base_object_t *)board_mgmt, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s Error getting DIMM info, status: 0x%X.\n",
                              __FUNCTION__, status);
        return status;
    }

    // generate the device string for local DIMM(used for tracing & events)  
    status = fbe_base_env_create_device_string(FBE_DEVICE_TYPE_DIMM, 
                                               pLocation, 
                                               &deviceStr[0],
                                               FBE_DEVICE_STRING_LENGTH);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)board_mgmt, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                              "%s, Failed to create DIMM device string.\n", 
                              __FUNCTION__); 

        return status;
    }

    if (newDimmInfo.inserted == FBE_MGMT_STATUS_TRUE)
    {
        if (!oldDimmInfo.faulted && newDimmInfo.faulted)
        {
            fbe_event_log_write(ESP_ERROR_DIMM_FAULTED,
                        NULL, 0,
                        "%s", 
                        &deviceStr[0]);
    
            fbe_base_object_trace((fbe_base_object_t *)board_mgmt, 
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s, %s is faulted.\n",
                                  __FUNCTION__, 
                                  &deviceStr[0]);
        }
        if (oldDimmInfo.faulted && !newDimmInfo.faulted)
        {
            fbe_event_log_write(ESP_INFO_DIMM_FAULT_CLEARED,
                        NULL, 0,
                        "%s", 
                        &deviceStr[0]);
    
            fbe_base_object_trace((fbe_base_object_t *)board_mgmt, 
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s, %s fault cleared.\n",
                                  __FUNCTION__, 
                                  &deviceStr[0]);
        }
    }

    if(newDimmInfo.hwErrMonFault != oldDimmInfo.hwErrMonFault)
    {
        fbe_base_object_trace((fbe_base_object_t *)board_mgmt, 
                               FBE_TRACE_LEVEL_INFO,
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "%s hwErrMonFault changed, old %d, new %d.\n",
                               &deviceStr[0],
                               oldDimmInfo.hwErrMonFault,
                               newDimmInfo.hwErrMonFault);
                             
    }

    fbe_board_mgmt_calculateDimmStateInfo(board_mgmt, &(newDimmInfo));

    if((newDimmInfo.state != oldDimmInfo.state) || 
       (newDimmInfo.subState != oldDimmInfo.subState))
    {
        dimmStatusChange=TRUE; 
        ledControlNeeded = TRUE;
        fbe_base_object_trace((fbe_base_object_t *)board_mgmt, 
                               FBE_TRACE_LEVEL_INFO,
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "%s dimmState changed, old %s, new %s, old sub %s, new sub %s.\n",
                               &deviceStr[0],
                               fbe_board_mgmt_decode_dimm_state(oldDimmInfo.state),
                               fbe_board_mgmt_decode_dimm_state(newDimmInfo.state),
                               fbe_board_mgmt_decode_dimm_subState(oldDimmInfo.subState),
                               fbe_board_mgmt_decode_dimm_subState(newDimmInfo.subState));

    }

    /* Save the dimmInfo to the object. */
    fbe_copy_memory(&(board_mgmt->dimmInfo[dimmIndex]), &(newDimmInfo), sizeof(fbe_board_mgmt_dimm_info_t));

    if(!board_mgmt->base_environment.isSingleSpSystem)
    {
        /* Process peer DIMM status */
        peerLocation.sp = (pLocation->sp == SP_A)? SP_B : SP_A;
        peerLocation.slot = pLocation->slot;
        
        status = fbe_board_mgmt_get_dimm_index(board_mgmt, &peerLocation, &peerDimmIndex);
        if (status != FBE_STATUS_OK) 
        {
            fbe_base_object_trace((fbe_base_object_t *)board_mgmt, 
                                  FBE_TRACE_LEVEL_WARNING,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s Error getting peer DIMM index, sp %d, slot %d.\n",
                                  __FUNCTION__, peerLocation.sp, peerLocation.slot);
            return status;
        }
        
        // Save old peer DIMM info
        oldPeerDimmInfo = board_mgmt->dimmInfo[peerDimmIndex];

        // generate the device string for peer DIMM(used for tracing & events)  
        status = fbe_base_env_create_device_string(FBE_DEVICE_TYPE_DIMM, 
                                                   &peerLocation, 
                                                   &peerDeviceStr[0],
                                                   FBE_DEVICE_STRING_LENGTH);
        if(status != FBE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t *)board_mgmt, 
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                                  "%s, Failed to create peer DIMM device string.\n", 
                                  __FUNCTION__); 

            return status;
        }
        
        newPeerDimmInfo.associatedSp = peerLocation.sp;
        newPeerDimmInfo.dimmID = peerLocation.slot;
        newPeerDimmInfo.isLocalFru = FBE_FALSE;
        /* Please note we save the peer DIMM fault(reported by the fault register) in the local DIMM edal in the physical package.
         * So here we copy peerFaultRegFault of local DIMM to the field of faultRegFault of peer DIMM.
         */
        newPeerDimmInfo.faultRegFault = newDimmInfo.peerFaultRegFault;

        if(newPeerDimmInfo.faultRegFault != oldPeerDimmInfo.faultRegFault)
        {
            fbe_base_object_trace((fbe_base_object_t *)board_mgmt, 
                                   FBE_TRACE_LEVEL_INFO,
                                   FBE_TRACE_MESSAGE_ID_INFO,
                                   "%s faultRegFault changed, old %d, new %d.\n",
                                   &peerDeviceStr[0],
                                   oldPeerDimmInfo.faultRegFault,
                                   newPeerDimmInfo.faultRegFault);
        }
        
        fbe_board_mgmt_calculateDimmStateInfo(board_mgmt, &(newPeerDimmInfo));
        
        if((newPeerDimmInfo.state != oldPeerDimmInfo.state) || 
           (newPeerDimmInfo.subState != oldPeerDimmInfo.subState))
        {
            peerDimmStatusChange=TRUE; 
            ledControlNeeded = TRUE;
            fbe_base_object_trace((fbe_base_object_t *)board_mgmt, 
                                   FBE_TRACE_LEVEL_INFO,
                                   FBE_TRACE_MESSAGE_ID_INFO,
                                   "%s dimmState changed, old %s, new %s, old sub %s, new sub %s.\n",
                                   &peerDeviceStr[0],
                                   fbe_board_mgmt_decode_dimm_state(oldPeerDimmInfo.state),
                                   fbe_board_mgmt_decode_dimm_state(newPeerDimmInfo.state),
                                   fbe_board_mgmt_decode_dimm_subState(oldPeerDimmInfo.subState),
                                   fbe_board_mgmt_decode_dimm_subState(newPeerDimmInfo.subState));
                                  
        }
        
        /* Save the peer dimmInfo to the object. */
        fbe_copy_memory(&(board_mgmt->dimmInfo[peerDimmIndex]), &(newPeerDimmInfo), sizeof(fbe_board_mgmt_dimm_info_t));
    }

    if (ledControlNeeded == TRUE)
    {
        /*
         * Check if Enclosure Fault LED needs updating
         */
        status = fbe_board_mgmt_update_encl_fault_led(board_mgmt, pLocation, FBE_ENCL_FAULT_LED_DIMM_FLT);
        if (status != FBE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t *)board_mgmt,
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s, error updating EnclFaultLed, status 0x%X.\n",
                                  &deviceStr[0],
                                  status);
        }
    }

    // send DIMM event notification if necessary
    if (dimmStatusChange)
    {
        /* Send DIMM data change */
        fbe_base_environment_send_data_change_notification((fbe_base_environment_t*)board_mgmt,
                                                               FBE_CLASS_ID_BOARD_MGMT, 
                                                               FBE_DEVICE_TYPE_DIMM, 
                                                               FBE_DEVICE_DATA_TYPE_GENERIC_INFO,
                                                               pLocation);
    }

    if (peerDimmStatusChange)
    {
        /* Send peer DIMM data change */
        fbe_base_environment_send_data_change_notification((fbe_base_environment_t*)board_mgmt,
                                                               FBE_CLASS_ID_BOARD_MGMT, 
                                                               FBE_DEVICE_TYPE_DIMM, 
                                                               FBE_DEVICE_DATA_TYPE_GENERIC_INFO,
                                                               &peerLocation);
    }

    return status;
}

/*!**************************************************************
 * fbe_board_mgmt_processSSDStatus()
 ****************************************************************
 * @brief
 *  This function processes the SSD status change at
 *  startup time discovery or at runtime.
 *
 * @param board_mgmt - This is the object handle, or in our case the board_mgmt.
 * @param objectId - 
 * @param pLocation - 
 *
 * @return fbe_status_t - 
 *
 * @author
 *  10-Oct-2014:  bphilbin - Created.
 *
 ****************************************************************/
static fbe_status_t 
fbe_board_mgmt_processSSDStatus(fbe_board_mgmt_t *board_mgmt,
                                fbe_device_physical_location_t *pLocation)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_board_mgmt_ssd_info_t               newSSDInfo={0};
    fbe_board_mgmt_ssd_info_t               newPeerSSDInfo={0};
    fbe_board_mgmt_ssd_info_t               oldSSDInfo={0};
    fbe_board_mgmt_ssd_info_t               oldPeerSSDInfo={0};
    fbe_bool_t                              ssdStatusChange = FALSE;
    fbe_bool_t                              peerSsdStatusChange = FALSE;
    fbe_u8_t                                deviceStr[FBE_EVENT_LOG_MESSAGE_STRING_LENGTH]={0};
    fbe_u8_t                                peerDeviceStr[FBE_EVENT_LOG_MESSAGE_STRING_LENGTH]={0};
    fbe_u32_t                               ssdIndex = 0;
    fbe_u32_t                               peerSsdIndex = 0;
    fbe_device_physical_location_t          peerLocation = {0};

    /* Process local SSD status */
    status = fbe_board_mgmt_get_ssd_index(board_mgmt, pLocation, &ssdIndex);
    if (status != FBE_STATUS_OK) 
    {
        fbe_base_object_trace((fbe_base_object_t *)board_mgmt, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s Error getting SSD index, sp %d, slot %d.\n",
                              __FUNCTION__, pLocation->sp, pLocation->slot);
        return status;
    }

    // Save old SSD info
    oldSSDInfo = board_mgmt->ssdInfo[ssdIndex];

    // Get the new SSD info 
    fbe_base_object_trace((fbe_base_object_t *)board_mgmt,  
                           FBE_TRACE_LEVEL_DEBUG_LOW,
                           FBE_TRACE_MESSAGE_ID_INFO,
                           "%s Requesting SSD Info from Physical Package for SSD, sp %d, slot %d.\n",
                           __FUNCTION__, 
                          pLocation->sp,
                          pLocation->slot);

    newSSDInfo.associatedSp = pLocation->sp;
    newSSDInfo.slot = pLocation->slot;
  
    // get the latest SSD status  
    status = fbe_api_board_get_ssd_info(board_mgmt->object_id, &newSSDInfo);
    if (status != FBE_STATUS_OK) 
    {
        fbe_base_object_trace((fbe_base_object_t *)board_mgmt, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s Error getting SSD info, sp %d, slot %d, status: 0x%X.\n",
                              __FUNCTION__, 
                              pLocation->sp, 
                              pLocation->slot, 
                              status);
        return status;
    }

    fbe_base_object_trace((fbe_base_object_t *)board_mgmt,  
                           FBE_TRACE_LEVEL_INFO,
                           FBE_TRACE_MESSAGE_ID_INFO,
                           "%s SSD, sp %d, slot:%d SpBlkCnt:%d PctLifeUsd:%d SlfTstPssd:%d\n",
                           __FUNCTION__, 
                          newSSDInfo.associatedSp,
                          newSSDInfo.slot,
                          newSSDInfo.remainingSpareBlkCount, 
                          newSSDInfo.ssdLifeUsed,
                          newSSDInfo.ssdSelfTestPassed);

    // generate the device string (used for tracing & events)  
    status = fbe_base_env_create_device_string(FBE_DEVICE_TYPE_SSD, 
                                               pLocation, 
                                               &deviceStr[0],
                                               FBE_DEVICE_STRING_LENGTH);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)board_mgmt, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                              "%s, Failed to create device string.\n", 
                              __FUNCTION__); 

        return status;
    }

    fbe_board_mgmt_calculateSsdStateInfo(board_mgmt, &newSSDInfo);

    if((oldSSDInfo.ssdState != newSSDInfo.ssdState) ||
       (oldSSDInfo.ssdSubState != newSSDInfo.ssdSubState)) 
    {
        ssdStatusChange = FBE_TRUE;

        fbe_base_object_trace((fbe_base_object_t *)board_mgmt, 
                                   FBE_TRACE_LEVEL_INFO,
                                   FBE_TRACE_MESSAGE_ID_INFO,
                                   "%s ssdState changed, old %s, new %s, old sub %s, new sub %s.\n",
                                   &deviceStr[0],
                                   fbe_board_mgmt_decode_ssd_state(oldSSDInfo.ssdState),
                                   fbe_board_mgmt_decode_ssd_state(newSSDInfo.ssdState),
                                   fbe_board_mgmt_decode_ssd_subState(oldSSDInfo.ssdSubState),
                                   fbe_board_mgmt_decode_ssd_subState(newSSDInfo.ssdSubState));
    }

    fbe_copy_memory(&(board_mgmt->ssdInfo[ssdIndex]), &(newSSDInfo), sizeof(fbe_board_mgmt_ssd_info_t));

    if(!board_mgmt->base_environment.isSingleSpSystem)
    {
        /* Process peer SSD status */
        peerLocation.sp = (pLocation->sp == SP_A)? SP_B : SP_A;
        peerLocation.slot = pLocation->slot;
        
        status = fbe_board_mgmt_get_ssd_index(board_mgmt, &peerLocation, &peerSsdIndex);
        if (status != FBE_STATUS_OK) 
        {
            fbe_base_object_trace((fbe_base_object_t *)board_mgmt, 
                                  FBE_TRACE_LEVEL_WARNING,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s Error getting peer SSD index, sp %d, slot %d.\n",
                                  __FUNCTION__, peerLocation.sp, peerLocation.slot);
            return status;
        }
        
        // Save old peer SSD info
        oldPeerSSDInfo = board_mgmt->ssdInfo[peerSsdIndex];

        // generate the device string for peer DIMM(used for tracing & events)  
        status = fbe_base_env_create_device_string(FBE_DEVICE_TYPE_SSD, 
                                                   &peerLocation, 
                                                   &peerDeviceStr[0],
                                                   FBE_DEVICE_STRING_LENGTH);
        if(status != FBE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t *)board_mgmt, 
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                                  "%s, Failed to create peer SSD device string.\n", 
                                  __FUNCTION__); 

            return status;
        }
        
        newPeerSSDInfo.associatedSp = peerLocation.sp;
        newPeerSSDInfo.slot = peerLocation.slot;
        newPeerSSDInfo.isLocalFru = FBE_FALSE;



        /* Please note we save the peer SSD fault(reported by the fault register) in the local SSD edal in the physical package.
         * So here we copy peerFaultRegFault of local SSD to the field of faultRegFault of peer SSD.
         */
        newPeerSSDInfo.isLocalFaultRegFail = newSSDInfo.isPeerFaultRegFail;

        if(newPeerSSDInfo.isLocalFaultRegFail != oldPeerSSDInfo.isLocalFaultRegFail)
        {
            fbe_base_object_trace((fbe_base_object_t *)board_mgmt, 
                                   FBE_TRACE_LEVEL_INFO,
                                   FBE_TRACE_MESSAGE_ID_INFO,
                                   "%s faultRegFault changed, old %d, new %d.\n",
                                   &peerDeviceStr[0],
                                   oldPeerSSDInfo.isLocalFaultRegFail,
                                   newPeerSSDInfo.isLocalFaultRegFail);
        }
        
        fbe_board_mgmt_calculateSsdStateInfo(board_mgmt, &(newPeerSSDInfo));
        
        if((newPeerSSDInfo.ssdState != oldPeerSSDInfo.ssdState) || 
           (newPeerSSDInfo.ssdSubState != oldPeerSSDInfo.ssdSubState))
        {
            peerSsdStatusChange=TRUE; 
            fbe_base_object_trace((fbe_base_object_t *)board_mgmt, 
                                   FBE_TRACE_LEVEL_INFO,
                                   FBE_TRACE_MESSAGE_ID_INFO,
                                   "%s ssdState changed, old %s, new %s, old sub %s, new sub %s.\n",
                                   &peerDeviceStr[0],
                                   fbe_board_mgmt_decode_ssd_state(oldPeerSSDInfo.ssdState),
                                   fbe_board_mgmt_decode_ssd_state(newPeerSSDInfo.ssdState),
                                   fbe_board_mgmt_decode_ssd_subState(oldPeerSSDInfo.ssdSubState),
                                   fbe_board_mgmt_decode_ssd_subState(newPeerSSDInfo.ssdSubState));
                                  
        }
        
        /* Save the peer ssdInfo to the object. */
        fbe_copy_memory(&(board_mgmt->ssdInfo[peerSsdIndex]), &(newPeerSSDInfo), sizeof(fbe_board_mgmt_ssd_info_t));
    }

    // send SSD event notification if necessary
    if (ssdStatusChange)
    {
        /* Send DIMM data change */
        fbe_base_environment_send_data_change_notification((fbe_base_environment_t*)board_mgmt,
                                                           FBE_CLASS_ID_BOARD_MGMT, 
                                                           FBE_DEVICE_TYPE_SSD, 
                                                           FBE_DEVICE_DATA_TYPE_GENERIC_INFO,
                                                           pLocation);
    }

    // send peer SSD event notification if necessary
    if (ssdStatusChange)
    {
        /* Send SSD data change */
        fbe_base_environment_send_data_change_notification((fbe_base_environment_t*)board_mgmt,
                                                           FBE_CLASS_ID_BOARD_MGMT, 
                                                           FBE_DEVICE_TYPE_SSD, 
                                                           FBE_DEVICE_DATA_TYPE_GENERIC_INFO,
                                                           &peerLocation);
    }

    return status;
}

/*!**************************************************************
 * fbe_board_mgmt_peer_boot_check_cond_function()
 ****************************************************************
 * @brief
 *  This is the condition function for FBE_BOARD_MGMT_PEER_BOOT_CHECK_COND.
 *
 * @param board_mgmt - This handle to board mgmt object 
 *
 * @return fbe_status_t - 
 * 
 * @author
 *  05-Jan-2011: Created  Vaibhav Gaonkar
 *  30-Jan-2013: Chengkai, Add fault register part. 
 *
 ****************************************************************/
static fbe_lifecycle_status_t
fbe_board_mgmt_peer_boot_check_cond_function(fbe_base_object_t * base_object,
                                             fbe_packet_t * packet)
{
    fbe_status_t        status;
    fbe_board_mgmt_t    *board_mgmt = (fbe_board_mgmt_t *) base_object;
    fbe_u32_t  fault_reg_count=0;

    status = fbe_api_board_get_fault_reg_count(board_mgmt->object_id, &fault_reg_count);

    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace(base_object, 
                                      FBE_TRACE_LEVEL_ERROR,
                                      FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                      "PBL:%s, error get_fault reg count, status 0x%x\n",
                                      __FUNCTION__, status);

        /* Need to set timer again, clear condition to start the timer  */
        status = fbe_lifecycle_clear_current_cond(base_object);
        if (status != FBE_STATUS_OK) 
        {
            fbe_base_object_trace(base_object, 
                                      FBE_TRACE_LEVEL_ERROR,
                                      FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                      "PBL: %s, can't clear timer condition, status: 0x%X\n",
                                      __FUNCTION__, status);
        }

        return FBE_LIFECYCLE_STATUS_DONE;
    }

    if (fault_reg_count == 0)
    {
        status = fbe_lifecycle_stop_timer(&FBE_LIFECYCLE_CONST_DATA(board_mgmt),
                                               base_object,
                                               FBE_BOARD_MGMT_PEER_BOOT_CHECK_COND);
        if (status != FBE_STATUS_OK)
        {
            fbe_base_object_trace(base_object, 
                                      FBE_TRACE_LEVEL_ERROR,
                                      FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                      "PBL:%s, error stopping timer, status 0x%x\n",
                                      __FUNCTION__, status);
        }

        return FBE_LIFECYCLE_STATUS_DONE;
    }

    if(board_mgmt->peerBootInfo.stateChangeProcessRequired)
    {
        /* stop the timer condition */
        status = fbe_lifecycle_stop_timer(&FBE_LIFECYCLE_CONST_DATA(board_mgmt),
                                           base_object,
                                           FBE_BOARD_MGMT_PEER_BOOT_CHECK_COND);
        if (status != FBE_STATUS_OK)
        {
            fbe_base_object_trace(base_object, 
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "PBL:%s, error stopping timer, status 0x%x\n",
                                  __FUNCTION__, status);
        }
    }
    else
    {
        /* Need to set timer again, clear condition to start the timer  */
        status = fbe_lifecycle_clear_current_cond(base_object);
        if (status != FBE_STATUS_OK) 
        {
            fbe_base_object_trace(base_object, 
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "PBL: %s, can't clear timer condition, status: 0x%X\n",
                                  __FUNCTION__, status);
        }            
    }
    
    return FBE_LIFECYCLE_STATUS_DONE;
}
/***********************************************************
* end of fbe_board_mgmt_peer_boot_check_cond_function()
***********************************************************/

/*!**************************************************************
 * fbe_board_mgmt_processFaultRegisterStatus()
 ****************************************************************
 * @brief
 *  This is called when there is change in Fault Register state,
 *  This function process that state changes and update the board_mgmt
 *  object with current fault expander state.
 *
 * @param board_mgmt - Handle to board mgmt object 
 * @param pLocation - Physical location
 *
 * @return fbe_status_t - TRUE if state change is updated to board_mgmt object
 *         otherwise FALSE
 * 
 * @author
 *  26-July-2012: Chengkai Hu, Created
 *
 ****************************************************************/
static fbe_status_t
fbe_board_mgmt_processFaultRegisterStatus(fbe_board_mgmt_t *board_mgmt,
                                          fbe_device_physical_location_t *pLocation)
{
    fbe_status_t                    status;
    fbe_base_object_t             * base_object = (fbe_base_object_t *)board_mgmt;
    fbe_peer_boot_flt_exp_info_t    fltRegInfo = {0};

    // only concerned about peer Fault Register changes (ignore local ones)
    if(pLocation->sp == board_mgmt->base_environment.spid)
    {
        return FBE_STATUS_OK;
    }

    if(pLocation->slot >= TOTAL_FLT_REG_PER_BLADE)
    {
        fbe_base_object_trace(base_object, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s, Error in getting the Fault Register ID, Flt Reg ID: 0x%X\n",
                              __FUNCTION__, pLocation->slot);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Get the change information from Physical Package */
    fltRegInfo.associatedSp = pLocation->sp;
    fltRegInfo.fltExpID = pLocation->slot;
    status = fbe_api_board_get_flt_reg_info(board_mgmt->object_id, 
                                            &fltRegInfo);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace(base_object, 
                            FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                            "%s, Error in getting the Fault Register info, status: 0x%X\n",
                            __FUNCTION__, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if(fltRegInfo.envInterfaceStatus == FBE_ENV_INTERFACE_STATUS_GOOD &&
       !fbe_equal_memory(&board_mgmt->peerBootInfo.fltRegStatus, 
                         &fltRegInfo.faultRegisterStatus,
                         sizeof(SPECL_FAULT_REGISTER_STATUS)))
    {
        fbe_base_object_trace((fbe_base_object_t *)board_mgmt, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO, 
                              "PBL: %s :Fault Register changed: %s\n",
                              __FUNCTION__,
                              fltRegInfo.faultRegisterStatus.anyFaultsFound?"faulted":"noFault");

        /* Process the Boot State change */
        fbe_board_mgmt_fltReg_statusChange(board_mgmt, &fltRegInfo);
        
        /* Send notificaton for data change */
        fbe_base_environment_send_data_change_notification((fbe_base_environment_t *) board_mgmt, 
                                                            FBE_CLASS_ID_BOARD_MGMT, 
                                                            FBE_DEVICE_TYPE_FLT_REG,
                                                            FBE_DEVICE_DATA_TYPE_GENERIC_INFO,
                                                            pLocation);
    }
    return FBE_STATUS_OK;
}
/***********************************************************
* end of fbe_board_mgmt_processFaultRegisterStatus()
***********************************************************/
/*!**************************************************************
 * fbe_board_mgmt_discoverFltRegisterInfo()
 ****************************************************************
 * @brief
 *  This function initialized the Fault Register info for board_mgmt object..
 *
 * @param board_mgmt - This handle to board mgmt object 
 *
 * @return void
 * 
 * @author
 *  28-July-2012: Chengkai Hu - Created  
 *
 ****************************************************************/
static void
fbe_board_mgmt_discoverFltRegisterInfo(fbe_board_mgmt_t *board_mgmt)
{
    fbe_status_t                    status;
    fbe_base_object_t              *base_object = (fbe_base_object_t *)board_mgmt;
    fbe_peer_boot_flt_exp_info_t    fltRegInfo = {0};
    fbe_u32_t                       fltRegPerBlade = 0;
    fbe_device_physical_location_t  location = {0};

    status = fbe_api_board_get_flt_reg_count_per_blade(board_mgmt->object_id, &fltRegPerBlade);    
    if (status != FBE_STATUS_OK)
    {
        return;
    }
    
    /* Sanity check */
    if (fltRegPerBlade > TOTAL_FLT_REG_PER_BLADE)
    {
        fbe_base_object_trace(base_object, 
                            FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                            "%s, Incorrect value of fault register number per blade: %d\n",
                            __FUNCTION__, fltRegPerBlade);
        return;
    }
    
    if (fltRegPerBlade > 0)
    {
        /* Set basic peer boot info */            
        board_mgmt->peerBootInfo.peerSp = ((board_mgmt->base_environment.spid == SP_A) ? SP_B : SP_A);
        board_mgmt->peerBootInfo.faultHwType = FBE_DEVICE_TYPE_FLT_REG;
        board_mgmt->peerBootInfo.StateChangeStartTime = fbe_get_time();
   }

    for(fltRegInfo.fltExpID = 0; fltRegInfo.fltExpID < fltRegPerBlade; fltRegInfo.fltExpID++)
    {
        /* Get the fault expander info */
        fltRegInfo.associatedSp = ((board_mgmt->base_environment.spid == SP_A) ? SP_B : SP_A);
        status = fbe_api_board_get_flt_reg_info(board_mgmt->object_id, 
                                                &fltRegInfo);
        if(status == FBE_STATUS_OK &&
           fltRegInfo.envInterfaceStatus == FBE_ENV_INTERFACE_STATUS_GOOD)
        {
            fbe_base_object_trace((fbe_base_object_t *)board_mgmt, 
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO, 
                                  "PBL: %s : %s Fault Register discovered: %s\n",
                                  __FUNCTION__, decodeSpId(fltRegInfo.associatedSp), 
                                  fltRegInfo.faultRegisterStatus.anyFaultsFound?"faulted":"noFault");

            /* Process the Boot State change */
            fbe_board_mgmt_fltReg_statusChange(board_mgmt, &fltRegInfo);
            
            /* Send notificaton for data change */
            location.sp = fltRegInfo.associatedSp;
            location.slot = fltRegInfo.fltExpID;
            
            fbe_base_environment_send_data_change_notification((fbe_base_environment_t *) board_mgmt, 
                                                                FBE_CLASS_ID_BOARD_MGMT, 
                                                                FBE_DEVICE_TYPE_FLT_REG,
                                                                FBE_DEVICE_DATA_TYPE_GENERIC_INFO,
                                                                &location);
            break;
        }
    }
    return;
}
/*************************************************
* end of fbe_board_mgmt_discoverFltRegisterInfo()
*************************************************/


/*!**************************************************************
 * fbe_board_mgmt_processBmcBistStatus()
 ****************************************************************
 * @brief
 *  This function will process the BMC's BIST status to 
 *  determine if the SP is faulted.
 *
 * @param board_mgmt - This handle to board mgmt object 
 * @param pLocation - location of the BMC
 * @param newBistStatus - new BIST status
 * @param prevBistStatus - previou BIST status
 *
 * @return fbe_status_t (OK if no problems)
 * 
 * @author
 *  13-Aug-2015: Joe Perry - Created  
 *
 ****************************************************************/
static fbe_status_t
fbe_board_mgmt_processBmcBistStatus(fbe_board_mgmt_t *board_mgmt,
                                    fbe_device_physical_location_t *pLocation,
                                    fbe_board_mgmt_bist_info_t *newBistStatus,
                                    fbe_board_mgmt_bist_info_t *prevBistStatus)
{
    fbe_status_t        status = FBE_STATUS_OK;
    fbe_u8_t            deviceStr[FBE_DEVICE_STRING_LENGTH];
    fbe_bool_t          spBistFailureDetected = FALSE;
    fbe_bool_t          i2cBistFailureDetected = FALSE;
    fbe_bool_t          uartBistFailureDetected = FALSE;
    fbe_u32_t           index;

    // Check BIST areas that have multiple tests
    for (index = 0; index < I2C_TEST_MAX; index++)
    {
        if (newBistStatus->i2cTests[index] == BMC_BIST_FAILED)
        {
            i2cBistFailureDetected = TRUE;
        }
    }
    for (index = 0; index < UART_TEST_MAX; index++)
    {
        if (newBistStatus->uartTests[index] == BMC_BIST_FAILED)
        {
            uartBistFailureDetected = TRUE;
        }
    }

    // Check if any of the BIST test failed that indict the SP
    if ((newBistStatus->cpuTest == BMC_BIST_FAILED) ||
        (newBistStatus->dramTest == BMC_BIST_FAILED) ||
        (newBistStatus->sramTest == BMC_BIST_FAILED) ||
        (newBistStatus->sspTest == BMC_BIST_FAILED) ||
        (newBistStatus->nvramTest == BMC_BIST_FAILED) ||
        (newBistStatus->sgpioTest == BMC_BIST_FAILED) ||
        (newBistStatus->dedicatedLanTest == BMC_BIST_FAILED) ||
        i2cBistFailureDetected || 
        uartBistFailureDetected)
    {
        spBistFailureDetected = TRUE;
    }

    if (spBistFailureDetected)
    {
        // BIST Failure on SP reported, see if already faulted the SP
        if (!board_mgmt->bmc_info[pLocation->sp][pLocation->slot].bistFailureDetected)
        {
            board_mgmt->bmc_info[pLocation->sp][pLocation->slot].bistFailureDetected = TRUE;
            // generate the device string (used for tracing & events)  
            status = fbe_base_env_create_device_string(FBE_DEVICE_TYPE_SUITCASE, 
                                                       pLocation, 
                                                       &deviceStr[0],
                                                       FBE_DEVICE_STRING_LENGTH);
            if(status != FBE_STATUS_OK)
            {
                fbe_base_object_trace((fbe_base_object_t *)board_mgmt, 
                                      FBE_TRACE_LEVEL_ERROR,
                                      FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                                      "%s, Failed to create device string.\n", 
                                      __FUNCTION__); 

                return status;
            }

            fbe_base_object_trace((fbe_base_object_t *)board_mgmt, 
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO, 
                                  "%s, BIST Failure - fault %s\n",
                                  __FUNCTION__, &deviceStr[0]);

            // provide additional details on which BIST test failed
            if ((prevBistStatus->cpuTest != BMC_BIST_FAILED) && (newBistStatus->cpuTest == BMC_BIST_FAILED))
            {
                fbe_base_object_trace((fbe_base_object_t *)board_mgmt, 
                                      FBE_TRACE_LEVEL_INFO,
                                      FBE_TRACE_MESSAGE_ID_INFO, 
                                      "%s, %s - CPU BIST Failure\n",
                                      __FUNCTION__, &deviceStr[0]);
            }
            if ((prevBistStatus->dramTest != BMC_BIST_FAILED) && (newBistStatus->dramTest == BMC_BIST_FAILED))
            {
                fbe_base_object_trace((fbe_base_object_t *)board_mgmt, 
                                      FBE_TRACE_LEVEL_INFO,
                                      FBE_TRACE_MESSAGE_ID_INFO, 
                                      "%s, %s - DRAM BIST Failure\n",
                                      __FUNCTION__, &deviceStr[0]);
            }
            if ((prevBistStatus->sramTest != BMC_BIST_FAILED) && (newBistStatus->sramTest == BMC_BIST_FAILED))
            {
                fbe_base_object_trace((fbe_base_object_t *)board_mgmt, 
                                      FBE_TRACE_LEVEL_INFO,
                                      FBE_TRACE_MESSAGE_ID_INFO, 
                                      "%s, %s - SRAM BIST Failure\n",
                                      __FUNCTION__, &deviceStr[0]);
            }
            if ((prevBistStatus->sspTest != BMC_BIST_FAILED) && (newBistStatus->sspTest == BMC_BIST_FAILED))
            {
                fbe_base_object_trace((fbe_base_object_t *)board_mgmt, 
                                      FBE_TRACE_LEVEL_INFO,
                                      FBE_TRACE_MESSAGE_ID_INFO, 
                                      "%s, %s - SSP BIST Failure\n",
                                      __FUNCTION__, &deviceStr[0]);
            }
            if ((prevBistStatus->nvramTest != BMC_BIST_FAILED) && (newBistStatus->nvramTest == BMC_BIST_FAILED))
            {
                fbe_base_object_trace((fbe_base_object_t *)board_mgmt, 
                                      FBE_TRACE_LEVEL_INFO,
                                      FBE_TRACE_MESSAGE_ID_INFO, 
                                      "%s, %s - NVRAM BIST Failure\n",
                                      __FUNCTION__, &deviceStr[0]);
            }
            if ((prevBistStatus->dedicatedLanTest != BMC_BIST_FAILED) && (newBistStatus->dedicatedLanTest == BMC_BIST_FAILED))
            {
                fbe_base_object_trace((fbe_base_object_t *)board_mgmt, 
                                      FBE_TRACE_LEVEL_INFO,
                                      FBE_TRACE_MESSAGE_ID_INFO, 
                                      "%s, %s - LAN BIST Failure\n",
                                      __FUNCTION__, &deviceStr[0]);
            }
            if ((prevBistStatus->sgpioTest != BMC_BIST_FAILED) && (newBistStatus->sgpioTest == BMC_BIST_FAILED))
            {
                fbe_base_object_trace((fbe_base_object_t *)board_mgmt, 
                                      FBE_TRACE_LEVEL_INFO,
                                      FBE_TRACE_MESSAGE_ID_INFO, 
                                      "%s, %s - SGPIO BIST Failure\n",
                                      __FUNCTION__, &deviceStr[0]);
            }
            if (i2cBistFailureDetected)
            {
                for (index = 0; index < I2C_TEST_MAX; index++) 
                {
                    if ((prevBistStatus->i2cTests[index] != BMC_BIST_FAILED) && (newBistStatus->i2cTests[index] == BMC_BIST_FAILED))
                    {
                        fbe_base_object_trace((fbe_base_object_t *)board_mgmt, 
                                              FBE_TRACE_LEVEL_INFO,
                                              FBE_TRACE_MESSAGE_ID_INFO, 
                                              "%s, %s - I2C BIST Failure\n",
                                              __FUNCTION__, &deviceStr[0]);
                        break;
                    }
                }
            }
            if (uartBistFailureDetected)
            {
                for (index = 0; index < UART_TEST_MAX; index++) 
                {
                    if ((prevBistStatus->uartTests[index] != BMC_BIST_FAILED) && (newBistStatus->uartTests[index] == BMC_BIST_FAILED))
                    {
                        fbe_base_object_trace((fbe_base_object_t *)board_mgmt, 
                                              FBE_TRACE_LEVEL_INFO,
                                              FBE_TRACE_MESSAGE_ID_INFO, 
                                              "%s, %s - UART BIST Failure\n",
                                              __FUNCTION__, &deviceStr[0]);
                        break;
                    }
                }
            }
        }   // state == OK, subState == NO_FAULT
    }   // spBistFailureDetected
    else
    {
        // No BIST Failure on SP reported, see if faulted the SP
        if (board_mgmt->bmc_info[pLocation->sp][pLocation->slot].bistFailureDetected)
        {
            board_mgmt->bmc_info[pLocation->sp][pLocation->slot].bistFailureDetected = FALSE;
            // generate the device string (used for tracing & events)  
            status = fbe_base_env_create_device_string(FBE_DEVICE_TYPE_SUITCASE, 
                                                       pLocation, 
                                                       &deviceStr[0],
                                                       FBE_DEVICE_STRING_LENGTH);
            if(status != FBE_STATUS_OK)
            {
                fbe_base_object_trace((fbe_base_object_t *)board_mgmt, 
                                      FBE_TRACE_LEVEL_ERROR,
                                      FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                                      "%s, Failed to create device string.\n", 
                                      __FUNCTION__); 

                return status;
            }

            fbe_base_object_trace((fbe_base_object_t *)board_mgmt, 
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO, 
                                  "%s, BIST Failures cleared on %s\n",
                                  __FUNCTION__, &deviceStr[0]);
        }
    }

    fbe_board_mgmt_calculateSuitcaseStateInfo(board_mgmt, pLocation->sp, pLocation->slot);

    fbe_base_object_trace((fbe_base_object_t *)board_mgmt, 
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO, 
                          "%s, %s, bistFailureDetected %d\n",
                          __FUNCTION__, &deviceStr[0],
                          board_mgmt->bmc_info[pLocation->sp][pLocation->slot].bistFailureDetected);
    return status;

}   // end of fbe_board_mgmt_processBmcBistStatus


static const fbe_char_t * fbe_board_mgmt_decodeBmcShutdownReason(SHUTDOWN_REASON *bmcShutdownReason)
{
    const char* rtnStr;

    if (bmcShutdownReason->embedIOOverTemp == FBE_TRUE)
    {
        rtnStr = "Embedded IO Module Over Temperature";
    }
    else if (bmcShutdownReason->fanFault == FBE_TRUE)
    {
        rtnStr = "Fan Fault";
    }
    else if (bmcShutdownReason->diskOverTemp == FBE_TRUE)
    {
        rtnStr = "Disk Over Temperature";
    }
    else if (bmcShutdownReason->slicOverTemp == FBE_TRUE)
    {
        rtnStr = "IO Module Over Temperature";
    }
    else if (bmcShutdownReason->psOverTemp == FBE_TRUE)
    {
        rtnStr = "Power Supply Over Temperature";
    }
    else if (bmcShutdownReason->memoryOverTemp == FBE_TRUE)
    {
        rtnStr = "Memory Over Temperature";
    }
    else if (bmcShutdownReason->cpuOverTemp == FBE_TRUE)
    {
        rtnStr = "CPU Over Temperature";
    }
    else if (bmcShutdownReason->ambientOverTemp == FBE_TRUE)
    {
        rtnStr = "Ambient Over Temperature";
    }
    else if (bmcShutdownReason->sspHang == FBE_TRUE)
    {
        rtnStr = "SSP Hang";
    }
    else
    {
        rtnStr = "Unknow Reason";
    }

    return (rtnStr);
}

/*!**************************************************************
 * fbe_board_mgmt_processSlavePortStatus()
 ****************************************************************
 * @brief
 *  This is called when there is change in slave port state,
 *  This function process that state changes and update the board_mgmt
 *  object with current slave port state.
 *
 * @param board_mgmt - Handle to board mgmt object 
 * @param pLocation - Physical location
 *
 * @return fbe_status_t - TRUE if state change is updated to board_mgmt object
 *         otherwise FALSE
 * 
 * @author
 *  04-Sep-2012: Chengkai Hu, Created
 *
 ****************************************************************/
static fbe_status_t
fbe_board_mgmt_processSlavePortStatus(fbe_board_mgmt_t *board_mgmt,
                                          fbe_device_physical_location_t *pLocation)
{
    fbe_status_t                         status;
    fbe_base_object_t                  * base_object = (fbe_base_object_t *)board_mgmt;
    fbe_board_mgmt_slave_port_info_t     slavePortInfo = {0};

    // Only concerned about peer slave port changes (ignore local ones)
    if(pLocation->sp == board_mgmt->base_environment.spid)
    {
        return FBE_STATUS_OK;
    }

    if(pLocation->slot >= MAX_SLAVE_PORT_PER_BLADE)
    {
        fbe_base_object_trace(base_object, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s, Error in getting the Slave Port ID: 0x%X\n",
                              __FUNCTION__, pLocation->slot);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Get the change information from Physical Package */
    slavePortInfo.associatedSp = pLocation->sp;
    slavePortInfo.slavePortID = pLocation->slot;
    status = fbe_api_board_get_slave_port_info(board_mgmt->object_id, 
                                            &slavePortInfo);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace(base_object, 
                            FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                            "%s, Error in getting the Slave Port info, status: 0x%X\n",
                            __FUNCTION__, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if(slavePortInfo.envInterfaceStatus == FBE_ENV_INTERFACE_STATUS_GOOD)
    {
        fbe_base_object_trace((fbe_base_object_t *)board_mgmt, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO, 
                              "EPBL: %s :Detected software status Change: 0x%x, 0x%x, 0x%x\n",
                              __FUNCTION__,
                              slavePortInfo.generalStatus, slavePortInfo.componentStatus, slavePortInfo.componentExtStatus);

       /* Process the State change */
        fbe_board_mgmt_slavePortStatusChange(board_mgmt, &slavePortInfo);

        /* Send notificaton for data change */
        fbe_base_environment_send_data_change_notification((fbe_base_environment_t *) board_mgmt, 
                                                            FBE_CLASS_ID_BOARD_MGMT, 
                                                            FBE_DEVICE_TYPE_SLAVE_PORT,
                                                            FBE_DEVICE_DATA_TYPE_GENERIC_INFO,
                                                            pLocation);
    }
    return FBE_STATUS_OK;
}
/***********************************************************
* end of fbe_board_mgmt_processSlavePortStatus()
***********************************************************/

/*!**************************************************************
 * fbe_board_mgmt_discoverSpPhysicalMemory()
 ****************************************************************
 * @brief
 *  This function get local physical memory size, and also fill it
 *      in board mgmt object. And also send CMI message to peer SP
 *      to request peer memory size.
 *
 * @param board_mgmt - pointer to board mgmt object
 *
 * @return N/A 
 *
 * @author
 *  2-July-2012:  Created. Chengkai
 *
 ****************************************************************/
static void 
fbe_board_mgmt_discoverSpPhysicalMemory(fbe_board_mgmt_t *board_mgmt)
{
    SP_ID       localSp;
    fbe_u32_t   memory_size = 0;
    fbe_status_t status;

    /* Get local physical memory size */
    fbe_base_env_get_local_sp_id((fbe_base_environment_t *)board_mgmt, &localSp);
    memory_size = fbe_board_mgmt_get_local_sp_physical_memory();

    if (localSp == SP_A)
    {
        board_mgmt->spPhysicalMemorySize[SP_A]= memory_size;
    }
    else
    {
        board_mgmt->spPhysicalMemorySize[SP_B]= memory_size;
    }

    /* Send request to peer to get peer board info */
    status = fbe_board_mgmt_sendCmiMessage(board_mgmt, FBE_BASE_ENV_CMI_MSG_BOARD_INFO_REQUEST);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)board_mgmt, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s Error sending BOARD info request to peer, status: 0x%X\n",
                              __FUNCTION__, status);
    }

    return;
}
/***********************************************************
* end of fbe_board_mgmt_discoverSpPhysicalMemory()
***********************************************************/
/*!**************************************************************
 * fbe_board_mgmt_updatePeerPhysicalMemory()
 ****************************************************************
 * @brief
 *  This function update peer physical memory size in board mgmt object.
 *
 * @param board_mgmt - pointer to board mgmt object
 *
 * @return fbe_u32_t - size of local physical memory 
 *
 * @author
 *  2-July-2012:  Created. Chengkai
 *
 ****************************************************************/
static void 
fbe_board_mgmt_updatePeerPhysicalMemory(fbe_board_mgmt_t *board_mgmt, 
                                        fbe_board_mgmt_cmi_msg_t *pBoardCmiPkt)
{
    SP_ID       localSp;

    fbe_base_env_get_local_sp_id((fbe_base_environment_t *)board_mgmt, &localSp);

    if (localSp == SP_A)
    {
        board_mgmt->spPhysicalMemorySize[SP_B]= pBoardCmiPkt->spPhysicalMemorySize;
    }
    else
    {
        board_mgmt->spPhysicalMemorySize[SP_A]= pBoardCmiPkt->spPhysicalMemorySize;
    }
}
/***********************************************************
* end of fbe_board_mgmt_updatePeerPhysicalMemory()
***********************************************************/

/*!**************************************************************
 * @fn fbe_board_mgmt_process_peer_cache_status_update()
 ****************************************************************
 * @brief
 *  This function will handle the message we received from peer 
 *  to update peerCacheStatus.
 *
 * @param board_mgmt -
 * @param cmiMsgPtr - Pointer to board_mgmt cmi packet
 * @param classId - board_mgmt class id.
 *
 * @return fbe_status_t 
 *
 * @author
 *  20-Nov-2012:  Dipak Patel - Created. 
 *
 ****************************************************************/
static fbe_status_t fbe_board_mgmt_process_peer_cache_status_update(fbe_board_mgmt_t *board_mgmt, 
                                                                    fbe_board_mgmt_cmi_packet_t * cmiMsgPtr)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_base_env_cacheStatus_cmi_packet_t *baseCmiPtr = (fbe_base_env_cacheStatus_cmi_packet_t *)cmiMsgPtr;
    fbe_common_cache_status_t prevCombineCacheStatus = FBE_CACHE_STATUS_OK;
    fbe_common_cache_status_t peerCacheStatus = FBE_CACHE_STATUS_OK;

    fbe_base_object_trace((fbe_base_object_t *)board_mgmt, 
               FBE_TRACE_LEVEL_INFO,
               FBE_TRACE_MESSAGE_ID_INFO,
               "%s entry.\n",
               __FUNCTION__);

    status = fbe_base_environment_get_peerCacheStatus((fbe_base_environment_t *)board_mgmt,
                                                       &peerCacheStatus); 

    prevCombineCacheStatus = fbe_base_environment_combine_cacheStatus((fbe_base_environment_t *)board_mgmt,
                                                                       board_mgmt->boardCacheStatus,
                                                                       peerCacheStatus,
                                                                       FBE_CLASS_ID_BOARD_MGMT);
    /* Update Peer cache status for this side */
    status = fbe_base_environment_set_peerCacheStatus((fbe_base_environment_t *)board_mgmt,
                                                       baseCmiPtr->CacheStatus);

    /* Compare it with local cache status and send notification to cache if require.*/ 
    status = fbe_base_environment_compare_cacheStatus((fbe_base_environment_t *)board_mgmt,
                                                      prevCombineCacheStatus,
                                                      baseCmiPtr->CacheStatus,
                                                      FBE_CLASS_ID_BOARD_MGMT);

    /* Check if peer cache status is not initialized at peer side.
       If so, we need to send CMI message to peer to update it's
       peer cache status */
    if (baseCmiPtr->isPeerCacheStatusInitilized == FALSE)
    {
        /* Send CMI to peer.
           Over here, peerCacheStatus for this SP will intilized as we have update it above.
           So we are returning it with TRUE */

        fbe_base_object_trace((fbe_base_object_t *)board_mgmt, 
                   FBE_TRACE_LEVEL_INFO,
                   FBE_TRACE_MESSAGE_ID_INFO,
                   "%s, peerCachestatus is uninitialized at peer\n",
                   __FUNCTION__);

        status = fbe_base_environment_send_cacheStatus_update_to_peer((fbe_base_environment_t *)board_mgmt, 
                                                                       board_mgmt->boardCacheStatus,
                                                                       TRUE);
    }

    return status;
}

/*!**************************************************************
 * @fn fbe_board_mgmt_checkCacheStatus()
 ****************************************************************
 * @brief
 *  This function will determine the CacheStatus for board_mgmt.
 *
 * @param board_mgmt -
 * @param *pLocation - location of Processor Enclosure. 
 *
 * @return fbe_status_t 
 *
 * @author
 *  04-Nov-2013:  Joe Perry - Created.
 *
 ****************************************************************/
static fbe_status_t 
fbe_board_mgmt_checkCacheStatus(fbe_board_mgmt_t *board_mgmt,
                                fbe_device_physical_location_t *pLocation)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_board_mgmt_suitcase_info_t          getSuitcaseInfo;
    fbe_device_physical_location_t          location;
    fbe_u32_t                               suitcaseShutdownCount = 0;
    fbe_u32_t                               suitcaseOverTempWarningCount = 0;
    fbe_u32_t                               suitcaseShutdownEnvIntFltCount = 0;
    fbe_u32_t                               suitcaseIndex;
    fbe_common_cache_status_t               boardCacheStatus = board_mgmt->boardCacheStatus;
    fbe_common_cache_status_t               peerBoardCacheStatus = FBE_CACHE_STATUS_OK;
    SP_ID                                   sp = SP_A;
    fbe_u32_t                               spCount=SP_ID_MAX;

    // Get the new Suitcase info (only valid from Board Object)
    getSuitcaseInfo.associatedSp = pLocation->sp;
    getSuitcaseInfo.suitcaseID = pLocation->slot;
  
    // get the latest suitcase status  
    if (board_mgmt->suitcaseCountPerBlade > 0)
    {
        status = fbe_api_board_get_suitcase_info(board_mgmt->object_id, &getSuitcaseInfo); 
        if (status != FBE_STATUS_OK) 
        {
            fbe_base_object_trace((fbe_base_object_t *)board_mgmt, 
                                  FBE_TRACE_LEVEL_WARNING,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s Error getting Suitcase info, status: 0x%X.\n",
                                  __FUNCTION__, status);
            return status;
        }
    }

    // Set SP count
    if (board_mgmt->base_environment.isSingleSpSystem == TRUE)
    {
        spCount = SP_COUNT_SINGLE;
    }
    else 
    {
        spCount = SP_ID_MAX;
    }

    // check if peer SP is down & this SP Suitcase is shutting down
    if ((!board_mgmt->base_environment.isSingleSpSystem == TRUE) && !fbe_cmi_is_peer_alive())
    {
        if ((board_mgmt->suitcase_info[board_mgmt->base_environment.spid][0].envInterfaceStatus == FBE_ENV_INTERFACE_STATUS_DATA_STALE) ||
            (board_mgmt->suitcase_info[board_mgmt->base_environment.spid][0].envInterfaceStatus == FBE_ENV_INTERFACE_STATUS_XACTION_FAIL))
        {
            boardCacheStatus = FBE_CACHE_STATUS_FAILED_SHUTDOWN_ENV_FLT;
            fbe_base_object_trace((fbe_base_object_t *)board_mgmt, 
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s, peerSP down & local SP has EnvIntf errors\n",
                                  __FUNCTION__);
        }
        else if (board_mgmt->suitcase_info[board_mgmt->base_environment.spid][0].shutdownWarning)
        {
            boardCacheStatus = FBE_CACHE_STATUS_FAILED_SHUTDOWN;
            fbe_base_object_trace((fbe_base_object_t *)board_mgmt, 
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s, peerSP down & local SP ShutdownWarning set\n",
                                  __FUNCTION__);
        }
        else if (board_mgmt->suitcase_info[board_mgmt->base_environment.spid][0].ambientOvertempWarning)
        {
            boardCacheStatus = FBE_CACHE_STATUS_APPROACHING_OVER_TEMP;
            fbe_base_object_trace((fbe_base_object_t *)board_mgmt, 
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s, peerSP down & local SP ambientOvertempWarning set\n",
                                  __FUNCTION__);
        }
        else
        {
            boardCacheStatus = FBE_CACHE_STATUS_DEGRADED;
            fbe_base_object_trace((fbe_base_object_t *)board_mgmt, 
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s, peerSP down & no local SP ShutdownWarning\n",
                                  __FUNCTION__);
        }
    }
    // check all Suitcase status
    else
    {
        suitcaseShutdownCount = suitcaseShutdownEnvIntFltCount = suitcaseOverTempWarningCount = 0;
        for(sp = SP_A; sp <spCount; sp ++)
        {
            for (suitcaseIndex = 0; suitcaseIndex < board_mgmt->suitcaseCountPerBlade; suitcaseIndex++)
            {
                if ((board_mgmt->suitcase_info[sp][suitcaseIndex].envInterfaceStatus == FBE_ENV_INTERFACE_STATUS_DATA_STALE) ||
                    (board_mgmt->suitcase_info[sp][suitcaseIndex].envInterfaceStatus == FBE_ENV_INTERFACE_STATUS_XACTION_FAIL))
                {
                    suitcaseShutdownEnvIntFltCount++;
                }
                else if (board_mgmt->suitcase_info[sp][suitcaseIndex].shutdownWarning == FBE_MGMT_STATUS_TRUE)
                {
                    suitcaseShutdownCount++;
                }
                else if (board_mgmt->suitcase_info[sp][suitcaseIndex].ambientOvertempWarning)
                {
                    suitcaseOverTempWarningCount++;
                }
            }
        }
        if (suitcaseShutdownCount == board_mgmt->suitcaseCountPerBlade * spCount)
        {
            boardCacheStatus = FBE_CACHE_STATUS_FAILED_SHUTDOWN;
            fbe_base_object_trace((fbe_base_object_t *)board_mgmt, 
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s, All available Suitcases shutting down\n",
                                  __FUNCTION__);
        }
        else if (suitcaseShutdownEnvIntFltCount == board_mgmt->suitcaseCountPerBlade * SP_ID_MAX)
        {
            boardCacheStatus = FBE_CACHE_STATUS_FAILED_SHUTDOWN_ENV_FLT;
            fbe_base_object_trace((fbe_base_object_t *)board_mgmt, 
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s, All available Suitcases have EnvIntf errors\n",
                                  __FUNCTION__);
        }
        else if (suitcaseOverTempWarningCount > 0)
        {
            boardCacheStatus = FBE_CACHE_STATUS_APPROACHING_OVER_TEMP;
            fbe_base_object_trace((fbe_base_object_t *)board_mgmt, 
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s, Some Suitcases are ambientOverTempWarning\n",
                                  __FUNCTION__);
        } 
        else if (suitcaseShutdownCount > 0) 
        {
            boardCacheStatus = FBE_CACHE_STATUS_DEGRADED;
            fbe_base_object_trace((fbe_base_object_t *)board_mgmt, 
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s, Some Suitcases are shutting down\n",
                                  __FUNCTION__);
        }
        else
        {
            boardCacheStatus = FBE_CACHE_STATUS_OK;
        }
    }

    if (board_mgmt->boardCacheStatus != boardCacheStatus)
    {
        fbe_base_object_trace((fbe_base_object_t *)board_mgmt, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s, boardCacheStatus changed from %d to %d\n",
                              __FUNCTION__, 
                              board_mgmt->boardCacheStatus,
                              boardCacheStatus);

        /* Log the event for cache status change */
        fbe_event_log_write(ESP_INFO_CACHE_STATUS_CHANGED,
                            NULL, 0,
                            "%s %s %s",
                            "BOARD_MGMT",
                            fbe_base_env_decode_cache_status(board_mgmt->boardCacheStatus),
                            fbe_base_env_decode_cache_status(boardCacheStatus));

        status = fbe_base_environment_get_peerCacheStatus((fbe_base_environment_t *)board_mgmt,
                                                             &peerBoardCacheStatus);
        /* Send CMI to peer */
        fbe_base_environment_send_cacheStatus_update_to_peer((fbe_base_environment_t *)board_mgmt, 
                                                              boardCacheStatus,
                                                             ((peerBoardCacheStatus == FBE_CACHE_STATUS_UNINITIALIZE)? FALSE : TRUE));

        board_mgmt->boardCacheStatus = boardCacheStatus;
        fbe_zero_memory(&location, sizeof(fbe_device_physical_location_t));     // location not relevant
        fbe_base_environment_send_data_change_notification((fbe_base_environment_t *)board_mgmt, 
                                                           FBE_CLASS_ID_BOARD_MGMT, 
                                                           FBE_DEVICE_TYPE_SP_ENVIRON_STATE, 
                                                           FBE_DEVICE_DATA_TYPE_GENERIC_INFO,
                                                           &location);    
        fbe_base_object_trace((fbe_base_object_t *)board_mgmt, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s, FBE_DEVICE_TYPE_SP_ENVIRON_STATE sent\n",
                              __FUNCTION__);
    }

    return status;

}   // end of fbe_board_mgmt_checkCacheStatus


/*!**************************************************************
 * @fn fbe_board_mgmt_realFltRegFaultDetected()
 ****************************************************************
 * @brief
 *  This function will determine if there is a Fault Expander
 *  fault which should cause Enclousre Fault LED to be ON.
 *
 * @param board_mgmt -
 *
 * @return fbe_bool_t 
 *
 * @author
 *  10-Jan-2014:  Joe Perry - Created.
 *
 ****************************************************************/
static fbe_bool_t fbe_board_mgmt_realFltRegFaultDetected(fbe_board_mgmt_t *board_mgmt)
{
    fbe_bool_t                      enclFaultLedFltExpfaulted = FALSE;
    fbe_status_t                    status;
    fbe_peer_boot_flt_exp_info_t    fltRegInfo;

    // if no faults, no need to look further
    if (!board_mgmt->peerBootInfo.fltRegStatus.anyFaultsFound)
    {
        return FALSE;
    }

    // get Fault Expander Register info from Phy Pkg
    fbe_zero_memory(&fltRegInfo, sizeof(fbe_peer_boot_flt_exp_info_t));
    if (board_mgmt->base_environment.spid == SP_A)
    {
        fltRegInfo.associatedSp = SP_B;
    }
    else 
    {
        fltRegInfo.associatedSp = SP_A;
    }
    status = fbe_api_board_get_flt_reg_info(board_mgmt->object_id, 
                                            &fltRegInfo);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)board_mgmt, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s, Error in getting the Fault Register info, status: 0x%X\n",
                              __FUNCTION__, status);
        return FALSE;
    }
    if(fltRegInfo.envInterfaceStatus != FBE_ENV_INTERFACE_STATUS_GOOD)
    {
        fbe_base_object_trace((fbe_base_object_t *)board_mgmt, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s, EnvironIntfFailure detected, EnvIntfStatus 0x%x\n",
                              __FUNCTION__, fltRegInfo.envInterfaceStatus);
        return FALSE;
    }

    // go through Fault Expander Register faults to see if any non-I2C Bus faults
    if (fbe_board_mgmt_fltReg_checkNewFault(board_mgmt, &fltRegInfo))
    {
        enclFaultLedFltExpfaulted = TRUE;
    }

    fbe_base_object_trace((fbe_base_object_t *)board_mgmt, 
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s, checkBiosPostFault returned %d\n",
                          __FUNCTION__, enclFaultLedFltExpfaulted);
    return enclFaultLedFltExpfaulted;

}   // end of fbe_board_mgmt_realFltRegFaultDetected

/*!***************************************************************
 * fbe_board_mgmt_calculateSuitcaseStateInfo()
 ****************************************************************
 * @brief
 *  This function will calculate & update the specified
 *  suitcase's State & SubState attributes.
 *
 * @param board_mgmt - Handle to board_mgmt.
 * @param sp - SP ID of suitcase
 * @param slot - slot of suitcase
 *
 * @return fbe_status_t
 *
 * @author
 *  5-Mar-2015: Created  Joe Perry
 *
 ****************************************************************/
static fbe_status_t 
fbe_board_mgmt_calculateSuitcaseStateInfo(fbe_board_mgmt_t *board_mgmt,
                                          SP_ID sp,
                                          fbe_u32_t slot)
{

    if (slot > board_mgmt->suitcaseCountPerBlade)
    {
        fbe_base_object_trace((fbe_base_object_t*)board_mgmt, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s, Invalid slot parameter %d\n",
                              __FUNCTION__, slot);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    if ((board_mgmt->fbeInternalCablePort1 == INTERNAL_CABLE_STATE_MISSING) && (sp == board_mgmt->base_environment.spid))
    {
            board_mgmt->suitcase_info[sp][slot].state = FBE_SUITCASE_STATE_FAULTED;
            board_mgmt->suitcase_info[sp][slot].subState = FBE_SUITCASE_SUBSTATE_INTERNAL_CABLE_MISSING;
    } 
    else if ((board_mgmt->fbeInternalCablePort1 == FBE_CABLE_STATUS_CROSSED) && (sp == board_mgmt->base_environment.spid))
    {
        board_mgmt->suitcase_info[sp][slot].state = FBE_SUITCASE_STATE_FAULTED;
        board_mgmt->suitcase_info[sp][slot].subState = FBE_SUITCASE_SUBSTATE_INTERNAL_CABLE_CROSSED;
    }
    else if (board_mgmt->suitcase_info[sp][slot].isCPUFaultRegFail)
    {
        board_mgmt->suitcase_info[sp][slot].state = FBE_SUITCASE_STATE_FAULTED;
        board_mgmt->suitcase_info[sp][slot].subState = FBE_SUITCASE_SUBSTATE_FLT_STATUS_REG_FAULT;
    }
    else if (board_mgmt->suitcase_info[sp][slot].hwErrMonFault)
    {
        board_mgmt->suitcase_info[sp][slot].state = FBE_SUITCASE_STATE_FAULTED;
        board_mgmt->suitcase_info[sp][slot].subState = FBE_SUITCASE_SUBSTATE_HW_ERR_MON_FAULT;
    }
    // Low Battery fault only reported on local SP
    else if ((sp == board_mgmt->base_environment.spid) && (board_mgmt->lowBattery))
    {
        board_mgmt->suitcase_info[sp][slot].state = FBE_SUITCASE_STATE_FAULTED;
        board_mgmt->suitcase_info[sp][slot].subState = FBE_SUITCASE_SUBSTATE_LOW_BATTERY;
    }
    // BIST failures reported
    else if (board_mgmt->bmc_info[sp][slot].bistFailureDetected)
    {
        board_mgmt->suitcase_info[sp][slot].state = FBE_SUITCASE_STATE_FAULTED;
        board_mgmt->suitcase_info[sp][slot].subState = FBE_SUITCASE_SUBSTATE_BIST_FAILURE;
    }
    else if (board_mgmt->suitcase_info[sp][slot].resumePromReadFailed &&
             (board_mgmt->suitcase_info[sp][slot].transactionStatus != STATUS_SMB_DEVICE_NOT_PRESENT))
    {
        board_mgmt->suitcase_info[sp][slot].state = FBE_SUITCASE_STATE_DEGRADED;
        board_mgmt->suitcase_info[sp][slot].subState = FBE_SUITCASE_SUBSTATE_RP_READ_FAILURE;
    }
    else if ((board_mgmt->suitcase_info[sp][slot].envInterfaceStatus == FBE_ENV_INTERFACE_STATUS_XACTION_FAIL) ||
        (board_mgmt->suitcase_info[sp][slot].envInterfaceStatus == FBE_ENV_INTERFACE_STATUS_DATA_STALE))
    {
        board_mgmt->suitcase_info[sp][slot].state = FBE_SUITCASE_STATE_DEGRADED;
        board_mgmt->suitcase_info[sp][slot].subState = FBE_SUITCASE_SUBSTATE_ENV_INTF_FAILURE;
    }
    else
    {
        board_mgmt->suitcase_info[sp][slot].state = FBE_SUITCASE_STATE_OK;
        board_mgmt->suitcase_info[sp][slot].subState = FBE_SUITCASE_SUBSTATE_NO_FAULT;
    }

    fbe_base_object_trace((fbe_base_object_t*)board_mgmt, 
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s, sp %d, slot %d, state(%d) %s, substate(%d) %s\n",
                          __FUNCTION__, 
                          sp, 
                          slot,
                          board_mgmt->suitcase_info[sp][slot].state,
                          fbe_board_mgmt_decode_suitcase_state(board_mgmt->suitcase_info[sp][slot].state),
                          board_mgmt->suitcase_info[sp][slot].subState,
                          fbe_board_mgmt_decode_suitcase_subState(board_mgmt->suitcase_info[sp][slot].subState));
    return FBE_STATUS_OK;

}   // end of fbe_board_mgmt_calculateSuitcaseStateInfo

/*!***************************************************************
 * fbe_board_mgmt_calculateDimmStateInfo()
 ****************************************************************
 * @brief
 *  This function will calculate & update the specified
 *  DIMM's State & SubState attributes.
 *
 * @param board_mgmt - Handle to board_mgmt.
 * @param pDimmInfo - The pointer to fbe_board_mgmt_dimm_info_t. 
 *
 * @return fbe_status_t
 *
 * @author
 *  5-Mar-2015: Created  Joe Perry
 *
 ****************************************************************/
static fbe_status_t 
fbe_board_mgmt_calculateDimmStateInfo(fbe_board_mgmt_t *board_mgmt,
                                      fbe_board_mgmt_dimm_info_t * pDimmInfo)
{
    SP_ID           localSp = SP_INVALID;
    fbe_u32_t       localDimmIndex = 0;
    fbe_status_t    status = FBE_STATUS_OK; 
    fbe_device_physical_location_t     localDimmLocation = {0};
    fbe_board_mgmt_dimm_info_t       * pLocalDimmInfo = NULL;

    fbe_base_env_get_local_sp_id((fbe_base_environment_t *)board_mgmt, &localSp);
    if(pDimmInfo->associatedSp == localSp)
    {
        /* This is local DIMM. */
        // Skip if the following states were set in Board Object
        if ((pDimmInfo->state != FBE_DIMM_STATE_EMPTY) &&
            (pDimmInfo->state != FBE_DIMM_STATE_MISSING) &&
            (pDimmInfo->state != FBE_DIMM_STATE_NEED_TO_REMOVE))
        {
            if (pDimmInfo->faulted == FBE_MGMT_STATUS_TRUE) 
            {
                pDimmInfo->state = FBE_DIMM_STATE_FAULTED;
                pDimmInfo->subState = FBE_DIMM_SUBSTATE_GEN_FAULT;
            }
            
            else if (pDimmInfo->hwErrMonFault)
            {
                pDimmInfo->state = FBE_DIMM_STATE_FAULTED;
                pDimmInfo->subState = FBE_DIMM_SUBSTATE_HW_ERR_MON_FAULT;
            }
            else if ((pDimmInfo->envInterfaceStatus == FBE_ENV_INTERFACE_STATUS_XACTION_FAIL) ||
                     (pDimmInfo->envInterfaceStatus == FBE_ENV_INTERFACE_STATUS_DATA_STALE))
            {
                pDimmInfo->state = FBE_DIMM_STATE_DEGRADED;
                pDimmInfo->subState = FBE_DIMM_SUBSTATE_ENV_INTF_FAILURE;
            }
            else
            {
                pDimmInfo->state = FBE_DIMM_STATE_OK;
                pDimmInfo->subState = FBE_DIMM_SUBSTATE_NO_FAULT;
            }
        }
    }
    else
    {
        /* This is peer DIMM. Get local DIMM location. */
        localDimmLocation.sp = localSp;
        localDimmLocation.slot = pDimmInfo->dimmID;

        status = fbe_board_mgmt_get_dimm_index(board_mgmt, &localDimmLocation, &localDimmIndex);
        if (status != FBE_STATUS_OK) 
        {
            fbe_base_object_trace((fbe_base_object_t *)board_mgmt, 
                                  FBE_TRACE_LEVEL_WARNING,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s Error getting local DIMM index, sp %d, slot %d.\n",
                                  __FUNCTION__, localDimmLocation.sp, localDimmLocation.slot);
            return status;
        }
    
        pLocalDimmInfo = &board_mgmt->dimmInfo[localDimmIndex];

        if (pDimmInfo->faultRegFault) 
        {
            if (pLocalDimmInfo->dimmDensity == 0) 
            {
                pDimmInfo->state = FBE_DIMM_STATE_NEED_TO_REMOVE;
                pDimmInfo->subState = FBE_DIMM_SUBSTATE_FLT_STATUS_REG_FAULT;
                /* We don't know the dimmDensity here. Just set it to 0. */
                pDimmInfo->dimmDensity = 0;
            }
            else
            {
                /* 
                 * We can not differentiate the Peer DIMM is missing or inserted but faulted. So we just use FAULTED.
                 */
                pDimmInfo->state = FBE_DIMM_STATE_FAULTED;
                pDimmInfo->subState = FBE_DIMM_SUBSTATE_FLT_STATUS_REG_FAULT;
                /* We don't know the dimmDensity here. Just set it to 0. */
                pDimmInfo->dimmDensity = 0;
            }
        }
        else if (pLocalDimmInfo->dimmDensity == 0) 
        {
            pDimmInfo->state = FBE_DIMM_STATE_EMPTY;
            pDimmInfo->subState = FBE_DIMM_SUBSTATE_NO_FAULT;
            pDimmInfo->dimmDensity = 0;
        }
        else
        {
            pDimmInfo->state = FBE_DIMM_STATE_OK;
            pDimmInfo->subState = FBE_DIMM_SUBSTATE_NO_FAULT;
            pDimmInfo->dimmDensity = pLocalDimmInfo->dimmDensity;
        }
    }

    fbe_base_object_trace((fbe_base_object_t*)board_mgmt, 
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s, %s DIMM %d, state %s, substate %s\n",
                          __FUNCTION__, 
                          pDimmInfo->associatedSp == SP_A? "SPA":"SPB",
                          pDimmInfo->dimmID,
                          fbe_board_mgmt_decode_dimm_state(pDimmInfo->state),
                          fbe_board_mgmt_decode_dimm_subState(pDimmInfo->subState));
    return FBE_STATUS_OK;

}   // end of fbe_board_mgmt_calculateDimmStateInfo

/*!***************************************************************
 * fbe_board_mgmt_get_dimm_index()
 ****************************************************************
 * @brief
 *  This function gets the dimm index for the dimm specified by the sp and dimm id.
 *
 * @param board_mgmt - Handle to board_mgmt.
 * @param pLocation - The pointer to the DIMM location.
 * @param pDimmIndex (OUTPUT) - The pointer to the dimmIndex.
 * 
 * @return fbe_status_t board_mgmt
 *
 * @author
 *  28-Apr-2015: PHE - Created
 *
 ****************************************************************/
fbe_status_t fbe_board_mgmt_get_dimm_index(fbe_board_mgmt_t *board_mgmt,
                                           fbe_device_physical_location_t *pLocation,
                                           fbe_u32_t * pDimmIndex)
{
    fbe_u32_t      spCount = 0;

    if (board_mgmt->base_environment.isSingleSpSystem == TRUE)
    {
        spCount = SP_COUNT_SINGLE;
    }
    else
    {
        spCount = SP_ID_MAX;
    }

    *pDimmIndex = pLocation->sp * (board_mgmt->dimmCount/spCount) + pLocation->slot;

    if(*pDimmIndex >= board_mgmt->dimmCount)
    {
        fbe_base_object_trace((fbe_base_object_t *)board_mgmt,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s Error getting DIMM index %d, sp %d, slot %d.\n",
                              __FUNCTION__, *pDimmIndex, pLocation->sp, pLocation->slot);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    return FBE_STATUS_OK;
}

/*!***************************************************************
 * fbe_board_mgmt_calculateSsdStateInfo()
 ****************************************************************
 * @brief
 *  This function will calculate & update the specified
 *  SSD state attribute.
 *
 * @param board_mgmt - Handle to board_mgmt.
 * @param pSsdInfo - The pointer to fbe_board_mgmt_ssd_info_t. 
 *
 * @return fbe_status_t
 *
 * @author
 *  26-June-2015: PHE - Created.
 *
 ****************************************************************/
static fbe_status_t fbe_board_mgmt_calculateSsdStateInfo(fbe_board_mgmt_t *board_mgmt,
                                                         fbe_board_mgmt_ssd_info_t * pSsdInfo)
{
    SP_ID           localSp = SP_INVALID;
    
    fbe_base_env_get_local_sp_id((fbe_base_environment_t *)board_mgmt, &localSp);
    if(pSsdInfo->associatedSp == localSp)
    {
        if(pSsdInfo->ssdLifeUsed == 100)
        {
            pSsdInfo->ssdState = FBE_SSD_STATE_FAULTED;
            pSsdInfo->ssdSubState = FBE_SSD_SUBSTATE_GEN_FAULT;
        }
        else if (pSsdInfo->remainingSpareBlkCount < 5 || pSsdInfo->ssdLifeUsed >= 90)
        {
            pSsdInfo->ssdState = FBE_SSD_STATE_DEGRADED;
            pSsdInfo->ssdSubState = FBE_SSD_SUBSTATE_NO_FAULT;
        }
        else
        {
            pSsdInfo->ssdState = FBE_SSD_STATE_OK;
            pSsdInfo->ssdSubState = FBE_SSD_SUBSTATE_NO_FAULT;
        }
    }
    else
    {
        if(pSsdInfo->isLocalFaultRegFail)
        {
            pSsdInfo->ssdState = FBE_SSD_STATE_FAULTED;
            pSsdInfo->ssdSubState = FBE_SSD_SUBSTATE_FLT_STATUS_REG_FAULT;

        }
        else
        {
            pSsdInfo->ssdState = FBE_SSD_STATE_OK;
            pSsdInfo->ssdSubState = FBE_SSD_SUBSTATE_NO_FAULT;
        }
    }
    
    return FBE_STATUS_OK;
}

/*!***************************************************************
 * fbe_board_mgmt_get_ssd_index()
 ****************************************************************
 * @brief
 *  This function gets the SSD index for the SSD specified by the sp and slot.
 *
 * @param board_mgmt - Handle to board_mgmt.
 * @param pLocation - The pointer to the SSD location.
 * @param pSsdIndex (OUTPUT) - The pointer to the SSD index.
 * 
 * @return fbe_status_t board_mgmt
 *
 * @author
 *  07-Aug-2015: PHE - Created
 *
 ****************************************************************/
fbe_status_t fbe_board_mgmt_get_ssd_index(fbe_board_mgmt_t *board_mgmt,
                                           fbe_device_physical_location_t *pLocation,
                                           fbe_u32_t * pSsdIndex)
{
    fbe_u32_t      spCount = 0;

    if (board_mgmt->base_environment.isSingleSpSystem == TRUE)
    {
        spCount = SP_COUNT_SINGLE;
    }
    else
    {
        spCount = SP_ID_MAX;
    }

    *pSsdIndex = pLocation->sp * (board_mgmt->ssdCount/spCount) + pLocation->slot;

    if(*pSsdIndex >= board_mgmt->ssdCount)
    {
        fbe_base_object_trace((fbe_base_object_t *)board_mgmt,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s Error getting SSD index %d, sp %d, slot %d.\n",
                              __FUNCTION__, *pSsdIndex, pLocation->sp, pLocation->slot);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    return FBE_STATUS_OK;
}



/*!**************************************************************
 * @fn fbe_board_mgmt_handle_destroy_state()
 ****************************************************************
 * @brief
 *  This is the handler function for life cycle change to the
 *  destroy state.
 *
 * @param pBoardMgmt - This is the object handle.
 * @param pUpdateObjectMsg - Pointer to the notification info
 *
 * @return fbe_status_t - FBE Status
 * 
 * @author
 *  19-Oct-2015:  PHE - Created.
 *
 ****************************************************************/
static fbe_status_t fbe_board_mgmt_handle_destroy_state(fbe_board_mgmt_t * pBoardMgmt, 
                                                     update_object_msg_t * pUpdateObjectMsg)
{
    fbe_status_t                  status = FBE_STATUS_OK;
    fbe_device_physical_location_t phys_location = {0};

    fbe_base_object_trace((fbe_base_object_t *)pBoardMgmt,
                              FBE_TRACE_LEVEL_DEBUG_LOW,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s entry.\n",
                              __FUNCTION__);

    //We only deal with the DPE's enclosure destroy.
    if(pBoardMgmt->platform_info.enclosureType != DPE_ENCLOSURE_TYPE)
    {
        return FBE_STATUS_OK;
    }

    if(pBoardMgmt->peObjectId != pUpdateObjectMsg->object_id)
    {
        /* This is not DAE0.*/
        return FBE_STATUS_OK;
    }

    fbe_base_object_trace((fbe_base_object_t *)pBoardMgmt,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s, handle DPE destroy state.\n",
                              __FUNCTION__);

    /* Since the DPE enclosure is destroyed, clean the data structure. */
    pBoardMgmt->peObjectId = FBE_OBJECT_ID_INVALID;
    fbe_zero_memory(&pBoardMgmt->lccInfo[0], SP_ID_MAX * sizeof(fbe_lcc_info_t));
    fbe_zero_memory(&pBoardMgmt->board_fup_info[0], SP_ID_MAX * sizeof(fbe_base_env_fup_persistent_info_t));
    
    phys_location.sp = pBoardMgmt->base_environment.spid;

    status = fbe_base_env_fup_handle_destroy_state((fbe_base_environment_t *)pBoardMgmt, 
                                                    &phys_location);

    if(status != FBE_STATUS_OK) 
    {
        fbe_base_object_trace((fbe_base_object_t *)pBoardMgmt,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "FUP:%s,fup_handle_destroy_state failed,status 0x%X.\n",
                              (phys_location.sp == SP_A)? "SPA" : "SPB", 
                              status);
    }

    return status;
}// End of function fbe_board_mgmt_handle_destroy_state


/*!**************************************************************
 * @fn fbe_board_mgmt_handle_ready_state()
 ****************************************************************
 * @brief
 *  This is the handler function for life cycle change to the
 *  ready state.
 *
 * @param pBoardMgmt - This is the object handle.
 * @param pUpdateObjectMsg - Pointer to the notification info
 *
 * @return fbe_status_t - FBE Status
 * 
 * @author
 *  19-Oct-2015:  PHE - Created.
 *
 ****************************************************************/
static fbe_status_t fbe_board_mgmt_handle_ready_state(fbe_board_mgmt_t * pBoardMgmt, 
                                                     update_object_msg_t * pUpdateObjectMsg)
{
    fbe_status_t                  status = FBE_STATUS_OK;
    fbe_device_physical_location_t phys_location = {0};
    fbe_object_id_t               peObjectId = FBE_OBJECT_ID_INVALID;

    fbe_base_object_trace((fbe_base_object_t *)pBoardMgmt,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s entry.\n",
                              __FUNCTION__);

    //We only deal with the DPE's enclosure destroy.
    if(pBoardMgmt->platform_info.enclosureType != DPE_ENCLOSURE_TYPE)
    {
        return FBE_STATUS_OK;
    }

    status = fbe_api_get_enclosure_object_id_by_location(0, //bus
                                                         0, // enclosure
                                                         &peObjectId);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)pBoardMgmt, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s Error in getting peObjectId, status: 0x%X\n",
                              __FUNCTION__, 
                              status);
        return status;
    }

    if(peObjectId != pUpdateObjectMsg->object_id)
    {
        /* This is not DAE0.*/
        return FBE_STATUS_OK;
    }

    fbe_base_object_trace((fbe_base_object_t *)pBoardMgmt,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s, handle DPE ready state.\n",
                              __FUNCTION__);

    pBoardMgmt->peObjectId = peObjectId;

    phys_location.sp = pBoardMgmt->base_environment.spid;

    status = fbe_board_mgmt_processSPLccStatus(pBoardMgmt, &phys_location);

    if(status != FBE_STATUS_OK) 
    {
        fbe_base_object_trace((fbe_base_object_t *)pBoardMgmt, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s fbe_board_mgmt_processSPLccStatus failed, status 0x%X.\n",
                              (phys_location.sp == SP_A)? "SPA" : "SPB",
                              status);
    }

    return status;
}// End of function fbe_board_mgmt_handle_ready_state
