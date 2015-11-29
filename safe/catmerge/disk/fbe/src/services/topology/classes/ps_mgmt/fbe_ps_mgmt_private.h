/***************************************************************************
 * Copyright (C) EMC Corporation 2007-2010 
 * All rights reserved. Licensed material -- property of EMC 
 * Corporation 
 ***************************************************************************/

#ifndef PS_MGMT_PRIVATE_H
#define PS_MGMT_PRIVATE_H

/*!**************************************************************************
 * @file fbe_ps_mgmt_private.h
 ***************************************************************************
 *
 * @brief
 *  This file contains private defines for the PS Management object. 
 * 
 * @ingroup ps_mgmt_class_files
 * 
 * @revision
 *   04/15/2010:  Created. Joe Perry
 *
 ***************************************************************************/

#include "fbe_base_object.h"
#include "fbe/fbe_board.h"
#include "fbe/fbe_ps_info.h"
#include "fbe/fbe_time.h"
#include "fbe/fbe_devices.h"
#include "base_object_private.h"
#include "fbe_base_environment_private.h"
#include "fbe/fbe_api_esp_encl_mgmt_interface.h"

#define FBE_PS_MGMT_CDES_FUP_IMAGE_PATH_KEY   "CDESPowerSupplyImagePath"
#define FBE_PS_MGMT_CDES_FUP_IMAGE_FILE_NAME  "SXP36x6g_Power_Supply_"
#define FBE_PS_MGMT_CDES11_FUP_IMAGE_PATH_KEY   "CDES11PowerSupplyImagePath"
#define FBE_PS_MGMT_CDES11_FUP_IMAGE_FILE_NAME  "CDES_1_1_Power_Supply_"
/* Use FBE_PS_MGMT_CDES_FUP_IMAGE_PATH_KEY for PS unifined firmware upgrade path */
#define FBE_PS_MGMT_CDES_UNIFIED_FUP_IMAGE_FILE_NAME  "CDES_Power_Supply_"

#define FBE_PS_MGMT_CDES2_FUP_IMAGE_PATH_KEY    "CDES2PowerSupplyImagePath"
#define FBE_PS_MGMT_CDES2_FUP_MANIFEST_PATH_KEY FBE_PS_MGMT_CDES2_FUP_IMAGE_PATH_KEY
#define FBE_PS_MGMT_CDES2_FUP_MANIFEST_NAME         "manifest_ps"
#define FBE_PS_MGMT_CDES2_FUP_FILE_JUNO2            "juno2"
#define FBE_PS_MGMT_CDES2_FUP_FILE_LASERBEAK        "laserbeak"
#define FBE_PS_MGMT_CDES2_FUP_FILE_3GVE             "3gve"
#define FBE_PS_MGMT_CDES2_FUP_FILE_OCTANE           "octane"
#define FBE_PS_MGMT_CDES2_FUP_FILE_OPTIMUS_FLEX     "optimus_flex"
#define FBE_PS_MGMT_CDES2_FUP_FILE_OPTIMUS_ACBEL    "optimus_acbel"
#define FBE_PS_MGMT_CDES2_FUP_FILE_OPTIMUS_FLEX2    "optimus_flex2"
#define FBE_PS_MGMT_CDES2_FUP_FILE_OPTIMUS_ACBEL2   "optimus_acbel2"
#define FBE_PS_MGMT_CDES2_FUP_FILE_TABASCO_DC       "tabasco_dc"

#define FBE_PS_MGMT_MAX_PROGRAMMABLE_COUNT_PER_PS_SLOT  1

#define FBE_PS_MGMT_PERSISTENT_DATA_STORE_SIZE sizeof(fbe_ps_mgmt_expected_ps_type_t)

/* Lifecycle definitions
 * Define the PS management lifecycle's instance.
 */
extern fbe_lifecycle_const_t FBE_LIFECYCLE_CONST_DATA(ps_mgmt);

/* These are the lifecycle condition ids for PS
   Management object.*/

/*! @enum fbe_ps_mgmt_lifecycle_cond_id_t  
 *  
 *  @brief These are the lifecycle conditions for the PS
 *  management object.
                                                               */
typedef enum fbe_ps_mgmt_lifecycle_cond_id_e 
{
    /*! Processing of new PS
     */
    FBE_PS_MGMT_DISCOVER_PS = FBE_LIFECYCLE_COND_FIRST_ID(FBE_CLASS_ID_PS_MGMT),
    FBE_PS_MGMT_DEBOUNCE_TIMER_COND,

    FBE_PS_MGMT_LIFECYCLE_COND_LAST /* must be last */
} 
fbe_ps_mgmt_lifecycle_cond_id_t;

typedef enum
{
    FBE_ESP_PS_SOURCE_NONE      = 0,
    FBE_ESP_PS_SOURCE_BOARD,
    FBE_ESP_PS_SOURCE_ENCLOSURE,
} fbe_esp_ps_info_source_t;

