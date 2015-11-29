/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_raid_group_main.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the main entry points for the raid group object.
 *  This includes the create and destroy methods for the raid group, as
 *  well as the event entry point, load and unload methods.
 * 
 * @ingroup raid_group_class_files
 * 
 * @version
 *   05/20/2009:  Created. RPF
 *
 ***************************************************************************/
  
/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_raid_group.h"
#include "fbe_base_config_private.h"
#include "fbe_raid_group_object.h"
#include "fbe_raid_group_bitmap.h"    
#include "fbe/fbe_notification_interface.h"
#include "fbe_notification.h"
#include "fbe_raid_library.h"
#include "fbe_transport_memory.h"

/* Class methods forward declaration.
 */
fbe_status_t fbe_raid_group_load(void);
fbe_status_t fbe_raid_group_unload(void);
fbe_status_t fbe_raid_group_create_object(fbe_packet_t * packet, fbe_object_handle_t * object_handle);
fbe_status_t fbe_raid_group_destroy( fbe_object_handle_t object_handle);
fbe_status_t fbe_raid_group_destroy_interface( fbe_object_handle_t object_handle);
fbe_status_t fbe_raid_group_io_entry(fbe_object_handle_t object_handle, fbe_packet_t * packet_p);
fbe_status_t fbe_raid_group_monitor_entry(fbe_object_handle_t object_handle, fbe_packet_t * packet_p);
fbe_block_count_t fbe_raid_group_calc_io_throttle(struct fbe_base_object_s *object_p, fbe_payload_block_operation_t *block_op_p);
static fbe_u32_t fbe_raid_group_calc_num_disk_ios(struct fbe_base_object_s *object_p, 
                                                  fbe_payload_block_operation_t *block_op_p,
                                                  fbe_packet_t *packet_p,
                                                  fbe_bool_t *b_do_not_queue_p);

/* We do not export class methods since we never expect to instantiate this class.
 */

/************************* 
 * GLOBALS 
 *************************/
/* This is our global transport constant, which we use to setup a portion of our block 
 * transport server. This is global since it is the same for all raid groups.
 */
fbe_block_transport_const_t fbe_raid_group_block_transport_const = {fbe_raid_group_block_transport_entry, 
                                                                    fbe_base_config_process_block_transport_event,
                                                                    fbe_raid_group_io_entry,
                                                                    fbe_raid_group_calc_io_throttle,
                                                                    fbe_raid_group_calc_num_disk_ios};

/*************************
 *   LOCAL GLOBALS
 *************************/
static fbe_raid_group_debug_flags_t fbe_raid_group_default_raid_group_debug_flags = FBE_RAID_GROUP_DEBUG_FLAG_NONE;
static fbe_time_t fbe_raid_group_paged_metadata_timeout_ms = FBE_RAID_GROUP_METADATA_EXPIRATION_TIME_MS;
static fbe_time_t fbe_raid_group_user_timeout_ms = FBE_RAID_GROUP_USER_EXPIRATION_TIME_MS;
static fbe_time_t fbe_raid_group_vault_wait_ms = FBE_RAID_GROUP_VAULT_ENCRYPT_WAIT_MS;
static fbe_u32_t fbe_raid_group_encrypt_blob_blocks = FBE_RAID_GROUP_ENCRYPT_BLOB_BLOCKS * FBE_BE_BYTES_PER_BLOCK;
static fbe_chunk_count_t fbe_raid_group_max_allowed_metadata_chunk_count = 0;
static fbe_u32_t fbe_raid_group_rebuild_speed = 120; /* 20 is 1 GB /min */
static fbe_u32_t fbe_raid_group_verify_speed = 10;

/*****************************************
 * LOCAL FUNCTION DEFINITIONS
 *****************************************/

/* Accessor methods for above globals.
 */
fbe_time_t fbe_raid_group_get_default_user_timeout_ms(fbe_raid_group_t *raid_group_p)
{
    return fbe_raid_group_user_timeout_ms;
}
fbe_time_t fbe_raid_group_get_paged_metadata_timeout_ms(fbe_raid_group_t *raid_group_p)
{
    return fbe_raid_group_paged_metadata_timeout_ms;
}
fbe_status_t fbe_raid_group_set_default_user_timeout_ms(fbe_time_t time)
{
    fbe_raid_group_user_timeout_ms = time;
    fbe_raid_library_set_max_io_msecs(time);
    return FBE_STATUS_OK;
}
fbe_status_t fbe_raid_group_set_paged_metadata_timeout_ms(fbe_time_t time)
{
    fbe_raid_group_paged_metadata_timeout_ms = time;
    return FBE_STATUS_OK;
}

