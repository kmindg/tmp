#ifndef FBE_API_PVD_REMOVED_TIME_THRESHOLD_INTERFACE_H
#define FBE_API_PVD_REMOVED_TIME_THRESHOLD_INTERFACE_H

/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_api_system_time_threshold_interface.h
 ***************************************************************************
 *
 * @brief
 *  This header file defines the FBE API for time threshold control
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
/*! @defgroup fbe_api_system_time_threshold_interface FBE API Power Saving Interface
 *  @brief 
 *    This is the set of FBE API PVD removed time threshold Interface. 
 *
 *  @details 
 *
 *  @ingroup fbe_api_storage_extent_package_class
 *  @{
 */
//----------------------------------------------------------------
fbe_status_t FBE_API_CALL fbe_api_get_system_time_threshold_info(fbe_system_time_threshold_info_t * time_threshold_info);
fbe_status_t FBE_API_CALL fbe_api_set_system_time_threshold_info(fbe_system_time_threshold_info_t * time_threshold_info);

/*! @} */ /* end of group fbe_api_system_time_threshold_interface */

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

#endif /*FBE_API_PVD_REMOVED_TIME_THRESHOLD_INTERFACE_H*/



