/***************************************************************************
 * Copyright (C) EMC Corporation 2007-2010 
 * All rights reserved. Licensed material -- property of EMC 
 * Corporation 
 ***************************************************************************/

#ifndef ENCL_MGMT_PRIVATE_H
#define ENCL_MGMT_PRIVATE_H

/*!**************************************************************************
 * @file fbe_encl_mgmt_private.h
 ***************************************************************************
 *
 * @brief
 *  This file contains private defines for the Enclosure
 *  Management object. 
 * 
 * @ingroup encl_mgmt_class_files
 * 
 * @revision
 *   02/25/2010:  Created. Ashok Tamilarasan
 *
 ***************************************************************************/

#include "fbe_base_object.h"
#include "base_object_private.h"
#include "fbe_base_environment_private.h"
#include "resume_prom.h"
#include "fbe/fbe_pe_types.h"
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_enclosure_data_access_public.h"
#include "fbe/fbe_enclosure_interface.h"
#include "fbe/fbe_eir_info.h"
#include "fbe/fbe_api_esp_encl_mgmt_interface.h"

#define FBE_ENCL_MGMT_CDES_FUP_IMAGE_PATH_KEY   "CDESLccImagePath"
#define FBE_ENCL_MGMT_CDES_FUP_IMAGE_FILE_NAME  "SXP36x6G_Bundled_Fw"
#define FBE_ENCL_MGMT_CDES11_FUP_IMAGE_PATH_KEY   "CDES11LccImagePath"
#define FBE_ENCL_MGMT_CDES11_FUP_IMAGE_FILE_NAME  "CDES_1_1_Bundled_Fw"
#define FBE_ENCL_MGMT_JDES_FUP_IMAGE_PATH_KEY   "JDESLccImagePath"
#define FBE_ENCL_MGMT_JDES_FUP_IMAGE_FILE_NAME  "JDES_Bundled_FW"
#define FBE_ENCL_MGMT_CDES_UNIFIED_FUP_IMAGE_PATH_KEY   "CDESUnifiedLccImagePath"
#define FBE_ENCL_MGMT_CDES_UNIFIED_FUP_IMAGE_FILE_NAME  "CDES_Bundled_FW"

#define FBE_ENCL_MGMT_CDES2_FUP_IMAGE_PATH_KEY   "CDES2LccImagePath"
#define FBE_ENCL_MGMT_CDES2_FUP_CDEF_IMAGE_FILE_NAME  "cdef"
#define FBE_ENCL_MGMT_CDES2_FUP_CDEF_DUAL_IMAGE_FILE_NAME  "cdef_dual"
#define FBE_ENCL_MGMT_CDES2_FUP_ISTR_IMAGE_FILE_NAME  "istr"
#define FBE_ENCL_MGMT_CDES2_FUP_CDES_ROM_IMAGE_FILE_NAME  "cdes_rom"

#define FBE_ENCL_MGMT_CDES2_FUP_MANIFEST_FILE_PATH_KEY  "CDES2LccImagePath"
#define FBE_ENCL_MGMT_CDES2_FUP_MANIFEST_FILE_NAME  "manifest_lcc"

#define FBE_ESP_MAX_LCCS_PER_ENCL 4
#define FBE_ESP_EE_LCC_START_SLOT 2 // the slot number of Voyager enclosure EE LCC starts from 2.
#define FBE_ESP_MAX_LCCS_PER_SUBENCL 2
#define FBE_ESP_MAX_CONNECTORS_PER_ENCL 16  // Includes the entire connectors on both SPA and SPB sides.
#define FBE_ESP_MAX_FANS_PER_ENCL 10
#define FBE_ESP_MAX_SUBENCLS_PER_ENCL 4  // if this change, change FBE_ENCLOSURE_MAX_SUBENCLS_PER_ENCL too.


#define FBE_ENCL_MGMT_SHUTDOWN_REASON_DEBOUNCE_TIMEOUT 15 /* Debounce timeout in seconds for shutdownReason */

