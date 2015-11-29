/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_virtual_drive_copy.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the virtual drive proactive copy and equalize copy
 *  operation.
 * 
 * @ingroup virtual_drive_class_files
 * 
 * @version
 *   11/12/2009:  Created. Dhaval Patel
 *
 ***************************************************************************/


/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_event_log_api.h"                      //  for fbe_event_log_write
#include "fbe/fbe_event_log_utils.h"                    //  for message codes
#include "fbe_raid_group_rebuild.h"
#include "fbe_virtual_drive_private.h"
#include "fbe_notification.h"


/*****************************************
 * LOCAL FUNCTION PROTOTYPES
 *****************************************/
static fbe_status_t fbe_virtual_drive_copy_send_event_to_mark_needs_verify(fbe_virtual_drive_t *virtual_drive_p,
                                                                           fbe_packet_t *packet_p);

static fbe_status_t fbe_virtual_drive_copy_send_event_to_check_lba_completion(
                                        fbe_event_t*                    in_event_p,
                                        fbe_event_completion_context_t  in_context);


/*!****************************************************************************
 *          fbe_virtual_drive_get_copy_block_count()
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
static fbe_status_t fbe_virtual_drive_get_copy_block_count(fbe_virtual_drive_t *virtual_drive_p,
                                                           fbe_lba_t rebuild_lba,
                                                           fbe_block_count_t *block_count_p)
{
    fbe_raid_geometry_t    *raid_geometry_p = fbe_raid_group_get_raid_geometry((fbe_raid_group_t *)virtual_drive_p);
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
    fbe_raid_group_get_metadata_positions((fbe_raid_group_t *)virtual_drive_p, &raid_group_metadata_positions);

    /* Get the chunk size (rebuild request must be a multiple of chunk_size).
     */ 
    chunk_size = fbe_raid_group_get_chunk_size((fbe_raid_group_t *)virtual_drive_p);

    /* Get our default `logical' block consumption for rebuild request.
     */
    logical_block_count = (FBE_VIRTUAL_DRIVE_COPY_MBYTES_CONSUMPTION * (1024 * 1024)) / FBE_BYTES_PER_BLOCK;
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
    *block_count_p = ((fbe_block_count_t)chunk_count * (fbe_block_count_t)chunk_size);
    return FBE_STATUS_OK;
} 
/**********************************************
 * end fbe_virtual_drive_get_copy_block_count()
 **********************************************/

/*!****************************************************************************
 * fbe_virtual_drive_copy_initialize_rebuild_context
 ******************************************************************************
 * @brief
 *   This function is called by the raid group monitor to initialize the rebuild
 *   context before starting the rebuild operation.
 *
 * @param virtual_drive_p - Pointer to virtual drive object
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
fbe_status_t fbe_virtual_drive_copy_initialize_rebuild_context(fbe_virtual_drive_t *virtual_drive_p,
                                        fbe_raid_group_rebuild_context_t *in_rebuild_context_p, 
                                        fbe_raid_position_bitmask_t       in_positions_to_be_rebuilt,
                                        fbe_lba_t                         in_rebuild_lba)
{

    fbe_block_count_t   block_count;    // number of physical blocks to rebuild

    //  Get the chunk size, which is the number of blocks to rebuild  
    fbe_virtual_drive_get_copy_block_count(virtual_drive_p,
                                           in_rebuild_lba,
                                           &block_count);

    //  Set the rebuild data in the rebuild tracking structure 
    fbe_raid_group_rebuild_context_set_rebuild_start_lba(in_rebuild_context_p, in_rebuild_lba); 
    fbe_raid_group_rebuild_context_set_rebuild_block_count(in_rebuild_context_p, block_count);
    fbe_raid_group_rebuild_context_set_rebuild_positions(in_rebuild_context_p, in_positions_to_be_rebuilt);
    fbe_raid_group_rebuild_context_set_lun_object_id(in_rebuild_context_p, FBE_OBJECT_ID_INVALID);
    fbe_raid_group_rebuild_context_set_rebuild_is_lun_start(in_rebuild_context_p, FBE_FALSE);
    fbe_raid_group_rebuild_context_set_rebuild_is_lun_end(in_rebuild_context_p, FBE_FALSE);
    fbe_raid_group_rebuild_context_set_rebuild_state(in_rebuild_context_p, FBE_RAID_GROUP_REBUILD_STATE_UNKNOWN);
    fbe_raid_group_rebuild_context_set_update_checkpoint_bitmask(in_rebuild_context_p, 0);
    fbe_raid_group_rebuild_context_set_update_checkpoint_lba(in_rebuild_context_p, FBE_LBA_INVALID);
    fbe_raid_group_rebuild_context_set_update_checkpoint_blocks(in_rebuild_context_p, 0);

    //  Return success
    return FBE_STATUS_OK;

}
/*********************************************************
 * end fbe_virtual_drive_copy_initialize_rebuild_context()
 *********************************************************/

/*!****************************************************************************
 * @fn fbe_virtual_drive_rebuild_process_rebuild_io_completion()
 ******************************************************************************
 * @brief
 *  This function is used to handle the rebuild i/o completion for the virtual
 *  drive object's rebuild (copy) i/o operation.
 *
 * @param virtual_drive_p   - Pointer to the virtual drive object.
 * @param packet_p          - Pointer the the I/O packet.
 *
 * @return fbe_status_t.
 *
 * @author
 *  4/28/2009 - Created. Dhaval Patel
 *
 ******************************************************************************/
fbe_status_t fbe_virtual_drive_rebuild_process_io_completion(fbe_packet_t *packet_p,
                                                             fbe_packet_completion_context_t *context)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_status_t                            packet_status;
    fbe_virtual_drive_t                    *virtual_drive_p;
    fbe_payload_ex_t                       *payload_p;
    fbe_payload_block_operation_t          *block_operation_p;
    fbe_payload_block_operation_status_t    block_status = FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID;
    fbe_payload_block_operation_qualifier_t block_qualifier;
    fbe_lba_t                               lba;
    fbe_block_count_t                       block_count;
    fbe_lba_t                               exported_capacity;

    /* Cast the context to a pointer to a virtual drive object.
     */
    virtual_drive_p = (fbe_virtual_drive_t *)context;

    /* Get the exported capacity of virtual drive object.
     */
    exported_capacity = fbe_raid_group_get_exported_disk_capacity((fbe_raid_group_t *)virtual_drive_p);

    /* Get the status of the operation that just completed.
     */
    packet_status = fbe_transport_get_status_code(packet_p);

    /* Get the payload and block operation for this I/O operation.
     */
    payload_p         = fbe_transport_get_payload_ex(packet_p);
    block_operation_p = fbe_payload_ex_get_block_operation(payload_p);

    /* Get the block operation status.
     */
    fbe_payload_block_get_status(block_operation_p, &block_status);
    fbe_payload_block_get_qualifier(block_operation_p, &block_qualifier);

    /* Get the start lba and block count of the block operaiton. 
     */
    fbe_payload_block_get_lba(block_operation_p, &lba);
    fbe_payload_block_get_block_count(block_operation_p, &block_count);

    /* Release the block operation.
     */
    fbe_payload_ex_release_block_operation(payload_p, block_operation_p);

    /* Trace if enabled
     */
    fbe_virtual_drive_trace(virtual_drive_p,
                            FBE_TRACE_LEVEL_INFO,
                            FBE_VIRTUAL_DRIVE_DEBUG_FLAG_REBUILD_TRACING,
                            "virtual_drive: rebuild io complete lba: 0x%llx blks: 0x%llx sts: 0x%x sts/qual: %d/%d \n",
                            (unsigned long long)lba,
                (unsigned long long)block_count, packet_status,
                block_status, block_qualifier);

    /* Check for any errors.
     */
    if ((packet_status != FBE_STATUS_OK) || (block_status != FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS))
    {
        /* We expect that errors may occur so only trace an information message.
         */
        fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p, 
                              FBE_TRACE_LEVEL_INFO, 
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "virtual_drive: rebuild io failed lba: 0x%012llx blks: 0x%08llx sts: 0x%x sts/qual: %d/%d \n",
                              (unsigned long long)lba,
                  (unsigned long long)block_count, packet_status,
                  block_status, block_qualifier);  
        
        /* Virtual drive object will handle only media errors by asking RAID to mark
         * needs rebuild for this range and so ignore all other errors.  Also if the
         * error occurred in the paged metadata the raid group executor will handle
         * the error (simply fail the rebuild request).
         */
        if ((block_status != FBE_PAYLOAD_BLOCK_OPERATION_STATUS_MEDIA_ERROR) ||
            (lba >= exported_capacity)                                          )
        {
            /* Since we have released the block operation mark the packet
             * packet status as failed.  The rebuild process will decide how to
             * proceed.
             */
            if (packet_status == FBE_STATUS_OK)
            {
                /* Replace the packet status with `non-retryable' so that we
                 * don't advance the checkpoint.
                 */
                packet_status = FBE_STATUS_IO_FAILED_NOT_RETRYABLE;
                fbe_transport_set_status(packet_p, packet_status, 0);
            }

            /* Since we processed the request return success.
             */
            return FBE_STATUS_OK;
        }
        else
        {
            /* Allocate the event to ask upstream object to mark the drive as 
             * needs rebuild. 
             */
            status = fbe_virtual_drive_copy_send_event_to_mark_needs_verify(virtual_drive_p, packet_p);
            if (status != FBE_STATUS_OK)
            {
                /* Generate an error trace but we always proceed so that we 
                 * don't hold the monitor.
                 */
                fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                                      FBE_TRACE_LEVEL_ERROR,
                                      FBE_TRACE_MESSAGE_ID_INFO,
                                      "virtual_drive: rebuild io complete lba: 0x%llx blks: 0x%llx mark verify failed - status: 0x%x\n",
                                      (unsigned long long)lba, (unsigned long long)block_count, status);
            }

            /* Even if we failed to makr needs verify we continue.
             */
            return FBE_STATUS_OK;

        } /* end else a media error was detected in the user area*/

    } /* end if there was an error */

    /* Return success.
     */
    return FBE_STATUS_OK;
}
/********************************************************
 * end fbe_virtual_drive_rebuild_process_io_completion()
*********************************************************/

/*!****************************************************************************
 *          fbe_virtual_drive_copy_send_io_request()
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
fbe_status_t fbe_virtual_drive_copy_send_io_request(fbe_virtual_drive_t *virtual_drive_p,
                                                    fbe_packet_t *packet_p,
                                                    fbe_bool_t b_queue_to_block_transport,
                                                    fbe_lba_t start_lba,
                                                    fbe_block_count_t block_count)
{

    fbe_traffic_priority_t              io_priority;                    //  priority for the I/O we are doing 
    fbe_status_t                        status;                         //  fbe status
    fbe_raid_group_rebuild_context_t   *rebuild_context_p;              // pointer to the rebuild context


    //  Trace function entry 
    fbe_base_object_trace((fbe_base_object_t*)virtual_drive_p,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n", __FUNCTION__);

    //  Get a pointer to the rebuild context from the packet
    fbe_raid_group_rebuild_get_rebuild_context((fbe_raid_group_t *)virtual_drive_p, packet_p, &rebuild_context_p);

    //  Set the rebuild operation state to rebuild i/o
    fbe_raid_group_rebuild_context_set_rebuild_state(rebuild_context_p, FBE_RAID_GROUP_REBUILD_STATE_REBUILD_IO);

    //  Set the rebuild I/O completion function
    fbe_transport_set_completion_function(packet_p, 
                                          (fbe_packet_completion_function_t)fbe_virtual_drive_rebuild_process_io_completion, 
                                          (fbe_packet_completion_context_t)virtual_drive_p);

    //  Set the rebuild priority in the packet
    io_priority = fbe_raid_group_get_rebuild_priority((fbe_raid_group_t *)virtual_drive_p);
    fbe_transport_set_traffic_priority(packet_p, io_priority);

    /* If this method is invoked from the event thread we must break the
     * context (to free the event thread).
     */
    if (b_queue_to_block_transport == FBE_TRUE)
    {
        /* Break the context before starting the I/O.
         */
        status = fbe_raid_group_executor_break_context_send_monitor_packet((fbe_raid_group_t *)virtual_drive_p,
                                                                           packet_p,
                                                                           FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_REBUILD, 
                                                                           start_lba, 
                                                                           block_count);
    }
    else
    {
        /* Else we can start the I/O immediately.
         */
        status = fbe_raid_group_executor_send_monitor_packet((fbe_raid_group_t *)virtual_drive_p,
                                                             packet_p,
                                                             FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_REBUILD, 
                                                             start_lba, 
                                                             block_count);
    }

    //  Return status from the call
    return status;

}
/*************************************************
 * end fbe_virtual_drive_copy_send_io_request()
 *************************************************/
 
/*!****************************************************************************
 * fbe_virtual_drive_copy_send_event_to_check_lba()
 ******************************************************************************
 * @brief
 *   This function will send an event in order to determine if the LBA and range
 *   to be rebuilt is within a LUN.  The event's completion function will proceed
 *   accordingly.  However, before sending the event, it will first check if the 
 *   LBA is outside of the RG's data area and instead is in the paged metadata.  
 *   In that case, it will proceed directly to the next step which is to send i/o request.
 *
 * @param virtual_drive_p - Pointer to virtual drive
 * @param rebuild_context_p - Pointer to rebuild context
 * @param packet_p - pointer to a control packet from the scheduler
 * @return fbe_status_t
 * 
 * @author
 *  05/10/2010 - Created. Jean Montes.
 *
 *****************************************************************************/
