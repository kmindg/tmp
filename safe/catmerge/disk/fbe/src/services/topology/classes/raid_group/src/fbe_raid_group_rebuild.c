/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_raid_group_rebuild.c
 ***************************************************************************
 *
 * @brief
 *   This file contains the rebuild processing code for the raid group object.
 * 
 * @ingroup raid_group_class_files
 * 
 * @version
 *   12/07/2009:  Created. Jean Montes.
 *
 ***************************************************************************/


/*************************
 *   INCLUDE FILES
 *************************/

#include "fbe_raid_group_rebuild.h"                     //  this file's .h file  
#include "fbe_raid_group_bitmap.h"                      //  for fbe_raid_group_bitmap_find_next_nr_chunk
#include "fbe_raid_group_object.h"                      //  utility macros
#include "fbe_raid_group_needs_rebuild.h"               //  for fbe_raid_group_needs_rebuild_clear_nr_for_iots  
#include "fbe_raid_group_logging.h"                     //  for logging rebuild complete/lun start/lun complete 
#include "fbe_raid_group_executor.h"                    // for metadata update
#include "fbe_transport_memory.h"                       //  for fbe_transport_allocate_buffer
#include "fbe_notification.h"                           // for fbe_notification_send

/*****************************************
 * LOCAL FUNCTION PROTOTYPES
 *****************************************/


//  Completion function for event which checks if this LBA is within a LUN
static fbe_status_t fbe_raid_group_rebuild_send_event_to_check_lba_completion(
                                    void*                           in_event_p,
                                    fbe_event_completion_context_t  in_context);

//  Send a rebuild I/O request to RAID 
static fbe_status_t fbe_raid_group_rebuild_send_io_request(
                                    fbe_raid_group_t*               in_raid_group_p, 
                                    fbe_packet_t*                   in_packet_p,
                                    fbe_bool_t                      b_queue_to_block_transport,
                                    fbe_lba_t                       in_start_lba,
                                    fbe_block_count_t               in_block_count);

//  Completion function for a rebuild I/O request 
static fbe_status_t fbe_raid_group_rebuild_send_io_request_completion(
                                    fbe_packet_t*                   in_packet_p, 
                                    fbe_packet_completion_context_t in_context);

//  Completion function for update to paged metadata for an unbound area
static fbe_status_t fbe_raid_group_clear_nr_control_operation_completion(
                                    fbe_packet_t*                   in_packet_p,
                                    fbe_packet_completion_context_t in_context);

//  Update the nonpaged metadata for an unbound area 
static fbe_status_t fbe_raid_group_rebuild_unbound_area_update_nonpaged_metadata(
                                    fbe_raid_group_t*               in_raid_group_p, 
                                    fbe_packet_t*                   in_packet_p);

static fbe_status_t fbe_raid_group_rebuild_update_checkpoint_after_io(
                                    fbe_raid_group_t*               in_raid_group_p,
                                    fbe_packet_t*                   in_packet_p);

// This function checks if we need to flip the NR bits for the proactive spare
//  and the proactive candidate
static fbe_bool_t fbe_raid_group_rebuild_flip_bitmap_for_proactive_spare(
                                        fbe_raid_group_t*              in_raid_group_p,
                                        fbe_raid_position_bitmask_t    in_nr_position_mask);

//  Get the chunk info for the next range of the rebuild LBA range.
static fbe_status_t fbe_raid_group_rebuild_get_chunk_info_completion(
                                    fbe_packet_t*                   in_packet_p,
                                    fbe_packet_completion_context_t in_context);                                    

static fbe_status_t fbe_raid_group_rebuild_perform_checkpoint_update_and_log(
                                    fbe_raid_group_t*               in_raid_group_p,
                                    fbe_packet_t*                   in_packet_p, 
                                    fbe_lba_t                       in_start_lba,
                                    fbe_block_count_t               in_block_count,
                                    fbe_raid_position_bitmask_t     in_positions_to_advance);

static fbe_status_t fbe_raid_group_rebuild_get_rebuilding_disk_position(
                                    fbe_raid_group_t*           in_raid_group_p,
                                    fbe_lba_t                   rebuild_io_lba,
                                    fbe_u32_t*                  out_disk_position_p);


static fbe_status_t fbe_raid_group_get_raid10_rebuild_info_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);


/***************
 * FUNCTIONS
 ***************/
 
static fbe_u32_t fbe_raid_group_rebuild_background_op_chunks = FBE_RAID_GROUP_REBUILD_BACKGROUND_OP_CHUNKS;

/*!****************************************************************************
 *  fbe_raid_group_get_rebuild_checkpoint()
 ******************************************************************************
 * @brief 
 *   Get the rebuild checkpoint for the given disk position.
 *
 * @param in_raid_group_p       - pointer to the raid group object 
 * @param in_position           - disk position in the RG 
 * @param out_checkpoint_p      - pointer to data which gets set to the rebuild
 *                                checkpoint.  This value is only valid when 
 *                                the function returns success.
 *
 * @return fbe_status_t        
 *
 * @author
 *   12/07/2009 - Created. Jean Montes.
 *
 ******************************************************************************/

fbe_status_t fbe_raid_group_get_rebuild_checkpoint(
                                            fbe_raid_group_t*           in_raid_group_p,
                                            fbe_u32_t                   in_position,
                                            fbe_lba_t*                  out_checkpoint_p)
{
    fbe_raid_group_nonpaged_metadata_t*     nonpaged_metadata_p;        // pointer to all of nonpaged metadata 
    fbe_u32_t                               rebuild_data_index;         // index of this checkpoint in rebuild 
                                                                        //   info struct 


    //  Find if this position has its checkpoint set in the non-paged MD's rebuild checkpoint info
    fbe_raid_group_rebuild_find_existing_checkpoint_entry_for_position(in_raid_group_p, in_position, 
        &rebuild_data_index);
    
    //  If it is not set in the structure, then it means the checkpoint value is the logical end marker.  Set the
    //  output parameter to that and return. 
    if (rebuild_data_index == FBE_RAID_GROUP_INVALID_INDEX)
    {
        *out_checkpoint_p = FBE_LBA_INVALID;
        return FBE_STATUS_OK; 
    }

    //  The position has a value set.  Get a pointer to the non-paged metadata.
    fbe_base_config_get_nonpaged_metadata_ptr((fbe_base_config_t*) in_raid_group_p, (void **)&nonpaged_metadata_p);

    //  Set the output parameter to the checkpoint in the non-paged MD at the found index 
    *out_checkpoint_p = nonpaged_metadata_p->rebuild_info.rebuild_checkpoint_info[rebuild_data_index].checkpoint;

    //  Return success 
    return FBE_STATUS_OK;

} // End fbe_raid_group_get_rebuild_checkpoint()

/*!****************************************************************************
 *          fbe_raid_group_is_another_position_rebuilding()
 ******************************************************************************
 *
 * @brief   Determines if there is a another position in the raid group that is
 *          either needs to be rebuilt (but is marked rebuild logging) ot is in
 *          the process of rebuilding.
 *
 * @param   raid_group_p - pointer to the raid group object 
 * @param   position - The position that is known to be rebuilding.  Do not
 *              check this position.
 *
 * @return  bool - FBE_TRUE - There is another raid group position that is
 *                  rebuilding.    
 *
 * @author
 *  05/03/2013  Ron Proulx  - Created.
 *
 ******************************************************************************/
static fbe_bool_t fbe_raid_group_is_another_position_rebuilding(fbe_raid_group_t *raid_group_p,
                                                                fbe_u32_t position)
{
    fbe_raid_group_nonpaged_metadata_t *nonpaged_metadata_p;
    fbe_u32_t                           rebuild_index;

    /* Check if any other positions are marked for rebuild.
     */
    fbe_base_config_get_nonpaged_metadata_ptr((fbe_base_config_t *)raid_group_p, (void **)&nonpaged_metadata_p);
    for (rebuild_index = 0; rebuild_index < FBE_RAID_GROUP_MAX_REBUILD_POSITIONS; rebuild_index++)
    {
        /* If the position is valid and not the position passed then return True.
         */
        if ((nonpaged_metadata_p->rebuild_info.rebuild_checkpoint_info[rebuild_index].position != FBE_RAID_INVALID_DISK_POSITION) &&
            (nonpaged_metadata_p->rebuild_info.rebuild_checkpoint_info[rebuild_index].position != position)                          )
        {
            /* Return True.
             */
            return FBE_TRUE;
        }
    }

    /* Return False
     */
    return FBE_FALSE;
}
/******************************************************
 * end fbe_raid_group_is_another_position_rebuilding()
 ******************************************************/

/*!****************************************************************************
 *  fbe_raid_group_get_min_rebuild_checkpoint()
 ******************************************************************************
 * @brief 
 *   Get the minimum rebuild checkpoint for the given raid group
 *
 * @param raid_group_p       - pointer to the raid group object 
 * @param out_checkpoint_p      - pointer to data which gets set to the rebuild
 *                                checkpoint.  This value is only valid when 
 *                                the function returns success.
 *
 * @return fbe_status_t        
 *
 * @author
 *  05/03/2013  Ron Proulx  - Created.
 *
 ******************************************************************************/
fbe_status_t fbe_raid_group_get_min_rebuild_checkpoint(fbe_raid_group_t *raid_group_p,
                                                       fbe_lba_t *out_checkpoint_p)
{
    fbe_raid_group_nonpaged_metadata_t *nonpaged_metadata_p;
    fbe_u32_t                           rebuild_index;
    fbe_lba_t                           exported_disk_capacity = 0;   
    fbe_lba_t                           min_rebuild_checkpoint = FBE_LBA_INVALID;

    /* Get the minimum rebuild checkpoint for the RG. 
     */
    fbe_base_config_get_nonpaged_metadata_ptr((fbe_base_config_t *)raid_group_p, (void **)&nonpaged_metadata_p);
    exported_disk_capacity = fbe_raid_group_get_exported_disk_capacity(raid_group_p);
    for (rebuild_index = 0; rebuild_index < FBE_RAID_GROUP_MAX_REBUILD_POSITIONS; rebuild_index++)
    {
        /* If the checkpoint is less than the current use that.
         */
        if (nonpaged_metadata_p->rebuild_info.rebuild_checkpoint_info[rebuild_index].checkpoint < min_rebuild_checkpoint)
        {
            min_rebuild_checkpoint = nonpaged_metadata_p->rebuild_info.rebuild_checkpoint_info[rebuild_index].checkpoint;
        }
    }

    /* If the minimum rebuilt checkpoint is greater than the exported capacity
     * or we need to run a metadata rebuild, return 0. 
     */
    if ( fbe_raid_group_background_op_is_metadata_rebuild_need_to_run(raid_group_p) ||
        ((min_rebuild_checkpoint != FBE_LBA_INVALID)       &&
         (min_rebuild_checkpoint > exported_disk_capacity)    )                        )
    {
        min_rebuild_checkpoint = 0;
    }

    /* Update the return value and return success.
     */
    *out_checkpoint_p = min_rebuild_checkpoint;
    return FBE_STATUS_OK;
}
/************************************************* 
 * end fbe_raid_group_get_min_rebuild_checkpoint()
 *************************************************/

/*!****************************************************************************
 *  fbe_raid_group_get_max_rebuild_checkpoint()
 ******************************************************************************
 * @brief 
 *   Get the maximum rebuild checkpoint for the given raid group
 *
 * @param raid_group_p       - pointer to the raid group object 
 * @param out_checkpoint_p      - pointer to data which gets set to the rebuild
 *                                checkpoint.  This value is only valid when 
 *                                the function returns success.
 *
 * @return fbe_status_t        
 *
 * @author
 *  05/03/2013  Ron Proulx  - Created.
 *
 ******************************************************************************/
fbe_status_t fbe_raid_group_get_max_rebuild_checkpoint(fbe_raid_group_t *raid_group_p,
                                                       fbe_lba_t *out_checkpoint_p)
{
    fbe_raid_group_nonpaged_metadata_t *nonpaged_metadata_p;
    fbe_u32_t                           rebuild_index;
    fbe_lba_t                           exported_disk_capacity = 0;   
    fbe_lba_t                           max_rebuild_checkpoint = 0;
    fbe_u32_t                           max_rebuild_index = 0;
    fbe_u32_t                           max_rebuild_position = FBE_RAID_INVALID_DISK_POSITION;
    fbe_bool_t                          b_found_rebuilding_disk = FBE_FALSE;

    /* Get the maximum rebuild checkpoint for the RG. 
     */
    fbe_base_config_get_nonpaged_metadata_ptr((fbe_base_config_t *)raid_group_p, (void **)&nonpaged_metadata_p);
    exported_disk_capacity = fbe_raid_group_get_exported_disk_capacity(raid_group_p);
    for (rebuild_index = 0; rebuild_index < FBE_RAID_GROUP_MAX_REBUILD_POSITIONS; rebuild_index++)
    {
        /* If the checkpoint is greater than the current use that.
         */
        if ((nonpaged_metadata_p->rebuild_info.rebuild_checkpoint_info[rebuild_index].checkpoint != FBE_LBA_INVALID)        &&
            (nonpaged_metadata_p->rebuild_info.rebuild_checkpoint_info[rebuild_index].checkpoint >= max_rebuild_checkpoint)    )
        {
            b_found_rebuilding_disk = FBE_TRUE;
            max_rebuild_position = nonpaged_metadata_p->rebuild_info.rebuild_checkpoint_info[rebuild_index].position;
            max_rebuild_index = rebuild_index;
            max_rebuild_checkpoint = nonpaged_metadata_p->rebuild_info.rebuild_checkpoint_info[rebuild_index].checkpoint;
        }
    }

    /* If no rebuilding positions found then the max is the end marker
     */
    if (b_found_rebuilding_disk == FBE_FALSE)
    {
        max_rebuild_checkpoint = FBE_LBA_INVALID;
    }

    /* If the maximum rebuilt checkpoint is greater than the exported capacity
     * or we need to run a metadata rebuild, return 0. 
     */
    if ( fbe_raid_group_background_op_is_metadata_rebuild_need_to_run(raid_group_p) ||
        ((max_rebuild_checkpoint != FBE_LBA_INVALID)       &&
         (max_rebuild_checkpoint > exported_disk_capacity)    )                        )
    {
        max_rebuild_checkpoint = 0;
    }

    /* Check for the special case where there are/were multiple positions rebuilding.
     * If this is the case we want to return the rebuild checkpoint (blocks_rebuilt)
     * of the first position that was rebuilt.
     */
    for (rebuild_index = 0; rebuild_index < FBE_RAID_GROUP_MAX_REBUILD_POSITIONS; rebuild_index++)
    {
        /* If the pvd object id is invalid but the blocks rebuilt is not 0
         * it indicates that there were multiple positions rebuilding BUT
         * the first position now completed the rebuild.
         */
        if ((b_found_rebuilding_disk == FBE_TRUE)                                                             &&
            (max_rebuild_index != rebuild_index)                                                              &&
            (raid_group_p->rebuilt_pvds[rebuild_index] == FBE_OBJECT_ID_INVALID)                              &&
            (raid_group_p->raid_group_metadata_memory.blocks_rebuilt[rebuild_index] > max_rebuild_checkpoint)    )
        {
            /* Trace some information if enabled and return the blocks rebuilt
             * for the first position rebuilt.
             */
            fbe_raid_group_trace(raid_group_p,
                                 FBE_TRACE_LEVEL_INFO,
                                 FBE_RAID_GROUP_DEBUG_FLAG_REBUILD_NOTIFICATION_TRACING,
                                 "raid_group: get max rebuild current pos: %d chkpt: 0x%llx first chkpt: 0x%llx \n",
                                 max_rebuild_position, 
                                 (unsigned long long)max_rebuild_checkpoint,
                                 (unsigned long long)raid_group_p->raid_group_metadata_memory.blocks_rebuilt[rebuild_index]);
            max_rebuild_checkpoint = raid_group_p->raid_group_metadata_memory.blocks_rebuilt[rebuild_index];
        }
    }

    /* Update the return value and return success.
     */
    *out_checkpoint_p = max_rebuild_checkpoint;
    return FBE_STATUS_OK;
}
/************************************************* 
 * end fbe_raid_group_get_max_rebuild_checkpoint()
 *************************************************/

/*!**************************************************************
 * fbe_raid_group_is_singly_degraded()
 ****************************************************************
 * @brief
 *  returns if we have only one degraded position.
 *
 * @param raid_group_p - raid group obj.
 *
 * @return fbe_bool_t TRUE if we have just one degraded position.
 *
 * @author
 *  10/10/2011 - Created. Rob Foley
 *
 ****************************************************************/

fbe_bool_t fbe_raid_group_is_singly_degraded(fbe_raid_group_t *raid_group_p)
{
    fbe_raid_group_nonpaged_metadata_t *nonpaged_metadata_p = NULL;
    fbe_base_config_get_nonpaged_metadata_ptr((fbe_base_config_t*) raid_group_p, (void **)&nonpaged_metadata_p);

    if ( ((nonpaged_metadata_p->rebuild_info.rebuild_checkpoint_info[0].position != FBE_RAID_INVALID_DISK_POSITION) &&
          (nonpaged_metadata_p->rebuild_info.rebuild_checkpoint_info[1].position == FBE_RAID_INVALID_DISK_POSITION)) ||
         ((nonpaged_metadata_p->rebuild_info.rebuild_checkpoint_info[0].position == FBE_RAID_INVALID_DISK_POSITION) &&
          (nonpaged_metadata_p->rebuild_info.rebuild_checkpoint_info[1].position != FBE_RAID_INVALID_DISK_POSITION)))
    {
        return FBE_TRUE;
    }
    return FBE_FALSE;
}
/******************************************
 * end fbe_raid_group_is_singly_degraded()
 ******************************************/
/*!****************************************************************************
 * fbe_raid_group_rebuild_find_disk_needing_action()
 ******************************************************************************
 * @brief
 *   This function will search through the disks in the RAID group to find if
 *   any rebuild actions are needed.  First it will look for any disk which 
 *   meets the following conditions:
 *   
 *   - The disk's rebuild checkpoint is not set to the logical end marker
 *   - The disk is not rebuild logging
 *   - The disk's edge is enabled
 *  
 *   Such a disk needs a rebuild action, as follows:
 * 
 *   - If the disk has its checkpoint set to the capacity of the disk, then it 
 *     has completed a rebuild and the checkpoint needs to be moved to the 
 *     logical end marker.  (This case would occur when the array went down in
 *     between performing the final I/O of the rebuild and moving the checkpoint.)
 *     This disk needs a "rebuild completion" and the out_complete_rebuild_pos_p
 *     parameter will be set with its value.
 *
 *   - Otherwise, the disk needs to actually be rebuilt and the out_do_rebuild_pos_p
 *     parameter will be set with its value.
 *
 *   If the function does not find a disk/edge, it sets the output parameters to 
 *   FBE_RAID_INVALID_DISK_POSITION.  
 *   
 * @param in_raid_group_p               - Pointer to a raid group object
 * @param out_degraded_positions_p      - Pointer to degraded positions bitmask
 * @param out_positions_to_be_rebuilt_p - Pointer to positions to be rebuilt in this request
 *                                        (Must have the same rebuild checkpoint).
 * @param out_complete_rebuild_positions_p - Gets populated with position of the first disk 
 *                                        in the RG that needs to "complete a rebuild"; set
 *                                        to FBE_RAID_INVALID_DISK_POSITION if none found
 * @param out_rebuild_checkpoint_p      - Pointer to rebuild checkpoint that we have 
 *                                        determined is the highest checkpoint.
 *
 * @return  fbe_status_t        
 *
 * @author
 *   12/07/2009 - Created. Jean Montes.
 *   07/19/2012 - Vamsi V. Changes to handle different rebuild areas (paged, user, journal).
 *
 ******************************************************************************/
fbe_status_t fbe_raid_group_rebuild_find_disk_needing_action(fbe_raid_group_t *in_raid_group_p,
                                                             fbe_raid_position_bitmask_t *out_degraded_positions_p,
                                                             fbe_raid_position_bitmask_t *out_positions_to_be_rebuilt_p,
                                                             fbe_raid_position_bitmask_t *out_complete_rebuild_positions_p,
                                                             fbe_lba_t *out_rebuild_checkpoint_p,
                                                             fbe_bool_t *out_checkpoint_reset_needed_p)
{

    fbe_u32_t                   width;                          //  raid group width
    fbe_u32_t                   position_index;                 //  current disk position 
    fbe_block_edge_t*           edge_p;                         //  pointer to an edge 
    fbe_path_state_t            path_state;                     //  path state of the edge 
    fbe_bool_t                  requires_rebuild_b;             //  true if this disk requires a rebuild 
    fbe_lba_t                   highest_checkpoint;             //  highest rebuild checkpoint found so far
    fbe_lba_t                   cur_checkpoint;                 //  current disk's rebuild checkpoint 
    fbe_bool_t                  b_rebuild_logging;
    fbe_lba_t                   exported_disk_capacity;         //  exported capacity per disk/RG ending LBA
    fbe_lba_t                   metadata_rebuild_lba;           //  rebuild lba for the metadata 
    fbe_lba_t                   rebuild_complete_checkpoint;    //  if rebuild complete
    fbe_lba_t                   journal_log_start_lba;
    fbe_lba_t                   journal_log_end_lba;
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_group_get_raid_geometry(in_raid_group_p);
    //fbe_u32_t                   highest_region = 0;
    fbe_bool_t                  b_need_reset_checkpoint;
    fbe_bool_t                  b_curr_need_reset_checkpoint;

    //  Initialize output parameters 
    *out_degraded_positions_p = 0;
    *out_positions_to_be_rebuilt_p = 0;
    *out_complete_rebuild_positions_p = 0;
    rebuild_complete_checkpoint = 0;
    b_need_reset_checkpoint = FBE_FALSE;

    //  Get the width of raid group
    fbe_base_config_get_width((fbe_base_config_t*) in_raid_group_p, &width);

    //  Get the exported capacity per disk, which is the disk ending LBA + 1 
    exported_disk_capacity = fbe_raid_group_get_exported_disk_capacity(in_raid_group_p);

    fbe_raid_geometry_get_journal_log_start_lba(raid_geometry_p, &journal_log_start_lba);
    fbe_raid_geometry_get_journal_log_end_lba(raid_geometry_p, &journal_log_end_lba);

    //  Initialize the highest checkpoint we have found for a disk that needs to be rebuilt 
    highest_checkpoint = FBE_LBA_INVALID;

    //  Loop through all of the disks in the raid group   
    for (position_index = 0; position_index < width; position_index++)
    {
        //  See if the disk needs to be rebuilt.  If not, move on to the next disk. 
        fbe_raid_group_rebuild_determine_if_rebuild_needed(in_raid_group_p, position_index, &requires_rebuild_b);
        if (requires_rebuild_b == FBE_FALSE)
        {
            continue;
        }

        /* Add the position to the degraded bitmask.
         */
        *out_degraded_positions_p |= (1 << position_index);

        //  If the disk is rb logging, then it can not be rebuilt.  Move on to the next disk. 
        fbe_raid_group_get_rb_logging(in_raid_group_p, position_index, &b_rebuild_logging); 
        if (b_rebuild_logging == FBE_TRUE)
        {
            continue; 
        }

        //  Get the edge state for this disk 
        fbe_base_config_get_block_edge((fbe_base_config_t*) in_raid_group_p, &edge_p, position_index);
        fbe_block_transport_get_path_state(edge_p, &path_state);

        //  Check if the edge is enabled (we can only rebuild if the disk/edge is ready).  If not, move onto 
        //  the next disk.
        if (path_state != FBE_PATH_STATE_ENABLED)
        {
            continue;
        }

        //  Get the rebuild checkpoint 
        fbe_raid_group_get_rebuild_checkpoint(in_raid_group_p, position_index, &cur_checkpoint); 

        /* If the checkpoint is invalid something is wrong (since we have already
         * determined that we are degraded). Log an error but continue.
         */
        if (cur_checkpoint == FBE_LBA_INVALID)
        {
            fbe_raid_group_trace(in_raid_group_p,
                                 FBE_TRACE_LEVEL_ERROR, 
                                 FBE_RAID_GROUP_DEBUG_FLAG_NONE,                               
                                 "raid_group: rebuild find disk needing action pos: %d checkpoint: 0x%llx unexpected \n",
                                 position_index,
                 (unsigned long long)cur_checkpoint);
            continue;
        }

        /* If the checkpoint is exactly at the per-disk exported (i.e. user space)
         * capacity it means that the user space rebuild has completed.  Since
         * we must perform the metadata rebuild prior to the user space rebuild,
         * this condition indicates that the entire rebuild is complete and we
         * need to advance the rebuild checkpoint to the end marker. 
         */
        if (cur_checkpoint == exported_disk_capacity)
        {
            /* Determine if we need to rebuild any metadata chunks.
             */
            fbe_raid_group_rebuild_get_next_metadata_needs_rebuild_lba(in_raid_group_p, position_index, &metadata_rebuild_lba);
            if (metadata_rebuild_lba == FBE_LBA_INVALID)
            {
                //  If no metadata chunks needs rebuild then set this position as rebuild complete
                *out_complete_rebuild_positions_p |= (1 << position_index);
                rebuild_complete_checkpoint = cur_checkpoint;
                continue;
            }
            else
            {
                /* Else set the current checkpoint as metadata rebuild lba.
                 */
                cur_checkpoint = metadata_rebuild_lba;
            }
        }

        //  If this checkpoint is the same or higher than that of any other disks ready to rebuild, then select 
        //  it as the disk to rebuild.
        // 
        //  We need to select it if it is the same, because we need to handle the case where the checkpoint is 
        //  0.  If two disks have the same checkpoint, it does not matter which of them we pick - both of their 
        //  checkpoints will be advanced after the rebuild.
        //
        //  Reset check point if we just finish rebuilding metadata 

        /*  Because we reset cur_checkpoint to 0 after rebuilding metadata, b_need_reset_checkpoint may not be updated to TRUE,
         *  if this is the later degraded position in a double degraded situation, if highest_checkpoint records 
         *  some different checkpoint from the first degraded position.
         *  That said, the second degrade position may not reset checkpoint to 0, 
         *  before first degrade position finish its rebuild.
         */
        if (cur_checkpoint > exported_disk_capacity)
        {
            cur_checkpoint = 0;
            b_curr_need_reset_checkpoint = FBE_TRUE;
        }
        else
        {
            b_curr_need_reset_checkpoint = FBE_FALSE;
        }
        if (highest_checkpoint == FBE_LBA_INVALID)
        {
            highest_checkpoint = cur_checkpoint;
            *out_positions_to_be_rebuilt_p = (1 << position_index);
            b_need_reset_checkpoint = b_curr_need_reset_checkpoint;
        }
        else if (cur_checkpoint > highest_checkpoint)
        {
            /* If this is the highest set the rebuild bitmask to this position.
             */
            highest_checkpoint = cur_checkpoint; 
            *out_positions_to_be_rebuilt_p = (1 << position_index);
            b_need_reset_checkpoint = b_curr_need_reset_checkpoint;
        }
        else if (cur_checkpoint == highest_checkpoint)
        {
            if (cur_checkpoint == 0)
            {
                /* if both need the same action, let's rebuild them together
                 */
                if (b_need_reset_checkpoint == b_curr_need_reset_checkpoint)
                {
                    *out_positions_to_be_rebuilt_p |= (1 << position_index);
                }
                else if (b_need_reset_checkpoint == FBE_TRUE)
                {
                    /* if last one needs reset, but current doesn't, let's hold off rebuild last position
                     */
                    *out_positions_to_be_rebuilt_p = (1 << position_index);
                    b_need_reset_checkpoint = FBE_FALSE;
                }
            }
            else
                {
                /* If it is equal to the highest, add it to the positions to be rebuilt.
                 */
                *out_positions_to_be_rebuilt_p |= (1 << position_index);
            }
        }

        //  Keep going in case we have another disk with a higher checkpoint and to find all the disks needing 
        //  rebuild

    } /* end for all positions in the raid group */

    /* If only `rebuild complete' is set update the highest checkpoint
     */
    if ((*out_complete_rebuild_positions_p != 0) &&
        (highest_checkpoint == FBE_LBA_INVALID))
    {
        highest_checkpoint = rebuild_complete_checkpoint;
    }

    fbe_raid_group_trace(in_raid_group_p,
                         FBE_TRACE_LEVEL_INFO, 
                         FBE_RAID_GROUP_DEBUG_FLAG_REBUILD_TRACING,                               
                         "rg: rebuild needing action needs_rb_bm: 0x%x rb_cmpl_bm: 0x%x checkpoint: 0x%llx reset %d\n",
                         *out_positions_to_be_rebuilt_p, *out_complete_rebuild_positions_p, (unsigned long long)highest_checkpoint, b_need_reset_checkpoint);

    //  Return success.  The output parameter has already been set as needed. 
    *out_rebuild_checkpoint_p = highest_checkpoint;
    *out_checkpoint_reset_needed_p = b_need_reset_checkpoint;

    return FBE_STATUS_OK;

} // End fbe_raid_group_rebuild_find_disk_needing_action()

/*!****************************************************************************
 * fbe_raid_group_rebuild_get_metadata_rebuild_position()
 ******************************************************************************
 * @brief
 *     This function finds the metadata rebuild position. We choose the position
 *    which has the lowest rebuild checkpoint and which is not rebuild logging.
 *   If 2 drives has the same checkpoint, it doesn't matter which drive we choose
 *   both drives will be rebuilt and checkpoints will be updated for both the drives
 *   
 * @param raid_group_p - pPointer to a raid group object
 * @param metadata_rebuild_pos_p - Pointer to rebuild position with lowest checkpoint.
 *
 * @return  fbe_status_t        
 *
 * @author
 *   01/27/2012 - Created. Ashwin Tamilarasan
 *
 ******************************************************************************/
fbe_status_t fbe_raid_group_rebuild_get_metadata_rebuild_position(
                                fbe_raid_group_t*               raid_group_p,
                                fbe_u32_t*                      metadata_rebuild_pos_p)
{

    fbe_u32_t                   width;
    fbe_u32_t                   position_index;
    fbe_block_edge_t*           edge_p;
    fbe_path_state_t            path_state;
    fbe_bool_t                  requires_rebuild_b;
    fbe_lba_t                   lowest_checkpoint;
    fbe_lba_t                   cur_checkpoint;
    fbe_bool_t                  b_rebuild_logging;
    fbe_lba_t                   configured_capacity;
    fbe_lba_t                   exported_disk_capacity;

    *metadata_rebuild_pos_p       = FBE_RAID_INVALID_DISK_POSITION;

    //  Get the width of raid group
    fbe_base_config_get_width((fbe_base_config_t*) raid_group_p, &width);
    configured_capacity = fbe_raid_group_get_disk_capacity(raid_group_p);
    exported_disk_capacity = fbe_raid_group_get_exported_disk_capacity(raid_group_p);

    //  Initialize the lowest checkpoint we have found for a disk that needs to be rebuilt 
    lowest_checkpoint = configured_capacity;

    //  Loop through all of the disks in the raid group   
    for (position_index = 0; position_index < width; position_index++)
    { 
        fbe_raid_group_rebuild_determine_if_rebuild_needed(raid_group_p, position_index, &requires_rebuild_b);
        if (requires_rebuild_b == FBE_FALSE)
        {
            continue;
        }

        fbe_raid_group_get_rb_logging(raid_group_p, position_index, &b_rebuild_logging); 
        if (b_rebuild_logging == FBE_TRUE)
        {
            continue; 
        }

        
        fbe_base_config_get_block_edge((fbe_base_config_t*) raid_group_p, &edge_p, position_index);
        fbe_block_transport_get_path_state(edge_p, &path_state);

        if (path_state != FBE_PATH_STATE_ENABLED)
        {
            continue;
        }

        fbe_raid_group_get_rebuild_checkpoint(raid_group_p, position_index, &cur_checkpoint); 

        //  If this checkpoint is the same or lower than that of any other disks ready to rebuild, then select 
        //  it as the disk to rebuild.
        if ((cur_checkpoint <= exported_disk_capacity) &&
            (fbe_raid_group_is_any_metadata_marked_NR(raid_group_p, (1<<position_index))==FBE_TRUE))
        {
            cur_checkpoint = 0;  /* reset check point back to 0 indicating needing MD rebuild */
        }
        if ((cur_checkpoint == 0) ||
            ((cur_checkpoint > exported_disk_capacity) && (cur_checkpoint <= lowest_checkpoint)))
        {
            lowest_checkpoint        = cur_checkpoint; 
            *metadata_rebuild_pos_p  = position_index;
        }

        //  Keep going in case we have another disk with a lower checkpoint and to find all the disks needing 
        //  rebuild
    }

    return FBE_STATUS_OK;

} // End fbe_raid_group_rebuild_get_metadata_rebuild_position()

/*!****************************************************************************
 * fbe_raid_group_rebuild_determine_if_rebuild_can_start
 ******************************************************************************
 * @brief
 *  This function is used to check if rebuild can start, it looks at
 *  the current load on its downstream edges to find whether it can start
 *  rebuild operation or not.
 * 
 *
 * @param in_raid_group_p           - pointer to a raid group object
 * @param out_can_rebuild_start_b   - pointer to boolean which tells caller whether
 *                                    rebuild can start or not.
 * 
 * @return fbe_status_t  
 *
 * @author
 *   10/26/2010 - Created. Dhaval Patel
 *
 ******************************************************************************/
fbe_status_t
fbe_raid_group_rebuild_determine_if_rebuild_can_start(
                                fbe_raid_group_t*   in_raid_group_p,
                                fbe_bool_t*         out_can_rebuild_start_b_p)
{

    fbe_bool_t is_load_okay_b;

    //  Initialize the rebuild start as false.
    *out_can_rebuild_start_b_p = FBE_FALSE;

    //  See if we are allowed to do a rebuild I/O based on the current load.  If not, just return here and the 
    //  monitor will reschedule on its normal cycle.
    fbe_raid_group_rebuild_check_permission_based_on_load(in_raid_group_p, &is_load_okay_b);
    if (is_load_okay_b == FBE_FALSE)
    {
        return FBE_STATUS_OK;
    }
    
    //  Set the can rebuild start as TRUE.
    *out_can_rebuild_start_b_p = FBE_TRUE;

    //  Return success
    return FBE_STATUS_OK;

} // End fbe_raid_group_rebuild_determine_if_rebuild_can_start()

/*!****************************************************************************
 * fbe_raid_group_rebuild_allocate_rebuild_context()
 ******************************************************************************
 * @brief
 *   This function is called by the raid group monitor to allocate the control
 *   operation which tracks rebuild operation.
 *
 * @param in_raid_group_p           - pointer to raid group
 * @param in_packet_p               - pointer to packet which will contain context 
 * @param out_rebuild_context_p     - pointer to the rebuild context
 * 
 * @return fbe_status_t  
 *
 * @author
 *   10/26/2010 - Created. Dhaval Patel.
 *
 ******************************************************************************/
