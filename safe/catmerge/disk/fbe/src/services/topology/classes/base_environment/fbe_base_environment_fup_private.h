/***************************************************************************
 * Copyright (C) EMC Corporation 2007-2010 
 * All rights reserved. Licensed material -- property of EMC 
 * Corporation 
 ***************************************************************************/

#ifndef FBE_BASE_ENVIORNMENT_FUP_PRIVATE_H
#define FBE_BASE_ENVIORNMENT_FUP_PRIVATE_H

/*!**************************************************************************
 * @file fbe_base_environment_fup_private.h
 ***************************************************************************
 *
 * @brief
 *  This file contains private defines for the base environment firmware upgrade functionality.
 * 
 * @ingroup base_environment_class_files
 * 
 * @revision
 *   15-June-2010:  PHE - Created.
 *
 ***************************************************************************/
#include "fbe/fbe_devices.h"
#include "fbe/fbe_enclosure_interface.h"
#include "fbe/fbe_enclosure.h"
#include "fbe/fbe_esp_base_environment.h"
#include "fbe_base_object.h"
#include "base_object_private.h"
#include "fbe/fbe_eses.h"


#define FBE_BASE_ENV_FUP_MAX_LOG_HEADER_SIZE       35
#define FBE_BASE_ENV_FUP_MAX_IMAGE_TYPES           12

#define FBE_BASE_ENV_FUP_MAX_IMAGE_PATH_LENGTH     256

#define FBE_BASE_ENV_FUP_MAX_IMAGE_PATH_KEY_LEN    256

#define FBE_BASE_ENV_FUP_MAX_NUM_IMAGE_PATH        12  /* The maximum number of images for all the ESP objects. */
#define FBE_BASE_ENV_FUP_MAX_RETRY_COUNT           5  /* a total of 6 */
#define FBE_BASE_ENV_FUP_MAX_DOWNLOAD_CMD_RETRY_TIMEOUT  1800  /* 1800 seconds(30 minutes) */
#define FBE_BASE_ENV_FUP_MAX_GET_STATUS_CMD_RETRY_TIMEOUT  30  /* 30 seconds */
#define FBE_BASE_ENV_FUP_CMI_RETRY_TIME             4       // 4 sec

#define FBE_BASE_ENV_FUP_JSON_MAX_NUM_TOKENS            400       

#define FBE_BASE_ENV_FUP_COMP_TYPE_STR_SIZE                5

typedef struct fbe_base_env_fup_image_info_s 
{
    fbe_char_t     imageRev[FBE_ENCLOSURE_MAX_FIRMWARE_REV_SIZE + 1]; 
    fbe_char_t     subenclProductId[FBE_ENCLOSURE_MAX_NUM_SUBENCL_PRODUCT_ID_IN_IMAGE][FBE_ENCLOSURE_MAX_SUBENCL_PRODUCT_ID_SIZE + 1]; 
    fbe_char_t     imagePath[FBE_BASE_ENV_FUP_MAX_IMAGE_PATH_LENGTH + 1];
    fbe_char_t   * pImage;
    fbe_u32_t    imageSize;
    fbe_u32_t     headerSize;
    fbe_enclosure_fw_target_t firmwareTarget;
    fbe_u16_t     esesVersion;
    fbe_bool_t    badImage;
} fbe_base_env_fup_image_info_t;

/*! @struct fbe_base_environment_cmi_message_s
 * @brief 
 *   This structure is the base environment class'
 *   message structure.  Leaf classes will inherit this
 *   to maintain a consistent location for their message
 *   opcodes.
 */
typedef struct fbe_base_environment_cmi_message_s
{
    fbe_u32_t opcode;
}fbe_base_environment_cmi_message_t;

typedef struct fbe_base_env_fup_cmi_packet_s 
{
    fbe_base_environment_cmi_message_t   baseCmiMsg;    //fbe_base_environment_cmi_message_opcode_t msgType;
    fbe_u64_t                    deviceType;
    fbe_device_physical_location_t       location;
    void *                               pRequestorWorkItem;
} fbe_base_env_fup_cmi_packet_t;

