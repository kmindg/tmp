#ifndef FBE_API_PERSIST_INTERFACE_H
#define FBE_API_PERSIST_INTERFACE_H

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
//  This header file defines the FBE API for the persist service.
//    
//-----------------------------------------------------------------------------

/*!**************************************************************************
 * @file fbe_api_persist_interface.h
 ***************************************************************************
 *
 * @brief
 *  This header file defines the FBE API for the persist service.
 * 
 * @ingroup fbe_api_storage_extent_package_interface_class_files
 * 
 *
 ***************************************************************************/

//-----------------------------------------------------------------------------
//  INCLUDES OF REQUIRED HEADER FILES:
//
#include "fbe/fbe_types.h"
#include "fbe/fbe_transport.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_persist_interface.h"


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
// Define the FBE API PERSIST Interface for the Usurper Interface. 
// This is where all the data structures defined. 
//----------------------------------------------------------------
/*! @defgroup fbe_api_persist_interface_usurper_interface FBE API PERSIST Interface Usurper Interface
 *  @brief 
 *    This is the set of definitions that comprise the FBE API PERSIST Interface class
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
// Define the group for the FBE API PERSIST Interface.  
// This is where all the function prototypes for the FBE API PERSIST.
//----------------------------------------------------------------
/*! @defgroup fbe_api_persist_interface FBE API PERSIST Interface
 *  @brief 
 *    This is the set of FBE API PERSIST Interface. 
 *
 *  @details 
 *    In order to access this library, please include fbe_api_persist_interface.h.
 *
 *  @ingroup fbe_api_storage_extent_package_class
 *  @{
 */
//----------------------------------------------------------------
fbe_status_t FBE_API_CALL fbe_api_persist_set_lun(fbe_object_id_t lu_object_id,
												fbe_persist_completion_function_t completion_function,
												fbe_persist_completion_context_t completion_context);

fbe_status_t FBE_API_CALL fbe_api_persist_start_transaction(fbe_persist_transaction_handle_t *transaction_handle);

fbe_status_t FBE_API_CALL fbe_api_persist_abort_transaction(fbe_persist_transaction_handle_t transaction_handle);

fbe_status_t FBE_API_CALL fbe_api_persist_commit_transaction(fbe_persist_transaction_handle_t transaction_handle,
															 fbe_persist_completion_function_t completion_function,
															 fbe_persist_completion_context_t completion_context);

fbe_status_t FBE_API_CALL fbe_api_persist_write_entry(fbe_persist_transaction_handle_t transaction_handle,
													  fbe_persist_sector_type_t target_sector,
													  fbe_u8_t *data,
													  fbe_u32_t data_length,
													  fbe_persist_entry_id_t *entry_id);

fbe_status_t FBE_API_CALL fbe_api_persist_modify_entry(fbe_persist_transaction_handle_t transaction_handle,
													  fbe_u8_t *data,
													  fbe_u32_t data_length,
													  fbe_persist_entry_id_t entry_id);

fbe_status_t FBE_API_CALL fbe_api_persist_delete_entry(fbe_persist_transaction_handle_t transaction_handle,
													   fbe_persist_entry_id_t entry_id);

fbe_status_t FBE_API_CALL fbe_api_persist_read_sector(fbe_persist_sector_type_t target_sector,
													  fbe_u8_t *read_buffer,
													  fbe_u32_t data_length,/*must be of size FBE_PERSIST_USER_DATA_READ_BUFFER_SIZE*/
													  fbe_persist_entry_id_t start_entry_id,
													  fbe_persist_read_multiple_entries_completion_function_t completion_function,
													  fbe_persist_completion_context_t completion_context);

fbe_status_t FBE_API_CALL fbe_api_persist_read_single_entry(fbe_persist_entry_id_t entry_id,
															fbe_u8_t *read_buffer,
															fbe_u32_t data_length,/*MUST BE size of FBE_PERSIST_DATA_BYTES_PER_ENTRY*/
															fbe_persist_single_read_completion_function_t completion_function,
															fbe_persist_completion_context_t completion_context);

fbe_status_t FBE_API_CALL fbe_api_persist_get_layout_info(fbe_persist_control_get_layout_info_t *get_info);

fbe_status_t FBE_API_CALL fbe_api_persist_write_entry_with_auto_entry_id_on_top(fbe_persist_transaction_handle_t transaction_handle,
																				fbe_persist_sector_type_t target_sector,
																				fbe_u8_t *data,
																				fbe_u32_t data_length,
																				fbe_persist_entry_id_t *entry_id);

fbe_status_t FBE_API_CALL fbe_api_persist_write_single_entry(fbe_persist_sector_type_t target_sector,
															 fbe_u8_t *data,
															 fbe_u32_t data_length,
															 fbe_persist_single_operation_completion_function_t completion_function,
															 fbe_persist_completion_context_t completion_context);

fbe_status_t FBE_API_CALL fbe_api_persist_modify_single_entry(fbe_persist_entry_id_t entry_id,
															  fbe_u8_t *data,
															  fbe_u32_t data_length,
															  fbe_persist_single_operation_completion_function_t completion_function,
															  fbe_persist_completion_context_t completion_context);

fbe_status_t FBE_API_CALL fbe_api_persist_delete_single_entry(fbe_persist_entry_id_t entry_id,
															  fbe_persist_single_operation_completion_function_t completion_function,
															  fbe_persist_completion_context_t completion_context);

fbe_status_t FBE_API_CALL fbe_api_persist_get_total_blocks_needed(fbe_lba_t *total_blocks);

fbe_status_t FBE_API_CALL fbe_api_persist_unset_lun(void);

fbe_status_t FBE_API_CALL fbe_api_persist_add_hook(fbe_persist_hook_type_t hook_type, fbe_package_id_t target_package);
fbe_status_t FBE_API_CALL fbe_api_persist_remove_hook(fbe_persist_hook_type_t hook_type, fbe_package_id_t target_package);
fbe_status_t FBE_API_CALL fbe_api_persist_wait_hook(fbe_persist_hook_type_t hook_type, fbe_u32_t timeout_ms,
                                                    fbe_package_id_t target_package);




/*! @} */ /* end of group fbe_api_persist_interface */

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

#endif /* FBE_API_PERSIST_INTERFACE_H */

