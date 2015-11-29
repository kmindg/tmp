#ifndef FBE_API_DRIVE_CONFIG_SERVICE_INTERFACE_H
#define FBE_API_DRIVE_CONFIG_SERVICE_INTERFACE_H

/***************************************************************************
 * Copyright (C) EMC Corporation 2001-2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_api_drive_configuration_service_interface.h
 ***************************************************************************
 *
 * @brief
 *  This file contains the APIs for getting information on the 
 *  object id's of devices.
 * 
 * @ingroup fbe_api_physical_package_interface_class_files
 * 
 * @version
 *   04/08/00 sharel    Created
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_types.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_drive_configuration_interface.h"
#include "fbe/fbe_drive_configuration_download.h"


//----------------------------------------------------------------
// Define the top level group for the FBE Physical Package APIs 
//----------------------------------------------------------------
/*! @defgroup fbe_physical_package_class FBE Physical Package APIs 
 *  @brief 
 *    This is the set of definitions for FBE Physical Package APIs.
 * 
 *  @ingroup fbe_api_physical_package_interface
 */ 
//----------------------------------------------------------------

//----------------------------------------------------------------
// Define the FBE API Drive Configuration Interface for the Usurper Interface. 
// This is where all the data structures defined. 
//----------------------------------------------------------------
/*! @defgroup fbe_api_drive_configuration_service_usurper_interface FBE API Drive Configuration Service Usurper Interface
 *  @brief 
 *    This is the set of definitions that comprise the FBE API Drive Configuration Service class
 *    usurper interface.
 *  
 *  @ingroup fbe_api_classes_usurper_interface
 *  @{
 */ 
//----------------------------------------------------------------

FBE_API_CPP_EXPORT_START

/*!********************************************************************* 
 * @enum fbe_api_drive_config_handle_type_t 
 *  
 * @brief 
 *  This contains the enumeration for the drive config type.
 *
 * @ingroup fbe_api_drive_configuration_service_interface
 * @ingroup fbe_api_drive_config_handle_type
 **********************************************************************/
typedef enum fbe_api_drive_config_handle_type_e{
    FBE_API_HANDLE_TYPE_INVALID, /*!< FBE_API_HANDLE_TYPE_INVALID */
    FBE_API_HANDLE_TYPE_DRIVE,   /*!< This is a handle to a drive */
    FBE_API_HANDLE_TYPE_PORT     /*!< The is a handle to a port */
}fbe_api_drive_config_handle_type_t;

/*FBE_DRIVE_CONFIGURATION_CONTROL_GET_HANDLES_LIST*/
/*FBE_DRIVE_CONFIGURATION_CONTROL_GET_PORT_HANDLES_LIST*/

/*!********************************************************************* 
 * @struct fbe_api_drive_config_get_handles_list_t 
 *  
 * @brief 
 *  This contains the data needed to get the list of all handles in the table
 *
 * @ingroup fbe_api_drive_configuration_service_interface
 * @ingroup fbe_api_drive_config_get_handles_list
 **********************************************************************/
typedef struct fbe_api_drive_config_get_handles_list_t{
    fbe_drive_configuration_handle_t    handles_list[MAX_THRESHOLD_TABLE_RECORDS]; /*!< Handle List */
    fbe_u32_t                           total_count; /*!< Total count */
}fbe_api_drive_config_get_handles_list_t;
/*! @} */ /* end of group fbe_api_drive_configuration_service_usurper_interface */


//----------------------------------------------------------------
// Define the group for the FBE API Drive Configuration Interface.  
// This is where all the function prototypes for the Drive Configuration API.
//----------------------------------------------------------------
/*! @defgroup fbe_api_drive_configuration_service_interface FBE API Drive Configuration Service Interface
 *  @brief 
 *    This is the set of FBE API Drive Configuration Service Interface.
 *
 *  @details 
 *    In order to access this library, please include fbe_api_drive_configuration_service_interface.h.
 *
 *  @ingroup fbe_physical_package_class
 *  @{
 */
//----------------------------------------------------------------

