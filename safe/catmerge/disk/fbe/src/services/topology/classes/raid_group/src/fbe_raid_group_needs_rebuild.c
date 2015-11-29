/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/
 
/*!**************************************************************************
 * @file fbe_raid_group_needs_rebuild.c
 ***************************************************************************
 *
 * @brief
 *   This file contains the Needs Rebuild (NR) processing code for the raid 
 *   group object.
 * 
 * @ingroup raid_group_class_files
 * 
 * @version
 *   10/27/2009:  Created. Jean Montes.
 *
 ***************************************************************************/


/*************************
 *   INCLUDE FILES
 *************************/

#include "fbe_raid_group_needs_rebuild.h"               //  this file's .h file  
#include "fbe_raid_group_bitmap.h"                      //  bitmap accessor functions 
#include "fbe_raid_group_object.h"                      //  for fbe_raid_group_get_disk_capacity
#include "fbe_raid_group_rebuild.h"                     //  for fbe_raid_group_rebuild_determine_if_rebuild_needed 
#include "fbe_transport_memory.h"                       //  for transport related memory allocation
#include "fbe_service_manager.h"                        //  to send the control packet
#include "fbe_raid_geometry.h"                          //  for raid geometry defines
#include "fbe_cmi.h"                                    //  for CMI defines
#include "fbe_raid_group_logging.h"
#include "fbe_raid_verify.h"


/*****************************************
 * LOCAL FUNCTION PROTOTYPES
 *****************************************/

//  Completion function for to handle marking NR
static fbe_status_t fbe_raid_group_needs_rebuild_handle_mark_nr_completion(
                                    fbe_packet_t*                   in_packet_p,
                                    fbe_packet_completion_context_t in_context);

//  Handle rebuild processing when an edge becomes enabled 
static fbe_status_t fbe_raid_group_needs_rebuild_handle_processing_on_edge_enable(
                                        fbe_raid_group_t*                   in_raid_group_p,
                                        fbe_u32_t                           in_position);

//  Completion function for marking NR on the user data area of a RG
static fbe_status_t fbe_raid_group_needs_rebuild_handle_mark_nr_for_user_data_completion(
                                    fbe_packet_t*                   in_packet_p,
                                    fbe_packet_completion_context_t in_context);

static fbe_status_t fbe_raid_group_needs_rebuild_force_nr_set_checkpoint_with_lock(fbe_packet_t * packet_p,                                    
                                                                                   fbe_packet_completion_context_t context);
static fbe_status_t fbe_raid_group_c4_mirror_mark_nr_completion(fbe_packet_t * in_packet_p,
                                                               fbe_packet_completion_context_t context);
/***************
 * FUNCTIONS
 ***************/

/*!****************************************************************************
 * fbe_raid_group_needs_rebuild_allocate_needs_rebuild_context()
 ******************************************************************************
 * @brief
 *   This function is called by the raid group monitor to allocate the control
 *   operation which tracks needs rebuild operation.
 *
 * @param in_raid_group_p               - pointer to the raid group object
 * @param packet_p                      - pointer to the packet
 * @param out_needs_rebuild_context_p   - pointer to the needs rebuild context
 * 
 * @return fbe_status_t  
 *
 * @author
 *   10/26/2010 - Created. Dhaval Patel.
 *
 ******************************************************************************/
fbe_status_t fbe_raid_group_needs_rebuild_allocate_needs_rebuild_context(
                                fbe_raid_group_t*   in_raid_group_p,
                                fbe_packet_t*       in_packet_p,
                                fbe_raid_group_needs_rebuild_context_t ** out_needs_rebuild_context_p)
{
    fbe_payload_ex_t *                         sep_payload_p;
    fbe_payload_control_operation_t *           control_operation_p = NULL;


    //  Initialize needs rebuild context as null
    *out_needs_rebuild_context_p = NULL;

    //  Get the sep payload from the packet
    sep_payload_p = fbe_transport_get_payload_ex(in_packet_p);

    //  Allocate the needs_rebuild control operation with needs_rebuild tracking information.
    control_operation_p = fbe_payload_ex_allocate_control_operation(sep_payload_p);
    if(control_operation_p == NULL)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    //  Allocate the buffer to hold the needs rebuild tracking information.
    *out_needs_rebuild_context_p = (fbe_raid_group_needs_rebuild_context_t *)fbe_transport_allocate_buffer();
    if(*out_needs_rebuild_context_p == NULL)
    {
        //  If memory allocation fails then release the control operation.
        fbe_payload_ex_release_control_operation(sep_payload_p, control_operation_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    //  Build the needs rebuild control operation
    fbe_payload_control_build_operation(control_operation_p,
                                        FBE_RAID_GROUP_CONTROL_CODE_NEEDS_REBUILD_OPERATION,
                                        *out_needs_rebuild_context_p,
                                        sizeof(fbe_raid_group_needs_rebuild_context_t));

    //  Increment the control operation
    fbe_payload_ex_increment_control_operation_level(sep_payload_p);

    //  Return good status
    return FBE_STATUS_OK;

} // End fbe_raid_group_needs_rebuild_allocate_needs_rebuild_context()

/*!****************************************************************************
 * fbe_raid_group_rebuild_initialize_rebuild_context
 ******************************************************************************
 * @brief
 *   This function is called by the raid group monitor to initialize the rebuild
 *   context before starting the rebuild operation.
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
fbe_status_t fbe_raid_group_needs_rebuild_initialize_needs_rebuild_context(
                                fbe_raid_group_needs_rebuild_context_t*     in_needs_rebuild_context_p, 
                                fbe_lba_t                                   start_lba,
                                fbe_block_count_t                           block_count,
                                fbe_u32_t                                   in_position)
{

    //  Set the rebuild data in the rebuild tracking structure 
    fbe_raid_group_needs_rebuild_context_set_start_lba(in_needs_rebuild_context_p, start_lba);
    fbe_raid_group_needs_rebuild_context_set_block_count(in_needs_rebuild_context_p, block_count);
    fbe_raid_group_needs_rebuild_context_set_position(in_needs_rebuild_context_p, in_position); 
    fbe_raid_group_needs_rebuild_context_set_nr_position_mask(in_needs_rebuild_context_p, (1 << in_position));

    //  Return success
    return FBE_STATUS_OK;

} // End fbe_raid_group_rebuild_initialize_rebuild_context()

/*!****************************************************************************
 * fbe_raid_group_needs_rebuild_determine_rg_broken_timeout()
 ******************************************************************************
 * @brief
 *  This function will determine whether we timed out waiting for the RAID
 *  group to be broken.
 * 
 * @param in_raid_group_p           - pointer to a raid group object
 * @param wait_timeout              - timeout in seconds
 * @param is_time_out_expired_b_p   - pointer to the expired timeout
 * @param elapsed_seconds_p         - pointer to the elapsed seconds
 * 
 * @return  fbe_status_t  
 *
 * @author
 *   02/24/2010 - Created. Dhaval Patel. 
 *
 ******************************************************************************/
fbe_status_t fbe_raid_group_needs_rebuild_determine_rg_broken_timeout(
                                           fbe_raid_group_t*    in_raid_group_p,
                                           fbe_time_t           wait_timeout,
                                           fbe_bool_t*          is_timeout_expired_b_p,
                                           fbe_time_t*          elapsed_seconds_p)
{

    fbe_time_t                              rg_broken_check_timestamp;  // how long waited so far for RG to go broken
    fbe_time_t                              current_time;               // used to store the current time for comparison

    //  Initialize timeout expired as failse
    *is_timeout_expired_b_p = FBE_FALSE;
    *elapsed_seconds_p = 0;

    //  Get the current time
    current_time = fbe_get_time();

    //  If the timestamp has not been set yet, set it
    rg_broken_check_timestamp = fbe_raid_group_get_check_rg_broken_timestamp(in_raid_group_p);
    if (rg_broken_check_timestamp == 0)
    {
        fbe_raid_group_set_check_rg_broken_timestamp(in_raid_group_p, current_time);
    }

    //  Get the elapsed time since the timestamp
    *elapsed_seconds_p = fbe_get_elapsed_seconds(in_raid_group_p->last_check_rg_broken_timestamp);

    //  Determine if there is more time to process the disk failure(s), i.e. to recognize that
    //  the RG is broken
    if (*elapsed_seconds_p > wait_timeout)
    {
        *is_timeout_expired_b_p = FBE_TRUE;
    }

    //  Return good status
    return FBE_STATUS_OK;

} // End fbe_raid_group_needs_rebuild_determine_rg_broken_timeout()

/*!**************************************************************
 * fbe_raid_group_is_peer_present()
 ****************************************************************
 * @brief
 *  Is the peer currently alive?
 *
 * @param raid_group_p - Ptr to current rg.               
 *
 * @return fbe_bool_t FBE_TRUE if the peer is present.
 *                   FBE_FALSE if the peer is not present
 *                   Note that not present means that this peer
 *                   has not joined yet for this object.
 *
 * @author
 *  7/14/2011 - Created. Rob Foley
 *
 ****************************************************************/

fbe_bool_t fbe_raid_group_is_peer_present(fbe_raid_group_t *raid_group_p)
{
    return fbe_base_config_has_peer_joined((fbe_base_config_t*)raid_group_p);
}
/******************************************
 * end fbe_raid_group_is_peer_present()
 ******************************************/

/*!****************************************************************************
 * fbe_raid_group_needs_rebuild_handle_processing_on_edge_state_change()
 ******************************************************************************
 * @brief
 *   This function will perform any action that needs to be done when an edge 
 *   changes state.  It runs in the context of the edge state change event and 
 *   not the monitor context. 
 * 
 * @param in_raid_group_p       - pointer to a raid group object
 * @param in_position           - disk position in the RG 
 * @param in_path_state         - new path/edge state of the position 
 * @param in_downstream_health  - overall health state of the downstream paths
 *
 * @return  fbe_status_t        
 *
 * @author
 *   05/03/2010 - Created. Jean Montes.
 *
 ******************************************************************************/
fbe_status_t fbe_raid_group_needs_rebuild_handle_processing_on_edge_state_change(
                                    fbe_raid_group_t*                           in_raid_group_p,
                                    fbe_u32_t                                   in_position,
                                    fbe_path_state_t                            in_path_state,
                                    fbe_base_config_downstream_health_state_t   in_downstream_health)

{
    fbe_block_edge_t *edge_p = NULL;

    fbe_base_config_get_block_edge(&in_raid_group_p->base_config, &edge_p, in_position);

    //  If the new path state is anything other than slumber, then set a condition so the object will wake up from
    //  hibernation.   This allows it to process conditions that only run in the ready or activate states.
    if (in_path_state != FBE_PATH_STATE_SLUMBER)
    {
        fbe_lifecycle_set_cond(&fbe_base_config_lifecycle_const, (fbe_base_object_t*) in_raid_group_p,
            FBE_BASE_CONFIG_LIFECYCLE_COND_EDGE_CHANGE_DURING_HIBERNATION);
    }

    //  Take specific action depending on the path state 
    switch (in_path_state)
    {
        //  If the edge became enabled, call function to handle that 
        case FBE_PATH_STATE_ENABLED:
        {
            fbe_raid_group_needs_rebuild_handle_processing_on_edge_enable(in_raid_group_p, in_position);
            break; 
        }

        //  If the edge went into slumber, we don't need to do anything 
        case FBE_PATH_STATE_SLUMBER:
        {
            break; 
        }

        //  In all other cases, the edge went away.  Call function to handle that.  
        default: 
            break; 
    }

    //  Return success  
    return FBE_STATUS_OK;

} // End fbe_raid_group_needs_rebuild_handle_processing_on_edge_state_change()


/*!****************************************************************************
 * fbe_raid_group_needs_rebuild_handle_processing_on_edge_enable()
 ******************************************************************************
 * @brief
 *   This function will set condition(s) to perform NR and rebuild related processing 
 *   when an edge becomes enabled.  It runs in the context of the edge state change 
 *   event and not the monitor context. 
 * 
 * @param in_raid_group_p       - pointer to a raid group object
 * @param in_position           - disk position in the RG 
 *
 * @return  fbe_status_t        
 *
 * @author
 *   12/07/2009 - Created. Jean Montes.
 *
 ******************************************************************************/
static fbe_status_t fbe_raid_group_needs_rebuild_handle_processing_on_edge_enable(
                                        fbe_raid_group_t*   raid_group_p,
                                        fbe_u32_t           in_position)
{
    /* It doesn't hurt to force raid go through this condition when we see an edge become available */
    fbe_raid_group_lock(raid_group_p);
    fbe_raid_group_set_local_state(raid_group_p, FBE_RAID_GROUP_LOCAL_STATE_EVAL_MARK_NR_REQUEST);
    fbe_raid_group_unlock(raid_group_p);
    fbe_lifecycle_set_cond(&fbe_raid_group_lifecycle_const, (fbe_base_object_t*) raid_group_p,
                           FBE_RAID_GROUP_LIFECYCLE_COND_EVAL_MARK_NR);

    //  Return success  
    return FBE_STATUS_OK;

} // End fbe_raid_group_needs_rebuild_handle_processing_on_edge_enable()


/*!****************************************************************************
 * fbe_raid_group_needs_rebuild_handle_processing_on_edge_unavailable()
 ******************************************************************************
 * @brief
 *   This function will take RL/rebuild-related actions when an edge becomes unavailable.
 *   It runs in the context of the edge state change event and not the monitor context.
 * 
 * @param raid_group_p       - pointer to a raid group object
 * @param position           - disk position in the RG 
 * @param downstream_health  - overall health state of the downstream paths
 *
 * @return  fbe_status_t        
 *
 * @author
 *   09/16/2010 - Created. Jean Montes.
 *
 ******************************************************************************/
fbe_status_t 
fbe_raid_group_needs_rebuild_handle_processing_on_edge_unavailable(fbe_raid_group_t *raid_group_p,
                                                                   fbe_u32_t position, 
                                                                   fbe_base_config_downstream_health_state_t downstream_health)
{

    // fbe_bool_t                  b_is_rb_logging;
    fbe_object_id_t object_id;

    //  Check if the downstream health is degraded.  If not, we don't need to do anything more - just return.
    //if (downstream_health != FBE_DOWNSTREAM_HEALTH_DEGRADED)
    //{
    //    return FBE_STATUS_OK;
    //}

    //  The health is degraded.  Get the rebuild logging setting for this disk.
    // fbe_raid_group_get_rb_logging(raid_group_p, position, &b_is_rb_logging);

    //  If we are rebuild logging ie. we already know that it is gone, then we don't need to do 
    //  anything more - just return. 
    // if (b_is_rb_logging == FBE_TRUE)
    // {
    //    return FBE_STATUS_OK;
    // }

    //  The edge went away for the first time.  We need to abort any outstanding stripe locks that the monitor is 
    //  waiting for.  Otherwise the monitor might never get a chance to run.
    fbe_base_object_trace((fbe_base_object_t*) raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO, 
        "rg_needs_rebd_edge_unavailable: edge %d down, aborting MD for MD element 0x%p\n", 
        position, &raid_group_p->base_config.metadata_element);
    fbe_metadata_stripe_lock_element_abort(&raid_group_p->base_config.metadata_element);

    /* Abort paged since a monitor operation might be waiting for paged. 
     * The monitor will not run to start eval rb logging or quiesce until the monitor op completes.
     */
    fbe_metadata_element_abort_paged(&raid_group_p->base_config.metadata_element);
    /* Monitor operations might be waiting for memory.  Make sure these get aborted.
     */
    fbe_base_config_abort_monitor_ops((fbe_base_config_t*)raid_group_p);

    fbe_base_object_get_object_id((fbe_base_object_t*)raid_group_p, &object_id);

    /* Monitor operations need to get aborted if they are queued in the block transport server 
     * due to bandwidth limitations.
     */
    fbe_block_transport_server_abort_monitor_operations(&raid_group_p->base_config.block_transport_server,
                                                        object_id);
    //  Return success  
    return FBE_STATUS_OK;

} // End fbe_raid_group_needs_rebuild_handle_processing_on_edge_unavailable()


/*!****************************************************************************
 * fbe_raid_group_needs_rebuild_mark_nr()
 ******************************************************************************
 * @brief
 *   This function will mark NR on the drive according to the checkpoint
 *
 * 
 * @param in_raid_group_p       - pointer to a raid group object
 * @param in_packet_p           - pointer to a control packet from the scheduler
 * @param in_disk_position      - disk that is marked for NR
 * @param in_start_lba          - checkpoint to start the rebuild from
 * @param block_count           - number of block to rebuild
 * 
 * @return  fbe_status_t  
 *
 * @author
 *   02/14/2011 - Created. Ashwin Tamilarasan
 *
 ******************************************************************************/
fbe_status_t fbe_raid_group_needs_rebuild_mark_nr(
                                        fbe_raid_group_t*   in_raid_group_p,                                        
                                        fbe_packet_t *      in_packet_p,
                                        fbe_u32_t           in_disk_position,
                                        fbe_lba_t           in_start_lba,
                                        fbe_block_count_t   in_block_count)
{

    fbe_raid_group_needs_rebuild_context_t *needs_rebuild_context_p = NULL;
    fbe_lba_t                               configured_disk_capacity;

    //  Allocate the needs rebuild control operation to manage the needs rebuild context
    fbe_raid_group_needs_rebuild_allocate_needs_rebuild_context(in_raid_group_p, in_packet_p, &needs_rebuild_context_p);
    if(needs_rebuild_context_p == NULL)
    {
        fbe_transport_complete_packet(in_packet_p);
        return FBE_STATUS_OK;
    }

    //  Initialize rebuild context before we start the rebuild opreation
    fbe_raid_group_needs_rebuild_initialize_needs_rebuild_context(needs_rebuild_context_p, in_start_lba,
                 in_block_count, in_disk_position);
    
    //  Set the condition completion function before we mark NR
    if (fbe_raid_group_is_c4_mirror_raid_group(in_raid_group_p)) {
        fbe_transport_set_completion_function(in_packet_p, fbe_raid_group_c4_mirror_mark_nr_completion, in_raid_group_p);
    } else {
        fbe_transport_set_completion_function(in_packet_p, fbe_raid_group_needs_rebuild_mark_nr_completion, in_raid_group_p);
    }
    
    // If the virtual drive is completely rebuild just complete the packet
    configured_disk_capacity = fbe_raid_group_get_disk_capacity(in_raid_group_p);

    /* If the virtual drive checkpoint is beyond the metadata capacity, then 
     * we consider the virtual drive fully rebuilt and there is nothing to mark.  
     * Set the clear rebuild logging condition.
     */
     if (in_start_lba >= configured_disk_capacity)
     {
        fbe_transport_set_status(in_packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(in_packet_p);
        return FBE_STATUS_OK;
     }

    //  Call a function to mark NR for the RAID object
    fbe_raid_group_needs_rebuild_process_nr_request(in_raid_group_p, in_packet_p); 
    
    return FBE_STATUS_MORE_PROCESSING_REQUIRED; 

} // End fbe_raid_group_needs_rebuild_mark_nr()

/*!****************************************************************************
 * fbe_raid_group_c4_mirror_set_new_pvd_completion()
 ******************************************************************************
 * @brief
 *   This function will set the clear rb logging condition.
 *
 * @param in_packet_p
 * @param context
 *
 * @return fbe_status_t
 *
 * @author
 *   03/12/2015 - Created. Kang, Jamin 
 *
 ******************************************************************************/
fbe_status_t fbe_raid_group_c4_mirror_set_new_pvd_completion(
                                fbe_packet_t * packet_p,
                                fbe_packet_completion_context_t context)
{
    fbe_raid_group_t*                           raid_group_p = NULL;
    fbe_status_t                                status;


    raid_group_p = (fbe_raid_group_t*) context;
    status = fbe_transport_get_status_code(packet_p);

    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *) raid_group_p, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s set new PVD failed: 0x%x\n",
                              __FUNCTION__, status);
        return status;
    }

    //  Set the condition to clear rb logging

    fbe_raid_group_lock(raid_group_p);
    fbe_raid_group_set_local_state(raid_group_p, FBE_RAID_GROUP_LOCAL_STATE_CLEAR_RL_REQUEST);
    fbe_lifecycle_set_cond(&fbe_raid_group_lifecycle_const, (fbe_base_object_t *) raid_group_p,
                            FBE_RAID_GROUP_LIFECYCLE_COND_CLEAR_RB_LOGGING);
    fbe_raid_group_unlock(raid_group_p);
    return FBE_STATUS_OK;

}// End fbe_raid_group_c4_mirror_set_new_pvd_completion

