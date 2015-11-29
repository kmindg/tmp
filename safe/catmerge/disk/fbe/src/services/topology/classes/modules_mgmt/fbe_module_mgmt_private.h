/***************************************************************************
 * Copyright (C) EMC Corporation 2007-2010 
 * All rights reserved. Licensed material -- property of EMC 
 * Corporation 
 ***************************************************************************/

#ifndef MODULE_MGMT_PRIVATE_H
#define MODULE_MGMT_PRIVATE_H

/*!**************************************************************************
 * @file fbe_module_mgmt_private.h
 ***************************************************************************
 *
 * @brief
 *  This file contains private defines for the Modules
 *  Management object. 
 * 
 * @ingroup module_mgmt_class_files
 * 
 * @revision
 *   03/11/2010: Created. Nayana Chaudhari
 *
 ***************************************************************************/

#include "fbe_base_object.h"
#include "base_object_private.h"
#include "fbe_base_environment_private.h"
#include "fbe/fbe_module_info.h"
#include "fbe/fbe_pe_types.h"
#include "fbe/fbe_environment_limit.h"

#define USE_PERSISTENT_SERVICE 1

#define FBE_ESP_MAX_CONFIGURED_PORTS_PER_SP 44
#define FBE_MODULE_MGMT_PERSISTENT_DATA_STORE_SIZE \
 (sizeof(fbe_io_port_persistent_info_t) * FBE_ESP_MAX_CONFIGURED_PORTS_PER_SP + \
  sizeof(fbe_mgmt_port_config_info_t) * FBE_ESP_MGMT_MODULE_MAX_COUNT_PER_SP)

#define FBE_IO_PORT_PERSISTENT_DATA_SIZE \
  (sizeof(fbe_io_port_persistent_info_t) * FBE_ESP_MAX_CONFIGURED_PORTS_PER_SP)

#define FBE_MGMT_PORT_PERSISTENT_DATA_OFFSET  FBE_IO_PORT_PERSISTENT_DATA_SIZE


#define FBE_ESP_MGMT_MODULE_MAX_COUNT_PER_SP  1
#define FBE_ESP_BEM_MAX_COUNT                 1
#define FBE_ESP_SLIC_IOM_OFFSET             FBE_ESP_MEZZANINE_MAX_COUNT

#define FBE_MODULE_MGMT_TLA_FULL_LENGTH (RESUME_PROM_EMC_TLA_PART_NUM_SIZE+1)

#define FBE_ESP_MGMT_PORT_CONFIG_TIMEOUT    45          // 45 Sec.


#define FBE_ESP_COMBINED_PORT_3M_SFP_PART_NUMBER  "038-004-094     "
#define FBE_ESP_COMBINED_PORT_4M_SFP_PART_NUMBER  "038-004-095     "
#define FBE_ESP_COMBINED_PORT_5M_SFP_PART_NUMBER  "038-004-096     "




#define FBE_MOD_MGMT_REG_MAX_STRING_LENGTH 255
#define MAX_PORT_REG_PARAMS 88 // Number of PortParams entries to parse in the registry

#define FBE_MODULE_MGMT_AFFINITY_SETTING_MODE_DISABLED 0
#define FBE_MODULE_MGMT_AFFINITY_SETTING_MODE_DEFAULT 1
#define FBE_MODULE_MGMT_AFFINITY_SETTING_MODE_DYNAMIC 2

/* FUP related macros */
#define FBE_MODULE_MGMT_JDES_FUP_IMAGE_PATH_KEY   "JDESLccImagePath"
#define FBE_MODULE_MGMT_JDES_FUP_IMAGE_FILE_NAME  "JDES_Bundled_FW"

#define FBE_MODULE_MGMT_MAX_PROGRAMMABLE_COUNT_PER_BEM_SLOT  1

#define FBE_MODULE_MGMT_CDES2_FUP_IMAGE_PATH_KEY             "CDES2LccImagePath"
#define FBE_MODULE_MGMT_CDES2_FUP_CDEF_IMAGE_FILE_NAME       "cdef"
#define FBE_MODULE_MGMT_CDES2_FUP_CDEF_DUAL_IMAGE_FILE_NAME  "cdef_dual"
#define FBE_MODULE_MGMT_CDES2_FUP_ISTR_IMAGE_FILE_NAME       "istr"
#define FBE_MODULE_MGMT_CDES2_FUP_CDES_ROM_IMAGE_FILE_NAME   "cdes_rom"

#define FBE_MODULE_MGMT_CDES2_FUP_MANIFEST_FILE_PATH_KEY  "CDES2LccImagePath"
#define FBE_MODULE_MGMT_CDES2_FUP_MANIFEST_FILE_NAME  "manifest_lcc"

// BRION - HACK delay 20 seconds after writing the file to give the OS a chance to flush this to disk.
//  These times were derived from original code in fbe_module_mgmt_handle_file_write_completion.
#ifndef ALAMOSA_WINDOWS_ENV
#define FBE_MODULE_MGMT_FLUSH_THREAD_DELAY_IN_MSECS  2000  // 2 seconds.
#else
#define FBE_MODULE_MGMT_FLUSH_THREAD_DELAY_IN_MSECS  20000 // 20 seconds.
#endif /* ALAMOSA_WINDOWS_ENV - ODDCASE - PCPC */

#define FBE_MODULE_MGMT_INVALID_CORE_NUM 0xff


/* Lifecycle definitions
 * Define the Modules management lifecycle's instance.
 */
extern fbe_lifecycle_const_t FBE_LIFECYCLE_CONST_DATA(module_mgmt);

/* These are the lifecycle condition ids for Module
   Management object.*/

/*! @enum fbe_module_mgmt_lifecycle_cond_id_t  
 *  
 *  @brief These are the lifecycle conditions for the module
 *  management object.
*/
typedef enum fbe_module_mgmt_lifecycle_cond_id_e 
{
    /*! Processing of new modules
     */
    FBE_MODULE_MGMT_DISCOVER_MODULE = FBE_LIFECYCLE_COND_FIRST_ID(FBE_CLASS_ID_MODULE_MGMT),
    FBE_MODULE_MGMT_CONFIGURE_PORTS,
    FBE_MODULE_MGMT_LIFECYCLE_COND_IN_FAMILY_PORT_CONVERSION,
    FBE_MODULE_MGMT_LIFECYCLE_COND_SLIC_UPGRADE,
    FBE_MODULE_MGMT_LIFECYCLE_COND_CHECK_REGISTRY,
    FBE_MODULE_MGMT_LIFECYCLE_COND_CONFIG_VLAN,
    FBE_MODULE_MGMT_LIFECYCLE_COND_SET_MGMT_PORT_SPEED,
    FBE_MODULE_MGMT_LIFECYCLE_COND_INIT_MGMT_PORT_CONFIG_REQUEST,

    FBE_MODULE_MGMT_LIFECYCLE_COND_LAST /* must be last */
} 
fbe_module_mgmt_lifecycle_cond_id_t;

