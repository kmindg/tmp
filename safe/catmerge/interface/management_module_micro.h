#ifndef MANAGEMENT_MODULE_MICRO_H
#define MANAGEMENT_MODULE_MICRO_H

/***************************************************************************
 * Copyright (C) EMC Corporation 2003
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/****************************************************************
 *  management_module_micro.h
 ****************************************************************
 *
 *  Description:
 *  This header file contains structures which define
 *  the format of the microcontroller registers within a 
 *  Management Module for the Hammer-series platforms. 
 *
 *  History:
 *      Mar-09-2007 . Phil Leef - Created
 *
 ****************************************************************/


#include "generic_types.h"

typedef enum _MGMT_MODULE_REG_BLOCK
{
    /* Mgmt Register Blocks */
    MGMT_GLOBAL_STATUS_BLOCK,
    MGMT_SYSTEM_BLOCK,
    MGMT_FAULT_BLOCK,
    MGMT_FAULT_MASK_BLOCK,
    MGMT_MFG_MODE_LED_CTRL_BLOCK,
    MGMT_SERIAL_XLINK_TIMEOUT_BLOCK,
    MGMT_FIRMWARE_DOWNLOAD_BLOCK,
    MGMT_MICROCODE_UPDATE_BLOCK,
    MGMT_CANNED_VLAN_CONFIG_BLOCK,
    MGMT_VLAN_MAPPING_BLOCK_0,
    MGMT_VLAN_MAPPING_BLOCK_1,
    MGMT_VLAN_MAPPING_BLOCK_2,
    MGMT_VLAN_MAPPING_BLOCK_3,
    MGMT_VLAN_MAPPING_BLOCK_4,
    MGMT_PORT_STS_CONFIG_BLOCK_0,
    MGMT_PORT_STS_CONFIG_BLOCK_1,
    MGMT_PORT_STS_CONFIG_BLOCK_2,
    MGMT_PORT_STS_CONFIG_BLOCK_3,
    MGMT_PORT_STS_CONFIG_BLOCK_4,
    MGMT_SPI_COMMAND_BLOCK,
    MGMT_SPI_READ_DATA_BLOCK,
    MGMT_SPECIAL_STS_RESPONSE_BLOCK,
    MGMT_SERIAL_BUFF_ADDR_PTR_BLOCK,
    MGMT_SERIAL_BUFF_BLOCK,
    MGMT_WATCHDOG_COUNT_CTRL_BLOCK,
    MGMT_CODE_REVISION_BLOCK,

    MGMT_MODULE_TOTAL_BLOCK,
} MGMT_MODULE_REG_BLOCK;

#pragma pack(push, management_module_micro, 1)

/****************************************************************
 * Global Status Block
 *
 * RO
 * Start Reg: 01h
 * Num Bytes: 32 (max of the ICH)
 ****************************************************************/

// Defined at the bottom of this file. 


/****************************************************************
 * System Block
 *
 * R/W
 * Start Reg: 03h
 * Num Bytes: 4
 ****************************************************************/
typedef struct 
{
    UINT_8E     usb_hub_reset:2,
                broadcom_reset:2,
                sps_reset:2,
                generation_id:2;
} SYSTEM_REG_MSB;

typedef struct 
{
    UINT_8E     enable_packet_forwarding:1,
                fault_state:1,
                slot_id:1,
                reserved_1:1,
                enable_loop_detect:2,
                enable_cable_diag:1,
                reserved:1;
} SYSTEM_REG_LSB;

typedef union
{
    UINT_8E                     systemReg;
    SYSTEM_REG_MSB              fields;
} SYSTEM_REG_MSB_RAW_INFO;

typedef union
{
    UINT_8E                     systemReg;
    SYSTEM_REG_LSB              fields;
} SYSTEM_REG_LSB_RAW_INFO;

typedef struct
{
    UINT_8E                     RegisterBlockNum;
    SYSTEM_REG_MSB_RAW_INFO     systemRegMSB;
    SYSTEM_REG_LSB_RAW_INFO     systemRegLSB;
    UINT_8E                     BlockChecksum;
} MGMT_SYSTEM_REG;


/****************************************************************
 * Fault Block / Fault Mask Block
 *
 * R/W
 * Start Reg: 04h / 05h
 * Num Bytes: 4
 ****************************************************************/
