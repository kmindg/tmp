#ifndef FBE_RAID_GROUP_NEEDS_REBUILD_H
#define FBE_RAID_GROUP_NEEDS_REBUILD_H

/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/
 
/*!**************************************************************************
 * @file fbe_raid_group_needs_rebuild.h
 ***************************************************************************
 *
 * @brief
 *   This file contains the private defines and function prototypes for the
 *   Needs Rebuild (NR) processing code for the raid group object.
 * 
 * @ingroup raid_group_class_files
 * 
 * @version
 *   10/27/2009:  Created. JMM.
 *
 ***************************************************************************/


//-----------------------------------------------------------------------------
//  INCLUDES OF REQUIRED HEADER FILES:
//

#include "fbe_raid_group_object.h"             // for the raid group object 


//-----------------------------------------------------------------------------
//  NAMED CONSTANTS (#defines):


//-----------------------------------------------------------------------------
//  ENUMERATIONS:


//-----------------------------------------------------------------------------
//  TYPEDEFS: 
//
/*!*******************************************************************
 * @struct fbe_raid_group_needs_rebuild_context_s
 *********************************************************************
 * @brief
 *  This structure contains the pertinent information about mark needs
 *  rebuild operation.
 *
 *********************************************************************/
typedef struct fbe_raid_group_needs_rebuild_context_s
{
    fbe_lba_t                                       start_lba;         // !< starting LBA
    fbe_block_count_t                               block_count;       // !< number of blocks
    fbe_u32_t                                       position;          // !< disk position driving the rebuild
    fbe_raid_position_bitmask_t                     nr_position_mask;  // !< needs rebuild position mask
}
fbe_raid_group_needs_rebuild_context_t;

//-----------------------------------------------------------------------------
//  MACROS:


//-----------------------------------------------------------------------------
//  FUNCTION PROTOTYPES: 

/*!****************************************************************************
 * fbe_raid_group_needs_rebuild_context_set_start_lba()
 ******************************************************************************
 * @brief 
 *   This function sets the start LBA in the needs_rebuild tracking structure. 
 *
 * @param in_needs_rebuild_context_p    - pointer to the needs_rebuild trackign structure.
 * @param in_start_lba                  - starting LBA. The LBA is relative to the RAID
 *                                        extent on a single disk. 
 * 
 * @return void      
 *
 ******************************************************************************/

static __forceinline void fbe_raid_group_needs_rebuild_context_set_start_lba(
                                        fbe_raid_group_needs_rebuild_context_t* in_needs_rebuild_context_p, 
                                        fbe_lba_t                               in_start_lba)

{
    //  Set the start LBA in the needs_rebuild tracking structure
    in_needs_rebuild_context_p->start_lba = in_start_lba; 

} // End fbe_raid_group_needs_rebuild_context_set_start_lba()


/*!****************************************************************************
 * fbe_raid_group_needs_rebuild_context_set_needs_rebuild_block_count()
 ******************************************************************************
 * @brief 
 *   This function sets the block count in the needs_rebuild tracking structure. 
 *
 * @param in_needs_rebuild_context_p    - pointer to the needs_rebuild trackign structure.
 * @param in_block_count                - number of blocks to needs_rebuild 
 * 
 * @return void      
 *
 ******************************************************************************/

static __forceinline void fbe_raid_group_needs_rebuild_context_set_block_count(
                                        fbe_raid_group_needs_rebuild_context_t*  in_needs_rebuild_context_p,
                                        fbe_block_count_t                   in_block_count)

{
    //  Set the needs_rebuild block count in the needs_rebuild tracking structure
    in_needs_rebuild_context_p->block_count = in_block_count; 

} // End fbe_raid_group_needs_rebuild_context_set_block_count()

/*!****************************************************************************
 * fbe_raid_group_needs_rebuild_context_set_position()
 ******************************************************************************
 * @brief 
 *   This function sets the disk position in the rebuild tracking structure.
 *   The next chunk to rebuild is determined by the checkpoint of one particular
 *   disk which is driving the rebuild; that is the disk indicated here. 
 *
 * @param in_rebuild_context_p      - pointer to the rebuild trackign structure.
 * @param in_position               - disk position in the RG 
 * 
 * @return void      
 *
 ******************************************************************************/