/*!****************************************************************************
 * fbe_raid_group_c4_mirror_mark_nr_completion()
 ******************************************************************************
 * @brief
 *   This function is completion function of c4mirror raid group makr NR
 *   
 * @param in_packet_p     - Pointer to a control packet from the scheduler.
 * @param context       
 *
 * @return fbe_status_t     - Return FBE_STATUS_OK on success/condition will
 *                            be cleared.
 *
 * @author 
 *   03/12/2015 - Created. Kang, Jamin
 *
 ******************************************************************************/
static fbe_status_t fbe_raid_group_c4_mirror_mark_nr_completion(fbe_packet_t * in_packet_p,
                                                               fbe_packet_completion_context_t context)
{

    fbe_raid_group_t *                          raid_group_p;
    fbe_payload_ex_t *                          sep_payload_p;
    fbe_payload_control_operation_t *           control_operation_p;
    fbe_raid_group_needs_rebuild_context_t*     needs_rebuild_context_p;
    fbe_status_t                                status;
    fbe_u32_t                                   disk_position;
    fbe_payload_control_status_t                control_status;


    //  Get pointer to raid group object
    raid_group_p = (fbe_raid_group_t *) context;

    //  Get the status of the packet
    status = fbe_transport_get_status_code(in_packet_p);

    //  Get control status
    sep_payload_p = fbe_transport_get_payload_ex(in_packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(sep_payload_p);
    fbe_payload_control_get_status(control_operation_p, &control_status);

    fbe_payload_control_get_buffer(control_operation_p, &needs_rebuild_context_p);
    disk_position = needs_rebuild_context_p->position;
    fbe_transport_release_buffer(needs_rebuild_context_p);
    fbe_payload_ex_release_control_operation(sep_payload_p, control_operation_p);
    if((status != FBE_STATUS_OK) || (control_status != FBE_PAYLOAD_CONTROL_STATUS_OK))
    {
        fbe_base_object_trace((fbe_base_object_t *) raid_group_p, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s marking of nr failed, status:0x%x\n",
                              __FUNCTION__, status);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_base_object_trace((fbe_base_object_t *) raid_group_p, 
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s marking of nr for pos: %d successful\n",
                          __FUNCTION__, disk_position);

    fbe_transport_set_completion_function(in_packet_p, fbe_raid_group_c4_mirror_set_new_pvd_completion,
                                          raid_group_p);
    status = fbe_raid_group_c4_mirror_set_new_pvd(raid_group_p, in_packet_p, disk_position);

    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
} // End fbe_raid_group_c4_mirror_mark_nr_completion()

/*!****************************************************************************
 * fbe_raid_group_needs_rebuild_mark_nr_completion()
 ******************************************************************************
 * @brief
 *   This function will frees up any control operation we have allocated
 *  and calls a function to update the nonpaged metadata
 *   
 * @param in_packet_p     - Pointer to a control packet from the scheduler.
 * @param context       
 *
 * @return fbe_status_t     - Return FBE_STATUS_OK on success/condition will
 *                            be cleared.
 *
 * @author 
 *   2/22/2011 - Created. Ashwin Tamilarasan. 
 *
 ******************************************************************************/
fbe_status_t fbe_raid_group_needs_rebuild_mark_nr_completion(
                                fbe_packet_t * in_packet_p,
                                fbe_packet_completion_context_t context)
{

    fbe_raid_group_t *                          raid_group_p;
    fbe_payload_ex_t *                          sep_payload_p;
    fbe_payload_control_operation_t *           control_operation_p;
    fbe_raid_group_needs_rebuild_context_t*     needs_rebuild_context_p;
    fbe_status_t                                status;
    fbe_u32_t                                   disk_position;
    fbe_payload_control_status_t                control_status;


    //  Get pointer to raid group object
    raid_group_p = (fbe_raid_group_t *) context;

    //  Get the status of the packet
    status = fbe_transport_get_status_code(in_packet_p);

    //  Get control status
    sep_payload_p = fbe_transport_get_payload_ex(in_packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(sep_payload_p);
    fbe_payload_control_get_status(control_operation_p, &control_status);

    
    fbe_payload_control_get_buffer(control_operation_p, &needs_rebuild_context_p);
    disk_position = needs_rebuild_context_p->position;
    fbe_transport_release_buffer(needs_rebuild_context_p);
    fbe_payload_ex_release_control_operation(sep_payload_p, control_operation_p);
    if((status != FBE_STATUS_OK) || (control_status != FBE_PAYLOAD_CONTROL_STATUS_OK))
    {
        fbe_base_object_trace((fbe_base_object_t *) raid_group_p, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s marking of nr failed, status:0x%x\n",
                              __FUNCTION__, status);

        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    fbe_base_object_trace((fbe_base_object_t *) raid_group_p, 
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s marking of nr for pos: %d successful\n",
                          __FUNCTION__, disk_position);    

    fbe_raid_group_needs_rebuild_ack_mark_nr_done(in_packet_p, raid_group_p, disk_position);

    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    
} // End fbe_raid_group_needs_rebuild_mark_nr_completion()


/*!****************************************************************************
 * fbe_raid_group_needs_rebuild_ack_mark_nr_done()
 ******************************************************************************
 * @brief
 *   This function will send a usurper to clear the permanent spare swapped
 *   bit since we have finished marking needs rebuild
 *   
 * @param packet_p     
 * @param raid_group_p 
 * @param disk_position             
 *
 * @return fbe_status_t     
 *
 * @author 
 *   09/05/2011 - Created. Ashwin Tamilarasan. 
 *
 ******************************************************************************/
fbe_status_t fbe_raid_group_needs_rebuild_ack_mark_nr_done(
                                fbe_packet_t*      packet_p,
                                fbe_raid_group_t*  raid_group_p,
                                fbe_u32_t          disk_position)
{
    fbe_block_edge_t*        edge_p;    
    fbe_raid_geometry_t*     raid_geometry_p = NULL; 
    fbe_class_id_t           class_id; 
    
    
    /* Always set the completion function.  There are (2) cases:
     *  1. This is virtual drive and therefore we need to clear the 
     *     `mark NR required' non-paged flag.
     *
     *  2. This is not a virtual drive and need to clear the `swapped'
     *     flag and set the rebuild checkpoint to the end marker in the
     *     virtual drive.
     */
    fbe_transport_set_completion_function(packet_p, fbe_raid_group_needs_rebuild_ack_mark_nr_done_completion,
                                          raid_group_p);
    
    /* If this is a virtual drive clear the `mark NR required' in the non-paged.
     * Else send the `needs rebuild done' to the virtual drive.   
     */    
    raid_geometry_p = fbe_raid_group_get_raid_geometry(raid_group_p);
    class_id = fbe_raid_geometry_get_class_id(raid_geometry_p);     
    if (class_id == FBE_CLASS_ID_VIRTUAL_DRIVE)
    {   
        fbe_base_object_trace((fbe_base_object_t *) raid_group_p, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s Clear mark NR required \n",
                              __FUNCTION__);    
        
        /* Clear the `mark NR required' in the non-paged.
         */
        fbe_raid_group_metadata_clear_mark_nr_required(raid_group_p, packet_p);

        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }

    /* Set the usurper to the virtual drive to indicate that we are done marking
     * needs rebuild.
     */
    fbe_raid_group_allocate_and_build_control_operation(raid_group_p,packet_p,
                                                        FBE_VIRTUAL_DRIVE_CONTROL_CODE_MARK_NEEDS_REBUILD_DONE,
                                                        NULL /*not used */,
                                                        -1/*NOT USED*/);

           
    /* Send the packet to the VD object on this edge
     */
    fbe_base_config_get_block_edge((fbe_base_config_t*)raid_group_p, &edge_p, disk_position);
    fbe_block_transport_send_control_packet(edge_p, packet_p);
    
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;

} // End fbe_raid_group_needs_rebuild_ack_mark_nr_done()


/*!****************************************************************************
 * fbe_raid_group_needs_rebuild_ack_mark_nr_done_completion()
 ******************************************************************************
 * @brief
 *   This function will set the clear rb logging condition.
 *   
 * @param in_packet_p  
 * @param context       
 *
 * @return fbe_status_t     
 *
 * @author 
 *   09/01/2011 - Created. Ashwin Tamilarasan. 
 *
 ******************************************************************************/
fbe_status_t fbe_raid_group_needs_rebuild_ack_mark_nr_done_completion(
                                fbe_packet_t * packet_p,
                                fbe_packet_completion_context_t context)
{
    fbe_raid_group_t*                           raid_group_p = NULL;
    fbe_payload_ex_t *                         sep_payload_p = NULL;
    fbe_payload_control_operation_t *           control_operation_p = NULL;
    fbe_status_t                                status;
    fbe_payload_control_status_t                control_status;


    raid_group_p = (fbe_raid_group_t*) context;
    status = fbe_transport_get_status_code(packet_p);

    //  Get control status
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(sep_payload_p); 
    fbe_payload_control_get_status(control_operation_p, &control_status);    
    
    fbe_payload_ex_release_control_operation(sep_payload_p, control_operation_p);
    
    
    if(status != FBE_STATUS_OK || control_status != FBE_PAYLOAD_CONTROL_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *) raid_group_p, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s clearing of swap done failed, status:0x%x, crl_status:0x%x\n",
                              __FUNCTION__, status, control_status);
        return status;
    }    

    //  Set the condition to clear rb logging

    fbe_raid_group_lock(raid_group_p);
    fbe_raid_group_set_local_state(raid_group_p, FBE_RAID_GROUP_LOCAL_STATE_CLEAR_RL_REQUEST);
    fbe_lifecycle_set_cond(&fbe_raid_group_lifecycle_const, (fbe_base_object_t *) raid_group_p,
                            FBE_RAID_GROUP_LIFECYCLE_COND_CLEAR_RB_LOGGING);
    fbe_raid_group_unlock(raid_group_p);
    return FBE_STATUS_OK;


}// End fbe_raid_group_needs_rebuild_ack_mark_nr_done_completion



/*!****************************************************************************
 * fbe_raid_group_needs_rebuild_quiesce_io_if_needed()
 ******************************************************************************
 * @brief
 *   This function will check if I/O has been quiesced on the raid group.  If it
 *   has not, then it will set the quiesce I/O condition and return a status to 
 *   indicate that it must wait for the quiesce.  It will also set the unquiesce
 *   condition.  The unquiesce will only run after all of the other higher 
 *   priority conditions such as eval RL have completed.  
 * 
 * @param in_raid_group_p       - pointer to a raid group object
 *
 * @return  fbe_status_t  
 *
 * @author
 *   02/24/2010 - Created. Jean Montes. 
 *
 ******************************************************************************/
fbe_status_t fbe_raid_group_needs_rebuild_quiesce_io_if_needed(
                                    fbe_raid_group_t*   in_raid_group_p)
{
    fbe_bool_t  peer_alive_b;
    fbe_bool_t  local_quiesced;
    fbe_bool_t  peer_quiesced;

    //  Lock the RAID group before checking the quiesced flag
    fbe_raid_group_lock(in_raid_group_p);
    
    peer_alive_b = fbe_base_config_has_peer_joined((fbe_base_config_t *) in_raid_group_p);
    local_quiesced = fbe_base_config_is_clustered_flag_set((fbe_base_config_t *) in_raid_group_p, 
                                                           FBE_BASE_CONFIG_CLUSTERED_FLAG_QUIESCED);
    peer_quiesced = fbe_base_config_is_any_peer_clustered_flag_set((fbe_base_config_t *) in_raid_group_p, 
                                                               (FBE_BASE_CONFIG_CLUSTERED_FLAG_QUIESCED|
                                                                FBE_BASE_CONFIG_METADATA_MEMORY_QUIESCE_COMPLETE));

    //  If the RAID group is already quiesced, we don't need to do anything more - return with normal status
    //  after unlocking the raid group
    //  We also need to check peer side to make sure both sides are quiesced, if peer is alive
    if ((local_quiesced == FBE_TRUE) &&
        ((peer_alive_b != FBE_TRUE) || (peer_quiesced == FBE_TRUE)))
    {
        fbe_raid_group_unlock(in_raid_group_p);
        return FBE_STATUS_OK; 
    }

    //  Unlock the raid group
    fbe_raid_group_unlock(in_raid_group_p);

    fbe_base_object_trace((fbe_base_object_t *)in_raid_group_p,
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s set quiesce/unquiesce conditions\n", __FUNCTION__);

    //  Set a condition to quiese
    fbe_lifecycle_set_cond(&fbe_raid_group_lifecycle_const, (fbe_base_object_t*) in_raid_group_p,
                    FBE_BASE_CONFIG_LIFECYCLE_COND_QUIESCE);

    //  Set a conditions to unquiese.  This is located in the RAID group rotary after the eval RL 
    //  condition.  So the result is that the quiesce will be processed, then the eval_rl will be
    //  processed, and when it is done, the unquiesce will be processed. 
    fbe_lifecycle_set_cond(&fbe_raid_group_lifecycle_const, (fbe_base_object_t*) in_raid_group_p,
                    FBE_BASE_CONFIG_LIFECYCLE_COND_UNQUIESCE);

    /*  Next we clear out an error attributes marked on downstream edges, which are holding our view of the drive state 
     * as bad. 
     */
    fbe_lifecycle_set_cond(&fbe_raid_group_lifecycle_const, (fbe_base_object_t*) in_raid_group_p,
                    FBE_RAID_GROUP_LIFECYCLE_COND_CLEAR_ERRORS);

    //  Return pending.  This signifies that the caller needs to wait for the quiesce to complete before 
    //  proceeding. 
    return FBE_STATUS_PENDING; 

} // End fbe_raid_group_needs_rebuild_quiesce_io_if_needed()

/*!****************************************************************************
 * fbe_raid_group_needs_rebuild_create_rebuild_nonpaged_data()
 ******************************************************************************
 * @brief 
 *   This function populates a rebuild nonpaged info structure with the data 
 *   needed to set/clear the RL bitmask for the position(s) indicated.
 *   It will also set the rebuild checkpoint to 0 in the structure for the
 *   given positions if the RL bits are being set.  The structure can 
 *   then be used in a non-paged metadata write by the caller. 
 *
 * @notes
 *   This functions should only be called when setting or clearing rb logging.
 *   The function assumes that the RG is quiesced and so the rebuild nonpaged  
 *   metadata is not being changed by any other entity. 
 *
 * @param in_raid_group_p           - pointer to the raid group object 
 * @param in_positions_to_change    - bitmask indicating which disk position(s)  
 *                                    to change the checkpoint for 
 * @param in_perform_clear_b        - set to TRUE if the bits should be cleared
 *                                    instead of set 
 * @param out_rebuild_data_p        - pointer to rebuild nonpaged data structure 
 *                                    that gets filled in by this function
 *
 * @return fbe_status_t         
 *
 * @author
 *   11/30/2010 - Created. Jean Montes. 
 *
 ******************************************************************************/
fbe_status_t 
fbe_raid_group_needs_rebuild_create_rebuild_nonpaged_data(fbe_raid_group_t*                       in_raid_group_p,
                                                          fbe_raid_position_bitmask_t             in_positions_to_change,
                                                          fbe_bool_t                              in_perform_clear_b, 
                                                          fbe_raid_group_rebuild_nonpaged_info_t* out_rebuild_data_p)

{
    fbe_status_t                        status;                                  
    fbe_u32_t                           width;                                  // raid group width
    fbe_u32_t                           disk_position;                          // current disk position  
    fbe_raid_position_bitmask_t         cur_position_mask;                      // mask of current disk position
    fbe_u32_t                           rebuild_data_index;                     // rebuild checkpoint entry index
                                                                                //   to be written to 
    fbe_u32_t                           start_index;                            // starting checkpoint entry index


    //  Get all of the rebuild non-paged data into a local struct
    status = fbe_raid_group_rebuild_get_nonpaged_info(in_raid_group_p, out_rebuild_data_p);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*) in_raid_group_p, FBE_TRACE_LEVEL_ERROR, 
            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, "%s error %d getting nonpaged data\n", __FUNCTION__, status);
        return status;
    }

    //  See if we are starting or stopping rb logging.  If we are stopping, then just set up the new bitmask in the
    //  output structure and return we are done.
    if (in_perform_clear_b == FBE_TRUE)
    {
        out_rebuild_data_p->rebuild_logging_bitmask = out_rebuild_data_p->rebuild_logging_bitmask & 
            (~in_positions_to_change); 
        return FBE_STATUS_OK; 
    }

    //  rb logging is being set.  The rebuild checkpoint needs to be set to 0 for each position specified as well as
    //  setting the rebuild logging bits.

    //  Set up the new bitmask in the output structure 
    out_rebuild_data_p->rebuild_logging_bitmask = out_rebuild_data_p->rebuild_logging_bitmask | in_positions_to_change;

    //  Get the width of raid group so we can loop through the disks 
    fbe_base_config_get_width((fbe_base_config_t*) in_raid_group_p, &width);

    //  Find the disk(s) indicated by the positions to change mask
    start_index = 0; 
    for (disk_position = 0; disk_position < width; disk_position++)
    {
        //  Create a bitmask for this position
        cur_position_mask = 1 << disk_position; 

        //  If this disk position does not need its checkpoint changed, the loop can move on 
        if ((in_positions_to_change & cur_position_mask) == 0)
        {
            continue;
        }

        //  We have a position to update.  First go through the rebuild checkpoints to find if this entry  
        //  already exists.  That would occur if the drive was still rebuilding and went down again before the 
        //  rebuild finished. 
        fbe_raid_group_rebuild_find_existing_checkpoint_entry_for_position(in_raid_group_p, disk_position, 
            &rebuild_data_index);
            
        //  See if we found an existing entry.  If not, then find the next open entry.
        if (rebuild_data_index == FBE_RAID_GROUP_INVALID_INDEX)
        {
            fbe_raid_group_rebuild_find_next_unused_checkpoint_entry(in_raid_group_p, start_index, 
                &rebuild_data_index);

            //  Adjust the start index so that next time we search for an open entry, we don't get the same entry
            //  that we already allocated for this position
            start_index = rebuild_data_index + 1; 
        }

        //  If no entry was found either way, then we have a problem - return error
        if (rebuild_data_index == FBE_RAID_GROUP_INVALID_INDEX)
        {
            fbe_base_object_trace((fbe_base_object_t*) in_raid_group_p, FBE_TRACE_LEVEL_ERROR, 
                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, "%s no rebuild checkpoint entry found\n", __FUNCTION__);
            return FBE_STATUS_GENERIC_FAILURE; 
        }

        //  We have the entry to use.  Set the checkpoint to 0 in the output structure. 
        out_rebuild_data_p->rebuild_checkpoint_info[rebuild_data_index].checkpoint = 0;

        //  Set the position value in the output structure 
        out_rebuild_data_p->rebuild_checkpoint_info[rebuild_data_index].position = disk_position; 

        /* Reset the `blocks_rebuilt' for this rebuild index.
         */
        fbe_raid_group_trace(in_raid_group_p,
                             FBE_TRACE_LEVEL_INFO,
                             FBE_RAID_GROUP_DEBUG_FLAG_REBUILD_NOTIFICATION_TRACING,
                             "raid_group: RB set rl for index: %d pos: %d reset blocks rebuilt: 0x%llx to 0 \n",
                             rebuild_data_index, disk_position, 
                             in_raid_group_p->raid_group_metadata_memory.blocks_rebuilt[rebuild_data_index]);
         in_raid_group_p->raid_group_metadata_memory.blocks_rebuilt[rebuild_data_index] = 0;
    }

    //  Return success   
    return FBE_STATUS_OK;

} // End fbe_raid_group_needs_rebuild_create_rebuild_nonpaged_data()