fbe_status_t fbe_virtual_drive_copy_send_event_to_check_lba(fbe_virtual_drive_t *virtual_drive_p,
                                                         fbe_raid_group_rebuild_context_t *rebuild_context_p,
                                                         fbe_packet_t *packet_p)
{

    fbe_event_t*                        event_p;                // pointer to event
    fbe_event_stack_t*                  event_stack_p;          // pointer to event stack
    fbe_raid_geometry_t*                raid_geometry_p;        // pointer to raid geometry info 
    fbe_u16_t                           data_disks;             // number of data disks in the rg
    fbe_lba_t                           logical_start_lba;      // start LBA relative to the whole RG extent
    fbe_block_count_t                   logical_block_count;    // block count relative to the whole RG extent
    fbe_lba_t                           external_capacity;      // capacity of the RG available for data 
    fbe_event_permit_request_t          permit_request_info;    // gets LUN info from the permit request
    fbe_medic_action_priority_t         medic_action_priority;
    fbe_block_count_t                   io_block_count;

    //  Trace function entry 
    fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,    
                          FBE_TRACE_LEVEL_DEBUG_HIGH,         
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n", __FUNCTION__);        

    //  Get the external capacity of the RG, which is the area available for data and does not include the paged
    //  metadata or signature area.  This is stored in the base config's capacity.
    fbe_base_config_get_capacity((fbe_base_config_t*)virtual_drive_p, &external_capacity);

    //  We want to check the start LBA against the external capacity.  When comparing it to the external capacity 
    //  or sending it up to the LUN, we need to use the "logical" LBA relative to the start of the RG.  The LBA
    //  passed in to this function is what is used by rebuild and is relative to the start of the disk, so convert
    //  it. 
    raid_geometry_p = fbe_raid_group_get_raid_geometry((fbe_raid_group_t *)virtual_drive_p);
    fbe_raid_geometry_get_data_disks(raid_geometry_p, &data_disks);
    logical_start_lba   = rebuild_context_p->start_lba * data_disks; 
    logical_block_count = rebuild_context_p->block_count * data_disks;

    /* Trace some information if enabled.
     */
    fbe_virtual_drive_trace(virtual_drive_p,
                            FBE_TRACE_LEVEL_INFO,
                            FBE_VIRTUAL_DRIVE_DEBUG_FLAG_REBUILD_TRACING,
                            "virtual_drive: permit request for lba: 0x%llx blocks: 0x%llx exported: 0x%llx data_disks: %d\n",
                            (unsigned long long)logical_start_lba,
                (unsigned long long)logical_block_count,
                (unsigned long long)external_capacity, data_disks);

    //  If the LBA to check is outside of (greater than) the external capacity, then it is in the paged metadata 
    //  area and we do not need to check if it is in a LUN.  We'll go directly to sending the rebuild I/O.  
    if (logical_start_lba >= external_capacity)
    {
        // Send the rebuild I/O .  Its completion function will execute when it finishes. 
        fbe_virtual_drive_copy_send_io_request(virtual_drive_p, packet_p, 
                                               FBE_FALSE, /* Don't need to break the context. */
                                               rebuild_context_p->start_lba, 
                                               rebuild_context_p->block_count);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED; 
    }

    /* Make sure we limit the event to one client. 
     * The permit request event is not capable of dealing with sending back an unconsumed block count if 
     * the request needs to get limited multiple times. 
     */
    io_block_count = rebuild_context_p->block_count;
    fbe_raid_group_monitor_update_client_blk_cnt_if_needed((fbe_raid_group_t*)virtual_drive_p, 
                                                           rebuild_context_p->start_lba, &io_block_count);

    if((io_block_count != FBE_LBA_INVALID) && (rebuild_context_p->block_count != io_block_count))
    {
        fbe_base_object_trace((fbe_base_object_t*) virtual_drive_p, 
                              FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO, 
                              "VD: send rebuild permit s_lba 0x%llx, orig blk 0x%llx, new blk 0x%llx \n", 
                              rebuild_context_p->start_lba,
                              rebuild_context_p->block_count, io_block_count);
        
        /* update the rebuild block count so that permit request will limit to only one client */               
        rebuild_context_p->block_count = io_block_count;
        logical_block_count = rebuild_context_p->block_count * data_disks;
        
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

    /* Set medic action priority. */
    fbe_medic_get_action_priority(FBE_MEDIC_ACTION_COPY, &medic_action_priority);
    fbe_event_set_medic_action_priority(event_p, medic_action_priority);

    //  Set the completion function in the event's stack data
    fbe_event_stack_set_completion_function(event_stack_p, 
                                            fbe_virtual_drive_copy_send_event_to_check_lba_completion, 
                                            virtual_drive_p);

    //  Send a "permit request" event to check if this LBA and block are in a LUN or not  
    fbe_base_config_send_event((fbe_base_config_t*)virtual_drive_p, FBE_EVENT_TYPE_PERMIT_REQUEST, event_p);

    //  Return success   
    return FBE_STATUS_OK;

}
/*********************************************************
 * end fbe_virtual_drive_copy_send_event_to_check_lba()
 *********************************************************/

/*!****************************************************************************
 * fbe_virtual_drive_copy_send_event_to_check_lba_completion()
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
 *  10/26/2012  Ron Proulx  - Updated.
 *
 *****************************************************************************/
static fbe_status_t fbe_virtual_drive_copy_send_event_to_check_lba_completion(
                                        fbe_event_t*                    in_event_p,
                                        fbe_event_completion_context_t  in_context)
{

    fbe_virtual_drive_t                *virtual_drive_p;                //  pointer to the virtual drive object
    fbe_event_stack_t                  *event_stack_p;                  //  event stack pointer
    fbe_event_status_t                  event_status;                   //  event status
    fbe_u32_t                           event_flags;                    // event flags
    fbe_packet_t                       *packet_p;                       //  pointer to fbe packet
    fbe_event_permit_request_t          permit_request_info;            //  gets LUN info from the permit request
    fbe_lba_t                           original_start_lba;             //  original (physical) rebuild start lba
    fbe_block_count_t                   original_block_count;           //  original rebuild block count
    fbe_lba_t                           updated_start_lba;              //  updated (physical) rebuild start lba
    fbe_block_count_t                   updated_block_count;            //  updated rebuild block count
    fbe_lba_t                           next_start_lba;                 //  next starting lba (physical)
    fbe_raid_position_bitmask_t         rebuild_bitmask;                //  bitmaks of positions to rebuild
    fbe_block_count_t                   unconsumed_block_count = 0;     //  unconsumed block count
    fbe_bool_t                          b_is_next_request_consumed = FBE_FALSE; // Is the next request to a consumed area or not
    fbe_raid_group_rebuild_context_t   *rebuild_context_p;              //  pointer to the rebuild context
    fbe_status_t                        status;                         //  status of the operation
    fbe_scheduler_hook_status_t         hook_status;
    fbe_raid_geometry_t                *raid_geometry_p;
    fbe_u16_t                           data_disks;
    fbe_chunk_size_t                    chunk_size;
    fbe_lba_t                           exported_capacity;
    fbe_virtual_drive_flags_t           vd_flags;

    //  Cast the context to a pointer to a virtual drive object 
    virtual_drive_p = (fbe_virtual_drive_t *)in_context;

    //  Trace function entry 
    fbe_base_object_trace((fbe_base_object_t*)virtual_drive_p,      
                          FBE_TRACE_LEVEL_DEBUG_HIGH,           
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,  
                          "%s entry\n", __FUNCTION__);          

    // Get the geometry
    raid_geometry_p = fbe_raid_group_get_raid_geometry((fbe_raid_group_t *)virtual_drive_p);
    fbe_raid_geometry_get_data_disks(raid_geometry_p, &data_disks);
    chunk_size = fbe_raid_group_get_chunk_size((fbe_raid_group_t *)virtual_drive_p);
    exported_capacity = fbe_raid_group_get_exported_disk_capacity((fbe_raid_group_t *)virtual_drive_p);
    fbe_virtual_drive_get_flags(virtual_drive_p, &vd_flags);

    //  Get the data that we'll need out of the event - stack, status, packet pointer 
    fbe_event_get_flags(in_event_p, &event_flags);
    event_stack_p = fbe_event_get_current_stack(in_event_p);
    fbe_event_get_status(in_event_p, &event_status);
    fbe_event_get_master_packet(in_event_p, &packet_p);

    //  Get a pointer to the rebuild context from the packet
    fbe_raid_group_rebuild_get_rebuild_context((fbe_raid_group_t *)virtual_drive_p, 
                                               packet_p, 
                                               &rebuild_context_p);
    original_start_lba = rebuild_context_p->start_lba;
    updated_start_lba = original_start_lba;
    original_block_count = rebuild_context_p->block_count;
    updated_block_count = original_block_count;
    rebuild_bitmask = rebuild_context_p->rebuild_bitmask;

    //  Get the permit request info about the LUN (object id and start/end of LUN) and store it in the rebuild
    //  tracking structure
    fbe_event_get_permit_request_data(in_event_p, &permit_request_info); 
    rebuild_context_p->lun_object_id     = permit_request_info.object_id;
    rebuild_context_p->is_lun_start_b    = permit_request_info.is_start_b;
    rebuild_context_p->is_lun_end_b      = permit_request_info.is_end_b;
    unconsumed_block_count               = permit_request_info.unconsumed_block_count;
    next_start_lba                       = original_start_lba + original_block_count;

    /*! @note Free all associated resources and release the event.
     *        Do not access event information after this point.
     */
    fbe_event_release_stack(in_event_p, event_stack_p);
    fbe_event_destroy(in_event_p);
    fbe_memory_ex_release(in_event_p);

    /* Trace some information if enalbed.
     */
    fbe_virtual_drive_trace(virtual_drive_p,
                            FBE_TRACE_LEVEL_INFO,
                            FBE_VIRTUAL_DRIVE_DEBUG_FLAG_REBUILD_TRACING,
                            "virtual_drive: rb permit status: %d flags: 0x%x obj: 0x%x bst: %d bend: %d lba: 0x%llx uncnsmd: 0x%llx \n",
                            event_status, event_flags, rebuild_context_p->lun_object_id, 
                            rebuild_context_p->is_lun_start_b, rebuild_context_p->is_lun_end_b, 
                            rebuild_context_p->start_lba, unconsumed_block_count);

    /* Complete the packet if permit request denied or returned busy 
     */
    if (fbe_virtual_drive_is_flag_set(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_COPY_MARK_VERIFY_IN_PROGRESS) ||
        (event_flags == FBE_EVENT_FLAG_DENY)                                                                || 
        (event_status == FBE_EVENT_STATUS_BUSY)                                                                )
    {
        if (event_flags & FBE_EVENT_FLAG_DENY)
        {
            /* This hook can only log from here, so ignore the status */
            fbe_virtual_drive_check_hook(virtual_drive_p,
                                         SCHEDULER_MONITOR_STATE_RAID_GROUP_REBUILD, 
                                         FBE_RAID_GROUP_SUBSTATE_REBUILD_NO_UPSTREAM_PERMISSION, 
                                         0, &hook_status);
        }

        /* Trace some information.
         */
        fbe_virtual_drive_trace(virtual_drive_p,
                                FBE_TRACE_LEVEL_INFO,
                                FBE_VIRTUAL_DRIVE_DEBUG_FLAG_REBUILD_TRACING,
                                "virtual_drive: rb permit denied. lba: 0x%llx blks: 0x%llx status: %d flags: 0x%x vd flags: 0x%x\n",
                                (unsigned long long)rebuild_context_p->start_lba, (unsigned long long)rebuild_context_p->block_count, 
                                event_status, event_flags, vd_flags);

        /* Our client is busy, we will need to try again later, we can't do the 
         * rebuild now. Complete the packet with `busy' status.
         */
        fbe_transport_set_status(packet_p, FBE_STATUS_BUSY, 0);
        fbe_transport_complete_packet(packet_p);

        return FBE_STATUS_OK;
    }

    /* Handle all the possible event status.
     */
    switch(event_status)
    {
        case FBE_EVENT_STATUS_OK:
            /* check if the rebuild IO range is overlap with unconsumed area 
             * either at beginning or at the end.
             */
            status = fbe_raid_group_monitor_io_get_consumed_info((fbe_raid_group_t *)virtual_drive_p,
                                                                 unconsumed_block_count,
                                                                 original_start_lba, 
                                                                 &updated_block_count,
                                                                 &b_is_next_request_consumed);
            if (status != FBE_STATUS_OK)
            {
                fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p, 
                                      FBE_TRACE_LEVEL_ERROR, 
                                      FBE_TRACE_MESSAGE_ID_INFO, 
                                      "virtual_drive: rb permit lba: 0x%llx blks: 0x%llx get consumed failed - status: 0x%x\n",
                                      original_start_lba, original_block_count, status);

                fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
                fbe_transport_complete_packet(packet_p);
                return FBE_STATUS_OK;
            }

            /* If the next request is consumed get the updated block count.
             */
            if (b_is_next_request_consumed)
            {
                /* If we need to break up the rebuild request.
                 */
                if (updated_block_count != original_block_count)
                {
                    /* IO range is consumed at the beginning and unconsumed at the end
                     * update the rebuild block count with updated block count which only cover the 
                     * consumed area
                     */
                    fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                                          FBE_TRACE_LEVEL_INFO,
                                          FBE_TRACE_MESSAGE_ID_INFO,
                                          "virtual_drive: rb permit orig lba: 0x%llx blks: 0x%llx updated blks: 0x%llx \n",
                                          (unsigned long long)original_start_lba, (unsigned long long)original_block_count, (unsigned long long)updated_block_count);

                    /* Change the block count for this request.
                     */
                    rebuild_context_p->block_count = updated_block_count;
                }
            }
            else
            {
                /* IO range is unconsumed at the beginning and consumed at the end
                 * so can't continue with rebuild
                 * clear the NR bits in the paged metadata and advance the checkpoint.
                 */
                fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                                      FBE_TRACE_LEVEL_INFO,
                                      FBE_TRACE_MESSAGE_ID_INFO,
                                      "virtual_drive: rb permit consumed orig lba: 0x%llx blks: 0x%llx skip blks: 0x%llx \n",
                                      original_start_lba, original_block_count, updated_block_count);
                
                /* Skip over the `unconsumed' blocks.
                 */
                rebuild_context_p->block_count = updated_block_count;
                status = fbe_raid_group_clear_nr_control_operation((fbe_raid_group_t *)virtual_drive_p, packet_p);
                return FBE_STATUS_MORE_PROCESSING_REQUIRED;
            }

            break;

        case FBE_EVENT_STATUS_NO_USER_DATA:
            /* There is no user data in the entire rebuild request range. Find
             * out the unconsumed block counts that needs to skip with advance 
             * rebuild checkpoint.
             */
            status = fbe_raid_group_monitor_io_get_unconsumed_block_counts((fbe_raid_group_t *)virtual_drive_p, 
                                                                           unconsumed_block_count, 
                                                                           original_start_lba,
                                                                           &updated_block_count);
            if (status != FBE_STATUS_OK)
            {
                fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p, 
                                      FBE_TRACE_LEVEL_ERROR, 
                                      FBE_TRACE_MESSAGE_ID_INFO, 
                                      "virtual_drive: rb permit lba: 0x%llx blks: 0x%llx get consumed failed - status: 0x%x\n",
                                      original_start_lba, original_block_count, status);

                fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
                fbe_transport_complete_packet(packet_p);
                return FBE_STATUS_OK;
            }

            /* Trace some information and skip over the unconsumed area.
             */
            fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "virtual_drive: rb permit unconsumed orig lba: 0x%llx blks: 0x%llx skip blks: 0x%llx \n",
                                  original_start_lba, original_block_count, updated_block_count);

            /* Set the updated block count to advance rebuild checkpoint 
             */
            rebuild_context_p->block_count = updated_block_count;

            /* If the area is not consumed by an upstream object, then we do 
             * not need to rebuild it.  Instead, clear the NR bits in the paged 
             * metadata and advance the checkpoint.
             */
            status = fbe_raid_group_clear_nr_control_operation((fbe_raid_group_t *)virtual_drive_p, packet_p);
            return FBE_STATUS_MORE_PROCESSING_REQUIRED;

        default:
            /* Unexpected event status.
             */
            fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p, 
                                  FBE_TRACE_LEVEL_ERROR, 
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                                  "%s unexpected event status: %d\n", __FUNCTION__, event_status);
            fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
            fbe_transport_complete_packet(packet_p);
            return FBE_STATUS_OK;

    } /* end if switch on event_status*/

    /* This hook can only log from here, so ignore the status */
    fbe_virtual_drive_check_hook(virtual_drive_p,  
                                 SCHEDULER_MONITOR_STATE_RAID_GROUP_REBUILD, 
                                 FBE_RAID_GROUP_SUBSTATE_REBUILD_SEND, 
                                 rebuild_context_p->start_lba, &hook_status);

    /* Send the rebuild I/O .  Its completion function will execute when it 
     * finishes.
     */ 
    status = fbe_virtual_drive_copy_send_io_request(virtual_drive_p, packet_p,
                                                    FBE_TRUE, /* We must break the context to release the event thread. */ 
                                                    rebuild_context_p->start_lba, 
                                                    rebuild_context_p->block_count);

    return status;

}
/********************************************************************
 * end fbe_virtual_drive_rebuild_send_event_to_check_lba_completion()
 ********************************************************************/

/*!****************************************************************************
 *          fbe_virtual_drive_copy_mark_needs_verify_event_completion()
 ******************************************************************************
 *
 * @brief   This is the `mark needs verify' for data lost during copy event
 *          completion function.  Check the event status.  If any error occurs
 *          we do NOT retry.  Since we have already written a specific invalidate
 *          pattern to the lost data, any reads to the data lost area will detect
 *          that a verify is required.
 *
 * @param   event_p - Pointer to event that was completed
 * @param   context - Pointer to the virtual drive object.
 *
 * @return  fbe_status_t.
 *
 * @author
 *  12/12/2012  Ron Proulx  - Updated to use native allocation
 *
 ******************************************************************************/
static fbe_status_t fbe_virtual_drive_copy_mark_needs_verify_event_completion(fbe_event_t *event_p,
                                                                        fbe_event_completion_context_t context)
{
    fbe_event_stack_t          *event_stack_p = NULL;
    fbe_lba_t                   event_lba;
    fbe_block_count_t           event_blocks;
    fbe_event_status_t          event_status;
    fbe_u32_t                   event_flags;
    fbe_virtual_drive_t        *virtual_drive_p = (fbe_virtual_drive_t *)context;

    /* Get the event stack and status.
     */
    event_stack_p = fbe_event_get_current_stack(event_p);
    fbe_event_stack_get_extent(event_stack_p, &event_lba, &event_blocks);
    fbe_event_get_status(event_p, &event_status);
    fbe_event_get_flags(event_p, &event_flags);

    /* free all associated resources before releasing event
     */
    fbe_event_release_stack(event_p, event_stack_p);
    fbe_event_destroy(event_p);
    fbe_memory_ex_release(event_p); 
    
    /* If enabled traces some information.
     */
    fbe_virtual_drive_trace(virtual_drive_p,
                            FBE_TRACE_LEVEL_INFO,
                            FBE_VIRTUAL_DRIVE_DEBUG_FLAG_REBUILD_TRACING,
                            "virtual_drive: copy mark verify complete lba: 0x%llx blks: 0x%llx event status: %d flags: 0x%x \n",
                            (unsigned long long)event_lba, (unsigned long long)event_blocks, event_status, event_flags);

    /* Check event status and flags
     */
    if (event_status == FBE_EVENT_STATUS_NO_USER_DATA)
    {
        /* Never expect to get this ideally since we do not issue the rebuild operation if it is 
         * not consumed (except metadata rebuild and it handles differently) by upstream objects,
         * VD object will ignore the media error in this case and proceed with next chunk for 
         * the copy operation.
         */
        fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s: unexpected event status, status:%d", __FUNCTION__, event_status);
    }
    else if(event_status != FBE_EVENT_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s: event status is not good, event_status:%d\n", __FUNCTION__, event_status);
    }

    if(event_flags & FBE_EVENT_FLAG_DENY)
    {
        fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s: deny event flag is set, event_flags:%d\n", __FUNCTION__, event_flags);
    }

    /* Clear the `mark verify in progress' flag.
     */
    fbe_virtual_drive_clear_flag(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_COPY_MARK_VERIFY_IN_PROGRESS);

    /* Awlays return success.
     */
    return FBE_STATUS_OK;
}
/*****************************************************************
 * end fbe_virtual_drive_copy_mark_needs_verify_event_completion()
 *****************************************************************/

/*!****************************************************************************
 *          fbe_virtual_drive_copy_send_event_to_mark_needs_verify()
 ******************************************************************************
 * @brief
 *  This function uses allocated event to send it to upstream object to mark
 *  the chunks for the rebuild.
 *
 * @param   virtual_drive_p - Pointer to virtual drive object
 * @param   packet_p - Pointer to monitor packet to retrieve the information to
 *              populate the event.
 *
 * @note    Since we have released the block operation we use the rebuild context
 *          to get the I/O information.
 *
 * @return fbe_status_t.
 *
 * @author
 *  12/12/2012  Ron Proulx  - Updated to use native allocation (i.e. make the
 *                            event completion asynchronous with the monitor
 *                            rebuild request).
 *
 ******************************************************************************/
