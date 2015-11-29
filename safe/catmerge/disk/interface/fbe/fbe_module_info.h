/***************************************************************************
 * Copyright (C) EMC Corporation 2010 - 2012
 * All rights reserved. Licensed material -- property of EMC 
 * Corporation 
 ***************************************************************************/

#ifndef FBE_MODULE_INFO_H
#define FBE_MODULE_INFO_H

/*!**************************************************************************
 * @file fbe_module_info.h
 ***************************************************************************
 *
 * @brief
 *  This file contains Module defines & data structures.
 * 
 * @ingroup 
 * 
 * @revision
 *   03/24/2010:  Created. bphilbin
 *
 ***************************************************************************/
#include "specl_types.h"
#include "fbe/fbe_types.h"
#include "fbe_pe_types.h"
#include "speeds.h"
#include "fbe/fbe_port.h"

/*
 * This is used as a value representing information for a particular module can not be found.
 */
#define INVALID_MODULE_NUMBER 0xFE
#define INVALID_PORT_U8       0xFE
#define INVALID_ENCLOSURE_NUMBER 0xFE
#define INVALID_CORE_NUMBER   0xFFFFFFFE

/*
 * The definition of io module limits info
 */
#define FBE_ESP_SLIC_MAX_COUNT              11
#define FBE_ESP_MEZZANINE_MAX_COUNT         1
#define FBE_ESP_IO_MODULE_MAX_COUNT         (FBE_ESP_SLIC_MAX_COUNT + FBE_ESP_MEZZANINE_MAX_COUNT)

#define FBE_ESP_IO_PORT_MAX_COUNT           (FBE_ESP_IO_MODULE_MAX_COUNT * MAX_IO_PORTS_PER_MODULE)


/*!********************************************************************* 
 * @struct fbe_esp_module_mgmt_discovered_hardware_limits_t 
 *  
 * @brief 
 *   Contains the physical characteristics of the hardware as it has been
 *   discovered.
 *   These are all listed on a per SP basis.
 *
 **********************************************************************/
typedef struct fbe_esp_module_mgmt_discovered_hardware_limits_s
{
    fbe_u32_t           num_slic_slots;             /*!< Number of SLIC Slots Present in this System */
    fbe_u32_t           num_mezzanine_slots;        /*!< Number of Mezzanine Slots Present in this System */ 
    fbe_u32_t           num_ports;                  /*!< Maximum number of ports that Could Populate this System */  
    fbe_u32_t           num_bem;                    /*!< Number of Back End Module Slots Present in this System */
    fbe_u32_t           num_mgmt_modules;           /*!< Number of Management Module Slots Present in this System */
}fbe_esp_module_mgmt_discovered_hardware_limits_t;

/*
 * The PORT_ROLE enum defines all the possible Roles the Port can take.
 */
typedef  enum
{
    /*! @todo: using sub script IOPORT for now, need to change later  */
    IOPORT_PORT_ROLE_UNINITIALIZED = 0,
    IOPORT_PORT_ROLE_FE = 1, /* This port is assigned to be Front end */
    IOPORT_PORT_ROLE_BE = 2 /* This port is assigned to be Back end */
} fbe_ioport_role_t;

/*
 * The PORT_SUBROLE enum defines the sub-role the port can take.
 */
typedef  enum
{
    FBE_PORT_SUBROLE_UNINTIALIZED = 0,
    FBE_PORT_SUBROLE_SPECIAL = 1, // FE Ports for MVS, MVA, and SC 
    FBE_PORT_SUBROLE_NORMAL = 2
} fbe_port_subrole_t;

/*
 * The IOM_GROUP enum differentiates between different compatible Io module types.
 * This is made to allow Field Spares of uniquely identified IO Modules.
 */
