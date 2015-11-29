#ifndef FBE_API_ENVIRONMENT_LIMIT_INTERFACE_H
#define FBE_API_ENVIRONMENT_LIMIT_INTERFACE_H
/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_api_environment_limit_interface.h
 ***************************************************************************
 *
 * @brief
 *  This file contains declaration of API use to interface with environment limit service.
 *  
 * @ingroup fbe_api_system_package_interface_class_files
 *
 * @version
 *   25 August - 2010 - Vishnu Sharma Created
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_types.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_environment_limit.h"

//------------------------------------------------------------
//  Define the top level group for the FBE System APIs.
//------------------------------------------------------------
/*! @defgroup fbe_system_package_class FBE System APIs
 *  @brief   
 *    This is the set of definations for FBE System APIs.
 *  
 *  @ingroup fbe_api_system_package_interface
 */
//---------------------------------------------------------------- 
//----------------------------------------------------------------
// Define the FBE API System Interface for the Usurper Interface. 
// This is where all the data structures defined. 
//----------------------------------------------------------------
/*! @defgroup fbe_api_system_interface_usurper_interface FBE API System Usurper Interface
 *  @brief 
 *    This is the set of definitions that comprise the FBE API System class
 *    usurper interface.
 *
 *  @ingroup fbe_api_classes_usurper_interface
 *  @{
 */ 
//----------------------------------------------------------------

/*use this macro and the closing one: FBE_API_CPP_EXPORT_END to export your functions to C++ applications*/
FBE_API_CPP_EXPORT_START
//------------------------------------------------------------
// Define the group for the environment limit service interface APIs.
//------------------------------------------------------------
/*! @defgroup fbe_api_environment_limit_service_interface FBE API Environment Limit Service Interface
 *  @brief   
 *    This is the set of FBE API for environment limit services.
 *  
 *  @details
 *    In order to access this library, please include fbe_api_environment_limit_interface.h.  
 *  
 *  @ingroup fbe_system_package_class
 *  @{  
 */
//---------------------------------------------------------------- 
fbe_status_t FBE_API_CALL fbe_api_get_environment_limits(fbe_environment_limits_t *env_limits,fbe_package_id_t package_id);
fbe_status_t FBE_API_CALL fbe_api_set_environment_limits(fbe_environment_limits_t *env_limits,fbe_package_id_t package_id);

/*! @} */ /* end of group fbe_api_environment_limit_service_interface */
FBE_API_CPP_EXPORT_END

//----------------------------------------------------------------
// Define the group for the FBE API System Interface class files. 
// This is where all the class files that belong to the FBE API Physical 
// Package define. 
//----------------------------------------------------------------
/*! @defgroup fbe_api_system_package_interface_class_files FBE System APIs Interface Class Files 
 *  @brief 
 *     This is the set of files for the FBE System Package Interface.
 * 
 *  @ingroup fbe_api_classes
 */
//----------------------------------------------------------------

#endif  /*FBE_API_ENVIRONMENT_LIMIT_INTERFACE_H*/