static __forceinline void fbe_raid_group_needs_rebuild_context_set_position(
                                        fbe_raid_group_needs_rebuild_context_t* in_needs_rebuild_context_p,
                                        fbe_u32_t                               in_position)

{
    //  Set the rebuild position in the rebuild tracking structure
    in_needs_rebuild_context_p->position = in_position;

} // End fbe_raid_group_needs_rebuild_context_set_position()

/*!****************************************************************************
 * fbe_raid_group_needs_rebuild_context_set_nr_position_mask()
 ******************************************************************************
 * @brief 
 *   This function sets the disk position in the rebuild tracking structure.
 *   The next chunk to rebuild is determined by the checkpoint of one particular
 *   disk which is driving the rebuild; that is the disk indicated here. 
 *
 * @param in_rebuild_context_p      - pointer to the rebuild trackign structure.
 * @param in_nr_position_mask       - NR position mask for the RG
 * 
 * @return void      
 *
 ******************************************************************************/

static __forceinline void fbe_raid_group_needs_rebuild_context_set_nr_position_mask(
                                        fbe_raid_group_needs_rebuild_context_t* in_needs_rebuild_context_p,
                                        fbe_raid_position_bitmask_t             in_nr_position_mask)

{
    //  Set the rebuild position in the rebuild tracking structure
    in_needs_rebuild_context_p->nr_position_mask = in_nr_position_mask;

} // End fbe_raid_group_needs_rebuild_context_set_nr_position_mask()


fbe_status_t fbe_raid_group_needs_rebuild_allocate_needs_rebuild_context(
                                fbe_raid_group_t*   in_raid_group_p,
                                fbe_packet_t*       in_packet_p,
                                fbe_raid_group_needs_rebuild_context_t ** out_needs_rebuild_context_p);

//  It initializes needs rebuild context before monitor start needs rebuid
fbe_status_t fbe_raid_group_needs_rebuild_initialize_needs_rebuild_context(
                                fbe_raid_group_needs_rebuild_context_t*     in_needs_rebuild_context_p, 
                                fbe_lba_t                                   start_lba,
                                fbe_block_count_t                           block_count,
                                fbe_u32_t                                   in_position);

//  Handle NR/needs_rebuild processing when an edge changes state
fbe_status_t fbe_raid_group_needs_rebuild_handle_processing_on_edge_state_change(
                                    fbe_raid_group_t*           in_raid_group_p,
                                    fbe_u32_t                   in_position,
                                    fbe_path_state_t            in_path_state,
                                    fbe_base_config_downstream_health_state_t in_downstream_health);

fbe_status_t fbe_raid_group_needs_rebuild_handle_processing_on_edge_unavailable(fbe_raid_group_t *in_raid_group_p,
                                                                                fbe_u32_t in_position, 
                                                                                fbe_base_config_downstream_health_state_t in_downstream_health);
// Determine the region that needs to be marked for NR
fbe_status_t fbe_raid_group_needs_rebuild_mark_nr(
                                        fbe_raid_group_t*   in_raid_group_p,                                        
                                        fbe_packet_t *      in_packet_p,
                                        fbe_u32_t           in_disk_position,
                                        fbe_lba_t           in_start_lba,
                                        fbe_block_count_t   in_block_count);

fbe_status_t fbe_raid_group_needs_rebuild_mark_nr_completion(
                                fbe_packet_t * in_packet_p,
                                fbe_packet_completion_context_t context);

//  Quiesce I/O and also set the unquiesce condition if needed
fbe_status_t fbe_raid_group_needs_rebuild_quiesce_io_if_needed(
                                    fbe_raid_group_t*           in_raid_group_p);

//  It is used to process the needs rebuild data event
fbe_status_t fbe_raid_group_needs_rebuild_process_needs_rebuild_event(
                                    fbe_raid_group_t*   in_raid_group_p,
                                    fbe_event_t*        in_data_event_p,
                                    fbe_packet_t*       in_packet_p);