fbe_status_t fbe_raid_group_rebuild_allocate_rebuild_context(
                                fbe_raid_group_t*   in_raid_group_p,
                                fbe_packet_t*       in_packet_p,
                                fbe_raid_group_rebuild_context_t ** out_rebuild_context_p)
{
    fbe_payload_ex_t *                   sep_payload_p;
    fbe_payload_control_operation_t *     control_operation_p = NULL;


    //  Initialize rebuild context as null
    *out_rebuild_context_p = NULL;

    //  Get the sep payload from the packet
    sep_payload_p = fbe_transport_get_payload_ex(in_packet_p);

    //  Allocate the rebuild control operation with rebuild tracking information.
    control_operation_p = fbe_payload_ex_allocate_control_operation(sep_payload_p);
    if(control_operation_p == NULL)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    //  Allocate the buffer to hold the rebuild tracking information.
    *out_rebuild_context_p = (fbe_raid_group_rebuild_context_t *) fbe_transport_allocate_buffer();
    if(*out_rebuild_context_p == NULL)
    {
        //  If memory allocation fails then release the control operation.
        fbe_payload_ex_release_control_operation(sep_payload_p, control_operation_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    //  Build the needs rebuild control operation
    fbe_payload_control_build_operation(control_operation_p,
                                        FBE_RAID_GROUP_CONTROL_CODE_REBUILD_OPERATION,
                                        *out_rebuild_context_p,
                                        sizeof(fbe_raid_group_rebuild_context_t));

    //  Increment the control operation
    fbe_payload_ex_increment_control_operation_level(sep_payload_p);

    //  Return good status
    return FBE_STATUS_OK;

} // End fbe_raid_group_rebuild_allocate_rebuild_context()


//  It releases rebuild context and control operation of payload in packet

/*!****************************************************************************
 * fbe_raid_group_rebuild_release_rebuild_context()
 ******************************************************************************
 * @brief
 *   This function is called by the raid group monitor to release the control
 *   operation which tracks rebuild operation.
 *
 * @param in_packet_p         - packet that the context was allocated to 
 * 
 * @return fbe_status_t  
 *
 * @author
 *   05/01/2013 - Created. Dave Agans.
 *
 ******************************************************************************/
fbe_status_t fbe_raid_group_rebuild_release_rebuild_context(
                                                fbe_packet_t*       in_packet_p)
{
    fbe_payload_ex_t                   *sep_payload_p;                 //  pointer to the payload
    fbe_payload_control_operation_t    *control_operation_p = NULL;    //  pointer to the control operation
    fbe_raid_group_rebuild_context_t   *rebuild_context_p = NULL;      //  rebuild context information
    fbe_status_t                        status;

    //  Get the sep payload, control operation, and rebuild context
    sep_payload_p = fbe_transport_get_payload_ex(in_packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(sep_payload_p);
    if (control_operation_p == NULL)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }
    fbe_payload_control_get_buffer(control_operation_p, &rebuild_context_p);

    //  Release the payload buffer
    fbe_transport_release_buffer(rebuild_context_p);

    //  Release the control operation
    status = fbe_payload_ex_release_control_operation(sep_payload_p, control_operation_p);

    //  Return status
    return status;

} // End fbe_raid_group_rebuild_release_rebuild_context()


/*!****************************************************************************
 *          fbe_raid_group_get_rebuild_block_count()
 ******************************************************************************
 *
 * @brief   This methods returns the number of `physical' blocks that should
 *          be rebuilt per rebuild request.  The number of blocks returned is
 *          always a multiple of chunk size.
 *
 * @param   raid_group_p  - Pointer to raid group information.
 * @param   rebuild_lba - The physical rebuild checkpoint.
 * @param   block_count_p - Pointer to block_count to be updated.
 * 
 * @return  fbe_status_t  
 *
 * @note    The returned block_count must be a multiple of block size.
 *
 * @author
 *  01/23/2012  Ron Proulx  - Created.
 *
 ******************************************************************************/
fbe_status_t fbe_raid_group_get_rebuild_block_count(fbe_raid_group_t *raid_group_p,
                                                    fbe_lba_t rebuild_lba,
                                                    fbe_block_count_t *block_count_p)
{
    fbe_raid_geometry_t    *raid_geometry_p = fbe_raid_group_get_raid_geometry(raid_group_p);
    fbe_chunk_size_t        chunk_size;
    fbe_block_count_t       logical_block_count;
    fbe_chunk_count_t       chunk_count;
    fbe_chunk_index_t       chunk_index;
    fbe_lba_t               end_lba;
    fbe_raid_group_metadata_positions_t raid_group_metadata_positions;
    fbe_u16_t               data_disks;
    fbe_lba_t               start_lba;

    /* Get the number of data disks in this raid object and convert teh physical
     * rebuild_lba to a logical.
     */
    fbe_raid_geometry_get_data_disks(raid_geometry_p, &data_disks);
    start_lba = (rebuild_lba * data_disks);

    /* Get the metadata positions.
     */
    fbe_raid_group_get_metadata_positions(raid_group_p, &raid_group_metadata_positions);

    /* Get the chunk size (rebuild request must be a multiple of chunk_size).
     */ 
    chunk_size = fbe_raid_group_get_chunk_size(raid_group_p);

    /* Get our default `logical' block consumption for rebuild request.
     */
    logical_block_count = (fbe_block_count_t)((chunk_size * fbe_raid_group_rebuild_background_op_chunks) * data_disks);
    end_lba = start_lba + logical_block_count - 1;

    /* We cannot span positions (i.e. cannot span the user -> metadata and
     * cannot span beyond metadata).
     */
    if ((start_lba < raid_group_metadata_positions.exported_capacity) &&
        (end_lba >= raid_group_metadata_positions.exported_capacity)     )
    {
        /* Request exceeds user space, truncate it.
         */
        end_lba = (raid_group_metadata_positions.exported_capacity - 1);
        logical_block_count = (end_lba - start_lba) + 1;
    }
    else if ((start_lba >= raid_group_metadata_positions.paged_metadata_lba)                                                         &&
             (end_lba >= (raid_group_metadata_positions.paged_metadata_lba + raid_group_metadata_positions.paged_metadata_capacity))    )
    {
        /* Request exceeds metadata space.  Truncate it.
         */
        end_lba = raid_group_metadata_positions.paged_metadata_lba + raid_group_metadata_positions.paged_metadata_capacity - 1;
        logical_block_count = (end_lba - start_lba) + 1;
    }

    /* Now calculate the number of chunks.
     */
    fbe_raid_geometry_calculate_chunk_range(raid_geometry_p, 
                                            start_lba, 
                                            logical_block_count,
                                            chunk_size,
                                            &chunk_index,
                                            &chunk_count);

    /* Now set the block count and return success.
     */
    *block_count_p = ((fbe_block_count_t)chunk_count * chunk_size);
    return FBE_STATUS_OK;
} 
/* end of fbe_raid_group_get_rebuild_block_count()*/

/*!****************************************************************************
 * fbe_raid_group_rebuild_initialize_rebuild_context
 ******************************************************************************
 * @brief
 *   This function is called by the raid group monitor to initialize the rebuild
 *   context before starting the rebuild operation.
 *
 * @param in_raid_group_p            - pointer to raid group information
 * @param in_rebuild_context_p       - pointer to rebuild context.
 * @param in_positions_to_be_rebuilt - Bitmask of all positions to be rebuilt
 * @param in_rebuild_lba             - Logical block address to start rebuild at
 * 
 * @return fbe_status_t  
 *
 * @author
 *   10/26/2010 - Created. Dhaval Patel.
 *
 ******************************************************************************/
fbe_status_t fbe_raid_group_rebuild_initialize_rebuild_context(
                                        fbe_raid_group_t                 *in_raid_group_p,
                                        fbe_raid_group_rebuild_context_t *in_rebuild_context_p, 
                                        fbe_raid_position_bitmask_t       in_positions_to_be_rebuilt,
                                        fbe_lba_t                         in_rebuild_lba)
{

    fbe_block_count_t   block_count;    // number of physical blocks to rebuild

    //  Get the chunk size, which is the number of blocks to rebuild  
    fbe_raid_group_get_rebuild_block_count(in_raid_group_p,
                                           in_rebuild_lba,
                                           &block_count);

    //  Set the rebuild data in the rebuild tracking structure 
    fbe_raid_group_rebuild_context_set_rebuild_start_lba(in_rebuild_context_p, in_rebuild_lba); 
    fbe_raid_group_rebuild_context_set_rebuild_block_count(in_rebuild_context_p, block_count);
    fbe_raid_group_rebuild_context_set_rebuild_positions(in_rebuild_context_p, in_positions_to_be_rebuilt);
    fbe_raid_group_rebuild_context_set_lun_object_id(in_rebuild_context_p, FBE_OBJECT_ID_INVALID);
    fbe_raid_group_rebuild_context_set_rebuild_is_lun_start(in_rebuild_context_p, FBE_FALSE);
    fbe_raid_group_rebuild_context_set_rebuild_is_lun_end(in_rebuild_context_p, FBE_FALSE);
    fbe_raid_group_rebuild_context_set_update_checkpoint_bitmask(in_rebuild_context_p, 0);
    fbe_raid_group_rebuild_context_set_update_checkpoint_lba(in_rebuild_context_p, FBE_LBA_INVALID);
    fbe_raid_group_rebuild_context_set_update_checkpoint_blocks(in_rebuild_context_p, 0);

    fbe_raid_group_trace(in_raid_group_p,
                         FBE_TRACE_LEVEL_INFO, 
                         FBE_RAID_GROUP_DEBUG_FLAG_REBUILD_TRACING,                               
                         "Raid Group: setup context start lba 0x%llx, block count 0x%llx\n", 
                         (unsigned long long)in_rebuild_context_p->start_lba,
                         (unsigned long long)in_rebuild_context_p->block_count);

    //  Return success
    return FBE_STATUS_OK;

} // End fbe_raid_group_rebuild_initialize_rebuild_context()

/*!****************************************************************************
 * fbe_raid_group_rebuild_get_rebuild_context()
 ******************************************************************************
 * @brief
 *   This function is used to get the pointer to rebuild context information.
 *
 * @param in_raid_group_p           - pointer to rebuild context
 * @param in_packet_p               - pointer to the packet
 * @param out_rebuild_context_p     - pointer to the rebuild context
 * 
 * @return fbe_status_t  
 *
 * @author
 *   10/26/2010 - Created. Dhaval Patel.
 *
 ******************************************************************************/
fbe_status_t fbe_raid_group_rebuild_get_rebuild_context(
                                fbe_raid_group_t*                           in_raid_group_p,
                                fbe_packet_t*                               in_packet_p,
                                fbe_raid_group_rebuild_context_t**          out_rebuild_context_p)
{
    fbe_payload_ex_t*                  sep_payload_p;                  // pointer to the sep payload
    fbe_payload_control_operation_t*    control_operation_p;            // pointer to the control operation

    //  Get a pointer to the rebuild context from the packet
    sep_payload_p = fbe_transport_get_payload_ex(in_packet_p);
    control_operation_p = fbe_payload_ex_get_present_control_operation(sep_payload_p);
    fbe_payload_control_get_buffer(control_operation_p, out_rebuild_context_p);
    return FBE_STATUS_OK;
} // End fbe_raid_group_rebuild_get_rebuild_context()

/*!****************************************************************************
 * fbe_raid_group_rebuild_get_needs_rebuild_context()
 ******************************************************************************
 * @brief
 *   This function is used to get the pointer to needs rebuild context information.
 *
 * @param in_raid_group_p           - pointer to rebuild context
 * @param in_packet_p               - pointer to the packet
 * @param out_needs_rebuild_context_p     - pointer to the needs rebuild context
 * 
 * @return fbe_status_t  
 *
 * @author
 *   07/26/2011 - Created. Ashwin Tamilarasan
 *
 ******************************************************************************/
fbe_status_t fbe_raid_group_rebuild_get_needs_rebuild_context(
                                fbe_raid_group_t*                           in_raid_group_p,
                                fbe_packet_t*                               in_packet_p,
                                fbe_raid_group_needs_rebuild_context_t**    out_needs_rebuild_context_p)
{
    fbe_payload_ex_t*                  sep_payload_p;                  
    fbe_payload_control_operation_t*    control_operation_p;            

    //  Get a pointer to the rebuild context from the packet
    sep_payload_p = fbe_transport_get_payload_ex(in_packet_p);
    if (sep_payload_p == NULL)
    {
        fbe_raid_group_trace(in_raid_group_p,
                             FBE_TRACE_LEVEL_WARNING, 
                             FBE_RAID_GROUP_DEBUG_FLAG_NONE,                               
                             "%s payload is NULL\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    control_operation_p = fbe_payload_ex_get_present_control_operation(sep_payload_p);
    if (control_operation_p == NULL)
    {
        fbe_raid_group_trace(in_raid_group_p,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_RAID_GROUP_DEBUG_FLAG_NONE,
                              "%s control operation is NULL\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    fbe_payload_control_get_buffer(control_operation_p, out_needs_rebuild_context_p);
    if (*out_needs_rebuild_context_p == NULL)
    {
        // The buffer pointer is NULL!
        fbe_raid_group_trace(in_raid_group_p,
                             FBE_TRACE_LEVEL_WARNING,
                             FBE_RAID_GROUP_DEBUG_FLAG_NONE,
                             "%s buffer pointer is NULL\n", __FUNCTION__);

        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        return FBE_STATUS_GENERIC_FAILURE;
    };
    return FBE_STATUS_OK;
} // End fbe_raid_group_rebuild_get_needs_rebuild_context()

/*!****************************************************************************
 * fbe_raid_group_rebuild_get_logical_rebuild_checkpoint()
 ******************************************************************************
 * @brief
 *   This function is used to get the pointer to rebuild context information.
 *
 * @param in_raid_group_p           - pointer to rebuild context.
 * @param in_position               - disk position in the RG 
 * @param in_all_rebuild_positions  - bitmask of all positions ready to rebuild 
 * 
 * @return fbe_status_t  
 *
 * @author
 *   10/26/2010 - Created. Dhaval Patel.
 *
 ******************************************************************************/
fbe_status_t fbe_raid_group_rebuild_get_logical_rebuild_checkpoint(
                                fbe_raid_group_t*               in_raid_group_p,
                                fbe_lba_t                       in_start_lba,
                                fbe_lba_t*                      out_logical_lba_p)
{

    fbe_u16_t               data_disks;
    fbe_raid_geometry_t*    raid_geometry_p;

    //  Determine if logical start lba is above the exported capacity of the RAID group object.
    raid_geometry_p = fbe_raid_group_get_raid_geometry(in_raid_group_p);
    fbe_raid_geometry_get_data_disks(raid_geometry_p, &data_disks);
    *out_logical_lba_p = in_start_lba * data_disks; 

    //  Return always good status.
    return FBE_STATUS_OK;

} // End fbe_raid_group_rebuild_get_logical_rebuild_checkpoint()


/*!****************************************************************************
 * fbe_raid_group_rebuild_get_next_metadata_needs_rebuild_lba
 ******************************************************************************
 * @brief
 *   This function is used to get the next metadata lba which needs rebuild.
 *
 * @param   in_raid_group_p             - pointer to a raid group object
 * @param   in_position                 - needs rebuild position
 * @param   out_metadata_rebuild_lba_p  - pointer to the next lba whihc needs rebuild
 * 
 * @return fbe_status_t  
 *
 * @author
 *   10/26/2010 - Created. Dhaval Patel.
 *
 ******************************************************************************/
fbe_status_t
fbe_raid_group_rebuild_get_next_metadata_needs_rebuild_lba(
                                        fbe_raid_group_t*           in_raid_group_p,
                                        fbe_u32_t                   in_position,
                                        fbe_lba_t*                  out_metadata_rebuild_lba_p)
{
    fbe_raid_position_bitmask_t         nr_position_mask;           // bitmask indicating drive to check NR for
    fbe_chunk_count_t                   cur_chunk_count;            // chunk number currently processing    
    fbe_raid_group_paged_metadata_t*    paged_md_md_p;              // pointer to paged metadata metadata 
    fbe_chunk_index_t                   paged_md_md_chunk_index;    // chunk index relative to paged MD MD
    fbe_chunk_index_t                   rg_chunk_index;             // chunk index relative to the RG 
    fbe_chunk_count_t                   chunk_count;
    fbe_lba_t                           current_rebuild_checkpoint;

    //  Initialize the metadata rebuild LBA as invalid
    *out_metadata_rebuild_lba_p = FBE_LBA_INVALID;

    //  Create a mask of the NR bit position that we are checking for 
    nr_position_mask = (1 << in_position);

    //  Get a pointer to the paged metadata metadata 
    paged_md_md_p = fbe_raid_group_get_paged_metadata_metadata_ptr(in_raid_group_p); 

    paged_md_md_chunk_index = FBE_CHUNK_INDEX_INVALID;
    fbe_raid_group_bitmap_get_md_of_md_count(in_raid_group_p, &chunk_count);
    //  Loop though the paged metadata metadata to find the first chunk that is marked need rebuild 
    for (cur_chunk_count = 0; cur_chunk_count < chunk_count; cur_chunk_count++)
    {
        if ((paged_md_md_p[cur_chunk_count].needs_rebuild_bits & nr_position_mask) != 0)
        {
            paged_md_md_chunk_index = cur_chunk_count;
            break;
        }
    }

    //  If no chunks are marked needs rebuild, then return here with success 
    if (paged_md_md_chunk_index == FBE_CHUNK_INDEX_INVALID)
    {
        return FBE_STATUS_OK;
    }

    // Get the rebuild checkpoint 
    fbe_raid_group_get_rebuild_checkpoint(in_raid_group_p, in_position, &current_rebuild_checkpoint);

    //  Translate the paged MD chunk to a RG-based chunk
    fbe_raid_group_bitmap_translate_paged_md_md_index_to_rg_chunk_index(in_raid_group_p, 
                                                                        paged_md_md_chunk_index,
                                                                        current_rebuild_checkpoint,
                                                                        &rg_chunk_index); 

    //  Get the LBA corresponding to the given chunk 
    fbe_raid_group_bitmap_get_lba_for_chunk_index(in_raid_group_p, rg_chunk_index, out_metadata_rebuild_lba_p); 

    //  Return success 
    return FBE_STATUS_OK;

} // End fbe_raid_group_rebuild_get_next_metadata_needs_rebuild_lba()


 
/*!****************************************************************************
 * fbe_raid_group_rebuild_send_event_to_check_lba()
 ******************************************************************************
 * @brief
 *   This function will send an event in order to determine if the LBA and range
 *   to be rebuilt is within a LUN.  The event's completion function will proceed
 *   accordingly.  However, before sending the event, it will first check if the 
 *   LBA is outside of the RG's data area and instead is in the paged metadata.  
 *   In that case, it will proceed directly to the next step which is to send i/o request.
 *
 * @param in_raid_group_p           - pointer to a raid group object
 * @param in_packet_p               - pointer to a control packet from the scheduler
 * @param in_start_lba              - starting LBA of the I/O. The LBA is relative 
 *                                    to the RAID extent on a single disk. 
 * @param in_block_count            - number of blocks to rebuild 
 *  
 * @return fbe_status_t
 * 
 * @author
 *  05/10/2010 - Created. Jean Montes.
 *
 *****************************************************************************/
fbe_status_t fbe_raid_group_rebuild_send_event_to_check_lba(
                                        fbe_raid_group_t*                   in_raid_group_p, 
                                        fbe_raid_group_rebuild_context_t*   rebuild_context_p,
                                        fbe_packet_t*                       in_packet_p)
{

    fbe_event_t*                        event_p;                // pointer to event
    fbe_event_stack_t*                  event_stack_p;          // pointer to event stack
    fbe_raid_geometry_t*                raid_geometry_p;        // pointer to raid geometry info 
    fbe_u16_t                           data_disks;             // number of data disks in the rg
    fbe_lba_t                           logical_start_lba;      // start LBA relative to the whole RG extent
    fbe_block_count_t                   logical_block_count;    // block count relative to the whole RG extent
    fbe_lba_t                           external_capacity;      // capacity of the RG available for data 
    fbe_event_permit_request_t          permit_request_info;    // gets LUN info from the permit request
    fbe_lba_t                           io_block_count = FBE_LBA_INVALID; // blk counts for permit IO check


    //  Trace function entry 
    FBE_RAID_GROUP_TRACE_FUNC_ENTRY(in_raid_group_p);

    //  Get the external capacity of the RG, which is the area available for data and does not include the paged
    //  metadata or signature area.  This is stored in the base config's capacity.
    fbe_base_config_get_capacity((fbe_base_config_t*) in_raid_group_p, &external_capacity);

    //  We want to check the start LBA against the external capacity.  When comparing it to the external capacity 
    //  or sending it up to the LUN, we need to use the "logical" LBA relative to the start of the RG.  The LBA
    //  passed in to this function is what is used by rebuild and is relative to the start of the disk, so convert
    //  it. 
    raid_geometry_p = fbe_raid_group_get_raid_geometry(in_raid_group_p);
    fbe_raid_geometry_get_data_disks(raid_geometry_p, &data_disks);
    logical_start_lba   = rebuild_context_p->start_lba * data_disks; 
    logical_block_count = rebuild_context_p->block_count * data_disks;

    /* Trace some information if enabled.
     */
    fbe_raid_group_trace(in_raid_group_p,
                         FBE_TRACE_LEVEL_INFO,
                         FBE_RAID_GROUP_DEBUG_FLAG_REBUILD_TRACING,
                         "raid_group: permit request for lba: 0x%llx blocks: 0x%llx exported: 0x%llx data_disks: %d\n",
                         (unsigned long long)logical_start_lba,
             (unsigned long long)logical_block_count,
             (unsigned long long)external_capacity,
             data_disks);

    //  If the LBA to check is outside of (greater than) the external capacity, then it is in the paged metadata 
    //  area and we do not need to check if it is in a LUN.  We'll go directly to sending the rebuild I/O.  
    if (logical_start_lba >= external_capacity)
    {
        // Send the rebuild I/O .  Its completion function will execute when it finishes. 
        fbe_raid_group_rebuild_send_io_request(in_raid_group_p, in_packet_p, 
                                               FBE_FALSE, /* Don't need to break the context. */
                                               rebuild_context_p->start_lba, 
                                               rebuild_context_p->block_count);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED; 
    }

    /* Make sure that the permit request is limit to one client only. If the IO range is
     * across one client, we need to update the block count so that permit request goes to 
     * only one client.
     */

    /* make sure that the permit request IO range limit to only one client */
    if(raid_geometry_p->raid_type != FBE_RAID_GROUP_TYPE_INTERNAL_MIRROR_UNDER_STRIPER)
    {
         io_block_count = rebuild_context_p->block_count;
         fbe_raid_group_monitor_update_client_blk_cnt_if_needed(in_raid_group_p, rebuild_context_p->start_lba, &io_block_count);
         if((io_block_count != FBE_LBA_INVALID) && (rebuild_context_p->block_count != io_block_count))
        {

            fbe_base_object_trace((fbe_base_object_t*) in_raid_group_p, 
                FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO, 
                "Raid Group: send rebuild permit s_lba 0x%llx, orig blk 0x%llx, new blk 0x%llx \n", (unsigned long long)rebuild_context_p->start_lba,
                (unsigned long long)rebuild_context_p->block_count, (unsigned long long)io_block_count);

            /* update the rebuild block count so that permit request will limit to only one client */               
            rebuild_context_p->block_count = io_block_count;
            logical_block_count = rebuild_context_p->block_count * data_disks;
            
        }
    }

    //  We need to check if the LBA is within a LUN.   Allocate an event and set it up. 
    event_p = fbe_memory_ex_allocate(sizeof(fbe_event_t));
    fbe_event_init(event_p);
    fbe_event_set_master_packet(event_p, in_packet_p);
    event_stack_p = fbe_event_allocate_stack(event_p);

    //  Initialize the permit request information to "no LUN found"
    fbe_event_init_permit_request_data(event_p, &permit_request_info,
                                       FBE_PERMIT_EVENT_TYPE_IS_CONSUMED_REQUEST);

    //  Set the starting LBA and the block count to be checked in the event stack data
    fbe_event_stack_set_extent(event_stack_p, logical_start_lba, logical_block_count); 

    //  Set the completion function in the event's stack data
    fbe_event_stack_set_completion_function(event_stack_p, 
            (fbe_event_completion_function_t)fbe_raid_group_rebuild_send_event_to_check_lba_completion, in_raid_group_p);

    //  Send a "permit request" event to check if this LBA and block are in a LUN or not  
    fbe_base_config_send_event((fbe_base_config_t*) in_raid_group_p, FBE_EVENT_TYPE_PERMIT_REQUEST, event_p);

    //  Return success   
    return FBE_STATUS_OK;

} // End fbe_raid_group_rebuild_send_event_to_check_lba()


/*!****************************************************************************
 * fbe_raid_group_rebuild_send_event_to_check_lba_completion()
 ******************************************************************************
 * @brief
 *   This is the completion function for the event which determines if the current 
 *   area to be rebuilt is in a bound area or an unbound area.  It will retrieve
 *   the data from the event and then call a function to get the paged metadata
 *   (chunk info).  
 *
 * @param in_event_p            - pointer to a Permit Request event
 * @param in_context            - context, which is a pointer to the raid group
 *                                object
 *
 * @return fbe_status_t
 *
 * @author
 *  05/10/2010 - Created. Jean Montes.
 *
 *****************************************************************************/
static fbe_status_t fbe_raid_group_rebuild_send_event_to_check_lba_completion(
                                        void *                    event_p,
                                        fbe_event_completion_context_t  in_context)
{
    fbe_event_t*                        in_event_p;                   //  pointer to the raid group 

    fbe_raid_group_t*                   raid_group_p;                   //  pointer to the raid group 
    fbe_event_stack_t*                  event_stack_p;                  //  event stack pointer
    fbe_event_status_t                  event_status;                   //  event status
    fbe_packet_t*                       packet_p;                       //  pointer to fbe packet
    fbe_event_permit_request_t          permit_request_info;            //  gets LUN info from the permit request
    fbe_block_count_t                   unconsumed_block_count = 0;     //  unconsumed block count
    fbe_raid_group_rebuild_context_t   *rebuild_context_p;              //  pointer to the rebuild context
    fbe_status_t                        status;                         //  status of the operation
    fbe_raid_geometry_t                *raid_geometry_p;
    fbe_u16_t                           data_disks;
    fbe_u32_t                           event_flags = 0;
    fbe_bool_t                          is_io_consumed;
    fbe_block_count_t                   io_block_count = FBE_LBA_INVALID;

    //  Trace function entry 
    FBE_RAID_GROUP_TRACE_FUNC_ENTRY((fbe_raid_group_t*) in_context);

    in_event_p = (fbe_event_t*)event_p;
    //  Cast the context to a pointer to a raid group object 
    raid_group_p = (fbe_raid_group_t*) in_context;
    raid_geometry_p = fbe_raid_group_get_raid_geometry(raid_group_p);
    fbe_raid_geometry_get_data_disks(raid_geometry_p, &data_disks);

    //  Get the data that we'll need out of the event - stack, status, packet pointer 
    event_stack_p = fbe_event_get_current_stack(in_event_p);
    fbe_event_get_status(in_event_p, &event_status);
    fbe_event_get_flags (in_event_p, &event_flags);
    fbe_event_get_master_packet(in_event_p, &packet_p);

    //  Get a pointer to the rebuild context from the packet
    fbe_raid_group_rebuild_get_rebuild_context(raid_group_p, packet_p, &rebuild_context_p);

    //  Get the permit request info about the LUN (object id and start/end of LUN) and store it in the rebuild
    //  tracking structure
    fbe_event_get_permit_request_data(in_event_p, &permit_request_info); 
    rebuild_context_p->lun_object_id     = permit_request_info.object_id;
    rebuild_context_p->is_lun_start_b    = permit_request_info.is_start_b;
    rebuild_context_p->is_lun_end_b      = permit_request_info.is_end_b;
    unconsumed_block_count               = permit_request_info.unconsumed_block_count;

    /*! @note Free all associated resources and release the event.
     *        Do not access event information after this point.
     */
    fbe_event_release_stack(in_event_p, event_stack_p);
    fbe_event_destroy(in_event_p);
    fbe_memory_ex_release(in_event_p);

    /* Trace some information if enalbed.
     */
    fbe_raid_group_trace(raid_group_p,
                         FBE_TRACE_LEVEL_INFO,
                         FBE_RAID_GROUP_DEBUG_FLAG_REBUILD_TRACING,
                         "raid_group: rebuild permit status: 0x%x obj: 0x%x is_start: %d is_end: %d unconsumed: 0x%llx \n",
                         event_status, rebuild_context_p->lun_object_id, rebuild_context_p->is_lun_start_b,
                         rebuild_context_p->is_lun_end_b, (unsigned long long)unconsumed_block_count);

    /* complete the packet if permit request denied or returned busy */
    if((event_flags == FBE_EVENT_FLAG_DENY) || (event_status == FBE_EVENT_STATUS_BUSY))
    {
        fbe_raid_group_trace(raid_group_p,
                             FBE_TRACE_LEVEL_INFO, FBE_RAID_GROUP_DEBUG_FLAGS_MONITOR_EVENT_TRACING,
                             "raid_group: rebuild permit event request denied. lba: 0x%llx blocks: 0x%llx status: %d flags: 0x%x\n",
                             rebuild_context_p->start_lba, rebuild_context_p->block_count, event_status, event_flags);

        // our client is busy, we will need to try again later, we can't do the rebuild now
        // Complete the packet
        fbe_transport_set_status(packet_p, FBE_STATUS_BUSY, 0);
        fbe_transport_complete_packet(packet_p);

        return FBE_STATUS_OK;
    }

    /* Handle all the possible event status.
     */
    switch(event_status)
    {
        case FBE_EVENT_STATUS_OK:

            io_block_count = rebuild_context_p->block_count;

            /* check if the rebuild IO range is overlap with unconsumed area either at beginning or at the end */
            status = fbe_raid_group_monitor_io_get_consumed_info(raid_group_p,
                                                    unconsumed_block_count,
                                                    rebuild_context_p->start_lba, 
                                                    &io_block_count,
                                                    &is_io_consumed);
            if(status != FBE_STATUS_OK)
            {
                fbe_base_object_trace((fbe_base_object_t*) raid_group_p, FBE_TRACE_LEVEL_INFO, 
                    FBE_TRACE_MESSAGE_ID_INFO, "raid_group: rebuild failed to determine IO consumed area\n");
                fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
                fbe_transport_complete_packet(packet_p);

                return FBE_STATUS_OK;
            }
            
            if(is_io_consumed)
            {
                if(rebuild_context_p->block_count != io_block_count)
                {
                    /* IO range is consumed at the beginning and unconsumed at the end
                     * update the rebuild block count with updated block count which only cover the 
                     * consumed area
                     */

                    fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                            FBE_TRACE_LEVEL_INFO,
                            FBE_TRACE_MESSAGE_ID_INFO,
                            "rebuild IO has unconsumed area at end, lba 0x%llx," 
                            "orig_b 0x%llx, (unsigned long long)new_b 0x%llx\n",
                            rebuild_context_p->start_lba, rebuild_context_p->block_count, io_block_count);                    

                    rebuild_context_p->block_count = io_block_count;
                    
                }
            }
            else
            {
                /* IO range is unconsumed at the beginning and consumed at the end
                 * so can't continue with rebuild
                 * clear the NR bits in the paged metadata and advance the checkpoint.
                 */
                fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                        FBE_TRACE_LEVEL_INFO,
                        FBE_TRACE_MESSAGE_ID_INFO,
                            "rebuild IO has unconsumed area at start, move chkpt, lba 0x%llx," 
                            "orig_b 0x%llx, (unsigned long long)new_b 0x%llx\n",
                        rebuild_context_p->start_lba, rebuild_context_p->block_count, io_block_count); 
                
                rebuild_context_p->block_count = io_block_count;
                status = fbe_raid_group_clear_nr_control_operation(raid_group_p, packet_p);

                return FBE_STATUS_MORE_PROCESSING_REQUIRED;
            }

            break;
        case FBE_EVENT_STATUS_NO_USER_DATA:


            io_block_count = rebuild_context_p->block_count;
            
            /* find out the unconsumed block counts that needs to skip with advance rebuild checkpoint */
            status = fbe_raid_group_monitor_io_get_unconsumed_block_counts(raid_group_p, unconsumed_block_count, 
                                                rebuild_context_p->start_lba, &io_block_count);
           if(status != FBE_STATUS_OK)
            {
                fbe_base_object_trace((fbe_base_object_t*) raid_group_p, FBE_TRACE_LEVEL_INFO, 
                    FBE_TRACE_MESSAGE_ID_INFO, "Rebuild io get_unconsumed blks failed, start lba 0x%llx blks 0x%llx\n",
                    (unsigned long long)rebuild_context_p->start_lba, (unsigned long long)rebuild_context_p->block_count);

                fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
                fbe_transport_complete_packet(packet_p);
                return FBE_STATUS_OK;
            }

            fbe_base_object_trace((fbe_base_object_t *)raid_group_p, FBE_TRACE_LEVEL_INFO, 
                          FBE_TRACE_MESSAGE_ID_INFO, 
                          "raid_group: Rebuild,  No user data: lba: 0x%llx  new blocks: 0x%llx unconsumed: 0x%llx\n",
                          (unsigned long long)rebuild_context_p->start_lba, (unsigned long long)io_block_count, (unsigned long long)unconsumed_block_count);

            if (io_block_count == 0) {
                /* The edge may have been attached after the event was returned with no user data.  
                 * The result is an io_block_count of zero.  Let the monitor run again to retry.
                 */
                fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
                fbe_transport_complete_packet(packet_p);
                return FBE_STATUS_MORE_PROCESSING_REQUIRED;
            }
            /* set the updated block count to advance rebuild checkpoint */
            rebuild_context_p->block_count = io_block_count;
            
            //  If the area is not consumed by an upstream object, then we do not need to rebuild it.  Instead, clear the NR 
            //  bits in the paged metadata and advance the checkpoint.
            status = fbe_raid_group_clear_nr_control_operation(raid_group_p, packet_p);
            return FBE_STATUS_MORE_PROCESSING_REQUIRED;

        default:
            fbe_base_object_trace((fbe_base_object_t*) raid_group_p, 
                                  FBE_TRACE_LEVEL_ERROR, 
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                                  "%s unexpected event status: %d\n", __FUNCTION__, event_status);
            fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
            fbe_transport_complete_packet(packet_p);
            return FBE_STATUS_OK;

    } /* end if switch on event_status*/

    // Send the rebuild I/O .  Its completion function will execute when it finishes. 
    status = fbe_raid_group_rebuild_send_io_request(raid_group_p, packet_p,
                                                    FBE_TRUE, /* We must break the context to release the event thread. */ 
                                                    rebuild_context_p->start_lba, 
                                                    rebuild_context_p->block_count);

    return status;

} // End fbe_raid_group_rebuild_send_event_to_check_lba_completion()


/*!****************************************************************************
 * fbe_raid_group_rebuild_send_io_request()
 ******************************************************************************
 * @brief
 *  This function will issue a rebuild I/O request.  When it finishes, its 
 *  completion function will be invoked. 
 *
 * @param in_raid_group_p           - pointer to a raid group object
 * @param in_packet_p               - pointer to a control packet from the scheduler
 * @param b_queue_to_block_transport - FBE_TRUE must break context
 * @param in_start_lba              - starting LBA of the I/O. The LBA is relative 
 *                                    to the RAID extent on a single disk. 
 * @param in_block_count            - number of blocks to rebuild 
 *
 * @return fbe_status_t
 *
 * @author
 *  05/13/2010 - Created. Jean Montes.
 *
 *****************************************************************************/
static fbe_status_t fbe_raid_group_rebuild_send_io_request(
                                    fbe_raid_group_t*           in_raid_group_p, 
                                    fbe_packet_t*               in_packet_p,
                                    fbe_bool_t b_queue_to_block_transport,
                                    fbe_lba_t                   in_start_lba,
                                    fbe_block_count_t           in_block_count)
{

    fbe_traffic_priority_t              io_priority;                    //  priority for the I/O we are doing 
    fbe_status_t                        status;                         //  fbe status
    fbe_raid_group_rebuild_context_t*   rebuild_context_p;              // pointer to the rebuild context

    //  Trace function entry 
    FBE_RAID_GROUP_TRACE_FUNC_ENTRY(in_raid_group_p);

    //  Get a pointer to the rebuild context from the packet
    fbe_raid_group_rebuild_get_rebuild_context(in_raid_group_p, in_packet_p, &rebuild_context_p);

    //  Set the rebuild operation state to rebuild i/o
    fbe_raid_group_rebuild_context_set_rebuild_state(rebuild_context_p, FBE_RAID_GROUP_REBUILD_STATE_REBUILD_IO);

    //  Set the rebuild I/O completion function
    fbe_transport_set_completion_function(in_packet_p, fbe_raid_group_rebuild_send_io_request_completion, 
            (fbe_packet_completion_context_t) in_raid_group_p);

    //  Set the rebuild priority in the packet
    io_priority = fbe_raid_group_get_rebuild_priority(in_raid_group_p);
    fbe_transport_set_traffic_priority(in_packet_p, io_priority);

    /* If this method is invoked from the event thread we must break the
     * context (to free the event thread).
     */
    if (b_queue_to_block_transport == FBE_TRUE)
    {
        /* Break the context before starting the I/O.
         */
        status = fbe_raid_group_executor_break_context_send_monitor_packet(in_raid_group_p, in_packet_p,
                                                         FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_REBUILD, in_start_lba, in_block_count);
    }
    else
    {
        /* Else we can start the I/O immediately.
         */
        status = fbe_raid_group_executor_send_monitor_packet(in_raid_group_p, in_packet_p,
                                                         FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_REBUILD, in_start_lba, in_block_count);
    }

    //  Return status from the call
    return status;

} // End fbe_raid_group_rebuild_send_io_request()


/*!****************************************************************************
 * fbe_raid_group_rebuild_send_io_request_completion()
 ******************************************************************************
 * @brief
 *  This is the completion function for a rebuild I/O operation.  It is called
 *  by the executor after it finishes a rebuild I/O.
 *
 * @param in_packet_p           - pointer to a control packet from the scheduler
 * @param in_context            - context, which is a pointer to the raid group
 *                                object
 *
 * @return fbe_status_t
 *
 * Note: This function must always return FBE_STATUS_OK so that the packet gets
 * completed to the next level.  (If any other status is returned, the packet  
 * will get stuck.)
 * 
 * @author
 *  12/07/2009 - Created. Jean Montes.
 *
 *****************************************************************************/
static fbe_status_t fbe_raid_group_rebuild_send_io_request_completion(
                                            fbe_packet_t*                   in_packet_p, 
                                            fbe_packet_completion_context_t in_context)
{

    fbe_raid_group_t*                       raid_group_p;                   //  pointer to the raid group 
    fbe_status_t                            packet_status;                  //  status from the packet 
    fbe_payload_ex_t*                      payload_p;                      //  pointer to sep payload
    fbe_payload_block_operation_t*          block_operation_p;              //  pointer to block operation
    fbe_payload_block_operation_status_t    block_status;                   //  block operation status 


    //  Cast the context to a pointer to a raid group object 
    raid_group_p = (fbe_raid_group_t*) in_context;

    //  Get the status of the operation that just completed
    packet_status = fbe_transport_get_status_code(in_packet_p);

    //  Get the payload and block operation for this I/O operation
    payload_p         = fbe_transport_get_payload_ex(in_packet_p);
    block_operation_p = fbe_payload_ex_get_block_operation(payload_p);

    //  Get the block status if the block operation is valid
    block_status = FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID; 
    if (block_operation_p != NULL)
    {
        fbe_payload_block_get_status(block_operation_p, &block_status);
    }

    //  Check for any error and trace if one occurred, but continue processing as usual
    if ((packet_status != FBE_STATUS_OK) || (block_status != FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS))
    {
        fbe_base_object_trace((fbe_base_object_t*) raid_group_p, 
            FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
            "%s rebuild I/O operation failed, status: 0x%x/0x%x \n", __FUNCTION__, packet_status, block_status);

        if (block_status != FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS)
        {
            fbe_transport_set_status(in_packet_p, FBE_STATUS_GENERIC_FAILURE, block_status);
        }
    }

    //  Release the block operation
    fbe_payload_ex_release_block_operation(payload_p, block_operation_p);

    //  Return status
    return packet_status;

} // End fbe_raid_group_rebuild_send_io_request_completion


/*!****************************************************************************
 * fbe_raid_group_clear_nr_control_operation()
 ******************************************************************************
 * @brief
 *  This function clears nr bits for region described by control operation.
 *
 * @param in_raid_group_p           - pointer to a raid group object
 * @param in_packet_p               - pointer to a control packet from the scheduler
 *
 * @return fbe_status_t
 *
 * @author
 *  06/13/2012 - Created. Prahlad Purohit - Created using refactored code.
 *
 *****************************************************************************/
fbe_status_t fbe_raid_group_clear_nr_control_operation(
                                        fbe_raid_group_t*       in_raid_group_p, 
                                        fbe_packet_t*           in_packet_p)
{

    fbe_raid_group_rebuild_context_t*   rebuild_context_p;          // pointer to the rebuild context

    fbe_lba_t                           end_lba;                    // end LBA to clear NR
    fbe_chunk_index_t                   start_chunk;                // first chunk to clear NR 
    fbe_chunk_index_t                   end_chunk;                  // last chunk to clear NR 
    fbe_chunk_count_t                   chunk_count;                // number of chunks to clear    
    fbe_raid_group_paged_metadata_t     chunk_data;                 // all data for a chunk 
    fbe_raid_geometry_t                 *raid_geometry_p;
    fbe_u16_t                           disk_count;
    fbe_lba_t                           rg_lba;


    //  Trace function entry 
    FBE_RAID_GROUP_TRACE_FUNC_ENTRY(in_raid_group_p);

    //  Get a pointer to the rebuild context from the packet
    fbe_raid_group_rebuild_get_rebuild_context(in_raid_group_p, in_packet_p, &rebuild_context_p);

    //  Set the completion function 
    fbe_transport_set_completion_function(in_packet_p, fbe_raid_group_clear_nr_control_operation_completion, (fbe_packet_completion_context_t) in_raid_group_p);

    //  Get the ending LBA to be marked.
    end_lba = rebuild_context_p->start_lba + rebuild_context_p->block_count - 1;

    //  Convert the starting and ending LBAs to chunk indexes 
    fbe_raid_group_bitmap_get_chunk_index_for_lba(in_raid_group_p, rebuild_context_p->start_lba, &start_chunk);
    fbe_raid_group_bitmap_get_chunk_index_for_lba(in_raid_group_p, end_lba, &end_chunk);

    //  Calculate the number of chunks to be marked 
    chunk_count = (fbe_chunk_count_t) (end_chunk - start_chunk + 1);

    fbe_raid_group_trace(in_raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_RAID_GROUP_DEBUG_FLAGS_TRACE_PMD_NR,
                         "clear nr for pkt: 0x%x lba %llx to %llx chunk %llx to %llx\n", 
                         (fbe_u32_t)in_packet_p, (unsigned long long)rebuild_context_p->start_lba, (unsigned long long)end_lba, (unsigned long long)start_chunk, (unsigned long long)end_chunk);


    //  Set up the bits of the metadata that need to be written, which are the NR bits 
    fbe_set_memory(&chunk_data, 0, sizeof(fbe_raid_group_paged_metadata_t));
    chunk_data.needs_rebuild_bits = FBE_RAID_POSITION_BITMASK_ALL_SET;

    /* Clear the verify bits since a rebuild of this chunk is as good as a verify.
     */
    chunk_data.verify_bits = FBE_RAID_BITMAP_VERIFY_ALL;

    raid_geometry_p = fbe_raid_group_get_raid_geometry(in_raid_group_p);
    fbe_raid_geometry_get_data_disks(raid_geometry_p, &disk_count);

    rg_lba = rebuild_context_p->start_lba * disk_count;

    //  If the chunk is not in the data area, then use the non-paged metadata service to update it
    if (fbe_raid_geometry_is_metadata_io(raid_geometry_p, rg_lba))
    {
        fbe_raid_group_bitmap_write_chunk_info_using_nonpaged(in_raid_group_p, in_packet_p, start_chunk,
                                                              chunk_count, &chunk_data, TRUE);
    }
    else
    {
        //  The chunks are in the user data area.  Use the paged metadata service to update them. 
        fbe_raid_group_bitmap_update_paged_metadata(in_raid_group_p, in_packet_p, start_chunk,
                                                    chunk_count, &chunk_data, FBE_TRUE);
    }

    //  Return more processing since we have a packet outstanding 
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;

} // End fbe_raid_group_clear_nr_control_operation()


/*!****************************************************************************
 * fbe_raid_group_clear_nr_control_operation_completion()
 ******************************************************************************
 * @brief
 *   This is the completion function NR clear using control operation.
 * 
 * @param in_packet_p   - pointer to a control packet from the scheduler
 * @param in_context    - completion context, which is a pointer to the raid 
 *                        group object
 * 
 * @return  fbe_status_t  
 *
 * @author
 *   06/13/2012 - Created. Prahlad Purohit
 *  
 ******************************************************************************/
static fbe_status_t fbe_raid_group_clear_nr_control_operation_completion(
                                    fbe_packet_t*                   in_packet_p,
                                    fbe_packet_completion_context_t in_context)

{
    fbe_raid_group_t*               raid_group_p;                   // pointer to raid group object 
    fbe_status_t                    status;                         // fbe status
    fbe_status_t                            packet_status;
    fbe_payload_ex_t*                   sep_payload_p = NULL;
    fbe_payload_control_operation_t     *control_operation_p = NULL;
    fbe_payload_control_status_t        control_status;
    

    //  Get the raid group pointer from the input context 
    raid_group_p = (fbe_raid_group_t*) in_context; 

    sep_payload_p           = fbe_transport_get_payload_ex(in_packet_p);
    control_operation_p = fbe_payload_ex_get_present_control_operation(sep_payload_p);

    packet_status = fbe_transport_get_status_code(in_packet_p);
    if (FBE_STATUS_OK == packet_status)
    {
        control_status = FBE_PAYLOAD_CONTROL_STATUS_OK;
    }
    else
    {
        control_status = FBE_PAYLOAD_CONTROL_STATUS_FAILURE;
    }

    fbe_payload_control_set_status(control_operation_p, control_status);

    if (control_status != FBE_PAYLOAD_CONTROL_STATUS_OK)
    {
        return FBE_STATUS_OK;
    }

    //  Update the non-paged metadata 
    status = fbe_raid_group_rebuild_unbound_area_update_nonpaged_metadata(raid_group_p, in_packet_p);

    //  Return more processing since we have a packet outstanding 
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;

} // End fbe_raid_group_clear_nr_control_operation_completion()


/*!****************************************************************************
 * fbe_raid_group_rebuild_unbound_area_update_nonpaged_metadata()
 ******************************************************************************
 * @brief
 *   This function will update the nonpaged metadata for an unbound area.  It
 *   will advance the rebuild checkpoint. 
 *
 * @param in_raid_group_p           - pointer to a raid group object
 * @param in_packet_p               - pointer to a control packet from the scheduler
 *
 * @return fbe_status_t
 *
 * @author
 *  05/18/2010 - Created. Jean Montes.
 *
 *****************************************************************************/
static fbe_status_t fbe_raid_group_rebuild_unbound_area_update_nonpaged_metadata(
                                        fbe_raid_group_t*       in_raid_group_p, 
                                        fbe_packet_t*           in_packet_p)
{

    fbe_raid_group_rebuild_context_t*   rebuild_context_p;          //  pointer to the rebuild context
    fbe_raid_position_bitmask_t         rb_logging_bitmask;
    fbe_raid_position_bitmask_t         positions_to_advance;       //  bitmask indicating which position(s) need their 
    fbe_lba_t                           new_checkpoint;             //  rebuild checkpoint advanced 
    fbe_u32_t                           rebuilding_disk_position;   //  current rebuilding disk position
    fbe_status_t                        status;                     //  fbe status


    //  Trace function entry 
    FBE_RAID_GROUP_TRACE_FUNC_ENTRY(in_raid_group_p);

    //  Get a pointer to the rebuild context from the packet
    fbe_raid_group_rebuild_get_rebuild_context(in_raid_group_p, in_packet_p, &rebuild_context_p);
    new_checkpoint = rebuild_context_p->start_lba + rebuild_context_p->block_count;

    //  Get a mask which shows which drives are rebuild logging
    fbe_raid_group_get_rb_logging_bitmask(in_raid_group_p, &rb_logging_bitmask);

    //  Get the current rebuilding disk position
    fbe_raid_group_rebuild_get_rebuilding_disk_position(in_raid_group_p,  rebuild_context_p->start_lba, &rebuilding_disk_position);
    if( rebuilding_disk_position == FBE_RAID_INVALID_DISK_POSITION)
    {
        fbe_base_object_trace((fbe_base_object_t*) in_raid_group_p, 
            FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
            "%s can't get rebuilding disk position for rebuild IO lba 0x%llx\n",
        __FUNCTION__, (unsigned long long)rebuild_context_p->start_lba);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    // Use the mask of the positions being rebuilt (that are not rebuild logging)
    // to set the positions to advance.
    positions_to_advance = rebuild_context_p->rebuild_bitmask & ~rb_logging_bitmask; 

    /* Trace some information if enabled.
     */
    fbe_base_object_trace((fbe_base_object_t*)in_raid_group_p, 
                         FBE_TRACE_LEVEL_DEBUG_LOW,
                         FBE_TRACE_MESSAGE_ID_INFO,
                         "raid_group: rebuild skip unbound pos_mask: 0x%x new checkpoint: 0x%llx\n",
                          positions_to_advance,
              (unsigned long long)new_checkpoint);

    fbe_raid_group_trace(in_raid_group_p, 
                         FBE_TRACE_LEVEL_INFO,
                         FBE_RAID_GROUP_DEBUG_FLAG_REBUILD_TRACING,
                         "raid_group: rebuild skip unbound pos_mask: 0x%x new checkpoint: 0x%llx\n",
                          positions_to_advance,
              (unsigned long long)new_checkpoint);

    //  Update the rebuild checkpoint and log LUN start/complete if needed.  This function sets up its own 
    //  completion function for the nonpaged metadata write, so we don't need to set one here. 
    status = fbe_raid_group_rebuild_perform_checkpoint_update_and_log(in_raid_group_p, 
                                                                      in_packet_p, 
                                                                      rebuild_context_p->start_lba, 
                                                                      rebuild_context_p->block_count, 
                                                                      positions_to_advance);

    //  Return more processing requred   
    return status;

} // End fbe_raid_group_rebuild_unbound_area_update_nonpaged_metadata()


/*!***************************************************************************
 *          fbe_raid_group_rebuild_process_iots_completion()
 *****************************************************************************
 *
 * @brief   This method handles the completion of (1) iots of a rebuild.  There
 *          could be multiple iots for a rebuild request.  Therefore we need to
 *          determine if this is the last iots and whether we need to clear 
 *          needs rebuild or not. 
 * 
 * @param   raid_group_p - Pointer to a raid group object
 * @param   packet_p - pointer to a packet 
 *
 * @note    It is assumed that the request is still on the termiantor queue.
 *
 * @author
 *  01/26/2012  Ron Proulx  - Created.
 *
 *****************************************************************************/
fbe_status_t fbe_raid_group_rebuild_process_iots_completion(fbe_raid_group_t *raid_group_p,
                                                            fbe_packet_t *packet_p)
{
    fbe_payload_ex_t           *sep_payload_p;
    fbe_raid_iots_t            *iots_p;
    fbe_payload_block_operation_status_t block_status;
    fbe_lba_t                   packet_lba;
    fbe_block_count_t           packet_blocks;
    fbe_bool_t                  b_is_request_complete = FBE_FALSE;
    fbe_raid_position_bitmask_t positions_rebuilt_mask;
    fbe_payload_block_operation_opcode_t original_opcode, current_opcode;
    fbe_raid_geometry_t*        raid_geometry_p = NULL; 
    fbe_class_id_t              class_id; 

    /* Get the payload, and IOTS for this I/O operation
     */
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    fbe_payload_ex_get_iots(sep_payload_p, &iots_p);
    fbe_raid_iots_get_packet_lba(iots_p, &packet_lba);
    fbe_raid_iots_get_packet_blocks(iots_p, &packet_blocks);

    /* Set up a bitmask of the positions that were actually rebuilt.  If any of 
     * the positions were marked rebuild logging in the I/O, then they were not 
     * rebuilt, and so that checkpoint should not be updated.
     */
    positions_rebuilt_mask = iots_p->chunk_info[0].needs_rebuild_bits & ~iots_p->rebuild_logging_bitmask; 

    /* Determine if the request is complete or not.
     */
    b_is_request_complete = fbe_raid_iots_is_request_complete(iots_p);
    fbe_raid_iots_get_block_status(iots_p, &block_status);

    /* There are (3) cases:
     *  1) The request failed.
     *          OR
     *  2) The request completed successfully.
     *          OR
     *  3) There is more work to do. 
     */
    raid_geometry_p = fbe_raid_group_get_raid_geometry(raid_group_p);
    class_id = fbe_raid_geometry_get_class_id(raid_geometry_p);    
    if (class_id == FBE_CLASS_ID_VIRTUAL_DRIVE && block_status == FBE_PAYLOAD_BLOCK_OPERATION_STATUS_MEDIA_ERROR)
    {
        /* Virtual drive copy with media error should be treated as complete.
         */
    }
    else if (block_status != FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS)
    {
        /* The request should have been marked complete.
         */
        if (b_is_request_complete == FBE_FALSE)
        {
            /* Trace an error and fail the request.
             */
            fbe_base_object_trace((fbe_base_object_t*)raid_group_p,
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "raid_group: iots lba: 0x%llx blks: 0x%llx failed with status: 0x%x but not complete.\n",
                                  (unsigned long long)iots_p->lba,
                  (unsigned long long)iots_p->blocks,
                  block_status); 
            fbe_raid_iots_set_block_status(iots_p, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID_REQUEST);
            fbe_raid_iots_set_block_qualifier(iots_p, FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_UNEXPECTED_ERROR);
        }

        /* The request failed.  Don't update the NR bits or the checkpoint.
         * Simply complete the request.
         */
        fbe_raid_group_iots_cleanup(packet_p, raid_group_p);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }

    /* The iots completed. We expect that the NR is set for all the
     * chunks.
     */
    if (positions_rebuilt_mask == 0)
    {
        /* Trace the error and cleanup without any modifications.
         */
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "raid_group: iots lba: 0x%llx blks: 0x%llx no positions rebuilt: 0x%x\n",
                              (unsigned long long)iots_p->lba,
                  (unsigned long long)iots_p->blocks,
                  positions_rebuilt_mask);
        fbe_raid_iots_set_block_status(iots_p, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID_REQUEST);
        fbe_raid_iots_set_block_qualifier(iots_p, FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_UNEXPECTED_ERROR);
        fbe_raid_group_iots_cleanup(packet_p, raid_group_p);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
     }

    /* Whether we are done or not we must update the NR metadata values.
     * The completion will determine if the the request is complete or
     * not before it updates the checkpoint.  Take the packet off the 
     * termination queue since the packet will be forwarded on and completed 
     * to us.
     */
    fbe_raid_group_remove_from_terminator_queue(raid_group_p, packet_p);
    fbe_transport_set_completion_function(packet_p, fbe_raid_group_iots_paged_metadata_update_completion, raid_group_p);

    /* Update the rebuild chunk info as required.
     */
    fbe_raid_group_needs_rebuild_clear_nr_for_iots(raid_group_p, iots_p);

    /* Set the opcode back to REBUILD.
     */
    fbe_raid_iots_get_opcode(iots_p, &original_opcode);
    fbe_raid_iots_get_current_opcode(iots_p, &current_opcode);
    if (current_opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_CHECK_ZEROED)
    {
        fbe_raid_iots_set_current_opcode(iots_p, original_opcode);
    }
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/* end of fbe_raid_group_rebuild_process_iots_completion() */

/*!****************************************************************************
 * fbe_raid_group_rebuild_update_checkpoint_after_io_completion()
 ******************************************************************************
 *
 * @brief   This function will update the rebuild checkpoint(s) after a rebuild
 *          I/O has completed.  It will get all of the data out of the packet 
 *          and then call a function to do the checkpoint update. 
 * 
 * @param in_raid_group_p           - pointer to a raid group object
 * @param in_packet_p               - pointer to a packet 
 *
 * @author
 *   04/16/2010 - Created. Jean Montes.
 *
 ******************************************************************************/
fbe_status_t fbe_raid_group_rebuild_update_checkpoint_after_io_completion(
                                        fbe_raid_group_t*       in_raid_group_p,
                                        fbe_packet_t*           in_packet_p)
{

    fbe_payload_ex_t                   *sep_payload_p;          //  pointer to sep payload
    fbe_payload_block_operation_t      *block_operation_p;      //  pointer to block operation
    fbe_payload_control_operation_t    *control_operation_p = NULL;    
    fbe_raid_iots_t                    *iots_p;                 //  pointer to the IOTS of the I/O
    fbe_lba_t                           start_lba;              //  starting LBA of the I/O
    fbe_block_count_t                   block_count;            //  number of blocks in the I/O
    fbe_raid_group_rebuild_context_t   *rebuild_context_p = NULL;              
    fbe_raid_position_bitmask_t         positions_rebuilt_mask; //  bitmask indicating position(s) rebuilt
    fbe_status_t                        status;                 //  status of the operation

    /* Get the payload, block operation, and IOTS for this I/O operation.
     */
    sep_payload_p     = fbe_transport_get_payload_ex(in_packet_p);
    block_operation_p = fbe_payload_ex_get_prev_block_operation(sep_payload_p);
    fbe_payload_ex_get_iots(sep_payload_p, &iots_p);

    /* Get a pointer to the rebuild context from the packet.
     */
    control_operation_p = fbe_payload_ex_get_present_control_operation(sep_payload_p);
    fbe_payload_control_get_buffer(control_operation_p, &rebuild_context_p);

    /* Set up a bitmask of the positions that were actually rebuilt.  If any 
     * of the positions were marked rebuild logging in the I/O, then they were 
     * not rebuilt, and so that checkpoint should not be updated.
     * (We always need to remove the positions that are missing)
     */
    positions_rebuilt_mask = rebuild_context_p->rebuild_bitmask & ~iots_p->rebuild_logging_bitmask; 
    positions_rebuilt_mask &= iots_p->chunk_info[0].needs_rebuild_bits; 

    //  Get the I/O's start LBA and block count from the block operation
    fbe_payload_block_get_lba(block_operation_p, &start_lba);
    fbe_payload_block_get_block_count(block_operation_p, &block_count);

    //  Update the checkpoint(s) and log LUN start/complete if needed 
    status = fbe_raid_group_rebuild_perform_checkpoint_update_and_log(in_raid_group_p, 
                                                                      in_packet_p, 
                                                                      start_lba, 
                                                                      block_count,
                                                                      positions_rebuilt_mask);

    //  Return success 
    return status;

} // End fbe_raid_group_rebuild_update_checkpoint_after_io_completion()

/*!****************************************************************************
 * fbe_raid_group_rebuild_perform_checkpoint_update_and_log
 ******************************************************************************
 * @brief
 *   This function will set the rebuild checkpoint for the indicated disk(s) 
 *   based on the input data.  The checkpoint will be set to the "start lba" 
 *   plus the block count.  
 * 
 *   If this function is called after a rebuild I/O has completed, the start 
 *   LBA is the start of the I/O.  If this function is called when the next chunk 
 *   in the bitmap does not need to be rebuilt and the checkpoint just needs to 
 *   be advanced, the "start lba" is the current rebuild checkpoint (of the disk
 *   that is "driving" the rebuild). 
 * 
 *   The function will log a message to the event log that a LUN has started or 
 *   completed rebuilding if that is the case.
 * 
 *   If the new checkpoint indicates that the rebuild of the disk has finished,  
 *   the function will also set a condition to move the checkpoint to the logical  
 *   end marker.
 * 
 * @param in_raid_group_p           - pointer to the raid group
 * @param in_packet_p               - pointer to a packet 
 * @param in_start_lba              - starting LBA; the new checkpoint is this value
 *                                    plus the block size 
 * @param in_block_count            - number of blocks to advance the checkpoint 
 * @param in_positions_to_advance   - bitmask indicating which disk position(s) to 
 *                                    advance the checkpoint for 
 *
 * @return fbe_status_t           
 *
 * @author
 *   04/28/2010 - Created. Jean Montes. 
 *
 ******************************************************************************/
static fbe_status_t fbe_raid_group_rebuild_perform_checkpoint_update_and_log(
                                    fbe_raid_group_t*           in_raid_group_p,
                                    fbe_packet_t*               in_packet_p, 
                                    fbe_lba_t                   in_start_lba,
                                    fbe_block_count_t           in_block_count,
                                    fbe_raid_position_bitmask_t in_positions_to_advance)
{

    fbe_lba_t                       exported_disk_capacity;     //  exported capacity per disk
    fbe_lba_t                       new_checkpoint;             //  new value for rebuild checkpoint
    fbe_u32_t                       width;                      //  raid group width
    fbe_u32_t                       position_index;             //  index for a specific disk position 
    fbe_lba_t                       cur_checkpoint;             //  current rebuild checkpoint
    fbe_raid_position_bitmask_t     cur_position_mask;          //  bitmask of the current disk's position
    fbe_raid_position_bitmask_t     checkpoint_change_bitmask;  //  bitmask of positions to change checkpoint for 
    fbe_status_t                    status;                     //  fbe status

    //  Get the per-disk exported capacity of the RG, which is its ending LBA + 1
    exported_disk_capacity = fbe_raid_group_get_exported_disk_capacity(in_raid_group_p);

    //  Calculate the new checkpoint based on the start LBA and block count.
    //  (The checkpoint reflects the last address rebuilt + 1.)
    cur_checkpoint = FBE_LBA_INVALID;
    new_checkpoint = in_start_lba + in_block_count;

    /* Trace some information if enabled.
     */
    position_index = fbe_raid_get_first_set_position(in_positions_to_advance);
    if (position_index != FBE_RAID_INVALID_DISK_POSITION)
    {
        fbe_raid_group_get_rebuild_checkpoint(in_raid_group_p, position_index, &cur_checkpoint); 
    }
    fbe_raid_group_trace(in_raid_group_p,
                         FBE_TRACE_LEVEL_INFO,
                         FBE_RAID_GROUP_DEBUG_FLAG_REBUILD_TRACING,
                         "raid_group: update checkpoint: 0x%llx lba: 0x%llx blks: 0x%llx advance: 0x%x first pos: %d \n",
                         (unsigned long long)cur_checkpoint,
                         (unsigned long long)in_start_lba,
                         (unsigned long long)in_block_count,
                         in_positions_to_advance, position_index);

    /* Always update the local copy of the blocks rebuilt.
     */
    fbe_raid_group_update_blocks_rebuilt(in_raid_group_p, in_start_lba, in_block_count, in_positions_to_advance);

    //  Get the width of raid group so we can loop through the disks 
    fbe_base_config_get_width((fbe_base_config_t*) in_raid_group_p, &width);

    //  Find the disk(s) indicated by the positions to advance mask, and update the checkpoints for each
    checkpoint_change_bitmask = 0; 
    for (position_index = 0; position_index < width; position_index++)
    {
        //  Create a bitmask for this position
        cur_position_mask = 1 << position_index; 

        /*! @note If this disk position does not need its checkpoint advanced
         *        skip it.  Note that when rebuilding metadata the `advance'
         *        maks will be set to -1 (all positions are checked).
         */
        if ((in_positions_to_advance & cur_position_mask) == 0)
        {
            continue;
        }

        //  This disk position may need to have its checkpoint moved.  Get its rebuild checkpoint. 
        fbe_raid_group_get_rebuild_checkpoint(in_raid_group_p, position_index, &cur_checkpoint); 

        /* If the current rebuild checkpoint is less than export capacity and the rebuild lba is exported 
         * capacity that means this is the first metadata chunk that was rebuilt. 
         * We will have to move the checkpoint from current to end of first metdata chunk.
         * current checkpoint can be either zero or non zero valid checkpoint.
         * In some cases, if RG persist its non paged metadata while rebuild is in progress,
         * rebuild checkpoint also get persisted and after that if both SP rebooted or paniced,
         * we will first rebuild metadata and in that case we will have non zero rebuild checkpoint
         */
        if ((cur_checkpoint < exported_disk_capacity) && 
                (in_start_lba == exported_disk_capacity)) 
        {
            /* `Increment' the checkpoint from current to the end of the last 
             * metadata lba that was just rebuilt.
             */
            in_start_lba = cur_checkpoint;
            in_block_count = in_block_count + (exported_disk_capacity - cur_checkpoint);

            /* Trace this transition.
             */
            fbe_base_object_trace((fbe_base_object_t*)in_raid_group_p, FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "raid_group: metadata update checkpoint: 0x%llx new: 0x%llx pos: %d \n",
                                  (unsigned long long)cur_checkpoint, (unsigned long long)(in_start_lba + in_block_count), position_index); 
            
        }

        /* There are (3) cases where we want to ignore this position:
         *      1) Checkpoint is set to end marker and therefore isn't 
         *         rebuilding (this should only occur for a metadata rebuild).
         *                          OR  
         *      2) There is more than (1) position that requires a rebuild
         *         (3-way mirror, RAID-6) but those positions are not part
         *         of this rebuild request.
         *                          OR  
         */
        else if (cur_checkpoint != in_start_lba) 
        {
            /* Only trace if the checkpoint isn't set to the end marker.
             */
            if (cur_checkpoint != FBE_LBA_INVALID)
            {
                fbe_base_object_trace((fbe_base_object_t*)in_raid_group_p, FBE_TRACE_LEVEL_INFO,
                                      FBE_TRACE_MESSAGE_ID_INFO,
                                      "raid_group: update checkpoint: 0x%llx pos: %d doesn't match rebuild lba: 0x%llx\n",
                                      (unsigned long long)cur_checkpoint,
                      position_index,
                      (unsigned long long)in_start_lba);
            }

            /* Skip this position when determining which positions to update 
             * for the rebuild completion.
             */
            continue;
        }
        /*  The third case.
         *      3) This position just finished rebuilding user area, and no metadata marked NR.
         */
        else if ((cur_checkpoint == exported_disk_capacity) && 
            ((fbe_raid_group_is_any_metadata_marked_NR(in_raid_group_p, cur_position_mask))==FBE_FALSE))
        {
            /* Skip this position when determining which positions to update 
             * for the rebuild completion.
             */
            continue;
        }

        //  Set this disk position in the bitmask indicating which checkpoints are to be changed 
        checkpoint_change_bitmask = checkpoint_change_bitmask | cur_position_mask;
    }

    /* If there is no position to update just complete the packet 
       We could get into this situation for triple mirror and RAID 6
    */
    if(checkpoint_change_bitmask == 0)
    {
        fbe_transport_set_status(in_packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(in_packet_p);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }

    //  Log a message to the event log if this was the start or the end of a LUN.  We are logging this
    //  before updating the checkpoint on disk in case the array goes down in between.  In that case, two
    //  messages will be logged instead of none.
    status = fbe_raid_group_logging_break_context_log_and_checkpoint_update(in_raid_group_p, in_packet_p, 
                                                                            checkpoint_change_bitmask, 
                                                                            in_start_lba,
                                                                            in_block_count); 

    //  Return the status from the call, which will indicate if we have a packet outstanding or not 
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;

} // End fbe_raid_group_rebuild_perform_checkpoint_update_and_log()


/*!****************************************************************************
 * fbe_raid_group_incr_rebuild_checkpoint()
 ******************************************************************************
 * @brief
 *   This function will increment the rebuild checkpoint to the new value 
 *   for all the disk positions passed in.
 *
 *
 * @param raid_group_p           - pointer to a raid group object
 * @param packet_p               - pointer to a control packet from the scheduler
 * @param checkpoint             - new checkpoint 
 * @param positions_to_change    - bitmask indicating which disk position(s) to 
 *                                    change the checkpoint for 
 * 
 * @return  fbe_status_t  
 *
 * @author
 *   01/25/2012 - Created. Ashwin Tamilarasan
 *
 ******************************************************************************/
fbe_status_t fbe_raid_group_incr_rebuild_checkpoint(
                                                fbe_raid_group_t*               raid_group_p,
                                                fbe_packet_t*                   packet_p,
                                                fbe_lba_t                       start_lba,
                                                fbe_block_count_t               block_count,
                                                fbe_raid_position_bitmask_t     positions_to_change) 
{
    fbe_status_t    status;
    fbe_u32_t       number_of_positions;
    fbe_u32_t       rebuild_index = FBE_RAID_GROUP_INVALID_INDEX;
    fbe_u64_t       metadata_offset;
    fbe_u64_t       second_metadata_offset = 0;
    fbe_u32_t       ms_since_checkpoint;
    fbe_time_t      last_checkpoint_time;
    fbe_u32_t       peer_update_period_ms;
    fbe_lba_t       exported_per_disk_capacity = 0;

    /*  If there are 2 positions being changed, always start at rebuild checkpoint info entry 0.  Otherwise, 
       start at the entry of the specific rebuild position to be changed.
    */
    number_of_positions = fbe_raid_get_bitcount((fbe_u32_t) positions_to_change);
    if (number_of_positions == FBE_RAID_GROUP_MAX_REBUILD_POSITIONS)
    {
        metadata_offset = (fbe_u64_t) 
            (&((fbe_raid_group_nonpaged_metadata_t*)0)->rebuild_info.rebuild_checkpoint_info[0]);

        second_metadata_offset = (fbe_u64_t) 
            (&((fbe_raid_group_nonpaged_metadata_t*)0)->rebuild_info.rebuild_checkpoint_info[1]);
    }
    else
    {
        status = fbe_raid_group_get_rebuild_index(raid_group_p, positions_to_change, &rebuild_index);
        if (status != FBE_STATUS_OK)
        {
            fbe_transport_set_status(packet_p, status, 0);
            fbe_transport_complete_packet(packet_p);
            return FBE_STATUS_MORE_PROCESSING_REQUIRED; 
        }
        metadata_offset = (fbe_u64_t) 
            (&((fbe_raid_group_nonpaged_metadata_t*)0)->rebuild_info.rebuild_checkpoint_info[rebuild_index]);
    }


    fbe_raid_group_get_last_checkpoint_time(raid_group_p, &last_checkpoint_time);
    exported_per_disk_capacity = fbe_raid_group_get_exported_disk_capacity(raid_group_p);
    ms_since_checkpoint = fbe_get_elapsed_milliseconds(last_checkpoint_time);
    fbe_raid_group_class_get_peer_update_checkpoint_interval_ms(&peer_update_period_ms);
    if ((ms_since_checkpoint > peer_update_period_ms)             ||
        ((start_lba + block_count) == exported_per_disk_capacity)    )
    {
        /* Periodically we will set the checkpoint to the peer. 
         * A set is needed since the peer might be out of date with us and an increment 
         * will not suffice. 
         */
        status = fbe_base_config_metadata_nonpaged_force_set_checkpoint((fbe_base_config_t *)raid_group_p,
                                                                        packet_p,
                                                                        metadata_offset,
                                                                        second_metadata_offset,
                                                                        start_lba + block_count);
        fbe_raid_group_update_last_checkpoint_time(raid_group_p);
    }
    else
    {
        /* Increment checkpoint locally without sending to peer in order to not 
         * thrash the CMI. 
         */
        status = fbe_base_config_metadata_nonpaged_incr_checkpoint_no_peer((fbe_base_config_t *)raid_group_p,
                                                                           packet_p,
                                                                           metadata_offset,
                                                                           second_metadata_offset,
                                                                           start_lba,
                                                                           block_count);
    }
        
    //  Check the status of the call and trace if error.  The called function is completing the packet on error
    //  so we can't complete it here.  The condition will remain set so we will try again.
    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*) raid_group_p, FBE_TRACE_LEVEL_WARNING, 
            FBE_TRACE_MESSAGE_ID_INFO, "%s: error %d on nonpaged call\n", __FUNCTION__, status);
    }

    //  Return more processing since a packet is outstanding 
    return FBE_STATUS_MORE_PROCESSING_REQUIRED; 

} // End fbe_raid_group_incr_rebuild_checkpoint()

/*!****************************************************************************
 * fbe_raid_group_rebuild_determine_if_rebuild_needed
 ******************************************************************************
 * @brief
 *   This function will determine if any drive in the RG needs to be rebuilt.  
 *   A drive needs to be rebuilt if its checkpoint is not at the logical end
 *   marker.
 *
 *   Note this function does not check if the disk is rebuild logging.  The 
 *   calling function should make that check if needed. 
 *
 * @param in_raid_group_p           - pointer to the raid group
 * @param in_position               - disk position in the RG
 * @param out_requires_rebuild_b_p  - pointer to output that gets set to FBE_TRUE
 *                                    if the disk requires a rebuild 
 *
 * @return  fbe_status_t           
 *
 * @author
 *   12/07/2009 - Created. Jean Montes. 
 *
 ******************************************************************************/
fbe_status_t fbe_raid_group_rebuild_determine_if_rebuild_needed(
                                    fbe_raid_group_t*       in_raid_group_p,
                                    fbe_u32_t               in_position,
                                    fbe_bool_t*             out_requires_rebuild_b_p)
{
    
    fbe_lba_t                       checkpoint;             //  rebuild checkpoint
    

    //  Initialize the output parameter.  By default, a rebuild is not needed. 
    *out_requires_rebuild_b_p = FBE_FALSE;

    //  Get the rebuild checkpoint 
    fbe_raid_group_get_rebuild_checkpoint(in_raid_group_p, in_position, &checkpoint);

    //  If the rebuild checkpoint is not at the logical end marker, then this disk needs to be rebuilt.  Set 
    //  output parameter to true.
    if (checkpoint != FBE_LBA_INVALID)
    {   
        *out_requires_rebuild_b_p = FBE_TRUE;
    }

    //  Return success  
    return FBE_STATUS_OK;

} // End fbe_raid_group_rebuild_determine_if_rebuild_needed()


/*!****************************************************************************
 * fbe_raid_group_rebuild_process_set_checkpoint_to_end_marker()
 ******************************************************************************
 * @brief
 *   This function will set the rebuild checkpoint to end marker
 * 
 * @param packet_p
 * @param context
 * 
 * @return  fbe_status_t  
 *
 * @author
 *   10/26/2011 - Created. Ashwin Tamilarasan
 *
 ******************************************************************************/
fbe_status_t fbe_raid_group_rebuild_set_checkpoint_to_end_marker(fbe_packet_t * packet_p,                                    
                                                      fbe_packet_completion_context_t context)

{
    fbe_raid_group_t*          raid_group_p = NULL;   
    fbe_status_t               status;                 
    fbe_u32_t                  disk_index;
    
    raid_group_p = (fbe_raid_group_t*)context;

    /* If the status is not ok, that means we didn't get the 
       lock. Just return. we are already in the completion routine
    */
    status = fbe_transport_get_status_code (packet_p);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, 
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s can't get the NP lock, status: 0x%X", __FUNCTION__, status); 
        return FBE_STATUS_OK;
    }

    // Set the completion function to release the NP lock
    fbe_transport_set_completion_function(packet_p, fbe_raid_group_release_NP_lock, raid_group_p);

    fbe_raid_group_rebuild_find_disk_to_set_checkpoint_to_end_marker(raid_group_p, &disk_index);
    if (disk_index == FBE_RAID_INVALID_DISK_POSITION)
    {
        fbe_lifecycle_clear_current_cond((fbe_base_object_t*) raid_group_p);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }

    status = fbe_raid_group_rebuild_write_checkpoint(raid_group_p, packet_p, FBE_LBA_INVALID, 0, (1 << disk_index));
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
               
} // End fbe_raid_group_rebuild_process_set_checkpoint_to_end_marker()

/*!****************************************************************************
 * fbe_raid_group_rebuild_write_checkpoint()
 ******************************************************************************
 * @brief
 *   This function will issue a write to the non-paged metadata to set the rebuild 
 *   checkpoint to the new value for all the disk positions passed in.
 *
 *   When this function is called, it is assumed that the rebuild checkpoint is 
 *   not set to the logical end marker, ie. there is an entry for this rebuild 
 *   position in the rebuild info structure in the non-paged metadata. 
 *
 * @param in_raid_group_p           - pointer to a raid group object
 * @param in_packet_p               - pointer to a control packet from the scheduler
 * @param in_checkpoint             - new checkpoint 
 * @param in_positions_to_change    - bitmask indicating which disk position(s) to 
 *                                    change the checkpoint for 
 * 
 * @return  fbe_status_t  
 *
 * @author
 *   03/31/2010 - Created. Jean Montes. 
 *
 ******************************************************************************/
fbe_status_t fbe_raid_group_rebuild_write_checkpoint(
                                                fbe_raid_group_t*               in_raid_group_p,
                                                fbe_packet_t*                   in_packet_p,
                                                fbe_lba_t                       in_start_lba,
                                                fbe_block_count_t               in_block_count,
                                                fbe_raid_position_bitmask_t     in_positions_to_change) 
{
    fbe_status_t                                status;                         
    fbe_u32_t                                   number_of_positions;            
    fbe_raid_group_rebuild_nonpaged_info_t      rebuild_nonpaged_data;          
    fbe_u32_t                                   rebuild_index;                  
    fbe_raid_group_rebuild_checkpoint_info_t*   rebuild_checkpoint_start_p;     
    fbe_u64_t                                   metadata_offset;                
    fbe_u32_t                                   metadata_write_size;            
    fbe_lba_t                                   new_checkpoint;
    fbe_lba_t                                   exported_disk_capacity;


    new_checkpoint = in_start_lba + in_block_count;
    
    //  Populate a non-paged rebuild data structure with the rebuild checkpoint info to write out 
    status = fbe_raid_group_rebuild_create_checkpoint_data(in_raid_group_p, new_checkpoint, in_positions_to_change, 
        &rebuild_nonpaged_data, &rebuild_index); 
 
    //  Check for an error with setting up the data and if so, return it without issuing the metadata write
    if (status != FBE_STATUS_OK)
    {
        fbe_transport_set_status(in_packet_p, status, 0);
        fbe_transport_complete_packet(in_packet_p);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED; 
    }

    //  Determine the metadata offset in the non-paged metadata to start the write to, and the offset in the  
    //  rebuild info structure to start the write from.  
    // 
    //  If there are 2 positions being changed, always start at rebuild checkpoint info entry 0.  Otherwise, 
    //  start at the entry of the specific rebuild position to be changed. 
    number_of_positions = fbe_raid_get_bitcount((fbe_u32_t) in_positions_to_change);
    if (number_of_positions == FBE_RAID_GROUP_MAX_REBUILD_POSITIONS)
    {
        fbe_base_object_trace((fbe_base_object_t*) in_raid_group_p, FBE_TRACE_LEVEL_ERROR, 
            FBE_TRACE_MESSAGE_ID_INFO, "%s: We cannot set 2 positions to end marker.\n", __FUNCTION__);

        rebuild_checkpoint_start_p = &rebuild_nonpaged_data.rebuild_checkpoint_info[0];

        metadata_offset = (fbe_u64_t) 
            (&((fbe_raid_group_nonpaged_metadata_t*)0)->rebuild_info.rebuild_checkpoint_info[0]);
    }
    else
    {
        rebuild_checkpoint_start_p = &rebuild_nonpaged_data.rebuild_checkpoint_info[rebuild_index];

        metadata_offset = (fbe_u64_t) 
            (&((fbe_raid_group_nonpaged_metadata_t*)0)->rebuild_info.rebuild_checkpoint_info[rebuild_index]);
    }

    /* If enabled trace the rebuild checkpoint update.
     */
    fbe_raid_group_trace(in_raid_group_p, FBE_TRACE_LEVEL_INFO, 
                         FBE_RAID_GROUP_DEBUG_FLAG_REBUILD_TRACING,
                         "raid_group: new checkpoint: 0x%llx for request: 0x%llx blks: 0x%llx pos_mask: 0x%x \n",
                         (unsigned long long)new_checkpoint,
                         (unsigned long long)in_start_lba, 
                         (unsigned long long)in_block_count,
                         in_positions_to_change);

    //  Set the metadata write size depending on how many positions we are writing
    metadata_write_size = sizeof(fbe_raid_group_rebuild_checkpoint_info_t) * number_of_positions; 
            
    exported_disk_capacity = fbe_raid_group_get_exported_disk_capacity(in_raid_group_p);
    if( new_checkpoint > exported_disk_capacity)
    {
        if (number_of_positions == FBE_RAID_GROUP_MAX_REBUILD_POSITIONS)
        {
            rebuild_nonpaged_data.rebuild_checkpoint_info[0].checkpoint = FBE_LBA_INVALID;
            rebuild_nonpaged_data.rebuild_checkpoint_info[0].position = FBE_RAID_INVALID_DISK_POSITION;
            rebuild_nonpaged_data.rebuild_checkpoint_info[1].checkpoint = FBE_LBA_INVALID;
            rebuild_nonpaged_data.rebuild_checkpoint_info[1].position = FBE_RAID_INVALID_DISK_POSITION;
            rebuild_checkpoint_start_p = &rebuild_nonpaged_data.rebuild_checkpoint_info[0];
        }
        else
        {
            rebuild_checkpoint_start_p->checkpoint = FBE_LBA_INVALID;
            rebuild_checkpoint_start_p->position = FBE_RAID_INVALID_DISK_POSITION;
        }
            
        status = fbe_base_config_metadata_nonpaged_write_persist((fbe_base_config_t*) in_raid_group_p, in_packet_p, 
                                                                 metadata_offset, 
                                                                 (fbe_u8_t*) rebuild_checkpoint_start_p,
                                                                 metadata_write_size);

        //  Return more processing since a packet is outstanding 
        return FBE_STATUS_MORE_PROCESSING_REQUIRED; 
    }
    else
    {
        fbe_transport_set_status(in_packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(in_packet_p);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }
} // End fbe_raid_group_rebuild_write_checkpoint()

/*!****************************************************************************
 * fbe_raid_group_rebuild_create_checkpoint_data()
 ******************************************************************************
 * @brief 
 *   This function populates a rebuild nonpaged info structure with the data 
 *   needed to modify checkpoints for the position(s) indicated.  This 
 *   structure can then be used by the caller in a non-paged metadata write. 
 *
 * @param in_raid_group_p           - pointer to the raid group object 
 * @param in_checkpoint             - new checkpoint 
 * @param in_positions_to_change    - bitmask indicating which disk position(s)  
 *                                    to change the checkpoint for 
 * @param out_rebuild_data_p        - pointer to rebuild nonpaged data structure 
 *                                    that gets filled in by this function
 * @param out_rebuild_data_index_p  - pointer to data that gets set to the index
 *                                    in the rebuild data to start the metadata 
 *                                    write from                                    
 *
 * @return fbe_status_t         
 *
 * @author
 *   11/30/2010 - Created. Jean Montes. 
 *
 ******************************************************************************/
fbe_status_t fbe_raid_group_rebuild_create_checkpoint_data(
                                        fbe_raid_group_t*                       in_raid_group_p,
                                        fbe_lba_t                               in_checkpoint,
                                        fbe_raid_position_bitmask_t             in_positions_to_change,
                                        fbe_raid_group_rebuild_nonpaged_info_t* out_rebuild_data_p,
                                        fbe_u32_t*                              out_rebuild_data_index_p)

{
    fbe_raid_group_nonpaged_metadata_t* nonpaged_metadata_p;                    // pointer to nonpaged metadata 
    fbe_u32_t                           width;                                  // raid group width
    fbe_u32_t                           disk_position;                          // current disk position  
    fbe_u32_t                           rebuild_index;                          // index into rebuild info struct 
    fbe_raid_position_bitmask_t         cur_position_mask;                      // mask of current disk position


    //  Get a pointer to the non-paged metadata 
    fbe_base_config_get_nonpaged_metadata_ptr((fbe_base_config_t*) in_raid_group_p, (void **)&nonpaged_metadata_p);

    //  Get the width of raid group so we can loop through the disks 
    fbe_base_config_get_width((fbe_base_config_t*) in_raid_group_p, &width);

    //  Find the disk(s) indicated by the positions to change mask
    for (disk_position = 0; disk_position < width; disk_position++)
    {
        //  Create a bitmask for this position
        cur_position_mask = 1 << disk_position; 

        //  If this disk position does not need its checkpoint changed, the loop can move on 
        if ((in_positions_to_change & cur_position_mask) == 0)
        {
            continue;
        }

        //  Find the entry in the non-paged MD's rebuild checkpoint info corresponding to this position 
        fbe_raid_group_rebuild_find_existing_checkpoint_entry_for_position(in_raid_group_p, disk_position, 
            &rebuild_index);

        //  If no entry was found, then we have a problem - return error
        if (rebuild_index == FBE_RAID_GROUP_INVALID_INDEX)
        {
            fbe_base_object_trace((fbe_base_object_t*) in_raid_group_p, FBE_TRACE_LEVEL_ERROR, 
                    FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, "%s no rebuild checkpoint entry found\n", __FUNCTION__);
            return FBE_STATUS_GENERIC_FAILURE; 
        }

        //  We have a match.  Set the checkpoint value in the output structure. 
        out_rebuild_data_p->rebuild_checkpoint_info[rebuild_index].checkpoint = in_checkpoint;

        //  Set the position in the output structure.  If we are setting the checkpoint to the end marker, ie. the 
        //  rebuild is done, then we need to clear the position value.  
        if (in_checkpoint == FBE_LBA_INVALID)
        {
            out_rebuild_data_p->rebuild_checkpoint_info[rebuild_index].position = FBE_RAID_INVALID_DISK_POSITION;
        }
        else
        {
            out_rebuild_data_p->rebuild_checkpoint_info[rebuild_index].position = disk_position; 
        }

        //  Set the rebuild index in the output parameter
        *out_rebuild_data_index_p = rebuild_index; 
    }

    //  Return success   
    return FBE_STATUS_OK;

} // End fbe_raid_group_rebuild_create_checkpoint_data()

/*!****************************************************************************
 * fbe_raid_group_rebuild_force_nr_write_rebuild_checkpoint_info()
 ******************************************************************************
 * @brief
 *   This function will issue a write to the non-paged metadata to set the rebuild 
 *   checkpoint to the new value for all the disk positions passed in.
 *  
 *  This function does not perform any of the sanity checks that fbe_raid_group_rebuild_write_checkpoint
 *  is doing and the caller knows exactly what is being done(mostly in engineering and debug mode)
 * 
 * @param in_raid_group_p           - pointer to a raid group object
 * @param in_packet_p               - pointer to a control packet from the scheduler
 * @param in_start_lba              - Start LBA to mark the NR
 * @param in_block_count            - Block count
 * @param in_positions_to_change    - bitmask indicating which disk position(s) to 
 *                                    change the checkpoint for 
 * 
 * @return  fbe_status_t  
 *
 * @author
 *   06/10/2012 - Created. Ashok Tamilarasan
 *
 ******************************************************************************/
fbe_status_t fbe_raid_group_rebuild_force_nr_write_rebuild_checkpoint_info(fbe_raid_group_t* in_raid_group_p,
                                                                           fbe_packet_t* in_packet_p,
                                                                           fbe_lba_t  in_start_lba,
                                                                           fbe_block_count_t in_block_count,
                                                                           fbe_raid_position_bitmask_t  in_positions_to_change) 
{
    fbe_status_t                                status;                         
    fbe_raid_group_rebuild_nonpaged_info_t      rebuild_nonpaged_data;          
    fbe_u32_t                                   rebuild_index;                  
    fbe_raid_group_rebuild_checkpoint_info_t*   rebuild_checkpoint_start_p;     
    fbe_u64_t                                   metadata_offset;                
    fbe_lba_t                                   new_checkpoint;
    
    new_checkpoint = in_start_lba + in_block_count;
    
    status = fbe_raid_group_rebuild_find_next_unused_checkpoint_entry(in_raid_group_p, 0, &rebuild_index); 
 
    //  Check for an error with setting up the data and if so, return it without issuing the metadata write
    if ((status != FBE_STATUS_OK) || (rebuild_index == FBE_RAID_GROUP_INVALID_INDEX))
    {
        fbe_raid_group_trace(in_raid_group_p, 
                         FBE_TRACE_LEVEL_INFO, 
                         FBE_TRACE_MESSAGE_ID_INFO,
                         "raid_group_force_nr: Error finding unused entry. status:%d, index:0x%x \n",
                         status, rebuild_index);
        fbe_transport_set_status(in_packet_p, status, 0);
        fbe_transport_complete_packet(in_packet_p);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED; 
    }

    rebuild_checkpoint_start_p = &rebuild_nonpaged_data.rebuild_checkpoint_info[rebuild_index];

    metadata_offset = (fbe_u64_t) 
            (&((fbe_raid_group_nonpaged_metadata_t*)0)->rebuild_info.rebuild_checkpoint_info[rebuild_index]);
    
    /* If enabled trace the rebuild checkpoint update.
     */
    fbe_raid_group_trace(in_raid_group_p, 
                         FBE_TRACE_LEVEL_INFO, 
                         FBE_RAID_GROUP_DEBUG_FLAGS_TRACE_PMD_NR,
                         "raid_group: force nr start: 0x%llx blks: 0x%llx new chkpt: 0x%llx pos_mask: 0x%x \n",
                         (unsigned long long)in_start_lba, (unsigned long long)in_block_count, (unsigned long long)new_checkpoint, in_positions_to_change);

            
    rebuild_checkpoint_start_p->checkpoint = new_checkpoint;
    rebuild_checkpoint_start_p->position = fbe_raid_get_first_set_position(in_positions_to_change);
    
    status = fbe_base_config_metadata_nonpaged_write_persist((fbe_base_config_t*) in_raid_group_p, in_packet_p, 
                                                              metadata_offset, 
                                                              (fbe_u8_t*) rebuild_checkpoint_start_p,
                                                              sizeof(fbe_raid_group_rebuild_checkpoint_info_t));

    //  Return more processing since a packet is outstanding 
    return FBE_STATUS_MORE_PROCESSING_REQUIRED; 

} // End fbe_raid_group_rebuild_force_nr_write_rebuild_checkpoint_info()


/*!****************************************************************************
 * fbe_raid_group_rebuild_find_disk_to_set_checkpoint_to_end_marker()
 ******************************************************************************
 * @brief
 *   This function finds the first disk/edge that needs its checkpoint moved
 *   to the logical end marker.  The checkpoint needs to be moved when the 
 *   rebuild is finished, ie. its checkpoint is at capacity + 1, and it is 
 *   not rebuild logging. 
 *
 *   If the function does not find a disk/edge, it sets the output parameter to 
 *   FBE_RAID_INVALID_DISK_POSITION.
 *
 * @param in_raid_group_p       - pointer to a raid group object
 * @param out_disk_index_p      - pointer which gets set to the index of the 
 *                                edge to move the checkpoint for; set to 
 *                                FBE_RAID_INVALID_DISK_POSITION if none found
 *
 * @return fbe_status_t         
 *
 * @author
 *   03/25/2010 - Created. Jean Montes.
 *
 *****************************************************************************/
fbe_status_t fbe_raid_group_rebuild_find_disk_to_set_checkpoint_to_end_marker(
                                    fbe_raid_group_t*           in_raid_group_p,
                                    fbe_u32_t*                  out_disk_index_p)
{

    fbe_u32_t                       width;                      // raid group width
    fbe_u32_t                       position_index;             // current disk position 
    fbe_lba_t                       checkpoint;                 // rebuild checkpoint
    fbe_lba_t                       rebuild_metadata_lba;       // rebuild lba for metadata region
    fbe_lba_t                       exported_disk_capacity;     // exported disk capacity
    fbe_bool_t                      b_is_rebuild_logging;
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_group_get_raid_geometry(in_raid_group_p);
    fbe_lba_t journal_log_end_lba;

    //  Trace function entry 
    FBE_RAID_GROUP_TRACE_FUNC_ENTRY(in_raid_group_p);
    fbe_raid_geometry_get_journal_log_end_lba(raid_geometry_p, &journal_log_end_lba);

    //  Initialize output parameter to no disk found 
    *out_disk_index_p = FBE_RAID_INVALID_DISK_POSITION;

    //  Get the width of raid group
    fbe_base_config_get_width((fbe_base_config_t*) in_raid_group_p, &width);

    //  Get the capacity per disk, which is the disk ending LBA + 1. 
    exported_disk_capacity = fbe_raid_group_get_exported_disk_capacity(in_raid_group_p);

    //  Loop through all of the disks in the raid group   
    for (position_index = 0; position_index < width; position_index++)
    {
        //  If the checkpoint is already set to the logical end, move on to the next disk
        fbe_raid_group_get_rebuild_checkpoint(in_raid_group_p, position_index, &checkpoint); 
        if (checkpoint == FBE_LBA_INVALID)
        {
            continue; 
        }

        /* Rebuild for the exported user area is complete when the checkpoint is equal to exported
         * capacity per drive, ie. drive ending LBA + 1. 
         */
        if (checkpoint < exported_disk_capacity)
        {
            continue;
        }

        //  Get the next metadata rebuild lba which we need to rebuid
        fbe_raid_group_rebuild_get_next_metadata_needs_rebuild_lba(in_raid_group_p,
            position_index, &rebuild_metadata_lba);

        //  Metadata Rebuild is completed if there is no chunk in metadata region needs to rebuild
        if(rebuild_metadata_lba != FBE_LBA_INVALID)
        {
            continue;
        }

        //  If the edge is rb logging, then we shouldn't be moving the checkpoint.  Move on to the next disk.
        fbe_raid_group_get_rb_logging(in_raid_group_p, position_index, &b_is_rebuild_logging); 
        if (b_is_rebuild_logging == FBE_TRUE)
        {
            continue; 
        }
        fbe_base_object_trace((fbe_base_object_t*) in_raid_group_p, FBE_TRACE_LEVEL_INFO, 
                              FBE_TRACE_MESSAGE_ID_INFO, "raid_group: set chkpt end marker pos %d chkpt: 0x%llx\n", 
                              position_index, (unsigned long long)checkpoint);
        //  We have found an disk whose checkpoint needs to be moved.  Set the output and return. 
        *out_disk_index_p = position_index; 
        return FBE_STATUS_OK; 
    } 

    //  Return success.  The output parameter has already been initialized to no disk found. 
    return FBE_STATUS_OK;

} // End fbe_raid_group_rebuild_find_disk_to_set_checkpoint_to_end_marker()


/*!****************************************************************************
 * fbe_raid_group_rebuild_check_permission_based_on_load 
 ******************************************************************************
 * @brief
 *   This function will check if a rebuild I/O is allowed based on the current
 *   I/O load and scheduler credits available for it.
 *
 * @param raid_group_p           - pointer to the raid group
 * @param ok_to_proceed_b_p     - pointer to output that gets set to FBE_TRUE
 *                                    if the operation can proceed 
 *
 * @return  fbe_status_t           
 *
 * @author
 *   05/06/2010 - Created. Jean Montes. 
 *
 ******************************************************************************/
fbe_status_t 
fbe_raid_group_rebuild_check_permission_based_on_load(fbe_raid_group_t *raid_group_p,
                                                      fbe_bool_t *ok_to_proceed_b_p)
{
    fbe_base_config_operation_permission_t      permission_data;
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_group_get_raid_geometry(raid_group_p);

    *ok_to_proceed_b_p = FBE_FALSE;

    permission_data.operation_priority = fbe_raid_group_get_rebuild_priority(raid_group_p);

    permission_data.credit_requests.cpu_operations_per_second = FBE_RAID_GROUP_REBUILD_CPUS_PER_SECOND;
    permission_data.credit_requests.mega_bytes_consumption    = FBE_RAID_GROUP_REBUILD_MBYTES_CONSUMPTION;

    /* IO credits is width since we know the rebuild will touch the entire width typically.
     */
    permission_data.io_credits = raid_geometry_p->width;

    fbe_base_config_get_operation_permission((fbe_base_config_t*) raid_group_p, &permission_data, FBE_TRUE,
                                             ok_to_proceed_b_p);

    if(*ok_to_proceed_b_p == FBE_FALSE){
        fbe_lifecycle_reschedule(&fbe_raid_group_lifecycle_const, (fbe_base_object_t*) raid_group_p, 500);
    }

    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_raid_group_rebuild_check_permission_based_on_load()
 **************************************/ 


/*!****************************************************************************
 *  fbe_raid_group_rebuild_get_nonpaged_info()
 ******************************************************************************
 * @brief 
 *   Get the rebuild nonpaged information.  The rebuild nonpaged info consists
 *   of the rb logging bitmask and the two rebuild checkpoint/position entries.
 *
 * @param in_raid_group_p           - pointer to the raid group object 
 * @param out_rebuild_data_p        - pointer to rebuild nonpaged data structure 
 *                                    that gets filled in by this function
 *
 * @return fbe_status_t        
 *
 * @author
 *   11/30/2010 - Created. Jean Montes. 
 *
 ******************************************************************************/
fbe_status_t fbe_raid_group_rebuild_get_nonpaged_info(
                                        fbe_raid_group_t*                       in_raid_group_p,
                                        fbe_raid_group_rebuild_nonpaged_info_t* out_rebuild_data_p)

{
    fbe_raid_group_nonpaged_metadata_t* nonpaged_metadata_p;                    // pointer to all of nonpaged metadata 
    fbe_u32_t                           rebuild_index;                          // index into rebuild info struct 


    //  If the non-paged metadata is not initialized yet, then return the default values with an error status
    if (fbe_raid_group_is_nonpaged_metadata_initialized(in_raid_group_p) == FBE_FALSE)
    {
        out_rebuild_data_p->rebuild_logging_bitmask = 0;

        for (rebuild_index = 0; rebuild_index < FBE_RAID_GROUP_MAX_REBUILD_POSITIONS; rebuild_index++)
        {
            out_rebuild_data_p->rebuild_checkpoint_info[rebuild_index].checkpoint = FBE_LBA_INVALID;
            out_rebuild_data_p->rebuild_checkpoint_info[rebuild_index].position   = FBE_RAID_INVALID_DISK_POSITION;
        }
        return FBE_STATUS_NOT_INITIALIZED;
    }

    //  Get a pointer to the non-paged metadata 
    fbe_base_config_get_nonpaged_metadata_ptr((fbe_base_config_t*) in_raid_group_p, (void **)&nonpaged_metadata_p);

    //  Copy the non-paged metadata into the output structure
    fbe_copy_memory(out_rebuild_data_p, &nonpaged_metadata_p->rebuild_info, 
        sizeof(fbe_raid_group_rebuild_nonpaged_info_t));

    //  Return success 
    return FBE_STATUS_OK;

} // End fbe_raid_group_rebuild_get_nonpaged_info()


/*!****************************************************************************
 * fbe_raid_group_rebuild_find_existing_checkpoint_entry_for_position()
 ******************************************************************************
 * @brief 
 *   This function will loop through the rebuild checkpoint info entries in the
 *   non-paged metadata to see if an entry exists for the given position.  If
 *   one is found, the index of the entry will be returned in the output parameter.
 *
 *   Otherwise, the output will be set to FBE_RAID_GROUP_INVALID_INDEX.  This will
 *   be the case when the disk position is not rebuilding or rb logging,
 *   ie. in the "normal" situation.
 *  
 * @param in_raid_group_p           - pointer to the raid group object 
 * @param in_position               - disk position in the RG 
 * @param out_rebuild_data_index_p  - pointer to data that gets set to the index
 *                                    in the rebuild checkpoint data of the entry 
 *                                    for this position; FBE_RAID_GROUP_INVALID_INDEX
 *                                    if not found 
 *                                 
 *
 * @return fbe_status_t         
 *
 * @author
 *   11/30/2010 - Created. Jean Montes. 
 *
 ******************************************************************************/
fbe_status_t fbe_raid_group_rebuild_find_existing_checkpoint_entry_for_position(
                                        fbe_raid_group_t*                       in_raid_group_p,
                                        fbe_u32_t                               in_position,
                                        fbe_u32_t*                              out_rebuild_data_index_p)

{
    fbe_raid_group_nonpaged_metadata_t* nonpaged_metadata_p;                    // pointer to nonpaged metadata 
    fbe_u32_t                           index;                                  // index into rebuild info struct 


    //  By default, the rebuild checkpoint data index will be invalid 
    *out_rebuild_data_index_p = FBE_RAID_GROUP_INVALID_INDEX;

    //  If the non-paged metadata is not initialized yet, then return the default value with a status indicating 
    //  such 
    if (fbe_raid_group_is_nonpaged_metadata_initialized(in_raid_group_p) == FBE_FALSE)
    {
        return FBE_STATUS_NOT_INITIALIZED;
    }

    //  Get a pointer to the non-paged metadata 
    fbe_base_config_get_nonpaged_metadata_ptr((fbe_base_config_t*) in_raid_group_p, (void **)&nonpaged_metadata_p);

    //  Loop through the rebuild checkpoints to see if the checkpoint is set for this disk position
    for (index = 0; index < FBE_RAID_GROUP_MAX_REBUILD_POSITIONS; index++)
    {
        if (nonpaged_metadata_p->rebuild_info.rebuild_checkpoint_info[index].position == in_position)
        {
            *out_rebuild_data_index_p = index;
            break; 
        }
    }

    //  Return success.  The output parameter has already been set. 
    return FBE_STATUS_OK;

} // End fbe_raid_group_rebuild_find_existing_checkpoint_entry_for_position()


/*!****************************************************************************
 * fbe_raid_group_rebuild_find_existing_position_entry_for_checkpoint()
 ******************************************************************************
 * @brief 
 *   This function will loop through the rebuild checkpoint info entries in the
 *   non-paged metadata to see if an entry exists for the given position.  If
 *   one is found, the index of the entry will be returned in the output parameter.
 *
 *   Otherwise, the output will be set to FBE_RAID_GROUP_INVALID_INDEX.  This will
 *   be the case when the disk position is not rebuilding or rb logging,
 *   ie. in the "normal" situation.
 *  
 * @param in_raid_group_p           - pointer to the raid group object 
 * @param in_position               - disk position in the RG 
 * @param out_rebuild_data_index_p  - pointer to data that gets set to the index
 *                                    in the rebuild checkpoint data of the entry 
 *                                    for this position; FBE_RAID_GROUP_INVALID_INDEX
 *                                    if not found 
 *                                 
 *
 * @return fbe_status_t         
 *
 * @author
 *   11/30/2010 - Created. Jean Montes. 
 *
 ******************************************************************************/
fbe_status_t fbe_raid_group_rebuild_find_existing_position_entry_for_checkpoint(
                                        fbe_raid_group_t*                       in_raid_group_p,
                                        fbe_lba_t                               in_checkpoint,
                                        fbe_u32_t*                              out_position_p)

{
    fbe_raid_group_nonpaged_metadata_t* nonpaged_metadata_p;                    // pointer to nonpaged metadata 
    fbe_u32_t                           index;                                  // index into rebuild info struct 


    //  By default, the rebuild checkpoint data index will be invalid 
    *out_position_p = FBE_RAID_INVALID_DISK_POSITION;

    //  If the non-paged metadata is not initialized yet, then return the default value with a status indicating 
    //  such 
    if (fbe_raid_group_is_nonpaged_metadata_initialized(in_raid_group_p) == FBE_FALSE)
    {
        return FBE_STATUS_NOT_INITIALIZED;
    }

    //  Get a pointer to the non-paged metadata 
    fbe_base_config_get_nonpaged_metadata_ptr((fbe_base_config_t*) in_raid_group_p, (void **)&nonpaged_metadata_p);

    //  Loop through the rebuild checkpoints to see if the checkpoint is set for this disk position
    for (index = 0; index < FBE_RAID_GROUP_MAX_REBUILD_POSITIONS; index++)
    {
        if (nonpaged_metadata_p->rebuild_info.rebuild_checkpoint_info[index].checkpoint == in_checkpoint)
        {
            *out_position_p = nonpaged_metadata_p->rebuild_info.rebuild_checkpoint_info[index].position;
            break; 
        }
    }

    //  Return success.  The output parameter has already been set. 
    return FBE_STATUS_OK;

} // End fbe_raid_group_rebuild_find_existing_position_entry_for_checkpoint()

/*!****************************************************************************
 * fbe_raid_group_rebuild_find_next_unused_checkpoint_entry
 ******************************************************************************
 * @brief 
 *   This function will loop through the rebuild checkpoint info entries in the
 *   non-paged metadata to find the next unused entry after the index that is
 *   passed in.  If one is found, its index will be returned in the output parameter.  
 *   Otherwise, the output will be set to FBE_RAID_GROUP_INVALID_INDEX.  
 *  
 * @param in_raid_group_p           - pointer to the raid group object 
 * @param in_start_index            - index of the rebuild checkpoint entries to 
 *                                    start the search from 
 * @param out_rebuild_data_index_p  - pointer to data that gets set to the index
 *                                    of the first open rebuild checkpoint entry; 
 *                                    FBE_RAID_GROUP_INVALID_INDEX if none is 
 *                                    available 
 *
 * @return fbe_status_t         
 *
 * @author
 *   11/30/2010 - Created. Jean Montes. 
 *
 ******************************************************************************/
fbe_status_t fbe_raid_group_rebuild_find_next_unused_checkpoint_entry(
                                        fbe_raid_group_t*                       in_raid_group_p,
                                        fbe_u32_t                               in_start_index,
                                        fbe_u32_t*                              out_rebuild_data_index_p)

{
    fbe_raid_group_nonpaged_metadata_t* nonpaged_metadata_p;                    // pointer to nonpaged metadata 
    fbe_u32_t                           index;                                  // index into rebuild info struct 


    //  By default, the rebuild checkpoint data index will be invalid 
    *out_rebuild_data_index_p = FBE_RAID_GROUP_INVALID_INDEX;

    //  If the start index is too large, return an error 
    if (in_start_index >= FBE_RAID_GROUP_MAX_REBUILD_POSITIONS)
    {
        fbe_base_object_trace((fbe_base_object_t*) in_raid_group_p, FBE_TRACE_LEVEL_ERROR, 
            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, "%s no rebuild checkpoint entry found\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    //  If the non-paged metadata is not initialized yet, then return the default value with an error.  Otherwise
    //  the checkpoint written now would be overwritten when the non-paged is initialized. 
    if (fbe_raid_group_is_nonpaged_metadata_initialized(in_raid_group_p) == FBE_FALSE)
    {
        fbe_base_object_trace((fbe_base_object_t*) in_raid_group_p, FBE_TRACE_LEVEL_ERROR, 
            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, "%s nonpaged MD not initialized\n", __FUNCTION__);
        return FBE_STATUS_NOT_INITIALIZED;
    }

    //  Get a pointer to the non-paged metadata 
    fbe_base_config_get_nonpaged_metadata_ptr((fbe_base_config_t*) in_raid_group_p, (void **)&nonpaged_metadata_p);

    //  Loop through the rebuild checkpoints to see if the checkpoint is set for this disk position
    for (index = in_start_index; index < FBE_RAID_GROUP_MAX_REBUILD_POSITIONS; index++)
    {
        if (nonpaged_metadata_p->rebuild_info.rebuild_checkpoint_info[index].position == 
            FBE_RAID_INVALID_DISK_POSITION)
        {
            *out_rebuild_data_index_p = index;
            break; 
        }
    }

    //  Return success.  The output parameter has already been set. 
    return FBE_STATUS_OK;

} // End fbe_raid_group_rebuild_find_next_unused_checkpoint_entry()



/*!****************************************************************************
 * fbe_raid_group_rebuild_get_chunk_info()
 ******************************************************************************
 * @brief
 *   This function gets the NR chunk info for the bitmap chunk at the specified 
 *   LBA.  
 *
 * @param in_raid_group_p           - pointer to the raid group
 * @param in_packet_p               - pointer to a control packet from the scheduler
 *
 * @return  fbe_status_t            
 *
 * Note: When this function is called, the LBA that is passed in is the rebuild 
 * checkpoint.  At this point, it has already been evaluated and it is known that 
 * it is not set to the logical end marker or the capacity of the disk.  
 *
 * @author
 *   04/28/2010 - Created. Jean Montes. 
 *
 ******************************************************************************/
fbe_status_t fbe_raid_group_rebuild_get_chunk_info(
                                        fbe_raid_group_t*                   in_raid_group_p,
                                        fbe_packet_t*                       in_packet_p)
{

    fbe_chunk_index_t                   chunk_index;               //  chunk index in the bitmap of chunk at this LBA
    fbe_chunk_count_t                   chunk_count;               //  chunk count for the next nr chunk rage
    fbe_payload_ex_t*                  sep_payload_p = NULL;      // pointer to the sep payload
    fbe_raid_iots_t *                   iots_p = NULL;
    fbe_payload_control_operation_t*    control_operation_p = NULL; // pointer to the control operation
    fbe_raid_group_rebuild_context_t*   rebuild_context_p = NULL;   // pointer to the rebuild context
    
    //  Get a pointer to iots from the packet
    sep_payload_p = fbe_transport_get_payload_ex(in_packet_p);
    fbe_payload_ex_get_iots(sep_payload_p, &iots_p);
    control_operation_p = fbe_payload_ex_get_present_control_operation(sep_payload_p);
    fbe_payload_control_get_buffer(control_operation_p, &rebuild_context_p);

    //  Get the chunk range for which we need to perform the rebuild operation
    fbe_raid_group_bitmap_get_chunk_range(in_raid_group_p, iots_p->lba,
                                          iots_p->blocks, &chunk_index, &chunk_count);

    /* If enabled trace some information.
     */
    fbe_raid_group_trace(in_raid_group_p, 
                         FBE_TRACE_LEVEL_INFO,
                         FBE_RAID_GROUP_DEBUG_FLAG_REBUILD_TRACING,
                         "raid_group: get chunk info lba: 0x%llx blks: 0x%llx chunk_index: 0x%llx chunk_count: 0x%x chunk_size: 0x%x\n",
                         (unsigned long long)iots_p->lba, 
             (unsigned long long)iots_p->blocks,
             (unsigned long long)chunk_index, chunk_count, in_raid_group_p->chunk_size);

    //  Set the rebuild operation state to preread metadata
    fbe_raid_group_rebuild_context_set_rebuild_state(rebuild_context_p, FBE_RAID_GROUP_REBUILD_STATE_PREREAD_METADATA);

    //  Set the completion function 
    fbe_transport_set_completion_function(in_packet_p, fbe_raid_group_rebuild_get_chunk_info_completion,
                                          in_raid_group_p);

    //  Get the chunk info for a single chunk 
    fbe_raid_group_rebuild_get_next_marked_chunk(in_raid_group_p, in_packet_p, chunk_index, chunk_count,
                                                 &(rebuild_context_p->rebuild_bitmask));

    //  Return success
    return FBE_STATUS_MORE_PROCESSING_REQUIRED; 

}   // End fbe_raid_group_rebuild_get_chunk_info()

/*!****************************************************************************
 *          fbe_raid_group_rebuild_trace_chunk_info()
 ******************************************************************************
 *
 * @brief   Display the `needs rebuild' chunk information.
 *   
 * @param   raid_group_p - Pointer to raid to display information for.
 * @param   chunk_info_p - Pointer to chunk information to display.
 * @param   chunk_count - Number of chunks to display.
 * @param   iots_p - current I/O ptr.
 *
 * @return  None         
 *
 ******************************************************************************/
void fbe_raid_group_rebuild_trace_chunk_info(fbe_raid_group_t *raid_group_p,
                                             fbe_raid_group_paged_metadata_t *chunk_info_p,
                                             fbe_chunk_count_t chunk_count,
                                             fbe_raid_iots_t *iots_p)
{
    /* Display up to (10) entries at (5) per line.
     */
    if (chunk_count > 0)
    {
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "iots: %x op: %d %s: 0x%x  %s: 0x%x  %s: 0x%x  %s: 0x%x  %s: 0x%x\n",
                              (fbe_u32_t)iots_p, iots_p->current_opcode,
                              (chunk_count > 0) ? "NR[0]" : " n/a ",
                              (chunk_count > 0) ? chunk_info_p[0].needs_rebuild_bits : 0,
                              (chunk_count > 1) ? "NR[1]" : " n/a ",
                              (chunk_count > 1) ? chunk_info_p[1].needs_rebuild_bits : 0,
                              (chunk_count > 2) ? "NR[2]" : " n/a ",
                              (chunk_count > 2) ? chunk_info_p[2].needs_rebuild_bits : 0,
                              (chunk_count > 3) ? "NR[3]" : " n/a ",
                              (chunk_count > 3) ? chunk_info_p[3].needs_rebuild_bits : 0,
                              (chunk_count > 4) ? "NR[4]" : " n/a ",
                              (chunk_count > 4) ? chunk_info_p[4].needs_rebuild_bits : 0);
        if (chunk_count > 5)
        {
            fbe_base_object_trace((fbe_base_object_t *)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                  "iots: %x op: %d %s: 0x%x  %s: 0x%x  %s: 0x%x  %s: 0x%x  %s: 0x%x\n",
                                  (fbe_u32_t)iots_p, iots_p->current_opcode,
                                  (chunk_count > 5) ? "NR[5]" : " n/a ",
                                  (chunk_count > 5) ? chunk_info_p[5].needs_rebuild_bits : 0,
                                  (chunk_count > 6) ? "NR[6]" : " n/a ",
                                  (chunk_count > 6) ? chunk_info_p[6].needs_rebuild_bits : 0,
                                  (chunk_count > 7) ? "NR[7]" : " n/a ",
                                  (chunk_count > 7) ? chunk_info_p[7].needs_rebuild_bits : 0,
                                  (chunk_count > 8) ? "NR[8]" : " n/a ",
                                  (chunk_count > 8) ? chunk_info_p[8].needs_rebuild_bits : 0,
                                  (chunk_count > 9) ? "NR[9]" : " n/a ",
                                  (chunk_count > 9) ? chunk_info_p[9].needs_rebuild_bits : 0);
        }
    }

    return;
}
/* end of fbe_raid_group_rebuild_trace_chunk_info() */

/*!***************************************************************************
 *          fbe_raid_group_rebuild_handle_chunks_without_needs_rebuild()
 *****************************************************************************
 *
 * @brief   This method is called during a rebuild when we encounter a chunk
 *          that does not need to be rebuilt (for any position).  There is
 *          only (2) possible outcomes:
 *              1) This is the last iots of the rebuild request and NR is
 *                 is 0 for ALL the chunks in the iots.
 *                                  OR
 *              2) This is not the last iots of the rebuild request or there
 *                 is at least (1) chunk with NR set.
 *
 *          Case 1):
 *              Set the iots completion.  Invoke method to update the rebuild
 *              checkpoint to the end of the entire request.
 *
 *          Case 2):
 *              Skip over the chunks without NR in the iots and generate the
 *              `next' iots for the request.
 *
 * @param   raid_group_p - Pointer for raid group for this rebuild request.
 * @param   packet_p   - Pointer to a packet for rebuld request.
 * @param   b_is_metadata_rebuild - FBE_TRUE - This is a metadata rebuild
 *                                  FBE_FALSE - User space rebuild
 * @param   chunk_count - Number of chunks in this iots.
 *
 * @return  fbe_status_t            
 *
 * @author
 *  01/27/2012  Ron Proulx  - Created.
 *
 *****************************************************************************/
static fbe_status_t fbe_raid_group_rebuild_handle_chunks_without_needs_rebuild(fbe_raid_group_t *raid_group_p,
                                                                               fbe_packet_t *packet_p,
                                                                               fbe_bool_t b_is_metadata_rebuild,
                                                                               fbe_chunk_count_t chunk_count,
                                                                               fbe_bool_t      move_rebuild_checkpoint,
                                                                               fbe_lba_t       new_checkpoint) 
{
    fbe_status_t                        status = FBE_STATUS_OK;                               
    fbe_chunk_index_t                   chunk_index;                          
    fbe_payload_ex_t                   *sep_payload_p = NULL;                  
    fbe_payload_control_operation_t    *control_operation_p = NULL;            
    fbe_raid_group_rebuild_context_t   *rebuild_context_p = NULL;              
    fbe_raid_iots_t                    *iots_p = NULL;
    fbe_raid_position_bitmask_t         positions_to_rebuild = 0;
    fbe_raid_position_bitmask_t         rebuild_positions_marked_nr = 0;
    fbe_bool_t                          b_is_last_iots_of_request = FBE_FALSE;
    fbe_bool_t                          b_are_all_chunks_zero = FBE_TRUE;
    fbe_lba_t                           packet_lba;
    fbe_block_count_t                   packet_blocks; 
    fbe_chunk_size_t                    chunk_size;

    /* Get a pointer to the rebuild context from the packet.
     */
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    fbe_payload_ex_get_iots(sep_payload_p, &iots_p);
    control_operation_p = fbe_payload_ex_get_present_control_operation(sep_payload_p);
    fbe_payload_control_get_buffer(control_operation_p, &rebuild_context_p);
    positions_to_rebuild = rebuild_context_p->rebuild_bitmask;
    fbe_raid_iots_get_packet_lba(iots_p, &packet_lba);
    fbe_raid_iots_get_packet_blocks(iots_p, &packet_blocks);

    /*! @note If the chunk is not in the data area, then the non-paged metadata 
     *        service was used to get the NR information.  In addition there is 
     *        no position associated with the metadata rebuild.
     */
    if (b_is_metadata_rebuild == FBE_TRUE)
    {
        /* Get the rebuild position bitmask.
         */
        fbe_raid_group_rebuild_get_all_rebuild_position_mask(raid_group_p, &positions_to_rebuild);
    }

    /* We assumed that the first chunk doesn't have NR set.
     * (We always need to remove the positions that are missing)
     */
    rebuild_positions_marked_nr = positions_to_rebuild & iots_p->chunk_info[0].needs_rebuild_bits;
    if ( (rebuild_positions_marked_nr != 0) && (move_rebuild_checkpoint == FBE_FALSE) )
    {
        /* Trace an error and complete the packet.
         */
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                              "raid_group: rebuild handle chunks w/o NR: NR[0]: 0x%x isn't 0 rl: 0x%x \n",
                              iots_p->chunk_info[0].needs_rebuild_bits, iots_p->rebuild_logging_bitmask);
        fbe_raid_iots_set_block_status(iots_p, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID_REQUEST);
        fbe_raid_iots_set_block_qualifier(iots_p, FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_UNEXPECTED_ERROR);
        fbe_raid_group_add_to_terminator_queue(raid_group_p, packet_p);
        fbe_raid_group_iots_cleanup(packet_p, (fbe_packet_completion_context_t)raid_group_p);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }

    /* First check if this is the last iots of the request.
     */
    if (fbe_raid_iots_is_flag_set(iots_p, FBE_RAID_IOTS_FLAG_LAST_IOTS_OF_REQUEST))
    {
        b_is_last_iots_of_request = FBE_TRUE;
    }

    /* If the checkpoint doesn't need to be moved, then check whether any chunk is marked for rebuild or not 
       If it is true that means the next chunk that needs to be rebuild is beyond the iots range 
     */
    if(!move_rebuild_checkpoint)
    {
        /* Now walk all the chunks associated with this iots and check if any are
         * none-zero.
         */
        chunk_size = fbe_raid_group_get_chunk_size(raid_group_p);
        for (chunk_index = 0; chunk_index < chunk_count; chunk_index++)
        {
            rebuild_positions_marked_nr = positions_to_rebuild & iots_p->chunk_info[chunk_index].needs_rebuild_bits;
            if (rebuild_positions_marked_nr != 0)
            {
                b_are_all_chunks_zero = FBE_FALSE;
                break;
            }
        }
    }
    
    /* There are (2) cases we must handle:
     *      1. All the chunks for this iots have a NR of 0 but this is not the
     *         last iots of the request (exceeded chunk limit).  In this case
     *         we don't update the check or complete the request we simply bump
     *         the lba and blocks remaining to skip over all the chunks in this
     *         request.  Then we start the next piece.
     *                          OR
     *      2. All the chunks for this iots have a NR of 0 and this is the last
     *         iots of the request.  In this case simply update the checkpoint
     *         and complete the request.
     */
    if ( (b_are_all_chunks_zero == FBE_TRUE) || (move_rebuild_checkpoint) )
    {
        /* If enabled trace some information.
         */
        fbe_raid_group_trace(raid_group_p,
                         FBE_TRACE_LEVEL_INFO,
                         FBE_RAID_GROUP_DEBUG_FLAG_REBUILD_TRACING,
                         "raid_group: all: %d chunks NR not set lba: 0x%llx blks: 0x%llx request complete: %d\n",
                         chunk_count, (unsigned long long)iots_p->lba,
             (unsigned long long)iots_p->blocks,
              b_is_last_iots_of_request);

        /* If this is not the last iots of the request skip over the entire iots
         * and start the next piece (i.e. set the blocks remaining to 0).
         */
        if (b_is_last_iots_of_request == FBE_FALSE)
        {
            /* Mark this piece as complete.
             */
            fbe_raid_iots_mark_piece_complete(iots_p);

            /* Return a Ok which indicates that we should start the next piece.
             */
            return FBE_STATUS_OK;
        }

        /* If the checkpoint needs to be moved and this is the last iots
           move the checkpoint to the new value
        */
        if(move_rebuild_checkpoint)
        {
            packet_blocks = new_checkpoint - packet_lba;
        }
        /* Else set the completion and update the checkpoint using the original 
         * request range.
         */
        fbe_transport_set_completion_function(packet_p, fbe_raid_group_rebuild_not_required_iots_completion, raid_group_p);
        status = fbe_raid_group_rebuild_perform_checkpoint_update_and_log(raid_group_p, 
                                                                          packet_p, 
                                                                          packet_lba,
                                                                          packet_blocks, 
                                                                          positions_to_rebuild);
        return status;
    }

    /* Skip over the chunks that don't require a rebuild.
     */
    status = fbe_raid_iots_remove_nondegraded(iots_p, positions_to_rebuild, chunk_size);

    /* If something went wrong complete the packet now.
     */
    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                              "raid_group: rebuild handle chunks w/o NR: NR[0]: 0x%x rl: 0x%x limit failed.\n",
                              iots_p->chunk_info[0].needs_rebuild_bits, iots_p->rebuild_logging_bitmask);
        fbe_raid_iots_set_block_status(iots_p, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED);
        fbe_raid_iots_set_block_qualifier(iots_p, FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_POSSIBLE);
        fbe_raid_group_add_to_terminator_queue(raid_group_p, packet_p);
        fbe_raid_group_iots_cleanup(packet_p, (fbe_packet_completion_context_t)raid_group_p);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }

    /* If enabled trace some information.
     */
    fbe_raid_group_trace(raid_group_p,
                         FBE_TRACE_LEVEL_INFO,
                         FBE_RAID_GROUP_DEBUG_FLAG_REBUILD_TRACING,
                         "raid_group: rebuild handle chunks w/o NR: updated lba: 0x%llx blks: 0x%llx NR[0]: 0x%x\n",
                         (unsigned long long)iots_p->lba,
             (unsigned long long)iots_p->blocks,
             iots_p->chunk_info[0].needs_rebuild_bits);

    /* We assume that the first chunk must have NR set since the raid library
     * will fail a checked-zero request that does not require a rebuild.
     */
    rebuild_positions_marked_nr = positions_to_rebuild & iots_p->chunk_info[0].needs_rebuild_bits;
    if (rebuild_positions_marked_nr == 0)
    {
        /* Trace an error and complete the packet.
         */
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                              "raid_group: rebuild handle chunks w/o NR: NR[0]: 0x%x is 0 rl: 0x%x \n",
                              iots_p->chunk_info[0].needs_rebuild_bits, iots_p->rebuild_logging_bitmask);
        fbe_raid_iots_set_block_status(iots_p, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID_REQUEST);
        fbe_raid_iots_set_block_qualifier(iots_p, FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_UNEXPECTED_ERROR);
        fbe_raid_group_add_to_terminator_queue(raid_group_p, packet_p);
        fbe_raid_group_iots_cleanup(packet_p, (fbe_packet_completion_context_t)raid_group_p);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }

    /* Return status
     */
    return status;
}
/* end of fbe_raid_group_rebuild_handle_chunks_without_needs_rebuild() */

/*!****************************************************************************
 * fbe_raid_group_rebuild_get_chunk_info_completion()
 ******************************************************************************
 * @brief
 *   This is the completion function for reading the NR paged metadata for a 
 *   chunk.  It will get the NR chunk info returned from the paged metadata  
 *   service and store it in the iots.  If rebuild is needed it will continue the iots.
 *   If not - it will update the checkpoint / Nr bits and finish the iots.
 *   
 *
 * @param in_packet_p   - pointer to a control packet from the scheduler
 * @param in_context    - completion context, which is a pointer to the raid 
 *                        group object
 *
 * @return  fbe_status_t            
 *
 * @author
 *   06/07/2010 - Created. Jean Montes. 
 *
 ******************************************************************************/
static fbe_status_t fbe_raid_group_rebuild_get_chunk_info_completion(
                                    fbe_packet_t*                   in_packet_p,
                                    fbe_packet_completion_context_t in_context)
{
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_raid_group_t                   *raid_group_p = NULL;
    fbe_raid_geometry_t                *raid_geometry_p = NULL; 
    fbe_chunk_index_t                   chunk_index;
    fbe_chunk_index_t                   new_chunk_index;
    fbe_chunk_count_t                   chunk_count; 
    fbe_chunk_count_t                   exported_chunks;
    fbe_status_t                        packet_status;  
    fbe_payload_metadata_status_t       metadata_status;
    fbe_payload_ex_t*                   sep_payload_p = NULL;
    fbe_payload_control_operation_t*    control_operation_p = NULL;
    fbe_payload_metadata_operation_t*   metadata_operation_p = NULL;
    fbe_raid_group_rebuild_context_t*   rebuild_context_p = NULL;
    fbe_raid_iots_t *                   iots_p = NULL;
    fbe_raid_position_bitmask_t         positions_to_rebuild = 0;
    fbe_raid_position_bitmask_t         np_rl_bitmask = 0;
    fbe_raid_position_bitmask_t         rebuild_positions_marked_nr = 0;
    fbe_bool_t                          b_is_in_data_area = FBE_TRUE;
    fbe_bool_t                          b_is_metadata_rebuild = FBE_FALSE;
    fbe_bool_t                          can_checkpoint_be_moved = FBE_FALSE;
    fbe_u64_t                           current_offset;
    fbe_lba_t                           new_rebuild_checkpoint = FBE_LBA_INVALID;
    fbe_u32_t                           max_dead_positions_before_shutdown = 0;
    fbe_u32_t                           positions_needing_rebuild = 0;

    /* Get the raid group pointer from the input context.
     */ 
    raid_group_p = (fbe_raid_group_t *)in_context;
    raid_geometry_p = fbe_raid_group_get_raid_geometry(raid_group_p); 

    /* Get a pointer to the rebuild context from the packet.
     */
    sep_payload_p = fbe_transport_get_payload_ex(in_packet_p);
    fbe_payload_ex_get_iots(sep_payload_p, &iots_p);
    control_operation_p = fbe_payload_ex_get_present_control_operation(sep_payload_p);
    fbe_payload_control_get_buffer(control_operation_p, &rebuild_context_p);
    positions_to_rebuild = rebuild_context_p->rebuild_bitmask;

    /* Get the chunk range for which we need to perform the rebuild operation.
     */
    fbe_raid_group_bitmap_get_chunk_range(raid_group_p, iots_p->lba,
                                          iots_p->blocks, &chunk_index, &chunk_count);

    /* Get the chunk info that was read determine if the chunk(s) to be read 
     * are in the user data area or the paged metadata area.  If we want to 
     * read a chunk in the user area, we use the paged metadata service to do 
     * that.  If we want to read a chunk in the paged metadata area itself, we
     * use the nonpaged metadata to do that (in the metadata data portion of it).
     */
    status = fbe_raid_group_bitmap_determine_if_chunks_in_data_area(raid_group_p,
                                                                    chunk_index,
                                                                    chunk_count,
                                                                    &b_is_in_data_area);
    if(status != FBE_STATUS_OK)
    {
        status = fbe_raid_group_rebuild_set_iots_status(raid_group_p, in_packet_p, iots_p, in_context,
                                                        FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED,
                                                        FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_POSSIBLE);
        return status;
    }

    /*! @todo If the chunk is not in the data area, then the non-paged metadata 
     *        service was used to get the NR information.  In addition there is 
     *        currently no position associated with the metadata rebuild (will
     *        be changing when metadata rebuild is implemented).
     */
    b_is_metadata_rebuild = (b_is_in_data_area == FBE_TRUE) ? FBE_FALSE : FBE_TRUE;
    if (b_is_metadata_rebuild == FBE_TRUE)
    {
        /* Get the rebuild position bitmask.
         */
        fbe_raid_group_rebuild_get_all_rebuild_position_mask(raid_group_p, &positions_to_rebuild);
        
        /* Get the rebuild chunk information using the non-paged metadata.
         */
        packet_status = fbe_transport_get_status_code(in_packet_p);
        status = fbe_raid_group_bitmap_process_chunk_info_read_using_nonpaged_done(raid_group_p, 
                                                                                   in_packet_p, 
                                                                                   chunk_index, 
                                                                                   chunk_count, 
                                                                                   &iots_p->chunk_info[0]);
        if( (status != FBE_STATUS_OK) || (packet_status != FBE_STATUS_OK) )
        {
            fbe_base_object_trace((fbe_base_object_t*)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "rb failed reading MD of MD. status: 0x%x\n",
                               status);
            status = fbe_raid_group_rebuild_set_iots_status(raid_group_p, in_packet_p, iots_p, in_context,
                                                            FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED,
                                                            FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_POSSIBLE);
            return status;
        }
    }
    /* We read the paged metadata for the user area */
    else
    {
        
        metadata_operation_p    = fbe_payload_ex_get_metadata_operation(sep_payload_p);
        fbe_payload_metadata_get_status(metadata_operation_p, &metadata_status);
        /* If the metadata status is bad, handle it */
        if(metadata_status != FBE_PAYLOAD_METADATA_STATUS_OK)
        {
            fbe_raid_group_handle_metadata_error(raid_group_p, in_packet_p, metadata_status);
        }

        /* If the status is bad, release the metadata operation and clean up the iots */
        packet_status = fbe_transport_get_status_code(in_packet_p);
        if(packet_status != FBE_STATUS_OK)
        {
            fbe_payload_ex_release_metadata_operation(sep_payload_p, metadata_operation_p);

            fbe_base_object_trace((fbe_base_object_t*)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "rb failed reading paged metadata. packet status: 0x%x\n",
                               packet_status);
            status = fbe_raid_group_rebuild_set_iots_status(raid_group_p, in_packet_p, iots_p, in_context,
                                                        FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED,
                                                        FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_POSSIBLE);
            return status;

        }
    
        /* Current offset gives the new metadata offset for the chunk that needs to be acted upon*/
        current_offset = metadata_operation_p->u.next_marked_chunk.current_offset;
    
        /* calculate the new checkpoint based on the new metadata offset */
        new_chunk_index = current_offset/sizeof(fbe_raid_group_paged_metadata_t);
        exported_chunks = fbe_raid_group_get_exported_chunks(raid_group_p);
        if(new_chunk_index >= exported_chunks)
        {
            fbe_base_object_trace((fbe_base_object_t*)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s: Setting the chkpt to exported capacity\n", __FUNCTION__);
            new_chunk_index = exported_chunks;
        }
        fbe_raid_group_bitmap_get_lba_for_chunk_index(raid_group_p,
                                                      new_chunk_index,
                                                      &new_rebuild_checkpoint);

        //  Copy the chunk info that was read to iots chunk info
        fbe_copy_memory(&iots_p->chunk_info[0], 
                        metadata_operation_p->u.metadata.record_data, 
                        metadata_operation_p->u.metadata.record_data_size);

        fbe_payload_ex_release_metadata_operation(sep_payload_p, metadata_operation_p);

        /* Get next bits would have returned the next chunk that needs to be rebuilt. If that next chunk
          is beyond the iots range then set the variable to true
        */
        can_checkpoint_be_moved = (new_rebuild_checkpoint >= (iots_p->lba + iots_p->blocks))? FBE_TRUE: FBE_FALSE;

    }

    /* Trace some information.
     */
    fbe_raid_group_trace(raid_group_p,
                         FBE_TRACE_LEVEL_INFO,
                         FBE_RAID_GROUP_DEBUG_FLAG_REBUILD_TRACING,
                         "raid_group: rebuild iots %x lba: 0x%llx blks: 0x%llx NR[0]: 0x%x rebuild: 0x%x rl mask: 0x%x \n",
                         (fbe_u32_t)iots_p, iots_p->lba, iots_p->blocks, iots_p->chunk_info[0].needs_rebuild_bits, 
                         positions_to_rebuild,
                         iots_p->rebuild_logging_bitmask);

    /* If enabled display the needs rebuild information for each chunk.
     */
    if (fbe_raid_group_is_debug_flag_set(raid_group_p, FBE_RAID_GROUP_DEBUG_FLAG_REBUILD_TRACING))
    {
        /* Trace all the `needs rebuild' information.
         */
        fbe_raid_group_rebuild_trace_chunk_info(raid_group_p, &iots_p->chunk_info[0], chunk_count, iots_p);
    }

    /* Now validate that the iots and non-paged metadata are in sync.
     */
    fbe_raid_group_get_rb_logging_bitmask(raid_group_p, &np_rl_bitmask);
    if (iots_p->rebuild_logging_bitmask != np_rl_bitmask)
    {
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                              "raid_group: rebuild non-paged: 0x%x rebuild logging out of sync with iots: 0x%x\n",
                              np_rl_bitmask, iots_p->rebuild_logging_bitmask);
        
        status = fbe_raid_group_rebuild_set_iots_status(raid_group_p, in_packet_p, iots_p, in_context,
                                                        FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID_REQUEST,
                                                        FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_UNEXPECTED_ERROR);
        return status;
    }

    /* Now validate that we have not exceeded the maximum number of degraded positions.
     */
    fbe_raid_geometry_get_max_dead_positions(raid_geometry_p, 
                                             &max_dead_positions_before_shutdown);
    positions_needing_rebuild = fbe_raid_group_util_count_bits_set_in_bitmask(iots_p->chunk_info[0].needs_rebuild_bits);
    if (positions_needing_rebuild > max_dead_positions_before_shutdown)
    {
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                              "raid_group: rebuild NR[0]: 0x%x positions marked NR: %d is greater than max: %d\n",
                              iots_p->chunk_info[0].needs_rebuild_bits, 
                              positions_needing_rebuild, max_dead_positions_before_shutdown);
        
        status = fbe_raid_group_rebuild_set_iots_status(raid_group_p, in_packet_p, iots_p, in_context,
                                                        FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID_REQUEST,
                                                        FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_TOO_MANY_DEAD_POSITIONS);
        return status;
    }

    /* If any of the positions we are trying to rebuild are now rebuild logging
     * (i.e. removed) fail the rebuild request with `retryable'.
     */
    if ((iots_p->rebuild_logging_bitmask & positions_to_rebuild) != 0)
    {
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "raid_group: rebuild one or more rebuild positions: 0x%x removed(rl: 0x%x)\n",
                              positions_to_rebuild, iots_p->rebuild_logging_bitmask);
        status = fbe_raid_group_rebuild_set_iots_status(raid_group_p, in_packet_p, iots_p, in_context,
                                                        FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED,
                                                        FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_POSSIBLE);
        return status;
    }

    /* Determine if the first chunk in the bitmap needs to be rebuilt for this 
     * position.  If it does not need to be rebuilt, update the checkpoint(s) 
     * to move to the next chunk.  
     */
    rebuild_positions_marked_nr = positions_to_rebuild & iots_p->chunk_info[0].needs_rebuild_bits;
    
    if ( (rebuild_positions_marked_nr == 0) || can_checkpoint_be_moved )
    {
        /* Invoke method that will handle one or more chunks that doesn't need
         * to be rebuilt.  If the return status is `more processing required'
         * we just return.  Otherwise we have skipped over the the chunks that
         * don't require a rebuild and there are more to do (i.e. continue).
         */
        status = fbe_raid_group_rebuild_handle_chunks_without_needs_rebuild(raid_group_p,
                                                                            in_packet_p,
                                                                            b_is_metadata_rebuild,
                                                                            chunk_count,
                                                                            can_checkpoint_be_moved,
                                                                            new_rebuild_checkpoint);
        if (status == FBE_STATUS_MORE_PROCESSING_REQUIRED)
        {
            return FBE_STATUS_MORE_PROCESSING_REQUIRED;
        }
    }

    /* Special case if there were not chunks with NR for the entire iots 
     * (limited by the maximum chunks per iots), then we want to go to the 
     * next iots.
     */
    if (fbe_raid_iots_is_piece_complete(iots_p) == FBE_TRUE)
    {
        /* Put the packet back on the termination queue since we had to take it 
         * off before reading the chunk info.
         */
        fbe_raid_group_add_to_terminator_queue(raid_group_p, in_packet_p);
        status = fbe_raid_group_iots_start_next_piece(in_packet_p, raid_group_p);
    }
    else
    {
        /* Else start the iots but skip chunks that don't require a rebuild.
         */
        status = fbe_raid_group_limit_and_start_iots(in_packet_p, raid_group_p);
    }

    /* Return the status determined.
     */
    return status;    

}   // End fbe_raid_group_rebuild_get_chunk_info_completion()


