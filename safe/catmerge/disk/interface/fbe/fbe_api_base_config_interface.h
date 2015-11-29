#ifndef FBE_API_BASE_CONFIG_INTERFACE_H
#define FBE_API_BASE_CONFIG_INTERFACE_H
/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_api_base_config_interface.h
 ***************************************************************************
 *
 * @brief
 *  This file contains definitions of functions that are exported by the 
 *  fbe_api_base_config_interface.
 * 
 * @ingroup fbe_api_storage_extent_package_interface_class_files
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
#include "fbe/fbe_base_config.h"

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
// Define the FBE API Base Config Interface for the Usurper Interface. 
// This is where all the data structures defined. 
//----------------------------------------------------------------
/*! @defgroup fbe_api_base_config_interface_usurper_interface FBE API Base Config Usurper Interface
 *  @brief 
 *    This is the set of definitions that comprise the FBE API Base Config class
 *    usurper interface.
 *
 *  @ingroup fbe_api_classes_usurper_interface
 *  @{
 */ 
//----------------------------------------------------------------

FBE_API_CPP_EXPORT_START

/*!********************************************************************* 
 * @struct fbe_api_base_config_metadata_paged_change_bits_t 
 *  
 * @brief 
 *   This contains the data info for the Metadata paged change bits.
 *
 * @ingroup fbe_api_base_config_interface
 * @ingroup fbe_api_base_config_metadata_paged_change_bits
 **********************************************************************/
typedef struct fbe_api_base_config_metadata_paged_change_bits_s {
    fbe_u64_t   metadata_offset;                                            /*!< Metadata offset */
    fbe_u8_t    metadata_record_data[FBE_PAYLOAD_METADATA_MAX_DATA_SIZE];   /*!< Metadata record data */
    fbe_u32_t   metadata_record_data_size;                                  /*!< Metadata record data size */
    fbe_u64_t   metadata_repeat_count;                                      /*!< Metadata repeat count */
    fbe_u64_t   metadata_repeat_offset;                                     /*!< Metadata repeat offset */
}fbe_api_base_config_metadata_paged_change_bits_t;


/*!********************************************************************* 
 * @struct fbe_api_base_config_metadata_paged_change_bits_t 
 *  
 * @brief 
 *   This contains the data info for the Metadata paged change bits.
 *
 * @ingroup fbe_api_base_config_interface
 * @ingroup fbe_api_base_config_metadata_paged_change_bits
 **********************************************************************/
typedef struct fbe_api_base_config_metadata_nonpaged_change_bits_s {
    fbe_u64_t   metadata_offset;                                            /*!< Metadata offset */
    fbe_u8_t    metadata_record_data[FBE_PAYLOAD_METADATA_MAX_DATA_SIZE];   /*!< Metadata record data */
    fbe_u32_t   metadata_record_data_size;                                  /*!< Metadata record data size */
    fbe_u64_t   metadata_repeat_count;                                      /*!< Metadata repeat count */
    fbe_u64_t   metadata_repeat_offset;                                     /*!< Metadata repeat offset */
}fbe_api_base_config_metadata_nonpaged_change_bits_t;

/*!********************************************************************* 
 * @struct fbe_api_base_config_metadata_paged_get_bits_t 
 *  
 * @brief 
 *   This contains the data info for Metadata paged get bits.
 *
 * @ingroup fbe_api_base_config_interface
 * @ingroup fbe_api_base_config_metadata_paged_get_bits
 **********************************************************************/
