
#ifndef FBE_API_DEST_INJECTION_INTERFACE_H
#define FBE_API_DEST_INJECTION_INTERFACE_H

/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_api_dest_injection_interface.h
 ***************************************************************************
 *
 * @brief
 *  This file contains definitions of functions that are exported by the 
 *  fbe_api_dest_injection_interface.c.
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
#include "fbe/fbe_dest_service.h"
#include "fbe/fbe_api_common.h"

FBE_API_CPP_EXPORT_START

//----------------------------------------------------------------
// Define the group for the FBE API DEST Injection Interface.  
// This is where all the function prototypes for the DEST Injection API.
//----------------------------------------------------------------
/*! @defgroup fbe_api_dest_injection_interface FBE API DEST Injection Interface
 *  @brief 
 *    This is the set of FBE API DEST Injection Interface.
 *
 *  @details 
 *    In order to access this library, please include fbe_api_dest_injection_interface.h.
 *
 *  @ingroup fbe_api_neit_package_class
 *  
 */
//----------------------------------------------------------------
fbe_status_t FBE_API_CALL  fbe_api_dest_injection_add_record(fbe_dest_error_record_t *new_record, fbe_dest_record_handle_t * record_handle);
fbe_status_t FBE_API_CALL  fbe_api_dest_injection_remove_record(fbe_dest_record_handle_t * record_handle);
fbe_status_t FBE_API_CALL  fbe_api_dest_injection_remove_all_records(void);
fbe_status_t FBE_API_CALL  fbe_api_dest_injection_get_record( fbe_dest_error_record_t * record_p, fbe_dest_record_handle_t handle_p);
fbe_status_t FBE_API_CALL  fbe_api_dest_injection_get_record_handle(fbe_dest_error_record_t* record_p, fbe_dest_record_handle_t* record_handle);
fbe_status_t FBE_API_CALL  fbe_api_dest_injection_search_record(fbe_dest_error_record_t* record_p, fbe_dest_record_handle_t* record_handle);
fbe_status_t FBE_API_CALL  fbe_api_dest_injection_get_record_next(fbe_dest_error_record_t* record_p, fbe_dest_record_handle_t* record_handle);
fbe_status_t FBE_API_CALL  fbe_api_dest_injection_record_update_times_to_insert(fbe_dest_record_handle_t *record_handle, fbe_dest_error_record_t *error_record);
fbe_status_t FBE_API_CALL  fbe_api_dest_injection_init(void);
fbe_status_t FBE_API_CALL  fbe_api_dest_injection_start(void);
fbe_status_t FBE_API_CALL  fbe_api_dest_injection_stop(void);
fbe_status_t FBE_API_CALL  fbe_api_dest_injection_get_state(fbe_dest_state_type *dest_state);
fbe_status_t FBE_API_CALL  fbe_api_dest_injection_init_scsi_error_record(fbe_dest_error_record_t *record_p);

/*! @} */ /* end of group fbe_api_dest_injection_interface */

//----------------------------------------------------------------
// Define the group for the FBE API DEST Injection Interface.  
// This is where all the function prototypes for the DEST Injection API.
//----------------------------------------------------------------
/*! @defgroup fbe_api_dest_injection_interface FBE API DEST Injection Interface
 *  @brief 
 *    This is the set of FBE API DEST Injection Interface.
 *
 *  @details 
 *    In order to access this library, please include fbe_api_dest_injection_interface.h.
 *
 *  @ingroup fbe_api_neit_package_class
 *  
 */
//----------------------------------------------------------------

FBE_API_CPP_EXPORT_END

#endif /*FBE_API_DEST_INJECTION_INTERFACE_H*/