fbe_status_t fbe_raid_group_get_max_allowed_metadata_chunk_count(fbe_chunk_count_t *max_allowed_metadata_chunk_count)
{
    *max_allowed_metadata_chunk_count = fbe_raid_group_max_allowed_metadata_chunk_count;
    return FBE_STATUS_OK;
}

static fbe_status_t fbe_raid_group_set_max_allowed_metadata_chunk_count(fbe_chunk_count_t allowed_chunk_count)
{
    fbe_raid_group_max_allowed_metadata_chunk_count = allowed_chunk_count;
    return FBE_STATUS_OK;
}

fbe_time_t fbe_raid_group_get_vault_wait_ms(fbe_raid_group_t *raid_group_p)
{
    return fbe_raid_group_vault_wait_ms;
}
fbe_status_t fbe_raid_group_set_vault_wait_ms(fbe_time_t time)
{
    fbe_raid_group_vault_wait_ms = time;
    return FBE_STATUS_OK;
}
fbe_u32_t fbe_raid_group_get_encrypt_blob_blocks(fbe_raid_group_t *raid_group_p)
{
    return fbe_raid_group_encrypt_blob_blocks;
}
fbe_status_t fbe_raid_group_set_encrypt_blob_blocks(fbe_u32_t blocks)
{
    fbe_raid_group_encrypt_blob_blocks = blocks;
    return FBE_STATUS_OK;
}
/*!***************************************************************
 * fbe_raid_group_load()
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
 *  05/20/2009 - Created. RPF
 *
 ****************************************************************/
fbe_status_t 
fbe_raid_group_load(void)
{
    fbe_chunk_count_t max_allowed_chunk_count;

    /* At present we do not have any global data to construct. */
    FBE_ASSERT_AT_COMPILE_TIME(sizeof(fbe_raid_group_t) <= FBE_MEMORY_CHUNK_SIZE);

    /* Paged metadata MUST be aligned */ 
    FBE_ASSERT_AT_COMPILE_TIME((FBE_METADATA_BLOCK_DATA_SIZE % FBE_RAID_GROUP_CHUNK_ENTRY_SIZE) == 0);

    /* Make sure the raid group's non-paged metadata has not exceeded its limit.  
     * The calls to read/write the non-paged MD check the value FBE_PAYLOAD_METADATA_MAX_DATA_SIZE instead of 
     * FBE_METADATA_NONPAGED_MAX_SIZE.
     */
    FBE_ASSERT_AT_COMPILE_TIME(sizeof(fbe_raid_group_nonpaged_metadata_t) <= FBE_METADATA_NONPAGED_MAX_SIZE);
    FBE_ASSERT_AT_COMPILE_TIME(sizeof(fbe_raid_group_nonpaged_metadata_t) <= FBE_PAYLOAD_METADATA_MAX_DATA_SIZE);

    /* calculate the max allowed metadata request chunks at a time */
    /* We want to allow only certain block worth of Metadata updates at a time especially when manipulating bits */
    /* @todo - This 64 should actually come from MDS define and API */
    max_allowed_chunk_count = (64 * FBE_METADATA_BLOCK_DATA_SIZE)/sizeof(fbe_raid_group_paged_metadata_t);

    fbe_raid_group_set_max_allowed_metadata_chunk_count(max_allowed_chunk_count);

    fbe_raid_group_class_load_block_transport_parameters();
    fbe_raid_group_class_load_debug_flags();
    return FBE_STATUS_OK;
}
/* end fbe_raid_group_load() */

/*!***************************************************************
 * fbe_raid_group_unload()
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
 *  05/20/2009 - Created. RPF
 *
 ****************************************************************/
fbe_status_t 
fbe_raid_group_unload(void)
{
    return FBE_STATUS_OK;
}
/* end fbe_raid_group_unload() */

/*****************************************************************************
 *          fbe_raid_group_get_default_raid_group_debug_flags()
 *****************************************************************************
 *
 * @brief   Return the setting of the default raid group debug flags that
 *          will be used for all newly created raid groups.
 *
 * @param   debug_flags_p - Pointer to value of default raid group debug flags 
 *
 * @return  None (always succeeds)
 *
 * @author
 *  03/15/2010  Ron Proulx  - Created
 *
 *****************************************************************************/
void fbe_raid_group_get_default_raid_group_debug_flags(fbe_raid_group_debug_flags_t *debug_flags_p)
{
    *debug_flags_p = fbe_raid_group_default_raid_group_debug_flags;
    return;
}
/* end of fbe_raid_group_get_default_raid_group_debug_flags() */

