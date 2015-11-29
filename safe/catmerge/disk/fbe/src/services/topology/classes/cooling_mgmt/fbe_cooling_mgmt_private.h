/***************************************************************************
 * Copyright (C) EMC Corporation 2007-2010 
 * All rights reserved. Licensed material -- property of EMC 
 * Corporation 
 ***************************************************************************/

#ifndef COOLING_MGMT_PRIVATE_H
#define COOLING_MGMT_PRIVATE_H

/*!**************************************************************************
 * @file fbe_cooling_mgmt_private.h
 ***************************************************************************
 *
 * @brief
 *  This file contains private defines for the Cooling Management object. 
 * 
 * @ingroup cooling_mgmt_class_files
 * 
 * @version
 *   17-Jan-2011: PHE - Created. 
 *
 ***************************************************************************/

#include "fbe_base_object.h"
#include "fbe/fbe_board.h"
#include "fbe/fbe_time.h"
#include "fbe/fbe_devices.h"
#include "base_object_private.h"
#include "fbe_base_environment_private.h"

#define FBE_COOLING_MGMT_MAX_FAN_COUNT 250

#define FBE_COOLING_MGMT_CDES11_FUP_IMAGE_PATH_KEY   "CDES11PowerSupplyImagePath"
#define FBE_COOLING_MGMT_CDES11_FUP_IMAGE_FILE_NAME  "CDES_1_1_Cooling_Module_"

#define FBE_COOLING_MGMT_MAX_PROGRAMMABLE_COUNT_PER_COOLING_SLOT  1


/* Lifecycle definitions
 * Define the COOLING management lifecycle's instance.
 */
extern fbe_lifecycle_const_t FBE_LIFECYCLE_CONST_DATA(cooling_mgmt);

/* These are the lifecycle condition ids for COOLING
   Management object.*/

/*! @enum fbe_cooling_mgmt_lifecycle_cond_id_t  
 *  
 *  @brief These are the lifecycle conditions for the COOLING
 *  management object.
                                                               */
typedef enum fbe_cooling_mgmt_lifecycle_cond_id_e 
{
    FBE_COOLING_MGMT_LIFECYCLE_COND_DISCOVER_COOLING = FBE_LIFECYCLE_COND_FIRST_ID(FBE_CLASS_ID_COOLING_MGMT),
    
    FBE_COOLING_MGMT_LIFECYCLE_COND_LAST /* must be last */
} 
fbe_cooling_mgmt_lifecycle_cond_id_t;

typedef void * fbe_cooling_mgmt_cmi_packet_t;

/*!****************************************************************************
 *    
 * @struct fbe_cooling_mgmt_cooling_info_s
 *  
 * @brief 
 *   This structure stores information about the cooling component.
 ******************************************************************************/
typedef struct fbe_cooling_mgmt_fan_info_s {
    fbe_object_id_t                    objectId; /* Source object id */
    fbe_bool_t                         inProcessEncl;
    fbe_device_physical_location_t     location; 
    fbe_cooling_info_t                 basicInfo;
    fbe_base_env_resume_prom_info_t    fanResumeInfo;
    fbe_base_env_fup_persistent_info_t fanFupInfo;
}fbe_cooling_mgmt_fan_info_t;


/*!****************************************************************************
 *    
 * @struct fbe_cooling_mgmt_s
 *  
 * @brief 
 *   This is the definition of the COOLING mgmt object. This object
 *   deals with handling COOLING related functions
 ******************************************************************************/
typedef struct fbe_cooling_mgmt_s {
    /*! This must be first.  This is the base object we inherit from.
     */
    fbe_base_environment_t      base_environment;

    fbe_u32_t                   fanCount;
    fbe_topology_object_type_t  dpeFanSourceObjectType;
    fbe_u32_t                   dpeFanSourceOffset;

    /*! A pointer to a list of FAN info structures.  One entry per FAN. 
     */
    fbe_cooling_mgmt_fan_info_t  * pFanInfo;

    fbe_common_cache_status_t       coolingCacheStatus;

    /* Lifecycle defines. */
    FBE_LIFECYCLE_DEF_INST_DATA;
    FBE_LIFECYCLE_DEF_INST_COND(FBE_LIFECYCLE_COND_MAX(FBE_COOLING_MGMT_LIFECYCLE_COND_LAST));
} fbe_cooling_mgmt_t;

/* fbe_cooling_mgmt_main.c */
fbe_status_t fbe_cooling_mgmt_create_object(fbe_packet_t * packet, 
                                        fbe_object_handle_t * object_handle);
fbe_status_t fbe_cooling_mgmt_destroy_object( fbe_object_handle_t object_handle);
fbe_status_t fbe_cooling_mgmt_control_entry(fbe_object_handle_t object_handle, 
                                        fbe_packet_t * packet);
fbe_status_t fbe_cooling_mgmt_monitor_entry(fbe_object_handle_t object_handle, 
                                        fbe_packet_t * packet);
fbe_status_t fbe_cooling_mgmt_event_entry(fbe_object_handle_t object_handle, 
                                      fbe_event_type_t event_type, 
                                      fbe_event_context_t event_context);

fbe_status_t fbe_cooling_mgmt_monitor_load_verify(void);

