#ifndef CM_ENVIRON_EXPORTS_H
#define CM_ENVIRON_EXPORTS_H 0x00000001 /* from common dir */
#define FLARE__STAR__H

/***************************************************************************
 * Copyright (C) Data General Corporation 1989-2009
 * All rights reserved.
 * Licensed material -- property of Data General Corporation
 ***************************************************************************/

/***************************************************************************
 * $Id: cm_environ_exports.h,v 1.2 1999/03/17 13:46:16 fladm Exp $
 ***************************************************************************
 *
 * DESCRIPTION:
 *   This header file contains the definitions for the interface to the
 *   environmental sub-component of the config manager
 *
 * NOTES:
 *
 * HISTORY:
 *      06/21/01 CJH    Created
 ***************************************************************************/

/*
 * LOCAL TEXT unique_file_id[] = "$Header: /fdescm/project/CVS/flare/src/include/cm_environ_if.h,v 1.2 1999/03/17 13:46:16 fladm Exp $"
 */

/*************************
 * INCLUDE FILES
 *************************/
#include "speeds.h"
#include "generic_types.h"

/*******************************
 * LITERAL DEFINITIONS
 *******************************/

// Number of ports per LCC
#define NUMBER_OF_LCC_PORTS 17  //15 drive ports, primary and expansion ports

/* The way CM handles too-many-enclosure is to allow
 * one extra enclosure show up in LCC_POLL reply, and
 * then mark it as duplicate enclosure.
 *
 * Of course if an enclosure can reply with a number
 * which is greater than ENCLOSURES_PER_BUS, 
 * we can't relly use it to index enclosure descriptor.
 * But that's a different issue. 
 */
#define CM_EXTRA_ENCL_PROCESSED  1

// address for XPE enclosure
#define XPE_ENCL_ADDRESS 0xFFFFFFFE
#define CM_XPE_FLT_LED_NO_FLT               0
#define CM_XPE_FLT_LED_FAN_FLT              0x1
#define CM_XPE_FLT_LED_PS_FLT               0x2
#define CM_XPE_FLT_LED_PEER_SP_FLT          0x4
#define CM_XPE_FLT_LED_SPS_FLT              0x8
#define CM_XPE_FLT_LED_CMI_FLT              0x10
#define CM_XPE_FLT_LED_RES_PROM_FLT         0x20
#define CM_XPE_FLT_LED_ENCL_STATE_FLT       0x40
#define CM_XPE_FLT_LED_MGMT_FRU_FLT         0x80
#define CM_XPE_FLT_LED_DRIVE_FLT            0x100
#define CM_XPE_FLT_LED_IO_MOD_FLT           0x200
#define CM_XPE_FLT_LED_BEM_FLT              0x400
#define CM_XPE_FLT_LED_FCLI_FLT             0x800
#define CM_XPE_FLT_LED_PS_SMB_FLT           0x1000
#define CM_XPE_FLT_LED_EXP_FLT              0x2000 
#define CM_XPE_FLT_LED_MGMT_SMB_FLT         0x4000
#define CM_XPE_FLT_LED_IO_MOD_SMB_FLT       0x8000
#define CM_XPE_FLT_LED_IO_ADAPTER_SMB_FLT   0x10000
#define CM_XPE_FLT_LED_SFP_FLT              0x20000
#define CM_XPE_FLT_LED_FAN_SMB_FLT          0x40000

/*
 * Definitions for bitmask for the reason the DAE's Fault LED is on
 */
#define CM_DAE_FLT_LED_NO_FLT               0
#define CM_DAE_FLT_LED_FAN_FLT              0x1
#define CM_DAE_FLT_LED_PS_FLT               0x2
#define CM_DAE_FLT_LED_PEER_SP_FLT          0x4
#define CM_DAE_FLT_LED_SPS_FLT              0x8
#define CM_DAE_FLT_LED_CMI_FLT              0x10
#define CM_DAE_FLT_LED_RES_PROM_FLT         0x20
#define CM_DAE_FLT_LED_ENCL_STATE_FLT       0x40
#define CM_DAE_FLT_LED_MC_CABLE_FLT         0x80
#define CM_DAE_FLT_LED_DRIVE_FLT            0x100
#define CM_DAE_FLT_LED_LCC_COM_FLT          0x200
#define CM_DAE_FLT_LED_PEER_LCC_FLT         0x400
#define CM_DAE_FLT_LED_FCLI_FLT             0x800
#define CM_DAE_FLT_LED_SPEED_MISMATCH_FLT   0x1000

