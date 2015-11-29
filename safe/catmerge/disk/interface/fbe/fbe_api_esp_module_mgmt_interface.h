#ifndef FBE_API_ESP_MODULE_MGMT_INTERFACE_H
#define FBE_API_ESP_MODULE_MGMT_INTERFACE_H
/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_api_esp_module_mgmt_interface.h
 ***************************************************************************
 *
 * @brief
 *  This header file defines the FBE API for the ESP MODULE Mgmt object.
 * 
 * @ingroup fbe_api_esp_interface_class_files
 * 
 * @version
 *   03/16/2010:  Created. bphilbin
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_types.h"
#include "fbe/fbe_transport.h"
#include "fbe/fbe_esp_module_mgmt.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_board.h"
#include "fbe/fbe_module_info.h"
#include "fbe/fbe_esp_base_environment.h"
#include "fbe/fbe_environment_limit.h"

FBE_API_CPP_EXPORT_START

//----------------------------------------------------------------
// Define the top level group for the FBE Storage Extent Package APIs
//----------------------------------------------------------------
/*! @defgroup fbe_api_esp_interface_class FBE ESP APIs
 *  @brief 
 *    This is the set of definitions for FBE ESP APIs.
 *
 *  @ingroup fbe_api_esp_interface
 */ 
//----------------------------------------------------------------

//----------------------------------------------------------------
// Define the FBE API ESP MODULE Mgmt Interface for the Usurper Interface. 
// This is where all the data structures defined. 
//----------------------------------------------------------------
/*! @defgroup fbe_api_esp_module_mgmt_interface_usurper_interface FBE API ESP MODULE Mgmt Interface Usurper Interface
 *  @brief 
 *    This is the set of definitions that comprise the FBE API ESP MODULE Mgmt Interface class
 *    usurper interface.
 *
 *  @ingroup fbe_api_classes_usurper_interface
 *  @{
 */ 
//----------------------------------------------------------------

/*!********************************************************************* 
 * @struct fbe_esp_module_mgmt_module_cmd_header_t 
 *  
 * @brief 
 *   Contains information specifying which module the command pertains to.
 *
 * @ingroup fbe_api_esp_module_mgmt_interface
 **********************************************************************/
typedef struct fbe_esp_module_mgmt_module_cmd_header_s
{
    fbe_u32_t                   sp;             /*!< SP*/
    fbe_u64_t           type;           /*!< MODULE device type */
    fbe_u32_t                   slot;           /*!< Slot Number */
} fbe_esp_module_mgmt_module_cmd_header_t;

/*!********************************************************************* 
 * @struct fbe_esp_module_mgmt_port_cmd_header_t 
 *  
 * @brief 
 *   Contains information specifying which module the command pertains to.
 *
 * @ingroup fbe_api_esp_module_mgmt_interface
 **********************************************************************/
typedef struct fbe_esp_module_mgmt_port_cmd_header_s
{
    fbe_u32_t                   sp;             /*!< SP*/
    fbe_u64_t           type;           /*!< MODULE device type */
    fbe_u32_t                   slot;           /*!< Slot Number */
    fbe_u32_t                   port;           /*!< Port Number (as labeled) */
} fbe_esp_module_mgmt_port_cmd_header_t;

// FBE_ESP_MODULE_MGMT_CONTROL_CODE_GET_GENERAL_STATUS

/*!********************************************************************* 
 * @struct fbe_esp_module_mgmt_general_status_t 
 *  
 * @brief 
 *   Contains the general status information from module_mgmt for this SP.
 *
 * @ingroup fbe_api_esp_module_mgmt_interface
 **********************************************************************/
typedef struct fbe_esp_module_mgmt_general_status_s
{
    fbe_bool_t                  port_configuration_loaded;  /*!< Port configuration loaded from persistent storage */
    fbe_module_mgmt_upgrade_type_t slic_upgrade_type; /*!<Current SLIC upgrade State */
    fbe_module_mgmt_reboot_cmd_t reboot_requested; /*!<Reboot requested and type */
} fbe_esp_module_mgmt_general_status_t;


// FBE_ESP_MODULE_MGMT_CONTROL_CODE_GET_MODULE_STATUS

