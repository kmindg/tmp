/***************************************************************************
 * Copyright (C) EMC Corporation 2007-2010 
 * All rights reserved. Licensed material -- property of EMC 
 * Corporation 
 ***************************************************************************/

#ifndef BASE_ENVIRONMENT_PRIVATE_H
#define BASE_ENVIRONMENT_PRIVATE_H

/*!**************************************************************************
 * @file fbe_base_environment_private.h
 ***************************************************************************
 *
 * @brief
 *  This file contains private defines for the base environment
 *  object.
 * 
 * @ingroup base_environment_class_files
 * 
 * @revision
 *   02/25/2010:  Created. Ashok Tamilarasan
 *
 ***************************************************************************/
#include "fbe_cmi.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_devices.h"
#include "fbe/fbe_esp_base_environment.h"
#include "fbe/fbe_enclosure.h"
#include "fbe_base_environment_fup_private.h"
#include "fbe_base_environment_resume_prom_private.h"
#include "fbe/fbe_persist_interface.h"
#include "fbe/fbe_limits.h"

#define FBE_ESP_MAX_ENCL_COUNT      (FBE_PHYSICAL_BUS_COUNT * FBE_ENCLOSURES_PER_BUS)
#define FBE_BASE_ENV_CMI_MSG_SHIFT  16
#define FBE_ESP_MAX_REBOOT_RETRIES  3
#define FBE_ESP_MAX_PS_MODE_OP_RETRIES 3
#define FBE_BASE_ENVIRONMENT_MAX_PERSISTENT_DATA_READ_RETRIES 150
#define ESP_BUGCHECK_ON_REBOOT 0
#define FBE_BASE_ENVIRONMENT_IMAGE_FILE_NAME_UID_STRING_LEN 8
#define FBE_BASE_ENV_UPGRADE_DELAY_AT_BOOT_UP_ON_SPB   3600    /* 60 minutes delay on hardware for SPB */
/* Due to the longer firmware activation time, we have seen the case that SPA sent the peer permission request but SPB was still doing the activation.
 * So the upgrade on SPA side could not proceed and had to wait for the peer permission. 
 * Increase the delay difference to 20 minutes to reduce the chance of not getting the peer permisison right away.
 */
#define FBE_BASE_ENV_UPGRADE_DELAY_AT_BOOT_UP_ON_SPA   4800    /* 80 minutes delay on hardware for SPA */
#define FBE_BASE_ENV_SOFTWARE_REVISION 0001 /* Revision for ESP software, bump this when changing CMI messages */

/* Lifecycle definitions
 * Define the base environment lifecycle's instance.
 */
extern fbe_lifecycle_const_t FBE_LIFECYCLE_CONST_DATA(base_environment);

/* These are the lifecycle condition ids for Base Environment
   object.*/

/*! @enum fbe_base_environment_lifecycle_cond_id_t  
 *  
 *  @brief These are the lifecycle conditions for the base
 *  environment object.
                                                               */
typedef enum fbe_base_environment_lifecycle_cond_id_e 
{
    /*! Register call back for notification so that FBE API can call
     *  back when there are events. Each leaf class NEEDS to
     *  implement their own condition hander function
     */
    FBE_BASE_ENV_LIFECYCLE_COND_REGISTER_STATE_CHANGE_CALLBACK = FBE_LIFECYCLE_COND_FIRST_ID(FBE_CLASS_ID_BASE_ENVIRONMENT),

    /*! Condition to register the CMI Callback so that CMI Service 
     *  can callback when there are cmi messages. Each leaf class NEEDS to
     *  implement their own condition hander function
     */
    FBE_BASE_ENV_LIFECYCLE_COND_REGISTER_CMI_CALLBACK,

    /*! Handler for Event Notification. This is primarily used to 
     *  break the context and call the function that leaf class had
     *  registered before.
     */
    FBE_BASE_ENV_LIFECYCLE_COND_HANDLE_EVENT_NOTIFICATION,

    /*! Handler for CMI message notification.This is primarily used to 
     *  break the context and call the function that leaf class had
     *  registered before. 
     */
    
    FBE_BASE_ENV_LIFECYCLE_COND_HANDLE_CMI_NOTIFICATION,
    FBE_BASE_ENV_LIFECYCLE_COND_SEND_PEER_REVISION,
    FBE_BASE_ENV_LIFECYCLE_COND_CHECK_PEER_REVISION,
    FBE_BASE_ENV_LIFECYCLE_COND_NOTIFY_PEER_PS_ALIVE,

    FBE_BASE_ENV_LIFECYCLE_COND_INITIALIZE_PERSISTENT_STORAGE, 
    FBE_BASE_ENV_LIFECYCLE_COND_SEND_PEER_PERSISTENT_DATA,
    FBE_BASE_ENV_LIFECYCLE_COND_SEND_PEER_PERSIST_WRITE_COMPLETE,

    FBE_BASE_ENV_LIFECYCLE_COND_WRITE_PERSISTENT_DATA,
    FBE_BASE_ENV_LIFECYCLE_COND_READ_PERSISTENT_DATA,
    FBE_BASE_ENV_LIFECYCLE_COND_READ_PERSISTENT_DATA_COMPLETE,
    FBE_BASE_ENV_LIFECYCLE_COND_WAIT_PEER_PERSISTENT_DATA,
    
    /*! Condition to register the firmare upgrade callback functions. */
    FBE_BASE_ENV_LIFECYCLE_COND_FUP_REGISTER_CALLBACK,
    /*! Condition for the firmare upgrade state machine. */
    FBE_BASE_ENV_LIFECYCLE_COND_FUP_TERMINATE_UPGRADE,
    FBE_BASE_ENV_LIFECYCLE_COND_FUP_ABORT_UPGRADE,
    FBE_BASE_ENV_LIFECYCLE_COND_FUP_WAIT_BEFORE_UPGRADE,
    FBE_BASE_ENV_LIFECYCLE_COND_FUP_WAIT_FOR_INTER_DEVICE_DELAY,
    FBE_BASE_ENV_LIFECYCLE_COND_FUP_READ_IMAGE_HEADER,
    FBE_BASE_ENV_LIFECYCLE_COND_FUP_CHECK_REV,
    FBE_BASE_ENV_LIFECYCLE_COND_FUP_READ_ENTIRE_IMAGE,
    FBE_BASE_ENV_LIFECYCLE_COND_FUP_DOWNLOAD_IMAGE,
    FBE_BASE_ENV_LIFECYCLE_COND_FUP_GET_DOWNLOAD_STATUS,
    FBE_BASE_ENV_LIFECYCLE_COND_FUP_GET_PEER_PERMISSION,
    FBE_BASE_ENV_LIFECYCLE_COND_FUP_CHECK_ENV_STATUS,
    FBE_BASE_ENV_LIFECYCLE_COND_FUP_ACTIVATE_IMAGE,
    FBE_BASE_ENV_LIFECYCLE_COND_FUP_GET_ACTIVATE_STATUS,
    FBE_BASE_ENV_LIFECYCLE_COND_FUP_CHECK_RESULT,
    FBE_BASE_ENV_LIFECYCLE_COND_FUP_REFRESH_DEVICE_STATUS,
    FBE_BASE_ENV_LIFECYCLE_COND_FUP_END_UPGRADE,
    FBE_BASE_ENV_LIFECYCLE_COND_FUP_RELEASE_IMAGE,

    /*! Condition to register the Resume Prom callback functions. */
    FBE_BASE_ENV_LIFECYCLE_COND_RESUME_PROM_REGISTER_CALLBACK,
    FBE_BASE_ENV_LIFECYCLE_COND_UPDATE_RESUME_PROM,
    FBE_BASE_ENV_LIFECYCLE_COND_READ_RESUME_PROM,
    FBE_BASE_ENV_LIFECYCLE_COND_CHECK_RESUME_PROM_OUTSTANDING_REQUEST,

    FBE_BASE_ENVIRONMENT_LIFECYCLE_COND_LAST /* must be last */
} 
fbe_base_environment_lifecycle_cond_id_t;


