#ifndef ADM_ENCLOSURE_API_H
#define ADM_ENCLOSURE_API_H 0x0000001
#define FLARE__STAR__H
/***************************************************************************
 * Copyright (C) EMC Corporation 2001-2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *  adm_enclosure_api.h
 ***************************************************************************
 *
 *  Description
 *
 *      This file contains the external interface to the enclosure-related
 *      interfaces of the Admin Interface "adm" module in the Flare driver.
 *
 *      This file is included by adm_api.h, the top-level interface for
 *      the ADM module.
 *
 *  History:
 *      06/15/01 CJH    Created
 *      11/01/01 HEW    Added ifdefs for C++ compilation
 *      09/18/02 HEW    Added dyn_enclosure_index()
 *      05/10/07 EDC    Merging enclosure type changes from main branch
 ***************************************************************************/

#include "cm_environ_exports.h"

/************
 * Literals
 ************/

typedef enum adm_encl_drive_type_tag
{
    ADM_ENCL_DRIVE_TYPE_UNKNOWN     = 0x0000,
    ADM_ENCL_DRIVE_TYPE_FC          = 0x0001,
    ADM_ENCL_DRIVE_TYPE_ATA         = 0x0002,
    ADM_ENCL_DRIVE_TYPE_SATA        = 0x0004,
    ADM_ENCL_DRIVE_TYPE_SATA2       = 0x0008,
    ADM_ENCL_DRIVE_TYPE_SAS         = 0x0010,
    ADM_ENCL_DRIVE_TYPE_INVALID     = 0x00FF
}
ADM_ENCL_DRIVE_TYPE;

typedef enum adm_drive_type_tag
{
    ADM_DRIVE_TYPE_UNKNOWN        = 0x00,
    ADM_DRIVE_TYPE_FC             = 0x01,
    ADM_DRIVE_TYPE_ATA      = 0x02,
    ADM_DRIVE_TYPE_SATA    = 0x03,
    ADM_DRIVE_TYPE_SATA2          = 0x04, 
    ADM_DRIVE_TYPE_SAS            = 0x05,
    ADM_DRIVE_TYPE_SATA_NORTHSTAR = 0x06,
    ADM_DRIVE_TYPE_FSSD           = 0x07
}
ADM_DRIVE_TYPE;


typedef enum adm_seb_fault_code_tag
{
    ADM_SEB_MC_FAULT                = 0x0001,
    ADM_SEB_CC_FAULT                = 0x0002,
    ADM_SEB_HBC_FAULT               = 0x0004,
    ADM_SEB_RP_ACCESS_FAULT         = 0x0008,
    ADM_SEB_DOWNSTREAM_PORT_FAULT   = 0x0010,
    ADM_SEB_UPSTREAM_PORT_FAULT     = 0x0020,
    ADM_SEB_CABLE_FAULT             = 0x0040,
    ADM_SEB_DRIVE_PHY_FAULT         = 0x0080,
    ADM_SEB_NOT_INSERTED_FAULT      = 0x0100,
    ADM_SEB_OVER_TEMP_FAULT         = 0x0200
}
ADM_SEB_FAULT_CODE;

typedef enum adm_cabling_fault_code_tag
{
    ADM_CABLING_NO_INPUT_CONNECTION     = 0x0001,
    ADM_CABLING_ISOLATED_LOOP           = 0x0002,
    ADM_CABLING_CONNECT_TO_PEER         = 0x0004,
    ADM_CABLING_INVERTED_CABLED         = 0x0008,
    ADM_CABLING_PEER_ON_FOREIGN_LOOP    = 0x0010,
    ADM_CABLING_ASYMETRICAL_CABLING     = 0x0020
}
ADM_CABLING_FAULT_CODE;

typedef enum adm_enclosure_chassis_type_tag
{
    ADM_ENCLOSURE_TOWER,
    ADM_ENCLOSURE_RACKMOUNT
}
ADM_ENCLOSURE_CHASSIS_TYPE;


//  The following structs are enums used in the dyn and adm functions to combine
//  both whether a value is meaningful in the current context, and if so, what
//  the current value is.  This simplifies the code for the Admin and Navi layers,
//  in that one function call replaces 2 or 3 calls.
//  EN_xx_NA means that this datum is not applicable in this  situation.
//  EN_xx_INDETERMIN means that the datum is applicable to this situation,
//  but the value could not be determined or obtained.  EN_ST_FALSE and EN_ST_TRUE
//  mean the datum is applicable and TRUE / FALSE value is valid.  Other values mean
//  that the data is applicable and valid and has the specific value.
typedef enum enclosure_status_enum
{
    EN_ST_FALSE       = 0,
    EN_ST_TRUE        = 1,
    EN_ST_INDETERMIN  = 254,
    EN_ST_NA          = 255
} ENCLOSURE_STATUS;

typedef enum enclosure_fru_slot_enum
{
    EN_FRU_SLOT_0           = 0,
    EN_FRU_SLOT_1           = 1,
    EN_FRU_SLOT_2           = 2,
    EN_FRU_SLOT_3           = 3,
    EN_FRU_SLOT_4           = 4,
    EN_FRU_SLOT_5           = 5,
    EN_FRU_SLOT_INDETERMIN  = 254,
    EN_FRU_SLOT_NA          = 255
} ENCLOSURE_FRU_SLOT;

/********************************************************************
 ** IMPORTANT NOTE: If you add any new enclosure types, you must add
 **                 it to all branches so no types have multiple
 **                 meanings.
 ********************************************************************/
typedef enum adm_enclosure_type_tag
{
    ADM_ENCL_TYPE_UNKNOWN = -1,       // Unknown type
    ADM_ENCL_TYPE_DPE = 0,          // Longbow SP/disk enclosure
    ADM_ENCL_TYPE_DAE,          // Longbow disk enclosure
    ADM_ENCL_TYPE_XPE,          // Cham II SP enclosure
    ADM_ENCL_TYPE_2GB_DAE,      // Cham II, X-1 disk enclosure
    ADM_ENCL_TYPE_2GB_iDAE,     // X-1 SP/disk enclosure
    ADM_ENCL_TYPE_SATA,         // Serial ATA (Klondike) enclosure
    ADM_ENCL_TYPE_SATA_DPE,     // S-ATA DPE (ATAtude) enclosure
    ADM_ENCL_TYPE_CTS_2G_DAE,   // Stiletto 2G enclosure
    ADM_ENCL_TYPE_CTS_4G_DAE,   // Stiletto 4G enclosure
    ADM_ENCL_TYPE_SATA2_DPE,    // SATA-2 DPE enclosure
    ADM_ENCL_TYPE_ZEPHYR,       // Hammerhead processor enclosure
    ADM_ENCL_TYPE_ZOMBIE,       // Sledge/Jackhammer processor enclosure
    ADM_ENCL_TYPE_SATA2_SAS_DPE,// Bigfoot DPE
    ADM_ENCL_TYPE_SATA2_SAS_DAE,// Bigfoot DAE
    ADM_ENCL_TYPE_FOGBOW,       // Dreadnought processor enclosure
    ADM_ENCL_TYPE_BROADSIDE,    // Trident/Ironclad processor enclosure  
    ADM_ENCL_TYPE_VIPER,        // 6GB 15 drive SAS DAE
    ADM_ENCL_TYPE_HOLSTER,      // Magnum DPE
    ADM_ENCL_TYPE_BUNKER,       // 15 drive sentry DPE
    ADM_ENCL_TYPE_CITADEL,      // 25 drive sentry DPE
    ADM_ENCL_TYPE_DERRINGER,    // 25 drive SAS DAE
    ADM_ENCL_TYPE_VOYAGER,      // 60 drive SAS DAE (Voyager)
    ADM_ENCL_TYPE_RATCHET,      // Megatron xPE
    ADM_ENCL_TYPE_FALLBACK,     // Jetfire 25 drive DPE
    ADM_ENCL_TYPE_BOXWOOD,      // Beachcomber 12 drive DPE
    ADM_ENCL_TYPE_KNOT,         // Beachcomber 25 drive DPE
    ADM_ENCL_TYPE_PINECONE,     // 12 drive pinecone DAE
    ADM_ENCL_TYPE_STEELJAW,     // 12 drive beachcomber steeljaw DPE
    ADM_ENCL_TYPE_RAMHORN,      // 25 drive beachcomber ramhorn DPE
    ADM_ENCL_TYPE_TABASCO,      // 12GB 25 drive SAS DAE
    ADM_ENCL_TYPE_ANCHO,        // 12GB 15 drive SAS DAE
    ADM_ENCL_TYPE_CAYENNE,      // 12GB 60 drive SAS DAE
    ADM_ENCL_TYPE_NAGA,         // 12GB 120 drive SAS DAE
    ADM_ENCL_TYPE_VIKING,       // 120 drive SAS DAE (Viking)
    ADM_ENCL_TYPE_TELESTO,      // Triton XPE
    ADM_ENCL_TYPE_CALYPSO,      // Hyperion 25 Drive DPE
    ADM_ENCL_TYPE_MIRANDA,      // Oberon/Charon 25 drive DPE
    ADM_ENCL_TYPE_RHEA,         // Oberon/Charon 12 drive DPE
    ADM_ENCL_TYPE_MERIDIAN = 100 // vVNX VPE
} ADM_ENCL_TYPE;

#define ADM_FLASH_STATES                2

/* SPS power & cabling status */
typedef enum adm_sps_state_enum
{
    ADM_SPS_OK,
    ADM_SPS_TESTING,
    ADM_SPS_NOT_AVAILABLE,
    ADM_SPS_FAULTED,
    ADM_SPS_NOT_READY,
    ADM_SPS_ON_BATTERY,
    ADM_SPS_BUSY,
    ADM_SPS_UNKNOWN,
    ADM_SPS_NOT_SUPPORTED,

    // The following "states" are used in the Admin-Navi TLD interface to
    // turn the SPS on & off (via the backdoor power IOCTL).  They are
    // included here to reserve the values to prevent their use in the
    // Flare-Admin interface.  The values are set to 98 & 99 so that new
    // states can be added without changing these values.
    ADM_SPS_TURN_ON = 98,
    ADM_SPS_TURN_OFF
}
ADM_SPS_STATE;

typedef enum adm_sps_cabling_enum
{
    ADM_SPS_VALID,
    ADM_SPS_INVALID_POWER_1,
    ADM_SPS_INVALID_POWER_2,
    ADM_SPS_INVALID_SERIAL,
    ADM_SPS_INVALID_MULTIPLE,
    ADM_SPS_CABLING_UNKNOWN,
    ADM_SPS_NA
}
ADM_SPS_CABLING;

/* Dummy bus & address values for XPE */
#define ADM_XPE_BUS         0xfffffffe
#define ADM_XPE_ADDRESS     0xfffffffe
#define ADM_XPE_POSITION    0xfffffffe


/************
 *  Types
 ************/

/*
 * The 2-element arrays control the enclosure LEDs.
 * If both bits are on, then the corresponding LED is turned on.
 * If both bits are off, then the corresponding LED is turned off.
 * If both bits are different, then the corresponding LED is flashed.
 */
typedef struct adm_flash_bits_struct
{
    BOOL    enclosure_flash[ADM_FLASH_STATES];
    BITS_32 flash_bits[ADM_FLASH_STATES];     // bit 0 is drive 0
    // bit 15 is drive 15
    BOOL    cable_port_flash[ADM_FLASH_STATES];
} ADM_FLASH_BITS;