/*!********************************************************************* 
 * @struct fbe_esp_module_mgmt_module_status_t 
 *  
 * @brief 
 *   Contains the logical data about an IO module.
 *
 * @ingroup fbe_api_esp_module_mgmt_interface
 * @ingroup fbe_esp_module_mgmt_moduleStatus
 **********************************************************************/
typedef struct fbe_esp_module_mgmt_module_status_s
{
    fbe_esp_module_mgmt_module_cmd_header_t header; /*!< MODULE Command Header */    
    fbe_module_state_t          state;              /*!< MODULE State */
    fbe_module_substate_t       substate;           /*!< MODULE Substate */
    fbe_module_slic_type_t      type;               /*!< MODULE SLIC Type */
    FBE_IO_MODULE_PROTOCOL      protocol;           /*!< MODULE Protocol */

} fbe_esp_module_mgmt_module_status_t;


// FBE_ESP_MODULE_MGMT_CONTROL_CODE_GET_PORT_STATUS

/*!********************************************************************* 
 * @struct fbe_esp_module_mgmt_port_status_t 
 *  
 * @brief 
 *   Contains the logical data about an IO port.
 *
 * @ingroup fbe_api_esp_module_mgmt_interface
 * @ingroup fbe_esp_module_mgmt_moduleStatus
 **********************************************************************/
typedef struct fbe_esp_module_mgmt_port_status_s
{
    fbe_esp_module_mgmt_port_cmd_header_t   header;     /*!< PORT Command Header */
    fbe_port_state_t                        state;      /*!< PORT State */
    fbe_port_substate_t                     substate;   /*!< PORT Substate */
   
} fbe_esp_module_mgmt_port_status_t;

// FBE_ESP_MODULE_MGMT_CONTROL_CODE_GET_IO_MODULE_INFO
// FBE_ESP_MODULE_MGMT_CONTROL_CODE_GET_IO_ANNEX_INFO
// FBE_ESP_MODULE_MGMT_CONTROL_CODE_GET_MEZZANINE_INFO

/*!********************************************************************* 
 * @struct fbe_esp_module_io_module_info_t 
 *  
 * @brief 
 *   Contains the physical data about an IO module.
 *
 * @ingroup fbe_api_esp_module_mgmt_interface
 * @ingroup fbe_esp_module_io_module_info_t
 **********************************************************************/
typedef struct fbe_esp_module_io_module_info_s
{
    fbe_esp_module_mgmt_module_cmd_header_t header;               /*!< MODULE Command Header */    
	    fbe_board_mgmt_io_comp_info_t       io_module_phy_info;   /*!< MODULE physical Info */
	    fbe_module_slic_type_t                  expected_slic_type;   /*!< MODULE expected slic type */
            fbe_char_t                          label_name[255];
} fbe_esp_module_io_module_info_t;

// FBE_ESP_MODULE_MGMT_CONTROL_CODE_GET_IO_PORT_INFO

/*!********************************************************************* 
 * @struct fbe_esp_module_io_port_info_t 
 *  
 * @brief 
 *   Contains the physical data about an IO port.
 *
 * @ingroup fbe_api_esp_module_mgmt_interface
 * @ingroup fbe_esp_module_io_module_info_t
 **********************************************************************/
typedef struct fbe_esp_module_io_port_info_s
{
    fbe_esp_module_mgmt_port_cmd_header_t header;              /*!< IO PORT Command Header */    
	fbe_esp_io_port_physical_info_t   io_port_info;              /*!< IO PORT Physical Info */
    fbe_esp_io_port_logical_info_t    io_port_logical_info;     /*!< IO PORT Logical Information */
} fbe_esp_module_io_port_info_t;


// FBE_ESP_MODULE_MGMT_CONTROL_CODE_GET_SFP_INFO

/*!********************************************************************* 
 * @struct fbe_esp_module_sfp_info_t 
 *  
 * @brief 
 *   Contains the physical data about an SFP port.
 *
 * @ingroup fbe_api_esp_module_mgmt_interface
 * @ingroup fbe_esp_module_io_module_info_t
 **********************************************************************/
