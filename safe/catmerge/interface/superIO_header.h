#ifndef __SUPERIO_HEADER__
#define __SUPERIO_HEADER__

//***************************************************************************
// Copyright (C) EMC Corporation 2011
// All rights reserved.
// Licensed material -- property of EMC Corporation
//**************************************************************************/
//++
// File Name:
//      superIO_header.h
//
// Contents:
//      SuperIO definitions common to modules like SPID and WDT.
//
// Revision History:
//--


//++
//          "Super I/O" chip constants
//
//      Values used by the "Super I/O" chip
//--
#define SUPER_IO_SELECTION_REG                  0x2E
#define SUPER_IO_DATA_REG                       0x2F

#define SUPER_IO_LOCK_VALUE                     0xA5
#define SUPER_IO_UNLOCK_VALUE                   0x5A

#define SUPER_IO_ID_REG                         0x20

#define SUPER_IO_DEVICE_SELECTION_VALUE         0x07

#define SUPER_IO_ACTIVATE_REG                   0x30
#define SUPER_IO_ACTIVATE_VALUE                 0x01

#define SUPER_IO_TOP_PORT_ADDR_REG              0x60
#define SUPER_IO_BOTTOM_PORT_ADDR_REG           0x61

#define SIO_PORT_PIN_MASK                       10

/* Super (or Server) I/O chip ID's
 */
#define SIO_PILOT3_BMC_FAMILY_ID                0x03

/* BMC SIO Specific Info
 */
#define SUPER_IO_GPIO_DATAOUT_REG_OFFSET        0x08
#define SUPER_IO_GPIO_DATAIN_REG_OFFSET         0x09
#define SUPER_IO_GPIO_HOST_GRP_SEL_REG          0x0E
#define SUPER_IO_GPIO_HOST_EXT_EN               0x80    // Bit 7

/* National SIO Specific Info
 */
#define SUPER_IO_XBUS_MEM_HIGH_BASE_ADDR_REG    0xF9
#define SUPER_IO_XBUS_MEM_LOW_BASE_ADDR_REG     0xFA

#define SUPER_IO_LEDBLINK_OFFSET                0x0B
#define SUPER_IO_LEDBLINK_MASK                  0x07

/* These two defines are applicable for the two GPIOs within the 
 * LDN SWC, which provide blinking capabilities
 */
#define LED1BLNK                                44
#define LED2BLNK                                45

/***************************
 * struct: SUPERIO_BMC_LDN
 *
 * Defines all possible Logical 
 * Devices within the BMC SuperIO
 ***************************/
typedef enum _SUPERIO_BMC_LDN
{
    SIO_BMC_PSR         = 0x00, // Pilot Specific Registers
    SIO_BMC_SP2         = 0x01, // Serial Port 2
    SIO_BMC_SP1         = 0x02, // Serial Port 1
    SIO_BMC_SWC         = 0x03, // System Wake-up Control
    SIO_BMC_GPIO        = 0x04, // General Purpose I/O
    SIO_BMC_SWDT        = 0x05, // Super I/O Watchdog Timer
    SIO_BMC_KCS3        = 0x08, // KCS3 Interface
    SIO_BMC_KCS4        = 0x09, // KCS4 Interface
    SIO_BMC_KCS5        = 0x0A, // KCS5 Interface
    SIO_BMC_BT          = 0x0B, // BT Interface
    SIO_BMC_SMIC        = 0x0C, // SMIC Interface
    SIO_BMC_MAILBOX     = 0x0D, // Mailbox Interface
    SIO_BMC_SPI         = 0x0F, // SPI Interface
} SUPERIO_BMC_LDN;

/****************************************
 * struct: SIO_TYPE
 *
 * Defines the possible Super I/O Chips
 * Black Widow is using Winbond 87417
 * Brown Widow is using the Winbond 87427 replacement 
 * for the 87417
 * Transformers use the Pilot3 BMC which 
 * contains a SIO
 ****************************************/
typedef enum _SIO_TYPE
{
    SIO_INVALID,
    SIO_PILOT3_BMC,
} SIO_TYPE;

#endif
