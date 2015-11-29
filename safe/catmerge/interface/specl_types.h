#ifndef SPECL_TYPES_H
#define SPECL_TYPES_H

/***************************************************************************
 *  Copyright (C)  EMC Corporation 1992-2011
 *  All rights reserved.
 *  Licensed material - property of EMC Corporation.
 ***************************************************************************/

/***************************************************************************
 * specl_types.h
 ***************************************************************************
 *
 * File Description:
 *  Header file for all externally visible structures, used in conjunction
 *  with SPECL.
 *
 * Author:
 *  Phillip Leef
 *
 * Revision History:
 *  March 3, 2007 - PHL - Created Inital Version
 *
 ***************************************************************************/

#include "EmcPAL.h"
#include "generic_types.h"

#include "spid_types.h"
#include "resume_prom.h"
#include "spd_dimms_info.h"
#include "smbus_driver.h"
#include "gpio_signals.h"
#include "familyfruid.h"
#include "pciConfig.h"
#include "spm_api.h"
#include "diplex_FPGA_micro.h"
#include "MiniportAnalogInfo.h"
#include "SPS_micro.h"
#include "ipmi.h"
#include "sfp_support.h"

#include "EmcPAL_Generic.h"

#pragma pack(1)

/*********************
 * Exported Constants
 *********************/
#define SPECL_BASE_DEVICE_NAME      "Specl"
#define SPECL_NT_DEVICE_NAME        "\\Device\\" SPECL_BASE_DEVICE_NAME
#define SPECL_DOSDEVICES_NAME       "\\DosDevices\\" SPECL_BASE_DEVICE_NAME
#define SPECL_WIN32_DEVICE_NAME     "\\\\.\\" SPECL_BASE_DEVICE_NAME

typedef UINT_32 SPECL_ERROR;

/******************************************************************************
 *                      R E S U M E   P R O M   T Y P E S
 ******************************************************************************/

/********************
 * SPECL_RESUME_DATA
 ********************/
typedef struct _SPECL_RESUME_DATA
{
    BOOL                    sfiMaskStatus;
    SPECL_ERROR             transactionStatus;
    BOOL                    retriesExhausted;
    RESUME_PROM_STRUCTURE   resumePromData;
    SPECL_ERROR             lastWriteStatus;
    EMCPAL_LARGE_INTEGER    timeStamp;
} SPECL_RESUME_DATA, *PSPECL_RESUME_DATA;



/******************************************************************************
*                   P O W E R   S U P P L Y   T Y P E S
 ******************************************************************************/

/**************
 * AC_DC_INPUT
 **************/
typedef enum _AC_DC_INPUT
{
    INVALID_AC_DC_INPUT,
    AC_INPUT,
    DC_INPUT,
} AC_DC_INPUT;

/*******************
 * POWER_SUPPLY_IDS
 *******************/
typedef enum _POWER_SUPPLY_IDS
{
    PS0                 = 0,
    PS1                 = 1,
    TOTAL_PS_PER_BLADE  = 2,
} POWER_SUPPLY_IDS;

#if defined(__cplusplus)
/* use extern C++ here to trick other C++ files including this as extern C
 */
extern "C++"{
/* Define a postfix increment operator for _POWER_SUPPLY_IDS
 */
inline _POWER_SUPPLY_IDS operator++( POWER_SUPPLY_IDS &ps_id, int )
{
   return ps_id = (POWER_SUPPLY_IDS)((int)ps_id + 1);
}
}
#endif

// this define is for the maximum possible number of PS's for the array
#define TOTAL_POSSIBLE_PS_PER_ARRAY  (TOTAL_PS_PER_BLADE * SP_ID_MAX)

/******************
 * SPECL_PS_STATUS
 ******************/
typedef struct _SPECL_PS_STATUS
{
    SPECL_ERROR                     transactionStatus;
    HW_MODULE_TYPE                  uniqueID;
    UINT_16E                        fruCapability;
    BOOL                            uniqueIDFinal;
    POWER_SUPPLY_IDS                localPSID;
    POWER_SUPPLY_IDS                peerPSID;
    BOOL                            localSupply;
    BOOL                            inserted;
    BOOL                            generalFault;
    BOOL                            ambientOvertempFault;
    BOOL                            DCPresent;
    BOOL                            ACFail;
    AC_DC_INPUT                     ACDCInput;
    UCHAR                           tempSensor;
    BOOL                            energyStarCompliant;
    csx_s16_t                       inputPower;
    UCHAR                           type;
    union {
        struct {
            csx_s16_t               outputPower;
            UCHAR                   firmwareRevMajor;
            UCHAR                   firmwareRevMinor;
        } type3;//Transformers
        struct {
            UCHAR                   inputVoltage;
            BOOL                    internalFanFault;
            UCHAR                   firmwareRevMajor;
            UCHAR                   firmwareRevMinor;
        } type4;//Moons
    };
    EMCPAL_LARGE_INTEGER    timeStamp;
} SPECL_PS_STATUS, *PSPECL_PS_STATUS;

/****************************
 * SPECL_PS_STATUS_PER_BLADE
 ****************************/
typedef struct _SPECL_PS_STATUS_PER_BLADE
{
    ULONG               psCount;
    SPECL_PS_STATUS     psStatus[TOTAL_PS_PER_BLADE];
} SPECL_PS_STATUS_PER_BLADE, *PSPECL_PS_STATUS_PER_BLADE;

/*******************
 * SPECL_PS_SUMMARY
 *******************/
typedef struct _SPECL_PS_SUMMARY
{
    BOOL                            sfiMaskStatus;
    ULONG                           bladeCount;
    SPECL_PS_STATUS_PER_BLADE       psSummary[SP_ID_MAX];
} SPECL_PS_SUMMARY, *PSPECL_PS_SUMMARY;


/*********************
 * BIST_RESULT GENERIC
 ********************/
typedef enum _BMC_BIST_RESULT
{
    BMC_BIST_UNKNOWN    = 0x00,
    BMC_BIST_PASSED     = 0x01,
    BMC_BIST_FAILED     = 0x02,
    BMC_BIST_RUNNING    = 0x04,
    BMC_BIST_SKIPPED    = 0x05,
    BMC_BIST_INVALID    = 0xFF
} BMC_BIST_RESULT, *PBMC_BIST_RESULT;


/******************************************************************************
 *                  B A T T E R Y   B A C K   U P   T Y P E S
 ******************************************************************************/

#define MAX_BATTERY_COUNT_PER_BLADE     3

typedef enum _BATTERY_ID
{
    BATTERY_ID_0 = 0,
    BATTERY_ID_1 = 1,
    BATTERY_ID_2 = 2,
    BATTERY_MAX_ID = 3,
    BATTERY_INVALID_ID = 0xff,
} BATTERY_ID;

typedef enum _BATTERY_CHARGE_STATE
{
    BATTERY_CHARGING,
    BATTERY_DISCHARGING,
    BATTERY_FULLY_CHARGED,
    BATTERY_CHARGE_STATE_UNKNOWN,
} BATTERY_CHARGE_STATE;

typedef enum _BATTERY_ENERGY_REQUIREMENTS_FORMAT
{
    BATTERY_ENERGY_REQUIREMENTS_FORMAT_ENERGY_AND_POWER = 0x01,
    BATTERY_ENERGY_REQUIREMENTS_FORMAT_POWER_AND_TIME   = 0x02
} BATTERY_ENERGY_REQUIREMENTS_FORMAT;

typedef struct _BATTERY_ENERGY_REQUIREMENTS_ENERGY_AND_POWER
{
    USHORT energy;  // in units of 0.01 WHr
    USHORT power;   // in units of 0.1Watts
} BATTERY_ENERGY_REQUIREMENTS_ENERGY_AND_POWER;

typedef struct _BATTERY_ENERGY_REQUIREMENTS_POWER_AND_TIME
{
    USHORT power;   // in units of 0.1Watts
    USHORT time;    // in units of seconds
} BATTERY_ENERGY_REQUIREMENTS_POWER_AND_TIME;

typedef struct _BATTERY_ENERGY_REQUIREMENTS
{
    BATTERY_ENERGY_REQUIREMENTS_FORMAT format;
    BATTERY_ID batteryID;
    union
    {
        BATTERY_ENERGY_REQUIREMENTS_ENERGY_AND_POWER energyAndPower;
        BATTERY_ENERGY_REQUIREMENTS_POWER_AND_TIME   powerAndTime;
    } data;
} BATTERY_ENERGY_REQUIREMENTS;

/***********************
 * SPECL_BATTERY_STATUS
 ***********************/
typedef struct _SPECL_BATTERY_STATUS
{
    SPECL_ERROR             transactionStatus;
    BOOL                    inserted;
    HW_MODULE_TYPE          uniqueID;
    UINT_16E                fruCapability;
    BOOL                    uniqueIDFinal;
    BOOL                    enabled;
    BOOL                    onBattery;
    BOOL                    batteryReady;
    BOOL                    generalFault;
    BOOL                    internalFailure;
    BOOL                    lowCharge;
    BATTERY_CHARGE_STATE    chargeState;
    UCHAR                   type;
    union {
        struct {
            csx_s16_t       energyRequirement;      /* in units of 0.01 WHr */
            csx_s16_t       maxLoad;                /* in units of 0.1 Watts */
            csx_s16_t       deliverableCapacity;    /* in units of 0.01 WHr */
            csx_s16_t       storageCapacity;        /* in units of 0.01 WHr */
            csx_u16_t       dischargeCycleCount;
            csx_u16_t       serviceLife;            /* in units of days */
            csx_char_t      worstCaseTemperature;   /* in units of degrees C */
            LED_BLINK_RATE  faultLEDStatus;
            BMC_BIST_RESULT testState;
            UCHAR           testResult;
            COPILOT_FAULT_FLAGS_RAW faultFlags;
            BATTERY_REG_OFFSET_72_READING_RAW_INFO_B0_B15   extendedTestResult;
        } type1;//Jetfire BOB
        struct {
            csx_s16_t       dischargeRate;          /* BBU data, in units of 0.1 Watts */
            csx_s16_t       remainingTime;          /* BBU data, in units of seconds */
            csx_u16_t       dischargeCycleCount;
            csx_char_t      temperature;            /* in units of degrees C */
            COPILOT_FAULT_FLAGS_RAW faultFlags;
            UCHAR           faultStatus;
        } type2;//Beachcomber BBU
        struct {
            csx_s16_t       energyRequirement;      /* in units of 0.01 WHr */
            csx_s16_t       maxLoad;                /* in units of 0.1 Watts */
            LED_BLINK_RATE  faultLEDStatus;
            BMC_BIST_RESULT testState;
            csx_u32_t       testErrorCode;
        } type3;//Moons BOB Copilot2
        struct {
            csx_s16_t       energyRequirement;      /* in units of 0.01 WHr */
            csx_s16_t       maxLoad;                /* in units of 0.1 Watts */
            BMC_BIST_RESULT testState;
            csx_u32_t       testErrorCode;
        } type4;//Moons BOB Navigator
    };
    UCHAR                   firmwareRevMajor;
    UCHAR                   firmwareRevMinor;
    EMCPAL_LARGE_INTEGER    timeStamp;
} SPECL_BATTERY_STATUS, *PSPECL_BATTERY_STATUS;

