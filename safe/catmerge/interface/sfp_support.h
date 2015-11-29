/***************************************************************************
 * Copyright (C) EMC Corp 2003 - 2014
 *
 * All rights reserved.
 * 
 * This software contains the intellectual property of EMC Corporation or
 * is licensed to EMC Corporation from third parties. Use of this software
 * and the intellectual property contained therein is expressly limited to
 * the terms and conditions of the License Agreement under which it is
 * provided by or on behalf of EMC
 **************************************************************************/

/***********************************************************************
 *
 ***********************************************************************
 *
 * DESCRIPTION:
 *      This file contains all structures and defines required to
 *      interface with the SFPs and support the
 *      CPD_IOCTL_GET_MEDIA_INTERFACE_INFO
 *
 * NOTES:
 *
 * HISTORY:
 *  2008-07-17 Created by John Pelletier
 *  2009-03-19 Modified by AGN
 *
 ***********************************************************************/
#ifndef _SFP_SUPPORT_H_
#define _SFP_SUPPORT_H_

#include "generic_types.h"

/************************************************************************
 * for optical iscsi ports
 ***********************************************************************/
#define SFP_SYS_ENTRY_ROOT "/sys/class/net/"
#define SFP_INFO_MAX_LEN (MAX_SERIAL_ID_BYTES * 2)

/************************************************************************
 * for single mode sfp support
 ***********************************************************************/
typedef enum _SFP_MEDIA_TYPE
{
    SFP_MEDIA_NA            = 0x00,
    SFP_MEDIA_SINGLE_MODE   = 0x01,
    SFP_MEDIA_MULTI_MODE_M5 = 0x04,
    SFP_MEDIA_MULTI_MODE_M6 = 0x08,
    SFP_MEDIA_MULTI_MODE    = SFP_MEDIA_MULTI_MODE_M5 | SFP_MEDIA_MULTI_MODE_M6,
} SFP_MEDIA_TYPE;

/************************************************************************
 * phys_dev defines (A0, byte 0)
 ***********************************************************************/
#define     SFP_DEVICE_TYPE_UNK         (0)
#define     SFP_DEVICE_TYPE_GBIC        (1)
#define     SFP_DEVICE_TYPE_SOLDERED    (2)
#define     SFP_DEVICE_TYPE_SFP         (3)

/* Bit defs for A0, byte 3 */
#define SFP_TEN_GIG_BASE_SR           (1<<4)
#define SFP_TEN_GIG_BASE_LR           (1<<5)
#define SFP_TEN_GIG_BASE_LRM          (1<<6)

/* Bit defs for A0, byte 10 */
#define NUMBER_OF_FIBRE_CHANNEL_SPEEDS 8
typedef enum _SFP_FIBRE_CHANNEL_SPEED
{
    SFP_FC_SPEED_100MBps            = (1<<0),
    SFP_FC_SPEED_200MBps            = (1<<2),
    SFP_FC_SPEED_400MBps            = (1<<4),
    SFP_FC_SPEED_1600MBps           = (1<<5),
    SFP_FC_SPEED_800MBps            = (1<<6),
    SFP_FC_SPEED_1200MBps           = (1<<7)
} SFP_FIBRE_CHANNEL_SPEED;

/* Bit defs for A2, byte 112 and 113 */
#define NUMBER_OF_ALARMS_OR_WARNINGS 16
typedef enum _SFP_ALARM_AND_WARNING_FLAG
{
    SFP_TX_POWER_LO                 = (1<<0),
    SFP_TX_POWER_HI                 = (1<<1),
    SFP_TX_BIAS_LO                  = (1<<2),
    SFP_TX_BIAS_HI                  = (1<<3),
    SFP_VCC_LO                      = (1<<4),
    SFP_VCC_HI                      = (1<<5),
    SFP_TEMP_LO                     = (1<<6),
    SFP_TEMP_HI                     = (1<<7),
    SFP_RX_POWER_LO                 = (1<<14),
    SFP_RX_POWER_HI                 = (1<<15)
} SFP_ALARM_AND_WARNING_FLAG;

/* tells whether the function was called by timer poll or user ioctl */
#define SFP_DIAGNOSE_CALLED_BY_POLL  1
#define SFP_DIAGNOSE_CALLED_BY_MII   2


