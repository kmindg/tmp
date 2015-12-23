#ifndef FBE_API_PROVISION_DRIVE_INTERFACE_H
#define FBE_API_PROVISION_DRIVE_INTERFACE_H

/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_api_provision_drive_interface.h
 ***************************************************************************
 *
 * @brief
 *  This header file defines the FBE API for the Provision Drive object.
 * 
 * @ingroup fbe_api_storage_extent_package_interface_class_files
 * 
 * @version
 *   11/20/2009:  Created. Peter Puhov
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_types.h"
#include "fbe/fbe_transport.h"
#include "fbe/fbe_provision_drive.h"
#include "fbe/fbe_api_common.h"
#include "fbe_database.h"

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
// Define the FBE API Provision Drive Interface for the Usurper Interface. 
// This is where all the data structures defined. 
//----------------------------------------------------------------
/*! @defgroup fbe_api_provision_drive_interface_usurper_interface FBE API Provision Drive Interface Usurper Interface
 *  @brief 
 *    This is the set of definitions that comprise the FBE API Provision Drive Interface class
 *    usurper interface.
 *
 *  @ingroup fbe_api_classes_usurper_interface
 *  @{
 */ 
//----------------------------------------------------------------

//----------------------------------------------------------------
// Define the timeout period during creating or unconfiguring the
// HS.
//----------------------------------------------------------------
/*! @def PVD_UPDATE_CONFIG_TYPE_WAIT_TIMEOUT
 *  @brief
 *    This is the definition for PVD Update Configuration Type API.
 *    During creating or unconfiguring the HS, job service
 *    will wait for the defined period for the job to complete.
 *
 *  @ingroup fbe_api_provision_drive_interface
 */ 
//----------------------------------------------------------------
#define PVD_UPDATE_CONFIG_TYPE_WAIT_TIMEOUT 600000

/*!*******************************************************************
 * @struct fbe_api_provision_drive_calculate_capacity_info_t
 *********************************************************************
 * @brief
 *  This is the provision drive calculation capacity data structure. 
 *
 * @ingroup fbe_api_provision_drive_interface
 * @ingroup fbe_api_provision_drive_calculate_capacity_info
 *********************************************************************/
typedef struct fbe_api_provision_drive_calculate_capacity_info_s{
    fbe_lba_t imported_capacity; /*!< LDO capacity */
    fbe_block_edge_geometry_t block_edge_geometry; /*!< Downstream edge geometry */
    fbe_lba_t exported_capacity; /*!< PVD calculated capacity */
}fbe_api_provision_drive_calculate_capacity_info_t;

/*!*******************************************************************
 * @struct fbe_api_provision_drive_set_configuration_t
 *********************************************************************
 * @brief
 *  This is the provision drive configuration data structure. 
 *
 * @ingroup fbe_api_provision_drive_interface
 * @ingroup fbe_api_provision_drive_set_configuration
 *********************************************************************/
typedef struct fbe_api_provision_drive_set_configuration_s{
    /*! PVD Configuration Type 
     */
    fbe_provision_drive_config_type_t   config_type;

    /*! PVD sniff verify state 
     */
    fbe_bool_t sniff_verify_state;

    /*! Put any future user configured parameters here.
     */
    fbe_lba_t configured_capacity; /* We can create provisioned drive when LDO does not exist yet */
    /*! PVD Configured Block Size 
     */
    fbe_provision_drive_configured_physical_block_size_t configured_physical_block_size; /* We can create provisioned drive when LDO does not exist yet */
}fbe_api_provision_drive_set_configuration_t;

/*!*******************************************************************
 * @struct fbe_api_provisional_drive_paged_bits
 *********************************************************************
 * @brief
 *  This is the provision drive paged metadata API request structure to test 
 *  get/set/clear paged metadata functionality. 
 *
 * @ingroup fbe_api_provision_drive_interface
 * @ingroup fbe_api_provisional_drive_paged_bits
 *********************************************************************/
