#ifndef __GPIO_SIGNALS__
#define __GPIO_SIGNALS__

//***************************************************************************
// Copyright (C) EMC Corporation 2005-2014
// All rights reserved.
// Licensed material -- property of EMC Corporation
//**************************************************************************/

//**************************************************************************
//  $Id$
//**************************************************************************

//++
// File Name:
//      gpio_signals.h
//
// Contents:
//      GPIO pin defines. 
//
// Revision History:
//  01-April-05   Phil Leef    Created.
//--

#include "generic_types.h"

/* W A R N I N G - W A R N I N G - W A R N I N G - W A R N I N G
 *
 * Writing to GPIO pins can cause undesirable affects to the SP. 
 * Please take caution, and be sure you know what you're doing before
 * using this header file. 
 *
 * W A R N I N G - W A R N I N G - W A R N I N G - W A R N I N G */

/*********************************************************************** 
 * The GPIO pins defined, via the method below, correlate to Hardware
 * Engineering specification documents.
 *
 * For example, GPIOs defined in the SuperIO (SIO) are referened by their
 * Port/Pin combination. Each port is referenced by an 8-bit register. 
 * Thus, pins in the SIO will never exceed x7. 
 * (ie: 00, 01, ..., 07, 10, 11, 12, ..., 17, 20, 21, ...., 27 ...)
 * Values of 08 or 18 (for example) are illegal in the SIO. 
 ***********************************************************************/

/* Three items define a signal:
 * 1) The device where the pin resides (ie: SUPERIO, SOUTHBRIDGE, FWH)
 * 2) The pin number AND
 * 3) Whether the pin is active low or not. 
 */

/* Defines the Type of GPIO access */

typedef enum _GPIO_DEVICE_TYPES
{
    SOUTHBRIDGE,
    SUPERIO,
    FWH
} GPIO_DEVICE_TYPES;

