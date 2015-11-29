/***************************************************************************
 * Copyright (C) EMC Corporation 2010 
 * All rights reserved. Licensed material -- property of EMC 
 * Corporation 
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_base_environment_monitor.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the Enclosure Management object lifecycle
 *  code.
 * 
 *  This includes the
 *  @ref fbe_base_environment_monitor_entry "base_environment monitor entry
 *  point", as well as all the lifecycle defines such as
 *  rotaries and conditions, along with the actual condition
 *  functions.
 * 
 * @ingroup base_environment_class_files
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
#include "fbe_base_environment_private.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_api_board_interface.h"
#include "fbe/fbe_api_enclosure_interface.h"
#include "fbe_transport_memory.h"
#include "fbe/fbe_api_cmi_interface.h"
#include "fbe/fbe_api_common_interface.h"
#include "fbe_base_environment_debug.h"
#include "fbe/fbe_api_esp_encl_mgmt_interface.h"
#include "fbe/fbe_registry.h"

#define BootFromFlashRegPath    "\\SYSTEM\\CurrentControlSet\\Services\\"
#define BootFromFlashKey        "BootFromFlash"

/*!***************************************************************
 * fbe_base_environment_monitor_entry()
 ****************************************************************
 * @brief
 *  Entry routine for the base environment monitor.
 *
 * @param object_handle - This is the object handle, or in our
 * case the base_environment.
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
fbe_base_environment_monitor_entry(fbe_object_handle_t object_handle, fbe_packet_t * packet)
{
	fbe_base_environment_t * base_environment = NULL;

	base_environment = (fbe_base_environment_t *)fbe_base_handle_to_pointer(object_handle);

	fbe_topology_class_trace(FBE_CLASS_ID_BASE_ENVIRONMENT,
			     FBE_TRACE_LEVEL_DEBUG_HIGH, 
			     FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
			     "%s entry\n", __FUNCTION__);

	return fbe_lifecycle_crank_object(&FBE_LIFECYCLE_CONST_DATA(base_environment),
	                                  (fbe_base_object_t*)base_environment, packet);
}
/******************************************
 * end fbe_base_environment_monitor_entry()
 ******************************************/

/*!**************************************************************
 * fbe_base_environment_monitor_load_verify()
 ****************************************************************
 * @brief
 *  This function simply validates the constant lifecycle data
 *  that is associated with the base environment.
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
fbe_status_t fbe_base_environment_monitor_load_verify(void)
{
	return fbe_lifecycle_class_const_verify(&FBE_LIFECYCLE_CONST_DATA(base_environment));
}
/******************************************
 * end fbe_base_environment_monitor_load_verify()
 ******************************************/

/*--- local function prototypes --------------------------------------------------------*/

static fbe_lifecycle_status_t fbe_base_environment_handle_cmi_notification_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet);
static fbe_lifecycle_status_t fbe_base_environment_handle_event_notification_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet);
static fbe_lifecycle_status_t fbe_base_environment_read_persistent_data_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet);
static fbe_lifecycle_status_t fbe_base_environment_wait_peer_persistent_data_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet);
static fbe_lifecycle_status_t fbe_base_environment_write_persistent_data_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet);
static fbe_lifecycle_status_t fbe_base_environment_initialize_persistent_storage_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet);
//static fbe_status_t fbe_base_environment_initialize_persistent_storage_cond_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);
static fbe_lifecycle_status_t fbe_base_environment_send_peer_persistent_data_cond_function(fbe_base_object_t * base_object, 
                                                                                 fbe_packet_t * packet);
static fbe_lifecycle_status_t fbe_base_environment_send_peer_persist_write_complete_cond_function(fbe_base_object_t * base_object, 
                                                                                 fbe_packet_t * packet);
static fbe_status_t fbe_base_environment_read_persistent_data_cond_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);
static fbe_status_t fbe_base_environment_write_persistent_data_cond_completion(fbe_packet_t * packet, 
                                                          fbe_packet_completion_context_t context);
static fbe_lifecycle_status_t fbe_base_environment_send_peer_revision_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet);
static fbe_lifecycle_status_t fbe_base_environment_check_peer_revision_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet);
static fbe_lifecycle_status_t fbe_base_environment_notify_peer_sp_alive_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet);


/*--- lifecycle callback functions -----------------------------------------------------*/


FBE_LIFECYCLE_DEF_CALLBACK_GET_INST_DATA(base_environment);
FBE_LIFECYCLE_DEF_CALLBACK_GET_INST_COND(base_environment);

/*  base_environment_lifecycle_callbacks This variable has all the
 *  callbacks the lifecycle needs.
 */
static fbe_lifecycle_const_callback_t FBE_LIFECYCLE_CALLBACKS(base_environment) = {
	FBE_LIFECYCLE_DEF_CONST_CALLBACKS(
		base_environment,
		FBE_LIFECYCLE_NULL_FUNC,       /* online function */
		FBE_LIFECYCLE_NULL_FUNC)         /* no pending function */
};

/*--- constant base condition entries --------------------------------------------------*/
/* FBE_BASE_ENVIRONMENT_REGISTER_STATE_CHANGE_CALLBACK condition
 *  - The purpose of this condition is to register
 *  notification with the FBE API. The leaf class needs to
 *  implement the actual condition handler function providing
 *  the callback that needs to be called
 */
static fbe_lifecycle_const_base_cond_t fbe_base_environment_register_state_change_callback_cond = {
	FBE_LIFECYCLE_DEF_CONST_BASE_COND(
		"fbe_base_environment_register_state_change_callback_cond",
		FBE_BASE_ENV_LIFECYCLE_COND_REGISTER_STATE_CHANGE_CALLBACK,
		NULL),
	FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
		FBE_LIFECYCLE_STATE_SPECIALIZE,     /* SPECIALIZE */
		FBE_LIFECYCLE_STATE_ACTIVATE,         /* ACTIVATE */
		FBE_LIFECYCLE_STATE_READY,         /* READY */
		FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
		FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
		FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
		FBE_LIFECYCLE_STATE_DESTROY)         /* DESTROY */
};

/* FBE_BASE_ENV_LIFECYCLE_COND_REGISTER_CMI_CALLBACK condition -
 *   The purpose of this condition is to register with the cmi
 *   service. The leaf class needs to
 *  implement the actual condition handler function providing
 *  the callback that needs to be called
 */
static fbe_lifecycle_const_base_cond_t 
    fbe_base_environment_register_cmi_callback_cond = {
        FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "fbe_base_environment_register_cmi_callback_cond",
        FBE_BASE_ENV_LIFECYCLE_COND_REGISTER_CMI_CALLBACK,
        NULL),
	FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
		FBE_LIFECYCLE_STATE_SPECIALIZE,     /* SPECIALIZE */
		FBE_LIFECYCLE_STATE_ACTIVATE,         /* ACTIVATE */
		FBE_LIFECYCLE_STATE_READY,         /* READY */
		FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
		FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
		FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
		FBE_LIFECYCLE_STATE_DESTROY)         /* DESTROY */
};
/* FBE_BASE_ENV_LIFECYCLE_COND_INITIALIZE_PERSISTENT_STORAGE condition -
 *  The purpose of this condition is to initialize the persistent
 *  storage handling in base environment.  !REQUIRED!: Please set the size of your 
 *  persistent storage before setting this condition.
 */


static fbe_lifecycle_const_base_cond_t fbe_base_environment_initialize_persistent_storage_cond = {
	FBE_LIFECYCLE_DEF_CONST_BASE_COND(
		"fbe_base_environment_initialize_persistent_storage_cond",
		FBE_BASE_ENV_LIFECYCLE_COND_INITIALIZE_PERSISTENT_STORAGE,
		fbe_base_environment_initialize_persistent_storage_cond_function),
	FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
		FBE_LIFECYCLE_STATE_SPECIALIZE,     /* SPECIALIZE */
		FBE_LIFECYCLE_STATE_ACTIVATE,         /* ACTIVATE */
		FBE_LIFECYCLE_STATE_READY,         /* READY */
		FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
		FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
		FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
		FBE_LIFECYCLE_STATE_DESTROY)         /* DESTROY */
};


/* FBE_BASE_ENV_LIFECYCLE_COND_SEND_PEER_PERSISTENT_DATA condition -
 *  The purpose of this condition is to send the peer an updated
 *  version of the persistent data, either after a read or write 
 *  of the data.
 */
static fbe_lifecycle_const_base_cond_t fbe_base_environment_send_peer_persistent_data_cond = {
	FBE_LIFECYCLE_DEF_CONST_BASE_COND(
		"fbe_base_environment_send_peer_persistent_data_cond",
		FBE_BASE_ENV_LIFECYCLE_COND_SEND_PEER_PERSISTENT_DATA,
		fbe_base_environment_send_peer_persistent_data_cond_function),
	FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
		FBE_LIFECYCLE_STATE_SPECIALIZE,     /* SPECIALIZE */
		FBE_LIFECYCLE_STATE_ACTIVATE,       /* ACTIVATE */
		FBE_LIFECYCLE_STATE_READY,          /* READY */
		FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
		FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
		FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
		FBE_LIFECYCLE_STATE_DESTROY)         /* DESTROY */
};

/* FBE_BASE_ENV_LIFECYCLE_COND_SEND_PEER_PERSISTENT_DATA condition -
 *  The purpose of this condition is to send the peer an updated
 *  version of the persistent data, either after a read or write 
 *  of the data.
 */
static fbe_lifecycle_const_base_cond_t fbe_base_environment_send_peer_persist_write_complete_cond = {
	FBE_LIFECYCLE_DEF_CONST_BASE_COND(
		"fbe_base_environment_send_peer_persist_write_complete_cond",
		FBE_BASE_ENV_LIFECYCLE_COND_SEND_PEER_PERSIST_WRITE_COMPLETE,
		fbe_base_environment_send_peer_persist_write_complete_cond_function),
	FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
		FBE_LIFECYCLE_STATE_SPECIALIZE,     /* SPECIALIZE */
		FBE_LIFECYCLE_STATE_ACTIVATE,       /* ACTIVATE */
		FBE_LIFECYCLE_STATE_READY,          /* READY */
		FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
		FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
		FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
		FBE_LIFECYCLE_STATE_DESTROY)         /* DESTROY */
};


/* FBE_BASE_ENV_LIFECYCLE_COND_READ_PERSISTENT_DATA condition -
 *  The purpose of this condition is to read persistent data into
 *  memory from the persistent storage service.
 */
static fbe_lifecycle_const_base_cond_t fbe_base_environment_read_persistent_data_cond = {
	FBE_LIFECYCLE_DEF_CONST_BASE_COND(
		"fbe_base_environment_read_persistent_data_cond",
		FBE_BASE_ENV_LIFECYCLE_COND_READ_PERSISTENT_DATA,
		fbe_base_environment_read_persistent_data_cond_function),
	FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
		FBE_LIFECYCLE_STATE_SPECIALIZE,     /* SPECIALIZE */
		FBE_LIFECYCLE_STATE_ACTIVATE,       /* ACTIVATE */
		FBE_LIFECYCLE_STATE_READY,          /* READY */
		FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
		FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
		FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
		FBE_LIFECYCLE_STATE_DESTROY)         /* DESTROY */
};

/* FBE_BASE_ENV_LIFECYCLE_COND_WAIT_PEER_PERSISTENT_DATA condition -
 *  The purpose of this condition is to hold this SP until we get the persistent data
 *  from the peer SP.
 */
static fbe_lifecycle_const_base_cond_t fbe_base_environment_wait_peer_persistent_data_cond = {
	FBE_LIFECYCLE_DEF_CONST_BASE_COND(
		"fbe_base_environment_wait_peer_persistent_data_cond",
		FBE_BASE_ENV_LIFECYCLE_COND_WAIT_PEER_PERSISTENT_DATA,
		fbe_base_environment_wait_peer_persistent_data_cond_function),
	FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
		FBE_LIFECYCLE_STATE_SPECIALIZE,     /* SPECIALIZE */
		FBE_LIFECYCLE_STATE_ACTIVATE,       /* ACTIVATE */
		FBE_LIFECYCLE_STATE_READY,          /* READY */
		FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
		FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
		FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
		FBE_LIFECYCLE_STATE_DESTROY)         /* DESTROY */
};

/* FBE_BASE_ENV_LIFECYCLE_COND_READ_PERSISTENT_DATA_COMPLETE condition -
 *  The purpose of this condition is to allow the leaf class to handle 
 *  the changes in their persistent data that have been loaded into their 
 *  memory.  This should be handled in the leaf class
 */
static fbe_lifecycle_const_base_cond_t fbe_base_environment_read_persistent_data_complete_cond = {
	FBE_LIFECYCLE_DEF_CONST_BASE_COND(
		"fbe_base_environment_read_persistent_data_complete_cond",
		FBE_BASE_ENV_LIFECYCLE_COND_READ_PERSISTENT_DATA_COMPLETE,
		NULL),
	FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
		FBE_LIFECYCLE_STATE_SPECIALIZE,     /* SPECIALIZE */
		FBE_LIFECYCLE_STATE_ACTIVATE,       /* ACTIVATE */
		FBE_LIFECYCLE_STATE_READY,          /* READY */
		FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
		FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
		FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
		FBE_LIFECYCLE_STATE_DESTROY)         /* DESTROY */
};

/* FBE_BASE_ENV_LIFECYCLE_COND_WRITE_PERSISTENT_DATA condition -
 *  The purpose of this condition is to write persistent data into
 *  the persistent storage service from memory.
 */