#pragma pack(1)
typedef struct fbe_api_provisional_drive_paged_bits_s{
    fbe_u16_t valid_bit:1; /* If TRUE, this paged metadata is valid. 
                              If FALSE, this paged metadata encountered uncorrectable read error and was zeroed by MDS */
    fbe_u16_t need_zero_bit:1;  /*!< associate chunk need to be zero */
    fbe_u16_t user_zero_bit:1;  /*!< user has requested zeroing for associate chunk */
    fbe_u16_t consumed_user_data_bit:1; /*!< associate chunk has valid user data */
    fbe_u64_t unused_bit:60;     /*!< unused bit */
}fbe_api_provisional_drive_paged_bits_t;
#pragma pack()

/*!*******************************************************************
 * @struct fbe_api_provisional_drive_paged_metadata_t
 *********************************************************************
 * @brief
 *  This is the provision drive paged metadata API request structure to test 
 *  get/set/clear paged metadata functionality. 
 *
 * @ingroup fbe_api_provision_drive_interface
 * @ingroup fbe_api_provisional_drive_paged_metadata
 *********************************************************************/
typedef struct fbe_api_provisional_drive_paged_metadata_s{
    fbe_u64_t       metadata_offset; /*!< metadata offset, start from 0 */
    fbe_u32_t       metadata_record_data_size; /*!< size of metadata structure */
    fbe_u64_t       metadata_repeat_count; /*!< repeat count for operation */
    fbe_u64_t       metadata_repeat_offset; /*!< repeat offset */
    fbe_api_provisional_drive_paged_bits_t   metadata_bits;  /*!< Metadata bits */
}fbe_api_provisional_drive_paged_metadata_t;


/*!*******************************************************************
 * @struct fbe_api_provision_drive_info_t
 *********************************************************************
 * @brief
 *  This structure is used to return provision drive object
 *  information.
 *
 *********************************************************************/
typedef struct fbe_api_provision_drive_info_s
{
    /*! Exported capacity of the provision drive object. 
     *  This is in terms of the provision drive block size.
     *  The provision drive block size is FBE_BE_BYTES_PER_BLOCK.
     */
    fbe_lba_t   capacity;

    /*! Configuration type of the pvd object. 
     */
    fbe_provision_drive_config_type_t   config_type;

    /*! Configured physical block size of the provision drive object.
     */
    fbe_provision_drive_configured_physical_block_size_t  configured_physical_block_size;

    /*! maximum number of the block we can send for particular i/o operation. */
    fbe_block_count_t max_drive_xfer_limit;
	
    /*! Debug flags to allow testing of different paths in the 
     *   provision drive object.
     */
    fbe_provision_drive_debug_flags_t debug_flags;     
  
    /*! It indiciates whether this drive has end of life state set, if it has 
     *  end of life state set and if it is configured as unknown then it will
     *  remain in fail state.
     */ 
    fbe_bool_t  end_of_life_state;

     /*! Will be used by sniff verify to save media error lba
     */ 
    fbe_lba_t   media_error_lba;

    /*! Zero on demand transition flag, it is to identify whether we are in 
     *  zero on demand mode or not.
     */
    fbe_bool_t  zero_on_demand;	 

    /*! Is this a system pvd or not. 
     */
    fbe_bool_t  is_system_drive;

    fbe_lba_t paged_metadata_lba; /*!< The lba where the paged metadata starts. */
    fbe_lba_t paged_metadata_capacity; /*!< Total capacity of paged metadata. */
    fbe_lba_t paged_mirror_offset;
    fbe_chunk_size_t chunk_size;    /*!< Number of blocks represented by one chunk. */
    fbe_chunk_count_t total_chunks; /*!< Total number of paged chunks. */
    fbe_u8_t  serial_num[FBE_SCSI_INQUIRY_SERIAL_NUMBER_SIZE+1];
    fbe_u32_t   port_number;
    fbe_u32_t   enclosure_number;
    fbe_u32_t   slot_number;
    fbe_drive_type_t    drive_type;
    fbe_lba_t           default_offset;
    fbe_block_count_t   default_chunk_size;
	fbe_lba_t			zero_checkpoint;/*can be obrained by a seperate interface but to reduce the calls we can supply it here as well*/
    fbe_provision_drive_slf_state_t  slf_state;
    fbe_bool_t  spin_down_qualified;     
    fbe_bool_t  drive_fault_state;
    fbe_bool_t  power_save_capable;
    fbe_bool_t  created_after_encryption_enabled;
    fbe_bool_t  scrubbing_in_progress;
    fbe_u32_t   percent_rebuilt;    /* A parent raid group will `push' this information down */

    fbe_bool_t  swap_pending;
    fbe_provision_drive_flags_t flags;
}
fbe_api_provision_drive_info_t;

