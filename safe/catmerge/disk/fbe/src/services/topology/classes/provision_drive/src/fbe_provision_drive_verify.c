/******************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ******************************************************************************/

/*!****************************************************************************
 * @file fbe_provision_drive_verify.c
 ******************************************************************************
 *
 * @brief
 *    This file contains the sniff verify code for the provision drive object.
 * 
 * @ingroup provision_drive_class_files
 * 
 * @version
 *   02/16/2010:  Created. Randy Black
 *
 ******************************************************************************/


/*
 * INCLUDE FILES:
 */

#include "fbe/fbe_winddk.h"
#include "fbe/fbe_provision_drive.h"
#include "fbe_provision_drive_private.h"
#include "fbe_database.h"

/*
 * local functions declerations
 */



// verify i/o completion function
static fbe_status_t
fbe_provision_drive_verify_io_completion(fbe_packet_t*    in_packet_p,
										 fbe_packet_completion_context_t   in_context);

// verify post i/o completion function
static fbe_status_t
fbe_provision_drive_verify_post_io_completion(fbe_packet_t*  in_packet_p,
											  fbe_packet_completion_context_t in_context);

//completion function used for the event which asks for verify i/o permission from the virtual drive (VD)
fbe_status_t
fbe_provision_drive_ask_verify_permission_completion(fbe_event_t*                   in_event_p,
                                                     fbe_event_completion_context_t in_context);

fbe_status_t 
fbe_provision_drive_sniff_prepare_for_remap_completion(fbe_packet_t * in_packet_p, 
													   fbe_packet_completion_context_t in_context);

static fbe_status_t 
fbe_provision_drive_verify_reconstruct_paged_metadata(fbe_provision_drive_t* in_provision_drive_p,
                                                      fbe_packet_t*          in_packet_p,
                                                      fbe_lba_t   in_media_error_lba);
static fbe_status_t 
fbe_provision_drive_metadata_reconstruct_with_lock(fbe_packet_t * packet_p,
                                                   fbe_packet_completion_context_t context);
static fbe_status_t
fbe_provision_drive_verify_invalidate_update_paged_metadata_completion(fbe_packet_t * packet_p,
                                                                     fbe_packet_completion_context_t context);
static fbe_status_t
fbe_provision_drive_metadata_incr_verify_invalidate_checkpoint_completion(fbe_packet_t * packet_p,
                                                                          fbe_packet_completion_context_t context);

/*!****************************************************************************
 * fbe_provision_drive_ask_verify_permission_completion
 ******************************************************************************
 *
 * @brief
 *    This is the completion function used for the event which asks for verify
 *    i/o permission from the virtual drive (VD) object.If the permission was granted
 *    check load on downstream.
 *
 * @param   event_p    -  pointer to ask permission event
 * @param   context    -  context, which is a pointer to provision drive
 *
 * @return  fbe_status_t  -  FBE_STATUS_MORE_PROCESSING_REQUIRED
 *                           FBE_STATUS_OK
 *                           
 *
 * @version
 *    03/04/2010 - Created. Randy Black
 *
 ******************************************************************************/

fbe_status_t
fbe_provision_drive_ask_verify_permission_completion(fbe_event_t *event_p,
                                                     fbe_event_completion_context_t context)
{
    fbe_u32_t                       event_flags;
    fbe_event_stack_t*              event_stack_p;
    fbe_event_status_t              event_status;
    fbe_packet_t*                   packet_p;
    fbe_provision_drive_t*          provision_drive_p;
    fbe_status_t                    status;
    fbe_bool_t                      verify_permission;
    fbe_lba_t                       default_offset = FBE_LBA_INVALID;
    fbe_object_id_t             object_id = FBE_OBJECT_ID_INVALID;
    fbe_bool_t                  b_is_system_drive = FBE_FALSE;
    fbe_lba_t                   sniff_checkpoint = FBE_LBA_INVALID;
    fbe_block_count_t blocks;
    fbe_block_count_t unconsumed_blocks = 0;
    fbe_scheduler_hook_status_t hook_status = FBE_SCHED_STATUS_OK;

    // trace function entry
    FBE_PROVISION_DRIVE_TRACE_FUNC_ENTRY( context );

    // cast the context to provision drive
    provision_drive_p = (fbe_provision_drive_t *) context;

    fbe_base_config_get_default_offset((fbe_base_config_t *)provision_drive_p, &default_offset);
    fbe_base_object_get_object_id((fbe_base_object_t *)provision_drive_p, &object_id);
    b_is_system_drive = fbe_database_is_object_system_pvd(object_id);

    // get current entry on event stack 
    event_stack_p = fbe_event_get_current_stack( event_p );
    sniff_checkpoint = event_stack_p->lba;
    blocks = event_stack_p->block_count;
    unconsumed_blocks = event_p->event_data.permit_request.unconsumed_block_count;

    // get event flags, event status, and master packet from event
    fbe_event_get_flags( event_p, &event_flags );
    fbe_event_get_status( event_p, &event_status );
    fbe_event_get_master_packet( event_p, &packet_p );

    // free all associated resources before releasing event
    fbe_event_release_stack( event_p, event_stack_p );
    fbe_event_destroy( event_p );
    //fbe_memory_ex_release(event_p);

    // if our client was busy or verify permission was denied by an
    // upstream object then complete this packet and try again later
    if ( (event_flags & FBE_EVENT_FLAG_DENY) || (event_status == FBE_EVENT_STATUS_BUSY) )
    {
        fbe_provision_drive_check_hook(provision_drive_p,  
                                       SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_SNIFF_VERIFY, 
                                       FBE_PROVISION_DRIVE_SUBSTATE_SNIFF_NO_PERMISSION, 0,  &hook_status);
        /* No need to check status since all we can do here is log.
         */
        fbe_provision_drive_utils_trace( provision_drive_p,
                                         FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_INFO, 
                                         FBE_PROVISION_DRIVE_DEBUG_FLAG_SNIFF_TRACING,
                                         "SNIFF: event status: %d flags: 0x%x", 
                                         event_status, event_flags);
        // set success status in packet and complete packet
        fbe_transport_set_status( packet_p, FBE_STATUS_GENERIC_FAILURE, 0 );
        fbe_transport_complete_packet( packet_p );

        return FBE_STATUS_OK;
    }
    
    // if unable to grant permission for verify i/o due
    // to an error then complete this packet and return
    if ( (event_status != FBE_EVENT_STATUS_OK ) && (event_status != FBE_EVENT_STATUS_NO_USER_DATA))
    {
        // trace error
        fbe_base_object_trace( (fbe_base_object_t *)provision_drive_p,
                                FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "SNIFF: received bad status from client: %d , can't proceed with sniff verify i/o\n",
                                event_status );

        // set failure status in packet and complete packet
        fbe_transport_set_status( packet_p, FBE_STATUS_GENERIC_FAILURE, 0 );
        fbe_transport_complete_packet( packet_p );

        return FBE_STATUS_OK;
    }
    else if (event_status == FBE_EVENT_STATUS_NO_USER_DATA)
    {
        /* Else if the current sniff checkpoint is below the default
         * offset (this should only occur for the system drive) and there
         * there is no user data, advance the sniff checkpoint.
         */ 
        if (sniff_checkpoint < default_offset)
        {
            /* System drives can have areas that are below the default_offset.
             */
            if (b_is_system_drive == FBE_TRUE)
            {
                /* Trace a debug message, set the completion function (done above) and 
                 * invoke method to update the sniff checkpoint to the next chunk.
                 */
                fbe_provision_drive_utils_trace(provision_drive_p,
                                                FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_INFO, 
                                                FBE_PROVISION_DRIVE_DEBUG_FLAG_SNIFF_TRACING,
                                                "SNIFF: chkpt: 0x%llx < def_off: 0x%llx unconsumed: 0x%llx .\n",
                                                sniff_checkpoint, default_offset, unconsumed_blocks);

                if (unconsumed_blocks == FBE_LBA_INVALID)
                {
                    /* If we find that there is nothing else consumed here,
                     * then advance the checkpoint to the end of the default offset to get beyond the unconsumed area. 
                     * We will continue sniffing unbound area beyond the offset.
                     */
                    fbe_provision_drive_metadata_set_sniff_verify_checkpoint(provision_drive_p, packet_p, default_offset);
                    return FBE_STATUS_OK;
                }
                else
                {
                    if(unconsumed_blocks == 0)
                    {
                        /* We should not be getting unconsumed block of Zero*/
                        fbe_provision_drive_utils_trace(provision_drive_p,
                                                        FBE_TRACE_LEVEL_ERROR,
                                                        FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                                                        FBE_PROVISION_DRIVE_DEBUG_FLAG_BGZ_TRACING,
                                                        "SNIFF: Unconsumed of 0 unexp. chk:0x%llx offset:0x%llx\n",
                                                        sniff_checkpoint, default_offset);

                        /* Just move on to the next chunk instead of just getting stuck */
                        unconsumed_blocks += blocks;
                        
                    }

                    /* System drive area is below default_offset and not consumed. Advance
                     * checkpoint to next chunk.
                     */
                    sniff_checkpoint += unconsumed_blocks;
                    fbe_provision_drive_metadata_set_sniff_verify_checkpoint(provision_drive_p, packet_p, sniff_checkpoint);
                    return FBE_STATUS_OK;
                }
            }
            else
            {
                /* It is expected we are unconsumed below the checkpoint.
                 */
                fbe_provision_drive_metadata_set_sniff_verify_checkpoint(provision_drive_p, packet_p, default_offset);
                return FBE_STATUS_OK;
            }
        }
        else
        {
            /* Else there is no upstream object and we are above the default_offset.
             * Issue the sniff. 
             */
        }
    }
     
    status = fbe_provision_drive_get_verify_permission_by_priority(provision_drive_p,&verify_permission);
	                                    
    // terminate current monitor cycle if verify i/o permission not granted
    if ( (status != FBE_STATUS_OK) || (!verify_permission) )
    {
        fbe_provision_drive_utils_trace(provision_drive_p,
                                        FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_INFO, 
                                        FBE_PROVISION_DRIVE_DEBUG_FLAG_SNIFF_TRACING,
                                        "SNIFF: no permission status: 0x%x chkpt: 0x%llx\n",
                                        status, sniff_checkpoint);
		// set success status in packet and complete packet
        fbe_transport_set_status( packet_p, FBE_STATUS_GENERIC_FAILURE, 0 );
        fbe_transport_complete_packet( packet_p );

        return FBE_STATUS_OK;
    }
    fbe_provision_drive_check_hook(provision_drive_p,  
                                   SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_SNIFF_VERIFY, 
                                   FBE_PROVISION_DRIVE_SUBSTATE_SNIFF_SEND_VERIFY, 0,  &hook_status);
    if (hook_status == FBE_SCHED_STATUS_DONE) 
    { 
        fbe_transport_set_status( packet_p, FBE_STATUS_OK, 0 );
        fbe_transport_complete_packet( packet_p );

        return FBE_STATUS_OK;
    }
 
    // now, start next verify i/o on this disk
    fbe_provision_drive_start_next_verify_io( provision_drive_p, packet_p);

    // pend current monitor cycle until this verify i/o completes
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;

}   // end fbe_provision_drive_ask_verify_permission_completion()


