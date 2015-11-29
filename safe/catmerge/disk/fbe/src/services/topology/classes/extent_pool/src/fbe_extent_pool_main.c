/***************************************************************************
 * Copyright (C) EMC Corporation 2007-2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_extent_pool_main.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the main entry points for the extent_pool object.
 *  This includes the create and destroy methods for the extent_pool, as
 *  well as the event entry point, load and unload methods.
 * 
 * @ingroup extent_pool_class_files
 * 
 * @version
 *   06/06/2014:  Created.
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_winddk.h"
//#include "fbe/fbe_provision_drive.h"
#include "base_object_private.h"
#include "fbe_extent_pool_private.h"
#include "fbe/fbe_notification_interface.h"
#include "fbe_notification.h"
#include "fbe_transport_memory.h"
#include "fbe_raid_library.h"
#include "fbe_service_manager.h"
#include "fbe_database.h" 
#include "fbe/fbe_time.h" 
#include "fbe_private_space_layout.h"


/* Export class methods.
 */
fbe_class_methods_t fbe_extent_pool_class_methods = {FBE_CLASS_ID_EXTENT_POOL,
                                                         fbe_extent_pool_load,
                                                         fbe_extent_pool_unload,
                                                         fbe_extent_pool_create_object,
                                                         fbe_extent_pool_destroy_object,
                                                         fbe_extent_pool_control_entry,
                                                         fbe_extent_pool_event_entry,
                                                         fbe_extent_pool_io_entry,
                                                         fbe_extent_pool_monitor_entry};

fbe_block_transport_const_t fbe_extent_pool_block_transport_const = {fbe_extent_pool_block_transport_entry,
																		 fbe_base_config_process_block_transport_event,
																		 fbe_extent_pool_io_entry,
                                                                         NULL, NULL};

/*****************************************
 * LOCAL FUNCTION DEFINITIONS
 *****************************************/
static fbe_status_t fbe_extent_pool_init(fbe_extent_pool_t * const extent_pool_p);


/*!***************************************************************
 * fbe_extent_pool_load()
 ****************************************************************
 * @brief
 *  This function is called to construct any
 *  global data for the class.
 *
 * @param  - None.
 *
 * @return fbe_status_t
 *
 * @author
 *   06/06/2014:  Created.
 *
 ****************************************************************/
fbe_status_t 
fbe_extent_pool_load(void)
{
    fbe_status_t status;
    
    FBE_ASSERT_AT_COMPILE_TIME(sizeof(fbe_extent_pool_t) < FBE_MEMORY_CHUNK_SIZE);
    //FBE_ASSERT_AT_COMPILE_TIME(sizeof(fbe_provision_drive_nonpaged_metadata_t) < FBE_METADATA_NONPAGED_MAX_SIZE);

    status = fbe_extent_pool_monitor_load_verify();

    if (status != FBE_STATUS_OK) { 
        return status; 
    }

    status = fbe_extent_pool_class_init_slice_memory();
    return status;
}
/* end fbe_extent_pool_load() */

/*!***************************************************************
 * fbe_extent_pool_unload()
 ****************************************************************
 * @brief
 *  This function is called to tear down any
 *  global data for the class.
 *
 * @param  - None.
 *
 * @return fbe_status_t
 *
 * @author
 *   06/06/2014:  Created.
 *
 ****************************************************************/
fbe_status_t 
fbe_extent_pool_unload(void)
{
    /* Destroy any global data.
     */
    fbe_extent_pool_class_release_slice_memory();
    return FBE_STATUS_OK;
}
/* end fbe_extent_pool_unload() */

/*!***************************************************************
 * fbe_extent_pool_create_object()
 ****************************************************************
 * @brief
 *  This function is called to create an instance of this class.
 *
 * @param packet_p - The packet used to construct the object.  
 * @param object_handle - The object being created.
 *
 * @return fbe_status_t
 *  The status of the operation typically FBE_STATUS_OK.
 *
 * @author
 *   06/06/2014:  Created.
 *
 ****************************************************************/