typedef struct
{
    UINT_8E     cable_diag_failed:1,
                loop_detected:1,
                spi_slave_abort:1,
                spi_read_overrun:1,
                spid_write_collision:1,
                sec_image_checksum_failure:1,
                reserved:2;
} FAULT_REG_MSB;

typedef struct 
{
    UINT_8E     supply_A_fault:1,
                supply_B_fault:1,
                switch_fault:1,
                spi_interface_timeout:1,
                invalid_rack_fault:1,
                slot_id_fault:1,
                nmi_button_stuck:1,
                peer_micro_comm_lost:1;
} FAULT_REG_LSB;

typedef union
{
    UINT_8E                     faultReg;
    FAULT_REG_MSB               fields;
} FAULT_REG_MSB_RAW_INFO;

typedef union
{
    UINT_8E                     faultReg;
    FAULT_REG_LSB               fields;
} FAULT_REG_LSB_RAW_INFO;

typedef struct
{
    UINT_8E                     RegisterBlockNum;
    FAULT_REG_MSB_RAW_INFO      faultRegMSB;
    FAULT_REG_LSB_RAW_INFO      faultRegLSB;
    UINT_8E                     BlockChecksum;
} MGMT_FAULT_REG;


/****************************************************************
 * Manufacturing Mode & LED Ctrl Block
 *
 * R/W
 * Start Reg: 06h
 * Num Bytes: 4
 ****************************************************************/
typedef struct
{
    UINT_8E     margin_low:1,
                margin_high:1,
                reserved:6;
} MFG_MODE_LED_CTRL_REG_MSB;

typedef struct 
{
    UINT_8E     sw_enabled_mm:1,
                hw_enabled_mm:1,
                fault_led_ctrl:2,
                servicePort_led_ctrl:2,
                mgmtPort_led_ctrl:2;
} MFG_MODE_LED_CTRL_REG_LSB;

typedef union
{
    UINT_8E                     mfgModeLEDCtrl;
    MFG_MODE_LED_CTRL_REG_MSB   fields;
} MFG_MODE_LED_CTRL_MSB_RAW_INFO;

typedef union
{
    UINT_8E                     mfgModeLEDCtrl;
    MFG_MODE_LED_CTRL_REG_LSB   fields;
} MFG_MODE_LED_CTRL_LSB_RAW_INFO;

typedef struct
{
    UINT_8E                         RegisterBlockNum;
    MFG_MODE_LED_CTRL_MSB_RAW_INFO  mfgModeLEDCtrlMSB;
    MFG_MODE_LED_CTRL_LSB_RAW_INFO  mfgModeLEDCtrlLSB;
    UINT_8E                         BlockChecksum;
} MGMT_MFG_MODE_LEG_CTRL;

typedef struct
{
    UINT_8E                         RegisterBlockNum;
    MFG_MODE_LED_CTRL_MSB_RAW_INFO  writeMaskMSB;
    MFG_MODE_LED_CTRL_LSB_RAW_INFO  writeMaskLSB;
    MFG_MODE_LED_CTRL_MSB_RAW_INFO  mfgModeLEDCtrlMSB;
    MFG_MODE_LED_CTRL_LSB_RAW_INFO  mfgModeLEDCtrlLSB;
    UINT_8E                         BlockChecksum;
} MGMT_MFG_MODE_LEG_CTRL_WRITE;

/****************************************************************
 * Serial x-link Timeout Block
 *
 * R/W
 * Start Reg: 07h
 * Num Bytes: 4
 ****************************************************************/

typedef struct
{
    UINT_8E                     RegisterBlockNum;
    UINT_8E                     xLinktimeoutValueMSB;
    UINT_8E                     xLinktimeoutValueLSB;
    UINT_8E                     BlockChecksum;
} MGMT_XLINK_TIMEOUT;

typedef struct
{
    UINT_8E                     RegisterBlockNum;
    UINT_8E                     writeMaskMSB;
    UINT_8E                     writeMaskLSB;
    UINT_8E                     xLinktimeoutValueMSB;
    UINT_8E                     xLinktimeoutValueLSB;
    UINT_8E                     BlockChecksum;
} MGMT_XLINK_TIMEOUT_WRITE;

