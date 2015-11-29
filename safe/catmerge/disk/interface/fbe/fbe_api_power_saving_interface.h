#ifndef FBE_API_POWER_SAVING_INTERFACE_H
#define FBE_API_POWER_SAVING_INTERFACE_H

/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_api_power_saving_interface.h
 ***************************************************************************
 *
 * @brief
 *  This header file defines the FBE API for power saving control
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
#include "fbe/fbe_power_save_interface.h"
#include "fbe/fbe_base_config.h"
#include "fbe/fbe_database_packed_struct.h"

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
// Define the group for the FBE API Power save Interface.  
// This is where all the function prototypes for the FBE API Power saving.
//----------------------------------------------------------------
/*! @defgroup fbe_api_power_save_interface FBE API Power Saving Interface
 *  @brief 
 *    This is the set of FBE API Power Saving Interface. 
 *
 *  @details 
 *    In order to access this library, please include fbe_api_provision_drive_interface.h.
 *
 *  @ingroup fbe_api_storage_extent_package_class
 *  @{
 */
//----------------------------------------------------------------
fbe_status_t FBE_API_CALL fbe_api_get_system_power_save_info(fbe_system_power_saving_info_t * power_save_info);
fbe_status_t FBE_API_CALL fbe_api_enable_system_power_save(void);
fbe_status_t FBE_API_CALL fbe_api_disable_system_power_save(void);
fbe_status_t FBE_API_CALL fbe_api_set_power_save_periodic_wakeup(fbe_u64_t minutes);
fbe_status_t FBE_API_CALL fbe_api_enable_object_power_save(fbe_object_id_t object_id);
fbe_status_t FBE_API_CALL fbe_api_disable_object_power_save(fbe_object_id_t object_id);
fbe_status_t FBE_API_CALL fbe_api_get_object_power_save_info(fbe_object_id_t object_id, fbe_base_config_object_power_save_info_t *info);
fbe_status_t FBE_API_CALL fbe_api_set_object_power_save_idle_time(fbe_object_id_t object_id, fbe_u64_t idle_time_in_seconds);
fbe_status_t FBE_API_CALL fbe_api_enable_raid_group_power_save(fbe_object_id_t rg_object_id);
fbe_status_t FBE_API_CALL fbe_api_disable_raid_group_power_save(fbe_object_id_t rg_object_id);
fbe_status_t FBE_API_CALL fbe_api_set_raid_group_power_save_idle_time(fbe_object_id_t rg_object_id, fbe_u64_t idle_time_in_seconds);
fbe_status_t FBE_API_CALL fbe_api_enable_system_power_save_statistics(void);
fbe_status_t FBE_API_CALL fbe_api_disable_system_power_save_statistics(void);

/*! @} */ /* end of group fbe_api_provision_drive_interface */

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

#endif /*FBE_API_POWER_SAVING_INTERFACE_H*/