typedef struct fbe_module_mgmt_cmi_msg_s
{
    fbe_base_environment_cmi_message_t  base_environment_message;
}fbe_module_mgmt_cmi_msg_t;

/*! @enum fbe_module_mgmt_port_config_request_source_e  
 *  
 *  @brief TThis enumerates the mgmt port speed config request source
*/
typedef enum fbe_mgmt_port_config_request_source_e
{
    FBE_REQUEST_NO_SOURCE,            // When no request initiated.
    FBE_REQUEST_SOURCE_INTERNAL,      // Internal request.
    FBE_REQUEST_SOURCE_USER           // Navi/FCLI.
}fbe_mgmt_port_config_request_source_t;


typedef enum fbe_module_mgmt_cmi_msg_code_e
{
    FBE_MODULE_MGMT_PORTS_CONFIGURED_MSG = 1,
    FBE_MODULE_MGMT_SLIC_UPGRADE_MSG = 2
}fbe_module_mgmt_cmi_msg_code_t;

typedef struct
{
    fbe_char_t      device_identity_string[FBE_MOD_MGMT_REG_MAX_STRING_LENGTH];
    fbe_char_t      device_instance_string[FBE_MOD_MGMT_REG_MAX_STRING_LENGTH];
    SPECL_PCI_DATA  pci_data;
    fbe_u32_t       processor_core;
    fbe_bool_t      present_in_system;
    fbe_bool_t      modify_needed;
}fbe_module_mgmt_configured_interrupt_data_t;

typedef struct
{
    fbe_char_t device_identity_string[FBE_MOD_MGMT_REG_MAX_STRING_LENGTH];
    fbe_char_t device_instance_string[FBE_MOD_MGMT_REG_MAX_STRING_LENGTH];
    fbe_bool_t has_match_in_registry_list;
}fbe_module_mgmt_miniport_instance_string_t;



typedef struct fbe_module_mgmt_port_reg_info_s
{
	SPECL_PCI_DATA          pci_data;            /*!< PORT PCI location */
    fbe_u32_t               logical_port_number; /*!< Logical # assigned for this port */
    fbe_iom_group_t         iom_group;           /*!< PORT iom group */
	fbe_ioport_role_t       port_role;           /*!< PORT role */
	fbe_port_subrole_t      port_subrole;        /*!< PORT sub role */
    fbe_u32_t               core_num;            /*!< PORT core affinity assignment */
    fbe_u32_t               port_count;          /*!< Number of ports on the IO Module */
    fbe_u32_t               phy_mapping;         /*!< PORT Phy mapping */
} fbe_module_mgmt_port_reg_info_t;


/*!****************************************************************************
 *    
 * @struct fbe_io_module_physical_info_s
 *  
 * @brief 
 *   This is the definition of the io module info. This structure
 *   stores physical information about the io module.
 ******************************************************************************/
typedef struct fbe_io_module_physical_info_s 
{
    fbe_board_mgmt_io_comp_info_t   module_comp_info;       /*!< MODULE component Info */
    fbe_u64_t               type;                   /*!< MODULE device type */
    fbe_base_env_resume_prom_info_t   module_resume_info;     /*!< MODULE resume prom info */
    fbe_base_env_fup_persistent_info_t  module_fup_info;
    FBE_IO_MODULE_PROTOCOL          protocol;
    fbe_char_t                      label_name[255];
}fbe_io_module_physical_info_t;

/*!****************************************************************************
 *    
 * @struct fbe_module_logical_info_s
 *  
 * @brief 
 *   This is the definition of the module info. This structure
 *   stores logical information about the module.
 ******************************************************************************/
typedef struct fbe_module_logical_info_s 
{
    fbe_module_state_t          module_state;           /*!< MODULE State */
    fbe_module_substate_t       module_substate;        /*!< MODULE Substate */
    fbe_module_slic_type_t      slic_type;              /*!< MODULE SLIC type */
} fbe_module_logical_info_t;

/*!****************************************************************************
 *    
 * @struct fbe_io_module_info_s
 *  
 * @brief 
 *   This is the definition of the io module info. This structure
 *   stores information about the io module
 ******************************************************************************/
typedef struct fbe_io_module_info_s 
{
    fbe_io_module_physical_info_t   physical_info[SP_ID_MAX];          /*!< MODULE Physical Info */
    fbe_module_logical_info_t       logical_info[SP_ID_MAX];           /*!< MODULE Logical Info */
}fbe_io_module_info_t;


/*!****************************************************************************
 *    
 * @struct fbe_io_port_location_s
 *  
 * @brief 
 *   This is the definition of the port location information.  This is the
 *   structure that is saved in the persistent storage that relates a
 *   logical configuration (FE/BE #) to a physical location
 ******************************************************************************/
typedef struct fbe_io_port_location_s
{
    fbe_u8_t                io_enclosure_number;
    fbe_u64_t               type;    /*!< MODULE device type */
    fbe_u8_t                slot;
    fbe_u8_t                port;
}fbe_io_port_location_t;

/*!****************************************************************************
 *    
 * @struct fbe_persistent_port_location_s
 *  
 * @brief 
 *   This is the definition of the port location information.  This is the
 *   structure that is saved in the persistent storage that relates a
 *   logical configuration (FE/BE #) to a physical location
 ******************************************************************************/
typedef struct fbe_persistent_port_location_s
{
    fbe_u8_t                io_enclosure_number;
    fbe_u32_t               type_32_bit;    /*!< MODULE device type */
    fbe_u8_t                slot;
    fbe_u8_t                port;
}fbe_persistent_port_location_t;


/*!****************************************************************************
 *    
 * @struct fbe_io_port_persistent_info_s
 *  
 * @brief 
 *   This is the definition of persitent porrt information.  This is configured
 *   data about a port that can be stored in persistent storage and reloaded.
 ******************************************************************************/
typedef struct fbe_io_port_persistent_info_s
{
    fbe_u8_t                  logical_num;        /*!< Logical # assigned for this port */
    fbe_iom_group_t           iom_group;          /*!< PORT iom group */
    fbe_ioport_role_t         port_role;          /*!< PORT role */
    fbe_port_subrole_t        port_subrole;       /*!< PORT sub role */
    fbe_persistent_port_location_t    port_location;      /*!< PORT locatoin */
}fbe_io_port_persistent_info_t;

/*!****************************************************************************
 *    
 * @struct fbe_io_port_logical_info_s
 *  
 * @brief 
 *   This is the definition of the io port info. This structure
 *   stores logical information about an io port
 ******************************************************************************/
typedef struct fbe_io_port_logical_info_s 
{

    fbe_port_state_t          port_state;         /*!< PORT State */
    fbe_port_substate_t       port_substate;      /*!< PORT Sub State */
    fbe_io_port_persistent_info_t   port_configured_info; /*!< PORT Persistent Configured Data */
    fbe_bool_t                combined_port;      /*!< PORT is a combined port */
    fbe_u32_t                 secondary_combined_port; /*!< PORT number of the other member of combined port pairing */
}fbe_io_port_logical_info_t;

