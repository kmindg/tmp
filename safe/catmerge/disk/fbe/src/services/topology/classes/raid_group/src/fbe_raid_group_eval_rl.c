/***************************************************************************
 * Copyright (C) EMC Corporation 2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/
 
/*!*************************************************************************
 * @file fbe_raid_group_eval_nl.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the raid group code that evaluates if rebuild logging
 *  needs to get set or not.
 * 
 * @version
 *   7/12/2011:  Created. Rob Foley
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe_raid_group_needs_rebuild.h"
#include "fbe_raid_group_bitmap.h"
#include "fbe_raid_group_object.h"
#include "fbe_raid_group_rebuild.h"
#include "fbe_transport_memory.h"
#include "fbe_service_manager.h"
#include "fbe_raid_geometry.h"
#include "fbe_cmi.h"
#include "fbe_raid_group_logging.h"
#include "fbe_private_space_layout.h"

/*****************************************
 * LOCAL GLOBALS
 *****************************************/

/*****************************************
 * LOCAL FUNCTION PROTOTYPES
 *****************************************/
static fbe_status_t fbe_raid_group_check_bitmap_and_continue(fbe_raid_group_t  *raid_group_p);
static fbe_status_t fbe_raid_group_write_rl_get_dl_completion(fbe_packet_t * packet_p, fbe_packet_completion_context_t context);
static fbe_status_t fbe_raid_group_write_rl_release_dl(fbe_packet_t * packet_p, fbe_packet_completion_context_t context);
static fbe_status_t fbe_raid_group_write_rl_update_nonpaged_drive_tier(fbe_packet_t * packet_p, fbe_packet_completion_context_t context);
/*************************
 *   FUNCTION DEFINITIONS
 *************************/


/*!****************************************************************************
 * fbe_raid_group_write_rl_get_dl()
 ******************************************************************************
 * @brief
 *  This function gets the distributed lock prior to .
 *
 * @param raid_group_p - Raid group object
 * @param packet_p - packet pointer.
 * @param failed_bitmap - bitmap to mark rebuild logging
 * @param clear_b - TRUE to clear rebuild logging bits
 *                 FALSE to set rebuild logging bits.
 * 
 * @return fbe_status_t
 *
 * @author
 *  10/21/2011 - Created. Rob Foley
 *
 ******************************************************************************/