/*!*******************************************************************
 * @struct fbe_api_provisional_drive_get_spare_drive_pool_t
 *********************************************************************
 * @brief
 *  This structure is used to get spare drive pool
 *  information.
 *
 *********************************************************************/
typedef struct fbe_api_provisional_drive_get_spare_drive_pool_s {
    fbe_u32_t       number_of_spare;
    fbe_object_id_t spare_object_list[FBE_MAX_SPARE_OBJECTS];  
}fbe_api_provisional_drive_get_spare_drive_pool_t;


/*!*******************************************************************
 * @struct fbe_api_provision_drive_send_download_asynch_t
 *********************************************************************
 * @brief
 *  This is the provision drive download data structure for async operations. 
 *
 * @ingroup fbe_api_provision_drive_interface
 * @ingroup fbe_api_provision_drive_send_download
 *********************************************************************/
typedef struct fbe_api_provision_drive_send_download_asynch_s{
    fbe_provision_drive_send_download_t       download_info;
    fbe_sg_element_t *                  download_sg_list;
    void                *caller_instance;     
    fbe_u8_t            bus, encl, slot;
    fbe_status_t        completion_status;  // indicates if call completed without errors
    void (* completion_function)(void *context);
}fbe_api_provision_drive_send_download_asynch_t;

/*!*******************************************************************
 * @struct fbe_api_provision_drive_update_config_type_s
 *********************************************************************
 * @brief
 *  This is fbe_api struct for FBE_CONFIG_UPDATE_PVD_TYPE.
 *  It is used by the client of fbe_api.
 *
 * @ingroup fbe_api_provision_drive_interface
 * @ingroup fbe_api_provision_drive_update_config_type
 *********************************************************************/
typedef struct fbe_api_provision_drive_update_config_type_s
{
    fbe_object_id_t                         pvd_object_id;      /*<!<PVD object id for which update the config type. */
    fbe_provision_drive_config_type_t       config_type;        /*<!<Config type of the provision drive object. */
    fbe_u8_t                                opaque_data[FBE_DATABASE_PROVISION_DRIVE_OPAQUE_DATA_MAX];
} fbe_api_provision_drive_update_config_type_t;

/*! @} */ /* end of group fbe_api_provision_drive_interface_usurper_interface */

/*!*******************************************************************
 * @struct fbe_api_provision_drive_update_pool_id_s
 *********************************************************************
 * @brief
 *  This is fbe_api struct for FBE_UPDATE_PVD_POOL_ID.
 *  It is used by the client of fbe_api.
 *
 * @ingroup fbe_api_provision_drive_interface
 * @ingroup fbe_api_provision_drive_update_pool_id
 *********************************************************************/
typedef struct fbe_api_provision_drive_update_pool_id_s
{
    fbe_u32_t          object_id;        /*<!<PVD object id for which update the config type. */
    fbe_u32_t          pool_id;          /*<!<PVD Pool id */ 
} fbe_api_provision_drive_update_pool_id_t;

/*!*******************************************************************
 * @struct fbe_api_provision_drive_update_pool_id_list_s
 *********************************************************************
 * @brief
 *  This is fbe_api struct for FBE_UPDATE_MULTIPLE_PVD_POOL_ID.
 *  It is used by the client of fbe_api.
 *
 * @ingroup fbe_api_provision_drive_interface
 * @ingroup fbe_api_provision_drive_update_pool_id_list
 *********************************************************************/
