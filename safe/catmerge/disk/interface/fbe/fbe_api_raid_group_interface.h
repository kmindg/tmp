#ifndef FBE_API_RAID_GROUP_INTERFACE_H
#define FBE_API_RAID_GROUP_INTERFACE_H

/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_api_raid_group_interface.h
 ***************************************************************************
 *
 * @brief
 *  This file contains definitions of functions that are exported by the 
 *  fbe_api_raid_group_interface.
 * 
 * @ingroup fbe_api_storage_extent_package_interface_class_files
 * 
 * @version
 *   11/05/2009:  Created. guov
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_types.h"
#include "fbe/fbe_transport.h"
#include "fbe/fbe_raid_group.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_lun.h"
#include "fbe_database.h"
#include "fbe_job_service.h"
#include "fbe_parity_write_log.h"

FBE_API_CPP_EXPORT_START

//----------------------------------------------------------------
// Define the timeout period during the RG creation
//----------------------------------------------------------------
/*! @def RG_WAIT_TIMEOUT
 *  @brief
 *    This is the definition for RG creation API. During RG creation, job service
 *    will wait for the defined period for RG creation to complete.
 *
 *  @ingroup fbe_api_raid_group_interface
 */ 
//----------------------------------------------------------------
#define RG_WAIT_TIMEOUT 120000


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
// Define the FBE API RAID Group Interface for the Usurper Interface. 
// This is where all the data structures defined. 
//----------------------------------------------------------------
/*! @defgroup fbe_api_raid_group_interface_usurper_interface FBE API RAID Group Usurper Interface
 *  @brief 
 *    This is the set of definitions that comprise the FBE API RAID Group class
 *    usurper interface.
 *
 *  @ingroup fbe_api_classes_usurper_interface
 *  @{
 */ 
//----------------------------------------------------------------


/*!*******************************************************************
 * @struct fbe_api_raid_group_calculate_capacity_info_t
 *********************************************************************
 * @brief
 *  This is fbe_api struct for calculate the RG capacity info.
 *  It is used by the client of fbe_api.
 *
 * @ingroup fbe_api_raid_group_interface
 * @ingroup fbe_api_raid_group_calculate_capacity_info
 *********************************************************************/
typedef struct fbe_api_raid_group_calculate_capacity_info_s
{
    /*! PVD exported
     */
    fbe_block_count_t imported_capacity;

    /*! Downstream edge geometry
     */
    fbe_block_edge_geometry_t block_edge_geometry;

    /*! RG exported capacity
     */
    fbe_block_count_t exported_capacity;
}
fbe_api_raid_group_calculate_capacity_info_t;

/*!*******************************************************************
 * @struct fbe_api_raid_group_class_get_info_t
 *********************************************************************
 * @brief
 *  This is fbe_api struct for getting the RG class info.
 *  It is used by the client of fbe_api.
 *
 * @ingroup fbe_api_raid_group_interface
 * @ingroup fbe_api_raid_group_class_get_info
 *********************************************************************/
typedef struct fbe_api_raid_group_class_get_info_s
{
    /*! Input: Number of drives in the raid group.
     */
    fbe_u32_t width;

    /*! Output: number of data disks (so on R5 for example this is width-1)
     */
    fbe_u16_t data_disks;
    /*! The overall capacity the raid group is exporting.
     * If this is not FBE_LBA_INVALID, then it is an input. 
     * Otherwise it is an output. 
     * This is imported capacity - the metadata capacity. 
     */
    fbe_block_count_t exported_capacity;

    /*! Output: The overall capacity the raid group is importing. 
     *          This is exported capacity + the metadata capacity. 
     */
    fbe_block_count_t imported_capacity;

    /*! This is the size imported of a single drive. 
     * If this is not FBE_LBA_INVALID, then it is an input. 
     * Otherwise it is an output. 
     */
    fbe_block_count_t single_drive_capacity;

    /*! Input: This is the type of raid group (user configurable).
     */
    fbe_raid_group_type_t raid_type;

    /*! Input: Determines if the user selected bandwidth or iops  
     */
    fbe_bool_t b_bandwidth;

    /*! Output: Size of each data element.
     */
    fbe_element_size_t  element_size;

    /*! Output: Number of elements (vertically) before parity rotates.
     */
    fbe_elements_per_parity_t  elements_per_parity;
}
fbe_api_raid_group_class_get_info_t;