/*!****************************************************************************
 * fbe_raid_group_get_failed_pos_bitmask()
 ******************************************************************************
 * @brief
 *   This function returns a bitmask of the failed drives in the given RG
 *   based on the health of the associated edges.
 *
 * @param in_raid_group_p             - pointer to a raid group object
 * @param out_position_bitmask_p      - pointer to the aggregated bitmask for
 *                                      all the failed drives 
 *                                
 *
 * @return fbe_status_t         
 *
 * @author
 *   07/07/2010 - Created. Ashwin Tamilarasan.
 *
 *****************************************************************************/
fbe_status_t fbe_raid_group_get_failed_pos_bitmask(fbe_raid_group_t *in_raid_group_p,
                                                   fbe_u16_t *out_position_bitmask_p)
{

    fbe_base_config_t*              base_config_p;              // pointer to a base config object
    fbe_block_edge_t*               edge_p;                     // pointer to an edge 
    fbe_u32_t                       edge_index;                 // index to the current edge
    fbe_path_state_t                path_state;                 // path state of the edge 
    fbe_u32_t                       width;                      // width of RG
    fbe_path_attr_t                 path_attrib;

    *out_position_bitmask_p = 0;

    //  Set up a pointer to the object as a base config object 
    base_config_p = (fbe_base_config_t*) in_raid_group_p; 

    //  Get the width of the object
    fbe_base_config_get_width(base_config_p, &width);

    for (edge_index = 0; edge_index < width; edge_index++)
    {        
        fbe_base_config_get_block_edge(base_config_p, &edge_p, edge_index);
        fbe_block_transport_get_path_state(edge_p, &path_state);
        fbe_block_transport_get_path_attributes(edge_p, &path_attrib);

        //  Check if the path state (edge state) is disabled, broken, gone, or invalid. 
        // If it is add it to the position bitmask
        // We also consider the edge bad if we have the timeout error attributes set.
        if ((path_state == FBE_PATH_STATE_DISABLED)                  ||
            (path_state == FBE_PATH_STATE_BROKEN)                    ||
            (path_state == FBE_PATH_STATE_GONE)                      ||
            (path_state == FBE_PATH_STATE_INVALID)                   ||
            (path_attrib & FBE_BLOCK_PATH_ATTR_FLAGS_TIMEOUT_ERRORS)    )
        {
            *out_position_bitmask_p |= (1 << edge_index);
        }
    }
    /* Treat new PVD as failed */
    if (fbe_raid_group_is_c4_mirror_raid_group(in_raid_group_p)) {
        fbe_u32_t new_pvd_bitmap = 0;
        fbe_raid_group_c4_mirror_get_new_pvd(in_raid_group_p, &new_pvd_bitmap);
        *out_position_bitmask_p |= new_pvd_bitmap;
    }

    return FBE_STATUS_OK;
} 
/**************************************
 * end fbe_raid_group_get_failed_pos_bitmask()
 **************************************/

/*!****************************************************************************
 * fbe_raid_group_get_disabled_pos_bitmask()
 ******************************************************************************
 * @brief
 *   This function returns a bitmask of the disabled drives in the given RG
 *   based on the health of the associated edges.
 *
 * @param raid_group_p             - pointer to a raid group object
 * @param position_bitmask_p      - pointer to the aggregated bitmask for
 *                                      all the diabled drives
 *
 * @return fbe_status_t         
 *
 * @author
 *  1/28/2013 - Created. Rob Foley
 *
 *****************************************************************************/
