#ifndef FBE_API_POWER_SUPPLY_INTERFACE_H
#define FBE_API_POWER_SUPPLY_INTERFACE_H

/***************************************************************************
 * Copyright (C) EMC Corporation 2000-2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_api_power_supply_interface.h
 ***************************************************************************
 *
 * @brief
 *  This file contains the Power Supply APIs.
 * 
 * @ingroup fbe_api_power_supply_interface_class_files
 * 
 * @version
 *   07/27/10   Joe Perry    Created
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
 *  @ingroup fbe_api_power_supply_interface
 */ 
//----------------------------------------------------------------

//----------------------------------------------------------------
// Define the FBE API Power Supply Interface for the Usurper Interface. 
// This is where all the data structures defined. 
//----------------------------------------------------------------
/*! @defgroup fbe_api_power_supply_usurper_interface FBE API Power Supply Usurper Interface
 *  @brief 
 *    This is the set of definitions that comprise the FBE API Power Supply Interface class
 *    usurper interface.
 *
 *  @ingroup fbe_api_classes_usurper_interface
 *  @{
 */ 
//----------------------------------------------------------------

FBE_API_CPP_EXPORT_START

/*! @} */ /* end of fbe_api_power_supply_usurper_interface */

//----------------------------------------------------------------
// Define the group for the FBE API Power Supply Interface.  
// This is where all the function prototypes for the Power Supply API.
//----------------------------------------------------------------
/*! @defgroup fbe_api_power_supply_interface FBE API Power Supply Interface
 *  @brief 
 *    This is the set of FBE API Power Supply Interface.
 *
 *  @details 
 *    In order to access this library, please include fbe_api_power_supply_interface.h.
 *
 *  @ingroup fbe_physical_package_class
 *  @{
 */
//----------------------------------------------------------------
fbe_status_t FBE_API_CALL 
fbe_api_power_supply_get_power_supply_info(fbe_object_id_t object_id, 
                                           fbe_power_supply_info_t *ps_info);

fbe_status_t FBE_API_CALL 
fbe_api_power_supply_get_ps_count(fbe_object_id_t objectId, 
                                  fbe_u32_t * pPsCount);

/*! @} */ /* end of group fbe_api_power_supply_interface */

FBE_API_CPP_EXPORT_END

//----------------------------------------------------------------
// Define the group for all FBE Power Supply APIs Interface class files.  
// This is where all the class files that belong to the FBE API Power
// Supply define. In addition, this group is displayed in the FBE Classes
// module.
//----------------------------------------------------------------
/*! @defgroup fbe_api_power_supply_interface_class_files FBE Power Supply APIs Interface Class Files 
 *  @brief 
 *    This is the set of files for the FBE Power Supply Interface.
 *
 *  @ingroup fbe_api_classes
 */
//----------------------------------------------------------------

#endif /*FBE_API_POWER_SUPPLY_INTERFACE_H*/