/*****************************************************************************
 *          fbe_raid_group_set_default_raid_group_debug_flags()
 *****************************************************************************
 *
 * @brief   Set the value to be used for the raid group debug flags for any
 *          newly created raid group.
 *
 * @param   new_default_raid_group_debug_flags - New raid group debug flags
 *
 * @return  status - Always FBE_STATUS_OK
 *
 * @author
 *  03/15/2010  Ron Proulx  - Created
 *
 *****************************************************************************/
fbe_status_t fbe_raid_group_set_default_raid_group_debug_flags(fbe_raid_group_debug_flags_t new_default_raid_group_debug_flags)
{
    fbe_raid_group_default_raid_group_debug_flags = new_default_raid_group_debug_flags;
    return(FBE_STATUS_OK);
}
/* end of fbe_raid_group_set_default_raid_group_debug_flags() */

/*!***************************************************************
 * fbe_raid_group_init()
 ****************************************************************
 * @brief
 *  This function initializes the raid group object.
 *
 *  Note that everything that is created here must be
 *  destroyed within fbe_raid_group_destroy().
 *
 * @param raid_group_p - The raid group object.
 * @param block_edge_p - The ptr to the block edge info
 *                       to use for this raid group.
 * @param generate_state - The state to use to kick off new siots.
 *
 * @return
 *  The status of the operation typically FBE_STATUS_OK.
 *
 * @author
 *  05/20/2009 - Created. RPF
 *
 ****************************************************************/
fbe_status_t 
fbe_raid_group_init(fbe_raid_group_t * const raid_group_p,
                    fbe_block_edge_t * block_edge_p,
                    fbe_raid_common_state_t generate_state)
{
    fbe_status_t                    status;
    fbe_raid_group_debug_flags_t    debug_flags;
    fbe_u32_t  index;
    fbe_object_id_t object_id;
    fbe_raid_emeh_mode_t emeh_mode;

    /*  Allocate memory for the persistent nonpaged metadata */
    /* raid_group_p->nonpaged_metadata_p = fbe_memory_native_allocate(sizeof(fbe_raid_group_nonpersit_nonpaged_metadata_t)); */
/*
    if (raid_group_p->nonpaged_metadata_p == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t*) raid_group_p, FBE_TRACE_LEVEL_ERROR, 
            FBE_TRACE_MESSAGE_ID_CREATE_OBJECT, "%s unable to allocate memory - object id: 0x%x ptr: 0x%p\n",
            __FUNCTION__, raid_group_p->base_config.base_object.object_id, raid_group_p);
        return FBE_STATUS_GENERIC_FAILURE; 
    }
*/
    fbe_base_object_get_object_id((fbe_base_object_t*)raid_group_p, &object_id);
    fbe_raid_group_get_default_object_debug_flags(object_id, &debug_flags);
    fbe_raid_group_init_debug_flags(raid_group_p, debug_flags);
    fbe_raid_group_init_flags(raid_group_p);
    fbe_raid_group_init_local_state(raid_group_p);
    fbe_raid_group_set_last_download_time(raid_group_p, fbe_get_time());

    /*  Initialize the priorities of the services supported by the RAID object */
    fbe_raid_group_set_rebuild_priority(raid_group_p, FBE_TRAFFIC_PRIORITY_HIGH);
    fbe_raid_group_set_verify_priority(raid_group_p, FBE_TRAFFIC_PRIORITY_NORMAL_LOW);

    /* Init the parent class first.
     */
    status = fbe_base_config_init((fbe_base_config_t *)raid_group_p);
    status = fbe_base_config_set_block_edge_ptr((fbe_base_config_t *)raid_group_p, block_edge_p);
    status = fbe_base_config_set_block_transport_const((fbe_base_config_t *)raid_group_p, &fbe_raid_group_block_transport_const);
    fbe_base_config_set_power_saving_idle_time((fbe_base_config_t *)raid_group_p, FBE_RAID_GROUP_DEFAULT_POWER_SAVE_IDLE_TIME);
    
    /* Set the default offset */
    status = fbe_base_config_set_default_offset((fbe_base_config_t *) raid_group_p);
    if(status != FBE_STATUS_OK)
    {
        return status;
    }

    /* Invoke API that initializes the geometry information.
     */
    status = fbe_raid_geometry_init(&raid_group_p->geo,
                                    raid_group_p->base_config.base_object.object_id,
                                    raid_group_p->base_config.base_object.class_id,
                                    &raid_group_p->base_config.metadata_element,
                                    block_edge_p,
                                    generate_state);

    /* Initialize the timestamp used to keep track of how long waiting for a RG to go broken */
    fbe_raid_group_set_check_rg_broken_timestamp(raid_group_p, 0);

    /*clear the list of degraded pvs we use to send notification to*/
    for (index = 0; index < FBE_RAID_GROUP_MAX_REBUILD_POSITIONS; index++)
    {
        raid_group_p->rebuilt_pvds[index] = FBE_OBJECT_ID_INVALID;
        raid_group_p->previous_rebuild_percent[index] = 0;
    }

    /* Set the extended media error handling values.
     */
    fbe_raid_group_emeh_get_class_current_mode(&emeh_mode);
    fbe_raid_group_set_emeh_request(raid_group_p, FBE_RAID_EMEH_COMMAND_INVALID);
    fbe_raid_group_set_emeh_enabled_mode(raid_group_p, emeh_mode);
    if (!fbe_raid_group_is_extended_media_error_handling_enabled(raid_group_p))
    {
        emeh_mode = FBE_RAID_EMEH_MODE_DISABLED;
        fbe_raid_group_set_emeh_enabled_mode(raid_group_p, emeh_mode);
    }
    fbe_raid_group_set_emeh_current_mode(raid_group_p, emeh_mode);
    fbe_raid_group_set_emeh_paco_position(raid_group_p, FBE_EDGE_INDEX_INVALID);
    fbe_raid_group_set_emeh_reliability(raid_group_p, FBE_RAID_EMEH_RELIABILITY_UNKNOWN);

    return status;
}
/* end fbe_raid_group_init() */