enum fbe_api_base_config_metadata_paged_get_bits_flags_e {
    FBE_API_BASE_CONFIG_METADATA_PAGED_GET_BITS_FLAGS_NONE      = 0x00000000, /* Default */
    FBE_API_BASE_CONFIG_METADATA_PAGED_GET_BITS_FLAGS_FUA_READ  = 0x00000001, /* Indicates that we must clear cache for the block before reading from disk*/
};
typedef fbe_u32_t fbe_api_base_config_metadata_paged_get_bits_flags_t;
typedef struct fbe_api_base_config_metadata_paged_get_bits_s {
    fbe_u64_t   metadata_offset;                                            /*!< Metadata offset */
    fbe_u8_t    metadata_record_data[FBE_PAYLOAD_METADATA_MAX_DATA_SIZE];   /*!< Metadata record data */
    fbe_u32_t   metadata_record_data_size;                                  /*!< Metadata record data size */
    fbe_api_base_config_metadata_paged_get_bits_flags_t get_bits_flags;
}fbe_api_base_config_metadata_paged_get_bits_t;
/*! @} */ /* end of group fbe_api_base_config_interface_usurper_interface */


/*!********************************************************************* 
 * @struct fbe_api_base_config_downstream_object_list_t 
 *  
 * @brief 
 *   This contains the data info for downstream object(s).
 *
 * @ingroup fbe_api_base_config_interface
 * @ingroup fbe_api_base_config_downstream_object_list
 **********************************************************************/
typedef struct fbe_api_base_config_downstream_object_list_s {
    /*! This field is used to return the number of downstream object
     *  information.
     */
    fbe_u32_t number_of_downstream_objects;

    /*! This filed is used to hold the list of downstream object-ids. 
     */
    fbe_object_id_t downstream_object_list[FBE_MAX_DOWNSTREAM_OBJECTS];
}fbe_api_base_config_downstream_object_list_t;

/*!********************************************************************* 
 * @struct fbe_api_base_config_upstream_object_list_t 
 *  
 * @brief 
 *   This contains the data info for upstream object(s).
 *
 * @ingroup fbe_api_base_config_interface
 * @ingroup fbe_api_base_config_downstream_object_list
 **********************************************************************/
typedef struct fbe_api_base_config_upstream_object_list_s {
    /*! This field is used to return the number of upstream object
     *  information.
     */
    fbe_u32_t number_of_upstream_objects;

    fbe_u32_t current_upstream_index;

    /*! This filed is used to hold the list of upstream object-ids. 
     */
    fbe_object_id_t upstream_object_list[FBE_MAX_UPSTREAM_OBJECTS];
}fbe_api_base_config_upstream_object_list_t;

/*!********************************************************************* 
 * @struct fbe_api_base_config_get_width_t
 *  
 * @brief 
 *   This contains the information for the base config width usurpur
 *   command.
 *
 * @ingroup fbe_api_base_config_interface
 * @ingroup fbe_api_base_config_get_width
 **********************************************************************/
typedef struct fbe_api_base_config_get_width_s {
    fbe_u32_t  width;   /*!< Width information. */
} fbe_api_base_config_get_width_t;
/*! @} */ /* end of group fbe_api_base_config_interface_usurper_interface */

//----------------------------------------------------------------
// Define the group for the FBE API Base Config Interface.  
// This is where all the function prototypes for the FBE API Base Config.
//----------------------------------------------------------------
/*! @defgroup fbe_api_base_config_interface FBE API Base Config Interface
 *  @brief 
 *    This is the set of FBE API Base Config.
 *
 *  @details 
 *    In order to access this library, please include fbe_api_base_config_interface.h.
 *
 *  @ingroup fbe_api_storage_extent_package_class
 *  @{
 */
//----------------------------------------------------------------
fbe_status_t FBE_API_CALL fbe_api_base_config_metadata_paged_set_bits(fbe_object_id_t object_id, 
																		fbe_api_base_config_metadata_paged_change_bits_t * change_bits);

fbe_status_t FBE_API_CALL fbe_api_base_config_metadata_paged_write(fbe_object_id_t object_id, 
                                                                   fbe_api_base_config_metadata_paged_change_bits_t * paged_change_bits);