/*!**************************************************************
 * fbe_raid_group_background_op_is_rebuild_need_to_run()
 ****************************************************************
 * @brief
 *  This function checks rebuild checkpoint and determine if rebuild 
 *  background operation needs to run or not.
 *
 * @param in_raid_group_p           - pointer to the raid group
 *
 * @return bool status - returns FBE_TRUE if the operation needs to run
 *                       otherwise returns FBE_FALSE
 *
 * @author
 *  07/28/2011 - Created. Amit Dhaduk
 *
 ****************************************************************/
fbe_bool_t
fbe_raid_group_background_op_is_rebuild_need_to_run(fbe_raid_group_t*    raid_group_p)
{

    fbe_raid_group_nonpaged_metadata_t* nonpaged_metadata_p = NULL;             // pointer to nonpaged metadata 
    fbe_u32_t                           index;                                  // index into rebuild info struct 
    fbe_raid_position_bitmask_t         rl_bitmask;
    
    if (!fbe_base_config_is_system_bg_service_enabled(FBE_BASE_CONFIG_CONTROL_SYSTEM_BG_SERVICE_ENABLE_REBUILD))
    {
        return FBE_FALSE;
    }

    /*  Get a pointer to the raid group object non-paged metadata  */
    fbe_base_config_get_nonpaged_metadata_ptr((fbe_base_config_t*) raid_group_p, (void **)&nonpaged_metadata_p);

    //  Get the rebuild logging bitmask 
    fbe_raid_group_get_rb_logging_bitmask(raid_group_p, &rl_bitmask);

    /*  Loop through the rebuild positions to see if the rebuild checkpoint needs to be driven forward
     */
    for (index = 0; index < FBE_RAID_GROUP_MAX_REBUILD_POSITIONS; index++)
    {
        /* check if rebuild checkpoint is valid
         */
        if (nonpaged_metadata_p->rebuild_info.rebuild_checkpoint_info[index].checkpoint != FBE_LBA_INVALID)
        {
            /* check if rebuild logging is not in progress
             */
            if( (1 << nonpaged_metadata_p->rebuild_info.rebuild_checkpoint_info[index].position) & (~rl_bitmask))
            {
                return FBE_TRUE;
            }
        }
    }
    return FBE_FALSE;
}
/******************************************************************************
 * end fbe_raid_group_background_op_is_rebuild_need_to_run()
 ******************************************************************************/

