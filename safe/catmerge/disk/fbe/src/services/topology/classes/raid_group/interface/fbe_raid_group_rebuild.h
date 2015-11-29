#ifndef FBE_RAID_GROUP_REBUILD_H
#define FBE_RAID_GROUP_REBUILD_H

/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_raid_group_rebuild.h
 ***************************************************************************
 *
 * @brief
 *   This file contains the private defines and function prototypes for the
 *   rebuild processing code for the raid group object.
 * 
 * @ingroup raid_group_class_files
 * 
 * @version
 *   12/07/2009:  Created. Jean Montes.
 *
 ***************************************************************************/


//-----------------------------------------------------------------------------
//  INCLUDES OF REQUIRED HEADER FILES:
//

#include "fbe_raid_group_object.h"             // for the raid group object 


//-----------------------------------------------------------------------------
//  NAMED CONSTANTS (#defines):

#define FBE_RAID_GROUP_INVALID_BLOCK_SIZE   (0)

//-----------------------------------------------------------------------------
//  ENUMERATIONS:

/*!*******************************************************************
 * @struct fbe_rebuild_operation_state_e
 *********************************************************************
 * @brief
 *  This enumerator contains the rebuild state of the rebuild monitor
 *  operation, it is used to debugging the current rebuild state.
 *
 *********************************************************************/
typedef enum fbe_rebuild_operation_state_e
{
    FBE_RAID_GROUP_REBUILD_STATE_UNKNOWN            = 0,           // !< Initialized with unknown state.
    FBE_RAID_GROUP_REBUILD_STATE_PREREAD_METADATA   = 1,           // !< Preread the metadate before sending to executor.
    FBE_RAID_GROUP_REBUILD_STATE_PERMIT_EVENT       = 2,           // !< Send a rebuild permission request to upstream object.
    FBE_RAID_GROUP_REBUILD_STATE_REBUILD_IO         = 3,           // !< Send I/O to RAID executor.
}
fbe_rebuild_operation_state_t;

//-----------------------------------------------------------------------------
//  TYPEDEFS: 
//

/*!*******************************************************************
 * @struct fbe_raid_group_rebuild_context_s
 *********************************************************************
 * @brief
 *  This structure contains the pertinent information about a rebuild
 *  monitor I/O operation.
 *
 *********************************************************************/
typedef struct fbe_raid_group_rebuild_context_s
{
    fbe_lba_t                       start_lba;                  // !< starting LBA
    fbe_block_count_t               block_count;                // !< number of blocks
    fbe_raid_position_bitmask_t     rebuild_bitmask;            // !< bitmask of positions being rebuilt
    fbe_object_id_t                 lun_object_id;              // !< lun object-id
    fbe_bool_t                      is_lun_start_b;             // !< is lun start of the extent
    fbe_bool_t                      is_lun_end_b;               // !< is lun end of the extent
    fbe_rebuild_operation_state_t   current_state;              // !< it indicates current state of the monitor rebuild operation.
    fbe_raid_position_bitmask_t     update_checkpoint_bitmask;  // !< checkpoint update position information
    fbe_lba_t                       update_checkpoint_lba;      // !< checkpoint update start lba
    fbe_block_count_t               update_checkpoint_blocks;   // !< checkpoint update block count
    fbe_raid_position_t             event_log_position;         // !< position for event log
    fbe_base_config_physical_drive_location_t  event_log_location; // !< bus number of the rebuilt disk
    fbe_raid_position_t             event_log_source_position;  // !< position for event log
    fbe_base_config_physical_drive_location_t  event_log_source_location; // !< bus number of the rebuilt disk
}
fbe_raid_group_rebuild_context_t;

//-----------------------------------------------------------------------------
//  MACROS:


//-----------------------------------------------------------------------------
//  FUNCTION PROTOTYPES: 