#define MAX_UPDATE_PVD_POOL_ID  DATABASE_TRANSACTION_MAX_USER_ENTRY
/* The definition of update_pvd_pool_data_t and update_pvd_pool_data_list_t 
 * should be same with the update_pvd_pool_id_t and update_pvd_pool_id_list_t defined in fbe_job_service.h */
typedef struct update_pvd_pool_data_s {
    fbe_u32_t          pvd_object_id;
    fbe_u32_t          pvd_pool_id;
} update_pvd_pool_data_t;

typedef struct update_pvd_pool_data_list_s {
    update_pvd_pool_data_t  pvd_pool_data[MAX_UPDATE_PVD_POOL_ID];
    fbe_u32_t               total_elements;
}update_pvd_pool_data_list_t;

typedef struct fbe_api_provision_drive_update_pool_id_list_s {
    update_pvd_pool_data_list_t pvd_pool_data_list;  /*<!PVD mutiple pool ids */
}fbe_api_provision_drive_update_pool_id_list_t;


/*!*******************************************************************
 * @def FBE_API_PROVISION_DRIVE_BIT_COMBINATIONS
 *********************************************************************
 * @brief Total combinations are 2^4 since there are 4 bits.
 *
 *********************************************************************/
#define FBE_API_PROVISION_DRIVE_BIT_COMBINATIONS (1 << 4)
/*!*******************************************************************
 * @struct fbe_api_provision_drive_get_paged_info_t
 *********************************************************************
 * @brief Returns summation of bits from the paged metadata.
 *
 *********************************************************************/
typedef struct fbe_api_provision_drive_get_paged_info_s
{
    fbe_u64_t num_valid_chunks; /*!< Total number of chunks with valid  */
    fbe_u64_t num_nz_chunks; /*!< Total number of chunks with nz set  */
    fbe_u64_t num_uz_chunks; /*!< Total number of chunks with uz set  */
    fbe_u64_t num_cons_chunks; /*!< Total number of chunks with Consumed  */
    fbe_u64_t bit_combinations_count[FBE_API_PROVISION_DRIVE_BIT_COMBINATIONS];
    fbe_chunk_count_t chunk_count;
}
fbe_api_provision_drive_get_paged_info_t;

/*!*******************************************************************
 * @struct fbe_api_provision_drive_set_swap_pending_t
 *********************************************************************
 * @brief   Mark the provision drive logically offline for the reason
 *          specified.
 *
 *********************************************************************/
typedef struct fbe_api_provision_drive_set_swap_pending_s
{
    fbe_provision_drive_swap_pending_reason_t  set_swap_pending_reason;
}
fbe_api_provision_drive_set_swap_pending_t;

/*!*******************************************************************
 * @struct fbe_api_provision_drive_get_swap_pending_t
 *********************************************************************
 * @brief   Get the provision drive logically offline for the reason
 *          specified.
 *
 *********************************************************************/
typedef struct fbe_api_provision_drive_get_swap_pending_s
{
    fbe_provision_drive_swap_pending_reason_t  get_swap_pending_reason;
}
fbe_api_provision_drive_get_swap_pending_t;

/*!*******************************************************************
 * @struct fbe_api_provision_drive_get_ssd_statistics_t
 *********************************************************************
 * @brief   Get the provision drive ssd statistics
 *
 *********************************************************************/
typedef struct fbe_api_provision_drive_get_ssd_statistics_s
{
    fbe_provision_drive_get_ssd_statistics_t  get_ssd_statistics;
}
fbe_api_provision_drive_get_ssd_statistics_t;

/*!*******************************************************************
 * @struct fbe_api_provision_drive_get_ssd_block_limits_t
 *********************************************************************
 * @brief   Get the provision drive ssd block_limits
 *
 *********************************************************************/