fbe_status_t            FBE_API_CALL  fbe_api_drive_configuration_interface_get_handles(fbe_api_drive_config_get_handles_list_t *get_hanlde, fbe_api_drive_config_handle_type_t type);
fbe_status_t            FBE_API_CALL  fbe_api_drive_configuration_interface_get_drive_table_entry(fbe_drive_configuration_handle_t handle, fbe_drive_configuration_record_t *entry);
fbe_status_t            FBE_API_CALL  fbe_api_drive_configuration_interface_change_drive_thresholds(fbe_drive_configuration_handle_t handle, fbe_drive_stat_t *new_thresholds);
fbe_status_t            FBE_API_CALL  fbe_api_drive_configuration_interface_add_drive_table_entry(fbe_drive_configuration_record_t *entry, fbe_drive_configuration_handle_t *handle);
fbe_dcs_dieh_status_t   FBE_API_CALL  fbe_api_drive_configuration_interface_start_update(void);
fbe_status_t            FBE_API_CALL  fbe_api_drive_configuration_interface_end_update(void);
fbe_status_t            FBE_API_CALL  fbe_api_drive_configuration_interface_add_port_table_entry(fbe_drive_configuration_port_record_t *entry, fbe_drive_configuration_handle_t *handle);
fbe_status_t            FBE_API_CALL  fbe_api_drive_configuration_interface_change_port_thresholds(fbe_drive_configuration_handle_t handle, fbe_port_stat_t *new_thresholds);
fbe_status_t            FBE_API_CALL  fbe_api_drive_configuration_interface_get_port_table_entry(fbe_drive_configuration_handle_t handle, fbe_drive_configuration_port_record_t *entry);
fbe_status_t            FBE_API_CALL  fbe_api_drive_configuration_interface_download_firmware(fbe_drive_configuration_control_fw_info_t *fw_info);
fbe_status_t            FBE_API_CALL  fbe_api_drive_configuration_interface_abort_download(void);
fbe_status_t            FBE_API_CALL  fbe_api_drive_configuration_interface_get_download_process(fbe_drive_configuration_download_get_process_info_t *process_info);
fbe_status_t            FBE_API_CALL  fbe_api_drive_configuration_interface_get_download_drive(fbe_drive_configuration_download_get_drive_info_t *drive_info);
fbe_status_t            FBE_API_CALL  fbe_api_drive_configuration_interface_get_download_max_drive_count(fbe_drive_configuration_get_download_max_drive_count_t *fw_max_dl_count);
fbe_status_t            FBE_API_CALL  fbe_api_drive_configuration_interface_get_all_download_drives(fbe_drive_configuration_download_get_drive_info_t *drive_info, fbe_u32_t expected_count, fbe_u32_t * actual_drives);
fbe_status_t            FBE_API_CALL  fbe_api_drive_configuration_interface_dieh_force_clear_update(void);
fbe_status_t            FBE_API_CALL  fbe_api_drive_configuration_interface_dieh_get_status(fbe_bool_t *is_updating);
fbe_status_t            FBE_API_CALL  fbe_api_drive_configuration_interface_get_params(fbe_dcs_tunable_params_t *params);
fbe_status_t            FBE_API_CALL  fbe_api_drive_configuration_interface_set_params(fbe_dcs_tunable_params_t *params);
fbe_status_t            FBE_API_CALL  fbe_api_drive_configuration_interface_get_mode_page_overrides(fbe_dcs_mode_select_override_info_t *ms_info);
fbe_status_t            FBE_API_CALL  fbe_api_drive_configuration_interface_set_mode_page_byte(fbe_drive_configuration_mode_page_override_table_id_t table_id, fbe_u8_t page, fbe_u8_t byte_offset, fbe_u8_t mask, fbe_u8_t value);
fbe_status_t            FBE_API_CALL  fbe_api_drive_configuration_interface_mode_page_addl_override_clear(void);
fbe_status_t            FBE_API_CALL  fbe_api_drive_configuration_set_service_timeout(fbe_time_t timeout);
fbe_status_t            FBE_API_CALL  fbe_api_drive_configuration_set_remap_service_timeout(fbe_time_t timeout);
fbe_status_t            FBE_API_CALL  fbe_api_drive_configuration_set_fw_image_chunk_size(fbe_u32_t size);
fbe_status_t            FBE_API_CALL  fbe_api_drive_configuration_set_control_flags(fbe_dcs_control_flags_t flags);
fbe_status_t            FBE_API_CALL  fbe_api_drive_configuration_set_control_flag(fbe_dcs_control_flags_t flag, fbe_bool_t do_enable);
fbe_status_t            FBE_API_CALL  fbe_api_drive_configuration_interface_reset_queuing_table(void);
fbe_status_t            FBE_API_CALL  fbe_api_drive_configuration_interface_activate_queuing_table(void);
fbe_status_t            FBE_API_CALL  fbe_api_drive_configuration_interface_add_queuing_entry(fbe_drive_configuration_queuing_record_t *entry);

/*! @} */ /* end of group fbe_api_drive_configuration_service_interface */

FBE_API_CPP_EXPORT_END

//----------------------------------------------------------------
// Define the group for all FBE Physical Package APIs Interface class files.  
// This is where all the class files that belong to the FBE API Physical 
// Package define. In addition, this group is displayed in the FBE Classes
// module.
//----------------------------------------------------------------
/*! @defgroup fbe_api_physical_package_interface_class_files FBE Physical Package APIs Interface Class Files 
 *  @brief 
 *    This is the set of files for the FBE Physical Package Interface.
 *
 *  @ingroup fbe_api_classes
 */
//----------------------------------------------------------------

#endif /* FBE_API_DRIVE_CONFIG_SERVICE_INTERFACE_H */