//  Handle the condition to mark NR for the whole RG  
fbe_status_t fbe_raid_group_needs_rebuild_process_nr_request(
                                    fbe_raid_group_t*           in_raid_group_p,
                                    fbe_packet_t*               in_packet_p);

//  Handle the condition to mark NR for a specific range
fbe_status_t fbe_raid_group_needs_rebuild_handle_mark_specific_nr_condition(
                                    fbe_raid_group_t*           in_raid_group_p,
                                    fbe_packet_t*               in_packet_p,
                                    fbe_u32_t                   in_position);                                    


//  Clear NR for the given LBA range, on the disk positions indicated 
fbe_status_t fbe_raid_group_needs_rebuild_clear_nr_for_iots(
                                    fbe_raid_group_t*           in_raid_group_p,
                                    fbe_raid_iots_t*            iots_p);

fbe_status_t fbe_raid_group_find_rl_disk(fbe_raid_group_t *in_raid_group_p,
                                         fbe_packet_t     *packet_p, 
                                         fbe_u32_t *out_disk_index_p);

//  Find the specific disk to mark NR for a specific range 
fbe_status_t fbe_raid_group_needs_rebuild_get_disk_position_to_mark_nr(
                                    fbe_raid_group_t*           in_raid_group_p,
                                    fbe_event_t*                data_event_p,
                                    fbe_u32_t*                  out_disk_index_p);

//  Update the rebuild checkpoing if needed
fbe_status_t fbe_raid_group_needs_rebuild_data_event_update_checkpoint_if_needed(
                                    fbe_raid_group_t*   in_raid_group_p,
                                    fbe_event_t *       data_event_p,
                                    fbe_packet_t*       in_packet_p);

fbe_status_t fbe_raid_group_set_rebuild_checkpoint(fbe_packet_t * packet_p,                                    
                                        fbe_packet_completion_context_t context);

//  Determine if parity RG going broken based on failed I/O requests
fbe_status_t fbe_raid_group_needs_rebuild_is_rg_going_broken(
                                        fbe_raid_group_t*   in_raid_group_p,
                                        fbe_bool_t*         out_is_rg_broken_b_p);

//  It calculates the time duration of wait and determine if elapsed seconds are greathern the timeout
fbe_status_t fbe_raid_group_needs_rebuild_determine_rg_broken_timeout(
                                           fbe_raid_group_t*    in_raid_group_p,
                                           fbe_time_t           wait_timeout,
                                           fbe_bool_t*          is_timeout_expired_b_p,
                                           fbe_time_t*          elapsed_seconds_p);
//  It is used to get the needs rebuild context information
fbe_status_t fbe_raid_group_rebuild_get_needs_rebuild_context(
                                fbe_raid_group_t*                           in_raid_group_p,
                                fbe_packet_t*                               in_packet_p,
                                fbe_raid_group_needs_rebuild_context_t**    out_needs_rebuild_context_p);
                                        
fbe_status_t fbe_raid_group_get_failed_pos_bitmask(fbe_raid_group_t *in_raid_group_p,
                                                   fbe_u16_t *out_position_bitmask_p);

fbe_status_t fbe_raid_group_get_disabled_pos_bitmask(fbe_raid_group_t *raid_group_p,
                                                     fbe_u16_t *position_bitmask_p);

fbe_status_t fbe_raid_group_get_disk_pos_by_server_id(fbe_raid_group_t *raid_group_p,
                                                      fbe_object_id_t   object_to_check,
                                                      fbe_u32_t   *disk_position);
fbe_status_t fbe_raid_group_rebuild_ok_to_mark_nr(fbe_raid_group_t *raid_group_p,
                                                  fbe_object_id_t   object_to_check,
                                                  fbe_bool_t   *ok_to_mark);

fbe_status_t fbe_raid_group_get_drives_marked_nr(fbe_raid_group_t *in_raid_group_p,
                                                 fbe_u16_t *out_position_bitmask_p);  
