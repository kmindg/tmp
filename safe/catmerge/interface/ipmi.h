#ifndef _IPMI_H_
#define _IPMI_H_

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Copyright (C) EMC Corporation, 2012
// All rights reserved.
// Licensed material -- property of EMC
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//++
// File Name:
//      ipmi.h
//
// Contents:
//      IPMI interface definitions.
//
// Revision History:
//      21-Feb-12   Sri             Created. 
//
// Notes:
//      Taken from shared Symmetrix file.
//--

//++
// Includes
//--
// #include "environment.h"
#include "EmcPAL.h"

//++
//.End Includes
//--

//++
// Constants
//--

//++
// IPMI Channel numbers.
//--
#define IPMI_CHANNEL_IPMB                       0x00
#define IPMI_CHANNEL_LAN                        0x01
#define IPMI_CHANNEL_SERIAL                     0x02
#define IPMI_CHANNEL_KCS                        0x03
// The interface to identify the current channel that the command is being received from
#define IPMI_CHANNEL_PRESENT                    0x0E
// Normally it is the KCS
#define IPMI_CHANNEL_SYSTEM                     0x0F

//++
// IPMI LUN numbers. Two bits.
//--
#define IPMI_LUN_BMC                            0x00
#define IPMI_LUN_HOST                           0x02
#define IPMI_LUN_IPMB                           0x02


//++
// BMC SLAVE ADDRESS
//--
#define IPMI_SLAVE_ADDR                         0x20

//++
// BMC SLAVE ADDRESS For Phobos
//--
#define IPMI_P_SLAVE_ADDRESS_A                  0x20
#define IPMI_P_SLAVE_ADDRESS_B                  0x1E

//++
// IPMI request netfn codes.  The response codes are 'odd' values corresponding to the 
// request codes.
//--
#define IPMI_NETFN_CHASSIS                      0x00
#define IPMI_NETFN_BRIDGE                       0x02
#define IPMI_NETFN_SENSOR                       0x04
#define IPMI_NETFN_APP                          0x06
#define IPMI_NETFN_FIRMWARE                     0x08
#define IPMI_NETFN_STORAGE                      0x0A
#define IPMI_NETFN_TRANSPORT                    0x0C
#define IPMI_NETFN_OEM                          0x30

//++
// Chassis command codes (netfn = 0x00)
// --
#define IPMI_CMD_GET_CHASSIS_STATUS             0x01
#define IPMI_CMD_CHASSIS_CONTROL                0x02

//++
// Sensor command codes (netfn = 0x4)
//--
#define IPMI_CMD_GET_EVENT_RECEIVER             0x01
#define IPMI_CMD_GET_DEVICE_SDR_INFO            0x20
#define IPMI_CMD_GET_DEVICE_SDR                 0x21
#define IPMI_CMD_RESERVE_DEVICE_SDR_REPOSITORY  0x22
#define IPMI_CMD_SET_SENSOR_HYSTERESIS          0x24
#define IPMI_CMD_GET_SENSOR_HYSTERESIS          0x25
#define IPMI_CMD_SET_SENSOR_THRESHOLD           0x26
#define IPMI_CMD_GET_SENSOR_THRESHOLD           0x27
#define IPMI_CMD_SET_SENSOR_EVENT_ENABLE        0x28
#define IPMI_CMD_GET_SENSOR_EVENT_ENABLE        0x29
#define IPMI_CMD_GET_SENSOR_EVENT_STATUS        0x2B
#define IPMI_CMD_GET_SENSOR_READING             0x2D

//++
// App command codes (netfn = 0x6)
//--
#define IPMI_CMD_GET_DEVICE_ID                  0x01
#define IPMI_CMD_COLD_RESET                     0x02
#define IPMI_CMD_WARM_RESET                     0x03
#define IPMI_CMD_GET_SELF_TEST_RESULTS          0x04
#define IPMI_CMD_GET_DEVICE_GUID                0x08
#define IPMI_CMD_GET_NETFN_SUPPORT              0x09
#define IPMI_CMD_GET_COMMAND_SUPPORT            0x0A
#define IPMI_CMD_GET_COMMAND_SUBFUNCTION_SUPPORT 0x0B
#define IPMI_CMD_GET_CONFIGURABLE_COMMANDS      0x0C
#define IPMI_CMD_GET_CONFIGURABLE_SUBFUNCTION_COMMANDS 0x0D
#define IPMI_CMD_RESET_WATCHDOG_TIMER           0x22
#define IPMI_CMD_SET_WATCHDOG_TIMER             0x24
#define IPMI_CMD_GET_WATCHDOG_TIMER             0x25
#define IPMI_CMD_SET_GLOBAL_ENABLES             0x2E
#define IPMI_CMD_GET_GLOBAL_ENABLES             0x2F
#define IPMI_CMD_CLEAR_MESSAGE_FLAGS            0x30
#define IPMI_CMD_GET_MESSAGE_FLAGS              0x31
#define IPMI_CMD_ENABLE_MESSAGE_CHANNEL_RECEIVE 0x32
#define IPMI_CMD_GET_MESSAGE                    0x33
#define IPMI_CMD_SEND_MESSAGE                   0x34
#define IPMI_CMD_READ_EVENT_MESSAGE_BUFFER      0x35
#define IPMI_CMD_GET_CHANNEL_INFO               0x42
#define IPMI_CMD_GET_CHANNEL_INFO               0x42

#define IPMI_CMD_SET_SYSTEM_INFO_PARAMETERS     0x58
#define IPMI_CMD_GET_SYSTEM_INFO_PARAMETERS     0x59

#define IPMI_CMD_SET_COMMAND_ENABLES            0x60
#define IPMI_CMD_GET_COMMAND_ENABLES            0x61
#define IPMI_CMD_SET_COMMAND_SUBFUNCTION_ENABLES 0x62
#define IPMI_CMD_GET_COMMAND_SUBFUNCTION_ENABLES 0x63
#define IPMI_CMD_GET_OEM_NETFN_IANA_SUPPORT     0x64

//++
// Firmware command codes (netfn = 0x08)
// --
#define IPMI_CMD_GET_FW_REVISION                0x01

//++
// Storage (SDR) command codes (netfn = 0x0A)
// --
#define IPMI_CMD_GET_SDR_REPOSITORY_INFO        0x20
#define IPMI_CMD_GET_SDR_REPOSITORY_ALLOC_INFO  0x21
#define IPMI_CMD_GET_SDR                        0x23
#define IPMI_CMD_GET_SDR_REPOSITORY_TIME        0x28


//++
// Storage (SEL) command codes (netfn = 0x0A)
//--
#define IPMI_CMD_GET_SEL_INFO                   0x40
#define IPMI_CMD_GET_SEL_ALLOCATION_INFO        0x41
#define IPMI_CMD_RESERVE_SEL                    0x42
#define IPMI_CMD_GET_SEL_ENTRY                  0x43
#define IPMI_CMD_ADD_SEL_ENTRY                  0x44
#define IPMI_CMD_PARTIAL_ADD_SEL_ENTRY          0x45
#define IPMI_CMD_DEL_SEL_ENTRY                  0x46
#define IPMI_CMD_CLEAR_SEL                      0x47
#define IPMI_CMD_GET_SEL_TIME                   0x48
#define IPMI_CMD_SET_SEL_TIME                   0x49
#define IPMI_CMD_GET_AUX_LOG_STATUS             0x5A
#define IPMI_CMD_SET_AUX_LOG_STATUS             0x5B
#define IPMI_CMD_GET_SEL_TIME_UTC_OFFSET        0x5C
#define IPMI_CMD_SET_SEL_TIME_UTC_OFFSET        0x5D

//++
// Storage (FRU) command codes (netfn = 0xA)
//--
#define IPMI_CMD_GET_FRU_INVENTORY_AREA_INFO    0x10
#define IPMI_CMD_READ_FRU_DATA                  0x11
#define IPMI_CMD_WRITE_FRU_DATA                 0x12
#define IPMI_CMD_MASTER_WRITE_READ              0x52

//++
// Common OEM command codes (netfn = 0x30)
//--
#define IPMI_CMD_NVRAM_READ                     0x11
#define IPMI_CMD_NVRAM_WRITE                    0x12
#define IPMI_CMD_GET_EXTEND_LOG_HEADER          0x20
#define IPMI_CMD_GET_EXTEND_LOG_DATA            0x21

//++
// Transformers OEM command codes (netfn = 0x30)
//--
#define IPMI_CMD_GET_DELAYED_SHTDN_TMR          0x0B
#define IPMI_CMD_SET_DELAYED_SHTDN_TMR          0x0C
#define IPMI_CMD_SERIAL_MUX                     0x18
#define IPMI_CMD_BMC_SSP_RESET_CMD              0x24
#define IPMI_CMD_I2C_ARBITRATION_CMD            0x25
#define IPMI_CMD_OEM_SSP_RESET                  0x26
#define IPMI_CMD_ETHS_SET_SWITCH_STATUS_CMD     0x27
#define IPMI_CMD_ETHS_GET_SWITCH_STATUS_CMD     0x28
#define IPMI_CMD_GET_BMC_PRIMARY_LAN_INTERFACE  0x2B
#define IPMI_CMD_SET_BMC_PRIMARY_LAN_INTERFACE  0x2C
#define IPMI_CMD_TWIS_CONFIGURE_BMC_POLLING     0x40
#define IPMI_CMD_TWIS_QUERY_BMC_POLLING         0x41
#define IPMI_CMD_CHAINED_I2C_RAW                0x48
#define IPMI_CMD_PEER_CACHE_UPDATE              0x60
#define IPMI_CMD_GET_PEER_BMC_STATUS            0x65
#define IPMI_CMD_SLIC_CONTROL                   0x70
#define IPMI_CMD_ISSUE_SMI                      0x7F
#define IPMI_CMD_SET_FAN_PWM                    0x81
#define IPMI_CMD_GET_FAN_PWM                    0x82
#define IPMI_CMD_SSP_POWER_RESET                0x83
#define IPMI_CMD_BATTERY_PASSTHROUGH            0x89
#define IPMI_CMD_FWU_DEVICE_STATUS              0x92
#define IPMI_CMD_SET_LED_BLINK_RATE             0x96
#define IPMI_CMD_GET_LED_BLINK_RATE             0x97
#define IPMI_CMD_MFB_CONFIG                     0x9A
#define IPMI_CMD_GET_FAN_RPM                    0xA0
#define IPMI_CMD_SET_FAN_RPM                    0xA1
#define IPMI_CMD_GET_ADAPTIVE_COOLING_CONFIG    0xA4
#define IPMI_CMD_SET_ADAPTIVE_COOLING_CONFIG    0xA5
#define IPMI_CMD_SET_CMI_WD_CONFIG              0xAA
#define IPMI_CMD_GET_CMI_WD_STATUS              0xAB
#define IPMI_CMD_GET_BIST_REPORT                0xB0
#define IPMI_CMD_GET_BATTERY_STATUS             0xB2
#define IPMI_CMD_SET_BATTERY_ENABLE_STATE       0xB3
#define IPMI_CMD_SET_BATTERY_REQUIREMENTS       0xB4
#define IPMI_CMD_CLEAR_BATTERY_FAULTS           0xB5
#define IPMI_CMD_GET_BEACHCOMBER_LED_STATE      0xBA
#define IPMI_CMD_SET_BEACHCOMBER_LED_STATE      0xBB
#define IPMI_CMD_RUN_BATTERY_SELF_TEST          0xBC
#define IPMI_CMD_GET_BATTERY_REQUIREMENTS       0xBD
#define IPMI_CMD_VAULT_DONE                     0xBE
#define IPMI_CMD_GET_BMC_GPIO_STATUS            0xC3
#define IPMI_CMD_SET_BMC_GPIO_STATUS            0xC4
#define IPMI_CMD_TOGGLE_BEM_SXP_CONSOLE         0xCE
#define IPMI_CMD_GET_SPS_STATUS                 0xD0
#define IPMI_CMD_SET_SPS_STATUS                 0xD1

//++
// Moons OEM command codes (netfn = 0x30)
//--
#define IPMI_CMD_P_INJECT_SNSR_READING          0x04
#define IPMI_CMD_P_SET_DELAYED_SHTDN_TMR        0x30
#define IPMI_CMD_P_GET_DELAYED_SHTDN_TMR        0x31
#define IPMI_CMD_P_SET_AUTOMATIC_FAN_CTRL       0x32
#define IPMI_CMD_P_GET_AUTOMATIC_FAN_CTRL       0x33
#define IPMI_CMD_P_SET_FAN_RPM                  0x34
#define IPMI_CMD_P_GET_FAN_RPM                  0x35
#define IPMI_CMD_P_INITIATE_BIST                0x44
#define IPMI_CMD_P_GET_BIST_RESULT              0x46
#define IPMI_CMD_P_SET_UART_MUX                 0x60
#define IPMI_CMD_P_GET_UART_MUX                 0x61
#define IPMI_CMD_P_SET_BMC_LAN_INTERFACE        0x79
#define IPMI_CMD_P_GET_BMC_LAN_INTERFACE        0x7A
#define IPMI_CMD_P_SLIC_CONTROL                 0x81
#define IPMI_CMD_P_SET_MFB_CONFIG               0x82
#define IPMI_CMD_P_GET_MFB_CONFIG               0x83
#define IPMI_CMD_P_ISSUE_SMI                    0x86
#define IPMI_CMD_P_SET_CMI_WD_CONFIG            0x87
#define IPMI_CMD_P_GET_CMI_WD_STATUS            0x88
#define IPMI_CMD_P_RESUME_CACHE_UPDATE          0x90
#define IPMI_CMD_P_OEM_RESET                    0xA0
#define IPMI_CMD_P_VAULT_DONE                   0xA1
#define IPMI_CMD_P_CHAINED_I2C_RAW              0xB2
#define IPMI_CMD_P_SET_BATTERY_ENABLE_STATE     0xC0
#define IPMI_CMD_P_SET_BATTERY_REQUIREMENTS     0xC2
#define IPMI_CMD_P_GET_BATTERY_REQUIREMENTS     0xC3
#define IPMI_CMD_P_GET_BATTERY_STATUS           0xC4
#define IPMI_CMD_P_CLEAR_BATTERY_FAULTS         0xC5
#define IPMI_CMD_P_SET_VAULT_MODE               0xCA
#define IPMI_CMD_P_GET_VAULT_MODE               0xCB
#define IPMI_CMD_P_SET_FRU_LED_STATE            0xD0
#define IPMI_CMD_P_GET_FRU_LED_STATE            0xD1
#define IPMI_CMD_P_RETRIEVE_DEBUG_LOGS          0xF5

//++
// Transport command codes (netfn = 0x0C)
//--
#define IPMI_CMD_SET_LAN_CONFIG                 0x01
#define IPMI_CMD_GET_LAN_CONFIG                 0x02

//++
// DONT_CARE command code DO NOT SEND TO BMC
//--
#define IPMI_CMD_DONT_CARE                      0xFF


//++
// Generic IPMI Completion Codes 0x00, 0xC0-0xFFh
//--
#define IPMI_CC_SUCCESS                         0x00
#define IPMI_CC_ERROR_GENERIC_RANGE_START       0xC0
#define IPMI_CC_ERROR_NODE_BUSY                 0xC0
#define IPMI_CC_ERROR_INVALID_COMMAND           0xC1
#define IPMI_CC_ERROR_INVALID_LUN               0xC2
#define IPMI_CC_ERROR_TIMEOUT                   0xC3
#define IPMI_CC_ERROR_OUT_OF_SPACE              0xC4
#define IPMI_CC_ERROR_INVALID_RESERVATION_ID    0xC5
#define IPMI_CC_ERROR_MESSAGE_TRUNCATED         0xC6
#define IPMI_CC_ERROR_INVALID_REQUEST_LENGTH    0xC7
#define IPMI_CC_ERROR_REQUEST_TOO_LONG          0xC8
#define IPMI_CC_ERROR_PARAMETER_OUT_OF_RANGE    0xC9
#define IPMI_CC_ERROR_INVALID_DATA_LENGTH       0xCA
#define IPMI_CC_ERROR_SDR_ENTRY_NOT_FOUND       0xCB
#define IPMI_CC_ERROR_INVALID_DATA_FIELD        0xCC
#define IPMI_CC_ERROR_ILLEGAL_COMMAND           0xCD
#define IPMI_CC_ERROR_NO_RESPONSE               0xCE
#define IPMI_CC_ERROR_DUPLICATE_REQUEST         0xCF
#define IPMI_CC_ERROR_SDR_IN_UPDATE_MODE        0xD0
#define IPMI_CC_ERROR_DEVICE_IN_FW_UPDATE_MODE  0xD1
#define IPMI_CC_ERROR_INIT_IN_PROGRESS          0xD2
#define IPMI_CC_ERROR_DESTINATION_UNAVAILABLE   0xD3
#define IPMI_CC_ERROR_INSUFFICIENT_PRIVILEGE    0xD4
#define IPMI_CC_ERROR_INVALID_STATE             0xD5
#define IPMI_CC_ERROR_SUB_FN_DISABLED           0xD6
#define IPMI_CC_ERROR_UNSPECIFIED               0xFF
#define IPMI_CC_ERROR_GENERIC_RANGE_END         0xFF


//++
// Command Specific IPMI Completion Codes 0x80-0xBE
//--
#define IPMI_CC_ERROR_CMD_SPECIFIC_RANGE_START  0x80
#define IPMI_CC_ERROR_CMD_SPECIFIC_RANGE_END    0xBE

//++
// IPMI_CMD_GET_MESSAGE
//--
#define IPMI_CC_DATA_NOT_AVAILABLE              0x80

//++
// IPMI_CMD_SEND_MESSAGE
//--
#define IPMI_CC_INVALID_SESSION_HANDLE          0x80
//#define IPMI_CC_LOST_ARBITRATION                0x81
//#define IPMI_CC_BUS_ERROR                       0x82
//#define IPMI_CC_NAK_ON_WRITE                    0x83

//++
// IPMI_CMD_MASTER_WRITE_READ
//--
#define IPMI_CC_LOST_ARBITRATION                0x81
#define IPMI_CC_BUS_ERROR                       0x82
#define IPMI_CC_NAK_ON_WRITE                    0x83
#define IPMI_CC_TRUNCATED_READ                  0x84

//++
// IPMI_CMD_CHAINED_I2C_RAW
//--
#define IPMI_CC_ARBITRATION_TIMEOUT             0x80
#define IPMI_CC_ARBITRATION_STUCK               0x81


//++
//  BMC CMI watchdog
//--
#define BMC_CMI_WD_TIMEOUT                      0x02
#define BMC_CMI_WD_PECIFAULT                    0x04
#define BMC_CMI_WD_ARM                          0x10
#define BMC_CMI_WD_ENABLE                       0x40

/* BMC CMI watchdog status */
#define BMC_CMI_WD_STS_PAUSED                   0x01
#define BMC_CMI_WD_STS_TIMEOUT                  0x02
#define BMC_CMI_WD_STS_PECIFAULT                0x04
#define BMC_CMI_WD_STS_ARMED                    0x10
#define BMC_CMI_WD_STS_RUNNING                  0x20
#define BMC_CMI_WD_STS_ENABLED                  0x40
#define BMC_CMI_WD_STS_AVAILABLE                0x80

//++
// Sub commands for 'Retrieve Debug Logs' command (NetFn=0x30, Cmd=0xF5) in Phobos. 
// The sub-command is the first byte in the 'data' portion of the IPMI command.
//--
#define IPMI_P_SC_DEBUG_LOGS_TFTP_TRANSFER_STATUS    0x00
#define IPMI_P_SC_DEBUG_LOGS_TFTP_START_TRANSFER     0x01
#define IPMI_P_SC_DEBUG_LOGS_IPMI_TRANSFER_STATUS    0x10
#define IPMI_P_SC_DEBUG_LOGS_IPMI_START_COLLECTION   0x11
#define IPMI_P_SC_DEBUG_LOGS_IPMI_READ               0x12
#define IPMI_P_SC_DEBUG_LOGS_IPMI_FREE_RESOURCES     0x13

//++
// Name:
//      IPMI_P_DEBUG_LOGS_TFTP_TRANSFER_STATUS
//
// Description:
//      Values for the response byte returned by the 'Debug Logs Tftp Transfer Status' IPMI command.
//
//--
typedef enum _IPMI_P_DEBUG_LOGS_TFTP_TRANSFER_STATUS
{
    IPMI_P_DEBUG_LOGS_TFTP_TRANSFER_STATUS_IDLE             = 0x00,
    IPMI_P_DEBUG_LOGS_TFTP_TRANSFER_STATUS_IN_PROGRESS,     // 0x1
    IPMI_P_DEBUG_LOGS_TFTP_TRANSFER_STATUS_COMPLETE,        // 0x2
    IPMI_P_DEBUG_LOGS_TFTP_TRANSFER_STATUS_FAILED,          // 0x3
}
IPMI_P_DEBUG_LOGS_TFTP_TRANSFER_STATUS, *PIPMI_P_DEBUG_LOGS_TFTP_TRANSFER_STATUS;

//++
// Name:
//      IPMI_P_DEBUG_LOGS_IPMI_TRANSFER_STATUS
//
// Description:
//      Values for the response byte returned by the 'Debug Logs Tftp Transfer Status' IPMI command.
//
//--
typedef enum _IPMI_P_DEBUG_LOGS_IPMI_TRANSFER_STATUS
{
    // 0x00: No log files available for reading.
    IPMI_P_DEBUG_LOGS_IPMI_TRANSFER_STATUS_NO_DATA          = 0x00,

    // 0x1: BMC is preparing the log files.
    IPMI_P_DEBUG_LOGS_IPMI_TRANSFER_STATUS_IN_PROGRESS,     

    // 0x2: The log files are available for reading.
    IPMI_P_DEBUG_LOGS_IPMI_TRANSFER_STATUS_COMPLETE,        

    // 0x3: The process of gathering log files in the BMC failed.
    IPMI_P_DEBUG_LOGS_IPMI_TRANSFER_STATUS_FAILED,          
}
IPMI_P_DEBUG_LOGS_IPMI_TRANSFER_STATUS, *PIPMI_P_DEBUG_LOGS_IPMI_TRANSFER_STATUS;

//++
// Name:
//      IPMI_P_FRU_IDS
//
// Description:
//      FRU ID values for Phobos BMC
//--
typedef enum _IPMI_P_FRU_IDS
{
    IPMI_FRU_LOCAL_SP           = 0x00,
    IPMI_FRU_PEER_SP            = 0x01,
    IPMI_FRU_CHASSIS            = 0x02, // Midplane
    IPMI_FRU_LOCAL_MGMT         = 0x03,
    IPMI_FRU_PEER_MGMT          = 0x04,
    IPMI_FRU_PS_0               = 0x05,
    IPMI_FRU_PS_1               = 0x06,
    IPMI_FRU_PS_2               = 0x07,
    IPMI_FRU_PS_3               = 0x08,
    IPMI_FRU_BATTERY_0          = 0x09,
    IPMI_FRU_BATTERY_1          = 0x0A,
    IPMI_FRU_BATTERY_2          = 0x0B,
    IPMI_FRU_BATTERY_3          = 0x0C,
    IPMI_FRU_PEER_BATTERY_0     = 0x0D,
    IPMI_FRU_PEER_BATTERY_1     = 0x0E,
    IPMI_FRU_PEER_BATTERY_2     = 0x0F,
    IPMI_FRU_PEER_BATTERY_3     = 0x10,
    IPMI_FRU_SLIC_0             = 0x11,
    IPMI_FRU_SLIC_1             = 0x12,
    IPMI_FRU_SLIC_2             = 0x13,
    IPMI_FRU_SLIC_3             = 0x14,
    IPMI_FRU_SLIC_4             = 0x15,
    IPMI_FRU_SLIC_5             = 0x16,
    IPMI_FRU_SLIC_6             = 0x17,
    IPMI_FRU_SLIC_7             = 0x18,
    IPMI_FRU_SLIC_8             = 0x19,
    IPMI_FRU_SLIC_9             = 0x1A,
    IPMI_FRU_SLIC_10            = 0x1B,
    IPMI_FRU_SLIC_11            = 0x1C,
    IPMI_FRU_SLIC_12            = 0x1D,
    IPMI_FRU_SLIC_13            = 0x1E,
    IPMI_FRU_SLIC_14            = 0x1F,
    IPMI_FRU_SLIC_15            = 0x20,
    IPMI_FRU_DRIVE_IO_CARD      = 0x21,
    IPMI_FRU_CM_0               = 0x22,
    IPMI_FRU_CM_1               = 0x23,
    IPMI_FRU_CM_2               = 0x24,
    IPMI_FRU_CM_3               = 0x25,

    IPMI_FRU_VEEPROM            = 0xFE
} IPMI_P_FRU_IDS;

//++
// Name:
//      IPMI_P_FRU_DEV_TYPE
//
// Description:
//      Device types associated with FRU ID values for Phobos BMC
//--
typedef enum _IPMI_P_FRU_DEV_TYPE
{
    IPMI_FRU_DEV_RESUME         = 0x00,
    IPMI_FRU_DEV_CMD            = 0x01,
    IPMI_FRU_DEV_PS_MCU         = 0x02,
    IPMI_FRU_DEV_BAT_MCU        = 0x03,
    IPMI_FRU_DEV_MGMT_MCU       = 0x04,
    IPMI_FRU_DEV_TEMP           = 0x05,
    IPMI_FRU_DEV_PROC           = 0x06,
    IPMI_FRU_DEV_DIMM           = 0x07,
    IPMI_FRU_DEV_PLX            = 0x08,
    IPMI_FRU_DEV_SFP            = 0x09,
    IPMI_FRU_DEV_SFP_DIAG       = 0x0A,
    IPMI_FRU_DEV_GPIO_EXP       = 0x0B,
    IPMI_FRU_DEV_LED_EXP        = 0x0C,
    IPMI_FRU_DEV_EMB_FAN        = 0x0D,
    IPMI_FRU_DEV_CLK            = 0x0E,
    IPMI_FRU_DEV_PCH            = 0x0F,
    IPMI_FRU_DEV_BMC_SPI        = 0x10,
    IPMI_FRU_DEV_HOST_SPI       = 0x11,
    IPMI_FRU_DEV_CM_MCU         = 0x12
} IPMI_P_FRU_DEV_TYPE;

//++
// Name:
//      IPMI_P_FRU_BMC_FW_INDEX
//
// Description:
//      Index values associated with the BMC Device ID for Phobos BMC
//          the full SPI value also applies to the Host SPI flash
//--
typedef enum _IPMI_P_FRU_BMC_FW_INDEX
{
    IPMI_FRU_DEV_BMC_FW_MAIN        = 0x00,
    IPMI_FRU_DEV_BMC_FW_BOOT        = 0x01,
    IPMI_FRU_DEV_BMC_FW_SSP         = 0x02,
    IPMI_FRU_DEV_BMC_FW_FULL_SPI    = 0x07
} IPMI_P_FRU_BMC_FW_INDEX;

//++
// Name:
//      IPMI_P_FRU_LED_TYPE
//
// Description:
//      LED types associated with FRU ID values for Phobos BMC
//--
typedef enum _IPMI_P_FRU_LED_TYPE
{
    IPMI_FRU_LED_MARKER         = 0x00,
    IPMI_FRU_LED_UNSAFE_TO_REM  = 0x01,
    IPMI_FRU_LED_FAN_0          = 0x02,
    IPMI_FRU_LED_FAN_1          = 0x03,
    IPMI_FRU_LED_FAN_2          = 0x04,
    IPMI_FRU_LED_FAN_3          = 0x05,
    IPMI_FRU_LED_FAN_4          = 0x06,
    IPMI_FRU_LED_FAN_5          = 0x07,
    IPMI_FRU_LED_FAN_6          = 0x08,
    IPMI_FRU_LED_FAN_7          = 0x09
} IPMI_P_FRU_LED_TYPE;

//++
// Name:
//      IPMI_P_FRU_LED_COLOR_TYPE
//
// Description:
//      LED Color types associated with FRU ID values for Phobos BMC
//--
typedef enum _IPMI_P_FRU_LED_COLOR_TYPE
{
    IPMI_FRU_LED_COLOR_DEFAULT      = 0x00,
    IPMI_FRU_LED_COLOR_GREEN        = 0x01,
    IPMI_FRU_LED_COLOR_BLUE         = 0x02,
    IPMI_FRU_LED_COLOR_AMBER        = 0x03,
    IPMI_FRU_LED_COLOR_WHITE        = 0x04,
    IPMI_FRU_LED_COLOR_NOT_CONFIG   = 0xFF
} IPMI_P_FRU_LED_COLOR_TYPE;

//++
// Name:
//      IPMI_P_LED_BLINK_RATE
//
// Description:
//      LED blink rate definitions for Phobos BMC
//--
typedef enum _IPMI_P_LED_BLINK_RATE
{
   IPMI_P_LED_FW_CTRL           = 0x00,
   IPMI_P_LED_ON                = 0x01,
   IPMI_P_LED_OFF               = 0x02,
   IPMI_P_LED_BLINK_QUARTER_HZ  = 0x03,
   IPMI_P_LED_BLINK_HALF_HZ     = 0x04,
   IPMI_P_LED_BLINK_1_HZ        = 0x05,
   IPMI_P_LED_BLINK_2_HZ        = 0x06,
   IPMI_P_LED_BLINK_4_HZ        = 0x07,
   IPMI_P_LED_INVALID           = 0xFF
} IPMI_P_LED_BLINK_RATE, *PIPMI_P_LED_BLINK_RATE;

//++
// Name:
//      IPMI_P_INTERNAL_ERROR_CODE
//
// Description:
//      Internal Error Codes for Phobos BMC
//--
typedef enum _IPMI_P_INTERNAL_ERROR_CODE
{
   IPMI_P_ERR_NO_ERR                = 0x00000000,
   IPMI_P_ERR_GENERIC_FAILURE       = 0x00000001,
   IPMI_P_ERR_NULL_POINTER          = 0x00000002,
   IPMI_P_ERR_FULL_QUEUE            = 0x00000003,
   IPMI_P_ERR_ENTRY_NOT_FOUND       = 0x00000004,
   IPMI_P_ERR_FILE_OPEN             = 0x00000005,
   IPMI_P_ERR_FILE_READ             = 0x00000006,
   IPMI_P_ERR_FILE_WRITE            = 0x00000007,
   IPMI_P_ERR_FILE_SEEK             = 0x00000008,
   IPMI_P_ERR_BAD_PARAMETER         = 0x00000009,
   IPMI_P_ERR_THREAD_CREATE         = 0x0000000A,
   IPMI_P_ERR_DEV_NOT_READY         = 0x0000000B,
   IPMI_P_ERR_MUTEX                 = 0x0000000C,
   IPMI_P_ERR_AIM                   = 0x0000000D,
   IPMI_P_ERR_MEM_ACCESS            = 0x0000000E,
   IPMI_P_ERR_MMAP                  = 0x0000000F,
   IPMI_P_ERR_MALLOC                = 0x00000010,
   IPMI_P_ERR_IOCTL                 = 0x00000011,
   IPMI_P_ERR_BAD_CHECKSUM          = 0x00000012,
   IPMI_P_ERR_DATA_MISCOMPARE       = 0x00000013,
   IPMI_P_ERR_DATA_OUT_OF_SEQ       = 0x00000014,
   IPMI_P_ERR_INVALID_SIZE          = 0x00000015,
   IPMI_P_ERR_NOT_PRESENT           = 0x00000016,
   IPMI_P_ERR_UNSUPPORTED_REQ       = 0x00000017,
   IPMI_P_ERR_NOT_POWERED           = 0x00000018,
   IPMI_P_ERR_UNTRUSTED_DATA        = 0x00000019,
   IPMI_P_ERR_INVALID_CONFIG        = 0x0000001A,
   IPMI_P_ERR_SENSOR_UNAVAIL        = 0x0000001B,
   IPMI_P_ERR_I2C_NOT_STARTED       = 0x00010000,
   IPMI_P_ERR_I2C_IN_PROGRESS       = 0x00010001,
   IPMI_P_ERR_I2C_ARB_STUCK         = 0x00010002,
   IPMI_P_ERR_I2C_ARB_TIMEOUT       = 0x00010003,
   IPMI_P_ERR_I2C_ARB_FREE          = 0x00010004,
   IPMI_P_ERR_I2C_DRV_INIT          = 0x00010005,
   IPMI_P_ERR_I2C_DRV_LOCK          = 0x00010006,
   IPMI_P_ERR_I2C_DRV_UNLOCK        = 0x00010007,
   IPMI_P_ERR_I2C_DRV_NACK          = 0x00010008,
   IPMI_P_ERR_I2C_DRV_ARB_LOS       = 0x00010009,
   IPMI_P_ERR_I2C_DRV_START         = 0x0001000A,
   IPMI_P_ERR_I2C_DRV_STOP          = 0x0001000B,
   IPMI_P_ERR_I2C_DRV_BUSY          = 0x0001000C,
   IPMI_P_ERR_I2C_DRV_TXN_TO        = 0x0001000D,
   IPMI_P_ERR_I2C_DRV_TIMEOUT       = 0x0001000E,
   IPMI_P_ERR_I2C_DRV_BUS_ERR       = 0x0001000F,
   IPMI_P_ERR_I2C_DRV_OVRFLOW       = 0x00010010,
   IPMI_P_ERR_I2C_DRV_RESET         = 0x00010011,
   IPMI_P_ERR_I2C_DRV_PRTCL         = 0x00010012,
   IPMI_P_ERR_I2C_DRV_BAD_SEG       = 0x00010013,
   IPMI_P_ERR_I2C_DRV_STK_DAT       = 0x00010014,
   IPMI_P_ERR_I2C_DRV_STK_CLK       = 0x00010015,
   IPMI_P_ERR_I2C_DRV_PARAM         = 0x00010016,
   IPMI_P_ERR_I2C_DRV_SYS_ERR       = 0x00010017,
   IPMI_P_ERR_I2C_DRV_ADR_NAK       = 0x00010018,
   IPMI_P_ERR_I2C_DRV_NAK_ELS       = 0x00010019,
   IPMI_P_ERR_I2C_DRV_OTHER         = 0x0001001A,
   IPMI_P_ERR_GPIO_LBL_INVLID       = 0x00020000,
   IPMI_P_ERR_GPIO_REV_ACCESS       = 0x00020001,
   IPMI_P_ERR_LED_STS_NOT_SET       = 0x00030000,
   IPMI_P_ERR_NVRAM_ACCESS          = 0x00040000,
   IPMI_P_ERR_PECI_CMD_FAIL         = 0x00050000,
   IPMI_P_ERR_PECI_RETRY            = 0x00050001,
   IPMI_P_ERR_SSP_SEND              = 0x00060000,
   IPMI_P_ERR_SSP_ARB_IN_PRG        = 0x00060001,
   IPMI_P_ERR_SSP_ARB_TIMER         = 0x00060002,
   IPMI_P_ERR_BIST_ALRDY_RNG        = 0x00070000,
   IPMI_P_ERR_BIST_UPDT_CFG_FAIL    = 0x00070001,
   IPMI_P_ERR_BIST_INV_TST_ID       = 0x00070002,
   IPMI_P_ERR_BIST_CPU              = 0x00070003,
   IPMI_P_ERR_BIST_MAX_FAN_SPEED    = 0x00070004,
   IPMI_P_ERR_BIST_SSP_INT          = 0x00070005,
   IPMI_P_ERR_BIST_SSP_SET_GP       = 0x00070006,
   IPMI_P_ERR_BIST_MEM_HIGH         = 0x00070007,
   IPMI_P_ERR_BIST_MEM_LOW          = 0x00070008,
   IPMI_P_ERR_BIST_LOOP_TO          = 0x00070009,
   IPMI_P_ERR_BIST_BBU_TO           = 0x0007000A,
   IPMI_P_ERR_BIST_BBU_UNKNOWN      = 0x0007000B,
   IPMI_P_ERR_BIST_BBU_INSUF_LOAD   = 0x0007000C,
   IPMI_P_ERR_BIST_BBU_FAIL         = 0x0007000D,
   IPMI_P_ERR_BIST_BBU_NOT_SUPP     = 0x0007000E,
   IPMI_P_ERR_BIST_BBU_INSUF_ENERGY = 0x0007000F,
   IPMI_P_ERR_BIST_BBU_NOT_ENABLED  = 0x00070010,
   IPMI_P_ERR_BIST_BBU_ABRT_ERLY    = 0x00070011,
   IPMI_P_ERR_BIST_BBU_STOP_USER    = 0x00070012,
   IPMI_P_ERR_RESET_LOCKED          = 0x00080000,
   IPMI_P_ERR_RESET_FW_UPD          = 0x00080001,
   IPMI_P_ERR_RESET_LOCK_CHK        = 0x00080002,
   IPMI_P_ERR_SEL_REC_NOT_PST       = 0x00090000,
   IPMI_P_ERR_SEL_ENT_NOT_PST       = 0x00090001,
   IPMI_P_ERR_SEL_NO_SPACE          = 0x00090002,
   IPMI_P_ERR_SEL_ADD_REC           = 0x00090003,
   IPMI_P_ERR_SEL_DEL_REC           = 0x00090004,
   IPMI_P_ERR_SEL_INV_REC_FMT       = 0x00090005,
   IPMI_P_ERR_SEL_INV_LENGTH        = 0x00090006,
   IPMI_P_ERR_SEL_INV_OFFSET        = 0x00090007,
   IPMI_P_ERR_ADAP_CL_SEN_RD        = 0x000A0000,
   IPMI_P_ERR_ADAP_CL_STATE_NF      = 0x000A0001,
   IPMI_P_ERR_ADAP_CL_NO_READ       = 0x000A0002,
   IPMI_P_ERR_ADAP_CL_FAULT         = 0x000A0003
} IPMI_P_INTERNAL_ERROR_CODE, *PIPMI_P_INTERNAL_ERROR_CODE;


//++
// LED constants
//--
#define IPMI_LED_BLINK_IN_PHASE                 0x00
#define IPMI_LED_BLINK_OUT_OF_PHASE             0x01
#define IPMI_LED_BLINK_FINAL_STATE_OFF          0x00
#define IPMI_LED_BLINK_FINAL_STATE_ON           0x01
#define IPMI_LED_BLINK_FOREVER                  0xFF

//++
// SEL constants
//--
#define IPMI_SEL_OEM_TS_DATA_LEN                6
#define IPMI_SEL_OEM_NOTS_DATA_LEN              13
// event_data[0] bit mask 
#define IPMI_SEL_DATA_BYTE2_SPECIFIED_MASK      0xC0
#define IPMI_SEL_DATA_BYTE3_SPECIFIED_MASK      0x30
#define IPMI_SEL_EVENT_OFFSET_MASK              0x0F

//++
// Extend Log Data constants
#define IPMI_EXTEND_LOG_DATA_MAX_SIZE           1024
#define IPMI_EXTEND_LOG_DATA_TYPE_ASCII         0x00
// Firmware update log
#define IPMI_EXTEND_LOG_DATA_TYPE_FWULOG        0x01
// Fault/Status Register
#define IPMI_EXTEND_LOG_DATA_TYPE_FAULTREG      0x02
#define IPMI_EXTEND_LOG_DATA_TYPE_HEXDUMP       0xFF

//++
// VEEPROM related constants start.
//--
#define IPMI_VEEPROM_SP_FLAG_MSB_OFFSET         0x00
#define IPMI_VEEPROM_SP_FLAG_LSB_OFFSET         0x80
#define IPMI_VEEPROM_SLIC_FLAG_MSB_OFFSET       0x00
#define IPMI_VEEPROM_SLIC_FLAG_LSB_OFFSET       0x10
#define IPMI_VEEPROM_P_SLIC_FLAG_LSB_OFFSET     0x00
#define IPMI_VEEPROM_QUICKBOOT_MSB_OFFSET       0x01
#define IPMI_VEEPROM_QUICKBOOT_REG1_LSB_OFFSET  0xE0
#define IPMI_VEEPROM_QUICKBOOT_REG2_LSB_OFFSET  0xF0
#define IPMI_VEEPROM_TCOREG_MSB_OFFSET          0x02
#define IPMI_VEEPROM_TCOREG_LSB_OFFSET          0x00
#define IPMI_VEEPROM_BIST_MSB_OFFSET            0x02
#define IPMI_VEEPROM_BIST_LSB_OFFSET            0x20
#define IPMI_VEEPROM_FLTSTS_MSB_OFFSET_1        0x02
#define IPMI_VEEPROM_FLTSTS_REG1_LSB_OFFSET     0x40
//#define IPMI_VEEPROM_FLTSTS_REG2_LSB_OFFSET     0x57
#define IPMI_VEEPROM_FLTSTS_REG3_LSB_OFFSET     0x6E
#define IPMI_VEEPROM_FLTSTS_REG4_LSB_OFFSET     0x85
#define IPMI_VEEPROM_FLTSTS_REG5_LSB_OFFSET     0x9C
#define IPMI_VEEPROM_CPU_STS_REG_LSB_OFFSET     0xA8
#define IPMI_VEEPROM_FLTSTS_MIRROR_FLAG_MSB_OFFSET     0x04
#define IPMI_VEEPROM_FLTSTS_MIRROR_FLAG_LSB_OFFSET     0x40
#define IPMI_VEEPROM_FLTSTS_MIRROR_MSB_OFFSET_1        0x03
#define IPMI_VEEPROM_FLTSTS_MIRROR_REG1_LSB_OFFSET     0x40
//#define IPMI_VEEPROM_FLTSTS_MIRROR_REG2_LSB_OFFSET     0x57
#define IPMI_VEEPROM_FLTSTS_MIRROR_REG3_LSB_OFFSET     0x6E
#define IPMI_VEEPROM_FLTSTS_MIRROR_REG4_LSB_OFFSET     0x85
#define IPMI_VEEPROM_FLTSTS_MIRROR_REG5_LSB_OFFSET     0x9C
#define IPMI_VEEPROM_FLTSTS_REG_DRIVES_LSB_OFFSET      0x50
#define IPMI_VEEPROM_FLTSTS_REG_SLICS_LSB_OFFSET       0x80
#define IPMI_VEEPROM_FLTSTS_REG_PS_LSB_OFFSET          0x88
#define IPMI_VEEPROM_FLTSTS_REG_BATTERY_LSB_OFFSET     0x89
#define IPMI_VEEPROM_FLTSTS_REG_FANS_LSB_OFFSET        0x90
#define IPMI_VEEPROM_FLTSTS_REG_I2C_LSB_OFFSET         0x98
#define IPMI_VEEPROM_FLTSTS_REG_SYSMISC_LSB_OFFSET     0xA0
/* Chaitanya Godbole:-
   Using this flag to allow/disallow the clearing of the fsm-flag. By default it will be commented out,
   so that we will NOT clear the flag & thus not enable the mirroring from the BMC side. Once ESP is done
   with making its changes for looking at/processing the fsmr data, we can uncomment this or completely remove
   the use of this flag, thus in that case, we will enable fsm-flag clearing & thus enable mirroring from
   the BMC side by default.
 */
//#define IPMI_ALLOW_FSMR_FLAG_CLEAR

/* SLIC Flags "scratch-pad bits" are from 0010 to 003F bytes.
 * So, 1st byte (i.e. 0010) is for slic 0
 * 2nd byte (i.e. 0011) is for slic 1 & so on, till byte 11 (i.e. 001A)
 * This setting is stored in the BMC's VEEPROM, so when we powercycle the BMC, we will lose this & it will default to all 0's.
 * Hence we will treat the default state of 0 as "enable the persisted power state", which means now ALL the slics will have 
 * it set to TRUE by default & so if you need to set it to FALSE, we will set the bit in VEEPROM to 1 to reflect that.
 */
#define IPMI_VEEPROM_SLIC_SCRTHPAD_FLAG_S0_S11_MSB_OFFSET   0x00
typedef enum _IPMI_VEEPROM_SLIC_SCRTHPAD_FLAG_LSB_OFFSETS
{
    IPMI_VEEPROM_SLIC_SCRTHPAD_FLAG_S0_LSB_OFFSET           = 0x10,
    IPMI_VEEPROM_SLIC_SCRTHPAD_FLAG_S1_LSB_OFFSET           = 0x11,
    IPMI_VEEPROM_SLIC_SCRTHPAD_FLAG_S2_LSB_OFFSET           = 0x12,
    IPMI_VEEPROM_SLIC_SCRTHPAD_FLAG_S3_LSB_OFFSET           = 0x13,
    IPMI_VEEPROM_SLIC_SCRTHPAD_FLAG_S4_LSB_OFFSET           = 0x14,
    IPMI_VEEPROM_SLIC_SCRTHPAD_FLAG_S5_LSB_OFFSET           = 0x15,
    IPMI_VEEPROM_SLIC_SCRTHPAD_FLAG_S6_LSB_OFFSET           = 0x16,
    IPMI_VEEPROM_SLIC_SCRTHPAD_FLAG_S7_LSB_OFFSET           = 0x17,
    IPMI_VEEPROM_SLIC_SCRTHPAD_FLAG_S8_LSB_OFFSET           = 0x18,
    IPMI_VEEPROM_SLIC_SCRTHPAD_FLAG_S9_LSB_OFFSET           = 0x19,
    IPMI_VEEPROM_SLIC_SCRTHPAD_FLAG_S10_LSB_OFFSET          = 0x1A,
    IPMI_VEEPROM_SLIC_SCRTHPAD_FLAG_INVALID_LSB_OFFSET      = 0xFF //Not sure if assigning it to 0xFF is a good idea or not. But I dont see us using a valid offset of 0xFF in the near-future.
} IPMI_VEEPROM_SLIC_SCRTHPAD_FLAG_LSB_OFFSETS;
#define IPMI_CLEAR_SLIC_PERSISTENT_POWER_STATE_FLAG         0x01 //Write 1 byte, with bit-0 set as 1
#define IPMI_SET_SLIC_PERSISTENT_POWER_STATE_FLAG           0x00 //Write 1 byte, with bit-0 set as 0
#define IPMI_VEEPROM_SP_BOOT_HOLD_FLAG_MSB_OFFSET           0x00
#define IPMI_VEEPROM_SP_BOOT_HOLD_FLAG_LSB_OFFSET           0x84
#define IPMI_CLEAR_HOLD_IN_POST_FLAG                        0x00 //Write 1 byte, with bit-0 set as 0
#define IPMI_SET_HOLD_IN_POST_FLAG                          0x01 //Write 1 byte, with bit-0 set as 1

#define IPMI_VEEPROM_QUICKBOOT_FLAG_LSB_OFFSET              0xE6
#define IPMI_VEEPROM_QUICKBOOT_FLAG_DISABLE                 0x00 //Write 1 byte, with bit-0 set as 0, then 5 bytes set as 0
#define IPMI_VEEPROM_QUICKBOOT_FLAG_ENABLE                  0x01 //Write 1 byte, with bit-0 set as 1, then 5 bytes set as 0
#define IPMI_VEEPROM_QUICKBOOT_FLAG_SIZE                    0x06 //Read/Write 6 bytes total for Flag and Status
#define IPMI_VEEPROM_QUICKBOOT_READ_SIZE                    0x10 //Read/Write 16 bytes at a time
#define IPMI_VEEPROM_QUICKBOOT_REGION_SIZE                  0x20 //Region is 32 bytes

#define IPMI_VEEPROM_P_SLIC_FLAG_REGION_SIZE                0x08
#define IPMI_VEEPROM_FLTSTS_REGION_SIZE                     0x70

#define IPMI_VEEPROM_MAX_WRITE_SIZE                         0x16 //Write 22 bytes
#define IPMI_VEEPROM_MAX_READ_SIZE                          0x17 //Read 23 bytes
#define IPMI_VEEPROM_CSR_READ_SIZE                          0x4  //Read 4 bytes


/************
 * CSR_AGENT
 ************/
typedef enum _CSR_AGENT
{
    CSR_BIOS    = 0x00,
    CSR_POST    = 0x01,
    CSR_OS      = 0x02,
} CSR_AGENT;

/* BIOS CODES */
#define CSR_BIOS_CPU                        0x00000000
#define CSR_BIOS_CPU_ALL_DIMMS              0x00000001
#define CSR_BIOS_CPU_IO_MODULE_0            0x00000002
#define CSR_BIOS_CPU_IO_MODULE_1            0x00000003
#define CSR_BIOS_CPU_IO_MODULE_2            0x00000004
#define CSR_BIOS_CPU_IO_MODULE_3            0x00000005
#define CSR_BIOS_CPU_IO_MODULE_4            0x00000006
#define CSR_BIOS_CPU_IO_MODULE_5            0x00000007
#define CSR_BIOS_CPU_IO_MODULE_6            0x00000008
#define CSR_BIOS_CPU_IO_MODULE_7            0x00000009
#define CSR_BIOS_CPU_IO_MODULE_8            0x0000000A
#define CSR_BIOS_CPU_IO_MODULE_9            0x0000000B
#define CSR_BIOS_CPU_IO_MODULE_10           0x0000000C
#define CSR_BIOS_CPU_IO_MODULE_11           0x0000000D
#define CSR_BIOS_CPU_IO_MODULE_12           0x0000000E
#define CSR_BIOS_CPU_IO_MODULE_13           0x0000000F
#define CSR_BIOS_CPU_IO_MODULE_14           0x00000010
#define CSR_BIOS_CPU_IO_MODULE_15           0x00000011
#define CSR_BIOS_CPU_IO_MODULE_16           0x00000012
#define CSR_BIOS_CPU_IO_MODULE_17           0x00000013
#define CSR_BIOS_CPU_IO_MODULE_18           0x00000014
#define CSR_BIOS_CPU_IO_MODULE_19           0x00000015
#define CSR_BIOS_CPU_IO_MODULE_20           0x00000016
#define CSR_BIOS_CPU_IO_MODULE_21           0x00000017
#define CSR_BIOS_CPU_IO_MODULE_22           0x00000018

/* POST CODES */
#define CSR_POST_INITIALIZING               0x01000000
#define CSR_POST_START                      0x01000001
#define CSR_POST_MOTHERBOARD                0x01000002
#define CSR_POST_IO_MODULE_0                0x01000003
#define CSR_POST_IO_MODULE_1                0x01000004
#define CSR_POST_IO_MODULE_2                0x01000005
#define CSR_POST_IO_MODULE_3                0x01000006
#define CSR_POST_IO_MODULE_4                0x01000007
#define CSR_POST_IO_MODULE_5                0x01000008
#define CSR_POST_IO_MODULE_6                0x01000009
#define CSR_POST_IO_MODULE_7                0x0100000A
#define CSR_POST_IO_MODULE_8                0x0100000B
#define CSR_POST_IO_MODULE_9                0x0100000C
#define CSR_POST_IO_MODULE_10               0x0100000D
#define CSR_POST_IO_MODULE_11               0x0100000E
#define CSR_POST_IO_MODULE_12               0x0100000F
#define CSR_POST_IO_MODULE_13               0x01000010
#define CSR_POST_IO_MODULE_14               0x01000011
#define CSR_POST_IO_MODULE_15               0x01000012
#define CSR_POST_MM_MODULE_A                0x01000013
#define CSR_POST_MM_MODULE_B                0x01000014
#define CSR_POST_PS_0                       0x01000015
#define CSR_POST_PS_1                       0x01000016
#define CSR_POST_BBU_A                      0x01000017
#define CSR_POST_BBU_B                      0x01000018
#define CSR_POST_END                        0x0100001B
#define CSR_POST_ILLEGAL_CONFIGURATION      0x0100001C
#define CSR_POST_SW_IMAGE_CORRUPT           0x0100001D
#define CSR_POST_CAN_NOT_ACCESS_DISK        0x0100001E
#define CSR_POST_BLADE_BEING_SERVICED       0x01000024
#define CSR_POST_MEMORY                     0x01000025
#define CSR_POST_DISK_0                     0x01000026
#define CSR_POST_DISK_1                     0x01000027
#define CSR_POST_DISK_2                     0x01000028
#define CSR_POST_DISK_3                     0x01000029
#define CSR_POST_EHORNET                    0x0100002A
#define CSR_POST_BACK_END_MODULE            0x0100002B
#define CSR_POST_COOLING_MODULE_0           0x0100002C
#define CSR_POST_COOLING_MODULE_1           0x0100002D
#define CSR_POST_COOLING_MODULE_2           0x0100002E
#define CSR_POST_COOLING_MODULE_3           0x0100002F
#define CSR_POST_COOLING_MODULE_4           0x01000030
#define CSR_POST_COOLING_MODULE_5           0x01000031

/*
 * POST Error Codes
 */

// UARTs
#define UART_REG_WR_TEST_ERR						0x0100
#define UART_INT_TIMEOUT_ERR						0x0101
#define UART_INT_ID_ERR							0x0102
#define UART_LOOPBACK_STATUS_ERR					0x0103
#define UART_LOOPBACK_DATA_ERR						0x0104
#define UART_INT_MASK_ERR						0x0105
#define UART_BREAK_ERR							0x0106
#define UART_PERR_ERR							0x0107
#define UART_FRAMING_ERR						0x0108
#define UART_OVERRUN_ERR						0x0109

// Tachyons
#define TACH_PCI_REG_ERR						0x0110
#define TACH_REG_ERR							0x0111
#define BAD_RST_TYPE_ERR						0x0112
#define TACH_ONLINE_UNEXPECTED_INT_ERR					0x0113
#define TACH_ONLINE_LNKFAIL_ERR						0x0114
#define FC_DIAG_ALPA_ERR						0x0115
#define TACH_NOT_ONLINE_ERR						0x0116
#define FC_DIAG_HOST_CTRL_ERR						0x0117
#define FC_OFFL_DIAG_INT_ERR						0x0118
#define FC_OFFL_UNEXPECTED_INT_ERR					0x0119
#define FC_DIAG_INT_ERR1						0x011A
#define FC_TACH_NO_DATA_INT_ERR						0x011B
#define BAD_TEST_MODE_TYPE_ERR						0x011C
#define FC_DIAG_LOOPBK_ERR						0x011D
#define FC_TACH_FRAME_LEN_ERR						0x011E
#define FC_BAD_RX_CHAR_ERR						0x011F
#define FC_BAD_RX_CHAR_ERR1						0x0120
#define FC_STATUS_1_ERR							0x0121
#define FC_STATUS_2_ERR							0x0122
#define SEST_SW_ERR							0x0123
#define TACH_SGBUF_SIZE_EXCEEDED_ERR					0x0124
#define TACH_DIS_INT_ERR						0x0125
#define FC_INDX_BAD_BOUNDARY						0x0126
#define FC_INCORRECT_DATA_INT_ERR					0x0127

// FRUMON
#define FRUMON_REQUEST_ERR						0x0130
#define FRUMON_REPLY_ERR						0x0131
#define FRUMON_VERIFY_ERR						0x0132
#define FRUMON_ERROR_REPLY_ERR						0x0133
#define FRUMON_UNKNOWN_CMD_ERR						0x0134
#define FRUMON_UCODE_DOWNLOAD_ERR					0x0135
#define FRUMON_POLL_REPLY_ERR						0x0136
#define UNEXPECTED_FRUMON_REPLY_ERR					0x0137
#define FRUMON_WRITE_ERR						0x0138
#define FRUMON_SERIAL_READ_ERR						0x0139
#define FRUMON_SERIAL_WRITE_ERR						0x013A
#define DAE_ENCL_ADDR_NOT_ZERO_POLL_ERR					0x013B
#define SP_ENCL_ADDR_UNEXPECTED_ERR					0x013C
#define FRUMON_OVERTEMP_ERR						0x013D
#define FRUMON_MULTI_FAN_FAULT_ERR					0x013E
#define LCC0_MISSING_ERR						0x013F
#define NO_DISK_ENCLOSURE_ERR						0x0140
#define LCC_INVALID_PORT_ID_ERR						0x0141
#define FRUMON_CHKSUM_ERR						0x0142
#define FRUMON_ENCL_DEGRADED_ERR					0x0143
#define ENCLOSURE_NOT_FOUND_ERR						0x0144
#define FRUM_UNKNOWN_COMMAND						0x0145
#define FRUM_CHKSUM_ERR_OUTGOING					0x0146
#define FRUM_CHKSUM_ERR_INCOMING					0x0147
#define FRUM_COMMAND_FAULT						0x0148
#define FRUM_TIMEOUT_FAULT						0x0149
#define FRUM_ADDRESS_FAULT						0x014A
#define FRUM_INVALID_DOWNLOAD_ADDR					0x014B
#define FRUM_INVALID_FW_IMAGE						0x014C
#define FRUM_I2C_PIN_TIMEOUT						0x014D
#define FRUM_I2C_ERROR							0x014E
#define FRUM_I2C_NO_SLAVE_ACK						0x014F
#define FRUM_I2C_BUS_TIMEOUT						0x0150
#define FRUM_I2C_BUS_ERROR						0x0151
#define FRUM_UCODE_CHKSUM_ERROR						0x0152
#define FRUM_FLASH_WRITE_ERROR						0x0153
#define FRUM_FLASH_VERIFY_ERROR						0x0154
#define FRUM_PKG_SIZE_ERROR						0x0155
#define FRUM_SYSTEM_ERROR						0x0156
#define FRUM_RCVD_NACK							0x0157
#define FRUM_NOT_RCVD_ACK						0x0158

// Unexpected Exceptions
#define UNEXPECTED_INTERRUPT_ERR					0x0160
#define MACHINE_CHECK_ERROR						0x0161

// Illegal Configuration: Any errors in this grouping of 0x17x will result in the SP indicating
// a configuration error using the fault status register
#define IOM_CONFIG_ERR_MASK						0x0170	// NOT A DUPLICATE!
#define INVALID_IOM_CONFIGURATION_ERR					0x0170
#define FC_SFP_NOT_PRESENT_ERR						0x0171
#define INVALID_MGMT_FRU_ERR						0x0172
#define ILLEGAL_MEMORY_SIZE_ERR						0x0173
#define SLOT_IND_ERR							0x0174
#define LCC_CABLE_ERR							0x0175
#define MIXED_ENCL_LOOP_ERR						0x0176
#define DAE_ENCL_ADDR_NOT_ZERO_ERR					0x0177
#define INVALID_MIDPLANE_CONFIG_ERR					0x0178
#define INVALID_CPU_MODULE_ERR						0x0179
#define NV_INVALID_PEER_SP_ERR						0x017A
#define CROSS_CABLING_ERR						0x017B

// I/O Module errors that don't result in an illegal configuration status
#define IOM_PCI_SCAN_ERR						0x0180

// Real Time Clock
#define RTC_INVALID_ERR							0x0190
#define RTC_RUNNING_ERR							0x0191
#define RTC_ROLLOVER_ERR						0x0192

// Disk I/O
#define SCSI_UNDERRUN_ERR						0x01A0
#define SCSI_BAD_PARAMETER_ERR						0x01A1
#define UNSUPPORTED_SCSI_COMMAND_ERR					0x01A2
#define SCSI_AUTO_SENSE_FAILED_ERR					0x01A3
#define DM_EXCHANGE_ERR							0x01A4
#define DISK_WRITE_ERR							0x01A5

// NVRAM errors
#define NV_MEM_TYPE_ERR							0x01B0
#define NV_ADDRESS_ERR							0x01B1
#define NV_INVERSE_ADDRESS_ERR						0x01B2
#define NV_PATTERN_ERR							0x01B3
#define NV_SERIAL_CONSOLE_ERR						0x01B4
#define NV_INVALID_WWN_ERR						0x01B5
#define NV_INVALID_CSN_ERR						0x01B6
#define NV_RESUME_CHECKSUM_ERR						0x01B7
#define NV_INVALID_SP_PART_NUM_ERR					0x01B8
#define NV_INVALID_WWN_SEED_ERR						0x01B9
#define NV_INVALID_PS_PART_NUM_ERR					0x01BA
#define BIOS_SMI_ENABLED_ERR						0x01BB
#define NVRAM_SIG_NOT_FOUND_ERR						0x01BC
#define NVRAM_MAP_TABLE_INVALID_ERR					0x01BD

// SMBUS/I2C interface errors
#define SMBUS_TIMEOUT_ERR						0x01C0
#define SMBUS_STATUS_ERR						0x01C1
#define I2C_ARBITRATION_ERR						0x01C2
#define I2C_INVALID_DEVICE_ERR						0x01C3

// Firmware Flash Update errors
#define FLASH_TIMEOUT_ERR						0x01D0
#define FLASH_STATUS_ERR						0x01D1
#define FLASH_ERASE_ERR							0x01D2
#define FLASH_BOOT_BLOCK_UPDATE_ERR					0x01D3
#define FLASH_INVALID_IMAGE_TYPE_ERR					0x01D4
#define FLASH_CHECKSUM_ERR						0x01D5
#define FLASH_SECTOR_PROTECT_ERR					0x01D6

// Resume PROM errors
#define RP_CHECKSUM_ERR							0x01E0
#define RP_INVALID_ID_ERR						0x01E1
#define INVALID_ASSEMBLY_REV_ERR					0x01E2
#define UNSUPPORTED_FEATURE_ERR						0x01E3
#define SFP_IDENTIFIER_ERR						0x01E4
#define SFP_CONNECTOR_ERR						0x01E5
#define SFP_TRANSCEIVER_ERR						0x01E6
#define RP_UNIQUE_CHECKSUM_ERR						0x01E7
#define RP_UNIQUE_AREA_TYPE_ERR						0x01E8
#define RP_UNIQUE_AREA_PREAMBLE_ERR					0x01E9
#define RP_UNIQUE_AREA_POSTAMBLE_ERR					0x01EA
#define SFP_INVALID_ERR							0x01EB

// Test/Diag errors
#define INDIVIDUAL_POST_ERR						0x01F0
#define INDIVIDUAL_DIAG_ERR						0x01F1
#define SYSTEM_TEST_ERR							0x01F2

// RAM errors
#define ECC_GLOBAL_STATUS_ERR						0x0200
#define ECC_NF_STATUS_REG_ERR						0x0201
#define ECC_NF_DEVICE_STATUS_ERR					0x0202
#define SYNDROME_ERR							0x0203
#define ECC_ADDRESS_ERR							0x0204
#define AUTO_DQS_ERR							0x0205
#define INVALID_DIMM_CONFIG_ERR						0x0206
#define MRC_LOG_ERR							0x0207
#define ECC_CHECK_DEVTAG_ERR						0x0208
#define MC_SYSTEM_MEM_FAULT_ERR						0x0209
#define INVALID_MIN_MEMORY_ERR						0x020A
#define BIOS_VECTOR_TABLE_ERR						0x020B
#define DIMM_SPD_CRC_ERR						0x020C
#define ECC_CHECK_EMC_CORRERRCNT_ERR					0x020D
#define ECC_CHECK_OVERFLOW_ERR						0x020E
#define ECC_CHECK_THRESHOLD_ERR						0x020F

// Image errors
#define IMAGE_ERR_MASK							0x0210	// NOT A DUPLICATE!
#define IMAGE_ID_ERR							0x0210
#define IMAGE_CHECKSUM_ERR						0x0211
#define IMAGE_LOAD_ERR							0x0212
#define NO_VALID_BOOT_DISK_ERR						0x0213
#define INVALID_MBR_ERR							0x0214
#define INVALID_PARTITION_TABLE_ERR					0x0215
#define ILLEGAL_BOOT_PATH_REQUEST					0x0216
#define BOOT_REQUEST_STOP						0x0217
#define IMAGE_SIZE_ERR							0x0218
#define IMAGE_HEADER_CORRUPT_ERR					0x0219

// Processor errors
#define PROCESSOR_TIMEOUT_ERR						0x0220
#define PROCESSOR_AP_INIT_ERR						0x0221
#define PROCESSOR_AP_HALT_ERR						0x0222
#define PROCESSOR_BP_AP_INT_P1_ERR					0x0223
#define PROCESSOR_BP_AP_INT_P2_ERR					0x0224
#define PROCESSOR_BP_AP_INT_NUM_ERR					0x0225
#define PROCESSOR_BP_AP_INT_TIMEOUT_ERR					0x0226
#define PROCESSOR_AP_BP_INT_ERR						0x0227
#define PROCESSOR_AP_BP_INT_NUM_ERR					0x0228
#define PROCESSOR_AP_BP_INT_TIMEOUT_ERR					0x0229
#define PROCESSOR_INVALID_SIGN_ID_ERR					0x022A
#define PROCESSOR_MISMATCH_ERR						0x022B
#define PROCESSOR_INVALID_COUNT_ERR					0x022C
#define PROCESSOR_BSP_ERR						0x022D
#define PROCESSOR_NO_PROTOCOL_ERR					0x022E

// Memory Manager error codes
#define MALLOC_ERR							0x0240
#define MEMORY_FREE_ERR							0x0241

// PIC error codes
#define PIC_RESPONSE_ERR						0x0250

// Diplex Device error codes
#define DD_CHANNEL_MISMATCH						0x0260
#define DD_CHKSUM_MISMATCH						0x0261
#define DD_COMPORT_ERR							0x0262
#define DD_REG_ERR							0x0263
#define DD_PACKET_CNT_ERR						0x0264
#define DD_NOT_IN_CONFIG_MODE_ERR					0x0265
#define DD_BAD_DATA_PRGRM_STAT_ERR					0x0266
#define DD_NO_CONFIG_DN_ERR						0x0267
#define DD_LPBK_DATA_ERR						0x0268
#define DD_CLOCK_ERR							0x0269

// Serial I/O error codes
#define SER_INVALID_PORT						0x0270
#define SER_TIME_OUT							0x0271
#define SER_MAX_PKG_SIZE						0x0272
#define SER_NO_ACK							0x0273
#define SER_ERROR							0x0274
#define SER_RCVD_NACK							0x0275

// Power Supply
#define PS_INVALID_RESPONSE_ERR						0x0281
#define PS_RESPONSE_ERR							0x0282
#define PS_INCORRECT_REV_ERR						0x0283

// Broadcom 570x LAN error codes
#define BC570X_SERIAL_ARB_TIMEOUT_ERR					0x0290
#define BC570X_SERIAL_READ_TIMEOUT_ERR					0x0291
#define BC570X_SERIAL_WRITE_TIMEOUT_ERR					0x0292
#define BC570X_FW_COMPLETION_TIMEOUT_ERR				0x0293
#define BC570X_BUFFER_MGR_DID_NOT_START_ERR				0x0294
#define BC570X_COALESCING_SHUTDOWN_ERR					0x0295
#define BC570X_MI_COM_READ_TIMEOUT_ERR					0x0296
#define BC570X_MI_COM_WRITE_TIMEOUT_ERR					0x0297
#define BC570X_MI_COM_READ_FAILED_ERR					0x0298
#define BC570X_PHY_NO_LINK_ERR						0x0299
#define BC570X_INT_TEST_TIMEOUT_ERR					0x029A
#define BC570X_PACKET_TX_TIMEOUT_ERR					0x029B
#define BC570X_PACKET_RX_TIMEOUT_ERR					0x029C
#define BC570X_PACKET_SIZE_ERR						0x029D
#define BC570X_PACKET_HEADER_ERR					0x029E
#define BC570X_PACKET_DATA_ERR						0x029F
#define BC570X_EEPROM_XSUM_INVALID_ERR					0x02A0
#define BC570X_I2C_ERR							0x02A1
#define BC570X_INIT_ERR							0x02A2
#define BC570X_DRIVER_INIT_ERR						0x02A3
#define BC570X_FLASH_ACCESS_ERR						0x02A4
#define BC570X_CHIP_NOT_SUPPORTED_ERR					0x02A5
#define BC570X_NVRAM_NOT_SUPPORTED_ERR      				0x02A6
#define BC570X_NVRAM_SIZE_UNKNOWN_ERR       				0x02A7
#define BC570X_LINK_SPEED_ERR						0x02A8
#define BC570X_NVRAM_PAGE_SIZE_ERR					0x02A9

// Generic utils error codes
#define PCI_REG_ERR							0x02B0
#define MM_REG_ERR							0x02B1
#define DATA_PAT_VER_ERR						0x02B2
#define MESSAGE_XSUM_ERR						0x02B3

// Timer utils error codes
#define WAIT_MS_EXC_TIMEOUT						0x02C0
#define WAIT_US_EXC_TIMEOUT						0x02C1
#define TIMER_ERR							0x02C2

// PCI Express Errors
#define PCIEXP_LINKWIDTH_ERR						0x02D0
#define PCIEXP_BUS_DISABLED_ERR						0x02D1
#define PCIEXP_LINKSPEED_ERR						0x02D2
#define PCIEXP_NO_DEVICE_ERR						0x02D3
#define PCIEXP_MAX_PAYLOAD_ERR						0x02D4
#define PCI_BASE_ADDRESS_ERR						0x02D5
#define DMI_LINKWIDTH_ERR						0x02D6
#define DMI_LINKSPEED_ERR						0x02D7
#define QPI_LINKSPEED_ERR						0x02D8
#define QPI_LINKSTATUS_ERR						0x02D9
#define CMI_LINKWIDTH_ERR           	    				0x02DA
#define CMI_LINKSPEED_ERR               				0x02DB
#define CMI_LINK_NOT_UP_ERR             				0x02DC
#define DMI_DMIBAR_DISABLED             				0x02DD
#define PCIEXP_LINKSPEED_WIDTH_ERR					0x02DE

// Fibre I2C bus error
#define I2C_INVALID_SEEPROM						0x02E0
#define I2C_TIP_ERR							0x02E1
#define I2C_NACK							0x02E2

// more I/O Module
#define IOM_DEVICE_ERR							0x02F0
#define IOM_MISMATCH_ETHORNET_ERR       				0x02F1

// Enclosure Error
#define NMI_BUTTON_STUCK_ERR						0x0300

// Serial Download Errors
#define SERIALD_BAD_PARAMS_ERR						0x0310
#define SERIALD_FAILED_ERR						0x0311
#define SERIALD_TIMEOUT_ERR						0x0312

// PLX Error Codes
#define PLX_COMMAND_TIMEOUT_ERR						0x0320
#define PLX_COMMAND_COMPLETION_TIMEOUT_ERR				0x0321
#define PLX_CRC_READ_ERR						0x0322
#define PLX_RESET_IMAGE_UPDATE_ERR					0x0323
#define PLX_EEPROM_NOT_PRESENT_ERR					0x0324
#define PLX_BIST_FAILURE_ERR						0x0325
#define PLX_JTAG_FAILURE_ERR						0x0326
#define PLX_JTAG_REWORK_ERR						0x0327

// USB Flash Map
#define USB_PID_NOT_FOUND_ERR						0x0330
#define USB_FLASH_MAP_INVALID_ERR					0x0331

// GPIO error
#define PEER_PRESENT_IND_ERR						0x0340

// CDES/ESES Failures
#define CDES_PHY_MAPPING_ERR						0x0350
#define CDES_COMMAND_ERR						0x0351
#define CDES_PHY_PARSING_ERR						0x0352
#define CDES_CLEAR_RECONFIG_ERR						0x0353

// Firmware Update
#define FMP_OPEN_PROTOCOL_ERR						0x0360
#define IMAGE_NOT_UPDATABLE						0x0361

// Management module
#define MGMT_SUPPLY_A_FAULT_ERR						0x0370
#define MGMT_SUPPLY_B_FAULT_ERR						0x0371
#define MGMT_SWITCH_FAULT_ERR						0x0372
#define MGMT_SPI_TIMEOUT_FAULT_ERR					0x0373
#define MGMT_RACK_FAULT_ERR						0x0374
#define MGMT_SLOT_ID_FAULT_ERR						0x0375
#define MGMT_NMI_STUCK_ERR						0x0376
#define MGMT_PEER_UC_COMM_LOST_ERR					0x0377
#define MGMT_CABLE_DIAG_FAULT_ERR					0x0378
#define MGMT_LOOP_DETECT_ERR						0x0379

// CMD errors
#define CMD_FAULT_ERR							0x0380
#define CMD_UNDER_VTG_MISMATCH_ERR					0x0381
#define CMD_OVER_VTG_MISMATCH_ERR					0x0382
#define CMD_PAGE_MISMATCH_ERR						0x0383

// Marvell Cache Card controller
#define MARV_MSG_TIMEOUT						0x0390
#define MARV_MSG_SIGNATURE						0x0391
#define MARV_MSG_INCORRECT_RESP						0x0392
#define MARV_MEM_ECC_ERR						0x0393
#define MARV_MSG_NO_RESP						0x0394
#define MARV_BAT_NOT_CONNECTED						0x0395
#define MARV_BAT_DISCHARGING						0x0396
#define MARV_DRAM_TEST_FAIL						0x0397
#define MARV_DRAM_POST_TEST_SKIPPED					0x0398
#define MARV_SSD_NOT_CONNECTED						0x0399
#define MARV_SSD_NOT_RESPONDING						0x039A
#define MARV_SSD_IO_FAILS						0x039B
#define MARV_DRAM_STATUS_INVALID					0x039C
#define MARV_BAT_STATUS_INVALID						0x039D
#define MARV_SSD_STATUS_INVALID						0x039E

// Broadcom 5709
#define BC5709_SERIAL_ARB_TIMEOUT_ERR					0x03A0
#define BC5709_SERIAL_READ_TIMEOUT_ERR					0x03A1
#define BC5709_SERIAL_WRITE_TIMEOUT_ERR					0x03A2
#define BC5709_FW_COMPLETION_TIMEOUT_ERR				0x03A3
#define BC5709_COALESCING_SHUTDOWN_ERR					0x03A5
#define BC5709_MI_COM_READ_TIMEOUT_ERR					0x03A6
#define BC5709_MI_COM_WRITE_TIMEOUT_ERR					0x03A7
#define BC5709_MI_COM_READ_FAILED_ERR					0x03A8
#define BC5709_PHY_NO_LINK_ERR						0x03A9
#define BC5709_INT_TEST_TIMEOUT_ERR					0x03AA
#define BC5709_PACKET_TX_TIMEOUT_ERR					0x03AB
#define BC5709_PACKET_RX_TIMEOUT_ERR					0x03AC
#define BC5709_PACKET_SIZE_ERR						0x03AD
#define BC5709_PACKET_HEADER_ERR					0x03AE
#define BC5709_PACKET_DATA_ERR						0x03AF
#define BC5709_EEPROM_XSUM_INVALID_ERR					0x03B0
#define BC5709_REG_WRRD_ERROR						0x03B1
#define BC5709_REG_WRRD_FLOAT_ERROR					0x03B2
#define BC5709_REG_WRRD_XOR_ERROR					0x03B3
#define BC5709_REG_CLOCK_ERROR						0x03B4
#define BC5709_PACKET_MAC_DATA_ERR					0x03B5
#define BC5709_PACKET_ID_ERR						0x03B6
#define BC5709_PACKET_FCS_DATA_ERR					0x03B7
#define BC5709_PHY_STATS_COUNT_ERR					0x03B8
#define BC5709_TX_RISC_STALLED_ERR					0x03B9
#define BC5709_RX_RISC_STALLED_ERR					0x03BA
#define BC5709_RX_RISC_FATAL_ERR					0x03BB
#define BC5709_TX_RISC_FATAL_ERR					0x03BC
#define BC5709_MTTS_PROGRESS_ERR					0x03BD
#define BC5709_INIT_ERR							0x03BE
#define BC5709_TX_TIMEOUT_ERR						0x03BF
#define BC5709_RX_TIMEOUT_ERR						0x03C0
#define BC5709_I2C_ERR							0x03C1
#define BC5709_PMD_BIST_DETECT_ERR					0x03C2
#define BC5709_PHY_OP_ERR						0x03C3
#define BC5709_PMD_BIST_ERRORCNT_ERR					0x03C4
#define BC5709_PMD_BIST_PKTCNT_ERR					0x03C5
#define BC5709_XGS_BIST_DETECT_ERR					0x03C6
#define BC5709_XGS_BIST_ERRORCNT_ERR					0x03C8
#define BC5709_XGS_BIST_PKTCNT_ERR					0x03C9
#define BC5709_SER_EEPROM_ERRORCNT_ERR					0x03CA
#define BC5709_SERDES_REG_WV_ERR					0x03CB
#define BC5709_SERDES_REG_RESET_ERR					0x03CC
#define BC5709_SERDES_ID_ERR						0x03CD
#define BC5709_SERDES_PCS_SIG_DETECT_ERR				0x03CE
#define BC5709_SERDES_XGS_SIG_DETECT_ERR				0x03CF
#define BC5709_LINK_UP_ERR						0x03D0
#define BC5709_PCIE_LINK_WIDTH_ERR					0x03D1
#define BC5709_DRIVER_INIT_ERR						0x03D2
#define BC5709_CHIP_PANIC_ERR						0x03D3
#define BC5709_LED_CTRL_ERR						0x03D4
#define BC5709_FLASH_ACCESS_ERR						0x03D5
#define BC5709_SELF_TEST_ERR						0x03D6

// IPMI Errors
#define IPMI_PROTOCOL_NOT_FOUND_ERR					0x03E0
#define SEND_IPMI_CMD_FAILED_ERR					0x03E1
#define IPMI_MASTER_WR_RD_FAILED_ERR					0x03E2
#define IPMI_CMD_FAILED_ERR						0x03E3
#define IMAGE_TRANSFER_FAILED_ERR					0x03E4
#define FW_UPDATE_FAILED_ERR						0x03E5
#define BMC_SELF_TEST_ERR						0x03E6
#define BMC_IOM_PWR_DIS_ERR						0x03E7
#define FAILED_READ_NVRAM_ERR						0x03E8
#define FAILED_WRITE_NVRAM_ERR						0x03E9
#define EMC_NVRAM_IPMI_NO_PROTOCOL_ERR					0x03EA
#define BUS_ERROR_ON_REVISION_READ_ERR      				0x03EB
#define BMC_FW_UPDATE_LOCK             					0x03EC

// TFTP Download Errors
#define TFTP_FAILED_ERR							0x03F0
#define TFTP_TIMEOUT_ERR						0x03F1

// network EDK errors
#define NETWORK_FAILED_ERR						0x0400
#define NETWORK_EFI_ERR							0x0401
#define NETWORK_ALREADY_STARTED_ERR					0x0402
#define NETWORK_CHILD_NOT_SUPPOTRT_ERR 					0x0403
#define NETWORK_CHILD_NOT_FOUND_ERR					0x0404

// UEFI Driver Errors
#define DRIVER_LOAD_ERR           		      			0x0410
#define DRIVER_UNLOAD_ERR               				0x0411
#define DRIVER_START_ERR                				0x0412
#define DRIVER_STOP_ERR                 				0x0413
#define DRIVER_DIAG_ERR                 				0x0414
#define FW_MGT_ERR                 					0x0415

// Block Storage Device
#define BLOCK_STORAGE_NO_DEVICE_ERR					0x0420
#define BLOCK_STORAGE_NO_PROTOCOL_ERR					0x0421
#define BLOCK_STORAGE_RESET_ERR						0x0422
#define BLOCK_STORAGE_FLUSH_ERR						0x0423
#define BLOCK_STORAGE_READ_ERR						0x0424
#define BLOCK_STORAGE_WRITE_ERR						0x0425

// File system protocol
#define SIMPLE_FILE_SYSTEM_NO_PROTOCOL_ERR				0x0430
#define FILE_NO_PROTOCOL_ERR						0x0431
#define FAILED_OPEN_FILE_ERR						0x0432
#define FAILED_WRITE_FILE_ERR						0x0433
#define FAILED_CLOSE_FILE_ERR						0x0434

// USB TI 3410/5052 Errors
#define TUSB_NO_PROTOCOL_ERR						0x0440
#define TUSB_PROTOCOL_FAIL_ERR						0x0441
#define TUSB_PROTOCOL_TIMEOUT_ERR					0x0442
#define TUSB_NOT_EXECUTE_ERR						0x0443
#define TUSB_STALL_ERR							0x0444
#define TUSB_BUFFER_ERR							0x0445
#define TUSB_BABBLE_ERR							0x0446
#define TUSB_NAK_ERR							0x0447
#define TUSB_CRC_ERR							0x0448
#define TUSB_TIMEOUT_ERR						0x0449
#define TUSB_BITSTUFF_ERR						0x044A
#define TUSB_SYSTEM_ERR							0x044B

// Broadcom 57710 LAN error codes
#define BC57710_SERIAL_ARB_TIMEOUT_ERR					0x0450
#define BC57710_SERIAL_READ_TIMEOUT_ERR					0x0451
#define BC57710_SERIAL_WRITE_TIMEOUT_ERR				0x0452
#define BC57710_FW_COMPLETION_TIMEOUT_ERR				0x0453
#define BC57710_COALESCING_SHUTDOWN_ERR					0x0454
#define BC57710_MI_COM_READ_TIMEOUT_ERR					0x0455
#define BC57710_MI_COM_WRITE_TIMEOUT_ERR				0x0456
#define BC57710_MI_COM_READ_FAILED_ERR					0x0457
#define BC57710_PHY_NO_LINK_ERR						0x0458
#define BC57710_INT_TEST_TIMEOUT_ERR					0x0459
#define BC57710_PACKET_TX_TIMEOUT_ERR					0x045A
#define BC57710_PACKET_RX_TIMEOUT_ERR					0x045B
#define BC57710_PACKET_SIZE_ERR						0x045C
#define BC57710_PACKET_HEADER_ERR					0x045D
#define BC57710_PACKET_DATA_ERR						0x045E
#define BC57710_EEPROM_XSUM_INVALID_ERR					0x045F
#define BC57710_REG_WRRD_ERROR						0x0460
#define BC57710_REG_WRRD_FLOAT_ERROR					0x0461
#define BC57710_REG_WRRD_XOR_ERROR					0x0462
#define BC57710_REG_CLOCK_ERROR						0x0463
#define BC57710_PACKET_MAC_DATA_ERR					0x0464
#define BC57710_PACKET_ID_ERR						0x0465
#define BC57710_PACKET_FCS_DATA_ERR					0x0466
#define BC57710_PHY_STATS_COUNT_ERR					0x0467
#define BC57710_TX_RISC_STALLED_ERR					0x0468
#define BC57710_RX_RISC_STALLED_ERR					0x0469
#define BC57710_RX_RISC_FATAL_ERR					0x046A
#define BC57710_TX_RISC_FATAL_ERR					0x046B
#define BC57710_MTTS_PROGRESS_ERR					0x046C
#define BC57710_INIT_ERR						0x046D
#define BC57710_TX_TIMEOUT_ERR						0x046E
#define BC57710_RX_TIMEOUT_ERR						0x046F
#define BC57710_I2C_ERR							0x0470
#define BC57710_PMD_BIST_DETECT_ERR					0x0471
#define BC57710_PHY_OP_ERR						0x0472
#define BC57710_PMD_BIST_ERRORCNT_ERR					0x0473
#define BC57710_PMD_BIST_PKTCNT_ERR					0x0474
#define BC57710_XGS_BIST_DETECT_ERR					0x0475
#define BC57710_XGS_BIST_ERRORCNT_ERR					0x0476
#define BC57710_XGS_BIST_PKTCNT_ERR					0x0477
#define BC57710_SER_EEPROM_ERRORCNT_ERR					0x0478
#define BC57710_SERDES_REG_WV_ERR					0x0479
#define BC57710_SERDES_REG_RESET_ERR					0x047A
#define BC57710_SERDES_ID_ERR						0x047B
#define BC57710_SERDES_PCS_SIG_DETECT_ERR				0x047C
#define BC57710_SERDES_XGS_SIG_DETECT_ERR				0x047D
#define BC57710_XAUI_LINK_UP_ERR					0x047E
#define BC57710_PHY_LINK_UP_ERR						0x047F
#define BC57710_PCIE_LINK_WIDTH_ERR					0x0480
#define BC57710_DRIVER_INIT_ERR						0x0481
#define BC57710_CHIP_PANIC_ERR						0x0482
#define BC57710_LED_CTRL_ERR						0x0483
#define BC57710_FLASH_ACCESS_ERR					0x0484
#define BC57710_REG_READ_ERR						0x0485
#define BC57710_PCIE_CONFIG_REG_ERR					0x0486
#define BC57710_PCIE_MEM_REG_ERR					0x0487
#define BC57710_PCIE_CORR_ERR						0x048A
#define BC57710_PCIE_UNCORR_ERR						0x048B
#define BC57710_PCIE_DEVSTAT_ERR					0x048C
#define BC57710_FLASH_PAGE_SIZE_ERR                                     0x048D

// LSI SAS
#define SSC_DIAG_WRITE_ENA_ERR						0x0490
#define SSC_HARD_RESET_ERR					        0x0491
#define SSC_DOORBELL_INUSE_ERR						0x0492
#define SSC_HANDSHAKE_TIMEOUT_ERR					0x0493
#define SSC_IOC_STATUS_ERR						0x0494
#define SSC_REQUEST_QUEUE_FULL_ERR					0x0495
#define SSC_NO_CONFIG_REPLY_ERR						0x0496
#define SSC_NO_EVENT_NOTIFICATION_REPLY_ERR				0x0497
#define SSC_NO_IO_UNIT_CTRL_REPLY_ERR					0x0498
#define SSC_PORT_ENABLE_ERR						0x0499
#define SSC_IOC_NOT_OPERATIONAL_ERR					0x049A
#define SSC_NO_SSP_COMPLETION_ERR					0x049B
#define SSC_DATA_MISCOMPARE_ERR						0x049C
#define SSC_RUNNING_DISPARITY_ERR					0x049D
#define SSC_INVALID_DWORD_ERR						0x049E
#define SSC_OUT_OF_SYNC_ERR						0x049F
#define SSC_PHY_RESET_PROBLEM_ERR					0x04A0
#define SSC_ELASTIC_BUF_OVERFLOW_ERR					0x04A1
#define SSC_LINK_UP_FAILURE_ERR						0x04A2
#define SSC_NO_IO_REQUEST_ERR						0x04A3
#define SSC_NO_POST_CMD_BASE_REPLY_ERR					0x04A4
#define SSC_NO_DIAGBUF_POST_REPLY_ERR					0x04A5
#define SSC_NO_DIAG_RELEASE_REPLY_ERR					0x04A6
#define SSC_MESSAGE_UNIT_RESET_ERR					0x04A7
#define SSC_INITIATOR_NOT_RESPONDING_ERR				0x04A8

// UEFI Device
#define UEFI_LOAD_DRIVER_ERR						0x04B0
#define UEFI_DEVICE_ERR							0x04B1
#define UEFI_DEVICE_GET_IMAGE_INFO_ERR					0x04B2
#define UEFI_DEVICE_IMAGE_SIZE_ERR					0x04B3
#define UEFI_DEVICE_HEADER_ERR						0x04B4
#define UEFI_DEVICE_PROGRAM_IMAGE_ERR					0x04B5
#define UEFI_DEVICE_VERIFY_IMAGE_ERR					0x04B6
#define UEFI_DEVICE_CHECK_IMAGE_ERR					0x04B7

//Security error codes
#define UEFI_SECURITY_DIG_PROTOCOL_NOT_FOUND_ERR			0x04C0
#define UEFI_SECURITY_GET_KEY_ERR                                  	0x04C1
#define UEFI_SECURITY_VALIDATE_CAPSULE_HEADER_ERR    			0x04C2
#define UEFI_SECURITY_CERTIFICATE_TYPE_ERR                   		0x04C3
#define UEFI_SECURITY_CAPSULE_IMAGE_SIEZ_ERR              		0x04C4
#define UEFI_SECURITY_CAPSULE_HEADER_ERR                    		0x04C5
#define UEFI_SECURITY_FW_CERTIFICATE_ERR                     		0x04C6

// Emulex IPL Device Errors
#define DEVICE_EMULEX_GET_DATA_ERR					0x04D0
#define DEVICE_EMULEX_IPL_VERSION_ERR					0x04D1

//SAS Cable existence Errors
#define SAS_CABLE_IS_NOT_PRESENT_ERR                			0x04E0

/*
 * The following POST codes will be used as WARNING/Checkpoint codes
 * Note: all codes of this type will have bit 15 set, error codes will not
 */
#define CP_POST_MASK							0x8000
#define CP_POST_FW_UPDATE                                               0x800F
#define CP_POST_INIT							0x8081
#define CP_POST_START							0x8082
#define CP_POST_SYSTEM_TEST						0x8083
#define CP_POST_END							0x8084
#define CP_POST_OS_BOOT_BE0						0x8085
#define CP_POST_OS_BOOT_BE1						0x8086
#define CP_POST_OS_BOOT_AUX0						0x8087
#define CP_POST_OS_BOOT_AUX1						0x8088
#define CP_POST_HOLD_IN_POST						0x8089
#define CP_POST_USER_ABORT						0x808A
#define CP_POST_PXE_BOOT						0x808B
#define CP_POST_UP_BOOT							0x808C

/* OS CODES */
#define CSR_OS_DEGRADED_MODE                0x02000000  /* written by NDUMon */
#define CSR_OS_CORE_DUMPING                 0x02000001  /* checked in cm */
#define CSR_OS_APPLICATION_RUNNING          0x02000002  /* written by NDUMon */
#define CSR_OS_RUNNING                      0x02000003  /* written by MIR */
#define CSR_OS_BLADE_BEING_SERVICED         0x02000004
#define CSR_OS_ILLEGAL_MEMORY_CONFIG        0x02000005  /* written by NDUMon */

// VEEPROM constants end.

//++
// RESUME related constants start.
//--
#define IPMI_RESUME_INDEX_OFFSET                            16
#define IPMI_RESUME_IDEAL_READ_SIZE                         0x10

#define IPMI_RESUME_EMC_PART_NUMBER_LSB_OFFSET              0xD2 // EMC Resume emc_tla_part_num - offset 0x000
#define IPMI_RESUME_EMC_PART_NUMBER_MSB_OFFSET              0x00

#define IPMI_RESUME_EMC_ASSEMBLY_NAME_1_LSB_OFFSET          0xA0 // EMC Resume tla_assembly_name - offset 0x150
#define IPMI_RESUME_EMC_ASSEMBLY_NAME_1_MSB_OFFSET          0x00
#define IPMI_RESUME_EMC_ASSEMBLY_NAME_2_LSB_OFFSET          0xB0
#define IPMI_RESUME_EMC_ASSEMBLY_NAME_2_MSB_OFFSET          0x00

#define IPMI_RESUME_EMC_ASSEMBLY_REV_LSB_OFFSET             0xE8 // EMC Resume emc_tla_assembly_rev - offset 0x013
#define IPMI_RESUME_EMC_ASSEMBLY_REV_MSB_OFFSET             0x00

#define IPMI_RESUME_EMC_SERIAL_NUM_LSB_OFFSET               0xC1 // EMC Resume emc_tla_serial_num - offset 0x020
#define IPMI_RESUME_EMC_SERIAL_NUM_MSB_OFFSET               0x00

#define IPMI_RESUME_EMC_FAMILY_FRU_ID_LSB_OFFSET            0xCA // EMC Resume sp_family_fru_id - offset 0x7C0
#define IPMI_RESUME_EMC_FAMILY_FRU_ID_MSB_OFFSET            0x01

#define IPMI_RESUME_EMC_FRU_CAPABILITY_LSB_OFFSET           0xE4 // EMC Resume fru_capability - offset 0x7C4, PS reg 0xB4
#define IPMI_RESUME_EMC_FRU_CAPABILITY_MSB_OFFSET           0x01

#define IPMI_RESUME_VENDOR_NAME_1_LSB_OFFSET                0x7F // EMC Resume vendor_name - offset 0x100
#define IPMI_RESUME_VENDOR_NAME_1_MSB_OFFSET                0x00
#define IPMI_RESUME_VENDOR_NAME_2_LSB_OFFSET                0x8F
#define IPMI_RESUME_VENDOR_NAME_2_MSB_OFFSET                0x00

#define IPMI_RESUME_VENDOR_ASSEMBLY_REV_LSB_OFFSET          0x37 // EMC Resume vendor_assembly_rev - 0x0A3
#define IPMI_RESUME_VENDOR_ASSEMBLY_REV_MSB_OFFSET          0x01

#define IPMI_RESUME_VENDOR_PART_NUM_1_LSB_OFFSET            0x12 // EMC Resume vendor_part_num - offset - 0x080
#define IPMI_RESUME_VENDOR_PART_NUM_1_MSB_OFFSET            0x01
#define IPMI_RESUME_VENDOR_PART_NUM_2_LSB_OFFSET            0x22
#define IPMI_RESUME_VENDOR_PART_NUM_2_MSB_OFFSET            0x01

#define IPMI_RESUME_VENDOR_SERIAL_NUM_1_LSB_OFFSET          0x43 // EMC Resume vendor_serial_num - offset 0x0B0
#define IPMI_RESUME_VENDOR_SERIAL_NUM_1_MSB_OFFSET          0x01
#define IPMI_RESUME_VENDOR_SERIAL_NUM_2_LSB_OFFSET          0x53
#define IPMI_RESUME_VENDOR_SERIAL_NUM_2_MSB_OFFSET          0x01

#define IPMI_RESUME_VENDOR_LOC_MFT_1_LSB_OFFSET             0x64 // EMC Resume loc_mft - offset 0x120
#define IPMI_RESUME_VENDOR_LOC_MFT_1_MSB_OFFSET             0x01
#define IPMI_RESUME_VENDOR_LOC_MFT_2_LSB_OFFSET             0x74
#define IPMI_RESUME_VENDOR_LOC_MFT_2_MSB_OFFSET             0x01

#define IPMI_TOTAL_RESUME_FIELD_READS                       15   // Need to make 15 IPMI reads total
// RESUME constants end.

//++
// Sensor analog-reading related conversion-constants start.
// Change these values if SDR changes.
//--
// Transformers
#define IPMI_TEMP_CONVERSION_CONSTANT                       0x80
#define IPMI_FANSPEED_CONVERSION_CONSTANT                   0x78
#define IPMI_FANPWM_CONVERSION_CONSTANT                     0xA6
#define IPMI_PERCENTAGE_CONVERSION_CONSTANT                 0x64
#define IPMI_INPUT_POWER_CONVERSION_CONSTANT                0x08
#define IPMI_POWER_OLD_UNITS_CONVERSION_CONSTANT            0x0A
#define IPMI_POWER_OLD_UNITS_CONVERSION_CONSTANT_AS_EXP     0x01
#define IPMI_TIME_REMAINING_CONVERSION_CONSTANT             0x0A
#define IPMI_ECC_CONVERSION_CONSTANT                        0x01
#define IPMI_COOLING_MODULE_PWM_RPM_CONVERSION_CONSTANT     0xDC
#define IPMI_COOLING_MODULE_INOUT_RPM_CONVERSION_EXPRESSION 39 / 40 // the inlet fan RPM is 97.5% of outlet fan RPM

// Phobos
#define IPMI_P_TEMP_CONVERSION_CONSTANT                     0x00
#define IPMI_P_FANSPEED_CONVERSION_CONSTANT                 0xff
#define IPMI_P_INPUT_POWER_CONVERSION_CONSTANT              0x01

// Sensor analog-reading conversion-constants end.

//++
// Name:
//      IPMI_PEER_BMC_STATUS
//
// Description:
//      State that indicates if the peer BMC is accessible.
//--
typedef enum _IPMI_PEER_BMC_STATUS
{
    IPMI_PEER_BMC_STATUS_INACCESSIBLE        = 0x00,
    IPMI_PEER_BMC_STATUS_ACCESSIBLE          = 0x01,

    IPMI_PEER_BMC_STATUS_INVALID             = 0xFF
}
IPMI_PEER_BMC_STATUS, *PIPMI_PEER_BMC_STATUS;

//++
// Name:
//      IPMI_LED_BLINK_RATE
//
// Description:
//      LED blink rate definitions
//--
typedef enum _IPMI_LED_BLINK_RATE
{
   IPMI_LED_BLINK_QUARTER_HZ                = 0x00,
   IPMI_LED_BLINK_HALF_HZ                   = 0x01,
   IPMI_LED_BLINK_1_HZ                      = 0x02,
   IPMI_LED_BLINK_2_HZ                      = 0x03,
   IPMI_LED_BLINK_4_HZ                      = 0x04,
   IPMI_LED_BLINK_8_HZ                      = 0x05,
   IPMI_LED_ON                              = 0x06,
   IPMI_LED_OFF                             = 0x07,
   IPMI_LED_INVALID                         = 0xFF
} IPMI_LED_BLINK_RATE, *PIPMI_LED_BLINK_RATE;

//++
// Name:
//      IPMI_EVENT_CALSS
//
// Description:
//      IPMI Event Classes
//--
typedef enum _IPMI_EVENT_CALSS
{
    IPMI_EVENT_CLASS_DISCRETE,
    IPMI_EVENT_CLASS_DIGITAL,
    IPMI_EVENT_CLASS_THRESHOLD,
    IPMI_EVENT_CLASS_OEM,
} IPMI_EVENT_CALSS, *PIPMI_EVENT_CALSS;


//++
// Name:
//      IPMI_DATA_LENGTH_MAX
//
// Description:
//      XXX ???
//--
#define IPMI_DATA_LENGTH_MAX                  256

//++
// Name:
//      IPMI_RESPONSE_NETFN_MODIFIER
//
// Description:
//      The netfn of an IPMI response is the 'odd' value corresponding to the 'even' value
//      in the request header.
//--
#define IPMI_RESPONSE_NETFN_MODIFIER        0x4

//++
// Name:
//      IPMI_OFFSET_LENGTH_MAX
//
// Description:
//      The maximum size (in bytes) of any 'offset' field within IPMI commands.
//--
#define IPMI_OFFSET_LENGTH_MAX              3

//++
#define IPMI_SIZE_LENGTH_MAX                2
//++
// Constants for NVRAM IPMI command data lengths.
//--
#define IPMI_NVRAM_DATA_LENGTH_MAX            0xC4
typedef CHAR IPMI_NVRAM_DATA_LENGTH_MAX_ASSERT[(IPMI_NVRAM_DATA_LENGTH_MAX < (IPMI_DATA_LENGTH_MAX - IPMI_OFFSET_LENGTH_MAX)) ? 1 : -1];

#if 0
#define IPMI_READ_NVRAM_SIZE_MAX            (IPMI_DATA_LENGTH_MAX - IPMI_OFFSET_LENGTH_MAX)
#define IPMI_WRITE_NVRAM_SIZE_MAX           (IPMI_DATA_LENGTH_MAX - IPMI_OFFSET_LENGTH_MAX)
#endif

//++
// Name:
//      IPMI_SDR_FULL_RECORD_SIZE
//
// Description:
//      The size (in bytes) of a full SDR.
//--
#define IPMI_SDR_FULL_RECORD_SIZE           64

//++
// Name:
//      IPMI_SDR_MAX_DATA_SIZE
//
// Description:
//      The size (in bytes) of a full SDR minus the common header (5bytes)
//--
#define IPMI_SDR_MAX_DATA_SIZE              59

//++
// Name:
//      IPMI_SDR_COMPACT_RECORD_SIZE
//
// Description:
//      The size (in bytes) of a compact SDR.
//--
#define IPMI_SDR_COMPACT_RECORD_SIZE        48

//++
// Name:
//      IPMI_SEL_RECORD_SIZE
//
// Description:
//      The size (in bytes) of a compact SEL.
//--
#define IPMI_SEL_RECORD_SIZE        16

#define IPMI_SEL_UNSPEC_EVENT_TYPE                      0x00
#define IPMI_SEL_THRESHOLD_EVENT_TYPE                   0x01
#define IPMI_SEL_SENSOR_SPEC_EVENT_TYPE                 0x6F
#define IPMI_SEL_OEM_EVENT_TYPE_LOW_LIMIT               0x70
#define IPMI_SEL_OEM_EVENT_TYPE_HIGH_LIMIT              0x7F
#define IPMI_SEL_GENERIC_EVENT_TYPE_LOW_LIMIT           0x02
#define IPMI_SEL_GENERIC_EVENT_TYPE_HIGH_LIMIT          0x0C


#define IPMI_SEL_PLATYPUS_GENERATOR_ID            0x4F
#define IPMI_SEL_DATA_DOMAIN_GENERATOR_ID         0x4D
#define IPMI_SEL_SVE_GENERATOR_ID                 0x4B
#define IPMI_SEL_ISC_GENERATOR_ID                 0x49
#define IPMI_SEL_CENTERA_GENERATOR_ID             0x47
#define IPMI_SEL_SYMMETRIX_GENERATOR_ID           0x45
#define IPMI_SEL_CELERRA_GENERATOR_ID             0x43
#define IPMI_SEL_CLARIION_GENERATOR_ID            0x41
#define IPMI_SEL_ME_GENERATOR_ID                  0x2C
#define IPMI_SEL_BMC_GENERATOR_ID                 0x20
#define IPMI_SEL_BMC_GENERATOR_ID_SIDE_A          0x20
#define IPMI_SEL_BMC_GENERATOR_ID_SIDE_B          0x1E
#define IPMI_SEL_SSP_GENERATOR_ID                 0x22
#define IPMI_SEL_BIOS_GENERATOR_ID                0x01
#define IPMI_SEL_SMI_GENERATOR_ID                 0x33
#define IPMI_SEL_POST_GENERATOR_ID                0x61

#define  IPMI_SEL_DATA_IS_UNSPECIFIED             0x00
#define  IPMI_SEL_DATA1_OFFSET_MASK               0x0F
#define  IPMI_SEL_DATA2_IS_READING                0x40
#define  IPMI_SEL_DATA2_IS_OEM                    0x80
#define  IPMI_SEL_DATA2_IS_SENSOR_SPEC            0xC0
#define  IPMI_SEL_DATA3_IS_THRESHOLD              0x10
#define  IPMI_SEL_DATA3_IS_OEM                    0x20
#define  IPMI_SEL_DATA3_IS_SENSOR_SPEC            0x30
#define  IPMI_SEL_DATA2_DECODE_MASK               0xC0
#define  IPMI_SEL_DATA3_DECODE_MASK               0x30
#define  IPMI_SEL_GENERIC_OFFSET_MASK             0x0F

//++
//  SDR constants
//
#define IPMI_SDR_EVENT_READING_TYPE_THRESHOLD                 0x01

//++
// Name:
//      IPMI_SDR_LINEARIZATION
//
// Description:
//      Sensor Data Record Linearization Types
//--
typedef enum _IPMI_SDR_LINEARIZATION
{
    IPMI_SDR_SENSOR_L_LINEAR                    = 0x00,
    IPMI_SDR_SENSOR_L_LN                        = 0x01,
    IPMI_SDR_SENSOR_L_LOG10                     = 0x02,
    IPMI_SDR_SENSOR_L_LOG2                      = 0x03,
    IPMI_SDR_SENSOR_L_E                         = 0x04,
    IPMI_SDR_SENSOR_L_EXP10                     = 0x05,
    IPMI_SDR_SENSOR_L_EXP2                      = 0x06,
    IPMI_SDR_SENSOR_L_1_X                       = 0x07,
    IPMI_SDR_SENSOR_L_SQR                       = 0x08,
    IPMI_SDR_SENSOR_L_CUBE                      = 0x09,
    IPMI_SDR_SENSOR_L_SQRT                      = 0x0A,
    IPMI_SDR_SENSOR_L_CUBERT                    = 0x0B,
    IPMI_SDR_SENSOR_L_NONLINEAR                 = 0x70,
    IPMI_SDR_SENSOR_L_UNKNOWN                   = 0x7F,
} IPMI_SDR_LINEARIZATION, *PIPMI_SDR_LINEARIZATION;

#define IPMI_SDR_SENSOR_L_IS_LINEAR(fn)             ((fn) == IPMI_SDR_SENSOR_L_LINEAR)

#define IPMI_SDR_SENSOR_L_IS_LINEARIZED(fn)        (((fn) == IPMI_SDR_SENSOR_L_LN)    || \
                                                    ((fn) == IPMI_SDR_SENSOR_L_LOG10) || \
                                                    ((fn) == IPMI_SDR_SENSOR_L_LOG2)  || \
                                                    ((fn) == IPMI_SDR_SENSOR_L_E)     || \
                                                    ((fn) == IPMI_SDR_SENSOR_L_EXP10) || \
                                                    ((fn) == IPMI_SDR_SENSOR_L_EXP2)  || \
                                                    ((fn) == IPMI_SDR_SENSOR_L_1_X)   || \
                                                    ((fn) == IPMI_SDR_SENSOR_L_SQR)   || \
                                                    ((fn) == IPMI_SDR_SENSOR_L_CUBE)  || \
                                                    ((fn) == IPMI_SDR_SENSOR_L_SQRT)  || \
                                                    ((fn) == IPMI_SDR_SENSOR_L_CUBERT))
                                                 
#define IPMI_SDR_SENSOR_L_IS_NONLINEAR(fn)         ((fn) == IPMI_SDR_SENSOR_L_NONLINEAR)

#define IPMI_SDR_SENSOR_L_MASK       0x7F

#define IPMI_SDR_SENSOR_M_LS_MASK    0x00FF      //[7:0] - M: LS 8 bits
#define IPMI_SDR_SENSOR_M_MS_MASK    0xC000      //[15:14] - M: MS 2 bits
#define IPMI_SDR_SENSOR_M_MS_OFFSET  6           //shifting bits 14-15 to positions 8-9

#define IPMI_SDR_SENSOR_T_MASK       0x3F00      //[13:8] - tolerance: 6 bits

#define IPMI_SDR_SENSOR_B_LS_MASK    0x000000FF  //[7:0] - B: LS 8 bits
#define IPMI_SDR_SENSOR_B_MS_MASK    0x0000C000  //[15:14] - B: MS 2 bits
#define IPMI_SDR_SENSOR_B_MS_OFFSET  6           //shifting bits 14-15 to positions 8-9

#define IPMI_SDR_SENSOR_A_LS_MASK    0x00003F00  //[13:8] - accuracy: 6 bits
#define IPMI_SDR_SENSOR_A_MS_MASK    0x00F00000  //[23:20]- accuracy: 4 bits
#define IPMI_SDR_SENSOR_A_MS_OFFSET  14          //shifting bits 20-23 to positions 6-9

#define IPMI_SDR_SENSOR_A_EXP_MASK   0x000C0000  //[19:18]- accuracy exp: 2 bits
#define IPMI_SDR_SENSOR_DIR_MASK     0x00030000  //[17:16]- sensor direction: 2 bits
#define IPMI_SDR_SENSOR_R_EXP_MASK   0xF0000000  //[31:28]- R exponent: 4 bits.
#define IPMI_SDR_SENSOR_B_EXP_MASK   0x0F000000  //[27:24]- B exponent: 4 bits 

#define IPMI_SDR_RECORD_ID_FIRST           0x0000
#define IPMI_SDR_RECORD_ID_NONE            0xFFFF
#define IPMI_SDR_DEFAULT_RESERVATION_ID    0x0000

//++
// Name:
//      IPMI_SDR_RECORD_TYPE
//
// Description:
//      Sensor Record Types
//--
typedef enum _IPMI_SDR_RECORD_TYPE
{
    IPMI_SDR_RECORD_TYPE_FULL_SENSOR            = 0x01,
    IPMI_SDR_RECORD_TYPE_COMPACT_SENSOR         = 0x02,
    IPMI_SDR_RECORD_TYPE_EVENTONLY_SENSOR       = 0x03,
    IPMI_SDR_RECORD_TYPE_ENTITY_ASSOC           = 0x08,
    IPMI_SDR_RECORD_TYPE_DEVICE_ENTITY_ASSOC    = 0x09,
    IPMI_SDR_RECORD_TYPE_GENERIC_DEVICE_LOCATOR = 0x10,
    IPMI_SDR_RECORD_TYPE_FRU_DEVICE_LOCATOR     = 0x11,
    IPMI_SDR_RECORD_TYPE_MC_DEVICE_LOCATOR      = 0x12,
    IPMI_SDR_RECORD_TYPE_MC_CONFIRMATION        = 0x13,
    IPMI_SDR_RECORD_TYPE_BMC_MSG_CHANNEL_INFO   = 0x14,
    IPMI_SDR_RECORD_TYPE_OEM                    = 0xc0
} IPMI_SDR_RECORD_TYPE, *PIPMI_SDR_RECORD_TYPE;

//++
// Name:
//      IPMI_SDR_READING_TYPE
//
// Description:
//      Sensor Reading Types/Event Types
//--
typedef enum _IPMI_SDR_READING_TYPE
{
    IPMI_SDR_READING_TYPE_UNSPECIFIED           = 0x00,
    IPMI_SDR_READING_TYPE_THRESHOLD             = 0x01,
    IPMI_SDR_READING_TYPE_DISCRETE              = 0x6F
} IPMI_SDR_READING_TYPE, *PIPMI_SDR_READING_TYPE;

//++
// Name:
//      IPMI_SENSOR_TYPE
//
// Description:
//      Sensor Type Codes (Table 42-3).
//--
typedef enum _IPMI_SENSOR_TYPE
{
    SENSOR_TYPE_UNKNOWN                     = 0x00, 
    SENSOR_TYPE_TEMPERATURE                 = 0x01,
    SENSOR_TYPE_VOLTAGE                     = 0x02, 
    SENSOR_TYPE_CURRENT                     = 0x03,
    SENSOR_TYPE_FAN                         = 0x04, 
    SENSOR_TYPE_PHYSICAL_SECURITY           = 0x05,
    SENSOR_TYPE_PLATFORM_SECURITY           = 0x06, 
    SENSOR_TYPE_PROCESSOR                   = 0x07,
    SENSOR_TYPE_POWER_SUPPLY                = 0x08, 
    SENSOR_TYPE_POWER_UNIT                  = 0x09,
    SENSOR_TYPE_COOLING_DEVICE              = 0x0A, 
    SENSOR_TYPE_UNITS_BASED                 = 0x0B,
    SENSOR_TYPE_MEMORY                      = 0x0C, 
    SENSOR_TYPE_DRIVE_SLOT                  = 0x0D,
    SENSOR_TYPE_POST_MEMORY_RESIZE          = 0x0E, 
    SENSOR_TYPE_POST_ERROR                  = 0x0F,
    SENSOR_TYPE_EVTLOG_DISABLED             = 0x10, 
    SENSOR_TYPE_WATCHDOG_1                  = 0x11,
    SENSOR_TYPE_SYSTEM_EVENT                = 0x12, 
    SENSOR_TYPE_CRITICAL_INTERRUPT          = 0x13,
    SENSOR_TYPE_BUTTON                      = 0x14, 
    SENSOR_TYPE_MODULE                      = 0x15,
    SENSOR_TYPE_COPROCESSOR                 = 0x16, 
    SENSOR_TYPE_ADDIN_CARD                  = 0x17,
    SENSOR_TYPE_CHASSIS                     = 0x18, 
    SENSOR_TYPE_CHIP_SET                    = 0x19,
    SENSOR_TYPE_OTHER_FRU                   = 0x1A, 
    SENSOR_TYPE_CABLE                       = 0x1B,
    SENSOR_TYPE_TERMINATOR                  = 0x1C, 
    SENSOR_TYPE_SYSTEM_BOOT                 = 0x1D,
    SENSOR_TYPE_BOOT_ERROR                  = 0x1E, 
    SENSOR_TYPE_OS_BOOT                     = 0x1F,
    SENSOR_TYPE_OS_STOP_OR_SHUTDOWN         = 0x20, 
    SENSOR_TYPE_SLOT_CONNECTOR              = 0x21,
    SENSOR_TYPE_ACPI_POWER_STATE            = 0x22, 
    SENSOR_TYPE_WATCHDOG_2                  = 0x23,
    SENSOR_TYPE_PLATFORM_ALERT              = 0x24, 
    SENSOR_TYPE_ENTITY_PRESENCE             = 0x25,
    SENSOR_TYPE_MLANMONITOR_ASIC            = 0x26, 
    SENSOR_TYPE_LAN                         = 0x27,
    SENSOR_TYPE_SUBSYSTEM_HEALTH            = 0x28, 
    SENSOR_TYPE_BATTERY                     = 0x29,
    SENSOR_TYPE_SESSION_AUDIT               = 0x2A, 
    SENSOR_TYPE_VERSION_CHANGE              = 0x2B,
    SENSOR_TYPE_FRU_STATE                   = 0x2C,
    SENSOR_TYPE_RESERVED                    = 0x2D,
    SENSOR_TYPE_POWER_RESET_ACTION          = 0xC0,  // OEM Sensor Starts
    SENSOR_TYPE_BMC_INTERNAL_HW_WATCHDOG    = 0xC1,
    SENSOR_TYPE_FW_UPDATE_ACTION            = 0xC2,
    SENSOR_TYPE_FAN_TEST_FAULT              = 0xC8,
    SENSOR_TYPE_CMD_LOG                     = 0xC9,
    SENSOR_TYPE_MFB_EVENT                   = 0xCA,
    SENSOR_TYPE_ECC_COUNT                   = 0xCB,
    SENSOR_TYPE_ARBITRATION_FAULT           = 0xD0,
    SENSOR_TYPE_TIME_REMAINING              = 0xD1,
    SENSOR_TYPE_SHUTDOWN_IN_PROGRESS        = 0xD2,
    SENSOR_TYPE_I2C_FAULT                   = 0xD3,
    SENSOR_TYPE_FAULT_STATUS_REGION         = 0xD4,
    SENSOR_TYPE_SEL_RATE                    = 0xD5,
    SENSOR_TYPE_ADAPTIVE_COOLING_EVENT      = 0xD6,
    SENSOR_TYPE_SLIC_STATUS                 = 0xE0,
    SENSOR_TYPE_SP_STATUS                   = 0xE1,
    SENSOR_TYPE_SP_CMD_WORKING_STATUS_LSB   = 0xE2,
    SENSOR_TYPE_SP_CMD_WORKING_STATUS_MSB   = 0xE3,
    SENSOR_TYPE_SP_CMD_WORKING_STATUS_PAGE2 = 0xE4,
    SENSOR_TYPE_SP_CMD_THERMAL_STATUS       = 0xE5,  // Jetfire
    SENSOR_TYPE_SP_CMD_THERMAL_STATUS_LSB   = 0xE5,  // Megatron, Devastator, Beachcomer
    SENSOR_TYPE_SP_CMD_THERMAL_STATUS_MSB   = 0xE6,  // Megatron, Devastator, Beachcomer
    SENSOR_TYPE_SP_CMD_POL_FAULTS           = 0xE7,  // Jetfire
    SENSOR_TYPE_SP_CMD_APOL_FAULTS          = 0xE7,  // Megatron, Devastator, Beachcomer
    SENSOR_TYPE_SP_CMD_VRM_FAULTS           = 0xE8,  // Jetfire
    SENSOR_TYPE_SP_CMD_DPOL_FAULTS_LSB      = 0xE8,  // Megatron, Devastator, Beachcomer
    SENSOR_TYPE_SP_CMD_DPOL_FAULTS_MSB      = 0xE9,  // Megatron, Devastator, Beachcomer
    SENSOR_TYPE_SP_CMD_ANALOG_FAULTS        = 0xE9,  // Jetfire
    SENSOR_TYPE_SP_CMD_COMPARATOR_FAULTS    = 0xEA,  // Jetfire
    SENSOR_TYPE_SP_CMD_PS_GPIO_EXPANDER     = 0xEA,  // Megatron, Devastator
    SENSOR_TYPE_CPU_GPIO_EXPANDER           = 0xEB,
    SENSOR_TYPE_SLIC_MM_OP_FLAGS_LSB        = 0xEC,
    SENSOR_TYPE_SLIC_MM_OP_FLAGS_MSB        = 0xED,
    SENSOR_TYPE_POWER_CONSUMPTION           = 0xEE,
    SENSOR_TYPE_MM_GPIO_SENSOR              = 0xEF,
    SENSOR_TYPE_SP_CMD_ANALOG_G_FAULT       = 0xF0,
    SENSOR_TYPE_BEM_CMD_WORKING_CONDITIONS  = 0xF2,
    SENSOR_TYPE_BATTERY_STATUS_I2C          = 0xF3,
    SENSOR_TYPE_BATTERY_STATUS_GPIO         = 0xF4,
    SENSOR_TYPE_BEM_CMD_OP_FLAGS_LSB        = 0xF5,
    SENSOR_TYPE_BEM_CMD_OP_FLAGS_MSB        = 0xF6,
    SENSOR_TYPE_MM_STATUS                   = 0xF8,
    SENSOR_TYPE_BMC_BOOT_PROGRESS           = 0xF9,
    SENSOR_TYPE_AIRWAVE_STATUS              = 0xFA,
    SENSOR_TYPE_BEM_POWER_FAULTS            = 0xFB,
    SENSOR_TYPE_FAN_FAULT_STATUS            = 0xFC,
    SENSOR_TYPE_OEM_RESERVED                = 0xFF   // OEM Sensor Ends
} IPMI_SENSOR_TYPE, *PIPMI_SENSOR_TYPE;

//++
// Name:
//      IPMI_SDR_SENSOR_UNIT_ANALOG_DATA_FORMAT
//
// Description:
//      Sensor Unit Data Format
//--
typedef enum _IPMI_SDR_SENSOR_UNIT_ANALOG_DATA_FORMAT
{
    IPMI_SDR_SENSOR_UNIT_ANALOG_DATA_FORMAT_UNSIGNED           = 0x00,
    IPMI_SDR_SENSOR_UNIT_ANALOG_DATA_FORMAT_SIGNED_1           = 0x01,  /* 1's complement */
    IPMI_SDR_SENSOR_UNIT_ANALOG_DATA_FORMAT_SIGNED_2           = 0x02   /* 2's complement */
} IPMI_SDR_SENSOR_UNIT_ANALOG_DATA_FORMAT, *PIPMI_SDR_SENSOR_UNIT_ANALOG_DATA_FORMAT;

//++
// Name:
//      IPMI_SENSOR_UNIT_TYPE
//
// Description:
//      Sensor Unit Type Codes (Table 43-15).
//--
typedef enum _IPMI_SENSOR_UNIT_TYPE
{
    SENSOR_UNIT_TYPE_UNKNOWN                = 0, 
    SENSOR_UNIT_TYPE_DEGREES_C              = 1,
    SENSOR_UNIT_TYPE_DEGREES_F              = 2, 
    SENSOR_UNIT_TYPE_DEGREES_K              = 3,
    SENSOR_UNIT_TYPE_VOLTS                  = 4, 
    SENSOR_UNIT_TYPE_AMPS                   = 5,
    SENSOR_UNIT_TYPE_WATTS                  = 6, 
    SENSOR_UNIT_TYPE_JOULES                 = 7,
    SENSOR_UNIT_TYPE_COULOMBS               = 8, 
    SENSOR_UNIT_TYPE_VA                     = 9,
    SENSOR_UNIT_TYPE_NITS                   = 10, 
    SENSOR_UNIT_TYPE_LUMEN                  = 11,
    SENSOR_UNIT_TYPE_LUX                    = 12, 
    SENSOR_UNIT_TYPE_CANDELA                = 13,
    SENSOR_UNIT_TYPE_KPA                    = 14, 
    SENSOR_UNIT_TYPE_PSA                    = 15,
    SENSOR_UNIT_TYPE_NEWTON                 = 16, 
    SENSOR_UNIT_TYPE_CFM                    = 17,
    SENSOR_UNIT_TYPE_RPM                    = 18, 
    SENSOR_UNIT_TYPE_HZ                     = 19,
    SENSOR_UNIT_TYPE_MICROSECOND            = 20, 
    SENSOR_UNIT_TYPE_MILLISECOND            = 21,
    SENSOR_UNIT_TYPE_SECOND                 = 22, 
    SENSOR_UNIT_TYPE_MINUTE                 = 23,
    SENSOR_UNIT_TYPE_HOUR                   = 24, 
    SENSOR_UNIT_TYPE_DAY                    = 25,
    SENSOR_UNIT_TYPE_WEEK                   = 26, 
    SENSOR_UNIT_TYPE_MIL                    = 27,
    SENSOR_UNIT_TYPE_INCHES                 = 28, 
    SENSOR_UNIT_TYPE_FEET                   = 29,
    SENSOR_UNIT_TYPE_CU_IN                  = 30, 
    SENSOR_UNIT_TYPE_CU_FEET                = 31,
    SENSOR_UNIT_TYPE_MM                     = 32, 
    SENSOR_UNIT_TYPE_CM                     = 33,
    SENSOR_UNIT_TYPE_M                      = 34, 
    SENSOR_UNIT_TYPE_CU_CM                  = 35,
    SENSOR_UNIT_TYPE_CU_M                   = 36, 
    SENSOR_UNIT_TYPE_LITERS                 = 37,
    SENSOR_UNIT_TYPE_FLUID_OUNCE            = 38, 
    SENSOR_UNIT_TYPE_RADIANS                = 39,
    SENSOR_UNIT_TYPE_STERADIANS             = 40, 
    SENSOR_UNIT_TYPE_REVOLUTIONS            = 41,
    SENSOR_UNIT_TYPE_CYCLES                 = 42, 
    SENSOR_UNIT_TYPE_GRAVITIES              = 43,
    SENSOR_UNIT_TYPE_OUNCE                  = 44, 
    SENSOR_UNIT_TYPE_POUND                  = 45, 
    SENSOR_UNIT_TYPE_FT_LB                  = 46,
    SENSOR_UNIT_TYPE_OZ_IN                  = 47, 
    SENSOR_UNIT_TYPE_GAUSS                  = 48,
    SENSOR_UNIT_TYPE_GILBERTS               = 49, 
    SENSOR_UNIT_TYPE_HENRY                  = 50,
    SENSOR_UNIT_TYPE_MILLIHENRY             = 51, 
    SENSOR_UNIT_TYPE_FARAD                  = 52,
    SENSOR_UNIT_TYPE_MICROFARAD             = 53, 
    SENSOR_UNIT_TYPE_OHMS                   = 54,
    SENSOR_UNIT_TYPE_SIEMENS                = 55, 
    SENSOR_UNIT_TYPE_MOLE                   = 56,
    SENSOR_UNIT_TYPE_BECQUEREL              = 57, 
    SENSOR_UNIT_TYPE_PPM                    = 58,
    SENSOR_UNIT_TYPE_RESERVED               = 59, 
    SENSOR_UNIT_TYPE_DECIBELS               = 60,
    SENSOR_UNIT_TYPE_DBA                    = 61, 
    SENSOR_UNIT_TYPE_DBC                    = 62,
    SENSOR_UNIT_TYPE_GRAY                   = 63, 
    SENSOR_UNIT_TYPE_SIEVERT                = 64,
    SENSOR_UNIT_TYPE_COLOR_TEMP_DEG_K       = 65, 
    SENSOR_UNIT_TYPE_BIT                    = 66,
    SENSOR_UNIT_TYPE_KILOBIT                = 67, 
    SENSOR_UNIT_TYPE_MEGABIT                = 68,
    SENSOR_UNIT_TYPE_GIGABIT                = 69, 
    SENSOR_UNIT_TYPE_BYTE                   = 70,
    SENSOR_UNIT_TYPE_KILOBYTE               = 71, 
    SENSOR_UNIT_TYPE_MEGABYTE               = 72,
    SENSOR_UNIT_TYPE_GIGABYTE               = 73, 
    SENSOR_UNIT_TYPE_WORD                   = 74,
    SENSOR_UNIT_TYPE_DWORD                  = 75, 
    SENSOR_UNIT_TYPE_QWORD                  = 76,
    SENSOR_UNIT_TYPE_LINE                   = 77, 
    SENSOR_UNIT_TYPE_HIT                    = 78,
    SENSOR_UNIT_TYPE_MISS                   = 79, 
    SENSOR_UNIT_TYPE_RETRY                  = 80,
    SENSOR_UNIT_TYPE_RESET                  = 81, 
    SENSOR_UNIT_TYPE_OVERRUN                = 82,
    SENSOR_UNIT_TYPE_UNDERRUN               = 83, 
    SENSOR_UNIT_TYPE_COLLISION              = 84,
    SENSOR_UNIT_TYPE_PACKETS                = 85, 
    SENSOR_UNIT_TYPE_MESSAGES               = 86,
    SENSOR_UNIT_TYPE_CHARACTERS             = 87, 
    SENSOR_UNIT_TYPE_ERROR                  = 88,
    SENSOR_UNIT_TYPE_CORRECTABLE_ERROR      = 89,
    SENSOR_UNIT_TYPE_UNCORRECTABLE_ERROR    = 90, 
    SENSOR_UNIT_TYPE_FATAL_ERROR            = 91, 
    SENSOR_UNIT_TYPE_GRAMS                  = 92,
    SENSOR_UNIT_TYPE_INVALID                = 255
} IPMI_SENSOR_UNIT_TYPE, *PIPMI_SENSOR_UNIT_TYPE;
//++
// Name:
//      IPMI_SENSOR_ENTITY_ID
//
// Description:
//      Sensor Entiry ID codes (Table 43-13).
//--
typedef enum _IPMI_SENSOR_ENTITY_ID
{
    SENSOR_ENTITY_ID_UNSPECIFIED                = 0, 
    SENSOR_ENTITY_ID_OTHER                      = 1,
    SENSOR_ENTITY_ID_UNKNOWN                    = 2, 
    SENSOR_ENTITY_ID_PROCESSOR                  = 3,
    SENSOR_ENTITY_ID_DISK                       = 4, 
    SENSOR_ENTITY_ID_PERIPHERAL_BAY1            = 5,
    SENSOR_ENTITY_ID_SYSTEM_MANAGEMENT_MODULE   = 6, 
    SENSOR_ENTITY_ID_SYSTEM_BOARD               = 7,
    SENSOR_ENTITY_ID_MEMORY_MODULE              = 8, 
    SENSOR_ENTITY_ID_PROCESSOR_MODULE           = 9,
    SENSOR_ENTITY_ID_POWER_SUPPLY               = 10, 
    SENSOR_ENTITY_ID_ADDIN_CARD                 = 11,
    SENSOR_ENTITY_ID_FRONT_PANEL_BOARD          = 12, 
    SENSOR_ENTITY_ID_BACK_PANEL_BOARD           = 13,
    SENSOR_ENTITY_ID_POWER_SYSTEM_BOARD         = 14, 
    SENSOR_ENTITY_ID_DRIVE_BACKPLANE            = 15,

    SENSOR_ENTITY_ID_SYSTEM_INTERNAL_EXP_BOARD  = 16, 
    SENSOR_ENTITY_ID_OTHER_SYSTEM_BOARD         = 17,
    SENSOR_ENTITY_ID_PROCESSOR_BOARD            = 18, 
    SENSOR_ENTITY_ID_POWER_UNIT                 = 19,
    SENSOR_ENTITY_ID_POWER_MODULE               = 20, 
    SENSOR_ENTITY_ID_POWER_MANAGEMENT           = 21,
    SENSOR_ENTITY_ID_CHASSIS_BACK_PANEL_BOARD   = 22, 
    SENSOR_ENTITY_ID_SYSTEM_CHASSIS             = 23,
    SENSOR_ENTITY_ID_SUB_CHASSIS                = 24, 
    SENSOR_ENTITY_ID_OTHER_CHASSIS_BOARD        = 25,
    SENSOR_ENTITY_ID_DISK_DRIVE_BAY             = 26, 
    SENSOR_ENTITY_ID_PERIPHERAL_BAY2            = 27,
    SENSOR_ENTITY_ID_DEVICE_BAY                 = 28, 
    SENSOR_ENTITY_ID_FAN_DEVICE                 = 29,
    SENSOR_ENTITY_ID_COOLING_UNIT               = 30, 
    SENSOR_ENTITY_ID_CABLE_INTERCONNECT         = 31,
    SENSOR_ENTITY_ID_MEMORY_DEVICE              = 32, 
    SENSOR_ENTITY_ID_SYSTEM_MANAGEMENT_SOFTWARE = 33,
    SENSOR_ENTITY_ID_SYSTEM_FIRMWARE            = 34, 
    SENSOR_ENTITY_ID_OPERATING_SYSTEM           = 35,
    SENSOR_ENTITY_ID_SYSTEM_BUS                 = 36, 
    SENSOR_ENTITY_ID_GROUP                      = 37,
    SENSOR_ENTITY_ID_OOB_MANAGEMENT_DEVICE      = 38, 
    SENSOR_ENTITY_ID_EXTERNAL_ENVIRONMENT       = 39,
    SENSOR_ENTITY_ID_BATTERY                    = 40, 
    SENSOR_ENTITY_ID_PROCESSING_BLADE           = 41,
    SENSOR_ENTITY_ID_CONNECTIVITY_SWITCH        = 42, 
    SENSOR_ENTITY_ID_PROCESSOR_MEMORY_MODULE    = 43,
    SENSOR_ENTITY_ID_IO_MODULE                  = 44, 
    SENSOR_ENTITY_ID_PROCESSOR_IO_MODULE        = 45,
    SENSOR_ENTITY_ID_MANAGEMENT_CONTROLLER_FW   = 46, 
    SENSOR_ENTITY_ID_IPMI_CHANNEL               = 47,
    SENSOR_ENTITY_ID_PCI_BUS                    = 48, 
    SENSOR_ENTITY_ID_PCI_EXPRESS_BUS            = 49,
    SENSOR_ENTITY_ID_SCSI_BUS                   = 50, 
    SENSOR_ENTITY_ID_SATA_SAS_BUS               = 51,
    SENSOR_ENTITY_ID_PROCSSOR_FRONT_SIDE_BUS    = 52, 
    SENSOR_ENTITY_ID_REAL_TIME_CLOCK            = 53,
    SENSOR_ENTITY_ID_RESERVED0                  = 54, 
    SENSOR_ENTITY_ID_AIR_INLET1                 = 55,
    SENSOR_ENTITY_ID_RESERVED1                  = 56, 
    SENSOR_ENTITY_ID_RESERVED2                  = 57, 
    SENSOR_ENTITY_ID_RESERVED3                  = 58, 
    SENSOR_ENTITY_ID_RESERVED4                  = 59, 
    SENSOR_ENTITY_ID_RESERVED5                  = 60, 
    SENSOR_ENTITY_ID_RESERVED6                  = 61, 
    SENSOR_ENTITY_ID_RESERVED7                  = 62, 
    SENSOR_ENTITY_ID_RESERVED8                  = 63, 
    SENSOR_ENTITY_ID_AIR_INLET2                 = 64, 
    SENSOR_ENTITY_ID_PROCESSOR_CPU              = 65,
    SENSOR_ENTITY_ID_BASEBOARD                  = 66,
    
    SENSOR_ENTITY_ID_CHASSIS_SPECIFIC_START     = 144, 
    SENSOR_ENTITY_ID_CHASSIS_SPECIFIC_END       = 175,
    
    SENSOR_ENTITY_ID_BOARD_SET_SPECIFIC_START   = 176, 
    SENSOR_ENTITY_ID_BOARD_SET_SPECIFIC_END     = 207,
    
    SENSOR_ENTITY_ID_OEM_SPECIFIC_START         = 208, 
    SENSOR_ENTITY_ID_OEM_SPECIFIC_END           = 255
} IPMI_SENSOR_ENTITY_ID, *PIPMI_SENSOR_ENTITY_ID;

//++
// Name:
//      IPMI_SDR_SENSOR_THRESHOLD_ACCESS_CAPABILITY
//
// Description:
//      Sensor Threshold Access Support Capability.
//--
typedef enum _IPMI_SDR_SENSOR_THRESHOLD_ACCESS_CAPABILITY
{
    IPMI_SDR_SENSOR_NO_THRESHOLD                = 0, 
    IPMI_SDR_SENSOR_THRESHOLD_READABLE          = 1,
    IPMI_SDR_SENSOR_THRESHOLD_READABLE_SETTABLE = 2, 
    IPMI_SDR_SENSOR_THRESHOLD_FIXED_UNREADABLE  = 3
} IPMI_SDR_SENSOR_THRESHOLD_ACCESS_CAPABILITY, *PIPMI_SDR_SENSOR_THRESHOLD_ACCESS_CAPABILITY;

//++
// Name:
//      IPMI_SDR_SENSOR_HYSTERESIS_CAPABILITY
//
// Description:
//      Sensor Hysteresis Support
//--
typedef enum _IPMI_SDR_SENSOR_HYSTERESIS_CAPABILITY
{
    IPMI_SDR_SENSOR_NO_HYSTERESIS                = 0, 
    IPMI_SDR_SENSOR_HYSTERESIS_READABLE          = 1,
    IPMI_SDR_SENSOR_HYSTERESIS_READABLE_SETTABLE = 2, 
    IPMI_SDR_SENSOR_HYSTERESIS_FIXED_UNREADABLE  = 3
} IPMI_SDR_SENSOR_HYSTERESIS_CAPABILITY, *PIPMI_SDR_SENSOR_HYSTERESIS_CAPABILITY;

//++
// Name:
//      IPMI_SDR_SENSOR_EVENT_MSG_CONTROL_CAPABILITY
//
// Description:
//      Senosr Event Message Control Support Capability.
//      Indicates whether this sensor generates Event Messages.
//--
typedef enum _IPMI_SDR_SENSOR_EVENT_MSG_CONTROL_CAPABILITY
{
    IPMI_SENSOR_EVENT_MSG_CONTROL_PER_THRESHOLD       = 0, 
    IPMI_SENSOR_EVENT_MSG_CONTROL_ENTIRE_SENSOR       = 1,
    IPMI_SENSOR_EVENT_MSG_CONTROL_GLOBAL_DISABLE_ONLY = 2, 
    IPMI_SENSOR_EVENT_MSG_CONTROL_NO_EVENT            = 3
} IPMI_SDR_SENSOR_EVENT_MSG_CONTROL_CAPABILITY, *PIPMI_SDR_SENSOR_EVENT_MSG_CONTROL_CAPABILITY;

//++
// Name:
//      IPMI_SDR_SENSOR_DIRECTION
//
// Description:
//      Sensor Direction Codes.
//--
typedef enum _IPMI_SDR_SENSOR_DIRECTION
{
    SENSOR_DIR_UNSPECIFIED        = 0, 
    SENSOR_DIR_INPUT              = 1,
    SENSOR_DIR_OUTPUT             = 2, 
    SENSOR_DIR_RESERVED           = 3
} IPMI_SDR_SENSOR_DIRECTION, *PIPMI_SDR_SENSOR_DIRECTION;

//++
//  BATTERY constants
//
#define IPMI_BATTERY_CLEAR_FAULTS_SUBCOMMAND                 0x11

//++
// Transport command constants
//
#define IPMI_BMC_LAN_CHANNEL_1                               0x01
#define IPMI_BMC_PARAMETER_SELECTOR_1                        0x03
#define IPMI_BMC_PARAMETER_SELECTOR_2                        0x04
#define IPMI_BMC_PARAMETER_SELECTOR_3                        0x06
#define IPMI_BMC_PARAMETER_SELECTOR_4                        0x0C
#define IPMI_BMC_SET_SELECTOR                                0x00
#define IPMI_BMC_BLOCK_SELECTOR                              0x00

//++
//.End Constants
//--

//++
// Macros
//--

//++
// Name:
//      IPMI_NETFN_LUN_TO_BYTE
//
// Description:
//      Combines the netfn and lun fields to fit in the IPMI header byte.
//
//--
#define IPMI_NETFN_LUN_TO_BYTE(netfn, lun) \
    ((((netfn) << 2) & 0xFC) | ((lun) & 0x03))

//++
// Name:
//      IPMI_BYTE_TO_NETFN_LUN
//
// Description:
//      Separates the IPMI header byte to netfn and lun.
//
//--
#define IPMI_BYTE_TO_NETFN_LUN(byte,netfn, lun) \
    (netfn) = (((byte) >> 2) & 0x3F),           \
    (lun) = ((byte) & 0x03)

//++
// Name:
//      IPMI_BYTE_TO_NETFN
//
// Description:
//      return the high 6 bits as netfn.
//
//--
#define IPMI_BYTE_TO_NETFN(byte) \
    (((byte) >> 2) & 0x3F)

//++
// Name:
//      IPMI_BYTE_TO_LUN
//
// Description:
//       return the low 2 bits as lun.
//
//--
#define IPMI_BYTE_TO_LUN(byte) \
    ((byte) & 0x03)

//++
//.End Macros
//--

//++
// Data Structures
//--

#pragma pack(push, 1)

//++
// Name:
//      IPMI_REQUEST_HEADER
//
// Description:
//      Defines the header for an IPMI request.
//
// Notes:
//      See IPMI spec 2.0.
//--
typedef struct _IPMI_REQUEST_HEADER
{
    //
    // Netfn code for the IPMI request.
    //
    UCHAR        netfn;

    //
    // Command code for the IPMI request.
    //
    UCHAR        cmd;
}
IPMI_REQUEST_HEADER, *PIPMI_REQUEST_HEADER;

//++
// Name:
//      IPMI_RESPONSE_HEADER
//
// Description:
//      Defines the header for an IPMI response.
//
// Notes:
//      See IPMI spec 2.0.
//--
typedef struct _IPMI_RESPONSE_HEADER
{
    //
    // Netfn code for the IPMI request that this response is for.
    //
    UCHAR        netfn;

    //
    // Command code for the IPMI request that this response is for.
    //
    UCHAR        cmd;

    //
    // Completion code for the response.
    //
    UCHAR        completionCode;
}
IPMI_RESPONSE_HEADER, *PIPMI_RESPONSE_HEADER;

//++
// Name:
//      IPMI_REQUEST
//
// Description:
//      An IPMI request.
//--
typedef struct _IPMI_REQUEST
{
    //
    // The header contains the netfn and cmd.
    //
    IPMI_REQUEST_HEADER     header;

    //
    // Length of the IPMI command data.
    //
    USHORT                  dataLength;

    //
    // Data for the IPMI command.
    //
    UCHAR                   data [IPMI_DATA_LENGTH_MAX];

}
IPMI_REQUEST, *PIPMI_REQUEST;

//++
// Name:
//      IPMI_RESPONSE
//
// Description:
//      An IPMI response.
//--
typedef struct _IPMI_RESPONSE
{
    //
    // The header contains the netfn, cmd and completion code.
    //
    IPMI_RESPONSE_HEADER    header;

    //
    // Length of the response data.
    //
    USHORT                  dataLength;

    //
    // Response data.
    //
    UCHAR                   data [IPMI_DATA_LENGTH_MAX];

}
IPMI_RESPONSE, *PIPMI_RESPONSE;

#if 0
//++
// Name:
//      IPMI_SEND_REQUEST_IN_BUF
//
// Description:
//      Describes the input buffer for the IPMI 'Send Message' IOCTL.
//
// Notes:
//      TBD - XXX
//--
typedef struct _IPMI_SEND_REQUEST_IN_BUF
{
    //
    // IPMI interface revision.  Must be set to IPMI_API_CURRENT_REVISION.
    //
    ULONG                           revisionId;

    //
    // IPMI request to send.
    //
    IPMI_REQUEST                    request;

}
IPMI_SEND_REQUEST_IN_BUF, *PIPMI_SEND_REQUEST_IN_BUF;
#endif

typedef struct _SENSOR_READING_SENSORSTATUS_BITS
{
    UINT_8E     reserved_0:5,
                readingUnavailable:1, //1=reading unavailable, so 0xCX is good, but 0x2X or 0x3X or 0xEX is bad.
                scanningDisabled:1, //0=disabled.
                allEventMessagesDisabled:1; //0=disabled.
} SENSOR_READING_SENSORSTATUS_BITS;

typedef union _SENSOR_READING_SENSORSTATUS_BITS_RAW_INFO
{
    UINT_8E                             StatusControl;
    SENSOR_READING_SENSORSTATUS_BITS    fields;
} SENSOR_READING_SENSORSTATUS_BITS_RAW_INFO;

//++
// Name:
//      IPMI_SEND_REQUEST_IN_OUT_BUF
//
// Description:
//      Describes the outpu buffer for the IPMI 'Send Message' IOCTL.
//
// Notes:
//      TBD - XXX
//--
#define _IPMI_SEND_REQUEST_IN_BUF        _IPMI_REQUEST
#define IPMI_SEND_REQUEST_IN_BUF         IPMI_REQUEST
#define PIPMI_SEND_REQUEST_IN_BUF        PIPMI_REQUEST
#define _IPMI_SEND_REQUEST_OUT_BUF       _IPMI_RESPONSE
#define IPMI_SEND_REQUEST_OUT_BUF        IPMI_RESPONSE
#define PIPMI_SEND_REQUEST_OUT_BUF       PIPMI_RESPONSE

//++
// Name:
//      IPMI_RESPONSE_GET_DEVICE_ID
//
// Description:
//      TBD - XXX
//--
typedef struct _IPMI_RESPONSE_GET_DEVICE_ID
{
    //
    // XXX
    //
    UCHAR           deviceId;
    UCHAR           deviceRevision;
    UCHAR           majorFirmwareRev;
    UCHAR           minorFirmwareRev;
    UCHAR           ipmiVersion;
    UCHAR           additionalDevSupport;
    UCHAR           mfgId [3];
    UCHAR           productId [2];
    UCHAR           auxFirmwareRev [4];
}
IPMI_RESPONSE_GET_DEVICE_ID, *PIPMI_RESPONSE_GET_DEVICE_ID;

//++
// Name:
//      IPMI_RESPONSE_GET_DEVICE_GUID
//
// Description:
//      TBD - XXX
//--
typedef struct _IPMI_RESPONSE_GET_DEVICE_GUID
{
    //
    // XXX
    //
    UCHAR           node [6];
    UCHAR           clockSeq [2];
    UCHAR           timeHigh [2];
    UCHAR           timeMid [2];
    UCHAR           timeLow [4];
}
IPMI_RESPONSE_GET_DEVICE_GUID, *PIPMI_RESPONSE_GET_DEVICE_GUID;

//++
// Name:
//      IPMI_RESPONSE_GET_SELF_TEST_RESULTS
//
// Description:
//      TBD - XXX
//--
typedef struct _IPMI_RESPONSE_GET_SELF_TEST_RESULTS
{
    //
    // XXX
    //
    UCHAR           code [2];
}
IPMI_RESPONSE_GET_SELF_TEST_RESULTS, *PIPMI_RESPONSE_GET_SELF_TEST_RESULTS;

//++
// Name:
//      IPMI_RESPONSE_GET_GLOBAL_ENABLES
//
// Description:
//      TBD - XXX
//--
typedef struct _IPMI_RESPONSE_GET_GLOBAL_ENABLES
{
    //
    // XXX
    //
    UCHAR           data;
}
IPMI_RESPONSE_GET_GLOBAL_ENABLES, *PIPMI_RESPONSE_GET_GLOBAL_ENABLES;

//++
// Name:
//      IPMI_REQUEST_SSP_POWER_RESET
//
// Description:
//      TBD - XXX
//--
typedef struct _IPMI_REQUEST_SSP_POWER_RESET
{
    //
    // XXX
    //
    UCHAR           action;
}
IPMI_REQUEST_SSP_POWER_RESET, *PIPMI_REQUEST_SSP_POWER_RESET;

//++
// Name:
//      IPMI_RESPONSE_SSP_POWER_RESET
//
// Description:
//      TBD - XXX
//--
typedef struct _IPMI_RESPONSE_SSP_POWER_RESET
{
    //
    // XXX
    //
    UCHAR           result;
}
IPMI_RESPONSE_SSP_POWER_RESET, *PIPMI_RESPONSE_SSP_POWER_RESET;

//++
// Name:
//      IPMI_REQUEST_NVRAM_READ
//
// Description:
//      Data portion of 'Read NVRAM' IPMI request.
//--
typedef struct _IPMI_REQUEST_NVRAM_READ
{
    //
    // Number of bytes to read, LSB first.
    //
    union
    {
        USHORT          s;
        UCHAR           c [IPMI_SIZE_LENGTH_MAX];
    } bytesToRead;

    //
    // Starting offset for the read, LSB first.
    //
    UCHAR            offset [IPMI_OFFSET_LENGTH_MAX];
}
IPMI_REQUEST_NVRAM_READ, *PIPMI_REQUEST_NVRAM_READ;

//++
// Name:
//      IPMI_RESPONSE_NVRAM_READ
//
// Description:
//      Data portion of 'Read NVRAM' IPMI response.
//--
typedef struct _IPMI_RESPONSE_NVRAM_READ
{
    //
    // Starting offset for the read, LSB first.
    //
    UCHAR            offset [IPMI_OFFSET_LENGTH_MAX];

    //
    // Data read from nvram.
    //
    UCHAR            data [IPMI_NVRAM_DATA_LENGTH_MAX];
}
IPMI_RESPONSE_NVRAM_READ, *PIPMI_RESPONSE_NVRAM_READ;

//++
// Name:
//      IPMI_REQUEST_NVRAM_WRITE
//
// Description:
//      Data portion of 'Write NVRAM' IPMI request.
//--
typedef struct _IPMI_REQUEST_NVRAM_WRITE
{
    //
    // Starting offset for the write, LSB first.
    //
    UCHAR            offset [IPMI_OFFSET_LENGTH_MAX];

    //
    // Data to write to nvram.
    //
    UCHAR            data [IPMI_NVRAM_DATA_LENGTH_MAX];
}
IPMI_REQUEST_NVRAM_WRITE, *PIPMI_REQUEST_NVRAM_WRITE;

//++
// Name:
//      IPMI_RESPONSE_GET_PEER_BMC_STATUS
//
// Description:
//      The response data for a 'Get Peer BMC Status' request (netfn=0x30, Cmd=0x65).
//--
typedef struct _IPMI_RESPONSE_GET_PEER_BMC_STATUS
{
    IPMI_PEER_BMC_STATUS        peerBmcStatus;
}
IPMI_RESPONSE_GET_PEER_BMC_STATUS, *PIPMI_RESPONSE_GET_PEER_BMC_STATUS;

//++
// Name:
//      IPMI_P_REQUEST_DEBUG_LOGS_TFTP_TRANSFER_STATUS
//
// Description:
//      Data portion of 'Debug Logs Tftp Transfer Status' IPMI request.
//--
typedef struct _IPMI_P_REQUEST_DEBUG_LOGS_TFTP_TRANSFER_STATUS
{
    //
    // Sub-command, should be set to IPMI_P_SC_DEBUG_LOGS_TFTP_TRANSFER_STATUS.
    //
    UCHAR               subCommand;
}
IPMI_P_REQUEST_DEBUG_LOGS_TFTP_TRANSFER_STATUS, *PIPMI_P_REQUEST_DEBUG_LOGS_TFTP_TRANSFER_STATUS;

//++
// Name:
//      IPMI_P_RESPONSE_DEBUG_LOGS_TFTP_TRANSFER_STATUS
//
// Description:
//      Data portion of 'Debug Logs Tftp Transfer Status' IPMI response.
//--
typedef struct _IPMI_P_RESPONSE_DEBUG_LOGS_TFTP_TRANSFER_STATUS
{
    //
    // Status of the TFTP transfer, of type IPMI_P_DEBUG_LOGS_TFTP_TRANSFER_STATUS.
    //
    UCHAR               status;
}
IPMI_P_RESPONSE_DEBUG_LOGS_TFTP_TRANSFER_STATUS, *PIPMI_P_RESPONSE_DEBUG_LOGS_TFTP_TRANSFER_STATUS;

//++
// Name:
//      IPMI_P_REQUEST_DEBUG_LOGS_TFTP_START_TRANSFER
//
// Description:
//      Data portion of 'Debug Logs Tftp Start Transfer' IPMI request.
//--
typedef struct _IPMI_P_REQUEST_DEBUG_LOGS_TFTP_START_TRANSFER
{
    //
    // Sub-command, should be set to IPMI_P_SC_DEBUG_LOGS_TFTP_START_TRANSFER.
    //
    UCHAR               subCommand;

    //
    // The IP address of the TFTP server, most significant octet first.
    //
    UCHAR               serverIP [4];

    //
    // File name.
    //
    UCHAR               fileName [128];

}
IPMI_P_REQUEST_DEBUG_LOGS_TFTP_START_TRANSFER, *PIPMI_P_REQUEST_DEBUG_LOGS_TFTP_START_TRANSFER;

//
// The maximum number of bytes that can be read for the 'Debug Logs IPMI Read' command.
#define IPMI_P_DEBUG_LOGS_IPMI_MAX_READ_BYTES       0xC2

//++
// Name:
//      IPMI_P_REQUEST_DEBUG_LOGS_IPMI_TRANSFER_STATUS
//
// Description:
//      Data portion of 'Debug Logs IPMI Transfer Status' IPMI request.
//--
typedef struct _IPMI_P_REQUEST_DEBUG_LOGS_IPMI_TRANSFER_STATUS
{
    //
    // Sub-command, should be set to IPMI_P_SC_DEBUG_LOGS_IPMI_TRANSFER_STATUS.
    //
    UCHAR               subCommand;
}
IPMI_P_REQUEST_DEBUG_LOGS_IPMI_TRANSFER_STATUS, *PIPMI_P_REQUEST_DEBUG_LOGS_IPMI_TRANSFER_STATUS;

//++
// Name:
//      IPMI_P_RESPONSE_DEBUG_LOGS_IPMI_TRANSFER_STATUS
//
// Description:
//      Data portion of 'Debug Logs IPMI Transfer Status' IPMI response.
//--
typedef struct _IPMI_P_RESPONSE_DEBUG_LOGS_IPMI_TRANSFER_STATUS
{
    //
    // Status of the TFTP transfer, of type IPMI_P_DEBUG_LOGS_IPMI_TRANSFER_STATUS.
    //
    UCHAR               status;
}
IPMI_P_RESPONSE_DEBUG_LOGS_IPMI_TRANSFER_STATUS, *PIPMI_P_RESPONSE_DEBUG_LOGS_IPMI_TRANSFER_STATUS;

//++
// Name:
//      IPMI_P_REQUEST_DEBUG_LOGS_IPMI_START_TRANSFER
//
// Description:
//      Data portion of 'Debug Logs IPMI Start Transfer' IPMI request.
//--
typedef struct _IPMI_P_REQUEST_DEBUG_LOGS_IPMI_START_TRANSFER
{
    //
    // Sub-command, should be set to IPMI_P_SC_DEBUG_LOGS_IPMI_START_TRANSFER.
    //
    UCHAR               subCommand;

}
IPMI_P_REQUEST_DEBUG_LOGS_IPMI_START_TRANSFER, *PIPMI_P_REQUEST_DEBUG_LOGS_IPMI_START_TRANSFER;

//++
// Name:
//      IPMI_P_REQUEST_DEBUG_LOGS_IPMI_FREE_RESOURCES
//
// Description:
//      Data portion of 'Debug Logs Tftp Abort' IPMI request.
//--
typedef struct _IPMI_P_REQUEST_DEBUG_LOGS_IPMI_FREE_RESOURCES
{
    //
    // Sub-command, should be set to IPMI_P_SC_DEBUG_LOGS_IPMI_FREE_RESOURCES.
    //
    UCHAR               subCommand;

}
IPMI_P_REQUEST_DEBUG_LOGS_IPMI_FREE_RESOURCES, *PIPMI_P_REQUEST_DEBUG_LOGS_IPMI_FREE_RESOURCES;

//++
// Name:
//      IPMI_P_REQUEST_DEBUG_LOGS_IPMI_READ
//
// Description:
//      Data portion of 'Debug Logs IPMI Read'.
//--
typedef struct _IPMI_P_REQUEST_DEBUG_LOGS_IPMI_READ
{
    //
    // Sub-command, should be set to IPMI_P_SC_DEBUG_LOGS_IPMI_READ.
    //
    UCHAR               subCommand;

    //
    // Number of bytes to read., must be <= IPMI_P_DEBUG_LOGS_IPMI_MAX_READ_BYTES.
    //
    UCHAR               bytesToRead;
}
IPMI_P_REQUEST_DEBUG_LOGS_IPMI_READ, *PIPMI_P_REQUEST_DEBUG_LOGS_IPMI_READ;

//++
// Name:
//      IPMI_P_RESPONSE_DEBUG_LOGS_IPMI_READ
//
// Description:
//      Data portion of 'Debug Logs IPMI Read'.
//--
typedef struct _IPMI_P_RESPONSE_DEBUG_LOGS_IPMI_READ
{
    //
    // Number of bytes remaining to be read (LSB first).
    //
    UCHAR               bytesRemaining [4];

    //
    // Number of bytes successfully read (by this command).
    //
    UCHAR               bytesRead;

    //
    // Data bytes (log file).
    //
    UCHAR               data [IPMI_P_DEBUG_LOGS_IPMI_MAX_READ_BYTES];

}
IPMI_P_RESPONSE_DEBUG_LOGS_IPMI_READ, *PIPMI_P_RESPONSE_DEBUG_LOGS_IPMI_READ;

//++
// Sensor Number values
//--
// For Megatron & Jetfire
typedef enum _IPMI_MT_JF_SNSRNMBR
{
    IPMI_MT_JF_SNSRNMBR_SLIC_0                                = 0x01,
    IPMI_MT_JF_SNSRNMBR_SLIC_1                                = 0x02,
    IPMI_MT_JF_SNSRNMBR_SLIC_2                                = 0x03,
    IPMI_MT_JF_SNSRNMBR_SLIC_3                                = 0x04,
    IPMI_MT_JF_SNSRNMBR_SLIC_4                                = 0x05,
    IPMI_MT_JF_SNSRNMBR_TIME_REMAINING                        = 0x82,
    IPMI_MT_JF_SNSRNMBR_SHTDN_IN_PRG                          = 0x83,
    IPMI_MT_JF_SNSRNMBR_ECC_COUNT                             = 0xEE,
    IPMI_MT_JF_SNSRNMBR_MFB                                   = 0xF8,
    IPMI_MT_JF_SNSRNMBR_POWER_RESET_ACTION                    = 0xFC,
    IPMI_MT_JF_SNSRNMBR_SEL_RATE                              = 0xF2,
    IPMI_MT_JF_SNSRNMBR_EVTLOG_DISABLED                       = 0xF3
} IPMI_MT_JF_SNSRNMBR;

// For Megatron only
typedef enum _IPMI_MT_SNSRNMBR
{
    IPMI_MT_SNSRNMBR_SLIC_5                                = 0x06,
    IPMI_MT_SNSRNMBR_SLIC_6                                = 0x07,
    IPMI_MT_SNSRNMBR_SLIC_7                                = 0x08,
    IPMI_MT_SNSRNMBR_SLIC_8                                = 0x09,
    IPMI_MT_SNSRNMBR_SLIC_9                                = 0x0A,
    IPMI_MT_SNSRNMBR_SLIC_10                               = 0x0B,
    IPMI_MT_SNSRNMBR_MM_GPIO                               = 0x24,
    IPMI_MT_SNSRNMBR_FAN0_SPEED                            = 0x26,
    IPMI_MT_SNSRNMBR_FAN1_SPEED                            = 0x27,
    IPMI_MT_SNSRNMBR_FAN2_SPEED                            = 0x28,
    IPMI_MT_SNSRNMBR_FAN3_SPEED                            = 0x29,
    IPMI_MT_SNSRNMBR_FAN4_SPEED                            = 0x2A,
    IPMI_MT_SNSRNMBR_SP_STATUS                             = 0x2B,
    IPMI_MT_SNSRNMBR_AMBIENT_TEMP                          = 0x2D,
    IPMI_MT_SNSRNMBR_COINCELL_BATTERY_VOLT                 = 0x51,
    IPMI_MT_SNSRNMBR_PS0_STATUS                            = 0x52,
    IPMI_MT_SNSRNMBR_PS0_INPUT_POWER                       = 0x53,
    IPMI_MT_SNSRNMBR_PS0_OUTVOLT                           = 0x56,
    IPMI_MT_SNSRNMBR_PS0_OUTCURRENT                        = 0x57,
    IPMI_MT_SNSRNMBR_PS0_TEMP2                             = 0x5B, //Temp Sensor 1 was removed from newer power supplies
    IPMI_MT_SNSRNMBR_PS0_TEMP3                             = 0x5C,
    IPMI_MT_SNSRNMBR_PS1_STATUS                            = 0x5E,
    IPMI_MT_SNSRNMBR_PS1_INPUT_POWER                       = 0x5F,
    IPMI_MT_SNSRNMBR_PS1_OUTVOLT                           = 0x62,
    IPMI_MT_SNSRNMBR_PS1_OUTCURRENT                        = 0x63,
    IPMI_MT_SNSRNMBR_PS1_TEMP2                             = 0x67, //Temp Sensor 1 was removed from newer power supplies
    IPMI_MT_SNSRNMBR_PS1_TEMP3                             = 0x68,
    IPMI_MT_SNSRNMBR_FAN_REDUNDANCY                        = 0x6B
} IPMI_MT_SNSRNMBR;

// For Jetfire only.
typedef enum _IPMI_JF_SNSRNMBR
{
    IPMI_JF_SNSRNMBR_MM_GPIO                            = 0x12,
    IPMI_JF_SNSRNMBR_SP_STATUS                          = 0x13,
    IPMI_JF_SNSRNMBR_AMBIENT_TEMP                       = 0x14,
    IPMI_JF_SNSRNMBR_COINCELL_BATTERY_VOLT              = 0x2B,
    IPMI_JF_SNSRNMBR_PSA_STATUS                         = 0x2C,
    IPMI_JF_SNSRNMBR_PSA_INPUT_POWER                    = 0x2D,
    IPMI_JF_SNSRNMBR_PSA_OUTVOLT                        = 0x30,
    IPMI_JF_SNSRNMBR_PSA_OUTCURRENT                     = 0x31,
    IPMI_JF_SNSRNMBR_PSA_TEMP2                          = 0x35, //Temp Sensor 1 was removed from newer power supplies
    IPMI_JF_SNSRNMBR_PSA_TEMP3                          = 0x36,
    IPMI_JF_SNSRNMBR_PSB_STATUS                         = 0x38,
    IPMI_JF_SNSRNMBR_PSB_INPUT_POWER                    = 0x39,
    IPMI_JF_SNSRNMBR_PSB_OUTVOLT                        = 0x3C,
    IPMI_JF_SNSRNMBR_PSB_OUTCURRENT                     = 0x3D,
    IPMI_JF_SNSRNMBR_PSB_TEMP2                          = 0x41, //Temp Sensor 1 was removed from newer power supplies
    IPMI_JF_SNSRNMBR_PSB_TEMP3                          = 0x42,
    IPMI_JF_SNSRNMBR_FAN0_SPEED                         = 0x45,
    IPMI_JF_SNSRNMBR_FAN1_SPEED                         = 0x46,
    IPMI_JF_SNSRNMBR_FAN2_SPEED                         = 0x47,
    IPMI_JF_SNSRNMBR_FAN3_SPEED                         = 0x48,
    IPMI_JF_SNSRNMBR_FAN_REDUNDANCY                     = 0x49,
    IPMI_JF_SNSRNMBR_BEM_STATUS                         = 0x4A,
    IPMI_JF_SNSRNMBR_BAT_STATUS_I2C                     = 0x4E, // Copilot I2C
    IPMI_JF_SNSRNMBR_BAT_STATUS_GPIO                    = 0x4F, // Copilot GPIO
    IPMI_JF_SNSRNMBR_BEM_PWR_FAULT                      = 0x51
} IPMI_JF_SNSRNMBR;

// for Beachcomber only
typedef enum _IPMI_BC_SNSRNMBR
{
    IPMI_BC_SNSRNMBR_ESLIC_STATUS                       = 0x01,
    IPMI_BC_SNSRNMBR_SP_STATUS                          = 0x02,
    IPMI_BC_SNSRNMBR_AMBIENT_TEMP                       = 0x16,
    IPMI_BC_SNSRNMBR_EHORNET_STATUS                     = 0x1A,
    IPMI_BC_SNSRNMBR_BAT_STATUS_I2C                     = 0x24,
    IPMI_BC_SNSRNMBR_BAT_STATUS_GPIO                    = 0x25,
    IPMI_BC_SNSRNMBR_PSA_STATUS                         = 0x30,
    IPMI_BC_SNSRNMBR_PSA_INPUT_POWER                    = 0x31,
    IPMI_BC_SNSRNMBR_PSA_INVOLT                         = 0x32,
    IPMI_BC_SNSRNMBR_PSA_INCURRENT                      = 0x33,
    IPMI_BC_SNSRNMBR_PSA_OUTVOLT                        = 0x34,
    IPMI_BC_SNSRNMBR_PSA_OUTCURRENT                     = 0x35,
    IPMI_BC_SNSRNMBR_PSA_STA_VOLT                       = 0x36,
    IPMI_BC_SNSRNMBR_PSA_STA_CURRENT                    = 0x37,
    IPMI_BC_SNSRNMBR_PSA_TEMP2                          = 0x39, 
    IPMI_BC_SNSRNMBR_PSA_TEMP3                          = 0x3A,
    IPMI_BC_SNSRNMBR_PSA_FAN_SPEED                      = 0x3B,
    IPMI_BC_SNSRNMBR_PSB_STATUS                         = 0x40,
    IPMI_BC_SNSRNMBR_PSB_INPUT_POWER                    = 0x41,
    IPMI_BC_SNSRNMBR_PSB_INVOLT                         = 0x42,
    IPMI_BC_SNSRNMBR_PSB_INCURRENT                      = 0x43,
    IPMI_BC_SNSRNMBR_PSB_OUTVOLT                        = 0x44,
    IPMI_BC_SNSRNMBR_PSB_OUTCURRENT                     = 0x45,
    IPMI_BC_SNSRNMBR_PSB_STA_VOLT                       = 0x46,
    IPMI_BC_SNSRNMBR_PSB_STA_CURRENT                    = 0x47,
    IPMI_BC_SNSRNMBR_PSB_TEMP2                          = 0x49,
    IPMI_BC_SNSRNMBR_PSB_TEMP3                          = 0x4A,
    IPMI_BC_SNSRNMBR_PSB_FAN_SPEED                      = 0x4B,
    IPMI_BC_SNSRNMBR_COINCELL_BATTERY_VOLT              = 0x51,
    IPMI_BC_SNSRNMBR_CM_A0_STATUS                       = 0x60,
    IPMI_BC_SNSRNMBR_CM_A0_INVOLT                       = 0x61,
    IPMI_BC_SNSRNMBR_CM_A0_INCURRENT                    = 0x62,
    IPMI_BC_SNSRNMBR_CM_A0_FANSPEED_H                   = 0x63,
    IPMI_BC_SNSRNMBR_CM_A0_FANSPEED_L                   = 0x64,
    IPMI_BC_SNSRNMBR_CM_A1_STATUS                       = 0x65,
    IPMI_BC_SNSRNMBR_CM_A1_INVOLT                       = 0x66,
    IPMI_BC_SNSRNMBR_CM_A1_INCURRENT                    = 0x67,
    IPMI_BC_SNSRNMBR_CM_A1_FANSPEED_H                   = 0x68,
    IPMI_BC_SNSRNMBR_CM_A1_FANSPEED_L                   = 0x69,
    IPMI_BC_SNSRNMBR_CM_A2_STATUS                       = 0x6A,
    IPMI_BC_SNSRNMBR_CM_A2_INVOLT                       = 0x6B,
    IPMI_BC_SNSRNMBR_CM_A2_INCURRENT                    = 0x6C,
    IPMI_BC_SNSRNMBR_CM_A2_FANSPEED_H                   = 0x6D,
    IPMI_BC_SNSRNMBR_CM_A2_FANSPEED_L                   = 0x6E,
    IPMI_BC_SNSRNMBR_CM_B0_STATUS                       = 0x6F,
    IPMI_BC_SNSRNMBR_CM_B0_INVOLT                       = 0x70,
    IPMI_BC_SNSRNMBR_CM_B0_INCURRENT                    = 0x71,
    IPMI_BC_SNSRNMBR_CM_B0_FANSPEED_H                   = 0x72,
    IPMI_BC_SNSRNMBR_CM_B0_FANSPEED_L                   = 0x73,
    IPMI_BC_SNSRNMBR_CM_B1_STATUS                       = 0x74,
    IPMI_BC_SNSRNMBR_CM_B1_INVOLT                       = 0x75,
    IPMI_BC_SNSRNMBR_CM_B1_INCURRENT                    = 0x76,
    IPMI_BC_SNSRNMBR_CM_B1_FANSPEED_H                   = 0x77,
    IPMI_BC_SNSRNMBR_CM_B1_FANSPEED_L                   = 0x78,
    IPMI_BC_SNSRNMBR_CM_B2_STATUS                       = 0x79,
    IPMI_BC_SNSRNMBR_CM_B2_INVOLT                       = 0x7A,
    IPMI_BC_SNSRNMBR_CM_B2_INCURRENT                    = 0x7B,
    IPMI_BC_SNSRNMBR_CM_B2_FANSPEED_H                   = 0x7C,
    IPMI_BC_SNSRNMBR_CM_B2_FANSPEED_L                   = 0x7D,
    IPMI_BC_SNSRNMBR_FAN_REDUNDANCY                     = 0x93
} IPMI_BC_SNSRNMBR;

// Common Phobos sensor numbers
typedef enum _IPMI_PHOBOS_SNSRNMBR
{
    // SLICs
    IPMI_PHOBOS_SNSRNMBR_SLIC_0_STATUS                  = 0x00,
    IPMI_PHOBOS_SNSRNMBR_SLIC_1_STATUS                  = 0x01,
    IPMI_PHOBOS_SNSRNMBR_SLIC_2_STATUS                  = 0x02,
    IPMI_PHOBOS_SNSRNMBR_SLIC_3_STATUS                  = 0x03,
    IPMI_PHOBOS_SNSRNMBR_SLIC_4_STATUS                  = 0x04,
    IPMI_PHOBOS_SNSRNMBR_SLIC_5_STATUS                  = 0x05,
    IPMI_PHOBOS_SNSRNMBR_SLIC_6_STATUS                  = 0x06,
    IPMI_PHOBOS_SNSRNMBR_SLIC_7_STATUS                  = 0x07,
    IPMI_PHOBOS_SNSRNMBR_SLIC_8_STATUS                  = 0x08,
    IPMI_PHOBOS_SNSRNMBR_SLIC_9_STATUS                  = 0x09,
    IPMI_PHOBOS_SNSRNMBR_SLIC_10_STATUS                 = 0x0A,
    IPMI_PHOBOS_SNSRNMBR_SLIC_11_STATUS                 = 0x0B,
    IPMI_PHOBOS_SNSRNMBR_SLIC_12_STATUS                 = 0x0C,
    IPMI_PHOBOS_SNSRNMBR_SLIC_13_STATUS                 = 0x0D,
    IPMI_PHOBOS_SNSRNMBR_SLIC_14_STATUS                 = 0x0E,
    IPMI_PHOBOS_SNSRNMBR_SLIC_15_STATUS                 = 0x0F,
    IPMI_PHOBOS_SNSRNMBR_SLIC_0_CMD                     = 0x10,
    IPMI_PHOBOS_SNSRNMBR_SLIC_1_CMD                     = 0x11,
    IPMI_PHOBOS_SNSRNMBR_SLIC_2_CMD                     = 0x12,
    IPMI_PHOBOS_SNSRNMBR_SLIC_3_CMD                     = 0x13,
    IPMI_PHOBOS_SNSRNMBR_SLIC_4_CMD                     = 0x14,
    IPMI_PHOBOS_SNSRNMBR_SLIC_5_CMD                     = 0x15,
    IPMI_PHOBOS_SNSRNMBR_SLIC_6_CMD                     = 0x16,
    IPMI_PHOBOS_SNSRNMBR_SLIC_7_CMD                     = 0x17,
    IPMI_PHOBOS_SNSRNMBR_SLIC_8_CMD                     = 0x18,
    IPMI_PHOBOS_SNSRNMBR_SLIC_9_CMD                     = 0x19,
    IPMI_PHOBOS_SNSRNMBR_SLIC_10_CMD                    = 0x1A,
    IPMI_PHOBOS_SNSRNMBR_SLIC_11_CMD                    = 0x1B,
    IPMI_PHOBOS_SNSRNMBR_SLIC_12_CMD                    = 0x1C,
    IPMI_PHOBOS_SNSRNMBR_SLIC_13_CMD                    = 0x1D,
    IPMI_PHOBOS_SNSRNMBR_SLIC_14_CMD                    = 0x1E,
    IPMI_PHOBOS_SNSRNMBR_SLIC_15_CMD                    = 0x1F,
    IPMI_PHOBOS_SNSRNMBR_SLIC_0_TEMP                    = 0x20,
    IPMI_PHOBOS_SNSRNMBR_SLIC_1_TEMP                    = 0x21,
    IPMI_PHOBOS_SNSRNMBR_SLIC_2_TEMP                    = 0x22,
    IPMI_PHOBOS_SNSRNMBR_SLIC_3_TEMP                    = 0x23,
    IPMI_PHOBOS_SNSRNMBR_SLIC_4_TEMP                    = 0x24,
    IPMI_PHOBOS_SNSRNMBR_SLIC_5_TEMP                    = 0x25,
    IPMI_PHOBOS_SNSRNMBR_SLIC_6_TEMP                    = 0x26,
    IPMI_PHOBOS_SNSRNMBR_SLIC_7_TEMP                    = 0x27,
    IPMI_PHOBOS_SNSRNMBR_SLIC_8_TEMP                    = 0x28,
    IPMI_PHOBOS_SNSRNMBR_SLIC_9_TEMP                    = 0x29,
    IPMI_PHOBOS_SNSRNMBR_SLIC_10_TEMP                   = 0x2A,
    IPMI_PHOBOS_SNSRNMBR_SLIC_11_TEMP                   = 0x2B,
    IPMI_PHOBOS_SNSRNMBR_SLIC_12_TEMP                   = 0x2C,
    IPMI_PHOBOS_SNSRNMBR_SLIC_13_TEMP                   = 0x2D,
    IPMI_PHOBOS_SNSRNMBR_SLIC_14_TEMP                   = 0x2E,
    IPMI_PHOBOS_SNSRNMBR_SLIC_15_TEMP                   = 0x2F,

    // DIMM Thermal
    IPMI_PHOBOS_SNSRNMBR_DIMM_BANK_0_TEMP               = 0x30,
    IPMI_PHOBOS_SNSRNMBR_DIMM_BANK_1_TEMP               = 0x31,
    IPMI_PHOBOS_SNSRNMBR_DIMM_BANK_2_TEMP               = 0x32,
    IPMI_PHOBOS_SNSRNMBR_DIMM_BANK_3_TEMP               = 0x33,
    IPMI_PHOBOS_SNSRNMBR_DIMM_BANK_4_TEMP               = 0x34,
    IPMI_PHOBOS_SNSRNMBR_DIMM_BANK_5_TEMP               = 0x35,
    IPMI_PHOBOS_SNSRNMBR_DIMM_BANK_6_TEMP               = 0x36,
    IPMI_PHOBOS_SNSRNMBR_DIMM_BANK_7_TEMP               = 0x37,
    IPMI_PHOBOS_SNSRNMBR_DIMM_BANK_8_TEMP               = 0x38,
    IPMI_PHOBOS_SNSRNMBR_DIMM_BANK_9_TEMP               = 0x39,
    IPMI_PHOBOS_SNSRNMBR_DIMM_BANK_10_TEMP              = 0x3A,
    IPMI_PHOBOS_SNSRNMBR_DIMM_BANK_11_TEMP              = 0x3B,
    IPMI_PHOBOS_SNSRNMBR_DIMM_BANK_12_TEMP              = 0x3C,
    IPMI_PHOBOS_SNSRNMBR_DIMM_BANK_13_TEMP              = 0x3D,
    IPMI_PHOBOS_SNSRNMBR_DIMM_BANK_14_TEMP              = 0x3E,
    IPMI_PHOBOS_SNSRNMBR_DIMM_BANK_15_TEMP              = 0x3F,

    // CPU
    IPMI_PHOBOS_SNSRNMBR_CPU_0_STATUS                   = 0x40,
    IPMI_PHOBOS_SNSRNMBR_CPU_1_STATUS                   = 0x41,
    IPMI_PHOBOS_SNSRNMBR_CPU_2_STATUS                   = 0x42,
    IPMI_PHOBOS_SNSRNMBR_CPU_3_STATUS                   = 0x43,
    IPMI_PHOBOS_SNSRNMBR_CPU_4_STATUS                   = 0x44,
    IPMI_PHOBOS_SNSRNMBR_CPU_5_STATUS                   = 0x45,
    IPMI_PHOBOS_SNSRNMBR_CPU_6_STATUS                   = 0x46,
    IPMI_PHOBOS_SNSRNMBR_CPU_7_STATUS                   = 0x47,
    IPMI_PHOBOS_SNSRNMBR_CPU_0_TEMP                     = 0x48,
    IPMI_PHOBOS_SNSRNMBR_CPU_1_TEMP                     = 0x49,
    IPMI_PHOBOS_SNSRNMBR_CPU_2_TEMP                     = 0x4A,
    IPMI_PHOBOS_SNSRNMBR_CPU_3_TEMP                     = 0x4B,
    IPMI_PHOBOS_SNSRNMBR_CPU_4_TEMP                     = 0x4C,
    IPMI_PHOBOS_SNSRNMBR_CPU_5_TEMP                     = 0x4D,
    IPMI_PHOBOS_SNSRNMBR_CPU_6_TEMP                     = 0x4E,
    IPMI_PHOBOS_SNSRNMBR_CPU_7_TEMP                     = 0x4F,

    // Local Thermal
    IPMI_PHOBOS_SNSRNMBR_SP_TEMP_0                      = 0x50,
    IPMI_PHOBOS_SNSRNMBR_SP_TEMP_1                      = 0x51,
    IPMI_PHOBOS_SNSRNMBR_SP_TEMP_2                      = 0x52,
    IPMI_PHOBOS_SNSRNMBR_SP_TEMP_3                      = 0x53,
    IPMI_PHOBOS_SNSRNMBR_MP_TEMP_0                      = 0x54,
    IPMI_PHOBOS_SNSRNMBR_MP_TEMP_1                      = 0x55,
    IPMI_PHOBOS_SNSRNMBR_MP_TEMP_2                      = 0x56,
    IPMI_PHOBOS_SNSRNMBR_MP_TEMP_3                      = 0x57,
    IPMI_PHOBOS_SNSRNMBR_AMBIENT_TEMP                   = 0x5F,

    // Fans
    IPMI_PHOBOS_SNSRNMBR_FAN_0_SPEED                    = 0x60,
    IPMI_PHOBOS_SNSRNMBR_FAN_1_SPEED                    = 0x61,
    IPMI_PHOBOS_SNSRNMBR_FAN_2_SPEED                    = 0x62,
    IPMI_PHOBOS_SNSRNMBR_FAN_3_SPEED                    = 0x63,
    IPMI_PHOBOS_SNSRNMBR_FAN_4_SPEED                    = 0x64,
    IPMI_PHOBOS_SNSRNMBR_FAN_5_SPEED                    = 0x65,
    IPMI_PHOBOS_SNSRNMBR_FAN_6_SPEED                    = 0x66,
    IPMI_PHOBOS_SNSRNMBR_FAN_7_SPEED                    = 0x67,
    IPMI_PHOBOS_SNSRNMBR_FAN_8_SPEED                    = 0x68,
    IPMI_PHOBOS_SNSRNMBR_FAN_9_SPEED                    = 0x69,
    IPMI_PHOBOS_SNSRNMBR_FAN_10_SPEED                   = 0x6A,
    IPMI_PHOBOS_SNSRNMBR_FAN_11_SPEED                   = 0x6B,
    IPMI_PHOBOS_SNSRNMBR_FAN_12_SPEED                   = 0x6C,
    IPMI_PHOBOS_SNSRNMBR_FAN_13_SPEED                   = 0x6D,
    IPMI_PHOBOS_SNSRNMBR_FAN_0_STATUS                   = 0x70,
    IPMI_PHOBOS_SNSRNMBR_FAN_1_STATUS                   = 0x71,
    IPMI_PHOBOS_SNSRNMBR_FAN_2_STATUS                   = 0x72,
    IPMI_PHOBOS_SNSRNMBR_FAN_3_STATUS                   = 0x73,
    IPMI_PHOBOS_SNSRNMBR_FAN_4_STATUS                   = 0x74,
    IPMI_PHOBOS_SNSRNMBR_FAN_5_STATUS                   = 0x75,
    IPMI_PHOBOS_SNSRNMBR_FAN_6_STATUS                   = 0x76,
    IPMI_PHOBOS_SNSRNMBR_FAN_7_STATUS                   = 0x77,
    IPMI_PHOBOS_SNSRNMBR_FAN_8_STATUS                   = 0x78,
    IPMI_PHOBOS_SNSRNMBR_FAN_9_STATUS                   = 0x79,
    IPMI_PHOBOS_SNSRNMBR_FAN_10_STATUS                  = 0x7A,
    IPMI_PHOBOS_SNSRNMBR_FAN_11_STATUS                  = 0x7B,
    IPMI_PHOBOS_SNSRNMBR_FAN_12_STATUS                  = 0x7C,
    IPMI_PHOBOS_SNSRNMBR_FAN_13_STATUS                  = 0x7D,

    // Power Supply
    IPMI_PHOBOS_SNSRNMBR_PS_0_STATUS                    = 0x90,
    IPMI_PHOBOS_SNSRNMBR_PS_0_INPUT_POWER               = 0x91,
    IPMI_PHOBOS_SNSRNMBR_PS_0_INVOLT                    = 0x92,
    IPMI_PHOBOS_SNSRNMBR_PS_0_TEMP_0                    = 0x93,
    IPMI_PHOBOS_SNSRNMBR_PS_0_TEMP_1                    = 0x94,
    IPMI_PHOBOS_SNSRNMBR_PS_0_STATUS_GPIO               = 0x97,
    IPMI_PHOBOS_SNSRNMBR_PS_1_STATUS                    = 0x98,
    IPMI_PHOBOS_SNSRNMBR_PS_1_INPUT_POWER               = 0x99,
    IPMI_PHOBOS_SNSRNMBR_PS_1_INVOLT                    = 0x9A,
    IPMI_PHOBOS_SNSRNMBR_PS_1_TEMP_0                    = 0x9B,
    IPMI_PHOBOS_SNSRNMBR_PS_1_TEMP_1                    = 0x9C,
    IPMI_PHOBOS_SNSRNMBR_PS_1_STATUS_GPIO               = 0x9F,

    // Battery
    IPMI_PHOBOS_SNSRNMBR_BATTERY_0_TEMP                 = 0xB0,
    IPMI_PHOBOS_SNSRNMBR_BATTERY_1_TEMP                 = 0xB1,
    IPMI_PHOBOS_SNSRNMBR_BATTERY_2_TEMP                 = 0xB2,
    IPMI_PHOBOS_SNSRNMBR_BATTERY_3_TEMP                 = 0xB3,
    IPMI_PHOBOS_SNSRNMBR_BATTERY_0_STATUS_I2C           = 0xC0,
    IPMI_PHOBOS_SNSRNMBR_BATTERY_1_STATUS_I2C           = 0xC1,
    IPMI_PHOBOS_SNSRNMBR_BATTERY_2_STATUS_I2C           = 0xC2,
    IPMI_PHOBOS_SNSRNMBR_BATTERY_3_STATUS_I2C           = 0xC3,
    IPMI_PHOBOS_SNSRNMBR_BATTERY_0_STATUS_GPIO          = 0xC8,
    IPMI_PHOBOS_SNSRNMBR_BATTERY_1_STATUS_GPIO          = 0xC9,
    IPMI_PHOBOS_SNSRNMBR_BATTERY_2_STATUS_GPIO          = 0xCA,
    IPMI_PHOBOS_SNSRNMBR_BATTERY_3_STATUS_GPIO          = 0xCB,

    // Enclosure Status
    IPMI_PHOBOS_SNSRNMBR_LOCAL_SP_STATUS                = 0xD0,
    IPMI_PHOBOS_SNSRNMBR_PEER_SP_STATUS                 = 0xD1,
    IPMI_PHOBOS_SNSRNMBR_LOCAL_SP_CMD_STATUS            = 0xD2,
    IPMI_PHOBOS_SNSRNMBR_LOCAL_SP_CMD_VIN               = 0xD3,
    IPMI_PHOBOS_SNSRNMBR_LOCAL_MM_STATUS                = 0xD4,
    IPMI_PHOBOS_SNSRNMBR_LOCAL_MM_CMD                   = 0xD5,
    IPMI_PHOBOS_SNSRNMBR_PEER_MM_STATUS                 = 0xD6,
    IPMI_PHOBOS_SNSRNMBR_PEER_MM_CMD                    = 0xD7,
    IPMI_PHOBOS_SNSRNMBR_COINCELL_BATTERY_VOLT          = 0xD8,
    IPMI_PHOBOS_SNSRNMBR_FAN_REDUNDANCY                 = 0xD9,
    IPMI_PHOBOS_SNSRNMBR_PS_REDUNDANCY                  = 0xDA,
    IPMI_PHOBOS_SNSRNMBR_PWR_CONSUMPTION                = 0xDB,
    IPMI_PHOBOS_SNSRNMBR_DRIVE_IO_STATUS                = 0xDD,
    IPMI_PHOBOS_SNSRNMBR_DRIVE_IO_CMD                   = 0xDE,
    IPMI_PHOBOS_SNSRNMBR_DRIVE_IO_TEMP                  = 0xDF,

    // Virtual Events
    IPMI_PHOBOS_SNSRNMBR_POWER_RESET_ACTION             = 0xE0,
    IPMI_PHOBOS_SNSRNMBR_BMC_INTERNAL_WDOG              = 0xE1,
    IPMI_PHOBOS_SNSRNMBR_FWU_EVENT                      = 0xE2,
    IPMI_PHOBOS_SNSRNMBR_BIST_EVENT                     = 0xE3,
    IPMI_PHOBOS_SNSRNMBR_CMD_LOG                        = 0xE4,
    IPMI_PHOBOS_SNSRNMBR_MFB_EVENT                      = 0xE5,
    IPMI_PHOBOS_SNSRNMBR_I2C_FAULT                      = 0xE6,
    IPMI_PHOBOS_SNSRNMBR_ADAPT_COOLING_EVENT            = 0xE8,
    IPMI_PHOBOS_SNSRNMBR_BMC_BOOT_PROG                  = 0xE9,
    IPMI_PHOBOS_SNSRNMBR_FW_EXCEPTION                   = 0xEA,

    // BMC Status
    IPMI_PHOBOS_SNSRNMBR_SYSTEM_POWER                   = 0xF0,
    IPMI_PHOBOS_SNSRNMBR_WATCHDOG                       = 0xF1,
    IPMI_PHOBOS_SNSRNMBR_SEL_RATE                       = 0xF2,
    IPMI_PHOBOS_SNSRNMBR_EVTLOG_DISABLED                = 0xF3,
    IPMI_PHOBOS_SNSRNMBR_SHTDN_IN_PRG                   = 0xF4,
    IPMI_PHOBOS_SNSRNMBR_TIME_REMAINING                 = 0xF5,
    IPMI_PHOBOS_SNSRNMBR_ECC_COUNT                      = 0xF6,
} IPMI_PHOBOS_SNSRNMBR;

//++
// Name:
//      IPMI_REQUEST_GET_SENSOR_READING
//
// Description:
//      Data buffer for 'Get Sensor Reading' IPMI request.
//--
typedef struct _IPMI_REQUEST_GET_SENSOR_READING
{
    //
    // The sensor number to read.
    //
    UCHAR            sensorNumber;
}
IPMI_REQUEST_GET_SENSOR_READING, *PIPMI_REQUEST_GET_SENSOR_READING;

typedef struct _SLIC_SENSOR_READING_REG_B0_B7
{
    UINT_8E     inserted:1,
                power_good:1,
                power_up_failure:1,
                power_enable:1,
                reset:1,
                led_on:1,
                led_flash:1,
                auto_reboot_enable:1;
} SLIC_SENSOR_READING_REG_B0_B7;

typedef union _SLIC_SENSOR_READING_RAW_INFO_B0_B7
{
    UINT_8E                             StatusControl;
    SLIC_SENSOR_READING_REG_B0_B7       fields;
} SLIC_SENSOR_READING_RAW_INFO_B0_B7;

typedef struct _SLIC_SENSOR_READING_REG_B8_B15
{
    UINT_8E     blade_reset_on_fault_or_removal:1, //Only for Moons, shows up as "reserved" bit on Transformers.
                low_power_mode_enable:1, //Only for Moons, shows up as "reserved" bit on Transformers.
                reserved_0:1,
                mhz_33_enable:1,
                width_0:1,
                width_1:1,
                width_2:1,
                reserved_1:1;
} SLIC_SENSOR_READING_REG_B8_B15;

typedef union _SLIC_SENSOR_READING_RAW_INFO_B8_B15
{
    UINT_8E                             StatusControl;
    SLIC_SENSOR_READING_REG_B8_B15      fields;
} SLIC_SENSOR_READING_RAW_INFO_B8_B15;

//++
// Name:
//      IPMI_RESPONSE_GET_SENSOR_READING_SLICS
//
// Description:
//      Data portion of IPMI response for 'Get Sensor Reading for SLICs'.
//--
typedef struct _IPMI_RESPONSE_GET_SENSOR_READING_SLICS
{
    UCHAR                                           sensorData;
    SENSOR_READING_SENSORSTATUS_BITS_RAW_INFO       sensorStatus;
    SLIC_SENSOR_READING_RAW_INFO_B0_B7              sensorState;
    SLIC_SENSOR_READING_RAW_INFO_B8_B15             additionalSensorState;
}
IPMI_RESPONSE_GET_SENSOR_READING_SLICS, *PIPMI_RESPONSE_GET_SENSOR_READING_SLICS;

typedef struct _BEM_PWR_FLT_SENSOR_READING_REG_B0_B7
{
    UINT_8E     peer_ecb_tripped_fault:1,
                local_ecb_tripped_fault:1,
                reserved_0:6;
} BEM_PWR_FLT_SENSOR_READING_REG_B0_B7;

typedef union _BEM_PWR_FLT_SENSOR_READING_RAW_INFO_B0_B7
{
    UINT_8E                                         StatusControl;
    BEM_PWR_FLT_SENSOR_READING_REG_B0_B7            fields;
} BEM_PWR_FLT_SENSOR_READING_RAW_INFO_B0_B7;

//++
// Name:
//      IPMI_RESPONSE_GET_SENSOR_READING_BEM_PWR_FAULT
//
// Description:
//      Data portion of IPMI response for 'Get Sensor Reading for BEM Power Fault'.
//--
typedef struct _IPMI_RESPONSE_GET_SENSOR_READING_BEM_PWR_FAULT
{
    UCHAR                                           sensorData;
    SENSOR_READING_SENSORSTATUS_BITS_RAW_INFO       sensorStatus;
    BEM_PWR_FLT_SENSOR_READING_RAW_INFO_B0_B7       sensorState;
    UCHAR                                           additionalSensorState;
}
IPMI_RESPONSE_GET_SENSOR_READING_BEM_PWR_FAULT, *PIPMI_RESPONSE_GET_SENSOR_READING_BEM_PWR_FAULT;

//++
// Name:
//      IPMI_RESPONSE_GET_SENSOR_READING_EHORNET
//
// Description:
//      Data portion of IPMI response for 'Get Sensor Reading for EHORNET'.
//--
typedef struct _IPMI_RESPONSE_GET_SENSOR_READING_EHORNET
{
    UCHAR                                           sensorData;
    SENSOR_READING_SENSORSTATUS_BITS_RAW_INFO       sensorStatus;
    SLIC_SENSOR_READING_RAW_INFO_B0_B7              sensorState;
    SLIC_SENSOR_READING_RAW_INFO_B8_B15             additionalSensorState;
} IPMI_RESPONSE_GET_SENSOR_READING_EHORNET, *PIPMI_RESPONSE_GET_SENSOR_READING_EHORNET;

typedef struct _AIRWAVE_STATUS_SENSOR_READING_REG_B0_B7
{
    UINT_8E     inserted:1,
                communication_error:1,
                fan_overridden:1,
                on_battery_power:1,
                disabled_via_host:1,
                external_fault:1,
                internal_fault:1,
                page_fault:1;
} AIRWAVE_STATUS_SENSOR_READING_REG_B0_B7;

typedef union _AIRWAVE_STATUS_SENSOR_READING_RAW_INFO_B0_B7
{
    UINT_8E                                         StatusControl;
    AIRWAVE_STATUS_SENSOR_READING_REG_B0_B7         fields;
} AIRWAVE_STATUS_SENSOR_READING_RAW_INFO_B0_B7;

typedef struct _AIRWAVE_STATUS_SENSOR_READING_REG_B8_B15
{
   UINT_8E     device_class_info_status_flag:1,
               reserved:7;
} AIRWAVE_STATUS_SENSOR_READING_REG_B8_B15;

typedef union _AIRWAVE_STATUS_SENSOR_READING_RAW_INFO_B8_B15
{
    UINT_8E                                         StatusControl;
    AIRWAVE_STATUS_SENSOR_READING_REG_B8_B15        fields;
} AIRWAVE_STATUS_SENSOR_READING_RAW_INFO_B8_B15;

//++
// Name:
//      IPMI_RESPONSE_GET_SENSOR_READING_AIRWAVE_STATUS
//
// Description:
//      Data portion of IPMI response for 'Get Sensor Reading for Airwave status'.
//--
typedef struct _IPMI_RESPONSE_GET_SENSOR_READING_AIRWAVE_STATUS
{
    UCHAR                                           sensorData;
    SENSOR_READING_SENSORSTATUS_BITS_RAW_INFO       sensorStatus;
    AIRWAVE_STATUS_SENSOR_READING_RAW_INFO_B0_B7    sensorState;
    AIRWAVE_STATUS_SENSOR_READING_RAW_INFO_B8_B15   additionalSensorState;
}
IPMI_RESPONSE_GET_SENSOR_READING_AIRWAVE_STATUS, *PIPMI_RESPONSE_GET_SENSOR_READING_AIRWAVE_STATUS;

typedef struct _PS_STATUS_SENSOR_READING_REG_B0_B7
{
    UINT_8E     inserted:1,
                ps_failure:1,
                predictive_failure:1, //Do a "ps_failure | predictive_failure" to get SPECL_PS_STATUS.generalFault
                ps_input_lost_ac_dc:1, //Basically we lost both AC & DC inputs i.e. no input at all, which gives us SPECL_PS_STATUS.ACFail
                ps_input_lost_or_oor:1, //Basically we lost AC input & now we have switched to DC i.e. SPS, whcih gives us SPECL_PS_STATUS.DCPresent
                ps_input_oor_but_present:1, //Basically we have some sort of AC input, but it is out-of-range
                config_error:1,
                reserved_0:1;
} PS_STATUS_SENSOR_READING_REG_B0_B7;

typedef union _PS_STATUS_SENSOR_READING_RAW_INFO_B0_B7
{
    UINT_8E                                         StatusControl;
    PS_STATUS_SENSOR_READING_REG_B0_B7              fields;
} PS_STATUS_SENSOR_READING_RAW_INFO_B0_B7;

//++
// Name:
//      IPMI_RESPONSE_GET_SENSOR_READING_PS_STATUS
//
// Description:
//      Data portion of IPMI response for 'Get Sensor Reading for power supply status'.
//--
typedef struct _IPMI_RESPONSE_GET_SENSOR_READING_PS_STATUS
{
    UCHAR                                           sensorData;
    SENSOR_READING_SENSORSTATUS_BITS_RAW_INFO       sensorStatus;
    PS_STATUS_SENSOR_READING_RAW_INFO_B0_B7         sensorState;
    UCHAR                                           additionalSensorState;
}
IPMI_RESPONSE_GET_SENSOR_READING_PS_STATUS, *PIPMI_RESPONSE_GET_SENSOR_READING_PS_STATUS;

//
// Phobos PS status sensor definitions
//
typedef struct _PS_STATUS_P_SENSOR_READING_REG_B0_B7
{
    UINT_8E     inserted:1,
                power_good:1,
                dc_present:1,
                input_lost:1,
                com_mem_logic_fault:1,
                fan_fault:1,
                over_temp:1,
                input_fault:1;
} PS_STATUS_P_SENSOR_READING_REG_B0_B7;

typedef union _PS_STATUS_P_SENSOR_READING_RAW_INFO_B0_B7
{
    UINT_8E                                 StatusControl;
    PS_STATUS_P_SENSOR_READING_REG_B0_B7    fields;
} PS_STATUS_P_SENSOR_READING_RAW_INFO_B0_B7;

typedef struct _PS_STATUS_P_SENSOR_READING_REG_B8_B15
{
    UINT_8E     output_fault:1,
                standby_output_fault:1,
                reserved_0:3,
                external_fault:1,
                internal_fault:1,
                reserved_1:1;
} PS_STATUS_P_SENSOR_READING_REG_B8_B15;

typedef union _PS_STATUS_P_SENSOR_READING_RAW_INFO_B8_B15
{
    UINT_8E                                 StatusControl;
    PS_STATUS_P_SENSOR_READING_REG_B8_B15   fields;
} PS_STATUS_P_SENSOR_READING_RAW_INFO_B8_B15;

//++
// Name:
//      IPMI_RESPONSE_P_GET_SENSOR_READING_PS_STATUS
//
// Description:
//      Data portion of IPMI response for 'Get Sensor Reading for power
//      supply status' for Phobos BMC.
//--
typedef struct _IPMI_RESPONSE_P_GET_SENSOR_READING_PS_STATUS
{
    UCHAR                                           sensorData;
    SENSOR_READING_SENSORSTATUS_BITS_RAW_INFO       sensorStatus;
    PS_STATUS_P_SENSOR_READING_RAW_INFO_B0_B7       sensorState;
    PS_STATUS_P_SENSOR_READING_RAW_INFO_B8_B15      additionalSensorState;
}
IPMI_RESPONSE_P_GET_SENSOR_READING_PS_STATUS, *PIPMI_RESPONSE_P_GET_SENSOR_READING_PS_STATUS;

typedef struct _THRESHOLD_BASED_SENSOR_READING_REG_B0_B7
{
    UINT_8E     lower_noncrit_tsh:1,
                lower_crit_tsh:1,
                lower_nonrecover_tsh:1,
                upper_noncrit_tsh:1,
                upper_crit_tsh:1,
                upper_nonrecover_tsh:1,
                reserved_0:2;
} THRESHOLD_BASED_SENSOR_READING_REG_B0_B7;

typedef union _THRESHOLD_BASED_SENSOR_READING_RAW_INFO_B0_B7
{
    UINT_8E                                             StatusControl;
    THRESHOLD_BASED_SENSOR_READING_REG_B0_B7            fields;
} THRESHOLD_BASED_SENSOR_READING_RAW_INFO_B0_B7;

//++
// Name:
//      IPMI_RESPONSE_GET_SENSOR_READING_AMBIENT_TEMP
//
// Description:
//      Data portion of IPMI response for 'Get Sensor Reading for threshold based sensors'.
//--
typedef struct _IPMI_RESPONSE_GET_THRESHOLD_BASED_SENSOR_READING
{
    UCHAR                                               sensorData;
    SENSOR_READING_SENSORSTATUS_BITS_RAW_INFO           sensorStatus;
    THRESHOLD_BASED_SENSOR_READING_RAW_INFO_B0_B7       sensorState;
}
IPMI_RESPONSE_GET_THRESHOLD_BASED_SENSOR_READING, *PIPMI_RESPONSE_GET_THRESHOLD_BASED_SENSOR_READING;

typedef struct _SP_STATUS_SENSOR_READING_REG_B0_B7
{
    UINT_8E     is_slot_B:1,
                slot_id_error:1,
                peer_inserted:1,
                pentium_powered_up:1,
                held_in_reset:1,
                boot_in_process:1,
                thermal_trip:1,
                reserved:1;
} SP_STATUS_SENSOR_READING_REG_B0_B7;

typedef union _SP_STATUS_SENSOR_READING_RAW_INFO_B0_B7
{
    UINT_8E                                         StatusControl;
    SP_STATUS_SENSOR_READING_REG_B0_B7              fields;
} SP_STATUS_SENSOR_READING_RAW_INFO_B0_B7;

//++
// Name:
//      IPMI_RESPONSE_GET_SENSOR_READING_SP_STATUS
//
// Description:
//      Data portion of IPMI response for 'Get Sensor Reading for SP status'.
//--
typedef struct _IPMI_RESPONSE_GET_SENSOR_READING_SP_STATUS
{
    UCHAR                                           sensorData;
    SENSOR_READING_SENSORSTATUS_BITS_RAW_INFO       sensorStatus;
    SP_STATUS_SENSOR_READING_RAW_INFO_B0_B7         sensorState;
    UCHAR                                           additionalSensorState;
}
IPMI_RESPONSE_GET_SENSOR_READING_SP_STATUS, *PIPMI_RESPONSE_GET_SENSOR_READING_SP_STATUS;

typedef struct _SP_STATUS_P_SENSOR_READING_REG_B0_B7
{
    UINT_8E     is_slot_B:1,
                slot_id_error:1,
                pentium_powered_up:1,
                held_in_reset:1,
                boot_in_process:1,
                thermal_trip:1,
                bios_alive_fault:1,
                reserved:1;
} SP_STATUS_P_SENSOR_READING_REG_B0_B7;

typedef union _SP_STATUS_P_SENSOR_READING_RAW_INFO_B0_B7
{
    UINT_8E                                         StatusControl;
    SP_STATUS_P_SENSOR_READING_REG_B0_B7            fields;
} SP_STATUS_P_SENSOR_READING_RAW_INFO_B0_B7;

//++
// Name:
//      IPMI_RESPONSE_GET_SENSOR_READING_SP_STATUS
//
// Description:
//      Data portion of IPMI response for 'Get Sensor Reading for SP status'.
//--
typedef struct _IPMI_RESPONSE_P_GET_SENSOR_READING_SP_STATUS
{
    UCHAR                                           sensorData;
    SENSOR_READING_SENSORSTATUS_BITS_RAW_INFO       sensorStatus;
    SP_STATUS_P_SENSOR_READING_RAW_INFO_B0_B7       sensorState;
    UCHAR                                           additionalSensorState;
}
IPMI_RESPONSE_P_GET_SENSOR_READING_SP_STATUS, *PIPMI_RESPONSE_P_GET_SENSOR_READING_SP_STATUS;

typedef struct _MM_GPIO_STATUS_SENSOR_READING_REG_B0_B7
{
    UINT_8E     local_mm_inserted:1,
                peer_mm_inserted:1,
                local_mm_powergood:1,
                reserved_0:5;
} MM_GPIO_STATUS_SENSOR_READING_REG_B0_B7;

typedef union _MM_GPIO_STATUS_SENSOR_READING_RAW_INFO_B0_B7
{
    UINT_8E                                             StatusControl;
    MM_GPIO_STATUS_SENSOR_READING_REG_B0_B7             fields;
} MM_GPIO_STATUS_SENSOR_READING_RAW_INFO_B0_B7;

//++
// Name:
//      IPMI_RESPONSE_GET_SENSOR_READING_MM_GPIO_STATUS
//
// Description:
//      Data portion of IPMI response for 'Get Sensor Reading for management module GPIO status'.
//--
typedef struct _IPMI_RESPONSE_GET_SENSOR_READING_MM_GPIO_STATUS
{
    UCHAR                                               sensorData;
    SENSOR_READING_SENSORSTATUS_BITS_RAW_INFO           sensorStatus;
    MM_GPIO_STATUS_SENSOR_READING_RAW_INFO_B0_B7        sensorState;
    UCHAR                                               additionalSensorState;
}
IPMI_RESPONSE_GET_SENSOR_READING_MM_GPIO_STATUS, *PIPMI_RESPONSE_GET_SENSOR_READING_MM_GPIO_STATUS;

typedef struct _SHTDN_IN_PRG_SENSOR_READING_REG_B0_B7
{
    UINT_8E     shutdown_in_progress:1,
                reboot_in_progress:1,
                timer_expired:1,
                reserved_0:4,
                disk_over_temp:1;
} SHTDN_IN_PRG_SENSOR_READING_REG_B0_B7;

typedef union _SHTDN_IN_PRG_SENSOR_READING_RAW_INFO_B0_B7
{
    UINT_8E                                         StatusControl;
    SHTDN_IN_PRG_SENSOR_READING_REG_B0_B7           fields;
} SHTDN_IN_PRG_SENSOR_READING_RAW_INFO_B0_B7;

typedef struct _SHTDN_IN_PRG_SENSOR_READING_REG_B8_B15
{
    UINT_8E     fan_fault:1,
                slic_over_temp:1,
                ps_over_temp:1,
                memory_over_temp:1,
                cpu_over_temp:1,
                ambient_over_temp:1,
                ssp_hang:1,
                reserved_0:1;
} SHTDN_IN_PRG_SENSOR_READING_REG_B8_B15;

typedef union _SHTDN_IN_PRG_SENSOR_READING_RAW_INFO_B8_B15
{
    UINT_8E                                         StatusControl;
    SHTDN_IN_PRG_SENSOR_READING_REG_B8_B15          fields;
} SHTDN_IN_PRG_SENSOR_READING_RAW_INFO_B8_B15;

//++
// Name:
//      IPMI_RESPONSE_GET_SENSOR_READING_SHTDN_IN_PRG
//
// Description:
//      Data portion of IPMI response for 'Get Sensor Reading for shutdown in progress'.
//--
typedef struct _IPMI_RESPONSE_GET_SENSOR_READING_SHTDN_IN_PRG
{
    UCHAR                                           sensorData;
    SENSOR_READING_SENSORSTATUS_BITS_RAW_INFO       sensorStatus;
    SHTDN_IN_PRG_SENSOR_READING_RAW_INFO_B0_B7      sensorState;
    SHTDN_IN_PRG_SENSOR_READING_RAW_INFO_B8_B15     additionalSensorState;
}
IPMI_RESPONSE_GET_SENSOR_READING_SHTDN_IN_PRG, *PIPMI_RESPONSE_GET_SENSOR_READING_SHTDN_IN_PRG;

typedef struct _P_SHTDN_IN_PRG_SENSOR_READING_REG_B0_B7
{
    UINT_8E     shutdown_in_progress:1,
                reboot_in_progress:1,
                timer_expired:1,
                reserved_0:3,
                embed_io_over_temp:1,
                fan_fault:1;
} P_SHTDN_IN_PRG_SENSOR_READING_REG_B0_B7;

typedef union _P_SHTDN_IN_PRG_SENSOR_READING_RAW_INFO_B0_B7
{
    UINT_8E                                         StatusControl;
    P_SHTDN_IN_PRG_SENSOR_READING_REG_B0_B7         fields;
} P_SHTDN_IN_PRG_SENSOR_READING_RAW_INFO_B0_B7;

typedef struct _P_SHTDN_IN_PRG_SENSOR_READING_REG_B8_B15
{
    UINT_8E     disk_over_temp:1,
                slic_over_temp:1,
                ps_over_temp:1,
                memory_over_temp:1,
                cpu_over_temp:1,
                ambient_over_temp:1,
                ssp_hang:1,
                reserved_0:1;
} P_SHTDN_IN_PRG_SENSOR_READING_REG_B8_B15;

typedef union _P_SHTDN_IN_PRG_SENSOR_READING_RAW_INFO_B8_B15
{
    UINT_8E                                         StatusControl;
    P_SHTDN_IN_PRG_SENSOR_READING_REG_B8_B15        fields;
} P_SHTDN_IN_PRG_SENSOR_READING_RAW_INFO_B8_B15;

//++
// Name:
//      IPMI_RESPONSE_P_GET_SENSOR_READING_SHTDN_IN_PRG
//
// Description:
//      Data portion of IPMI response for 'Get Sensor Reading for shutdown in progress'.
//      for Phobos BMC.
//--
typedef struct _IPMI_RESPONSE_P_GET_SENSOR_READING_SHTDN_IN_PRG
{
    UCHAR                                           sensorData;
    SENSOR_READING_SENSORSTATUS_BITS_RAW_INFO       sensorStatus;
    P_SHTDN_IN_PRG_SENSOR_READING_RAW_INFO_B0_B7    sensorState;
    P_SHTDN_IN_PRG_SENSOR_READING_RAW_INFO_B8_B15   additionalSensorState;
}
IPMI_RESPONSE_P_GET_SENSOR_READING_SHTDN_IN_PRG, *PIPMI_RESPONSE_P_GET_SENSOR_READING_SHTDN_IN_PRG;

typedef struct _P_FAN_STATUS_SENSOR_READING_REG_B0_B7
{
    UINT_8E     inserted:1,
                max_speed:1,
                under_speed:1,
                over_speed:1,
                no_tach:1,
                power_fault:1,
                under_test:1,
                overridden:1;
} P_FAN_STATUS_SENSOR_READING_REG_B0_B7;

typedef union _P_FAN_STATUS_SENSOR_READING_RAW_INFO_B0_B7
{
    UINT_8E                                StatusControl;
    P_FAN_STATUS_SENSOR_READING_REG_B0_B7  fields;
} P_FAN_STATUS_SENSOR_READING_RAW_INFO_B0_B7;

typedef struct _P_FAN_STATUS_SENSOR_READING_REG_B8_B15
{
    UINT_8E     reserved_0:5,
                external_fault:1,
                internal_fault:1,
                reserved_1:1;
} P_FAN_STATUS_SENSOR_READING_REG_B8_B15;

typedef union _P_FAN_STATUS_SENSOR_READING_RAW_INFO_B8_B15
{
    UINT_8E                                 StatusControl;
    P_FAN_STATUS_SENSOR_READING_REG_B8_B15  fields;
} P_FAN_STATUS_SENSOR_READING_RAW_INFO_B8_B15;

//++
// Name:
//      IPMI_RESPONSE_P_GET_SENSOR_READING_FAN_STATUS
//
// Description:
//      Data portion of IPMI response for 'Get Sensor Reading for Fan Status'.
//      for Phobos BMC.
//--
typedef struct _IPMI_RESPONSE_P_GET_SENSOR_READING_FAN_STATUS
{
    UCHAR                                           sensorData;
    SENSOR_READING_SENSORSTATUS_BITS_RAW_INFO       sensorStatus;
    P_FAN_STATUS_SENSOR_READING_RAW_INFO_B0_B7      sensorState;
    P_FAN_STATUS_SENSOR_READING_RAW_INFO_B8_B15     additionalSensorState;
}
IPMI_RESPONSE_P_GET_SENSOR_READING_FAN_STATUS, *PIPMI_RESPONSE_P_GET_SENSOR_READING_FAN_STATUS;

typedef struct _FAN_REDUNDANCY_SENSOR_READING_REG_B0_B7
{
    UINT_8E     fully_redundant:1,
                redundancy_lost:1,//Includes both, sufficient-resources & insufficient-resources.
                reserved_0:3,//These are definied in the fspec, but BMC never sets them, hence ignore here as well.
                non_redundant:1,//Insufficient-resources to maintain normal operation.
                reserved_1:2;//These are definied in the fspec, but BMC never sets them, hence ignore here as well.
} FAN_REDUNDANCY_SENSOR_READING_REG_B0_B7;

typedef union _FAN_REDUNDANCY_SENSOR_READING_RAW_INFO_B0_B7
{
    UINT_8E                                 StatusControl;
    FAN_REDUNDANCY_SENSOR_READING_REG_B0_B7 fields;
} FAN_REDUNDANCY_SENSOR_READING_RAW_INFO_B0_B7;

//++
// Name:
//      IPMI_RESPONSE_GET_SENSOR_READING_FAN_REDUNDANCY
//
// Description:
//      Data portion of IPMI response for 'Get Sensor Reading for Fan Redundancy'.
//--
typedef struct _IPMI_RESPONSE_GET_SENSOR_READING_FAN_REDUNDANCY
{
    UCHAR                                           sensorData;
    SENSOR_READING_SENSORSTATUS_BITS_RAW_INFO       sensorStatus;
    FAN_REDUNDANCY_SENSOR_READING_RAW_INFO_B0_B7    sensorState;
    UCHAR                                           additionalSensorState;
}
IPMI_RESPONSE_GET_SENSOR_READING_FAN_REDUNDANCY, *PIPMI_RESPONSE_GET_SENSOR_READING_FAN_REDUNDANCY;

typedef struct _EVTLOG_DISABLED_SENSOR_READING_REG_B0_B7
{
    UINT_8E     correctable_memory_error_logging_disabled:1,
                event_type_logging_disabled:1,
                log_area_reset_or_cleared:1,
                all_event_logging_disabled:1,
                log_full:1,
                log_almost_full:1,
                correctable_machine_check_error_logging_disabled:1;
} EVTLOG_DISABLED_SENSOR_READING_REG_B0_B7;

typedef union _EVTLOG_DISABLED_SENSOR_READING_RAW_INFO_B0_B7
{
    UINT_8E                                         StatusControl;
    EVTLOG_DISABLED_SENSOR_READING_REG_B0_B7        fields;
} EVTLOG_DISABLED_SENSOR_READING_RAW_INFO_B0_B7;

//++
// Name:
//      IPMI_RESPONSE_GET_SENSOR_READING_EVTLOG_DISABLED
//
// Description:
//      Data portion of IPMI response for 'Get Sensor Reading for EvtLogDisabled'.
//--
typedef struct _IPMI_RESPONSE_GET_SENSOR_READING_EVTLOG_DISABLED
{
    UCHAR                                           sensorData;
    SENSOR_READING_SENSORSTATUS_BITS_RAW_INFO       sensorStatus;
    EVTLOG_DISABLED_SENSOR_READING_RAW_INFO_B0_B7   sensorState;
    UCHAR                                           additionalSensorState;
}
IPMI_RESPONSE_GET_SENSOR_READING_EVTLOG_DISABLED, *PIPMI_RESPONSE_GET_SENSOR_READING_EVTLOG_DISABLED;

#define IPMI_DEFAULT_DELAYED_SHTDN_TIMER_VALUE      300
#define IPMI_SHUTDOWN_TIMER_CONVERSION_FACTOR       60      // Transformers use minutes for the shutdown timer

//++
// Name:
//      IPMI_DELAYED_SHTDN_TIMER_CONFIG
//
// Description:
//      Data portion of IPMI response for 'Delayed Shutdown Timer Configuration'.
//--
typedef struct _IPMI_DELAYED_SHTDN_TIMER_CONFIG
{
    UCHAR           delayedShtdnTimerValue; // In unit of minutes
}
IPMI_DELAYED_SHTDN_TIMER_CONFIG, *PIPMI_DELAYED_SHTDN_TIMER_CONFIG;

//++
// Name:
//      IPMI_P_DELAYED_SHTDN_TIMER_CONFIG
//
// Description:
//      Data portion of IPMI response for 'Delayed Shutdown Timer Configuration'.
//      for Phobos BMC.
//--
typedef struct _IPMI_P_DELAYED_SHTDN_TIMER_CONFIG
{
    USHORT           delayedShtdnTimerValue; // In unit of seconds.
}
IPMI_P_DELAYED_SHTDN_TIMER_CONFIG, *PIPMI_P_DELAYED_SHTDN_TIMER_CONFIG;

//++
// Name:
//      IPMI_RESPONSE_RESERVE_SEL
//
// Description:
//      Data portion of IPMI response for 'Reserve SEL' command.
//--
typedef struct _IPMI_RESPONSE_RESERVE_SEL
{
    USHORT  reservationID;
}
IPMI_RESPONSE_RESERVE_SEL, *PIPMI_RESPONSE_RESERVE_SEL;


//++
// Name:
//      IPMI_REQUEST_CLEAR_SEL
//
// Description:
//      Data portion of IPMI request for 'Clear SEL' command.
//      operation: 0xAA = initiate erase; 0x00 = get erasure status
//--
typedef struct _IPMI_REQUEST_CLEAR_SEL
{
    USHORT  reservationID;
    UCHAR   signature[3];
    UCHAR   operation;
}
IPMI_REQUEST_CLEAR_SEL, *PIPMI_REQUEST_CLEAR_SEL;

//++
// Name:
//      IPMI_RESPONSE_CLEAR_SEL
//
// Description:
//      Data portion of IPMI response for 'Clear SEL' command.
//      0h = erasure in progress; 1h = erase completed.
//--
typedef struct _IPMI_RESPONSE_CLEAR_SEL
{
    UCHAR   erasureProgess;
}
IPMI_RESPONSE_CLEAR_SEL, *PIPMI_RESPONSE_CLEAR_SEL;

//++
// Name:
//      IPMI_RESPONSE_GET_SEL_INFO
//
// Description:
//      Data portion of IPMI response for 'Get SEL INFO' command.
//--
typedef struct _IPMI_RESPONSE_GET_SEL_INFO
{
    //
    // SEL Version. 51h for IPMIv2.0. BCD encoded.
    //
    // bit [3:0]
    UCHAR          majorVersion:4;
    // bit [7:4]
    UCHAR          minVersion:4;
    USHORT         numEntries;
    USHORT         freeSpaceSize;
    UINT32         addTimestamp;
    UINT32         eraseTimestamp;
    // bit [0]
    UCHAR          supportGetAllocInfo:1;
    // bit [1]
    UCHAR          supportResv:1;
    // bit [2]
    UCHAR          supportPartialAdd:1;
    // bit [3]
    UCHAR          supportDel:1;
    // bit [6:4]
    UCHAR          resved:3;
    // bit [7]
    UCHAR          overflow:1;
}
IPMI_RESPONSE_GET_SEL_INFO, *PIPMI_RESPONSE_GET_SEL_INFO;

//++
// Name:
//      IPMI_RESPONSE_GET_SEL_TIME
//
// Description:
//      Data portion of IPMI response for 'Get SEL TIME' command.
//--
typedef struct _IPMI_RESPONSE_GET_SEL_TIME
{
    UINT32         SELTimestamp;
}
IPMI_RESPONSE_GET_SEL_TIME, *PIPMI_RESPONSE_GET_SEL_TIME;


//++
// Name:
//      IPMI_REQUEST_GET_SEL_ENTRY
//
// Description:
//      'Get SEL Entry' command data buffer.
//--
typedef struct _IPMI_REQUEST_GET_SEL_ENTRY
{
    //
    // Reservation ID, only required for partial reads with a non-zero offset-into-record field.
    // Use 0000h otherwise.
    //
    USHORT           reservationId;
    
    //
    // Record ID of record to get.
    //
    USHORT           recordId;
    
    //
    // Offset into record for partial reads.
    //
    UCHAR            offset;
    
    //
    // Bytes to read (0xff for entire record).
    //
    UCHAR            bytesToRead;

}
IPMI_REQUEST_GET_SEL_ENTRY, *PIPMI_REQUEST_GET_SEL_ENTRY;

//++
// Name:
//      IPMI_RESPONSE_GET_SEL_ENTRY
//
// Description:
//      Data portion of IPMI response for 'Get SEL ENTRY' command.
//--
typedef struct _IPMI_RESPONSE_GET_SEL_ENTRY
{
    // Next SEL Record ID. 
    // Returned 0xFFFF means the requested ID is the last.
    USHORT         nextRecordId;
    // 16 bytes Record Data
    UCHAR          data[IPMI_SEL_RECORD_SIZE];
}
IPMI_RESPONSE_GET_SEL_ENTRY, *PIPMI_RESPONSE_GET_SEL_ENTRY;


//++
// Name:
//      IPMI_STD_SEL_RECORD
//
// Description:
//      'SEL Record Standard Format, IPMIv2.0, Table 32-1.
//      Record Type should be 0x02.
//--
typedef struct _IPMI_STD_SEL_RECORD 
{
    UINT32        timeStamp;
    USHORT        genId;
    UCHAR         evmRev;
    UCHAR         sensorType;
    UCHAR         sensorNumber;
    UCHAR         eventType:7;
    UCHAR         eventDir:1;
    UCHAR         eventData[3];
}
IPMI_STD_SEL_RECORD, *PIPMI_STD_SEL_RECORD;


//++
// Name:
//      IPMI_OEM_TS_SEL_RECORD
//
// Description:
//      'SEL Record OEM Format, IPMIv2.0, Table 32-2.
//      Record Type should be 0xC0 - 0xDF.
//--
typedef struct _IPMI_OEM_TS_SEL_RECORD
{
    UINT32        timeStamp;
    UINT8         manufactureId[3];
    UINT8         oemData[IPMI_SEL_OEM_TS_DATA_LEN];
}
IPMI_OEM_TS_SEL_RECORD, *PIPMI_OEM_TS_SEL_RECORD;


//++
// Name:
//      IPMI_OEM_NOTS_SEL_RECORD
//
// Description:
//      'SEL Record OEM Format, IPMIv2.0, Table 32-3.
//      Record Type should be 0xE0 - 0xFF.
//--
typedef struct _IPMI_OEM_NOTS_SEL_RECORD
{
    UINT8         oemData[IPMI_SEL_OEM_NOTS_DATA_LEN];
}
IPMI_OEM_NOTS_SEL_RECORD, *PIPMI_OEM_NOTS_SEL_RECORD;

//++
// Name:
//      IPMI_SEL_RECORD
//
// Description:
//      'SEL Record general Format
//--
typedef struct _IPMI_SEL_RECORD
{
    USHORT        recordId;
    UCHAR         recordType;
    union 
    {
        IPMI_STD_SEL_RECORD              stdType;
        IPMI_OEM_TS_SEL_RECORD           oemTsType;
        IPMI_OEM_NOTS_SEL_RECORD         oemNoTsType;
    } selType;
}
IPMI_SEL_RECORD, *PIPMI_SEL_RECORD;


typedef struct _IPMI_EVENT_SENSOR_TYPES
{
    UCHAR             code;
    UCHAR             offset;
    UCHAR             data;
    UCHAR             sensorClass;
    const CHAR        *type;
    const CHAR        *desc;
}
IPMI_EVENT_SENSOR_TYPES, *PIPMI_EVENT_SENSOR_TYPES;
//++
// Name:
//      IPMI_REQUEST_EXTEND_LOG_HEADER
//
// Description:
//     Get extended log header command request
//--
typedef struct _IPMI_REQUEST_EXTEND_LOG_HEADER
{
    // LSB first.
    USHORT                  recordId;
    // always 0xFF
    UCHAR                   reserved;
}
IPMI_REQUEST_EXTEND_LOG_HEADER, *PIPMI_REQUEST_EXTEND_LOG_HEADER;


//++
// Name:
//      IPMI_EXTEND_LOG_HEADER
//
// Description:
//     Get extended log header command response data.
//--
typedef struct _IPMI_EXTEND_LOG_HEADER
{
    IPMI_SEL_RECORD         standardSEL;
    // current revision is 0x01 .
    UCHAR                   extendLogRevision;
    // LSB first.
    USHORT                  extendLogDataSize;
}
IPMI_EXTEND_LOG_HEADER, *PIPMI_EXTEND_LOG_HEADER;

//++
// Name:
//      IPMI_RESPONSE_EXTEND_LOG_HEADER
//
// Description:
//     Get extended log header command response.
//--
typedef struct _IPMI_RESPONSE_EXTEND_LOG_HEADER
{
    // LSB first.
    USHORT                  nextRecordId;
    IPMI_EXTEND_LOG_HEADER  extendLogHeader;
}
IPMI_RESPONSE_EXTEND_LOG_HEADER, *PIPMI_RESPONSE_EXTEND_LOG_HEADER;


//++
// Name:
//      IPMI_REQUEST_EXTEND_LOG_DATA
//
// Description:
//     Get extended log data command request
//--
typedef struct _IPMI_REQUEST_EXTEND_LOG_DATA
{
    // LSB first.
    USHORT                  recordId;
    // always 0xFF
    UCHAR                   reserved;
    UCHAR                   bytesToRead;
    // LSB fisrt
    USHORT                  offset;
}
IPMI_REQUEST_EXTEND_LOG_DATA, *PIPMI_REQUEST_EXTEND_LOG_DATA;


//++
// Name:
//      IPMI_EXTEND_LOG_DATA
//
// Description:
//     structure to save the whole extend log data. 
//--
typedef struct _IPMI_EXTEND_LOG_DATA
{
    // add this for tracking.
    USHORT          recordId;
    USHORT          dataLength;
    // returned in response.
    UCHAR           dataType;
    UCHAR           data[1023];
}
IPMI_EXTEND_LOG_DATA, *PIPMI_EXTEND_LOG_DATA;


//++
// Name:
//      IPMI_RESPONSE_GET_SDR_INFO
//
// Description:
//      'Get SDR Repository Info' command data buffer.
//--
typedef struct _IPMI_RESPONSE_GET_SDR_INFO
{
    //
    // SDR version, should be 51h.
    //
    UCHAR            sdrVersion;
    
    //
    // Number of records in the repository.
    //
    USHORT           recordCount;

    //
    // Free space
    //
    USHORT           freeSpace;

    //
    // most recent addition timestamp. LSB first
    //
    UINT32           addTimestamp;
    
    //
    // most recent erase(delete or clear) timestamp. LSB first
    //
    UINT32           eraseTimestamp;

    // Byte-15: Operation Support Flags
#ifdef CSX_AD_ENDIAN_TYPE_LITTLE
    // bit 0: Get SDR Repository Allocation Information command supported.
    //
    UCHAR            getSdrRepositoryAlloInfo:1;
    //
    // bit 1: Reserve SDR Repository command supported.
    //
    UCHAR            reserveSdrRepository:1;
    //
    // bit 2: Partial Add SDR command supported.
    //
    UCHAR            partialAddSdr:1;
    //
    // bit 3: Delete SDR command supported.
    //
    UCHAR            DeleteSdr:1;
    //
    // bit 4: reserved, shoud be 0.
    //
    UCHAR            reserved:1;
    //
    // bit 5-6: modal operation support.
    // 00 - modal/non-modal SDR Repository Update operation unspecified 
    // 01 - non-modal SDR Repository Update operation supported
    // 10 - modal SDR Repository Update operation supported
    // 11 - both modal and non-modal SDR Repository Update supported 
    //
    UCHAR            modalUpdate:2;
    //
    // bit 7: Overflow flag. If 1, SDR could not be written due to lack of space.
    //
    UCHAR            overflow:1;
#else
    UCHAR            overflow:1;
    UCHAR            modalUpdate:2;
    UCHAR            reserved:1;
    UCHAR            DeleteSdr:1;
    UCHAR            partialAddSdr:1;
    UCHAR            reserveSdrRepository:1;
    UCHAR            getSdrRepositoryAlloInfo:1;
#endif
}
IPMI_RESPONSE_GET_SDR_INFO, *PIPMI_RESPONSE_GET_SDR_INFO;


//++
// Name:
//      IPMI_REQUEST_GET_SDR
//
// Description:
//      'Get SDR' command data buffer.
//--
typedef struct _IPMI_REQUEST_GET_SDR
{
    //
    // Reservation ID, only required for partial reads with a non-zero offset-into-record field.
    //
    USHORT           reservationId;
    
    //
    // Record ID of record to get.
    //
    USHORT           recordId;
    
    //
    // Offset into record for partial reads.
    //
    UCHAR            offset;
    
    //
    // Bytes to read (0xff for entire record).
    //
    UCHAR            bytesToRead;

}
IPMI_REQUEST_GET_SDR, *PIPMI_REQUEST_GET_SDR;


//++
// Name:
//      IPMI_RESPONSE_GET_SDR
//
// Description:
//      Data portion of IPMI response for 'Get SDR' command.
//--
typedef struct _IPMI_RESPONSE_GET_SDR
{
    //
    // Record Id for the next record in the SDR repository.
    //
    USHORT          nextRecordId;

    //
    // Record data.
    //
    UCHAR           data [IPMI_SDR_FULL_RECORD_SIZE];
}
IPMI_RESPONSE_GET_SDR, *PIPMI_RESPONSE_GET_SDR;

//++
// Name:
//      IPMI_SDR_RECORD_HEADER
//
// Description:
//      SDR sensor record header.
//--
typedef struct _IPMI_SDR_RECORD_HEADER 
{
    USHORT                  id;
    UCHAR                   version;
    UCHAR                   type;
    UCHAR                   length;
}
IPMI_SDR_RECORD_HEADER, *PIPMI_SDR_RECORD_HEADER;

//++
// Name:
//      IPMI_ENTITY_ID
//
// Description:
//      The Entity ID field is used for identifying the physical entity that 
//      a sensor or device is associated with. See Table 43-13 for Entity ID Codes.
//      The Entity ID is associated with an Entity Instance value that is used to 
//      indicate the particular instance of an entity.
//--
typedef struct _IPMI_ENTITY_ID 
{
    // physical entity id, table 43-13
    UCHAR    id;            
    // instance number, implementer defined
    // 00h-5Fh  system-relative Entity Instance.
    // 60h-7Fh  device-relative Entity Instance. 
    UCHAR    instance:7;    
    // physical/logical
    UCHAR    logical:1;    
}
IPMI_ENTITY_ID, *PIPMI_ENTITY_ID;


//++
// Name:
//      IPMI_SDR_RECORD_MASK
//
// Description:
//      To store all SDR records queried from BMC. Total 2*3 bytes. 
//      Used in Full and Compact SDR types. See Table 43-1, 43-2.
//--
typedef struct _IPMI_SDR_RECORD_MASK
{
    union 
    {
        // For Discrete or non-threshold based Sensors
        struct 
        {
            // assertion event mask
            USHORT assert_event;    
            // de-assertion event mask 
            USHORT deassert_event;    
            // discrete reading mask
            USHORT read;    
        } discrete;

        // For threshold based sensors
        // lnc - low non-critical
        // lcr - low critical
        // lnr - low non-recoverable
        // unr - upper non-recoverable
        // unc - upper non-critical
        // ucr - upper critical
        struct 
        {
            // byte 15, 16:
            //Assertion Event Mask or Lower threshold reading mask
            USHORT assert_lnc_low:1;
            USHORT assert_lnc_high:1;
            USHORT assert_lcr_low:1;
            USHORT assert_lcr_high:1;
            USHORT assert_lnr_low:1;
            USHORT assert_lnr_high:1;
            USHORT assert_unc_low:1;
            USHORT assert_unc_high:1;
            USHORT assert_ucr_low:1;
            USHORT assert_ucr_high:1;
            USHORT assert_unr_low:1;
            USHORT assert_unr_high:1;
            USHORT status_lnc:1;
            USHORT status_lcr:1;
            USHORT status_lnr:1;
            USHORT reserved:1;
            // byte 17, 18:
            // Deassertion Event Mask / Upper Threshold Reading Mask
            USHORT deassert_lnc_low:1;
            USHORT deassert_lnc_high:1;
            USHORT deassert_lcr_low:1;
            USHORT deassert_lcr_high:1;
            USHORT deassert_lnr_low:1;
            USHORT deassert_lnr_high:1;
            USHORT deassert_unc_low:1;
            USHORT deassert_unc_high:1;
            USHORT deassert_ucr_low:1;
            USHORT deassert_ucr_high:1;
            USHORT deassert_unr_low:1;
            USHORT deassert_unr_high:1;
            USHORT status_unc:1;
            USHORT status_ucr:1;
            USHORT status_unr:1;
            USHORT reserved_2:1;

            // byte 19, 20, LSB first.
            union 
            {
                // settable threshold mask
                struct 
                {
                    USHORT readable:8;
                    USHORT lnc:1;
                    USHORT lcr:1;
                    USHORT lnr:1;
                    USHORT unc:1;
                    USHORT ucr:1;
                    USHORT unr:1;
                    USHORT reserved:2;
                } set;

                // Readable threshold mask
                struct 
                {
                    USHORT lnc:1;
                    USHORT lcr:1;
                    USHORT lnr:1;
                    USHORT unc:1;
                    USHORT ucr:1;
                    USHORT unr:1;
                    USHORT reserved:2;
                    USHORT settable:8;
                } read;
            };
        } threshold;
    } type;
}
IPMI_SDR_RECORD_MASK, *PIPMI_SDR_RECORD_MASK;

//++
// Name:
//      IPMI_SDR_RECORD_FULL_SENSOR
//
// Description:
//      SDR Full sensor type.
//--
typedef struct _IPMI_SDR_RECORD_FULL_SENSOR 
{
    struct 
    {
        UCHAR owner_id;
        // sensor owner lun
        UCHAR lun:2;
        UCHAR __reserved0:2;
        UCHAR channel:4;
        //Unique number identifying the sensor behind a given slave address and LUN.
        UCHAR sensor_num;
    } keys;

    IPMI_ENTITY_ID entity;

    struct 
    {
        struct 
        {
            UCHAR sensor_scan:1;
            UCHAR event_gen:1;
            UCHAR type:1;
            UCHAR hysteresis:1;
            UCHAR thresholds:1;
            UCHAR events:1;
            UCHAR scanning:1;
            UCHAR settable:1;
        } init;
        
        struct 
        {
            UCHAR event_msg:2;
            UCHAR threshold:2;
            UCHAR hysteresis:2;
            UCHAR rearm:1;
            UCHAR ignore:1;
        } capabilities;

        // table 42-3
        UCHAR type;
    } sensor;

    // event/reading type code, table 42-1
    UCHAR event_type;

    IPMI_SDR_RECORD_MASK mask;

    struct 
    {
        UCHAR pct:1;
        UCHAR modifier:2;
        UCHAR rate:3;
        UCHAR analog:2;
        
        struct 
        {
            UCHAR base;
            UCHAR modifier;
        } type;
    } unit;

    // 70h=non linear, 71h-7Fh=non linear, OEM
    UCHAR linearization;
    
    // M, tolerance
    // M: 10bit signed
    // Tolerance: 6bit unsigned
    USHORT  mtol;
    // accuracy, Aexp, B, Bexp, Rexp, sensor direction
    // B: 10bit signed
    // A: 10bit unsigned
    // Aexp: 2bit unsigned
    // Sensor Dir: 2bit
    // Rexp: 4bit signed
    // Bexp: 4bit signed
    UINT32 bacc;

    struct 
    {
        // nominal reading field specified 
        UCHAR nominal_read:1;
        // normal max field specified 
        UCHAR normal_max:1;
        // normal min field specified
        UCHAR normal_min:1;
        UCHAR __reserved1:5;
    } analog_flag;

    // nominal reading, raw value
    UCHAR nominal_read;    
    // normal maximum, raw value 
    UCHAR normal_max;    
    // normal minimum, raw value
    UCHAR normal_min;    
    // sensor maximum, raw value
    UCHAR sensor_max;    
    // sensor minimum, raw value
    UCHAR sensor_min;    

    struct 
    {
        struct 
        {
            UCHAR non_recover;
            UCHAR critical;
            UCHAR non_critical;
        } upper;
        
        struct 
        {
            UCHAR non_recover;
            UCHAR critical;
            UCHAR non_critical;
        } lower;
        
        struct 
        {
            UCHAR positive;
            UCHAR negative;
        } hysteresis;
    } threshold;
    
    UCHAR __reserved2[2];
    // reserved for OEM use
    UCHAR oem;        
    // sensor ID string type/length code 
    UCHAR id_code;    
    // sensor ID string bytes, only if id_code != 0 
    UCHAR id_string[16];    
}
IPMI_SDR_RECORD_FULL_SENSOR, *PIPMI_SDR_RECORD_FULL_SENSOR;

//++
// Name:
//      IPMI_SDR_RECORD_COMPACT_SENSOR
//
// Description:
//      SDR compact sensor type.
//--
typedef struct _IPMI_SDR_RECORD_COMPACT_SENSOR
{
    struct 
    {
        UCHAR owner_id;
        // sensor owner lun
        UCHAR lun:2;
        UCHAR FRUOwnerLun:2;
        UCHAR channel:4;
        //Unique number identifying the sensor behind a given slave address and LUN.
        UCHAR sensor_num;
    } keys;

    IPMI_ENTITY_ID entity;

    struct 
    {
        struct 
        {
            UCHAR sensor_scan:1;
            UCHAR event_gen:1;
            UCHAR type:1;
            UCHAR hysteresis:1;
            UCHAR __reserved0:1;
            UCHAR events:1;
            UCHAR scanning:1;
            UCHAR settable:1;
        } init;
        
        struct 
        {
            UCHAR event_msg:2;
            UCHAR threshold:2;
            UCHAR hysteresis:2;
            UCHAR rearm:1;
            // ignore sensor if Entity is not present or disabled
            UCHAR ignore:1;
        } capabilities;

        // table 42-3
        UCHAR type;
    } sensor;

    // event/reading type code, table 42-1
    UCHAR event_type;

    IPMI_SDR_RECORD_MASK mask;

    struct 
    {
        UCHAR percentage:1;
        UCHAR modifier:2;
        UCHAR rate:3;
        // write as 11b
        UCHAR __reserved1:2;
        
        struct 
        {
            UCHAR base;
            UCHAR modifier;
        } type;
    } unit;

    struct 
    {
        UCHAR count:4;
        UCHAR mod_type:2;
        UCHAR sensorDirection:2;
        UCHAR mod_offset:7;
        UCHAR entity_inst:1;
    } share;

    struct 
    {
        struct 
        {
            UCHAR positive;
            UCHAR negative;
        } hysteresis;
    } threshold;

    // write as 00h
    UCHAR __reserved2[3];
    // reserved for OEM use
    UCHAR oem;        
    // sensor ID string type/length code 
    UCHAR id_code;    
    // sensor ID string bytes, only if id_code != 0 
    UCHAR id_string[16];    
}
IPMI_SDR_RECORD_COMPACT_SENSOR, *PIPMI_SDR_RECORD_COMPACT_SENSOR;

//++
// Name:
//      IPMI_SDR_RECORD_EVENTONLY_SENSOR
//
// Description:
//      SDR event only sensor type.
//--
typedef struct _IPMI_SDR_RECORD_EVENTONLY_SENSOR
{
    struct 
    {
        UCHAR owner_id;
        // sensor owner lun
        UCHAR lun:2;
        UCHAR FRUOwnerLun:2;
        UCHAR channel:4;
        //Unique number identifying the sensor behind a given slave address and LUN.
        UCHAR sensor_num;
    } keys;

    IPMI_ENTITY_ID entity;

    // table 36-3
    UCHAR type;

    // event/reading type code, table 36-1
    UCHAR event_type;

    struct 
    {
        UCHAR count:4;
        UCHAR mod_type:2;
        UCHAR sensorDirection:2;
        UCHAR mod_offset:7;
        UCHAR entity_inst:1;
    } share;
    // write as 00h
    UCHAR __reserved0;
    // reserved for OEM use
    UCHAR oem;        
    // sensor ID string type/length code 
    UCHAR id_code;    
    // sensor ID string bytes, only if id_code != 0 
    UCHAR id_string[16];    
}
IPMI_SDR_RECORD_EVENTONLY_SENSOR, *PIPMI_SDR_RECORD_EVENTONLY_SENSOR;

//++
// Name:
//      IPMI_SDR_RECORD_ENTITY_ASSOC_SENSOR
//
// Description:
//      SDR entity association sensor type.
//--
typedef struct _IPMI_SDR_RECORD_ENTITY_ASSOC_SENSOR
{
    // container entity ID and instance
    IPMI_ENTITY_ID entity;
    
    struct 
    {
        // 00000b
        UCHAR __reserved0:5;
        UCHAR isaccessable:1;
        UCHAR islinked:1;
        UCHAR isrange:1;
    } flags;

    // entity ID 1    |  range 1 entity 
    UCHAR entity_id_1;    
    // entity inst 1  |  range 1 first instance
    UCHAR entity_inst_1;  
    // entity ID 2    |  range 1 entity 
    UCHAR entity_id_2;    
    // entity inst 2  |  range 1 last instance 
    UCHAR entity_inst_2;  
    // entity ID 3    |  range 2 entity 
    UCHAR entity_id_3;    
    // entity inst 3  |  range 2 first instance 
    UCHAR entity_inst_3;  
    // entity ID 4    |  range 2 entity 
    UCHAR entity_id_4;    
    // entity inst 4  |  range 2 last instance 
    UCHAR entity_inst_4;    

}
IPMI_SDR_RECORD_ENTITY_ASSOC_SENSOR, *PIPMI_SDR_RECORD_ENTITY_ASSOC_SENSOR;

//++
// Name:
//      IPMI_SDR_RECORD_GENERIC_LOCATOR_SENSOR
//
// Description:
//      SDR generic device locator sensor type.
//--
typedef struct _IPMI_SDR_RECORD_GENERIC_LOCATOR_SENSOR
{
    UCHAR           __reserved0:1;
    UCHAR           dev_access_addr:7;
    // Channel Number MS 1bit
    UCHAR           channel_num_ms:1;
    UCHAR           dev_slave_addr:7;
    UCHAR           busID:3;
    UCHAR           lun:2;
    // Channel Number LS 3bits
    UCHAR           channel_num_ls:3;
    UCHAR           __reserved1:5;
    UCHAR           addr_span:3;
    UCHAR           __reserved2;
    UCHAR           dev_type;
    UCHAR           dev_type_modifier;
    IPMI_ENTITY_ID  entity;
    UCHAR           oem;
    UCHAR           id_code;
    UCHAR           id_string[16];
}
IPMI_SDR_RECORD_GENERIC_LOCATOR_SENSOR, *PIPMI_SDR_RECORD_GENERIC_LOCATOR_SENSOR;

//++
// Name:
//      IPMI_SDR_RECORD_FRU_LOCATOR_SENSOR
//
// Description:
//      SDR FRU device locator sensor type.
//--
typedef struct _IPMI_SDR_RECORD_FRU_LOCATOR_SENSOR
{
    //Slave address of controller used to access device
    // 0000000b if directly on IPMB
    UCHAR           dev_access_addr;
    
    union
    {
        struct
        {
            UCHAR           __reserved1:1;
            UCHAR           dev_slave_addr:7;
        } non_intelligent_fru;
        UCHAR  logical_fru;
    } device_id;
    
    UCHAR bus:3;
    UCHAR lun:2;
    UCHAR __reserved2:2;
    UCHAR logical:1;
    
    UCHAR __reserved3:4;
    UCHAR channel_num:4;
    
    UCHAR __reserved4;
    // table 43-12
    UCHAR dev_type;
    UCHAR dev_type_modifier;
    
    IPMI_ENTITY_ID  entity;
    
    UCHAR oem;
    UCHAR id_code;
    UCHAR id_string[16];
}
IPMI_SDR_RECORD_FRU_LOCATOR_SENSOR, *PIPMI_SDR_RECORD_FRU_LOCATOR_SENSOR;

//++
// Name:
//      IPMI_SDR_RECORD_MC_LOCATOR_SENSOR
//
// Description:
//      SDR management controller device locator sensor type.
//--
typedef struct _IPMI_SDR_RECORD_MC_LOCATOR_SENSOR
{
    //Slave address of controller used to access device
    // 0000000b if directly on IPMB
    UCHAR           dev_slave_addr;
    
    UCHAR           channel_num:4;
    UCHAR           __reserved1:4;
    
    UCHAR           global_init:4;
    UCHAR           __reserved2:1;
    UCHAR           pwr_state_notify:3;
    
    UCHAR           dev_support;

    UCHAR           __reserved3[3];
    
    IPMI_ENTITY_ID  entity;

    UCHAR           oem;
    UCHAR           id_code;
    UCHAR           id_string[16];
}
IPMI_SDR_RECORD_MC_LOCATOR_SENSOR, *PIPMI_SDR_RECORD_MC_LOCATOR_SENSOR;


//++
// Name:
//      IPMI_SDR_RECORD_LIST
//
// Description:
//      To store all SDR records queried from BMC.
//--
typedef struct _IPMI_SDR_RECORD_LIST
{
    EMCPAL_LIST_ENTRY       list;
    IPMI_SDR_RECORD_HEADER  header;
    union 
    {
        IPMI_SDR_RECORD_FULL_SENSOR                 fullSensor;
        IPMI_SDR_RECORD_COMPACT_SENSOR              compactSensor;
        IPMI_SDR_RECORD_EVENTONLY_SENSOR            eventonlySensor;
        IPMI_SDR_RECORD_GENERIC_LOCATOR_SENSOR      genloc;
        IPMI_SDR_RECORD_FRU_LOCATOR_SENSOR          fruloc;
        IPMI_SDR_RECORD_MC_LOCATOR_SENSOR           mcloc;
        IPMI_SDR_RECORD_ENTITY_ASSOC_SENSOR         entassoc;
        UCHAR                                       rawdata[IPMI_SDR_MAX_DATA_SIZE];
    } record;
}
IPMI_SDR_RECORD_LIST, *PIPMI_SDR_RECORD_LIST;

//++
// Name:
//      IPMI_LED_ID
//
// Description:
//      LED IDs
//--
typedef enum _IPMI_LED_ID_MEGATRON_JETFIRE
{
   IPMI_MT_JF_LED_ID_NOT_SAFE_TO_REMOVE              = 0x00,
   IPMI_MT_JF_LED_ID_MANAGEMENT_FAULT                = 0x01,
   IPMI_MT_JF_LED_ID_SP_FAULT                        = 0x02,
   IPMI_MT_JF_LED_ID_INVALID                         = 0xFF
} IPMI_LED_ID_MEGATRON_JETFIRE, *PIMPI_LED_ID_MEGATRON_JETFIRE;

typedef enum _IPMI_LED_ID_MEGATRON
{
   IPMI_MT_LED_ID_FAN0_FAULT                      = 0x03,
   IPMI_MT_LED_ID_FAN1_FAULT                      = 0x04,
   IPMI_MT_LED_ID_FAN2_FAULT                      = 0x05,
   IPMI_MT_LED_ID_FAN3_FAULT                      = 0x06,
   IPMI_MT_LED_ID_FAN4_FAULT                      = 0x07,
   IPMI_MT_LED_ID_SLIC0_FAULT                     = 0x08,
   IPMI_MT_LED_ID_SLIC1_FAULT                     = 0x09,
   IPMI_MT_LED_ID_SLIC2_FAULT                     = 0x0A,
   IPMI_MT_LED_ID_SLIC3_FAULT                     = 0x0B,
   IPMI_MT_LED_ID_SLIC4_FAULT                     = 0x0C,
   IPMI_MT_LED_ID_SLIC5_FAULT                     = 0x0D,
   IPMI_MT_LED_ID_SLIC6_FAULT                     = 0x0E,
   IPMI_MT_LED_ID_SLIC7_FAULT                     = 0x0F,
   IPMI_MT_LED_ID_SLIC8_FAULT                     = 0x10,
   IPMI_MT_LED_ID_SLIC9_FAULT                     = 0x11,
   IPMI_MT_LED_ID_SLIC10_FAULT                    = 0x12,
   IPMI_MT_LED_ID_PSU0_FAULT                      = 0x13,
   IPMI_MT_LED_ID_PSU1_FAULT                      = 0x14,
   IPMI_MT_LED_ID_INVALID                         = 0xFF
} IPMI_LED_ID_MEGATRON, *PIMPI_LED_ID_MEGATRON;

typedef enum _IPMI_LED_ID_JETFIRE
{
   IPMI_JF_LED_ID_SAS1_GREEN_FAULT              = 0x03,
   IPMI_JF_LED_ID_SAS1_BLUE_FAULT               = 0x04,
   IPMI_JF_LED_ID_FAN0_FAULT                    = 0x05,
   IPMI_JF_LED_ID_FAN1_FAULT                    = 0x06,
   IPMI_JF_LED_ID_FAN2_FAULT                    = 0x07,
   IPMI_JF_LED_ID_FAN3_FAULT                    = 0x08,
   IPMI_JF_LED_ID_BOB_FAULT                     = 0x09,
   IPMI_JF_LED_ID_SLIC0_FAULT                   = 0x0A,
   IPMI_JF_LED_ID_SLIC1_FAULT                   = 0x0B,
   IPMI_JF_LED_ID_SLIC2_FAULT                   = 0x0C,
   IPMI_JF_LED_ID_SLIC3_FAULT                   = 0x0D,
   IPMI_JF_LED_ID_SLIC4_FAULT                   = 0x0E,
   IPMI_JF_LED_ID_BEM_FAULT                     = 0x0F,
   IPMI_JF_LED_ID_PSUA_FAULT                    = 0x10,
   IPMI_JF_LED_ID_PSUB_FAULT                    = 0x11,
   IPMI_JF_LED_ID_INVALID                       = 0xFF
} IPMI_LED_ID_JETFIRE, *PIMPI_LED_ID_JETFIRE;

typedef enum _IPMI_LED_ID_BEACHCOMBER
{
   IPMI_BC_LED_ID_NOT_SAFE_TO_REMOVE              = 0x00,
   IPMI_BC_LED_ID_SP_FAULT                        = 0x01,
   IPMI_BC_CM_A0_FAULT                            = 0x02,
   IPMI_BC_CM_A1_FAULT                            = 0x03,
   IPMI_BC_CM_A2_FAULT                            = 0x04,
   IPMI_BC_CM_B0_FAULT                            = 0x05,
   IPMI_BC_CM_B1_FAULT                            = 0x06,
   IPMI_BC_CM_B2_FAULT                            = 0x07,
   IPMI_BC_LED_ID_EHORNET_FAULT                   = 0x08,
   IPMI_BC_LED_ID_PSUA_FAULT                      = 0x09,
   IPMI_BC_LED_ID_PSUB_FAULT                      = 0x0A,
   IPMI_BC_LED_ID_INVALID                         = 0xFF
} IPMI_LED_ID_BEACHCOMBER, *PIPMI_LED_ID_BEACHCOMBER;

//++
// Name:
//      IPMI_REQUEST_GET_LED_BLINK_RATE
//
// Description:
//      'Get LED Blink Rate' command data buffer.
//--
typedef struct _IPMI_REQUEST_GET_LED_BLINK_RATE
{
    //
    // LED Id.
    //
    UCHAR           ledId;

}
IPMI_REQUEST_GET_LED_BLINK_RATE, *PIPMI_REQUEST_GET_LED_BLINK_RATE;


//++
// Name:
//      IPMI_RESPONSE_GET_LED_BLINK_RATE
//
// Description:
//      Data portion of IPMI response for 'Get LED Blink Rate' command.
//--
typedef struct _IPMI_RESPONSE_GET_LED_BLINK_RATE
{
    UCHAR                   ledId;
    UCHAR                   blinkRate;
    UCHAR                   currentState;
    UCHAR                   finalState;
    UCHAR                   repeatCount;
    UCHAR                   blinksRemaining;
    UCHAR                   reserved;
}
IPMI_RESPONSE_GET_LED_BLINK_RATE, *PIPMI_RESPONSE_GET_LED_BLINK_RATE;

#define IPMI_SETLEDBLNKRT_MANUAL_OVERRIDE    0x01
#define IPMI_SETLEDBLNKRT_BMC_CONTROL        0x00
//++
// Name:
//      IPMI_REQUEST_SET_LED_BLINK_RATE
//
// Description:
//      'Set LED Blink Rate' command data buffer.
//--
typedef struct _IPMI_REQUEST_SET_LED_BLINK_RATE
{
    UCHAR                       ledId;
    UCHAR                       blinkRate;
    UCHAR                       phase;
    UCHAR                       finalState;
    UCHAR                       repeatCount;
    UCHAR                       reserved;
}
IPMI_REQUEST_SET_LED_BLINK_RATE, *PIPMI_REQUEST_SET_LED_BLINK_RATE;

//++
// Name:
//      IPMI_AIRDAM_LED_ID
//
// Description:
//      Airdam LED IDs
//--
typedef enum _IPMI_PROTOCOL_LED_ID_BEACHCOMBER
{
    IPMI_BC_LED_GE_M_PORT                       = 0x00,
    IPMI_BC_LED_GE_S_PORT                       = 0x01,
    IPMI_BC_AIRDAM_LED_ID_10G_PORT_0            = 0x02,
    IPMI_BC_AIRDAM_LED_ID_10G_PORT_1            = 0x03,
    IPMI_BC_AIRDAM_LED_ID_10G_PORT_2            = 0x04,
    IPMI_BC_AIRDAM_LED_ID_10G_PORT_3            = 0x05,
    IPMI_BC_AIRDAM_LED_ID_SAS_PORT_0            = 0x06,
    IPMI_BC_AIRDAM_LED_ID_SAS_PORT_1            = 0x07,
    IPMI_BC_AIRDAM_LED_ID_INVALID               = 0xFF
} IPMI_PROTOCOL_LED_ID_BEACHCOMBER, *PIPMI_PROTOCOL_LED_ID_BEACHCOMBER;

//++
// Name:
//      IPMI_PROTOCOL_LED_ID_BEARCAT
//
// Description:
//      Airdam LED IDs on Bearcat
//--
typedef enum _IPMI_PROTOCOL_LED_ID_BEARCAT
{
    IPMI_BT_LED_GE_M_PORT                       = 0x00,
    IPMI_BT_LED_GE_S_PORT                       = 0x01,
    IPMI_BT_AIRDAM_LED_ID_HILDA_PORT_0          = 0x02, // Labeled with "CNA3"
    IPMI_BT_AIRDAM_LED_ID_HILDA_PORT_1          = 0x03, // Labeled with "CNA2"
    IPMI_BT_AIRDAM_LED_ID_SAS_PORT_0            = 0x06,
    IPMI_BT_AIRDAM_LED_ID_SAS_PORT_1            = 0x07,
    IPMI_BT_AIRDAM_LED_ID_INVALID               = 0xFF
} IPMI_PROTOCOL_LED_ID_BEARCAT, *PIPMI_PROTOCOL_LED_ID_BEARCAT;

#define IPMI_PROTOCOL_LED_NOT_CONTROLLED      0x00
#define IPMI_PROTOCOL_LED_MARK                0x01
#define IPMI_PROTOCOL_SAS_LED_SOLID_BLUE      0x02
#define IPMI_PROTOCOL_ETH_LED_LNKACT_SOLID_ON 0x02

#define IPMI_PROTOCOL_HILDA_LED_SOLID_OFF     0x01
#define IPMI_PROTOCOL_HILDA_LED_SOLID_GREEN   0x02
#define IPMI_PROTOCOL_HILDA_LED_BLINK_GREEN   0x03
#define IPMI_PROTOCOL_HILDA_LED_SOLID_BLUE    0x04
#define IPMI_PROTOCOL_HILDA_LED_BLINK_BLUE    0x05



//++
// Name:
//      IPMI_REQUEST_SET_BEACHCOMBER_LED_STATE
//
// Description:
//      'Set Beachcomber LED State' command data buffer.
//--
typedef struct _IPMI_REQUEST_SET_BEACHCOMBER_LED_STATE
{
    UCHAR                       ledId;
    UCHAR                       ledState;
    UCHAR                       reserved_1;
    UCHAR                       reserved_2;
}
IPMI_REQUEST_SET_BEACHCOMBER_LED_STATE, *PIPMI_REQUEST_SET_BEACHCOMBER_LED_STATE;

//++
// Name:
//      IPMI_RESPONSE_GET_BEACHCOMBER_LED_STATE
//
// Description:
//      Data portion of IPMI response for 'Get Beachcomber LED State' command.
//--
typedef struct _IPMI_RESPONSE_GET_BEACHCOMBER_LED_STATE
{
    UCHAR                       ledId;
    UCHAR                       ledState;
    UCHAR                       reserved_1;
    UCHAR                       reserved_2;
}
IPMI_RESPONSE_GET_BEACHCOMBER_LED_STATE, *PIPMI_RESPONSE_GET_BEACHCOMBER_LED_STATE;


//++
// Name:
//      IPMI_REQUEST_P_SET_LED_BLINK_RATE
//
// Description:
//      'Set LED Blink Rate' command data buffer for Phobos BMC.
//--
typedef struct _IPMI_REQUEST_P_SET_LED_BLINK_RATE
{
    UCHAR           fruId;
    UCHAR           ledId;
    UCHAR           colorWhenOn;
    UCHAR           colorWhenOff;
    UCHAR           blinkRate;
}
IPMI_REQUEST_P_SET_LED_BLINK_RATE, *PIPMI_REQUEST_P_SET_LED_BLINK_RATE;

//++
// Name:
//      IPMI_REQUEST_P_GET_LED_BLINK_RATE
//
// Description:
//      'Get LED Blink Rate' command data buffer for Phobos BMC.
//--
typedef struct _IPMI_REQUEST_P_GET_LED_BLINK_RATE
{
    UCHAR           fruId;
    UCHAR           ledId;
}
IPMI_REQUEST_P_GET_LED_BLINK_RATE, *PIPMI_REQUEST_P_GET_LED_BLINK_RATE;


//++
// Name:
//      IPMI_RESPONSE_P_GET_LED_BLINK_RATE
//
// Description:
//      Data portion of IPMI response for 'Get LED Blink Rate' command for Phobos BMC.
//--
typedef struct _IPMI_RESPONSE_P_GET_LED_BLINK_RATE
{
    UCHAR           colorWhenOn;
    UCHAR           colorWhenOff;
    UCHAR           blinkRate;
    UCHAR           currentState;
}
IPMI_RESPONSE_P_GET_LED_BLINK_RATE, *PIPMI_RESPONSE_P_GET_LED_BLINK_RATE;


//++
// Name:
//      IPMI_SSP_PWR_RST_REQ
//
// Description:
//      SSP power reset request data-field values
//--
typedef enum _IPMI_SSP_PWR_RST_REQ
{
    IPMI_SSP_PWR_RST_REQ_NO_ACTION                = 0x00,
    IPMI_SSP_PWR_RST_REQ_COLD_RESET               = 0x05,
    IPMI_SSP_PWR_RST_REQ_WARM_RESET               = 0x06,
    IPMI_SSP_PWR_RST_REQ_HOLD_IN_COLD_RESET       = 0x07,
    IPMI_SSP_PWR_RST_REQ_REL_FROM_COLD_RESET      = 0x08,
    IPMI_SSP_PWR_RST_REQ_PWR_DOWN                 = 0x0B,
    IPMI_SSP_PWR_RST_REQ_PWR_UP                   = 0x0C
} IPMI_SSP_PWR_RST_REQ;

//++
// Name:
//      IPMI_SSP_PWR_RESET_STATE
//
// Description:
//      'SSP Power Reset Request' command data buffer.
//--
typedef struct _IPMI_SSP_PWR_RESET_STATE
{
    UCHAR        state;
}
IPMI_SSP_PWR_RESET_STATE, *PIPMI_SSP_PWR_RESET_STATE;

//++
// Name:
//      IPMI_FRU_IDS
//
// Description:
//      FRU ID values
//--
typedef enum _IPMI_FRU_IDS
{
    IPMI_VEEPROM                                  = 0x00,
    IPMI_MIDPLANE                                 = 0x01,
    IPMI_LOCAL_SP                                 = 0x02,
    IPMI_LOC_MGMT                                 = 0x03,
    IPMI_REMOTE_SP                                = 0x04,
    IPMI_REMOTE_MGMT                              = 0x05,
    IPMI_PS_0                                     = 0x06,
    IPMI_PS_1                                     = 0x08
} IPMI_FRU_IDS;

typedef enum _BEACHCOMBER_FRU_IDS
{
    BEACHCOMBER_LOCAL_PS                          = 0x03,
    BEACHCOMBER_REMOTE_PS                         = 0x05,
    BEACHCOMBER_BATTERY                           = 0x06,
    BEACHCOMBER_IOSLOT                            = 0x07,
    BEACHCOMBER_LOCAL_CM_0                        = 0x08,
    BEACHCOMBER_LOCAL_CM_1                        = 0x09,
    BEACHCOMBER_LOCAL_CM_2                        = 0x0A,
    BEACHCOMBER_REMOTE_CM_0                       = 0x0B,
    BEACHCOMBER_REMOTE_CM_1                       = 0x0C,
    BEACHCOMBER_REMOTE_CM_2                       = 0x0D
} BEACHCOMBER_FRU_IDS;

//++
// Name:
//      JETFIRE_FRU_IDS
//
// Description:
//      Machine specific FRU ID values for jetfire
//--
typedef enum _JETFIRE_FRU_IDS
{
    JETFIRE_BATTERY_LOCAL                         = 0x07,
    JETFIRE_BATTERY_REMOTE                        = 0x09,
    JETFIRE_BEM                                   = 0x0A,
    JETFIRE_IOSLOT_0                              = 0x0B,
    JETFIRE_IOSLOT_1                              = 0x0C,
    JETFIRE_IOSLOT_2                              = 0x0D,
    JETFIRE_IOSLOT_3                              = 0x0E,
    JETFIRE_IOSLOT_4                              = 0x0F
}JETFIRE_FRU_IDS;

//++
// Name:
//      MEGATRON_FRU_IDS
//
// Description:
//      Machine specific FRU ID values for megatron
//--
typedef enum _MEGATRON_FRU_IDS
{
    MEGATRON_IOSLOT_0                            = 0x09,
    MEGATRON_IOSLOT_1                            = 0x0a,
    MEGATRON_IOSLOT_2                            = 0x0b,
    MEGATRON_IOSLOT_3                            = 0x0c,
    MEGATRON_IOSLOT_4                            = 0x0d,
    MEGATRON_IOSLOT_5                            = 0x0e,
    MEGATRON_IOSLOT_6                            = 0x0f,
    MEGATRON_IOSLOT_7                            = 0x10,
    MEGATRON_IOSLOT_8                            = 0x11,
    MEGATRON_IOSLOT_9                            = 0x12,
    MEGATRON_IOSLOT_10                           = 0x13
}MEGATRON_FRU_IDS;


//++
// Name:
//      I2C_DEVICE_STATUS
//
// Description:
//      I2C Device Status
//--
typedef enum _I2C_DEVICE_STATUS
{
    I2C_DEVICE_NOT_PRESENT                      = 0x00,
    I2C_DEVICE_PRESENT_NOT_ACCESSIBLE           = 0x02,
    I2C_DEVICE_ACCESSIBLE                       = 0x03
}I2C_DEVICE_STATUS;

//++
// Name:
//      I2C_DEVICE_INDEX
// Description:
//      I2C Device Index used by Get I2C Device Status Command
//--
typedef enum _I2C_DEVICE_INDEX
{
    DEV_MSHD_P2_SPC_A0                          = 0x1D,
}I2C_DEVICE_INDEX;

//++
// Name:
//      IPMI_WRITE_FRU_DATA
//
// Description:
//      'Write FRU Data' command data buffer.
//--
typedef struct _IPMI_WRITE_FRU_DATA
{
    UCHAR                      fruId;
    UCHAR                      lsbOffset;
    UCHAR                      msbOffset;
    CHAR                       data[IPMI_DATA_LENGTH_MAX-5]; //This "-5" is due to above 3 fields + 2 (to be on a safer side)
}IPMI_WRITE_FRU_DATA;

//++
// Name:
//      IPMI_RESPONSE_GET_BMC_NETWORK_ADDRESS
//
// Description:
//      'Get LAN Configuration Parameters' command data buffer for getting IP address.
//--
typedef struct _IPMI_RESPONSE_GET_BMC_NETWORK_ADDRESS
{
    UCHAR                      subField;
    UCHAR                      addressFields[4];
}IPMI_RESPONSE_GET_BMC_NETWORK_ADDRESS, *PIPMI_RESPONSE_GET_BMC_NETWORK_ADDRESS;

//++
// Name:
//      IPMI_RESPONSE_GET_BMC_NETWORK_MODE
//
// Description:
//      'Get LAN Configuration Parameters' command data buffer for getting network mode.
//--
typedef struct _IPMI_RESPONSE_GET_BMC_NETWORK_MODE
{
    UCHAR                      subField;
    UCHAR                      networkMode;
}IPMI_RESPONSE_GET_BMC_NETWORK_MODE, *PIPMI_RESPONSE_GET_BMC_NETWORK_MODE;

//++
// Name:
//      IPMI_RESPONSE_GET_BMC_NETWORK_PORT
//
// Description:
//      'Get BMC Primary LAN Interface' command data buffer for getting network port.
//--
typedef struct _IPMI_RESPONSE_GET_BMC_NETWORK_PORT
{
    UCHAR                      networkPort;
}IPMI_RESPONSE_GET_BMC_NETWORK_PORT, *PIPMI_RESPONSE_GET_BMC_NETWORK_PORT;

//++
// Name:
//      IPMI_RESPONSE_SET_BMC_NETWORK_PORT
//
// Description:
//      'Set BMC Primary LAN Interface' command data buffer for setting network port.
//--
typedef struct _IPMI_RESPONSE_SET_BMC_NETWORK_PORT
{
    UCHAR                      networkPort;
}IPMI_RESPONSE_SET_BMC_NETWORK_PORT, *PIPMI_RESPONSE_SET_BMC_NETWORK_PORT;

//++
// Name:
//      IPMI_REQUEST_SET_BMC_NETWORK_ADDRESS
//
// Description:
//      'Set LAN Configuration Parameters' command data buffer for setting IP address.
//--
typedef struct _IPMI_REQUEST_SET_BMC_NETWORK_ADDRESS
{
    UCHAR                      subFields[2];
    UCHAR                      addressFields[4];
}IPMI_REQUEST_SET_BMC_NETWORK_ADDRESS, *PIPMI_REQUEST_SET_BMC_NETWORK_ADDRESS;

//++
// Name:
//      IPMI_REQUEST_SET_BMC_NETWORK_MODE
//
// Description:
//      'Set LAN Configuration Parameters' command data buffer for setting network mode.
//--
typedef struct _IPMI_REQUEST_SET_BMC_NETWORK_MODE
{
    UCHAR                      subFields[2];
    UCHAR                      networkMode;
}IPMI_REQUEST_SET_BMC_NETWORK_MODE, *PIPMI_REQUEST_SET_BMC_NETWORK_MODE;

//++
// Name:
//      IPMI_OEM_SSP_RESET_TYPE
//
// Description:
//      Reset types for the OEM SSP Reset command.
//--
typedef enum _IPMI_OEM_SSP_RESET_TYPE
{
    IPMI_OEM_SSP_WARM_RESET_SSP       = 0x00,
    IPMI_OEM_SSP_FWUPDT_RESET_SSP     = 0x01,
    IPMI_OEM_SSP_COLD_RESET_BMC_SSP   = 0x02,
    IPMI_OEM_SSP_COLD_RESET_BMC       = 0x03,
    IPMI_OEM_SSP_RESET_TYPE_INVALID   = 0xFF
} IPMI_OEM_SSP_RESET_TYPE;

#define IPMI_OEM_SSP_RESET_REQUEST_RESERVED_BYTE     0x00
//++
// Name:
//      IPMI_REQUEST_OEM_SSP_RESET
//
// Description:
//      Data portion of IPMI request for 'OEM SSP Reset' command.
//--
typedef struct _IPMI_REQUEST_OEM_SSP_RESET
{
    UCHAR    resetType;
    UCHAR    reserved;
}IPMI_REQUEST_OEM_SSP_RESET, *PIPMI_REQUEST_OEM_SSP_RESET;

//++
// Name:
//      IPMI_P_OEM_RESET_TYPE
//
// Description:
//      Reset types for the OEM Reset command on Phobos BMC.
//--
typedef enum _IPMI_P_OEM_RESET_TYPE
{
    IPMI_P_OEM_NO_RESET             = 0x00,
    IPMI_P_OEM_COLD_RESET           = 0x01,
    IPMI_P_OEM_WARM_RESET           = 0x02,
    IPMI_P_OEM_PERSIST_RESET        = 0x03, // Host Only
    IPMI_P_OEM_FWUPDT_RESET         = 0x04, // BMC & SSP, which will automatically reset Host as well
    IPMI_P_OEM_HOLD_IN_RESET        = 0x05, // Host Only
    IPMI_P_OEM_RELEASE_FROM_RESET   = 0x06, // Host Only
    IPMI_P_OEM_RESET_INVALID        = 0xFF
} IPMI_P_OEM_RESET_TYPE;


#define IPMI_OEM_SSP_RESET_REQUEST_RESERVED_BYTE     0x00
//++
// Name:
//      IPMI_REQUEST_P_OEM_SSP_RESET
//
// Description:
//      Data portion of IPMI request for 'OEM SSP Reset' command
//       on Phobos BMC.
//--
typedef struct _IPMI_REQUEST_P_OEM_RESET
{
    UCHAR    bmcResetType;
    UCHAR    hostResetType;
}IPMI_REQUEST_P_OEM_RESET, *PIPMI_REQUEST_P_OEM_RESET;

//++
// Name:
//      IPMI_FAN_ID
//
// Description:
//      FAN IDs
//--
typedef enum _IPMI_FAN_ID
{
   IPMI_FAN_ID_0          = 0x00,
   IPMI_FAN_ID_1          = 0x01,
   IPMI_FAN_ID_2          = 0x02,
   IPMI_FAN_ID_3          = 0x03,
   IPMI_FAN_ID_4          = 0x04,
   IPMI_FAN_ID_5          = 0x05,
   IPMI_FAN_ID_6          = 0x06,
   IPMI_FAN_ID_7          = 0x07,
   IPMI_FAN_ID_8          = 0x08,
   IPMI_FAN_ID_9          = 0x09,
   IPMI_FAN_ID_10         = 0x0A,
   IPMI_FAN_ID_11         = 0x0B,
   IPMI_FAN_ID_INVALID    = 0xFF
} IPMI_FAN_ID;

//++
// Name:
//      IPMI_REQUEST_SET_FAN_PWM
//
// Description:
//      Data portion of IPMI request for 'Set Fan Duty i.e. PWM' command.
//--
typedef struct _IPMI_REQUEST_SET_FAN_PWM
{
    UCHAR                   pwmId;
    UCHAR                   pwmDuty;
}IPMI_REQUEST_SET_FAN_PWM, *PIPMI_REQUEST_SET_FAN_PWM;

//++
// Name:
//      IPMI_RESPONSE_GET_FAN_PWM
//
// Description:
//      Data portion of IPMI response for 'Get Fan Duty i.e. PWM' command.
//--
typedef struct _IPMI_RESPONSE_GET_FAN_PWM
{
    UCHAR                   pwmDuty;
}IPMI_RESPONSE_GET_FAN_PWM, *PIPMI_RESPONSE_GET_FAN_PWM;

// Name:
//      IPMI_REQUEST_SET_FAN_RPM
//
// Description:
//      Data portion of IPMI request for 'Set Fan RPM' command.
//--
typedef struct _IPMI_REQUEST_SET_FAN_RPM
{
    UINT_8E                 fanId;
    UINT_8E                 rpmHigh;
    UINT_8E                 rpmLow;
}IPMI_REQUEST_SET_FAN_RPM, *PIPMI_REQUEST_SET_FAN_RPM;

//++
// Name:
//      IPMI_RESPONSE_GET_FAN_RPM
//
// Description:
//      Data portion of IPMI response for 'Get Fan RPM' command.
//--
typedef struct _IPMI_RESPONSE_GET_FAN_RPM
{
    UINT_8E                 rpmHigh;
    UINT_8E                 rpmLow;
}IPMI_RESPONSE_GET_FAN_RPM, *PIPMI_RESPONSE_GET_FAN_RPM;

//++
// Name:
//      IPMI_RESPONSE_P_GET_FAN_RPM
//
// Description:
//      Data portion of IPMI response for 'Get Fan RPM' command for Phobos BMC
//--
typedef struct _IPMI_RESPONSE_P_GET_FAN_RPM
{
    UINT_32E                targetRPM;
    UINT_32E                measuredRPM;
}IPMI_RESPONSE_P_GET_FAN_RPM, *PIPMI_RESPONSE_P_GET_FAN_RPM;

//++
// Name:
//      IPMI_SERIAL_MUX_SUBCOMMAND
//
// Description:
//      GET/SET SERIAL MUX SUBCOMMANDs
//--
typedef enum _IPMI_SERIAL_MUX_SUBCOMMAND
{
   IPMI_GET_SERIAL_MUX_STATUS          = 0x00,
   IPMI_SET_SERIAL_MUX_STATUS          = 0x01,
   IPMI_SERIAL_MUX_SUBCOMMAND_INVALID  = 0xFF
} IPMI_SERIAL_MUX_SUBCOMMAND;

//++
// Name:
//      IPMI_SERIAL_MUX_ID
//
// Description:
//      SERIAL MUX IDs
//--
typedef enum _IPMI_SERIAL_MUX_ID
{
   IPMI_MUX_DB9_1          = 0x00,
   IPMI_MUX_DB9_2          = 0x01,
   IPMI_MUX_DB9_INVALID    = 0xFF
} IPMI_SERIAL_MUX_ID;

//++
// Name:
//      IPMI_SERIAL_MUX_MODE
//
// Description:
//      IPMI SERIAL MUX MODEs
//--
typedef enum _IPMI_SERIAL_MUX_MODE
{
    IPMI_MUX_MODE_PHYSICAL_DB9      = 0x00,
    IPMI_MUX_MODE_DEBUG             = 0x01, // also applies to virtual UART for SOL
    IPMI_MUX_MODE_INVALID           = 0xFF
} IPMI_SERIAL_MUX_MODE, *PIPMI_SERIAL_MUX_MODE;

//++
// Name:
//      IPMI_SERIAL_MUX
//
// Description:
//      Get/Set Serial port Mux.
//--
typedef struct _IPMI_SERIAL_MUX
{
    UINT_8E                 commandCode;
    UINT_8E                 uartChannel;
    UINT_8E                 muxMode;
}
IPMI_SERIAL_MUX, *PIPMI_SERIAL_MUX;

//++
// Name:
//      IPMI_BEM_SXP_CONSOLE_TOGGLE_SETTING
//
// Description:
//      IPMI BEM SXP CONSOLE TOGGLE SETTINGs
//--
typedef enum _IPMI_BEM_SXP_CONSOLE_TOGGLE_SETTING
{
   IPMI_BEM_SXP_CONSOLE_TOGGLE_DB9_2_SXP     = 0x00,
   IPMI_BEM_SXP_CONSOLE_TOGGLE_DB9_2_BMC     = 0x01,
} IPMI_BEM_SXP_CONSOLE_TOGGLE_SETTING, *PIPMI_BEM_SXP_CONSOLE_TOGGLE_SETTING;

//++
// Name:
//      IPMI_BEM_SXP_CONSOLE_TOGGLE_PERSISTENCE
//
// Description:
//      IPMI BEM SXP CONSOLE TOGGLE PERSISTENCE
//--
typedef enum _IPMI_BEM_SXP_CONSOLE_TOGGLE_PERSISTENCE
{
   IPMI_BEM_SXP_CONSOLE_TOGGLE_NONPERSISTENT = 0x00,
   IPMI_BEM_SXP_CONSOLE_TOGGLE_PERSISTENT    = 0x01,
} IPMI_BEM_SXP_CONSOLE_TOGGLE_PERSISTENCE, *PIPMI_BEM_SXP_CONSOLE_TOGGLE_PERSISTENCE;

//++
// Name:
//      IPMI_TOGGLE_SXP_MUX
//
// Description:
//      Toggle SXP console port Mux.
//--
typedef struct _IPMI_TOGGLE_SXP_MUX
{
    UINT_8E                 setting;
    UINT_8E                 persistence;
}
IPMI_TOGGLE_SXP_MUX, *PIPMI_TOGGLE_SXP_MUX;

//++
// Name:
//      IPMI_UART_ID
//
// Description:
//      UART Port IDs
//--
typedef enum _IPMI_UART_ID
{
   IPMI_UART_PORT_1         = 0x01,
   IPMI_UART_PORT_2         = 0x02
} IPMI_UART_ID;

//++
// Name:
//      IPMI_UART_MUX_MODE
//
// Description:
//      IPMI_UART_MUX_MODEs
//--
typedef enum _IPMI_UART_MUX_MODE
{
    IPMI_UART_MUX_MODE_HOST_COM1        = 0x00,
    IPMI_UART_MUX_MODE_SOL              = 0x01,
    IPMI_UART_MUX_MODE_HOST_COM2        = 0x02,
    IPMI_UART_MUX_MODE_BMC_CONSOLE      = 0x03,
    IPMI_UART_MUX_MODE_SXP_CONSOLE      = 0x04,
    IPMI_UART_MUX_MODE_INVALID          = 0xFF
} IPMI_UART_MUX_MODE, *PIPMI_UART_MUX_MODE;

//++
// Name:
//      IPMI_REQUEST_SET_UART_MUX
//
// Description:
//      Data portion of IPMI request for 'Set UART Mux' command.
//--
typedef struct _IPMI_REQUEST_SET_UART_MUX
{
    UINT_8E                 uartChannel;
    UINT_8E                 muxMode;
}
IPMI_REQUEST_SET_UART_MUX, *PIPMI_REQUEST_SET_UART_MUX;

//++
// Name:
//      IPMI_RESPONSE_GET_UART_MUX
//
// Description:
//      Data portion of IPMI response for 'Get UART Mux' command.
//--
typedef struct _IPMI_RESPONSE_GET_UART_MUX
{
    UINT_8E                 muxMode;
}
IPMI_RESPONSE_GET_UART_MUX, *PIPMI_RESPONSE_GET_UART_MUX;

//++
// Name:
//      IPMI_SPS_PRESENCE
//
// Description:
//      IPMI SPS Presence. Indicating whether the SSP believes an SPS is connected or not.
//
//-
typedef enum _IPMI_SPS_PRESENCE
{
    IPMI_SPS_DISCONNECTED       = 0,
    IPMI_SPS_CONNECTED          = 1,
    IPMI_SPS_UNKNOWN            = 0xFF
}
IPMI_SPS_PRESENCE, *PIPMI_SPS_PRESENCE;

//++
// Name:
//      IPMI_RESPONSE_GET_SPS_PRESENT
//
// Description:
//      Data portion of IPMI response for 'Get SPS status' command.
//--
typedef struct _IPMI_RESPONSE_GET_SPS_PRESENT
{
    UCHAR              spsPresent;
}
IPMI_RESPONSE_GET_SPS_PRESENT, *PIPMI_RESPONSE_GET_SPS_PRESENT;

//++
// Name:
//      IPMI_REQUEST_SET_SPS_PRESENT
//
// Description:
//      'Set SPS Present' command data buffer.
//--
typedef struct _IPMI_REQUEST_SET_SPS_PRESENT
{
    UCHAR              spsPresent;
}
IPMI_REQUEST_SET_SPS_PRESENT, *PIPMI_REQUEST_SET_SPS_PRESENT;

//++
// Name:
//      IPMI_SLIC_ID
//
// Description:
//      SLIC IDs
//--
typedef enum _IPMI_SLIC_ID
{
   IPMI_SLIC_ID_0                  = 0x00,
   IPMI_SLIC_ID_1                  = 0x01,
   IPMI_SLIC_ID_2                  = 0x02,
   IPMI_SLIC_ID_3                  = 0x03,
   IPMI_SLIC_ID_4                  = 0x04,
   IPMI_SLIC_ID_5                  = 0x05,
   IPMI_SLIC_ID_6                  = 0x06,
   IPMI_SLIC_ID_7                  = 0x07,
   IPMI_SLIC_ID_8                  = 0x08,
   IPMI_SLIC_ID_9                  = 0x09,
   IPMI_SLIC_ID_10                 = 0x0A,
   IPMI_SLIC_ID_INVALID            = 0xFF
} IPMI_SLIC_ID;

//++
// Slic Control command "state" values
//--
//
typedef enum _IPMI_SLIC_CONTROL_STATE
{
    IPMI_SLIC_CONTROL_STATE_DISABLE_POWER                  = 0x00,
    IPMI_SLIC_CONTROL_STATE_ENABLE_POWER                   = 0x01,
    IPMI_SLIC_CONTROL_STATE_HOLD_RST                       = 0x02,
    IPMI_SLIC_CONTROL_STATE_REL_RST                        = 0x03,
    IPMI_SLIC_CONTROL_STATE_PWRCYCLE_EHORNET               = 0x04,
    IPMI_SLIC_CONTROL_STATE_DISABLE_PUPENABLE              = 0x05,
    IPMI_SLIC_CONTROL_STATE_ENABLE_PUPENABLE               = 0x06
} IPMI_SLIC_CONTROL_STATE;

//++
// Name:
//      IPMI_REQUEST_SLIC_CONTROL
//
// Description:
//      Data portion of IPMI request for 'SLIC Control' command.
//--
typedef struct _IPMI_REQUEST_SLIC_CONTROL
{
    UCHAR                   slicID;
    UCHAR                   state;
}
IPMI_REQUEST_SLIC_CONTROL;

//++
// Slic Control command register definitions
//--
typedef struct _IPMI_P_SLIC_CONTROL_REG
{
    UINT_16E    power_enable:1,
                power_cycle:1,
                reset:1,
                powerup_enable:1,
                sp_reset_enable:1,
                diplex_enable_33mhz:1,
                reserved:10;
} IPMI_P_SLIC_CONTROL_REG;

typedef union _IPMI_P_SLIC_CONTROL_RAW_INFO
{
    UINT_16E                    StatusControl;
    IPMI_P_SLIC_CONTROL_REG     fields;
} IPMI_P_SLIC_CONTROL_RAW_INFO;

//++
// Name:
//      IPMI_REQUEST_P_SLIC_CONTROL
//
// Description:
//      Data portion of IPMI request for 'SLIC Control' command
//       for Phobos BMC.
//--
typedef struct _IPMI_REQUEST_P_SLIC_CONTROL
{
    UCHAR                           fruID;
    IPMI_P_SLIC_CONTROL_RAW_INFO    writeMask;
    IPMI_P_SLIC_CONTROL_RAW_INFO    slicControl;
}
IPMI_REQUEST_P_SLIC_CONTROL;

//++
// Name:
//      IPMI_FAN_AUTO_REARM_AFTER_RESET
//
// Description:
//      IPMI Fan Auto Rearm after reset Configuration. Indicating whether Auto Rearm after reset is enabled or disabled.
//
//-
typedef enum _IPMI_FAN_AUTO_REARM_AFTER_RESET
{
    IPMI_FAN_AUTO_REARM_AFTER_RESET_DISABLED           = 0x00,
    IPMI_FAN_AUTO_REARM_AFTER_RESET_ENABLED            = 0x01,
    IPMI_FAN_AUTO_REARM_AFTER_RESET_INVALID            = 0xFF
}
IPMI_FAN_AUTO_REARM_AFTER_RESET, *PIPMI_FAN_AUTO_REARM_AFTER_RESET;

//++
// Name:
//      IPMI_FAN_AUTO_ADAPTIVE_COOLING
//
// Description:
//      IPMI Fan Auto Adaptive Cooling Configuration. Indicating whether Auto Adaptive Cooling is enabled or disabled.
//
//-
typedef enum _IPMI_FAN_AUTO_ADAPTIVE_COOLING
{
    IPMI_FAN_AUTO_ADAPTIVE_COOLING_DISABLED           = 0x00,
    IPMI_FAN_AUTO_ADAPTIVE_COOLING_ENABLED            = 0x01,
    IPMI_FAN_AUTO_ADAPTIVE_COOLING_INVALID            = 0xFF
}
IPMI_FAN_AUTO_ADAPTIVE_COOLING, *PIPMI_FAN_AUTO_ADAPTIVE_COOLING;


//++
// Name:
//      IPMI_REQUEST_SET_ADAPTIVE_COOLING_CONFIG
//
// Description:
//      Data portion of IPMI response for 'Set Adaptive Cooling Configuration' command.
//--
typedef struct _IPMI_REQUEST_SET_ADAPTIVE_COOLING_CONFIG
{
    UCHAR                                    autoRearmAfterReset;
    UCHAR                                    autoAdaptiveCooling;
}
IPMI_REQUEST_SET_ADAPTIVE_COOLING_CONFIG, *PIPMI_REQUEST_SET_ADAPTIVE_COOLING_CONFIG;


//++
// Name:
//      IPMI_RESPONSE_GET_ADAPTIVE_COOLING_CONFIG
//
// Description:
//      Data portion of IPMI response for 'Get Adaptive Cooling Configuration' command.
//--
typedef struct _IPMI_RESPONSE_GET_ADAPTIVE_COOLING_CONFIG
{
    UCHAR                                    autoRearmAfterReset;
    UCHAR                                    autoAdaptiveCooling;
}
IPMI_RESPONSE_GET_ADAPTIVE_COOLING_CONFIG, *PIPMI_RESPONSE_GET_ADAPTIVE_COOLING_CONFIG;



//++
// Name:
//      IPMI_REQUEST_SET_AUTOMATIC_FAN_CTRL
//
// Description:
//      Data portion of IPMI response for 'Set Adaptive Cooling Configuration' command.
//--
typedef struct _IPMI_REQUEST_SET_AUTOMATIC_FAN_CTRL
{
    UCHAR                                    automaticFanCtrl;
}
IPMI_REQUEST_SET_AUTOMATIC_FAN_CTRL, *PIPMI_REQUEST_SET_AUTOMATIC_FAN_CTRL;


//++
// Name:
//      IPMI_RESPONSE_GET_AUTOMATIC_FAN_CTRL
//
// Description:
//      Data portion of IPMI response for 'Get Adaptive Cooling Configuration' command.
//--
typedef struct _IPMI_RESPONSE_GET_AUTOMATIC_FAN_CTRL
{
    UCHAR                                    automaticFanCtrl;
}
IPMI_RESPONSE_GET_AUTOMATIC_FAN_CTRL, *PIPMI_RESPONSE_GET_AUTOMATIC_FAN_CTRL;



//++
// VEEPROM related data structures start.
//--

typedef struct _VEEPROM_SLICFLAGS_READING_REG
{
    UINT_8E     persistent_power_state:1,
                reserved_0:7;
} VEEPROM_SLICFLAGS_READING_REG;

typedef union _VEEPROM_SLICFLAGS_READING_RAW_INFO
{
    UINT_8E                             StatusControl;
    VEEPROM_SLICFLAGS_READING_REG             fields;
} VEEPROM_SLICFLAGS_READING_RAW_INFO;

//++
// Name:
//      IPMI_RESPONSE_VEEPROM_SLICFLAG_BYTES
//
// Description:
//      Data portion of IPMI response for 'Read SLIC scratchpad bytes'.
//--
typedef struct _IPMI_RESPONSE_VEEPROM_SLICFLAG_BYTES
{
    UCHAR                                    bytesRead; //DONT take this line out when switching to IPMI_RESPONSE_READ_SOFT_FLAGS, as the for-loops will always line up fine for the 0th iteration.
    // First 11 bytes correspond to the 11(max) SLICs
    VEEPROM_SLICFLAGS_READING_RAW_INFO       slicFlags[11];
    // Rest 12 bytes are currently undefined
    UCHAR                                    reserved[12];
}
IPMI_RESPONSE_READ_SLICFLAG_BYTES, *PIPMI_RESPONSE_READ_SLICFLAG_BYTES;

typedef struct _VEEPROM_P_SLICFLAGS_READING_REG
{
    UINT_16E    persistent_power_state_slic_0:1,
                persistent_power_state_slic_1:1,
                persistent_power_state_slic_2:1,
                persistent_power_state_slic_3:1,
                persistent_power_state_slic_4:1,
                persistent_power_state_slic_5:1,
                persistent_power_state_slic_6:1,
                persistent_power_state_slic_7:1,
                persistent_power_state_slic_8:1,
                persistent_power_state_slic_9:1,
                persistent_power_state_slic_10:1,
                persistent_power_state_slic_11:1,
                persistent_power_state_slic_12:1,
                persistent_power_state_slic_13:1,
                persistent_power_state_slic_14:1,
                persistent_power_state_slic_15:1;
} VEEPROM_P_SLICFLAGS_READING_REG;

typedef union _VEEPROM_P_SLICFLAGS_READING_RAW_INFO
{
    UINT_16E                            StatusControl;
    VEEPROM_P_SLICFLAGS_READING_REG     fields;
} VEEPROM_P_SLICFLAGS_READING_RAW_INFO;

//++
// Name:
//      IPMI_RESPONSE_P_VEEPROM_SLICFLAG_BYTES
//
// Description:
//      Data portion of IPMI response for 'Read SLIC scratchpad bytes'
//       for Phobos BMC.
//--
typedef struct _IPMI_RESPONSE_P_VEEPROM_SLICFLAG_BYTES
{
    UCHAR                                   bytesRead; //DONT take this line out when switching to IPMI_RESPONSE_READ_SOFT_FLAGS, as the for-loops will always line up fine for the 0th iteration.
    VEEPROM_P_SLICFLAGS_READING_RAW_INFO    slicFlags;
    UCHAR                                   reserved[6];
}
IPMI_RESPONSE_P_READ_SLICFLAG_BYTES, *PIPMI_RESPONSE_P_READ_SLICFLAG_BYTES;

typedef struct _VEEPROM_SP_FLAG_WAIT_FOR_BOOT_REG
{
    UINT_8E     hold_in_post:1,
                hold_in_bios:1,
                reserved:6;
} VEEPROM_SP_FLAG_BOOT_OPTIONS_REG;

typedef union _VEEPROM_SP_FLAG_WAIT_FOR_BOOT_RAW_INFO
{
    UINT_8E                             StatusControl;
    VEEPROM_SP_FLAG_BOOT_OPTIONS_REG    fields;
} VEEPROM_SP_FLAG_WAIT_FOR_BOOT_RAW_INFO;

//++
// Name:
//      IPMI_RESPONSE_READ_SPFLAG_BYTES
//
// Description:
//      Data portion of IPMI response for 'Read SP Flag bytes' command.
//--
typedef struct _IPMI_RESPONSE_READ_SPFLAG_BYTES
{
    UCHAR                                    bytesRead; //take this line out when switching to IPMI_RESPONSE_READ_SOFT_FLAGS as for-loop-offsets will not line up with this.
    UCHAR                                    mfb_short_press;
    UCHAR                                    mfb_long_press;
    UCHAR                                    mfb_pressed;
    UCHAR                                    mfb_stuck;
    VEEPROM_SP_FLAG_WAIT_FOR_BOOT_RAW_INFO   wait_for_boot;
    UCHAR                                    boot_flag;
    UCHAR                                    reserved[17];
}
IPMI_RESPONSE_READ_SPFLAG_BYTES, *PIPMI_RESPONSE_READ_SPFLAG_BYTES;

typedef struct _IPMI_RESPONSE_READ_SOFT_FLAGS //0000h to 008Ah = 138bytes; 138/23=6cycles; hence total length=138+6(for bytesRead) = 144
{
    IPMI_RESPONSE_READ_SLICFLAG_BYTES        slic_flag_bytes; //bytesRead + first 23bytes (0000 to 0017h)
    UCHAR                                    reserved_0[109]; //bytesRead + 23bytes + bytesRead + 23bytes + bytesRead + 23bytes + bytesRead + 23bytes + bytesRead + 12bytes (0018 to 007Fh)
    IPMI_RESPONSE_READ_SPFLAG_BYTES          sp_flag_bytes; //6bytes(0080 to 0085)
    UCHAR                                    reserved_1[5]; //5bytes(upto 008A)
}
IPMI_RESPONSE_READ_SOFT_FLAGS, *PIPMI_RESPONSE_READ_SOFT_FLAGS;

//++
// Name:
//      IPMI_RESPONSE_QUICKBOOT_REG1
//
// Description:
//      Data portion of IPMI response for 'Read Quickboot Region 1' command.
//--
#define IPMI_RESPONSE_QUICKBOOT_SIGNATURE     0x42544f50     // objId should equal "BTOP"
typedef struct _IPMI_RESPONSE_QUICKBOOT_REG1
{
    UCHAR                     bytesRead;
    UINT32                    objId;
    UINT_16E                  revision;
    UINT_16E                  bootOptionFlag;
    UINT32                    quickBootStatus;
    UINT32                    fwUpdateLoadStatus;
}
IPMI_RESPONSE_QUICKBOOT_REG1, *PIPMI_RESPONSE_QUICKBOOT_REG1;

//++
// Name:
//      IPMI_RESPONSE_QUICKBOOT_REG2
//
// Description:
//      Data portion of IPMI response for 'Read Quickboot Region 2' command.
//--
typedef struct _IPMI_RESPONSE_QUICKBOOT_REG2
{
    UCHAR                     bytesRead;
    UINT32                    hwPostBasicStatus;
    UINT32                    hwPostEnhancedStatus;
    UINT32                    quickBootType;
    UINT32                    reserved;
}
IPMI_RESPONSE_QUICKBOOT_REG2, *PIPMI_RESPONSE_QUICKBOOT_REG2;

//++
// Name:
//      IPMI_RESPONSE_QUICKBOOT_REG_COMPLETE
//
// Description:
//      Data portion of IPMI response for 'Read Quickboot Region' command.
//      Both of the sections above, combined.
//--
typedef struct _IPMI_RESPONSE_QUICKBOOT_REG_COMPLETE
{
    IPMI_RESPONSE_QUICKBOOT_REG1    reg1;
    IPMI_RESPONSE_QUICKBOOT_REG2    reg2;
}
IPMI_RESPONSE_QUICKBOOT_REG_COMPLETE, *PIPMI_RESPONSE_QUICKBOOT_REG_COMPLETE;

//++
// Name:
//      IPMI_RESPONSE_READ_TCOREG_BYTES
//
// Description:
//      Data portion of IPMI response for 'Read TCO Register bytes' command.
//--
typedef struct _IPMI_RESPONSE_READ_TCOREG_BYTES
{
    UCHAR                                    bytesRead;
    UCHAR                                    capabilities;
    UCHAR                                    power_state;
    UCHAR                                    reserved_0;
    UCHAR                                    watchdog;
    UCHAR                                    events_1;
    UCHAR                                    events_2;
    UCHAR                                    message_1;
    UCHAR                                    message_2;
    UCHAR                                    rtc_reg_tcowdcnt;
    UCHAR                                    rtc_reg[7];
    UCHAR                                    reserved_1[7];
}
IPMI_RESPONSE_READ_TCOREG_BYTES, *PIPMI_RESPONSE_READ_TCOREG_BYTES;

typedef struct _VEEPROM_BIST_BMCFAILURE_REG
{
    UINT_8E     firmware_checksum_failure:1,
                uboot_checksum_failure:1,
                ssp_internal_fail:1,
                reserved_0:1,
                TOD_interrupt_failure:1,
                reserved_1:1,
                fatal_exception:1,
                watchdog_trip:1;
} VEEPROM_BIST_BMCFAILURE_REG;

typedef union _VEEPROM_BIST_BMCFAILURE_RAW_INFO
{
    UINT_8E                             StatusControl;
    VEEPROM_BIST_BMCFAILURE_REG         fields;
} VEEPROM_BIST_BMCFAILURE_RAW_INFO;

typedef struct _VEEPROM_BIST_BMCPERIPHERALS_REG
{
    UINT_8E     timer_fail:1,
                PWM_fail:1,
                SP_temp_sensor_failure:1,
                ambient_air_sensor_failure:1,
                PS_temp_sensor_failure:1,
                PIF_SSP_monitor:1,
                reserved:2;
} VEEPROM_BIST_BMCPERIPHERALS_REG;

typedef union _VEEPROM_BIST_BMCPERIPHERALS_RAW_INFO
{
    UINT_8E                             StatusControl;
    VEEPROM_BIST_BMCPERIPHERALS_REG     fields;
} VEEPROM_BIST_BMCPERIPHERALS_RAW_INFO;

typedef struct _VEEPROM_BIST_BLADEFAILURE_REG
{
    UINT_8E     slotID_failure:1,
                PS_standby_fuse_failure:1,
                reserved:1,
                fan_ctrl_ckt_failure:1,
                master_I2C_failure:1,
                power_good_glitch:1,
                button_stuck:1,
                CMD_failure:1;
} VEEPROM_BIST_BLADEFAILURE_REG;

typedef union _VEEPROM_BIST_BLADEFAILURE_RAW_INFO
{
    UINT_8E                             StatusControl;
    VEEPROM_BIST_BLADEFAILURE_REG       fields;
} VEEPROM_BIST_BLADEFAILURE_RAW_INFO;

typedef struct _VEEPROM_BIST_SUITCASEFAILURE_REG
{
    UINT_8E     fan_pack_failure:1,
                BBU_enable_test_failure:1,
                peer_bad:1,
                reserved:5;
} VEEPROM_BIST_SUITCASEFAILURE_REG;

typedef union _VEEPROM_BIST_SUITCASEFAILURE_RAW_INFO
{
    UINT_8E                             StatusControl;
    VEEPROM_BIST_SUITCASEFAILURE_REG         fields;
} VEEPROM_BIST_SUITCASEFAILURE_RAW_INFO;

//++
// Name:
//      IPMI_RESPONSE_READ_BIST_BYTES
//
// Description:
//      Data portion of IPMI response for 'Read BIST bytes' command.
//--
typedef struct _IPMI_RESPONSE_READ_BIST_BYTES
{
    UCHAR                                    bytesRead;
    VEEPROM_BIST_BMCFAILURE_RAW_INFO         bmc_failure;
    VEEPROM_BIST_BMCPERIPHERALS_RAW_INFO     bmc_peripherals;
    VEEPROM_BIST_BLADEFAILURE_RAW_INFO       blade_failure;
    VEEPROM_BIST_SUITCASEFAILURE_RAW_INFO    suitcase_failure;
    UCHAR                                    reserved[19];
}
IPMI_RESPONSE_READ_BIST_BYTES, *PIPMI_RESPONSE_READ_BIST_BYTES;

//++
// Name:
//      IPMI_RESPONSE_READ_FLTSTS_MIRROR_FLAG
//
// Description:
//      Data portion of IPMI response for 'Read Fault Status mirror flag' command.
//--
typedef struct _IPMI_RESPONSE_READ_FLTSTS_MIRROR_FLAG
{
    UCHAR             bytesRead;
    UCHAR             flagStatus;
}
IPMI_RESPONSE_READ_FLTSTS_MIRROR_FLAG, *PIPMI_RESPONSE_READ_FLTSTS_MIRROR_FLAG;

typedef struct _VEEPROM_FLTSTS_GENERIC_REG
{
    UINT_8E     bit_0:1,
                bit_1:1,
                bit_2:1,
                bit_3:1,
                bit_4:1,
                bit_5:1,
                bit_6:1,
                bit_7:1;
} VEEPROM_FLTSTS_GENERIC_REG;

typedef union _VEEPROM_FLTSTS_GENERIC_RAW_INFO
{
    UINT_8E                             StatusControl;
    VEEPROM_FLTSTS_GENERIC_REG          fields;
} VEEPROM_FLTSTS_GENERIC_RAW_INFO;

//++
// Name:
//      IPMI_RESPONSE_READ_FLTSTS_MEMORY_DRVS_0_55_BYTES
//
// Description:
//      Data portion of IPMI response for 'Read Fault Status, memory/DIMMS bytes + drives bytes for drives 0 to 55' command.
//--
typedef struct _IPMI_RESPONSE_READ_FLTSTS_MEMORY_DRVS_0_55_BYTES
{
    UCHAR                                     bytesRead; //retain this line when switching to IPMI_RESPONSE_READ_ALL_FAULT_STATUS_BYTES
    VEEPROM_FLTSTS_GENERIC_RAW_INFO           dimm_0_7; //Address 0240
    VEEPROM_FLTSTS_GENERIC_RAW_INFO           dimm_8_15; //Address 0241
    VEEPROM_FLTSTS_GENERIC_RAW_INFO           dimm_16_23; //Address 0242
    VEEPROM_FLTSTS_GENERIC_RAW_INFO           dimm_24_31; //Address 0243
    VEEPROM_FLTSTS_GENERIC_RAW_INFO           reserved_dimms[12]; //Address 0244 to 024F
    VEEPROM_FLTSTS_GENERIC_RAW_INFO           drives_0_7; //Address 0250
    VEEPROM_FLTSTS_GENERIC_RAW_INFO           drives_8_15; //Address 0251
    VEEPROM_FLTSTS_GENERIC_RAW_INFO           drives_16_23; //Address 0252
    VEEPROM_FLTSTS_GENERIC_RAW_INFO           drives_24_31; //Address 0253        currently only supports upto drive-bit 25 i.e. total 26 drives, bit 26 to 31 are reserved.
    VEEPROM_FLTSTS_GENERIC_RAW_INFO           reserved_drives_32_55[3]; //Address 0254 to 0256
}
IPMI_RESPONSE_READ_FLTSTS_MEMORY_DRVS_0_55_BYTES, *PIPMI_RESPONSE_READ_FLTSTS_MEMORY_DRVS_0_55_BYTES;

//++
// Name:
//      IPMI_RESPONSE_READ_FLTSTS_DRVS_56_239_BYTES
//
// Description:
//      Data portion of IPMI response for 'Read Fault Status, drives bytes for drives 56 to 239' command.
//--
typedef struct _IPMI_RESPONSE_READ_FLTSTS_DRVS_56_239_BYTES
{
    UCHAR                                     bytesRead; //retain this line when switching to IPMI_RESPONSE_READ_ALL_FAULT_STATUS_BYTES
    VEEPROM_FLTSTS_GENERIC_RAW_INFO           reserved_drives_56_239[23]; //Address 0257 to 026D
}
IPMI_RESPONSE_READ_FLTSTS_DRVS_56_239_BYTES, *PIPMI_RESPONSE_READ_FLTSTS_DRVS_56_239_BYTES;

//++
// Name:
//      IPMI_RESPONSE_READ_FLTSTS_DRVS_240_383_SLICS_BYTES
//
// Description:
//      Data portion of IPMI response for 'Read Fault Status, drives bytes for drives 240 to 383 + SLICS(5bytes)' command.
//--
typedef struct _IPMI_RESPONSE_READ_FLTSTS_DRVS_240_383_SLICS_BYTES
{
    UCHAR                                     bytesRead; //retain this line when switching to IPMI_RESPONSE_READ_ALL_FAULT_STATUS_BYTES
    VEEPROM_FLTSTS_GENERIC_RAW_INFO           reserved_drives_240_383[18]; //Address 026E to 027F
    VEEPROM_FLTSTS_GENERIC_RAW_INFO           slics_0_7; //Address 0280
    VEEPROM_FLTSTS_GENERIC_RAW_INFO           slics_8_15; //Address 0281          (this really needs to go till 12 slics)
    VEEPROM_FLTSTS_GENERIC_RAW_INFO           reserved_slics_16_39[3]; //Address 0282 to 0284
}
IPMI_RESPONSE_READ_FLTSTS_DRVS_240_383_SLICS_BYTES, *PIPMI_RESPONSE_READ_FLTSTS_DRVS_240_383_SLICS_BYTES;

typedef struct _VEEPROM_FLTSTS_POWER_1_REG
{
    UINT_8E     ps_0:1,
                ps_1:1,
                ps_2:1,
                ps_3:1,
                reserved:4;
} VEEPROM_FLTSTS_POWER_1_REG;

typedef union _VEEPROM_FLTSTS_POWER_1_RAW_INFO
{
    UINT_8E                             StatusControl;
    VEEPROM_FLTSTS_POWER_1_REG          fields;
} VEEPROM_FLTSTS_POWER_1_RAW_INFO;

typedef struct _VEEPROM_FLTSTS_POWER_2_REG
{
    UINT_8E     bbu_0:1,
                bbu_1:1,
                bbu_2:1,
                bbu_3:1,
                reserved:4;
} VEEPROM_FLTSTS_POWER_2_REG;

typedef union _VEEPROM_FLTSTS_POWER_2_RAW_INFO
{
    UINT_8E                             StatusControl;
    VEEPROM_FLTSTS_POWER_2_REG          fields;
} VEEPROM_FLTSTS_POWER_2_RAW_INFO;

typedef struct _VEEPROM_FLTSTS_FANCOOLING_0_REG
{
    UINT_8E     fan_0:1,
                fan_1:1,
                fan_2:1,
                fan_3:1,
                fan_4:1,
                fan_5:1,
                fan_6:1,
                fan_7:1;
} VEEPROM_FLTSTS_FANCOOLING_0_REG;

typedef union _VEEPROM_FLTSTS_FANCOOLING_0_RAW_INFO
{
    UINT_8E                             StatusControl;
    VEEPROM_FLTSTS_FANCOOLING_0_REG     fields;
} VEEPROM_FLTSTS_FANCOOLING_0_RAW_INFO;

typedef struct _VEEPROM_FLTSTS_FANCOOLING_1_REG
{
    UINT_8E     fan_8:1,
                fan_9:1,
                fan_10:1,
                fan_11:1,
                reserved:4;
} VEEPROM_FLTSTS_FANCOOLING_1_REG;

typedef union _VEEPROM_FLTSTS_FANCOOLING_1_RAW_INFO
{
    UINT_8E                             StatusControl;
    VEEPROM_FLTSTS_FANCOOLING_1_REG     fields;
} VEEPROM_FLTSTS_FANCOOLING_1_RAW_INFO;

//++
// Name:
//      IPMI_RESPONSE_READ_FLTSTS_SLICS_POWER_FANCOOLING_I2C_BYTES
//
// Description:
//      Data portion of IPMI response for 'Read Fault Status, SLICS(3bytes) + POWER + FAN/COOLING + I2C(4bytes)' command.
//--
typedef struct _IPMI_RESPONSE_READ_FLTSTS_SLICS_POWER_FANCOOLING_I2C_BYTES
{
    UCHAR                                   bytesRead; //retain this line when switching to IPMI_RESPONSE_READ_ALL_FAULT_STATUS_BYTES
    VEEPROM_FLTSTS_GENERIC_RAW_INFO         reserved_slics_40_63[3]; //Address 0285 to 0287
    VEEPROM_FLTSTS_POWER_1_RAW_INFO         power_bits_0_7; //Address 0288
    VEEPROM_FLTSTS_POWER_2_RAW_INFO         power_bits_8_15; //Address 0289
    VEEPROM_FLTSTS_GENERIC_RAW_INFO         reserved_power_bits_16_63[6]; //Address 028A to 028F
    VEEPROM_FLTSTS_FANCOOLING_0_RAW_INFO    fancooling_bits_0_7; //Address 0290
    VEEPROM_FLTSTS_FANCOOLING_1_RAW_INFO    fancooling_bits_8_15; //Address 0291
    VEEPROM_FLTSTS_GENERIC_RAW_INFO         reserved_fancooling_bits_8_63[6]; //Address 0292 to 0297
    VEEPROM_FLTSTS_GENERIC_RAW_INFO         i2c_buses_0_7; //Address 0298
    VEEPROM_FLTSTS_GENERIC_RAW_INFO         reserved_i2c_buses_8_31[3]; //Address 0299 to 029B
}
IPMI_RESPONSE_READ_FLTSTS_SLICS_POWER_FANCOOLING_I2C_BYTES, *PIPMI_RESPONSE_READ_FLTSTS_SLICS_POWER_FANCOOLING_I2C_BYTES;

typedef struct _VEEPROM_FLTSTS_SYSMISC_1_REG
{
    UINT_8E     cpu_module:1,
                mgmt_module:1,
                backend_module:1,
                uSSD:1,
                cache_card_eHornet:1,
                enclosure_midplane:1,
                cmi_path:1,
                entire_blade:1;
} VEEPROM_FLTSTS_SYSMISC_1_REG;

typedef union _VEEPROM_FLTSTS_SYSMISC_1_RAW_INFO
{
    UINT_8E                             StatusControl;
    VEEPROM_FLTSTS_SYSMISC_1_REG        fields;
} VEEPROM_FLTSTS_SYSMISC_1_RAW_INFO;

typedef struct _VEEPROM_FLTSTS_SYSMISC_2_REG
{
    UINT_8E     bldencl_external_fru:1,
                reserved:7;
} VEEPROM_FLTSTS_SYSMISC_2_REG;

typedef union _VEEPROM_FLTSTS_SYSMISC_2_RAW_INFO
{
    UINT_8E                             StatusControl;
    VEEPROM_FLTSTS_SYSMISC_2_REG        fields;
} VEEPROM_FLTSTS_SYSMISC_2_RAW_INFO;

typedef struct _VEEPROM_FLTSTS_CPUSTATUS_REG
{
    csx_u32_t   classification:24,
                agent:8;
} VEEPROM_FLTSTS_CPUSTATUS_REG;

typedef union _VEEPROM_FLTSTS_CPUSTATUS_RAW_INFO
{
    csx_u32_t                           status;
    VEEPROM_FLTSTS_CPUSTATUS_REG        fields;
} VEEPROM_FLTSTS_CPUSTATUS_RAW_INFO;

//++
// Name:
//      IPMI_RESPONSE_READ_FLTSTS_I2C_SYSMISC_CPU_BYTES
//
// Description:
//      Data portion of IPMI response for 'Read Fault Status, I2C(4bytes) + SYSMISC + CPU' command.
//--
typedef struct _IPMI_RESPONSE_READ_FLTSTS_I2C_SYSMISC_CPU_BYTES
{
    UCHAR                                     bytesRead; //retain this line when switching to IPMI_RESPONSE_READ_ALL_FAULT_STATUS_BYTES
    VEEPROM_FLTSTS_GENERIC_RAW_INFO           reserved_i2c_buses_32_63[4]; //Address 029C to 029F
    VEEPROM_FLTSTS_SYSMISC_1_RAW_INFO         sysmisc_bits_0_7; //Address 02A0
    VEEPROM_FLTSTS_SYSMISC_2_RAW_INFO         sysmisc_bits_8_15; //Address 02A1
    VEEPROM_FLTSTS_GENERIC_RAW_INFO           reserved_sysmisc_bits_16_63[6]; //Address 02A2 to 02A7
//Currently only 32bits i.e. 4 bytes are defined in the spec for cpu-status, so rest are kept reserved for now.
    VEEPROM_FLTSTS_CPUSTATUS_RAW_INFO         cpu_status_bytes_0_3; //Address 02A8 to 02AB
    UCHAR                                     reserved_cpu_bytes_4_7[4]; //Address 02AC to 02AF
    UCHAR                                     reserved[3]; //Address 02B0 to 02B2
}
IPMI_RESPONSE_READ_FLTSTS_I2C_SYSMISC_CPU_BYTES, *PIPMI_RESPONSE_READ_FLTSTS_I2C_SYSMISC_CPU_BYTES;

// IMPORTANT NOTE:-
// IPMI_RESPONSE_READ_FLTSTS_MEMORY_DRVS_0_55_BYTES, IPMI_RESPONSE_READ_FLTSTS_DRVS_56_239_BYTES, IPMI_RESPONSE_READ_FLTSTS_DRVS_240_383_SLICS_BYTES,
// IPMI_RESPONSE_READ_FLTSTS_SLICS_POWER_FANCOOLING_I2C_BYTES, IPMI_RESPONSE_READ_FLTSTS_I2C_SYSMISC_CPU_BYTES
// need to be polled at the same frequency, as they have overlapping fault-status-information for the "types" of faults recorded in them (the names indicate the overlap as well).

//++
// Name:
//      IPMI_RESPONSE_READ_ALL_FAULT_STATUS_BYTES
//
// Description:
//      Data portion of IPMI response for 'Read All Fault Status bytes' command.
//--
typedef struct _IPMI_RESPONSE_READ_ALL_FAULT_STATUS_BYTES
{
//    IPMI_RESPONSE_READ_FLTSTS_MEMORY_DRVS_0_55_BYTES;  starting at 0240h
    UCHAR                                     bytesRead;            // the size of response data 
    VEEPROM_FLTSTS_GENERIC_RAW_INFO           dimm_0_7;             // Address 0240
    VEEPROM_FLTSTS_GENERIC_RAW_INFO           dimm_8_15;            // Address 0241
    VEEPROM_FLTSTS_GENERIC_RAW_INFO           dimm_16_23;           // Address 0242
    VEEPROM_FLTSTS_GENERIC_RAW_INFO           dimm_24_31;           // Address 0243
    VEEPROM_FLTSTS_GENERIC_RAW_INFO           reserved_dimms[12];   // Address 0244 to 024F
    VEEPROM_FLTSTS_GENERIC_RAW_INFO           drives_0_7;           // Address 0250
    VEEPROM_FLTSTS_GENERIC_RAW_INFO           drives_8_15;          // Address 0251
    VEEPROM_FLTSTS_GENERIC_RAW_INFO           drives_16_23;         // Address 0252
    VEEPROM_FLTSTS_GENERIC_RAW_INFO           drives_24_31;         // Address 0253        currently only supports upto drive-bit 25 i.e. total 26 drives, bit 26 to 31 are reserved.
    VEEPROM_FLTSTS_GENERIC_RAW_INFO           reserved_drives_32_55[3];     // Address 0254 to 0256

//    IPMI_RESPONSE_READ_FLTSTS_DRVS_56_239_BYTES
    VEEPROM_FLTSTS_GENERIC_RAW_INFO           reserved_drives_56_239[23];   // Address 0257 to 026D

//    IPMI_RESPONSE_READ_FLTSTS_DRVS_240_383_SLICS_BYTES
    VEEPROM_FLTSTS_GENERIC_RAW_INFO           reserved_drives_240_383[18];  // Address 026E to 027F
    VEEPROM_FLTSTS_GENERIC_RAW_INFO           slics_0_7;                    // Address 0280
    VEEPROM_FLTSTS_GENERIC_RAW_INFO           slics_8_15;                   // Address 0281          (this really needs to go till 12 slics)
    VEEPROM_FLTSTS_GENERIC_RAW_INFO           reserved_slics_16_39[3];      // Address 0282 to 0284
 
//    IPMI_RESPONSE_READ_FLTSTS_SLICS_POWER_FANCOOLING_I2C_BYTES
    VEEPROM_FLTSTS_GENERIC_RAW_INFO           reserved_slics_40_63[3];  // Address 0285 to 0287
    VEEPROM_FLTSTS_POWER_1_RAW_INFO           power_bits_0_7;           // Address 0288
    VEEPROM_FLTSTS_POWER_2_RAW_INFO           power_bits_8_15;          // Address 0289
    VEEPROM_FLTSTS_GENERIC_RAW_INFO           reserved_power_bits_16_63[6]; // Address 028A to 028F
    VEEPROM_FLTSTS_FANCOOLING_0_RAW_INFO      fancooling_bits_0_7;      // Address 0290
    VEEPROM_FLTSTS_FANCOOLING_1_RAW_INFO      fancooling_bits_8_15;     // Address 0291
    VEEPROM_FLTSTS_GENERIC_RAW_INFO           reserved_fancooling_bits_8_63[6]; // Address 0292 to 0297
    VEEPROM_FLTSTS_GENERIC_RAW_INFO           i2c_buses_0_7;            // Address 0298
    VEEPROM_FLTSTS_GENERIC_RAW_INFO           reserved_i2c_buses_8_31[3];   // Address 0299 to 029B

//    IPMI_RESPONSE_READ_FLTSTS_I2C_SYSMISC_CPU_BYTES
    VEEPROM_FLTSTS_GENERIC_RAW_INFO           reserved_i2c_buses_32_63[4];  // Address 029C to 029F
    VEEPROM_FLTSTS_SYSMISC_1_RAW_INFO         sysmisc_bits_0_7;             // Address 02A0
    VEEPROM_FLTSTS_SYSMISC_2_RAW_INFO         sysmisc_bits_8_15;            // Address 02A1
    VEEPROM_FLTSTS_GENERIC_RAW_INFO           reserved_sysmisc_bits_16_63[6];// Address 02A2 to 02A7
   //Currently only 32bits i.e. 4 bytes are defined in the spec for cpu-status, so rest are kept reserved for now.
    VEEPROM_FLTSTS_CPUSTATUS_RAW_INFO         cpu_status_bytes_0_3;         // Address 02A8 to 02AB
    UCHAR                                     reserved_cpu_bytes_4_7[4];    // Address 02AC to 02AF
    UCHAR                                     reserved[3];                  // Address 02B0 to 02B2
}
IPMI_RESPONSE_READ_ALL_FAULT_STATUS_BYTES, *PIPMI_RESPONSE_READ_ALL_FAULT_STATUS_BYTES; //115 bytes in all.

typedef enum _IPMI_FSR_FRU_TYPES
{
    IPMI_FSR_FRU_ALL,
    IPMI_FSR_FRU_DIMM, //Managed by BIOS/POST (sticky).
    IPMI_FSR_FRU_DRIVE, //Managed by BIOS/POST (sticky).
    IPMI_FSR_FRU_SLIC, //Managed by BIOS/POST (sticky). Also set by BMC/SSP during run-time when a power-failure-event happens on a SLIC.
    IPMI_FSR_FRU_PS, //Managed by BMC/SSP (not sticky).
    IPMI_FSR_FRU_BATTERY, //Managed by BMC/SSP (not sticky).
    IPMI_FSR_FRU_FAN_COOLING, //Managed by BMC/SSP (not sticky).
    IPMI_FSR_FRU_I2C, //Managed by BMC/SSP (sticky).
    //SYSMISC is used for cpuFault, mgmtFault, bemFault, eFlashFault, cacheCardFault, midplaneFault, cmiFault, allFrusFault, externalFruFault.
    IPMI_FSR_FRU_SYSMISC, //Managed by BIOS/POST (sticky). Also set by BMC/SSP during run-time when a power-failure-event happens on the BEM.
    IPMI_FSR_FRU_RESERVED
} IPMI_FSR_FRU_TYPES, *PIPMI_FSR_FRU_TYPES;

// VEEPROM data structures end.

//++
// Name:
//      IPMI_RESPONSE_READ_FRU_RESUME
//
// Description:
//      Data portion of IPMI response for 'Read FRU resume' command.
//--
typedef struct _IPMI_RESPONSE_READ_FRU_RESUME
{
    UCHAR                                     bytesRead;
    UCHAR                                     data_bytes[32];
}
IPMI_RESPONSE_READ_FRU_RESUME, *PIPMI_RESPONSE_READ_FRU_RESUME;

// Battery (Copilot) enums and structures start

//++
// Name:
//      IPMI_BATTERY_COMMAND_DATAFORMAT
//
// Description:
//      IPMI BATTERY COMMAND DATAFORMATs
//--
typedef enum _IPMI_BATTERY_COMMAND_DATAFORMAT
{
   IPMI_BATTERY_DATA_FORMAT_COPILOT          = 0x00,
   IPMI_BATTERY_DATA_FORMAT_BEACHCOMBER      = 0x01,
   IPMI_BATTERY_DATA_FORMAT_INVALID          = 0xFF
} IPMI_BATTERY_COMMAND_DATAFORMAT;

//++
// Name:
//      IPMI_BATTERY_COPILOT_SLOT_ID
//
// Description:
//      IPMI BATTERY COPILOT SLOT ID (For Copilot only)
//--
typedef enum _IPMI_BATTERY_COPILOT_SLOT_ID
{
   IPMI_BATTERY_COPILOT_SLOT_ID_0          = 0x00,
   IPMI_BATTERY_COPILOT_SLOT_ID_1          = 0x01,
   IPMI_BATTERY_COPILOT_SLOT_ID_INVALID    = 0xFF
} IPMI_BATTERY_COPILOT_SLOT_ID;

//++
// Name:
//      IPMI_BATTERY_ENABLE_STATE_CODE
//
// Description:
//      IPMI BATTERY ENABLE STATE CODE (For Copilot only)
//--
typedef enum _IPMI_BATTERY_ENABLE_STATE_CODE
{
   IPMI_BATTERY_DISABLE_VIA_GPIO         = 0x00,
   IPMI_BATTERY_ENABLE_VIA_GPIO          = 0x01,
   IPMI_BATTERY_OVERRIDE_DISABLE_VIA_I2C = 0x11,
   IPMI_BATTERY_OVERRIDE_ENABLE_VIA_I2C  = 0x22,
   IPMI_BATTERY_ENABLE_STATE_CODE_INVALID= 0xFF
} IPMI_BATTERY_ENABLE_STATE_CODE;

typedef struct _COPILOT_FAULT_FLAGS_REG
{
    UINT_16E                  lowCellChargeCapacity:1,
                              deepCellUV:1,
                              excessiveCellUV:1,
                              overMaxChargeTime:1,
                              endOfCellLife:1,
                              highImpedance:1,
                              cellOV:1,
                              underVoltage12V:1,
                              overVoltage12V:1,
                              overCurrent12V:1,
                              underVoltage:1,
                              overVoltage:1,
                              overCurrent:1,
                              tempLow:1,
                              tempHigh:1,
                              cellExtremeUnderVoltage:1;

} COPILOT_FAULT_FLAGS_REG;

typedef union _COPILOT_FAULT_FLAGS_RAW
{
    UINT_16E                  flags;
    COPILOT_FAULT_FLAGS_REG   fields;
} COPILOT_FAULT_FLAGS_RAW;

typedef struct _COPILOT_REGULATOR_CURRENT
{
    UINT_8E                   state:1,              // 0 - discharging, 1 - charging
                              current:7;            // in units of amps

} COPILOT_REGULATOR_CURRENT;

//++
// Name:
//      IPMI_RESPONSE_GET_BATTERY_STATUS
//
// Description:
//      Data portion of IPMI response for 'Get Battery Status' command.
//--
typedef struct _IPMI_RESPONSE_GET_BATTERY_STATUS
{
    UINT_8E                   minorFirmwareRevision;
    UINT_8E                   majorFirmwareRevision;
    UINT_16E                  dischargeCycleCount;
    UINT_16E                  serviceLife;          // in units of days
    UINT_16E                  esr;                  // in units of 0.01 ohm
    UINT_8E                   worstCaseTemperature; // in units of degress C.
    COPILOT_FAULT_FLAGS_RAW   faultFlags;
    UINT_16E                  maximumBatteryEnergy;
    UINT_16E                  currentStoredEnergy;
    UINT_8E                   senseLineAdjustment;
    COPILOT_REGULATOR_CURRENT regulatorCurrent;
    UINT_8E                   reserved;
}
IPMI_RESPONSE_GET_BATTERY_STATUS, *PIPMI_RESPONSE_GET_BATTERY_STATUS;

typedef struct 
{
    UINT_8E                   minorFirmwareRevision;
    UINT_8E                   majorFirmwareRevision;
    UINT_16E                  dischargeCycleCount;
    UINT_16E                  reserved1;
    UINT_8E                   temperature;
    COPILOT_FAULT_FLAGS_RAW   faultFlags;
    UINT_16E                  reserved2;
    UINT_16E                  reserved3;
    UINT_8E                   reserved4;
}IPMI_RESPONSE_GET_BC_BATTERY_STATUS, *PIPMI_RESPONSE_GET_BC_BATTERY_STATUS;

//++
// Name:
//      IPMI_REQUEST_SET_BATTERY_ENABLE_STATE
//
// Description:
//      'Set Battery Enable State' command data buffer.
//--
typedef struct _IPMI_REQUEST_SET_BATTERY_ENABLE_STATE
{
    union {
        struct {
            UINT_8E                     dataFormat;
            UINT_8E                     batterySlot;    // This applies on Copilot
            UINT_8E                     enableCode;
        }Copilot;
        struct {
            UINT_8E                     dataFormat;
            UINT_8E                     enableCode;
        }BBU;
    };
}IPMI_REQUEST_SET_BATTERY_ENABLE_STATE, *PIPMI_REQUEST_SET_BATTERY_ENABLE_STATE;

//++
// Name:
//      IPMI_REQUEST_P_SET_BATTERY_ENABLE_STATE
//
// Description:
//      'Set Battery Enable State' command data buffer.
//       for Phobos BMC
//--
typedef struct _IPMI_REQUEST_P_SET_BATTERY_ENABLE_STATE
{
    UINT_8E                     enableCode;
}IPMI_REQUEST_P_SET_BATTERY_ENABLE_STATE, *PIPMI_REQUEST_P_SET_BATTERY_ENABLE_STATE;

//++
// Name:
//      IPMI_REQUEST_SET_BATTERY_ENERGY_REQUIREMENTS
//
// Description:
//      'Set Battery energy requirements' command data buffer.
//--
typedef struct _IPMI_REQUEST_SET_BATTERY_ENERGY_REQUIREMENTS
{
    union{
        struct {
            UINT_8E                     dataFormat;
            UINT_8E                     batterySlot;    // This applies on Copilot
            UINT_16E                    energy;
            UINT_16E                    maxLoad;
        }Copilot;
        struct {
            UINT_8E                     dataFormat;
            UINT_16E                    dischargeRate;
            UINT_16E                    remainingTime;
        }BBU;
    };
}IPMI_REQUEST_SET_BATTERY_ENERGY_REQUIREMENTS, *PIPMI_REQUEST_SET_BATTERY_ENERGY_REQUIREMENTS;

typedef struct _BATTERY_CONFIG_MASK_REG
{
    UINT_16E                  systemRequirements:1,
                              batteryConfig:1,
                              reserved:14;
} BATTERY_CONFIG_MASK_REG;

typedef union _BATTERY_CONFIG_MASK_RAW
{
    UINT_16E                  flags;
    BATTERY_CONFIG_MASK_REG   fields;
} BATTERY_CONFIG_MASK_RAW;

//++
// Name:
//      IPMI_REQUEST_P_SET_BATTERY_ENERGY_REQUIREMENTS
//
// Description:
//      'Set Battery energy requirements' command data buffer.
//      for Phobos BMC
//--
typedef struct _IPMI_REQUEST_P_SET_BATTERY_ENERGY_REQUIREMENTS
{
    UINT_8E                     fruID;          //of the battery to set
    BATTERY_CONFIG_MASK_RAW     configMask;
    UINT_16E                    energy;         //systemRequirements
    UINT_16E                    maxLoad;        //systemRequirements
    UINT_16E                    dischargeRate;  //batteryConfig
    UINT_16E                    remainingTime;  //batteryConfig
}IPMI_REQUEST_P_SET_BATTERY_ENERGY_REQUIREMENTS, *PIPMI_REQUEST_P_SET_BATTERY_ENERGY_REQUIREMENTS;

//++
// Name:
//      IPMI_REQUEST_GET_BATTERY_ENERGY_REQUIREMENTS
//
// Description:
//      'Get Battery energy requirements' command data buffer.
//--
typedef struct _IPMI_REQUEST_GET_BATTERY_ENERGY_REQUIREMENTS
{
    UINT_8E                     dataFormat;
    UINT_8E                     batterySlot;    // This applies on Copilot
}IPMI_REQUEST_GET_BATTERY_ENERGY_REQUIREMENTS, *PIPMI_REQUEST_GET_BATTERY_ENERGY_REQUIREMENTS;

//++
// Name:
//      IPMI_REQUEST_P_GET_BATTERY_ENERGY_REQUIREMENTS
//
// Description:
//      'Get Battery energy requirements' command data buffer.
//      for Phobos BMC
//--
typedef struct _IPMI_REQUEST_P_GET_BATTERY_ENERGY_REQUIREMENTS
{
    UINT_8E                     fruID;    // of the battery requested
}IPMI_REQUEST_P_GET_BATTERY_ENERGY_REQUIREMENTS, *PIPMI_REQUEST_P_GET_BATTERY_ENERGY_REQUIREMENTS;

//++
// Name:
//      IPMI_RESPONSE_GET_BATTERY_ENERGY_REQUIREMENTS_FORMAT0
//
// Description:
//      Data portion of IPMI response for 'Get Battery energy requirements' command.
//      Format 0, for Copilot data.
//--
typedef struct _IPMI_RESPONSE_GET_BATTERY_ENERGY_REQUIREMENTS_FORMAT0
{
    UINT_16E                    energy;
    UINT_16E                    maxLoad;
}IPMI_RESPONSE_GET_BATTERY_ENERGY_REQUIREMENTS_FORMAT0, *PIPMI_RESPONSE_GET_BATTERY_ENERGY_REQUIREMENTS_FORMAT0;

//++
// Name:
//      IPMI_RESPONSE_GET_BATTERY_ENERGY_REQUIREMENTS_FORMAT1
//
// Description:
//      Data portion of IPMI response for 'Get Battery energy requirements' command.
//      Format 1, for BBU data.
//--
typedef struct _IPMI_RESPONSE_GET_BATTERY_ENERGY_REQUIREMENTS_FORMAT1
{
    UINT_16E                    dischargeRate;
    UINT_16E                    remainingTime;
}IPMI_RESPONSE_GET_BATTERY_ENERGY_REQUIREMENTS_FORMAT1, *PIPMI_RESPONSE_GET_BATTERY_ENERGY_REQUIREMENTS_FORMAT1;

//++
// Name:
//      IPMI_RESPONSE_P_GET_BATTERY_ENERGY_REQUIREMENTS
//
// Description:
//      Data portion of IPMI response for 'Get Battery energy requirements' command.
//      for Phobos BMC
//--
typedef struct _IPMI_RESPONSE_P_GET_BATTERY_ENERGY_REQUIREMENTS
{
    BATTERY_CONFIG_MASK_RAW     configMask;
    UINT_16E                    energy;         //systemRequirements
    UINT_16E                    maxLoad;        //systemRequirements
    UINT_16E                    dischargeRate;  //batteryConfig
    UINT_16E                    remainingTime;  //batteryConfig
}IPMI_RESPONSE_P_GET_BATTERY_ENERGY_REQUIREMENTS, *PIPMI_RESPONSE_P_GET_BATTERY_ENERGY_REQUIREMENTS;

//++
// Name:
//      IPMI_REQUEST_CLEAR_BATTERY_FAULTS
//
// Description:
//      'Clear Battery Faults' command data buffer.
//--
typedef struct _IPMI_REQUEST_CLEAR_BATTERY_FAULTS
{
    union {
        struct {
            UINT_8E                     dataFormat;
            UINT_8E                     batterySlot;    // This applies on Copilot
            UINT_8E                     clearCommandCode;
        }Copilot;
        struct {
            UINT_8E                     dataFormat;
            UINT_8E                     clearCommandCode;
        }BBU;
    };
}
IPMI_REQUEST_CLEAR_BATTERY_FAULTS, *PIPMI_REQUEST_CLEAR_BATTERY_FAULTS;

//++
// Name:
//      IPMI_REQUEST_P_CLEAR_BATTERY_FAULTS
//
// Description:
//      'Clear Battery Faults' command data buffer.
//      for Phobos BMC
//--
typedef struct _IPMI_REQUEST_P_CLEAR_BATTERY_FAULTS
{
    UINT_8E             fruID;  //of the battery to clear
    UINT_8E             charC;  //'C'
    UINT_8E             charL;  //'L'
    UINT_8E             charR;  //'R'
}
IPMI_REQUEST_P_CLEAR_BATTERY_FAULTS, *PIPMI_REQUEST_P_CLEAR_BATTERY_FAULTS;

typedef struct _BATTERY_I2C_SENSOR_READING_REG_B0_B7
{
    UINT_8E     inserted:1,
                communication_error:1,
                enabled:1,
                faulted:1,
                holdup_ready:1,
                charging:1,
                discharging:1,
                full_charged:1;
} BATTERY_I2C_SENSOR_READING_REG_B0_B7;

typedef union _BATTERY_I2C_SENSOR_READING_RAW_INFO_B0_B7
{
    UINT_8E                                 StatusControl;
    BATTERY_I2C_SENSOR_READING_REG_B0_B7    fields;
} BATTERY_I2C_SENSOR_READING_RAW_INFO_B0_B7;

typedef struct _BATTERY_I2C_SENSOR_READING_REG_B8_B15
{
    UINT_8E     sense_line_modified:1,  // Copilot only
                green_led_state_lsb:1,  // Copilot only
                green_led_state_msb:1,  // Copilot only
                armber_led_state_lsb:1, // Copilot only
                armber_led_state_msb:1, // Copilot only
                output_enabled:1,       // bbu only
                reserved:2;
} BATTERY_I2C_SENSOR_READING_REG_B8_B15;

typedef union _BATTERY_I2C_SENSOR_READING_RAW_INFO_B8_B15
{
    UINT_8E                                 StatusControl;
    BATTERY_I2C_SENSOR_READING_REG_B8_B15   fields;
} BATTERY_I2C_SENSOR_READING_RAW_INFO_B8_B15;

//++
// Name:
//      IPMI_RESPONSE_GET_SENSOR_READING_BATTERY_I2C
//
// Description:
//      Data portion of IPMI response for 'Get Sensor Reading for Battery over I2C'.
//--
typedef struct _IPMI_RESPONSE_GET_SENSOR_READING_BATTERY_I2C
{
    UCHAR                                               sensorData;
    SENSOR_READING_SENSORSTATUS_BITS_RAW_INFO           sensorStatus;
    BATTERY_I2C_SENSOR_READING_RAW_INFO_B0_B7           sensorState;
    BATTERY_I2C_SENSOR_READING_RAW_INFO_B8_B15          additionalSensorState;
}IPMI_RESPONSE_GET_SENSOR_READING_BATTERY_I2C, *PIPMI_RESPONSE_GET_SENSOR_READING_BATTERY_I2C;

typedef struct _BATTERY_P_I2C_SENSOR_READING_REG_B0_B7
{
    UINT_8E     inserted:1,
                enabled:1,
                charging:1,
                discharging:1,
                holdup_ready:1,
                full_charged:1,
                comm_mem_logic_fault:1,
                input_fault:1;
} BATTERY_P_I2C_SENSOR_READING_REG_B0_B7;

typedef union _BATTERY_P_I2C_SENSOR_READING_RAW_INFO_B0_B7
{
    UINT_8E                                 StatusControl;
    BATTERY_P_I2C_SENSOR_READING_REG_B0_B7  fields;
} BATTERY_P_I2C_SENSOR_READING_RAW_INFO_B0_B7;

typedef struct _BATTERY_P_I2C_SENSOR_READING_REG_B8_B15
{
    UINT_8E     output_fault:1,
                cell_fault:1,
                battery_fault:1,
                internal_power_fault:1,
                temperature_fault:1,
                external_fault:1,
                internal_fault:1,
                reserved:1;
} BATTERY_P_I2C_SENSOR_READING_REG_B8_B15;

typedef union _BATTERY_P_I2C_SENSOR_READING_RAW_INFO_B8_B15
{
    UINT_8E                                 StatusControl;
    BATTERY_P_I2C_SENSOR_READING_REG_B8_B15 fields;
} BATTERY_P_I2C_SENSOR_READING_RAW_INFO_B8_B15;

//++
// Name:
//      IPMI_RESPONSE_P_GET_SENSOR_READING_BATTERY_I2C
//
// Description:
//      Data portion of IPMI response for 'Get Sensor Reading for Battery over I2C'.
//      For Phobos BMC
//--
typedef struct _IPMI_RESPONSE_P_GET_SENSOR_READING_BATTERY_I2C
{
    UCHAR                                               sensorData;
    SENSOR_READING_SENSORSTATUS_BITS_RAW_INFO           sensorStatus;
    BATTERY_P_I2C_SENSOR_READING_RAW_INFO_B0_B7         sensorState;
    BATTERY_P_I2C_SENSOR_READING_RAW_INFO_B8_B15        additionalSensorState;
}IPMI_RESPONSE_P_GET_SENSOR_READING_BATTERY_I2C, *PIPMI_RESPONSE_P_GET_SENSOR_READING_BATTERY_I2C;

typedef struct _BATTERY_GPIO_SENSOR_READING_REG_B0_B7
{
    UINT_8E     inserted:1,
                status_line_responsive:1,  // Copilot only
                holdup_ready:1,
                invalid_tach_rate:1,
                enabled:1,
                on_battery:1,
                reserved:2;
} BATTERY_GPIO_SENSOR_READING_REG_B0_B7;

typedef union _BATTERY_GPIO_SENSOR_READING_RAW_INFO_B0_B7
{
    UINT_8E                                 StatusControl;
    BATTERY_GPIO_SENSOR_READING_REG_B0_B7   fields;
} BATTERY_GPIO_SENSOR_READING_RAW_INFO_B0_B7;

//++
// Name:
//      IPMI_RESPONSE_GET_SENSOR_READING_BATTERY_GPIO
//
// Description:
//      Data portion of IPMI response for 'Get Sensor Reading for Battery over GPIO'.
//--
typedef struct _IPMI_RESPONSE_GET_SENSOR_READING_BATTERY_GPIO
{
    UCHAR                                           sensorData;
    SENSOR_READING_SENSORSTATUS_BITS_RAW_INFO       sensorStatus;
    BATTERY_GPIO_SENSOR_READING_RAW_INFO_B0_B7      sensorState;
    UCHAR                                           additionalSensorState;
}IPMI_RESPONSE_GET_SENSOR_READING_BATTERY_GPIO, *PIPMI_RESPONSE_GET_SENSOR_READING_BATTERY_GPIO;

typedef struct _BATTERY_P_GPIO_SENSOR_READING_REG_B0_B7
{
    UINT_8E     inserted:1,
                holdup_ready:1,
                invalid_tach_rate:1,
                enabled:1,
                on_battery:1,
                not_charging_cant_sustain_discharge:1,
                reserved:2;
} BATTERY_P_GPIO_SENSOR_READING_REG_B0_B7;

typedef union _BATTERY_P_GPIO_SENSOR_READING_RAW_INFO_B0_B7
{
    UINT_8E                                 StatusControl;
    BATTERY_P_GPIO_SENSOR_READING_REG_B0_B7 fields;
} BATTERY_P_GPIO_SENSOR_READING_RAW_INFO_B0_B7;

//++
// Name:
//      IPMI_RESPONSE_P_GET_SENSOR_READING_BATTERY_GPIO
//
// Description:
//      Data portion of IPMI response for 'Get Sensor Reading for Battery over GPIO'.
//      for Phobos BMC.
//--
typedef struct _IPMI_RESPONSE_P_GET_SENSOR_READING_BATTERY_GPIO
{
    UCHAR                                           sensorData;
    SENSOR_READING_SENSORSTATUS_BITS_RAW_INFO       sensorStatus;
    BATTERY_P_GPIO_SENSOR_READING_RAW_INFO_B0_B7    sensorState;
    UCHAR                                           additionalSensorState;
}IPMI_RESPONSE_P_GET_SENSOR_READING_BATTERY_GPIO, *PIPMI_RESPONSE_P_GET_SENSOR_READING_BATTERY_GPIO;

//++
// Name:
//      IPMI_RESPONSE_BATTERY_SELFTEST_RESULT_1
//
// Description:
//      Data portion of IPMI response for 'Battery passthrough command to read self test result in reg_71'.
//--
typedef struct _IPMI_RESPONSE_BATTERY_SELFTEST_RESULT_1
{
    UCHAR             testResult;
}
IPMI_RESPONSE_BATTERY_SELFTEST_RESULT_1, *PIPMI_RESPONSE_BATTERY_SELFTEST_RESULT_1;

typedef struct _BATTERY_REG_OFFSET_72_READING_B0_B15
{
    UINT_16E     buck_converter_fault:1,
                rail_lift_ckt_fault:1,
                V_BAT_SWITCH_bias_voltage_ckt_fault:1,
                load_share_ckt_fault:1,
                rt_clock_calendar_chip_fault:1,
                reserved:11;
} BATTERY_REG_OFFSET_72_READING_B0_B15;

typedef union _BATTERY_REG_OFFSET_72_READING_RAW_INFO_B0_B15
{
    UINT_16E                                 flags;
    BATTERY_REG_OFFSET_72_READING_B0_B15     fields;
} BATTERY_REG_OFFSET_72_READING_RAW_INFO_B0_B15;

//++
// Name:
//      IPMI_RESPONSE_BATTERY_SELFTEST_RESULT_2
//
// Description:
//      Data portion of IPMI response for 'Battery passthrough command to read self test result in reg_72'.
//--
typedef struct _IPMI_RESPONSE_BATTERY_SELFTEST_RESULT_2
{
    BATTERY_REG_OFFSET_72_READING_RAW_INFO_B0_B15             dataBytes;
}
IPMI_RESPONSE_BATTERY_SELFTEST_RESULT_2, *PIPMI_RESPONSE_BATTERY_SELFTEST_RESULT_2;

//++
// Name:
//      IPMI_RESPONSE_BATTERY_FAULT_STATUS
//
// Description:
//      Data portion of IPMI response for 'Battery passthrough command to read fault status in non-paged reg_22'.
//--
typedef struct _IPMI_RESPONSE_BATTERY_FAULT_STATUS
{
    UCHAR             faultStatus;
}
IPMI_RESPONSE_BATTERY_FAULT_STATUS, *PIPMI_RESPONSE_BATTERY_FAULT_STATUS;

typedef enum _IPMI_BATTERY_FAULT_STATUS_CODES
{
    IPMI_BATTERY_NO_FAULT_EXISTS                  = 0x11, //All faults are cleared or no fault exists.
    IPMI_BATTERY_CLEARABLE_FAULT_EXISTS           = 0xAA, //Clearable fault exists. To clear the fault & re-enable BOB module's operation, 0x11 shall be written to this register.
    IPMI_BATTERY_SELF_CLEARING_FAULT_EXISTS       = 0xBB, //This is a self-clearing-fault (say like temperature fault). BOB will return to normal operation when the fault clears.
    IPMI_BATTERY_PERSISTENT_LATCHING_FAULT_EXISTS = 0xCC, //This fault cannot be cleared, replace the BOB.
    IPMI_BATTERY_FAULT_STATUS_UNKNOWN             = 0xFF
} IPMI_BATTERY_FAULT_STATUS_CODES;

typedef enum _IPMI_BATTERY_SELFTEST_PROGRESS_CODE
{
   IPMI_BATTERY_SELFTEST_NO_FAULT_CLEAR_FAULT         = 0x00,
   IPMI_BATTERY_SELFTEST_INPROGRESS                   = 0x11,
   IPMI_BATTERY_SELFTEST_SUCCESSFUL                   = 0x22,
   IPMI_BATTERY_INSUFFICIENT_SYSLOAD_FOR_SELFTEST     = 0x55,
   IPMI_BATTERY_SELFTEST_ABORTED                      = 0x66,//Used only for Copilot2
   IPMI_BATTERY_SELFTEST_FORCED_STOP                  = 0x77,//Used only for Copilot2
   IPMI_BATTERY_SELFTEST_UNSUCCESSFUL                 = 0xAA,
   IPMI_BATTERY_SELFTEST_UNSUPPORTED                  = 0xBB,
   IPMI_BATTERY_INSUFFICIENT_ENERGY_FOR_SELFTEST      = 0xCC,
   IPMI_BATTERY_SELFTEST_FAIL_COPILOT_NOT_ENABLED     = 0xDD,
   IPMI_BATTERY_SELFTEST_BATTERY_DISCHARGING          = 0xEE,//Used only for Copilot2. Basically means system is vaulting, so dont run self-test now.
   IPMI_BATTERY_SELFTEST_STATE_CODE_INVALID           = 0xFF
} IPMI_BATTERY_SELFTEST_PROGRESS_CODE;

//++
// Name:
//      IPMI_REQUEST_P_SET_VAULT_MODE_CONFIGURATION
//
// Description:
//      'Set Vault Mode Requirements' command data buffer.
//      for Phobos BMC
//--
typedef struct _IPMI_REQUEST_P_SET_VAULT_MODE_CONFIGURATION
{
    UINT_8E                     lowPowerMode;
    UINT_8E                     rideThroughTime;
    UINT_8E                     vaultTimeout;
}IPMI_REQUEST_P_SET_VAULT_MODE_CONFIGURATION, *PIPMI_REQUEST_P_SET_VAULT_MODE_CONFIGURATION;

//++
// Name:
//      IPMI_RESPONSE_P_GET_VAULT_MODE_CONFIGURATION
//
// Description:
//      'Set Vault Mode Requirements' command data buffer.
//      for Phobos BMC
//--
typedef struct _IPMI_RESPONSE_P_GET_VAULT_MODE_CONFIGURATION
{
    UINT_8E                     lowPowerMode;
    UINT_8E                     rideThroughTime;
    UINT_8E                     vaultTimeout;
}IPMI_RESPONSE_P_GET_VAULT_MODE_CONFIGURATION, *PIPMI_RESPONSE_P_GET_VAULT_MODE_CONFIGURATION;

// Battery (Copilot) structures end

// BMC GPIO structures start

//++
// Name:
//      IPMI_BMC_GPIO_STATE
//
// Description:
//      IPMI BMC GPIO states.
//
//-
typedef enum _IPMI_BMC_GPIO_STATE
{
    IPMI_BMC_GPIO_INPUT_HIGH_NO_PULL          = 0,
    IPMI_BMC_GPIO_INPUT_LOW_NO_PULL           = 1,
    IPMI_BMC_GPIO_INPUT_HIGH_PULL_UP          = 2,
    IPMI_BMC_GPIO_INPUT_LOW_PULL_UP           = 3,
    IPMI_BMC_GPIO_INPUT_HIGH_PULL_DOWN        = 4,
    IPMI_BMC_GPIO_INPUT_LOW_PULL_DOWN         = 5,
    IPMI_BMC_GPIO_OUTPUT_HIGH_NO_PULL         = 6,
    IPMI_BMC_GPIO_OUTPUT_LOW_NO_PULL          = 7,
    IPMI_BMC_GPIO_OUTPUT_HIGH_PULL_UP         = 8,
    IPMI_BMC_GPIO_OUTPUT_LOW_PULL_UP          = 9,
    IPMI_BMC_GPIO_OUTPUT_HIGH_PULL_DOWN       = 10,
    IPMI_BMC_GPIO_OUTPUT_LOW_PULL_DOWN        = 11,
    IPMI_BMC_GPIO_STATE_INVALID               = 0xFF
}
IPMI_BMC_GPIO_STATE;

//++
// Name:
//      IPMI_BMC_SET_GPIO_STATE
//
// Description:
//      Set GPIO pin state
//      'Set GPIO state' command data buffer.
//--
typedef struct _IPMI_BMC_SET_GPIO_STATE
{
    UINT_8E                     portNumber;
    UINT_8E                     pinOffset;
    UINT_8E                     pinState;
}
IPMI_BMC_SET_GPIO_STATE, *PIPMI_BMC_SET_GPIO_STATE;

// BMC GPIO structures end

//++
// Name:
//      IPMI_MFB_OPERATION
//
// Description:
//      IPMI MFB operations.
//
//-
typedef enum _IPMI_MFB_OPERATION
{
    IPMI_MFB_READ_CONFIG          = 0,
    IPMI_MFB_WRITE_CONFIG         = 1,
    IPMI_MFB_OPERATION_INVALID    = 0xFF
}
IPMI_MFB_OPERATION;

//++
// Name:
//      IPMI_MFB_LED_FEEDBACK
//
// Description:
//      IPMI LED feedback configurations.
//
//-
typedef enum _IPMI_MFB_LED_FEEDBACK
{
    IPMI_MFB_LED_FEEDBACK_DISABLED        = 0,
    IPMI_MFB_LED_FEEDBACK_ENABLED         = 1,
    IPMI_MFB_LED_FEEDBACK_INVALID         = 0xFF
}
IPMI_MFB_LED_FEEDBACK;

//++
// Name:
//      IPMI_MFB_PRESS_ACTION
//
// Description:
//      IPMI MFB press actions.
//
//-
typedef enum _IPMI_MFB_PRESS_ACTION
{
    IPMI_MFB_ACTION_DO_NOTHING          = 0,
    IPMI_MFB_ACTION_RESET_HOST          = 1,
    IPMI_MFB_ACTION_ASSERT_SOFT_FLAG    = 2,
    IPMI_MFB_ACTION_ASSERT_SMI          = 3,
    IPMI_MFB_ACTION_POWER_ON_OFF        = 4,
    IPMI_MFB_ACTION_INVALID             = 0xFF
}
IPMI_MFB_PRESS_ACTION;

//++
// Name:
//      IPMI_REQUEST_MFB_CONFIGURATION
//
// Description:
//      'Get/Set MFB Configuration' command data buffer.
//--
typedef struct _IPMI_REQUEST_MFB_CONFIGURATION
{
    UINT_8E                     operation;                  // Read or Write
    UINT_8E                     ledFeedbackConfig;          // Ignored on read
    UINT_8E                     shortPressPreBootConfig;    // Ignored on read
    UINT_8E                     shortPressPostBootConfig;   // Ignored on read
    UINT_8E                     longPressPreBootConfig;     // Ignored on read
    UINT_8E                     longPressPostBootConfig;    // Ignored on read
}
IPMI_REQUEST_MFB_CONFIGURATION, *PIPMI_REQUEST_MFB_CONFIGURATION;

//++
// Name:
//      IPMI_RESPONSE_MFB_CONFIGURATION
//
// Description:
//      Data portion of IPMI response for 'Get/Set MFB Configuration' command.
//--
typedef struct _IPMI_RESPONSE_MFB_CONFIGURATION
{
    UINT_8E                     ledFeedbackConfig;
    UINT_8E                     shortPressPreBootConfig;
    UINT_8E                     shortPressPostBootConfig;
    UINT_8E                     longPressPreBootConfig;
    UINT_8E                     longPressPostBootConfig;
}
IPMI_RESPONSE_MFB_CONFIGURATION, *PIPMI_RESPONSE_MFB_CONFIGURATION;

//++
// Name:
//      IPMI_REQUEST_P_MFB_CONFIGURATION
//
// Description:
//      'Set MFB Configuration' command data buffer
//       for Phobos BMC
//--
typedef struct _IPMI_REQUEST_P_MFB_CONFIGURATION
{
    UINT_8E                     shortPressPreBootConfig;
    UINT_8E                     shortPressPostBootConfig;
    UINT_8E                     longPressPreBootConfig;
    UINT_8E                     longPressPostBootConfig;
}
IPMI_REQUEST_P_MFB_CONFIGURATION, *PIPMI_REQUEST_P_MFB_CONFIGURATION;

//++
// Name:
//      IPMI_RESPONSE_P_MFB_CONFIGURATION
//
// Description:
//      'Get MFB Configuration' command data buffer
//       for Phobos BMC
//--
typedef struct _IPMI_RESPONSE_P_MFB_CONFIGURATION
{
    UINT_8E                     shortPressPreBootConfig;
    UINT_8E                     shortPressPostBootConfig;
    UINT_8E                     longPressPreBootConfig;
    UINT_8E                     longPressPostBootConfig;
}
IPMI_RESPONSE_P_MFB_CONFIGURATION, *PIPMI_RESPONSE_P_MFB_CONFIGURATION;

//++
// Name:
//      IPMI_I2C_BUS_ID
//
// Description:
//      I2C Bus IDs
//--
typedef enum _IPMI_I2C_BUS_ID
{
   IPMI_I2C_BUS_ID_0          = 0x00,
   IPMI_I2C_BUS_ID_1          = 0x01,
   IPMI_I2C_BUS_ID_2          = 0x02,
   IPMI_I2C_BUS_ID_3          = 0x03,
   IPMI_I2C_BUS_ID_4          = 0x04,
   IPMI_I2C_BUS_ID_5          = 0x05,
   IPMI_I2C_BUS_ID_6          = 0x06,
   IPMI_I2C_BUS_ID_7          = 0x07,
   IPMI_I2C_BUS_ID_INVALID    = 0xFF
} IPMI_I2C_BUS_ID;

//++
// Name:
//      IPMI_I2C_BUS_POLLING_STATE
//
// Description:
//      IPMI I2C BUS POLLING STATE.
//
//-
typedef enum _IPMI_I2C_BUS_POLLING_STATE
{
    IPMI_I2C_BUS_POLLING_DISABLED         = 0,
    IPMI_I2C_BUS_POLLING_ENABLED          = 1,
    IPMI_I2C_BUS_POLLING_INVALID          = 0xFF
}
IPMI_I2C_BUS_POLLING_STATE, *PIPMI_I2C_BUS_POLLING_STATE;

//++
// Name:
//      IPMI_REQUEST_CONFIGURE_BMC_POLLING
//
// Description:
//      'Configure BMC polling' command data buffer.
//--
typedef struct _IPMI_REQUEST_CONFIGURE_BMC_POLLING
{
    UINT_8E                     i2cBusNum;
    UINT_8E                     enable;
}
IPMI_REQUEST_CONFIGURE_BMC_POLLING, *PIPMI_REQUEST_CONFIGURE_BMC_POLLING;

//++
// Name:
//      IPMI_RESPONSE_GET_BIST_REPORT
//
// Description:
//      Data portion of IPMI response for 'Get BIST Report' command.
//--
typedef struct _IPMI_RESPONSE_GET_BIST_REPORT
{
    UCHAR             cpuTest;
    UCHAR             dramTest;
    UCHAR             sramTest;
    UCHAR             i2cTest;
    UCHAR             fan0Test1;
    UCHAR             fan0Test2;
    UCHAR             fan0Test3;
    UCHAR             fan1Test1;
    UCHAR             fan1Test2;
    UCHAR             fan1Test3;
    UCHAR             fan2Test1;
    UCHAR             fan2Test2;
    UCHAR             fan2Test3;
    UCHAR             fan3Test1;
    UCHAR             fan3Test2;
    UCHAR             fan3Test3;
    UCHAR             fan4Test1;
    UCHAR             fan4Test2;
    UCHAR             fan4Test3;
    UCHAR             superCapTest;
    UCHAR             bbuTest;
    UCHAR             sspTest;
    UCHAR             uart2Test;
    UCHAR             nvramTest;
    UCHAR             usbTest;
    UCHAR             sgpioTest;
    UCHAR             bobTest;
    UCHAR             uart3Test;
    UCHAR             uart4Test;
    UCHAR             virtUart0Test;
    UCHAR             virtUart1Test;
}IPMI_RESPONSE_GET_BIST_REPORT, *PIPMI_RESPONSE_GET_BIST_REPORT;

//++
// Name:
//      IPMI_P_BIST_TEST_ID
//
// Description:
//      A List of all BIST Test IDs for Phobos BMC.
//--
typedef enum _IPMI_P_BIST_TEST_ID
{
   IPMI_P_BIST_CPU_TEST         = 0x00,
   IPMI_P_BIST_DRAM_TEST        = 0x01,
   IPMI_P_BIST_SRAM_TEST        = 0x02,
   IPMI_P_BIST_I2C_0_TEST       = 0x03,
   IPMI_P_BIST_I2C_1_TEST       = 0x04,
   IPMI_P_BIST_I2C_2_TEST       = 0x05,
   IPMI_P_BIST_I2C_3_TEST       = 0x06,
   IPMI_P_BIST_I2C_4_TEST       = 0x07,
   IPMI_P_BIST_I2C_5_TEST       = 0x08,
   IPMI_P_BIST_I2C_6_TEST       = 0x09,
   IPMI_P_BIST_I2C_7_TEST       = 0x0A,
   IPMI_P_BIST_UART_2_TEST      = 0x0B,
   IPMI_P_BIST_UART_3_TEST      = 0x0C,
   IPMI_P_BIST_UART_4_TEST      = 0x0D,
   IPMI_P_BIST_SSP_TEST         = 0x0E,
   IPMI_P_BIST_BBU_0_TEST       = 0x0F,
   IPMI_P_BIST_BBU_1_TEST       = 0x10,
   IPMI_P_BIST_BBU_2_TEST       = 0x11,
   IPMI_P_BIST_BBU_3_TEST       = 0x12,
   IPMI_P_BIST_NVRAM_TEST       = 0x13,
   IPMI_P_BIST_SGPIO_TEST       = 0x14,
   IPMI_P_BIST_FAN_0_TEST       = 0x15,
   IPMI_P_BIST_FAN_1_TEST       = 0x16,
   IPMI_P_BIST_FAN_2_TEST       = 0x17,
   IPMI_P_BIST_FAN_3_TEST       = 0x18,
   IPMI_P_BIST_FAN_4_TEST       = 0x19,
   IPMI_P_BIST_FAN_5_TEST       = 0x1A,
   IPMI_P_BIST_FAN_6_TEST       = 0x1B,
   IPMI_P_BIST_FAN_7_TEST       = 0x1C,
   IPMI_P_BIST_FAN_8_TEST       = 0x1D,
   IPMI_P_BIST_FAN_9_TEST       = 0x1E,
   IPMI_P_BIST_FAN_10_TEST      = 0x1F,
   IPMI_P_BIST_FAN_11_TEST      = 0x20,
   IPMI_P_BIST_FAN_12_TEST      = 0x21,
   IPMI_P_BIST_FAN_13_TEST      = 0x22,
   IPMI_P_BIST_ARB_0_TEST       = 0x23,
   IPMI_P_BIST_ARB_1_TEST       = 0x24,
   IPMI_P_BIST_ARB_2_TEST       = 0x25,
   IPMI_P_BIST_ARB_3_TEST       = 0x26,
   IPMI_P_BIST_ARB_4_TEST       = 0x27,
   IPMI_P_BIST_ARB_5_TEST       = 0x28,
   IPMI_P_BIST_ARB_6_TEST       = 0x29,
   IPMI_P_BIST_ARB_7_TEST       = 0x2A,
   IPMI_P_BIST_NCSI_LAN_TEST    = 0x2B,
   IPMI_P_BIST_DEDICATED_LAN_TEST = 0x2C
}IPMI_P_BIST_TEST_ID;

//++
// Name:
//      IPMI_P_BIST_CPU_TEST_REG
//
// Description:
//      Defines the BIST CPU test register
//      for Phobos BMC.
//--
typedef struct _IPMI_P_BIST_CPU_TEST_REG
{
    UINT_8E     cpu_cache_test:1,
                cpu_register_test:1,
                reserved:6;
}IPMI_P_BIST_CPU_TEST_REG;

typedef union _IPMI_P_BIST_CPU_TEST_RAW
{
    UINT_8E                     testConfig;
    IPMI_P_BIST_CPU_TEST_REG    fields;
}IPMI_P_BIST_CPU_TEST_RAW;

//++
// Name:
//      IPMI_P_BIST_RAM_TEST_REG
//
// Description:
//      Defines the BIST RAM test register
//      for Phobos BMC.
//--
typedef struct _IPMI_P_BIST_RAM_TEST_REG
{
    UINT_8E     address_line_test:1,
                data_line_test:1,
                short_mem_test:1,
                long_mem_test:1,
                reserved:4;
}IPMI_P_BIST_RAM_TEST_REG;

typedef union _IPMI_P_BIST_RAM_TEST_RAW
{
    UINT_8E                     testConfig;
    IPMI_P_BIST_RAM_TEST_REG    fields;
}IPMI_P_BIST_RAM_TEST_RAW;

//++
// Name:
//      IPMI_P_BIST_I2C_TEST_REG
//
// Description:
//      Defines the BIST I2C test register
//      for Phobos BMC.
//--
typedef struct _IPMI_P_BIST_I2C_TEST_REG
{
    UINT_8E     ctlr_access_test:1,
                reserved:7;
}IPMI_P_BIST_I2C_TEST_REG;

typedef union _IPMI_P_BIST_I2C_TEST_RAW
{
    UINT_8E                     testConfig;
    IPMI_P_BIST_I2C_TEST_REG    fields;
}IPMI_P_BIST_I2C_TEST_RAW;

//++
// Name:
//      IPMI_P_BIST_UART_TEST_REG
//
// Description:
//      Defines the BIST UART test register
//      for Phobos BMC.
//--
typedef struct _IPMI_P_BIST_UART_TEST_REG
{
    UINT_8E     uart_loopback_test:1,
                reserved:7;
}IPMI_P_BIST_UART_TEST_REG;

typedef union _IPMI_P_BIST_UART_TEST_RAW
{
    UINT_8E                     testConfig;
    IPMI_P_BIST_UART_TEST_REG   fields;
}IPMI_P_BIST_UART_TEST_RAW;

//++
// Name:
//      IPMI_P_BIST_SSP_TEST_REG
//
// Description:
//      Defines the BIST SSP test register
//      for Phobos BMC.
//--
typedef struct _IPMI_P_BIST_SSP_TEST_REG
{
    UINT_8E     ssp_internal_bist:1,
                reserved:7;
}IPMI_P_BIST_SSP_TEST_REG;

typedef union _IPMI_P_BIST_SSP_TEST_RAW
{
    UINT_8E                     testConfig;
    IPMI_P_BIST_SSP_TEST_REG    fields;
}IPMI_P_BIST_SSP_TEST_RAW;

//++
// Name:
//      IPMI_P_BIST_BBU_TEST_REG
//
// Description:
//      Defines the BIST BBU test register
//      for Phobos BMC.
//--
typedef struct _IPMI_P_BIST_BBU_TEST_REG
{
    UINT_8E     battery_self_test:1,
                reserved:7;
}IPMI_P_BIST_BBU_TEST_REG;

typedef union _IPMI_P_BIST_BBU_TEST_RAW
{
    UINT_8E                     testConfig;
    IPMI_P_BIST_BBU_TEST_REG    fields;
}IPMI_P_BIST_BBU_TEST_RAW, *PIPMI_P_BIST_BBU_TEST_RAW;

//++
// Name:
//      IPMI_P_BIST_NVRAM_TEST_REG
//
// Description:
//      Defines the BIST NVRAM test register
//      for Phobos BMC.
//--
typedef struct _IPMI_P_BIST_NVRAM_TEST_REG
{
    UINT_8E     sequential_test:1,
                random_test:1,
                reserved:6;
}IPMI_P_BIST_NVRAM_TEST_REG;

typedef union _IPMI_P_BIST_NVRAM_TEST_RAW
{
    UINT_8E                     testConfig;
    IPMI_P_BIST_NVRAM_TEST_REG  fields;
}IPMI_P_BIST_NVRAM_TEST_RAW;

//++
// Name:
//      IPMI_P_BIST_SGPIO_TEST_REG
//
// Description:
//      Defines the BIST SGPIO test register
//      for Phobos BMC.
//--
typedef struct _IPMI_P_BIST_SGPIO_TEST_REG
{
    UINT_8E     ssp_sgpio_test:1,
                reserved:7;
}IPMI_P_BIST_SGPIO_TEST_REG;

typedef union _IPMI_P_BIST_SGPIO_TEST_RAW
{
    UINT_8E                     testConfig;
    IPMI_P_BIST_SGPIO_TEST_REG  fields;
}IPMI_P_BIST_SGPIO_TEST_RAW;

//++
// Name:
//      IPMI_P_BIST_FAN_TEST_REG
//
// Description:
//      Defines the BIST FAN test register
//      for Phobos BMC.
//--
typedef struct _IPMI_P_BIST_FAN_TEST_REG
{
    UINT_8E     max_speed_test:1,
                reserved:7;
}IPMI_P_BIST_FAN_TEST_REG;

typedef union _IPMI_P_BIST_FAN_TEST_RAW
{
    UINT_8E                     testConfig;
    IPMI_P_BIST_FAN_TEST_REG    fields;
}IPMI_P_BIST_FAN_TEST_RAW;

//++
// Name:
//      IPMI_P_BIST_ARB_TEST_REG
//
// Description:
//      Defines the BIST ARB test register
//      for Phobos BMC.
//--
typedef struct _IPMI_P_BIST_ARB_TEST_REG
{
    UINT_8E     arbitration_test:1,
                reserved:7;
}IPMI_P_BIST_ARB_TEST_REG;

typedef union _IPMI_P_BIST_ARB_TEST_RAW
{
    UINT_8E                     testConfig;
    IPMI_P_BIST_ARB_TEST_REG    fields;
}IPMI_P_BIST_ARB_TEST_RAW;

//++
// Name:
//      IPMI_P_NCSI_LAN_TEST_REG
//
// Description:
//      Defines the BIST NCSI LAN test register
//      for Phobos BMC.
//--
typedef struct _IPMI_P_NCSI_LAN_TEST_REG
{
    UINT_8E     phy_connectivity_test:1,
                reserved:7;
}IPMI_P_NCSI_LAN_TEST_REG;

typedef union _IPMI_P_NCSI_LAN_TEST_RAW
{
    UINT_8E                     testConfig;
    IPMI_P_NCSI_LAN_TEST_REG    fields;
}IPMI_P_NCSI_LAN_TEST_RAW;

//++
// Name:
//      IPMI_P_DEDICATED_LAN_TEST_REG
//
// Description:
//      Defines the BIST DEDICATED LAN test register
//      for Phobos BMC.
//--
typedef struct _IPMI_P_DEDICATED_LAN_TEST_REG
{
    UINT_8E     phy_connectivity_test:1,
                phy_loopback_test:1,
                reserved:6;
}IPMI_P_DEDICATED_LAN_TEST_REG;

typedef union _IPMI_P_DEDICATED_LAN_TEST_RAW
{
    UINT_8E                     testConfig;
    IPMI_P_DEDICATED_LAN_TEST_REG    fields;
}IPMI_P_DEDICATED_LAN_TEST_RAW;


//++
// Name:
//      IPMI_P_BIST_LIST
//
// Description:
//      A List of all BIST commands, used in multiple commands
//      for Phobos BMC.
//--
typedef struct _IPMI_P_BIST_LIST
{
    IPMI_P_BIST_CPU_TEST_RAW        cpuTest;
    IPMI_P_BIST_RAM_TEST_RAW        dramTest;
    IPMI_P_BIST_RAM_TEST_RAW        sramTest;
    IPMI_P_BIST_I2C_TEST_RAW        i2c0Test;
    IPMI_P_BIST_I2C_TEST_RAW        i2c1Test;
    IPMI_P_BIST_I2C_TEST_RAW        i2c2Test;
    IPMI_P_BIST_I2C_TEST_RAW        i2c3Test;
    IPMI_P_BIST_I2C_TEST_RAW        i2c4Test;
    IPMI_P_BIST_I2C_TEST_RAW        i2c5Test;
    IPMI_P_BIST_I2C_TEST_RAW        i2c6Test;
    IPMI_P_BIST_I2C_TEST_RAW        i2c7Test;
    IPMI_P_BIST_UART_TEST_RAW       uart2Test;
    IPMI_P_BIST_UART_TEST_RAW       uart3Test;
    IPMI_P_BIST_UART_TEST_RAW       uart4Test;
    IPMI_P_BIST_SSP_TEST_RAW        sspTest;
    IPMI_P_BIST_BBU_TEST_RAW        bbu0Test;
    IPMI_P_BIST_BBU_TEST_RAW        bbu1Test;
    IPMI_P_BIST_BBU_TEST_RAW        bbu2Test;
    IPMI_P_BIST_BBU_TEST_RAW        bbu3Test;
    IPMI_P_BIST_NVRAM_TEST_RAW      nvramTest;
    IPMI_P_BIST_SGPIO_TEST_RAW      sgpioTest;
    IPMI_P_BIST_FAN_TEST_RAW        fan0Test;
    IPMI_P_BIST_FAN_TEST_RAW        fan1Test;
    IPMI_P_BIST_FAN_TEST_RAW        fan2Test;
    IPMI_P_BIST_FAN_TEST_RAW        fan3Test;
    IPMI_P_BIST_FAN_TEST_RAW        fan4Test;
    IPMI_P_BIST_FAN_TEST_RAW        fan5Test;
    IPMI_P_BIST_FAN_TEST_RAW        fan6Test;
    IPMI_P_BIST_FAN_TEST_RAW        fan7Test;
    IPMI_P_BIST_FAN_TEST_RAW        fan8Test;
    IPMI_P_BIST_FAN_TEST_RAW        fan9Test;
    IPMI_P_BIST_FAN_TEST_RAW        fan10Test;
    IPMI_P_BIST_FAN_TEST_RAW        fan11Test;
    IPMI_P_BIST_FAN_TEST_RAW        fan12Test;
    IPMI_P_BIST_FAN_TEST_RAW        fan13Test;
    IPMI_P_BIST_ARB_TEST_RAW        arb0Test;
    IPMI_P_BIST_ARB_TEST_RAW        arb1Test;
    IPMI_P_BIST_ARB_TEST_RAW        arb2Test;
    IPMI_P_BIST_ARB_TEST_RAW        arb3Test;
    IPMI_P_BIST_ARB_TEST_RAW        arb4Test;
    IPMI_P_BIST_ARB_TEST_RAW        arb5Test;
    IPMI_P_BIST_ARB_TEST_RAW        arb6Test;
    IPMI_P_BIST_ARB_TEST_RAW        arb7Test;
    IPMI_P_NCSI_LAN_TEST_RAW        ncsiLanTest;
    IPMI_P_DEDICATED_LAN_TEST_RAW   dedicatedLanTest;
}IPMI_P_BIST_LIST, *PIPMI_P_BIST_LIST;

//++
// Name:
//      IPMI_REQUEST_P_INITIATE_BIST
//
// Description:
//      Data portion of IPMI request for 'Get BIST Result' command.
//      for Phobos BMC
//--
typedef struct _IPMI_REQUEST_P_INITIATE_BIST
{
    IPMI_P_BIST_LIST    bistConfig;
}IPMI_REQUEST_P_INITIATE_BIST, *PIPMI_REQUEST_P_INITIATE_BIST;

//++
// Name:
//      IPMI_P_BIST_TEST_STATUS
//
// Description:
//      BIST Test status for Phobos BMC
//--
typedef enum _IPMI_P_BIST_TEST_STATUS
{
   IPMI_P_BIST_TEST_CLEARED             = 0x00,
   IPMI_P_BIST_TEST_COMPLETE            = 0x01,
   IPMI_P_BIST_TEST_IN_PROGRESS         = 0x02,
   IPMI_P_BIST_TEST_SKIPPED             = 0x03,
   IPMI_P_BIST_TEST_NOT_SUPPORTED       = 0x04,
   IPMI_P_BIST_TEST_FAILED_TO_EXECUTE   = 0x05
}IPMI_P_BIST_TEST_STATUS, *PIPMI_P_BIST_TEST_STATUS;

//++
// Name:
//      IPMI_RESPONSE_P_GET_BIST_RESULT
//
// Description:
//      Data portion of IPMI response for 'Get BIST Result' command.
//      for Phobos BMC
//--
typedef struct _IPMI_RESPONSE_P_GET_BIST_RESULT
{
    UINT_8E             testStatus;
    UINT_8E             testErrorMask;
    UINT_32E            testErrorCode;
    UINT_32E            testStartTime;
    UINT_32E            testEndTime;
}IPMI_RESPONSE_P_GET_BIST_RESULT, *PIPMI_RESPONSE_P_GET_BIST_RESULT;

//++
// Name:
//      IPMI_REQUEST_BMC_PEER_CACHE_UPDATE
//
// Description:
//      'Peer Cache Update' command data buffer.
//--
typedef struct _IPMI_REQUEST_BMC_PEER_CACHE_UPDATE
{
    UINT_8E                     fruid;
}
IPMI_REQUEST_BMC_PEER_CACHE_UPDATE, *PIPMI_REQUEST_BMC_PEER_CACHE_UPDATE;

//++
// Name:
//      IPMI_FIRMWARE_INTERFACE_TYPE
//
// Description:
//      Firmware types for SPI flash
//--
typedef enum _IPMI_FIRMWARE_INTERFACE_TYPE
{
   IPMI_FIRMWARE_INTERFACE_TYPE_SPI     = 0x00,
   IPMI_FIRMWARE_INTERFACE_TYPE_I2C     = 0x01,
   IPMI_FIRMWARE_INTERFACE_TYPE_INVALID = 0xFF
} IPMI_FIRMWARE_INTERFACE_TYPE;

//++
// Name:
//      IPMI_SPI_FIRMWARE_TYPE
//
// Description:
//      Firmware types for SPI flash
//--
typedef enum _IPMI_SPI_FIRMWARE_TYPE
{
   IPMI_SPI_FIRMWARE_TYPE_BMC_MAIN_APP              = 0x00,
   IPMI_SPI_FIRMWARE_TYPE_BMC_SSP                   = 0x01,
   IPMI_SPI_FIRMWARE_TYPE_BMC_BOOT                  = 0x02,
   IPMI_SPI_FIRMWARE_TYPE_BMC_PRODUCT_CONFIG_TABLE  = 0x03,
   IPMI_SPI_FIRMWARE_TYPE_EMC_APP                   = 0x04,
   IPMI_SPI_FIRMWARE_TYPE_BMC_APP_REDUNDANT         = 0x05,
   IPMI_SPI_FIRMWARE_TYPE_INVALID                   = 0xFF
} IPMI_SPI_FIRMWARE_TYPE;

//++
// Name:
//      IPMI_REQUEST_GET_FW_UPDATE_STATUS
//
// Description:
//      'Get Device revision and Firmware Update Status' command data buffer.
//--
typedef struct _IPMI_REQUEST_GET_FW_UPDATE_STATUS
{
    UCHAR       interfaceType;                  // SPI or I2C
    UCHAR       busNumber;                      // I2C port on the BMC that goes to the target, 00 for SPI
    UCHAR       slaveAddress;                   // SPI fw type or I2C address
    UCHAR       i2cAddrOfSwitch0;               // Valid for I2C target only
    UCHAR       portNumOnSwitch0;               // Valid for I2C target only
    UCHAR       i2cAddrOfSwitch1;               // Valid for I2C target only
    UCHAR       portNumOnSwitch1;               // Valid for I2C target only
    UCHAR       i2cAddrOfSwitch2;               // Valid for I2C target only
    UCHAR       portNumOnSwitch2;               // Valid for I2C target only
    UCHAR       i2cAddrOfSwitch3;               // Valid for I2C target only
    UCHAR       portNumOnSwitch3;               // Valid for I2C target only
    UCHAR       i2cAddrOfSwitch4;               // Valid for I2C target only
    UCHAR       portNumOnSwitch4;               // Valid for I2C target only
    UCHAR       i2cAddrOfSwitch5;               // Valid for I2C target only
    UCHAR       portNumOnSwitch5;               // Valid for I2C target only
    UCHAR       i2cAddrOfSwitch6;               // Valid for I2C target only
    UCHAR       portNumOnSwitch6;               // Valid for I2C target only
    UCHAR       i2cAddrOfSwitch7;               // Valid for I2C target only
    UCHAR       portNumOnSwitch7;               // Valid for I2C target only
}
IPMI_REQUEST_GET_FW_UPDATE_STATUS, *PIPMI_REQUEST_GET_FW_UPDATE_STATUS;

#define IPMI_GET_FW_UPDATE_STATUS_DEV_NOT_PRESENT_ERROR    0x03
#define IPMI_GET_FW_UPDATE_STATUS_BUS_ERROR_ON_REV_READ    0x04
//++
// Name:
//      IPMI_RESPONSE_GET_FW_UPDATE_STATUS
//
// Description:
//      Data portion of IPMI response for 'Get Device revision and Firmware Update Status' command.
//--
typedef struct _IPMI_RESPONSE_GET_FW_UPDATE_STATUS
{
    USHORT             lastUpdateImageID;
    UCHAR              lastUpdateResult;
    UCHAR              currentMajorVersion;
    UCHAR              currentMinorVersion;
    UCHAR              currentConfigVersion; //this byte is not returned if the device does not have more than 2 bytes for a revision.
    UCHAR              currentBootBlockVersion; //this byte is not returned if the device does not have more than 2 bytes for a revision.
}
IPMI_RESPONSE_GET_FW_UPDATE_STATUS, *PIPMI_RESPONSE_GET_FW_UPDATE_STATUS;

//++
// Name:
//      IPMI_REQUEST_GET_FW_REVISION
//
// Description:
//      'Get Firmware Revision' command data buffer.
//--
typedef struct _IPMI_REQUEST_GET_FW_REVISION
{
    UCHAR       fruID;
    UCHAR       deviceID;
    UCHAR       index;
}

IPMI_REQUEST_GET_FW_REVISION, *PIPMI_REQUEST_GET_FW_REVISION;

//++
// Name:
//      IPMI_RESPONSE_GET_FW_REVISION
//
// Description:
//      Data portion of IPMI response for 'Get Firmware Revision' command.
//--
typedef struct _IPMI_RESPONSE_GET_FW_REVISION
{
    UCHAR              currentMajorVersion;
    UCHAR              currentMinorVersion;
    UCHAR              currentConfigVersion; //this byte is not returned if the device does not have more than 2 bytes for a revision.
    UCHAR              currentBootBlockVersion; //this byte is not returned if the device does not have more than 2 bytes for a revision.
}
IPMI_RESPONSE_GET_FW_REVISION, *PIPMI_RESPONSE_GET_FW_REVISION;

//++
// Name:
//      IPMI_RESPONSE_GET_CHASSIS_STATUS
//
// Description:
//      Table 28-3, Get Chassis Status Command from IPMI spec
//--

//++
// Name:
//      IPMI_CHASSIS_POWER_RESTORE_POLICY
//
// Description:
//      power restore policy for chassis
//--
typedef enum _IPMI_CHASSIS_POWER_RESTORE_POLICY
{
   IPMI_POWER_RESTORE_STAY_OFF      = 0x0,
   IPMI_POWER_RESTORE_LAST_STATE    = 0x1,
   IPMI_POWER_RESTORE_POWER_UP      = 0x2,
   IPMI_POWER_RESTORE_UNKNOWN       = 0x3,
} IPMI_CHASSIS_POWER_RESTORE_POLICY;

typedef struct _CHASSIS_STATUS_REG_B0_B7
{
    UINT_8E     power_on:1,
                power_overload:1,
                chassis_interlock:1,
                power_fault:1,
                power_control_fault:1,
                power_restore_policy:1,
                reserved:1;
} CHASSIS_STATUS_REG_B0_B7;

typedef union _CHASSIS_STATUS_RAW_INFO_B0_B7
{
    UINT_8E                     StatusControl;
    CHASSIS_STATUS_REG_B0_B7    fields;
} CHASSIS_STATUS_RAW_INFO_B0_B7;

typedef struct _CHASSIS_STATUS_REG_B8_B15
{
    UINT_8E     ac_failed:1,
                power_overload:1,
                chassis_interlock:1,
                power_fault:1,
                powerup_over_ipmi:1,
                reserved:3;
} CHASSIS_STATUS_REG_B8_B15;

typedef union _CHASSIS_STATUS_RAW_INFO_B8_B15
{
    UINT_8E                     StatusControl;
    CHASSIS_STATUS_REG_B8_B15   fields;
} CHASSIS_STATUS_RAW_INFO_B8_B15;

//++
// Name:
//      IPMI_CHASSIS_IDENTIFY_STATE
//
// Description:
//      chassis identify state values
//--
typedef enum _IPMI_CHASSIS_IDENTIFY_STATE
{
   IPMI_CHASSIS_IDENTIFY_OFF        = 0x0,
   IPMI_CHASSIS_IDENTIFY_TEMP_ON    = 0x1,
   IPMI_CHASSIS_IDENTIFY_ON         = 0x2,
   IPMI_CHASSIS_IDENTIFY_RESERVED   = 0x3,
} IPMI_CHASSIS_IDENTIFY_STATE;

typedef struct _CHASSIS_STATUS_REG_B16_B23
{
    UINT_8E     chassis_intrusion:1,
                front_panel_lockout:1,
                drive_fault:1,
                cooling_fault:1,
                chassis_identify_state:2,
                chassis_identify_supported:1,
                reserved:1;
} CHASSIS_STATUS_REG_B16_B23;

typedef union _CHASSIS_STATUS_RAW_INFO_B16_B23
{
    UINT_8E                     StatusControl;
    CHASSIS_STATUS_REG_B16_B23  fields;
} CHASSIS_STATUS_RAW_INFO_B16_B23;

//++
// Name:
//      IPMI_RESPONSE_GET_CHASSIS_STATUS
//
// Description:
//      Data portion of IPMI response for 'Get Chassis Status'.
//--
typedef struct _IPMI_RESPONSE_GET_CHASSIS_STATUS
{
    CHASSIS_STATUS_RAW_INFO_B0_B7       currentState;
    CHASSIS_STATUS_RAW_INFO_B8_B15      lastEvent;
    CHASSIS_STATUS_RAW_INFO_B16_B23     miscState;
}
IPMI_RESPONSE_GET_CHASSIS_STATUS, *PIPMI_RESPONSE_GET_CHASSIS_STATUS;


//++
// Name:
//      IPMI_REQUEST_CHASSIS_CONTROL
//
// Description:
//      Table 28-4, Chassis Control Command from IPMI spec
//--

//++
// Name:
//      IPMI_CHASSIS_CONTROL_ACTION
//
// Description:
//      Data to send for a chassis control command
//--
typedef enum _IPMI_CHASSIS_CONTROL_ACTION
{
   IPMI_CHASSIS_CONTROL_POWER_DOWN      = 0x0,
   IPMI_CHASSIS_CONTROL_POWER_UP        = 0x1,
   IPMI_CHASSIS_CONTROL_POWER_CYCLE     = 0x2,
   IPMI_CHASSIS_CONTROL_HARD_RESET      = 0x3,
   IPMI_CHASSIS_CONTROL_DIAG_INTERRUPT  = 0x4,// not supported by current BMC FW
   IPMI_CHASSIS_CONTROL_SOFT_SHUTDOWN   = 0x5,// optional, simulated through fatal overtemp (unsupported w/ current FW)
} IPMI_CHASSIS_CONTROL_ACTION;

typedef struct _CHASSIS_CONTROL_REG_B0_B7
{
    UINT_8E     chassis_control:4,
                reserved:4;
} CHASSIS_CONTROL_REG_B0_B7;

typedef union _CHASSIS_CONTROL_RAW_INFO_B0_B7
{
    UINT_8E                     StatusControl;
    CHASSIS_CONTROL_REG_B0_B7   fields;
} CHASSIS_CONTROL_RAW_INFO_B0_B7;

//++
// Name:
//      IPMI_REQUEST_CHASSIS_CONTROL
//
// Description:
//      Data portion of IPMI request for 'Chassis Control'.
//--
typedef struct _IPMI_REQUEST_CHASSIS_CONTROL
{
    CHASSIS_CONTROL_RAW_INFO_B0_B7  chassis_control;
}
IPMI_REQUEST_CHASSIS_CONTROL, *PIPMI_REQUEST_CHASSIS_CONTROL;



//++
// Name:
//      IPMI_RAW_I2C_REQUEST_STATUS
//
// Description:
//      Status of the chained Raw I2C request
//--
typedef enum _IPMI_RAW_I2C_REQUEST_STATUS
{
   IPMI_RAW_I2C_REQUEST_SUCCESS                     = 0x00,
   IPMI_RAW_I2C_REQUEST_DRIVER_OPEN_ERROR           = 0x03,
   IPMI_RAW_I2C_REQUEST_DRIVER_TRANSACTION_ERROR    = 0x06,
   IPMI_RAW_I2C_REQUEST_DEVICE_NACK_ERROR           = 0x31,
   IPMI_RAW_I2C_REQUEST_IN_BAND_ARBITRATION_LOSS    = 0x32,
   IPMI_RAW_I2C_REQUEST_TRANSACTION_START_ERROR     = 0x33,
   IPMI_RAW_I2C_REQUEST_TRANSACTION_STOP_ERROR      = 0x34,
   IPMI_RAW_I2C_REQUEST_BUS_BUSY_ERROR              = 0x35,
   IPMI_RAW_I2C_REQUEST_TRANSACTION_TIMEOUT_ERROR   = 0x36,
   IPMI_RAW_I2C_REQUEST_TRANSACTION_ERROR           = 0x37,
   IPMI_RAW_I2C_REQUEST_BUSY_ERROR                  = 0x38,
   IPMI_RAW_I2C_REQUEST_BUFFER_OVERFLOW_ERROR       = 0x39,
   IPMI_RAW_I2C_REQUEST_BUS_RESET_ERROR             = 0x3A,
   IPMI_RAW_I2C_REQUEST_BUS_PROTOCOL_ERROR          = 0x3B,
   IPMI_RAW_I2C_REQUEST_BUS_SEGMENT_ERROR           = 0x3C,
   IPMI_RAW_I2C_REQUEST_STUCK_DATA_ERROR            = 0x3D,
   IPMI_RAW_I2C_REQUEST_STUCK_CLOCK_ERROR           = 0x3F,
   IPMI_RAW_I2C_REQUEST_PARAMETER_ERROR             = 0x40,
   IPMI_RAW_I2C_REQUEST_SYSTEM_ERROR                = 0x41
} IPMI_RAW_I2C_REQUEST_STATUS;

//++
// Name:
//      IPMI_P_RAW_I2C_REQUEST_STATUS
//
// Description:
//      Status of the chained Raw I2C request for Phobos
//--
typedef enum _IPMI_P_RAW_I2C_REQUEST_STATUS
{
   IPMI_P_RAW_I2C_REQUEST_SUCCESS                   = 0x00,
   IPMI_P_RAW_I2C_REQUEST_ARB_TIMEOUT               = 0x80, //Should not see this for the individual I2C request from the chained request.
   IPMI_P_RAW_I2C_REQUEST_ARB_STUCK                 = 0x81, //Should not see this for the individual I2C request from the chained request.
   IPMI_P_RAW_I2C_REQUEST_ARB_FREE_ERROR            = 0x82, //Should not see this for the individual I2C request from the chained request.
   IPMI_P_RAW_I2C_REQUEST_I2C_NACK                  = 0x83,
   IPMI_P_RAW_I2C_REQUEST_I2C_SCL_STUCK             = 0x84,
   IPMI_P_RAW_I2C_REQUEST_I2C_SDA_STUCK             = 0x85,
   IPMI_P_RAW_I2C_REQUEST_I2C_BAD_BUS_SEGMENT       = 0x86,
   IPMI_P_RAW_I2C_REQUEST_I2C_DRV_INIT_ERROR        = 0x87,
   IPMI_P_RAW_I2C_REQUEST_I2C_DRV_MISC_ERRORS       = 0x88,
   IPMI_P_RAW_I2C_REQUEST_I2C_QUEUE_FULL            = 0x89, //Should not see this for the individual I2C request from the chained request.
   IPMI_P_RAW_I2C_REQUEST_I2C_PKT_CKSUM_ERROR       = 0x90,
   IPMI_P_RAW_I2C_REQUEST_REGISTER_ECHO_ERROR       = 0x91,
   IPMI_P_RAW_I2C_REQUEST_DEVICE_ABSENT             = 0x98,
   IPMI_P_RAW_I2C_REQUEST_DEVICE_OFF                = 0x99
} IPMI_P_RAW_I2C_REQUEST_STATUS;


// BMC and IPMI specification:
// http://opseroom01.corp.emc.com/eRoomReq/Files/SPOsymmhwdev/Symmetrix2/0_ee584/Avocent%20IPMI%20OEM%20Command%20Specification.docx
// The wiki of Dual_IEER:
// https://teamforge6.usd.lab.emc.com/sf/wiki/do/viewPage/projects.c4cb_lx/wiki/DualIERR
//++
//  IPMI_SET_BMC_CMI_WD_Request
//--

typedef struct _IPMI_SET_BMC_CMI_WD_Request {
    UCHAR socket_id;     // always 0
    UCHAR offset   ;     // Offset (0x60, 0x64, 0x68,..., 0x9C); 0 = Do not modify
    UCHAR poll_rate;     // Poll rate (1 = 500ms, 2 = 1000ms, etc) (Max =10); 0 = Do not modify
    UCHAR timeout_value; // Timeout value (1 = 500ms, 2 = 1000ms, etc) (Max = 40); 0 = Do not modify
    UCHAR config_mask;   // Bit 1: Modify timeout (1 = modify); Bit 2: Modify PECI fault (1 = modify)
                         // Bit 4: Modify arm (1 = modify);  Bit 6: Modify enable (1 = modify)
    UCHAR config;        // Bit 1: Timeout; Bit 2: PECI Fault; Bit 4: Arm; Bit 6: Enable
} IPMI_SET_BMC_CMI_WD_Request, *PIPMI_SET_BMC_CMI_WD_Request;

// Poll rate (1 = 500ms, 2 = 1000ms, etc) (Max =10); 0 = Do not modify
#define BMC_WD_500_MSEC_POLL_RATE       1
#define BMC_WD_1000_MSEC_POLL_RATE      2
#define BMC_WD_1500_MSEC_POLL_RATE      3
#define BMC_WD_2000_MSEC_POLL_RATE      4 

// Timeout value (1 = 500ms, 2 = 1000ms, etc) (Max = 40); 0 = Do not modify
#define BMC_WD_500_MSEC_TIMEOUT         1
#define BMC_WD_1_SEC_TIMEOUT            2
#define BMC_WD_2_SEC_TIMEOUT            4
#define BMC_WD_3_SEC_TIMEOUT            6
#define BMC_WD_4_SEC_TIMEOUT            8
#define BMC_WD_5_SEC_TIMEOUT            10
#define BMC_WD_6_SEC_TIMEOUT            12

//++
//  IPMI_GET_BMC_CMI_WD_Request 
//--

typedef struct _IPMI_GET_IPMI_CMI_WD_Response {
//    UCHAR compltetion_code; // not in the response data section..
    UCHAR offset   ;     // Offset (0x60, 0x64, 0x68,..., 0x9C); 0 = Do not modify
    UCHAR poll_rate;     // Poll rate (1 = 500ms, 2 = 1000ms, etc) (Max =10); 0 = Do not modify
    UCHAR timeout_value; // Timeout value (1 = 500ms, 2 = 1000ms, etc) (Max = 40); 0 = Do not modify
    UCHAR status;        // Bit 0: Paused; Bit 1: Timeout; Bit 2: PECI Fault; 
                         // Bit 4: Armed; Bit 5: Running; Bit 6: Enabled; Bit 7: Available
    ULONG count;         // copy of scratchpad
} IPMI_GET_BMC_CMI_WD_Response, *PIPMI_GET_BMC_CMI_WD_Response;

#if defined(__cplusplus)
#pragma warning( push )
#pragma warning( disable : 4200 )
#endif
//++
// Name:
//      IPMI_INDIVIDUAL_RAW_I2C_REQUEST_HEADER
//
// Description:
//      Request header for each individual request in the IPMI_CMD_CHAINED_I2C_RAW command 
//--
typedef struct _IPMI_INDIVIDUAL_RAW_I2C_REQUEST_HEADER
{
    UCHAR       byteCount;                      // Number of bytes, not including this byte
    UCHAR       slaveAddress;                   // The I2C slave address to access
    UCHAR       bytesToRead;                    // The number of bytes to read back

    /********************************************
     **** Maintain as last element of struct ****
     ********************************************/
    UCHAR       writeData[0];                   // The data to write to the I2C device
}
IPMI_INDIVIDUAL_RAW_I2C_REQUEST_HEADER, *PIPMI_INDIVIDUAL_RAW_I2C_REQUEST_HEADER;

//++
// Name:
//      IPMI_RAW_I2C_REQUEST_HEADER
//
// Description:
//      Request header for the IPMI_CMD_CHAINED_I2C_RAW command 
//--
typedef struct _IPMI_RAW_I2C_REQUEST_HEADER
{
    UCHAR       busNumber;                      // I2C port on the BMC to use
    UCHAR       requestCount;                   // Number of individual requests in request

    /********************************************
     **** Maintain as last element of struct ****
     ********************************************/
    IPMI_INDIVIDUAL_RAW_I2C_REQUEST_HEADER  request;
}
IPMI_RAW_I2C_REQUEST_HEADER, *PIPMI_RAW_I2C_REQUEST_HEADER;

//++
// Name:
//      IPMI_INDIVIDUAL_RAW_I2C_RESPONSE_HEADER
//
// Description:
//      Response header for each individual response in the IPMI_CMD_CHAINED_I2C_RAW command 
//--
typedef struct _IPMI_INDIVIDUAL_RAW_I2C_RESPONSE_HEADER
{
    UCHAR       byteCount;                      // Number of bytes, not including this byte
    UCHAR       requestStatus;                  // Status of the individual request

    /********************************************
     **** Maintain as last element of struct ****
     ********************************************/
    UCHAR       readData[0];                    // The data read from the I2C device
}
IPMI_INDIVIDUAL_RAW_I2C_RESPONSE_HEADER, *PIPMI_INDIVIDUAL_RAW_I2C_RESPONSE_HEADER;

//++
// Name:
//      IPMI_RAW_I2C_RESPONSE_HEADER
//
// Description:
//      Response header for the IPMI_CMD_CHAINED_I2C_RAW command 
//--
typedef struct _IPMI_RAW_I2C_RESPONSE_HEADER
{
    UCHAR       responseCount;                   // Number of individual responses in response

    /********************************************
     **** Maintain as last element of struct ****
     ********************************************/
    IPMI_INDIVIDUAL_RAW_I2C_RESPONSE_HEADER response;
}
IPMI_RAW_I2C_RESPONSE_HEADER, *PIPMI_RAW_I2C_RESPONSE_HEADER;

//++
// Name:
//      IPMI_INTERFACE_REVIVE_INTERVAL_IN_MSECS
//
// Description:
//      The amount of time after the BMC interface is declared dead that a revive of
//      the interface will be attempted.  Until then, all requests to the local
//      interface will return with an error.  This value is currently set to 10 secs.
//      This is used for specifying wait times when IPMILib attempts retries of a
//      failed command.
//
//--
#define IPMI_INTERFACE_REVIVE_INTERVAL_IN_MSECS     5000

//++
// Name:
//      IPMI_REQUEST_LOG_NUM_DATA_BYTES
//
// Description:
//      The maximum bytes of data from the payload of an IPMI request that are logged.
//
// Revision History:
//      12-Feb-13   Sri             Created. 
//
//--
#define IPMI_REQUEST_LOG_NUM_DATA_BYTES     4

//++
// Name:
//      IPMI_REQUEST_TYPE
//
// Description:
//      Specifies whether an IPMI request is local (targeted to the local BMC) or 
//      intended for the peer BMC
//
// Revision History:
//      12-Dec-12   Windyon Zhou    Created.
//
//--
typedef enum _IPMI_REQUEST_TYPE 
{
    IPMI_REQUEST_TYPE_LOCAL = 0x0, 
    IPMI_REQUEST_TYPE_PEER,
    IPMI_REQUEST_TYPE_INVALID
} IPMI_REQUEST_TYPE, *PIPMI_REQUEST_TYPE;  

//
//  Whether to collect statistics or not
//
#define IPMI_STATS_COLLECTION

#ifdef IPMI_STATS_COLLECTION

//
// The max entry count in the response for a GETSTATS IOCTL request.
//
#define IPMI_STATS_REQUEST_ENTRIES_MAX  512 

//++
// Name:
//      IPMI_REQUEST_STATS_ENTRY
//
// Description:
//      A data type to collect stats about IPMI requests and responses, both local and peer.  
//      This tracks various time points in the life of an IPMI request within the driver.
//
// Revision History:
//      12-Dec-12   Windyon Zhou    Created.
//
//--
typedef struct _IPMI_REQUEST_STATS_ENTRY
{
    //
    // Timestamp to enter request queue
    //
    EMCPAL_TIME_USECS       timeEnqueue;
    //
    // Timestamp of starting process request, which is the KCS writing start time
    // of local requests or peer reqeust process
    //
    EMCPAL_TIME_USECS       timeStart;
    union _KCS_TIMESTAMPS{ 
        //
        // This datatype for timestamps used in local request process 
        // 
        struct _LOCAL_TIMESTAMPS 
        {
            //
            // Timestamp of KCS command processing start, which is regarded as the
            // time when sending the last byte of request to BMC.
            //
            EMCPAL_TIME_USECS       timeProc;
            //
            // Timestamp of KCS command processing start, which is time when getting 
            // first byte of response got from BMC.
            //
            EMCPAL_TIME_USECS       timeRead;
        } local;
        //
        // This datatype for timestamps used in peer request process 
        // 
        struct _PEER_TIMESTAMPS
        {
            //
            // Timestamp of KCS command processing start, which is regarded as the
            // time when sending the last byte of request of sent peer message to BMC.
            //
            EMCPAL_TIME_USECS       timeProc1;
            //
            // Timestamp of KCS read start, which is regarded as the time when
            // getting the first byte of response of sent peer message from BMC.
            //
            EMCPAL_TIME_USECS       timeRead1;
            //
            // Timestamp of the end of sending peer message.
            //
            EMCPAL_TIME_USECS       timeComp1;
            //
            // Timestamp of start of receiving peer message.
            //
            EMCPAL_TIME_USECS       timeWrite2;
            //
            // Timestamp of KCS command processing start of receiving peer message,
            // which is regarded as the time when sending the last byte of request of
            // receiving peer message to BMC.
            //
            EMCPAL_TIME_USECS       timeProc2;
            //
            // Timestamp of getting peer response KCS reading starting, regarded
            // as time of getting first byte of response of receiving peer message.
            //
            EMCPAL_TIME_USECS       timeRead2;
        } peer;
    } kcs;
    //
    // Timestamp of the end of processing a request. For peer requests, it equals
    // to the end of receiving peer message.
    // When this value is Non-Zero, we could assume that this stats entry is finished
    //
    EMCPAL_TIME_USECS       timeComp;
    //
    // The returned status of a IPMI request
    //
    EMCPAL_STATUS           status;
    //
    // The total spin wait times used in KCS writing for flags waiting 
    //
    ULONG32                 readSpinTimes;
    //
    // The total spin wait times used in KCS reading for flags waiting 
    //
    ULONG32                 writeSpinTimes;
    //
    // IPMI header of a request
    //
    IPMI_REQUEST_HEADER     header;
    //
    // IPMI request data length
    //
    USHORT                  reqLen;
    //
    // IPMI response data length
    //
    USHORT                  respLen;
    //
    // The total sleep wait times used in KCS reading for flags waiting 
    //
    USHORT                  readWaitTimes;
    //
    // The total sleep wait times used in KCS writing for flags waiting 
    //
    USHORT                  writeWaitTimes;
    //
    // Length of IPMI request queue when the request enqueue
    //
    USHORT                  curQLen;
    //
    // The completion code of a request
    //
    UCHAR                   CCcode;
    //
    // The completion code of a request
    //
    IPMI_REQUEST_TYPE       reqType;
    //
    // The data part of a request
    //
    UCHAR                   data[IPMI_REQUEST_LOG_NUM_DATA_BYTES];
} IPMI_REQUEST_STATS_ENTRY, *PIPMI_REQUEST_STATS_ENTRY;

//++
// Name:
//      IPMI_GLOBAL_STATS_ENTRY
//
// Description:
//      A data type to collect global stats for IPMI requests.
//
// Revision History:
//      27-Feb-13   Windyon Zhou    Created.
//
//--
typedef struct _IPMI_GLOBAL_STATS_ENTRY
{
    //
    // The time of IPMI stats collection starts.
    //
    EMCPAL_TIME_USECS   startTime;
    //
    // The time of IPMI stats collection end.
    //
    EMCPAL_TIME_USECS   endTime;
    //
    // Total local requests processed from stats collection starts.
    //
    ULONG               totalLocalRequests;
    //
    // Total peer requests processed from stats collection starts.
    //
    ULONG               totalPeerRequests;
    //
    // Total failed requests processed from stats collection starts.
    //
    ULONG               failedRequests;
    //
    // Total aborted requests processed from stats collection starts.
    //
    ULONG               abortRequests;
    //
    // Total time spent on all aborted requests from stats collection starts.
    //
    EMCPAL_TIME_USECS   abortSpentTime;
    //
    // Minimal process time spent on local requests from stats collection starts.
    //
    EMCPAL_TIME_USECS   minLocalRequestSpentTime;
    //
    // Maximal process time spent on local requests from stats collection starts.
    //
    EMCPAL_TIME_USECS   maxLocalRequestSpentTime;
    //
    // Minimal process time spent on peer requests from stats collection starts.
    //
    EMCPAL_TIME_USECS   minPeerRequestSpentTime;
    //
    // Maximal process time spent on peer requests from stats collection starts.
    //
    EMCPAL_TIME_USECS   maxPeerRequestSpentTime;
    //
    // Total time spent on all local requests from stats collection starts.
    //
    EMCPAL_TIME_USECS   totalLocalRequestSpentTime;
    //
    // Total time spent on all peer requests from stats collection starts.
    //
    EMCPAL_TIME_USECS   totalPeerRequestSpentTime;
} IPMI_GLOBAL_STATS_ENTRY, *PIPMI_GLOBAL_STATS_ENTRY;

//++
// Name:
//      IPMI_STATS_GET_STATS_IN_BUF
//
// Description:
//      This data type is the request (input) buffer for "Get Stats" IOCTL.
//
// Revision History:
//      27-Feb-13   Windyon Zhou    Created.
//
//--
typedef struct _IPMI_STATS_GET_STATS_IN_BUF
{
    //
    // The number of stats entries that client expects to get from stats buffer.
    //
    ULONG32 entryCount;
} IPMI_STATS_GET_STATS_IN_BUF, *PIPMI_STATS_GET_STATS_IN_BUF;

//++
// Name:
//      IPMI_STATS_STATUS_OUT_BUF
//
// Description:
//      This data type is the response (output) buffer for "Get Status" IOCTL.
//
// Revision History:
//      27-Feb-13   Windyon Zhou    Created.
//
//--
typedef struct _IPMI_STATS_STATUS_OUT_BUF
{
    //
    // If dynamic stat collection is enabled in driver.
    //
    BOOL            enabled;
    //
    // The amount of entries that the stats buffer contains.
    //
    ULONG32         totalEntryCount;
    //
    // The amount of entries in stats buffer that is currently not used.
    //
    ULONG32         freeEntryCount;
} IPMI_STATS_STATUS_OUT_BUF, *PIPMI_STATS_STATUS_OUT_BUF;

//++
// Name:
//      IPMI_STATS_GET_GLOBAL_STATS_OUT_BUF
// Description:
//      This data type is the response (output) buffer for "Get Global Stats" IOCTL.
//      Actually, it is the type of global stats entry.
//
// Revision History:
//      27-Feb-13   Windyon Zhou    Created.
//
//--
typedef IPMI_GLOBAL_STATS_ENTRY         IPMI_STATS_GET_GLOBAL_STATS_OUT_BUF;
typedef IPMI_GLOBAL_STATS_ENTRY *       PIPMI_STATS_GET_GLOBAL_STATS_OUT_BUF;

//++
// Name:
//      IPMI_STATS_GET_STATS_OUT_BUF
//
// Description:
//      This data type is the response (output) buffer for "Get Stats" IOCTL.
//
// Revision History:
//      27-Feb-13   Windyon Zhou    Created.
//
//--
typedef struct _IPMI_STATS_GET_STATS_OUT_BUF
{
    //
    // The amount of stats entries that the driver returns to client.
    //
    ULONG32         entryCount;
    //
    // The amount of entries in stats buffer that is currently not used.
    //
    ULONG32         freeEntryCount;
    //
    // The buffer that contains entries which the driver returns to client.
    //
    UCHAR           buffer[IPMI_STATS_REQUEST_ENTRIES_MAX * sizeof (IPMI_REQUEST_STATS_ENTRY)];
} IPMI_STATS_GET_STATS_OUT_BUF, *PIPMI_STATS_GET_STATS_OUT_BUF;

#endif // IPMI_STATS_COLLECTION

//++
// Name:
//      IPMI_ENABLE_SFI_IN_BUF
//
// Description:
//      Used as input buffer for the ENABLE_SFI ioctl, this data type specifies parameters
//      used to simulate timeouts for IPMI requests issued to the SAFE IPMI driver.
//
//      Usage Notes:  The IOCTL can be issued multiple times in succession to change 
//      any of the parameters of error injection; the last set of parameters will
//      take effect. 
//
//      Note that this buffer is overloaded: to disable SFI, the 'enableXXX' parameters 
//      should be FALSE.
//
// Revision History:
//      21-Aug-14   Sri             Created.
//
//--
typedef struct _IPMI_ENABLE_SFI_IN_BUF
{
    //
    // Boolean to indicate whether errors should be returned for local IPMI requests.
    //
    BOOL                    enableLocal;

    //
    // Boolean to indicate whether errors should be returned for peer IPMI requests.
    //
    BOOL                    enablePeer;

    //
    // Value to specify a duration, in seconds, for which error simulation should remain
    // active.
    //
    ULONG32                 durationInSeconds;

    //
    // An upper bound on the number of simulated faults returned.
    //
    ULONG32                 maxCount;

} IPMI_ENABLE_SFI_IN_BUF, *PIPMI_ENABLE_SFI_IN_BUF;

//++
// Name:
//      IPMI_GET_SFI_STATE_OUT_BUF
//
// Description:
//      The output buffer for the GET_SFI_STATE ioctl, this data type is used to return the
//      state of SFI in the SAFE IPMI driver.
//
//      Usage Notes:  The GET_SFI_STATE ioctl is used to obtain information about the state
//      of SFI in the SAFE IPMI driver.
//
// Revision History:
//      21-Aug-14   Sri             Created.
//
//--
typedef struct _IPMI_GET_SFI_STATE_OUT_BUF
{
    //
    // Current SFI parameters, as set in the driver.
    //
    struct _IPMI_ENABLE_SFI_IN_BUF      sfiParams;

    //
    // The number of local requests failed by the driver (so far).
    // 
    ULONG32                             localCount;

    //
    // The number of peer requests failed by the driver (so far).
    // 
    ULONG32                             peerCount;

} IPMI_GET_SFI_STATE_OUT_BUF, *PIPMI_GET_SFI_STATE_OUT_BUF;

#if defined(__cplusplus)
#pragma warning( pop )
#endif
#pragma pack(pop)

//++
//.End Data Structures
//--

//++
// IPMI Error Code Definitions
//--
#define IPMI_FACILITY_CODE                      0x234
#define IPMI_CUSTOM_CODE                        0xA

#define IPMI_SEVERITY_SUCCESS                   0x0
#define IPMI_SEVERITY_INFO                      0x6
#define IPMI_SEVERITY_WARN                      0xA
#define IPMI_SEVERITY_ERROR                     0xE

#define IPMI_BUILD_STATUS(_SEV_, _CODE_)        \
    ((EMCPAL_STATUS) (((_SEV_) << 28) | (IPMI_FACILITY_CODE << 16) | (IPMI_CUSTOM_CODE << 12) | (_CODE_)))

// IPMI Error codes
#define STATUS_IPMI_PEER_INTERFACE_DEAD         IPMI_BUILD_STATUS (IPMI_SEVERITY_ERROR, 0x001)
#define STATUS_IPMI_INTERFACE_DEAD              IPMI_BUILD_STATUS (IPMI_SEVERITY_ERROR, 0x002)

//++
//.End IPMI Error Code Definitions
//--

//++
// IPMI IOCTL Interface Definitions
//--

//++
// Name:
//      IPMI_API_REVISION_1
//
// Description:
//      Defines the revision number for the IPMI API, to be used by clients to set the
//      revision id field in IOCTL input buffers.
//
//-
#define IPMI_API_REVISION_1                 1

//++
// Name:
//      IPMI_API_REVISION_CURRENT
//
// Description:
//      Defines the revision number for the IPMI API that is currently in use.
//
//-
#define IPMI_API_REVISION_CURRENT           IPMI_API_REVISION_1

//++
// Name:
//      IPMI_XXX_DEVICE_NAME
//
// Description:
//      Defines the device object names/paths of the IPMI device.  Clients must open this
//      device to issue IOCTLs to the IPMI driver. The NT device name is used from kernel
//      space and the Win32 device name is used from user space.
//
//-
#define IPMI_BASE_DEVICE_NAME               "CLARiiONipmi"
#define IPMI_NT_DEVICE_NAME                 "\\Device\\" IPMI_BASE_DEVICE_NAME
#define IPMI_DOS_DEVICE_NAME                "\\??\\" IPMI_BASE_DEVICE_NAME
#define IPMI_WIN32_DEVICE_NAME              "\\\\.\\" IPMI_BASE_DEVICE_NAME

#define IPMI_BASE_DEVICE_NAME_CHAR          "IPMI"
#define IPMI_NT_DEVICE_NAME_CHAR            "\\Device\\" IPMI_BASE_DEVICE_NAME_CHAR
#define IPMI_DOS_DEVICE_NAME_CHAR           "\\??\\" IPMI_BASE_DEVICE_NAME_CHAR
#define IPMI_WIN32_DEVICE_NAME_CHAR         "\\\\.\\" IPMI_BASE_DEVICE_NAME_CHAR

#ifdef C4_INTEGRATED
#define IPMI_DEVICE_NAME                    "/dev/ipmi0"
#endif // C4_INTEGRATED


//++
// Name:
//      IPMI_IOCTL_BASE
//
// Description:
//      Function code base for IPMI ioctls.
//      XXX - picked arbitrarily.
//
//--
#define IPMI_IOCTL_BASE                     6789

//++
// Name:
//      IPMI_BUILD_IOCTL
//
// Description:
//      Macro to help build IPMI ioctl codes.
//
//--
#define IPMI_BUILD_IOCTL(X)                         \
    (EMCPAL_IOCTL_CTL_CODE (FILE_DEVICE_UNKNOWN,    \
               IPMI_IOCTL_BASE + (X),               \
               EMCPAL_IOCTL_METHOD_BUFFERED,        \
               EMCPAL_IOCTL_FILE_ANY_ACCESS))

//++
// Name:
//      IPMI_IOCTL_INDEX
//
// Description:
//      IPMI ioctl indices.
//
//-
typedef enum _IPMI_IOCTL_INDEX
{
    IPMI_SEND_REQUEST                = 1,
    IPMI_SEND_PEER_REQUEST           = 2,
    IPMI_PANIC                       = 3,
    IPMI_STATS_ENABLE                = 4,
    IPMI_STATS_DISABLE               = 5,
    IPMI_STATS_STATUS                = 6,
    IPMI_STATS_GET_STATS             = 7,
    IPMI_STATS_GET_GLOBAL_STATS      = 8,
    IPMI_SET_BMC_MY_ADDR             = 9,
    IPMI_SET_BMC_SLAVE_ADDR          = 10,
    IPMI_ENABLE_SFI                  = 11,
    IPMI_GET_SFI_STATE               = 12,
}
IPMI_IOCTL_INDEX;

//++
// Name:
//      IPMI_IOCTL_XXX
//
// Description:
//      IPMI ioctl definitions.
//
//--
#define IOCTL_IPMI_SEND_REQUEST                 IPMI_BUILD_IOCTL (IPMI_SEND_REQUEST)
#define IOCTL_IPMI_SEND_PEER_REQUEST            IPMI_BUILD_IOCTL (IPMI_SEND_PEER_REQUEST)
#define IOCTL_IPMI_PANIC                        IPMI_BUILD_IOCTL (IPMI_PANIC)
#define IOCTL_IPMI_STATS_ENABLE                 IPMI_BUILD_IOCTL (IPMI_STATS_ENABLE)
#define IOCTL_IPMI_STATS_DISABLE                IPMI_BUILD_IOCTL (IPMI_STATS_DISABLE)
#define IOCTL_IPMI_STATS_STATUS                 IPMI_BUILD_IOCTL (IPMI_STATS_STATUS)
#define IOCTL_IPMI_STATS_GET_STATS              IPMI_BUILD_IOCTL (IPMI_STATS_GET_STATS)
#define IOCTL_IPMI_STATS_GET_GLOBAL_STATS       IPMI_BUILD_IOCTL (IPMI_STATS_GET_GLOBAL_STATS)
#define IOCTL_IPMI_SET_BMC_MY_ADDR              IPMI_BUILD_IOCTL (IPMI_SET_BMC_MY_ADDR)
#define IOCTL_IPMI_SET_BMC_SLAVE_ADDR           IPMI_BUILD_IOCTL (IPMI_SET_BMC_SLAVE_ADDR)
#define IOCTL_IPMI_ENABLE_SFI                   IPMI_BUILD_IOCTL (IPMI_ENABLE_SFI)
#define IOCTL_IPMI_GET_SFI_STATE                IPMI_BUILD_IOCTL (IPMI_GET_SFI_STATE)
//++
//.End IPMI ioctl interface
//--

typedef enum _IPMI_SET_BMC_ADDR_TYPE
{
    IPMI_SET_BMC_MY_ADDR_TYPE,
    IPMI_SET_BMC_SLAVE_ADDR_TYPE,
}
IPMI_SET_BMC_ADDR_TYPE;

//++
// Name:
//      IPMI_REQUEST_P_INJECT_SNSR_READING
//
// Description:
//      Data portion of IPMI request for 'Inject Sensor Reading' command
//       on Phobos BMC.
//--
typedef struct _IPMI_REQUEST_P_INJECT_SNSR_READING
{
    UCHAR    sensorID;
    UCHAR    operation;
    UCHAR    analogReading;
    UCHAR    discrete0;
    UCHAR    discrete1;
}IPMI_REQUEST_P_INJECT_SNSR_READING, *PIPMI_REQUEST_P_INJECT_SNSR_READING;


typedef enum _IPMI_COMMAND
{
    IPMI_CMD_INVALID=0,
    IPMI_MGMT_FLT_LED,
    IPMI_BLADE_FLT_LED,
    IPMI_NOT_SAFE_TO_REMOVE_LED,
    IPMI_SLIC_0_FLT_LED,
    IPMI_SLIC_1_FLT_LED,
    IPMI_SLIC_2_FLT_LED,
    IPMI_SLIC_3_FLT_LED,
    IPMI_SLIC_4_FLT_LED,
    IPMI_SLIC_5_FLT_LED,
    IPMI_SLIC_6_FLT_LED,
    IPMI_SLIC_7_FLT_LED,
    IPMI_SLIC_8_FLT_LED,
    IPMI_SLIC_9_FLT_LED,
    IPMI_SLIC_10_FLT_LED,
    IPMI_SLIC_11_FLT_LED,
    IPMI_CACHE_CARD_FLT_LED,
    IPMI_BEM_FLT_LED,
    IPMI_AIRDAM_10G_PORT_0_LED,
    IPMI_AIRDAM_10G_PORT_1_LED,
    IPMI_AIRDAM_10G_PORT_2_LED,
    IPMI_AIRDAM_10G_PORT_3_LED,
    IPMI_AIRDAM_SAS_PORT_0_LED,
    IPMI_AIRDAM_SAS_PORT_1_LED,
    IPMI_AIRDAM_HILDA_PORT_0_LED,
    IPMI_AIRDAM_HILDA_PORT_1_LED,
    IPMI_FANPACK0_FLT_LED,
    IPMI_FANPACK1_FLT_LED,
    IPMI_FANPACK2_FLT_LED,
    IPMI_FANPACK3_FLT_LED,
    IPMI_FANPACK4_FLT_LED,
    IPMI_FANPACK5_FLT_LED,
    IPMI_FANPACK6_FLT_LED,
    IPMI_FANPACK7_FLT_LED,
    IPMI_BATTERY_0_FLT_LED,
    IPMI_BATTERY_1_FLT_LED,
    IPMI_BATTERY_2_FLT_LED,
    IPMI_VEEPROM_SPFLAGS,
    IPMI_VEEPROM_SLICFLAGS,
    IPMI_VEEPROM_TCOREG,
    IPMI_VEEPROM_BIST,
    IPMI_VEEPROM_QUICKBOOT_REG,
    IPMI_VEEPROM_QUICKBOOT_REG1,
    IPMI_VEEPROM_QUICKBOOT_REG2,
    IPMI_VEEPROM_QUICKBOOT_FLAG,
    IPMI_VEEPROM_FLTSTS_ALL,
    IPMI_VEEPROM_FLTSTS_1,
//    IPMI_VEEPROM_FLTSTS_2,
    IPMI_VEEPROM_FLTSTS_3,
    IPMI_VEEPROM_FLTSTS_4,
    IPMI_VEEPROM_FLTSTS_5,
    IPMI_VEEPROM_CPU_STS_REG,
    IPMI_VEEPROM_FSMR_FLAG, //read the flag before reading the mirror region.
    IPMI_VEEPROM_FSMR_ALL,
    IPMI_VEEPROM_FSMR_1,
//    IPMI_VEEPROM_FSMR_2,
    IPMI_VEEPROM_FSMR_3,
    IPMI_VEEPROM_FSMR_4,
    IPMI_VEEPROM_FSMR_5,
    IPMI_FSR_DIMMS,
    IPMI_FSR_DRIVES,
    IPMI_FSR_SLICS,
    IPMI_FSR_PS,
    IPMI_FSR_BATTERY,
    IPMI_FSR_FANS,
    IPMI_FSR_I2C,
    IPMI_FSR_SYSMISC,
    IPMI_SLIC_0_STATUS,
    IPMI_SLIC_1_STATUS,
    IPMI_SLIC_2_STATUS,
    IPMI_SLIC_3_STATUS,
    IPMI_SLIC_4_STATUS,
    IPMI_SLIC_5_STATUS,
    IPMI_SLIC_6_STATUS,
    IPMI_SLIC_7_STATUS,
    IPMI_SLIC_8_STATUS,
    IPMI_SLIC_9_STATUS,
    IPMI_SLIC_10_STATUS,
    IPMI_SLIC_11_STATUS,
    IPMI_CACHE_CARD_STATUS,
    IPMI_BEM_STATUS,
    IPMI_MM_GPIO_STATUS,
    IPMI_SLIC_0_CONTROL,
    IPMI_SLIC_1_CONTROL,
    IPMI_SLIC_2_CONTROL,
    IPMI_SLIC_3_CONTROL,
    IPMI_SLIC_4_CONTROL,
    IPMI_SLIC_5_CONTROL,
    IPMI_SLIC_6_CONTROL,
    IPMI_SLIC_7_CONTROL,
    IPMI_SLIC_8_CONTROL,
    IPMI_SLIC_9_CONTROL,
    IPMI_SLIC_10_CONTROL,
    IPMI_SLIC_11_CONTROL,
    IPMI_CACHE_CARD_CONTROL,
    IPMI_BEM_CONTROL,
    IPMI_SLIC_0_SCRATCHPAD,
    IPMI_SLIC_1_SCRATCHPAD,
    IPMI_SLIC_2_SCRATCHPAD,
    IPMI_SLIC_3_SCRATCHPAD,
    IPMI_SLIC_4_SCRATCHPAD,
    IPMI_SLIC_5_SCRATCHPAD,
    IPMI_SLIC_6_SCRATCHPAD,
    IPMI_SLIC_7_SCRATCHPAD,
    IPMI_SLIC_8_SCRATCHPAD,
    IPMI_SLIC_9_SCRATCHPAD,
    IPMI_SLIC_10_SCRATCHPAD,
    IPMI_SLIC_11_SCRATCHPAD,
    IPMI_BEM_SCRATCHPAD,
    IPMI_FAN0_STATUS,
    IPMI_FAN1_STATUS,
    IPMI_FAN2_STATUS,
    IPMI_FAN3_STATUS,
    IPMI_FAN4_STATUS,
    IPMI_FAN5_STATUS,
    IPMI_FAN6_STATUS,
    IPMI_FAN7_STATUS,
    IPMI_FAN8_STATUS,
    IPMI_FAN9_STATUS,
    IPMI_FAN10_STATUS,
    IPMI_FAN11_STATUS,
    IPMI_FAN0_SPEED,
    IPMI_FAN1_SPEED,
    IPMI_FAN2_SPEED,
    IPMI_FAN3_SPEED,
    IPMI_FAN4_SPEED,
    IPMI_FAN5_SPEED,
    IPMI_FAN6_SPEED,
    IPMI_FAN7_SPEED,
    IPMI_FAN8_SPEED,
    IPMI_FAN9_SPEED,
    IPMI_FAN10_SPEED,
    IPMI_FAN11_SPEED,
    IPMI_FAN0_PWM,
    IPMI_FAN1_PWM,
    IPMI_FAN2_PWM,
    IPMI_FAN3_PWM,
    IPMI_FAN4_PWM,
    IPMI_FAN5_PWM,
    IPMI_FAN0_RPM,
    IPMI_FAN1_RPM,
    IPMI_FAN2_RPM,
    IPMI_FAN3_RPM,
    IPMI_FAN4_RPM,
    IPMI_FAN5_RPM,
    IPMI_FAN6_RPM,
    IPMI_FAN7_RPM,
    IPMI_FAN8_RPM,
    IPMI_FAN9_RPM,
    IPMI_FAN10_RPM,
    IPMI_FAN11_RPM,
    IPMI_CM_A0_FWVER,
    IPMI_CM_A1_FWVER,
    IPMI_CM_A2_FWVER,
    IPMI_CM_B0_FWVER,
    IPMI_CM_B1_FWVER,
    IPMI_CM_B2_FWVER,
    IPMI_SP_STATUS,
    IPMI_AMBIENT_TEMP,
    IPMI_COINCELL_VOLT,
    IPMI_PS0_STATUS,
    IPMI_PS0_INPUT_PWR,
    IPMI_PS0_INVOLT,
    IPMI_PS0_OUTVOLT,
    IPMI_PS0_OUTCURRENT,
    IPMI_PS0_TEMP2,
    IPMI_PS0_TEMP3,
    IPMI_PS0_FWVER,
    IPMI_PS1_STATUS,
    IPMI_PS1_INPUT_PWR,
    IPMI_PS1_INVOLT,
    IPMI_PS1_OUTVOLT,
    IPMI_PS1_OUTCURRENT,
    IPMI_PS1_TEMP2,
    IPMI_PS1_TEMP3,
    IPMI_PS1_FWVER,
    IPMI_SHUTDOWN_IN_PROGRESS,
    IPMI_SSP_POWER_RESET,
    IPMI_HOLD_IN_POST,
    IPMI_ISSUE_SMI,
    IPMI_SHUTDOWN_TIME_REMAINING,
    IPMI_MULTIFUNCTION_BUTTON,
    IPMI_DELAYED_SHUTDOWN_TIMER,
    IPMI_CHASSIS_STATUS,
    IPMI_CHASSIS_CONTROL,
    IPMI_ADAPTIVE_COOLING_CONFIG,
    IPMI_RESUME_EMC_PART_NUMBER,
    IPMI_RESUME_EMC_ASSEMBLY_NAME_1,
    IPMI_RESUME_EMC_ASSEMBLY_NAME_2,
    IPMI_RESUME_EMC_ASSEMBLY_REV,
    IPMI_RESUME_EMC_SERIAL_NUM,
    IPMI_RESUME_EMC_FAMILY_FRU_ID,
    IPMI_RESUME_EMC_FRU_CAPABILITY,
    IPMI_RESUME_VENDOR_NAME_1,
    IPMI_RESUME_VENDOR_NAME_2,
    IPMI_RESUME_VENDOR_PART_NUM_1,
    IPMI_RESUME_VENDOR_PART_NUM_2,
    IPMI_RESUME_VENDOR_ASSEMBLY_REV,
    IPMI_RESUME_VENDOR_SERIAL_NUM_1,
    IPMI_RESUME_VENDOR_SERIAL_NUM_2,
    IPMI_RESUME_VENDOR_LOC_MFT_1,
    IPMI_RESUME_VENDOR_LOC_MFT_2,
    IPMI_SPS_PRESENT,
    IPMI_COM1_MUX,
    IPMI_COM2_MUX,
    IPMI_ECC_COUNT,
    IPMI_EVTLOG_DISABLED,
    IPMI_GET_SDR_REPOSITORY_INFO,
    IPMI_GET_SDR,
    IPMI_BMC_POLLING_I2C_BUS_0,
    IPMI_BMC_POLLING_I2C_BUS_1,
    IPMI_BMC_POLLING_I2C_BUS_2,
    IPMI_BMC_POLLING_I2C_BUS_3,
    IPMI_BMC_POLLING_I2C_BUS_4,
    IPMI_BMC_POLLING_I2C_BUS_5,
    IPMI_BMC_POLLING_I2C_BUS_6,
    IPMI_BMC_POLLING_I2C_BUS_7,
    IPMI_BMC_BIST_REPORT,
    IPMI_BMC_BIST_REPORT_CPU,
    IPMI_BMC_BIST_REPORT_DRAM,
    IPMI_BMC_BIST_REPORT_SRAM,
    IPMI_BMC_BIST_REPORT_I2C_0,
    IPMI_BMC_BIST_REPORT_I2C_1,
    IPMI_BMC_BIST_REPORT_I2C_2,
    IPMI_BMC_BIST_REPORT_I2C_3,
    IPMI_BMC_BIST_REPORT_I2C_4,
    IPMI_BMC_BIST_REPORT_I2C_5,
    IPMI_BMC_BIST_REPORT_I2C_6,
    IPMI_BMC_BIST_REPORT_I2C_7,
    IPMI_BMC_BIST_REPORT_UART_2,
    IPMI_BMC_BIST_REPORT_UART_3,
    IPMI_BMC_BIST_REPORT_UART_4,
    IPMI_BMC_BIST_REPORT_SSP,
    IPMI_BMC_BIST_REPORT_BBU_0,
    IPMI_BMC_BIST_REPORT_BBU_1,
    IPMI_BMC_BIST_REPORT_BBU_2,
    IPMI_BMC_BIST_REPORT_BBU_3,
    IPMI_BMC_BIST_REPORT_NVRAM,
    IPMI_BMC_BIST_REPORT_SGPIO,
    IPMI_BMC_BIST_REPORT_FAN_0,
    IPMI_BMC_BIST_REPORT_FAN_1,
    IPMI_BMC_BIST_REPORT_FAN_2,
    IPMI_BMC_BIST_REPORT_FAN_3,
    IPMI_BMC_BIST_REPORT_FAN_4,
    IPMI_BMC_BIST_REPORT_FAN_5,
    IPMI_BMC_BIST_REPORT_FAN_6,
    IPMI_BMC_BIST_REPORT_FAN_7,
    IPMI_BMC_BIST_REPORT_FAN_8,
    IPMI_BMC_BIST_REPORT_FAN_9,
    IPMI_BMC_BIST_REPORT_FAN_10,
    IPMI_BMC_BIST_REPORT_FAN_11,
    IPMI_BMC_BIST_REPORT_FAN_12,
    IPMI_BMC_BIST_REPORT_FAN_13,
    IPMI_BMC_BIST_REPORT_ARB_0,
    IPMI_BMC_BIST_REPORT_ARB_1,
    IPMI_BMC_BIST_REPORT_ARB_2,
    IPMI_BMC_BIST_REPORT_ARB_3,
    IPMI_BMC_BIST_REPORT_ARB_4,
    IPMI_BMC_BIST_REPORT_ARB_5,
    IPMI_BMC_BIST_REPORT_ARB_6,
    IPMI_BMC_BIST_REPORT_ARB_7,
    IPMI_BMC_BIST_REPORT_NCSI_LAN,
    IPMI_BMC_BIST_REPORT_DEDICATED_LAN,
    IPMI_BMC_APP_MAIN_FW_STATUS,
    IPMI_BMC_APP_REDUNDANT_FW_STATUS,
    IPMI_BMC_SSP_FW_STATUS,
    IPMI_BMC_BOOT_BLOCK_FW_STATUS,
    IPMI_BEM_SXP_CONSOLE_MUX_CONTROL,
    IPMI_BEM_PWR_FAULT,
    IPMI_BATTERY_ENABLE,
    IPMI_RUN_BATTERY_SELFTEST,
    IPMI_BATTERY_0_STATUS,
    IPMI_BATTERY_0_EXT_STATUS_I2C,
    IPMI_BATTERY_0_EXT_STATUS_GPIO,
    IPMI_BATTERY_0_ENERGY_RQMTS,
    IPMI_BATTERY_0_FAULTS,
    IPMI_BATTERY_0_SELFTEST_RESULT_1,//reg_71 should always be read first.
    IPMI_BATTERY_0_SELFTEST_RESULT_2,//reg_72 should be read after reg_71.
    IPMI_BATTERY_0_FAULT_STATUS,//non-paged reg_22
    IPMI_BATTERY_0_BIST_RESULTS,
    IPMI_BATTERY_0_FWVER,
    IPMI_BATTERY_1_EXT_STATUS_I2C,
    IPMI_BATTERY_1_EXT_STATUS_GPIO,
    IPMI_BATTERY_1_ENERGY_RQMTS,
    IPMI_BATTERY_1_FAULTS,
    IPMI_BATTERY_1_BIST_RESULTS,
    IPMI_BATTERY_1_FWVER,
    IPMI_BATTERY_2_EXT_STATUS_I2C,
    IPMI_BATTERY_2_EXT_STATUS_GPIO,
    IPMI_BATTERY_2_ENERGY_RQMTS,
    IPMI_BATTERY_2_FAULTS,
    IPMI_BATTERY_2_BIST_RESULTS,
    IPMI_BATTERY_2_FWVER,
    IPMI_BMC_NETWORK_PORT,
    IPMI_BMC_NETWORK_MODE,
    IPMI_BMC_IP_ADDR,
    IPMI_BMC_SUBNET,
    IPMI_BMC_GATEWAY,
    IPMI_OEM_SSP_RESET,
    IPMI_PEER_CACHE_UPDATE,
    IPMI_FAN_REDUNDANCY,
    IPMI_SNSR_READING,
    IPMI_VAULT_MODE,
    TOTAL_IPMI_COMMANDS
} IPMI_COMMAND;

#if defined(__cplusplus)
/* use extern C++ here to trick other C++ files including this as extern C
 */
extern "C++"{
/* Define a postfix increment operator for _IPMI_COMMAND
 */
inline _IPMI_COMMAND operator++( IPMI_COMMAND &ipmiCmd, int )
{
   return ipmiCmd = (IPMI_COMMAND)((int)ipmiCmd + 1);
}
}
#endif


/* Fake devices SPECL will use for grouping together commands
 */
typedef enum _IPMI_DEV
{
    IPMI_DEV_INVALID,

    IPMI_LOCAL_SLICS,
    IPMI_PEER_SLICS,

    IPMI_LOCAL_MGMT,
    IPMI_PEER_MGMT,

    IPMI_LOCAL_FAULT_STATUS,
    IPMI_PEER_FAULT_STATUS,

    IPMI_LOCAL_SLAVE_PORT,
    IPMI_PEER_SLAVE_PORT,

    IPMI_LOCAL_PS_0,
    IPMI_LOCAL_PS_1,
    IPMI_PEER_PS_0,
    IPMI_PEER_PS_1,

    IPMI_LOCAL_LEDS,
    IPMI_PEER_LEDS,

    IPMI_LOCAL_PROTOCOL_PORT_LEDS,
    IPMI_PEER_PROTOCOL_PORT_LEDS,

    IPMI_CACHE_CARD,

    IPMI_LOCAL_MISC,
    IPMI_PEER_MISC,

    IPMI_LOCAL_FANS_GLOBAL,
    IPMI_PEER_FANS_GLOBAL,

    IPMI_LOCAL_FANPACK_0,
    IPMI_LOCAL_FANPACK_1,
    IPMI_LOCAL_FANPACK_2,
    IPMI_LOCAL_FANPACK_3,
    IPMI_LOCAL_FANPACK_4,
    IPMI_LOCAL_FANPACK_5,
    IPMI_LOCAL_FANPACK_6,
    IPMI_LOCAL_FANPACK_7,
    IPMI_PEER_FANPACK_0,
    IPMI_PEER_FANPACK_1,
    IPMI_PEER_FANPACK_2,
    IPMI_PEER_FANPACK_3,
    IPMI_PEER_FANPACK_4,
    IPMI_PEER_FANPACK_5,

    IPMI_LOCAL_AMBIENT_TEMP,
    IPMI_PEER_AMBIENT_TEMP,

    IPMI_LOCAL_BMC,
    IPMI_PEER_BMC,

    IPMI_LOCAL_BATTERY_0,
    IPMI_LOCAL_BATTERY_1,
    IPMI_LOCAL_BATTERY_2,
    IPMI_PEER_BATTERY_0,
    IPMI_PEER_BATTERY_1,
    IPMI_PEER_BATTERY_2,

    IPMI_LOCAL_PS_0_RESUME,
    IPMI_LOCAL_PS_1_RESUME,
    IPMI_PEER_PS_0_RESUME,
    IPMI_PEER_PS_1_RESUME,

    IPMI_LOCAL_CM_0_RESUME,
    IPMI_LOCAL_CM_1_RESUME,
    IPMI_LOCAL_CM_2_RESUME,
    IPMI_LOCAL_CM_3_RESUME,
    IPMI_LOCAL_CM_4_RESUME,
    IPMI_LOCAL_CM_5_RESUME,

    IPMI_LOCAL_SDR,
    IPMI_PEER_SDR,

    IPMI_LOCAL_FLTSTS_MR,

    TOTAL_IPMI_DEV
} IPMI_DEV;

#if defined(__cplusplus)
/* use extern C++ here to trick other C++ files including this as extern C
 */
extern "C++"{
/* Define a postfix increment operator for _IPMI_DEV
 */
inline _IPMI_DEV operator++( IPMI_DEV &ipmiDev, int )
{
   return ipmiDev = (IPMI_DEV)((int)ipmiDev + 1);
}
}
#endif

#endif // _IPMI_H_