/* Lifecycle definitions
 * Define the Enclosure management lifecycle's instance.
 */
extern fbe_lifecycle_const_t FBE_LIFECYCLE_CONST_DATA(encl_mgmt);

/* These are the lifecycle condition ids for Enclosure
   Management object.*/

/*! @enum fbe_encl_mgmt_lifecycle_cond_id_t  
 *  
 *  @brief These are the lifecycle conditions for the enclosure
 *  management object.
                                                               */
typedef enum fbe_encl_mgmt_lifecycle_cond_id_e 
{
    /*! Processing of new enclosure
     */
    FBE_ENCL_MGMT_DISCOVER_ENCLOSURE = FBE_LIFECYCLE_COND_FIRST_ID(FBE_CLASS_ID_ENCL_MGMT),

    FBE_ENCL_MGMT_GET_EIR_DATA_COND,
    
    FBE_ENCL_MGMT_CHECK_ENCL_SHUTDOWN_DELAY_COND,

    FBE_ENCL_MGMT_RESET_ENCL_SHUTDOWN_TIMER_COND,

    FBE_ENCL_MGMT_PROCESS_CACHE_STATUS_COND,

    FBE_ENCL_MGMT_READ_ARRAY_MIDPLANE_RP_COND,

    FBE_BASE_ENV_LIFECYCLE_COND_CHECK_SYSTEM_ID_INFO,

    FBE_ENCL_MGMT_WRITE_ARRAY_MIDPLANE_RP_COND,

    FBE_ENCL_MGMT_SHUTDOWN_DEBOUNCE_TIMER_COND,

    FBE_ENCL_MGMT_LIFECYCLE_COND_LAST /* must be last */
} 
fbe_encl_mgmt_lifecycle_cond_id_t;

typedef enum fbe_encl_mgmt_drive_count_per_bank_e
{
    FBE_ENCL_MGMT_DRIVE_COUNT_PER_BANK_NA = 0,
    FBE_ENCL_MGMT_DRIVE_COUNT_PER_BANK_VOYAGER = 12,
    FBE_ENCL_MGMT_DRIVE_COUNT_PER_BANK_VIKING = 20,
    FBE_ENCL_MGMT_DRIVE_COUNT_PER_BANK_CAYENNE = 12,
    FBE_ENCL_MGMT_DRIVE_COUNT_PER_BANK_NAGA = 20,
} fbe_encl_mgmt_drive_count_per_bank_t;


typedef void * fbe_encl_mgmt_cmi_packet_t;

typedef struct fbe_encl_mgmt_user_modified_wwn_seed_flag_cmi_packet_s
{
    fbe_base_environment_cmi_message_t   baseCmiMsg;    //fbe_base_environment_cmi_message_opcode_t msgType;
    fbe_bool_t                           user_modified_wwn_seed_flag;
} fbe_encl_mgmt_user_modified_wwn_seed_flag_cmi_packet_t;

/*!****************************************************************************
 *    
 * @struct fbe_eir_sample_data_s
 *  
 * @brief 
 *   This is the definition of the encl info. This structure
 *   stores information about an enclosure's EIR samples.
 ******************************************************************************/
typedef struct fbe_eir_sample_data_s
{
    fbe_device_physical_location_t location;
    fbe_eir_input_power_sample_t   inputPowerSamples[FBE_SAMPLE_DATA_SIZE]; 
    fbe_eir_temp_sample_t          airInletTempSamples[FBE_SAMPLE_DATA_SIZE]; 
} fbe_eir_sample_data_t;

/*!****************************************************************************
 *    
 * @struct fbe_encl_mgmt_eir_samples_s
 *  
 * @brief 
 *   This is the definition of the encl info. This structure
 *   stores information about all the enclosures' EIR samples.
 ******************************************************************************/