/*!***************************************************************
 * fbe_raid_group_destroy_interface()
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
 *  12/04/07 - Created. RPF
 *
 ****************************************************************/
fbe_status_t 
fbe_raid_group_destroy_interface(fbe_object_handle_t object_handle)
{
    fbe_status_t status;

    fbe_base_object_trace((fbe_base_object_t*)fbe_base_handle_to_pointer(object_handle), 
                          FBE_TRACE_LEVEL_DEBUG_HIGH, 
                          FBE_TRACE_MESSAGE_ID_CREATE_OBJECT,
                          "fbe_raid_group_main: %s entry\n", __FUNCTION__);

    /* Destroy this object.
     */
    fbe_raid_group_destroy(object_handle);
    
    /* Call parent destructor.
     */
    status = fbe_base_config_destroy_object(object_handle);

    return status;
}
/* end fbe_raid_group_destroy_interface */

/*!***************************************************************
 * fbe_raid_group_destroy()
 ****************************************************************
 * @brief
 *  This function is called to destroy the raid group object.
 *
 *  This function tears down everything that was created in
 *  fbe_raid_group_init().
 *
 * @param object_handle - The object being destroyed.
 *
 * @return fbe_status_t
 *  STATUS of the destroy operation.
 *
 * @author
 *  05/20/2009 - Created. RPF
 *
 ****************************************************************/
fbe_status_t 
fbe_raid_group_destroy(fbe_object_handle_t object_handle)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_raid_group_t * raid_group_p;
    fbe_raid_group_bg_op_info_t *bg_op_p = NULL;

    fbe_base_object_trace((fbe_base_object_t*)fbe_base_handle_to_pointer(object_handle), 
                          FBE_TRACE_LEVEL_DEBUG_HIGH, 
                          FBE_TRACE_MESSAGE_ID_CREATE_OBJECT, 
                          "fbe_raid_group_main: %s entry\n", __FUNCTION__);
    
    raid_group_p = (fbe_raid_group_t *)fbe_base_handle_to_pointer(object_handle);

    bg_op_p = fbe_raid_group_get_bg_op_ptr(raid_group_p);

    if (bg_op_p != NULL) {
        fbe_memory_native_release(bg_op_p);
    }
    return status;
}
/* end fbe_raid_group_destroy */


/*!**************************************************************
 * fbe_raid_group_get_disk_capacity()
 ****************************************************************
 * @brief Returns the per drive configured capacity of this raid group.
 * @param in_raid_group_p       - Pointer to tThe raid group.
 *
 * @return fbe_lba_t - The per disk capacity.
 *
 ****************************************************************/
fbe_lba_t fbe_raid_group_get_disk_capacity(fbe_raid_group_t* in_raid_group_p)
{
    fbe_lba_t           capacity;
    fbe_u16_t           data_disks;
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_group_get_raid_geometry(in_raid_group_p);

    fbe_raid_geometry_get_configured_capacity(raid_geometry_p, &capacity);
    fbe_raid_geometry_get_data_disks(raid_geometry_p, &data_disks);
    
    return capacity / data_disks;
}