/*! @enum fbe_base_environment_failure_type_t  
 *  
 *  @brief These are types of failures that come from Base Env.
 */
typedef enum fbe_base_environment_failure_type_e 
{
    FBE_BASE_ENV_RP_READ_FAILURE,
    FBE_BASE_ENV_FUP_FAILURE,
} fbe_base_environment_failure_type_t;

/*! @struct fbe_base_environment_event_element_t
 * @brief 
 *   This structure represents all the information needed to
 *   save the leaf class information with regards to event
 *   notification. We save the information here so that we can
 *   break the context when the callback happens
 */
typedef struct fbe_base_environment_event_element_s {

    /*! use as registration ID to register with notification of FBE API
     */
    fbe_notification_registration_id_t      notification_registration_id;

    /*! Leaf class callback function that needs to be called when an 
     *  event happens 
     */
    fbe_api_notification_callback_function_t event_call_back;

    /*! Queue to hold all the event that the leaf class had 
     *  registered for 
     */
    fbe_queue_head_t event_queue;

    /*! Lock for the event queue
     */
    fbe_spinlock_t event_queue_lock;

    /*! Leaf class context that was passed in during registration.
     */
    void *context;

    /*! The lifecycle mask that leaf class wants to get notified 
     * about 
     */
    fbe_notification_type_t life_cycle_mask;

    /*! The device mask is the list of devices that leaf class wants
     * to get notified about
     */
    fbe_u64_t device_mask;

}fbe_base_environment_event_element_t;


/*! @struct fbe_base_environment_cmi_message_info_s
 * @brief 
 *   This structure represents all the information needed to
 *   save the CMI information that was received. We save the
 *   information here so that we can break the context when the
 *   callback happens
 */
typedef struct fbe_base_environment_cmi_message_info_s {
    /*! This must be first.  
     */
    fbe_queue_element_t         queue_element; /* MUST be first */

    /*! Event type for this message
     */
    fbe_cmi_event_t event;

    /*! Buffer to store the user message
     */
    fbe_cmi_message_t user_message;

    /*! Caller context
     */
    fbe_cmi_event_callback_context_t context;
}fbe_base_environment_cmi_message_info_t;

typedef fbe_status_t (* fbe_base_environment_cmi_callback_function_t) (fbe_base_object_t *base_environment, fbe_base_environment_cmi_message_info_t *cmi_message);

/*! @struct fbe_base_environment_cmi_element_s
 * @brief 
 *  This structure represents all the information needed to
 *   save the leaf class information with regards to CMI message
 *   notification. We save the information here so that we
 *   can break the context when the callback happens
 */
typedef struct fbe_base_environment_cmi_element_s {

    /*! Client ID of the leaf class
     */
    fbe_cmi_client_id_t client_id;

    /*! Callback function of the leaf class that needs to be called 
     *  when a message is received 
     */
    fbe_base_environment_cmi_callback_function_t cmi_callback;

    /*! Queue to hold all the CMI messages
     */
    fbe_queue_head_t cmi_callback_queue;

    /*! Lock for the CMI message queue
     */
    fbe_spinlock_t cmi_queue_lock;

}fbe_base_environment_cmi_element_t;

/*!****************************************************************************
 *    
 * @enum fbe_base_environment_cmi_message_opcode_t
 *  
 * @brief 
 *   Enumeration of CMI messages that are common enough to be handled at
 *   base level.
 ******************************************************************************/
typedef enum fbe_base_environment_cmi_message_opcode_e
{
    FBE_BASE_ENV_CMI_MSG_INVALID,
    FBE_BASE_ENV_CMI_MSG_PERSISTENT_WRITE_REQUEST = (1 << FBE_BASE_ENV_CMI_MSG_SHIFT),
    FBE_BASE_ENV_CMI_MSG_PERSISTENT_WRITE_COMPLETE,
    FBE_BASE_ENV_CMI_MSG_PERSISTENT_READ_REQUEST,
    FBE_BASE_ENV_CMI_MSG_PERSISTENT_READ_COMPLETE,
    FBE_BASE_ENV_CMI_MSG_FUP_PEER_PERMISSION_REQUEST,    // request permission for fup
    FBE_BASE_ENV_CMI_MSG_FUP_PEER_PERMISSION_GRANT,      // grant permission to fup
    FBE_BASE_ENV_CMI_MSG_FUP_PEER_PERMISSION_DENY,       // permission denied by peer
    FBE_BASE_ENV_CMI_MSG_USER_MODIFY_WWN_SEED_FLAG_CHANGE,       // user modified wwn seed info
    FBE_BASE_ENV_CMI_MSG_USER_MODIFIED_SYS_ID_FLAG,              // user modified system id flag
    FBE_BASE_ENV_CMI_MSG_PERSISTENT_SYS_ID_CHANGE,       // persistent storage sys id change
    FBE_BASE_ENV_CMI_MSG_ENCL_CABLING_REQUEST,
    FBE_BASE_ENV_CMI_MSG_ENCL_CABLING_ACK,
    FBE_BASE_ENV_CMI_MSG_ENCL_CABLING_NACK,
    FBE_BASE_ENV_CMI_MSG_CACHE_STATUS_UPDATE,
    FBE_BASE_ENV_CMI_MSG_REQ_SPS_INFO,
    FBE_BASE_ENV_CMI_MSG_SEND_SPS_INFO,
    FBE_BASE_ENV_CMI_MSG_TEST_REQ_PERMISSION,
    FBE_BASE_ENV_CMI_MSG_TEST_REQ_ACK,
    FBE_BASE_ENV_CMI_MSG_TEST_REQ_NACK,
    FBE_BASE_ENV_CMI_MSG_TEST_COMPLETED,
    FBE_BASE_ENV_CMI_MSG_BOARD_INFO_REQUEST,
    FBE_BASE_ENV_CMI_MSG_BOARD_INFO_RESPONSE,
    FBE_BASE_ENV_CMI_MSG_PEER_BBU_TEST_STATUS,          // BBU Self Test status only accessible on local SP
    FBE_BASE_ENV_CMI_MSG_REQUEST_PEER_REVISION,
    FBE_BASE_ENV_CMI_MSG_PEER_REVISION_RESPONSE,
    FBE_BASE_ENV_CMI_MSG_PEER_SP_ALIVE,
    FBE_BASE_ENV_CMI_MSG_PEER_DISK_BATTERY_BACKED_SET,
    FBE_BASE_ENV_CMI_MSG_REQ_PEER_DISK_BATTERY_BACKED_SET,
    FBE_BASE_ENV_CMI_MSG_FUP_PEER_ACTIVATE_START,
    FBE_BASE_ENV_CMI_MSG_FUP_PEER_UPGRADE_END,
}fbe_base_environment_cmi_message_opcode_t;

typedef enum fbe_persistent_id_e
{
    FBE_PERSISTENT_ID_ENCL_MGMT = 92,
    FBE_PERSISTENT_ID_SPS_MGMT = 93,
    FBE_PERSISTENT_ID_MODULE_MGMT = 95,
    FBE_PERSISTENT_ID_PS_MGMT = 96
}fbe_persistent_id_t;

