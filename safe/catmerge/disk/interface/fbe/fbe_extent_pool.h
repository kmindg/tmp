#ifndef FBE_EXTENT_POOL_H
#define FBE_EXTENT_POOL_H
/***************************************************************************
 * Copyright (C) EMC Corporation 2014
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/
 
/*!**************************************************************************
 * @file fbe_extent_pool.h
 ***************************************************************************
 *
 * @brief
 *  This file contains definitions of functions that are exported by the
 *  extent pool object.
 * 
 * @revision
 *   6/11/2014 - Created. Rob Foley
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/

#ifndef UEFI_ENV
#include "fbe/fbe_object.h"
#include "fbe/fbe_transport.h"
#include "fbe/fbe_base_config.h"
#include "fbe_metadata.h"
#endif

typedef fbe_u64_t fbe_extent_pool_clustered_flags_t;
#ifndef UEFI_ENV
typedef struct fbe_extent_pool_metadata_memory_s{
    fbe_base_config_metadata_memory_t base_config_metadata_memory; /* MUST be first */
    fbe_extent_pool_clustered_flags_t flags;
}fbe_extent_pool_metadata_memory_t;

#pragma pack(1)
typedef struct fbe_extent_pool_nonpaged_metadata_s
{
    /*! nonpaged metadata which is common to all the objects derived from
     *  the base config.
     */
    fbe_base_config_nonpaged_metadata_t base_config_nonpaged_metadata;  /* MUST be first */

    /*NOTES!!!!!!*/
    /*non-paged metadata versioning requires that any modification for this data structure should 
      pay attention to version difference between disk and memory. Please note that:
      1) the data structure should be appended only (after MCR released first version)
      2) the metadata verify conditon in its monitor will detect version difference, The
         new version developer is requried to set its defualt value when the new version
         software detects old version data in the metadata verify condition of specialize state
      3) database service maintain the committed metadata size, so any modification please
         invalidate the disk
    */
}
fbe_extent_pool_nonpaged_metadata_t;
#pragma pack()
#endif /* #ifndef UEFI_ENV */

/*!********************************************************************* 
 * @enum fbe_extent_pool_control_code_t 
 *  
 * @brief 
 * This enumeration lists all the base config specific control codes. 
 * These are control codes specific to this object, which only this object will accept. 
 *         
 **********************************************************************/
#ifndef UEFI_ENV
typedef enum fbe_extent_pool_control_code_e
{
    FBE_EXTENT_POOL_CONTROL_CODE_INVALID = FBE_OBJECT_CONTROL_CODE_INVALID_DEF(FBE_CLASS_ID_EXTENT_POOL),

    FBE_EXTENT_POOL_CONTROL_CODE_GET_INFO,
    FBE_EXTENT_POOL_CONTROL_CODE_CLASS_SET_OPTIONS,
    FBE_EXTENT_POOL_CONTROL_CODE_SET_CONFIGURATION,
    /* Insert new control codes here.
     */
    FBE_EXTENT_POOL_CONTROL_CODE_LAST
}
fbe_extent_pool_control_code_t;
#endif


/*!*******************************************************************
 * @struct fbe_extent_pool_get_info_t
 *********************************************************************
 * @brief
 *  This structure is used with the
 *  FBE_RAID_GROUP_CONTROL_CODE_GET_INFO control code.
 *
 *********************************************************************/
#ifndef UEFI_ENV
typedef struct fbe_extent_pool_get_info_s
{
    fbe_u32_t drives; /*!< Drives in pool*/
    fbe_block_count_t blocks_per_slice;
}
fbe_extent_pool_get_info_t;
#endif /* #ifndef UEFI_ENV */

/*!*******************************************************************
 * @struct fbe_extent_pool_class_set_options_t
 *********************************************************************
 * @brief   This structure is used with the following control codes: 
 *          o   FBE_EXTENT_POOL_CONTROL_CODE_CLASS_SET_OPTIONS
 *
 *********************************************************************/
#ifndef UEFI_ENV
typedef struct fbe_extent_pool_class_set_options_s
{
    fbe_block_count_t blocks_per_slice;
}
fbe_extent_pool_class_set_options_t;
#endif

typedef fbe_status_t (* fbe_extent_pool_hash_callback_function_t)(fbe_metadata_element_t *mde, 
																  fbe_ext_pool_lock_slice_entry_t * entry, 
																  void * context);

fbe_status_t fbe_extent_pool_slice_get_drive_position(void *slice,  
                                                      void *context,
                                                      fbe_u32_t *position_p);

fbe_status_t fbe_extent_pool_slice_get_position_offset(void *slice,  
                                                       void *context,
                                                       fbe_u32_t position,
                                                       fbe_lba_t *lba_p);
fbe_status_t fbe_extent_pool_class_get_blocks_per_slice(fbe_block_count_t *blocks_p);
fbe_status_t fbe_extent_pool_class_set_blocks_per_slice(fbe_block_count_t blocks);

fbe_metadata_lock_slot_state_t fbe_ext_pool_lock_get_slot_state(fbe_metadata_element_t *mde, fbe_lba_t start_lba);
void fbe_ext_pool_lock_set_slot_state(fbe_metadata_element_t *mde, fbe_lba_t start_lba, fbe_metadata_lock_slot_state_t set_state);
fbe_ext_pool_lock_slice_entry_t * fbe_ext_pool_lock_get_lock_table_entry(fbe_metadata_element_t *mde, fbe_lba_t start_lba);
void fbe_ext_pool_lock_get_slice_region(fbe_metadata_element_t *mde, fbe_lba_t start_lba, fbe_lba_t *region_first, fbe_lba_t *region_last);
void fbe_ext_pool_lock_traverse_hash_table(fbe_metadata_element_t *mde, fbe_extent_pool_hash_callback_function_t callback_function, void *context);

#endif /* FBE_EXTENT_POOL_H */

/*****************************
 * end file fbe_extent_pool.h
 *****************************/