/* Register(s) 08h & 09h deal with microcode updates */

/****************************************************************
 * Canned Configuration Block
 *
 * R/W
 * Start Reg: 0Ah
 * Num Bytes: 3
 ****************************************************************/

typedef struct
{
    UINT_8E                     RegisterBlockNum;
    UINT_8E                     CannedConfig;
    UINT_8E                     BlockChecksum;
} CANNED_VLAN_CONFIG;

/****************************************************************
 * vLAN Mapping Block
 *
 * R/W
 * Start Reg: 10h - 14h
 * Num Bytes: 4 ea
 ****************************************************************/
typedef struct 
{
    UINT_8E     vlan_port_0:1,
                vlan_port_1:1,
                vlan_port_2:1,
                vlan_port_3:1,
                vlan_port_4:1,
                reserved:3;
} VLAN_MAPPING_REG_LSB;

typedef union
{
    UINT_8E                     vLANMapping;
    VLAN_MAPPING_REG_LSB        fields;
} VLAN_MAPPING_LSB_RAW_INFO;

typedef struct
{
    UINT_8E                     RegisterBlockNum;
    UINT_8E                     vLANMappingMSB;
    VLAN_MAPPING_LSB_RAW_INFO   vLANMappingLSB;
    UINT_8E                     BlockChecksum;
} MGMT_VLAN_MAPPING;

typedef struct
{
    UINT_8E                         RegisterBlockNum;
    UINT_8E                         writeMaskMSB;
    VLAN_MAPPING_LSB_RAW_INFO       writeMaskLSB;
    UINT_8E                         vLANMappingMSB;
    VLAN_MAPPING_LSB_RAW_INFO       vLANMappingLSB;
    UINT_8E                         BlockChecksum;
} MGMT_VLAN_MAPPING_WRITE;

/****************************************************************
 * Port Status & Config Block
 *
 * R/W
 * Start Reg: 20h - 24h
 * Num Bytes: 4 ea
 ****************************************************************/
typedef struct 
{
    UINT_8E     port_speed:2,
                port_link_status:1,
                port_auto_negotiate:1,
                port_duplex_mode:1,
                reserved:3;
} PORT_STS_CONFIG_REG_LSB;

typedef union
{
    UINT_8E                     portStatusConfig;
    PORT_STS_CONFIG_REG_LSB     fields;
} PORT_STS_CONFIG_LSB_RAW_INFO;

typedef struct
{
    UINT_8E                         RegisterBlockNum;
    UINT_8E                         portStatusConfigMSB;
    PORT_STS_CONFIG_LSB_RAW_INFO    portStatusConfigLSB;
    UINT_8E                         BlockChecksum;
} MGMT_PORT_STS_CONFIG;

typedef struct
{
    UINT_8E                         RegisterBlockNum;
    UINT_8E                         writeMaskMSB;
    PORT_STS_CONFIG_LSB_RAW_INFO    writeMaskLSB;
    UINT_8E                         portStatusConfigMSB;
    PORT_STS_CONFIG_LSB_RAW_INFO    portStatusConfigLSB;
    UINT_8E                         BlockChecksum;
} MGMT_PORT_STS_CONFIG_WRITE;


/****************************************************************
 * SPI Command / SPI Data Block
 *
 * WO / RO
 * Start Reg: 70h / 71h
 * Num Bytes: 7
 ****************************************************************/
typedef struct 
{
    UINT_8E     length:4,
                direction:4;
} SPI_CMD_REG;

typedef union
{
    UINT_8E                     spiCommand;
    SPI_CMD_REG                 fields;
} SPI_CMD_RAW_INFO;

typedef struct
{
    UINT_8E                     RegisterBlockNum;
    SPI_CMD_RAW_INFO            spiCommand;
    UINT_8E                     spiPage;
    UINT_8E                     spiRegister;
    UINT_8E                     spiData[8];
    UINT_8E                     BlockChecksum;
} MGMT_SPI_CMD;

/****************************************************************
 * Special Status Response Block
 *
 * RO
 * Start Reg: BBh
 * Num Bytes: 4
 ****************************************************************/

typedef struct
{
    UINT_8E                     RegisterBlockNum;
    UINT_8E                     specialStatusResponseMSB;
    UINT_8E                     specialStatusResponseLSB;
    UINT_8E                     BlockChecksum;
} MGMT_SPECIAL_STS_RESPONSE;