/*!*******************************************************************
 * @struct fbe_api_raid_group_get_info_t
 *********************************************************************
 * @brief
 *  This structure is used to return raid group information.
 *
 *********************************************************************/
typedef struct fbe_api_raid_group_get_info_s
{
    fbe_u32_t width; /*!< Number of drives in the raid group. For R10 this is number of mirrors. */ 
    fbe_u32_t physical_width; /*! For R10 this is the actual # of physical drives. For other Raid types it is equal to width */
    fbe_block_count_t capacity;  /*!< The user capacity of the raid group. */ 
    fbe_block_count_t raid_capacity; /*!< Raid protected capacity. Includes user + metadata. */
    fbe_block_count_t imported_blocks_per_disk; /*!< Total blocks imported by the raid group on every disk. */

    fbe_lba_t paged_metadata_start_lba; /*!< The lba where the paged metadata starts. */
    fbe_block_count_t paged_metadata_capacity; /*!< Total capacity of paged metadata. */
    fbe_chunk_count_t total_chunks; /*!< Total chunks of paged */

    fbe_lba_t write_log_start_pba; /*!< The lba where the write_log starts. */
    fbe_block_count_t write_log_physical_capacity; /*!< Total capacity of write_log. */
    fbe_lba_t physical_offset; /*!< The offset on a disk  basis to get to physical address */

    fbe_u16_t num_data_disk; /*!< Total drives not including redundancy. */

    /*! raid group debug flags to force different options.
     */
    fbe_raid_group_debug_flags_t debug_flags;

    /*! These are debug flags passed to the raid library.
     */
    fbe_raid_library_debug_flags_t library_debug_flags;

    /*! flags related to I/O processing.
     */
    fbe_raid_group_flags_t flags;

    /*! Flags related to I/O processing. */
    fbe_base_config_flags_t base_config_flags;
 
    /*! Clustered flags related to I/O processing. */
    fbe_base_config_clustered_flags_t base_config_clustered_flags;

    /*! Clustered flags related to I/O processing. */
    fbe_base_config_clustered_flags_t base_config_peer_clustered_flags;

    /*! This is the type of raid group (user configurable).
     */
    fbe_raid_group_type_t raid_type;

    fbe_element_size_t          element_size; /*!< Blocks in each stripe element.*/
    fbe_block_size_t    exported_block_size; /*!< The logical block size that RAID exports to clients. */  
    fbe_block_size_t    imported_block_size; /*!< The imported physical block size. */
    fbe_block_size_t    optimal_block_size;  /*!< Number of blocks needed to avoid extra pre-reads. */

    /*! Total Stripe count.
     */
    fbe_block_count_t stripe_count;
    
    fbe_raid_position_bitmask_t bitmask_4k; /*!< Bitmask of positions that are 4k. */

    /*! Total sectors per stripe and does not include partity drive
     */
    fbe_block_count_t sectors_per_stripe;

    /*! array of flags, one for each disk in the RG.  When set to TRUE, signifies that the RG is 
     *  rb logging for a given disk.  Every I/O will be tracked and its chunk marked as NR when in
     *  this mode. 
     */
    fbe_bool_t b_rb_logging[FBE_RAID_MAX_DISK_ARRAY_WIDTH]; 

    /*!  Rebuild checkpoint - one for each disk in the RG */
    fbe_lba_t rebuild_checkpoint[FBE_RAID_MAX_DISK_ARRAY_WIDTH];

    /*! Verify checkpoint */
    fbe_lba_t ro_verify_checkpoint; 
    fbe_lba_t rw_verify_checkpoint; 
    fbe_lba_t error_verify_checkpoint;
    fbe_lba_t journal_verify_checkpoint;   
    fbe_lba_t incomplete_write_verify_checkpoint;
    fbe_lba_t system_verify_checkpoint;    

    /*! NP metadata flags, values defined in fbe_raid_group_np_flag_status_e */    
    fbe_u8_t  raid_group_np_md_flags;

    /*! NP metadata flags, values defined in fbe_raid_group_np_flag_extended_status_e */    
    fbe_u64_t  raid_group_np_md_extended_flags;

    /*! The size that lun should be aligned to  
     * currently lun_align_size = chunk_size*num_data_disks.
     */
    fbe_lba_t       lun_align_size;

    fbe_raid_group_local_state_t       local_state; /*state of the raid group - locally */
    fbe_raid_group_local_state_t       peer_state;  /*state of the raid group - on the peer */

    fbe_metadata_element_state_t metadata_element_state; /* indicates whether the SP is active or passive for the object */

     /*! Count for number of times paged metadata region has been verified */
    fbe_u32_t               paged_metadata_verify_pass_count;

    /*! elements per parity stripe */
    fbe_elements_per_parity_t elements_per_parity_stripe;
    
    fbe_bool_t  user_private;   /* indicate it's a user private raid group or not*/
    fbe_bool_t b_is_event_q_empty; /* Indciates if the rg objects event q is empty or not. */

    fbe_u32_t background_op_seconds; /*! Total seconds Background op is running for. */
    fbe_lba_t rekey_checkpoint; /*! Current rekey checkpoint. */
}
fbe_api_raid_group_get_info_t;