/*
 * This is the header associated with all persistent data.  This header 
 * is used to identify which leaf class object the persistent data belongs to. 
 * Other identifying information is handled by the leaf class. 
 */
typedef struct fbe_base_environment_persistent_data_header_s {
    fbe_persistent_id_t persistent_id;
    fbe_u8_t data;
}fbe_base_environment_persistent_data_header_t;


#pragma pack()
typedef struct fbe_base_environment_persistent_data{
    fbe_persist_user_header_t                                   persist_header;/*can be used with any data that is persisted*/
    fbe_base_environment_persistent_data_header_t               header;
    //fbe_u8_t                                                  data[FBE_BASE_ENVIRONMENT_PERSISTENT_DATA_SIZE];/*pad to use a full 2K buffer, but w/o the persist header*/
    
}fbe_base_environment_persistent_data_t;
#pragma pack()
typedef struct fbe_base_environment_persistent_read_data{
    fbe_persist_user_header_t                                   persist_header;/*can be used with any data that is persisted*/
    fbe_persist_user_header_t                                   persist_header2;/*can be used with any data that is persisted*/
    fbe_base_environment_persistent_data_header_t               header;
    //fbe_u8_t                                                  data[FBE_BASE_ENVIRONMENT_PERSISTENT_DATA_SIZE];/*pad to use a full 2K buffer, but w/o the persist header*/
    
}fbe_base_environment_persistent_read_data_t;

#pragma pack()
typedef struct fbe_base_environment_persistent_data_entry_s{
    fbe_persist_user_header_t   persist_header;/*can be used with any data that is persisted*/
    fbe_persistent_id_t         persistent_id;
    fbe_u8_t                    data_block[(FBE_PERSIST_DATA_BYTES_PER_BLOCK * FBE_PERSIST_BLOCKS_PER_ENTRY) - sizeof(fbe_class_id_t)];
}fbe_base_environment_persistent_data_entry_t;

/*! @struct fbe_base_environment_persistent_data_cmi_message_s
 * @brief 
 *   This structure is the base environment class'
 *   message structure for sending persistent data
 *   to the peer SP.
 */
typedef struct fbe_base_environment_persistent_data_cmi_message_s
{
    fbe_base_environment_cmi_message_t msg;
    fbe_base_environment_persistent_data_t persistent_data;
}fbe_base_environment_persistent_data_cmi_message_t;

typedef enum fbe_base_environment_persistent_data_wait_peer_state_e
{
    PERSISTENT_DATA_WAIT_NONE = 0,
    PERSISTENT_DATA_WAIT_READ = 1,
    PERSISTENT_DATA_WAIT_WRITE = 2
}fbe_base_environment_persistent_data_wait_peer_state_t;



typedef struct fbe_base_env_cacheStatus_cmi_packet_s 
{
    fbe_base_environment_cmi_message_t   baseCmiMsg;    //fbe_base_environment_cmi_message_opcode_t msgType;
    fbe_common_cache_status_t            CacheStatus;
    fbe_u8_t                             isPeerCacheStatusInitilized;
} fbe_base_env_cacheStatus_cmi_packet_t;

typedef struct fbe_base_environment_peer_revision_cmi_message_s
{
    fbe_base_environment_cmi_message_t msg;
    fbe_u32_t revision;
}fbe_base_environment_peer_revision_cmi_message_t;

typedef enum fbe_base_environment_request_peer_revision_state_s
{
    NO_PEER_REVISION_REQUEST = 0,
    WAIT_PEER_REVISION_RESPONSE = 1,
    PEER_REVISION_RESPONSE_RECEIVED = 3
}fbe_base_environment_request_peer_revision_state_t;

typedef struct fbe_base_environment_notify_peer_ready_cmi_message_s
{
    fbe_base_environment_cmi_message_t msg;
}fbe_base_environment_notify_peer_ready_cmi_message_t;

/*!****************************************************************************
 *    
 * @struct fbe_base_environment_s
 *  
 * @brief 
 *   This is the definition of the encl mgmt object. This object
 *   deals with handling enclosure related functions
 ******************************************************************************/
typedef struct fbe_base_environment_s {
    /*! This must be first.  This is the base object we inherit from.
     */
    fbe_base_object_t base_object;

    /*! Event element to store leaf class event notification 
     *  information 
     */
    fbe_base_environment_event_element_t event_element;

    /*! CMI info
     */
    fbe_base_environment_cmi_element_t cmi_element;

    /*! Firmware upgrade info 
     */

    fbe_base_env_fup_element_t * fup_element_ptr;
    
    fbe_base_env_resume_prom_element_t resume_prom_element;

    fbe_semaphore_t sem;

    /* Commonly used pieces of information */
    SP_ID                               spid;
    HW_ENCLOSURE_TYPE                   processorEnclType;
    fbe_device_physical_location_t      bootEnclLocation;
    fbe_device_physical_location_t      enclLocationForOsDrives; /* The encl that OS drives reside */
    fbe_bool_t                          isSingleSpSystem; // whether this is a single SP system. eg. beachcomber single mode
    fbe_bool_t                          isSimulation; // whether this is a simulated system.
 
    /* Persistent data access */
    fbe_u8_t                                *my_persistent_data;
    fbe_u32_t                                my_persistent_data_size;
    fbe_persist_entry_id_t                   my_persistent_entry_id;
    fbe_u8_t                                *persistent_storage_read_buffer;
    fbe_u8_t                                *persistent_storage_write_buffer;
    fbe_base_environment_persistent_data_wait_peer_state_t wait_for_peer_data;
    fbe_bool_t                              read_outstanding;
    fbe_persistent_id_t                     my_persistent_id;
    
    /*memory leak detector*/
    fbe_u32_t                           memory_ex_allocate_count;
    fbe_spinlock_t                      base_env_lock;

    fbe_u32_t                           read_persistent_data_retries;

    fbe_common_cache_status_t           peerCacheStatus;

    fbe_bool_t                          abortUpgradeCmdReceived;
    fbe_bool_t                          waitBeforeUpgrade; /* whether wait before upgrade or not. */
    fbe_u32_t                           upgradeDelayInSec; /* In seconds */
    fbe_time_t                          timeStartToWaitBeforeUpgrade;   /* In milliseconds*/
    fbe_bool_t                          requestPeerRevision;
    fbe_u32_t                           peerRevision;

    fbe_bool_t                          isBootFlash;
    fbe_bool_t                          persist_db_disabled;

    /* Lifecycle defines. */
    FBE_LIFECYCLE_DEF_INST_DATA;
    FBE_LIFECYCLE_DEF_INST_COND(FBE_LIFECYCLE_COND_MAX(FBE_BASE_ENVIRONMENT_LIFECYCLE_COND_LAST));
} fbe_base_environment_t;

#define FBE_BASE_ENVIRONMENT_PERSISTENT_DATA_SIZE(base_env) ( (fbe_u8_t)sizeof(fbe_base_environment_persistent_data_header_t) + base_env->my_persistent_data_size)
#define FBE_BASE_ENVIRONMENT_PERSISTENT_DATA_AND_HEADER_SIZE(base_env) ( (fbe_u8_t)sizeof(fbe_base_environment_persistent_data_t) + base_env->my_persistent_data_size)
#define FBE_BASE_ENVIRONMENT_PERSIST_MESSAGE_SIZE(base_env) ( (fbe_u8_t)sizeof(fbe_base_environment_persistent_data_cmi_message_t) + base_env->my_persistent_data_size)