typedef struct fbe_api_provision_drive_get_ssd_block_limits_s
{
    fbe_provision_drive_get_ssd_block_limits_t  get_ssd_block_limits;
}
fbe_api_provision_drive_get_ssd_block_limits_t;

//----------------------------------------------------------------
// Define the group for the FBE API Provision Drive Interface.  
// This is where all the function prototypes for the FBE API Provision Drive.
//----------------------------------------------------------------
/*! @defgroup fbe_api_provision_drive_interface FBE API Provision Drive Interface
 *  @brief 
 *    This is the set of FBE API Provision Drive Interface. 
 *
 *  @details 
 *    In order to access this library, please include fbe_api_provision_drive_interface.h.
 *
 *  @ingroup fbe_api_storage_extent_package_class
 *  @{
 */
//----------------------------------------------------------------
fbe_status_t FBE_API_CALL fbe_api_provision_drive_calculate_capacity(fbe_api_provision_drive_calculate_capacity_info_t * capacity_info);
fbe_status_t FBE_API_CALL fbe_api_provision_drive_get_zero_checkpoint(fbe_object_id_t object_id, fbe_lba_t * zero_checkpoint);
fbe_status_t FBE_API_CALL fbe_api_provision_drive_set_zero_checkpoint(fbe_object_id_t object_id, fbe_lba_t zero_checkpoint);
fbe_status_t FBE_API_CALL fbe_api_provision_drive_get_verify_invalidate_checkpoint(fbe_object_id_t object_id, fbe_lba_t * verify_invalidate_checkpoint);
fbe_status_t FBE_API_CALL fbe_api_provision_drive_set_verify_invalidate_checkpoint(fbe_object_id_t    in_provision_drive_object_id,
                                                                                   fbe_lba_t          in_checkpoint);
fbe_status_t FBE_API_CALL fbe_api_provision_drive_set_configuration(fbe_object_id_t object_id, fbe_api_provision_drive_set_configuration_t * configuration);
fbe_status_t FBE_API_CALL fbe_api_provision_drive_initiate_disk_zeroing(fbe_object_id_t provision_drive_object_id);
fbe_status_t FBE_API_CALL fbe_api_provision_drive_initiate_consumed_area_zeroing(fbe_object_id_t provision_drive_object_id);
fbe_status_t FBE_API_CALL fbe_api_provision_drive_get_obj_id_by_location_from_topology(fbe_u32_t port, fbe_u32_t encl, fbe_u32_t slot, fbe_object_id_t *object_id);
/* Below function gets PVD location from nonpaged metadata, 
  * it maybe stale when original PVD is in FAIL state, and another 
  * disk gets into the same slot, DB would create a new PVD with
  * same location information as that original PVD.
  */
fbe_status_t FBE_API_CALL fbe_api_provision_drive_get_obj_id_by_location(fbe_u32_t port, fbe_u32_t encl, fbe_u32_t slot, fbe_object_id_t *object_id);
fbe_status_t FBE_API_CALL fbe_api_provision_drive_clear_verify_report(
                              fbe_object_id_t in_provision_drive_object_id, fbe_packet_attr_t in_attribute );
fbe_status_t FBE_API_CALL fbe_api_provision_drive_get_verify_report(
                              fbe_object_id_t in_provision_drive_object_id, fbe_packet_attr_t in_attribute, fbe_provision_drive_verify_report_t* out_verify_report_p );
fbe_status_t FBE_API_CALL fbe_api_provision_drive_get_verify_status(
                              fbe_object_id_t in_provision_drive_object_id, fbe_packet_attr_t in_attribute, fbe_provision_drive_get_verify_status_t* out_verify_status_p );
fbe_status_t FBE_API_CALL fbe_api_provision_drive_set_verify_checkpoint(
                              fbe_object_id_t in_provision_drive_object_id, fbe_packet_attr_t in_attribute, fbe_lba_t in_checkpoint );