/*!*******************************************************************
 * @struct fbe_api_raid_group_class_set_options_t
 *********************************************************************
 * @brief Allows us to set various raid group options.
 *
 *********************************************************************/
typedef struct fbe_api_raid_group_class_set_options_s
{
    fbe_time_t user_io_expiration_time; /*!< Time for user I/Os to expire. */
    fbe_time_t paged_metadata_io_expiration_time; /*!< Time for metadata I/Os to expire. */
    fbe_u32_t encrypt_vault_wait_time; /*! Time to wait for no io on vault before encrypting. */
    fbe_u32_t encryption_blob_blocks; /*! Size of blob used to scan paged metadata.  */
}
fbe_api_raid_group_class_set_options_t;

/*!*******************************************************************
 * @struct fbe_api_raid_group_get_io_info_t
 *********************************************************************
 * @brief   Get the I/O information about this raid group:
 *              o Outstanding I/O count
 *              o Quiesced I/O count
 *              o Is quiesced
 *
 *********************************************************************/
typedef struct fbe_api_raid_group_get_io_info_s
{
    fbe_u32_t   outstanding_io_count;   /*!< Number of I/Os outstanding */
    fbe_bool_t  b_is_quiesced;          /*!< Is the raid group currently quiesced */
    fbe_u32_t   quiesced_io_count;      /*!< Number of I/Os that have been quiesced */
}
fbe_api_raid_group_get_io_info_t;

/*! @} */ /* end of group fbe_api_raid_group_interface_usurper_interface */



/*!*******************************************************************
 * @struct fbe_api_raid_library_error_testing_stats_t
 *********************************************************************
 * @brief
 *  This is fbe_api struct to return the statistics of unexpected
 *  error testing of raid library.
 *********************************************************************/
typedef struct fbe_api_raid_library_error_testing_stats_s
{
    fbe_u32_t error_count;  /*<! number of unexpected error path covered. */
}
fbe_api_raid_library_error_testing_stats_t;

/*!*******************************************************************
 * @struct fbe_api_raid_group_create_t
 *********************************************************************
 * @brief
 *  This is fbe_api struct for creating a RG
 *  It is used by the client of fbe_api.
 *
 * @ingroup fbe_api_raidgroup_interface
 * @ingroup fbe_api_rg_create
 *********************************************************************/