typedef struct fbe_base_environment_common_context_s
{
    fbe_packet_t            *packet;
    fbe_base_environment_t *base_environment;
}fbe_base_environment_common_context_t;


typedef fbe_status_t (* fbe_base_env_fup_transition_func_ptr_t)(fbe_base_environment_t * pBaseEnv, fbe_base_env_fup_work_item_t * pWorkItem);

static __forceinline 
fbe_status_t fbe_base_env_get_local_sp_id(fbe_base_environment_t * pBaseEnv, SP_ID * pLocalSp)
{
    *pLocalSp = pBaseEnv->spid;
    return FBE_STATUS_OK;
}

static __forceinline 
fbe_status_t fbe_base_env_get_processor_encl_type(fbe_base_environment_t * pBaseEnv, 
                                             HW_ENCLOSURE_TYPE * pProcessorEnclType)
{
    *pProcessorEnclType = pBaseEnv->processorEnclType;
    return FBE_STATUS_OK;
}

static __forceinline 
fbe_status_t fbe_base_env_get_boot_encl_location(fbe_base_environment_t * pBaseEnv, fbe_device_physical_location_t * pBootEnclLocation)
{
    *pBootEnclLocation = pBaseEnv->bootEnclLocation;
    return FBE_STATUS_OK;
}

static __forceinline 
fbe_status_t fbe_base_env_get_encl_location_for_os_drives(fbe_base_environment_t * pBaseEnv, fbe_device_physical_location_t * pEnclLocationForOsDrives)
{
    *pEnclLocationForOsDrives = pBaseEnv->enclLocationForOsDrives;
    return FBE_STATUS_OK;
}

static __forceinline
fbe_queue_head_t * fbe_base_env_get_fup_work_item_queue_head_ptr(fbe_base_environment_t * pBaseEnv)
{
    return (&pBaseEnv->fup_element_ptr->workItemQueueHead);
}

static __forceinline 
fbe_spinlock_t * fbe_base_env_get_fup_work_item_queue_lock_ptr(fbe_base_environment_t * pBaseEnv)
{
    return (&pBaseEnv->fup_element_ptr->workItemQueueLock);
}

static __forceinline 
fbe_base_env_fup_image_info_t * fbe_base_env_get_fup_image_info_ptr(fbe_base_environment_t * pBaseEnv, fbe_u32_t imageIndex)
{
    return (&pBaseEnv->fup_element_ptr->imageInfo[imageIndex]);
}

static __forceinline 
fbe_u8_t * fbe_base_env_get_fup_image_path_ptr(fbe_base_environment_t * pBaseEnv, fbe_u32_t index)
{
    return (&pBaseEnv->fup_element_ptr->imagePath[index][0]);
}

static __forceinline 
fbe_u32_t  fbe_base_env_get_fup_next_image_index(fbe_base_environment_t * pBaseEnv)
{
    return (pBaseEnv->fup_element_ptr->nextImageIndex);
}

static __forceinline 
fbe_status_t fbe_base_env_set_fup_next_image_index(fbe_base_environment_t * pBaseEnv, fbe_u32_t nextImageIndex)
{
    pBaseEnv->fup_element_ptr->nextImageIndex = nextImageIndex;
    return FBE_STATUS_OK;
}

static __forceinline 
fbe_status_t fbe_base_env_set_fup_get_full_image_path_callback(fbe_base_environment_t * pBaseEnv, 
                                   fbe_base_env_fup_get_full_image_path_callback_func_ptr_t pGetFullImagePathCallback)
{
    pBaseEnv->fup_element_ptr->pGetFullImagePathCallback = pGetFullImagePathCallback;
    return FBE_STATUS_OK;
}

static __forceinline 
fbe_status_t fbe_base_env_set_fup_get_manifest_file_full_path_callback(fbe_base_environment_t * pBaseEnv, 
                                   fbe_base_env_fup_get_manifest_file_full_path_callback_func_ptr_t pGetManifestFileFullPathCallback)
{
    pBaseEnv->fup_element_ptr->pGetManifestFileFullPathCallback = pGetManifestFileFullPathCallback;
    return FBE_STATUS_OK;
}

static __forceinline 
fbe_status_t fbe_base_env_set_fup_check_env_status_callback(fbe_base_environment_t * pBaseEnv, 
                                   fbe_base_env_fup_check_env_status_callback_func_ptr_t pCheckEnvStatusCallback)
{
    pBaseEnv->fup_element_ptr->pCheckEnvStatusCallback = pCheckEnvStatusCallback;
    return FBE_STATUS_OK;
}

static __forceinline 
fbe_status_t fbe_base_env_set_fup_get_firmware_rev_callback(fbe_base_environment_t * pBaseEnv, 
                                  fbe_base_env_fup_get_firmware_rev_callback_func_ptr_t pGetFirmwareRevCallback)
{
    pBaseEnv->fup_element_ptr->pGetFirmwareRevCallback = pGetFirmwareRevCallback;
    return FBE_STATUS_OK;
}

static __forceinline 
fbe_status_t fbe_base_env_set_fup_refresh_device_status_callback(fbe_base_environment_t * pBaseEnv, 
                                  fbe_base_env_fup_refresh_device_status_callback_func_ptr_t pRefreshDeviceStatusCallback)
{
    pBaseEnv->fup_element_ptr->pRefreshDeviceStatusCallback = pRefreshDeviceStatusCallback;
    return FBE_STATUS_OK;
}

static __forceinline 
fbe_status_t fbe_base_env_set_fup_priority_check_callback(fbe_base_environment_t *pBaseEnv, 
                                                              fbe_base_env_fup_check_priority_callback_func_ptr_t pCheckPriorityCallback)
{
    pBaseEnv->fup_element_ptr->pCheckPriorityCallback = pCheckPriorityCallback;
    return FBE_STATUS_OK;
}

static __forceinline 
fbe_status_t fbe_base_env_set_fup_copy_fw_rev_to_resume_prom(fbe_base_environment_t *pBaseEnv, 
                                                             fbe_base_env_fup_copy_fw_rev_to_resume_prom_callback_func_ptr_t pCopyFwRevToResumePromCallback)
{
    pBaseEnv->fup_element_ptr->pCopyFwRevToResumePromCallback = pCopyFwRevToResumePromCallback;
    return FBE_STATUS_OK;
}

static __forceinline 
fbe_status_t fbe_base_env_set_fup_info_ptr_callback(fbe_base_environment_t * pBaseEnv, 
                                  fbe_base_env_fup_get_fup_info_ptr_callback_func_ptr_t pGetFupInfoPtrCallback)
{
    pBaseEnv->fup_element_ptr->pGetFupInfoPtrCallback = pGetFupInfoPtrCallback;
    return FBE_STATUS_OK;
}

static __forceinline 
fbe_status_t fbe_base_env_set_fup_info_callback(fbe_base_environment_t * pBaseEnv, 
                                  fbe_base_env_fup_get_fup_info_callback_func_ptr_t  pGetFupInfoCallback)
{
    pBaseEnv->fup_element_ptr->pGetFupInfoCallback = pGetFupInfoCallback;
    return FBE_STATUS_OK;
}

static __forceinline 
fbe_status_t fbe_base_env_set_fup_initiate_upgrade_callback(fbe_base_environment_t * pBaseEnv, 
                                  fbe_base_env_fup_initiate_upgrade_callback_func_ptr_t pInitiateUpgradeCallback)
{
    pBaseEnv->fup_element_ptr->pInitiateUpgradeCallback = pInitiateUpgradeCallback;
    return FBE_STATUS_OK;
}