/*!****************************************************************************
 * fbe_raid_group_rebuild_context_set_event_log_position()
 ******************************************************************************
 * @brief 
 *   This function sets the monitor rebuild operation state. 
 *
 * @param in_rebuild_context_p      - pointer to the rebuild trackign structure.
 * @param event_log_position        - rebuild state.
 * 
 * @return void      
 *
 ******************************************************************************/

static __forceinline void fbe_raid_group_rebuild_context_set_event_log_position(
                                        fbe_raid_group_rebuild_context_t*  in_rebuild_context_p, 
                                        fbe_raid_position_t in_event_log_position)

{
    //  Set the start LBA in the rebuild tracking structure
    in_rebuild_context_p->event_log_position = in_event_log_position; 

} // End fbe_raid_group_rebuild_context_set_event_log_position()

/*!****************************************************************************
 * fbe_raid_group_rebuild_context_set_event_log_source_position()
 ******************************************************************************
 * @brief 
 *   This function sets the monitor rebuild operation state. 
 *
 * @param in_rebuild_context_p      - pointer to the rebuild trackign structure.
 * @param event_log_position        - rebuild state.
 * 
 * @return void      
 *
 ******************************************************************************/

static __forceinline void fbe_raid_group_rebuild_context_set_event_log_source_position(
                                        fbe_raid_group_rebuild_context_t*  in_rebuild_context_p, 
                                        fbe_raid_position_t in_event_log_position)

{
    //  Set the start LBA in the rebuild tracking structure
    in_rebuild_context_p->event_log_source_position = in_event_log_position; 

} // End fbe_raid_group_rebuild_context_set_event_log_source_position()

/*!****************************************************************************
 * fbe_raid_group_rebuild_context_set_rebuild_state()
 ******************************************************************************
 * @brief 
 *   This function sets the monitor rebuild operation state. 
 *
 * @param in_rebuild_context_p      - pointer to the rebuild trackign structure.
 * @param rebuild_state             - rebuild state.
 * 
 * @return void      
 *
 ******************************************************************************/

static __forceinline void fbe_raid_group_rebuild_context_set_rebuild_state(
                                        fbe_raid_group_rebuild_context_t*  in_rebuild_context_p, 
                                        fbe_rebuild_operation_state_t rebuild_state)

{
    //  Set the start LBA in the rebuild tracking structure
    in_rebuild_context_p->current_state = rebuild_state; 

} // End fbe_raid_group_rebuild_context_set_rebuild_state()

/*!****************************************************************************
 * fbe_raid_group_rebuild_context_set_rebuild_positions()
 ******************************************************************************
 * @brief 
 *   This function sets the rebuild position in the rebuild tracking structure. 
 *
 * @param in_rebuild_context_p      - pointer to the rebuild trackign structure.
 * @param in_rebuild_position       - position in raid group to be rebuilt
 * 
 * @return void      
 *
 ******************************************************************************/

static __forceinline void fbe_raid_group_rebuild_context_set_rebuild_positions(
                                        fbe_raid_group_rebuild_context_t*  in_rebuild_context_p, 
                                        fbe_raid_position_bitmask_t        in_rebuild_positions)

{
    //  Set the rebuild positions in the rebuild tracking structure
    in_rebuild_context_p->rebuild_bitmask = in_rebuild_positions; 

} // End fbe_raid_group_rebuild_context_set_rebuild_positions()

/*!****************************************************************************
 * fbe_raid_group_rebuild_context_set_rebuild_start_lba()
 ******************************************************************************
 * @brief 
 *   This function sets the start LBA in the rebuild tracking structure. 
 *
 * @param in_rebuild_context_p      - pointer to the rebuild trackign structure.
 * @param in_start_lba              - starting LBA. The LBA is relative to the RAID
 *                                    extent on a single disk. 
 * 
 * @return void      
 *
 ******************************************************************************/

static __forceinline void fbe_raid_group_rebuild_context_set_rebuild_start_lba(
                                        fbe_raid_group_rebuild_context_t*  in_rebuild_context_p, 
                                        fbe_lba_t                           in_start_lba)