/*!**************************************************************
 * fbe_raid_group_get_exported_disk_capacity()
 ****************************************************************
 * @brief Returns the per drive exported capacity of this raid group.
 *
 * @param in_raid_group_p       - Pointer to tThe raid group.
 *
 * @return fbe_lba_t - The per disk capacity.
 *
 ****************************************************************/
fbe_lba_t fbe_raid_group_get_exported_disk_capacity(fbe_raid_group_t* in_raid_group_p)
{
    fbe_lba_t           capacity;
    fbe_u16_t           data_disks;
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_group_get_raid_geometry(in_raid_group_p);


    fbe_base_config_get_capacity((fbe_base_config_t*) in_raid_group_p, &capacity);
    fbe_raid_geometry_get_data_disks(raid_geometry_p, &data_disks);
    
    return capacity / data_disks;

} // End fbe_raid_group_get_exported_disk_capacity()


/*!**************************************************************
 * fbe_raid_group_get_disk_capacity()
 ****************************************************************
 * @brief Returns the per drive configured capacity of this raid group.
 * @param in_raid_group_p       - Pointer to tThe raid group.
 *
 * @return fbe_lba_t - The per disk capacity.
 *
 ****************************************************************/
fbe_status_t fbe_raid_group_get_configured_capacity(fbe_raid_group_t* in_raid_group_p, fbe_lba_t* capacity_p)
{
    
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_group_get_raid_geometry(in_raid_group_p);

    fbe_raid_geometry_get_configured_capacity(raid_geometry_p, capacity_p);
    
    return FBE_STATUS_OK;
}

/*!**************************************************************
 * fbe_raid_group_get_imported_capacity()
 ****************************************************************
 * @brief Returns the imported capacity of this raid group.
 * @param in_raid_group_p       - Pointer to tThe raid group.
 *
 * @return fbe_lba_t - The imported capacity.
 *
 ****************************************************************/
fbe_status_t fbe_raid_group_get_imported_capacity(fbe_raid_group_t* in_raid_group_p, fbe_lba_t* capacity_p)
{
    
    fbe_lba_t                   configured_capacity;
    fbe_raid_geometry_t*        raid_geometry_p = NULL;
    fbe_raid_group_metadata_positions_t raid_group_metadata_positions;


    raid_geometry_p = fbe_raid_group_get_raid_geometry(in_raid_group_p);

    fbe_raid_geometry_get_configured_capacity(raid_geometry_p, &configured_capacity);

    fbe_raid_group_get_metadata_positions(in_raid_group_p, &raid_group_metadata_positions);
    *capacity_p = raid_group_metadata_positions.imported_capacity;

    //*capacity_p = configured_capacity + journal_capacity;
    
    return FBE_STATUS_OK;
}

/*!****************************************************************************
 * fbe_raid_group_get_total_chunks()
 ******************************************************************************
 * @brief 
 *   Get the total number of chunks in the raid group bitmap.
 *
 * @param in_raid_group_p       - pointer to the raid group object 
 *
 * @return fbe_chunk_count_t    - total number of chunks in the bitmap 
 *
 ******************************************************************************/

fbe_chunk_count_t fbe_raid_group_get_total_chunks(fbe_raid_group_t* in_raid_group_p)
{
 
    fbe_lba_t                           capacity_per_disk;  // capacity per disk/RG ending LBA
    fbe_chunk_count_t                   total_chunks;       // total chunks in the bitmap 
    fbe_chunk_size_t                    chunk_size;         // size of each chunk


    //  Get the capacity per disk
    capacity_per_disk = fbe_raid_group_get_disk_capacity(in_raid_group_p);

    //  Get the chunk size 
    chunk_size = fbe_raid_group_get_chunk_size(in_raid_group_p); 

    //  Calculate the total number of chunks
    total_chunks = (fbe_chunk_count_t) (capacity_per_disk / chunk_size);

    //  Return the total number of chunks 
    return total_chunks;

} // End fbe_raid_group_get_total_chunks()


/*!****************************************************************************
 * fbe_raid_group_get_exported_chunks()
 ******************************************************************************
 * @brief 
 *   Get the number of exported chunks in the raid group bitmap.
 *
 * @param in_raid_group_p       - pointer to the raid group object 
 *
 * @return fbe_chunk_count_t    - number of chunks in the bitmap for the user 
 *                                data area (exported area)
 *
 ******************************************************************************/