fbe_status_t 
fbe_raid_group_write_rl_get_dl(fbe_raid_group_t *raid_group_p, 
                               fbe_packet_t *packet_p,
                               fbe_raid_position_bitmask_t failed_bitmap, 
                               fbe_bool_t clear_b)
{
    fbe_raid_iots_t *iots_p = NULL;
    fbe_payload_ex_t *sep_payload_p = fbe_transport_get_payload_ex(packet_p);

    /* Get the iots to save the bitmap.
     */
    fbe_payload_ex_get_iots(sep_payload_p, &iots_p);

    /* Trace some information.
     */
    fbe_base_object_trace((fbe_base_object_t *)raid_group_p, 
                          FBE_TRACE_LEVEL_INFO, 
                          FBE_TRACE_MESSAGE_ID_INFO, 
                          "%s: %s rl for failed_bitmap: 0x%x\n", 
                          __FUNCTION__, clear_b ? "clear" : "set", failed_bitmap);
     
    /* Save the information on the mark in the iots. 
     * This is needed since we still need to fetch the np dist lock, which is 
     * asynchronous. 
     */
    fbe_raid_iots_set_rebuild_logging_bitmask(iots_p, failed_bitmap);
    fbe_raid_iots_set_callback(iots_p, NULL, (fbe_raid_iots_completion_context_t)(fbe_ptrhld_t)clear_b);

    fbe_transport_set_completion_function(packet_p, fbe_raid_group_write_rl_get_dl_completion, raid_group_p);

    /* Callback is always called, no need to check status.
     */
    fbe_base_config_get_np_distributed_lock((fbe_base_config_t*)raid_group_p,
                                            packet_p);
    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_raid_group_write_rl_get_dl()
 **************************************/

/*!**************************************************************
 * fbe_raid_group_write_rl_get_dl_completion()
 ****************************************************************
 * @brief
 *  This is the completion function for fetching the np distributed lock.
 *  Now we will complete writing the non-paged by calling the
 *  write rl function.
 *
 * @param packet - packet ptr
 * @param context - raid group ptr.
 *
 * @return fbe_status_t 
 *
 * @author
 *  10/21/2011 - Created. Rob Foley
 *
 ****************************************************************/

static fbe_status_t 
fbe_raid_group_write_rl_get_dl_completion(fbe_packet_t * packet_p, fbe_packet_completion_context_t context)
{
    fbe_raid_group_t           *raid_group_p = (fbe_raid_group_t*)context;
    fbe_status_t                packet_status;
    fbe_raid_position_bitmask_t failed_bitmap = 0;
    fbe_bool_t                  clear_b = FBE_FALSE;
    fbe_raid_iots_t            *iots_p = NULL;
    fbe_payload_ex_t           *sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    fbe_raid_group_nonpaged_metadata_t   *raid_group_nonpaged_metadata_p = NULL;

    /* Get the iots and the information to determine the action to take.
     */
    fbe_payload_ex_get_iots(sep_payload_p, &iots_p);
    
    packet_status = fbe_transport_get_status_code(packet_p);
    if (packet_status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p, 
                              FBE_TRACE_LEVEL_WARNING, 
                              FBE_TRACE_MESSAGE_ID_INFO, 
                              "%s: error %d on get np dl call\n", 
                              __FUNCTION__, packet_status);
        return FBE_STATUS_OK;
    }
    /* Now we are done getting the lock, write the np.
     */
    fbe_raid_iots_get_rebuild_logging_bitmask(iots_p, &failed_bitmap);
    clear_b = (fbe_bool_t)iots_p->callback_context;

    fbe_base_object_trace((fbe_base_object_t *)raid_group_p, 
                          FBE_TRACE_LEVEL_INFO, 
                          FBE_TRACE_MESSAGE_ID_INFO, 
                          "%s: %s rl for failed_bitmap: 0x%x\n", 
                          __FUNCTION__, clear_b ? "clear" : "set", failed_bitmap);
    /* update drive tier if necessary
     */
    fbe_base_config_get_nonpaged_metadata_ptr((fbe_base_config_t*)raid_group_p, (void **)&raid_group_nonpaged_metadata_p);
    if (fbe_raid_group_is_drive_tier_query_needed(raid_group_p) && 
        raid_group_nonpaged_metadata_p->drive_tier == FBE_DRIVE_PERFORMANCE_TIER_TYPE_INVALID)
    {
        fbe_transport_set_completion_function(packet_p, fbe_raid_group_write_rl_update_nonpaged_drive_tier, raid_group_p);
    }

    fbe_transport_set_completion_function(packet_p, fbe_raid_group_write_rl_release_dl, raid_group_p);

    fbe_raid_group_write_rl(raid_group_p, packet_p, failed_bitmap, clear_b);
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/**************************************
 * end fbe_raid_group_write_rl_get_dl_completion()
 **************************************/

/*!**************************************************************
 * fbe_raid_group_write_rl_update_nonpaged_drive_tier()
 ****************************************************************
 * @brief
 *  update the drive tier in nonpaged metadata 
 *
 * @param packet - packet ptr
 * @param context - raid group ptr.
 *
 * @return fbe_status_t 
 *
 * @author
 *  10/14/2014 - Created. Geng Han
 *
 ****************************************************************/
static fbe_status_t 
fbe_raid_group_write_rl_update_nonpaged_drive_tier(fbe_packet_t * packet_p, fbe_packet_completion_context_t context)
{
    fbe_raid_group_t * raid_group_p = (fbe_raid_group_t*)context;
    fbe_raid_group_monitor_update_nonpaged_drive_tier(((fbe_base_object_t*) raid_group_p), packet_p);
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}

/*!**************************************************************
 * fbe_raid_group_write_rl_release_dl()
 ****************************************************************
 * @brief
 *  This is the completion function for writing the np metadata
 *  where we need to release the np dl.
 *
 * @param packet - packet ptr
 * @param context - raid group ptr.
 *
 * @return fbe_status_t 
 *
 * @author
 *  10/21/2011 - Created. Rob Foley
 *
 ****************************************************************/

static fbe_status_t 
fbe_raid_group_write_rl_release_dl(fbe_packet_t * packet_p, fbe_packet_completion_context_t context)
{
    fbe_raid_group_t * raid_group_p = (fbe_raid_group_t*)context;

    fbe_base_config_release_np_distributed_lock((fbe_base_config_t*)raid_group_p, packet_p);
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/**************************************
 * end fbe_raid_group_write_rl_release_dl()
 **************************************/

/*!****************************************************************************
 * fbe_raid_group_write_rl()
 ******************************************************************************
 * @brief
 *  This function writes the rebuild logging bitmask using metadata service
 *
 * @param raid_group_p - Raid group object
 * @param packet_p - packet pointer.
 * @param failed_bitmap - bitmap to mark rebuild logging
 * @param clear_b - used for clear rebuild logging
 * 
 * @return fbe_lifecycle_status_t
 *
 * @author
 *  6/28/2011 - Rewrote Ashwin Tamilarasan
 *
 ******************************************************************************/
fbe_status_t 
fbe_raid_group_write_rl(fbe_raid_group_t *raid_group_p, fbe_packet_t *packet_p,
                        fbe_raid_position_bitmask_t failed_bitmap, fbe_bool_t clear_b)
{

    fbe_status_t                            status;                         
    fbe_u64_t                               metadata_offset;                
    fbe_raid_group_rebuild_nonpaged_info_t  rebuild_nonpaged_data;
    fbe_class_id_t                          class_id;          
    fbe_payload_ex_t *sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    fbe_raid_iots_t *iots_p = NULL;

    class_id = fbe_raid_group_get_class_id(raid_group_p);

    //  Set attribute so upper layers know we are degraded.
    //  This applies to the RAID object, not VD.
    if (FBE_CLASS_ID_VIRTUAL_DRIVE != class_id) {
        /* Flag path degraded so that client make appropriate choice.
         */
        fbe_block_transport_server_set_path_attr_all_servers(&((fbe_base_config_t *)raid_group_p)->block_transport_server,
                                                             FBE_BLOCK_PATH_ATTR_FLAGS_DEGRADED);
    }

    //  Populate a non-paged rebuild data structure with the rebuild logging bitmask and rebuild checkpoint info 
    //  to write out, as needed
    status = fbe_raid_group_needs_rebuild_create_rebuild_nonpaged_data(raid_group_p, 
                                                                       failed_bitmap, 
                                                                       clear_b, 
                                                                       &rebuild_nonpaged_data);    
    if (status != FBE_STATUS_OK) {
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED; 
    }

    fbe_payload_ex_get_iots(sep_payload_p, &iots_p);
    fbe_raid_iots_set_rebuild_logging_bitmask(iots_p, failed_bitmap);
           
    metadata_offset = (fbe_u64_t) (&((fbe_raid_group_nonpaged_metadata_t*)0)->rebuild_info);    
    
    status = fbe_base_config_metadata_nonpaged_write_persist((fbe_base_config_t*) raid_group_p, packet_p, 
                                                 metadata_offset, (fbe_u8_t*) &rebuild_nonpaged_data, 
                                                 sizeof(rebuild_nonpaged_data));
    if (status != FBE_STATUS_OK) {
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED; 
    }

   return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/**************************************************
 * end fbe_raid_group_write_rl()
 **************************************************/


/*!****************************************************************************
 * fbe_raid_group_clear_glitching_drive_completion()
 ******************************************************************************
 * @brief
 *   Completion function called after it has cleared glitching drive bitmask.
 * 
 * @param packet_p   - pointer to a control packet from the scheduler
 * @param context    - completion context, which is a pointer to the raid
 *                        group object
 * 
 * @return  fbe_status_t  
 *
 * @author
 *   04/30/2012 - Created Prahlad Purohit
 *
 ******************************************************************************/

static fbe_status_t 
fbe_raid_group_clear_glitching_drive_completion(fbe_packet_t * packet_p, 
                                                fbe_packet_completion_context_t context)
{
    fbe_raid_group_t            *raid_group_p = (fbe_raid_group_t *) context;


    /* Once the rebuild logging bitmask been updated in the NP, send a continue to the IOs
     */
    fbe_raid_group_check_bitmap_and_continue(raid_group_p);  


    return FBE_STATUS_OK;
}


/*!****************************************************************************
 * fbe_raid_group_set_nr_or_iw_completion()
 ******************************************************************************
 * @brief
 *   Completion function called after it has marked MD for NR or IncompleteWrite
 *  verify.
 * 
 * @param packet_p   - pointer to a control packet from the scheduler
 * @param context    - completion context, which is a pointer to the raid
 *                        group object
 * 
 * @return  fbe_status_t  
 *
 * @author
 *   04/30/2012 - Created Prahlad Purohit
 *
 ******************************************************************************/

static fbe_status_t 
fbe_raid_group_set_nr_or_iw_completion(fbe_packet_t * packet_p, 
                                          fbe_packet_completion_context_t context)
{
    fbe_raid_group_t            *raid_group_p = (fbe_raid_group_t *) context;
    fbe_u64_t                   metadata_offset = 0;
    fbe_raid_position_bitmask_t glitching_drive_bitmask = 0xFFFF;

    fbe_base_object_trace((fbe_base_object_t*) raid_group_p, 
                          FBE_TRACE_LEVEL_INFO, 
                          FBE_TRACE_MESSAGE_ID_INFO, 
                          "%s: Clearing glitching drive bitmask.\n", 
                          __FUNCTION__);


    /* Set completion function to send continue to iots after it has cleared
     * glitched drive bitmask.
     */
    fbe_transport_set_completion_function(packet_p, 
                                          fbe_raid_group_clear_glitching_drive_completion, 
                                          raid_group_p);

   

    metadata_offset = (fbe_u64_t) (&((fbe_raid_group_nonpaged_metadata_t*)0)->glitching_disks_bitmask);

    fbe_base_config_metadata_nonpaged_clear_bits((fbe_base_config_t*) raid_group_p, 
                                               packet_p,
                                               metadata_offset,
                                               (fbe_u8_t*) &glitching_drive_bitmask,
                                               sizeof(glitching_drive_bitmask), 
                                               1);  

    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}


/*!****************************************************************************
 * fbe_raid_group_set_nr_or_iw()
 ******************************************************************************
 * @brief
 *   This function is used to mark metadata chunks for NR or Incomplete write 
 *  verify as the case may be.
 * 
 * @param packet_p   - pointer to a control packet from the scheduler
 * @param context    - completion context, which is a pointer to the raid
 *                        group object
 * 
 * @return  fbe_status_t  
 *
 * @author
 *   04/30/2012 - Created Prahlad Purohit
 *
 ******************************************************************************/
static fbe_status_t 
fbe_raid_group_set_nr_or_iw(fbe_packet_t * packet_p, 
                            fbe_packet_completion_context_t context)
{
    fbe_raid_group_t            *raid_group_p = (fbe_raid_group_t *) context;
    fbe_bool_t                  mark_for_nr = FALSE;
    fbe_raid_geometry_t         *raid_geometry_p = NULL;
    fbe_lba_t                   metadata_start_lba = FBE_LBA_INVALID;
    fbe_lba_t                   metadata_capacity = FBE_LBA_INVALID;
    fbe_u16_t                   num_data_disks;
    fbe_raid_position_bitmask_t rl_bitmask = 0;
    fbe_raid_position_bitmask_t glitched_drives_bitmap = 0;
    fbe_scheduler_hook_status_t hook_status = FBE_SCHED_STATUS_OK;
    fbe_chunk_index_t           chunk_index = FBE_CHUNK_INDEX_INVALID;
    fbe_chunk_count_t           chunk_count = 0;


    // Push a completion on stack to release NP lock.
    fbe_transport_set_completion_function(packet_p, fbe_raid_group_release_NP_lock, raid_group_p);    

    fbe_raid_group_get_rb_logging_bitmask(raid_group_p, &rl_bitmask);

    // Get the glitched drive bitmask and bitmask of drives that failed or are rebuilding.
    fbe_raid_group_get_glitched_drive_bitmask(raid_group_p, &glitched_drives_bitmap);    

    raid_geometry_p = fbe_raid_group_get_raid_geometry(raid_group_p);
    fbe_raid_geometry_get_metadata_start_lba(raid_geometry_p, &metadata_start_lba);
    fbe_raid_geometry_get_metadata_capacity(raid_geometry_p, &metadata_capacity);
    fbe_raid_geometry_get_data_disks(raid_geometry_p, &num_data_disks);

    

    /* Set completion function to clear glitched drives bitmap and send 
     * continue to iots.
     */
    fbe_transport_set_completion_function(packet_p, 
                                          fbe_raid_group_set_nr_or_iw_completion, 
                                          raid_group_p);

    fbe_raid_group_bitmap_get_chunk_index_and_chunk_count(raid_group_p, 
                                                          metadata_start_lba,
                                                          metadata_capacity,
                                                          &chunk_index,
                                                          &chunk_count);

    /* If we are already degraded and see glitch on drive thats marked for RL
     * then marked paged metadata for NR.
     */
    mark_for_nr = fbe_raid_group_is_degraded(raid_group_p) &&
                  (0 == ((glitched_drives_bitmap ^ rl_bitmask) & glitched_drives_bitmap));

    if(mark_for_nr)
    {
        fbe_raid_group_paged_metadata_t chunk_data;

        fbe_check_rg_monitor_hooks(raid_group_p,
                                   SCHEDULER_MONITOR_STATE_RAID_GROUP_EVAL_RL,
                                   FBE_RAID_GROUP_SUBSTATE_MARK_METADATA_NEEDS_REBUILD,
                                   &hook_status);

        //  Set up the bits of the metadata that need to be written, which are the NR bits
        fbe_set_memory(&chunk_data, 0, sizeof(fbe_raid_group_paged_metadata_t));
        chunk_data.needs_rebuild_bits = glitched_drives_bitmap;
        fbe_base_object_trace((fbe_base_object_t*) raid_group_p, 
                              FBE_TRACE_LEVEL_INFO, 
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s: Marking MDD NR\n", 
                              __FUNCTION__);
        fbe_raid_group_bitmap_write_chunk_info_using_nonpaged(raid_group_p, 
                                                              packet_p, 
                                                              chunk_index,
                                                              chunk_count, 
                                                              &chunk_data, 
                                                              FALSE);
        fbe_base_object_trace((fbe_base_object_t*) raid_group_p, 
                              FBE_TRACE_LEVEL_INFO, 
                              FBE_TRACE_MESSAGE_ID_INFO, 
                              "%s: Marked MDD NR\n", 
                              __FUNCTION__);
    }
    else
    {
        fbe_base_object_trace((fbe_base_object_t*) raid_group_p, 
                              FBE_TRACE_LEVEL_ERROR, 
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                              "%s: mark nr not set.\n", 
                              __FUNCTION__);

#if 0 /* We should not be here */
        fbe_u64_t         metadata_offset = 0;
        
        fbe_check_rg_monitor_hooks(raid_group_p,
                                   SCHEDULER_MONITOR_STATE_RAID_GROUP_EVAL_RL,
                                   FBE_RAID_GROUP_SUBSTATE_MARK_METADATA_INCOMPLETE_WRITE,
                                   &hook_status);     

        metadata_offset = (fbe_u64_t)(&((fbe_raid_group_nonpaged_metadata_t*)0)->incomplete_write_verify_checkpoint);      

        // Convert the metadata start lba and block count into per disk chunk index and chunk count
        fbe_raid_group_bitmap_get_chunk_index_and_chunk_count(raid_group_p, 
                                                              metadata_start_lba,
                                                              metadata_capacity,
                                                              &chunk_index, 
                                                              &chunk_count);

        // Update the checkpoint in non paged metdata.
        fbe_raid_group_bitmap_set_non_paged_metadata_for_verify(packet_p,raid_group_p,
                                                                chunk_index, chunk_count,
                                                                fbe_raid_group_mark_mdd_for_incomplete_write_verify,
                                                                metadata_offset,
                                                                FBE_RAID_BITMAP_VERIFY_INCOMPLETE_WRITE);
#endif
    }

    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}

/*!****************************************************************************
 * fbe_raid_group_verify_set_iw()
 ******************************************************************************
 * @brief
 *   This is the completion function for obtaining NP lock for incomplete write verify.
 * 
 * @param packet_p   - pointer to the packet
 * @param context    - completion context, which is a pointer to the raid
 *                        group object
 * 
 * @return  fbe_status_t  
 *
 * @author
 *  11/20/2012 - Created. Lili Chen
 *
 ******************************************************************************/
static fbe_status_t 
fbe_raid_group_verify_set_iw(fbe_packet_t * packet_p, fbe_packet_completion_context_t context)
{
    fbe_raid_group_t    *raid_group_p = (fbe_raid_group_t *)context;
    fbe_status_t        status;

    /* If the status is not ok, that means we didn't get the lock.
     */
    status = fbe_transport_get_status_code (packet_p);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, 
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_INFO,
                              "%s can't get the NP lock, status: 0x%X\n", __FUNCTION__, status); 
        return FBE_STATUS_OK;
    }

    fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s: Fetched NP lock.\n",
                          __FUNCTION__);

    /* Push a completion on stack to release NP lock. */
    fbe_transport_set_completion_function(packet_p, fbe_raid_group_release_NP_lock, raid_group_p);        

    fbe_transport_set_completion_function(packet_p, 
                                          fbe_raid_group_set_nr_or_iw_completion, 
                                          raid_group_p);

    return FBE_STATUS_CONTINUE;
}
/**************************************************
 * end fbe_raid_group_verify_set_iw()
 **************************************************/