{
    //  Set the start LBA in the rebuild tracking structure
    in_rebuild_context_p->start_lba = in_start_lba; 

} // End fbe_raid_group_rebuild_context_set_rebuild_start_lba()

/*!****************************************************************************
 * fbe_raid_group_rebuild_context_get_rebuild_start_lba()
 ******************************************************************************
 * @brief 
 *   This function gets the start LBA in the rebuild tracking structure. 
 *
 * @param in_rebuild_context_p      - pointer to the rebuild trackign structure.
 * @param in_start_lba_p              - starting LBA. The LBA is relative to the RAID
 *                                    extent on a single disk. 
 * 
 * @return void      
 *
 ******************************************************************************/

static __forceinline void fbe_raid_group_rebuild_context_get_rebuild_start_lba(
                                        fbe_raid_group_rebuild_context_t*  in_rebuild_context_p, 
                                        fbe_lba_t                        *in_start_lba_p)

{
    //  Get the start LBA in the rebuild tracking structure
    *in_start_lba_p = in_rebuild_context_p->start_lba; 

} // End fbe_raid_group_rebuild_context_get_rebuild_start_lba()

/*!****************************************************************************
 * fbe_raid_group_rebuild_context_set_rebuild_block_count()
 ******************************************************************************
 * @brief 
 *   This function sets the block count in the rebuild tracking structure. 
 *
 * @param in_rebuild_context_p      - pointer to the rebuild trackign structure.
 * @param in_block_count            - number of blocks to rebuild 
 * 
 * @return void      
 *
 ******************************************************************************/

static __forceinline void fbe_raid_group_rebuild_context_set_rebuild_block_count(
                                        fbe_raid_group_rebuild_context_t*  in_rebuild_context_p,
                                        fbe_block_count_t                   in_block_count)

{
    //  Set the rebuild block count in the rebuild tracking structure
    in_rebuild_context_p->block_count = in_block_count; 

} // End fbe_raid_group_rebuild_context_set_rebuild_block_count()



/*!****************************************************************************
 * fbe_raid_group_rebuild_context_set_lun_object_id()
 ******************************************************************************
 * @brief 
 *   This function sets the LUN object id field in the rebuild tracking structure. 
 *
 * @param in_raid_group_p           - pointer to the raid group object 
 * @param in_lun_object_id          - object id of the LUN that contains the LBA
 *                                    range of this rebuild; use FBE_OBJECT_ID_INVALID
 *                                    if unknown or not in a LUN 
 * 
 * @return void      
 *
 ******************************************************************************/

static __forceinline void fbe_raid_group_rebuild_context_set_lun_object_id(
                                        fbe_raid_group_rebuild_context_t*   in_rebuild_context_p,
                                        fbe_object_id_t                     in_lun_object_id)

{
    //  Set the object ID of the LUN that this rebuild is for 
    in_rebuild_context_p->lun_object_id = in_lun_object_id; 

} // End fbe_raid_group_rebuild_context_set_lun_object_id()


/*!****************************************************************************
 * fbe_raid_group_rebuild_context_set_rebuild_is_lun_start()
 ******************************************************************************
 * @brief 
 *   This function sets the boolean in the rebuild tracking structure as to 
 *   whether this rebuild is for the start of a LUN. 
 *
 * @param in_raid_group_p           - pointer to the raid group object 
 * @param in_is_lun_start_b         - TRUE indicates this is a LUN's start
 * 
 * @return void      
 *
 ******************************************************************************/

static __forceinline void fbe_raid_group_rebuild_context_set_rebuild_is_lun_start(
                                        fbe_raid_group_rebuild_context_t*   in_rebuild_context_p,
                                        fbe_bool_t                          in_is_lun_start_b)