/*!****************************************************************************
 * fbe_provision_drive_ask_verify_permission
 ******************************************************************************
 *
 * @brief
 *    This function allocates and initializes a permission request event and
 *    sends it to the virtual drive object to ask for verify i/o permission.
 *
 * @param   in_provision_drive_p  -  pointer to provision drive
 * @param   in_packet_p           -  pointer to control packet from scheduler
 * @param   in_action_priority    -  verify action priority
 *
 * @return  fbe_status_t          -  FBE_STATUS_INSUFFICIENT_RESOURCES
 *                                   FBE_STATUS_OK
 *
 * @version
 *    03/04/2010 - Created. Randy Black
 *
 ******************************************************************************/

fbe_status_t
fbe_provision_drive_ask_verify_permission( fbe_base_object_t* in_object_p,
                                           fbe_packet_t*      in_packet_p  )                                           
{
    fbe_provision_drive_t*          provision_drive_p = NULL;  // pointer to provision drive
    fbe_event_t*                    event_p = NULL;            // pointer to event
    fbe_event_stack_t*              event_stack_p = NULL;      // event stack pointer
    fbe_lba_t                       start_lba = 0;          // starting lba - checkpoint
    fbe_event_permit_request_t      permit_request = {0};
    fbe_medic_action_priority_t     medic_action_priority;
    fbe_lba_t                       next_consumed_lba = FBE_LBA_INVALID;
    fbe_lba_t                       default_offset = FBE_LBA_INVALID;
   
    // trace function entry
    FBE_PROVISION_DRIVE_TRACE_FUNC_ENTRY( in_object_p );

    // cast the base object to provision drive object
    provision_drive_p = (fbe_provision_drive_t *) in_object_p;

    // ask for permission from RAID
    
    // get starting lba for next verify i/o
    fbe_provision_drive_metadata_get_sniff_verify_checkpoint( provision_drive_p, &start_lba );
    fbe_base_object_trace( (fbe_base_object_t *)provision_drive_p,
                           FBE_TRACE_LEVEL_DEBUG_LOW,
                           FBE_TRACE_MESSAGE_ID_INFO,
                           "%s: sniff verify checkpoint is 0x%x \n",
                           __FUNCTION__,
                           (unsigned int)start_lba);

    /* For system RGs the PVD directly connects to RAID. So calculate the next consumed lba
       and set the sniff checkpoint
    */
    fbe_block_transport_server_find_next_consumed_lba(&provision_drive_p->base_config.block_transport_server,
                                                      start_lba,
                                                      &next_consumed_lba);

    fbe_base_config_get_default_offset((fbe_base_config_t *)provision_drive_p, &default_offset);

    if((next_consumed_lba != start_lba) && (start_lba < default_offset))
    {
        fbe_provision_drive_verify_update_sniff_checkpoint(provision_drive_p, in_packet_p, start_lba, next_consumed_lba,
                                                           FBE_PROVISION_DRIVE_CHUNK_SIZE);
        return FBE_STATUS_OK;
    }

    /* Sniff Verifies should be 1MB aligned.   If not, then align to previous LBA.   There are APIs that allow this to be set, so
       this protects against any user errors.
    */
    if ( (start_lba % FBE_PROVISION_DRIVE_CHUNK_SIZE) != 0 )
    {
        fbe_lba_t new_start_lba = (start_lba / FBE_PROVISION_DRIVE_CHUNK_SIZE) * FBE_PROVISION_DRIVE_CHUNK_SIZE;  /* align to previous 1MB*/
        fbe_provision_drive_utils_trace(provision_drive_p,
                                        FBE_TRACE_LEVEL_WARNING,
                                        FBE_TRACE_MESSAGE_ID_INFO, 
                                        FBE_PROVISION_DRIVE_DEBUG_FLAG_SNIFF_TRACING,
                                        "SNIFF: verify_checkpoint 0x%llx, not aligned to 0x%x blks.  Aligning to 0x%llx\n",
                                        start_lba, FBE_PROVISION_DRIVE_CHUNK_SIZE, new_start_lba);

        fbe_provision_drive_metadata_set_sniff_verify_checkpoint(provision_drive_p, in_packet_p, new_start_lba);
        return FBE_STATUS_OK;
    }
    
    // allocate an event
    //event_p = fbe_memory_ex_allocate(sizeof(fbe_event_t));
	event_p = &provision_drive_p->permision_event;

    // if unable to allocate an event then complete
    // this packet and return error status
    if ( event_p == NULL )
    { 
        // trace error
        fbe_base_object_trace( (fbe_base_object_t *)provision_drive_p,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s unable to allocate event\n", __FUNCTION__
                             );

        // set failure status in packet and complete packet
        fbe_transport_set_status( in_packet_p, FBE_STATUS_INSUFFICIENT_RESOURCES, 0 );
        fbe_transport_complete_packet( in_packet_p );

        return FBE_STATUS_INSUFFICIENT_RESOURCES;
    }

    // initialize this event
    fbe_event_init( event_p );

    // set master packet in event
    fbe_event_set_master_packet( event_p, in_packet_p );

    /* setup the verify permit request event. */
    permit_request.permit_event_type = FBE_PERMIT_EVENT_TYPE_VERIFY_REQUEST;
    permit_request.unconsumed_block_count = FBE_PROVISION_DRIVE_CHUNK_SIZE;
    fbe_event_set_permit_request_data(event_p, &permit_request);

    // allocate entry on event stack
    event_stack_p = fbe_event_allocate_stack( event_p );

    /* fill the LBA range to request for the verify operation. */
    fbe_event_stack_set_extent(event_stack_p, start_lba, FBE_PROVISION_DRIVE_CHUNK_SIZE);

    /* Set medic action priority. */
    fbe_medic_get_action_priority(FBE_MEDIC_ACTION_SNIFF, &medic_action_priority);
    fbe_event_set_medic_action_priority(event_p, medic_action_priority);

    // set ask permission completion function
    fbe_event_stack_set_completion_function(event_stack_p,
                                            fbe_provision_drive_ask_verify_permission_completion,
                                            provision_drive_p);


    // now, ask for verify i/o permission from upstream object
    fbe_base_config_send_event( (fbe_base_config_t *)provision_drive_p, FBE_EVENT_TYPE_PERMIT_REQUEST, event_p );

    return FBE_STATUS_OK;

}   // end fbe_provision_drive_ask_verify_permission()