/*!****************************************************************************
 * fbe_raid_group_verify_set_iw_completion()
 ******************************************************************************
 * @brief
 *   This is the completion function for marking incomplete write verify. It 
 *   will get NP lock first.
 * 
 * @param packet_p   - pointer to the packet
 * @param context    - completion context, which is a pointer to the raid
 *                        group object
 * 
 * @return  fbe_status_t  
 *
 * @author
 *  11/20/2012 - Created. Lili Chen
 *
 ******************************************************************************/
static fbe_status_t 
fbe_raid_group_verify_set_iw_completion(fbe_packet_t * packet_p, 
                                        fbe_packet_completion_context_t context)
{
    fbe_raid_group_t            *raid_group_p = (fbe_raid_group_t *) context;

    fbe_raid_group_get_NP_lock(raid_group_p, packet_p, fbe_raid_group_verify_set_iw);

    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/**************************************************
 * end fbe_raid_group_verify_set_iw_completion()
 **************************************************/

/*!****************************************************************************
 * fbe_raid_group_handle_glitching_drive()
 ******************************************************************************
 * @brief
 *   check glitching drive bitmask 
 * 
 * @param raid_group_p    - completion context, which is a pointer to the raid group object
 * @param packet_p   - pointer to a control packet from the scheduler
 * 
 * @return  fbe_status_t  
 *
 * @author
 *   08/06/2013 - Created Jibing
 *
 ******************************************************************************/
static fbe_status_t 
fbe_raid_group_handle_glitching_drive(fbe_raid_group_t* raid_group_p, fbe_packet_t * packet_p)
{
    fbe_raid_position_bitmask_t rl_bitmask;
    fbe_raid_position_bitmask_t glitched_drives_bitmap = 0;

    fbe_raid_group_get_rb_logging_bitmask(raid_group_p, &rl_bitmask);

    // Get the glitched drive bitmask and bitmask of drives that failed or are rebuilding.
    fbe_raid_group_get_glitched_drive_bitmask(raid_group_p, &glitched_drives_bitmap);

    

    /* Check if glitched drives bitmap is set. If it is then mark metadata
     * region for NR or Incomplete write verify as the case may be.
     */
    if (glitched_drives_bitmap)
    {
        fbe_bool_t                  mark_for_nr = FALSE;

        fbe_base_object_trace((fbe_base_object_t*) raid_group_p, FBE_TRACE_LEVEL_INFO, 
                              FBE_TRACE_MESSAGE_ID_INFO, "%s: ====glitched_drives_bitmap = 0x%x\n", __FUNCTION__, 
                              glitched_drives_bitmap);

        mark_for_nr = fbe_raid_group_is_degraded(raid_group_p) &&
            (0 == ((glitched_drives_bitmap ^ rl_bitmask) & glitched_drives_bitmap));
        //CSX_BREAK();
        if(!mark_for_nr)
        {
            fbe_transport_set_completion_function(packet_p, 
                                                  fbe_raid_group_verify_set_iw_completion, 
                                                  raid_group_p);
            fbe_raid_group_verify_mark_iw_verify(packet_p, raid_group_p);
        }
        else
        {
            fbe_raid_group_get_NP_lock(raid_group_p, 
                                       packet_p, 
                                       fbe_raid_group_set_nr_or_iw);
        }

        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }

    return FBE_STATUS_OK; 
}
/**************************************************
 * end fbe_raid_group_handle_glitching_drive()
 **************************************************/

/*!****************************************************************************
 * fbe_raid_group_write_rl_completion()
 ******************************************************************************
 * @brief
 *   This function is the completion function for writing rebuild logging bitmask
 *   where it will send a continue for the IOs
 * 
 * @param packet_p   - pointer to a control packet from the scheduler
 * @param context    - completion context, which is a pointer to the raid
 *                        group object
 * 
 * @return  fbe_status_t  
 *
 * @author
 *   06/28/2011 - Rewrote Ashwin Tamilarasan
 *   04/30/2011 - Modified Prahlad Purohit
 *
 ******************************************************************************/
fbe_status_t fbe_raid_group_write_rl_completion(fbe_packet_t *packet_p,
                                            fbe_packet_completion_context_t context_p)

{
    fbe_raid_group_t*                       raid_group_p = (fbe_raid_group_t*) context_p;                   
    fbe_status_t                            status;                      
    fbe_payload_ex_t *sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    fbe_raid_iots_t *iots_p = NULL;
    fbe_raid_position_t position;
    fbe_u32_t width;
    fbe_raid_position_bitmask_t rl_bitmask;

    status = fbe_transport_get_status_code(packet_p);
    if (status != FBE_STATUS_OK) {
        fbe_base_object_trace((fbe_base_object_t*) raid_group_p, FBE_TRACE_LEVEL_WARNING, 
                              FBE_TRACE_MESSAGE_ID_INFO, "%s: error %d on nonpaged write call\n", __FUNCTION__, 
                              status);
        return FBE_STATUS_OK;
    }

    /* We are done with the writing, the bits are set, so we can log now.
     */
    fbe_payload_ex_get_iots(sep_payload_p, &iots_p);
    fbe_raid_iots_get_rebuild_logging_bitmask(iots_p, &rl_bitmask);

    position = 0;
    fbe_raid_group_get_width(raid_group_p, &width);
    for ( position = 0; position < width; position++) {
        if (rl_bitmask & (1 << position)) {
            fbe_raid_group_logging_log_rl_started(raid_group_p, packet_p, position);
        }
    }

    /* If we have set rl, send usurper to disable media error thresholds.  
     * This is only done on the active side.  The condition will
     * coordinate with the peer so that the EMEH condition is also set on the
     * passive side.
     */
    if ( fbe_raid_group_is_extended_media_error_handling_capable(raid_group_p) &&
         (rl_bitmask != 0)                                                        ) {
        fbe_raid_emeh_command_t emeh_request;

        /* If enabled send the condition to send the EMEH (Extended Media Error 
         * Handling) usurper to remaining PDOs.
         */
        fbe_raid_group_lock(raid_group_p);
        fbe_raid_group_get_emeh_request(raid_group_p, &emeh_request);
        if (emeh_request != FBE_RAID_EMEH_COMMAND_INVALID)
        {
            /* Simply trace and don't set the condition.  This is warning since
             * RAID-6 etc can mark for different positions before the EMEH condition
             * gets a chance to run.
             */
            fbe_raid_group_unlock(raid_group_p);
            fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                                  FBE_TRACE_LEVEL_WARNING,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "emeh write rl completion: %d already in progress \n",
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
                                  "emeh write rl completion: set: %d emeh request\n",
                                  FBE_RAID_EMEH_COMMAND_RAID_GROUP_DEGRADED);
            fbe_lifecycle_set_cond(&fbe_raid_group_lifecycle_const, (fbe_base_object_t *)raid_group_p,
                                   FBE_RAID_GROUP_LIFECYCLE_COND_EMEH_REQUEST);
        }
    }

    /* Now handling glitching drives */
    status = fbe_raid_group_handle_glitching_drive(raid_group_p, packet_p);
    if (status == FBE_STATUS_OK) {
        /* Once the rebuild logging bitmask been updated in the NP, send a continue to the IOs
        */
        fbe_raid_group_check_bitmap_and_continue(raid_group_p);    
    }

    return status;
} 

/**************************************************
 * end fbe_raid_group_write_rl_completion()
 **************************************************/

/*!****************************************************************************
 * fbe_raid_group_check_bitmap_and_continue()
 ******************************************************************************
 * @brief
 *   Utility function to send a continue
 * 
 * @param raid_group_p   
 * 
 * @return  fbe_status_t  
 *
 * @author
 *   06/28/2011 - created Ashwin Tamilarasan
 *
 ******************************************************************************/
static fbe_status_t fbe_raid_group_check_bitmap_and_continue(fbe_raid_group_t  *raid_group_p)
                                            
{
    fbe_raid_position_bitmask_t             continue_bitmask;               
    fbe_raid_position_bitmask_t             rl_bitmask;   
    fbe_raid_position_bitmask_t             failed_drives_bitmap_local = 0;
    fbe_raid_position_bitmask_t             failed_ios_pos_bitmap_local = 0;
    fbe_raid_position_bitmask_t             failed_drives_bitmap_peer = 0;
    fbe_raid_position_bitmask_t             failed_ios_pos_bitmap_peer = 0;

    /* Combine the local and peer failed pos bitmap
     */
    fbe_raid_group_lock(raid_group_p);    
    fbe_raid_group_get_local_bitmap(raid_group_p, &failed_drives_bitmap_local, &failed_ios_pos_bitmap_local);
    fbe_raid_group_get_peer_bitmap(raid_group_p, FBE_LIFECYCLE_STATE_READY,
                                   &failed_drives_bitmap_peer, &failed_ios_pos_bitmap_peer);
    fbe_raid_group_unlock(raid_group_p);

    continue_bitmask = failed_drives_bitmap_local | failed_ios_pos_bitmap_local |failed_drives_bitmap_peer| failed_ios_pos_bitmap_peer;

    fbe_raid_group_get_rb_logging_bitmask(raid_group_p, &rl_bitmask);

    /* If the continue bitmask is not a subset of what is there on NP    
     * then there is a problem.
     */
    if(!((rl_bitmask | continue_bitmask) == rl_bitmask) )       
    {
        fbe_base_object_trace((fbe_base_object_t*) raid_group_p, FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO, 
                           "rl_bitmask is 0x%x. Combined failed bitmap of local and peer 0x%x\n", rl_bitmask,
                            continue_bitmask);    
    }

    fbe_base_object_trace((fbe_base_object_t*) raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO, 
                           "rl_bitmask is 0x%x. Calling continue with mask 0x%x\n", rl_bitmask,
                            continue_bitmask);    

    /* Before we issue the continue, it is necessary to clear hold.
     */
    fbe_metadata_element_clear_abort(&raid_group_p->base_config.metadata_element);
    fbe_raid_group_handle_continue(raid_group_p);
    return FBE_STATUS_OK;
}
/**************************************************
 * end fbe_raid_group_check_bitmap_and_continue()
 **************************************************/

/*!****************************************************************************
 * fbe_raid_group_eval_rb_logging_request()
 ******************************************************************************
 * @brief
 *  This function initiates the eval of rebuild logging. Which ever SP sees the drive go 
 * away first will set the cond and sets its state to request. It will
 * wait for both SPs to acknowledge that it is ready to eval.
 * Once both SPs are ready the SPs will move to the started state.
 *
 * @param raid_group_p - Raid group object
 * @param packet_p - packet pointer.
 * 
 * @return fbe_lifecycle_status_t
 *
 * @author
 *  6/27/2011 - Created Ashwin Tamilarasan
 *
 ******************************************************************************/