fbe_status_t 
fbe_extent_pool_create_object(fbe_packet_t * packet_p, fbe_object_handle_t * object_handle)
{
    fbe_extent_pool_t * extent_pool_p = NULL;
    fbe_status_t status;

    /* Call the base class create function. */
    status = fbe_base_config_create_object(packet_p, object_handle);    
    if(status != FBE_STATUS_OK){
        return status;
    }

    extent_pool_p = (fbe_extent_pool_t *)fbe_base_handle_to_pointer(*object_handle);

    /* Set class id.*/
    fbe_base_object_set_class_id((fbe_base_object_t *) extent_pool_p, FBE_CLASS_ID_EXTENT_POOL);

    fbe_base_config_send_specialize_notification((fbe_base_config_t *) extent_pool_p);

    /* Simply call our static init function to init members that have no
     * dependencies. 
     */
    status = fbe_extent_pool_init(extent_pool_p);

    /* Force verify to make sure the object is properly initialized before scheduling it.
     * This avoids race condition between scheduler thread and usurper command trying to 
     * initialize the object rotary preset at the same time.
     */
    fbe_lifecycle_verify((fbe_lifecycle_const_t *)&fbe_extent_pool_lifecycle_const, (fbe_base_object_t *) extent_pool_p);

    /* Enable lifecycle tracing if requested */
    status = fbe_base_object_enable_lifecycle_debug_trace((fbe_base_object_t *)extent_pool_p);

    return status;
}
/* end fbe_extent_pool_create_object() */

/*!***************************************************************
 * fbe_extent_pool_init()
 ****************************************************************
 * @brief
 *  This function initializes the extent_pool object.
 *
 *  Note that everything that is created here must be
 *  destroyed within fbe_extent_pool_destroy_object().
 *
 * @param extent_pool_p - The extent_pool object.
 *
 * @return
 *  The status of the operation typically FBE_STATUS_OK.
 *
 * @author
 *   06/06/2014:  Created.
 *
 ****************************************************************/
static fbe_status_t 
fbe_extent_pool_init(fbe_extent_pool_t * const extent_pool_p)
{
    /*init base*/
    /* fbe_base_config_init((fbe_base_config_t *)extent_pool_p); */

    /* Initialize our members. */

    /* For now we just hard code the width to 1.
     * We also immediately set the configured bit. 
     */
    fbe_base_config_set_width((fbe_base_config_t *)extent_pool_p, 1);
    fbe_base_config_set_block_transport_const((fbe_base_config_t *) extent_pool_p,
                                              &fbe_extent_pool_block_transport_const);
    fbe_base_config_set_block_edge_ptr((fbe_base_config_t *)extent_pool_p, NULL);
    fbe_base_config_set_outstanding_io_max((fbe_base_config_t *) extent_pool_p, 0);
	fbe_base_config_set_stack_limit((fbe_base_config_t *) extent_pool_p);

    /*set the power saving to be enabled by default. Once the system power saving is enabled,
     we will go into power save in FBE_BASE_CONFIG_DEFUALT_IDLE_TIME_IN_SECONDS minutes*/
    //base_config_enable_object_power_save((fbe_base_config_t *) extent_pool_p);

    return FBE_STATUS_OK;
}
/* end fbe_extent_pool_init() */

fbe_status_t 
fbe_extent_pool_release_hash_table_slices(fbe_extent_pool_t *extent_pool_p)
{
    fbe_u32_t hash_index;
    fbe_extent_pool_hash_table_t *hash_table_p = extent_pool_p->hash_table;

    /* Release all hash table slice.
     */
    for (hash_index = 0; hash_index < extent_pool_p->total_slices; hash_index++) {
        if (hash_table_p[hash_index].slice_ptr) {
            fbe_extent_pool_deallocate_slice(extent_pool_p, hash_table_p[hash_index].slice_ptr);
            hash_table_p[hash_index].slice_ptr = NULL;
        }
    }

    return FBE_STATUS_OK;
}

/*!****************************************************************************
 * fbe_extent_pool_release_memory()
 ******************************************************************************
 * @brief
 *  Release the memory for our maps.
 *
 * @param extent_pool_p          - pointer to object.
 * 
 * @return fbe_lifecycle_status_t
 *
 * @author
 *  6/13/2014 - Created. Rob Foley
 *
 ******************************************************************************/