static fbe_lifecycle_const_base_cond_t fbe_base_environment_write_persistent_data_cond = {
	FBE_LIFECYCLE_DEF_CONST_BASE_COND(
		"fbe_base_environment_write_persistent_data_cond",
		FBE_BASE_ENV_LIFECYCLE_COND_WRITE_PERSISTENT_DATA,
		fbe_base_environment_write_persistent_data_cond_function),
	FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
		FBE_LIFECYCLE_STATE_SPECIALIZE,     /* SPECIALIZE */
		FBE_LIFECYCLE_STATE_ACTIVATE,       /* ACTIVATE */
		FBE_LIFECYCLE_STATE_READY,          /* READY */
		FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
		FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
		FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
		FBE_LIFECYCLE_STATE_DESTROY)         /* DESTROY */
};


/* FBE_BASE_ENV_LIFECYCLE_COND_HANDLE_EVENT_NOTIFICATION condition -
 *  The purpose of this condition is to handle the notication
 *  from the API. The purpose of this condition is to break the
 *  context for notification and run in our leaf class context.
 *  This will use the context that was passed in during the
 *  register notification calls
 */
static fbe_lifecycle_const_base_cond_t fbe_base_environment_handle_event_notification_cond = {
	FBE_LIFECYCLE_DEF_CONST_BASE_COND(
		"fbe_base_environment_handle_event_notification_cond",
		FBE_BASE_ENV_LIFECYCLE_COND_HANDLE_EVENT_NOTIFICATION,
		fbe_base_environment_handle_event_notification_cond_function),
	FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
		FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
		FBE_LIFECYCLE_STATE_ACTIVATE,         /* ACTIVATE */
		FBE_LIFECYCLE_STATE_READY,          /* READY */
		FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
		FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
		FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
		FBE_LIFECYCLE_STATE_DESTROY)         /* DESTROY */
};

/* FBE_BASE_ENV_LIFECYCLE_COND_HANDLE_CMI_NOTIFICATION condition -
*   The purpose of this condition is to handle the cmi
*   notification. The purpose of this condition is to break the
 *  context for notification and run in our leaf class context.
 *  This will use the context that was passed in during the
 *  register notification calls
 */
static fbe_lifecycle_const_base_cond_t 
    fbe_base_environment_handle_cmi_notification_cond = {
        FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "fbe_base_environment_handle_cmi_notification_cond",
        FBE_BASE_ENV_LIFECYCLE_COND_HANDLE_CMI_NOTIFICATION,
        fbe_base_environment_handle_cmi_notification_cond_function),
	FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
		FBE_LIFECYCLE_STATE_SPECIALIZE,     /* SPECIALIZE */
		FBE_LIFECYCLE_STATE_ACTIVATE,       /* ACTIVATE */
		FBE_LIFECYCLE_STATE_READY,          /* READY */
		FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
		FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
		FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
		FBE_LIFECYCLE_STATE_DESTROY)         /* DESTROY */
};

/*  FBE_BASE_ENV_LIFECYCLE_COND_PEER_REVISION_RESPONSE condition -
 *  The purpose of this condition is to respond to the peer's 
 *  request for our local software revision.
 */
static fbe_lifecycle_const_base_cond_t 
    fbe_base_environment_send_peer_revision_cond = {
        FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "fbe_base_environment_send_peer_revision_cond",
        FBE_BASE_ENV_LIFECYCLE_COND_SEND_PEER_REVISION,
        fbe_base_environment_send_peer_revision_cond_function),
	FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
		FBE_LIFECYCLE_STATE_SPECIALIZE,     /* SPECIALIZE */
		FBE_LIFECYCLE_STATE_ACTIVATE,       /* ACTIVATE */
		FBE_LIFECYCLE_STATE_READY,          /* READY */
		FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
		FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
		FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
		FBE_LIFECYCLE_STATE_DESTROY)         /* DESTROY */
};


/* FBE_BASE_ENV_LIFECYCLE_COND_CHECK_PEER_REVISION condition -
*   The purpose of this condition is communicate to the peer 
*   our software revision and get the peer's software revision
*   in response.  This is intended to avoid sending the peer
*   incompatible message structures sent across CMI to the peer.
*/
static fbe_lifecycle_const_base_cond_t 
    fbe_base_environment_check_peer_revision_cond = {
        FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "fbe_base_environment_check_peer_revision_cond",
        FBE_BASE_ENV_LIFECYCLE_COND_CHECK_PEER_REVISION,
        fbe_base_environment_check_peer_revision_cond_function),
	FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
		FBE_LIFECYCLE_STATE_SPECIALIZE,     /* SPECIALIZE */
		FBE_LIFECYCLE_STATE_ACTIVATE,       /* ACTIVATE */
		FBE_LIFECYCLE_STATE_READY,          /* READY */
		FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
		FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
		FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
		FBE_LIFECYCLE_STATE_DESTROY)         /* DESTROY */
};

/* FBE_BASE_ENV_LIFECYCLE_COND_NOTIFY_PEER_PS_ALIVE condition -
*   The purpose of this condition is send notify to peer when local
*   esp object is get ready to handle communication request.
*/
static fbe_lifecycle_const_base_cond_t
    fbe_base_environment_notify_peer_sp_alive_cond = {
        FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "fbe_base_environment_notify_peer_sp_alive_cond",
        FBE_BASE_ENV_LIFECYCLE_COND_NOTIFY_PEER_PS_ALIVE,
        fbe_base_environment_notify_peer_sp_alive_cond_function),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,     /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,       /* ACTIVATE */
        FBE_LIFECYCLE_STATE_READY,          /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)         /* DESTROY */
};

/* FBE_BASE_ENV_LIFECYCLE_COND_FUP_REGISTER_CALLBACK condition -
 *   The purpose of this condition is to register with the cmi
 *   service. The leaf class needs to
 *  implement the actual condition handler function providing
 * the callback that needs to be called.
 */
static fbe_lifecycle_const_base_cond_t fbe_base_env_fup_register_callback_cond = {
        FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "fbe_base_env_fup_register_callback_cond",
        FBE_BASE_ENV_LIFECYCLE_COND_FUP_REGISTER_CALLBACK,
        NULL),
	FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
		FBE_LIFECYCLE_STATE_SPECIALIZE,     /* SPECIALIZE */
		FBE_LIFECYCLE_STATE_ACTIVATE,         /* ACTIVATE */
		FBE_LIFECYCLE_STATE_READY,         /* READY */
		FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
		FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
		FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
		FBE_LIFECYCLE_STATE_DESTROY)         /* DESTROY */
};

/* Firmware upgrade related conditions. 
 */
fbe_lifecycle_const_base_cond_t fbe_base_env_fup_terminate_upgrade_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "fbe_base_env_fup_terminate_upgrade_cond",
        FBE_BASE_ENV_LIFECYCLE_COND_FUP_TERMINATE_UPGRADE,
        fbe_base_env_fup_terminate_upgrade_cond_function),
	FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
		FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
		FBE_LIFECYCLE_STATE_ACTIVATE,         /* ACTIVATE */
		FBE_LIFECYCLE_STATE_READY,          /* READY */
		FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
		FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
		FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
		FBE_LIFECYCLE_STATE_DESTROY)         /* DESTROY */
};

fbe_lifecycle_const_base_cond_t fbe_base_env_fup_abort_upgrade_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "fbe_base_env_fup_abort_upgrade_cond",
        FBE_BASE_ENV_LIFECYCLE_COND_FUP_ABORT_UPGRADE,
        NULL),
	FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
		FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
		FBE_LIFECYCLE_STATE_ACTIVATE,         /* ACTIVATE */
		FBE_LIFECYCLE_STATE_READY,          /* READY */
		FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
		FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
		FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
		FBE_LIFECYCLE_STATE_DESTROY)         /* DESTROY */
};

fbe_lifecycle_const_base_timer_cond_t fbe_base_env_fup_wait_before_upgrade_cond = {
    {
    FBE_LIFECYCLE_DEF_CONST_BASE_TIMER_COND(
        "fbe_base_env_fup_wait_before_upgrade_cond",
        FBE_BASE_ENV_LIFECYCLE_COND_FUP_WAIT_BEFORE_UPGRADE,
        FBE_LIFECYCLE_NULL_FUNC),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,        /* ACTIVATE */
        FBE_LIFECYCLE_STATE_READY,           /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,       /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,            /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)         /* DESTROY */
    },
    FBE_LIFECYCLE_CANARY_CONST_BASE_TIMER_COND,
    6000 /* fires every 60 seconds */
};

fbe_lifecycle_const_base_cond_t fbe_base_env_fup_wait_for_inter_device_delay_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "fbe_base_env_fup_wait_for_inter_device_delay_cond",
        FBE_BASE_ENV_LIFECYCLE_COND_FUP_WAIT_FOR_INTER_DEVICE_DELAY,
        NULL),
	FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
		FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
		FBE_LIFECYCLE_STATE_ACTIVATE,         /* ACTIVATE */
		FBE_LIFECYCLE_STATE_READY,          /* READY */
		FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
		FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
		FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
		FBE_LIFECYCLE_STATE_DESTROY)         /* DESTROY */
};

fbe_lifecycle_const_base_cond_t fbe_base_env_fup_read_image_header_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "fbe_base_env_fup_read_image_header_cond",
        FBE_BASE_ENV_LIFECYCLE_COND_FUP_READ_IMAGE_HEADER,
        NULL),
	FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
		FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
		FBE_LIFECYCLE_STATE_ACTIVATE,         /* ACTIVATE */
		FBE_LIFECYCLE_STATE_READY,          /* READY */
		FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
		FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
		FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
		FBE_LIFECYCLE_STATE_DESTROY)         /* DESTROY */
};

fbe_lifecycle_const_base_cond_t fbe_base_env_fup_check_rev_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "fbe_base_env_fup_check_rev_cond",
        FBE_BASE_ENV_LIFECYCLE_COND_FUP_CHECK_REV,
        NULL),
	FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
		FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
		FBE_LIFECYCLE_STATE_ACTIVATE,         /* ACTIVATE */
		FBE_LIFECYCLE_STATE_READY,          /* READY */
		FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
		FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
		FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
		FBE_LIFECYCLE_STATE_DESTROY)         /* DESTROY */
};

fbe_lifecycle_const_base_cond_t fbe_base_env_fup_read_entire_image_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "fbe_base_env_fup_read_entire_image_cond",
        FBE_BASE_ENV_LIFECYCLE_COND_FUP_READ_ENTIRE_IMAGE,
        NULL),
	FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
		FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
		FBE_LIFECYCLE_STATE_ACTIVATE,         /* ACTIVATE */
		FBE_LIFECYCLE_STATE_READY,          /* READY */
		FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
		FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
		FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
		FBE_LIFECYCLE_STATE_DESTROY)         /* DESTROY */
};

fbe_lifecycle_const_base_cond_t fbe_base_env_fup_download_image_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "fbe_base_env_fup_download_image_cond",
        FBE_BASE_ENV_LIFECYCLE_COND_FUP_DOWNLOAD_IMAGE,
        NULL),
	FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
		FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
		FBE_LIFECYCLE_STATE_ACTIVATE,         /* ACTIVATE */
		FBE_LIFECYCLE_STATE_READY,          /* READY */
		FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
		FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
		FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
		FBE_LIFECYCLE_STATE_DESTROY)         /* DESTROY */
};

fbe_lifecycle_const_base_cond_t fbe_base_env_fup_get_download_status_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "fbe_base_env_fup_get_download_status_cond",
        FBE_BASE_ENV_LIFECYCLE_COND_FUP_GET_DOWNLOAD_STATUS,
        NULL),
	FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
		FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
		FBE_LIFECYCLE_STATE_ACTIVATE,         /* ACTIVATE */
		FBE_LIFECYCLE_STATE_READY,          /* READY */
		FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
		FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
		FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
		FBE_LIFECYCLE_STATE_DESTROY)         /* DESTROY */
};

fbe_lifecycle_const_base_cond_t fbe_base_env_fup_get_peer_permission_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "fbe_base_env_fup_get_peer_permission_cond",
        FBE_BASE_ENV_LIFECYCLE_COND_FUP_GET_PEER_PERMISSION,
        NULL),
	FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
		FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
		FBE_LIFECYCLE_STATE_ACTIVATE,         /* ACTIVATE */
		FBE_LIFECYCLE_STATE_READY,          /* READY */
		FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
		FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
		FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
		FBE_LIFECYCLE_STATE_DESTROY)         /* DESTROY */
};

fbe_lifecycle_const_base_cond_t fbe_base_env_fup_check_env_status_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "fbe_base_env_fup_check_env_status_cond",
        FBE_BASE_ENV_LIFECYCLE_COND_FUP_CHECK_ENV_STATUS,
        NULL),
	FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
		FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
		FBE_LIFECYCLE_STATE_ACTIVATE,         /* ACTIVATE */
		FBE_LIFECYCLE_STATE_READY,          /* READY */
		FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
		FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
		FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
		FBE_LIFECYCLE_STATE_DESTROY)         /* DESTROY */
};

fbe_lifecycle_const_base_cond_t fbe_base_env_fup_activate_image_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "fbe_base_env_fup_activate_image_cond",
        FBE_BASE_ENV_LIFECYCLE_COND_FUP_ACTIVATE_IMAGE,
        NULL),
	FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
		FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
		FBE_LIFECYCLE_STATE_ACTIVATE,         /* ACTIVATE */
		FBE_LIFECYCLE_STATE_READY,          /* READY */
		FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
		FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
		FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
		FBE_LIFECYCLE_STATE_DESTROY)         /* DESTROY */
};

fbe_lifecycle_const_base_cond_t fbe_base_env_fup_get_activate_status_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "fbe_base_env_fup_get_activate_status_cond",
        FBE_BASE_ENV_LIFECYCLE_COND_FUP_GET_ACTIVATE_STATUS,
        NULL),
	FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
		FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
		FBE_LIFECYCLE_STATE_ACTIVATE,         /* ACTIVATE */
		FBE_LIFECYCLE_STATE_READY,          /* READY */
		FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
		FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
		FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
		FBE_LIFECYCLE_STATE_DESTROY)         /* DESTROY */
};