/*!****************************************************************************
 * fbe_provision_drive_classify_io_status
 ******************************************************************************
 *
 * @brief
 *    This function handles classifying the specified transport and block i/o
 *    status (including block qualifier) into a provision drive i/o status.
 *
 * @param   in_transport_status              -  transport status
 * @param   in_block_status                  -  block operation status
 * @param   in_block_qualifier               -  block operation qualifier
 * 
 * @return  fbe_provision_drive_io_status_t  -  provision drive i/o status
 *
 * @version
 *    03/29/2010 - Created. Randy Black
 *
 ******************************************************************************/

fbe_provision_drive_io_status_t
fbe_provision_drive_classify_io_status(
                                        fbe_status_t in_transport_status,
                                        fbe_payload_block_operation_status_t in_block_status,
                                        fbe_payload_block_operation_qualifier_t in_block_qualifier
                                      )
{
    // classify i/o completion status for unexpected errors
    if ( (in_transport_status != FBE_STATUS_OK) ||
         ((in_block_status != FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS) &&
          (in_block_status != FBE_PAYLOAD_BLOCK_OPERATION_STATUS_MEDIA_ERROR)) )
    {
        return FBE_PROVISION_DRIVE_IO_STATUS_ERROR;
    }

    // classify i/o completion status for hard media errors
    if ( in_block_status == FBE_PAYLOAD_BLOCK_OPERATION_STATUS_MEDIA_ERROR )
    {
        return FBE_PROVISION_DRIVE_IO_STATUS_HARD_MEDIA_ERROR;
    }

    // classify i/o completion status for soft media errors
    if ( (in_block_status == FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS) &&
         (in_block_qualifier == FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_REMAP_REQUIRED) )
    {
        return FBE_PROVISION_DRIVE_IO_STATUS_SOFT_MEDIA_ERROR;
    }

    // classify success i/o completion status
    return FBE_PROVISION_DRIVE_IO_STATUS_SUCCESS;

}   // end fbe_provision_drive_classify_io_status()


/*!****************************************************************************
 * fbe_provision_drive_start_next_verify_io
 ******************************************************************************
 *
 * @brief
 *    This function is called  by  the  provision  drive's  monitor  when  the
 *    "do_verify" condition is set. It sends a single sniff
 *    verify i/o request to the provision drive's executor.
 *
 * @param   in_provision_drive_p  -  pointer to provision drive
 * @param   in_packet_p           -  pointer to control packet from scheduler
 * 
 * @return  fbe_status_t          -  status of this operation
 *
 * @version
 *    02/12/2010 - Created. Randy Black
 *
 ******************************************************************************/

fbe_status_t
fbe_provision_drive_start_next_verify_io(fbe_provision_drive_t* in_provision_drive_p,
                                         fbe_packet_t*          in_packet_p)
{
    fbe_block_count_t                     block_count;      // block count
    fbe_payload_block_operation_opcode_t  block_opcode;     // block opcode
    fbe_lba_t                             start_lba;        // starting lba
    fbe_status_t                          status;           // fbe status 

    // trace function entry
    FBE_PROVISION_DRIVE_TRACE_FUNC_ENTRY( in_provision_drive_p );

    // set default block count for next verify i/o (blocks per chunk)
    block_count = FBE_PROVISION_DRIVE_CHUNK_SIZE;

    // get starting lba for next verify i/o
    fbe_provision_drive_metadata_get_sniff_verify_checkpoint( in_provision_drive_p, &start_lba );

    // set block operation opcode for next i/o
    block_opcode = FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_VERIFY;

    // clear retry count for next i/o in verify tracking structure
    //fbe_provision_drive_reset_remap_retry_count( in_provision_drive_p);

    // set verify post i/o completion function
    fbe_transport_set_completion_function(in_packet_p, 
                                          fbe_provision_drive_verify_post_io_completion, 
                                          in_provision_drive_p );

    // set verify i/o completion function
    fbe_transport_set_completion_function(in_packet_p, 
                                          fbe_provision_drive_verify_io_completion, 
                                          in_provision_drive_p );

    // set verify i/o traffic priority
    fbe_transport_set_traffic_priority(in_packet_p, 
                                       in_provision_drive_p->verify_priority );

    // mark this block i/o as a sniff verify i/o 
    fbe_transport_set_packet_attr(in_packet_p, FBE_PACKET_FLAG_BACKGROUND_SNIFF);

    fbe_provision_drive_utils_trace(in_provision_drive_p,
                                    FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_INFO, 
                                    FBE_PROVISION_DRIVE_DEBUG_FLAG_SNIFF_TRACING,
                                    "SNIFF: send verify lba: 0x%llx bl: 0x%llx\n", start_lba, block_count);
    // now, start block i/o on this provision drive
    status = fbe_provision_drive_send_monitor_packet(in_provision_drive_p, 
                                                     in_packet_p, 
                                                     block_opcode, start_lba, block_count );

    return status;
}   // end fbe_provision_drive_start_next_verify_io()

/*!****************************************************************************
 * fbe_provision_drive_set_metadata_sniff_lba_completion
 ******************************************************************************
 *
 * @brief
 *   This function is called after a media error was found by
 *   sniff verify, and media error lba was updated.
 *   
 * @param in_packet_p      - pointer to a control packet from the scheduler
 * @param in_context       - context, which is a pointer to the provision drive
 *                           object
 * 
 * @return  fbe_status_t   - FBE_STATUS_OK
 * 
 * @version
 *    03/01/2012 - Created. Ashok Tamilarasan
 *
 ******************************************************************************/