fbe_chunk_count_t fbe_raid_group_get_exported_chunks(fbe_raid_group_t* in_raid_group_p)
{
 
    fbe_lba_t                           exported_capacity_per_disk; // data area capacity per disk
    fbe_chunk_count_t                   num_exported_chunks;        // number chunks in bitmap for data area
    fbe_chunk_size_t                    chunk_size;                 // size of each chunk


    //  Get the exported capacity per disk.  This is the size of the RG's user data area on each disk.
    exported_capacity_per_disk = fbe_raid_group_get_exported_disk_capacity(in_raid_group_p);

    //  Get the chunk size 
    chunk_size = fbe_raid_group_get_chunk_size(in_raid_group_p); 

    //  Calculate the number of chunks per disk in the user RG data area (the exported area) and round up as needed
    num_exported_chunks = (fbe_chunk_count_t) (exported_capacity_per_disk / chunk_size); 
    if (exported_capacity_per_disk % chunk_size != 0)
    {
        fbe_base_object_trace((fbe_base_object_t*)in_raid_group_p, FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s: exported_capacity per disk 0x%llx should be multiple of chunk_size 0x%x\n",
                              __FUNCTION__,
                  (unsigned long long)exported_capacity_per_disk,
                  chunk_size);

        num_exported_chunks++;
    }

    //  Return the number of exported chunks 
    return num_exported_chunks;

} // End fbe_raid_group_get_exported_chunks()

/*!****************************************************************************
 *          fbe_raid_group_get_and_clear_next_set_position
 ******************************************************************************
 *
 * @brief   This function find the lowest set position, updates the edge index
 *          value with the position and then clear the bit associated with
 *          that position in the bitmask passed.
 *
 * @param   raid_group_p         - pointer to the raid group object.
 * @param   clear_bitmask_p      - pointer to position bitmask.
 * @param   edge_index_p         - pointer to edge index.
 *
 * @return  status 
 *
 * @author
 *  09/02/2011 - Created. Ashwin Tamilarasan
 *  10/23/2013  Ron Proulx  - Fixed to only clear and not shift.
 ******************************************************************************/
fbe_status_t fbe_raid_group_get_and_clear_next_set_position(fbe_raid_group_t *raid_group_p,
                                                            fbe_raid_position_bitmask_t *clear_bitmask_p,
                                                            fbe_u32_t *edge_index_p)
{
    fbe_u32_t   shift_left_count = 0;

    /* Find and clear next set bit in bitmask.
     */
    while (*clear_bitmask_p > 0)
    {
        /* If a set bit is found return the index an clear the bit in the
         * mask.
         */
        if ((*clear_bitmask_p & (1 << shift_left_count)) != 0)
        {            
            *edge_index_p = shift_left_count;
            fbe_raid_group_trace(raid_group_p,
                                 FBE_TRACE_LEVEL_INFO,
                                 FBE_RAID_GROUP_DEBUG_FLAGS_TRACE_NEEDS_REBUILD,                        
                                 "%s: clear pos: %d clear bitmask: 0x%x\n", 
                                 __FUNCTION__,
                                 *edge_index_p, *clear_bitmask_p);
            *clear_bitmask_p &= ~(1 << shift_left_count);
            return FBE_STATUS_OK;
        }
        shift_left_count++;
    }
    
    return FBE_STATUS_OK;
}
/******************************************************
 * end fbe_raid_group_get_and_clear_next_set_position()
 ******************************************************/

/*!****************************************************************************
 * fbe_raid_group_add_to_terminator_queue()
 ******************************************************************************
 * @brief 
 *   Adds the packet to the raid groups terminator queue. Also sets the cancel
 *   routine.
 *
 * @param raid_group_p       - pointer to the raid group object 
 * @param packet             - packet to add to the queue.
 *
 * @return void       
 *
 ******************************************************************************/

void fbe_raid_group_add_to_terminator_queue(fbe_raid_group_t *raid_group_p,
                                            fbe_packet_t *packet_p)
{
    fbe_raid_iots_status_t iots_status;
    fbe_payload_ex_t *sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    fbe_raid_iots_t *iots_p = NULL;

    fbe_payload_ex_get_iots(sep_payload_p, &iots_p);

    /* Set the cancel routine prior to adding to the terminator queue since 
     * adding to the terminator queue might cause the cancel fn to get invoked. 
     */
    fbe_transport_set_cancel_function(packet_p, 
                                      fbe_base_object_packet_cancel_function, 
                                      (fbe_base_object_t *)raid_group_p);
    fbe_raid_iots_get_status(iots_p, &iots_status);

    if (iots_status >= FBE_RAID_IOTS_STATUS_LAST)
    {
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s add to terminator queue iots status invalid status: 0x%x iots: 0x%p\n",
                              __FUNCTION__, iots_status, iots_p);
    }
    // Add the packet to the terminator q.
    fbe_base_object_add_to_terminator_queue((fbe_base_object_t*) raid_group_p, packet_p);

}
/**************************************
 * end fbe_raid_group_add_to_terminator_queue
 **************************************/

