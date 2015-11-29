#ifndef EXTENT_POOL_PRIVATE_H
#define EXTENT_POOL_PRIVATE_H
/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_extent_pool_private.h
 ***************************************************************************
 *
 * @brief
 *  This file contains private defines for the extent pool object.
 *  This includes the definitions of the
 *  structures and defines.
 * 
 * @ingroup extent_pool_class_files
 * 
 * @version
 *   07/23/2009:  Created. Peter Puhov
 *
 ***************************************************************************/

/*
 * INCLUDE FILES:
 */

#include "csx_ext.h"
#include "fbe/fbe_transport.h"
#include "fbe_block_transport.h"
#include "fbe_base_config_private.h"
#include "fbe_raid_geometry.h"
#include "fbe/fbe_extent_pool.h"

/*
 * MACROS:
 */

//
// Description:
//
//    Macro to trace function entry in extent pool class.
//
// Synopsis:
//
//    FBE_EXTENT_POOL_TRACE_FUNC_ENTRY( object_p )
//
// Parameters:
//
//    object_p  -  pointer to extent pool object
//
#define FBE_EXTENT_POOL_TRACE_FUNC_ENTRY(m_object_p)                   \
                                                                           \
fbe_base_object_trace( (fbe_base_object_t *)m_object_p,                    \
                       FBE_TRACE_LEVEL_DEBUG_HIGH,                         \
                       FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,                \
                       "%s entry\n", __FUNCTION__                          \
                     )


/*
 * ENUMERATION CONSTANTS:
 */

/* Lifecycle definitions
 * Define the extent_pool lifecycle's instance.
 */
extern fbe_lifecycle_const_t FBE_LIFECYCLE_CONST_DATA(extent_pool);

/*! @enum fbe_extent_pool_lifecycle_cond_id_t  
 *  
 *  @brief These are the lifecycle conditions for the extent_pool object. 
 */
typedef enum fbe_extent_pool_lifecycle_cond_id_e 
{

    /*!< remap operation needs to be performed
     */
    FBE_EXTENT_POOL_LIFECYCLE_COND_INIT_POOL = FBE_LIFECYCLE_COND_FIRST_ID(FBE_CLASS_ID_EXTENT_POOL),


    FBE_EXTENT_POOL_LIFECYCLE_COND_LAST /* must be last */
} 
fbe_extent_pool_lifecycle_cond_id_t;


/*! @def FBE_PARITY_MAX_EDGES 
 *  @brief this is the max number of edges we will allow for the extent pool.
 *         If more edge is needed, we need to allocate memory for edges.
 */
#define FBE_EXTENT_POOL_MAX_WIDTH 960


typedef fbe_u64_t fbe_slice_address_t;
enum extent_pool_defines_e {
    FBE_EXTENT_POOL_SLICE_POSITIONS = 5,
    FBE_EXTENT_POOL_SLICE_SPARE_POSITIONS = 2,
    FBE_EXTENT_POOL_HASH_BUCKETS = 8 * 1024, /* Enough for 8 TB Drive */
    FBE_EXTENT_POOL_SLICE_BLOCKS = 0x800 * 0x400, /* 1 GB */
    FBE_EXTENT_POOL_SLICE_METADATA_BLOCKS = 128,
    FBE_EXTENT_POOL_GEOMETRIES = 2,
    FBE_EXTENT_POOL_METADATA_CHUNK_SIZE = 0x800,

    FBE_EXTENT_POOL_DISK_SLICE_FLAGS_SHIFT = 56,
    FBE_EXTENT_POOL_DISK_SLICE_FLAGS_MASK = 0xFF,
    FBE_EXTENT_POOL_DISK_SLICE_POSITION_SHIFT = 48,
    FBE_EXTENT_POOL_DISK_SLICE_POSITION_MASK = 0xFF,

    FBE_EXTENT_POOL_SLICE_LUN_SHIFT = 48,
    FBE_EXTENT_POOL_SLICE_LUN_MASK = 0xFFFF,
    FBE_EXTENT_POOL_START_OFFSET = 0x10000,
    FBE_EXTENT_POOL_END_RESERVED = 0x10000,
    FBE_EXTENT_POOL_MAX_LUNS = 64,
    FBE_EXTENT_POOL_MAX_SLICES = 4000,
    FBE_EXTENT_POOL_DEFAULT_WIDTH = 5,
    FBE_EXTENT_POOL_DEFAULT_DATA_DISKS = 5,
};
typedef fbe_u64_t fbe_extent_pool_slice_flags_t;