typedef enum _GPIO_SIGNAL_NAME
{
    GPIO_INVALID,       // Must be First Item

    /* Power Supply */
    GPIO_PSA0_INS,
    GPIO_PSA1_INS,
    GPIO_PSB0_INS,
    GPIO_PSB1_INS,
    GPIO_PSA0_DC_GOOD,
    GPIO_PSA1_DC_GOOD,
    GPIO_PSB0_DC_GOOD,
    GPIO_PSB1_DC_GOOD,
    GPIO_PSA0_EPOW,
    GPIO_PSA1_EPOW,
    GPIO_PSB0_EPOW,
    GPIO_PSB1_EPOW,

    GPIO_PS0_INS,
    GPIO_PS1_INS,
    GPIO_PS0_DC_GOOD,
    GPIO_PS1_DC_GOOD,
    GPIO_PS0_EPOW,
    GPIO_PS1_EPOW,
    GPIO_PS0_SMBALERT,
    GPIO_PS1_SMBALERT,

    /* CMI / Heart-Beat */
    GPIO_ALT_CMI_TX,
    GPIO_ALT_CMI_RX,

    GPIO_PLX00_FATAL_ERR,
    GPIO_PLX01_FATAL_ERR,
    GPIO_PLX10_FATAL_ERR,
    GPIO_PLX11_FATAL_ERR,
    GPIO_PLX20_FATAL_ERR,
    GPIO_PLX21_FATAL_ERR,
    GPIO_PLX30_FATAL_ERR,
    GPIO_PLX31_FATAL_ERR,

    GPIO_PLX0_FATAL_ERR,
    GPIO_PLX1_FATAL_ERR,
    GPIO_PLX2_FATAL_ERR,
    GPIO_PLX3_FATAL_ERR,
    GPIO_PLX0_RESET,
    GPIO_PLX1_RESET,
    GPIO_PLX2_RESET,
    GPIO_PLX3_RESET,

    /* SMBus */

    GPIO_I2C_GLBL_BUS0_RST,
    GPIO_I2C_GLBL_BUS1_RST,
    GPIO_PS0_I2C_RST,
    GPIO_PS1_I2C_RST,

    /* Slot / Engine ID */
    GPIO_SLOTB,
    GPIO_SLOTA,

    /* Fault Expander */
    GPIO_FAULT_REG_0,
    GPIO_FAULT_REG_1,
    GPIO_FAULT_REG_2,
    GPIO_FAULT_REG_3,
    GPIO_FAULT_REG_4,
    GPIO_FAULT_REG_5,
    GPIO_FAULT_REG_6,
    GPIO_FAULT_REG_7,

    /* Inserted Status */
    GPIO_PEER_INS,

    /* System LEDs */
    GPIO_ENCL_FLT_LED,
    GPIO_BLU_YEL_CTRL_LED,
    GPIO_PEER_FLT_LED,
    GPIO_PEER_ENCL_FLT_LED,
    GPIO_SP_PWR_LED_BLINK,
    GPIO_MEM_FLT_LED,
    GPIO_BEM_BBU_FLT_LED,

    /* IO Port Status/Control */
    GPIO_10G_RST,
    GPIO_10G_A_LED_BYPASS,
    GPIO_10G_B_LED_BYPASS,
    GPIO_10G_C_LED_BYPASS,
    GPIO_10G_D_LED_BYPASS,
    GPIO_SAS1_LED_BYPASS,
    GPIO_SAS1_LED_BLINK,
    GPIO_1G_P0_LED_BYPASS,
    GPIO_1G_P1_LED_BYPASS,
    GPIO_1G_LED_BLINK,

    GPIO_SB_DPLX_EN_P0,
    GPIO_SB_DPLX_EN_P1,

    GPIO_SPC_RST,
    GPIO_MSASHD_P0_INS,
    GPIO_MSASHD_P1_INS,
    GPIO_SAS_P0_LED_BYPASS,
    GPIO_SAS_P1_LED_BYPASS,

    GPIO_HILDA_RST,
    GPIO_HILDA_PA_BLINK,
    GPIO_HILDA_PB_BLINK,
    GPIO_HILDA_PA_BYPASS,
    GPIO_HILDA_PB_BYPASS,
    GPIO_HILDA_PA_GLINK,
    GPIO_HILDA_PB_GLINK,
    GPIO_HILDA_PA_GBYPASS,
    GPIO_HILDA_PB_GBYPASS,
    GPIO_HILDA_SFP_MUX_SEL,

    /* FPGA Programming */
    GPIO_FPGA_DCLK0,
    GPIO_FPGA_DATA0,
    GPIO_FPGA_CONFIG0,
    GPIO_FPGA_STATUS0,
    GPIO_FPGA_CONFIG_DONE,
    GPIO_FPGA0_CBT_OE,
    GPIO_FPGA1_CBT_OE,
    GPIO_FPGA2_CBT_OE,
    GPIO_FPGA3_CBT_OE,

    /* Thermal */
    GPIO_DDR_VRS_HOT,
    GPIO_CPUS_MEM_HOT,
    GPIO_CPUS_VRHOT,
    GPIO_CPUS_THERMTRIP,
    GPIO_CPUS_PROCHOT,
    GPIO_CPU0_MEM01_HOT,
    GPIO_CPU0_MEM23_HOT,
    GPIO_CPU0_VRHOT,
    GPIO_CPU0_VR_AB_HOT,
    GPIO_CPU0_VR_CD_HOT,
    GPIO_CPU0_THERMTRIP,
    GPIO_CPU1_MEM01_HOT,
    GPIO_CPU1_MEM23_HOT,
    GPIO_CPU1_VRHOT,
    GPIO_CPU1_VR_AB_HOT,
    GPIO_CPU1_VR_CD_HOT,
    GPIO_CPU1_THERMTRIP,
    GPIO_MEM_HOT_C01,
    GPIO_MEM_HOT_C23,
    GPIO_BMC_MEMS_HOT,

    GPIO_BMC_MEM0_HOT_C01,
    GPIO_BMC_MEM0_HOT_C23,
    GPIO_BMC_MEM1_HOT_C01,
    GPIO_BMC_MEM1_HOT_C23,
    GPIO_PCH_THERMTRIP,

    GPIO_FAN0_INS,
    GPIO_FAN1_INS,
    GPIO_FAN2_INS,
    GPIO_FAN3_INS,
    GPIO_FAN4_INS,
    GPIO_FAN5_INS,

    GPIO_FAN0_FLT_LED,
    GPIO_FAN1_FLT_LED,
    GPIO_FAN2_FLT_LED,
    GPIO_FAN3_FLT_LED,
    GPIO_FAN4_FLT_LED,
    GPIO_FAN5_FLT_LED,

    GPIO_CMA0_INS,
    GPIO_CMA1_INS,
    GPIO_CMA2_INS,
    GPIO_CMB0_INS,
    GPIO_CMB1_INS,
    GPIO_CMB2_INS,

    GPIO_CMA0_FLT,
    GPIO_CMA1_FLT,
    GPIO_CMA2_FLT,
    GPIO_CMB0_FLT,
    GPIO_CMB1_FLT,
    GPIO_CMB2_FLT,

    GPIO_CMA0_FLT_LED,
    GPIO_CMA1_FLT_LED,
    GPIO_CMA2_FLT_LED,
    GPIO_CMB0_FLT_LED,
    GPIO_CMB1_FLT_LED,
    GPIO_CMB2_FLT_LED,

    /* Misc */
    GPIO_MCU_FORCE_SMI,
    GPIO_SIO_FORCE_SMI,
    GPIO_BTN_IS_SMI,
    GPIO_PEER_SPS_ENABLED,
    GPIO_RST_CPU_EAR,
    GPIO_ADR_START,
    GPIO_ADR_TRIGGER,
    GPIO_ADR_COMPLETE,

    GPIO_BOARD_INSERT,
    GPIO_SXP_RST_REQ,
    GPIO_SXP_DBG_EN,
    GPIO_LOW_PWR_MODE,
    GPIO_PEER_PRSNT_BA,
    GPIO_ENCL_ID_C,
    GPIO_NVDIMM_SAVE,
    GPIO_BIOS_ALIVE,
    GPIO_CMI_CABLE_PRSNT,

    GPIO_BMC_PLTRST_EN,
    GPIO_IO_SHED,
    GPIO_PCH_LP_VAULT,
    GPIO_PCH_HDA_SDO,

    GPIO_10G_LP_MODE,
    GPIO_CMD_LP_MODE,
    GPIO_SXP_LP_MODE,

    /* BMC */
    GPIO_BMC_RESET,
    GPIO_SRST_TO_BMC,
    GPIO_BMC_READY,
    GPIO_PEER_BMC_READY,

    /* PIB (Longhaul) */
    GPIO_PIB_INS,
    GPIO_PIB_FLT_LED,

    /* BBU */
    GPIO_BBU_INSERT,
    GPIO_BBU_ENABLE,
    GPIO_BBU_READY,
    GPIO_BBU_TWI_ALERT,

    /* BOB */
    GPIO_BOB_ENABLE,
    GPIO_BOB_ACTIVE,
    GPIO_BOB_STATUS,

    GPIO_BOB0_INS,
    GPIO_BOB1_INS,
    GPIO_BOB2_INS,

    GPIO_BOB0_FLT_LED,
    GPIO_BOB1_FLT_LED,
    GPIO_BOB2_FLT_LED,

    /* eSLIC */
    GPIO_ESLIC_INS,
    GPIO_ESLIC_FLT_LED,
    GPIO_ESLIC_RST,

    /* Cache Card */
    GPIO_EHORNET_INS,
    GPIO_EHORNET_PWREN,

    /* MGMT Modules */
    GPIO_MGMTA_INS,
    GPIO_MGMTB_INS,
    GPIO_MGMT_FLT,
    GPIO_MGMT_RST,
    GPIO_MGMT_PWRGD,
    GPIO_MGMT_PWREN,

    /* BEM */
    GPIO_BEM_INS,
    GPIO_BEM_FLT_LED,
    GPIO_BEM_RST,
    GPIO_BEM_PWRGD,
    GPIO_BEM_PWREN,
    GPIO_BEM_WIDTH0,
    GPIO_BEM_WIDTH1,
    GPIO_BEM_WIDTH2,
    GPIO_BEM_ECB_TRIP,
    GPIO_BEM_PEER_PWR_FLT,

    /* SLICs */
    GPIO_SLIC0_INS,
    GPIO_SLIC0_FLT_LED,
    GPIO_SLIC0_RST,
    GPIO_SLIC0_PWRGD,
    GPIO_SLIC0_PWREN,
    GPIO_SLIC0_WIDTH0,
    GPIO_SLIC0_WIDTH1,
    GPIO_SLIC0_WIDTH2,
    GPIO_SLIC0_33MHZ_EN,

    GPIO_SLIC1_INS,
    GPIO_SLIC1_FLT_LED,
    GPIO_SLIC1_RST,
    GPIO_SLIC1_PWRGD,
    GPIO_SLIC1_PWREN,
    GPIO_SLIC1_WIDTH0,
    GPIO_SLIC1_WIDTH1,
    GPIO_SLIC1_WIDTH2,
    GPIO_SLIC1_33MHZ_EN,

    GPIO_SLIC2_INS,
    GPIO_SLIC2_FLT_LED,
    GPIO_SLIC2_RST,
    GPIO_SLIC2_PWRGD,
    GPIO_SLIC2_PWREN,
    GPIO_SLIC2_WIDTH0,
    GPIO_SLIC2_WIDTH1,
    GPIO_SLIC2_WIDTH2,
    GPIO_SLIC2_33MHZ_EN,

    GPIO_SLIC3_INS,
    GPIO_SLIC3_FLT_LED,
    GPIO_SLIC3_RST,
    GPIO_SLIC3_PWRGD,
    GPIO_SLIC3_PWREN,
    GPIO_SLIC3_WIDTH0,
    GPIO_SLIC3_WIDTH1,
    GPIO_SLIC3_WIDTH2,
    GPIO_SLIC3_33MHZ_EN,

    GPIO_SLIC4_INS,
    GPIO_SLIC4_FLT_LED,
    GPIO_SLIC4_RST,
    GPIO_SLIC4_PWRGD,
    GPIO_SLIC4_PWREN,
    GPIO_SLIC4_WIDTH0,
    GPIO_SLIC4_WIDTH1,
    GPIO_SLIC4_WIDTH2,
    GPIO_SLIC4_33MHZ_EN,

    GPIO_SLIC5_INS,
    GPIO_SLIC5_FLT_LED,
    GPIO_SLIC5_RST,
    GPIO_SLIC5_PWRGD,
    GPIO_SLIC5_PWREN,
    GPIO_SLIC5_WIDTH0,
    GPIO_SLIC5_WIDTH1,
    GPIO_SLIC5_WIDTH2,
    GPIO_SLIC5_33MHZ_EN,

    GPIO_SLIC6_INS,
    GPIO_SLIC6_FLT_LED,
    GPIO_SLIC6_RST,
    GPIO_SLIC6_PWRGD,
    GPIO_SLIC6_PWREN,
    GPIO_SLIC6_WIDTH0,
    GPIO_SLIC6_WIDTH1,
    GPIO_SLIC6_WIDTH2,
    GPIO_SLIC6_33MHZ_EN,

    GPIO_SLIC7_INS,
    GPIO_SLIC7_FLT_LED,
    GPIO_SLIC7_RST,
    GPIO_SLIC7_PWRGD,
    GPIO_SLIC7_PWREN,
    GPIO_SLIC7_WIDTH0,
    GPIO_SLIC7_WIDTH1,
    GPIO_SLIC7_WIDTH2,
    GPIO_SLIC7_33MHZ_EN,

    GPIO_SLIC8_INS,
    GPIO_SLIC8_FLT_LED,
    GPIO_SLIC8_RST,
    GPIO_SLIC8_PWRGD,
    GPIO_SLIC8_PWREN,
    GPIO_SLIC8_WIDTH0,
    GPIO_SLIC8_WIDTH1,
    GPIO_SLIC8_WIDTH2,
    GPIO_SLIC8_33MHZ_EN,

    GPIO_SLIC9_INS,
    GPIO_SLIC9_FLT_LED,
    GPIO_SLIC9_RST,
    GPIO_SLIC9_PWRGD,
    GPIO_SLIC9_PWREN,
    GPIO_SLIC9_WIDTH0,
    GPIO_SLIC9_WIDTH1,
    GPIO_SLIC9_WIDTH2,
    GPIO_SLIC9_33MHZ_EN,

    GPIO_SLIC10_INS,
    GPIO_SLIC10_FLT_LED,
    GPIO_SLIC10_RST,
    GPIO_SLIC10_PWRGD,
    GPIO_SLIC10_PWREN,
    GPIO_SLIC10_WIDTH0,
    GPIO_SLIC10_WIDTH1,
    GPIO_SLIC10_WIDTH2,
    GPIO_SLIC10_33MHZ_EN,

    GPIO_SLIC11_INS,
    GPIO_SLIC11_FLT_LED,
    GPIO_SLIC11_RST,
    GPIO_SLIC11_PWRGD,
    GPIO_SLIC11_PWREN,

    /* Drives */
    GPIO_MSATA0_INS,
    GPIO_MSATA1_INS,
    GPIO_MSATA_RST,
    GPIO_MSATA_PWRDN,

    GPIO_DRIVE0_INS,
    GPIO_DRIVE0_FLT_LED,
    GPIO_DRIVE0_RST,
    GPIO_DRIVE0_PWRDN,

    GPIO_DRIVE1_INS,
    GPIO_DRIVE1_FLT_LED,
    GPIO_DRIVE1_RST,
    GPIO_DRIVE1_PWRDN,

    GPIO_DRIVE2_INS,
    GPIO_DRIVE2_FLT_LED,
    GPIO_DRIVE2_RST,
    GPIO_DRIVE2_PWRDN,

    GPIO_DRIVE3_INS,
    GPIO_DRIVE3_FLT_LED,
    GPIO_DRIVE3_RST,
    GPIO_DRIVE3_PWRDN,


    /* FSB / MCH Errors */
    GPIO_FSB0_IERR,
    GPIO_CAT_ERR,
    GPIO_SB_ERR0,
    GPIO_SB_ERR1,
    GPIO_SB_ERR2,
    GPIO_CPU0_IVT_ID,
    GPIO_CPU1_IVT_ID,
    GPIO_CPU_FIVR_FAULT,

    GPIO_MAX_GPIO_PINS      // Must be Last Item
} GPIO_SIGNAL_NAME;