static fbe_status_t fbe_virtual_drive_copy_send_event_to_mark_needs_verify(fbe_virtual_drive_t *virtual_drive_p,
                                                                           fbe_packet_t *packet_p)
{
    fbe_event_t                        *event_p = NULL;
    fbe_event_stack_t                  *event_stack_p;
    fbe_payload_ex_t                   *sep_payload_p;
    fbe_payload_control_operation_t    *control_operation_p;
    fbe_raid_group_rebuild_context_t   *rebuild_context_p;
    fbe_lba_t                           start_lba;
    fbe_block_count_t                   block_count;
    fbe_event_data_request_t            data_request;
    fbe_status_t                        status;

    /* Get the sep payload and control operation.
     */
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(sep_payload_p);
    status = fbe_transport_get_status_code(packet_p);

    /* Get the I/O information from the rebuild context.
     */
    fbe_payload_control_get_buffer(control_operation_p, &rebuild_context_p);
    start_lba = rebuild_context_p->start_lba;
    block_count = rebuild_context_p->block_count;

    /* Use native allocation for the event.
     */
    event_p = fbe_memory_ex_allocate(sizeof(fbe_event_t));

    /* Check allocation status */
    if (event_p == NULL)
    {
        /* Currently this callback doesn't expect any aborted requests */
        fbe_base_object_trace((fbe_base_object_t*) virtual_drive_p, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                              "%s: memory allocation failed\n", __FUNCTION__);        
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Set the `mark needs verify' in progress flag.
     */
    fbe_virtual_drive_set_flag(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_COPY_MARK_VERIFY_IN_PROGRESS);

    /* initialize the event before sending to upstream object. */
    fbe_event_init(event_p);

    /* populate the data request structure and set the event data */
    data_request.data_event_type = FBE_DATA_EVENT_TYPE_MARK_VERIFY;
    fbe_event_set_data_request_data(event_p, &data_request);

    /* allocate the event stack to specify the range to mark neeeds rebuild. */
    event_stack_p = fbe_event_allocate_stack(event_p);
    fbe_event_stack_set_extent(event_stack_p, start_lba, block_count);

    /* set the event completion function before sending event. */
    fbe_event_stack_set_completion_function(event_stack_p,
                                            fbe_virtual_drive_copy_mark_needs_verify_event_completion,
                                            virtual_drive_p);

    /* send data event to upstream objects. */
    status = fbe_base_config_send_event((fbe_base_config_t *)virtual_drive_p, FBE_EVENT_TYPE_DATA_REQUEST, event_p);

    return status;
}
/******************************************************************************
 * end fbe_virtual_drive_copy_send_event_to_mark_needs_verify()
*******************************************************************************/

/*!****************************************************************************
 * fbe_virtual_drive_logging_log_all_copies_complete
 ******************************************************************************
 * @brief
 *   This function will log a "copy complete" message when all LUNs on the 
 *   drive have finished rebuilding. 
 *
 * @param virtual_drive_p - Pointer to virtual drive object
 * @param in_packet_p               - pointer to a packet 
 * @param in_position               - disk position in the RG (of the destination
 *                                    drive/proactive spare)
 *
 * @return fbe_status_t           
 *
 * @author
 *   09/20/2010 - Created. Jean Montes. 
 *
 ******************************************************************************/
fbe_status_t fbe_virtual_drive_logging_log_all_copies_complete(fbe_virtual_drive_t *virtual_drive_p,
                                    fbe_packet_t*               in_packet_p, 
                                    fbe_u32_t                   in_position)
{
    fbe_u32_t                       source_position;            // position in the mirror of the source drive             
    fbe_u32_t                       dest_drive_bus;             // destination/proactive spare bus number
    fbe_u32_t                       dest_drive_enclosure;       // destination/proactive spare enclosure
    fbe_u32_t                       dest_drive_slot;            // destination/proactive spare slot 
    fbe_u32_t                       source_drive_bus;           // source/proactive candidate bus number
    fbe_u32_t                       source_drive_enclosure;     // source/proactive candidate enclosure
    fbe_u32_t                       source_drive_slot;          // source/proactive candidate slot 


    //  The position that is passed in is for the destination/proactive spare drive.  Determine the position 
    //  of the source/proactive candidate drive.  Since it is a mirror, one must be position 0 and the other
    //  position 1. 
    if (in_position == 0)
    {
        source_position = 1;
    }
    else
    {
        source_position = 0;
    }

    //  Get the b-e-d info of the PVD that is being copied from 
    fbe_raid_group_logging_get_disk_info_for_position((fbe_raid_group_t *)virtual_drive_p, in_packet_p, source_position,  
                                                      &source_drive_bus, 
                                                      &source_drive_enclosure, 
                                                      &source_drive_slot);

    //  Get the b-e-d info of the PVD that is being copied to 
    fbe_raid_group_logging_get_disk_info_for_position((fbe_raid_group_t *)virtual_drive_p, in_packet_p, in_position, 
                                                      &dest_drive_bus, 
                                                      &dest_drive_enclosure, 
                                                      &dest_drive_slot);

    //  Log the message to the event log 
    fbe_event_log_write(SEP_INFO_RAID_OBJECT_RAID_GROUP_COPY_COMPLETED, NULL, 0, 
        "%d %d %d %d %d %d", source_drive_bus, source_drive_enclosure, source_drive_slot, 
        dest_drive_bus, dest_drive_enclosure, dest_drive_slot);

    //  Return success  
    return FBE_STATUS_OK;

}
/*********************************************************
 * end fbe_virtual_drive_logging_log_all_copies_complete()
 *********************************************************/

/*!****************************************************************************
 * fbe_virtual_drive_copy_handle_processing_on_edge_enable()
 ******************************************************************************
 * @brief
 *   This function will set condition(s) to perform NR and rebuild related processing 
 *   when an edge becomes enabled.  It runs in the context of the edge state change 
 *   event and not the monitor context. 
 * 
 * @param   virtual_drive_p - Pointer to virtual drive object
 *
 * @return  fbe_status_t        
 *
 * @author
 *   12/07/2009 - Created. Jean Montes.
 *
 ******************************************************************************/
static fbe_status_t fbe_virtual_drive_copy_handle_processing_on_edge_enable(fbe_virtual_drive_t *virtual_drive_p)
{
    fbe_raid_group_t   *raid_group_p = (fbe_raid_group_t *)virtual_drive_p;
    fbe_virtual_drive_configuration_mode_t configuration_mode;
    fbe_bool_t          b_is_mirror_mode = FBE_TRUE;

    /* Get the configuration mode
     */
    fbe_virtual_drive_get_configuration_mode(virtual_drive_p, &configuration_mode);
    b_is_mirror_mode = fbe_virtual_drive_is_mirror_configuration_mode_set(virtual_drive_p);

    /* This is only required/desired for mirror mode.
     */
    if (b_is_mirror_mode == FBE_FALSE)
    {
        return FBE_STATUS_OK;
    }

    /* It doesn't hurt to force raid go through this condition when we see an edge become available */
    fbe_raid_group_lock(raid_group_p);
    fbe_raid_group_set_local_state(raid_group_p, FBE_RAID_GROUP_LOCAL_STATE_EVAL_MARK_NR_REQUEST);
    fbe_raid_group_unlock(raid_group_p);
    fbe_lifecycle_set_cond(&fbe_virtual_drive_lifecycle_const, 
                           (fbe_base_object_t *)virtual_drive_p,
                           FBE_RAID_GROUP_LIFECYCLE_COND_EVAL_MARK_NR);

    /* Trace some information if enabled.
     */
    fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "virtual_drive: handle edge enable mode: %d cond_id: 0x%x \n",
                          configuration_mode, FBE_RAID_GROUP_LIFECYCLE_COND_EVAL_MARK_NR);

    //  Return success  
    return FBE_STATUS_OK;

}
/*********************************************************************
 * end fbe_virtual_drive_copy_handle_processing_on_edge_enable()
 *********************************************************************/

/*!****************************************************************************
 * fbe_virtual_drive_copy_handle_processing_on_edge_unavailable()
 ******************************************************************************
 * @brief
 *   This function will take RL/rebuild-related actions when an edge becomes unavailable.
 *   It runs in the context of the edge state change event and not the monitor context.
 * 
 * @param virtual_drive_p       - pointer to virtual drive object
 * @param in_position           - disk position in the RG 
 * @param in_downstream_health  - overall health state of the downstream paths
 *
 * @return  fbe_status_t        
 *
 * @author
 *   09/16/2010 - Created. Jean Montes.
 *
 ******************************************************************************/
static fbe_status_t fbe_virtual_drive_copy_handle_processing_on_edge_unavailable(fbe_virtual_drive_t *virtual_drive_p,
                                fbe_u32_t                                   in_position, 
                                fbe_base_config_downstream_health_state_t   in_downstream_health)
{
    fbe_raid_group_t   *raid_group_p = (fbe_raid_group_t *)virtual_drive_p;
    fbe_bool_t          b_is_rb_logging = FBE_TRUE;
    fbe_virtual_drive_configuration_mode_t configuration_mode;
    fbe_bool_t          b_is_mirror_mode = FBE_TRUE;

    //  Check if the downstream health is degraded.  If not, we don't need to do anything more - just return.
    //if (in_downstream_health != FBE_DOWNSTREAM_HEALTH_DEGRADED)
    //{
    //    return FBE_STATUS_OK;
    //}

    /* Get the configuration mode
     */
    fbe_virtual_drive_get_configuration_mode(virtual_drive_p, &configuration_mode);
    b_is_mirror_mode = fbe_virtual_drive_is_mirror_configuration_mode_set(virtual_drive_p);

    //  The health is degraded.  Get the rebuild logging setting for this disk.
    fbe_raid_group_get_rb_logging(raid_group_p, in_position, &b_is_rb_logging);

    //  If we are rebuild logging i.e. we already know that it is gone, then we don't need to do 
    //  anything more - just return. 
    if ((b_is_mirror_mode == FBE_FALSE) ||
        (b_is_rb_logging == FBE_TRUE)      )
    {
        return FBE_STATUS_OK;
    }

    //  The edge went away for the first time.  We need to abort any outstanding stripe locks that the monitor is 
    //  waiting for.  Otherwise the monitor might never get a chance to run.
    fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p, 
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "virtual_drive: handle edge unavailable mode: %d index: %d health: %d abort SL, paged, monitor\n", 
                          configuration_mode, in_position, in_downstream_health);

    fbe_metadata_stripe_lock_element_abort(&virtual_drive_p->spare.raid_group.base_config.metadata_element);

    /* Abort paged since a monitor operation might be waiting for paged. 
     * The monitor will not run to start eval rb logging or quiesce until the monitor op completes.
     */
    fbe_metadata_element_abort_paged(&virtual_drive_p->spare.raid_group.base_config.metadata_element);

    /* Monitor operations might be waiting for memory.  Make sure these get aborted.
     */
    fbe_base_config_abort_monitor_ops((fbe_base_config_t*)raid_group_p);

    //  Return success  
    return FBE_STATUS_OK;

}
/*********************************************************************
 * end fbe_virtual_drive_copy_handle_processing_on_edge_unavailable()
 *********************************************************************/

/*!****************************************************************************
 * fbe_virtual_drive_copy_handle_processing_on_edge_state_change()
 ******************************************************************************
 * @brief
 *   This function will perform any action that needs to be done when an edge 
 *   changes state.  It runs in the context of the edge state change event and 
 *   not the monitor context. 
 * 
 * @param virtual_drive_p       - pointer to virtual drive object
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
fbe_status_t fbe_virtual_drive_copy_handle_processing_on_edge_state_change(fbe_virtual_drive_t *virtual_drive_p,
                                    fbe_u32_t                                   in_position,
                                    fbe_path_state_t                            in_path_state,
                                    fbe_base_config_downstream_health_state_t   in_downstream_health)

{
    fbe_block_edge_t   *edge_p = NULL;
    fbe_path_attr_t     path_attrib = 0;
    fbe_virtual_drive_configuration_mode_t configuration_mode;
    fbe_bool_t          b_is_mirror_mode = FBE_TRUE;

    /* Get the configuration mode
     */
    fbe_virtual_drive_get_configuration_mode(virtual_drive_p, &configuration_mode);
    b_is_mirror_mode = fbe_virtual_drive_is_mirror_configuration_mode_set(virtual_drive_p);

    /* This is only required/desired for mirror mode.
     */
    if (b_is_mirror_mode == FBE_FALSE)
    {
        return FBE_STATUS_OK;
    }

    
    fbe_base_config_get_block_edge(&virtual_drive_p->spare.raid_group.base_config, &edge_p, in_position);
    fbe_block_transport_get_path_attributes(edge_p, &path_attrib);

    //  If the new path state is anything other than slumber, then set a condition so the object will wake up from
    //  hibernation.   This allows it to process conditions that only run in the ready or activate states.
    if (in_path_state != FBE_PATH_STATE_SLUMBER)
    {
        fbe_lifecycle_set_cond(&fbe_base_config_lifecycle_const, (fbe_base_object_t*)virtual_drive_p,
            FBE_BASE_CONFIG_LIFECYCLE_COND_EDGE_CHANGE_DURING_HIBERNATION);
    }

    /* If this path attribute is set, it overrides the path state.
     */
    if (path_attrib & FBE_BLOCK_PATH_ATTR_FLAGS_TIMEOUT_ERRORS)
    {
        in_path_state = FBE_PATH_STATE_BROKEN;
    }

    //  Take specific action depending on the path state 
    switch (in_path_state)
    {
        //  If the edge became enabled, call function to handle that 
        case FBE_PATH_STATE_ENABLED:
        {
            fbe_virtual_drive_copy_handle_processing_on_edge_enable(virtual_drive_p);
            break; 
        }

        //  If the edge went into slumber, we don't need to do anything 
        case FBE_PATH_STATE_SLUMBER:
        {
            break; 
        }

        //  For all other edge states (disabled, broken) quiesce non-user I/O
        default: 
        {
            fbe_virtual_drive_copy_handle_processing_on_edge_unavailable(virtual_drive_p, 
                                                                         in_position, 
                                                                         in_downstream_health);
            break; 
        }
    }

    //  Return success  
    return FBE_STATUS_OK;

}
/*********************************************************************
 * end fbe_virtual_drive_copy_handle_processing_on_edge_state_change()
 *********************************************************************/

/*!***************************************************************************
 *          fbe_virtual_drive_copy_update_pvd_percent_rebuilt()
 *****************************************************************************
 *
 * @brief   If the percent rebuilt of a position has changed send a usurper
 *          to the provision drive being rebuilt to update the percent rebuilt
 *          for that position.  This is needed so that a `mega' poll will get
 *          the updated percentage rebuilt.
 *
 * @param   virtual_drive_p - Pointer to virtual drive object
 * @param   packet_p - Pointer to monitor packet to use for request
 * @param   downstream_edge_p - Pointer to downstream edge to send packet to
 * @param   pvd_object_id - Provision drive object id of pvd to update
 * @param   percent_rebuilt - The percent rebuilt for this position
 * 
 * @return  fbe_status_t 
 *
 * @author
 *  03/20/2013  Ron Proulx  - Created.
 *
 *****************************************************************************/
static fbe_status_t fbe_virtual_drive_copy_update_pvd_percent_rebuilt(fbe_virtual_drive_t *virtual_drive_p,
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

    /* Validate that the downstream edge is enabled.  The copy condition should
     * not run unless the destination is healthy but it can go away at any time.
     */
    if (downstream_edge_p->base_edge.path_state != FBE_PATH_STATE_ENABLED)
    {
        /* This is a warning since it can occur.
         */
        fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p, 
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
        fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p, 
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

    fbe_payload_ex_increment_control_operation_level(payload);
    fbe_transport_set_sync_completion_type(packet_p, FBE_TRANSPORT_COMPLETION_TYPE_MORE_PROCESSING_REQUIRED);

    fbe_block_transport_send_control_packet(downstream_edge_p, packet_p);  
    
    fbe_transport_wait_completion(packet_p);

    status = fbe_transport_get_status_code(packet_p);
    fbe_payload_control_get_status(control_operation, &control_status);
    fbe_payload_ex_release_control_operation(payload, control_operation);

    /*any chance this drive just can't satisy the time we give it to spin up, or simply does not suppot power save*/
    if ((control_status != FBE_PAYLOAD_CONTROL_STATUS_OK) ||
        (status != FBE_STATUS_OK)                            ) 
    {
         fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p, 
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
 * end fbe_virtual_drive_copy_update_pvd_percent_rebuilt()
 ******************************************************************************/

/*!***************************************************************************
 *          fbe_virtual_drive_copy_send_start_end_notification()
 *****************************************************************************
 *
 * @brief   Send the copy start/end notifications for both the source and
 *          destination.  This is done on both the active and passive SPs.
 *
 * @param   virtual_drive_p - Pointer to virtual drive object
 * @param   packet_p - Pointer to monitor packet
 * @param   downstream_edge_p - Pointer to downstream edge to send packet to
 * @param   source_edge_index - source edge index
 * @param   destination_edge_index - destination edge index
 * @param   opcode - Determines if this is a start or end
 *
 * @return fbe_status_t
 *
 * @note    Since this is only a start/end notification the `percent rebuilt'
 *          is always reset to 0.
 *
 * @author
 *  07/31/2012  Ron Proulx  - Updated.
 *****************************************************************************/
static fbe_status_t fbe_virtual_drive_copy_send_start_end_notification(fbe_virtual_drive_t *virtual_drive_p,
                                                                       fbe_packet_t *packet_p,
                                                                       fbe_block_edge_t *downstream_edge_p,
                                                                       fbe_edge_index_t source_edge_index,
                                                                       fbe_edge_index_t destination_edge_index,
                                                                       fbe_raid_group_data_reconstrcution_notification_op_t opcode)
{
    fbe_status_t            status = FBE_STATUS_OK;
    fbe_raid_group_t       *raid_group_p = (fbe_raid_group_t *)virtual_drive_p;
    fbe_u32_t               width;
    fbe_notification_info_t notify_info;
    fbe_trace_level_t       error_trace_level = FBE_TRACE_LEVEL_ERROR;

    /* Validate the input index.
     */
    fbe_base_config_get_width((fbe_base_config_t *)raid_group_p, &width);
    if ((source_edge_index >= width)                  ||
        (destination_edge_index >= width)             ||
        (source_edge_index == destination_edge_index)    )
    {
        fbe_base_object_trace((fbe_base_object_t*)virtual_drive_p, 
                              FBE_TRACE_LEVEL_ERROR, 
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                              "%s unexpected source: %d or dest index: %d width: %d\n", 
                              __FUNCTION__, source_edge_index, destination_edge_index, width);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    /* Validate the opcode (only start and end are supported)
     */
    if ((opcode != DATA_RECONSTRUCTION_START) &&
        (opcode != DATA_RECONSTRUCTION_END)      )
    {
        fbe_base_object_trace((fbe_base_object_t*)virtual_drive_p, 
                              FBE_TRACE_LEVEL_ERROR, 
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                              "%s unexpected opcode: %d \n", 
                              __FUNCTION__, opcode);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Populate the notification information.
     */
    fbe_zero_memory(&notify_info, sizeof(fbe_notification_info_t));
    notify_info.notification_type = FBE_NOTIFICATION_TYPE_DATA_RECONSTRUCTION;
    notify_info.class_id = FBE_CLASS_ID_PROVISION_DRIVE;
    notify_info.object_type = FBE_TOPOLOGY_OBJECT_TYPE_PROVISIONED_DRIVE;
    notify_info.notification_data.data_reconstruction.state = opcode;
    notify_info.notification_data.data_reconstruction.percent_complete = raid_group_p->previous_rebuild_percent[destination_edge_index];

    /* Always reset the blocks rebuilt to 0 for both positions.
     */
    raid_group_p->raid_group_metadata_memory.blocks_rebuilt[source_edge_index] = 0;
    raid_group_p->raid_group_metadata_memory.blocks_rebuilt[destination_edge_index] = 0;

    /* First check the source rebuild pvd.  If it is valid generate either
     * the start or end reconstruction notification.
     */
    if (raid_group_p->rebuilt_pvds[source_edge_index] != FBE_OBJECT_ID_INVALID)
    {
        /* If the source pvd rebuild object id is valid send the start/end
         * notification.
         */
        fbe_base_object_trace((fbe_base_object_t*)virtual_drive_p, 
                              FBE_TRACE_LEVEL_INFO, 
                              FBE_TRACE_MESSAGE_ID_INFO, 
                              "COPY: source      index: %d pvd obj: 0x%x %s\n", 
                              source_edge_index, raid_group_p->rebuilt_pvds[source_edge_index], 
                              (opcode == DATA_RECONSTRUCTION_START) ? "DATA_RECONSTRUCTION_START" : "DATA_RECONSTRUCTION_END");

        status = fbe_notification_send(raid_group_p->rebuilt_pvds[source_edge_index], notify_info);
        if (status != FBE_STATUS_OK) 
        { 
            fbe_base_object_trace((fbe_base_object_t*)virtual_drive_p, 
                                  FBE_TRACE_LEVEL_WARNING, 
                                  FBE_TRACE_MESSAGE_ID_INFO, 
                                  "%s unexpected status: %d on notification send \n", 
                                  __FUNCTION__, status);
        }

        /* If this is an end, clear the source pvd object id.
         */
        if (opcode == DATA_RECONSTRUCTION_END) 
        {
            raid_group_p->rebuilt_pvds[source_edge_index] = FBE_OBJECT_ID_INVALID;/*clear for next time*/
        }
    }

    /* Now update the `percent rebuilt' in the associated pvd.  The use cases
     * are:
     *  o This is a start notification so set the percent rebuilt to 1.  
     *    This needs to set to 1 since percent rebuild of 0 means "not rebuilding". 
     *    This will handle the case where we have a glitch in the system or SLF that
     *    causes Admin to wipe out the previous information after a Mega Poll.  
     *
     *  o This is a end notification so the rebuild is complete, set the percent
     *    rebuilt to 0 (by default, not rebuilding).
     */
    if (raid_group_p->rebuilt_pvds[destination_edge_index] != FBE_OBJECT_ID_INVALID)
    {
        status = fbe_virtual_drive_copy_update_pvd_percent_rebuilt(virtual_drive_p,
                                                                   packet_p,
                                                                   downstream_edge_p,
                                                                   raid_group_p->rebuilt_pvds[destination_edge_index],
                                                                   ((opcode == DATA_RECONSTRUCTION_START) ? 1 : 0));
        if (status != FBE_STATUS_OK)
        {
            /*! @note If we cannot update the downstream pvd rebuilt percent
             *        it most likely means that the drive was removed. In any 
             *        case reset this rebuild index to `not rebuilding'.
             */
            error_trace_level = (status == FBE_STATUS_EDGE_NOT_ENABLED) ? FBE_TRACE_LEVEL_WARNING : FBE_TRACE_LEVEL_ERROR;
            fbe_base_object_trace((fbe_base_object_t*)virtual_drive_p, 
                                  error_trace_level, 
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                                  "%s update pvd rebuilt percent for destination: 0x%x failed - status: 0x%x\n", 
                                  __FUNCTION__, raid_group_p->rebuilt_pvds[destination_edge_index], status);

            /* If we are generating the END just let it cleanup.
             */
            if (opcode != DATA_RECONSTRUCTION_END)
            {
                /* Cleanup everything.
                 */
                raid_group_p->rebuilt_pvds[source_edge_index] = FBE_OBJECT_ID_INVALID;
                raid_group_p->rebuilt_pvds[destination_edge_index] = FBE_OBJECT_ID_INVALID;
                raid_group_p->previous_rebuild_percent[source_edge_index] = 0;
                raid_group_p->previous_rebuild_percent[destination_edge_index] = 0;

                return status;
            }
        }

        /* If the destination pvd rebuild object id is valid send the start/end
         * notification.
         */
        fbe_base_object_trace((fbe_base_object_t*)virtual_drive_p, 
                              FBE_TRACE_LEVEL_INFO, 
                              FBE_TRACE_MESSAGE_ID_INFO, 
                              "COPY: destination index: %d pvd obj: 0x%x %s\n", 
                              destination_edge_index, raid_group_p->rebuilt_pvds[destination_edge_index], 
                              (opcode == DATA_RECONSTRUCTION_START) ? "DATA_RECONSTRUCTION_START" : "DATA_RECONSTRUCTION_END");

        status = fbe_notification_send(raid_group_p->rebuilt_pvds[destination_edge_index], notify_info);
        if (status != FBE_STATUS_OK) 
        { 
            fbe_base_object_trace((fbe_base_object_t*)virtual_drive_p, 
                                  FBE_TRACE_LEVEL_WARNING, 
                                  FBE_TRACE_MESSAGE_ID_INFO, 
                                  "%s unexpected status: %d on notification send \n", 
                                  __FUNCTION__, status);
        }

        /* If this is an end, clear the source pvd object id.
         */
        if (opcode == DATA_RECONSTRUCTION_END) 
        {
            raid_group_p->rebuilt_pvds[destination_edge_index] = FBE_OBJECT_ID_INVALID;/*clear for next time*/
        }
    }

    /* Always reset the previous rebuild percent.
     */
    raid_group_p->previous_rebuild_percent[source_edge_index] = 0;
    raid_group_p->previous_rebuild_percent[destination_edge_index] = 0;

    return FBE_STATUS_OK;
}
/**********************************************************
 * end fbe_virtual_drive_copy_send_start_end_notification()
 **********************************************************/

/*!****************************************************************************
 *          fbe_virtual_drive_copy_send_progress_notification
 ******************************************************************************
 * @brief
 *  Send notification to upper layers about the fact we start/stop data reconstruction
 *  This happen on the passive side. The active side will do it in a different context
 *
 * @param   virtual_drive_p - Pointer to virtual drive to update progress for
 * @param   packet_p - Pointer to monitor packet to use
 * @param   downstream_edge_p - Pointer to downstream edge to send packet to
 * @param   source_edge_index - source edge index
 * @param   destination_edge_index - destination edge index
 * @param   percent_complete  - The current percentage complete
 * 
 * @return status
 *
 * @author
 *  07/31/2012  Ron Proulx  - Updated.
 ******************************************************************************/
static fbe_status_t fbe_virtual_drive_copy_send_progress_notification(fbe_virtual_drive_t *virtual_drive_p,
                                                                      fbe_packet_t *packet_p,
                                                                      fbe_block_edge_t *downstream_edge_p,
                                                                      fbe_edge_index_t source_edge_index,
                                                                      fbe_edge_index_t destination_edge_index, 
                                                                      fbe_u32_t percent_complete)
{
    fbe_status_t            status = FBE_STATUS_OK;
    fbe_raid_group_t       *raid_group_p = (fbe_raid_group_t *)virtual_drive_p;
    fbe_u32_t               width;
    fbe_object_id_t         source_pvd_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t         destination_pvd_id = FBE_OBJECT_ID_INVALID;
    fbe_notification_info_t notify_info;
    fbe_trace_level_t       error_trace_level = FBE_TRACE_LEVEL_ERROR;

    /* Validate the input index.
     */
    fbe_base_config_get_width((fbe_base_config_t *)raid_group_p, &width);
    if ((source_edge_index >= width)                  ||
        (destination_edge_index >= width)             ||
        (source_edge_index == destination_edge_index)    )
    {
        fbe_base_object_trace((fbe_base_object_t*)virtual_drive_p, 
                              FBE_TRACE_LEVEL_ERROR, 
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                              "%s unexpected source: %d or dest index: %d width: %d\n", 
                              __FUNCTION__, source_edge_index, destination_edge_index, width);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    /* Validate both the source and destination rebuild pvd ids.
     */
    if ((raid_group_p->rebuilt_pvds[source_edge_index] == FBE_OBJECT_ID_INVALID)      ||
        (raid_group_p->rebuilt_pvds[destination_edge_index] == FBE_OBJECT_ID_INVALID)    )
    {
        fbe_base_object_trace((fbe_base_object_t*)virtual_drive_p, 
                              FBE_TRACE_LEVEL_ERROR, 
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                              "%s source rebuild pvd obj: 0x%x or destination: 0x%x is INVALID\n", 
                              __FUNCTION__, raid_group_p->rebuilt_pvds[source_edge_index], 
                              raid_group_p->rebuilt_pvds[destination_edge_index]);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    source_pvd_id = raid_group_p->rebuilt_pvds[source_edge_index];
    destination_pvd_id = raid_group_p->rebuilt_pvds[destination_edge_index];

    /* First update the `percent rebuilt' in the associated pvd.
     */
    status = fbe_virtual_drive_copy_update_pvd_percent_rebuilt(virtual_drive_p,
                                                               packet_p,
                                                               downstream_edge_p,
                                                               destination_pvd_id,
                                                               percent_complete);
    if (status != FBE_STATUS_OK)
    {
        /*! @note If we cannot update the downstream pvd rebuilt percent
         *        it most likely means that the drive was removed. In any 
         *        case reset this rebuild index to `not rebuilding'.
         */
        error_trace_level = (status == FBE_STATUS_EDGE_NOT_ENABLED) ? FBE_TRACE_LEVEL_WARNING : FBE_TRACE_LEVEL_ERROR;
        fbe_base_object_trace((fbe_base_object_t*)virtual_drive_p, 
                              error_trace_level, 
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                              "%s update pvd rebuilt percent for destination: 0x%x failed - status: 0x%x\n", 
                              __FUNCTION__, destination_pvd_id, status);

        /* Reset everthing.
         */
        raid_group_p->rebuilt_pvds[source_edge_index] = FBE_OBJECT_ID_INVALID;
        raid_group_p->rebuilt_pvds[destination_edge_index] = FBE_OBJECT_ID_INVALID;
        raid_group_p->previous_rebuild_percent[source_edge_index] = 0;
        raid_group_p->previous_rebuild_percent[destination_edge_index] = 0;
        raid_group_p->raid_group_metadata_memory.blocks_rebuilt[source_edge_index] = 0;
        raid_group_p->raid_group_metadata_memory.blocks_rebuilt[destination_edge_index] = 0;

        return status;
    }

    /* Populate the notification information.
     */
    fbe_zero_memory(&notify_info, sizeof(fbe_notification_info_t));
    notify_info.notification_type = FBE_NOTIFICATION_TYPE_DATA_RECONSTRUCTION;
    notify_info.class_id = FBE_CLASS_ID_PROVISION_DRIVE;
    notify_info.object_type = FBE_TOPOLOGY_OBJECT_TYPE_PROVISIONED_DRIVE;
    notify_info.notification_data.data_reconstruction.percent_complete = percent_complete;
    notify_info.notification_data.data_reconstruction.state = DATA_RECONSTRUCTION_IN_PROGRESS;

    /* Send the notification for the source.
     */
    fbe_base_object_trace((fbe_base_object_t*)virtual_drive_p, 
                          FBE_TRACE_LEVEL_INFO, 
                          FBE_TRACE_MESSAGE_ID_INFO, 
                          "COPY: source      index: %d pvd obj: 0x%x progress: %d%%\n",
                          source_edge_index, source_pvd_id, percent_complete);
    status = fbe_notification_send(source_pvd_id, notify_info);
    if (status != FBE_STATUS_OK) 
    { 
        fbe_base_object_trace((fbe_base_object_t*)virtual_drive_p, 
                              FBE_TRACE_LEVEL_WARNING, 
                              FBE_TRACE_MESSAGE_ID_INFO, 
                              "%s failed for source index: %d pvd obj: 0x%x status: 0x%x\n", 
                              __FUNCTION__, source_edge_index, source_pvd_id, status);
    }

    /* Send the notification for the destination.
     */
    fbe_base_object_trace((fbe_base_object_t*)virtual_drive_p, 
                          FBE_TRACE_LEVEL_INFO, 
                          FBE_TRACE_MESSAGE_ID_INFO, 
                          "COPY: destination index: %d pvd obj: 0x%x progress: %d%%\n",
                          destination_edge_index, destination_pvd_id, percent_complete);
    status = fbe_notification_send(destination_pvd_id, notify_info);
    if (status != FBE_STATUS_OK) 
    { 
        fbe_base_object_trace((fbe_base_object_t*)virtual_drive_p, 
                              FBE_TRACE_LEVEL_WARNING, 
                              FBE_TRACE_MESSAGE_ID_INFO, 
                              "%s failed for destination index: %d pvd obj: 0x%x status: 0x%x\n", 
                              __FUNCTION__, destination_edge_index, destination_pvd_id, status);                
    }

    /* Update the previous rebuild percentage complete to the current.
     */
    raid_group_p->previous_rebuild_percent[source_edge_index] = 0;
    raid_group_p->previous_rebuild_percent[destination_edge_index] = percent_complete;

    /* Return success.
     */
    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_virtual_drive_copy_send_progress_notification()
 ******************************************************************************/

/*!****************************************************************************
 *          fbe_virtual_drive_copy_generate_notifications()
 ******************************************************************************
 * @brief
 *  Send notification to upper layers about the fact we start/stop data reconstruction
 *  This happen on the passive side. The active side will do it in a different context
 *
 * @param   virtual_drive_p - Pointer to virtual drive object
 * @param   packet_p - Pointer to monitor packet
 * 
 * @return status
 *
 * @author
 *  03/20/2013  Ron Proulx  - Updated
 *
 ******************************************************************************/
fbe_status_t fbe_virtual_drive_copy_generate_notifications(fbe_virtual_drive_t *virtual_drive_p,
                                                           fbe_packet_t *packet_p)
{
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_virtual_drive_configuration_mode_t configuration_mode;
    fbe_raid_group_t                   *raid_group_p = (fbe_raid_group_t *)virtual_drive_p;
    fbe_object_id_t                     source_pvd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_edge_index_t                    source_edge_index = FBE_EDGE_INDEX_INVALID;
    fbe_object_id_t                     destination_pvd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_edge_index_t                    destination_edge_index = FBE_EDGE_INDEX_INVALID;
    fbe_block_edge_t                   *downstream_edge_p = NULL;
    fbe_u32_t                           percent_complete;
    fbe_raid_position_bitmask_t         rl_bitmask;
    fbe_bool_t                          b_is_copy_complete = FBE_FALSE;
    fbe_lba_t                           cur_checkpoint = 0;
    fbe_lba_t                           exported_disk_capacity = 0;
    fbe_spare_swap_command_t            copy_request_type;
    fbe_bool_t                          b_is_copy_request_aborted = FBE_FALSE;     

    /* Get the virtual drive config mode.
     */
    fbe_virtual_drive_get_configuration_mode(virtual_drive_p, &configuration_mode);
    b_is_copy_complete = fbe_virtual_drive_is_copy_complete(virtual_drive_p);
    fbe_virtual_drive_get_copy_request_type(virtual_drive_p, &copy_request_type);
    b_is_copy_request_aborted = (copy_request_type == FBE_SPARE_ABORT_COPY_COMMAND) ? FBE_TRUE : FBE_FALSE;

    /* If the virtual drive is in pass-thru mode and both raid group rebuild
     * percentages are 0, then invalidate the pvds (if required).
     */
    if (((configuration_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_FIRST_EDGE)  ||
         (configuration_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_SECOND_EDGE)    ) &&
        ((raid_group_p->previous_rebuild_percent[FIRST_EDGE_INDEX] == 0)             &&
         (raid_group_p->previous_rebuild_percent[SECOND_EDGE_INDEX] == 0)               )    )
    {
        /* If the first or second are not invalid set them to invalid
         */
        if ((raid_group_p->rebuilt_pvds[FIRST_EDGE_INDEX]  != FBE_OBJECT_ID_INVALID) ||
            (raid_group_p->rebuilt_pvds[SECOND_EDGE_INDEX] != FBE_OBJECT_ID_INVALID)    )
        {
            /* Trace and debug message and return.
             */
            raid_group_p->rebuilt_pvds[FIRST_EDGE_INDEX] = FBE_OBJECT_ID_INVALID;
            raid_group_p->rebuilt_pvds[SECOND_EDGE_INDEX] = FBE_OBJECT_ID_INVALID;
            fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "virtual_drive: generate notifications mode: %d not rebuilding \n",
                                  configuration_mode);
            return FBE_STATUS_OK;
        }

        /* Validate that we were not rebuilding in the past.
         */
        if ((raid_group_p->rebuilt_pvds[FIRST_EDGE_INDEX] == FBE_OBJECT_ID_INVALID)  &&
            (raid_group_p->rebuilt_pvds[SECOND_EDGE_INDEX] == FBE_OBJECT_ID_INVALID)    )
        {
            /* We are are in pass-thru mode and not started rebuild.  Just exit.
             */
            return FBE_STATUS_OK;
        }
    }

    /* The virtual drive uses the configuration mode to determine the proper
     * notifications to send.
     */
    switch (configuration_mode)
    {
        case FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_FIRST_EDGE:
            source_edge_index = FIRST_EDGE_INDEX;
            destination_edge_index = SECOND_EDGE_INDEX;
            break;
        case FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_SECOND_EDGE:
            source_edge_index = SECOND_EDGE_INDEX;
            destination_edge_index = FIRST_EDGE_INDEX;
            break;
        case FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_FIRST_EDGE:
            source_edge_index = FIRST_EDGE_INDEX;
            destination_edge_index = SECOND_EDGE_INDEX;
            break;
        case FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_SECOND_EDGE:  
            source_edge_index = SECOND_EDGE_INDEX;
            destination_edge_index = FIRST_EDGE_INDEX;
            break;
        default:
            fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                                  FBE_TRACE_LEVEL_ERROR, 
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s unknown config mode: %d\n", __FUNCTION__, configuration_mode);
            return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Get the object ids.  We must get the destination last.
     */
    fbe_base_config_get_block_edge((fbe_base_config_t*)virtual_drive_p, &downstream_edge_p, source_edge_index);
    source_pvd_object_id = downstream_edge_p->base_edge.server_id;
    fbe_base_config_get_block_edge((fbe_base_config_t*)virtual_drive_p, &downstream_edge_p, destination_edge_index);
    destination_pvd_object_id = downstream_edge_p->base_edge.server_id;

    /* Based on the configuration mode generate the proper notifications.
     * We will only come here if and copy is: 
     *  o About to start
     *  o Is in progress
     *  o Has completed
     */
    switch (configuration_mode)
    {
        case FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_FIRST_EDGE:
        case FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_SECOND_EDGE:
            /* If the virtual drive is in pass-thru mode either:
             *  o The copy completed and we need to send the `end'
             *                      OR
             *  o The source drive was removed and we need send the `end'
             *                      OR
             *  o The desttination drive was removed and we need to send the `end'
             */
            status = fbe_virtual_drive_copy_send_start_end_notification(virtual_drive_p,
                                                               packet_p,
                                                               downstream_edge_p, 
                                                               source_edge_index, 
                                                               destination_edge_index,
                                                               DATA_RECONSTRUCTION_END);
            if (status != FBE_STATUS_OK)
            {
                fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                                      FBE_TRACE_LEVEL_WARNING,
                                      FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                      "%s send start / end  failed - status: 0x%x \n",
                                      __FUNCTION__, status);
                return status;
            }
            break;

        case FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_FIRST_EDGE:
        case FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_SECOND_EDGE:  
            /* If we are in mirror mode the copy operation can be in one of
             * three states: 
             *  1. Copy is just starting an thus we need to populate rebuild
             *     pvd ids and send the start (and possibly send the progress)
             *                      OR
             *  2. The copy is in progress but has not completed.  We need to
             *     send the progress notification.
             *                      OR
             *  3. The copy has completed.  We may need to send the progress
             *     notification and we must send the end notification.
             *                      OR
             *  4. Copy has been aborted.  Generate end notification.
             */
            fbe_raid_group_get_rb_logging_bitmask(raid_group_p, &rl_bitmask);
            fbe_raid_group_get_rebuild_checkpoint(raid_group_p, destination_edge_index, &cur_checkpoint); 
            exported_disk_capacity = fbe_raid_group_get_exported_disk_capacity(raid_group_p);
            percent_complete = fbe_raid_group_get_percent_rebuilt(raid_group_p);
            if (b_is_copy_complete == FBE_TRUE)
            {
                percent_complete = 100;
            }
            else if (cur_checkpoint > exported_disk_capacity)
            {
                percent_complete = 0;
            }

            /* Case 1: If either rebuild pvd id is invalid and the previous
             *         rebuild percent is 0, then we need to populate the rebuild
             *         pvd ids and send the `start'.
             */
            if (((raid_group_p->rebuilt_pvds[source_edge_index] == FBE_OBJECT_ID_INVALID)      ||
                 (raid_group_p->rebuilt_pvds[destination_edge_index] == FBE_OBJECT_ID_INVALID)    ) &&
                ((raid_group_p->previous_rebuild_percent[FIRST_EDGE_INDEX] == 0)               &&
                 (raid_group_p->previous_rebuild_percent[SECOND_EDGE_INDEX] == 0)                  )    )
            {
                /* If the copy is complete and we have cleared the rebuild 
                 * percent simply return.
                 */
                if (b_is_copy_complete == FBE_TRUE)
                {
                    return FBE_STATUS_OK;
                }
                /* Trace and error if we were not able to get the source or
                 * destination pvd id.
                 */
                if (raid_group_p->rebuilt_pvds[source_edge_index] == FBE_OBJECT_ID_INVALID)
                {
                    /* Some logic error occurred if we cannot locate the
                     * source pvd id.
                     */
                    if ((source_pvd_object_id == 0)                     ||
                        (source_pvd_object_id == FBE_OBJECT_ID_INVALID)    )
                    {
                        status = FBE_STATUS_GENERIC_FAILURE;
                        fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                                              FBE_TRACE_LEVEL_ERROR, 
                                              FBE_TRACE_MESSAGE_ID_INFO,
                                              "virtual_drive: generate notifications mode: %d source index: %d no pvd id \n",
                                              configuration_mode, source_edge_index);
                    }
                    else
                    {
                        /* Else populate the source rebuild pvd id.
                         */
                        raid_group_p->rebuilt_pvds[source_edge_index] = source_pvd_object_id;
                    }
                }
                if (raid_group_p->rebuilt_pvds[destination_edge_index] == FBE_OBJECT_ID_INVALID)
                {
                    /* Some logic error occurred if we cannot locate the
                     * destination pvd id.
                     */
                    if ((destination_pvd_object_id == 0)                     ||
                        (destination_pvd_object_id == FBE_OBJECT_ID_INVALID)    )
                    {
                        fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                                              FBE_TRACE_LEVEL_ERROR, 
                                              FBE_TRACE_MESSAGE_ID_INFO,
                                              "virtual_drive: generate notifications mode: %d destination index: %d no pvd id \n",
                                              configuration_mode, destination_edge_index);
                    }
                    else
                    {
                        /* Else populate the destination rebuild pvd id.
                         */
                        raid_group_p->rebuilt_pvds[destination_edge_index] = destination_pvd_object_id;
                    }
                }

                /* Now generate the `start' notifications.
                 */
                status = fbe_virtual_drive_copy_send_start_end_notification(virtual_drive_p,
                                                                   packet_p,
                                                                   downstream_edge_p,  
                                                                   source_edge_index, 
                                                                   destination_edge_index,
                                                                   DATA_RECONSTRUCTION_START);
                if (status != FBE_STATUS_OK)
                {
                    fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                                      FBE_TRACE_LEVEL_WARNING,
                                      FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                      "%s send start / end  failed - status: 0x%x \n",
                                      __FUNCTION__, status);
                    return status;
                }
                
                /* Only generate the start notification.
                 */
                return status;
            }

            /* Case 3: Generate the progress notification if required.
             *         We need to validate the neither the source or destination
             *         has been removed.
             */
            if ((rl_bitmask == 0)                                                                    &&
                (raid_group_p->previous_rebuild_percent[destination_edge_index] != percent_complete) &&
                fbe_raid_group_background_op_is_rebuild_need_to_run(raid_group_p)                    &&
                ((cur_checkpoint <= exported_disk_capacity) ||
                 (cur_checkpoint == FBE_LBA_INVALID)           )                                        )
            {
                status = fbe_virtual_drive_copy_send_progress_notification(virtual_drive_p,
                                                                           packet_p,
                                                                           downstream_edge_p, 
                                                                           source_edge_index, 
                                                                           destination_edge_index, 
                                                                           percent_complete);
                if (status != FBE_STATUS_OK)
                {
                    fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                                          FBE_TRACE_LEVEL_WARNING,
                                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                          "%s send progress failed - status: 0x%x \n",
                                          __FUNCTION__, status);
                    return status;
                }
            }
            else if ((b_is_copy_complete == FBE_TRUE)                                                     &&
                     (raid_group_p->previous_rebuild_percent[destination_edge_index] != percent_complete)    )
            {
                status = fbe_virtual_drive_copy_send_progress_notification(virtual_drive_p,
                                                                           packet_p,
                                                                           downstream_edge_p, 
                                                                           source_edge_index, 
                                                                           destination_edge_index, 
                                                                           percent_complete);
                if (status != FBE_STATUS_OK)
                {
                    fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                                          FBE_TRACE_LEVEL_WARNING,
                                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                          "%s send progress failed - status: 0x%x \n",
                                          __FUNCTION__, status);
                    return status;
                }
            }

            /* Case 4: The rebuild percent went to are is at 100.  Independant
             *         of the state (i.e. rebuild logging etc) generate the
             *         the complete (`end') notifications.
             */
            if ((percent_complete == 100)               ||
                (b_is_copy_request_aborted == FBE_TRUE)    )
            {
                status = fbe_virtual_drive_copy_send_start_end_notification(virtual_drive_p,
                                                                   packet_p,           
                                                                   downstream_edge_p,  
                                                                   source_edge_index, 
                                                                   destination_edge_index,
                                                                   DATA_RECONSTRUCTION_END);
                if (status != FBE_STATUS_OK)
                {
                    fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                                      FBE_TRACE_LEVEL_WARNING,
                                      FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                      "%s send start / end  failed - status: 0x%x \n",
                                      __FUNCTION__, status);
                    return status;
                }    
            }
            break;


        default:
            fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                                  FBE_TRACE_LEVEL_ERROR, 
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s unknown config mode: %d\n", __FUNCTION__, configuration_mode);
            return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Return the execution status.
     */
    return status;
}
/******************************************************************************
 * end fbe_virtual_drive_copy_generate_notifications()
 ******************************************************************************/

/*!****************************************************************************
 *          fbe_virtual_drive_copy_validate_swapped_out_edge_index()
 ****************************************************************************** 
 * 
 * @brief   This function validates the swapped out edge index using the current
 *          configuration mode.
 * 
 * @param   virtual_drive_p - Pointer to virtual
 *
 * @return  status  
 *
 * @author
 *  04/14/2013  Ron Proulx  - Created.
 * 
 ******************************************************************************/
fbe_status_t fbe_virtual_drive_copy_validate_swapped_out_edge_index(fbe_virtual_drive_t *virtual_drive_p)
{
    fbe_edge_index_t                        edge_index_swapped_out = FBE_EDGE_INDEX_INVALID;           
    fbe_virtual_drive_configuration_mode_t  configuration_mode;
    fbe_virtual_drive_flags_t               flags;
    fbe_lifecycle_state_t                   my_state;

    /* Get the configuration mode.
     */
    fbe_virtual_drive_get_configuration_mode(virtual_drive_p, &configuration_mode);
    fbe_virtual_drive_get_flags(virtual_drive_p, &flags);
    fbe_base_object_get_lifecycle_state((fbe_base_object_t *)virtual_drive_p, &my_state);

    /* Get the edge that was just swapped out.
     */
    edge_index_swapped_out = fbe_virtual_drive_get_swap_out_edge_index(virtual_drive_p);

    /* Validate the swapped out edge based on current configuration mode.
     */
    switch(configuration_mode)
    {
        case FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_FIRST_EDGE:
            /* The swapped out edge should be the second edge.
             */
            if (edge_index_swapped_out != SECOND_EDGE_INDEX)
            {
                /* Since the job completed successfully this is unexpected.
                 */
                fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                                      FBE_TRACE_LEVEL_ERROR,
                                      FBE_TRACE_MESSAGE_ID_INFO,
                                      "virtual_drive: validate swapped out index mode: %d life: %d flags: 0x%x expected swapped index: %d actual: %d\n",
                                      configuration_mode, my_state, flags,
                                      SECOND_EDGE_INDEX, edge_index_swapped_out);

                return FBE_STATUS_GENERIC_FAILURE;
            }
            break;

        case FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_SECOND_EDGE:
            /* The swapped out edge should be the first edge.
             */
            if (edge_index_swapped_out != FIRST_EDGE_INDEX)
            {
                /* Since the job completed successfully this is unexpected.
                 */
                fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                                      FBE_TRACE_LEVEL_ERROR,
                                      FBE_TRACE_MESSAGE_ID_INFO,
                                      "virtual_drive: validate swapped out index mode: %d life: %d flags: 0x%x expected swapped index: %d actual: %d\n",
                                      configuration_mode, my_state, flags,
                                      FIRST_EDGE_INDEX, edge_index_swapped_out);

                return FBE_STATUS_GENERIC_FAILURE;
            }
            break;

        case FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_FIRST_EDGE:
        case FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_SECOND_EDGE:
        default:
            /* Unexpected configuration mode.
             */
            fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "virtual_drive: validate swapped out index mode: %d life: %d flags: 0x%x unexpected mode\n",
                                  configuration_mode, my_state, flags);

            return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Return success
     */
    return FBE_STATUS_OK;
}
/**************************************************************
 * end fbe_virtual_drive_copy_validate_swapped_out_edge_index()
 **************************************************************/

/*!****************************************************************************
 *          fbe_virtual_drive_copy_swap_out_validate_nonpaged_metadata()
 ****************************************************************************** 
 * 
 * @brief   This function validates that we have properly set the non-paged
 *          metadata before we swap-out a completed copy position.
 * 
 * @param   virtual_drive_p - Pointer to virtual
 * @param   edge_index_to_swap_out - Edge index to swap-out
 *
 * @return  status  
 *
 * @author
 *  09/27/2012  Ron Proulx  - Created.
 * 
 ******************************************************************************/
fbe_status_t fbe_virtual_drive_copy_swap_out_validate_nonpaged_metadata(fbe_virtual_drive_t *virtual_drive_p,
                                                                        fbe_edge_index_t edge_index_to_swap_out)
{
    fbe_virtual_drive_configuration_mode_t  configuration_mode;
    fbe_lba_t                               first_edge_checkpoint = 0;
    fbe_lba_t                               second_edge_checkpoint = 0;
    fbe_lba_t                               primary_checkpoint = 0;
    fbe_lba_t                               swap_out_checkpoint = 0;
    fbe_raid_position_bitmask_t             rl_bitmask;
    fbe_edge_index_t                        primary_edge_index = FBE_EDGE_INDEX_INVALID;
    fbe_lifecycle_state_t                   my_state = FBE_LIFECYCLE_STATE_INVALID;

    /* Trace the function entry.
     */
    fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                          FBE_TRACE_LEVEL_DEBUG_LOW,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry.\n", __FUNCTION__);

    /* Get the checkpoint of the virtual drive object. */
    fbe_base_object_get_lifecycle_state((fbe_base_object_t *)virtual_drive_p, &my_state);
    fbe_virtual_drive_get_configuration_mode(virtual_drive_p, &configuration_mode);
    fbe_raid_group_get_rebuild_checkpoint((fbe_raid_group_t *)virtual_drive_p, FIRST_EDGE_INDEX, &first_edge_checkpoint);        
    fbe_raid_group_get_rebuild_checkpoint((fbe_raid_group_t *)virtual_drive_p, SECOND_EDGE_INDEX, &second_edge_checkpoint);
    fbe_raid_group_get_rb_logging_bitmask((fbe_raid_group_t *)virtual_drive_p, &rl_bitmask);

    /* First validate the configuration mode.
     */
    if ((configuration_mode != FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_FIRST_EDGE)  &&
        (configuration_mode != FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_SECOND_EDGE)    )
    {
        /* Unsupported configuration mode.
         */
        fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_INFO,
                          "virtual_drive: copy validate swap-out mode: %d index: %d mask: 0x%x chkpt:[0x%llx-0x%llx] bad mode\n",
                          configuration_mode, edge_index_to_swap_out, rl_bitmask, (unsigned long long)first_edge_checkpoint, (unsigned long long)second_edge_checkpoint);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    if (configuration_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_FIRST_EDGE)
    {
        primary_edge_index = FIRST_EDGE_INDEX;
        primary_checkpoint = first_edge_checkpoint;
        swap_out_checkpoint = second_edge_checkpoint;
    }
    else
    {
        primary_edge_index = SECOND_EDGE_INDEX;
        primary_checkpoint = second_edge_checkpoint;
        swap_out_checkpoint = first_edge_checkpoint;
    }

    /* Validate that the edge to swap out agrees with the configuration mode.
     */
    if ((configuration_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_FIRST_EDGE) &&
        (edge_index_to_swap_out != SECOND_EDGE_INDEX)                                 )
    {
        /* Incorrect edge index.
         */
        fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_INFO,
                          "virtual_drive: copy validate swap-out mode: %d index: %d mask: 0x%x chkpt:[0x%llx-0x%llx] bad index\n",
                          configuration_mode, edge_index_to_swap_out, rl_bitmask, (unsigned long long)first_edge_checkpoint, (unsigned long long)second_edge_checkpoint);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    else if ((configuration_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_SECOND_EDGE) &&
             (edge_index_to_swap_out != FIRST_EDGE_INDEX)                                   )
    {
        /* Incorrect edge index.
         */
        fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_INFO,
                          "virtual_drive: copy validate swap-out mode: %d index: %d mask: 0x%x chkpt:[0x%llx-0x%llx] bad index\n",
                          configuration_mode, edge_index_to_swap_out, rl_bitmask, (unsigned long long)first_edge_checkpoint, (unsigned long long)second_edge_checkpoint);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Validate that rebuild logging is not set for the primary index and is
     * set for the swap-out index.
     */
    if (((rl_bitmask & (1 << primary_edge_index)) != 0)     ||
        ((rl_bitmask & (1 << edge_index_to_swap_out)) == 0)    )
    {
        /* Rebuild logging not set
         */
        fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_INFO,
                          "virtual_drive: copy validate swap-out mode: %d index: %d mask: 0x%x chkpt:[0x%llx-0x%llx] rl bad\n",
                          configuration_mode, edge_index_to_swap_out, rl_bitmask, (unsigned long long)first_edge_checkpoint, (unsigned long long)second_edge_checkpoint);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Validate that the swap-out checkpoint is 0.
     */
    if (swap_out_checkpoint != 0)
    {
        /* Bad checkpoint.
         */
        fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_INFO,
                          "virtual_drive: copy validate swap-out mode: %d index: %d mask: 0x%x chkpt:[0x%llx-0x%llx] bad swap\n",
                          configuration_mode, edge_index_to_swap_out, rl_bitmask, (unsigned long long)first_edge_checkpoint, (unsigned long long)second_edge_checkpoint);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* If the lifecycle state is not failed validate that the primary
     * checkpoint is set to the end marker.
     */
    if ((my_state != FBE_LIFECYCLE_STATE_FAIL)  &&
        (primary_checkpoint != FBE_LBA_INVALID)    )
    {
        /* Bad checkpoint.
         */
        fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_INFO,
                          "virtual_drive: copy validate swap-out mode: %d index: %d mask: 0x%x chkpt:[0x%llx-0x%llx] bad primary\n",
                          configuration_mode, edge_index_to_swap_out, rl_bitmask, (unsigned long long)first_edge_checkpoint, (unsigned long long)second_edge_checkpoint);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Else we passed all the checks.
     */
    return FBE_STATUS_OK;
}
/********************************************************************************
 * end fbe_virtual_drive_copy_swap_out_validate_nonpaged_metadata()
 ********************************************************************************/

/*!***************************************************************************
 *          fbe_virtual_drive_is_copy_complete()
 ***************************************************************************** 
 * 
 * @brief   This method determines if the copy operation has completed on the
 *          virtual drive.
 *
 * @param   virtual_drive_p    - Virtual drive object.
 * 
 * @return  bool - The copy operation is complete.
 *
 * @note    This method purposefully doesn't look at path states etc.
 *
 * @author
 *  04/14/2012  Ron Proulx  - Updated to allow checking of copy complete when
 *                            the virtual drive is in pass-thru mode also.
 *
 *****************************************************************************/
fbe_bool_t fbe_virtual_drive_is_copy_complete(fbe_virtual_drive_t *virtual_drive_p)
{
    fbe_bool_t                              b_is_copy_complete = FBE_FALSE; /*! @note Default is copy is not complete. */
    fbe_virtual_drive_configuration_mode_t  configuration_mode;
    fbe_lba_t                               first_edge_checkpoint = 0;
    fbe_lba_t                               second_edge_checkpoint = 0;
    fbe_lba_t                               exported_disk_capacity = 0;
    fbe_bool_t                              b_is_metadata_rebuild_need_to_run = FBE_FALSE;

    /* Trace the function entry.
     */
    fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                          FBE_TRACE_LEVEL_DEBUG_LOW,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry.\n", __FUNCTION__);

    /* If the NP metdata is not initialized yet, the return TRUE*/
    if (fbe_raid_group_is_nonpaged_metadata_initialized((fbe_raid_group_t *)virtual_drive_p) == FBE_FALSE)
    {
        return FBE_TRUE;
    }

    /* If the checkpoint is exactly at the per-disk exported (i.e. user space)
     * capacity it means that the user space rebuild has completed.  Since
     * we must perform the metadata rebuild prior to the user space rebuild,
     * this condition indicates that the entire rebuild is complete and we
     * need to advance the rebuild checkpoint to the end marker.
     */
    fbe_virtual_drive_get_configuration_mode(virtual_drive_p, &configuration_mode);
    fbe_raid_group_get_rebuild_checkpoint((fbe_raid_group_t *)virtual_drive_p, FIRST_EDGE_INDEX, &first_edge_checkpoint);        
    fbe_raid_group_get_rebuild_checkpoint((fbe_raid_group_t *)virtual_drive_p, SECOND_EDGE_INDEX, &second_edge_checkpoint);
    exported_disk_capacity = fbe_raid_group_get_exported_disk_capacity((fbe_raid_group_t *)virtual_drive_p);
    b_is_metadata_rebuild_need_to_run = fbe_raid_group_background_op_is_metadata_rebuild_need_to_run((fbe_raid_group_t *)virtual_drive_p);

    /* If the destination is fully rebuilt (independent of the edge health) the
     * copy is complete.
     */
    switch(configuration_mode)
    {
        case FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_FIRST_EDGE:
            /* If the primary checkpoint is already at the end marker then
             * there is no copy in progress and therefore the copy is not 
             * complete.  Else if the checkpoint is at the exported capacity
             * the copy is complete and we need to set the checkpoint to the
             * end marker.
             */
            if (first_edge_checkpoint == FBE_LBA_INVALID)
            {
                /* Pass-thru return False.
                 */
                return FBE_FALSE;
            }

            /* If there are no paged metadata chunks that need to be rebuilt and
             * the first edge checkpoint is exactly at the end of user space then 
             * the rebuild is complete. 
             */
            if ((b_is_metadata_rebuild_need_to_run == FBE_FALSE)  &&
                (first_edge_checkpoint == exported_disk_capacity)    )
            {
                /* If the pass-thru primary position is completely rebuilt
                 * then the copy is complete. 
                 */
                b_is_copy_complete = FBE_TRUE;
            }
            break;

        case FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_SECOND_EDGE:
            /* If the primary checkpoint is already at the end marker then
             * there is no copy in progress and therefore the copy is not 
             * complete.  Else if the checkpoint is at the exported capacity
             * the copy is complete and we need to set the checkpoint to the
             * end marker.
             */
            if (second_edge_checkpoint == FBE_LBA_INVALID)
            {
                /* Pass-thru return False.
                 */
                return FBE_FALSE;
            }

            /* If there are no paged metadata chunks that need to be rebuilt and
             * the first edge checkpoint is exactly at the end of user space then 
             * the rebuild is complete. 
             */
            if ((b_is_metadata_rebuild_need_to_run == FBE_FALSE)   &&
                (second_edge_checkpoint == exported_disk_capacity)    )
            {
                /* If the pass-thru primary position is completely rebuilt
                 * then the copy is complete. 
                 */
                b_is_copy_complete = FBE_TRUE;
            }
            break;

        case FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_FIRST_EDGE:
            /* If there are no paged metadata chunks that need to be rebuilt and
             * the second edge checkpoint is either at the end marker or exactly at 
             * the end of user space, then the rebuild is complete. 
             */
            if ((b_is_metadata_rebuild_need_to_run == FBE_FALSE)         &&
                ((second_edge_checkpoint == FBE_LBA_INVALID)        ||
                 (second_edge_checkpoint == exported_disk_capacity)    )    )
            {
                /* If the mirror destination position is completely rebuilt
                 * then the copy is complete. 
                 */
                b_is_copy_complete = FBE_TRUE;
            }
            break;

        case FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_SECOND_EDGE:
            /* If there are no paged metadata chunks that need to be rebuilt and
             * the first edge checkpoint is either at the end marker or exactly at 
             * the end of user space, then the rebuild is complete. 
             */
            if ((b_is_metadata_rebuild_need_to_run == FBE_FALSE)       &&
                ((first_edge_checkpoint == FBE_LBA_INVALID)        ||
                 (first_edge_checkpoint == exported_disk_capacity)    )    )
            {
                /* If the mirror destination position is completely rebuilt
                 * then the copy is complete. 
                 */
                b_is_copy_complete = FBE_TRUE;
            }
            break;

        default:
            /* Else unsupported configuration mode
             */
            fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s unsupported mode: %d \n",
                              __FUNCTION__, configuration_mode);
            return FBE_FALSE;
    }

    /* Trace some information if enabled.
     */
    fbe_virtual_drive_trace(virtual_drive_p,
                            FBE_TRACE_LEVEL_INFO,
                            FBE_VIRTUAL_DRIVE_DEBUG_FLAG_COPY_TRACING,
                            "virtual_drive: is copy complete: %d mode: %d first: 0x%llx second: 0x%llx \n",
                            b_is_copy_complete, configuration_mode, (unsigned long long)first_edge_checkpoint, (unsigned long long)second_edge_checkpoint);

    /* Return if copy is complete or not.
     */
    return b_is_copy_complete;
}
/******************************************
 * end fbe_virtual_drive_is_copy_complete()
 ******************************************/

/*!****************************************************************************
 *          fbe_virtual_drive_copy_complete_write_checkpoints()
 ******************************************************************************
 *
 * @brief   A copy operation has successfully completed.  Update the non-paged
 *          metadata appropriately
 * 
 * @param   packet_p - Pointer to monitor packet 
 * @param   context - Pointer to virtual drive object
 * 
 * @return  status - FBE_STATUS_OK - Previous request failed - complete packet
 *                   FBE_STATUS_MORE_PROCESSING_REQUIRED - Continue 
 *
 * @note    Completion - Just return status
 *
 * @author
 *  04/14/2013  Ron Proulx  - Only execute checkpoint update after the copy
 *                            job has completed.
 *
 ******************************************************************************/
static fbe_status_t fbe_virtual_drive_copy_complete_write_checkpoints(fbe_packet_t *packet_p,                                    
                                                                      fbe_packet_completion_context_t context)

{
    fbe_status_t                            status = FBE_STATUS_OK;                 
    fbe_virtual_drive_t                    *virtual_drive_p = NULL;
    fbe_edge_index_t                        edge_index_to_set = FBE_EDGE_INDEX_INVALID;
    fbe_edge_index_t                        edge_index_to_clear = FBE_EDGE_INDEX_INVALID;
    fbe_virtual_drive_configuration_mode_t  configuration_mode;    
    fbe_lba_t                               first_edge_checkpoint = 0;
    fbe_lba_t                               second_edge_checkpoint = 0;
    fbe_raid_position_bitmask_t             rl_bitmask;
    fbe_u64_t                               metadata_offset;
    fbe_u32_t                               metadata_write_size;               
    fbe_raid_group_nonpaged_metadata_t     *nonpaged_metadata_p = NULL;
    fbe_raid_group_rebuild_nonpaged_info_t  rebuild_nonpaged_info;
    fbe_raid_group_rebuild_nonpaged_info_t *rebuild_nonpaged_info_p = NULL;
    fbe_lifecycle_state_t                   my_state = FBE_LIFECYCLE_STATE_INVALID;
    
    /* Get the virtual drive object.
     */
    virtual_drive_p = (fbe_virtual_drive_t *)context;

    /* If the status is not ok, that means we didn't get the 
     * lock. Just return. we are already in the completion routine
     */
    status = fbe_transport_get_status_code(packet_p);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*)virtual_drive_p, 
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s can't get the NP lock, status: 0x%x", 
                              __FUNCTION__, status); 
        return FBE_STATUS_OK;
    }

    /* Set the completion function to release the NP lock.
     * Once we set the completion we must:
     *  o Complete the packet
     *  o Return `more processing required'
     */
    fbe_transport_set_completion_function(packet_p, fbe_raid_group_release_NP_lock, virtual_drive_p);

    /* Use the virtual drive specific method to locate the disk that needs it
     * rebuild checkpoint set.
     */
    edge_index_to_clear = fbe_virtual_drive_get_swap_out_edge_index(virtual_drive_p);
    edge_index_to_set = (edge_index_to_clear == FIRST_EDGE_INDEX) ? SECOND_EDGE_INDEX : FIRST_EDGE_INDEX;

    /* Trace some information if enabled.
     */
    fbe_virtual_drive_trace(virtual_drive_p,
                            FBE_TRACE_LEVEL_INFO,
                            FBE_VIRTUAL_DRIVE_DEBUG_FLAG_SWAP_TRACING,
                            "virtual_drive: copy complete write chkpts index to set: %d\n",
                            edge_index_to_set);

    /* If we could not locate the edge that needs to have the checkpoint set
     * it is an error.
     */
    status = fbe_virtual_drive_copy_validate_swapped_out_edge_index(virtual_drive_p);
    if (status != FBE_STATUS_OK)
    {
        /* Unexpected swapped out edge index or mode.
         */
        fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s validate swapped index: %d failed - status: 0x%x\n",
                              __FUNCTION__, edge_index_to_clear, status);

        /* Perform any neccessary cleanup
         */
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }

    /* Generate the index to clear.
     */

    /*! @note Invoke virtual drive specific method that:
     *          o Sets the primary position checkpoint to the end marker
     *          o Sets the alternate position checkpoint to zero (0)
     *          o Sets the `rebuild logging' flag for the alternate position
     */
    fbe_base_object_get_lifecycle_state((fbe_base_object_t *)virtual_drive_p, &my_state);
    fbe_virtual_drive_get_configuration_mode(virtual_drive_p, &configuration_mode);
    fbe_raid_group_get_rebuild_checkpoint((fbe_raid_group_t *)virtual_drive_p, FIRST_EDGE_INDEX, &first_edge_checkpoint);        
    fbe_raid_group_get_rebuild_checkpoint((fbe_raid_group_t *)virtual_drive_p, SECOND_EDGE_INDEX, &second_edge_checkpoint);
    fbe_raid_group_get_rb_logging_bitmask((fbe_raid_group_t *)virtual_drive_p, &rl_bitmask);

    /* Always trace this method.
     */
    fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "virtual_drive: copy complete write chkpts mode: %d set index: %d mask: 0x%x first: 0x%llx second: 0x%llx \n",
                          configuration_mode, edge_index_to_set, rl_bitmask, 
                          (unsigned long long)first_edge_checkpoint, 
                          (unsigned long long)second_edge_checkpoint);

    /* Get a pointer to the non-paged metadata.
     */ 
    fbe_base_config_get_nonpaged_metadata_ptr((fbe_base_config_t *)virtual_drive_p, (void **)&nonpaged_metadata_p);

    /* Copy the existing data to the structure we will write.
     */
    rebuild_nonpaged_info_p = &rebuild_nonpaged_info;
    *rebuild_nonpaged_info_p = *(&(nonpaged_metadata_p->rebuild_info));

    /* Clear rebuild logging for the destination (position to set end marker
     * for) and set it for the source.
     */
    rebuild_nonpaged_info_p->rebuild_logging_bitmask &= ~(1 << edge_index_to_set);
    rebuild_nonpaged_info_p->rebuild_logging_bitmask |= (1 << edge_index_to_clear);

    /* Now set the source checkpoint to 0 and the destination checkpoint to the
     * end marker.
     */
    rebuild_nonpaged_info_p->rebuild_checkpoint_info[0].checkpoint = 0;
    rebuild_nonpaged_info_p->rebuild_checkpoint_info[0].position = edge_index_to_clear;
    rebuild_nonpaged_info_p->rebuild_checkpoint_info[1].checkpoint = FBE_LBA_INVALID;
    rebuild_nonpaged_info_p->rebuild_checkpoint_info[1].position = FBE_RAID_INVALID_DISK_POSITION;

    /* Trace some information.
     */
    fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                          FBE_TRACE_LEVEL_WARNING,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "virtual_drive: copy complete mode: %d life: %d rl_mask: 0x%x chkpts:[0x%llx-0x%llx] \n",
                          configuration_mode, my_state,
                          rebuild_nonpaged_info_p->rebuild_logging_bitmask,
                          rebuild_nonpaged_info_p->rebuild_checkpoint_info[0].checkpoint,
                          rebuild_nonpaged_info_p->rebuild_checkpoint_info[1].checkpoint);

    /* Now generate the write size and offset
     */
    metadata_write_size = sizeof(*rebuild_nonpaged_info_p);
    metadata_offset = (fbe_u64_t)(&((fbe_raid_group_nonpaged_metadata_t*)0)->rebuild_info);

    /* Now send the write for the nonpaged.
     */ 
    status = fbe_base_config_metadata_nonpaged_write_persist((fbe_base_config_t*)virtual_drive_p, 
                                                             packet_p, 
                                                             metadata_offset, 
                                                             (fbe_u8_t *)rebuild_nonpaged_info_p,
                                                             metadata_write_size);

    /* Return more processing since a packet is outstanding.
     */
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/***************************************************************************
 * end fbe_virtual_drive_copy_complete_write_checkpoints()
 ***************************************************************************/ 