fbe_status_t FBE_API_CALL fbe_api_provision_drive_metadata_paged_write(fbe_object_id_t object_id, fbe_api_provisional_drive_paged_metadata_t* get_bits_data);
fbe_status_t FBE_API_CALL fbe_api_provision_drive_get_background_priorities(fbe_object_id_t object_id, fbe_provision_drive_set_priorities_t* get_priorities);
fbe_status_t FBE_API_CALL fbe_api_provision_drive_set_background_priorities(fbe_object_id_t object_id, fbe_provision_drive_set_priorities_t* set_priorities);

fbe_status_t FBE_API_CALL
fbe_api_provision_drive_configure_as_spare(fbe_object_id_t object_id);

fbe_status_t FBE_API_CALL fbe_api_provision_drive_get_location(fbe_object_id_t object_id, 
                                                               fbe_u32_t * port_number,
                                                               fbe_u32_t * enclosure_number, 
                                                               fbe_u32_t * drive_number);

fbe_status_t FBE_API_CALL fbe_api_provision_drive_get_info(fbe_object_id_t object_id, 
                                                           fbe_api_provision_drive_info_t *provision_drive_info_p);

fbe_status_t FBE_API_CALL 
fbe_api_provision_drive_get_metadata_memory(fbe_object_id_t object_id, 
                                            fbe_provision_drive_control_get_metadata_memory_t * provisional_drive_control_get_metadata_memory);

fbe_status_t FBE_API_CALL 
fbe_api_provision_drive_get_versioned_metadata_memory(fbe_object_id_t object_id, 
                                            fbe_provision_drive_control_get_versioned_metadata_memory_t * provisional_drive_control_get_metadata_memory);


fbe_status_t FBE_API_CALL fbe_api_provision_drive_set_debug_flag(fbe_object_id_t object_id, fbe_provision_drive_debug_flags_t in_flag);
fbe_status_t FBE_API_CALL fbe_api_provision_drive_set_class_debug_flag(fbe_provision_drive_debug_flags_t in_flag);
fbe_status_t FBE_API_CALL 
fbe_api_provision_drive_get_spare_drive_pool(fbe_api_provisional_drive_get_spare_drive_pool_t *in_spare_drive_pool_p);

fbe_status_t FBE_API_CALL fbe_api_provision_drive_set_config_hs_flag(fbe_bool_t enable);

fbe_status_t FBE_API_CALL fbe_api_provision_drive_get_config_hs_flag(fbe_bool_t *b_is_test_spare_config_set_p);

fbe_status_t FBE_API_CALL fbe_api_provision_drive_set_eol_state(
                              fbe_object_id_t in_provision_drive_object_id, fbe_packet_attr_t in_attribute );
fbe_status_t FBE_API_CALL 
fbe_api_provision_drive_clear_eol_state(fbe_object_id_t   in_provision_drive_object_id,
                                        fbe_packet_attr_t in_attribute);