/*!****************************************************************************
 *    
 * @struct fbe_io_port_physical_info_s
 *  
 * @brief 
 *   This is the definition of the io port info. This structure
 *   stores physical information about an io port
 ******************************************************************************/
typedef struct fbe_io_port_physical_info_s 
{
    fbe_board_mgmt_io_port_info_t    port_comp_info[SP_ID_MAX];      /*!< PORT component Info */
    fbe_port_hardware_info_t         port_hardware_info;             /*!< PORT Hardware Info (local only) */
    SPECL_PCI_DATA                   selected_pci_function;          /*!< PORT Current Active PCI Funciton */
    fbe_port_sfp_info_t              sfp_info;                       /*!< PORT SFP Info (local only) */
    fbe_bool_t                       sfp_inserted;                   /*!< PORT SFP insertedness (local only) */
    fbe_port_link_information_t      link_info;                      /*!< PORT link information (local only) */
}fbe_io_port_physical_info_t;

/*!****************************************************************************
 *    
 * @struct fbe_io_port_info_s
 *  
 * @brief 
 *   This is the definition of the io port info. This structure
 *   stores information about the io port
 ******************************************************************************/
typedef struct fbe_io_port_info_s 
{
    fbe_io_port_physical_info_t   port_physical_info;                     /*!< PORT Physical Info */
    fbe_io_port_logical_info_t    port_logical_info[SP_ID_MAX];           /*!< PORT Logical Info */
    fbe_object_id_t               port_object_id;                         /*!< PORT object id (local only) */

}fbe_io_port_info_t;

/*!****************************************************************************
 *    
 * @struct fbe_mgmt_port_config_op_s
 *  
 * @brief 
 *   This is the definition of the management module mgmt port config operation.
 *   This structure is used to configure mgmt port.
 ******************************************************************************/
typedef struct fbe_mgmt_port_config_op_s
{
    fbe_u32_t       mgmtId;
    /* The following values are from the Navi/fbe_cli user's management port configuration request.*/
    fbe_mgmt_port_auto_neg_t    mgmtPortAutoNegRequested;
    fbe_mgmt_port_speed_t       mgmtPortSpeedRequested;
    fbe_mgmt_port_duplex_mode_t mgmtPortDuplexModeRequested;

    /* This variable set to initialize the mgmt port config process.for given Mgmt module
     * Once it initialized reset this variable.
     */
    fbe_bool_t      sendMgmtPortConfig;

    /* The following value indicates if there is a management port configuration in progress.*/
    fbe_bool_t      mgmtPortConfigInProgress;  /*This remain set during throughout process of mgmt port configuration */
    fbe_time_t      configRequestTimeStamp;

    /* The following value indicates if the management port is reverting to the last known working config.*/
    fbe_bool_t      revertEnabled;   
    fbe_bool_t      revertMgmtPortConfig;
    
    /* In case of revert operation use this values to revert it*/
    fbe_mgmt_port_auto_neg_t    previousMgmtPortAutoNeg;
    fbe_mgmt_port_speed_t       previousMgmtPortSpeed;
    fbe_mgmt_port_duplex_mode_t previousMgmtPortDuplexMode;
}fbe_mgmt_port_config_op_t;

/*!****************************************************************************
 *    
 * @struct fbe_mgmt_module_info_s
 *  
 * @brief 
 *   This is the definition of the management module info. This structure
 *   stores information about the mgmt module
 ******************************************************************************/
typedef struct fbe_mgmt_module_info_s 
{
    fbe_board_mgmt_mgmt_module_info_t   mgmt_module_comp_info[SP_ID_MAX];       /*!< Mgmt module component Info */
    fbe_base_env_resume_prom_info_t     mgmt_module_resume_info[SP_ID_MAX];     /*!< Mgmt module resume prom info */
    fbe_mgmt_port_config_info_t         mgmt_port_persistent_info;              /*!< Use during persistent data read write */
    fbe_mgmt_port_config_info_t         user_requested_mgmt_port_config;             /*!< Save the user requested mgmt port config info */
    fbe_mgmt_port_config_op_t           mgmt_port_config_op;                  /*!< Mgmt port speed config info for mgmtID*/    
}fbe_mgmt_module_info_t;



/*!****************************************************************************
 *    
 * @struct fbe_module_mgmt_s
 *  
 * @brief 
 *   This is the definition of the module mgmt object. This object
 *   deals with handling module related functions
 ******************************************************************************/
typedef struct fbe_module_mgmt_s 
{
    /*! This must be first.  This is the base object we inherit from.
     */
    fbe_base_environment_t base_environment;
    // holds io module and mezzanine info only
	fbe_io_module_info_t   *io_module_info; //[FBE_ESP_MODULE_MAX_COUNT];
    //
    fbe_io_port_info_t     *io_port_info; //[FBE_ESP_IO_PORT_MAX_COUNT];
    fbe_mgmt_module_info_t *mgmt_module_info; //[FBE_ESP_MGMT_MODULE_MAX_COUNT_PER_SP];

    fbe_u32_t total_module_count;
    fbe_lifecycle_state_t       state;

    fbe_object_id_t              board_object_id;
    SP_ID                        local_sp;
    fbe_module_mgmt_reboot_cmd_t reboot_sp;

    fbe_environment_limits_platform_hardware_limits_t      platform_hw_limit;
    fbe_esp_module_mgmt_discovered_hardware_limits_t    discovered_hw_limit;
    fbe_environment_limits_platform_port_limits_t          platform_port_limit;
    
    fbe_bool_t                      port_persist_enabled;
    fbe_module_mgmt_upgrade_type_t  slic_upgrade;
    fbe_module_mgmt_conversion_type_t conversion_type;
    fbe_bool_t                      discovering_hardware;
    fbe_bool_t                      loading_config;
    fbe_u32_t                       reg_port_param_count;
    fbe_module_mgmt_port_reg_info_t *iom_port_reg_info;
    fbe_u32_t                       port_affinity_mode;
    fbe_bool_t                      boot_device_found;
    fbe_bool_t                      configuration_change_made;
    fbe_bool_t                      port_configuration_loaded;
    /* 
     * This is a table of the configured interrupt affinity settings for all the miniports.  It is 
     * persisted in the Registry. 
     * This array is sized to double the number of ports that can be configured to support a wost case 
     * where all the hardware has been swapped with new hardware. 
     */
    fbe_module_mgmt_configured_interrupt_data_t *configured_interrupt_data;
    
    /* This structure would have the info about all modules 
    applicable to a platform.E.g. management module, io modules,
    IO annex */

    /* Lifecycle defines. */
    FBE_LIFECYCLE_DEF_INST_DATA;
    FBE_LIFECYCLE_DEF_INST_COND(FBE_LIFECYCLE_COND_MAX(FBE_MODULE_MGMT_LIFECYCLE_COND_LAST));
} fbe_module_mgmt_t;