/*!**************************************************************
 * fbe_raid_group_background_op_is_metadata_rebuild_need_to_run()
 ****************************************************************
 * @brief
 *  This function checks rebuild metadata of metadata and determine 
 *  if metadata rebuild background operation needs to run or not.
 *
 * @param in_raid_group_p           - pointer to the raid group
 *
 * @return bool status - returns FBE_TRUE if the operation needs to run
 *                       otherwise returns FBE_FALSE
 *
 * @author
 *  08/01/2011 - Created. Amit Dhaduk
 *
 ****************************************************************/
fbe_bool_t
fbe_raid_group_background_op_is_metadata_rebuild_need_to_run(fbe_raid_group_t*    raid_group_p)
{

    fbe_chunk_count_t                   cur_chunk_count;            // chunk number currently processing    
    fbe_raid_group_paged_metadata_t*    paged_md_md_p;              // pointer to paged metadata metadata 
    fbe_raid_position_bitmask_t         rl_bitmask;
    fbe_chunk_count_t                   chunk_count;
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_group_get_raid_geometry(raid_group_p);

    if (!fbe_raid_geometry_has_paged_metadata(raid_geometry_p))
    {
        /* Raw mirrors do not use paged metadata.  They use non-paged mdd for all 
         * their user area. 
         */
        return FBE_FALSE;
    }

    /*  Get a pointer to the paged metadata metadata  */
    paged_md_md_p = fbe_raid_group_get_paged_metadata_metadata_ptr(raid_group_p); 

    /*  Get the rebuild logging bitmask */
    fbe_raid_group_get_rb_logging_bitmask(raid_group_p, &rl_bitmask);

    /*  Loop through the all rebuild metada chunks to see if any chunks needs rebuild
     */
    fbe_raid_group_bitmap_get_md_of_md_count(raid_group_p, &chunk_count);
    for (cur_chunk_count = 0; cur_chunk_count < chunk_count; cur_chunk_count++)
    {
        if ((paged_md_md_p[cur_chunk_count].needs_rebuild_bits)& (~rl_bitmask))
        {
            return FBE_TRUE;
        }
    }

    return FBE_FALSE;
}
/******************************************************************************
 * end fbe_raid_group_background_op_is_metadata_rebuild_need_to_run()
 ******************************************************************************/