/*********************************
 * SPECL_BATTERY_STATUS_PER_BLADE
 *********************************/
typedef struct _SPECL_BATTERY_STATUS_PER_BLADE
{
    ULONG                   batteryCount;
    SPECL_BATTERY_STATUS    batteryStatus[MAX_BATTERY_COUNT_PER_BLADE];
} SPECL_BATTERY_STATUS_PER_BLADE, *PSPECL_BATTERY_STATUS_PER_BLADE;

/************************
 * SPECL_BATTERY_SUMMARY
 ************************/
typedef struct _SPECL_BATTERY_SUMMARY
{
    BOOL                            sfiMaskStatus;
    ULONG                           bladeCount;
    SPECL_BATTERY_STATUS_PER_BLADE  batterySummary[SP_ID_MAX];
} SPECL_BATTERY_SUMMARY, *PSPECL_BATTERY_SUMMARY;



/******************************************************************************
 *             S T A N D B Y   P O W E R   S U P P L Y   T Y P E S
 ******************************************************************************/

#define SPS_MANUFACTURER_LENGTH     37
#define SPS_DATE_LENGTH             11
#define SPS_PART_NUM_LENGTH         12
#define SPS_REVISION_LENGTH         4
#define SPS_SERIAL_NUM_LENGTH       15

/* Li-ion 2.2 kw SPS Family and FRU ID */
#define SPS_FFID_LENGTH             5
#define SPS_BAT_FFID_LENGTH         14
#define SPS_BAT_HW_VERSION_LENGTH   5

#define MAX_SPS_ISP_STATE_LOG_ENTRY 10

/************
 * SPS_STATE
 ************/
typedef enum _SPS_STATE
{
    SPS_STATE_UNKNOWN,
    SPS_STATE_AVAILABLE,
    SPS_STATE_CHARGING,
    SPS_STATE_ON_BATTERY,
    SPS_STATE_FAULTED,
    SPS_STATE_SHUTDOWN,
    SPS_STATE_TESTING,
} SPS_STATE;

/************
 * SPS_ISP_STATE
 ************/
typedef enum _SPS_ISP_STATE
{
    SPS_ISP_UNKNOWN,
    SPS_ISP_IDLE,
    SPS_ISP_INIT,
    SPS_ISP_READY_TO_RECEIVE,
    SPS_ISP_DOWNLOADING,
    SPS_ISP_DOWNLOAD_COMPLETE,
    SPS_ISP_ACTIVATED,
    SPS_ISP_ACTIVATE_FAILED,
    SPS_ISP_ABORTED,
    SPS_ISP_NOT_READY,
} SPS_ISP_STATE;

/********************
 * SPS_ISP_LOG_ENTRY
 *******************/
typedef struct _SPS_ISP_LOG_ENTRY
{
    SPS_ISP_STATE           state;
    EMCPAL_LARGE_INTEGER    timeStamp;
} SPS_ISP_LOG_ENTRY;

/******************
 * SPS_TEST_STATUS
 ******************/
typedef enum _SPS_TEST_STATUS
{
    SPS_TEST_UNKNOWN,
    SPS_TEST_PASS,
    SPS_TEST_FAIL,
    SPS_TEST_TESTING,
} SPS_TEST_STATUS;

/***********
 * SPS_TYPE
 ***********/
typedef enum _SPS_TYPE
{
    SPS_TYPE_UNKNOWN,
    SPS_TYPE_1_0_KW,
    SPS_TYPE_1_2_KW,
    SPS_TYPE_2_2_KW,
    SPS_TYPE_LI_ION_2_2_KW,
    SPS_TYPE_MAX,           // max value, used for limits
} SPS_TYPE;
/**********************
 * SPS_SWITCHOVER_TIME
 **********************/
typedef enum _SPS_SWITCHOVER_TIME
{
    SPS_SWTIME_UNKNOWN,
    SPS_SWTIME_25MS,
    SPS_SWTIME_10MS,
    SPS_SWTIME_MAX,           // max value, used for limits
} SPS_SWITCHOVER_TIME;

/***************************
 * SPECL_SPS_BATTERY_RESUME
 ***************************/
typedef struct _SPECL_SPS_BATTERY_RESUME
{
    SPECL_ERROR             transactionStatus;
    CHAR                    batFfid[SPS_BAT_FFID_LENGTH];
    CHAR                    batHardwareVersion[SPS_BAT_HW_VERSION_LENGTH];
    CHAR                    batSupplier[SPS_MANUFACTURER_LENGTH];
    CHAR                    batSupplierModelNum[SPS_MANUFACTURER_LENGTH];
    CHAR                    batFirmwareRev[SPS_MANUFACTURER_LENGTH];        /* in XX.XX format */
    CHAR                    batFirmwareDate[SPS_DATE_LENGTH];               /* in MM/DD/YY format */
    CHAR                    batEmcPartNum[SPS_PART_NUM_LENGTH];
    CHAR                    batEmcRevision[SPS_REVISION_LENGTH];
    CHAR                    batEmcSerialNum[SPS_SERIAL_NUM_LENGTH];
    EMCPAL_LARGE_INTEGER    timeStamp;
} SPECL_SPS_BATTERY_RESUME, *PSPECL_SPS_BATTERY_RESUME;

/*******************
 * SPECL_SPS_RESUME
 *******************/
typedef struct _SPECL_SPS_RESUME
{
    BOOL                        sfiMaskStatus;
    SPECL_ERROR                 transactionStatus;
    CHAR                        supplier[SPS_MANUFACTURER_LENGTH];
    CHAR                        supplierModelNum[SPS_MANUFACTURER_LENGTH];
    CHAR                        primaryFirmwareRev[SPS_MANUFACTURER_LENGTH];    /* in XX.XX format */
    CHAR                        secondaryFirmwareRev[SPS_MANUFACTURER_LENGTH];  /* in XX.XX format */
    CHAR                        firmwareDate[SPS_DATE_LENGTH];                  /* in MM/DD/YY format */
    CHAR                        emcPartNum[SPS_PART_NUM_LENGTH];
    CHAR                        emcRevision[SPS_REVISION_LENGTH];
    CHAR                        emcSerialNum[SPS_SERIAL_NUM_LENGTH];
    CHAR                        ffid[SPS_FFID_LENGTH];
    SPECL_SPS_BATTERY_RESUME    batteryResume;
    EMCPAL_LARGE_INTEGER        timeStamp;
} SPECL_SPS_RESUME, *PSPECL_SPS_RESUME;

/***************************
 * SPECL_SPS_BATTERY_STATUS
 ***************************/
typedef struct _SPECL_SPS_BATTERY_STATUS
{
    SPECL_ERROR             transactionStatus;
    BOOL                    inserted;
    HW_MODULE_TYPE          batteryUniqueID;
    ULONG                   batteryVoltage;
    UCHAR                   batteryTemperatureL;
    UCHAR                   batteryTemperatureH;
    ULONG                   nominalCap;
    ULONG                   remainingCap;
    SPS_BMS_REGISTER        bmsState;
    EMCPAL_LARGE_INTEGER    timeStamp;
} SPECL_SPS_BATTERY_STATUS, *PSPECL_SPS_BATTERY_STATUS;

/********************
 * SPECL_SPS_SUMMARY
 ********************/
typedef struct _SPECL_SPS_SUMMARY
{
    BOOL                        sfiMaskStatus;
    SPECL_ERROR                 transactionStatus;
    BOOL                        inserted;
    SPS_STATE                   state;
    SPS_TYPE                    type;
    SPS_TEST_STATUS             testStatus;
    ULONG                       batteryTime;
    BOOL                        batteryEOL;
    BOOL                        internalFaults;
    CHAR                        batteryInstallDate[SPS_DATE_LENGTH]; /* in MM/DD/YY format */
    BOOL                        chargeFail;
    BOOL                        latchedStop;
    BOOL                        enhancedMode;
    ULONG                       toyYear;
    ULONG                       toyWeek;
    SPS_MODE                    mode;
    SPS_ISP_LOG_ENTRY           ispLog[MAX_SPS_ISP_STATE_LOG_ENTRY]; // sorted latest first
    BOOL                        fastSwitchover;
    BOOL                        batteryPackModuleNotEngagedFault;
    BOOL                        cableConnectivity;
    HW_MODULE_TYPE              uniqueID;
    SPECL_SPS_BATTERY_STATUS    batteryStatus;
    SPECL_ERROR                 lastWriteStatus;
    EMCPAL_LARGE_INTEGER        timeStamp;
} SPECL_SPS_SUMMARY, *PSPECL_SPS_SUMMARY;



/******************************************************************************
 *                        S U I T C A S E   T Y P E S
 ******************************************************************************/

#define MAX_SUITCASE_COUNT_PER_BLADE    1

/************************
 * SPECL_SUITCASE_STATUS
 ************************/
typedef struct _SPECL_SUITCASE_STATUS
{
    SPECL_ERROR             transactionStatus;
    BOOL                    Tap12VMissing;
    BOOL                    shutdownWarning;
    BOOL                    ambientOvertempWarning;
    BOOL                    ambientOvertempFault;
    BOOL                    fanPackFailure;
    UCHAR                   tempSensor;
    EMCPAL_LARGE_INTEGER    timeStamp;
} SPECL_SUITCASE_STATUS, *PSPECL_SUITCASE_STATUS;

/**********************************
 * SPECL_SUITCASE_STATUS_PER_BLADE
 **********************************/
typedef struct _SPECL_SUITCASE_STATUS_PER_BLADE
{
    ULONG                   suitcaseCount;
    SPECL_SUITCASE_STATUS   suitcaseStatus[MAX_SUITCASE_COUNT_PER_BLADE];
} SPECL_SUITCASE_STATUS_PER_BLADE, *PSPECL_SUITCASE_STATUS_PER_BLADE;

/*************************
 * SPECL_SUITCASE_SUMMARY
 *************************/
typedef struct _SPECL_SUITCASE_SUMMARY
{
    BOOL                            sfiMaskStatus;
    ULONG                           bladeCount;
    SPECL_SUITCASE_STATUS_PER_BLADE suitcaseSummary[SP_ID_MAX];
} SPECL_SUITCASE_SUMMARY, *PSPECL_SUITCASE_SUMMARY;



/******************************************************************************
 *                             C A C H E   C A R D   T Y P E S
 ******************************************************************************/

#define MAX_CACHE_CARD_COUNT    1
#define CACHE_CARD_MODULE_ID    251

/**************************
 * SPECL_CACHE_CARD_STATUS
 **************************/
typedef struct _SPECL_CACHE_CARD_STATUS
{
    SPECL_ERROR             transactionStatus;
    HW_MODULE_TYPE          uniqueID;
    UINT_16E                fruCapability;
    BOOL                    uniqueIDFinal;
    BOOL                    inserted;
    LED_BLINK_RATE          faultLEDStatus;
    BOOL                    powerGood;
    BOOL                    powerUpFailure;
    BOOL                    powerUpEnable;
    BOOL                    reset;
    BOOL                    powerEnable;
    EMCPAL_LARGE_INTEGER    timeStamp;
} SPECL_CACHE_CARD_STATUS, *PSPECL_CACHE_CARD_STATUS;