fbe_status_t fbe_module_mgmt_create_object(fbe_packet_t * packet, fbe_object_handle_t * object_handle);
fbe_status_t fbe_module_mgmt_destroy_object( fbe_object_handle_t object_handle);
fbe_status_t fbe_module_mgmt_control_entry(fbe_object_handle_t object_handle, fbe_packet_t * packet);
fbe_status_t fbe_module_mgmt_monitor_entry(fbe_object_handle_t object_handle, fbe_packet_t * packet);
fbe_status_t fbe_module_mgmt_event_entry(fbe_object_handle_t object_handle, fbe_event_type_t event_type, fbe_event_context_t event_context);

fbe_status_t fbe_module_mgmt_monitor_load_verify(void);

fbe_status_t fbe_module_mgmt_init(fbe_module_mgmt_t * module_mgmt);
void fbe_module_mgmt_init_mgmt_port_config_op(fbe_mgmt_port_config_op_t  *mgmt_port_config_op);
fbe_status_t fbe_module_mgmt_convert_mgmt_port_config_request(fbe_module_mgmt_t  *module_mgmt,
                                                                      fbe_u32_t  mgmt_id,
                                                                      fbe_mgmt_port_config_request_source_t reqSource,
                                                                      fbe_mgmt_port_config_info_t  *mgmt_port_config_info);

/* Utility Function Definitions */
fbe_u8_t fbe_module_mgmt_get_io_port_index(fbe_u8_t slot, fbe_u8_t port);
void fbe_module_mgmt_set_module_state(fbe_module_mgmt_t *module_mgmt, SP_ID sp_id, fbe_u8_t iom_num, fbe_module_state_t module_state);
fbe_module_state_t fbe_module_mgmt_get_module_state(fbe_module_mgmt_t *module_mgmt, SP_ID sp_id, fbe_u8_t iom_num);
void fbe_module_mgmt_set_module_substate(fbe_module_mgmt_t *module_mgmt, SP_ID sp_id, fbe_u8_t iom_num, fbe_module_substate_t module_substate);
fbe_module_substate_t fbe_module_mgmt_get_module_substate(fbe_module_mgmt_t *module_mgmt, SP_ID sp_id, fbe_u8_t iom_num);
void fbe_module_mgmt_set_port_state(fbe_module_mgmt_t *module_mgmt, SP_ID sp_id, fbe_u8_t iom_num, fbe_u8_t port_num, fbe_port_state_t port_state);
void fbe_module_mgmt_set_port_substate(fbe_module_mgmt_t *module_mgmt, SP_ID sp_id, fbe_u8_t iom_num, fbe_u8_t port_num, fbe_port_substate_t port_substate);
fbe_u8_t fbe_module_mgmt_get_platform_limit(fbe_module_mgmt_t *module_mgmt,
                                            IO_CONTROLLER_PROTOCOL port_protocol, 
                                            fbe_ioport_role_t port_role,
                                            fbe_module_slic_type_t slic_type);
fbe_ioport_role_t fbe_module_mgmt_get_port_role(fbe_module_mgmt_t *module_mgmt, SP_ID sp_id, fbe_u8_t iom_num, fbe_u8_t port_num);
void fbe_module_mgmt_set_port_role(fbe_module_mgmt_t *module_mgmt, SP_ID sp_id, fbe_u8_t iom_num, fbe_u8_t port_num, fbe_ioport_role_t port_role);
fbe_ioport_role_t fbe_module_mgmt_derive_port_role(fbe_module_mgmt_t *module_mgmt, SP_ID sp_id, fbe_u8_t iom_num, fbe_u8_t port_num);
fbe_port_subrole_t fbe_module_mgmt_get_port_subrole(fbe_module_mgmt_t *module_mgmt, 
                                              SP_ID sp_id, fbe_u8_t iom_num, 
                                              fbe_u8_t port_num);
void fbe_module_mgmt_set_port_subrole(fbe_module_mgmt_t *module_mgmt, 
                                   SP_ID sp_id, fbe_u8_t iom_num, 
                                   fbe_u8_t port_num, fbe_port_subrole_t port_subrole);
fbe_iom_group_t fbe_module_mgmt_get_iom_group(fbe_module_mgmt_t *module_mgmt, SP_ID sp_id, fbe_u8_t iom_num, fbe_u8_t port_num);
void fbe_module_mgmt_set_iom_group(fbe_module_mgmt_t *module_mgmt, SP_ID sp_id, fbe_u8_t iom_num, fbe_u8_t port_num, fbe_iom_group_t iom_group);
void fbe_module_mgmt_set_iom_group(fbe_module_mgmt_t *module_mgmt, SP_ID sp_id, fbe_u8_t iom_num, fbe_u8_t port_num, fbe_iom_group_t iom_group);
fbe_iom_group_t fbe_module_mgmt_derive_iom_group(fbe_module_mgmt_t *module_mgmt, SP_ID sp_id, fbe_u8_t iom_num, fbe_u8_t port_num, HW_MODULE_TYPE unique_id);
fbe_iom_group_t fbe_module_mgmt_derive_iom_group_from_sfp(fbe_module_mgmt_t *module_mgmt, 
                                                         SP_ID sp_id, fbe_u8_t iom_index, 
                                                         fbe_u8_t port_num);
fbe_sfp_protocol_t fbe_module_mgmt_derive_sfp_protocols(fbe_module_mgmt_t *module_mgmt, 
                                                 SP_ID sp_id, fbe_u8_t iom_index, 
                                                 fbe_u8_t port_num);
void fbe_module_mgmt_check_and_power_down_slic(fbe_module_mgmt_t *module_mgmt, SP_ID sp_id, fbe_u32_t iom_num);
void fbe_module_mgmt_check_platform_limits(fbe_module_mgmt_t *module_mgmt, SP_ID sp_id, fbe_u8_t iom_num, fbe_u8_t port_num);
fbe_bool_t fbe_module_mgmt_is_iom_supported(fbe_module_mgmt_t *module_mgmt, SP_ID sp_id, fbe_u8_t iom_num);
void fbe_module_mgmt_check_all_module_and_port_states(fbe_module_mgmt_t *module_mgmt);
void fbe_module_mgmt_check_module_state(fbe_module_mgmt_t *module_mgmt, SP_ID sp_id, fbe_u8_t iom_num);
void fbe_module_mgmt_check_port_state(fbe_module_mgmt_t *module_mgmt, SP_ID sp_id, fbe_u8_t iom_num, fbe_u8_t port_num);
fbe_bool_t fbe_module_mgmt_check_combined_port_fault(fbe_module_mgmt_t *module_mgmt, 
                                                     SP_ID sp_id, fbe_u8_t iom_num, 
                                                     fbe_u8_t port_num);