typedef struct fbe_encl_mgmt_eir_samples_s
{
    fbe_u32_t               enclCount;
    fbe_u32_t               sampleCount;
    fbe_u32_t               sampleIndex;
    fbe_eir_sample_data_t   enclEirSamples[FBE_ESP_MAX_ENCL_COUNT];
} fbe_encl_mgmt_eir_samples_t;


typedef struct fbe_sub_encl_info_s {
    fbe_object_id_t                     objectId; // Component object id in the physical package.
    fbe_device_physical_location_t      location; // The component id in location will be populated.
    fbe_u32_t                           driveSlotCount; 
    fbe_lcc_info_t                      lccInfo[FBE_ESP_MAX_LCCS_PER_SUBENCL];
    fbe_base_env_fup_persistent_info_t  lccFupInfo[FBE_ESP_MAX_LCCS_PER_SUBENCL];
}fbe_sub_encl_info_t;

typedef struct fbe_encl_mgmt_system_id_info_s{
    fbe_char_t      product_part_number[RESUME_PROM_PRODUCT_PN_SIZE+1];
    fbe_char_t      product_serial_number[RESUME_PROM_PRODUCT_SN_SIZE+1];
    fbe_char_t      product_revision[RESUME_PROM_PRODUCT_REV_SIZE+1];
} fbe_encl_mgmt_system_id_info_t;

typedef struct fbe_encl_mgmt_persistent_info_s{
    fbe_encl_mgmt_system_id_info_t system_id_info;
    fbe_char_t      pad[2];
} fbe_encl_mgmt_persistent_info_t;

typedef struct fbe_encl_mgmt_modify_system_id_info_s{
    fbe_bool_t    changeSerialNumber;
    fbe_char_t    serialNumber[RESUME_PROM_PRODUCT_SN_SIZE];
    fbe_bool_t    changePartNumber;
    fbe_char_t    partNumber[RESUME_PROM_PRODUCT_PN_SIZE];
    fbe_bool_t    changeRev;
    fbe_char_t    revision[RESUME_PROM_PRODUCT_REV_SIZE];
}fbe_encl_mgmt_modify_system_id_info_t;

typedef struct fbe_encl_mgmt_user_modified_sys_id_flag_cmi_packet_s
{
    fbe_base_environment_cmi_message_t     baseCmiMsg;    //fbe_base_environment_cmi_message_opcode_t msgType;
    fbe_bool_t                             user_modified_sys_id_flag;
} fbe_encl_mgmt_user_modified_sys_id_flag_cmi_packet_t;

typedef struct fbe_encl_mgmt_persistent_sys_id_change_cmi_packet_s
{
    fbe_base_environment_cmi_message_t     baseCmiMsg;    //fbe_base_environment_cmi_message_opcode_t msgType;
    fbe_encl_mgmt_modify_system_id_info_t  persistent_sys_id_change_info;
} fbe_encl_mgmt_persistent_sys_id_change_cmi_packet_t;

typedef struct fbe_encl_mgmt_peer_encl_info_s{
    fbe_device_physical_location_t     location;
    fbe_u8_t                           serial_number[RESUME_PROM_EMC_SUB_ASSY_SERIAL_NUM_SIZE];
} fbe_encl_mgmt_encl_info_t;

typedef struct fbe_encl_mgmt_encl_info_cmi_packet_s
{
    fbe_base_environment_cmi_message_t baseCmiMsg;
    fbe_encl_mgmt_encl_info_t     encl_info;
} fbe_encl_mgmt_encl_info_cmi_packet_t;

/*!****************************************************************************
 *    
 * @struct fbe_encl_info_s
 *  
 * @brief 
 *   This is the definition of the encl info. This structure
 *   stores information about the enclosure
 ******************************************************************************/