/*!**************************************************************
 * fbe_raid_group_is_any_metadata_marked_NR()
 ****************************************************************
 * @brief
 *  This function checks whether any the metadata of metadata chunks
 *  is marked NR or not
 *
 * @param in_raid_group_p           
 * @param nr_bimtask
 *
 * @return bool status - returns FBE_TRUE if any metadata of metadata
 *                       is marked NR, otherwise return false
 *
 * @author
 *  9/29/2012 - Created. NCHIU
 *
 ****************************************************************/
fbe_bool_t fbe_raid_group_is_any_metadata_marked_NR(fbe_raid_group_t* raid_group_p,
                                                    fbe_raid_position_bitmask_t nr_bitmask)
{

    fbe_chunk_count_t                   cur_chunk_count; 
    fbe_raid_group_paged_metadata_t*    paged_md_md_p; 
    fbe_chunk_count_t                   chunk_count;

    /*  Get a pointer to the paged metadata metadata  */
    paged_md_md_p = fbe_raid_group_get_paged_metadata_metadata_ptr(raid_group_p);    
    fbe_raid_group_bitmap_get_md_of_md_count(raid_group_p, &chunk_count);
    /*  Loop through the all rebuild metada chunks to see if any chunks needs rebuild
     */
    for (cur_chunk_count = 0; cur_chunk_count < chunk_count; cur_chunk_count++)
    {
        if ( (paged_md_md_p[cur_chunk_count].needs_rebuild_bits & nr_bitmask) == nr_bitmask)
        {
            return FBE_TRUE;
        }
    }
    return FBE_FALSE;
}
/******************************************************************************
 * end fbe_raid_group_is_any_metadata_marked_NR()
 ******************************************************************************/

/*!**************************************************************
 * fbe_raid_group_is_metadata_marked_NR()
 ****************************************************************
 * @brief
 *  This function checks whether all the metadata of metadata chunks
 *  is marked NR or not
 *
 * @param in_raid_group_p           
 * @param nr_bimtask
 *
 * @return bool status - returns FBE_TRUE if all metadata of metadata
 *                       is marked NR, otherwise return false
 *
 * @author
 *  10/27/2011 - Created. Ashwin Tamilarasan
 *
 ****************************************************************/
fbe_bool_t
fbe_raid_group_is_metadata_marked_NR(fbe_raid_group_t* raid_group_p,
                                     fbe_raid_position_bitmask_t nr_bitmask)
{

    fbe_chunk_count_t                   cur_chunk_count; 
    fbe_raid_group_paged_metadata_t*    paged_md_md_p; 
    fbe_chunk_count_t                   chunk_count;

    /*  Get a pointer to the paged metadata metadata  */
    paged_md_md_p = fbe_raid_group_get_paged_metadata_metadata_ptr(raid_group_p);    
    fbe_raid_group_bitmap_get_md_of_md_count(raid_group_p, &chunk_count);
    /*  Loop through the all rebuild metada chunks to see if any chunks needs rebuild
     */
    for (cur_chunk_count = 0; cur_chunk_count < chunk_count; cur_chunk_count++)
    {
        if ( (paged_md_md_p[cur_chunk_count].needs_rebuild_bits & nr_bitmask) != nr_bitmask)
        {
            return FBE_FALSE;
        }
    }
    return FBE_TRUE;
}
/******************************************************************************
 * end fbe_raid_group_is_metadata_marked_NR()
 ******************************************************************************/

/*!**************************************************************
 * fbe_raid_group_rebuild_get_rebuilding_disk_position()
 ****************************************************************
 * @brief
 *  This function find out the rebuilding disk position for given lba.
 *
 * @param in_raid_group_p           - pointer to the raid group
 *        in_rebuild_io_lba         - rebuild IO lba
 *        out_disk_position_p       - pointer to the rebuilding disk position
 *
 * @return  fbe_status_t            
 *
 * @author
 *  08/11/2011 - Created. Amit Dhaduk
 *
 ****************************************************************/