/*!****************************************************************************
 *          fbe_virtual_drive_copy_complete_set_checkpoint_to_end_marker()
 ******************************************************************************
 *
 * @brief   This function is called when a copy job has completed successfully.
 *          It updates both checkpoints of the virtual drive and the rebuild
 *          logging mask.  It simply uses the configuration mode (it does NOT
 *          look at any paths states etc since this virtual drive is not in
 *          pass-thru mode and we have acknowledged the copy completion !).
 *              o Set the previous source edge (now invalid) checkpoint to 0
 *              o Set the new pass-thru edge checkpoint to the end marker
 *              o Mark the old source edge as rebuild logging
 *              o If not rebuild logging on the pass-thru edge clear it
 *
 * @param   virtual_drive_p - Pointer to virtual drive object to set checkpoint for
 * @param   packet_p - pointer to a control packet from the scheduler
 *
 * @return  status - Typically FBE_STATUS_MORE_PROCESSING_REQUIRED
 * 
 * @author
 *  04/14/2013  Ron Proulx  - Created.
 *
 ******************************************************************************/
fbe_status_t fbe_virtual_drive_copy_complete_set_checkpoint_to_end_marker(fbe_virtual_drive_t *virtual_drive_p,
                                                                          fbe_packet_t *packet_p)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_edge_index_t                        swapped_out_edge_index = FBE_EDGE_INDEX_INVALID;
    fbe_bool_t                              b_is_active;
    fbe_bool_t                              b_is_edge_swapped;
    fbe_bool_t                              b_is_source_failed;
    fbe_virtual_drive_configuration_mode_t  configuration_mode;

    /* Trace the function entry
     */
    fbe_base_object_trace((fbe_base_object_t*)virtual_drive_p,     
                          FBE_TRACE_LEVEL_DEBUG_HIGH,          
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY, 
                          "%s entry\n", __FUNCTION__); 

    /* Get the configuration mode for debug purposes.
     */
    fbe_virtual_drive_get_configuration_mode(virtual_drive_p, &configuration_mode);

    /* We we are not active (alternate could have taken over?)
     */
    fbe_raid_group_monitor_check_active_state((fbe_raid_group_t *)virtual_drive_p, &b_is_active);
   
    /* Determine if we need to clear the swapped bit or not.
     */
    fbe_virtual_drive_metadata_is_edge_swapped(virtual_drive_p, &b_is_edge_swapped);
    
    /* Determine if the source edge was failed
     */
    fbe_virtual_drive_metadata_is_source_failed(virtual_drive_p, &b_is_source_failed);
      
    /* Get the swapped out edge index.
     */
    swapped_out_edge_index = fbe_virtual_drive_get_swap_out_edge_index(virtual_drive_p);

    /* Trace some information if enabled.
     */
    fbe_virtual_drive_trace(virtual_drive_p,
                            FBE_TRACE_LEVEL_INFO,
                            FBE_VIRTUAL_DRIVE_DEBUG_FLAG_SWAP_TRACING,
                            "virtual_drive: copy complete set chkpt to end active: %d mode: %d swapped: %d swapped edge index: %d\n",
                            b_is_active, configuration_mode, b_is_edge_swapped, swapped_out_edge_index);

    /*! @note Currently we expect this condition to only occur in the Ready 
     *        state and thus we must find an edge index to set.
     */
    status = fbe_virtual_drive_copy_validate_swapped_out_edge_index(virtual_drive_p);
    if (status != FBE_STATUS_OK)
    {
        /* Unexpected swapped out edge index or mode.
         */
        fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s validate swapped index: %d failed - status: 0x%x\n",
                              __FUNCTION__, swapped_out_edge_index, status);

        /* Perform any neccessary cleanup
         */
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return status;
    }

    /* Set the rebuild checkpoint to the end marker on the permanent spare.
     */
    fbe_transport_set_completion_function(packet_p, 
                                          fbe_virtual_drive_metadata_set_checkpoint_to_end_marker_completion, 
                                          virtual_drive_p);    
    status = fbe_raid_group_get_NP_lock((fbe_raid_group_t *)virtual_drive_p, 
                                        packet_p, 
                                        fbe_virtual_drive_copy_complete_write_checkpoints);
        
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/*************************************************************************************
 * end fbe_virtual_drive_copy_complete_set_checkpoint_to_end_marker()
 *************************************************************************************/

