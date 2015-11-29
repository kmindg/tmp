#ifndef FBE_API_ESP_DRIVE_MGMT_INTERFACE_H
#define FBE_API_ESP_DRIVE_MGMT_INTERFACE_H
/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_api_esp_drive_mgmt_interface.h
 ***************************************************************************
 *
 * @brief 
 *  This header file defines the FBE API for the ESP DRIVE Mgmt object.  
 *   
 * @ingroup fbe_api_esp_interface_class_files  
 *   
 * @version  
 *   03/16/2010:  Created. Ashok Tamilarasan  
 ****************************************************************************/

/*************************  
 *   INCLUDE FILES  
 *************************/
#include "fbe/fbe_types.h"
#include "fbe/fbe_transport.h"
#include "fbe/fbe_enclosure.h"
#include "fbe/fbe_esp_drive_mgmt.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_physical_drive.h"    


FBE_API_CPP_EXPORT_START

//---------------------------------------------------------------- 
// Define the top level group for the FBE Environment Service Package APIs
// ----------------------------------------------------------------
/*! @defgroup fbe_api_esp_interface_class FBE ESP APIs 
 *
 *  @brief 
 *    This is the set of definitions for FBE ESP APIs.
 * 
 *  @ingroup fbe_api_esp_interface
 */ 
//----------------------------------------------------------------

//---------------------------------------------------------------- 
// Define the FBE API ESP DRIVE Mgmt Interface for the Usurper Interface. 
// This is where all the data structures defined.  
//---------------------------------------------------------------- 
/*! @defgroup fbe_api_esp_drive_mgmt_interface_usurper_interface FBE API ESP DRIVE Mgmt Interface Usurper Interface  
 *  @brief   
 *    This is the set of definitions that comprise the FBE API ESP DRIVE Mgmt Interface class  
 *    usurper interface.  
 *  
 *  @ingroup fbe_api_classes_usurper_interface  
 *  @{  
 */ 
//----------------------------------------------------------------

// FBE_ESP_DRIVE_MGMT_CONTROL_CODE_GET_DRIVE_INFO


/*!********************************************************************* 
 * @struct fbe_esp_drive_mgmt_drive_info_t 
 *  
 * @brief 
 *   Contains the drive info.
 *
 * @ingroup fbe_api_esp_drive_mgmt_interface
 * @ingroup fbe_esp_drive_mgmt_drive_info
 **********************************************************************/
typedef struct fbe_esp_drive_mgmt_drive_info_s
{
    fbe_device_physical_location_t location; /*!< IN - Set by the caller for the device we want the info about */
    fbe_bool_t inserted;
    fbe_bool_t faulted;
    fbe_bool_t loop_a_valid;
    fbe_bool_t loop_b_valid;
    fbe_bool_t bypass_enabled_loop_a;
    fbe_bool_t bypass_enabled_loop_b;
    fbe_bool_t bypass_requested_loop_a;
    fbe_bool_t bypass_requested_loop_b;
    fbe_drive_type_t drive_type; /*!< Drive type. */
    fbe_lba_t gross_capacity;    /*!< Gross block capacity. */
    fbe_u8_t vendor_id[FBE_SCSI_INQUIRY_VENDOR_ID_SIZE+1];  /*!< Vendor ID. */
    fbe_u8_t product_id[FBE_SCSI_INQUIRY_PRODUCT_ID_SIZE+1];  /*!< Product ID. */
    fbe_u8_t tla_part_number[FBE_SCSI_INQUIRY_TLA_SIZE+1];  /*!< TLA Part Number. */
    fbe_u8_t product_rev[FBE_SCSI_INQUIRY_VPD_PROD_REV_SIZE+1]; /*!< Product Revision. */
    fbe_u8_t product_serial_num[FBE_SCSI_INQUIRY_SERIAL_NUMBER_SIZE+1]; /*!< Product Serial Number. */
    fbe_lifecycle_state_t       state;
    fbe_u32_t                   death_reason;
    fbe_time_t                  last_log;
    fbe_lba_t                   block_count;
    fbe_block_size_t            block_size;
    fbe_u32_t                   drive_qdepth;
    fbe_u32_t                   drive_RPM;
    fbe_physical_drive_speed_t  speed_capability;
    fbe_u8_t                    drive_description_buff[FBE_SCSI_DESCRIPTION_BUFF_SIZE+1];
    fbe_u8_t                    dg_part_number_ascii[FBE_SCSI_INQUIRY_PART_NUMBER_SIZE+1];  /* addl_inquiry_info */
    fbe_u8_t                    bridge_hw_rev[FBE_SCSI_INQUIRY_VPD_BRIDGE_HW_REV_SIZE + 1];
    fbe_bool_t                  spin_down_qualified;
    fbe_u32_t                   spin_up_time_in_minutes;/*how long the drive was spinning*/
    fbe_u32_t                   stand_by_time_in_minutes;/*how long the drive was not spinning*/
    fbe_u32_t                   spin_up_count;/*how many times the drive spun up*/
    fbe_block_transport_logical_state_t     logical_state; 
} fbe_esp_drive_mgmt_drive_info_t;

typedef struct fbe_esp_drive_mgmt_debug_control_s
{
    fbe_u8_t            bus;        // input
    fbe_u8_t            encl;       // input
    fbe_u8_t            slot;       // input
    fbe_bool_t          defer;      // input
    fbe_bool_t          dualsp;     // input
    fbe_base_object_mgmt_drv_dbg_ctrl_t dbg_ctrl;
} fbe_esp_drive_mgmt_debug_control_t;


/*!********************************************************************* 
 * @struct fbe_esp_drive_mgmt_info_t 
 *  
 * @brief 
 *   Contains general DMO DriveConfiguration xml info.
 *
 **********************************************************************/
