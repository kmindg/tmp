/***************************************************************************
 * Copyright (C) EMC Corporation 2010 
 * All rights reserved. Licensed material -- property of EMC 
 * Corporation 
 ***************************************************************************/

#ifndef FBE_SPS_INFO_H
#define FBE_SPS_INFO_H

/*!**************************************************************************
 * @file fbe_sps_info.h
 ***************************************************************************
 *
 * @brief
 *  This file SPS defines & data structures.
 * 
 * @ingroup 
 * 
 * @revision
 *   03/24/2010:  Created. Joe Perry
 *
 ***************************************************************************/

#include "fbe/fbe_types.h"
#include "fbe_time.h"
#include "fbe/fbe_eir_info.h"
#include "specl_types.h"
#include "fbe/fbe_devices.h"
#include "fbe/fbe_enclosure_interface.h"
#include "fbe/fbe_pe_types.h"


#define FBE_SPS_MAX_COUNT       2
#define FBE_LOCAL_SPS           0
#define FBE_PEER_SPS            1
#define FBE_SPS_ARRAY_MAX_COUNT 4

#define FBE_BOB_MAX_COUNT       6
#define FBE_SPA_BOB             0
#define FBE_SPB_BOB             1

typedef enum
{
    FBE_BACKUP_TYPE_SPS = 0,
    FBE_BACKUP_TYPE_BOB,
    FBE_BACKUP_TYPE_BBU,
} fbe_backup_type_t;

typedef enum
{
    FBE_SPS_MGMT_PE = 0,
    FBE_SPS_MGMT_DAE0,
    FBE_SPS_MGMT_ENCL_MAX,
    FBE_SPS_MGMT_ALL = 0xFF           // all SPS's (PE & DAE0)
} fbe_sps_ps_encl_num_t;

typedef enum fbe_sps_test_type_e
{
	FBE_SPS_CABLING_TEST_TYPE,				// short test to verify cabling
	FBE_SPS_BATTERY_TEST_TYPE,				// long test to verify battery charge
} fbe_sps_test_type_t;

typedef enum fbe_sps_testing_state_e
{
    FBE_SPS_NO_TESTING_IN_PROGRESS = 0,
    FBE_SPS_LOCAL_SPS_TESTING,
    FBE_SPS_PEER_SPS_TESTING,
    FBE_SPS_WAIT_FOR_PEER_PERMISSION,
    FBE_SPS_WAIT_FOR_PEER_TEST,
    FBE_SPS_WAIT_FOR_STABLE_ARRAY,
} fbe_sps_testing_state_t;

/*
 * SPS reported Faults
 */
typedef struct
{
    fbe_bool_t      spsModuleFault;
    fbe_bool_t      spsBatteryFault;
    fbe_bool_t      spsInternalFault;
    fbe_bool_t      spsChargerFailure;
    fbe_bool_t      spsBatteryEOL;
    fbe_bool_t      spsBatteryNotEngagedFault;
    fbe_bool_t      spsCableConnectivityFault;
} fbe_sps_fault_info_t;

typedef enum
{
    FBE_SPS_FAULT_NONE = 0,
    FBE_SPS_FAULT_CHARGER_FAILURE,
    FBE_SPS_FAULT_INTERNAL_FAULT,
    FBE_SPS_FAULT_BATTERY_EOL,
    FBE_SPS_FAULT_MULTIPLE_FAULTS,
    FBE_SPS_FAULT_UNKNOWN,
    FBE_SPS_FAULT_NO_CHANGE,                // used by debug commands
} fbe_sps_fault_t;

/*
 * SPS/PS Cabling Status (determine during SPS Testing)
 */
typedef enum
{
    FBE_SPS_CABLING_UNKNOWN = 0,
    FBE_SPS_CABLING_VALID,              // SPS Testing not run yet
    FBE_SPS_CABLING_INVALID_PE,         // Processor Enclosure is not properly cabled to SPS
    FBE_SPS_CABLING_INVALID_DAE0,       // DAE0 (Vault drives) is not properly cabled to SPS
    FBE_SPS_CABLING_INVALID_SERIAL,     // Serial cable is connected to the wrong SPS
    FBE_SPS_CABLING_INVALID_MULTI,      // Multiple cables are incorrect (check all of them)
} fbe_sps_cabling_status_t;

typedef enum
{
    FBE_SPS_MODE_NORMAL = 0,
    FBE_SPS_MODE_ENHANCED,
} fbe_sps_mode_t;