{
    //  Set whether or not this is a LUN's starting LBA in the rebuild tracking structure
    in_rebuild_context_p->is_lun_start_b = in_is_lun_start_b; 

} // End fbe_raid_group_rebuild_context_set_rebuild_is_lun_start()


/*!****************************************************************************
 * fbe_raid_group_rebuild_context_set_rebuild_is_lun_end()
 ******************************************************************************
 * @brief 
 *   This function sets the boolean in the rebuild tracking structure as to 
 *   whether this rebuild is for the end of a LUN. 
 *
 * @param in_raid_group_p           - pointer to the raid group object 
 * @param in_is_lun_end_b           - TRUE indicates this is a LUN's end
 * 
 * @return void      
 *
 ******************************************************************************/

static __forceinline void fbe_raid_group_rebuild_context_set_rebuild_is_lun_end(
                                        fbe_raid_group_rebuild_context_t*   in_rebuild_context_p,
                                        fbe_bool_t                          in_is_lun_end_b)

{
    //  Set whether or not this is a LUN's ending LBA in the rebuild tracking structure
    in_rebuild_context_p->is_lun_end_b = in_is_lun_end_b; 

} // End fbe_raid_group_rebuild_context_set_rebuild_is_lun_end()

/*!****************************************************************************
 * fbe_raid_group_rebuild_context_set_update_checkpoint_bitmask()
 ******************************************************************************
 * @brief 
 *   This function sets update checkpoint bitmask in the rebuild context
 *
 * @param in_rebuild_context_p      - pointer to the rebuild trackign structure.
 * @param checkpoint_update_bitmask - bitmask of positions to update checkpoint for
 * 
 * @return void      
 *
 ******************************************************************************/

static __forceinline void fbe_raid_group_rebuild_context_set_update_checkpoint_bitmask(
                                        fbe_raid_group_rebuild_context_t *in_rebuild_context_p, 
                                        fbe_raid_position_bitmask_t checkpoint_update_bitmask)

{
    // Set the update checkpoint bitmask
    in_rebuild_context_p->update_checkpoint_bitmask = checkpoint_update_bitmask; 

} // End fbe_raid_group_rebuild_context_set_update_checkpoint_bitmask()

/*!****************************************************************************
 * fbe_raid_group_rebuild_context_get_update_checkpoint_bitmask()
 ******************************************************************************
 * @brief 
 *   This function gets update checkpoint bitmask in the rebuild context
 *
 * @param in_rebuild_context_p      - pointer to the rebuild trackign structure.
 * @param checkpoint_update_bitmask_p - bitmask of positions to update checkpoint for
 * 
 * @return void      
 *
 ******************************************************************************/

static __forceinline void fbe_raid_group_rebuild_context_get_update_checkpoint_bitmask(
                                        fbe_raid_group_rebuild_context_t *in_rebuild_context_p, 
                                        fbe_raid_position_bitmask_t *checkpoint_update_bitmask_p)

{
    // Set the update checkpoint bitmask
    *checkpoint_update_bitmask_p = in_rebuild_context_p->update_checkpoint_bitmask; 

} // End fbe_raid_group_rebuild_context_get_update_checkpoint_bitmask()

/*!****************************************************************************
 * fbe_raid_group_rebuild_context_set_update_checkpoint_lba()
 ******************************************************************************
 * @brief 
 *   This function sets update checkpoint lba in the rebuild context
 *
 * @param in_rebuild_context_p      - pointer to the rebuild trackign structure.
 * @param checkpoint_update_lba - lba to update checkpoint for
 * 
 * @return void      
 *
 ******************************************************************************/

static __forceinline void fbe_raid_group_rebuild_context_set_update_checkpoint_lba(
                                        fbe_raid_group_rebuild_context_t *in_rebuild_context_p, 
                                        fbe_lba_t checkpoint_update_lba)

{
    // Set the update checkpoint start lba
    in_rebuild_context_p->update_checkpoint_lba = checkpoint_update_lba; 

} // End fbe_raid_group_rebuild_context_set_update_checkpoint_lba()