static __forceinline 
fbe_status_t fbe_base_env_set_fup_resume_upgrade_callback(fbe_base_environment_t * pBaseEnv, 
                                  fbe_base_env_fup_resume_upgrade_callback_func_ptr_t pResumeUpgradeCallback)
{
    pBaseEnv->fup_element_ptr->pResumeUpgradeCallback = pResumeUpgradeCallback;
    return FBE_STATUS_OK;
}

static __forceinline 
fbe_device_physical_location_t * fbe_base_env_get_fup_work_item_location_ptr(fbe_base_env_fup_work_item_t * pWorkItem)
{
    return (&pWorkItem->location);
}

static __forceinline 
fbe_base_env_fup_image_info_t * fbe_base_env_get_fup_work_item_image_info_ptr(fbe_base_env_fup_work_item_t * pWorkItem)
{
    return (pWorkItem->pImageInfo);
}

static __forceinline 
fbe_status_t fbe_base_env_set_fup_work_item_image_info_ptr(fbe_base_env_fup_work_item_t * pWorkItem, 
                                                           fbe_base_env_fup_image_info_t * pImageInfo)
{
    pWorkItem->pImageInfo = pImageInfo;
    return FBE_STATUS_OK;
}

static __forceinline 
fbe_char_t * fbe_base_env_get_fup_work_item_image_path_ptr(fbe_base_env_fup_work_item_t * pWorkItem)
{
    return (&pWorkItem->imagePath[0]);
}

static __forceinline 
fbe_base_env_fup_work_state_t fbe_base_env_get_fup_work_item_state(fbe_base_env_fup_work_item_t * pWorkItem)
{
    return (pWorkItem->workState);
}

static __forceinline 
fbe_status_t fbe_base_env_set_fup_work_item_state(fbe_base_env_fup_work_item_t * pWorkItem, 
                                             fbe_base_env_fup_work_state_t workState)
{
    pWorkItem->workState = workState;
    return FBE_STATUS_OK;
}

static __forceinline 
fbe_base_env_fup_completion_status_t fbe_base_env_get_fup_work_item_completion_status(fbe_base_env_fup_work_item_t * pWorkItem)
{
    return (pWorkItem->completionStatus);
}

static __forceinline 
fbe_status_t fbe_base_env_set_fup_work_item_completion_status(fbe_base_env_fup_work_item_t * pWorkItem, 
                                             fbe_base_env_fup_completion_status_t completionStatus)
{
    pWorkItem->completionStatus = completionStatus;
    return FBE_STATUS_OK;
}

static __forceinline 
fbe_u8_t * fbe_base_env_get_fup_work_item_logheader(fbe_base_env_fup_work_item_t * pWorkItem)
{
    return (&pWorkItem->logHeader[0]);
}

static __forceinline 
fbe_status_t fbe_base_environment_set_cmi_msg_opcode(fbe_base_environment_cmi_message_t *cmi_message, fbe_u32_t opcode)
{
    cmi_message->opcode = opcode;
    return FBE_STATUS_OK;
}

static __forceinline 
fbe_u32_t fbe_base_environment_get_cmi_msg_opcode(fbe_base_environment_cmi_message_t *cmi_message)
{
    return cmi_message->opcode;
}

static __forceinline 
fbe_base_env_fup_manifest_info_t * fbe_base_env_fup_get_manifest_info_ptr(fbe_base_environment_t * pBaseEnv,
                                                                            fbe_u32_t subEnclIndex)
{
    return (&pBaseEnv->fup_element_ptr->manifestInfo[subEnclIndex]);
}

static __forceinline 
fbe_bool_t fbe_base_env_fup_get_manifest_info_avail(fbe_base_environment_t * pBaseEnv)
{
    return (pBaseEnv->fup_element_ptr->manifestInfoAvail);
}

static __forceinline 
fbe_status_t fbe_base_env_fup_set_manifest_info_avail(fbe_base_environment_t * pBaseEnv, fbe_bool_t manifestInfoAvail)
{
    pBaseEnv->fup_element_ptr->manifestInfoAvail = manifestInfoAvail;
    return FBE_STATUS_OK;
}

static __forceinline 
fbe_enclosure_fw_target_t fbe_base_env_fup_get_manifest_fw_target(fbe_base_environment_t * pBaseEnv,
                                                                            fbe_u32_t subEnclIndex,
                                                                            fbe_u32_t imageIndex)
{
    return (pBaseEnv->fup_element_ptr->manifestInfo[subEnclIndex].firmwareTarget[imageIndex]);
}

/* fbe_base_environment_main.c */
fbe_status_t fbe_base_environment_create_object(fbe_packet_t * packet, fbe_object_handle_t * object_handle);
fbe_status_t fbe_base_environment_destroy_object( fbe_object_handle_t object_handle);
fbe_status_t fbe_base_environment_event_entry(fbe_object_handle_t object_handle, fbe_event_type_t event_type, fbe_event_context_t event_context);

fbe_status_t fbe_base_environment_monitor_load_verify(void);

fbe_status_t fbe_base_environment_init(fbe_base_environment_t * base_environment);
fbe_status_t fbe_base_environment_register_event_notification(fbe_base_environment_t * base_environment,
                                                              fbe_api_notification_callback_function_t call_back, 
                                                              fbe_notification_type_t state_mask,
                                                              fbe_u64_t device_mask,
                                                              void *context,
                                                              fbe_topology_object_type_t object_type,
                                                              fbe_package_id_t package_id);
fbe_status_t fbe_base_environment_register_cmi_notification(fbe_base_environment_t * base_environment,
                                                            fbe_cmi_client_id_t cmi_client,
                                                            fbe_base_environment_cmi_callback_function_t call_back);
fbe_status_t fbe_base_environment_send_cmi_message(fbe_base_environment_t *base_environment,
                                                   fbe_u32_t message_length,
                                                   fbe_cmi_message_t message);
fbe_status_t fbe_base_env_file_read(fbe_base_environment_t * pBaseEnv, 
                       char * fileName, 
                       fbe_u8_t * pBuffer, 
                       fbe_u32_t bytesToRead,
                       fbe_u32_t fileAccessOptions, 
                       fbe_u32_t * pBytesRead);
fbe_status_t fbe_base_env_get_file_size(fbe_base_environment_t * pBaseEnv, 
                                        char *fileName, 
                                        fbe_u32_t *pFileSize);
fbe_status_t fbe_base_env_get_and_adjust_encl_location(fbe_base_environment_t * pBaseEnv,
                                                fbe_object_id_t objectId,
                                                fbe_device_physical_location_t * pLocation);
fbe_status_t fbe_base_env_is_processor_encl(fbe_base_environment_t * pBaseEnv,
                                            fbe_device_physical_location_t * pLocation,
                                            fbe_bool_t * pProcessorEncl);
fbe_status_t fbe_base_environment_init_persistent_storage_data(fbe_base_environment_t * base_environment, fbe_packet_t *packet);
fbe_status_t fbe_base_environment_peer_gone(fbe_base_environment_t *base_environment);
fbe_status_t fbe_base_environment_init_persistent_storage_data_completion(fbe_base_environment_t * base_environment);
void fbe_base_environment_set_persistent_storage_data_size(fbe_base_environment_t * base_environment, fbe_u32_t size);
fbe_status_t fbe_base_env_read_persistent_data(fbe_base_environment_t *base_environment); 
fbe_status_t fbe_base_env_write_persistent_data(fbe_base_environment_t *base_environment);
fbe_status_t fbe_base_env_reboot_sp(fbe_base_environment_t *base_env, 
                                    SP_ID sp, 
                                    fbe_bool_t hold_in_post, 
                                    fbe_bool_t hold_in_reset,
                                    fbe_bool_t is_sync_cmd);
