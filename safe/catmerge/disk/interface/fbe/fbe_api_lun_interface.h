#ifndef FBE_API_LUN_INTERFACE_H
#define FBE_API_LUN_INTERFACE_H

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
//  This header file defines the FBE API for the lun object.
//    
//-----------------------------------------------------------------------------

/*!**************************************************************************
 * @file fbe_api_lun_interface.h
 ***************************************************************************
 *
 * @brief
 *  This header file defines the FBE API for the lun object.
 * 
 * @ingroup fbe_api_storage_extent_package_interface_class_files
 * 
 * @version
 *   11/12/2009:  Created. MEV 
 *
 ***************************************************************************/

//-----------------------------------------------------------------------------
//  INCLUDES OF REQUIRED HEADER FILES:
//
#include "fbe/fbe_types.h"
#include "fbe/fbe_transport.h"
#include "fbe/fbe_lun.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_base_config_interface.h"
#include "fbe/fbe_api_perfstats_interface.h"
#include "fbe_job_service.h"



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

/*!*******************************************************************
 * @struct fbe_api_lun_calculate_capacity_info_t
 *********************************************************************
 * @brief
 *  This is the LUN capacity data structure. This structure is used by
 *  FBE API to get imported or exported capacity of lun.
 *
 * @ingroup fbe_api_lun_interface
 * @ingroup fbe_api_lun_calculate_capacity_info
 *********************************************************************/
typedef struct fbe_api_lun_calculate_capacity_info_s {

    fbe_lba_t       imported_capacity;  /*!< lun imported capacity */
    fbe_lba_t       exported_capacity;  /*!< lun exported capacity */
    fbe_lba_t       lun_align_size;  /*!< imported capacity is aligned with RG chunk boundry, so need to know the size */
}fbe_api_lun_calculate_capacity_info_t;

/*!*******************************************************************
 * @struct fbe_api_lun_calculate_cache_zero_bitmap_blocks_t
 *********************************************************************
 * @brief
 *  This is to get the cache zero bitmap block count for given lun capacity.
 *
 *********************************************************************/
               
typedef struct fbe_api_lun_calculate_cache_zero_bitmap_blocks_s {

    fbe_block_count_t lun_capacity_in_blocks;                   /* lun capacity in blocks*/
    fbe_block_count_t cache_zero_bitmap_block_count;            /* cache zero bitmap block count*/
}fbe_api_lun_calculate_cache_zero_bitmap_blocks_t;


/*!*******************************************************************
 * @struct fbe_api_lun_create_t
 *********************************************************************
 * @brief
 *  This is fbe_api struct for creating a LUN
 *  It is used by the client of fbe_api.
 *
 * @ingroup fbe_api_lun_interface
 * @ingroup fbe_api_lun_create
 *********************************************************************/
typedef struct fbe_api_lun_create_s {
    /* for validation */
    fbe_raid_group_type_t        raid_type;      /*!< RAID Type */
    fbe_raid_group_number_t           raid_group_id;  /*!< RG ID */

    /* this following are the user specified parameters for LUN*/
    fbe_assigned_wwid_t          world_wide_name;      /*! World-Wide Name associated with the LUN */
    fbe_user_defined_lun_name_t  user_defined_name;    /*! User-Defined Name for the LUN */
    fbe_lun_number_t        lun_number;        /*!< LU Number (If user populate with FBE_LUN_ID_INVALID, MCR will assign LUn number)*/
    fbe_lba_t               capacity;          /*!< LU Capacity */
    fbe_block_transport_placement_t placement; /*!< Tranport Placement */
    fbe_lba_t                       addroffset; /*!< Address offset; used for NDB */
    fbe_bool_t                      ndb_b;      /*!< NDB bool */
    fbe_bool_t                      noinitialverify_b;/*!< Noinitialverify bool */
    fbe_bool_t                      user_private;	/*!< user private lun flag */
    fbe_bool_t                      is_system_lun;  /* used to create a system lun */
    fbe_object_id_t                 lun_id;        /*used to create a system lun. the object id is hardcoded */
    fbe_bool_t                      export_lun_b;  /* export lun by blkshim */
    /* add additional parameters here */
}fbe_api_lun_create_t;