fbe_bool_t fbe_module_mgmt_is_iom_over_limit(fbe_module_mgmt_t *module_mgmt, SP_ID sp_id, fbe_u32_t iom_num);
fbe_bool_t fbe_module_mgmt_is_jetfire_with_octane_system(fbe_module_mgmt_t *module_mgmt);
fbe_u32_t fbe_module_mgmt_get_combined_port_count(fbe_module_mgmt_t *module_mgmt, 
                                                     SP_ID sp_id, fbe_u8_t iom_num);
fbe_u32_t fbe_module_mgmt_get_secondary_combined_port(fbe_module_mgmt_t *module_mgmt,
                                                      SP_ID sp_id, fbe_u8_t iom_num,
                                                      fbe_u8_t port_num);
fbe_u32_t fbe_module_mgmt_get_port_phy_mapping(fbe_module_mgmt_t *module_mgmt,
                                               SP_ID sp_id, fbe_u8_t iom_num,
                                               fbe_u8_t port_num);
fbe_u32_t fbe_module_mgmt_get_combined_port_phy_mapping(fbe_module_mgmt_t *module_mgmt,
                                               SP_ID sp_id, fbe_u8_t iom_num,
                                               fbe_u8_t port_num);
fbe_module_slic_type_t fbe_module_mgmt_get_slic_type(fbe_module_mgmt_t *module_mgmt, SP_ID sp_id, fbe_u8_t iom_num);
void fbe_module_mgmt_set_slic_type(fbe_module_mgmt_t *module_mgmt, 
                                                      SP_ID sp_id, 
                                                      fbe_u8_t iom_num,
                                                      fbe_module_slic_type_t slic_type);
fbe_module_slic_type_t fbe_module_mgmt_derive_slic_type(fbe_module_mgmt_t *module_mgmt, HW_MODULE_TYPE unique_id);
void fbe_module_mgmt_derive_label_name(fbe_module_mgmt_t * module_mgmt, HW_MODULE_TYPE unique_id, char *label_name);
void fbe_module_mgmt_set_io_module_protocol(fbe_module_mgmt_t *module_mgmt, SP_ID sp_id, fbe_u8_t iom_num, FBE_IO_MODULE_PROTOCOL iom_protocol);
FBE_IO_MODULE_PROTOCOL fbe_module_mgmt_derive_io_module_protocol(fbe_module_mgmt_t *module_mgmt, HW_MODULE_TYPE unique_id);
FBE_IO_MODULE_PROTOCOL fbe_module_mgmt_get_io_module_protocol(fbe_module_mgmt_t *module_mgmt, SP_ID sp_id, fbe_u8_t iom_num);
IO_CONTROLLER_PROTOCOL fbe_module_mgmt_get_port_protocol(fbe_module_mgmt_t *module_mgmt, SP_ID sp_id, fbe_u8_t iom_num, fbe_u8_t port_num);
IO_CONTROLLER_PROTOCOL fbe_module_mgmt_get_port_controller_protocol(fbe_module_mgmt_t *module_mgmt, 
                                                                    SP_ID sp_id, fbe_u8_t iom_num, 
                                                                    fbe_u8_t port_num);
fbe_mgmt_status_t fbe_module_mgmt_get_port_sfp_capable(fbe_module_mgmt_t *module_mgmt, SP_ID sp_id, fbe_u8_t iom_num, fbe_u8_t port_num);
fbe_bool_t fbe_module_mgmt_is_port_configured_combined_port(fbe_module_mgmt_t *module_mgmt, 
                                                  SP_ID sp_id, fbe_u32_t iom_num, 
                                                  fbe_u32_t port_num);
fbe_u32_t fbe_module_mgmt_get_io_module_configured_combined_port_count(fbe_module_mgmt_t *module_mgmt, SP_ID sp_id, fbe_u32_t iom_num);
fbe_bool_t fbe_module_mgmt_is_port_combined_port(fbe_module_mgmt_t *module_mgmt, 
                                                  SP_ID sp_id, fbe_u32_t iom_num, 
                                                  fbe_u32_t port_num);
fbe_bool_t fbe_module_mgmt_is_port_second_combined_port(fbe_module_mgmt_t *module_mgmt, 
                                                  SP_ID sp_id, fbe_u32_t iom_num, 
                                                  fbe_u32_t port_num);
fbe_bool_t fbe_module_mgmt_is_port_initialized(fbe_module_mgmt_t *module_mgmt, SP_ID sp_id, fbe_u8_t iom_num, fbe_u8_t port_num);
fbe_bool_t fbe_module_mgmt_is_iom_initialized(fbe_module_mgmt_t *module_mgmt, SP_ID sp_id, fbe_u8_t iom_num);
fbe_bool_t fbe_module_mgmt_is_iom_type_ok(fbe_module_mgmt_t *module_mgmt, SP_ID sp_id, fbe_u8_t iom_num);
fbe_bool_t fbe_module_mgmt_is_iom_inserted(fbe_module_mgmt_t *module_mgmt, SP_ID sp_id, fbe_u8_t iom_num);
fbe_bool_t fbe_module_mgmt_is_iom_supported(fbe_module_mgmt_t *module_mgmt, SP_ID sp_id, fbe_u8_t iom_num);
fbe_bool_t fbe_module_mgmt_is_iom_peerboot_fault(fbe_module_mgmt_t *module_mgmt, SP_ID sp_id, fbe_u8_t iom_num);
fbe_module_slic_type_t fbe_module_mgmt_get_expected_slic_type(fbe_module_mgmt_t *module_mgmt, SP_ID sp_id, fbe_u8_t iom_num);
fbe_bool_t fbe_module_mgmt_is_iom_power_good(fbe_module_mgmt_t *module_mgmt, SP_ID sp_id, fbe_u8_t iom_num);
fbe_bool_t fbe_module_mgmt_is_iom_hwErrMonFault(fbe_module_mgmt_t *module_mgmt, SP_ID sp_id, fbe_u8_t iom_num);
fbe_power_status_t fbe_module_mgmt_get_iom_power_status(fbe_module_mgmt_t *module_mgmt, SP_ID sp_id, fbe_u8_t iom_num);
fbe_bool_t fbe_module_mgmt_get_iom_power_enabled(fbe_module_mgmt_t *module_mgmt,SP_ID sp_id, fbe_u8_t iom_num);
fbe_bool_t fbe_module_mgmt_get_internal_fan_fault(fbe_module_mgmt_t *module_mgmt, SP_ID sp_id, fbe_u8_t iom_num);
fbe_u32_t fbe_module_mgmt_get_num_ports_present_on_iom(fbe_module_mgmt_t *module_mgmt, SP_ID sp_id, fbe_u8_t iom_num);
fbe_port_state_t fbe_module_mgmt_get_port_state(fbe_module_mgmt_t *module_mgmt, 
                                    SP_ID sp_id, fbe_u8_t iom_num, 
                                    fbe_u8_t port_num);
fbe_port_substate_t fbe_module_mgmt_get_port_substate(fbe_module_mgmt_t *module_mgmt, 
                                       SP_ID sp_id, fbe_u8_t iom_num, 
                                       fbe_u8_t port_num);