/***************************
 * SPECL_CACHE_CARD_SUMMARY
 ***************************/
typedef struct _SPECL_CACHE_CARD_SUMMARY
{
    BOOL                    sfiMaskStatus;
    ULONG                   cacheCardCount;
    SPECL_CACHE_CARD_STATUS cacheCardSummary[MAX_CACHE_CARD_COUNT];
} SPECL_CACHE_CARD_SUMMARY, *PSPECL_CACHE_CARD_SUMMARY;



/******************************************************************************
 *                              F A N   T Y P E S
 ******************************************************************************/

#define MAX_FANPACK_COUNT_PER_BLADE 6
#define MAX_FAN_COUNT_PER_FANPACK   3
#define MAX_FAN_COUNT_PER_BLADE    (MAX_FANPACK_COUNT_PER_BLADE*MAX_FAN_COUNT_PER_FANPACK)

/*****************
 * FAN_STS_CONFIG
 *****************/
typedef struct _FAN_STS_CONFIG
{
    BOOLEAN             fanFault;
    ULONG               inletRPM;
    UCHAR               type;
    union {
        struct {
            UCHAR       inletPWM;       /*percentage*/
        } type2;//Transformers
        struct {
            BOOLEAN     inserted;       /*Obtained from fan_status sensor data for both local & peer fans.*/
            ULONG       targetRPM;      /*Only applies to Moons family.*/
        } type3;//Moons
    };
}FAN_STS_CONFIG, *PFAN_STS_CONFIG;

/*******************
 * SPECL_FAN_STATUS
 *******************/
typedef struct _SPECL_FANPACK_STATUS
{
    SPECL_ERROR             transactionStatus;
    UCHAR                   type;
    union {
        struct {
            LED_BLINK_RATE  faultLEDStatus;
        } type1;//Megatron, Jefire, Triton
        struct {
            BOOLEAN         inserted;
            LED_BLINK_RATE  faultLEDStatus;
            HW_MODULE_TYPE  uniqueID;
            UINT_16E        fruCapability;
            BOOLEAN         uniqueIDFinal;
            UCHAR           firmwareRevMajor;
            UCHAR           firmwareRevMinor;
        } type2;//Cooling Module
        struct {
            BOOLEAN         inserted;
            LED_BLINK_RATE  faultLEDStatus;
        } type3;//Hyperion
        //Oberons dont have a type.
    };
    ULONG                   fanCount;
    FAN_STS_CONFIG          fanStatus[MAX_FAN_COUNT_PER_FANPACK];
    EMCPAL_LARGE_INTEGER    timeStamp;
} SPECL_FANPACK_STATUS, *PSPECL_FANPACK_STATUS;

/*****************************
 * SPECL_FAN_STATUS_PER_BLADE
 *****************************/
typedef struct _SPECL_FAN_STATUS_PER_BLADE
{
    SPECL_ERROR             transactionStatus;
    BOOLEAN                 automaticFanControl;
    UCHAR                   type;
    union {
        struct {
            BOOLEAN         rearmCtrlAfterReset;
        } type1;//Transformers
        struct {
            BOOLEAN         singleFanPerPack;
        } type2;//Moons
    };
    BOOLEAN                 multFanFault;
    ULONG                   fanPackCount;
    SPECL_FANPACK_STATUS    fanPackStatus[MAX_FANPACK_COUNT_PER_BLADE];
    EMCPAL_LARGE_INTEGER    timeStamp;
} SPECL_FAN_STATUS_PER_BLADE, *PSPECL_FAN_STATUS_PER_BLADE;

/********************
 * SPECL_FAN_SUMMARY
 ********************/
typedef struct _SPECL_FAN_SUMMARY
{
    BOOL                                sfiMaskStatus;
    ULONG                               bladeCount;
    SPECL_FAN_STATUS_PER_BLADE          fanSummary[SP_ID_MAX];
} SPECL_FAN_SUMMARY, *PSPECL_FAN_SUMMARY;



/******************************************************************************
 *              M A N A G E M E N T   M O D U L E   T Y P E S
 ******************************************************************************/

#define MAX_MGMT_COUNT_PER_BLADE    1

/*************
 * PORT_SPEED
 *************/
typedef enum _PORT_SPEED
{
    SPEED_10_Mbps       = 0x0,
    SPEED_100_Mbps      = 0x1,
    SPEED_1000_Mbps     = 0x2,
    SPEED_RESERVED_Mbps = 0x3,
} PORT_SPEED;

/**************
 * PORT_DUPLEX
 **************/
typedef enum _PORT_DUPLEX
{
    HALF_DUPLEX     = 0,
    FULL_DUPLEX     = 1,
} PORT_DUPLEX;

/***************
 * PORT_ID_TYPE
 ***************/
typedef enum _PORT_ID_TYPE
{
    LAN_CROSSLINK               = 0x0,
    MANAGEMENT_PORT0_INTERNAL   = 0x1,
    SERVICE_PORT0_INTERNAL      = 0x2,
    MANAGEMENT_PORT0            = 0x3,
    SERVICE_PORT0               = 0x4,
    TOTAL_PORT_ID_PER_BLADE     = 0x5,
} PORT_ID_TYPE;

#if defined(__cplusplus)
/* use extern C++ here to trick other C++ files including this as extern C
 */
extern "C++"{
/* Define a postfix increment operator for _PORT_ID_TYPE
 */
inline _PORT_ID_TYPE operator++( PORT_ID_TYPE &port_id, int )
{
   return port_id = (PORT_ID_TYPE)((int)port_id + 1);
}
}
#endif

/******************
 * PORT_STS_CONFIG
 ******************/
typedef struct _PORT_STS_CONFIG
{
    BOOL            linkStatus;
    PORT_SPEED      portSpeed;
    PORT_DUPLEX     portDuplex;
    BOOL            autoNegotiate;
    UCHAR           vLanMapping;
}PORT_STS_CONFIG;

/*******************
 * VLAN_CONFIG_MODE
 *******************/
typedef enum _VLAN_CONFIG_MODE
{
    CUSTOM_VLAN_MODE            = 0x00,
    DEFAULT_VLAN_MODE           = 0x01,
    CLARiiON_VLAN_MODE          = 0x02,
    SYMM_VLAN_MODE              = 0x03,
    SAN_AUX_VLAN_MODE           = 0x04,
    CUSTOM_MGMT_ONLY_VLAN_MODE  = 0xF0,
    CUSTOM_SVR_ONLY_VLAN_MODE   = 0xF1,
    CUSTOM_NO_VLAN_MODE         = 0xF2,
    UNKNOWN_VLAN_MODE           = 0xFF,
} VLAN_CONFIG_MODE;

/********************
 * SPECL_MGMT_STATUS
 ********************/
typedef struct _SPECL_MGMT_STATUS
{
    SPECL_ERROR             transactionStatus;
    SMB_DEVICE              smbDevice;
    HW_MODULE_TYPE          uniqueID;
    UINT_16E                fruCapability;
    BOOL                    uniqueIDFinal;
    BOOL                    inserted;
    BOOL                    generalFault;
    csx_s16_t               faultInfoStatus;
    LED_BLINK_RATE          faultLEDStatus;
    VLAN_CONFIG_MODE        vLanConfigMode;
    BOOL                    loopDetect;
    PORT_STS_CONFIG         portStatus[TOTAL_PORT_ID_PER_BLADE];
    UCHAR                   firmwareRevMajor;
    UCHAR                   firmwareRevMinor;
    EMCPAL_LARGE_INTEGER    timeStamp;
} SPECL_MGMT_STATUS, *PSPECL_MGMT_STATUS;

/******************************
 * SPECL_MGMT_STATUS_PER_BLADE
 ******************************/
typedef struct _SPECL_MGMT_STATUS_PER_BLADE
{
    ULONG                   mgmtCount;
    SPECL_MGMT_STATUS       mgmtStatus[MAX_MGMT_COUNT_PER_BLADE];
} SPECL_MGMT_STATUS_PER_BLADE, *PSPECL_MGMT_STATUS_PER_BLADE;

/*********************
 * SPECL_MGMT_SUMMARY
 *********************/
typedef struct _SPECL_MGMT_SUMMARY
{
    BOOL                            sfiMaskStatus;
    ULONG                           bladeCount;
    SPECL_MGMT_STATUS_PER_BLADE     mgmtSummary[SP_ID_MAX];
} SPECL_MGMT_SUMMARY, *PSPECL_MGMT_SUMMARY;



/******************************************************************************
 *                              P C I   T Y P E S
 ******************************************************************************/

#define MAX_PCI_DEV_PER_BLADE   300

/*****************
 * SPECL_PCI_DATA
 *****************/
typedef struct _SPECL_PCI_DATA
{
    ULONG       bus;
    ULONG       device;
    ULONG       function;
} SPECL_PCI_DATA, *PSPECL_PCI_DATA;

/************************
 * SPECL_PCI_CONFIG_INFO
 ************************/
typedef struct _SPECL_PCI_CONFIG_INFO
{
    SPECL_PCI_DATA      address;
    PCI_COMMON_CONFIG   configData;
} SPECL_PCI_CONFIG_INFO, *PSPECL_PCI_CONFIG_INFO;

/********************
 * SPECL_PCI_SUMMARY
 ********************/
typedef struct _SPECL_PCI_SUMMARY
{
    BOOL                    sfiMaskStatus;
    ULONG                   devCount;
    SPECL_PCI_CONFIG_INFO   pciSummary[MAX_PCI_DEV_PER_BLADE];
} SPECL_PCI_SUMMARY, *PSPECL_PCI_SUMMARY;



/******************************************************************************
 *                   I / O   C O N T R O L L E R   T Y P E S
 ******************************************************************************/

#define MAX_IO_PORTS_PER_CONTROLLER         4
#define MAX_SERIAL_ID_BYTES                 256

/*************************
 * IO_CONTROLLER_PROTOCOL
 *************************/
typedef enum
{
    IO_CONTROLLER_PROTOCOL_UNKNOWN  = 0x0,
    IO_CONTROLLER_PROTOCOL_FIBRE    = 0x1,
    IO_CONTROLLER_PROTOCOL_ISCSI    = 0x2,
    IO_CONTROLLER_PROTOCOL_SAS      = 0x3,
    IO_CONTROLLER_PROTOCOL_FCOE     = 0x4,
    IO_CONTROLLER_PROTOCOL_AGNOSTIC = 0x5,
    IO_CONTROLLER_PROTOCOL_TOE      = 0x6,
} IO_CONTROLLER_PROTOCOL;

/*********************
 * SPECL_IO_PORT_LED_STATUS
 *********************/
typedef struct _SPECL_IO_PORT_LED_STATUS
{
    SPECL_ERROR                 transactionStatus;
    LED_BLINK_RATE              LEDBlinkRate;
    LED_COLOR_TYPE              LEDColor;
    BOOL                        sfpMuxOverride;
    BOOL                        sfpMuxGlobalBus;
    EMCPAL_LARGE_INTEGER        timeStamp;
} SPECL_IO_PORT_LED_STATUS, *PSPECL_IO_PORT_LED_STATUS;

/************************
 * SPECL_SFP_DATA_REGION
 ************************/