typedef enum fbe_esp_ps_image_index_e
{
    FBE_PS_MGMT_CDES1_CDES_INDEX = 0,
    FBE_PS_MGMT_CDES1_CDES11_INDEX,
    FBE_PS_MGMT_CDES1_UNIFIED_INDEX,
    FBE_PS_MGMT_CDES2_JUNO_INDEX,
    FBE_PS_MGMT_CDES2_LASERBEAK_INDEX,
    FBE_PS_MGMT_CDES2_3GVE_INDEX,
    FBE_PS_MGMT_CDES2_OCTANE_INDEX,
    FBE_PS_MGMT_CDES2_OPTIMUS_ACBEL_INDEX,
    FBE_PS_MGMT_CDES2_OPTIMUS_FLEX_INDEX,
    FBE_PS_MGMT_CDES2_TABASCO_DC_INDEX,
    FBE_PS_MGMT_CDES2_OPTIMUS_FLEX2_INDEX,
    FBE_PS_MGMT_CDES2_OPTIMUS_ACBEL2_INDEX
} fbe_esp_ps_image_index_t;

typedef void * fbe_ps_mgmt_cmi_packet_t;


/*!****************************************************************************
 *    
 * @struct fbe_ps_debounce_info_s
 *  
 * @brief 
 *   This is the definition of the PS debounce structure. This is used to
 *   ignore short duration PS faults.
 ******************************************************************************/
typedef struct fbe_ps_debounce_info_s
{
    fbe_bool_t          debounceTimerActive;    // timer is running
    fbe_bool_t          faultMasked;            // fault is suppressed
    fbe_bool_t          debouncePeriodExpired;    // debounce timer condition has run
    //fbe_mgmt_status_t   generalFault;
} fbe_ps_debounce_info_t;

/*!****************************************************************************
 *    
 * @struct fbe_encl_ps_info_s
 *  
 * @brief 
 *   This is the definition of the PS mgmt object. This object
 *   deals with handling PS related functions
 ******************************************************************************/
typedef struct fbe_encl_ps_info_s
{
    fbe_bool_t                          inUse;
    fbe_object_id_t                     objectId;
    fbe_u32_t                           psCount;
    fbe_device_physical_location_t      location;
    fbe_power_supply_info_t             psInfo[FBE_ESP_PS_MAX_COUNT_PER_ENCL];
    fbe_base_env_fup_persistent_info_t  psFupInfo[FBE_ESP_PS_MAX_COUNT_PER_ENCL];
    fbe_base_env_resume_prom_info_t     psResumeInfo[FBE_ESP_PS_MAX_COUNT_PER_ENCL];
    fbe_ps_debounce_info_t              psDebounceInfo[FBE_ESP_PS_MAX_COUNT_PER_ENCL];
} fbe_encl_ps_info_t;

typedef struct fbe_encl_ps_info_array_s
{
    fbe_encl_ps_info_t      enclPsInfo[FBE_ESP_MAX_ENCL_COUNT];
} fbe_encl_ps_info_array_t;

/*!****************************************************************************
 *    
 * @struct fbe_ps_mgmt_s
 *  
 * @brief 
 *   This is the definition of the PS mgmt object. This object
 *   deals with handling PS related functions
 ******************************************************************************/
typedef struct fbe_ps_mgmt_s {
    /*! This must be first.  This is the base object we inherit from.
     */
    fbe_base_environment_t      base_environment;

    fbe_lifecycle_state_t       state;

    //  PS info
    fbe_u8_t                    enclCount;
    fbe_encl_ps_info_array_t    *psInfo;

    fbe_common_cache_status_t   psCacheStatus;
    fbe_ps_mgmt_expected_ps_type_t expected_power_supply;

    /* Lifecycle defines. */
    FBE_LIFECYCLE_DEF_INST_DATA;
    FBE_LIFECYCLE_DEF_INST_COND(FBE_LIFECYCLE_COND_MAX(FBE_PS_MGMT_LIFECYCLE_COND_LAST));
} fbe_ps_mgmt_t;


/* fbe_ps_mgmt_main.c */
fbe_status_t fbe_ps_mgmt_create_object(fbe_packet_t * packet, 
                                        fbe_object_handle_t * object_handle);
fbe_status_t fbe_ps_mgmt_destroy_object( fbe_object_handle_t object_handle);
fbe_status_t fbe_ps_mgmt_control_entry(fbe_object_handle_t object_handle, 
                                        fbe_packet_t * packet);
fbe_status_t fbe_ps_mgmt_monitor_entry(fbe_object_handle_t object_handle, 
                                        fbe_packet_t * packet);
fbe_status_t fbe_ps_mgmt_event_entry(fbe_object_handle_t object_handle, 
                                      fbe_event_type_t event_type, 
                                      fbe_event_context_t event_context);

fbe_status_t fbe_ps_mgmt_monitor_load_verify(void);

fbe_status_t fbe_ps_mgmt_init(fbe_ps_mgmt_t * ps_mgmt);
fbe_status_t fbe_ps_mgmt_init_encl_ps_info(fbe_encl_ps_info_t * pEnclPsInfo);