fbe_status_t fbe_base_env_release_cmi_msg_memory(fbe_base_environment_t *base_environment,
                                fbe_base_environment_cmi_message_info_t *cmi_message);
void fbe_base_environment_send_data_change_notification(fbe_base_environment_t *pBaseEnv,
                                                        fbe_class_id_t classId,
                                                        fbe_u64_t device_type,
                                                        fbe_device_data_type_t data_type,
                                                        fbe_device_physical_location_t *location);
fbe_bool_t fbe_base_env_is_active_sp(fbe_base_environment_t *base_environment);
fbe_bool_t fbe_base_env_is_active_sp_esp_cmi(fbe_base_environment_t *base_environment);
char * fbe_base_env_decode_cmi_msg_opcode(fbe_base_environment_cmi_message_opcode_t opcode);
void *  _fbe_base_env_memory_ex_allocate(fbe_base_environment_t * pBaseEnv, fbe_u32_t allocation_size_in_bytes, const char* callerFunctionNameString, fbe_u32_t callerLineNumber);
void  _fbe_base_env_memory_ex_release(fbe_base_environment_t * pBaseEnv, void * ptr, const char* callerFunctionNameString, fbe_u32_t callerLineNumber);
#define fbe_base_env_memory_ex_allocate(pBaseEnv, allocation_size_in_bytes) _fbe_base_env_memory_ex_allocate(pBaseEnv, allocation_size_in_bytes, __FUNCTION__, __LINE__)
#define fbe_base_env_memory_ex_release(pBaseEnv, ptr)                       _fbe_base_env_memory_ex_release(pBaseEnv, ptr, __FUNCTION__, __LINE__)

fbe_status_t fbe_base_environment_get_peerCacheStatus(fbe_base_environment_t *base_environment,
                                                     fbe_common_cache_status_t * peerCacheStatus);

fbe_status_t fbe_base_environment_set_peerCacheStatus(fbe_base_environment_t *base_environment,
                                                     fbe_common_cache_status_t peerCacheStatus);

fbe_persistent_id_t fbe_base_environment_convert_class_id_to_persistent_id(fbe_base_environment_t *base_environment, fbe_class_id_t class_id);


/* fbe_base_environment_executer.c */
fbe_status_t fbe_base_environment_send_read_persistent_data(fbe_base_environment_t *base_environment, fbe_packet_t * packet);
fbe_status_t fbe_base_environment_send_write_persistent_data(fbe_base_environment_t *base_environment, fbe_packet_t * packet);

/* fbe_base_environment_monitor.c */
fbe_status_t fbe_base_environment_monitor_entry(fbe_object_handle_t object_handle, fbe_packet_t * packet);
fbe_status_t fbe_base_environment_cmi_message_handler(fbe_base_object_t *base_object, fbe_base_environment_cmi_message_info_t *cmi_message);
fbe_status_t fbe_base_environment_cmi_process_received_message(fbe_base_environment_t *base_environment, fbe_base_environment_cmi_message_t *cmi_message);
fbe_status_t fbe_base_environment_cmi_processPeerNotPresent(fbe_base_environment_t *base_environment, fbe_base_environment_cmi_message_t *cmi_message);
fbe_status_t fbe_base_environment_updateEnclFaultLed(fbe_base_object_t * base_object,
                                                     fbe_object_id_t objectId,
                                                     fbe_bool_t enclFaultLedOn,
                                                     fbe_encl_fault_led_reason_t enclFaultLedReason);
fbe_status_t fbe_base_environment_send_cacheStatus_update_to_peer(fbe_base_environment_t *base_environment, 
                                                      fbe_common_cache_status_t CacheStatus,
                                                      fbe_u8_t CacheStatusFlag);

fbe_common_cache_status_t fbe_base_environment_combine_cacheStatus(fbe_base_environment_t *base_environment, 
                                                     fbe_common_cache_status_t localCacheStatus,
                                                     fbe_common_cache_status_t PeerCacheStatus,
                                                     fbe_class_id_t classId);

fbe_status_t fbe_base_environment_compare_cacheStatus(fbe_base_environment_t *base_environment, 
                                                     fbe_common_cache_status_t localCacheStatus,
                                                     fbe_common_cache_status_t PeerCacheStatus,
                                                     fbe_class_id_t classId);
fbe_status_t
fbe_base_environment_isPsSupportedByEncl(fbe_base_object_t * base_object,
                                         fbe_device_physical_location_t *pLocation,
                                         HW_MODULE_TYPE psFfid,
                                         fbe_bool_t dcTelcoArray,
                                         fbe_bool_t *psSupported,
                                         fbe_bool_t *eirSupported);
fbe_bool_t
fbe_base_environment_isSpInBootflash(fbe_base_object_t * base_object);

/* fbe_base_environment_usurper.c */
fbe_status_t fbe_base_environment_control_entry(fbe_object_handle_t object_handle, fbe_packet_t * packet);
fbe_status_t fbe_base_env_get_any_upgrade_in_progress(fbe_base_environment_t * pBaseEnv,
                                                      fbe_u64_t deviceType,
                                                      fbe_bool_t * pAnyUpgradeInProgress);
fbe_status_t
fbe_base_env_get_specific_upgrade_in_progress(fbe_base_environment_t * pBaseEnv,
                                              fbe_u64_t deviceType,
                                              fbe_device_physical_location_t *locationPtr,
                                              fbe_base_env_fup_work_state_t workStateToCheck,
                                              fbe_bool_t * pAnyUpgradeInProgress);

/* fbe_base_environment_fup.c */
fbe_lifecycle_status_t fbe_base_env_fup_initiate_upgrade_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet);
fbe_lifecycle_status_t fbe_base_env_fup_abort_upgrade_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet);
fbe_lifecycle_status_t fbe_base_env_fup_terminate_upgrade_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet);
fbe_lifecycle_status_t fbe_base_env_fup_wait_before_upgrade_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet);
fbe_lifecycle_status_t fbe_base_env_fup_wait_for_inter_device_delay_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet);          
fbe_lifecycle_status_t fbe_base_env_fup_read_image_header_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet);
fbe_lifecycle_status_t fbe_base_env_fup_check_rev_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet);
fbe_lifecycle_status_t fbe_base_env_fup_read_entire_image_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet);
fbe_lifecycle_status_t fbe_base_env_fup_download_image_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet);
fbe_lifecycle_status_t fbe_base_env_fup_get_download_status_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet);
fbe_lifecycle_status_t fbe_base_env_fup_get_peer_permission_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet);
fbe_lifecycle_status_t fbe_base_env_fup_check_env_status_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet);
fbe_lifecycle_status_t fbe_base_env_fup_activate_image_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet);
fbe_lifecycle_status_t fbe_base_env_fup_get_activate_status_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet);
fbe_lifecycle_status_t fbe_base_env_fup_check_result_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet);
fbe_lifecycle_status_t fbe_base_env_fup_refresh_device_status_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet);
fbe_lifecycle_status_t fbe_base_env_fup_end_upgrade_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet);
fbe_lifecycle_status_t fbe_base_env_fup_release_image_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet);