typedef enum
{   
    FBE_IOM_GROUP_UNKNOWN = 0,

    FBE_IOM_GROUP_A = 1,  // FC 4G - Tomahawk
    FBE_IOM_GROUP_B = 2,  // iSCSI 1G - Harpoon
    FBE_IOM_GROUP_C = 3,  // SAS BE only - Coromandel/Hypernova/MoonLite
    FBE_IOM_GROUP_D = 4,  // FC 8G - Glacier
    FBE_IOM_GROUP_E = 5,  // iSCSI 10G - Poseidon
    FBE_IOM_GROUP_F = 6,  // FCoE - Heatwave
    FBE_IOM_GROUP_G = 7,  // iSCSI 1G 4 Port - Supercell
    FBE_IOM_GROUP_H = 8,  // SAS FE only - Coldfront
    FBE_IOM_GROUP_I = 9,  // iSCSI 10G Copper - Eruption
    FBE_IOM_GROUP_J = 10, // iSCSI 10G Optical - El Nino
    FBE_IOM_GROUP_K = 11, // SAS 6G BE - Moonlite
    FBE_IOM_GROUP_L = 12, // iSCSI 10G 4 Port Optical - Landslide
    FBE_IOM_GROUP_M = 13, // FC 16G - Vortex
    FBE_IOM_GROUP_N = 14, // SAS 12G BE - Snowdevil/Dustdevil
    FBE_IOM_GROUP_O = 15, // iSCSI 1G 4 Port SLIC 2.0 - Thunderhead
    FBE_IOM_GROUP_P = 16, // iSCSI 10G - CNA iSCSI
    FBE_IOM_GROUP_Q = 17, // iSCSI 10G 4 Port SLIC 2.0 - Rockslide
    FBE_IOM_GROUP_R = 18, // iSCSI 10G 2 Port SLIC Gen 3 - MaelstromX 
    FBE_IOM_GROUP_S = 19, // iSCSI 10G 4 Port SLIC Gen 3 - DownburstX 

    FBE_IOM_GROUP_END       // This *must* always be the last entry
} fbe_iom_group_t;



/*
 * The PORT_STATE enum defines the possible states for the port.
 */
typedef enum
{
    FBE_PORT_STATE_UNINITIALIZED = 0,

    /*
     * This is to handle the case  when the IO Module is
     * powered down and don't know what is plugged in.
     */
    FBE_PORT_STATE_UNKNOWN,

    /* Nothing is plugged in and is un-initialized */
    FBE_PORT_STATE_EMPTY,

    /* Nothing is plugged in but the port is initialized */ 
    FBE_PORT_STATE_MISSING,

    /*
     * Problem with the port, the sub-state will provide
     * the reason for fault
     */
    FBE_PORT_STATE_FAULTED,

    /* Everything is good with the port */
    FBE_PORT_STATE_ENABLED,
    FBE_PORT_STATE_UNAVAILABLE,
    FBE_PORT_STATE_DISABLED

} fbe_port_state_t;


/*
* The MODULE_STATE enum defines the possible states for the IO Module.
*/
typedef  enum
{
    MODULE_STATE_UNINITIALIZED = 0,

    /* Nothing is plugged in and is un-initialized. */
    MODULE_STATE_EMPTY,

    /* Nothing is plugged in but the io module is initialized. */   
    MODULE_STATE_MISSING,

    /*
     * Problem with the IO Module, the sub-state will provide
     * the reason for fault.
     */
    MODULE_STATE_FAULTED,

    /* Everything is good with the IO Module */
    MODULE_STATE_ENABLED,

    /* Unsupported IO Module */
    MODULE_STATE_UNSUPPORTED_MODULE

} fbe_module_state_t;


/*
 * The PORT_SUBSTATE enum gives extra information about the port 
 * state, specifically, when the state is MODULE_PORT_STATE_FAULTED. 
 */
