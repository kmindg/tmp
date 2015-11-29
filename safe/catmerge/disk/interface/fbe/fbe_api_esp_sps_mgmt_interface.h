#ifndef FBE_API_ESP_SPS_MGMT_INTERFACE_H
#define FBE_API_ESP_SPS_MGMT_INTERFACE_H
/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_api_esp_sps_mgmt_interface.h
 ***************************************************************************
 *
 * @brief
 *  This header file defines the FBE API for the ESP SPS Mgmt object.
 * 
 * @ingroup fbe_api_esp_interface_class_files
 * 
 * @version
 *   03/16/2010:  Created. Joe Perry
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_types.h"
#include "fbe/fbe_transport.h"
#include "fbe/fbe_esp_sps_mgmt.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_board.h"
#include "fbe/fbe_sps_info.h"
#include "fbe/fbe_eir_info.h"


FBE_API_CPP_EXPORT_START

//----------------------------------------------------------------
// Define the top level group for the FBE Storage Extent Package APIs
//----------------------------------------------------------------
/*! @defgroup fbe_api_esp_interface_class FBE ESP APIs
 *  @brief 
 *    This is the set of definitions for FBE ESP APIs.
 *
 *  @ingroup fbe_api_esp_interface
 */ 
//----------------------------------------------------------------

//----------------------------------------------------------------
// Define the FBE API ESP SPS Mgmt Interface for the Usurper Interface. 
// This is where all the data structures defined. 
//----------------------------------------------------------------
/*! @defgroup fbe_api_esp_sps_mgmt_interface_usurper_interface FBE API ESP SPS Mgmt Interface Usurper Interface
 *  @brief 
 *    This is the set of definitions that comprise the FBE API ESP SPS Mgmt Interface class
 *    usurper interface.
 *
 *  @ingroup fbe_api_classes_usurper_interface
 *  @{
 */ 
//----------------------------------------------------------------

// FBE_ESP_SPS_MGMT_CONTROL_CODE_GET_SPS_STATUS

/*!********************************************************************* 
 * @struct fbe_esp_sps_mgmt_spsStatus_t 
 *  
 * @brief 
 *   Contains the sps status.
 *
 * @ingroup fbe_api_esp_sps_mgmt_interface
 * @ingroup fbe_esp_sps_mgmt_spsStatus
 **********************************************************************/
typedef struct
{
    fbe_device_physical_location_t  spsLocation;    /*!< Input - location of SPS */
    fbe_bool_t                  dualComponentSps;   /*!< SPS Dual Component */
    fbe_bool_t                  spsModuleInserted;  /*!< SPS Module Inserted */
    fbe_bool_t                  spsBatteryInserted; /*!< SPS Battery Inserted */
    SPS_STATE                   spsStatus;          /*!< SPS Status */
    fbe_sps_fault_info_t        spsFaultInfo;       /*!< SPS Fault Information */
    fbe_sps_cabling_status_t    spsCablingStatus;   /*!< SPS Cabling Status */
    SPS_TYPE                    spsModel;           /*!< SPS Model Type */
    HW_MODULE_TYPE              spsModuleFfid;      /*!< SPS Module FFID */
    HW_MODULE_TYPE              spsBatteryFfid;     /*!< SPS Battery FFID */
    fbe_u8_t                    primaryFirmwareRev[FBE_ENCLOSURE_MAX_FIRMWARE_REV_SIZE + 1];
    fbe_u8_t                    secondaryFirmwareRev[FBE_ENCLOSURE_MAX_FIRMWARE_REV_SIZE + 1];
    fbe_u8_t                    batteryFirmwareRev[FBE_ENCLOSURE_MAX_FIRMWARE_REV_SIZE + 1];
} fbe_esp_sps_mgmt_spsStatus_t;


// FBE_ESP_SPS_MGMT_CONTROL_CODE_GET_SPS_COUNT