typedef struct fbe_api_raid_group_create_s
{
    fbe_raid_group_number_t  raid_group_id;         /*!< User provided id If the user passes FBE_RAID_ID_INVALID in rg_create_strcut->raid_group_id 
                                                         MCR will generte the ID and return it in rg_create_strcut->raid_group_id*/
    fbe_u32_t                drive_count;           /*!< number of drives that make up the Raid Group */
    fbe_u64_t               power_saving_idle_time_in_seconds; /*!< How long RG have to wait without IO to start saving power */
    fbe_u64_t               max_raid_latency_time_is_sec;  /*!< What's the longest time it should take the RG/drives to be ready after saving power */
    fbe_raid_group_type_t raid_type;          /*!< indicates Raid Group type, i.e Raid 5, Raid 0, etc */
    //fbe_bool_t               explicit_removal;      /*!< Indicates to remove the Raid Group after removing the last lun in the Raid Group */
    fbe_job_service_tri_state_t   is_private;            /*!< defines raid group as private */   
    fbe_bool_t               b_bandwidth;           /*!< Determines if the user selected bandwidth or iops */
    fbe_block_count_t    capacity;              /*!< indicates specific capacity settings for Raid group */
    fbe_job_service_bes_t    disk_array[FBE_RAID_MAX_DISK_ARRAY_WIDTH]; /*!< set of disks for raid group creation */
    fbe_bool_t               user_private;          /*!< defines raid group as user private */
    fbe_bool_t               is_system_rg;          /* Define raid group is a system RG*/
    fbe_object_id_t           rg_id;                /* If we create a system Raid group, we need to define the rg object id*/
}
fbe_api_raid_group_create_t;

/*!*******************************************************************
 * @struct fbe_api_raid_memory_stats_t
 *********************************************************************
 * @brief
 *  This is fbe_api struct for getting raid memory stats
 *  It is used by the client of fbe_api.
 *
 * @ingroup fbe_api_raidgroup_interface
 *********************************************************************/
typedef struct fbe_api_raid_memory_stats_s
{
    fbe_atomic_t allocations;
    fbe_atomic_t frees;
    fbe_atomic_t allocated_bytes;
    fbe_atomic_t freed_bytes;
    fbe_atomic_t deferred_allocations;
    fbe_atomic_t pending_allocations;
    fbe_atomic_t aborted_allocations;
    fbe_atomic_t allocation_errors;
    fbe_atomic_t free_errors;           /*! < Indicates a memory leak */
}
fbe_api_raid_memory_stats_t;

/*!*******************************************************************
 * @struct fbe_api_raid_group_get_paged_info_t
 *********************************************************************
 * @brief Returns summation of bits from the paged metadata.
 *
 *********************************************************************/
typedef struct fbe_api_raid_group_get_paged_info_s
{
    fbe_u64_t num_nr_chunks; /*!< Total number of chunks needing NR. */
    fbe_u64_t pos_nr_chunks[FBE_RAID_MAX_DISK_ARRAY_WIDTH]; /*!< Totals number of chunks needing rebuild by position. */
    fbe_u64_t num_user_vr_chunks; /* Number of user verify chunks. */
    fbe_u64_t num_error_vr_chunks; /* Number of error verify chunks. */
    fbe_u64_t num_read_only_vr_chunks; /* Number of read only verify chunks. */
    fbe_u64_t num_incomplete_write_vr_chunks; /* Number of incomplete write verify chunks. */
    fbe_u64_t num_system_vr_chunks; /* Number of system verify chunks. */
    fbe_u64_t num_rekey_chunks; /* Number of chunks marked as rekeyed. */
    fbe_raid_position_bitmask_t needs_rebuild_bitmask; /*!< Positions marked nr. */
    fbe_chunk_index_t first_rekey_chunk; /*!< First chunk marked as rekey. */
    fbe_chunk_index_t last_rekey_chunk; /*!< Last chunk marked as rekey. */
    fbe_chunk_count_t chunk_count;
}
fbe_api_raid_group_get_paged_info_t;


/*!*******************************************************************
 * @struct fbe_api_rg_get_status
 *********************************************************************
 * @brief
 *  This is fbe_api struct used to get the checkpoint & percentage complete
 *  status for different operations on RG object.
 *
 * @ingroup fbe_api_rg_interface
 * @ingroup fbe_api_rg_get_status
 *********************************************************************/
