#ifndef FBE_API_ENCRYPTION_INTERFACE_H
#define FBE_API_ENCRYPTION_INTERFACE_H

/***************************************************************************
 * Copyright (C) EMC Corporation 2013
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_api_encryption_interface.h
 ***************************************************************************
 *
 * @brief
 *  This header file defines the FBE API for encryption control
 * 
 * @ingroup fbe_api_storage_extent_package_interface_class_files
 * 
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_types.h"
#include "fbe/fbe_transport.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_base_config.h"
#include "fbe/fbe_database_packed_struct.h"
#include "fbe/fbe_api_job_service_interface.h"

FBE_API_CPP_EXPORT_START

//----------------------------------------------------------------
// Define the top level group for the FBE Storage Extent Package APIs
//----------------------------------------------------------------
/*! @defgroup fbe_api_storage_extent_package_class FBE Storage Extent Package APIs
 *  @brief 
 *    This is the set of definitions for FBE Storage Extent Package APIs.
 *
 *  @ingroup fbe_api_storage_extent_package_interface
 */ 
//----------------------------------------------------------------

    
//----------------------------------------------------------------
// Define the group for the FBE API Encryption Interface.  
// This is where all the function prototypes for the FBE API Encryption.
//----------------------------------------------------------------
/*! @defgroup fbe_api_power_save_interface FBE API Encryption Interface
 *  @brief 
 *    This is the set of FBE API Encryption Interface. 
 *
 *  @details 
 *
 *  @ingroup fbe_api_storage_extent_package_class
 *  @{
 */
//----------------------------------------------------------------
fbe_status_t FBE_API_CALL fbe_api_get_system_encryption_info(fbe_system_encryption_info_t *encryption_info_p);
fbe_status_t FBE_API_CALL fbe_api_enable_system_encryption(fbe_u64_t stamp1, fbe_u64_t stamp2);
fbe_status_t FBE_API_CALL fbe_api_disable_system_encryption(void);
fbe_status_t FBE_API_CALL fbe_api_clear_system_encryption(void);
fbe_status_t fbe_api_change_rg_encryption_mode(fbe_object_id_t object_id,
                                               fbe_base_config_encryption_mode_t mode);
fbe_status_t FBE_API_CALL fbe_api_encryption_check_block_encrypted(fbe_u32_t bus,
                                                                   fbe_u32_t enclosure,
                                                                   fbe_u32_t slot,
                                                                   fbe_lba_t pba,
                                                                   fbe_block_count_t blocks_to_check,
                                                                   fbe_block_count_t *num_encrypted);
fbe_status_t FBE_API_CALL fbe_api_process_encryption_keys(fbe_job_service_encryption_key_operation_t in_operation, fbe_encryption_key_table_t *keys_p);
fbe_status_t FBE_API_CALL fbe_api_get_hardware_ssv_data(fbe_encryption_hardware_ssv_t *ssv_p);

fbe_status_t FBE_API_CALL  fbe_api_set_system_encryption_info(fbe_system_encryption_info_t *encryption_info_p);
fbe_status_t FBE_API_CALL fbe_api_get_pvd_key_info(fbe_object_id_t pvd_object_id, fbe_provision_drive_get_key_info_t *key_info_p);

/*! @} */ 

FBE_API_CPP_EXPORT_END
//----------------------------------------------------------------
// Define the group for all FBE Storage Extent Package APIs Interface class files.  
// This is where all the class files that belong to the FBE API Storage Extent
// Package define. In addition, this group is displayed in the FBE Classes
// module.
//----------------------------------------------------------------
/*! @defgroup fbe_api_storage_extent_package_interface_class_files FBE Storage Extent Package APIs Interface Class Files 
 *  @brief 
 *    This is the set of files for the FBE Storage Extent Package APIs Interface.
 * 
 *  @ingroup fbe_api_classes
 */
//----------------------------------------------------------------

#endif /*FBE_API_ENCRYPTION_INTERFACE_H*/