fbe_lifecycle_const_base_cond_t fbe_base_env_fup_check_result_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "fbe_base_env_fup_check_result_cond",
        FBE_BASE_ENV_LIFECYCLE_COND_FUP_CHECK_RESULT,
        NULL),
	FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
		FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
		FBE_LIFECYCLE_STATE_ACTIVATE,         /* ACTIVATE */
		FBE_LIFECYCLE_STATE_READY,          /* READY */
		FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
		FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
		FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
		FBE_LIFECYCLE_STATE_DESTROY)         /* DESTROY */
};

fbe_lifecycle_const_base_cond_t fbe_base_env_fup_refresh_device_status_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "fbe_base_env_fup_refresh_device_status_cond",
        FBE_BASE_ENV_LIFECYCLE_COND_FUP_REFRESH_DEVICE_STATUS,
        NULL),
	FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
		FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
		FBE_LIFECYCLE_STATE_ACTIVATE,         /* ACTIVATE */
		FBE_LIFECYCLE_STATE_READY,          /* READY */
		FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
		FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
		FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
		FBE_LIFECYCLE_STATE_DESTROY)         /* DESTROY */
};

fbe_lifecycle_const_base_cond_t fbe_base_env_fup_end_upgrade_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "fbe_base_env_fup_end_upgrade_cond",
        FBE_BASE_ENV_LIFECYCLE_COND_FUP_END_UPGRADE,
        NULL),
	FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
		FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
		FBE_LIFECYCLE_STATE_ACTIVATE,         /* ACTIVATE */
		FBE_LIFECYCLE_STATE_READY,          /* READY */
		FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
		FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
		FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
		FBE_LIFECYCLE_STATE_DESTROY)         /* DESTROY */
};

fbe_lifecycle_const_base_cond_t fbe_base_env_fup_release_image_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "fbe_base_env_fup_release_image_cond",
        FBE_BASE_ENV_LIFECYCLE_COND_FUP_RELEASE_IMAGE,
        NULL),
	FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
		FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
		FBE_LIFECYCLE_STATE_ACTIVATE,         /* ACTIVATE */
		FBE_LIFECYCLE_STATE_READY,          /* READY */
		FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
		FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
		FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
		FBE_LIFECYCLE_STATE_DESTROY)         /* DESTROY */
};

/* FBE_BASE_ENV_LIFECYCLE_COND_RESUME_PROM_REGISTER_CALLBACK condition -
 *   The purpose of this condition is to register with the cmi
 *   service. The leaf class needs to
 *  implement the actual condition handler function providing
 *  the callback that needs to be called
 */
static fbe_lifecycle_const_base_cond_t fbe_base_env_resume_prom_register_callback_cond = {
        FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "fbe_base_env_resume_prom_register_callback_cond",
        FBE_BASE_ENV_LIFECYCLE_COND_RESUME_PROM_REGISTER_CALLBACK,
        NULL),
	FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
		FBE_LIFECYCLE_STATE_SPECIALIZE,     /* SPECIALIZE */
		FBE_LIFECYCLE_STATE_ACTIVATE,         /* ACTIVATE */
		FBE_LIFECYCLE_STATE_READY,         /* READY */
		FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
		FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
		FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
		FBE_LIFECYCLE_STATE_DESTROY)         /* DESTROY */
};

fbe_lifecycle_const_base_cond_t fbe_base_env_update_resume_prom_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "fbe_base_env_update_resume_prom_cond",
        FBE_BASE_ENV_LIFECYCLE_COND_UPDATE_RESUME_PROM,
        fbe_base_env_update_resume_prom_cond_function),
	FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
		FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
		FBE_LIFECYCLE_STATE_ACTIVATE,         /* ACTIVATE */
		FBE_LIFECYCLE_STATE_READY,          /* READY */
		FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
		FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
		FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
		FBE_LIFECYCLE_STATE_DESTROY)        /* DESTROY */
};

fbe_lifecycle_const_base_cond_t fbe_base_env_read_resume_prom_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "fbe_base_env_read_resume_prom_cond",
        FBE_BASE_ENV_LIFECYCLE_COND_READ_RESUME_PROM,
        fbe_base_env_read_resume_prom_cond_function),
	FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
		FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
		FBE_LIFECYCLE_STATE_ACTIVATE,         /* ACTIVATE */
		FBE_LIFECYCLE_STATE_READY,          /* READY */
		FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
		FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
		FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
		FBE_LIFECYCLE_STATE_DESTROY)         /* DESTROY */
};

fbe_lifecycle_const_base_cond_t fbe_base_env_check_resume_prom_outstanding_request_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "fbe_base_env_check_resume_prom_outstanding_request_cond",
        FBE_BASE_ENV_LIFECYCLE_COND_CHECK_RESUME_PROM_OUTSTANDING_REQUEST,
        fbe_base_env_check_resume_prom_outstanding_request_cond_function),
	FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
		FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
		FBE_LIFECYCLE_STATE_ACTIVATE,         /* ACTIVATE */
		FBE_LIFECYCLE_STATE_READY,         /* READY */
		FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
		FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
		FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
		FBE_LIFECYCLE_STATE_DESTROY)        /* DESTROY */
};

static fbe_lifecycle_const_base_cond_t * FBE_LIFECYCLE_BASE_COND_ARRAY(base_environment)[] = {
	&fbe_base_environment_register_state_change_callback_cond,
	&fbe_base_environment_register_cmi_callback_cond, 
	&fbe_base_environment_handle_event_notification_cond,
	&fbe_base_environment_handle_cmi_notification_cond,
    &fbe_base_environment_send_peer_revision_cond,
    &fbe_base_environment_check_peer_revision_cond,
    &fbe_base_environment_notify_peer_sp_alive_cond,
    &fbe_base_environment_initialize_persistent_storage_cond,
    &fbe_base_environment_send_peer_persistent_data_cond,
    &fbe_base_environment_send_peer_persist_write_complete_cond,
    &fbe_base_environment_write_persistent_data_cond, 
    &fbe_base_environment_read_persistent_data_cond,
    &fbe_base_environment_read_persistent_data_complete_cond,
    &fbe_base_environment_wait_peer_persistent_data_cond,
    &fbe_base_env_fup_register_callback_cond,
    &fbe_base_env_fup_terminate_upgrade_cond,
    &fbe_base_env_fup_abort_upgrade_cond,
    (fbe_lifecycle_const_base_cond_t*)&fbe_base_env_fup_wait_before_upgrade_cond,
    &fbe_base_env_fup_wait_for_inter_device_delay_cond,
    &fbe_base_env_fup_read_image_header_cond,
    &fbe_base_env_fup_check_rev_cond,
    &fbe_base_env_fup_read_entire_image_cond,
    &fbe_base_env_fup_download_image_cond,
    &fbe_base_env_fup_get_download_status_cond,
    &fbe_base_env_fup_get_peer_permission_cond,
    &fbe_base_env_fup_check_env_status_cond,
    &fbe_base_env_fup_activate_image_cond,
    &fbe_base_env_fup_get_activate_status_cond,
    &fbe_base_env_fup_check_result_cond,
    &fbe_base_env_fup_refresh_device_status_cond,
    &fbe_base_env_fup_end_upgrade_cond,
    &fbe_base_env_fup_release_image_cond,
    &fbe_base_env_resume_prom_register_callback_cond,
    &fbe_base_env_update_resume_prom_cond,
    &fbe_base_env_read_resume_prom_cond,
    &fbe_base_env_check_resume_prom_outstanding_request_cond,
};

FBE_LIFECYCLE_DEF_CONST_BASE_CONDITIONS(base_environment);

/*--- constant rotaries ----------------------------------------------------------------*/
static fbe_lifecycle_const_rotary_cond_t fbe_base_environment_specialize_rotary[] = {
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_base_environment_register_state_change_callback_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET), 
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_base_environment_register_cmi_callback_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
};

static fbe_lifecycle_const_rotary_cond_t fbe_base_environment_activate_rotary[] = {
    
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_base_environment_initialize_persistent_storage_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_base_environment_handle_cmi_notification_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_base_environment_send_peer_persistent_data_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_base_environment_send_peer_persist_write_complete_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_base_environment_write_persistent_data_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_base_environment_read_persistent_data_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_base_environment_read_persistent_data_complete_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_base_environment_send_peer_revision_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_base_environment_check_peer_revision_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_base_environment_wait_peer_persistent_data_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
};

static fbe_lifecycle_const_rotary_cond_t fbe_base_environment_ready_rotary[] = {
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_base_environment_notify_peer_sp_alive_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_base_environment_send_peer_revision_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_base_env_fup_terminate_upgrade_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_base_environment_handle_cmi_notification_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_base_environment_send_peer_persistent_data_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_base_environment_send_peer_persist_write_complete_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_base_environment_write_persistent_data_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_base_environment_read_persistent_data_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_base_environment_read_persistent_data_complete_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_base_environment_wait_peer_persistent_data_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_base_environment_handle_event_notification_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_base_env_update_resume_prom_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_base_env_read_resume_prom_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
};

static fbe_lifecycle_const_rotary_cond_t fbe_base_environment_destroy_rotary[] = {
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_base_env_update_resume_prom_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(fbe_base_env_check_resume_prom_outstanding_request_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
};

static fbe_lifecycle_const_rotary_t FBE_LIFECYCLE_ROTARY_ARRAY(base_environment)[] = {
	FBE_LIFECYCLE_DEF_CONST_ROTARY(FBE_LIFECYCLE_STATE_SPECIALIZE, fbe_base_environment_specialize_rotary),
    FBE_LIFECYCLE_DEF_CONST_ROTARY(FBE_LIFECYCLE_STATE_ACTIVATE, fbe_base_environment_activate_rotary),
    FBE_LIFECYCLE_DEF_CONST_ROTARY(FBE_LIFECYCLE_STATE_READY, fbe_base_environment_ready_rotary),
    FBE_LIFECYCLE_DEF_CONST_ROTARY(FBE_LIFECYCLE_STATE_DESTROY, fbe_base_environment_destroy_rotary),
};

FBE_LIFECYCLE_DEF_CONST_ROTARIES(base_environment);

/*--- global base board lifecycle constant data ----------------------------------------*/

fbe_lifecycle_const_t FBE_LIFECYCLE_CONST_DATA(base_environment) = {
	FBE_LIFECYCLE_DEF_CONST_DATA(
		base_environment,
		FBE_CLASS_ID_BASE_ENVIRONMENT,                  /* This class */
		FBE_LIFECYCLE_CONST_DATA(base_object))    /* The super class */
};

/*--- Condition Functions --------------------------------------------------------------*/

/*!**************************************************************
 * fbe_base_environment_handle_event_notification_cond_function()
 ****************************************************************
 * @brief
 *  This function just loops through the event queue and calls
 *  the client callback function for event notification
 * 
 * @param object_handle - This is the object handle, or in our
 * case the base_environment.
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
fbe_base_environment_handle_event_notification_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet)
{
	fbe_status_t status;
	fbe_base_environment_t *base_environment = (fbe_base_environment_t *)base_object;
	update_object_msg_t *notification_info;

	status = fbe_lifecycle_clear_current_cond(base_object);
	if (status != FBE_STATUS_OK) {
		fbe_base_object_trace(base_object, 
								FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"%s can't clear current condition, status: 0x%X\n",
								__FUNCTION__, status);
	}

	/* Loop through the event queue to check if there are any events
	 * to be notified to the leaf class. If there are, then just
	 * call the client callback that was registered before.
	 */
	fbe_spinlock_lock(&base_environment->event_element.event_queue_lock);
	while(!fbe_queue_is_empty(&base_environment->event_element.event_queue)){
		notification_info = (update_object_msg_t *)fbe_queue_pop(&base_environment->event_element.event_queue);
		fbe_spinlock_unlock(&base_environment->event_element.event_queue_lock);
		base_environment->event_element.event_call_back(notification_info, base_environment->event_element.context);
fbe_base_env_memory_ex_release(base_environment, notification_info);
		fbe_spinlock_lock(&base_environment->event_element.event_queue_lock);
	}

	fbe_spinlock_unlock(&base_environment->event_element.event_queue_lock);
	return FBE_LIFECYCLE_STATUS_DONE;
}
/*******************************************************************
 * end 
 * fbe_base_environment_handle_event_notification_cond_function() 
 *******************************************************************/

/*!**************************************************************
 * fbe_base_environment_handle_cmi_notification_cond_function()
 ****************************************************************
 * @brief
 *  This function just loops through the cmi queue and calls the
 *  client callback function for event notification
 * 
 * @param object_handle - This is the object handle, or in our
 * case the base_environment.
 * @param packet_p - The packet arriving from the monitor
 * scheduler.
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
static fbe_lifecycle_status_t 
fbe_base_environment_handle_cmi_notification_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet)
{
	fbe_status_t status;
	fbe_base_environment_t *base_environment = (fbe_base_environment_t *)base_object;
	fbe_base_environment_cmi_message_info_t *cmi_message = NULL;
    fbe_base_object_trace(base_object,FBE_TRACE_LEVEL_INFO,FBE_TRACE_MESSAGE_ID_INFO,
                          "%s function enter.\n",
                              __FUNCTION__);

    status = fbe_lifecycle_clear_current_cond(base_object);
    if (status != FBE_STATUS_OK) {
        fbe_base_object_trace(base_object, 
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s can't clear current condition, status: 0x%X\n",
                                __FUNCTION__, status);
    }
    /* Loop through the CMI queue to check if there are any cmi
	 * messages to be notified to the leaf class. If there are, then
	 * just call the client callback that was registered before.
	 */
	fbe_spinlock_lock(&base_environment->cmi_element.cmi_queue_lock);
	while(!fbe_queue_is_empty(&base_environment->cmi_element.cmi_callback_queue)){
		cmi_message = (fbe_base_environment_cmi_message_info_t *) fbe_queue_pop(&base_environment->cmi_element.cmi_callback_queue);
		fbe_spinlock_unlock(&base_environment->cmi_element.cmi_queue_lock);
		base_environment->cmi_element.cmi_callback(base_object, cmi_message);
        fbe_base_env_release_cmi_msg_memory(base_environment, cmi_message);
		fbe_spinlock_lock(&base_environment->cmi_element.cmi_queue_lock);
	}

	fbe_spinlock_unlock(&base_environment->cmi_element.cmi_queue_lock);

    return FBE_LIFECYCLE_STATUS_DONE;
        
    	
}
/*******************************************************************
 * end
 * fbe_base_environment_handle_cmi_notification_cond_function()
 *******************************************************************/