static fbe_status_t fbe_raid_group_rebuild_get_rebuilding_disk_position(
                                    fbe_raid_group_t*           in_raid_group_p,
                                    fbe_lba_t                   in_rebuild_io_lba,
                                    fbe_u32_t*                  out_disk_position_p)
{
    fbe_raid_group_nonpaged_metadata_t* nonpaged_metadata_p = NULL;             // pointer to nonpaged metadata 
    fbe_u32_t                           index;                                  // index into rebuild info struct 
    fbe_bool_t                          b_rebuild_logging = FBE_FALSE;

    *out_disk_position_p = FBE_RAID_INVALID_DISK_POSITION;
    /*  Get a pointer to the raid group object non-paged metadata  */
    fbe_base_config_get_nonpaged_metadata_ptr((fbe_base_config_t*) in_raid_group_p, (void **)&nonpaged_metadata_p);

    /*  Loop through the rebuild positions to see if the rebuild checkpoint needs to be driven forward
     */
    for (index = 0; index < FBE_RAID_GROUP_MAX_REBUILD_POSITIONS; index++)
    {
        /*  If the disk is rb logging, then it can not be rebuilt.  Move on to the next disk. */
        fbe_raid_group_get_rb_logging(in_raid_group_p, nonpaged_metadata_p->rebuild_info.rebuild_checkpoint_info[index].position, &b_rebuild_logging); 
        if (b_rebuild_logging == FBE_TRUE)
        {
            continue; 
        }

        /* return the disk position if the given rebuild checkpoint match */
        if (nonpaged_metadata_p->rebuild_info.rebuild_checkpoint_info[index].checkpoint == in_rebuild_io_lba)
        {
            *out_disk_position_p = nonpaged_metadata_p->rebuild_info.rebuild_checkpoint_info[index].position;
            break;
        }
    }

    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_raid_group_rebuild_get_rebuilding_disk_position()
 ******************************************************************************/

/*!**************************************************************
 * fbe_raid_group_rebuild_get_all_rebuild_position_mask()
 ****************************************************************
 * @brief
 *  This function returns mask of all disk position which can be bebuild.
 *
 * @param in_raid_group_p            - pointer to the raid group
 *        out_rebuild_all_mask_p     - pointer to the disk position mask 
 *                                     which can be rebuild
 *
 * @return  fbe_status_t            
 *
 * @author
 *  08/11/2011 - Created. Amit Dhaduk
 *
 ****************************************************************/
fbe_status_t fbe_raid_group_rebuild_get_all_rebuild_position_mask(
                                    fbe_raid_group_t*               in_raid_group_p,
                                    fbe_raid_position_bitmask_t*    out_rebuild_all_mask_p)
{
    fbe_raid_group_nonpaged_metadata_t* nonpaged_metadata_p = NULL;             // pointer to nonpaged metadata 
    fbe_u32_t                           index;                                  // index into rebuild info struct 
    fbe_bool_t                          b_rebuild_logging = FBE_FALSE;          // rebuild logging flag

    *out_rebuild_all_mask_p = 0;

    /*  Get a pointer to the raid group object non-paged metadata  */
    fbe_base_config_get_nonpaged_metadata_ptr((fbe_base_config_t*) in_raid_group_p, (void **)&nonpaged_metadata_p);

    /*  Loop through the rebuild positions and mask it if it can be rebuild
     */
    for (index = 0; index < FBE_RAID_GROUP_MAX_REBUILD_POSITIONS; index++)
    {
        if (nonpaged_metadata_p->rebuild_info.rebuild_checkpoint_info[index].position != FBE_RAID_INVALID_DISK_POSITION)
        {
            /*  If the disk is rb logging, then it can not be rebuilt.  Move on to the next disk. 
             */
            fbe_raid_group_get_rb_logging(in_raid_group_p, nonpaged_metadata_p->rebuild_info.rebuild_checkpoint_info[index].position, &b_rebuild_logging); 
            if (b_rebuild_logging == FBE_TRUE)
            {
                continue; 
            }
            *out_rebuild_all_mask_p |= (1 << nonpaged_metadata_p->rebuild_info.rebuild_checkpoint_info[index].position);
        }
    }

    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_raid_group_rebuild_get_all_rebuild_position_mask()
 ******************************************************************************/

/*!**************************************************************
 * fbe_raid_group_get_degraded_mask()
 ****************************************************************
 * @brief
 *  This function returns mask of all degraded positions.
 *
 * @param raid_group_p            - pointer to the raid group
 *        rebuild_all_mask_p     - pointer to the mask of positions
 *                                     that are degraded.
 *
 * @return  fbe_status_t            
 *
 * @author
 *  12/16/2013 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t 
fbe_raid_group_get_degraded_mask(fbe_raid_group_t*               raid_group_p,
                                 fbe_raid_position_bitmask_t*    rebuild_all_mask_p)
{
    fbe_raid_group_nonpaged_metadata_t* nonpaged_metadata_p = NULL;
    fbe_u32_t                           index;

    *rebuild_all_mask_p = 0;

    /*  Get a pointer to the raid group object non-paged metadata  */
    fbe_base_config_get_nonpaged_metadata_ptr((fbe_base_config_t*) raid_group_p, (void **)&nonpaged_metadata_p);

    /*  Loop through the rebuild positions and mask it if it can be rebuild
     */
    for (index = 0; index < FBE_RAID_GROUP_MAX_REBUILD_POSITIONS; index++) {
        if (nonpaged_metadata_p->rebuild_info.rebuild_checkpoint_info[index].position != FBE_RAID_INVALID_DISK_POSITION) {
            *rebuild_all_mask_p |= (1 << nonpaged_metadata_p->rebuild_info.rebuild_checkpoint_info[index].position);
        }
    }
    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_raid_group_get_degraded_mask()
 ******************************************************************************/


/*!****************************************************************************
 * fbe_raid_group_rebuild_get_next_metadata_needs_rebuild_lba
 ******************************************************************************
 * @brief
 *   This function is used to get the next metadata lba which needs rebuild.
 *
 * @param   in_raid_group_p             - pointer to a raid group object
 * @param   out_metadata_rebuild_lba_p  - pointer to the next lba which needs rebuild
 * @param   lowest_rebuild_position_p   - pointer to lowest rebuild position
 * 
 * @return fbe_status_t  
 *
 * @author
 *   08/11/2011 - Created. Amit Dhaduk.
 *
 ******************************************************************************/
fbe_status_t
fbe_raid_group_rebuild_get_next_metadata_lba_to_rebuild(
                                        fbe_raid_group_t*           in_raid_group_p,
                                        fbe_lba_t*                  out_metadata_rebuild_lba_p,
                                        fbe_u32_t*                  out_lowest_rebuild_position_p)
{
    fbe_raid_position_bitmask_t         rl_bitmask;                 // rebuild logging bitmask
    fbe_lba_t                           rebuild_checkpoint;
    fbe_lba_t                           exported_capacity;
    fbe_u32_t                           rebuild_position;

    /*  Initialize the metadata rebuild LBA as invalid */
    *out_metadata_rebuild_lba_p = FBE_LBA_INVALID;
    *out_lowest_rebuild_position_p = FBE_RAID_INVALID_DISK_POSITION;

    fbe_raid_group_get_rb_logging_bitmask(in_raid_group_p, &rl_bitmask);

    /* Get the position which has the lowest checkpoint.
     */
    exported_capacity = fbe_raid_group_get_exported_disk_capacity(in_raid_group_p);
    fbe_raid_group_rebuild_get_metadata_rebuild_position(in_raid_group_p, &rebuild_position);
    if(rebuild_position != FBE_RAID_INVALID_DISK_POSITION)
    {
        *out_lowest_rebuild_position_p = rebuild_position;
        fbe_raid_group_get_rebuild_checkpoint(in_raid_group_p, rebuild_position, &rebuild_checkpoint);
    }
    else
    {
        return FBE_STATUS_OK;
    }

    /* If we make it to this point that means we have a position that needs its metadata to be rebuilt */
    if(rebuild_checkpoint <= exported_capacity )
    {
        *out_metadata_rebuild_lba_p = exported_capacity;
    }
    else 
    {
        *out_metadata_rebuild_lba_p = rebuild_checkpoint;
    }

    fbe_base_object_trace((fbe_base_object_t*)in_raid_group_p, 
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "raid_group: run md reb cpt 0x%llx, cur cpt 0x%llx, exp c: 0x%llx rb pos:0x%x\n" , 
                          (unsigned long long)*out_metadata_rebuild_lba_p, (unsigned long long)rebuild_checkpoint, (unsigned long long)exported_capacity, rebuild_position);
    
    
    /*  Return success  */
    return FBE_STATUS_OK;

}  
/******************************************************************************
 * fbe_raid_group_rebuild_get_all_rebuild_position_mask()
 *****************************************************************************/

/*!****************************************************************************
 *  fbe_raid_group_get_raid10_rebuild_info()
 ******************************************************************************
 * @brief 
 *   Get the rebuild checkpoint for a R10 for the given mirror position.
 *
 * @param packet_p               - pointer to packet since we need to to send to downstream objects
 * @param raid_group_p       - pointer to the raid group object 
 * @param position           - disk position in the RG 
 * @param checkpoint_p      - pointer to data which gets set to the rebuild
 *                                checkpoint.  This value is only valid when 
 *                                the function returns success.
 *
 * @return fbe_status_t        
 * @author
 *   12/21/2011 - Created. Shay Harel
 ******************************************************************************/
fbe_status_t fbe_raid_group_get_raid10_rebuild_info(fbe_packet_t *packet_p,
                                                    fbe_raid_group_t *raid_group_p,
                                                    fbe_u32_t position,
                                                    fbe_lba_t *checkpoint_p,
                                                    fbe_bool_t *b_rb_logging_p)
{

    fbe_payload_ex_t*                   payload_p;
    fbe_payload_control_operation_t*    control_operation_p;  
    fbe_raid_group_get_info_t           get_info;/*can stay on the stack this is a sync. call*/
    fbe_block_edge_t *                  block_edge_p = NULL;
    fbe_path_state_t                    path_state = FBE_PATH_STATE_INVALID;
    fbe_status_t                        status;
    fbe_u32_t                           position_index = 0;
    fbe_semaphore_t                     sem;

    /* Get the control op buffer data pointer from the packet payload.*/
    payload_p = fbe_transport_get_payload_ex(packet_p);
    if (payload_p == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                                FBE_TRACE_LEVEL_WARNING,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s payload is NULL\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*we need to send a packet to the mirror RG below us to get it's info and extrapolate from that
    what is the minimal one, and then apply it to us*/

    /*find the object id of our mirror server*/
    fbe_base_config_get_block_edge((fbe_base_config_t*)raid_group_p, &block_edge_p, position);
    if (block_edge_p == NULL){
        *checkpoint_p = FBE_LBA_INVALID ;
        return FBE_STATUS_OK;/*no edge to process*/
    }

    fbe_block_transport_get_path_state (block_edge_p, &path_state);
    if (path_state == FBE_PATH_STATE_INVALID){
        *checkpoint_p = FBE_LBA_INVALID;
        return FBE_STATUS_OK;/*no edge to process*/
    }

    fbe_semaphore_init(&sem, 0, 1);
    
    /* Set packet address.*/
    fbe_transport_set_address(packet_p,
                              FBE_PACKAGE_ID_SEP_0,
                              FBE_SERVICE_ID_TOPOLOGY,
                              FBE_CLASS_ID_INVALID,
                              block_edge_p->base_edge.server_id); 


    /* allocate the new control operation*/
    control_operation_p = fbe_payload_ex_allocate_control_operation(payload_p);
    fbe_payload_control_build_operation(control_operation_p,
                                        FBE_RAID_GROUP_CONTROL_CODE_GET_INFO,
                                        &get_info,
                                        sizeof(fbe_raid_group_get_info_t));

    fbe_transport_set_completion_function(packet_p, fbe_raid_group_get_raid10_rebuild_info_completion, (fbe_packet_completion_context_t) &sem);

    fbe_payload_ex_increment_control_operation_level(payload_p);
    
    /*send it*/
    fbe_block_transport_send_control_packet(block_edge_p, packet_p);

    fbe_semaphore_wait(&sem, NULL);
    fbe_semaphore_destroy(&sem);

    fbe_payload_ex_release_control_operation(payload_p, control_operation_p);

    /*when we get here the data is there for us to use(in case the packet completed nicely*/
    status = fbe_transport_get_status_code(packet_p);
    if (status != FBE_STATUS_OK) {
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                                FBE_TRACE_LEVEL_WARNING,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s failed to get RG info from mirror id 0x%X\n", __FUNCTION__,block_edge_p->base_edge.server_id);
        return status;
    }

    for (position_index = 0; position_index < get_info.width; position_index++){
         checkpoint_p[position_index] = get_info.rebuild_checkpoint[position_index];
         b_rb_logging_p[position_index] = get_info.b_rb_logging[position_index];
    }

    return FBE_STATUS_OK;
}

/******************************************************************************
 * end fbe_raid_group_get_raid10_rebuild_checkpoint()
 *****************************************************************************/

/*!****************************************************************************
 * fbe_raid_group_set_rebuild_checkpoint_to_zero()
 ******************************************************************************
 * @brief
 *      This function sets the rebuild checkpoint to 0 for the given disk position
 *      This function will only be called when we finished the metadata rebuild
 *      and we would like to start the user rebuild. At that point we need to
 *      move the rebuild checkpoint from the end of configured capacity to 0
 * 
 * @param packet_p - Pointer to originating packet
 * @param raid_group_p - Pointer to raid group
 * @param metadata_rebuilt_positions - Bitmask of positions that need checkpoint
 *              reset.
 * 
 * @return fbe_status_t 
 *
 *
 * @author
 *   01/25/2012 - Created. Ashwin Tamilarasan
 *
 ******************************************************************************/
fbe_status_t fbe_raid_group_set_rebuild_checkpoint_to_zero(fbe_raid_group_t *raid_group_p,
                                                           fbe_packet_t *packet_p,
                                                           fbe_raid_position_bitmask_t metadata_rebuilt_positions)
{
    fbe_status_t    status = FBE_STATUS_OK;
    fbe_bool_t      b_metadata_offset_in_use = FBE_FALSE;
    fbe_u64_t       metadata_offset;
    fbe_u64_t       second_metadata_offset = 0;
    fbe_u32_t       width;
    fbe_u32_t       num_of_positions_requiring_reset;
    fbe_u32_t       raid_group_index;
    fbe_u32_t       rebuild_index;
    fbe_u32_t       rebuild_position;
    fbe_lba_t       rebuild_checkpoint;

    /* For all positions that just completed the rebuild.
     */
    num_of_positions_requiring_reset = fbe_raid_get_bitcount((fbe_u32_t)metadata_rebuilt_positions);;
    fbe_raid_group_get_width(raid_group_p, &width);

    /*! @note Currently there are only (2) valid values for number of rebuild 
     *        positions to reset:
     *          o 1 - Typically case where only (1) position is rebuilding
     *                              OR
     *          o 2 - 3-way mirror or RAID-6
     *  
     *        A value of (0) is not allowed or expected.
     */
    if ((num_of_positions_requiring_reset < 1)                                    ||
        (num_of_positions_requiring_reset > FBE_RAID_GROUP_MAX_REBUILD_POSITIONS)    )
    {
        /* Trace an error and return a failure.
         */
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "raid_group: rebuild metadata reset: num of positions: %d (bitmask: 0x%x) unexpected max: %d \n", 
                              num_of_positions_requiring_reset, 
                              metadata_rebuilt_positions, 
                              FBE_RAID_GROUP_MAX_REBUILD_POSITIONS);

        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_run_queue_push_packet(packet_p, FBE_TRANSPORT_RQ_METHOD_SAME_CORE);

        /* Return a failure. */
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Locate the (1) or (2) positions to reset.
     */
    for (raid_group_index = 0; raid_group_index < width; raid_group_index++)
    {
        /*! @note There could be more than (1) position that completed a rebuild. 
         */
        if ((1 << raid_group_index) & metadata_rebuilt_positions)
        {
            rebuild_position = raid_group_index;
            fbe_raid_group_get_rebuild_checkpoint(raid_group_p, 
                                                  rebuild_position, 
                                                  &rebuild_checkpoint);
            fbe_raid_group_rebuild_find_existing_checkpoint_entry_for_position(raid_group_p, 
                                                                               rebuild_position, 
                                                                               &rebuild_index);

            /* Trace some information.
             */
            fbe_base_object_trace((fbe_base_object_t*)raid_group_p, 
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "raid_group: rebuild metadata reset checkpoint: 0x%llx to 0 pos: %d rebuild index: %d \n", 
                                  (unsigned long long)rebuild_checkpoint,
                  rebuild_position, rebuild_index);
           
            /* Validate rebuild_index.
             */
            if (rebuild_index >= FBE_RAID_GROUP_MAX_REBUILD_POSITIONS)
            {
                fbe_base_object_trace((fbe_base_object_t*)raid_group_p, 
                                      FBE_TRACE_LEVEL_WARNING,
                                      FBE_TRACE_MESSAGE_ID_INFO,
                                      "raid_group: rebuild metadata reset checkpoint: 0x%llx rebuild index: %d greater than max: %d\n", 
                                      (unsigned long long)rebuild_checkpoint, rebuild_index, (FBE_RAID_GROUP_MAX_REBUILD_POSITIONS - 1));

                fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
                fbe_transport_run_queue_push_packet(packet_p, FBE_TRANSPORT_RQ_METHOD_SAME_CORE);

                /* Return a failure. */
                return FBE_STATUS_GENERIC_FAILURE;
            }
             
            /* Reset the `blocks_rebuilt'.
             */
            fbe_raid_group_trace(raid_group_p,
                                 FBE_TRACE_LEVEL_INFO,
                                 FBE_RAID_GROUP_DEBUG_FLAG_REBUILD_NOTIFICATION_TRACING,
                                 "raid_group: RB reset pos: %d index: %d from: 0x%llx to 0 \n",
                                 rebuild_position, rebuild_index,
                                 (unsigned long long)raid_group_p->raid_group_metadata_memory.blocks_rebuilt[rebuild_index]);
            raid_group_p->raid_group_metadata_memory.blocks_rebuilt[rebuild_index] = 0;

            /* Determine if it is the first or second.
            */
            if (b_metadata_offset_in_use == FBE_FALSE)
            {
                b_metadata_offset_in_use = FBE_TRUE;
                metadata_offset = (fbe_u64_t) 
                            (&((fbe_raid_group_nonpaged_metadata_t*)0)->rebuild_info.rebuild_checkpoint_info[rebuild_index]);
            }
            else
            {
                second_metadata_offset = (fbe_u64_t) 
                            (&((fbe_raid_group_nonpaged_metadata_t*)0)->rebuild_info.rebuild_checkpoint_info[rebuild_index]);
            }

        } /* end if this is one of the positions completing rebuild*/

    } /* end for all positions in the raid group */
    
    /* Now reset (1) or (2) checkpoints.
     */
    status = fbe_base_config_metadata_nonpaged_set_checkpoint((fbe_base_config_t *)raid_group_p,
                                                              packet_p,
                                                              metadata_offset,
                                                              second_metadata_offset,
                                                              0 /* checkpoint*/);
    if(status == FBE_STATUS_GENERIC_FAILURE)
    {
        fbe_base_object_trace((fbe_base_object_t*) raid_group_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s: Failed to set rebuild chkpt to zero\n",
                  __FUNCTION__);
    }

    return FBE_STATUS_PENDING;

} // End fbe_raid_group_set_rebuild_checkpoint_to_zero()


/*!****************************************************************************
 * fbe_raid_group_get_rebuild_index()
 ******************************************************************************
 * @brief
 *      This function returns the rebuild index from the pos bitmask that was passed in.
 * 
 * 
 * @param raid_group_p
 * @param pos_mask
 * @param rebuild_index_p
 * 
 * @return fbe_status_t 
 *
 *
 * @author
 *   01/25/2012 - Created. Ashwin Tamilarasan
 *
 ******************************************************************************/
fbe_status_t fbe_raid_group_get_rebuild_index(fbe_raid_group_t*   raid_group_p,
                                              fbe_raid_position_bitmask_t    pos_bitmask,
                                              fbe_u32_t*          rebuild_index_p)
{

    fbe_u32_t                           width;
    fbe_u32_t                           disk_position;
    fbe_raid_position_bitmask_t         cur_position_mask;

    
    fbe_base_config_get_width((fbe_base_config_t*) raid_group_p, &width);

    /*  Find the disk(s) indicated by the positions to change mask */
    for (disk_position = 0; disk_position < width; disk_position++)
    {
        
        cur_position_mask = 1 << disk_position; 
        if ((pos_bitmask & cur_position_mask) == 0)
        {
            continue;
        }

        //  Find the entry in the non-paged MD's rebuild checkpoint info corresponding to this position 
        fbe_raid_group_rebuild_find_existing_checkpoint_entry_for_position(raid_group_p, disk_position, rebuild_index_p);

        //  If no entry was found, then we have a problem - return error
        if (*rebuild_index_p == FBE_RAID_GROUP_INVALID_INDEX)
        {
            fbe_base_object_trace((fbe_base_object_t*) raid_group_p, FBE_TRACE_LEVEL_ERROR, 
                                   FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, "%s no rebuild checkpoint entry found\n", __FUNCTION__);
            return FBE_STATUS_GENERIC_FAILURE; 
        }
    }

   return FBE_STATUS_OK;

} // End fbe_raid_group_get_rebuild_index()

/*!**************************************************************
 * fbe_raid_group_get_percent_rebuilt()
 ****************************************************************
 * @brief
 *  Determine the percentage complete for the rebuild.
 *
 * @param raid_group_p           - pointer to the raid group
 *
 * @return percent_complete
 *
 * @author
 *  3/20/2012 - Created. Rob Foley
 *
 ****************************************************************/
fbe_u32_t
fbe_raid_group_get_percent_rebuilt(fbe_raid_group_t *raid_group_p)
{
    fbe_raid_group_nonpaged_metadata_t* nonpaged_metadata_p = NULL;
    fbe_u32_t                           index;
    fbe_raid_position_bitmask_t         rl_bitmask;
    fbe_lba_t per_disk_capacity;
    fbe_lba_t rebuilding_capacity = 0;
    fbe_lba_t rebuilt_capacity = 0;
    fbe_lba_t exported_per_disk_capacity = 0;
    fbe_u32_t percent_rebuilt;
    fbe_u32_t drives_rebuilding = 0;

    per_disk_capacity = fbe_raid_group_get_disk_capacity(raid_group_p);
    exported_per_disk_capacity = fbe_raid_group_get_exported_disk_capacity(raid_group_p);

    /*  Get a pointer to the raid group object non-paged metadata  */
    fbe_base_config_get_nonpaged_metadata_ptr((fbe_base_config_t*) raid_group_p, (void **)&nonpaged_metadata_p);

    //  Get the rebuild logging bitmask 
    fbe_raid_group_get_rb_logging_bitmask(raid_group_p, &rl_bitmask);

    /* For each drive we find that needs to be rebuilt we will add in its capacity to rebuilding_capacity
     * and we will add in the amount rebuilt to rebuilt_capacity. 
     * Then when we're done we can calculate total rebuild percentage. 
     */
    for (index = 0; index < FBE_RAID_GROUP_MAX_REBUILD_POSITIONS; index++)
    {
        fbe_raid_group_rebuild_checkpoint_info_t *rb_info_p = 
            &nonpaged_metadata_p->rebuild_info.rebuild_checkpoint_info[index];
        /* check if rebuild checkpoint is valid
         */
        if (rb_info_p->checkpoint != FBE_LBA_INVALID)
        {
            drives_rebuilding++;
            /* check if rebuild logging is not in progress
             */
            if ( (1 << rb_info_p->position) & (~rl_bitmask))
            {
                /* We rebuild the metadata first.
                 * If the checkpoint is at the end of the metadata we either just finished rebuilding the user data 
                 * or we just started to rebuild the metadata.
                 */
                if ((rb_info_p->checkpoint >= exported_per_disk_capacity) &&
                    !fbe_raid_group_is_metadata_marked_NR(raid_group_p, (1 << rb_info_p->position)))
                {
                    /* We are done rebuilding since we're at the end of the user area and
                     * the metadata is not marked. 
                     */
                    rebuilding_capacity += per_disk_capacity;
                    rebuilt_capacity += per_disk_capacity;
                }
                else if (rb_info_p->checkpoint >= exported_per_disk_capacity)
                {
                    /* Still rebuilding metadata.  Only calculate percentage of md rebuilt since
                     * we rebuild metadata first. 
                     */
                    rebuilding_capacity += per_disk_capacity;
                    rebuilt_capacity += rb_info_p->checkpoint - exported_per_disk_capacity;
                }
                else
                {
                    /* Checkpoint is in user area.  Add on total user area rebuilt and all of md.
                     */
                    rebuilding_capacity += per_disk_capacity;
                    rebuilt_capacity += rb_info_p->checkpoint;
                    
                    /* add on metadata rebuilt already
                     */
                    rebuilt_capacity += per_disk_capacity - exported_per_disk_capacity;
                }
            }
            else
            {
                /* Not rebuilding yet, add on to what needs to get rebuilt, but not what has already been rebuilt.
                 * This will be 0% rebuilt. 
                 */
                rebuilding_capacity += per_disk_capacity;
            }
        }
        else
        {
            /* Completely rebuilt.  Don't include it.
             */
        }
    }
    if (rebuilding_capacity != 0)
    {
        percent_rebuilt = (fbe_u32_t)((rebuilt_capacity * 100) / rebuilding_capacity);
    }
    else
    {
        /* Did not find anything needing to be rebuilt, just say we are at 100%.
         */
        percent_rebuilt = 100;
    }
    
    return percent_rebuilt;
}
/******************************************************************************
 * end fbe_raid_group_get_percent_rebuilt()
 ******************************************************************************/

/*!***************************************************************************
 *          fbe_raid_group_is_position_fully_rebuilt()
 *****************************************************************************
 *
 * @brief   Determine the requested position is `fully' (i.e. user space and
 *          journal) rebuilt.
 *
 * @param   raid_group_p      - pointer to the raid group
 * @param   position_to_check - Position to check
 *
 * @return  bool - Position is completely rebuilt
 *
 * @note    The rebuild checkpoint is per-disk (i.e. physical).
 *
 * @author
 *  03/26/2013  Ron Proulx  - Created from fbe_raid_group_get_percent_rebuilt
 *
 *****************************************************************************/
static fbe_bool_t fbe_raid_group_is_position_fully_rebuilt(fbe_raid_group_t *raid_group_p,
                                                          fbe_u32_t position_to_check)
{

    fbe_u32_t                           percent_rebuilt = 100;
    fbe_raid_group_metadata_positions_t raid_group_metadata_positions;
    fbe_raid_geometry_t                *raid_geometry_p = fbe_raid_group_get_raid_geometry(raid_group_p);
    fbe_u16_t                           num_data_disks = 0;
    fbe_lba_t                           rebuild_checkpoint_for_position = 0;
    fbe_lba_t                           exported_per_disk_capacity = 0;
    fbe_lba_t                           metadata_start_lba = 0;
    fbe_lba_t                           journal_start_lba = 0;
    fbe_lba_t                           journal_end_lba = 0;
    fbe_chunk_size_t                    chunk_size = 0;
    fbe_lba_t                           per_disk_capacity = 0;
    fbe_raid_position_bitmask_t         rl_bitmask;
    fbe_lba_t                           rebuilt_capacity = 0;

    /* Get the metadata positions.
     */
    fbe_raid_group_get_metadata_positions(raid_group_p, &raid_group_metadata_positions);

    /* Get the number of data disks.
     */
    fbe_raid_geometry_get_data_disks(raid_geometry_p, &num_data_disks);

    /* Get the chunk size.
     */
    chunk_size = fbe_raid_group_get_chunk_size(raid_group_p);

    /* Get the rebuild checkpoint (which is per-disk).
     */
    fbe_raid_group_get_rebuild_checkpoint(raid_group_p, position_to_check, &rebuild_checkpoint_for_position);

    /* Get the per disk exported capacity.  This assumes that the user space
     * is at the lowest address.
     */
    exported_per_disk_capacity = raid_group_metadata_positions.exported_capacity / num_data_disks;

    /* Get the per disk metadata lba and capacity.
     */
    metadata_start_lba = raid_group_metadata_positions.paged_metadata_lba / num_data_disks;

    /* Get the journal start and end per-disk information.
     */
    if (raid_group_metadata_positions.journal_write_log_physical_capacity > 0)
    {
        journal_start_lba = raid_group_metadata_positions.journal_write_log_pba;
        journal_end_lba = journal_start_lba + raid_group_metadata_positions.journal_write_log_physical_capacity;
    }

    /* The per-disk capacity is considered the user capacity.
     */
    per_disk_capacity = exported_per_disk_capacity;

    /* Get the rebuild logging bitmask.
     */ 
    fbe_raid_group_get_rb_logging_bitmask(raid_group_p, &rl_bitmask);

    /* If the rebuild checkpoint is at the end marker then this position is
     * 100 percent rebuilt.
     */
    if (rebuild_checkpoint_for_position == FBE_LBA_INVALID)
    {
        /* Trace some information if enabled.
         */
        fbe_raid_group_trace(raid_group_p,
                             FBE_TRACE_LEVEL_INFO,
                             FBE_RAID_GROUP_DEBUG_FLAG_REBUILD_NOTIFICATION_TRACING,
                             "raid_group: RB is rebuilt pos: %d percent: %d per disk chkpt: 0x%llx cap: 0x%llx\n",
                             position_to_check, percent_rebuilt, (unsigned long long)rebuild_checkpoint_for_position,
                             (unsigned long long)per_disk_capacity);
        return FBE_TRUE;
    }

    /* Validate that this position is not rebuild logging.
     */
    if ((1 << position_to_check) & (~rl_bitmask))
    {
        /*! @note The journal information is per-disk.  Thus the journal 
         *        capacity is the per-disk capacity.  If there is a journal and 
         *        the checkpoint is in the journal range treat the journal space 
         *        as `user' space.
         */
        if ( (raid_group_metadata_positions.journal_write_log_physical_capacity > 0)      &&
            !fbe_raid_group_is_metadata_marked_NR(raid_group_p, (1 << position_to_check)) &&
             (rebuild_checkpoint_for_position >= journal_start_lba)                       &&
             (rebuild_checkpoint_for_position <= journal_end_lba)                            )
        {
            /* If the consumed area has not been rebuilt report 0%.  Else only 
             * report 100% if this is the last chunk of the journal.
             */
            rebuilt_capacity = rebuild_checkpoint_for_position - journal_start_lba;

            /* If the user area still need to be rebuilt report 0%.
             */
            if ((rebuild_checkpoint_for_position == journal_start_lba) &&
                (raid_group_p->previous_rebuild_percent == 0)             )
            {
                /* We have complete rebuilding the metadata but have not
                 * started the rebuild of the user area.  Report 0%.
                 */
                rebuilt_capacity = 0;
            }
            else if (rebuilt_capacity == raid_group_metadata_positions.journal_write_log_physical_capacity)
            {
                /* We have finished rebuilding the journal.  Report 100%.
                 */
                rebuilt_capacity = per_disk_capacity;
            }
            else
            {
                /* Else report the per disk capacity minus (1).
                 */
                rebuilt_capacity = per_disk_capacity - 1;
            }
        }
        else if (fbe_raid_group_is_metadata_marked_NR(raid_group_p, (1 << position_to_check)))
        {
            /* Else if we are rebuilding the metadata none of the user
             * extent has been rebuilt.
             */
            rebuilt_capacity = 0;
        }
        else if (rebuild_checkpoint_for_position > metadata_start_lba)
        {
            /* Else there is a case where we just complete the metadata rebuild
             * but have not started the consumed rebuild.  The rebuilt percent
             * is 0.
             */
            rebuilt_capacity = 0;
        }
        else
        {
            /* Else we are rebuilding the user extent.  If there is a journal area
             * subtract (1) (don't report rebuild complete until the journal is rebuilt).
             */
            rebuilt_capacity = rebuild_checkpoint_for_position;
            if ((raid_group_metadata_positions.journal_write_log_physical_capacity > 0) &&
                (rebuilt_capacity >= per_disk_capacity)                                    )
            {
                /* Wait until the journal is rebuilt before reporting 100%.
                 */
                rebuilt_capacity = rebuilt_capacity - 1;
            }
        }
    }
    else
    {
        /* Not rebuilding yet, add on to what needs to get rebuilt, but not what has already been rebuilt.
         * This will be 0% rebuilt. 
         */
        rebuilt_capacity = 0;
    }
    if (per_disk_capacity != 0)
    {
        /* Compute the rebuilt percentage.
         */
        percent_rebuilt = (fbe_u32_t)((rebuilt_capacity * 100) / per_disk_capacity);

        /* If the percent is greater than 100 something went wrong.
         */
        if (percent_rebuilt > 100)
        {
            /* Generate an error trace and change it to 100 to recover from the
             * error.
             */
            fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "raid_group: RB is rebuilt pos: %d percent: %d per disk chkpt: 0x%llx cap: 0x%llx bad\n",
                                  position_to_check, percent_rebuilt, (unsigned long long)rebuild_checkpoint_for_position,
                                  (unsigned long long)per_disk_capacity);
            percent_rebuilt = 100;
        }

        /* Trace some information if enabled.
         */
        fbe_raid_group_trace(raid_group_p,
                             FBE_TRACE_LEVEL_INFO,
                             FBE_RAID_GROUP_DEBUG_FLAG_REBUILD_NOTIFICATION_TRACING,
                             "raid_group: RB is rebuilt pos: %d percent: %d per disk chkpt: 0x%llx cap: 0x%llx\n",
                             position_to_check, percent_rebuilt, (unsigned long long)rebuild_checkpoint_for_position,
                             (unsigned long long)per_disk_capacity);
    }
    else
    {
        /* Did not find anything needing to be rebuilt, just say we are at 100%.
         */
        percent_rebuilt = 100;

        /* Trace some information if enabled.
         */
        fbe_raid_group_trace(raid_group_p,
                             FBE_TRACE_LEVEL_INFO,
                             FBE_RAID_GROUP_DEBUG_FLAG_REBUILD_NOTIFICATION_TRACING,
                             "raid_group: RB is rebuilt pos: %d percent: %d per disk chkpt: 0x%llx cap: 0x%llx\n",
                             position_to_check, percent_rebuilt, (unsigned long long)rebuild_checkpoint_for_position,
                             (unsigned long long)per_disk_capacity);
    }
    
    /* Simply return if fully rebuilt or not.
     */
    if (percent_rebuilt >= 100)
    {
        return FBE_TRUE;
    }
    return FBE_FALSE;
}
/******************************************************************************
 * end fbe_raid_group_is_position_fully_rebuilt()
 ******************************************************************************/

/*!***************************************************************************
 *          fbe_raid_group_get_percent_consumed_space_rebuilt_for_position()
 *****************************************************************************
 *
 * @brief   Determine the percentage of the `consumed' (i.e. non-metadata)
 *          space that has been rebuilt.
 *
 * @param   raid_group_p      - pointer to the raid group
 * @param   position_to_check - Position to check
 *
 * @return  percent_complete
 *
 * @note    The rebuild checkpoint is per-disk (i.e. physical).
 *
 * @author
 *  11/19/2012  Ron Proulx  - Created from fbe_raid_group_get_percent_rebuilt
 *
 *****************************************************************************/
static fbe_u32_t fbe_raid_group_get_percent_user_space_rebuilt_for_position(fbe_raid_group_t *raid_group_p,
                                                                         fbe_u32_t position_to_check)
{
    fbe_bool_t                          b_is_fully_rebuilt = FBE_FALSE;
    fbe_u32_t                           percent_rebuilt = 100;
    fbe_raid_group_metadata_positions_t raid_group_metadata_positions;
    fbe_raid_geometry_t                *raid_geometry_p = fbe_raid_group_get_raid_geometry(raid_group_p);
    fbe_u32_t                           rebuild_index = FBE_RAID_GROUP_INVALID_INDEX;
    fbe_u16_t                           num_data_disks = 0;
    fbe_lba_t                           rebuild_checkpoint_for_position = 0;
    fbe_lba_t                           exported_per_disk_capacity = 0;
    fbe_lba_t                           metadata_start_lba = 0;
    fbe_lba_t                           per_disk_capacity = 0;
    fbe_raid_position_bitmask_t         rl_bitmask;
    fbe_lba_t                           rebuilt_capacity = 0;

    /* Get the metadata positions.
     */
    fbe_raid_group_get_metadata_positions(raid_group_p, &raid_group_metadata_positions);

    /* Get the number of data disks.
     */
    fbe_raid_geometry_get_data_disks(raid_geometry_p, &num_data_disks);

    /* Get the per disk exported capacity.  This assumes that the user space
     * is at the lowest address.
     */
    exported_per_disk_capacity = raid_group_metadata_positions.exported_capacity / num_data_disks;

    /* Get the per disk metadata lba and capacity.
     */
    metadata_start_lba = raid_group_metadata_positions.paged_metadata_lba / num_data_disks;

    /* The per-disk capacity is considered the user capacity.
     */
    per_disk_capacity = exported_per_disk_capacity;

    /* Only report `fully rebuilt' if it is `fully rebuilt.
     */
    b_is_fully_rebuilt = fbe_raid_group_is_position_fully_rebuilt(raid_group_p, position_to_check);
    if (b_is_fully_rebuilt)
    {
        /* Trace some information if enabled.
         */
        percent_rebuilt = 100;
        fbe_raid_group_trace(raid_group_p,
                             FBE_TRACE_LEVEL_INFO,
                             FBE_RAID_GROUP_DEBUG_FLAG_REBUILD_NOTIFICATION_TRACING,
                             "raid_group: RB pos: %d fully rebuilt percent: %d per disk chkpt: 0x%llx cap: 0x%llx\n",
                             position_to_check, percent_rebuilt, (unsigned long long)rebuild_checkpoint_for_position,
                             (unsigned long long)per_disk_capacity);
        return percent_rebuilt;
    }

    /* Get the number of blocks rebuilt
     */
    fbe_raid_group_rebuild_find_existing_checkpoint_entry_for_position(raid_group_p, 
                                                                       position_to_check, 
                                                                       &rebuild_index);
    if ((rebuild_index == FBE_RAID_GROUP_INVALID_INDEX)         ||
        (rebuild_index >= FBE_RAID_GROUP_MAX_REBUILD_POSITIONS)    )
    {
        /* This is unexpected.  Trace an error and return 0 for the percent
         */
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "raid_group: RB: could not locate rebuild index for position: %d \n",
                              position_to_check);
        return 0;
    }

    /* The `blocks_rebuilt' value represents the number of physical blocks that
     * have been rebuilt in user space.  There the rebuild checkpoint for this
     * position is simply 0 plus the number of blocks rebuilt.
     */
    rebuild_checkpoint_for_position = 0 + raid_group_p->raid_group_metadata_memory.blocks_rebuilt[rebuild_index];

    /* Get the rebuild logging bitmask.
     */ 
    fbe_raid_group_get_rb_logging_bitmask(raid_group_p, &rl_bitmask);

    /* Validate that this position is not rebuild logging.
     */
    if ((1 << position_to_check) & (~rl_bitmask))
    {
        /*! @note The `blocks_rebuilt' calculation now takes care of the (3) transitions:
         *          o Initial (0) checkpoint to metadata start
         *          o Journal rebuild
         *          o Journal rebuild complete
         */
        rebuilt_capacity = rebuild_checkpoint_for_position;
    }
    else
    {
        /* Not rebuilding yet, add on to what needs to get rebuilt, but not what has already been rebuilt.
         * This will be 0% rebuilt. 
         */
        rebuilt_capacity = 0;
    }
    if (per_disk_capacity != 0)
    {
        /* Compute the rebuilt percentage.
         */
        percent_rebuilt = (fbe_u32_t)((rebuilt_capacity * 100) / per_disk_capacity);

        /* If the percent is greater than 100 something went wrong.
         */
        if (percent_rebuilt > 100)
        {
            /* Generate an error trace and change it to 100 to recover from the
             * error.
             */
            fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "raid_group: RB pos: %d percent: %d per disk chkpt: 0x%llx cap: 0x%llx bad\n",
                                  position_to_check, percent_rebuilt, (unsigned long long)rebuild_checkpoint_for_position,
                                  (unsigned long long)per_disk_capacity);
            percent_rebuilt = 100;
        }

        /* Trace some information if enabled.
         */
        fbe_raid_group_trace(raid_group_p,
                             FBE_TRACE_LEVEL_INFO,
                             FBE_RAID_GROUP_DEBUG_FLAG_REBUILD_NOTIFICATION_TRACING,
                             "raid_group: RB pos: %d percent: %d per disk chkpt: 0x%llx cap: 0x%llx \n",
                             position_to_check, percent_rebuilt, (unsigned long long)rebuild_checkpoint_for_position,
                             (unsigned long long)per_disk_capacity);
    }
    else
    {
        /* Did not find anything needing to be rebuilt, just say we are at 99%.
         */
        percent_rebuilt = 99;

        /* Trace some information if enabled.
         */
        fbe_raid_group_trace(raid_group_p,
                             FBE_TRACE_LEVEL_INFO,
                             FBE_RAID_GROUP_DEBUG_FLAG_REBUILD_NOTIFICATION_TRACING,
                             "raid_group: RB pos: %d percent: %d per disk chkpt: 0x%llx cap: 0x%llx meta: 0x%llx \n",
                             position_to_check, percent_rebuilt, (unsigned long long)rebuild_checkpoint_for_position,
                             (unsigned long long)per_disk_capacity, (unsigned long long)metadata_start_lba);
    }
    
    /* Wait until the checkpoint is set before reporting 100%
     */
    if (percent_rebuilt >= 100)
    {
        percent_rebuilt = 99;
    }
    return percent_rebuilt;
}
/******************************************************************************
 * end fbe_raid_group_get_percent_user_space_rebuilt_for_position()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_raid_group_rebuild_get_next_marked_chunk()
 ******************************************************************************
 * @brief
 *   This function will read the chunk info for a range of chunks.  It will 
 *   determine if the range is inside or outside of the user data area.  Then it 
 *   will read the chunk info from either the paged metadata or the non-paged 
 *   metadata accordingly.
 *
 * @param in_raid_group_p           - pointer to a raid group object
 * @param in_packet_p               - pointer to the packet
 * @param in_start_chunk_index      - index of the first chunk to read 
 * @param in_chunk_count            - number of chunks to read
 *
 * @return fbe_status_t  
 *
 * @author
 *   07/20/2010 - Created. Jean Montes.
 *
 ******************************************************************************/