typedef struct fbe_esp_module_sfp_info_s
{
    fbe_esp_module_mgmt_port_cmd_header_t header;              /*!< IO PORT Command Header */    
	fbe_esp_sfp_physical_info_t           sfp_info;            /*!< SFP Info */
} fbe_esp_module_sfp_info_t;

// FBE_ESP_MODULE_MGMT_CONTROL_CODE_GET_LIMITS_INFO

/*!********************************************************************* 
 * @struct fbe_esp_module_limits_info_t 
 *  
 * @brief 
 *   Contains the platform limits information pertainint to modules.
 *   These are all listed on a per SP basis.
 *
 * @ingroup fbe_api_esp_module_mgmt_interface
 * @ingroup fbe_esp_module_io_module_info_t
 **********************************************************************/
typedef struct fbe_esp_module_limits_info_s
{
    fbe_environment_limits_platform_hardware_limits_t      platform_hw_limit;
    fbe_esp_module_mgmt_discovered_hardware_limits_t    discovered_hw_limit;
    fbe_environment_limits_platform_port_limits_t          platform_port_limit;
} fbe_esp_module_limits_info_t;

// FBE_ESP_MODULE_MGMT_CONTROL_CODE_SET_PORT_CONFIG

typedef enum
{
    FBE_ESP_MODULE_MGMT_PERSIST_ALL_PORTS_CONFIG,                /*!< Configure all detected ports */
    FBE_ESP_MODULE_MGMT_PERSIST_PORT_CONFIG_LIST,                /*!< Configure the specified port list (no overwrite) */
    FBE_ESP_MODULE_MGMT_PERSIST_PORT_CONFIG_LIST_WITH_OVERWRITE, /*!< Configure the specified port list (with overwrite) */
    FBE_ESP_MODULE_MGMT_REMOVE_ALL_PORTS_CONFIG,                 /*!< Destroy the entire port configuration */
    FBE_ESP_MODULE_MGMT_REMOVE_PORT_CONFIG_LIST,                 /*!< Remove the specified port configuration */
    FBE_ESP_MODULE_MGMT_UPGRADE_PORTS,                           /*!< Upgrade SLICs if possible */
    FBE_ESP_MODULE_MGMT_REPLACE_PORTS                            /*!< Reboot and hold in reset to replace SLICs */
}fbe_esp_module_mgmt_set_port_opcode_t;

typedef struct fbe_esp_module_mgmt_port_config_s
{
    fbe_esp_module_mgmt_port_cmd_header_t   port_location;
    fbe_u32_t                               logical_number; /*!< PORT Logical Number */
    fbe_ioport_role_t                       port_role;      /*!< PORT Role */
    fbe_port_subrole_t                      port_subrole;   /*!< PORT SubRole */
    fbe_iom_group_t                         iom_group;      /*!< PORT IO Module Group */
}fbe_esp_module_mgmt_port_config_t;


/*!********************************************************************* 
 * @struct fbe_esp_module_mgmt_set_port_t 
 *  
 * @brief 
 *   These are the commands to persist, upgrade, or remove a port
 *   configuration.
 *
 * @ingroup fbe_api_esp_module_mgmt_interface
 **********************************************************************/
typedef struct fbe_esp_module_mgmt_set_port_s
{
    fbe_esp_module_mgmt_set_port_opcode_t   opcode;
    fbe_u32_t                               num_ports;
    fbe_esp_module_mgmt_port_config_t       port_config[FBE_ESP_IO_PORT_MAX_COUNT];
} fbe_esp_module_mgmt_set_port_t;

typedef enum
{
    FBE_MODULE_MGMT_CLASS_SLIC,
    FBE_MODULE_MGMT_CLASS_MEZZANINE,
    FBE_MODULE_MGMT_CLASS_MAX,
} fbe_esp_module_mgmt_iom_class_t;

/*!********************************************************************* 
 * @struct fbe_esp_module_mgmt_mark_module_t 
 *  
 * @brief 
 *   These are the commands to Mark (blink LED) a IO Module.
 *
 * @ingroup fbe_api_esp_module_mgmt_interface
 **********************************************************************/