/*!**************************************************************
 * fbe_base_environment_peer_revision_response_cond_function()
 ****************************************************************
 * @brief
 *  This function sends a message to the peer notifying it of our
 *  software revision and checking the peer's software revision in
 *  response.
 * 
 * @param object_handle - This is the object handle, or in our
 * case the base_environment.
 * @param packet_p - The packet arriving from the monitor
 * scheduler.
 *
 * @return fbe_status_t - The status of the call to
 *                        fbe_lifecycle_class_const_verify()
 *                        for the enclosure management's
 *                        constant lifecycle data.
 *
 * @author
 *  02-May-2013:  Created. Brion Philbin
 *
 ****************************************************************/
fbe_lifecycle_status_t 
fbe_base_environment_send_peer_revision_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet)
{
    fbe_status_t status;
	fbe_base_environment_t *base_environment = (fbe_base_environment_t *)base_object;
    fbe_base_environment_peer_revision_cmi_message_t message;

    fbe_base_object_trace(base_object, 
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s entry requestPeerRevision:%d\n",
                          __FUNCTION__, base_environment->requestPeerRevision);

    status = fbe_lifecycle_clear_current_cond(base_object);
    if (status != FBE_STATUS_OK) {
        fbe_base_object_trace(base_object, 
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s can't clear current condition, status: 0x%X\n",
                                __FUNCTION__, status);
    }

    /*
     * Send a message to the peer containing our revision
     */
    
    
    fbe_zero_memory(&message, sizeof(fbe_base_environment_peer_revision_cmi_message_t));

    message.msg.opcode = FBE_BASE_ENV_CMI_MSG_PEER_REVISION_RESPONSE;

    /*
     * Use the persistent_entry_id we got when we read the data, or a default we set if there was
     * no data found previously.
     */
    message.revision = FBE_BASE_ENV_SOFTWARE_REVISION;

    status = fbe_base_environment_send_cmi_message(base_environment,
                                                   sizeof(fbe_base_environment_peer_revision_cmi_message_t), 
                                                   &message);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *) base_environment, 
                            FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                            "%s Failed sending cmi message with status 0x%x\n",
                            __FUNCTION__, status);
    }


    return FBE_LIFECYCLE_STATUS_DONE;

}


/*!**************************************************************
 * fbe_base_environment_check_peer_revision_cond_function()
 ****************************************************************
 * @brief
 *  This function sends a message to the peer notifying it of our
 *  software revision and checking the peer's software revision in
 *  response.
 * 
 * @param object_handle - This is the object handle, or in our
 * case the base_environment.
 * @param packet_p - The packet arriving from the monitor
 * scheduler.
 *
 * @return fbe_status_t - The status of the call to
 *                        fbe_lifecycle_class_const_verify()
 *                        for the enclosure management's
 *                        constant lifecycle data.
 *
 * @author
 *  02-May-2013:  Created. Brion Philbin
 *
 ****************************************************************/
fbe_lifecycle_status_t 
fbe_base_environment_check_peer_revision_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet)
{
    fbe_status_t status;
    fbe_base_environment_t *base_environment = (fbe_base_environment_t *)base_object;
    fbe_base_environment_peer_revision_cmi_message_t message;

    fbe_base_object_trace(base_object, 
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s entry requestPeerRevision:%d\n",
                          __FUNCTION__, base_environment->requestPeerRevision);

    if( (base_environment->requestPeerRevision == PEER_REVISION_RESPONSE_RECEIVED) ||
             (!fbe_cmi_is_peer_alive()))
    {
        /*
         * We got our response from the peer, clear the condition and 
         * complete here.
         */
        fbe_base_object_trace(base_object, 
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s peer is running Version %d of ESP software.\n",
                                  __FUNCTION__, base_environment->peerRevision);

        status = fbe_lifecycle_clear_current_cond(base_object);
        if (status != FBE_STATUS_OK)
        {
            fbe_base_object_trace(base_object, 
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s can't clear current condition, status: 0x%X\n",
                                  __FUNCTION__, status);
         }
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /*
     * Send a message to the peer containing our revision and requesting the peer's revision
     */
    base_environment->requestPeerRevision = WAIT_PEER_REVISION_RESPONSE;
    
    fbe_zero_memory(&message, sizeof(fbe_base_environment_peer_revision_cmi_message_t));

    message.msg.opcode = FBE_BASE_ENV_CMI_MSG_REQUEST_PEER_REVISION;

    /*
     * Use the persistent_entry_id we got when we read the data, or a default we set if there was
     * no data found previously.
     */
    message.revision = FBE_BASE_ENV_SOFTWARE_REVISION;

    status = fbe_base_environment_send_cmi_message(base_environment,
                                                   sizeof(fbe_base_environment_peer_revision_cmi_message_t),
                                                   &message);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *) base_environment,
                            FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                            "%s Failed sending cmi message with status 0x%x\n",
                            __FUNCTION__, status);
    }

    return FBE_LIFECYCLE_STATUS_DONE;

}




/*!**************************************************************
 * fbe_base_environment_initialize_persistent_storage_cond_function()
 ****************************************************************
 * @brief
 *  This function initializes the persistent storage information
 *  and kicks off the process of loading the persistent data for
 *  this object.
 * 
 * @param object_handle - This is the object handle, or in our
 * case the base_environment.
 * @param packet_p - The packet arriving from the monitor
 * scheduler.
 *
 * @return fbe_status_t - The status of the call to
 *                        fbe_lifecycle_class_const_verify()
 *                        for the enclosure management's
 *                        constant lifecycle data.
 *
 * @author
 *  30-Dec-2010:  Created. Brion Philbin
 *
 ****************************************************************/
fbe_lifecycle_status_t 
fbe_base_environment_initialize_persistent_storage_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet)
{
    fbe_status_t status;
    fbe_status_t cmi_status;
    fbe_base_environment_t *base_environment = (fbe_base_environment_t *)base_object;
    fbe_cmi_service_get_info_t  get_cmi_info;

    fbe_base_object_trace(base_object,FBE_TRACE_LEVEL_INFO,FBE_TRACE_MESSAGE_ID_INFO,
                          "%s function enter.\n",
                              __FUNCTION__);

    /*
     * We need to be able to determine what SEP thinks is the ACTIVE vs. PASSIVE
     * SP.  This can be different from the ESP determination of things.  Delay in
     * this condition until we can determine this information.
     */

    memset(&get_cmi_info, 0, sizeof(get_cmi_info)); // SAFEBUG - uninitialized fields seen to be used

    if (!fbe_cmi_service_disabled())
    {
       cmi_status = fbe_api_cmi_service_get_info(&get_cmi_info);
    } 
    else 
    {
       cmi_status = FBE_STATUS_COMPONENT_NOT_FOUND;
       fbe_base_object_trace(base_object,
                             FBE_TRACE_LEVEL_INFO,
                             FBE_TRACE_MESSAGE_ID_INFO,
                             "%s boot in service mode\n",
                             __FUNCTION__);
    }

    if( ((get_cmi_info.sp_state != FBE_CMI_STATE_ACTIVE && 
            get_cmi_info.sp_state != FBE_CMI_STATE_PASSIVE && 
            get_cmi_info.sp_state != FBE_CMI_STATE_SERVICE_MODE) ||
            cmi_status != FBE_STATUS_OK) && 
           (cmi_status != FBE_STATUS_COMPONENT_NOT_FOUND))
    {
        // go no further and allow this condition to be rescheduled and checked again.

        if(cmi_status != FBE_STATUS_OK)
        {
            fbe_base_object_trace(base_object,
                                      FBE_TRACE_LEVEL_INFO,
                                      FBE_TRACE_MESSAGE_ID_INFO,
                                      "%s Error in getting cmi SP State, status:0x%x\n",
                                      __FUNCTION__, cmi_status);
        }
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    fbe_base_object_trace(base_object,
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s detected cmi_state %d\n",
                          __FUNCTION__, get_cmi_info.sp_state);


    status = fbe_base_environment_init_persistent_storage_data(base_environment, packet);

    if(status == FBE_STATUS_OK)
    {
        /*
         * Now that we have initialized we need to kick off the first read
         */
        if((cmi_status == FBE_STATUS_COMPONENT_NOT_FOUND) || base_environment->persist_db_disabled)
        {
            if ( ! base_environment->persist_db_disabled )
            {
                // There is no SEP CMI service because the SEP package has not loaded
                fbe_base_object_trace(base_object,
                                      FBE_TRACE_LEVEL_WARNING,
                                      FBE_TRACE_MESSAGE_ID_INFO,
                                      "%s SEP not detected.\n",
                                      __FUNCTION__);
            } else {
                // Virtual platform, SEP not present
                fbe_base_object_trace(base_object, 
                                      FBE_TRACE_LEVEL_INFO,
                                      FBE_TRACE_MESSAGE_ID_INFO,
                                      "%s SEP disabled. Virtual platform.\n",
                                      __FUNCTION__);

            }
    
            /*
             *  There is no persistent storage available for us to use,
             *  clear the condition and move on.
             */
            status = fbe_lifecycle_clear_current_cond(base_object);
            if (status != FBE_STATUS_OK) {
                fbe_base_object_trace(base_object, 
                                        FBE_TRACE_LEVEL_ERROR,
                                        FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                        "%s can't clear current condition, status: 0x%X\n",
                                        __FUNCTION__, status);
            }
            return FBE_LIFECYCLE_STATUS_DONE;
        }
        status = fbe_base_env_read_persistent_data(base_environment);
        if(status != FBE_STATUS_OK)
        {
            fbe_base_object_trace(base_object,
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s Error in setting READ_PERSISTENT_DATA condition, status:0x%x\n",
                                  __FUNCTION__, status);
        }
    }

    status = fbe_lifecycle_clear_current_cond(base_object);
    if (status != FBE_STATUS_OK) {
        fbe_base_object_trace(base_object, 
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s can't clear current condition, status: 0x%X\n",
                                __FUNCTION__, status);
    }
    return FBE_LIFECYCLE_STATUS_DONE;
}
/*********************************************************
 * end 
 * fbe_base_environment_initialize_persistent_storage_cond_function()
 ********************************************************/

/*!**************************************************************
 * fbe_base_environment_read_persistent_data_cond_function()
 ****************************************************************
 * @brief
 *  This function checks if the peer is writing to the persistent
 *  storage.  If not then it reads from the persistent stroage
 *  service into memory the persistent data.  It does this by
 *  reading all the persistent data for ESP and traverses looking
 *  for the data with a header specifying this object's class id.
 * 
 * @param object_handle - This is the object handle, or in our
 * case the base_environment.
 * @param packet_p - The packet arriving from the monitor
 * scheduler.
 *
 * @return fbe_status_t - The status of the call to
 *                        fbe_lifecycle_class_const_verify()
 *                        for the enclosure management's
 *                        constant lifecycle data.
 *
 * @author
 *  30-Dec-2010:  Created. Brion Philbin
 *
 ****************************************************************/
static fbe_lifecycle_status_t
fbe_base_environment_read_persistent_data_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet)
{
    fbe_status_t status;
	fbe_base_environment_t *base_environment = (fbe_base_environment_t *)base_object;

    fbe_base_object_trace(base_object,FBE_TRACE_LEVEL_INFO,FBE_TRACE_MESSAGE_ID_INFO,
                          "%s function enter.\n",
                              __FUNCTION__);

    if(base_environment->persist_db_disabled)
    {
        fbe_base_object_trace(base_object,FBE_TRACE_LEVEL_INFO,FBE_TRACE_MESSAGE_ID_INFO,
                          "%s no persistent storage access for read.\n",
                              __FUNCTION__);

        status = fbe_lifecycle_clear_current_cond((fbe_base_object_t *)base_environment);
        if(status != FBE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t *) base_environment, 
                                      FBE_TRACE_LEVEL_ERROR,
                                      FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                      "%s failed to clear condition, status: 0x%X\n",
                                      __FUNCTION__, status);
        }
        return FBE_LIFECYCLE_STATUS_DONE;

    }
    if(base_environment->read_outstanding == TRUE)
    {
        return FBE_LIFECYCLE_STATUS_PENDING;
    }

    

    /* We just push additional context on the monitor packet stack */
    status = fbe_transport_set_completion_function(packet, fbe_base_environment_read_persistent_data_cond_completion, base_environment);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *) base_environment, 
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s can't set completion function, status: 0x%X\n",
                                  __FUNCTION__, status);
    }

    status = fbe_base_environment_send_read_persistent_data(base_environment, packet);
    if( (status != FBE_STATUS_OK) && (status != FBE_STATUS_PENDING) )
    {
        /*
         * We have finished here but with a failure, leave the condition set and
         * return a done to the lifecycle handler.
         */
        return FBE_LIFECYCLE_STATUS_DONE;
    }
    else
    {
        /* 
         * The reads are asynchronous, so we leave the condition set until
         * the read completes and retry if it failed.
         */
        return FBE_LIFECYCLE_STATUS_PENDING;
    }
}
/*******************************************************************
 * end
 * fbe_base_environment_read_persistent_data_cond_function()
 *******************************************************************/

