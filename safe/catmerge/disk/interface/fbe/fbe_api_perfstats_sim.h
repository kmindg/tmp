

#ifndef FBE_API_PERFSTATS_SIM_H
#define FBE_API_PERFSTATS_SIM_H

//-----------------------------------------------------------------------------
// Copyright (C) EMC Corporation 2012
// All rights reserved.
// Licensed material -- property of EMC Corporation
//
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
//
//  MODULE ABSTRACT: 
//
//  This header file defines the FBE API for the Perfstats service in simulation
//    
//-----------------------------------------------------------------------------

/*!**************************************************************************
 * @file fbe_api_perfstats_sim.h
 ***************************************************************************
 *
 * @brief
 *  This file contains definitions of functions that are exported by the 
 *  fbe_api_perfstats_interface, FOR SIMULATION ONLY
 * 
 * @ingroup fbe_api_storage_extent_package_interface_class_files
 * 
 * @version
 *   4/04/2012:  Created. Ryan Gadsby
 *
 ***************************************************************************/

//-----------------------------------------------------------------------------
//  INCLUDES OF REQUIRED HEADER FILES:
//
#include "fbe/fbe_types.h"
#include "fbe/fbe_transport.h"
#include "fbe/fbe_lun.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_perfstats_interface.h"

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
// Define the FBE API Block Transport Interface for the Usurper Interface. 
// This is where all the data structures defined. 
//----------------------------------------------------------------
/*! @defgroup fbe_api_perfstats_interface_usurper_interface FBE API CMI Interface Usurper Interface
 *  @brief 
 *    This is the set of definitions that comprise the FBE API CMI Interface class
 *    usurper interface.
 *
 *  @ingroup fbe_api_classes_usurper_interface
 *  @{
 */ 
//----------------------------------------------------------------
/*! @} */ /* end of group fbe_api_cmi_interface_usurper_interface */
//-----------------------------------------------------------------------------
//  FUNCTION PROTOTYPES: 

//----------------------------------------------------------------
// Define the group for the FBE API CMI Interface.  
// This is where all the function prototypes for the FBE API CMI.
//----------------------------------------------------------------
/*! @defgroup fbe_api_perfstats_interface FBE API Perfstats Interface
 *  @brief 
 *    This is the set of FBE API Perfstats Interface. 
 *
 *  @details 
 *    In order to access this library, please include fbe_api_perfstats_interface.h.
 *
 *  @ingroup fbe_api_storage_extent_package_class
 *  @{
 */
//----------------------------------------------------------------
fbe_status_t FBE_API_CALL fbe_api_perfstats_sim_get_statistics_container_for_package(fbe_u64_t *ptr, fbe_package_id_t package_id);

/*! @} */ /* end of group fbe_api_perfstats_interface */

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
 *  @ingroup fbe_classes
 */
//----------------------------------------------------------------

#endif /* FBE_API_PERFSTATS_INTERFACE_H */