fbe_status_t 
fbe_provision_drive_set_metadata_sniff_lba_completion(
                                    fbe_packet_t * in_packet_p, 
                                    fbe_packet_completion_context_t in_context)
{
    fbe_provision_drive_t *                 provision_drive_p = NULL;
    fbe_payload_ex_t *                     sep_payload_p = NULL;
    fbe_status_t                            status;  
    fbe_payload_control_operation_t *       control_operation_p = NULL;
    fbe_payload_control_status_t            control_status = FBE_PAYLOAD_CONTROL_STATUS_OK;

    provision_drive_p = (fbe_provision_drive_t *)in_context;

    sep_payload_p = fbe_transport_get_payload_ex(in_packet_p);

    /* Get control operation. */
    control_operation_p = fbe_payload_ex_get_control_operation(sep_payload_p);

    /* Get the packet status */
    status = fbe_transport_get_status_code(in_packet_p);

    // check if checkpoint update completed successfully
    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*)provision_drive_p, 
                                FBE_TRACE_LEVEL_WARNING,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s sniff verify checkpoint update failed, status:%d.",
                                __FUNCTION__, status);
        control_status = FBE_PAYLOAD_CONTROL_STATUS_FAILURE;

        fbe_payload_control_set_status(control_operation_p, control_status);
        fbe_transport_set_status(in_packet_p, status, 0);
		return FBE_STATUS_OK;
    }

    /* At this point we have saved the LBA that took the error, before we kick of 
     * reconstruction we need to get the non-paged lock since we are going to update
     * paged region - this is to avoid race condition when PVD receives control command to mark
     * the region for Zeroing
     */

    status = fbe_provision_drive_get_NP_lock(provision_drive_p, in_packet_p, fbe_provision_drive_metadata_reconstruct_with_lock);
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}

/*!****************************************************************************
 * fbe_provision_drive_metadata_reconstruct_with_lock()
 ******************************************************************************
 * @brief
 *  This function reconstructs the paged metadata if applicable
 *
 * @param in_packet_p      - pointer to a control packet from the scheduler.
 * @param in_context       - context, which is a pointer to the base object.
 * 
 * @return fbe_status_t 
 *
 * @author
 *  03/02/2012 - Created. Ashok Tamilarasan
 *
 ******************************************************************************/
static fbe_status_t 
fbe_provision_drive_metadata_reconstruct_with_lock(fbe_packet_t * packet_p,
                                                   fbe_packet_completion_context_t context)
{
    fbe_provision_drive_t *             provision_drive_p = NULL;    
    fbe_status_t                        status;
    fbe_lba_t                           zero_checkpoint;
    fbe_lba_t                           media_error_lba;

    provision_drive_p = (fbe_provision_drive_t*)context;

    /* If the status is not ok, that means we didn't get the 
       lock. Just return. we are already in the completion function
    */
    status = fbe_transport_get_status_code (packet_p);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*)provision_drive_p, 
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s can't get the NP lock, status: 0x%X", __FUNCTION__, status); 
        return FBE_STATUS_OK;
    }
    

    fbe_transport_set_completion_function(packet_p, fbe_provision_drive_release_NP_lock, provision_drive_p);

    /* At this point we got the lock, but during this period, there could be a request to mark for Zero which 
     * could have updated the region we are going to reconstruct. So if the checkpoint is valid, then
     * return from here as we cannot blindly reconstruct. Return failure so that the 
     * sniff checkpoint are not updated and the next sniff we take care of this
     */
    fbe_provision_drive_metadata_get_background_zero_checkpoint(provision_drive_p,
                                                                &zero_checkpoint);
    if( zero_checkpoint != FBE_LBA_INVALID )
    {
        /* We just cannot proceed at this point. Also return generic failure since we dont want the
         * sniff completion function to advance the checkpoint
         */
        fbe_base_object_trace((fbe_base_object_t*)provision_drive_p, 
                               FBE_TRACE_LEVEL_INFO,
                               FBE_TRACE_MESSAGE_ID_INFO,
                              "%s No-op. Checkpoint:0x%llX is valid.\n",
			      __FUNCTION__,
			      (unsigned long long)zero_checkpoint); 
        fbe_transport_set_status( packet_p, FBE_STATUS_GENERIC_FAILURE, 0 );
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }

    fbe_provision_drive_metadata_get_sniff_media_error_lba(provision_drive_p, &media_error_lba);

    fbe_base_object_trace((fbe_base_object_t*)provision_drive_p, 
                           FBE_TRACE_LEVEL_INFO,
                           FBE_TRACE_MESSAGE_ID_INFO,
                          "%s Reconstruction LBA:0x%llX.\n", __FUNCTION__,
			  (unsigned long long)media_error_lba); 

    /* We are good to go ahead and reconstruct the region*/
    fbe_provision_drive_verify_reconstruct_paged_metadata(provision_drive_p,
                                                          packet_p,
                                                          media_error_lba);
    
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}

/*!****************************************************************************
 * fbe_provision_drive_sniff_prepare_for_remap_completion
 ******************************************************************************
 *
 * @brief
 *   This function is called after a media error was found by
 *   sniff verify, and media error lba was updated.
 *   It sets do_remap condition.
 *   
 * @param in_packet_p      - pointer to a control packet from the scheduler
 * @param in_context       - context, which is a pointer to the provision drive
 *                           object
 * 
 * @return  fbe_status_t   - FBE_STATUS_OK
 * 
 * @version
 *    12/16/2010 - Created. Maya Dagon
 *
 ******************************************************************************/
fbe_status_t 
fbe_provision_drive_sniff_prepare_for_remap_completion(
                                    fbe_packet_t * in_packet_p, 
                                    fbe_packet_completion_context_t in_context)
{
    fbe_provision_drive_t *                 provision_drive_p = NULL;
    fbe_payload_ex_t *                     sep_payload_p = NULL;
    fbe_status_t                            status;  
    fbe_payload_control_operation_t *       control_operation_p = NULL;
    fbe_payload_control_status_t            control_status = FBE_PAYLOAD_CONTROL_STATUS_OK;

    provision_drive_p = (fbe_provision_drive_t *)in_context;

    sep_payload_p = fbe_transport_get_payload_ex(in_packet_p);

    /* Get control operation. */
    control_operation_p = fbe_payload_ex_get_control_operation(sep_payload_p);

    /* Get the packet status */
    status = fbe_transport_get_status_code(in_packet_p);

    // check if checkpoint update completed successfully
    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*)provision_drive_p, 
                                FBE_TRACE_LEVEL_WARNING,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s sniff verify checkpoint update failed, status:%d.",
                                __FUNCTION__, status);
        control_status = FBE_PAYLOAD_CONTROL_STATUS_FAILURE;

        fbe_payload_control_set_status(control_operation_p, control_status);
        fbe_transport_set_status(in_packet_p, status, 0);
		return FBE_STATUS_OK;
    }

    fbe_base_object_trace((fbe_base_object_t*)provision_drive_p, 
                        FBE_TRACE_LEVEL_INFO,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s Setting the send remap cond\n",
                        __FUNCTION__);

	// set "do_remap" lifecycle condition to
    // force remap i/o to run on this chunk
	fbe_lifecycle_set_cond( &fbe_provision_drive_lifecycle_const,
							(fbe_base_object_t *)provision_drive_p,
							FBE_PROVISION_DRIVE_LIFECYCLE_COND_SEND_REMAP_EVENT
						  );

	// now, set status in packet and complete packet - set failure to prevent checkpoint from advancing
    fbe_transport_set_status( in_packet_p, FBE_STATUS_GENERIC_FAILURE, 0 );


	// set generic failure status
    return FBE_STATUS_OK;


}