/*!****************************************************************************
 *          fbe_virtual_drive_dest_drive_failed_write_checkpoints()
 ******************************************************************************
 *
 * @brief   The destination drive died during a copy operation.  The copy
 *          operation was successfully aborted and now we need to reset the 
 *          checkpoints etc, in the virtual drive accordingly.
 * 
 * @param   packet_p - Pointer to monitor packet 
 * @param   context - Pointer to virtual drive object
 * 
 * @return  status - FBE_STATUS_OK - Previous request failed - complete packet
 *                   FBE_STATUS_MORE_PROCESSING_REQUIRED - Continue 
 *
 * @note    Completion - Just return status
 *
 * @author
 *  04/14/2013  Ron Proulx  - Only execute checkpoint update after the copy
 *                            job has completed.
 *
 ******************************************************************************/
static fbe_status_t fbe_virtual_drive_dest_drive_failed_write_checkpoints(fbe_packet_t *packet_p,                                    
                                                                          fbe_packet_completion_context_t context)

{
    fbe_status_t                            status = FBE_STATUS_OK;                 
    fbe_virtual_drive_t                    *virtual_drive_p = NULL;
    fbe_edge_index_t                        source_edge_index = FBE_EDGE_INDEX_INVALID;
    fbe_edge_index_t                        dest_edge_index = FBE_EDGE_INDEX_INVALID;
    fbe_virtual_drive_configuration_mode_t  configuration_mode;    
    fbe_lba_t                               first_edge_checkpoint = 0;
    fbe_lba_t                               second_edge_checkpoint = 0;
    fbe_lba_t                               source_drive_checkpoint = 0;
    fbe_raid_position_bitmask_t             rl_bitmask;
    fbe_u64_t                               metadata_offset;
    fbe_u32_t                               metadata_write_size;               
    fbe_raid_group_nonpaged_metadata_t     *nonpaged_metadata_p = NULL;
    fbe_raid_group_rebuild_nonpaged_info_t  rebuild_nonpaged_info;
    fbe_raid_group_rebuild_nonpaged_info_t *rebuild_nonpaged_info_p = NULL;
    fbe_lifecycle_state_t                   my_state = FBE_LIFECYCLE_STATE_INVALID;
    
    /* Get the virtual drive object.
     */
    virtual_drive_p = (fbe_virtual_drive_t *)context;

    /* If the status is not ok, that means we didn't get the 
     * lock. Just return. we are already in the completion routine
     */
    status = fbe_transport_get_status_code(packet_p);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*)virtual_drive_p, 
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s can't get the NP lock, status: 0x%x", 
                              __FUNCTION__, status); 
        return FBE_STATUS_OK;
    }

    /* Set the completion function to release the NP lock.
     * Once we set the completion we must:
     *  o Complete the packet
     *  o Return `more processing required'
     */
    fbe_transport_set_completion_function(packet_p, fbe_raid_group_release_NP_lock, virtual_drive_p);

    /* Use the virtual drive specific method to locate the disk that needs it
     * rebuild checkpoint set.
     */
    dest_edge_index = fbe_virtual_drive_get_swap_out_edge_index(virtual_drive_p);
    source_edge_index = (dest_edge_index == FIRST_EDGE_INDEX) ? SECOND_EDGE_INDEX : FIRST_EDGE_INDEX;

    /* Trace some information if enabled.
     */
    fbe_virtual_drive_trace(virtual_drive_p,
                            FBE_TRACE_LEVEL_INFO,
                            FBE_VIRTUAL_DRIVE_DEBUG_FLAG_SWAP_TRACING,
                            "virtual_drive: dest failed write chkpts index to clear: %d\n",
                            dest_edge_index);

    /* If we could not locate the edge that needs to have the checkpoint set
     * it is an error.
     */
    status = fbe_virtual_drive_copy_validate_swapped_out_edge_index(virtual_drive_p);
    if (status != FBE_STATUS_OK)
    {
        /* Unexpected swapped out edge index or mode.
         */
        fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s validate swapped index: %d failed - status: 0x%x\n",
                              __FUNCTION__, dest_edge_index, status);

        /* Perform any neccessary cleanup
         */
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }

    /*! @note Invoke virtual drive specific method that:
     *          o Sets the primary position checkpoint to the end marker
     *          o Sets the alternate position checkpoint to zero (0)
     *          o Sets the `rebuild logging' flag for the alternate position
     */
    fbe_base_object_get_lifecycle_state((fbe_base_object_t *)virtual_drive_p, &my_state);
    fbe_virtual_drive_get_configuration_mode(virtual_drive_p, &configuration_mode);
    fbe_raid_group_get_rebuild_checkpoint((fbe_raid_group_t *)virtual_drive_p, FIRST_EDGE_INDEX, &first_edge_checkpoint);        
    fbe_raid_group_get_rebuild_checkpoint((fbe_raid_group_t *)virtual_drive_p, SECOND_EDGE_INDEX, &second_edge_checkpoint);
    fbe_raid_group_get_rebuild_checkpoint((fbe_raid_group_t *)virtual_drive_p, source_edge_index, &source_drive_checkpoint);
    fbe_raid_group_get_rb_logging_bitmask((fbe_raid_group_t *)virtual_drive_p, &rl_bitmask);

    /* Always trace this method.
     */
    fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "virtual_drive: dest failed write chkpts mode: %d clear index: %d mask: 0x%x first: 0x%llx second: 0x%llx \n",
                          configuration_mode, dest_edge_index, rl_bitmask, 
                          (unsigned long long)first_edge_checkpoint, 
                          (unsigned long long)second_edge_checkpoint);

    /* Get a pointer to the non-paged metadata.
     */ 
    fbe_base_config_get_nonpaged_metadata_ptr((fbe_base_config_t *)virtual_drive_p, (void **)&nonpaged_metadata_p);

    /* Copy the existing data to the structure we will write.
     */
    rebuild_nonpaged_info_p = &rebuild_nonpaged_info;
    *rebuild_nonpaged_info_p = *(&(nonpaged_metadata_p->rebuild_info));

    /* Clear rebuild logging for the source position and set rebuild logging
     * for the destination.
     */
    rebuild_nonpaged_info_p->rebuild_logging_bitmask &= ~(1 << source_edge_index);
    rebuild_nonpaged_info_p->rebuild_logging_bitmask |= (1 << dest_edge_index);

    /* Set the destination checkpoint to 0 and the source to invalid.
     */
    rebuild_nonpaged_info_p->rebuild_checkpoint_info[0].checkpoint = 0;
    rebuild_nonpaged_info_p->rebuild_checkpoint_info[0].position = dest_edge_index;
    rebuild_nonpaged_info_p->rebuild_checkpoint_info[1].checkpoint = source_drive_checkpoint;
    rebuild_nonpaged_info_p->rebuild_checkpoint_info[1].position = FBE_RAID_INVALID_DISK_POSITION;

    /* Trace some information.
     */
    fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                          FBE_TRACE_LEVEL_WARNING,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "virtual_drive: dest failed mode: %d life: %d rl_mask: 0x%x chkpts:[0x%llx-0x%llx] \n",
                          configuration_mode, my_state,
                          rebuild_nonpaged_info_p->rebuild_logging_bitmask,
                          rebuild_nonpaged_info_p->rebuild_checkpoint_info[0].checkpoint,
                          rebuild_nonpaged_info_p->rebuild_checkpoint_info[1].checkpoint);

    /* Now generate the write size and offset
     */
    metadata_write_size = sizeof(*rebuild_nonpaged_info_p);
    metadata_offset = (fbe_u64_t)(&((fbe_raid_group_nonpaged_metadata_t*)0)->rebuild_info);

    /* Now send the write for the nonpaged.
     */ 
    status = fbe_base_config_metadata_nonpaged_write_persist((fbe_base_config_t*)virtual_drive_p, 
                                                             packet_p, 
                                                             metadata_offset, 
                                                             (fbe_u8_t *)rebuild_nonpaged_info_p,
                                                             metadata_write_size);

    /* Return more processing since a packet is outstanding.
     */
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/***************************************************************************
 * end fbe_virtual_drive_dest_drive_failed_write_checkpoints()
 ***************************************************************************/ 