typedef struct _SPECL_SFP_DATA_REGION
{
    SPECL_ERROR             transactionStatus;
    UCHAR                   sfpEEPROM[MAX_SERIAL_ID_BYTES];
    EMCPAL_LARGE_INTEGER    timeStamp;
} SPECL_SFP_DATA_REGION, *PSPECL_SFP_DATA_REGION;


/*********************************
 * SPECL_PCI_TO_IO_MODULE_MAPPING
 *********************************/
typedef struct _SPECL_PCI_TO_IO_MODULE_MAPPING
{
    SPECL_PCI_DATA              ioPortInfo;
    ULONG                       PHYmapping;
    ULONG                       ioModuleNumber;
    ULONG                       ioPhysicalPortNumber;
    ULONG                       ioControllerNumber;
    ULONG                       ioControllerPortNumber;
    SMB_DEVICE                  resumeSmbDevice;
} SPECL_PCI_TO_IO_MODULE_MAPPING, *PSPECL_PCI_TO_IO_MODULE_MAPPING;

/*******************
 * SPECL_SFP_STATUS
 *******************/
typedef struct _SPECL_SFP_STATUS
{
    SMB_DEVICE                      smbDevice;
    BOOL                            inserted;
    UCHAR                           type;
    union {
        struct {
            BOOL                    txDisable;
            BOOL                    txReset;
            BOOL                    rxLossOfSignal;
            SPECL_SFP_DATA_REGION   serialID;
            SPECL_SFP_DATA_REGION   diagStatus;
        } type1;//SFP
        struct {
            SPECL_SFP_DATA_REGION   miniSasPage00;
        } type2;//miniSAS HD
    };
} SPECL_SFP_STATUS, *PSPECL_SFP_STATUS;

/*************************
 * SPECL_IO_PORT_PCI_DATA
 *************************/
typedef struct _SPECL_IO_PORT_PCI_DATA
{
    SPECL_PCI_DATA              pciData;
    ULONG                       vendorID;
    ULONG                       deviceID;
    IO_CONTROLLER_PROTOCOL      protocol;
} SPECL_IO_PORT_PCI_DATA, *PSPECL_IO_PORT_PCI_DATA;

/*********************
 * SPECL_IO_PORT_DATA
 *********************/
/* In miniport info struct, so changes here result in revlock with miniports.
 */
typedef struct _SPECL_IO_PORT_DATA
{
    /* PCI data only applicable for local ports
     */
    ULONG                       pciDataCount;
    SPECL_IO_PORT_PCI_DATA      portPciData[PCI_MAX_FUNCTION];

    BOOL                        SFPcapable;
    BOOL                        SFPaware;
    SFP_MEDIA_TYPE              SFPmedia;
    ULONG                       physicalPortIndex;
    ULONG                       moduleIndex;
    BOOL                        boot_device;
    ULONG                       PHYmapping;
    PORT_MINIPORT_ANALOG_INFO   portMiniportAnalogInfo;

    /* LED and SFP status only applicable for local ports
     */
    SPECL_IO_PORT_LED_STATUS    LEDStatus;
    SPECL_SFP_STATUS            sfpStatus;
} SPECL_IO_PORT_DATA, *PSPECL_IO_PORT_DATA;

/*****************************
 * SPECL_IO_CONTROLLER_STATUS
 *****************************/
typedef struct _SPECL_IO_CONTROLLER_STATUS
{
    IO_CONTROLLER_PROTOCOL      protocol;
    ULONG                       availableControllerLinkSpeeds;
    CTRL_MINIPORT_ANALOG_INFO   ctrlMiniportAnalogInfo;
    ULONG                       ioPortCount;
    SPECL_IO_PORT_DATA          ioPortInfo[MAX_IO_PORTS_PER_CONTROLLER];
} SPECL_IO_CONTROLLER_STATUS, *PSPECL_IO_CONTROLLER_STATUS;


/******************************************************************************
 *                       I / O   M O D U L E   T Y P E S
 ******************************************************************************/

#define TOTAL_IO_MOD_PER_BLADE              12
#define MAX_IO_CONTROLLERS_PER_MODULE       3

/*****************
 * IO_MODULE_TYPE
 *****************/
typedef enum
{
    IO_MODULE_TYPE_UNKNOWN = 0,
    IO_MODULE_TYPE_SLIC_1_0,
    IO_MODULE_TYPE_SLIC_2_0 = IO_MODULE_TYPE_SLIC_1_0,  //They're the same for now.
    IO_MODULE_TYPE_BEM,
    IO_MODULE_TYPE_MEZZANINE,
    IO_MODULE_TYPE_ONBOARD,
    IO_MODULE_TYPE_MAX,
} IO_MODULE_TYPE;

/******************
 * SPECL_IO_STATUS
 ******************/
typedef struct _SPECL_IO_STATUS
{
    SPECL_ERROR                 transactionStatus;
    HW_MODULE_TYPE              uniqueID;
    UINT_16E                    fruCapability;
    BOOL                        uniqueIDFinal;
    BOOL                        inserted;
    BOOL                        diplexFPGAPresent;
    IO_MODULE_TYPE              moduleType;
    UCHAR                       type;
    union {
        struct {
            LED_BLINK_RATE      faultLEDStatus;
            BOOL                powerGood;
            BOOL                powerUpFailure;
            BOOL                powerUpEnable;
            BOOL                reset;
            BOOL                powerEnable;
            BOOL                persistedPowerState;
        } type1;//IO Module/BEM
    };
    ULONG                       ioControllerCount;
    SPECL_IO_CONTROLLER_STATUS  ioControllerInfo[MAX_IO_CONTROLLERS_PER_MODULE];
    EMCPAL_LARGE_INTEGER        timeStamp;
} SPECL_IO_STATUS, *PSPECL_IO_STATUS;

/***********************************
 * SPECL_IO_STATUS_PER_BLADE
 ***********************************/
typedef struct _SPECL_IO_STATUS_PER_BLADE
{
    ULONG                       ioModuleCount;
    SPECL_IO_STATUS             ioStatus[TOTAL_IO_MOD_PER_BLADE];
} SPECL_IO_STATUS_PER_BLADE, *PSPECL_IO_STATUS_PER_BLADE;

/*******************
 * SPECL_IO_SUMMARY
 *******************/
typedef struct _SPECL_IO_SUMMARY
{
    BOOL                        sfiMaskStatus;
    ULONG                       bladeCount;
    SPECL_IO_STATUS_PER_BLADE   ioSummary[SP_ID_MAX];
} SPECL_IO_SUMMARY, *PSPECL_IO_SUMMARY;


/******************************************************************************
 *                              B M C   T Y P E S
 ******************************************************************************/

#define TOTAL_BMC_PER_BLADE         1
#define TOTAL_I2C_BUSSES_PER_BMC    8

/******************
 * SHUTDOWN_REASON
 ******************/
typedef struct _SHUTDOWN_REASON
{
    BOOL embedIOOverTemp;
    BOOL fanFault;
    BOOL diskOverTemp;
    BOOL slicOverTemp;
    BOOL psOverTemp;
    BOOL memoryOverTemp;
    BOOL cpuOverTemp;
    BOOL ambientOverTemp;
    BOOL sspHang;
} SHUTDOWN_REASON, *PSHUTDOWN_REASON;

/*********************
 * SEL_LOGGING_STATUS
 *********************/
typedef enum _SEL_LOGGING_STATUS
{
    SEL_CORRECTABLE_MEMORY_LOGGING_DISABLED         = 0x00,
    SEL_SOME_LOGGING_DISABLED                       = 0x01,
    SEL_LOG_RESET_CLEARED                           = 0x02,
    SEL_ALL_LOGGING_DISABLED                        = 0x03,
    SEL_FULL                                        = 0x04,
    SEL_ALMOST_FULL                                 = 0x05,
    SEL_CORRECTABLE_MACHINE_CHECK_LOGGING_DISABLED  = 0x06,
    SEL_ENABLED,
    SEL_UNKNOWN,
} SEL_LOGGING_STATUS;

/**************
 * BIST_RESULT
 **************/
typedef enum _BIST_RESULT
{
    BIST_PASSED                 = 0x00,
    BIST_FAILED                 = 0x01,
    BIST_IN_PROGRESS            = 0x02,
    BIST_SKIPPED_UNSUPPORTED    = 0x03,
    BIST_DELAYED_RESOURCES      = 0x04,
    BIST_CANCELLED_RESOURCES    = 0x05,
    BIST_DELAYED_BUSY           = 0x06,
    BIST_CANCELLED_BUSY         = 0x07,
    BIST_FAILED_TO_EXECUTE      = 0x08,
    BIST_INVALID_INIT_VAL       = 0xFF
} BIST_RESULT;

/*************
 * MFB_ACTION
 *************/
typedef enum _MFB_ACTION
{
    MFB_DO_NOTHING              = 0x00,
    MFB_RESET_HOST              = 0x01,
    MFB_ASSERT_SOFT_FLAG        = 0x02,
    MFB_ASSERT_SMI              = 0x03,
    MFB_POWER_ON_OFF            = 0x04,
} MFB_ACTION;

/************************
 * SPECL_BMC_BIST_STATUS
 ************************/
typedef struct _SPECL_BMC_BIST_STATUS
{
    BMC_BIST_RESULT             cpuTest;
    BMC_BIST_RESULT             dramTest;
    BMC_BIST_RESULT             sramTest;
    BMC_BIST_RESULT             i2cTest;
    BMC_BIST_RESULT             fan0Test1;
    BMC_BIST_RESULT             fan0Test2;
    BMC_BIST_RESULT             fan0Test3;
    BMC_BIST_RESULT             fan1Test1;
    BMC_BIST_RESULT             fan1Test2;
    BMC_BIST_RESULT             fan1Test3;
    BMC_BIST_RESULT             fan2Test1;
    BMC_BIST_RESULT             fan2Test2;
    BMC_BIST_RESULT             fan2Test3;
    BMC_BIST_RESULT             fan3Test1;
    BMC_BIST_RESULT             fan3Test2;
    BMC_BIST_RESULT             fan3Test3;
    BMC_BIST_RESULT             fan4Test1;
    BMC_BIST_RESULT             fan4Test2;
    BMC_BIST_RESULT             fan4Test3;
    BMC_BIST_RESULT             superCapTest;
    BMC_BIST_RESULT             bbuTest;
    BMC_BIST_RESULT             sspTest;
    BMC_BIST_RESULT             uart2Test;
    BMC_BIST_RESULT             nvramTest;
    BMC_BIST_RESULT             usbTest;
    BMC_BIST_RESULT             sgpioTest;
    BMC_BIST_RESULT             bobTest;
    BMC_BIST_RESULT             uart3Test;
    BMC_BIST_RESULT             uart4Test;
    BMC_BIST_RESULT             virtUart0Test;
    BMC_BIST_RESULT             virtUart1Test;
} SPECL_BMC_BIST_STATUS, *PSPECL_BMC_BIST_STATUS;

typedef struct _SPECL_BMC_P_BIST_CPU_TEST_STATUS
{
    BMC_BIST_RESULT            overallStatus;
    csx_u32_t                  testErrorCode;
    IPMI_P_BIST_CPU_TEST_RAW   rawConfig;
}SPECL_BMC_P_BIST_CPU_TEST_STATUS;