/*!****************************************************************************
 * fbe_raid_group_rebuild_context_set_update_checkpoint_blocks()
 ******************************************************************************
 * @brief 
 *   This function sets update checkpoint bitmask in the rebuild context
 *
 * @param in_rebuild_context_p      - pointer to the rebuild trackign structure.
 * @param checkpoint_update_blocks  - number of blocks to adjust checkpoint for
 * 
 * @return void      
 *
 ******************************************************************************/

static __forceinline void fbe_raid_group_rebuild_context_set_update_checkpoint_blocks(
                                        fbe_raid_group_rebuild_context_t *in_rebuild_context_p, 
                                        fbe_block_count_t checkpoint_update_blocks)

{
    // Set the update checkpoint bitmask
    in_rebuild_context_p->update_checkpoint_blocks = checkpoint_update_blocks; 

} // End fbe_raid_group_rebuild_context_set_update_checkpoint_blocks()

/*!****************************************************************************
 * fbe_raid_group_rebuild_context_get_update_checkpoint_blocks()
 ******************************************************************************
 * @brief 
 *   This function sets update checkpoint bitmask in the rebuild context
 *
 * @param in_rebuild_context_p      - pointer to the rebuild trackign structure.
 * @param checkpoint_update_blocks_p  - number of blocks to adjust checkpoint for
 * 
 * @return void      
 *
 ******************************************************************************/

static __forceinline void fbe_raid_group_rebuild_context_get_update_checkpoint_blocks(
                                        fbe_raid_group_rebuild_context_t *in_rebuild_context_p, 
                                        fbe_block_count_t *checkpoint_update_blocks_p)

{
    // Set the update checkpoint bitmask
    *checkpoint_update_blocks_p = in_rebuild_context_p->update_checkpoint_blocks; 

} // End fbe_raid_group_rebuild_context_get_update_checkpoint_blocks()

//  It allocates control operation and rebuild context for packet payload
fbe_status_t fbe_raid_group_rebuild_allocate_rebuild_context(
                                fbe_raid_group_t*   in_raid_group_p,
                                fbe_packet_t*       in_packet_p,
                                fbe_raid_group_rebuild_context_t ** out_rebuild_context_p);

//  It releases rebuild context and control operaton for payload in packet
fbe_status_t fbe_raid_group_rebuild_release_rebuild_context(
                                fbe_packet_t*       in_packet_p);


//  It initializes rebuild context
fbe_status_t fbe_raid_group_rebuild_initialize_rebuild_context(
                                fbe_raid_group_t*                           raid_group_p,
                                fbe_raid_group_rebuild_context_t*           rebuild_context_p, 
                                fbe_raid_position_bitmask_t                 positions_to_be_rebuilt, 
                                fbe_lba_t                                   in_rebuild_lba);

//  It is used to get the rebuild context information
fbe_status_t fbe_raid_group_rebuild_get_rebuild_context(
                                fbe_raid_group_t*                           raid_group_p,
                                fbe_packet_t*                               packet_p,
                                fbe_raid_group_rebuild_context_t**          out_rebuild_context_p);

//  Get the next metadata chunk which needs to rebuild
fbe_status_t fbe_raid_group_rebuild_get_next_metadata_needs_rebuild_lba(
                                fbe_raid_group_t*               in_raid_group_p,
                                fbe_u32_t                       in_position,
                                fbe_lba_t*                      out_metadata_rebuild_lba_p);

//  Determine if a particular drive needs to be rebuilt
fbe_status_t fbe_raid_group_rebuild_determine_if_rebuild_needed(
                                    fbe_raid_group_t*       in_raid_group_p,
                                    fbe_u32_t               in_position,
                                    fbe_bool_t*             out_requires_rebuild_b_p);