typedef struct fbe_base_env_fup_work_item_s
{
    /*! This must be first.  
     */
    fbe_queue_element_t                 queueElement; /* MUST be first */

    fbe_u64_t                   deviceType;
    fbe_device_physical_location_t      location;
    fbe_u16_t                           esesVersion; // carry the eses version in the work item
    fbe_char_t                          firmwareRev[FBE_ENCLOSURE_MAX_FIRMWARE_REV_SIZE + 1];
    fbe_char_t                          newFirmwareRev[FBE_ENCLOSURE_MAX_FIRMWARE_REV_SIZE + 1];
    fbe_char_t                          subenclProductId[FBE_ENCLOSURE_MAX_SUBENCL_PRODUCT_ID_SIZE + 1];
    fbe_char_t                          imagePath[FBE_BASE_ENV_FUP_MAX_IMAGE_PATH_LENGTH + 1];
    fbe_char_t                          logHeader[FBE_BASE_ENV_FUP_MAX_LOG_HEADER_SIZE + 1];
    HW_MODULE_TYPE                      uniqueId;
    fbe_base_env_fup_image_info_t     * pImageInfo;
    fbe_base_env_fup_force_flags_t      forceFlags;
    fbe_base_env_fup_work_state_t       workState;
    fbe_base_env_fup_completion_status_t completionStatus;
    fbe_base_env_fup_cmi_packet_t       pendingPeerRequest;
    fbe_enclosure_fw_target_t     firmwareTarget;
    fbe_enclosure_firmware_status_t     firmwareStatus;
    fbe_enclosure_firmware_ext_status_t firmwareExtStatus;
    fbe_u32_t                           upgradeRetryCount;
    fbe_time_t                          cmdStartTimeStamp;
    fbe_time_t                          interDeviceDelayStartTime;
    fbe_u32_t                           interDeviceDelayInSec;  // In seconds.
} fbe_base_env_fup_work_item_t;


/* Used by the leaf class to track the firmware upgrade status */
typedef struct fbe_base_env_fup_persistent_info_s
{
    fbe_base_env_fup_work_item_t * pWorkItem;
    fbe_base_env_fup_completion_status_t completionStatus;
    fbe_u32_t forceFlags;
    fbe_u8_t  imageRev[FBE_ENCLOSURE_MAX_FIRMWARE_REV_SIZE + 1]; /*image file rev on SP*/
    fbe_u8_t  currentFirmwareRev[FBE_ENCLOSURE_MAX_FIRMWARE_REV_SIZE + 1]; /*current HW Firmware rev*/
    fbe_u8_t  preFirmwareRev[FBE_ENCLOSURE_MAX_FIRMWARE_REV_SIZE + 1]; /*previous HW Firmware rev*/
    fbe_bool_t suppressFltDueToPeerFup;
} fbe_base_env_fup_persistent_info_t;

typedef struct fbe_base_env_fup_format_fw_rev_s
{
    fbe_u32_t majorNumber;
    fbe_u32_t minorNumber;
    fbe_u32_t patchNumber;
} fbe_base_env_fup_format_fw_rev_t;

typedef fbe_status_t (* fbe_base_env_fup_initiate_upgrade_callback_func_ptr_t)(void * pContext, 
                                                                      fbe_u64_t deviceType, 
                                                                      fbe_device_physical_location_t * pLocation,
                                                                      fbe_u32_t forceFlags,
                                                                      fbe_u32_t upgradeRetryCount);

typedef fbe_status_t (* fbe_base_env_fup_resume_upgrade_callback_func_ptr_t)(void * pContext);

typedef fbe_status_t (* fbe_base_env_fup_get_full_image_path_callback_func_ptr_t)(void * pCallbackContext, 
                                                                                  fbe_base_env_fup_work_item_t *pWorkItem);

typedef fbe_status_t (* fbe_base_env_fup_get_manifest_file_full_path_callback_func_ptr_t)(void * pCallbackContext, 
                                                                                  fbe_char_t *pManifestFileFullPath);

typedef fbe_status_t (* fbe_base_env_fup_check_env_status_callback_func_ptr_t)(void * pCallbackContext, 
                                                                               fbe_base_env_fup_work_item_t *pWorkItem);