typedef struct fbe_esp_drive_mgmt_driveconfig_info_s
{
    fbe_esp_dmo_driveconfig_xml_info_t xml_info;

} fbe_esp_drive_mgmt_driveconfig_info_t;



/*! @} */ /* end of group fbe_api_esp_drive_mgmt_interface_usurper_interface */

//---------------------------------------------------------------- 
// Define the group for the FBE API ESP DRIVE Mgmt Interface.   
// This is where all the function prototypes for the FBE API ESP DRIVE Mgmt. 
//---------------------------------------------------------------- 
/*! @defgroup fbe_api_esp_drive_mgmt_interface FBE API ESP DRIVE Mgmt Interface  
 *  @brief   
 *    This is the set of FBE API ESP DRIVE Mgmt Interface.   
 *  
 *  @details   
 *    In order to access this library, please include fbe_api_esp_drive_mgmt_interface.h.  
 *  
 *  @ingroup fbe_api_esp_interface_class  
 *  @{  
 */
//---------------------------------------------------------------- 
fbe_status_t FBE_API_CALL fbe_api_esp_drive_mgmt_get_drive_configuration_info(fbe_esp_drive_mgmt_driveconfig_info_t *dmo_info);
fbe_status_t FBE_API_CALL fbe_api_esp_drive_mgmt_get_drive_info(fbe_esp_drive_mgmt_drive_info_t *drive_info);
fbe_status_t FBE_API_CALL fbe_api_esp_drive_mgmt_get_policy(fbe_drive_mgmt_policy_t *policy);
fbe_status_t FBE_API_CALL fbe_api_esp_drive_mgmt_change_policy(fbe_drive_mgmt_policy_t *policy);
fbe_status_t FBE_API_CALL fbe_api_esp_drive_mgmt_enable_fuel_gauge(fbe_u32_t enable);
fbe_status_t FBE_API_CALL fbe_api_esp_drive_mgmt_get_fuel_gauge_poll_interval(fbe_u32_t *interval);
fbe_status_t FBE_API_CALL fbe_api_esp_drive_mgmt_set_fuel_gauge_poll_interval(fbe_u32_t interval);
fbe_status_t FBE_API_CALL fbe_api_esp_drive_mgmt_get_drive_log(fbe_device_physical_location_t *location);
fbe_status_t FBE_API_CALL fbe_api_esp_drive_mgmt_get_fuel_gauge(fbe_u32_t *enable);
fbe_status_t FBE_API_CALL fbe_api_esp_drive_mgmt_send_debug_control(fbe_u32_t bus, fbe_u32_t encl, fbe_u32_t slot, fbe_u32_t maxSlots, 
																	fbe_base_object_mgmt_drv_dbg_action_t command, fbe_bool_t defer, fbe_bool_t dualsp);
fbe_dieh_load_status_t FBE_API_CALL fbe_api_esp_drive_mgmt_dieh_load_config_file(fbe_drive_mgmt_dieh_load_config_t *config_info);
fbe_status_t FBE_API_CALL fbe_api_esp_drive_mgmt_get_all_drives_info(fbe_esp_drive_mgmt_drive_info_t *drive_info, fbe_u32_t expected_count, fbe_u32_t *actual_count);
fbe_status_t FBE_API_CALL fbe_api_esp_drive_mgmt_get_total_drives_count(fbe_u32_t *total_drive);
fbe_status_t FBE_API_CALL fbe_api_esp_drive_mgmt_get_max_platform_drive_count(fbe_u32_t *max_platform_drive_count);

fbe_status_t FBE_API_CALL fbe_api_esp_drive_mgmt_add_drive_price_class_support_entry(fbe_drive_price_class_t  drive_price_class, SPID_HW_TYPE platform);
fbe_status_t FBE_API_CALL fbe_api_esp_drive_mgmt_remove_drive_price_class_support_entry(fbe_drive_price_class_t  drive_price_class, SPID_HW_TYPE platform);
fbe_status_t FBE_API_CALL fbe_api_esp_drive_mgmt_check_drive_price_class_supported(fbe_drive_price_class_t  drive_price_class, SPID_HW_TYPE platform,
                                                                                   fbe_bool_t *is_supported);
fbe_status_t FBE_API_CALL fbe_api_esp_drive_mgmt_set_crc_actions(fbe_pdo_logical_error_action_t *action);
char *       FBE_API_CALL fbe_api_esp_drive_mgmt_map_lifecycle_state_to_str(fbe_lifecycle_state_t state);

fbe_status_t FBE_API_CALL fbe_api_esp_drive_mgmt_reduce_system_drive_queue_depth(void);
fbe_status_t FBE_API_CALL fbe_api_esp_drive_mgmt_restore_system_drive_queue_depth(void);
fbe_status_t FBE_API_CALL fbe_api_esp_drive_mgmt_reset_system_drive_queue_depth(void);

// Map death reason
char * FBE_API_CALL fbe_api_esp_drive_mgmt_map_death_reason_to_str(fbe_u32_t death_reason) ;

// Write BMS data
fbe_status_t FBE_API_CALL fbe_api_esp_write_bms_logpage_data_asynch(fbe_physical_drive_bms_op_asynch_t   *bms_op_p);

/*! @} */ /* end of group fbe_api_esp_drive_mgmt_interface */

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


/* TESTING ONLY INTERFACE FUNCTIONS 
 */
fbe_status_t FBE_API_CALL fbe_api_esp_drive_mgmt_send_simulation_command(void *command_data, fbe_u32_t size);



/**********************************************
 * end file fbe_api_esp_drive_mgmt_interface.h
 **********************************************/

#endif /* FBE_API_ESP_DRIVE_MGMT_INTERFACE_H */