typedef struct fbe_encl_info_s {
    fbe_object_id_t object_id;   // Source object id. For DPE, this is the enclosure object id not board object id.
    fbe_esp_encl_state_t enclState;
    fbe_esp_encl_fault_sym_t  enclFaultSymptom;
    fbe_bool_t     processorEncl;  // Is processor enclosure or not?
    fbe_spinlock_t encl_info_lock;
    fbe_esp_encl_type_t enclType;
    fbe_device_physical_location_t location;
    fbe_u8_t serial_number[RESUME_PROM_EMC_SUB_ASSY_SERIAL_NUM_SIZE];
    fbe_base_env_resume_prom_info_t     enclResumeInfo;
    fbe_u32_t                           lccCount;
    fbe_lcc_info_t                      lccInfo[FBE_ESP_MAX_LCCS_PER_ENCL];
    fbe_base_env_fup_persistent_info_t  lccFupInfo[FBE_ESP_MAX_LCCS_PER_ENCL];
    fbe_base_env_resume_prom_info_t     lccResumeInfo[FBE_ESP_MAX_LCCS_PER_ENCL];
    fbe_connector_info_t                connectorInfo[FBE_ESP_MAX_CONNECTORS_PER_ENCL];
    fbe_u8_t                            sscCount;
    fbe_ssc_info_t                      sscInfo;
    fbe_u8_t                            driveMidplaneCount; 
    fbe_base_env_resume_prom_info_t     driveMidplaneResumeInfo;
    fbe_base_env_resume_prom_info_t     sscResumeInfo;

    fbe_u32_t                           subEnclCount;   // the maximum number of subEnclosures.
    fbe_u32_t                           currSubEnclCount; // the number of subEnclosures which are up.
    fbe_u32_t                           subEnclLccStartSlot;
    fbe_sub_encl_info_t                 subEnclInfo[FBE_ESP_MAX_SUBENCLS_PER_ENCL];
    fbe_u32_t                           psCount;
    fbe_u32_t                           fanCount;
    fbe_u32_t                           driveSlotCount;
    fbe_u32_t                           driveSlotCountPerBank;
    fbe_u32_t                           connectorCount;
    fbe_u32_t                           bbuCount;
    fbe_u32_t                           spsCount;
    fbe_u32_t                           spCount;
    fbe_u32_t                           dimmCount;
    fbe_u32_t                           ssdCount;

    fbe_u32_t                           shutdownReason;
    fbe_bool_t                          overTempWarning;
    fbe_bool_t                          overTempFailure;
    fbe_bool_t                          shutdownReasonDebounceInProgress;
    fbe_bool_t                          enclShutdownDelayInProgress;
    fbe_bool_t                          shutdownReasonDebounceComplete;
    fbe_time_t                          debounceStartTime;
    fbe_u32_t                           maxEnclSpeed;
    fbe_u32_t                           currentEnclSpeed;
    fbe_led_status_t                    enclFaultLedStatus;
    fbe_encl_fault_led_reason_t         enclFaultLedReason;
    fbe_bool_t                          crossCable;

    fbe_esp_encl_eir_data_t             eir_data;
    fbe_eir_sample_data_t               *eirSampleData;
    fbe_encl_display_info_t             display;
    fbe_led_status_t                    enclDriveFaultLeds[FBE_ENCLOSURE_MAX_NUMBER_OF_DRIVE_SLOTS];
}fbe_encl_info_t;


/*!****************************************************************************
 *    
 * @struct fbe_encl_mgmt_s
 *  
 * @brief 
 *   This is the definition of the encl mgmt object. This object
 *   deals with handling enclosure related functions
 ******************************************************************************/