fbe_status_t fbe_cooling_mgmt_init(fbe_cooling_mgmt_t * cooling_mgmt);

fbe_status_t fbe_cooling_mgmt_init_fan_info(fbe_cooling_mgmt_fan_info_t * pFanInfo);

/* fbe_cooling_mgmt_monitor.c */
fbe_status_t fbe_cooling_mgmt_get_fan_info_by_location(fbe_cooling_mgmt_t * pCoolingMgmt,
                                                       fbe_device_physical_location_t * pLocation,
                                                       fbe_cooling_mgmt_fan_info_t ** pFanInfoPtr);

fbe_status_t fbe_cooling_mgmt_update_encl_fault_led(fbe_cooling_mgmt_t *pCoolingMgmt,
                                    fbe_device_physical_location_t *pLocation,
                                    fbe_encl_fault_led_reason_t enclFaultLedReason);

fbe_status_t fbe_cooling_mgmt_process_fan_status(fbe_cooling_mgmt_t * pCoolingMgmt,
                                                 fbe_object_id_t objectId,
                                                 fbe_device_physical_location_t * pLocation,
                                                 fbe_u32_t secondaryFanSourceOffset);

/* fbe_cooling_mgmt_fup.c */
fbe_status_t fbe_cooling_mgmt_fup_handle_fan_status_change(fbe_cooling_mgmt_t * pCoolingMgmt,
                                                 fbe_device_physical_location_t * pLocation,
                                                 fbe_cooling_info_t * pNewBasicFanInfo,
                                                 fbe_cooling_info_t * pOldBasicFanInfo);

fbe_status_t fbe_cooling_mgmt_fup_initiate_upgrade(void * pContext, 
                                              fbe_u64_t deviceType, 
                                              fbe_device_physical_location_t * pLocation,
                                              fbe_u32_t forceFlags,
                                              fbe_u32_t upgradeRetryCount);

fbe_status_t fbe_cooling_mgmt_fup_get_full_image_path(void * pContext,                               
                               fbe_base_env_fup_work_item_t * pWorkItem);

fbe_status_t fbe_cooling_mgmt_fup_check_env_status(void * pContext, 
                                 fbe_base_env_fup_work_item_t * pWorkItem);

fbe_status_t fbe_cooling_mgmt_fup_get_firmware_rev(void * pContext,
                          fbe_base_env_fup_work_item_t * pWorkItem,
                          fbe_u8_t * pFirmwareRev);

fbe_status_t fbe_cooling_mgmt_get_fup_info_ptr(void * pContext,
                                                   fbe_u64_t deviceType,
                                                   fbe_device_physical_location_t * pLocation,
                                                   fbe_base_env_fup_persistent_info_t ** pFupInfoPtr);

fbe_status_t fbe_cooling_mgmt_get_fup_info(void * pContext,
                                                   fbe_u64_t deviceType,
                                                   fbe_device_physical_location_t * pLocation,
                                                   fbe_esp_base_env_get_fup_info_t *  pGetFupInfo);

fbe_status_t fbe_cooling_mgmt_fup_resume_upgrade(void * pContext);

fbe_status_t fbe_cooling_mgmt_fup_copy_fw_rev_to_resume_prom(fbe_cooling_mgmt_t *pCoolingMgmt,
                                                        fbe_u64_t deviceType,
                                                        fbe_device_physical_location_t *pLocation);

fbe_status_t fbe_cooling_mgmt_fup_initiate_upgrade_for_all(fbe_cooling_mgmt_t * pCoolingMgmt);

fbe_status_t fbe_cooling_mgmt_fup_refresh_device_status(void * pContext,
                                             fbe_base_env_fup_work_item_t * pWorkItem);

fbe_status_t fbe_cooling_mgmt_fup_new_contact_init_upgrade(fbe_cooling_mgmt_t *pCoolingMgmt);

/* fbe_cooling_mgmt_resume_prom.c */
fbe_status_t fbe_cooling_mgmt_resume_prom_handle_status_change(fbe_cooling_mgmt_t * pCoolMgmt,
                                                               fbe_device_physical_location_t * pLocation,
                                                               fbe_cooling_info_t * pNewFanInfo,
                                                               fbe_cooling_info_t * pOldFanInfo);
fbe_status_t fbe_cooling_mgmt_initiate_resume_prom_read(void * pContext, 
                                                        fbe_u64_t deviceType,
                                                        fbe_device_physical_location_t * pLocation);

fbe_status_t fbe_cooling_mgmt_get_resume_prom_info_ptr(fbe_cooling_mgmt_t *pCoolMgmt,
                                                       fbe_u64_t deviceType,
                                                       fbe_device_physical_location_t * pLocation,
                                                       fbe_base_env_resume_prom_info_t ** pResumePromInfoPtr);

fbe_status_t fbe_cooling_mgmt_resume_prom_update_encl_fault_led(fbe_cooling_mgmt_t *pCoolMgmt,
                                                                fbe_u64_t deviceType,
                                                                fbe_device_physical_location_t *pLocation);

#endif /* COOLING_MGMT_PRIVATE_H */

/*******************************
 * end fbe_cooling_mgmt_private.h
 *******************************/
