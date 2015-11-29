/***************************************************************************
 * Copyright (C) EMC Corporation 2007-2010 
 * All rights reserved. Licensed material -- property of EMC 
 * Corporation 
 ***************************************************************************/

#ifndef SPS_MGMT_PRIVATE_H
#define SPS_MGMT_PRIVATE_H

/*!**************************************************************************
 * @file fbe_sps_mgmt_private.h
 ***************************************************************************
 *
 * @brief
 *  This file contains private defines for the SPS
 *  Management object. 
 * 
 * @ingroup sps_mgmt_class_files
 * 
 * @revision
 *   03/02/2010:  Created. Joe Perry
 *
 ***************************************************************************/

#include "fbe_base_object.h"
#include "fbe/fbe_board.h"
#include "fbe/fbe_sps_info.h"
#include "fbe/fbe_eir_info.h"
#include "fbe/fbe_time.h"
#include "base_object_private.h"
#include "fbe_base_environment_private.h"
#include "fbe/fbe_esp_sps_mgmt.h"

#define FBE_SPS_MGMT_CDES_FUP_IMAGE_PATH_KEY   "CDES11PowerSupplyImagePath"
#define FBE_SPS_MGMT_PRI_CDES_FUP_IMAGE_FILE_NAME  "CDES_SPS_FW_"
#define FBE_SPS_MGMT_SEC_CDES_FUP_IMAGE_FILE_NAME  "CDES_SPS2_FW_"
#define FBE_SPS_MGMT_BAT_CDES_FUP_IMAGE_FILE_NAME  "CDES_SPS_BBU_FW_"

#define FBE_SPS_MGMT_FUP_INTER_DEVICE_DELAY       15      // 15 seconds
                                                                    //
#define FBE_SPS_MGMT_MAX_BBU_CHARGE_TIME        (60 * 60 * 16)      // 16 hours

/* Lifecycle definitions
 * Define the SPS management lifecycle's instance.
 */
extern fbe_lifecycle_const_t FBE_LIFECYCLE_CONST_DATA(sps_mgmt);

/* These are the lifecycle condition ids for SPS
   Management object.*/

/*! @enum fbe_sps_mgmt_lifecycle_cond_id_t  
 *  
 *  @brief These are the lifecycle conditions for the SPS
 *  management object.
                                                               */
typedef enum fbe_sps_mgmt_lifecycle_cond_id_e 
{
    /*! Processing of new SPS
     */
    FBE_SPS_MGMT_DISCOVER_SPS = FBE_LIFECYCLE_COND_FIRST_ID(FBE_CLASS_ID_SPS_MGMT),
    FBE_SPS_MGMT_DISCOVER_PS,
    FBE_SPS_MGMT_INIT_TEST_TIME_COND,
    FBE_SPS_MGMT_CHECK_TEST_TIME_COND,
    FBE_SPS_MGMT_SPS_TEST_CHECK_NEEDED_COND,
    FBE_SPS_MGMT_CHECK_STOP_TEST_COND,
    FBE_SPS_MGMT_DEBOUNCE_TIMER_COND,
    FBE_SPS_MGMT_HUNG_SPS_TEST_TIMER_COND,
    FBE_SPS_MGMT_TEST_NEEDED_COND,
    FBE_SPS_MGMT_SPS_MFG_INFO_NEEDED_TIMER_COND,
    FBE_SPS_MGMT_PROCESS_SPS_STATUS_CHANGE_COND,
    
    FBE_SPS_MGMT_LIFECYCLE_COND_LAST /* must be last */
} 
fbe_sps_mgmt_lifecycle_cond_id_t;


#define FBE_SPS_MGMT_TWO_SPS_COUNT      2
#define FBE_SPS_MGMT_FOUR_SPS_COUNT     4
#define FBE_SPS_MGMT_SPS_PER_ENCL       2

#define FBE_SPS_MGMT_SPS_STRING_LEN     10
#define FBE_SPS_MGMT_BOB_STRING_LEN     10