#define CM_DAE_FLT_LED_EXP_FLT              0x2000 
#define CM_DAE_FLT_LED_FRUMON_UPGRADE_FLT   0x4000
#define CM_DAE_FLT_LED_OVER_TEMP_FLT        0x8000
#define CM_DAE_FLT_LED_PEER_OVER_TEMP_FLT   0x10000
#define CM_DAE_FLT_LED_ENCL_FAIL_FLT        0x20000
#define CM_DAE_FLT_LED_LCC_FLT              0x40000
#define CM_DAE_FLT_LED_EXP_DUP_DRIVE_SLOTS  0x80000

/*
 * These 4 values are family maximums for the Fleet (Proteus release)
 */

/*
 * The maximum number of ports per IO module is 4. iSCSI 
 * will have 2 ports per IO Module.
 */
#define MAX_PORTS_PER_IOM   4 // This is an outdated definition and will be removed soon.
#define MAX_ISCSI_PORTS_PER_IOM  2
#define MAX_FC_PORTS_PER_IOM  4
#define MAX_SAS_PORTS_PER_IOM  4
#define MAX_FCOE_PORTS_PER_IOM 2
#define MAX_IO_CONTROLLERS_PER_IOM 2
#define MAX_IO_PORTS_PER_CONTROLLER 4
#define MAX_IO_PORTS_PER_IOM (MAX_IO_PORTS_PER_CONTROLLER * MAX_IO_CONTROLLERS_PER_IOM)

/*
 * The maximum number of IO Modules a platform can have. 
 * Megatron will have 11
 */
#define MAX_NUM_OF_IOM   11

/* 
 * The maximum number of ports a SP can have. These values can change
 * for different platforms but this the maximum value that will be used 
 * for structure allocation.
 */
#define MAX_IO_PORTS_PER_SP    (MAX_NUM_OF_IOM * MAX_PORTS_PER_IOM)
// There is no user simulation for XPE_0.

#define INVALID_PORT_NUMBER 0xFFFFFFFE
#define INVALID_IO_SLOT 0xFFFFFFFE
#define INVALID_PCI_BUS 0xFFFFFFFE
#define INVALID_PCI_FUNC 0xFFFFFFFE
#define INVALID_PCI_SLOT 0xFFFFFFFE

// config io port command option when resetting all ports
#define RESET_ALL_NUM_PORTS_VALUE   0xFFFE


//  The next 3 items define the numbering scheme for the COM / serial devices
//  that the diplex threads see as the path to the diplex circuit and thus to
//  the LCCs.  These defines are also used by SPM to create the devices that
//  the diplex threads will open.
#define CM_DIPLEX_STARTING_SERIAL_DEVICE    0x64  //  Decimal 100
#define CM_DIPLEX_SERIAL_DEVICE_INCREMENT   0x0A  //  Decimal 10  
#define CM_DIPLEX_DEVICES_PER_SLOT          0x04  //  I.e. the ports per IO module.


/*************************************************************************
 | Enumeration: CM_ENCL_STATE
 |
 | Description:
 |  This enumerates the values for enclosures fault symptoms
 |
 ***********************************************************************/