typedef struct fbe_esp_module_mgmt_mark_module_s
{
    fbe_esp_module_mgmt_module_cmd_header_t iomHeader;
//    fbe_esp_module_mgmt_iom_class_t     iomClass;
//    fbe_u32_t                           ioModuleNum;
    fbe_bool_t                          markPortOn;
} fbe_esp_module_mgmt_mark_module_t;

/*!********************************************************************* 
 * @struct fbe_esp_module_mgmt_mark_port_t 
 *  
 * @brief 
 *   These are the commands to Mark (blink LED) a port.
 *
 * @ingroup fbe_api_esp_module_mgmt_interface
 **********************************************************************/
typedef struct fbe_esp_module_mgmt_mark_port_s
{
    fbe_esp_module_mgmt_port_cmd_header_t iomHeader;
//    fbe_esp_module_mgmt_iom_class_t     iomClass;
//    fbe_u32_t                           ioModuleNum;
//    fbe_u32_t                           ioPortNum;
    fbe_bool_t                          markPortOn;
} fbe_esp_module_mgmt_mark_port_t;

// FBE_ESP_MODULE_MGMT_CONTROL_CODE_GET_PORT_AFFINITY
/*!*********************************************************************
 * @struct fbe_esp_module_mgmt_port_affinity_t
 *
 * @brief
 *   Contains the configured port interrupt affinity settings.
 *
 * @ingroup fbe_api_esp_module_mgmt_interface
 **********************************************************************/
typedef struct fbe_esp_module_mgmt_port_affinity_s
{
    fbe_esp_module_mgmt_port_cmd_header_t   header;               /*!< MODULE Command Header */
    SPECL_PCI_DATA                          pciData;              /*!< PORT PCI Address */
    fbe_u32_t                               processor_core;       /*!< PORT Affinity */
} fbe_esp_module_mgmt_port_affinity_t;


/*!********************************************************************* 
 * @struct fbe_esp_module_mgmt_get_mgmt_comp_info_s 
 *  
 * @brief 
 *   This struct used to get the mgmt comp info
 *   Use for FBE_ESP_MODULE_MGMT_CONTROL_CODE_GET_MGMT_COMP_INFO
 *
 * @ingroup fbe_api_esp_module_mgmt_interface
 **********************************************************************/
typedef struct fbe_esp_module_mgmt_get_mgmt_comp_info_s
{
    fbe_device_physical_location_t      phys_location;          /*!< IN */
    fbe_board_mgmt_mgmt_module_info_t   mgmt_module_comp_info;  /*!< OUT */
}fbe_esp_module_mgmt_get_mgmt_comp_info_t;

/*!********************************************************************* 
 * @struct fbe_esp_module_mgmt_set_mgmt_port_config_s 
 *  
 * @brief 
 *   This struct used to configure the mgmt port speed
 *   Use for FBE_ESP_MODULE_MGMT_CONTROL_CODE_SET_MGMT_PORT_CONFIG
 *
 * @ingroup fbe_api_esp_module_mgmt_interface
 **********************************************************************/
typedef struct fbe_esp_module_mgmt_set_mgmt_port_config_s
{
    fbe_device_physical_location_t      phys_location;                  /*!< IN */
    fbe_bool_t                          revert;                         /*!< IN */
    fbe_mgmt_port_config_info_t         mgmtPortRequest;                /*!< IN */
}fbe_esp_module_mgmt_set_mgmt_port_config_t;

/*!********************************************************************* 
 * @struct fbe_esp_module_mgmt_get_mgmt_port_config_s 
 *  
 * @brief 
 *   This struct used to get the config mgmt port info
 *   Use for FBE_ESP_MODULE_MGMT_CONTROL_CODE_GET_REQUESTED_MGMT_PORT_CONFIG
 *
 * @ingroup fbe_api_esp_module_mgmt_interface
 **********************************************************************/
typedef struct fbe_esp_module_mgmt_get_mgmt_port_config_s
{
    fbe_device_physical_location_t       phys_location;                   /*!< IN */
    fbe_mgmt_port_config_info_t          mgmtPortConfig;                  /*!< OUT */
}fbe_esp_module_mgmt_get_mgmt_port_config_t;

/*! @} */ /* end of group fbe_api_esp_module_mgmt_interface_usurper_interface */