typedef struct _SPECL_BMC_P_BIST_RAM_TEST_STATUS
{
    BMC_BIST_RESULT            overallStatus;
    csx_u32_t                  testErrorCode;
    IPMI_P_BIST_RAM_TEST_RAW   rawConfig;
}SPECL_BMC_P_BIST_RAM_TEST_STATUS;

typedef struct _SPECL_BMC_P_BIST_I2C_TEST_STATUS
{
    BMC_BIST_RESULT            overallStatus;
    csx_u32_t                  testErrorCode;
    IPMI_P_BIST_I2C_TEST_RAW   rawConfig;
}SPECL_BMC_P_BIST_I2C_TEST_STATUS;

typedef struct _SPECL_BMC_P_BIST_UART_TEST_STATUS
{
    BMC_BIST_RESULT            overallStatus;
    csx_u32_t                  testErrorCode;
    IPMI_P_BIST_UART_TEST_RAW   rawConfig;
}SPECL_BMC_P_BIST_UART_TEST_STATUS;

typedef struct _SPECL_BMC_P_BIST_SSP_TEST_STATUS
{
    BMC_BIST_RESULT            overallStatus;
    csx_u32_t                  testErrorCode;
    IPMI_P_BIST_SSP_TEST_RAW   rawConfig;
}SPECL_BMC_P_BIST_SSP_TEST_STATUS;

typedef struct _SPECL_BMC_P_BIST_BBU_TEST_STATUS
{
    BMC_BIST_RESULT            overallStatus;
    csx_u32_t                  testErrorCode;
    IPMI_P_BIST_BBU_TEST_RAW   rawConfig;
}SPECL_BMC_P_BIST_BBU_TEST_STATUS;

typedef struct _SPECL_BMC_P_BIST_NVRAM_TEST_STATUS
{
    BMC_BIST_RESULT            overallStatus;
    csx_u32_t                  testErrorCode;
    IPMI_P_BIST_NVRAM_TEST_RAW   rawConfig;
}SPECL_BMC_P_BIST_NVRAM_TEST_STATUS;

typedef struct _SPECL_BMC_P_BIST_SGPIO_TEST_STATUS
{
    BMC_BIST_RESULT              overallStatus;
    csx_u32_t                    testErrorCode;
    IPMI_P_BIST_SGPIO_TEST_RAW   rawConfig;
}SPECL_BMC_P_BIST_SGPIO_TEST_STATUS;

typedef struct _SPECL_BMC_P_BIST_FAN_TEST_STATUS
{
    BMC_BIST_RESULT            overallStatus;
    csx_u32_t                  testErrorCode;
    IPMI_P_BIST_FAN_TEST_RAW   rawConfig;
}SPECL_BMC_P_BIST_FAN_TEST_STATUS;

typedef struct _SPECL_BMC_P_BIST_ARB_TEST_STATUS
{
    BMC_BIST_RESULT            overallStatus;
    csx_u32_t                  testErrorCode;
    IPMI_P_BIST_ARB_TEST_RAW   rawConfig;
}SPECL_BMC_P_BIST_ARB_TEST_STATUS;

typedef struct _SPECL_BMC_P_BIST_NCSI_LAN_TEST_STATUS
{
    BMC_BIST_RESULT            overallStatus;
    csx_u32_t                  testErrorCode;
    IPMI_P_NCSI_LAN_TEST_RAW   rawConfig;
}SPECL_BMC_P_BIST_NCSI_LAN_TEST_STATUS;

typedef struct _SPECL_BMC_P_BIST_DED_LAN_TEST_STATUS
{
    BMC_BIST_RESULT            overallStatus;
    csx_u32_t                  testErrorCode;
    IPMI_P_DEDICATED_LAN_TEST_RAW   rawConfig;
}SPECL_BMC_P_BIST_DED_LAN_TEST_STATUS;

typedef enum _BIST_P_I2C_TEST_IDS
{
    I2C_0_TEST = 0,
    I2C_1_TEST,
    I2C_2_TEST,
    I2C_3_TEST,
    I2C_4_TEST,
    I2C_5_TEST,
    I2C_6_TEST,
    I2C_7_TEST,
    I2C_TEST_MAX
} BIST_P_I2C_TEST_IDS;

typedef enum _BIST_P_FAN_TEST_IDS
{
    FAN_0_TEST = 0,
    FAN_1_TEST,
    FAN_2_TEST,
    FAN_3_TEST,
    FAN_4_TEST,
    FAN_5_TEST,
    FAN_6_TEST,
    FAN_7_TEST,
    FAN_8_TEST,
    FAN_9_TEST,
    FAN_10_TEST,
    FAN_11_TEST,
    FAN_12_TEST,
    FAN_13_TEST,
    FAN_TEST_MAX
} BIST_P_FAN_TEST_IDS;

typedef enum _BIST_P_BBU_TEST_IDS
{
    BBU_0_TEST = 0,
    BBU_1_TEST,
    BBU_2_TEST,
    BBU_3_TEST,
    BBU_TEST_MAX
} BIST_P_BBU_TEST_IDS;

#define BIST_P_UART_TEST_INDEX_OFFSET 2 //UART tests start from UART2, so need this to help while printing them.
typedef enum _BIST_P_UART_TEST_IDS
{
    UART_2_TEST = 0,
    UART_3_TEST,
    UART_4_TEST,
    UART_TEST_MAX
} BIST_P_UART_TEST_IDS;

typedef enum _BIST_P_ARB_TEST_IDS
{
    ARB_0_TEST = 0,
    ARB_1_TEST,
    ARB_2_TEST,
    ARB_3_TEST,
    ARB_4_TEST,
    ARB_5_TEST,
    ARB_6_TEST,
    ARB_7_TEST,
    ARB_TEST_MAX
} BIST_P_ARB_TEST_IDS;

/**************************
 * SPECL_BMC_P_BIST_STATUS
 **************************/
typedef struct _SPECL_BMC_P_BIST_STATUS
{
    SPECL_BMC_P_BIST_CPU_TEST_STATUS      cpuTest;
    SPECL_BMC_P_BIST_RAM_TEST_STATUS      dramTest;
    SPECL_BMC_P_BIST_RAM_TEST_STATUS      sramTest;
    SPECL_BMC_P_BIST_I2C_TEST_STATUS      i2cTests[I2C_TEST_MAX];
    SPECL_BMC_P_BIST_UART_TEST_STATUS     uartTests[UART_TEST_MAX];
    SPECL_BMC_P_BIST_SSP_TEST_STATUS      sspTest;
    SPECL_BMC_P_BIST_BBU_TEST_STATUS      bbuTests[BBU_TEST_MAX];
    SPECL_BMC_P_BIST_NVRAM_TEST_STATUS    nvramTest;
    SPECL_BMC_P_BIST_SGPIO_TEST_STATUS    sgpioTest;
    SPECL_BMC_P_BIST_FAN_TEST_STATUS      fanTests[FAN_TEST_MAX];
    SPECL_BMC_P_BIST_ARB_TEST_STATUS      arbTests[ARB_TEST_MAX];
    SPECL_BMC_P_BIST_NCSI_LAN_TEST_STATUS ncsiLanTest;
    SPECL_BMC_P_BIST_DED_LAN_TEST_STATUS  dedicatedLanTest;
} SPECL_BMC_P_BIST_STATUS, *PSPECL_BMC_P_BIST_STATUS;

/***********************
 * SPECL_BMC_MFB_STATUS
 ***********************/
typedef struct _SPECL_BMC_MFB_STATUS
{
    BOOL                    ledFeedbackEnabled;
    MFB_ACTION              shortPressPreBootAction;    /* < 10 seconds */
    MFB_ACTION              shortPressPostBootAction;   /* < 10 seconds */
    MFB_ACTION              longPressPreBootAction;     /* > 10 seconds, < 60 seconds */
    MFB_ACTION              longPressPostBootAction;    /* > 10 seconds, < 60 seconds */
} SPECL_BMC_MFB_STATUS, *PSPECL_BMC_MFB_STATUS;

/*************************
 * SPECL_BMC_P_MFB_STATUS
 *************************/
typedef struct _SPECL_BMC_P_MFB_STATUS
{
    MFB_ACTION              shortPressPreBootAction;    /* < 10 seconds */
    MFB_ACTION              shortPressPostBootAction;   /* < 10 seconds */
    MFB_ACTION              longPressPreBootAction;     /* > 10 seconds, < 60 seconds */
    MFB_ACTION              longPressPostBootAction;    /* > 10 seconds, < 60 seconds */
} SPECL_BMC_P_MFB_STATUS, *PSPECL_BMC_P_MFB_STATUS;

/***********************
 * SPECL_FW_UPDATE_STATUS
 ***********************/
typedef enum _SPECL_FW_UPDATE_STATUS
{
    UPDATE_SUCCESSFUL                       = 0x00,
    IMAGE_CHECKSUM_ERROR                    = 0x01,
    IMAGE_SIZE_ERROR                        = 0x02,
    DEVICE_NOT_PRESENT_ERROR                = 0x03,
    BUS_ERROR_ON_REVISION_READ              = 0x04,
    UPDATE_IN_PROGRESS                      = 0x05,
    UPDATE_SKIPPED                          = 0x06,
    NO_UPDATE_ATTEMPTED                     = 0x07,
    UPDATE_FAILED                           = 0x08,
} SPECL_FW_UPDATE_STATUS, *PSPECL_FW_UPDATE_STATUS;

/*************************************
 * SPECL_FW_UPDATE_STATUS_PER_FW_TYPE
 *************************************/
typedef struct _SPECL_FW_UPDATE_STATUS_PER_FW_TYPE
{
    SPECL_FW_UPDATE_STATUS          updateStatus;
    UCHAR                           revisionMajor;
    UCHAR                           revisionMinor;
} SPECL_FW_UPDATE_STATUS_PER_FW_TYPE, *PSPECL_FW_UPDATE_STATUS_PER_FW_TYPE;

/*****************************
 * SPECL_BMC_FW_UPDATE_STATUS
 *****************************/
typedef struct _SPECL_BMC_FW_UPDATE_STATUS
{
    SPECL_FW_UPDATE_STATUS_PER_FW_TYPE bmcAppMain;
    SPECL_FW_UPDATE_STATUS_PER_FW_TYPE bmcAppRedundant;
    SPECL_FW_UPDATE_STATUS_PER_FW_TYPE ssp;
    SPECL_FW_UPDATE_STATUS_PER_FW_TYPE bootBlock;
} SPECL_BMC_FW_UPDATE_STATUS, *PSPECL_BMC_FW_UPDATE_STATUS;

/***************************************
 * SPECL_P_FW_UPDATE_STATUS_PER_FW_TYPE
 ***************************************/
typedef struct _SPECL_P_FW_UPDATE_STATUS_PER_FW_TYPE
{
    UCHAR                           revisionMajor;
    UCHAR                           revisionMinor;
} SPECL_P_FW_UPDATE_STATUS_PER_FW_TYPE, *PSPECL_P_FW_UPDATE_STATUS_PER_FW_TYPE;

/*******************************
 * SPECL_BMC_P_FW_UPDATE_STATUS
 *******************************/