typedef enum
{
    /* This particular port is present and no faults were found
     * but is uninitialized.  This substate is applicable for IO
     * modules that do not have SFPs e.g. ISCSI
     */
    FBE_PORT_SUBSTATE_UNINITIALIZED,
    /* This port is uninitialized, but SFP is not inserted.  This
     * substate will be applicable only for FC IO modules.
     */
    FBE_PORT_SUBSTATE_SFP_NOT_PRESENT,
    /* This port is not initialized and IO module is not present */
    FBE_PORT_SUBSTATE_MODULE_NOT_PRESENT,
    /* The port is not present in this module.  The max port per IO
     * module is defined as 4 but iSCSI IO module has only two ports,
     * so the two ports that do not exist will be set to this substate.
     */
    FBE_PORT_SUBSTATE_PORT_NOT_PRESENT,
    /* THis port was initialized but SFP is not plugged in */
    FBE_PORT_SUBSTATE_MISSING_SFP,
    /* This port was initialized but IO Module is not present */
    FBE_PORT_SUBSTATE_MISSING_MODULE,
    /*
     * This port was initialized but an incorrect SFP is inserted that does 
     * not match the persisted info
     */
    FBE_PORT_SUBSTATE_INCORRECT_SFP_TYPE,
    /*
     * The port was initialized but incorrect IO Module is inserted and 
     * thereby the port will also be incorrect 
     */ 
    FBE_PORT_SUBSTATE_INCORRECT_MODULE,
    /* The SFP information could not be read */
    FBE_PORT_SUBSTATE_SFP_READ_ERROR,
     /* This SFP is unsupported for this release */
    FBE_PORT_SUBSTATE_UNSUPPORTED_SFP,
    /* 
     * IO Module had resume PROM read error and cannot determine the type of 
     * port. For uninitialized port the state will be PORT_STATE_UNKNOWN but 
     * for initialized port, since it is stored in the Database, it will be 
     * set to PORT_STATE_ENABLED.
     */
    FBE_PORT_SUBSTATE_MODULE_READ_ERROR,
    /*
     * The number of ports exceeded the allowed limits. If iSCSI MODULE, 
     * the state will be UNINITIALIED but for FC Modules the port will
     * be faulted.
     */
    FBE_PORT_SUBSTATE_EXCEEDED_MAX_LIMITS,
    /*
     * The IO Module is powered off. If the port is not initialized
     * then the state will be known but if the port is initialized 
     * then the port will be faulted.
     */
    FBE_PORT_SUBSTATE_MODULE_POWERED_OFF,
    /* This IO Module is not supported for this release */
    FBE_PORT_SUBSTATE_UNSUPPORTED_MODULE,
    /* 
     * This particular port is present and no faults were 
     * found and in initialized.
     */
    FBE_PORT_SUBSTATE_GOOD,
    FBE_PORT_SUBSTATE_DB_READ_ERROR,
    FBE_PORT_SUBSTATE_FAULTED_SFP,
    FBE_PORT_SUBSTATE_HW_FAULT,
    FBE_PORT_SUBSTATE_UNAVAILABLE,
    FBE_PORT_SUBSTATE_DISABLED_USER_INIT,
    FBE_PORT_SUBSTATE_DISABLED_ENCRYPTION_REQUIRED,
    FBE_PORT_SUBSTATE_DISABLED_HW_FAULT
} fbe_port_substate_t;

/*
 * The MODULE_SUBSTATE enum gives extra information about the IO Module sub-state,
 * specifically, when the state is MODULE_PORT_STATE_FAULTED. 
 */
typedef  enum
{
    /* Module NOT Present but none of the ports are initialized */
    MODULE_SUBSTATE_NOT_PRESENT,    
    /* Module NOT present but atlease one port is initialized */
    MODULE_SUBSTATE_MISSING, 
    /* Error reading the resume PROM on the IO Module */
    MODULE_SUBSTATE_PROM_READ_ERROR,
    /* 
     * Atleast one of the port was initialized in this IO Module but a wrong 
     * IO Module was plugged in 
     */
    MODULE_SUBSTATE_INCORRECT_MODULE,
    /* This IO Module is not supported for this release */
    MODULE_SUBSTATE_UNSUPPORTED_MODULE,
    MODULE_SUBSTATE_GOOD,
    MODULE_SUBSTATE_POWERED_OFF,
    MODULE_SUBSTATE_POWERUP_FAILED,
    MODULE_SUBSTATE_FAULT_REG_FAILED,
    /* This IO Module is not supported until this release is committed */
    MODULE_SUBSTATE_UNSUPPORTED_NOT_COMMITTED,
    /* This IO Module's internal fan is faulted. Like Base Module in Jetfire */
    MODULE_SUBSTATE_INTERNAL_FAN_FAULTED,
    MODULE_SUBSTATE_EXCEEDED_MAX_LIMITS,
    MODULE_SUBSTATE_HW_ERR_MON_FAULT,
} fbe_module_substate_t;


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
    FBE_PORT_PROTOCOL_UNKNOWN = 0,
    FBE_PORT_PROTOCOL_FC,
    FBE_PORT_PROTOCOL_ISCSI,
    FBE_PORT_PROTOCOL_SAS
} FBE_PORT_PROTOCOL;


