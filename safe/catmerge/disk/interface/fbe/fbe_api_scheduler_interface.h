#ifndef FBE_API_SCHEDULER_INTERFACE_H
#define FBE_API_SCHEDULER_INTERFACE_H

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
//  This header file defines the FBE API for the scheduler object.
//    
//-----------------------------------------------------------------------------

/*!**************************************************************************
 * @file fbe_api_scheduler_interface.h
 ***************************************************************************
 *
 * @brief
 *  This header file defines the FBE API for the scheduler object.
 * 
 * @ingroup fbe_api_storage_extent_package_interface_class_files
 * 
 *
 ***************************************************************************/

//-----------------------------------------------------------------------------
//  INCLUDES OF REQUIRED HEADER FILES:
//
#include "fbe/fbe_types.h"
#include "fbe/fbe_scheduler_interface.h"
#include "fbe/fbe_api_common.h"
#include "fbe_testability.h"

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
// Define the FBE API SCHEDULER Interface for the Usurper Interface. 
// This is where all the data structures defined. 
//----------------------------------------------------------------
/*! @defgroup fbe_api_scheduler_interface_usurper_interface FBE API SCHEDULER Interface Usurper Interface
 *  @brief 
 *    This is the set of definitions that comprise the FBE API SCHEDULER Interface class
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

//-----------------------------------------------------------------------------
//  MACROS:


//-----------------------------------------------------------------------------
//  FUNCTION PROTOTYPES: 

//----------------------------------------------------------------
// Define the group for the FBE API SCHEDULER Interface.  
// This is where all the function prototypes for the FBE API SCHEDULER.
//----------------------------------------------------------------
/*! @defgroup fbe_api_scheduler_interface FBE API SCHEDULER Interface
 *  @brief 
 *    This is the set of FBE API SCHEDULER Interface. 
 *
 *  @details 
 *    In order to access this library, please include fbe_api_scheduler_interface.h.
 *
 *  @ingroup fbe_api_storage_extent_package_class
 *  @{
 */
//----------------------------------------------------------------
fbe_status_t FBE_API_CALL fbe_api_scheduler_set_credits(fbe_scheduler_set_credits_t *credits);
fbe_status_t FBE_API_CALL fbe_api_scheduler_get_credits(fbe_scheduler_set_credits_t *credits);
fbe_status_t FBE_API_CALL fbe_api_scheduler_remove_cpu_credits_per_core(fbe_scheduler_change_cpu_credits_per_core_t *credits);
fbe_status_t FBE_API_CALL fbe_api_scheduler_add_cpu_credits_per_core(fbe_scheduler_change_cpu_credits_per_core_t *credits);
fbe_status_t FBE_API_CALL fbe_api_scheduler_remove_cpu_credits_all_cores(fbe_scheduler_change_cpu_credits_all_cores_t *credits);
fbe_status_t FBE_API_CALL fbe_api_scheduler_add_cpu_credits_all_cores(fbe_scheduler_change_cpu_credits_all_cores_t *credits);
fbe_status_t FBE_API_CALL fbe_api_scheduler_add_memory_credits(fbe_scheduler_change_memory_credits_t *credits);
fbe_status_t FBE_API_CALL fbe_api_scheduler_remove_memory_credits(fbe_scheduler_change_memory_credits_t *credits);
fbe_status_t FBE_API_CALL fbe_api_scheduler_set_scale(fbe_scheduler_set_scale_t *set_scale);
fbe_status_t FBE_API_CALL fbe_api_scheduler_get_scale(fbe_scheduler_set_scale_t *get_scale);
fbe_status_t FBE_API_CALL fbe_api_scheduler_add_debug_hook(fbe_object_id_t object_id, fbe_u32_t monitor_state, fbe_u32_t monitor_substate, fbe_u64_t val1, fbe_u64_t val2, fbe_u32_t check_type, fbe_u32_t action);
fbe_status_t FBE_API_CALL fbe_api_scheduler_add_debug_hook_pp(fbe_object_id_t object_id, fbe_u32_t monitor_state, fbe_u32_t monitor_substate, fbe_u64_t val1, fbe_u64_t val2, fbe_u32_t check_type, fbe_u32_t action);
fbe_status_t FBE_API_CALL fbe_api_scheduler_get_debug_hook(fbe_scheduler_debug_hook_t *hook);
fbe_status_t FBE_API_CALL fbe_api_scheduler_get_debug_hook_pp(fbe_scheduler_debug_hook_t *hook);
fbe_status_t FBE_API_CALL fbe_api_scheduler_del_debug_hook(fbe_object_id_t object_id, fbe_u32_t monitor_state, fbe_u32_t monitor_substate, fbe_u64_t val1, fbe_u64_t val2, fbe_u32_t check_type, fbe_u32_t action);
fbe_status_t FBE_API_CALL fbe_api_scheduler_del_debug_hook_pp(fbe_object_id_t object_id, fbe_u32_t monitor_state, fbe_u32_t monitor_substate, fbe_u64_t val1, fbe_u64_t val2, fbe_u32_t check_type, fbe_u32_t action);
fbe_status_t FBE_API_CALL fbe_api_scheduler_clear_all_debug_hooks(fbe_scheduler_debug_hook_t *hook);
fbe_status_t FBE_API_CALL fbe_api_scheduler_clear_all_debug_hooks_pp(fbe_scheduler_debug_hook_t *hook);

/*! @} */ /* end of group fbe_api_scheduler_interface */

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

#endif /* FBE_API_SCHEDULER_INTERFACE_H */