fbe_status_t fbe_raid_group_needs_rebuild_create_rebuild_nonpaged_data(fbe_raid_group_t*                   in_raid_group_p,
                                                                       fbe_raid_position_bitmask_t         in_positions_to_change,
                                                                       fbe_bool_t                          in_perform_clear_b, 
                                                                       fbe_raid_group_rebuild_nonpaged_info_t* out_rebuild_data_p);

//  Determine if it can mark the specific range for needs rebuild
fbe_status_t fbe_raid_group_needs_rebuild_determine_if_mark_specific_range_for_nr(
                                                fbe_raid_group_t * raid_group_p,
                                                fbe_edge_index_t   edge_index,
                                                fbe_bool_t * out_can_mark_nr_b);

fbe_status_t fbe_raid_group_get_failed_pos_bitmask_to_clear(fbe_raid_group_t *in_raid_group_p,
                                                            fbe_raid_position_bitmask_t *out_position_bitmask_p);

fbe_lifecycle_status_t fbe_raid_group_activate_set_rb_logging(fbe_raid_group_t * raid_group_p, fbe_packet_t * packet_p);

fbe_status_t fbe_raid_group_write_rl_get_dl(fbe_raid_group_t *raid_group_p, fbe_packet_t *packet_p,
                                            fbe_u16_t failed_bitmap, fbe_bool_t clear_b);
fbe_status_t fbe_raid_group_write_rl(fbe_raid_group_t *raid_group_p, fbe_packet_t *packet_p,
                                     fbe_raid_position_bitmask_t failed_bitmap, fbe_bool_t clear_b);
fbe_status_t fbe_raid_group_write_rl_completion(fbe_packet_t *packet_p,
                                                fbe_packet_completion_context_t context_p);
fbe_lifecycle_status_t fbe_raid_group_eval_mark_nr_active(fbe_raid_group_t *  raid_group_p, fbe_packet_t * packet_p);
fbe_lifecycle_status_t fbe_raid_group_eval_mark_nr_passive(fbe_raid_group_t *  raid_group_p, fbe_packet_t * packet_p);
fbe_lifecycle_status_t fbe_raid_group_eval_mark_nr_request(fbe_raid_group_t *  raid_group_p, fbe_packet_t * packet_p);
fbe_lifecycle_status_t fbe_raid_group_eval_mark_nr_done(fbe_raid_group_t *  raid_group_p, fbe_packet_t * packet_p);
fbe_lifecycle_status_t fbe_raid_group_eval_rb_logging_request(fbe_raid_group_t *  raid_group_p, fbe_packet_t * packet_p);
fbe_lifecycle_status_t fbe_raid_group_set_rb_logging_passive(fbe_raid_group_t *  raid_group_p, fbe_packet_t * packet_p);
fbe_lifecycle_status_t fbe_raid_group_set_rb_logging_active(fbe_raid_group_t *  raid_group_p, fbe_packet_t * packet_p);
fbe_lifecycle_status_t fbe_raid_group_eval_rb_logging_done(fbe_raid_group_t *  raid_group_p, fbe_packet_t * packet_p);
fbe_status_t fbe_raid_group_get_number_of_virtual_drive_edges_to_wait_for(fbe_raid_group_t *raid_group_p,
                                                                          fbe_packet_t *packet_p,
                                                                          fbe_u32_t *number_of_edges_to_wait_for_p);
fbe_status_t fbe_raid_group_is_virtual_drive_in_mirror_mode(fbe_raid_group_t *raid_group_p,
                                                            fbe_packet_t *packet_p,
                                                            fbe_bool_t *b_is_in_mirror_mode_p);
fbe_status_t fbe_raid_group_get_virtual_drive_valid_edge_bitmask(fbe_raid_group_t *raid_group_p,
                                                                 fbe_packet_t *packet_p,
                                                                 fbe_raid_position_bitmask_t *bitmask_p);