/*!****************************************************************************
 * fbe_provision_drive_verify_invalidate_remap_completion
 ******************************************************************************
 *
 * @brief
 *    This is the completion function used for the data request event to ask
 *    an upstream object in we can proceed with invalidating the specified
 *    chunk.
 *
 *    The action to perform is based on the event status as follows:
 *
 *     1. FBE_EVENT_STATUS_OK
 *     2. FBE_EVENT_STATUS_NO_USER_DATA
 *          Either the upper level object was okay and so set that we can 
 *     proceed with the invalidate
 *    3. Anything else, we did not get permission and so this request will 
 *       be retried in the next monitor cycle
 *      
 * @param   in_event_p    -  pointer to ask permission event
 * @param   in_context    -  context, which is a pointer to provision drive
 *
 * @return  fbe_status_t  
 *
 * @version
 *    05/10/2012 - Created. Ashok Tamilarasan
 *
 ******************************************************************************/
fbe_status_t
fbe_provision_drive_verify_invalidate_remap_completion(
                                                 fbe_event_t*                   in_event_p,
                                                 fbe_event_completion_context_t in_context
                                               )
{
    fbe_event_stack_t*              event_stack_p;      // event stack pointer
    fbe_event_status_t              event_status;       // event status
    fbe_provision_drive_t*          provision_drive_p;  // pointer to provision drive


    
    FBE_PROVISION_DRIVE_TRACE_FUNC_ENTRY( in_context );

    provision_drive_p = (fbe_provision_drive_t *) in_context;
    event_stack_p = fbe_event_get_current_stack( in_event_p );

    fbe_event_get_status( in_event_p, &event_status );

    // free all associated resources before releasing event
    fbe_event_release_stack( in_event_p, event_stack_p );
    fbe_event_destroy( in_event_p );
    fbe_memory_ex_release(in_event_p); 
    

    /* The event completed and so clear the condition */
    fbe_lifecycle_set_cond( &fbe_provision_drive_lifecycle_const,
                          (fbe_base_object_t *)provision_drive_p,
                          FBE_PROVISION_DRIVE_LIFECYCLE_COND_CLEAR_EVENT_FLAG);

    // dispatch on remap action to perform
    switch ( event_status )
    {
        // RAID has marked the chunk for verify or there is no user data and 
        // both the cases we can proceed
        case FBE_EVENT_STATUS_OK:
        case FBE_EVENT_STATUS_NO_USER_DATA:
            {                
                fbe_base_object_trace( (fbe_base_object_t *)provision_drive_p,
                       FBE_TRACE_LEVEL_DEBUG_HIGH,
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: Setting the remap done condition\n",
                       __FUNCTION__);

                fbe_provision_drive_set_verify_invalidate_proceed(provision_drive_p,
                                                                  FBE_TRUE);
            }
            break;

        default:
            {
                // trace error
                fbe_base_object_trace( (fbe_base_object_t *)provision_drive_p,
                                       FBE_TRACE_LEVEL_WARNING,
                                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                       "%s Event status:%d, cannot proceed\n",
                                       __FUNCTION__, event_status
                                     );
                /* We did not get persmission to proceed and so set it to false */
                fbe_provision_drive_set_verify_invalidate_proceed(provision_drive_p,
                                                                  FBE_FALSE);
                break;
            }
    }

    return FBE_STATUS_OK;
}   // end fbe_provision_drive_verify_invalidate_remap_completion()


/*!****************************************************************************
 * fbe_provision_drive_verify_io_completion
 ******************************************************************************
 *
 * @brief
 *    This is the completion function for a block i/o operation.  It is called
 *    by the provision drive's executor after it finishes a verify i/o.
 *
 * @param   in_packet_p   -  pointer to control packet from the scheduler
 * @param   in_context    -  context, which is a pointer to the base object
 *
 * @return  fbe_status_t  -  status of this operation
 *
 * @version
 *    02/12/2010 - Created. Randy Black
 *
 ******************************************************************************/

fbe_status_t
fbe_provision_drive_verify_io_completion(fbe_packet_t*                   in_packet_p,
                                          fbe_packet_completion_context_t in_context)
{
    fbe_payload_block_operation_t*            block_operation_p;  // block operation pointer
    fbe_payload_block_operation_qualifier_t   block_qualifier;    // block operation qualifier
    fbe_payload_block_operation_status_t      block_status;       // block operation status
    fbe_provision_drive_io_status_t           io_status;          // provision drive i/o status
    fbe_lba_t                                 media_error_lba;    // media error lba
    fbe_lba_t                                 prev_media_error_lba;    // previous media error lba
    fbe_provision_drive_t*                    provision_drive_p;  // pointer to provision drive
    fbe_payload_ex_t*                        sep_payload_p;      // sep payload pointer
    fbe_status_t                              transport_status;   // transport status
    fbe_lba_t                                paged_metadata_start_lba;

    // trace function entry
    FBE_PROVISION_DRIVE_TRACE_FUNC_ENTRY( in_context );

    // cast the context to provision drive object
    provision_drive_p = (fbe_provision_drive_t *) in_context;

    // get verify i/o completion status
    transport_status = fbe_transport_get_status_code(in_packet_p);

    // get verify block operation in sep payload
    sep_payload_p     = fbe_transport_get_payload_ex(in_packet_p);
    block_operation_p = fbe_payload_ex_get_block_operation(sep_payload_p);

    // get verify block i/o status, block qualifier, and media error lba
    fbe_payload_block_get_status( block_operation_p, &block_status );
    fbe_payload_block_get_qualifier( block_operation_p, &block_qualifier );
    fbe_payload_ex_get_media_error_lba( sep_payload_p, &media_error_lba );
    // release verify i/o block operation
    fbe_payload_ex_release_block_operation( sep_payload_p, block_operation_p );

    // classify verify i/o completion status
    io_status = fbe_provision_drive_classify_io_status( transport_status, block_status, block_qualifier );

    // dispatch on verify i/o completion status
    switch ( io_status )
    {
        // verify i/o completed successfully
        case FBE_PROVISION_DRIVE_IO_STATUS_SUCCESS:
            {
                fbe_provision_drive_utils_trace(provision_drive_p,
                                                FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_INFO, 
                                                FBE_PROVISION_DRIVE_DEBUG_FLAG_SNIFF_TRACING,
                                                "SNIFF: verify success lba: 0x%llx bl: 0x%llx\n", 
                                                block_operation_p->lba, block_operation_p->block_count);
                // set success status in packet
                fbe_transport_set_status( in_packet_p, FBE_STATUS_OK, 0 );
            }
            break;

        // verify i/o completed with media errors 
        case FBE_PROVISION_DRIVE_IO_STATUS_HARD_MEDIA_ERROR:
        case FBE_PROVISION_DRIVE_IO_STATUS_SOFT_MEDIA_ERROR:
            {
				fbe_provision_drive_metadata_get_sniff_media_error_lba(provision_drive_p, &prev_media_error_lba);
		
				// if this is the first media error in the chunk - update verify report 
				if (prev_media_error_lba == FBE_LBA_INVALID) {
					// update verify i/o error counts
					fbe_provision_drive_update_verify_report_error_counts( provision_drive_p, io_status );
				}
		
				// trace error in verify i/o operation
				fbe_base_object_trace( (fbe_base_object_t *)provision_drive_p,
									   FBE_TRACE_LEVEL_WARNING,
									   FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
									   "SNIFF: verify i/o fail-media err at lba:0x%llx,prev lba:0x%llx status:0x%x/0x%x\n",
									   (unsigned long long)media_error_lba, (unsigned long long)prev_media_error_lba, transport_status, block_status);

                /* get the paged metadata start lba of the pvd object. */
                fbe_base_config_metadata_get_paged_record_start_lba((fbe_base_config_t *) provision_drive_p,
                                                                    &paged_metadata_start_lba);

                /* Check if the media error LBA is in the paged metadata region. If it is, then
                 * we will have to reconstruct the region. Save the media Error LBA and setup a 
                 * completion function
                 */
                if(media_error_lba >= paged_metadata_start_lba)
                {
                    // set completion function
                    fbe_transport_set_completion_function(in_packet_p, fbe_provision_drive_set_metadata_sniff_lba_completion , provision_drive_p);    
                }
                else
                {
                    // set completion function
                    fbe_transport_set_completion_function(in_packet_p, fbe_provision_drive_sniff_prepare_for_remap_completion , provision_drive_p);    
                }
		
		
				// set media error lba 
				fbe_provision_drive_metadata_set_sniff_media_error_lba( provision_drive_p,in_packet_p, media_error_lba );
		
		
				// return "more processing" since there is more
				// work to perform before completing the packet
				return FBE_STATUS_MORE_PROCESSING_REQUIRED;
		 
            }
            break;

        // verify i/o completed with unexpected errors
        case FBE_PROVISION_DRIVE_IO_STATUS_ERROR:
        default:
            {
                // trace error in verify i/o operation
                fbe_base_object_trace( (fbe_base_object_t *)provision_drive_p,
                                       FBE_TRACE_LEVEL_WARNING,
                                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                       "pvd_verify_io_compl verify i/o fail,stat:0x%x/0x%x\n",
                                       transport_status, block_status);
                
                // set generic failure status and return
                fbe_transport_set_status( in_packet_p, FBE_STATUS_GENERIC_FAILURE, 0 );
            }
            break;
    }
    
    return FBE_STATUS_OK;

}   // end fbe_provision_drive_verify_io_completion()