// FBE_BASE_BOARD_CONTROL_CODE_GET_SPS_MANUF_INFO
#define FBE_SPS_SERIAL_NUM_REVSION_SIZE         16
#define FBE_SPS_PART_NUM_REVSION_SIZE           16
#define FBE_SPS_PART_NUM_REVSION_SIZE           16
#define FBE_SPS_VENDOR_NAME_SIZE                32
#define FBE_SPS_VENDOR_PART_NUMBER_SIZE         32
#define FBE_SPS_FW_REVISION_SIZE                8
#define FBE_SPS_MODEL_ID_QUERY_SIZE             8
#define FBE_SPS_MODEL_STRING_SIZE               12

typedef struct fbe_sps_manuf_record_s{
    fbe_bool_t  spsManufValid;
    fbe_u8_t    spsSerialNumber[FBE_SPS_SERIAL_NUM_REVSION_SIZE];
    fbe_u8_t    spsPartNumber[FBE_SPS_PART_NUM_REVSION_SIZE];
    fbe_u8_t    spsPartNumRevision[FBE_SPS_PART_NUM_REVSION_SIZE];
    fbe_u8_t    spsVendor[FBE_SPS_VENDOR_NAME_SIZE];
    fbe_u8_t    spsVendorModelNumber[FBE_SPS_VENDOR_PART_NUMBER_SIZE];
    fbe_u8_t    spsFirmwareRevision[FBE_SPS_FW_REVISION_SIZE];
    fbe_u8_t    spsSecondaryFirmwareRevision[FBE_SPS_FW_REVISION_SIZE];
    fbe_u8_t    spsModelString[FBE_SPS_MODEL_STRING_SIZE];
}fbe_sps_manuf_record_t;

typedef struct fbe_sps_manuf_info_s{
    fbe_sps_manuf_record_t  spsModuleManufInfo;
    fbe_sps_manuf_record_t  spsBatteryManufInfo;
} fbe_sps_manuf_info_t;

/*!****************************************************************************
 *    
 * @struct fbe_base_sps_info_s
 *  
 * @brief 
 *   This is the definition of the SPS info. This structure
 *   stores information about the SPS
 ******************************************************************************/
typedef struct fbe_base_sps_info_s 
{
    fbe_env_inferface_status_t          envInterfaceStatus;
    SPECL_ERROR                         transactionStatus;
    // data available from Board Object (don't duplicate, try to remove)
    fbe_bool_t                          spsModulePresent;
    fbe_bool_t                          spsBatteryPresent;
    SPS_STATE                           spsStatus;
    fbe_sps_fault_info_t                spsFaultInfo;
    SPS_TYPE                            spsModel;
    fbe_u32_t                           spsBatteryTime;
    HW_MODULE_TYPE                      spsModuleFfid;
    HW_MODULE_TYPE                      spsBatteryFfid;
    fbe_bool_t                          lccFwActivationInProgress;
//    SPECL_SPS_RESUME                    spsManufInfo;
    fbe_eir_input_power_sample_t        spsInputPower;
    fbe_bool_t                          bSpsModuleDownloadable;
    fbe_bool_t                          bSpsBatteryDownloadable;
    fbe_u32_t                           programmableCount;
    fbe_u8_t                            primaryFirmwareRev[FBE_ENCLOSURE_MAX_FIRMWARE_REV_SIZE + 1];
    fbe_u8_t                            secondaryFirmwareRev[FBE_ENCLOSURE_MAX_FIRMWARE_REV_SIZE + 1];
    fbe_u8_t                            batteryFirmwareRev[FBE_ENCLOSURE_MAX_FIRMWARE_REV_SIZE + 1];
}fbe_base_sps_info_t;

/*
 * SPS Hardware Definitions 
 */
// SPS Status - bit meanings
#define FBE_HW_SPS_STATUS_ON_BATTERY               0x0001       // State
#define FBE_HW_SPS_STATUS_BATTERY_EOL              0x0004       // Fault
#define FBE_HW_SPS_STATUS_CHARGER_FAILURE          0x0008       // Fault
#define FBE_HW_SPS_STATUS_LATCHED_STOP             0x0020
#define FBE_HW_SPS_STATUS_TESTING                  0x0080       // State
#define FBE_HW_SPS_STATUS_INTERNAL_FAULT           0x0100       // Fault
#define FBE_HW_SPS_STATUS_BATTERY_AVAILABLE        0x0200       // State
#define FBE_HW_SPS_STATUS_ENHANCED_MODE            0x0400
#define FBE_HW_SPS_STATUS_CABLE_CONNECTIVITY       0x0800
#define FBE_HW_SPS_STATUS_BATTERY_NOT_ENGAGED      0x1000