typedef enum _CM_ENCL_STATE
{
    /********************************************************
     * NOTE: IF A NEW VALUE IS ADDED, NAVI NEEDS TO BE
     * INFORMED. FOR EACH FAULT SYMPTOM, NAVI DISPLAYS A STRING
     * TO THE USER. SINCE THESE VALUES ARE USED BY NAVI, THEY
     * SHOULD BE CHANGED ONLY AFTER INFORMING NAVI. PLEASE DO 
     * NOT DELETE ANY DEFINES EVEN IF OBSELETE AND ADD NEW ONES 
     * TOWARDS THE END
     *********************************************************/
    CM_ENCL_MISSING         = 0x00,     // Must be 0. Some code inits to this value via cm_clear_mem()
    CM_ENCL_OK              = 0x01,
    CM_ENCL_DEGRADING       = 0x02,
    CM_ENCL_BYPASSED        = 0x03,
    CM_ENCL_FAILED          = 0x04,
    CM_ENCL_UNKNOWN_STATE   = 0x05,
    CM_ENCL_TRANSITIONING   = 0x06,
    CM_ENCL_UNRESOLVED      = 0x07,
    CM_ENCL_REMOVING_FAILED = 0x08,
    CM_ENCL_REMOVING_GONE   = 0x09
}
CM_ENCL_STATE;

/*************************************************************************
 | Enumeration: CM_ENCL_FAULT_SYMPTOM
 |
 | Description:
 |  This enumerates the values for enclosures fault symptoms
 |
 |
 ***********************************************************************/
typedef enum _CM_ENCL_FAULT_SYMPTOM
{
    /********************************************************
     * NOTE: IF A NEW VALUE IS ADDED, NAVI NEEDS TO BE
     * INFORMED. FOR EACH FAULT SYMPTOM, NAVI DISPLAYS A STRING
     * TO THE USER. SINCE THESE VALUES ARE USED BY NAVI, THEY
     * SHOULD BE CHANGED ONLY AFTER INFORMING NAVI. PLEASE DO 
     * NOT DELETE ANY DEFINES EVEN IF OBSELETE AND ADD NEW ONES 
     * TOWARDS THE END
     *********************************************************/
    CM_ENCL_SYM_NO_FAULT                = 0x00, // 0x00000000 // Must be 0. Some code inits to this value via cm_clear_mem()
    CM_ENCL_SYM_COOLING_FAULT,                  // 0x00000001
    CM_ENCL_SYM_POWER_FAULT,                    // 0x00000002
    CM_ENCL_SYM_BAD_ADDR_FAULT,                 // 0x00000003
    CM_ENCL_SYM_CABLE_X_FAULT,                  // 0x00000004
    CM_ENCL_SYM_NO_RESP_FAULT,                  // 0x00000005
    CM_ENCL_SYM_BAD_RESP_FAULT,                 // 0x00000006
    CM_ENCL_SYM_UPSTREAM_FAULT,                 // 0x00000007
    CM_ENCL_SYM_WRONG_INPUT_PORT_FAULT,         // 0x00000008
    CM_ENCL_SYM_BE_FLT_RECOVERY_FAULT_PREFLEET, // 0x00000009 
    CM_ENCL_SYM_EXCEEDED_MAX,                   // 0x0000000A
    CM_ENCL_SYM_ENCL_33V_FAULT,                 // 0x0000000B
    CM_ENCL_SYM_ENCL_25V_FAULT,                 // 0x0000000C
    CM_ENCL_SYM_DED_CMD_FAILED,                 // 0x0000000D
    CM_ENCL_SYM_USER_INDUCED_FAULT,             // 0x0000000E
    CM_ENCL_SYM_BE_LOOP_MISCABLED,              // 0x0000000F
    CM_ENCL_SYM_UNSUPPORTED_ENCL,               // 0x00000010
    CM_ENCL_SYM_INVALID_LINK_SPEED,             // 0x00000011
    CM_ENCL_SYM_GENERAL_FAULT,                  // 0x00000012
    CM_ENCL_SYM_LOOP_SPEED_MISMATCH,            // 0x00000013
    CM_ENCL_SYM_LCC_TYPE_MISMATCH,              // 0x00000014
    CM_ENCL_SYM_SPEED_CHANGE_RETRIES_EXCEEDED,  // 0x00000015
    CM_ENCL_SYM_DRIVE_TYPE_MISMATCH,            // 0x00000016
    CM_ENCL_SYM_ENCL_TYPE_RG_TYPE_MISMATCH,     // 0x00000017
    CM_ENCL_SYM_CHANGING_SPEED,                 // 0x00000018
    CM_ENCL_SYM_SFP_INVALID_INFO,               // 0x00000019
    CM_ENCL_SYM_SFP_CHECKSUM_ERROR,             // 0x0000001A
    CM_ENCL_SYM_SFP_I2C_ERROR,                  // 0x0000001B
    CM_ENCL_SYM_SFP_UNQUAL_ERROR,               // 0x0000001C
    CM_ENCL_SYM_UNSTABLE_ENCL,                  // 0x0000001D
    CM_ENCL_SYM_POST_FAULT,                     // 0x0000001E

    /* Bigfoot Symptoms start */
    CM_ENCL_SYM_ARRAY_MISMATCH,                 // 0x0000001F
    CM_ENCL_SYM_ADDRESS_MISMATCH,               // 0x00000020
    CM_ENCL_SYM_NO_NUMBER_AVAILABLE,            // 0x00000021
    CM_ENCL_SYM_DUPLICATE_NUMBER,               // 0x00000022
    CM_ENCL_SYM_FIRMWARE_REGISTRY_FAULT,        // 0x00000023
    CM_ENCL_SYM_FIRMWARE_REVISION_FAULT,        // 0x00000024
    CM_ENCL_SYM_FIRMWARE_DOWNLOAD_FAULT,        // 0x00000025
    CM_ENCL_SYM_FIRMWARE_UPDATE_FAULT,          // 0x00000026
    /* Bigfoot Symptoms end */

    CM_ENCL_SYM_BE_FLT_RECOVERY_FAULT,          // 0x00000027

    // More from ess_main
    CM_ENCL_SYM_BAD_LCC_RESP_FAULT,             // 0x00000028
    CM_ENCL_SYM_LCC_INVALID_UID,                // 0x00000029
    CM_ENCL_SYM_EXP_SGPIO_READ_FAULT,           // 0x0000002A
    // For Multibus support
    CM_ENCL_SYM_POSITION_MISMATCH,              // 0x0000002B
    CM_ENCL_SYM_SFP_FAULTED,                    // 0x0000002C
    CM_ENCL_HAS_DUP_DRIVE_SLOTS,                // 0x0000002D
    CM_ENCL_SYM_PS_TYPE_MIX                     // 0x0000002E

} CM_ENCL_FAULT_SYMPTOM;