#if defined(__cplusplus)
/* use extern C++ here to trick other C++ files including this as extern C
 */
extern "C++"{
/* Define a postfix increment operator for GPIO_SIGNAL_NAME
 */
inline _GPIO_SIGNAL_NAME operator++( GPIO_SIGNAL_NAME &gpio, int )
{
   return gpio = (GPIO_SIGNAL_NAME)((int)gpio + 1);
}
}
#endif
/*************************
 * Maximum number of pins
 * in a port of the SIO
 *************************/
#define GPIO_MAX_GPIO_PINS_PER_PORT 8

/*********************
 * LED GPIO definitions
 *********************/
#define LED_COLOR_AMBER     FALSE
#define LED_COLOR_BLUE      TRUE

typedef enum _LED_COLOR_TYPE
{
    LED_COLOR_TYPE_INVALID,
    LED_COLOR_TYPE_BLUE,
    LED_COLOR_TYPE_GREEN,
    LED_COLOR_TYPE_AMBER,
    LED_COLOR_TYPE_BLUE_GREEN_ALT,
    LED_COLOR_TYPE_BLUE_GREEN_SYNC,
    LED_COLOR_TYPE_AMBER_GREEN_ALT,
    LED_COLOR_TYPE_AMBER_GREEN_SYNC,
} LED_COLOR_TYPE;