typedef struct fbe_encl_mgmt_s {
    /*! This must be first.  This is the base object we inherit from.
     */
    fbe_base_environment_t base_environment;
    fbe_encl_info_t *encl_info[FBE_ESP_MAX_ENCL_COUNT];
    fbe_u32_t total_encl_count;
    fbe_u32_t total_drive_slot_count;
    fbe_u32_t platformFruLimit;

    fbe_spinlock_t              encl_mgmt_lock;     

    fbe_common_cache_status_t   enclCacheStatus;
    fbe_u32_t                   eirSampleCount;
    fbe_u32_t                   eirSampleIndex;

    fbe_bool_t                  system_id_info_initialized;
    fbe_encl_mgmt_system_id_info_t     system_id_info;
    fbe_encl_mgmt_modify_system_id_info_t modify_rp_sys_id_info;

    fbe_bool_t                  ignoreUnsupportedEnclosures;

    /* Lifecycle defines. */
    FBE_LIFECYCLE_DEF_INST_DATA;
    FBE_LIFECYCLE_DEF_INST_COND(FBE_LIFECYCLE_COND_MAX(FBE_ENCL_MGMT_LIFECYCLE_COND_LAST));
} fbe_encl_mgmt_t;

fbe_status_t fbe_encl_mgmt_create_object(fbe_packet_t * packet, fbe_object_handle_t * object_handle);
fbe_status_t fbe_encl_mgmt_destroy_object( fbe_object_handle_t object_handle);
fbe_status_t fbe_encl_mgmt_control_entry(fbe_object_handle_t object_handle, fbe_packet_t * packet);
fbe_status_t fbe_encl_mgmt_monitor_entry(fbe_object_handle_t object_handle, fbe_packet_t * packet);
fbe_status_t fbe_encl_mgmt_event_entry(fbe_object_handle_t object_handle, fbe_event_type_t event_type, fbe_event_context_t event_context);

fbe_status_t fbe_encl_mgmt_monitor_load_verify(void);

fbe_status_t fbe_encl_mgmt_init(fbe_encl_mgmt_t * encl_mgmt);
fbe_status_t fbe_encl_mgmt_init_encl_info(fbe_encl_mgmt_t *encl_mgmt, fbe_encl_info_t *encl_info);
fbe_status_t fbe_encl_mgmt_init_subencl_info(fbe_sub_encl_info_t * pSubEnclInfo);
fbe_status_t fbe_encl_mgmt_init_all_encl_info(fbe_encl_mgmt_t * encl_mgmt);
fbe_status_t fbe_encl_mgmt_get_lcc_info_ptr(fbe_encl_mgmt_t * pEnclMgmt,
                                            fbe_device_physical_location_t * pLocation,
                                            fbe_lcc_info_t ** pLccInfoPtr);
/* fbe_encl_mgmt_monitor.c */
fbe_status_t fbe_encl_mgmt_get_encl_info_by_location(fbe_encl_mgmt_t * pEnclMgmt,
                                                     fbe_device_physical_location_t * pLocation,
                                                     fbe_encl_info_t ** pEnclInfoPtr);

fbe_status_t fbe_encl_mgmt_update_encl_fault_led(fbe_encl_mgmt_t *pEnclMgmt,
                                    fbe_device_physical_location_t *pLocation,
                                    fbe_encl_fault_led_reason_t enclFaultLedReason);

fbe_status_t fbe_encl_mgmt_process_lcc_status(fbe_encl_mgmt_t * pEnclMgmt,
                               fbe_object_id_t objectId,
                               fbe_device_physical_location_t * pLocation);

fbe_status_t fbe_encl_mgmt_process_subencl_all_lcc_status(fbe_encl_mgmt_t * pEnclMgmt,
                                             fbe_u32_t subEnclLccStartSlot,
                                             fbe_object_id_t subEnclObjectId,
                                             fbe_device_physical_location_t * pSubEnclLocation);

/* fbe_encl_mgmt_fup.c */
fbe_status_t fbe_encl_mgmt_fup_generate_logheader(fbe_device_physical_location_t * pLocation, 
                                                         fbe_u8_t * pLogHeader, 
                                                         fbe_u32_t logHeaderSize);

fbe_status_t fbe_encl_mgmt_fup_handle_lcc_status_change(fbe_encl_mgmt_t * pEnclMgmt,
                                                 fbe_device_physical_location_t * pLocation,
                                                 fbe_lcc_info_t * pNewLccInfo,
                                                 fbe_lcc_info_t * pOldLccInfo);

