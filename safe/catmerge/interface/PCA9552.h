#ifndef PCA9552_H
#define PCA9552_H

/***************************************************************************
 * Copyright (C) EMC Corporation 2005
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/****************************************************************
 *  PCA9552.h
 ****************************************************************
 *
 *  Description:
 *  This header file contains definitions which define the
 *  registers within the PCA9552 16-bit I2C bus LED driver 
 *  (with programmable blink rates)
 *
 *  History:
 *      Sept-04-2007    Phil Leef   Created
 *
 ****************************************************************/

/* The Frequency Prescalor (PSC) is used to program the period
 * of the Pulse Width Modulation output.
 * The period (in seconds) is defined as: (PSC + 1) / 44
 */
typedef struct
{
    UINT_8E freq_prescalor;
} PCA9552_FREQ_PRESCALOR, *PPCA9552_FREQ_PRESCALOR;

/* The Pulse Width Modulation (PWM) is used to determine the 
 * duty cycle.
 * The duty cycle is defined as: (256 - PWM) / 256
 */
typedef struct
{
    UINT_8E pulse_width_mod;
} PCA9552_PULSE_WIDTH_MOD, *PPCA9552_PULSE_WIDTH_MOD;

/* The LS (LED Select) registers determine the source
 * of the LED data. Either, from PWM0 or PWM1. 
 */

typedef struct
{
    UINT_8E     LSX_LED0:2,
                LSX_LED1:2,
                LSX_LED2:2,
                LSX_LED3:2;
}PCA9552_LED_SELECT, *PPCA9552_LED_SELECT;

typedef union
{
    UINT_8E                 LEDSelectorRegister;
    PCA9552_LED_SELECT      fields;
}PCA9552_LED_SELECT_INFO, *PPCA9552_LED_SELECT_INFO;

#define PCA9552_LED_SELECT_OUTPUT_LOW           0x00
#define PCA9552_LED_SELECT_OUTPUT_HIGH          0x01
#define PCA9552_LED_SELECT_OUTPUT_BLINKS_PWM0   0x02
#define PCA9552_LED_SELECT_OUTPUT_BLINKS_PWM1   0x03

/* Register Offsets for the PCA9552
 */
typedef enum _PCA9552_REG_BLOCK
{
    PCA9552_INPUT0_BLOCK,
    PCA9552_INPUT1_BLOCK,
    PCA9552_PSC0_BLOCK,
    PCA9552_PWM0_BLOCK,
    PCA9552_PSC1_BLOCK,
    PCA9552_PWM1_BLOCK,
    PCA9552_LS0_BLOCK,
    PCA9552_LS1_BLOCK,
    PCA9552_LS2_BLOCK,
    PCA9552_LS3_BLOCK,

    PCA9552_TOTAL_BLOCK,
} PCA9552_REG_BLOCK;

#define PCA9552_AUTO_INCREMENT_BIT  0x10

#define PCA9552_INT_CLOCK_FREQ      0x2C    // 44Hz

#define PCA9552_LS0_OFFSET      0x06

/* The Frequency Prescalor (PSC) register value is defined as: ((clock_frequency * the_period_of_BLINK) - 1) */
#define PCA9552_PSC_0_25_HZ    ((PCA9552_INT_CLOCK_FREQ*4) - 1)
#define PCA9552_PSC_0_50_HZ    ((PCA9552_INT_CLOCK_FREQ*2) - 1)
#define PCA9552_PSC_1_HZ       (PCA9552_INT_CLOCK_FREQ - 1)
#define PCA9552_PSC_2_HZ       ((PCA9552_INT_CLOCK_FREQ/2) - 1)
#define PCA9552_PSC_3_HZ       ((PCA9552_INT_CLOCK_FREQ/3) - 1)
#define PCA9552_PSC_4_HZ       ((PCA9552_INT_CLOCK_FREQ/4) - 1)
#endif //PCA9552_H