#define SFP_ALARMS_PRESENT(m_p)     ( (m_p->alarms[0]   & 0xFF) || (m_p->alarms[1]   & 0xC0) )
#define SFP_WARNINGS_PRESENT(m_p)   ( (m_p->warnings[0] & 0xFF) || (m_p->warnings[1] & 0xC0) )

#define SFP_FC_SPEED_ALL_BITS (     SFP_FC_SPEED_100MBps    \
                                    | SFP_FC_SPEED_200MBps  \
                                    | SFP_FC_SPEED_400MBps  \
                                    | SFP_FC_SPEED_800MBps  \
                                    | SFP_FC_SPEED_1600MBps \
                                    | SFP_FC_SPEED_1200MBps )

#define SFP_DIAGS_ALL_BITS (        SFP_TX_POWER_LO     \
                                    | SFP_TX_POWER_HI   \
                                    | SFP_TX_BIAS_LO    \
                                    | SFP_TX_BIAS_HI    \
                                    | SFP_VCC_LO        \
                                    | SFP_VCC_HI        \
                                    | SFP_TEMP_LO       \
                                    | SFP_TEMP_HI       \
                                    | SFP_RX_POWER_LO   \
                                    | SFP_RX_POWER_HI   )

#define SFP_TEXT_TX_POWER_HI        "tx power high"
#define SFP_TEXT_TX_POWER_LO        "tx power low"
#define SFP_TEXT_TX_BIAS_HI         "tx bias high"
#define SFP_TEXT_TX_BIAS_LO         "tx bias low"
#define SFP_TEXT_VCC_HI             "vcc high"
#define SFP_TEXT_VCC_LO             "vcc low"
#define SFP_TEXT_TEMP_HI            "temp high"
#define SFP_TEXT_TEMP_LO            "temp low"
#define SFP_TEXT_RX_POWER_LO        "rx power low"
#define SFP_TEXT_RX_POWER_HI        "rx power high"
#define SFP_TEXT_RX_LOS             "rx loss of signal"
#define SFP_TEXT_RX_SIGNAL          "rx has a signal"
#define SFP_TEXT_TX_DISABLED        "tx disabled"
#define SFP_TEXT_SFP_REMOVED        "sfp is not present"
#define SFP_TEXT_IOM_REMOVED        "io module was removed"
#define SFP_TXT_BAD_SFP_INSERTED    "bad sfp is present"
#define SFP_TXT_NEW_SFP_INSERTED    "sfp is present"
#define SFP_TXT_BAD_DIAG_CHECKSUM   "sfp diagnostic checksum failure"
#define SFP_TXT_BAD_CONFIG_CHECKSUM "sfp config checksum failure"
#define SFP_TXT_BAD_EMC_CHECKSUM    "sfp emc checksum failure"
#define SFP_TEXT_ALARM              "alarm"
#define SFP_TEXT_WARNING            "warning"
#define SFP_TXT_UNKNOWN_TYPE        "sfp type is unknown"
#define SFP_TXT_UNSUPPORTED_TYPE    "sfp type is unsupported"
#define SFP_TXT_SPEED_MISMATCH      "sfp speed mismatch"
#define SFP_TXT_SMB_DEVICE_ERROR    "smbus device error"
#define SFP_TXT_EDC_MODE_MISMATCH   "sfp type edc mode mismatch"
#define SFP_TXT_SAS_SPECL_ERROR     "sas sfp eeprom read error"


/************************************************************************
 * defines for sfp_serial_id
 ***********************************************************************/
#define SFP_SERIAL_MODULE   (4)


/************************************************************************
 * defines for sfp_connector
 ***********************************************************************/
#define SFP_CONN_TYPE_UNK       (0x00) //Unknown or unspecified
#define SFP_CONN_TYPE_SC        (0x01) //SC
#define SFP_CONN_TYPE_FC1C      (0x02) //Fibre Channel Style 1 copper connector
#define SFP_CONN_TYPE_FC2C      (0x03) //Fibre Channel Style 2 copper connector
#define SFP_CONN_TYPE_BNC       (0x04) //BNC/TNC
#define SFP_CONN_TYPE_FC_COAX   (0x05) //Fibre Channel coaxial headers
#define SFP_CONN_TYPE_FJACK     (0x06) //FiberJack
#define SFP_CONN_TYPE_LC        (0x07) //LC (!!this is what 10g SFPs will have!!)
#define SFP_CONN_TYPE_MT_RJ     (0x08) //MT-RJ
#define SFP_CONN_TYPE_MU        (0x09) //MU
#define SFP_CONN_TYPE_SG        (0x0A) //SG
#define SFP_CONN_TYPE_OPT       (0x0B) //Optical pigtail
#define SFP_CONN_TYPE_HSSDC     (0x20) //HSSDC II
#define SFP_CONN_TYPE_CPIG      (0x21) //Copper Pigtail