fbe_status_t fbe_raid_group_get_disabled_pos_bitmask(fbe_raid_group_t *raid_group_p,
                                                     fbe_u16_t *position_bitmask_p)
{
    fbe_base_config_t*              base_config_p = NULL;
    fbe_block_edge_t*               edge_p = NULL;
    fbe_u32_t                       edge_index;
    fbe_path_state_t                path_state;
    fbe_u32_t                       width;
    fbe_path_attr_t                 path_attrib;

    *position_bitmask_p = 0;
    base_config_p = (fbe_base_config_t*) raid_group_p; 
    fbe_base_config_get_width(base_config_p, &width);

    for (edge_index = 0; edge_index < width; edge_index++)
    {        
        fbe_base_config_get_block_edge(base_config_p, &edge_p, edge_index);
        fbe_block_transport_get_path_state(edge_p, &path_state);
        fbe_block_transport_get_path_attributes(edge_p, &path_attrib);

        /* We consider the edge disabled only if the timeout errs bit is not set. 
         * Timeout errs bit makes the raid group become degraded for this position 
         * long enough to start rebuild logging. 
         * The timeout errs bit can only be cleared by the raid group and only when 
         * the raid group finishes eval mark nr.
         */
        if ((path_state == FBE_PATH_STATE_DISABLED) &&
            (path_attrib & FBE_BLOCK_PATH_ATTR_FLAGS_TIMEOUT_ERRORS) == 0)
        {
            *position_bitmask_p |= (1 << edge_index);
        }
    }
    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_raid_group_get_disabled_pos_bitmask()
 **************************************/


/*!****************************************************************************
 * fbe_raid_group_get_disk_pos_by_server_id()
 ******************************************************************************
 * @brief
 *   This function returns a bitmask of the failed drives in the given RG
 *   based on the health of the associated edges.
 *
 * @param in_raid_group_p             - pointer to a raid group object
 * @param out_position_bitmask_p      - pointer to the aggregated bitmask for
 *                                      all the failed drives 
 *                                
 *
 * @return fbe_status_t         
 *
 * @author
 *   05/09/2012 - Created. Ashwin Tamilarasan.
 *
 *****************************************************************************/
fbe_status_t fbe_raid_group_get_disk_pos_by_server_id(fbe_raid_group_t *raid_group_p,
                                                      fbe_object_id_t   object_to_check,
                                                      fbe_u32_t   *disk_position)
{

    fbe_base_config_t*              base_config_p;             
    fbe_block_edge_t*               edge_p;                    
    fbe_u32_t                       edge_index;                
    fbe_path_state_t                path_state;                
    fbe_u32_t                       width;                     
    fbe_path_attr_t                 path_attrib;
    fbe_object_id_t                 object_id;

    //  Set up a pointer to the object as a base config object 
    base_config_p = (fbe_base_config_t*) raid_group_p; 

    //  Get the width of the object
    fbe_base_config_get_width(base_config_p, &width);

    //  Loop through all the edges attached to this object 
    for (edge_index = 0; edge_index < width; edge_index++)
    {        

        //  Get the edge's path state 
        fbe_base_config_get_block_edge(base_config_p, &edge_p, edge_index);
        fbe_block_transport_get_path_state(edge_p, &path_state);
        fbe_block_transport_get_path_attributes(edge_p, &path_attrib);
        fbe_base_transport_get_server_id((fbe_base_edge_t*)edge_p, &object_id);

        if(object_id == object_to_check)
        {
            *disk_position = edge_index;
            return FBE_STATUS_OK;
        }
    } // end for 

    return FBE_STATUS_GENERIC_FAILURE;

} // End fbe_raid_group_get_disk_pos_by_server_id()

/*!****************************************************************************
 * fbe_raid_group_get_disk_pos_by_server_id()
 ******************************************************************************
 * @brief
 *   This function check the edge path state for the object mentioned to see 
 * if we can mark NR for the object's position
 *
 * @param in_raid_group_p             - pointer to a raid group object
 * @param object_to_check             - Path state of the object
 * @param ok_to_mark                  - Boolean to indicate if okay to mark NR                               
 *
 * @return fbe_status_t         
 *
 * @author
 *   06/09/2012 - Created. Ashok Tamilarasan.
 *
 *****************************************************************************/
fbe_status_t fbe_raid_group_rebuild_ok_to_mark_nr(fbe_raid_group_t *raid_group_p,
                                                  fbe_object_id_t   object_to_check,
                                                  fbe_bool_t   *ok_to_mark)
{

    fbe_base_config_t*              base_config_p;             
    fbe_block_edge_t*               edge_p;                    
    fbe_u32_t                       edge_index;                
    fbe_path_state_t                path_state;                
    fbe_u32_t                       width;                     
    fbe_path_attr_t                 path_attrib;
    fbe_object_id_t                 object_id;

    //  Set up a pointer to the object as a base config object 
    base_config_p = (fbe_base_config_t*) raid_group_p; 

    //  Get the width of the object
    fbe_base_config_get_width(base_config_p, &width);

    *ok_to_mark = FBE_FALSE;

    //  Loop through all the edges attached to this object 
    for (edge_index = 0; edge_index < width; edge_index++)
    {        

        //  Get the edge's path state 
        fbe_base_config_get_block_edge(base_config_p, &edge_p, edge_index);
        fbe_block_transport_get_path_state(edge_p, &path_state);
        fbe_block_transport_get_path_attributes(edge_p, &path_attrib);
        fbe_base_transport_get_server_id((fbe_base_edge_t*)edge_p, &object_id);

        //  Check if the path state (edge state) is disabled, broken, gone, or invalid. 
        // We can mark NR only for the below mentioned conditions otherwise the drive 
        // is good and we cannot mark NR
        // We also consider the edge bad if we have the timeout error attributes set.
        if ((path_state == FBE_PATH_STATE_DISABLED)                  ||
            (path_state == FBE_PATH_STATE_BROKEN)                    ||
            (path_state == FBE_PATH_STATE_GONE)                      ||
            (path_state == FBE_PATH_STATE_INVALID)                   ||
            (path_attrib & FBE_BLOCK_PATH_ATTR_FLAGS_TIMEOUT_ERRORS)    ) 

        {
            if(object_id == object_to_check)
            {
                *ok_to_mark = FBE_TRUE;
                return FBE_STATUS_OK;
            }
        }

    } // end for 

    //  Return success 
    return FBE_STATUS_OK;

} // End fbe_raid_group_get_disk_pos_by_server_id()

/*!****************************************************************************
 * fbe_raid_group_get_drives_marked_nr()
 ******************************************************************************
 * @brief
 *   This function returns a bitmask of the drives that are marked NR
 *
 * @param in_raid_group_p             - pointer to a raid group object
 * @param out_position_bitmask_p      - pointer to the aggregated bitmask for
 *                                      all the failed drives 
 *                                
 *
 * @return fbe_status_t         
 *
 * @author
 *   10/07/2011 - Created. Ashwin Tamilarasan.
 *
 *****************************************************************************/
fbe_status_t fbe_raid_group_get_drives_marked_nr(fbe_raid_group_t *in_raid_group_p,
                                                 fbe_u16_t *out_position_bitmask_p)
{

    fbe_base_config_t*              base_config_p;              
    fbe_block_edge_t*               edge_p;                     
    fbe_u32_t                       edge_index;                 
    fbe_path_state_t                path_state;                 
    fbe_u32_t                       width;                      
    fbe_path_attr_t                 path_attrib;
    fbe_lba_t                      checkpoint;

    *out_position_bitmask_p = 0;

    //  Set up a pointer to the object as a base config object 
    base_config_p = (fbe_base_config_t*) in_raid_group_p; 

    //  Get the width of the object
    fbe_base_config_get_width(base_config_p, &width);

    //  Loop through all the edges attached to this object 
    for (edge_index = 0; edge_index < width; edge_index++)
    {        

        //  Get the edge's path state 
        fbe_base_config_get_block_edge(base_config_p, &edge_p, edge_index);
        fbe_block_transport_get_path_state(edge_p, &path_state);
        fbe_block_transport_get_path_attributes(edge_p, &path_attrib);

        //  Check if the path state (edge state) is disabled, broken, gone, or invalid. 
        // If it is add it to the position bitmask
        // We also consider the edge bad if we have the timeout error attributes set.
        if ( ((path_state == FBE_PATH_STATE_ENABLED) ||
            (path_state == FBE_PATH_STATE_SLUMBER) ) &&   
            !(path_attrib & FBE_BLOCK_PATH_ATTR_FLAGS_TIMEOUT_ERRORS))

        {
            fbe_raid_group_get_rebuild_checkpoint(in_raid_group_p, edge_index, &checkpoint);
            if(checkpoint != FBE_LBA_INVALID)
            {
                *out_position_bitmask_p |= (1 << edge_index);
            }
            
        }

    } // end for 

    //  Return success 
    return FBE_STATUS_OK;

} // End fbe_raid_group_get_drives_marked_nr()

/*!****************************************************************************
 * fbe_raid_group_get_failed_pos_bitmask_to_clear()
 ******************************************************************************
 * @brief
 *   This function returns a bitmask of the failed drives in the given RG
 *   for which rebuild logging can now be cleared.
 *
 * @param in_raid_group_p             - pointer to a raid group object
 * @param out_position_bitmask_p      - pointer to the aggregated bitmask for
 *                                      all the failed drives that can be cleared now 
 * @return fbe_status_t         
 *
 * @author
 *   10/2010 - Created. Susan Rundbaken (rundbs)
 *
 *****************************************************************************/
fbe_status_t 
fbe_raid_group_get_failed_pos_bitmask_to_clear(fbe_raid_group_t *in_raid_group_p, 
                                               fbe_raid_position_bitmask_t *out_position_bitmask_p)
{

    fbe_base_config_t*              base_config_p;              // pointer to a base config object
    fbe_block_edge_t*               edge_p;                     // pointer to an edge 
    fbe_u32_t                       edge_index;                 // index to the current edge
    fbe_path_state_t                path_state;                 // path state of the edge 
    fbe_u32_t                       width;                      // width of RG
    fbe_bool_t                      b_is_rb_logging;    
    fbe_path_attr_t                 path_attrib;
    fbe_u32_t                       new_pvd_bitmap = 0;

    //  Trace function entry 
    FBE_RAID_GROUP_TRACE_FUNC_ENTRY(in_raid_group_p);

    //  Set up a pointer to the object as a base config object 
    base_config_p = (fbe_base_config_t*) in_raid_group_p; 

    //  Get the width of the object
    fbe_base_config_get_width(base_config_p, &width);

    if (fbe_raid_group_is_c4_mirror_raid_group(in_raid_group_p)) {
        fbe_raid_group_c4_mirror_get_new_pvd(in_raid_group_p, &new_pvd_bitmap);
    }
    //  Loop through all the edges attached to this object 
    for (edge_index = 0; edge_index < width; edge_index++)
    {        
        //  Check if the disk is marked rebuild logging.  If not, then move on to the next disk/edge. 
        fbe_raid_group_get_rb_logging(in_raid_group_p, edge_index, &b_is_rb_logging); 
        if (b_is_rb_logging == FBE_FALSE)
        {
            fbe_raid_group_trace(in_raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_RAID_GROUP_DEBUG_FLAGS_TRACE_NEEDS_REBUILD,
                                 "%s - edge %d not rebuild logging\n", __FUNCTION__, edge_index);
            continue; 
        }
        

        //  Get the edge's path state 
        fbe_base_config_get_block_edge(base_config_p, &edge_p, edge_index);
        fbe_block_transport_get_path_state(edge_p, &path_state);
        fbe_block_transport_get_path_attributes(edge_p, &path_attrib);

        //  Check if the path state (edge state) is enabled.  
        // If not or if our timeout errors attribute is set, move on to the next edge.
        if ((path_state != FBE_PATH_STATE_ENABLED) ||
            (path_attrib & FBE_BLOCK_PATH_ATTR_FLAGS_TIMEOUT_ERRORS))
        {
            fbe_raid_group_trace(in_raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_RAID_GROUP_DEBUG_FLAGS_TRACE_NEEDS_REBUILD,
                                 "%s - edge %d has path_state %d\n", __FUNCTION__, edge_index, path_state);
            continue;
        }
        if ((1 << edge_index) & new_pvd_bitmap) {
            fbe_raid_group_trace(in_raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_RAID_GROUP_DEBUG_FLAGS_TRACE_NEEDS_REBUILD,
                                 "%s - edge %d is new PVD\n", __FUNCTION__, edge_index);
            continue;
        }

        *out_position_bitmask_p |= (1 << edge_index);

    } // end for 

    //  Return success 
    return FBE_STATUS_OK;

} // End fbe_raid_group_get_failed_pos_bitmask_to_clear()


/*!****************************************************************************
 * fbe_raid_group_needs_rebuild_get_disk_position_to_mark_nr()
 ******************************************************************************
 * @brief
 *   This function finds the first disk/edge for which NR should be marked on
 *   on a specific range.  For now, for NR to be marked, the disk must be
 *   rebuild logging.
 *   
 *   This function may be temporary until events are queued to the monitor. 
 *
 *   If the function does not find a disk/edge, it sets the output parameter to 
 *   FBE_RAID_INVALID_DISK_POSITION.
 *
 * @param in_raid_group_p       - pointer to a raid group object
 * @param out_disk_index_p      - pointer which gets set to the index of the 
 *                                disk to mark NR; set to FBE_RAID_INVALID_DISK_
 *                                POSITION if none found
 *
 * @return fbe_status_t         
 *
 * @author
 *   06/03/2010 - Created. Jean Montes.
 *
 *****************************************************************************/
fbe_status_t fbe_raid_group_needs_rebuild_get_disk_position_to_mark_nr(
                                    fbe_raid_group_t*           in_raid_group_p,
                                    fbe_event_t*                data_event_p,
                                    fbe_u32_t*                  out_disk_index_p)
{

    fbe_base_edge_t*                base_edge_p;                    // pointer to the base edge
    fbe_edge_index_t                edge_index;                     // edge index to mark needs rebuild
    fbe_bool_t                      can_mark_specific_range_nr_b;   // can it update rebuild checkpoint?

    //  Trace function entry 
    FBE_RAID_GROUP_TRACE_FUNC_ENTRY(in_raid_group_p);

    //  Initialize output parameter to no disk found 
    *out_disk_index_p = FBE_RAID_INVALID_DISK_POSITION;

    //  Get the edge pointer from the event
    fbe_event_get_edge(data_event_p, &base_edge_p);
    if (base_edge_p == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t*) in_raid_group_p, FBE_TRACE_LEVEL_ERROR, 
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, "%s - base edge pointer is null\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    //  Get the client index of the edge
    fbe_base_transport_get_client_index(base_edge_p, &edge_index);
    if(edge_index == FBE_EDGE_INDEX_INVALID)
    {
        fbe_base_object_trace((fbe_base_object_t*) in_raid_group_p, FBE_TRACE_LEVEL_ERROR, 
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, "%s - edge index in data event is invalid\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    //  Determine if we can mark this edge as needs rebuild for the specific range
    fbe_raid_group_needs_rebuild_determine_if_mark_specific_range_for_nr(in_raid_group_p, 
                                                                         edge_index,
                                                                         &can_mark_specific_range_nr_b);
    if(can_mark_specific_range_nr_b == FBE_FALSE)
    {
        return FBE_STATUS_OK;
    }

    /* update the edge index. */
    *out_disk_index_p = edge_index;

    //  Return success.  The output parameter has already been initialized to no disk found. 
    return FBE_STATUS_OK;

} // End fbe_raid_group_needs_rebuild_get_disk_position_to_mark_nr()

/*!****************************************************************************
 * fbe_raid_group_find_rl_disk()
 ******************************************************************************
 * @brief
 *   This function finds the first disk/edge which is enabled and rebuild logging.
 *
 *   If the function does not find a disk/edge, it sets the output parameter to 
 *   FBE_RAID_INVALID_DISK_POSITION.
 *
 * @param raid_group_p      - pointer to a raid group object
 * @param packet_p          - Pointer to monitor packet to use for vd request
 * @param out_disk_index_p  - pointer which gets set to the index of the 
 *                                disk that meets the criteria; set to 
 *                                FBE_RAID_INVALID_DISK_POSITION if none found
 *
 * @return fbe_status_t         
 *
 * @author
 *   07/28/2011 - Created. Ashwin tamilarasan
 *
 *****************************************************************************/
fbe_status_t fbe_raid_group_find_rl_disk(fbe_raid_group_t *raid_group_p,
                                         fbe_packet_t *packet_p,  
                                         fbe_u32_t *out_disk_index_p)
{
    fbe_status_t                status;
    fbe_base_config_t          *base_config_p;                      
    fbe_block_edge_t           *edge_p;                             
    fbe_edge_index_t            edge_index;                         
    fbe_path_state_t            path_state;                         
    fbe_bool_t                  b_is_rb_logging;    
    fbe_u32_t                   width;
    fbe_path_attr_t             path_attrib;
    fbe_raid_position_bitmask_t valid_position_bitmask = FBE_RAID_POSITION_BITMASK_ALL_SET;

    
    FBE_RAID_GROUP_TRACE_FUNC_ENTRY(raid_group_p);
    
    *out_disk_index_p = FBE_RAID_INVALID_DISK_POSITION;
    base_config_p = (fbe_base_config_t*) raid_group_p; 
    
    /* At any time the virtual drive could have (2) edge but be in pass-thru
     * mode.  Therefore get the `valid' bitmask.
     */
    if (base_config_p->base_object.class_id == FBE_CLASS_ID_VIRTUAL_DRIVE)
    {
        /* Get the bitmask of valid virtual drive edges.
         */
        status = fbe_raid_group_get_virtual_drive_valid_edge_bitmask(raid_group_p, packet_p,
                                                                     &valid_position_bitmask);
        if (status != FBE_STATUS_OK)
        {
            /* Generate an error trace and return `invalid position'
             */
            fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s get virtual drive valid edge bitmask failed - status: 0x%x \n",
                                  __FUNCTION__, status);
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }
    
    fbe_base_config_get_width(base_config_p, &width);
    
    for (edge_index = 0; edge_index < width; edge_index++)
    { 
        fbe_base_config_get_block_edge(base_config_p, &edge_p, edge_index);
        fbe_block_transport_get_path_state(edge_p, &path_state);
        fbe_block_transport_get_path_attributes(edge_p, &path_attrib);
       
        /* If this edge is not valid skip it.
         */
        if (((1 << edge_index) & valid_position_bitmask) == 0)
        {
            /* This is not a `valid' edge.
             */
            continue;
        }
        fbe_raid_group_get_rb_logging(raid_group_p, edge_index, &b_is_rb_logging); 
        if ((b_is_rb_logging == FBE_TRUE)                                           ||
            ((path_attrib & FBE_BLOCK_PATH_ATTR_FLAGS_DEGRADED_NEEDS_REBUILD) != 0)    )
        {
            /*! @note Position is either marked rebuild logging or needs rebuild.
             */
        }
        else
        {
            /* Else this edge is not marked rebuild logging 
             */
            continue;
        }


        /* Check if the path state (edge state) is enabled or if it is marked bad.  
         * If not or if our timeout errors attribute is set, move on to the next edge.
         */
        if ((path_state != FBE_PATH_STATE_ENABLED) ||
            (path_attrib & FBE_BLOCK_PATH_ATTR_FLAGS_TIMEOUT_ERRORS))
        {
            continue;
        }
    
        /* We have found the disk that meets the criteria.  Set the output and return.
         */ 
        *out_disk_index_p = edge_index; 
        return FBE_STATUS_OK;
    } 
     
    return FBE_STATUS_OK;

}
/**************************************
 * end fbe_raid_group_find_rl_disk
 **************************************/

/*!****************************************************************************
 * fbe_raid_group_needs_rebuild_process_needs_rebuild_event()
 ******************************************************************************
 * @brief
 * 
 * @param in_raid_group_p       - pointer to a raid group object
 * @param in_packet_p           - pointer to a control packet from the scheduler
 * @param in_data_event_p       - data event to mark needs rebuild
 * 
 * @return fbe_status_t        
 *
 * @author
 *   11/12/2009 - Created. Dhaval Patel
 *
 ******************************************************************************/
fbe_status_t fbe_raid_group_needs_rebuild_process_needs_rebuild_event(
                                    fbe_raid_group_t*   in_raid_group_p,
                                    fbe_event_t*        in_data_event_p,
                                    fbe_packet_t*       in_packet_p)
{
    fbe_lba_t                                   start_lba;                          // start lba of the event extent
    fbe_block_count_t                           block_count;                        // block count of the event extent
    fbe_event_stack_t*                          event_stack_p;                      // event stack
    fbe_edge_index_t                            edge_index;                         // edge index
    fbe_base_edge_t*                            base_edge_p;                        // pointer to the base edge
    fbe_bool_t                                  can_mark_specific_range_nr_b;       // can rebuild checkpoint update possible?
    fbe_status_t                                status;                             // status of the operation

    //  Get the current stack of event
    event_stack_p = fbe_event_get_current_stack(in_data_event_p);

    //  Get the extent of the event stack
    fbe_event_stack_get_extent(event_stack_p, &start_lba, &block_count);

    //  Get the edge pointer from the event
    fbe_event_get_edge(in_data_event_p, &base_edge_p);

    //  Get the client index of the edge
    fbe_base_transport_get_client_index(base_edge_p, (fbe_edge_index_t *) &edge_index);

    //  Determine if we can update the checkpoint in curent state of the RAID group object
    fbe_raid_group_needs_rebuild_determine_if_mark_specific_range_for_nr(in_raid_group_p,
                                                                         edge_index,
                                                                         &can_mark_specific_range_nr_b);
    if(can_mark_specific_range_nr_b == FBE_FALSE)
    {
        //  Pop the the data event and complete the event
        fbe_base_config_event_queue_pop_event((fbe_base_config_t *)in_raid_group_p, &in_data_event_p);

        //  Send deny response if processing needs rebuild event failed
        fbe_event_set_flags(in_data_event_p, FBE_EVENT_FLAG_DENY);
        fbe_event_complete(in_data_event_p);
    
        //  Clear the condition if event process failed
        fbe_lifecycle_clear_current_cond((fbe_base_object_t *) in_raid_group_p);
        return FBE_STATUS_OK;
    }

    //  Mark the specific range for needs rebuild, if required it will update the checkpoint
    status = fbe_lifecycle_set_cond(&fbe_raid_group_lifecycle_const, (fbe_base_object_t *) in_raid_group_p,
                                    FBE_RAID_GROUP_LIFECYCLE_COND_MARK_SPECIFIC_NR);

    //  Return good status
    return status;

} // End fbe_raid_group_needs_rebuild_process_needs_rebuild_event()

/*!****************************************************************************
 * fbe_raid_group_needs_rebuild_determine_if_mark_specific_range_for_nr()
 ******************************************************************************
 * @brief
 *   This function is used while processing the data event to mark needs rebuild
 *   and it determines whether it can update the checkpoint in current state
 *   of the raid group object.
 * 
 * @param in_raid_group_p                       - pointer to a raid group object
 * @param in_edge_index                         - edge index to mark for needs rebuild.
 * @param out_can_mark_specific_range_for_nr_b  - return whether we can mark needs
 *                                                rebuild for the specific range.
 * 
 * @return  fbe_status_t  
 *
 * @author
 *   11/29/2010 - Created. Dhaval Patel
 *
 ******************************************************************************/
fbe_status_t fbe_raid_group_needs_rebuild_determine_if_mark_specific_range_for_nr(
                                                fbe_raid_group_t *  in_raid_group_p,
                                                fbe_edge_index_t    in_edge_index,
                                                fbe_bool_t *        out_can_mark_for_nr_b)
{

    fbe_path_state_t            path_state;                 // path state
    fbe_bool_t                  is_degraded_b;              // if it has degraded set
    fbe_bool_t                  b_is_rb_logging;
    fbe_block_edge_t *          block_edge_p;               // pointer to the block edge

    // initialize mark needs rebuild as false
    *out_can_mark_for_nr_b = FBE_FALSE;

    fbe_raid_group_get_rb_logging(in_raid_group_p, in_edge_index, &b_is_rb_logging);
    if(b_is_rb_logging == FBE_TRUE)
    {
        return FBE_STATUS_OK;
    }

    //  Get the block edge of the specific edge index
    fbe_base_config_get_block_edge((fbe_base_config_t *) in_raid_group_p, &block_edge_p, in_edge_index);

    //  Determine edge state from which this event came from, if it is available then needs rebuild code code will
    //  eventually will update the checkpoint
    fbe_block_transport_get_path_state(block_edge_p, &path_state);
    if((path_state != FBE_PATH_STATE_ENABLED) && (path_state != FBE_PATH_STATE_SLUMBER))
    {
        return FBE_STATUS_OK;
    }

    //  Deterine if raid group is degraded, if it is degraded then it cannot update the checkpoint on other drives
    //  for proactive copy errors, so clear the condition with fail event status
    is_degraded_b = fbe_raid_group_is_degraded(in_raid_group_p);
    if(is_degraded_b == FBE_TRUE)
    {
        return FBE_STATUS_OK;
    }

    //  It can be marked as needs rebuild
    *out_can_mark_for_nr_b = FBE_TRUE;

    //  Return good status
    return FBE_STATUS_OK;

} // End fbe_raid_group_needs_rebuild_determine_if_mark_specific_range_for_nr()

/*!****************************************************************************
 * fbe_raid_group_needs_rebuild_data_event_update_checkpoint_if_needed()
 ******************************************************************************
 * @brief
 *   This function will look at the data event and determine if we need to update
 *   rebuild checkpoint, if it requires to update checkpoint then it will first
 *   quiesce the i/o and then it will update the checkpoint
 * 
 * @param in_raid_group_p       - pointer to a raid group object
 * @param in_packet_p           - pointer to a control packet from the scheduler 
 * 
 * @return  fbe_status_t  
 *
 * @author
 *   11/19/2010 - Created. Dhaval Patel
 *
 ******************************************************************************/
fbe_status_t fbe_raid_group_needs_rebuild_data_event_update_checkpoint_if_needed(
                                    fbe_raid_group_t*   in_raid_group_p,
                                    fbe_event_t *       data_event_p,
                                    fbe_packet_t*       in_packet_p)
{

    fbe_lba_t                                   start_lba;                          // start lba of the event extent
    fbe_block_count_t                           block_count;                        // block count of the event extent
    fbe_event_stack_t*                          event_stack_p;                      // event stack
    fbe_edge_index_t                            edge_index;                         // edge index
    fbe_base_edge_t*                            base_edge_p;                        // base edge pointer of event
    fbe_lba_t                                   rebuild_checkpoint;                 // rebuid checkpoint of specific edge
    fbe_status_t                                status;                             // status of the operation
    fbe_bool_t                                  rg_is_going_broken_b;               // is rg going broken?
    fbe_bool_t                                  is_timeout_expired_b;               // it indicates time out expired boolean
    fbe_time_t                                  elapsed_seconds;                    // elapsed seconds
    fbe_bool_t                                  can_mark_specific_range_nr_b;       // it determine can checkpoint be updated?
    fbe_raid_iots_t                             *iots_p = NULL;
    fbe_payload_ex_t                           *sep_payload_p = NULL;

    //  Get the current stack of event
    event_stack_p = fbe_event_get_current_stack(data_event_p);

    //  Get the extent of the event stack
    fbe_event_stack_get_extent(event_stack_p, &start_lba, &block_count);

    //  Get the edge pointer from the event
    fbe_event_get_edge(data_event_p, &base_edge_p);

    //  Get the client index of the edge
    fbe_base_transport_get_client_index(base_edge_p, (fbe_edge_index_t *) &edge_index);

    //  Determine if we need checkpoint update for the edge in current state of the raid group object
    fbe_raid_group_needs_rebuild_determine_if_mark_specific_range_for_nr(in_raid_group_p,
                                                                         edge_index, &can_mark_specific_range_nr_b);

    //  If rebuild checkpoint update is not required then clear the condition
    if(can_mark_specific_range_nr_b == FBE_FALSE)
    {
        fbe_lifecycle_clear_current_cond((fbe_base_object_t *) in_raid_group_p);
        return FBE_STATUS_OK;
    }

    //  Get the current rebuild checkpoint of this edge to find out whether the checkpoint is already less
    //  than the start lba of needs rebuild data event
    fbe_raid_group_get_rebuild_checkpoint(in_raid_group_p, edge_index, &rebuild_checkpoint);
    if(rebuild_checkpoint <= start_lba)
    {
        fbe_lifecycle_clear_current_cond((fbe_base_object_t *) in_raid_group_p);
        return FBE_STATUS_OK;
    }

    //  Determine if updating checkpoint on this drive causes RAID group to go broken, if it causes RAID group to
    //  go broken then we cannot update the checkpoint
    fbe_raid_group_needs_rebuild_is_rg_going_broken(in_raid_group_p, &rg_is_going_broken_b);
    if (rg_is_going_broken_b)
    {
        //  wait for the timeout before going degraded - RG may go broken
        fbe_raid_group_needs_rebuild_determine_rg_broken_timeout(in_raid_group_p,
                                                                 FBE_RAID_GROUP_MAX_TIME_WAIT_FOR_BROKEN,
                                                                 &is_timeout_expired_b, &elapsed_seconds);

        if(!is_timeout_expired_b)
        {
            return FBE_STATUS_PENDING;
        }
        else
        {
            fbe_base_object_trace((fbe_base_object_t*) in_raid_group_p, FBE_TRACE_LEVEL_ERROR, 
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY, "%s - RG 0x%x should be broken!! Waited %llu seconds\n", 
                                  __FUNCTION__, in_raid_group_p->base_config.base_object.object_id, (unsigned long long)elapsed_seconds);
        }
    }

    //  If got this far, either we've waited long enough, or raid group is not broken. Reset the timestamp.
    fbe_raid_group_set_check_rg_broken_timestamp(in_raid_group_p, 0);

    //  If I/O is not already quiesed, set a condition to quiese I/O and and return with a status to indicate that
    //  we are not yet done.  When the quiesce has completed we'll re-enter this function and proceed through here.
    status = fbe_raid_group_needs_rebuild_quiesce_io_if_needed(in_raid_group_p);
    if (status == FBE_STATUS_PENDING)
    {
        return status;
    }

    
    sep_payload_p = fbe_transport_get_payload_ex(in_packet_p);
    fbe_payload_ex_get_iots(sep_payload_p, &iots_p);
    fbe_raid_iots_set_rebuild_logging_bitmask(iots_p, (1 << edge_index));
    fbe_raid_iots_set_lba(iots_p, start_lba);
    /* Get the NP clustered lock before updating the rebuild checkpoint */
    fbe_raid_group_get_NP_lock(in_raid_group_p, in_packet_p, fbe_raid_group_set_rebuild_checkpoint);    
    
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;

} // End fbe_raid_group_needs_rebuild_process_update_checkpoint_if_needed()


/*!****************************************************************************
 * fbe_raid_group_set_rebuild_checkpoint()
 ******************************************************************************
 * @brief
 *   This function will update the rebuild checkpoint and persist it
 * 
 * @param packet_p
 * @param context
 * 
 * @return  fbe_status_t  
 *
 * @author
 *   10/27/2011 - Created. Ashwin Tamilarasan
 *
 ******************************************************************************/
fbe_status_t fbe_raid_group_set_rebuild_checkpoint(fbe_packet_t * packet_p,                                    
                                        fbe_packet_completion_context_t context)

{
    fbe_raid_group_t*                        raid_group_p = NULL;
    fbe_status_t                             status;
    fbe_raid_group_rebuild_nonpaged_info_t   not_used;
    fbe_u32_t                                rebuild_index;
    fbe_u64_t                                metadata_offset;
    fbe_raid_iots_t*                         iots_p = NULL;
    fbe_payload_ex_t*                       sep_payload_p = NULL;
    fbe_raid_position_bitmask_t              pos_bitmask;
    fbe_lba_t                                rebuild_checkpoint;

    
    raid_group_p = (fbe_raid_group_t*)context;

    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    fbe_payload_ex_get_iots(sep_payload_p, &iots_p);

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
    

    /* Now we are done getting the lock, write the np.
     */
    fbe_raid_iots_get_rebuild_logging_bitmask(iots_p, &pos_bitmask);
    fbe_raid_iots_get_lba(iots_p, &rebuild_checkpoint);
    
    //  Populate a non-paged rebuild data structure with the rebuild checkpoint info to write out 
    status = fbe_raid_group_rebuild_create_checkpoint_data(raid_group_p, rebuild_checkpoint, pos_bitmask, 
                                                           &not_used, &rebuild_index); 

    metadata_offset = (fbe_u64_t) (&((fbe_raid_group_nonpaged_metadata_t*)0)->rebuild_info.rebuild_checkpoint_info[rebuild_index]);

    fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s:Setting the rebuild chkpt. rebuild_chk:0x%llx\n",
                          __FUNCTION__, (unsigned long long)rebuild_checkpoint);

    // Set the completion function to release the NP lock
    fbe_transport_set_completion_function(packet_p, fbe_raid_group_release_NP_lock, raid_group_p);
    status = fbe_base_config_metadata_nonpaged_set_checkpoint_persist((fbe_base_config_t *) raid_group_p,
                                                                      packet_p,
                                                                      metadata_offset,
                                                                      0, /* Currently we only set (1) checkpoint at-a-time.*/
                                                                      rebuild_checkpoint);
                                                        
    
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
               
} // End fbe_raid_group_set_rebuild_checkpoint()


/*!****************************************************************************
 * fbe_raid_group_needs_rebuild_process_needs_rebuild_request()
 ******************************************************************************
 * @brief
 *   This function will mark NR on the entire RG when the condition has been set 
 *   to do so. When the drive is enabled, the condition "evaluate
 *   mark NR" will call to evaluate if drive needs to mark for NR. If it evaluates
 *   that the drive which has just become enabled is a new drive, then it needs to be marked NR on the 
 *   whole extent, and the "mark NR for RG" condition gets set. 
 *
 *   When this function finishes marking NR, it will set the condition to clear RL. 
 * 
 * @param in_raid_group_p       - pointer to a raid group object
 * @param in_packet_p           - pointer to a control packet from the scheduler
 * 
 * @return fbe_status_t        
 *
 * @author
 *   11/05/2009 - Created. Jean Montes.
 *
 ******************************************************************************/
fbe_status_t fbe_raid_group_needs_rebuild_process_nr_request(
                                    fbe_raid_group_t*   in_raid_group_p,
                                    fbe_packet_t*       in_packet_p)

{
    fbe_status_t                                status = FBE_STATUS_OK;                         // status of the operation
    fbe_bool_t                                  is_span_across_user_data_b;     // if mark nr range span user and metadata area.
    fbe_lba_t                                   exported_disk_capacity;         // exported disk capacity or raid group
    fbe_raid_group_needs_rebuild_context_t*     needs_rebuild_context_p = NULL;        // pointer to the needs rebuild context
    fbe_block_count_t                           block_count = 0;                    // block count to mark needs rebuid
    fbe_lba_t                                   capacity_per_disk;  //  per-disk capacity of the raid group 

    //  Trace function entry 
    FBE_RAID_GROUP_TRACE_FUNC_ENTRY(in_raid_group_p);

    //  Get the control buffer from the packet payload
    status = fbe_raid_group_usurper_get_control_buffer(in_raid_group_p,
                                                       in_packet_p, 
                                                       sizeof(fbe_raid_group_needs_rebuild_context_t),
                                                       (fbe_payload_control_buffer_t) &needs_rebuild_context_p);
    if (status != FBE_STATUS_OK) 
    { 
        fbe_transport_set_status(in_packet_p, status, 0);
        fbe_transport_complete_packet(in_packet_p);
        return status; 
    }

    /* Truncate request to end of metadata.  Parity raid group types contain
     * a journal area following the metadata area.  This area does not and
     * should not be rebuilt.
     */
    capacity_per_disk = fbe_raid_group_get_disk_capacity(in_raid_group_p);
    if ((needs_rebuild_context_p->start_lba + needs_rebuild_context_p->block_count) > capacity_per_disk)
    {        
        /* Log an informational message. */
        fbe_base_object_trace((fbe_base_object_t*)in_raid_group_p, 
                              FBE_TRACE_LEVEL_INFO, 
                              FBE_TRACE_MESSAGE_ID_INFO, 
                              "raid_group: mark nr lba: 0x%llx blks: 0x%llx spans per disk capacity: 0x%llx\n", 
                              (unsigned long long)needs_rebuild_context_p->start_lba, (unsigned long long)needs_rebuild_context_p->block_count,
                              (unsigned long long)capacity_per_disk);

        /* Truncate the request to the per disk capacity.
         */
        if (needs_rebuild_context_p->start_lba >= capacity_per_disk)
        {
            /* No truncation required. Just complete the packet.
             */
            fbe_transport_set_status(in_packet_p, FBE_STATUS_OK, 0);
            fbe_transport_complete_packet(in_packet_p);
            return FBE_STATUS_OK;
        }
        
        /* Truncate the request.
         */
        needs_rebuild_context_p->block_count = capacity_per_disk - needs_rebuild_context_p->start_lba;
    }

    //  Determine if lba range span across the user data boundary
    fbe_raid_group_bitmap_determine_if_lba_range_span_user_and_paged_metadata(in_raid_group_p,
        needs_rebuild_context_p->start_lba, needs_rebuild_context_p->block_count, &is_span_across_user_data_b);

    //  If it spans across user data boundary then first update only paged metadata for user data range
    if(is_span_across_user_data_b)
    {
        //  Get the number of chunks per disk in the user RG data area (exported area)
        exported_disk_capacity = fbe_raid_group_get_exported_disk_capacity(in_raid_group_p);
        block_count = (fbe_block_count_t) (exported_disk_capacity - needs_rebuild_context_p->start_lba);

        //  Set the completion function to process user data range completion 
        fbe_transport_set_completion_function(in_packet_p, fbe_raid_group_needs_rebuild_handle_mark_nr_for_user_data_completion, 
                in_raid_group_p);
    }
    else
    {
        block_count = needs_rebuild_context_p->block_count;
        //  Set the completion function to process user data range completion 
        fbe_transport_set_completion_function(in_packet_p, fbe_raid_group_needs_rebuild_handle_mark_nr_completion, 
                in_raid_group_p);
    }
       
    fbe_raid_group_trace(in_raid_group_p,
                         FBE_TRACE_LEVEL_INFO, 
                         FBE_RAID_GROUP_DEBUG_FLAG_NONE,
                         "raid_group: needs rebuild mark nr start: 0x%llx blks: 0x%llx nr mask: 0x%x\n",
                         (unsigned long long)needs_rebuild_context_p->start_lba, (unsigned long long)block_count, needs_rebuild_context_p->nr_position_mask); 

    // Get the stripe lock for the region we are tring to mark NR
    fbe_transport_set_completion_function(in_packet_p, fbe_raid_group_process_and_release_block_operation, in_raid_group_p);
    status = fbe_raid_group_allocate_block_operation_and_send_to_executor(in_raid_group_p,
                                                                 in_packet_p,
                                                                 needs_rebuild_context_p->start_lba,
                                                                 block_count,
                                                                 FBE_PAYLOAD_BLOCK_OPERATION_MARK_FOR_REBUILD);
    
        
    return FBE_STATUS_MORE_PROCESSING_REQUIRED; 

} // End fbe_raid_group_needs_rebuild_process_nr_request()

/*!****************************************************************************
 * fbe_raid_group_needs_rebuild_handle_mark_nr_for_user_data_completion()
 ******************************************************************************
 * @brief
 *   This function is called when the paged metadata for the user data range
 *   is completed, it will actually send a request to update the nonpaged
 *   metadata if required.
 * 
 * @param in_packet_p   - pointer to a control packet from the scheduler
 * @param in_context    - completion context, which is a pointer to the raid 
 *                        group object
 * 
 * @return  fbe_status_t  
 *
 * @author
 *   05/17/2010 - Created. Dhaval Patel. 
 *
 ******************************************************************************/
static fbe_status_t fbe_raid_group_needs_rebuild_handle_mark_nr_for_user_data_completion(
                                    fbe_packet_t*                   in_packet_p,
                                    fbe_packet_completion_context_t in_context)

{

    fbe_raid_group_t*                       raid_group_p;               // pointer to raid group object 
    fbe_status_t                            status;                     // overall status from the packet
    fbe_raid_group_needs_rebuild_context_t* needs_rebuild_context_p;    // needs rebuild context
    fbe_block_count_t                       block_count;                // block count to represent the nr range
    fbe_lba_t                               exported_capacity;          // exported capacity of the raid object
    fbe_lba_t                               capacity_per_disk;          // configured capacity of RAID group per disk

    //  Get the raid group pointer from the input context 
    raid_group_p = (fbe_raid_group_t*) in_context; 

    //  Check the packet status.  If it had an error, then we want to return here instead of proceeding.
    status = fbe_transport_get_status_code(in_packet_p);
    if (status != FBE_STATUS_OK)
    {
        return status;
    }

    //  Get the control buffer from the packet payload
    status = fbe_raid_group_rebuild_get_needs_rebuild_context(raid_group_p, in_packet_p, &needs_rebuild_context_p);       
    if (status != FBE_STATUS_OK) 
    { 
        fbe_transport_set_status(in_packet_p, status, 0);
        return status; 
    }

    //  Get the capacity per disk
    capacity_per_disk = fbe_raid_group_get_disk_capacity(raid_group_p);

    //  Get the number of chunks per disk in the user RG data area
    exported_capacity = fbe_raid_group_get_exported_disk_capacity(raid_group_p);

    //  If lba range exceeds configured capacity of the RAID object the trim it
    if ((needs_rebuild_context_p->start_lba + needs_rebuild_context_p->block_count) > capacity_per_disk) 
    {
        // It represents entire paged metadata range
        block_count = (fbe_block_count_t) (capacity_per_disk - exported_capacity);
    }
    else
    {
        // It represents partial metadata range
        block_count = needs_rebuild_context_p->block_count - 
            (fbe_block_count_t) (exported_capacity - needs_rebuild_context_p->start_lba);
    }

    //  Update the request to mark nr for metadata range
    needs_rebuild_context_p->start_lba = exported_capacity;
    needs_rebuild_context_p->block_count = block_count;

    fbe_raid_group_mark_nr_control_operation(raid_group_p, in_packet_p, fbe_raid_group_needs_rebuild_handle_mark_nr_completion);

    //  Return more processing since we have a packet outstanding 
    return FBE_STATUS_MORE_PROCESSING_REQUIRED; 

} // End fbe_raid_group_needs_rebuild_handle_mark_nr_for_user_data_completion()

/*!****************************************************************************
 * fbe_raid_group_needs_rebuild_handle_mark_nr_completion()
 ******************************************************************************
 * @brief
 *   This function is called when the paged metadata write to mark NR has 
 *   completed.  This is a common completion function used whether marking NR 
 *   on the whole extent or on a specific range. 
 * 
 * @param in_packet_p   - pointer to a control packet from the scheduler
 * @param in_context    - completion context, which is a pointer to the raid 
 *                        group object
 * 
 * @return  fbe_status_t  
 *
 * @author
 *   05/17/2010 - Created. Jean Montes. 
 *
 ******************************************************************************/
static fbe_status_t fbe_raid_group_needs_rebuild_handle_mark_nr_completion(
                                    fbe_packet_t*                   in_packet_p,
                                    fbe_packet_completion_context_t in_context)

{
    fbe_raid_group_t*                       raid_group_p;               // pointer to raid group object 
    fbe_status_t                            status;                     // overall status from the packet

    //  Get the raid group pointer from the input context 
    raid_group_p = (fbe_raid_group_t*) in_context; 

    //  Check the packet status.  If it had an error, then we want to return here instead of proceeding.
    status = fbe_transport_get_status_code(in_packet_p);
    if (status != FBE_STATUS_OK)
    {
        return status;
    }

    // Return good status to allow packet to complete at next level
    return FBE_STATUS_OK;

} // End fbe_raid_group_needs_rebuild_handle_mark_nr_completion()


/*!****************************************************************************
 * fbe_raid_group_needs_rebuild_clear_nr_for_iots()
 ******************************************************************************
 * @brief
 *   This function will clear NR on a range of chunks, for the disk position(s) 
 *   indicated by the bitmask.  The range cleared is based on the start LBA and
 *   the block count.  
 *
 * @param in_raid_group_p       - pointer to a raid group object
 * @param iots_p                - pointer to iots.
 *
 * @return  fbe_status_t  
 *
 * @author
 *   06/13/2012 - Created. Prahlad Purohit - Created using refactored code.
 *
 ******************************************************************************/
fbe_status_t fbe_raid_group_needs_rebuild_clear_nr_for_iots(
                                    fbe_raid_group_t*           in_raid_group_p,
                                    fbe_raid_iots_t*            iots_p)
{
    fbe_lba_t                           start_lba;
    fbe_lba_t                           end_lba;
    fbe_chunk_index_t                   start_chunk;
    fbe_block_count_t                   block_count;
    fbe_chunk_index_t                   end_chunk;
    fbe_chunk_count_t                   chunk_count;
    fbe_raid_group_paged_metadata_t     chunk_data;
    fbe_raid_position_bitmask_t         clear_nr_position_mask;
    fbe_packet_t                        *packet_p = NULL;
    fbe_raid_geometry_t                 *raid_geometry_p;
    fbe_lba_t                           rg_lba;
    fbe_u16_t                           disk_count;

      //  Trace function entry 
    FBE_RAID_GROUP_TRACE_FUNC_ENTRY(in_raid_group_p);

    /* Update the NR information for this iots range.
     */
    packet_p = fbe_raid_iots_get_packet(iots_p);

    start_lba = iots_p->lba;
    block_count = iots_p->blocks;

    //  Set up a bitmask of the positions for which NR needs to be cleared.  If any of the positions were marked rebuild
    //  logging in the I/O, then they were not rebuilt and so NR should not be cleared.
    clear_nr_position_mask = iots_p->chunk_info[0].needs_rebuild_bits & ~(iots_p->rebuild_logging_bitmask);       

    //  Get the ending LBA to be marked.
    end_lba = start_lba + block_count - 1;

    //  Convert the starting and ending LBAs to chunk indexes 
    fbe_raid_group_bitmap_get_chunk_index_for_lba(in_raid_group_p, start_lba, &start_chunk);
    fbe_raid_group_bitmap_get_chunk_index_for_lba(in_raid_group_p, end_lba, &end_chunk);

    //  Calculate the number of chunks to be marked 
    chunk_count = (fbe_chunk_count_t) (end_chunk - start_chunk + 1);

    fbe_raid_group_trace(in_raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_RAID_GROUP_DEBUG_FLAGS_TRACE_PMD_NR,
                         "clear nr for pkt: 0x%x lba %llx to %llx chunk %llx to %llx, pos 0x%x\n", 
                         (fbe_u32_t)packet_p, (unsigned long long)start_lba, (unsigned long long)end_lba, (unsigned long long)start_chunk, (unsigned long long)end_chunk, clear_nr_position_mask);

    //  Set up the bits of the metadata that need to be written, which are the NR bits 
    fbe_set_memory(&chunk_data, 0, sizeof(fbe_raid_group_paged_metadata_t));
    chunk_data.needs_rebuild_bits = clear_nr_position_mask;

    /* Clear the verify bits since a rebuild of this chunk is as good as a verify.
     */
    chunk_data.verify_bits = FBE_RAID_BITMAP_VERIFY_ALL;

    raid_geometry_p = fbe_raid_group_get_raid_geometry(in_raid_group_p);
    fbe_raid_geometry_get_data_disks(raid_geometry_p, &disk_count);

    rg_lba = start_lba * disk_count;

    //  If the chunk is not in the data area, then use the non-paged metadata service to update it
    if (fbe_raid_geometry_is_metadata_io(raid_geometry_p, rg_lba) ||
        !fbe_raid_geometry_has_paged_metadata(raid_geometry_p))
    {
        fbe_raid_group_bitmap_write_chunk_info_using_nonpaged(in_raid_group_p, packet_p, start_chunk,
                                                              chunk_count, &chunk_data, FBE_TRUE);
    }
    else
    {
        //  The chunks are in the user data area.  Use the paged metadata service to update them. 
        fbe_raid_group_bitmap_update_paged_metadata(in_raid_group_p, packet_p, start_chunk,
                                                    chunk_count, &chunk_data, FBE_TRUE);
    }

    //  Return more processing since we have a packet outstanding 
    return FBE_STATUS_OK;

} // End fbe_raid_group_needs_rebuild_clear_nr_for_iots()

/*!****************************************************************************
 * fbe_raid_group_needs_rebuild_is_parity_rg_going_broken()
 ******************************************************************************
 * @brief
 *  This function checks for failed I/Os in the RG to determine if the RG
 *  is going broken. This is applicable to parity RGs.
 *
 * @param in_raid_group_p       - pointer to a raid group object
 * @param out_is_rg_broken_b_p  - pointer to boolean indicating if RG is going broken or not
 *
 * @return fbe_status_t
 *
 * @author
 *  6/2010 - Created. rundbs
 *
 ******************************************************************************/
fbe_status_t
fbe_raid_group_needs_rebuild_is_rg_going_broken(
                                        fbe_raid_group_t*   in_raid_group_p, 
                                        fbe_bool_t*         out_is_rg_broken_b_p)
{
    fbe_raid_geometry_t*                        raid_geometry_p;
    fbe_u16_t                                   num_data_disks;
    fbe_u16_t                                   num_parity_disks;
    fbe_u32_t                                   width;
    fbe_u16_t                                   failed_disk_position_bitmap;
    fbe_u16_t                                   failed_io_position_bitmap;
    fbe_u32_t                                   number_of_edges_down;
    fbe_base_config_path_state_counters_t       path_state_counters;
    fbe_u32_t                                   bitcount;

    //  Trace function entry 
    FBE_RAID_GROUP_TRACE_FUNC_ENTRY(in_raid_group_p);

    //  Initialize output variables
    *out_is_rg_broken_b_p  = FALSE;

    //  Initialize local variables
    failed_disk_position_bitmap     = 0;
    failed_io_position_bitmap       = 0;

    //  Get the number of parity disks in the RG
    raid_geometry_p = fbe_raid_group_get_raid_geometry((fbe_raid_group_t*) in_raid_group_p);
    fbe_raid_geometry_get_data_disks(raid_geometry_p, &num_data_disks);
    fbe_raid_geometry_get_width(raid_geometry_p, &width);
    num_parity_disks = width - num_data_disks;

    //  If the number of parity disks is zero, return.
    //  The failed I/Os check is applicable only to parity RAID groups since that is where
    //  we can run into problems if we allow RL processing to continue if the RG is going broken. Some I/Os may continue
    //  to other chunks, causing incoherent stripes to remain incoherent unexpectedly. I/O after a rebuild in this case
    //  can result in media errors (data loss). This can happen if multiple drives are pulled and reinserted.
    if (num_parity_disks <= 0)
    {
        return FBE_STATUS_OK;
    }

    //  Determine the health of all the downstream edges 
    fbe_raid_group_determine_edge_health((fbe_raid_group_t*) in_raid_group_p, &path_state_counters);

    //  Set the number of edges which are unavailable 
    number_of_edges_down = path_state_counters.disabled_counter + path_state_counters.broken_counter +
                           path_state_counters.invalid_counter;

    //  If the RG is going broken, set the output parameter to TRUE and return
    if (number_of_edges_down > num_parity_disks) 
    {
        fbe_base_object_trace((fbe_base_object_t*) in_raid_group_p, FBE_TRACE_LEVEL_INFO, 
                FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY, "%s - RG broken; edges down: 0x%x, parity_disks: 0x%x\n", __FUNCTION__, 
                number_of_edges_down, num_parity_disks);
        *out_is_rg_broken_b_p = TRUE;
        return FBE_STATUS_OK;
    }

    //  If got this far, the parity RG does not appear to be going broken based on edge health; check for failed I/O

    //  Get a bitmap of the failed positions in the RG based on its I/O tracking structures
    fbe_raid_group_find_failed_io_positions(in_raid_group_p, &failed_io_position_bitmap);

    //  If the number of positions with failed I/Os is enough to make the RG go broken, set the output
    //  parameter to TRUE and return.
    bitcount = fbe_raid_get_bitcount(failed_io_position_bitmap);
    if (bitcount > num_parity_disks)
    {
        fbe_base_object_trace((fbe_base_object_t*) in_raid_group_p, FBE_TRACE_LEVEL_INFO, 
                FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY, "%s - RG broken; failed I/O disk pos: 0x%x\n", __FUNCTION__, 
                failed_io_position_bitmap);
        *out_is_rg_broken_b_p = TRUE;
        return FBE_STATUS_OK;
    }

    fbe_raid_group_get_failed_pos_bitmask(in_raid_group_p, &failed_disk_position_bitmap);
    //  If a combination of edge failure and failed I/O will make the RG go broken, set the output parameter to TRUE
    failed_disk_position_bitmap |= failed_io_position_bitmap;
    bitcount = fbe_raid_get_bitcount(failed_disk_position_bitmap);
    if (bitcount > num_parity_disks) 
    {
        fbe_base_object_trace((fbe_base_object_t*) in_raid_group_p, FBE_TRACE_LEVEL_INFO, 
                FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY, "%s - RG broken; failed edge and I/O disk pos: 0x%x\n", __FUNCTION__, 
                failed_disk_position_bitmap);
        *out_is_rg_broken_b_p = TRUE;
    }

    //  Return good status
    return FBE_STATUS_OK;

}  // End fbe_raid_group_needs_rebuild_is_parity_rg_going_broken()

/*!****************************************************************************
 * fbe_raid_group_activate_set_rb_logging()
 ******************************************************************************
 * @brief
 *  This function should look at the downstream health to see whether we
 *  need to mark rl on any drive. If there is a drive to be marked rl
 *  it will mark rl.
 * 
 * @param raid_group_p           - pointer to raid object.
 * @param packet_p               - pointer to a control packet.
 * 
 * @return fbe_lifecycle_status_t 
 *
 * @author
 *  8/08/2011 - Created. Ashwin Tamilarasan
 *
 ******************************************************************************/
fbe_lifecycle_status_t fbe_raid_group_activate_set_rb_logging(fbe_raid_group_t * raid_group_p, fbe_packet_t * packet_p)
{
    fbe_raid_position_bitmask_t        failed_pos_bitmap = 0;
    fbe_raid_position_bitmask_t        rl_bitmask = 0;

    /* If failed bitmask is not a subset of NP bitmask
       that means we have some drive that needs to be marked for rebuild logging    
    */
    fbe_raid_group_get_failed_pos_bitmask(raid_group_p, &failed_pos_bitmap);    
    fbe_raid_group_get_rb_logging_bitmask(raid_group_p, &rl_bitmask);        

    if ((rl_bitmask | failed_pos_bitmap) != rl_bitmask)
    {
        fbe_raid_group_trace(raid_group_p, 
                             FBE_TRACE_LEVEL_INFO, 
                             FBE_RAID_GROUP_DEBUG_FLAG_NONE,
                             "%s: Set rl for failed: 0x%x current: 0x%x in activate state\n", 
                             __FUNCTION__, failed_pos_bitmap, rl_bitmask);        
        fbe_transport_set_completion_function(packet_p, fbe_raid_group_write_rl_completion,
                                              (fbe_packet_completion_context_t)raid_group_p);                                               
        fbe_raid_group_write_rl_get_dl(raid_group_p, packet_p, failed_pos_bitmap,
                                FBE_FALSE/* used for clear rb logging*/);

        return FBE_LIFECYCLE_STATUS_PENDING;
    }
    else if (fbe_raid_group_is_extended_media_error_handling_capable(raid_group_p) &&
             (rl_bitmask != 0)                                                        ) 
    {
        /* If we have set rl, send usurper to disable media error thresholds.  
         * This is only done on the active side.  The condition will
         * coordinate with the peer so that the EMEH condition is also set on the
         * passive side.
         */
        fbe_raid_emeh_command_t emeh_request;

        /* If enabled send the condition to send the EMEH (Extended Media Error 
         * Handling) usurper to remaining PDOs.
         */
        fbe_raid_group_lock(raid_group_p);
        fbe_raid_group_get_emeh_request(raid_group_p, &emeh_request);
        if (emeh_request != FBE_RAID_EMEH_COMMAND_INVALID)
        {
            /* Since a new failed position may have been discovered which we
             * will set the EMEH set degraded condition for, when we come thru
             * here a second time the EMEH request will already be set.  Therefore
             * this is simply an informational trace.
             */
            fbe_raid_group_unlock(raid_group_p);
            fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "emeh activate set rl: %d already in progress \n",
                                  emeh_request);
        }
        else
        {
            /* Else set the request type and condition
             */
            fbe_raid_group_set_emeh_request(raid_group_p, FBE_RAID_EMEH_COMMAND_RAID_GROUP_DEGRADED);
            fbe_raid_group_unlock(raid_group_p);
            fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "emeh activate rl set: %d emeh request\n",
                                  FBE_RAID_EMEH_COMMAND_RAID_GROUP_DEGRADED);
            fbe_lifecycle_set_cond(&fbe_raid_group_lifecycle_const, (fbe_base_object_t *)raid_group_p,
                                   FBE_RAID_GROUP_LIFECYCLE_COND_EMEH_REQUEST);
        }
    }

    return FBE_LIFECYCLE_STATUS_DONE;


} // fbe_raid_group_activate_set_rb_logging()