/*!****************************************************************************
 *          fbe_virtual_drive_dest_drive_failed_set_checkpoint_to_end_marker()
 ******************************************************************************
 *
 * @brief   This function is called when a copy job has failed (in the Ready state).
 *          It updates both checkpoints of the virtual drive and the rebuild
 *          logging mask.  It simply uses the configuration mode (it does NOT
 *          look at any paths states etc since this virtual drive is not in
 *          pass-thru mode and we have acknowledged the copy completion !).
 *              o Set the previous source edge (now invalid) checkpoint to 0
 *              o Set the new pass-thru edge checkpoint to the end marker
 *              o Mark the old source edge as rebuild logging
 *              o If not rebuild logging on the pass-thru edge clear it
 *
 * @param   virtual_drive_p - Pointer to virtual drive object to set checkpoint for
 * @param   packet_p - pointer to a control packet from the scheduler
 *
 * @return  status - Typically FBE_STATUS_MORE_PROCESSING_REQUIRED
 * 
 * @author
 *  04/14/2013  Ron Proulx  - Created.
 *
 ******************************************************************************/
fbe_status_t fbe_virtual_drive_dest_drive_failed_set_checkpoint_to_end_marker(fbe_virtual_drive_t *virtual_drive_p,
                                                                              fbe_packet_t *packet_p)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_edge_index_t                        swapped_out_edge_index = FBE_EDGE_INDEX_INVALID;
    fbe_bool_t                              b_is_active;
    fbe_bool_t                              b_is_edge_swapped;
    fbe_bool_t                              b_is_source_failed;
    fbe_virtual_drive_configuration_mode_t  configuration_mode;

    /* Trace the function entry
     */
    fbe_base_object_trace((fbe_base_object_t*)virtual_drive_p,     
                          FBE_TRACE_LEVEL_DEBUG_HIGH,          
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY, 
                          "%s entry\n", __FUNCTION__); 

    /* Get the configuration mode for debug purposes.
     */
    fbe_virtual_drive_get_configuration_mode(virtual_drive_p, &configuration_mode);

    /* We we are not active (alternate could have taken over?)
     */
    fbe_raid_group_monitor_check_active_state((fbe_raid_group_t *)virtual_drive_p, &b_is_active);
   
    /* Determine if we need to clear the swapped bit or not.
     */
    fbe_virtual_drive_metadata_is_edge_swapped(virtual_drive_p, &b_is_edge_swapped);
    
    /* Determine if the source edge was failed
     */
    fbe_virtual_drive_metadata_is_source_failed(virtual_drive_p, &b_is_source_failed);
      
    /* Get the swapped out edge index.
     */
    swapped_out_edge_index = fbe_virtual_drive_get_swap_out_edge_index(virtual_drive_p);

    /* Trace some information if enabled.
     */
    fbe_virtual_drive_trace(virtual_drive_p,
                            FBE_TRACE_LEVEL_INFO,
                            FBE_VIRTUAL_DRIVE_DEBUG_FLAG_SWAP_TRACING,
                            "virtual_drive: copy complete set chkpt to end active: %d mode: %d swapped: %d swapped edge index: %d\n",
                            b_is_active, configuration_mode, b_is_edge_swapped, swapped_out_edge_index);

    /*! @note Currently we expect this condition to only occur in the Ready 
     *        state and thus we must find an edge index to set.
     */
    status = fbe_virtual_drive_copy_validate_swapped_out_edge_index(virtual_drive_p);
    if (status != FBE_STATUS_OK)
    {
        /* Unexpected swapped out edge index or mode.
         */
        fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s validate swapped index: %d failed - status: 0x%x\n",
                              __FUNCTION__, swapped_out_edge_index, status);

        /* Perform any neccessary cleanup
         */
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return status;
    }

    /* Set the rebuild checkpoint to the end marker on the permanent spare.
     */
    fbe_transport_set_completion_function(packet_p, 
                                          fbe_virtual_drive_metadata_set_checkpoint_to_end_marker_completion, 
                                          virtual_drive_p);    
    status = fbe_raid_group_get_NP_lock((fbe_raid_group_t *)virtual_drive_p, 
                                        packet_p, 
                                        fbe_virtual_drive_dest_drive_failed_write_checkpoints);
        
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/*************************************************************************************
 * end fbe_virtual_drive_dest_drive_failed_set_checkpoint_to_end_marker()
 *************************************************************************************/