/*!**************************************************************
 * fbe_base_environment_read_persistent_data_cond_completion()
 ****************************************************************
 * @brief
 *  This function checks the status of the persistent read,
 *  completes the read condition, and sets a condition notifying
 *  the leaf class that new data is available.
 * 
 * @param context - This is the object handle, or in our
 * case the base_environment.
 * @param packet - The packet arriving from the monitor
 * scheduler.
 *
 * @return fbe_status_t - The status of the call to
 *                        fbe_lifecycle_class_const_verify()
 *                        for the enclosure management's
 *                        constant lifecycle data.
 *
 * @author
 *  30-Dec-2010:  Created. Brion Philbin
 *
 ****************************************************************/
static fbe_status_t fbe_base_environment_read_persistent_data_cond_completion(fbe_packet_t * packet, 
                                                                              fbe_packet_completion_context_t context)
{
    fbe_base_environment_t *base_environment;
    fbe_status_t status;
    

    base_environment = (fbe_base_environment_t *)context;
    base_environment->read_outstanding = FALSE;
    
    status = fbe_transport_get_status_code(packet);

    fbe_base_object_trace((fbe_base_object_t *)base_environment,FBE_TRACE_LEVEL_INFO,FBE_TRACE_MESSAGE_ID_INFO,
                          "%s function enter read status %d.\n",
                              __FUNCTION__, status);

    if(status == FBE_STATUS_OK)
    {
        // reset the retry counter as we have a good read here
        base_environment->read_persistent_data_retries = 0;

        status = fbe_lifecycle_clear_current_cond((fbe_base_object_t *)base_environment);
        
        if(status != FBE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t *) base_environment, 
                                      FBE_TRACE_LEVEL_ERROR,
                                      FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                      "%s failed to clear condition, status: 0x%X\n",
                                      __FUNCTION__, status);
        }
        status = fbe_lifecycle_set_cond(&fbe_base_environment_lifecycle_const,
                                                (fbe_base_object_t*)base_environment,
                                                FBE_BASE_ENV_LIFECYCLE_COND_READ_PERSISTENT_DATA_COMPLETE);
        if(status != FBE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t *) base_environment, 
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s can't set READ_PERSISTENT_DATA_COMPLETE condition, status: 0x%X\n",
                                  __FUNCTION__, status);
        }
        if(fbe_cmi_is_peer_alive())
        {
           // set condition to notify the peer of the updated data
            
            status = fbe_lifecycle_set_cond(&fbe_base_environment_lifecycle_const,
                                                (fbe_base_object_t*)base_environment,
                                                FBE_BASE_ENV_LIFECYCLE_COND_SEND_PEER_PERSISTENT_DATA);
            if(status != FBE_STATUS_OK)
            {
                fbe_base_object_trace((fbe_base_object_t *) base_environment, 
                                      FBE_TRACE_LEVEL_ERROR,
                                      FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                      "%s can't set SEND_PEER_PERSISTENT_DATA condition, status: 0x%X\n",
                                      __FUNCTION__, status);
            }
        }
    }
    else
    {
        if(base_environment->read_persistent_data_retries > FBE_BASE_ENVIRONMENT_MAX_PERSISTENT_DATA_READ_RETRIES)
        {
            /*
             *  We have exceeded the maximum number of attempts to read the persistent data, we will clear the condition and remain
             *  in our current lifecycle state.
             */
            fbe_base_object_trace((fbe_base_object_t *) base_environment, 
                                          FBE_TRACE_LEVEL_WARNING,
                                          FBE_TRACE_MESSAGE_ID_INFO,
                                          "%s MAX RETRIES EXCEEDED, PERSISTENT DATA NOT LOADED!\n",
                                          __FUNCTION__);
            status = fbe_lifecycle_clear_current_cond((fbe_base_object_t *)base_environment);
            if(status != FBE_STATUS_OK)
            {
                fbe_base_object_trace((fbe_base_object_t *) base_environment, 
                                          FBE_TRACE_LEVEL_ERROR,
                                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                          "%s failed to clear condition, status: 0x%X\n",
                                          __FUNCTION__, status);
            }
        }
        base_environment->read_persistent_data_retries++;
    }
    return FBE_STATUS_OK;
}
/*********************************************************
 * end 
 * fbe_base_environment_read_persistent_data_cond_completion()
 ********************************************************/


/*!**************************************************************
 * fbe_base_environment_send_peer_persistent_data_cond_function()
 ****************************************************************
 * @brief
 *  This function copies the data read or written to the persistent
 *  storage and sends that data to the peer.  
 * 
 * @param object_handle - This is the object handle, or in our
 * case the base_environment.
 * @param packet_p - The packet arriving from the monitor
 * scheduler.
 *
 * @return fbe_status_t - The status of the call to
 *                        fbe_lifecycle_class_const_verify()
 *                        for the enclosure management's
 *                        constant lifecycle data.
 *
 * @author
 *  04-Sep-2012:  Created. Brion Philbin
 *
 ****************************************************************/
static fbe_lifecycle_status_t fbe_base_environment_send_peer_persistent_data_cond_function(fbe_base_object_t * base_object, 
                                                                                 fbe_packet_t * packet)
{
    fbe_status_t status;
	fbe_base_environment_t *base_environment = (fbe_base_environment_t *)base_object;
    fbe_base_environment_persistent_data_cmi_message_t *message;

    status = fbe_lifecycle_clear_current_cond((fbe_base_object_t *)base_environment);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *) base_environment, 
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s failed to clear condition, status: 0x%X\n",
                                  __FUNCTION__, status);
    }
     /* Send a cmi message to the peer to notify that the write has completed. */
    message = fbe_base_env_memory_ex_allocate(base_environment, FBE_BASE_ENVIRONMENT_PERSIST_MESSAGE_SIZE(base_environment));
    if(message == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t *) base_environment, 
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s failed to allocate memory for cmi message.\n",
                                __FUNCTION__);
    }
    else
    {
        fbe_zero_memory(message, FBE_BASE_ENVIRONMENT_PERSIST_MESSAGE_SIZE(base_environment));

        message->msg.opcode = FBE_BASE_ENV_CMI_MSG_PERSISTENT_READ_COMPLETE;

        /*
         * Use the persistent_entry_id we got when we read the data, or a default we set if there was
         * no data found previously.
         */
        message->persistent_data.persist_header.entry_id = base_environment->my_persistent_entry_id;
        message->persistent_data.header.persistent_id = base_environment->my_persistent_id;
    
        fbe_copy_memory(&message->persistent_data.header.data, 
                         base_environment->my_persistent_data, 
                         base_environment->my_persistent_data_size);
        status = fbe_base_environment_send_cmi_message(base_environment,
                                                       FBE_BASE_ENVIRONMENT_PERSIST_MESSAGE_SIZE(base_environment), 
                                                       message);
        if(status != FBE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t *) base_environment, 
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s Failed sending cmi message with status 0x%x\n",
                                __FUNCTION__, status);
        }
        fbe_base_env_memory_ex_release(base_environment, message);
    }
    return FBE_LIFECYCLE_STATUS_DONE;
}
/*********************************************************
 * end 
 * fbe_base_environment_send_peer_persistent_data_cond_function()
 ********************************************************/


/*!**************************************************************
 * fbe_base_environment_send_peer_persist_write_complete_cond_function()
 ****************************************************************
 * @brief
 *  This function sends a message to the peer to notify it that
 *  we have completed it's persistent write request.  
 * 
 * @param object_handle - This is the object handle, or in our
 * case the base_environment.
 * @param packet_p - The packet arriving from the monitor
 * scheduler.
 *
 * @return fbe_status_t - The status of the call to
 *                        fbe_lifecycle_class_const_verify()
 *                        for the enclosure management's
 *                        constant lifecycle data.
 *
 * @author
 *  22-April-2013:  Created. Brion Philbin
 *
 ****************************************************************/
static fbe_lifecycle_status_t fbe_base_environment_send_peer_persist_write_complete_cond_function(fbe_base_object_t * base_object, 
                                                                                 fbe_packet_t * packet)
{
    fbe_status_t status;
	fbe_base_environment_t *base_environment = (fbe_base_environment_t *)base_object;
    fbe_base_environment_persistent_data_cmi_message_t *message;

    fbe_base_object_trace(base_object,FBE_TRACE_LEVEL_INFO,FBE_TRACE_MESSAGE_ID_INFO,
                          "%s function enter.\n",
                              __FUNCTION__);

    status = fbe_lifecycle_clear_current_cond((fbe_base_object_t *)base_environment);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *) base_environment, 
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s failed to clear condition, status: 0x%X\n",
                                  __FUNCTION__, status);
    }
     /* Send a cmi message to the peer to notify that the write has completed. */
    message = fbe_base_env_memory_ex_allocate(base_environment, FBE_BASE_ENVIRONMENT_PERSIST_MESSAGE_SIZE(base_environment));
    if(message == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t *) base_environment, 
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s failed to allocate memory for cmi message.\n",
                                __FUNCTION__);
    }
    else
    {
        fbe_zero_memory(message, FBE_BASE_ENVIRONMENT_PERSIST_MESSAGE_SIZE(base_environment));

        message->msg.opcode = FBE_BASE_ENV_CMI_MSG_PERSISTENT_WRITE_COMPLETE;

       status = fbe_base_environment_send_cmi_message(base_environment,
                                                       FBE_BASE_ENVIRONMENT_PERSIST_MESSAGE_SIZE(base_environment), 
                                                       message);
        if(status != FBE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t *) base_environment, 
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s Failed sending cmi message with status 0x%x\n",
                                __FUNCTION__, status);
        }
        fbe_base_env_memory_ex_release(base_environment, message);
    }
    return FBE_LIFECYCLE_STATUS_DONE;
}
/*********************************************************
 * end 
 * fbe_base_environment_send_peer_persistent_data_cond_function()
 ********************************************************/


/*!**************************************************************
 * fbe_base_environment_wait_peer_persistent_data_cond_function()
 ****************************************************************
 * @brief
 *  This function handles the wait for peer persistent data.  
 *  The condition clears when the peer SP either goes away
 *  or responds with the persistent data read from disk.
 * 
 * @param context - This is the object handle, or in our
 * case the base_environment.
 * @param packet - The packet arriving from the monitor
 * scheduler.
 *
 * @return fbe_status_t - The status of the call to
 *                        fbe_lifecycle_class_const_verify()
 *                        for the enclosure management's
 *                        constant lifecycle data.
 *
 * @author
 *  24-Mar-2011:  Created. Brion Philbin
 *
 ****************************************************************/
static fbe_lifecycle_status_t fbe_base_environment_wait_peer_persistent_data_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet)
{
    fbe_status_t status;
	fbe_base_environment_t *base_environment = (fbe_base_environment_t *)base_object;

    fbe_base_object_trace(base_object,FBE_TRACE_LEVEL_INFO,FBE_TRACE_MESSAGE_ID_INFO,
                          "%s function enter.\n",
                              __FUNCTION__);

    if(!base_environment->wait_for_peer_data)
    {
        status = fbe_lifecycle_clear_current_cond(base_object);
    	if (status != FBE_STATUS_OK) 
        {
    		fbe_base_object_trace(base_object, 
    								FBE_TRACE_LEVEL_ERROR,
    								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
    								"%s can't clear current condition, status: 0x%X\n",
    								__FUNCTION__, status);
    	}
    }
    return FBE_LIFECYCLE_STATUS_DONE;

}


/*!**************************************************************
 * fbe_base_environment_write_persistent_data_cond_function()
 ****************************************************************
 * @brief
 *  This function assumes we have gotten the ok from the peer sp
 *  to do the write.  We will just go ahead here and write the data
 *  out to the persistent storage service.
 * 
 * @param object_handle - This is the object handle, or in our
 * case the base_environment.
 * @param packet_p - The packet arriving from the monitor
 * scheduler.
 *
 * @return fbe_status_t - The status of the call to
 *                        fbe_lifecycle_class_const_verify()
 *                        for the enclosure management's
 *                        constant lifecycle data.
 *
 * @author
 *  30-Dec-2010:  Created. Brion Philbin
 *
 ****************************************************************/
static fbe_lifecycle_status_t
fbe_base_environment_write_persistent_data_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet)
{
    fbe_status_t status;
	fbe_base_environment_t *base_environment = (fbe_base_environment_t *)base_object;

    fbe_base_object_trace(base_object,FBE_TRACE_LEVEL_INFO,FBE_TRACE_MESSAGE_ID_INFO,
                          "%s function enter.\n",
                              __FUNCTION__);

    if(base_environment->persist_db_disabled)
    {
        fbe_base_object_trace(base_object,FBE_TRACE_LEVEL_INFO,FBE_TRACE_MESSAGE_ID_INFO,
                          "%s no persistent storage access for write.\n",
                              __FUNCTION__);

        status = fbe_lifecycle_clear_current_cond((fbe_base_object_t *)base_environment);
        if(status != FBE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t *) base_environment, 
                                      FBE_TRACE_LEVEL_ERROR,
                                      FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                      "%s failed to clear condition, status: 0x%X\n",
                                      __FUNCTION__, status);
        }
        return FBE_LIFECYCLE_STATUS_DONE;

    }

    // issue the write to the persistent storage service
    status = fbe_base_environment_send_write_persistent_data(base_environment, packet);

    if(status == FBE_STATUS_OK)
    {
        /* We just push additional context on the monitor packet stack */
        status = fbe_transport_set_completion_function(packet, fbe_base_environment_write_persistent_data_cond_completion, base_environment);
        if(status != FBE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t *) base_environment, 
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s can't set completion function, status: 0x%X\n",
                                  __FUNCTION__, status);
        }
        return FBE_LIFECYCLE_STATUS_PENDING;
    }
    else
    {
        /* We did not send the write successfully, do not clear the condition */
        return FBE_LIFECYCLE_STATUS_DONE;
    }
}