//  Find a drive that needs its checkpoint moved to the logical end or is ready to be rebuilt 
fbe_status_t fbe_raid_group_rebuild_find_disk_needing_action(fbe_raid_group_t            *in_raid_group_p,
                                                             fbe_raid_position_bitmask_t *out_degraded_positions_p,
                                                             fbe_raid_position_bitmask_t *out_positions_to_be_rebuilt_p,
                                                             fbe_raid_position_bitmask_t *out_complete_rebuild_positions_p,
                                                             fbe_lba_t                   *out_rebuild_checkpoint_p,
                                                             fbe_bool_t                  *out_checkpoint_reset_needed_p);
fbe_bool_t fbe_raid_group_is_singly_degraded(fbe_raid_group_t *raid_group_p);

//  It determine whether we can start the rebuild operation in current state.
fbe_status_t fbe_raid_group_rebuild_determine_if_rebuild_can_start(
                                            fbe_raid_group_t * in_raid_group_p,
                                            fbe_bool_t * out_can_rebuild_start_b_p);

//  Issue a write to the metadata to set the rebuild checkpoint(s) 
fbe_status_t fbe_raid_group_rebuild_write_checkpoint(
                                            fbe_raid_group_t*               in_raid_group_p,
                                            fbe_packet_t*                   in_packet_p, 
                                            fbe_lba_t                       in_start_lba,
                                            fbe_block_count_t               in_block_count,
                                            fbe_raid_position_bitmask_t     in_positions_to_change);

//  Process the condition to move the checkpoint to the logical end marker 
fbe_status_t fbe_raid_group_rebuild_set_checkpoint_to_end_marker(fbe_packet_t * packet_p,                                    
                                                      fbe_packet_completion_context_t context);

//  Check if a rebuild I/O is currently allowed based on the I/O load 
fbe_status_t fbe_raid_group_rebuild_check_permission_based_on_load(
                                            fbe_raid_group_t*               in_raid_group_p,
                                            fbe_bool_t*                     out_ok_to_proceed_b_p);

//  Populate a rebuild non-paged info data structure with the current values from nonpaged memory 
fbe_status_t fbe_raid_group_rebuild_get_nonpaged_info(
                                            fbe_raid_group_t*               in_raid_group_p,
                                            fbe_raid_group_rebuild_nonpaged_info_t* out_rebuild_data_p);

//  Find an existing entry in the non-paged rebuild checkpoint info for this position
fbe_status_t fbe_raid_group_rebuild_find_existing_checkpoint_entry_for_position(
                                            fbe_raid_group_t*               in_raid_group_p,
                                            fbe_u32_t                       in_position,
                                            fbe_u32_t*                      out_rebuild_data_index_p);

fbe_status_t fbe_raid_group_rebuild_find_existing_position_entry_for_checkpoint(
                                        fbe_raid_group_t*                       in_raid_group_p,
                                        fbe_lba_t                               in_checkpoint,
                                        fbe_u32_t*                              out_position_p);

//  Find the next unused entry in the non-paged rebuild checkpoint info 
fbe_status_t fbe_raid_group_rebuild_find_next_unused_checkpoint_entry(
                                            fbe_raid_group_t*               in_raid_group_p,
                                            fbe_u32_t                       in_start_index,
                                            fbe_u32_t*                      out_rebuild_data_index_p);

// Must update the NR metadata after each iots completion
fbe_status_t fbe_raid_group_rebuild_process_iots_completion(fbe_raid_group_t *raid_group_p,
                                                            fbe_packet_t *packet_p);

//  Update the rebuild checkpoint after a rebuild I/O has completed 
fbe_status_t fbe_raid_group_rebuild_update_checkpoint_after_io_completion(
                                    fbe_raid_group_t*               in_raid_group_p,
                                    fbe_packet_t*                   in_packet_p);