fbe_lifecycle_status_t 
fbe_raid_group_eval_rb_logging_request(fbe_raid_group_t *  raid_group_p, fbe_packet_t * packet_p)
{
    fbe_bool_t           b_is_active;
    fbe_bool_t           b_peer_flag_set = FBE_FALSE;
    fbe_scheduler_hook_status_t        hook_status = FBE_SCHED_STATUS_OK;
    
    fbe_raid_group_monitor_check_active_state(raid_group_p, &b_is_active);

    fbe_raid_group_lock(raid_group_p);

    if (!fbe_raid_group_is_local_state_set(raid_group_p, FBE_RAID_GROUP_LOCAL_STATE_EVAL_RL_REQUEST))
    {
        fbe_raid_group_set_local_state(raid_group_p, FBE_RAID_GROUP_LOCAL_STATE_EVAL_RL_REQUEST);
    }
    /* Either the active or passive side can initiate this. 
     * We start the process by setting eval signature. 
     * This gets the peer running the same condition. 
     */
    if (!fbe_raid_group_is_clustered_flag_set(raid_group_p, FBE_RAID_GROUP_CLUSTERED_FLAG_EVAL_RL_REQ))
    {
        fbe_raid_group_set_clustered_flag(raid_group_p, FBE_RAID_GROUP_CLUSTERED_FLAG_EVAL_RL_REQ);

        fbe_raid_group_unlock(raid_group_p);
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s marking EVAL_RL_REQ state: 0x%llx\n", 
                              b_is_active ? "Active" : "Passive",
                              (unsigned long long)raid_group_p->local_state);
        return FBE_LIFECYCLE_STATUS_RESCHEDULE;
    }

    fbe_check_rg_monitor_hooks(raid_group_p,
                               SCHEDULER_MONITOR_STATE_RAID_GROUP_EVAL_RL,
                               FBE_RAID_GROUP_SUBSTATE_EVAL_RL_REQUEST, 
                               &hook_status);
    if (hook_status == FBE_SCHED_STATUS_DONE) {
        fbe_raid_group_unlock(raid_group_p);
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /* Now make sure the peer also has this flag set.
     */
    fbe_raid_group_is_peer_flag_set(raid_group_p, &b_peer_flag_set, 
                                    FBE_RAID_GROUP_CLUSTERED_FLAG_EVAL_RL_REQ);

    if (!b_peer_flag_set)
    {
        fbe_raid_group_unlock(raid_group_p);
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "eval rl %s, wait peer request local: 0x%llx peer: 0x%llx state: 0x%llx\n", 
                              b_is_active ? "Active" : "Passive",
                              (unsigned long long)raid_group_p->raid_group_metadata_memory.flags,
                              (unsigned long long)raid_group_p->raid_group_metadata_memory_peer.flags,
                              (unsigned long long)raid_group_p->local_state);
        return FBE_LIFECYCLE_STATUS_DONE;
    }
    else
    {
        /* The peer has set the flag, we are ready to do the next set of checks.
         */
        fbe_raid_group_set_local_state(raid_group_p, FBE_RAID_GROUP_LOCAL_STATE_EVAL_RL_STARTED);
        fbe_raid_group_clear_local_state(raid_group_p, FBE_RAID_GROUP_LOCAL_STATE_EVAL_RL_REQUEST);
        fbe_raid_group_unlock(raid_group_p);
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "eval rl %s peer req set local: 0x%llx peer: 0x%llx state: 0x%llx 0x%x\n", 
                              b_is_active ? "Active" : "Passive",
                              (unsigned long long)raid_group_p->raid_group_metadata_memory.flags,
                              (unsigned long long)raid_group_p->raid_group_metadata_memory_peer.flags,
                              (unsigned long long)raid_group_p->local_state,
                              raid_group_p->raid_group_metadata_memory_peer.base_config_metadata_memory.lifecycle_state);
        return FBE_LIFECYCLE_STATUS_RESCHEDULE;
    }            
}
/**************************************************
 * end fbe_raid_group_eval_rb_logging_request()
 **************************************************/

/*!****************************************************************************
 * fbe_raid_group_set_rb_logging_passive()
 ******************************************************************************
 * @brief
 *  Passive side does not do much. It will just sets its flag and bitmap
 *  and wait for the active side to do the Eval. Once rebuild logging
 *  ia marked, it will send a continue for the IOs.
 *
 * @param raid_group_p - Raid group object
 * @param packet_p - packet pointer.
 * 
 * @return fbe_lifecycle_status_t
 *
 * @author
 *  6/27/2011 - Created Ashwin Tamilarasan
 *
 ******************************************************************************/
fbe_lifecycle_status_t 
fbe_raid_group_set_rb_logging_passive(fbe_raid_group_t *  raid_group_p, fbe_packet_t * packet_p)
{
    fbe_bool_t       b_peer_flag_set = FBE_FALSE;
    fbe_bool_t       b_local_flag_set = FBE_FALSE;
    fbe_raid_position_bitmask_t        failed_pos_bitmap_local = 0;
    fbe_raid_position_bitmask_t        failed_ios_bitmap       = 0;
    fbe_bool_t           b_is_active;
    fbe_scheduler_hook_status_t        hook_status = FBE_SCHED_STATUS_OK;
    fbe_bool_t       b_peer_broken_flag_set = FBE_FALSE;
    fbe_raid_position_bitmask_t local_disabled_bitmask;

    fbe_raid_group_lock(raid_group_p);

    fbe_raid_group_is_peer_flag_set(raid_group_p, &b_peer_flag_set, 
                                    FBE_RAID_GROUP_CLUSTERED_FLAG_EVAL_RL_STARTED);
    fbe_raid_group_is_peer_flag_set(raid_group_p, &b_peer_broken_flag_set, 
                                    FBE_RAID_GROUP_CLUSTERED_FLAG_EVAL_RL_DONE_BROKEN);

    if (fbe_raid_group_is_clustered_flag_set(raid_group_p, FBE_RAID_GROUP_CLUSTERED_FLAG_EVAL_RL_STARTED))
    {
        b_local_flag_set = FBE_TRUE;
    }    

    /* We first wait for the peer to start the eval. 
     * This is needed since either active or passive can initiate the eval. 
     * Once we see this set, we will set our flag, which the peer waits for before it completes the eval.  
     */
    if (!b_peer_flag_set && !b_local_flag_set)
    {
        fbe_raid_group_unlock(raid_group_p);
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "Passive waiting peer EVAL_RL_STARTED local: 0x%llx peer: 0x%llx state: 0x%llx\n",
                              (unsigned long long)raid_group_p->raid_group_metadata_memory.flags,
                              (unsigned long long)raid_group_p->raid_group_metadata_memory_peer.flags,
                              (unsigned long long)raid_group_p->local_state);
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    fbe_check_rg_monitor_hooks(raid_group_p,
                               SCHEDULER_MONITOR_STATE_RAID_GROUP_EVAL_RL,
                               FBE_RAID_GROUP_SUBSTATE_EVAL_RL_STARTED2, 
                               &hook_status);
    if (hook_status == FBE_SCHED_STATUS_DONE) {
        fbe_raid_group_unlock(raid_group_p);
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /* At this point the peer started flag is set.
     * If we have not set this yet locally, set it and wait for the condition to run to send it to the peer.
     */
    if (!b_local_flag_set)
    {
        fbe_raid_group_unlock(raid_group_p);        
        fbe_raid_group_get_failed_pos_bitmask(raid_group_p, &failed_pos_bitmap_local);
        fbe_raid_group_find_failed_io_positions(raid_group_p, &failed_ios_bitmap);
        fbe_raid_group_get_disabled_pos_bitmask(raid_group_p, &local_disabled_bitmask);        
        fbe_raid_group_lock(raid_group_p); 
        fbe_raid_group_set_clustered_flag_and_bitmap(raid_group_p, FBE_RAID_GROUP_CLUSTERED_FLAG_EVAL_RL_STARTED,
                                                     failed_pos_bitmap_local, failed_ios_bitmap, local_disabled_bitmask);

        fbe_raid_group_unlock(raid_group_p);
        /*Split trace to two lines*/
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "Passive marking EVAL_RL_STARTED loc fl: 0x%llx peer fl: 0x%x>\n",
                              (unsigned long long)raid_group_p->raid_group_metadata_memory.flags,
                              (unsigned int)raid_group_p->raid_group_metadata_memory_peer.flags);
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "loc map: 0x%x peer map: 0x%x state: 0x%llx<\n",
                              raid_group_p->raid_group_metadata_memory.failed_drives_bitmap,
                              raid_group_p->raid_group_metadata_memory_peer.failed_drives_bitmap,
                              (unsigned long long)(unsigned long long)raid_group_p->local_state);


        return FBE_LIFECYCLE_STATUS_DONE;
    }
    else
    {
        fbe_raid_position_bitmask_t clustered_failed_pos_bitmap = 0;
        fbe_raid_position_bitmask_t clustered_failed_ios_bitmap = 0;
        fbe_raid_position_bitmask_t clustered_disabled_bitmap = 0;

        fbe_raid_group_unlock(raid_group_p);
        fbe_raid_group_get_failed_pos_bitmask(raid_group_p, &failed_pos_bitmap_local);
        fbe_raid_group_get_disabled_pos_bitmask(raid_group_p, &local_disabled_bitmask);
               
        fbe_raid_group_get_local_bitmap(raid_group_p, &clustered_failed_pos_bitmap, &clustered_failed_ios_bitmap);
        fbe_raid_group_get_disabled_local_bitmap(raid_group_p, &clustered_disabled_bitmap);

        /* If either of the bitmaps that can change do change, then trace and update them in clustered.
         */
        if ((failed_pos_bitmap_local != clustered_failed_pos_bitmap) ||
            (local_disabled_bitmask != clustered_disabled_bitmap))
        {
            fbe_base_object_trace((fbe_base_object_t *)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                  "Passive RL bm changed: failed bm old/new: %x/%x disabled bm old/new %x/%x\n",
                                  clustered_failed_pos_bitmap, failed_pos_bitmap_local, 
                                  clustered_disabled_bitmap, local_disabled_bitmask);
            fbe_raid_group_set_clustered_bitmap(raid_group_p, failed_pos_bitmap_local);
            fbe_raid_group_set_clustered_disabled_bitmap(raid_group_p, local_disabled_bitmask);
            return FBE_LIFECYCLE_STATUS_DONE;
        }
    }

    /* If we become active, just return out.
     * We need to handle the active's work ourself. 
     * When we come back into the condition we'll automatically execute the active code.
     */
    fbe_raid_group_monitor_check_active_state(raid_group_p, &b_is_active);
    if (b_is_active)
    {
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p, FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                              "Passive eval rl became active local: 0x%llx peer: 0x%llx lstate: 0x%llx\n",
                              (unsigned long long)raid_group_p->raid_group_metadata_memory.base_config_metadata_memory.flags,
                              (unsigned long long)raid_group_p->raid_group_metadata_memory_peer.base_config_metadata_memory.flags,
                              (unsigned long long)raid_group_p->local_state);
        return FBE_LIFECYCLE_STATUS_RESCHEDULE;
    }

    fbe_check_rg_monitor_hooks(raid_group_p,
                               SCHEDULER_MONITOR_STATE_RAID_GROUP_EVAL_RL,
                               FBE_RAID_GROUP_SUBSTATE_EVAL_RL_STARTED, 
                               &hook_status);
    if (hook_status == FBE_SCHED_STATUS_DONE) {
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /* Once we have set the started flag, we simply wait for the active side to complete. 
     * We know Active is done when it clears the flags. 
     */
    if (!b_peer_flag_set)
    {
    
        fbe_class_id_t  class_id = fbe_raid_group_get_class_id(raid_group_p);

        if (fbe_raid_group_is_degraded(raid_group_p) && FBE_CLASS_ID_VIRTUAL_DRIVE != class_id)
        {
            fbe_block_transport_server_set_path_attr_all_servers(&((fbe_base_config_t *)raid_group_p)->block_transport_server,
                                                                 FBE_BLOCK_PATH_ATTR_FLAGS_DEGRADED);
        }
    

        if (!b_peer_broken_flag_set)
        {
            fbe_raid_group_check_bitmap_and_continue(raid_group_p);
        }
        fbe_raid_group_lock(raid_group_p);
        fbe_raid_group_set_local_state(raid_group_p, FBE_RAID_GROUP_LOCAL_STATE_EVAL_RL_DONE);
        fbe_raid_group_clear_local_state(raid_group_p, FBE_RAID_GROUP_LOCAL_STATE_EVAL_RL_STARTED);
        fbe_raid_group_unlock(raid_group_p);
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "eval rl Passive start cleanup local: 0x%llx peer: 0x%llx\n",
                              (unsigned long long)raid_group_p->raid_group_metadata_memory.flags,
                              (unsigned long long)raid_group_p->raid_group_metadata_memory_peer.flags);
        /* Set condition to clear out error attributes marked on downstream edges, which are holding our view of the drive state 
         * as bad. 
         * We only want to do this once, so we do this just before we exit this state. 
         */
        fbe_lifecycle_set_cond(&fbe_raid_group_lifecycle_const, (fbe_base_object_t*) raid_group_p,
                               FBE_RAID_GROUP_LIFECYCLE_COND_CLEAR_ERRORS);
        return FBE_LIFECYCLE_STATUS_RESCHEDULE;
    }
    else
    {
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "eval rl Passive wait peer complete loc: 0x%llx peer: 0x%llx lstate: 0x%llx bcf: 0x%llx pp: %d\n",
                              (unsigned long long)raid_group_p->raid_group_metadata_memory.flags,
                              (unsigned long long)raid_group_p->raid_group_metadata_memory_peer.flags,
                              (unsigned long long)raid_group_p->local_state,
                              (unsigned long long)raid_group_p->raid_group_metadata_memory_peer.base_config_metadata_memory.flags,
                              (raid_group_p->base_config.metadata_element.peer_metadata_element_ptr != 0));
    }
    return FBE_LIFECYCLE_STATUS_DONE;    

}
/**************************************************
 * end fbe_raid_group_set_rb_logging_passive()
 **************************************************/