fbe_status_t fbe_encl_mgmt_fup_initiate_upgrade(void * pContext, 
                                              fbe_u64_t deviceType, 
                                              fbe_device_physical_location_t * pLocation,
                                              fbe_u32_t forceFlags,
                                              fbe_u32_t upgradeRetryCount);

fbe_status_t fbe_encl_mgmt_fup_get_full_image_path(void * pContext,                               
                               fbe_base_env_fup_work_item_t * pWorkItem);

fbe_status_t fbe_encl_mgmt_fup_get_manifest_file_full_path(void * pContext,
                                                           fbe_char_t * pManifestFileFullPath);

fbe_status_t fbe_encl_mgmt_fup_check_env_status(void * pContext, 
                                 fbe_base_env_fup_work_item_t * pWorkItem);

fbe_status_t fbe_encl_mgmt_fup_get_firmware_rev(void * pContext,
                          fbe_base_env_fup_work_item_t * pWorkItem,
                          fbe_u8_t * pFirmwareRev);

fbe_status_t fbe_encl_mgmt_get_fup_info_ptr(void * pContext,
                                            fbe_u64_t deviceType,
                                            fbe_device_physical_location_t * pLocation,
                                            fbe_base_env_fup_persistent_info_t ** pLccFupInfoPtr);

fbe_status_t fbe_encl_mgmt_get_fup_info(void * pContext,
                                            fbe_u64_t deviceType,
                                            fbe_device_physical_location_t * pLocation,
                                            fbe_esp_base_env_get_fup_info_t *  pGetFupInfo);

fbe_status_t fbe_encl_mgmt_fup_lcc_priority_check(void *pContext,
                                                  fbe_base_env_fup_work_item_t *pWorkItem,
                                                  fbe_bool_t *pWait);

fbe_status_t fbe_encl_mgmt_fup_resume_upgrade(void * pContext);

void fbe_encl_mgmt_clearInputPowerSample(fbe_eir_input_power_sample_t *samplePtr);

fbe_status_t fbe_encl_mgmt_fup_new_contact_init_upgrade(fbe_encl_mgmt_t * pEnclMgmt);

fbe_status_t fbe_encl_mgmt_fup_copy_fw_rev_to_resume_prom(fbe_encl_mgmt_t *pEnclMgmt,
                                                        fbe_u64_t deviceType,
                                                        fbe_device_physical_location_t *pLocation);

fbe_status_t fbe_encl_mgmt_fup_is_activate_in_progress(fbe_encl_mgmt_t * pEnclMgmt,
                                         fbe_u64_t deviceType,
                                         fbe_device_physical_location_t * pLocation,
                                         fbe_bool_t * pActivateInProgress);

fbe_status_t fbe_encl_mgmt_fup_refresh_device_status(void * pContext,
                                             fbe_base_env_fup_work_item_t * pWorkItem);


/* Resume Prom */
fbe_status_t fbe_encl_mgmt_initiate_resume_prom_read(void * pContext, 
                                                     fbe_u64_t deviceType, 
                                                     fbe_device_physical_location_t * pLocation);

fbe_status_t fbe_encl_mgmt_resume_prom_handle_lcc_status_change(fbe_encl_mgmt_t * pEnclMgmt, 
                                                                fbe_device_physical_location_t * pLocation, 
                                                                fbe_lcc_info_t * pNewLccInfo, 
                                                                fbe_lcc_info_t * pOldLccInfo);

fbe_status_t fbe_encl_mgmt_resume_prom_handle_ssc_status_change(fbe_encl_mgmt_t *pEnclMgmt, 
                                                                fbe_device_physical_location_t *pLocation, 
                                                                fbe_ssc_info_t *pNewSscInfo, 
                                                                fbe_ssc_info_t *pOldSScInfo);

fbe_status_t fbe_encl_mgmt_find_available_encl_info(fbe_encl_mgmt_t * pEnclMgmt, 
                                                    fbe_encl_info_t ** pEnclInfoPtr);