//----------------------------------------------------------------
// Define the group for the FBE API ESP MODULE Mgmt Interface.  
// This is where all the function prototypes for the FBE API ESP MODULE Mgmt.
//----------------------------------------------------------------
/*! @defgroup fbe_api_esp_module_mgmt_interface FBE API ESP MODULE Mgmt Interface
 *  @brief 
 *    This is the set of FBE API ESP MODULE Mgmt Interface. 
 *
 *  @details 
 *    In order to access this library, please include fbe_api_esp_module_mgmt_interface.h.
 *
 *  @ingroup fbe_api_esp_interface_class
 *  @{
 */
//----------------------------------------------------------------

fbe_status_t FBE_API_CALL 
fbe_api_esp_module_mgmt_general_status(fbe_esp_module_mgmt_general_status_t  *general_status);
fbe_status_t FBE_API_CALL 
fbe_api_esp_module_mgmt_get_module_status(fbe_esp_module_mgmt_module_status_t *module_status_info);
fbe_status_t FBE_API_CALL 
fbe_api_esp_module_mgmt_getIOModuleInfo(fbe_esp_module_io_module_info_t *io_module_info);
fbe_status_t FBE_API_CALL 
fbe_api_esp_module_mgmt_getIOModulePortInfo(fbe_esp_module_io_port_info_t *io_port_info);
fbe_status_t FBE_API_CALL 
fbe_api_esp_module_mgmt_get_sfp_info(fbe_esp_module_sfp_info_t *sfp_info);
fbe_status_t FBE_API_CALL 
fbe_api_esp_module_mgmt_getMezzanineInfo(fbe_esp_module_io_module_info_t *mezzanine_info);
fbe_status_t FBE_API_CALL 
fbe_api_esp_module_mgmt_getBemInfo(fbe_esp_module_io_module_info_t *bem_info);
fbe_status_t FBE_API_CALL 
fbe_api_esp_module_mgmt_get_limits_info(fbe_esp_module_limits_info_t *limits_info);
fbe_status_t FBE_API_CALL 
fbe_api_esp_module_mgmt_set_port_config(fbe_esp_module_mgmt_set_port_t *set_port_cmd);
fbe_status_t FBE_API_CALL 
fbe_api_esp_module_mgmt_set_mgmt_port_config(fbe_esp_module_mgmt_set_mgmt_port_config_t *mgmt_port_config);
fbe_status_t FBE_API_CALL 
fbe_api_esp_module_mgmt_get_mgmt_comp_info(fbe_esp_module_mgmt_get_mgmt_comp_info_t *mgmt_comp_info);
fbe_status_t FBE_API_CALL 
fbe_api_esp_module_mgmt_get_config_mgmt_port_info(fbe_esp_module_mgmt_get_mgmt_port_config_t *mgmt_port_config);
fbe_status_t FBE_API_CALL 
fbe_api_esp_module_mgmt_markIoPort(fbe_esp_module_mgmt_mark_port_t *markPortCmd);
fbe_status_t FBE_API_CALL 
fbe_api_esp_module_mgmt_markIoModule(fbe_esp_module_mgmt_mark_module_t *markModuleCmd);
fbe_status_t FBE_API_CALL
fbe_api_esp_module_mgmt_get_port_affinity(fbe_esp_module_mgmt_port_affinity_t *port_affinity);

/*! @} */ /* end of group fbe_api_esp_module_mgmt_interface */

FBE_API_CPP_EXPORT_END

//----------------------------------------------------------------
// Define the group for all FBE ESP Package APIs Interface class files.  
// This is where all the class files that belong to the FBE API ESP 
// Package define. In addition, this group is displayed in the FBE Classes
// module.
//----------------------------------------------------------------
/*! @defgroup fbe_api_esp_interface_class_files FBE ESP APIs Interface Class Files 
 *  @brief 
 *    This is the set of files for the FBE ESP Interface.
 *
 *  @ingroup fbe_api_classes
 */
//----------------------------------------------------------------


#endif /* FBE_API_ESP_MODULE_MGMT_INTERFACE_H */

/*************************
 * end file fbe_api_esp_module_mgmt_interface.h
 *************************/