typedef enum fbe_sps_test_config_e
{
    FBE_SPS_TEST_CONFIG_UNKNOWN = 0,
    FBE_SPS_TEST_CONFIG_DPE = 1,
    FBE_SPS_TEST_CONFIG_XPE = 2,
    FBE_SPS_TEST_CONFIG_XPE_VOYAGER = 3,
    FBE_SPS_TEST_CONFIG_XPE_VIKING = 4,
} fbe_sps_test_config_t;

/*!****************************************************************************
 *    
 * @struct fbe_sps_mgmt_eir_data_s
 *  
 * @brief 
 *   This is the definition of Energy Info Reporting (EIR) data needed by
 *   sps_mgmt object.
 ******************************************************************************/
typedef struct fbe_sps_mgmt_eir_data_s
{
    fbe_u32_t                       spsSampleCount;
    fbe_u32_t                       spsSampleIndex;
    fbe_eir_input_power_sample_t    spsCurrentInputPower[FBE_SPS_MGMT_ENCL_MAX][FBE_SPS_MAX_COUNT];
    fbe_eir_input_power_sample_t    spsMaxInputPower[FBE_SPS_MGMT_ENCL_MAX][FBE_SPS_MAX_COUNT];
    fbe_eir_input_power_sample_t    spsAverageInputPower[FBE_SPS_MGMT_ENCL_MAX][FBE_SPS_MAX_COUNT];
    fbe_eir_input_power_sample_t    spsInputPowerSamples[FBE_SPS_MGMT_ENCL_MAX][FBE_SPS_MAX_COUNT][FBE_SAMPLE_DATA_SIZE];
} fbe_sps_mgmt_eir_data_t;

/*!****************************************************************************
 *    
 * @enum fbe_sps_supported_status_e
 *  
 * @brief 
 *   This is an enumeration of SPS supported status (certain platforms 
 *   require specific SPS types).
 ******************************************************************************/
typedef enum fbe_sps_supported_status_e {
    FBE_SPS_SUPP_NO = 0,
    FBE_SPS_SUPP_YES,
    FBE_SPS_SUPP_NOT_REC,
    FBE_SPS_SUPP_UNKNOWN,
} fbe_sps_supported_status_t;

/*
typedef struct fbe_sps_ac_fail_status_s
{
    fbe_bool_t      acFailPe;
    fbe_bool_t      acFailDae0[2];          // Voyager will have two AC_FAIL's to check
} fbe_sps_ac_fail_status_t;
*/


/*!****************************************************************************
 *    
 * @struct fbe_sps_info_s
 *  
 * @brief 
 *   This is the definition of the SPS info. This structure
 *   stores information about the SPS
 ******************************************************************************/
typedef struct fbe_sps_info_s 
{
    fbe_env_inferface_status_t          envInterfaceStatus;
    SPECL_ERROR                         transactionStatus;
    // data available from Board Object (don't duplicate, try to remove)
    fbe_bool_t                          dualComponentSps;           // SPS has a module & battery
    fbe_bool_t                          spsModulePresent;
    fbe_bool_t                          spsBatteryPresent;
    fbe_bool_t                          bSpsModuleDownloadable;
    fbe_bool_t                          bSpsBatteryDownloadable;
    fbe_u32_t                           programmableCount;
    fbe_u8_t                            primaryFirmwareRev[FBE_ENCLOSURE_MAX_FIRMWARE_REV_SIZE + 1];
    fbe_u8_t                            secondaryFirmwareRev[FBE_ENCLOSURE_MAX_FIRMWARE_REV_SIZE + 1];
    fbe_u8_t                            batteryFirmwareRev[FBE_ENCLOSURE_MAX_FIRMWARE_REV_SIZE + 1];
    SPS_STATE                           spsStatus;
    fbe_sps_fault_info_t                spsFaultInfo;
    SPS_TYPE                            spsModel;
    fbe_sps_supported_status_t          spsSupportedStatus;
    HW_MODULE_TYPE                      spsModuleFfid;
    HW_MODULE_TYPE                      spsBatteryFfid;
    fbe_sps_manuf_info_t                spsManufInfo;
    fbe_eir_input_power_sample_t        spsInputPower;
    fbe_u32_t                           spsBatteryTime;
    fbe_char_t                          spsNameString[FBE_SPS_MGMT_SPS_STRING_LEN];
    fbe_sps_cabling_status_t            spsCablingStatus;
    fbe_bool_t                          needToTest;
    fbe_bool_t                          testInProgress;
    fbe_bool_t                          lccFwActivationInProgress;
}fbe_sps_info_t;