#define FBE_EXTENT_POOL_DISK_SLICE_LBA_MASK  0xFFFFFFFFFFFF


/*!*******************************************************************
 * @struct fbe_extent_pool_slice_t
 *********************************************************************
 * @brief
 *  This is the hash table for our slice map.
 * 
 *********************************************************************/
#pragma pack(1)
typedef struct fbe_extent_pool_slice_s
{
    fbe_queue_element_t queue_element;
    fbe_slice_address_t address;
    fbe_extent_pool_slice_flags_t flags;
    fbe_slice_address_t drive_map[FBE_EXTENT_POOL_SLICE_POSITIONS + FBE_EXTENT_POOL_SLICE_SPARE_POSITIONS];
    FBE_ALIGN(8) fbe_ext_pool_lock_slice_entry_t slice_stripe_lock;
}
fbe_extent_pool_slice_t;
#pragma pack()


/*!*******************************************************************
 * @struct fbe_extent_pool_hash_table_entry_t
 *********************************************************************
 * @brief
 *  This is the hash table entry structure for our slice map.
 * 
 *********************************************************************/
#pragma pack(1)
typedef struct fbe_extent_pool_hash_table_entry_s
{
    fbe_extent_pool_slice_t * slice_ptr;
}
fbe_extent_pool_hash_table_entry_t;
#pragma pack()

typedef fbe_extent_pool_hash_table_entry_t fbe_extent_pool_hash_table_t;
/*!*******************************************************************
 * @struct fbe_extent_pool_disk_slice_t
 *********************************************************************
 * @brief
 *  This is the set of information about the disk information.
 * 
 *********************************************************************/
#pragma pack(1)
typedef struct fbe_extent_pool_disk_slice_s
{
    fbe_slice_address_t extent_address;

    fbe_slice_address_t disk_address;
}
fbe_extent_pool_disk_slice_t;
#pragma pack()

enum fbe_disk_slice_address_flags_s {
    FBE_DISK_SLICE_ADDRESS_FLAG_NONE = 0,
    FBE_DISK_SLICE_ADDRESS_FLAG_ALLOCATED = 0x1,
};
static __forceinline fbe_lba_t fbe_extent_pool_disk_slice_address_get_lba(fbe_slice_address_t address)
{
   return (address & FBE_EXTENT_POOL_DISK_SLICE_LBA_MASK);
}
static __forceinline fbe_u8_t fbe_extent_pool_disk_slice_address_get_position(fbe_slice_address_t address)
{
   return ( (address >> FBE_EXTENT_POOL_DISK_SLICE_POSITION_SHIFT) & FBE_EXTENT_POOL_DISK_SLICE_POSITION_MASK);
}
static __forceinline fbe_u8_t fbe_extent_pool_disk_slice_address_get_flags(fbe_slice_address_t address)
{
   return ( (address >> FBE_EXTENT_POOL_DISK_SLICE_FLAGS_SHIFT) & FBE_EXTENT_POOL_DISK_SLICE_FLAGS_MASK);
}
static __forceinline void fbe_extent_pool_disk_slice_address_set(fbe_slice_address_t *address_p,
                                                                 fbe_lba_t lba,
                                                                 fbe_u8_t position,
                                                                 fbe_u8_t flags)
{
    *address_p = (((fbe_slice_address_t)position) << FBE_EXTENT_POOL_DISK_SLICE_POSITION_SHIFT) |
                 (((fbe_slice_address_t)flags) << FBE_EXTENT_POOL_DISK_SLICE_FLAGS_SHIFT)       |
                 lba;
}

static __forceinline fbe_lba_t fbe_extent_pool_slice_address_get_lba(fbe_slice_address_t address)
{
   return (address & FBE_EXTENT_POOL_DISK_SLICE_LBA_MASK);
}
static __forceinline fbe_u16_t fbe_extent_pool_slice_address_get_lun(fbe_slice_address_t address)
{
   return ( (address >> FBE_EXTENT_POOL_SLICE_LUN_SHIFT) & FBE_EXTENT_POOL_SLICE_LUN_MASK);
}
static __forceinline void fbe_extent_pool_slice_address_set(fbe_slice_address_t *address_p,
                                                            fbe_lba_t lba,
                                                            fbe_u16_t lun)
{
    *address_p = (((fbe_slice_address_t)lun) << FBE_EXTENT_POOL_SLICE_LUN_SHIFT) |
                 lba;
}
/*!*******************************************************************
 * @struct fbe_extent_pool_disk_info_t
 *********************************************************************
 * @brief
 *  This is the set of information about the disk information.
 * 
 *********************************************************************/