typedef struct _SPECL_BMC_P_FW_UPDATE_STATUS
{
    SPECL_P_FW_UPDATE_STATUS_PER_FW_TYPE bmcAppMain;
    SPECL_P_FW_UPDATE_STATUS_PER_FW_TYPE ssp;
    SPECL_P_FW_UPDATE_STATUS_PER_FW_TYPE bootBlock;
} SPECL_BMC_P_FW_UPDATE_STATUS, *PSPECL_BMC_P_FW_UPDATE_STATUS;

/***********************
 * SPECL_NETWORK_MODE
 ***********************/
typedef enum _SPECL_NETWORK_MODE
{
    NETWORK_UNKNOWN  = 0x00,
    NETWORK_STATIC   = 0x01,
    NETWORK_DYNAMIC  = 0x02
} SPECL_NETWORK_MODE, *PSPECL_NETWORK_MODE;

/***********************
 * SPECL_NETWORK_PORT
 ***********************/
typedef enum _SPECL_NETWORK_PORT
{
    MGMT_PORT     = 0x00,
    SERVICE_PORT  = 0x01,
    UNKNOWN_PORT  = 0xFF
} SPECL_NETWORK_PORT, *PSPECL_NETWORK_PORT;

/*******************
 * SPECL_BMC_STATUS
 *******************/
typedef struct _SPECL_BMC_STATUS
{
    SPECL_ERROR             transactionStatus;
    BOOL                    delayedShutdownEnabled;
    ULONG                   delayedShutdownSeconds;
    BOOL                    shutdownWarning;
    SHUTDOWN_REASON         shutdownReason;
    ULONG                   shutdownTimeRemaining;
    BOOL                    spPowerDisabled;
    ULONG                   eccCount;
    SEL_LOGGING_STATUS      selState;
    SPECL_NETWORK_PORT      networkPort;
    SPECL_NETWORK_MODE      networkMode;
    UCHAR                   ipAddress[4];
    UCHAR                   subnet[4];
    UCHAR                   gatewayIP[4];
    UCHAR                   type;
    union {
        struct {
            BOOL                            spsPresent;
            BOOL                            pollingEnabled[TOTAL_I2C_BUSSES_PER_BMC];   /* Per bus */
            SPECL_BMC_MFB_STATUS            mfbStatus;
            SPECL_BMC_BIST_STATUS           bistReport;     /* BIST is local only */
            SPECL_BMC_FW_UPDATE_STATUS      firmwareStatus;
        } type1;//Megatron
        struct {
            BOOL                            pollingEnabled[TOTAL_I2C_BUSSES_PER_BMC];   /* Per bus */
            SPECL_BMC_MFB_STATUS            mfbStatus;
            SPECL_BMC_BIST_STATUS           bistReport;     /* BIST is local only */
            SPECL_BMC_FW_UPDATE_STATUS      firmwareStatus;
        } type2;//Jetfire
        struct {
            BOOL                            lowPowerMode;
            UCHAR                           rideThroughTime;    /* in seconds */
            UCHAR                           vaultTimeout;       /* in minutes */
            SPECL_BMC_P_MFB_STATUS          mfbStatus;
            SPECL_BMC_P_BIST_STATUS         bistReport;
            SPECL_BMC_P_FW_UPDATE_STATUS    firmwareStatus;
        } type3;//Moons
    };
    EMCPAL_LARGE_INTEGER    timeStamp;
} SPECL_BMC_STATUS, *PSPECL_BMC_STATUS;

/*****************************
 * SPECL_BMC_STATUS_PER_BLADE
 *****************************/
typedef struct _SPECL_BMC_STATUS_PER_BLADE
{
    ULONG               bmcCount;
    SPECL_BMC_STATUS    bmcStatus[TOTAL_BMC_PER_BLADE];
} SPECL_BMC_STATUS_PER_BLADE, *PSPECL_BMC_STATUS_PER_BLADE;

/********************
 * SPECL_BMC_SUMMARY
 ********************/
typedef struct _SPECL_BMC_SUMMARY
{
    BOOL                            sfiMaskStatus;
    ULONG                           bladeCount;
    SPECL_BMC_STATUS_PER_BLADE      bmcSummary[SP_ID_MAX];
} SPECL_BMC_SUMMARY, *PSPECL_BMC_SUMMARY;


/******************************************************************************
 *                              S D R   T Y P E S
 ******************************************************************************/

#define MAX_SDR_COUNT_PER_BMC         256

 /************************
 * SPECL_IPMI_SDR_DATA
 ************************/
typedef struct _SPECL_IPMI_SDR_DATA
{
    BOOL                    valid;
    ULONG                   recordID;
    IPMI_SDR_RECORD_TYPE    type;
    union 
    {
        IPMI_SDR_RECORD_FULL_SENSOR                 fullSensor;
        IPMI_SDR_RECORD_COMPACT_SENSOR              compactSensor;
        IPMI_SDR_RECORD_EVENTONLY_SENSOR            eventonlySensor;
        IPMI_SDR_RECORD_GENERIC_LOCATOR_SENSOR      genLocator;
        IPMI_SDR_RECORD_FRU_LOCATOR_SENSOR          fruLocator;
        IPMI_SDR_RECORD_MC_LOCATOR_SENSOR           mcLocator;
        IPMI_SDR_RECORD_ENTITY_ASSOC_SENSOR         entAssoc;
        UCHAR                                       rawData[IPMI_SDR_MAX_DATA_SIZE];
    } record;

}SPECL_IPMI_SDR_DATA, *PSPECL_IPMI_SDR_DATA;

/*******************
 * SPECL_SDR_STATUS
 *******************/
typedef struct _SPECL_SDR_STATUS
{
    SPECL_ERROR                     transactionStatus;
    UCHAR                           sdrVersion;
    ULONG                           sdrCount;
    SPECL_IPMI_SDR_DATA             sdrData[MAX_SDR_COUNT_PER_BMC];
    EMCPAL_LARGE_INTEGER            timeStamp;
} SPECL_SDR_STATUS, *PSPECL_SDR_STATUS;

/*****************************
 * SPECL_SDR_STATUS_PER_BLADE
 *****************************/
typedef struct _SPECL_SDR_STATUS_PER_BLADE
{
    ULONG               bmcCount;
    SPECL_SDR_STATUS    sdrStatus[TOTAL_BMC_PER_BLADE];
} SPECL_SDR_STATUS_PER_BLADE, *PSPECL_SDR_STATUS_PER_BLADE;

/********************
 * SPECL_SDR_SUMMARY
 ********************/
typedef struct _SPECL_SDR_SUMMARY
{
    BOOL                            sfiMaskStatus;
    ULONG                           bladeCount;
    SPECL_SDR_STATUS_PER_BLADE      sdrSummary[SP_ID_MAX];
} SPECL_SDR_SUMMARY, *PSPECL_SDR_SUMMARY;

/******************************************************************************
 *      F A U L T   E X P A N D E R / S L A V E   P O R T   T Y P E S
 ******************************************************************************/

#define MAX_SLAVE_PORT_PER_BLADE        1

#define MAX_DIMM_FAULT              32
#define MAX_DRIVE_FAULT             25
#define MAX_SLIC_FAULT              12
#define MAX_POWER_FAULT             4
#define MAX_BATTERY_FAULT           4
#define MAX_FAN_FAULT               12
#define MAX_I2C_FAULT               8

/**************
 * FLT_REG_IDS
 **************/
typedef enum _FLT_REG_IDS
{
    PRIMARY_FLT_REG         = 0,
    MIRROR_FLT_REG          = 1,
    TOTAL_FLT_REG_PER_BLADE = 2,
} FLT_REG_IDS;

#define SPECL_C4_SMI_FAULT_CODE_HOLD_TIME   600  /* in seconds */
#define SPECL_C4_IS_BLADE_FAULT_CODE(code) \
     ( ((code>=0x0)&&(code<=0x26)) || ((code>=0x40)&&(code<=0x45)) || \
       ((code>=0x68)&&(code<=0x75)) || (code==0x5B) || (code==0x60)|| \
        (code==0x7A) || (code==0x7D) )

#define SPECL_C4_IS_BLADE_HANG_CODE(code) \
     ( ((code>=0x34)&&(code<=0x3F)) || ((code>=0x46)&&(code<=0x5A)) || \
       ((code>=0x5C)&&(code<=0x5F)) || ((code>=0x61)&&(code<=0x65)) || \
        (code==0x2A) || (code==0x2E) || (code==0x7C) || (code==0xFF) )

#define SPECL_C4_BLADE_HANG_CODE_INTERVAL(code) \
    ( ((code==0x2A)||(code==0x2E)) ? 480 : ((code==0x3A)||(code==0x7C)) ? 780 : 300 )

#define SPECL_C4_BBU_TEST_RETRY_INTERVAL   180  /* in seconds */
#define SPECL_C4_BBU_TEST_RETRY_ATTEMPTS   20

/******************************
 * SPECL_FAULT_REGISTER_STATUS
 ******************************/
typedef struct _SPECL_FAULT_REGISTER_STATUS
{
    SPECL_ERROR             transactionStatus;
    BOOL                    dimmFault[MAX_DIMM_FAULT];
    BOOL                    driveFault[MAX_DRIVE_FAULT];
    BOOL                    slicFault[MAX_SLIC_FAULT];
    BOOL                    powerFault[MAX_POWER_FAULT];
    BOOL                    batteryFault[MAX_BATTERY_FAULT];
    BOOL                    superCapFault;
    BOOL                    fanFault[MAX_FAN_FAULT];
    BOOL                    i2cFault[MAX_I2C_FAULT];
    BOOL                    cpuFault;
    BOOL                    mgmtFault;
    BOOL                    bemFault;
    BOOL                    eFlashFault;
    BOOL                    cacheCardFault;
    BOOL                    midplaneFault;
    BOOL                    cmiFault;
    BOOL                    allFrusFault;
    BOOL                    externalFruFault;
    BOOL                    anyFaultsFound; /* Indicates that any one of the bits above is set */
    csx_u32_t               cpuStatusRegister;
    EMCPAL_LARGE_INTEGER    timeStamp;
} SPECL_FAULT_REGISTER_STATUS, *PSPECL_FAULT_REGISTER_STATUS;

/*********************************
 * SPECL_FLT_EXP_STATUS_PER_BLADE
 *********************************/
typedef struct _SPECL_FLT_EXP_STATUS_PER_BLADE
{
    ULONG                       faultRegisterCount;
    SPECL_FAULT_REGISTER_STATUS faultRegisterStatus[TOTAL_FLT_REG_PER_BLADE];//fault status region & fault status mirror region
    BOOL                        faultStatusMirrorValid;
} SPECL_FLT_EXP_STATUS_PER_BLADE, *PSPECL_FLT_EXP_STATUS_PER_BLADE;

/************************
 * SPECL_FLT_EXP_SUMMARY
 ************************/
typedef struct _SPECL_FLT_EXP_SUMMARY
{
    BOOL                                sfiMaskStatus;
    ULONG                               bladeCount;
    SPECL_FLT_EXP_STATUS_PER_BLADE      fltExpSummary[SP_ID_MAX];
} SPECL_FLT_EXP_SUMMARY, *PSPECL_FLT_EXP_SUMMARY;