/*!****************************************************************************
 *    
 * @struct fbe_sps_ac_fail_info_s
 *  
 * @brief 
 *   This is the definition of the SPS AC_FAIL info. This structure
 *   stores information about the SPS AC_FAIL info.
 ******************************************************************************/
typedef struct fbe_sps_ac_fail_info_s
{
    fbe_bool_t      acFailDetected;
    fbe_bool_t      acFailExpected;
    fbe_bool_t      acFailOpposite;
} fbe_sps_ac_fail_info_t;

/*!****************************************************************************
 *    
 * @struct fbe_array_sps_info_s
 *  
 * @brief 
 *   This is the definition of the arrays SPS info. This structure
 *   stores information about the SPS
 ******************************************************************************/
typedef struct fbe_array_sps_info_s 
{
    fbe_object_id_t                     spsObjectId[FBE_SPS_MGMT_ENCL_MAX];
    fbe_sps_info_t                      sps_info[FBE_SPS_MGMT_ENCL_MAX][FBE_SPS_MAX_COUNT];
    fbe_sps_test_config_t               spsTestConfig;
    fbe_ps_info_t                       ps_info[FBE_SPS_MGMT_ENCL_MAX][FBE_ESP_PS_MAX_COUNT_PER_ENCL];
    fbe_u32_t                           psEnclCounts[FBE_SPS_MGMT_ENCL_MAX];
    fbe_sps_ac_fail_info_t              spsAcFailInfo[FBE_SPS_MGMT_ENCL_MAX][FBE_ESP_PS_MAX_COUNT_PER_ENCL];
    fbe_bool_t                          spsTestTimeDetected;
    fbe_bool_t                          needToTest;
    fbe_bool_t                          testingCompleted;
    fbe_sps_test_type_t                 spsTestType;
    fbe_time_t                          spsTestStartTime;
    fbe_common_cache_status_t           spsCacheStatus;
    fbe_bool_t                          mfgInfoNeeded[FBE_SPS_MGMT_ENCL_MAX];
    fbe_bool_t                          simulateSpsInfo;
    fbe_u32_t                           spsBatteryOnlineCount;
    fbe_bool_t                          spsStatusDelayAfterLccFup;
    fbe_u32_t                           spsStatusDelayAfterLccFupCount;
    SPS_STATE                           debouncedSpsStatus[FBE_SPS_MGMT_ENCL_MAX];
    fbe_u32_t                           debouncedSpsStatusCount[FBE_SPS_MGMT_ENCL_MAX];
    fbe_sps_fault_info_t                debouncedSpsFaultInfo[FBE_SPS_MGMT_ENCL_MAX];
    fbe_u32_t                           debouncedSpsFaultsCount[FBE_SPS_MGMT_ENCL_MAX];
} fbe_array_sps_info_t;

typedef struct fbe_encl_sps_fup_info_s
{
    fbe_base_env_fup_persistent_info_t  fupPersistentInfo[FBE_SPS_COMPONENT_ID_MAX];
} fbe_sps_fup_info_t;

typedef struct fbe_array_sps_fup_info_s
{
    fbe_sps_fup_info_t      spsFupInfo[FBE_SPS_MGMT_ENCL_MAX][FBE_SPS_MAX_COUNT];
} fbe_array_sps_fup_info_t;

/*!****************************************************************************
 *    
 * @struct fbe_bob_info_s
 *  
 * @brief 
 *   This is the definition of the BOB info. This structure
 *   stores information about the BOB
 ******************************************************************************/
