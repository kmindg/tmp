#ifndef FBE_API_SYSTEM_INTERFACE_H
#define FBE_API_SYSTEM_INTERFACE_H

//-----------------------------------------------------------------------------
// Copyright (C) EMC Corporation 2009
// All rights reserved.
// Licensed material -- property of EMC Corporation
//
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
//
//  MODULE ABSTRACT: 
//
//  This header file defines the FBE API for the lun object.
//    
//-----------------------------------------------------------------------------

/*!**************************************************************************
 * @file fbe_api_system_interface.h
 ***************************************************************************
 *
 * @brief
 *  This header file defines the FBE API for the whole system.
 * 
 * @ingroup fbe_api_storage_extent_package_interface_class_files
 * 
 *
 ***************************************************************************/

//-----------------------------------------------------------------------------
//  INCLUDES OF REQUIRED HEADER FILES:
//
#include "fbe/fbe_types.h"
#include "fbe/fbe_transport.h"
#include "fbe/fbe_lun.h"
#include "fbe/fbe_api_common.h"
#include "fbe_job_service.h"



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
// Define the FBE API SYSTEM Interface for the Usurper Interface. 
// This is where all the data structures defined. 
//----------------------------------------------------------------
/*! @defgroup fbe_api_system_interface_usurper_interface FBE API SYSTEM Interface Usurper Interface
 *  @brief 
 *    This is the set of definitions that comprise the FBE API SYSTEM Interface class
 *    usurper interface.
 *
 *  @ingroup fbe_api_classes_usurper_interface
 *  @{
 */ 
//----------------------------------------------------------------

FBE_API_CPP_EXPORT_START

//-----------------------------------------------------------------------------
//  NAMED CONSTANTS (#defines):


//-----------------------------------------------------------------------------
//  ENUMERATIONS:


//-----------------------------------------------------------------------------
//  TYPEDEFS: 
//

/*!*******************************************************************
 * @struct fbe_api_system_get_failure_info_t
 *********************************************************************
 * @brief
 *  This is the LUN capacity data structure. This structure is used by
 *  FBE API to get imported capacity of lun.
 *
 * @ingroup fbe_api_system_interface
 * @ingroup fbe_api_system_get_failure_info
 *********************************************************************/
typedef struct fbe_api_system_get_failure_info_s {
    fbe_api_trace_error_counters_t	error_counters[FBE_PACKAGE_ID_LAST];/*one per pacakge*/
}fbe_api_system_get_failure_info_t;


//-----------------------------------------------------------------------------
//  FUNCTION PROTOTYPES: 

//----------------------------------------------------------------
// Define the group for the FBE API SYSTEM Interface.  
// This is where all the function prototypes for the FBE API LUN.
//----------------------------------------------------------------
/*! @defgroup fbe_api_system_interface FBE API SYSTEM Interface
 *  @brief 
 *    This is the set of FBE API LUN Interface. 
 *
 *  @details 
 *    In order to access this library, please include fbe_api_system_interface.h.
 *
 *  @ingroup fbe_api_storage_extent_package_class
 *  @{
 */
//----------------------------------------------------------------
fbe_status_t FBE_API_CALL fbe_api_system_reset_failure_info(fbe_package_notification_id_mask_t package_mask);
fbe_status_t FBE_API_CALL fbe_api_system_get_failure_info(fbe_api_system_get_failure_info_t *err_info_ptr);

/*! @} */ /* end of group fbe_api_system_interface */

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

#endif /* FBE_API_SYSTEM_INTERFACE_H */