/*******************************************************************
 * end
 * fbe_base_environment_write_persistent_data_cond_function()
 *******************************************************************/

/*!**************************************************************
 * fbe_base_environment_write_persistent_data_cond_completion()
 ****************************************************************
 * @brief
 *  This function checks the status of the persistent write commit,
 *  completes the write condition, and notifies the peer of
 *  the new data.
 * 
 * @param context - This is the object handle, or in our
 * case the base_environment.
 * @param packet - The packet arriving from the monitor
 * scheduler.
 *
 * @return fbe_status_t - The status of the call to
 *                        fbe_lifecycle_class_const_verify()
 *                        for the enclosure management's
 *                        constant lifecycle data.
 *
 * @author
 *  30-Dec-2010:  Created. Brion Philbin
 *
 ****************************************************************/
static fbe_status_t
fbe_base_environment_write_persistent_data_cond_completion(fbe_packet_t * packet, 
                                                           fbe_packet_completion_context_t context)
{
    fbe_base_environment_t *base_environment;
    fbe_status_t status;

    base_environment = (fbe_base_environment_t *)context;
    fbe_base_object_trace((fbe_base_object_t *)base_environment,FBE_TRACE_LEVEL_INFO,FBE_TRACE_MESSAGE_ID_INFO,
                          "%s function enter.\n",
                              __FUNCTION__);
   
    
    status = fbe_transport_get_status_code(packet);

    if(status == FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)base_environment,FBE_TRACE_LEVEL_INFO,FBE_TRACE_MESSAGE_ID_INFO,
                              "%s write completed successfully.\n",
                                  __FUNCTION__);

        status = fbe_lifecycle_clear_current_cond((fbe_base_object_t *)base_environment);
        if(status != FBE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t *) base_environment,
                                      FBE_TRACE_LEVEL_ERROR,
                                      FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                      "%s can't clear current condition, status: 0x%X\n",
                                      __FUNCTION__, status);
        }
        if(fbe_cmi_is_peer_alive())
        {
            // set the condition to notify the peer that a write has completed.
            status = fbe_lifecycle_set_cond(&fbe_base_environment_lifecycle_const,
                                                (fbe_base_object_t*)base_environment,
                                                FBE_BASE_ENV_LIFECYCLE_COND_SEND_PEER_PERSIST_WRITE_COMPLETE);
            if(status != FBE_STATUS_OK)
            {
                fbe_base_object_trace((fbe_base_object_t *) base_environment, 
                                      FBE_TRACE_LEVEL_ERROR,
                                      FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                      "%s can't set SEND_PEER_PERSIST_WRITE_COMPLETE condition, status: 0x%X\n",
                                      __FUNCTION__, status);
            }

            
           // set condition to update our local in memory copy of the data and notify the peer of the updated data
            status = fbe_lifecycle_set_cond(&fbe_base_environment_lifecycle_const,
                                                (fbe_base_object_t*)base_environment,
                                                FBE_BASE_ENV_LIFECYCLE_COND_READ_PERSISTENT_DATA);
            if(status != FBE_STATUS_OK)
            {
                fbe_base_object_trace((fbe_base_object_t *) base_environment, 
                                      FBE_TRACE_LEVEL_ERROR,
                                      FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                      "%s can't set READ_PERSISTENT_DATA condition, status: 0x%X\n",
                                      __FUNCTION__, status);
            }
        }
    }

    return FBE_STATUS_OK;
}

/*********************************************************
 * end 
 * fbe_base_environment_write_persistent_data_cond_completion()
 ********************************************************/


/*!**************************************************************
 * fbe_base_environment_cmi_message_handler()
 ****************************************************************
 * @brief
 *  This function handles base_environment related cmi message
 *  events.  
 * 
 * @param object_handle - This is the object handle, or in our
 * case the base_environment.
 * @param packet_p - The packet arriving from the monitor
 * scheduler.
 *
 * @return fbe_status_t - The status of the call to
 *                        fbe_lifecycle_class_const_verify()
 *                        for the enclosure management's
 *                        constant lifecycle data.
 *
 * @author
 *  30-Dec-2010:  Created. Brion Philbin
 *
 ****************************************************************/
fbe_status_t 
fbe_base_environment_cmi_message_handler(fbe_base_object_t *base_object, 
                                 fbe_base_environment_cmi_message_info_t *cmi_message)
{
    fbe_base_environment_t *base_environment;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_base_environment_cmi_message_t *base_env_cmi_msg;

    base_env_cmi_msg = cmi_message->user_message;

    base_environment = (fbe_base_environment_t *)base_object;

    fbe_base_object_trace((fbe_base_object_t *) base_environment, 
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s received message event:%d from the peer.\n",
                          __FUNCTION__,
                          cmi_message->event);

    switch(cmi_message->event)
    {
    case FBE_CMI_EVENT_MESSAGE_RECEIVED:
        status = fbe_base_environment_cmi_process_received_message(base_environment, base_env_cmi_msg);
        break;

    case FBE_CMI_EVENT_MESSAGE_TRANSMITTED:
        break;

    case FBE_CMI_EVENT_PEER_NOT_PRESENT:
        fbe_base_environment_cmi_processPeerNotPresent(base_environment, base_env_cmi_msg);
        break;
        
	case FBE_CMI_EVENT_SP_CONTACT_LOST:
	case FBE_CMI_EVENT_PEER_BUSY:
        fbe_base_environment_peer_gone((fbe_base_environment_t *)base_object);
        break;

    case FBE_CMI_EVENT_CLOSE_COMPLETED:
    case FBE_CMI_EVENT_DMA_ADDRESSES_NEEDED:
    case FBE_CMI_EVENT_FATAL_ERROR:
    case FBE_CMI_EVENT_INVALID:
    default:
        fbe_base_object_trace(base_object, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s Error sending CMI message 0x%llx, event 0x%x\n",
                               __FUNCTION__, 
                              (unsigned long long)base_env_cmi_msg->opcode,
                              cmi_message->event);
        status = FBE_STATUS_GENERIC_FAILURE;
        break;
    }
    return status;

}
/*********************************************************
 * end 
 * fbe_base_environment_cmi_message_handler()
 ********************************************************/

/*!**************************************************************
 * fbe_base_environment_cmi_process_received_message()
 ****************************************************************
 * @brief
 *  This function handles base_environment specific cmi messages
 * 
 * @param object_handle - This is the object handle, or in our
 * case the base_environment.
 * @param cmi_message - The base environment cmi message
 *
 * @return fbe_status_t - The status of the call to
 *                        fbe_lifecycle_class_const_verify()
 *                        for the enclosure management's
 *                        constant lifecycle data.
 *
 * @author
 *  30-Dec-2010:  Created. Brion Philbin
 *
 ****************************************************************/
fbe_status_t fbe_base_environment_cmi_process_received_message(fbe_base_environment_t *base_environment, 
                                                  fbe_base_environment_cmi_message_t *cmi_message)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_base_environment_persistent_data_cmi_message_t *peer_msg = (fbe_base_environment_persistent_data_cmi_message_t *)cmi_message;
    fbe_base_environment_peer_revision_cmi_message_t *peer_rev_msg = (fbe_base_environment_peer_revision_cmi_message_t *)cmi_message;
    fbe_base_env_fup_cmi_packet_t *pBaseFupCmiPkt = (fbe_base_env_fup_cmi_packet_t *)cmi_message;

    fbe_base_object_trace((fbe_base_object_t *) base_environment, 
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s received message opcode:%s(0x%llX) from the peer.\n",
                          __FUNCTION__, 
                          fbe_base_env_decode_cmi_msg_opcode(cmi_message->opcode),
                          (unsigned long long)cmi_message->opcode);

    switch(cmi_message->opcode)
    {
    case FBE_BASE_ENV_CMI_MSG_PERSISTENT_READ_REQUEST:
        /*
         * Peer is requesting the persistent data.
         */
        status = fbe_lifecycle_set_cond(&fbe_base_environment_lifecycle_const,
                                        (fbe_base_object_t*)base_environment,
                                        FBE_BASE_ENV_LIFECYCLE_COND_READ_PERSISTENT_DATA);
        if(status != FBE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t *) base_environment, 
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s Failed setting LIFECYCLE_COND_READ_PERSISTENT_DATA status:0x%x\n",
                                __FUNCTION__, status);
        }
        break;
    case FBE_BASE_ENV_CMI_MSG_PERSISTENT_READ_COMPLETE:
        if(base_environment->wait_for_peer_data != PERSISTENT_DATA_WAIT_WRITE)
        {
            // as long as we aren't waiting for our own write to complete we can process whatever the peer read
            status = fbe_lifecycle_set_cond(&fbe_base_environment_lifecycle_const,
                                                    (fbe_base_object_t*)base_environment,
                                                    FBE_BASE_ENV_LIFECYCLE_COND_READ_PERSISTENT_DATA_COMPLETE);
            if(base_environment->my_persistent_data)
            {
                /*
                 * Copy the peer's data into our persistent data storage if 
                 * it has been initialized.
                 */

                fbe_base_object_trace((fbe_base_object_t *) base_environment, 
                                FBE_TRACE_LEVEL_INFO,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "BaseEnv proc cmi msg: got read complete msg for entry:0x%llx\n",
                                peer_msg->persistent_data.persist_header.entry_id);

                base_environment->my_persistent_entry_id = peer_msg->persistent_data.persist_header.entry_id;
                fbe_copy_memory(base_environment->my_persistent_data,
                                &peer_msg->persistent_data.header.data,  
                                base_environment->my_persistent_data_size);
                base_environment->wait_for_peer_data = FALSE;
            }
        }
        break;
    case FBE_BASE_ENV_CMI_MSG_PERSISTENT_WRITE_COMPLETE:
        fbe_base_object_trace((fbe_base_object_t *) base_environment, 
                                FBE_TRACE_LEVEL_INFO,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "BaseEnv proc cmi msg: got write complete from Peer\n");
        base_environment->wait_for_peer_data = PERSISTENT_DATA_WAIT_NONE;
        break;
       
        break;
    case FBE_BASE_ENV_CMI_MSG_PERSISTENT_WRITE_REQUEST:
        /*
         * Peer is requesting we write persistent data to disk.  
         * Copy the peer's data into our persistent data storage.
         */
        fbe_copy_memory(base_environment->my_persistent_data,
                        &peer_msg->persistent_data.header.data,  
                        base_environment->my_persistent_data_size);

        status = fbe_lifecycle_set_cond(&fbe_base_environment_lifecycle_const,
                                        (fbe_base_object_t*)base_environment,
                                        FBE_BASE_ENV_LIFECYCLE_COND_WRITE_PERSISTENT_DATA);
        if(status != FBE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t *) base_environment, 
                                    FBE_TRACE_LEVEL_ERROR,
                                    FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                    "%s failed to set WRITE_PERSISTENT_DATA condition.\n",
                                    __FUNCTION__);
        }

        break;
    case FBE_BASE_ENV_CMI_MSG_REQUEST_PEER_REVISION:
        base_environment->peerRevision = peer_rev_msg->revision;
        status = fbe_lifecycle_set_cond(&fbe_base_environment_lifecycle_const,
                                        (fbe_base_object_t*)base_environment,
                                        FBE_BASE_ENV_LIFECYCLE_COND_SEND_PEER_REVISION);
        if(status != FBE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t *) base_environment,
                                    FBE_TRACE_LEVEL_ERROR,
                                    FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                    "%s failed to set PEER_REVISION_RESPONSE condition.\n",
                                    __FUNCTION__);
        }

        break;
    case FBE_BASE_ENV_CMI_MSG_PEER_REVISION_RESPONSE:
        base_environment->peerRevision = peer_rev_msg->revision;
        // Set the response received, the condition will be cleared the next time it is scheduled
        base_environment->requestPeerRevision = PEER_REVISION_RESPONSE_RECEIVED;
        break;

    case FBE_BASE_ENV_CMI_MSG_FUP_PEER_ACTIVATE_START:
        // peer has started download.
        status = fbe_base_env_fup_process_peer_activate_start(base_environment,
                                                              pBaseFupCmiPkt->deviceType,
                                                              &pBaseFupCmiPkt->location);
        break;

    case FBE_BASE_ENV_CMI_MSG_FUP_PEER_UPGRADE_END:
        // peer has ended the upgrade.
        status = fbe_base_env_fup_process_peer_upgrade_end(base_environment,
                                                           pBaseFupCmiPkt->deviceType,
                                                           &pBaseFupCmiPkt->location);
        break;

    default:
        break;
    }
    return status;
}

/*********************************************************
 * end 
 * fbe_base_environment_cmi_process_received_message()
 ********************************************************/

/*!**************************************************************
 * fbe_base_environment_cmi_processPeerNotPresent()
 ****************************************************************
 * @brief
 *  This function gets called when recieving the CMI message with 
 *  FBE_CMI_EVENT_PEER_NOT_PRESENT.
 * 
 * @param object_handle - This is the object handle, or in our
 * case the base_environment.
 * @param cmi_message - The base environment cmi message
 *
 * @return fbe_status_t - 
 *              FBE_STATUS_OK - successful.
 *              others - failed.
 *
 * @author
 *  8-May-2012:  Created. Chengkai Hu
 *
 ****************************************************************/
fbe_status_t fbe_base_environment_cmi_processPeerNotPresent(fbe_base_environment_t *base_environment, 
                                                  fbe_base_environment_cmi_message_t *cmi_message)
{
    fbe_status_t status = FBE_STATUS_OK;

    fbe_base_object_trace((fbe_base_object_t *) base_environment, 
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "BaseEnv: CMI peer not present, opcode:%s(0x%X).\n",
                          fbe_base_env_decode_cmi_msg_opcode(cmi_message->opcode),
                          (unsigned int)cmi_message->opcode);

    switch(cmi_message->opcode)
    {
        //In case current SP is requesting read/write persistent data from peer
        case FBE_BASE_ENV_CMI_MSG_PERSISTENT_READ_REQUEST:
        case FBE_BASE_ENV_CMI_MSG_PERSISTENT_WRITE_REQUEST:
            status = fbe_base_environment_peer_gone(base_environment);
            break;

        case FBE_BASE_ENV_CMI_MSG_REQUEST_PEER_REVISION:
            base_environment->requestPeerRevision = PEER_REVISION_RESPONSE_RECEIVED;
            base_environment->peerRevision = 0;
            break;


        default:
            status = FBE_STATUS_OK;
            break;
    }
    return status;
}