fbe_status_t FBE_API_CALL fbe_api_base_config_metadata_paged_clear_bits(fbe_object_id_t object_id, 
																		fbe_api_base_config_metadata_paged_change_bits_t * change_bits);

fbe_status_t FBE_API_CALL fbe_api_base_config_metadata_nonpaged_set_bits(fbe_object_id_t object_id, 
																		fbe_api_base_config_metadata_nonpaged_change_bits_t * change_bits);

fbe_status_t FBE_API_CALL fbe_api_base_config_metadata_nonpaged_clear_bits(fbe_object_id_t object_id, 
																			fbe_api_base_config_metadata_nonpaged_change_bits_t * change_bits);

fbe_status_t FBE_API_CALL fbe_api_base_config_metadata_nonpaged_write_persist(fbe_object_id_t object_id,
                                                                              fbe_api_base_config_metadata_nonpaged_change_bits_t * nonpaged_change);

fbe_status_t FBE_API_CALL fbe_api_base_config_metadata_paged_clear_cache(fbe_object_id_t object_id);

fbe_status_t FBE_API_CALL fbe_api_base_config_metadata_nonpaged_set_checkpoint(fbe_object_id_t object_id, 
																				fbe_u64_t   metadata_offset,
																				fbe_u64_t   checkpoint);

fbe_status_t FBE_API_CALL fbe_api_base_config_metadata_nonpaged_incr_checkpoint(fbe_object_id_t object_id, 
																				fbe_u64_t   metadata_offset,
																				fbe_u64_t   checkpoint,
																				fbe_u32_t	repeat_count);

fbe_status_t FBE_API_CALL fbe_api_base_config_metadata_paged_get_bits(fbe_object_id_t object_id, 
                                                           fbe_api_base_config_metadata_paged_get_bits_t * paged_get_bits);
/*! @} */ /* end of group fbe_api_database_interface */

fbe_status_t FBE_API_CALL fbe_api_base_config_metadata_get_info(fbe_object_id_t object_id, fbe_metadata_info_t * metadata_info);

fbe_status_t FBE_API_CALL fbe_api_base_config_metadata_get_data(fbe_object_id_t object_id, fbe_metadata_nonpaged_data_t * metadata);

fbe_status_t FBE_API_CALL fbe_api_base_config_metadata_set_nonpaged_size(fbe_object_id_t object_id, fbe_base_config_control_metadata_set_size_t * set_size_info);
fbe_status_t FBE_API_CALL fbe_api_base_config_metadata_memory_read(fbe_object_id_t object_id, fbe_base_config_control_metadata_memory_read_t * md_data);
fbe_status_t FBE_API_CALL fbe_api_base_config_metadata_memory_update(fbe_object_id_t object_id, fbe_base_config_control_metadata_memory_update_t * md_data);

fbe_status_t FBE_API_CALL fbe_api_base_config_get_downstream_object_list(fbe_object_id_t object_id,
                                                                         fbe_api_base_config_downstream_object_list_t * downstrea_object_list_p);
fbe_status_t FBE_API_CALL fbe_api_base_config_get_ds_object_list(fbe_object_id_t object_id,
                                                                 fbe_api_base_config_downstream_object_list_t * downstrea_object_list_p,
                                                                 fbe_packet_attr_t attrib);
fbe_status_t FBE_API_CALL fbe_api_base_config_get_upstream_object_list(fbe_object_id_t object_id,
                                                                         fbe_api_base_config_upstream_object_list_t * downstrea_object_list_p);
fbe_status_t FBE_API_CALL fbe_api_base_config_get_encryption_state(fbe_object_id_t object_id,
                                                                   fbe_base_config_encryption_state_t * encryption_state_p);
fbe_status_t FBE_API_CALL fbe_api_base_config_set_encryption_state(fbe_object_id_t object_id,
                                                                   fbe_base_config_encryption_state_t encryption_state);