typedef enum enclosure_sp_id_enum
{
    EN_SP_ID_A           = 0,
    EN_SP_ID_B           = 1,
    EN_SP_ID_INDETERMIN  = 254,
    EN_SP_ID_NA          = 255
} ENCLOSURE_SP_ID;


typedef enum enclosure_fru_location_enum
{
    FRU_LOC_IO_SLOT             = 0,   // Fru is plugged in the SP IO slot.
    FRU_LOC_IO_ANNEX            = 1,   // Fru is plugged directly into io annex.
    FRU_LOC_IO_ANNEX_FRU        = 2,   // Fru is plugged into fru in io annex. E.G. an IO module plugged into a Tornado.
    FRU_LOC_IO_MEZZANINE_SLOT   = 3,   // Fru is plugged in the SP mezzanine slot.
    FRU_LOC_BEM                 = 4,   // Fru is plugged directly into Backend Module.
    FRU_LOC_UNKNOWN             = 253,  
    FRU_LOC_INDETERMIN          = 254,
    FRU_LOC_NA                  = 255
} ENCLOSURE_FRU_LOCATION;

typedef enum enclosure_fru_sub_location_enum
{
    FRU_SUB_LOC_POS_0          = 0,
    FRU_SUB_LOC_POS_1          = 1,
    FRU_SUB_LOC_POS_2          = 2,
    FRU_SUB_LOC_POS_3          = 3,
    FRU_SUB_LOC_POS_4          = 4,
    FRU_SUB_LOC_UNKNOWN        = 253,
    FRU_SUB_LOC_INDETERMIN     = 254,
    FRU_SUB_LOC_NA             = 255
} ENCLOSURE_FRU_SUB_LOCATION;