/*!****************************************************************************
 * fbe_rg_needs_rebuild_mark_nr_from_usurper()
 ******************************************************************************
 * @brief
 *   This function will mark NR on the drive according to the checkpoint
 *
 * 
 * @param in_raid_group_p       - pointer to a raid group object
 * @param in_packet_p           - pointer to a control packet from the scheduler
 * @param in_disk_position      - disk that is marked for NR
 * @param in_start_lba          - checkpoint to start the rebuild from
 * @param block_count           - number of block to rebuild
 * 
 * @return  fbe_status_t  
 *
 * @author
 *   05/09/2012 - Created. Ashwin Tamilarasan
 *
 ******************************************************************************/
fbe_status_t fbe_rg_needs_rebuild_mark_nr_from_usurper(
                                        fbe_raid_group_t*   in_raid_group_p,                                        
                                        fbe_packet_t *      in_packet_p,
                                        fbe_u32_t           in_disk_position,
                                        fbe_lba_t           in_start_lba,
                                        fbe_block_count_t   in_block_count,
                                        fbe_raid_group_needs_rebuild_context_t*     needs_rebuild_context_p)
{
    /*! @note  This should be fixed.  Set the debug flag that disables the 
     *         `update paged NR validation'.  We should be updating the
     *         non-paged before updating the paged metadata.
     */
    fbe_raid_group_set_debug_flag(in_raid_group_p, FBE_RAID_GROUP_DEBUG_FLAG_DISABLE_UPDATE_NR_VALIDATION);

    //  Initialize rebuild context before we start the rebuild opreation
    fbe_raid_group_needs_rebuild_initialize_needs_rebuild_context(needs_rebuild_context_p, in_start_lba,
                 in_block_count, in_disk_position);
    
    //  Call a function to mark NR for the RAID object
    fbe_raid_group_needs_rebuild_process_nr_request(in_raid_group_p, in_packet_p); 
    
    return FBE_STATUS_MORE_PROCESSING_REQUIRED; 

} // End fbe_rg_needs_rebuild_mark_nr_from_usurper()


