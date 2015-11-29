#ifndef __SPID_ENUM_TYPES__
#define __SPID_ENUM_TYPES__

//***************************************************************************
// Copyright (C) EMC Corporation 2007, 2010
// All rights reserved.
// Licensed material -- property of EMC Corporation
//**************************************************************************/

//**************************************************************************
//  $Id$
//**************************************************************************

//++
// File Name:
//      spid_enum_types.h
//
// Contents:
//      Definitions of various SPID-related enumerated types.
//      None of the definitions in this file require any data types to be
//      defined, making it safe for any component to #include.
//
// Revision History:
//  15-Jan-07   Mike Hamel        Split apart from spid_types.h
//--


// Description:
//      Enumeration that describes possible hardware types
//
// Values:
//
//	SPID_NON_X1_HW_TYPE: Hardware is something other than X1 or X1 Lite.
//  SPID_X1_HW_TYPE: Hardware is X1 SP.
//  SPID_X1_LITE_HW_TYPE: Hardware is X1 Lite SP.
//
//--

/***************************
 * struct: SPID_HW_TYPE
 *
 * Defines all possible
 * SPID HW Types
 ***************************/

typedef enum _SPID_HW_TYPE
{
    SPID_UNKNOWN_HW_TYPE            = 0x0,
    SPID_X1_HW_TYPE                 = 0x1,
    SPID_X1_LITE_HW_TYPE            = 0x2,
    SPID_CHAM2_HW_TYPE              = 0x3,
    SPID_X1_SUNLITE_HW_TYPE         = 0x4,
    SPID_TARPON_HW_TYPE             = 0x5,
    SPID_BARRACUDA_HW_TYPE          = 0x6,
    SPID_SNAPPER_HW_TYPE            = 0x7,
    SPID_ACADIA_HW_TYPE             = 0x8,
    SPID_GRAND_TETON_HW_TYPE        = 0x9,
    SPID_FRESHFISH_HW_TYPE          = 0xA,
    SPID_YOSEMITE_HW_TYPE           = 0xB,
    SPID_BIG_SUR_HW_TYPE            = 0xC,
    SPID_HAMMERHEAD_HW_TYPE         = 0xD,
    SPID_SLEDGEHAMMER_HW_TYPE       = 0xE,
    SPID_JACKHAMMER_HW_TYPE         = 0xF,
    SPID_VMWARE_HW_TYPE             = 0x10,
    SPID_TACKHAMMER_HW_TYPE         = 0x11,
    SPID_DREADNOUGHT_HW_TYPE        = 0x12,
    SPID_TRIDENT_HW_TYPE            = 0x13,
    SPID_IRONCLAD_HW_TYPE           = 0x14,
    SPID_NAUTILUS_HW_TYPE           = 0x15,
    SPID_BOOMSLANG_HW_TYPE          = 0x16,
    SPID_VIPER_HW_TYPE              = 0x17,
    SPID_COMMUP_HW_TYPE             = 0x18,
    SPID_MAGNUM_HW_TYPE             = 0x19,
    SPID_ZODIAC_HW_TYPE             = 0x1A,
    SPID_CORSAIR_HW_TYPE            = 0x1B,
    SPID_SPITFIRE_HW_TYPE           = 0x1C,
    SPID_LIGHTNING_HW_TYPE          = 0x1D,
    SPID_HELLCAT_HW_TYPE            = 0x1E,
    SPID_HELLCAT_LITE_HW_TYPE       = 0x1F,
    SPID_MUSTANG_HW_TYPE            = 0x20,
    SPID_MUSTANG_XM_HW_TYPE         = 0x21,
    SPID_HELLCAT_XM_HW_TYPE         = 0x22,
    SPID_PROMETHEUS_M1_HW_TYPE      = 0x23,
    SPID_PROMETHEUS_S1_HW_TYPE      = 0x24,     //Simulator
    SPID_DEFIANT_M1_HW_TYPE         = 0x25,
    SPID_DEFIANT_M2_HW_TYPE         = 0x26,
    SPID_DEFIANT_M3_HW_TYPE         = 0x27,
    SPID_DEFIANT_M4_HW_TYPE         = 0x28,
    SPID_DEFIANT_S1_HW_TYPE         = 0x29,     //Simulator
    SPID_DEFIANT_S4_HW_TYPE         = 0x2A,     //Simulator
    SPID_NOVA1_HW_TYPE              = 0x2B,
    SPID_TRIN_HW_TYPE               = 0x2C,
    SPID_EVERGREEN_HW_TYPE          = 0x2D,
    SPID_SENTRY_HW_TYPE             = 0x2E,
    SPID_NOVA_S1_HW_TYPE            = 0x2F,     //Simulator
    SPID_TRIN_S1_HW_TYPE            = 0x30,     //Simulator
    SPID_DEFIANT_M5_HW_TYPE         = 0x31,
    SPID_NOVA3_HW_TYPE              = 0x32,
    SPID_DEFIANT_K2_HW_TYPE         = 0x33,
    SPID_DEFIANT_K3_HW_TYPE         = 0x34,
    SPID_NOVA3_XM_HW_TYPE           = 0x35,
    SPID_ENTERPRISE_HW_TYPE         = 0x36,
    SPID_OBERON_1_HW_TYPE           = 0x37,
    SPID_OBERON_2_HW_TYPE           = 0x38,
    SPID_OBERON_3_HW_TYPE           = 0x39,
    SPID_OBERON_S1_HW_TYPE          = 0x3A,
    SPID_HYPERION_1_HW_TYPE         = 0x3B,
    SPID_HYPERION_2_HW_TYPE         = 0x3C,
    SPID_HYPERION_3_HW_TYPE         = 0x3D,
    SPID_TRITON_1_HW_TYPE           = 0x3E,
    SPID_MERIDIAN_ESX_HW_TYPE       = 0x3F,     // Meridian(vVNX). ESX Hypervisor
    SPID_TUNGSTEN_HW_TYPE           = 0x40,     // Tungsten(Virtual platform)
    SPID_OBERON_4_HW_TYPE           = 0x41,
    SPID_BEARCAT_HW_TYPE            = 0x42,
    SPID_MAXIMUM_HW_TYPE            = 0x43      //Must be the last value.  Used to specify the total number of values.
} SPID_HW_TYPE;