typedef struct fbe_api_rg_get_status_s {
    fbe_lba_t       checkpoint;
    fbe_u16_t       percentage_completed;
}fbe_api_rg_get_status_t;

/*!*******************************************************************
 * @struct fbe_api_destroy_rg_t
 *********************************************************************
 * @brief
 *  This is fbe_api struct for destroying a RG
 *  It is used by the client of fbe_api.
 *
 * @ingroup fbe_api_raidgroup_interface
 * @ingroup fbe_api_destroy_rg
 *********************************************************************/
typedef struct fbe_api_destroy_rg_s 
{
    fbe_raid_group_number_t  raid_group_id; /*!< User provided RG id */

} fbe_api_destroy_rg_t;

/*! @} */ /* end of group fbe_api_raid_group_interface_usurper_interface */



//----------------------------------------------------------------
// Define the group for the FBE API RAID Group Interface.  
// This is where all the function prototypes for the FBE API RAID Group.
//----------------------------------------------------------------
/*! @defgroup fbe_api_raid_group_interface FBE API RAID Group Interface
 *  @brief 
 *    This is the set of FBE API RAID Group Interface. 
 *
 *  @details 
 *    In order to access this library, please include fbe_api_raid_group_interface.h.
 *
 *  @ingroup fbe_api_storage_extent_package_class
 *  @{
 */
//----------------------------------------------------------------
fbe_status_t FBE_API_CALL
fbe_api_create_rg(fbe_api_raid_group_create_t *rg_create_strcut,
                                             fbe_bool_t wait_ready, 
                                             fbe_u32_t ready_timeout_msec,
                                             fbe_object_id_t *rg_object_id,
                                             fbe_job_service_error_type_t *job_error_type);
fbe_status_t FBE_API_CALL
fbe_api_raid_group_get_info(fbe_object_id_t rg_object_id, 
                            fbe_api_raid_group_get_info_t * raid_group_info_p,
                            fbe_packet_attr_t attribute);


fbe_status_t FBE_API_CALL fbe_api_raid_group_get_write_log_info(fbe_object_id_t                  rg_object_id,
                                                                fbe_parity_get_write_log_info_t *write_log_info_p,
                                                                fbe_packet_attr_t                attribute);
fbe_status_t FBE_API_CALL
 fbe_api_raid_group_initiate_verify(fbe_object_id_t        in_raid_group_object_id, 
                                    fbe_packet_attr_t      in_attribute,
                                    fbe_lun_verify_type_t  in_verify_type);
                                               
fbe_status_t FBE_API_CALL fbe_api_raid_group_get_flush_error_counters(
                            fbe_object_id_t                 in_raid_group_object_id,
                            fbe_packet_attr_t               in_attribute,
                            fbe_raid_flush_error_counts_t*  out_verify_report_p);

fbe_status_t FBE_API_CALL fbe_api_raid_group_clear_flush_error_counters(
                            fbe_object_id_t             in_raid_group_object_id, 
                            fbe_packet_attr_t           in_attribute);

fbe_status_t FBE_API_CALL fbe_api_raid_group_get_io_info(
                            fbe_object_id_t             in_raid_group_object_id,
                            fbe_api_raid_group_get_io_info_t *raid_group_io_info_p); 

fbe_status_t FBE_API_CALL
fbe_api_raid_group_quiesce(fbe_object_id_t rg_object_id, 
                           fbe_packet_attr_t attribute);
fbe_status_t FBE_API_CALL
fbe_api_raid_group_unquiesce(fbe_object_id_t rg_object_id,
                             fbe_packet_attr_t attribute);

fbe_status_t FBE_API_CALL
fbe_api_raid_group_calculate_capacity(fbe_api_raid_group_calculate_capacity_info_t * capacity_info_p);

fbe_status_t FBE_API_CALL
fbe_api_raid_group_class_get_info(fbe_api_raid_group_class_get_info_t * get_info_p, fbe_class_id_t class_id);