fbe_status_t fbe_encl_mgmt_resume_prom_handle_encl_status_change(fbe_encl_mgmt_t * pEnclMgmt, 
                                                                 fbe_device_physical_location_t * pLocation);

fbe_status_t fbe_encl_mgmt_get_resume_prom_info_ptr(fbe_encl_mgmt_t * pEnclMgmt,
                                                  fbe_u64_t deviceType,
                                                  fbe_device_physical_location_t * pLocation,
                                                  fbe_base_env_resume_prom_info_t ** pResumePromInfoPtr);

fbe_status_t fbe_encl_mgmt_resume_prom_update_encl_fault_led(fbe_encl_mgmt_t *pEnclMgmt,
                                                             fbe_u64_t deviceType,
                                                             fbe_device_physical_location_t *pLocation);

fbe_status_t fbe_encl_mgmt_reg_set_user_modified_wwn_seed_info(fbe_encl_mgmt_t *pEnclMgmt,
                                        fbe_bool_t user_modified_wwn_seed_flag);

fbe_status_t fbe_encl_mgmt_reg_get_user_modified_wwn_seed_info(fbe_encl_mgmt_t *pEnclMgmt,
                                                fbe_bool_t *user_modified_wwn_seed_flag);

fbe_status_t fbe_encl_mgmt_set_user_modified_system_info(fbe_encl_mgmt_t *pEnclMgmt,
                                        fbe_bool_t user_modified_system_info);

fbe_status_t fbe_encl_mgmt_get_user_modified_system_info(fbe_encl_mgmt_t *pEnclMgmt,
                                                fbe_bool_t *user_modified_system_info);

fbe_status_t fbe_encl_mgmt_reg_set_user_modified_system_id_flag(fbe_encl_mgmt_t *pEnclMgmt,
                                        fbe_bool_t user_modified_system_id_flag);

fbe_status_t fbe_encl_mgmt_reg_get_user_modified_system_id_flag(fbe_encl_mgmt_t *pEnclMgmt,
                                                fbe_bool_t *user_modified_system_id_flag);

fbe_bool_t fbe_encl_mgmt_validate_serial_number(fbe_u8_t * serial_number);

fbe_bool_t fbe_encl_mgmt_validate_part_number(fbe_u8_t * part_number);

fbe_bool_t fbe_encl_mgmt_validate_rev(fbe_u8_t * part_number);

fbe_status_t fbe_encl_mgmt_set_resume_prom_system_id_info(fbe_encl_mgmt_t * encl_mgmt,
                                            fbe_device_physical_location_t pe_location,
                                            fbe_encl_mgmt_modify_system_id_info_t *modify_sys_info);

fbe_status_t fbe_encl_mgmt_set_resume_prom_system_id_info_async(fbe_encl_mgmt_t * encl_mgmt,
                                            fbe_device_physical_location_t pe_location,
                                            fbe_encl_mgmt_modify_system_id_info_t *modify_sys_info);

fbe_status_t fbe_encl_mgmt_set_persistent_system_id_info(fbe_encl_mgmt_t * encl_mgmt,
                                                         fbe_encl_mgmt_modify_system_id_info_t *modify_sys_info);

void fbe_encl_mgmt_load_persistent_data(fbe_encl_mgmt_t *encl_mgmt);

fbe_status_t fbe_encl_mgmt_resume_prom_write(fbe_encl_mgmt_t * encl_mgmt,
                                             fbe_device_physical_location_t location,
                                             fbe_u8_t * buffer,
                                             fbe_u32_t buffer_size,
                                             RESUME_PROM_FIELD field,
                                             fbe_u64_t device_type,
                                             fbe_bool_t issue_read);

fbe_encl_info_t* fbe_encl_mgmt_get_pe_info(fbe_encl_mgmt_t * pEnclMgmt);

