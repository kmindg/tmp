#ifndef FBE_API_METADATA_INTERFACE_H
#define FBE_API_METADATA_INTERFACE_H

//-----------------------------------------------------------------------------
// Copyright (C) EMC Corporation 2010
// All rights reserved.
// Licensed material -- property of EMC Corporation
//
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
//
//  MODULE ABSTRACT: 
//
//  This header file defines the FBE API for the metadata service
//    
//-----------------------------------------------------------------------------

/*!**************************************************************************
 * @file fbe_api_metadata_interface.h
 ***************************************************************************
 *
 * @brief
 *  This header file defines the FBE API for the metadata service.
 * 
 * @ingroup fbe_api_storage_extent_package_interface_class_files
 * 
 *
 ***************************************************************************/

//-----------------------------------------------------------------------------
//  INCLUDES OF REQUIRED HEADER FILES:
//
#include "fbe/fbe_types.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_metadata_interface.h"

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
// Define the FBE API LUN Interface for the Usurper Interface. 
// This is where all the data structures defined. 
//----------------------------------------------------------------
/*! @defgroup fbe_api_lun_interface_usurper_interface FBE API LUN Interface Usurper Interface
 *  @brief 
 *    This is the set of definitions that comprise the FBE API LUN Interface class
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


/*! @} */ /* end of group fbe_api_lun_interface_usurper_interface */

//-----------------------------------------------------------------------------
//  MACROS:


//-----------------------------------------------------------------------------
//  FUNCTION PROTOTYPES: 

//----------------------------------------------------------------
// Define the group for the FBE API metadata.  
// This is where all the function prototypes for the FBE API metadata.
//----------------------------------------------------------------
/*! @defgroup fbe_api_metadata_interface FBE API Metadata Interface
 *  @brief 
 *    This is the set of FBE API Metadata Interfaces. 
 *
 *  @details 
 *    In order to access this library, please include fbe_api_metadata_interface.h.
 *
 *  @ingroup fbe_api_storage_extent_package_class
 *  @{
 */
//----------------------------------------------------------------
fbe_status_t FBE_API_CALL fbe_api_metadata_set_single_object_state(fbe_object_id_t object_id, fbe_metadata_element_state_t  new_state);
fbe_status_t FBE_API_CALL fbe_api_metadata_set_all_objects_state(fbe_metadata_element_state_t   new_state);

/*! @} */ /* end of group fbe_api_metadata_interface */

typedef struct fbe_api_metadata_debug_lock_s{
	fbe_object_id_t object_id;

	fbe_payload_stripe_lock_operation_opcode_t	opcode;		
	fbe_payload_stripe_lock_status_t			status;
	fbe_bool_t									b_allow_hold; 
	fbe_u32_t									stripe_count;

	fbe_u64_t			stripe_number;	
	fbe_packet_t * packet_ptr; /* Pointer to packet allocated for the lock operation */
}fbe_api_metadata_debug_lock_t;


/*for interface to get non-paged metadata version information of an object*/
typedef struct fbe_api_nonpaged_metadata_version_info_s{
    fbe_u32_t                metadata_initialized;
    fbe_sep_version_header_t ondisk_ver_header;     /*version header of the on-disk non-paged metadata*/  
    fbe_u32_t                cur_structure_size;    /*size of current non-paged metadata data structure*/
    fbe_u32_t                commit_nonpaged_size;  /*size of the non-paged metadata which already commit*/
}fbe_api_nonpaged_metadata_version_info_t;

typedef struct fbe_api_metadata_get_sl_info_s{
    fbe_object_id_t object_id;
    fbe_u32_t large_io_count;
    fbe_u32_t blob_size;
    fbe_u8_t slots[1026];
}fbe_api_metadata_get_sl_info_t;

fbe_status_t FBE_API_CALL fbe_api_metadata_debug_lock(fbe_packet_t * packet, fbe_api_metadata_debug_lock_t *  debug_lock);
fbe_status_t FBE_API_CALL fbe_api_metadata_debug_ext_pool_lock(fbe_packet_t * packet, fbe_api_metadata_debug_lock_t *  debug_lock);
fbe_status_t FBE_API_CALL fbe_api_metadata_nonpaged_system_load(void);
fbe_status_t FBE_API_CALL fbe_api_metadata_nonpaged_system_persist(void);
fbe_status_t FBE_API_CALL fbe_api_metadata_nonpaged_load(void);
fbe_status_t FBE_API_CALL fbe_api_metadata_nonpaged_persist(void);
fbe_status_t FBE_API_CALL fbe_api_metadata_nonpaged_system_load_with_disk_bitmask(fbe_u16_t in_disk_bitmask);
fbe_status_t FBE_API_CALL fbe_api_metadata_nonpaged_get_version_info(fbe_object_id_t object_id, 
                                                                     fbe_api_nonpaged_metadata_version_info_t *get_version_info);
fbe_status_t FBE_API_CALL fbe_api_metadata_get_system_object_need_rebuild_count(fbe_object_id_t in_object_id,fbe_u32_t*   need_rebuild_count);


/*interface for nonpaged metadata backup/restore*/


fbe_status_t FBE_API_CALL fbe_api_metadata_nonpaged_backup_objects(fbe_nonpaged_metadata_backup_restore_t *backup_p);
fbe_status_t FBE_API_CALL fbe_api_metadata_nonpaged_restore_objects_to_disk(fbe_nonpaged_metadata_backup_restore_t *backup_p);
fbe_status_t FBE_API_CALL fbe_api_metadata_nonpaged_restore_objects_to_memory(fbe_nonpaged_metadata_backup_restore_t *backup_p);

fbe_status_t FBE_API_CALL fbe_api_metadata_get_sl_info(fbe_api_metadata_get_sl_info_t * sl_info);

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

#endif /* FBE_API_METADATA_INTERFACE_H */