/************************************************************************
 * defines for sfp fc_len_and_tech (bytes 7 & 8)
 ***********************************************************************/
#define SFP_FC_LEN_AND_TECH_COPPER_ACTIVE  (0x08)
#define SFP_FC_LEN_AND_TECH_COPPER_PASSIVE (0x04)

/************************************************************************
 * defines for sfp fc_xmt_media (byte 9)
 ***********************************************************************/
#define SFP_FC_XMT_MEDIA_TWIN_AXIAL_PAIR   (0x80)

/************************************************************************
 * defines for sfp vendor_oui (bytes 37-39)
 ***********************************************************************/
#define SFP_EMC_OUI_BYTE1                  (0x00)
#define SFP_EMC_OUI_BYTE2                  (0x60)
#define SFP_EMC_OUI_BYTE3                  (0x16)

#define SFP_BROCADE_OUI_BYTE1              (0x00)
#define SFP_BROCADE_OUI_BYTE2              (0x05)
#define SFP_BROCADE_OUI_BYTE3              (0x1E)

/************************************************************************
 * defines for sfp_bit_rate
 ***********************************************************************/
#define SFP_BIT_RATE_10G        (0x67) //10.3 Gbit/sec
#define SFP_BIT_RATE_16G        (0x8C) //14.0 Gbit/Sec

//Force Big Endian
#define SFP_MSB (0) //As defined by the SFP MSA for EEPROMs A0 and A2
#define SFP_LSB (1) //
#define sfp_word(m_2b_array) ((m_2b_array[SFP_MSB]<<8)||m_2b_array[SFP_LSB])

/* Connector type ... parallel to CPD_MII_MEDIA_TYPE in cpd_generics.h */
#define CONNECTOR_TYPE_COPPER 1
#define CONNECTOR_TYPE_OPTICAL 2
#define CONNECTOR_TYPE_NAS_COPPER 3
#define CONNECTOR_TYPE_UNKNOWN 4
#define CONNECTOR_TYPE_MINI_SAS_HD 5

/*****************************************************/
/* SFF 8472, OPTICAL and COPPER SFP A0 EEPROM LAYOUT */
/*****************************************************/
#define SFP_CONNECTOR_INDEX 2
#define SFP_10G_ETH_COMPLIANCE_CODE_INDEX 3
#define SFP_CABLE_TECH_INDEX 8
#define SFP_FC_SPEEDS 10
#define SFP_BIT_RATE_INDEX 12

#define SFP_EMC_SFP_A0_CC_BASE_START_INDEX 0
#define SFP_EMC_SFP_A0_CC_BASE_END_INDEX 62
#define SFP_EMC_SFP_A0_CC_BASE_INDEX 63

#define SFP_EMC_SFP_A0_CC_EXT_START_INDEX 64
#define SFP_EMC_SFP_A0_CC_EXT_END_INDEX 94
#define SFP_EMC_SFP_A0_CC_EXT_INDEX 95

#define SFP_VENDOR_NAME_INDEX 20
#define SFP_VENDOR_NAME_LEN 16

#define SFP_VENDOR_PN_INDEX 40
#define SFP_VENDOR_PN_LEN 16

#define SFP_VENDOR_REV_INDEX 56
#define SFP_VENDOR_REV_LEN 4

#define SFP_COPPER_CABLE_COMPLIANCE_INDEX 60
#define SFP_COPPER_PASSIVE_COMPLIANT_SFF_8431           1 //bit 0
#define SFP_COPPER_ACTIVE_COMPLIANT_SFF_8431_LIMITING   4 //bit 2

#define SFP_VENDOR_SN_INDEX 68
#define SFP_VENDOR_SN_LEN 16

#define SFP_EMC_FORMAT_HEADER_INDEX 128
#define SFP_EMC_FORMAT_HEADER_VALUE 0x40

#define SFP_EMC_CHECKSUM_SEED 0x5A5A5A5A
#define SFP_EMC_CHECKSUM_START_INDEX 128
#define SFP_EMC_CHECKSUM_END_INDEX 243
#define SFP_EMC_CHECKSUM_INDEX 244