void fbe_encl_mgmt_string_check_and_copy(fbe_u8_t *dest, fbe_u8_t *source, fbe_u32_t len, fbe_bool_t *copied);

BOOL fbe_encl_mgmt_isEnclosureSupported(fbe_encl_mgmt_t * pEnclMgmt,
                                        SPID_PLATFORM_INFO *platformInfo,
                                        fbe_esp_encl_type_t enclosureType);

fbe_status_t fbe_encl_mgmt_reg_set_system_id_sn(fbe_encl_mgmt_t *pEnclMgmt,
                                        fbe_char_t *system_id_sn);

fbe_status_t fbe_encl_mgmt_reg_get_system_id_sn(fbe_encl_mgmt_t *pEnclMgmt,
                                                fbe_char_t *system_id_sn);

fbe_status_t fbe_encl_mgmt_reg_set_system_id_pn(fbe_encl_mgmt_t *pEnclMgmt,
                                        fbe_char_t *system_id_pn);

fbe_status_t fbe_encl_mgmt_reg_get_system_id_pn(fbe_encl_mgmt_t *pEnclMgmt,
                                                fbe_char_t *system_id_pn);

fbe_status_t fbe_encl_mgmt_reg_set_system_id_rev(fbe_encl_mgmt_t *pEnclMgmt,
                                        fbe_char_t *system_id_rev);

fbe_status_t fbe_encl_mgmt_reg_get_system_id_rev(fbe_encl_mgmt_t *pEnclMgmt,
                                                fbe_char_t *system_id_rev);

fbe_status_t fbe_encl_mgmt_process_user_modified_sys_id_flag_change(fbe_encl_mgmt_t * pEnclMgmt,
                                                                    fbe_encl_mgmt_cmi_packet_t *pEnclCmiPkt);

fbe_status_t fbe_encl_mgmt_process_persistent_sys_id_change(fbe_encl_mgmt_t * pEnclMgmt,
                                                                    fbe_encl_mgmt_cmi_packet_t *pEnclCmiPkt);

fbe_status_t fbe_encl_mgmt_process_encl_cabling_request(fbe_encl_mgmt_t * pEnclMgmt,
                                                  fbe_encl_mgmt_encl_info_t * peerEnclInfo);

fbe_status_t fbe_encl_mgmt_process_encl_cabling_ack(fbe_encl_mgmt_t * pEnclMgmt,
                                                  fbe_encl_mgmt_encl_info_t * peerEnclInfo);

fbe_status_t fbe_encl_mgmt_process_encl_cabling_nack(fbe_encl_mgmt_t * pEnclMgmt,
                                                  fbe_encl_mgmt_encl_info_t * peerEnclInfo);
fbe_status_t 
fbe_encl_mgmt_resume_prom_write_async_completion_function(fbe_packet_completion_context_t pContext, 
                                                          void * rpWriteAsynchOp);

/* fbe_encl_mgmt_kernel_main.c and fbe_encl_mgmt_sim_main.c*/
fbe_status_t fbe_encl_mgmt_fup_build_image_file_name(fbe_encl_mgmt_t * pEnclMgmt,
                                                     fbe_base_env_fup_work_item_t * pWorkItem,
                                                     fbe_u8_t * pImageFileNameBuffer,
                                                     char * pImageFileNameConstantPortion,
                                                     fbe_u8_t bufferLen,
                                                     fbe_u32_t * pImageFileNameLen);
#if 0
fbe_status_t fbe_encl_mgmt_fup_build_manifest_file_name(fbe_encl_mgmt_t * pEnclMgmt,
                                                        fbe_u8_t * pManifestFileNameBuffer,
                                                        fbe_char_t * pManifestFileNameConstantPortion,
                                                        fbe_u8_t bufferLen,
                                                        fbe_u32_t * pManifestFileNameLen);
#endif
#endif /* ENCL_MGMT_PRIVATE_H */

/*******************************
 * end fbe_encl_mgmt_private.h
 *******************************/