typedef enum _ADM_LCC_TYPE
{
    ADM_LCC_TYPE_UNKNOWN = 0x00,
    ADM_LCC_TYPE_2G_FC,
    ADM_LCC_TYPE_2G_ATA,
    ADM_LCC_TYPE_2G_CTS,
    ADM_LCC_TYPE_4G_CTS,
    ADM_LCC_TYPE_3G_BOOM,
    ADM_LCC_TYPE_6G_VIPER,
    ADM_LCC_TYPE_6G_DERRINGER,
    ADM_LCC_TYPE_6G_VOYAGER_ICM,
    ADM_LCC_TYPE_6G_VOYAGER_EE,
    ADM_LCC_TYPE_6G_SENTRY_15,
    ADM_LCC_TYPE_6G_SENTRY_25,
    ADM_LCC_TYPE_6G_JETFIRE,
    ADM_LCC_TYPE_6G_BEACHCOMBER_12,
    ADM_LCC_TYPE_6G_BEACHCOMBER_25,
    ADM_LCC_TYPE_12G_TABASCO,
    ADM_LCC_TYPE_MAX
}
ADM_LCC_TYPE;


typedef enum xpe_fru_op_mode_enum
{
    OP_MODE_CLARIION           = 0,
    OP_MODE_SYMMETRIX          = 1,
    OP_MODE_USE_CURRENT        = 252,
    OP_MODE_INVALID            = 253,
    OP_MODE_INDETERMIN         = 254,
    OP_MODE_NA                 = 255
} XPE_FRU_OP_MODE;

typedef enum mgmt_port_vlan_config_mode_enum
{
    VLAN_CONFIG_MODE_CLARIION           = 0,
    VLAN_CONFIG_MODE_SYMM               = 1,
    VLAN_CONFIG_MODE_INVALID            = 253,
    VLAN_CONFIG_MODE_INDETERMIN         = 254,
    VLAN_CONFIG_MODE_NA                 = 255
} MGMT_PORT_VLAN_CONFIG_MODE;

typedef enum mgmt_fru_type_enum
{
    MGMT_TYPE_AVALANCHE           = 0,
    MGMT_TYPE_NOREASTER           = 1,
    MGMT_TYPE_AKULA               = 2,
    MGMT_TYPE_SOLARFLARE          = 3,
    MGMT_TYPE_INVALID             = 253,
    MGMT_TYPE_INDETERMIN          = 254,
    MGMT_TYPE_NA                  = 255
} MGMT_FRU_TYPE;

typedef enum bem_type_enum
{
    BEM_TYPE_IRONHIDE          = 0,
    BEM_TYPE_INVALID           = 253,
    BEM_TYPE_INDETERMIN        = 254,
    BEM_TYPE_NA                = 255
} BEM_TYPE;

typedef enum led_state_enum
{
    LED_STATE_STEADY_OFF           = 0,
    LED_STATE_STEADY_ON            = 1,
    LED_STATE_FLASH                = 2,
    LED_STATE_FLASH_2_COLOR        = 3,
    LED_STATE_FLASH_FOR_FAULT      = 4,
    LED_STATE_FALSE_FOR_SERVICE    = 5,
    LED_STATE_USE_CURRENT          = 252,  // For write.
    LED_STATE_INVALID              = 253,
    LED_STATE_INDETERMIN           = 254,
    LED_STATE_NA                   = 255
} LED_STATE;



#define ADM_DRIVE_SERIAL_NUMBER_MAX_SIZE    20

typedef enum
{
    ADM_FRU_PUP_BLOCKED_BY_NONE                 = 0x00000000,
    ADM_FRU_PUP_BLOCKED_BY_FOREIGN_DRIVE        = 0x00000001,
    ADM_FRU_PUP_BLOCKED_BY_CRITICAL_DRIVE       = 0x00000002,
    ADM_FRU_PUP_BLOCKED_BY_UNKNOWN              = 0xFFFFFFFF
}ADM_FRU_BLOCKED_TYPE;

typedef enum adm_mezzanine_type_tag
{
    MEZZANINE_TYPE_MUZZLE_WITH_SAS                 = 0,
    MEZZANINE_TYPE_MUZZLE_WITHOUT_SAS              = 1,
    MEZZANINE_TYPE_PEACEMAKER_WITH_SAS             = 2,
    MEZZANINE_TYPE_PEACEMAKER_WITHOUT_SAS          = 3,
    MEZZANINE_TYPE_INVALID             = 253,
    MEZZANINE_TYPE_INDETERMIN          = 254,
    MEZZANINE_TYPE_NA                  = 255
}
ADM_MEZZANINE_TYPE;

typedef struct adm_fru_cf_drive_struct_tag
{
    UINT_32                 fru_position_number;
    ADM_FRU_BLOCKED_TYPE    fru_blocked_type;
    UINT_32                 expected_disk_sn_buffer_size;
    UINT_8                  *expected_disk_sn_buffer;
    UINT_32                 inserted_disk_sn_buffer_size;
    UINT_8                  *inserted_disk_sn_buffer;
    TRI_STATE               is_slot_needs_rebuild;
} ADM_FRU_CF_DRIVE_STRUCT;


typedef struct adm_fru_missing_disk_struct_tag
{
    UINT_32                 expected_disk_sn_buffer_size;
    UINT_8                  *expected_disk_sn_buffer;
} ADM_FRU_MISSING_DISK_STRUCT;

/***************************
 *  Macro Definitions
 ***************************/


/********************************
 *  Function Prototypes
 ********************************/