fbe_u32_t fbe_raid_group_get_rebuild_speed(void)
{
    return fbe_raid_group_rebuild_speed;
}

void fbe_raid_group_set_rebuild_speed(fbe_u32_t rebuild_speed)
{
    fbe_raid_group_rebuild_speed = rebuild_speed;
}


fbe_u32_t fbe_raid_group_get_verify_speed(void)
{
    return fbe_raid_group_verify_speed;
}

void fbe_raid_group_set_verify_speed(fbe_u32_t verify_speed)
{
    fbe_raid_group_verify_speed = verify_speed;
}
/*!**************************************************************
 * fbe_raid_group_calc_io_throttle()
 ****************************************************************
 * @brief
 *  Determine what the cost is of this I/O.
 *
 * @param object_p
 * @param block_op_p
 * 
 * @return fbe_block_count_t - Cost of the I/O.
 *
 * @author
 *  3/19/2013 - Created. Rob Foley
 *
 ****************************************************************/

fbe_block_count_t fbe_raid_group_calc_io_throttle(struct fbe_base_object_s *object_p, fbe_payload_block_operation_t *block_op_p)
{
    fbe_block_count_t io_cost;
    fbe_medic_action_priority_t ds_medic_priority;
    fbe_raid_group_t *raid_group_p = (fbe_raid_group_t *)object_p;
    fbe_bool_t b_is_zeroing;
    fbe_bool_t b_is_degraded;
    fbe_block_count_t block_count;
    fbe_payload_block_operation_opcode_t opcode;
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_group_get_raid_geometry(raid_group_p);
    fbe_u32_t width;
    fbe_u16_t data_disks;
    fbe_u32_t multiplier = 1;

    fbe_payload_block_get_block_count(block_op_p, &block_count);
    fbe_payload_block_get_opcode(block_op_p, &opcode);
    fbe_raid_geometry_get_width(raid_geometry_p, &width);
    fbe_raid_geometry_get_data_disks(raid_geometry_p, &data_disks);

    /* If zeroing and read, cost is higher since we have a bitmap read and possibly a write also.
     */
    ds_medic_priority = fbe_base_config_get_downstream_edge_priority((fbe_base_config_t*)raid_group_p);
    b_is_degraded = fbe_raid_group_io_is_degraded(raid_group_p);
    b_is_zeroing = (ds_medic_priority == FBE_MEDIC_ACTION_ZERO);
    
    switch (opcode)
    {
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ:
            if (b_is_zeroing)
            {
                multiplier += 1;
            }
            io_cost = block_count * multiplier;
            break;

        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE:
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_NONCACHED:
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_CORRUPT_DATA:
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ZERO:
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ENCRYPTION_REKEY_WRITE:
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ENCRYPTION_REKEY_WRITE_ZEROS:
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_ZEROS:
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ENCRYPTION_REKEY_ZERO:

            /* Check If we are stripe aligned? 
             */
            multiplier += 1;
            if (b_is_zeroing)
            {
                multiplier += 2;
            }
            if (b_is_degraded)
            {
                multiplier += 3;
            }
            io_cost = block_count * multiplier;
            /* Since it is a write, add in a rough cost of the parity update.
             */
            io_cost += (block_count / data_disks) * multiplier;
            break;
            
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_VERIFY_WRITE:
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_VERIFY:
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ_ONLY_VERIFY:
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ERROR_VERIFY:
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INCOMPLETE_WRITE_VERIFY:
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_SYSTEM_VERIFY:
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_VERIFY_SPECIFIC_AREA:
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ_ONLY_VERIFY_SPECIFIC_AREA:
            io_cost = width * block_count;
            break;
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INITIATE_RO_VERIFY:
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INITIATE_ERROR_VERIFY:
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INITIATE_INCOMPLETE_WRITE_VERIFY:
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INITIATE_SYSTEM_VERIFY:
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INITIATE_USER_VERIFY:
            /* We can't allow these to prevent background ops from coming in. 
             * This gets the NP lock and BG ops have the np lock when then come into 
             * the block transport server.  Thus if we allow these initiate requests 
             * to hold up BG ops we will have a deadlock. 
             */
            io_cost = 0;
            break;
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_REBUILD:
            io_cost = width * block_count;
            break;
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ENCRYPTION_READ_PAGED:
            io_cost = 2 * block_count; /* 1 read and 1 write */
            break;
        default:
            io_cost = block_count;
            break;
    }
    return io_cost;
}
/******************************************
 * end fbe_raid_group_calc_io_throttle()
 ******************************************/