/* fbe_ps_mgmt_monitor.c */
fbe_status_t fbe_ps_mgmt_get_encl_ps_info_by_location(fbe_ps_mgmt_t * pPsMgmt,
                                                       fbe_device_physical_location_t * pLocation,
                                                       fbe_encl_ps_info_t ** pEnclPsInfoPtr);

fbe_status_t fbe_ps_mgmt_update_encl_fault_led(fbe_ps_mgmt_t *pPsMgmt,
                                    fbe_device_physical_location_t *pLocation,
                                    fbe_encl_fault_led_reason_t enclFaultLedReason);

fbe_bool_t fbe_ps_mgmt_get_ps_logical_fault(fbe_power_supply_info_t *pPsInfo);

fbe_status_t fbe_ps_mgmt_processPsStatus(fbe_ps_mgmt_t * pPsMgmt, 
                                                fbe_object_id_t objectId,
                                                fbe_device_physical_location_t * pLocation);

/* fbe_ps_mgmt_fup.c */
fbe_status_t fbe_ps_mgmt_fup_handle_ps_status_change(fbe_ps_mgmt_t * pPsMgmt,
                                                     fbe_device_physical_location_t * pLocation,
                                                     fbe_power_supply_info_t * pPsInfo,
                                                     fbe_power_supply_info_t * pPrevPsInfo,
                                                     fbe_bool_t debounceInProgress);

fbe_status_t fbe_ps_mgmt_fup_initiate_upgrade(void * pContext, 
                                              fbe_u64_t deviceType, 
                                              fbe_device_physical_location_t * pLocation,
                                              fbe_u32_t forceFlags,
                                              fbe_u32_t upgradeRetryCount);

fbe_status_t fbe_ps_mgmt_fup_get_full_image_path(void * pContext,                               
                               fbe_base_env_fup_work_item_t * pWorkItem);

fbe_status_t fbe_ps_mgmt_fup_get_manifest_file_full_path(void * pContext,
                                                         fbe_char_t * pManifestFilePath);

fbe_status_t fbe_ps_mgmt_fup_check_env_status(void * pContext, 
                                 fbe_base_env_fup_work_item_t * pWorkItem);

fbe_status_t fbe_ps_mgmt_fup_get_firmware_rev(void * pContext,
                          fbe_base_env_fup_work_item_t * pWorkItem,
                          fbe_u8_t * pFirmwareRev);

fbe_status_t fbe_ps_mgmt_get_fup_info_ptr(void * pContext,
                                            fbe_u64_t deviceType,
                                            fbe_device_physical_location_t * pLocation,
                                            fbe_base_env_fup_persistent_info_t ** pFupInfoPtr);

fbe_status_t fbe_ps_mgmt_get_fup_info(void * pContext,
                                            fbe_u64_t deviceType,
                                            fbe_device_physical_location_t * pLocation,
                                            fbe_esp_base_env_get_fup_info_t *  pGetFupInfo);

fbe_status_t fbe_ps_mgmt_fup_resume_upgrade(void * pContext);

fbe_status_t fbe_ps_mgmt_fup_copy_fw_rev_to_resume_prom(fbe_ps_mgmt_t *pPsMgmt,
                                                        fbe_u64_t deviceType,
                                                        fbe_device_physical_location_t *pLocation);

fbe_status_t fbe_ps_mgmt_fup_refresh_device_status(void * pContext,
                                             fbe_base_env_fup_work_item_t * pWorkItem);

fbe_status_t fbe_ps_mgmt_fup_new_contact_init_upgrade(fbe_ps_mgmt_t *pPsMgmt);

/* fbe_ps_mgmt_resume_prom.c */
fbe_status_t fbe_ps_mgmt_initiate_resume_prom_read(void * pContext, 
                                              fbe_u64_t deviceType,
                                              fbe_device_physical_location_t * pLocation);

fbe_status_t fbe_ps_mgmt_resume_prom_handle_ps_status_change(fbe_ps_mgmt_t * pPsMgmt,
                                                 fbe_device_physical_location_t * pLocation,
                                                 fbe_power_supply_info_t * pNewPsInfo,
                                                 fbe_power_supply_info_t * pOldPsInfo);

fbe_status_t fbe_ps_mgmt_get_resume_prom_info_ptr(fbe_ps_mgmt_t * pPsMgmt,
                                                  fbe_u64_t deviceType,
                                                  fbe_device_physical_location_t * pLocation,
                                                  fbe_base_env_resume_prom_info_t ** pResumePromInfoPtr);

fbe_status_t fbe_ps_mgmt_resume_prom_update_encl_fault_led(fbe_ps_mgmt_t *pPsMgmt,
                                                           fbe_u64_t deviceType,
                                                           fbe_device_physical_location_t *pLocation);
#endif /* PS_MGMT_PRIVATE_H */

/*******************************
 * end fbe_ps_mgmt_private.h
 *******************************/
