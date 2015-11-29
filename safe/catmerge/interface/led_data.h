#ifndef LED_DATA_H
#define LED_DATA_H

/************************************************************************************
 * Copyright (C) EMC Corporation  2007
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 * Unpublished - all rights reserved under the copyright laws of the United States
 ***********************************************************************************/

/************************************************************************************
 *  led_data.h
 ************************************************************************************
 *
 * DESCRIPTION:
 *   This file is the header file that contains typedefs and defines used by
 *   the various functions related to LED's.
 *
 *  HISTORY:
 *      13-Aug-2007     Joe Perry   Created.
 *
 ***********************************************************************************/

#include "gpio_signals.h"

/*
 * Identifiers for various LED's
 */
typedef enum led_ctrl_type_tag
{
    LED_ID_IOPORT = 0,
    LED_ID_IOMODULE,
    LED_ID_UNSAFE_REMOVAL,
    LED_ID_XPE_FAULT,
    LED_ID_DAE_FAULT,
    LED_ID_SP_FAULT,
    LED_ID_IOPORTS,              // used for marking all Ports of an I/O Module
    LED_ID_IOPORTS_AND_MODULE
}
LED_ID_TYPE;

/*
 * Various colors of array LED's
 */
typedef enum led_general_color_type_tag
{
    LED_GEN_COLOR_BLUE = 0,
    LED_GEN_COLOR_GREEN,
    LED_GEN_COLOR_AMBER,
    LED_GEN_COLOR_WHITE,
    LED_GEN_COLOR_GREEN_BLUE
} LED_GEN_COLOR_TYPE;

/*
 * Various blink rates of array LED's (non-hardware specific)
 */
typedef enum led_general_blink_type_tag
{
    LED_GEN_BLINK_OFF       = 0,
    LED_GEN_BLINK_0_25HZ,
    LED_GEN_BLINK_0_50HZ,
    LED_GEN_BLINK_1HZ,
    LED_GEN_BLINK_2HZ,
    LED_GEN_BLINK_3HZ,
    LED_GEN_BLINK_4HZ,
    LED_GEN_BLINK_ON,
    LED_GEN_BLINK_16HZ
} LED_GEN_BLINK_RATE;

typedef enum fault_led_general_control
{
    FAULT_LED_GEN_CTRL_OFF,
    FAULT_LED_GEN_CTRL_ON,
    FAULT_LED_GEN_CTRL_FLASH_OFF,
    FAULT_LED_GEN_CTRL_FLASH_ON
}FAULT_LED_GENERAL_CONTROL;

/*
 * Various actions for I/O Port LED's
 */
typedef enum led_ioport_action_tag
{
    LED_IOPORT_NO_ACTION,
    LED_IOPORT_MARK_ON_1HZ,
    LED_IOPORT_MARK_ON_16HZ,
    LED_IOPORT_MARK_OFF,
    LED_IOPORT_SFP_FAULT_ON,
    LED_IOPORT_SFP_FAULT_OFF,
    LED_IOPORT_NO_SFP,
    LED_IOPORT_2G_SPEED,
    LED_IOPORT_4G_SPEED
} LED_IOPORT_ACTION;

/* 
 * Various actions for IO Module Fault LED's
 */
typedef enum led_iomodule_flt_action_tag
{
    LED_IOMODULE_FLT_NO_ACTION,
    LED_IOMODULE_FLT_MARK_ON,
    LED_IOMODULE_FLT_MARK_OFF
} LED_IOMODULE_FLT_ACTION;


/*
 * This structure holds the data related to controling I/O Port LED's
 */
typedef struct _LED_BLINK_DATA
{
    LED_IOPORT_ACTION   portAction;
    LED_GEN_BLINK_RATE  BlinkRate;
    LED_GEN_COLOR_TYPE  BlinkColor;
} LED_BLINK_DATA,*PLED_BLINK_DATA;


#endif  // ifndef LED_DATA_H
