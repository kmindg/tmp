#ifndef FBE_API_COOLING_INTERFACE_H
#define FBE_API_COOLING_INTERFACE_H

/***************************************************************************
 * Copyright (C) EMC Corporation 2000-2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_api_cooling_interface.h
 ***************************************************************************
 *
 * @brief
 *  This file contains the Cooling APIs.
 * 
 * @ingroup fbe_api_cooling_interface_class_files
 * 
 * @version
 *   17-Jan-2011: PHE - Created.
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_types.h"
#include "fbe/fbe_enclosure.h"
#include "fbe/fbe_eses.h"
#include "fbe/fbe_api_common.h"

//----------------------------------------------------------------
// Define the top level group for the FBE Physical Package APIs 
//----------------------------------------------------------------
/*! @defgroup fbe_physical_package_class FBE Physical Package APIs 
 *  @brief This is the set of definitions for FBE Physical Package APIs.
 *  @ingroup fbe_api_cooling_interface
 */ 
//----------------------------------------------------------------

//----------------------------------------------------------------
// Define the FBE API Cooling Interface for the Usurper Interface. 
// This is where all the data structures defined. 
//----------------------------------------------------------------
/*! @defgroup fbe_api_power_supply_usurper_interface FBE API Cooling Usurper Interface
 *  @brief 
 *    This is the set of definitions that comprise the FBE API Cooling Interface class
 *    usurper interface.
 *
 *  @ingroup fbe_api_classes_usurper_interface
 *  @{
 */ 
//----------------------------------------------------------------

FBE_API_CPP_EXPORT_START

/*! @} */ /* end of fbe_api_cooling_usurper_interface */

//----------------------------------------------------------------
// Define the group for the FBE API Cooling Interface.  
// This is where all the function prototypes for the Cooling API.
//----------------------------------------------------------------
/*! @defgroup fbe_api_cooling_interface FBE API Cooling Interface
 *  @brief 
 *    This is the set of FBE API Cooling Interface.
 *
 *  @details 
 *    In order to access this library, please include fbe_api_cooling_interface.h.
 *
 *  @ingroup fbe_physical_package_class
 *  @{
 */
//----------------------------------------------------------------
fbe_status_t FBE_API_CALL fbe_api_cooling_get_fan_info(fbe_object_id_t objectId, 
                                       fbe_cooling_info_t * pFanInfo);

fbe_status_t FBE_API_CALL fbe_api_cooling_get_fan_count(fbe_object_id_t objectId, 
                              fbe_u32_t * pFanCount);

/*! @} */ /* end of group fbe_api_cooling_interface */

FBE_API_CPP_EXPORT_END

//----------------------------------------------------------------
// Define the group for all FBE Cooling APIs Interface class files.  
// This is where all the class files that belong to the FBE API Cooling
// define. In addition, this group is displayed in the FBE Classes
// module.
//----------------------------------------------------------------
/*! @defgroup fbe_api_cooling_interface_class_files FBE Cooling APIs Interface Class Files 
 *  @brief 
 *    This is the set of files for the FBE Cooling Interface.
 *
 *  @ingroup fbe_api_classes
 */
//----------------------------------------------------------------

#endif /*FBE_API_COOLING_INTERFACE_H*/