/*!****************************************************************************
 *          fbe_virtual_drive_copy_failed_write_checkpoints()
 ******************************************************************************
 *
 * @brief   A copy operation has failed. Update the non-paged
 *          metadata appropriately.
 * 
 * @param   packet_p - Pointer to monitor packet 
 * @param   context - Pointer to virtual drive object
 * 
 * @return  status - FBE_STATUS_OK - Previous request failed - complete packet
 *                   FBE_STATUS_MORE_PROCESSING_REQUIRED - Continue 
 *
 * @note    Completion - Just return status
 *
 * @author
 *  04/14/2013  Ron Proulx  - Only execute checkpoint update after the copy
 *                            job has completed.
 *
 ******************************************************************************/
static fbe_status_t fbe_virtual_drive_copy_failed_write_checkpoints(fbe_packet_t *packet_p,                                    
                                                                    fbe_packet_completion_context_t context)

{
    fbe_status_t                            status = FBE_STATUS_OK;                 
    fbe_virtual_drive_t                    *virtual_drive_p = NULL;
    fbe_edge_index_t                        edge_index_to_set = FBE_EDGE_INDEX_INVALID;
    fbe_edge_index_t                        edge_index_to_clear = FBE_EDGE_INDEX_INVALID;
    fbe_edge_index_t                        source_edge_index = FBE_EDGE_INDEX_INVALID;
    fbe_edge_index_t                        dest_edge_index = FBE_EDGE_INDEX_INVALID;
    fbe_bool_t                              b_is_source_failed = FBE_TRUE;
    fbe_virtual_drive_configuration_mode_t  configuration_mode;    
    fbe_lba_t                               first_edge_checkpoint = 0;
    fbe_lba_t                               second_edge_checkpoint = 0;
    fbe_lba_t                               checkpoint_to_set = 0;
    fbe_lba_t                               source_drive_checkpoint = 0;
    fbe_lba_t                               dest_drive_checkpoint = 0;
    fbe_raid_position_bitmask_t             rl_bitmask;
    fbe_u64_t                               metadata_offset;
    fbe_u32_t                               metadata_write_size;               
    fbe_raid_group_nonpaged_metadata_t     *nonpaged_metadata_p = NULL;
    fbe_raid_group_rebuild_nonpaged_info_t  rebuild_nonpaged_info;
    fbe_raid_group_rebuild_nonpaged_info_t *rebuild_nonpaged_info_p = NULL;
    fbe_lifecycle_state_t                   my_state = FBE_LIFECYCLE_STATE_INVALID;
    fbe_path_state_t                        source_edge_path_state = FBE_PATH_STATE_INVALID;
    fbe_path_state_t                        dest_edge_path_state = FBE_PATH_STATE_INVALID;
    
    /* Get the virtual drive object.
     */
    virtual_drive_p = (fbe_virtual_drive_t *)context;

    /* If the status is not ok, that means we didn't get the 
     * lock. Just return. we are already in the completion routine
     */
    status = fbe_transport_get_status_code(packet_p);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*)virtual_drive_p, 
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s can't get the NP lock, status: 0x%x", 
                              __FUNCTION__, status); 
        return FBE_STATUS_OK;
    }

    /* Set the completion function to release the NP lock.
     * Once we set the completion we must:
     *  o Complete the packet
     *  o Return `more processing required'
     */
    fbe_transport_set_completion_function(packet_p, fbe_raid_group_release_NP_lock, virtual_drive_p);

    /* Use the virtual drive specific method to locate the disk that needs it
     * rebuild checkpoint set.
     */
    fbe_virtual_drive_metadata_is_source_failed(virtual_drive_p, &b_is_source_failed);
    edge_index_to_clear = fbe_virtual_drive_get_swap_out_edge_index(virtual_drive_p);
    edge_index_to_set = (edge_index_to_clear == FIRST_EDGE_INDEX) ? SECOND_EDGE_INDEX : FIRST_EDGE_INDEX;


    /* Trace some information if enabled.
     */
    fbe_virtual_drive_trace(virtual_drive_p,
                            FBE_TRACE_LEVEL_INFO,
                            FBE_VIRTUAL_DRIVE_DEBUG_FLAG_SWAP_TRACING,
                            "virtual_drive: copy failed write chkpts index to set: %d source failed: %d\n",
                            edge_index_to_set, b_is_source_failed);

    /* If we could not locate the edge that needs to have the checkpoint set
     * it is an error.
     */
    status = fbe_virtual_drive_copy_validate_swapped_out_edge_index(virtual_drive_p);
    if (status != FBE_STATUS_OK)
    {
        /* Unexpected swapped out edge index or mode.
         */
        fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s validate swapped index: %d failed - status: 0x%x\n",
                              __FUNCTION__, edge_index_to_clear, status);

        /* Perform any neccessary cleanup
         */
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }

    /* Generate the index to clear.
     */

    /*! @note Invoke virtual drive specific method that:
     *          o Sets the primary position checkpoint to the end marker
     *          o Sets the alternate position checkpoint to zero (0)
     *          o Sets the `rebuild logging' flag for the alternate position
     */
    fbe_base_object_get_lifecycle_state((fbe_base_object_t *)virtual_drive_p, &my_state);
    fbe_virtual_drive_get_configuration_mode(virtual_drive_p, &configuration_mode);
    fbe_raid_group_get_rebuild_checkpoint((fbe_raid_group_t *)virtual_drive_p, FIRST_EDGE_INDEX, &first_edge_checkpoint);        
    fbe_raid_group_get_rebuild_checkpoint((fbe_raid_group_t *)virtual_drive_p, SECOND_EDGE_INDEX, &second_edge_checkpoint);
    fbe_raid_group_get_rebuild_checkpoint((fbe_raid_group_t *)virtual_drive_p, edge_index_to_set, &checkpoint_to_set);
    fbe_raid_group_get_rb_logging_bitmask((fbe_raid_group_t *)virtual_drive_p, &rl_bitmask);

    /* We have already changed to pass-thru mode.  We simply use the checkpoint
     * of the edge index to set to the end marker to `determine' what `was' the
     * source drive and what was the destination drive.  We PURPOSEFULLY don't
     * use path states to determine how to set the checkpoints.
     */
    if (checkpoint_to_set == FBE_LBA_INVALID)
    {
        /* This implies that both the destination and failed because the
         * checkpoint should only be at the end marker if we selected the
         * source and that implies both destination and source failed.
         */
        source_edge_index = edge_index_to_set;
        dest_edge_index = edge_index_to_clear;
    }
    else
    {
        /* Else only the source drive failed so we as setting the destination
         * drive to the `end marker' (we don't modify it).
         */
        source_edge_index = edge_index_to_clear;
        dest_edge_index = edge_index_to_set;
    }
    fbe_raid_group_get_rebuild_checkpoint((fbe_raid_group_t *)virtual_drive_p, source_edge_index, &source_drive_checkpoint);        
    fbe_raid_group_get_rebuild_checkpoint((fbe_raid_group_t *)virtual_drive_p, dest_edge_index, &dest_drive_checkpoint);
    fbe_block_transport_get_path_state(&((fbe_base_config_t *)virtual_drive_p)->block_edge_p[dest_edge_index],
                                       &dest_edge_path_state);
    fbe_block_transport_get_path_state(&((fbe_base_config_t *)virtual_drive_p)->block_edge_p[source_edge_index],
                                       &source_edge_path_state);

    /* Always trace this method.
     */
    fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "virtual_drive: copy failed write chkpts mode: %d set index: %d mask: 0x%x first: 0x%llx second: 0x%llx \n",
                          configuration_mode, edge_index_to_set, rl_bitmask, 
                          (unsigned long long)first_edge_checkpoint, 
                          (unsigned long long)second_edge_checkpoint);

    /* Get a pointer to the non-paged metadata.
     */ 
    fbe_base_config_get_nonpaged_metadata_ptr((fbe_base_config_t *)virtual_drive_p, (void **)&nonpaged_metadata_p);

    /* Copy the existing data to the structure we will write.
     */
    rebuild_nonpaged_info_p = &rebuild_nonpaged_info;
    *rebuild_nonpaged_info_p = *(&(nonpaged_metadata_p->rebuild_info));

    /* If the copy was aborted (due to the source and destination drives failing)
     * the following conditions will be true:
     *  o Edge index to set is the original source
     *  o Source drive checkpoint will be at end marker
     */
    if ((source_edge_index == edge_index_to_set)     &&
        (source_drive_checkpoint == FBE_LBA_INVALID)    )
    {
        /* When both the source and destination drives fail we fail the
         * destination drive.
         */
        rebuild_nonpaged_info_p->rebuild_logging_bitmask &= ~(1 << source_edge_index);
        rebuild_nonpaged_info_p->rebuild_logging_bitmask |= (1 << dest_edge_index);
    
        /* Set the destination checkpoint to zero and leave the source checkpoint
         * as is.
         */
        rebuild_nonpaged_info_p->rebuild_checkpoint_info[0].checkpoint = 0;
        rebuild_nonpaged_info_p->rebuild_checkpoint_info[0].position = dest_edge_index;
        rebuild_nonpaged_info_p->rebuild_checkpoint_info[1].checkpoint = FBE_LBA_INVALID;
        rebuild_nonpaged_info_p->rebuild_checkpoint_info[1].position = FBE_RAID_INVALID_DISK_POSITION;
    
        /* Trace some information.
         */
        fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "virtual_drive: copy failed mode: %d life: %d rl_mask: 0x%x chkpts:[0x%llx-0x%llx] source and dest failed \n",
                              configuration_mode, my_state,
                              rebuild_nonpaged_info_p->rebuild_logging_bitmask,
                              rebuild_nonpaged_info_p->rebuild_checkpoint_info[0].checkpoint,
                              rebuild_nonpaged_info_p->rebuild_checkpoint_info[1].checkpoint);
    }
    else if ((dest_edge_index == edge_index_to_set)       &&
             (source_drive_checkpoint == FBE_LBA_INVALID)    )
    {
        /* Else if the copy was aborted (due to the source drive failing) the following 
         * conditions will be true:
         *  o The destination edge is the edge to set
         *  o Source drive checkpoint will be at end marker
         */
        /* Clear rebuild logging for the destination (position to set end marker
         * for) and set it for the source.
         */
        rebuild_nonpaged_info_p->rebuild_logging_bitmask &= ~(1 << dest_edge_index);
        rebuild_nonpaged_info_p->rebuild_logging_bitmask |= (1 << source_edge_index);

        /* Set the source checkpoint to zero and leave the destination checkpoint
         * as is.
         */
        rebuild_nonpaged_info_p->rebuild_checkpoint_info[0].checkpoint = 0;
        rebuild_nonpaged_info_p->rebuild_checkpoint_info[0].position = source_edge_index;
        rebuild_nonpaged_info_p->rebuild_checkpoint_info[1].checkpoint = dest_drive_checkpoint;
        rebuild_nonpaged_info_p->rebuild_checkpoint_info[1].position = dest_edge_index;

        /* Trace some information.
         */
        fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "virtual_drive: copy failed mode: %d life: %d rl_mask: 0x%x chkpts:[0x%llx-0x%llx] source failed \n",
                              configuration_mode, my_state,
                              rebuild_nonpaged_info_p->rebuild_logging_bitmask,
                              rebuild_nonpaged_info_p->rebuild_checkpoint_info[0].checkpoint,
                              rebuild_nonpaged_info_p->rebuild_checkpoint_info[1].checkpoint);
    }

    else
    {
        /* Else something went wrong.
         */
        fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "virtual_drive: copy failed mode: %d life: %d path state:[%d-%d] chkpts:[0x%llx-0x%llx] unexpected\n",
                              configuration_mode, my_state,
                              ((source_edge_index == 0) ? source_edge_path_state : dest_edge_path_state),
                              ((source_edge_index == 1) ? source_edge_path_state : dest_edge_path_state),
                              ((source_edge_index == 0) ? source_drive_checkpoint : dest_drive_checkpoint),
                              ((source_edge_index == 1) ? source_drive_checkpoint : dest_drive_checkpoint));

        /* Perform any neccessary cleanup
         */
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }

    /* Now generate the write size and offset
     */
    metadata_write_size = sizeof(*rebuild_nonpaged_info_p);
    metadata_offset = (fbe_u64_t)(&((fbe_raid_group_nonpaged_metadata_t*)0)->rebuild_info);

    /* Now send the write for the nonpaged.
     */ 
    status = fbe_base_config_metadata_nonpaged_write_persist((fbe_base_config_t*)virtual_drive_p, 
                                                             packet_p, 
                                                             metadata_offset, 
                                                             (fbe_u8_t *)rebuild_nonpaged_info_p,
                                                             metadata_write_size);

    /* Return more processing since a packet is outstanding.
     */
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/***************************************************************************
 * end fbe_virtual_drive_copy_failed_write_checkpoints()
 ***************************************************************************/ 