#define SFP_EMC_PN_LEN_INDEX 133
#define SFP_EMC_PN_INDEX 134

#define SFP_EMC_REV_LEN_INDEX 145
#define SFP_EMC_REV_INDEX 146

#define SFP_EMC_SN_LEN_INDEX 149
#define SFP_EMC_SN_INDEX 150

#define SFP_VENDOR_OUI_BYTE1_INDEX 37
#define SFP_VENDOR_OUI_BYTE2_INDEX 38
#define SFP_VENDOR_OUI_BYTE3_INDEX 39

#define SFP_EMC_SFP_A2_CC_DMI_START_INDEX 0
#define SFP_EMC_SFP_A2_CC_DMI_END_INDEX 94
#define SFP_EMC_SFP_A2_CC_DMI_INDEX 95

#define SFP_ALARMS_INDEX 112
#define SFP_WARNINGS_INDEX 116

#define SFP_CISCO_PID_INDEX 192
#define SFP_CISCO_PID_LEN 20
#define SFP_CISCO_PID_PREFIX_LEN 13
#define SFP_CISCO_PID_PREFIX  "SFP-H10GB-ACU"

/*****************************************************/
/* SFF 8636, MINISAS PG00 EEPROM LAYOUT              */
/*****************************************************/
#define SFP_MINI_SAS_HD_CONNECTOR_INDEX 130
#define SFP_CONN_TYPE_MINI_SAS_HD (0x24)

#define SFP_MINI_SAS_HD_COMPLIANCE_CODE_INDEX 133

#define SFP_EMC_MINI_SAS_HD_CC_BASE_START_INDEX 128
#define SFP_EMC_MINI_SAS_HD_CC_BASE_END_INDEX 190
#define SFP_EMC_MINI_SAS_HD_CC_BASE_INDEX 191

#define SFP_EMC_MINI_SAS_HD_CC_EXT_START_INDEX 192
#define SFP_EMC_MINI_SAS_HD_CC_EXT_END_INDEX 222
#define SFP_EMC_MINI_SAS_HD_CC_EXT_INDEX 223

#define SFP_MINI_SAS_HD_VENDOR_NAME_INDEX 148
#define SFP_MINI_SAS_HD_VENDOR_NAME_LEN 16

#define SFP_MINI_SAS_HD_VENDOR_PN_INDEX 168
#define SFP_MINI_SAS_HD_VENDOR_PN_LEN 16

#define SFP_MINI_SAS_HD_VENDOR_REV_INDEX 184
#define SFP_MINI_SAS_HD_VENDOR_REV_LEN 2

#define SFP_MINI_SAS_HD_VENDOR_SN_INDEX 196
#define SFP_MINI_SAS_HD_VENDOR_SN_LEN 16

#define SFP_MINI_SAS_HD_EMC_PN_INDEX 168
#define SFP_MINI_SAS_HD_EMC_PN_LEN 16

#define SFP_MINI_SAS_HD_EMC_REV_INDEX 184
#define SFP_MINI_SAS_HD_EMC_REV_LEN 2

#define SFP_MINI_SAS_HD_EMC_SN_INDEX 196
#define SFP_MINI_SAS_HD_EMC_SN_LEN 16

#define SFP_MINI_SAS_HD_VENDOR_OUI_BYTE1_INDEX 165
#define SFP_MINI_SAS_HD_VENDOR_OUI_BYTE2_INDEX 166
#define SFP_MINI_SAS_HD_VENDOR_OUI_BYTE3_INDEX 167

#define SFP_MINI_SAS_HD_BIT_RATE_INDEX 140

#define SFP_MINI_SAS_HD_SAS_SATA_COMPLIANCE_CODES 133

#define SFP_SAS_SPEED_ALL_BITS ( SFP_MINI_SAS_HD_SAS_3G | SFP_MINI_SAS_HD_SAS_6G )

#define SFP_MINI_SAS_HD_SAS_SPEEDS (1<<4) | (1<<5)
#define SFP_MINI_SAS_HD_SAS_3G (1<<4)
#define SFP_MINI_SAS_HD_SAS_6G (1<<5)

#define SFP_MINI_SAS_HD_BIT_RATE_6G (0x3C) //6 Gbit/sec

#endif  /* _SFP_SUPPORT_H */