#pragma pack(1)
typedef struct fbe_extent_pool_disk_info_s
{
    fbe_block_count_t capacity;
    fbe_object_id_t object_id;
    fbe_slice_index_t free_slice_index;
    fbe_extent_pool_disk_slice_t *drive_map_table_p;
}
fbe_extent_pool_disk_info_t;
#pragma pack()
/*!****************************************************************************
 *    
 * @struct fbe_extent_pool_t
 *  
 * @brief 
 *   This is the definition of the extent pool object.
 *   This object represents a extent pool.
 ******************************************************************************/
typedef struct fbe_extent_pool_s
{    
    /*! This must be first.  This is the object we inherit from. */
    fbe_base_config_t base_config;

    /*! Metadata memory. */
    fbe_extent_pool_metadata_memory_t extent_pool_metadata_memory;
    fbe_extent_pool_metadata_memory_t extent_pool_metadata_memory_peer;

    fbe_metadata_stripe_lock_blob_t stripe_lock_blob;

    /*! block edge pointer to downtream edge. */
    fbe_u32_t         pool_id;
    
    fbe_u32_t         pool_width; /*!< Total number of drives in pool. */

    fbe_slice_count_t total_slices;

    fbe_extent_pool_hash_table_t *hash_table; /*! Array of ptrs to LUN hash tables. */
    fbe_extent_pool_slice_t      *slice_p; /*!< Slice memory */
    fbe_queue_head_t slice_list; /* Free slice list. */
    fbe_extent_pool_disk_info_t  *disk_info_p; /*!< Disk info memory.*/
    fbe_raid_geometry_t *geometry_p;

    /* Lifecycle defines. */
    FBE_LIFECYCLE_DEF_INST_DATA;
    FBE_LIFECYCLE_DEF_INST_COND(FBE_LIFECYCLE_COND_MAX(FBE_EXTENT_POOL_LIFECYCLE_COND_LAST));
} fbe_extent_pool_t;

static __forceinline fbe_status_t 
fbe_extent_pool_get_slice_count(fbe_extent_pool_t *extent_pool_p, fbe_slice_count_t *slice_count_p)
{
    /* @todo Eventually set to actual pool capacity. 
     */ 
    *slice_count_p = FBE_EXTENT_POOL_HASH_BUCKETS;
    return FBE_STATUS_OK;
}
fbe_status_t fbe_extent_pool_class_get_blocks_per_slice(fbe_block_count_t *blocks_p);
static __forceinline fbe_status_t 
fbe_extent_pool_get_slice_blocks(fbe_extent_pool_t *extent_pool_p, fbe_block_count_t *slice_blocks_p)
{
    fbe_extent_pool_class_get_blocks_per_slice(slice_blocks_p);
    return FBE_STATUS_OK;
}
static __forceinline fbe_status_t 
fbe_extent_pool_get_pool_blocks(fbe_extent_pool_t *extent_pool_p, fbe_block_count_t *blocks_p)
{
    *blocks_p = extent_pool_p->total_slices * FBE_EXTENT_POOL_SLICE_BLOCKS;
    return FBE_STATUS_OK;
}

static __forceinline fbe_status_t 
fbe_extent_pool_get_pool_slices(fbe_extent_pool_t *extent_pool_p, fbe_slice_count_t *slice_count_p)
{
    *slice_count_p = extent_pool_p->total_slices;
    return FBE_STATUS_OK;
}
static __forceinline fbe_status_t 
fbe_extent_pool_get_pool_width(fbe_extent_pool_t *extent_pool_p, fbe_u32_t *pool_width_p)
{
    *pool_width_p = extent_pool_p->pool_width;
    return FBE_STATUS_OK;
}


static __forceinline fbe_status_t 
fbe_extent_pool_get_slice_metadata_blocks(fbe_extent_pool_t *extent_pool_p, fbe_block_count_t *slice_blocks_p)
{
    *slice_blocks_p = FBE_EXTENT_POOL_SLICE_METADATA_BLOCKS;
    return FBE_STATUS_OK;
}