typedef struct fbe_bob_info_s 
{
    // data available from Board Object (don't duplicate, try to remove)
    fbe_bool_t                          bobPresent;
    BATTERY_CHARGE_STATE                bobState;
    fbe_battery_fault_t                 bobFaults;
    fbe_battery_test_status_t           bobTestStatus;
}fbe_bob_info_t;

/*!****************************************************************************
 *    
 * @struct fbe_array_bob_info_s
 *  
 * @brief 
 *   This is the definition of the arrays BOB info. This structure
 *   stores information about the BOB
 ******************************************************************************/
typedef struct fbe_array_bob_info_s 
{
    fbe_object_id_t                     bobObjectId;
    fbe_base_battery_info_t             bob_info[FBE_BOB_MAX_COUNT];
    fbe_char_t                          bobNameString[FBE_BOB_MAX_COUNT][FBE_SPS_MGMT_BOB_STRING_LEN];
    fbe_base_env_resume_prom_info_t     bobResumeInfo[FBE_BOB_MAX_COUNT];
    fbe_bool_t                          needToTest;
    fbe_bool_t                          testingCompleted;
    fbe_bool_t                          bobNeedsTesting[FBE_BOB_MAX_COUNT];
    fbe_bool_t                          simulateBobInfo;
    fbe_battery_status_t                debouncedBobOnBatteryState[FBE_BOB_MAX_COUNT];
    fbe_u32_t                           debouncedBobOnBatteryStateCount[FBE_BOB_MAX_COUNT];
    fbe_time_t                          notBatteryReadyTimeStart[FBE_BOB_MAX_COUNT];
    fbe_bool_t                          localDiskBatteryBackedSet;
    fbe_bool_t                          peerDiskBatteryBackedSet;
    // use SPS Test Time for BOB testing
} fbe_array_bob_info_t;

/*!****************************************************************************
 *    
 * @struct fbe_array_backup_info_u
 *  
 * @brief 
 *   This is the definition of the SPS mgmt object. This is a union of
 *   the backup info (either SPS or BoB).
 ******************************************************************************/
typedef union fbe_array_backup_info_u
{
    fbe_array_sps_info_t    arraySpsInfo;
    fbe_array_bob_info_t    arrayBobInfo;
} fbe_array_backup_info_t;

#define FBE_SPS_MGMT_PERSISTENT_STORAGE_REVISION    1
#define FBE_SPS_BBU_TEST_TIME_CANARY                0xFFEE2211
// default Test Time: Sunday at 1:00 AM
#define FBE_SPS_DEFAULT_TEST_TIME_HOUR  1
#define FBE_SPS_DEFAULT_TEST_TIME_MIN   0
#define FBE_SPS_DEFAULT_TEST_TIME_DAY   0

#define FBE_SPS_MGMT_BBU_COUNT_TBD      0xFF
typedef struct
{
    fbe_u32_t           revision;
    fbe_u32_t           testTimeValid;
    fbe_system_time_t   testStartTime;
} fbe_sps_bbu_test_info_t;

/*!****************************************************************************
 *    
 * @struct fbe_sps_mgmt_s
 *  
 * @brief 
 *   This is the definition of the SPS mgmt object. This object
 *   deals with handling SPS related functions
 ******************************************************************************/
typedef struct fbe_sps_mgmt_s {
    /*! This must be first.  This is the base object we inherit from.
     */
    fbe_base_environment_t  base_environment;

    fbe_object_id_t         object_id;                  // Board Object reports SPS info
    fbe_lifecycle_state_t   state;
    fbe_array_sps_info_t    *arraySpsInfo;
    fbe_array_bob_info_t    *arrayBobInfo;
    fbe_sps_mgmt_eir_data_t *spsEirData;
    fbe_u32_t               total_sps_count;
    fbe_u32_t               sps_per_side;
    fbe_u32_t               encls_with_sps;
    fbe_u32_t               total_bob_count;

    fbe_sps_testing_state_t testingState;

    fbe_time_t              testStartTime;

    fbe_array_sps_fup_info_t *arraySpsFupInfo;

    /* Lifecycle defines. */
    FBE_LIFECYCLE_DEF_INST_DATA;
    FBE_LIFECYCLE_DEF_INST_COND(FBE_LIFECYCLE_COND_MAX(FBE_SPS_MGMT_LIFECYCLE_COND_LAST));
} fbe_sps_mgmt_t;