fbe_status_t 
fbe_extent_pool_release_memory(fbe_extent_pool_t *extent_pool_p)
{
    fbe_block_edge_t *      block_edge_p;

    if (extent_pool_p->hash_table) {
        fbe_extent_pool_release_hash_table_slices(extent_pool_p);
        fbe_memory_release_required(extent_pool_p->hash_table);
    }
    if (extent_pool_p->disk_info_p) {
        fbe_memory_release_required(extent_pool_p->disk_info_p);
    }
    if (extent_pool_p->slice_p) {
        fbe_memory_release_required(extent_pool_p->slice_p);
    }
    if (extent_pool_p->geometry_p) {
        fbe_memory_release_required(extent_pool_p->geometry_p);
    }

    fbe_base_config_get_block_edge_ptr((fbe_base_config_t *)extent_pool_p, &block_edge_p);
    fbe_memory_release_required(block_edge_p);

    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_extent_pool_release_memory()
 **************************************/

/*!***************************************************************
 * fbe_extent_pool_destroy_object()
 ****************************************************************
 * @brief
 *  This function is the interface function, which is called
 *  by the topology to destroy an instance of this class.
 *
 * @param object_handle - The object being destroyed.
 *
 * @return fbe_status_t
 *  STATUS of the destroy operation.
 *
 * @author
 *   06/06/2014:  Created.
 *
 ****************************************************************/
fbe_status_t 
fbe_extent_pool_destroy_object(fbe_object_handle_t object_handle)
{
    fbe_status_t status;
    fbe_extent_pool_t *extent_pool_p = NULL;
    extent_pool_p = (fbe_extent_pool_t *)fbe_base_handle_to_pointer(object_handle);

    fbe_extent_pool_release_memory(extent_pool_p);

    /* Call parent destructor. */
    status = fbe_base_config_destroy_object(object_handle);

    return status;
}
/* end fbe_extent_pool_destroy_object */

fbe_status_t fbe_extent_pool_slice_get_drive_position(void *slice, 
                                                      void *context,
                                                      fbe_u32_t *position_p)
{
    //fbe_extent_pool_t       *extent_pool_p = (fbe_extent_pool_t *)context;
    fbe_extent_pool_slice_t *slice_p = (fbe_extent_pool_slice_t *)slice;
    fbe_slice_address_t      slice_disk_address;
    fbe_block_count_t        blocks_per_slice;

    fbe_extent_pool_class_get_blocks_per_slice(&blocks_per_slice);

    slice_disk_address = slice_p->drive_map[*position_p];
    
    *position_p = fbe_extent_pool_disk_slice_address_get_position(slice_disk_address);
    return FBE_STATUS_OK;
}

fbe_status_t fbe_extent_pool_disk_info_get_slice(fbe_extent_pool_t *extent_pool_p,
                                                 fbe_extent_pool_disk_info_t *disk_info_p,
                                                 fbe_lba_t disk_lba,
                                                 fbe_extent_pool_disk_slice_t **disk_slice_p)
{
    fbe_slice_index_t    slice_index;
    fbe_block_count_t    blocks_per_slice;

    fbe_extent_pool_class_get_blocks_per_slice(&blocks_per_slice);

    if (disk_lba < FBE_EXTENT_POOL_START_OFFSET) {
         fbe_base_object_trace((fbe_base_object_t*)extent_pool_p, 
                              FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "Extent Pool:  disk lba 0x%llx < start offset: 0x%x\n", 
                               disk_lba, FBE_EXTENT_POOL_START_OFFSET);
    }
    slice_index = (disk_lba - FBE_EXTENT_POOL_START_OFFSET) / blocks_per_slice;
    *disk_slice_p = &disk_info_p->drive_map_table_p[slice_index];
    return FBE_STATUS_OK;
}

fbe_status_t fbe_extent_pool_slice_get_position_offset(void *slice, 
                                                       void *context,
                                                       fbe_u32_t position,
                                                       fbe_lba_t *lba_p)
{
    fbe_extent_pool_t            *extent_pool_p = (fbe_extent_pool_t *)context;
    fbe_extent_pool_slice_t      *slice_p = (fbe_extent_pool_slice_t *)slice;
    fbe_extent_pool_disk_info_t  *disk_info_p = NULL;
    fbe_extent_pool_disk_slice_t *disk_slice_p = NULL;
    fbe_lba_t                     input_disk_lba = *lba_p;
    fbe_lba_t                     slice_base_address;
    fbe_lba_t                     slice_offset;
    fbe_block_count_t             blocks_per_slice;
    fbe_slice_address_t           slice_disk_address;
    fbe_u32_t                     pool_position;
    fbe_lba_t                     disk_lba;

    fbe_extent_pool_class_get_blocks_per_slice(&blocks_per_slice);

    slice_disk_address = slice_p->drive_map[position];
    
    pool_position = fbe_extent_pool_disk_slice_address_get_position(slice_disk_address);
    disk_lba = fbe_extent_pool_disk_slice_address_get_lba(slice_disk_address);

    /* Lookup the disk information and disk slice.
     */
    fbe_extent_pool_get_disk_info(extent_pool_p, pool_position, &disk_info_p);
    fbe_extent_pool_disk_info_get_slice(extent_pool_p, disk_info_p, disk_lba, &disk_slice_p);

    /* The address we return is simply the base address plus the offset in the slice.
     *  
     * ************* <- Base address 
     * *           *         /|\    
     * *           *          | Offset to data
     * *           *          | 
     * *           *         \|/ 
     * * ********* * <- Data start
     * * ********* *
     * ************* 
     */
    slice_base_address = fbe_extent_pool_disk_slice_address_get_lba(disk_slice_p->disk_address);

    /* Normalize the disk lba to be within one slice.
     */
    slice_offset = input_disk_lba % blocks_per_slice;

    *lba_p = slice_base_address + slice_offset;
    return FBE_STATUS_OK;
}

fbe_status_t fbe_extent_pool_get_disk_info(fbe_extent_pool_t *extent_pool_p,
                                           fbe_u32_t position,
                                           fbe_extent_pool_disk_info_t **disk_info_p)
{
    *disk_info_p = &extent_pool_p->disk_info_p[position];
    return FBE_STATUS_OK;
}

fbe_status_t fbe_extent_pool_get_user_pool_geometry(fbe_extent_pool_t *extent_pool_p,
                                                    fbe_raid_geometry_t **geometry_p)
{
    
    *geometry_p = &extent_pool_p->geometry_p[0];
    return FBE_STATUS_OK;
}


static __forceinline fbe_status_t 
fbe_extent_pool_lock_get_hash_slice(fbe_metadata_element_t *mde, 
                                    fbe_lba_t start_lba,
                                    fbe_extent_pool_slice_t **slice_p)
{
    fbe_extent_pool_t           *extent_pool_p = (fbe_extent_pool_t *)fbe_base_config_metadata_element_to_base_config_object(mde);
    fbe_u16_t                    data_disks;
    fbe_raid_geometry_t         *geometry_p = NULL;
    fbe_u64_t                    slice_hash;
    fbe_extent_pool_hash_table_t *hash_table_p = NULL;

    fbe_extent_pool_get_user_pool_geometry(extent_pool_p, &geometry_p);
    fbe_raid_geometry_get_data_disks(geometry_p, &data_disks);
    /* We used disk lba for stripe locks. So revert back to logical lba */
    slice_hash = fbe_extent_pool_hash(extent_pool_p, start_lba * data_disks);

    fbe_extent_pool_get_hash_table(extent_pool_p, &hash_table_p);
    *slice_p = hash_table_p[slice_hash].slice_ptr;
    return FBE_STATUS_OK;
}

fbe_metadata_lock_slot_state_t fbe_ext_pool_lock_get_slot_state(fbe_metadata_element_t *mde, fbe_lba_t start_lba)
{
    fbe_extent_pool_slice_t *slice_p;

    fbe_extent_pool_lock_get_hash_slice(mde, start_lba, &slice_p);
    return (slice_p->slice_stripe_lock.slice_lock_state);
}

void fbe_ext_pool_lock_set_slot_state(fbe_metadata_element_t *mde, fbe_lba_t start_lba, fbe_metadata_lock_slot_state_t set_state)
{
    fbe_extent_pool_slice_t *slice_p;

    fbe_extent_pool_lock_get_hash_slice(mde, start_lba, &slice_p);
    slice_p->slice_stripe_lock.slice_lock_state = set_state;
    return;
}

fbe_ext_pool_lock_slice_entry_t * 
fbe_ext_pool_lock_get_lock_table_entry(fbe_metadata_element_t *mde, fbe_lba_t start_lba)
{
    fbe_extent_pool_slice_t *slice_p;

    fbe_extent_pool_lock_get_hash_slice(mde, start_lba, &slice_p);
    if (slice_p) {
        return (&slice_p->slice_stripe_lock);
    } else {
        return NULL;
    }
}

void fbe_ext_pool_lock_get_slice_region(fbe_metadata_element_t *mde, fbe_lba_t start_lba, fbe_lba_t *region_first, fbe_lba_t *region_last)
{
    fbe_block_count_t        blocks_per_slice;

    fbe_extent_pool_class_get_blocks_per_slice(&blocks_per_slice);
    *region_first = start_lba / blocks_per_slice * blocks_per_slice;
    *region_last = *region_first + blocks_per_slice - 1;

    return;
}

void fbe_ext_pool_lock_traverse_hash_table(fbe_metadata_element_t *mde, fbe_extent_pool_hash_callback_function_t callback_function, void *context)
{
    fbe_extent_pool_t           *extent_pool_p = (fbe_extent_pool_t *)fbe_base_config_metadata_element_to_base_config_object(mde);
    fbe_extent_pool_hash_table_t *hash_table_p;
    fbe_u32_t hash_index;

    fbe_extent_pool_get_hash_table(extent_pool_p, &hash_table_p);

    /* Initialize hash table to empty.
     */
    for (hash_index = 0; hash_index < extent_pool_p->total_slices; hash_index++) {
        if (hash_table_p[hash_index].slice_ptr) {
            callback_function(mde, &hash_table_p[hash_index].slice_ptr->slice_stripe_lock, context);
        }
    }

    return;
}

/*******************************
 * end fbe_extent_pool_main.c
 *******************************/


