#ifndef FBE_API_PROTOCOL_ERROR_INJECTION_INTERFACE_H
#define FBE_API_PROTOCOL_ERROR_INJECTION_INTERFACE_H

/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_api_protocol_error_injection_interface.h
 ***************************************************************************
 *
 * @brief
 *  This file contains definitions of functions that are exported by the 
 *  fbe_api_protocol_error_injection_interface.c.
 * 
 * @ingroup fbe_api_neit_package_interface_class_files
 * 
 * @version
 *   10/10/08    sharel - Created
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_types.h"
#include "fbe/fbe_transport.h"
#include "fbe/fbe_protocol_error_injection_service.h"
#include "fbe/fbe_api_common.h"

FBE_API_CPP_EXPORT_START

//----------------------------------------------------------------
// Define the top level group for the FBE NEIT Package APIs 
//----------------------------------------------------------------
/*! @defgroup fbe_api_neit_package_class FBE NEIT Package APIs 
 *  @brief 
 *    This is the set of definitions for FBE NEIT Package APIs.
 * 
 *  @ingroup fbe_api_neit_package_interface
 */ 
//----------------------------------------------------------------

//----------------------------------------------------------------
// Define the FBE API Protocal Error Injection Interface for the Usurper Interface. 
// This is where all the data structures defined. 
//----------------------------------------------------------------
/*! @defgroup fbe_api_protocol_error_injection_usurper_interface FBE API Protocol Error Injection Usurper Interface
 *  @brief 
 *    This is the set of definitions that comprise the FBE API Protocol Error Injection class
 *    usurper interface.
 *  
 *  @ingroup fbe_api_classes_usurper_interface
 *  @{
 */ 
//----------------------------------------------------------------
/*! @} */ /* end of group fbe_api_protocol_error_injection_usurper_interface */

//----------------------------------------------------------------
// Define the group for the FBE API Protocol Error Injection Interface.  
// This is where all the function prototypes for the Protocol Error Injection API.
//----------------------------------------------------------------
/*! @defgroup fbe_api_protocol_error_injection_interface FBE API Protocol Error Injection Interface
 *  @brief 
 *    This is the set of FBE API Protocol Error Injection Interface.
 *
 *  @details 
 *    In order to access this library, please include fbe_api_protocol_error_injection_interface.h.
 *
 *  @ingroup fbe_api_neit_package_class
 *  @{
 */
//----------------------------------------------------------------
fbe_status_t FBE_API_CALL
fbe_api_protocol_error_injection_add_record(fbe_protocol_error_injection_error_record_t *new_record,
                                            fbe_protocol_error_injection_record_handle_t * record_handle);
fbe_status_t FBE_API_CALL
fbe_api_protocol_error_injection_remove_record(fbe_protocol_error_injection_record_handle_t record_handle);
fbe_status_t FBE_API_CALL
fbe_api_protocol_error_injection_remove_object(fbe_object_id_t object_id);
fbe_status_t FBE_API_CALL
fbe_api_protocol_error_injection_start(void);
fbe_status_t FBE_API_CALL
fbe_api_protocol_error_injection_stop(void);
fbe_status_t FBE_API_CALL 
fbe_api_protocol_error_injection_get_record(fbe_protocol_error_injection_record_handle_t handle_p, fbe_protocol_error_injection_error_record_t * record_p);
fbe_status_t FBE_API_CALL 
fbe_api_protocol_error_injection_get_record_next(fbe_protocol_error_injection_record_handle_t handle_p, fbe_protocol_error_injection_error_record_t * record_p);
fbe_status_t FBE_API_CALL 
fbe_api_protocol_error_injection_get_record_handle(fbe_protocol_error_injection_error_record_t* record_p, fbe_protocol_error_injection_record_handle_t* record_handle);
fbe_status_t FBE_API_CALL
fbe_api_protocol_error_injection_record_update_times_to_insert(fbe_protocol_error_injection_record_handle_t record_handle, fbe_protocol_error_injection_error_record_t *record_p);
fbe_status_t FBE_API_CALL
fbe_api_protocol_error_injection_remove_all_records(void);

/*! @} */ /* end of group fbe_api_protocol_error_injection_interface */

//----------------------------------------------------------------
// Define the group for all FBE NEIT Package APIs Interface class files.  
// This is where all the class files that belong to the FBE API NEIT 
// Package define. In addition, this group is displayed in the FBE Classes
// module.
//----------------------------------------------------------------
/*! @defgroup fbe_api_neit_package_interface_class_files FBE NEIT Package APIs Interface Class Files 
 *  @brief 
 *    This is the set of files for the FBE NEIT Package Interface.
 *
 *  @ingroup fbe_api_classes
 */
//----------------------------------------------------------------

FBE_API_CPP_EXPORT_END

#endif /*FBE_API_PROTOCOL_ERROR_INJECTION_INTERFACE_H*/