/*!****************************************************************************
 * fbe_raid_group_set_rb_logging_active()
 ******************************************************************************
 * @brief
 *  This function is the active SP function which writes out the
 *  rb logging bitmask. failed IO bitmap and failed disk bitmap are exchanged between SPs
 *  Once both sides exchanges the bitmap, it will check whether we need to mark rb logging.
 *  If there is, it will set rebuild logging.  Otherwise it sends a continue.
 *  We will need to send a continue for the use case where a drive blips(edge bounces)
 *  
 *
 * @param raid_group_p - Raid group object
 * @param packet_p - packet pointer.
 * 
 * @return fbe_lifecycle_status_t
 *
 * @author
 *  6/27/2011 - Created Ashwin Tamilarasan
 *
 ******************************************************************************/
fbe_lifecycle_status_t 
fbe_raid_group_set_rb_logging_active(fbe_raid_group_t *raid_group_p, fbe_packet_t * packet_p)
{    
    fbe_bool_t                      b_peer_flag_set = FBE_FALSE;
    fbe_class_id_t                  class_id;
    fbe_raid_position_bitmask_t     failed_pos_bitmap_local = 0;
    fbe_raid_position_bitmask_t     drives_marked_nr = 0;
    fbe_raid_position_bitmask_t     failed_ios_bitmap = 0;
    fbe_raid_position_bitmask_t     failed_drives_bitmap_peer = 0;
    fbe_raid_position_bitmask_t     failed_ios_bitmap_peer = 0;
    fbe_raid_position_bitmask_t     combined_failed_bitmap = 0;
    fbe_raid_position_bitmask_t     mark_rl_bitmap = 0;
    fbe_raid_position_bitmask_t     local_disabled_bitmask;
    fbe_raid_position_bitmask_t     peer_disabled_bitmask;
    fbe_u16_t                       num_parity_drives = 0;
    fbe_u16_t                       failed_drives = 0;
    fbe_raid_position_bitmask_t     rl_bitmask;
    fbe_status_t                    status;
    fbe_raid_geometry_t            *raid_geometry_p = NULL;            
    fbe_scheduler_hook_status_t     hook_status = FBE_SCHED_STATUS_OK;
    fbe_time_t                      elapsed_time;
    fbe_bool_t                      b_is_timeout_expired;
    fbe_u32_t                       rebuild_index;
    
    raid_geometry_p = fbe_raid_group_get_raid_geometry(raid_group_p);
    class_id = fbe_raid_group_get_class_id(raid_group_p); 

    /* Quiesce the IO before proceeding */
    status = fbe_raid_group_needs_rebuild_quiesce_io_if_needed(raid_group_p);
    if (status == FBE_STATUS_PENDING)
    {
        return FBE_LIFECYCLE_STATUS_RESCHEDULE; 
    }

    /* Get the current and new rebuild logging bitmasks.
     */
    fbe_raid_group_get_rb_logging_bitmask(raid_group_p, &rl_bitmask);  
    fbe_raid_group_get_failed_pos_bitmask(raid_group_p, &failed_pos_bitmap_local);
    fbe_raid_group_find_failed_io_positions(raid_group_p, &failed_ios_bitmap);
    fbe_raid_group_get_disabled_pos_bitmask(raid_group_p, &local_disabled_bitmask);  

    fbe_base_object_trace((fbe_base_object_t*) raid_group_p, 
                          FBE_TRACE_LEVEL_INFO,           
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,  
                          "eval rl active quiesced rl: 0x%x failed: 0x%x failed io: 0x%x local state: 0x%llx \n", 
                          rl_bitmask, failed_pos_bitmap_local, failed_ios_bitmap,
                          (unsigned long long)raid_group_p->local_state);

    fbe_check_rg_monitor_hooks(raid_group_p,
                               SCHEDULER_MONITOR_STATE_RAID_GROUP_EVAL_RL,
                               FBE_RAID_GROUP_SUBSTATE_EVAL_RL_STARTED2, 
                               &hook_status);
    if (hook_status == FBE_SCHED_STATUS_DONE) {
        return FBE_LIFECYCLE_STATUS_DONE;
    }


    fbe_raid_group_lock(raid_group_p);
    /* Now we set the bit that says we are starting. 
     * This bit always gets set first by the Active side. 
     * When passive sees the started peer bit, it will also set the started bit. 
     * The passive side will thereafter know we are done when this bit is cleared. 
     */
    if (!fbe_raid_group_is_clustered_flag_set(raid_group_p, FBE_RAID_GROUP_CLUSTERED_FLAG_EVAL_RL_STARTED))
    {                       
        fbe_raid_group_set_clustered_flag_and_bitmap(raid_group_p, FBE_RAID_GROUP_CLUSTERED_FLAG_EVAL_RL_STARTED,
                                                     failed_pos_bitmap_local, failed_ios_bitmap, local_disabled_bitmask);

        /*Split trace to two lines*/
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "Active marking EVAL_RL_STARTED loc fl: 0x%llx peer fl: 0x%x>\n",
                              (unsigned long long)raid_group_p->raid_group_metadata_memory.flags,
                              (unsigned int)raid_group_p->raid_group_metadata_memory_peer.flags);
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "loc map: 0x%x peer map: 0x%x state: 0x%llx<\n",
                              raid_group_p->raid_group_metadata_memory.failed_drives_bitmap,
                              raid_group_p->raid_group_metadata_memory_peer.failed_drives_bitmap,
                              (unsigned long long)(unsigned long long)raid_group_p->local_state);

        /* Since we are active side, we don't have to wait, we can just go ahead */
    }
    else
    {
        fbe_raid_position_bitmask_t clustered_failed_pos_bitmap = 0;
        fbe_raid_position_bitmask_t clustered_failed_ios_bitmap = 0;
        fbe_raid_position_bitmask_t clustered_disabled_bitmap = 0;
        fbe_raid_group_get_local_bitmap(raid_group_p, &clustered_failed_pos_bitmap, &clustered_failed_ios_bitmap);
        fbe_raid_group_get_disabled_local_bitmap(raid_group_p, &clustered_disabled_bitmap);

        /* If either of the bitmaps that can change do change, then trace and update them in clustered.
         */
        if ((failed_pos_bitmap_local != clustered_failed_pos_bitmap) ||
            (local_disabled_bitmask != clustered_disabled_bitmap))
        {
            fbe_base_object_trace((fbe_base_object_t *)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                  "Active RL bm changed: failed old/new: %x/%x disabled old/new %x/%x\n",
                                  clustered_failed_pos_bitmap, failed_pos_bitmap_local, 
                                  clustered_disabled_bitmap, local_disabled_bitmask) ;
            fbe_raid_group_set_clustered_bitmap(raid_group_p, failed_pos_bitmap_local);
            fbe_raid_group_set_clustered_disabled_bitmap(raid_group_p, local_disabled_bitmask);
        }
    }

    fbe_check_rg_monitor_hooks(raid_group_p,
                               SCHEDULER_MONITOR_STATE_RAID_GROUP_EVAL_RL,
                               FBE_RAID_GROUP_SUBSTATE_EVAL_RL_STARTED, 
                               &hook_status);
    if (hook_status == FBE_SCHED_STATUS_DONE) {
        fbe_raid_group_unlock(raid_group_p);
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /* Now we need to wait for the peer to acknowledge that it saw us start.
     * By waiting for the passive side to get to this point, it allows the 
     * active side to clear the bits when it is complete. 
     * The passive side will also be able to know we are done when the bits are cleared.
     */
    fbe_raid_group_is_peer_flag_set_and_get_peer_bitmap(raid_group_p, &b_peer_flag_set, 
                                                        FBE_LIFECYCLE_STATE_READY, 
                                                        FBE_RAID_GROUP_CLUSTERED_FLAG_EVAL_RL_STARTED,
                                                        &failed_drives_bitmap_peer,
                                                        &failed_ios_bitmap_peer);
    if (!b_peer_flag_set)
    {
        fbe_raid_group_unlock(raid_group_p);
        /*Split trace to two lines*/
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "Active wait peer EVAL_RL_START locfl:0x%llx peerfl:0x%llx>\n",
                              (unsigned long long)raid_group_p->raid_group_metadata_memory.flags,
                              (unsigned long long)raid_group_p->raid_group_metadata_memory_peer.flags);
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "locmap:0x%x peermap:0x%x state:0x%llx<\n",
                              raid_group_p->raid_group_metadata_memory.failed_drives_bitmap,
                              raid_group_p->raid_group_metadata_memory_peer.failed_drives_bitmap,
                              (unsigned long long)(unsigned long long)raid_group_p->local_state);

        return FBE_LIFECYCLE_STATUS_DONE;
    } 

    /* Combine the local and peer failed pos bitmap
     */
    fbe_raid_group_get_local_bitmap(raid_group_p, &failed_pos_bitmap_local, &failed_ios_bitmap);
    fbe_raid_group_get_peer_bitmap(raid_group_p, FBE_LIFECYCLE_STATE_READY,
                                   &failed_drives_bitmap_peer,
                                   &failed_ios_bitmap_peer);
    fbe_raid_group_unlock(raid_group_p);    

    /* If the peer is joining, then we should get the failed drives bitmap directly. 
     * The above call will not get the failed drives bitmap if the peer is not present. 
     */
    if (fbe_cmi_is_peer_alive() &&
        fbe_base_config_is_peer_clustered_flag_set((fbe_base_config_t*)raid_group_p, 
                                                   (FBE_BASE_CONFIG_CLUSTERED_FLAG_JOIN_REQ |
                                                    FBE_BASE_CONFIG_CLUSTERED_FLAG_JOIN_STARTED)))
    {  
        /* We only need the failed drives bitmap in this case since the peer is in activate and is not 
         * performing I/Os, so it cannot have any failed I/Os. 
         */
        fbe_raid_position_bitmask_t unused;
        fbe_raid_group_get_clustered_peer_bitmap(raid_group_p, &failed_drives_bitmap_peer, &unused);
    }
    fbe_raid_group_get_drives_marked_nr(raid_group_p, &drives_marked_nr);
    combined_failed_bitmap = failed_pos_bitmap_local | failed_ios_bitmap | failed_drives_bitmap_peer | failed_ios_bitmap_peer | drives_marked_nr;
    failed_drives = fbe_raid_get_bitcount(combined_failed_bitmap);
    fbe_raid_geometry_get_parity_disks(raid_geometry_p, &num_parity_drives);

    /* We can only get into the situation where one side has seen a drive blip 
     * and the other side has seen some other drive going dead. 
     *  
     * If an I/O failed on one position X and a different position Y needs to get marked rebuild logging, 
     * then we may need to shutdown.  Marking RL in this situation would cause data loss for the stripe 
     * where we failed to write X since that stripe is now inconsistent and the rebuild would detect this inconsistency. 
     * Even more interesting is that it is actually possible that X and Y are in the same chunk, making the 
     * situation intractable. 
     *  
     * For this above case we will first check here if we are going broken at a coarse level. 
     * Then we will try to resolve this to a finer level, which might avoid us going broken in some cases. 
     * We may choose to wait for disabled drives to come back to prevent going broken. 
     * We might also choose to avoid going broken if we are not marking anything new and only have some I/Os that 
     * failed and are now retryable. 
     */
    if (failed_drives > num_parity_drives)
    {
        fbe_u16_t failed_drives_less_disabled = 0;
        fbe_raid_group_get_disabled_peer_bitmap(raid_group_p, &peer_disabled_bitmask);
        fbe_raid_group_get_disabled_pos_bitmask(raid_group_p, &local_disabled_bitmask);

        fbe_base_object_trace((fbe_base_object_t*) raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,  
                              "rg_set_rl_active: fdl/p:%x/%x dmnrl:%x fiosl/p:%x/%x dsl/p:%x/%x\n", 
                              failed_pos_bitmap_local, failed_drives_bitmap_peer, 
                              drives_marked_nr, failed_ios_bitmap, failed_ios_bitmap_peer,
                              local_disabled_bitmask, peer_disabled_bitmask);

        /*! @note If this is a virtual drive, determine if we are in pass-thru
         *        mode.  If we are in pass-thru mode we stay in Ready so that
         *        the parent raid group can mark NR on this virtual drive.
         */
        if ((class_id == FBE_CLASS_ID_VIRTUAL_DRIVE)                                                &&
            (fbe_raid_group_allow_virtual_drive_to_continue_degraded(raid_group_p,
                                                                     packet_p,
                                                                     drives_marked_nr) == FBE_TRUE)     )
        {
            fbe_check_rg_monitor_hooks(raid_group_p,
                                       SCHEDULER_MONITOR_STATE_RAID_GROUP_EVAL_RL,
                                       FBE_RAID_GROUP_SUBSTATE_EVAL_RL_ALLOW_CONTINUE_BROKEN, 
                                       &hook_status);
            if (hook_status == FBE_SCHED_STATUS_DONE) 
            {
                return FBE_LIFECYCLE_STATUS_DONE;
            }

            /* If we are broken and but allow the raid group to remain in the
             * ready state we must allow I/O (from the parent) so that it can
             * set NR.
             */
            fbe_raid_group_check_bitmap_and_continue(raid_group_p);
            fbe_raid_group_lock(raid_group_p);
            fbe_raid_group_set_local_state(raid_group_p, FBE_RAID_GROUP_LOCAL_STATE_EVAL_RL_DONE);
            fbe_raid_group_clear_local_state(raid_group_p, FBE_RAID_GROUP_LOCAL_STATE_EVAL_RL_STARTED);
            fbe_raid_group_unlock(raid_group_p);
            fbe_base_object_trace((fbe_base_object_t *)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "eval rl Active vd pass-thru loc fl: 0x%llx peer fl: 0x%llx "
                              " loc map: 0x%x peer map: 0x%x state: 0x%llx\n", 
                              raid_group_p->raid_group_metadata_memory.flags,
                              raid_group_p->raid_group_metadata_memory_peer.flags,
                              raid_group_p->raid_group_metadata_memory.failed_drives_bitmap,
                              raid_group_p->raid_group_metadata_memory_peer.failed_drives_bitmap,
                              raid_group_p->local_state);        
            return FBE_LIFECYCLE_STATUS_RESCHEDULE;
        }
        /* if failed drive seen on active sp and passive sp are not in sync then lets wait for some time before goes to
         *  the broken condition. If failed drives will be in sync on both sps or timer get expired, move on.
         */  
        if ((failed_pos_bitmap_local != failed_drives_bitmap_peer) && 
            fbe_raid_group_is_peer_present(raid_group_p)              )
        {
    
            /* wait for sometime before goes to the fail state */
            fbe_raid_group_needs_rebuild_determine_rg_broken_timeout(raid_group_p, FBE_RAID_GROUP_ACTIVATE_DEGRADED_WAIT_SEC,
                                                                     &b_is_timeout_expired, &elapsed_time);
    
            if(!b_is_timeout_expired)
            {
                /* wait till timer expired or if drives are in sync
                 */
                return FBE_LIFECYCLE_STATUS_DONE;
            }
        }
        local_disabled_bitmask &= ~drives_marked_nr;
        peer_disabled_bitmask &= ~drives_marked_nr;
        failed_drives_less_disabled = fbe_raid_get_bitcount(combined_failed_bitmap & ~(local_disabled_bitmask | peer_disabled_bitmask));

        /* If we take out the disabled drives and it is less than parity drives, then wait
         * for the disabled drives to come back, up to some limit. 
         */
        if (failed_drives_less_disabled <= num_parity_drives)
        {
            fbe_raid_group_needs_rebuild_determine_rg_broken_timeout(raid_group_p, FBE_RAID_GROUP_ACTIVATE_DEGRADED_WAIT_SEC,
                                                                     &b_is_timeout_expired, &elapsed_time);
    
            if (!b_is_timeout_expired)
            {
                /* Try to wait a bit before we go broken.
                 */
                fbe_base_object_trace((fbe_base_object_t *)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                      "rg_set_rl_active:  waiting for disabled drives. ldis_b: 0x%x pdis_b: 0x%x c_b: 0x%x\n",
                                      local_disabled_bitmask,
                                      peer_disabled_bitmask,
                                      combined_failed_bitmap);
                return FBE_LIFECYCLE_STATUS_DONE;
            }
        }
        /* clear the broken timestamp
         */
        fbe_raid_group_set_check_rg_broken_timestamp(raid_group_p, 0);

        /* Check to see if we can avoid broken by simply retrying failed I/Os.
         */
        mark_rl_bitmap = failed_pos_bitmap_local | failed_drives_bitmap_peer;

        if ( ((rl_bitmask | mark_rl_bitmap) == rl_bitmask) &&
             ((failed_ios_bitmap | failed_ios_bitmap_peer) != 0) )
        {
            /* We can avoid rebuild logging if nothing is changing. 
             */
            fbe_base_object_trace((fbe_base_object_t*) raid_group_p,      
                                  FBE_TRACE_LEVEL_INFO,           
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,  
                                  "rg_set_rl_active: Avoiding broken. we will retry failed I/Os l/p:%x/%x rl:%x\n",
                                  failed_ios_bitmap, failed_ios_bitmap_peer, rl_bitmask);
        }
        else
        {
            /* This debug hook will prevent raid group to go into broken state if it is set 
             */
            fbe_check_rg_monitor_hooks(raid_group_p,
                                       SCHEDULER_MONITOR_STATE_RAID_GROUP_EVAL_RL,
                                       FBE_RAID_GROUP_SUBSTATE_EVAL_RL_STARTED_BROKEN, 
                                       &hook_status);
            if (hook_status == FBE_SCHED_STATUS_DONE) {
             
                    return FBE_LIFECYCLE_STATUS_DONE;
            } 
    
            fbe_base_object_trace((fbe_base_object_t*) raid_group_p,      
                                  FBE_TRACE_LEVEL_INFO,           
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,  
                                  "rg_set_rl_active: Changing to broken f_drv:%d n_p_drv:%d rl:%x\n", 
                                  failed_drives, num_parity_drives, rl_bitmask);

            /* If enabled trace the fact that we are going to clear all the `blocks_rebuilt'.
             */
            for (rebuild_index = 0; rebuild_index < FBE_RAID_GROUP_MAX_REBUILD_POSITIONS; rebuild_index++)
            {
                fbe_raid_group_trace(raid_group_p,
                             FBE_TRACE_LEVEL_INFO,
                             FBE_RAID_GROUP_DEBUG_FLAG_REBUILD_NOTIFICATION_TRACING,
                             "raid_group: RB rg broken for index: %d reset blocks rebuilt: 0x%llx to 0\n",
                             rebuild_index,
                             raid_group_p->raid_group_metadata_memory.blocks_rebuilt[rebuild_index]);
                raid_group_p->raid_group_metadata_memory.blocks_rebuilt[rebuild_index] = 0;
            }

            /* The rg is going broken.  As part of cleaning up we will transition to the broken condition.
             */
            fbe_raid_group_lock(raid_group_p);
            fbe_raid_group_set_local_state(raid_group_p, FBE_RAID_GROUP_LOCAL_STATE_EVAL_RL_DONE);
            fbe_raid_group_set_local_state(raid_group_p, FBE_RAID_GROUP_LOCAL_STATE_EVAL_RL_BROKEN);
            fbe_raid_group_clear_local_state(raid_group_p, FBE_RAID_GROUP_LOCAL_STATE_EVAL_RL_STARTED);
            fbe_raid_group_unlock(raid_group_p);
            return FBE_LIFECYCLE_STATUS_RESCHEDULE;
        }
    }

    fbe_base_object_trace((fbe_base_object_t*) raid_group_p,
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s RL metadata memory; local bitmap 0x%x; peer bitmap 0x%x\n", __FUNCTION__,
                          failed_pos_bitmap_local | failed_ios_bitmap, 
                          failed_drives_bitmap_peer | failed_ios_bitmap_peer);

    /* If failed bitmask is not a subset of NP bitmask
     * that means we have some drive that needs to be marked for rebuild logging    
     */
    mark_rl_bitmap = failed_pos_bitmap_local | failed_drives_bitmap_peer;

    if ((rl_bitmask | mark_rl_bitmap) != rl_bitmask)
    {
        /*! @todo The previous set rb logging has event logging code. That needs to be added when
        rl bitmask is written out */ 

        fbe_transport_set_completion_function(packet_p, fbe_raid_group_write_rl_completion, 
                                              (fbe_packet_completion_context_t) raid_group_p);
        fbe_raid_group_write_rl_get_dl(raid_group_p, packet_p, mark_rl_bitmap,
                                FBE_FALSE/* used for clear rb logging*/);

        return FBE_LIFECYCLE_STATUS_PENDING;
    }

    status = fbe_raid_group_handle_glitching_drive(raid_group_p, packet_p);

    if (status == FBE_STATUS_OK)
    {
        /* This is the case where drive went away and came back immediately(edge bounce)
        */
        fbe_raid_group_check_bitmap_and_continue(raid_group_p);    
        fbe_raid_group_lock(raid_group_p);
        fbe_raid_group_set_local_state(raid_group_p, FBE_RAID_GROUP_LOCAL_STATE_EVAL_RL_DONE);
        fbe_raid_group_clear_local_state(raid_group_p, FBE_RAID_GROUP_LOCAL_STATE_EVAL_RL_STARTED);
        fbe_raid_group_unlock(raid_group_p);
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "eval rl Active no action loc fl: 0x%llx peer fl: 0x%llx "
                              " loc map: 0x%x peer map: 0x%x state: 0x%llx\n", 
                              (unsigned long long)raid_group_p->raid_group_metadata_memory.flags,
                              (unsigned long long)raid_group_p->raid_group_metadata_memory_peer.flags,
                              raid_group_p->raid_group_metadata_memory.failed_drives_bitmap,
                              raid_group_p->raid_group_metadata_memory_peer.failed_drives_bitmap,
                              (unsigned long long)raid_group_p->local_state);        
        return FBE_LIFECYCLE_STATUS_RESCHEDULE;
    }

    return FBE_LIFECYCLE_STATUS_PENDING;
}
/**************************************************
 * end fbe_raid_group_set_rb_logging_active()
 **************************************************/