/**************************
 * SPECL_SLAVE_PORT_STATUS
 **************************/
typedef struct _SPECL_SLAVE_PORT_STATUS
{
    SPECL_ERROR             transactionStatus;
    SMB_DEVICE              smbDevice;
    UCHAR                   generalStatus;
    UCHAR                   componentStatus;
    UCHAR                   componentExtStatus;
    EMCPAL_LARGE_INTEGER    timeStamp;
} SPECL_SLAVE_PORT_STATUS, *PSPECL_SLAVE_PORT_STATUS;

/************************************
 * SPECL_SLAVE_PORT_STATUS_PER_BLADE
 ************************************/
typedef struct _SPECL_SLAVE_PORT_STATUS_PER_BLADE
{
    ULONG                       slavePortCount;
    SPECL_SLAVE_PORT_STATUS     slavePortStatus[MAX_SLAVE_PORT_PER_BLADE];
} SPECL_SLAVE_PORT_STATUS_PER_BLADE, *PSPECL_SLAVE_PORT_STATUS_PER_BLADE;

/***************************
 * SPECL_SLAVE_PORT_SUMMARY
 ***************************/
typedef struct _SPECL_SLAVE_PORT_SUMMARY
{
    BOOL                                    sfiMaskStatus;
    ULONG                                   bladeCount;
    SPECL_SLAVE_PORT_STATUS_PER_BLADE       slavePortSummary[SP_ID_MAX];
} SPECL_SLAVE_PORT_SUMMARY, *PSPECL_SLAVE_PORT_SUMMARY;

typedef struct _SPECL_I2C_SUMMARY
{
    BOOL   cmi_fault;
    BOOL   i2c_fault;
    BOOL   other_fault;
} SPECL_I2C_SUMMARY, *PSPECL_I2C_SUMMARY;

typedef struct _SPECL_EHORNET_SLAVE_PORT_STATUS
{
    SPECL_ERROR         transactionStatus;
    SMB_DEVICE          smbDevice;
    BYTE                status0;
    BYTE                status1;
    BYTE                status2;
    BYTE                status3;
    BYTE                status4;
    LARGE_INTEGER       timeStamp;
} SPECL_EHORNET_SLAVE_PORT_STATUS, *PSPECL_EHORNET_SLAVE_PORT_STATUS;


/******************************************************************************
 *                          L E D   T Y P E S
 ******************************************************************************/

#define FAULT_CONDITION     LED_COLOR_AMBER
#define BOOT_CONDITION      LED_COLOR_BLUE

/********************
 * SPECL_LED_SUMMARY
 ********************/
typedef struct _SPECL_LED_SUMMARY
{
    BOOL                sfiMaskStatus;
    LED_BLINK_RATE      SPFaultLED;
    BOOL                SPFaultLEDColor;
    LED_BLINK_RATE      UnsafeToRemoveLED;
    LED_BLINK_RATE      EnclosureFaultLED;
    LED_BLINK_RATE      ManagementPortLED;
    LED_BLINK_RATE      ServicePortLED;
    LED_BLINK_RATE      PeerSPFaultLED;
    UCHAR               type;
    union {
        struct {
            LED_BLINK_RATE      MemoryFaultLED;
        } type1;//Beachcomber & Oberon
        struct {
            LED_BLINK_RATE      PeerEnclosureFaultLED;
        } type2;//Megatron, Triton
    };
} SPECL_LED_SUMMARY, *PSPECL_LED_SUMMARY;



/******************************************************************************
 *                          D I M M  T Y P E S
 ******************************************************************************/

#define TOTAL_DIMM_PER_BLADE            24

/*********************
 * SPECL_DIMMS_STATUS
 *********************/
typedef struct _SPECL_DIMM_STATUS
{
    SPECL_ERROR             transactionStatus;
    BOOL                    retriesExhausted;
    SMB_DEVICE              smbDevice;
    BOOL                    inserted;
    SPD_DEVICE_TYPE         dimmType;
    SPD_REVISION            dimmRevision;
    ULONG                   dimmDensity;
    ULONG                   SPDBytesInUse;
    ULONG                   numOfBanks;
    ULONG                   numOfBankGroups;        /* Only for DDR4 */
    ULONG                   colAddressBits;
    ULONG                   rowAddressBits;
    ULONG                   deviceWidth;
    ULONG                   numberOfRanks;
    BOOL                    vendorRegionValid;
    UCHAR                   jedecIdCode             [JEDEC_ID_CODE_SIZE];
    UCHAR                   manufacturingLocation;
    UCHAR                   manufacturingDate       [MANUFACTURING_DATE_SIZE];
    UCHAR                   moduleSerialNumber      [MODULE_SERIAL_NUMBER_SIZE];
    UCHAR                   partNumber              [PART_NUMBER_SIZE_2];
    UCHAR                   EMCDimmPartNumber       [EMC_DIMM_PART_NUMBER_SIZE];
    UCHAR                   EMCDimmSerialNumber     [EMC_DIMM_SERIAL_NUMBER_SIZE];
    BYTE                    OldEMCDimmSerialNumber  [OLD_EMC_DIMM_SERIAL_NUMBER_SIZE];
    SPD_DIMM_INFO           rawInfo;
    EMCPAL_LARGE_INTEGER    timeStamp;
} SPECL_DIMM_STATUS, *PSPECL_DIMM_STATUS;

/*********************
 * SPECL_DIMM_SUMMARY
 *********************/
typedef struct _SPECL_DIMM_SUMMARY
{
    BOOL                    sfiMaskStatus;
    ULONG                   dimmCount;
    SPECL_DIMM_STATUS       dimmStatus[TOTAL_DIMM_PER_BLADE];
} SPECL_DIMM_SUMMARY, *PSPECL_DIMM_SUMMARY;



/******************************************************************************
 *                          M P L X R  T Y P E S
 ******************************************************************************/

#define MAX_PORTS_PER_HOST_UART         8

/*****************************
 * SPECL_IO_PORT_DIPLEX_STATUS
 *****************************/
typedef struct _SPECL_IO_PORT_UART_STATUS
{

    FPGA_BAUD_RATES             ioPortBaudRate;
    FPGA_VOLTAGE                ioPortVoltage;
    FPGA_LOOPBACK               ioPortLoopback;
} SPECL_IO_PORT_UART_STATUS, *PSPECL_IO_PORT_UART_STATUS;

/*********************
 * SPECL_MPLXR_STATUS
 *********************/
typedef struct _SPECL_MPLXR_STATUS
{
    SPECL_ERROR                 transactionStatus;
    MPLXR_DEVICE                mplxrDevice;
    BOOL                        inserted;
    UCHAR                       chipID;
    UCHAR                       revID;
    UCHAR                       mplxrType;
    UCHAR                       packetFormat;
    UCHAR                       hostUARTTimeout;
    UCHAR                       hostUARTFlowControl;
    FPGA_BAUD_RATES             hostUARTBaudRate;
    UCHAR                       diplexTimer;
    UCHAR                       LEDConfig;
    ULONG                       numberOfUARTS;
    SPECL_IO_PORT_UART_STATUS   diplexUARTInfo[MAX_PORTS_PER_HOST_UART];
    EMCPAL_LARGE_INTEGER        timeStamp;
} SPECL_MPLXR_STATUS, *PSPECL_MPLXR_STATUS;

/*********************
 * SPECL_MPLXR_SUMMARY
 *********************/
typedef struct _SPECL_MPLXR_SUMMARY
{
    BOOL                    sfiMaskStatus;
    ULONG                   ioModuleCount;
    SPECL_MPLXR_STATUS      mplxrStatus[TOTAL_IO_MOD_PER_BLADE];
} SPECL_MPLXR_SUMMARY, *PSPECL_MPLXR_SUMMARY;



/******************************************************************************
 *                         F I R M W A R E   T Y P E S
 ******************************************************************************/

#define MAX_PROGRAMMABLES_PER_FRU       15
#define INVALID_FW_REVISION             UNSIGNED_CHAR_MINUS_1

/*************************
 * SPECL_PROGRAMMABLE_REV
 *************************/
typedef struct _SPECL_PROGRAMMABLE_REV
{
    UINT_8E     major;
    UINT_8E     minor;
} SPECL_PROGRAMMABLE_REV, *PSPECL_PROGRAMMABLE_REV;

/**************************
 * SPECL_PROGRAMMABLE_INFO
 **************************/
typedef struct _SPECL_PROGRAMMABLE_INFO
{
    CHAR                    fwName [8];
    SPECL_PROGRAMMABLE_REV  currentRev;
    SPECL_PROGRAMMABLE_REV  postToFlashRev;
} SPECL_PROGRAMMABLE_INFO, *PSPECL_PROGRAMMABLE_INFO;

/************************
 * SPECL_FIRMWARE_STATUS
 ************************/
typedef struct _SPECL_FIRMWARE_STATUS
{
    SPECL_ERROR             transactionStatus;
    SMB_DEVICE              smbDevice;
    HW_MODULE_TYPE          familyFruId;
    ULONG                   programmableCount;
    SPECL_PROGRAMMABLE_INFO programmableInfo[MAX_PROGRAMMABLES_PER_FRU];
    EMCPAL_LARGE_INTEGER    timeStamp;
} SPECL_FIRMWARE_STATUS, *PSPECL_FIRMWARE_STATUS;

/*************************
 * SPECL_FIRMWARE_SUMMARY
 *************************/
typedef struct _SPECL_FIRMWARE_SUMMARY
{
    BOOL                    sfiMaskStatus;
    SPECL_FIRMWARE_STATUS   spFirmwareStatus;
    SPECL_FIRMWARE_STATUS   ps0FirmwareStatus;
    SPECL_FIRMWARE_STATUS   ps1FirmwareStatus;
    SPECL_FIRMWARE_STATUS   mgmtFirmwareStatus;
    SPECL_FIRMWARE_STATUS   bemFirmwareStatus;
    SPECL_FIRMWARE_STATUS   ioSlotFirmwareStatusArray[TOTAL_IO_MOD_PER_BLADE];
} SPECL_FIRMWARE_SUMMARY, *PSPECL_FIRMWARE_SUMMARY;



/******************************************************************************
 *                          M I S C   T Y P E S
 ******************************************************************************/

#define MAX_DRIVE_COUNT_PER_BLADE   6
#define MAX_PLX_COUNT_PER_BLADE     4

/*********************
 * SPECL_DEVICE_CLASS
 *********************/
typedef enum _SPECL_DEVICE_CLASS
{
    DEVICE_CLASS_INVALID = 0x0,
    DEVICE_CLASS_SMBUS,
    DEVICE_CLASS_GPIO,
    DEVICE_CLASS_MPLXR,
    DEVICE_CLASS_SPS,
    DEVICE_CLASS_IPMI,
} SPECL_DEVICE_CLASS;

/***********
 * COM_PORT
 ***********/
typedef enum _COM_PORT
{
    INVALID_COM_PORT,
    COM_1,
    COM_2,
} COM_PORT;

/***************************
 * COM_PORT_MUX_SELECT_CODE
 ***************************/