/*********************************************************
 * end 
 * fbe_base_environment_cmi_processPeerNotPresent()
 ********************************************************/

/*!**************************************************************
 * fbe_base_environment_updateEnclFaultLed()
 ****************************************************************
 * @brief
 *  This function sends a command to update Enclosure Fault LED
 *  to the appropriate object (BaseBoard or EnclObject).
 * 
 * @param pLocation - location of the Enclosure
 * @param objectId - Object ID of the Enclosure
 * @param enclFaultLedOn - TRUE to turn On, FALSE to turn off
 * @param enclFaultLedReason - the reason the Fault LED is
 *                             being updated.
 *
 * @return fbe_status_t - The status of the call to
 *                        fbe_lifecycle_class_const_verify()
 *                        for the enclosure management's
 *                        constant lifecycle data.
 *
 * @author
 *  02-Feb-2011:  Created. Joe Perry
 *
 ****************************************************************/
fbe_status_t 
fbe_base_environment_updateEnclFaultLed(fbe_base_object_t * base_object,
                                        fbe_object_id_t objectId,
                                        fbe_bool_t enclFaultLedOn,
                                        fbe_encl_fault_led_reason_t enclFaultLedReason)
{
    fbe_status_t                                status;
    fbe_base_enclosure_led_status_control_t     ledStatusControlInfo;
    fbe_api_board_enclFaultLedInfo_t            enclFaultLedInfo;
    fbe_topology_object_type_t                  object_type;

    fbe_zero_memory(&ledStatusControlInfo, sizeof(fbe_base_enclosure_led_status_control_t));

    status = fbe_api_get_object_type(objectId, &object_type, FBE_PACKAGE_ID_PHYSICAL);
    if (status != FBE_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_INFO, 
                       "%s: cannot get Object Type from ID 0x%x, status %d\n", 
                       __FUNCTION__, objectId, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if (object_type == FBE_TOPOLOGY_OBJECT_TYPE_BOARD)
    {
        if (enclFaultLedOn)
        {
            enclFaultLedInfo.blink_rate = LED_BLINK_ON;
        }
        else
        {
            enclFaultLedInfo.blink_rate = LED_BLINK_OFF;
        }

        enclFaultLedInfo.enclFaultLedReason = enclFaultLedReason;
        status = fbe_api_board_set_enclFaultLED(objectId, &enclFaultLedInfo);
    }
    else if (object_type == FBE_TOPOLOGY_OBJECT_TYPE_ENCLOSURE)
    {
        ledStatusControlInfo.ledAction = FBE_ENCLOSURE_LED_CONTROL;
        if (enclFaultLedOn)
        {
            ledStatusControlInfo.ledInfo.enclFaultLed = FBE_ENCLOSURE_LED_BEHAVIOR_TURN_ON;
        }
        else
        {
            ledStatusControlInfo.ledInfo.enclFaultLed = FBE_ENCLOSURE_LED_BEHAVIOR_TURN_OFF;
        }
        ledStatusControlInfo.ledInfo.enclFaultReason = enclFaultLedReason;
        status = fbe_api_enclosure_send_set_led(objectId,
                                                &ledStatusControlInfo);
    }

    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace(base_object, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s, EnclFltLed failed, ObjId %d, enclFaultLedOn %d, status: 0x%X.\n",
                              __FUNCTION__, 
                              objectId,
                              enclFaultLedOn,
                              status);
        return status;
    }

    return status;

}
/*******************************************************************
 * end fbe_base_environment_updateEnclFaultLed()
 *******************************************************************/
/*!**************************************************************
 * fbe_base_environment_send_cacheStatus_update_to_peer()
 ****************************************************************
 * @brief
 *  This function sends a CMI message to update cache status. This
 *  cache will be updated as peerCacheStatus at peer side.
 * 
 * @param base_environment - 
 * @param CacheStatus - Local cache status
 * @param CacheStatusFlag - Flag to confirm if peerCacheStatus at this
 *                          side initialized or not.
 *
 * @return fbe_status_t 
 *
 * @author
 *  26-Nov-2012:  Created. Dipak Patel
 *
 ****************************************************************/
fbe_status_t fbe_base_environment_send_cacheStatus_update_to_peer(fbe_base_environment_t *base_environment, 
                                                                  fbe_common_cache_status_t CacheStatus,
                                                                  fbe_u8_t CacheStatusFlag)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_base_env_cacheStatus_cmi_packet_t *message;

    message = fbe_base_env_memory_ex_allocate(base_environment, sizeof(fbe_base_env_cacheStatus_cmi_packet_t));
    if(message == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t *) base_environment, 
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s failed to allocate memory for cmi message.\n",
                                __FUNCTION__);
    }
    else
    {
        fbe_zero_memory(message, sizeof(fbe_base_env_cacheStatus_cmi_packet_t));

        message->baseCmiMsg.opcode = FBE_BASE_ENV_CMI_MSG_CACHE_STATUS_UPDATE;
        message->CacheStatus = CacheStatus;
        message->isPeerCacheStatusInitilized = CacheStatusFlag;

        status = fbe_base_environment_send_cmi_message(base_environment,
                                                       sizeof(fbe_base_env_cacheStatus_cmi_packet_t), 
                                                       message);
        if(status != FBE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t *) base_environment, 
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s Failed sending cmi message with status 0x%x\n",
                                __FUNCTION__, status);
        }
        fbe_base_env_memory_ex_release(base_environment, message);
    }
    return status;
}

/*!**************************************************************
 * fbe_base_environment_combine_cacheStatus()
 ****************************************************************
 * @brief
 *  This function combines cache statu's of local and peer based
 *  on severity and provide combined status to caller.
 * 
 * @param base_environment - 
 * @param localCacheStatus - Cache status of local SP object.
 * @param PeerCacheStatus -  Cache status of peer SP object.
 *
 * @return combineCacheStatus - Combined cache status which will be returned
 *                              to DRAMCache.
 *
 * @author
 *  20-Nov-2012:  Created. Dipak Patel
 *
 ****************************************************************/
fbe_common_cache_status_t fbe_base_environment_combine_cacheStatus(fbe_base_environment_t *base_environment, 
                                                     fbe_common_cache_status_t localCacheStatus,
                                                     fbe_common_cache_status_t PeerCacheStatus,
                                                     fbe_class_id_t classId)
{
    fbe_common_cache_status_t combineCacheStatus = FBE_CACHE_STATUS_FAILED;

    if ((localCacheStatus == PeerCacheStatus) || 
        (PeerCacheStatus == FBE_CACHE_STATUS_UNINITIALIZE))
    {
        // if both sides cannot get status due to Environmental Interface faults, play it safe & mark faulted
        if ((localCacheStatus == FBE_CACHE_STATUS_FAILED_SHUTDOWN_ENV_FLT) &&
            (PeerCacheStatus == FBE_CACHE_STATUS_FAILED_SHUTDOWN_ENV_FLT))
        {
            combineCacheStatus = FBE_CACHE_STATUS_FAILED_SHUTDOWN;
        }
        else if ((localCacheStatus == FBE_CACHE_STATUS_FAILED_ENV_FLT) ||
                 (PeerCacheStatus == FBE_CACHE_STATUS_FAILED_ENV_FLT)) 
        {
            combineCacheStatus = FBE_CACHE_STATUS_FAILED;
        }
        else
        {
            combineCacheStatus = localCacheStatus;
        }
    }
    else
    {
        /* Now we have differnt cache status and also peercache status is intialized.
           So we will comapre them and will create combine status */
        if ((localCacheStatus == FBE_CACHE_STATUS_FAILED_SHUTDOWN) ||
            (PeerCacheStatus == FBE_CACHE_STATUS_FAILED_SHUTDOWN))
        {
            combineCacheStatus = FBE_CACHE_STATUS_FAILED_SHUTDOWN;
        }
        else if ((localCacheStatus == FBE_CACHE_STATUS_FAILED) ||
                 (PeerCacheStatus == FBE_CACHE_STATUS_FAILED)) 
        {
            combineCacheStatus = FBE_CACHE_STATUS_FAILED;
        }
        else if ((localCacheStatus == FBE_CACHE_STATUS_APPROACHING_OVER_TEMP) ||
                 (PeerCacheStatus == FBE_CACHE_STATUS_APPROACHING_OVER_TEMP)) 
        {
            combineCacheStatus = FBE_CACHE_STATUS_APPROACHING_OVER_TEMP;
        }
        else if ((localCacheStatus == FBE_CACHE_STATUS_DEGRADED) ||
                 (PeerCacheStatus == FBE_CACHE_STATUS_DEGRADED)) 
        {
            combineCacheStatus = FBE_CACHE_STATUS_DEGRADED;
        }
        else
        {
            combineCacheStatus = FBE_CACHE_STATUS_OK; 
        }
    }

    fbe_base_object_trace((fbe_base_object_t *) base_environment, 
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s Class:%d, Local:%s, Peer:%s, Combine:%s\n",
                          __FUNCTION__,
                          classId,
                          fbe_base_env_decode_cache_status(localCacheStatus),
                          fbe_base_env_decode_cache_status(PeerCacheStatus),
                          fbe_base_env_decode_cache_status(combineCacheStatus));

    return combineCacheStatus;
}

/*!**************************************************************
 * fbe_base_environment_compare_cacheStatus()
 ****************************************************************
 * @brief
 *  This function will compare local vs peer cache status on
 *  the basis of severity and if requires, then sends notification
 *  to cache at local side.
 * 
 * @param base_environment - 
 * @param localCacheStatus - Cache status of local SP object.
 * @param PeerCacheStatus -  Cache status of peer SP object.
 * @param classId - Class Id of the object.
 *
 * @return fbe_status_t - 
 *
 * @author
 *  20-Nov-2012:  Created. Dipak Patel
 *
 ****************************************************************/
fbe_status_t fbe_base_environment_compare_cacheStatus(fbe_base_environment_t *base_environment, 
                                                     fbe_common_cache_status_t localCacheStatus,
                                                     fbe_common_cache_status_t PeerCacheStatus,
                                                     fbe_class_id_t classId)
{
//    fbe_status_t status = FBE_STATUS_OK;
    fbe_device_physical_location_t  location;

    fbe_base_object_trace((fbe_base_object_t *)base_environment, 
                           FBE_TRACE_LEVEL_INFO,
                           FBE_TRACE_MESSAGE_ID_INFO,
                           "%s entry.\n",
                           __FUNCTION__); 


    if ((localCacheStatus == FBE_CACHE_STATUS_UNINITIALIZE) ||
        (PeerCacheStatus == FBE_CACHE_STATUS_UNINITIALIZE))
    {
        /* This should not happen.
           Trace it and return */
        fbe_base_object_trace((fbe_base_object_t *) base_environment, 
                                FBE_TRACE_LEVEL_INFO,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s. Invalid cache status Local:%d, Peer:%d\n",
                                __FUNCTION__, localCacheStatus, PeerCacheStatus);

    }
    else
    {
        if (PeerCacheStatus != localCacheStatus)
        {
           /* location not relevant, so making it zero for all the notifications
                 to cache */
           fbe_zero_memory(&location, sizeof(fbe_device_physical_location_t));

           /* Send notification to read cache status*/ 
           fbe_base_environment_send_data_change_notification(base_environment, 
                                                               classId, 
                                                               FBE_DEVICE_TYPE_SP_ENVIRON_STATE, 
                                                               FBE_DEVICE_DATA_TYPE_GENERIC_INFO,
                                                               &location);

           fbe_base_object_trace((fbe_base_object_t *)base_environment, 
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s, FBE_DEVICE_TYPE_SP_ENVIRON_STATE sent. ClassID:%d\n",
                          __FUNCTION__, classId);
        }
    }
	
	return FBE_STATUS_OK;
	
	}

/*!**************************************************************
 * fbe_base_environment_notify_peer_sp_alive_cond_function()
 ****************************************************************
 * @brief
 *  This function sends a message to the peer notifying the object
 *  is come up and ready for receving cmi message.
 *
 * @param object_handle - This is the object handle, or in our
 * case the base_environment.
 * @param packet_p - The packet arriving from the monitor
 * scheduler.
 *
 * @return fbe_status_t - The status of the call to
 *                        fbe_lifecycle_class_const_verify()
 *                        for the enclosure management's
 *                        constant lifecycle data.
 *
 * @author
 *  29-May-2013:  Created. Dongz
 *
 ****************************************************************/
fbe_lifecycle_status_t
fbe_base_environment_notify_peer_sp_alive_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet)
{
    fbe_status_t status;
    fbe_base_environment_t *base_environment = (fbe_base_environment_t *)base_object;
    fbe_base_environment_notify_peer_ready_cmi_message_t message;

    fbe_base_object_trace(base_object,
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s entry\n",
                          __FUNCTION__);

    /*
     * Send a message to the peer containing our revision
     */


    fbe_zero_memory(&message, sizeof(fbe_base_environment_notify_peer_ready_cmi_message_t));

    message.msg.opcode = FBE_BASE_ENV_CMI_MSG_PEER_SP_ALIVE;

    status = fbe_base_environment_send_cmi_message(base_environment,
                                                   sizeof(fbe_base_environment_notify_peer_ready_cmi_message_t),
                                                   &message);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *) base_environment,
                            FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                            "%s Failed sending cmi message with status 0x%x\n",
                            __FUNCTION__, status);
    }
    else
    {
        status = fbe_lifecycle_clear_current_cond(base_object);
        if (status != FBE_STATUS_OK) {
            fbe_base_object_trace(base_object,
                                    FBE_TRACE_LEVEL_ERROR,
                                    FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                    "%s can't clear current condition, status: 0x%X\n",
                                    __FUNCTION__, status);
        }
    }
    return FBE_LIFECYCLE_STATUS_DONE;
}