/*!****************************************************************************
 * fbe_raid_group_eval_rb_logging_done()
 ******************************************************************************
 * @brief
 *  This function is the last step in eval where the clustered flags
 * and local flags will be cleared on both sides and the cond will be cleared
 *
 * @param raid_group_p - Raid group object
 * @param packet_p - packet pointer.
 * 
 * @return fbe_lifecycle_status_t
 *
 * @author
 *  6/28/2011 - Created Ashwin Tamilarasan
 *
 ******************************************************************************/
fbe_lifecycle_status_t 
fbe_raid_group_eval_rb_logging_done(fbe_raid_group_t *  raid_group_p, fbe_packet_t * packet_p)
{
    fbe_status_t status;
    fbe_bool_t b_is_active;
    fbe_bool_t b_peer_flag_set = FBE_FALSE;
    fbe_bool_t CSX_MAYBE_UNUSED b_peer_present = fbe_raid_group_is_peer_present(raid_group_p);
    fbe_scheduler_hook_status_t        hook_status = FBE_SCHED_STATUS_OK;

    fbe_raid_group_monitor_check_active_state(raid_group_p, &b_is_active);

    fbe_raid_group_lock(raid_group_p);
    /* If we are done and broken we need to send the broken bit to the peer.
     */
    if (fbe_raid_group_is_local_state_set(raid_group_p, FBE_RAID_GROUP_LOCAL_STATE_EVAL_RL_BROKEN) &&
        !fbe_raid_group_is_clustered_flag_set(raid_group_p, FBE_RAID_GROUP_CLUSTERED_FLAG_EVAL_RL_DONE_BROKEN))
    {
        fbe_raid_group_set_clustered_flag(raid_group_p, FBE_RAID_GROUP_CLUSTERED_FLAG_EVAL_RL_DONE_BROKEN);    
    }
    else
    {
        /* In order for the passive side to see we are broken, the
         * active side will set the broken clustered flag. 
         */
        fbe_raid_group_is_peer_flag_set(raid_group_p, &b_peer_flag_set,
                                        FBE_RAID_GROUP_CLUSTERED_FLAG_EVAL_RL_DONE_BROKEN);
        if (fbe_raid_group_is_peer_present(raid_group_p) &&
            b_peer_flag_set)
        {
            /* Peer has said we are done and broken.
             * Set the local state so that when we finish we will set the broken condition.
             */
            fbe_raid_group_set_local_state(raid_group_p, FBE_RAID_GROUP_LOCAL_STATE_EVAL_RL_BROKEN);
            fbe_base_object_trace((fbe_base_object_t *)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                  "eval rl %s, peer done broken is set local: 0x%llx peer: 0x%llx state:0x%llx\n", 
                                  b_is_active ? "Active" : "Passive",
                                  (unsigned long long)raid_group_p->raid_group_metadata_memory.flags,
                                  (unsigned long long)raid_group_p->raid_group_metadata_memory_peer.flags,
                                  (unsigned long long)raid_group_p->local_state);
        }
    }

    fbe_check_rg_monitor_hooks(raid_group_p,
                               SCHEDULER_MONITOR_STATE_RAID_GROUP_EVAL_RL,
                               FBE_RAID_GROUP_SUBSTATE_EVAL_RL_DONE, 
                               &hook_status);
    if (hook_status == FBE_SCHED_STATUS_DONE) {
        fbe_raid_group_unlock(raid_group_p);
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /* We want to clear our local flags first.
     */
    if (fbe_raid_group_is_any_clustered_flag_set(raid_group_p, 
                            (FBE_RAID_GROUP_CLUSTERED_FLAG_EVAL_RL_REQ|FBE_RAID_GROUP_CLUSTERED_FLAG_EVAL_RL_STARTED)))
    {
        fbe_raid_group_clear_clustered_flag_and_bitmap(raid_group_p, 
                                                       (FBE_RAID_GROUP_CLUSTERED_FLAG_EVAL_RL_REQ | 
                                                        FBE_RAID_GROUP_CLUSTERED_FLAG_EVAL_RL_STARTED));
        fbe_raid_group_unlock(raid_group_p);
        return FBE_LIFECYCLE_STATUS_RESCHEDULE;
    }

    /* We then wait for the peer to indicate it is done by clearing the started flag.
     */
    fbe_raid_group_is_peer_flag_set(raid_group_p, &b_peer_flag_set,
                                    FBE_RAID_GROUP_CLUSTERED_FLAG_EVAL_RL_STARTED);
    if (fbe_raid_group_is_peer_present(raid_group_p) && 
        b_peer_flag_set)
    {
        fbe_raid_group_unlock(raid_group_p);
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "eval rl cleanup %s, wait peer done local: 0x%llx peer: 0x%llx state:0x%llx\n", 
                              b_is_active ? "Active" : "Passive",
                              (unsigned long long)raid_group_p->raid_group_metadata_memory.flags,
                              (unsigned long long)raid_group_p->raid_group_metadata_memory_peer.flags,
                              (unsigned long long)raid_group_p->local_state);
        return FBE_LIFECYCLE_STATUS_DONE;
    }
    else
    {
        /* The local flags tell us that we are waiting for the peer to clear their flags.
         * Since it seems they are complete, it is safe to clear our local flags. 
         */
        fbe_raid_group_clear_local_state(raid_group_p, 
                                         (FBE_RAID_GROUP_LOCAL_STATE_EVAL_RL_STARTED | 
                                          FBE_RAID_GROUP_LOCAL_STATE_EVAL_RL_DONE));        
       
        if (fbe_raid_group_is_local_state_set(raid_group_p, FBE_RAID_GROUP_LOCAL_STATE_EVAL_RL_BROKEN))
        {
            /* If we decided we are broken, then we need to set the broken condition now.
             */ 
            fbe_base_object_trace((fbe_base_object_t *)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                  "eval rl done %s, broken is set local: 0x%llx peer: 0x%llx state:0x%llx\n", 
                                  b_is_active ? "Active" : "Passive",
                                  (unsigned long long)raid_group_p->raid_group_metadata_memory.flags,
                                  (unsigned long long)raid_group_p->raid_group_metadata_memory_peer.flags,
                                  (unsigned long long)raid_group_p->local_state);
            fbe_lifecycle_set_cond(&fbe_raid_group_lifecycle_const, (fbe_base_object_t *)raid_group_p, 
                                   FBE_BASE_CONFIG_LIFECYCLE_COND_DOWNSTREAM_HEALTH_BROKEN);
            fbe_raid_group_clear_clustered_flag_and_bitmap(raid_group_p, FBE_RAID_GROUP_CLUSTERED_FLAG_EVAL_RL_DONE_BROKEN);
            fbe_raid_group_clear_local_state(raid_group_p, FBE_RAID_GROUP_LOCAL_STATE_EVAL_RL_BROKEN);
        }

        /* It is possible that we received a request to eval either locally or from the peer.
         * If a request is not set, let's clear the condition. 
         * Otherwise we will run this condition again, and when we see that the started and done flags are not set, 
         * we'll re-evaluate rb logging. 
         */
        else if (!fbe_raid_group_is_local_state_set(raid_group_p, FBE_RAID_GROUP_LOCAL_STATE_EVAL_RL_REQUEST))
        {
            status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)raid_group_p);
            if (status != FBE_STATUS_OK)
            {
                fbe_base_object_trace((fbe_base_object_t *)raid_group_p, 
                                      FBE_TRACE_LEVEL_ERROR,
                                      FBE_TRACE_MESSAGE_ID_INFO,
                                      "%s can't clear current condition, status: 0x%x\n",
                                      __FUNCTION__, status);
            }
        }
        fbe_raid_group_unlock(raid_group_p);
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "eval rl %s cleanup complete local: 0x%llx peer: 0x%llx state: 0x%llx\n",
                              b_is_active ? "Active" : "Passive",
                              (unsigned long long)raid_group_p->raid_group_metadata_memory.flags,
                              (unsigned long long)raid_group_p->raid_group_metadata_memory_peer.flags,
                              (unsigned long long)raid_group_p->local_state);
        return FBE_LIFECYCLE_STATUS_RESCHEDULE;
    }    
}
/**************************************************
 * end fbe_raid_group_eval_rb_logging_done()
 **************************************************/