fbe_status_t FBE_API_CALL 
fbe_api_provision_drive_send_download_asynch(fbe_object_id_t object_id, fbe_api_provision_drive_send_download_asynch_t * fw_info);
fbe_status_t FBE_API_CALL fbe_api_provision_drive_abort_download(fbe_object_id_t object_id);
fbe_status_t FBE_API_CALL 
fbe_api_provision_drive_get_download_info(fbe_object_id_t object_id, fbe_provision_drive_get_download_info_t * get_download_p);
fbe_status_t FBE_API_CALL 
fbe_api_provision_drive_get_health_check_info(fbe_object_id_t object_id, fbe_provision_drive_get_health_check_info_t * get_health_check_p);
fbe_status_t FBE_API_CALL 
fbe_api_provision_drive_update_config_type(fbe_api_provision_drive_update_config_type_t*  update_pvd_config_type_p);
fbe_status_t FBE_API_CALL 
fbe_api_provision_drive_get_max_opaque_data_size(fbe_u32_t * max_opaque_data_size);
fbe_status_t FBE_API_CALL 
fbe_api_provision_drive_get_opaque_data(fbe_object_id_t object_id, fbe_u32_t max_opaque_data_size, fbe_u8_t * opaque_data_ptr);
fbe_status_t FBE_API_CALL 
fbe_api_provision_drive_get_pool_id(fbe_object_id_t object_id, fbe_u32_t *pool_id);
fbe_status_t FBE_API_CALL 
fbe_api_provision_drive_update_pool_id(fbe_api_provision_drive_update_pool_id_t *update_pvd_pool_id);
fbe_status_t FBE_API_CALL 
fbe_api_provision_drive_get_zero_percentage(fbe_object_id_t pvd_object_id, fbe_u16_t * zero_percentage);
fbe_status_t FBE_API_CALL 
fbe_api_copy_to_replacement_disk(fbe_object_id_t source_pvd_id, fbe_object_id_t destination_pvd_id, fbe_provision_drive_copy_to_status_t* copy_status);
fbe_status_t FBE_API_CALL 
fbe_api_provision_drive_user_copy(fbe_object_id_t source_pvd_id, fbe_provision_drive_copy_to_status_t* copy_status);
fbe_status_t FBE_API_CALL 
fbe_api_provision_drive_get_vd_object_id(fbe_object_id_t source_pvd_id, fbe_object_id_t *vd_object_id_p);
fbe_status_t FBE_API_CALL 
fbe_api_provision_drive_get_removed_timestamp(fbe_object_id_t pvd_object_id, fbe_system_time_t * removed_timestamp);
fbe_status_t FBE_API_CALL 
fbe_api_provision_drive_set_removed_timestamp(fbe_object_id_t pvd_object_id, fbe_system_time_t  removed_timestamp,fbe_bool_t is_persist);
fbe_status_t FBE_API_CALL 
fbe_api_provision_drive_set_slf_enabled(fbe_bool_t slf_enabled);
fbe_status_t FBE_API_CALL 
fbe_api_provision_drive_get_slf_enabled(fbe_bool_t *slf_enabled);
fbe_status_t FBE_API_CALL 
fbe_api_provision_drive_is_slf(fbe_object_id_t pvd_object_id, fbe_bool_t * is_slf);
fbe_status_t FBE_API_CALL 
fbe_api_provision_drive_test_slf(fbe_object_id_t provision_drive_object_id, fbe_u32_t op);

fbe_status_t FBE_API_CALL 
fbe_api_provision_drive_set_config_type(fbe_api_provision_drive_update_config_type_t*  update_pvd_config_type_p);

fbe_status_t FBE_API_CALL 
fbe_api_provision_drive_get_zero_checkpiont_offset(fbe_object_id_t pvd_object_id, fbe_u64_t * zero_checkpiont_offset);
fbe_status_t fbe_api_provision_drive_get_max_read_chunks(fbe_u32_t *num_chunks_p);
fbe_status_t fbe_api_provision_drive_get_paged_metadata(fbe_object_id_t pvd_object_id,
                                                        fbe_chunk_index_t chunk_offset,
                                                        fbe_chunk_index_t chunk_count,
                                                        fbe_provision_drive_paged_metadata_t *paged_md_p,
                                                        fbe_bool_t b_force_unit_access);
fbe_status_t FBE_API_CALL fbe_api_provision_drive_map_lba_to_chunk(fbe_object_id_t object_id, 
                                                                   fbe_provision_drive_map_info_t * provision_drive_info_p);
