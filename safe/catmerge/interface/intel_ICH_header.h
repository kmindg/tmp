#ifndef __INTEL_ICH_HEADER__
#define __INTEL_ICH_HEADER__

//***************************************************************************
// Copyright (C) EMC Corporation 2009
// All rights reserved.
// Licensed material -- property of EMC Corporation
//**************************************************************************/
//++
// File Name:
//      intel_ICH_header.h
//
// Contents:
//      ICH definitions common to modules like SPID and WDT.
//......This file has been created as part of DIMS 224632 work.
//
// Revision History:
//--

#include "vendorDeviceID.h"

/*******************************
 * PCH - (Platform Controller Hub)
 *  Patsburg & Wellsburg Definitions
 *******************************/

#define PCH_LPC_BUS                     0x00
#define PCH_LPC_DEVICE                  0x1F
#define PCH_LPC_FUNCTION                0x00

#define PCH_PMBASE_OFFSET               0x40
#define PCH_PMBASE_OFFSET_MASK          0x0000FF80  // Bits 15:7

#define PCH_TCOBASE_OFFSET              0x60
#define PCH_TCO1_CTRL_OFFSET            0x08
#define PCH_NMI_NOW_BITMASK             0x0100      // Bit 8

#define PCH_GPIOBASE_OFFSET             0x48
#define PCH_GPIOBASE_OFFSET_MASK        0x0000FF80  // Bits 15:7

#define PCH_GPIO_LVL_OFFSET             0x0C
#define PCH_GPIO_LVL2_OFFSET            0x38
#define PCH_GPIO_LVL3_OFFSET            0x48
#define PCH_GPIO_LVL2_PIN_OFFSET        32
#define PCH_GPIO_LVL3_PIN_OFFSET        64

#define PCH_IS_VALID_LVL1_GPIO(m_pin)            \
    ((((LONG)m_pin >= 0)  && (m_pin <= 31)) ? TRUE : FALSE)

#define PCH_IS_VALID_LVL2_GPIO(m_pin)            \
    (((m_pin >= 32) && (m_pin <= 63)) ? TRUE : FALSE)

#define PCH_IS_VALID_LVL3_GPIO(m_pin)            \
    (((m_pin >= 64) && (m_pin <= 75)) ? TRUE : FALSE)
    

#define PCH_TCO_TMR_OFFSET              (PCH_TCOBASE_OFFSET + 0x12)
#define PCH_TCO_TMR_TMR_INIT_VAL_MASK   (0x3FF)     // Bits 9:0

#define PCH_TCO_RLD_OFFSET              (PCH_TCOBASE_OFFSET)
#define PCH_TCO_DAT_IN_OFFSET           (PCH_TCOBASE_OFFSET + 2)
#define PCH_TCO_DAT_OUT_OFFSET          (PCH_TCOBASE_OFFSET + 3)
#define PCH_TCO1_STS_OFFSET             (PCH_TCOBASE_OFFSET + 4)
#define PCH_TCO2_STS_OFFSET             (PCH_TCOBASE_OFFSET + 6)
#define PCH_TCO1_CNT_OFFSET             (PCH_TCOBASE_OFFSET + 8)

#define PCH_TCO_TMR_MIN_TICKS           (0x02)      // PCH min is 2;
#define PCH_TCO_TMR_MAX_TICKS           (0x3FF)     // PCH max is 0x3ff; 

#define PCH_TCO_RLD_REFRESH             (0x01)      // writing any value to register causes refresh of WDT
#define PCH_TCO_TMR_HLT                 (0x0800)    // Bit 11
#define PCH_TCO1_STS_TIMEOUT            (0x0008)    // Bit 3
#define PCH_TCO1_STS_CLEAR_ALL_MASK     (0x1C09)    // Mask NMI2SMI_STS, TIMEOUT, HUBSMI_STS, HUBNMI_STS, and HUBSERR_STS
#define PCH_TCO2_STS_CLEAR_ALL_MASK     (0x0016)    // Mask SECOND_TO_STS, BOOT_STS, and SMLINK_SLV_SMI_STS
#define PCH_TCO_DAT_OUT_TIMEOUT_MSG     (PCH_TCO1_STS_TIMEOUT)


/*******************************
 * Processor Definitions
 * (Sandybridge, Ivybridge, Haswell)
 *******************************/

#define CPU_MMCFG_REG_BUS               0x00
#define CPU_MMCFG_REG_DEVICE            0x05
#define CPU_MMCFG_REG_FUNCTION          0x00

#define CPU_MMCFG_OFFSET                0x84
#define CPU_MMCFG_OFFSET_HW             0x90
#define CPU_MMCFG_OFFSET_MASK           0xFC000000  // Bits 31:26

/***************************
 * struct: ICH_TYPE
 *
 * Defines the possible ICHs
 ***************************/
typedef enum _ICH_TYPE
{
    ICH_INVALID,
    PCH_PATSBURG,
    PCH_WELLSBURG,
} ICH_TYPE;

#endif