fbe_u32_t fbe_module_mgmt_convert_device_type_and_slot_to_index(fbe_u64_t device_type, fbe_u32_t slot);

void fbe_module_mgmt_iom_num_to_slot_and_device_type(fbe_module_mgmt_t *module_mgmt, SP_ID sp, 
                                                             fbe_u8_t iom_num, fbe_u32_t *slot, 
                                                             fbe_u64_t *device_type);
void fbe_module_mgmt_set_port_logical_number(fbe_module_mgmt_t *module_mgmt, SP_ID sp_id, fbe_u32_t iom_num, fbe_u32_t port_num, fbe_u32_t logical_num);
fbe_u32_t fbe_module_mgmt_get_port_logical_number(fbe_module_mgmt_t *module_mgmt, SP_ID sp_id, fbe_u32_t iom_num, fbe_u32_t port_num);

fbe_status_t fbe_module_mgmt_assign_port_logical_numbers(fbe_module_mgmt_t *module_mgmt, SP_ID sp_id);
fbe_bool_t fbe_module_mgmt_assign_port_subrole(fbe_module_mgmt_t *module_mgmt, SP_ID sp_id);
fbe_u32_t fbe_module_mgmt_get_next_available_port(fbe_module_mgmt_t *module_mgmt, fbe_ioport_role_t role);
fbe_bool_t fbe_module_mgmt_upgrade_ports(fbe_module_mgmt_t *module_mgmt);
fbe_bool_t fbe_module_mgmt_is_slic_upgradeable(fbe_module_mgmt_t *module_mgmt, SP_ID sp, fbe_u32_t iom_num);
fbe_bool_t fbe_module_mgmt_is_slic_convertable(fbe_module_mgmt_t *module_mgmt, SP_ID sp, fbe_u32_t iom_num);
void fbe_module_mgmt_set_device_type(fbe_module_mgmt_t *module_mgmt, SP_ID sp, fbe_u8_t iom_num, fbe_u64_t device_type);
fbe_u64_t fbe_module_mgmt_get_device_type(fbe_module_mgmt_t *module_mgmt, SP_ID sp, fbe_u8_t iom_num);
fbe_u32_t fbe_module_mgmt_get_slot_num(fbe_module_mgmt_t *module_mgmt, SP_ID sp, fbe_u8_t iom_num);
void fbe_module_mgmt_set_persistent_port_location(fbe_module_mgmt_t *module_mgmt, SP_ID sp, fbe_u8_t iom_num, fbe_u8_t port_num);
void fbe_module_mgmt_load_mgmt_port_persistent_data(fbe_module_mgmt_t *module_mgmt);
void fbe_module_mgmt_persist_mgmt_port_speed(fbe_module_mgmt_t *mgmt_module);
void fbe_module_mgmt_store_mgmt_port_speed_persistent_data(fbe_module_mgmt_t *module_mgmt);
void fbe_module_mgmt_persist_mgmt_port_speed(fbe_module_mgmt_t *mgmt_module);
fbe_u8_t fbe_module_mgmt_get_io_port_module_number(fbe_module_mgmt_t *module_mgmt, 
                                                   SP_ID sp, fbe_u8_t port_index);
fbe_u8_t fbe_module_mgmt_get_io_port_number(fbe_module_mgmt_t *module_mgmt, 
                                            SP_ID sp, fbe_u8_t port_index);

SPECL_PCI_DATA fbe_module_mgmt_get_pci_info(fbe_module_mgmt_t *module_mgmt, SP_ID sp_id, fbe_u32_t iom_num, fbe_u32_t port_num);
void fbe_module_mgmt_set_pci_info(fbe_module_mgmt_t *module_mgmt, 
                                            SP_ID sp_id, fbe_u32_t iom_num, 
                                            fbe_u32_t port_num,
                                            SPECL_PCI_DATA pci_info);
fbe_u32_t fbe_module_mgmt_get_port_index_from_pci_info(fbe_module_mgmt_t *module_mgmt, fbe_u32_t bus, fbe_u32_t device, fbe_u32_t function);
fbe_u32_t fbe_module_mgmt_get_iom_num_from_port_index(fbe_module_mgmt_t *module_mgmt, fbe_u32_t port_index);
fbe_u32_t fbe_module_mgmt_get_port_num_from_port_index(fbe_module_mgmt_t *module_mgmt, fbe_u32_t port_index);
fbe_port_link_state_t fbe_module_mgmt_get_port_link_state(fbe_module_mgmt_t *module_mgmt, 
                                                                     SP_ID sp_id, fbe_u32_t iom_num,
                                                                     fbe_u32_t port_num);
void fbe_module_mgmt_set_port_link_state(fbe_module_mgmt_t *module_mgmt,
                                                    SP_ID sp_id, fbe_u32_t iom_num,
                                                    fbe_u32_t port_num, fbe_port_link_state_t new_link_state);
void fbe_module_mgmt_set_port_link_info(fbe_module_mgmt_t *module_mgmt,
                                                    SP_ID sp_id, fbe_u32_t iom_num,
                                                    fbe_u32_t port_num, fbe_port_link_information_t *new_link_info);
fbe_port_link_information_t fbe_module_mgmt_get_port_link_info(fbe_module_mgmt_t *module_mgmt,
                                                    SP_ID sp_id, fbe_u32_t iom_num,
                                                    fbe_u32_t port_num);
fbe_port_sfp_condition_type_t fbe_module_mgmt_get_port_sfp_condition(fbe_module_mgmt_t *module_mgmt, SP_ID sp_id, fbe_u32_t iom_num, fbe_u32_t port_num);
fbe_port_sfp_sub_condition_type_t fbe_module_mgmt_get_port_sfp_subcondition(fbe_module_mgmt_t *module_mgmt, SP_ID sp_id, fbe_u32_t iom_num, fbe_u32_t port_num);
void fbe_module_mgmt_handle_file_write_completion(fbe_base_object_t * base_object, fbe_u32_t thread_delay);

fbe_status_t fbe_module_mgmt_set_upgrade_ports_and_reboot(fbe_module_mgmt_t *module_mgmt);
fbe_status_t fbe_module_mgmt_reboot_to_allow_port_replace(fbe_module_mgmt_t *module_mgmt);
fbe_status_t fbe_module_mgmt_set_persist_ports_and_reboot(fbe_module_mgmt_t *module_mgmt);

fbe_status_t fbe_module_mgmt_convert_hellcat_lite_to_sentry(fbe_module_mgmt_t *module_mgmt);
fbe_status_t fbe_module_mgmt_convert_sentry_to_argonaut(fbe_module_mgmt_t *module_mgmt);
void fbe_module_mgmt_initialize_configured_port_info(fbe_module_mgmt_t *module_mgmt);

fbe_status_t fbe_module_mgm_set_port_list(fbe_module_mgmt_t *module_mgmt, 
                                          fbe_u32_t num_ports, 
                                          fbe_io_port_persistent_info_t *new_port_config,
                                          fbe_bool_t overwrite);