#if defined(__cplusplus)
#define CALL_TYPE __stdcall
extern "C"
{
#else
#define CALL_TYPE
#endif

/*
 * Enclosure data
 */
// returns enclosure physical bus
UINT_32 CALL_TYPE dyn_enclosure_bus(OPAQUE_PTR handle,
                                    UINT_32 enclosure);

// returns enclosure address (front panel switch)
UINT_32 CALL_TYPE dyn_enclosure_address(OPAQUE_PTR handle,
                                        UINT_32 enclosure);

// returns unique id for the enclosure
UINT_32 CALL_TYPE dyn_enclosure_enclosure_number(OPAQUE_PTR handle,
                                                 UINT_32 enclosure);

// returns fault_reason_code for the enclosure
UINT_32 CALL_TYPE dyn_enclosure_fault_reason_code(OPAQUE_PTR handle,
                                                  UINT_32 enclosure);

// returns enclosure loop location (order on bus)
UINT_32 CALL_TYPE dyn_enclosure_loop_location(OPAQUE_PTR handle,
                                              UINT_32 enclosure);

// returns enclosure loop location (order on bus)
UINT_32 CALL_TYPE dyn_enclosure_index(OPAQUE_PTR handle,
                                              UINT_32 enclosure);

// returns enclosure chassis type (0=tower, 1=rackmount)
ADM_ENCLOSURE_CHASSIS_TYPE CALL_TYPE
    dyn_enclosure_chassis_type(OPAQUE_PTR handle,
                               UINT_32 enclosure);

// returns enclosure type (DPE, DAE, XPE)
ADM_ENCL_TYPE CALL_TYPE dyn_enclosure_encl_type(OPAQUE_PTR handle,
                                                UINT_32 enclosure);

// returns the number of SP slots in this enclosure
// DPE-2, DAE-normally 0, except 2 in X-1 DAE0_0, XPE-2
UINT_32 CALL_TYPE dyn_enclosure_sp_count(OPAQUE_PTR handle,
                                         UINT_32 enclosure);

// returns enclosure state, see cm_environ_if.h for values
CM_ENCL_STATE CALL_TYPE dyn_enclosure_state(OPAQUE_PTR handle,
                                            UINT_32 enclosure);

// returns TRUE if the flash masks have been sent to the LCC
TRI_STATE CALL_TYPE dyn_enclosure_flash_enable(OPAQUE_PTR handle,
                                               UINT_32 enclosure);

// returns the SP's current values for the flash masks (may not
// correspond to the actual LCC settings)
ADM_FLASH_BITS CALL_TYPE dyn_enclosure_flash_bits(OPAQUE_PTR handle,
                                                  UINT_32 enclosure);

// returns TRUE if the LCC is on the wrong loop
BOOL CALL_TYPE dyn_enclosure_cross_cable(OPAQUE_PTR handle,
                                         UINT_32 enclosure);

// returns TRUE if the enclosure fault LED is lit
BOOL CALL_TYPE dyn_enclosure_fault(OPAQUE_PTR handle,
                                   UINT_32 enclosure);

// returns TRUE if there is an lcc peer channel fault reported in the enclosure
BOOL CALL_TYPE dyn_enclosure_lcc_peer_to_peer_channel_fault(OPAQUE_PTR handle, 
                                                            UINT_32 enclosure);

// returns TRUE if there is an lcc speed mismatch fault reported in the enclosure
BOOL CALL_TYPE dyn_enclosure_lcc_speed_mismatch_fault(OPAQUE_PTR handle, 
                                                      UINT_32 enclosure);

// returns the enclosure fault symptom
CM_ENCL_FAULT_SYMPTOM CALL_TYPE dyn_enclosure_flt_symptom(OPAQUE_PTR handle, UINT_32 enclosure);

// returns the current speed of the enclosure
UINT_32 CALL_TYPE dyn_enclosure_current_speed(OPAQUE_PTR handle, UINT_32 enclosure);

// returns the maximum speed capability of the enclosure
UINT_32 CALL_TYPE dyn_enclosure_max_speed(OPAQUE_PTR handle, UINT_32 enclosure);

// returns the duplicate enclosure count
UINT_32 CALL_TYPE dyn_enclosure_duplicate_count(OPAQUE_PTR handle, UINT_32 enclosure);

UINT_32 CALL_TYPE   dyn_enclosure_missing_numbers(OPAQUE_PTR handle, UINT_32 enclosure);

UINT_32 CALL_TYPE   dyn_enclosure_available_slots(OPAQUE_PTR handle, UINT_32 enclosure);

BOOL CALL_TYPE dyn_bus_max_enclosures_exceeded(OPAQUE_PTR handle, UINT_32 bus);

/*
 * Enclosure Resum� PROM
 */

// fills in the buffer with the EMC part number string
// the buffer_size parameter indicates the maximum size accepted by the
// caller
// the return value indicates the actual number of bytes entered into
// the buffer (ADM_EMC_PART_NUMBER_SIZE)
UINT_32 CALL_TYPE dyn_encl_emc_part_no(OPAQUE_PTR handle,
                                       UINT_32 enclosure,
                                       UINT_32 buffer_size,
                                       TEXT *buffer);

// fills in the buffer with the EMC artwork revision level string
// the buffer_size parameter indicates the maximum size accepted by the
// caller
// the return value indicates the actual number of bytes entered into
// the buffer (ADM_ARTWORK_REVISION_SIZE)
UINT_32 CALL_TYPE dyn_encl_emc_artwork_revision_level(OPAQUE_PTR handle,
                                                      UINT_32 enclosure,
                                                      UINT_32 buffer_size,
                                                      TEXT *buffer);

// fills in the buffer with the EMC assembly revision level string
// the buffer_size parameter indicates the maximum size accepted by the
// caller
// the return value indicates the actual number of bytes entered into
// the buffer (ADM_ASSEMBLY_REVISION_SIZE)
UINT_32 CALL_TYPE dyn_encl_emc_assembly_revision_level(OPAQUE_PTR handle,
                                                       UINT_32 enclosure,
                                                       UINT_32 buffer_size,
                                                       TEXT *buffer);

// fills in the buffer with the EMC serial number string
// the buffer_size parameter indicates the maximum size accepted by the
// caller
// the return value indicates the actual number of bytes entered into
// the buffer (ADM_EMC_SERIAL_NO_SIZE)
UINT_32 CALL_TYPE dyn_encl_emc_serial_number(OPAQUE_PTR handle,
                                             UINT_32 enclosure,
                                             UINT_32 buffer_size,
                                             TEXT *buffer);

// fills in the buffer with the vendor part number
// the buffer_size parameter indicates the maximum size accepted by the
// caller
// the return value indicates the actual number of bytes entered into
// the buffer (ADM_VENDOR_PART_NO_SIZE)
UINT_32 CALL_TYPE dyn_encl_vendor_part_no(OPAQUE_PTR handle,
                                          UINT_32 enclosure,
                                          UINT_32 buffer_size,
                                          TEXT *buffer);

// fills in the buffer with the Vendor artwork revision level string
// the buffer_size parameter indicates the maximum size accepted by the
// caller
// the return value indicates the actual number of bytes entered into
// the buffer (ADM_ARTWORK_REVISION_SIZE)
UINT_32 CALL_TYPE dyn_encl_vendor_artwork_revision_level(OPAQUE_PTR handle,
                                                         UINT_32 enclosure,
                                                         UINT_32 buffer_size,
                                                         TEXT *buffer);


// fills in the buffer with the Vendor assembly revision level string
// the buffer_size parameter indicates the maximum size accepted by the
// caller
// the return value indicates the actual number of bytes entered into
// the buffer (ADM_ASSEMBLY_REVISION_SIZE)
UINT_32 CALL_TYPE dyn_encl_vendor_assembly_revision_level(OPAQUE_PTR handle,
                                                       UINT_32 enclosure,
                                                       UINT_32 buffer_size,
                                                       TEXT *buffer);

// fills in the buffer with the Vendor serial number string
// the buffer_size parameter indicates the maximum size accepted by the
// caller
// the return value indicates the actual number of bytes entered into
// the buffer (ADM_VENDOR_SERIAL_NO_SIZE)
UINT_32 CALL_TYPE dyn_encl_vendor_serial_number(OPAQUE_PTR handle,
                                                UINT_32 enclosure,
                                                UINT_32 buffer_size,
                                                TEXT *buffer);

// fills in the buffer with the vendor name string
// the buffer_size parameter indicates the maximum size accepted by the
// caller
// the return value indicates the actual number of bytes entered into
// the buffer (ADM_VENDOR_NAME_SIZE)
UINT_32 CALL_TYPE dyn_encl_vendor_name(OPAQUE_PTR handle,
                                       UINT_32 enclosure,
                                       UINT_32 buffer_size,
                                       TEXT *buffer);

// fills in the buffer with the location of manufacture string
// the buffer_size parameter indicates the maximum size accepted by the
// caller
// the return value indicates the actual number of bytes entered into
// the buffer (ADM_LOCATION_MANUFACTURE_SIZE)
UINT_32 CALL_TYPE dyn_encl_location_of_manufacture(OPAQUE_PTR handle,
                                                   UINT_32 enclosure,
                                                   UINT_32 buffer_size,
                                                   TEXT *buffer);

// fills in the buffer with the date of manufacture string (yyyymmdd)
// the buffer_size parameter indicates the maximum size accepted by the
// caller
// the return value indicates the actual number of bytes entered into
// the buffer (ADM_DATE_MANUFACTURE_SIZE)
UINT_32 CALL_TYPE dyn_encl_date_of_manufacture(OPAQUE_PTR handle,
                                               UINT_32 enclosure,
                                               UINT_32 buffer_size,
                                               TEXT *buffer);

// fills in the buffer with the assembly name string
// the buffer_size parameter indicates the maximum size accepted by the
// caller
// the return value indicates the actual number of bytes entered into
// the buffer (ADM_ASSEMBLY_NAME_SIZE)
UINT_32 CALL_TYPE dyn_encl_assembly_name(OPAQUE_PTR handle,
                                         UINT_32 enclosure,
                                         UINT_32 buffer_size,
                                         TEXT *buffer);

// returns the WWN seed from the enclosure resum� PROM
UINT_32 CALL_TYPE dyn_encl_wwn_seed(OPAQUE_PTR handle, UINT_32 enclosure);

//  This function returns TRUE it the last resume PROM read was not successful and FALSE
//  if the last read was successful. If TRUE is returned than CM is lighting a
//  FLT light!

BOOL CALL_TYPE dyn_encl_resume_faulted(OPAQUE_PTR handle, UINT_32 enclosure);

// returns the product serial number
UINT_32 CALL_TYPE dyn_product_serial_number(OPAQUE_PTR handle,
                                           UINT_32 buffer_size,
                                           TEXT *buffer);

// returns the product part number
UINT_32 CALL_TYPE dyn_product_part_number(OPAQUE_PTR handle,
                                           UINT_32 buffer_size,
                                           TEXT *buffer);
// returns the product revision
UINT_32 CALL_TYPE dyn_product_rev(OPAQUE_PTR handle,
                                  UINT_32 buffer_size,
                                  TEXT *buffer);

UINT_32 CALL_TYPE dyn_encl_emc_original_serial_number(OPAQUE_PTR handle,
                                             UINT_32 enclosure,
                                             UINT_32 buffer_size,
                                             TEXT *buffer);
/*
 * Interposer
 */

UINT_32 CALL_TYPE dyn_enclosure_interposer_count(OPAQUE_PTR handle, UINT_32 enclosure);

BOOL    CALL_TYPE dyn_enclosure_interposer_faulted(OPAQUE_PTR handle, UINT_32 enclosure, UINT_32 intpo);
UINT_32 CALL_TYPE dyn_enclosure_interposer_fault_code(OPAQUE_PTR handle, UINT_32 enclosure, UINT_32 intpo);
BOOL    CALL_TYPE dyn_enclosure_interposer_mux_fault(OPAQUE_PTR handle, UINT_32 enclosure, UINT_32 intpo, UINT_32 mux);
UINT_32 CALL_TYPE dyn_enclosure_interposer_mux_fault_code(OPAQUE_PTR handle, UINT_32 enclosure, UINT_32 intpo, UINT_32 mux);

UINT_32  CALL_TYPE dyn_enclosure_interposer_isolated_fault_code
    (OPAQUE_PTR handle, UINT_32 enclosure, UINT_32 intpo);

UINT_32  CALL_TYPE dyn_enclosure_interposer_isolated_fault_slot_bitmap
    (OPAQUE_PTR handle, UINT_32 enclosure, UINT_32 intpo);

BOOL  CALL_TYPE dyn_enclosure_interposer_has_isolated_fault_on_slot
    (OPAQUE_PTR handle, UINT_32 enclosure, UINT_32 intpo, UINT_32 slot);

UINT_32  CALL_TYPE dyn_enclosure_interposer_isolated_fault_lcc_bitmap
    (OPAQUE_PTR handle, UINT_32 enclosure, UINT_32 intpo);

BOOL  CALL_TYPE dyn_enclosure_interposer_has_isolated_fault_on_lcc
    (OPAQUE_PTR handle, UINT_32 enclosure, UINT_32 intpo, UINT_32 lcc);

BOOL  CALL_TYPE dyn_enclosure_interposer_has_definitive_isolated_fault
    (OPAQUE_PTR handle, UINT_32 enclosure, UINT_32 intpo);

BOOL  CALL_TYPE dyn_enclosure_interposer_has_ambiguous_isolated_fault
    (OPAQUE_PTR handle, UINT_32 enclosure, UINT_32 intpo);

/* personality card */

UINT_32 CALL_TYPE dyn_enclosure_perscard_count(OPAQUE_PTR handle, UINT_32 enclosure);

UINT_32 CALL_TYPE dyn_enclosure_perscard_status(OPAQUE_PTR handle, UINT_32 enclosure, UINT_32 perscard_idx);

TRI_STATE CALL_TYPE dyn_enclosure_perscard_inserted(OPAQUE_PTR handle, UINT_32 enclosure, UINT_32 perscard_idx);

//
// Personality Card Resume
//
BOOL CALL_TYPE dyn_encl_pers_card_resume_faulted(OPAQUE_PTR handle, UINT_32 enclosure, UINT_32 perscard_idx);
UINT_32 CALL_TYPE dyn_encl_pers_card_emc_serial_number(OPAQUE_PTR handle,
                                                    UINT_32 enclosure,
                                                    UINT_32 perscard_idx,
                                                    UINT_32 buffer_size,
                                                    TEXT *buffer);
UINT_32 CALL_TYPE dyn_encl_pers_card_vendor_name(OPAQUE_PTR handle,
                                                UINT_32 enclosure,
                                                UINT_32 perscard_idx,
                                                UINT_32 buffer_size,
                                                TEXT *buffer);
UINT_32 CALL_TYPE dyn_encl_pers_card_vendor_part_no(OPAQUE_PTR handle,
                                                UINT_32 enclosure,
                                                UINT_32 perscard_idx,
                                                UINT_32 buffer_size,
                                                TEXT *buffer);
UINT_32 CALL_TYPE dyn_encl_pers_card_vendor_artwork_revision_level(OPAQUE_PTR handle,
                                                UINT_32 enclosure,
                                                UINT_32 perscard_idx,
                                                UINT_32 buffer_size,
                                                TEXT *buffer);
UINT_32 CALL_TYPE dyn_encl_pers_card_vendor_assembly_revision_level(OPAQUE_PTR handle,
                                                UINT_32 enclosure,
                                                UINT_32 perscard_idx,
                                                UINT_32 buffer_size,
                                                TEXT *buffer);
UINT_32 CALL_TYPE dyn_encl_pers_card_vendor_serial_number(OPAQUE_PTR handle,
                                                UINT_32 enclosure,
                                                UINT_32 perscard_idx,
                                                UINT_32 buffer_size,
                                                TEXT *buffer);

UINT_32 CALL_TYPE dyn_encl_pers_card_location_of_manufacture(OPAQUE_PTR handle,
                                                        UINT_32 enclosure,
                                                        UINT_32 perscard_idx,
                                                        UINT_32 buffer_size,
                                                        TEXT *buffer);
UINT_32 CALL_TYPE dyn_encl_pers_card_date_of_manufacture(OPAQUE_PTR handle,
                                                        UINT_32 enclosure,
                                                        UINT_32 perscard_idx,
                                                        UINT_32 buffer_size,
                                                        TEXT *buffer);
UINT_32 CALL_TYPE dyn_encl_pers_card_assembly_name(OPAQUE_PTR handle,
                                                UINT_32 enclosure,
                                                UINT_32 perscard_idx,
                                                UINT_32 buffer_size,
                                                TEXT *buffer);
UINT_32 CALL_TYPE dyn_encl_pers_card_emc_part_no(OPAQUE_PTR handle,
                                       UINT_32 enclosure,
                                       UINT_32 perscard_idx,
                                     UINT_32 buffer_size,
                                     TEXT *buffer);
UINT_32 CALL_TYPE dyn_encl_pers_card_emc_artwork_revision_level(OPAQUE_PTR handle,
                                       UINT_32 enclosure,
                                       UINT_32 perscard_idx,
                                                    UINT_32 buffer_size,
                                                    TEXT *buffer);
UINT_32 CALL_TYPE dyn_encl_pers_card_emc_assembly_revision_level(OPAQUE_PTR handle,
                                       UINT_32 enclosure,
                                       UINT_32 perscard_idx,
                                                     UINT_32 buffer_size,
                                                     TEXT *buffer);

/*
 * LCC
 */

// returns the number of LCCs in this enclosure
UINT_32 CALL_TYPE dyn_enclosure_lcc_count(OPAQUE_PTR handle,
                                          UINT_32 enclosure);

// returns TRUE if this LCC is physically last in the loop
BOOL CALL_TYPE dyn_enclosure_lcc_expansion_port_open(OPAQUE_PTR handle,
                                                     UINT_32 enclosure,
                                                     UINT_32 lcc);

// returns TRUE if status has been read from the peer LCC
// in that case, only the fault and inserted flags are valid
BOOL CALL_TYPE dyn_enclosure_lcc_peer_information_bit(OPAQUE_PTR handle,
                                                      UINT_32 enclosure,
                                                      UINT_32 lcc);

// returns TRUE if this LCC has a primary media converter fault
BOOL CALL_TYPE
    dyn_enclosure_lcc_primary_media_converter_fault(OPAQUE_PTR handle,
                                                    UINT_32 enclosure,
                                                    UINT_32 lcc);

// returns TRUE if this LCC has a primary media converter fault
BOOL CALL_TYPE
dyn_enclosure_lcc_expansion_port_media_converter_fault(OPAQUE_PTR handle,
                                                       UINT_32 enclosure,
                                                       UINT_32 lcc);

// returns TRUE if this LCC is logically last in the loop
BOOL CALL_TYPE dyn_enclosure_lcc_shunted(OPAQUE_PTR handle,
                                         UINT_32 enclosure,
                                         UINT_32 lcc);

// returns TRUE if this LCC is faulted
BOOL CALL_TYPE dyn_enclosure_lcc_faulted(OPAQUE_PTR handle,
                                         UINT_32 enclosure,
                                         UINT_32 lcc);

// returns TRUE if this LCC is inserted
BOOL CALL_TYPE dyn_enclosure_lcc_inserted(OPAQUE_PTR handle,
                                          UINT_32 enclosure,
                                          UINT_32 lcc);

// returns the type of the lcc
ADM_LCC_TYPE CALL_TYPE dyn_enclosure_lcc_type(OPAQUE_PTR handle, 
                                UINT_32 enclosure, 
                                UINT_32 lcc);

// returns the maximum capable speed of the lcc
UINT_32 CALL_TYPE dyn_enclosure_lcc_max_speed(OPAQUE_PTR handle, 
                                UINT_32 enclosure, 
                                UINT_32 lcc);

// returns the current speed of the lcc
UINT_32 CALL_TYPE dyn_enclosure_lcc_current_speed(OPAQUE_PTR handle, 
                                UINT_32 enclosure, 
                                UINT_32 lcc);

// returns the current status of the lcc firmware upgrade                                 
BOOL CALL_TYPE dyn_lcc_upgrade_in_progress(OPAQUE_PTR handle);

// returns the current status of the lcc firmware update                                 
BOOL CALL_TYPE dyn_lcc_update_in_progress(OPAQUE_PTR handle);

// returns the current status of the yokon                                  
BOOL CALL_TYPE dyn_lcc_yukon_dl_in_progress(OPAQUE_PTR handle);

// returns the remaining time in sencond for lcc firmware download 
UINT_32 CALL_TYPE dyn_lcc_sec_bef_ugrade_chk(OPAQUE_PTR handle);

// returns the lcc currentstatus of interval 
UINT_16 CALL_TYPE dyn_lcc_CurrentStatusInterval(OPAQUE_PTR handle);

// returns the lcc current state 
UINT_8 CALL_TYPE dyn_lcc_current_state(OPAQUE_PTR handle,
                                UINT_8 bus_num);

// returns the lcc current status code
UINT_8 CALL_TYPE dyn_lcc_current_StatusCode(OPAQUE_PTR handle);

// returns the lcc current error code
UINT_8 CALL_TYPE dyn_lcc_current_ErrorCode(OPAQUE_PTR handle);

// returns the number of total enclosures to upgrade.
UINT_8 CALL_TYPE dyn_lcc_tot_encl_to_ugrade(OPAQUE_PTR handle);

// returns the number of enclosures completed firmware upgradation
UINT_8 CALL_TYPE dyn_lcc_enclosures_completed(OPAQUE_PTR handle);

//returns the Enclosure currently being upgraded
UINT_8 CALL_TYPE dyn_lcc_enclosure_number(OPAQUE_PTR handle);

//returns the bus number of enclosure currently being upgraded
UINT_8 CALL_TYPE dyn_lcc_bus_number(OPAQUE_PTR handle);

//returns the frumon revision
UINT_32 CALL_TYPE dyn_lcc_frumon_rev(OPAQUE_PTR handle,
                                               UINT_32 encl_no,
                                               UINT_32 buffer_size,
                                               TEXT *buffer);

//returns the MAX_CONNECTABLE_ENCL_TYPES
UINT_8 CALL_TYPE dyn_lcc_MAX_CONNECTABLE_ENCL_TYPES(void);

//returns the LCC firmware download percent complete
UINT_8 CALL_TYPE dyn_lcc_dwPercentComplete(OPAQUE_PTR handle);

//returns the queued upgrade requests 
UINT_8 CALL_TYPE dyn_lcc_queued_upgrade_requests(OPAQUE_PTR handle);

// return ssp_sas_addr
UINT_64 CALL_TYPE dyn_enclosure_lcc_ssp_sas_addr(OPAQUE_PTR handle,
                                                 UINT_32 enclosure,
                                                 UINT_32 lcc);

// return unique id
UINT_32 CALL_TYPE dyn_enclosure_lcc_unique_id(OPAQUE_PTR handle,
                                              UINT_32 enclosure,
                                              UINT_32 lcc);

// return fault code for the expander
UINT_32 CALL_TYPE dyn_enclosure_lcc_fault_code(OPAQUE_PTR handle,
                                               UINT_32 enclosure,
                                               UINT_32 lcc);

// return fault code for the LCC
UINT_32  CALL_TYPE dyn_enclosure_lcc_isolated_fault_code(
    OPAQUE_PTR handle, UINT_32 enclosure, UINT_32 lcc);

UINT_32  CALL_TYPE dyn_enclosure_lcc_isolated_fault_slot_bitmap(
    OPAQUE_PTR handle, UINT_32 enclosure, UINT_32 lcc);

BOOL  CALL_TYPE dyn_enclosure_lcc_has_isolated_fault_on_slot(
    OPAQUE_PTR handle, UINT_32 enclosure, UINT_32 lcc, UINT_32 slot);

// TRUE if the LCC has a definitive isolated fault
BOOL  CALL_TYPE dyn_enclosure_lcc_has_definitive_isolated_fault(
    OPAQUE_PTR handle, UINT_32 enclosure, UINT_32 lcc);

// TRUE if the LCC has anambiguous isolated fault
BOOL  CALL_TYPE dyn_enclosure_lcc_has_ambiguous_isolated_fault(
    OPAQUE_PTR handle, UINT_32 enclosure, UINT_32 lcc);

// TRUE if the LCC has a cabling fault
BOOL  CALL_TYPE dyn_enclosure_lcc_has_cabling_fault(
    OPAQUE_PTR handle, UINT_32 enclosure, UINT_32 lcc);

// return cabling fault code for the expander
UINT_32 CALL_TYPE dyn_enclosure_lcc_cabling_fault_code(OPAQUE_PTR handle,
                                                        UINT_32 enclosure,
                                                        UINT_32 lcc);

// return downstream neighbor sas address
UINT_64 CALL_TYPE dyn_enclosure_lcc_downstream_neighbor_sas_addr(OPAQUE_PTR handle,
                                                                UINT_32 enclosure,
                                                                UINT_32 lcc);

// return downstream neighbor unique id
UINT_32 CALL_TYPE dyn_enclosure_lcc_downstream_neighbor_unique_id(OPAQUE_PTR handle,
                                                                UINT_32 enclosure,
                                                                UINT_32 lcc);

//return the lcc_upgrade_start_time
LARGE_INTEGER CALL_TYPE dyn_lcc_upgrade_start_time(OPAQUE_PTR handle);

//return the lcc_firmware_check_start_time
LARGE_INTEGER   CALL_TYPE dyn_lcc_firmware_check_start_time(OPAQUE_PTR handle);
  
//return the lcc_upgrade_state
UINT_8 CALL_TYPE dyn_lcc_upgrade_state(OPAQUE_PTR handle ,UINT_8  EncAddr);
 

/*
 * LCC Resum�
 */

// fills in the buffer with the EMC part number string
// the buffer_size parameter indicates the maximum size accepted by the
// caller
// the return value indicates the actual number of bytes entered into
// the buffer (ADM_EMC_PART_NUMBER_SIZE)
UINT_32 CALL_TYPE dyn_lcc_emc_part_no(OPAQUE_PTR handle,
                                      UINT_32 enclosure,
                                      UINT_32 lcc,
                                      UINT_32 buffer_size,
                                      TEXT *buffer);

// fills in the buffer with the EMC artwork revision level string
// the buffer_size parameter indicates the maximum size accepted by the
// caller
// the return value indicates the actual number of bytes entered into
// the buffer (ADM_ARTWORK_REVISION_SIZE)
UINT_32 CALL_TYPE dyn_lcc_emc_artwork_revision_level(OPAQUE_PTR handle,
                                                     UINT_32 enclosure,
                                                     UINT_32 lcc,
                                                     UINT_32 buffer_size,
                                                     TEXT *buffer);

// fills in the buffer with the EMC assembly revision level string
// the buffer_size parameter indicates the maximum size accepted by the
// caller
// the return value indicates the actual number of bytes entered into
// the buffer (ADM_ASSEMBLY_REVISION_SIZE)
UINT_32 CALL_TYPE dyn_lcc_emc_assembly_revision_level(OPAQUE_PTR handle,
                                                      UINT_32 enclosure,
                                                      UINT_32 lcc,
                                                      UINT_32 buffer_size,
                                                      TEXT *buffer);

// fills in the buffer with the EMC serial number string
// the buffer_size parameter indicates the maximum size accepted by the
// caller
// the return value indicates the actual number of bytes entered into
// the buffer (ADM_EMC_SERIAL_NO_SIZE)
UINT_32 CALL_TYPE dyn_lcc_emc_serial_number(OPAQUE_PTR handle,
                                            UINT_32 enclosure,
                                            UINT_32 lcc,
                                            UINT_32 buffer_size,
                                            TEXT *buffer);

// fills in the buffer with the Vendor part number string
// the buffer_size parameter indicates the maximum size accepted by the
// caller
// the return value indicates the actual number of bytes entered into
// the buffer (ADM_VENDOR_PART_NUMBER_SIZE)
UINT_32 CALL_TYPE dyn_lcc_vendor_part_no(OPAQUE_PTR handle,
                                         UINT_32 enclosure,
                                         UINT_32 lcc,
                                         UINT_32 buffer_size,
                                         TEXT *buffer);

// fills in the buffer with the Vendor artwork revision level string
// the buffer_size parameter indicates the maximum size accepted by the
// caller
// the return value indicates the actual number of bytes entered into
// the buffer (ADM_ARTWORK_REVISION_SIZE)
UINT_32 CALL_TYPE dyn_lcc_vendor_artwork_revision_level(OPAQUE_PTR handle,
                                                        UINT_32 enclosure,
                                                        UINT_32 lcc,
                                                        UINT_32 buffer_size,
                                                        TEXT *buffer);

// fills in the buffer with the Vendor assembly revision level string
// the buffer_size parameter indicates the maximum size accepted by the
// caller
// the return value indicates the actual number of bytes entered into
// the buffer (ADM_ASSEMBLY_REVISION_SIZE)
UINT_32 CALL_TYPE dyn_lcc_vendor_assembly_revision_level(OPAQUE_PTR handle,
                                                         UINT_32 enclosure,
                                                         UINT_32 lcc,
                                                         UINT_32 buffer_size,
                                                         TEXT *buffer);

// fills in the buffer with the Vendor serial number string
// the buffer_size parameter indicates the maximum size accepted by the
// caller
// the return value indicates the actual number of bytes entered into
// the buffer (ADM_VENDOR_SERIAL_NO_SIZE)
UINT_32 CALL_TYPE dyn_lcc_vendor_serial_number(OPAQUE_PTR handle,
                                               UINT_32 enclosure,
                                               UINT_32 lcc,
                                               UINT_32 buffer_size,
                                               TEXT *buffer);

// fills in the buffer with the vendor name string
// the buffer_size parameter indicates the maximum size accepted by the
// caller
// the return value indicates the actual number of bytes entered into
// the buffer (ADM_VENDOR_NAME_SIZE)
UINT_32 CALL_TYPE dyn_lcc_vendor_name(OPAQUE_PTR handle,
                                      UINT_32 enclosure,
                                      UINT_32 lcc,
                                      UINT_32 buffer_size,
                                      TEXT *buffer);

// fills in the buffer with the location of manufacture string
// the buffer_size parameter indicates the maximum size accepted by the
// caller
// the return value indicates the actual number of bytes entered into
// the buffer (ADM_LOCATION_MANUFACTURE_SIZE)
UINT_32 CALL_TYPE dyn_lcc_location_of_manufacture(OPAQUE_PTR handle,
                                                  UINT_32 enclosure,
                                                  UINT_32 lcc,
                                                  UINT_32 buffer_size,
                                                  TEXT *buffer);

// fills in the buffer with the date of manufacture string (yyyymmdd)
// the buffer_size parameter indicates the maximum size accepted by the
// caller
// the return value indicates the actual number of bytes entered into
// the buffer (ADM_DATE_MANUFACTURE_SIZE)
UINT_32 CALL_TYPE dyn_lcc_date_of_manufacture(OPAQUE_PTR handle,
                                              UINT_32 enclosure,
                                              UINT_32 lcc,
                                              UINT_32 buffer_size,
                                              TEXT *buffer);

// fills in the buffer with the assembly name string
// the buffer_size parameter indicates the maximum size accepted by the
// caller
// the return value indicates the actual number of bytes entered into
// the buffer (ADM_ASSEMBLY_NAME_SIZE)
UINT_32 CALL_TYPE dyn_lcc_assembly_name(OPAQUE_PTR handle,
                                        UINT_32 enclosure,
                                        UINT_32 lcc,
                                        UINT_32 buffer_size,
                                        TEXT *buffer);

// the return value indicates the number of programmables listed in the
// resum� PROM
UINT_32 CALL_TYPE dyn_lcc_num_programmables(OPAQUE_PTR handle,
                                            UINT_32 enclosure,
                                            UINT_32 lcc);

// Fills in the buffer with the name of the requested programmable
// The programmable parameter indicates which programmable is requested
// The buffer_size parameter indicates the maximum size accepted by the
// caller
// The return value indicates the actual number of bytes entered into
// the buffer (ADM_PROGRAMMABLE_NAME_SIZE)
UINT_32 CALL_TYPE dyn_lcc_programmable_name(OPAQUE_PTR handle,
                                            UINT_32 enclosure,
                                            UINT_32 lcc,
                                            UINT_32 programmable,
                                            UINT_32 buffer_size,
                                            TEXT *buffer);

// Fills in the buffer with the revision of the requested programmable
// The programmable parameter indicates which programmable is requested
// The buffer_size parameter indicates the maximum size accepted by the
// caller
// The return value indicates the actual number of bytes entered into
// the buffer (ADM_PROGRAMMABLE_NAME_SIZE)
UINT_32 CALL_TYPE dyn_lcc_programmable_revision(OPAQUE_PTR handle,
                                                UINT_32 enclosure,
                                                UINT_32 lcc,
                                                UINT_32 programmable,
                                                UINT_32 buffer_size,
                                                TEXT *buffer);

//  This function returns TRUE it the last resume PROM read was not successful and FALSE
//  if the last read was successful. If TRUE is returned than CM is lighting a
//  FLT light!

BOOL CALL_TYPE dyn_lcc_resume_faulted(OPAQUE_PTR handle,
                     UINT_32 enclosure,
                     UINT_32 lcc);

/*
 * Power Supply
 */

// returns the number of power supplies in this enclosure
UINT_32 CALL_TYPE dyn_enclosure_ps_count(OPAQUE_PTR handle,
                                         UINT_32 enclosure);

// returns TRUE if this power supply has a thermal fault
BOOL CALL_TYPE dyn_enclosure_ps_thermal_fault(OPAQUE_PTR handle,
                                              UINT_32 enclosure,
                                              UINT_32 ps);

// returns TRUE if this power supply has shutdown
BOOL CALL_TYPE dyn_enclosure_ps_shutdown(OPAQUE_PTR handle,
                                         UINT_32 enclosure,
                                         UINT_32 ps);

// returns TRUE if this power supply is faulted
BOOL CALL_TYPE dyn_enclosure_ps_faulted(OPAQUE_PTR handle,
                                        UINT_32 enclosure,
                                        UINT_32 ps);

// returns TRUE if this power supply is inserted
BOOL CALL_TYPE dyn_enclosure_ps_inserted(OPAQUE_PTR handle,
                                         UINT_32 enclosure,
                                         UINT_32 ps);

// returns TRUE if this power supply is supplying 12V power
TRI_STATE CALL_TYPE dyn_enclosure_ps_12v_ok(OPAQUE_PTR handle,
                                            UINT_32 enclosure,
                                            UINT_32 ps);

// returns TRUE if AC power to this power supply has failed
ENCLOSURE_STATUS CALL_TYPE dyn_enclosure_ps_ac_fail(OPAQUE_PTR handle,
                                        UINT_32 enclosure,
                                        UINT_32 ps);

// returns ps_state_invalid field
BOOL CALL_TYPE dyn_enclosure_ps_state_invalid(OPAQUE_PTR handle, UINT_32 enclosure, UINT_32 ps);

UINT_32  CALL_TYPE dyn_enclosure_ps_isolated_fault_code
    (OPAQUE_PTR handle, UINT_32 enclosure, UINT_32 ps);

BOOL  CALL_TYPE dyn_enclosure_ps_has_definitive_isolated_fault
    (OPAQUE_PTR handle, UINT_32 enclosure, UINT_32 ps);

BOOL  CALL_TYPE dyn_enclosure_ps_has_ambiguous_isolated_fault
    (OPAQUE_PTR handle, UINT_32 enclosure, UINT_32 ps);

//  See description in function header in dyn_power_supply.c
BOOL CALL_TYPE dyn_enclosure_ps_data_valid(OPAQUE_PTR handle,
                                           UINT_32 enclosure,
                                           UINT_32 ps);

//  See description in function header in dyn_power_supply.c
UINT_8 CALL_TYPE dyn_enclosure_ps_number_of_system_ps(OPAQUE_PTR handle,
                                                      UINT_32 enclosure,
                                                      UINT_32 ps);

//  See description in function header in dyn_power_supply.c
UINT_8 CALL_TYPE dyn_enclosure_ps_number_of_local_ps(OPAQUE_PTR handle,
                                                     UINT_32 enclosure,
                                                     UINT_32 ps);

//  See description in function header in dyn_power_supply.c
ENCLOSURE_FRU_SLOT CALL_TYPE dyn_enclosure_ps_slot_number(OPAQUE_PTR handle,
                                                          UINT_32 enclosure,
                                                          UINT_32 ps);

//  See description in function header in dyn_power_supply.c
ENCLOSURE_SP_ID CALL_TYPE dyn_enclosure_ps_associated_sp(OPAQUE_PTR handle,
                                                         UINT_32 enclosure,
                                                         UINT_32 ps);

//  See description in function header in dyn_power_supply.c
ENCLOSURE_STATUS CALL_TYPE dyn_enclosure_ps_general_ps_failure(OPAQUE_PTR handle,
                                                               UINT_32 enclosure,
                                                               UINT_32 ps);

//  See description in function header in dyn_power_supply.c
ENCLOSURE_STATUS CALL_TYPE dyn_enclosure_ps_fully_seated_flt(OPAQUE_PTR handle,
                                                             UINT_32 enclosure,
                                                             UINT_32 ps);

//  See description in function header in dyn_power_supply.c
ENCLOSURE_STATUS CALL_TYPE dyn_enclosure_ps_ps_avail_flt(OPAQUE_PTR handle,
                                                         UINT_32 enclosure,
                                                         UINT_32 ps);

//  See description in function header in dyn_power_supply.c
ENCLOSURE_STATUS CALL_TYPE dyn_enclosure_ps_peer_aggregate_power_flt(OPAQUE_PTR handle,
                                                                     UINT_32 enclosure,
                                                                     UINT_32 ps);
// See description in function header in dyn_power_supply.c
ENCLOSURE_STATUS CALL_TYPE dyn_enclosure_ps_smb_access_fail(OPAQUE_PTR handle,
                                        UINT_32 enclosure,
                                        UINT_32 ps);

// See description in function header in dyn_power_supply.c
ENCLOSURE_STATUS CALL_TYPE dyn_enclosure_ps_data_stale(OPAQUE_PTR handle,
                                        UINT_32 enclosure,
                                        UINT_32 ps);

// See description in function header in dyn_power_supply.c
BOOL CALL_TYPE dyn_enclosure_ps_supported(OPAQUE_PTR handle,
                                          UINT_32 enclosure,
                                          UINT_32 ps);

// See description in function header in dyn_power_supply.c
BOOL CALL_TYPE dyn_enclosure_ps_primary_path_data_valid(OPAQUE_PTR handle,
                                 UINT_32 enclosure,
                                 UINT_32 ps);

// See description in function header in dyn_power_supply.c
BOOL CALL_TYPE dyn_enclosure_ps_secondary_path_data_valid(OPAQUE_PTR handle,
                                 UINT_32 enclosure,
                                 UINT_32 ps);

// returns the current status of the ps firmware upgrade                                 
BOOL CALL_TYPE dyn_ps_upgrade_in_progress(OPAQUE_PTR handle);

// returns the current status of the fan firmware upgrade                                 
BOOL CALL_TYPE dyn_fan_upgrade_in_progress(OPAQUE_PTR handle);


/*
 * Power Supply Resum�
 */

// fills in the buffer with the EMC part number string
// the buffer_size parameter indicates the maximum size accepted by the
// caller
// the return value indicates the actual number of bytes entered into
// the buffer (ADM_EMC_PART_NUMBER_SIZE)
UINT_32 CALL_TYPE dyn_ps_emc_part_no(OPAQUE_PTR handle,
                                     UINT_32 enclosure,
                                     UINT_32 ps,
                                     UINT_32 buffer_size,
                                     TEXT *buffer);

// fills in the buffer with the EMC artwork revision level string
// the buffer_size parameter indicates the maximum size accepted by the
// caller
// the return value indicates the actual number of bytes entered into
// the buffer (ADM_ARTWORK_REVISION_SIZE)
UINT_32 CALL_TYPE dyn_ps_emc_artwork_revision_level(OPAQUE_PTR handle,
                                                    UINT_32 enclosure,
                                                    UINT_32 ps,
                                                    UINT_32 buffer_size,
                                                    TEXT *buffer);

// fills in the buffer with the EMC assembly revision level string
// the buffer_size parameter indicates the maximum size accepted by the
// caller
// the return value indicates the actual number of bytes entered into
// the buffer (ADM_ASSEMBLY_REVISION_SIZE)
UINT_32 CALL_TYPE dyn_ps_emc_assembly_revision_level(OPAQUE_PTR handle,
                                                     UINT_32 enclosure,
                                                     UINT_32 ps,
                                                     UINT_32 buffer_size,
                                                     TEXT *buffer);

// fills in the buffer with the EMC serial number string
// the buffer_size parameter indicates the maximum size accepted by the
// caller
// the return value indicates the actual number of bytes entered into
// the buffer (ADM_EMC_SERIAL_NO_SIZE)
UINT_32 CALL_TYPE dyn_ps_emc_serial_number(OPAQUE_PTR handle,
                                           UINT_32 enclosure,
                                           UINT_32 ps,
                                           UINT_32 buffer_size,
                                           TEXT *buffer);

// fills in the buffer with the vendor part number string
// the buffer_size parameter indicates the maximum size accepted by the
// caller
// the return value indicates the actual number of bytes entered into
// the buffer (ADM_VENDOR_PART_NUMBER_SIZE)
UINT_32 CALL_TYPE dyn_ps_vendor_part_no(OPAQUE_PTR handle,
                                        UINT_32 enclosure,
                                        UINT_32 ps,
                                        UINT_32 buffer_size,
                                        TEXT *buffer);

// fills in the buffer with the vendor artwork revision level string
// the buffer_size parameter indicates the maximum size accepted by the
// caller
// the return value indicates the actual number of bytes entered into
// the buffer (ADM_ARTWORK_REVISION_SIZE)
UINT_32 CALL_TYPE dyn_ps_vendor_artwork_revision_level(OPAQUE_PTR handle,
                                                       UINT_32 enclosure,
                                                       UINT_32 ps,
                                                       UINT_32 buffer_size,
                                                       TEXT *buffer);

// fills in the buffer with the vendor assembly revision level string
// the buffer_size parameter indicates the maximum size accepted by the
// caller
// the return value indicates the actual number of bytes entered into
// the buffer (ADM_ASSEMBLY_REVISION_SIZE)
UINT_32 CALL_TYPE dyn_ps_vendor_assembly_revision_level(OPAQUE_PTR handle,
                                                        UINT_32 enclosure,
                                                        UINT_32 ps,
                                                        UINT_32 buffer_size,
                                                        TEXT *buffer);

// fills in the buffer with the vendor serial number string
// the buffer_size parameter indicates the maximum size accepted by the
// caller
// the return value indicates the actual number of bytes entered into
// the buffer (ADM_VENDOR_SERIAL_NO_SIZE)
UINT_32 CALL_TYPE dyn_ps_vendor_serial_number(OPAQUE_PTR handle,
                                              UINT_32 enclosure,
                                              UINT_32 ps,
                                              UINT_32 buffer_size,
                                              TEXT *buffer);

// fills in the buffer with the vendor name string
// the buffer_size parameter indicates the maximum size accepted by the
// caller
// the return value indicates the actual number of bytes entered into
// the buffer (ADM_VENDOR_NAME_SIZE)
UINT_32 CALL_TYPE dyn_ps_vendor_name(OPAQUE_PTR handle,
                                     UINT_32 enclosure,
                                     UINT_32 ps,
                                     UINT_32 buffer_size,
                                     TEXT *buffer);

// fills in the buffer with the location of manufacture string
// the buffer_size parameter indicates the maximum size accepted by the
// caller
// the return value indicates the actual number of bytes entered into
// the buffer (ADM_LOCATION_MANUFACTURE_SIZE)
UINT_32 CALL_TYPE dyn_ps_location_of_manufacture(OPAQUE_PTR handle,
                                                 UINT_32 enclosure,
                                                 UINT_32 ps,
                                                 UINT_32 buffer_size,
                                                 TEXT *buffer);

// fills in the buffer with the date of manufacture string (yyyymmdd)
// the buffer_size parameter indicates the maximum size accepted by the
// caller
// the return value indicates the actual number of bytes entered into
// the buffer (ADM_DATE_MANUFACTURE_SIZE)
UINT_32 CALL_TYPE dyn_ps_date_of_manufacture(OPAQUE_PTR handle,
                                             UINT_32 enclosure,
                                             UINT_32 ps,
                                             UINT_32 buffer_size,
                                             TEXT *buffer);

// fills in the buffer with the assembly name string
// the buffer_size parameter indicates the maximum size accepted by the
// caller
// the return value indicates the actual number of bytes entered into
// the buffer (ADM_ASSEMBLY_NAME_SIZE)
UINT_32 CALL_TYPE dyn_ps_assembly_name(OPAQUE_PTR handle,
                                       UINT_32 enclosure,
                                       UINT_32 ps,
                                       UINT_32 buffer_size,
                                       TEXT *buffer);

// the return value indicates the number of programmables listed in the
// resum� PROM
UINT_32 CALL_TYPE dyn_ps_num_programmables(OPAQUE_PTR handle,
                                            UINT_32 enclosure,
                                            UINT_32 ps);

// Fills in the buffer with the name of the requested programmable
// The programmable parameter indicates which programmable is requested
// The buffer_size parameter indicates the maximum size accepted by the
// caller
// The return value indicates the actual number of bytes entered into
// the buffer (ADM_PROGRAMMABLE_NAME_SIZE)
UINT_32 CALL_TYPE dyn_ps_programmable_name(OPAQUE_PTR handle,
                                            UINT_32 enclosure,
                                            UINT_32 ps,
                                            UINT_32 programmable,
                                            UINT_32 buffer_size,
                                            TEXT *buffer);

// Fills in the buffer with the revision of the requested programmable
// The programmable parameter indicates which programmable is requested
// The buffer_size parameter indicates the maximum size accepted by the
// caller
// The return value indicates the actual number of bytes entered into
// the buffer (ADM_PROGRAMMABLE_REV_SIZE)
UINT_32 CALL_TYPE dyn_ps_programmable_revision(OPAQUE_PTR handle,
                                                UINT_32 enclosure,
                                                UINT_32 ps,
                                                UINT_32 programmable,
                                                UINT_32 buffer_size,
                                                TEXT *buffer);

//  This function returns TRUE it the last resume PROM read was not successful and FALSE
//  if the last read was successful. If TRUE is returned than CM is lighting a
//  FLT light!
BOOL CALL_TYPE dyn_ps_resume_faulted(OPAQUE_PTR handle,
                     UINT_32 enclosure,
                     UINT_32 ps);

/*
 * Fan Pack
 */

// returns the number of fan packs in this enclosure
UINT_32 CALL_TYPE dyn_enclosure_fan_pack_count(OPAQUE_PTR handle, 
                                               UINT_32 enclosure);

// returns TRUE if fatal fan fault on this pack
BOOL CALL_TYPE dyn_enclosure_fan_fatal_fault(OPAQUE_PTR handle, 
                                             UINT_32 enclosure, 
                                             UINT_32 fp_idx);

// returns TRUE if fan info is not valid
BOOL CALL_TYPE dyn_enclosure_fan_state_invalid(OPAQUE_PTR handle, 
                                               UINT_32 enclosure, 
                                               UINT_32 fp_idx);

// returns number of individual fans
UINT_32 dyn_enclosure_individual_fan_count(OPAQUE_PTR handle, 
                                           UINT_32 enclosure, 
                                           UINT_32 fp_idx);

// returns individual fan's state
BOOL CALL_TYPE dyn_enclosure_individual_fan_fault(OPAQUE_PTR handle, 
                                                  UINT_32 enclosure, 
                                                  UINT_32 fp_idx, 
                                                  UINT_32 fan);

// returns the number of fan in this enclosure
UINT_32 CALL_TYPE dyn_enclosure_fan_count(OPAQUE_PTR handle,
                                          UINT_32 enclosure);

// returns TRUE if this fan pack has a single fan fault
BOOL CALL_TYPE dyn_enclosure_fan_single_fault(OPAQUE_PTR handle,
                                              UINT_32 enclosure,
                                              UINT_32 fan);

// returns TRUE if this fan pack has a multiple fan fault
BOOL CALL_TYPE dyn_enclosure_fan_multiple_fault(OPAQUE_PTR handle,
                                                UINT_32 enclosure,
                                                UINT_32 fan);

// returns TRUE if this fan pack is inserted
TRI_STATE CALL_TYPE dyn_enclosure_fan_inserted(OPAQUE_PTR handle,
                                               UINT_32 enclosure,
                                               UINT_32 fan);
// returns EN_ST_TRUE if the fan has SMBus access error.
ENCLOSURE_STATUS CALL_TYPE dyn_enclosure_fan_smb_access_fail(OPAQUE_PTR handle,
                                                                UINT_32 enclosure,
                                                                UINT_32 fan);
// returns EN_ST_TRUE if the fan data are stale.
ENCLOSURE_STATUS CALL_TYPE dyn_enclosure_fan_data_stale(OPAQUE_PTR handle,
                                            UINT_32 enclosure,
                                            UINT_32 fan);
// returns EN_ST_TRUE if the general failure of fan is true interpreted by CM.
ENCLOSURE_STATUS CALL_TYPE dyn_enclosure_fan_general_fan_failure(OPAQUE_PTR handle,
                                UINT_32 enclosure,
                                UINT_32 fan);
// returns EN_ST_TRUE if the fan data which needs to be read from SMBus are valid.
BOOL CALL_TYPE dyn_enclosure_fan_data_valid(OPAQUE_PTR handle,
                                UINT_32 enclosure,
                                UINT_32 fan);

UINT_32  CALL_TYPE dyn_enclosure_fan_isolated_fault_code
    (OPAQUE_PTR handle, UINT_32 enclosure, UINT_32 fp_idx);

BOOL  CALL_TYPE dyn_enclosure_fan_has_definitive_isolated_fault
    (OPAQUE_PTR handle, UINT_32 enclosure, UINT_32 fp_idx);

BOOL  CALL_TYPE dyn_enclosure_fan_has_ambiguous_isolated_fault
    (OPAQUE_PTR handle, UINT_32 enclosure, UINT_32 fp_idx);

/*
 * Fan Module Resume
 */

// fills in the buffer with the EMC part number string
// the buffer_size parameter indicates the maximum size accepted by the
// caller
// the return value indicates the actual number of bytes entered into
// the buffer (ADM_EMC_PART_NUMBER_SIZE)
UINT_32 CALL_TYPE dyn_fan_emc_part_no(OPAQUE_PTR handle,
                                     UINT_32 enclosure,
                                     UINT_32 fan,
                                     UINT_32 buffer_size,
                                     TEXT *buffer);

// fills in the buffer with the EMC artwork revision level string
// the buffer_size parameter indicates the maximum size accepted by the
// caller
// the return value indicates the actual number of bytes entered into
// the buffer (ADM_ARTWORK_REVISION_SIZE)
UINT_32 CALL_TYPE dyn_fan_emc_artwork_revision_level(OPAQUE_PTR handle,
                                                    UINT_32 enclosure,
                                                    UINT_32 fan,
                                                    UINT_32 buffer_size,
                                                    TEXT *buffer);

// fills in the buffer with the EMC assembly revision level string
// the buffer_size parameter indicates the maximum size accepted by the
// caller
// the return value indicates the actual number of bytes entered into
// the buffer (ADM_ASSEMBLY_REVISION_SIZE)
UINT_32 CALL_TYPE dyn_fan_emc_assembly_revision_level(OPAQUE_PTR handle,
                                                     UINT_32 enclosure,
                                                     UINT_32 fan,
                                                     UINT_32 buffer_size,
                                                     TEXT *buffer);

// fills in the buffer with the EMC serial number string
// the buffer_size parameter indicates the maximum size accepted by the
// caller
// the return value indicates the actual number of bytes entered into
// the buffer (ADM_EMC_SERIAL_NO_SIZE)
UINT_32 CALL_TYPE dyn_fan_emc_serial_number(OPAQUE_PTR handle,
                                           UINT_32 enclosure,
                                           UINT_32 fan,
                                           UINT_32 buffer_size,
                                           TEXT *buffer);

// fills in the buffer with the vendor part number string
// the buffer_size parameter indicates the maximum size accepted by the
// caller
// the return value indicates the actual number of bytes entered into
// the buffer (ADM_VENDOR_PART_NUMBER_SIZE)
UINT_32 CALL_TYPE dyn_fan_vendor_part_no(OPAQUE_PTR handle,
                                        UINT_32 enclosure,
                                        UINT_32 fan,
                                        UINT_32 buffer_size,
                                        TEXT *buffer);

// fills in the buffer with the vendor artwork revision level string
// the buffer_size parameter indicates the maximum size accepted by the
// caller
// the return value indicates the actual number of bytes entered into
// the buffer (ADM_ARTWORK_REVISION_SIZE)
UINT_32 CALL_TYPE dyn_fan_vendor_artwork_revision_level(OPAQUE_PTR handle,
                                                       UINT_32 enclosure,
                                                       UINT_32 fan,
                                                       UINT_32 buffer_size,
                                                       TEXT *buffer);

// fills in the buffer with the vendor assembly revision level string
// the buffer_size parameter indicates the maximum size accepted by the
// caller
// the return value indicates the actual number of bytes entered into
// the buffer (ADM_ASSEMBLY_REVISION_SIZE)
UINT_32 CALL_TYPE dyn_fan_vendor_assembly_revision_level(OPAQUE_PTR handle,
                                                        UINT_32 enclosure,
                                                        UINT_32 fan,
                                                        UINT_32 buffer_size,
                                                        TEXT *buffer);

// fills in the buffer with the vendor serial number string
// the buffer_size parameter indicates the maximum size accepted by the
// caller
// the return value indicates the actual number of bytes entered into
// the buffer (ADM_VENDOR_SERIAL_NO_SIZE)
UINT_32 CALL_TYPE dyn_fan_vendor_serial_number(OPAQUE_PTR handle,
                                              UINT_32 enclosure,
                                              UINT_32 fan,
                                              UINT_32 buffer_size,
                                              TEXT *buffer);

// fills in the buffer with the vendor name string
// the buffer_size parameter indicates the maximum size accepted by the
// caller
// the return value indicates the actual number of bytes entered into
// the buffer (ADM_VENDOR_NAME_SIZE)
UINT_32 CALL_TYPE dyn_fan_vendor_name(OPAQUE_PTR handle,
                                     UINT_32 enclosure,
                                     UINT_32 fan,
                                     UINT_32 buffer_size,
                                     TEXT *buffer);

// fills in the buffer with the location of manufacture string
// the buffer_size parameter indicates the maximum size accepted by the
// caller
// the return value indicates the actual number of bytes entered into
// the buffer (ADM_LOCATION_MANUFACTURE_SIZE)
UINT_32 CALL_TYPE dyn_fan_location_of_manufacture(OPAQUE_PTR handle,
                                                 UINT_32 enclosure,
                                                 UINT_32 fan,
                                                 UINT_32 buffer_size,
                                                 TEXT *buffer);

// fills in the buffer with the date of manufacture string (yyyymmdd)
// the buffer_size parameter indicates the maximum size accepted by the
// caller
// the return value indicates the actual number of bytes entered into
// the buffer (ADM_DATE_MANUFACTURE_SIZE)
UINT_32 CALL_TYPE dyn_fan_date_of_manufacture(OPAQUE_PTR handle,
                                             UINT_32 enclosure,
                                             UINT_32 fan,
                                             UINT_32 buffer_size,
                                             TEXT *buffer);

// fills in the buffer with the assembly name string
// the buffer_size parameter indicates the maximum size accepted by the
// caller
// the return value indicates the actual number of bytes entered into
// the buffer (ADM_ASSEMBLY_NAME_SIZE)
UINT_32 CALL_TYPE dyn_fan_assembly_name(OPAQUE_PTR handle,
                                       UINT_32 enclosure,
                                       UINT_32 fan,
                                       UINT_32 buffer_size,
                                       TEXT *buffer);

// the return value indicates the number of programmables listed in the
// resum� PROM
UINT_32 CALL_TYPE dyn_fan_num_programmables(OPAQUE_PTR handle,
                                            UINT_32 enclosure,
                                            UINT_32 fan);

// Fills in the buffer with the name of the requested programmable
// The programmable parameter indicates which programmable is requested
// The buffer_size parameter indicates the maximum size accepted by the
// caller
// The return value indicates the actual number of bytes entered into
// the buffer (ADM_PROGRAMMABLE_NAME_SIZE)
UINT_32 CALL_TYPE dyn_fan_programmable_name(OPAQUE_PTR handle,
                                            UINT_32 enclosure,
                                            UINT_32 fan,
                                            UINT_32 programmable,
                                            UINT_32 buffer_size,
                                            TEXT *buffer);

// Fills in the buffer with the revision of the requested programmable
// The programmable parameter indicates which programmable is requested
// The buffer_size parameter indicates the maximum size accepted by the
// caller
// The return value indicates the actual number of bytes entered into
// the buffer (ADM_PROGRAMMABLE_REV_SIZE)
UINT_32 CALL_TYPE dyn_fan_programmable_revision(OPAQUE_PTR handle,
                                                UINT_32 enclosure,
                                                UINT_32 fan,
                                                UINT_32 programmable,
                                                UINT_32 buffer_size,
                                                TEXT *buffer);

//  This function returns TRUE it the last resume PROM read was not successful and FALSE
//  if the last read was successful. If TRUE is returned than CM is lighting a
//  FLT light!
BOOL CALL_TYPE dyn_fan_resume_faulted(OPAQUE_PTR handle,
                     UINT_32 enclosure,
                     UINT_32 fan);
/*
 * SPS
 */

// returns the number of SPSs in this enclosure
UINT_32 CALL_TYPE dyn_enclosure_sps_count(OPAQUE_PTR handle,
                                          UINT_32 enclosure);

// returns current state of SPS
ADM_SPS_STATE CALL_TYPE dyn_enclosure_sps_state(OPAQUE_PTR handle,
                                                UINT_32 enclosure,
                                                UINT_32 sps);

// returns validity of SPS serial/power cable wiring
ADM_SPS_CABLING CALL_TYPE dyn_enclosure_sps_cabling(OPAQUE_PTR handle,
                                                    UINT_32 enclosure,
                                                    UINT_32 sps);

// returns TRUE if this SPS has an AC line fault
BOOL CALL_TYPE dyn_enclosure_sps_ac_line_fault(OPAQUE_PTR handle,
                                               UINT_32 enclosure,
                                               UINT_32 sps);

// returns TRUE if this SPS is charging
BOOL CALL_TYPE dyn_enclosure_sps_charging(OPAQUE_PTR handle,
                                          UINT_32 enclosure,
                                          UINT_32 sps);

// returns TRUE if this SPS has a low battery
BOOL CALL_TYPE dyn_enclosure_sps_low_battery(OPAQUE_PTR handle,
                                             UINT_32 enclosure,
                                             UINT_32 sps);

// returns TRUE if this SPS has failed or its lifetime period has
// expired
BOOL CALL_TYPE dyn_enclosure_sps_replace(OPAQUE_PTR handle,
                                         UINT_32 enclosure,
                                         UINT_32 sps);

// returns TRUE if this SPS is faulted
BOOL CALL_TYPE dyn_enclosure_sps_faulted(OPAQUE_PTR handle,
                                         UINT_32 enclosure,
                                         UINT_32 sps);

// returns TRUE if this SPS is inserted
BOOL CALL_TYPE dyn_enclosure_sps_inserted(OPAQUE_PTR handle,
                                          UINT_32 enclosure,
                                          UINT_32 sps);

// the following functions return SPS "Resume PROM" type info
BOOL CALL_TYPE dyn_is_sps_resume_accessible(OPAQUE_PTR handle,
                                            UINT_32 spId,
                                            UINT_32 spsSlotNumber);
UINT_32 CALL_TYPE dyn_sps_emc_serial_number(OPAQUE_PTR handle,
                                            UINT_32 enclosure,
                                            UINT_32 sps,
                                            UINT_32 buffer_size,
                                            TEXT *buffer);
UINT_32 CALL_TYPE dyn_sps_emc_part_no(OPAQUE_PTR handle,
                                      UINT_32 enclosure,
                                      UINT_32 sps,
                                      UINT_32 buffer_size,
                                      TEXT *buffer);
UINT_32 CALL_TYPE dyn_sps_emc_artwork_revision_level(OPAQUE_PTR handle,
                                                     UINT_32 enclosure,
                                                     UINT_32 sps,
                                                     UINT_32 buffer_size,
                                                     TEXT *buffer);
UINT_32 CALL_TYPE dyn_sps_emc_assembly_revision_level(OPAQUE_PTR handle,
                                                      UINT_32 enclosure,
                                                      UINT_32 sps,
                                                      UINT_32 buffer_size,
                                                      TEXT *buffer);
UINT_32 CALL_TYPE dyn_sps_vendor_part_no(OPAQUE_PTR handle,
                                         UINT_32 enclosure,
                                         UINT_32 sps,
                                         UINT_32 buffer_size,
                                         TEXT *buffer);
UINT_32 CALL_TYPE dyn_sps_vendor_artwork_revision_level(OPAQUE_PTR handle,
                                                        UINT_32 enclosure,
                                                        UINT_32 sps,
                                                        UINT_32 buffer_size,
                                                        TEXT *buffer);
UINT_32 CALL_TYPE dyn_sps_vendor_assembly_revision_level(OPAQUE_PTR handle,
                                                         UINT_32 enclosure,
                                                         UINT_32 sps,
                                                         UINT_32 buffer_size,
                                                         TEXT *buffer);
UINT_32 CALL_TYPE dyn_sps_vendor_serial_number(OPAQUE_PTR handle,
                                               UINT_32 enclosure,
                                               UINT_32 sps,
                                               UINT_32 buffer_size,
                                               TEXT *buffer);
UINT_32 CALL_TYPE dyn_sps_vendor_name(OPAQUE_PTR handle,
                                      UINT_32 enclosure,
                                      UINT_32 sps,
                                      UINT_32 buffer_size,
                                      TEXT *buffer);
UINT_32 CALL_TYPE dyn_sps_location_of_manufacture(OPAQUE_PTR handle,
                                                  UINT_32 enclosure,
                                                  UINT_32 sps,
                                                  UINT_32 buffer_size,
                                                  TEXT *buffer);
UINT_32 CALL_TYPE dyn_sps_date_of_manufacture(OPAQUE_PTR handle,
                                              UINT_32 enclosure,
                                              UINT_32 sps,
                                              UINT_32 buffer_size,
                                              TEXT *buffer);
UINT_32 CALL_TYPE dyn_sps_assembly_name(OPAQUE_PTR handle,
                                        UINT_32 enclosure,
                                        UINT_32 sps,
                                        UINT_32 buffer_size,
                                        TEXT *buffer);
UINT_32 CALL_TYPE dyn_sps_num_programmables(OPAQUE_PTR handle,
                                            UINT_32 enclosure,
                                            UINT_32 sps);
UINT_32 CALL_TYPE dyn_sps_programmable_name(OPAQUE_PTR handle,
                                            UINT_32 enclosure,
                                            UINT_32 sps,
                                            UINT_32 programmable,
                                            UINT_32 buffer_size,
                                            TEXT *buffer);
UINT_32 CALL_TYPE dyn_sps_programmable_revision(OPAQUE_PTR handle,
                                                UINT_32 enclosure,
                                                UINT_32 sps,
                                                UINT_32 programmable,
                                                UINT_32 buffer_size,
                                                TEXT *buffer);
BOOL CALL_TYPE dyn_sps_resume_faulted(OPAQUE_PTR handle,
                                      UINT_32 enclosure,
                                      UINT_32 sps);
ULONG CALL_TYPE dyn_sps_current_input_power(OPAQUE_PTR handle,
                                            UINT_16 sps,
                                            UINT_32 enclosure);
ULONG CALL_TYPE dyn_sps_max_input_power(OPAQUE_PTR handle,
                                        UINT_16 sps,
                                        UINT_32 enclosure);
ULONG CALL_TYPE dyn_sps_average_input_power(OPAQUE_PTR handle,
                                            UINT_16 sps,
                                            UINT_32 enclosure);
ULONG CALL_TYPE dyn_sps_input_power_status(OPAQUE_PTR handle,
                                           UINT_16 sps,
                                           UINT_32 enclosure);
UINT_32  CALL_TYPE dyn_enclosure_sps_isolated_fault_code
    (OPAQUE_PTR handle, UINT_32 enclosure, UINT_32 sps);

BOOL  CALL_TYPE dyn_enclosure_sps_has_definitive_isolated_fault
    (OPAQUE_PTR handle, UINT_32 enclosure, UINT_32 sps);

BOOL  CALL_TYPE dyn_enclosure_sps_has_ambiguous_isolated_fault
    (OPAQUE_PTR handle, UINT_32 enclosure, UINT_32 sps);


/*
 * Disk Drive
 */

// returns the number of drive slots in this enclosure
UINT_32 CALL_TYPE dyn_enclosure_drive_count(OPAQUE_PTR handle,
                                            UINT_32 enclosure);

// returns the FRU number of the drive in this slot
UINT_32 CALL_TYPE dyn_enclosure_drive_fru_number(OPAQUE_PTR handle,
                                                 UINT_32 enclosure,
                                                 UINT_32 slot);

// returns TRUE if the loop A status of the drive in this slot is valid
BOOL CALL_TYPE dyn_enclosure_drive_loop_a_valid(OPAQUE_PTR handle,
                                                UINT_32 enclosure,
                                                UINT_32 slot);

// returns TRUE if the loop B status of the drive in this slot is valid
BOOL CALL_TYPE dyn_enclosure_drive_loop_b_valid(OPAQUE_PTR handle,
                                                UINT_32 enclosure,
                                                UINT_32 slot);

// returns TRUE if the drive in this slot has been taken off loop A
BOOL CALL_TYPE dyn_enclosure_drive_loop_a_bypass(OPAQUE_PTR handle,
                                                 UINT_32 enclosure,
                                                 UINT_32 slot);

// returns TRUE if the drive in this slot has been taken off loop B
BOOL CALL_TYPE dyn_enclosure_drive_loop_b_bypass(OPAQUE_PTR handle,
                                                 UINT_32 enclosure,
                                                 UINT_32 slot);

// returns TRUE if the drive in this slot has requested a bypass on
// loop A
BOOL CALL_TYPE dyn_enclosure_drive_bypass_requested_loop_a(OPAQUE_PTR handle,
                                                           UINT_32 enclosure,
                                                           UINT_32 slot);

// returns TRUE if the drive in this slot has requested a bypass on
// loop B
BOOL CALL_TYPE dyn_enclosure_drive_bypass_requested_loop_b(OPAQUE_PTR handle,
                                                           UINT_32 enclosure,
                                                           UINT_32 slot);

// returns TRUE if the drive in this slot has its fault LED on
BOOL CALL_TYPE dyn_enclosure_drive_faulted(OPAQUE_PTR handle,
                                           UINT_32 enclosure,
                                           UINT_32 slot);


TRI_STATE CALL_TYPE dyn_enclosure_drive_is_marked_NR(OPAQUE_PTR handle, UINT_32 enclosure, UINT_32 slot);
BOOL CALL_TYPE dyn_enclosure_drive_get_CF_drive_info(OPAQUE_PTR handle, 
                                          UINT_32 enclosure, 
                                          UINT_32 slot, 
                                          ADM_FRU_CF_DRIVE_STRUCT *adm_fru_cf_drive_struct_ptr);

BOOL CALL_TYPE dyn_enclosure_drive_get_missing_disk_info(OPAQUE_PTR handle,
                                                         UINT_32 enclosure,
                                                         UINT_32 slot,
                                                         ADM_FRU_MISSING_DISK_STRUCT *missing_disk_info_ptr);

TRI_STATE CALL_TYPE dyn_enclosure_drive_slot_state(OPAQUE_PTR handle, UINT_32 enclosure, UINT_32 slot);

UINT_32 CALL_TYPE dyn_enclosure_drive_fault_code(OPAQUE_PTR handle, UINT_32 enclosure, UINT_32 slot);

UINT_32  CALL_TYPE dyn_enclosure_drive_isolated_fault_code
    (OPAQUE_PTR handle, UINT_32 enclosure, UINT_32 slot);

BOOL  CALL_TYPE dyn_enclosure_drive_has_definitive_isolated_fault
    (OPAQUE_PTR handle, UINT_32 enclosure, UINT_32 slot);

BOOL  CALL_TYPE dyn_enclosure_drive_has_ambiguous_isolated_fault
    (OPAQUE_PTR handle, UINT_32 enclosure, UINT_32 slot);

/*
 * management modules
 */

// Returns the management fru type.
MGMT_FRU_TYPE CALL_TYPE dyn_mgmt_fru_type(OPAQUE_PTR handle, ADM_SP_ID associated_sp);

// Returns the management fru fault led state.
LED_STATE CALL_TYPE dyn_mgmt_fru_led_state(OPAQUE_PTR handle, ADM_SP_ID associated_sp);

// Returns whether the management port link is up or down.
ENCLOSURE_STATUS CALL_TYPE dyn_mgmt_port_link_up(OPAQUE_PTR handle, ADM_SP_ID associated_sp);

// Returns if management port speed configuration is supported or not. 
BOOL CALL_TYPE dyn_mgmt_port_config_speed_supported(OPAQUE_PTR handle, ADM_SP_ID associated_sp);

// Returns all the allowed speeds for the management port.
UINT_32 CALL_TYPE dyn_mgmt_port_allowed_speeds(OPAQUE_PTR handle, ADM_SP_ID associated_sp);

// Returns the management port auto negatiation mode.
MGMT_PORT_AUTO_NEG CALL_TYPE dyn_mgmt_port_auto_negotiate(OPAQUE_PTR handle,
                                                   ADM_SP_ID associated_sp);

// Returns the management port speed.
MGMT_PORT_SPEED CALL_TYPE dyn_mgmt_port_speed(OPAQUE_PTR handle, ADM_SP_ID associated_sp);

// Returns the management port duplex mode.
MGMT_PORT_DUPLEX_MODE CALL_TYPE dyn_mgmt_port_duplex_mode(OPAQUE_PTR handle,
               ADM_SP_ID associated_sp);

// Returns the requested management port auto negotiation mode.
MGMT_PORT_AUTO_NEG CALL_TYPE dyn_mgmt_port_requested_auto_neg(OPAQUE_PTR handle,
                                                   ADM_SP_ID associated_sp);

// Returns the requested management port speed.
MGMT_PORT_SPEED CALL_TYPE dyn_mgmt_port_requested_speed(OPAQUE_PTR handle, ADM_SP_ID associated_sp);

// Returns the requested management duplex mode.
MGMT_PORT_DUPLEX_MODE CALL_TYPE dyn_mgmt_port_requested_duplex_mode(OPAQUE_PTR handle,
               ADM_SP_ID associated_sp);

// Returns the management module firmware revision major part.
UINT_8E CALL_TYPE dyn_mgmt_fru_firmwareRevMajor(OPAQUE_PTR handle,
                                      ADM_SP_ID associated_sp);

// Returns the management module firmware revision minor part.
UINT_8E CALL_TYPE dyn_mgmt_fru_firmwareRevMinor(OPAQUE_PTR handle,
                                 ADM_SP_ID associated_sp);

// Returns the management module vlan configure mode.
MGMT_PORT_VLAN_CONFIG_MODE CALL_TYPE dyn_mgmt_fru_vLanConfigMode(OPAQUE_PTR handle,
                                    ADM_SP_ID associated_sp); 

// Returns if the management module is inserted.
ENCLOSURE_STATUS CALL_TYPE dyn_mgmt_fru_inserted(OPAQUE_PTR handle,
                                    ADM_SP_ID sp); 

// Returns if the management module hardware signal generalFault is faulted.  
ENCLOSURE_STATUS CALL_TYPE dyn_mgmt_fru_generalFault(OPAQUE_PTR handle,
                                    ADM_SP_ID sp); 

// Returns if the management module is faulted interpreted by CM. 
ENCLOSURE_STATUS CALL_TYPE dyn_mgmt_fru_faulted(OPAQUE_PTR handle,
                                    ADM_SP_ID sp);

// Returns if the management module SMBus access failed for a certain amount of time.
ENCLOSURE_STATUS CALL_TYPE dyn_mgmt_fru_smb_access_fail(OPAQUE_PTR handle,
                                    ADM_SP_ID sp);

// Returns if the management module data are stale for a certain amount of time.
ENCLOSURE_STATUS CALL_TYPE dyn_mgmt_fru_data_stale(OPAQUE_PTR handle,
                                    ADM_SP_ID sp);


/*
 * IO adapters
 */

// Returns io adapter power state.
UINT_32 CALL_TYPE dyn_io_adapter_powerState(OPAQUE_PTR handle,
                                    ADM_SP_ID sp);

// Returns io adapter operating mode.
XPE_FRU_OP_MODE CALL_TYPE dyn_io_adapter_operatingMode(OPAQUE_PTR handle,
                                    ADM_SP_ID associated_sp);

// Returns io adapter firmwareRevMajor.
UINT_8E CALL_TYPE dyn_io_adapter_firmwareRevMajor(OPAQUE_PTR handle,
                                    ADM_SP_ID associated_sp);

// Returns io adapter firmwareRevMinor.
UINT_8E CALL_TYPE dyn_io_adapter_firmwareRevMinor(OPAQUE_PTR handle,
                                    ADM_SP_ID associated_sp);

// Returns io adapter fault led state.
LED_STATE CALL_TYPE dyn_io_adapter_led_state(OPAQUE_PTR handle, ADM_SP_ID associated_sp);

// Returns io adapter location.
ENCLOSURE_FRU_LOCATION CALL_TYPE dyn_io_adapter_location(OPAQUE_PTR handle, ADM_SP_ID associated_sp);

// Returns io adapter sub-location.
ENCLOSURE_FRU_SUB_LOCATION CALL_TYPE dyn_io_adapter_sub_location(OPAQUE_PTR handle, ADM_SP_ID associated_sp);

// Returns if the IO adapter data are stale. 
ENCLOSURE_STATUS CALL_TYPE dyn_io_adapter_data_stale(OPAQUE_PTR handle,
                                           ADM_SP_ID sp);

// Returns if the IO adapter SMBus access failed for a certain amount of time.
ENCLOSURE_STATUS CALL_TYPE dyn_io_adapter_smb_access_fail(OPAQUE_PTR handle,
                                           ADM_SP_ID sp); 

// Returns if the general failure of IO adapter is true interpreted by CM.
ENCLOSURE_STATUS CALL_TYPE dyn_io_adapter_faulted(OPAQUE_PTR handle,
                                           ADM_SP_ID sp);

// Returns if the IO adapter generalFault hardware signal is faulted.
ENCLOSURE_STATUS CALL_TYPE dyn_io_adapter_generalFault(OPAQUE_PTR handle,
                                           ADM_SP_ID sp);

// Returns if the IO adapter is inserted.
ENCLOSURE_STATUS CALL_TYPE dyn_io_adapter_inserted(OPAQUE_PTR handle,
                                           ADM_SP_ID sp);

/*
 * CPU modules
 */
ENCLOSURE_STATUS CALL_TYPE dyn_cpu_faulted(OPAQUE_PTR handle, ADM_SP_ID sp);

/*
 * IO modules
 */
// Returns IO module's smbus access fault.
ENCLOSURE_STATUS CALL_TYPE dyn_io_module_smb_access_fail(OPAQUE_PTR handle, ADM_SP_ID sp, IO_MODULE_CLASS iom_class, UINT_32 slot);

// Returns IO module's data stale fault.
ENCLOSURE_STATUS CALL_TYPE dyn_io_module_data_stale(OPAQUE_PTR handle, ADM_SP_ID sp, IO_MODULE_CLASS iom_class, UINT_32 slot);

/*
 * Engine id fault
 */
ENCLOSURE_STATUS CALL_TYPE dyn_engine_id_faulted(OPAQUE_PTR handle);

/*
 * UnsafeToRemoveSp LED
 */
ENCLOSURE_STATUS CALL_TYPE dyn_unsafeToRemoveSpLed(OPAQUE_PTR handle);

/*
 * Energy Info Reporting statistics
 */
ULONG CALL_TYPE dyn_enclosure_current_input_power(OPAQUE_PTR handle, UINT_32 enclosure);
ULONG CALL_TYPE dyn_enclosure_max_input_power(OPAQUE_PTR handle, UINT_32 enclosure);
ULONG CALL_TYPE dyn_enclosure_average_input_power(OPAQUE_PTR handle, UINT_32 enclosure);
ULONG CALL_TYPE dyn_enclosure_input_power_status(OPAQUE_PTR handle, UINT_32 enclosure);
ULONG CALL_TYPE dyn_enclosure_current_air_inlet_temp(OPAQUE_PTR handle, UINT_32 enclosure);
ULONG CALL_TYPE dyn_enclosure_max_air_inlet_temp(OPAQUE_PTR handle, UINT_32 enclosure);
ULONG CALL_TYPE dyn_enclosure_average_air_inlet_temp(OPAQUE_PTR handle, UINT_32 enclosure);
ULONG CALL_TYPE dyn_enclosure_air_inlet_temp_status(OPAQUE_PTR handle, UINT_32 enclosure);
ULONG CALL_TYPE dyn_enclosure_drives_per_bank(OPAQUE_PTR handle, UINT_32 enclosure);

#if defined(__cplusplus)
}
#endif
#endif /* ADM_ENCLOSURE_API_H */

