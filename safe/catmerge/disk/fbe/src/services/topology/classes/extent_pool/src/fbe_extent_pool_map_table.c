/***************************************************************************
 * Copyright (C) EMC Corporation 2014
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_extent_pool_map_table.c
 ***************************************************************************
 *
 * @brief
 *  This file contains functions for the mapping tables of the extent pool.
 *
 * @version
 *   6/18/2014:  Created. Rob Foley
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe_extent_pool_private.h"
#include "fbe_raid_library.h"
#include "fbe_traffic_trace.h"
#include "fbe_service_manager.h"
#include "fbe_transport_memory.h"
#include "fbe/fbe_sector.h"

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

static fbe_status_t fbe_extent_pool_hash_init_slice_lock(fbe_extent_pool_t *extent_pool_p,
                                                         fbe_extent_pool_slice_t *slice_p);

/*!**************************************************************
 * fbe_extent_pool_hash()
 ****************************************************************
 * @brief
 *  calculate hash for slice.
 *
 * @param extent_pool_p - Pool object.
 * @param lun - lun number
 * @param address - address of this slice.
 *
 * @return fbe_u64_t
 *
 * @author
 *  6/11/2014 - Created. Rob Foley
 *
 ****************************************************************/

fbe_u64_t fbe_extent_pool_hash(fbe_extent_pool_t *extent_pool_p,
                               fbe_slice_address_t address)
{
    fbe_block_count_t            blocks_per_slice;
    fbe_u16_t                    data_disks;
    fbe_raid_geometry_t         *geometry_p = NULL;
    fbe_lba_t                    lba;

    fbe_extent_pool_get_user_pool_geometry(extent_pool_p, &geometry_p);
    fbe_raid_geometry_get_data_disks(geometry_p, &data_disks);
    fbe_extent_pool_class_get_blocks_per_slice(&blocks_per_slice);

    lba = fbe_extent_pool_slice_address_get_lba(address);
    return lba / (blocks_per_slice * data_disks);
}
/******************************************
 * end fbe_extent_pool_hash()
 ******************************************/

fbe_status_t fbe_extent_pool_get_hash_table(fbe_extent_pool_t *extent_pool_p,
                                            fbe_extent_pool_hash_table_t **hash_table_p)
{
    *hash_table_p = extent_pool_p->hash_table;
    return FBE_STATUS_OK;
}
/*!**************************************************************
 * fbe_extent_pool_hash_insert_slice()
 ****************************************************************
 * @brief
 *  Put slice into hash table.
 *
 * @param extent_pool_p - Pool object.
 * @param slice_p - Slice to insert to hash table.
 *
 * @return fbe_status_t
 *
 * @author
 *  6/11/2014 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_extent_pool_hash_insert_slice(fbe_extent_pool_t *extent_pool_p,
                                               fbe_u32_t lun,
                                               fbe_extent_pool_slice_t *slice_p)
{
    fbe_u64_t slice_hash = fbe_extent_pool_hash(extent_pool_p, slice_p->address);
    fbe_extent_pool_hash_table_t *hash_table_p = NULL;
    fbe_extent_pool_get_hash_table(extent_pool_p, &hash_table_p);
    hash_table_p[slice_hash].slice_ptr = slice_p;
    fbe_extent_pool_hash_init_slice_lock(extent_pool_p, slice_p);
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_extent_pool_hash_insert_slice()
 ******************************************/

/*!**************************************************************
 * fbe_extent_pool_hash_lookup_slice()
 ****************************************************************
 * @brief
 *  Fetch the slice from the table.
 *
 * @param extent_pool_p - Pool object.
 * @param slice_p - Slice to insert to hash table.
 *
 * @return fbe_status_t
 *
 * @author
 *  6/11/2014 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_extent_pool_hash_lookup_slice(fbe_extent_pool_t *extent_pool_p,
                                               fbe_slice_address_t address,
                                               fbe_u32_t lun,
                                               fbe_extent_pool_slice_t **slice_p)
{
    fbe_u64_t slice_hash = fbe_extent_pool_hash(extent_pool_p, address);
    fbe_extent_pool_hash_table_t *hash_table_p = NULL;
    fbe_extent_pool_get_hash_table(extent_pool_p, &hash_table_p);
    *slice_p = hash_table_p[slice_hash].slice_ptr;
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_extent_pool_hash_lookup_slice()
 ******************************************/