// SPS Commands
#define FBE_HW_SPS_BATTEST_CMD          "BATTEST"
#define FBE_HW_SPS_BATTEST_CMD_LEN      7
#define FBE_HW_SPS_STOP_CMD             "STOP"
#define FBE_HW_SPS_STOP_CMD_LEN         4

typedef enum fbe_sps_action_type_e {
    FBE_SPS_ACTION_NONE = 0,
    FBE_SPS_ACTION_START_TEST,
    FBE_SPS_ACTION_STOP_TEST,
    FBE_SPS_ACTION_SHUTDOWN,
    FBE_SPS_ACTION_SET_SPS_PRESENT,
    FBE_SPS_ACTION_CLEAR_SPS_PRESENT,
    FBE_SPS_ACTION_ENABLE_SPS_FAST_SWITCHOVER,
    FBE_SPS_ACTION_DISABLE_SPS_FAST_SWITCHOVER,
} fbe_sps_action_type_t;

/*
 * Battery (BoB or BBU) Info
 */
typedef enum
{
    FBE_BATTERY_STATUS_UNKNOWN = 0,
    FBE_BATTERY_STATUS_BATTERY_READY,
    FBE_BATTERY_STATUS_ON_BATTERY,
    FBE_BATTERY_STATUS_CHARGING,
    FBE_BATTERY_STATUS_FULL_CHARGED,
    FBE_BATTERY_STATUS_REMOVED,
    FBE_BATTERY_STATUS_TESTING,
} fbe_battery_status_t;

typedef enum
{
    FBE_BATTERY_FAULT_NO_FAULT = 0,
    FBE_BATTERY_FAULT_INTERNAL_FAILURE,
    FBE_BATTERY_FAULT_COMM_FAILURE,
    FBE_BATTERY_FAULT_LOW_CHARGE,
    FBE_BATTERY_FAULT_TEST_FAILED,
    FBE_BATTERY_FAULT_CANNOT_CHARGE,
    FBE_BATTERY_FAULT_NOT_READY,
    FBE_BATTERY_FAULT_FLT_STATUS_REG,
} fbe_battery_fault_t;

typedef enum
{
    FBE_BATTERY_TEST_STATUS_NONE = 0,
    FBE_BATTERY_TEST_STATUS_STARTED,
    FBE_BATTERY_TEST_STATUS_COMPLETED,
    FBE_BATTERY_TEST_STATUS_FAILED,
    FBE_BATTERY_TEST_STATUS_INSUFFICIENT_LOAD,
} fbe_battery_test_status_t;

// BBU Power Requirements (Jetfire)
#define FBE_JETFIRE_BBU_ENERGY_INIT_VAULE       3600        // units in 1/1000th wattHours
#define FBE_JETFIRE_BBU_MAX_LOAD_INIT_VAULE     3600        // units in tenths of watts
// BBU Power Requirements (Beachcomber)
#define FBE_BEACHCOMBER_BBU_POWER_INIT_VAULE     750        // units in 0.1 watts
#define FBE_BEACHCOMBER_BBU_TIME_INIT_VAULE       85        // units in seconds
// BBU Power Requirements (Oberon)
#define FBE_OBERON_BBU_ENERGY_INIT_VAULE         700        // units in 1/1000th wattHours
#define FBE_OBERON_BBU_MAX_LOAD_INIT_VAULE      1600        // units in tenths of watts
// BBU Power Requirements (Hyperion)
#define FBE_HYPERION_BBU_ENERGY_INIT_VAULE      3000        // units in 1/1000th wattHours
#define FBE_HYPERION_BBU_MAX_LOAD_INIT_VAULE    4100        // units in tenths of watts
// BBU Power Requirements (Triton)
#define FBE_TRITON_BBU_ENERGY_INIT_VAULE         700        // units in 1/1000th wattHours
#define FBE_TRITON_BBU_MAX_LOAD_INIT_VAULE      1600        // units in tenths of watts

typedef struct fbe_battery_energy_requirement_s
{
    fbe_u16_t                           energy;                 /* For Copilot, in units of 0.01 WHr */
    fbe_u16_t                           maxLoad;                /* For Copilot, in units of 0.1 Watts */
    fbe_u16_t                           power;                  /* For Beachcomber BBU, in units of 0.1 Watts */
    fbe_u16_t                           time;                   /* For Beachcomber BBU, in units of seconds */
}fbe_battery_energy_requirement_t;