fbe_status_t fbe_raid_group_rebuild_get_next_marked_chunk(
                                        fbe_raid_group_t*                   in_raid_group_p,
                                        fbe_packet_t*                       in_packet_p,
                                        fbe_chunk_index_t                   in_start_chunk_index, 
                                        fbe_chunk_count_t                   in_chunk_count,
                                        fbe_raid_position_bitmask_t*         positions_to_rebuild_p)
{
    fbe_bool_t                          is_in_data_area_b;                  // true if chunk is in the data area
    fbe_status_t                        status;                             // fbe status


    //  Determine if the chunk(s) to be read are in the user data area or the paged metadata area.  If we want
    //  to read a chunk in the user area, we use the paged metadata service to do that.  If we want to read a 
    //  chunk in the paged metadata area itself, we use the nonpaged metadata to do that (in the metadata data
    //  portion of it).
    status = fbe_raid_group_bitmap_determine_if_chunks_in_data_area(in_raid_group_p, in_start_chunk_index,
                                                                    in_chunk_count, &is_in_data_area_b);

    //  Check for an error on the call, which means there is an issue with the chunks.  If so, we will complete 
    //  the packet and return.  
    if (status != FBE_STATUS_OK)
    {
        fbe_transport_set_status(in_packet_p, status, 0);
        fbe_transport_complete_packet(in_packet_p);
        return FBE_STATUS_OK;
    }

    //  If the chunk is not in the data area, then use the non-paged metadata service to read it
    if (is_in_data_area_b == FBE_FALSE)
    {
        fbe_raid_group_bitmap_read_chunk_info_using_nonpaged(in_raid_group_p, in_packet_p, in_start_chunk_index,
                                                             in_chunk_count);
        return FBE_STATUS_OK;
    }

    //  The chunks are in the user data area.  Use the paged metadata service to read them. 
    fbe_raid_group_bitmap_get_next_marked_paged_metadata(in_raid_group_p, in_packet_p, in_start_chunk_index, in_chunk_count,
                                                         fbe_raid_group_get_next_chunk_marked_for_rebuild,
                                                         (void*)positions_to_rebuild_p);

    //  Return success
    return FBE_STATUS_OK;

} 
/******************************************************************************
 * end fbe_raid_group_rebuild_get_next_marked_chunk()
 ******************************************************************************/

/*!****************************************************************************
 *  fbe_raid_group_rebuild_set_iots_status()
 ******************************************************************************
 * @brief
 *  This function just sets the iots status and cleanup the iots
 *
 * @return fbe_status_t.        - status of the operation.
 *
 * @author
 *   03/19/2012 - Created. from
 *                fbe_raid_group_bitmap_get_paged_metadata_completion
 *
 ******************************************************************************/
fbe_status_t
fbe_raid_group_rebuild_set_iots_status(fbe_raid_group_t*    raid_group_p,
                                       fbe_packet_t*        packet_p,
                                       fbe_raid_iots_t*     iots_p,
                                       fbe_packet_completion_context_t   context,
                                       fbe_payload_block_operation_status_t   block_status,
                                       fbe_payload_block_operation_qualifier_t block_qualifier)
                                          
{
    fbe_raid_iots_set_block_status(iots_p, block_status);
    fbe_raid_iots_set_block_qualifier(iots_p, block_qualifier);
    fbe_raid_group_add_to_terminator_queue(raid_group_p, packet_p);
    fbe_raid_group_iots_cleanup(packet_p, context);
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;

}
/******************************************************************************
 * end fbe_raid_group_rebuild_set_iots_status()
 ******************************************************************************/

/*!****************************************************************************
 *  fbe_raid_group_get_next_chunk_marked_for_rebuild()
 ******************************************************************************
 * @brief
 *  This function will go over the search data and find out if the chunk
 *  is marked for rebuild or not
 * 
 * @param search_data -       pointer to the search data
 * @param search_size  -      search data size
 * @param raid_group_p  -     pointer to the raid group object
 *
 * @return fbe_status_t.
 *
 * @author
 *  03/26/2012 - Created. Ashwin Tamilarasan
 *
 ******************************************************************************/
fbe_bool_t fbe_raid_group_get_next_chunk_marked_for_rebuild(fbe_u8_t*     search_data,
                                                            fbe_u32_t    search_size,
                                                            context_t    context)
                                           
{
    fbe_raid_position_bitmask_t*             positions_to_rebuild_p = NULL;
    fbe_raid_group_paged_metadata_t          paged_metadata;
    fbe_u8_t*                                source_ptr = NULL;
    fbe_bool_t                               is_chunk_marked_for_processing = FBE_FALSE;
    fbe_u32_t                                index;

    positions_to_rebuild_p = (fbe_raid_position_bitmask_t*)context;
    source_ptr = search_data;

    for(index = 0; index < search_size; index += sizeof(fbe_raid_group_paged_metadata_t))
    {
        fbe_copy_memory(&paged_metadata, source_ptr, sizeof(fbe_raid_group_paged_metadata_t));
        source_ptr += sizeof(fbe_raid_group_paged_metadata_t);

        if(paged_metadata.needs_rebuild_bits & *positions_to_rebuild_p)
        {
            is_chunk_marked_for_processing = FBE_TRUE;
            break;
        }
    }

    return is_chunk_marked_for_processing;
}

/*******************************************************
 * end fbe_raid_group_get_next_chunk_marked_for_rebuild()
 ******************************************************/

/*!****************************************************************************
 * fbe_raid_group_quiesce_for_bg_op()
 ******************************************************************************
 * @brief
 *   Start a quiesce for a raw mirror that is undergoing a background op.
 * 
 * @param raid_group_p - pointer to a raid group object
 *
 * @return  fbe_status_t  
 *
 * @author
 *  6/18/2012 - Created. Rob Foley
 *
 ******************************************************************************/
fbe_status_t fbe_raid_group_quiesce_for_bg_op(fbe_raid_group_t *raid_group_p)
{
    fbe_bool_t  peer_alive_b;
    fbe_bool_t  local_quiesced;
    fbe_bool_t  peer_quiesced;

    fbe_raid_group_lock(raid_group_p);
    peer_alive_b = fbe_base_config_has_peer_joined((fbe_base_config_t *) raid_group_p);
    local_quiesced = fbe_base_config_is_clustered_flag_set((fbe_base_config_t *) raid_group_p, 
                                                           FBE_BASE_CONFIG_CLUSTERED_FLAG_QUIESCED);
    peer_quiesced = fbe_base_config_is_any_peer_clustered_flag_set((fbe_base_config_t *) raid_group_p, 
                                                               (FBE_BASE_CONFIG_CLUSTERED_FLAG_QUIESCED|
                                                                FBE_BASE_CONFIG_METADATA_MEMORY_QUIESCE_COMPLETE));
    /* Return OK if we are quiesced in single SP mode, or if both SPs are quiesced.
     */
    if ((local_quiesced == FBE_TRUE) &&
        ((peer_alive_b != FBE_TRUE) || (peer_quiesced == FBE_TRUE)))
    {
        fbe_raid_group_unlock(raid_group_p);
        return FBE_STATUS_OK; 
    }

    fbe_raid_group_unlock(raid_group_p);

    fbe_base_object_trace((fbe_base_object_t *)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s set quiesce/unquiesce conditions\n", __FUNCTION__);

    /* Since we are doing background operations quiesced, set the quiesce, unquiesce and bg ops conditions. 
     * Once we're done, we'll automatically unquiesce. 
     */
    fbe_lifecycle_set_cond(&fbe_raid_group_lifecycle_const, (fbe_base_object_t*) raid_group_p,
                           FBE_BASE_CONFIG_LIFECYCLE_COND_QUIESCE);
    fbe_lifecycle_set_cond(&fbe_raid_group_lifecycle_const, (fbe_base_object_t*) raid_group_p,
                           FBE_BASE_CONFIG_LIFECYCLE_COND_UNQUIESCE);
    fbe_lifecycle_set_cond(&fbe_raid_group_lifecycle_const, (fbe_base_object_t*) raid_group_p,
                           FBE_RAID_GROUP_LIFECYCLE_COND_QUIESCED_BACKGROUND_OPERATION);

    return FBE_STATUS_PENDING; 
}
/**************************************
 * end fbe_raid_group_quiesce_for_bg_op()
 **************************************/

/*!***************************************************************************
 *          fbe_raid_group_get_raid10_degraded_info()
 ***************************************************************************** 
 * 
 * @brief   Get the degraded information for a RAID-10 (striped mirror).
 *          The following values are populated:
 *              o Rebuild percent - Smallest rebuilt percent for mirror raid groups
 *              o Is Degraded - If any of the downstream mirror raid groups are
 *                  degraded then the RAID-10 is considered degraded.
 *
 * @param   packet_p   - pointer to packet since we need to to send to downstream objects
 * @param   raid_group_p - pointer to the raid group object
 * @param   degraded_info_p - Pointer to degraded info to populate.
 *
 * @return fbe_status_t
 * 
 * @author
 *  09/06/2012  Ron Proulx  - Created.
 *****************************************************************************/
fbe_status_t fbe_raid_group_get_raid10_degraded_info(fbe_packet_t *packet_p,
                                                     fbe_raid_group_t *raid_group_p,
                                                     fbe_raid_group_get_degraded_info_t *degraded_info_p)
{

    fbe_payload_ex_t                   *payload_p;
    fbe_payload_control_operation_t    *control_operation_p;
    fbe_raid_group_get_degraded_info_t  raid_group_degraded_info;  
    fbe_block_edge_t                   *block_edge_p = NULL;
    fbe_path_state_t                    path_state = FBE_PATH_STATE_INVALID;
    fbe_status_t                        status;
    fbe_u32_t                           mirror_index;
    fbe_u32_t                           width;

    /* Get the control op buffer data pointer from the packet payload.*/
    payload_p = fbe_transport_get_payload_ex(packet_p);
    if (payload_p == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s payload is NULL\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Initialize the return buffer to default values.
     */
    degraded_info_p->rebuild_percent = 100;
    degraded_info_p->b_is_raid_group_degraded = FBE_FALSE;

    /* For each downstream edge get the edge info.
     */
    fbe_base_config_get_width((fbe_base_config_t*)raid_group_p, &width);
    for (mirror_index = 0; mirror_index < width; mirror_index++)
    {
        /*find the object id of our mirror server*/
        fbe_base_config_get_block_edge((fbe_base_config_t*)raid_group_p, &block_edge_p, mirror_index);
        if (block_edge_p == NULL)
        {
            /*! @note This is the striper for a RAID-10 if ANY of the downstream 
             *        edges are missing it means we are shutdown.  Since we allow
             *        this method to be invoked on a raid group that is broken
             *        generate a warning trace and return appropriate information.
             */
            fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                                  FBE_TRACE_LEVEL_WARNING,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "raid_group: get RAID-10 degraded info mirror index: %d is gone \n",
                                  mirror_index);
            degraded_info_p->rebuild_percent = 0;
            degraded_info_p->b_is_raid_group_degraded = FBE_TRUE;
            return FBE_STATUS_OK;/*no edge to process*/
        }
    
        /* If the path is currently not enabled we cannot set the usurper.
         */
        fbe_block_transport_get_path_state (block_edge_p, &path_state);
        if ((path_state != FBE_PATH_STATE_ENABLED) &&
            (path_state != FBE_PATH_STATE_SLUMBER)    )
        {
            fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                                  FBE_TRACE_LEVEL_WARNING,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "raid_group: get RAID-10 degraded info mirror index: %d path state: %d\n",
                                  mirror_index, path_state);
            degraded_info_p->rebuild_percent = 0;
            degraded_info_p->b_is_raid_group_degraded = FBE_TRUE;
            return FBE_STATUS_OK;/*no edge to process*/
        }
        
        /* Set packet address.*/
        fbe_transport_set_address(packet_p,
                                  FBE_PACKAGE_ID_SEP_0,
                                  FBE_SERVICE_ID_TOPOLOGY,
                                  FBE_CLASS_ID_INVALID,
                                  block_edge_p->base_edge.server_id); 
    
        /* allocate the new control operation*/
        control_operation_p = fbe_payload_ex_allocate_control_operation(payload_p);
        fbe_payload_control_build_operation(control_operation_p,
                                            FBE_RAID_GROUP_CONTROL_CODE_GET_DEGRADED_INFO,
                                            &raid_group_degraded_info,
                                            sizeof(fbe_raid_group_get_degraded_info_t));
    
        fbe_payload_ex_increment_control_operation_level(payload_p);
        fbe_transport_set_sync_completion_type(packet_p, FBE_TRANSPORT_COMPLETION_TYPE_MORE_PROCESSING_REQUIRED);
    
        /*send it*/
        fbe_block_transport_send_control_packet(block_edge_p, packet_p);
    
        fbe_transport_wait_completion(packet_p);
    
        fbe_payload_ex_release_control_operation(payload_p, control_operation_p);
    
        /*when we get here the data is there for us to use(in case the packet completed nicely*/
        status = fbe_transport_get_status_code(packet_p);
        if (status != FBE_STATUS_OK) 
        {
            /* If the usurper failed return the failure status.
             */
            fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                                  FBE_TRACE_LEVEL_WARNING,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "raid_group: get RAID-10 degraded info mirror index: %d get degraded failed. status: 0x%x\n",
                                  mirror_index, status);
            degraded_info_p->rebuild_percent = 0;
            degraded_info_p->b_is_raid_group_degraded = FBE_TRUE;
            return status;
        }

        /* Now take the minimum rebuilt percent.
         */
        if (raid_group_degraded_info.rebuild_percent < degraded_info_p->rebuild_percent)
        {
            /* Use the smaller value.
             */
            degraded_info_p->rebuild_percent = raid_group_degraded_info.rebuild_percent;
        }

        /* If the mirror raid group is degraded then the striped mirror is
         * degraded.
         */
        if (raid_group_degraded_info.b_is_raid_group_degraded == FBE_TRUE)
        {
            /* Flag striped mirror as degraded.
             */
            degraded_info_p->b_is_raid_group_degraded = FBE_TRUE;
        }

    } /* end for all mirror positions */

    /* If we made it this far it is success.
     */
    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_raid_group_get_raid10_degraded_info()
 *****************************************************************************/

/*!****************************************************************************
 *          fbe_raid_group_get_id_of_pvd_being_worked_on()
 ******************************************************************************
 *
 * @brief   Get the ID of the PVD that is being rebuilt or copied into
 *
 * @param   raid_group_p - Pointer to raid group object
 * @param   packet_p  - pointer to a monitor packet
 * @param   downstream_edge_p - who to send it to
 * @param   pvd_id -  id of the drive being rebuilt or copied into.
 * 
 * @return fbe_status_t 
 *
 * @author
 *  03/20/2013  Shay Harel
 *
 ******************************************************************************/
fbe_status_t fbe_raid_group_get_id_of_pvd_being_worked_on(fbe_raid_group_t *raid_group_p,
                                                                 fbe_packet_t *packet_p,
                                                                 fbe_block_edge_t *downstream_edge_p,
                                                                 fbe_object_id_t *pvd_id)
{
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_payload_control_operation_t    *control_operation = NULL;
    fbe_payload_ex_t                   *payload = NULL;
    fbe_virtual_drive_get_info_t        virtual_drive_info;
    fbe_payload_control_status_t        control_status;

    /* Initialize the return value to invalid.
     */
    *pvd_id = FBE_OBJECT_ID_INVALID;

    /* System RGs are directly connected to PVD. The server id is the 
     * PVD id here.
     */
    if(fbe_database_is_object_system_rg(raid_group_p->base_config.base_object.object_id))
    {
        *pvd_id = downstream_edge_p->base_edge.server_id;

        return FBE_STATUS_OK;
    }

    /* Validate that the downstream edge is enabled.  The rebuild condition should
     * not run unless the destination is healthy but it can go away at any time.
     */
    if (downstream_edge_p->base_edge.path_state != FBE_PATH_STATE_ENABLED)
    {
        /* This is a warning since it can occur.
         */
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s downstream edge state: %d is not ENABLED\n",
                              __FUNCTION__, downstream_edge_p->base_edge.path_state);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    payload = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_allocate_control_operation(payload); 
    if (control_operation == NULL) 
    {    
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s Failed to allocate control operation\n",
                              __FUNCTION__);

        return FBE_STATUS_GENERIC_FAILURE;    
    }

    /* Set packet address.*/
    fbe_transport_set_address(packet_p,
                              FBE_PACKAGE_ID_SEP_0,
                              FBE_SERVICE_ID_TOPOLOGY,
                              FBE_CLASS_ID_INVALID,
                              downstream_edge_p->base_edge.server_id); 

    fbe_payload_control_build_operation(control_operation,  
                                        FBE_VIRTUAL_DRIVE_CONTROL_CODE_GET_INFO,  
                                        &virtual_drive_info, 
                                        sizeof(fbe_virtual_drive_get_info_t)); 

    fbe_transport_set_sync_completion_type(packet_p, FBE_TRANSPORT_COMPLETION_TYPE_MORE_PROCESSING_REQUIRED);

    fbe_block_transport_send_control_packet(downstream_edge_p, packet_p);  
    
    fbe_transport_wait_completion(packet_p);

    status = fbe_transport_get_status_code(packet_p);
    fbe_payload_control_get_status(control_operation, &control_status);
    fbe_payload_ex_release_control_operation(payload, control_operation);

    /* The base edge might have been populated when we sent the packet
     * from RAID to VD object. Clear out the edge data in packet so
     * that it does not get reused in the monitor context    
     */
    packet_p->base_edge = NULL;

    /*any chance this drive just can't satisy the time we give it to spin up, or simply does not suppot power save*/
    if ((control_status != FBE_PAYLOAD_CONTROL_STATUS_OK) ||(status != FBE_STATUS_OK)) 
    {
         fbe_base_object_trace((fbe_base_object_t *)raid_group_p, 
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s control status: 0x%x status: 0x%x\n",
                               __FUNCTION__, control_status, status);

        return FBE_STATUS_GENERIC_FAILURE; 
    }

    /*! @note Due to the fact that the virtual drive could be in the middle of
     *        a copy operation, first check if the original object id is valid
     *        or not.  If the original is not valid check the spare object id.
     *        If neither is valid return an error.
     */
    if (virtual_drive_info.original_pvd_object_id != FBE_OBJECT_ID_INVALID)
    {
        *pvd_id = virtual_drive_info.original_pvd_object_id;
    }
    else if (virtual_drive_info.spare_pvd_object_id != FBE_OBJECT_ID_INVALID)
    {
        *pvd_id = virtual_drive_info.spare_pvd_object_id;
    }
    else
    {
        /*! @note Even if the downstream pvd was removed there should be the 
         *        `old' pvd object id on the edge.  Therefore this is an
         *        error not a warning.
         */
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s pvd id invalid: 0x%x\n",
                               __FUNCTION__, *pvd_id);

        return FBE_STATUS_GENERIC_FAILURE; 
    }

    return FBE_STATUS_OK;

}
/******************************************************************************
 * end fbe_raid_group_get_id_of_pvd_being_worked_on()
 ******************************************************************************/

/*!***************************************************************************
 *          fbe_raid_group_rebuild_update_pvd_percent_rebuilt()
 *****************************************************************************
 *
 * @brief   If the percent rebuilt of a position has changed send a usurper
 *          to the provision drive being rebuilt to update the percent rebuilt
 *          for that position.  This is needed so that a `mega' poll will get
 *          the updated percentage rebuilt.
 *
 * @param   raid_group_p - Pointer to raid group object
 * @param   packet_p - Pointer to monitor packet to use for request
 * @param   downstream_edge_p - Downstream edge pointer
 * @param   pvd_object_id - Provision drive object id of pvd to update
 * @param   percent_rebuilt - The percent rebuilt for this position
 * 
 * @return  fbe_status_t 
 *
 * @author
 *  03/20/2013  Ron Proulx  - Created.
 *
 *****************************************************************************/
static fbe_status_t fbe_raid_group_rebuild_update_pvd_percent_rebuilt(fbe_raid_group_t *raid_group_p,
                                                                      fbe_packet_t *packet_p,
                                                                      fbe_block_edge_t *downstream_edge_p,
                                                                      fbe_object_id_t pvd_object_id,
                                                                      fbe_u32_t percent_rebuilt)
{
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_payload_control_operation_t    *control_operation = NULL;
    fbe_payload_ex_t                   *payload = NULL;
    fbe_payload_control_status_t        control_status;
    fbe_provision_drive_control_set_percent_rebuilt_t set_percent_rebuilt;

    /* Validate that the downstream edge is enabled.  The rebuild condition should
     * not run unless the destination is healthy but it can go away at any time.
     */
    if (downstream_edge_p->base_edge.path_state != FBE_PATH_STATE_ENABLED)
    {
        /* This is a warning since it can occur.
         */
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s downstream edge state: %d is not ENABLED\n",
                              __FUNCTION__, downstream_edge_p->base_edge.path_state);
        return FBE_STATUS_EDGE_NOT_ENABLED;
    }

    /* Allocate a control operation from the monitor packet.
     */
    payload = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_allocate_control_operation(payload); 
    if (control_operation == NULL) 
    {    
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s Failed to allocate control operation\n",
                              __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;    
    }

    /* Set packet address.*/
    fbe_transport_set_address(packet_p,
                              FBE_PACKAGE_ID_SEP_0,
                              FBE_SERVICE_ID_TOPOLOGY,
                              FBE_CLASS_ID_INVALID,
                              pvd_object_id); 

    /* Build the control operation*/
    fbe_zero_memory(&set_percent_rebuilt, sizeof(fbe_provision_drive_control_set_percent_rebuilt_t));
    fbe_payload_control_build_operation(control_operation,  
                                        FBE_PROVISION_DRIVE_CONTROL_CODE_SET_PERCENT_REBUILT,  
                                        &set_percent_rebuilt, 
                                        sizeof(fbe_provision_drive_control_set_percent_rebuilt_t));
    set_percent_rebuilt.percent_rebuilt = percent_rebuilt;

    fbe_transport_set_packet_attr(packet_p, FBE_PACKET_FLAG_TRAVERSE);
    fbe_transport_set_sync_completion_type(packet_p, FBE_TRANSPORT_COMPLETION_TYPE_MORE_PROCESSING_REQUIRED);

    fbe_block_transport_send_control_packet(downstream_edge_p, packet_p);  
    
    fbe_transport_wait_completion(packet_p);

    status = fbe_transport_get_status_code(packet_p);
    fbe_payload_control_get_status(control_operation, &control_status);
    fbe_payload_ex_release_control_operation(payload, control_operation);

    /* The base edge might have been populated when we sent the packet
     * from RAID to VD object. Clear out the edge data in packet so
     * that it does not get reused in the monitor context    
     */
    fbe_transport_clear_packet_attr(packet_p, FBE_PACKET_FLAG_TRAVERSE);
    packet_p->base_edge = NULL;

    /*any chance this drive just can't satisy the time we give it to spin up, or simply does not suppot power save*/
    if ((control_status != FBE_PAYLOAD_CONTROL_STATUS_OK) ||
        (status != FBE_STATUS_OK)                            ) 
    {
         fbe_base_object_trace((fbe_base_object_t *)raid_group_p, 
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s control status: 0x%x status: 0x%x\n",
                               __FUNCTION__, control_status, status);
        if (status == FBE_STATUS_OK)
        {
            return FBE_STATUS_GENERIC_FAILURE;
        }
        return status;
    }

    return FBE_STATUS_OK;

}
/******************************************************************************
 * end fbe_raid_group_rebuild_update_pvd_percent_rebuilt()
 ******************************************************************************/

/*!***************************************************************************
 *          fbe_raid_group_data_reconstruction_progress_send_start_end_notification()
 *****************************************************************************
 *
 * @brief   This function generates the START or END reconstruct notification.  
 *
 * @param   raid_group_p - Pointer to raid group being worked on
 * @param   packet_p - Monitor packet pointer
 * @param   downstream_edge_p - Downstream edge pointer
 * @param   opcode - The opcode (only START or END allowed) for the notification
 * @param   rebuild_index_for_notification - The rebuild index for the notification
 * @param   b_clear_percent_rebuilt - FBE_TRUE - Clear the `previous percent
 *              rebuilt' in the raid group object.
 *
 * @return  status
 *
 *****************************************************************************/
static fbe_status_t fbe_raid_group_data_reconstruction_progress_send_start_end_notification(fbe_raid_group_t *raid_group_p,
                                                                                    fbe_packet_t *packet_p,
                                                                                    fbe_block_edge_t *downstream_edge_p,
                                                                                    fbe_raid_group_data_reconstrcution_notification_op_t opcode,
                                                                                    fbe_u32_t rebuild_index_for_notification,
                                                                                    fbe_bool_t b_clear_percent_rebuilt)
{
    fbe_status_t            status;
    fbe_notification_info_t notify_info;
    fbe_trace_level_t       error_trace_level = FBE_TRACE_LEVEL_ERROR;
    fbe_raid_group_nonpaged_metadata_t *nonpaged_metadata_p = NULL;
    fbe_raid_group_rebuild_checkpoint_info_t *rb_info_p = NULL;
    fbe_u32_t               other_rebuild_index;

    /* Get the position information.
     */
    fbe_base_config_get_nonpaged_metadata_ptr((fbe_base_config_t*)raid_group_p, (void **)&nonpaged_metadata_p);
    rb_info_p = &nonpaged_metadata_p->rebuild_info.rebuild_checkpoint_info[rebuild_index_for_notification];

    /* If this is a start notification and the blocks_rebuilt is not 0.
     * It means that the SP was rebooted.  Leave blocks_rebuilt as is
     * and don't generate the start notification.
     */
    if ((opcode == DATA_RECONSTRUCTION_START)                                                          &&
        (raid_group_p->raid_group_metadata_memory.blocks_rebuilt[rebuild_index_for_notification] != 0)    )
    {
        /* Generate an informational trace and return success.
         */
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "raid_group: send start index: %d pvd: 0x%x percent: %d blocks rebuilt: 0x%llx - reboot\n",
                              rebuild_index_for_notification, 
                              raid_group_p->rebuilt_pvds[rebuild_index_for_notification],
                              raid_group_p->previous_rebuild_percent[rebuild_index_for_notification],
                              (unsigned long long)raid_group_p->raid_group_metadata_memory.blocks_rebuilt[rebuild_index_for_notification]);
        return FBE_STATUS_OK;
    }

    /* If the object id is active send the START/END notification.
     */
    if (raid_group_p->rebuilt_pvds[rebuild_index_for_notification] != FBE_OBJECT_ID_INVALID)
    {
        /* First update the `percent rebuilt' in the associated pvd.  The use cases
         * are:
         *  o This is a start notification so set the percent rebuilt to 1.  
         *    This needs to set to 1 since percent rebuild of 0 means "not rebuilding". 
         *    This will handle the case where we have a glitch in the system or SLF that
         *    causes Admin to wipe out the previous information after a Mega Poll. 
         *
         *  o This is a end notification so the rebuild is complete, set the percent
         *    rebuilt to 0 (by default, not rebuilding).
         */
        status = fbe_raid_group_rebuild_update_pvd_percent_rebuilt(raid_group_p,
                                                                   packet_p,
                                                                   downstream_edge_p,
                                                                   raid_group_p->rebuilt_pvds[rebuild_index_for_notification],
                                                                   ((opcode == DATA_RECONSTRUCTION_START) ? 1 : 0));
        if (status != FBE_STATUS_OK)
        {
            /*! @note If we cannot update the downstream pvd rebuilt percent
             *        it most likely means that the drive was removed. In any 
             *        case reset this rebuild index to `not rebuilding'.
             */
            b_clear_percent_rebuilt = FBE_TRUE;
            error_trace_level = (status == FBE_STATUS_EDGE_NOT_ENABLED) ? FBE_TRACE_LEVEL_WARNING : FBE_TRACE_LEVEL_ERROR;
            fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                                  error_trace_level,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "raid_group: send start end update pvd percent rebuilt for: %d or pvd obj: 0x%x percent: %d failed - status: 0x%x\n",
                                  rebuild_index_for_notification, 
                                  raid_group_p->rebuilt_pvds[rebuild_index_for_notification],
                                  raid_group_p->previous_rebuild_percent[rebuild_index_for_notification], status);
            
            /* Now reset the associated values (drive was removed).  Unless
             * this was an END notification.  In that case send the END.
             */
            if (opcode != DATA_RECONSTRUCTION_END)
            {
                raid_group_p->rebuilt_pvds[rebuild_index_for_notification] = FBE_OBJECT_ID_INVALID;
                raid_group_p->previous_rebuild_percent[rebuild_index_for_notification] = 0;
                raid_group_p->raid_group_metadata_memory.blocks_rebuilt[rebuild_index_for_notification] = 0;

                return status;
            }
        }

        /*some rebuild was done here*/
        notify_info.notification_type = FBE_NOTIFICATION_TYPE_DATA_RECONSTRUCTION;
        notify_info.class_id = FBE_CLASS_ID_PROVISION_DRIVE;
        notify_info.object_type = FBE_TOPOLOGY_OBJECT_TYPE_PROVISIONED_DRIVE;
        notify_info.notification_data.data_reconstruction.state = opcode;
        notify_info.notification_data.data_reconstruction.percent_complete = raid_group_p->previous_rebuild_percent[rebuild_index_for_notification];

        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, 
                              FBE_TRACE_LEVEL_INFO, 
                              FBE_TRACE_MESSAGE_ID_INFO, 
                              "RB: pos: %d pvd obj: 0x%x %s \n",
                              rb_info_p->position, 
                              raid_group_p->rebuilt_pvds[rebuild_index_for_notification], 
                              opcode==DATA_RECONSTRUCTION_START ? "DATA_RECONSTRUCTION_START" : "DATA_RECONSTRUCTION_END");

        status = fbe_notification_send(raid_group_p->rebuilt_pvds[rebuild_index_for_notification], notify_info);
        if (status != FBE_STATUS_OK) 
        { 
            fbe_base_object_trace((fbe_base_object_t*)raid_group_p, 
                                  FBE_TRACE_LEVEL_WARNING, 
                                  FBE_TRACE_MESSAGE_ID_INFO, 
                                  "%s unexpected status: %d on notification send \n", __FUNCTION__, status);
        }

        if (opcode == DATA_RECONSTRUCTION_END) 
        {
            raid_group_p->rebuilt_pvds[rebuild_index_for_notification] = FBE_OBJECT_ID_INVALID;/*clear for next time*/
        }
        
    }

    /* If requested, clear the previous rebuild percent
     */
    if (b_clear_percent_rebuilt == FBE_TRUE)
    {
        raid_group_p->previous_rebuild_percent[rebuild_index_for_notification] = 0;
        raid_group_p->raid_group_metadata_memory.blocks_rebuilt[rebuild_index_for_notification] = 0;

        /* Check for special case where multiple positions rebuilt.
         */
        other_rebuild_index = (rebuild_index_for_notification == 0) ? 1 : 0;
        if ((raid_group_p->rebuilt_pvds[other_rebuild_index] == FBE_OBJECT_ID_INVALID) &&
            (raid_group_p->previous_rebuild_percent[other_rebuild_index] != 0)            )
        {
            raid_group_p->previous_rebuild_percent[other_rebuild_index] = 0;
            raid_group_p->raid_group_metadata_memory.blocks_rebuilt[other_rebuild_index] = 0;
        }
    }

    return FBE_STATUS_OK;
}
/***************************************************************************
 * end fbe_raid_group_data_reconstruction_progress_send_start_end_notification()
 ***************************************************************************/

/*!***************************************************************************
 *          fbe_raid_group_rebuild_cleanup_notify_if_required()
 ***************************************************************************** 
 * 
 * @brief   This function checks the contents of the rebuild notification data
 *          and if the raid group is not rebuilding and any of the notification
 *          data indicates that it is.  This can happen if (1) or both SPs are
 *          rebooted during a rebuild.
 *
 * @param   raid_group_p - pointer to the raid group
 * @param   packet_p - Pointer to monitor packet to execute cleanup
 * @param   b_is_rebuild_needed - If FBE_TRUE this raid group needs a rebuild
 *
 * @return  status
 *
 * @author
 *  11/19/2012  Ron Proulx  - Created.
 *
 *****************************************************************************/