typedef enum mgmt_port_auto_neg_tag
{
    PORT_AUTO_NEG_OFF            = 0,
    PORT_AUTO_NEG_ON             = 1,
    PORT_AUTO_NEG_UNSPECIFIED    = 252,  //  For Write;
    PORT_AUTO_NEG_INVALID        = 253,
    PORT_AUTO_NEG_INDETERMIN     = 254,  //  For Read.
    PORT_AUTO_NEG_NA             = 255   //  For Read.
} MGMT_PORT_AUTO_NEG;

typedef enum mgmt_port_speed_tag
{
    PORT_SPEED_1000MBPS                 = SPEED_ONE_GIGABIT,  // The value matches the standard speed value SPEED_ONE_GIGABIT
    PORT_SPEED_10MBPS                   = SPEED_TEN_MEGABIT,  // The value matches the standard speed value SPEED_TEN_MEGABIT 
    PORT_SPEED_100MBPS                  = SPEED_100_MEGABIT,  // The value matches the standard speed value SPEED_100_MEGABIT
    PORT_SPEED_UNSPECIFIED              = 0xFFFc,  //  For Write;
    PORT_SPEED_INVALID                  = 0xFFFd,
    PORT_SPEED_INDETERMIN               = 0xFFFe,  //  For Read.
    PORT_SPEED_NA                       = 0xFFFF   //  For Read.
} MGMT_PORT_SPEED;

typedef enum mgmt_port_duplex_mode_tag
{
    PORT_DUPLEX_MODE_HALF               = 0,
    PORT_DUPLEX_MODE_FULL               = 1,
    PORT_DUPLEX_MODE_UNSPECIFIED        = 252,  //  For Write;  
    PORT_DUPLEX_MODE_INVALID            = 253,
    PORT_DUPLEX_MODE_INDETERMIN         = 254,  //  For Read.
    PORT_DUPLEX_MODE_NA                 = 255
} MGMT_PORT_DUPLEX_MODE;


/*
 * The PORT_ROLE enum defines all the possible Roles the Port can take.
 */
typedef  enum
{
    PORT_ROLE_UNINITIALIZED = 0,
    PORT_ROLE_FE, /* This port is assigned to be Front end */
    PORT_ROLE_BE /* This port is assigned to be Back end */
} PORT_ROLE;

/*
 * The PORT_SUBROLE enum defines the sub-role the port can take.
 */
typedef  enum
{
    PORT_SUBROLE_UNINTIALIZED = 0,
    PORT_SUBROLE_SPECIAL, // FE Ports for MVS, MVA, and SC 
    PORT_SUBROLE_NORMAL 
} PORT_SUBROLE;

/*
 * The PORT_STATE enum defines the possible states for the port.
 */
typedef enum
{
    PORT_STATE_UNINITIALIZED = 0,

    /*
     * This is to handle the case  when the IO Module is
     * powered down and don’t know what is plugged in.
     */
    PORT_STATE_UNKNOWN,

    /* Nothing is plugged in and is un-initialized */
    PORT_STATE_EMPTY,

    /* Nothing is plugged in but the port is initialized */ 
    PORT_STATE_MISSING,

    /*
     * Problem with the port, the sub-state will provide
     * the reason for fault
     */
    PORT_STATE_FAULTED,

    /* Everything is good with the port */
    PORT_STATE_ENABLED,
    PORT_STATE_UNAVAILABLE,
    PORT_STATE_DISABLED

} PORT_STATE;


/*
* The IOM_STATE enum defines the possible states for the IO Module.
*/
typedef  enum
{
    IOM_STATE_UNINITIALIZED = 0,

    /* Nothing is plugged in and is un-initialized. */
    IOM_STATE_EMPTY,

    /* Nothing is plugged in but the io module is initialized. */   
    IOM_STATE_MISSING,

    /*
     * Problem with the IO Module, the sub-state will provide
     * the reason for fault.
     */
    IOM_STATE_FAULTED,

    /* Everything is good with the IO Module */
    IOM_STATE_ENABLED,

    /* Unsupported IO Module */
    IOM_STATE_UNSUPPORTED_IOM

} IOM_STATE;