/***************************
 * struct: LED_BLINK_RATE
 *
 * Defines all possible blink 
 * states for the LED light. 
 ***************************/
typedef enum _LED_BLINK_RATE
{
    LED_BLINK_OFF       = 0x00,
    LED_BLINK_0_25HZ    = 0x01,
    LED_BLINK_0_50HZ    = 0x02,
    LED_BLINK_1HZ       = 0x03,
    LED_BLINK_2HZ       = 0x04,
    LED_BLINK_3HZ       = 0x05,
    LED_BLINK_4HZ       = 0x06,
    LED_BLINK_ON        = 0x07,
    LED_BLINK_INVALID   = 0xFF

} LED_BLINK_RATE;

//++
// Type:
//      GPIO_REQUEST
//
// Description:
//      Structure that defines the necessary data to yield the value of a GPIO
//      pin. At the time this is written, that data includes:
//
// Values:
//
//--

typedef struct _GPIO_REQUEST
{
    GPIO_SIGNAL_NAME    gpioSignal;     // Signal name of the GPIO
    BOOL                pinValue;       // Value of the pin (for Set func)
    LED_BLINK_RATE      blinkRate;      // Blink Rate (if GPIO is an LED)
} GPIO_REQUEST, *PGPIO_REQUEST;

#define GPIO_REQUEST_INIT(m_gpio_req_p,m_gpio_signal_name,m_pin_value, m_blink_rate)\
    ((m_gpio_req_p)->gpioSignal = (m_gpio_signal_name),     \
     (m_gpio_req_p)->pinValue   = (m_pin_value),            \
     (m_gpio_req_p)->blinkRate  = (m_blink_rate))

