
#ifndef FBE_API_PERFSTATS_INTERFACE_H
#define FBE_API_PERFSTATS_INTERFACE_H

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
//  This header file defines the FBE API for the Perfstats service
//    
//-----------------------------------------------------------------------------

/*!**************************************************************************
 * @file fbe_api_perfstats_interface.h
 ***************************************************************************
 *
 * @brief
 *  This file contains definitions of functions that are exported by the 
 *  fbe_api_perfstats_interface.
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
#include "fbe/fbe_api_common_transport.h"

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
/*! @defgroup fbe_api_cmi_interface_usurper_interface FBE API CMI Interface Usurper Interface
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

/*!***************************************************************
 * @fn fbe_api_perfstats_get_statistics_container_for_package(fbe_u64_t *ptr, 
                                                              fbe_package_id_t package_id)
 ****************************************************************
 * @brief
 *  This function returns a user-space pointer to the performance statistics container
 * for the specified package (FBE_PACKAGE_ID_PHYSICAL or FBE_PACKAGE_ID_SEP_0). 
 *
 * The pointer is returned as an unsigned 64 bit integer, and has to be cast as a
 * pointer to an fbe_performance_container_t. This is to get around pointer incompatibilities with
 * the 32-bit libraries that this function is designed to be called by.
 *
 * @param ptr - pointer to the variable that will hold the address
 * @param package_id - which package to get the container from
 *
 * @return
 *  FBE_STATUS_GENERIC_FAILURE - Mapping failed or statistics container unallocated
 *  FBE_STATUS_OK - returned pointer is valid
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_perfstats_get_statistics_container_for_package(fbe_u64_t *ptr,
                                                                                 fbe_package_id_t package_id);

/*!***************************************************************
 * @fn fbe_api_perfstats_zero_statistics_for_package(fbe_package_id_t package_id)
 ****************************************************************
 * @brief
 *  This function zeroes all the data in the performance statistics container for
 *  the provided package. This will always be called whenever statistics are enabled.
 *
 * @param package_id - which package to zero the container for
 *
 * @return
 *  FBE_STATUS_GENERIC_FAILURE - statistics container unallocated
 *  FBE_STATUS_OK - zeroing successful
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_perfstats_zero_statistics_for_package(fbe_package_id_t package_id);

/*!***************************************************************
 * @fn fbe_api_perfstats_enable_statistics_for_package(fbe_package_id_t package_id)
 ****************************************************************
 * @brief
 * This function zeroes the performance struct and enables statistics for every object
 * for which we can collect stats for in the provided package.
 *
 * @param package_id - which package to enable statistics for
 *
 * @return
 *  FBE_STATUS_GENERIC_FAILURE - statistics enabling failed
 *  FBE_STATUS_OK - every collectable object in the package has collection enabled
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_perfstats_enable_statistics_for_package(fbe_package_id_t package_id);

/*!***************************************************************
 * @fn fbe_api_perfstats_disable_statistics_for_package(fbe_package_id_t package_id)
 ****************************************************************
 * @brief
 * This function disables statistics for every object
 * for which we can collect stats for in the provided package.
 *
 * @param package_id - which package to disable statistics for
 *
 * @return
 *  FBE_STATUS_GENERIC_FAILURE - statistics disabling failed
 *  FBE_STATUS_OK - every collectable object in the package has collection disabled
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_perfstats_disable_statistics_for_package(fbe_package_id_t package_id);

/*!***************************************************************
 * @fn fbe_status_t FBE_API_CALL fbe_api_perfstats_get_offset_for_object(fbe_object_id_t object_id,
                                                                  fbe_package_id_t package_id,
                                                                  fbe_u32_t *offset)
 ****************************************************************
 * @brief
 * Within the performance struct there is an array that holds the per-object stats for that package.
 *
 * This function returns the offset within the performance statistics container for
 * a given package the per-object statistics for the object with the provided object_id.
 *
 * Offsets are persistant. Even if an object is destroyed and another object is created with
 * the same object ID, it will use the same offset within the container.
 *
 * @param object_id - which object we're trying to find the offset for.
 * @param package_id - which package to look in
 * @param *offset - returned offset
 *
 * @return
 *  FBE_STATUS_GENERIC_FAILURE - object isn't in internal map, so there's no stats memory for it.
 *  FBE_STATUS_OK - offset is valid
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_perfstats_get_offset_for_object(fbe_object_id_t object_id,
                                                                  fbe_package_id_t package_id,
                                                                  fbe_u32_t *offset);

/*!***************************************************************
 * @fn fbe_status_t FBE_API_CALL fbe_api_perfstats_is_statistics_collection_enabled_for_package(fbe_package_id_t package_id,
                                                                                                fbe_bool_t *is_enabled
 ****************************************************************
 * @brief
 * Checks to see if package-wide statistics enabling has been toggled.
 *
 * @param package_id - which package to look in
 * @param *is_enabled - whether or not collection is enabled
 *
 * @return
 *  FBE_STATUS_GENERIC_FAILURE - Something failed sending the packet to the service, or there's an unsupported package_id
 *  FBE_STATUS_OK - check successful
 ****************************************************************/

fbe_status_t FBE_API_CALL fbe_api_perfstats_is_statistics_collection_enabled_for_package(fbe_package_id_t package_id,
                                                                                         fbe_bool_t *is_enabled);

/*!***************************************************************
 * @fn fbe_status_t FBE_API_CALL fbe_api_perfstats_get_summed_lun_stats(fbe_perfstats_lun_sum_t *summed_stats,
                                                                 fbe_lun_performance_counters_t *lun_stats)
 ****************************************************************
 * @brief
 * This is a function that sums all of our per-core counters for a given LUN and
 * puts them in an aggregate struct.
 *
 * @param *summed stats - pointer to the aggregate stats constainer for a single object.
 * @param *lun_stats - pointer to the lun stats we wish to sum
 *
 * @return
 *  FBE_STATUS_GENERIC_FAILURE - one of the input pointers was null
 *  FBE_STATUS_OK - counters successfully summed
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_perfstats_get_summed_lun_stats(fbe_perfstats_lun_sum_t *summed_stats,
                                                                 fbe_lun_performance_counters_t *lun_stats);

/*!***************************************************************
 * @fn fbe_status_t FBE_API_CALL fbe_api_perfstats_get_summed_pdo_stats(fbe_perfstats_pdo_sum_t *summed_stats,
                                                                 fbe_pdo_performance_counters_t *pdo_stats)
 ****************************************************************
 * @brief
 * This is a function that sums all of our per-core counters for a given PDO and
 * puts them in an aggregate struct.
 *
 * @param *summed stats - pointer to the aggregate stats constainer for a single object.
 * @param *pdo_stats - pointer to the pdo stats we wish to sum
 *
 * @return
 *  FBE_STATUS_GENERIC_FAILURE - one of the input pointers was null
 *  FBE_STATUS_OK - counters successfully summed.
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_perfstats_get_summed_pdo_stats(fbe_perfstats_pdo_sum_t *summed_stats,
                                                                 fbe_pdo_performance_counters_t *pdo_stats);
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