/*!********************************************************************* 
 * @struct fbe_esp_sps_mgmt_spsCounts_t 
 *  
 * @brief 
 *   Contains the sps status.
 *
 * @ingroup fbe_api_esp_sps_mgmt_interface
 * @ingroup fbe_esp_sps_mgmt_spsCounts
 **********************************************************************/
typedef struct
{
    fbe_u32_t                   totalNumberOfSps;
    fbe_u32_t                   enclosuresWithSps;
    fbe_u32_t                   spsPerEnclosure;
} fbe_esp_sps_mgmt_spsCounts_t;

// FBE_ESP_SPS_MGMT_CONTROL_CODE_GET_SPS_TEST_TIME
// FBE_ESP_SPS_MGMT_CONTROL_CODE_SET_SPS_TEST_TIME

/*!********************************************************************* 
 * @struct fbe_esp_sps_mgmt_spsTestTime_t 
 *  
 * @brief 
 *   Contains the SPS Test time.
 *
 * @ingroup fbe_api_esp_sps_mgmt_interface
 * @ingroup fbe_esp_sps_mgmt_spsTestTime
 **********************************************************************/
typedef struct
{
    fbe_system_time_t   spsTestTime;    /*!< SPS Test Time */
} fbe_esp_sps_mgmt_spsTestTime_t;

//----------------------------------------------------------------

// FBE_ESP_SPS_MGMT_CONTROL_CODE_GET_SPS_INPUT_POWER_TOTAL
/*!********************************************************************* 
 * @struct fbe_esp_sps_mgmt_spsInputPowerTotal_t 
 *  
 * @brief 
 *   Contains the sps total input power stats.
 *
 * @ingroup fbe_api_esp_sps_mgmt_interface
 * @ingroup fbe_esp_sps_mgmt_spsInputPower
 **********************************************************************/
typedef struct fbe_esp_sps_mgmt_spsInputPowerTotal_s
{
    fbe_u32_t                       spsCount;
    fbe_eir_input_power_sample_t    inputPowerAll;
} fbe_esp_sps_mgmt_spsInputPowerTotal_t;

// FBE_ESP_SPS_MGMT_CONTROL_CODE_GET_SPS_INPUT_POWER
/*!********************************************************************* 
 * @struct fbe_esp_sps_mgmt_spsInputPower_t 
 *  
 * @brief 
 *   Contains the sps input power stats.
 *
 * @ingroup fbe_api_esp_sps_mgmt_interface
 * @ingroup fbe_esp_sps_mgmt_spsInputPower
 **********************************************************************/
typedef struct
{
    fbe_device_physical_location_t  spsLocation;    /*!< Input - location of SPS */
    fbe_eir_input_power_sample_t    spsCurrentInputPower;    /*!< Current SPS Input Power */
    fbe_eir_input_power_sample_t    spsAverageInputPower;    /*!< Average SPS Input Power */
    fbe_eir_input_power_sample_t    spsMaxInputPower;        /*!< Maximum SPS Input Power */
} fbe_esp_sps_mgmt_spsInputPower_t;

typedef struct
{
    fbe_device_physical_location_t  spsLocation;    /*!< Input - location of SPS */
    fbe_eir_input_power_sample_t    spsInputPowerSamples[FBE_SAMPLE_DATA_SIZE];
} fbe_esp_sps_mgmt_spsInputPowerSample_t;

//----------------------------------------------------------------

// FBE_ESP_SPS_MGMT_CONTROL_CODE_GET_SPS_MANUF_INFO

/*!********************************************************************* 
 * @struct fbe_esp_sps_mgmt_spsManufInfo_t 
 *  
 * @brief 
 *   Contains information about the sps manufacturer.
 *
 * @ingroup fbe_api_esp_sps_mgmt_interface
 * @ingroup fbe_esp_sps_mgmt_spsManufInfo
 **********************************************************************/
typedef struct
{
    fbe_device_physical_location_t  spsLocation;    /*!< Input - location of SPS */
    fbe_sps_manuf_info_t            spsManufInfo;
} fbe_esp_sps_mgmt_spsManufInfo_t;