/*!**************************************************************
 * fbe_extent_pool_hash_init_slice_lock()
 ****************************************************************
 * @brief
 *  Initialize lock state for the slice.
 *
 * @param extent_pool_p - Pool object.
 * @param slice_p - Slice to insert to hash table.
 *
 * @return fbe_status_t
 *
 * @author
 *  6/17/2014 - Created. Lili Chen
 *
 ****************************************************************/
static fbe_status_t 
fbe_extent_pool_hash_init_slice_lock(fbe_extent_pool_t *extent_pool_p,
                                     fbe_extent_pool_slice_t *slice_p)
{
    fbe_ext_pool_lock_slice_entry_t * lock_p;
    fbe_u64_t state;

    if(((fbe_base_config_t *)extent_pool_p)->metadata_element.metadata_element_state == FBE_METADATA_ELEMENT_STATE_ACTIVE){
        state = METADATA_STRIPE_LOCK_SLOT_STATE_EXCLUSIVE_LOCAL; /* Exclusive local lock */
    } else {
        state = METADATA_STRIPE_LOCK_SLOT_STATE_EXCLUSIVE_PEER; /* Exclusive local lock */
    }

    lock_p = &slice_p->slice_stripe_lock;
    lock_p->slice_lock_state = state;
    fbe_queue_init(&lock_p->head);
    lock_p->lock = (void *)slice_p;

    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_extent_pool_hash_init_slice_lock()
 ******************************************/


/*!**************************************************************
 * fbe_extent_pool_construct_user_slices()
 ****************************************************************
 * @brief
 *  This generates all the user slices via a reverse lookup
 *  from the drive map.
 *
 * @param extent_pool_p - Pool object.               
 *
 * @return fbe_status_t
 *
 * @author
 *  6/13/2014 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_extent_pool_construct_user_slices(fbe_extent_pool_t *extent_pool_p)
{
    fbe_extent_pool_slice_t      *slice_p = NULL;
    fbe_extent_pool_disk_slice_t *disk_slice_p = NULL;
    fbe_u32_t                    disk_index;
    fbe_extent_pool_disk_info_t *disk_info_p = NULL;
    fbe_slice_count_t            disk_slices;
    fbe_slice_index_t            slice_index;
    fbe_u8_t                     position;
    fbe_lba_t                    disk_lba;
    fbe_lba_t                    lba;
    fbe_u32_t                    lun;
    fbe_u32_t                    pool_width;
    fbe_block_count_t            blocks_per_disk_slice;
    fbe_u8_t                     flags;

    fbe_extent_pool_class_get_blocks_per_disk_slice(&blocks_per_disk_slice);

    fbe_extent_pool_get_pool_width(extent_pool_p, &pool_width);
    /* Perform reverse lookup of the extent.
     */
    for (disk_index = 0; disk_index < pool_width; disk_index++) {

        /* Initialize the user slices.
         * Associate the user slice with the disk. 
         */
        fbe_extent_pool_get_disk_info(extent_pool_p, disk_index, &disk_info_p);
        disk_slices = disk_info_p->capacity / blocks_per_disk_slice;

        for (slice_index = 0; slice_index < disk_slices; slice_index++) {

            /* Get the disk slice.
             */
            disk_slice_p = &disk_info_p->drive_map_table_p[slice_index];
            flags = fbe_extent_pool_disk_slice_address_get_flags(disk_slice_p->disk_address);
            if (flags & FBE_DISK_SLICE_ADDRESS_FLAG_ALLOCATED) {
                position = fbe_extent_pool_disk_slice_address_get_position(disk_slice_p->disk_address);
                disk_lba = fbe_extent_pool_disk_slice_address_get_lba(disk_slice_p->disk_address);
                lun = fbe_extent_pool_slice_address_get_lun(disk_slice_p->extent_address);
                lba = fbe_extent_pool_slice_address_get_lba(disk_slice_p->extent_address);
    
                /* Find the user slice.  If it does not exist, then create it and put it in the table.
                 */
                fbe_extent_pool_hash_lookup_slice(extent_pool_p, lba, lun, &slice_p);
                if (slice_p == NULL) {
                    fbe_extent_pool_allocate_slice(extent_pool_p, &slice_p);
                    fbe_extent_pool_slice_address_set(&slice_p->address, lba, lun);
                    fbe_extent_pool_hash_insert_slice(extent_pool_p, lun, slice_p);
                }
                /* Setup the information for this position.
                 */
                fbe_extent_pool_disk_slice_address_set(&slice_p->drive_map[position],
                                                       disk_lba, 
                                                       disk_index, /* Disk position in pool */
                                                       0 /* flags */);
            }
        }
    }
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_extent_pool_construct_user_slices()
 ******************************************/


/*!**************************************************************
 * fbe_extent_pool_construct_lun_slices()
 ****************************************************************
 * @brief
 *  This generates all the user slices via a reverse lookup
 *  from the drive map.
 *
 * @param extent_pool_p - Pool object.               
 *
 * @return fbe_status_t
 *
 * @author
 *  6/19/2014 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_extent_pool_construct_lun_slices(fbe_extent_pool_t *extent_pool_p,
                                                  fbe_u32_t lun)
{
    fbe_extent_pool_slice_t      *slice_p = NULL;
    fbe_extent_pool_disk_slice_t *disk_slice_p = NULL;
    fbe_u32_t                    disk_index;
    fbe_extent_pool_disk_info_t *disk_info_p = NULL;
    fbe_slice_count_t            disk_slices;
    fbe_slice_index_t            slice_index;
    fbe_u8_t                     position;
    fbe_lba_t                    disk_lba;
    fbe_lba_t                    lba;
    fbe_u32_t                    current_lun;
    fbe_u32_t                    pool_width;
    fbe_block_count_t            blocks_per_disk_slice;

    fbe_extent_pool_class_get_blocks_per_disk_slice(&blocks_per_disk_slice);

    fbe_extent_pool_get_pool_width(extent_pool_p, &pool_width);
    /* Perform reverse lookup of the extent.
     */
    for (disk_index = 0; disk_index < pool_width; disk_index++) {

        /* Initialize the user slices.
         * Associate the user slice with the disk. 
         */
        fbe_extent_pool_get_disk_info(extent_pool_p, disk_index, &disk_info_p);
        disk_slices = disk_info_p->capacity / blocks_per_disk_slice;

        for (slice_index = 0; slice_index < disk_slices; slice_index++) {

            /* Get the disk slice.
             */
            disk_slice_p = &disk_info_p->drive_map_table_p[slice_index];
            position = fbe_extent_pool_disk_slice_address_get_position(disk_slice_p->disk_address);
            disk_lba = fbe_extent_pool_disk_slice_address_get_lba(disk_slice_p->disk_address);
            current_lun = fbe_extent_pool_slice_address_get_lun(disk_slice_p->extent_address);

            if (current_lun == lun) {
                lba = fbe_extent_pool_slice_address_get_lba(disk_slice_p->extent_address);
    
                /* Find the user slice.  If it does not exist, then create it and put it in the table.
                 */
                fbe_extent_pool_hash_lookup_slice(extent_pool_p, lba, current_lun, &slice_p);
                if (slice_p == NULL) {
                    fbe_extent_pool_allocate_slice(extent_pool_p, &slice_p);
                    fbe_extent_pool_slice_address_set(&slice_p->address, lba, current_lun);
                    fbe_extent_pool_hash_insert_slice(extent_pool_p, current_lun, slice_p);
                }
                /* Setup the information for this position.
                 */
                fbe_extent_pool_disk_slice_address_set(&slice_p->drive_map[position],
                                                       disk_lba, 
                                                       disk_index, /* Disk position in pool */
                                                       0 /* flags */);
            }
        }
    }
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_extent_pool_construct_lun_slices()
 ******************************************/

/*!**************************************************************
 * fbe_extent_pool_disk_info_get_free_slice()
 ****************************************************************
 * @brief
 *  Populates the disk slices as if they were already allocated.
 *
 * @param extent_pool_p - Pool object.
 * @param disk_info_p -
 * @param slice_p - output free slice.
 *
 * @return fbe_status_t
 *
 * @author
 *  6/18/2014 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_extent_pool_disk_info_get_free_slice(fbe_extent_pool_t *extent_pool_p,
                                                      fbe_extent_pool_disk_info_t *disk_info_p,
                                                      fbe_slice_index_t disk_slice_offset,
                                                      fbe_extent_pool_disk_slice_t **slice_p)
{
    fbe_slice_index_t max_slices;
    fbe_block_count_t blocks_per_disk_slice;
    fbe_u8_t          flags;
    fbe_slice_index_t slice_index;

    fbe_extent_pool_class_get_blocks_per_disk_slice(&blocks_per_disk_slice);
    max_slices = disk_info_p->capacity / blocks_per_disk_slice;

    if (disk_info_p->free_slice_index >= max_slices) {
        *slice_p = NULL;
        return FBE_STATUS_INSUFFICIENT_RESOURCES;
    }

    disk_slice_offset = disk_info_p->free_slice_index;
    for (slice_index = disk_slice_offset; slice_index < max_slices; slice_index++) {
        
        flags = fbe_extent_pool_disk_slice_address_get_flags(disk_info_p->drive_map_table_p[slice_index].disk_address);

        if ((flags & FBE_DISK_SLICE_ADDRESS_FLAG_ALLOCATED) == 0) {
            *slice_p = &disk_info_p->drive_map_table_p[slice_index];
            disk_info_p->free_slice_index = slice_index + 1;
            return FBE_STATUS_OK;
        }
    }
    *slice_p = NULL;
    disk_info_p->free_slice_index = max_slices;
    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_extent_pool_disk_info_get_free_slice()
 **************************************/

/*!**************************************************************
 * fbe_extent_pool_fully_map_lun()
 ****************************************************************
 * @brief
 *  Populates the disk slices as if they were already allocated.
 *
 * @param extent_pool_p - Pool object.               
 *
 * @return fbe_status_t
 *
 * @author
 *  6/13/2014 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_extent_pool_fully_map_lun(fbe_extent_pool_t *extent_pool_p,
                                           fbe_u32_t lun,
                                           fbe_block_count_t capacity,
                                           fbe_block_count_t offset)
{
    fbe_extent_pool_disk_slice_t *slice_p = NULL;
    fbe_u32_t                    pool_index;
    fbe_u32_t                    array_index;
    fbe_extent_pool_disk_info_t *disk_info_p = NULL;
    fbe_slice_count_t            lun_slices;
    fbe_slice_index_t            slice_index;
    fbe_lba_t                    lba;
    fbe_lba_t                    disk_lba;
    fbe_block_count_t            blocks_per_slice;
    fbe_u32_t                    width;
    fbe_u32_t                    pool_width;
    fbe_u16_t                    data_disks;
    fbe_raid_geometry_t         *geometry_p = NULL;
    fbe_block_count_t            pool_slice_size;
    fbe_slice_index_t            pool_slice_start;

    fbe_extent_pool_class_get_blocks_per_slice(&blocks_per_slice);
    fbe_extent_pool_get_user_pool_geometry(extent_pool_p, &geometry_p);
    fbe_raid_geometry_get_width(geometry_p, &width);
    fbe_raid_geometry_get_data_disks(geometry_p, &data_disks);
    fbe_extent_pool_get_pool_width(extent_pool_p, &pool_width);
    pool_slice_size = (pool_width / width) * data_disks * blocks_per_slice;

    pool_slice_start = offset / pool_slice_size;
    if (offset % pool_slice_size) {
        pool_slice_start++;
    }
    lun_slices = capacity / (blocks_per_slice * data_disks);
    fbe_base_object_trace((fbe_base_object_t*)extent_pool_p, 
                          FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                          "map: lun %u capacity: %llx offset: %llx start_slice: 0x%llx\n",
                          lun, capacity, offset, pool_slice_start);
    fbe_extent_pool_lock(extent_pool_p);
    /* For every slice in the LUN, initialize the disk addresses.
     */
    lba = 0;
    pool_index = 0;
    for (slice_index = 0; slice_index < lun_slices; slice_index++) {
        array_index = 0;
        while (array_index < width) {
            fbe_extent_pool_get_disk_info(extent_pool_p, pool_index, &disk_info_p);
            
            fbe_extent_pool_disk_info_get_free_slice(extent_pool_p, disk_info_p, pool_slice_start, &slice_p);
            if (slice_p == NULL) {
                pool_index++;
                if (pool_index >= pool_width) {
                    fbe_base_object_trace((fbe_base_object_t*)extent_pool_p, 
                                          FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                          "Ran out of disk slices\n");
                    fbe_extent_pool_unlock(extent_pool_p);
                    return FBE_STATUS_GENERIC_FAILURE;
                }
                continue;
            }
            disk_lba = fbe_extent_pool_disk_slice_address_get_lba(slice_p->disk_address);
            /* Set the position and lba in the slice.
             */
            fbe_extent_pool_disk_slice_address_set(&slice_p->disk_address,
                                                   disk_lba, 
                                                   array_index,    /* Position in slice */
                                                   FBE_DISK_SLICE_ADDRESS_FLAG_ALLOCATED);
            /* Set the host lba.
             */
            fbe_extent_pool_slice_address_set(&slice_p->extent_address, lba, lun);
            fbe_base_object_trace((fbe_base_object_t*)extent_pool_p, 
                                  FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                  "map: slice: %p lun %u lba: %llx slice: %llx array_pos: %u \n",
                                  slice_p, lun, lba, slice_index, array_index);
            fbe_base_object_trace((fbe_base_object_t*)extent_pool_p, 
                                  FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                  "map: lun: %u disk_addr: %llx disk_lba: 0x%llx pool_idx: 0x%x\n",
                                  lun, slice_p->disk_address, disk_lba, pool_index);
            array_index++;
            pool_index++;
            if (pool_index >= pool_width) {
                pool_index = 0;
            }
        }
        lba += blocks_per_slice * data_disks;
    }
    fbe_extent_pool_unlock(extent_pool_p);
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_extent_pool_fully_map_lun()
 ******************************************/


/*!**************************************************************
 * fbe_extent_pool_fully_map_pool()
 ****************************************************************
 * @brief
 *  Populates the disk slices as if they were already allocated.
 *
 * @param extent_pool_p - Pool object.               
 *
 * @return fbe_status_t
 *
 * @author
 *  6/24/2014 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_extent_pool_fully_map_pool(fbe_extent_pool_t *extent_pool_p)
{
    fbe_extent_pool_disk_slice_t *slice_p = NULL;
    fbe_u32_t                    pool_index;
    fbe_u32_t                    array_index;
    fbe_extent_pool_disk_info_t *disk_info_p = NULL;
    fbe_slice_count_t            pool_slices;
    fbe_slice_index_t            slice_index;
    fbe_lba_t                    lba;
    fbe_lba_t                    disk_lba;
    fbe_block_count_t            blocks_per_slice;
    fbe_u32_t                    width;
    fbe_u32_t                    pool_width;
    fbe_u16_t                    data_disks;
    fbe_raid_geometry_t         *geometry_p = NULL;
    fbe_block_count_t            pool_slice_size;
    fbe_block_count_t            capacity;
    fbe_extent_pool_disk_slice_t *slice_array[FBE_RAID_MAX_DISK_ARRAY_WIDTH];

    fbe_extent_pool_class_get_blocks_per_slice(&blocks_per_slice);
    fbe_extent_pool_get_user_pool_geometry(extent_pool_p, &geometry_p);
    fbe_raid_geometry_get_width(geometry_p, &width);
    fbe_raid_geometry_get_data_disks(geometry_p, &data_disks);
    fbe_extent_pool_get_pool_width(extent_pool_p, &pool_width);
    pool_slice_size = (pool_width / width) * data_disks * blocks_per_slice;

    capacity = extent_pool_p->total_slices * blocks_per_slice;
    pool_slices = extent_pool_p->total_slices;
    fbe_base_object_trace((fbe_base_object_t*)extent_pool_p, 
                          FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                          "map: fully map pool with capacity: 0x%llx total_slices: 0x%llx\n",
                          capacity, pool_slices);

    fbe_extent_pool_lock(extent_pool_p);
    /* For every slice in the pool, initialize the disk addresses.
     */
    lba = 0;
    pool_index = 0;
    for (slice_index = 0; slice_index < pool_slices; slice_index++) {
        array_index = 0;
        while (array_index < width) {
            fbe_extent_pool_get_disk_info(extent_pool_p, pool_index, &disk_info_p);
            
            fbe_extent_pool_disk_info_get_free_slice(extent_pool_p, disk_info_p, 0, &slice_p);
            if (slice_p == NULL) {
                pool_index++;
                if (pool_index >= pool_width) {
                    fbe_lba_t total_capacity;

                    extent_pool_p->total_slices = slice_index;
                    total_capacity = extent_pool_p->total_slices * blocks_per_slice * FBE_EXTENT_POOL_DEFAULT_DATA_DISKS;

                    fbe_base_object_trace((fbe_base_object_t*)extent_pool_p, 
                                          FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                          "total slices is: 0x%llx total_capacity: 0x%llx\n",
                                          extent_pool_p->total_slices, total_capacity);

                    fbe_base_config_set_capacity((fbe_base_config_t*)extent_pool_p, total_capacity);
                    for (slice_index = 0; slice_index < array_index; slice_index++) {
                        slice_array[slice_index]->disk_address = 0;
                        slice_array[slice_index]->extent_address = 0;
                    }
                    fbe_extent_pool_unlock(extent_pool_p);
                    return FBE_STATUS_OK;
                }
                continue;
            }
            disk_lba = fbe_extent_pool_disk_slice_address_get_lba(slice_p->disk_address);
            /* Set the position and lba in the slice.
             */
            fbe_extent_pool_disk_slice_address_set(&slice_p->disk_address,
                                                   disk_lba, 
                                                   array_index,    /* Position in slice */
                                                   FBE_DISK_SLICE_ADDRESS_FLAG_ALLOCATED);
            /* Set the host lba.
             */
            fbe_extent_pool_slice_address_set(&slice_p->extent_address, lba, 0 /* no LUN */);
            fbe_base_object_trace((fbe_base_object_t*)extent_pool_p, 
                                  FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                  "map: slice: %p  lba: %llx slice: %llx array_pos: %u \n",
                                  slice_p, lba, slice_index, array_index);
            fbe_base_object_trace((fbe_base_object_t*)extent_pool_p, 
                                  FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                  "map: disk_addr: %llx disk_lba: 0x%llx pool_idx: 0x%x\n",
                                  slice_p->disk_address, disk_lba, pool_index);
            slice_array[array_index] = slice_p;
            array_index++;
            pool_index++;
            if (pool_index >= pool_width) {
                pool_index = 0;
            }
        }
        lba += blocks_per_slice * data_disks;
    }
    fbe_extent_pool_unlock(extent_pool_p);
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_extent_pool_fully_map_pool()
 ******************************************/
/*************************
 * end file fbe_extent_pool_map_table.c
 *************************/