/***************************
 * struct: HW_CPU_MODULE
 *
 * Defines all possible
 * CPU Modules
 ***************************/
typedef enum _HW_CPU_MODULE
{
    INVALID_CPU_MODULE,
    BLACKWIDOW_CPU_MODULE,
    WILDCATS_CPU_MODULE,
    MAGNUM_CPU_MODULE,
    BOOMSLANG_CPU_MODULE,
    INTREPID_CPU_MODULE,
    ARGONAUT_CPU_MODULE,
    SENTRY_CPU_MODULE,
    EVERGREEN_CPU_MODULE,
    DEVASTATOR_CPU_MODULE,
    MEGATRON_CPU_MODULE,
    STARSCREAM_CPU_MODULE,
    JETFIRE_CPU_MODULE,
    BEACHCOMBER_CPU_MODULE,
    SILVERBOLT_CPU_MODULE,
    TRITON_ERB_CPU_MODULE,
    TRITON_CPU_MODULE,
    HYPERION_CPU_MODULE,
    OBERON_CPU_MODULE,
    CHARON_CPU_MODULE,
    // Meridian is a codename for vVNX product line
    MERIDIAN_CPU_MODULE,
    TUNGSTEN_CPU_MODULE,
    UNKNOWN_CPU_MODULE,
    MAX_CPU_MODULE,
} HW_CPU_MODULE;

/***************************
 * struct: HW_ENCLOSURE_TYPE
 *
 * Defines the enclosure
 * Type
 ***************************/
typedef enum _HW_ENCLOSURE_TYPE
{
    INVALID_ENCLOSURE_TYPE,
    XPE_ENCLOSURE_TYPE,
    DPE_ENCLOSURE_TYPE,
    // Virtual Processor Enclosure for Meridian; Not implemented yet
    VPE_ENCLOSURE_TYPE,
    UNKNOWN_ENCLOSURE_TYPE,
    MAX_ENCLOSURE_TYPE,
} HW_ENCLOSURE_TYPE;

/***************************
 * struct: HW_MIDPLANE_TYPE
 *
 * Defines the midplane
 * Types
 ***************************/
typedef enum _HW_MIDPLANE_TYPE
{
    INVALID_MIDPLANE_TYPE,
    FOGBOW_MIDPLANE,
    BROADSIDE_MIDPLANE,
    STILETTO_MIDPLANE,
    D15_MIDPLANE,
    BULLET_MIDPLANE,
    HOLSTER_MIDPLANE,
    TAIPAN_MIDPLANE,
    BUNKER_MIDPLANE,
    CITADEL_MIDPLANE,
    BOXWOOD_MIDPLANE,
    KNOT_MIDPLANE,
    RATCHET_MIDPLANE,
    SIDESWIPE_MIDPLANE,
    STRATOSPHERE_MIDPLANE,
    STEELJAW_MIDPLANE,
    RAMHORN_MIDPLANE,
    PROTEUS_MIDPLANE,
    TELESTO_MIDPLANE,
    SINOPE_MIDPLANE,
    MIRANDA_MIDPLANE,
    RHEA_MIDPLANE,
    // Meridian is a codename for vVNX product line
    MERIDIAN_MIDPLANE,
    UNKNOWN_MIDPLANE_TYPE,
    MAX_MIDPLANE_TYPE,
} HW_MIDPLANE_TYPE;

/***************************
 * struct: HW_FAMILY_TYPE
 *
 * Defines the H/W family
 * Types
 ***************************/
typedef enum _HW_FAMILY_TYPE
{
    INVALID_FAMILY_TYPE,
    FLEET_FAMILY,
    ARMADA_FAMILY,
    TRANSFORMERS_FAMILY,
    MOONS_FAMILY,
    // This is global family for MERIDIANs and TUNGSTENs
    VIRTUAL_VNX_FAMILY,
    UNKNOWN_FAMILY_TYPE,
    MAX_FAMILY_TYPE,
} HW_FAMILY_TYPE;

//.End

#endif // __SPID_ENUM_TYPES__