// FBE_ESP_SPS_MGMT_CONTROL_CODE_GET_BOB_STATUS

/*!********************************************************************* 
 * @struct fbe_esp_sps_mgmt_bobStatus_t 
 *  
 * @brief 
 *   Contains the bob status.
 *
 * @ingroup fbe_api_esp_sps_mgmt_interface
 * @ingroup fbe_esp_sps_mgmt_bobStatus
 **********************************************************************/
typedef struct
{
    fbe_u8_t                    bobIndex;           /*!< BOB Index (0-A side, 1-B side) */
    fbe_base_battery_info_t     bobInfo;
    fbe_bool_t                  dataStale;
    fbe_bool_t                  environmentalInterfaceFault;
    fbe_bool_t                  resumePromReadFailed;
    fbe_bool_t                  fupFailure;
} fbe_esp_sps_mgmt_bobStatus_t;


typedef struct
{
    fbe_bool_t                          spsTestTimeDetected;
    fbe_bool_t                          needToTest;
    fbe_bool_t                          testingCompleted;
    fbe_sps_test_type_t                 spsTestType;
    fbe_time_t                          spsTestStartTime;
    fbe_common_cache_status_t           spsCacheStatus;
    fbe_bool_t                          mfgInfoNeeded[FBE_SPS_MGMT_ENCL_MAX];
    fbe_bool_t                          simulateSpsInfo;
    fbe_u32_t                           spsBatteryOnlineCount;
    SPS_STATE                           debouncedSpsStatus[FBE_SPS_MGMT_ENCL_MAX];
    fbe_u32_t                           debouncedSpsStatusCount[FBE_SPS_MGMT_ENCL_MAX];
    fbe_sps_fault_info_t                debouncedSpsFaultInfo[FBE_SPS_MGMT_ENCL_MAX];
    fbe_u32_t                           debouncedSpsFaultsCount[FBE_SPS_MGMT_ENCL_MAX];
} fbe_sps_mgmt_privateSpsData_t;

typedef struct
{
    fbe_bool_t                          needToTest;
    fbe_bool_t                          needToTestPerBbu[FBE_BOB_MAX_COUNT];
    fbe_bool_t                          testingCompleted;
    fbe_bool_t                          simulateBobInfo;
    fbe_bool_t                          debouncedBobOnBatteryState[FBE_BOB_MAX_COUNT];
    fbe_u32_t                           debouncedBobOnBatteryStateCount[FBE_BOB_MAX_COUNT];
    fbe_time_t                          notBatteryReadyTimeStart[FBE_BOB_MAX_COUNT];
} fbe_sps_mgmt_privateBbuData_t;

typedef union
{
    fbe_sps_mgmt_privateSpsData_t   privateSpsData;
    fbe_sps_mgmt_privateBbuData_t   privateBbuData;
} fbe_sps_mgmt_privateData_t;

/*!********************************************************************* 
 * @struct fbe_esp_sps_mgmt_objectData_t 
 *  
 * @brief 
 *   Contains sps_mgmt object data.
 *
 * @ingroup fbe_api_esp_sps_mgmt_interface
 * @ingroup fbe_esp_sps_mgmt_objectData
 **********************************************************************/
typedef struct
{
    fbe_sps_testing_state_t     testingState;
    fbe_time_t                  testStartTime;
    fbe_sps_mgmt_privateData_t  privateObjectData;
} fbe_esp_sps_mgmt_objectData_t;

/*! @} */ /* end of group fbe_api_esp_sps_mgmt_interface_usurper_interface */

//----------------------------------------------------------------
// Define the group for the FBE API ESP SPS Mgmt Interface.  
// This is where all the function prototypes for the FBE API ESP SPS Mgmt.
//----------------------------------------------------------------
/*! @defgroup fbe_api_esp_sps_mgmt_interface FBE API ESP SPS Mgmt Interface
 *  @brief 
 *    This is the set of FBE API ESP SPS Mgmt Interface. 
 *
 *  @details 
 *    In order to access this library, please include fbe_api_esp_sps_mgmt_interface.h.
 *
 *  @ingroup fbe_api_esp_interface_class
 *  @{
 */