/*
 * The PORT_SUBSTATE enum gives extra information about the port 
 * state, specifically, when the state is IOM_PORT_STATE_FAULTED. 
 */
typedef enum
{
    /* This particular port is present and no faults were found
     * but is uninitialized.  This substate is applicable for IO
     * modules that do not have SFPs e.g. ISCSI
     */
    PORT_SUBSTATE_UNINITIALIZED,
    /* This port is uninitialized, but SFP is not inserted.  This
     * substate will be applicable only for FC IO modules.
     */
    PORT_SUBSTATE_SFP_NOT_PRESENT,
    /* This port is not initialized and IO module is not present */
    PORT_SUBSTATE_IOM_NOT_PRESENT,
    /* The port is not present in this module.  The max port per IO
     * module is defined as 4 but iSCSI IO module has only two ports,
     * so the two ports that do not exist will be set to this substate.
     */
    PORT_SUBSTATE_PORT_NOT_PRESENT,
    /* THis port was initialized but SFP is not plugged in */
    PORT_SUBSTATE_MISSING_SFP,
    /* This port was initialized but IO Module is not present */
    PORT_SUBSTATE_MISSING_IOM,
    /*
     * This port was initialized but an incorrect SFP is inserted that does 
     * not match the persisted info
     */
    PORT_SUBSTATE_INCORRECT_SFP_TYPE,
    /*
     * The port was initialized but incorrect IO Module is inserted and 
     * thereby the port will also be incorrect 
     */ 
    PORT_SUBSTATE_INCORRECT_IOM,
    /* The SFP information could not be read */
    PORT_SUBSTATE_SFP_READ_ERROR,
     /* This SFP is unsupported for this release */
    PORT_SUBSTATE_UNSUPPORTED_SFP,
    /* 
     * IO Module had resume PROM read error and cannot determine the type of 
     * port. For uninitialized port the state will be PORT_STATE_UNKNOWN but 
     * for initialized port, since it is stored in the Database, it will be 
     * set to PORT_STATE_ENABLED.
     */
    PORT_SUBSTATE_IOM_READ_ERROR,
    /*
     * The number of ports exceeded the allowed limits. If iSCSI IOM, 
     * the state will be UNINITIALIED but for FC Modules the port will
     * be faulted.
     */
    PORT_SUBSTATE_EXCEEDED_MAX_LIMITS,
    /*
     * The IO Module is powered off. If the port is not initialized
     * then the state will be known but if the port is initialized 
     * then the port will be faulted.
     */
    PORT_SUBSTATE_IOM_POWERED_OFF,
    /* This IO Module is not supported for this release */
    PORT_SUBSTATE_UNSUPPORTED_IOM,
    /* 
     * This particular port is present and no faults were 
     * found and in initialized.
     */
    PORT_SUBSTATE_GOOD,
    PORT_SUBSTATE_DB_READ_ERROR,
    PORT_SUBSTATE_FAULTED_SFP,
    PORT_SUBSTATE_HW_FAULT,
    PORT_SUBSTATE_UNAVAILABLE,
    PORT_SUBSTATE_DISABLED_USER_INIT,
    PORT_SUBSTATE_DISABLED_ENCRYPTION_REQUIRED,
    PORT_SUBSTATE_DISABLED_HW_FAULT
} PORT_SUBSTATE;

/*
 * The IOM_SUBSTATE enum gives extra information about the IO Module sub-state,
 * specifically, when the state is IOM_PORT_STATE_FAULTED. 
 */
