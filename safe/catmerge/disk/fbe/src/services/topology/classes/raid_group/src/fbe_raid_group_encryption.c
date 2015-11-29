/***************************************************************************
 * Copyright (C) EMC Corporation 2013
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_raid_group_encryption.c
 ***************************************************************************
 *
 * @brief
 *  This file contains Monitor related encryption functionality.
 *
 * @version
 *   10/15/2013:  Created. Rob Foley
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/

#include "fbe_raid_group_rebuild.h"
#include "fbe_raid_group_bitmap.h"
#include "fbe_raid_group_object.h"
#include "fbe_raid_group_needs_rebuild.h"
#include "fbe_raid_group_logging.h"
#include "fbe_raid_group_executor.h"
#include "fbe_transport_memory.h"
#include "fbe_notification.h"
#include "fbe_raid_verify.h"
#include "fbe/fbe_memory_persistence.h"
/*************************
 *   FUNCTION DEFINITIONS
 *************************/

static fbe_status_t fbe_raid_group_encryption_send_io(fbe_raid_group_t*           raid_group_p, 
                                                         fbe_packet_t*               packet_p,
                                                         fbe_bool_t b_queue_to_block_transport,
                                                         fbe_lba_t                   start_lba,
                                                         fbe_block_count_t           block_count);

static fbe_status_t fbe_raid_group_encryption_send_io_completion(fbe_packet_t*                   packet_p, 
                                                                 fbe_packet_completion_context_t context);

static fbe_status_t fbe_raid_group_encryption_request_permission(fbe_raid_group_t *raid_group_p, 
                                                                 fbe_raid_group_encryption_context_t *encryption_context_p,
                                                                 fbe_packet_t *packet_p);
static fbe_status_t fbe_raid_group_encryption_request_permission_completion(void *event_p,
                                                                            fbe_event_completion_context_t  context);
static fbe_status_t fbe_raid_group_encryption_completion(fbe_packet_t *packet_p,
                                                                     fbe_packet_completion_context_t context);
static fbe_status_t fbe_raid_group_incr_rekey_checkpoint(fbe_raid_group_t *raid_group_p,
                                                         fbe_packet_t *packet_p,
                                                         fbe_lba_t start_lba,
                                                         fbe_block_count_t block_count); 

fbe_status_t fbe_raid_group_clear_encryption(fbe_raid_group_t *raid_group_p, 
                                             fbe_packet_t *packet_p);
fbe_status_t fbe_raid_group_encryption_write_zeros(fbe_raid_group_t *raid_group_p, 
                                                   fbe_packet_t *packet_p);

static fbe_status_t 
fbe_raid_group_clear_encryption_unbound_completion(fbe_packet_t *packet_p,
                                                   fbe_packet_completion_context_t context);

static fbe_status_t fbe_raid_group_clr_encryption_unbound_update_np(fbe_raid_group_t *raid_group_p, 
                                                                    fbe_packet_t *packet_p);

static fbe_status_t fbe_raid_group_encryption_complete_update_np(fbe_packet_t * packet_p, 
                                                                 fbe_packet_completion_context_t context);
static fbe_status_t fbe_raid_group_encryption_inc_pos(fbe_packet_t * packet_p, 
                                                      fbe_packet_completion_context_t context);

static fbe_status_t fbe_raid_group_notify_pvd_encr_alloc_complete(fbe_memory_request_t * memory_request_p,
                                                                  fbe_memory_completion_context_t context);

static fbe_status_t fbe_raid_group_notify_pvd_encryption_subpacket_completion(fbe_packet_t * packet_p,
                                                                              fbe_packet_completion_context_t context);

static fbe_status_t fbe_raid_group_encryption_mark_pvd_notified(fbe_packet_t * packet_p, 
                                                                fbe_packet_completion_context_t context);
static fbe_status_t fbe_raid_group_notify_pvd_encryption_completion(fbe_packet_t * packet_p,
                                                                    fbe_packet_completion_context_t context);
static fbe_status_t fbe_raid_group_clear_paged_nr(fbe_packet_t * packet_p, 
                                                  fbe_packet_completion_context_t context);
static fbe_status_t fbe_raid_group_rekey_handle_chunks(fbe_raid_group_t *raid_group_p,
                                                       fbe_packet_t *packet_p);

static fbe_status_t fbe_raid_group_get_encryption_context(fbe_raid_group_t *raid_group_p,
                                                   fbe_packet_t *packet_p,
                                                   fbe_raid_group_encryption_context_t **encryption_context_p);
static fbe_status_t fbe_raid_group_release_encryption_context(fbe_packet_t*       packet_p);
static fbe_status_t fbe_raid_group_initialize_encryption_context(fbe_raid_group_t *raid_group_p,
                                                          fbe_raid_group_encryption_context_t *encryption_context_p, 
                                                          fbe_lba_t encryption_lba);
static fbe_status_t fbe_raid_group_allocate_encryption_context(fbe_raid_group_t*   raid_group_p,
                                                               fbe_packet_t*       packet_p,
                                                               fbe_raid_group_encryption_context_t *encryption_context_p);

static fbe_status_t fbe_raid_group_rekey_read_completion(fbe_packet_t*  packet_p,
                                                         fbe_packet_completion_context_t context);
static fbe_status_t fbe_raid_group_notify_lower_encr_alloc_complete(fbe_memory_request_t * memory_request_p,
                                                                    fbe_memory_completion_context_t context);
static fbe_status_t fbe_raid_group_notify_lower_encryption_subpacket_completion(fbe_packet_t * packet_p,
                                                                                fbe_packet_completion_context_t context);
static fbe_status_t fbe_raid_group_rekey_set_chkpt_ds(fbe_raid_group_t *raid_group_p,
                                                      fbe_packet_t *packet_p,
                                                      fbe_lba_t lba,
                                                      fbe_block_count_t blocks);
static fbe_status_t fbe_raid_group_encryption_write_zeros_completion(fbe_packet_t *packet_p,
                                                                     fbe_packet_completion_context_t context);
static fbe_status_t fbe_raid_group_notify_vd_encryption_start(fbe_raid_group_t *raid_group_p,
                                                              fbe_packet_t *packet_p);
static fbe_status_t fbe_raid_group_notify_vd_encryption_subpacket_completion(fbe_packet_t * packet_p,
                                                                             fbe_packet_completion_context_t context);

static fbe_status_t fbe_raid_group_get_ds_encryption_state_alloc_complete(fbe_memory_request_t * memory_request_p,
                                                                          fbe_memory_completion_context_t context);

static fbe_status_t fbe_raid_group_encrption_state_chk_subpacket_completion(fbe_packet_t * packet_p,
                                                                            fbe_packet_completion_context_t context);
    
static fbe_status_t fbe_raid_group_encrption_state_chk_completion(fbe_packet_t * packet_p,
                                                                  fbe_packet_completion_context_t context);

static fbe_status_t fbe_raid_group_post_checkpoint_update(fbe_packet_t *packet_p,
                                                          fbe_packet_completion_context_t context);

static fbe_status_t fbe_raid_group_rekey_clear_degraded(fbe_packet_t *packet_p,    
                                                        fbe_raid_group_t *raid_group_p);
/*!**************************************************************
 * fbe_raid_group_encryption_is_rekey_needed()
 ****************************************************************
 * @brief
 *  This function checks to see if a rekey is required.
 *
 * @param raid_group_p           - pointer to the raid group
 *
 * @return bool status - returns FBE_TRUE if the operation needs to run
 *                       otherwise returns FBE_FALSE
 *
 * @author
 *  10/15/2013 - Created. Rob Foley
 *
 ****************************************************************/
fbe_bool_t
fbe_raid_group_encryption_is_rekey_needed(fbe_raid_group_t*    raid_group_p)
{
    fbe_base_config_encryption_mode_t mode;
    fbe_base_config_encryption_state_t state;
    fbe_raid_group_nonpaged_metadata_t* nonpaged_metadata_p = NULL;
    fbe_raid_geometry_t *raid_geometry_p = NULL;
    fbe_lba_t checkpoint;
    fbe_lba_t exported_capacity;

    fbe_base_config_get_encryption_mode((fbe_base_config_t*)raid_group_p, &mode);
    fbe_base_config_get_encryption_state((fbe_base_config_t*)raid_group_p, &state);

    if (!fbe_base_config_is_system_bg_service_enabled(FBE_BASE_CONFIG_CONTROL_SYSTEM_BG_SERVICE_ENABLE_ENCRYPTION_REKEY)){
        return FBE_FALSE;
    }
    
    /*  Get a pointer to the raid group object non-paged metadata  */
    fbe_base_config_get_nonpaged_metadata_ptr((fbe_base_config_t*) raid_group_p, (void **)&nonpaged_metadata_p);

    raid_geometry_p = fbe_raid_group_get_raid_geometry(raid_group_p);

    checkpoint = fbe_raid_group_get_rekey_checkpoint(raid_group_p);
    exported_capacity = fbe_raid_group_get_exported_disk_capacity(raid_group_p);

    if (fbe_raid_geometry_is_mirror_under_striper(raid_geometry_p) &&
        fbe_raid_group_is_encryption_state_set(raid_group_p, FBE_RAID_GROUP_ENCRYPTION_FLAG_PVD_NOTIFIED) &&
        fbe_raid_group_is_encryption_state_set(raid_group_p, FBE_RAID_GROUP_ENCRYPTION_FLAG_PAGED_ENCRYPTED) &&
        (checkpoint < exported_capacity)) {
        /* The striper level drives encryption.
         * The mirror level will notify pvd and encrypt the paged. 
         * The mirror level also needs to "complete" encryption once that the checkpoint gets far enough
         */
        return FBE_FALSE;
    }
    /* We need to run rekey code if we are in rekey or we need to complete rekey.
     */
    if (fbe_base_config_is_rekey_mode((fbe_base_config_t *)raid_group_p) ||
        (fbe_base_config_is_encrypted_mode((fbe_base_config_t *)raid_group_p) &&
         (nonpaged_metadata_p->encryption.flags != 0))){
        return FBE_TRUE;
    }
    return FBE_FALSE;
}
/******************************************************************************
 * end fbe_raid_group_encryption_is_rekey_needed()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_raid_group_encryption_request_permission()
 ******************************************************************************
 * @brief
 *   This function will send an event in order to determine if the LBA and range
 *   to be encrypted is within a LUN.  The event's completion function will proceed
 *   accordingly.  However, before sending the event, it will first check if the 
 *   LBA is outside of the RG's data area and instead is in the paged metadata.  
 *   In that case, it will proceed directly to the next step which is to send i/o request.
 *
 * @param raid_group_p           - pointer to a raid group object
 * @param packet_p               - pointer to a control packet from the scheduler
 * @param start_lba              - starting LBA of the I/O. The LBA is relative 
 *                                    to the RAID extent on a single disk. 
 * @param block_count            - number of blocks to rekey 
 *  
 * @return fbe_status_t
 * 
 * @author
 *  10/15/2013 - Created. Rob Foley
 *
 *****************************************************************************/
static fbe_status_t fbe_raid_group_encryption_request_permission(
                                        fbe_raid_group_t*                   raid_group_p, 
                                        fbe_raid_group_encryption_context_t*   encryption_context_p,
                                        fbe_packet_t*                       packet_p)
{

    fbe_event_t*                        event_p = NULL;
    fbe_event_stack_t*                  event_stack_p = NULL;
    fbe_raid_geometry_t*                raid_geometry_p = NULL;
    fbe_u16_t                           data_disks;
    fbe_lba_t                           logical_start_lba;
    fbe_block_count_t                   logical_block_count;
    fbe_lba_t                           external_capacity;
    fbe_event_permit_request_t          permit_request_info;
    fbe_lba_t                           io_block_count = FBE_LBA_INVALID;

    //  Get the external capacity of the RG, which is the area available for data and does not include the paged
    //  metadata or signature area.  This is stored in the base config's capacity.
    fbe_base_config_get_capacity((fbe_base_config_t*) raid_group_p, &external_capacity);

    //  We want to check the start LBA against the external capacity.  When comparing it to the external capacity 
    //  or sending it up to the LUN, we need to use the "logical" LBA relative to the start of the RG.  The LBA
    //  passed in to this function is what is used by rekey and is relative to the start of the disk, so convert
    //  it. 
    raid_geometry_p = fbe_raid_group_get_raid_geometry(raid_group_p);
    fbe_raid_geometry_get_data_disks(raid_geometry_p, &data_disks);
    logical_start_lba   = encryption_context_p->start_lba * data_disks; 
    logical_block_count = encryption_context_p->block_count * data_disks;

    /* Trace some information if enabled.
     */
    fbe_raid_group_trace(raid_group_p,
                         FBE_TRACE_LEVEL_INFO,
                         FBE_RAID_GROUP_DEBUG_FLAG_ENCRYPTION_TRACING,
                         "Encryption permit rq for lba: 0x%llx blocks: 0x%llx exported: 0x%llx data_disks: %d\n",
                         (unsigned long long)logical_start_lba,
                         (unsigned long long)logical_block_count,
                         (unsigned long long)external_capacity,
                         data_disks);

    //  If the LBA to check is outside of (greater than) the external capacity, then it is in the paged metadata 
    //  area and we do not need to check if it is in a LUN.  We'll go directly to sending the encryption I/O.  
    if (logical_start_lba >= external_capacity)
    {
        // Send the rekey I/O .  Its completion function will execute when it finishes. 
        fbe_raid_group_encryption_send_io(raid_group_p, packet_p, 
                                             FBE_FALSE, /* Don't need to break the context. */
                                             encryption_context_p->start_lba, 
                                             encryption_context_p->block_count);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED; 
    }

    /* Make sure that the permit request is limit to one client only. If the IO range is
     * across one client, we need to update the block count so that permit request goes to 
     * only one client.
     */

    /* make sure that the permit request IO range limit to only one client */
    if(raid_geometry_p->raid_type != FBE_RAID_GROUP_TYPE_INTERNAL_MIRROR_UNDER_STRIPER)
    {
         io_block_count = encryption_context_p->block_count;
         fbe_raid_group_monitor_update_client_blk_cnt_if_needed(raid_group_p, encryption_context_p->start_lba, &io_block_count);
         if((io_block_count != FBE_LBA_INVALID) && (encryption_context_p->block_count != io_block_count))
         {

            fbe_base_object_trace((fbe_base_object_t*) raid_group_p, 
                FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO, 
                "Encryption: send permit s_lba 0x%llx, orig blk 0x%llx, new blk 0x%llx \n", (unsigned long long)encryption_context_p->start_lba,
                (unsigned long long)encryption_context_p->block_count, (unsigned long long)io_block_count);

            /* update the rekey block count so that permit request will limit to only one client */
            encryption_context_p->block_count = io_block_count;
            logical_block_count = encryption_context_p->block_count * data_disks;
            
        }
    }

    //  We need to check if the LBA is within a LUN.   Allocate an event and set it up. 
    event_p = fbe_memory_ex_allocate(sizeof(fbe_event_t));
    fbe_event_init(event_p);
    fbe_event_set_master_packet(event_p, packet_p);
    event_stack_p = fbe_event_allocate_stack(event_p);

    //  Initialize the permit request information to "no LUN found"
    fbe_event_init_permit_request_data(event_p, &permit_request_info,
                                       FBE_PERMIT_EVENT_TYPE_IS_CONSUMED_REQUEST);

    //  Set the starting LBA and the block count to be checked in the event stack data
    fbe_event_stack_set_extent(event_stack_p, logical_start_lba, logical_block_count); 

    //  Set the completion function in the event's stack data
    fbe_event_stack_set_completion_function(event_stack_p, 
            (fbe_event_completion_function_t)fbe_raid_group_encryption_request_permission_completion, raid_group_p);

    //  Send a "permit request" event to check if this LBA and block are in a LUN or not  
    fbe_base_config_send_event((fbe_base_config_t*) raid_group_p, FBE_EVENT_TYPE_PERMIT_REQUEST, event_p);

    //  Return success   
    return FBE_STATUS_OK;

} // End fbe_raid_group_encryption_request_permission()

/*!****************************************************************************
 * fbe_raid_group_encryption_request_permission_completion()
 ******************************************************************************
 * @brief
 *   This is the completion function for the event which determines if the current 
 *   area to be rekeyed is in a bound area or an unbound area.  It will retrieve
 *   the data from the event and then call a function to get the paged metadata
 *   (chunk info).  
 *
 * @param event_p            - pointer to a Permit Request event
 * @param context            - context, which is a pointer to the raid group
 *                                object
 *
 * @return fbe_status_t
 *
 * @author
 *  10/15/2013 - Created. Rob Foley
 *
 *****************************************************************************/
static fbe_status_t 
fbe_raid_group_encryption_request_permission_completion(void *evt_p,
                                                        fbe_event_completion_context_t  context)
{
    fbe_event_t*                        event_p = NULL;

    fbe_raid_group_t*                   raid_group_p = NULL;
    fbe_event_stack_t*                  event_stack_p = NULL;
    fbe_event_status_t                  event_status;
    fbe_packet_t*                       packet_p = NULL;
    fbe_event_permit_request_t          permit_request_info;
    fbe_block_count_t                   unconsumed_block_count = 0;
    fbe_raid_group_encryption_context_t   *encryption_context_p = NULL;
    fbe_status_t                        status;
    fbe_raid_geometry_t                *raid_geometry_p = NULL;
    fbe_u16_t                           data_disks;
    fbe_u32_t                           event_flags = 0;
    fbe_bool_t                          is_io_consumed;
    fbe_block_count_t                   io_block_count = FBE_LBA_INVALID;

    event_p = (fbe_event_t*)evt_p;

    raid_group_p = (fbe_raid_group_t*) context;
    raid_geometry_p = fbe_raid_group_get_raid_geometry(raid_group_p);
    fbe_raid_geometry_get_data_disks(raid_geometry_p, &data_disks);

    event_stack_p = fbe_event_get_current_stack(event_p);
    fbe_event_get_status(event_p, &event_status);
    fbe_event_get_flags (event_p, &event_flags);
    fbe_event_get_master_packet(event_p, &packet_p);

    fbe_raid_group_get_encryption_context(raid_group_p, packet_p, &encryption_context_p);

    //  Get the permit request info about the LUN (object id and start/end of LUN) and store it in the rekey
    //  tracking structure
    fbe_event_get_permit_request_data(event_p, &permit_request_info); 
    encryption_context_p->lun_object_id     = permit_request_info.object_id;
    encryption_context_p->is_lun_start_b    = permit_request_info.is_start_b;
    encryption_context_p->is_lun_end_b      = permit_request_info.is_end_b;
    unconsumed_block_count               = permit_request_info.unconsumed_block_count;
    encryption_context_p->object_index = permit_request_info.object_index;
    encryption_context_p->b_beyond_capacity = permit_request_info.is_beyond_capacity_b;
    encryption_context_p->top_lba = permit_request_info.top_lba;

    /*! @note Free all associated resources and release the event.
     *        Do not access event information after this point.
     */
    fbe_event_release_stack(event_p, event_stack_p);
    fbe_event_destroy(event_p);
    fbe_memory_ex_release(event_p);

    /* Trace some information if enalbed.
     */
    fbe_raid_group_trace(raid_group_p,
                         FBE_TRACE_LEVEL_INFO,
                         FBE_RAID_GROUP_DEBUG_FLAG_ENCRYPTION_TRACING,
                         "Encryption permit status: 0x%x obj: 0x%x is_start: %d is_end: %d unconsumed: 0x%llx \n",
                         event_status, encryption_context_p->lun_object_id, encryption_context_p->is_lun_start_b,
                         encryption_context_p->is_lun_end_b, (unsigned long long)unconsumed_block_count);

    /* complete the packet if permit request denied or returned busy */
    if((event_flags == FBE_EVENT_FLAG_DENY) || (event_status == FBE_EVENT_STATUS_BUSY))
    {
        fbe_raid_group_trace(raid_group_p,
                             FBE_TRACE_LEVEL_INFO, FBE_RAID_GROUP_DEBUG_FLAGS_MONITOR_EVENT_TRACING,
                             "Encryption permit event request denied. lba: 0x%llx blocks: 0x%llx status: %d flags: 0x%x\n",
                             encryption_context_p->start_lba, encryption_context_p->block_count, event_status, event_flags);

        // our client is busy, we will need to try again later, we can't do the rekey now
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

            io_block_count = encryption_context_p->block_count;

            /* check if the rekey IO range is overlap with unconsumed area either at beginning or at the end */
            status = fbe_raid_group_monitor_io_get_consumed_info(raid_group_p,
                                                    unconsumed_block_count,
                                                    encryption_context_p->start_lba, 
                                                    &io_block_count,
                                                    &is_io_consumed);
            if(status != FBE_STATUS_OK)
            {
                fbe_base_object_trace((fbe_base_object_t*) raid_group_p, FBE_TRACE_LEVEL_INFO, 
                    FBE_TRACE_MESSAGE_ID_INFO, "Encryption failed to determine IO consumed area\n");
                fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
                fbe_transport_complete_packet(packet_p);

                return FBE_STATUS_OK;
            }
            
            if(is_io_consumed)
            {
                if(encryption_context_p->block_count != io_block_count)
                {
                    /* IO range is consumed at the beginning and unconsumed at the end
                     * update the rekey block count with updated block count which only cover the 
                     * consumed area
                     */

                    fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                            FBE_TRACE_LEVEL_INFO,
                            FBE_TRACE_MESSAGE_ID_INFO,
                            "Encryption IO has unconsumed area at end, lba 0x%llx," 
                            "orig_b 0x%llx, (unsigned long long)new_b 0x%llx\n",
                            encryption_context_p->start_lba, encryption_context_p->block_count, io_block_count);                    

                    encryption_context_p->block_count = io_block_count;
                    
                }
            }
            else
            {
                /* IO range is unconsumed at the beginning and consumed at the end.
                 * We will write zeros to this area, updated the paged metadata and advance the checkpoint.
                 */
                fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                        FBE_TRACE_LEVEL_INFO,
                        FBE_TRACE_MESSAGE_ID_INFO,
                            "Encryption IO has unconsumed area at start, move chkpt, lba 0x%llx," 
                            "orig_b 0x%llx, (unsigned long long)new_b 0x%llx\n",
                        encryption_context_p->start_lba, encryption_context_p->block_count, io_block_count); 
                
                encryption_context_p->block_count = io_block_count;
                status = fbe_raid_group_encryption_write_zeros(raid_group_p, packet_p);

                return FBE_STATUS_MORE_PROCESSING_REQUIRED;
            }
            break;

        case FBE_EVENT_STATUS_NO_USER_DATA:


            io_block_count = encryption_context_p->block_count;
            
            /* find out the unconsumed block counts that needs to skip with advance rekey checkpoint */
            status = fbe_raid_group_monitor_io_get_unconsumed_block_counts(raid_group_p, unconsumed_block_count, 
                                                encryption_context_p->start_lba, &io_block_count);
           if(status != FBE_STATUS_OK)
            {
                fbe_base_object_trace((fbe_base_object_t*) raid_group_p, FBE_TRACE_LEVEL_INFO, 
                    FBE_TRACE_MESSAGE_ID_INFO, "Encryption io get_unconsumed blks failed, start lba 0x%llx blks 0x%llx\n",
                    (unsigned long long)encryption_context_p->start_lba, (unsigned long long)encryption_context_p->block_count);

                fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
                fbe_transport_complete_packet(packet_p);
                return FBE_STATUS_OK;
            }

            if (io_block_count == 0) {
                /* The edge may have been attached after the event was returned with no user data.  
                 * The result is an io_block_count of zero.  Let the monitor run again to retry.
                 */
                fbe_base_object_trace((fbe_base_object_t*) raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO, 
                                      "Encryption no user data io block count: 0x%llx, start lba 0x%llx blks 0x%llx ublks: 0x%llx\n",
                                      (unsigned long long)io_block_count, (unsigned long long)encryption_context_p->start_lba, 
                                      (unsigned long long)encryption_context_p->block_count, (unsigned long long)unconsumed_block_count);

                fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
                fbe_transport_complete_packet(packet_p);
                return FBE_STATUS_MORE_PROCESSING_REQUIRED; 
            }
           
           fbe_raid_group_trace(raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_RAID_GROUP_DEBUG_FLAG_ENCRYPTION_TRACING,
                                "Encryption No user data: lba: 0x%llx  new blocks: 0x%llx unconsumed: 0x%llx\n",
                                (unsigned long long)encryption_context_p->start_lba, 
                                (unsigned long long)io_block_count, 
                                (unsigned long long)unconsumed_block_count);

            /* set the updated block count to advance rekey checkpoint */
            encryption_context_p->block_count = io_block_count;
            
            //  If the area is not consumed by an upstream object, then we do not need to rekey it.  Instead, clear the NR 
            //  bits in the paged metadata and advance the checkpoint.
            status = fbe_raid_group_encryption_write_zeros(raid_group_p, packet_p);
            return FBE_STATUS_MORE_PROCESSING_REQUIRED;

        default:
            fbe_base_object_trace((fbe_base_object_t*) raid_group_p, 
                                  FBE_TRACE_LEVEL_ERROR, 
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                                  "Encryption %s unexpected event status: %d\n", __FUNCTION__, event_status);
            fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
            fbe_transport_complete_packet(packet_p);
            return FBE_STATUS_OK;

    } /* end if switch on event_status*/

    // Send the rekey I/O .  Its completion function will execute when it finishes. 
    status = fbe_raid_group_encryption_send_io(raid_group_p, packet_p,
                                                  FBE_TRUE, /* We must break the context to release the event thread. */ 
                                                  encryption_context_p->start_lba, 
                                                  encryption_context_p->block_count);
    return status;
}
/**************************************
 * end fbe_raid_group_encryption_request_permission_completion()
 **************************************/

/*!****************************************************************************
 * fbe_raid_group_encryption_write_zeros()
 ******************************************************************************
 * @brief
 *  This function writes zeros for region that is not bound.
 *
 * @param raid_group_p           - pointer to a raid group object
 * @param packet_p               - pointer to a control packet from the scheduler
 *
 * @return fbe_status_t
 *
 * @author
 *  1/14/2014 - Created. Rob Foley
 *
 *****************************************************************************/
fbe_status_t fbe_raid_group_encryption_write_zeros(fbe_raid_group_t *raid_group_p, 
                                                   fbe_packet_t *packet_p)
{
    fbe_raid_group_encryption_context_t *encryption_context_p = NULL;
    fbe_lba_t                           lba;
    fbe_block_count_t                   blocks;
    fbe_raid_geometry_t                 *raid_geometry_p = NULL;
    fbe_u16_t                           disk_count;
    fbe_raid_group_monitor_packet_params_t params;

    raid_geometry_p = fbe_raid_group_get_raid_geometry(raid_group_p);
    fbe_raid_group_get_encryption_context(raid_group_p, packet_p, &encryption_context_p);

    fbe_raid_geometry_get_data_disks(raid_geometry_p, &disk_count);
    /* We need to limit how much we operate on at a time.
     */
    encryption_context_p->block_count = FBE_RAID_DEFAULT_CHUNK_SIZE;
    lba = encryption_context_p->start_lba * disk_count;
    blocks = encryption_context_p->block_count * disk_count;

    fbe_raid_group_trace(raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_RAID_GROUP_DEBUG_FLAG_ENCRYPTION_TRACING,
                         "Encryption unconsumed zero for pkt: 0x%x lba %llx blks: %llx\n", 
                         (fbe_u32_t)packet_p, (unsigned long long)lba,  (unsigned long long)blocks);

    /* We must do a forced write since this area may be unconsumed. 
     * Do not lock on unconsumed areas.  No lock is required.
     */
    fbe_raid_group_setup_monitor_params(&params, 
                                        FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ENCRYPTION_REKEY_WRITE_ZEROS, 
                                        lba, 
                                        blocks, 
                                        (FBE_PAYLOAD_BLOCK_OPERATION_FLAGS_METADATA_OP | 
                                         FBE_PAYLOAD_BLOCK_OPERATION_FLAGS_NO_SL_THIS_LEVEL |
                                         FBE_PAYLOAD_BLOCK_OPERATION_FLAGS_FORCED_WRITE));

    fbe_raid_group_setup_monitor_packet(raid_group_p, packet_p, &params);

    fbe_transport_set_completion_function(packet_p, 
                                          fbe_raid_group_encryption_write_zeros_completion, 
                                          (fbe_packet_completion_context_t) raid_group_p);
    /* There are some cases where we are using the monitor context.
     * In those cases it is ok to send the I/O immediately.
     */
    fbe_base_config_bouncer_entry((fbe_base_config_t *)raid_group_p, packet_p);
    //  Return more processing since we have a packet outstanding 
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
} // End fbe_raid_group_encryption_write_zeros()

/*!****************************************************************************
 * fbe_raid_group_encryption_write_zeros_completion()
 ******************************************************************************
 * @brief
 *   This is the completion function for zeroing unbound area.
 * 
 * @param packet_p   - pointer to a control packet from the scheduler
 * @param context    - completion context, which is a pointer to the raid 
 *                        group object
 * 
 * @return  fbe_status_t  
 *
 * @author
 *  1/14/2014 - Created. Rob Foley
 *  
 ******************************************************************************/
static fbe_status_t 
fbe_raid_group_encryption_write_zeros_completion(fbe_packet_t *packet_p,
                                                 fbe_packet_completion_context_t context)

{
    fbe_raid_group_t*                       raid_group_p = (fbe_raid_group_t*)context;
    fbe_status_t                            packet_status;
    fbe_payload_ex_t*                       payload_p = NULL;
    fbe_payload_block_operation_t*          block_operation_p = NULL;
    fbe_payload_block_operation_status_t    block_status;
    fbe_payload_block_operation_qualifier_t block_qualifier;

    packet_status = fbe_transport_get_status_code(packet_p);
    payload_p         = fbe_transport_get_payload_ex(packet_p);
    block_operation_p = fbe_payload_ex_get_block_operation(payload_p);

    block_status = FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID; 
    block_qualifier = FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_INVALID; 
    if (block_operation_p != NULL) {
        fbe_payload_block_get_status(block_operation_p, &block_status);
        fbe_payload_block_get_qualifier(block_operation_p, &block_qualifier);
    }
    fbe_payload_ex_release_block_operation(payload_p, block_operation_p);

    //  Check for any error and trace if one occurred, but continue processing as usual
    if ((packet_status != FBE_STATUS_OK) || (block_status != FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS))
    {
        fbe_base_object_trace((fbe_base_object_t*) raid_group_p, 
            FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
            "Encryption zero unbound failed, status: 0x%x/0x%x q: 0x%x \n", 
                              packet_status, block_status, block_qualifier);

        if (block_status != FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS) {
            fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, block_status);
        }
    }
    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_raid_group_encryption_write_zeros_completion()
 **************************************/
/*!****************************************************************************
 * fbe_raid_group_clear_encryption()
 ******************************************************************************
 * @brief
 *  This function clears encryption bits for region that is not bound.
 *
 * @param raid_group_p           - pointer to a raid group object
 * @param packet_p               - pointer to a control packet from the scheduler
 *
 * @return fbe_status_t
 *
 * @author
 *  10/15/2013 - Created. Rob Foley
 *
 *****************************************************************************/
fbe_status_t fbe_raid_group_clear_encryption(fbe_raid_group_t *raid_group_p, 
                                             fbe_packet_t *packet_p)
{
    fbe_raid_group_encryption_context_t *encryption_context_p = NULL;
    fbe_lba_t                           end_lba;
    fbe_chunk_index_t                   start_chunk;
    fbe_chunk_index_t                   end_chunk;
    fbe_chunk_count_t                   chunk_count;
    fbe_raid_group_paged_metadata_t     chunk_data;
    fbe_raid_geometry_t                 *raid_geometry_p = NULL;
    fbe_u16_t                           disk_count;
    fbe_lba_t                           rg_lba;
    fbe_u8_t rekeyed_bit;

    raid_geometry_p = fbe_raid_group_get_raid_geometry(raid_group_p);
    fbe_raid_group_get_encryption_context(raid_group_p, packet_p, &encryption_context_p);

    fbe_transport_set_completion_function(packet_p, 
                                          fbe_raid_group_clear_encryption_unbound_completion, 
                                          (fbe_packet_completion_context_t) raid_group_p);

    if (fbe_raid_geometry_is_raid10(raid_geometry_p)) {
        /*! RAID-10 first needs to allow the mirrors to advance the checkpoint, 
         * and then it can advance the checkpoint. 
         * The common completion function will check status and then advance the checkpoint.
         */
        fbe_raid_group_rekey_set_chkpt_ds(raid_group_p, packet_p, 
                                          encryption_context_p->start_lba, 
                                          encryption_context_p->block_count);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }

    end_lba = encryption_context_p->start_lba + encryption_context_p->block_count - 1;

    //  Convert the starting and ending LBAs to chunk indexes 
    fbe_raid_group_bitmap_get_chunk_index_for_lba(raid_group_p, encryption_context_p->start_lba, &start_chunk);
    fbe_raid_group_bitmap_get_chunk_index_for_lba(raid_group_p, end_lba, &end_chunk);

    //  Calculate the number of chunks to be marked 
    chunk_count = (fbe_chunk_count_t) (end_chunk - start_chunk + 1);

    fbe_raid_group_trace(raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_RAID_GROUP_DEBUG_FLAG_ENCRYPTION_TRACING,
                         "Encryption clr for pk: 0x%x lba %llx to %llx chunk %llx to %llx\n", 
                         (fbe_u32_t)packet_p, (unsigned long long)encryption_context_p->start_lba, 
                         (unsigned long long)end_lba, (unsigned long long)start_chunk, (unsigned long long)end_chunk);

    fbe_set_memory(&chunk_data, 0, sizeof(fbe_raid_group_paged_metadata_t));

    /* We only use the set bit to indicate we already encrypted the data.
     */
    rekeyed_bit = 1;
    chunk_data.rekey = rekeyed_bit;

    fbe_raid_geometry_get_data_disks(raid_geometry_p, &disk_count);

    rg_lba = encryption_context_p->start_lba * disk_count;

    //  If the chunk is not in the data area, then use the non-paged metadata service to update it
    if (fbe_raid_geometry_is_metadata_io(raid_geometry_p, rg_lba)) {
        fbe_raid_group_bitmap_write_chunk_info_using_nonpaged(raid_group_p, packet_p, start_chunk,
                                                              chunk_count, &chunk_data, TRUE);
    }
    else {
        //  The chunks are in the user data area.  Use the paged metadata service to update them. 
        fbe_raid_group_bitmap_update_paged_metadata(raid_group_p, packet_p, start_chunk,
                                                    chunk_count, &chunk_data, FBE_TRUE);
    }
    //  Return more processing since we have a packet outstanding 
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
} // End fbe_raid_group_clear_encryption()

/*!****************************************************************************
 * fbe_raid_group_clear_encryption_unbound_completion()
 ******************************************************************************
 * @brief
 *   This is the completion function for clearing encryption.
 * 
 * @param packet_p   - pointer to a control packet from the scheduler
 * @param context    - completion context, which is a pointer to the raid 
 *                        group object
 * 
 * @return  fbe_status_t  
 *
 * @author
 *  10/15/2013 - Created. Rob Foley
 *  
 ******************************************************************************/
static fbe_status_t 
fbe_raid_group_clear_encryption_unbound_completion(fbe_packet_t *packet_p,
                                                   fbe_packet_completion_context_t context)

{
    fbe_raid_group_t*               raid_group_p = NULL;
    fbe_status_t                    status;
    fbe_status_t                            packet_status;
    fbe_payload_ex_t*                   sep_payload_p = NULL;
    fbe_payload_control_operation_t     *control_operation_p = NULL;
    fbe_payload_control_status_t        control_status;
    

    //  Get the raid group pointer from the input context 
    raid_group_p = (fbe_raid_group_t*) context; 

    sep_payload_p           = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_present_control_operation(sep_payload_p);

    packet_status = fbe_transport_get_status_code(packet_p);
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
    status = fbe_raid_group_clr_encryption_unbound_update_np(raid_group_p, packet_p);

    //  Return more processing since we have a packet outstanding 
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/**************************************
 * end fbe_raid_group_clear_encryption_unbound_completion()
 **************************************/


/*!****************************************************************************
 * fbe_raid_group_clr_encryption_unbound_update_np()
 ******************************************************************************
 * @brief
 *   This function will update the nonpaged metadata for an unbound area.  It
 *   will advance the encryption checkpoint. 
 *
 * @param raid_group_p           - pointer to a raid group object
 * @param packet_p               - pointer to a control packet from the scheduler
 *
 * @return fbe_status_t
 *
 * @author
 *  10/15/2013 - Created. Rob Foley
 *
 *****************************************************************************/
static fbe_status_t fbe_raid_group_clr_encryption_unbound_update_np(fbe_raid_group_t *raid_group_p, 
                                                                    fbe_packet_t *packet_p)
{
    fbe_status_t status;
    fbe_raid_group_encryption_context_t *encryption_context_p = NULL;

    fbe_raid_group_get_encryption_context(raid_group_p, packet_p, &encryption_context_p);

    /*! @todo should we log too?
     */
    status = fbe_raid_group_incr_rekey_checkpoint(raid_group_p, 
                                                  packet_p, 
                                                  encryption_context_p->start_lba, 
                                                  encryption_context_p->block_count);
    return status;
}
/**************************************
 * end fbe_raid_group_clr_encryption_unbound_update_np()
 **************************************/
/*!****************************************************************************
 * fbe_raid_group_incr_rekey_checkpoint()
 ******************************************************************************
 * @brief
 *   Increment encryption checkpoint.
 *
 * @param raid_group_p           - pointer to a raid group object
 * @param packet_p               - pointer to a control packet from the scheduler
 * @param checkpoint             - new checkpoint
 * 
 * @return  fbe_status_t  
 *
 * @author
 *  10/15/2013 - Created. Rob Foley
 *
 ******************************************************************************/
static fbe_status_t fbe_raid_group_incr_rekey_checkpoint(fbe_raid_group_t *raid_group_p,
                                                         fbe_packet_t *packet_p,
                                                         fbe_lba_t start_lba,
                                                         fbe_block_count_t block_count) 
{
    fbe_status_t    status;
    fbe_u64_t       metadata_offset;
    fbe_u32_t       ms_since_checkpoint;
    fbe_time_t      last_checkpoint_time;
    fbe_u32_t       peer_update_period_ms;
    fbe_lba_t       exported_per_disk_capacity = 0;

    metadata_offset = (fbe_u64_t) (&((fbe_raid_group_nonpaged_metadata_t*)0)->encryption.rekey_checkpoint);

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
                                                                        0,
                                                                        start_lba + block_count);
        fbe_raid_group_update_last_checkpoint_time(raid_group_p);
        fbe_raid_group_update_last_checkpoint_to_peer(raid_group_p, start_lba + block_count);
    }
    else
    {
        /* Increment checkpoint locally without sending to peer in order to not 
         * thrash the CMI. 
         */
        status = fbe_base_config_metadata_nonpaged_incr_checkpoint_no_peer((fbe_base_config_t *)raid_group_p,
                                                                           packet_p,
                                                                           metadata_offset,
                                                                           0,
                                                                           start_lba,
                                                                           block_count);
    }
        
    //  Check the status of the call and trace if error.  The called function is completing the packet on error
    //  so we can't complete it here.  The condition will remain set so we will try again.
    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*) raid_group_p, FBE_TRACE_LEVEL_WARNING, 
                              FBE_TRACE_MESSAGE_ID_INFO, "Encryption error %d on nonpaged call\n", status);
    }

    //  Return more processing since a packet is outstanding 
    return FBE_STATUS_MORE_PROCESSING_REQUIRED; 

}
/**************************************
 * end fbe_raid_group_incr_rekey_checkpoint
 **************************************/
/*!****************************************************************************
 * fbe_raid_group_encryption_send_io()
 ******************************************************************************
 * @brief
 *  This function will issue a rekey I/O request.  When it finishes, its 
 *  completion function will be invoked. 
 *
 * @param raid_group_p           - pointer to a raid group object
 * @param packet_p               - pointer to a control packet from the scheduler
 * @param b_queue_to_block_transport - FBE_TRUE must break context
 * @param start_lba              - starting LBA of the I/O. The LBA is relative 
 *                                    to the RAID extent on a single disk. 
 * @param block_count            - number of blocks to rekey 
 *
 * @return fbe_status_t
 *
 * @author
 *  05/13/2010 - Created. Jean Montes.
 *
 *****************************************************************************/
static fbe_status_t fbe_raid_group_encryption_send_io(fbe_raid_group_t*           raid_group_p, 
                                                      fbe_packet_t*               packet_p,
                                                      fbe_bool_t b_queue_to_block_transport,
                                                      fbe_lba_t                   start_lba,
                                                      fbe_block_count_t           block_count)
{

    fbe_traffic_priority_t              io_priority;
    fbe_status_t                        status;
    fbe_raid_group_encryption_context_t*   encryption_context_p = NULL;

    fbe_raid_group_get_encryption_context(raid_group_p, packet_p, &encryption_context_p);

    //fbe_raid_group_rebuild_context_set_rebuild_state(encryption_context_p, FBE_RAID_GROUP_REBUILD_STATE_REBUILD_IO);

    fbe_transport_set_completion_function(packet_p, fbe_raid_group_encryption_send_io_completion, 
            (fbe_packet_completion_context_t) raid_group_p);

    io_priority = fbe_raid_group_get_rebuild_priority(raid_group_p);
    fbe_transport_set_traffic_priority(packet_p, io_priority);

    /* If this method is invoked from the event thread we must break the
     * context (to free the event thread).
     */
    if (b_queue_to_block_transport == FBE_TRUE) {
        /* Break the context before starting the I/O.
         */
        status = fbe_raid_group_executor_break_context_send_monitor_packet(raid_group_p, packet_p,
                                                         FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ENCRYPTION_READ_PAGED, start_lba, block_count);
    }
    else {
        /* Else we can start the I/O immediately.
         */
        status = fbe_raid_group_executor_send_monitor_packet(raid_group_p, packet_p,
                                                         FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ENCRYPTION_READ_PAGED, start_lba, block_count);
    }
    return status;
}
/**************************************
 * end fbe_raid_group_encryption_send_io()
 **************************************/
/*!**************************************************************
 * fbe_raid_group_encryption_unpin_complete()
 ****************************************************************
 * @brief
 *   This completion function handles a completion of the read/persist.
 *
 * @param packet_p - Current usurper packet.
 * @param context - Raid group, not used.
 * 
 * @return fbe_status_t
 *
 * @author
 *  12/2/2013 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_raid_group_encryption_unpin_complete(fbe_packet_t * packet_p, fbe_packet_completion_context_t context)
{
    fbe_status_t status;
    fbe_raid_group_t * raid_group_p = (fbe_raid_group_t*)context;
    fbe_payload_ex_t*                       payload_p = NULL;
    fbe_payload_persistent_memory_operation_t*     persistent_operation_p = NULL;

    fbe_raid_group_bg_op_clear_flag(raid_group_p, FBE_RAID_GROUP_BG_OP_FLAG_UNPIN_OUTSTANDING);

    payload_p         = fbe_transport_get_payload_ex(packet_p);
    persistent_operation_p = fbe_payload_ex_get_persistent_memory_operation(payload_p);

    status = fbe_transport_get_status_code(packet_p);
    fbe_payload_ex_release_persistent_memory_operation(payload_p, persistent_operation_p);

    if ((status != FBE_STATUS_OK) || 
        (persistent_operation_p->persistent_memory_status != FBE_PAYLOAD_PERSISTENT_MEMORY_STATUS_OK)){
        /* We failed, return the packet with this bad status.
         * Also take the raid group out of ready since the paged is inconsistent. 
         */
        fbe_base_object_trace( (fbe_base_object_t *)raid_group_p,
                                FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "Encryption unpin failed. status: 0x%x pstatus: 0x%x\n", status, 
                                persistent_operation_p->persistent_memory_status);
        fbe_transport_set_status(packet_p, FBE_STATUS_FAILED, 0);
        return FBE_STATUS_OK;
    }
    fbe_raid_group_trace(raid_group_p,
                         FBE_TRACE_LEVEL_INFO,
                         FBE_RAID_GROUP_DEBUG_FLAG_ENCRYPTION_TRACING,
                          "Encryption unpin successful. lba/bl: 0x%llx/0x%llx\n",
                          persistent_operation_p->lba, persistent_operation_p->blocks);
    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_raid_group_encryption_unpin_complete
 **************************************/
/*!**************************************************************
 * fbe_raid_group_encryption_write_complete()
 ****************************************************************
 * @brief
 *   This completion function handles a completion of the encryption write.
 *
 * @param packet_p - Current usurper packet.
 * @param context - Raid group, not used.
 * 
 * @return fbe_status_t
 *
 * @author
 *  12/2/2013 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_raid_group_encryption_write_complete(fbe_packet_t * packet_p, fbe_packet_completion_context_t context)
{
    fbe_raid_group_t*                       raid_group_p = (fbe_raid_group_t*)context;
    fbe_status_t                            packet_status;
    fbe_payload_ex_t*                       payload_p = NULL;
    fbe_payload_block_operation_t*          block_operation_p = NULL;
    fbe_payload_control_operation_t*        control_operation_p = NULL;
    fbe_payload_persistent_memory_operation_t*     persistent_operation_p = NULL;
    fbe_payload_block_operation_status_t    block_status;
    fbe_payload_block_operation_qualifier_t block_qualifier;
    fbe_raid_group_encryption_context_t   *encryption_context_p = NULL;
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_group_get_raid_geometry(raid_group_p);
    fbe_u16_t data_disks;
    fbe_payload_persistent_memory_unpin_mode_t mode;
    fbe_scheduler_hook_status_t         hook_status = FBE_SCHED_STATUS_OK;

    packet_status = fbe_transport_get_status_code(packet_p);
    payload_p         = fbe_transport_get_payload_ex(packet_p);
    block_operation_p = fbe_payload_ex_get_block_operation(payload_p);

    block_status = FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID; 
    block_qualifier = FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_INVALID;
    if (block_operation_p != NULL) {
        fbe_payload_block_get_status(block_operation_p, &block_status);
        fbe_payload_block_get_qualifier(block_operation_p, &block_qualifier);
    }

    fbe_payload_ex_release_block_operation(payload_p, block_operation_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);
    fbe_payload_control_get_buffer(control_operation_p, &encryption_context_p);

    if ((packet_status != FBE_STATUS_OK) || (block_status != FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS)) {
        /* On any error unpin and cause a verify/write to occur to fix any errors.
         */
        fbe_base_object_trace((fbe_base_object_t*) raid_group_p, 
                              FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                              "Encryption write failed, status: 0x%x/0x%x/0x%x \n", 
                              packet_status, block_status, block_qualifier);
        mode = FBE_PAYLOAD_PERSISTENT_MEMORY_UNPIN_MODE_VERIFY_REQUIRED;
        encryption_context_p->encryption_status = FBE_STATUS_GENERIC_FAILURE;
    } else if (encryption_context_p->b_was_dirty) {
        /* Pages need to be flushed by cache since they are dirty.
         */
        mode = FBE_PAYLOAD_PERSISTENT_MEMORY_UNPIN_MODE_FLUSH_REQUIRED;

        fbe_raid_group_check_hook(raid_group_p,
                                  SCHEDULER_MONITOR_STATE_RAID_GROUP_ENCRYPTION, 
                                  FBE_RAID_GROUP_SUBSTATE_ENCRYPTION_UNPIN_DIRTY, 0,  &hook_status);
    } else {
        mode = FBE_PAYLOAD_PERSISTENT_MEMORY_UNPIN_MODE_DATA_WRITTEN;
    }

    // Remove once cache changes are ready
#if 0
    if (encryption_context_p->b_beyond_capacity) {
        fbe_sg_element_t *sg_p = NULL;
        fbe_payload_ex_get_sg_list(payload_p, &sg_p, 0);
        fbe_memory_native_release(sg_p);
        fbe_payload_ex_set_sg_list(payload_p, NULL, 0);
        /* No need to unpin, we did not read and persist. Instead we did a manual read/write.
         */
        return FBE_STATUS_OK;
    }
#endif
    persistent_operation_p = fbe_payload_ex_allocate_persistent_memory_operation(payload_p);

    fbe_raid_geometry_get_data_disks(raid_geometry_p, &data_disks);
    fbe_payload_persistent_memory_build_unpin(persistent_operation_p, 
                                              encryption_context_p->top_lba,
                                              encryption_context_p->block_count * data_disks,
                                              encryption_context_p->object_index,
                                              encryption_context_p->lun_object_id,
                                              encryption_context_p->opaque,
                                              mode,
                                              encryption_context_p->chunk_p);
    fbe_payload_ex_increment_persistent_memory_operation_level(payload_p);
    
    fbe_raid_group_bg_op_set_pin_time(raid_group_p, fbe_get_time());
    fbe_raid_group_bg_op_set_flag(raid_group_p, FBE_RAID_GROUP_BG_OP_FLAG_UNPIN_OUTSTANDING);

    fbe_transport_set_completion_function(packet_p, fbe_raid_group_encryption_unpin_complete, raid_group_p);
    /* Continue rekey with the persist operation.
     */
    fbe_persistent_memory_operation_entry(packet_p);
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/**************************************
 * end fbe_raid_group_encryption_write_complete
 **************************************/
/*!**************************************************************
 * fbe_raid_group_encryption_pin_complete()
 ****************************************************************
 * @brief
 *   This completion function handles a completion of the read and pin.
 *
 * @param packet_p - Current usurper packet.
 * @param context - Raid group, not used.
 * 
 * @return fbe_status_t
 *
 * @author
 *  11/27/2013 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_raid_group_encryption_pin_complete(fbe_packet_t * packet_p, fbe_packet_completion_context_t context)
{
    fbe_scheduler_hook_status_t         hook_status = FBE_SCHED_STATUS_OK;
    fbe_status_t                        status;
    fbe_raid_group_t                   *raid_group_p = (fbe_raid_group_t*)context;
    fbe_payload_ex_t                   *payload_p = NULL;
    fbe_payload_persistent_memory_operation_t*     persistent_operation_p = NULL;
    fbe_payload_control_operation_t*        control_operation_p = NULL;
    fbe_raid_group_encryption_context_t   *encryption_context_p = NULL;
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_group_get_raid_geometry(raid_group_p);
    fbe_u16_t data_disks;

    fbe_raid_group_bg_op_clear_flag(raid_group_p, FBE_RAID_GROUP_BG_OP_FLAG_PIN_OUTSTANDING);

    payload_p         = fbe_transport_get_payload_ex(packet_p);
    persistent_operation_p = fbe_payload_ex_get_persistent_memory_operation(payload_p);

    status = fbe_transport_get_status_code(packet_p);
    fbe_payload_ex_release_persistent_memory_operation(payload_p, persistent_operation_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);
    fbe_payload_control_get_buffer(control_operation_p, &encryption_context_p);

    if ((status != FBE_STATUS_OK) || 
        ((persistent_operation_p->persistent_memory_status != FBE_PAYLOAD_PERSISTENT_MEMORY_STATUS_OK) &&
         (persistent_operation_p->persistent_memory_status != FBE_PAYLOAD_PERSISTENT_MEMORY_STATUS_PINNED_NOT_PERSISTENT))){
        /* We failed, return the packet with this bad status.
         * Also take the raid group out of ready since the paged is inconsistent. 
         */
        fbe_base_object_trace( (fbe_base_object_t *)raid_group_p,
                                FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "Encryption read/pin failed. status: 0x%x pstatus: 0x%x\n", status, 
                                persistent_operation_p->persistent_memory_status);
        fbe_transport_set_status(packet_p, FBE_STATUS_FAILED, 0);
        return FBE_STATUS_OK;
    }
    else if (persistent_operation_p->persistent_memory_status == FBE_PAYLOAD_PERSISTENT_MEMORY_STATUS_PINNED_NOT_PERSISTENT) {
        /* We pinned it clean, so we need to unpin and fail the request. 
         * Cache is disabled and we will not allow rekey to proceed. 
         */
        fbe_raid_group_trace(raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_RAID_GROUP_DEBUG_FLAG_ENCRYPTION_TRACING,
                             "Encryption read/pin clean. status: 0x%x pstatus: 0x%x\n", status, 
                             persistent_operation_p->persistent_memory_status);
        fbe_raid_group_check_hook(raid_group_p,
                                  SCHEDULER_MONITOR_STATE_RAID_GROUP_ENCRYPTION, 
                                  FBE_RAID_GROUP_SUBSTATE_ENCRYPTION_PIN_CLEAN, 0,  &hook_status);

        persistent_operation_p = fbe_payload_ex_allocate_persistent_memory_operation(payload_p);

        fbe_raid_geometry_get_data_disks(raid_geometry_p, &data_disks);
        fbe_payload_persistent_memory_build_unpin(persistent_operation_p, 
                                                  encryption_context_p->top_lba,
                                                  encryption_context_p->block_count * data_disks,
                                                  encryption_context_p->object_index,
                                                  encryption_context_p->lun_object_id,
                                                  persistent_operation_p->opaque,
                                                  FBE_PAYLOAD_PERSISTENT_MEMORY_UNPIN_MODE_FLUSH_REQUIRED,
                                                  encryption_context_p->chunk_p);
        fbe_payload_ex_increment_persistent_memory_operation_level(payload_p);
        
        encryption_context_p->encryption_status = FBE_STATUS_GENERIC_FAILURE;

        fbe_transport_set_completion_function(packet_p, fbe_raid_group_encryption_unpin_complete, raid_group_p);
        /* Continue rekey with the persist operation.
         */
        fbe_persistent_memory_operation_entry(packet_p);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);
    fbe_payload_control_get_buffer(control_operation_p, &encryption_context_p);
    fbe_raid_geometry_get_data_disks(raid_geometry_p, &data_disks);

    fbe_raid_group_trace(raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_RAID_GROUP_DEBUG_FLAG_ENCRYPTION_TRACING,
                         "Encryption pin successful. lba: 0x%llx bl: 0x%llx Start write. \n",
                         encryption_context_p->start_lba * data_disks,
                         encryption_context_p->block_count * data_disks);

    /* Save the context from the read and pin.  We will need this for the write.
     */
    encryption_context_p->b_was_dirty = persistent_operation_p->b_was_dirty;
    encryption_context_p->opaque = persistent_operation_p->opaque;

    fbe_payload_ex_set_sg_list(payload_p, persistent_operation_p->sg_p, 0);

    /* Continue with the write of data.  This is a special write where we instruct to use the new key.
     */
    fbe_transport_set_completion_function(packet_p, fbe_raid_group_encryption_write_complete, raid_group_p);
    fbe_raid_group_executor_break_context_send_monitor_packet(raid_group_p, packet_p,
                                                              FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ENCRYPTION_REKEY_WRITE, 
                                                              encryption_context_p->start_lba * data_disks,
                                                              encryption_context_p->block_count * data_disks);
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/**************************************
 * end fbe_raid_group_encryption_pin_complete
 **************************************/

static fbe_status_t fbe_raid_group_encryption_read_manual_completion(
                                            fbe_packet_t*                   packet_p, 
                                            fbe_packet_completion_context_t context)
{
    fbe_raid_group_t*                       raid_group_p = (fbe_raid_group_t*)context;
    fbe_status_t                            packet_status;
    fbe_payload_ex_t*                       payload_p = NULL;
    fbe_payload_block_operation_t*          block_operation_p = NULL;
    fbe_payload_control_operation_t*        control_operation_p = NULL;
    fbe_payload_block_operation_status_t    block_status;
    fbe_payload_block_operation_qualifier_t block_qualifier;
    fbe_raid_group_encryption_context_t   *encryption_context_p = NULL;
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_group_get_raid_geometry(raid_group_p);
    fbe_u16_t data_disks;

    packet_status = fbe_transport_get_status_code(packet_p);
    payload_p         = fbe_transport_get_payload_ex(packet_p);
    block_operation_p = fbe_payload_ex_get_block_operation(payload_p);

    block_status = FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID; 
    if (block_operation_p != NULL) {
        fbe_payload_block_get_status(block_operation_p, &block_status);
        fbe_payload_block_get_qualifier(block_operation_p, &block_qualifier);
    }

    //  Check for any error and trace if one occurred, but continue processing as usual
    if ((packet_status != FBE_STATUS_OK) || (block_status != FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS))
    {
        fbe_base_object_trace((fbe_base_object_t*) raid_group_p, 
            FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
            "Encryption read manual failed, status: 0x%x/0x%x \n", packet_status, block_status);

        if (block_status != FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS) {
            fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, block_status);
        }
        fbe_payload_ex_release_block_operation(payload_p, block_operation_p);
    } else {
        fbe_payload_ex_release_block_operation(payload_p, block_operation_p);

        control_operation_p = fbe_payload_ex_get_control_operation(payload_p);
        fbe_payload_control_get_buffer(control_operation_p, &encryption_context_p);
        fbe_raid_geometry_get_data_disks(raid_geometry_p, &data_disks);

        fbe_transport_set_completion_function(packet_p, fbe_raid_group_encryption_write_complete, raid_group_p);
        fbe_raid_group_executor_break_context_send_monitor_packet(raid_group_p, packet_p,
                                                                  FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ENCRYPTION_REKEY_WRITE, 
                                                                  encryption_context_p->start_lba * data_disks,
                                                                  encryption_context_p->block_count * data_disks);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }
    return packet_status;
}
/**************************************
 * end fbe_raid_group_encryption_read_manual_completion()
 **************************************/
fbe_status_t fbe_raid_group_rekey_read_manual(fbe_raid_group_t *raid_group_p, fbe_packet_t *packet_p)
{
    fbe_status_t                            packet_status;
    fbe_payload_ex_t*                       payload_p = NULL;
    fbe_payload_control_operation_t*        control_operation_p = NULL;
    fbe_raid_group_encryption_context_t   *encryption_context_p = NULL;
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_group_get_raid_geometry(raid_group_p);
    fbe_u16_t data_disks;
    fbe_u32_t bytes;
    void *memory_p = NULL;
    fbe_sg_element_t *sg_p = NULL;
    packet_status = fbe_transport_get_status_code(packet_p);
    payload_p     = fbe_transport_get_payload_ex(packet_p);

    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);
    fbe_payload_control_get_buffer(control_operation_p, &encryption_context_p);

    fbe_raid_geometry_get_data_disks(raid_geometry_p, &data_disks);
    bytes = (fbe_u32_t)(FBE_BE_BYTES_PER_BLOCK * encryption_context_p->block_count * data_disks);
    bytes += sizeof(fbe_sg_element_t) * 2;
    memory_p = fbe_memory_native_allocate(bytes);
    if (memory_p == NULL) {
        return FBE_STATUS_GENERIC_FAILURE;
    }
    sg_p = memory_p;
    fbe_sg_element_init(&sg_p[0],
                        (fbe_u32_t)(FBE_BE_BYTES_PER_BLOCK * encryption_context_p->block_count * data_disks),
                        ((fbe_u8_t *)memory_p) + sizeof(fbe_sg_element_t) * 2);
    fbe_sg_element_terminate(&sg_p[1]);
    fbe_payload_ex_set_sg_list(payload_p, sg_p, 0);

    fbe_transport_set_completion_function(packet_p, fbe_raid_group_encryption_read_manual_completion, raid_group_p);
    fbe_raid_group_executor_break_context_send_monitor_packet(raid_group_p, packet_p,
                                                              FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ, 
                                                              encryption_context_p->start_lba * data_disks,
                                                              encryption_context_p->block_count * data_disks);
    return FBE_STATUS_OK;
}
/*!****************************************************************************
 * fbe_raid_group_encryption_send_io_completion()
 ******************************************************************************
 * @brief
 *  This is the completion function for a rekey I/O operation.  It is called
 *  by the executor after it finishes a rekey I/O.
 *
 * @param packet_p           - pointer to a control packet from the scheduler
 * @param context            - context, which is a pointer to the raid group
 *                                object
 *
 * @return fbe_status_t
 *
 * Note: This function must always return FBE_STATUS_OK so that the packet gets
 * completed to the next level.  (If any other status is returned, the packet  
 * will get stuck.)
 * 
 * @author
 *  10/15/2013 - Created. Rob Foley
 *
 *****************************************************************************/
static fbe_status_t fbe_raid_group_encryption_send_io_completion(
                                            fbe_packet_t*                   packet_p, 
                                            fbe_packet_completion_context_t context)
{
    fbe_raid_group_t*                       raid_group_p = (fbe_raid_group_t*)context;
    fbe_status_t                            packet_status;
    fbe_payload_ex_t*                       payload_p = NULL;
    fbe_payload_block_operation_t*          block_operation_p = NULL;
    fbe_payload_control_operation_t*        control_operation_p = NULL;
    fbe_payload_persistent_memory_operation_t*     persistent_operation_p = NULL;
    fbe_payload_block_operation_status_t    block_status;
    fbe_payload_block_operation_qualifier_t block_qualifier;
    fbe_raid_group_encryption_context_t   *encryption_context_p = NULL;
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_group_get_raid_geometry(raid_group_p);
    fbe_u16_t data_disks;

    packet_status = fbe_transport_get_status_code(packet_p);
    payload_p         = fbe_transport_get_payload_ex(packet_p);
    block_operation_p = fbe_payload_ex_get_block_operation(payload_p);

    block_status = FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID; 
    block_qualifier = FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_INVALID;
    if (block_operation_p != NULL) {
        fbe_payload_block_get_status(block_operation_p, &block_status);
        fbe_payload_block_get_qualifier(block_operation_p, &block_qualifier);
    }

    //  Check for any error and trace if one occurred, but continue processing as usual
    if ((packet_status != FBE_STATUS_OK) || (block_status != FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS))
    {
        fbe_base_object_trace((fbe_base_object_t*) raid_group_p, 
                              FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                              "Encryption read paged failed, status: 0x%x/0x%x/0x%x \n", 
                              packet_status, block_status, block_qualifier);

        if (block_status != FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS) {
            fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, block_status);
        }
    } else if (block_qualifier == FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_CONTINUE_REKEY){
        /* The read for rekey saw that there is work to do for this chunk, so continue.
         */
        fbe_payload_ex_release_block_operation(payload_p, block_operation_p);
        control_operation_p = fbe_payload_ex_get_control_operation(payload_p);
        fbe_payload_control_get_buffer(control_operation_p, &encryption_context_p);

// Remove once cache changes are ready
#if 0
        if (encryption_context_p->b_beyond_capacity) {
            /* For now we simply do a raw read of the special areas.
             */
            fbe_raid_group_rekey_read_manual(raid_group_p, packet_p);
            return FBE_STATUS_MORE_PROCESSING_REQUIRED;
        } else 
#endif
        {
            persistent_operation_p = fbe_payload_ex_allocate_persistent_memory_operation(payload_p);

            fbe_raid_geometry_get_data_disks(raid_geometry_p, &data_disks);
            fbe_payload_persistent_memory_build_read_and_pin(persistent_operation_p, 
                                                             encryption_context_p->top_lba,
                                                             encryption_context_p->block_count * data_disks,
                                                             encryption_context_p->object_index,
                                                             encryption_context_p->lun_object_id,
                                                             encryption_context_p->sg_p,
                                                             (FBE_MEMORY_CHUNK_SIZE_FOR_64_BLOCKS_IO /
                                                              sizeof(fbe_sg_element_t)) - 1,    /* Max sgs */
                                                             encryption_context_p->chunk_p,
                                                             encryption_context_p->b_beyond_capacity);
            fbe_payload_ex_increment_persistent_memory_operation_level(payload_p);

            fbe_transport_set_completion_function(packet_p, fbe_raid_group_encryption_pin_complete, raid_group_p);
            fbe_raid_group_bg_op_set_pin_time(raid_group_p, fbe_get_time());
            fbe_raid_group_bg_op_set_flag(raid_group_p, FBE_RAID_GROUP_BG_OP_FLAG_PIN_OUTSTANDING);
            /* Continue rekey with the persist operation.
             */
            fbe_persistent_memory_operation_entry(packet_p);
            return FBE_STATUS_MORE_PROCESSING_REQUIRED;
        }
    }
    fbe_payload_ex_release_block_operation(payload_p, block_operation_p);
    return packet_status;
}
/**************************************
 * end fbe_raid_group_encryption_send_io_completion()
 **************************************/

/*!**************************************************************
 * fbe_raid_group_encryption_can_complete()
 ****************************************************************
 * @brief
 *  Determine if we need to wait for the keys to be removed
 *  before completing the rekey.
 *
 * @param raid_group_p - Current RG.
 * @param b_complete_p - Output containing TRUE if we can complete.
 *                             FALSE if we need to wait.            
 *
 * @return fbe_status_t
 *
 * @author
 *  10/29/2013 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_raid_group_encryption_can_complete(fbe_raid_group_t *raid_group_p,
                                                    fbe_bool_t *b_complete_p)
{
    fbe_base_config_encryption_mode_t mode;

    fbe_base_config_get_encryption_mode((fbe_base_config_t*)raid_group_p, &mode);
    if (mode == FBE_BASE_CONFIG_ENCRYPTION_MODE_ENCRYPTED) {
        *b_complete_p = FBE_TRUE;
    } else {
        *b_complete_p = FBE_FALSE;
    }
    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_raid_group_encryption_can_complete()
 **************************************/
/*!**************************************************************
 * fbe_raid_group_encryption_can_continue()
 ****************************************************************
 * @brief
 *  Determine if we need to quiesce the raid group to transition
 *  the rekey state to another mode.
 *
 * @param raid_group_p - Current RG.
 * @param b_quiesce_needed_p - Output containing TRUE if we need to Quiesce.
 *                             FALSE if we can continue with the rekey.     
 * @param b_can_continue_p - Output containing TRUE if we need to continue
 *                           FALSE if we simply need to idle the rekey.       
 *
 * @return fbe_status_t
 *
 * @author
 *  10/29/2013 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_raid_group_encryption_can_continue(fbe_raid_group_t *raid_group_p,
                                                    fbe_bool_t *b_quiesce_needed_p,
                                                    fbe_bool_t *b_can_continue_p)
{
    fbe_lba_t checkpoint;
    fbe_lba_t exported_capacity;
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_group_get_raid_geometry(raid_group_p);

    *b_quiesce_needed_p = FBE_FALSE;
    *b_can_continue_p = FBE_TRUE;

    /* Everything but RAID-10 encrypts the paged at the beginning.
     */
    if (!fbe_raid_geometry_is_raid10(raid_geometry_p) &&
        !fbe_raid_group_is_encryption_state_set(raid_group_p, FBE_RAID_GROUP_ENCRYPTION_FLAG_PAGED_ENCRYPTED)) {
        *b_quiesce_needed_p = FBE_TRUE;
        return FBE_STATUS_OK;
    }

    checkpoint = fbe_raid_group_get_rekey_checkpoint(raid_group_p);
    exported_capacity = fbe_raid_group_get_exported_disk_capacity(raid_group_p);

    /* Check to see if we are done with this position or done in general.
     */
    if (checkpoint >= exported_capacity) {

        /* Simply set our condition to quiesce and continue with encryption.
         */
        fbe_bool_t b_can_complete;
        fbe_raid_group_encryption_can_complete(raid_group_p, &b_can_complete);

        /* RAID-10 quiesces at the end to reconstruct paged.
         */
        if (fbe_raid_geometry_is_raid10(raid_geometry_p) &&
            !fbe_raid_group_is_encryption_state_set(raid_group_p, FBE_RAID_GROUP_ENCRYPTION_FLAG_PAGED_ENCRYPTED)) {
            *b_quiesce_needed_p = FBE_TRUE;
            return FBE_STATUS_OK;
        }
        if (b_can_complete) {
            *b_quiesce_needed_p = FBE_TRUE;
        } else {
            fbe_base_config_encryption_mode_t mode;
            fbe_raid_group_encryption_flags_t flags;
            fbe_base_config_encryption_state_t encryption_state;
            *b_can_continue_p = FBE_FALSE;
            fbe_base_config_get_encryption_state((fbe_base_config_t*)raid_group_p, &encryption_state);
            fbe_base_config_get_encryption_mode((fbe_base_config_t*)raid_group_p, &mode);
            fbe_raid_group_encryption_get_state(raid_group_p, &flags);

            if ((encryption_state != FBE_BASE_CONFIG_ENCRYPTION_STATE_REKEY_COMPLETE) &&
                (encryption_state != FBE_BASE_CONFIG_ENCRYPTION_STATE_REKEY_COMPLETE_PUSH_DONE)) {

                fbe_scheduler_hook_status_t hook_status = FBE_SCHED_STATUS_OK;
      
                // If a debug hook is set, we need to execute the specified action...
                fbe_raid_group_check_hook(raid_group_p, 
                                          SCHEDULER_MONITOR_STATE_RAID_GROUP_ENCRYPTION,
                                          FBE_RAID_GROUP_SUBSTATE_ENCRYPTION_SET_REKEY_COMPLETE, 0, &hook_status);
                if (hook_status == FBE_SCHED_STATUS_DONE) {return FBE_LIFECYCLE_STATUS_DONE;}

                if (fbe_raid_geometry_is_raid10(raid_geometry_p)) {
                    fbe_base_config_set_encryption_state((fbe_base_config_t*)raid_group_p, 
                                                         FBE_BASE_CONFIG_ENCRYPTION_STATE_REKEY_COMPLETE);
                } else {
                    fbe_base_config_set_encryption_state_with_notification((fbe_base_config_t*)raid_group_p, 
                                                                           FBE_BASE_CONFIG_ENCRYPTION_STATE_REKEY_COMPLETE);
                }

                //fbe_base_config_set_encryption_state((fbe_base_config_t*)raid_group_p, FBE_BASE_CONFIG_ENCRYPTION_STATE_REKEY_COMPLETE);
                fbe_base_object_trace((fbe_base_object_t*)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                      "Encryption: %d -> REKEY_COMPLETE cp: %llx fl: 0x%x md: %d\n",
                                      encryption_state, checkpoint, flags, mode);
            }
        }
    }
    return FBE_TRUE;
}
/******************************************
 * end fbe_raid_group_encryption_can_continue()
 ******************************************/

static fbe_status_t fbe_raid_group_encryption_memory_alloc_completion(fbe_memory_request_t * memory_request_p, 
                                                                      fbe_memory_completion_context_t context)
{  
    fbe_packet_t                       *packet_p = NULL;
    fbe_payload_ex_t                   *payload_p = NULL;
    fbe_raid_group_t *raid_group_p = (fbe_raid_group_t *)context;
    fbe_lba_t checkpoint;
    fbe_memory_header_t *memory_header_p = NULL;
    fbe_memory_header_t *next_memory_header_p = NULL;
    fbe_u8_t *data_p = NULL;
    fbe_sg_element_t *sg_p = NULL;
    fbe_raid_group_encryption_context_t *encryption_context_p = NULL;
    fbe_cpu_id_t cpu_id;
    fbe_object_id_t raid_group_object_id;

    if (fbe_memory_request_is_allocation_complete(memory_request_p) == FBE_FALSE)
    {
        fbe_raid_group_bg_op_clear_flag(raid_group_p, FBE_RAID_GROUP_BG_OP_FLAG_OUTSTANDING);
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s allocation failed\n", __FUNCTION__);
        fbe_memory_free_request_entry(memory_request_p);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }
    memory_header_p = fbe_memory_get_first_memory_header(memory_request_p);
    data_p = fbe_memory_header_to_data(memory_header_p);
    packet_p = (fbe_packet_t*)data_p;
    data_p += FBE_MEMORY_CHUNK_SIZE;

    encryption_context_p = (fbe_raid_group_encryption_context_t *)data_p;
    data_p += FBE_MEMORY_CHUNK_SIZE;

    fbe_memory_get_next_memory_header(memory_header_p, &next_memory_header_p);
    sg_p = (fbe_sg_element_t*)fbe_memory_header_to_data(next_memory_header_p);

    checkpoint = fbe_raid_group_get_rekey_checkpoint(raid_group_p);

    /* Allocate the new packet so that this read and pin operation can run asynchronously.
     */
    fbe_transport_initialize_packet(packet_p);
    fbe_base_object_get_object_id((fbe_base_object_t *) raid_group_p, &raid_group_object_id);
    fbe_transport_set_address(packet_p,
                              FBE_PACKAGE_ID_SEP_0,
                              FBE_SERVICE_ID_TOPOLOGY,
                              FBE_CLASS_ID_INVALID,
                              raid_group_object_id);
    payload_p = fbe_transport_get_payload_ex(packet_p);

     /* we need this to make sure memory_io_master is properly set up so that
      * we won't have problem when siots needs deferred allocation.
      */
    fbe_payload_ex_allocate_memory_operation(payload_p);

    cpu_id = (memory_request_p->io_stamp & FBE_PACKET_IO_STAMP_MASK) >> FBE_PACKET_IO_STAMP_SHIFT;
    fbe_transport_set_cpu_id(packet_p, cpu_id);

    /* Allocate the rekey context.  We save a ptr to it in a control operation.
     */
    fbe_raid_group_allocate_encryption_context(raid_group_p, packet_p, encryption_context_p);

    fbe_raid_group_initialize_encryption_context(raid_group_p, encryption_context_p, checkpoint);
    encryption_context_p->sg_p = sg_p;
    encryption_context_p->chunk_p = data_p;

    /* Set time stamp so we can use it to detect slow I/O.
     */
    fbe_transport_set_physical_drive_io_stamp(packet_p, fbe_get_time());
    fbe_transport_set_completion_function(packet_p, fbe_raid_group_encryption_completion, raid_group_p);

    fbe_raid_group_encryption_request_permission(raid_group_p, encryption_context_p, packet_p);

    return FBE_STATUS_OK;
}
fbe_status_t fbe_raid_group_encryption_allocate_memory(fbe_raid_group_t *raid_group_p) 
{
    fbe_memory_request_t*                memory_request_p = NULL;
    fbe_status_t                         status;
    fbe_raid_group_bg_op_info_t *bg_op_p = NULL;

    bg_op_p = fbe_raid_group_get_bg_op_ptr(raid_group_p);

    /* We need this request to run independent of the monitor so allocate a structure now. 
     * We will free it once encryption is done. 
     */
    if (bg_op_p == NULL) {
        bg_op_p = fbe_memory_native_allocate(sizeof(fbe_raid_group_bg_op_info_t));
        if (bg_op_p == NULL) {
            fbe_base_object_trace((fbe_base_object_t*)raid_group_p, FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s unable to allocate %u bytes, retry\n",
                                  __FUNCTION__, (fbe_u32_t)sizeof(fbe_raid_group_bg_op_info_t));
            return FBE_STATUS_MORE_PROCESSING_REQUIRED;
        }
        fbe_zero_memory(bg_op_p, sizeof(fbe_raid_group_bg_op_info_t));
        fbe_raid_group_set_bg_op_ptr(raid_group_p, bg_op_p);
    }
    memory_request_p = fbe_raid_group_get_bg_op_memory_request(raid_group_p);

    status = fbe_memory_build_chunk_request(memory_request_p,
                                            FBE_MEMORY_CHUNK_SIZE_64_BLOCKS_64_BLOCKS,
                                            2,    /* SG, packet and control structures*/
                                            0, /* Priority */
                                            0, /* Io stamp*/
                                            (fbe_memory_completion_function_t)fbe_raid_group_encryption_memory_alloc_completion,
                                            raid_group_p);
    if (status == FBE_STATUS_GENERIC_FAILURE) {
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "Encryption setup for mem alloc failed, status:0x%x, line:%d\n", status, __LINE__);  
        return FBE_STATUS_OK;
    }
    /* Set a flag to indicate the monitor should not run the bg op condition.  
     */
    fbe_raid_group_bg_op_set_time(raid_group_p, fbe_get_time());
    fbe_raid_group_bg_op_set_flag(raid_group_p, FBE_RAID_GROUP_BG_OP_FLAG_OUTSTANDING);

    fbe_memory_request_entry(memory_request_p);
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/*!****************************************************************************
 * fbe_raid_group_monitor_run_encryption()
 ******************************************************************************
 * @brief
 *  Start an encryption operation.
 *
 * @param object_p               - pointer to a base object 
 * @param packet_p               - pointer to a control packet from the scheduler
 *
 * @return fbe_lifecycle_status_t   - FBE_LIFECYCLE_STATUS_RESCHEDULE on success
 *                                  - FBE_LIFECYCLE_STATUS_PENDING when I/O is
 *                                    being issued 
 *                                  - FBE_LIFECYCLE_STATUS_DONE  when operation done
 *                                    without I/O is being issued
 * @author
 *
 ******************************************************************************/
fbe_lifecycle_status_t 
fbe_raid_group_monitor_run_encryption(fbe_base_object_t *object_p,
                                      fbe_packet_t *packet_p)
{   
    fbe_raid_group_t*                   raid_group_p = (fbe_raid_group_t*) object_p;
    fbe_lba_t                           checkpoint;
    fbe_scheduler_hook_status_t         hook_status = FBE_SCHED_STATUS_OK;
    fbe_bool_t                          b_encryption_rekey_enabled;
    fbe_medic_action_priority_t         ds_medic_priority;
    fbe_medic_action_priority_t         rg_medic_priority;
    fbe_bool_t b_can_rekey_start;
    fbe_bool_t b_quiesce_needed;
    fbe_bool_t b_can_continue;
    fbe_status_t status;
    fbe_bool_t encryption_paused;
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_group_get_raid_geometry(raid_group_p);

    fbe_base_config_is_background_operation_enabled((fbe_base_config_t *) raid_group_p, 
                                                    FBE_RAID_GROUP_BACKGROUND_OP_ENCRYPTION_REKEY, &b_encryption_rekey_enabled);

    status = fbe_database_get_encryption_paused(&encryption_paused);

    if((status != FBE_STATUS_OK) || (encryption_paused == FBE_TRUE))
    {


        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "Encryption run encryption: 0x%x is paused \n",
                              FBE_RAID_GROUP_BACKGROUND_OP_ENCRYPTION_REKEY);

        fbe_raid_group_check_hook(raid_group_p,
                                  SCHEDULER_MONITOR_STATE_RAID_GROUP_ENCRYPTION, 
                                  FBE_RAID_GROUP_SUBSTATE_ENCRYPTION_PAUSED, 0,  &hook_status);

        return FBE_LIFECYCLE_STATUS_DONE;
    }
    else
    {
        fbe_raid_group_check_hook(raid_group_p,
                                  SCHEDULER_MONITOR_STATE_RAID_GROUP_ENCRYPTION, 
                                  FBE_RAID_GROUP_SUBSTATE_ENCRYPTION_UNPAUSED, 0,  &hook_status);
    }


    if ( b_encryption_rekey_enabled == FBE_FALSE ) {
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "Encryption run encryption: 0x%x is disabled \n",
                              FBE_RAID_GROUP_BACKGROUND_OP_ENCRYPTION_REKEY);
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    checkpoint = fbe_raid_group_get_rekey_checkpoint(raid_group_p);

    fbe_raid_group_trace(raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_RAID_GROUP_DEBUG_FLAG_ENCRYPTION_TRACING,
                              "Encryption entry chkpt: 0x%llx\n", checkpoint);

    fbe_raid_group_check_hook(raid_group_p,  SCHEDULER_MONITOR_STATE_RAID_GROUP_ENCRYPTION, 
                              FBE_RAID_GROUP_SUBSTATE_ENCRYPTION_ENTRY, checkpoint, &hook_status);
    if (hook_status == FBE_SCHED_STATUS_DONE) {
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /* When we start, move the checkpoint from invalid to 0. 
     * Other raid types do this automatically when they inform PVD and reconstruct paged. 
     * But since RAID-10 does that step last, we need to update the NP first. 
     */
    if (fbe_raid_geometry_is_raid10(raid_geometry_p) &&
        (checkpoint == FBE_LBA_INVALID)) {
        fbe_raid_group_encryption_start(raid_group_p, packet_p);
        return FBE_LIFECYCLE_STATUS_PENDING;
    }
    /* When we start and end rekey we need to quiesce. 
     * Quiesce is also needed when we switch from one drive to the next. 
     */
    fbe_raid_group_encryption_can_continue(raid_group_p, &b_quiesce_needed, &b_can_continue);

    if (b_quiesce_needed) {
        fbe_raid_group_encryption_flags_t flags;
        fbe_raid_group_encryption_get_state(raid_group_p, &flags);
        /* We need to perform an operation under quiesce.
         */
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "Encryption Quiesce Raid Group state: 0x%x.\n", flags);
        /* Set this bit so the quiesce will first quiesce the bts. 
         * This enables us to quiesce with nothing in flight on the terminator queue and holding 
         * stripe locks. 
         */
        fbe_base_config_set_clustered_flag((fbe_base_config_t *) raid_group_p, 
                                           FBE_BASE_CONFIG_METADATA_MEMORY_FLAGS_QUIESCE_HOLD);
        fbe_lifecycle_set_cond(&fbe_raid_group_lifecycle_const, (fbe_base_object_t*) raid_group_p,
                               FBE_BASE_CONFIG_LIFECYCLE_COND_QUIESCE);
        fbe_lifecycle_set_cond(&fbe_raid_group_lifecycle_const, (fbe_base_object_t*) raid_group_p,
                               FBE_BASE_CONFIG_LIFECYCLE_COND_UNQUIESCE);
        fbe_lifecycle_set_cond(&fbe_raid_group_lifecycle_const, (fbe_base_object_t*) raid_group_p,
                               FBE_RAID_GROUP_LIFECYCLE_COND_REKEY_OPERATION);
        return FBE_LIFECYCLE_STATUS_DONE;
    } else if (!b_can_continue) {
        /* Must wait for something before rekey can continue.
         */
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    ds_medic_priority = fbe_base_config_get_downstream_edge_priority((fbe_base_config_t*)raid_group_p);
    fbe_raid_group_get_medic_action_priority(raid_group_p, &rg_medic_priority);

    if (ds_medic_priority > rg_medic_priority) {
        fbe_raid_group_check_hook(raid_group_p,
                                  SCHEDULER_MONITOR_STATE_RAID_GROUP_ENCRYPTION, 
                                  FBE_RAID_GROUP_SUBSTATE_ENCRYPTION_DOWNSTREAM_HIGHER_PRIORITY, 0,  &hook_status);
        /* No hook status is handled since we can only log here.
         */
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                              FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "Encryption downstream medic priority %d > rg medic priority %d\n",
                              ds_medic_priority, rg_medic_priority);
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /* Trace some information if enabled.
     */
    fbe_raid_group_trace(raid_group_p,
                         FBE_TRACE_LEVEL_INFO,
                         FBE_RAID_GROUP_DEBUG_FLAG_ENCRYPTION_TRACING,
                         "Encryption run encryption: checkpoint: 0x%llx \n",
                         (unsigned long long)fbe_raid_group_get_rekey_checkpoint(raid_group_p));


    if (checkpoint == 0) {
        raid_group_p->background_start_time = fbe_get_time();
    }

    /* Else rekey is able to run.  Check the debug hook and make sure the checkpoint matches.
     */
    fbe_raid_group_check_hook(raid_group_p,  SCHEDULER_MONITOR_STATE_RAID_GROUP_ENCRYPTION, 
                              FBE_RAID_GROUP_SUBSTATE_ENCRYPTION_START, checkpoint, &hook_status);
    if (hook_status == FBE_SCHED_STATUS_DONE) {
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    //  See if we are allowed to do a rekey I/O based on the current load and active/passive state
    fbe_raid_group_rebuild_determine_if_rebuild_can_start(raid_group_p, &b_can_rekey_start);
    if (b_can_rekey_start == FBE_FALSE) {
        /* Check the debug hook.*/
        fbe_raid_group_check_hook(raid_group_p,
                                  SCHEDULER_MONITOR_STATE_RAID_GROUP_ENCRYPTION, 
                                  FBE_RAID_GROUP_SUBSTATE_ENCRYPTION_NO_PERMISSION, 
                                  0, &hook_status);
        if (hook_status == FBE_SCHED_STATUS_DONE) {
            return FBE_LIFECYCLE_STATUS_DONE;
        }

        /* We were not granted permission.  Wait the normal monitor cycle to see 
         * if the load changes.
         */
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /* Else rekey is able to run.  Check the debug hook and make sure the checkpoint matches.
     */
    fbe_raid_group_check_hook(raid_group_p,  SCHEDULER_MONITOR_STATE_RAID_GROUP_ENCRYPTION, 
                              FBE_RAID_GROUP_SUBSTATE_ENCRYPTION_START, checkpoint, &hook_status);
    if (hook_status == FBE_SCHED_STATUS_DONE) {
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    fbe_raid_group_encryption_allocate_memory(raid_group_p);
    return FBE_LIFECYCLE_STATUS_DONE;
}
/**************************************
 * end fbe_raid_group_monitor_run_encryption()
 **************************************/

/*!****************************************************************************
 * fbe_raid_group_encryption_completion()
 ******************************************************************************
 * @brief
 *   This is the completion function for the do_rekey condition.  It is invoked
 *   when a rekey I/O operation has completed.  It reschedules the monitor to
 *   immediately run again in order to perform the next rekey operation.
 *
 * @param packet_p               - pointer to a control packet from the scheduler
 * @param context                - context, which is a pointer to the base object 
 * 
 * @return fbe_status_t             - Always FBE_STATUS_OK
 *   
 * Note: This function must always return FBE_STATUS_OK so that the packet gets
 * completed to the next level.  (If any other status is returned, the packet will 
 * get stuck.)
 *
 * @author
 *  10/15/2013 - Created. Rob Foley
 *
 ******************************************************************************/
static fbe_status_t 
fbe_raid_group_encryption_completion(fbe_packet_t*                   packet_p,
                                     fbe_packet_completion_context_t context)
{

    fbe_status_t                        status;
    fbe_raid_group_t                   *raid_group_p = NULL;
    fbe_payload_ex_t                   *sep_payload_p = NULL;
    fbe_payload_control_operation_t    *control_operation_p = NULL;
    fbe_raid_group_encryption_context_t   *encryption_context_p = NULL;
    fbe_payload_control_status_t        control_status;
    fbe_time_t time_stamp;
    fbe_u32_t service_time_ms;
    fbe_memory_request_t*                memory_request_p = NULL;

    raid_group_p = (fbe_raid_group_t*) context;
    memory_request_p = fbe_raid_group_get_bg_op_memory_request(raid_group_p);

    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(sep_payload_p);
    status = fbe_transport_get_status_code(packet_p);

    fbe_payload_control_get_buffer(control_operation_p, &encryption_context_p);
    fbe_payload_control_get_status(control_operation_p, &control_status);

    /* If enabled trace some information.
     */
    fbe_raid_group_trace(raid_group_p,
                         FBE_TRACE_LEVEL_INFO,
                         FBE_RAID_GROUP_DEBUG_FLAG_ENCRYPTION_TRACING,
                         "Encryption rekey complete lba: 0x%llx blocks: 0x%llx status: %d\n",
                         (unsigned long long)encryption_context_p->start_lba,
                         (unsigned long long)encryption_context_p->block_count,
                         status);

    //  Release the context and control operation
    fbe_raid_group_release_encryption_context(packet_p);

    /* We keep the start time in the pdo time stamp since it is not used by raid.
     * The right thing to do is to have a "generic" time stamp. 
     */
    fbe_transport_get_physical_drive_io_stamp(packet_p, &time_stamp);
    service_time_ms = fbe_get_elapsed_milliseconds(time_stamp);

    /* We originally allocated the packet and associated memory when this request started.
     */
    fbe_transport_destroy_packet(packet_p);
    fbe_memory_free_request_entry(memory_request_p);

    /* Clear this flag only after we free the shared resource.
     */
    if (!fbe_raid_group_bg_op_is_flag_set(raid_group_p, FBE_RAID_GROUP_BG_OP_FLAG_OUTSTANDING)){
        fbe_base_object_trace((fbe_base_object_t*) raid_group_p, FBE_TRACE_LEVEL_ERROR, 
                              FBE_TRACE_MESSAGE_ID_INFO, "Encryption: outstanding is not set flags: 0x%x\n", 
                              (raid_group_p->bg_info_p) ? raid_group_p->bg_info_p->flags : 0);
    }
    fbe_raid_group_bg_op_clear_flag(raid_group_p, FBE_RAID_GROUP_BG_OP_FLAG_OUTSTANDING);

    //  Check for a failure on the I/O.  If one occurred, trace and return here so the monitor is scheduled on its
    //  normal cycle.
    if ((status != FBE_STATUS_OK) || 
        (control_status != FBE_PAYLOAD_CONTROL_STATUS_OK) ||
        (encryption_context_p->encryption_status != FBE_STATUS_OK)) {
        fbe_raid_group_trace(raid_group_p, FBE_TRACE_LEVEL_WARNING, FBE_RAID_GROUP_DEBUG_FLAG_ENCRYPTION_TRACING,
                             "Encryption failed, status: 0x%x/0x%x control_status: %d\n", 
                             status, encryption_context_p->encryption_status, control_status);
        /* Don't reschedule.  Wait for next monitor cycle.
         */
        return FBE_STATUS_GENERIC_FAILURE;
    }


    if (service_time_ms > FBE_RAID_GROUP_MAX_IO_SERVICE_TIME) {
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "Encryption Completion: service time: %d Reschedule for: %d ms\n",
                              service_time_ms, FBE_RAID_GROUP_SLOW_IO_RESCHEDULE_TIME);
        /* Simply reschedule for a greater time if the I/O took longer than our threshold..
         */
        fbe_lifecycle_reschedule(&fbe_raid_group_lifecycle_const, 
                                 (fbe_base_object_t*) raid_group_p, 
                                 FBE_RAID_GROUP_SLOW_IO_RESCHEDULE_TIME);
        return FBE_STATUS_OK;
    }

    if(raid_group_p->base_config.background_operation_count < fbe_raid_group_get_rebuild_speed()){
        raid_group_p->base_config.background_operation_count++;
        fbe_lifecycle_reschedule(&fbe_raid_group_lifecycle_const, (fbe_base_object_t*) raid_group_p, 0);
    } else {
        raid_group_p->base_config.background_operation_count = 0;
        fbe_lifecycle_reschedule(&fbe_raid_group_lifecycle_const, (fbe_base_object_t*) raid_group_p, 200);
    }

    //  Return success in all cases so the packet is completed to the next level 
    return FBE_STATUS_OK;

} // End fbe_raid_group_encryption_completion()

/*!**************************************************************
 * fbe_raid_group_encryption_start()
 ****************************************************************
 * @brief
 *  This function handles the rekey start.
 *
 * @param raid_group_p - Current rg.
 * @param packet_p - Packet for this usurper.
 *
 * @return fbe_status_t
 *
 * @author
 *  10/25/2013 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t
fbe_raid_group_encryption_start(fbe_raid_group_t *raid_group_p, fbe_packet_t * packet_p)
{
    fbe_base_config_encryption_mode_t mode;
    fbe_base_config_encryption_state_t state;

    fbe_base_config_get_encryption_mode((fbe_base_config_t*)raid_group_p, &mode);
    fbe_base_config_get_encryption_state((fbe_base_config_t*)raid_group_p, &state);
    fbe_base_object_trace((fbe_base_object_t*)raid_group_p,
                          FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                          "Encryption starting mode: 0x%x state: 0x%x\n", mode, state);
    if ((mode != FBE_BASE_CONFIG_ENCRYPTION_MODE_ENCRYPTED) &&
        (mode != FBE_BASE_CONFIG_ENCRYPTION_MODE_ENCRYPTION_IN_PROGRESS) && 
        (mode != FBE_BASE_CONFIG_ENCRYPTION_MODE_REKEY_IN_PROGRESS)) {
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p,
                              FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                              "Encryption error mode: %d not allowed\n", mode);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }

    /* First grab the NP lock so we can update it.
     */
    fbe_raid_group_get_NP_lock(raid_group_p, packet_p, fbe_raid_group_encryption_start_mark_np);
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/**************************************
 * end fbe_raid_group_encryption_start()
 **************************************/

/*!****************************************************************************
 * fbe_raid_group_encryption_start_mark_np()
 ******************************************************************************
 * @brief
 *  This function marks NP and persists, after obtaining NP lock.
 *
 * @param packet_p - The pointer to the packet.
 * @param context  - The pointer to the object.
 *
 * @return fbe_status_t
 *
 * @author
 *  10/25/2013 - Created. Rob Foley
 *
 *****************************************************************************/
fbe_status_t 
fbe_raid_group_encryption_start_mark_np(fbe_packet_t * packet_p, 
                                        fbe_packet_completion_context_t context)
{
    fbe_raid_group_t *raid_group_p = (fbe_raid_group_t *)context;
    fbe_status_t status;         
    fbe_u64_t metadata_offset;                
    fbe_raid_group_encryption_nonpaged_info_t encryption_nonpaged_data;
    fbe_lba_t                         checkpoint;
    fbe_raid_group_encryption_flags_t flags;

    /* If the status is not ok, that means we didn't get the lock.
     */
    status = fbe_transport_get_status_code(packet_p);
    if (status != FBE_STATUS_OK) {
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s can't get the NP lock, status: 0x%X\n", __FUNCTION__, status); 
        return FBE_STATUS_OK;
    }
    checkpoint = fbe_raid_group_get_rekey_checkpoint(raid_group_p);
    fbe_raid_group_encryption_get_state(raid_group_p, &flags);

    fbe_base_object_trace((fbe_base_object_t *)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                          "Encryption start old chkpt: 0x%llx old flags: 0x%x\n",
                          checkpoint, flags);

    /* Reset our nonpaged to indicate we are just getting started.
     */
    encryption_nonpaged_data.rekey_checkpoint = 0;
    encryption_nonpaged_data.unused = 0; /* Start with position 0 */
    encryption_nonpaged_data.flags = 0;

    /* Push a completion on stack to release NP lock. */
    fbe_transport_set_completion_function(packet_p, fbe_raid_group_release_NP_lock, raid_group_p);

    metadata_offset = (fbe_u64_t) (&((fbe_raid_group_nonpaged_metadata_t*)0)->encryption);    

    status = fbe_base_config_metadata_nonpaged_write_persist((fbe_base_config_t*) raid_group_p, packet_p, 
                                                             metadata_offset, (fbe_u8_t*) &encryption_nonpaged_data, 
                                                             sizeof(encryption_nonpaged_data));
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/******************************************************************************
 * end fbe_raid_group_encryption_start_mark_np()
 ******************************************************************************/

/*!**************************************************************
 * fbe_raid_group_write_paged_encryption_completion()
 ****************************************************************
 * @brief
 *   This completion function handles a completion
 *   of a paged reconstruct.
 *
 * @param packet_p - Current usurper packet.
 * @param context - Raid group, not used.
 * 
 * @return fbe_status_t
 *
 * @author
 *  10/25/2013 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_raid_group_write_paged_encryption_completion(fbe_packet_t * packet_p, fbe_packet_completion_context_t context)
{
    fbe_status_t status;
    fbe_raid_group_t * raid_group_p = (fbe_raid_group_t*)context;

    status = fbe_transport_get_status_code(packet_p);

    if (status != FBE_STATUS_OK) {
        /* We failed, return the packet with this bad status.
         * Also take the raid group out of ready since the paged is inconsistent. 
         */
        fbe_base_object_trace( (fbe_base_object_t *)raid_group_p,
                                FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "Encryption write of paged failed. status: 0x%x\n", status);
        fbe_lifecycle_set_cond(&fbe_base_config_lifecycle_const, (fbe_base_object_t*) raid_group_p,
                               FBE_BASE_CONFIG_LIFECYCLE_COND_CLUSTERED_ACTIVATE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        return FBE_STATUS_OK;
    }
    fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                          FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                          "Encryption paged write successful.  Mark NP.\n");
    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_raid_group_write_paged_encryption_completion
 **************************************/
/*!****************************************************************************
 * fbe_raid_group_write_paged_metadata()
 ******************************************************************************
 * @brief
 *  Write the entire paged.
 *
 * @param packet_p               - pointer to a control packet from the scheduler
 * @param raid_group_p           - pointer to a base object 
 *
 * @return fbe_lifecycle_status_t   - FBE_LIFECYCLE_STATUS_RESCHEDULE on success
 *                                  - FBE_LIFECYCLE_STATUS_PENDING when I/O is
 *                                    being issued 
 *                                  - FBE_LIFECYCLE_STATUS_DONE  when operation done
 *                                    without I/O is being issued
 * @author
 *  10/25/2013 - Created. Rob Foley
 *
 ******************************************************************************/
fbe_status_t 
fbe_raid_group_write_paged_metadata(fbe_packet_t *packet_p,
                                    fbe_packet_completion_context_t context)
{
    fbe_raid_group_t *raid_group_p = (fbe_raid_group_t*)context;
    fbe_status_t status;
    fbe_lba_t paged_md_start_lba;
    fbe_block_count_t paged_md_blocks;
    fbe_u16_t data_disks;
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_group_get_raid_geometry(raid_group_p);

    status = fbe_transport_get_status_code(packet_p);
    if (status != FBE_STATUS_OK) {
        /* We failed, return the packet with this bad status.
         * Also take the raid group out of ready since the paged is inconsistent. 
         */
        fbe_base_object_trace( (fbe_base_object_t *)raid_group_p,
                                FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "Encryption Write of journal failed. status: 0x%x\n", status);
        fbe_lifecycle_set_cond(&fbe_base_config_lifecycle_const, (fbe_base_object_t*) raid_group_p,
                               FBE_BASE_CONFIG_LIFECYCLE_COND_CLUSTERED_ACTIVATE);
        return FBE_STATUS_OK;
    }

    fbe_base_config_metadata_get_paged_record_start_lba(&(raid_group_p->base_config),
                                                        &paged_md_start_lba);
    fbe_base_config_metadata_get_paged_metadata_capacity(&(raid_group_p->base_config), &paged_md_blocks);

    /* Determine the start lba for the rebuild of paged.
     */
    raid_geometry_p = fbe_raid_group_get_raid_geometry(raid_group_p);
    fbe_raid_geometry_get_data_disks(raid_geometry_p, &data_disks);
    paged_md_start_lba /= data_disks;
    paged_md_blocks /= data_disks;

    fbe_base_object_trace( (fbe_base_object_t *)raid_group_p,
                           FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                           "Encryption paged write start lba: 0x%llx bl: 0x%llx.\n", paged_md_start_lba, paged_md_blocks);
    fbe_transport_set_completion_function(packet_p, fbe_raid_group_write_paged_encryption_completion, raid_group_p);
    fbe_raid_group_bitmap_reconstruct_paged_metadata(raid_group_p, 
                                                     packet_p, paged_md_start_lba, paged_md_blocks, 
                                                     FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_VERIFY);
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/**************************************
 * end fbe_raid_group_write_paged_metadata
 **************************************/

/*!**************************************************************
 * fbe_raid_group_encryption_zero_paged_completion()
 ****************************************************************
 * @brief
 *   This completion function handles a completion
 *   of a paged reconstruct.
 *
 * @param packet_p - Current usurper packet.
 * @param context - Raid group, not used.
 * 
 * @return fbe_status_t
 *
 * @author
 *  11/5/2013 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_raid_group_encryption_zero_paged_completion(fbe_packet_t * packet_p, fbe_packet_completion_context_t context)
{
    fbe_status_t status;
    fbe_raid_group_t * raid_group_p = (fbe_raid_group_t*)context;
    fbe_payload_ex_t *payload_p = NULL;
    fbe_payload_block_operation_t *block_operation_p = NULL;
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_group_get_raid_geometry(raid_group_p);
    fbe_lba_t journal_log_start_lba;

    status = fbe_transport_get_status_code(packet_p);
    payload_p = fbe_transport_get_payload_ex(packet_p);
    block_operation_p = fbe_payload_ex_get_block_operation(payload_p);
    if (block_operation_p == NULL) {
        fbe_base_object_trace( (fbe_base_object_t *)raid_group_p,
                               FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "Encryption Zero of paged no payload %s\n", __FUNCTION__);
    }
    fbe_payload_ex_release_block_operation(payload_p, block_operation_p);
    if ((status != FBE_STATUS_OK) ||
        (block_operation_p->status != FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS)) {
        /* We failed, return the packet with this bad status.
         * Also take the raid group out of ready since the paged is inconsistent. 
         */
        fbe_base_object_trace( (fbe_base_object_t *)raid_group_p,
                                FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "Encryption zero of paged failed. status: 0x%x bl_status: 0x%x\n", 
                               status, block_operation_p->status);

        if (block_operation_p->status != FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS) {
            fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        }
        fbe_lifecycle_set_cond(&fbe_base_config_lifecycle_const, (fbe_base_object_t*) raid_group_p,
                               FBE_BASE_CONFIG_LIFECYCLE_COND_CLUSTERED_ACTIVATE);
        return FBE_STATUS_OK;
    }
    fbe_transport_set_completion_function(packet_p, fbe_raid_group_write_paged_metadata, raid_group_p);
    fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                          FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                          "Encryption paged zero successful.  Mark NP.\n");

    fbe_raid_geometry_get_journal_log_start_lba(raid_geometry_p, &journal_log_start_lba);
    if (journal_log_start_lba != FBE_LBA_INVALID) {
        fbe_raid_iots_t *iots_p = NULL;
        fbe_payload_ex_get_iots(payload_p, &iots_p);
        /* Only parity raid groups have a journal area.  For other raid groups this is invalid.
         */
        iots_p->lba = journal_log_start_lba;
        fbe_raid_group_monitor_initialize_journal((fbe_base_object_t*)raid_group_p, packet_p);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }
    return FBE_STATUS_CONTINUE;
}
/**************************************
 * end fbe_raid_group_encryption_zero_paged_completion
 **************************************/
/*!****************************************************************************
 * fbe_raid_group_zero_paged_metadata()
 ******************************************************************************
 * @brief
 *  Zero the entire paged.  This prevents us from reading drives with the
 *  incorrect key.  RAID will do a pre-read for unaligned writes.
 *
 * @param packet_p               - pointer to a control packet from the scheduler
 * @param raid_group_p           - pointer to a base object 
 *
 * @return fbe_lifecycle_status_t   - FBE_LIFECYCLE_STATUS_RESCHEDULE on success
 *                                  - FBE_LIFECYCLE_STATUS_PENDING when I/O is
 *                                    being issued 
 *                                  - FBE_LIFECYCLE_STATUS_DONE  when operation done
 *                                    without I/O is being issued
 * @author
 *  11/5/2013 - Created. Rob Foley
 *
 ******************************************************************************/
fbe_status_t 
fbe_raid_group_zero_paged_metadata(fbe_packet_t *packet_p,
                                    fbe_packet_completion_context_t context)
{
    fbe_raid_group_t *raid_group_p = (fbe_raid_group_t*)context;
    fbe_status_t status;
    fbe_lba_t paged_md_start_lba;
    fbe_block_count_t paged_md_blocks;
    fbe_payload_ex_t *payload_p = NULL;
    fbe_payload_block_operation_t *block_operation_p = NULL;

    payload_p = fbe_transport_get_payload_ex(packet_p);
    status = fbe_transport_get_status_code(packet_p);
    if (status != FBE_STATUS_OK) {
        fbe_base_object_trace( (fbe_base_object_t *)raid_group_p,
                               FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "Encryption zero paged error 0x%x\n", status);
        return FBE_STATUS_OK;
    }

    fbe_base_config_metadata_get_paged_record_start_lba(&(raid_group_p->base_config),
                                                        &paged_md_start_lba);
    fbe_base_config_metadata_get_paged_metadata_capacity(&(raid_group_p->base_config), &paged_md_blocks);

    fbe_base_object_trace( (fbe_base_object_t *)raid_group_p,
                           FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                           "Encryption zero paged start lba: 0x%llx bl: 0x%llx.\n", paged_md_start_lba, paged_md_blocks);
    fbe_transport_set_completion_function(packet_p, fbe_raid_group_encryption_zero_paged_completion, raid_group_p);

    block_operation_p = fbe_payload_ex_allocate_block_operation(payload_p);
    fbe_payload_block_build_operation(block_operation_p, 
                                      FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_ZEROS,
                                      paged_md_start_lba,
                                      paged_md_blocks,
                                      FBE_BE_BYTES_PER_BLOCK, 1, /* optimal block size */ 
                                      NULL);
    /* Mark as md op so it does not get quiesced.
     */
    fbe_payload_block_set_flag(block_operation_p, FBE_PAYLOAD_BLOCK_OPERATION_FLAGS_METADATA_OP);
    status = fbe_payload_ex_increment_block_operation_level(payload_p);

    /* We use the new key so that the zero data is encrypted with the new key.
     */
    fbe_base_config_bouncer_entry((fbe_base_config_t *)raid_group_p, packet_p);
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/**************************************
 * end fbe_raid_group_zero_paged_metadata
 **************************************/
/*!****************************************************************************
 * fbe_raid_group_set_paged_encrypted()
 ******************************************************************************
 * @brief
 *  This function marks NP and persists, after obtaining NP lock.
 *  We are updating the np in order to mark that the paged is encrypted.
 *
 *  We need to mark this since once we reconstruct the paged with the new
 *  encryption key we need to remember that it is already encrypted.
 *
 * @param packet_p - The pointer to the packet.
 * @param context  - The pointer to the object.
 *
 * @return fbe_status_t
 *
 * @author
 *  10/25/2013 - Created. Rob Foley
 *
 *****************************************************************************/
fbe_status_t 
fbe_raid_group_set_paged_encrypted(fbe_packet_t * packet_p, 
                                    fbe_packet_completion_context_t context)
{
    fbe_raid_group_t *raid_group_p = (fbe_raid_group_t *)context;
    fbe_status_t status;         
    fbe_u64_t metadata_offset;
    fbe_raid_group_encryption_nonpaged_info_t encryption_nonpaged_data;
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_group_get_raid_geometry(raid_group_p);

    /* If the status is not ok, that means we didn't get the lock.
     */
    status = fbe_transport_get_status_code(packet_p);
    if (status != FBE_STATUS_OK) {
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "Encryption %s can't get the NP lock, status: 0x%X\n", __FUNCTION__, status); 
        return FBE_STATUS_OK;
    }

    fbe_base_object_trace((fbe_base_object_t *)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                          "Encryption Mark Paged Encrypted Fetched NP lock.\n");

    /* Push a completion on stack to release NP lock. */
    fbe_transport_set_completion_function(packet_p, fbe_raid_group_release_NP_lock, raid_group_p);

    if (fbe_raid_geometry_is_raid10(raid_geometry_p)) {
        
        /* RAID-10 writes the paged last.
         * We will leave the checkpoint as is, but we will update the flags.
         */
        encryption_nonpaged_data.flags = FBE_RAID_GROUP_ENCRYPTION_FLAG_PVD_NOTIFIED | FBE_RAID_GROUP_ENCRYPTION_FLAG_PAGED_ENCRYPTED;
        metadata_offset = (fbe_u64_t) (&((fbe_raid_group_nonpaged_metadata_t*)0)->encryption.flags);    
    
        status = fbe_base_config_metadata_nonpaged_write_persist((fbe_base_config_t*) raid_group_p, packet_p, 
                                                                 metadata_offset, (fbe_u8_t*) &encryption_nonpaged_data.flags, 
                                                                 sizeof(fbe_raid_group_encryption_flags_t));
    } else if (fbe_raid_geometry_is_vault(raid_geometry_p)) {
        /* We only set the paged encrypted after we zeroed the entire user capacity. 
         * Mark the encryption as done. 
         */
        encryption_nonpaged_data.rekey_checkpoint = fbe_raid_group_get_exported_disk_capacity(raid_group_p);
        encryption_nonpaged_data.unused = 0; /* Position 0 */
        encryption_nonpaged_data.flags = FBE_RAID_GROUP_ENCRYPTION_FLAG_PVD_NOTIFIED | FBE_RAID_GROUP_ENCRYPTION_FLAG_PAGED_ENCRYPTED;
        metadata_offset = (fbe_u64_t) (&((fbe_raid_group_nonpaged_metadata_t*)0)->encryption);    
    
        status = fbe_base_config_metadata_nonpaged_write_persist((fbe_base_config_t*) raid_group_p, packet_p, 
                                                                 metadata_offset, (fbe_u8_t*) &encryption_nonpaged_data, 
                                                                 sizeof(fbe_raid_group_encryption_nonpaged_info_t));
    }
    else {
        /* Mark PVD notified and setup other fields also in paged since we are starting the rekey.
         * We cannot do this until the paged is initted since setting rekey positions and checkpoints will cause I/Os to 
         * expect the paged is in a certain state with the rekey bit cleared. 
         */
        encryption_nonpaged_data.rekey_checkpoint = 0;
        encryption_nonpaged_data.unused = 0; /* Position 0 */
        encryption_nonpaged_data.flags = FBE_RAID_GROUP_ENCRYPTION_FLAG_PVD_NOTIFIED | FBE_RAID_GROUP_ENCRYPTION_FLAG_PAGED_ENCRYPTED;
        metadata_offset = (fbe_u64_t) (&((fbe_raid_group_nonpaged_metadata_t*)0)->encryption);    
    
        status = fbe_base_config_metadata_nonpaged_write_persist((fbe_base_config_t*) raid_group_p, packet_p, 
                                                                 metadata_offset, (fbe_u8_t*) &encryption_nonpaged_data, 
                                                                 sizeof(fbe_raid_group_encryption_nonpaged_info_t));
    }
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/******************************************************************************
 * end fbe_raid_group_set_paged_encrypted()
 ******************************************************************************/
/*!****************************************************************************
 * fbe_raid_group_encryption_mark_paged_nr()
 ******************************************************************************
 * @brief
 *  This function marks NP and persists, after obtaining NP lock.
 *  We are updating the np in order to mark that the paged needs to be reconstructed.
 *  The reason is because we are going to write the entire paged as encrypted.
 *  If this fails then we will need to re-write the paged from scratch since
 *  an incomplete write might leave the paged in a partially encrypted state.
 *
 * @param packet_p - The pointer to the packet.
 * @param context  - The pointer to the object.
 *
 * @return fbe_status_t
 *
 * @author
 *  10/25/2013 - Created. Rob Foley
 *
 *****************************************************************************/
fbe_status_t 
fbe_raid_group_encryption_mark_paged_nr(fbe_packet_t * packet_p, 
                                        fbe_packet_completion_context_t context)
{
    fbe_raid_group_t *raid_group_p = (fbe_raid_group_t *)context;
    fbe_status_t status;    
    fbe_u64_t metadata_offset;
    fbe_raid_group_encryption_flags_t flags = (FBE_RAID_GROUP_ENCRYPTION_FLAG_PAGED_NEEDS_RECONSTRUCT |
                                               FBE_RAID_GROUP_ENCRYPTION_FLAG_PVD_NOTIFIED);

    /* If the status is not ok, that means we didn't get the lock.
     */
    status = fbe_transport_get_status_code(packet_p);
    if (status != FBE_STATUS_OK) {
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "Encryption %s can't get the NP lock, status: 0x%X\n", __FUNCTION__, status); 
        return FBE_STATUS_OK;
    }

    fbe_base_object_trace((fbe_base_object_t *)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                          "Encryption Mark Paged NR Fetched NP lock.\n");

    /* Push a completion on stack to release NP lock. */
    fbe_transport_set_completion_function(packet_p, fbe_raid_group_release_NP_lock, raid_group_p);

    metadata_offset = (fbe_u64_t) (&((fbe_raid_group_nonpaged_metadata_t*)0)->encryption.flags);    

    status = fbe_base_config_metadata_nonpaged_write_persist((fbe_base_config_t*) raid_group_p, packet_p, 
                                                             metadata_offset, (fbe_u8_t*) &flags, 
                                                             sizeof(fbe_raid_group_encryption_flags_t));
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/******************************************************************************
 * end fbe_raid_group_encryption_mark_paged_nr()
 ******************************************************************************/
/*!****************************************************************************
 * fbe_raid_group_encrypt_paged_completion()
 ******************************************************************************
 * @brief
 *  Get the NP lock so we can mark the NP flag that paged is encrypted.
 *
 * @param packet_p               - pointer to a control packet from the scheduler
 * @param context                - pointer to a base object 
 *
 * @return fbe_lifecycle_status_t   - FBE_LIFECYCLE_STATUS_RESCHEDULE on success
 *                                  - FBE_LIFECYCLE_STATUS_PENDING when I/O is
 *                                    being issued 
 *                                  - FBE_LIFECYCLE_STATUS_DONE  when operation done
 *                                    without I/O is being issued
 * @author
 *
 ******************************************************************************/
fbe_status_t 
fbe_raid_group_encrypt_paged_completion(fbe_packet_t *packet_p,
                                        fbe_packet_completion_context_t context)
{
    fbe_raid_group_t *raid_group_p = (fbe_raid_group_t*)context;
    fbe_status_t status;
    fbe_scheduler_hook_status_t         hook_status = FBE_SCHED_STATUS_OK;

    status = fbe_transport_get_status_code(packet_p);
    if (status != FBE_STATUS_OK) {
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "Encryption %s paged failed status: 0x%X\n", __FUNCTION__, status); 
        return FBE_STATUS_OK;
    }

    fbe_raid_group_check_hook(raid_group_p,
                              SCHEDULER_MONITOR_STATE_RAID_GROUP_ENCRYPTION,
                              FBE_RAID_GROUP_SUBSTATE_ENCRYPTION_PAGED_WRITE_COMPLETE,
                              0, &hook_status);
    if (hook_status == FBE_SCHED_STATUS_DONE) { return FBE_LIFECYCLE_STATUS_DONE; }

    fbe_raid_group_get_NP_lock(raid_group_p, packet_p, fbe_raid_group_set_paged_encrypted);
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/**************************************
 * end fbe_raid_group_encrypt_paged_completion
 **************************************/
/*!****************************************************************************
 * fbe_raid_group_encrypt_paged()
 ******************************************************************************
 * @brief
 *  Start an encryption operation on the paged.
 *
 * @param raid_group_p           - pointer to a base object 
 * @param packet_p               - pointer to a control packet from the scheduler
 *
 * @return fbe_lifecycle_status_t   - FBE_LIFECYCLE_STATUS_RESCHEDULE on success
 *                                  - FBE_LIFECYCLE_STATUS_PENDING when I/O is
 *                                    being issued 
 *                                  - FBE_LIFECYCLE_STATUS_DONE  when operation done
 *                                    without I/O is being issued
 * @author
 *
 ******************************************************************************/
fbe_status_t 
fbe_raid_group_encrypt_paged(fbe_raid_group_t *raid_group_p,
                             fbe_packet_t *packet_p)
{
    fbe_transport_set_completion_function(packet_p, fbe_raid_group_encrypt_paged_completion, raid_group_p);
    fbe_transport_set_completion_function(packet_p, fbe_raid_group_zero_paged_metadata, raid_group_p);
    fbe_raid_group_get_NP_lock(raid_group_p, packet_p, fbe_raid_group_encryption_mark_paged_nr);
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/**************************************
 * end fbe_raid_group_encrypt_paged
 **************************************/

/*!****************************************************************************
 * fbe_raid_group_is_encryption_state_set()
 ****************************************************************************** 
 * @brief
 *  Determine if this RG is in a particular encryption state.
 *
 * @param raid_group_p - Pointer to RG object
 * @param flags - Flag to check for.
 *
 * @return  fbe_bool_t - FBE_TRUE - iw_verify_required is set and thus
 *                                  another verify pass is needed.
 *                       FBE_FALSE - No special action required.
 * @author
 *  10/25/2013 - Created. Rob Foley
 *
 ******************************************************************************/
fbe_bool_t fbe_raid_group_is_encryption_state_set(fbe_raid_group_t *raid_group_p,
                                                 fbe_raid_group_encryption_flags_t flags)
{
    fbe_raid_group_nonpaged_metadata_t *nonpaged_metadata_p;
    
    fbe_base_config_get_nonpaged_metadata_ptr((fbe_base_config_t*)raid_group_p, (void **)&nonpaged_metadata_p);

    return (nonpaged_metadata_p->encryption.flags & flags);
}
/*******************************************************
 * end fbe_raid_group_is_encryption_state_set()
********************************************************/
/*!****************************************************************************
 * fbe_raid_group_encryption_get_state()
 ****************************************************************************** 
 * @brief
 *  Determine if this RG is in a particular encryption state.
 *
 * @param raid_group_p - Pointer to RG object
 * @param flags - Flag to check for.
 *
 * @return  fbe_bool_t - FBE_TRUE - iw_verify_required is set and thus
 *                                  another verify pass is needed.
 *                       FBE_FALSE - No special action required.
 * @author
 *  10/29/2013 - Created. Rob Foley
 *
 ******************************************************************************/
void fbe_raid_group_encryption_get_state(fbe_raid_group_t *raid_group_p,
                                            fbe_raid_group_encryption_flags_t *flags_p)
{
    fbe_raid_group_nonpaged_metadata_t *nonpaged_metadata_p;
    
    fbe_base_config_get_nonpaged_metadata_ptr((fbe_base_config_t*)raid_group_p, (void **)&nonpaged_metadata_p);

    *flags_p = nonpaged_metadata_p->encryption.flags;
}
/*******************************************************
 * end fbe_raid_group_encryption_get_state()
********************************************************/

/*!**************************************************************
 * fbe_raid_group_encryption_persist_chkpt_continue()
 ****************************************************************
 * @brief
 *  NP lock is obtained, continue with the persist of chkpt.
 *
 * @param packet_p -
 * @param context - rg ptr.
 *
 * @return fbe_status_t
 *
 * @author
 *  4/23/2014 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_raid_group_encryption_persist_chkpt_continue(fbe_packet_t * packet_p,                                    
                                                              fbe_packet_completion_context_t context)
{
    fbe_raid_group_t *raid_group_p = (fbe_raid_group_t *)context;
    fbe_payload_ex_t*                sep_payload_p = NULL;
    fbe_payload_block_operation_t   *block_operation_p = NULL;
    fbe_u64_t                        metadata_offset = 0;
    fbe_lba_t                        new_checkpoint;
    fbe_status_t                     status;


    status = fbe_transport_get_status_code(packet_p);
    if (status != FBE_STATUS_OK) {
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "Encryption can't get the NP lock for chkpt persist status: 0x%X\n",  status); 
        return FBE_STATUS_OK;
    }
    metadata_offset = (fbe_u64_t)(&((fbe_raid_group_nonpaged_metadata_t*)0)->encryption.rekey_checkpoint);

    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    block_operation_p = fbe_payload_ex_get_prev_block_operation(sep_payload_p); // NP lock is first payload.

    new_checkpoint = block_operation_p->lba + block_operation_p->block_count;
    fbe_transport_set_completion_function(packet_p, fbe_raid_group_release_NP_lock, raid_group_p);

    fbe_transport_set_completion_function(packet_p, fbe_raid_group_post_checkpoint_update, raid_group_p);

    status = fbe_base_config_metadata_nonpaged_write_persist((fbe_base_config_t*) raid_group_p, packet_p, 
                                                             metadata_offset, 
                                                             (fbe_u8_t*)&new_checkpoint,
                                                             sizeof(fbe_lba_t));
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/******************************************
 * end fbe_raid_group_encryption_persist_chkpt_continue()
 ******************************************/
/*!****************************************************************************
 * fbe_raid_group_update_rekey_checkpoint()
 ******************************************************************************
 * @brief
 *   This function updates the rekey checkpoint
 * 
 * @param packet_p              - pointer to the packet
 * @param raid_group_p          - pointer to the raid group
 * @param start_lba       - Start lba of the verify
 * @param caller
 * 
 * @return  fbe_status_t            
 *
 * @author
 *  10/28/2013 - Created. Rob Foley
 ******************************************************************************/
fbe_status_t fbe_raid_group_update_rekey_checkpoint(fbe_packet_t*     packet_p,
                                                    fbe_raid_group_t* raid_group_p,
                                                    fbe_lba_t         start_lba,
                                                    fbe_block_count_t block_count,
                                                    const char *caller)
{
    fbe_lba_t                                     capacity;
    fbe_lba_t                                     exported_capacity;
    fbe_lba_t                                     new_checkpoint;
    fbe_status_t                                  status; 
    fbe_u64_t                                     metadata_offset = 0;
    fbe_u64_t                                     second_metadata_offset = 0;
    fbe_lba_t                                     current_checkpoint;
    fbe_raid_group_nonpaged_metadata_t*           raid_group_non_paged_metadata_p = NULL;
    fbe_payload_block_operation_t                *block_operation_p = NULL;
    fbe_payload_ex_t                             *sep_payload_p = NULL;

    sep_payload_p = fbe_transport_get_payload_ex(packet_p);

    // Update the rekey checkpoint
    new_checkpoint = start_lba + block_count;
    
    fbe_base_config_get_nonpaged_metadata_ptr((fbe_base_config_t *)raid_group_p, (void **)&raid_group_non_paged_metadata_p);
   
    current_checkpoint = raid_group_non_paged_metadata_p->encryption.rekey_checkpoint;
    metadata_offset = (fbe_u64_t)(&((fbe_raid_group_nonpaged_metadata_t*)0)->encryption.rekey_checkpoint);

    /*  Get the configured and exported capacity per disk, which is the disk ending LBA + 1 */
    capacity = fbe_raid_group_get_disk_capacity(raid_group_p);
    exported_capacity = fbe_raid_group_get_exported_disk_capacity(raid_group_p);

    /*  If check point has reached to the capacity we will make check point value FBE_LBA_INVALID.
        Metadata service will perform the following operation to update check point to FBE_LBA_INVALID
        checkpoint = start_lba + block_count;
        checkpoint = start_lba + FBE_LBA_INVALID - start_lba;
        
    */      
    
    /* When the start_lba < exported_capacity && checkpoint >= capacity we are crossing the user data region and moving
     * to metadata region. Since we are done with verify persist the checkpoint.
     */
    fbe_base_object_trace((fbe_base_object_t*) raid_group_p, FBE_TRACE_LEVEL_DEBUG_LOW, 
                          FBE_TRACE_MESSAGE_ID_INFO, "Encryption inc chkpt: new_c: 0x%llx cap: 0x%llx blks: 0x%llx, lba:0x%llx\n",  
                          (unsigned long long)new_checkpoint, (unsigned long long)capacity, (unsigned long long)block_count, (unsigned long long)start_lba);
    fbe_base_object_trace((fbe_base_object_t*) raid_group_p, FBE_TRACE_LEVEL_DEBUG_LOW, 
                          FBE_TRACE_MESSAGE_ID_INFO, "Encryption inc chkpt: c: 0x%llx offset: %d excap:0x%llx\n",  
                          (unsigned long long)current_checkpoint, (int)metadata_offset, (unsigned long long)exported_capacity);
    fbe_raid_group_trace(raid_group_p,
                         FBE_TRACE_LEVEL_INFO,
                         FBE_RAID_GROUP_DEBUG_FLAG_ENCRYPTION_TRACING,
                         "rg update rekey chkpt: 0x%llx from: %s \n",
                         (unsigned long long)current_checkpoint, caller);
    fbe_raid_group_trace(raid_group_p,
                         FBE_TRACE_LEVEL_INFO,
                         FBE_RAID_GROUP_DEBUG_FLAG_ENCRYPTION_TRACING,
                         "rg update rekey chkpt: 0x%llx start lba: 0x%llx blks: 0x%llx\n",
                         (unsigned long long)new_checkpoint, (unsigned long long)start_lba, (unsigned long long)block_count);

    if ((start_lba < exported_capacity) && (new_checkpoint >= exported_capacity))
    {
        /* It is possible that someone moved the checkpoint.  If so, we should skip this step since otherwise
         * we will overwrite their value.  This check is safe since we hold the np distributed lock. 
         */
        if (start_lba != current_checkpoint)
        {
            fbe_base_object_trace((fbe_base_object_t*) raid_group_p, FBE_TRACE_LEVEL_DEBUG_LOW, 
                                  FBE_TRACE_MESSAGE_ID_INFO, "Encryption skip inc chk old_c: 0x%llx start_lba: 0x%llx cap: 0x%llx\n",  
                                  (unsigned long long)current_checkpoint, (unsigned long long)start_lba, (unsigned long long)capacity);
            fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
            fbe_transport_complete_packet(packet_p);
            return FBE_STATUS_MORE_PROCESSING_REQUIRED;
        }
        block_operation_p = fbe_payload_ex_allocate_block_operation(sep_payload_p);
        fbe_payload_block_build_operation(block_operation_p,
                                          FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ENCRYPTION_REKEY_WRITE,
                                          start_lba, block_count, FBE_BE_BYTES_PER_BLOCK, 1, NULL);
        fbe_payload_ex_increment_block_operation_level(sep_payload_p);
        fbe_payload_block_set_status(block_operation_p, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, 
                                     FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE);
        fbe_transport_set_completion_function(packet_p, fbe_raid_group_process_and_release_block_operation, raid_group_p);
        fbe_raid_group_get_NP_lock(raid_group_p, packet_p, fbe_raid_group_encryption_persist_chkpt_continue);
    }
    else if (new_checkpoint < current_checkpoint) 
    {
        /* Never change the checkpoint to a value that is below the non-paged.
         * Mark the request as success and complete packet.
         */
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }
    else
    {
        fbe_lba_t   last_chkpt_to_peer;

        fbe_raid_group_get_last_checkpoint_to_peer(raid_group_p, &last_chkpt_to_peer);

        if (new_checkpoint >= (last_chkpt_to_peer + FBE_RAID_GROUP_PEER_CHECKPOINT_UPDATE_WINDOW))
        {
            fbe_u32_t percentage;
            /* Periodically we will set the checkpoint to the peer. 
             * We will need to send the checkpoint to the peer if we go out of our window 
             * since the peer is assuming that above the window we are not encrypted and do not read paged. 
             * A set is needed since the peer might be out of date with us and an increment 
             * will not suffice. 
             */
            status = fbe_base_config_metadata_nonpaged_force_set_checkpoint((fbe_base_config_t *)raid_group_p,
                                                                            packet_p,
                                                                            metadata_offset,
                                                                            second_metadata_offset,
                                                                            start_lba + block_count);
            fbe_raid_group_update_last_checkpoint_time(raid_group_p);
            fbe_raid_group_update_last_checkpoint_to_peer(raid_group_p, new_checkpoint);
            if (new_checkpoint >= exported_capacity){
                percentage = 0;
            } else {
                percentage = (fbe_u32_t)((new_checkpoint * 100) / exported_capacity);
            }

            fbe_base_object_trace((fbe_base_object_t*) raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                  "Encryption: update chkpt/lst: 0x%llx/0x%llx (%3d %%) start lba: 0x%llx blks: 0x%llx\n",
                                  (unsigned long long)new_checkpoint, last_chkpt_to_peer, percentage,
                                  (unsigned long long)start_lba, (unsigned long long)block_count);
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
    }
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/**************************************
 * end fbe_raid_group_update_rekey_checkpoint
 **************************************/

/*!****************************************************************************
 * fbe_raid_group_post_checkpoint_update()
 ******************************************************************************
 * @brief
 *  Completion for when we finish updating the checkpoint in case we need
 *  to update the encryption state.
 *
 * @param packet_p - The packet sent to us from the scheduler.
 * @param context  - The pointer to the object.
 *
 * @return fbe_status_t
 *
 * @author
 *  3/24/2014 - Created. Rob Foley
 *
 *****************************************************************************/
static fbe_status_t 
fbe_raid_group_post_checkpoint_update(fbe_packet_t *packet_p,
                                      fbe_packet_completion_context_t context)
{
    fbe_raid_group_t                        *raid_group_p = (fbe_raid_group_t *) context;
    fbe_payload_ex_t                        *sep_payload_p = NULL;
    fbe_payload_block_operation_t           *block_operation_p = NULL;
    fbe_status_t                            status;
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_group_get_raid_geometry(raid_group_p);

    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    block_operation_p = fbe_payload_ex_get_present_block_operation(sep_payload_p);

    status = fbe_transport_get_status_code(packet_p);

    if (status != FBE_STATUS_OK) {
        return FBE_STATUS_OK;
    }

    /* We might need to transition the state when the peer updates its checkpoint.
     */
    if (!fbe_base_config_is_active((fbe_base_config_t*)raid_group_p) && 
        fbe_raid_geometry_is_mirror_under_striper(raid_geometry_p) &&
        (fbe_raid_group_encryption_is_rekey_needed(raid_group_p) ||
         (fbe_base_config_is_encrypted_mode((fbe_base_config_t*)raid_group_p) &&
          !fbe_base_config_is_encrypted_state((fbe_base_config_t*)raid_group_p)))) {
        /* On the passive side we might need to change our state. */
        fbe_raid_group_trace(raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_RAID_GROUP_DEBUG_FLAG_ENCRYPTION_TRACING,
                             "raid_group: passive mirror under striper, call passive change state.\n" );
        fbe_raid_group_encryption_change_passive_state(raid_group_p);
    }
    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_raid_group_post_checkpoint_update()
 **************************************/

/*!**************************************************************
 * fbe_raid_group_rekey_write_post_restart()
 ****************************************************************
 * @brief
 *  Restart writing the bits to say that the encryption write finished.
 *
 * @param packet_p
 * @param context
 * 
 * @return fbe_status_t
 *
 * @author
 *  7/21/2014 - Created. Rob Foley
 *  
 ****************************************************************/
fbe_raid_state_status_t
fbe_raid_group_rekey_write_post_restart(fbe_raid_iots_t *iots_p)
{
    fbe_status_t status;
    fbe_raid_group_t *raid_group_p = (fbe_raid_group_t*)iots_p->callback_context;
    fbe_packet_t *packet_p = iots_p->packet_p;

    status = fbe_transport_get_status_code(packet_p);

    fbe_base_object_trace((fbe_base_object_t *)raid_group_p, FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                          "Encryption write post restart iots: %p packet_status: 0x%x\n", iots_p, status);

    //  Remove the packet from the terminator queue.  We have to do this before any paged metadata access.
    fbe_raid_group_remove_from_terminator_queue(raid_group_p, packet_p);

    /* Perform the necessary bookkeeping to get this ready to restart.
     */
    fbe_raid_iots_set_callback(iots_p, fbe_raid_group_iots_completion, raid_group_p);
    fbe_raid_iots_mark_unquiesced(iots_p);
    fbe_raid_iots_reset_generate_state(iots_p);
    fbe_raid_iots_mark_complete(iots_p);

    fbe_raid_group_rekey_clear_degraded(packet_p, raid_group_p);
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/******************************************
 * end fbe_raid_group_rekey_write_post_restart()
 ******************************************/
/*!****************************************************************************
 * fbe_raid_group_post_clear_rekey_for_chunk()
 ******************************************************************************
 * @brief
 *  This is the completion function that is invoked after clearing degraded
 *  for a given chunk.
 *
 * @param packet_p - The packet sent to us from the scheduler.
 * @param context  - The pointer to the object.
 *
 * @return fbe_status_t
 *
 * @author
 *  10/28/2013 - Created. Rob Foley
 *
 *****************************************************************************/
static fbe_status_t 
fbe_raid_group_post_clear_rekey_for_chunk(fbe_packet_t *packet_p,
                                          fbe_packet_completion_context_t context)
{
    fbe_raid_group_t                        *raid_group_p = (fbe_raid_group_t *) context;
    fbe_payload_ex_t                        *sep_payload_p;
    fbe_payload_block_operation_t           *block_operation_p;
    fbe_lba_t                               start_lba; 
    fbe_block_count_t                       block_count;
    fbe_status_t                            status;
    fbe_payload_block_operation_status_t    block_status;
    fbe_payload_block_operation_qualifier_t block_qualifier;
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_group_get_raid_geometry(raid_group_p);
    fbe_raid_iots_t*                        iots_p;
    fbe_u16_t                               data_disks;
    fbe_bool_t                              packet_failed;

    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    block_operation_p = fbe_payload_ex_get_present_block_operation(sep_payload_p);
    fbe_payload_ex_get_iots(sep_payload_p, &iots_p);  

    fbe_payload_block_get_status(block_operation_p, &block_status);
    fbe_payload_block_get_qualifier(block_operation_p, &block_qualifier);

    status = fbe_transport_get_status_code(packet_p);

    if (status != FBE_STATUS_OK) {
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p, FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                "Encryption post clear rk pkt: 0x%x block_status:0x%x block_qual: 0x%x \n",
                                status, block_status, block_qualifier);
        
        fbe_raid_group_executor_handle_metadata_error(raid_group_p, packet_p,
                                                      fbe_raid_group_rekey_write_post_restart, &packet_failed,
                                                      status);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }

    if (fbe_raid_geometry_has_paged_metadata(raid_geometry_p) &&
        (block_status != FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS)) {
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p, FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                "Encryption post clear rk packet_status: 0x%x block_status:0x%x block_qual: 0x%x \n",
                                 status, block_status, block_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    start_lba = block_operation_p->lba;

    /* Since the iots block count may have been limited by the next marked extent,
     * use that to determine the new checkpoint. AR 536292
     */
    fbe_raid_iots_get_blocks(iots_p, &block_count);

    fbe_raid_geometry_get_data_disks(raid_geometry_p, &data_disks);
    start_lba /= data_disks;
    block_count /= data_disks;

    status = fbe_raid_group_update_rekey_checkpoint(packet_p, raid_group_p, start_lba, block_count, __FUNCTION__);
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/**************************************
 * end fbe_raid_group_post_clear_rekey_for_chunk()
 **************************************/
/*!****************************************************************************
 * fbe_raid_group_rekey_clear_degraded()
 ******************************************************************************
 * @brief
 *   This function clears the degraded bit in the rekey position.
 * 
 * @param packet_p           - pointer to the packet 
 * @param raid_group_p       - pointer to a raid group object
 *
 * @return  fbe_status_t  
 *
 * @author
 *  01/07/2014  Ron Proulx  - Created.
 *
 ******************************************************************************/
static fbe_status_t 
fbe_raid_group_rekey_clear_degraded_update_metadata_callback(fbe_packet_t * packet, fbe_metadata_callback_context_t context)
{
    fbe_raid_group_t                   *raid_group_p;
    fbe_raid_position_bitmask_t         degraded_mask;
    fbe_payload_ex_t                   *sep_payload = fbe_transport_get_payload_ex(packet);
    fbe_payload_metadata_operation_t   *mdo = NULL;
    fbe_raid_group_paged_metadata_t    *paged_metadata_p;
    fbe_sg_element_t                   *sg_list = NULL;
    fbe_sg_element_t                   *sg_ptr = NULL;
    fbe_lba_t                           lba_offset;
    fbe_u64_t                           slot_offset;

    /* Get the metadata operation
     */
    if(sep_payload->current_operation->payload_opcode == FBE_PAYLOAD_OPCODE_METADATA_OPERATION){
        mdo = fbe_payload_ex_get_metadata_operation(sep_payload);
    } else if(sep_payload->current_operation->payload_opcode == FBE_PAYLOAD_OPCODE_STRIPE_LOCK_OPERATION){
        mdo = fbe_payload_ex_get_any_metadata_operation(sep_payload);
    }
    if ((mdo == NULL) || (mdo->metadata_element == NULL))
    {
        /* The metadata code will complete a packet in error.
         */
        fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR,
                                 FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                 "%s NULL metadata operation\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Get the raid group object from the metadata element.
     */
    raid_group_p = (fbe_raid_group_t *)fbe_base_config_metadata_element_to_base_config_object(mdo->metadata_element);

    /* Clear out every position that was set except for the degraded positions.
     */
    fbe_raid_group_get_degraded_mask(raid_group_p, &degraded_mask);

    lba_offset = fbe_metadata_paged_get_lba_offset(mdo);
    slot_offset = fbe_metadata_paged_get_slot_offset(mdo);
    sg_list = mdo->u.metadata_callback.sg_list;
    sg_ptr = &sg_list[lba_offset - mdo->u.metadata_callback.start_lba];
    paged_metadata_p = (fbe_raid_group_paged_metadata_t *)(sg_ptr->address + slot_offset);

    /* For each chunk clear the NR bit for any position that is not degraded
     * and set the rekey bit to True.
     */
    while ((sg_ptr->address != NULL) && (mdo->u.metadata_callback.current_count_private < mdo->u.metadata_callback.repeat_count))
    {
        /* Set the NR mask for only those positions that are degraded.
         */
        paged_metadata_p->needs_rebuild_bits = degraded_mask;

        /* Flag the fact that the rekey is complete.
         */
        paged_metadata_p->rekey = 1;

        /* Perform required book-keeping
         */
        mdo->u.metadata_callback.current_count_private++;
        paged_metadata_p++;
        if (paged_metadata_p == (fbe_raid_group_paged_metadata_t *)(sg_ptr->address + FBE_METADATA_BLOCK_DATA_SIZE))
        {
            sg_ptr++;
            paged_metadata_p = (fbe_raid_group_paged_metadata_t *)sg_ptr->address;
        }
    }

    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/********************************************************************
 * end fbe_raid_group_rekey_clear_degraded_update_metadata_callback()
 ********************************************************************/
/*!****************************************************************************
 * fbe_raid_group_rekey_clear_degraded()
 ******************************************************************************
 * @brief
 *   This function clears the degraded bit in the rekey position.
 * 
 * @param packet_p           - pointer to the packet 
 * @param raid_group_p       - pointer to a raid group object
 *
 * @return  fbe_status_t  
 *
 * @author
 *  11/20/2013 - Created. Rob Foley
 *
 ******************************************************************************/
static fbe_status_t 
fbe_raid_group_rekey_clear_degraded(fbe_packet_t *packet_p,    
                                    fbe_raid_group_t *raid_group_p)
{
    fbe_status_t                    status;
    fbe_payload_ex_t               *sep_payload_p = NULL;
    fbe_payload_metadata_operation_t *metadata_operation_p;
    fbe_payload_block_operation_t  *block_operation_p;
    fbe_u64_t                       chunk_count;
    fbe_chunk_count_t               chunk_count_per_request;
    fbe_lba_t                       start_lba; 
    fbe_lba_t                       end_lba;
    fbe_block_count_t               block_count;
    fbe_chunk_index_t               start_chunk_index;
    fbe_chunk_index_t               end_chunk_index;    
    fbe_raid_iots_t*                iots_p;
    fbe_u16_t                       data_disks;
    fbe_u32_t                       width;
    fbe_raid_position_bitmask_t     degraded_mask;
    fbe_raid_position_bitmask_t     clear_mask;
    fbe_raid_geometry_t            *raid_geometry_p = fbe_raid_group_get_raid_geometry(raid_group_p);
    fbe_u64_t                       metadata_offset;

    sep_payload_p        = fbe_transport_get_payload_ex(packet_p);    
    block_operation_p = fbe_payload_ex_get_present_block_operation(sep_payload_p);
    fbe_payload_ex_get_iots(sep_payload_p, &iots_p);  

    start_lba = block_operation_p->lba;

    /* Since the iots block count may have been limited by the next marked extent,
     * use that to determine how many marks to clear. AR 536292
     */
    fbe_raid_iots_get_blocks(iots_p, &block_count);

    end_lba = start_lba + block_count - 1;
    
    fbe_raid_geometry_get_data_disks(raid_geometry_p, &data_disks);
    start_lba /= data_disks;
    end_lba /= data_disks;

    // Clear all the rekey bits in this chunk
    fbe_raid_group_bitmap_get_chunk_index_for_lba(raid_group_p, start_lba, &start_chunk_index);
    fbe_raid_group_bitmap_get_chunk_index_for_lba(raid_group_p, end_lba, &end_chunk_index);
    chunk_count = end_chunk_index - start_chunk_index + 1;
    
    /* For debug trace the positions being cleared
     */
    fbe_raid_geometry_get_width(raid_geometry_p, &width);
    fbe_raid_group_get_degraded_mask(raid_group_p, &degraded_mask);
    clear_mask = ((1 << width) - 1) & (~degraded_mask);

    fbe_raid_group_trace(raid_group_p, 
                         FBE_TRACE_LEVEL_INFO, 
                         FBE_RAID_GROUP_DEBUG_FLAGS_TRACE_PMD_NR,
                         "Encryption clear degraded chunk %llx to %llx clear mask: 0x%x deg: 0x%x\n", 
                         (unsigned long long)start_chunk_index, (unsigned long long)end_chunk_index, clear_mask, degraded_mask);

    /* This method does not support multiple requests.
     */
    fbe_raid_group_get_max_allowed_metadata_chunk_count(&chunk_count_per_request);
    if (chunk_count > chunk_count_per_request)
    {
        /* Fail the request and complete the packet.
         */
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "Encryption clr deg chunk_count: %llx greater than max: 0x%x \n",
                              chunk_count, chunk_count_per_request);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_OK;
    }

    // Setup the completion function to be called when the paged metadata write completes
    fbe_transport_set_completion_function(packet_p, 
                                          fbe_raid_group_post_clear_rekey_for_chunk,
                                          raid_group_p);

    /* Setup the completion for the original block operation.
     */
    fbe_transport_set_completion_function(packet_p, 
                                          fbe_raid_group_bitmap_paged_metadata_block_op_completion,
                                          raid_group_p); 
     
    /* The only purpose of the block operation is to keep track of request
     * that need to be split.  This method does not support a split but it
     * uses the same completion so we need to allocate a block operation.
     */
    block_operation_p = fbe_payload_ex_allocate_block_operation(sep_payload_p);
    fbe_payload_ex_increment_block_operation_level(sep_payload_p);

    /* Save the original block and LBA values */
    fbe_payload_block_set_lba(block_operation_p, start_chunk_index);
    fbe_payload_block_set_block_count(block_operation_p, chunk_count);

    /* Setup the updated paged metadata operation.
     */
    metadata_operation_p = fbe_payload_ex_allocate_metadata_operation(sep_payload_p);
    fbe_payload_ex_increment_metadata_operation_level(sep_payload_p);

    //  Set up the offset of this chunk in the metadata (ie. where we'll start to write to)
    metadata_offset = start_chunk_index * sizeof(fbe_raid_group_paged_metadata_t);

    /* Set the completion that handles the metadata operation completion.
     */
    fbe_transport_set_completion_function(packet_p, fbe_raid_group_bitmap_update_paged_metadata_completion,
                                          (fbe_packet_completion_context_t)raid_group_p);

    /* Build the paged metadata update request. */
    fbe_payload_metadata_build_paged_update(metadata_operation_p,
                                            &(((fbe_base_config_t *) raid_group_p)->metadata_element),
                                            metadata_offset,
                                            sizeof(fbe_raid_group_paged_metadata_t),
                                            chunk_count,
                                            fbe_raid_group_rekey_clear_degraded_update_metadata_callback,
                                            (void *)metadata_operation_p);
    fbe_transport_set_packet_attr(packet_p, FBE_PACKET_FLAG_DO_NOT_CANCEL);  

    /* Now populate the metadata strip lock information */
    status = fbe_raid_group_bitmap_metadata_populate_stripe_lock(raid_group_p,
                                                                 metadata_operation_p,
                                                                 (fbe_chunk_count_t)chunk_count);
    if (status != FBE_STATUS_OK)
    {
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_OK;
    }

    //  Issue the metadata operation to write the chunk info.  This function returns FBE_STATUS_OK if successful. 
    status = fbe_metadata_operation_entry(packet_p);

    return status; 
}
/**************************************
 * end fbe_raid_group_rekey_clear_degraded()
 **************************************/
/*!****************************************************************************
 * fbe_raid_rekey_update_metadata()
 ******************************************************************************
 * @brief
 *  This is the completion function for updating the rekey bits.
 *
 * @param packet_p - The packet sent to us from the scheduler.
 * @param context  - The pointer to the object.
 *
 * @return fbe_status_t
 *
 * @author
 *  10/28/2013 - Created. Rob Foley
 *
 *****************************************************************************/
fbe_status_t fbe_raid_rekey_update_metadata(fbe_packet_t* packet_p, 
                                            fbe_raid_group_t* raid_group_p)
{    
    
    fbe_payload_ex_t*                       payload_p = NULL;
    fbe_raid_iots_t*                        iots_p = NULL;  
    fbe_status_t                            status;
    fbe_payload_block_operation_t           *block_operation_p = NULL;
    fbe_payload_block_operation_status_t    block_status;
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_group_get_raid_geometry(raid_group_p);
    fbe_u16_t                               data_disks;
    fbe_lba_t                               start_lba;
    fbe_block_count_t                       blocks;

    // Get the payload and block operation for this I/O operation
    payload_p = fbe_transport_get_payload_ex(packet_p);
    fbe_payload_ex_get_iots(payload_p, &iots_p);  
    block_operation_p = fbe_raid_iots_get_block_operation(iots_p);
    fbe_raid_iots_get_block_status(iots_p, &block_status);
    status = fbe_transport_get_status_code(packet_p);

    if( (status == FBE_STATUS_OK) &&
        (block_status == FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS) )
    {
        if (fbe_raid_geometry_is_raid10(raid_geometry_p)) {
            /* RAID-10 does not use paged metadata.  Just update the checkpoint.
             */
            start_lba = block_operation_p->lba;
            fbe_raid_iots_get_blocks(iots_p, &blocks);

            fbe_raid_geometry_get_data_disks(raid_geometry_p, &data_disks);
            start_lba /= data_disks;
            blocks /= data_disks;

            status = fbe_raid_group_update_rekey_checkpoint(packet_p, raid_group_p, start_lba, blocks, __FUNCTION__);
        } else {
            fbe_raid_group_rekey_clear_degraded(packet_p, raid_group_p);
        }
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }

    // Operation failed.
    fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                            FBE_TRACE_LEVEL_WARNING,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                            "Encryption rekey update md operation failed, status: 0x%x block status: 0x%x\n",
                            status, block_status);

    fbe_transport_complete_packet(packet_p);
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}   
/**************************************
 * end fbe_raid_rekey_update_metadata
 **************************************/

/*!**************************************************************
 * fbe_raid_group_rekey_mark_deg_completion()
 ****************************************************************
 * @brief
 *  Completion for marking a chunk as needs rebuild during rekey.
 * 
 * @param packet_p   - packet that is completing
 * @param context    - completion context, which is a pointer to 
 *                        the raid group object
 *
 * @return fbe_status_t.
 *
 * @author
 *  11/18/2013 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_raid_group_rekey_mark_deg_completion(fbe_packet_t *packet_p,
                                                      fbe_packet_completion_context_t context)
{
    fbe_raid_group_t *raid_group_p = NULL;
    fbe_payload_ex_t *sep_payload_p = NULL; 
    fbe_raid_iots_t *iots_p = NULL;
    fbe_status_t status;
    raid_group_p = (fbe_raid_group_t*)context;

    /* Put the packet back on the termination queue since we had to take it off before reading the chunk info
     */
    fbe_raid_group_add_to_terminator_queue(raid_group_p, packet_p);
    status = fbe_transport_get_status_code(packet_p);

    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    fbe_payload_ex_get_iots(sep_payload_p, &iots_p);

    if (status == FBE_STATUS_OK) {
        /* Restart the waiting siots.
         */
        fbe_queue_head_t restart_queue;
        fbe_raid_siots_t *siots_p = NULL;
        fbe_queue_init(&restart_queue);
        siots_p = (fbe_raid_siots_t *)fbe_queue_front(&iots_p->siots_queue);
        fbe_queue_push(&restart_queue, &siots_p->common.wait_queue_element);
        fbe_raid_restart_common_queue(&restart_queue);
        fbe_queue_destroy(&restart_queue);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }
    /* We can return success since:
     *  o Either not I/O was performed
     *      OR
     *  o The packet status will be processed by another method
     */
    /* AR598712: we could not clear the error FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_TOO_MANY_DEAD_POSITIONS
       since fbe_raid_group_iots_finished would check that to set proper conditions so as to recover the full paged metadtata
     */
    if (!(iots_p->error == FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID_REQUEST && iots_p->qualifier == FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_TOO_MANY_DEAD_POSITIONS))
    {
        fbe_raid_iots_set_block_status(iots_p, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS);
        fbe_raid_iots_set_status(iots_p, FBE_RAID_IOTS_STATUS_COMPLETE);
    }

    fbe_raid_group_iots_cleanup(packet_p, raid_group_p);

    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/**********************************************************
 * end fbe_raid_group_rekey_mark_deg_completion()
 **********************************************************/
/*!****************************************************************************
 * fbe_raid_rekey_mark_degraded()
 ******************************************************************************
 * @brief
 *   This function marks degraded for the rekey position and persists it.
 * 
 * @param packet_p           - pointer to the packet 
 * @param raid_group_p       - pointer to a raid group object
 *
 * @return  fbe_status_t  
 *
 * @author
 *  11/18/2013 - Created. Rob Foley
 *
 ******************************************************************************/
fbe_status_t 
fbe_raid_rekey_mark_degraded(fbe_packet_t *packet_p,    
                                             fbe_raid_group_t *raid_group_p)
{
    fbe_raid_group_paged_metadata_t         paged_clear_bits;
    fbe_status_t                            status;
    fbe_payload_ex_t*                       sep_payload_p = NULL;
    fbe_payload_block_operation_opcode_t    block_opcode;
    fbe_chunk_count_t                       chunk_count;
    fbe_payload_block_operation_t           *block_operation_p;
    fbe_lba_t                               start_lba; 
    fbe_lba_t                               end_lba;
    fbe_block_count_t                       block_count;
    fbe_chunk_index_t                       start_chunk_index;
    fbe_chunk_index_t                       end_chunk_index;    
    fbe_raid_iots_t*                        iots_p;
    sep_payload_p        = fbe_transport_get_payload_ex(packet_p);    
    block_operation_p = fbe_payload_ex_get_present_block_operation(sep_payload_p);
    fbe_payload_ex_get_iots(sep_payload_p, &iots_p);  

    start_lba = block_operation_p->lba;
    block_opcode = block_operation_p->block_opcode;

    /* Since the iots block count may have been limited by the next marked extent,
     * use that to determine how many marks to clear. AR 536292
     */
    fbe_raid_iots_get_blocks(iots_p, &block_count);

    end_lba = start_lba + block_count - 1;

    fbe_raid_group_bitmap_get_chunk_index_for_lba(raid_group_p, start_lba, &start_chunk_index);
    fbe_raid_group_bitmap_get_chunk_index_for_lba(raid_group_p, end_lba, &end_chunk_index);    

    fbe_raid_group_trace(raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_RAID_GROUP_DEBUG_FLAGS_TRACE_PMD_VERIFY,
                         "set degraded rekey chunk %llx to %llx\n", 
                         (unsigned long long)start_chunk_index, (unsigned long long)end_chunk_index);

    // Setup the completion function to be called when the paged metadata write completes
    fbe_transport_set_completion_function(packet_p, 
                                          fbe_raid_group_bitmap_paged_metadata_block_op_completion,
                                          raid_group_p);  

    // Initialize the bit data with 0 before read operation 
    fbe_set_memory(&paged_clear_bits, 0, sizeof(fbe_raid_group_paged_metadata_t));

    chunk_count = (fbe_chunk_count_t)(end_chunk_index - start_chunk_index) + 1;

    //fbe_raid_group_encryption_rekey_get_pos(raid_group_p, &pos);

    //paged_clear_bits.needs_rebuild_bits = (1 << pos);

    /* We only clear if it is an even position, otherwise we set.
     */
    status = fbe_raid_group_bitmap_update_paged_metadata(raid_group_p,
                                                         packet_p,
                                                         start_chunk_index,
                                                         chunk_count,
                                                         &paged_clear_bits,
                                                         FBE_FALSE /* Not clear, set. */);
    return status; 
}
/**************************************
 * end fbe_raid_rekey_mark_degraded()
 **************************************/

/*!****************************************************************************
 * fbe_raid_group_encryption_inc_pos()
 ******************************************************************************
 * @brief
 *  This function marks NP and persists, after obtaining NP lock.
 *
 * @param packet_p - The pointer to the packet.
 * @param context  - The pointer to the object.
 *
 * @return fbe_status_t
 *
 * @author
 *  10/28/2013 - Created. Rob Foley
 *
 *****************************************************************************/
static fbe_status_t 
fbe_raid_group_encryption_inc_pos(fbe_packet_t * packet_p, 
                                  fbe_packet_completion_context_t context)
{
    fbe_raid_group_t *raid_group_p = (fbe_raid_group_t *)context;
    fbe_status_t status;         
    fbe_u64_t metadata_offset;                
    fbe_raid_group_encryption_nonpaged_info_t encryption_nonpaged_data;

    /* If the status is not ok, that means we didn't get the lock.
     */
    status = fbe_transport_get_status_code(packet_p);
    if (status != FBE_STATUS_OK) {
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "Encryption %s can't get the NP lock, status: 0x%X\n", __FUNCTION__, status); 
        return FBE_STATUS_OK;
    }

    fbe_base_object_trace((fbe_base_object_t *)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                          "Encryption  Completed drive pos   Increment NP to next drive. Fetched NP lock.\n");
    /* Reset our nonpaged to indicate we are just getting started with this next position.
     */
    encryption_nonpaged_data.rekey_checkpoint = 0;
    encryption_nonpaged_data.unused = 0;
    encryption_nonpaged_data.flags = FBE_RAID_GROUP_ENCRYPTION_FLAG_PAGED_ENCRYPTED;

    /* Push a completion on stack to release NP lock. */
    fbe_transport_set_completion_function(packet_p, fbe_raid_group_release_NP_lock, raid_group_p);

    metadata_offset = (fbe_u64_t) (&((fbe_raid_group_nonpaged_metadata_t*)0)->encryption);    

    status = fbe_base_config_metadata_nonpaged_write_persist((fbe_base_config_t*) raid_group_p, packet_p, 
                                                             metadata_offset, (fbe_u8_t*) &encryption_nonpaged_data, 
                                                             sizeof(encryption_nonpaged_data));
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/******************************************************************************
 * end fbe_raid_group_encryption_inc_pos()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_raid_group_rekey_done_update_np_completion()
 ******************************************************************************
 * @brief
 *  This is the completion routine for updating the nonpaged to indicate
 *  that rekey is done.
 *
 * @param packet_p - The pointer to the packet.
 * @param context  - The pointer to the object.
 *
 * @return fbe_status_t
 *
 * @author
 *  11/1/2013 - Created. Rob Foley
 *
 *****************************************************************************/
static fbe_status_t 
fbe_raid_group_rekey_done_update_np_completion(fbe_packet_t * packet_p, 
                                               fbe_packet_completion_context_t context)
{
    fbe_raid_group_t *raid_group_p = (fbe_raid_group_t *)context;
    fbe_status_t status;
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_group_get_raid_geometry(raid_group_p);
    fbe_base_config_encryption_state_t encryption_state;

    /* If the status is not ok, that means we didn't get the lock.
     */
    status = fbe_transport_get_status_code(packet_p);
    if (status != FBE_STATUS_OK) {
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                              "Encryption %s can't get the NP lock, status: 0x%X\n", __FUNCTION__, status); 
        return FBE_STATUS_OK;
    }

    /* Normally RAID-10 will not get the key pushes from the client.
     * Just transition the encryption state when encryption completes. 
     */
    fbe_base_config_get_encryption_state((fbe_base_config_t*)raid_group_p, &encryption_state);

    fbe_base_object_trace((fbe_base_object_t *)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                          "Encryption Completed np update enc state: %d\n", encryption_state);

    if (fbe_raid_geometry_is_raid10(raid_geometry_p) &&
        (encryption_state == FBE_BASE_CONFIG_ENCRYPTION_STATE_REKEY_COMPLETE)) {
        fbe_base_config_set_encryption_state((fbe_base_config_t *)raid_group_p, 
                                             FBE_BASE_CONFIG_ENCRYPTION_STATE_KEYS_PROVIDED);
    }
    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_raid_group_rekey_done_update_np_completion()
 **************************************/
/*!****************************************************************************
 * fbe_raid_group_encryption_complete_update_np()
 ******************************************************************************
 * @brief
 *  This function marks NP and persists, after obtaining NP lock.
 *
 * @param packet_p - The pointer to the packet.
 * @param context  - The pointer to the object.
 *
 * @return fbe_status_t
 *
 * @author
 *  10/28/2013 - Created. Rob Foley
 *
 *****************************************************************************/
static fbe_status_t 
fbe_raid_group_encryption_complete_update_np(fbe_packet_t * packet_p, 
                                             fbe_packet_completion_context_t context)
{
    fbe_raid_group_t *raid_group_p = (fbe_raid_group_t *)context;
    fbe_status_t status;         
    fbe_u64_t metadata_offset;                
    fbe_raid_group_encryption_nonpaged_info_t encryption_nonpaged_data;

    /* If the status is not ok, that means we didn't get the lock.
     */
    status = fbe_transport_get_status_code(packet_p);
    if (status != FBE_STATUS_OK) {
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "Encryption %s can't get the NP lock, status: 0x%X\n", __FUNCTION__, status); 
        return FBE_STATUS_OK;
    }

    fbe_base_object_trace((fbe_base_object_t *)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                          "Encryption Completed Encryption Mark NP. Fetched NP lock.\n");

    /* Reset our nonpaged to indicate we are not rekeying.
     */
    encryption_nonpaged_data.rekey_checkpoint = FBE_LBA_INVALID;
    encryption_nonpaged_data.unused = 0;
    encryption_nonpaged_data.flags = 0;

    /* Push a completion on stack to release NP lock. */
    fbe_transport_set_completion_function(packet_p, fbe_raid_group_release_NP_lock, raid_group_p);
    fbe_transport_set_completion_function(packet_p, fbe_raid_group_rekey_done_update_np_completion, raid_group_p);

    metadata_offset = (fbe_u64_t) (&((fbe_raid_group_nonpaged_metadata_t*)0)->encryption);    

    status = fbe_base_config_metadata_nonpaged_write_persist((fbe_base_config_t*) raid_group_p, packet_p, 
                                                             metadata_offset, (fbe_u8_t*) &encryption_nonpaged_data, 
                                                             sizeof(encryption_nonpaged_data));
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/******************************************************************************
 * end fbe_raid_group_encryption_complete_update_np()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_raid_group_should_read_paged_for_rekey()
 ******************************************************************************
 * @brief
 *  Decide if the paged needs to be read in to decide if we are rekeying.
 *
 * @param raid_group_p  - The pointer to the object.
 * @param packet_p - The packet for an I/O.
 *
 * @return fbe_status_t
 *
 * @author
 *  10/28/2013 - Created. Rob Foley
 *
 *****************************************************************************/
fbe_bool_t fbe_raid_group_should_read_paged_for_rekey(fbe_raid_group_t* raid_group_p,
                                                      fbe_packet_t* packet_p)
{
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_group_get_raid_geometry(raid_group_p);
    fbe_payload_ex_t *payload_p = fbe_transport_get_payload_ex(packet_p);
    fbe_raid_iots_t *iots_p = NULL;
    fbe_bool_t b_read_paged = FBE_FALSE;
    /* The times 2 is to account for the fact that we might not be aligned and 
     * might overlap into the next window area. 
     */
    fbe_block_count_t window_size = FBE_RAID_GROUP_PEER_CHECKPOINT_UPDATE_WINDOW * 2;
    fbe_lba_t rekey_checkpoint = fbe_raid_group_get_rekey_checkpoint(raid_group_p);
    fbe_lba_t lba;
    fbe_block_count_t blocks;
    fbe_u64_t stripe_number;
    fbe_u64_t stripe_count;
    fbe_payload_block_operation_opcode_t opcode;
    fbe_bool_t b_is_lba_disk_based;
    fbe_lba_t end_lba;
    fbe_lba_t paged_start_lba;
    fbe_u16_t data_disks;

    if (fbe_raid_geometry_is_raid10(raid_geometry_p)) {
        /* RAID-10 does not use the paged.
         */
        return FBE_FALSE;
    }

    /* Get the block operation information
     */
    fbe_payload_ex_get_iots(payload_p, &iots_p);
    fbe_raid_iots_get_lba(iots_p, &lba);
    fbe_raid_iots_get_blocks(iots_p, &blocks);
    fbe_raid_iots_get_opcode(iots_p, &opcode);

    if (opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ENCRYPTION_READ_PAGED) {
        /* We always read the paged for rekey.
         */
        return FBE_TRUE;
    }
    b_is_lba_disk_based = fbe_raid_library_is_opcode_lba_disk_based(opcode);
    
    /* Determine what we should be locking.
     */
    if (b_is_lba_disk_based) {
        /* These use a physical address.
         */
        fbe_raid_geometry_calculate_lock_range_physical(raid_geometry_p,
                                                        lba,
                                                        blocks,
                                                        &stripe_number,
                                                        &stripe_count);
    } else {
        /* These use a logical address.
         */
        fbe_raid_geometry_calculate_lock_range(raid_geometry_p,
                                               lba,
                                               blocks,
                                               &stripe_number,
                                               &stripe_count);
    }
    end_lba = stripe_number + stripe_count - 1;
    fbe_raid_geometry_get_data_disks(raid_geometry_p, &data_disks);
    fbe_raid_geometry_get_metadata_start_lba(raid_geometry_p, &paged_start_lba);
    paged_start_lba /= data_disks;

    if (end_lba > paged_start_lba) {
        /* Never read the paged for metadata I/Os, they are either completely encrypted or 
         * completely not encrypted. 
         */
        return FBE_FALSE;
    }
    /* We are required to quiesce in between positions. 
     * The rekey is also required to not progress beyond a certain window before shipping 
     * the checkpoint to the peer. 
     * Therefore we can make a more specific check and only read the paged if we are 
     * within the window where the rekey is operating. 
     */
    if (end_lba < rekey_checkpoint) {
        /* No need to read paged since we already rekeyed this lba.
         */
        return FBE_FALSE;
    }
    else if (end_lba > rekey_checkpoint + window_size) {
        /* No need to read paged since we are above the window.
         * Peer is guaranteed to send us the checkpoint within this window. 
         */
        return FBE_FALSE;
    }
    else {
        /* Yes read the paged since we are in the area where we might be rekeying.
         */
        return FBE_TRUE;
    }
    return b_read_paged;
}
/**************************************
 * end fbe_raid_group_should_read_paged_for_rekey()
 **************************************/


/*!****************************************************************************
 * fbe_raid_group_rekey_get_bitmap()
 ******************************************************************************
 * @brief
 *  Decide if the paged needs to be read in to decide if we are rekeying.
 *
 * @param raid_group_p  - The pointer to the object.
 * @param packet_p - The packet for an I/O.
 * #param bitmask_p - Output bitmask of positions to use new key on.
 *
 * @return fbe_status_t
 *
 * @author
 *  10/28/2013 - Created. Rob Foley
 *
 *****************************************************************************/
fbe_status_t fbe_raid_group_rekey_get_bitmap(fbe_raid_group_t* raid_group_p,
                                             fbe_packet_t* packet_p,
                                             fbe_raid_position_bitmask_t *bitmask_p)
{
    fbe_raid_iots_t *iots_p = NULL;
    fbe_payload_block_operation_t *block_operation_p = NULL;
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_group_get_raid_geometry(raid_group_p);
    fbe_payload_ex_t *payload_p = fbe_transport_get_payload_ex(packet_p);
    /* The times 2 is to account for the fact that we might not be aligned and 
     * might overlap into the next window area. 
     */
    fbe_block_count_t window_size = FBE_RAID_GROUP_PEER_CHECKPOINT_UPDATE_WINDOW * 2;
    fbe_lba_t rekey_checkpoint = fbe_raid_group_get_rekey_checkpoint(raid_group_p);
    fbe_lba_t lba;
    fbe_block_count_t blocks;
    fbe_u64_t stripe_number;
    fbe_u64_t stripe_count;
    fbe_payload_block_operation_opcode_t opcode;
    fbe_bool_t b_is_lba_disk_based;
    fbe_lba_t end_lba;
    fbe_lba_t paged_start_lba;
    fbe_u16_t data_disks;
    fbe_u32_t width;

    /* Get the block operation information
     */
    if (payload_p->current_operation->payload_opcode == FBE_PAYLOAD_OPCODE_STRIPE_LOCK_OPERATION) {
        /* In this case we have stripe locks.
         */
        block_operation_p = fbe_payload_ex_search_prev_block_operation(payload_p);
    } else {
        block_operation_p = fbe_payload_ex_get_block_operation(payload_p);
    }
    fbe_payload_ex_get_iots(payload_p, &iots_p);
    fbe_payload_block_get_opcode(block_operation_p, &opcode);
        
    if (opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_LOG_FLUSH){
        fbe_raid_iots_get_current_lba(iots_p, &lba);
        fbe_raid_iots_get_current_op_blocks(iots_p, &blocks);
        fbe_raid_iots_get_current_opcode(iots_p, &opcode);
    } else {
        fbe_payload_block_get_lba(block_operation_p, &lba);
        fbe_payload_block_get_block_count(block_operation_p, &blocks);
    }
     
    b_is_lba_disk_based = fbe_raid_library_is_opcode_lba_disk_based(opcode);

    fbe_raid_geometry_get_width(raid_geometry_p, &width);

    /* Add in the positions that already rekeyed.
     */
    *bitmask_p = 0;

    /* Determine what we should be locking.
     */
    if (b_is_lba_disk_based) {
        /* These use a physical address.
         */
        fbe_raid_geometry_calculate_lock_range_physical(raid_geometry_p,
                                                        lba,
                                                        blocks,
                                                        &stripe_number,
                                                        &stripe_count);
    } else {
        /* These use a logical address.
         */
        fbe_raid_geometry_calculate_lock_range(raid_geometry_p,
                                               lba,
                                               blocks,
                                               &stripe_number,
                                               &stripe_count);
    }
    end_lba = stripe_number + stripe_count - 1;
    fbe_raid_geometry_get_data_disks(raid_geometry_p, &data_disks);
    fbe_raid_geometry_get_metadata_start_lba(raid_geometry_p, &paged_start_lba);
    paged_start_lba /= data_disks;
    if (end_lba >= paged_start_lba) {
        /* Paged Metadata I/Os use rekey if the paged is either rekeyed or in progress.
         */
        if (fbe_raid_group_is_encryption_state_set(raid_group_p, FBE_RAID_GROUP_ENCRYPTION_FLAG_PAGED_ENCRYPTED) ||
            fbe_raid_group_is_encryption_state_set(raid_group_p, FBE_RAID_GROUP_ENCRYPTION_FLAG_PAGED_NEEDS_RECONSTRUCT)){
            *bitmask_p = (1 << width) - 1;
        }
    }
    else if (rekey_checkpoint == FBE_LBA_INVALID) {
        /* We're in the user area and the checkpoint is invalid. 
         * We are just starting encryption, so treat this as unencrypted.
         */
    }
    else if ((rekey_checkpoint == 0) &&
             !fbe_raid_group_is_encryption_state_set(raid_group_p, 
                                                     FBE_RAID_GROUP_ENCRYPTION_FLAG_PAGED_ENCRYPTED)) {
        /* Unencrypted.  We have not started encryption yet if the checkpoint is 0 and
         * our paged is not yet in use for encryption. 
         */
    }
    else if ((opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ENCRYPTION_READ_PAGED) ||
             (opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ENCRYPTION_REKEY_WRITE) ||
             (opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ENCRYPTION_REKEY_WRITE_ZEROS) ||
             (opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ENCRYPTION_REKEY_ZERO)) {
        /* We are a rekey.  Set the bitmask to indicate position being rekeyed now. 
         */
        *bitmask_p = (1 << width) - 1;
        fbe_raid_group_trace(raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_RAID_GROUP_DEBUG_FLAG_ENCRYPTION_TRACING,
                             "Enc bitmap: o/lba/b: %x/%llx/%llx c: %llx rk: %x bm: %x\n", 
                             opcode, lba, blocks, rekey_checkpoint, iots_p->chunk_info[0].rekey, *bitmask_p);
    }
    /* Next determine for user area if we should read the rekey drive with the old or new key.
     */
    else if (end_lba < rekey_checkpoint) {
        /* We are in the area that was already rekeyed.
         */
        *bitmask_p |= (1 << width) - 1;
    }
    else if (end_lba > rekey_checkpoint + window_size) {
        /*  We are in the area that was not yet rekeyed.  We should use the old key.  Don't add the bit.
         */
    }
    else {
        /* We are in the window that could be getting rekeyed, so use the paged value we read in.
         */
        if (iots_p->chunk_info[0].rekey) {
            /* If we are even and the rekey is SET, then treat this position as rekeyed.
             */
            *bitmask_p |= (1 << width) - 1;
        }
    }

    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_raid_group_rekey_get_bitmap()
 **************************************/

/*!**************************************************************
 * fbe_raid_group_continue_encryption()
 ****************************************************************
 * @brief
 *  Under quiesce, perform various operations to transition
 *  the state of rekey.
 *
 * @param raid_group_p - Current RG.
 * @param packet_p - Current monitor packet.
 *
 * @return fbe_lifecycle_status_t  
 *
 * @author
 *  10/29/2013 - Created. Rob Foley
 *
 ****************************************************************/

fbe_lifecycle_status_t 
fbe_raid_group_continue_encryption(fbe_raid_group_t *raid_group_p, fbe_packet_t* packet_p)
{
    fbe_lba_t checkpoint;
    fbe_lba_t exported_capacity;
    fbe_base_config_encryption_state_t state;
    fbe_raid_group_encryption_flags_t flags;
    fbe_bool_t b_can_complete;
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_group_get_raid_geometry(raid_group_p);
    fbe_object_id_t object_id;
    fbe_base_config_encryption_mode_t mode;

    fbe_base_config_get_encryption_mode((fbe_base_config_t*)raid_group_p, &mode);
    fbe_raid_group_encryption_get_state(raid_group_p, &flags);

    fbe_base_config_get_encryption_state((fbe_base_config_t*)raid_group_p, &state);
    if ((state == FBE_BASE_CONFIG_ENCRYPTION_STATE_KEYS_PROVIDED) ||
        (state == FBE_BASE_CONFIG_ENCRYPTION_STATE_KEYS_PROVIDED_FOR_REKEY)) {
        fbe_base_config_set_encryption_state((fbe_base_config_t*)raid_group_p, FBE_BASE_CONFIG_ENCRYPTION_STATE_ENCRYPTION_IN_PROGRESS);
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "Encryption: KEYS_PROVIDED -> ENCRYPTION_IN_PROGRESS cp: %llx fl: 0x%x md: %d\n",
                              (unsigned long long)fbe_raid_group_get_rekey_checkpoint(raid_group_p), flags, mode);
    }

    /* R10 does not notify PVD.
     */
    if (!fbe_raid_geometry_is_raid10(raid_geometry_p)) {

        fbe_base_object_get_object_id((fbe_base_object_t *)raid_group_p, &object_id);
        if(fbe_database_is_object_system_rg(object_id) &&
           (flags < FBE_RAID_GROUP_ENCRYPTION_FLAG_PAGED_ENCRYPTED)){
            fbe_raid_group_reinitialize_vault(raid_group_p, packet_p);
            return FBE_LIFECYCLE_STATUS_PENDING;
        }

        if (flags < FBE_RAID_GROUP_ENCRYPTION_FLAG_PVD_NOTIFIED) {
            fbe_transport_set_completion_function(packet_p, fbe_raid_group_notify_pvd_encryption_completion, raid_group_p);
            fbe_raid_group_notify_pvd_encryption_start(raid_group_p, packet_p);
            return FBE_LIFECYCLE_STATUS_PENDING;
        }
        else if (flags < FBE_RAID_GROUP_ENCRYPTION_FLAG_PAGED_ENCRYPTED) {
            fbe_raid_group_encrypt_paged(raid_group_p, packet_p);
            return FBE_LIFECYCLE_STATUS_PENDING;
        }
    }

    checkpoint = fbe_raid_group_get_rekey_checkpoint(raid_group_p);
    exported_capacity = fbe_raid_group_get_exported_disk_capacity(raid_group_p);

    /* Check to see if we are done with this position or done in general.
     */
    if (checkpoint >= exported_capacity) {
        if (fbe_raid_geometry_is_raid10(raid_geometry_p)) {
            /* RAID-10 does the encryption of paged last. 
             * But first it needs to notify the mirrors to push out their checkpoint past the paged 
             * so that when we do write the paged it will be written with the new key. 
             */
            if (flags < FBE_RAID_GROUP_ENCRYPTION_FLAG_PVD_NOTIFIED) {
                fbe_transport_set_completion_function(packet_p, fbe_raid_group_notify_pvd_encryption_completion, raid_group_p);
                fbe_raid_group_notify_lower_encryption_start(raid_group_p, packet_p);
                return FBE_LIFECYCLE_STATUS_PENDING;
            }
            else if (flags < FBE_RAID_GROUP_ENCRYPTION_FLAG_PAGED_ENCRYPTED) {
                fbe_raid_group_encrypt_paged(raid_group_p, packet_p);
                return FBE_LIFECYCLE_STATUS_PENDING;
            }
        }
        /* Simply set our condition to quiesce and continue with encryption.
         */
        fbe_raid_group_encryption_can_complete(raid_group_p, &b_can_complete);
        if (b_can_complete) {
            /* Done with all drives.  Keys were removed so we can complete.
             */
            fbe_raid_group_get_NP_lock(raid_group_p, packet_p, fbe_raid_group_encryption_complete_update_np);
        }
        /* Otherwise we wait for keys to be removed. 
         */
        return FBE_LIFECYCLE_STATUS_PENDING;
    }
    /* Nothing is needed, complete packet.
     */
    fbe_transport_complete_packet(packet_p);
    return FBE_LIFECYCLE_STATUS_PENDING;
}
/******************************************
 * end fbe_raid_group_continue_encryption()
 ******************************************/
/*!****************************************************************************
 * fbe_raid_group_notify_pvd_encryption_start()
 ******************************************************************************
 * @brief
 *  Notify PVD of encryption.
 *
 * @param raid_group_p           - pointer to a base object 
 * @param packet_p               - pointer to a control packet from the scheduler
 *
 * @return fbe_lifecycle_status_t   - FBE_LIFECYCLE_STATUS_RESCHEDULE on success
 *                                  - FBE_LIFECYCLE_STATUS_PENDING when I/O is
 *                                    being issued 
 *                                  - FBE_LIFECYCLE_STATUS_DONE  when operation done
 *                                    without I/O is being issued
 * @author
 *  12/11/2013 - Created. Rob Foley
 *
 ******************************************************************************/
fbe_status_t 
fbe_raid_group_notify_pvd_encryption_start(fbe_raid_group_t *raid_group_p,
                                           fbe_packet_t *packet_p) 
{
    fbe_status_t status;
    fbe_u32_t width;
    fbe_base_config_memory_allocation_chunks_t mem_alloc_chunks;

    /* get the width of the raid group object. */
    fbe_base_config_get_width((fbe_base_config_t *) raid_group_p, &width);

    /* We need to send the request down to all the edges and so get the width and allocate
     * packet for every edge */
    /* Set the various counts and buffer size */
    mem_alloc_chunks.buffer_count = 0;
    mem_alloc_chunks.number_of_packets = width;
    mem_alloc_chunks.sg_element_count = 0;
    mem_alloc_chunks.buffer_size = 0;
    mem_alloc_chunks.alloc_pre_read_desc = FALSE;
    mem_alloc_chunks.pre_read_desc_count = 0;

    /* allocate the subpackets to collect the zero checkpoint for all the edges. */
    status = fbe_base_config_memory_allocate_chunks((fbe_base_config_t *) raid_group_p,
                                                    packet_p,
                                                    mem_alloc_chunks,
                                                    (fbe_memory_completion_function_t)fbe_raid_group_notify_pvd_encr_alloc_complete);    /* SAFEBUG - cast to supress warning but must fix because memory completion function shouldn't return status */
    if (status != FBE_STATUS_OK) {
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s memory allocation failed, status:0x%x\n", __FUNCTION__, status);
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return status; 
    }
    return status;
}
/**************************************
 * end fbe_raid_group_notify_pvd_encryption_start
 **************************************/

/*!****************************************************************************
 * fbe_raid_group_notify_pvd_encr_alloc_complete()
 ******************************************************************************
 * @brief
 *  This function gets called after memory has been allocated and sends the 
 *  register request down to the PVD
 *
 * @param memory_request_p  - memory request.
 * @param context           - context passed to acquire the memory.
 *
 * @return status           - status of the operation.
 *
 * @author
 *  10/30/2013 - Created. Rob Foley
 *
 ******************************************************************************/
static fbe_status_t
fbe_raid_group_notify_pvd_encr_alloc_complete(fbe_memory_request_t * memory_request_p,
                                              fbe_memory_completion_context_t context) 
{
    fbe_packet_t *                                  packet_p = NULL;
    fbe_packet_t **                                 new_packet_p = NULL;
    fbe_payload_ex_t *                              sep_payload_p = NULL;
    fbe_payload_ex_t *                              new_sep_payload_p = NULL;
    fbe_payload_control_operation_t *               control_operation_p = NULL;
    fbe_payload_control_operation_t *               new_control_operation_p = NULL;
    fbe_status_t                                    status = FBE_STATUS_OK;
    fbe_raid_group_t *                              raid_group_p = NULL;
    fbe_u32_t                                       width = 0;
    fbe_u32_t                                       index = 0;
    fbe_block_edge_t *                              block_edge_p = NULL;
    fbe_base_config_memory_alloc_chunks_address_t   mem_alloc_chunks_addr = {0};
    fbe_sg_element_t                               *pre_read_sg_list_p = NULL;
    fbe_payload_pre_read_descriptor_t              *pre_read_desc_p = NULL;
    fbe_packet_attr_t                               packet_attr;

    /* get the pointer to original packet. */
    packet_p = fbe_transport_memory_request_to_packet(memory_request_p);

    /* get the pointer to raid group object. */
    raid_group_p = (fbe_raid_group_t *) context;

    /* get the payload and metadata operation. */
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p =  fbe_payload_ex_get_control_operation(sep_payload_p);

    /* Check allocation status */
    if (fbe_memory_request_is_allocation_complete(memory_request_p) == FBE_FALSE) {
        /* Currently this callback doesn't expect any aborted requests */
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s: memory request: 0x%p failed. request state: %d \n",
                              __FUNCTION__, memory_request_p, memory_request_p->request_state);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_FAILED;
    }

    /* get the width of the base config object. */
    fbe_base_config_get_width((fbe_base_config_t *) raid_group_p, &width);

    /* subpacket control buffer size. */
    mem_alloc_chunks_addr.buffer_size = 0;
    mem_alloc_chunks_addr.number_of_packets = width;
    mem_alloc_chunks_addr.buffer_count = 0;
    mem_alloc_chunks_addr.sg_element_count = 0;
    mem_alloc_chunks_addr.sg_list_p = NULL;
    mem_alloc_chunks_addr.assign_pre_read_desc = FALSE;
    mem_alloc_chunks_addr.pre_read_sg_list_p = &pre_read_sg_list_p;
    mem_alloc_chunks_addr.pre_read_desc_p = &pre_read_desc_p;

    /* get subpacket pointer from the allocated memory. */
    status = fbe_base_config_memory_assign_address_from_allocated_chunks((fbe_base_config_t *) raid_group_p,
                                                                         memory_request_p,
                                                                         &mem_alloc_chunks_addr);
    if (status != FBE_STATUS_OK) {
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s memory distribution failed, status:0x%x, width:0x%x, buffer_size:0x%x\n",
                              __FUNCTION__, status, width, mem_alloc_chunks_addr.buffer_size);
        fbe_memory_free_request_entry(memory_request_p);

        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_INSUFFICIENT_RESOURCES);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return status;
    }

    /* Set good status.  We will set bad status on each individual bad completion.
     */
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_OK);

    new_packet_p = mem_alloc_chunks_addr.packet_array_p;
    while (index < width) {
        /* Initialize the sub packets. */
        fbe_transport_initialize_sep_packet(new_packet_p[index]);
        new_sep_payload_p = fbe_transport_get_payload_ex(new_packet_p[index]);

        /* allocate and initialize the control operation. */
        new_control_operation_p = fbe_payload_ex_allocate_control_operation(new_sep_payload_p);
        fbe_payload_control_build_operation(new_control_operation_p,
                                            FBE_PROVISION_DRIVE_CONTROL_CODE_RECONSTRUCT_PAGED,
                                            NULL,
                                            0);

        /* Initialize the new packet with FBE_PACKET_FLAG_EXTERNAL same as the master packet */
        fbe_transport_get_packet_attr(packet_p, &packet_attr);
        if (packet_attr & FBE_PACKET_FLAG_EXTERNAL) {
            fbe_transport_set_packet_attr(new_packet_p[index], FBE_PACKET_FLAG_EXTERNAL);            
        }

        /* set completion functon. */
        fbe_transport_set_completion_function(new_packet_p[index],
                                              fbe_raid_group_notify_pvd_encryption_subpacket_completion,
                                              raid_group_p);

        /* add the subpacket to master packet queue. */
        fbe_transport_add_subpacket(packet_p, new_packet_p[index]);
        index++;
    }

    /* send all the subpackets together. */
    for (index = 0; index < width; index++) {
        fbe_base_config_get_block_edge((fbe_base_config_t *)raid_group_p, &block_edge_p, index);
        fbe_transport_set_packet_attr(new_packet_p[index], FBE_PACKET_FLAG_TRAVERSE);
        fbe_block_transport_send_control_packet(block_edge_p, new_packet_p[index]);
    }

    /* send all the subpackets. */
     return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/******************************************************************************
 * end fbe_raid_group_notify_pvd_encr_alloc_complete()
 ******************************************************************************/
/*!****************************************************************************
 * fbe_raid_group_notify_pvd_encryption_subpacket_completion()
 ******************************************************************************
 * @brief
 *  This function is used to handle the subpacket completion for the registration
 *  function for all the downstream objects.
 *
 * @param memory_request_p  - memory request.
 * @param context           - context passed to acquire the memory.
 *
 * @return status           - status of the operation.
 *
 * @author
 *  10/30/2013 - Created. Rob Foley
 *
 ******************************************************************************/
static fbe_status_t 
fbe_raid_group_notify_pvd_encryption_subpacket_completion(fbe_packet_t * packet_p,
                                                          fbe_packet_completion_context_t context)
{
    fbe_raid_group_t *                                          raid_group_p = NULL;
    fbe_packet_t *                                              master_packet_p = NULL;
    fbe_payload_ex_t *                                         master_sep_payload_p = NULL;
    fbe_payload_control_operation_t *                           master_control_operation_p = NULL;
    fbe_payload_ex_t *                                         sep_payload_p = NULL;
    fbe_payload_control_operation_t *                           control_operation_p = NULL;
    fbe_payload_control_status_t                                control_status;
    fbe_payload_control_status_qualifier_t                      control_qualifier;
    fbe_status_t                                                status;
    fbe_bool_t is_empty;

    /* Cast the context to base config object pointer. */
    raid_group_p = (fbe_raid_group_t *) context;

    /* Get the master packet, its payload and associated control operation. */
    master_packet_p = fbe_transport_get_master_packet(packet_p);
    master_sep_payload_p = fbe_transport_get_payload_ex(master_packet_p);
    master_control_operation_p = fbe_payload_ex_get_control_operation(master_sep_payload_p);

    /* get the control operation of the subpacket. */
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(sep_payload_p);

    /* get the status of the subpacket operation. */
    status = fbe_transport_get_status_code(packet_p);
    fbe_payload_control_get_status(control_operation_p, &control_status);
    fbe_payload_control_get_status_qualifier(control_operation_p, &control_qualifier);

    /* if any of the subpacket failed with bad status then update the master status. */
    if (status != FBE_STATUS_OK) {
        fbe_transport_set_status(master_packet_p, status, 0);
    }

    /* if any of the subpacket failed with failure status then update the master status. */
    if (control_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_payload_control_set_status(master_control_operation_p, control_status);
        fbe_payload_control_set_status_qualifier(master_control_operation_p, control_qualifier);
        fbe_transport_set_status(master_packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
    }

    /* remove the sub packet from master queue. */
    //fbe_transport_remove_subpacket(packet_p);
    fbe_transport_remove_subpacket_is_queue_empty(packet_p, &is_empty);

    /* when the queue is empty, all subpackets have completed. */
    if (is_empty) {
        fbe_transport_destroy_subpackets(master_packet_p);
        status = fbe_transport_get_status_code(master_packet_p);
        fbe_payload_control_get_status(master_control_operation_p, &control_status);

        if ((status == FBE_STATUS_OK) && (control_status == FBE_PAYLOAD_CONTROL_STATUS_OK)) {
            /* We need to notify VD after notifying PVD */
            fbe_raid_group_notify_vd_encryption_start(raid_group_p, master_packet_p);
        } else {
            fbe_memory_free_request_entry(&master_packet_p->memory_request);
            /* When we start status is set to good.  When we get each completion we set bad status on failure.
             */
            fbe_transport_complete_packet(master_packet_p);
        }
    } else {
        /* Not done with processing sub packets.
         */
    }
    return FBE_STATUS_MORE_PROCESSING_REQUIRED; 
}
/**************************************
 * end fbe_raid_group_notify_pvd_encryption_subpacket_completion()
 **************************************/

/*!****************************************************************************
 * fbe_raid_group_notify_vd_encryption_start()
 ******************************************************************************
 * @brief
 *  This function gets called after memory has been allocated and sends the 
 *  register request down to the PVD
 *
 * @param raid_group_p      - pointer to a base object 
 * @param packet_p          - pointer to a control packet from the scheduler
 *
 * @return status           - status of the operation.
 *
 * @author
 *  02/03/2013 - Created. Lili Chen
 *
 ******************************************************************************/
static fbe_status_t
fbe_raid_group_notify_vd_encryption_start(fbe_raid_group_t *raid_group_p,
                                          fbe_packet_t *packet_p) 
{
    fbe_packet_t **                                 new_packet_p = NULL;
    fbe_payload_ex_t *                              sep_payload_p = NULL;
    fbe_payload_ex_t *                              new_sep_payload_p = NULL;
    fbe_payload_control_operation_t *               control_operation_p = NULL;
    fbe_payload_control_operation_t *               new_control_operation_p = NULL;
    fbe_status_t                                    status = FBE_STATUS_OK;
    fbe_u32_t                                       width = 0;
    fbe_u32_t                                       index = 0;
    fbe_block_edge_t *                              block_edge_p = NULL;
    fbe_base_config_memory_alloc_chunks_address_t   mem_alloc_chunks_addr = {0};
    fbe_sg_element_t                               *pre_read_sg_list_p = NULL;
    fbe_payload_pre_read_descriptor_t              *pre_read_desc_p = NULL;
    fbe_packet_attr_t                               packet_attr;
    fbe_memory_request_t *                          memory_request_p = NULL; 

    memory_request_p = fbe_transport_get_memory_request(packet_p);

    /* get the payload and metadata operation. */
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p =  fbe_payload_ex_get_control_operation(sep_payload_p);

    /* get the width of the base config object. */
    fbe_base_config_get_width((fbe_base_config_t *) raid_group_p, &width);

    /* subpacket control buffer size. */
    mem_alloc_chunks_addr.buffer_size = 0;
    mem_alloc_chunks_addr.number_of_packets = width;
    mem_alloc_chunks_addr.buffer_count = 0;
    mem_alloc_chunks_addr.sg_element_count = 0;
    mem_alloc_chunks_addr.sg_list_p = NULL;
    mem_alloc_chunks_addr.assign_pre_read_desc = FALSE;
    mem_alloc_chunks_addr.pre_read_sg_list_p = &pre_read_sg_list_p;
    mem_alloc_chunks_addr.pre_read_desc_p = &pre_read_desc_p;

    /* get subpacket pointer from the allocated memory. */
    status = fbe_base_config_memory_assign_address_from_allocated_chunks((fbe_base_config_t *) raid_group_p,
                                                                         memory_request_p,
                                                                         &mem_alloc_chunks_addr);
    if (status != FBE_STATUS_OK) {
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s memory distribution failed, status:0x%x, width:0x%x, buffer_size:0x%x\n",
                              __FUNCTION__, status, width, mem_alloc_chunks_addr.buffer_size);
        fbe_memory_free_request_entry(memory_request_p);

        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_INSUFFICIENT_RESOURCES);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return status;
    }

    /* Set good status.  We will set bad status on each individual bad completion.
     */
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_OK);

    new_packet_p = mem_alloc_chunks_addr.packet_array_p;
    while (index < width) {
        /* Initialize the sub packets. */
        fbe_transport_initialize_sep_packet(new_packet_p[index]);
        new_sep_payload_p = fbe_transport_get_payload_ex(new_packet_p[index]);

        /* allocate and initialize the control operation. */
        new_control_operation_p = fbe_payload_ex_allocate_control_operation(new_sep_payload_p);
        fbe_payload_control_build_operation(new_control_operation_p,
                                            FBE_VIRTUAL_DRIVE_CONTROL_CODE_ENCRYPTION_START,
                                            NULL,
                                            0);

        /* Initialize the new packet with FBE_PACKET_FLAG_EXTERNAL same as the master packet */
        fbe_transport_get_packet_attr(packet_p, &packet_attr);
        if (packet_attr & FBE_PACKET_FLAG_EXTERNAL) {
            fbe_transport_set_packet_attr(new_packet_p[index], FBE_PACKET_FLAG_EXTERNAL);            
        }

        /* set completion functon. */
        fbe_transport_set_completion_function(new_packet_p[index],
                                              fbe_raid_group_notify_vd_encryption_subpacket_completion,
                                              raid_group_p);

        /* add the subpacket to master packet queue. */
        fbe_transport_add_subpacket(packet_p, new_packet_p[index]);
        index++;
    }

    /* send all the subpackets together. */
    for (index = 0; index < width; index++) {
        fbe_base_config_get_block_edge((fbe_base_config_t *)raid_group_p, &block_edge_p, index);
        fbe_transport_set_packet_attr(new_packet_p[index], FBE_PACKET_FLAG_TRAVERSE);
        fbe_block_transport_send_control_packet(block_edge_p, new_packet_p[index]);
    }

    /* send all the subpackets. */
     return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/******************************************************************************
 * end fbe_raid_group_notify_vd_encryption_start()
 ******************************************************************************/
/*!****************************************************************************
 * fbe_raid_group_notify_vd_encryption_subpacket_completion()
 ******************************************************************************
 * @brief
 *  This function is used to handle the subpacket completion for the registration
 *  function for all the downstream objects.
 *
 * @param memory_request_p  - memory request.
 * @param context           - context passed to acquire the memory.
 *
 * @return status           - status of the operation.
 *
 * @author
 *  02/03/2013 - Created. Lili Chen
 *
 ******************************************************************************/
static fbe_status_t 
fbe_raid_group_notify_vd_encryption_subpacket_completion(fbe_packet_t * packet_p,
                                                         fbe_packet_completion_context_t context)
{
    fbe_raid_group_t *                                          raid_group_p = NULL;
    fbe_packet_t *                                              master_packet_p = NULL;
    fbe_payload_ex_t *                                         master_sep_payload_p = NULL;
    fbe_payload_control_operation_t *                           master_control_operation_p = NULL;
    fbe_payload_ex_t *                                         sep_payload_p = NULL;
    fbe_payload_control_operation_t *                           control_operation_p = NULL;
    fbe_payload_control_status_t                                control_status;
    fbe_payload_control_status_qualifier_t                      control_qualifier;
    fbe_status_t                                                status;
    fbe_bool_t is_empty;

    /* Cast the context to base config object pointer. */
    raid_group_p = (fbe_raid_group_t *) context;

    /* Get the master packet, its payload and associated control operation. */
    master_packet_p = fbe_transport_get_master_packet(packet_p);
    master_sep_payload_p = fbe_transport_get_payload_ex(master_packet_p);
    master_control_operation_p = fbe_payload_ex_get_control_operation(master_sep_payload_p);

    /* get the control operation of the subpacket. */
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(sep_payload_p);

    /* get the status of the subpacket operation. */
    status = fbe_transport_get_status_code(packet_p);
    fbe_payload_control_get_status(control_operation_p, &control_status);
    fbe_payload_control_get_status_qualifier(control_operation_p, &control_qualifier);

    /* if any of the subpacket failed with bad status then update the master status. */
    if (status != FBE_STATUS_OK) {
        fbe_transport_set_status(master_packet_p, status, 0);
    }

    /* if any of the subpacket failed with failure status then update the master status. */
    if (control_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_payload_control_set_status(master_control_operation_p, control_status);
        fbe_payload_control_set_status_qualifier(master_control_operation_p, control_qualifier);
        fbe_transport_set_status(master_packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
    }

    /* remove the sub packet from master queue. */
    //fbe_transport_remove_subpacket(packet_p);
    fbe_transport_remove_subpacket_is_queue_empty(packet_p, &is_empty);

    /* when the queue is empty, all subpackets have completed. */
    if (is_empty) {
        fbe_transport_destroy_subpackets(master_packet_p);
        fbe_memory_free_request_entry(&master_packet_p->memory_request);
        /* When we start status is set to good.  When we get each completion we set bad status on failure.
         */
        status = fbe_transport_get_status_code(master_packet_p);
        fbe_transport_complete_packet(master_packet_p);
    } else {
        /* Not done with processing sub packets.
         */
    }
    return FBE_STATUS_MORE_PROCESSING_REQUIRED; 
}
/**************************************
 * end fbe_raid_group_notify_vd_encryption_subpacket_completion()
 **************************************/

/*!****************************************************************************
 * fbe_raid_group_notify_pvd_encryption_completion()
 ******************************************************************************
 * @brief
 *  The notification of the PVD is complete.  Check status and continue.
 *
 * @param packet_p
 * @param context - this is the raid group object ptr.
 *
 * @return status - status of the operation.
 *
 * @author
 *  11/18/2013 - Created. Rob Foley
 *
 ******************************************************************************/
static fbe_status_t 
fbe_raid_group_notify_pvd_encryption_completion(fbe_packet_t * packet_p,
                                                fbe_packet_completion_context_t context)
{
    fbe_raid_group_t *raid_group_p = (fbe_raid_group_t *)context;
    fbe_status_t status;

    /* If the status is not ok, that means we didn't get the lock.
     */
    status = fbe_transport_get_status_code(packet_p);
    if (status != FBE_STATUS_OK) {
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "Encryption failed notifying PVD, status: 0x%X\n", status); 
        return FBE_STATUS_OK;
    }

    fbe_base_object_trace((fbe_base_object_t *)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                          "Mark PVD notified, fetch NP lock.\n");
    fbe_raid_group_get_NP_lock(raid_group_p, packet_p, fbe_raid_group_encryption_mark_pvd_notified);
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/**************************************
 * end fbe_raid_group_notify_pvd_encryption_completion()
 **************************************/

/*!****************************************************************************
 * fbe_raid_group_encryption_mark_pvd_notified()
 ******************************************************************************
 * @brief
 *  This function marks NP and persists, after obtaining NP lock.
 *  We are updating the np in order to mark that the PVD already was notified.
 *
 * @param packet_p - The pointer to the packet.
 * @param context  - The pointer to the object.
 *
 * @return fbe_status_t
 *
 * @author
 *  10/30/2013 - Created. Rob Foley
 *
 *****************************************************************************/
static fbe_status_t 
fbe_raid_group_encryption_mark_pvd_notified(fbe_packet_t * packet_p, 
                                            fbe_packet_completion_context_t context)
{
    fbe_raid_group_t *raid_group_p = (fbe_raid_group_t *)context;
    fbe_status_t status;         
    fbe_u64_t metadata_offset;
    fbe_raid_group_encryption_flags_t flags = FBE_RAID_GROUP_ENCRYPTION_FLAG_PVD_NOTIFIED;

    /* If the status is not ok, that means we didn't get the lock.
     */
    status = fbe_transport_get_status_code(packet_p);
    if (status != FBE_STATUS_OK) {
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "Encryption %s can't get the NP lock, status: 0x%X\n", __FUNCTION__, status); 
        return FBE_STATUS_OK;
    }

    fbe_base_object_trace((fbe_base_object_t *)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                          "Encryption Mark pvd notified Fetched NP lock.\n");

    /* Push a completion on stack to release NP lock. */
    fbe_transport_set_completion_function(packet_p, fbe_raid_group_release_NP_lock, raid_group_p);

    /* Mark PVD notified and setup other fields also in paged.
     */
    metadata_offset = (fbe_u64_t) (&((fbe_raid_group_nonpaged_metadata_t*)0)->encryption.flags); 

    status = fbe_base_config_metadata_nonpaged_write_persist((fbe_base_config_t*) raid_group_p, packet_p, 
                                                             metadata_offset, (fbe_u8_t*) &flags, 
                                                             sizeof(fbe_raid_group_encryption_flags_t));
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/******************************************************************************
 * end fbe_raid_group_encryption_mark_pvd_notified()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_raid_group_rekey_set_chkpt_ds()
 ******************************************************************************
 * @brief
 *  Notify Lower RAID Group to set the checkpoint.
 *
 * @param raid_group_p           - pointer to a base object 
 * @param packet_p               - pointer to a control packet from the scheduler
 *
 * @return fbe_status_t
 * 
 * @author
 *  12/12/2013 - Created. Rob Foley
 *
 ******************************************************************************/
static fbe_status_t 
fbe_raid_group_rekey_set_chkpt_ds(fbe_raid_group_t *raid_group_p,
                                  fbe_packet_t *packet_p,
                                  fbe_lba_t lba,
                                  fbe_block_count_t blocks) 
{
    fbe_status_t status;
    fbe_u32_t width;
    fbe_base_config_memory_allocation_chunks_t mem_alloc_chunks;
    fbe_payload_ex_t *payload_p = NULL;
    fbe_payload_block_operation_t *block_operation_p = NULL;


    /* Allocate a block operation to track the set chkpt.
     */
    payload_p = fbe_transport_get_payload_ex(packet_p);
    block_operation_p = fbe_payload_ex_allocate_block_operation(payload_p);
    fbe_payload_block_build_operation(block_operation_p,
                                      FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ENCRYPTION_SET_CHKPT,
                                      lba,
                                      blocks,
                                      FBE_BE_BYTES_PER_BLOCK,
                                      1,
                                      NULL);
    fbe_payload_ex_increment_block_operation_level(payload_p);

    /* get the width of the raid group object. */
    fbe_base_config_get_width((fbe_base_config_t *) raid_group_p, &width);

    /* We need to send the request down to all the edges and so get the width and allocate
     * packet for every edge */
    /* Set the various counts and buffer size */
    fbe_zero_memory(&mem_alloc_chunks, sizeof(fbe_base_config_memory_allocation_chunks_t));
    mem_alloc_chunks.buffer_count = 0;
    mem_alloc_chunks.number_of_packets = width;
    mem_alloc_chunks.sg_element_count = 0;
    mem_alloc_chunks.buffer_size = 0;
    mem_alloc_chunks.alloc_pre_read_desc = FALSE;

    /* allocate the subpackets to collect the zero checkpoint for all the edges. */
    status = fbe_base_config_memory_allocate_chunks((fbe_base_config_t *) raid_group_p,
                                                    packet_p,
                                                    mem_alloc_chunks,
                                                    (fbe_memory_completion_function_t)fbe_raid_group_notify_lower_encr_alloc_complete);
    if (status != FBE_STATUS_OK) {
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s memory allocation failed, status:0x%x\n", __FUNCTION__, status);
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return status; 
    }
    return status;
}
/**************************************
 * end fbe_raid_group_rekey_set_chkpt_ds
 **************************************/
/*!****************************************************************************
 * fbe_raid_group_notify_lower_encryption_start()
 ******************************************************************************
 * @brief
 *  Notify Lower RAID Group of encryption.
 *  Allows a striper to notify the lower raid group to push out the checkpoint
 *  to the end of the consumed space.
 *  This allows the striper to then reconstruct its paged and know that the new
 *  key will be used.
 *
 * @param raid_group_p           - pointer to a base object 
 * @param packet_p               - pointer to a control packet from the scheduler
 *
 * @return fbe_status_t
 * 
 * @author
 *  12/11/2013 - Created. Rob Foley
 *
 ******************************************************************************/
fbe_status_t 
fbe_raid_group_notify_lower_encryption_start(fbe_raid_group_t *raid_group_p,
                                             fbe_packet_t *packet_p) 
{
    fbe_status_t status;
    fbe_u32_t width;
    fbe_base_config_memory_allocation_chunks_t mem_alloc_chunks;
    fbe_lba_t paged_md_start_lba;
    fbe_block_count_t paged_md_blocks;
    fbe_payload_ex_t *payload_p = NULL;
    fbe_payload_block_operation_t *block_operation_p = NULL;
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_group_get_raid_geometry(raid_group_p);
    fbe_u16_t data_disks;

    fbe_raid_geometry_get_data_disks(raid_geometry_p, &data_disks);
    fbe_base_config_metadata_get_paged_record_start_lba(&(raid_group_p->base_config),
                                                        &paged_md_start_lba);
    fbe_base_config_metadata_get_paged_metadata_capacity(&(raid_group_p->base_config), &paged_md_blocks);

    /* Allocate a block operation to track the set chkpt.
     */
    payload_p = fbe_transport_get_payload_ex(packet_p);
    block_operation_p = fbe_payload_ex_allocate_block_operation(payload_p);
    fbe_payload_block_build_operation(block_operation_p,
                                      FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ENCRYPTION_SET_CHKPT,
                                      paged_md_start_lba / data_disks,
                                      paged_md_blocks / data_disks,
                                      FBE_BE_BYTES_PER_BLOCK,
                                      1,
                                      NULL);
    fbe_payload_ex_increment_block_operation_level(payload_p);

    /* get the width of the raid group object. */
    fbe_base_config_get_width((fbe_base_config_t *) raid_group_p, &width);

    /* We need to send the request down to all the edges and so get the width and allocate
     * packet for every edge */
    /* Set the various counts and buffer size */
    fbe_zero_memory(&mem_alloc_chunks, sizeof(fbe_base_config_memory_allocation_chunks_t));
    mem_alloc_chunks.buffer_count = 0;
    mem_alloc_chunks.number_of_packets = width;
    mem_alloc_chunks.sg_element_count = 0;
    mem_alloc_chunks.buffer_size = 0;
    mem_alloc_chunks.alloc_pre_read_desc = FALSE;

    /* allocate the subpackets to collect the zero checkpoint for all the edges. */
    status = fbe_base_config_memory_allocate_chunks((fbe_base_config_t *) raid_group_p,
                                                    packet_p,
                                                    mem_alloc_chunks,
                                                    (fbe_memory_completion_function_t)fbe_raid_group_notify_lower_encr_alloc_complete);
    if (status != FBE_STATUS_OK) {
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s memory allocation failed, status:0x%x\n", __FUNCTION__, status);
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return status; 
    }
    return status;
}
/**************************************
 * end fbe_raid_group_notify_lower_encryption_start
 **************************************/

/*!****************************************************************************
 * fbe_raid_group_notify_lower_encr_alloc_complete()
 ******************************************************************************
 * @brief
 *  This function gets called after memory has been allocated and sends the 
 *  update encryption checkpoint to the lower raid groups.
 *
 * @param memory_request_p  - memory request.
 * @param context           - context passed to acquire the memory.
 *
 * @return status           - status of the operation.
 *
 * @author
 *  12/11/2013 - Created. Rob Foley
 *
 ******************************************************************************/
static fbe_status_t
fbe_raid_group_notify_lower_encr_alloc_complete(fbe_memory_request_t * memory_request_p,
                                                fbe_memory_completion_context_t context) 
{
    fbe_packet_t *                                  packet_p = NULL;
    fbe_packet_t **                                 new_packet_p = NULL;
    fbe_payload_ex_t *                              sep_payload_p = NULL;
    fbe_payload_ex_t *                              new_sep_payload_p = NULL;
    fbe_payload_block_operation_t *               block_operation_p = NULL;
    fbe_payload_block_operation_t *               new_block_operation_p = NULL;
    fbe_status_t                                    status = FBE_STATUS_OK;
    fbe_raid_group_t *                              raid_group_p = NULL;
    fbe_u32_t                                       width = 0;
    fbe_u32_t                                       index = 0;
    fbe_block_edge_t *                              block_edge_p = NULL;
    fbe_base_config_memory_alloc_chunks_address_t   mem_alloc_chunks_addr = {0};
    fbe_sg_element_t                               *pre_read_sg_list_p = NULL;
    fbe_payload_pre_read_descriptor_t              *pre_read_desc_p = NULL;
    fbe_packet_attr_t                               packet_attr;

    /* get the pointer to original packet. */
    packet_p = fbe_transport_memory_request_to_packet(memory_request_p);

    /* get the pointer to raid group object. */
    raid_group_p = (fbe_raid_group_t *) context;

    /* get the payload and metadata operation. */
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    block_operation_p =  fbe_payload_ex_get_block_operation(sep_payload_p);

    /* Check allocation status */
    if (fbe_memory_request_is_allocation_complete(memory_request_p) == FBE_FALSE) {
        /* Currently this callback doesn't expect any aborted requests */
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s: memory request: 0x%p failed. request state: %d \n",
                              __FUNCTION__, memory_request_p, memory_request_p->request_state);
        fbe_payload_block_set_status(block_operation_p, 
                                     FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED,
                                     FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_NOT_POSSIBLE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_FAILED;
    }

    /* get the width of the base config object. */
    fbe_base_config_get_width((fbe_base_config_t *) raid_group_p, &width);

    /* subpacket control buffer size. */
    mem_alloc_chunks_addr.buffer_size = 0;
    mem_alloc_chunks_addr.number_of_packets = width;
    mem_alloc_chunks_addr.buffer_count = 0;
    mem_alloc_chunks_addr.sg_element_count = 0;
    mem_alloc_chunks_addr.sg_list_p = NULL;
    mem_alloc_chunks_addr.assign_pre_read_desc = FALSE;
    mem_alloc_chunks_addr.pre_read_sg_list_p = &pre_read_sg_list_p;
    mem_alloc_chunks_addr.pre_read_desc_p = &pre_read_desc_p;

    /* get subpacket pointer from the allocated memory. */
    status = fbe_base_config_memory_assign_address_from_allocated_chunks((fbe_base_config_t *) raid_group_p,
                                                                         memory_request_p,
                                                                         &mem_alloc_chunks_addr);
    if (status != FBE_STATUS_OK) {
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s memory distribution failed, status:0x%x, width:0x%x, buffer_size:0x%x\n",
                              __FUNCTION__, status, width, mem_alloc_chunks_addr.buffer_size);
        fbe_memory_free_request_entry(memory_request_p);

        fbe_payload_block_set_status(block_operation_p, 
                                     FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED,
                                     FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_NOT_POSSIBLE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return status;
    }

    /* Set good status.  We will set bad status on each individual bad completion.
     */
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_payload_block_set_status(block_operation_p, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, 0);

    new_packet_p = mem_alloc_chunks_addr.packet_array_p;
    while (index < width) {
        /* Initialize the sub packets. */
        fbe_transport_initialize_sep_packet(new_packet_p[index]);
        new_sep_payload_p = fbe_transport_get_payload_ex(new_packet_p[index]);

        /* allocate and initialize the control operation. */
        new_block_operation_p = fbe_payload_ex_allocate_block_operation(new_sep_payload_p);
        fbe_payload_block_build_operation(new_block_operation_p,
                                          block_operation_p->block_opcode,
                                          block_operation_p->lba,
                                          block_operation_p->block_count,
                                          FBE_BE_BYTES_PER_BLOCK,
                                          1,
                                          NULL);

        /* Initialize the new packet with FBE_PACKET_FLAG_EXTERNAL same as the master packet */
        fbe_transport_get_packet_attr(packet_p, &packet_attr);
        if (packet_attr & FBE_PACKET_FLAG_EXTERNAL) {
            fbe_transport_set_packet_attr(new_packet_p[index], FBE_PACKET_FLAG_EXTERNAL);            
        }

        /* set completion functon. */
        fbe_transport_set_completion_function(new_packet_p[index],
                                              fbe_raid_group_notify_lower_encryption_subpacket_completion,
                                              raid_group_p);

        /* add the subpacket to master packet queue. */
        fbe_transport_add_subpacket(packet_p, new_packet_p[index]);
        index++;
    }

    /* send all the subpackets together. */
    for (index = 0; index < width; index++) {
        fbe_base_config_get_block_edge((fbe_base_config_t *)raid_group_p, &block_edge_p, index);
        fbe_block_transport_send_functional_packet(block_edge_p, new_packet_p[index]);
    }

    /* send all the subpackets. */
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/******************************************************************************
 * end fbe_raid_group_notify_lower_encr_alloc_complete()
 ******************************************************************************/
/*!****************************************************************************
 * fbe_raid_group_notify_lower_encryption_subpacket_completion()
 ******************************************************************************
 * @brief
 *  This function is used to handle the subpacket completion for sending
 *  the notification of encryption completion.
 *
 * @param memory_request_p  - memory request.
 * @param context           - context passed to acquire the memory.
 *
 * @return status           - status of the operation.
 *
 * @author
 *  12/11/2013 - Created. Rob Foley
 *
 ******************************************************************************/
static fbe_status_t 
fbe_raid_group_notify_lower_encryption_subpacket_completion(fbe_packet_t * packet_p,
                                                            fbe_packet_completion_context_t context)
{
    fbe_raid_group_t *                      raid_group_p = NULL;
    fbe_packet_t *                          master_packet_p = NULL;
    fbe_payload_ex_t *                      master_sep_payload_p = NULL;
    fbe_payload_block_operation_t *         master_block_operation_p = NULL;
    fbe_payload_ex_t *                      sep_payload_p = NULL;
    fbe_payload_block_operation_t *         block_operation_p = NULL;
    fbe_payload_block_operation_status_t    block_status;
    fbe_payload_block_operation_qualifier_t block_qualifier;
    fbe_status_t                            status;
    fbe_bool_t is_empty;

    /* Cast the context to base config object pointer. */
    raid_group_p = (fbe_raid_group_t *) context;

    /* Get the master packet, its payload and associated block operation. */
    master_packet_p = fbe_transport_get_master_packet(packet_p);
    master_sep_payload_p = fbe_transport_get_payload_ex(master_packet_p);
    master_block_operation_p = fbe_payload_ex_get_block_operation(master_sep_payload_p);

    /* get the block operation of the subpacket. */
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    block_operation_p = fbe_payload_ex_get_block_operation(sep_payload_p);

    /* get the status of the subpacket operation. */
    status = fbe_transport_get_status_code(packet_p);
    fbe_payload_block_get_status(block_operation_p, &block_status);
    fbe_payload_block_get_qualifier(block_operation_p, &block_qualifier);

    /* if any of the subpacket failed with bad status then update the master status. */
    if (status != FBE_STATUS_OK) {
        fbe_transport_set_status(master_packet_p, status, 0);
    }

    /* if any of the subpacket failed with failure status then update the master status. */
    if (block_status != FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS) {
        fbe_payload_block_set_status(master_block_operation_p, block_status, block_qualifier);
        fbe_transport_set_status(master_packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
    }

    fbe_transport_remove_subpacket_is_queue_empty(packet_p, &is_empty);

    /* when the queue is empty, all subpackets have completed. */
    if (is_empty) {
        fbe_transport_destroy_subpackets(master_packet_p);
        fbe_memory_free_request_entry(&master_packet_p->memory_request);
        fbe_payload_ex_release_block_operation(master_sep_payload_p, master_block_operation_p);
        /* When we start status is set to good.  When we get each completion we set bad status on failure.
         */
        fbe_transport_complete_packet(master_packet_p);
    } else {
        /* Not done with processing sub packets.
         */
    }
    return FBE_STATUS_MORE_PROCESSING_REQUIRED; 
}
/**************************************
 * end fbe_raid_group_notify_lower_encryption_subpacket_completion()
 **************************************/
/*!**************************************************************
 * fbe_raid_group_encryption_change_passive_state()
 ****************************************************************
 * @brief
 *  Transition the passive encryption state.
 *
 * @param raid_group_p - Raid group currently under encryption/rekey.              
 *
 * @return fbe_status_t
 *
 * @author
 *  11/11/2013 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_raid_group_encryption_change_passive_state(fbe_raid_group_t *raid_group_p)
{
    fbe_base_config_encryption_state_t state;
    fbe_base_config_encryption_mode_t mode;
    fbe_raid_group_encryption_flags_t flags;
    fbe_lba_t checkpoint;

    fbe_base_config_get_encryption_state((fbe_base_config_t*)raid_group_p, &state);
    fbe_base_config_get_encryption_mode((fbe_base_config_t*)raid_group_p, &mode);
    fbe_raid_group_encryption_get_state(raid_group_p, &flags);
    checkpoint = fbe_raid_group_get_rekey_checkpoint(raid_group_p);

    if (mode == FBE_BASE_CONFIG_ENCRYPTION_MODE_ENCRYPTED){
    }
    else if ((mode == FBE_BASE_CONFIG_ENCRYPTION_MODE_ENCRYPTION_IN_PROGRESS) ||
             (mode == FBE_BASE_CONFIG_ENCRYPTION_MODE_REKEY_IN_PROGRESS) ) {
    
        if ((state == FBE_BASE_CONFIG_ENCRYPTION_STATE_KEYS_PROVIDED) ||
            (state == FBE_BASE_CONFIG_ENCRYPTION_STATE_KEYS_PROVIDED_FOR_REKEY)) {
            fbe_base_config_set_encryption_state((fbe_base_config_t*)raid_group_p, FBE_BASE_CONFIG_ENCRYPTION_STATE_ENCRYPTION_IN_PROGRESS);
            fbe_base_object_trace((fbe_base_object_t*)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                  "Encryption: passive KEYS_PROVIDED -> ENCRYPTION_IN_PROGRESS cp: %llx fl: 0x%x md: %d\n",
                                  (unsigned long long)checkpoint, flags, mode);
        }
        else {
            fbe_lba_t exported_capacity;
            fbe_base_config_encryption_state_t encryption_state;
    
            exported_capacity = fbe_raid_group_get_exported_disk_capacity(raid_group_p);
            fbe_base_config_get_encryption_state((fbe_base_config_t*)raid_group_p, &encryption_state);

            fbe_raid_group_trace(raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_RAID_GROUP_DEBUG_FLAG_ENCRYPTION_TRACING,
                                 "Encryption: passive encryption_state: %d cp: %llx fl: 0x%x md: %d\n",
                                 encryption_state, checkpoint, flags, mode);
            /* If we are done with the last position, then check to see if we need to
             * transition our state. 
             */
            if (checkpoint >= exported_capacity) {

                if ( ((mode == FBE_BASE_CONFIG_ENCRYPTION_MODE_ENCRYPTION_IN_PROGRESS) ||
                      (mode == FBE_BASE_CONFIG_ENCRYPTION_MODE_REKEY_IN_PROGRESS)) &&
                     (encryption_state != FBE_BASE_CONFIG_ENCRYPTION_STATE_REKEY_COMPLETE) &&
                     (encryption_state != FBE_BASE_CONFIG_ENCRYPTION_STATE_REKEY_COMPLETE_PUSH_DONE) &&
                     (encryption_state != FBE_BASE_CONFIG_ENCRYPTION_STATE_REKEY_COMPLETE_PUSH_KEYS)) {
                    fbe_base_config_set_encryption_state((fbe_base_config_t*)raid_group_p,
                                                         FBE_BASE_CONFIG_ENCRYPTION_STATE_REKEY_COMPLETE);
                    fbe_base_object_trace((fbe_base_object_t*)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                          "Encryption: passive %d -> REKEY_COMPLETE cp: %llx fl: 0x%x md: %d\n",
                                          encryption_state, checkpoint, flags, mode);
                }
            }
        }
    }
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_raid_group_encryption_change_passive_state()
 ******************************************/

/*!****************************************************************************
 *  fbe_raid_group_get_next_rekey_chunk()
 ******************************************************************************
 * @brief
 *  This function will go over the search data and find out if the chunk
 *  is marked for rekey or not.
 * 
 * @param search_data -       pointer to the search data
 * @param search_size  -      search data size
 * @param raid_group_p  -     pointer to the raid group object
 *
 * @return fbe_bool_t FBE_TRUE
 *
 * @author
 *  11/27/2013 - Created. Rob Foley
 *
 ******************************************************************************/
fbe_bool_t fbe_raid_group_get_next_rekey_chunk(fbe_u8_t*     search_data,
                                               fbe_u32_t    search_size,
                                               context_t    context)
                                           
{
    fbe_raid_group_paged_metadata_t          paged_metadata;
    fbe_u8_t*                                source_ptr = NULL;
    fbe_bool_t                               b_process_chunk = FBE_FALSE;
    fbe_u32_t                                index;

    source_ptr = search_data;

    for(index = 0; index < search_size; index += sizeof(fbe_raid_group_paged_metadata_t)) {
        fbe_copy_memory(&paged_metadata, source_ptr, sizeof(fbe_raid_group_paged_metadata_t));
        source_ptr += sizeof(fbe_raid_group_paged_metadata_t);

        if (paged_metadata.rekey == 0) {
            /* If the rekey is CLEAR then we need to rekey this chunk.
             */
            b_process_chunk = FBE_TRUE;
        }
    }
    return b_process_chunk;
}
/*******************************************************
 * end fbe_raid_group_get_next_rekey_chunk()
 ******************************************************/

/*!****************************************************************************
 * fbe_raid_group_rekey_get_paged_metadata_completion()
 ******************************************************************************
 * @brief
 *   This function checks the status of the metadata operation to determine whether
 *   to proceed with the rekey or not. 
 * 
 * @param packet_p              - pointer to the packet
 * @param context                  - completion context
 *
 * @return  fbe_status_t            
 *
 * @author
 *  11/27/2013 - Created. Rob Foley
 *
 ******************************************************************************/
fbe_status_t fbe_raid_group_rekey_get_paged_metadata_completion(fbe_packet_t*  packet_p,
                                                                fbe_packet_completion_context_t context)
{

    fbe_raid_group_t*                        raid_group_p;
    fbe_payload_ex_t*                       sep_payload_p = NULL;
    fbe_payload_metadata_operation_t*       metadata_operation_p = NULL;
    fbe_raid_iots_t*                        iots_p = NULL;
    fbe_payload_block_operation_opcode_t    block_opcode;
    fbe_payload_metadata_status_t           metadata_status;  
    fbe_lba_t                               start_lba;
    fbe_lba_t                               new_rekey_checkpoint;
    fbe_status_t                            status;
    fbe_chunk_index_t                       chunk_index; 
    fbe_chunk_index_t                       new_chunk_index;
    fbe_chunk_count_t                       exported_chunks;
    fbe_block_count_t                       block_count;
    fbe_u64_t                               metadata_offset = FBE_LBA_INVALID;
    fbe_u64_t                               current_offset = FBE_LBA_INVALID;
    fbe_lba_t                               current_checkpoint;

    raid_group_p = (fbe_raid_group_t*)context;

    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    metadata_operation_p =  fbe_payload_ex_get_metadata_operation(sep_payload_p);
    fbe_payload_metadata_get_status(metadata_operation_p, &metadata_status);

    status = fbe_transport_get_status_code(packet_p);
    if ((metadata_status != FBE_PAYLOAD_METADATA_STATUS_OK) || (status!=FBE_STATUS_OK)){
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "Encryption get pged meta op: %d status: 0x%x packet status: 0x%x \n",
                          metadata_operation_p->opcode,
                          metadata_status, status);
    }
    // If the metadata status is bad, handle the error status
    if(metadata_status != FBE_PAYLOAD_METADATA_STATUS_OK) {
        fbe_raid_group_handle_metadata_error(raid_group_p, packet_p, metadata_status);
    }

    // If the status is not good set the packet status and return 
    // so that the packet gets completed
    status = fbe_transport_get_status_code(packet_p);
    if(status != FBE_STATUS_OK) {
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "Encryption paged metadata get bits failed 0x%x.\n", status);
        // Release the metadata operation
        fbe_payload_ex_release_metadata_operation(sep_payload_p, metadata_operation_p);
        
                
        /*! Is this correct way to handle? 
            If there is a metadata error, set this completion function so that
            it will clean up the iots, release the stripe lock and will clean it up
        */
        fbe_transport_set_completion_function(packet_p, fbe_raid_group_paged_metadata_iots_completion,
                                              raid_group_p);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;                                    
    }
    
    /* Current offset gives the new metadata offset for the chunk that needs to be acted upon*/
    current_offset = metadata_operation_p->u.next_marked_chunk.current_offset;

    /* calculate the new checkpoint based on the new metadata offset */
    new_chunk_index = current_offset/sizeof(fbe_raid_group_paged_metadata_t);
    exported_chunks = fbe_raid_group_get_exported_chunks(raid_group_p);
    if(new_chunk_index >= exported_chunks) {
        new_chunk_index = exported_chunks;
    }
    fbe_raid_group_bitmap_get_lba_for_chunk_index(raid_group_p,
                                                  new_chunk_index,
                                                  &new_rekey_checkpoint);

    //  Copy the chunk info that was read to iots chunk info
    fbe_payload_ex_get_iots(sep_payload_p, &iots_p);
    fbe_copy_memory(&iots_p->chunk_info[0], 
                    metadata_operation_p->u.metadata.record_data, 
                    metadata_operation_p->u.metadata.record_data_size);
   
    // Get the metadata offset 
    fbe_payload_metadata_get_metadata_offset(metadata_operation_p, &metadata_offset);
    chunk_index = metadata_offset/sizeof(fbe_raid_group_paged_metadata_t);

    // Release the metadata operation
    fbe_payload_ex_release_metadata_operation(sep_payload_p, metadata_operation_p);

    // Get the lba of the next chunk
    status = fbe_raid_group_bitmap_get_lba_for_chunk_index(raid_group_p, chunk_index, &start_lba);

    if(status != FBE_STATUS_OK) {
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "Encryption %s invalid chunk index. 0x%llx\n",
                              __FUNCTION__, (unsigned long long)chunk_index);
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_set_completion_function(packet_p, fbe_raid_group_paged_metadata_iots_completion,
                                              raid_group_p);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }

    fbe_raid_iots_get_opcode(iots_p, &block_opcode);
    fbe_raid_iots_get_lba(iots_p, &start_lba);
    fbe_raid_iots_get_blocks(iots_p, &block_count);

    /* If the new checkpoint is greater than the current lba just move the checkpoint */
    if(new_rekey_checkpoint >= (start_lba + block_count)) {
        fbe_transport_set_completion_function(packet_p, fbe_raid_group_paged_metadata_iots_completion,
                                              raid_group_p);  
        
        block_count =   new_rekey_checkpoint - start_lba;  

        current_checkpoint = fbe_raid_group_get_rekey_checkpoint(raid_group_p);

        fbe_raid_iots_set_block_status(iots_p, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS); 
        fbe_raid_iots_set_block_qualifier(iots_p, FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NOTHING_TO_REKEY); 
        fbe_payload_ex_set_media_error_lba(sep_payload_p, start_lba + block_count);
        
        if (new_rekey_checkpoint > current_checkpoint) {
            fbe_base_object_trace((fbe_base_object_t*)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                  "Encryption gt pged chkpt: 0x%llx lba:0x%llx bl:0x%llx new_chkpt:0x%llx\n",
                                  (unsigned long long)current_checkpoint,
                                  (unsigned long long)start_lba, (unsigned long long)block_count, 
                                  (unsigned long long)new_rekey_checkpoint);
            status = fbe_raid_group_update_rekey_checkpoint(packet_p, raid_group_p, start_lba, block_count, __FUNCTION__);
        } else {
            fbe_base_object_trace((fbe_base_object_t*)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                  "Encryption gt pged1 chkpt: 0x%llx lba:0x%llx bl:0x%llx new_chkpt:0x%llx\n",
                                  (unsigned long long)current_checkpoint,
                                  (unsigned long long)start_lba, (unsigned long long)block_count, 
                                  (unsigned long long)new_rekey_checkpoint);
            fbe_transport_complete_packet(packet_p);
        }
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }
    fbe_raid_group_rekey_handle_chunks(raid_group_p, packet_p); 
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;

} // End fbe_raid_group_rekey_get_paged_metadata_completion

/*!****************************************************************************
 * fbe_raid_group_rekey_read_paged()
 ******************************************************************************
 * @brief
 *  Initiate a read of paged for a rekey operation.
 * 
 * @param packet_p
 * @param context
 * 
 * @return  fbe_status_t  
 *
 * @author
 *  11/27/2013 - Created. Rob Foley
 *
 ******************************************************************************/
fbe_status_t fbe_raid_group_rekey_read_paged(fbe_packet_t * packet_p,                                    
                                             fbe_packet_completion_context_t context)

{
    fbe_raid_group_t*                    raid_group_p = NULL;
    fbe_raid_iots_t*                     iots_p = NULL;
    fbe_payload_ex_t*                   sep_payload_p = NULL;
    fbe_status_t                         status; 
    fbe_lba_t                            rekey_lba;
    fbe_lba_t                            end_lba;
    fbe_chunk_index_t                    chunk_index;
    fbe_chunk_index_t                    end_chunk_index;
    fbe_chunk_count_t                    chunk_count;
    fbe_payload_block_operation_opcode_t block_opcode = FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INVALID;
    fbe_lba_t blocks;
    fbe_raid_geometry_t *raid_geometry_p = NULL;

    raid_group_p = (fbe_raid_group_t*)context;
    raid_geometry_p = fbe_raid_group_get_raid_geometry(raid_group_p);

    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    fbe_payload_ex_get_iots(sep_payload_p, &iots_p);

    /* Until the mirror has rekeyed the paged, the striper cannot proceed.
     */
    if (fbe_raid_geometry_is_mirror_under_striper(raid_geometry_p) &&
        (!fbe_raid_group_is_encryption_state_set(raid_group_p, FBE_RAID_GROUP_ENCRYPTION_FLAG_PVD_NOTIFIED) ||
         !fbe_raid_group_is_encryption_state_set(raid_group_p, FBE_RAID_GROUP_ENCRYPTION_FLAG_PAGED_ENCRYPTED))) {
        fbe_raid_group_encryption_flags_t flags;
        /* Set status to unexpected and fail the I/O.
         */
        fbe_raid_group_add_to_terminator_queue(raid_group_p, packet_p);
        fbe_raid_iots_set_block_status(iots_p, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED);
        fbe_raid_iots_set_block_qualifier(iots_p, FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_POSSIBLE);
        fbe_raid_group_encryption_get_state(raid_group_p, &flags);
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "Encryption read paged cannot proceed encryption flags: 0x%x\n",  flags);
        fbe_raid_group_iots_completion(packet_p, raid_group_p);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }

    /* With RAID-10 we need to distribute the read to all the drives.
     */
    if (fbe_raid_geometry_is_raid10(raid_geometry_p)) {

        fbe_transport_set_completion_function(packet_p, fbe_raid_group_rekey_read_completion, raid_group_p);
        status = fbe_striper_send_read_rekey(packet_p, (fbe_base_object_t *)raid_group_p, raid_geometry_p);
        return status;
    }

    fbe_raid_iots_get_current_opcode(iots_p, &block_opcode);

    // Get the lba from the block operation and use it to get the chunk index
    fbe_raid_iots_get_lba(iots_p, &rekey_lba);
    fbe_raid_iots_get_blocks(iots_p, &blocks);
    end_lba = rekey_lba + blocks;

    fbe_raid_group_bitmap_get_chunk_index_for_lba(raid_group_p, rekey_lba, &chunk_index);
    fbe_raid_group_bitmap_get_chunk_index_for_lba(raid_group_p, end_lba, &end_chunk_index);
    chunk_count = (fbe_chunk_count_t)(end_chunk_index - chunk_index);

    fbe_transport_set_completion_function(packet_p, fbe_raid_group_rekey_get_paged_metadata_completion, raid_group_p);
    status = fbe_raid_group_bitmap_get_next_marked(raid_group_p, packet_p, chunk_index, chunk_count, 
                                                   fbe_raid_group_get_next_rekey_chunk,
                                                   raid_group_p);
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/**************************************
 * end fbe_raid_group_rekey_read_paged()
 **************************************/

/*!**************************************************************
 * fbe_raid_group_get_blocks_to_rekey()
 ****************************************************************
 * @brief
 *  Get the number of contiguous blocks that are marked for rekey.
 *
 * @param raid_group_p
 * @param iots_p - The chunk info of the iots is what we check for
 *                 the rekey flag.
 * @param block_p - Block count of contiguous blocks not marked for rekey.
 *
 * @return fbe_status_t
 *
 * @author
 *  12/18/2013 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_raid_group_get_blocks_to_rekey(fbe_raid_group_t *raid_group_p,
                                                    fbe_raid_iots_t *iots_p,
                                                    fbe_u8_t rekeyed_bit,
                                                    fbe_block_count_t *block_p)
{
    fbe_u32_t chunk_index;
    fbe_block_count_t blocks = 0;
    fbe_block_count_t iots_blocks;
    fbe_chunk_size_t chunk_size = fbe_raid_group_get_chunk_size(raid_group_p);
    fbe_chunk_count_t chunk_count;

    fbe_raid_iots_get_current_op_blocks(iots_p, &iots_blocks);
    chunk_count = (fbe_chunk_count_t) (iots_blocks / chunk_size);
    for (chunk_index = 0; chunk_index < chunk_count; chunk_index++) {
        /* Count the blocks that have not been rekeyed.
         * Break out when we find a rekeyed chunk. 
         */
        if ((iots_p->chunk_info[chunk_index].rekey & 1) == rekeyed_bit)
        {
            break;
        }
        else
        {
            blocks += chunk_size;
        }
    }
    *block_p = blocks;
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_raid_group_get_blocks_to_rekey()
 ******************************************/
/*!**************************************************************
 * fbe_raid_group_rekey_get_rekeyed_blocks()
 ****************************************************************
 * @brief
 *  Get the number of contiguous blocks that are not marked
 *  for rekey.
 *
 * @param raid_group_p
 * @param iots_p - The chunk info of the iots is what we check for
 *                 the rekey flag.
 * @param block_p - Block count of contiguous blocks not marked for rekey.
 *
 * @return fbe_status_t
 *
 * @author
 *  12/18/2013 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_raid_group_rekey_get_rekeyed_blocks(fbe_raid_group_t *raid_group_p,
                                                      fbe_raid_iots_t *iots_p,
                                                      fbe_u8_t rekeyed_bit,
                                                      fbe_block_count_t *block_p)
{
    fbe_u32_t chunk_index;
    fbe_block_count_t iots_blocks;
    fbe_block_count_t blocks = 0;
    fbe_chunk_size_t chunk_size = fbe_raid_group_get_chunk_size(raid_group_p);
    fbe_chunk_count_t chunk_count;

    fbe_raid_iots_get_current_op_blocks(iots_p, &iots_blocks);
    chunk_count = (fbe_chunk_count_t) (iots_blocks / chunk_size);

    for (chunk_index = 0; chunk_index < chunk_count; chunk_index++)
    {
        /* Count blocks that were already rekeyed.
         */
        if ((iots_p->chunk_info[chunk_index].rekey & 1) != rekeyed_bit)
        {
            break;
        }
        else
        {
            blocks += chunk_size;
        }
    }
    *block_p = blocks;
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_raid_group_rekey_get_rekeyed_blocks()
 ******************************************/

/*!***************************************************************************
 * fbe_raid_group_rekey_handle_chunks()
 *****************************************************************************
 * @brief
 *  This function simply moves the checkpoint if there are
 *  any leading chunks that are not marked.
 *  This function also limits the rekey blocks if it find any trailing chunks 
 *  that are not marked for rekey.
 *
 * @param   raid_group_p - Pointer for raid group for this rebuild request.
 * @param   packet_p   - Pointer to a packet for rebuld request.
 *
 * @return  fbe_status_t            
 *
 * @author
 *  4/10/2012 - Created. Rob Foley
 *
 *****************************************************************************/
static fbe_status_t fbe_raid_group_rekey_handle_chunks(fbe_raid_group_t *raid_group_p,
                                                       fbe_packet_t *packet_p) 
{
    fbe_status_t                        status = FBE_STATUS_OK;   
    fbe_payload_ex_t*                   sep_payload_p = NULL;
    fbe_raid_iots_t*                    iots_p = NULL; 
    fbe_payload_block_operation_opcode_t block_opcode;
    fbe_block_count_t blocks;
    fbe_u8_t rekeyed_bit = 1;
    fbe_lba_t start_lba;
    fbe_raid_position_bitmask_t all_nr_bitmask;
    fbe_u32_t width;
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_group_get_raid_geometry(raid_group_p);

    sep_payload_p = fbe_transport_get_payload_ex(packet_p); 
    fbe_payload_ex_get_iots(sep_payload_p, &iots_p);

    fbe_raid_iots_get_opcode(iots_p, &block_opcode);
    fbe_raid_iots_get_lba(iots_p, &start_lba);

    if ((iots_p->chunk_info[0].rekey & 1) == rekeyed_bit)
    {
        /* Skip over the already rekeyed chunks by moving the checkpoint. 
         * We will perform the actual rekey on this next monitor cycle. 
         */
        fbe_raid_group_rekey_get_rekeyed_blocks(raid_group_p, iots_p, rekeyed_bit, &blocks);

        /* We will move the checkpoint for this number of blocks and complete the iots.
         */
        fbe_transport_set_completion_function(packet_p, fbe_raid_group_verify_iots_completion, raid_group_p); 

        fbe_raid_group_trace(raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_RAID_GROUP_DEBUG_FLAG_ENCRYPTION_TRACING,
                             "raid_group rekey is set. current_chkpt:0x%llx, bl: 0x%llx\n",
                             (unsigned long long)start_lba, (unsigned long long)blocks);

        fbe_raid_iots_set_block_status(iots_p, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS); 
        fbe_raid_iots_set_block_qualifier(iots_p, FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NOTHING_TO_REKEY); 
        fbe_payload_ex_set_media_error_lba(sep_payload_p, start_lba + blocks);
        status = fbe_raid_group_update_rekey_checkpoint(packet_p, raid_group_p,
                                                         start_lba, blocks, __FUNCTION__);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }
    else
    {
        fbe_block_count_t iots_blocks;
        fbe_raid_iots_get_current_op_blocks(iots_p, &iots_blocks);

        /* We definately have something marked.  See how far it goes.
         */
        fbe_raid_group_get_blocks_to_rekey(raid_group_p, iots_p, rekeyed_bit, &blocks);

        if (blocks < iots_blocks)
        {
            /* We have something unmarked.  Let's do the rekey for the area that is marked only.
             * Another monitor cycle will handle the unmarked area.
             */
            fbe_raid_iots_set_blocks(iots_p, blocks);
            fbe_raid_iots_set_blocks_remaining(iots_p, blocks);
            fbe_raid_iots_set_current_op_blocks(iots_p, blocks);
            fbe_raid_iots_set_flag(iots_p, FBE_RAID_IOTS_FLAG_LAST_IOTS_OF_REQUEST);

            fbe_raid_group_trace(raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_RAID_GROUP_DEBUG_FLAG_VERIFY_TRACING,
                                 "raid_group rekey limit marked orig_blks: 0x%llx new_blks: 0x%llx\n",
                                 (unsigned long long)iots_blocks, (unsigned long long)blocks);
        }
    }
    /* We will tell the caller that we are done.  This is necessary to release the stripe lock.
     * We indicate to continue rekey so that the caller will know that there is something to do 
     * immediately. 
     */
    fbe_raid_iots_set_block_status(iots_p, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS); 
    fbe_raid_iots_set_block_qualifier(iots_p, FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_CONTINUE_REKEY);
    fbe_payload_ex_set_media_error_lba(sep_payload_p, start_lba);

    /* If we find that multiple bits are set, then we will need to issue an invalidate for the entire stripe.
     */
    fbe_raid_geometry_get_width(raid_geometry_p, &width);
    all_nr_bitmask = (1 << width) - 1;
    if ((iots_p->chunk_info[0].needs_rebuild_bits & all_nr_bitmask) == all_nr_bitmask){
        fbe_raid_iots_set_block_status(iots_p, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS); 
        fbe_raid_iots_set_block_qualifier(iots_p, FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_INCOMPLETE_WRITE_FOUND); 
    }
    /* Put the packet back on the termination queue since we had to take it off before reading the chunk info
     */
    fbe_raid_group_add_to_terminator_queue(raid_group_p, packet_p);
    fbe_raid_group_iots_cleanup(packet_p, raid_group_p);
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/* end of fbe_raid_group_rekey_handle_chunks() */

/*!****************************************************************************
 * fbe_raid_group_allocate_encryption_context()
 ******************************************************************************
 * @brief
 *   This function is called by the raid group monitor to allocate the control
 *   operation which tracks encryption operation.
 *
 * @param raid_group_p           - pointer to raid group
 * @param packet_p               - pointer to packet which will contain context 
 * @param encryption_context_p     - pointer to the encryption context
 * 
 * @return fbe_status_t  
 *
 * @author
 *  12/3/2013 - Created. Rob Foley
 *
 ******************************************************************************/
static fbe_status_t fbe_raid_group_allocate_encryption_context(fbe_raid_group_t*   raid_group_p,
                                                               fbe_packet_t*       packet_p,
                                                               fbe_raid_group_encryption_context_t *encryption_context_p)
{
    fbe_payload_ex_t *                   sep_payload_p;
    fbe_payload_control_operation_t *     control_operation_p = NULL;

    sep_payload_p = fbe_transport_get_payload_ex(packet_p);

    control_operation_p = fbe_payload_ex_allocate_control_operation(sep_payload_p);
    if(control_operation_p == NULL) {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_build_operation(control_operation_p,
                                        FBE_RAID_GROUP_CONTROL_CODE_REKEY_OPERATION,
                                        encryption_context_p,
                                        sizeof(fbe_raid_group_encryption_context_t));

    fbe_payload_ex_increment_control_operation_level(sep_payload_p);

    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_raid_group_allocate_encryption_context()
 **************************************/
/*!****************************************************************************
 * fbe_raid_group_release_encryption_context()
 ******************************************************************************
 * @brief
 *   This function is called by the raid group monitor to release the control
 *   operation which tracks rekey operation.
 *
 * @param packet_p         - packet that the context was allocated to 
 * 
 * @return fbe_status_t  
 *
 * @author
 *  12/3/2013 - Created. Rob Foley
 *
 ******************************************************************************/
static fbe_status_t fbe_raid_group_release_encryption_context(fbe_packet_t*       packet_p)
{
    fbe_payload_ex_t                   *sep_payload_p;
    fbe_payload_control_operation_t    *control_operation_p = NULL;
    fbe_raid_group_encryption_context_t   *encryption_context_p = NULL;
    fbe_status_t                        status;

    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(sep_payload_p);
    if (control_operation_p == NULL) {
        return FBE_STATUS_GENERIC_FAILURE;
    }
    fbe_payload_control_get_buffer(control_operation_p, &encryption_context_p);

    status = fbe_payload_ex_release_control_operation(sep_payload_p, control_operation_p);

    return status;
}
/**************************************
 * end fbe_raid_group_release_encryption_context()
 **************************************/
/*!****************************************************************************
 * fbe_raid_group_initialize_encryption_context
 ******************************************************************************
 * @brief
 *   This function is called by the raid group monitor to initialize the encryption
 *   context before starting the encryption operation.
 *
 * @param raid_group_p            - pointer to raid group information
 * @param encryption_context_p       - pointer to encryption context.
 * @param encryption_lba             - Logical block address to start encryption at
 * 
 * @return fbe_status_t  
 *
 * @author
 *  12/3/2013 - Created. Rob Foley
 *
 ******************************************************************************/
static fbe_status_t fbe_raid_group_initialize_encryption_context(
                                        fbe_raid_group_t                 *raid_group_p,
                                        fbe_raid_group_encryption_context_t *encryption_context_p, 
                                        fbe_lba_t                         encryption_lba)
{
    fbe_block_count_t   block_count;

    block_count = fbe_raid_group_get_chunk_size(raid_group_p);

    //  Set the encryption data in the encryption tracking structure 
    encryption_context_p->start_lba = encryption_lba;
    encryption_context_p->block_count = block_count;
    encryption_context_p->lun_object_id = FBE_OBJECT_ID_INVALID;
    encryption_context_p->is_lun_start_b = encryption_context_p->is_lun_end_b = FBE_FALSE;
    encryption_context_p->update_checkpoint_bitmask = 0;
    encryption_context_p->update_checkpoint_lba = FBE_LBA_INVALID;
    encryption_context_p->update_checkpoint_blocks = 0;
    encryption_context_p->b_beyond_capacity = FBE_FALSE;
    encryption_context_p->encryption_status = FBE_STATUS_OK;
    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_raid_group_initialize_encryption_context()
 **************************************/

/*!****************************************************************************
 * fbe_raid_group_get_encryption_context()
 ******************************************************************************
 * @brief
 *   This function is used to get the pointer to encryption context information.
 *
 * @param raid_group_p           - pointer to encryption context
 * @param packet_p               - pointer to the packet
 * @param encryption_context_p     - pointer to the encryption context
 * 
 * @return fbe_status_t  
 *
 * @author
 *   10/26/2010 - Created. Dhaval Patel.
 *
 ******************************************************************************/
static fbe_status_t 
fbe_raid_group_get_encryption_context(fbe_raid_group_t * raid_group_p,
                                      fbe_packet_t *packet_p,
                                      fbe_raid_group_encryption_context_t **encryption_context_p)
{
    fbe_payload_ex_t*                  sep_payload_p = NULL;
    fbe_payload_control_operation_t*    control_operation_p = NULL;

    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_present_control_operation(sep_payload_p);
    fbe_payload_control_get_buffer(control_operation_p, encryption_context_p);
    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_raid_group_get_encryption_context()
 **************************************/

/*!****************************************************************************
 * fbe_raid_group_rekey_read_completion()
 ******************************************************************************
 * @brief
 *   The read to the lower level completed, now decide if we can continue or not.
 * 
 * @param packet_p              - pointer to the packet
 * @param context                  - completion context
 *
 * @return  fbe_status_t            
 *
 * @author
 *  11/27/2013 - Created. Rob Foley
 *
 ******************************************************************************/
static fbe_status_t fbe_raid_group_rekey_read_completion(fbe_packet_t*  packet_p,
                                                         fbe_packet_completion_context_t context)
{
    fbe_raid_group_t*                       raid_group_p = (fbe_raid_group_t*)context;
    fbe_payload_ex_t*                       sep_payload_p = NULL;
    fbe_payload_block_operation_t          *block_operation_p = NULL;
    fbe_raid_iots_t*                        iots_p = NULL;
    fbe_lba_t                               new_rekey_checkpoint;
    fbe_status_t                            status;
    fbe_block_count_t                       block_count;
    fbe_payload_block_operation_status_t    block_status;
    fbe_payload_block_operation_qualifier_t block_qualifier;
    fbe_raid_group_encryption_context_t    *encryption_context_p = NULL;

    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    block_operation_p = fbe_payload_ex_get_block_operation(sep_payload_p);
    fbe_payload_block_get_status(block_operation_p, &block_status);
    fbe_payload_block_get_qualifier(block_operation_p, &block_qualifier);
    fbe_raid_group_get_encryption_context(raid_group_p, packet_p, &encryption_context_p);
    fbe_payload_ex_get_iots(sep_payload_p, &iots_p);

    status = fbe_transport_get_status_code(packet_p);
    if ((block_status != FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS) || 
        (status != FBE_STATUS_OK)){
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "read/rekey from mirrors failed op: %d status: 0x%x packet status: 0x%x \n",
                          block_operation_p->block_opcode, block_status, status);
        fbe_transport_set_status(packet_p, FBE_STATUS_FAILED, 0);
        fbe_payload_ex_get_iots(sep_payload_p, &iots_p);
        fbe_raid_iots_set_block_status(iots_p, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED); 
        fbe_raid_iots_set_block_qualifier(iots_p, FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_POSSIBLE); 
        fbe_raid_group_add_to_terminator_queue(raid_group_p, packet_p);
        fbe_raid_group_iots_cleanup(packet_p, raid_group_p);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }
    if ((block_qualifier == FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_CONTINUE_REKEY) ||
        (block_qualifier == FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NOTHING_TO_REKEY)) {
        fbe_payload_ex_get_media_error_lba(sep_payload_p, &new_rekey_checkpoint);

        /* If the new checkpoint is greater than the current lba just move the checkpoint
         */
        if ((new_rekey_checkpoint != FBE_LBA_INVALID) &&
            (new_rekey_checkpoint > encryption_context_p->start_lba)) {
            fbe_transport_set_completion_function(packet_p, fbe_raid_group_paged_metadata_iots_completion, raid_group_p);  

            block_count = new_rekey_checkpoint - encryption_context_p->start_lba;  

            fbe_base_object_trace((fbe_base_object_t*)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                  "Encryption: read move chkpt current_chkpt:0x%llx,bl:0x%llx new_chkpt:0x%llx\n",
                                  (unsigned long long)encryption_context_p->start_lba, 
                                  (unsigned long long)block_count, (unsigned long long)new_rekey_checkpoint);
            status = fbe_raid_group_update_rekey_checkpoint(packet_p, raid_group_p, encryption_context_p->start_lba, block_count, __FUNCTION__);
            return FBE_STATUS_MORE_PROCESSING_REQUIRED;        
        }
    }
    /* We will tell the caller that we are done.  This is necessary to release the stripe lock.
     */
    fbe_payload_ex_get_iots(sep_payload_p, &iots_p);
    fbe_raid_iots_set_block_status(iots_p, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS); 
    fbe_raid_iots_set_block_qualifier(iots_p, FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_CONTINUE_REKEY); 
    /* Put the packet back on the termination queue since we had to take it off before reading the chunk info
     */
    fbe_raid_group_add_to_terminator_queue(raid_group_p, packet_p);
    fbe_raid_group_iots_cleanup(packet_p, raid_group_p);
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/**************************************
 * end fbe_raid_group_rekey_read_completion()
 **************************************/

/*!****************************************************************************
 * fbe_raid_group_rekey_set_chkpt_paged_completion()
 ******************************************************************************
 * @brief
 *  As part of handling the set checkpoint, the paged update completed.
 *  Now update the checkpoint.
 * 
 * @param packet_p              - pointer to the packet
 * @param context                  - completion context
 *
 * @return  fbe_status_t            
 *
 * @author
 *  12/12/2013 - Created. Rob Foley
 *
 ******************************************************************************/
static fbe_status_t fbe_raid_group_rekey_set_chkpt_paged_completion(fbe_packet_t*  packet_p,
                                                                    fbe_packet_completion_context_t context)
{
    fbe_raid_group_t*                       raid_group_p = (fbe_raid_group_t*)context;
    fbe_payload_ex_t*                       sep_payload_p = NULL;
    fbe_payload_block_operation_t          *block_operation_p = NULL;
    fbe_status_t                            status;

    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    block_operation_p = fbe_payload_ex_get_block_operation(sep_payload_p);

    status = fbe_transport_get_status_code(packet_p);
    if (status != FBE_STATUS_OK){
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                              FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                              "Encryption set chkpt meta op: %d packet status: 0x%x \n",
                               block_operation_p->block_opcode, status);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }

    status = fbe_raid_group_update_rekey_checkpoint(packet_p, raid_group_p, 
                                                    block_operation_p->lba, block_operation_p->block_count,
                                                    __FUNCTION__);
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/**************************************
 * end fbe_raid_group_rekey_set_chkpt_paged_completion()
 **************************************/
/*!**************************************************************
 * fbe_raid_group_start_set_encryption_chkpt()
 ****************************************************************
 * @brief
 *  Start the update of checkpoint by updating the paged
 *  to set that we rekeyed these chunks.
 *
 * @param raid_group_p
 * @param packet_p
 * 
 * @return fbe_status_t
 *
 * @author
 *  12/11/2013 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_raid_group_start_set_encryption_chkpt(fbe_raid_group_t *raid_group_p, 
                                                       fbe_packet_t *packet_p)
{
    fbe_lba_t                           start_lba;
    fbe_block_count_t                   blocks;
    fbe_lba_t                           end_lba;
    fbe_chunk_index_t                   start_chunk;
    fbe_chunk_index_t                   end_chunk;
    fbe_chunk_count_t                   chunk_count;
    fbe_raid_group_paged_metadata_t     chunk_data;
    fbe_raid_geometry_t                 *raid_geometry_p = NULL;
    fbe_u16_t                           disk_count;
    fbe_lba_t                           rg_lba;
    fbe_u8_t rekeyed_bit;
    fbe_payload_ex_t*                       sep_payload_p = NULL;
    fbe_payload_block_operation_t          *block_operation_p = NULL;

    raid_geometry_p = fbe_raid_group_get_raid_geometry(raid_group_p);
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    block_operation_p = fbe_payload_ex_get_block_operation(sep_payload_p);

    if (fbe_raid_geometry_is_mirror_under_striper(raid_geometry_p) &&
        (!fbe_raid_group_is_encryption_state_set(raid_group_p, FBE_RAID_GROUP_ENCRYPTION_FLAG_PVD_NOTIFIED) ||
         !fbe_raid_group_is_encryption_state_set(raid_group_p, FBE_RAID_GROUP_ENCRYPTION_FLAG_PAGED_ENCRYPTED))) {
        fbe_raid_group_encryption_flags_t flags;
        /* mirror under striper should not let the RAID-10 do anything until our paged is encrypted.
         */
        fbe_payload_block_set_status(block_operation_p, 
                                     FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED,
                                     FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_POSSIBLE);
        fbe_raid_group_encryption_get_state(raid_group_p, &flags);
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "read rekey cannot proceed encryption flags: 0x%x\n",  flags);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }

    /* Set block op to success.  From here on out any failure will update packet status.
     */
    fbe_payload_block_set_status(block_operation_p, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS, 0);

    fbe_transport_set_completion_function(packet_p, 
                                          fbe_raid_group_rekey_set_chkpt_paged_completion,
                                          (fbe_packet_completion_context_t) raid_group_p);

    fbe_payload_block_get_lba(block_operation_p, &start_lba);
    fbe_payload_block_get_block_count(block_operation_p, &blocks);
    end_lba = start_lba + blocks - 1;

    //  Convert the starting and ending LBAs to chunk indexes 
    fbe_raid_group_bitmap_get_chunk_index_for_lba(raid_group_p, start_lba, &start_chunk);
    fbe_raid_group_bitmap_get_chunk_index_for_lba(raid_group_p, end_lba, &end_chunk);

    //  Calculate the number of chunks to be marked 
    chunk_count = (fbe_chunk_count_t) (end_chunk - start_chunk + 1);

    fbe_raid_group_trace(raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_RAID_GROUP_DEBUG_FLAG_ENCRYPTION_TRACING,
                         "clear enc for pkt: 0x%x lba %llx to %llx chunk %llx to %llx\n", 
                         (fbe_u32_t)packet_p, (unsigned long long)start_lba, 
                         (unsigned long long)end_lba, (unsigned long long)start_chunk, (unsigned long long)end_chunk);

    fbe_set_memory(&chunk_data, 0, sizeof(fbe_raid_group_paged_metadata_t));

    /* We only use the set bit to indicate we already encrypted the data.
     */
    rekeyed_bit = 1;
    chunk_data.rekey = rekeyed_bit;

    fbe_raid_geometry_get_data_disks(raid_geometry_p, &disk_count);

    rg_lba = start_lba * disk_count;

    //  If the chunk is not in the data area, then use the non-paged metadata service to update it
    if (fbe_raid_geometry_is_metadata_io(raid_geometry_p, rg_lba)) {
        fbe_raid_group_bitmap_write_chunk_info_using_nonpaged(raid_group_p, packet_p, start_chunk,
                                                              chunk_count, &chunk_data, TRUE);
    }
    else {
        //  The chunks are in the user data area.  Use the paged metadata service to update them. 
        fbe_raid_group_bitmap_update_paged_metadata(raid_group_p, packet_p, start_chunk,
                                                    chunk_count, &chunk_data, FBE_TRUE);
    }
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/******************************************
 * end fbe_raid_group_start_set_encryption_chkpt()
 ******************************************/
/*!**************************************************************
 * fbe_raid_group_encryption_is_active()
 ****************************************************************
 * @brief
 *  This function checks to see if a encryption is active on
 *  this raid group.  Active simply means that we are
 *  past encrypting the paged and actively driving the checkpoint
 *  forward.
 *
 * @param raid_group_p           - pointer to the raid group
 *
 * @return bool status - returns FBE_TRUE if the operation needs to run
 *                       otherwise returns FBE_FALSE
 *
 * @author
 *  12/18/2013 - Created. Rob Foley
 *
 ****************************************************************/
fbe_bool_t
fbe_raid_group_encryption_is_active(fbe_raid_group_t *raid_group_p)
{
    fbe_lba_t checkpoint;
    fbe_lba_t exported_capacity;
    fbe_class_id_t  class_id;
    
    class_id = fbe_raid_group_get_class_id(raid_group_p);
    if (class_id == FBE_CLASS_ID_VIRTUAL_DRIVE) {
        return FBE_FALSE;
    }

    checkpoint = fbe_raid_group_get_rekey_checkpoint(raid_group_p);
    exported_capacity = fbe_raid_group_get_exported_disk_capacity(raid_group_p);

    /* Do not run if encryption is running (encryption will take care of that chunk).
     */
    if (fbe_raid_group_is_request_quiesced(raid_group_p,
                                           FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ENCRYPTION_REKEY_WRITE)) {
        /* Encryption is running. Don't run.
         */
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "encryption is active - encryption request in progress return False\n");
        return FBE_FALSE;
    }

    /* If encryption has not encrypted paged or the checkpoint is done, then 
     * we are done moving the checkpoint forward and just need to complete. 
     */
    if (fbe_base_config_is_rekey_mode((fbe_base_config_t *)raid_group_p) &&
        fbe_raid_group_is_encryption_state_set(raid_group_p, FBE_RAID_GROUP_ENCRYPTION_FLAG_PVD_NOTIFIED) &&
        fbe_raid_group_is_encryption_state_set(raid_group_p, FBE_RAID_GROUP_ENCRYPTION_FLAG_PAGED_ENCRYPTED) &&
        (checkpoint < exported_capacity)) {
        
        return FBE_TRUE;
    }
    return FBE_FALSE;
}
/******************************************************************************
 * end fbe_raid_group_encryption_is_active()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_raid_group_enc_invalidate_completion()
 ******************************************************************************
 * @brief
 *   Completion for invalidating a chunk stripe due to iw during rekey.
 *
 * @param packet_p               - pointer to a control packet from the scheduler
 * @param context                - context, which is a pointer to the base object 
 * 
 * @return fbe_status_t             - Always FBE_STATUS_OK
 * 
 * @author
 *  1/6/2014 - Created. Rob Foley
 *
 ******************************************************************************/
static fbe_status_t 
fbe_raid_group_enc_invalidate_completion(fbe_packet_t*                   packet_p,
                                         fbe_packet_completion_context_t context)
{

    fbe_status_t                        status;
    fbe_raid_group_t                   *raid_group_p = NULL;
    fbe_payload_ex_t                   *payload_p = NULL;
    fbe_payload_block_operation_t *block_operation_p = NULL;
    fbe_payload_block_operation_status_t block_status;
    fbe_memory_request_t *memory_request_p = NULL;
    fbe_packet_t *master_packet_p = NULL;

    raid_group_p = (fbe_raid_group_t*) context;
    master_packet_p = fbe_transport_get_master_packet(packet_p);
    memory_request_p = fbe_transport_get_memory_request(master_packet_p);

    payload_p = fbe_transport_get_payload_ex(packet_p);
    block_operation_p = fbe_payload_ex_get_block_operation(payload_p);
    status = fbe_transport_get_status_code(packet_p);

    fbe_payload_block_get_status(block_operation_p, &block_status);

    /* If enabled trace some information.
     */
    fbe_base_object_trace((fbe_base_object_t*)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                         "encryption: invalidate complete lba: 0x%llx blocks: 0x%llx status: %d bl_status: %d\n",
                         (unsigned long long)block_operation_p->lba,
                         (unsigned long long)block_operation_p->block_count, status, block_status);

    /* We originally allocated the packet and associated memory when this request started.
     */
    fbe_transport_remove_subpacket(packet_p);
    fbe_transport_destroy_packet(packet_p);
    fbe_memory_free_request_entry(memory_request_p);

    if ((status != FBE_STATUS_OK) || (block_status != FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS)) {
        fbe_base_object_trace((fbe_base_object_t*) raid_group_p, FBE_TRACE_LEVEL_WARNING, 
                              FBE_TRACE_MESSAGE_ID_INFO, "Encryption invalidate failed, status: 0x%x bl_st: 0x%x\n", 
                              status, block_status);
        fbe_transport_set_status(master_packet_p, FBE_STATUS_FAILED, 0);
        fbe_transport_complete_packet(master_packet_p);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }
    fbe_transport_set_status(master_packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(master_packet_p);
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/**************************************
 * end fbe_raid_group_enc_invalidate_completion()
 **************************************/
/*!**************************************************************
 * fbe_raid_group_enc_invalidate_alloc_completion()
 ****************************************************************
 * @brief
 *  The memory is available.
 *  Setup the packet and sg list. 
 *
 * Send the operation to invalidate the chunk stripe.
 * 
 * @param memory_request_p
 * @param context
 * 
 * @return fbe_status_t
 *
 * @author
 *  1/8/2014 - Created. Rob Foley
 *
 ****************************************************************/
static fbe_status_t fbe_raid_group_enc_invalidate_alloc_completion(fbe_memory_request_t * memory_request_p, 
                                                                   fbe_memory_completion_context_t context)
{
    fbe_packet_t *master_packet_p = NULL;
    fbe_packet_t *packet_p = NULL;
    fbe_payload_ex_t *payload_p = NULL;
    fbe_raid_group_t *raid_group_p = (fbe_raid_group_t *)context;
    fbe_memory_header_t *memory_header_p = NULL;
    fbe_memory_header_t *next_memory_header_p = NULL;
    fbe_u8_t *data_p = NULL;
    fbe_sg_element_t *sg_p = NULL;
    fbe_sg_element_t *current_sg_p = NULL;
    fbe_cpu_id_t cpu_id;
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_group_get_raid_geometry(raid_group_p);
    fbe_u16_t data_disks = 0;
    fbe_lba_t checkpoint;
    fbe_lba_t lba;
    fbe_u32_t block_count;
    fbe_u32_t blocks_remaining;
    fbe_u32_t sg_blocks;
    fbe_raid_group_monitor_packet_params_t params;
    fbe_object_id_t raid_group_object_id;

    master_packet_p = fbe_transport_memory_request_to_packet(memory_request_p);

    if (fbe_memory_request_is_allocation_complete(memory_request_p) == FBE_FALSE){
        memory_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO, "%s allocation failed\n", __FUNCTION__);
        fbe_memory_free_request_entry(memory_request_p);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }
    memory_header_p = fbe_memory_get_first_memory_header(memory_request_p);
    data_p = fbe_memory_header_to_data(memory_header_p);
    packet_p = (fbe_packet_t*)data_p;
    data_p += FBE_MEMORY_CHUNK_SIZE;

    sg_p = (fbe_sg_element_t*)data_p;

    /* Allocate the new packet so that this read and pin operation can run asynchronously.
     */
    fbe_transport_initialize_packet(packet_p);

    fbe_base_object_get_object_id((fbe_base_object_t *) raid_group_p, &raid_group_object_id);
    fbe_transport_set_address(packet_p,
                              FBE_PACKAGE_ID_SEP_0,
                              FBE_SERVICE_ID_TOPOLOGY,
                              FBE_CLASS_ID_INVALID,
                              raid_group_object_id);
    fbe_transport_add_subpacket(master_packet_p, packet_p);
    payload_p = fbe_transport_get_payload_ex(packet_p);

    cpu_id = (memory_request_p->io_stamp & FBE_PACKET_IO_STAMP_MASK) >> FBE_PACKET_IO_STAMP_SHIFT;
    fbe_transport_set_cpu_id(packet_p, cpu_id);

    fbe_transport_set_physical_drive_io_stamp(packet_p, fbe_get_time());
    fbe_transport_set_completion_function(packet_p, fbe_raid_group_enc_invalidate_completion, raid_group_p);

    fbe_raid_geometry_get_data_disks(raid_geometry_p, &data_disks);

    checkpoint = fbe_raid_group_get_rekey_checkpoint(raid_group_p);
    lba = checkpoint * data_disks;
    block_count = FBE_RAID_DEFAULT_CHUNK_SIZE * data_disks;

    fbe_payload_ex_set_sg_list(payload_p, sg_p, 0);

    /* Fill out the sg list with the new memory.
     */
    blocks_remaining = block_count;
    current_sg_p = sg_p;
    sg_blocks = (FBE_MEMORY_CHUNK_SIZE_FOR_64_BLOCKS_IO / FBE_BE_BYTES_PER_BLOCK);

    while (blocks_remaining > 0) {
        
        fbe_memory_get_next_memory_header(memory_header_p, &next_memory_header_p);
        data_p = fbe_memory_header_to_data(next_memory_header_p);
        memory_header_p = next_memory_header_p;
        fbe_sg_element_init(current_sg_p, (FBE_MIN(sg_blocks, blocks_remaining) * FBE_BE_BYTES_PER_BLOCK), data_p);

        blocks_remaining -= FBE_MIN(sg_blocks, blocks_remaining);
        current_sg_p ++;
    }
    fbe_sg_element_terminate(current_sg_p);

    /* Invalidate all blocks in the sg list.
     */
    fbe_xor_lib_fill_invalid_sectors( sg_p, lba, block_count, 0, 
                                      XORLIB_SECTOR_INVALID_REASON_VERIFY,
                                      XORLIB_SECTOR_INVALID_WHO_RAID);

    /* We need to bypass quiesce and stripe locks.
     */
    fbe_base_object_trace((fbe_base_object_t*)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                          "encryption: invalidate start op: %d lba:0x%llx blks:0x%llx \n",
                          FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ENCRYPTION_REKEY_WRITE,
                          (unsigned long long)lba, (unsigned long long)block_count);
    fbe_raid_group_setup_monitor_params(&params, 
                                        FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ENCRYPTION_REKEY_WRITE, 
                                        lba, 
                                        block_count, 
                                        (FBE_PAYLOAD_BLOCK_OPERATION_FLAGS_METADATA_OP | 
                                         FBE_PAYLOAD_BLOCK_OPERATION_FLAGS_NO_SL_THIS_LEVEL));

    fbe_raid_group_setup_monitor_packet(raid_group_p, packet_p, &params);

    /* There are some cases where we are using the monitor context.
     * In those cases it is ok to send the I/O immediately.
     */
    fbe_base_config_bouncer_entry((fbe_base_config_t *)raid_group_p, packet_p);
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_raid_group_enc_invalidate_alloc_completion()
 ******************************************/

/*!**************************************************************
 * fbe_raid_group_encryption_full_stripe_invalidate()
 ****************************************************************
 * @brief
 *  We want to envalidate a full chunk stripe.
 *  Simply start by allocating memory for this operation.
 *
 * @param raid_group_p
 * @param packet_p
 * 
 * @return fbe_status_t
 *
 * @author
 *  1/8/2014 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t fbe_raid_group_encryption_full_stripe_invalidate(fbe_raid_group_t *raid_group_p, fbe_packet_t * packet_p) 
{
    fbe_memory_request_t*                memory_request_p = NULL;
    fbe_status_t                         status;
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_group_get_raid_geometry(raid_group_p);
    fbe_u16_t data_disks;
    fbe_u32_t sg_blocks;
    fbe_u32_t buffer_count;
    fbe_u32_t blocks;

    fbe_raid_geometry_get_data_disks(raid_geometry_p, &data_disks);
    memory_request_p = fbe_transport_get_memory_request(packet_p);

    /* We need 1 sg entry per buffer.  We need 1 buffer per ~64 blocks.
     */
    sg_blocks = (FBE_MEMORY_CHUNK_SIZE_FOR_64_BLOCKS_IO / FBE_BE_BYTES_PER_BLOCK);
    blocks = FBE_RAID_DEFAULT_CHUNK_SIZE * data_disks;

    buffer_count = (blocks / sg_blocks) + 2; /* One for terminator and on in case there is an remainder. */

    status = fbe_memory_build_chunk_request(memory_request_p,
                                            FBE_MEMORY_CHUNK_SIZE_64_BLOCKS_64_BLOCKS,
                                            1 + buffer_count,    /* One for sg and packet */
                                            0, /* Priority */
                                            0, /* Io stamp*/
                                            (fbe_memory_completion_function_t)fbe_raid_group_enc_invalidate_alloc_completion,
                                            raid_group_p);
    if (status == FBE_STATUS_GENERIC_FAILURE) {

        memory_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,                                        
                             "iw invalidate Setup for memory allocation failed, status:0x%x, line:%d\n", status, __LINE__);  
        return FBE_STATUS_OK;
    }

    fbe_memory_request_entry(memory_request_p);
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/******************************************
 * end fbe_raid_group_encryption_full_stripe_invalidate()
 ******************************************/

/*!****************************************************************************
 * fbe_raid_group_iw_check_update_chkpt_completion()
 ******************************************************************************
 * @brief
 *  When the checkpoint set completes, cleanup and reschedule monitor.
 * 
 * @param packet_p - pointer to the packet
 * @param context  - completion context
 *
 * @return  fbe_status_t            
 *
 * @author
 *  4/25/2014 - Created. Rob Foley
 *
 ******************************************************************************/
static fbe_status_t fbe_raid_group_iw_check_update_chkpt_completion(fbe_packet_t*  packet_p,
                                                                    fbe_packet_completion_context_t context)
{
    fbe_raid_group_t *raid_group_p = (fbe_raid_group_t*)context;

    /* Reschedule immediately so the check proceeds quickly.
     */
    fbe_lifecycle_reschedule(&fbe_raid_group_lifecycle_const, (fbe_base_object_t*) raid_group_p, 10);

    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_raid_group_iw_check_update_chkpt_completion()
 **************************************/
/*!****************************************************************************
 * fbe_raid_group_encryption_iw_check_completion()
 ******************************************************************************
 * @brief
 *  As part of handling the set checkpoint, the paged update completed.
 *  Now update the checkpoint.
 * 
 * @param packet_p - pointer to the packet
 * @param context  - completion context
 *
 * @return  fbe_status_t            
 *
 * @author
 *  12/18/2013 - Created. Rob Foley
 *
 ******************************************************************************/
static fbe_status_t fbe_raid_group_encryption_iw_check_completion(fbe_packet_t*  packet_p,
                                                                  fbe_packet_completion_context_t context)
{
    fbe_raid_group_t*                       raid_group_p = (fbe_raid_group_t*)context;
    fbe_payload_ex_t*                       sep_payload_p = NULL;
    fbe_payload_block_operation_t          *block_operation_p = NULL;
    fbe_status_t                            status;
    fbe_block_count_t                       block_count;
    fbe_payload_block_operation_status_t    block_status;
    fbe_payload_block_operation_qualifier_t block_qualifier;
    fbe_lba_t                               lba;
    fbe_scheduler_hook_status_t             hook_status = FBE_SCHED_STATUS_OK;
    fbe_packet_t                           *master_packet_p = NULL;
    fbe_memory_request_t                   *memory_request_p = NULL;
    fbe_lba_t                              new_rekey_checkpoint;

    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    block_operation_p = fbe_payload_ex_get_block_operation(sep_payload_p);

    fbe_payload_ex_get_media_error_lba(sep_payload_p, &new_rekey_checkpoint);
    fbe_payload_block_get_status(block_operation_p, &block_status);
    fbe_payload_block_get_qualifier(block_operation_p, &block_qualifier);
    fbe_payload_block_get_lba(block_operation_p, &lba);
    fbe_payload_block_get_block_count(block_operation_p, &block_count);

    fbe_payload_ex_release_block_operation(sep_payload_p, block_operation_p);

    master_packet_p = fbe_transport_get_master_packet(packet_p);
    memory_request_p = fbe_transport_get_memory_request(master_packet_p);

    fbe_transport_remove_subpacket(packet_p);
    status = fbe_transport_get_status_code(packet_p);
    fbe_transport_set_status(master_packet_p, status, 0);
    fbe_transport_destroy_subpackets(master_packet_p);
    fbe_memory_free_request_entry(memory_request_p);

    if ((block_status != FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS) || 
        (status != FBE_STATUS_OK)){
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "encrypt iw check failed op: %d status: 0x%x packet status: 0x%x \n",
                          block_operation_p->block_opcode, block_status, status);
        fbe_transport_set_status(master_packet_p, FBE_STATUS_FAILED, 0);
        fbe_transport_complete_packet(master_packet_p);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }

    if ((block_qualifier == FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_CONTINUE_REKEY) ||
        (block_qualifier == FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NOTHING_TO_REKEY)) {

        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "encryption: iw check current_chkpt:0x%llx,bl:0x%llx new_chkpt:0x%llx\n",
                              (unsigned long long)lba, (unsigned long long)block_count, (unsigned long long)new_rekey_checkpoint);

        /* If the new checkpoint is greater than the current lba just move the checkpoint
         */
        if ((new_rekey_checkpoint != FBE_LBA_INVALID) &&
            (new_rekey_checkpoint > lba)) {
            fbe_raid_group_check_hook(raid_group_p,
                                      SCHEDULER_MONITOR_STATE_RAID_GROUP_ENCRYPTION,
                                      FBE_RAID_GROUP_SUBSTATE_ENCRYPTION_IW_CHECK_MOVE_CHKPT,
                                      0, &hook_status);
            if (hook_status == FBE_SCHED_STATUS_DONE) { 
                fbe_transport_complete_packet(master_packet_p); 
                return FBE_STATUS_MORE_PROCESSING_REQUIRED; 
            }

            fbe_transport_set_completion_function(master_packet_p, fbe_raid_group_iw_check_update_chkpt_completion, raid_group_p);  

            block_count = new_rekey_checkpoint - lba;  

            fbe_base_object_trace((fbe_base_object_t*)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                  "encryption: iw check move chkpt current_chkpt:0x%llx,bl:0x%llx new_chkpt:0x%llx\n",
                                  (unsigned long long)lba, (unsigned long long)block_count, (unsigned long long)new_rekey_checkpoint);

            status = fbe_raid_group_update_rekey_checkpoint(master_packet_p, raid_group_p, lba, block_count, __FUNCTION__);
            return FBE_STATUS_MORE_PROCESSING_REQUIRED;        
        }
    } else if (block_qualifier == FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_INCOMPLETE_WRITE_FOUND) {
        /* The read of paged found an incomplete write.
         * We need to do a full stripe invalidate. 
         * The cache will flush out the data later. 
         */
        fbe_raid_group_check_hook(raid_group_p,
                                  SCHEDULER_MONITOR_STATE_RAID_GROUP_ENCRYPTION,
                                  FBE_RAID_GROUP_SUBSTATE_ENCRYPTION_IW_CHECK_INVALIDATE,
                                  0, &hook_status);
        if (hook_status == FBE_SCHED_STATUS_DONE) { 
            fbe_transport_complete_packet(master_packet_p); 
            return FBE_STATUS_MORE_PROCESSING_REQUIRED; 
        }
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "encryption: iw check chunk invalidate current_chkpt:0x%llx,bl:0x%llx \n",
                              (unsigned long long)lba, (unsigned long long)block_count);
        fbe_raid_group_encryption_full_stripe_invalidate(raid_group_p, master_packet_p);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }

    fbe_raid_group_check_hook(raid_group_p,
                              SCHEDULER_MONITOR_STATE_RAID_GROUP_ENCRYPTION,
                              FBE_RAID_GROUP_SUBSTATE_ENCRYPTION_IW_CHECK_DONE,
                              0, &hook_status);
    if (hook_status == FBE_SCHED_STATUS_DONE) { 
        fbe_transport_complete_packet(master_packet_p); 
        return FBE_STATUS_MORE_PROCESSING_REQUIRED; 
    }

    fbe_base_object_trace((fbe_base_object_t*)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                          "encryption: iw check Done bs/bq: 0x%x/0x%xcurrent_chkpt:0x%llx bl:0x%llx\n",
                          block_status, block_qualifier, (unsigned long long)lba, (unsigned long long)block_count);

    status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)raid_group_p);
    if (status != FBE_STATUS_OK){
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s can't clear current condition, status: 0x%x\n",
                              __FUNCTION__, status);
    }
    fbe_transport_complete_packet(master_packet_p);
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/**************************************
 * end fbe_raid_group_encryption_iw_check_completion()
 **************************************/

/*!**************************************************************
 * fbe_raid_group_encryption_iw_check_alloc_completion()
 ****************************************************************
 * @brief
 *  The memory is available.
 *  Setup the packet and sg list. 
 *
 * Send the operation to invalidate the chunk stripe.
 * 
 * @param memory_request_p
 * @param context
 * 
 * @return fbe_status_t
 *
 * @author
 *  1/8/2014 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_raid_group_encryption_iw_check_alloc_completion(fbe_memory_request_t * memory_request_p, 
                                                                 fbe_memory_completion_context_t context)
{
    fbe_packet_t *master_packet_p = NULL;
    fbe_packet_t *packet_p = NULL;
    fbe_payload_ex_t *payload_p = NULL;
    fbe_raid_group_t *raid_group_p = (fbe_raid_group_t *)context;
    fbe_memory_header_t *memory_header_p = NULL;
    fbe_memory_header_t *next_memory_header_p = NULL;
    fbe_sg_element_t *sg_p = NULL;
    fbe_u8_t *data_p = NULL;
    fbe_cpu_id_t cpu_id;
    fbe_lba_t start_lba;
    fbe_u32_t block_count;
    fbe_raid_group_monitor_packet_params_t params;
    fbe_object_id_t raid_group_object_id;
    fbe_u32_t encrypt_blob_blocks;
    fbe_u32_t bytes_per_buffer = FBE_MEMORY_CHUNK_SIZE_BLOCK_COUNT_64 * FBE_BE_BYTES_PER_BLOCK;

    start_lba = fbe_raid_group_get_rekey_checkpoint(raid_group_p);
    block_count = fbe_raid_group_get_chunk_size(raid_group_p);

    master_packet_p = fbe_transport_memory_request_to_packet(memory_request_p);

    if (fbe_memory_request_is_allocation_complete(memory_request_p) == FBE_FALSE){
        memory_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO, "%s allocation failed\n", __FUNCTION__);
        fbe_memory_free_request_entry(memory_request_p);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }
    memory_header_p = fbe_memory_get_first_memory_header(memory_request_p);
    data_p = (fbe_u8_t*)fbe_memory_header_to_data(memory_header_p);
    packet_p = (fbe_packet_t*)data_p;
    data_p += FBE_MEMORY_CHUNK_SIZE;
    sg_p = (fbe_sg_element_t*)data_p;
    data_p += sizeof(fbe_sg_element_t) * 3;

    fbe_sg_element_init(&sg_p[0], bytes_per_buffer - (sizeof(fbe_sg_element_t) * 3) - FBE_MEMORY_CHUNK_SIZE, data_p);
    fbe_memory_get_next_memory_header(memory_header_p, &next_memory_header_p);
    data_p = (fbe_u8_t*)fbe_memory_header_to_data(next_memory_header_p);
    fbe_sg_element_init(&sg_p[1], FBE_MEMORY_CHUNK_SIZE_BLOCK_COUNT_64 * FBE_BE_BYTES_PER_BLOCK, data_p);
    fbe_sg_element_terminate(&sg_p[2]);

    encrypt_blob_blocks = fbe_raid_group_get_encrypt_blob_blocks(raid_group_p);
    if ( encrypt_blob_blocks < FBE_MEMORY_CHUNK_SIZE_BLOCK_COUNT_64 ) {
        fbe_sg_element_init(&sg_p[0], (encrypt_blob_blocks * FBE_BE_BYTES_PER_BLOCK), data_p);
        fbe_sg_element_terminate(&sg_p[1]);
    }

    /* Allocate the new packet so that this read and pin operation can run asynchronously.
     */
    fbe_transport_initialize_packet(packet_p);

    fbe_base_object_get_object_id((fbe_base_object_t *) raid_group_p, &raid_group_object_id);
    fbe_transport_set_address(packet_p,
                              FBE_PACKAGE_ID_SEP_0,
                              FBE_SERVICE_ID_TOPOLOGY,
                              FBE_CLASS_ID_INVALID,
                              raid_group_object_id);
    fbe_transport_add_subpacket(master_packet_p, packet_p);
    payload_p = fbe_transport_get_payload_ex(packet_p);

    cpu_id = (memory_request_p->io_stamp & FBE_PACKET_IO_STAMP_MASK) >> FBE_PACKET_IO_STAMP_SHIFT;
    fbe_transport_set_cpu_id(packet_p, cpu_id);

    fbe_transport_set_physical_drive_io_stamp(packet_p, fbe_get_time());
    fbe_payload_ex_set_sg_list(payload_p, sg_p, 0);

    fbe_transport_set_completion_function(packet_p, 
                                          fbe_raid_group_encryption_iw_check_completion,
                                          (fbe_packet_completion_context_t) raid_group_p);

    /* We need to bypass quiesce and stripe locks.
     */
    fbe_raid_group_setup_monitor_params(&params, 
                                        FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ENCRYPTION_READ_PAGED, 
                                        start_lba, 
                                        block_count, 
                                        (FBE_PAYLOAD_BLOCK_OPERATION_FLAGS_METADATA_OP | 
                                         FBE_PAYLOAD_BLOCK_OPERATION_FLAGS_NO_SL_THIS_LEVEL));

    fbe_raid_group_setup_monitor_packet(raid_group_p, packet_p, &params);

    /* There are some cases where we are using the monitor context.
     * In those cases it is ok to send the I/O immediately.
     */
    fbe_base_config_bouncer_entry((fbe_base_config_t *)raid_group_p, packet_p);
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/**************************************
 * end fbe_raid_group_encryption_iw_check_alloc_completion()
 **************************************/

/*!**************************************************************
 * fbe_raid_group_encryption_iw_check()
 ****************************************************************
 * @brief
 *  Check if we need to write out an invalid stripe to handle the
 *  incomplete write case of encryption.
 *
 * @param raid_group_p
 * @param packet_p
 * 
 * @return fbe_status_t
 *
 * @author
 *  12/18/2013 - Created. Rob Foley 
 *  
 ****************************************************************/

fbe_status_t fbe_raid_group_encryption_iw_check(fbe_raid_group_t *raid_group_p, 
                                                fbe_packet_t *packet_p)
{
    fbe_memory_request_t*                memory_request_p = NULL;
    fbe_status_t                         status;
    fbe_scheduler_hook_status_t hook_status = FBE_SCHED_STATUS_OK;

    fbe_raid_group_check_hook(raid_group_p,
                              SCHEDULER_MONITOR_STATE_RAID_GROUP_ENCRYPTION,
                              FBE_RAID_GROUP_SUBSTATE_ENCRYPTION_IW_CHECK_START,
                              0, &hook_status);
    if (hook_status == FBE_SCHED_STATUS_DONE) {
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }

    memory_request_p = fbe_transport_get_memory_request(packet_p);

    status = fbe_memory_build_chunk_request(memory_request_p,
                                            FBE_MEMORY_CHUNK_SIZE_64_BLOCKS_64_BLOCKS,
                                            2,    /* One for sg and packet */
                                            0, /* Priority */
                                            0, /* Io stamp*/
                                            (fbe_memory_completion_function_t)fbe_raid_group_encryption_iw_check_alloc_completion,
                                            raid_group_p);
    if (status == FBE_STATUS_GENERIC_FAILURE) {

        memory_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,                                        
                             "iw invalidate Setup for memory allocation failed, status:0x%x, line:%d\n", status, __LINE__);  
        return FBE_STATUS_OK;
    }

    fbe_memory_request_entry(memory_request_p);
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/**************************************
 * end fbe_raid_group_encryption_iw_check
 **************************************/
/*!**************************************************************
 * fbe_raid_group_rekey_write_update_restart()
 ****************************************************************
 * @brief
 *  Restart writing the bits to say that the encryption write finished.
 *
 * @param packet_p
 * @param context
 * 
 * @return fbe_status_t
 *
 * @author
 *  4/28/2014 - Created. Rob Foley
 *  
 ****************************************************************/
fbe_raid_state_status_t
fbe_raid_group_rekey_write_update_restart(fbe_raid_iots_t *iots_p)
{
    fbe_status_t status;
    fbe_raid_group_t *raid_group_p = (fbe_raid_group_t*)iots_p->callback_context;
    fbe_packet_t *packet_p = iots_p->packet_p;

    status = fbe_transport_get_status_code(packet_p);

    fbe_base_object_trace((fbe_base_object_t *)raid_group_p, FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                          "Encryption write update restart iots: %p packet_status: 0x%x\n", iots_p, status);

    //  Remove the packet from the terminator queue.  We have to do this before any paged metadata access.
    fbe_raid_group_remove_from_terminator_queue(raid_group_p, packet_p);

    /* Perform the necessary bookkeeping to get this ready to restart.
     */
    fbe_raid_iots_set_callback(iots_p, fbe_raid_group_iots_completion, raid_group_p);
    fbe_raid_iots_mark_unquiesced(iots_p);
    fbe_raid_iots_reset_generate_state(iots_p);

    fbe_raid_group_rekey_write_update_paged(raid_group_p, packet_p);
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/******************************************
 * end fbe_raid_group_rekey_write_update_restart()
 ******************************************/
/*!**************************************************************
 * fbe_raid_group_rekey_write_update_completion()
 ****************************************************************
 * @brief
 *  Handle the completion of writing the paged with the 
 *  nr bits set for all positions.
 *
 * @param packet_p
 * @param context
 * 
 * @return fbe_status_t
 *
 * @author
 *  1/8/2014 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t
fbe_raid_group_rekey_write_update_completion(fbe_packet_t*  packet_p,
                                             fbe_packet_completion_context_t context)
{
    fbe_status_t status;
    fbe_raid_group_t *raid_group_p = (fbe_raid_group_t*)context;
    fbe_raid_position_bitmask_t degraded_mask;
    fbe_raid_iots_t *iots_p = NULL;
    fbe_payload_ex_t *payload_p = NULL;    
    fbe_bool_t packet_was_failed_b;

    payload_p = fbe_transport_get_payload_ex(packet_p);
    fbe_payload_ex_get_iots(payload_p, &iots_p);
    status = fbe_transport_get_status_code(packet_p);

    if (status != FBE_STATUS_OK){
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p, FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                              "Encryption write update iots: %p packet_status: 0x%x\n", iots_p, status);

        fbe_raid_group_executor_handle_metadata_error(raid_group_p, packet_p,
                                                      fbe_raid_group_rekey_write_update_restart, &packet_was_failed_b,
                                                      status);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }
    fbe_raid_group_get_degraded_mask(raid_group_p, &degraded_mask);

    /* Since all of the bits are now set, set only for drives that are not present.
     */
    iots_p->chunk_info[0].needs_rebuild_bits = degraded_mask;

    fbe_raid_group_limit_and_start_iots(packet_p, raid_group_p);
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/******************************************
 * end fbe_raid_group_rekey_write_update_completion()
 ******************************************/
/*!****************************************************************************
 * fbe_raid_group_rekey_write_update_paged()
 ******************************************************************************
 * @brief
 *   This function clears the degraded bit in the rekey position.
 * 
 * @param packet_p           - pointer to the packet 
 * @param raid_group_p       - pointer to a raid group object
 *
 * @return  fbe_status_t  
 *
 * @author
 *  1/6/2014 - Created. Rob Foley
 *
 ******************************************************************************/
fbe_status_t 
fbe_raid_group_rekey_write_update_paged(fbe_raid_group_t *raid_group_p,
                                        fbe_packet_t *packet_p)
{   
    fbe_raid_group_paged_metadata_t         paged_set_bits;
    fbe_status_t                            status;
    fbe_payload_ex_t*                       sep_payload_p = NULL;
    fbe_chunk_count_t                       chunk_count;
    fbe_payload_block_operation_t           *block_operation_p;
    fbe_lba_t                               start_lba; 
    fbe_lba_t                               end_lba;
    fbe_block_count_t                       block_count;
    fbe_chunk_index_t                       start_chunk_index;
    fbe_chunk_index_t                       end_chunk_index;    
    fbe_raid_iots_t*                        iots_p;
    fbe_u16_t                               data_disks;
    fbe_u32_t                               width;
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_group_get_raid_geometry(raid_group_p);
    sep_payload_p        = fbe_transport_get_payload_ex(packet_p);    
    block_operation_p = fbe_payload_ex_get_present_block_operation(sep_payload_p);
    fbe_payload_ex_get_iots(sep_payload_p, &iots_p);  

    start_lba = block_operation_p->lba;

    /* Since the iots block count may have been limited by the next marked extent,
     * use that to determine how many marks to clear. AR 536292
     */
    fbe_raid_iots_get_blocks(iots_p, &block_count);

    end_lba = start_lba + block_count - 1;
    
    fbe_raid_geometry_get_width(raid_geometry_p, &width);
    fbe_raid_geometry_get_data_disks(raid_geometry_p, &data_disks);
    start_lba /= data_disks;
    end_lba /= data_disks;
    // Clear all the rekey bits in this chunk
    fbe_raid_group_bitmap_get_chunk_index_for_lba(raid_group_p, start_lba, &start_chunk_index);
    fbe_raid_group_bitmap_get_chunk_index_for_lba(raid_group_p, end_lba, &end_chunk_index);    

    // Setup the completion function to be called when the paged metadata write completes
    fbe_transport_set_completion_function(packet_p, 
                                          fbe_raid_group_rekey_write_update_completion,
                                          raid_group_p);
    fbe_transport_set_completion_function(packet_p, 
                                          fbe_raid_group_bitmap_paged_metadata_block_op_completion,
                                          raid_group_p);

    // Initialize the bit data with 0 before read operation 
    fbe_set_memory(&paged_set_bits, 0, sizeof(fbe_raid_group_paged_metadata_t));
    chunk_count = (fbe_chunk_count_t)(end_chunk_index - start_chunk_index) + 1;
    
    /* set degraded for all positions.
     */
    paged_set_bits.needs_rebuild_bits = (1 << width) - 1;

    fbe_raid_group_trace(raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_RAID_GROUP_DEBUG_FLAG_ENCRYPTION_TRACING,
                         "enc set degraded chunk %llx to %llx\n", 
                         (unsigned long long)start_chunk_index, (unsigned long long)end_chunk_index);

    status = fbe_raid_group_bitmap_update_paged_metadata(raid_group_p,
                                                         packet_p,
                                                         start_chunk_index,
                                                         chunk_count,
                                                         &paged_set_bits,
                                                         FBE_FALSE /* not clear, set*/ );
    return status; 
}
/**************************************
 * end fbe_raid_group_rekey_write_update_paged()
 **************************************/

/*!**************************************************************
 * fbe_raid_group_rekey_iots_completion()
 ****************************************************************
 * @brief
 *   This is the completion function after we are done with rekey.
 *   This will clean up and complete the iots.
 * 
 * @param packet_p   - packet that is completing
 * @param context    - completion context, which is a pointer to 
 *                        the raid group object
 *
 * @return fbe_status_t.
 *
 * @author
 *  1/7/2014 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_raid_group_rekey_iots_completion(fbe_packet_t *packet_p,
                                                   fbe_packet_completion_context_t context)
{
    fbe_raid_group_t *raid_group_p = (fbe_raid_group_t*)context;

    /* Put the packet back on the termination queue since we had to take it off before reading the chunk info
     */
    fbe_raid_group_add_to_terminator_queue(raid_group_p, packet_p);

    fbe_raid_group_iots_start_next_piece(packet_p, raid_group_p);
    //fbe_raid_group_iots_cleanup(packet_p, raid_group_p);

    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/**********************************************************
 * end fbe_raid_group_rekey_iots_completion()
 **********************************************************/

/*!****************************************************************************
 * fbe_raid_group_monitor_run_vault_encryption()
 ******************************************************************************
 * @brief
 *  Start an encryption operation.
 *
 * @param object_p               - pointer to a base object 
 * @param packet_p               - pointer to a control packet from the scheduler
 *
 * @return fbe_lifecycle_status_t   - FBE_LIFECYCLE_STATUS_RESCHEDULE on success
 *                                  - FBE_LIFECYCLE_STATUS_PENDING when I/O is
 *                                    being issued 
 *                                  - FBE_LIFECYCLE_STATUS_DONE  when operation done
 *                                    without I/O is being issued
 * @author
 *
 ******************************************************************************/
fbe_lifecycle_status_t 
fbe_raid_group_monitor_run_vault_encryption(fbe_base_object_t *object_p,
                                            fbe_packet_t *packet_p)
{   
    fbe_raid_group_t*                   raid_group_p = (fbe_raid_group_t*) object_p;
    fbe_lba_t                           checkpoint;
    fbe_bool_t                          b_encryption_rekey_enabled;
    fbe_bool_t b_quiesce_needed;
    fbe_bool_t b_can_continue;
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_group_get_raid_geometry(raid_group_p);
    fbe_time_t last_io_time;
    fbe_u32_t elapsed_ms;
    fbe_scheduler_hook_status_t hook_status = FBE_SCHED_STATUS_OK;

    fbe_base_config_is_background_operation_enabled((fbe_base_config_t *) raid_group_p, 
                                                    FBE_RAID_GROUP_BACKGROUND_OP_ENCRYPTION_REKEY, &b_encryption_rekey_enabled);
    if ( b_encryption_rekey_enabled == FBE_FALSE ) {
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "Encryption vault run encryption: 0x%x is disabled \n",
                              FBE_RAID_GROUP_BACKGROUND_OP_ENCRYPTION_REKEY);
        return FBE_LIFECYCLE_STATUS_DONE;
    }
    if (!fbe_raid_geometry_is_attribute_set(raid_geometry_p, FBE_RAID_ATTRIBUTE_VAULT)){
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                              "Encryption vault run encryption: rg is not a vault\n");
        return FBE_LIFECYCLE_STATUS_DONE;
    }
    /* Only quiesce if we have not had I/O for a long time.
     */
    fbe_block_transport_server_get_last_io_time(&raid_group_p->base_config.block_transport_server,
                                                &last_io_time);
    elapsed_ms = fbe_get_elapsed_milliseconds(last_io_time);

    if (!fbe_raid_group_is_encryption_state_set(raid_group_p, FBE_RAID_GROUP_ENCRYPTION_FLAG_PAGED_ENCRYPTED) &&
        (elapsed_ms < fbe_raid_group_get_vault_wait_ms(raid_group_p))) {
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_INFO,
                              "Encryption vault %u ms since last I/O.  Wait for %llu ms\n",
                              elapsed_ms, fbe_raid_group_get_vault_wait_ms(raid_group_p));
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    fbe_raid_group_check_hook(raid_group_p,
                              SCHEDULER_MONITOR_STATE_RAID_GROUP_ENCRYPTION, 
                              FBE_RAID_GROUP_SUBSTATE_ENCRYPTION_VAULT_ENTRY, 
                              0, &hook_status);
    if (hook_status == FBE_SCHED_STATUS_DONE) {return FBE_LIFECYCLE_STATUS_DONE;}

    checkpoint = fbe_raid_group_get_rekey_checkpoint(raid_group_p);

    /* When we start and end rekey we need to quiesce. 
     * Quiesce is also needed when we switch from one drive to the next. 
     */
    fbe_raid_group_encryption_can_continue(raid_group_p, &b_quiesce_needed, &b_can_continue);

    if (b_quiesce_needed) {
        fbe_raid_group_encryption_flags_t flags;
        fbe_raid_group_encryption_get_state(raid_group_p, &flags);
        /* We need to perform an operation under quiesce.
         */
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "Encryption Quiesce Vault Raid Group state: 0x%x.\n", flags);
        /* Set this bit so the quiesce will first quiesce the bts. 
         * This enables us to quiesce with nothing in flight on the terminator queue and holding 
         * stripe locks. 
         */
        fbe_base_config_set_clustered_flag((fbe_base_config_t *) raid_group_p, 
                                           FBE_BASE_CONFIG_METADATA_MEMORY_FLAGS_QUIESCE_HOLD);
        fbe_lifecycle_set_cond(&fbe_raid_group_lifecycle_const, (fbe_base_object_t*) raid_group_p,
                               FBE_BASE_CONFIG_LIFECYCLE_COND_QUIESCE);
        fbe_lifecycle_set_cond(&fbe_raid_group_lifecycle_const, (fbe_base_object_t*) raid_group_p,
                               FBE_BASE_CONFIG_LIFECYCLE_COND_UNQUIESCE);
        fbe_lifecycle_set_cond(&fbe_raid_group_lifecycle_const, (fbe_base_object_t*) raid_group_p,
                               FBE_RAID_GROUP_LIFECYCLE_COND_REKEY_OPERATION);
        return FBE_LIFECYCLE_STATUS_DONE;
    } else if (!b_can_continue) {
        /* Must wait for something before rekey can continue.
         */
        return FBE_LIFECYCLE_STATUS_DONE;
    }
    return FBE_LIFECYCLE_STATUS_DONE;
}
/**************************************
 * end fbe_raid_group_monitor_run_vault_encryption()
 **************************************/

/*!**************************************************************
 * fbe_raid_group_vault_paged_reconstruct_completion()
 ****************************************************************
 * @brief
 *  Vault paged has been reconstructed.  Check status
 *  and continue by zeroing user area.
 *
 * @param packet_p
 * @param context
 * 
 * @return fbe_status_t
 *
 * @author
 *  1/20/2014 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t
fbe_raid_group_vault_paged_reconstruct_completion(fbe_packet_t*  packet_p,
                                                  fbe_packet_completion_context_t context)
{
    fbe_status_t status;
    fbe_raid_group_t *raid_group_p = (fbe_raid_group_t*)context;
    fbe_payload_ex_t *payload_p = NULL;

    payload_p = fbe_transport_get_payload_ex(packet_p);
    status = fbe_transport_get_status_code(packet_p);

    if (status != FBE_STATUS_OK){
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p, FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                              "reconstruct vault paged status packet_status: 0x%x\n", status);
    } else {
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "reconstruct vault paged status packet_status: 0x%x\n", status);
    }
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_raid_group_vault_paged_reconstruct_completion()
 ******************************************/

/*!**************************************************************
 * fbe_raid_group_vault_user_zero_completion()
 ****************************************************************
 * @brief
 *  Vault paged has been reconstructed.  Check status
 *  and continue by zeroing user area.
 *
 * @param packet_p
 * @param context
 * 
 * @return fbe_status_t
 *
 * @author
 *  1/20/2014 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t
fbe_raid_group_vault_user_zero_completion(fbe_packet_t*  packet_p,
                                          fbe_packet_completion_context_t context)
{
    fbe_status_t status;
    fbe_raid_group_t *raid_group_p = (fbe_raid_group_t*)context;
    fbe_payload_ex_t *payload_p = NULL;
    fbe_payload_block_operation_t *block_operation_p = NULL;
    
    payload_p = fbe_transport_get_payload_ex(packet_p);
    block_operation_p = fbe_payload_ex_get_block_operation(payload_p);
    status = fbe_transport_get_status_code(packet_p);
    fbe_payload_ex_release_block_operation(payload_p, block_operation_p);

    if ((status != FBE_STATUS_OK) ||
        (block_operation_p->status != FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS)){
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p, FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                              "vault user area zero status packet_status: 0x%x block_st: 0x%x\n", 
                              status, block_operation_p->status);
        return FBE_STATUS_OK;
    }
    fbe_base_object_trace((fbe_base_object_t *)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                          "vault user area zero status packet_status: 0x%x block_st: 0x%x\n", 
                          status, block_operation_p->status);

    /* Continue with reconstructing the paged.
     */
    fbe_transport_set_completion_function(packet_p, fbe_raid_group_vault_paged_reconstruct_completion, raid_group_p);

    fbe_raid_group_encrypt_paged(raid_group_p, packet_p);
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/******************************************
 * end fbe_raid_group_vault_user_zero_completion()
 ******************************************/

/*!**************************************************************
 * fbe_raid_group_zero_user_area()
 ****************************************************************
 * @brief
 *  Zero out the user area of the object.
 *
 * @param packet_p
 * @param context
 * 
 * @return fbe_status_t
 *
 * @author
 *  1/20/2014 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t
fbe_raid_group_zero_user_area(fbe_packet_t*  packet_p,
                              fbe_packet_completion_context_t context)
{
    fbe_status_t status;
    fbe_raid_group_t *raid_group_p = (fbe_raid_group_t*)context;
    fbe_payload_ex_t *payload_p = NULL;
    fbe_payload_block_operation_t *block_operation_p = NULL;
    fbe_lba_t capacity;

    payload_p = fbe_transport_get_payload_ex(packet_p);
    status = fbe_transport_get_status_code(packet_p);

    if (status != FBE_STATUS_OK){
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p, FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                              "check vault status packet_status: 0x%x\n", status);
        return FBE_STATUS_OK;
    }

    fbe_base_config_get_capacity((fbe_base_config_t*)raid_group_p, &capacity);

    block_operation_p = fbe_payload_ex_allocate_block_operation(payload_p);
    fbe_payload_block_build_operation(block_operation_p, 
                                      FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ENCRYPTION_REKEY_ZERO,
                                      0,
                                      capacity,
                                      FBE_BE_BYTES_PER_BLOCK, 1, /* optimal block size */ 
                                      NULL);
    /* Mark as md op so it does not get quiesced.
     */
    fbe_payload_block_set_flag(block_operation_p, (FBE_PAYLOAD_BLOCK_OPERATION_FLAGS_METADATA_OP | 
                                                   FBE_PAYLOAD_BLOCK_OPERATION_FLAGS_NO_SL_THIS_LEVEL));
    status = fbe_payload_ex_increment_block_operation_level(payload_p);
    fbe_transport_set_completion_function(packet_p, fbe_raid_group_vault_user_zero_completion, raid_group_p);

    /* We use the new key so that the zero data is encrypted with the new key.
     */
    fbe_base_config_bouncer_entry((fbe_base_config_t *)raid_group_p, packet_p);
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/******************************************
 * end fbe_raid_group_zero_user_area()
 ******************************************/
/*!**************************************************************
 * fbe_raid_group_check_vault_completion()
 ****************************************************************
 * @brief
 *  Checking the status of the vault has completed.
 *  If the vault is in use then clear the condition.
 *  Otherwise allow the vault to be reinitialized.
 *
 * @param packet_p
 * @param context
 * 
 * @return fbe_status_t
 *
 * @author
 *  1/20/2014 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t
fbe_raid_group_check_vault_completion(fbe_packet_t*  packet_p,
                                      fbe_packet_completion_context_t context)
{
    fbe_status_t status;
    fbe_raid_group_t *raid_group_p = (fbe_raid_group_t*)context;
    fbe_payload_ex_t *payload_p = NULL;
    fbe_payload_persistent_memory_operation_t* persistent_operation_p = NULL;
    fbe_memory_request_t *memory_request_p = NULL;
    fbe_scheduler_hook_status_t         hook_status = FBE_SCHED_STATUS_OK;

    memory_request_p = fbe_transport_get_memory_request(packet_p);
    fbe_memory_free_request_entry(memory_request_p);

    payload_p = fbe_transport_get_payload_ex(packet_p);
    status = fbe_transport_get_status_code(packet_p);
    persistent_operation_p = fbe_payload_ex_get_persistent_memory_operation(payload_p);

    fbe_payload_ex_release_persistent_memory_operation(payload_p, persistent_operation_p);
    if ((status != FBE_STATUS_OK) || 
        (persistent_operation_p->persistent_memory_status != FBE_PAYLOAD_PERSISTENT_MEMORY_STATUS_OK)){
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p, FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                              "check vault status packet_status: 0x%x persist_status: 0x%x\n", 
                              status,
                              persistent_operation_p->persistent_memory_status);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);

        fbe_raid_group_check_hook(raid_group_p,
                                  SCHEDULER_MONITOR_STATE_RAID_GROUP_ENCRYPTION,
                                  FBE_RAID_GROUP_SUBSTATE_ENCRYPTION_VAULT_BUSY,
                                  0, &hook_status);
    } else {
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p, FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_INFO,
                              "check vault packet_status: 0x%x\n", status);
    }
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_raid_group_check_vault_completion()
 ******************************************/
/*!**************************************************************
 * fbe_raid_group_check_vault_memory_alloc_completion()
 ****************************************************************
 * @brief
 *  Allocate memory that the persistent memory operation neeeds
 *  to check the vault status.
 *
 * @param memory_request_p
 * @param context
 * 
 * @return fbe_status_t
 *
 * @author
 *  1/20/2014 - Created. Rob Foley
 *
 ****************************************************************/
static fbe_status_t fbe_raid_group_check_vault_memory_alloc_completion(fbe_memory_request_t * memory_request_p, 
                                                                       fbe_memory_completion_context_t context)
{  
    fbe_packet_t                       *packet_p = NULL;
    fbe_payload_ex_t                   *payload_p = NULL;
    fbe_raid_group_t *raid_group_p = (fbe_raid_group_t *)context;
    fbe_memory_header_t *memory_header_p = NULL;
    fbe_u8_t *data_p = NULL;
    fbe_payload_persistent_memory_operation_t* persistent_operation_p = NULL;

    packet_p = fbe_transport_memory_request_to_packet(memory_request_p);
    payload_p = fbe_transport_get_payload_ex(packet_p);

    if (fbe_memory_request_is_allocation_complete(memory_request_p) == FBE_FALSE)
    {
        memory_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO, "%s allocation failed\n", __FUNCTION__);
        fbe_memory_free_request_entry(memory_request_p);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }
    memory_header_p = fbe_memory_get_first_memory_header(memory_request_p);
    data_p = fbe_memory_header_to_data(memory_header_p);

    persistent_operation_p = fbe_payload_ex_allocate_persistent_memory_operation(payload_p);

    fbe_payload_persistent_memory_build_check_vault(persistent_operation_p, data_p);

    fbe_payload_ex_increment_persistent_memory_operation_level(payload_p);

    fbe_transport_set_completion_function(packet_p, 
                                          fbe_raid_group_check_vault_completion,
                                          raid_group_p);
    fbe_persistent_memory_operation_entry(packet_p);
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/******************************************
 * end fbe_raid_group_check_vault_memory_alloc_completion()
 ******************************************/
/*!****************************************************************************
 * fbe_raid_group_check_vault()
 ******************************************************************************
 * @brief
 *  Check if the vault is in use.
 *
 *  This starts a state
 *
 * @param raid_group_p           - pointer to a base object 
 * @param packet_p               - pointer to a control packet from the scheduler
 *
 * @return fbe_lifecycle_status_t   - FBE_LIFECYCLE_STATUS_RESCHEDULE on success
 *                                  - FBE_LIFECYCLE_STATUS_PENDING when I/O is
 *                                    being issued 
 *                                  - FBE_LIFECYCLE_STATUS_DONE  when operation done
 *                                    without I/O is being issued
 * @author
 *
 ******************************************************************************/
fbe_status_t 
fbe_raid_group_check_vault(fbe_raid_group_t *raid_group_p,
                           fbe_packet_t *packet_p) 
{
    fbe_memory_request_t*                memory_request_p = NULL;
    fbe_status_t                         status;

    memory_request_p = fbe_transport_get_memory_request(packet_p);

    status = fbe_memory_build_chunk_request(memory_request_p,
                                            FBE_MEMORY_CHUNK_SIZE_64_BLOCKS_64_BLOCKS,
                                            1,    /* structures for persistent memory service */
                                            0, /* Priority */
                                            0, /* Io stamp*/
                                            (fbe_memory_completion_function_t)fbe_raid_group_check_vault_memory_alloc_completion,
                                            raid_group_p);
    if (status == FBE_STATUS_GENERIC_FAILURE) {

        memory_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,                                        
                             "Check vault Setup for mem alloc failed, status:0x%x, line:%d\n", status, __LINE__);  
        return FBE_STATUS_OK;
    }

    fbe_memory_request_entry(memory_request_p);
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/**************************************
 * end fbe_raid_group_check_vault
 **************************************/

/*!****************************************************************************
 * fbe_raid_group_reconstruct_vault()
 ******************************************************************************
 * @brief
 *  First check if the vault is in use.
 *  If not in use, then kick off the zero of the user area of the vault.
 *  Finally kick off the reconstruct of paged and the clearing of the journal.
 *
 *
 * @param raid_group_p           - pointer to a base object 
 * @param packet_p               - pointer to a control packet from the scheduler
 *
 * @return fbe_lifecycle_status_t   - FBE_LIFECYCLE_STATUS_RESCHEDULE on success
 *                                  - FBE_LIFECYCLE_STATUS_PENDING when I/O is
 *                                    being issued 
 *                                  - FBE_LIFECYCLE_STATUS_DONE  when operation done
 *                                    without I/O is being issued
 * @author
 *
 ******************************************************************************/
fbe_status_t 
fbe_raid_group_reinitialize_vault(fbe_raid_group_t *raid_group_p,
                                 fbe_packet_t *packet_p) 
{

    /* Continue with the zero of user capacity.
     */
    fbe_transport_set_completion_function(packet_p, fbe_raid_group_zero_user_area, raid_group_p);

    fbe_raid_group_check_vault(raid_group_p, packet_p);
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/**************************************
 * end fbe_raid_group_reconstruct_vault
 **************************************/

/*!****************************************************************************
 * fbe_raid_group_get_ds_encryption_state_alloc()
 ******************************************************************************
 * @brief
 *  Check downstream the encryption state.
 *
 * @param raid_group_p           - pointer to a base object 
 * @param packet_p               - pointer to a control packet from the scheduler
 *
 * @return fbe_lifecycle_status_t   - FBE_LIFECYCLE_STATUS_RESCHEDULE on success
 *                                  - FBE_LIFECYCLE_STATUS_PENDING when I/O is
 *                                    being issued 
 *                                  - FBE_LIFECYCLE_STATUS_DONE  when operation done
 *                                    without I/O is being issued
 * @author
 *  2/24/2014 - Created. Rob Foley
 *
 ******************************************************************************/
fbe_status_t fbe_raid_group_get_ds_encryption_state(fbe_raid_group_t *raid_group_p,
                                                    fbe_packet_t *packet_p)
{
    fbe_payload_ex_t                *sep_payload_p = NULL;
    fbe_payload_control_operation_t *new_control_operation_p = NULL;

    sep_payload_p = fbe_transport_get_payload_ex(packet_p);

    new_control_operation_p = fbe_payload_ex_allocate_control_operation(sep_payload_p);

    /* Add the control operation now.
     * We will allocate the memory for this once we allocate the subpackets.
     */
    fbe_payload_control_build_operation(new_control_operation_p,
                                        FBE_PROVISION_DRIVE_CONTROL_CODE_RECONSTRUCT_PAGED,
                                        NULL, /* Allocate it later. */
                                        sizeof(fbe_base_config_control_get_encryption_state_t));
    fbe_payload_ex_increment_control_operation_level(sep_payload_p);

    fbe_transport_set_completion_function(packet_p, fbe_raid_group_encrption_state_chk_completion, raid_group_p);

    fbe_raid_group_get_ds_encryption_state_alloc(raid_group_p, packet_p);

    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/**************************************
 * end fbe_raid_group_get_ds_encryption_state()
 **************************************/
/*!****************************************************************************
 * fbe_raid_group_get_ds_encryption_state_alloc()
 ******************************************************************************
 * @brief
 *  Check downstream the encryption state.
 *
 * @param raid_group_p           - pointer to a base object 
 * @param packet_p               - pointer to a control packet from the scheduler
 *
 * @return fbe_lifecycle_status_t   - FBE_LIFECYCLE_STATUS_RESCHEDULE on success
 *                                  - FBE_LIFECYCLE_STATUS_PENDING when I/O is
 *                                    being issued 
 *                                  - FBE_LIFECYCLE_STATUS_DONE  when operation done
 *                                    without I/O is being issued
 * @author
 *  2/21/2014 - Created. Rob Foley
 *
 ******************************************************************************/
fbe_status_t 
fbe_raid_group_get_ds_encryption_state_alloc(fbe_raid_group_t *raid_group_p,
                                             fbe_packet_t *packet_p) 
{
    fbe_status_t status;
    fbe_u32_t width;
    fbe_base_config_memory_allocation_chunks_t mem_alloc_chunks;

    /* get the width of the raid group object. */
    fbe_base_config_get_width((fbe_base_config_t *) raid_group_p, &width);

    /* We need to send the request down to all the edges and so get the width and allocate
     * packet for every edge */
    /* Set the various counts and buffer size */
    mem_alloc_chunks.buffer_count = width + 1;
    mem_alloc_chunks.number_of_packets = width;
    mem_alloc_chunks.sg_element_count = 0;
    mem_alloc_chunks.buffer_size = sizeof(fbe_base_config_control_get_encryption_state_t);
    mem_alloc_chunks.alloc_pre_read_desc = FALSE;
    mem_alloc_chunks.pre_read_desc_count = 0;

    /* allocate the subpackets to collect the zero checkpoint for all the edges. */
    status = fbe_base_config_memory_allocate_chunks((fbe_base_config_t *) raid_group_p,
                                                    packet_p,
                                                    mem_alloc_chunks,
                                                    (fbe_memory_completion_function_t)fbe_raid_group_get_ds_encryption_state_alloc_complete);    /* SAFEBUG - cast to supress warning but must fix because memory completion function shouldn't return status */
    if (status != FBE_STATUS_OK) {
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s memory allocation failed, status:0x%x\n", __FUNCTION__, status);
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return status; 
    }
    return status;
}
/**************************************
 * end fbe_raid_group_get_ds_encryption_state_alloc
 **************************************/

/*!****************************************************************************
 * fbe_raid_group_get_ds_encryption_state_alloc_complete()
 ******************************************************************************
 * @brief
 *  This function gets called after memory has been allocated and sends
 *  the request for encryption state to the PVDs.
 *
 * @param memory_request_p  - memory request.
 * @param context           - context passed to acquire the memory.
 *
 * @return status           - status of the operation.
 *
 * @author
 *  2/24/2014 - Created. Rob Foley
 *
 ******************************************************************************/
static fbe_status_t
fbe_raid_group_get_ds_encryption_state_alloc_complete(fbe_memory_request_t * memory_request_p,
                                                      fbe_memory_completion_context_t context) 
{
    fbe_packet_t *                                  packet_p = NULL;
    fbe_packet_t **                                 new_packet_p = NULL;
    fbe_payload_ex_t *                              sep_payload_p = NULL;
    fbe_payload_ex_t *                              new_sep_payload_p = NULL;
    fbe_payload_control_operation_t *               control_operation_p = NULL;
    fbe_payload_control_operation_t *               new_control_operation_p = NULL;
    fbe_status_t                                    status = FBE_STATUS_OK;
    fbe_raid_group_t *                              raid_group_p = NULL;
    fbe_u32_t                                       width = 0;
    fbe_u32_t                                       index = 0;
    fbe_block_edge_t *                              block_edge_p = NULL;
    fbe_base_config_memory_alloc_chunks_address_t   mem_alloc_chunks_addr = {0};
    fbe_sg_element_t                               *pre_read_sg_list_p = NULL;
    fbe_payload_pre_read_descriptor_t              *pre_read_desc_p = NULL;
    fbe_packet_attr_t                               packet_attr;
    fbe_u8_t                                      **buffer_p = NULL;
    fbe_base_config_control_get_encryption_state_t *get_encryption_state_p = NULL;

    /* get the pointer to original packet. */
    packet_p = fbe_transport_memory_request_to_packet(memory_request_p);

    /* get the pointer to raid group object. */
    raid_group_p = (fbe_raid_group_t *) context;

    /* get the payload and metadata operation. */
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p =  fbe_payload_ex_get_control_operation(sep_payload_p);

    /* Check allocation status */
    if (fbe_memory_request_is_allocation_complete(memory_request_p) == FBE_FALSE) {
        /* Currently this callback doesn't expect any aborted requests */
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s: memory request: 0x%p failed. request state: %d \n",
                              __FUNCTION__, memory_request_p, memory_request_p->request_state);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_FAILED;
    }

    /* get the width of the base config object. */
    fbe_base_config_get_width((fbe_base_config_t *) raid_group_p, &width);

    /* subpacket control buffer size. */
    mem_alloc_chunks_addr.buffer_size = sizeof(fbe_base_config_control_get_encryption_state_t);
    mem_alloc_chunks_addr.number_of_packets = width;
    mem_alloc_chunks_addr.buffer_count = width;
    mem_alloc_chunks_addr.sg_element_count = 0;
    mem_alloc_chunks_addr.sg_list_p = NULL;
    mem_alloc_chunks_addr.assign_pre_read_desc = FALSE;
    mem_alloc_chunks_addr.pre_read_sg_list_p = &pre_read_sg_list_p;
    mem_alloc_chunks_addr.pre_read_desc_p = &pre_read_desc_p;

    /* get subpacket pointer from the allocated memory. */
    status = fbe_base_config_memory_assign_address_from_allocated_chunks((fbe_base_config_t *) raid_group_p,
                                                                         memory_request_p,
                                                                         &mem_alloc_chunks_addr);
    if (status != FBE_STATUS_OK) {
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s memory distribution failed, status:0x%x, width:0x%x, buffer_size:0x%x\n",
                              __FUNCTION__, status, width, mem_alloc_chunks_addr.buffer_size);
        fbe_memory_free_request_entry(memory_request_p);

        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_INSUFFICIENT_RESOURCES);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return status;
    }

    /* Set good status.  We will set bad status on each individual bad completion.
     */
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_OK);

    new_packet_p = mem_alloc_chunks_addr.packet_array_p;
    buffer_p = mem_alloc_chunks_addr.buffer_array_p;

    while (index < width) {
        /* Initialize the sub packets. */
        fbe_transport_initialize_sep_packet(new_packet_p[index]);
        new_sep_payload_p = fbe_transport_get_payload_ex(new_packet_p[index]);
        get_encryption_state_p = (fbe_base_config_control_get_encryption_state_t *) buffer_p[index];
        get_encryption_state_p->encryption_state = FBE_BASE_CONFIG_ENCRYPTION_STATE_INVALID;

        /* allocate and initialize the control operation. */
        new_control_operation_p = fbe_payload_ex_allocate_control_operation(new_sep_payload_p);
        fbe_payload_control_build_operation(new_control_operation_p,
                                            FBE_PROVISION_DRIVE_CONTROL_CODE_RECONSTRUCT_PAGED,
                                            get_encryption_state_p,
                                            sizeof(fbe_base_config_control_get_encryption_state_t));

        /* Initialize the new packet with FBE_PACKET_FLAG_EXTERNAL same as the master packet */
        fbe_transport_get_packet_attr(packet_p, &packet_attr);
        if (packet_attr & FBE_PACKET_FLAG_EXTERNAL) {
            fbe_transport_set_packet_attr(new_packet_p[index], FBE_PACKET_FLAG_EXTERNAL);            
        }

        /* set completion functon. */
        fbe_transport_set_completion_function(new_packet_p[index],
                                              fbe_raid_group_encrption_state_chk_subpacket_completion,
                                              raid_group_p);

        /* add the subpacket to master packet queue. */
        fbe_transport_add_subpacket(packet_p, new_packet_p[index]);
        index++;
    }
    /* We will save the result in the master packet.
     */
    get_encryption_state_p = (fbe_base_config_control_get_encryption_state_t *) buffer_p[index];
    get_encryption_state_p->encryption_state = FBE_BASE_CONFIG_ENCRYPTION_STATE_INVALID;
    control_operation_p->buffer = get_encryption_state_p;
    fbe_payload_ex_increment_control_operation_level(sep_payload_p);

    /* send all the subpackets together. */
    for (index = 0; index < width; index++) {
        fbe_base_config_get_block_edge((fbe_base_config_t *)raid_group_p, &block_edge_p, index);
        fbe_transport_set_packet_attr(new_packet_p[index], FBE_PACKET_FLAG_TRAVERSE);
        fbe_block_transport_send_control_packet(block_edge_p, new_packet_p[index]);
    }

    /* send all the subpackets. */
     return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/******************************************************************************
 * end fbe_raid_group_get_ds_encryption_state_alloc_complete()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_raid_group_encrption_state_chk_subpacket_completion()
 ******************************************************************************
 * @brief
 *  This function is used to handle the subpacket completion for the registration
 *  function for all the downstream objects.
 *
 * @param memory_request_p  - memory request.
 * @param context           - context passed to acquire the memory.
 *
 * @return status           - status of the operation.
 *
 * @author
 *  10/30/2013 - Created. Rob Foley
 *
 ******************************************************************************/
static fbe_status_t 
fbe_raid_group_encrption_state_chk_subpacket_completion(fbe_packet_t * packet_p,
                                                          fbe_packet_completion_context_t context)
{
    fbe_raid_group_t *                                          raid_group_p = NULL;
    fbe_packet_t *                                              master_packet_p = NULL;
    fbe_payload_ex_t *                                         master_sep_payload_p = NULL;
    fbe_payload_control_operation_t *                           master_control_operation_p = NULL;
    fbe_payload_ex_t *                                         sep_payload_p = NULL;
    fbe_payload_control_operation_t *                           control_operation_p = NULL;
    fbe_payload_control_status_t                                control_status;
    fbe_payload_control_status_qualifier_t                      control_qualifier;
    fbe_status_t                                                status;
    fbe_bool_t is_empty;

    /* Cast the context to base config object pointer. */
    raid_group_p = (fbe_raid_group_t *) context;

    /* Get the master packet, its payload and associated control operation. */
    master_packet_p = fbe_transport_get_master_packet(packet_p);
    master_sep_payload_p = fbe_transport_get_payload_ex(master_packet_p);
    master_control_operation_p = fbe_payload_ex_get_control_operation(master_sep_payload_p);

    /* get the control operation of the subpacket. */
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(sep_payload_p);

    /* get the status of the subpacket operation. */
    status = fbe_transport_get_status_code(packet_p);
    fbe_payload_control_get_status(control_operation_p, &control_status);
    fbe_payload_control_get_status_qualifier(control_operation_p, &control_qualifier);

    /* if any of the subpacket failed with bad status then update the master status. */
    if (status != FBE_STATUS_OK) {
        fbe_transport_set_status(master_packet_p, status, 0);
    }

    /* if any of the subpacket failed with failure status then update the master status. */
    if (control_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_payload_control_set_status(master_control_operation_p, control_status);
        fbe_payload_control_set_status_qualifier(master_control_operation_p, control_qualifier);
        fbe_transport_set_status(master_packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
    }
    /* On success, check the encryption state we got back and 
     * save it in the master if it was the one we were looking for. 
     */
    if ((status == FBE_STATUS_OK) &&
        (control_status == FBE_PAYLOAD_CONTROL_STATUS_OK)) {
        fbe_base_config_control_get_encryption_state_t *master_encryption_state_p = NULL;
        fbe_base_config_control_get_encryption_state_t *encryption_state_p = NULL;
        fbe_payload_control_get_buffer(master_control_operation_p, &master_encryption_state_p);
        fbe_payload_control_get_buffer(control_operation_p, &encryption_state_p);

        if (encryption_state_p->encryption_state == FBE_BASE_CONFIG_ENCRYPTION_STATE_LOCKED_KEYS_INCORRECT) {
            master_encryption_state_p->encryption_state = FBE_BASE_CONFIG_ENCRYPTION_STATE_LOCKED_KEYS_INCORRECT;
        }
    }
    /* remove the sub packet from master queue. */
    fbe_transport_remove_subpacket_is_queue_empty(packet_p, &is_empty);

    /* when the queue is empty, all subpackets have completed. */
    if (is_empty) {
        fbe_transport_destroy_subpackets(master_packet_p);
        status = fbe_transport_get_status_code(master_packet_p);
        fbe_payload_control_get_status(master_control_operation_p, &control_status);
        /* When we start status is set to good.  When we get each completion we set bad status on failure.
         */
        fbe_transport_complete_packet(master_packet_p);
    } else {
        /* Not done with processing sub packets.
         */
    }
    return FBE_STATUS_MORE_PROCESSING_REQUIRED; 
}
/**************************************
 * end fbe_raid_group_encrption_state_chk_subpacket_completion()
 **************************************/

/*!****************************************************************************
 * fbe_raid_group_encrption_state_chk_completion()
 ******************************************************************************
 * @brief
 *  This function is used to handle the subpacket completion for the registration
 *  function for all the downstream objects.
 *
 * @param memory_request_p  - memory request.
 * @param context           - context passed to acquire the memory.
 *
 * @return status           - status of the operation.
 *
 * @author
 *  10/30/2013 - Created. Rob Foley
 *
 ******************************************************************************/
static fbe_status_t 
fbe_raid_group_encrption_state_chk_completion(fbe_packet_t * packet_p,
                                              fbe_packet_completion_context_t context)
{
    fbe_raid_group_t *                                          raid_group_p = NULL;
    fbe_payload_ex_t *                                          sep_payload_p = NULL;
    fbe_payload_control_operation_t *                           control_operation_p = NULL;
    fbe_payload_control_status_t                                control_status;
    fbe_payload_control_status_qualifier_t                      control_qualifier;
    fbe_status_t                                                status;
    fbe_base_config_control_get_encryption_state_t *encryption_state_p = NULL;

    /* get the status of the subpacket operation. */
    status = fbe_transport_get_status_code(packet_p);

    /* Cast the context to base config object pointer. */
    raid_group_p = (fbe_raid_group_t *) context;

    /* get the control operation of the subpacket. */
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(sep_payload_p);

    /* get the status of the subpacket operation. */
    status = fbe_transport_get_status_code(packet_p);
    fbe_payload_control_get_status(control_operation_p, &control_status);
    fbe_payload_control_get_status_qualifier(control_operation_p, &control_qualifier);
    fbe_payload_control_get_buffer(control_operation_p, &encryption_state_p);

    fbe_payload_ex_release_control_operation(sep_payload_p, control_operation_p);

    if ((status == FBE_STATUS_OK) &&
        (control_status == FBE_PAYLOAD_CONTROL_STATUS_OK) &&
        (encryption_state_p->encryption_state != FBE_BASE_CONFIG_ENCRYPTION_STATE_LOCKED_KEYS_INCORRECT)) {
        /* Continue with the next piece. 
         */ 
    } else {
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "raid_group: encryption state %d  status: 0x%x control_status: 0x%x\n",
                              encryption_state_p->encryption_state, status, control_status);
    }

    fbe_memory_free_request_entry(&packet_p->memory_request);

    fbe_transport_complete_packet(packet_p);
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/**************************************
 * end fbe_raid_group_encrption_state_chk_completion()
 **************************************/
/*!**************************************************************
 * fbe_raid_group_has_identical_keys()
 ****************************************************************
 * @brief
 *  Determine if both keys are valid and the same.
 *
 * @param raid_group_p - Current rg.  
 *
 * @return fbe_bool_t FBE_TRUE - Match
 *                   FBE_FALSE - No match.   
 *
 * @author
 *  4/10/2014 - Created. Rob Foley
 *
 ****************************************************************/

fbe_bool_t fbe_raid_group_has_identical_keys(fbe_raid_group_t *raid_group_p)
{
    fbe_u32_t index;

    if ((raid_group_p->base_config.key_handle != NULL) &&
        fbe_encryption_key_mask_both_valid(&raid_group_p->base_config.key_handle->encryption_keys[0].key_mask)){
        /* We have two valid keys.  Check if they match.
         */
        for (index = 0; index < FBE_ENCRYPTION_KEY_SIZE; index++) {
            if (raid_group_p->base_config.key_handle->encryption_keys[0].key1[index] !=
                raid_group_p->base_config.key_handle->encryption_keys[0].key2[index]) {
                /* Key mismatch found.
                 */
                return FBE_FALSE;
            }
        }
        /* Keys match.
         */
        return FBE_TRUE;
    } else {
        return FBE_FALSE;
    }
}
/******************************************
 * end fbe_raid_group_has_identical_keys()
 ******************************************/
/*!**************************************************************
 * fbe_raid_group_update_write_log_for_encryption()
 ****************************************************************
 * @brief
 *  Update the parity write log info flags based on the current
 *  state of encryption.
 *
 * @param raid_group_p - Current rg.  
 *
 * @return fbe_status_t
 *
 * @author
 *  5/23/2014 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_raid_group_update_write_log_for_encryption(fbe_raid_group_t *raid_group_p)
{
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_group_get_raid_geometry(raid_group_p);

    if (fbe_raid_geometry_is_parity_type(raid_geometry_p)) {
        if (fbe_raid_group_is_encryption_state_set(raid_group_p, FBE_RAID_GROUP_ENCRYPTION_FLAG_PAGED_ENCRYPTED)) {
            if (!fbe_raid_geometry_journal_flag_is_set(raid_geometry_p, FBE_PARITY_WRITE_LOG_FLAGS_ENCRYPTED)) {
                fbe_raid_geometry_journal_flag_set(raid_geometry_p, FBE_PARITY_WRITE_LOG_FLAGS_ENCRYPTED);
                 fbe_base_object_trace((fbe_base_object_t *)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                  "raid_group: Set parity write log encrypted.\n");
    
            }
        } else {
            if (fbe_raid_geometry_journal_flag_is_set(raid_geometry_p, FBE_PARITY_WRITE_LOG_FLAGS_ENCRYPTED)) {
                fbe_raid_geometry_journal_flag_clear(raid_geometry_p, FBE_PARITY_WRITE_LOG_FLAGS_ENCRYPTED);
                fbe_base_object_trace((fbe_base_object_t *)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                      "raid_group: Clear parity write log encrypted.\n");
            }
        }
    }
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_raid_group_update_write_log_for_encryption()
 ******************************************/


/*************************
 * end file fbe_raid_group_encryption.c
 *************************/