/*
 * The IOM_POWER enum defines the power status of the IO Module.
 */
typedef enum 
{
    /* 
     * The power status is not available. This is the default setting 
     * if the IO Module is not inserted.
     */
    MODULE_POWER_STATUS_NA,
    MODULE_POWER_STATUS_POWER_ON,
    MODULE_POWER_STATUS_POWER_OFF,
    /* The IO Module was tried to be powered up but it failed */
    MODULE_POWER_STATUS_POWERUP_FAILED
} MODULE_POWER_STATUS;

// SLIC_TYPE Bitmask
// This new bitmask will represent the user-visible SLIC type and can map to a Common SPO Labl Name.
// This is derived from the unique_id in the iom_config_info struct.
typedef enum
{
    FBE_SLIC_TYPE_UNKNOWN = 0x00000000,
    FBE_SLIC_TYPE_FC_4G             = 0x00000001, // Tomahawk
    FBE_SLIC_TYPE_ISCSI_1G          = 0x00000002, // Harpoon
    FBE_SLIC_TYPE_FC_8G             = 0x00000004, // Glacier
    FBE_SLIC_TYPE_ISCSI_10G         = 0x00000008, // Poseidon
    FBE_SLIC_TYPE_SAS               = 0x00000010, // Cormandel
    FBE_SLIC_TYPE_FCOE              = 0x00000020, // Heatwave
    FBE_SLIC_TYPE_4PORT_ISCSI_1G    = 0x00000040, // Supercell
    FBE_SLIC_TYPE_6G_SAS_1          = 0x00000080, // Hypernova
    FBE_SLIC_TYPE_6G_SAS_2          = 0x00000100, // Coldfront
    FBE_SLIC_TYPE_ISCSI_COPPER      = 0x00000200, // Eruption
    FBE_SLIC_TYPE_ISCSI_10G_V2      = 0x00000400, // El Nino
    FBE_SLIC_TYPE_6G_SAS_3          = 0x00000800, // Moonlite
    FBE_SLIC_TYPE_4PORT_ISCSI_10G   = 0x00001000, // eLandslide
    FBE_SLIC_TYPE_FC_16G            = 0x00002000, // Vortex
    FBE_SLIC_TYPE_12G_SAS           = 0x00004000, // Snowdevil/Dustdevil
    FBE_SLIC_TYPE_V2_4PORT_ISCSI_1G = 0x00008000, // ThunderheadX
    FBE_SLIC_TYPE_V2_4PORT_ISCSI_10G =0x00010000, // Rockslide
    FBE_SLIC_TYPE_FC_8G_4S          = 0x01000000, // Glacier 4 Single Mode
    FBE_SLIC_TYPE_FC_8G_1S3M        = 0x02000000, // Glacier 1 Single Mode 3 Multi Mode
    FBE_SLIC_TYPE_V3_2PORT_ISCSI_10G =0x04000000, // MaelstromX
    FBE_SLIC_TYPE_V3_4PORT_ISCSI_10G =0x08000000, // DownburstX
    FBE_SLIC_TYPE_NA                = 0x40000000,       // Mezzanine, Back end Module or other non-SLICs
    FBE_SLIC_TYPE_UNSUPPORTED       = 0x80000000
} fbe_module_slic_type_t;


#define FBE_MODULE_MGMT_ALL_SUPPORTED_SLIC_TYPES \
       (FBE_SLIC_TYPE_FC_8G              |       \
        FBE_SLIC_TYPE_ISCSI_10G          |       \
        FBE_SLIC_TYPE_FCOE               |       \
        FBE_SLIC_TYPE_4PORT_ISCSI_1G     |       \
        FBE_SLIC_TYPE_6G_SAS_1           |       \
        FBE_SLIC_TYPE_ISCSI_COPPER       |       \
        FBE_SLIC_TYPE_ISCSI_10G_V2       |       \
        FBE_SLIC_TYPE_6G_SAS_3           |       \
        FBE_SLIC_TYPE_4PORT_ISCSI_10G    |       \
        FBE_SLIC_TYPE_V2_4PORT_ISCSI_1G  |       \
        FBE_SLIC_TYPE_V2_4PORT_ISCSI_10G |       \
        FBE_SLIC_TYPE_FC_16G             |       \
        FBE_SLIC_TYPE_FC_8G_4S          |        \
        FBE_SLIC_TYPE_FC_8G_1S3M        |        \
        FBE_SLIC_TYPE_V3_2PORT_ISCSI_10G |       \
        FBE_SLIC_TYPE_V3_4PORT_ISCSI_10G |       \
        FBE_SLIC_TYPE_NA)