static fbe_status_t fbe_raid_group_rebuild_cleanup_notify_if_required(fbe_raid_group_t *raid_group_p,
                                                                      fbe_packet_t *packet_p,
                                                                      fbe_bool_t b_is_rebuild_needed)
{
    fbe_status_t                                status = FBE_STATUS_OK;
    fbe_u32_t                                   rebuild_index;
    fbe_u32_t                                   position;
    fbe_u32_t                                   width;
    fbe_bool_t                                  b_cleanup_required = FBE_FALSE;
    fbe_bool_t                                  b_is_rebuilding = FBE_FALSE;
    fbe_raid_group_nonpaged_metadata_t         *nonpaged_metadata_p = NULL;
    fbe_raid_group_rebuild_checkpoint_info_t   *rb_info_p = NULL;
    fbe_block_edge_t                           *downstream_edge_p;
    fbe_bool_t                                  b_rebuild_index_needs_cleanup[FBE_RAID_GROUP_MAX_REBUILD_POSITIONS];
    fbe_object_id_t                             pvd_object_id;

    /* Loop through the rebuild positions and determine if that index is
     * rebuilding
     */
    for (rebuild_index = 0; rebuild_index < FBE_RAID_GROUP_MAX_REBUILD_POSITIONS; rebuild_index++)
    {
        if (raid_group_p->rebuilt_pvds[rebuild_index] != FBE_OBJECT_ID_INVALID)
        {
            b_is_rebuilding = FBE_TRUE;
        }
    }

    /* Loop through the rebuild positions to see if the rebuild checkpoint needs to be driven forward
     */
    for (rebuild_index = 0; rebuild_index < FBE_RAID_GROUP_MAX_REBUILD_POSITIONS; rebuild_index++)
    {
        /* If a rebuild is not longer needed but there is a still valid
         * rebuild notify pvd object id.
         */
        b_rebuild_index_needs_cleanup[rebuild_index] = FBE_FALSE;
        if ((b_is_rebuild_needed == FBE_FALSE)                                   &&
            (raid_group_p->rebuilt_pvds[rebuild_index] != FBE_OBJECT_ID_INVALID)    )
        {
            /* This can happen if (1) or both SPs were rebooted but it should
             * not occur often so trace a warning.
             */
            fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                                  FBE_TRACE_LEVEL_WARNING,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "raid_group: rebuild cleanup notify rb not needed: %d for index: %d pvd obj: 0x%x percent complete: %d\n",
                                  b_is_rebuild_needed, rebuild_index, raid_group_p->rebuilt_pvds[rebuild_index], 
                                  raid_group_p->previous_rebuild_percent[rebuild_index]);
            b_rebuild_index_needs_cleanup[rebuild_index] = FBE_TRUE;
            b_cleanup_required = FBE_TRUE;
        }
        else if ((b_is_rebuild_needed == FBE_TRUE)                                    &&
                 (raid_group_p->previous_rebuild_percent[rebuild_index] != 0)         &&
                 (b_is_rebuilding == FBE_FALSE)                                       &&
                 (raid_group_p->rebuilt_pvds[rebuild_index] == FBE_OBJECT_ID_INVALID)    )

        {
            /* Now determine if we need to complete (send END notification) for this
             * position for a previous rebuild that was interrupted by a reboot.
             */
            fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                                  FBE_TRACE_LEVEL_WARNING,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "raid_group: rebuild cleanup notify rb needed: %d for index: %d pvd obj: 0x%x percent complete: %d\n",
                                  b_is_rebuild_needed, rebuild_index, raid_group_p->rebuilt_pvds[rebuild_index], 
                                  raid_group_p->previous_rebuild_percent[rebuild_index]);
            b_rebuild_index_needs_cleanup[rebuild_index] = FBE_TRUE;
            b_cleanup_required = FBE_TRUE;
        }

    } /* end for all rebuilding positions */

    /* If we need to cleanup do it now.
     */
    if (b_cleanup_required)
    {
        /*  Get a pointer to the raid group object non-paged metadata  */
        fbe_raid_group_get_width(raid_group_p, &width);
        fbe_base_config_get_nonpaged_metadata_ptr((fbe_base_config_t*)raid_group_p, (void **)&nonpaged_metadata_p);
    
        /* Now determine which position is actively being rebuilt.
         */
        for (rebuild_index = 0; rebuild_index < FBE_RAID_GROUP_MAX_REBUILD_POSITIONS; rebuild_index++)
        {
            /* If this index needs to be cleanup
             */
            if (b_rebuild_index_needs_cleanup[rebuild_index] == FBE_TRUE)
            {
                /* Walk thru the raid group and determine which position needs 
                 * to be updated.
                 */
                for (position = 0; position < width; position++)
                {
                    /* Get the object id of the downstream object for this position.
                     */ 
                    fbe_base_config_get_block_edge((fbe_base_config_t*)raid_group_p, &downstream_edge_p, position);

                    /* Get the pvd object id of the active edge of the virtual drive.
                     */
                    status = fbe_raid_group_get_id_of_pvd_being_worked_on(raid_group_p, packet_p, downstream_edge_p, &pvd_object_id);
                    if (status != FBE_STATUS_OK)
                    {
                        /* If this fails it does not mean its an error.  
                         * That position could be removed.
                         */
                        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                                              FBE_TRACE_LEVEL_WARNING,
                                              FBE_TRACE_MESSAGE_ID_INFO,
                                              "%s get pvd id failed - status: 0x%x\n",
                                              __FUNCTION__, status); 
                        continue; 
                    }

                    /* Determine if this position is rebuilding or not.
                     */
                    rb_info_p = &nonpaged_metadata_p->rebuild_info.rebuild_checkpoint_info[rebuild_index];
    
                    /* If the rebuild pvd is valid and it matches this position cleanup
                     * the notification.
                     */
                    if (raid_group_p->rebuilt_pvds[rebuild_index] == pvd_object_id)
                    {
                        /* Cleanup this position
                         */
                        fbe_raid_group_data_reconstruction_progress_send_start_end_notification(raid_group_p,
                                                                                                packet_p,
                                                                                                downstream_edge_p, 
                                                                                                DATA_RECONSTRUCTION_END,
                                                                                                rebuild_index,
                                                                                                FBE_TRUE    /* Clear the rebuilt percent*/);
                    }
                    else if (rb_info_p->position != FBE_RAID_INVALID_DISK_POSITION)
                    {
                        /* We never sent the complete for this position. Send it now.
                         */
                        fbe_raid_group_data_reconstruction_progress_send_start_end_notification(raid_group_p,
                                                                                                packet_p,
                                                                                                downstream_edge_p, 
                                                                                                DATA_RECONSTRUCTION_END,
                                                                                                rebuild_index,
                                                                                                FBE_TRUE    /* Clear the rebuilt percent*/);
                    }

                } /* end for all raid group positions */

            } /* if this rebuild index needs to be cleaned up */

        } /* end for all rebuild indexes */

    } /* if cleanup is required */

    /* Return success */
    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_raid_group_rebuild_cleanup_notify_if_required()
 ******************************************************************************/

/*!***************************************************************************
 *          fbe_raid_group_get_raid10_rebuild_info_completion()
 ***************************************************************************** 
 * 
 * @brief   This function signals a rebuild complstion for a RAID-10
 *
 * @param   packet_p - Pointer to monitor packet to execute cleanup
 * @param   context - opaque semaphoare pointer
 *
 * @return  status
 *
 *****************************************************************************/
static fbe_status_t fbe_raid_group_get_raid10_rebuild_info_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_semaphore_t * sem = (fbe_semaphore_t *)context;
    fbe_semaphore_release(sem, 0, 1, FBE_FALSE);
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;/*important, otherwise we'll complete the original one*/
}
/******************************************************************************
 * end fbe_raid_group_get_raid10_rebuild_info_completion()
 ******************************************************************************/

/*!***************************************************************************
 *          fbe_raid_group_update_blocks_rebuilt()
 *****************************************************************************
 *
 * @brief   This function updates the in-memory `blocks rebuilt' for ALL
 *          positions that were involved in the last rebuild request.  Thus
 *          as long as the position is Enabled / Not Rebuild Logging we will
 *          update how many blocks have been rebuilt. 
 *
 * @param   raid_group_p - Pointer to raid group being worked on
 * @param   rebuild_start_lba - The starting lba for the rebuild.  This
 *              value is used to determine if the rebuild was restarted, is a 
 *              metadata rebuild or is a user space rebuild.
 * @param   rebuild_blocks - Number of physical blocks that where
 *              rebuilt for the last rebuild request.
 * @param   update_checkpoint_bitmask - Bitmask of positions that where updated
 *              for the last rebuild request.
 *
 * @return  status
 *
 * @author
 *  03/20/2013  Ron Proulx  - Created.
 *
 *****************************************************************************/
fbe_status_t fbe_raid_group_update_blocks_rebuilt(fbe_raid_group_t *raid_group_p,                          
                                                  fbe_lba_t rebuild_start_lba,
                                                  fbe_block_count_t rebuild_blocks,
                                                  fbe_raid_position_bitmask_t update_checkpoint_bitmask)
{
    fbe_raid_geometry_t    *raid_geometry_p; 
    fbe_u32_t               width;
    fbe_u16_t               data_disks;
    fbe_u32_t               position;
    fbe_u32_t               rebuild_index;
    fbe_lba_t               updated_checkpoint;
    fbe_time_t              last_checkpoint_time;
    fbe_u32_t               ms_since_checkpoint;
    fbe_u32_t               peer_update_period_ms;
    fbe_lba_t               exported_per_disk_capacity;
    fbe_lba_t               metadata_lba;
    fbe_chunk_size_t        chunk_size;

    /* Only update the check point if the start lba is in the user space.
     */
    updated_checkpoint = rebuild_start_lba + rebuild_blocks;
    fbe_raid_group_get_width(raid_group_p, &width);
    exported_per_disk_capacity = fbe_raid_group_get_exported_disk_capacity(raid_group_p);
    raid_geometry_p = fbe_raid_group_get_raid_geometry(raid_group_p);
    fbe_raid_geometry_get_metadata_start_lba(raid_geometry_p, &metadata_lba);
    chunk_size = fbe_raid_group_get_chunk_size(raid_group_p);
    fbe_raid_type_get_data_disks(raid_geometry_p->raid_type, 
                                 width,
                                 &data_disks);
    if (metadata_lba != FBE_LBA_INVALID)
    {
        metadata_lba = metadata_lba / data_disks;
    }

    /* Walk thru the update checkpoint bitmask and locate the rebuild
     * index that needs to be updated.  We only update the `blocks_rebuilt'
     * when the start lba is in user space.
     */
    for (position = 0; position < width; position++)
    {
        /* Check if this position is set
         */
        if (((1 << position) & update_checkpoint_bitmask) != 0)
        {
            /* Update the number of blocks rebuilt
             */
            fbe_raid_group_rebuild_find_existing_checkpoint_entry_for_position(raid_group_p, 
                                                                               position, 
                                                                               &rebuild_index);
            if ((rebuild_index == FBE_RAID_GROUP_INVALID_INDEX)         ||
                (rebuild_index >= FBE_RAID_GROUP_MAX_REBUILD_POSITIONS)    )
            {
                /* This is unexpected.  Trace an error and return 0 for the percent
                 */
                fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                                      FBE_TRACE_LEVEL_ERROR,
                                      FBE_TRACE_MESSAGE_ID_INFO,
                                      "%s - could not locate rebuild index for pos: %d \n",
                                      __FUNCTION__, position);
                return FBE_STATUS_GENERIC_FAILURE;
            }

            if ((metadata_lba != FBE_LBA_INVALID)           &&
                (rebuild_start_lba >= metadata_lba))
            {
                /* Only update the checkpoint if the request was in user space.
                 */
                fbe_raid_group_trace(raid_group_p,
                             FBE_TRACE_LEVEL_INFO,
                             FBE_RAID_GROUP_DEBUG_FLAG_REBUILD_NOTIFICATION_TRACING,
                             "raid_group: RB update start lba: 0x%llx blocks: 0x%llx mask: 0x%x metadata rebuild 0x%llx\n",
                             (unsigned long long)rebuild_start_lba,
                             (unsigned long long)rebuild_blocks,
                             update_checkpoint_bitmask,
                             raid_group_p->raid_group_metadata_memory.blocks_rebuilt[position]);
                return FBE_STATUS_OK;
            }
            else if (rebuild_start_lba >= exported_per_disk_capacity)
            {
                /* Only update the checkpoint if the request was in user space.
                 */
                fbe_raid_group_trace(raid_group_p,
                             FBE_TRACE_LEVEL_INFO,
                             FBE_RAID_GROUP_DEBUG_FLAG_REBUILD_NOTIFICATION_TRACING,
                             "raid_group: RB update start lba: 0x%llx blocks: 0x%llx mask: 0x%x not user io\n",
                             (unsigned long long)rebuild_start_lba,
                             (unsigned long long)rebuild_blocks,
                             update_checkpoint_bitmask);
                return FBE_STATUS_OK;
            }
            else
            {
                /* Else update by the full amount.  Validate that if the 
                 * start lba is 0 that the blocks rebuild is 0.
                 */
                if ((rebuild_start_lba == 0)                                                      &&
                    (raid_group_p->raid_group_metadata_memory.blocks_rebuilt[rebuild_index] != 0)    )
                {
                    /* Generate an error trace but continue.
                     */
                    fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                                      FBE_TRACE_LEVEL_ERROR,
                                      FBE_TRACE_MESSAGE_ID_INFO,
                             "raid_group: RB update start lba: 0x%llx blocks: 0x%llx blocks rebuilt: 0x%llx not 0\n",
                             (unsigned long long)rebuild_start_lba,
                             (unsigned long long)rebuild_blocks,
                             (unsigned long long)raid_group_p->raid_group_metadata_memory.blocks_rebuilt[rebuild_index]);
                    raid_group_p->raid_group_metadata_memory.blocks_rebuilt[rebuild_index] = 0;
                }
                raid_group_p->raid_group_metadata_memory.blocks_rebuilt[rebuild_index] += rebuild_blocks;
                fbe_raid_group_trace(raid_group_p,
                                     FBE_TRACE_LEVEL_INFO,
                                     FBE_RAID_GROUP_DEBUG_FLAG_REBUILD_NOTIFICATION_TRACING,
                                     "raid_group: RB update pos: %d blocks: 0x%llx blocks rebuilt to: 0x%llx\n",
                                     position, (unsigned long long)rebuild_blocks,
                                     (unsigned long long)raid_group_p->raid_group_metadata_memory.blocks_rebuilt[rebuild_index]);

            }

            /* If sufficient time has elapsed update the peer
             */
            fbe_raid_group_get_last_checkpoint_time(raid_group_p, &last_checkpoint_time);
            ms_since_checkpoint = fbe_get_elapsed_milliseconds(last_checkpoint_time);
            fbe_raid_group_class_get_peer_update_checkpoint_interval_ms(&peer_update_period_ms);
            if (ms_since_checkpoint > peer_update_period_ms)
            {
                /* Set the condition to update the peer
                 */
                fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                                      FBE_TRACE_LEVEL_INFO,
                                      FBE_TRACE_MESSAGE_ID_INFO,
                                      "raid_group: RB update pos: %d blocks rebuilt: 0x%llx update peer\n",
                                      position, 
                                      (unsigned long long)raid_group_p->raid_group_metadata_memory.blocks_rebuilt[rebuild_index]);
                fbe_lifecycle_set_cond(&fbe_raid_group_lifecycle_const, 
                                       (fbe_base_object_t*)raid_group_p, 
                                       FBE_BASE_CONFIG_LIFECYCLE_COND_UPDATE_METADATA_MEMORY);
            }

            /* If the blocks rebuilt is beyond the user capacity something
             * is wrong.
             */
            if (raid_group_p->raid_group_metadata_memory.blocks_rebuilt[rebuild_index] > exported_per_disk_capacity)
            {
                fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                                      FBE_TRACE_LEVEL_WARNING,
                                      FBE_TRACE_MESSAGE_ID_INFO,
                             "raid_group: RB update start lba: 0x%llx blocks: 0x%llx blocks rebuilt: 0x%llx exceeds cap\n",
                             (unsigned long long)rebuild_start_lba,
                             (unsigned long long)rebuild_blocks,
                             (unsigned long long)raid_group_p->raid_group_metadata_memory.blocks_rebuilt[rebuild_index]);

                /* Rebuild check point is beyond the user area, for now to keep rebuild alive we are 
                 * setting block rebuilt to exported per disk capacity.
                 */
                 raid_group_p->raid_group_metadata_memory.blocks_rebuilt[rebuild_index] = exported_per_disk_capacity;
            }

        } /* end if this was a position that was rebuilt */

    } /* end for all raid group positions*/
    
    /* Return success
     */
    return FBE_STATUS_OK;
}
/********************************************
 * end fbe_raid_group_update_blocks_rebuilt()
 ********************************************/

/*!***************************************************************************
 *          fbe_raid_group_data_reconstruction_send_progress_notification()
 *****************************************************************************
 *
 * @brief   This function generates the PROGRESS reconstruct notification.  
 *
 * @param   raid_group_p - Pointer to raid group being worked on.
 * @param   packet_p - Pointer to monitor packet to use to update pvd
 * @param   downstream_edge_p - Downstream edge pointer
 * @param   rebuild_index_for_notification - The rebuild index for the notification
 * @param   percent_complete - Percent complete
 *
 * @return  status
 *
 *****************************************************************************/
static fbe_status_t fbe_raid_group_data_reconstruction_send_progress_notification(fbe_raid_group_t *raid_group_p,
                                                                          fbe_packet_t *packet_p,
                                                                          fbe_block_edge_t *downstream_edge_p,
                                                                          fbe_u32_t rebuild_index_for_notification,
                                                                          fbe_u32_t percent_complete)
{
    fbe_status_t            status;
    fbe_notification_info_t notify_info;
    fbe_object_id_t         pvd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_trace_level_t       error_trace_level = FBE_TRACE_LEVEL_ERROR;
    fbe_raid_group_nonpaged_metadata_t *nonpaged_metadata_p = NULL;
    fbe_raid_group_rebuild_checkpoint_info_t *rb_info_p = NULL;

    /* Get the position information.
     */
    fbe_base_config_get_nonpaged_metadata_ptr((fbe_base_config_t*)raid_group_p, (void **)&nonpaged_metadata_p);
    rb_info_p = &nonpaged_metadata_p->rebuild_info.rebuild_checkpoint_info[rebuild_index_for_notification];

    /* Validate the rebuild pvd object id
     */
    if ((rebuild_index_for_notification >= FBE_RAID_GROUP_MAX_REBUILD_POSITIONS)              ||
        (raid_group_p->rebuilt_pvds[rebuild_index_for_notification] == FBE_OBJECT_ID_INVALID)    )
    {
        /* Generate an error trace and return
         */
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "raid_group: send progress rebuild index: %d or pvd obj: 0x%x invalid \n",
                              rebuild_index_for_notification,
                              (rebuild_index_for_notification >= FBE_RAID_GROUP_MAX_REBUILD_POSITIONS) ? FBE_OBJECT_ID_INVALID : raid_group_p->rebuilt_pvds[rebuild_index_for_notification]);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Validate the percent complete.
     */
    pvd_object_id = raid_group_p->rebuilt_pvds[rebuild_index_for_notification];
    if (((fbe_s32_t)percent_complete < 0)        ||
        (percent_complete > 100)                 ||
        (pvd_object_id == FBE_OBJECT_ID_INVALID)    )
    {
        /* Generate an error trace and return
         */
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "raid_group: send progress rebuild index: %d or pvd obj: 0x%x percent complete: %d invalid\n",
                              rebuild_index_for_notification, pvd_object_id, percent_complete);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* If the percent complete is greater than 0, send the update to PVD.  Otherwise, do not send it since when 
     * DATA_RECONSTRUCTION_START is already set to 1 to cover the corner case where we have a glitch in the system 
     * or SLF that causes Admin to wipe out the previous information after a Mega Poll.
     */
    if (percent_complete > 0) {
        /* First update the `percent rebuilt' in the associated pvd.
         */
        status = fbe_raid_group_rebuild_update_pvd_percent_rebuilt(raid_group_p,
                                                                   packet_p,
                                                                   downstream_edge_p,
                                                                   pvd_object_id,
                                                                   percent_complete);
        if (status != FBE_STATUS_OK)
        {
            /*! @note If we cannot update the downstream pvd rebuilt percent
             *        it most likely means that the drive was removed. In any 
             *        case reset this rebuild index to `not rebuilding'.
             */
            error_trace_level = (status == FBE_STATUS_EDGE_NOT_ENABLED) ? FBE_TRACE_LEVEL_WARNING : FBE_TRACE_LEVEL_ERROR;
            fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                                  error_trace_level,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "raid_group: send progress update pvd percent rebuilt for: %d or pvd obj: 0x%x percent: %d failed - status: 0x%x\n",
                                  rebuild_index_for_notification, pvd_object_id, percent_complete, status);

            /* Now reset the associated values (drive was removed)
             */
            raid_group_p->rebuilt_pvds[rebuild_index_for_notification] = FBE_OBJECT_ID_INVALID;
            raid_group_p->previous_rebuild_percent[rebuild_index_for_notification] = 0;
            raid_group_p->raid_group_metadata_memory.blocks_rebuilt[rebuild_index_for_notification] = 0;

            return status;
        }
    }

    /* Generate the notification.
     */
    notify_info.notification_type = FBE_NOTIFICATION_TYPE_DATA_RECONSTRUCTION;
    notify_info.class_id = FBE_CLASS_ID_PROVISION_DRIVE;
    notify_info.object_type = FBE_TOPOLOGY_OBJECT_TYPE_PROVISIONED_DRIVE;
    notify_info.notification_data.data_reconstruction.percent_complete = percent_complete;
    notify_info.notification_data.data_reconstruction.state = DATA_RECONSTRUCTION_IN_PROGRESS;

    fbe_base_object_trace((fbe_base_object_t*)raid_group_p, 
                          FBE_TRACE_LEVEL_INFO, 
                          FBE_TRACE_MESSAGE_ID_INFO, 
                          "RB: pos: %d pvd obj: 0x%x progress: %d%% \n",
                          rb_info_p->position, pvd_object_id, percent_complete);

    status = fbe_notification_send(pvd_object_id, notify_info);
    if (status != FBE_STATUS_OK) 
    { 
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, 
                              FBE_TRACE_LEVEL_WARNING, 
                              FBE_TRACE_MESSAGE_ID_INFO, 
                              "%s unexpected status: %d on notification send \n", __FUNCTION__, status);
    }

    return FBE_STATUS_OK;
}
/*****************************************************************
 * end fbe_raid_group_data_reconstruction_send_progress_notification()
 *****************************************************************/

/*!****************************************************************************
 *          fbe_raid_group_data_reconstruction_progress_function()
 ******************************************************************************
 * @brief
 *  Send notification to upper layers about the fact we start/stop data reconstruction
 *  This happen on the passive side. The active side will do it in a different context
 *
 * @param   riad_group_p - Pointer to raid group that may be rebuilding
 * @param   packet_p - pointer to a monitor packet.
 * 
 * @return nothing
 ******************************************************************************/
void fbe_raid_group_data_reconstruction_progress_function(fbe_raid_group_t *raid_group_p,
                                                          fbe_packet_t *packet_p)
{
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_class_id_t                      my_class_id;
    fbe_bool_t                          b_is_rebuild_needed = FBE_FALSE;
    fbe_u32_t                           percent_complete[FBE_RAID_GROUP_MAX_REBUILD_POSITIONS];
    fbe_object_id_t                     pvd_object_id;
    fbe_raid_group_nonpaged_metadata_t *nonpaged_metadata_p = NULL;
    fbe_raid_group_rebuild_checkpoint_info_t *rb_info_p = NULL;
    fbe_u32_t                           rebuild_index;
    fbe_raid_position_bitmask_t         rl_bitmask;
    fbe_block_edge_t                   *downstream_edge_p;
    fbe_bool_t                          b_raid_group_fully_rebuilt = FBE_FALSE;
   
    /*if we are the VD, we are all set, we know based on the server who is the PVD
      otherwise, we'll need to dig a bit deeper */
    my_class_id = fbe_base_object_get_class_id(fbe_base_pointer_to_handle((fbe_base_t *) raid_group_p)); 

    /*! @note This method no longer supports the virtual drive. 
     */
    if (my_class_id == FBE_CLASS_ID_VIRTUAL_DRIVE)
    {
        /* This method should not be invoked for the virtual drive.
         */
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, 
                              FBE_TRACE_LEVEL_ERROR, 
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                              "%s no longer supported for vd class id: %d \n", 
                              __FUNCTION__, my_class_id);
        return;
    }

    /* First check if rebuild needs to run.
     */
    b_is_rebuild_needed = fbe_raid_group_background_op_is_rebuild_need_to_run(raid_group_p);

    /* First check if it is disabled.
     */
    if (!fbe_base_config_is_system_bg_service_enabled(FBE_BASE_CONFIG_CONTROL_SYSTEM_BG_SERVICE_ENABLE_REBUILD))
    {
        /* Rebuild is currently disabled.  Just return.
         */
        return;
    }

    /* Step 1: Handle the case where we did not clean up properly from the last
     *         rebuild.
     */
    fbe_raid_group_rebuild_cleanup_notify_if_required(raid_group_p,
                                                      packet_p,
                                                      b_is_rebuild_needed);

    /* Initialize percentage complete to -1
     */
    for (rebuild_index = 0; rebuild_index < FBE_RAID_GROUP_MAX_REBUILD_POSITIONS; rebuild_index++)
    {
        percent_complete[rebuild_index] = FBE_U32_MAX;
    }

    /* Only send the notification if we are still rebuilding.
     */
    if (b_is_rebuild_needed)
    {
        /*  Get a pointer to the raid group object non-paged metadata  */
        fbe_base_config_get_nonpaged_metadata_ptr((fbe_base_config_t*) raid_group_p, (void **)&nonpaged_metadata_p);

        /* Get the rebuild logging bitmask */
        fbe_raid_group_get_rb_logging_bitmask(raid_group_p, &rl_bitmask);

        /* Now determine which position is actively being rebuilt.
         */
        for (rebuild_index = 0; rebuild_index < FBE_RAID_GROUP_MAX_REBUILD_POSITIONS; rebuild_index++)
        {
            rb_info_p = &nonpaged_metadata_p->rebuild_info.rebuild_checkpoint_info[rebuild_index];

            /* If this position is rebuilding, get the percent rebuild complete.
             */
            if ((rb_info_p->position != FBE_RAID_INVALID_DISK_POSITION) &&
                ((1 << rb_info_p->position) & (~rl_bitmask))                )
            {
                /* Get the percent rebuilt for this position
                 */
                percent_complete[rebuild_index] = fbe_raid_group_get_percent_user_space_rebuilt_for_position(raid_group_p,
                                                                                          rb_info_p->position);
            }

        } /* end for all rebuilding positions get the current rebuilding info */

        /* Now generate the proper notifications for all rebuilding positions
         */
        for (rebuild_index = 0; rebuild_index < FBE_RAID_GROUP_MAX_REBUILD_POSITIONS; rebuild_index++)
        {
            rb_info_p = &nonpaged_metadata_p->rebuild_info.rebuild_checkpoint_info[rebuild_index];

            /* check if rebuild logging is not in progress
             */
            if ((rb_info_p->position != FBE_RAID_INVALID_DISK_POSITION) &&
                (1 << rb_info_p->position) & (~rl_bitmask)                 )
            {
                /* There are (3) possible cases:
                 *  1. This is the first time this rebuild index has started 
                 *     rebuilding - Initialize the rebuild info
                 *  2. This is the position that is rebuilding but there is
                 *     no change in the rebuild percentage.
                 *  3. This is the position that is rebuilding and there has
                 *     been a change in the percent rebuilt.  We need to
                 *     `refresh' the rebuilt pvd info since this can change.
                 */
                if (raid_group_p->rebuilt_pvds[rebuild_index] == FBE_OBJECT_ID_INVALID)
                {
                    /* Case 1: The rebuild pvd for this rebuild index has not 
                     *         yet been initialized.
                     */

                    /* Get the object id of the downstream object for this position.
                     */ 
                    fbe_base_config_get_block_edge((fbe_base_config_t*)raid_group_p, &downstream_edge_p, rb_info_p->position);

                    /* Get the pvd object id of the active edge of the virtual drive.
                     */
                    status  = fbe_raid_group_get_id_of_pvd_being_worked_on(raid_group_p, packet_p, downstream_edge_p, &pvd_object_id);
                    if (status != FBE_STATUS_OK) 
                    { 
                        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, 
                                              FBE_TRACE_LEVEL_WARNING, 
                                              FBE_TRACE_MESSAGE_ID_INFO, 
                                              "%s get pvd id failed - status: 0x%x\n", 
                                              __FUNCTION__, status);
                        continue;
                    }

                    /* Populate the pvd id.
                     */
                    raid_group_p->rebuilt_pvds[rebuild_index] = pvd_object_id;

                    /* Generate the start event (don't clear rebuild percent)
                     */
                    status = fbe_raid_group_data_reconstruction_progress_send_start_end_notification(raid_group_p,
                                                                                            packet_p,
                                                                                            downstream_edge_p, 
                                                                                            DATA_RECONSTRUCTION_START,
                                                                                            rebuild_index,
                                                                                            FBE_FALSE   /* Don't clear rebuilt percent*/);
                    if (status != FBE_STATUS_OK) 
                    { 
                        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, 
                                              FBE_TRACE_LEVEL_WARNING, 
                                              FBE_TRACE_MESSAGE_ID_INFO, 
                                              "%s send start / end failed - status: 0x%x\n", 
                                              __FUNCTION__, status);
                        continue;
                    }

                    /* Generate the progress
                     */
                    status = fbe_raid_group_data_reconstruction_send_progress_notification(raid_group_p,
                                                                                  packet_p,
                                                                                  downstream_edge_p, 
                                                                                  rebuild_index, 
                                                                                  percent_complete[rebuild_index]);
                    if (status != FBE_STATUS_OK) 
                    { 
                        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, 
                                              FBE_TRACE_LEVEL_WARNING, 
                                              FBE_TRACE_MESSAGE_ID_INFO, 
                                              "%s send progress failed - status: 0x%x\n", 
                                              __FUNCTION__, status);
                        continue;
                    }

                    /* Update the previous percent rebuilt for this rebuild index 
                     */
                    raid_group_p->previous_rebuild_percent[rebuild_index] = percent_complete[rebuild_index];
                }
                else if (percent_complete[rebuild_index] == raid_group_p->previous_rebuild_percent[rebuild_index])
                {
                    /* Case 2: This is the position being rebuilt but there has
                     *         not been any change in the percentage rebuilt.
                     *         Continue to the next rebuild position or return.
                     */
                    continue;
                }
                else if (percent_complete[rebuild_index] != raid_group_p->previous_rebuild_percent[rebuild_index])
                {
                    /* Case 3: This is the position being rebuilt and the 
                     *         percentage rebuilt has changed.  We `refresh'
                     *         the pvd info for this position in case there has
                     *         been a change downstream.
                     */

                    /* Get the object id of the downstream object for this position.
                     */ 
                    fbe_base_config_get_block_edge((fbe_base_config_t*)raid_group_p, &downstream_edge_p, rb_info_p->position);

                    /* Get the pvd object id of the active edge of the virtual drive.
                     */
                    status  = fbe_raid_group_get_id_of_pvd_being_worked_on(raid_group_p, packet_p, downstream_edge_p, &pvd_object_id);
                    if (status != FBE_STATUS_OK) 
                    { 
                        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, 
                                              FBE_TRACE_LEVEL_WARNING, 
                                              FBE_TRACE_MESSAGE_ID_INFO, 
                                              "%s get pvd id failed - status: 0x%x\n",
                                              __FUNCTION__, status);
                        continue;
                    }

                    /* Populate the pvd id.
                     */
                    raid_group_p->rebuilt_pvds[rebuild_index] = pvd_object_id;

                    /* Generate the progress
                     */
                    status = fbe_raid_group_data_reconstruction_send_progress_notification(raid_group_p,
                                                                                  packet_p,
                                                                                  downstream_edge_p, 
                                                                                  rebuild_index, 
                                                                                  percent_complete[rebuild_index]);
                    if (status != FBE_STATUS_OK) 
                    { 
                        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, 
                                              FBE_TRACE_LEVEL_WARNING, 
                                              FBE_TRACE_MESSAGE_ID_INFO, 
                                              "%s send progress failed - status: 0x%x\n", 
                                              __FUNCTION__, status);
                        continue;
                    }

                    /* Update the previous.
                     */
                    raid_group_p->previous_rebuild_percent[rebuild_index] = percent_complete[rebuild_index];

                    /* If this position is done send the end notification.
                     */
                    if (percent_complete[rebuild_index] == 100)
                    {
                        /*! @note There is a special case where there are multiple
                         *        positions rebuilding.
                         */
                        b_raid_group_fully_rebuilt = !fbe_raid_group_is_another_position_rebuilding(raid_group_p, rb_info_p->position);
                        fbe_raid_group_data_reconstruction_progress_send_start_end_notification(raid_group_p,
                                                                                                packet_p,
                                                                                                downstream_edge_p,
                                                                                                DATA_RECONSTRUCTION_END,
                                                                                                rebuild_index,
                                                                                                b_raid_group_fully_rebuilt /* Only clear if done */);
                    }

                } /* end else if we need to update the progress*/

            } /* end if this position is not rebuild logging */

        } /* end for all rebuild positions */

    } /* end if rebuild is needed */

    return;
}
/******************************************************************************
 * end fbe_raid_group_data_reconstruction_progress_function()
 ******************************************************************************/

/*!***************************************************************************
 *          fbe_raid_group_rebuild_peer_memory_udpate()
 ***************************************************************************** 
 * 
 * @brief   This method is called where the SP receives a `peer metadata memory
 *          update' message.  We need to update the local blocks rebuilt from
 *          the peers.
 *
 * @param   raid_group_p - Pointer to riad group object that was updated
 * 
 * @return nothing
 *****************************************************************************/
fbe_status_t fbe_raid_group_rebuild_peer_memory_udpate(fbe_raid_group_t *raid_group_p)
{
    fbe_bool_t                          b_is_active = FBE_FALSE;
    fbe_raid_group_metadata_memory_t   *local_raid_group_metadata_p = NULL;
    fbe_raid_group_metadata_memory_t   *peer_raid_group_metadata_p = NULL;
    fbe_u32_t                           rebuild_index;
    
    /* If we are the active SP then there is nothing to do (the active SP `owns'
     * the `blocks_rebuilt').
     */
    b_is_active = fbe_base_config_is_active((fbe_base_config_t *)raid_group_p);
    if (b_is_active)
    {
        return FBE_STATUS_OK;
    }

    /* If the metadata was not initialized trace a warning.
     */
    if (raid_group_p->base_config.metadata_element.metadata_element_state == FBE_METADATA_ELEMENT_STATE_INVALID)
    {
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, 
                              FBE_TRACE_LEVEL_WARNING, 
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s FBE_METADATA_ELEMENT_STATE_INVALID \n", 
                              __FUNCTION__);
        return FBE_STATUS_OK;
    }
    local_raid_group_metadata_p = (fbe_raid_group_metadata_memory_t *)raid_group_p->base_config.metadata_element.metadata_memory.memory_ptr;

    /* We expect the peer memory to be valid.
     */
    peer_raid_group_metadata_p = (fbe_raid_group_metadata_memory_t *)raid_group_p->base_config.metadata_element.metadata_memory_peer.memory_ptr;
    if (peer_raid_group_metadata_p == NULL) 
    {
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s NULL peer metadata pointer\n", 
                              __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Only update the `blocks_rebuilt' if this is the passive SP.
     */
    for (rebuild_index = 0; rebuild_index < FBE_RAID_GROUP_MAX_REBUILD_POSITIONS; rebuild_index++)
    {
        /* If enabled and the peer value is not zero trace the update
         */
        if (peer_raid_group_metadata_p->blocks_rebuilt[rebuild_index] != 0)
        {
            fbe_raid_group_trace(raid_group_p,
                                 FBE_TRACE_LEVEL_INFO,
                                 FBE_RAID_GROUP_DEBUG_FLAG_REBUILD_NOTIFICATION_TRACING,
                                 "raid_group: RB peer memory update index: %d local blocks rebuilt: 0x%llx peer: 0x%llx \n",
                                 rebuild_index,
                                 (unsigned long long)local_raid_group_metadata_p->blocks_rebuilt[rebuild_index],
                                 (unsigned long long)peer_raid_group_metadata_p->blocks_rebuilt[rebuild_index]);
        }

        /* Just copy the value from the peer.
         */
        local_raid_group_metadata_p->blocks_rebuilt[rebuild_index] = peer_raid_group_metadata_p->blocks_rebuilt[rebuild_index];
    }

    return FBE_STATUS_OK;;
}
/***************************************************
 * end fbe_raid_group_rebuild_peer_memory_udpate()
 ***************************************************/

/*!****************************************************************************
 *  fbe_raid_group_rebuild_set_background_ops_chunks()
 ******************************************************************************
 * @brief
 *  Set the number of chunks to build per rebuild
 *
 * @param   chunks_per_rebuild - Number of chunks to build per rebuild
 * 
 * @return nothing
 ******************************************************************************/
fbe_status_t fbe_raid_group_rebuild_set_background_op_chunks(fbe_u32_t chunks_per_rebuild)
{
    if (chunks_per_rebuild > FBE_RAID_IOTS_MAX_CHUNKS)
    {
        fbe_topology_class_trace(FBE_CLASS_ID_RAID_GROUP,
                                 FBE_TRACE_LEVEL_WARNING, 
                                 FBE_TRACE_MESSAGE_ID_INFO,
                                 "%s chunks per rebuild > max io chunks (%d > %d)\n", 
                                 __FUNCTION__, chunks_per_rebuild, FBE_RAID_IOTS_MAX_CHUNKS);
        return FBE_STATUS_GENERIC_FAILURE;
    } 

    fbe_topology_class_trace(FBE_CLASS_ID_RAID_GROUP, 
                             FBE_TRACE_LEVEL_INFO, 
                             FBE_TRACE_MESSAGE_ID_INFO,
                             "%s Setting chunks per rebuild to %d\n", 
                             __FUNCTION__, chunks_per_rebuild);
    fbe_raid_group_rebuild_background_op_chunks = chunks_per_rebuild; 
    return FBE_STATUS_OK;
        
}
 /*****************************************************************
 * end fbe_raid_group_rebuild_set_background_op_chunks()
 *****************************************************************/

/*******************************
 * end fbe_raid_group_rebuild.c
 *******************************/