//  Send an event to check if this LBA is within a LUN 
fbe_status_t fbe_raid_group_rebuild_send_event_to_check_lba(
                                    fbe_raid_group_t*                   in_raid_group_p, 
                                    fbe_raid_group_rebuild_context_t*   rebuild_context_p,
                                    fbe_packet_t*                       in_packet_p);

void fbe_raid_group_rebuild_trace_chunk_info(fbe_raid_group_t *raid_group_p,
                                             fbe_raid_group_paged_metadata_t *chunk_info_p,
                                             fbe_chunk_count_t chunk_count,
                                             fbe_raid_iots_t *iots_p);

//  Get the chunk info for the next range of the rebuild LBA range.
fbe_status_t fbe_raid_group_rebuild_get_chunk_info(
                                    fbe_raid_group_t*                   in_raid_group_p,
                                    fbe_packet_t*                       in_packet_p);

fbe_bool_t fbe_raid_group_background_op_is_rebuild_need_to_run(fbe_raid_group_t*    raid_group_p);
fbe_bool_t fbe_raid_group_background_op_is_metadata_rebuild_need_to_run( fbe_raid_group_t*    raid_group_p);

fbe_bool_t fbe_raid_group_is_metadata_marked_NR(fbe_raid_group_t* raid_group_p,
                                     fbe_raid_position_bitmask_t nr_bitmask);

fbe_status_t fbe_raid_group_rebuild_get_all_rebuild_position_mask(
                                    fbe_raid_group_t*               in_raid_group_p,
                                    fbe_raid_position_bitmask_t*    out_rebuild_all_mask_p);

fbe_status_t fbe_raid_group_get_degraded_mask(fbe_raid_group_t*               raid_group_p,
                                              fbe_raid_position_bitmask_t*    rebuild_all_mask_p);
//  Get metadata lba to rebuild
fbe_status_t
fbe_raid_group_rebuild_get_next_metadata_lba_to_rebuild(
                                    fbe_raid_group_t*    in_raid_group_p,
                                    fbe_lba_t*           out_metadata_rebuild_lba_p,
                                    fbe_u32_t*           out_lowest_rebuild_position_p);

fbe_status_t fbe_raid_group_rebuild_create_checkpoint_data(
                                        fbe_raid_group_t*                       in_raid_group_p,
                                        fbe_lba_t                               in_checkpoint,
                                        fbe_raid_position_bitmask_t             in_positions_to_change,
                                        fbe_raid_group_rebuild_nonpaged_info_t* out_rebuild_data_p,
                                        fbe_u32_t*                              out_rebuild_data_index_p);
fbe_status_t fbe_raid_group_rebuild_force_nr_write_rebuild_checkpoint_info(
                                                fbe_raid_group_t*               in_raid_group_p,
                                                fbe_packet_t*                   in_packet_p,
                                                fbe_lba_t                       in_start_lba,
                                                fbe_block_count_t               in_block_count,
                                                fbe_raid_position_bitmask_t     in_positions_to_change); 

fbe_status_t fbe_raid_group_rebuild_find_disk_to_set_checkpoint_to_end_marker(
                                    fbe_raid_group_t*           in_raid_group_p,
                                    fbe_u32_t*                  out_disk_index_p);

fbe_status_t fbe_raid_group_set_rebuild_checkpoint_to_zero(fbe_raid_group_t *raid_group_p,
                                                           fbe_packet_t *packet_p,
                                                           fbe_raid_position_bitmask_t positions_completing_rebuild);

fbe_status_t fbe_raid_group_incr_rebuild_checkpoint(
                                                fbe_raid_group_t*               in_raid_group_p,
                                                fbe_packet_t*                   in_packet_p,
                                                fbe_lba_t                       in_start_lba,
                                                fbe_block_count_t               in_block_count,
                                                fbe_raid_position_bitmask_t     in_positions_to_change);

fbe_status_t fbe_raid_group_get_rebuild_index(fbe_raid_group_t*   raid_group_p,
                                              fbe_raid_position_bitmask_t    pos_bitmask,
                                              fbe_u32_t*          rebuild_index);