fbe_lifecycle_status_t fbe_raid_group_clear_rl_request(fbe_raid_group_t *  raid_group_p, fbe_packet_t * packet_p);
fbe_lifecycle_status_t fbe_raid_group_clear_rl_passive(fbe_raid_group_t *  raid_group_p, fbe_packet_t * packet_p);
fbe_lifecycle_status_t fbe_raid_group_clear_rl_active(fbe_raid_group_t *  raid_group_p, fbe_packet_t * packet_p);
fbe_lifecycle_status_t fbe_raid_group_clear_rl_done(fbe_raid_group_t *  raid_group_p, fbe_packet_t * packet_p);

fbe_lifecycle_status_t fbe_raid_group_join_request(fbe_raid_group_t *  raid_group_p, fbe_packet_t * packet_p);
fbe_lifecycle_status_t fbe_raid_group_join_done(fbe_raid_group_t *  raid_group_p, fbe_packet_t * packet_p);
fbe_lifecycle_status_t fbe_raid_group_join_passive(fbe_raid_group_t *  raid_group_p, fbe_packet_t * packet_p);
fbe_lifecycle_status_t fbe_raid_group_join_active(fbe_raid_group_t *  raid_group_p, fbe_packet_t * packet_p);
fbe_lifecycle_status_t fbe_raid_group_join_sync_complete(fbe_raid_group_t *  raid_group_p, fbe_packet_t * packet_p);

fbe_status_t fbe_raid_group_needs_rebuild_ack_mark_nr_done(
                                fbe_packet_t*      packet_p,
                                fbe_raid_group_t*  raid_group_p,
                                fbe_u32_t          disk_position);
fbe_status_t fbe_raid_group_needs_rebuild_ack_mark_nr_done_completion(
                                fbe_packet_t * packet_p,
                                fbe_packet_completion_context_t context);

fbe_status_t fbe_rg_needs_rebuild_mark_nr_from_usurper(
                                        fbe_raid_group_t*   in_raid_group_p,                                        
                                        fbe_packet_t *      in_packet_p,
                                        fbe_u32_t           in_disk_position,
                                        fbe_lba_t           in_start_lba,
                                        fbe_block_count_t   in_block_count,
                                        fbe_raid_group_needs_rebuild_context_t*     needs_rebuild_context_p);

fbe_status_t fbe_rg_needs_rebuild_mark_nr_from_usurper_completion(
                                fbe_packet_t * in_packet_p,
                                fbe_packet_completion_context_t context);

fbe_status_t fbe_rg_needs_rebuild_force_mark_nr_set_checkpoint(fbe_packet_t * in_packet_p,
                                                               fbe_packet_completion_context_t context);

fbe_bool_t fbe_raid_group_allow_virtual_drive_to_continue_degraded(fbe_raid_group_t *raid_group_p,
                                                                   fbe_packet_t *packet_p,
                                                                   fbe_raid_position_bitmask_t drives_marked_nr);

fbe_status_t fbe_raid_group_handle_virtual_drive_peer_nonpaged_write(fbe_raid_group_t *raid_group_p);

fbe_status_t fbe_raid_group_metadata_is_mark_nr_required(fbe_raid_group_t *raid_group_p,
                                                         fbe_bool_t *b_is_mark_nr_required_p);

fbe_status_t fbe_raid_group_metadata_clear_mark_nr_required(fbe_raid_group_t *raid_group_p,
                                                            fbe_packet_t *packet_p);

fbe_status_t fbe_raid_group_metadata_clear_paged_metadata_reconstruct_required(fbe_raid_group_t *raid_group_p,
                                                            fbe_packet_t *packet_p);

fbe_status_t fbe_raid_group_metadata_set_paged_metadata_reconstruct_required(fbe_raid_group_t *raid_group_p,
                                                             fbe_packet_t *packet_p);

fbe_status_t fbe_raid_group_metadata_is_paged_metadata_reconstruct_required(fbe_raid_group_t *raid_group_p,
                                                         fbe_bool_t *b_is_reconstruct_required_p);

fbe_status_t 
fbe_raid_group_metadata_reconstruct_paged_metadata(fbe_raid_group_t * raid_group_p,
                                                     fbe_packet_t *packet_p);

#endif // FBE_RAID_GROUP_NEEDS_REBUILD_H