/*!*******************************************************************
 * @struct fbe_api_lun_destroy_t
 *********************************************************************
 * @brief
 *  This is fbe_api struct for destroying a LUN
 *  It is used by the client of fbe_api.
 *
 * @ingroup fbe_api_lun_interface
 * @ingroup fbe_api_lun_destroy
 *********************************************************************/
typedef struct fbe_api_lun_destroy_s {
    fbe_u32_t        lun_number;   /*!< only need lun number to identify the lun to be destroyed*/	
    fbe_bool_t       allow_destroy_broken_lun;
}fbe_api_lun_destroy_t;

/*!*******************************************************************
 * @struct fbe_api_lun_update_t
 *********************************************************************
 * @brief
 *  This is fbe_api struct for updating a LUN
 *  It is used by the client of fbe_api.
 *
 * @ingroup fbe_api_lun_interface
 * @ingroup fbe_api_lun_update
 *********************************************************************/
typedef struct fbe_api_lun_update_s {
    fbe_assigned_wwid_t          world_wide_name;      /*! World-Wide Name associated with the LUN */
}fbe_api_lun_update_t;


/*!*******************************************************************
 * @struct fbe_api_lun_get_zero_status_s
 *********************************************************************
 * @brief
 *  This is fbe_api struct used to get the zero status for the LU
 *  object.
 *
 * @ingroup fbe_api_lun_interface
 * @ingroup fbe_api_lun_get_zero_status
 *********************************************************************/
typedef struct fbe_api_lun_get_zero_status_s {
    fbe_lba_t       zero_checkpoint;
    fbe_u16_t       zero_percentage;
}fbe_api_lun_get_zero_status_t;

/*!*******************************************************************
 * @struct fbe_api_lun_get_lun_info_s
 *********************************************************************
 * @brief
 *  This is fbe_api struct used to get info for the LU
 *  object.
 *
 * @ingroup fbe_api_lun_interface
 * @ingroup fbe_api_lun_get_lun_info
 *********************************************************************/
typedef struct fbe_api_lun_get_lun_info_s {
    fbe_lifecycle_state_t   peer_lifecycle_state; 
    fbe_lba_t               capacity;          /*!< LU Capacity (this is bigger than what the user sees) */    
    fbe_lba_t               offset; /*!< Lun offset */ 
    fbe_bool_t              b_initial_verify_requested; /*!< When LUN was created, was initial verify requested? */
    fbe_bool_t              b_initial_verify_already_run; /*!< Did the initial verify run on this lun? */
}fbe_api_lun_get_lun_info_t;

/*!*******************************************************************
 * @struct fbe_api_lun_get_rebuild_status
 *********************************************************************
 * @brief
 *  This is fbe_api struct used to get the rebuild status for the LU
 *  object.
 *
 * @ingroup fbe_api_lun_interface
 * @ingroup fbe_api_lun_get_rebuild_status
 *********************************************************************/
typedef struct fbe_api_lun_get_status_s {
    fbe_lba_t       checkpoint;
    fbe_u16_t       percentage_completed;
}fbe_api_lun_get_status_t;

/*! @} */ /* end of group fbe_api_lun_interface_usurper_interface */

//-----------------------------------------------------------------------------
//  MACROS:



//-----------------------------------------------------------------------------
//  FUNCTION PROTOTYPES: 

//----------------------------------------------------------------
// Define the group for the FBE API LUN Interface.  
// This is where all the function prototypes for the FBE API LUN.
//----------------------------------------------------------------
/*! @defgroup fbe_api_lun_interface FBE API LUN Interface
 *  @brief 
 *    This is the set of FBE API LUN Interface. 
 *
 *  @details 
 *    In order to access this library, please include fbe_api_lun_interface.h.
 *
 *  @ingroup fbe_api_storage_extent_package_class
 *  @{
 */