typedef enum _COM_PORT_MUX_SELECT_CODE
{
    DB_9                    = 0x0,
    PEER_COM1               = 0x1,
    PEER_COM2               = 0x2,
    MCU_DEBUG_CONSOLE       = 0x3,  /* MCU debug console or BMC console */
    LOCAL_MCU_CONNECTION    = 0x4,
    BMC_SOL_PORT            = 0x5,  /* BMC Serial Over LAN */
    BEM_DEBUG_PORT          = 0x6,  /* BEM debug port, valid for Jetfire only */
    INVALID_MUX_SELECTION   = 0xF,
} COM_PORT_MUX_SELECT_CODE;

/*********************
 * SPECL_DRIVE_STATUS
 *********************/
typedef struct _SPECL_DRIVE_STATUS
{
    BOOL                    inserted;
    UCHAR                   type;
    union {
        struct {
            BOOL            powerDown;
            LED_BLINK_RATE  faultLEDStatus;
        } type1;//External Drives (Megatron/Triton/Triton ERB)
        struct {
            BOOL            powerDown;
        } type2;//Internal Drives (Oberon/Hyperion/Triton/Triton ERB)
    };
} SPECL_DRIVE_STATUS, *PSPECL_DRIVE_STATUS;

/**************************
 * SPECL_QUICKBOOT_SUMMARY
 **************************/
typedef struct _SPECL_QUICKBOOT_SUMMARY
{
    IPMI_RESPONSE_QUICKBOOT_REG1  region1;
    IPMI_RESPONSE_QUICKBOOT_REG2  region2;
} SPECL_QUICKBOOT_SUMMARY, *PSPECL_QUICKBOOT_SUMMARY;

/*****************
 * SPECL_CNA_MODE
 *****************/
typedef enum _SPECL_CNA_MODE
{
    CNA_MODE_UNKNOWN        = 0,
    CNA_MODE_FIBRE          = 1,
    CNA_MODE_ETHERNET       = 2,
} SPECL_CNA_MODE;

/*****************************
 * SPECL_INTERNAL_CABLE_STATE
 *****************************/
typedef enum _SPECL_INTERNAL_CABLE_STATE
{
    INTERNAL_CABLE_STATE_UNKNOWN    = 0,
    INTERNAL_CABLE_STATE_INSERTED,
    INTERNAL_CABLE_STATE_MISSING,
    INTERNAL_CABLE_STATE_CROSSED,
} SPECL_INTERNAL_CABLE_STATE;


/*********************
 * SPECL_MISC_SUMMARY
 *********************/
typedef struct _SPECL_MISC_SUMMARY
{
    BOOL                        sfiMaskStatus;
    SP_ID                       spid;
    BOOL                        engineIdFault; /* NOTE: This is sticky (it will remain true after the fault clears) */
    BOOL                        peerInserted;
    SPID_HW_TYPE                peerHwType;
    SPID_HW_TYPE                localHwType;
    BOOL                        low_battery;
    BOOL                        peerHoldInReset;
    BOOL                        peerHoldInPost;
    BOOL                        localHoldInPost;
    SPECL_IO_PORT_DATA          bootDeviceInfo;
    COM_PORT_MUX_SELECT_CODE    localMuxSelectionCOM1;
    COM_PORT_MUX_SELECT_CODE    localMuxSelectionCOM2;
    COM_PORT_MUX_SELECT_CODE    peerMuxSelectionCOM1;
    COM_PORT_MUX_SELECT_CODE    peerMuxSelectionCOM2;
    ULONG                       localDriveCount;
    SPECL_DRIVE_STATUS          localDriveStatus[MAX_DRIVE_COUNT_PER_BLADE];
    ULONG                       localPlxCount;
    UCHAR                       localPlxRevId[MAX_PLX_COUNT_PER_BLADE];
    SPECL_QUICKBOOT_SUMMARY     quickBootInfo[SP_ID_MAX];
    UCHAR                       type;
    union {
        struct {
            BOOL                localPowerECBFault; /* Indicate if local Electric Circuit Breaker is tripped */
            BOOL                peerPowerECBFault;  /* Indicate if peer Electric Circuit Breaker is tripped */
        } type1;//Jetfire
        struct {
            SPECL_CNA_MODE              localCnaMode;
            SPECL_INTERNAL_CABLE_STATE  internalCableStatePort1;
        } type2;//Oberon
        struct {
            SPECL_CNA_MODE              localCnaMode;
        } type3;//Bearcat
    };
} SPECL_MISC_SUMMARY, *PSPECL_MISC_SUMMARY;

//++
// Name:
//      SPECL_RESET_TYPE
//
// Description:
//      Reset types for various reset (SP & BMC/SSP) commands in specl.
//--
typedef enum _SPECL_RESET_TYPE
{
    SPECL_COLD_RESET_BMC_SSP   = 0x00,
    SPECL_COLD_RESET_BMC       = 0x01,
    SPECL_COLD_RESET_SP        = 0x02,
    SPECL_WARM_RESET_SP        = 0x03,
    SPECL_PERSISTENT_RESET_SP  = 0x04,
    SPECL_RESET_TYPE_INVALID   = 0xFF
} SPECL_RESET_TYPE;


/******************************************************************************
 *                      T E M P E R A T U R E   T Y P E S
 ******************************************************************************/

#define MAX_TEMPERATURE_COUNT   1

/***************************
 * SPECL_TEMPERATURE_STATUS
 ***************************/
typedef struct _SPECL_TEMPERATURE_STATUS
{
    SPECL_ERROR             transactionStatus;
    SMB_DEVICE              smbDevice;
    UCHAR                   TemperatureMSB;
    UCHAR                   TemperatureLSB;
    EMCPAL_LARGE_INTEGER    timeStamp;
} SPECL_TEMPERATURE_STATUS, *PSPECL_TEMPERATURE_STATUS;

/***************************
 * SPECL_TEMPERATURE_SUMMARY
 ***************************/
typedef struct _SPECL_TEMPERATURE_SUMMARY
{
    BOOL                        sfiMaskStatus;
    ULONG                       temperatureCount;
    SPECL_TEMPERATURE_STATUS    temperatureStatus[MAX_TEMPERATURE_COUNT];
} SPECL_TEMPERATURE_SUMMARY, *PSPECL_TEMPERATURE_SUMMARY;



/******************************************************************************
 *                        M I N I S E T U P   T Y P E S
 ******************************************************************************/

/***************************
 * SPECL_MINISETUP_PROGRESS
 ***************************/
typedef enum _SPECL_MINISETUP_PROGRESS
{
    MINISETUP_NO_STATUS     = 0,
    MINISETUP_IO_POWERDOWN  = 1,
    MINISETUP_IN_PROGRESS   = 2,
    MINISETUP_FINISHED      = 3,
    MINISETUP_COMPLETED     = 4,
} SPECL_MINISETUP_PROGRESS;

/**************************
 * SPECL_MINISETUP_SUMMARY
 **************************/
typedef struct _SPECL_MINISETUP_SUMMARY
{
    BOOL                        sfiMaskStatus;
    SPECL_ERROR                 transactionStatus;
    SPECL_MINISETUP_PROGRESS    flareSysprepStatus;
    SPECL_MINISETUP_PROGRESS    utilitySysprepStatus;
} SPECL_MINISETUP_SUMMARY, *PSPECL_MINISETUP_SUMMARY;



/******************************************************************************
 *                             T I M E   T Y P E S
 ******************************************************************************/

/*****************
 * Timestamp Data
 *****************/
typedef struct
{
    ULONG                  tickIncrement; /* From EmcpalQueryTimeIncrement */
    EMCPAL_LARGE_INTEGER          currTickCount; /* From EmcpalQueryTickCount */
} SPECL_TIME_DATA, *PSPECL_TIME_DATA;

/*******************
 * Timescale setting
 *******************/
typedef enum _SPECL_TIMESCALE_UNIT
{
    SPECL_1_SECOND_UNITS = 1,
    SPECL_100_MILLI_SEC_UNITS,
    SPECL_10_MILLI_SEC_UNITS,
    SPECL_1_MILLI_SEC_UNITS,

    //The remaining are not supported if the granulatity of the clock is 1 millisecond
    SPECL_100_MICRO_SEC_UNITS,
    SPECL_10_MICRO_SEC_UNITS,
    SPECL_1_MICRO_SEC_UNITS,
    SPECL_100_NANO_SEC_UNITS
} SPECL_TIMESCALE_UNIT;


/******************************************************************************
 *                          N T - S T A T U S   C O D E S
 ******************************************************************************/


/******************************************
    ---------------------------------------
    | 31 30 | 29 | 28  -  16  | 15  -  0  |
    ---------------------------------------
    | Sev   | C  |  Facility  |    Code   |
    ---------------------------------------

    Severity
    --------
    STATUS_SEVERITY_SUCCESS         - 0x0
    STATUS_SEVERITY_INFORMATIONAL   - 0x1
    STATUS_SEVERITY_WARNING         - 0x2
    STATUS_SEVERITY_ERROR           - 0x3

    Custom
    -------
    * Asserted to indicate NTSTATUS value is specific to a particular component

    Facility
    ---------
    * Defines the particular facility (component) that generated the error

    Code
    -----
    * Specific error code

*******************************************/
/* ASCII letters 'S' 'P' 'E' 'C' 'L' added together
 */
#define SPECL_FACILITY_CODE 0x177
#define SPECL_CUSTOM_COMPONENT  1

/* To make sure user-space clients can use SPECL's transactions status codes
 */
#ifndef STATUS_SEVERITY_SUCCESS
#define STATUS_SEVERITY_SUCCESS          0x0
#endif
#ifndef STATUS_SEVERITY_INFORMATIONAL
#define STATUS_SEVERITY_INFORMATIONAL    0x1
#endif
#ifndef STATUS_SEVERITY_WARNING
#define STATUS_SEVERITY_WARNING          0x2
#endif
#ifndef STATUS_SEVERITY_ERROR
#define STATUS_SEVERITY_ERROR            0x3
#endif

#define SPECL_SEV_INFORMATIONAL (STATUS_SEVERITY_INFORMATIONAL << 30)
#define SPECL_SEV_WARNING       (STATUS_SEVERITY_WARNING << 30)
#define SPECL_SEV_ERROR         (STATUS_SEVERITY_ERROR << 30)

#define SPECL_BASE_CODE ((SPECL_CUSTOM_COMPONENT << 29) | (SPECL_FACILITY_CODE << 16))

#define STATUS_SPECL_TRANSACTION_NOT_YET_STARTED    (EMCPAL_STATUS)(SPECL_SEV_INFORMATIONAL | SPECL_BASE_CODE | 0x0001)
#define STATUS_SPECL_TRANSACTION_IN_PROGRESS        (EMCPAL_STATUS)(SPECL_SEV_INFORMATIONAL | SPECL_BASE_CODE | 0x0002)
#define STATUS_SPECL_TRANSACTION_PENDING            (EMCPAL_STATUS)(SPECL_SEV_INFORMATIONAL | SPECL_BASE_CODE | 0x0003)
#define STATUS_SPECL_INVALID_CHECKSUM               (EMCPAL_STATUS)(SPECL_SEV_ERROR | SPECL_BASE_CODE | 0x0003)



#pragma pack()
#endif
/***************************************************************************
 * END specl_types.h
 ***************************************************************************/