/*!**************************************************************
 * fbe_raid_group_calc_num_disk_ios()
 ****************************************************************
 * @brief
 *  Determine what the cost is of this I/O.
 *
 * @param object_p
 * @param block_op_p
 * @param packet_p
 * @param b_do_not_queue
 * 
 * @return fbe_u32_t
 * 
 * @return fbe_block_count_t - Cost of the I/O.
 *
 * @author
 *  4/12/2013 - Created. Rob Foley
 *
 ****************************************************************/
static fbe_u32_t fbe_raid_group_calc_num_disk_ios(struct fbe_base_object_s *object_p, 
                                                  fbe_payload_block_operation_t *block_op_p,
                                                  fbe_packet_t *packet_p,
                                                  fbe_bool_t *b_do_not_queue_p)
{
    fbe_raid_group_t *raid_group_p = (fbe_raid_group_t *)object_p;
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_group_get_raid_geometry(raid_group_p);
    fbe_medic_action_priority_t ds_medic_priority;
    fbe_bool_t b_is_zeroing;
    fbe_bool_t b_is_degraded;
    fbe_packet_priority_t priority;
    fbe_u32_t max_credits;

    if (b_do_not_queue_p != NULL)
    {
        if (fbe_transport_is_monitor_packet(packet_p, raid_geometry_p->object_id)){
            *b_do_not_queue_p = FBE_TRUE;
        }
        else {
            *b_do_not_queue_p = FBE_FALSE;
        }
    }
    ds_medic_priority = fbe_base_config_get_downstream_edge_priority((fbe_base_config_t*)raid_group_p);
    b_is_degraded = fbe_raid_group_io_is_degraded(raid_group_p);
    b_is_zeroing = (ds_medic_priority == FBE_MEDIC_ACTION_ZERO);
    max_credits = raid_group_p->base_config.block_transport_server.io_credits_max;

    fbe_transport_get_packet_priority(packet_p, &priority);
    if (fbe_raid_geometry_is_parity_type(raid_geometry_p))
    {
        return fbe_raid_geometry_calc_parity_disk_ios(raid_geometry_p, block_op_p, b_is_zeroing, b_is_degraded, priority, max_credits);
    }
    else if (fbe_raid_geometry_is_mirror_type(raid_geometry_p))
    {
        return fbe_raid_geometry_calc_mirror_disk_ios(raid_geometry_p, block_op_p, b_is_zeroing, b_is_degraded, priority, max_credits);
    }
    else if (fbe_raid_geometry_is_striper_type(raid_geometry_p))
    {
        return fbe_raid_geometry_calc_striper_disk_ios(raid_geometry_p, block_op_p, b_is_zeroing);
    }
    else 
    {
        return 1;
    }
}
/******************************************
 * end fbe_raid_group_calc_num_disk_ios()
 ******************************************/

/*!**************************************************************
 * fbe_raid_group_is_r10_degraded()
 ****************************************************************
 * @brief
 *  This function checks to see the striper is degraded.
 *
 * @param in_raid_group_p           - pointer to a striper object
 *
 * @return TRUE or FALSE         - return true or false.
 *
 * @author
 *
 ****************************************************************/
fbe_bool_t fbe_raid_group_is_r10_degraded(fbe_raid_group_t* in_raid_group_p)
{
    fbe_u32_t           width, index;
    fbe_bool_t          is_degraded = FBE_FALSE;
    fbe_block_edge_t *  edge_p = NULL;
    fbe_path_state_t    path_state;
    fbe_path_attr_t     attr;

    /* Get the mirror pair */
    fbe_base_config_get_width((fbe_base_config_t *)in_raid_group_p, &width);

    /* Check for the path state is enabled.  If it is enabled, get the attributes.
     * If the attribute is degraded, set the striper_degraded to TRUE.
     */
    for (index = 0; index < width; index ++)
    {
        /* Get the block edge */
        fbe_base_config_get_block_edge( (fbe_base_config_t *) in_raid_group_p, &edge_p, index);

        /* Get edge path state */
        fbe_block_transport_get_path_state(edge_p, &path_state);

        /* If the path state is enabled, set the attribute and check to see if it is degraded. If it is, set to TRUE. */
        if (path_state == FBE_PATH_STATE_ENABLED)
        {
            fbe_block_transport_get_path_attributes(edge_p, &attr);

            if (attr & FBE_BLOCK_PATH_ATTR_FLAGS_DEGRADED) 
            {
                is_degraded = FBE_TRUE;

                break;
            }
        }
    }

    return is_degraded;
}

/*******************************
 * end fbe_raid_group_main.c
 *******************************/