fbe_status_t FBE_API_CALL fbe_api_base_config_get_encryption_mode(fbe_object_id_t object_id,
                                                                   fbe_base_config_encryption_mode_t * encryption_mode_p);
fbe_status_t FBE_API_CALL fbe_api_base_config_set_encryption_mode(fbe_object_id_t object_id,
                                                                  fbe_base_config_encryption_mode_t  encryption_mode);
fbe_status_t FBE_API_CALL fbe_api_base_config_get_generation_number(fbe_object_id_t object_id,
                                                                    fbe_config_generation_t *gen_num);

fbe_status_t FBE_API_CALL fbe_api_base_config_send_io_control_operation(fbe_packet_t * packet_p,
                                                                        fbe_object_id_t object_id,
                                                                        fbe_base_config_io_control_operation_t * base_config_io_control_operation_p,
                                                                        fbe_packet_completion_function_t packet_completion_function,
                                                                        fbe_packet_completion_context_t packet_completion_context);
fbe_status_t FBE_API_CALL fbe_api_base_config_get_width(fbe_object_id_t object_id,
                                                              fbe_api_base_config_get_width_t * get_width_p);

fbe_status_t FBE_API_CALL fbe_api_base_config_metadata_nonpaged_persist(fbe_object_id_t object_id);

fbe_status_t FBE_API_CALL fbe_api_base_config_disable_all_bg_services(void);
fbe_status_t FBE_API_CALL fbe_api_base_config_enable_all_bg_services(void);
fbe_status_t FBE_API_CALL fbe_api_base_config_send_event(fbe_packet_t *packet_p, 
                                                                fbe_object_id_t object_id, 
                                                                fbe_base_config_control_send_event_t *send_event,
                                                                fbe_packet_completion_function_t packet_completion_function,
                                                                fbe_semaphore_t* sem);
fbe_status_t FBE_API_CALL fbe_api_base_config_metadata_set_default_nonpaged_metadata(fbe_object_id_t object_id);
fbe_status_t FBE_API_CALL fbe_api_base_config_metadata_set_nonpaged_metadata(fbe_object_id_t object_id,fbe_u8_t* metadata_record_data_p,fbe_u32_t record_data_size);

fbe_status_t FBE_API_CALL 
fbe_api_base_config_is_background_operation_enabled(fbe_object_id_t object_id, 
                                            fbe_base_config_background_operation_t background_op, fbe_bool_t * is_enabled);
fbe_status_t FBE_API_CALL 
fbe_api_base_config_enable_background_operation(fbe_object_id_t object_id, 
                                            fbe_base_config_background_operation_t background_op);
fbe_status_t FBE_API_CALL 
fbe_api_base_config_disable_background_operation(fbe_object_id_t object_id, 
                                            fbe_base_config_background_operation_t background_op);

typedef struct fbe_api_base_config_get_metadata_statistics_s{
	fbe_u64_t			stripe_lock_count;
	fbe_u64_t			local_collision_count;
	fbe_u64_t			peer_collision_count;
	fbe_u64_t			cmi_message_count;
}fbe_api_base_config_get_metadata_statistics_t;

fbe_status_t FBE_API_CALL fbe_api_base_config_get_metadata_statistics(fbe_object_id_t object_id, 
																	  fbe_api_base_config_get_metadata_statistics_t * metadata_statistics);

fbe_status_t FBE_API_CALL fbe_api_base_config_passive_request(fbe_object_id_t object_id);

fbe_status_t FBE_API_CALL fbe_api_base_config_set_deny_operation_permission(fbe_object_id_t object_id);
fbe_status_t FBE_API_CALL fbe_api_base_config_clear_deny_operation_permission(fbe_object_id_t object_id);

fbe_status_t FBE_API_CALL fbe_api_base_config_get_stripe_blob(fbe_object_id_t object_id, fbe_base_config_control_get_stripe_blob_t * blob);

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

#endif /* FBE_API_BASE_CONFIG_INTERFACE_H */