fbe_status_t fbe_module_mgmt_check_duplicate_port_in_list(fbe_module_mgmt_t *module_mgmt, 
                                                          fbe_io_port_persistent_info_t *port_config_list, 
                                                          fbe_u32_t logical_num, 
                                                          fbe_ioport_role_t port_role);
fbe_status_t fbe_module_mgmt_check_overlimit_port_in_list(fbe_module_mgmt_t *module_mgmt,
                                                         fbe_io_port_persistent_info_t *port_config_list);
fbe_status_t fbe_module_mgmt_remove_all_ports(fbe_module_mgmt_t *module_mgmt);
void fbe_module_mgmt_generate_slic_list_string(fbe_module_slic_type_t slic_type, fbe_char_t *types_string);
fbe_status_t fbe_module_mgmt_log_bm_lcc_event(fbe_module_mgmt_t * module_mgmt,
                                       fbe_board_mgmt_io_comp_info_t * pNewBmInfo,
                                       fbe_board_mgmt_io_comp_info_t * pOldBmInfo);
fbe_status_t fbe_module_mgmt_update_encl_fault_led(fbe_module_mgmt_t *module_mgmt,
                                    fbe_device_physical_location_t *pLocation,
                                    fbe_u64_t device_type,
                                    fbe_encl_fault_led_reason_t enclFaultLedReason);
fbe_bool_t fbe_module_mgmt_could_set_io_port_led(fbe_module_mgmt_t *module_mgmt,
                                                 fbe_u8_t           iom_num,
                                                 SP_ID              sp_id);
fbe_status_t fbe_module_mgmt_setIoPortLedBasedOnLink(fbe_module_mgmt_t *module_mgmt,
                                                     fbe_u8_t iom_num,
                                                     fbe_u8_t port_num);
fbe_u32_t fbe_module_mgmt_get_portal_number(fbe_module_mgmt_t *module_mgmt, 
                                                  SP_ID sp_id, fbe_u32_t iom_num, 
                                                  fbe_u32_t port_num);
void fbe_module_mgmt_set_portal_number(fbe_module_mgmt_t *module_mgmt, 
                                                  SP_ID sp_id, fbe_u32_t iom_num, 
                                                  fbe_u32_t port_num, fbe_u32_t portal_num);

fbe_status_t fbe_module_mgmt_set_selected_pci_function(fbe_module_mgmt_t *module_mgmt, fbe_u32_t port_index, SP_ID sp_id);
IO_CONTROLLER_PROTOCOL fbe_module_mgmt_derive_protocol_from_iom_group(fbe_module_mgmt_t *module_mgmt, fbe_iom_group_t iom_group);
fbe_bool_t fbe_module_mgmt_check_cna_ports_match(fbe_module_mgmt_t *module_mgmt, SP_ID sp, fbe_u32_t iom_num, fbe_u32_t port_num);
fbe_status_t fbe_module_mgmt_check_cna_port_protocol(fbe_module_mgmt_t *module_mgmt);
fbe_status_t fbe_module_mgmt_get_first_cna_port(fbe_module_mgmt_t *module_mgmt, fbe_u32_t *iom_num, fbe_u32_t *port_num);
fbe_bool_t fbe_module_mgmt_is_cna_port(fbe_module_mgmt_t *module_mgmt, fbe_u32_t iom_num, fbe_u32_t port_num);
fbe_bool_t fbe_module_mgmt_is_sfp_cna_capable(fbe_module_mgmt_t *module_mgmt, 
                                                 SP_ID sp_id, fbe_u8_t iom_index, 
                                                 fbe_u8_t port_num);

/* String Generation Functionality */
const fbe_char_t * fbe_module_mgmt_cmi_msg_to_string(fbe_module_mgmt_cmi_msg_code_t msg_code);

/* Registry IO Functionality */

fbe_bool_t fbe_module_mgmt_reg_check_and_update_registry(fbe_module_mgmt_t *module_mgmt);
fbe_status_t fbe_module_mgmt_set_cpd_params(fbe_u32_t port, fbe_char_t *port_param_string);
fbe_status_t fbe_module_mgmt_get_cpd_params(fbe_u32_t port, fbe_char_t *port_param_string);
fbe_status_t fbe_module_mgmt_set_persist_port_info(fbe_module_mgmt_t *module_mgmt, fbe_bool_t persist_port_info);
fbe_status_t fbe_module_mgmt_get_persist_port_info(fbe_module_mgmt_t *module_mgmt, fbe_bool_t *persist_port_info);
fbe_status_t fbe_module_mgmt_set_netconf_reinitialize(void);
fbe_status_t fbe_module_mgmt_set_slics_marked_for_upgrade(fbe_module_mgmt_upgrade_type_t *marked_for_upgrade);
fbe_status_t fbe_module_mgmt_get_slics_marked_for_upgrade (fbe_module_mgmt_upgrade_type_t *marked_for_upgrade);
fbe_status_t fbe_module_mgmt_get_conversion_type(fbe_module_mgmt_conversion_type_t *conversion_type);
fbe_status_t fbe_module_mgmt_get_reset_port_interrupt_affinity(fbe_u32_t *reset_port_interrupt_affinity);
fbe_status_t fbe_module_mgmt_set_reset_port_interrupt_affinity(UINT_32 reset_port_interrupt_affinity);
fbe_status_t fbe_module_mgmt_get_miniport_instance_count(fbe_char_t *miniport_name, fbe_u32_t *instance_count);
fbe_status_t fbe_module_mgmt_get_miniport_instance_string(fbe_char_t *miniport_name, fbe_u32_t *instance_number, fbe_char_t *instance_string);
fbe_status_t fbe_module_mgmt_get_configured_interrupt_affinity_string(fbe_u32_t *instance_number, fbe_char_t *instance_string);
fbe_status_t fbe_module_mgmt_set_configured_interrupt_affinity_string(fbe_u32_t *instance_number, fbe_char_t *instance_string);
fbe_status_t fbe_module_mgmt_clear_configured_interrupt_affinity_string(fbe_u32_t *instance_number);
VOID fbe_module_mgmt_get_pci_location_from_instance(fbe_char_t *device_id, fbe_char_t *instance_id, fbe_char_t *pci_string);
fbe_status_t fbe_module_mgmt_set_processor_affinity(fbe_char_t *device_id, fbe_char_t *instance_id, fbe_u32_t core_num);
fbe_status_t fbe_module_mgmt_set_msi_message_limit(fbe_char_t *device_id, fbe_char_t *instance_id, fbe_u32_t limit);
fbe_status_t fbe_module_mgmt_get_enable_port_interrupt_affinity(fbe_u32_t *port_interrupt_affinity_enabled);
fbe_status_t fbe_module_mgmt_set_enable_port_interrupt_affinity(fbe_u32_t port_interrupt_affinity_enabled);
fbe_status_t fbe_module_mgmt_get_processor_affinity(fbe_module_mgmt_t *module_mgmt, fbe_char_t *device_id, fbe_char_t *instance_id, fbe_u32_t *core_num);