typedef fbe_status_t (* fbe_base_env_fup_get_firmware_rev_callback_func_ptr_t)(void * pCallbackContext, 
                                                                                     fbe_base_env_fup_work_item_t * pWorkItem, 
                                                                               fbe_u8_t * pNewFirmwareRev);

typedef fbe_status_t (* fbe_base_env_fup_refresh_device_status_callback_func_ptr_t)(void * pCallbackContext, 
                                                                               fbe_base_env_fup_work_item_t * pWorkItem);

typedef fbe_status_t (* fbe_base_env_fup_get_fup_info_ptr_callback_func_ptr_t)(void * pCallbackContext, 
                                                                           fbe_u64_t deviceType,
                                                                           fbe_device_physical_location_t * pLocation, 
                                                                           fbe_base_env_fup_persistent_info_t ** pFupInfoPtr);
typedef fbe_status_t (* fbe_base_env_fup_get_fup_info_callback_func_ptr_t)(void * pCallbackContext, 
                                                                           fbe_u64_t deviceType,
                                                                           fbe_device_physical_location_t * pLocation, 
                                                                           fbe_esp_base_env_get_fup_info_t  * pGetFupInfo);

typedef fbe_status_t (* fbe_base_env_fup_check_priority_callback_func_ptr_t)(void *pContext,
                                                                                 fbe_base_env_fup_work_item_t *pWorkItem,
                                                                                 fbe_bool_t *pWait);

typedef fbe_status_t (* fbe_base_env_fup_copy_fw_rev_to_resume_prom_callback_func_ptr_t)(void *pContext,
                                                                      fbe_u64_t deviceType, 
                                                                      fbe_device_physical_location_t * pLocation);
typedef struct fbe_base_env_fup_element_s 
{
    /*! Queue to hold all the work items
     */
    fbe_queue_head_t workItemQueueHead;

    /*! Lock for the work item queue
     */
    fbe_spinlock_t workItemQueueLock;
    fbe_base_env_fup_image_info_t imageInfo[FBE_BASE_ENV_FUP_MAX_IMAGE_TYPES];   
    fbe_bool_t manifestInfoAvail; 
    fbe_base_env_fup_manifest_info_t manifestInfo[FBE_BASE_ENV_FUP_MAX_SUBENCL_COUNT_IN_MANIFEST];
    fbe_u8_t imagePath[FBE_BASE_ENV_FUP_MAX_NUM_IMAGE_PATH][FBE_BASE_ENV_FUP_MAX_IMAGE_PATH_LENGTH];
    fbe_u32_t nextImageIndex;

    fbe_base_env_fup_get_full_image_path_callback_func_ptr_t    pGetFullImagePathCallback;
    fbe_base_env_fup_get_manifest_file_full_path_callback_func_ptr_t    pGetManifestFileFullPathCallback;
    fbe_base_env_fup_check_env_status_callback_func_ptr_t       pCheckEnvStatusCallback;
    fbe_base_env_fup_get_firmware_rev_callback_func_ptr_t       pGetFirmwareRevCallback;
    fbe_base_env_fup_refresh_device_status_callback_func_ptr_t  pRefreshDeviceStatusCallback;
    fbe_base_env_fup_get_fup_info_ptr_callback_func_ptr_t       pGetFupInfoPtrCallback;
    fbe_base_env_fup_get_fup_info_callback_func_ptr_t             pGetFupInfoCallback;
    fbe_base_env_fup_initiate_upgrade_callback_func_ptr_t       pInitiateUpgradeCallback;
    fbe_base_env_fup_resume_upgrade_callback_func_ptr_t         pResumeUpgradeCallback;
    fbe_base_env_fup_check_priority_callback_func_ptr_t         pCheckPriorityCallback;
    fbe_base_env_fup_copy_fw_rev_to_resume_prom_callback_func_ptr_t pCopyFwRevToResumePromCallback;
}fbe_base_env_fup_element_t;



#endif /* FBE_BASE_ENVIORNMENT_FUP_PRIVATE_H */
