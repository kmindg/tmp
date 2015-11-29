#ifndef DIPLEX_FPGA_MICR0_H
#define DIPLEX_FPGA_MICR0_H

/***************************************************************************
 * Copyright (C) EMC Corporation 2003
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/****************************************************************
 *  diplex_FPGA_micro.h
 ****************************************************************
 *
 *  Description:
 *  This header file contains structures which define
 *  the format of the diplex FPGA, which was first introduced
 *  for the Hammer-series platforms. 
 *
 *  History:
 *      Jun-05-2009 . Phil Leef - Created
 *
 ****************************************************************/


#include "generic_types.h"

typedef enum _FPGA_REG_BLOCK
{
    FPGA_INVALID,

    /* FPGA Register Blocks */
    FPGA_CHIP_ID_BLOCK,
    FPGA_REV_ID_BLOCK,
    FPGA_MULTIPLEXER_TYPE_BLOCK,
    FPGA_PACKET_FORMAT_BLOCK,
    FPGA_NUMBER_OF_UARTS_BLOCK,
    FPGA_HOST_TIMEOUT_BLOCK,
    FPGA_FLOW_CTRL_BLOCK,
    FPGA_HOST_BAUDRATE_BLOCK,
    FPGA_DIPLEX_BAUDRATE_PORT0_BLOCK,
    FPGA_DIPLEX_BAUDRATE_PORT1_BLOCK,
    FPGA_DIPLEX_BAUDRATE_PORT2_BLOCK,
    FPGA_DIPLEX_BAUDRATE_PORT3_BLOCK,
    FPGA_DIPLEX_BAUDRATE_PORT4_BLOCK,
    FPGA_DIPLEX_BAUDRATE_PORT5_BLOCK,
    FPGA_DIPLEX_BAUDRATE_PORT6_BLOCK,
    FPGA_DIPLEX_BAUDRATE_PORT7_BLOCK,
    FPGA_LED_CTRL_PORT0_BLOCK,
    FPGA_LED_CTRL_PORT1_BLOCK,
    FPGA_LED_CTRL_PORT2_BLOCK,
    FPGA_LED_CTRL_PORT3_BLOCK,
    FPGA_LED_CTRL_PORT4_BLOCK,
    FPGA_LED_CTRL_PORT5_BLOCK,
    FPGA_LED_CTRL_PORT6_BLOCK,
    FPGA_LED_CTRL_PORT7_BLOCK,
    FPGA_DIPLEX_VOLTAGE_BLOCK,
    FPGA_DIPLEX_TIMER_BLOCK,
    FPGA_DIPLEX_LOOPBACK_BLOCK,
    FPGA_DIPLEX_PORT_ENABLE_BLOCK,
    FPGA_DIPLEX_LED_CONFIG_BLOCK,

    FPGA_TOTAL_BLOCK,
} FPGA_REG_BLOCK;

#pragma pack(push, fpga_diplex_micro, 1)

/****************************************************************
 * Register Definitions
 *
 * NOTE: ALL Register are exactly 8-bits (1 Byte)
 ****************************************************************/

typedef enum _FPGA_BAUD_RATES
{
    FPGA_BAUD_RATE_9600         = 0,
    FPGA_BAUD_RATE_19200        = 1,
    FPGA_BAUD_RATE_38400        = 2,
    FPGA_BAUD_RATE_57600        = 3,
    FPGA_BAUD_RATE_115200       = 4,
} FPGA_BAUD_RATES;

typedef enum _FPGA_VOLTAGE
{
    FPGA_VOLTAGE_5V             = 0,
    FPGA_VOLTAGE_33V            = 1,
} FPGA_VOLTAGE;

typedef enum _FPGA_LOOPBACK
{
    FPGA_LOOPBACK_DISABLED      = 0,
    FPGA_LOOPBACK_ENABLED       = 1,
} FPGA_LOOPBACK;

typedef enum _FPGA_LED_CONFIG
{
    FPGA_LED_CONFIG_DISABLED    = 0,
    FPGA_LED_CONFIG_NORMAL      = 1,
    FPGA_LED_CONFIG_TWISTED     = 2,
    FPGA_LED_INVALID,
} FPGA_LED_CONFIG;

typedef struct 
{
    UINT_8E     blue_green:1,
                mark_port_16HZ:1,
                mark_port_1HZ:1,
                fault_port:1,                
                reserved:3,
                color_ctrl:1;
} FPGA_LED_CONTROL, *PFPGA_LED_CONTROL;


#define FPGA_FLOW_CTRL_HARDWARE_BASED           0x01
#define FPGA_FLOW_CTRL_CREDIT_BASED             0x02

#pragma pack(pop, fpga_diplex_micro)

#endif //DIPLEX_FPGA_MICR0_H