/*!****************************************************************************
 *          fbe_virtual_drive_copy_failed_set_rebuild_checkpoint_to_end_marker()
 ******************************************************************************
 *
 * @brief   This function is called when a copy job has failed.
 *          It updates both checkpoints of the virtual drive and the rebuild
 *          logging mask.  It simply uses the configuration mode (it does NOT
 *          look at any paths states etc since this virtual drive is not in
 *          pass-thru mode and we have acknowledged the copy completion !).
 *              o Set the previous source edge (now invalid) checkpoint to 0
 *              o Set the new pass-thru edge checkpoint to the end marker
 *              o Mark the old source edge as rebuild logging
 *              o If not rebuild logging on the pass-thru edge clear it
 *
 * @param   virtual_drive_p - Pointer to virtual drive object to set checkpoint for
 * @param   packet_p - pointer to a control packet from the scheduler
 *
 * @return  status - Typically FBE_STATUS_MORE_PROCESSING_REQUIRED
 * 
 * @author
 *  04/14/2013  Ron Proulx  - Created.
 *
 ******************************************************************************/
fbe_status_t fbe_virtual_drive_copy_failed_set_rebuild_checkpoint_to_end_marker(fbe_virtual_drive_t *virtual_drive_p,
                                                                                fbe_packet_t *packet_p)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_edge_index_t                        swapped_out_edge_index = FBE_EDGE_INDEX_INVALID;
    fbe_bool_t                              b_is_active;
    fbe_bool_t                              b_is_edge_swapped;
    fbe_bool_t                              b_is_source_failed;
    fbe_virtual_drive_configuration_mode_t  configuration_mode;

    /* Trace the function entry
     */
    fbe_base_object_trace((fbe_base_object_t*)virtual_drive_p,     
                          FBE_TRACE_LEVEL_DEBUG_HIGH,          
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY, 
                          "%s entry\n", __FUNCTION__); 

    /* Get the configuration mode for debug purposes.
     */
    fbe_virtual_drive_get_configuration_mode(virtual_drive_p, &configuration_mode);

    /* We we are not active (alternate could have taken over?)
     */
    fbe_raid_group_monitor_check_active_state((fbe_raid_group_t *)virtual_drive_p, &b_is_active);
   
    /* Determine if we need to clear the swapped bit or not.
     */
    fbe_virtual_drive_metadata_is_edge_swapped(virtual_drive_p, &b_is_edge_swapped);
    
    /* Determine if the source edge was failed
     */
    fbe_virtual_drive_metadata_is_source_failed(virtual_drive_p, &b_is_source_failed);
      
    /* Get the swapped out edge index.
     */
    swapped_out_edge_index = fbe_virtual_drive_get_swap_out_edge_index(virtual_drive_p);

    /* Trace some information if enabled.
     */
    fbe_virtual_drive_trace(virtual_drive_p,
                            FBE_TRACE_LEVEL_INFO,
                            FBE_VIRTUAL_DRIVE_DEBUG_FLAG_SWAP_TRACING,
                            "virtual_drive: copy failed set chkpt to end active: %d mode: %d swapped: %d swapped edge index: %d\n",
                            b_is_active, configuration_mode, b_is_edge_swapped, swapped_out_edge_index);

    /*! @note Currently we expect this condition to only occur in the Ready 
     *        state and thus we must find an edge index to set.
     */
    status = fbe_virtual_drive_copy_validate_swapped_out_edge_index(virtual_drive_p);
    if (status != FBE_STATUS_OK)
    {
        /* Unexpected swapped out edge index or mode.
         */
        fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s validate swapped index: %d failed - status: 0x%x\n",
                              __FUNCTION__, swapped_out_edge_index, status);

        /* Perform any neccessary cleanup
         */
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return status;
    }

    /* Set the rebuild checkpoint to the end marker on the permanent spare.
     */
    fbe_transport_set_completion_function(packet_p, 
                                          fbe_virtual_drive_metadata_set_checkpoint_to_end_marker_completion, 
                                          virtual_drive_p);    
    status = fbe_raid_group_get_NP_lock((fbe_raid_group_t *)virtual_drive_p, 
                                        packet_p, 
                                        fbe_virtual_drive_copy_failed_write_checkpoints);
        
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/*************************************************************************************
 * end fbe_virtual_drive_copy_failed_set_rebuild_checkpoint_to_end_marker()
 *************************************************************************************/


/************************************
 * end file fbe_virtual_drive_copy.c
 ************************************/