static __forceinline void fbe_extent_pool_lock(fbe_extent_pool_t *extent_pool_p)
{
    fbe_base_object_lock((fbe_base_object_t *)extent_pool_p);
    return;
}
static __forceinline void fbe_extent_pool_unlock(fbe_extent_pool_t *extent_pool_p)
{
    fbe_base_object_unlock((fbe_base_object_t *)extent_pool_p);
    return;
}

/* fbe_extent_pool_main.c */
fbe_status_t fbe_extent_pool_load(void);
fbe_status_t fbe_extent_pool_unload(void);
fbe_status_t fbe_extent_pool_create_object(fbe_packet_t * packet, fbe_object_handle_t * object_handle);
fbe_status_t fbe_extent_pool_destroy_object( fbe_object_handle_t object_handle);

fbe_status_t fbe_extent_pool_io_entry(fbe_object_handle_t object_handle, fbe_packet_t * packet_p);
fbe_status_t fbe_extent_pool_monitor_entry(fbe_object_handle_t object_handle, fbe_packet_t * packet_p);
fbe_status_t fbe_extent_pool_get_disk_info(fbe_extent_pool_t *extent_pool_p,
                                           fbe_u32_t position,
                                           fbe_extent_pool_disk_info_t **disk_info_p);

fbe_status_t fbe_extent_pool_get_user_pool_geometry(fbe_extent_pool_t *extent_pool_p,
                                                    fbe_raid_geometry_t **geometry_p);
/* fbe_extent_pool_executor.c */
fbe_status_t fbe_extent_pool_block_transport_entry(fbe_transport_entry_context_t context, fbe_packet_t * packet);

/* fbe_extent_pool_monitor.c */
fbe_status_t fbe_extent_pool_monitor_load_verify(void);

/* fbe_extent_pool_usurper.c */
fbe_status_t fbe_extent_pool_control_entry(fbe_object_handle_t object_handle, fbe_packet_t * packet);

/* fbe_extent_pool_event.c */
fbe_status_t fbe_extent_pool_event_entry(fbe_object_handle_t object_handle, 
                                             fbe_event_type_t event_type,
                                             fbe_event_context_t event_context);
/* fbe_extent_pool_map.c */
fbe_status_t fbe_extent_pool_release_memory(fbe_extent_pool_t *extent_pool_p);
fbe_status_t fbe_extent_pool_hash_lookup_slice(fbe_extent_pool_t *extent_pool_p,
                                               fbe_slice_address_t address,
                                               fbe_u32_t lun,
                                               fbe_extent_pool_slice_t **slice_p);
fbe_status_t fbe_extent_pool_allocate_slice(fbe_extent_pool_t *extent_pool_p,
                                            fbe_extent_pool_slice_t **slice_p);
fbe_status_t fbe_extent_pool_deallocate_slice(fbe_extent_pool_t *extent_pool_p,
                                            fbe_extent_pool_slice_t *slice_p);
fbe_status_t fbe_extent_pool_get_hash_table(fbe_extent_pool_t *extent_pool_p,
                                            fbe_extent_pool_hash_table_t **hash_table_p);

fbe_status_t fbe_extent_pool_hash_insert_slice(fbe_extent_pool_t *extent_pool_p,
                                               fbe_u32_t lun,
                                               fbe_extent_pool_slice_t *slice_p);
fbe_status_t fbe_extent_pool_fully_map_lun(fbe_extent_pool_t *extent_pool_p,
                                           fbe_u32_t lun,
                                           fbe_block_count_t capacity,
                                           fbe_block_count_t offset);
fbe_status_t fbe_extent_pool_fully_map_pool(fbe_extent_pool_t *extent_pool_p);
fbe_status_t fbe_extent_pool_construct_lun_slices(fbe_extent_pool_t *extent_pool_p,
                                                  fbe_u32_t lun);
fbe_status_t fbe_extent_pool_construct_user_slices(fbe_extent_pool_t *extent_pool_p);

/* fbe_extent_pool_class.c */
fbe_status_t fbe_extent_pool_class_control_entry(fbe_packet_t * packet);
fbe_status_t fbe_extent_pool_class_init_slice_memory(void);
fbe_status_t fbe_extent_pool_class_release_slice_memory(void);

fbe_u64_t fbe_extent_pool_hash(fbe_extent_pool_t *extent_pool_p,
                               fbe_slice_address_t address);

fbe_status_t fbe_extent_pool_class_get_blocks_per_disk_slice(fbe_block_count_t *blocks_p);

#endif /* EXTENT_POOL_PRIVATE_H */

/*******************************
 * end fbe_extent_pool_private.h
 *******************************/