fbe_status_t FBE_API_CALL
fbe_api_raid_group_set_library_debug_flags(fbe_object_id_t rg_object_id, 
                                           fbe_raid_library_debug_flags_t raid_library_debug_flags);

fbe_status_t FBE_API_CALL
fbe_api_raid_group_set_group_debug_flags(fbe_object_id_t rg_object_id, 
                                         fbe_raid_group_debug_flags_t raid_group_debug_flags);

fbe_status_t FBE_API_CALL
fbe_api_raid_group_get_library_debug_flags(fbe_object_id_t rg_object_id, 
                                           fbe_raid_library_debug_flags_t *raid_library_debug_flags_p);

fbe_status_t FBE_API_CALL
fbe_api_raid_group_get_group_debug_flags(fbe_object_id_t rg_object_id, 
                                         fbe_raid_group_debug_flags_t *raid_group_debug_flags_p);

fbe_status_t FBE_API_CALL
fbe_api_raid_group_set_class_library_debug_flags(fbe_class_id_t raid_group_class_id,
                                                 fbe_raid_library_debug_flags_t raid_library_debug_flags);

fbe_status_t FBE_API_CALL
fbe_api_raid_group_set_class_group_debug_flags(fbe_class_id_t raid_group_class_id,
                                               fbe_raid_group_debug_flags_t raid_group_debug_flags);
fbe_status_t FBE_API_CALL
fbe_api_raid_group_set_default_debug_flags(fbe_class_id_t raid_group_class_id,
                                           fbe_raid_group_debug_flags_t user_debug_flags,
                                           fbe_raid_group_debug_flags_t system_debug_flags);

fbe_status_t FBE_API_CALL
fbe_api_raid_group_set_default_library_flags(fbe_class_id_t raid_group_class_id,
                                             fbe_raid_group_debug_flags_t user_debug_flags,
                                             fbe_raid_group_debug_flags_t system_debug_flags);
fbe_status_t FBE_API_CALL
fbe_api_raid_group_get_default_library_flags(fbe_class_id_t raid_group_class_id,
                                             fbe_raid_group_default_library_flag_payload_t *debug_flags_p);
fbe_status_t FBE_API_CALL
fbe_api_raid_group_get_default_debug_flags(fbe_class_id_t raid_group_class_id,
                                           fbe_raid_group_default_debug_flag_payload_t *debug_flags_p);

fbe_status_t FBE_API_CALL fbe_api_raid_group_set_raid_lib_error_testing(void);
fbe_status_t FBE_API_CALL fbe_api_raid_group_enable_raid_lib_random_errors(fbe_u32_t percentage,
                                                                           fbe_bool_t b_only_user_rgs);
fbe_status_t FBE_API_CALL
fbe_api_raid_group_get_raid_library_error_testing_stats(fbe_api_raid_library_error_testing_stats_t *error_testing_stats_p);

fbe_status_t FBE_API_CALL
fbe_api_raid_group_get_raid_memory_stats(fbe_api_raid_memory_stats_t *memory_stats_p);

fbe_status_t FBE_API_CALL fbe_api_raid_group_reset_raid_memory_stats(void);

fbe_status_t FBE_API_CALL fbe_api_raid_group_reset_raid_lib_error_testing_stats(void);
fbe_status_t FBE_API_CALL fbe_api_raid_group_class_set_options(fbe_api_raid_group_class_set_options_t * set_options_p,
                                                               fbe_class_id_t class_id);
fbe_status_t FBE_API_CALL fbe_api_raid_group_class_init_options(fbe_api_raid_group_class_set_options_t * set_options_p);

fbe_status_t FBE_API_CALL fbe_api_raid_group_get_power_saving_policy(
                            fbe_object_id_t                 in_rg_object_id,
                            fbe_raid_group_get_power_saving_info_t*    in_policy_p);