/*!****************************************************************************
 * fbe_rg_needs_rebuild_mark_nr_from_usurper_completion()
 ******************************************************************************
 * @brief
 *   This function will frees up any control operation we have allocated.
 *   If the mark NR was successful, we update the buffer we got in raid group
 *   usurper with success
 *   
 * @param in_packet_p     - Pointer to a control packet from the scheduler.
 * @param context       
 *
 * @return fbe_status_t     - Return FBE_STATUS_OK on success/condition will
 *                            be cleared.
 *
 * @author 
 *   05/09/2012 - Created. Ashwin Tamilarasan. 
 *
 ******************************************************************************/
fbe_status_t fbe_rg_needs_rebuild_mark_nr_from_usurper_completion(
                                fbe_packet_t * in_packet_p,
                                fbe_packet_completion_context_t context)
{

    fbe_raid_group_t *                          raid_group_p;
    fbe_payload_ex_t *                         sep_payload_p;
    fbe_payload_control_operation_t *           control_operation_p;
    fbe_raid_group_needs_rebuild_context_t*     needs_rebuild_context_p;
    fbe_status_t                                status;   
    fbe_u32_t                                   disk_position;
    fbe_base_config_drive_to_mark_nr_t          *drive_to_mark_nr_p = NULL;
    fbe_payload_control_status_t                control_status;


    //  Get pointer to raid group object
    raid_group_p = (fbe_raid_group_t *) context;

    //  Get the status of the packet
    status = fbe_transport_get_status_code(in_packet_p);

    //  Get control status
    sep_payload_p = fbe_transport_get_payload_ex(in_packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(sep_payload_p);
    fbe_payload_control_get_status(control_operation_p, &control_status);
    
    fbe_payload_control_get_buffer(control_operation_p, &needs_rebuild_context_p);
    disk_position = needs_rebuild_context_p->position;
    fbe_transport_release_buffer(needs_rebuild_context_p);
    fbe_payload_ex_release_control_operation(sep_payload_p, control_operation_p);

    /* Get the usurper control buffer */
    fbe_raid_group_usurper_get_control_buffer(raid_group_p,
                                              in_packet_p, 
                                              sizeof(fbe_base_config_drive_to_mark_nr_t),
                                              (fbe_payload_control_buffer_t)&drive_to_mark_nr_p);

    if((status != FBE_STATUS_OK) || (control_status != FBE_PAYLOAD_CONTROL_STATUS_OK))
    {        
        fbe_base_object_trace((fbe_base_object_t *) raid_group_p, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s marking of nr failed, status:0x%x \n",
                              __FUNCTION__, status);
        drive_to_mark_nr_p->nr_status = FBE_FALSE;

        fbe_transport_set_status(in_packet_p, status, 0);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Marking of NR was successful. Update the nr status field */
    drive_to_mark_nr_p->nr_status = FBE_TRUE;
    
    fbe_base_object_trace((fbe_base_object_t *) raid_group_p, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s marking of nr for drive 0x%x successful\n",
                              __FUNCTION__, disk_position); 
   
    /*! @note  This should be fixed.  Clear the debug flag that disables the 
     *         `update paged NR validation'.  We should be updating the
     *         non-paged before updating the paged metadata.
     */
    fbe_raid_group_clear_debug_flag(raid_group_p, FBE_RAID_GROUP_DEBUG_FLAG_DISABLE_UPDATE_NR_VALIDATION);   

    return FBE_STATUS_OK;
    
} // End fbe_rg_needs_rebuild_mark_nr_from_usurper_completion()

/*!****************************************************************************
 * fbe_rg_needs_rebuild_force_mark_nr_set_checkpoint()
 ******************************************************************************
 * @brief
 *   This function tries to get the Non paged lock to set the NR checkpoint
 *   
 * @param in_packet_p     - Pointer to a control packet from the scheduler.
 * @param context       
 *
 * @return fbe_status_t     - status
 *
 * @author 
 *   06/18/2012 - Created. Ashok Tamilarasan. 
 *
 ******************************************************************************/
fbe_status_t fbe_rg_needs_rebuild_force_mark_nr_set_checkpoint(fbe_packet_t * in_packet_p,
                                                               fbe_packet_completion_context_t context)
{

    fbe_raid_group_t *                          raid_group_p;
    fbe_status_t                                status;   


    //  Get pointer to raid group object
    raid_group_p = (fbe_raid_group_t *) context;

    //  Get the status of the packet
    status = fbe_transport_get_status_code(in_packet_p);

    if(status != FBE_STATUS_OK)
    {        
        fbe_base_object_trace((fbe_base_object_t *) raid_group_p, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s marking of nr failed, status:0x%x",
                              __FUNCTION__, status);
        return status;
    }

    /* Need to get the NP lock before updating the checkpoint */
    fbe_raid_group_get_NP_lock(raid_group_p, in_packet_p, fbe_raid_group_needs_rebuild_force_nr_set_checkpoint_with_lock);

    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
} // End fbe_rg_needs_rebuild_force_mark_nr_set_checkpoint()

/*!****************************************************************************
 * fbe_raid_group_needs_rebuild_force_nr_set_checkpoint_with_lock()
 ******************************************************************************
 * @brief
 *   This function is called after getting the Non-paged lock to set the NR 
 * checkpoint to Zero
 * 
 * @param packet_p
 * @param context
 * 
 * @return  fbe_status_t  
 *
 * @author
 *   06/15/2012 - Created. Ashok Tamilarasan
 *
 ******************************************************************************/
static fbe_status_t fbe_raid_group_needs_rebuild_force_nr_set_checkpoint_with_lock(fbe_packet_t * packet_p,                                    
                                                                                   fbe_packet_completion_context_t context)

{
    fbe_raid_group_t*                    raid_group_p = NULL;   
    fbe_status_t                         status;
    fbe_payload_ex_t*                      sep_payload_p;              // pointer to the sep payload
    fbe_payload_control_operation_t*        control_operation_p;        // pointer to the control operation
    fbe_raid_group_needs_rebuild_context_t* needs_rebuild_context_p;    // needs rebuild context
                                                                        // 
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

    //  Get the control operation from the packet to get needs rebuild context
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_present_control_operation(sep_payload_p);
    fbe_payload_control_get_buffer(control_operation_p, &needs_rebuild_context_p);

    /* This is needed to avoid a deadlock. The usurper or monitor gets the NP lock
      and tries to mark a chunk for verify. The raid group is degraded. Since it is degraded
      first it will try to mark NR on the md of md which will try to get the NP lock and it wont get
      it since the verify has the lock. 
    */
    fbe_transport_set_packet_attr(packet_p, FBE_PACKET_FLAG_NP_LOCK_HELD);

    // Set the completion function to release the NP lock
    fbe_transport_set_completion_function(packet_p, fbe_raid_group_release_NP_lock, raid_group_p);

    fbe_raid_group_rebuild_force_nr_write_rebuild_checkpoint_info(raid_group_p, packet_p, 0, 0, needs_rebuild_context_p->nr_position_mask);
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
} // End fbe_raid_group_needs_rebuild_force_nr_set_checkpoint_with_lock()

/*!***************************************************************************
 *          fbe_raid_group_allow_virtual_drive_to_continue_degraded()
 ***************************************************************************** 
 * 
 * @brief   This function will determine if it is Ok to allow the virtual drive
 *          to continue thru Ready processing in the degraded state or not.
 *          The virtual drive is different than other raid group types in that
 *          if it is in pass-thru mode it is the responsibility of the parent
 *          raid group to rebuild the virtual drive (the virtual drive cannot
 *          rebuild itself in pass thru mode).
 *
 * 
 * @param   raid_group_p - pointer to a raid group object
 * @param   packet_p - Monitor packet to use for virtual drive usurper
 * @param   drives_marked_nr - Bitmask of drives marked needs rebuild
 * 
 * @return  bool - FBE_TRUE - Allow virtual drive to remain degraded (parent
 *                  raid group will rebuild).
 *               - FBE_FALSE - Mark the virtual drive broken.
 *
 * @author
 *  09/16/2012  Ron Proulx  - Created.
 *****************************************************************************/
fbe_bool_t fbe_raid_group_allow_virtual_drive_to_continue_degraded(fbe_raid_group_t *raid_group_p,
                                                                   fbe_packet_t *packet_p,
                                                                   fbe_raid_position_bitmask_t drives_marked_nr)
{
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_bool_t                          b_continue_degraded = FBE_FALSE;    /*! @note Default is to not allow it. */
    fbe_object_id_t                     object_id;
    fbe_class_id_t                      class_id;
    fbe_lba_t                           first_edge_checkpoint;
    fbe_lba_t                           second_edge_checkpoint;
    fbe_lba_t                           checkpoint;
    fbe_payload_ex_t                   *payload_p = NULL;
    fbe_payload_control_operation_t    *control_operation_p = NULL;
    fbe_edge_index_t                    upstream_edge_index;
    fbe_object_id_t                     upstream_object_id;
    fbe_block_edge_t                   *upstream_edge_p = NULL;
    fbe_path_state_t                    upstream_edge_path_state;
    fbe_path_attr_t                     upstream_edge_path_attr;
    fbe_virtual_drive_configuration_t   get_configuration;
    fbe_payload_control_status_t        control_status;
    fbe_virtual_drive_configuration_mode_t configuration_mode = FBE_VIRTUAL_DRIVE_CONFIG_MODE_UNKNOWN;

    /* Validate the class_id.
     */
    object_id = fbe_raid_group_get_object_id(raid_group_p);
    class_id = fbe_raid_group_get_class_id(raid_group_p);
     
    if (class_id != FBE_CLASS_ID_VIRTUAL_DRIVE)
    {
        /* Generate an error trace and return False.
         */
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "raid_group: allow virtual drive degraded invalid class id: %d \n",
                              class_id);
        return FBE_FALSE;
    }


    /* No get the configuration mode to determine if we are going to allow the
     * virtual drive to continue degraded or not.
     */
	payload_p = fbe_transport_get_payload_ex(packet_p);
	control_operation_p = fbe_payload_ex_allocate_control_operation(payload_p);
    if (control_operation_p == NULL)
    {
        /* Generate an error trace and return False.
         */
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "raid_group: allow virtual drive degraded failed to allocate control op.\n");
        return FBE_FALSE;
    }

    /* Get the upstream edge information.
     */
    fbe_block_transport_find_first_upstream_edge_index_and_obj_id(&raid_group_p->base_config.block_transport_server,
                                                                  &upstream_edge_index, &upstream_object_id);

    /* If there is no upstream edge return false.
     */
    if (upstream_object_id == FBE_OBJECT_ID_INVALID)
    {
        /* Generate an warning trace and return False.
         */
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "raid_group: allow virtual drive degraded - no upstream edge. Return False\n");
        return FBE_FALSE;
    }
    upstream_edge_p = (fbe_block_edge_t *)fbe_base_transport_server_get_client_edge_by_client_id((fbe_base_transport_server_t *)&raid_group_p->base_config.block_transport_server,
                                                                             upstream_object_id);


    /* Set the destination address.
     */
    fbe_transport_set_address(packet_p,
                              FBE_PACKAGE_ID_SEP_0,
                              FBE_SERVICE_ID_TOPOLOGY,
                              FBE_CLASS_ID_INVALID,
                              object_id); 

    /* Build the get configuration information.
     */
    fbe_payload_control_build_operation(control_operation_p,  
										FBE_VIRTUAL_DRIVE_CONTROL_CODE_GET_CONFIGURATION,
										&get_configuration, 
										sizeof(fbe_virtual_drive_configuration_t)); 
	fbe_payload_ex_increment_control_operation_level(payload_p);

    /* Send and wait sync.
     */
    fbe_transport_set_sync_completion_type(packet_p, FBE_TRANSPORT_COMPLETION_TYPE_MORE_PROCESSING_REQUIRED);
    fbe_block_transport_send_control_packet((fbe_block_edge_t *)upstream_edge_p, packet_p);  
	fbe_transport_wait_completion(packet_p);

    /* Check the completion status.
     */
	status = fbe_transport_get_status_code(packet_p);
	fbe_payload_control_get_status(control_operation_p, &control_status);
	fbe_payload_ex_release_control_operation(payload_p, control_operation_p);

    /* If the request failed trace the error and return false.
     */
	if ((status != FBE_STATUS_OK)                         ||
        (control_status != FBE_PAYLOAD_CONTROL_STATUS_OK)    ) 
    {
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "raid_group: allow virtual drive degraded failed to get config status: 0x%x control: 0x%x \n",
                              status, control_status);
        return FBE_FALSE;
    }

    /* First get the path state and path attributes of the upstream edge
     */
    configuration_mode = get_configuration.configuration_mode;
    fbe_block_transport_get_path_state(upstream_edge_p, &upstream_edge_path_state);
    fbe_block_transport_get_path_attributes(upstream_edge_p, &upstream_edge_path_attr);

    /* Now get the rebuild checkpoint for both the edges.
     */
    fbe_raid_group_get_rebuild_checkpoint(raid_group_p, FIRST_EDGE_INDEX, &first_edge_checkpoint);
    fbe_raid_group_get_rebuild_checkpoint(raid_group_p, SECOND_EDGE_INDEX, &second_edge_checkpoint);
    if ((configuration_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_FIRST_EDGE) ||
        (configuration_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_SECOND_EDGE)      )
    {
        checkpoint = first_edge_checkpoint;
    }
    else
    {
        checkpoint = second_edge_checkpoint;
    }

    /* Trace some information.
     */
    fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "raid_group: allow virtual drive degraded mode: %d upstream path state: %d path attr: 0x%x chkpt: 0x%llx\n",
                          configuration_mode, upstream_edge_path_state, upstream_edge_path_attr, (unsigned long long)checkpoint);

    if (get_configuration.b_job_in_progress)
    {
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "raid_group: Job in progress don't continue degraded\n");
        return FBE_FALSE;
    }
    /* Now based on the virtual drive configuration mode determine if we are allowed
     * degraded or not.
     */
    switch(configuration_mode)
    {
        case FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_FIRST_EDGE:
            /* If the first edge index is not marked `needs rebuild' or this
             * position is fully rebuilt, I shouldn't be here.
             */
            if (((drives_marked_nr & (1 << FIRST_EDGE_INDEX)) == 0) ||
                (first_edge_checkpoint == FBE_LBA_INVALID)             )
            {
                /* There will always be a race condition when the parent setting
                 * the rebuild checkpoint to the end marker.  Trace a warning and 
                 * allow the continue. 
                 */
                fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                                      FBE_TRACE_LEVEL_WARNING,
                                      FBE_TRACE_MESSAGE_ID_INFO,
                                      "raid_group: allow virtual drive degraded mode: %d index: %d dmnr: 0x%x chkpt: 0x%llx end\n",
                                      configuration_mode, FIRST_EDGE_INDEX, drives_marked_nr, first_edge_checkpoint); 
                return FBE_TRUE;
            }

            /* Else validate that `degraded needs rebuild' attribute is set.
             */
            if ((upstream_edge_path_attr & FBE_BLOCK_PATH_ATTR_FLAGS_DEGRADED_NEEDS_REBUILD) == 0)
            {
                /* The virtual drive should have set this flag.  If it is not
                 * set something is wrong.
                 */
                fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                                      FBE_TRACE_LEVEL_ERROR,
                                      FBE_TRACE_MESSAGE_ID_INFO,
                                      "raid_group: allow virtual drive degraded mode: %d dmnr: 0x%x chkpt: 0x%llx dnr not set.\n",
                                      configuration_mode, drives_marked_nr, first_edge_checkpoint); 
                return FBE_FALSE;
            }

            /* Otherwise return `True' (continue degraded).
             */
            b_continue_degraded = FBE_TRUE;
            break;

        case FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_SECOND_EDGE:
            /* If the second edge index is not marked `needs rebuild' or this
             * position is fully rebuilt, I shouldn't be here.
             */
            if (((drives_marked_nr & (1 << SECOND_EDGE_INDEX)) == 0) ||
                (second_edge_checkpoint == FBE_LBA_INVALID)             )
            {
                /* There will always be a race condition when the parent setting
                 * the rebuild checkpoint to the end marker.  Trace a warning and 
                 * allow the continue. 
                 */
                fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                                      FBE_TRACE_LEVEL_WARNING,
                                      FBE_TRACE_MESSAGE_ID_INFO,
                                      "raid_group: allow virtual drive degraded mode: %d index: %d dmnr: 0x%x chkpt: 0x%llx end\n",
                                      configuration_mode, SECOND_EDGE_INDEX, drives_marked_nr, second_edge_checkpoint); 
                return FBE_TRUE;
            }

            /* Else validate that `degraded needs rebuild' attribute is set.
             */
            if ((upstream_edge_path_attr & FBE_BLOCK_PATH_ATTR_FLAGS_DEGRADED_NEEDS_REBUILD) == 0)
            {
                /* The virtual drive should have set this flag.  If it is not
                 * set something is wrong.
                 */
                fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                                      FBE_TRACE_LEVEL_ERROR,
                                      FBE_TRACE_MESSAGE_ID_INFO,
                                      "raid_group: allow virtual drive degraded mode: %d dmnr: 0x%x chkpt: 0x%llx dnr not set.\n",
                                      configuration_mode, drives_marked_nr, second_edge_checkpoint); 
                return FBE_FALSE;
            }

            /* Otherwise return `True' (continue degraded).
             */
            b_continue_degraded = FBE_TRUE;
            break;

        case FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_FIRST_EDGE:
        case FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_SECOND_EDGE:
        default:
            /* Otherwise it is not ok to continue degraded (i.e. goto Fail).
             */
            fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "raid_group: allow virtual drive degraded mode: %d dmnr: 0x%x chkpt: 0x%llx/0x%llx\n",
                                  configuration_mode, drives_marked_nr, first_edge_checkpoint, second_edge_checkpoint);
            b_continue_degraded = FBE_FALSE;
    }

    /* Return if we should continue degraded or not.
     */
    return b_continue_degraded; 
}
/***************************************************************
 * end fbe_raid_group_allow_virtual_drive_to_continue_degraded()
 ***************************************************************/