/*!***************************************************************************
 *          fbe_raid_group_get_number_of_virtual_drive_edges_to_wait_for()
 ***************************************************************************** 
 * 
 * @brief   This function will determine how many edges on the virtual drive
 *          we should wait for before eval rebuild logging should proceed.
 *
 * 
 * @param   raid_group_p - pointer to a raid group object
 * @param   packet_p - Monitor packet to use for virtual drive usurper
 * @param   number_of_edges_to_wait_for_p - Pointer to number of edges to wait for
 * 
 * @return  status
 *
 * @author
 *  12/11/2012  Ron Proulx  - Created.
 *****************************************************************************/
fbe_status_t fbe_raid_group_get_number_of_virtual_drive_edges_to_wait_for(fbe_raid_group_t *raid_group_p,
                                                                          fbe_packet_t *packet_p,
                                                                          fbe_u32_t *number_of_edges_to_wait_for_p)
{
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_object_id_t                     object_id;
    fbe_class_id_t                      class_id;
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

    /* By default the number of edges to wait for is 1.
     */
    *number_of_edges_to_wait_for_p = 1;

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
                              "raid_group: get vd edges to wait for invalid class id: %d \n",
                              class_id);
        return FBE_STATUS_GENERIC_FAILURE;
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
                              "raid_group: get vd edges to wait for failed to allocate control op.\n");
        return FBE_STATUS_GENERIC_FAILURE;
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
                              "raid_group: get vd edges to wait for - no upstream edge. Return 1\n");

        return FBE_STATUS_OK;
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
                              "raid_group: get vd edges to wait for failed to get config status: 0x%x control: 0x%x \n",
                              status, control_status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* First get the path state and path attributes of the upstream edge
     */
    configuration_mode = get_configuration.configuration_mode;
    fbe_block_transport_get_path_state(upstream_edge_p, &upstream_edge_path_state);
    fbe_block_transport_get_path_attributes(upstream_edge_p, &upstream_edge_path_attr);

    /* Trace some information.
     */
    fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                          FBE_TRACE_LEVEL_DEBUG_LOW,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "raid_group: get vd edges to wait mode: %d upstream path state: %d path attr: 0x%x\n",
                          configuration_mode, upstream_edge_path_state, upstream_edge_path_attr);

    /* Now based on the virtual drive configuration mode determine if we are allowed
     * degraded or not.
     */
    switch(configuration_mode)
    {
        case FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_FIRST_EDGE:
        case FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_SECOND_EDGE:
            /* If pass-thru return (1) edge to wait for.
             */
            *number_of_edges_to_wait_for_p = 1;
            break;

        case FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_FIRST_EDGE:
        case FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_SECOND_EDGE:
            /* If mirror mode wait for (2) edges.
             */
            *number_of_edges_to_wait_for_p = 2;
            break;

        default:
            /* Unsupported mode. Sets edges to wait for to 1
             */
            fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                                  FBE_TRACE_LEVEL_WARNING,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "raid_group: get vd edges to wait mode: %d unsupported. set edge to wait for to 1\n",
                                  configuration_mode);
    }

    /* Return success
     */
    return FBE_STATUS_OK; 
}
/********************************************************************
 * end fbe_raid_group_get_number_of_virutal_drive_edges_to_wait_for()
 ********************************************************************/

