
#ifndef FBE_API_SYSTEM_BG_SERVICE_INTERFACE_H
#define FBE_API_SYSTEM_BG_SERVICE_INTERFACE_H

/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_api_system_bg_service_interface.h
 ***************************************************************************
 *
 * @brief
 *  This header file defines the FBE API for system background service control
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
/*! @defgroup fbe_api_system_bg_service_interface FBE API SYSTEM BG SERVICE Interface
 *  @brief 
 *    This is the set of FBE API SYSTEM BG SERVICE Interface. 
 *
 *  @details 
 *    In order to access this library, please include fbe_api_provision_drive_interface.h.
 *
 *  @ingroup fbe_api_storage_extent_package_class
 *  @{
 */
//----------------------------------------------------------------
fbe_status_t fbe_api_get_system_bg_service_status(fbe_base_config_control_system_bg_service_t * system_bg_service);
fbe_status_t FBE_API_CALL fbe_api_enable_system_sniff_verify(void);
fbe_status_t FBE_API_CALL fbe_api_disable_system_sniff_verify(void);
fbe_bool_t FBE_API_CALL fbe_api_system_sniff_verify_is_enabled(void);
fbe_status_t FBE_API_CALL fbe_api_enable_all_bg_services(void);
fbe_status_t FBE_API_CALL fbe_api_disable_all_bg_services(void);
fbe_bool_t FBE_API_CALL fbe_api_all_bg_services_are_enabled(void);
fbe_status_t FBE_API_CALL fbe_api_enable_all_bg_services_single_sp(void);
fbe_status_t FBE_API_CALL fbe_api_disable_all_bg_services_single_sp(void);
fbe_status_t FBE_API_CALL fbe_api_control_system_bg_service(fbe_base_config_control_system_bg_service_t * system_bg_service);



/*! @} */ /* end of group fbe_api_system_bg_service_interface */

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

#endif /*FBE_API_SYSTEM_BG_SERVICE_INTERFACE_H*/