/*!***************************************************************************
 *          fbe_raid_group_handle_virtual_drive_peer_nonpaged_write()
 ***************************************************************************** 
 * 
 * @brief   This function handles a non-paged update on hte peer on behalf of
 *          the virtual drive.  Currently the only action that maybe required
 *          is to clear the `degraded needs rebuild' upstream path attribute.
 *
 * 
 * @param   raid_group_p - pointer to a raid group object
 * 
 * @return  status
 *
 * @author
 *  04/14/2013  Ron Proulx  - Created.
 *****************************************************************************/
fbe_status_t fbe_raid_group_handle_virtual_drive_peer_nonpaged_write(fbe_raid_group_t *raid_group_p)
{
    fbe_class_id_t              class_id;
    fbe_bool_t                  b_is_degraded_needs_rebuild = FBE_FALSE;
    fbe_edge_index_t            upstream_edge_index;
    fbe_object_id_t             upstream_object_id;
    fbe_block_edge_t           *upstream_edge_p = NULL;
    fbe_path_state_t            upstream_edge_path_state;
    fbe_path_attr_t             upstream_edge_path_attr;

    /* Validate the class_id.
     */
    class_id = fbe_raid_group_get_class_id(raid_group_p);
    if (class_id != FBE_CLASS_ID_VIRTUAL_DRIVE)
    {
        /* Generate an error trace and return False.
         */
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "raid_group: handle virtual drive peer nonpaged write - invalid class id: %d \n",
                              class_id);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Get the upstream edge information and the current value of `degraded
     * needs rebuild'
     */
    fbe_block_transport_find_first_upstream_edge_index_and_obj_id(&raid_group_p->base_config.block_transport_server,
                                                                  &upstream_edge_index, &upstream_object_id);
    fbe_raid_group_metadata_is_degraded_needs_rebuild(raid_group_p, &b_is_degraded_needs_rebuild);

    /* If there is no upstream edge there is nothing to do.
     */
    if (upstream_object_id == FBE_OBJECT_ID_INVALID)
    {
        /* Generate an warning trace and return False.
         */
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "raid_group: handle virtual drive peer nonpaged write - no upstream object\n");
        return FBE_STATUS_OK;
    }
    upstream_edge_p = (fbe_block_edge_t *)fbe_base_transport_server_get_client_edge_by_client_id((fbe_base_transport_server_t *)&raid_group_p->base_config.block_transport_server,
                                                                             upstream_object_id);
    fbe_block_transport_get_path_state(upstream_edge_p, &upstream_edge_path_state);
    fbe_block_transport_get_path_attributes(upstream_edge_p, &upstream_edge_path_attr);

    /* Currently we only need to take any action if the `degraded needs rebuild'
     * path attribute is set. 
     */
    if (((upstream_edge_path_attr & FBE_BLOCK_PATH_ATTR_FLAGS_DEGRADED_NEEDS_REBUILD) != 0) &&
        (b_is_degraded_needs_rebuild == FBE_FALSE)                                             )
    {
        /* Clear the upstream path attribute `degraded needs rebuild'
         * (it will only clear it if it is set).
         */
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "raid_group: handle virtual drive peer nonpaged write upstream obj: 0x%x degraded: %d clear degraded: 0x%x\n",
                              upstream_object_id, b_is_degraded_needs_rebuild, FBE_BLOCK_PATH_ATTR_FLAGS_DEGRADED_NEEDS_REBUILD);

        /* Else clear the upstream path attribute `degraded needs rebuild'
         * (it will only clear it if it is set).
         */
        fbe_block_transport_server_clear_path_attr_all_servers(&((fbe_base_config_t *)raid_group_p)->block_transport_server,
                                                               FBE_BLOCK_PATH_ATTR_FLAGS_DEGRADED_NEEDS_REBUILD);
    }

    /* Return success.
     */
    return FBE_STATUS_OK;
}
/***************************************************************
 * end fbe_raid_group_handle_virtual_drive_peer_nonpaged_write()
 ***************************************************************/