//++
// Type:
//      GPIO_ATTR_REQUEST
//
// Description:
//      Structure that defines the necessary data to yield the value of a GPIO
//      pin. At the time this is written, that data includes:
//          * requested pin number
//          * Active Low / Active High
//          * Device where the pin resides (ie: SIO, Southbridge, FWH)
//
// Values:
//
//--

typedef struct _GPIO_ATTR_REQUEST
{
    GPIO_SIGNAL_NAME    gpioSignal; // Signal name of the GPIO
    char                niceName[40];
    ULONG               reqPinNum;  // Requested Pin Number
    BOOL                activeLow;  // Active Low / Active High (for Get func)
    BOOL                pinValue;   // Value of the pin (for Set func)
    BOOL                writeable;  // Input / Output status
    BOOL                outputOnly; // Read pin value from output register of BMC
    BOOL                valid;      // Validity for a given platform
    GPIO_DEVICE_TYPES   DevType;    // Device where the pin lives
} GPIO_ATTR_REQUEST, *PGPIO_ATTR_REQUEST;

#define GPIO_REQUEST_ATTR_INIT(m_gpio_req_p,m_gpio_signal_name,m_req_pin,m_active_low,m_pin_value,m_dev_type)\
    ((m_gpio_req_p)->gpioSignal = (m_gpio_signal_name),     \
     (m_gpio_req_p)->niceName   = NULL,                     \
     (m_gpio_req_p)->reqPinNum  = (m_req_pin),              \
     (m_gpio_req_p)->activeLow  = (m_active_low),           \
     (m_gpio_req_p)->pinValue   = (m_pin_value),            \
     (m_gpio_req_p)->writeable  = FALSE,                    \
     (m_gpio_req_p)->outputOnly = FALSE,                    \
     (m_gpio_req_p)->valid      = FALSE,                    \
     (m_gpio_req_p)->DevType    = (m_dev_type))

/*****************************************************/
#endif //__GPIO_SIGNALS__
// End gpio_signals.h