typedef enum
{
    FBE_BATTERY_STATE_UNKNOWN = 0,
    FBE_BATTERY_STATE_OK,
    FBE_BATTERY_STATE_DEGRADED,
    FBE_BATTERY_STATE_REMOVED,
    FBE_BATTERY_STATE_FAULTED,
} fbe_battery_state_t;

typedef enum
{
    FBE_BATTERY_SUBSTATE_NO_FAULT = 0,
    FBE_BATTERY_SUBSTATE_GEN_FAULT,
    FBE_BATTERY_SUBSTATE_FAN_FAULT,
    FBE_BATTERY_SUBSTATE_UNSUPPORTED,
    FBE_BATTERY_SUBSTATE_FLT_STATUS_REG_FAULT,
    FBE_BATTERY_SUBSTATE_RP_READ_FAILURE,
    FBE_BATTERY_SUBSTATE_ENV_INTF_FAILURE,
} fbe_battery_subState_t;

/*!****************************************************************************
 *    
 * @struct fbe_base_battery_info_s
 *  
 * @brief 
 *   This is the definition of the Battery info. This structure
 *   stores information about the Battery
 ******************************************************************************/
typedef struct fbe_base_battery_info_s 
{
    fbe_battery_state_t                 state;
    fbe_battery_subState_t              subState;
    fbe_env_inferface_status_t          envInterfaceStatus;
    SPECL_ERROR                         transactionStatus;    
    fbe_bool_t                          resumePromReadFailed;
    fbe_bool_t                          fupFailure;
    // data available from Board Object (don't duplicate, try to remove)
    fbe_bool_t                          batteryInserted;
    fbe_bool_t                          batteryEnabled;
    fbe_bool_t                          onBattery;
    fbe_bool_t                          batteryReady;
    fbe_battery_status_t                batteryState;
    BATTERY_CHARGE_STATE                batteryChargeState;
    fbe_battery_fault_t                 batteryFaults;
    fbe_battery_test_status_t           batteryTestStatus;
    fbe_u16_t                           batteryStorageCapacity;
    fbe_u16_t                           batteryDeliverableCapacity;
    HW_MODULE_TYPE                      batteryFfid;
    SP_ID                               associatedSp;
    fbe_u32_t                           slotNumOnSpBlade;
    fbe_bool_t                          hasResumeProm;
    fbe_u8_t                            firmwareRevMajor;
    fbe_u8_t                            firmwareRevMinor;
    fbe_battery_energy_requirement_t    energyRequirement; // keep current value for battery
    BATTERY_ENERGY_REQUIREMENTS         defaultEnergyRequirement; // keep value we want to set for battery
    fbe_bool_t                          setEnergyRequirementSupported; // whether this Battery support set energy requirement
    /* Only for BOB/BBU which on processor enclosure */
    fbe_bool_t                          isFaultRegFail;
}fbe_base_battery_info_t;

//Function Prototypes For some common functions for SPS object.
const char *fbe_sps_mgmt_getSpsFaultString (fbe_sps_fault_info_t *spsFaultInfo);
const char *fbe_sps_mgmt_getSpsStatusString (SPS_STATE spsStatus);
const char *fbe_sps_mgmt_getSpsCablingStatusString (fbe_sps_cabling_status_t spsCablingStatus);
const char *fbe_sps_mgmt_getSpsString (fbe_device_physical_location_t *spsLocation);
const char *fbe_sps_mgmt_getSpsModelString (SPS_TYPE spsModelType);
const char *fbe_sps_mgmt_getSpsActionTypeString (fbe_sps_action_type_t spsActionType);
const char *fbe_sps_mgmt_getBobStateString (fbe_battery_status_t bobStatus);
const char *fbe_sps_mgmt_getBobFaultString (fbe_battery_fault_t bobFault);
const char *fbe_sps_mgmt_getBobTestStatusString (fbe_battery_test_status_t testStatus);
const char *fbe_sps_mgmt_decode_bbu_state(fbe_battery_state_t testStatus);
const char *fbe_sps_mgmt_decode_bbu_subState(fbe_battery_subState_t testStatus);
const char *fbe_getWeekDayString (fbe_u16_t   weekday);
void fbe_getTimeString (fbe_system_time_t * time, fbe_s8_t * timeString);

#endif /* FBE_SPS_INFO_H */

/*******************************
 * end fbe_sps_info.h
 *******************************/