fbe_u32_t fbe_module_mgmt_port_affinity_setting(void);

fbe_bool_t fbe_module_mgmt_check_and_update_interrupt_affinities(fbe_module_mgmt_t *module_mgmt, 
                                                                 fbe_bool_t force_affinity_setting_update, 
                                                                 fbe_u32_t mode,
                                                                 fbe_bool_t clear_affinity_settings);
fbe_u32_t fbe_module_mgmt_generate_affinity_setting(fbe_module_mgmt_t *module_mgmt,
                                                    fbe_bool_t set_default, 
                                                    SPECL_PCI_DATA *pci_info);
fbe_module_mgmt_port_reg_info_t * fbe_module_mgmt_reg_get_port_info(fbe_module_mgmt_t *module_mgmt,
                                                                    fbe_u32_t bus, fbe_u32_t slot, fbe_u32_t func);

fbe_bool_t fbe_module_mgmt_reg_scan_string_for_string(fbe_char_t stop, fbe_u32_t length, 
                                                fbe_char_t *buffer, fbe_u32_t *index, 
                                                fbe_char_t *result);
fbe_bool_t fbe_module_mgmt_reg_scan_string_for_int(fbe_char_t stop, fbe_u32_t length, 
                                             fbe_char_t *buffer, fbe_u32_t *index, 
                                             fbe_u32_t *result);
fbe_u32_t fbe_module_mgmt_get_port_affinity_processor_core(fbe_module_mgmt_t *module_mgmt,
                                                           SPECL_PCI_DATA  pciData);
/* fbe_module_mgmt_fup.c */
fbe_status_t fbe_module_mgmt_fup_handle_bem_status_change(fbe_module_mgmt_t * pModuleMgmt,
                                                 fbe_device_physical_location_t * pLocation,
                                                 fbe_lcc_info_t * pNewLccInfo,
                                                 fbe_lcc_info_t * pOldLccInfo);

fbe_status_t fbe_module_mgmt_fup_initiate_upgrade(void * pContext, 
                                              fbe_u64_t deviceType, 
                                              fbe_device_physical_location_t * pLocation,
                                              fbe_u32_t forceFlags,
                                              fbe_u32_t upgradeRetryCount);

fbe_status_t fbe_module_mgmt_fup_get_full_image_path(void * pContext,                               
                               fbe_base_env_fup_work_item_t * pWorkItem);

fbe_status_t fbe_module_mgmt_fup_get_manifest_file_full_path(void * pContext,
                                                           fbe_char_t * pManifestFileFullPath);

fbe_status_t fbe_module_mgmt_fup_check_env_status(void * pContext, 
                                 fbe_base_env_fup_work_item_t * pWorkItem);

fbe_status_t fbe_module_mgmt_fup_get_firmware_rev(void * pContext,
                          fbe_base_env_fup_work_item_t * pWorkItem,
                          fbe_u8_t * pFirmwareRev);

fbe_status_t fbe_module_mgmt_get_fup_info_ptr(void * pContext,
                                            fbe_u64_t deviceType,
                                            fbe_device_physical_location_t * pLocation,
                                            fbe_base_env_fup_persistent_info_t ** pLccFupInfoPtr);

fbe_status_t fbe_module_mgmt_get_fup_info(void * pContext,
                                            fbe_u64_t deviceType,
                                            fbe_device_physical_location_t * pLocation,
                                            fbe_esp_base_env_get_fup_info_t *  pGetFupInfo);

fbe_status_t fbe_module_mgmt_fup_resume_upgrade(void * pContext);

fbe_status_t fbe_module_mgmt_fup_new_contact_init_upgrade(fbe_module_mgmt_t * pModuleMgmt, 
                                                        fbe_u64_t deviceType);

fbe_status_t fbe_module_mgmt_fup_copy_fw_rev_to_resume_prom(fbe_module_mgmt_t *pModuleMgmt,
                                                        fbe_u64_t deviceType,
                                                        fbe_device_physical_location_t *pLocation);

/* fbe_module_mgmt_resume_prom.c */
fbe_status_t fbe_module_mgmt_initiate_resume_prom_read(fbe_module_mgmt_t * pModuleMgmt,
                                                     fbe_u64_t deviceType,
                                                     fbe_device_physical_location_t * pLocation);

fbe_status_t fbe_module_mgmt_resume_prom_handle_io_comp_status_change(fbe_module_mgmt_t * pModuleMgmt, 
                                                                      fbe_device_physical_location_t * pLocation, 
                                                                      fbe_u64_t device_type, 
                                                                      fbe_board_mgmt_io_comp_info_t * pNewIoCompInfo, 
                                                                      fbe_board_mgmt_io_comp_info_t * pOldIoCompInfo);

fbe_status_t fbe_module_mgmt_resume_prom_handle_mgmt_module_status_change(fbe_module_mgmt_t * pModuleMgmt, 
                                                                          fbe_device_physical_location_t * pLocation, 
                                                                          fbe_u64_t device_type, 
                                                                          fbe_board_mgmt_mgmt_module_info_t * pNewMgmtModInfo, 
                                                                          fbe_board_mgmt_mgmt_module_info_t * pOldMgmtModInfo);

fbe_status_t fbe_module_mgmt_get_resume_prom_info_ptr(fbe_module_mgmt_t * pModuleMgmt,
                                                      fbe_u64_t deviceType,
                                                      fbe_device_physical_location_t * pLocation,
                                                      fbe_base_env_resume_prom_info_t ** pResumePromInfoPtr);

fbe_status_t fbe_module_mgmt_resume_prom_update_encl_fault_led(fbe_module_mgmt_t * pModuleMgmt,
                                                               fbe_u64_t deviceType,
                                                               fbe_device_physical_location_t *pLocation);

fbe_bool_t fbe_module_mgmt_is_special_port_assigned(fbe_module_mgmt_t *module_mgmt, SP_ID sp_id, fbe_iom_group_t group);

fbe_status_t fbe_module_mgmt_get_disable_reg_update_info(fbe_module_mgmt_t *module_mgmt, fbe_bool_t *disable_reg_update_info);
fbe_status_t fbe_module_mgmt_set_reboot_required(void);

/* fbe_module_mgmt_kernel_main.c and fbe_module_mgmt_sim_main.c */
fbe_status_t fbe_module_mgmt_fup_build_image_file_name(fbe_module_mgmt_t * pModuleMgmt,
                                                       fbe_base_env_fup_work_item_t * pWorkItem,
                                                       fbe_u8_t * pImageFileNameBuffer,
                                                       char * pImageFileNameConstantPortion,
                                                       fbe_u8_t bufferLen,
                                                       fbe_u32_t * pImageFileNameLen);

#endif /* MODULE_MGMT_PRIVATE_H */

/*******************************
 * end fbe_module_mgmt_private.h
 *******************************/