fbe_status_t FBE_API_CALL fbe_api_destroy_rg(fbe_api_destroy_rg_t *destroy_rg_struct_p, 
                                             fbe_bool_t wait_destroy_b, 
                                             fbe_u32_t destroy_timeout_msec,
                                             fbe_job_service_error_type_t *job_error_type);
fbe_status_t FBE_API_CALL fbe_api_raid_group_get_paged_bits(fbe_object_id_t rg_object_id,
                                                            fbe_api_raid_group_get_paged_info_t *paged_info_p,
                                                            fbe_bool_t b_force_unit_access);

fbe_status_t FBE_API_CALL fbe_api_raid_group_get_rebuild_status(fbe_object_id_t rg_object_id, 
                                                                fbe_api_rg_get_status_t *rebuild_status);
fbe_status_t FBE_API_CALL fbe_api_raid_group_get_bgverify_status(fbe_object_id_t rg_object_id, 
                                                                 fbe_api_rg_get_status_t *bgverify_status,
                                                                 fbe_lun_verify_type_t verify_type);
fbe_status_t FBE_API_CALL fbe_api_raid_group_get_physical_from_logical_lba(fbe_object_id_t rg_object_id,
                                                                           fbe_lba_t lba,
                                                                           fbe_u64_t *pba_p);
fbe_status_t FBE_API_CALL fbe_api_raid_group_get_paged_metadata_verify_pass_count(fbe_object_id_t rg_object_id, 
                                                                                  fbe_u32_t *pass_count);
fbe_status_t FBE_API_CALL 
fbe_api_raid_group_get_max_unused_extent_size(fbe_object_id_t  in_rg_object_id, 
                                              fbe_block_transport_control_get_max_unused_extent_size_t* out_extent_size_p);
fbe_status_t FBE_API_CALL 
fbe_api_raid_group_set_rebuild_checkpoint(fbe_object_id_t  in_rg_object_id, 
                                          fbe_raid_group_set_rb_checkpoint_t in_rg_set_checkpoint);
fbe_status_t FBE_API_CALL 
fbe_api_raid_group_set_verify_checkpoint(fbe_object_id_t  in_rg_object_id, 
                                         fbe_raid_group_set_verify_checkpoint_t in_rg_set_checkpoint);
fbe_bool_t fbe_api_raid_group_info_is_degraded(fbe_api_raid_group_get_info_t *rg_info_p);

fbe_status_t fbe_api_raid_group_get_verify_checkpoint(fbe_api_raid_group_get_info_t *rg_info_p,
                                                      fbe_lun_verify_type_t verify_type,
                                                      fbe_lba_t *chkpt_p);
fbe_status_t FBE_API_CALL fbe_api_raid_group_map_lba(fbe_object_id_t rg_object_id, 
                                                     fbe_raid_group_map_info_t * rg_map_info_p);
fbe_status_t FBE_API_CALL fbe_api_raid_group_map_pba(fbe_object_id_t rg_object_id, 
                                                     fbe_raid_group_map_info_t * rg_map_info_p);
fbe_status_t fbe_api_raid_group_max_get_paged_chunks(fbe_u32_t *num_chunks_p);

fbe_status_t fbe_api_raid_group_get_paged_metadata(fbe_object_id_t rg_object_id,
                                                   fbe_chunk_index_t chunk_offset,
                                                   fbe_chunk_index_t chunk_count,
                                                   fbe_raid_group_paged_metadata_t *paged_md_p,
                                                   fbe_bool_t b_force_unit_access);

fbe_status_t fbe_api_raid_group_set_paged_metadata(fbe_object_id_t rg_object_id,
                                                   fbe_chunk_index_t chunk_offset,
                                                   fbe_chunk_index_t chunk_count,
                                                   fbe_raid_group_paged_metadata_t *paged_md_p);

fbe_status_t FBE_API_CALL fbe_api_raid_group_force_mark_nr(fbe_object_id_t rg_object_id,
                                                           fbe_u32_t port,
                                                           fbe_u32_t encl,
                                                           fbe_u32_t slot);
fbe_status_t FBE_API_CALL fbe_api_raid_group_set_mirror_prefered_position(fbe_object_id_t rg_object_id, 
                                                                          fbe_u32_t prefered_position);