/*!***************************************************************************
 *          fbe_raid_group_is_virtual_drive_in_mirror_mode()
 ***************************************************************************** 
 * 
 * @brief   This function will determine if the virtual drive virtual drive
 *          is in mirror mode or not (i.e. it is in pass-thru mode).
 *
 * @param   raid_group_p - pointer to a raid group object
 * @param   packet_p - Monitor packet to use for virtual drive usurper
 * @param   b_is_in_mirror_mode_p - Pointer to bool to update:
 *              FBE_TRUE - Virtual drive is in mirror mode
 *              FBE_FALSE - Virtual drive is in pass-thru mode
 * 
 * @return  status
 *
 * @author
 *  12/12/2012  Ron Proulx  - Created.
 *****************************************************************************/
fbe_status_t fbe_raid_group_is_virtual_drive_in_mirror_mode(fbe_raid_group_t *raid_group_p,
                                                            fbe_packet_t *packet_p,
                                                            fbe_bool_t *b_is_in_mirror_mode_p)
{
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_object_id_t                     object_id;
    fbe_class_id_t                      class_id;
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

    /* By default the virtual drive is in pass-thru mode
     */
    *b_is_in_mirror_mode_p = FBE_FALSE;

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
                              "raid_group: is vd in mirror mode for invalid class id: %d \n",
                              class_id);
        return FBE_STATUS_GENERIC_FAILURE;
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
                              "raid_group: is vd in mirror mode for failed to allocate control op.\n");
        return FBE_STATUS_GENERIC_FAILURE;
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
                              "raid_group: is vd in mirror mode - no upstream edge. Return False\n");

        return FBE_STATUS_OK;
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

    /* Check the completion status and release the control operation and clear
     * the block edge (monitor request).
     */
    status = fbe_transport_get_status_code(packet_p);
    fbe_payload_control_get_status(control_operation_p, &control_status);
    fbe_payload_ex_release_control_operation(payload_p, control_operation_p);
    fbe_transport_set_status(packet_p, FBE_STATUS_INVALID, 0);
    packet_p->base_edge = NULL;

    /* If the request failed trace the error and return false.
     */
    if ((status != FBE_STATUS_OK)                         ||
        (control_status != FBE_PAYLOAD_CONTROL_STATUS_OK)    ) 
    {
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "raid_group: is vd in mirror mode failed to get config status: 0x%x control: 0x%x \n",
                              status, control_status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* First get the path state and path attributes of the upstream edge
     */
    configuration_mode = get_configuration.configuration_mode;
    fbe_block_transport_get_path_state(upstream_edge_p, &upstream_edge_path_state);
    fbe_block_transport_get_path_attributes(upstream_edge_p, &upstream_edge_path_attr);

    /* Trace some information.
     */
    fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                          FBE_TRACE_LEVEL_DEBUG_LOW,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "raid_group: is vd in mirror mode: %d upstream path state: %d path attr: 0x%x\n",
                          configuration_mode, upstream_edge_path_state, upstream_edge_path_attr);

    /* Now based on the virtual drive configuration mode determine if we are allowed
     * degraded or not.
     */
    switch(configuration_mode)
    {
        case FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_FIRST_EDGE:
        case FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_SECOND_EDGE:
            /* If pass-thru return False.
             */
            *b_is_in_mirror_mode_p = FBE_FALSE;
            break;

        case FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_FIRST_EDGE:
        case FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_SECOND_EDGE:
            /* If mirror mode return True
             */
            *b_is_in_mirror_mode_p = FBE_TRUE;
            break;

        default:
            /* Unsupported mode. Sets edges to wait for to 1
             */
            fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                                  FBE_TRACE_LEVEL_WARNING,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "raid_group: is vd in mirror mode mode: %d unsupported. return False\n",
                                  configuration_mode);
    }

    /* Return success
     */
    return FBE_STATUS_OK; 
}
/******************************************************
 * end fbe_raid_group_is_virtual_drive_in_mirror_mode()
 ******************************************************/

/*!***************************************************************************
 *          fbe_raid_group_get_virtual_drive_valid_edge_bitmask()
 ***************************************************************************** 
 * 
 * @brief   This function will determine the bitmask of which edges of the
 *          virtual drive are valid.  This is needed so that the evaluate
 *          mark NR condition will not attempt to mark NR for a virtual drive
 *          that is in pass-thru mode with more than (1) valid edge.  That
 *          scenario can occur if the eval mark NR condition is set but the
 *          copy job/virtual drive has not yet updated the configuration mode.
 * 
 * @param   raid_group_p - pointer to a raid group object
 * @param   packet_p - Monitor packet to use for virtual drive usurper
 * @param   bitmask_p - Pointer to bitmask to set with the valid positions
 * 
 * @return  status
 *
 * @author
 *  02/07/2013  Ron Proulx  - Created.
 *****************************************************************************/
fbe_status_t fbe_raid_group_get_virtual_drive_valid_edge_bitmask(fbe_raid_group_t *raid_group_p,
                                                                 fbe_packet_t *packet_p,
                                                                 fbe_raid_position_bitmask_t *bitmask_p)
{
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_object_id_t                     object_id;
    fbe_class_id_t                      class_id;
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

    /* By default there are no edges.
     */
    *bitmask_p = 0;

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
                              "raid_group: get vd edge bitmask - invalid class id: %d \n",
                              class_id);
        return FBE_STATUS_GENERIC_FAILURE;
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
                              "raid_group: get vd edge bitmask failed to allocate control op.\n");
        return FBE_STATUS_GENERIC_FAILURE;
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
                              "raid_group: get vd edge bitmask - no upstream edge. Return 0\n");

        return FBE_STATUS_OK;
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
                              "raid_group: get vd edge bitmask failed to get config status: 0x%x control: 0x%x \n",
                              status, control_status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* First get the path state and path attributes of the upstream edge
     */
    configuration_mode = get_configuration.configuration_mode;
    fbe_block_transport_get_path_state(upstream_edge_p, &upstream_edge_path_state);
    fbe_block_transport_get_path_attributes(upstream_edge_p, &upstream_edge_path_attr);

    /* Trace some information.
     */
    fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                          FBE_TRACE_LEVEL_DEBUG_LOW,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "raid_group: get vd edge bitmask mode: %d upstream path state: %d path attr: 0x%x\n",
                          configuration_mode, upstream_edge_path_state, upstream_edge_path_attr);

    /* Now based on the virtual drive configuration mode determine if we are allowed
     * degraded or not.
     */
    switch(configuration_mode)
    {
        case FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_FIRST_EDGE:
            /* Pass-thru first edge.  Set the mask accordingly.
             */
            *bitmask_p = (1 << FIRST_EDGE_INDEX);
            break;

        case FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_SECOND_EDGE:
            /* Pass-thru second edge.  Set the mask accordingly.
             */
            *bitmask_p = (1 << SECOND_EDGE_INDEX);
            break;

        case FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_FIRST_EDGE:
        case FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_SECOND_EDGE:
            /* If mirror mode wait for (2) edges.
             */
            *bitmask_p = ((1 << FIRST_EDGE_INDEX) | (1 << SECOND_EDGE_INDEX));
            break;

        default:
            /* Unsupported mode. Sets edges to wait for to 0
             */
            status = FBE_STATUS_GENERIC_FAILURE;
            fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                                  FBE_TRACE_LEVEL_WARNING,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "raid_group: get vd edge bitmask mode: %d unsupported. set edge to wait for to 1\n",
                                  configuration_mode);
    }

    /* Return status
     */
    return status; 
}
/********************************************************************
 * end fbe_raid_group_get_virtual_drive_valid_edge_bitmask()
 ********************************************************************/


/***********************************
 * end file fbe_raid_group_eval_rl.c
 ***********************************/