typedef  enum
{
    /* Module NOT Present but none of the ports are initialized */
    IOM_SUBSTATE_NOT_PRESENT,    
    /* Module NOT present but atlease one port is initialized */
    IOM_SUBSTATE_MISSING, 
    /* Error reading the resume PROM on the IO Module */
    IOM_SUBSTATE_PROM_READ_ERROR,
    /* 
     * Atleast one of the port was initialized in this IO Module but a wrong 
     * IO Module was plugged in 
     */
    IOM_SUBSTATE_INCORRECT_IOM,
    /* This IO Module is not supported for this release */
    IOM_SUBSTATE_UNSUPPORTED_IOM,
    IOM_SUBSTATE_GOOD,
    IOM_SUBSTATE_POWERED_OFF,
    IOM_SUBSTATE_POWERUP_FAILED,
    IOM_SUBSTATE_POST_BIOS_FAILED,
    /* This IO Module is not supported until this release is committed */
    IOM_SUBSTATE_UNSUPPORTED_NOT_COMMITTED
} IOM_SUBSTATE;


/*
 * The PORT_PROTOCOL enum is needed so that the port protocol information
 * is persisted in the Database incase the user removes the IO Module whose
 * ports are configured as FC FE and replaces it with iSCSI module. We could
 * techinically use the IOM_TYPE but in the future if we have combo IO modules
 * where it can support multiple protocols, then the design should be scalable.
 * Hence this enum.
 */
typedef  enum
{
    PORT_PROTOCOL_UNKNOWN = 0,
    PORT_PROTOCOL_FC,
    PORT_PROTOCOL_ISCSI,
    PORT_PROTOCOL_SAS,
    PORT_PROTOCOL_FCOE
} PORT_PROTOCOL;
/*
 * The IOM_GROUP enum differentiates between different compatible Io module types.
 * Backward compatible with PORT_PROTOCOL.
 */
typedef enum
{   
    IOM_GROUP_UNKNOWN  = 0,
    IOM_GROUP_ADAMS    = 1,  // FC_4G – Tomahawk
    IOM_GROUP_BOURNE   = 2,  // iSCSI_1G - Harpoon
    IOM_GROUP_CARVER  = 3,  // SAS BE only - Cormandel, Hypernova, Moons (x4)
    IOM_GROUP_DOVER   = 4,  // FC_8G - Glacier
    IOM_GROUP_EASTON  = 5,  // ISCSI_10G - Poseidon
    IOM_GROUP_FRANKLIN = 6, // FCoE - Heatwave
    IOM_GROUP_GRAFTON  = 7, // iSCSI_1G 4 Port - Super Cell
    IOM_GROUP_HOPKINTON = 8, // SAS FE only - Coldfront
    IOM_GROUP_IPSWITCH = 9,  // iSCSI Copper - Eruption
    IOM_GROUP_J = 10,        // 10G iSCSI Optical - El Nino
    IOM_GROUP_K = 11,        // 6G SAS BE - Moonlite
    IOM_GROUP_END           // This *must* always be the last entry
} IOM_GROUP;


/*
 * The IOM_POWER enum defines the power status of the IO Module.
 */
typedef enum 
{
    /* 
     * The power status is not available. This is the default setting 
     * if the IO Module is not inserted.
     */
    IOM_POWER_STATUS_NA,
    IOM_POWER_STATUS_POWER_ON,
    IOM_POWER_STATUS_POWER_OFF,
    /* The IO Module was tried to be powered up but it failed */
    IOM_POWER_STATUS_POWERUP_FAILED
} IOM_POWER_STATUS;

// SLIC_TYPE Bitmask
// This new bitmask will represent the user-visible SLIC type and can map to a Common SPO Labl Name.
// This is derived from the unique_id in the iom_config_info struct.
//This enum is sync with enum defined in AdminTypes.h
#ifndef SLIC_TYPE_BIT_MASK
#define SLIC_TYPE_BIT_MASK 
typedef enum
{
    SLIC_TYPE_UNKNOWN        = 0x00000000,
    SLIC_TYPE_FC_4G          = 0x00000001, // Tomahawk
    SLIC_TYPE_ISCSI_1G       = 0x00000002, // Harpoon
    SLIC_TYPE_FC_8G          = 0x00000004, // Glacier
    SLIC_TYPE_ISCSI_10G      = 0x00000008, // Poseidon
    SLIC_TYPE_SAS            = 0x00000010, // Cormandel
    SLIC_TYPE_FCOE           = 0x00000020, // Heatwave
    SLIC_TYPE_4PORT_ISCSI_1G = 0x00000040, // Supercell
    SLIC_TYPE_6G_SAS_1       = 0x00000080, // Hypernova, Moons(x4)
    SLIC_TYPE_6G_SAS_2       = 0x00000100, // Coldfront
    SLIC_TYPE_ISCSI_COPPER   = 0x00000200, // Eruption
    SLIC_TYPE_ISCSI_10G_V2   = 0x00000400, // El Nino
    SLIC_TYPE_NA             = 0x40000000, // Mezzanine or other non-SLICs
    SLIC_TYPE_UNSUPPORTED    = 0x80000000
} SLIC_TYPE;
#endif