/*!****************************************************************************
 * fbe_provision_drive_verify_post_io_completion
 ******************************************************************************
 *
 * @brief
 *    This is the post i/o completion function for verify i/o operations. It
 *    advances the verify checkpoint for the next verify i/o
 *
 * @param   in_packet_p   -  pointer to control packet from the scheduler
 * @param   in_context    -  context, which is a pointer to the base object
 * 
 * @return  fbe_status_t  -  status of this operation
 *
 * @version
 *    05/21/2010 - Created. Randy Black
 *
 ******************************************************************************/

fbe_status_t
fbe_provision_drive_verify_post_io_completion(
                                               fbe_packet_t*                   in_packet_p,
                                               fbe_packet_completion_context_t in_context
                                             )
{
    fbe_provision_drive_t*          provision_drive_p;  // pointer to provision drive
    fbe_status_t                    status;             // fbe status

    // trace function entry
    FBE_PROVISION_DRIVE_TRACE_FUNC_ENTRY( in_context );

    // cast the context to provision drive object
    provision_drive_p = (fbe_provision_drive_t *) in_context;

    // get verify i/o completion status
    status = fbe_transport_get_status_code( in_packet_p );

    // if verify i/o completed successfully or a media error occurred on
    // consumed extent then advance checkpoint 
    if ( status == FBE_STATUS_OK )
    {
        // advance verify checkpoint for next verify i/o
       fbe_provision_drive_advance_verify_checkpoint( provision_drive_p,  in_packet_p);

       return FBE_STATUS_MORE_PROCESSING_REQUIRED;       
    }
    
    return status;

}   // end fbe_provision_drive_verify_post_io_completion()


/*!**************************************************************
 * fbe_provision_drive_background_op_is_sniff_need_to_run()
 ****************************************************************
 * @brief
 *  This function checks if sniff is enabled or not to run.
 *
 * @param provision_drive_p           - pointer to the provision drive
 *
 * @return bool status - returns FBE_TRUE if the operation needs to run
 *                       otherwise returns FBE_FALSE
 *
 * @author
 *  07/28/2011 - Created. Amit Dhaduk
 *
 ****************************************************************/
fbe_bool_t
fbe_provision_drive_background_op_is_sniff_need_to_run(fbe_provision_drive_t * provision_drive_p)
{
    fbe_bool_t                                 is_enabled_b;       // sniff verify enable
    fbe_bool_t                                 b_sniff_op_enabled; // sniff background operation enabled
	
    /* if SNIFF background operation is disabled in base config, return FALSE so that in _monitor_operation_cond_function, 
     * it can set upstream edge priority to FBE_MEDIC_ACTION_IDLE.
     */
    fbe_base_config_is_background_operation_enabled((fbe_base_config_t *)provision_drive_p,
                                            FBE_PROVISION_DRIVE_BACKGROUND_OP_SNIFF, &b_sniff_op_enabled);
    if( b_sniff_op_enabled == FBE_FALSE)
    {
        return FBE_FALSE;
    }

    // get current setting of the sniff verify enable flag
    fbe_provision_drive_get_verify_enable( provision_drive_p, &is_enabled_b );

    // if verify is enabled then need to run further sniffing of this disk
    if ( is_enabled_b )
    {
        return FBE_TRUE;
    }

    return FBE_FALSE;
}
/******************************************************************************
 * end fbe_provision_drive_background_op_is_sniff_need_to_run()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_provision_drive_verify_reconstruct_paged_metadata
 ******************************************************************************
 *
 * @brief
 *    This function reconstructs the paged metadata for a chunk which had the 
 *    verify failed. 
 *
 *  @param   in_provision_drive_p  -  pointer to provision drive
 *  @param   in_packet_p   -  pointer to control packet from the scheduler
 *  @param   in_media_error_lba - The LBA that took the error during verify
 * 
 *
 * @return  fbe_status_t  -  status of this operation
 *
 * @notes : At this point when reconstruction is done because of verify
 * there are background operation going on and so we can just reconstruct
 * the data by setting the valid and consumed bits. 
 *
 * @version
 *    02/27/2012 - Created. Ashok Tamilarasan
 *
 ******************************************************************************/