fbe_status_t FBE_API_CALL fbe_api_provision_drive_clear_drive_fault(fbe_object_id_t provision_drive_object_id);
fbe_status_t FBE_API_CALL 
fbe_api_provision_drive_set_background_operation_speed(fbe_provision_drive_background_op_t bg_op, fbe_u32_t bg_op_speed);
fbe_status_t FBE_API_CALL 
fbe_api_provision_drive_get_background_operation_speed(fbe_provision_drive_control_get_bg_op_speed_t *pvd_bg_op);
fbe_status_t FBE_API_CALL fbe_api_provision_drive_clear_all_pvds_drive_states(void);
fbe_status_t FBE_API_CALL fbe_api_provision_drive_disable_spare_select_unconsumed(void);
fbe_status_t FBE_API_CALL fbe_api_provision_drive_enable_spare_select_unconsumed(void);
fbe_status_t FBE_API_CALL fbe_api_provision_drive_get_disable_spare_select_unconsumed(fbe_bool_t *b_is_disable_spare_select_unconsumed_set_p);
fbe_status_t FBE_API_CALL fbe_api_provision_drive_send_logical_error(fbe_object_id_t object_id, fbe_block_transport_error_type_t error);
fbe_status_t FBE_API_CALL fbe_api_provision_drive_get_paged_bits(fbe_object_id_t pvd_object_id,
                                                                 fbe_api_provision_drive_get_paged_info_t *paged_info_p,
                                                                 fbe_bool_t b_force_unit_access);
fbe_status_t FBE_API_CALL fbe_api_provision_drive_update_pool_id_list(fbe_api_provision_drive_update_pool_id_list_t *pvd_pool_list);
fbe_status_t FBE_API_CALL fbe_api_provision_drive_get_miniport_key_handle(fbe_object_id_t object_id, 
                                                                          fbe_provision_drive_control_get_mp_key_handle_t *get_mp_key_handle);
fbe_status_t FBE_API_CALL fbe_api_provision_drive_quiesce(fbe_object_id_t object_id);
fbe_status_t FBE_API_CALL fbe_api_provision_drive_unquiesce(fbe_object_id_t object_id);
fbe_status_t FBE_API_CALL fbe_api_provision_drive_get_clustered_flag(fbe_object_id_t object_id, fbe_base_config_clustered_flags_t *flags);
fbe_status_t FBE_API_CALL fbe_api_provision_drive_disable_paged_cache(fbe_object_id_t object_id);
fbe_status_t FBE_API_CALL fbe_api_provision_drive_get_paged_cache_info(fbe_object_id_t object_id, fbe_provision_drive_get_paged_cache_info_t *get_info);
fbe_status_t FBE_API_CALL fbe_api_provision_drive_set_swap_pending(fbe_object_id_t pvd_object_id);
fbe_status_t FBE_API_CALL fbe_api_provision_drive_clear_swap_pending(fbe_object_id_t pvd_object_id);
fbe_status_t FBE_API_CALL fbe_api_provision_drive_get_swap_pending(fbe_object_id_t pvd_object_id,
                                                                   fbe_api_provision_drive_get_swap_pending_t *get_offline_info_p);
fbe_status_t FBE_API_CALL fbe_api_provision_drive_set_set_pvd_swap_pending_with_reason(fbe_object_id_t pvd_object_id,
                                                               fbe_api_provision_drive_set_swap_pending_t *set_swap_pending_p);
fbe_status_t FBE_API_CALL fbe_api_provision_drive_set_eas_start(fbe_object_id_t pvd_object_id);
fbe_status_t FBE_API_CALL fbe_api_provision_drive_set_eas_complete(fbe_object_id_t pvd_object_id);
fbe_status_t FBE_API_CALL fbe_api_provision_drive_get_eas_state(fbe_object_id_t pvd_object_id, fbe_bool_t * is_started, fbe_bool_t * is_complete);
fbe_status_t FBE_API_CALL fbe_api_provision_drive_get_ssd_statistics(fbe_object_id_t pvd_object_id,
																     fbe_api_provision_drive_get_ssd_statistics_t * get_stats_p);
fbe_status_t FBE_API_CALL fbe_api_provision_drive_get_ssd_block_limits(fbe_object_id_t pvd_object_id,
                                                                       fbe_api_provision_drive_get_ssd_block_limits_t * get_block_limits_p);
fbe_status_t FBE_API_CALL fbe_api_provision_drive_set_wear_leveling_timer(fbe_u64_t wear_leveling_timer_sec);

/*! @} */ /* end of group fbe_api_provision_drive_interface */

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

#endif /*FBE_API_PROVISION_DRIVE_INTERFACE_H*/