// This is the protocol used by the SLIC.  In the case where
// there is more than one protocol, MUTLI is used.
typedef enum
{
    FBE_IO_MODULE_PROTOCOL_UNKNOWN  = 0x0,
    FBE_IO_MODULE_PROTOCOL_FIBRE    = 0x1,
    FBE_IO_MODULE_PROTOCOL_ISCSI    = 0x2,
    FBE_IO_MODULE_PROTOCOL_SAS      = 0x3, 
    FBE_IO_MODULE_PROTOCOL_FCOE     = 0x4,
    FBE_IO_MODULE_PROTOCOL_MULTI    = 0x5
} FBE_IO_MODULE_PROTOCOL;



// IO port structure used by ESP.
typedef struct fbe_esp_io_port_physical_info_t 
{
    SP_ID                          associatedSp;
    fbe_u32_t                      slotNumOnBlade;  
    fbe_u32_t                      portNumOnModule;  
    fbe_bool_t                     isLocalFru;  
    fbe_mgmt_status_t              present;
    fbe_power_status_t             powerStatus;
    IO_CONTROLLER_PROTOCOL         protocol;      
    fbe_mgmt_status_t              SFPcapable;
    fbe_u32_t                      supportedSpeeds;
    SPECL_PCI_DATA                 pciData; 
    fbe_u64_t              deviceType; 
    LED_BLINK_RATE                 ioPortLED;
    LED_COLOR_TYPE                 ioPortLEDColor;
    fbe_port_link_information_t    link_info;
    fbe_u32_t                      portal_number;
} fbe_esp_io_port_physical_info_t;



typedef struct fbe_esp_io_port_logical_info_s
{
    fbe_port_state_t            port_state;         /*!< PORT State */
    fbe_port_substate_t         port_substate;      /*!< PORT Sub State */
    fbe_u32_t                   logical_number;     /*!< PORT Logical Number */
    fbe_ioport_role_t           port_role;          /*!< PORT Role */
    fbe_port_subrole_t          port_subrole;       /*!< PORT SubRole */
    fbe_iom_group_t             iom_group;          /*!< PORT IO Module Group */
    fbe_bool_t                  is_combined_port;   /*!< PORT Is member of a combined port pair */
}fbe_esp_io_port_logical_info_t;

// SFP info structure used by ESP.
typedef struct fbe_esp_sfp_physical_info_s 
{
    fbe_bool_t               inserted;
    fbe_u32_t                capable_speeds;
    fbe_port_sfp_media_type_t     media_type;
    fbe_u32_t                cable_length;
    fbe_u32_t                hw_type;

    fbe_u8_t                 emc_part_number[FBE_PORT_SFP_EMC_DATA_LENGTH];
    fbe_u8_t                 emc_serial_number[FBE_PORT_SFP_EMC_DATA_LENGTH];

    fbe_u8_t                 vendor_part_number[FBE_PORT_SFP_VENDOR_DATA_LENGTH];
    fbe_u8_t                 vendor_serial_number[FBE_PORT_SFP_VENDOR_DATA_LENGTH];

    fbe_port_sfp_condition_type_t condition_type;
    fbe_port_sfp_sub_condition_type_t   sub_condition_type;
    fbe_u32_t                condition_additional_info;
    fbe_sfp_protocol_t       supported_protocols; /*!< Bitmask of supported protocols */
    
} fbe_esp_sfp_physical_info_t;