//----------------------------------------------------------------

fbe_status_t FBE_API_CALL 
fbe_api_esp_sps_mgmt_getSpsCount(fbe_esp_sps_mgmt_spsCounts_t *spsCountInfo);
fbe_status_t FBE_API_CALL 
fbe_api_esp_sps_mgmt_getBobCount(fbe_u32_t *bobCount);
fbe_status_t FBE_API_CALL 
fbe_api_esp_sps_mgmt_getSpsStatus(fbe_esp_sps_mgmt_spsStatus_t *spsStatusInfo);
fbe_status_t FBE_API_CALL 
fbe_api_esp_sps_mgmt_getBobStatus(fbe_esp_sps_mgmt_bobStatus_t *bobStatusInfo);
fbe_status_t FBE_API_CALL 
fbe_api_esp_sps_mgmt_getObjectData(fbe_esp_sps_mgmt_objectData_t *spsMgmtObjectData);
fbe_status_t FBE_API_CALL 
fbe_api_esp_sps_mgmt_getSpsTestTime(fbe_esp_sps_mgmt_spsTestTime_t *spsTestTimeInfo);
fbe_status_t FBE_API_CALL 
fbe_api_esp_sps_mgmt_setSpsTestTime(fbe_esp_sps_mgmt_spsTestTime_t *spsTestTimeInfo);
fbe_status_t FBE_API_CALL 
fbe_api_esp_sps_mgmt_getSpsInputPower(fbe_esp_sps_mgmt_spsInputPower_t *spsInputPowerInfo);
fbe_status_t FBE_API_CALL 
fbe_api_esp_sps_mgmt_getManufInfo(fbe_esp_sps_mgmt_spsManufInfo_t *spsManufInfo);
fbe_status_t FBE_API_CALL 
fbe_api_esp_sps_mgmt_getCacheStatus(fbe_common_cache_status_t *cacheStatus);
fbe_status_t FBE_API_CALL fbe_api_esp_sps_mgmt_powerdown(void);
fbe_status_t FBE_API_CALL fbe_api_esp_sps_mgmt_verifyPowerdown(void);
fbe_status_t FBE_API_CALL fbe_api_esp_sps_mgmt_getSimulateSps(fbe_bool_t *simulateSps);
fbe_status_t FBE_API_CALL fbe_api_esp_sps_mgmt_setSimulateSps(fbe_bool_t simulateSps);
fbe_status_t FBE_API_CALL
fbe_api_esp_sps_mgmt_getSpsInputPowerSample(fbe_esp_sps_mgmt_spsInputPowerSample_t *spsSampleInfo);
fbe_status_t FBE_API_CALL
fbe_api_esp_sps_mgmt_getLocalBatteryTime(fbe_u32_t *localBatteryTime);
fbe_status_t FBE_API_CALL fbe_api_esp_sps_mgmt_initiateSelfTest(void);


/*! @} */ /* end of group fbe_api_esp_sps_mgmt_interface */

FBE_API_CPP_EXPORT_END

//----------------------------------------------------------------
// Define the group for all FBE ESP Package APIs Interface class files.  
// This is where all the class files that belong to the FBE API ESP 
// Package define. In addition, this group is displayed in the FBE Classes
// module.
//----------------------------------------------------------------
/*! @defgroup fbe_api_esp_interface_class_files FBE ESP APIs Interface Class Files 
 *  @brief 
 *    This is the set of files for the FBE ESP Interface.
 *
 *  @ingroup fbe_api_classes
 */
//----------------------------------------------------------------


#endif /* FBE_API_ESP_SPS_MGMT_INTERFACE_H */

/*************************
 * end file fbe_api_esp_sps_mgmt_interface.h
 *************************/