/****************************************************************
 * Serial Buffer Address Block
 *
 * R/W
 * Start Reg: F3h
 * Num Bytes: 4
 ****************************************************************/

typedef struct
{
    UINT_8E                     RegisterBlockNum;
    UINT_8E                     serialBufferAddressMSB;
    UINT_8E                     serialBufferAddressLSB;
    UINT_8E                     BlockChecksum;
} MGMT_SERIAL_BUFF_ADDR;


/****************************************************************
 * Serial Buffer Block
 *
 * RO
 * Start Reg: F4h
 * Num Bytes: 18
 ****************************************************************/

typedef struct
{
    UINT_8E                     RegisterBlockNum;
    UINT_8E                     serialBuffer[16];
    UINT_8E                     BlockChecksum;
} MGMT_SERIAL_BUFF;

/****************************************************************
 * Code Revision Block
 *
 * RO
 * Start Reg: F6h
 * Num Bytes: 4
 ****************************************************************/

typedef struct
{
    UINT_8E                     RegisterBlockNum;
    UINT_8E                     PriFirmwareMajor;
    UINT_8E                     PriFirmwareMinor;
    UINT_8E                     BlockChecksum;
} MGMT_CODE_REVISION;


/***********************************************************************
 * MGMT_MODULE_GLOBAL_STATUS
 *      This structure defines the Global Status register block set for 
 *      both Akula / Solar Flare components
 ***********************************************************************
 */
typedef struct _MGMT_MODULE_GLOBAL_STATUS
{
    UINT_8E                         RegisterBlockNum;
    SYSTEM_REG_MSB_RAW_INFO         systemRegMSB;
    SYSTEM_REG_LSB_RAW_INFO         systemRegLSB;
    FAULT_REG_MSB_RAW_INFO          faultRegMSB;
    FAULT_REG_LSB_RAW_INFO          faultRegLSB;
    FAULT_REG_MSB_RAW_INFO          faultMaskRegMSB;
    FAULT_REG_LSB_RAW_INFO          faultMaskRegLSB;
    MFG_MODE_LED_CTRL_LSB_RAW_INFO  mfgModeLEDCtrlLSB;
    UINT_8E                         CannedConfigLSB;
    UINT_8E                         Port0vLANMappingMSB;
    VLAN_MAPPING_LSB_RAW_INFO       Port0vLANMappingLSB;
    UINT_8E                         Port1vLANMappingMSB;
    VLAN_MAPPING_LSB_RAW_INFO       Port1vLANMappingLSB;
    UINT_8E                         Port2vLANMappingMSB;
    VLAN_MAPPING_LSB_RAW_INFO       Port2vLANMappingLSB;
    UINT_8E                         Port3vLANMappingMSB;
    VLAN_MAPPING_LSB_RAW_INFO       Port3vLANMappingLSB;
    UINT_8E                         Port4vLANMappingMSB;
    VLAN_MAPPING_LSB_RAW_INFO       Port4vLANMappingLSB;
    UINT_8E                         Port0portStatusConfigMSB;
    PORT_STS_CONFIG_LSB_RAW_INFO    Port0portStatusConfigLSB;
    UINT_8E                         Port1portStatusConfigMSB;
    PORT_STS_CONFIG_LSB_RAW_INFO    Port1portStatusConfigLSB;
    UINT_8E                         Port2portStatusConfigMSB;
    PORT_STS_CONFIG_LSB_RAW_INFO    Port2portStatusConfigLSB;
    UINT_8E                         Port3portStatusConfigMSB;
    PORT_STS_CONFIG_LSB_RAW_INFO    Port3portStatusConfigLSB;
    UINT_8E                         Port4portStatusConfigMSB;
    PORT_STS_CONFIG_LSB_RAW_INFO    Port4portStatusConfigLSB;
    UINT_8E                         PriFirmwareMajor;
    UINT_8E                         PriFirmwareMinor;
    UINT_8E                         BlockChecksum;
} MGMT_MODULE_GLOBAL_STATUS, *PMGMT_MODULE_GLOBAL_STATUS;

#pragma pack(pop, management_module_micro)

#endif //MANAGEMENT_MODULE_MICRO_H