typedef enum 
{
    FBE_MODULE_MGMT_CONVERSION_NONE,
    FBE_MODULE_MGMT_CONVERSION_HCL_TO_SENTRY,
    FBE_MODULE_MGMT_CONVERSION_SENTRY_TO_ARGONAUT
}fbe_module_mgmt_conversion_type_t;

typedef enum 
{
    FBE_MODULE_MGMT_NO_SLIC_UPGRADE,
    FBE_MODULE_MGMT_SLIC_UPGRADE_LOCAL_SP,
    FBE_MODULE_MGMT_SLIC_UPGRADE_PEER_SP
}fbe_module_mgmt_upgrade_type_t;

typedef enum
{
    REBOOT_NONE = 0,
    REBOOT_LOCAL,
    REBOOT_PEER,
    REBOOT_BOTH
} fbe_module_mgmt_reboot_cmd_t;




/* Registry IO related definitions for module management */

#define FBE_MODULE_MGMT_PORT_PERSIST_KEY "ESPPersistPortConfig"
#define FBE_MODULE_MGMT_IN_FAMILY_CONVERSION_KEY "ESPSLICsInFamilyConversionType"
#define FBE_MODULE_MGMT_RESET_PORT_INTERRUPT_AFFINITY "ESPResetPortInterruptAffinity"
#define FBE_MODULE_MGMT_DISABLE_REG_UPDATE_KEY "ESPDisableRegUpdate"
#define FBE_MODULE_MGMT_PERSIST_DB_DISABLED_KEY "ESPPersistDatabaseDisabled"

/* String Generation Functionality( file fbe_module_mgmt_string_util.c ) */
const fbe_char_t * fbe_module_mgmt_slic_type_to_string(fbe_module_slic_type_t slic_type);
const fbe_char_t * fbe_module_mgmt_device_type_to_string(fbe_u64_t device_type);
const fbe_char_t * fbe_module_mgmt_module_state_to_string(fbe_module_state_t state);
const fbe_char_t * fbe_module_mgmt_module_substate_to_string(fbe_module_substate_t substate);
const fbe_char_t * fbe_module_mgmt_port_state_to_string(fbe_port_state_t state);
const fbe_char_t * fbe_module_mgmt_port_substate_to_string(fbe_port_substate_t substate);
const fbe_char_t * fbe_module_mgmt_mgmt_status_to_string(fbe_mgmt_status_t mgmt_status);
const fbe_char_t * fbe_module_mgmt_port_role_to_string(fbe_ioport_role_t port_role);
const fbe_char_t * fbe_module_mgmt_port_subrole_to_string(fbe_port_subrole_t port_subrole);
const fbe_char_t * fbe_module_mgmt_protocol_to_string(IO_CONTROLLER_PROTOCOL protocol);
const fbe_char_t * fbe_module_mgmt_power_status_to_string(fbe_power_status_t power_status);
const fbe_char_t * fbe_module_mgmt_supported_speed_to_string(fbe_u32_t supported_speed);
const fbe_char_t * fbe_module_mgmt_conversion_type_to_string(fbe_module_mgmt_conversion_type_t type);
const fbe_char_t * fbe_module_mgmt_convert_general_fault_to_string(fbe_mgmt_status_t  mgmtStatus);
const fbe_char_t * fbe_module_mgmt_convert_env_interface_status_to_string(fbe_env_inferface_status_t   envInterfaceStatus);
const fbe_char_t * fbe_module_mgmt_convert_mgmtPortAutoNeg_to_string(fbe_mgmt_port_auto_neg_t portAutoNeg);
const fbe_char_t * fbe_module_mgmt_convert_externalMgmtPortSpeed_to_string(fbe_mgmt_port_speed_t portSpeed);
const fbe_char_t * fbe_module_mgmt_convert_externalMgmtPortDuplexMode_to_string(fbe_mgmt_port_duplex_mode_t portDuplex);
const fbe_char_t * fbe_module_mgmt_convert_link_state_to_string(fbe_port_link_state_t link_state);
void fbe_module_mgmt_get_port_logical_number_string(fbe_u32_t logical_number, fbe_char_t *log_num_string);
fbe_status_t fbe_module_mgmt_get_esp_lun_location(fbe_char_t *location);

#endif /* FBE_MODULE_INFO_H */

/*******************************
 * end fbe_module_info.h
 *******************************/