//----------------------------------------------------------------
fbe_status_t FBE_API_CALL fbe_api_lun_initiate_verify(
                            fbe_object_id_t             in_lun_object_id, 
                            fbe_packet_attr_t           in_attribute,
                            fbe_lun_verify_type_t       in_verify_type,
                            fbe_bool_t                  in_b_verify_entire_lun,
                            fbe_lba_t                   in_start_lba,
                            fbe_block_count_t           in_blocks);

fbe_status_t FBE_API_CALL fbe_api_lun_get_verify_report(
                            fbe_object_id_t             in_lun_object_id,
                            fbe_packet_attr_t           in_attribute,
                            fbe_lun_verify_report_t*    out_verify_report_p);

fbe_status_t FBE_API_CALL fbe_api_lun_get_verify_status(
                            fbe_object_id_t                 in_lun_object_id,
                            fbe_packet_attr_t               in_attribute,
                            fbe_lun_get_verify_status_t*    out_verify_status_p);

fbe_status_t FBE_API_CALL fbe_api_lun_trespass_op(fbe_object_id_t             in_lun_object_id,
                                     fbe_packet_attr_t           in_attribute,
                                     fbe_lun_trespass_op_t*      in_lun_trespass_op_p);

fbe_status_t FBE_API_CALL fbe_api_lun_clear_verify_report(
                            fbe_object_id_t             in_lun_object_id, 
                            fbe_packet_attr_t           in_attribute);

fbe_status_t FBE_API_CALL fbe_api_lun_calculate_max_lun_size(fbe_u32_t in_rg_id, fbe_lba_t in_capacity, 
                                               fbe_u32_t in_num_luns, fbe_lba_t* out_capacity_p);
fbe_status_t FBE_API_CALL fbe_api_lun_calculate_imported_capacity(fbe_api_lun_calculate_capacity_info_t * capacity_info);

fbe_status_t FBE_API_CALL fbe_api_lun_calculate_exported_capacity(fbe_api_lun_calculate_capacity_info_t * capacity_info);

fbe_status_t FBE_API_CALL fbe_api_lun_calculate_cache_zero_bit_map_size_to_remove(
                            fbe_api_lun_calculate_cache_zero_bitmap_blocks_t * cache_zero_bitmap_blocks_info);

fbe_status_t FBE_API_CALL fbe_api_lun_set_power_saving_policy(
                            fbe_object_id_t                 in_lun_object_id,
                            fbe_lun_set_power_saving_policy_t*    in_policy_p);

fbe_status_t FBE_API_CALL fbe_api_lun_get_power_saving_policy(
                            fbe_object_id_t                 in_lun_object_id,
                            fbe_lun_set_power_saving_policy_t*    in_policy_p);

fbe_status_t FBE_API_CALL fbe_api_create_lun(fbe_api_lun_create_t *lu_create_strcut, 
                                             fbe_bool_t wait_ready, fbe_u32_t ready_timeout_msec, 
                                             fbe_object_id_t *lu_object_id,
                                             fbe_job_service_error_type_t *job_error_type);

fbe_status_t FBE_API_CALL fbe_api_destroy_lun(fbe_api_lun_destroy_t *lu_destroy_strcut,
											  fbe_bool_t wait_destroy, 
											  fbe_u32_t destroy_timeout_msec,
                                              fbe_job_service_error_type_t *job_error_type);

fbe_status_t FBE_API_CALL fbe_api_lun_get_zero_status(fbe_object_id_t  lun_object_id,
                                                      fbe_api_lun_get_zero_status_t * fbe_api_lun_get_zero_status_t);
fbe_status_t FBE_API_CALL fbe_api_lun_get_lun_info(fbe_object_id_t  lun_object_id,
                                                   fbe_api_lun_get_lun_info_t * fbe_api_lun_get_lun_info_p);
fbe_status_t FBE_API_CALL fbe_api_lun_enable_peformance_stats(fbe_object_id_t in_lun_object_id);
fbe_status_t FBE_API_CALL fbe_api_lun_disable_peformance_stats(fbe_object_id_t in_lun_object_id);
fbe_status_t FBE_API_CALL fbe_api_lun_get_performance_stats(fbe_object_id_t in_lun_object_id,
                                                           fbe_lun_performance_counters_t   *lun_stats);