// This is the protocol used by the SLIC.  In the case where
// there is more than one protocol, MUTLI is used.
typedef enum
{
    IO_MODULE_PROTOCOL_UNKNOWN  = 0x0,
    IO_MODULE_PROTOCOL_FIBRE    = 0x1,
    IO_MODULE_PROTOCOL_ISCSI    = 0x2,
    IO_MODULE_PROTOCOL_SAS      = 0x3, 
    IO_MODULE_PROTOCOL_FCOE     = 0x4,
    IO_MODULE_PROTOCOL_MULTI    = 0x5
} IO_MODULE_PROTOCOL;


typedef enum
{
    IO_MODULE_CLASS_UNKNOWN,
    IO_MODULE_CLASS_SLIC,
    IO_MODULE_CLASS_MEZZANINE,
    IO_MODULE_CLASS_BACK_END_MODULE,
    IO_MODULE_CLASS_MAX
}  IO_MODULE_CLASS;

/*************************************************************************
 | Enumeration: CM_ENCL_TYPE
 |
 | Description:
 |  This enumerates the values for various enclosure types
 |
 | Note:
 |  Whenever a new type of SAS Enclosure is added to this
 |  enumeration, it should also be added to the macro
 |  CM_IS_SAS_ENCL.
 |
 ***********************************************************************/
typedef enum _CM_ENCL_TYPE
{
    CM_ENCL_TYPE_UNKNOWN        = 0x0000,  // Must be 0. Some code inits to this value via cm_clear_mem()
    CM_ENCL_TYPE_LONGBOW,
    CM_ENCL_TYPE_KATANA,
    CM_ENCL_TYPE_KLONDIKE,
    CM_ENCL_TYPE_PIRANHA,
    CM_ENCL_TYPE_STILETTO_2G,
    CM_ENCL_TYPE_STILETTO_4G,
    CM_ENCL_TYPE_BIGFOOT        = 0x0007, /* NOTE!! this MUST be 7 since it is used in the LCC MC DWLD request !!! */
    CM_ENCL_TYPE_BOOMSLANG,
    CM_ENCL_TYPE_VIPER, 
    CM_ENCL_TYPE_HOLSTER, 
    CM_ENCL_TYPE_BUNKER, 
    CM_ENCL_TYPE_CITADEL, 
    CM_ENCL_TYPE_DERRINGER,
    CM_ENCL_TYPE_VOYAGER,
    CM_ENCL_TYPE_FALLBACK,
    CM_ENCL_TYPE_BOXWOOD,
    CM_ENCL_TYPE_KNOT,
    CM_ENCL_TYPE_TABASCO,
    CM_ENCL_TYPE_INVALID = 0xFFFF
}
CM_ENCL_TYPE;


/*********************************
 * STRUCTURE DEFINITIONS
 *********************************/
 typedef struct cm_encl_peer_info_tag
{
    UINT_32 encl_addr;
    UINT_32 number_of_slots;
    CM_ENCL_STATE state;
    CM_ENCL_TYPE encl_type;
}CM_ENCL_PEER_INFO;


/*********************************
 * EXTERNAL REFERENCE DEFINITIONS
 *********************************/


/*
 * End $Id: cm_environ_exports.h,v 1.2 1999/03/17 13:46:16 fladm Exp $
 */

#endif /* CM_ENVIRON_EXPORTS_H */