/*!****************************************************************************
 *    
 * @struct fbe_sps_platform_support_s
 *  
 * @brief 
 *   This is structure with a platform type & SPS Supported Status per
 *   SPS model type.
 ******************************************************************************/
typedef struct fbe_sps_platform_support_s
{
    SPID_HW_TYPE                platformType;
    fbe_sps_supported_status_t  spsSupportedStatus[SPS_TYPE_MAX];
} fbe_sps_platform_support_t;

/*!****************************************************************************
 *    
 * @struct fbe_sps_mgmt_cmi_msg_s
 *  
 * @brief 
 *   This is the definition of the SPS mgmt object structure used for
 *   CMI (Peer-to-Peer) message communication.
 ******************************************************************************/
typedef struct fbe_sps_mgmt_cmi_msg_s 
{
    fbe_base_environment_cmi_message_t  msgType;
    fbe_u8_t                            spsCount;
    fbe_sps_info_t                      sendSpsInfo[FBE_SPS_MGMT_ENCL_MAX];
    fbe_bool_t                          testBothSps;
    fbe_battery_test_status_t           bbuTestStatus;
    fbe_bool_t                          diskBatteryBackedSet;
} fbe_sps_mgmt_cmi_msg_t;

typedef void * fbe_sps_mgmt_cmi_packet_t;

/*
 * sps_mgmt function prototypes
 */
fbe_status_t fbe_sps_mgmt_create_object(fbe_packet_t * packet, 
                                        fbe_object_handle_t * object_handle);
fbe_status_t fbe_sps_mgmt_destroy_object( fbe_object_handle_t object_handle);
fbe_status_t fbe_sps_mgmt_control_entry(fbe_object_handle_t object_handle, 
                                        fbe_packet_t * packet);
fbe_status_t fbe_sps_mgmt_monitor_entry(fbe_object_handle_t object_handle, 
                                        fbe_packet_t * packet);
fbe_status_t fbe_sps_mgmt_event_entry(fbe_object_handle_t object_handle, 
                                      fbe_event_type_t event_type, 
                                      fbe_event_context_t event_context);

fbe_status_t fbe_sps_mgmt_monitor_load_verify(void);

fbe_status_t fbe_sps_mgmt_init(fbe_sps_mgmt_t * sps_mgmt);
fbe_status_t fbe_sps_mgmt_sendSpsCommand(fbe_sps_mgmt_t *sps_mgmt,
                                         fbe_sps_action_type_t spsCommand,
                                         fbe_sps_ps_encl_num_t spsToSendTo);
fbe_status_t fbe_sps_mgmt_processSpsStatus(fbe_sps_mgmt_t *sps_mgmt, 
                                           fbe_sps_ps_encl_num_t spsEnclIndex,
                                           fbe_u8_t spsIndex);
fbe_status_t fbe_sps_mgmt_processBobStatus(fbe_sps_mgmt_t *sps_mgmt,
                                           fbe_device_physical_location_t *pLocation);
fbe_status_t fbe_sps_mgmt_get_sps_info_ptr(void * pContext,
                                            fbe_u64_t deviceType,
                                            fbe_device_physical_location_t * pLocation,
                                            fbe_sps_info_t ** pSpsInfoPtr);

fbe_status_t fbe_sps_mgmt_convertSpsLocation(fbe_sps_mgmt_t *spsMgmtPtr,
                                                    fbe_device_physical_location_t *location,
                                                    fbe_u32_t *spsEnclIndex,
                                                    fbe_u32_t *spsIndex);

fbe_status_t fbe_sps_mgmt_convertSpsIndex(fbe_sps_mgmt_t * pSpsMgmt,
                                          fbe_u32_t spsEnclIndex,
                                          fbe_u32_t spsIndex,
                                          fbe_device_physical_location_t * pLocation);