fbe_status_t FBE_API_CALL fbe_api_lun_get_rebuild_status(
                           fbe_object_id_t lun_object_id,
                           fbe_api_lun_get_status_t *lun_rebuild_status);

fbe_status_t FBE_API_CALL fbe_api_lun_get_bgverify_status(fbe_object_id_t lun_object_id, 
                                                          fbe_api_lun_get_status_t *lun_bgverify_status,
                                                          fbe_lun_verify_type_t verify_type);
fbe_status_t FBE_API_CALL fbe_api_lun_initiate_zero(fbe_object_id_t in_lun_object_id);

fbe_status_t FBE_API_CALL fbe_api_lun_initiate_hard_zero(fbe_object_id_t in_lun_object_id,
                                                         fbe_lba_t in_lba,
                                                         fbe_block_count_t in_blocks,
                                                         fbe_u64_t in_threads,
                                                         fbe_bool_t clear_paged_metadata);

fbe_status_t FBE_API_CALL fbe_api_lun_initiate_reboot_zero(fbe_object_id_t in_lun_object_id);

fbe_status_t FBE_API_CALL fbe_api_lun_initiate_verify_on_all_existing_luns(fbe_lun_verify_type_t in_verify_type);

fbe_status_t FBE_API_CALL fbe_api_lun_clear_verify_reports_on_all_existing_luns(void);

fbe_status_t FBE_API_CALL fbe_api_lun_clear_verify_reports_on_rg(fbe_object_id_t in_rg_id);

fbe_status_t FBE_API_CALL fbe_api_lun_set_write_bypass_mode( fbe_object_id_t in_lun_object_id, fbe_bool_t write_bypass_mode);
fbe_status_t FBE_API_CALL fbe_api_update_lun_wwn(fbe_object_id_t lu_object_id,
                                                 fbe_assigned_wwid_t* world_wide_name,
                                                 fbe_job_service_error_type_t *job_error_type);
fbe_status_t FBE_API_CALL fbe_api_update_lun_udn(fbe_object_id_t lu_object_id,
                                                 fbe_user_defined_lun_name_t* user_defined_name,
                                                 fbe_job_service_error_type_t *job_error_type);
fbe_status_t FBE_API_CALL fbe_api_update_lun_attributes(fbe_object_id_t lu_object_id,
                                                        fbe_u32_t attributes,
                                                        fbe_job_service_error_type_t *job_error_type);
fbe_status_t FBE_API_CALL fbe_api_lun_clear_unexpected_errors(fbe_object_id_t lun_object_id);
fbe_status_t FBE_API_CALL fbe_api_lun_disable_unexpected_errors(fbe_object_id_t lun_object_id);
fbe_status_t FBE_API_CALL fbe_api_lun_enable_unexpected_errors(fbe_object_id_t lun_object_id);

fbe_status_t FBE_API_CALL fbe_api_lun_map_lba(fbe_object_id_t lun_object_id,     
                                              fbe_raid_group_map_info_t * rg_map_info_p);

fbe_status_t FBE_API_CALL fbe_api_lun_initiate_verify_on_all_user_and_system_luns(fbe_lun_verify_type_t 
in_verify_type);

fbe_status_t FBE_API_CALL fbe_api_lun_enable_io_loopback(fbe_object_id_t lun_object_id);
fbe_status_t FBE_API_CALL fbe_api_lun_disable_io_loopback(fbe_object_id_t lun_object_id);
fbe_status_t FBE_API_CALL fbe_api_lun_get_io_loopback(fbe_object_id_t lun_object_id, fbe_bool_t *enabled_p);

fbe_status_t FBE_API_CALL fbe_api_lun_prepare_for_power_shutdown(fbe_object_id_t lun_object_id);

/*! @} */ /* end of group fbe_api_lun_interface */

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

#endif /* FBE_API_LUN_INTERFACE_H */


