#ifndef FBE_API_CMS_INTERFACE_H
#define FBE_API_CMS_INTERFACE_H

/***************************************************************************
 * Copyright (C) EMC Corporation 2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_api_cms_interface.h
 ***************************************************************************
 *
 * @brief
 *  This file contains definitions of functions that are exported by the 
 *  fbe_api_persistent_interface.
 * 
 * @ingroup fbe_api_storage_extent_package_interface_class_files
 * 
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_types.h"
#include "fbe/fbe_api_common.h"
#include "fbe_cms.h"
#include "fbe/fbe_cms_interface.h"


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
// Define the FBE API Persistant Memory Interface for the Usurper Interface. 
// This is where all the data structures defined. 
//----------------------------------------------------------------
/*! @defgroup fbe_api_cms_interface_usurper_interface FBE API database Usurper Interface
 *  @brief 
 *    This is the set of definitions that comprise the FBE API persistent memory class
 *    usurper interface.
 *
 *  @ingroup fbe_api_classes_usurper_interface
 *  @{
 */ 
//----------------------------------------------------------------

FBE_API_CPP_EXPORT_START

/*! @} */ /* end of group fbe_api_database_interface_usurper_interface */

//----------------------------------------------------------------
// Define the group for the FBE API database Interface.  
// This is where all the function prototypes for the FBE API database.
//----------------------------------------------------------------
/*! @defgroup fbe_api_cms_interface FBE API Persistant Memory Interface
 *  @brief 
 *    This is the set of FBE API for persistent memory. 
 *
 *  @details 
 *    In order to access this library, please include fbe_api_cms_interface.h.
 *
 *  @ingroup fbe_api_storage_extent_package_class
 *  @{
 */
//----------------------------------------------------------------


fbe_status_t FBE_API_CALL fbe_api_cms_get_info(fbe_cms_get_info_t *get_info);
fbe_status_t FBE_API_CALL fbe_api_cms_service_get_persistence_status(fbe_cms_memory_persist_status_t *status_p);
fbe_status_t FBE_API_CALL fbe_api_cms_service_request_persistence(fbe_cms_client_id_t client_id, fbe_bool_t request);
fbe_status_t FBE_API_CALL fbe_api_cms_get_state_machine_history(fbe_cms_control_get_sm_history_t *history);


/*! @} */ /* end of group fbe_api_cms_interface */

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
FBE_API_CPP_EXPORT_END

#endif /*FBE_API_CMS_INTERFACE_H*/