fbe_status_t FBE_API_CALL 
fbe_api_raid_group_get_background_operation_speed(fbe_raid_group_control_get_bg_op_speed_t *rg_bg_op);
fbe_status_t FBE_API_CALL 
fbe_api_raid_group_set_background_operation_speed(fbe_raid_group_background_op_t bg_op, fbe_u32_t bg_op_speed);

fbe_status_t FBE_API_CALL fbe_api_raid_group_set_update_peer_checkpoint_interval(fbe_u32_t update_peer_checkpoint_interval);

fbe_status_t FBE_API_CALL fbe_api_raid_group_get_stats(fbe_object_id_t rg_object_id, 
                                                       fbe_raid_group_get_statistics_t * raid_group_info_p);

fbe_status_t FBE_API_CALL fbe_api_raid_group_set_bts_params(fbe_raid_group_set_default_bts_params_t * params_p);

fbe_status_t FBE_API_CALL fbe_api_raid_group_get_bts_params(fbe_raid_group_get_default_bts_params_t * params_p);
fbe_status_t FBE_API_CALL fbe_api_raid_group_expand(fbe_object_id_t rg_object_id, fbe_lba_t new_capacity, fbe_lba_t new_edge_capacity);

fbe_status_t FBE_API_CALL fbe_api_raid_group_set_encryption_state(fbe_object_id_t rg_object_id, 
                                                                  fbe_raid_group_set_encryption_mode_t * set_enc_p);

fbe_status_t FBE_API_CALL fbe_api_raid_group_get_lowest_drive_tier(fbe_object_id_t rg_object_id, 
                                                                   fbe_get_lowest_drive_tier_t * drive_tier_p);

fbe_status_t FBE_API_CALL fbe_api_raid_group_set_raid_attributes(fbe_object_id_t rg_object_id, 
                                                                 fbe_u32_t  attributes);

fbe_status_t FBE_API_CALL fbe_api_raid_group_class_get_extended_media_error_handling(fbe_raid_group_class_get_extended_media_error_handling_t *get_emeh_p);

fbe_status_t FBE_API_CALL fbe_api_raid_group_class_set_extended_media_error_handling(fbe_raid_emeh_mode_t set_mode,
                                                                               fbe_bool_t b_change_threshold,
                                                                               fbe_u32_t percent_threshold_increase,
                                                                               fbe_bool_t b_persist);

fbe_status_t FBE_API_CALL fbe_api_raid_group_get_extended_media_error_handling(fbe_object_id_t rg_object_id,
                                                     fbe_raid_group_get_extended_media_error_handling_t *get_emeh_p);

fbe_status_t FBE_API_CALL fbe_api_raid_group_set_extended_media_error_handling(fbe_object_id_t rg_object_id,
                                                     fbe_raid_emeh_command_t set_control,
                                                     fbe_bool_t b_change_threshold,
                                                     fbe_u32_t percent_threshold_increase);
fbe_status_t FBE_API_CALL fbe_api_raid_group_get_wear_level(fbe_object_id_t rg_object_id, 
                                                            fbe_raid_group_get_wear_level_t * wear_level_p);
fbe_status_t FBE_API_CALL fbe_api_raid_group_set_lifecycle_timer_interval(fbe_object_id_t rg_object_id, 
                                                                          fbe_lifecycle_timer_info_t * timer_info_p);

fbe_status_t FBE_API_CALL fbe_api_raid_group_set_raid_group_debug_state(fbe_object_id_t rg_object_id,
                                              fbe_u64_t set_local_state_mask,     
                                              fbe_u64_t set_clustered_flags,      
                                              fbe_u32_t set_raid_group_condition);


fbe_status_t FBE_API_CALL fbe_api_raid_group_set_chunks_per_rebuild(fbe_u32_t num_chunks_per_rebuild);
/*! @} */ /* end of group fbe_api_raid_group_interface */


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

#endif /*FBE_API_RAID_GROUP_INTERFACE_H*/