/* fbe_sps_mgmt_fup.c */
fbe_status_t fbe_sps_mgmt_fup_handle_sps_status_change(fbe_sps_mgmt_t *pSpsMgmt,
                                                 fbe_device_physical_location_t *pLocation,
                                                 fbe_sps_info_t *pSpsNewInfo,
                                                 fbe_sps_info_t *pSpsOldInfo);

fbe_status_t fbe_sps_mgmt_fup_initiate_upgrade(void * pContext, 
                                              fbe_u64_t deviceType, 
                                              fbe_device_physical_location_t * pLocation,
                                              fbe_u32_t forceFlags,
                                              fbe_u32_t upgradeRetryCount);

fbe_status_t fbe_sps_mgmt_fup_get_full_image_path(void * pContext,                               
                               fbe_base_env_fup_work_item_t * pWorkItem);

fbe_status_t fbe_sps_mgmt_fup_check_env_status(void * pContext, 
                                 fbe_base_env_fup_work_item_t * pWorkItem);

fbe_status_t fbe_sps_mgmt_fup_get_firmware_rev(void * pContext,
                                              fbe_base_env_fup_work_item_t * pWorkItem,
                                              fbe_u8_t * pFirmwareRev);

fbe_status_t fbe_sps_mgmt_get_component_fup_info_ptr(void * pContext,
                                            fbe_u64_t deviceType,
                                            fbe_device_physical_location_t * pLocation,
                                            fbe_base_env_fup_persistent_info_t ** pComponentFupInfoPtr);

fbe_status_t fbe_sps_mgmt_get_sps_fup_info_ptr(void * pContext,
                                            fbe_u64_t deviceType,
                                            fbe_device_physical_location_t * pLocation,
                                            fbe_sps_fup_info_t ** pSpsFupInfoPtr);

fbe_status_t fbe_sps_mgmt_get_fup_info(void * pContext,
                                            fbe_u64_t deviceType,
                                            fbe_device_physical_location_t * pLocation,
                                            fbe_esp_base_env_get_fup_info_t *  pGetFupInfo);

fbe_status_t fbe_sps_mgmt_fup_resume_upgrade(void * pContext);

fbe_status_t fbe_sps_mgmt_fup_refresh_device_status(void * pContext,
                                             fbe_base_env_fup_work_item_t * pWorkItem);

fbe_status_t fbe_sps_mgmt_fup_new_contact_init_upgrade(fbe_sps_mgmt_t *pSpsMgmt);

/* fbe_sps_mgmt_resume_prom.c */
fbe_status_t fbe_sps_mgmt_initiate_resume_prom_read(void * pContext, 
                                                        fbe_u64_t deviceType,
                                                        fbe_device_physical_location_t * pLocation);
fbe_status_t fbe_sps_mgmt_get_resume_prom_info_ptr(fbe_sps_mgmt_t *pSpsMgmt,
                                                       fbe_u64_t deviceType,
                                                       fbe_device_physical_location_t * pLocation,
                                                       fbe_base_env_resume_prom_info_t ** pResumePromInfoPtr);
fbe_status_t fbe_sps_mgmt_resume_prom_handle_bob_status_change(fbe_sps_mgmt_t * pSpsMgmt,
                                                               fbe_device_physical_location_t * pLocation,
                                                               fbe_base_battery_info_t * pNewBobInfo,
                                                               fbe_base_battery_info_t * pOldBobInfo);
fbe_status_t fbe_sps_mgmt_update_encl_fault_led(fbe_sps_mgmt_t *pSpsMgmt,
                                    fbe_device_physical_location_t *pLocation,
                                    fbe_encl_fault_led_reason_t enclFaultLedReason);
fbe_status_t fbe_sps_mgmt_resume_prom_update_encl_fault_led(fbe_sps_mgmt_t *pSpsMgmt,
                                                            fbe_u64_t deviceType,
                                                            fbe_device_physical_location_t *pLocation);
#endif /* SPS_MGMT_PRIVATE_H */

/*******************************
 * end fbe_sps_mgmt_private.h
 *******************************/