fbe_u32_t fbe_raid_group_get_percent_rebuilt(fbe_raid_group_t *raid_group_p);

fbe_status_t fbe_raid_group_rebuild_get_metadata_rebuild_position(
                                fbe_raid_group_t*               raid_group_p,
                                fbe_u32_t*                      metadata_rebuild_pos_p);

fbe_bool_t fbe_raid_group_is_any_metadata_marked_NR(fbe_raid_group_t *raid_group_p,
                                                    fbe_raid_position_bitmask_t nr_bitmask);


//  Update the paged metadata for an unbound area 
fbe_status_t fbe_raid_group_clear_nr_control_operation(
                                    fbe_raid_group_t*               in_raid_group_p, 
                                    fbe_packet_t*                   in_packet_p);


//  Get the disk information (bus-enclosure-slot) for a specific position in the RG 
fbe_status_t fbe_raid_group_logging_get_disk_info_for_position(
                                    fbe_raid_group_t*                       in_raid_group_p,
                                    fbe_packet_t*                           in_packet_p, 
                                    fbe_u32_t                               in_position, 
                                    fbe_u32_t*                              out_bus_num_p,
                                    fbe_u32_t*                              out_encl_num_p,
                                    fbe_u32_t*                              out_slot_num_p);


//  Get the raid group number of the raid group and the corresponding object ID, adjusted for RAID-10 if needed
fbe_status_t fbe_raid_group_logging_get_raid_group_number_and_object_id(
                                    fbe_raid_group_t*                       in_raid_group_p,
                                    fbe_u32_t*                              out_raid_group_num_p,
                                    fbe_object_id_t*                        out_raid_group_object_id_p);

fbe_status_t fbe_raid_group_rebuild_set_iots_status(fbe_raid_group_t*    raid_group_p,
                                       fbe_packet_t*        packet_p,
                                       fbe_raid_iots_t*     iots_p,
                                       fbe_packet_completion_context_t   context,
                                       fbe_payload_block_operation_status_t   block_status,
                                       fbe_payload_block_operation_qualifier_t block_qualifier);

fbe_status_t fbe_raid_group_rebuild_get_next_marked_chunk(
                                        fbe_raid_group_t*                   in_raid_group_p,
                                        fbe_packet_t*                       in_packet_p,
                                        fbe_chunk_index_t                   in_start_chunk_index, 
                                        fbe_chunk_count_t                   in_chunk_count,
										fbe_raid_position_bitmask_t*         positions_to_rebuild);

fbe_bool_t fbe_raid_group_get_next_chunk_marked_for_rebuild(fbe_u8_t*     search_data,
                                                            fbe_u32_t    search_size,
                                                            context_t    context);

fbe_status_t fbe_raid_group_quiesce_for_bg_op(fbe_raid_group_t *raid_group_p);

fbe_status_t fbe_raid_group_get_raid10_degraded_info(fbe_packet_t *packet_p,
                                                     fbe_raid_group_t *raid_group_p,
                                                     fbe_raid_group_get_degraded_info_t *degraded_info_p);

void fbe_raid_group_data_reconstruction_progress_function(fbe_raid_group_t *raid_group_p,
                                                          fbe_packet_t *packet_p);

fbe_status_t fbe_raid_group_update_blocks_rebuilt(fbe_raid_group_t *raid_group_p,                          
                                                  fbe_lba_t rebuild_start_lba,
                                                  fbe_block_count_t rebuild_blocks,
                                                  fbe_raid_position_bitmask_t update_checkpoint_bitmask);

fbe_status_t fbe_raid_group_rebuild_peer_memory_udpate(fbe_raid_group_t *raid_group_p);

fbe_status_t fbe_raid_group_rebuild_set_background_op_chunks(fbe_u32_t chunks_per_rebuild);

#endif // FBE_RAID_GROUP_REBUILD_H