static fbe_status_t 
fbe_provision_drive_verify_reconstruct_paged_metadata(fbe_provision_drive_t* in_provision_drive_p,
                                                      fbe_packet_t*          in_packet_p,
                                                      fbe_lba_t   in_media_error_lba)
{
    fbe_provision_drive_paged_metadata_t paged_set_bits;
    fbe_chunk_index_t chunk_index;
    
    /* Get the Metadata chunk index this LBA is part of.
     * For now, we are reconstucting the entire chunk that got an error, we could be
     * more efficient and reconstuct the block that was just taking an error
     */
    fbe_provision_drive_metadata_get_md_chunk_index_for_lba(in_provision_drive_p,
                                                            in_media_error_lba,
                                                            &chunk_index);
                                                        

    /* Fill in the paged information that we want to write out 
     * This is part of sniff verify and if we get here means, that region was already zeroed
     * because this condition will run only after all zeroing is complete
     * But we dont know if this region was consumed or not and so just to err
     * on the side of caution mark it as consumed
     */
    fbe_zero_memory(&paged_set_bits, sizeof(fbe_provision_drive_paged_metadata_t));
    paged_set_bits.valid_bit = 1;
    paged_set_bits.consumed_user_data_bit = 1;
    

    /* Write out the entire chunk */
    fbe_provision_drive_metadata_write_paged_metadata_chunk(in_provision_drive_p, 
                                                            in_packet_p, 
                                                            &paged_set_bits, 
                                                            chunk_index,
                                                            FBE_TRUE);

    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/*!****************************************************************************
 * @fn fbe_provision_drive_verify_invalidate_update_paged_metadata()
 ******************************************************************************
 * @brief
 *
 *  This function is used to to update the paged metadata after verify and invalidation
 *  operation, after the end of this background 
 * 
 * @param memory_request    - Pointer to memory request.
 * @param context           - Pointer to the provision drive object.
 *
 * @return fbe_status_t.
 *  03/09/2012 - Created. Ashok Tamilarasan
 *
 ******************************************************************************/
fbe_status_t
fbe_provision_drive_verify_invalidate_update_paged_metadata(fbe_provision_drive_t * provision_drive_p,
                                                            fbe_packet_t * packet_p)
{
    fbe_payload_metadata_operation_t *                  metadata_operation_p = NULL;    
    fbe_payload_block_operation_t *                     block_operation_p = NULL;
    fbe_payload_ex_t *                                 sep_payload_p = NULL;
    fbe_status_t                                        status = FBE_STATUS_OK;
    fbe_u64_t                                           metadata_offset;
    fbe_provision_drive_paged_metadata_t *              paged_data_bits_p = NULL;
    fbe_lba_t                                           start_lba;
    fbe_block_count_t                                   block_count;
    fbe_chunk_index_t                                   chunk_index = 0;
    fbe_chunk_index_t                                   start_chunk_index;
    fbe_chunk_count_t                                   chunk_count = 0;
    fbe_provision_drive_paged_metadata_t                paged_bits[FBE_PROVISION_DRIVE_MAX_CHUNKS];
    fbe_lba_t           metadata_start_lba;
    fbe_block_count_t   metadata_block_count;


    /* Get the payload and metadata operation. */
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    metadata_operation_p =  fbe_payload_ex_get_metadata_operation(sep_payload_p);

    /* get the block operation */
    block_operation_p =  fbe_payload_ex_get_present_block_operation(sep_payload_p);

    /* get the start lba and block count. */
    fbe_payload_block_get_lba(block_operation_p, &start_lba);
    fbe_payload_block_get_block_count(block_operation_p, &block_count);

    /* Get the start chunk index and chunk count from the start lba and end lba of the range. */
    fbe_provision_drive_utils_calculate_chunk_range(start_lba,
                                                    block_count,
                                                    FBE_PROVISION_DRIVE_CHUNK_SIZE,
                                                    &start_chunk_index,
                                                    &chunk_count);

    /* get the current paged data bits from the metadata operation. */
    fbe_payload_metadata_get_metadata_record_data_ptr(metadata_operation_p, (void **)&paged_data_bits_p);

    /*  initialize the paged xor bits with zero before we update it. */
    fbe_zero_memory(paged_bits, (sizeof(fbe_provision_drive_paged_metadata_t) * chunk_count));

    chunk_index = 0;
    while(chunk_index < chunk_count)
    {
        paged_bits[chunk_index].valid_bit = 1;
        paged_bits[chunk_index].consumed_user_data_bit = 1;
        chunk_index++;
    }

    /* Release the existing read metadata operation before we send a request for paged metadata update. */
    fbe_payload_ex_release_metadata_operation(sep_payload_p, metadata_operation_p);

    /* get the metadata offset for the chuk index. */
    metadata_offset = start_chunk_index * sizeof(fbe_provision_drive_paged_metadata_t);

    /* allocate new metadata operation to update the paged metadata. */
    metadata_operation_p =  fbe_payload_ex_allocate_metadata_operation(sep_payload_p); 

    /* Set the completion function before issuing paged metadata xor operation. */
    fbe_transport_set_completion_function(packet_p,
                                          fbe_provision_drive_verify_invalidate_update_paged_metadata_completion,
                                          provision_drive_p);

	/* Build the paged metadata set bit request. */
#if 0
	fbe_payload_metadata_build_paged_write_verify(metadata_operation_p,
                                                  &(((fbe_base_config_t *) provision_drive_p)->metadata_element),
                                                  metadata_offset,
                                                  (fbe_u8_t *) paged_bits,
                                                  sizeof(fbe_provision_drive_paged_metadata_t),
                                                  chunk_count);
#else
    fbe_payload_metadata_build_paged_update(metadata_operation_p,
                                              &(((fbe_base_config_t *) provision_drive_p)->metadata_element),
                                              metadata_offset,
                                              sizeof(fbe_provision_drive_paged_metadata_t),
                                              chunk_count,
                                              fbe_provision_drive_metadata_write_verify_paged_metadata_callback,
                                              (void *)metadata_operation_p);
    fbe_payload_metadata_set_metadata_operation_flags(metadata_operation_p, FBE_PAYLOAD_METADATA_OPERATION_PAGED_WRITE_VERIFY);
    /* Initialize cient_blob, skip the subpacket */
    fbe_provision_drive_metadata_paged_init_client_blob(packet_p, metadata_operation_p);
#endif

	fbe_metadata_paged_get_lock_range(metadata_operation_p, &metadata_start_lba, &metadata_block_count);

    fbe_payload_metadata_set_metadata_stripe_offset(metadata_operation_p, metadata_start_lba);
    fbe_payload_metadata_set_metadata_stripe_count(metadata_operation_p, metadata_block_count);

    fbe_payload_ex_increment_metadata_operation_level(sep_payload_p);
    status =  fbe_metadata_operation_entry(packet_p);
    return status;
}
/******************************************************************************
 * end fbe_provision_drive_verify_invalidate_update_paged_metadata()
 ******************************************************************************/

/*!****************************************************************************
 *  fbe_provision_drive_verify_invalidate_update_paged_metadata_completion()
 ******************************************************************************
 * @brief
 *  This function handles the completion of paged metadata update request for
 *  the verify invalidate request.
 *
 * @param packet_p          - pointer to a control packet 
 * @param in_context        - context, which is a pointer to the provision drive
 *                            object
 *
 *
 * @return  fbe_status_t  
 *
 * @author
 *   03/09/2012 - Created. Ashok Tamilarasan
 *
 ******************************************************************************/
static fbe_status_t
fbe_provision_drive_verify_invalidate_update_paged_metadata_completion(fbe_packet_t * packet_p,
                                                                     fbe_packet_completion_context_t context)
{
    fbe_provision_drive_t *             provision_drive_p = NULL;
    fbe_payload_metadata_operation_t *  metadata_operation_p = NULL;
    fbe_payload_block_operation_t *     block_operation_p = NULL;
    fbe_payload_ex_t *                 sep_payload_p = NULL;
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_payload_metadata_status_t       metadata_status;
    fbe_u64_t                           metadata_offset;
    fbe_payload_metadata_operation_opcode_t metadata_opcode;
    fbe_lba_t                               start_lba;
    fbe_block_count_t                       block_count;
    
    provision_drive_p = (fbe_provision_drive_t *) context;

    /* Get the payload, metadata operation operation. */
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    metadata_operation_p =  fbe_payload_ex_get_metadata_operation(sep_payload_p);

    /* Get the status of the paged metadata operation. */
    fbe_payload_metadata_get_status(metadata_operation_p, &metadata_status);
    status = fbe_transport_get_status_code(packet_p);

    /* Get the metadata offset */
    fbe_payload_metadata_get_metadata_offset(metadata_operation_p, &metadata_offset);

    fbe_payload_metadata_get_opcode(metadata_operation_p, &metadata_opcode);

    /* Release the metadata operation. */
    fbe_payload_ex_release_metadata_operation(sep_payload_p, metadata_operation_p);

    /* Get the block operation. */
    block_operation_p = fbe_payload_ex_get_present_block_operation(sep_payload_p);

    if((status != FBE_STATUS_OK) ||
       (metadata_status != FBE_PAYLOAD_METADATA_STATUS_OK))
    {
        /*!@todo Add more error handling for the metadata update failure. */
        fbe_provision_drive_utils_trace(provision_drive_p,
                                        FBE_TRACE_LEVEL_WARNING,
                                        FBE_TRACE_MESSAGE_ID_INFO,
                                        FBE_PROVISION_DRIVE_DEBUG_FLAG_ZOD_TRACING,
                                        "pvd_verify_invalidate_md_update failed, offset:0x%llx,MD status:0x%x,status:0x%x\n",
                                        metadata_offset, metadata_status, status);

        /* Even non-retryable errors back to the monitor must be accompanied 
         * by a state change. Since we are not guaranteed to be getting a state 
         * change (since it might be caused by a quiesce), we must go ahead and 
         * return this as retryable.  If the downstream object is still accessible
         * the monitor will retry the background zero request.
         */
        fbe_payload_block_set_status(block_operation_p,
                                     FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED,
                                     FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_POSSIBLE);
      
        fbe_transport_set_status(packet_p, status, 0);
        return status;
    }

    fbe_payload_block_get_lba(block_operation_p, &start_lba);
    fbe_payload_block_get_block_count(block_operation_p, &block_count);

    /* set the completion function before we update the checkpoint. */
    fbe_transport_set_completion_function(packet_p, fbe_provision_drive_metadata_incr_verify_invalidate_checkpoint_completion, provision_drive_p);

    /* update the checkpoint.
     * Note we pass in what we think the checkpoint is, which is the current lba. 
     * If the lba has moved (such as due to a user zero request), then the checkpoint update will fail. 
     */ 
    status = fbe_provision_drive_metadata_incr_verify_invalidate_checkpoint(provision_drive_p, packet_p, start_lba, block_count);
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}

/*!****************************************************************************
 *  fbe_provision_drive_metadata_incr_verify_invalidate_checkpoint_completion()
 ******************************************************************************
 * @brief
 *  This function handles the completion of the checkpoint update for the verify
 *  invalidate request.
 *
 * @param packet_p          - pointer to a control packet 
 * @param in_context        - context, which is a pointer to the provision drive
 *                            object
 *
 * @return  fbe_status_t  
 *
 * @author
 *   03/09/2012 - Created.  Ashok Tamilarasan
 *
 ******************************************************************************/
static fbe_status_t
fbe_provision_drive_metadata_incr_verify_invalidate_checkpoint_completion(fbe_packet_t * packet_p,
                                                                          fbe_packet_completion_context_t context)
{                      
    fbe_provision_drive_t *             provision_drive_p = NULL;
    fbe_payload_ex_t *                 sep_payload_p = NULL;
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_payload_block_operation_t *     block_operation_p = NULL;
    
    provision_drive_p = (fbe_provision_drive_t *) context;

    /* get the payload and control operation. */
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    
    /* get the status of the paged metadata operation. */
    status = fbe_transport_get_status_code(packet_p);

   /* based on the metadata status set the appropriate block operation status. */
    block_operation_p =  fbe_payload_ex_get_present_block_operation(sep_payload_p);
    if(status != FBE_STATUS_OK)
    {
        fbe_provision_drive_utils_trace(provision_drive_p,
                                        FBE_TRACE_LEVEL_WARNING,
                                        FBE_TRACE_MESSAGE_ID_INFO,
                                        FBE_PROVISION_DRIVE_DEBUG_FLAG_ZOD_TRACING,
                                        "BG_ZERO:nonpaged metadata update failed.\n");

        fbe_payload_block_set_status(block_operation_p,
                                     FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED,
                                     FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_POSSIBLE);
        fbe_transport_set_status(packet_p, status, 0);
        return status;
    }

    /* after successful checkpoint update, complete the background zeroing request. */
    fbe_payload_block_set_status(block_operation_p,
                                 FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS,
                                 FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE);
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_provision_drive_metadata_incr_verify_invalidate_checkpoint_completion()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_provision_drive_verify_update_sniff_checkpoint
 ******************************************************************************
 *
 * @brief
 *   This function calculates the unconsumed blocks and checks whether it is
 *   a system drive or not and updates the checkpoint.
 *
 * @param   provision_drive_p    
 * @param   packet_p
 * @param   sniff_checkpoint
 * @param   next_consumed_lba
 *
 * @return  fbe_status_t     
 *                           
 *
 * @version
 *    04/26/2012 - Created. Ashwin Tamilarasan
 *
 ******************************************************************************/

fbe_status_t
fbe_provision_drive_verify_update_sniff_checkpoint(fbe_provision_drive_t*  provision_drive_p,
                                                   fbe_packet_t*  packet_p,
                                                 fbe_lba_t     sniff_checkpoint,
                                                 fbe_lba_t     next_consumed_lba,
                                                 fbe_block_count_t   blocks)
{
    fbe_lba_t                   unconsumed_blocks = 0;
    fbe_lba_t                   default_offset = FBE_LBA_INVALID;
    fbe_object_id_t             object_id = FBE_OBJECT_ID_INVALID;
    fbe_bool_t                  b_is_system_drive = FBE_FALSE;


    if ((next_consumed_lba != FBE_LBA_INVALID) && (next_consumed_lba > sniff_checkpoint))
    {
        unconsumed_blocks = (next_consumed_lba - sniff_checkpoint);
    }
    else if (next_consumed_lba == FBE_LBA_INVALID)
    {
        unconsumed_blocks = FBE_LBA_INVALID;
    }

    
    fbe_base_config_get_default_offset((fbe_base_config_t *)provision_drive_p, &default_offset);
    fbe_base_object_get_object_id((fbe_base_object_t *)provision_drive_p, &object_id);
    b_is_system_drive = fbe_database_is_object_system_pvd(object_id);

    /* System drives can have areas that are below the default_offset.
     */
    if (b_is_system_drive == FBE_TRUE)
    {
        /* Trace a debug message, set the completion function (done above) and 
         * invoke method to update the sniff checkpoint to the next chunk.
         */
        fbe_provision_drive_utils_trace(provision_drive_p,
                                        FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_INFO, 
                                        FBE_PROVISION_DRIVE_DEBUG_FLAG_SNIFF_TRACING,
                                        "SNIFF: chkpt: 0x%llx < def_off: 0x%llx unconsumed: 0x%llx .\n",
                                        sniff_checkpoint, default_offset, unconsumed_blocks);

        if (unconsumed_blocks == FBE_LBA_INVALID)
        {
            /* If we find that there is nothing else consumed here,
             * then advance the checkpoint to the end of the default offset to get beyond the unconsumed area. 
             * We will continue sniffing unbound area beyond the offset.
             */
            fbe_provision_drive_metadata_set_sniff_verify_checkpoint(provision_drive_p, packet_p, default_offset);
            return FBE_STATUS_OK;
        }
        else
        {
            if(unconsumed_blocks == 0)
            {
                /* We should not be getting unconsumed block of Zero*/
                fbe_provision_drive_utils_trace(provision_drive_p,
                                                FBE_TRACE_LEVEL_ERROR,
                                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                                                FBE_PROVISION_DRIVE_DEBUG_FLAG_BGZ_TRACING,
                                                "SNIFF: Unconsumed of 0 unexp. chk:0x%llx offset:0x%llx\n",
                                                sniff_checkpoint, default_offset);

                /* Just move on to the next chunk instead of just getting stuck */
                unconsumed_blocks += blocks;
                
            }

            /* System drive area is below default_offset and not consumed. Advance
             * checkpoint to next chunk.
             */
            sniff_checkpoint += unconsumed_blocks;
            fbe_provision_drive_metadata_set_sniff_verify_checkpoint(provision_drive_p, packet_p, sniff_checkpoint);
            return FBE_STATUS_OK;
        }
    }
    else
    {
        /* It is expected we are unconsumed below the checkpoint.
         */
        fbe_provision_drive_metadata_set_sniff_verify_checkpoint(provision_drive_p, packet_p, default_offset);
        return FBE_STATUS_OK;
    }


    return FBE_STATUS_OK;

}

/*************************************************************
 * end fbe_provision_drive_verify_update_sniff_checkpoint()
 *************************************************************/