/*!***************************************************************************
 *          fbe_raid_group_mark_nr_from_usurper()
 ***************************************************************************** 
 * 
 * @brief   This function is invoked when the Needs Rebuild bitmask (in both
 *          the non-paged and the paged metadata) need to be updated via a
 *          `back door' (i..e for provision re-initialization, testing etc).
 *          The non-paged must be updated first then the paged can be updated.
 *
 * @param   raid_group_p - Raid group object
 * @param   packet_p - packet pointer.
 * 
 * @return  status
 *
 * @author
 *  03/14/2013  Ron Proulx  - Created
 *
 *****************************************************************************/
fbe_status_t fbe_raid_group_mark_nr_from_usurper(fbe_raid_group_t *raid_group_p,
                                                 fbe_packet_t *packet_p)
{
    fbe_base_config_drive_to_mark_nr_t         *drive_to_mark_nr_p = NULL;
    fbe_u32_t                                  disk_position = FBE_EDGE_INDEX_INVALID;
    fbe_lba_t                                  configured_capacity = FBE_LBA_INVALID;
    fbe_raid_group_needs_rebuild_context_t*     needs_rebuild_context_p = NULL;
    fbe_cpu_id_t                                cpu_id;

    /*! @note Move this into a separate method that is only invoked after 
     *        the non-paged is updated.
     */

    //  Allocate the needs rebuild control operation to manage the needs rebuild context
    fbe_raid_group_needs_rebuild_allocate_needs_rebuild_context(raid_group_p, packet_p, &needs_rebuild_context_p);
    if(needs_rebuild_context_p == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "fbe_rg_mark_nr: RL not set\n");
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_OK;
    }

    /* Check
     */
    if (drive_to_mark_nr_p == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "fbe_rg_mark_nr: RL not set\n");
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_OK;
    }

    /* Its packet coming from job service. The cpu id might not be
       filled out. Fill it up.
    */
    fbe_transport_get_cpu_id(packet_p, &cpu_id);
    if(cpu_id == FBE_CPU_ID_INVALID)
    {
        cpu_id = fbe_get_cpu_id();
        fbe_transport_set_cpu_id(packet_p, cpu_id);
    }
    //  Set the condition completion function before we mark NR
    fbe_transport_set_completion_function(packet_p, fbe_rg_needs_rebuild_mark_nr_from_usurper_completion, raid_group_p);
    if(drive_to_mark_nr_p->force_nr)
    {
        /* If force NR, we need to set the checkpoint to Zero so that rebuild can start */
        fbe_transport_set_completion_function(packet_p, fbe_rg_needs_rebuild_force_mark_nr_set_checkpoint, raid_group_p);
    }
    configured_capacity = fbe_raid_group_get_disk_capacity(raid_group_p);
    fbe_rg_needs_rebuild_mark_nr_from_usurper(raid_group_p, packet_p,
                                              disk_position, 0, configured_capacity,
                                              needs_rebuild_context_p);

    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/********************************************
 * end fbe_raid_group_mark_nr_from_usurper()
 ********************************************/


/******************************************************************************
 * end file fbe_raid_group_needs_rebuild.c
 ******************************************************************************/
