#ifndef FBE_API_EXTENT_POOL_INTERFACE_H
#define FBE_API_EXTENT_POOL_INTERFACE_H

/***************************************************************************
 * Copyright (C) EMC Corporation 2014
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_api_extent_pool_interface.h
 ***************************************************************************
 *
 * @brief
 *  This file contains definitions of functions that are exported by the 
 *  fbe_api_extent_pool_interface.
 * 
 * @ingroup fbe_api_storage_extent_package_interface_class_files
 * 
 * @version
 *  6/13/2014 - Created. Rob Foley
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_types.h"
#include "fbe/fbe_transport.h"
#include "fbe/fbe_extent_pool.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_lun.h"
#include "fbe_database.h"
#include "fbe_job_service.h"

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
// Define the FBE API Extent Pool  for the Usurper Interface.
// This is where all the data structures defined.
//----------------------------------------------------------------
/*! @defgroup fbe_api_extent_pool_interface_usurper_interface FBE API Extent Pool Usurper Interface
 *  @brief 
 *    This is the set of definitions that comprise the FBE API Extent Pool class
 *    usurper interface.
 *
 *  @ingroup fbe_api_classes_usurper_interface
 *  @{
 */ 
//----------------------------------------------------------------


/*! @} */ /* end of group fbe_api_extent_pool_interface_usurper_interface */





//----------------------------------------------------------------
// Define the group for the FBE API Extent Pool Interface.
// This is where all the function prototypes for the FBE API extent pool
//----------------------------------------------------------------
/*! @defgroup fbe_api_extent_pool_interface FBE API Extent Pool Interface
 *  @brief 
 *    This is the set of FBE API Extent Pool Interface. 
 *
 *  @details 
 *    In order to access this library, please include fbe_api_extent_pool_interface.h.
 *
 *  @ingroup fbe_api_storage_extent_package_class
 *  @{
 */
//----------------------------------------------------------------
fbe_status_t FBE_API_CALL
fbe_api_extent_pool_get_info(fbe_object_id_t object_id, 
                             fbe_extent_pool_get_info_t * info_p);

fbe_status_t FBE_API_CALL fbe_api_extent_pool_class_set_options(fbe_extent_pool_class_set_options_t * set_options_p,
                                                                fbe_class_id_t class_id);
/*! @} */ /* end of group fbe_api_extent_pool_interface */


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

#endif /*FBE_API_EXTENT_POOL_INTERFACE_H*/