/*!**************************************************************
 * fbe_base_environment_isPsSupportedByEncl()
 ****************************************************************
 * @brief
 *  This function will determine if the PS is supported by this
 *  specific enclosure type, on this specific array.
 *
 * @param base_object - This is the base object for the ESP object.
 * @param pLocation - pointer to location info
 * @param psFfid - Family FRU ID of the PS
 * @param dcTelcoArray - boolean for whether this is a DC array
 * @param psSupported - (OUTPUT) boolean for whether PS is supported
 * @param eirSupported - (OUTPUT) boolean for whether EIR is supported
 *
 * @return fbe_status_t - The status of the function's processing.
 *
 * @author
 *  12-Dec-2014:  Created. Joe Perry
 *
 ****************************************************************/
fbe_status_t
fbe_base_environment_isPsSupportedByEncl(fbe_base_object_t * base_object,
                                         fbe_device_physical_location_t *pLocation,
                                         HW_MODULE_TYPE psFfid,
                                         fbe_bool_t dcTelcoArray,
                                         fbe_bool_t *psSupported,
                                         fbe_bool_t *eirSupported)
{
    fbe_object_id_t                         objectId, enclObjectId;
    fbe_board_mgmt_platform_info_t          platform_info;
    fbe_enclosure_types_t                   enclType;
    fbe_status_t                            status;

    // initialize return status to FALSE
    *psSupported = FALSE;
    *eirSupported = FALSE;

    // get Platform Type
    status = fbe_api_get_board_object_id(&objectId);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace(base_object,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s Error getting board object id, status: 0x%X.\n",
                              __FUNCTION__, status);
        return status;
    }

    fbe_zero_memory(&platform_info, sizeof(fbe_board_mgmt_platform_info_t));
    status = fbe_api_board_get_platform_info(objectId, &platform_info);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace(base_object,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s Error getting platform info, status: 0x%X.\n",
                              __FUNCTION__, status);
        return status;
    }

    // get Enclosure Type
    status = fbe_api_get_enclosure_object_id_by_location(pLocation->bus,
                                                         pLocation->enclosure,
                                                         &enclObjectId);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace(base_object, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s Error in getting enclObjectId for %d_%d, status: 0x%X\n",
                              __FUNCTION__, 
                              pLocation->bus,
                              pLocation->enclosure,
                              status);
        return status;
    }

    status = fbe_api_enclosure_get_encl_type(enclObjectId, &enclType);
    if(status != FBE_STATUS_OK)
    {
        // Allow Megatron use - internal use only
        if ((platform_info.hw_platform_info.platformType == SPID_PROMETHEUS_M1_HW_TYPE) ||
            (platform_info.hw_platform_info.platformType == SPID_PROMETHEUS_S1_HW_TYPE))
        {
            switch (psFfid)
            {
            case AC_ACBEL_OCTANE:
                *psSupported = TRUE;
                *eirSupported = TRUE;
                break;
            default:
                fbe_base_object_trace(base_object,
                                      FBE_TRACE_LEVEL_WARNING,
                                      FBE_TRACE_MESSAGE_ID_INFO,
                                      "%s, %d_%d_%d Unsupported, ffid 0x%x, platform %d, enclType %d\n",
                                      __FUNCTION__, 
                                      pLocation->bus, pLocation->enclosure, pLocation->slot,
                                      psFfid,
                                      platform_info.hw_platform_info.platformType,
                                      enclType);
                break;
            }
            return FBE_STATUS_OK;
        }
        else
        {
            fbe_base_object_trace(base_object, 
                                  FBE_TRACE_LEVEL_WARNING,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s Error in getting EnclType for object id %d, status: 0x%X\n",
                                  __FUNCTION__, objectId, status);
            return status;
        }
    }

    // Determine if PS supported based on platform & enclosure
    switch (enclType)
    {

    /*
     * 6G Enclosures
     */

    // Beachcomber DPE's
    case FBE_ENCL_SAS_STEELJAW:
    case FBE_ENCL_SAS_RAMHORN:
        switch (psFfid)
        {
        case AC_ACBEL_OCTANE:
            *psSupported = TRUE;
            *eirSupported = TRUE;
            break;
        case DC_ACBEL_BLASTOFF_GEN_1_PLUS:
        case DC_ACBEL_BLASTOFF:
            *eirSupported = TRUE;
            if (dcTelcoArray)
            {
                *psSupported = TRUE;
            }
        default:
            break;
        }
        break; 
    // DAE's
    case FBE_ENCL_SAS_PINECONE:
        switch (psFfid)
        {
        case AC_DC_JUNIPER:
        case DC_JUNIPER_700:
        case AC_ACBEL_JUNIPER:
            *psSupported = TRUE;
            *eirSupported = TRUE;
            break;
        default:
            break;
        }
        break;
    case FBE_ENCL_SAS_DERRINGER:
        switch (psFfid)
        {
        case AC_ACBEL_DERRINGER:
        case ACBEL_DERRINGER_BLOWER_MODULE:
        case AC_ACBEL_DERRINGER_071:
        case HVDC_ACBEL_DERRINGER:
        case DERRINGER_EMERSON:
            *psSupported = TRUE;
            *eirSupported = TRUE;
            break;
        case DC_ACBEL_DERRINGER:        // Does not support Energy Star statistics
            *psSupported = TRUE;
            break;
        default:
            break;
        }
        break;
    case FBE_ENCL_SAS_VIPER:
        switch (psFfid)
        {
        case AC_ACBEL_DAE_GEN3:
        case AC_EMERSON_DAE_GEN3:
        case ACBEL_3GEN_VE_DAE:
        case EMERSON_3GEN_VE_DAE:
            *psSupported = TRUE;
            *eirSupported = TRUE;
            break;
        default:
            break;
        }
        break;
    case FBE_ENCL_SAS_VOYAGER_ICM:
        switch (psFfid)
        {
        case AC_ACBEL_VOYAGER:
        case VOYAGER_EMERSON:
            *psSupported = TRUE;
            *eirSupported = TRUE;
            break;
        default:
            break;
        }
        break;
    case FBE_ENCL_SAS_VIKING_IOSXP:
        switch (psFfid)
        {
        case AC_ACBEL_DERRINGER:
            *psSupported = TRUE;
            *eirSupported = TRUE;
            break;
        default:
            break;
        }
        break;

        /*
         * 12G Enclosures
         */

    // Oberon DPE's
    case FBE_ENCL_SAS_RHEA:
    case FBE_ENCL_SAS_MIRANDA:
        switch (psFfid)
        {
        case AC_FLEXPOWER_OPTIMUS_GEN_1_PLUS:
        case AC_ACBEL_OPTIMUS_GEN_1_PLUS:
        case AC_ACBEL_OPTIMUS_GEN_1_PLUS_611:
        case AC_OPTIMUS:                        // JAP - do not allow on shipping Oberons
        case AC_FLEXPOWER_OPTIMUS:              // JAP - do not allow on shipping Oberons
        case AC_ACBEL_BLASTOFF:                 // JAP - remove Blastoff support when Optimus available
        case AC_FLEXPOWER_BLASTOFF:             // JAP - remove Blastoff support when Optimus available
            *psSupported = TRUE;
            *eirSupported = TRUE;
            break;
        case DC_ACBEL_BLASTOFF_GEN_1_PLUS:
        case DC_ACBEL_BLASTOFF:                 // JAP - remove older Blastoff support when newer ones available
            *eirSupported = TRUE;
            if (dcTelcoArray)
            {
                *psSupported = TRUE;
            }
            break;
        default:
            break;
        }
        break; 
    // Hyperion DPE
    case FBE_ENCL_SAS_CALYPSO:
        // different PS's based on type of platform
        switch (platform_info.hw_platform_info.platformType)
        {
        case SPID_HYPERION_1_HW_TYPE:
        case SPID_HYPERION_2_HW_TYPE:
            switch (psFfid)
            {
            case AC_FLEXPOWER_OPTIMUS_GEN_1_PLUS:
            case AC_ACBEL_OPTIMUS_GEN_1_PLUS:
            case AC_OPTIMUS:
            case AC_FLEXPOWER_OPTIMUS:
            case AC_ACBEL_BLASTOFF:                 // JAP - remove Blastoff support when Optimus available
            case AC_FLEXPOWER_BLASTOFF:             // JAP - remove Blastoff support when Optimus available
                *psSupported = TRUE;
                *eirSupported = TRUE;
                break;
            default:
                break;
            }
            break;
        case SPID_HYPERION_3_HW_TYPE:
            switch (psFfid)
            {
            case AC_ACBEL_RAMJET:
            case AC_EMERSON_RAMJET:
            case AC_FLEXPOWER_RAMJET:
            case AC_ACBEL_RAMJET_197:
            case AC_FLEXPOWER_OPTIMUS_GEN_1_PLUS:       // JAP - allow Optimus on Hyperion3 until Ramjets available
            case AC_ACBEL_OPTIMUS_GEN_1_PLUS:           // JAP - allow Optimus on Hyperion3 until Ramjets available
            case AC_OPTIMUS:                            // JAP - allow Optimus on Hyperion3 until Ramjets available
            case AC_FLEXPOWER_OPTIMUS:                  // JAP - allow Optimus on Hyperion3 until Ramjets available
            case AC_ACBEL_BLASTOFF:                     // JAP - remove Blastoff support when Optimus available
            case AC_FLEXPOWER_BLASTOFF:                 // JAP - remove Blastoff support when Optimus available
                *psSupported = TRUE;
                *eirSupported = TRUE;
                break;
            default:
                break;
            }
            break; 
        }
        break; 
    // DAE's
    case FBE_ENCL_SAS_ANCHO:
        switch (psFfid)
        {
        case AC_ACBEL_DAE_GEN3:
        case AC_EMERSON_DAE_GEN3:
        case ACBEL_3GEN_VE_DAE:
        case EMERSON_3GEN_VE_DAE:
            *psSupported = TRUE;
            *eirSupported = TRUE;
            break;
        case DC_ACBEL_STILETTO:             // Does not support Energy Star statistics
            *psSupported = TRUE;
            break;
        default:
            break;
        }
        break;
    case FBE_ENCL_SAS_TABASCO:
        switch (psFfid)
        {
        case TABASCO_PS_COOLING_MODULE:
        case DC_TABASCO_PS_COOLING_MODULE:
            *psSupported = TRUE;
            *eirSupported = TRUE;
            break;
        default:
            break;
        }
        break;
    case FBE_ENCL_SAS_CAYENNE_IOSXP:
        switch (psFfid)
        {
        case AC_JUNO_2:
            *psSupported = TRUE;
            *eirSupported = TRUE;
            break;
        default:
            break;
        }
        break;
    case FBE_ENCL_SAS_NAGA_IOSXP:
        switch (psFfid)
        {
        case AC_FLEXPOWER_OPTIMUS_GEN_1_PLUS:
        case AC_ACBEL_OPTIMUS_GEN_1_PLUS:
        case AC_ACBEL_OPTIMUS_GEN_1_PLUS_611:
        case AC_OPTIMUS:                        // JAP - do not allow on shipping Oberons
        case AC_FLEXPOWER_OPTIMUS:              // JAP - do not allow on shipping Oberons
        case DAE_AC_1080:                       // JAP - support Laserbeak PS until purged from lab
            *psSupported = TRUE;
            *eirSupported = TRUE;
            break;
        default:
            break;
        }
        break;

    default:
        break; 
    }

    if (!(*psSupported))
    {
        fbe_base_object_trace(base_object,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s, %d_%d_%d Unsupported, ffid 0x%x, platform %d, enclType %d\n",
                              __FUNCTION__, 
                              pLocation->bus, pLocation->enclosure, pLocation->slot,
                              psFfid,
                              platform_info.hw_platform_info.platformType,
                              enclType);
    }
    return FBE_STATUS_OK;

}   // end of fbe_base_environment_isPsSupportedByEncl



/*!**************************************************************
 * fbe_base_environment_isSpInBootflash()
 ****************************************************************
 * @brief
 *  This function determines if the SP is in Bootflash.
 *
 * @param base_object - This is the base object for the ESP object.
 *
 * @return fbe_bool_t - TRUE if SP in Bootflash
 *
 * @author
 *  01-Jul-2015:  Created. Joe Perry
 *
 ****************************************************************/
fbe_bool_t
fbe_base_environment_isSpInBootflash(fbe_base_object_t * base_object)
{
    fbe_u32_t bootFromFlash = FALSE;   
    fbe_u8_t defaultBootFromFlash =0;
    fbe_status_t status;

    status = fbe_registry_read(NULL,
                               BootFromFlashRegPath,
                               BootFromFlashKey, 
                               &bootFromFlash,
                               sizeof(bootFromFlash),
                               FBE_REGISTRY_VALUE_DWORD,
                               &defaultBootFromFlash,
                               0,
                               FALSE);
    if(status != FBE_STATUS_OK)
    {
        // if cannot read Registry, assume BootFlash
        fbe_base_object_trace(base_object,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s, error reading Registry BootFromFlash, status %d\n",
                              __FUNCTION__, status);
        return TRUE;
    }

    fbe_base_object_trace(base_object,
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s, BootFromFlash %d\n",
                          __FUNCTION__, bootFromFlash);

    if (bootFromFlash == 0)
    {
        return FALSE;
    }
    else
    {
        return TRUE;
    }

}   // end of fbe_base_environment_isSpInBootflash