fbe_status_t fbe_base_env_fup_get_first_work_item(fbe_base_environment_t * pBaseEnv,
                                                  fbe_base_env_fup_work_item_t ** ppReturnWorkItem);
fbe_status_t fbe_base_env_fup_get_next_work_item(fbe_base_environment_t * pBaseEnv,
                                                 fbe_base_env_fup_work_item_t *pWorkItem,
                                                 fbe_base_env_fup_work_item_t ** ppNextWorkItem);

fbe_status_t fbe_base_env_fup_find_work_item(fbe_base_environment_t * pBaseEnv,
                                fbe_u64_t deviceType,
                                fbe_device_physical_location_t * pLocation,
                                fbe_base_env_fup_work_item_t ** pReturnWorkItemPtr);

fbe_status_t 
fbe_base_env_fup_find_work_item_with_specific_fw_target(fbe_base_environment_t * pBaseEnv,
                                fbe_u64_t deviceType,
                                fbe_device_physical_location_t * pLocation,
                                fbe_enclosure_fw_target_t firmwareTarget,
                                fbe_base_env_fup_work_item_t ** pReturnWorkItemPtr);

fbe_status_t fbe_base_env_fup_get_image_path_from_registry(fbe_base_environment_t * pBaseEnv, 
                                              fbe_u8_t * pImagePathKey, 
                                              fbe_u8_t * pImagePath,
                                              fbe_u32_t imagePathLength);

fbe_status_t fbe_base_env_fup_build_manifest_file_name(fbe_base_environment_t * pBaseEnv,
                                                       fbe_u8_t * pManifestFileNameBuffer,
                                                       fbe_char_t * pManifestFileNameConstantPortion,
                                                       fbe_u8_t bufferLen,
                                                       fbe_u32_t * pManifestFileNameLen);

fbe_status_t fbe_base_env_fup_create_and_add_work_item(fbe_base_environment_t * pBaseEnv,                                           
                                          fbe_device_physical_location_t * pLocation,
                                          fbe_u64_t deviceType,
                                          fbe_enclosure_fw_target_t firmwareTarget,
                                          fbe_char_t * pFirmwareRev,
                                          fbe_char_t * pSubenclProductId,
                                          HW_MODULE_TYPE uniqueId,
                                          fbe_u32_t forceFlags,
                                          fbe_char_t * pLogHeader,
                                          fbe_u32_t interDeviceDelayInSec,
                                          fbe_u32_t upgradeRetryCount,
                                          fbe_u16_t esesVersion);

fbe_status_t fbe_base_env_fup_destroy_queue(fbe_base_environment_t * pBaseEnv);
fbe_status_t fbe_base_env_fup_release_image(fbe_base_environment_t * pBaseEnv);

fbe_status_t fbe_base_env_fup_handle_firmware_status_change(fbe_base_environment_t * pBaseEnv,
                                                      fbe_u64_t deviceType,
                                                      fbe_device_physical_location_t * pLocation);

fbe_status_t fbe_base_env_fup_handle_destroy_state(fbe_base_environment_t * pBaseEnv,
                                      fbe_device_physical_location_t * pLocation);

fbe_status_t fbe_base_env_fup_handle_fail_state(fbe_base_environment_t * pBaseEnv,
                                      fbe_u32_t bus,
                                      fbe_u32_t enclosure);

fbe_status_t fbe_base_env_fup_handle_device_removal(fbe_base_environment_t * pBaseEnv, 
                                       fbe_u64_t deviceType,
                                       fbe_device_physical_location_t * pLocation);

fbe_status_t fbe_base_env_fup_get_full_image_path(fbe_base_environment_t * pBaseEnv,
                                     fbe_base_env_fup_work_item_t * pWorkItem,
                                     fbe_u32_t imagePathIndex,
                                     fbe_char_t * pImagePathKey,
                                     fbe_char_t * pImageFileName,
                                     fbe_u32_t imageFileNameSize);

fbe_status_t fbe_base_env_fup_get_manifest_file_full_path(fbe_base_environment_t * pBaseEnv,
                                             fbe_char_t * pManifestFilePathKey,
                                             fbe_char_t * pManifestFileName,
                                             fbe_u32_t manifestFileNameSize,
                                             fbe_char_t * pManifestFileFullPath);

fbe_status_t fbe_base_env_fup_remove_and_release_work_item(fbe_base_environment_t * pBaseEnv, 
                                  fbe_base_env_fup_work_item_t * pWorkItem);

fbe_status_t fbe_base_env_fup_processDeniedPeerPerm(fbe_base_environment_t * pBaseEnv, 
                                                    fbe_base_env_fup_work_item_t *pWorkItem);
fbe_status_t fbe_base_env_fup_processGotPeerPerm(fbe_base_environment_t * pBaseEnv, 
                                                     fbe_base_env_fup_work_item_t *pWorkItem);
fbe_status_t fbe_base_env_fup_processSpContactLost(fbe_base_environment_t * pBaseEnv);

fbe_status_t fbe_base_env_fup_fill_and_send_cmi(fbe_base_environment_t *pBaseEnv,
                                                fbe_base_environment_cmi_message_opcode_t msgType, 
                                                fbe_u64_t deviceType,
                                                fbe_device_physical_location_t * pLocation,
                                                fbe_base_env_fup_work_item_t *pWorkItem);

fbe_status_t fbe_base_env_fup_peerPermRetry(fbe_base_environment_t *pBaseEnv,
                                                fbe_base_env_fup_work_item_t *pWorkItem);
fbe_status_t fbe_base_env_fup_processRequestForPeerPerm(fbe_base_environment_t * pBaseEnv,
                                                        fbe_base_env_fup_cmi_packet_t *pFupCmiPacket);

fbe_status_t fbe_base_env_fup_get_image_file_name_uid_string(char *pIdString, fbe_u32_t bufferSize, fbe_u32_t uuid);

fbe_status_t fbe_base_env_fup_set_condition(fbe_base_environment_t *pBaseEnv, fbe_lifecycle_cond_id_t condId);

fbe_status_t fbe_base_env_fup_format_fw_rev(fbe_base_env_fup_format_fw_rev_t * pFwRevAfterFormatting,
                                            fbe_u8_t * pFwRev,
                                            fbe_u8_t fwRevSize);

fbe_status_t fbe_base_env_fup_process_peer_activate_start(fbe_base_environment_t * pBaseEnv, 
                                                          fbe_u64_t deviceType,
                                                          fbe_device_physical_location_t *pLocation);

fbe_status_t fbe_base_env_fup_process_peer_upgrade_end(fbe_base_environment_t * pBaseEnv, 
                                                       fbe_u64_t deviceType,
                                                       fbe_device_physical_location_t *pLocation);

fbe_status_t fbe_base_env_fup_find_file_name_from_manifest_info(fbe_base_environment_t * pBaseEnv, 
                                      fbe_base_env_fup_work_item_t * pWorkItem,
                                      fbe_char_t ** pImageFileNamePtr);

fbe_status_t fbe_base_env_fup_fw_comp_type_to_fw_target(fbe_base_environment_t * pBaseEnv, 
                                      ses_comp_type_enum firmwareCompType,
                                      fbe_enclosure_fw_target_t *pFirmwareTarget);

fbe_status_t fbe_base_env_fup_convert_fw_rev_for_rp(fbe_base_environment_t * pBaseEnv, 
                                                    fbe_u8_t * pFwRevForResume,
                                                    fbe_u8_t * pFwRev);

fbe_status_t fbe_base_env_fup_read_and_parse_manifest_file(fbe_base_environment_t *pBaseEnv);

fbe_status_t fbe_base_env_fup_init_manifest_info(fbe_base_environment_t * pBaseEnv);

fbe_u32_t fbe_base_env_get_wait_time_before_upgrade(fbe_base_environment_t * pBaseEnv);

/******************* Resume Prom Queue *********************/

static __forceinline
fbe_queue_head_t * fbe_base_env_get_resume_prom_work_item_queue_head_ptr(fbe_base_environment_t * pBaseEnv)
{
    return (&pBaseEnv->resume_prom_element.workItemQueueHead);
}

static __forceinline 
fbe_spinlock_t * fbe_base_env_get_resume_prom_work_item_queue_lock_ptr(fbe_base_environment_t * pBaseEnv)
{
    return (&pBaseEnv->resume_prom_element.workItemQueueLock);
}

static __forceinline 
fbe_device_physical_location_t * fbe_base_env_get_resume_prom_work_item_location_ptr(fbe_base_env_resume_prom_work_item_t * pWorkItem)
{
    return (&pWorkItem->location);
}

static __forceinline 
fbe_status_t fbe_base_env_initiate_resume_prom_read_callback(fbe_base_environment_t * pBaseEnv, 
                                  fbe_base_env_initiate_resume_prom_read_callback_func_ptr_t pInitiateResumePromReadCallback)
{
    pBaseEnv->resume_prom_element.pInitiateResumePromReadCallback = pInitiateResumePromReadCallback;
    return FBE_STATUS_OK;
}

static __forceinline 
fbe_status_t fbe_base_env_get_resume_prom_info_ptr_callback(fbe_base_environment_t * pBaseEnv, 
                                                        fbe_base_env_get_resume_prom_info_ptr_func_callback_ptr_t pGetResumePromInfoPtrCallback)
{
    pBaseEnv->resume_prom_element.pGetResumePromInfoPtrCallback = pGetResumePromInfoPtrCallback;
    return FBE_STATUS_OK;
}

static __forceinline 
fbe_status_t fbe_base_env_resume_prom_update_encl_fault_led_callback(fbe_base_environment_t * pBaseEnv, 
                 fbe_base_env_resume_prom_update_encl_fault_led_callback_func_ptr_t pUpdateEnclFaultLedCallback)
{
    pBaseEnv->resume_prom_element.pUpdateEnclFaultLedCallback = pUpdateEnclFaultLedCallback;
    return FBE_STATUS_OK;
}

static __forceinline 
fbe_status_t fbe_base_env_set_resume_prom_work_item_resume_status(fbe_base_env_resume_prom_work_item_t * pWorkItem, 
                                                                      fbe_resume_prom_status_t resumeStatus)
{
    pWorkItem->resume_status = resumeStatus;
    return FBE_STATUS_OK;
}

static __forceinline 
fbe_u8_t * fbe_base_env_get_resume_prom_work_item_logheader(fbe_base_env_resume_prom_work_item_t * pWorkItem)
{
    return (&pWorkItem->logHeader[0]);
}

fbe_status_t fbe_base_env_resume_prom_destroy_queue(fbe_base_environment_t * pBaseEnv);

fbe_status_t
fbe_base_env_resume_prom_create_and_add_work_item(fbe_base_environment_t * pBaseEnv, 
                                                  fbe_object_id_t object_id,
                                                  fbe_u64_t deviceType, 
                                                  fbe_device_physical_location_t * pLocation,
                                                  fbe_u8_t * pLogHeader);

fbe_status_t 
fbe_base_env_resume_prom_find_work_item(fbe_base_environment_t * pBaseEnv, 
                                        fbe_u64_t deviceType, 
                                        fbe_device_physical_location_t * pLocation,
                                        fbe_base_env_resume_prom_work_item_t ** pReturnWorkItemPtr);

fbe_status_t 
fbe_base_env_resume_prom_remove_and_release_work_item(fbe_base_environment_t * pBaseEnv, 
                                                      fbe_base_env_resume_prom_work_item_t * pWorkItem);

fbe_status_t fbe_base_env_resume_prom_get_completion_status(void *pContext, 
                                                            fbe_u64_t deviceType, 
                                                            fbe_device_physical_location_t * pLocation, 
                                                            fbe_base_env_resume_prom_info_t *resume_info);

fbe_lifecycle_status_t fbe_base_env_read_resume_prom_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet);
fbe_lifecycle_status_t fbe_base_env_update_resume_prom_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet);
fbe_lifecycle_status_t fbe_base_env_write_resume_prom_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet);
fbe_lifecycle_status_t 
fbe_base_env_check_resume_prom_outstanding_request_cond_function(fbe_base_object_t * base_object, 
                                                                 fbe_packet_t * packet);

fbe_status_t 
fbe_base_env_resume_prom_handle_device_removal(fbe_base_environment_t * pBaseEnv, 
                                               fbe_u64_t deviceType, 
                                               fbe_device_physical_location_t * pLocation);

fbe_bool_t fbe_base_env_resume_prom_is_retry_needed(fbe_base_environment_t * pBaseEnv, 
                                                    fbe_resume_prom_status_t resumeStatus,
                                                    fbe_u8_t retryCount);

fbe_status_t fbe_base_env_resume_prom_verify_checksum(fbe_base_environment_t * pBaseEnv, 
                                                      fbe_base_env_resume_prom_work_item_t * pWorkItem);

fbe_status_t fbe_base_env_resume_prom_handle_destroy_state(fbe_base_environment_t * pBaseEnv, 
                                                           fbe_u32_t bus, 
                                                           fbe_u32_t enclosure);

fbe_status_t fbe_base_env_send_resume_prom_read_sync_cmd(fbe_base_environment_t * pBaseEnv, 
                                     fbe_resume_prom_cmd_t * pReadResumePromCmd);

fbe_status_t fbe_base_env_send_resume_prom_write_sync_cmd(fbe_base_environment_t * pBaseEnv, 
                                     fbe_resume_prom_cmd_t * pWriteResumeCmd, fbe_bool_t issue_read);

fbe_status_t fbe_base_env_send_resume_prom_write_async_cmd(fbe_base_environment_t * pBaseEnv, 
                                                           fbe_resume_prom_write_resume_async_op_t *rpWriteAsynchOp);

fbe_bool_t fbe_base_env_get_resume_prom_fault(fbe_resume_prom_status_t resumeStatus);

fbe_status_t fbe_base_env_send_resume_prom_read_async_cmd(fbe_base_environment_t * pBaseEnv, 
                                                          fbe_base_env_resume_prom_work_item_t * pWorkItem);

fbe_status_t fbe_base_env_init_resume_prom_info(fbe_base_env_resume_prom_info_t * pResumePromInfo);

/* fbe_base_environmemt_kernel_main.c, fbe_base_environment_sim_main.c */
fbe_status_t fbe_base_env_fup_build_image_file_name(fbe_u8_t * pImageFileNameBuffer,
                                                   char * pImageFileNameConstantPortion,
                                                   fbe_u8_t bufferLen,
                                                   fbe_u8_t * pSubenclProductId,
                                                   fbe_u32_t * pImageFileNameLen,
                                                   fbe_u16_t esesVersion);

fbe_status_t fbe_base_env_init_fup_params(fbe_base_environment_t * pBaseEnv,
                                          fbe_u32_t delayInSecOnSPA,
                                          fbe_u32_t delayInSecOnSPB);

#endif /* BASE_ENVIRONMENT_PRIVATE_H */

/*************************************
 * end fbe_base_environment_private.h
 *************************************/
